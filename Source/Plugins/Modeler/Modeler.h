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
#pragma once

#include <EnvExtension.h>
#include <MapLayer.h>
#include <VData.h>
#include <afxtempl.h>
#include <queryengine.h>
#include <tinyxml.h>
#include <mtparser\mtparser.h>
#include <histogramArray.h>
#include <networktree.h>
//#include <Raster.h>
#include <PtrArray.h>


#define _EXPORT __declspec( dllexport )

//#include "resource.h"		// main symbols

class ModelBase;
class ModelCollection;

/*-------------------------------------------------------------------------------------------------------
 * Modeler is a basic computational processor that can handle several types of expressions, including:
 * 
 * 1) Lookup Table Variables
 * 2) Histogram sample variables
 * 3) Map-based expressions 
 *
 * Expressions can be tagged as AutoProcess or EvalModel, in which case they are treated appropriately
 * Expressions can be tagged as Outputs, in which case they are exposed in Envision is outputs
 * Expressions can be spatially targeted, in which case they are applied only to subset of IDU's
 *
 * Constants - contain a single floating point value

 *--------------------------------------------------------------------------------------------------------
 */


enum EVAL_TYPE { 
   ET_VALUE_HIGH        = 0,    // maximize the area within the target range
   ET_VALUE_LOW         = 1,    // minimize the area within the target range
   ET_TREND_INCREASING  = 2,    // maximize the trend within the target range (not implemented)
   ET_TREND_DECREASING  = 3,    // minimize the trend within the target range (not implemented)
   ET_UNDEFINED         = 4 
   };

enum EVAL_METHOD {
   EM_SUM         = 0,     // sum the value over the entire coverage ("sum")
   EM_MEAN        = 1,     // compute the average value over the entire coverage
   EM_AREAWTMEAN  = 2,     // compute an area-weighted average value over the entire coverage ("areaWtMean")
   EM_PCT_AREA    = 3,      // compute an percent (0-100) area over the entire coverage ("pctArea")
   EM_DENSITY     = 4      // value is densit, sum absolute over entire IDUs
   };

enum {  /*constraints*/
   C_INCREASE_ONLY = 1,
   C_DECREASE_ONLY = 2
   };

enum MODEL_TYPE {
   MT_EVAL_MODEL      = 0,
   MT_NETWORK_MODEL   = 1,
   MT_AUTO_PROCESS    = 10,
   MT_NETWORK_PROCESS = 11
   };

enum NETWORK_CA_METHOD {   // contributing areas method
   NCAM_NONE        = 0,      // "none" - ignore contributing areas
   NCAM_SUM         = 1,      // "sum" - sum the value over the entire contributing area for a reach
   NCAM_MEAN        = 2,      // "mean" - compute the average value over the entire contributing area for a reach
   NCAM_AREAWTMEAN  = 3       // "areawtmean" - compute an area-weighted average value over the entire contributing area for a reach
   };

enum NETWORK_METHOD {
   NM_CUMULATIVE_MASS_BALANCE = 0,  // "cmb" - perform a cumulative mass balance through the network.  Requires flow_col be defined in the input file
   NM_CUMULATIVE_SCORE        = 1,  // "cs" - edge accumulates upstream values
   NM_CAREA_SCORE             = 2   // "ca" - edge is scored based on contributing area score
   }; 

enum VAR_TYPE {               //  Description                                              |      Extent         | useDelta?|
   VT_UNDEFINED        = -1,  //|----------------------------------------------------------|---------------------|-----------
   VT_EXPRESSION       =  0,  // (default) value defined only by a evaluatable expression  | always idu-extent   |    no   |
                              //
   VT_PCT_AREA         =  1,  // "pctArea" - value defined as a percent of the total IDU   | always global extent|    no    |
                              // area that satisfies the specified query, expressed as a
                              // non-decimal percent.  No "value" attr specified.
                              //
   VT_PCT_CONTRIB_AREA =  2,  // "pctContribArea" - not implemented
                              //
   VT_SUM_AREA         =  4,  // "sum" sums the "value" expression over an area            | always global extent |   yes   |
                              //  defined by the query.  If "use_delta" specified, the 
                              //  query area is further restricted to those IDUs in which
                              //  a change, specified by the useDelta list, occured
                              // 
   VT_AREAWTMEAN       =  8,  // "areaWtMean" - area wt mean of the "value" expression,    | always global extent |   yes   |
                              // summed over the query area. If "use_delta" specified, the 
                              //  query area is further restricted to those IDUs in which
                              //  a change, specified by the useDelta list, occured 
   VT_DIFFEQ           = 16,  // "diffeq" - differential equation requiring integration
   VT_GLOBALEXT        = 32,  // "global" - global-scope variable, computed based on other | always global extent  |   no   |
                              // global scope variables only (not implemented)
   VT_RASTER           = 64,  // "raster" a variable defined by a raster
   VT_RULE            = 128,  // variable defined by a set of one or more local or spatial rules (not implemented)
   VT_DENSITY         = 256   // variable value is a density - multiply by area and sum over IDUS
   };


struct USE_DELTA
   {
   int col;
   VData value;

   USE_DELTA() : col( -1 ), value() { }
   USE_DELTA( USE_DELTA &ud ) { *this = ud; }
   USE_DELTA( int _col, VData _value ) : col( _col ), value( _value ) { }
   USE_DELTA &operator = ( USE_DELTA &ud ) { col = ud.col; value = ud.value; return *this; }
   };


class TimeFct : public MTFunctionI
{
  virtual const MTCHAR* getSymbol() { return _T("time"); }

  virtual const MTCHAR* getHelpString(){ return _T("time()"); }
  virtual const MTCHAR* getDescription()
      { return _T("Return the current simulation time"); }
  virtual int getNbArgs(){ return 0; }
  virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg);
       
  virtual MTFunctionI* spawn(){ return new TimeFct(); }

  	virtual bool isConstant(){ return false; }
};    

class ExpFct : public MTFunctionI
{
  virtual const MTCHAR* getSymbol() { return _T("exp"); }

  virtual const MTCHAR* getHelpString(){ return _T("exp()"); }
  virtual const MTCHAR* getDescription()
      { return _T("Return exp"); }
  virtual int getNbArgs(){ return 1; }
  virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg);
       
  virtual MTFunctionI* spawn(){ return new ExpFct(); }

  	virtual bool isConstant(){ return false; }
};      


class GetNeighborCountFct : public MTFunctionI
{
  virtual const MTCHAR* getSymbol() { return _T("getNeighborCount"); }

  virtual const MTCHAR* getHelpString(){ return _T("getNeighborCount()"); }
  virtual const MTCHAR* getDescription()
      { return _T("Return number of neighbors for the current IDU"); }
  virtual int getNbArgs(){ return 0; }
  virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg);
       
  virtual MTFunctionI* spawn(){ return new GetNeighborCountFct(); }

  	virtual bool isConstant(){ return false; }
};      



/*
class GetNearbyAreaFct : public MTFunctionI
{
// float GetNearbyArea( "LULC=2", diameter )
  virtual const MTCHAR* getSymbol() { return _T("GetNearbyArea"); }

  virtual const MTCHAR* getHelpString(){ return _T("GetNearbyArea()"); }
  virtual const MTCHAR* getDescription()
      { return _T("GetNearbyArea()"); }
  virtual int getNbArgs(){ return 2; }
  virtual MTDOUBLE evaluate(unsigned int nbArgs, const MTDOUBLE *pArg);
       
  virtual MTFunctionI* spawn(){ return new GetNearbyAreaFct(); }

  	virtual bool isConstant(){ return false; }
};      
*/



class MParser : public MTParser
{
public:
   MParser() : MTParser() 
      { 
      this->defineFunc(new TimeFct()); 
      this->defineFunc(new ExpFct());
      this->defineFunc(new GetNeighborCountFct());
      } 
};


// class report provides one of two types of summaries. 
//   1) if an expression is specified through the "value" element of the specification, then a summary
//      value over the coverage is computed for this report variable.  If a query is specified, the
//      calculation is limited to the portion of the coverate that satisifies the query.
//   2) if NO expression is specified with a "value" element, then the report generates only a summary 
//        of the portion of the area in the coverage that satisfies a query
/*
class Report
{
public:
   int m_id;
   CString m_name;
   CString m_query;
   Query *m_pQuery;

   CString m_expr;
   MParser *m_pParser;

   float  m_value;      // output variable - decimal percent of allocation satisfying query

   Report( int id, LPCTSTR name,  LPCTSTR expr, LPCTSTR query ) 
      : m_id( id ), m_name( name ), m_expr( expr ), m_pParser( NULL ), m_query( query ), m_pQuery( NULL ), m_value( 0 ) { }

   ~Report();
};
*/

// class LookupTable defines a lookup table variable.  It maps the value of the variable in specified column to some lookup value
//   The field referenced in the MapLayer must be of type TYPE_INT, or at least convertible to type int
class LookupTable : public CMap< int, int, double, double >
{
public:
   LookupTable( void ) : m_col( -1 ), m_colAttr( -1 ), m_colOutput( -1 ), m_value( 0 ), m_defaultValue( 0 ) { }
   CString m_name;          // name of this LookupTable
   CString m_attrColName;   // name of the referenced MapLayer column
   int     m_colAttr;       // index of the references MapLayer column
   CString m_colName;       // name of the column populated with the lookup value (empty means no column populated)
   int     m_col;           // index of the column populated with the lookup value (<0 means no column populated)
   
   CString m_outputColName; // column to write to
   int     m_colOutput;
   
   double  m_value;         // evaluated (looked-up) value
   double  m_defaultValue;  

   bool Evaluate( int idu );
};


// class Constant defines a constant value, stored in m_value

class Constant
{
public:
   CString m_name;      // name of constant
   double  m_value;     // value of constant
   bool    m_isInput;
   CString m_description;

   Constant() : m_value( 0), m_isInput( false ) { }

   bool Evaluate() { return true; }  // nothing required
};


//-- class Variable defines a variable. ----------------------------------------
//
//  There are currently four types of variables:
//     1) expression variables, defined by an evaluatable expression
//     2) pctArea variables, that return the pctArea of the target 
//           areas that satisfies the variable's query
//     3) sumArea variables, that return the values of the expression summed over
//        query/total area.  This are always global in extent
//------------------------------------------------------------------------------

enum EXTENT { EXTENT_IDU, EXTENT_GLOBAL };

class Variable
{
public:
   CString   m_name;
   VAR_TYPE  m_varType;
   TYPE      m_dataType;

   CString   m_expression;
   double    m_value;
   float     m_cumValue;
   float     m_queryArea;
   bool      m_isOutput;
   int       m_col;     // column associated with this variable (-1 if no col)

   MParser  *m_pParser;
   CString   m_query;      // query tha constrains where "value" is evaluated
   Query    *m_pQuery;      // null for expression variable
   EXTENT    m_extent;      // IDU based or global based

   CString   m_valueQuery;  // "extra" query informatio associated with a value expression
   Query    *m_pValueQuery;

   // the following are only allowed for global-extent variables
   bool      m_useDelta;    // 
   CString   m_deltaStr;
   CArray< USE_DELTA, USE_DELTA& > m_useDeltas;

   // the following are used only for raster-based variables
   //Raster   *m_pRaster;
   float     m_cellSize;

   int       m_timing;          // flag indicating if global variables are solved pre (0) or post (1) run.  Default is 0
   double    m_stepSize;

   // additional members
   bool      m_evaluate;    // temporary variable to indicate this variable should be evaluated for an IDU
    
   ModelBase       *m_pModel;       // NULL for collection-level variable;
   ModelCollection *m_pCollection;  // collection this variable belongs to

   bool Compile( ModelBase*, ModelCollection*, MapLayer*);
   bool Evaluate( void );
   
   Variable() : m_value( 0 ), m_cumValue( 0 ), m_queryArea( 0 ), m_isOutput( false ), m_col( -1 ), 
         m_varType( VT_UNDEFINED ), m_dataType( TYPE_DOUBLE ), m_pParser( NULL ), 
         m_pQuery( NULL ), m_pValueQuery( NULL ), m_extent( EXTENT_IDU ),
         m_useDelta( false ), /*m_pRaster( NULL ),*/ m_cellSize( 0 ), m_timing( 0 ), m_stepSize( 1 ), m_pModel( NULL ), m_pCollection( NULL ) { }

   ~Variable();
};


class Expression
{
public:
   CString m_name;
   CString m_expression;
   CString m_queryStr;

   Query  *m_pQuery;
   MParser *m_pParser;

   Expression( void ) : m_name(), m_expression(), m_queryStr(), m_pParser( NULL ), m_pQuery( NULL ) { }
  ~Expression( void );
};


class LookupArray   : public PtrArray< LookupTable > { };
class ConstantArray : public PtrArray< Constant > { };
class VariableArray : public PtrArray< Variable > { };
class HistoArray    : public PtrArray< HistogramArray > { };
class ExprArray     : public PtrArray< Expression > { };


class ModelBase       // base class for EvalModels, AutoProcesses - just some basic info
{
public:
   CString m_initFnInfo;

   CString m_name;
   CString m_fieldName;    // empty if not used
   int     m_col;

   ExprArray m_exprArray;

   //CString m_expression;
   //CString m_queryStr;     // query defining area of application (empty=global)
   int     m_constraints;

   bool    m_useDelta;
   CString m_deltaStr;
   bool ParseUseDelta( LPCTSTR deltaStr );
   CArray< USE_DELTA, USE_DELTA& > m_useDeltas;
 
   //MTParser *m_pParser;
   //Query    *m_pQuery;
   bool     m_isRaster;
   //Raster  *m_pRaster;     // NULL for idu-based models, nonNULL for raster-based models

   ModelCollection *m_pCollection;     // collection this belongs to

public:
   HistoArray    m_histogramsArray;
   LookupArray   m_lookupTableArray;
   ConstantArray m_constantArray;
   VariableArray m_variableArray;

public:
   ModelBase( ModelCollection *pCollection ) 
      : m_col( -1 ),  m_constraints( 0 ), m_useDelta( false ), m_isRaster( false ),
        /*m_pRaster( NULL ),*/ m_pCollection( pCollection ) { }

   ~ModelBase() { Clear(); }

   Expression *AddExpression( LPCTSTR expr, LPCTSTR query );

   void SetVars( EnvExtension *pExt );

   bool  Compile( MapLayer* );
   bool  UpdateVariables( EnvContext*, int idu, bool useDelta );

   void  Clear();
   bool  LoadModelBaseXml( TiXmlNode *pXmlModelNode, MapLayer *pLayer );

protected:
   bool LoadInclude( TiXmlNode *pXmlNode, MapLayer* );

public:
   virtual MODEL_TYPE GetModelType() = 0;
   //virtual BOOL Run( EnvContext* ) { return TRUE; }
};


// base class for NetworkEvalModel, NetworkAutoProcess
class NetworkModel
{
protected:
   NetworkModel( NETWORK_METHOD method ) : m_netMethod( method ), m_contributingAreaMethod( NCAM_NONE ),
      m_pNetworkLayer( NULL ), m_pIDULayer( NULL ),
      m_colNetwork( -1 ), m_colIDU( -1 ),
      m_colOutput( -1 ), m_colOrder( -1 ), m_colFlow( -1 ),
      m_edgesReportingScores( -1 )
   { } 

public:
   MapLayer *m_pNetworkLayer;
   MapLayer *m_pIDULayer;
   CString   m_networkLayerName;

   int m_colNetwork;             // column in the network coverage used to store results
   int m_colIDU;                 // IDU column used to compute areas (should be query???)
   int m_colReachIndex;			   // column in the IDU database that contains the corresponding reach
   int m_colOutput;              // network layer col receiving output
   int m_colOrder;               // network layer containing network order column
   int m_colFlow;                // network layer contain flow information (only use for cumulative mass balance method)

protected:
   int m_edgesReportingScores;   // count of network lines reporting scores
   
public:
   NETWORK_METHOD    m_netMethod;
   NETWORK_CA_METHOD m_contributingAreaMethod;

protected:
   NetworkTree m_network;
   CArray< CUIntArray, CUIntArray& > m_iduIndexArray; // length =  #stream reaches, contains array of IDU's adjacent to that reach

   void BuildIndex( void ); 

   void ComputeNetworkScore( ModelBase *pBase, int year, int endYear );
   float ComputeNetworkScore( ModelBase *pBase, NetworkNode *pNode, float &area, int year, int endYear );

public:
   BOOL InitNetwork( LPCTSTR name, int colTo, int colFrom, int colEdge, int colTreeID );
};


class EvalModel : public ModelBase, public EnvEvaluator
{
public:
   EVAL_TYPE   m_evalType;
   EVAL_METHOD m_evalMethod;

   float m_lowerBound;
   float m_upperBound;

   float m_score;
   float m_rawScore;

public:
   EvalModel( ModelCollection *pCollection ) : ModelBase( pCollection ), EnvEvaluator(), m_evalType( ET_UNDEFINED ), 
      m_evalMethod( EM_MEAN ), m_lowerBound( 0 ), m_upperBound( 0 ) { /*AddVar( false, "Score", m_rawScore, _T("Evaluative (raw) score for this model" ) ); */ }

   virtual MODEL_TYPE GetModelType() { return MT_EVAL_MODEL; }
   virtual BOOL Run( EnvContext*, bool useAddDelta );

protected:
   bool RunMap( EnvContext*, bool useAddDelta );
   bool RunDelta( EnvContext* );
};


class NetworkEvalModel : public EvalModel, public NetworkModel
{
public:
   NetworkEvalModel( ModelCollection *pCollection ) : EvalModel( pCollection ), NetworkModel( NM_CUMULATIVE_MASS_BALANCE ) { }

   virtual MODEL_TYPE GetModelType() { return MT_NETWORK_MODEL; }
   virtual BOOL Run( EnvContext*, bool useAddDelta );
};


class AutoProcess : public ModelBase, public EnvModelProcess
{
public:
   AutoProcess( ModelCollection *pCollection ) : ModelBase( pCollection ), EnvModelProcess() { }

   MODEL_TYPE GetModelType() { return MT_AUTO_PROCESS; }

   virtual BOOL Run( EnvContext*, bool useAddDelta );
   BOOL RunDelta( EnvContext* );
};


class NetworkAutoProcess : public AutoProcess, public NetworkModel
{
protected:

public:
   NetworkAutoProcess( ModelCollection *pCollection ) : AutoProcess( pCollection ), NetworkModel( NM_CUMULATIVE_MASS_BALANCE ) { }
   
   MODEL_TYPE GetModelType() { return MT_NETWORK_PROCESS; }

   virtual BOOL Run( EnvContext*, bool useAddDelta );
   BOOL RunDelta( EnvContext* );
};


class ModelCollection
{
public:
   ModelCollection() : m_nextID( 0 ) { }
   ~ModelCollection() { Clear(); }
   
   int m_nextID;

   // ModelCollection-scope variables
   HistoArray    m_histogramsArray;
   LookupArray   m_lookupTableArray;
   ConstantArray m_constantArray;
   VariableArray m_variableArray;

   // model collection info
   CString   m_fileName;
   PtrArray< AutoProcess > m_autoProcessArray;
   PtrArray< EvalModel >   m_evalModelArray;
   //CArray< NetworkAutoProcess*, NetworkAutoProcess* > m_netAutoProcessArray;
   //CArray< NetworkEvalModel*,   NetworkEvalModel*   > m_netEvalModelArray;

   bool LoadXml( LPCTSTR filename, MapLayer *pLayer,  bool isImporting );       // returns number of nodes in the first level
   bool LoadXml( TiXmlNode*, bool isImporting );
   int  SaveXml( LPCTSTR filename );
   int  SaveXml( FILE *fp, bool includeHdr, bool useFileRef=true );

   void Clear( void );

   EvalModel   *GetEvalModelFromID( int id );
   AutoProcess *GetAutoProcessFromID( int id );

   bool UpdateVariables( EnvContext*, int idu, bool useDelta ); // sets parservars and evaluates ModelCollection-scope variables
   bool UpdateGlobalExtentVariables( EnvContext*, int timing, bool useAddDelta );
   int  CollectGlobalExtentVariables( VariableArray &va, int timing );

protected:
   bool LoadAutoProcess( TiXmlNode *pXmlNode, MapLayer* );
   bool LoadEvalModel( TiXmlNode *pXmlNode, MapLayer* );
   bool LoadNetworkAutoProcess( TiXmlNode *pXmlNode, MapLayer* );
   bool LoadNetworkEvalModel( TiXmlNode *pXmlNode, MapLayer* );
   void ErrorMsg( LPCTSTR msg );      
};


class Modeler
{
protected:
   CArray< ModelCollection*, ModelCollection* > m_modelCollectionArray;

public:
   Modeler();
   ~Modeler();

   bool Initialize();
   bool m_isInitialized;

   // idu field variable values shared by all parsers
   static double *m_parserVars;
   static CMap<int, int, int, int > m_usedParserVars;  // key and value is column index
   //static void SetParserVars( MapLayer *pLayer, int record );

   static int     m_nextID;
   static int     m_colArea;
   ModelCollection *m_pCurrentCollection;

   static QueryEngine *m_pQueryEngine;
   static MapLayer    *m_pMapLayer;

   EvalModel   *GetEvalModelFromID( int id );
   AutoProcess *GetAutoProcessFromID( int id );

   // autoprocess entry points
   BOOL ApInit( EnvContext *pEnvContext, LPCTSTR initStr );
   BOOL ApInitRun( EnvContext *pEnvContext, bool );
   BOOL ApRun( EnvContext *pEnvContext );
   BOOL ApSetup( EnvContext *pContext, HWND hWnd );
   int  ApInputVar( int id, MODEL_VAR** modelVar );
   int  ApOutputVar( int id, MODEL_VAR** modelVar );
   //BOOL ApProcessMap( MapLayer *pLayer, int id ) { ASSERT( 0 );  return TRUE; }

   // eval model entry points
   virtual BOOL EmInit( EnvContext *pEnvContext, LPCTSTR initStr );
   BOOL EmInitRun( EnvContext *pEnvContext, bool );
   BOOL EmRun( EnvContext *pEnvContext );
   BOOL EmSetup( EnvContext *pContext, HWND hWnd );
   int  EmInputVar( int id, MODEL_VAR** modelVar );
   int  EmOutputVar( int id, MODEL_VAR** modelVar );

   int AddModelCollection( ModelCollection *pModel ) { return (int) m_modelCollectionArray.Add( pModel ); }

   void Clear();
};


extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new Modeler; }
