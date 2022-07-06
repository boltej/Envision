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
// Policy.h: interface for the Policy class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_POLICY_H__7035E45D_7B3D_4C4B_BC3D_FFFE266EDBCC__INCLUDED_)
#define AFX_POLICY_H__7035E45D_7B3D_4C4B_BC3D_FFFE266EDBCC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_MFC
#include <afxtempl.h>
#endif
#include <Vdata.h>
#include <QueryEngine.h>
#include <MapExprEngine.h>
#include <tixml.h>
#include <Vdataobj.h>
#include <DOIndex.h>

#include "EnvAPI.h"

/*
#ifdef ENV_IMPORT
#define ENV_EXPORT __declspec( dllimport )
#else
#define ENV_EXPORT  __declspec( dllexport )
#endif
*/
//#include "DeltaArray.h"

// define a policy element.
class DeltaArray;
class PolicyManager;
class MapLayer;
class EnvModel;
class MTParser;
class PolicyConstraint;
class GlobalConstraint;
class Policy;

typedef PolicyConstraint OutcomeConstraint;   // these are the same object


enum CTYPE   { CT_UNKNOWN=0, CT_RESOURCE_LIMIT, CT_MAX_AREA, CT_MAX_COUNT };
enum CTBASIS { CTB_UNKNOWN=0, CTB_ANNUAL, CTB_CUMULATIVE };
enum CBASIS  { CB_UNKNOWN=0, CB_UNIT_AREA, CB_ABSOLUTE };

// SCORE_MODIFIER types
enum SM_TYPE
   {
   SM_SCENARIO = 1,
   SM_INCR     = 2,
   SM_DECR     = 4,
   SM_ABS      = 8  // absolute quantity - not supported at this time
   };

//SCORE_MODIFIERS are specifications that modify computed objective (goal) 
//  scores in specific circumstances
struct ENVAPI SCORE_MODIFIER
   {
   SM_TYPE type;        // scenarion of incr/decr
   float   value;       // incr/decr value 
   CString condition;   // query or scenario name (depends on type)
   int     scenario;    // scenario identifier (-1 is not used) 
   Query  *pQuery;      // query (only valid for incr/decr types)

   SCORE_MODIFIER( SM_TYPE _type, float _value, LPCTSTR _condition, int _scenario )
      : type( _type ), value( _value ), condition( _condition ), scenario( _scenario ), pQuery( NULL ) { }

   SCORE_MODIFIER() : type ( SM_SCENARIO ), value( 0 ), scenario( -1 ), pQuery( NULL ) { }

   SCORE_MODIFIER &operator = (const SCORE_MODIFIER &sm ) 
      { type=sm.type; value=sm.value; condition=sm.condition; scenario=sm.scenario; pQuery=NULL; return *this; }

   SCORE_MODIFIER(const SCORE_MODIFIER &sm ) { *this = sm; }

   ~SCORE_MODIFIER( void );
   };


//GOAL_SCORES store information about policy scoring under specific circumstances
//  indicated by the spatial query
struct ENVAPI GOAL_SCORE
   {   
   float score;    // -3 to +3

   CString queryStr;     
   Query *pQuery;

   GOAL_SCORE() : score( 0.0f ), pQuery( NULL ) { }
   
   GOAL_SCORE( float _score, LPCTSTR _queryStr ) 
      : score( _score ), queryStr( _queryStr ), pQuery( NULL ) { }

   GOAL_SCORE( const GOAL_SCORE &g ) { *this = g; }

   GOAL_SCORE &operator = (const GOAL_SCORE &g) { score = g.score; queryStr = g.queryStr; pQuery = NULL; return *this; }

   ~GOAL_SCORE( void );
   };


class GoalScoreArray : public CArray< GOAL_SCORE, GOAL_SCORE& >
{
};

class ModifierArray : public CArray< SCORE_MODIFIER, SCORE_MODIFIER& >
{
};


// ScoreArray's hold all relevant scoring information an objective for this policy.  This includes
//   both goal scores and modifiers for a given metagoal
class ENVAPI ObjectiveScores
{
public:
   int     m_metagoalIndex;        // -1=global, 0 or greater corresponds to metagoal Model Index.
   CString m_objectiveName;

   GoalScoreArray m_goalScoreArray;
   ModifierArray  m_modifierArray;

public:
   ObjectiveScores() : m_metagoalIndex( -2 ) { };

   ObjectiveScores( LPCTSTR name, int metagoalIndex )
      : m_metagoalIndex( metagoalIndex ), m_objectiveName( name ) { }

   ObjectiveScores( ObjectiveScores &osa ) { *this = osa; }

   ObjectiveScores &operator = ( ObjectiveScores &osa );

   void Clear();

   void AddScore(float value, LPCTSTR query) { GOAL_SCORE gs(value, query); m_goalScoreArray.Add(gs); }

   void AddModifier(SM_TYPE type, float value, LPCTSTR condition, int scenario)
      {
      SCORE_MODIFIER sm(type, value, condition, scenario); m_modifierArray.Add(sm);
      }
};




// An ObjectiveArray holds all the scoring information for all of the policy's metagoals
class ENVAPI ObjectiveArray : public CArray< ObjectiveScores*, ObjectiveScores* >
{
public:
   ~ObjectiveArray() { Clear(); }

   ObjectiveArray() : CArray<ObjectiveScores*, ObjectiveScores*>() { }

   ObjectiveArray( ObjectiveArray &oa ) { *this = oa; }

   ObjectiveArray &operator = (ObjectiveArray &oa );

   void Clear() { for ( int i=0; i < GetSize(); i++) delete GetAt( i ); RemoveAll(); }      

   ObjectiveScores *AddObjective( LPCTSTR name, int metagoalIndex );
};


/*----------------------------------------------------------------------------------
 * Programmers notes about Outcomes and Multioutcomes
 *
 * Basic concepts
 *
 *  OutcomeInfo:  The finest units of outcomes.  Contains the column, value, and delay
 *       associated with a particular outcome (see exception below for Buffer function)
 *  MultiOutcomeInfo:  zero or more OutcomeInfo's applied as a group, w/ an associated
 *       probability.
 *  MultiOutcomeArray:  A collection of MultiOutcomeInfo's, representing all possible
 *       MultiOutcomes for a policy.  Each policy has one of these collections, and upon
 *       application, a single MultiOutcome is selected for application
 *
 * OutcomeFunction:  An Outcome can be defined with a function (derived from OutcomeFunction)
 *       Currently, we support only several these, e.g.:
 *
 *       1) BufferOutcomeFunction - contains two MultiOutcomeInfo's in its own 
 *             MultiOutcomeArray.  The first applies to the area inside the buffer, 
 *             the second to area outside the buffer.  NOTE:  This function DOES NOT 
 *             set the OutcomeInfo col or value fields - you need to look at the contained
 *             MultiOutcomeInfo's for this information.
 *
 *       Additional outcome functions described below - look for classes derived from OutcomeFunction
 *
 * Outcomes can be blocked from being applied in a given cell by previous persistent
 *    policies.  Information about blocked outcomes by cell is maintained by the
 *    operative EnvModel (gpModel) and accessed through the OUTCOME_STATUS, OutcomeStatusArray,
 *    and OutcomeStatusMap classes.  EnvModel stores an m_outcomeStatusMap member that
 *-----------------------------------------------------------------------------------*/

enum OUTCOME_DIRECTIVE_TYPE { ODT_EXPIRE, ODT_PERSIST, ODT_DELAY };

class OutcomeDirective
   {
   public:
      OUTCOME_DIRECTIVE_TYPE type; 
      int period;          // time period associated with the directive
      VData outcome;       // optional data argument

      OutcomeDirective( LPCTSTR typeStr, int _period ) : period( _period ) { outcome.SetNull(); SetType( typeStr); }
      OutcomeDirective( LPCTSTR typeStr, int _period, LPCTSTR value ) : period( _period ), outcome( value ) { SetType( typeStr ); }
      OutcomeDirective( LPCTSTR typeStr, int _period, float  value ) : period( _period ), outcome( value ) { SetType( typeStr ); }
      OutcomeDirective( LPCTSTR typeStr, int _period, double value ) : period( _period ), outcome( value ) { SetType( typeStr ); }
      OutcomeDirective( LPCTSTR typeStr, int _period, int    value ) : period( _period ), outcome( value ) { SetType( typeStr ); }

      OutcomeDirective(const OutcomeDirective &od ) : type( od.type ), period( od.period ), outcome( od.outcome ) { }

      void ToString( CString& str );

      void SetType( LPCTSTR typeStr )
         {
         switch( *typeStr )
            {
            case 'e':   type = ODT_EXPIRE;   break;
            case 'p':   type = ODT_PERSIST;  break;
            case 'd':   type = ODT_DELAY;    break;
            default:
               CString msg( "Unrecognized directive in outcome string: " );
               msg += typeStr;
               Report::LogWarning( msg );
            }
         }
   };


enum OUTCOME_FUNCTION_TYPE { OFT_BUFFER, OFT_EXPAND, OFT_CONNECT, OFT_ADDPOINT, OFT_INCR };

class OutcomeFunction
   {
   public:
      OutcomeFunction( Policy *pPolicy ) : m_pPolicy( pPolicy ){}
      virtual ~OutcomeFunction() {}
      virtual OutcomeFunction* Clone() = 0;
      virtual OUTCOME_FUNCTION_TYPE GetType() = 0;

      Policy *m_pPolicy;

   public:
      virtual void SetArgs( CStringArray *pArgs ) = 0;
      virtual bool GetArg( int i, CString &arg ) = 0;
      virtual bool Build() = 0;
      virtual bool Run( DeltaArray *pDeltaArray, int cell, int currentYear, int persistence ) = 0;

      virtual void ToString( CString& ) = 0;

      static OutcomeFunction* MakeFunction( LPCTSTR functionName, Policy *pPolicy );
   };


class ENVAPI OutcomeInfo
   {
   public:
      Policy *pPolicy;

   public:
      char  field[ 32 ];
      int   col;
      VData value;
      MapExpr *m_pMapExpr;    // NULL if not defined

      OutcomeFunction *m_pFunction;    // NULL unless this outcome executes a function
      OutcomeDirective *m_pDirective;  // NULL unless this outcome has extra directive information

      void ToString( CString&, bool useHtml );

   private:
      OutcomeInfo() { ASSERT(0); }

   public:
      OutcomeInfo( Policy *_pPolicy, LPCTSTR _field, VData _value, int _delay, OutcomeDirective *pDirective=NULL );
//         : value( _value ), col( -1 ), m_pFunction( NULL ), m_pDirective( pDirective )
//         { lstrcpy( field, _field ); }

      OutcomeInfo(Policy *_pPolicy, OutcomeFunction *pFunction )
         : pPolicy(_pPolicy), col( -1 ), m_pFunction( pFunction ), m_pDirective( NULL ), m_pMapExpr( NULL )
         { field[ 0 ]=NULL; value.type = TYPE_NULL; }

      OutcomeInfo( Policy *_pPolicy ) : pPolicy(_pPolicy), col( -1 ), m_pFunction( NULL ), m_pDirective( NULL ), m_pMapExpr( NULL ) { field[ 0 ]=NULL; value.type = TYPE_NULL; }

      ~OutcomeInfo( )
         {
         pPolicy = NULL;

         if ( m_pFunction )
            delete m_pFunction;

         if ( m_pDirective )
            delete m_pDirective;
         }

      OutcomeInfo( const OutcomeInfo& oi ){ *this = oi; }

      OutcomeInfo& operator=( const OutcomeInfo& oi )
         {
         pPolicy = oi.pPolicy;
         strcpy_s( field, 32, oi.field );
         col   = oi.col;
         value = oi.value;
         //delay = oi.delay;
         
         if ( oi.m_pFunction )
            m_pFunction = oi.m_pFunction->Clone();
         else
            m_pFunction = NULL;

         if ( oi.m_pDirective != NULL )
            m_pDirective = new OutcomeDirective( *(oi.m_pDirective ) );
         else
            m_pDirective = NULL;

         if (m_pMapExpr != NULL )
            m_pMapExpr = oi.m_pMapExpr;

         return *this;
         }
   };


//---------------------------------------------------------------------------
// a MultiOutcomeInfo store information for a single collection of outcomes.
// Policies can have 0-many Multioutcomes
//---------------------------------------------------------------------------

class ENVAPI MultiOutcomeInfo
{
public:
   MultiOutcomeInfo() : probability( -1.0f ) { }
   ~MultiOutcomeInfo() { RemoveAll(); }
   MultiOutcomeInfo( const MultiOutcomeInfo &m ){ *this = m; }

   MultiOutcomeInfo& operator = ( const MultiOutcomeInfo &m );

   void RemoveAll();

   int AddOutcome( OutcomeInfo *pOutcome ){ return (int) outcomeInfoArray.Add( pOutcome ); }

   int GetOutcomeCount() const { return (int) outcomeInfoArray.GetSize(); }
   OutcomeInfo &GetOutcome( int i ) { return *outcomeInfoArray[ i ]; }
   const OutcomeInfo &GetOutcome( int i ) const { return *outcomeInfoArray[ i ]; }
   float GetProbability() const { return probability; }

   void ToString( CString&, bool includeOutcomeProb=true );

public:
   CArray< OutcomeInfo*, OutcomeInfo* > outcomeInfoArray;
   float  probability;    // 0-100, chance of this one being selected.

// outcome constraints
public:
   int  AddConstraint( OutcomeConstraint *pConstraint ) { return (int) m_constraintArray.Add( pConstraint ); }
   void DeleteConstraint( int i ) { return m_constraintArray.RemoveAt( i ); }
   int  GetConstraintCount( void ) const { return (int) m_constraintArray.GetSize(); }
   OutcomeConstraint *GetConstraint( int i ) { return m_constraintArray[ i ]; }

protected:
   PtrArray< OutcomeConstraint > m_constraintArray;
};


class ENVAPI MultiOutcomeArray
   {
   friend class PolicyManager;

   public:
      MultiOutcomeArray( void ){}
      MultiOutcomeArray( const MultiOutcomeArray &multiOutcomeArray ) { *this = multiOutcomeArray; }
      ~MultiOutcomeArray( void ){ RemoveAll(); }

      MultiOutcomeArray& operator = ( const MultiOutcomeArray &m )
         {
         m_multiOutcomeInfoArray.RemoveAll();

         for ( int i=0; i < (int) m.GetSize(); i++ )
            AddMultiOutcome( new MultiOutcomeInfo( m.GetMultiOutcome(i) ) );

         return *this;
         }

      int AddMultiOutcome( MultiOutcomeInfo *pMultiOutcome ){ return (int) m_multiOutcomeInfoArray.Add( pMultiOutcome ); }
      
      int GetSize( void ) const { return (int) m_multiOutcomeInfoArray.GetCount(); }
      MultiOutcomeInfo &GetMultiOutcome( int i ){ return *m_multiOutcomeInfoArray.GetAt( i ); }
      const MultiOutcomeInfo &GetMultiOutcome( int i ) const { return *m_multiOutcomeInfoArray.GetAt( i ); }

      void RemoveAll( void )
         {
         for ( int i=0; i < m_multiOutcomeInfoArray.GetCount(); i++ )
            delete m_multiOutcomeInfoArray.GetAt(i);
         m_multiOutcomeInfoArray.RemoveAll(); 
         }

   protected:
      CArray< MultiOutcomeInfo*, MultiOutcomeInfo* > m_multiOutcomeInfoArray;
   };


class ENVAPI BufferOutcomeFunction : public OutcomeFunction
   {
   //
   // Entry syntax:
   //    Buffer( distance, query string, Buffer Outcome string, Other Outcome string )
   //
   // This functions make two polygons out of one.  
   //    1) the intersection of the cell and a buffer created around the cells that satisfy the query string
   //    2) the cell minus the cell created in step 1.
   //
   // Both new cells have the same attributes as the original cell, including the application of this policy
   // The "Buffer Outcome" is applied only to the cell created in step 1
   // The "Other Outcome" is applied only to the cell created in step 2

   public:
      BufferOutcomeFunction( Policy *pPolicy ) : OutcomeFunction( pPolicy ), m_pArgs( NULL ) {}
      ~BufferOutcomeFunction() { if ( m_pArgs ) delete m_pArgs; }
      virtual OutcomeFunction* Clone();
      virtual OUTCOME_FUNCTION_TYPE GetType() { return OFT_BUFFER; }


      virtual void SetArgs( CStringArray *pArgs ){ m_pArgs = pArgs; }
      virtual bool GetArg( int i, CString &arg ) { if( i >= m_pArgs->GetSize() ) return false; arg = m_pArgs->GetAt( i ); return true; }

      virtual bool Build();
      virtual bool Run( DeltaArray *pDeltaArray, int cell, int currentYear, int persistence );

      virtual void ToString( CString &str ) { str.Format( "Buffer( %s, %s, %s, %s )", 
         (LPCTSTR) m_pArgs->GetAt( 0 ), (LPCTSTR) m_pArgs->GetAt( 1 ),
         (LPCTSTR) m_pArgs->GetAt(2), (LPCTSTR) m_pArgs->GetAt(3) ); }

      //static bool m_enabled;

   protected:
      float  m_distance;
      Query *m_pQuery;

      MultiOutcomeArray m_insideBufferMultiOutcomeArray;
      MultiOutcomeArray m_outsideBufferMultiOutcomeArray;

      CStringArray *m_pArgs;  // had to move this from the base class.  Its a bad design
   };


class ENVAPI AddPointOutcomeFunction : public OutcomeFunction
   {
   //
   // Entry syntax:
   //    AddPoint( layerName, x, y )

   public:
      AddPointOutcomeFunction( Policy *pPolicy ) : OutcomeFunction( pPolicy ), m_pArgs( NULL ), m_x( 0 ), m_y( 0 ) {}
      ~AddPointOutcomeFunction() { if ( m_pArgs ) delete m_pArgs; }
      virtual OutcomeFunction* Clone();
      virtual OUTCOME_FUNCTION_TYPE GetType() { return OFT_ADDPOINT; }

      virtual void SetArgs( CStringArray *pArgs ){ m_pArgs = pArgs; }
      virtual bool GetArg( int i, CString &arg ) { if( i >= m_pArgs->GetSize() ) return false; arg = m_pArgs->GetAt( i ); return true; }
      virtual bool Build();
      virtual bool Run( DeltaArray *pDeltaArray, int cell, int currentYear, int persistence );

      virtual void ToString( CString &str ) { str.Format( "AddPoint( %s, %s, %s )", 
         (LPCTSTR) m_pArgs->GetAt( 0 ), (LPCTSTR) m_pArgs->GetAt( 1 ), (LPCTSTR) m_pArgs->GetAt(2) ); }

   protected:
      CString   m_layerName;
      MapLayer *m_pLayer;
      float     m_x;
      float     m_y;
      
      CStringArray *m_pArgs;  // had to move this from the base class.  Its a bad design
   };


class IncrementOutcomeFunction : public OutcomeFunction
   {
   //
   // Entry syntax:
   //    Increment( field, increment )
   //
   public:
      IncrementOutcomeFunction( Policy *pPolicy ) : OutcomeFunction( pPolicy ), m_pArgs( NULL ), m_pLayer( NULL ), m_col( -1 ), m_increment( 0 ) {}
      ~IncrementOutcomeFunction() { if ( m_pArgs ) delete m_pArgs; }

      virtual OutcomeFunction* Clone();
      virtual OUTCOME_FUNCTION_TYPE GetType() { return OFT_INCR; }
      virtual void SetArgs( CStringArray *pArgs ){ m_pArgs = pArgs; }
      virtual bool GetArg( int i, CString &arg ) { if( i >= m_pArgs->GetSize() ) return false; arg = m_pArgs->GetAt( i ); return true; }
      virtual bool Build();
      virtual bool Run( DeltaArray *pDeltaArray, int cell, int currentYear, int persistence );

      virtual void ToString( CString &str ) { str.Format( "Increment( %s, %s )", (LPCTSTR) m_pArgs->GetAt( 0 ), (LPCTSTR) m_pArgs->GetAt( 1 ) ); }

   protected:
      CString   m_layerName;
      CString   m_fieldName;
      MapLayer *m_pLayer;
      int       m_col;

      float m_increment;

      CStringArray *m_pArgs;  // had to move this from the base class.  Its a bad design

   };



class ExpandOutcomeFunction : public OutcomeFunction
   {
   //
   // Entry syntax:
   //    Expand( contraint query string, area, Outcome string )
   //
   // This function expands out from the current IDU into IDU's that satisfy the constraint query, executing the outcome, until the area parameter is met.
   
   public:
      ExpandOutcomeFunction( Policy *pPolicy ) : OutcomeFunction( pPolicy ), m_pArgs( NULL ), m_pQuery( NULL ), m_colArea( -1 ), m_area( 0 ), m_requireAdjacent( true ) {}
      ~ExpandOutcomeFunction() { if ( m_pArgs ) delete m_pArgs; }
      virtual OutcomeFunction* Clone();
      virtual OUTCOME_FUNCTION_TYPE GetType() { return OFT_EXPAND; }

      virtual void SetArgs( CStringArray *pArgs ){ m_pArgs = pArgs; }
      virtual bool GetArg( int i, CString &arg ) { if( i >= m_pArgs->GetSize() ) return false; arg = m_pArgs->GetAt( i ); return true; }
      virtual bool Build();
      virtual bool Run( DeltaArray *pDeltaArray, int cell, int currentYear, int persistence );

      virtual void ToString( CString &str ) { str.Format( "Expand( %s, %s, %s )", 
         (LPCTSTR) m_pArgs->GetAt( 0 ), (LPCTSTR) m_pArgs->GetAt( 1 ), (LPCTSTR) m_pArgs->GetAt( 2 ) ); }

      //static bool m_enabled;

   protected:
      CArray< int, int > m_expansionIDUs;   // idus that
      int    m_colArea;
      float  m_area;
      bool   m_requireAdjacent;
      Query *m_pQuery;

      MultiOutcomeArray m_multiOutcomeArray;

      CStringArray *m_pArgs;  // had to move this from the base class.  Its a bad design
   
      float AddIDU( int idu, bool &added, float areaSoFar, float maxArea );

   };


// indexing for policy constraint lookups

struct COL_INFO  // stores info about field/values pairs specified in a policy constraint lookup table spec
   {
   int  doCol;       // column in the attached VDataObj for this entry (used when Build()ing index
   int  mapCol;      // column in the IDU coverage for creating IDU-based keys
   bool isValueFieldRef;   // specifies whether the value is a numveric value or a self field reference
   VData keyValue;   //  key value (from field/value pair). if field ref, holds the col for the self-reference field 
                     //  or the value if not a field ref    
   CString fieldName;

   COL_INFO( void ) : doCol( -1 ), mapCol( -1 ), isValueFieldRef( false ), keyValue(), fieldName() { }
   };


// Lookup table index.  This class allows for up to four columns and/or values to be specified 
// that are used to look up a cost from the attached dataobj table
class ENVAPI LTIndex : public DOIndex
{
protected:
   PtrArray< COL_INFO > m_keyColArray;
   MapLayer  *m_pMapLayer;

public:
   LTIndex( MapLayer *pLayer, DataObj *pDataObj ) 
      : DOIndex( pDataObj, -1 )  // no size
      , m_pMapLayer( pLayer )
      { }

   LTIndex( LTIndex &index ) { *this = index; }
   
   LTIndex &operator = ( LTIndex& ); 

   int AddCol( COL_INFO *pInfo ) { return (int) m_keyColArray.Add( pInfo );  }

   UINT GetColValue( int colIndex, int idu )
      {
      if ( colIndex >= (int) m_keyColArray.GetSize() ) return 0;

      COL_INFO *pInfo = m_keyColArray[ colIndex ];
      UINT key = 0;
      
      if ( pInfo->isValueFieldRef )
         key = m_pMapLayer->m_pData->GetAsUInt( m_keyColArray[ colIndex ]->mapCol, idu );
      else
         pInfo->keyValue.GetAsUInt( key );

      return key;
      }

   UINT GetKeyFromMap( int idu );

   virtual UINT GetKey( int row ); 

   bool Lookup( int idu, int lookupCol, float &value )
      {      
      UINT key=GetKeyFromMap( idu );
      
      int  doRow = 0;
      if ( m_map.Lookup( key, doRow ) == FALSE )
         return FALSE;

      value = m_pDataObj->GetAsFloat( lookupCol, doRow );
      return true;      
      }
};
  



//------------------------------------------------------
// About Constraints:
//
// Constraints are defined globally, and contributors to those
// global constraints include PolicyConstraints and OutcomeConstraints.
//
// The basic idea is to track contributions from individual policies and
// compare these to the global constraint;  Once the global constraint is 
// exceeded, the policy is no longer applied in that year.  So, it is 
// necessary to track year by year contributions. Contributions come in
// two forms:  1) Initial costs when the policy is first applied, and 
// 2) maintenance costs, appiled over a period after the policy is applied.
// Maintenance costs are tracked in a list (gpModel->m_activeConstraintList)
// Maintenance costs are applied at the start of the year to the global constraints.
// After that, each new policy application's initial cost is added into the
// global constraint.
//---------------------------------------------------------

class ENVAPI GlobalConstraint
{
friend class DataManager;

public:
   GlobalConstraint( PolicyManager *pPM, LPCTSTR name, CTYPE type, LPCTSTR expr );
   GlobalConstraint( GlobalConstraint &gc ) : m_name( gc.m_name ), m_type( gc.m_type ), m_value( gc.m_value ), m_currentValue( gc.m_currentValue ), m_expression( gc.m_expression ), m_pMapExpr( gc.m_pMapExpr ) { }   
   
   GlobalConstraint & operator = ( GlobalConstraint & gc )
       { m_pPolicyManager = gc.m_pPolicyManager,
         m_name = gc.m_name; m_type = gc.m_type; 
         m_value = gc.m_value; m_currentValue = gc.m_currentValue; 
         m_expression = gc.m_expression; 
         gc.m_pMapExpr = gc.m_pMapExpr; 
         return *this; }   

   void  Reset( void );  // called at the start of each time step - updates value, reset currentValue=0, increments cumulative
   void  ResetCumulative( void ) { m_cumulativeValue = 0; }
   bool  DoesConstraintApply( float increment=0 ); 
   bool  ApplyPolicyConstraint( PolicyConstraint *pConstraint, int idu, float area, bool useInitCost );
   float AvailableCapacity( void ) { return ( m_value - m_currentValue ); }  

   PolicyManager *m_pPolicyManager;

   CString m_name;
   CTYPE   m_type;     // type of constraint
   CString m_expression;      // expression string for computing value

   LPCTSTR GetTypeString( CTYPE type );
   LPCTSTR GetExpression() { return (LPCTSTR)m_expression; }
   
protected:
   float   m_value;           // max value before constraint hits. (NOTE: assumed constant for now)
   float   m_currentValue;    // current value of constraint - sum of costs, or how much of the resource has been consumed?
   float   m_cumulativeValue; // sum of costs since last reset
   float   m_currentMaintenanceValue;
   float   m_currentInitValue;


   MapExpr *m_pMapExpr;       // associated MapExpr (NULL if not an evaluated expression)

};


class ENVAPI PolicyConstraint
{
friend class Policy;

private:
   PolicyConstraint() {}  // prevent default constructor

public:
   PolicyConstraint( Policy *pPolicy, LPCTSTR name, CBASIS basis, LPCTSTR initCostExpr, LPCTSTR maintenanceCostExpr, LPCTSTR durationExpr, LPCTSTR lookup );

   PolicyConstraint( PolicyConstraint &pc ) { *this=pc; }

   ~PolicyConstraint( void ) { if ( m_pLookupTableIndex != NULL ) delete m_pLookupTableIndex; }
   
   PolicyConstraint & operator = ( PolicyConstraint &pc )
      {
      m_pPolicy             = pc.m_pPolicy;
      m_name                = pc.m_name;
      m_gcName              = pc.m_gcName; 
      m_basis               = pc.m_basis; 
      m_initialCostExpr     = pc.m_initialCostExpr; 
      m_maintenanceCostExpr = pc.m_maintenanceCostExpr;
      m_durationExpr        = pc.m_durationExpr;
      m_initialCost         = pc.m_initialCost;
      m_maintenanceCost     = pc.m_maintenanceCost;
      m_duration            = pc.m_duration;
      m_pGlobalConstraint   = pc.m_pGlobalConstraint; 
      m_pLookupTable        = pc.m_pLookupTable;
      m_pLookupTableIndex   = pc.m_pLookupTableIndex ? new LTIndex( *(pc.m_pLookupTableIndex) ) : NULL;
      m_colMaintenanceCost  = pc.m_colMaintenanceCost;
      m_colInitialCost      = pc.m_colInitialCost; 
      
      m_pMainCostMapExpr    = pc.m_pMainCostMapExpr;    // NULL if not defined
      m_pInitCostMapExpr    = pc.m_pInitCostMapExpr;    // NULL if not defined
      m_pDurationMapExpr    = pc.m_pDurationMapExpr;    // NULL if not defined

      return *this;
      }

   bool ParseCost( VDataObj *pLookupTable,LPCTSTR costExpr, float &cost, int &col, MapExpr*& pMapExpr);
   VDataObj *GetLookupTable( void ) { return m_pLookupTable; }
   
   bool GetInitialCost( int idu, float area, float &cost );
   bool GetMaintenanceCost( int idu, float area, float &cost );
   bool GetDuration(int idu, int &duration); // { return m_duration; }

   Policy *m_pPolicy;
   CString m_name;
   CString m_gcName;    // name of the associated global constraint
   CBASIS  m_basis;     // see enum

protected:
   float m_initialCost;       // init cost (NOTE: assumed constant for now)
   float m_maintenanceCost;   // maintenance cost
   int  m_duration;

   bool  GetCost( int idu, MapExpr *pMapExpr, float costToUse, int colCost, float &cost );
   float AdjustCostToBasis( float cost, float area );

public:
   CString m_initialCostExpr;       // expression used for computing value
   CString m_maintenanceCostExpr;   // expression used for computing value
   CString m_durationExpr; 

   MapExpr *m_pMainCostMapExpr;     // NULL if not defined
   MapExpr *m_pInitCostMapExpr;     // NULL if not defined
   MapExpr *m_pDurationMapExpr;     // NULL if not defined

   GlobalConstraint *m_pGlobalConstraint;    // associated global constraint   
   
public:
   // cost identification through table lookups
   CString   m_lookupStr;
   VDataObj *m_pLookupTable;                  // associated data object for cost information (NULL if not defined)
   LTIndex  *m_pLookupTableIndex;

protected:
   int   m_colMaintenanceCost;
   int   m_colInitialCost;

public:
   LPCTSTR GetBasisString( CBASIS type );

protected:
   static PtrArray< VDataObj > m_lookupTableArray;
};


/////////////////////////// P O L I C Y ///////////////////////////////////

class ENVAPI Policy
{
friend class PolicyManager;

public:
   PolicyManager *m_pPolicyManager;

private:
   Policy() { ASSERT(0); }

public:
	Policy( PolicyManager *pPM );
   Policy( Policy &policy ) { *this = policy; }      // copy constructor
	~Policy();              // CFM: For some reason, having a virtual destructor causes problems when
                           //      a dll "news" a Policy, which is needed for the PolicyMetaProcess
   Policy& operator = ( Policy& );

   int  m_id;               // id of this policy
   int  m_index;            // offset in policyManager array
   int  m_age;              // how long has this policy been active
   bool m_use;
   int  m_useInSensitivity;  // 0=not included, 1=included but not current, -1=included and current
   bool m_isShared;
   bool m_isEditable;
   bool m_isAdded;          // was added this session
   bool m_isScheduled;      // requires start date, excluded from actor decisionmaking
   bool m_mandatory;
   float m_compliance;      // level of mandatory compliance
   int  m_startDate;        // -1 = disable
   int  m_endDate;          // -1 =disable

   // policy attributes (from policy template)
   CString  m_name;                  // name of the policy
   CString  m_source;                // ini files, databases, etc...
   CString  m_originator;
   CString  m_narrative;
   CString  m_studyArea;
   long     m_color;                 // RGB( red, grn,blue)
   bool     m_exclusive;             // precludes other policies from being applied during it's persistence
   int      m_persistenceMin;
   int      m_persistenceMax;
   //float    m_preference;            // -3 to +3, 0 = no preference
   CString  m_mapLayer;
   MapLayer *m_pMapLayer;
   
   CString  m_siteAttrStr;           // should be READ-ONLY
   CString  m_outcomeStr;            // should be READ-ONLY
   //CString  m_prefSelStr; 

   // summary stats over the course of a run
   int   m_appliedCount;         // how often has this policy been used in the current cycle
   int   m_cumAppliedCount;      // how often has this policy been used
   float m_appliedArea;          // in current invocation (doesn't include expand areas)
   float m_expansionArea;        // expansion area, in current invocation (doesn't include applied area)

   float m_cumAppliedArea;       // total area for this policy
   float m_cumExpansionArea;     // total expansion area for this policy, for this run
   float m_potentialArea;        // computed at most once at the beginning
   float m_cumCost;              // cost of constraints applied for this policy
   
   // FROM PROBES 
   int m_passedCount;      
   int m_rejectedSiteAttr;
   int m_rejectedPolicyConstraint;
   int m_rejectedNoCostInfo;
   int m_noOutcomeCount;

public:
   int  AddConstraint( PolicyConstraint *pConstraint ) { return (int) m_constraintArray.Add( pConstraint ); }
   void DeleteConstraint( int i ) { return m_constraintArray.RemoveAt( i ); }
   void RemoveAllConstraints( void ) { m_constraintArray.RemoveAll(); }
   int  GetConstraintCount( void ) { return (int) m_constraintArray.GetSize(); }
   PolicyConstraint *GetConstraint( int i ) { return m_constraintArray[ i ]; }

   float GetMaxApplicationArea( int idu );

protected:
   PtrArray< PolicyConstraint > m_constraintArray;

public:
   ObjectiveArray m_objectiveArray;    // global objective, + one for each MetaGoal
  
   Query *m_pSiteAttrQuery;

   //MTParser *m_pParser;       // parser for preferential selection string: if NULL, assume bias=0;

//protected:
   MultiOutcomeArray m_multiOutcomeArray;

public:
   void SetSiteAttrQuery( LPCTSTR queryStr ) { m_siteAttrStr = queryStr; }
   void GetSiteAttrFriendly( CString &str, int options );
   void GetOutcomesFriendly( CString &str );
   void GetScoresFriendly( CString &str );
   bool SetOutcomes( LPCTSTR outcomes );
      
   int   GetMultiOutcomeCount() { return m_multiOutcomeArray.GetSize(); }
   const MultiOutcomeInfo &GetMultiOutcome( int i ) { return m_multiOutcomeArray.GetMultiOutcome( i ); }
   void  ClearMultiOutcomeArray(){ m_multiOutcomeArray.RemoveAll(); }
   int   AddMultiOutcome( MultiOutcomeInfo *pMOI ) { return m_multiOutcomeArray.AddMultiOutcome( pMOI ); } 
   int   GetPersistence();      // randomly retrieve a persistence for this policy
   //float EvaluatePreference();

public:
   static bool ParseOutcomes( Policy *pPolicy, CString &outcomeStr, MultiOutcomeArray & );  // array version
   //static bool ParseOutcome( CString &outcomeStr, MultiOutcomeInfo & );    // single MultiOutcomeInfo version

   //void FixUpOutcomePolicyPtrs();

   // variable management for parser variables (pref selection calculations)
public:
   //static double *m_parserVars;
   //static CMap<int, int, int, int > m_usedParserVars;  // key and value is column index
   //static void SetParserVars( MapLayer *pLayer, int record );

public:
   ObjectiveScores *FindObjective( LPCTSTR objName );

   void AddObjective( LPCTSTR name, int metagoalIndex )  { m_objectiveArray.AddObjective( name, metagoalIndex ); }

   float GetGoalScore( int metagoalIndex );  // -1 <= metagoalIndex < gpModel->GetMetagoalCount();  use -1 for global objective

   void  GetIduObjectiveScores( int idu, float *scores );
   float GetIduObjectiveScore( int idu, int objective );  // returns objective score for the IDU in range [-3,3]. use -1 for global objective score

   int GetGoalCount() { return (int) m_objectiveArray.GetSize() - 1; }   // Note: DOES NOT include "Global" objective

   void ResetStats( bool initRun ) 
      {
      // alway reset the following (during initRun and during a run)
      m_appliedCount      = 0;     // how often has this policy been used in the current cycle
      m_appliedArea       = 0;     // in current invocation
      m_expansionArea     = 0;
      m_passedCount       = 0;      
      m_rejectedSiteAttr  = 0;
      m_rejectedPolicyConstraint = 0;
      m_rejectedNoCostInfo = 0;
      m_noOutcomeCount     = 0;
   
      if ( initRun == true )        // only reset these during initRun
         {
         m_cumAppliedCount  = 0;    // how often has this policy been used
         m_cumAppliedArea   = 0;    // total area for this policy for this run
         m_cumExpansionArea = 0;    // total expansion area for this policy, for this run
         m_cumCost          = 0; }      
      }

};


//////////////////////  P O L I C Y M A N A G E R  //////////////////////////////////

class ENVAPI PolicyArray : public CArray< Policy*, Policy* >
{
public:
   bool m_deleteOnDelete;

   PolicyArray() : m_deleteOnDelete( true ) { }
   PolicyArray( PolicyArray &policyArray ) { *this = policyArray; }      // copy constructor
   PolicyArray& operator = ( PolicyArray & );
   void RemoveAll();      
};


struct POLICY_SCHEDULE     // for managing scheduled policies
   {
   int year;
   Policy *pPolicy;

   POLICY_SCHEDULE() : year( -1 ), pPolicy( NULL ) {}
   POLICY_SCHEDULE( int _year, Policy *_pPolicy ) : year( _year ), pPolicy( _pPolicy ) { }
   POLICY_SCHEDULE( const POLICY_SCHEDULE &p ) { *this = p; }      // copy constructor
   POLICY_SCHEDULE &operator = (const POLICY_SCHEDULE &p ) { year = p.year; pPolicy = p.pPolicy; return *this; }   
   };


class ENVAPI PolicyManager
{
public:
   PolicyManager( EnvModel* );
   virtual ~PolicyManager();
  
   void    SetQueryEngine( QueryEngine *pQE ) { m_pQueryEngine = pQE; }
   int     AddPolicy( Policy *pPolicy );  // returns index of added item
   void    DeletePolicy( int index );
   void    RemoveAllPolicies( void ) { m_policyArray.RemoveAll(); m_policyIDToIndexMap.RemoveAll(); }

   // global constraints
   int     AddGlobalConstraint( GlobalConstraint *pConstraint ) { return (int) m_constraintArray.Add( pConstraint ); }
   int     GetGlobalConstraintCount( void ) { return (int) m_constraintArray.GetSize(); }
   GlobalConstraint *GetGlobalConstraint( int i ) { return m_constraintArray[ i ]; }
   GlobalConstraint *FindGlobalConstraint( LPCTSTR name );
   PtrArray<GlobalConstraint>& GetGlobalConstraintArray() { return m_constraintArray; }
   void    RemoveAllGlobalConstraints( void ) { m_constraintArray.RemoveAll(); }
   bool    ReplaceGlobalConstraints( PtrArray< GlobalConstraint > &newConstraintArray );

   void    ClearPolicySchedule() { m_policySchedule.RemoveAll(); }
   int     AddPolicySchedule(int year, Policy* pPolicy) { POLICY_SCHEDULE ps(year, pPolicy); return (int)m_policySchedule.Add(ps); }
   int     GetPolicyScheduleCount() { return (int) m_policySchedule.GetSize(); }
   POLICY_SCHEDULE &GetPolicySchedule( int i ) { return m_policySchedule[ i ]; }
   void    BuildPolicySchedule();

   //void    ResetPolicyAppliedCount();
   void    ResetPolicyIndices();
   Policy *FindPolicy( LPCTSTR name, int *index=NULL );
   
   int     GetNextPolicyID() { ASSERT( m_nextPolicyID >= -1 ); if ( m_nextPolicyID < 0 ) return 0; int id = m_nextPolicyID; m_nextPolicyID++; return id; }

   PolicyArray &GetPolicyArray() { return m_policyArray; }
   int     GetPolicyCount() { return (int) m_policyArray.GetSize(); }
   int     GetUsedPolicyCount();
   int     GetMandatoryPolicyCount() { int count=0; for ( int i=0; i < m_policyArray.GetSize(); i++ ) if ( m_policyArray[ i ]->m_mandatory ) count++; return count; }
   Policy *GetPolicy( int i ) { return m_policyArray[ i ]; }
   Policy *GetPolicyFromID( int id );
   int     GetPolicyIndexFromID( int id ) { int index=-1; bool ok=m_policyIDToIndexMap.Lookup( id, index ); return ok ? index : -1; }

   int     GetDeletedPolicyCount() { return (int) m_deletedPolicyArray.GetSize(); }
   Policy *GetDeletedPolicy( int i ) { return m_deletedPolicyArray[ i ]; }
   Policy *GetDeletedPolicyFromID( int id );
   void    RemoveAllDeletedPolicies( void ){ m_deletedPolicyArray.RemoveAll(); }

   int     CompilePolicies( MapLayer *pLayer );
   bool    CompilePolicy( Policy *pPolicy, MapLayer *pLayer );
   bool    ValidateOutcomeFields( Policy *pPolicy, MapLayer *pLayer );

   QueryEngine *m_pQueryEngine;  // memory managed elsewhere.  Must be set before and policies are added

public:
   EnvModel       *m_pEnvModel;
protected:
   PolicyArray     m_policyArray;
   PolicyArray     m_deletedPolicyArray;
   CArray< POLICY_SCHEDULE, POLICY_SCHEDULE& > m_policySchedule;

   PtrArray< GlobalConstraint > m_constraintArray;

   CMap< int, int, int, int > m_policyIDToIndexMap;

public:
   CString m_path;         // location of policy file - envx or xml, depending 
   CString m_importPath;   // if imported (as in "file=''"), path of  import 
   int     m_loadStatus;   // -1=undefined, 0=loaded from envx, 1=loaded from xml file
   bool    m_includeInSave;       // when saving project with external ref, update this file?

   int LoadXml( LPCTSTR filename, bool isImporting, bool appendToExisting ); 
   int LoadXml( TiXmlNode *pPoliciesNode, bool appendToExisting );
   int SaveXml( LPCTSTR filename );
   int SaveXml( FILE *fp, bool includeHdr, bool useFileRef );

protected:
   PolicyConstraint *LoadXmlConstraint( TiXmlElement *pXmlConstraint, Policy *pPolicy );

public:
   int     m_nextPolicyID;
};


inline
LPCTSTR GlobalConstraint::GetTypeString( CTYPE type)
   {
   switch( type )
      {
      case CT_RESOURCE_LIMIT:    return _T("resource_limit");
      case CT_MAX_AREA:          return _T("max_area");
      case CT_MAX_COUNT:         return _T("max_count");

      case CT_UNKNOWN:
         return _T("unknown");

      default:
         ASSERT( 0 );
         return NULL;
      }
   }


inline
LPCTSTR PolicyConstraint::GetBasisString( CBASIS basis )
   {
   switch( basis )
      {
      case CB_UNIT_AREA:    return _T("unit_area");
      case CB_ABSOLUTE:     return _T("absolute");

      case CT_UNKNOWN:
         return _T("unknown");

      default:
         ASSERT( 0 );
         return NULL;
      }
   }


inline
MultiOutcomeInfo& MultiOutcomeInfo::operator = ( const MultiOutcomeInfo &m )
      {
      MultiOutcomeInfo &_m = (MultiOutcomeInfo&) m;

      probability = m.probability;

      RemoveAll();
      for ( int i=0; i < m.GetOutcomeCount(); i++ )
         outcomeInfoArray.Add( new OutcomeInfo( m.GetOutcome(i) ) );

      m_constraintArray.RemoveAll();
      for ( int i=0; i < m.GetConstraintCount(); i++ )
         {
         OutcomeConstraint *pConstraint = _m.GetConstraint( i );
         AddConstraint( new OutcomeConstraint( *pConstraint ) );
         }

      return *this;
      }

inline
void MultiOutcomeInfo::RemoveAll()
   {
   for ( int i=0; i < GetOutcomeCount(); i++ )
      delete outcomeInfoArray.GetAt(i);
   outcomeInfoArray.RemoveAll();
   }



#endif // !defined(AFX_POLICY_H__7035E45D_7B3D_4C4B_BC3D_FFFE266EDBCC__INCLUDED_)
