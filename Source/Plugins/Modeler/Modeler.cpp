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

#include "modeler.h"
#include <map.h>
#include <envmodel.h>
#include <DeltaArray.h>
#include <PathManager.h>
#include <randgen\randunif.hpp>

#include <limits.h>
#include <omp.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// idu field variable values shared by all parsers
Modeler     *gpModeler = nullptr;

int          Modeler::m_nextID = 0;
int          Modeler::m_colArea = -1;

Variable *gpCurrentVar = nullptr;
int gCurrentIDU  = -1;


double simTime = 0;

RandUniform rn( 0, 1 );
RandUniform rn1( 0, 1 );

bool CompileExpression( ModelBase *pModel, ModelCollection *pCollection, MParser *pParser, LPCTSTR expr, MapLayer *pLayer );
bool LoadLookup( TiXmlNode *pXmlNode, LookupArray&, MapLayer* );
bool LoadConstant( TiXmlNode *pXmlNode, ConstantArray&, bool isInput  );
bool LoadVariable( TiXmlNode *pXmlNode, VariableArray&, bool isReport );
bool LoadExpr    ( TiXmlNode *pXmlNode, ModelBase* );
bool LoadHistograms( TiXmlNode *pXmlNode, HistoArray&, MapLayer* ); //, LPCTSTR defaultName=nullptr, LPCTSTR defaultUse=nullptr );
bool ParseUseDelta( LPCTSTR deltaStr, CArray< USE_DELTA, USE_DELTA& > &useDeltaArray );

HistogramArray *FindHistogram( HistoArray &histogramsArray, LPCTSTR name );
LookupTable    *FindLookup( LookupArray &lookupTableArray, LPCTSTR name );
Constant       *FindConstant( ConstantArray &constantArray, LPCTSTR name );
Variable       *FindVariable( VariableArray &variableArray, LPCTSTR name );

MTDOUBLE TimeFct::evaluate(unsigned int nbArgs, const MTDOUBLE *pArg) { return simTime; }
MTDOUBLE ExpFct::evaluate(unsigned int nbArgs, const MTDOUBLE *pArg) { return exp(*pArg); }

MTDOUBLE GetNeighborCountFct::evaluate(unsigned int nbArgs, const MTDOUBLE *pArg)
   {
   if ( gCurrentIDU < 0 )
      return -1;

   int neighbors[ 32 ];
   Poly *pPoly = gpModeler->m_pMapLayer->GetPolygon( gCurrentIDU );
   int count = gpModeler->m_pMapLayer->GetNearbyPolys( pPoly, neighbors, nullptr, 32, 0.001f );

   int retCount = 0;

   //solve with an associate query?
   if ( gpCurrentVar != nullptr && gpCurrentVar->m_pValueQuery != nullptr )
      {
      for ( int i=0; i < count; i++ )
         {
         int idu = neighbors[ i ];
         // check to see if neighbor passes query

         bool result = false;
         if (gpModeler->m_pQueryEngine->Run( gpCurrentVar->m_pValueQuery, idu, result ) && result == true )
            retCount++;         
         }

      count = retCount;
      }
         
   return (MTDOUBLE) count;
   }


HistogramArray *FindHistogram( HistoArray &histogramsArray, LPCTSTR name )
   {
   for ( int i=0; i < histogramsArray.GetSize(); i++ )
      {
      HistogramArray *pHistos = histogramsArray[ i ];
      if ( pHistos->m_name.Compare( name ) == 0 )
         return pHistos;
      }

   return nullptr;
   }

LookupTable *FindLookup( LookupArray &lookupTableArray, LPCTSTR name )
   {
   for ( int i=0; i < lookupTableArray.GetSize(); i++ )
      {
      LookupTable *pTable = lookupTableArray[ i ];
      if ( pTable->m_name.Compare( name ) == 0 )
         return pTable;
      }

   return nullptr;
   }

Constant *FindConstant( ConstantArray &constantArray, LPCTSTR name )
   {
   for ( int i=0; i < constantArray.GetSize(); i++ )
      {
      Constant *pConst = constantArray[ i ];
      if ( pConst->m_name.Compare( name ) == 0 )
         return pConst;
      }

   return nullptr;
   }

Variable *FindVariable( VariableArray &variableArray, LPCTSTR name )
   {
   for ( int i=0; i < variableArray.GetSize(); i++ )
      {
      Variable *pVar = variableArray[ i ];
      if ( pVar->m_name.Compare( name ) == 0 )
         return pVar;
      }

   return nullptr;
   }

//-- LookupTable methods ---------------------------------------------------------
bool LookupTable::Evaluate( int idu )
   {
   ASSERT(gpModeler->m_pMapLayer != nullptr );
   int fieldVal;

   ASSERT ( this->m_colAttr >= 0 );

   gpModeler->m_pMapLayer->GetData( idu, this->m_colAttr, fieldVal );
   bool ok = Lookup( fieldVal, m_value ) ? true : false;

   if ( ! ok )
      m_value = m_defaultValue;

   return ok;
   }


//-- Variable methods ---------------------------------------------------------

Variable::~Variable()
   { 
   if ( m_pParser != nullptr ) 
      delete m_pParser; 

   //if ( m_pRaster != nullptr )
   //   delete m_pRaster;
   
   if ( m_pQuery ) 
      gpModeler->m_pQueryEngine->RemoveQuery( m_pQuery );

   if ( m_pValueQuery ) 
      gpModeler->m_pQueryEngine->RemoveQuery( m_pValueQuery );
   }


bool Variable::Compile( ModelBase *pModel, ModelCollection *pCollection, MapLayer *pLayer )
   {
   ASSERT( m_pParser == nullptr );
   //ASSERT( m_pQuery == nullptr );
   ASSERT( ! m_expression.IsEmpty()  );

   // NOTE: ASSUMES all evaluable expressions/field have been defined
   switch( m_varType )
      {
      case VT_EXPRESSION:
      case VT_SUM_AREA:
      case VT_DENSITY:
      case VT_AREAWTMEAN:
      case VT_GLOBALEXT:
         {
         ASSERT( m_pParser == nullptr );
         m_pParser = new MParser;

         if ( CompileExpression( pModel, pCollection, m_pParser, m_expression, pLayer ) == false )
            {
            delete m_pParser;
            m_pParser = nullptr;
            return false;
            }
         }
         break;

      case VT_PCT_AREA:
      //case VT_PCT_CONTRIB_AREA:
         {
         //m_pQuery = Modeler::m_pQueryEngine->ParseQuery( m_expression );  // parsed when first read
         if ( m_pQuery == nullptr )
            return false;
         }
         break;

      default:
         ASSERT( 0 );
      }

   return true;
   }


bool Variable::Evaluate( void )
   {
   gpCurrentVar = this;
   bool retVal = false;

   switch( m_varType )
      {
      case VT_EXPRESSION:
      case VT_SUM_AREA:
      case VT_AREAWTMEAN:
      case VT_DENSITY:
      //case VT_GLOBALEXT:  // only evaluated once at end of cycle
         // for expressions, evaluate the expression with the parser. 
         // Assumes all variables defined, current for the given IDU
         if ( ( m_pParser != nullptr ) && ( ! m_pParser->evaluate( m_value ) ) )
            m_value = 0; 

         retVal = true;
         break;

      case VT_PCT_AREA:
      //case VT_PCT_CONTRIB_AREA:
         // for pctArea variables, nothing is required
         ASSERT( m_pQuery != nullptr );
         retVal =  true;
         break;
      }

   gpCurrentVar = nullptr;
   return retVal;
   }


Expression::~Expression( void )
   {
   if ( m_pQuery != nullptr )
      {
      //Modeler::m_pQueryEngine->RemoveQuery( m_pQuery );  // deletes query
      //m_pQuery = nullptr;
      }

   if ( m_pParser != nullptr )
      {
      delete m_pParser;
      m_pParser = nullptr;
      }
   }


//-- ModelBase methods methods ---------------------------------------------------------

Expression *ModelBase::AddExpression( LPCTSTR expr, LPCTSTR query )
   {
   ASSERT ( expr != nullptr );

   Expression *pExpr = new Expression;
   pExpr->m_expression = expr;
   pExpr->m_queryStr = query;

   pExpr->m_pParser = new MParser;

   //bool CompileExpression( this, ModelCollection *pCollection, MParser *pParser, LPCTSTR expr, MapLayer *pLayer )      
   //pExpr->m_pParser->compile( expr );

   if ( query != nullptr )
      {
      pExpr->m_pQuery = gpModeler->m_pQueryEngine->ParseQuery( query, 0, "Modeler" );

      if ( pExpr->m_pQuery == nullptr )
         {
         CString msg( _T("Error parsing query for model expression " ) );
         msg += this->m_modelName;
         msg += _T(": Query=");
         msg += query;
         msg += _T( ". Query will be ignored" );
         Report::LogWarning(msg);
         }
      }

   m_exprArray.Add( pExpr );  // this is a ptrarray
   return pExpr;
   }


void ModelBase::SetVars( EnvExtension *pExt )
   {
   for ( INT_PTR i=0; i < m_variableArray.GetSize(); i++ )
      {
      Variable *pVar = m_variableArray[ i ];
      
      if ( pVar->m_isOutput )
         pExt->AddOutputVar( pVar->m_name, pVar->m_value, _T("") );
      }
   }


void ModelBase::Clear()
   {
   m_histogramsArray.RemoveAll();
   m_lookupTableArray.RemoveAll();
   m_constantArray.RemoveAll();
   m_variableArray.RemoveAll();

   m_exprArray.RemoveAll();

   //if ( m_pRaster != nullptr )
   //   {
   //   delete m_pRaster;
   //   m_pRaster = nullptr;
   //   }
   }


bool ModelBase::LoadModelBaseXml( TiXmlNode *pXmlModelNode, MapLayer *pLayer )
   {
   bool loadSuccess = true;

   TiXmlNode *pXmlNode = nullptr;
   while( pXmlNode = pXmlModelNode->IterateChildren( pXmlNode ) )
      {
      if ( pXmlNode->Type() != TiXmlNode::ELEMENT )
            continue;

      // is this an include statment contined within an eval_model block?
      //if ( lstrcmpi( "include", pXmlNode->Value() ) == 0 )
      //   {
      //   if ( ! LoadInclude( pXmlNode, pLayer ) )
      //      loadSuccess = false;
      //   }

      // is this a lookup table?
      if ( lstrcmpi( "lookup", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadLookup( pXmlNode, m_lookupTableArray, pLayer ) )
            loadSuccess = false;
         }

      else if ( lstrcmpi( "histograms", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadHistograms( pXmlNode, m_histogramsArray, pLayer ) )
            loadSuccess = false;
         }

      // is this a constant?
      else if ( lstrcmpi( "const", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadConstant( pXmlNode, m_constantArray, false ) )
            loadSuccess = false;
         }

      // is this a constant?
      else if ( lstrcmpi( "input", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadConstant( pXmlNode, m_constantArray, true ) )
            loadSuccess = false;
         }

      // is this a variable?
      else if ( lstrcmpi( "var", pXmlNode->Value() ) == 0
             || lstrcmpi( "report", pXmlNode->Value() ) == 0  
             || lstrcmpi( "output", pXmlNode->Value() ) == 0 )
         {
         bool isReport = false;
         if ( lstrcmpi( "report", pXmlNode->Value() ) == 0 )
            isReport = true;
         else if ( lstrcmpi( "output", pXmlNode->Value() ) == 0 )
            isReport = true;

         if ( ! LoadVariable( pXmlNode, m_variableArray, isReport ) )
            loadSuccess = false;
         else
            {
            Variable *pVar = m_variableArray.GetAt( m_variableArray.GetSize() -1 );
            pVar->m_pCollection = this->m_pCollection;
            pVar->m_pModel = this;
            }
         }

      else if ( lstrcmpi( "eval", pXmlNode->Value() ) == 0 || lstrcmpi( "case", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadExpr( pXmlNode, this ) )
            loadSuccess = false;
         }
      }
      
   return loadSuccess;
   }


bool ModelBase::LoadInclude( TiXmlNode *pXmlIncludeNode, MapLayer *pLayer )
   {
   //NOTE DOESN"T WORK AT THIS POINT
   ASSERT( 0 );
   bool loadSuccess = true;
   if ( pXmlIncludeNode->Type() != TiXmlNode::ELEMENT )
       return false;
       
   TiXmlElement *pXmlIncludeElement = pXmlIncludeNode->ToElement();
   LPCTSTR file = pXmlIncludeElement->Attribute( "file" );

   if ( file == nullptr )
      return false;

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( file );

   if ( ! ok )
      {      
      CString msg( _T("  Error loading include file ") );
      msg += file;
      msg += ": ";
      msg += doc.ErrorDesc();
      Report::ErrorMsg( msg );
      return false;
      }
 
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();

   ok = LoadModelBaseXml( pXmlRoot, pLayer );
   return ok;
   }


bool ParseUseDelta( LPCTSTR deltaStr, CArray< USE_DELTA, USE_DELTA&> &useDeltaArray )
   {
   TCHAR _deltaStr[ 256 ];
   lstrcpy( _deltaStr, deltaStr );

   LPTSTR p = _deltaStr;
   LPTSTR field = p;
   LPTSTR value = nullptr;

   VData vdata;

   CString _field;
   CString _value;

   while ( *p != 0 )
      {
      switch( *p )
         {
         case '=':
            *p = 0;
            value = p+1;
            break;

         case ';':
            {
            *p = 0;

            _field = field;
            _field.Trim();

            int col = gpModeler->m_pMapLayer->GetFieldCol( _field );

            if ( col < 0 )
               {
               CString msg( _T("  Unable to find field ") );
               msg += field;
               msg += _T(" when parsing useDelta element");
               Report::ErrorMsg( msg );
               }
            else
               {
               _value = value;
               _value.Trim();
               vdata.Parse( _value );
               useDeltaArray.Add( USE_DELTA( col, vdata ) );
               }

            field = p+1;
            }
            break;
         }

      ++p;
      }

   _field = field;
   _field.Trim();

   int col = gpModeler->m_pMapLayer->GetFieldCol( _field );

   if ( col < 0 )
      {
      CString msg( _T("  Unable to find field ") );
      msg += field;
      msg += _T(" when parsing useDelta element");
      Report::ErrorMsg( msg );
      }
   else
      {
      _value = value;
      _value.Trim();
      vdata.Parse( _value );
      useDeltaArray.Add( USE_DELTA( col, vdata ) );
      }
   return true;
   }


bool LoadLookup( TiXmlNode *pXmlLookupNode, LookupArray &lookupArray, MapLayer *pLayer )
   {
   bool loadSuccess = true;
   if ( pXmlLookupNode->Type() != TiXmlNode::ELEMENT )
         return false;
            
   // process lookup table
   //    <lookup name="ErosionCoeff" col="lulc_c">
   //         <map src="1" value="5.1" />
   //    </lookup>

   TiXmlElement *pXmlLookupElement = pXmlLookupNode->ToElement();
   LPCTSTR name          = pXmlLookupElement->Attribute( "name" );
   LPCTSTR attrColName   = pXmlLookupElement->Attribute( "attr_col" );
   LPCTSTR outputColName = pXmlLookupElement->Attribute( "col" );
   LPCTSTR defValue      = pXmlLookupElement->Attribute( "default" );

   if ( name == nullptr )
      name = attrColName;

   if ( attrColName == nullptr )
      {
      CString msg( "  Misformed <lookup> element [" );
      msg += name;
      msg += "]: A required attribute 'attr_col' is missing.";
      Report::ErrorMsg( msg );
      return false;
      }

   LookupTable *pTable = new LookupTable;
   pTable->m_name = name;
   pTable->m_colName = attrColName;
   pTable->m_colAttr  = pLayer->GetFieldCol( attrColName );
   
   if ( outputColName != nullptr )
      {
      pTable->m_outputColName = outputColName;
      pTable->m_colOutput = pLayer->GetFieldCol( outputColName ); 

      if ( pTable->m_colOutput < 0 )
         {
         CString msg( "  A field referenced by a lookup table is missing: " );
         msg += outputColName;
         msg += ".  This field will be added to the IDU coverage.";
         Report::WarningMsg( msg );
         pTable->m_colOutput = pLayer->m_pDbTable->AddField( outputColName, TYPE_FLOAT ); 
         }
      }

   lookupArray.Add( pTable );

   if ( pTable->m_colAttr < 0 )
      {
      CString msg( "  A field referenced by a lookup table is missing: " );
      msg += attrColName;
      msg += " and should be added to the IDU coverage.";
      Report::WarningMsg( msg );
      }

   if ( defValue != nullptr )
      pTable->m_defaultValue = atof( defValue );

   // have basics, get map elements for this lookup table
   TiXmlNode *pXmlMapNode = nullptr;
   while( pXmlMapNode = pXmlLookupNode->IterateChildren( pXmlMapNode ) )
      {
      if ( pXmlMapNode->Type() != TiXmlNode::ELEMENT )
         continue;
         
      ASSERT( lstrcmp( pXmlMapNode->Value(), "map" ) == 0 );
      TiXmlElement *pXmlMapElement = pXmlMapNode->ToElement();

      LPCTSTR attr = pXmlMapElement->Attribute( "attr_val" );
      LPCTSTR val  = pXmlMapElement->Attribute( "value" );

      if ( attr == nullptr || val == nullptr )
         {
         Report::ErrorMsg( _T("  Misformed <map> element: A required attribute is missing."));
         loadSuccess = false;
         continue;
         }

      int attrValue = 0;
      attrValue = atoi( attr );
      double value = atof( val );
      pTable->SetAt( attrValue, value );
      }  // end of: while ( map elements )
      
   return loadSuccess;
   }


bool LoadHistograms( TiXmlNode *pXmlHistogramsNode, HistoArray &histoArray, MapLayer *pLayer )
   {
   bool loadSuccess = true;
   if ( pXmlHistogramsNode->Type() != TiXmlNode::ELEMENT )
         return false;            
   
   TiXmlElement *pXmlHistogramsElement = pXmlHistogramsNode->ToElement();  // note - this could be from an include file or direct
   LPCTSTR name    = pXmlHistogramsElement->Attribute( "name" );
   LPCTSTR file    = pXmlHistogramsElement->Attribute( "file" );
   LPCTSTR use     = pXmlHistogramsElement->Attribute( "use" );

   if ( name == nullptr || file == nullptr || use== nullptr )
      {      
      CString msg( _T("Error loading <histograms> - A required attribute is missing loading Modeler file: ") );
      msg += name;
      return false;
      }

   HistogramArray *pHistoArray = new HistogramArray( pLayer );
   histoArray.Add( pHistoArray );

   pHistoArray->LoadXml( file );

   pHistoArray->m_name = name;

   if ( use )
      {
      switch( use[ 0 ] )
         {
         case 'U':
         case 'u':
            pHistoArray->m_use = HU_UNIFORM;
            break;

         case 'M':
         case 'm':
            pHistoArray->m_use = HU_MEAN;
            break;
            
         case 'A':
         case 'a':
            pHistoArray->m_use = HU_AREAWTMEAN;
            break;

         case 'S':
         case 's':
            pHistoArray->m_use = HU_STDDEV;
            break;
            
         default:
            {
            CString msg( "  Invalid Histogram use specified in [" );
            msg += name;
            msg += "] - ";
            msg += use;
            msg += "... defaulting to uniform sampling";
            Report::WarningMsg( msg );
            pHistoArray->m_use = HU_UNIFORM;
            }
         }
      }

   return loadSuccess;
   }

bool LoadConstant( TiXmlNode *pXmlConstantNode, ConstantArray &constantArray, bool isInput  )
   {
   bool loadSuccess = true;
   if ( pXmlConstantNode->Type() != TiXmlNode::ELEMENT )
      return false;
            
   // process constant
   //
   //   <constant name="C1" value='10' input='true'/>  
   //   
   TiXmlElement *pXmlElement = pXmlConstantNode->ToElement();
   LPCTSTR name  = pXmlElement->Attribute( "name" );
   LPCTSTR value = pXmlElement->Attribute( "value" );
   LPCTSTR input = pXmlElement->Attribute( "input" );  // optional
   LPCTSTR descr = pXmlElement->Attribute( "description" );  // optional

   if ( name == nullptr )
      {
      Report::ErrorMsg( _T("  <const/input> element missing name attribute" ) );
      return false;
      }

   if ( value == nullptr )
      {
      Report::ErrorMsg( _T("  <const/input> element missing value attribute" ) );
      return false;
      }

   if ( ! isInput )
      {      
      if ( input && ( *input == 't' || *input == '1' ) )
         isInput = true;
      }

   Constant *pConstant = new Constant;
   pConstant->m_name    = name;
   pConstant->m_value   = atof( value );
   pConstant->m_isInput = isInput; 
   pConstant->m_description = descr;

   constantArray.Add( pConstant ); 
      
   return loadSuccess;
   } 


bool LoadVariable( TiXmlNode *pXmlVariableNode, VariableArray &variableArray, bool isReport  )
   {
   bool loadSuccess = true;
   if ( pXmlVariableNode->Type() != TiXmlNode::ELEMENT )
      return false;
            
   // process Variable
   //  
   //  <var name="C1" value='10' input='true'/>    
   //  
   TiXmlElement *pXmlElement = pXmlVariableNode->ToElement();
   LPCTSTR name     = pXmlElement->Attribute( "name" );
   LPCTSTR type     = pXmlElement->Attribute( "type" );
   LPCTSTR value    = pXmlElement->Attribute( "value" );
   LPCTSTR query    = pXmlElement->Attribute( "query" );
   LPCTSTR valueQuery = pXmlElement->Attribute( "value_query" );
   LPCTSTR output   = pXmlElement->Attribute( "output" );  // optional
   LPCTSTR useDelta = pXmlElement->Attribute( "use_delta" );  // optionals
   LPCTSTR timing   = pXmlElement->Attribute( "timing" );
   LPCTSTR col      = pXmlElement->Attribute( "col" );

   //LPCTSTR extent  = pXmlElement->Attribute( "extent" );   // options - defaults to EXTENT_IDU - note - this in inferred from the type

   if ( name == nullptr )
      {
      Report::ErrorMsg( _T("  <var> element missing 'name' attribute" ) );
      return false;
      }

   // make sure name doesn't conflict with any field name
   int _col_ = gpModeler->m_pMapLayer->GetFieldCol( name );
   if ( _col_ >= 0 )
      {
      CString msg;
      msg.Format( _T("  Variable name conflict: The 'name' attribute specified  <var name='%s'...> matches a field name in the IDU coverage.  This is not allowed; variable names must be unique.  This variable will be ignored."), name );
      Report::ErrorMsg( msg );
      return false;
      }

   // get type
   VAR_TYPE vtype = VT_UNDEFINED;
   if ( type == nullptr )
      vtype = VT_EXPRESSION;
   else if ( lstrcmpi( "pctArea", type ) == 0 )
      vtype = VT_PCT_AREA;
   else if ( lstrcmpi( "sum", type ) == 0 )
      vtype = VT_SUM_AREA;
   else if ( lstrcmpi( "areaWtMean", type ) == 0 )
      vtype = VT_AREAWTMEAN;
   else if ( lstrcmpi( "global", type ) == 0 )
      vtype = VT_GLOBALEXT;
   else if ( lstrcmpi( "raster", type ) == 0 )
      vtype = VT_RASTER;
   else if ( lstrcmpi( "density", type ) == 0 )
      vtype = VT_DENSITY;
   else
      {
      CString msg( "  Invalid Variable type encountered reading <var name='" );
      msg += name;
      msg += ">: ";
      msg += type;
      Report::ErrorMsg( type );
      return false;
      }

   if ( ( vtype != VT_PCT_AREA && vtype != VT_RASTER ) && value == nullptr )
      {
      CString msg;
      msg.Format( _T("  <var> '%s' missing 'value' expression attribute - this is required" ), name );
      Report::ErrorMsg( msg );
      return false;
      }

   if ( vtype == VT_PCT_AREA )
      {
      if ( query == nullptr )
         {
         CString msg;
         msg.Format( _T("  <var> '%s' missing 'query' attribute - this is required for pctArea and pctContribArea-type variables" ), name );
         Report::ErrorMsg( msg );
         return false;
         }
      else if ( lstrlen( query ) == 0 )
         {
         CString msg;
         msg.Format( _T("  <var> '%s's 'query' attribute is empty - this is required for pctArea and pctContribArea-type variables" ), name );
         Report::ErrorMsg( msg );
         return false;
         }
      }

   float cellSize = 0;
   if ( vtype == VT_RASTER )
      {
      LPCTSTR _cellSize = pXmlElement->Attribute( "cell_size" );
      if ( _cellSize == nullptr )
         {
         CString msg;
         msg.Format( _T("  <var> element '%s' is missing a 'cell-size' attribute - this is required for raster variables" ), name );
         Report::ErrorMsg( msg );
         return false;
         }
      else
         cellSize = (float) atof( _cellSize );

      if ( col == nullptr )
         {
         CString msg;
         msg.Format( _T("  <var> '%s' is missing a 'col' attribute is empty - this is required for raster variables" ), name );
         Report::ErrorMsg( msg );
         return false;
         }
      }

   bool isOutput = false;
   if ( output && (*output == 't' || *output=='1' ) )
      isOutput = true;

   EXTENT _extent = EXTENT_IDU;
   if ( vtype == VT_SUM_AREA || vtype == VT_AREAWTMEAN || vtype == VT_PCT_AREA || vtype == VT_GLOBALEXT ) // || vtype == VT_DENSITY )
      _extent = EXTENT_GLOBAL;
      
   Variable *pVariable = new Variable;
   pVariable->m_name    = name;
   pVariable->m_varType = vtype;
   
   if ( useDelta != nullptr )
      {
      if ( _extent != EXTENT_GLOBAL )
         {
         CString msg;
         msg.Format( "  Invalid use of 'use_delta' attribute for variable [%s] - This attribute is incompatible with IDU-extent variables", name );
         Report::ErrorMsg( msg );
         }
      else
         {
         pVariable->m_useDelta = true;
         pVariable->m_deltaStr = useDelta;

         //pModel->ParseUseDelta( useDelta );
         ParseUseDelta( useDelta, pVariable->m_useDeltas );
         }
      }

   if ( timing != nullptr )
      {
      if ( _extent != EXTENT_GLOBAL )
         {
         CString msg;
         msg.Format( "  Invalid use of 'timing' attribute for variable [%s] - This attribute is incompatible with IDU-extent variables", name );
         Report::ErrorMsg( msg );
         }
      else
         pVariable->m_timing = atoi( timing );
      }

   // store and process query if defined
   if ( query != nullptr )
      {
      pVariable->m_query = query;
      ASSERT(gpModeler->m_pQueryEngine != nullptr );
      CString source( "Modeler Variable '" );
      source += pVariable->m_name;
      source += "'";

      pVariable->m_pQuery = gpModeler->m_pQueryEngine->ParseQuery( query, 0, source );
      if ( pVariable->m_pQuery == nullptr )
         {
         CString msg;
         msg.Format( _T("  Invalid query encountered processing <var name='%s' query='%s'...>" ), name, query );
         Report::ErrorMsg( msg );
         delete pVariable;
         return false;
         }
      }

   if ( valueQuery != nullptr )
      {
      pVariable->m_valueQuery = query;
      ASSERT(gpModeler->m_pQueryEngine != nullptr );
      CString source( "Modeler Variable '" );
      source += pVariable->m_name;
      source += "'";

      pVariable->m_pValueQuery = gpModeler->m_pQueryEngine->ParseQuery( valueQuery, 0, source );
      if ( pVariable->m_pValueQuery == nullptr )
         {
         CString msg;
         msg.Format( _T("  Invalid value query encountered processing <var name='%s' value_query='%s'...>" ), name, valueQuery );
         Report::ErrorMsg( msg );
         delete pVariable;
         return false;
         }
      }

   if ( col != nullptr )
      {
      int _col = gpModeler->m_pMapLayer->GetFieldCol( col );

      if ( _col < 0 )
         {
         CString msg;
         msg.Format( _T("Unable to find field specified in 'col' attribute when processing var < name='%s' col='%s'...>.  Do you want to add it?" ), name, col );

         int result = AfxMessageBox( msg, MB_YESNO );

         if ( result == IDYES )
            _col = gpModeler->m_pMapLayer->m_pDbTable->AddField( col, TYPE_FLOAT, true );
         }

      if ( _col >= 0 )
         {
         pVariable->m_col = _col;
         pVariable->m_dataType = gpModeler->m_pMapLayer->GetFieldType( _col );
         }
      }

   // set expression.  This varies based the type of the variable
   pVariable->m_expression = value; 
   pVariable->m_isOutput = isOutput;
   pVariable->m_extent = _extent;
   pVariable->m_cellSize = cellSize;

   if ( isReport )
      {
      if ( pVariable->m_extent != EXTENT_GLOBAL )
         {
         CString msg;
         msg.Format( _T("  Invalid <report> element type:  reports must have global extent (type='sum|pctArea|pctContribArea|global') for report '%s'..." ), name );
         Report::ErrorMsg( msg );
         }

      pVariable->m_isOutput = true;
      pVariable->m_timing = 1;
      }

   //if ( vtype == VT_RASTER )
   //   pVariable->m_pRaster = new Raster( Modeler::m_pMapLayer, nullptr, nullptr, false );

   variableArray.Add( pVariable ); 

   return loadSuccess;
   }
   
   
// load <eval> elements
bool LoadExpr( TiXmlNode *pXmlExprNode, ModelBase *pModel  )
   {
   bool loadSuccess = true;
   if ( pXmlExprNode->Type() != TiXmlNode::ELEMENT )
      return false;
            
   // process Expression
   //  
   //  <eval name="C1" value='10' query='' />    
   //  
   TiXmlElement *pXmlElement = pXmlExprNode->ToElement();
   //LPCTSTR name   = pXmlElement->Attribute( "name" );
   LPCTSTR value  = pXmlElement->Attribute( "value" );
   LPCTSTR query  = pXmlElement->Attribute( "query" );
   //LPCTSTR output = pXmlElement->Attribute( "output" );  // optional
   //LPCTSTR extent  = pXmlElement->Attribute( "extent" );   // options - defaults to EXTENT_IDU

   //if ( name == nullptr )
   //   {
   //   ErrorMsg( _T("<eval> element missing 'name' attribute - this is required" ) );
   //   return false;
   //   }

   if ( value == nullptr )
      {
      Report::ErrorMsg( _T("  Modeler: <eval> element missing 'value' expression attribute - this is required" ) );
      return false;
      }

   pModel->AddExpression( value, query );

   return loadSuccess;
   }




bool ModelBase::Compile( MapLayer *pLayer )
   {
   // compile model.  Note that this requires several steps.
   //  1) compile the basic parser expression.
   //  2) collect the variables used in the expression
   //      these can be a) field cols, b) lookup tables,
   //        c) constants, d) variables, or e) histograms
   //  3) define each parser variable appropriately
   //ASSERT( m_pParser != nullptr );

   // compile any variables
   for ( int i=0; i < m_variableArray.GetSize(); i++ )
      {
      Variable *pVar = m_variableArray.GetAt( i );
      pVar->Compile( this, m_pCollection, pLayer ); // extent include both this ModelBase and the ModelCollection
      }

   // compile any expressions
   for ( int i=0; i < m_exprArray.GetSize(); i++ )
      {
      Expression *pExpr = m_exprArray[ i ];
      bool success = CompileExpression( this, m_pCollection, pExpr->m_pParser, pExpr->m_expression, pLayer );
      if ( ! success )
         {
         delete pExpr->m_pParser;
         pExpr->m_pParser = nullptr;
         return false;
         }
      }

   return true;
   }



bool ModelBase::UpdateVariables( EnvContext *pEnvContext, int idu, bool useAddDelta )
   {
   // evaluate lookups
   for ( int j=0; j < this->m_lookupTableArray.GetSize(); j++ )
      {
      LookupTable *pTable = this->m_lookupTableArray[ j ];
      pTable->Evaluate( idu );

      if ( pTable->m_colOutput >= 0 )
         {
         if ( useAddDelta )
            {
            // any change?
            double value = 0;
            pEnvContext->pMapLayer->GetData( idu, pTable->m_colOutput, value );

            if ( value != pTable->m_value ) 
               {
               int hModel = pEnvContext->pEnvExtension->m_handle;
               pEnvContext->ptrAddDelta( pEnvContext->pEnvModel, idu, pTable->m_colOutput, pEnvContext->currentYear, VData( pTable->m_value ), hModel );
               }
            }
         else
            {
            MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
            pLayer->SetData( idu, pTable->m_colOutput, pTable->m_value );
            }
         }
      }

   // evaluate histograms
   for ( int j=0; j < this->m_histogramsArray.GetSize(); j++ )
      {
      HistogramArray *pHistoArray = this->m_histogramsArray[ j ];
      double prevVal = pHistoArray->m_value;
      double value;
      bool ok = pHistoArray->Evaluate( idu, value );
      if ( m_constraints == C_INCREASE_ONLY && value < prevVal )
         pHistoArray->m_value = prevVal;
      //ASSERT( ok );
      }

   // evaluate idu-scope variables
   for ( int j=0; j < this->m_variableArray.GetSize(); j++ )
      {
      Variable *pVar = this->m_variableArray[ j ];
      if ( pVar->m_extent == EXTENT_IDU )
         {
         gCurrentIDU  = idu;
         bool ok = pVar->Evaluate();
         ASSERT( ok );
         gCurrentIDU  = -1;

         if ( pVar->m_col >= 0 )
            {
            bool ok = true;
            if ( pVar->m_pQuery != nullptr )
               {
               bool result = false;
               bool ok2 = pVar->m_pQuery->Run( idu, result );

               if ( !ok2 || !result )
                  ok = false;               
               }

            if ( ok )
               {
               VData value( pVar->m_value );
               value.ChangeType( pVar->m_dataType );

               if ( useAddDelta )
                  {
                  VData oldValue;
                  pEnvContext->pMapLayer->GetData( idu, pVar->m_col, oldValue );

                  if ( value != oldValue )
                     {
                     int hModel = pEnvContext->pEnvExtension->m_handle;
                     pEnvContext->ptrAddDelta( pEnvContext->pEnvModel, idu, pVar->m_col, pEnvContext->currentYear, value, hModel );
                     }
                  }
               else
                  {
                  MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
                  pLayer->SetData( idu, pVar->m_col, value );
                  }
               }
            }
         }
      }

   // note: no need to evaluate constants - once is enough
   return true;
   }



bool CompileExpression( ModelBase *pModel, ModelCollection *pCollection, MParser *pParser, LPCTSTR expr, MapLayer *pLayer )
   {
   pParser->enableAutoVarDefinition( true );
   pParser->compile( expr );

   int varCount = pParser->getNbUsedVars();

   for ( int i=0; i < varCount; i++ )
      {
      bool found = false;

      CString varName = pParser->getUsedVar( i ).c_str();

      // is it a legal field in the map?
      int col = pLayer->GetFieldCol( varName );
      if ( col >= 0 )
         {
         // have we already seen this column?
         int index = col;
         if (gpModeler->m_usedParserVars.Lookup( col, index ) )
            {
            ASSERT( col == index );
            }
         else  // add it
            gpModeler->m_usedParserVars.SetAt( col, col );

         pParser->redefineVar( varName, gpModeler->m_parserVars+index );
         continue;
         }

      // ModelBase scope?
      if ( pModel != nullptr )
         {
         // is it a histogram?
         HistogramArray *pHisto = FindHistogram( pModel->m_histogramsArray, varName );
         if ( pHisto != nullptr )
            {
            pParser->redefineVar( varName, &pHisto->m_value );
            continue;
            }

         // is it a lookup table
         LookupTable *pLookup = FindLookup( pModel->m_lookupTableArray, varName );
         if ( pLookup != nullptr )
            {
            pParser->redefineVar( varName, &pLookup->m_value );
            continue;
            }

         // is it a constant?
         Constant *pConst = FindConstant( pModel->m_constantArray, varName );
         if ( pConst != nullptr )
            {
            pParser->redefineVar( varName, &pConst->m_value );
            continue;
            }

         // is it a variable?
         Variable *pVar = FindVariable( pModel->m_variableArray, varName );
         if ( pVar != nullptr )
            {
            pParser->redefineVar( varName, &pVar->m_value );

            // compile variable expression parser as well.   NOTE: this should already be done by the time we get here
            //pVar->Compile( pModel, pCollection, pLayer );
            continue;
            }
         }

      if ( pCollection )
         {
         // is it a histogram?
         HistogramArray *pHisto = FindHistogram( pCollection->m_histogramsArray, varName );
         if ( pHisto != nullptr )
            {
            pParser->redefineVar( varName, &pHisto->m_value );
            continue;
            }

         // is it a lookup table
         LookupTable *pLookup = FindLookup( pCollection->m_lookupTableArray, varName );
         if ( pLookup != nullptr )
            {
            pParser->redefineVar( varName, &pLookup->m_value );
            continue;
            }

         // is it a constant?
         Constant *pConst = FindConstant( pCollection->m_constantArray, varName );
         if ( pConst != nullptr )
            {
            pParser->redefineVar( varName, &pConst->m_value );
            continue;
            }

         // is it a variable?
         Variable *pVar = FindVariable( pCollection->m_variableArray, varName );
         if ( pVar != nullptr )
            {
            pParser->redefineVar( varName, &pVar->m_value );

            // compile variable expression parser as well
            //pVar->Compile( pModel, pCollection, pLayer );
            continue;
            }
         }

      // if we get this far, the variable is undefiend
      CString msg( "Unable to compile Modeler expression for Model: " );
      if ( pModel != nullptr )
         {
         msg += pModel->m_modelName;
         msg +=" [";
         msg += expr;
         msg += "]. ";
         }
      msg += "Expression: ";
      msg += expr;
      msg += " This expression will be ignored";
      Report::LogWarning( msg );
      return false;
      }  // end of:  for ( i < varCount )

   return true;
   }




bool Evaluator::Init(EnvContext*, LPCTSTR)
   {
   return true;
   }

bool Evaluator::InitRun(EnvContext*)
   {
   return true;
   }

bool Evaluator::Run( EnvContext *pEnvContext)
   {
   m_rawScore = 0;
   bool useAddDelta = true;
   m_pCollection->UpdateGlobalExtentVariables( pEnvContext, 0, useAddDelta ); // this evaluates all global-extent variables, including model-scope variables
   
   if ( this->m_useDelta )
      RunDelta( pEnvContext );      // sets rawscore
   else
      RunMap( pEnvContext, useAddDelta ); // sets rawscore

   m_pCollection->UpdateGlobalExtentVariables( pEnvContext, 1, useAddDelta ); // this evaluates all global-extent variables, including model-scope variables

   // scale to (-3, +3)
   if (m_autoBounds)
      {



      }


   float range = m_upperBound - m_lowerBound;
   m_score = ((( m_rawScore - m_lowerBound ) / range) * 6) - 3;
         
   if ( m_evalType == ET_VALUE_LOW )
   m_score *= -1;

   // make sure score is in range
   if ( m_score < -3.0f ) m_score = -3.0f;
   else if ( m_score > 3.0f ) m_score = 3.0f;

   pEnvContext->score = m_score;
   pEnvContext->rawScore = m_rawScore;
   
   return TRUE;
   }


bool Evaluator::RunMap( EnvContext *pEnvContext, bool useAddDelta )
   {
   INT_PTR exprCount = m_exprArray.GetSize();

   if ( exprCount == 0 )
      {
      pEnvContext->score = 0;
      pEnvContext->rawScore = m_rawScore = 0;
      return true;
      }

   m_rawScore = 0;

   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   int colArea = pLayer->GetFieldCol( _T("Area") );
   if ( colArea < 0 )
      return false;

   int iduCount = 0;

   // iterate over ACTIVE records
   //#pragma omp parallel for private( v, type, area, result, iduValue ) reduction (iduCount, m_rawScore )

   bool *runExpr = new bool[ exprCount ];

   for ( MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++ )
      {
      bool isIduUpdated = false;
      bool runIDU = false;

      for ( INT_PTR i=0; i < exprCount; i++ )
         {
         Expression *pExpr = m_exprArray[ i ];

         if ( pExpr->m_pQuery == nullptr )
            {
            runExpr[ i ] = true;
            runIDU = true;
            }
         else 
            {
            bool result;
            pExpr->m_pQuery->Run( idu, result );
            runExpr[ i ] = result;

            if ( result )
               runIDU = true;
            }
         }

      if ( runIDU )
         {
         iduCount = iduCount+1;

         if ( ! isIduUpdated )
            {
            m_pCollection->UpdateVariables( pEnvContext, idu, useAddDelta ); // this loads all parserVars and evaluates ModelCollection scope variables for the current IDU
            UpdateVariables( pEnvContext, idu, useAddDelta );     // update variables for this collection
            isIduUpdated = true;
            }

         float area = 0;
         pLayer->GetData( idu, colArea, area );
      
         // done evalating all inputs - run model formula(s)
         double iduValue = 0;
         for ( INT_PTR i=0; i < exprCount; i++ )
            {
            if ( runExpr[ i ] )
               {
               Expression *pExpr = m_exprArray[ i ];

               ASSERT( pExpr->m_pParser != nullptr );
               if ( ! pExpr->m_pParser->evaluate( iduValue ) )
                  iduValue = 0;
               }
            }

         if ( m_col >= 0 )   // populate the IDU database?
            {
            TYPE type = pLayer->GetFieldType( m_col );

            VData v;
            v.type = type;

            v.SetKeepType( iduValue );

            if ( useAddDelta )
               {
               VData oldValue;
               pLayer->GetData( idu, m_col, oldValue );

               if ( oldValue.Compare( v ) == false )
                  AddDelta( pEnvContext, idu, m_col, v );  // needs a critical section!!!!!!
               }
            else
               pLayer->SetData( idu, m_col, v );
            }

         switch( m_evalMethod )
            {
            case EM_SUM:    // sum the scores over the entire coverage
            case EM_MEAN:    // compute the average score over the entire coverage
               m_rawScore += (float) iduValue;
               break;

            case EM_AREAWTMEAN:  // compute an area-weighted average value over the entire coverage
            case EM_DENSITY:
               m_rawScore += float( iduValue ) * area;
               break;

            case EM_PCT_AREA:
               m_rawScore += float( iduValue) * area;
            }
         }  // end of: if ( runIDU )
      }  // end of: MapIterator

   delete [] runExpr;
   
   // normalize the scores, depending on the evalMethod
   if ( iduCount == 0 )
      {
      pEnvContext->score = pEnvContext->rawScore = 0;
      return true;
      }
   
   switch( m_evalMethod )
      {
      case EM_SUM:    // sum the scores over the entire coverage
      case EM_DENSITY:
         break;         // no additional processing needed

      case EM_MEAN:    // compute the average score over the entire coverage
         m_rawScore /= iduCount;
         break;

      case EM_AREAWTMEAN:  // compute an area-weighted average value over the entire coverage
         {
         float totalArea = pLayer->GetTotalArea();
         m_rawScore /= totalArea;
         }
         break;

      case EM_PCT_AREA:
         {
         float totalArea = pLayer->GetTotalArea();
         m_rawScore /= totalArea;
         }
         break;
      }

   return true;
   }


bool Evaluator::RunDelta( EnvContext *pEnvContext )
   {
   ASSERT( m_useDelta == true );

   float lb = LONG_MAX;
   float up = LONG_MIN;
   float mean = 0;
   float stddev = 0;

   m_rawScore = 0;

   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   int colArea = pLayer->GetFieldCol( _T("Area") );
   if ( colArea < 0 )
      return false;

   // Load variables
   int iduCount = 0;

   // load current values into the m_parserVars array 
   DeltaArray *deltaArray = pEnvContext->pDeltaArray;// get a ptr to the delta array

   // HACK FOR NOW _ ASSUME ONLY ONE EXPRESSION!!!!
   ASSERT ( m_exprArray.GetSize() == 1 );
   Expression *pExpr = m_exprArray[ 0 ];
   
      // iterate through deltas added since last “seen”
   for ( INT_PTR i=pEnvContext->firstUnseenDelta; i < deltaArray->GetSize(); ++i )
      {
      if ( i < 0 )
         break;

      DELTA &delta = ::EnvGetDelta( deltaArray, i );

      bool processThisDelta = false;

      for ( int j=0; j < (int) m_useDeltas.GetSize(); j++ )
         {
         USE_DELTA &ud = m_useDeltas.GetAt( j );
         if ( delta.col == ud.col && ud.value == delta.newValue )
            {
            // does it pass the query?
            bool passQuery = true;
            if ( pExpr->m_pQuery != nullptr )
               pExpr->m_pQuery->Run( delta.cell, passQuery );

            if ( passQuery )
               {
               processThisDelta = true;
               break;
               }
            }
         }

      if ( processThisDelta )
         {
         iduCount++;

         m_pCollection->UpdateVariables( pEnvContext, delta.cell, true );  // always use useAddDelta
         UpdateVariables( pEnvContext, delta.cell, true );

         // done evalating all inputs - run model formula(s)                     
         ASSERT( pExpr->m_pParser != nullptr );
         double iduValue;
         if ( ! pExpr->m_pParser->evaluate( iduValue ) )
            iduValue = 0;

         switch( m_evalMethod )
            {
            case EM_SUM:    // sum the scores over the entire coverage
            case EM_MEAN:    // compute the average score over the entire coverage
               m_rawScore += (float) iduValue;
               break;

            case EM_AREAWTMEAN:  // compute an area-weighted average value over the entire coverage
            case EM_DENSITY:
               {
               float area;
               pLayer->GetData( delta.cell, colArea, area );
               m_rawScore += float( iduValue ) * area;
               }
               break;
            }

         if ( m_col >= 0 )
            {
            float oldValue;
            pLayer->GetData( delta.cell, m_col, oldValue );

            if ( oldValue != iduValue )
               AddDelta( pEnvContext, delta.cell, m_col, (float) iduValue ); // populate the IDU database
            }
         }  // end of: if(  processThisDelta )
      }  // end of: deltaArray loop

   // normalize the scores, depending on the evalMethod
   if ( iduCount == 0 )
      {
      pEnvContext->score = pEnvContext->rawScore = 0;
      return TRUE;
      }

   switch( m_evalMethod )
      {
      case EM_SUM:    // sum the scores over the entire coverage
      case EM_DENSITY:
         break;         // no additional processing needed

      case EM_MEAN:    // compute the average score over the entire coverage
         m_rawScore /= iduCount;
         break;

      case EM_AREAWTMEAN:  // compute an area-weighted average value over the entire coverage
         {
         float totalArea = pLayer->GetTotalArea();
         m_rawScore /= totalArea;
         }
         break;
      }

   return TRUE;
   }


bool ModelProcess::Init(EnvContext*, LPCTSTR)
   {
   return true;
   }

bool ModelProcess::InitRun(EnvContext*)
   {
   return true;
   }


bool ModelProcess::Run( EnvContext *pEnvContext )
   {
   //if ( m_col < 0 )
   //   return true;

   bool useAddDelta = true;
   m_pCollection->UpdateGlobalExtentVariables(pEnvContext, 0, useAddDelta); // this loads all parserVars and evaluates ModelCollection extent variables for the current IDU

   if ( this->m_useDelta )
      {
      BOOL retVal = RunDelta( pEnvContext );

      if ( retVal )
        m_pCollection->UpdateGlobalExtentVariables( pEnvContext, 1, useAddDelta ); // this loads all parserVars and evaluates ModelCollection extent variables for the current IDU

      return retVal;
      }

   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;

   INT_PTR exprCount = m_exprArray.GetSize();

   if ( exprCount > 0 )
      {
      int iduCount = 0;
    
      // iterate over ACTIVE records
      //#pragma omp parallel for private( v, type, area, result, iduValue ) reduction (iduCount, m_rawScore )

      bool *runExpr = new bool[ exprCount ];

      for ( MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++ )
         {
         bool isIduUpdated = false;
         bool runIDU = false;
         bool multiUpdateCheck=false;

         for ( INT_PTR i=0; i < exprCount; i++ )
            {
            Expression *pExpr = m_exprArray[ i ];

            if ( pExpr->m_pQuery == nullptr )
               {
               runExpr[ i ] = true;
               runIDU = true;
               }
            else 
               {
               bool result;
               pExpr->m_pQuery->Run( idu, result );
               runExpr[ i ] = result;

               if ( result )
                  runIDU = true;
               }

            if ( runExpr[ i ] )
               {
               if ( multiUpdateCheck == true )
                  {
                  CString msg(_T("  Multiple updates to Modeler column:  ModelProcess: ") );
                  msg += this->m_name;
                  msg  += _T(", Query: ");
                  if ( pExpr->m_pQuery )
                     msg += pExpr->m_queryStr;
                  else
                     msg += _T("All");
                  msg += _T("\n");

                  TRACE( msg );
                  Report::WarningMsg( msg );
                  }

               multiUpdateCheck = true;
               }
            }

         if ( runIDU )
            {
            iduCount = iduCount+1;

            if ( ! isIduUpdated )
               {
               m_pCollection->UpdateVariables( pEnvContext, idu, useAddDelta ); // this loads all parserVars and evaluates ModelCollection scope variables for the current IDU
               UpdateVariables( pEnvContext, idu, useAddDelta );     // update local variables for this model
               isIduUpdated = true;
               }

            // done evalating all inputs - run model formula(s)
            double iduValue = 0;
            for ( INT_PTR i=0; i < exprCount; i++ )
               {
               if ( runExpr[ i ] )
                  {
                  Expression *pExpr = m_exprArray[ i ];

                  ASSERT( pExpr->m_pParser != nullptr );
                  if ( ! pExpr->m_pParser->evaluate( iduValue ) )
                     iduValue = 0;
                  }
               }

            if ( m_col >= 0 )   // populate the IDU database?
               {
               TYPE type = pLayer->GetFieldType( m_col );

               VData v;
               v.type = type;
               v.SetKeepType( iduValue );

               if ( useAddDelta )
                  {
                  VData oldValue;
                  pLayer->GetData( idu, m_col, oldValue );

                  if ( oldValue.Compare( v ) == false )
                     AddDelta( pEnvContext, idu, m_col, v );  // needs a critical section!!!!!!
                  }
               else
                  pLayer->SetData( idu, m_col, v );
               }  // end of:  if ( mCol >= 0 )
            }  // end of: if ( runIDU )
         }  // end of: MapIterator

      delete [] runExpr;
      }

   m_pCollection->UpdateGlobalExtentVariables( pEnvContext, 1, useAddDelta ); // this loads all parserVars and evaluates ModelCollection extent variables for the current IDU

   return true;
   }


BOOL ModelProcess::RunDelta( EnvContext *pEnvContext )
   {
   if ( m_col < 0 )
      return true;

   ASSERT( m_useDelta == true );

   const MapLayer *pLayer = pEnvContext->pMapLayer;

   // Load variables
   int   colCount = pLayer->GetColCount();
   int   iduCount = 0;

   // load current values into the m_parserVars array 
   DeltaArray *deltaArray = pEnvContext->pDeltaArray;// get a ptr to the delta array

   //HACK FOR NOW - Assume one and only on expression
   ASSERT( m_exprArray.GetSize() == 1 );
   Expression *pExpr = m_exprArray[ 0 ] ;

   // iterate through deltas added since last “seen”
   for ( INT_PTR i=pEnvContext->firstUnseenDelta; i < deltaArray->GetSize(); ++i )
      {   
      DELTA &delta = ::EnvGetDelta( deltaArray, i );

      bool processThisDelta = false;
      for ( int j=0; j < (int) m_useDeltas.GetSize(); j++ )
         {
         USE_DELTA &ud = m_useDeltas.GetAt( j );
         if ( delta.col == ud.col && ud.value == delta.newValue )
            {
            processThisDelta = true;
            break;
            }

         if ( processThisDelta )
            {     
            m_pCollection->UpdateVariables( pEnvContext, delta.cell, true );
            UpdateVariables( pEnvContext, delta.cell, true );

            // done evalating all inputs - run model formula
            ASSERT( pExpr->m_pParser != nullptr );
            double iduValue;
            if ( ! pExpr->m_pParser->evaluate( iduValue ) )
               iduValue = 0;

            float oldValue;
            pLayer->GetData( delta.cell, m_col, oldValue );

            if ( m_col >= 0 && oldValue != iduValue )
               AddDelta( pEnvContext, delta.cell, m_col, (float) iduValue ); // populate the IDU database
            }  /// end of: matching delta
         }

      }  // end of: deltaArray loop

   return TRUE;
   }




Evaluator *ModelCollection::GetEvaluatorFromID( int id )
   {
   for ( INT_PTR i=0; i < m_evaluatorArray.GetSize(); i++ )
      {
      if ( m_evaluatorArray[ i ]->m_id == id )
         return m_evaluatorArray[ i ];
      }

   return nullptr;
   }


ModelProcess *ModelCollection::GetModelProcessFromID( int id )
   {
   for ( INT_PTR i=0; i < m_modelProcessArray.GetSize(); i++ )
      {
      if ( m_modelProcessArray[ i ]->m_id == id )
         return m_modelProcessArray[ i ];
      }

   return nullptr;
   }



bool ModelCollection::UpdateVariables( EnvContext *pEnvContext, int idu, bool useAddDelta )
   {
   // updates  variables for this model collection.  if 'idu' < 0, updates global extent variables, otherwise updates IDU extent variables
   ASSERT ( idu >= 0 );

   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   int colCount = pIDULayer->GetFieldCount();

   // (NOTE: THIS SHOULD ONLY HAPPEN DURING THE FIRST INVOCATION OF A MODEL IN A GIVEN TIMESTEP...
   for ( int j=0; j < colCount; j++ )
      {
      VData value(0);
      float fValue = 0;

      bool okay = pIDULayer->GetData( idu, j, value );

      if ( okay && value.GetAsFloat( fValue ) )
         gpModeler->m_parserVars[ j ] = fValue;
      }

   // evaluate lookups
   for ( int j=0; j < this->m_lookupTableArray.GetSize(); j++ )
      {
      LookupTable *pTable = this->m_lookupTableArray[ j ];
      pTable->Evaluate( idu );
      
      if ( pTable->m_colOutput >= 0 )
         {
         if ( useAddDelta )
            {
            int hModel = pEnvContext->pEnvExtension->m_handle;
            pEnvContext->ptrAddDelta( pEnvContext->pEnvModel, idu, pTable->m_colOutput, pEnvContext->currentYear, VData( pTable->m_value ), hModel );
            }
         else
            {
            pIDULayer->SetData( idu, pTable->m_colOutput, pTable->m_value );
            }
         }
      }

   // evaluate histograms
   for ( int j=0; j < this->m_histogramsArray.GetSize(); j++ )
      {
      HistogramArray *pHistoArray = this->m_histogramsArray[ j ];
      double prevVal = pHistoArray->m_value;
      double value;
      bool ok = pHistoArray->Evaluate( idu, value );
      //if ( m_constraints == C_INCREASE_ONLY && value < prevVal )
      //   pHistoArray->m_value = prevVal;
      ASSERT( ok );
      }

   // evaluate variables
   for ( int j=0; j < this->m_variableArray.GetSize(); j++ )
      {
      Variable *pVar = this->m_variableArray[ j ];
      if ( pVar->m_extent == EXTENT_IDU )
         {
         gCurrentIDU = idu;
         bool ok = pVar->Evaluate();
         gCurrentIDU = -1;

         ASSERT( ok );

         if ( pVar->m_col >= 0 )
            {
            bool ok = true;
            if ( pVar->m_pQuery != nullptr )
               {
               bool result = false;
               bool ok2 = pVar->m_pQuery->Run( idu, result );
               
               if ( !ok2 || !result )
               ok = false;
               }

            if ( ok )
               {
               if ( useAddDelta )
                  {
                  int hModel = pEnvContext->pEnvExtension->m_handle;
                  pEnvContext->ptrAddDelta( pEnvContext->pEnvModel, idu, pVar->m_col, pEnvContext->currentYear, VData( pVar->m_value ), hModel );
                  }
               else
                  {
                  pIDULayer->SetData( idu, pVar->m_col, pVar->m_value );
                  }
               }
            } 
         }
      }

   // note: no need to evaluate constants - once is enough
   return true;
   }


// Note: this updates ALL global-extent variables, regardless of scope, in this collection
bool ModelCollection::UpdateGlobalExtentVariables( EnvContext *pEnvContext, int timing, bool useAddDelta )
   {
   VariableArray va;
   va.m_deleteOnDelete = false;
   int globalCount = CollectGlobalExtentVariables( va, timing );

   if ( globalCount == 0 )
      return true;

   int deltaCount = 0;
   int mapCount   = 0;

   for ( int i=0; i < globalCount; i++ )
      {
      va[ i ]->m_cumValue = 0;
      va[ i ]->m_queryArea = 0;

      if ( va[ i ]->m_useDelta )
         deltaCount++;
      else
         mapCount++;
      }

   // global variables exist.  Evaluate them.  SOme of these may be delta-based, others may be map based.
   // Do Delta-based ones first, followed be map-based ones.
   if ( deltaCount > 0 && pEnvContext->pDeltaArray != nullptr )
      {
      DeltaArray *deltaArray = pEnvContext->pDeltaArray;// get a ptr to the delta array

      int iduCount = 0;
   
      // iterate through deltas added since last “seen”
      for ( INT_PTR i=pEnvContext->firstUnseenDelta; i < deltaArray->GetSize(); ++i )
         {
         if ( i < 0 )
            break;

         DELTA &delta = ::EnvGetDelta( deltaArray, i );

         bool processThisDelta = false;

         for ( int j=0; j < globalCount; j++ )
            {
            Variable *pVar = va[ j ];
            pVar->m_evaluate = false;

            if ( pVar->m_useDelta == false )
               continue;

            for ( int k=0; k < (int) pVar->m_useDeltas.GetSize(); k++ )
               {
               USE_DELTA &ud = pVar->m_useDeltas.GetAt( k );
               if ( delta.col == ud.col && ud.value == delta.newValue )
                  {
                  // does it pass the query?
                  bool passQuery = true;
                  if ( pVar->m_pQuery != nullptr )
                     pVar->m_pQuery->Run( delta.cell, passQuery );

                  if ( passQuery )
                     {
                     processThisDelta = true;
                     pVar->m_evaluate = true;
                     break;
                     }
                  }
               }
            }  // end of : for ( j < globalCount )

         if ( processThisDelta )
            {
            iduCount++;

            //m_pCollection->UpdateVariables( delta.cell );
            UpdateVariables( pEnvContext, delta.cell, useAddDelta );  // this updates ModelCollection-scope variables only

            float area;
            ASSERT( Modeler::m_colArea >= 0 );
            gpModeler->m_pMapLayer->GetData( delta.cell, Modeler::m_colArea, area );

            // done evalating all inputs - run model formula(s) 
            for ( int j=0; j < globalCount; j++ )
               {
               Variable *pVar = va[ j ];
               if ( pVar->m_evaluate )
                  {
                  gCurrentIDU = delta.cell;
                  pVar->Evaluate();
                  gCurrentIDU = delta.cell;

                  pVar->m_queryArea += area;

                  switch( pVar->m_varType )
                     {
                     case VT_SUM_AREA:
                        pVar->m_cumValue += (float) pVar->m_value;
                        break;

                     case VT_AREAWTMEAN:
                     case VT_DENSITY:
                        pVar->m_cumValue += (float) pVar->m_value * area; 
                        break;

                     case VT_PCT_AREA:
                     //case VT_PCT_CONTRIB_AREA:
                        //pVar->m_cumValue += area;
                        break;       
                     } 
                  }  // end of:  if ( pVar->m_evaluate )
               }  // end of:  for ( j < globalCount );
            }  // end of: if ( processThisDelta )
         }  // end of: for ( i < deltaArray.GetSize() )         
      }  // end of: if ( deltaCount > 0 )

   if ( mapCount > 0 )
      {
      for ( MapLayer::Iterator i = gpModeler->m_pMapLayer->Begin(); i != gpModeler->m_pMapLayer->End(); i++ )
         {
         bool evalIDU = false;

         for ( int j=0; j < globalCount; j++ )
            {
            Variable *pVar = va[ j ];
            pVar->m_evaluate = false;

            if ( pVar->m_useDelta )
               continue;
         
            if ( pVar->m_pQuery == nullptr || pVar->m_query.GetLength() == 0 )
               {
               pVar->m_evaluate = true;
               evalIDU = true;
               }
            else
               {
               bool result;
               bool ok = pVar->m_pQuery->Run( i, result );

               if ( ok && result )
                  {
                  pVar->m_evaluate = true;
                  evalIDU = true;
                  }
               }
            }  // end of: if ( j < varCount )

         if ( evalIDU )
            {
            UpdateVariables( pEnvContext, i, useAddDelta ); // update ModelCollection-scope variables only
            float area;
            gpModeler->m_pMapLayer->GetData( i, Modeler::m_colArea, area );

            for ( int j=0; j < globalCount; j++ )
               {
               Variable *pVar = va[ j ];

               // does this variable need to be evaluated? (i.e. was its query satisfied?)
               if ( pVar->m_evaluate == true )
                  {
                  pVar->Evaluate();
                  pVar->m_queryArea += area;

                  switch( pVar->m_varType )
                     {
                     case VT_SUM_AREA:
                        pVar->m_cumValue += (float) pVar->m_value;
                        break;

                     case VT_AREAWTMEAN:
                     case VT_DENSITY:
                        pVar->m_cumValue += (float) pVar->m_value * area; 
                        break;

                     case VT_PCT_AREA:
                     //case VT_PCT_CONTRIB_AREA:
                        break;                     
                     } 
                  }  // end of:  if ( pVar->m_evaluate )
               } // end of: if ( j < varCount )
            }  // end of: if ( evalIDU )
         } // end of: for ( i < iduCount )
      }

   for ( int j=0; j < globalCount; j++ )
      {
      Variable *pVar = va[ j ];

      switch( pVar->m_varType )
         {
         case VT_SUM_AREA:
         case VT_DENSITY:
            pVar->m_value = pVar->m_cumValue;
            break;

         case VT_AREAWTMEAN:
            pVar->m_value = pVar->m_cumValue / pVar->m_queryArea;
            break;

         case VT_PCT_AREA:
         //case VT_PCT_CONTRIB_AREA:
            {
            float totalArea = gpModeler->m_pMapLayer->GetTotalArea();
            pVar->m_value = pVar->m_queryArea*100/totalArea;
            }
            break;

		 case VT_GLOBALEXT:
			 break;

         default:
            ASSERT( 0 );
         }
      } // end of: if ( j < varCount )  

   // last, evaluate globalext variables
   for ( int j=0; j < globalCount; j++ )
      {
      Variable *pVar = va[ j ];

      if ( pVar->m_varType == VT_GLOBALEXT )   // these are based only on other global extent variables, so they only
         {                                     // are only evaluate here.
         ASSERT( pVar->m_pParser != nullptr );
         if ( ! pVar->m_pParser->evaluate( pVar->m_value ) )
            pVar->m_value = 0;
         }
      }

   return true;
   }


// Note: this gets ALL global-extent variables, regardless of scope
int ModelCollection::CollectGlobalExtentVariables( VariableArray &va, int timing )
   {
   va.Clear();

   // any global extent model collection variables?
   bool globalCount = 0;
   INT_PTR varCount = this->m_variableArray.GetSize();
   for ( INT_PTR j=0; j < varCount; j++ )
      {
      Variable *pVar = m_variableArray[ j ];
      if ( pVar->m_extent == EXTENT_GLOBAL && pVar->m_timing == timing )
         va.Add( pVar );
      }

   for ( INT_PTR i=0; i < this->m_evaluatorArray.GetSize(); i++ )
      {
      Evaluator *pModel = m_evaluatorArray[ i ];

      for ( INT_PTR j=0; j < pModel->m_variableArray.GetSize(); j++ )
         {
         Variable *pVar = pModel->m_variableArray[ j ];

         if ( pVar->m_extent == EXTENT_GLOBAL && pVar->m_timing == timing )
            va.Add( pVar );
         }
      }

   for ( INT_PTR i=0; i < this->m_modelProcessArray.GetSize(); i++ )
      {
      ModelProcess *pModel = m_modelProcessArray[ i ];

      for ( INT_PTR j=0; j < pModel->m_variableArray.GetSize(); j++ )
         {
         Variable *pVar = pModel->m_variableArray[ j ];

         if ( pVar->m_extent == EXTENT_GLOBAL && pVar->m_timing == timing )
            va.Add( pVar );
         }
      }

   return (int) va.GetSize();
   }


void ModelCollection::Clear()
   {
   m_fileName.Empty();
   
   m_modelProcessArray.RemoveAll();  // memory managed by EnvModel for these two
   m_evaluatorArray.RemoveAll();
   m_histogramsArray.RemoveAll();
   m_lookupTableArray.RemoveAll();
   m_constantArray.RemoveAll();
   m_variableArray.RemoveAll();
   }


// LoadXml() is called during factory creation
bool ModelCollection::LoadXml( LPCTSTR _filename, MapLayer *pLayer, bool isImporting )
   {
   CString filename;
   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "  Input file '%s' not found - this process will be disabled", _filename );
      Report::ErrorMsg( msg );
      return false;
      }

   bool loadSuccess = true;

   if ( ! isImporting )
      {
      m_fileName = filename;
      gpModeler->m_pMapLayer = pLayer;
      }

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   if ( ! ok )
      {      
      Report::ErrorMsg( doc.ErrorDesc() );
      return false;
      }
 
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();

   LPCTSTR checkCols = pXmlRoot->Attribute("check_cols");
   if (checkCols != nullptr)
      {
      CStringArray tokens;
      int count = ::Tokenize(checkCols, _T(",;"), tokens);

      int col = 0;
      for (int i = 0; i < count; i++)
         {
         CStringArray token;
         int _count = ::Tokenize(tokens[i], _T(":"), token);

         TYPE type = TYPE_INT;
         if (_count == 2)
            {
            switch (_tolower(token[1][0]))
               {
               case 'f':   type = TYPE_FLOAT;   break;
               case 'd':   type = TYPE_DOUBLE;  break;
               case 's':   type = TYPE_STRING;  break;
               case 'l':   type = TYPE_LONG;    break;
               }
            }
         pLayer->CheckCol(col, token[0], type, CC_AUTOADD);
         }
      }

   TiXmlNode *pXmlNode = nullptr;
   while( pXmlNode = pXmlRoot->IterateChildren( pXmlNode ) )
      {
      if ( pXmlNode->Type() != TiXmlNode::ELEMENT )
            continue;

      if ( lstrcmpi( _T("var"), pXmlNode->Value()) == 0 )      // note: reports must be at the Model level
         {
         if ( ! LoadVariable( pXmlNode, m_variableArray, false ) )
            loadSuccess = false;
         else
            {
            Variable *pVar = m_variableArray.GetAt( m_variableArray.GetSize()-1 );     // one just added
            pVar->m_pCollection = this;
            pVar->Compile( nullptr, this, pLayer );
            }
         }

      else if ( lstrcmpi( _T("const"), pXmlNode->Value() )== 0 )
         {
         if ( ! LoadConstant( pXmlNode, m_constantArray, false ) )
            loadSuccess = false;
         }

      else if ( lstrcmpi( _T("input"), pXmlNode->Value() )== 0 )
         {
         if ( ! LoadConstant( pXmlNode, m_constantArray, true ) )
            loadSuccess = false;
         }

      else if ( lstrcmpi( _T("lookup"), pXmlNode->Value() )== 0 )
         {
         if ( ! LoadLookup( pXmlNode, m_lookupTableArray, pLayer ) )
            loadSuccess = false;
         }

      else if ( lstrcmpi( _T("histograms"), pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadHistograms( pXmlNode, m_histogramsArray, pLayer ) )
            loadSuccess = false;
         }

      // is this an include file?
      else if ( lstrcmpi( _T("include"), pXmlNode->Value() ) == 0 )
         {
         TiXmlElement *pXmlElement = pXmlNode->ToElement();
         LPCTSTR file = pXmlElement->Attribute( "file" );
         if ( file == nullptr )
            {
            Report::ErrorMsg( _T("No 'file' attribute specified for <include> element") );
            continue;
            }
         if ( ! LoadXml( file, pLayer, true ) )
            loadSuccess = false;
         }

      // is this a model
      else if ( lstrcmpi( "evaluator", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadEvaluator( pXmlNode, pLayer, filename ) )
            loadSuccess = false;
		    else
			   {
            Evaluator *pModel = m_evaluatorArray.GetAt( m_evaluatorArray.GetCount()-1 ); 
            ASSERT( pModel != nullptr );
            pModel->SetVars( (EnvExtension*) pModel );
            ////pModel->Init( pEnvContext, initStr );
            }
         }

      // is this a process
      else if ( lstrcmpi( "model", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadModelProcess( pXmlNode, pLayer, filename) )
            loadSuccess = false;
         else
            {
            ModelProcess *pModel = m_modelProcessArray.GetAt( m_modelProcessArray.GetCount()-1 ); 
            ASSERT( pModel != nullptr );
            pModel->SetVars( (EnvExtension*) pModel );
            }
         }
      else {
         CString msg;
         msg.Format("  Unrecognized xml element <%s> encountered reading file %s", pXmlNode->Value(), filename);
         Report::LogWarning(msg);
         }
      }

   // all done loading xml, compile any ModelCollection-scope variables
   //for ( int i=0; i < (int) m_variableArray.GetSize(); i++ )
   //   {
   //   Variable *pVar = m_variableArray.GetAt( i );
   //   pVar->Compile( nullptr, this, pLayer );
   //   }

   return loadSuccess;
   }


bool ModelCollection::LoadEvaluator( TiXmlNode *pXmlNode, MapLayer *pLayer, LPCTSTR filename )
   {
   bool loadSuccess = true;
   if ( pXmlNode->Type() != TiXmlNode::ELEMENT )
      return false;
      
   // process Model element
   //
   // <eval_model name="Impervious Surfaces (%)" col="EM_IMPERV" value="FRACIMPERV" 
   //      evalType="LOW" lowerBound="0.10" upperBound="0.15" method="AREAWTAVG"/>
   //
   Evaluator* pEval = new Evaluator(this);
   LPCTSTR constraints=nullptr, evalType=nullptr, method=nullptr, type=nullptr;
   LPCTSTR lb = nullptr, ub = nullptr;

   XML_ATTR attrs[] = {
      // attr                     type          address                     isReq  checkCol
      { _T("name"),              TYPE_CSTRING,  &pEval->m_modelName,       true,    0 },
      { _T("query"),             TYPE_CSTRING,  &pEval->m_query,           false,   0 },
      { _T("value"),             TYPE_CSTRING,  &pEval->m_valueExpr,       true,    0 },
      { _T("use_delta"),         TYPE_CSTRING,  &pEval->m_deltaStr,        false,   0 },
      { _T("eval_type"),         TYPE_STRING,   &evalType,                 true,    0 },
      //{ _T("lower_bound"),       TYPE_FLOAT,    &pEval->m_lowerBound,      true,    0 },
      //{ _T("upper_bound"),       TYPE_FLOAT,    &pEval->m_upperBound,      true,    0 },
      { _T("lower_bound"),       TYPE_STRING,   &lb,                        true,    0 },
      { _T("upper_bound"),       TYPE_STRING,   &ub,                        true,    0 },
      { _T("method"),            TYPE_STRING,   &method,                   true,    0 },
      { _T("constraints"),       TYPE_STRING,   &constraints,              false,   0 },
      { _T("type"),              TYPE_STRING,   &type,                     false,   0 },

      { _T("path"),              TYPE_CSTRING,  &pEval->m_path,            false,   0 },
      { _T("col"),               TYPE_CSTRING,  &pEval->m_fieldName,       false,   CC_AUTOADD | TYPE_FLOAT },
      { _T("initInfo"),          TYPE_CSTRING,  &pEval->m_initInfo,        false,   0 },
      { _T("use"),               TYPE_INT,      &pEval->m_use,             false,   0 },
      //{ _T("freq"),              TYPE_FLOAT,    &pEval->m_freq,               false,   0 },
      //{ _T("showInResults"),     TYPE_INT,      &pEval->m_showInResults,      false,   0 },
      //{ _T("gain"),              TYPE_FLOAT,    &pEval->m_gain,               false,   0 },
      //{ _T("offset"),            TYPE_FLOAT,    &pEval->m_offset,             false,   0 },
      //{ _T("initRunOnStartup"),  TYPE_INT,      &pEval->m_initRunOnStartup,   false,   0 },   // deprecated
      { _T("dependencies"),      TYPE_CSTRING,  &pEval->m_dependencyNames, false,   0 },
      { _T("description"),       TYPE_CSTRING,  &pEval->m_description,     false,   0 },
      { _T("imageURL"),          TYPE_CSTRING,  &pEval->m_imageURL,        false,   0 },
      { nullptr,                    TYPE_NULL,     nullptr,                false,   0 } };

   bool ok = TiXmlGetAttributes(pXmlNode->ToElement(), attrs, filename, pLayer);
   if (!ok)
      {
      CString msg;
      msg.Format(_T("  Misformed element reading <evaluator> attributes in input file %s"), filename);
      Report::ErrorMsg(msg);
      return false;
      }

   if (lb == nullptr || lb[0] == 'a' || ub == nullptr || ub[0] == 'a')
      pEval->m_autoBounds=true;
   else
      {
      pEval->m_autoBounds = false;
      pEval->m_lowerBound = atof(lb);
      pEval->m_upperBound = atof(ub);
      }

   EVAL_METHOD _evalMethod = EM_AREAWTMEAN;
   if (method != nullptr)
      {
      if (method[0] == 'S' || method[0] == 's')
         _evalMethod = EM_SUM;
      else if (method[0] == 'A' || method[0] == 'a')
         {
         if (method[1] == 'V' || method[1] == 'v')
            _evalMethod = EM_MEAN;
         }
      else if (method[0] == 'p' || method[0] == 'P')
         _evalMethod = EM_PCT_AREA;
      else if (method[0] == 'd' || method[0] == 'D')
         _evalMethod = EM_DENSITY;
      }
      
   pEval->m_evalMethod = _evalMethod;

   EVAL_TYPE _evalType;
   switch( evalType[ 0 ] )
      {
      case 'H':   _evalType = ET_VALUE_HIGH;    break;
      case 'h':   _evalType = ET_VALUE_HIGH;    break;
      case 'L':   _evalType = ET_VALUE_LOW;     break;
      case 'l':   _evalType = ET_VALUE_LOW;     break;
      //case 'I':   _evalType = ET_TREND_INCREASING;    break;
      //case 'D':   _evalType = ET_TREND_DECREASING;    break;
      default:
         {
         CString msg( "  Invalid 'evalType' attribute specified for model " );
         msg += pEval->m_modelName;
         Report::LogError( msg );
         delete pEval;
         return false;
         }
      }

   pEval->m_evalType = _evalType;

   if ( pEval->m_deltaStr.GetLength() > 0 )
      {
      pEval->m_useDelta = true;

      //pModel->ParseUseDelta( useDelta );
      ParseUseDelta( pEval->m_deltaStr, pEval->m_useDeltas );
      }
 
   if ( constraints )
      {
      if ( constraints[ 0 ] == 'I' )  // INCREASE
         pEval->m_constraints = C_INCREASE_ONLY;
      else if ( constraints[ 0 ] == 'D' )
         pEval->m_constraints = C_DECREASE_ONLY;
      }

   //if ( type != nullptr )
   //   {
   //   if ( lstrcmpi( type, "raster" ) == 0 )
   //      {
   //      LPCTSTR _cellSize = pXmlElement->Attribute( "cell_size" );
   //      if ( _cellSize == nullptr )
   //         {
   //         CString msg;
   //         msg.Format( "'cell_size' attribute missing from raster model '%s' - this is a required element", name );
   //         Report::ErrorMsg( msg );
   //         delete pModel;
   //         return false;
   //         }
   //
   //      float cellSize = (float) atof( _cellSize );
   //
   //      //pModel->m_pRaster = new Raster( pLayer, nullptr, nullptr, false ); 
   //      }
   //   }

   // make a parser for this formula and add to the parser array
   //ASSERT( pModel->m_pParser == nullptr );

   //MParser *pParser = new MParser; 
   //ASSERT( pParser != nullptr );
   if ( _evalMethod == EM_PCT_AREA && pEval->m_valueExpr.IsEmpty())
      pEval->m_valueExpr = "1";

   if (pEval->m_valueExpr.GetLength() > 0)
      pEval->AddExpression(pEval->m_valueExpr, pEval->m_query );

   pEval->LoadModelBaseXml( pXmlNode, pLayer );

   // Compile model    note: Parser set up later
   pEval->Compile( pLayer );

   // store this parser
   m_evaluatorArray.Add( pEval ); 
      
   return loadSuccess;
   }


bool ModelCollection::LoadModelProcess( TiXmlNode *pXmlNode, MapLayer *pLayer, LPCTSTR filename )
   {
   bool loadSuccess = true;
   if ( pXmlNode->Type() != TiXmlNode::ELEMENT )
      return false;
      
   // process Model element
   //
   // <model name="Impervious Surfaces (%)" col="EM_IMPERV" value="FRACIMPERV" />
   //

   ModelProcess* pModel = new ModelProcess(this);
   LPCTSTR constraints;

   XML_ATTR attrs[] = {
      // attr                     type          address                     isReq  checkCol
      { _T("name"),              TYPE_CSTRING,  &pModel->m_modelName,       true,    0 },
      { _T("query"),             TYPE_CSTRING,  &pModel->m_query,           false,   0 },
      { _T("value"),             TYPE_CSTRING,  &pModel->m_valueExpr,       true,    0 },
      { _T("use_delta"),         TYPE_CSTRING,  &pModel->m_deltaStr,        false,   0 },
      { _T("constraints"),       TYPE_STRING,   &constraints,               false,   0 },

      { _T("path"),              TYPE_CSTRING,  &pModel->m_path,            false,   0 },
      { _T("col"),               TYPE_INT,      &pModel->m_col,             false,   CC_AUTOADD | TYPE_FLOAT },
      { _T("initInfo"),          TYPE_CSTRING,  &pModel->m_initInfo,        false,   0 },
      { _T("use"),               TYPE_INT,      &pModel->m_use,             false,   0 },
      //{ _T("showInResults"),     TYPE_INT,      &pModel->m_showInResults,      false,   0 },
      { _T("dependencies"),      TYPE_CSTRING,  &pModel->m_dependencyNames, false,   0 },
      { _T("description"),       TYPE_CSTRING,  &pModel->m_description,     false,   0 },
      { _T("imageURL"),          TYPE_CSTRING,  &pModel->m_imageURL,        false,   0 },
      { nullptr,                    TYPE_NULL,     nullptr,                false,   0 } };

   bool ok = TiXmlGetAttributes(pXmlNode->ToElement(), attrs, filename, nullptr);
   if (!ok)
      {
      CString msg;
      msg.Format(_T("  Misformed element reading <evaluator> attributes in input file %s"), filename);
      Report::ErrorMsg(msg);
      return false;
      }


   if ( pModel->m_deltaStr.GetLength() > 0 )
      {
      pModel->m_useDelta = true;
      //pModel->ParseUseDelta( useDelta );
      ParseUseDelta( pModel->m_deltaStr, pModel->m_useDeltas );
      }

   if ( constraints )
      {
      if ( constraints[ 0 ] == 'I' )  // INCREASE
         pModel->m_constraints = C_INCREASE_ONLY;
      else if ( constraints[ 0 ] == 'D' )
         pModel->m_constraints = C_DECREASE_ONLY;
      }

   if ( pModel->m_valueExpr.GetLength() > 0 )
      pModel->AddExpression( pModel->m_valueExpr, pModel->m_query );

   pModel->LoadModelBaseXml( pXmlNode, pLayer );

   // Compile modelnote: Parser set up later
   pModel->Compile( pLayer );

   // store this parser
   m_modelProcessArray.Add( pModel ); 
   
   return loadSuccess;
   }



      
//============================================================================//
//==================== M O D E L E R   M E T H O D S =========================//
//============================================================================//

Modeler::Modeler()
: m_isInitialized( false )
, m_pCurrentCollection( nullptr )
   {
   ASSERT( m_pQueryEngine == nullptr );
   }


Modeler::~Modeler()
   {
   if ( m_parserVars != nullptr )
      {
      delete [] m_parserVars;
      m_parserVars = nullptr;
      }

   Clear();
   }


void Modeler::Clear()
   {
   for ( int i=0; i < m_modelCollectionArray.GetSize(); i++ )
      { 
      m_modelCollectionArray[ i ]->Clear();
      delete m_modelCollectionArray[ i ];
      } 

   m_modelCollectionArray.RemoveAll(); 
   }



Evaluator *Modeler::GetEvaluatorFromID( int id )
   {
   for ( INT_PTR i=0; i < m_modelCollectionArray.GetSize(); i++ )
      {
      Evaluator *pModel = m_modelCollectionArray[ i ]->GetEvaluatorFromID( id );

      if ( pModel != nullptr )
         return pModel;
      }

   return nullptr;
   }

ModelProcess *Modeler::GetModelProcessFromID( int id )
   {
   for ( INT_PTR i=0; i < m_modelCollectionArray.GetSize(); i++ )
      {
      ModelProcess *pProcess = m_modelCollectionArray[ i ]->GetModelProcessFromID( id );

      if ( pProcess != nullptr )
         return pProcess;
      }

   return nullptr;
   }


//--------------------------------------------------------------------------------
//           eval model entry points
//--------------------------------------------------------------------------------


bool Modeler::Init(EnvContext* pEnvContext)
   {
   simTime = pEnvContext->currentYear;

   // take care of static stuff if needed
   m_pMapLayer = (MapLayer*)pEnvContext->pMapLayer;
   Modeler::m_colArea = m_pMapLayer->GetFieldCol(_T("Area"));

   // make a query engine if one doesn't exist already
   m_pQueryEngine = pEnvContext->pQueryEngine;

   // allocate field vars if not already allocated
   if (m_parserVars == nullptr)
      {
      int fieldCount = m_pMapLayer->GetFieldCount();
      m_parserVars = new double[fieldCount];
      }

   // proceed with loading the file specified by the initStr
   ModelCollection* pCollection = new ModelCollection;

   if (pCollection->LoadXml(pEnvContext->initInfo, m_pMapLayer, false))
      {
      m_modelCollectionArray.Add(pCollection);
      EnvModel* pEnvModel = pEnvContext->pEnvModel;

      // add any new models  to the EnvModel
      for (int i = 0; i < pCollection->m_modelProcessArray.GetSize(); i++)
         {
         ModelProcess* pModel = pCollection->m_modelProcessArray[i];
         if (pModel != nullptr)
            {
            pModel->m_id = i;
            pModel->m_name = pModel->m_modelName;              // name of the model
            //pModel->m_path = "";              // path to the dll
            //pModel->m_initInfo = pEnvContext->initInfo;      // string passed to the model, specified in the project file
            //pModel->m_description = ""; // description;  // description, from ENVX file
            //pModel->m_imageURL = tru;     // not currently used
            pModel->m_use = true;          // use this model during the run?
            pModel->m_hDLL = (HINSTANCE)0;
            //pModel->m_frequency;    // how often this process is executed
            //pModel->m_dependencyNames = dependencyNames;
            //pModel->m_showInResults = true; // showInResults;

            CString msg("  Adding Model Process ");
            msg += pModel->m_name;
            Report::LogInfo(msg);

            pEnvModel->AddModelProcess(pModel);
            }
         }

      // add any new evaluators to the EnvModel
      for (int i = 0; i < pCollection->m_evaluatorArray.GetSize(); i++)
         {
         Evaluator* pEval = pCollection->m_evaluatorArray[i];
         if (pEval != nullptr)
            {
            pEval->m_id = i;
            pEval->m_name = pEval->m_modelName;              // name of the model
            //pEval->m_path = "";              // path to the dll
            //pEval->m_initInfo = pEnvContext->initInfo;      // string passed to the model, specified in the project file
            //pEval->m_description = ""; // description;  // description, from ENVX file
            //pEval->m_imageURL = tru;     // not currently used
            pEval->m_use = true;          // use this model during the run?
            pEval->m_hDLL = (HINSTANCE) 0;
            //pModel->m_frequency;    // how often this process is executed
            //pEval->m_dependencyNames = dependencyNames;
            pEval->m_showInResults = true; // showInResults;

            CString msg("  Adding Evaluator ");
            msg += pEval->m_name;
            Report::LogInfo(msg);

            pEnvModel->AddEvaluator(pEval);
            }
         }

      //pCollection->SetOutputs();
      }
   else
      {
      delete pCollection;
      return false;
      }
   
   return true;
   }

/*

BOOL Modeler::EmInit( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   simTime = pEnvContext->currentYear;

   // note: this gets called for each eval model specified in the envx file.
   Evaluator *pModel = GetEvaluatorFromID( pEnvContext->id );
   if ( pModel != nullptr && pModel->m_name != pEnvContext->pEnvExtension->m_name )   // already exists - generate error and return
      {
      CString msg;
      msg.Format( _T("The model [%s] (id:%i) conflicts with a previously specified model [%s] - change the [id] in your .envx file before proceeding..."),
         (LPCTSTR) pEnvContext->pEnvExtension->m_name, pEnvContext->id, (LPCTSTR) pModel->m_name );
      ::AfxMessageBox( msg );
      return FALSE;
      }

   if ( pModel != nullptr )   // already exist?  no other work needed
      return TRUE;

   // take care of static stuff if needed
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   Modeler::m_pMapLayer = pLayer;
   Modeler::m_colArea = pLayer->GetFieldCol( _T("Area") );

   // make a query engine if one doesn't exist already
   if (m_pQueryEngine == nullptr)
      m_pQueryEngine = pEnvContext->pQueryEngine;

   // allocate field vars if not already allocated
   if ( m_parserVars == nullptr )
      {
      int fieldCount = pLayer->GetFieldCount();
      m_parserVars = new double[ fieldCount ];
      }
  
   // proceed with loading the file specified by the initStr
   ModelCollection *pCollection = new ModelCollection;

   if ( pCollection->LoadXml( initStr, pLayer, false ) )
      {
      m_modelCollectionArray.Add( pCollection );
      
      //pCollection->SetOutputs();
      }
   else
      {
      delete pCollection;
      return FALSE;
      }



   //EmInitRun( pEnvContext );
   return TRUE;
   }


BOOL Modeler::EmInitRun( EnvContext *pEnvContext, bool )
   {
   simTime = pEnvContext->currentYear;

   Evaluator *pModel = GetEvaluatorFromID( pEnvContext->id );
   ASSERT( pModel != nullptr );
   if ( pModel == nullptr )
      return FALSE;

   return pModel->Run( pEnvContext, false );
   }


BOOL Modeler::EmRun( EnvContext *pEnvContext )
   {
   simTime = pEnvContext->currentYear;

   Evaluator *pModel = GetEvaluatorFromID( pEnvContext->id );
   ASSERT( pModel != nullptr );
   if ( pModel == nullptr )
      return FALSE;

   return pModel->Run( pEnvContext, true );
   }


BOOL Modeler::EmSetup( EnvContext *pContext, HWND hWnd )
   {
   return FALSE;
   }

int Modeler::EmInputVar( int id, MODEL_VAR** modelVar )
   {
   Evaluator *pModel = GetEvaluatorFromID( id );

   if ( pModel != nullptr )
      return pModel->InputVar( id, modelVar );
   else
      return 0;
   }

int Modeler::EmOutputVar( int id, MODEL_VAR** modelVar )
   {
   Evaluator *pModel = GetEvaluatorFromID( id );

   if ( pModel != nullptr )
      return pModel->OutputVar( id, modelVar );
   else
      return 0;
   }



BOOL Modeler::ApInit( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   simTime = pEnvContext->currentYear;

   // note: this gets called for each eval model specified in the envx file.
   ModelProcess *pModel = GetModelProcessFromID( pEnvContext->id );
   if ( pModel != nullptr && pModel->m_name != pEnvContext->pEnvExtension->m_name )   // already exists - generate error and return
      {
      CString msg;
      msg.Format( _T("The model [%s] (id:%i) conflicts with a previously specified process [%s] - change the [id] in your .envx file before proceeding..."),
         (LPCTSTR) pEnvContext->pEnvExtension->m_name, pEnvContext->id, (LPCTSTR) pModel->m_name );
      ::AfxMessageBox( msg );
      return FALSE;
      }

   if ( pModel != nullptr )   // already loaded?
      {
      ApInitRun( pEnvContext, false );
      return TRUE; 
      }

   // take care of static stuff if needed
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   Modeler::m_pMapLayer = pLayer;
   Modeler::m_colArea = pLayer->GetFieldCol( _T("Area") );

   // make a query engine if one doesn't exist already
   if (m_pQueryEngine == nullptr)
      m_pQueryEngine = pEnvContext->pQueryEngine;

   // allocate field vars if not already allocated
   if ( m_parserVars == nullptr )
      {
      int fieldCount = pEnvContext->pMapLayer->GetFieldCount();
      m_parserVars = new double[ fieldCount ];
      }
  
   // proceed with loading the file specified by the 
   ModelCollection *pCollection = new ModelCollection;

   if ( pCollection->LoadXml( initStr, (MapLayer*) pEnvContext->pMapLayer, false ) )
      {
      m_modelCollectionArray.Add( pCollection );
      }
   else
      {
      delete pCollection;
      return FALSE;
      }

   ApInitRun( pEnvContext, false );
   return TRUE;
   }


BOOL Modeler::ApInitRun( EnvContext *pEnvContext, bool )
   {
   simTime = pEnvContext->currentYear;

   ModelProcess *pModel = GetModelProcessFromID( pEnvContext->id );
   ASSERT( pModel != nullptr );
   if ( pModel == nullptr )
      return FALSE;

   return pModel->Run( pEnvContext, false );
   }


BOOL Modeler::ApRun( EnvContext *pEnvContext )
   {
   simTime = pEnvContext->currentYear;

   ModelProcess *pModel = GetModelProcessFromID( pEnvContext->id );
   ASSERT( pModel != nullptr );
   if ( pModel == nullptr )
      return FALSE;


   return pModel->Run( pEnvContext, true );
   }


BOOL Modeler::ApSetup( EnvContext *pContext, HWND hWnd )
   {
   return FALSE;
   }

int Modeler::ApInputVar( int id, MODEL_VAR** modelVar )
   {
   ModelProcess *pModel = GetModelProcessFromID( id );

   if ( pModel != nullptr )
      return pModel->InputVar( id, modelVar );
   else
      return 0;
   }

int Modeler::ApOutputVar( int id, MODEL_VAR** modelVar )
   {
   ModelProcess *pModel = GetModelProcessFromID( id );

   if ( pModel != nullptr )
      return pModel->OutputVar( id, modelVar );
   else
      return 0;
   }
   */

EnvExtension * Factory(EnvContext *pContext)
   {
   ASSERT(gpModeler == nullptr);
   if (gpModeler == nullptr)
      {
      gpModeler = new Modeler;
      gpModeler->Init(pContext);
      }

   return gpModeler;  // nullptr
   }
