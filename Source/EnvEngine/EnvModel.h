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
//#include  <GsTL\cdf.h>
#ifdef NO_MFC
#include <no_mfc_files/NoMfcFiles.h>
#endif

#include "Delta.h"
#include "Actor.h"
#include "Policy.h"
#include "EnvContext.h"
#include "Scenario.h"
#include "EnvExtension.h"

#include <PtrArray.h>
#include <randgen/Randunif.hpp>
#include <randgen/Randnorm.hpp>
#include <randgen/Randweib.hpp>
#include <randgen/Randln.hpp>
#include <MapExprEngine.h>
#include <QueryEngine.h>
#include <LULCTREE.H>

#include "EnvAPI.h"

/*
#ifdef ENV_IMPORT
#define ENV_EXPORT __declspec( dllimport )
#else
#define ENV_EXPORT  __declspec( dllexport )
#endif
*/

class MapLayer;
class DataManager;
class DeltaArray;
class SocialNetwork;
class Actor;
class Policy;


enum EM_NOTIFYTYPE { EMNT_IDUCHANGE, EMNT_INIT, EMNT_INITRUN, EMNT_STARTSTEP, EMNT_ENDSTEP, EMNT_RUNAP, EMNT_ACTORLOOP, EMNT_RUNEVAL, EMNT_RUNVIZ, EMNT_ENDRUN, EMNT_UPDATEUI, 
   EMNT_SETSCENARIO, EMNT_MULTIRUN };

LPCTSTR GetNotifyTypeStr(EM_NOTIFYTYPE);

// arguments are type, param, extra, (INT_PTR) pModel
typedef void  (*ENVMODEL_NOTIFYPROC)( EM_NOTIFYTYPE, INT_PTR, INT_PTR, INT_PTR );

//extern PolicyManager *gpPolicyManager;

enum RUNSTATUS { RS_PRERUN, RS_PAUSED, RS_RUNNING };  // NOTES:  RS_PRERUN  - Model has been reset but InitRun has not been called
                                                      //         RS_PAUSED  - A model run is not running
                                                      //         RS_RUNNING - A model run is underway

//-------------------------------------------------------------------------------------------------
// DECISION_USE - Flag indicating how each evaluative model is used in actor decision-making
// NOTES:
//   1) A model is defined as a "Metagoal" model if it decision use flag is set (i.e. non-zero)
//      Each policy will contain an Objective matching each metagoal model (plus one additional 
//      "global" objective.
//   2) Each actor will have a Goal corresponding to each model that sets the DU_SELFINTEREST flag
//-------------------------------------------------------------------------------------------------
enum
   {
   DU_SELFINTEREST = 1,  // indicates that the model is used by actors when computing how effective a policy is at
                         // addressing their self interest.  Each actor has a goal for each model that sets this bit

   DU_ALTRUISM = 2,      // indicates that the model is used by actors when computing how effective a policy is at addressing
                         // a particular scarcity.  It is used for the altriustic portion of actor decisionmaking                              

   DU_BOTH = 3           // Note: Metagoals are defined for those models that either of this uses set (i.e. != 0 )
   };                    // POLICIES a effectiveness score for each metagoal


// decision elements - indicatd which elements are actually used in actor decision-making
enum { DE_SELFINTEREST=1, DE_ALTRUISM=2, DE_GLOBALPOLICYPREF=4, DE_UTILITY=8, DE_ASSOCIATIONS=16, DE_SOCIALNETWORK=32 };

// decision making behaviors
enum DECISION_TYPE { DT_PROBABILISTIC=1, DT_MAX=2 };
enum POLICY_TYPE   { PT_MANDATORY=1, PT_NON_MANDATORY=2 };

enum { OVT_MODEL=1, OVT_EVALUATOR=2, OVT_APPVAR=4, OVT_PDATAOBJ=8, OVT_ALL=15 };

// defines any high level goals used in decision-making.  These can represent altruistic, self-interest, or both types
// of decision-making.  Self-interested only requires specification of a name; altruistic requires an associated 
// evaluative model providing landscape feeds (and scarcity) metrics.
// These are defined in the envx file.  These also represent policy intentions
struct METAGOAL
   {
   CString name;
   EnvEvaluator *pEvaluator;   // NULL if not associated with an eval model

   int   decisionUse; // 1 = use in actor self-interest decision, 2 = use in altruism decision, 3=both, 0=neither
   float weight;      // weighting factor for this metagoal (-3 to +3)

   int   useInSensitivity;   // use in sensitivity analysis?
   float saValue;            // perturbation value for sensitivity analysis

   METAGOAL( void ) : name(), pEvaluator( NULL ), decisionUse( 0 ), weight( 0.5f ), useInSensitivity( 0 ), saValue( 0 ) { }
   };


struct POLICY_SCORE 
   { 
   Policy *pPolicy;
   float   score;   //0-1 value indicating how effective a particulalr policy was for a particular site.
   float   cf;      // CFM:  What is this for???

   POLICY_SCORE( ) :  pPolicy( NULL ), score( 0 ), cf( 0 ){ }
   };


class PolicyScoreArray : public CArray< POLICY_SCORE, POLICY_SCORE& >
   {
   };


struct RESET_INFO
   {
   long       initialSeedRandUnif;
   long       initialSeedRandNormal;
   bool       useInitialSeed;
   PolicyArray policyArray;   // copies of initial policies
   ActorArray  actorArray;  // copy of initial actors

   RESET_INFO( void ) : initialSeedRandUnif( 1 ), initialSeedRandNormal( 1 ), useInitialSeed( true ) { }

   ~RESET_INFO( void ) { Clear(); }

   void Clear( void ) { policyArray.RemoveAll(); actorArray.RemoveAll(); }  // clear out everything
   };


struct OUTCOME_STATUS
   {
   OUTCOME_DIRECTIVE_TYPE type;  // see policy.h
   int     col;
   int     targetYear;           // expiration year for expiring outcomes, start year for delayed outcomes
   VData   value;                // value to apply

   OUTCOME_STATUS() :  type( ODT_PERSIST ), col( -1 ), targetYear( -1 ) { }

   OUTCOME_STATUS( const OUTCOME_STATUS &o ) : type( o.type ), col( o.col ), targetYear( o.targetYear ), value( o.value ) { }

   OUTCOME_STATUS& operator = (const OUTCOME_STATUS& o )
      { type = o.type; col = o.col; targetYear = o.targetYear; value = o.value; return *this; }
   
   OUTCOME_STATUS( OUTCOME_DIRECTIVE_TYPE _type, int _col, int _targetYear )
      : type( _type ), col( _col ), targetYear( _targetYear ) { }

   OUTCOME_STATUS( OUTCOME_DIRECTIVE_TYPE _type, int _col, int _targetYear, VData _value )
      : type( _type ), col( _col ), targetYear( _targetYear ), value( _value ) { }
   };


class OutcomeStatusArray : public CArray< OUTCOME_STATUS, OUTCOME_STATUS& > 
   {
   public:
      int AddOutcomeStatus( OUTCOME_DIRECTIVE_TYPE type, int col, int targetYear )
         {
         OUTCOME_STATUS os(type, col, targetYear); return (int)this->Add(os);
         }

      int AddOutcomeStatus( OUTCOME_DIRECTIVE_TYPE type, int col, int targetYear, VData value )
         {
         OUTCOME_STATUS os(type, col, targetYear, value);  return (int)this->Add(os);
         }

   };

// class for storing cell-level OutcomeStatusArrays
class OutcomeStatusMap : public CMap< int, int, OutcomeStatusArray*, OutcomeStatusArray* >
   {
   };


struct EVAL_METRICS
   {
   float scaled;
   float raw;

   EVAL_METRICS() : scaled( 0 ), raw( 0 ) { }
   EVAL_METRICS( const EVAL_METRICS &m ) : scaled( m.scaled ), raw( m.raw ) { };
   EVAL_METRICS &operator = ( const EVAL_METRICS &m ) { scaled = m.scaled; raw = m.raw; return *this; }
   }; 


//--- APP_VAR ------------------------------------------
//
// an Application Variable (AppVar) is a class that contains information
// about application-defined global variables (meaning, they are visible to
// policies and plugins, and are exposed as scenario variables (VAR_INFOs)
//
// APP_VAR's come in several flavors
//  1) Evaluator outputs (defined via pModelVar ) type=AVT_OUTPUT
//  2) eval model scores - type=AVT_SCORE, AVT_RAWSCORE
//  3) application-defined in envx file - type=AVT_APP
//
// APP_VARS are variable values that are available globally through Envision.
// They can store a variety of types in information, stored in a variable 
// of type VDATA.  In some cases, ptrs to other variables are stored.  Specifically:
//
//    AVTYPE    useMapExpr?   VData TYPE
// ------------ -----------  ---------------
//  AVT_OUTPUT      no       TYPE_PTRXXX    pointer to a variable of type defined by the ModelVar
//  AVT_SCORE       no       TYPE_PTRFLOAT  pointer to float containing model score
// AVT_RAWSCORE     no       TYPE_PTRFLOAT  pointer to float containing model raw score
//   AVT_APP        no       TYPE_PTRFLOAT  pointer to float used to store Expression evaluation results
//---------------------------------------------------------
enum AVTYPE { AVT_UNDEFINED, AVT_OUTPUT, AVT_SCORE, AVT_RAWSCORE, AVT_APP };

class ENVAPI AppVar
{
public:
   CString  m_name;
   CString  m_description;
   AVTYPE   m_avType; // AVT_UNDEFINED, AVT_OUTPUT, AVT_SCORE, AVT_RAWSCORE, AVT_APP
   CString  m_expr;
   int      m_timing;    // 1=prerun, 2=postrun, 3=both, default=1;
   int      m_useInSensitivity;
   VData    m_saValue;     // perturbation value used in sensitivity analyses

protected:
   int      m_col;         // if >= 0 , value of appvar is put into IDU layer   
   VData    m_value;

public:
   MapExpr *m_pMapExpr;             // associated MapExpr (NULL if not an evaluated expression)
                                    // (memory managed by this AppVar) 
   EnvProcess *m_pEnvProcess;      // associated process (NULL for value only variables)
   MODEL_VAR *m_pModelVar;          // associate MODEL_VAR (NULL for appVars not associated with a model output)

   AppVar( LPCTSTR name, LPCTSTR descr, LPCTSTR expr ); 
//      : m_name( name ), m_description( descr ), m_expr( expr ), m_timing( 1 ), m_col( -1 ), m_avType( AVT_UNDEFINED ), 
//        m_pMapExpr( NULL ), m_pProcessInfo( NULL ), m_pModelVar( NULL ) { }

   ~AppVar();

   //bool SetValue( void*, TYPE );
   void SetCol( int col ) { m_col = col; }
   int  GetCol( void ) { return m_col; }

   bool SetValue( const VData &v ); 
   bool Evaluate( int idu=-1 ); // updates 'value' based on evaluating 'expr'

   VData &GetValue( void )     { return m_value; }
   bool GetValue( float &val ) { return m_value.GetAsFloat( val ); }
   bool GetValue( int   &val ) { return m_value.GetAsInt( val ); }

   bool IsGlobal( void ) { return ( m_pMapExpr != NULL && m_pMapExpr->IsGlobal() ) ? true : false; }
 };


struct CONSTRAINT_APP_INFO
   {
   CONSTRAINT_APP_INFO( PolicyConstraint *_pConstraint, int _idu, float _area ) : pConstraint( _pConstraint ), idu( _idu ), applicationArea( _area ), periodToDate( 0 ) { }

   PolicyConstraint *pConstraint;
   int   idu;
   float applicationArea;     // area for this application, either idu area or idu+expand area
   int   periodToDate;        // how long has it been in place?
   };


class ConstraintAppInfoList : public CList< CONSTRAINT_APP_INFO*, CONSTRAINT_APP_INFO* >
   {
   public:
      ~ConstraintAppInfoList( void ) { RemoveAll(); }

      void AddConstraint( PolicyConstraint *pConstraint, int idu, float area ) { this->AddTail( new CONSTRAINT_APP_INFO( pConstraint, idu, area ) ); }
      void RemoveAll( void );   
   };

/*
enum PROBE_TYPE { PT_POLICY=1, PT_IDU=2 };  // note: only policy currently supported

class Probe
   {
   friend class EnvModel;

   protected:
      PROBE_TYPE m_type;

   public:
      Policy    *m_pPolicy;           // valid for PT_POLICY, otherwise NULL
      int        m_idu;               // valid for PT_IDU, otherwise -1 (not currently implmemented)

   public:
      //int m_appliedCount;      // PT_POLICY | PT_IDU, number of times policy applied
      //int m_passedCount;      
      //int m_rejectedSiteAttr;
      //int m_rejectedPolicyConstraint;
      //int m_rejectedNoCostInfo;
      //int m_noOutcomeCount;
   
   public:
      Probe( Policy *pPolicy ) : m_type( PT_POLICY ), m_pPolicy ( pPolicy ), m_idu( -1 ) { Reset(); }

      LPCTSTR GetName( void ) { if ( m_pPolicy != NULL ) return m_pPolicy->m_name; return _T( "Probe" ); }

      void Reset( void ) { } // m_appliedCount = m_passedCount = m_rejectedSiteAttr = m_rejectedPolicyConstraint = m_rejectedNoCostInfo = m_noOutcomeCount = 0; }

      PROBE_TYPE GetType( void ) { return m_type; }
   };
*/

class ENVAPI EnvModel
   {
   friend class AppVar;

   public:
      EnvModel(void);
      ~EnvModel(void);

   //protected:
      EnvModel( EnvModel& );

      // Attributes
   public:
      //float m_altruismWt;    // Altruism dial: number in (0,1) that determines 
                                    // how much actors consider scarcity when choosing policies.
      //float m_selfInterestWt;// Self-interest dial: number in (0,1) that determines 
                                    // how much actors consider own values when choosing policies.   
      float m_policyPrefWt;  // policy preference mdial: number in (0,1) that determines 
                                    // how much actors consider policy preferences when choosing policies.
      LulcTree m_lulcTree;   // store lulc heirarchy

      // simulation control
      RUNSTATUS  m_runStatus;
      RESET_INFO m_resetInfo;

      int   m_startYear;         // calendar year defining start of run (default=0)
      int   m_currentYear;       // current calendar year of current simulation (incremented from startYear)
      int   m_endYear;           // calendar year to stop simulation - run will pause(single run) or reset(multi-run) when m_currentYear == m_endYear
      int   m_yearsToRun;        // years for each iteration (0 = monte carlo)
      int   m_iterationsToRun;   // for monte-carlo runs (if greater than 1)
      int   m_currentRun;        // number of the current run (incremented at end of run)(0 means no run, 1 mean one run completed, etc...)
      int   m_startRunNumber;    // default=0;
      int   m_currentMultiRun;
      
      int   m_dynamicUpdate;     // flag indicating whether display should be updated dynamically 0=no update,1=update views,2=update main map, 3=update both, 4=year, 8=process name
      bool  m_showMessages;      // flag indicating whether to show messages to the screen
      //bool  m_showMapUpdates;    // update on-screen updates
      bool  m_inMultiRun;        // flag indicating that current run is a multirun
      bool  m_showRunProgress;
      int   m_collectPolicyData; // 0=dont, 1=do

      bool  m_exportMaps;
      int   m_exportMapInterval; // -1 for no export, positive value for generating map export

      bool  m_exportBmps;
      int   m_exportBmpInterval;  // -1 for no export
      CString m_exportBmpFields;  // empty means current active field, otherwise comma-separated list
      float m_exportBmpSize;      // virtual (map) coords

      bool  m_exportOutputs;
      bool  m_exportDeltas;
      CString m_exportDeltaFields;  // empty means all fields
      bool  m_discardInRunDeltas;
      bool  m_discardMultiRunDeltas;
      CString m_searchPaths;        // comma-separated string or search paths for input files

      int   m_debug;
      int   m_logMsgLevel;       // 0=log everything, 1=log errors, 2=log warnings, 4=log infos, add together as necessary
      float m_areaCutoff;        // minimum area considered during actor decision-making
      int   m_updateActorsFreq;  // update actor groups at run time, <= 0 means don't update
      bool  m_shuffleActorIDUs;  // during policy decision, shuffle order of IDU searched
      bool  m_runParallel;
      bool  m_addReturnsToBudget; 

      int   m_evalFreq;
      bool  m_policyUpdateEnabled;
      int   m_policyUpdateFreq;
      bool  m_actorUpdateEnabled;
      int   m_actorUpdateFreq;

      // decision-making
      int     m_decisionElements;      // DE_SELFINTEREST, DE_ALTRUISM, DE_GLOBALPOLICYPREF, DE_UTILITY, DE_ASSOCIATIONS
      DECISION_TYPE  m_decisionType;   // DT_PROBABILISTIC (default), DT_MAX

      bool m_allowActorIDUChange;
//      bool m_manageMapMemory;         // delete map on delete? 

      INT_PTR m_extra;
      int   m_evalsCompleted;         // flags indicating wich evaluative model results are available
      int   m_consideredCellCount;    // Count of cells that actors have "considered" applying a policy.  Used with m_notifyProc for the for progress bar
      int   m_spatialIndexDistance;

      float m_appliedArea;            // area current policy applies to, used only to track Expand() application area
                                      // reset to zero before each policy application (appies to only a single policy app)

      bool m_areModelsInitialized;


      void SetIDULayer( MapLayer *pLayer );

  // Application Variable stuff
  public:
      MapLayer *m_pIDULayer;

   protected:
      PtrArray< AppVar > m_appVarArray;

   public:
      MapExprEngine *m_pExprEngine;   // for evaluating global and site-based expressions
      QueryEngine   *m_pQueryEngine;

   public:
      MapExpr *AddExpr( LPCTSTR name, LPCTSTR expr );
      bool     RemoveExpr( MapExpr* );
      bool     EvaluateExpr( MapExpr *pMapExpr, bool useQuery, float &result );
      
   public:
      int     GetAppVarCount( int avtypes=0 ); // { return (int) m_appVarArray.GetSize(); }
      AppVar *GetAppVar( int i ) { return m_appVarArray[ i ]; }
      int     AddAppVar( AppVar *pVar, bool useMapExpr );
      AppVar *FindAppVar( LPCTSTR name, int *index=NULL );
      bool    UpdateAppVars( int idu, int applyToCoverage, int timing ); // applyToCoverage - 0=dont, 1= use AddDelta, 2=use SetData; timing: 1=prerun,2=postrun,3=both
      bool    StoreAppVarsToCoverage( int timing, bool useDelta );  // timing: 1=pre, 2=post
      void    RemoveAppVar( int i ) { m_appVarArray.RemoveAt( i ); }
      void    RemoveAllAppVars(); // { m_appVarArray.Clear(); }
      //void    CreateEvaluatorAppVars();    // built-in vars representing exposed model outputs
     
/*
      bool GetAppVarValue( int i, int     &value ) { AppVar *av = m_appVarArray[ i ]; return av->m_value.GetAsInt( value ); }
      bool GetAppVarValue( int i, float   &value ) { AppVar *av = m_appVarArray[ i ]; return av->m_value.GetAsFloat( value ); }
      bool GetAppVarValue( int i, double  &value ) { AppVar *av = m_appVarArray[ i ]; return av->m_value.GetAsDouble( value ); }
      bool GetAppVarValue( int i, CString &value ) { AppVar *av = m_appVarArray[ i ]; return av->m_value.GetAsString( value ); }

      bool GetAppVarValue( AppVar *pVar, int     &value ) { return pVar->m_value.GetAsInt( value ); }
      bool GetAppVarValue( AppVar *pVar, float   &value ) { return pVar->m_value.GetAsFloat( value ); }
      bool GetAppVarValue( AppVar *pVar, double  &value ) { return pVar->m_value.GetAsDouble( value ); }
      bool GetAppVarValue( AppVar *pVar, CString &value ) { return pVar->m_value.GetAsString( value ); }

      bool GetAppVarValue( LPCTSTR name, int     &value ) { AppVar *pVar = FindAppVar( name ); return pVar == NULL ? false : pVar->m_value.GetAsInt( value ); }
      bool GetAppVarValue( LPCTSTR name, float   &value ) { AppVar *pVar = FindAppVar( name ); return pVar == NULL ? false : pVar->m_value.GetAsFloat( value ); }
      bool GetAppVarValue( LPCTSTR name, double  &value ) { AppVar *pVar = FindAppVar( name ); return pVar == NULL ? false : pVar->m_value.GetAsDouble( value ); }
      bool GetAppVarValue( LPCTSTR name, CString &value ) { AppVar *pVar = FindAppVar( name ); return pVar == NULL ? false : pVar->m_value.GetAsString( value ); }
    */      
      
      /*
      int  AddAppVar( LPCTSTR name, LPCTSTR descr, LPCTSTR def, int     value ) { return (int) m_appVarArray.Add( new AppVar( name, descr, def, value ) ); }
      int  AddAppVar( LPCTSTR name, LPCTSTR descr, LPCTSTR def, double  value ) { return (int) m_appVarArray.Add( new AppVar( name, descr, def, value ) ); }
      int  AddAppVar( LPCTSTR name, LPCTSTR descr, LPCTSTR def, LPCTSTR value ) { return (int) m_appVarArray.Add( new AppVar( name, descr, def, value ) ); }
      int  AddAppVar( LPCTSTR name, LPCTSTR descr, LPCTSTR def, bool    value ) { return (int) m_appVarArray.Add( new AppVar( name, descr, def, value ) ); }
      int  AddAppVar( LPCTSTR name, LPCTSTR descr, LPCTSTR def, VData   value ) { return (int) m_appVarArray.Add( new AppVar( name, descr, def, value ) ); }
      */
 
   // active policy constraints
   protected:
      ConstraintAppInfoList m_activeConstraintList;

   public:
      EnvContext   m_envContext;      // exra data associated with this model
      
      PolicyManager   *m_pPolicyManager;
      ScenarioManager *m_pScenarioManager;
      ActorManager    *m_pActorManager;
      
      DataManager *m_pDataManager;    // DataManager associated with this model, or NULL for no data management
      DeltaArray  *m_pDeltaArray;     // DeltaArray associated with this model.  NOTE - this is shared with the 
                                      // dataManager (if the DataManager exists)
      Scenario      *m_pScenario;
      SocialNetwork *m_pSocialNetwork;

      
   protected:
      int       m_scenarioIndex;

      CString   m_constraint;
      Query    *m_pConstraintQuery;

   public:
      bool     *m_targetPolyArray;          // cells to run the model on, (all if NULL or m_targetPolyCount < 0)
      int       m_targetPolyCount;          //

   protected:
      CString   m_postRunConstraint;
      Query    *m_pPostRunConstraintQuery;
 
      float m_actorDecisionRunTime;     // amout of time this process has executed
      float m_dataCollectionRunTime;    // amout of time this process has executed

      PolicyScoreArray m_policyScoreArray;  // used in the ActorLoop

      // Nonstatic Process Info
      CArray< EVAL_METRICS, EVAL_METRICS& > m_landscapeMetrics;  // current (raw and scaled) scores (-3 to +3) reported by eval models
      CArray< INT_PTR, INT_PTR > m_emFirstUnseenDelta;
      CArray< INT_PTR, INT_PTR > m_apFirstUnseenDelta;
      CArray< INT_PTR, INT_PTR > m_vizFirstUnseenDelta;
      OutcomeStatusMap m_outcomeStatusMap;

   public:
      INT_PTR GetFirstUnseenDeltaAP ( int i ) { return m_apFirstUnseenDelta[ i ]; }
      INT_PTR GetFirstUnseenDeltaEM ( int i ) { return m_emFirstUnseenDelta[ i ]; }
      INT_PTR GetFirstUnseenDeltaViz( int i ) { return m_vizFirstUnseenDelta[ i ]; }

      // modeling support
   protected:
      bool SetRunConstraint( void );

/*
   protected:
      PtrArray< Probe > m_probeArray;
      CMap< Policy*, Policy*, Probe*, Probe* > m_probeMap;

   public:
      int    AddProbe( Probe *pProbe ) { int index = (int) m_probeArray.Add( pProbe ); m_probeMap.SetAt( pProbe->m_pPolicy, pProbe ); return index; }
      int    GetProbeCount( void ) { return (int) m_probeArray.GetSize(); }
      Probe *GetProbe( int i ) { return m_probeArray[ i ]; }
      Probe *FindProbe( Policy *pPolicy ) { Probe *pProbe = NULL; m_probeMap.Lookup( pPolicy, pProbe ); return pProbe; } // for ( INT_PTR i=0; i < m_probeArray.GetSize(); i++ ) if ( m_probeArray[ i ]->m_pPolicy == pPolicy ) return m_probeArray[ i ]; return NULL; }
*/
      void   ResetPolicyStats( bool initRun ) { for ( int i=0; i < (int) m_pPolicyManager->GetPolicyCount(); i++ ) m_pPolicyManager->GetPolicy(i)->ResetStats( initRun ); }

   public:
      // main execution entry points
      int   RunMultiple( void );
      int   RunSensitivityAnalysis( void );
      int   Run( int runFlag );
      bool  Reset( void ); 

      bool  m_continueToRunMultiple;  // quick hack to support stopping a multiple run
      
   public:
      Scenario *GetScenario() { return m_pScenario; }
      int   GetScenarioIndex() { return m_scenarioIndex; }
      void  SetScenario( Scenario *pScenario );
      int   GetScenarioNames(std::vector<std::string> &labels);

      void SetConstraint( LPCTSTR constraint );
      void SetPostRunConstraint( LPCTSTR constraint );

      float GetLandscapeMetric( int i ){ return m_landscapeMetrics[ i ].scaled; }
      float GetLandscapeMetricRaw( int i ){ return m_landscapeMetrics[ i ].raw; }
      void  SetNotifyProc( ENVMODEL_NOTIFYPROC proc, INT_PTR extra ) { m_notifyProc = proc; m_extra = extra; }
      bool  CreateDeltaArray( MapLayer *pLayer );
      int   ApplyDeltaArray( MapLayer *pLayer, DeltaArray *pDeltaArray=NULL );  // start defaults to firstUnapplied, end defaults to last delta.  These are []
      int   ApplyDeltaArray( MapLayer *pLayer, DeltaArray *pDeltaArray, INT_PTR start, INT_PTR end );
      int   ApplyDelta( MapLayer *pLayer, DELTA &delta );
      void  POP(const DELTA & delta );
      INT_PTR UnApplyDeltaArray( MapLayer *pLayer, DeltaArray *pDelta=NULL );  // start defaults to GetFirstUnapplied()
      INT_PTR UnApplyDeltaArray( MapLayer *pLayer, DeltaArray *pDeltaArray, INT_PTR start ) const ;  // NULL means use internal delta array;
      INT_PTR AddDelta( int cell, int col, int year, VData newValue, int type ); 
      //void SetFirstUnevaluatedDelta() { m_pDeltaArray->SetFirstUnevaluated(); }
      void  SetTargetPolys( int targetPolys[], int size );
      void  ClearTargetPolys();;
      
      int  SelectPolicyOutcome( MultiOutcomeArray &multiOutcomeArray );
      void ApplyMultiOutcome( const MultiOutcomeInfo &multiOutcome, int cell, int persistence );

      bool IsColBlocked( int cell, int col );      // determines if an outcome col is blocked on the specified cell
      void SetDecisionElements( int decisionElements ) { m_decisionElements = decisionElements; }
      void InitModels();
      bool InitSocialNetwork( void );

      //bool ChangeIDUActor( int idu, Actor *pNewActor );
      int ChangeIDUActor( EnvContext*, int idu, int groupID, bool randomize );      // only valid for AIM_IDU_GROUPS, returns groupID if successful

      //void UpdateUI( int flags, LONG_PTR extra=NULL );  // 1=map, 2=year, 3=process 

   protected:

      void  InitRun();
      void  EndRun();
      void  RunGlobalConstraints();
      void  RunActorLoop();
      void  ApplyScheduledPolicies();
      void  ApplyMandatoryPolicies();
      void  CheckPolicyOutcomes();
      int   CollectRelevantPolicies( int cell, int year, POLICY_TYPE pType, PolicyScoreArray &policyScoreArray );
      bool  DoesPolicyApply( Policy *pPolicy, int cell );
      int   ScoreRelevantPolicies( Actor *pActor, int cell, PolicyScoreArray &policyScoreArray, int relevantPolicyCount );
      int   SelectPolicyToApply( int cell, PolicyScoreArray &policyScoreArray, int relevantPolicyCount );
      void  ApplyPolicy( Policy *pPolicy, int cell, float score );
      
      float GetActorScore( Actor *pActor, int cell, float &cf, float *polObjScores ); // gets score in [0,1]
      float GetAltruismScore( int cell, float *polObjScores ); // gets score in [0,1]
      float GetSocialNetworkScore( Actor *pActor, Policy *pPolicy );   // gets score in [0,1]
      //float GetUtilityScore( int cell );

      void  AddOutcomeStatus( int cell, OUTCOME_DIRECTIVE_TYPE type, int col, int targetYear, VData outcomeData );
      bool  RunEvaluation();	 
      void  RunModelProcesses( bool postYear );
#ifndef NO_MFC
	   bool  RunEvaluationParallel();	 
	   void  RunModelProcessesParallel( bool postYear );
#endif
      void  RunVisualizers( bool postYear );
	   
      int   RunPolicyMetaprocess();
      int   RunCulturalMetaprocess();

   //----------------------------------------------------------------------------------------------------//
   //-------------------------------- shared data/methods across all models -----------------------------//
   //----------------------------------------------------------------------------------------------------//
   protected:
      int m_referenceCount;
      ENVMODEL_NOTIFYPROC m_notifyProc;
      void Notify( EM_NOTIFYTYPE type, INT_PTR param );                 // passe "m_extra" as extra param
      void Notify( EM_NOTIFYTYPE type, INT_PTR param, INT_PTR extra );  // override what gets passed as extra
      
      // metagoals
      PtrArray< METAGOAL > m_metagoalInfoArray;

      // evaluative functions
      //PtrArray< EnvEvaluator   > m_evaluatorArray;
      //PtrArray< EnvModelProcess > m_modelProcessArray;
      //PtrArray< EnvVisualizer   > m_vizInfoArray;
      PtrArray< EnvEvaluator   > m_evaluatorArray;
      PtrArray< EnvModelProcess > m_modelProcessArray;
      PtrArray< EnvVisualizer > m_vizInfoArray;


      // CArray<>s that store indices of m_evaluatorArray that reference the models to be used in each decision processes.
      // For example, the i-th entry in the metagoal map include the index of the EVAL_MODEL that corresponds to that metagoal
      CArray< int, int > m_actorValueToMetagoalMap;   // contains indices of models that address actor self-interest
      CArray< int, int > m_scarcityToEvaluatorMap;               // maps which models address global scarcity (altruism)
      CArray< int, int > m_resultsToEvaluatorMap;                // maps which models are to be shown in results
      CArray< int, int > m_metagoalToEvaluatorMap;               // maps which models are considered as metagoals

      float GetGsFromAbundance( float abundanceScore, float bias );
      float GetGoalNorm( float *pPoint1, float *pPoint2, int dimension ); // normalized distance between two points in the [-3,3]^dimension hypercube

   // Operations
   public:
      bool  GetConfig(EnvExtension *pExtension, std::string &configStr);
      bool  SetConfig(EnvExtension *pExtension, LPCTSTR configStr);

      //int   GetEvaluatorCount() { return (int) m_evaluatorArray.GetSize(); }
      //EnvEvaluator* GetEvaluatorInfo( int i ) { return m_evaluatorArray[ i ]; }
      //EnvEvaluator *FindEvaluatorInfo( LPCTSTR name );
      //int   FindEvaluatorIndex( LPCTSTR name );
      //int   AddEvaluator( EnvEvaluator *mi );
      //void  RemoveEvaluator( int i ) { m_evaluatorArray.RemoveAt( i ); }

      int   GetEvaluatorCount() { return (int) m_evaluatorArray.GetSize(); }
      EnvEvaluator *GetEvaluatorInfo( int i ) { return m_evaluatorArray[ i ]; }
      EnvEvaluator *FindEvaluatorInfo( LPCTSTR name );
      int   FindEvaluatorIndex( LPCTSTR name );
      int   AddEvaluator( EnvEvaluator *mi );
      void  RemoveEvaluator( int i ) { m_evaluatorArray.RemoveAt( i ); }


      // the following methods return mapping information for those models that address actor self interest decision
      int GetMetagoalIndexFromActorValueIndex( int valueIndex ) { return m_actorValueToMetagoalMap[ valueIndex ]; }
      int GetActorValueIndexFromMetagoalIndex( int metagoalIndex );
      int GetActorValueCount( void ) { return (int) m_actorValueToMetagoalMap.GetSize(); }    // returns number of value-mapped models
      METAGOAL *GetActorValueMetagoalInfo( int valueIndex ) { return GetMetagoalInfo( GetMetagoalIndexFromActorValueIndex( valueIndex ) ); }

      // the following models return mapping information for those models that address altriustic decisionmaking based on scarcity
      int GetEvaluatorIndexFromScarcityIndex( int scarcityIndex ) { return m_scarcityToEvaluatorMap[ scarcityIndex ]; }
      int GetScarcityIndexFromEvaluatorIndex( int modelIndex );
      int GetScarcityCount( void ) { return (int) m_scarcityToEvaluatorMap.GetSize(); }        // returns number of scarcity-mapped models
      EnvEvaluator *GetScarcityEvaluatorInfo( int i ) { return GetEvaluatorInfo( GetEvaluatorIndexFromScarcityIndex( i ) ); }

      // the following models return mapping information for those models that are to be use as metagoals.  These are models
      // with decisionUse != 0, and have slots in each policy for effectiveness
      int GetEvaluatorIndexFromMetagoalIndex( int metagoalIndex ) { return m_metagoalToEvaluatorMap[ metagoalIndex ]; }
      int GetMetagoalIndexFromEvaluatorIndex( int modelIndex );
      EnvEvaluator *GetMetagoalEvaluatorInfo( int i ) { return m_metagoalInfoArray[ i ]->pEvaluator; /*GetEvaluatorInfo( GetEvaluatorIndexFromMetagoalIndex( i ) );*/ }

      // the following models return mapping information for those models that are to be use in the results
      int GetEvaluatorIndexFromResultsIndex( int resultsIndex ) { return m_resultsToEvaluatorMap[ resultsIndex ]; }
      int GetResultsIndexFromEvaluatorIndex( int modelIndex );
      int GetResultsCount( void ) { return (int) m_resultsToEvaluatorMap.GetSize(); }
      EnvEvaluator *GetResultsEvaluatorInfo( int i ) { return GetEvaluatorInfo( GetEvaluatorIndexFromResultsIndex( i ) ); }

      // methods related to autonomous processes
      int   GetModelProcessCount() { return (int) m_modelProcessArray.GetSize(); }
      EnvModelProcess* GetModelProcessInfo( int i ) { return m_modelProcessArray[ i ]; }
      EnvModelProcess* FindModelProcessInfo( LPCTSTR name );
      void  AddModelProcess( EnvModelProcess *ai ) { m_modelProcessArray.Add( ai ); }
      int   FindModelProcessIndex( LPCTSTR name );
      void  RemoveModelProcess( int i ) { m_modelProcessArray.RemoveAt( i ); }

      // methods related to visualizers
      int   GetVisualizerCount() { return (int) m_vizInfoArray.GetSize(); }
      EnvVisualizer* GetVisualizerInfo( int i ) { return m_vizInfoArray[ i ]; }
      EnvVisualizer* FindVisualizerInfo( LPCTSTR name );
      void  AddVisualizer( EnvVisualizer *pViz ) { m_vizInfoArray.Add( pViz ); }
      int   FindVisualizerIndex( LPCTSTR name );
      void  RemoveVisualizer( int i ) {  m_vizInfoArray.RemoveAt( i ); }

      // methods related to metagoals - these are policy intentions
      int   GetMetagoalCount() { return (int) m_metagoalInfoArray.GetSize(); }
      METAGOAL* GetMetagoalInfo( int i ) { return m_metagoalInfoArray[ i ]; }
      METAGOAL* FindMetagoalInfo( LPCTSTR name );
      void  AddMetagoal( METAGOAL *pGoal );
      int   FindMetagoalIndex( LPCTSTR name );
      void  RemoveMetagoal( int i );

      // methods related to input/output variables
      int GetInputVarCount ( int flag );              // See OVT_xxxx enums
      int GetOutputVarCount( int flag );              // See OVT_xxxx enums
      MODEL_VAR &GetInputVar ( int flag, int i );     // See OVT_xxxx enums
      MODEL_VAR &GetOutputVar( int flag, int i );     // See OVT_xxxx enums
      MODEL_VAR *FindModelVar(LPCTSTR name, int &flag);

      //int GetOutputVarLabels(int flag, CStringArray &labels);
      // 1=aps, 2=eval models, 3=both, 4=appvars, 7=all
      int GetOutputVarLabels(int flag, std::vector<std::string> &labels);// 1=aps, 2=eval models, 3=both, 4=appvars, 7=all


      // methods related to MetaProcesses (currently deprecated)
      //int GetMetaProcessCount() { return (int) m_mpInfoArray.GetSize(); }
      //EnvModelProcess* GetMetaProcessInfo( int i ) { return m_mpInfoArray[ i ]; }
      //void AddMetaProcess( EnvModelProcess *ai ) { m_mpInfoArray.Add( ai ); }
      int LoadNeighborTable();
      
      void  FreeLibraries();

   public:
      RandUniform     m_randUnif;
      RandNormal      m_randNormal;
      
      // column information
      void  StoreIDUCols();             // doesn't store model cols
      void  StoreModelCols();
      bool  VerifyIDULayer();
      bool  CheckValidFieldNames();

      int   m_colArea;                    // area of a cell polygon
      int   m_colLulcA;
      int   m_colLulcB;
      int   m_colLulcC;
      int   m_colLulcD;
      int   m_colActor;                   // contains index of actor applied (NOT id)
//    int   m_colActorGroup;            // contains id of actor group (not index)
      int   m_colPolicy;                  // contains ID of policy applied
      int   m_colPolicyApps;
      int   m_colScore;
      int   m_colStartLulc;
      int   m_colStartCons;               // starting col for CONSERV
      int   m_colLastDecision;            // year of last decision on this cell
      int   m_colNextDecision;            // year of next possible decision on this cell (-1 = anytime)
      int   m_colUtility;

      CUIntArray m_colActorValues;

      CStringArray m_postRunCmds;
 };


 
// DLL plug-ins call this
extern int  emApplyDelta(EnvModel *pModel,  int cell, int col, int year, VData newValue, int type );

inline
bool AppVar::SetValue( const VData &v )
   {
   m_value = v; 
   if ( m_pMapExpr ) 
      {
      float value;
      if ( v.GetAsFloat( value ) == false )
         return false;

      m_pMapExpr->SetValue( value );
      }

   return true;
   }
   

inline
AppVar *EnvModel::FindAppVar( LPCTSTR name, int *index )
   {
   for ( int i=0; i < m_appVarArray.GetSize(); i++ )
      {
      if ( lstrcmpi( m_appVarArray[ i ]->m_name, name ) == 0 )
         {
         if ( index != NULL )
            *index = i;

         return m_appVarArray[ i ];
         }
      }

   if ( index != NULL )
      *index = -1;

   return NULL;
   }

inline
LPCTSTR  GetNotifyTypeStr(EM_NOTIFYTYPE type)
   {
   switch( type )
      {
      case EMNT_IDUCHANGE:     return "IDU Change"; 
      case EMNT_INIT:          return "Init"; 
      case EMNT_INITRUN:       return "Init Run"; 
      case EMNT_STARTSTEP:     return "Start Step"; 
      case EMNT_ENDSTEP:       return "End Step"; 
      case EMNT_RUNAP:         return "Run AP"; 
      case EMNT_ACTORLOOP:     return "Run Actor Loop"; 
      case EMNT_RUNEVAL:       return "Run Eval"; 
      case EMNT_RUNVIZ:        return "Run Viz"; 
      case EMNT_ENDRUN:        return "End Run"; 
      case EMNT_UPDATEUI:      return "Update UI"; 
      case EMNT_SETSCENARIO:   return "Set Scenario"; 
      case EMNT_MULTIRUN:      return "Multirun"; 
      };
   return "none";
   }