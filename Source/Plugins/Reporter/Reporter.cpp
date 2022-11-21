/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
// Modeler.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#pragma hdrstop

#include "Reporter.h"

#include <Report.h>
#include <MAP.h>
#include <EnvModel.h>
#include <EnvConstants.h>
#include <PathManager.h>

#include <randgen/Randunif.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

#include <omp.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool BuildPivotTable( Output *pOutput );
bool BuildGroupPivotTable( OutputGroup *pGroup );


//Constant       *FindConstant( ConstantArray &constantArray, LPCTSTR name );
//
//MTDOUBLE TimeFct::evaluate(unsigned int nbArgs, const MTDOUBLE *pArg) { return simTime; }
//MTDOUBLE ExpFct::evaluate(unsigned int nbArgs, const MTDOUBLE *pArg) { return exp(*pArg); }



Pivotable::Pivotable( void ) 
: m_makePivotTable( false )
, m_pPivotData( NULL ) 
   { }


Pivotable::~Pivotable( void )
   {
   if ( m_pPivotData != NULL ) 
      delete m_pPivotData; 
   }


Stratifiable::Stratifiable( void )
: m_colStratifyField( -1 )
, m_pStratifyFieldInfo( NULL )
, m_pStratifyData( NULL )
   { }


Stratifiable::~Stratifiable( void ) 
   {
   if ( m_pStratifyData != NULL ) 
      delete m_pStratifyData; 
   }



int Stratifiable::ParseFieldSpec( LPCTSTR stratifyBy, MapLayer *pLayer )
   {
   ASSERT( stratifyBy != NULL );
   if ( stratifyBy == NULL )
      return -1;

   TCHAR buffer[ 128 ];
   lstrcpy( buffer, stratifyBy );

   m_stratifyByStr = stratifyBy;

   // parse field name if needed - {field} or {field}=[v1,v2,v3] are valid
   TCHAR *p = _tcschr( buffer, '=' );

   if ( p != NULL )  // '=' found?
      {
      *p = NULL;  // then terminate field name and move p ptr to the list
      p++;
      }

   m_stratifyField = buffer;
   m_stratifyField.Trim();

   // have field name, store it and fid corresponding MAP_FIELD_INFO
   //m_pStratifyFieldInfo = pLayer->FindFieldInfo( m_stratifyField );
   MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( m_stratifyField );
   m_colStratifyField    = pLayer->GetFieldCol( m_stratifyField );
   m_pStratifyFieldInfo  = pInfo;

   if ( pInfo == NULL || m_colStratifyField < 0 )
      {
      CString msg;
      msg.Format( "Reporter:  Field Information for field '%s' specified in 'stratify_by' attribute could not be found in the IDU coverage",
                  (LPCTSTR) m_stratifyField );
      Report::ErrorMsg( msg );
      return -1;
      }

   // collect attributes - if not specified, assume all
   if ( p != NULL )  // is a list specified?
      {
      p = _tcschr( p, '[' );
      if ( p == NULL )
         {
         CString msg;
         msg.Format( "Reporter: Syntax error reading '%s': invalid value list specified for field '%s'", 
            stratifyBy, (LPCTSTR) m_stratifyField );
         Report::ErrorMsg( msg );         
         }
      else
         {
         p++;
         TCHAR *end = _tcschr( p, ']');
         
         if ( end == NULL )
            {
            CString msg;
            msg.Format( "Reporter: Syntax error reading '%s': missing list terminator", stratifyBy );
            Report::ErrorMsg( msg );
            return -2;
            }

         *end = NULL;

         CStringArray tokens;
         int count = ::Tokenize( p, ",|", tokens ); 

         ASSERT( count == (int) tokens.GetSize() );
         
         for ( int i=0; i < count; i++  )
            {
            VData v;
            v.Parse( tokens[ i ] );
            v.ChangeType( pInfo->type );
            m_stratifiedAttrs.Add( v );
            }
         }
      }  // end of: if p != NULL
   else
      {  // get all possible values variables for this field from MAP_FIELD_INFO
      ASSERT( pInfo->mfiType == MFIT_CATEGORYBINS );

      for ( int i=0; i < pInfo->GetAttributeCount(); i++ )
         {
         FIELD_ATTR &fa = pInfo->GetAttribute( i );
         if (pInfo->mfiType == MFIT_CATEGORYBINS)
            m_stratifiedAttrs.Add(fa.value);
         else if (pInfo->mfiType == MFIT_QUANTITYBINS)
            m_stratifiedAttrs.Add(VData(fa.maxVal));      // for quantity bins, use max value of bin
         }
      }

   // set up mapping
   int attrCount = (int) m_stratifiedAttrs.GetSize();
   for ( int i=0; i < attrCount; i++ )
      {
      int attrIndex = -1;
      FIELD_ATTR* fa = pInfo->FindAttribute(m_stratifiedAttrs[i], &attrIndex);

      ASSERT( fa != NULL );

      if ( attrIndex >= 0 )
         m_stratifiedLabels.Add( fa->label );
      else
         m_stratifiedLabels.Add( "<Undefined>");

      m_attrMap.SetAt( attrIndex, i );
      }

   return (int) m_stratifiedAttrs.GetSize();
   }


ReportObject::ReportObject( void ) 
: Stratifiable()
, Pivotable()
, m_pMapLayer( NULL )
, m_use( true )
   { }


//-----------------------------------------------------------------------------------------
//----- Output class ----------------------------------------------------------------------
//-----------------------------------------------------------------------------------------

Output::Output() 
: ReportObject()
, m_varType( VT_UNDEFINED )
, m_dataType( TYPE_DOUBLE )
, m_col( -1 )
, m_value( 0 )
, m_cumValue( 0 )
, m_queryArea( 0 )
, m_count( 0 )
, m_pMapExprEngine( NULL )
, m_pMapExpr( NULL )
, m_pGroup( NULL )
   { }


//bool Output::Evaluate( void )
//   {
//   switch( m_varType )
//      {
//      case VT_SUM_AREA:
//      case VT_AREAWTMEAN:
//         //if ( ( m_pParser != NULL ) && ( ! m_pParser->evaluate( m_value ) ) )
//         //   m_value = 0;
//         return true;
//
//      case VT_PCT_AREA:
//         // for pctArea variables, nothing is required
//         //ASSERT( m_pQuery != NULL );
//         return true;
//      }
//
//   return false;
//   }


OutputGroup::OutputGroup( void ) 
: ReportObject(), 
m_pData( NULL )
   { }


OutputGroup::~OutputGroup( void )
   {
   if (m_pData != NULL ) 
      delete m_pData;

	m_outputArray.RemoveAll();
   }


//-----------------------------------------------------------------------------------------
//----- Reporter class --------------------------------------------------------------------
//-----------------------------------------------------------------------------------------

Reporter::Reporter( void )
: EnvModelProcess()
//, m_pMapExprEngine( NULL )
//, m_colArea( -1 )
   { }


Reporter::~Reporter( void )
   {
   m_constArray.RemoveAll();
   m_outputGroupArray.RemoveAll();
   m_outputArray.RemoveAll();
	m_inputArray.RemoveAll();

   //if ( m_pMapExprEngine )
   //   delete m_pMapExprEngine;
   }


bool Reporter::Init( EnvContext *pEnvContext, LPCTSTR initStr ) 
   { 
   EnvModelProcess::Init( pEnvContext, initStr );

   bool ok = LoadXml( pEnvContext, initStr );   // load basics for various reporter objects
   if ( ! ok )
      return FALSE;

   // get date of run
   time_t     now = time(0);
   struct tm  tstruct;
   localtime_s( &tstruct, &now);
   TCHAR      buffer[80];
   // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
   // for more information about date/time format
   strftime(buffer, 80*sizeof(TCHAR), "%Y-%m-%d.%X", &tstruct);
   this->m_startTimeStr = buffer;
   
   // set up ungrouped output vars
   for ( int j=0; j < (int) m_outputArray.GetSize(); j++ )
      {
      Output *pOutput = m_outputArray[ j ];

      if ( pOutput->m_use == false )
         continue;

      if ( pOutput->IsStratified() )
         {
         CString label;
         ///label.Format( "%s by %s", (LPCTSTR) pOutput->m_name, (LPCTSTR) pOutput->m_stratifyField );
         label.Format("%s", (LPCTSTR)pOutput->m_name); // , (LPCTSTR)pOutput->m_stratifyField);

         ASSERT( pOutput->m_pStratifyData != NULL );

         AddOutputVar( label, pOutput->m_pStratifyData, "" );
         }
      else
         AddOutputVar( pOutput->m_name, pOutput->m_value, "" );      
      }

   // set up grouped output varies
   for ( int j=0; j < (int) m_outputGroupArray.GetSize(); j++ )
      {
      OutputGroup *pGroup = m_outputGroupArray[ j ];

      if ( pGroup->m_stratifyByStr.IsEmpty() == false )
         pGroup->ParseFieldSpec( pGroup->m_stratifyByStr, pGroup->m_pMapLayer );

      // set up output data object for this group
      ASSERT( pGroup->m_pData == NULL );
      int cols = 1;  // time
      int varCount = 0;  // variables in use
      for ( int k=0; k < pGroup->GetOutputCount(); k++ )
         if ( pGroup->GetOutput( k )->m_use )
            {
            cols++;
            varCount++;
            }
      
      pGroup->m_pData = new FDataObj( cols, 0, U_YEARS );
      pGroup->m_pData->SetName( pGroup->m_name );
      pGroup->m_pData->SetLabel( 0, "Year");
      
      int col = 1;
      for ( int k=0; k < pGroup->GetOutputCount(); k++ )
         {
         Output *pOutput = pGroup->GetOutput( k );
      
         if ( pOutput->m_use )
            pGroup->m_pData->SetLabel( col++, pOutput->m_name );
         }
      
      // if pivot table specified for this group, add it now
      if ( pGroup->m_makePivotTable )
         BuildGroupPivotTable( pGroup );

      ASSERT( pGroup->m_pData != NULL );

      if ( pGroup->IsStratified() )
         {
         CString label;
         label.Format( "%s by %s", (LPCTSTR) pGroup->m_name, (LPCTSTR) pGroup->m_stratifyField );
         AddOutputVar( label, NULL, "" );

         for ( int k=0; k < (int) pGroup->m_outputArray.GetSize(); k++ )
            {
            Output *pOutput = pGroup->m_outputArray[ k ];
            //AddOutputVar( pOutput->m_name, pOutput->m_value, "" );      
         
            ASSERT( pOutput->m_pStratifyData != NULL );

            if ( pOutput->m_pStratifyData != NULL )  // collecting summarized data?
               {
               CString label;
               //label.Format("%s by %s", (LPCTSTR)pOutput->m_name, (LPCTSTR)pOutput->m_stratifyField);
               label.Format("%s", (LPCTSTR)pOutput->m_name); // , (LPCTSTR)pOutput->m_stratifyField);
               AddOutputVar( label, pOutput->m_pStratifyData, "", 1 );
               }
            }
         }
      else
         {
         ASSERT( pGroup->m_pData != NULL );
         AddOutputVar( pGroup->m_name, pGroup->m_pData, "" );
         }
      }

   // set up inputs
   for ( int j = 0; j < (int) m_inputArray.GetSize(); j++ )
      {
      Input *pInput = m_inputArray[ j ];

      for ( int k = 0; k < (int) m_mapExprEngineArray.GetSize(); k++ )
         { 
         MapExprEngine *pEngine = m_mapExprEngineArray[ k ];
         pEngine->AddVariable( pInput->m_name, &(pInput->m_value) );
         this->AddInputVar( pInput->m_name, pInput->m_value, "" );
         }
      }

   return TRUE;
   }


bool Reporter::InitRun( EnvContext *pEnvContext, bool useInitialSeed )
   {
   // clear out data objs
   for ( int j=0; j < (int) m_outputGroupArray.GetSize(); j++ )
      {
      OutputGroup *pGroup = m_outputGroupArray[ j ];

      if ( pGroup->m_pData == NULL )
         continue;

      pGroup->m_pData->ClearRows();

      for ( int i=0; i < (int) pGroup->m_outputArray.GetSize(); i++ )
         {
         Output *pOutput = pGroup->m_outputArray[ i ];
         
         if ( pOutput->m_pStratifyData != NULL )
            pOutput->m_pStratifyData->ClearRows();
         }
      }

   // clear out summarized data for ungroup Outputs
   for ( int i=0; i < (int) m_outputArray.GetSize(); i++ )
      {
      Output *pOutput = m_outputArray[ i ];
      
      pOutput->m_count = 0;

      if ( pOutput->m_pStratifyData != NULL )
         pOutput->m_pStratifyData->ClearRows();
      }

   Run( pEnvContext );

   return TRUE;
   }


bool Reporter::Run( EnvContext *pEnvContext  )
   {
   //if ( m_pMapExprEngine == NULL || m_colArea < 0 )
   if ( m_mapExprEngineArray.GetSize() == 0 )
      {
      ASSERT( 0 );
      return FALSE;
      }

   // reset outputs
   ResetOutputs( m_outputArray );
   for ( int j=0; j < (int) m_outputGroupArray.GetSize(); j++ )
      {
      OutputGroup *pGroup = m_outputGroupArray[ j ];
      ResetOutputs( pGroup->m_outputArray );
      }

   float totalArea = 0;

   //for each map expression, iterate through it's features
   for ( int m = 0; m < (int) m_mapExprEngineArray.GetSize(); m++ )
      {      
      MapExprEngine *pMapExprEngine = m_mapExprEngineArray[ m ];
      MapLayer *pLayer = pMapExprEngine->GetMapLayer();
      int colArea = m_colAreaArray[ m ];
      
      // iterate over ACTIVE records, evaluating expressions as we go
      for ( MapLayer::Iterator i = pLayer->Begin(); i != pLayer->End(); i++ )
         {
         float area = 0;    // "LENGTH" forline layers
         if ( colArea >= 0 )
            {
            pLayer->GetData( i, colArea, area );
            totalArea += area;
            }

         pMapExprEngine->SetCurrentRecord( i );

         // evaluate any individual outputs
         EvaluateOutputs( m_outputArray, pLayer, i, area );

         // evaluate groups
         for ( int j=0; j < (int) m_outputGroupArray.GetSize(); j++ )
            {
            OutputGroup *pGroup = m_outputGroupArray[ j ];
            EvaluateOutputs( pGroup->m_outputArray, pLayer, i, area );
            } 
         }  // end of:  for each IDU

      // fix up values as needed
      if (totalArea > 0)
         {
         NormalizeOutputs(m_outputArray, pLayer, totalArea);

         for (int j = 0; j < (int)m_outputGroupArray.GetSize(); j++)
            {
            OutputGroup* pGroup = m_outputGroupArray[j];
            NormalizeOutputs(pGroup->m_outputArray, pLayer, totalArea);
            }
         }
      }

   CollectOutput( pEnvContext );  
   return TRUE;
   }

bool Reporter::EndRun( EnvContext* )
   {
   // dump any session (pivot) dataobjects, starting with ungrouped outputs
   for ( int i=0; i < (int) m_outputArray.GetSize(); i++ )
      {
      Output *pOutput = m_outputArray[ i ];
      
      if ( pOutput->m_pPivotData != NULL )
         {
         CString filename;

         //if ( pOutput->IsStratified() )
         //   filename.Format( "%s_by_%s_pivot.csv", (LPCTSTR) pOutput->m_name, (LPCTSTR) pOutput->m_stratifyField );  // (LPCTSTR) m_startTimeStr );
         //else
            filename.Format( "%s_pivot.csv", (LPCTSTR) pOutput->m_name ); //, (LPCTSTR) m_startTimeStr );

         WritePivotData( pOutput->m_pPivotData, filename );
         }
      }

   // next do group pivot data
   for ( int i=0; i < (int) m_outputGroupArray.GetSize(); i++ )
      {
      OutputGroup *pGroup = m_outputGroupArray[ i ];

      if ( pGroup->m_pPivotData )
         {
         CString filename;
         //if ( pGroup->IsStratified() )
         //   filename.Format( "%s_by_%s_pivot.csv", (LPCTSTR) pGroup->m_name, (LPCTSTR) pGroup->m_stratifyField ); //, (LPCTSTR) m_startTimeStr );
         //else
            filename.Format( "%s_pivot.csv", (LPCTSTR) pGroup->m_name ); //, (LPCTSTR) m_startTimeStr );

         WritePivotData( pGroup->m_pPivotData, filename );
         }
      }
   
   return TRUE;
   }


void Reporter::ResetOutputs( PtrArray< Output > &outputArray )
   {
   for ( int k=0; k < (int) outputArray.GetSize(); k++ )
      {
      Output *pOutput = outputArray[ k ];

      if ( pOutput->m_use )
         {
         pOutput->m_value = 0;
         pOutput->m_cumValue = 0;
         pOutput->m_queryArea = 0;

         for ( int j=0; j < (int) pOutput->m_stratifiedValues.GetSize(); j++ )
            pOutput->m_stratifiedValues[ j ] = 0;
         }
      }
   }


 void Reporter::EvaluateOutputs( PtrArray < Output > &outputArray, MapLayer *pLayer, int currentIDU, float area /*or length*/ )
   {
   // for each output in the passed in array, calculate a values for the give IDU and accummulate it
   // ditto for any stratified variables
    MapExprEngine* pMapExprEngine = pLayer->GetMapExprEngine();   // FindMapExprEngine( pLayer );

   if ( pMapExprEngine == NULL )
      return;

   for ( int k=0; k < (int) outputArray.GetSize(); k++ )
      {
      Output *pOutput = outputArray[ k ];

      if ( pOutput->m_use && pOutput->m_pMapLayer == pLayer )
         {
         double value = 0;
         bool ok = pMapExprEngine->EvaluateExpr( pOutput->m_pMapExpr, true );

         if ( ok )  // passed query?
            {
            pOutput->m_queryArea += area;

            switch( pOutput->m_varType )
               {
               case VT_PCT_AREA:
               case VT_SUM_AREA:
               case VT_MEAN:
                  pOutput->m_value += pOutput->m_pMapExpr->GetValue();
                  pOutput->m_count++;
                  break;

               case VT_AREAWTMEAN:
               case VT_LENWTMEAN:
                  pOutput->m_value += area * pOutput->m_pMapExpr->GetValue();
                  break;

               } // end of: switch()

            // are we collecting sumarized data?
            if ( pOutput->IsStratified() )
               {
               VData iduValue;
               pLayer->GetData( currentIDU, pOutput->m_colStratifyField, iduValue );

               // have value from IDU - get the corresponding FIELD_ATTR - this is
               int attrIndex = -1;
               pOutput->m_pStratifyFieldInfo->FindAttribute( iduValue, &attrIndex );

               if ( attrIndex >= 0 )
                  {
                  // have the correct field (bin), need to map this into the corresponding index in the m_stratifiedValues array
                  int index = -1;
                  BOOL found = pOutput->m_attrMap.Lookup( attrIndex, index );

                  if ( found && index >= 0 )
                     {
                     switch( pOutput->m_varType )
                        {
                        case VT_PCT_AREA:
                        case VT_SUM_AREA:
                        case VT_MEAN:
                           pOutput->m_stratifiedValues[ index ] += (float) pOutput->m_pMapExpr->GetValue();
                           break;

                        case VT_AREAWTMEAN:
                        case VT_LENWTMEAN:
                           pOutput->m_stratifiedValues[ index ] += float(  area * pOutput->m_pMapExpr->GetValue() );
                           break;
                        } // end of: switch()
                     }
                  }
               }  // end of: if ( pOutput->m_pStratifyData != NULL )
            } // end of: if ( ok )
         }  // end of: if (pOutput->m_use)
      }  // end of: for each output
   }  


void Reporter::NormalizeOutputs( PtrArray < Output > &outputArray, MapLayer *pLayer, float totalArea )
   {
   for ( int k=0; k < (int) outputArray.GetSize(); k++ )
      {
      Output *pOutput = outputArray[ k ];

      if ( pOutput->m_use && pOutput->m_pMapLayer == pLayer )
         {
         switch( pOutput->m_varType )
            {
            case VT_PCT_AREA:
               pOutput->m_value *= pOutput->m_queryArea / totalArea;
               break;

            case VT_SUM_AREA:
               break;      // nothing extra needed
          
            case VT_AREAWTMEAN:
               if ( pOutput->m_queryArea > 0 )
                  pOutput->m_value /= pOutput->m_queryArea;  // normalize to query area
               break;

            case VT_LENWTMEAN:
               if (pOutput->m_queryArea > 0)
                  pOutput->m_value /= pOutput->m_queryArea;  // normalize to query area
               break;

            case VT_MEAN:
               if ( pOutput->m_count > 0 )
                  pOutput->m_value /= pOutput->m_count;
            }

         // are we collecting sumarized data?
         if ( pOutput->m_pStratifyData != NULL )
            {
            for ( int i=0; i < pOutput->m_stratifiedValues.GetSize(); i++ )
               {
               switch( pOutput->m_varType )
                  {
                  case VT_PCT_AREA:
                     pOutput->m_stratifiedValues[ i ] *= pOutput->m_queryArea / totalArea;
                     break;
         
                  case VT_SUM_AREA:
                     break;      // nothing extra needed
          
                  case VT_AREAWTMEAN:
                  case VT_LENWTMEAN:
                  case VT_MEAN:
                     if ( pOutput->m_queryArea > 0 )
                        pOutput->m_stratifiedValues[ i ] /= pOutput->m_queryArea;  // normalize to query area
                     break;
                  } // end of: switch()
               }
            }  // end of: if ( pOutput->m_pStratifyData != NULL )
         }
      }
   }


void Reporter::CollectOutput( EnvContext *pContext )
   {
   float time = (float) pContext->currentYear;

   CString scname = "Unknown";
   if ( pContext->pEnvModel->m_pScenario != NULL )
	   scname=pContext->pEnvModel->m_pScenario->m_name;    // Scenario
   scname.Replace( ',', '_');

   // First, collect Output var data in FDataObjs
   // note - this only applies to output groups, since they are the only FDataObjs
   for ( int j=0; j < (int) m_outputGroupArray.GetSize(); j++ )
      {
      OutputGroup *pGroup = m_outputGroupArray[ j ];

      if ( pGroup->m_pData == NULL )
         continue;

      CArray< float, float > data;
      data.Add( time );
      
      for ( int k=0; k < pGroup->GetOutputCount(); k++ )
         {
         Output *pOutput = pGroup->GetOutput( k );

         if ( pOutput->m_use )
            data.Add( (float) pOutput->m_value );
         }

      pGroup->m_pData->AppendRow( data );
      }

   // Next, collect summarized data
   // - first, ungrouped outputs
   for ( int j=0; j < (int) m_outputArray.GetSize(); j++ )
      {
      Output *pOutput = m_outputArray[ j ];

      if ( pOutput->IsStratified() )
         {
         int count = (int) pOutput->m_stratifiedValues.GetSize() + 1;
         
         CArray< float, float > data;
         data.SetSize( count );
         
         data[ 0 ] = (float) pContext->currentYear;
         
         for ( int k=1; k < count; k++ )
            data[ k ] = pOutput->m_stratifiedValues[ k-1 ];

         pOutput->m_pStratifyData->AppendRow( data );
         }
      }

   // then, grouped outputs
   for ( int j=0; j < (int) m_outputGroupArray.GetSize(); j++ )
      {
      OutputGroup *pGroup = m_outputGroupArray[ j ];

      for ( int k=0; k < pGroup->GetOutputCount(); k++ )
         {
         Output *pOutput = pGroup->GetOutput( k );
   
         if ( pOutput->IsStratified() )
            {
            int count = (int) pOutput->m_stratifiedValues.GetSize() + 1;
            
            CArray< float, float > data;
            data.SetSize( count );
            
            data[ 0 ] = (float) pContext->currentYear;
            
            for ( int k=1; k < count; k++ )
               data[ k ] = pOutput->m_stratifiedValues[ k-1 ];

            pOutput->m_pStratifyData->AppendRow( data );
            }
         }
      }

   // Next, collect any pivot table data.  Note that this may be stored at the group or individual output level
   // - first, do pivot tables defined for an ungrouped Output
   for ( int j=0; j < (int) m_outputArray.GetSize(); j++ )
      {
      Output *pOutput = m_outputArray[ j ];

      if ( pOutput->m_pPivotData == NULL )
         continue;

      // add to existing dataObj
      //           0      1   2      3        4          
      // cols = Scenario/Run/Year/Version/TimeStamp/(StrataCol?)/VarName

      int colCount = pOutput->m_pPivotData->GetColCount();
      CArray< VData, VData > rowData;
      
      rowData.SetSize( colCount );
      ASSERT( colCount > 4 );
      rowData[ 0 ].AllocateString( scname );                      // Scenario
      rowData[ 1 ] = pContext->pEnvModel->m_currentRun;           // Run
      rowData[ 2 ] = pContext->pEnvModel->m_currentYear;          // Year
      rowData[ 3 ] = 0;  // pContext->pEnvModel->m_version;       // Version
      rowData[ 4 ] = (LPTSTR)(LPCTSTR) this->m_startTimeStr;      // Time

      // if using summaries, we will add a record for each attribute; otherswise, only one record need
      if ( pOutput->IsStratified() ) 
         {
         int attrCount = pOutput->GetStratifiedAttrCount();
         for ( int k=0; k < attrCount; k++ )
            {
            int col = 5;
            rowData[ col++ ] = pOutput->m_stratifiedAttrs[ k ];
            rowData[ col++ ] = pOutput->m_stratifiedLabels[ k ];
            rowData[ col++ ] = pOutput->m_stratifiedValues[ k ];
            pOutput->m_pPivotData->AppendRow( rowData );
            }
         }  // end of: if ( useSummaries)
      else
         {  // not using summaries - only one record needed, no attribute col required
         //int col = 5;
         rowData[ 5 ] = pOutput->m_value;
         pOutput->m_pPivotData->AppendRow( rowData );
         }  // end of: else (not using summaries)
      }

   // next, do pivot tables for any output groups with pivots
   for ( int j=0; j < (int) m_outputGroupArray.GetSize(); j++ )
      {
      OutputGroup *pGroup = m_outputGroupArray[ j ];

      if ( pGroup->m_pPivotData == NULL )
         continue;

      // add to existing dataObj
      // cols = Scenario/Run/Year/Version/TimeStamp/(StrataCol?)/VarName/Repeat for n output variables
      bool useSummaries =  pGroup->m_stratifyField.GetLength() > 0 ? true : false;
      
      int colCount = pGroup->m_pPivotData->GetColCount();
      
      CArray< VData, VData > rowData;
      rowData.SetSize( colCount );
      
      rowData[ 0 ].AllocateString( scname );                      // Scenario
      rowData[ 1 ] = pContext->pEnvModel->m_currentRun;           // Run
      rowData[ 2 ] = pContext->pEnvModel->m_currentYear;          // Year
      rowData[ 3 ] = 0;  // pContext->pEnvModel->m_version;       // Version
      rowData[ 4 ] = (LPTSTR)(LPCSTR) this->m_startTimeStr;       // Time

      // if using summaries, we will add a record for each attribute; otherswise, only one record need
      if ( useSummaries ) 
         {
         int attrCount = pGroup->GetStratifiedAttrCount(); 
         
         for ( int k=0; k < attrCount; k++ )
            {
            int col = 5;
            rowData[ col++ ] = pGroup->m_stratifiedAttrs[ k ];
            rowData[ col++ ] = pGroup->m_stratifiedLabels[ k ];

            for ( int l=0; l < pGroup->GetOutputCount(); l++ )
               {
               Output *pOutput = pGroup->GetOutput( l );
               rowData[ col++ ] = pOutput->m_stratifiedValues[ k ];
               }

            ASSERT( pGroup->m_pPivotData != NULL );
            pGroup->m_pPivotData->AppendRow( rowData );
            }
         }  // end of: if ( useSummaries)
      else
         {  // not using summaries - only one record needed, no attribute col required
         int col = 5;
         for ( int k=0; k < pGroup->GetOutputCount(); k++ )
            {
            Output *pOutput = pGroup->GetOutput( k );
            rowData[ col++ ] = pOutput->m_value;
            }

         ASSERT( pGroup->m_pPivotData != NULL );
         pGroup->m_pPivotData->AppendRow( rowData );
         }  // end of: else (not using summaries)
      }
   }


bool Reporter::WritePivotData( VDataObj *pData, LPCTSTR filename )
   {
   // if pivot output exists for this object, then dump it to disk.
   // filename is {tablename}_{startTime}.csv
   // before we dump this, we need to make sure columns don't have any commas
   for ( int j=0; j < pData->GetColCount(); j++ )
      {
      CString label( pData->GetLabel( j ));
      if ( label.Replace( ',', ' ' ) > 0 )
         pData->SetLabel( j, label );
      }

   CString path = PathManager::GetPath( PM_IDU_DIR );  // this gets to a specific scenario
   path += "Outputs\\";
   CleanFileName( filename );  // replace illegal chars with '_'
   
   path += filename;

   pData->WriteAscii( path );
   return true;
   }


Constant *Reporter::FindConstant( LPCTSTR name )
   {
   for ( int i=0; i < m_constArray.GetSize(); i++ )
      {
      Constant *pConst = m_constArray[ i ];
      if ( pConst->m_name.Compare( name ) == 0 )
         return pConst;
      }

   return NULL;
   }


OutputGroup *Reporter::FindOutputGroup( LPCTSTR name )
   {
   for ( int i=0; i < m_outputGroupArray.GetSize(); i++ )
      {
      OutputGroup *pOutputGroup = m_outputGroupArray[ i ];

      if ( pOutputGroup->m_name.Compare( name ) == 0 )
         return pOutputGroup;
      }

   return NULL;
   }


bool Reporter::SetConfig(EnvContext *pEnvContext, LPCTSTR configStr)
   {
   m_configStr = configStr;

   TiXmlDocument doc;
   doc.Parse(configStr);
   TiXmlElement *pRoot = doc.RootElement();

   bool ok = LoadXml(pRoot, pEnvContext);

   return ok ? TRUE : FALSE;
   }


bool Reporter::LoadXml(EnvContext *pEnvContext, LPCTSTR filename)
   {
   CString path;
   int result = PathManager::FindPath(filename, path);
   if (PathManager::FindPath(filename, path) < 0) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format("Reporter: Input file '%s' not found - this process will be disabled", filename);
      Report::ErrorMsg(msg);
      return false;
      }

   m_configFile = (LPCTSTR) path;

   FileToString(path, m_configStr);

   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.Parse(m_configStr.c_str());

   if (!ok)
      {
      CString msg;
      msg.Format("Reporter: Error reading input file %s:  %s", filename, doc.ErrorDesc());
      Report::ErrorMsg(msg);
      return false;
      }

   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // <reporter>
   return LoadXml(pXmlRoot, pEnvContext);
   }


bool Reporter::LoadXml(TiXmlElement *pXmlRoot, EnvContext *pEnvContext)
   {
   LPTSTR layer = NULL;
   XML_ATTR rootAttrs[] = {
         // attr        type          address            isReq  checkCol
         { NULL,       TYPE_NULL,     NULL,              false, 0 } };

   bool ok = TiXmlGetAttributes( pXmlRoot, rootAttrs, m_configFile.c_str(), NULL );

   MapLayer *pIDULayer = (MapLayer*) pEnvContext->pMapLayer;

   //------------------- <const> ------------------
   TiXmlElement *pXmlConst = pXmlRoot->FirstChildElement( "const" ); 

   while( pXmlConst != NULL )
      {
      Constant *pConst = new Constant;
      XML_ATTR constAttrs[] = {
         // attr        type          address            isReq  checkCol
         { "name",     TYPE_CSTRING,  &(pConst->m_name),  true, 0 },
         { "value",    TYPE_CSTRING,  &(pConst->m_value), true, 0 },
         { NULL,       TYPE_NULL,     NULL,              false, 0 } };

      ok = TiXmlGetAttributes( pXmlConst, constAttrs, m_configFile.c_str(), NULL );
      if ( ok )
         m_constArray.Add( pConst );
      else
         delete pConst;

      pXmlConst = pXmlConst->NextSiblingElement( "const" );
      }

   //------------------- <input> ------------------
   TiXmlElement *pXmlInput = pXmlRoot->FirstChildElement( "input" ); 

   while( pXmlInput != NULL )
      {
      Input *pInput = new Input;
      float value;
      XML_ATTR inputAttrs[] = {
         // attr        type          address            isReq  checkCol
         { "name",     TYPE_CSTRING,  &(pInput->m_name),  true, 0 },
         { "value",    TYPE_FLOAT,    &value,             true, 0 },
         { NULL,       TYPE_NULL,     NULL,              false, 0 } };

      ok = TiXmlGetAttributes( pXmlInput, inputAttrs, m_configFile.c_str(), NULL );
      if ( ok )
         {
         // add it to the list
         pInput->m_value = (MTDOUBLE) value;
         m_inputArray.Add( pInput );

         ///////m_pMapExprEngine->AddVariable( pInput->m_name, &(pInput->m_value) );
         ///////this->AddInputVar( pInput->m_name, pInput->m_value, "" );
         }
      else
         delete pInput;

      pXmlInput = pXmlInput->NextSiblingElement( "input" );
      }

   // outputs not associated with a groups
   if ( LoadXmlOutputs( pXmlRoot, NULL, pIDULayer, pEnvContext->pQueryEngine, m_configFile.c_str()) > 0 )
      {
      CString msg;
      msg.Format( "Reporter: Loaded %i ungrouped outputs", (int) m_outputArray.GetSize() );
      Report::Log( msg );
      }

   //------------------- <outputgroup> ------------------
   TiXmlElement *pXmlOutputGroup = pXmlRoot->FirstChildElement( "output_group" ); 

   while( pXmlOutputGroup != NULL )
      {
      OutputGroup *pGroup = new OutputGroup;

      LPTSTR groupBy=NULL;
      XML_ATTR groupAttrs[] = {
         // attr            type          address                       isReq  checkCol
         { "name",          TYPE_CSTRING,  &(pGroup->m_name),           true,  0 },
         { "stratify_by",   TYPE_CSTRING,  &(pGroup-> m_stratifyByStr), false, 0 },
         { "pivot_table",   TYPE_INT,      &(pGroup->m_makePivotTable), false, 0 },
         { "layer",         TYPE_CSTRING,  &(pGroup->m_mapLayerName),   false, 0 },
         { NULL,            TYPE_NULL,     NULL,                        false, 0 } };

      ok = TiXmlGetAttributes( pXmlOutputGroup, groupAttrs, m_configFile.c_str());
      if ( ! ok )
         {
         delete pGroup;
         continue;
         }

      if (groupBy != NULL)
         pGroup->m_stratifyByStr = groupBy;

      // map layer not specfied?  then use the IDU layer as default
      if ( pGroup->m_mapLayerName.IsEmpty() )
         pGroup->m_mapLayerName = pIDULayer->m_name;

      pGroup->m_pMapLayer = pIDULayer->m_pMap->GetLayer( pGroup->m_mapLayerName );
      if ( pGroup->m_pMapLayer == NULL )
         {
         CString msg;
         msg.Format( "Reporter: Map Layer '%s' not found - this layer is referred to by the '%s' output group.  This output group will be ignored...", (LPCTSTR) pGroup->m_mapLayerName, (LPCTSTR) pGroup->m_name );
         Report::LogError( msg );
         pGroup->m_use = false;
         }
 
      m_outputGroupArray.Add( pGroup );

      // get outputs
      LoadXmlOutputs( pXmlOutputGroup, pGroup, pIDULayer, pEnvContext->pQueryEngine, m_configFile.c_str());

      CString msg;
      msg.Format( "Reporter: Loaded Output Group '%s' (%i outputs)", (LPCTSTR) pGroup->m_name, pGroup->GetOutputCount() );
      Report::Log( msg );

      pXmlOutputGroup = pXmlOutputGroup->NextSiblingElement( "output_group" );
      }

   return true;
   }


int Reporter::LoadXmlOutputs( TiXmlElement *pXmlParent, OutputGroup *pGroup,  MapLayer *pIDULayer, QueryEngine *pQueryEngine, LPCTSTR filename )
   {
   TiXmlElement *pXmlOutput = pXmlParent->FirstChildElement( "output" ); 

   int count = 0;

   while( pXmlOutput != NULL )
      {
      Output *pOutput = new Output;

      bool pivotTable = false;
      if ( pGroup )
         pivotTable = pGroup->m_makePivotTable;

      if ( pGroup && pGroup->m_stratifyByStr.GetLength() > 0 )
         pOutput->m_stratifyByStr = pGroup->m_stratifyByStr;

      pOutput->m_pGroup = pGroup;

      LPCTSTR type = NULL, groupByStr = NULL;
      //CString stratifyBy;

      XML_ATTR outputAttrs[] = {
         // attr                type          address                   isReq  checkCol
         { "name",             TYPE_CSTRING,  &(pOutput->m_name),           true,  0 },
         { "query",            TYPE_CSTRING,  &(pOutput->m_query),          false, 0 },
         { "value",            TYPE_CSTRING,  &(pOutput->m_expression ),    true,  0 },
         { "type" ,            TYPE_STRING,   &type,                        true,  0 },
         { "group_by",         TYPE_STRING,   &groupByStr,                  false, 0 },
         { "stratify_by",      TYPE_CSTRING,  &(pOutput->m_stratifyByStr),  false, 0 },
         { "pivot_table",      TYPE_BOOL,     &(pOutput->m_makePivotTable), false, 0 },
         { "layer",            TYPE_CSTRING,  &(pOutput->m_mapLayerName),   false, 0 },
         { NULL,               TYPE_NULL,     NULL,                         false, 0 } };
   
      bool ok = TiXmlGetAttributes( pXmlOutput, outputAttrs, filename );
      if ( ok )
         {
         // map layer identified?
         if ( pOutput->m_mapLayerName.IsEmpty() )
            pOutput->m_mapLayerName = pIDULayer->m_name;

         pOutput->m_pMapLayer = pIDULayer->m_pMap->GetLayer( pOutput->m_mapLayerName );
         if ( pOutput->m_pMapLayer == NULL )
            {
            CString msg;
            msg.Format( "Reporter: Map Layer '%s' not found - this layer is referred to by the '%s' output.  This output will be ignored...", (LPCTSTR) pOutput->m_mapLayerName, (LPCTSTR) pOutput->m_name );
            Report::LogError( msg );
            pOutput->m_use = false;
            }

         if (groupByStr != NULL)
            pOutput->m_stratifyByStr = groupByStr;

         switch (type[0])
            {
            case 's':
            case 'S':      pOutput->m_varType = VT_SUM_AREA;     break;    // "sum"
            case 'p':      
            case 'P':      pOutput->m_varType = VT_PCT_AREA;     break;    // "pctArea"
            case 'a':
            case 'A':      pOutput->m_varType = VT_AREAWTMEAN;   break;    // "areaWtMean"
            case 'l':
            case 'L':      pOutput->m_varType = VT_LENWTMEAN;   break;     // "lenWtMean"
            case 'm':
            case 'M':      pOutput->m_varType = VT_MEAN;   break;          // "mean"
            //case 'd':
            //case 'D':      pOutput->m_varType = VT_DELTA;   break;          // "mean"
            //case 'f':
            //case 'F':      pOutput->m_varType = VT_FRACTION;   break;          // "mean"

            default:
               {
               CString msg;
               msg.Format( "Reporter: Unrecognized 'type' attribute for output '%s'- must be 'sum', 'pctArea', 'mean', lenWtMean, or 'areaWtMean'.  This output will be ignored...", (LPCTSTR) pOutput->m_name );

               Report::LogError( msg );
               pOutput->m_use = false;
               }
            }

         CString expr = pOutput->m_expression;
         for ( int i=0; i < (int) m_constArray.GetSize(); i++ )
            {
            Constant *pConst = m_constArray[ i ];
            expr.Replace( pConst->m_name, pConst->m_value );
            }

         // allocate a map expr engine if needed
         if ( pOutput->m_pMapLayer != NULL )
            {
            MapExprEngine *pEngine = FindMapExprEngine( pOutput->m_pMapLayer );
            if ( pEngine == NULL )
               m_mapExprEngineArray.Add(pOutput->m_pMapLayer->GetMapExprEngine());  // pEngine = new MapExprEngine(pOutput->m_pMapLayer, pQueryEngine) );

            if (pOutput->m_pMapLayer->GetType() == LT_LINE)
               m_colAreaArray.Add(pOutput->m_pMapLayer->GetFieldCol("LENGTH"));
            else if (pOutput->m_pMapLayer->GetType() == LT_POINT)
               m_colAreaArray.Add(-1);
            else
               m_colAreaArray.Add( pOutput->m_pMapLayer->GetFieldCol( "AREA"));
            //   }

            //pOutput->m_pQueryEngine = pOutput->m_pMapLayer->GetQueryEngine();
            pOutput->m_pMapExprEngine = pOutput->m_pMapLayer->GetMapExprEngine();
            pOutput->m_pMapExpr = pOutput->m_pMapExprEngine->AddExpr( pOutput->m_name, expr, pOutput->m_query );
         
            if ( pOutput->m_pMapExpr == NULL )
               {
               CString msg;
               msg.Format( "Reporter: Invalid 'value' expression '%s' encountered for output '%s' This output will be ignored...", (LPCTSTR) pOutput->m_expression, (LPCTSTR) pOutput->m_name );
            
               Report::ErrorMsg( msg );
               pOutput->m_use = false;
               }
            else
               {
               CString source;
               source.Format( "Reporter.%s", pOutput->m_name );
               pOutput->m_pMapExpr->Compile( source );
               }

            if ( pGroup == NULL )
               m_outputArray.Add( pOutput );
            else
               pGroup->AddOutput( pOutput );
            }

         // includes summarize-by spec?, make an approrpriate dataobj with the individual summaries,
         // one for each unique attribute value for the specified column
         bool useSummaries = false;
         if ( pOutput->m_use && pOutput->m_stratifyByStr.GetLength() > 0 && pOutput->m_pMapLayer->GetFieldCol( pOutput->m_stratifyByStr ) )
            {
            int fields = pOutput->ParseFieldSpec( pOutput->m_stratifyByStr, pOutput->m_pMapLayer );
         
            // get unique elements from field information
            if ( fields >= 0 )
               {
               useSummaries = true;
               int cols = 1 + pOutput->GetStratifiedAttrCount();        // time + slot for each attribute
               
               ASSERT( cols > 1 );
               pOutput->m_pStratifyData = new FDataObj( cols, 0, U_YEARS);
               pOutput->m_stratifiedValues.SetSize( pOutput->GetStratifiedAttrCount() );
         
               CString name;
               if ( pGroup != NULL )
                  name.Format("%s-%s", (LPCTSTR)pGroup->m_name, (LPCTSTR)pOutput->m_name); // , (LPCTSTR)pOutput->m_stratifyField);
                  //name.Format("%s-%s_by_%s", (LPCTSTR)pGroup->m_name, (LPCTSTR)pOutput->m_name, (LPCTSTR)pOutput->m_stratifyField);
               else
                  name.Format("%s", (LPCTSTR)pOutput->m_name);// , (LPCTSTR)pOutput->m_stratifyField );
                  //name.Format("%s_by_%s", (LPCTSTR)pOutput->m_name, (LPCTSTR)pOutput->m_stratifyField);

               pOutput->m_pStratifyData->SetName( name );
               pOutput->m_pStratifyData->SetLabel( 0, "Year" );
         
               for ( int i=1; i < cols; i++ )
                  pOutput->m_pStratifyData->SetLabel( i, pOutput->m_stratifiedLabels[i-1] );
               }  // data obj set up, but nothing else
            }
         
         // includes pivot_table spec and NOT in a group?
         if ( pGroup == NULL && pOutput->m_makePivotTable )
            BuildPivotTable( pOutput );

         count++;
         }  // end of: if ( ok = TiXmlGetAttributes( pXmlOutput, outputAttrs, filename, pLayer );
      else
         delete pOutput;

      pXmlOutput = pXmlOutput->NextSiblingElement( "output" );
      }

   return count;
   }


bool BuildPivotTable( Output *pOutput )
   {
   int cols = 6;
   if ( pOutput->IsStratified() )
      cols += 2;

   pOutput->m_pPivotData = new VDataObj( cols, 0, U_UNDEFINED);

   CString name;
   //if ( pOutput->IsStratified() )
   //   name.Format( "%s_by_%s", (LPCTSTR) pOutput->m_name, (LPCTSTR) pOutput->m_stratifyField );
   //else
      name = pOutput->m_name;
   
   int col = 0;
   pOutput->m_pPivotData->SetName( name );
   pOutput->m_pPivotData->SetLabel( col++, "Scenario");
   pOutput->m_pPivotData->SetLabel( col++, "Run");
   pOutput->m_pPivotData->SetLabel( col++, "Year");
   pOutput->m_pPivotData->SetLabel( col++, "Version");
   pOutput->m_pPivotData->SetLabel( col++, "Timestamp");

   if ( pOutput->IsStratified() )
      {
      CString label( pOutput->m_stratifyField );
      label += "_value";
      pOutput->m_pPivotData->SetLabel( col++, label );

      label = pOutput->m_stratifyField;
      label += "_label";
      pOutput->m_pPivotData->SetLabel( col++, label );
      }

   pOutput->m_pPivotData->SetLabel( col, pOutput->m_name );

   return true;
   }



bool BuildGroupPivotTable( OutputGroup *pGroup )
   {
   bool useSummaries =  pGroup->IsStratified();

   int cols = 5 + pGroup->GetOutputCount();
   if ( useSummaries )
      cols += 2;

   pGroup->m_pPivotData = new VDataObj( cols, 0, U_UNDEFINED);
   
   CString name;
   if ( useSummaries )
      name.Format( "%s_by_%s", (LPCTSTR) pGroup->m_name, (LPCTSTR) pGroup->m_stratifyField );
   else
      name = pGroup->m_name;
   
   int col = 0;
   pGroup->m_pPivotData->SetName( name );
   pGroup->m_pPivotData->SetLabel( col++, "Scenario");
   pGroup->m_pPivotData->SetLabel( col++, "Run");
   pGroup->m_pPivotData->SetLabel( col++, "Year");
   pGroup->m_pPivotData->SetLabel( col++, "Version");
   pGroup->m_pPivotData->SetLabel( col++, "Timestamp");
   if ( useSummaries )
      {
      CString label( pGroup->m_stratifyField );
      label += "_value";
      pGroup->m_pPivotData->SetLabel( col++, label );

      label = pGroup->m_stratifyField;
      label += "_label";
      pGroup->m_pPivotData->SetLabel( col++, label );
      }

   for ( int i=0; i < pGroup->GetOutputCount(); i++ )
      pGroup->m_pPivotData->SetLabel( col++, pGroup->GetOutput( i )->m_name );

   return true;
   }


bool Reporter::ExportSourceCode( LPCTSTR className, LPCTSTR fileName )
   {
   // header file first
   CString hdrFile = fileName;
   hdrFile += ".h";
   
   ofstream file;
   file.open( hdrFile );

   if ( ! file )
      return false;

   // more needed
   return true;
   }


bool Reporter::GenerateEvalCode( LPCTSTR className, stringstream &evalCode )
   {
   /*
   //MapLayer *pLayer = m_pMapExprEngine->GetMapLayer();
   //ASSERT( m_pMapLayer == pLayer );

   evalCode.clear();

   // first, output constants
   //  e.g.   const float m2perHa = 10000f;

   evalCode << "   // constants" << endl;

   for ( int i=0; i < (int) this->m_constArray.GetSize(); i++ )
      {
      Constant *pConst = m_constArray[ i ];

      evalCode << "   const float " << pConst->m_name << " = " << pConst->m_value << "f;" << endl;
      }

   // next, referenced field names
   // e.g.  float AREA = 0;
   //       int colAREA = pLayer->Get
   //       pLayer->GetData( idu, cols[0], AREA );
   /////int colCount = m_pMapExprEngine->GetUsedColCount();
   
   evalCode << endl << "   // referenced field names" << endl;

   for ( int i=0; i < colCount; i++ )
      {
      int col = m_pMapExprEngine->GetUsedCol( i );
      ASSERT( col >= 0 );

      LPCTSTR field = pLayer->GetFieldLabel( col );
      //int     col   = pLayer->GetFieldCol( field );
      
      evalCode << "   " << field << " = 0;" << endl;
      evalCode << "   pLayer->GetData( idu, cols[ i ], " << field << " );" << endl << endl;
      }

   // next ungrouped output variables
   // e.g.   
   //    float WUI_AREA__ha_0 = AREA/m2PerHa;
   //    values[ 0 ] = WUI_AREA__ha_0;
   evalCode << endl << "   // variables" << endl << endl;
   evalCode << endl << "   // ungrouped outputs" << endl;

   int count = 0;
   for (int i=0; i < (int) m_outputArray.GetSize(); i++ )
      {
      Output *pOutput = m_outputArray[ i ];

      CString name = pOutput->m_name;
      LegalizeName( name );
      evalCode << "   float " << name << "_" << count << " = " << pOutput->m_expression << ";" << endl;

      count++;
      }

   // next grouped output variables
   for ( int j=0; j < (int) m_outputGroupArray.GetSize(); j++ )
      {
      OutputGroup *pGroup = m_outputGroupArray[ j ];

      CString groupName = pGroup->m_name;
      LegalizeName( groupName );

      for (int i=0; i < (int) m_outputArray.GetSize(); i++ )
         {
         Output *pOutput = m_outputArray[ i ];
   
         CString name = pOutput->m_name;
         LegalizeName( name );
         evalCode << "   float " << groupName << "." << name << "_" << count << " = " << pOutput->m_expression << ";" << endl;
   
         count++;
         }
      } */
   return TRUE;
   }


bool Reporter::LegalizeName( CString &name )
   {
   name.Replace( ' ', '_' );
   name.Replace( '(', '_' );
   name.Replace( ')', '_' );
   return TRUE;
   }


/*
// autogenerated function
void Reporter::GetOutputsValues( MapLayer *pLayer, int cols[], int idu, float values[] )
   {
   //@GetEvalOutputs
   return;
   }
   // referenced field names
   float AREA = 0;
   pLayer->GetData( idu, cols[0], AREA );

   float HARVESTVOL = 0;
   pLayer->GetData( idu, cols[ 1 ], HARVESTVOL );


   // output expressions
   float WUI_AREA__ha_0 = AREA/m2PerHa;
   values[ 0 ] = WUI_AREA__ha_0;

   float Harvest__m3_1 = HARVESTVOL*AREA/m2PerHa;
   values[ 1 ] = Harvest__m3_1;

   etc...

   */
