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

#include <omp.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// idu field variable values shared by all parsers
double      *Modeler::m_parserVars = NULL;
int          Modeler::m_nextID = 0;
int          Modeler::m_colArea;
MapLayer    *Modeler::m_pMapLayer    = NULL;
QueryEngine *Modeler::m_pQueryEngine = NULL;
CMap<int, int, int, int > Modeler::m_usedParserVars;

Variable *gpCurrentVar = NULL;
int gCurrentIDU  = -1;


double simTime = 0;

RandUniform rn( 0, 1 );
RandUniform rn1( 0, 1 );

bool CompileExpression( ModelBase *pModel, ModelCollection *pCollection, MParser *pParser, LPCTSTR expr, MapLayer *pLayer );
bool LoadLookup( TiXmlNode *pXmlNode, LookupArray&, MapLayer* );
bool LoadConstant( TiXmlNode *pXmlNode, ConstantArray&, bool isInput  );
bool LoadVariable( TiXmlNode *pXmlNode, VariableArray&, bool isReport );
bool LoadExpr    ( TiXmlNode *pXmlNode, ModelBase* );
bool LoadHistograms( TiXmlNode *pXmlNode, HistoArray&, MapLayer* ); //, LPCTSTR defaultName=NULL, LPCTSTR defaultUse=NULL );
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
   Poly *pPoly = Modeler::m_pMapLayer->GetPolygon( gCurrentIDU );
   int count = Modeler::m_pMapLayer->GetNearbyPolys( pPoly, neighbors, NULL, 32, 0.001f );

   int retCount = 0;

   //solve with an associate query?
   if ( gpCurrentVar != NULL && gpCurrentVar->m_pValueQuery != NULL )
      {
      for ( int i=0; i < count; i++ )
         {
         int idu = neighbors[ i ];
         // check to see if neighbor passes query

         bool result = false;
         if ( Modeler::m_pQueryEngine->Run( gpCurrentVar->m_pValueQuery, idu, result ) && result == true )
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

   return NULL;
   }

LookupTable *FindLookup( LookupArray &lookupTableArray, LPCTSTR name )
   {
   for ( int i=0; i < lookupTableArray.GetSize(); i++ )
      {
      LookupTable *pTable = lookupTableArray[ i ];
      if ( pTable->m_name.Compare( name ) == 0 )
         return pTable;
      }

   return NULL;
   }

Constant *FindConstant( ConstantArray &constantArray, LPCTSTR name )
   {
   for ( int i=0; i < constantArray.GetSize(); i++ )
      {
      Constant *pConst = constantArray[ i ];
      if ( pConst->m_name.Compare( name ) == 0 )
         return pConst;
      }

   return NULL;
   }

Variable *FindVariable( VariableArray &variableArray, LPCTSTR name )
   {
   for ( int i=0; i < variableArray.GetSize(); i++ )
      {
      Variable *pVar = variableArray[ i ];
      if ( pVar->m_name.Compare( name ) == 0 )
         return pVar;
      }

   return NULL;
   }

//-- LookupTable methods ---------------------------------------------------------
bool LookupTable::Evaluate( int idu )
   {
   ASSERT( Modeler::m_pMapLayer != NULL );
   int fieldVal;

   ASSERT ( this->m_colAttr >= 0 );

   Modeler::m_pMapLayer->GetData( idu, this->m_colAttr, fieldVal );
   bool ok = Lookup( fieldVal, m_value ) ? true : false;

   if ( ! ok )
      m_value = m_defaultValue;

   return ok;
   }


//-- Variable methods ---------------------------------------------------------

Variable::~Variable()
   { 
   if ( m_pParser != NULL ) 
      delete m_pParser; 

   //if ( m_pRaster != NULL )
   //   delete m_pRaster;
   
   if ( m_pQuery ) 
      Modeler::m_pQueryEngine->RemoveQuery( m_pQuery ); 

   if ( m_pValueQuery ) 
      Modeler::m_pQueryEngine->RemoveQuery( m_pValueQuery ); 
   }


bool Variable::Compile( ModelBase *pModel, ModelCollection *pCollection, MapLayer *pLayer )
   {
   ASSERT( m_pParser == NULL );
   //ASSERT( m_pQuery == NULL );
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
         ASSERT( m_pParser == NULL );
         m_pParser = new MParser;

         if ( CompileExpression( pModel, pCollection, m_pParser, m_expression, pLayer ) == false )
            {
            delete m_pParser;
            m_pParser = NULL;
            return false;
            }
         }
         break;

      case VT_PCT_AREA:
      case VT_PCT_CONTRIB_AREA:
         {
         //m_pQuery = Modeler::m_pQueryEngine->ParseQuery( m_expression );  // parsed when first read
         if ( m_pQuery == NULL )
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
         if ( ( m_pParser != NULL ) && ( ! m_pParser->evaluate( m_value ) ) )
            m_value = 0; 

         retVal = true;
         break;

      case VT_PCT_AREA:
      case VT_PCT_CONTRIB_AREA:
         // for pctArea variables, nothing is required
         ASSERT( m_pQuery != NULL );
         retVal =  true;
         break;
      }

   gpCurrentVar = NULL;
   return retVal;
   }


Expression::~Expression( void )
   {
   if ( m_pQuery != NULL )
      {
      Modeler::m_pQueryEngine->RemoveQuery( m_pQuery );  // deletes query
      m_pQuery = NULL;
      }

   if ( m_pParser != NULL )
      {
      delete m_pParser;
      m_pParser = NULL;
      }
   }


//-- ModelBase methods methods ---------------------------------------------------------

Expression *ModelBase::AddExpression( LPCTSTR expr, LPCTSTR query )
   {
   ASSERT ( expr != NULL );

   Expression *pExpr = new Expression;
   pExpr->m_expression = expr;
   pExpr->m_queryStr = query;

   pExpr->m_pParser = new MParser;

   //bool CompileExpression( this, ModelCollection *pCollection, MParser *pParser, LPCTSTR expr, MapLayer *pLayer )      
   //pExpr->m_pParser->compile( expr );

   if ( query != NULL )
      {
      pExpr->m_pQuery = Modeler::m_pQueryEngine->ParseQuery( query, 0, "Modeler" );

      if ( pExpr->m_pQuery == NULL )
         {
         CString msg( _T("Error parsing query for model expression " ) );
         msg += this->m_name;
         msg += _T(": Query=");
         msg += query;
         msg += _T( ". Query will be ignored" );
         AfxMessageBox( msg, MB_OK );
         }
      }

   m_exprArray.Add( pExpr );
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

   //if ( m_pRaster != NULL )
   //   {
   //   delete m_pRaster;
   //   m_pRaster = NULL;
   //   }
   }


bool ModelBase::LoadModelBaseXml( TiXmlNode *pXmlModelNode, MapLayer *pLayer )
   {
   bool loadSuccess = true;

   TiXmlNode *pXmlNode = NULL;
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

   if ( file == NULL )
      return false;

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( file );

   if ( ! ok )
      {      
      CString msg( _T("Error loading include file ") );
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
   LPTSTR value = NULL;

   VData vdata;

   CString _field;
   CString _value;

   while ( *p != NULL )
      {
      switch( *p )
         {
         case '=':
            *p = NULL;
            value = p+1;
            break;

         case ';':
            {
            *p = NULL;

            _field = field;
            _field.Trim();

            int col = Modeler::m_pMapLayer->GetFieldCol( _field );

            if ( col < 0 )
               {
               CString msg( _T("Unable to find field ") );
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

   int col = Modeler::m_pMapLayer->GetFieldCol( _field );

   if ( col < 0 )
      {
      CString msg( _T("Unable to find field ") );
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

   if ( name == NULL )
      name = attrColName;

   if ( attrColName == NULL )
      {
      CString msg( "Misformed <lookup> element [" );
      msg += name;
      msg += "]: A required attribute 'attr_col' is missing.";
      Report::ErrorMsg( msg );
      return false;
      }

   LookupTable *pTable = new LookupTable;
   pTable->m_name = name;
   pTable->m_colName = attrColName;
   pTable->m_colAttr  = pLayer->GetFieldCol( attrColName );
   
   if ( outputColName != NULL )
      {
      pTable->m_outputColName = outputColName;
      pTable->m_colOutput = pLayer->GetFieldCol( outputColName ); 

      if ( pTable->m_colOutput < 0 )
         {
         CString msg( "A field referenced by a lookup table is missing: " );
         msg += outputColName;
         msg += ".  This field will be added to the IDU coverage.";
         Report::WarningMsg( msg );
         pTable->m_colOutput = pLayer->m_pDbTable->AddField( outputColName, TYPE_FLOAT ); 
         }
      }

   lookupArray.Add( pTable );

   if ( pTable->m_colAttr < 0 )
      {
      CString msg( "A field referenced by a lookup table is missing: " );
      msg += attrColName;
      msg += " and should be added to the IDU coverage.";
      Report::WarningMsg( msg );
      }

   if ( defValue != NULL )
      pTable->m_defaultValue = atof( defValue );

   // have basics, get map elements for this lookup table
   TiXmlNode *pXmlMapNode = NULL;
   while( pXmlMapNode = pXmlLookupNode->IterateChildren( pXmlMapNode ) )
      {
      if ( pXmlMapNode->Type() != TiXmlNode::ELEMENT )
         continue;
         
      ASSERT( lstrcmp( pXmlMapNode->Value(), "map" ) == 0 );
      TiXmlElement *pXmlMapElement = pXmlMapNode->ToElement();

      LPCTSTR attr = pXmlMapElement->Attribute( "attr_val" );
      LPCTSTR val  = pXmlMapElement->Attribute( "value" );

      if ( attr == NULL || val == NULL )
         {
         Report::ErrorMsg( _T("Misformed <map> element: A required attribute is missing."));
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

   if ( name == NULL || file == NULL || use== NULL )
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
            CString msg( "Invalid Histogram use specified in [" );
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

   if ( name == NULL )
      {
      Report::ErrorMsg( _T("<const/input> element missing name attribute" ) );
      return false;
      }

   if ( value == NULL )
      {
      Report::ErrorMsg( _T("<const/input> element missing value attribute" ) );
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

   if ( name == NULL )
      {
      Report::ErrorMsg( _T("<var> element missing 'name' attribute" ) );
      return false;
      }

   // make sure name doesn't conflict with any field name
   int _col_ = Modeler::m_pMapLayer->GetFieldCol( name );
   if ( _col_ >= 0 )
      {
      CString msg;
      msg.Format( _T("Variable name conflict: The 'name' attribute specified  <var name='%s'...> matches a field name in the IDU coverage.  This is not allowed; variable names must be unique.  This variable will be ignored."), name );
      Report::ErrorMsg( msg );
      return false;
      }

   // get type
   VAR_TYPE vtype = VT_UNDEFINED;
   if ( type == NULL )
      vtype = VT_EXPRESSION;
   else if ( lstrcmpi( "pctArea", type ) == 0 )
      vtype = VT_PCT_AREA;
   else if ( lstrcmpi( "sum", type ) == 0 )
      vtype = VT_SUM_AREA;
   else if ( lstrcmpi( "areaWtMean", type ) == 0 )
      vtype = VT_AREAWTMEAN;
   else if ( lstrcmpi( "pctContribArea", type ) == 0 )
      vtype = VT_PCT_CONTRIB_AREA;
   else if ( lstrcmpi( "global", type ) == 0 )
      vtype = VT_GLOBALEXT;
   else if ( lstrcmpi( "raster", type ) == 0 )
      vtype = VT_RASTER;
   else if ( lstrcmpi( "density", type ) == 0 )
      vtype = VT_DENSITY;
   else
      {
      CString msg( "Invalid Variable type encountered reading <var name='" );
      msg += name;
      msg += ">: ";
      msg += type;
      Report::ErrorMsg( type );
      return false;
      }

   if ( ( vtype != VT_PCT_AREA && vtype != VT_PCT_CONTRIB_AREA && vtype != VT_RASTER ) && value == NULL )
      {
      CString msg;
      msg.Format( _T("<var> '%s' missing 'value' expression attribute - this is required" ), name );
      Report::ErrorMsg( msg );
      return false;
      }

   if ( vtype == VT_PCT_AREA || vtype == VT_PCT_CONTRIB_AREA)
      {
      if ( query == NULL )
         {
         CString msg;
         msg.Format( _T("<var> '%s' missing 'query' attribute - this is required for pctArea and pctContribArea-type variables" ), name );
         Report::ErrorMsg( msg );
         return false;
         }
      else if ( lstrlen( query ) == 0 )
         {
         CString msg;
         msg.Format( _T("<var> '%s's 'query' attribute is empty - this is required for pctArea and pctContribArea-type variables" ), name );
         Report::ErrorMsg( msg );
         return false;
         }
      }

   float cellSize = 0;
   if ( vtype == VT_RASTER )
      {
      LPCTSTR _cellSize = pXmlElement->Attribute( "cell_size" );
      if ( _cellSize == NULL )
         {
         CString msg;
         msg.Format( _T("<var> element '%s' is missing a 'cell-size' attribute - this is required for raster variables" ), name );
         Report::ErrorMsg( msg );
         return false;
         }
      else
         cellSize = (float) atof( _cellSize );

      if ( col == NULL )
         {
         CString msg;
         msg.Format( _T("<var> '%s' is missing a 'col' attribute is empty - this is required for raster variables" ), name );
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
   
   if ( useDelta != NULL )
      {
      if ( _extent != EXTENT_GLOBAL )
         {
         CString msg;
         msg.Format( "Invalid use of 'use_delta' attribute for variable [%s] - This attribute is incompatible with IDU-extent variables", name );
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

   if ( timing != NULL )
      {
      if ( _extent != EXTENT_GLOBAL )
         {
         CString msg;
         msg.Format( "Invalid use of 'timing' attribute for variable [%s] - This attribute is incompatible with IDU-extent variables", name );
         Report::ErrorMsg( msg );
         }
      else
         pVariable->m_timing = atoi( timing );
      }

   // store and process query if defined
   if ( query != NULL )
      {
      pVariable->m_query = query;
      ASSERT( Modeler::m_pQueryEngine != NULL );
      CString source( "Modeler Variable '" );
      source += pVariable->m_name;
      source += "'";

      pVariable->m_pQuery = Modeler::m_pQueryEngine->ParseQuery( query, 0, source );
      if ( pVariable->m_pQuery == NULL )
         {
         CString msg;
         msg.Format( _T("Invalid query encountered processing <var name='%s' query='%s'...>" ), name, query );
         Report::ErrorMsg( msg );
         delete pVariable;
         return false;
         }
      }

   if ( valueQuery != NULL )
      {
      pVariable->m_valueQuery = query;
      ASSERT( Modeler::m_pQueryEngine != NULL );
      CString source( "Modeler Variable '" );
      source += pVariable->m_name;
      source += "'";

      pVariable->m_pValueQuery = Modeler::m_pQueryEngine->ParseQuery( valueQuery, 0, source );
      if ( pVariable->m_pValueQuery == NULL )
         {
         CString msg;
         msg.Format( _T("Invalid value query encountered processing <var name='%s' value_query='%s'...>" ), name, valueQuery );
         Report::ErrorMsg( msg );
         delete pVariable;
         return false;
         }
      }

   if ( col != NULL )
      {
      int _col = Modeler::m_pMapLayer->GetFieldCol( col );

      if ( _col < 0 )
         {
         CString msg;
         msg.Format( _T("Unable to find field specified in 'col' attribute when processing var < name='%s' col='%s'...>.  Do you want to add it?" ), name, col );

         int result = AfxMessageBox( msg, MB_YESNO );

         if ( result == IDYES )
            _col = Modeler::m_pMapLayer->m_pDbTable->AddField( col, TYPE_FLOAT, true );
         }

      if ( _col >= 0 )
         {
         pVariable->m_col = _col;
         pVariable->m_dataType = Modeler::m_pMapLayer->GetFieldType( _col );
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
         msg.Format( _T("Invalid <report> element type:  reports must have global extent (type='sum|pctArea|pctContribArea|global') for report '%s'..." ), name );
         Report::ErrorMsg( msg );
         }

      pVariable->m_isOutput = true;
      pVariable->m_timing = 1;
      }

   //if ( vtype == VT_RASTER )
   //   pVariable->m_pRaster = new Raster( Modeler::m_pMapLayer, NULL, NULL, false );

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

   //if ( name == NULL )
   //   {
   //   ErrorMsg( _T("<eval> element missing 'name' attribute - this is required" ) );
   //   return false;
   //   }

   if ( value == NULL )
      {
      Report::ErrorMsg( _T("Modeler: <eval> element missing 'value' expression attribute - this is required" ) );
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
   //ASSERT( m_pParser != NULL );

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
         pExpr->m_pParser = NULL;
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
            if ( pVar->m_pQuery != NULL )
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
         if ( Modeler::m_usedParserVars.Lookup( col, index ) )
            {
            ASSERT( col == index );
            }
         else  // add it
            Modeler::m_usedParserVars.SetAt( col, col );

         pParser->redefineVar( varName, Modeler::m_parserVars+index );
         continue;
         }

      // ModelBase scope?
      if ( pModel != NULL )
         {
         // is it a histogram?
         HistogramArray *pHisto = FindHistogram( pModel->m_histogramsArray, varName );
         if ( pHisto != NULL )
            {
            pParser->redefineVar( varName, &pHisto->m_value );
            continue;
            }

         // is it a lookup table
         LookupTable *pLookup = FindLookup( pModel->m_lookupTableArray, varName );
         if ( pLookup != NULL )
            {
            pParser->redefineVar( varName, &pLookup->m_value );
            continue;
            }

         // is it a constant?
         Constant *pConst = FindConstant( pModel->m_constantArray, varName );
         if ( pConst != NULL )
            {
            pParser->redefineVar( varName, &pConst->m_value );
            continue;
            }

         // is it a variable?
         Variable *pVar = FindVariable( pModel->m_variableArray, varName );
         if ( pVar != NULL )
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
         if ( pHisto != NULL )
            {
            pParser->redefineVar( varName, &pHisto->m_value );
            continue;
            }

         // is it a lookup table
         LookupTable *pLookup = FindLookup( pCollection->m_lookupTableArray, varName );
         if ( pLookup != NULL )
            {
            pParser->redefineVar( varName, &pLookup->m_value );
            continue;
            }

         // is it a constant?
         Constant *pConst = FindConstant( pCollection->m_constantArray, varName );
         if ( pConst != NULL )
            {
            pParser->redefineVar( varName, &pConst->m_value );
            continue;
            }

         // is it a variable?
         Variable *pVar = FindVariable( pCollection->m_variableArray, varName );
         if ( pVar != NULL )
            {
            pParser->redefineVar( varName, &pVar->m_value );

            // compile variable expression parser as well
            //pVar->Compile( pModel, pCollection, pLayer );
            continue;
            }
         }

      // if we get this far, the variable is undefiend
      CString msg( "Unable to compile Modeler expression for Model: " );
      if ( pModel != NULL )
         {
         msg += pModel->m_name;
         msg +=" [";
         msg += expr;
         msg += "]. ";
         }
      msg += "Expression: ";
      msg += expr;
      msg += " This expression will be ignored";
      AfxMessageBox( msg );
      return false;
      }  // end of:  for ( i < varCount )

   return true;
   }



BOOL NetworkModel::InitNetwork( LPCTSTR name, int colTo, int colFrom, int colEdge, int colTreeID )
   {
   ASSERT( this->m_pNetworkLayer != NULL );
   ASSERT( this->m_pIDULayer != NULL );
   ASSERT( colTo >= 0 || colFrom  >= 0 );

   int retVal = m_network.BuildTree( m_pNetworkLayer, colEdge, colTo );
   ASSERT( retVal > 0 );

   if ( this->m_colOrder >= 0 )
      m_network.PopulateOrder();

   if ( colTreeID >= 0 )
      m_network.PopulateTreeID( colTreeID );

   BuildIndex();
   //ComputeContributingAreas( 0, 100000 );
 
   return TRUE;
   }



void NetworkModel::BuildIndex( void )
   {
   m_iduIndexArray.RemoveAll();

   m_iduIndexArray.SetSize( m_pNetworkLayer->GetRowCount() );

   int iduCount = m_pIDULayer->GetRowCount();
   //int reachIndex;

   int colReachIndex = this->m_colReachIndex;
   ASSERT( colReachIndex >= 0 );
/*
   int bufferIDUs = 0;
   int pctBufferArea = 0;
   for ( int i=0; i < iduCount; i++ )
      {
      m_pIDULayer->GetData( i, m_colAreaBuffer, pctBufferArea );
      if ( pctBufferArea > 0 ) 
         {
         m_pIDULayer->GetData( i, colReachIndex, reachIndex );
         m_iduIndexArray[ reachIndex ].Add( i );
         bufferIDUs++;
         }
      }
*/
   
   // compute statistics
   int zeroIduReaches = 0;
   float avgIdusPerReach = 0;
   for ( int i=0; i < m_iduIndexArray.GetSize(); i++ )
      {
      if ( m_iduIndexArray[ i ].GetSize() == 0 )
         zeroIduReaches++;

      avgIdusPerReach += m_iduIndexArray[ i ].GetSize();
      }

   avgIdusPerReach /= m_iduIndexArray.GetSize();
   //float _avgIdusPerReach = float( bufferIDUs ) / m_iduIndexArray.GetSize();

   TRACE1( "IDUs/Reach indexed: %f", avgIdusPerReach );
   }


/*

float NetworkModel::ComputeContributingArea( ModelBase *pBase, int year, int endYear )
   {
   ASSERT( pBase->m_pParser != NULL );

   m_edgesReportingScores = 0;

   float totalArea = 0;

   for ( int i=0; i < m_network.GetRootCount(); i++ )
      {
      float score = ComputeNetworkScore( m_network.GetRoot( i ), area, year, endYear );
      totalArea += area;
      }

   return totalArea; //????????????
   }

*/
float NetworkModel::ComputeNetworkScore( ModelBase *pBase, NetworkNode *pNode, float&area, int year, int endYear )
   {
   /*
   float leftScore  = 0;
   float rightScore = 0;
   
   float leftArea = 0;
   float rightArea = 0;

   // get upstream reach scores
   if ( pNode->m_pLeft )
      leftScore = ComputeNetworkScore( pNode->m_pLeft, leftArea, year, endYear );
      
   if ( pNode->m_pRight )
      rightScore = ComputeNetworkScore( pNode->m_pRight, rightArea, year, endYear );

   // iterate through each IDU assocatiated with this reach, getting scores for thoes IDUS that sataisfy the query   
   int reachIndex = pNode->GetDownstreamVectorIndex();
   if ( reachIndex >= 0 )
      {
      CUIntArray &iduArray = this->m_iduIndexArray[ reachIndex ];

      float thisArea = 0;
      float iduScore = 0;
      float totalIduScore = 0;

      for ( int i=0; i < iduArray.GetSize(); i++ )
         {
         int idu = iduArray[ i ];

         if ( m_pQuery != NULL )
            {
            bool result;
            Modeler::m_pQueryEngine->Run( m_pQuery, idu, result );

            if ( result == false )
               continue;
            }

         pBase->UpdateVariables( idu );
         iduScore = (float) pBase->m_pParser->evaluate();

         switch( pBase->m_m

         totalIduScore += iduScore;
         }

      // we have the area for this reach, add it to the rest of the reaches..
            }

         if ( computeFishIBI )
            fishIBI = ComputeFishIBI( areaAg, areaDev, areaBuffer );

         m_reachesReportingScores++;
         }

      m_pStreamLayer->SetData( reachIndex, m_colFishIBI, fishIBI );

      }
*/
   return 0; //fishIBI;
   }





BOOL EvalModel::Run( EnvContext *pEnvContext, bool useAddDelta )
   {
   m_rawScore = 0;

   m_pCollection->UpdateGlobalExtentVariables( pEnvContext, 0, useAddDelta ); // this evaluates all global-extent variables, including model-scope variables
   
   if ( this->m_useDelta )
      RunDelta( pEnvContext );      // sets rawscore
   else
      RunMap( pEnvContext, useAddDelta ); // sets rawscore

   m_pCollection->UpdateGlobalExtentVariables( pEnvContext, 1, useAddDelta ); // this evaluates all global-extent variables, including model-scope variables

   // scale to (-3, +3)
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


bool EvalModel::RunMap( EnvContext *pEnvContext, bool useAddDelta )
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

         if ( pExpr->m_pQuery == NULL )
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

               ASSERT( pExpr->m_pParser != NULL );
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


bool EvalModel::RunDelta( EnvContext *pEnvContext )
   {
   ASSERT( m_useDelta == true );

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
            if ( pExpr->m_pQuery != NULL )
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
         ASSERT( pExpr->m_pParser != NULL );
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


   
BOOL NetworkEvalModel::Run( EnvContext *pEnvContext, bool useAddDelta )
   {
   /*
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   int colArea = pLayer->GetFieldCol( _T("Area") );
   if ( colArea < 0 )
      return FALSE;

   int   colCount = pLayer->GetColCount();
   float totalArea = pLayer->GetTotalArea();
   VData value;
   float fValue;
   int   iduCount = 0;

   m_rawScore = 0;
      
   // iterate over ACTIVE records
   for ( MapLayer::Iterator i = pLayer->Begin(); i != pLayer->End(); i++ )
      {
      currentIDU = i;
      iduCount++;

      float area = 0;
      pLayer->GetData( i, colArea, area );

      // load current values into the m_parserVars array 
      // (NOTE: THIS SHOULD ONLY HAPPEN DURING THE FIRST INVOCATION OF A MODEL IN A GIVEN TIMESTEP...
      for ( int j=0; j < colCount; j++ )
         {
         bool okay = pLayer->GetData( i, j, value );

         if ( okay && value.GetAsFloat( fValue ) )
            Modeler::m_parserVars[ j ] = fValue;
         }

      // evaluate lookups
      for ( int j=0; j < this->m_lookupTableArray.GetSize(); j++ )
         {
         LookupTable *pTable = this->m_lookupTableArray[ j ];
         bool ok = pTable->Evaluate();
         ASSERT( ok );
         }

      // evaluate histograms
      for ( int j=0; j < this->m_histogramsArray.GetSize(); j++ )
         {
         HistogramArray *pHisto = this->m_histogramsArray[ j ];
         bool ok = pHisto->Evaluate();
         ASSERT( ok );
         }

      // evaluate variables
      for ( int j=0; j < this->m_variableArray.GetSize(); j++ )
         {
         Variable *pVar = this->m_variableArray[ j ];
         bool ok = pVar->Evaluate();
         ASSERT( ok );
         }

      // note: no need to evaluate constants - once is enough

      // done evalating all inputs - run model formula
      ASSERT( m_pParser != NULL );
      float iduValue = (float) m_pParser->evaluate();

      switch( m_evalMethod )
         {
         case EM_SUM:    // sum the scores over the entire coverage
         case EM_MEAN:    // compute the average score over the entire coverage
            m_rawScore += iduValue;
            break;

         case EM_AREAWTMEAN:  // compute an area-weighted average value over the entire coverage
            m_rawScore += iduValue * area;
            break;
         }

      if ( m_col >= 0 )   // populate the IDU database?
         {
         TYPE type = pLayer->GetFieldType( m_col );

         VData v;
         v.type = type;

         if ( m_evalMethod == EM_AREAWTMEAN ) // make any adjustments before recording to IDU database
             iduValue /= area;

         v.SetKeepType( iduValue );
         AddDelta( pEnvContext, i, m_col, v );
         }
      }  // end of: MapIterator

   // normalize the scores, depending on the evalMethod

   // normalize scores to total area and apply scaling
   switch( m_evalMethod )
      {
      case EM_SUM:    // sum the scores over the entire coverage
         break;         // no additional processing needed

      case EM_MEAN:    // compute the average score over the entire coverage
         m_rawScore /= iduCount;
         break;

      case EM_AREAWTMEAN:  // compute an area-weighted average value over the entire coverage
         m_rawScore /= totalArea;
         break;
      }

   // scale to (-3, +3)
   double range = m_upperBound - m_lowerBound;
   m_score = ((( m_rawScore - m_lowerBound ) / range) * 6) - 3;
         
   if ( m_evalType == ET_VALUE_LOW )
   m_score *= -1;

   // make sure score is in range
   if ( m_score < -3.0f ) m_score = -3.0f;
   else if ( m_score > 3.0f ) m_score = 3.0f; */
   
   return TRUE;
   }

   
BOOL AutoProcess::Run( EnvContext *pEnvContext, bool useAddDelta )
   {
   //if ( m_col < 0 )
   //   return true;

  m_pCollection->UpdateGlobalExtentVariables( pEnvContext, 0, useAddDelta ); // this loads all parserVars and evaluates ModelCollection extent variables for the current IDU

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

            if ( pExpr->m_pQuery == NULL )
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
                  CString msg(_T("WARNING: Multiple updates to Modeler column:  AutoProcess: ") );
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

                  ASSERT( pExpr->m_pParser != NULL );
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


BOOL AutoProcess::RunDelta( EnvContext *pEnvContext )
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
            ASSERT( pExpr->m_pParser != NULL );
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






BOOL NetworkAutoProcess::Run( EnvContext *pEnvContext, bool useAddDelta )
   {
   if ( m_col < 0 )
      return true;

   if ( m_pNetworkLayer == NULL )
      return FALSE;

   if ( this->m_useDelta )
      return RunDelta( pEnvContext );

   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   //int colArea = pLayer->GetFieldCol( _T("Area") );
   //if ( colArea < 0 )
   //   return FALSE;

   int   iduCount = 0;
   // iterate over ACTIVE records in the network layer

   /*











   for ( MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++ )
      {
      m_pCollection->UpdateVariables( idu ); // this loads all parserVars and evaluates ModelCollection extent variables for the current IDU

      if ( pExpr->m_pQuery != NULL ) // does this model apply here?
         {
         bool result;
         Modeler::m_pQueryEngine->Run( m_pQuery, idu, result );

         if ( result == false )
            continue;
         }

      iduCount++;

      UpdateVariables( idu );

      // done evalating all inputs - run model formula
      ASSERT( m_pParser != NULL );
      float iduValue = (float) m_pParser->evaluate();

      if ( useAddDelta )
         {
         float oldValue;
         pLayer->GetData( idu, m_col, oldValue );

         if ( oldValue != iduValue )
            AddDelta( pEnvContext, idu, m_col, iduValue ); // populate the IDU database
         }
      else
         pLayer->SetData( idu, m_col, iduValue );

      }  // end of: MapIterator
      */
   return TRUE;
   }


BOOL NetworkAutoProcess::RunDelta( EnvContext *pEnvContext )
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

   ASSERT( m_exprArray.GetSize() == 1 );
   Expression *pExpr = m_exprArray[ 0 ];
   
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
            ASSERT( pExpr->m_pParser != NULL );
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


EvalModel *ModelCollection::GetEvalModelFromID( int id )
   {
   for ( INT_PTR i=0; i < m_evalModelArray.GetSize(); i++ )
      {
      if ( m_evalModelArray[ i ]->m_id == id )
         return m_evalModelArray[ i ];
      }

   return NULL;
   }


AutoProcess *ModelCollection::GetAutoProcessFromID( int id )
   {
   for ( INT_PTR i=0; i < m_autoProcessArray.GetSize(); i++ )
      {
      if ( m_autoProcessArray[ i ]->m_id == id )
         return m_autoProcessArray[ i ];
      }

   return NULL;
   }



bool ModelCollection::UpdateVariables( EnvContext *pEnvContext, int idu, bool useAddDelta )
   {
   // updates  variables for this model collection.  if 'idu' < 0, updates global extent variables, otherwise updates IDU extent variables
   ASSERT ( idu >= 0 );

   VData value;
   float fValue;
   int colCount = Modeler::m_pMapLayer->GetFieldCount();

   // (NOTE: THIS SHOULD ONLY HAPPEN DURING THE FIRST INVOCATION OF A MODEL IN A GIVEN TIMESTEP...
   for ( int j=0; j < colCount; j++ )
      {
      bool okay = Modeler::m_pMapLayer->GetData( idu, j, value );

      if ( okay && value.GetAsFloat( fValue ) )
         Modeler::m_parserVars[ j ] = fValue;
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
            if ( pVar->m_pQuery != NULL )
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
                  MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
                  pLayer->SetData( idu, pVar->m_col, pVar->m_value );
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
   if ( deltaCount > 0 && pEnvContext->pDeltaArray != NULL )
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
                  if ( pVar->m_pQuery != NULL )
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
            Modeler::m_pMapLayer->GetData( delta.cell, Modeler::m_colArea, area );

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
                     case VT_PCT_CONTRIB_AREA:
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
      for ( MapLayer::Iterator i = Modeler::m_pMapLayer->Begin(); i != Modeler::m_pMapLayer->End(); i++ )
         {
         bool evalIDU = false;

         for ( int j=0; j < globalCount; j++ )
            {
            Variable *pVar = va[ j ];
            pVar->m_evaluate = false;

            if ( pVar->m_useDelta )
               continue;
         
            if ( pVar->m_pQuery == NULL || pVar->m_query.GetLength() == 0 )
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
            Modeler::m_pMapLayer->GetData( i, Modeler::m_colArea, area );

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
                     case VT_PCT_CONTRIB_AREA:
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
         case VT_PCT_CONTRIB_AREA:
            {
            float totalArea = Modeler::m_pMapLayer->GetTotalArea();
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
         ASSERT( pVar->m_pParser != NULL );
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

   for ( INT_PTR i=0; i < this->m_evalModelArray.GetSize(); i++ )
      {
      EvalModel *pModel = m_evalModelArray[ i ];

      for ( INT_PTR j=0; j < pModel->m_variableArray.GetSize(); j++ )
         {
         Variable *pVar = pModel->m_variableArray[ j ];

         if ( pVar->m_extent == EXTENT_GLOBAL && pVar->m_timing == timing )
            va.Add( pVar );
         }
      }

   for ( INT_PTR i=0; i < this->m_autoProcessArray.GetSize(); i++ )
      {
      AutoProcess *pModel = m_autoProcessArray[ i ];

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
   
   m_autoProcessArray.RemoveAll();
   m_evalModelArray.RemoveAll();
   m_histogramsArray.RemoveAll();
   m_lookupTableArray.RemoveAll();
   m_constantArray.RemoveAll();
   m_variableArray.RemoveAll();
   }


// LoadXml() is called during Init()
bool ModelCollection::LoadXml( LPCTSTR _filename, MapLayer *pLayer, bool isImporting )
   {
   CString filename;
   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "Modeler: Input file '%s' not found - this process will be disabled", _filename );
      Report::ErrorMsg( msg );
      return false;
      }

   bool loadSuccess = true;

   if ( ! isImporting )
      {
      m_fileName = filename;
      Modeler::m_pMapLayer = pLayer;
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

   TiXmlNode *pXmlNode = NULL;
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
            pVar->Compile( NULL, this, pLayer );
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
         if ( file == NULL )
            {
            Report::ErrorMsg( _T("No 'file' attribute specified for <include> element") );
            continue;
            }
         if ( ! LoadXml( file, pLayer, true ) )
            loadSuccess = false;
         }

      // is this a model
      else if ( lstrcmpi( "eval_model", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadEvalModel( pXmlNode, pLayer ) )
            loadSuccess = false;
		    else
			   {
            EvalModel *pModel = m_evalModelArray.GetAt( m_evalModelArray.GetCount()-1 ); 
            ASSERT( pModel != NULL );
            pModel->SetVars( (EnvExtension*) pModel );
            ////pModel->Init( pEnvContext, initStr );
            }
         }

      // is this a network model
      else if ( lstrcmpi( "network_model", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadNetworkEvalModel( pXmlNode, pLayer ) )
            loadSuccess = false;
         }

      // is this a process
      else if ( lstrcmpi( "auto_process", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadAutoProcess( pXmlNode, pLayer ) )
            loadSuccess = false;
         else
            {
            AutoProcess *pModel = m_autoProcessArray.GetAt( m_autoProcessArray.GetCount()-1 ); 
            ASSERT( pModel != NULL );
            pModel->SetVars( (EnvExtension*) pModel );
            }
         }

      // is this a process
      else if ( lstrcmpi( "network_process", pXmlNode->Value() ) == 0 )
         {
         if ( ! LoadNetworkAutoProcess( pXmlNode, pLayer ) )
            loadSuccess = false;
         }
      }

   // all done loading xml, compile any ModelCollection-scope variables
   //for ( int i=0; i < (int) m_variableArray.GetSize(); i++ )
   //   {
   //   Variable *pVar = m_variableArray.GetAt( i );
   //   pVar->Compile( NULL, this, pLayer );
   //   }

   return loadSuccess;
   }


bool ModelCollection::LoadEvalModel( TiXmlNode *pXmlModelNode, MapLayer *pLayer  )
   {
   bool loadSuccess = true;
   if ( pXmlModelNode->Type() != TiXmlNode::ELEMENT )
      return false;
      
   // process Model element
   //
   // <eval_model name="Impervious Surfaces (%)" col="EM_IMPERV" value="FRACIMPERV" 
   //      evalType="LOW" lowerBound="0.10" upperBound="0.15" method="AREAWTAVG"/>
   //
   TiXmlElement *pXmlElement = pXmlModelNode->ToElement();
   LPCTSTR name   = pXmlElement->Attribute( "name" );
   LPCTSTR id     = pXmlElement->Attribute( "id" );
   LPCTSTR col    = pXmlElement->Attribute( "col" );
   LPCTSTR value  = pXmlElement->Attribute( "value" );  // optional, but if missing, requires child <eval> tags
   LPCTSTR query  = pXmlElement->Attribute( "query" );
   LPCTSTR evalType = pXmlElement->Attribute( "eval_type" );
   LPCTSTR lbound = pXmlElement->Attribute( "lower_bound" );
   LPCTSTR ubound = pXmlElement->Attribute( "upper_bound" );
   LPCTSTR method = pXmlElement->Attribute( "eval_method" );
   LPCTSTR constraints = pXmlElement->Attribute( "constraints" );
   LPCTSTR useDelta = pXmlElement->Attribute( "use_delta" );
   LPCTSTR type   =  pXmlElement->Attribute( "type" );

   if ( name == NULL  )
      {
      CString msg;
      msg.Format( _T("<eval_model> is missing its 'name' attribute" ) );
      Report::ErrorMsg( msg );
      return false;
      }

   // note:"col" is optional
   if ( lbound == NULL || ubound == NULL || method == NULL || id == NULL || evalType == NULL || method == NULL )
      {
      CString msg;
      msg.Format( _T("<eval_model> '%s' element missing required attribute" ), name );
      Report::ErrorMsg( msg );
      return false;
      }
      
   EvalModel *pModel = new EvalModel( this );

   pModel->m_name = name;
   pModel->m_id   = atoi( id );

   // check to see that ID not already used
   EvalModel *_pModel = GetEvalModelFromID( pModel->m_id );
   if ( _pModel != NULL )
      {
      CString msg;
      msg.Format( _T("Duplicate eval_model ID found (%s and %s)" ), name, (LPCTSTR) _pModel->m_name );
      Report::ErrorMsg( msg );
      }

   // populate model
   pModel->m_fieldName = col;
   pModel->m_lowerBound = (float) atof( lbound );
   pModel->m_upperBound = (float) atof( ubound );

   EVAL_METHOD _evalMethod = EM_AREAWTMEAN;
   if ( method[ 0 ] == 'S' || method[ 0 ] == 's' )
      _evalMethod = EM_SUM;
   else if ( method[ 0 ] == 'A' || method[ 0 ] == 'a' )
      {
      if ( method[ 1 ] == 'V' || method[ 1 ] == 'v' )
      _evalMethod = EM_MEAN;
      }
   else if ( method[ 0 ] == 'p' || method[ 0 ] == 'P' )
      _evalMethod = EM_PCT_AREA;
   else if ( method[ 0 ] == 'd' || method[ 0 ] == 'D' )
      _evalMethod = EM_DENSITY;
      
   pModel->m_evalMethod = _evalMethod;

   EVAL_TYPE _evalType;
   switch( evalType[ 0 ] )
      {
      case 'H':   _evalType = ET_VALUE_HIGH;    break;
      case 'L':   _evalType = ET_VALUE_LOW;     break;
      //case 'I':   _evalType = ET_TREND_INCREASING;    break;
      //case 'D':   _evalType = ET_TREND_DECREASING;    break;
      default:
         {
         CString msg( "Invalid 'evalType' attribute specified for model " );
         msg += name;
         Report::ErrorMsg( msg );
         delete pModel;
         return false;
         }
      }
   pModel->m_evalType = _evalType;

   if ( useDelta != NULL )
      {
      pModel->m_useDelta = true;
      pModel->m_deltaStr = useDelta;

      //pModel->ParseUseDelta( useDelta );
      ParseUseDelta( useDelta, pModel->m_useDeltas );
      }
 
   if ( constraints )
      {
      if ( constraints[ 0 ] == 'I' )  // INCREASE
         pModel->m_constraints = C_INCREASE_ONLY;
      else if ( constraints[ 0 ] == 'D' )
         pModel->m_constraints = C_DECREASE_ONLY;
      }

   if ( type != NULL )
      {
      if ( lstrcmpi( type, "raster" ) == 0 )
         {
         LPCTSTR _cellSize = pXmlElement->Attribute( "cell_size" );
         if ( _cellSize == NULL )
            {
            CString msg;
            msg.Format( "'cell_size' attribute missing from raster model '%s' - this is a required element", name );
            Report::ErrorMsg( msg );
            delete pModel;
            return false;
            }

         float cellSize = (float) atof( _cellSize );

         //pModel->m_pRaster = new Raster( pLayer, NULL, NULL, false ); 
         }
      }

   // make a parser for this formula and add to the parser array
   //ASSERT( pModel->m_pParser == NULL );

   //MParser *pParser = new MParser; 
   //ASSERT( pParser != NULL );
   if ( _evalMethod == EM_PCT_AREA && value == NULL )
      value = "1";

   if ( value != NULL )
      pModel->AddExpression( value, query );

   // make sure the col exists if specified
   int _col = -1;
   if ( col != NULL )
      {
      _col = pLayer->GetFieldCol( col );

      if ( _col < 0 )   // missing?
         {
         CString msg( "Missing field column specified by a model in Modeler input file (" );
         msg += col;
         msg += ").  This column is being added to the database.  You should save the database using File->Save->Cell Database";
         AfxMessageBox( msg, MB_OK );
         
         int width, decimals;
         GetTypeParams( TYPE_DOUBLE, width, decimals );
         pLayer->m_pDbTable->AddField( col, TYPE_DOUBLE, width, decimals, true );
         _col = pLayer->GetColCount() - 1;
         }
      }
   pModel->m_col = _col;

   pModel->LoadModelBaseXml( pXmlModelNode, pLayer );

   // Compile model    note: Parser set up later
   pModel->Compile( pLayer );

   // store this parser
   m_evalModelArray.Add( pModel ); 
   
   return loadSuccess;
   }


bool ModelCollection::LoadNetworkEvalModel( TiXmlNode *pXmlModelNode, MapLayer *pLayer  )
   {
   bool loadSuccess = true;
   if ( pXmlModelNode->Type() != TiXmlNode::ELEMENT )
      return false;
      
   // process Model element
   //
   // <network_model name="Impervious Surfaces (%)" col="EM_IMPERV" value="FRACIMPERV" 
   //      evalType="LOW" lowerBound="0.10" upperBound="0.15" method="AREAWTAVG"/>
   //
   TiXmlElement *pXmlElement = pXmlModelNode->ToElement();
   LPCTSTR name       = pXmlElement->Attribute( "name" );
   LPCTSTR netLayer   = pXmlElement->Attribute( "network_layer" );
   LPCTSTR netMethod  = pXmlElement->Attribute( "network_method" );
   LPCTSTR cAreaMethod= pXmlElement->Attribute( "ca_method" );
   LPCTSTR netCol     = pXmlElement->Attribute( "network_col" );
   LPCTSTR indexCol   = pXmlElement->Attribute( "index_col" );
   LPCTSTR orderCol   = pXmlElement->Attribute( "order_col" );
   LPCTSTR toCol      = pXmlElement->Attribute( "to_col" );
   LPCTSTR fromCol    = pXmlElement->Attribute( "from_col" );
   LPCTSTR edgeCol    = pXmlElement->Attribute( "edge_col" );
   LPCTSTR treeIDCol  = pXmlElement->Attribute( "treeid_col" );
   LPCTSTR id         = pXmlElement->Attribute( "id" );
   LPCTSTR col        = pXmlElement->Attribute( "col" );
   LPCTSTR value      = pXmlElement->Attribute( "value" );
   LPCTSTR type       = pXmlElement->Attribute( "eval_type" );
   LPCTSTR lbound     = pXmlElement->Attribute( "lower_bound" );
   LPCTSTR ubound     = pXmlElement->Attribute( "upper_bound" );
   LPCTSTR method     = pXmlElement->Attribute( "eval_method" );
   LPCTSTR useDelta   = pXmlElement->Attribute( "use_delta" );  // ignored for now
   LPCTSTR query      = pXmlElement->Attribute( "query" );

   // note: "col", "network_col" is optional
   if ( value == NULL || type == NULL || lbound == NULL || ubound == NULL || method == NULL || netMethod == NULL || netLayer == NULL && indexCol == NULL )
      {
      Report::ErrorMsg( _T("<network_model> element missing required attribute" ) );
      return false;
      }

   // make sure the netLayer exists
   Map *pMap = pLayer->GetMapPtr();
   ASSERT( pMap != NULL );
   MapLayer *pNetLayer = pMap->GetLayer( netLayer );
   if ( pNetLayer == NULL )
      {
      CString msg;
      msg.Format( _T("Network Layer [%s] not found - this model will be ignored" ), netLayer );
      Report::ErrorMsg( msg );
      return false;
      }

   NetworkEvalModel *pModel = new NetworkEvalModel( this );
   pModel->m_name = name;
   pModel->m_id   = atoi( id );

   pModel->m_pIDULayer = pLayer;

   // methods
   if ( netMethod != NULL )
      {
      if ( _strnicmp( netMethod, "cmb", 3 ) == 0 )
         pModel->m_netMethod = NM_CUMULATIVE_MASS_BALANCE;
      else if ( _strnicmp( netMethod, "cs", 2 ) == 0 )
         pModel->m_netMethod = NM_CUMULATIVE_SCORE;
      else if ( _strnicmp( netMethod, "ca", 2 ) == 0 )
         pModel->m_netMethod = NM_CAREA_SCORE;
      }

   if ( cAreaMethod != NULL )
      {
      if ( _strnicmp( cAreaMethod, "none", 4 ) == 0 )
         pModel->m_contributingAreaMethod = NCAM_NONE;
      else if ( _strnicmp( cAreaMethod, "sum", 3 ) == 0 )
         pModel->m_contributingAreaMethod = NCAM_SUM;
      else if ( _strnicmp( cAreaMethod, "mean", 4 ) == 0 )
         pModel->m_contributingAreaMethod = NCAM_MEAN;
      else if ( _strnicmp( cAreaMethod, "area", 4 ) == 0 )
         pModel->m_contributingAreaMethod = NCAM_AREAWTMEAN;
      }

   pModel->m_networkLayerName = netLayer;
   pModel->m_pNetworkLayer = pNetLayer;

   // check to see that ID not already used
   EvalModel *_pModel = GetEvalModelFromID( pModel->m_id );
   if ( _pModel != NULL && _pModel->m_name != pModel->m_name )
      {
      CString msg;
      msg.Format( _T("Duplicate eval_model ID found (%s and %s)" ), name, (LPCTSTR) _pModel->m_name );
      Report::ErrorMsg( msg );
      }

   pModel->m_fieldName = col;
   pModel->m_lowerBound = (float) atof( lbound );
   pModel->m_upperBound = (float) atof( ubound );

   EVAL_METHOD _evalMethod = EM_AREAWTMEAN;
   if ( method[ 0 ] == 'S' )
      _evalMethod = EM_SUM;
   else if ( method[ 0 ] == 'A' )
      if ( method[ 1 ] == 'V' )
   _evalMethod = EM_MEAN;
      
   pModel->m_evalMethod = _evalMethod;

   EVAL_TYPE _evalType;
   switch( type[ 0 ] )
      {
      case 'H':   _evalType = ET_VALUE_HIGH;    break;
      case 'L':   _evalType = ET_VALUE_LOW;     break;
      //case 'I':   _evalType = ET_TREND_INCREASING;    break;
      //case 'D':   _evalType = ET_TREND_DECREASING;    break;
      default:
         {
         CString msg( "Invalid 'evalType' attribute specified for model " );
         msg += name;
         Report::ErrorMsg( msg );
         delete pModel;
         return false;
         }
      }
   pModel->m_evalType = _evalType;

   // make a parser for this model and add to the parser array
   MParser *pParser = new MParser;
   ASSERT( pParser != NULL );

   if ( value != NULL )
      pModel->AddExpression( value, query );

   // make sure the col exists if specified
   int _col = -1;
   if ( col != NULL )
      {
      _col = pLayer->GetFieldCol( col );

      if ( _col < 0 )   // missing?
         {
         CString msg( "Missing field column specified by a model in Modeler input file (" );
         msg += col;
         msg += ").  This column is being added to the database.  You should save the database using File->Save->Cell Database";
         AfxMessageBox( msg, MB_OK );
         int width, decimals;
         GetTypeParams( TYPE_DOUBLE, width, decimals );
         pLayer->m_pDbTable->AddField( col, TYPE_DOUBLE, width, decimals, true );
         _col = pLayer->GetColCount() - 1;
         }
      }
   pModel->m_col = _col;
   
   // make sure the network_col exists if specified
   int _netCol = -1;
   if ( netCol != NULL )
      {
      _netCol = pNetLayer->GetFieldCol( netCol );

      if ( _netCol < 0 )   // missing?
         {
         CString msg( "Missing network layer column specified by a model in Modeler input file (" );
         msg += netCol;
         msg += ").  This column is being added to the network database.  You should save the database";
         AfxMessageBox( msg, MB_OK );
         int width, decimals;
         GetTypeParams( TYPE_DOUBLE, width, decimals );
         pNetLayer->m_pDbTable->AddField( netCol, TYPE_DOUBLE, width, decimals, true );
         _netCol = pNetLayer->GetColCount() - 1;
         }
      }

   pModel->m_colNetwork = _netCol;

   // make sure the order col exists if specified
   int _orderCol = -1;
   if ( orderCol != NULL )
      {
      _orderCol = pNetLayer->GetFieldCol( orderCol );

      if ( _orderCol < 0 )   // missing?
         {
         CString msg( "Missing 'Order' column specified by a model in Modeler input file (" );
         msg += orderCol;
         msg += ").  This column is being added to the network database.  You should save the database";
         AfxMessageBox( msg, MB_OK );
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         pNetLayer->m_pDbTable->AddField( orderCol, TYPE_INT, true );
         _orderCol = pNetLayer->GetColCount() - 1;
         }
      }

   pModel->m_colOrder = _orderCol;

   int _indexCol = pLayer->GetFieldCol( indexCol );
   if ( _indexCol < 0 )   // missing?
      {
      CString msg( "Missing Reach Index column specified by a model in Modeler input file (" );
      msg += indexCol;
      msg += ").  This column is being added to the IDU database.  You should save the database";
      AfxMessageBox( msg, MB_OK );
      int width, decimals;
      GetTypeParams( TYPE_INT, width, decimals );
      pLayer->m_pDbTable->AddField( indexCol, TYPE_INT, true );
      _indexCol = pLayer->GetColCount() - 1;
      }

   pModel->m_colReachIndex = _indexCol;

   pModel->LoadModelBaseXml( pXmlModelNode, pLayer );

   // store this model
   m_evalModelArray.Add( pModel ); 

   // set up network tree
   int _toCol = -1;
   if ( toCol )
      {
      _toCol = pNetLayer->GetFieldCol( toCol );

      if ( _toCol < 0 )
         {
         CString msg;
         msg.Format( _T("Network coverage is missing specified 'to_col' field (%s) - unable to build network tree" ), toCol );
         AfxMessageBox( msg, MB_OK );
         }
      }

   int _fromCol = -1;
   if ( fromCol )
      {
      _fromCol = pNetLayer->GetFieldCol( fromCol );

      if ( _fromCol < 0 )
         {
         CString msg;
         msg.Format( _T("Network coverage is missing specified 'from_col' field (%s) - unable to build network tree" ), fromCol );
         AfxMessageBox( msg, MB_OK );
         }
      }

   int _edgeCol = -1;
   if ( edgeCol )
      {
      _edgeCol = pNetLayer->GetFieldCol( edgeCol );

      if ( _edgeCol < 0 )
         {
         CString msg;
         msg.Format( _T("Network coverage is missing specified 'edge_col' field (%s) - unable to build network tree" ), edgeCol );
         AfxMessageBox( msg, MB_OK );
         }
      }

   int _treeIDCol = pNetLayer->GetFieldCol( treeIDCol );
   if ( _treeIDCol < 0 && treeIDCol != NULL )
      {
      CString msg;
      msg.Format( _T("Network coverage is missing specified 'treeid_col' field (%s) - this will be added to the coverage" ), treeIDCol );
      AfxMessageBox( msg, MB_OK );
      int width, decimals;
      GetTypeParams( TYPE_INT, width, decimals );
      _treeIDCol = pNetLayer->m_pDbTable->AddField( treeIDCol, TYPE_INT, true );         
      }


   if ( ( _toCol >= 0 || _fromCol >= 0 ) && ( edgeCol == NULL || _edgeCol >= 0 ) )
      pModel->InitNetwork( name, _toCol, _fromCol, _edgeCol, _treeIDCol );
   else
      loadSuccess = false;
   
   return loadSuccess;
   }
   

bool ModelCollection::LoadAutoProcess( TiXmlNode *pXmlNode, MapLayer *pLayer )
   {
   bool loadSuccess = true;
   if ( pXmlNode->Type() != TiXmlNode::ELEMENT )
      return false;
      
   // process Model element
   //
   // <auto_process name="Impervious Surfaces (%)" col="EM_IMPERV" value="FRACIMPERV" />
   //
   TiXmlElement *pXmlElement = pXmlNode->ToElement();
   LPCTSTR name   = pXmlElement->Attribute( "name" );
   LPCTSTR id     = pXmlElement->Attribute( "id" );
   LPCTSTR col    = pXmlElement->Attribute( "col" );
   LPCTSTR value  = pXmlElement->Attribute( "value" );
   LPCTSTR query  = pXmlElement->Attribute( "query" );
   LPCTSTR constraints = pXmlElement->Attribute( "constraints" );
   LPCTSTR useDelta = pXmlElement->Attribute( "use_delta" );

   // note: "query", "constraints" are optional
   if ( name == NULL || id == NULL )
      {
      Report::ErrorMsg( _T("<auto_process> element missing required attribute" ) );
      return false;
      }
      
   AutoProcess *pModel = new AutoProcess( this );

   pModel->m_name = name;
   pModel->m_id   = atoi( id );

   // check to see that ID not already used
   AutoProcess *_pModel = GetAutoProcessFromID( pModel->m_id );
   if ( _pModel != NULL && _pModel->m_name != pModel->m_name )
      {
      CString msg;
      msg.Format( _T("Duplicate auto_process ID found (%s and %s)" ), name, (LPCTSTR) _pModel->m_name );
      Report::ErrorMsg( msg );
      }

   // populate model
   pModel->m_fieldName = col;

   if ( useDelta != NULL )
      {
      pModel->m_useDelta = true;
      pModel->m_deltaStr = useDelta;

      //pModel->ParseUseDelta( useDelta );
      ParseUseDelta( useDelta, pModel->m_useDeltas );
      }

   if ( constraints )
      {
      if ( constraints[ 0 ] == 'I' )  // INCREASE
         pModel->m_constraints = C_INCREASE_ONLY;
      else if ( constraints[ 0 ] == 'D' )
         pModel->m_constraints = C_DECREASE_ONLY;
      }

   if ( value != NULL )
      pModel->AddExpression( value, query );

   // make sure the col exists if specified
   int _col = -1;
   if ( col != NULL )
      {
      _col = pLayer->GetFieldCol( col );

      if ( _col < 0 )   // missing?
         {
         CString msg( "Missing field column specified by a process in Modeler input file (" );
         msg += col;
         msg += ").  This column is being added to the database.  You should save the database using File->Save->Cell Database";
         AfxMessageBox( msg, MB_OK );
         int width, decimals;
         GetTypeParams( TYPE_DOUBLE, width, decimals );
         pLayer->m_pDbTable->AddField( col, TYPE_DOUBLE, width, decimals, true );
         _col = pLayer->GetColCount() - 1;
         }
      }

   pModel->m_col = _col;

   pModel->LoadModelBaseXml( pXmlNode, pLayer );

   // Compile modelnote: Parser set up later
   pModel->Compile( pLayer );

   // store this parser
   m_autoProcessArray.Add( pModel ); 
   
   return loadSuccess;
   }


bool ModelCollection::LoadNetworkAutoProcess( TiXmlNode *pXmlNode, MapLayer* )
   {
   return false;
   }


void ModelCollection::ErrorMsg( LPCTSTR msg )
   {
   CString caption;
   caption.Format("Error reading Modeler input file, %s", m_fileName);
   AfxGetMainWnd()->MessageBox( msg, caption );
   }



      
//============================================================================//
//==================== M O D E L E R   M E T H O D S =========================//
//============================================================================//

Modeler::Modeler()
: m_isInitialized( false )
, m_pCurrentCollection( NULL )
   {
   ASSERT( m_pQueryEngine == NULL );
   }


Modeler::~Modeler()
   {
   if ( m_parserVars != NULL )
      {
      delete [] m_parserVars;
      m_parserVars = NULL;
      }

   Clear();

   //if ( m_pQueryEngine != NULL )
   //   {
   //   delete m_pQueryEngine;
   //   m_pQueryEngine = NULL;
   //   }
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



EvalModel *Modeler::GetEvalModelFromID( int id )
   {
   for ( INT_PTR i=0; i < m_modelCollectionArray.GetSize(); i++ )
      {
      EvalModel *pModel = m_modelCollectionArray[ i ]->GetEvalModelFromID( id );

      if ( pModel != NULL )
         return pModel;
      }

   return NULL;
   }

AutoProcess *Modeler::GetAutoProcessFromID( int id )
   {
   for ( INT_PTR i=0; i < m_modelCollectionArray.GetSize(); i++ )
      {
      AutoProcess *pProcess = m_modelCollectionArray[ i ]->GetAutoProcessFromID( id );

      if ( pProcess != NULL )
         return pProcess;
      }

   return NULL;
   }


//--------------------------------------------------------------------------------
//           eval model entry points
//--------------------------------------------------------------------------------

BOOL Modeler::EmInit( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   simTime = pEnvContext->currentYear;

   // note: this gets called for each eval model specified in the envx file.
   EvalModel *pModel = GetEvalModelFromID( pEnvContext->id );
   if ( pModel != NULL && pModel->m_name != pEnvContext->pEnvExtension->m_name )   // already exists - generate error and return
      {
      CString msg;
      msg.Format( _T("The model [%s] (id:%i) conflicts with a previously specified model [%s] - change the [id] in your .envx file before proceeding..."),
         (LPCTSTR) pEnvContext->pEnvExtension->m_name, pEnvContext->id, (LPCTSTR) pModel->m_name );
      ::AfxMessageBox( msg );
      return FALSE;
      }

   if ( pModel != NULL )   // already exist?  no other work needed
      return TRUE;

   // take care of static stuff if needed
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   Modeler::m_pMapLayer = pLayer;
   Modeler::m_colArea = pLayer->GetFieldCol( _T("Area") );

   // make a query engine if one doesn't exist already
   if (m_pQueryEngine == NULL)
      m_pQueryEngine = pEnvContext->pQueryEngine;

   // allocate field vars if not already allocated
   if ( m_parserVars == NULL )
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

   EvalModel *pModel = GetEvalModelFromID( pEnvContext->id );
   ASSERT( pModel != NULL );
   if ( pModel == NULL )
      return FALSE;

   return pModel->Run( pEnvContext, false );
   }


BOOL Modeler::EmRun( EnvContext *pEnvContext )
   {
   simTime = pEnvContext->currentYear;

   EvalModel *pModel = GetEvalModelFromID( pEnvContext->id );
   ASSERT( pModel != NULL );
   if ( pModel == NULL )
      return FALSE;

   return pModel->Run( pEnvContext, true );
   }


BOOL Modeler::EmSetup( EnvContext *pContext, HWND hWnd )
   {
   return FALSE;
   }

int Modeler::EmInputVar( int id, MODEL_VAR** modelVar )
   {
   EvalModel *pModel = GetEvalModelFromID( id );

   if ( pModel != NULL )
      return pModel->InputVar( id, modelVar );
   else
      return 0;
   }

int Modeler::EmOutputVar( int id, MODEL_VAR** modelVar )
   {
   EvalModel *pModel = GetEvalModelFromID( id );

   if ( pModel != NULL )
      return pModel->OutputVar( id, modelVar );
   else
      return 0;
   }



BOOL Modeler::ApInit( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   simTime = pEnvContext->currentYear;

   // note: this gets called for each eval model specified in the envx file.
   AutoProcess *pModel = GetAutoProcessFromID( pEnvContext->id );
   if ( pModel != NULL && pModel->m_name != pEnvContext->pEnvExtension->name )   // already exists - generate error and return
      {
      CString msg;
      msg.Format( _T("The model [%s] (id:%i) conflicts with a previously specified process [%s] - change the [id] in your .envx file before proceeding..."),
         (LPCTSTR) pEnvContext->pEnvExtension->name, pEnvContext->id, (LPCTSTR) pModel->m_name );
      ::AfxMessageBox( msg );
      return FALSE;
      }

   if ( pModel != NULL )   // already loaded?
      {
      ApInitRun( pEnvContext, false );
      return TRUE; 
      }

   // take care of static stuff if needed
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   Modeler::m_pMapLayer = pLayer;
   Modeler::m_colArea = pLayer->GetFieldCol( _T("Area") );

   // make a query engine if one doesn't exist already
   if (m_pQueryEngine == NULL)
      m_pQueryEngine = pEnvContext->pQueryEngine;

   // allocate field vars if not already allocated
   if ( m_parserVars == NULL )
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

   AutoProcess *pModel = GetAutoProcessFromID( pEnvContext->id );
   ASSERT( pModel != NULL );
   if ( pModel == NULL )
      return FALSE;

   return pModel->Run( pEnvContext, false );
   }


BOOL Modeler::ApRun( EnvContext *pEnvContext )
   {
   simTime = pEnvContext->currentYear;

   AutoProcess *pModel = GetAutoProcessFromID( pEnvContext->id );
   ASSERT( pModel != NULL );
   if ( pModel == NULL )
      return FALSE;


   return pModel->Run( pEnvContext, true );
   }


BOOL Modeler::ApSetup( EnvContext *pContext, HWND hWnd )
   {
   return FALSE;
   }

int Modeler::ApInputVar( int id, MODEL_VAR** modelVar )
   {
   AutoProcess *pModel = GetAutoProcessFromID( id );

   if ( pModel != NULL )
      return pModel->InputVar( id, modelVar );
   else
      return 0;
   }

int Modeler::ApOutputVar( int id, MODEL_VAR** modelVar )
   {
   AutoProcess *pModel = GetAutoProcessFromID( id );

   if ( pModel != NULL )
      return pModel->OutputVar( id, modelVar );
   else
      return 0;
   }
