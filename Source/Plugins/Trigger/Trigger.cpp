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
// Trigger.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "trigger.h"
#include <deltaarray.h>
#include <randgen\randunif.hpp>
#include <Envmodel.h>
#include <tinyxml.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


RandUniform rn( 0.0, 1.0, 1 );

double *TriggerProcessCollection::m_parserVars = NULL;
MapLayer *TriggerProcessCollection::m_pMapLayer = NULL;
QueryEngine *TriggerProcessCollection::m_pQueryEngine = NULL;

CMap<int, int, int, int > TriggerProcessCollection::m_usedParserVars;



bool TriggerOutcome::Compile(  const MapLayer *pLayer )
   {
//   ASSERT( m_pParser == NULL );
//   ASSERT( ! m_targetExpr.IsEmpty()  );

   // NOTE: ASSUMES all evaluable expressions/field have been defined
//   ASSERT( m_pParser == NULL );
//   m_pParser = new MTParser;

   m_parser.enableAutoVarDefinition( true );
   m_parser.compile( m_targetExpr );

   int varCount = m_parser.getNbUsedVars();

   for ( int i=0; i < varCount; i++ )
      {
      bool found = false;

      CString varName = m_parser.getUsedVar( i ).c_str();

      // is it a legal field in the map?
      int col = pLayer->GetFieldCol( varName );
      if ( col >= 0 )
         {
         // have we already seen this column?
         int index = col;
         if ( TriggerProcessCollection::m_usedParserVars.Lookup( col, index ) )
            {
            ASSERT( col == index );
            }
         else  // add it
            TriggerProcessCollection::m_usedParserVars.SetAt( col, col );

         m_parser.redefineVar( varName, TriggerProcessCollection::m_parserVars+index );
         continue;
         }

      // if we get this far, the variable is undefiend
      CString msg( "Unable to compile Trigger expression for Trigger value: " );
      msg += m_targetExpr;
      msg +="... This expression will be ignored";
      AfxMessageBox( msg );
      return false;
      }  // end of:  for ( i < varCount )

   return true;
   }


bool TriggerOutcome::Evaluate()
   {
   // for expressions, evualte the expression with the parser.  Assumes all variables defined, current
   //ASSERT( m_pParser != NULL );
   double value;
   if ( ! m_parser.evaluate( value ) )
      value = 0;

   m_value = (float) value;
   return true;
   }


Trigger::~Trigger()
   { 
   if ( m_pQuery != NULL )
      TriggerProcessCollection::m_pQueryEngine->RemoveQuery( m_pQuery );
   }

   
bool Trigger::CompileQuery()
   {
   ASSERT( TriggerProcessCollection::m_pQueryEngine != NULL );
   ASSERT( m_pQuery == NULL );

   if ( m_queryStr.IsEmpty() )
      return true;

   CString source( "Trigger Process Query" );

   m_pQuery = TriggerProcessCollection::m_pQueryEngine->ParseQuery( m_queryStr, 0, source );
   
   if ( m_pQuery == NULL )
      {
      CString msg( "Unable to compile Trigger query for expression:  " );
      msg += m_queryStr;
      msg +="... This expression will be ignored";
      AfxMessageBox( msg );
      return false;
      }

   return true;
   }


TriggerProcess::~TriggerProcess( void )
   {
   for ( int i=0; i < m_triggerArray.GetSize(); i++ )
      delete m_triggerArray[ i ];

   m_triggerArray.RemoveAll();
   }


// this method reads an xml input file
bool TriggerProcess::LoadXml( LPCTSTR filename, const MapLayer *pLayer )
   {
   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   bool loadSuccess = true;

   if ( ! ok )
      {
      CString msg;
      msg.Format("Error reading input file, %s", filename );
      AfxGetMainWnd()->MessageBox( doc.ErrorDesc(), msg );
      return false;
      }

  /* start iterating through the document
   * general structure is of the form:
   *
   * <?xml version="1.0" encoding="utf-8"?>
   *
   * <triggers>
   *    <trigger query="STAND_AGE < 10" use='0'>
   *      <outcome target_col="STAND_AGE" target_value="4" probability="20" />
   *      <outcome target_col="STAND_AGE" target_value="5" probability="80" />
   *    </trigger>
   * </triggers>
   */

   TiXmlElement *pXmlRoot = doc.RootElement();  // <triggers  ....>

   // get any sync_maps
   TiXmlElement *pXmlTrigger = pXmlRoot->FirstChildElement( _T("trigger") );
   while( pXmlTrigger != NULL )
      {
      Trigger *pTrigger = new Trigger;

      XML_ATTR triggerAttrs[] = {
         // attr                type          address                     isReq  checkCol
         { _T("name"),       TYPE_CSTRING,   &(pTrigger->m_name),       true,   0 },
         { _T("query"),      TYPE_CSTRING,   &(pTrigger->m_queryStr),   true,  0 },
         { _T("use"),        TYPE_BOOL   ,   &(pTrigger->m_use),        false,  0 },
         { NULL,             TYPE_NULL,      NULL,                      false,  0 } };

      ok = TiXmlGetAttributes(pXmlTrigger, triggerAttrs, filename, NULL);
      if (!ok)
         {
         CString msg;
         msg.Format(_T("Flow: Misformed root element reading <trigger> attributes in input file %s"), filename);
         Report::ErrorMsg(msg);
         delete pTrigger;
         continue;
         }

      bool ok = pTrigger->CompileQuery();
      if ( ! ok )
         {
         CString msg;
         msg.Format( _T("Unable to compile target query %s.  This trigger will be ignored" ), (LPCTSTR) pTrigger->m_queryStr );
         Report::ErrorMsg( msg );
         delete pTrigger;
         continue;
         }

      // have trigger, get outcomes
      const TiXmlElement *pXmlOutcome = pXmlTrigger->FirstChildElement( _T("outcome") );
      while( pXmlOutcome != NULL )
         {
         LPCTSTR targetCol   = pXmlOutcome->Attribute(_T("target_col") );
         LPCTSTR targetValue = pXmlOutcome->Attribute(_T("target_value") );
         LPCTSTR probability = pXmlOutcome->Attribute( _T("probability") );

         if ( targetCol == NULL || targetValue == NULL )
            {
            CString msg;
            msg.Format( _T("Trigger <outcome> tag is missing a required attribute (target_col or target_value) for query %s.  This outcome will be ignored." ), pTrigger->m_queryStr);
            Report::ErrorMsg( msg );
            pXmlOutcome = pXmlOutcome->NextSiblingElement( _T("outcome" ));
            }

         TriggerOutcome *pOutcome = new TriggerOutcome;
         pOutcome->m_colTarget = pLayer->GetFieldCol( targetCol );
         if ( pOutcome->m_colTarget < 0 )
            {
            CString msg;
            msg.Format( _T("<outcome> target_column %s not found in IDU coverage when reading <trigger> %s" ), targetCol, pTrigger->m_queryStr);
            Report::ErrorMsg( msg );
            delete pOutcome;
            pXmlOutcome = pXmlOutcome->NextSiblingElement( _T("outcome" ));
            continue;
            }

         pOutcome->m_targetExpr = targetValue;

         bool ok = pOutcome->Compile( pLayer );
         if ( ! ok )
            {
            CString msg;
            msg.Format( _T("Unable to compile <outcome> target_value %s when reading <outcome> %s" ), targetValue, pTrigger->m_queryStr);
            Report::ErrorMsg( msg );
            delete pOutcome;
            pXmlOutcome = pXmlOutcome->NextSiblingElement( _T("outcome" ));
            continue;
            }

         pTrigger->m_outcomeArray.Add( pOutcome );
         
         if ( probability != NULL )
             pOutcome->m_probability = 1.0f;
         else
            {
            float p = (float) atof( probability );
            if ( p > 1.01f )   // fix values that are (0-100)
               p /= 100; 

            pOutcome->m_probability = p;
            }

         pXmlOutcome = pXmlOutcome->NextSiblingElement( _T("outcome" ) );
         }  // end of: while (pXmlOutcome != NULL )

      this->m_triggerArray.Add( pTrigger );

      CString msg;
      msg.Format( _T("Loaded trigger for query %s, (%i outcomes)\n"), 
         pTrigger->m_queryStr, pTrigger->m_outcomeArray.GetSize() );
      Report::LogInfo( msg );

      pXmlTrigger = pXmlTrigger->NextSiblingElement( _T("trigger") );
      }  // end of: while ( pXmlTrigger != NULL )

   m_filename = filename;
   return true;
   }

   
bool TriggerProcess::Run( EnvContext *pContext )
   {
   // Basic idea:
   //
   const MapLayer *pLayer = pContext->pMapLayer;
                        
   //bool ok = pTrigger->Evaluate();
   
   int triggerCount = (int) this->m_triggerArray.GetSize();
   for ( MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++ )
      {
      for ( int j=0; j < triggerCount; j++ )
         {
         Trigger *pTrigger = this->m_triggerArray[ j ];

         if ( pTrigger->m_use == false )
            continue;

         bool useThisTrigger = true;
         if ( pTrigger->m_pQuery != NULL )
            {
            bool result;
            bool ok = pTrigger->m_pQuery->Run( idu, result );
            if ( ! ok || ! result )
               useThisTrigger = false;
            }
         
         if ( useThisTrigger )
            {
            // trigger satisified, randomly get an outcome      
            float randVal = (float) rn.RandValue();  // 0-1
            float value = 0;
            int   i = 0;
            TriggerOutcome *pOutcome = NULL;

            for ( ; i < pTrigger->m_outcomeArray.GetSize(); i++ )
                {
                pOutcome = pTrigger->m_outcomeArray[ i ];

                value += pOutcome->m_probability;
                if ( value >= randVal )
                    break;
                }

            if ( j < pTrigger->m_outcomeArray.GetSize() )
               {
               TriggerProcessCollection::UpdateVariables( idu ); // for parser
               bool ok = pOutcome->Evaluate();

               if ( ok )
                  pContext->ptrAddDelta( pContext->pEnvModel, idu, pOutcome->m_colTarget, 
                           pContext->currentYear, pOutcome->m_value, pContext->handle );
               }
         
            break;   // move on to next IDU
            }  // end of: if ( useThisTrigger )
         }  // end of: for( j < triggerCount )
      }  // end of:  for ( idu < iduCount )
    
   return true;
   }
   

TriggerProcessCollection::~TriggerProcessCollection()
   {
   for ( int i=0; i < m_triggerProcessArray.GetCount(); i++ ) 
      delete m_triggerProcessArray.GetAt( i ); 
    
   m_triggerProcessArray.RemoveAll();

   if ( m_parserVars != NULL )
      {
      delete m_parserVars;
      m_parserVars = NULL;
      }
   }


bool TriggerProcessCollection::Init( EnvContext *pEnvContext, LPCTSTR  initStr /*xml input file*/ )
   {
   CString string( initStr );
   ASSERT( ! string.IsEmpty() );

   const MapLayer *pLayer = pEnvContext->pMapLayer;

   if ( this->m_triggerProcessArray.GetSize() == 0 )
      {
      // take care of static stuff if needed
      m_pMapLayer = (MapLayer*) pLayer;

      // make a query engine if one doesn't exist already
      if (m_pQueryEngine == NULL)
         m_pQueryEngine = pEnvContext->pQueryEngine;

      // allocate field vars if not already allocated
      ASSERT( m_parserVars == NULL );
      int fieldCount = pLayer->GetFieldCount();
      m_parserVars = new double[ fieldCount ];
      }

   TriggerProcess *pProcess = new TriggerProcess( pEnvContext->id );
   if ( pProcess == NULL )
      {
      CString msg( _T("Unable to create TriggerProcess instance " ) );
      msg += pEnvContext->pEnvExtension->m_name ;      
      Report::ErrorMsg( msg );
      return FALSE;
      }

   // load XML input file specified in initStr
   bool ok = pProcess->LoadXml( initStr, pLayer );

   if ( ! ok )
      return FALSE;

   this->m_triggerProcessArray.Add( pProcess );

   // expose input variable

   for (int i = 0; i < (int) pProcess->m_triggerArray.GetSize(); i++ )
      {
      Trigger *pTrigger = pProcess->m_triggerArray[i];

      // add output variables
      CString varName(pTrigger->m_name);
      varName += ".InUse";
      AddInputVar(varName, pTrigger->m_use, "Use this Trigger for the given scenario");
      }

   return TRUE;
   }
   

bool TriggerProcessCollection::Run( EnvContext *pEnvContext )
   {
   const MapLayer *pLayer = pEnvContext->pMapLayer;
 
   int index;
   TriggerProcess *pProcess = GetTriggerProcessFromID( pEnvContext->id, &index );
   ASSERT( pProcess != NULL );

   bool ok = pProcess->Run( pEnvContext );

   return ok ? TRUE : FALSE;
   }

   
TriggerProcess *TriggerProcessCollection::GetTriggerProcessFromID( int id, int *index )
   {
   for ( int i=0; i < m_triggerProcessArray.GetSize(); i++ )
      {
      TriggerProcess *pProcess = m_triggerProcessArray.GetAt( i );

      if ( pProcess->m_processID == id )
         {
         if ( index != NULL )
            *index = i;

         return pProcess;
         }
      }

   if ( index != NULL )
      *index = -1;

   return NULL;
   }


bool TriggerProcessCollection::UpdateVariables( int idu )
   {
   VData value;
   float fValue;
   int colCount = m_pMapLayer->GetFieldCount();

   for ( int j=0; j < colCount; j++ )
      {
      bool okay = m_pMapLayer->GetData( idu, j, value );

      if ( okay && value.GetAsFloat( fValue ) )
         m_parserVars[ j ] = fValue;
      }

   return true;
   }
