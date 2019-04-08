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
#include <mtparser\mtparser.h>
#include <PtrArray.h>

#define _EXPORT __declspec( dllexport )

class QueryEngine;
class Query;
class TargetDlg;
class Target;
class TargetProcess;
class FDataObj;

/*---------------------------------------------------------------------------------------------------------------------------------------
 * The TargetProcess class manages "Target" models.  The basic idea of a target is to  provide a mechanism by which
 * an application can specify landscape attribute "targets" to achieve.  Targets are specified by a set of columns to use,
 * by a target to reach (expressed in terms of absolutle levels or percent of landscape in the specified condition) at various points in time.
 * These "various points in time" are defined in terms of linear or exponential growth rates or a specified schedule.
 *
 * To reach a Target's specified quantities, the Target also specifies a "target surface", using a set of spatial queries and
 * associated surface height formulas, that is in effect a two-dimensional (across a map) set of points that represent,
 * in dimensionless terms, the spatial distribution pattern of the target variable. To spatially allocate the target
 * variable, the Target process computes the taret surface at each time step, and compare this surface to the current
 * distribution pattern of the target variable.  It then allocates the increment (or decrement) of the target variable 
 * needed to be accomodated by landscape based on the difference between the target surface and the existing variable surface.
 *
 * The Target process can accomodate multiple target models (variables).  Each target is assigned its own modelID in the 
 * Envision project file (envx) as per the usual model specification process.  Modeled allocations are specified in xml
 * input files according to the documentation in the Envision Developer's Manual.
 *
 *  Required Project Settings (<All Configurations>)
 *    1) General->Character Set: Not Set
 *    2) C/C++ ->General->Additional Include Directories: $(SolutionDir);$(SolutionDir)\libs;
 *    3) C/C++ ->Preprocessor->Preprocessor Definitions: WIN32;_WINDOWS;_AFXEXT;"__EXPORT__=__declspec( dllimport )";
 *    4) Linker->General->Additional Library Directories: $(SolutionDir)\$(ConfigurationName)
 *    5) Linker->Input->Additional Dependencies: libs.lib
 *    6) Build Events->Post Build Events: copy $(TargetPath) $(SolutionDir)
 *
 *    Debug Configuration Only:  C/C++ ->Preprocessor->Preprocessor Definitions:  Add DEBUG;_DEBUG
 *    Release Configuration Only:  C/C++ ->Preprocessor->Preprocessor Definitions:  Add NDEBUG
 *---------------------------------------------------------------------------------------------------------------------------------------
 */


enum TARGET_METHOD
   {
   TM_UNDEFINED   = -1,
   TM_RATE_LINEAR =  0,
   TM_RATE_EXP    =  1,
   TM_TIMESERIES  =  2,   // not currently implemented
   TM_TIMESERIES_CONSTANT_RATE=3
   };
#define TM_TIMESERIES_LINEAR TM_TIMESERIES

class Constant
{
public:
   CString   m_name;
   CString   m_expression;
   double    m_value;

   Constant( ) : m_value( 0 ) {} 
};



// this corresponds to an <allocation> element in the target file

struct PARSER_VAR
{
int col;
MTDOUBLE value;
};


struct PREFERENCE
   {
   CString name;
   
   CString queryStr;
   Query  *pQuery;

   Target *pTarget;

   float   value;       // preference value (multiplier)


   ///
   MTParser parser;
   UINT parserVarCount;
   PARSER_VAR *parserVars;  // array of parser variable

   ///

   //PREFERENCE() : pTarget(NULL), pQuery( NULL ), value( 1.0f ), target( -1 ), currentValue( 0 ) { }
   PREFERENCE( LPCTSTR _name, Target *_pTarget, LPCTSTR query, float _value ) : name( _name ),
      pTarget(_pTarget), queryStr( query ), pQuery( NULL ), value( _value ), currentValue( 0 ), target( -1 ) { }

   // the following are for estimating parameters (value)
   float currentValue;  // this has units of #
   float target;        // this has units of #
   
   ~PREFERENCE( void );
   };


struct ALLOCATION
   {
   CString name;
   float   multiplier;   // obsolete, deprecated

   CString queryStr;
   Query  *pQuery;

   Target *pTarget;

   CString expression;
   float value;

   MTParser parser;

   UINT parserVarCount;
   PARSER_VAR *parserVars;   // array of parser variable

   float   currentValue;     // total target value at any given point in time (non-density)
   float   currentCapacity;  // total capacity for accomodating new values of the target variable (absolute numbers - not density)

   //ALLOCATION() : pTarget(NULL), pQuery( NULL ), parser(), multiplier( 1.0f ), parserVars( NULL ), currentValue(0), currentCapacity(0) { }
   
   ALLOCATION( LPCTSTR _name, Target *_pTarget, LPCTSTR query, LPCTSTR expr, float _multiplier ) : name( _name ), 
      pTarget(_pTarget), queryStr( query ), expression( expr ), pQuery( NULL ), parserVars( NULL ), multiplier( _multiplier ),
      currentValue(0), currentCapacity(0)
      { }

   ~ALLOCATION();

   void ResetCurrent() { currentValue = currentCapacity = 0; }
   bool Compile( Target *pTarget, const MapLayer *pLayer );
   };


class PreferenceArray : public PtrArray< PREFERENCE >
   {
   public:
      PREFERENCE *Add( LPCTSTR name, Target *pTarget, LPCTSTR queryStr, float value ) { PREFERENCE *p= new PREFERENCE( name, pTarget, queryStr, value ); CArray::Add( p ); return p; }
   };


// this is a collection of <allocation>'s corresponding to an <allocations> element in the target file
class AllocationSet : public PtrArray< ALLOCATION >
   {
   public:
      AllocationSet() : m_id( -1 ) { }

      int     m_id;
      CString m_name;
      CString m_description;

      PreferenceArray m_preferences;

      ALLOCATION *AddAllocation( LPCTSTR name, Target*, LPCTSTR queryStr, LPCTSTR expr, float multiplier );
      PREFERENCE *AddPreference( LPCTSTR name, Target*, LPCTSTR queryStr, float value );
   };

class AllocationSetArray : public PtrArray< AllocationSet > { };


class TargetReport
   {
   public:
      CString m_name;
      CString m_query;
      Query *m_pQuery;

      Target *m_pTarget;

      float  m_value;      // output variable - decimal percent of allocation satisfying query

      TargetReport( LPCTSTR name, Target *_pTarget, LPCTSTR query ) 
         :  m_name( name ), m_pTarget( _pTarget), m_query( query ), m_pQuery( NULL ), m_value( 0 ) { }
      ~TargetReport();
   };


class TargetReportArray : public CArray< TargetReport*, TargetReport* >
   {
   public:
      ~TargetReportArray() { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); RemoveAll(); }
   };

//---------------------------------------------------------------------------
// a "Target" provides for implementation of a target variable.  It corresponds
//    to a <target> collection in an input file.
//
//   NOTE:  It is NOT an eval model, just a "model" for a target variable within a Target autonomous process
//
// it consists of a target variable and an associated Allocation Set that
// contains the allocation descriptors for that target
//---------------------------------------------------------------------------

class Target
{
friend class TargetDlg;

public:
   Target( TargetProcess*, int id, LPCTSTR name );
  ~Target();

   CString m_name;
   CString m_description;
   CString m_initStr;
   CString m_queryStr;
   int     m_modelID;

   float   m_startTotalActualValue;      // total target value at beginning of Run() (non-density)
   float   m_lastTotalActualValue;       // current total value of the target variable (updated during Run()) (non-density)
   float   m_curTotalActualValue;        // actual (realized) total value of the target variable (updated during Run()) (non-density)

   float   m_curTotalTargetValue;        // desired value of target variable at any given time


   //float   m_newTotalTarget;              // actual (realized) total value of the target variable at year end (non-density)

   float   m_totalCapacity;            // total capacity for accomodating new values of the target variable (absolute numbers - not density)
   float   m_totalAvailCapacity;       // difference between capacity and current density (absolute number - not density)
   float   m_totalModifiedAvailCapacity;  // total available capacity adjusted with preference modifiers (absolute number - not density)
   float   m_totalOverCapacity;
   float   m_totalPercentAvailable;    // (avail/total) - decimal percent
   float   m_totalAllocated;           // amount of target actually allocated

   double m_rate;                      
   int    m_method;
   int    m_areaUnits;

   CString   m_targetValues;
   FDataObj *m_pTargetData;      // input data table for target inputs
   float     m_targetData_year0;

   int    m_colArea;
   int    m_colTarget;                 // column containing the value of the target variable
   int    m_colDensXarea;              // column containing the product of density and area (e.g. POP)
   int    m_colStartTarget;            // column containing the initial value of the target variable
   int    m_colTargetCapacity;         // computed capacity (based on allocation formula)
   int    m_colCapDensity;       // capacity expressed in density terms
   int    m_colAvailCapacity;    // difference between computed capacity and current level
   int    m_colAvailDens;        // difference between computed capacity and current level, pre unit area basis
   int    m_colPctAvailCapacity;
   int    m_colPrefs;

   bool   m_estimateParams;

   int m_activeAllocSetID;

   CArray< Constant*, Constant* > m_constArray;

   QueryEngine *m_pQueryEngine;   // memory managed in EnvModel
   //MTParser    *m_pParser;
   Query *m_pQuery;

   int m_inVarStartIndex;    // always one input 
   int m_outVarStartIndex;   // variable number of outputs
   int m_outVarCount; 

   Constant     *AddConstant( LPCTSTR name, LPCTSTR expr );
   TargetReport *AddReport( LPCTSTR name, LPCTSTR query );

   bool SetQuery( LPCTSTR query );
  
   bool Init   ( EnvContext *pEnvContext  );
   bool InitRun( EnvContext *pEnvContext  );
   bool Run    ( EnvContext *pEnvContext );
   bool EndRun ( EnvContext *pEnvContext );

   AllocationSet *GetActiveAllocationSet() { return GetAllocationSetFromID( m_activeAllocSetID ); }
   AllocationSet *GetAllocationSetFromID( int id, int *index=NULL );

public:
   AllocationSetArray m_allocationSetArray; // array of allocation pointers
   TargetReportArray m_reportArray;

   std::vector< float > m_capacityArray;        // holds value of avail capacity for all IDU's (#'s)  - size=#records
   std::vector< float > m_availCapacityArray;   // holds value of total capacity for all IDU's (#'s)  - size=#records
   std::vector< float > m_allocatedArray;       // hold amount newly allocated (current step) for each IDU (#'s) - size=#records
   std::vector< float > m_preferenceArray;      // hold any preferences for each IDU - defaults to 1.0 if no preference applies
   ///std::vector< c float *m_priorPopArray;        // holds value of last seen population numbers (absolute) - size=#records
   //std::vector< double > m_fieldVarsArray;      // hold values of all fields to be included in an IDU
 
protected:
   void PopulateCapacity( EnvContext *pContext, bool useAddDelta );
   float GetCurrentTargetValue( EnvContext *pEnvContext );
   float GetCurrentActualValue( EnvContext *pEnvContext );

public:
   TargetProcess *m_pProcess;
};


// this is a wrapper class for interfacing with Envision's AP extension
class TargetProcess : public EnvModelProcess
{
friend class TargetDlg;

public:
   TargetProcess( void );
  ~TargetProcess( void );

  int m_id;
  int m_outVarsSoFar;

  //QueryEngine *m_pQueryEngine;

   // overrides
   bool Init   ( EnvContext *pContext, LPCTSTR initStr /*xml input file*/ );
   bool InitRun( EnvContext *pContext, bool );
   bool Run    ( EnvContext *pContext );
   bool EndRun ( EnvContext *pContext );

   bool Setup( EnvContext*, HWND );

   bool GetConfig(EnvContext*,std::string &configStr) { configStr = m_configStr; return TRUE; }
   bool SetConfig(EnvContext*, LPCTSTR configStr);
   
   //bool ProcessMap( MapLayer *pLayer, int id ) { return TRUE;}   /**** TODO:  Override if needed *****/
   void LoadFieldVars( const MapLayer *pLayer, int idu );

protected:
   std::string m_configFile;  // includes path
   std::string m_configStr;

   bool LoadXml( LPCTSTR filename, EnvContext*);
   bool SaveXml( LPCTSTR filename, MapLayer *pLayer );

   bool LoadXml(TiXmlElement *pRoot, EnvContext*);

protected:
   PtrArray< Target > m_targetArray;
   int m_colArea;

   bool m_initialized;

   // Notes on naming conventions
   //  total  = summed of IDUs
   //  actual = realized values
   //  target = desired values 
   //  
   //  value  = attribute being cosidered (eg. population)
   //  abs    = absolute (non-density) value 
   //  density = "per unit area" value


   // cumulative over all targets
   float   m_totalActualValue;   // current total value of the target variable (updated during Run()) (non-density)
   float   m_totalTargetValue;

   // is the following redundant with above?
   //float   m_newTotalTarget;   // actual (realized) total value of the target variable at year end (non-density)
   //

   //float   m_lastTotalActualValue;     //  last value of m_curTotalActualValue
   float   m_totalCapacity;            // available capacity for accomodating new values of the target variable (absolute numbers - not density)
   float   m_totalAvailCapacity;       // difference between capacity and current density (absolute number - not density)
   float   m_totalPercentAvailable;         // (avail/total) - decimal percent

public:
   // globally defined constants
   CArray< Constant*, Constant* > m_constArray;
   Constant *AddConstant(LPCTSTR name, LPCTSTR expr);
};


class TargetProcessCollection
{
public:
   PtrArray< TargetProcess > m_targetProcessArray;
   
   TargetProcess *GetTargetProcessFromID( int id, int *index=NULL );

   bool Init   ( EnvContext*, LPCTSTR initStr /*xml input file*/ );
   bool InitRun( EnvContext*, bool );
   bool Run    ( EnvContext* );
   bool EndRun ( EnvContext* );
   int  Setup  ( EnvContext*, HWND );

   bool GetConfig(EnvContext*, std::string &configStr);
   bool SetConfig(EnvContext*, LPCTSTR configStr);

   int  InputVar ( int id, MODEL_VAR** modelVar );
   int  OutputVar( int id, MODEL_VAR** modelVar );

};

