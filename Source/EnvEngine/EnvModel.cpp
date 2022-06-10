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
#include "stdafx.h"
#include "EnvModel.h"

#include "Policy.h"
#include "DataManager.h"
#include "EnvConstants.h"
#include "EnvException.h"
#include "Scenario.h"
#include "SocialNetwork.h"
#include "QueryEngine.h"
#include <FDATAOBJ.H>
#include <Report.h>
#include <Maplayer.h>
#include <MAP.h>
#include <Path.h>
#include <math.h>
#include <algorithm>
#include <PathManager.h>
#include <omp.h>
#include "EnvProcessScheduler.h"

#ifdef _WINDOWS
//#include <Raster.h>
#endif

//#include <omp.h>

//not sure why the cdecl is here. for now, preserve it for windows
#ifndef NO_MFC
int cdecl SortPolicyScores(const void *elem1, const void *elem2 );
#else
int __cdecl SortPolicyScores(const void *elem1, const void *elem2 );
#endif

double NormalizeScores( double );

//ENVMODEL_NOTIFYPROC EnvModel::m_notifyProc = NULL;
//INT_PTR EnvModel::m_extra = 0;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// EnvModel construction/destruction
bool showModelMessages = true;

int EnvEvaluator::m_nextHandle = DT_MODEL;
int EnvModelProcess::m_nextHandle = DT_PROCESS;


// DLL plug-ins call this
INT_PTR emAddDelta(EnvModel *pEnvModel,  int cell, int col, int year, VData newValue, int type )
   {
   return pEnvModel->AddDelta(cell, col, year, newValue, type);
   }

void ProcessWinMsg()
{
  //nop for NonWindows
  #ifndef NO_MFC
	MSG  msg;
	while( PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
#endif
}

//=======================================================================================
inline int EnvEvaluator::ScaleScore(float & arg) const
   {
   enum { OK = 0, OUT_OF_BOUNDS = 1, NOT_NUMBER = -1 };

   //if ( NULL != apNpCdf.get() )
   //   arg = HistogramCDF::getEnvScore(*apNpCdf, arg);
   //else
   //arg = gain*(arg + offset); 

   if  ( 1 != m_showInResults )
      return OK;

   if ( ! ( -3.0f <= arg && arg <= 3.0 ) )
      {
      if ( arg > 3.0f )
         {
         arg = 3.0f;
         return OUT_OF_BOUNDS;
         }
      else if ( arg < -3.0f )
         {
         arg = -3.0f;
         return OUT_OF_BOUNDS;
         }
      else  // arg was NaN
         {
         ASSERT(0);
         arg = 0.0f;  
         return NOT_NUMBER;
         }  
      }
   return OK;
   }

void ConstraintAppInfoList::RemoveAll( void )
   {
   POSITION pos=GetHeadPosition(); 
   while ( pos != NULL ) 
      {
      CONSTRAINT_APP_INFO *pConstraintInfo = GetNext( pos );
      delete pConstraintInfo;
      }
  
   CList< CONSTRAINT_APP_INFO*, CONSTRAINT_APP_INFO*>::RemoveAll();
   }



AppVar::AppVar( LPCTSTR name, LPCTSTR descr, LPCTSTR expr ) 
: m_name( name )
, m_description( descr )
, m_expr( expr )
, m_timing( 1 )
, m_useInSensitivity( 0 )
, m_saValue( 0 )
, m_col( -1 )
, m_avType( AVT_UNDEFINED )
, m_pMapExpr( NULL )
, m_pEnvProcess( NULL )
, m_pModelVar( NULL )
   {
   }


AppVar::~AppVar( void )
   {
   //if ( this->m_pMapExpr != NULL )
      //gpDoc->m_model.m_pExprEngine->RemoveExpr( this->m_pMapExpr );
   }


bool AppVar::Evaluate( int idu /*=-1*/ )
   {
   switch( m_avType )
      {
      case AVT_APP:
         {
         ASSERT( m_pMapExpr != NULL );
         if ( idu < 0  && ! m_pMapExpr->IsGlobal() )
            return false;

         if ( idu >= 0 && m_pMapExpr->IsGlobal() )
            return false;

         //if ( this->m_expr.IsEmpty() )
         //   return true;

         if ( m_timing <= 0 )
            return true;

         bool ok = m_pMapExpr->Evaluate( idu );
         if (ok)
            this->m_value = m_pMapExpr->GetValue();   // TYPE=float
         }
      }

   return true;
   }



EnvModel::EnvModel()
   : m_startYear(0),
   m_currentYear(0),
   m_endYear(25),
   m_yearsToRun(25),
   m_iterationsToRun(20),    // montecarlo
   m_pScenario(NULL),
   m_pSocialNetwork(NULL),
   m_scenarioIndex(-1),  // 500
   m_currentRun(-1),
   m_startRunNumber(0),
   m_currentMultiRun(0),
   m_evalsCompleted(0),
   m_consideredCellCount(0),
   m_spatialIndexDistance(0),
   m_pQueryEngine(NULL),
   m_pExprEngine(NULL),
   m_showMessages(true),
   m_inMultiRun(false),
   m_debug(0),
   m_logMsgLevel(0),        // log everything
   m_areaCutoff(0),
   m_areModelsInitialized(false),
   m_evalFreq(1),
   //m_actorDecisionProbability( 50 ),
   m_envContext(NULL),
   m_pPolicyManager(NULL),
   m_pScenarioManager(NULL),
   m_pActorManager(NULL),
   m_pDataManager(NULL),
   m_pDeltaArray(NULL),
   m_runStatus(RS_PRERUN),
   m_pConstraintQuery(NULL),
   m_targetPolyArray(NULL),
   m_targetPolyCount(-1),
   m_pPostRunConstraintQuery(NULL),
   m_policyUpdateEnabled(false),
   m_policyUpdateFreq(10),
   m_actorUpdateEnabled(true),
   m_actorUpdateFreq(2),
   m_decisionElements(DE_SELFINTEREST | DE_ALTRUISM | DE_GLOBALPOLICYPREF), //, DE_UTILITY, DE_ASSOCIATIONS
   m_decisionType(DT_PROBABILISTIC),    // DT_MAX
   m_allowActorIDUChange(true),
//   m_manageMapMemory(false),
   m_showRunProgress( true ),
   m_dynamicUpdate( 0xFFFF ),
   m_exportMaps( false ),
   m_exportMapInterval( 10 ),
   m_exportBmps( false ),
   m_exportBmpInterval( 10 ),
   m_exportBmpSize( 120 ),
   m_exportOutputs( false ),
   m_exportDeltas( false ),
   m_discardInRunDeltas( false ),
   m_discardMultiRunDeltas( false ),
   m_updateActorsFreq( -1 ),
   m_shuffleActorIDUs( false ),  // during policy decision, shuffle order of IDU searched
   m_runParallel( false ),
   m_addReturnsToBudget( false ),
   m_continueToRunMultiple( false ),

   m_pIDULayer(NULL),
   m_referenceCount(0),
   m_policyPrefWt(0.5f),
   m_colArea(-1),
   m_colLulcA(-1),
   m_colLulcB(-1),
   m_colLulcC(-1),
   m_colLulcD(-1),
   m_colPolicy(-1),
   m_colActor(-1),
   m_colScore(-1),
   m_colStartLulc(-1),
   m_colStartCons(-1),
   m_colLastDecision(-1),
   m_colNextDecision(-1),
   m_colPolicyApps(-1),
   m_colUtility(-1),
   m_colActorValues(),
   m_randUnif( 1, 1 ),              // stream, seed
   m_randNormal( 0, 1, 42 ),        // mean, stddev, seed
   m_evaluatorArray( false ),
   m_modelProcessArray( false ),
   m_vizInfoArray( false ),
   m_metagoalInfoArray( true ),
   m_notifyProc(NULL)
   //int EnvEvaluator::nextHandle = DT_MODEL;
   //int EnvModelProcess::nextHandle = DT_PROCESS;
   {
   m_referenceCount++;

   m_pPolicyManager = new PolicyManager(this);
   m_pScenarioManager = new ScenarioManager(this);
   m_pActorManager = new ActorManager(this);
   m_pDataManager = new DataManager(this);

   m_envContext.ptrAddDelta    = emAddDelta;
   m_envContext.startYear      = m_startYear;
   m_envContext.endYear        = m_endYear;
   m_envContext.yearOfRun      = 0;
   m_envContext.currentYear    = m_startYear;
   m_envContext.run            = -1;
   m_envContext.scenarioIndex  = -1;
   m_envContext.showMessages   = m_showMessages;
   m_envContext.exportMapInterval = m_exportMapInterval;
   m_envContext.pActorManager  = m_pActorManager;
   m_envContext.pPolicyManager = m_pPolicyManager;
   m_envContext.pDeltaArray    = NULL;      // reset in init run
   m_envContext.pLulcTree      = &m_lulcTree;
   m_envContext.pExprEngine    = NULL;   // fill this in later after created
   m_envContext.pScenario      = NULL;   // fill this in later after created
   m_envContext.score          = 0;
   m_envContext.rawScore       = 0;
   m_envContext.pDataObj       = NULL;
   m_envContext.pEnvExtension = NULL;
#ifndef NO_MFC
   m_envContext.pWnd           = NULL;
#endif
   m_envContext.pEnvModel      = this;
   m_envContext.col            = -1;
   m_envContext.id             = -1;
   m_envContext.handle         = -1;
   m_envContext.logMsgLevel    = m_logMsgLevel > 0 ? m_logMsgLevel : 0xFFFF;

   m_resetInfo.initialSeedRandNormal = 42;
   m_resetInfo.initialSeedRandUnif   = 1;
   m_resetInfo.useInitialSeed = true;
   m_randNormal.SetSeed( m_resetInfo.initialSeedRandNormal );
   m_randUnif.SetSeed( m_resetInfo.initialSeedRandUnif );
   }


   EnvModel::EnvModel(EnvModel &model)
      : m_yearsToRun(model.m_yearsToRun),
      m_currentYear(model.m_currentYear),
      m_startYear(model.m_startYear),
      m_endYear(model.m_endYear),
      m_iterationsToRun(model.m_iterationsToRun),
      m_pScenario(model.m_pScenario),
      m_scenarioIndex(model.m_scenarioIndex),
      m_currentRun(-1),
      m_startRunNumber(0),
      m_currentMultiRun(0),
      m_evalsCompleted(0),
      m_consideredCellCount(0),
      m_spatialIndexDistance(model.m_spatialIndexDistance),
      m_pQueryEngine(NULL),
      m_pExprEngine(NULL),
      //m_pQueryEngine( NULL ),
      m_showMessages(model.m_showMessages),
      m_dynamicUpdate(model.m_dynamicUpdate),
      m_inMultiRun(model.m_inMultiRun),
      m_debug(model.m_debug),
      m_logMsgLevel(model.m_logMsgLevel),
      //m_dynamicUpdate( model.m_dynamicUpdate ),
      m_evalFreq(model.m_evalFreq),
      m_updateActorsFreq(model.m_updateActorsFreq),
      //m_actorDecisionProbability( model.m_actorDecisionProbability ),
      m_envContext(NULL),
      m_pPolicyManager(NULL),
      m_pScenarioManager(NULL),
      m_pActorManager(NULL),
      m_pDataManager(NULL),
      m_pDeltaArray(NULL),
      m_runStatus(RS_PRERUN),
      m_pConstraintQuery(NULL),
      m_targetPolyArray(NULL),
      m_targetPolyCount(-1),
      m_pPostRunConstraintQuery(NULL),
      m_policyUpdateEnabled(model.m_policyUpdateEnabled),
      m_policyUpdateFreq(model.m_policyUpdateFreq),
      m_actorUpdateEnabled(model.m_actorUpdateEnabled),
      m_actorUpdateFreq(model.m_actorUpdateFreq),
      m_decisionElements(model.m_decisionElements),
      m_decisionType(model.m_decisionType),
      m_allowActorIDUChange(model.m_allowActorIDUChange),
//      m_manageMapMemory(model.m_manageMapMemory),
      m_showRunProgress(model.m_showRunProgress),
      m_exportMaps(model.m_exportMaps),
      m_exportMapInterval(model.m_exportMapInterval),
      m_exportBmps(model.m_exportBmps),
      m_exportBmpInterval(model.m_exportBmpInterval),
      m_exportBmpSize(model.m_exportBmpSize),
      m_exportOutputs(model.m_exportOutputs),
      m_exportDeltas(model.m_exportDeltas),
      m_discardMultiRunDeltas(model.m_discardMultiRunDeltas),
      m_runParallel(model.m_runParallel),
      m_addReturnsToBudget(model.m_addReturnsToBudget),
      m_continueToRunMultiple(model.m_continueToRunMultiple),

      m_pIDULayer(model.m_pIDULayer),
      m_referenceCount(0),
      m_policyPrefWt(model.m_policyPrefWt),
      m_colArea(model.m_colArea),
      m_colLulcA(model.m_colLulcA),
      m_colLulcB(model.m_colLulcB),
      m_colLulcC(model.m_colLulcC),
      m_colLulcD(model.m_colLulcD),
      m_colPolicy(model.m_colPolicy),
      m_colActor(model.m_colActor),
      m_colScore(model.m_colScore),
      m_colStartLulc(model.m_colStartLulc),
      m_colStartCons(model.m_colStartCons),
      m_colLastDecision(model.m_colLastDecision),
      m_colNextDecision(model.m_colNextDecision),
      m_colPolicyApps(model.m_colPolicyApps),
      m_colUtility(model.m_colUtility),
      //m_colActorValues(model.m_colActorValues),
      m_randUnif(1, 1),              // stream, seed
      m_randNormal(0, 1, 42),        // mean, stddev, seed
      m_evaluatorArray(model.m_evaluatorArray),
      m_modelProcessArray(model.m_modelProcessArray),
      m_vizInfoArray(model.m_vizInfoArray),
      m_metagoalInfoArray(model.m_metagoalInfoArray)
   {
   m_referenceCount++;

   m_pPolicyManager = new PolicyManager(this);
   m_pScenarioManager = new ScenarioManager(this);
   m_pActorManager = new ActorManager(this);
   m_pDataManager = new DataManager(this);
   
   m_envContext.ptrAddDelta   = emAddDelta;
   m_envContext.startYear     = m_startYear;
   m_envContext.endYear       = m_endYear;
   m_envContext.currentYear   = m_startYear;
   m_envContext.yearOfRun     = 0;
   m_envContext.run           = -1;
   m_envContext.showMessages  = m_showMessages;
   m_envContext.pActorManager = m_pActorManager;
   m_envContext.pPolicyManager= m_pPolicyManager;
   m_envContext.pDeltaArray   = NULL;      // reset in init run
   m_envContext.pLulcTree     = &m_lulcTree;
   m_envContext.pQueryEngine  = m_pQueryEngine;
   m_envContext.pExprEngine   = NULL;
   m_envContext.pScenario     = NULL;
   m_envContext.score         = 0;
   m_envContext.rawScore      = 0;
   m_envContext.pDataObj      = NULL;
   m_envContext.pEnvModel     = this;
   m_envContext.pMapLayer     = m_pIDULayer;
   m_envContext.col           = -1;
   m_envContext.pEnvExtension  = NULL;
#ifndef NO_MFC
   m_envContext.pWnd          = NULL;
#endif
   m_envContext.id            = -1;
   m_envContext.handle        = -1;
   m_envContext.logMsgLevel   = m_logMsgLevel > 0 ? m_logMsgLevel : 0xFFFF;

   m_resetInfo.initialSeedRandNormal = 42;
   m_resetInfo.initialSeedRandUnif = 1;
   m_resetInfo.useInitialSeed = false;

   //m_randNormal.SetSeed( m_resetInfo.initialSeedRandNormal );
   //m_randUnif.SetSeed( m_resetInfo.initialSeedRandUnif );

   int count = (int) model.m_landscapeMetrics.GetCount();
   m_landscapeMetrics.SetSize( count, 0 );

   for ( int i=0; i<count; i++ )
      m_landscapeMetrics[i] = model.m_landscapeMetrics[i];

   count = (int) model.m_emFirstUnseenDelta.GetCount();
   m_emFirstUnseenDelta.SetSize( count, 0 );

   for ( int i=0; i<count; i++ )
      m_emFirstUnseenDelta[i] = model.m_emFirstUnseenDelta[i];

   count = (int) model.m_apFirstUnseenDelta.GetCount();
   m_apFirstUnseenDelta.SetSize( count, 0 );

   for ( int i=0; i<count; i++ )
      m_apFirstUnseenDelta[i] = model.m_apFirstUnseenDelta[i];

   count = (int) model.m_vizFirstUnseenDelta.GetCount();
   m_vizFirstUnseenDelta.SetSize( count, 0 );

   for ( int i=0; i<count; i++ )
      m_vizFirstUnseenDelta[i] = model.m_vizFirstUnseenDelta[i];

   if ( model.m_pSocialNetwork != NULL )
      m_pSocialNetwork = new SocialNetwork( *model.m_pSocialNetwork );
   else
      m_pSocialNetwork = NULL;

   //m_pExprEngine = NULL;     // new MapExprEngine( *(model.m_pExprEngine) );
   }

// note - doesn't delete the map
EnvModel::~EnvModel()
   {
   m_referenceCount--;

   // clear out outcomeStatusMap
   POSITION pos = m_outcomeStatusMap.GetStartPosition();
   while( pos != NULL )
      {
      OutcomeStatusArray *pOutcomeStatusArray = NULL;
      int cell;
      m_outcomeStatusMap.GetNextAssoc( pos, cell, pOutcomeStatusArray );
      ASSERT( pOutcomeStatusArray != NULL );
      delete pOutcomeStatusArray;
      }

   m_outcomeStatusMap.RemoveAll();

   RemoveAllAppVars();

   if (m_referenceCount == 0)
      FreeLibraries();  // ?????? note: this must happen after delete m_pDataManager!!

   if ( m_pSocialNetwork )
      delete m_pSocialNetwork;

   if ( m_pExprEngine )
      delete m_pExprEngine;

   if ( m_pQueryEngine != NULL )
      {
      delete m_pQueryEngine;
      m_pQueryEngine = NULL;
      m_pPolicyManager->m_pQueryEngine = NULL;
      }

   if (m_pPolicyManager)
      delete m_pPolicyManager;

   if (m_pScenarioManager)
      delete m_pScenarioManager;

   if (m_pActorManager)
      delete m_pActorManager;

   if (m_pDataManager)
      delete m_pDataManager;
   else if (m_pDeltaArray)  // if there is no dataManager, need to deallocate memory
      delete m_pDeltaArray;


   //if (m_referenceCount == 0)
   //   FreeLibraries();  // note: this must happen after delete m_pDataManager!!
   ClearTargetPolys();

//   if (m_manageMapMemory)
//      {
//      Map *pMap = this->m_pIDULayer->GetMapPtr();
//      delete pMap;
//      this->m_pIDULayer = NULL;
//      }
   }


int EnvModel::AddEvaluator( EnvEvaluator *mi ) 
   {
   int index = (int) m_evaluatorArray.Add( mi );

   if ( mi->m_use && mi->m_showInResults ) 
      m_resultsToEvaluatorMap.Add( index );
   
   return index;
   }


void EnvModel::AddMetagoal( METAGOAL *pGoal )
   {
   int metagoalIndex = (int) m_metagoalInfoArray.Add( pGoal ); 

   if ( pGoal->decisionUse & DU_SELFINTEREST ) 
      m_actorValueToMetagoalMap.Add( metagoalIndex );

   if ( pGoal->pEvaluator == NULL )
      m_metagoalToEvaluatorMap.Add( -1 );
   else
      {
      int modelIndex = EnvModel::FindEvaluatorIndex( pGoal->pEvaluator->m_name );
      m_metagoalToEvaluatorMap.Add( modelIndex );

      ASSERT( pGoal->decisionUse & DU_ALTRUISM );
      m_scarcityToEvaluatorMap.Add( modelIndex );
      }
   }


void EnvModel::RemoveMetagoal( int i )
   {
   m_metagoalInfoArray.RemoveAt( i );
   m_metagoalToEvaluatorMap.RemoveAt( i );
   }


METAGOAL *EnvModel::FindMetagoalInfo( LPCTSTR name )
   {
   for ( int i=0; i < EnvModel::GetMetagoalCount(); i++ )
      {
      if ( m_metagoalInfoArray[ i ]->name.CompareNoCase( name ) == 0 )
         return m_metagoalInfoArray[ i ];
      }

   return NULL;
   }

int EnvModel::FindMetagoalIndex( LPCTSTR name )
   {
   for ( int i=0; i < EnvModel::GetMetagoalCount(); i++ )
      {
      if ( m_metagoalInfoArray[ i ]->name.CompareNoCase( name ) == 0 )
         return i;
      }

   return -1;
   }


void EnvModel::FreeLibraries()
   {
   for (int i = 0; i < m_modelProcessArray.GetSize(); i++)
      {
      EnvModelProcess *pModel = m_modelProcessArray[i];
      HINSTANCE hDLL = pModel->m_hDLL;
      delete pModel;
      FREE_LIBRARY(hDLL);
      }

   for ( int i=0; i < m_evaluatorArray.GetSize(); i++ )
      {
      EnvEvaluator *pEval = m_evaluatorArray[i];
      HINSTANCE hDLL = pEval->m_hDLL;
      delete pEval;
      FREE_LIBRARY(hDLL);
      }

   for ( int i=0; i < m_vizInfoArray.GetSize(); i++ )
      {
      EnvVisualizer *pViz = m_vizInfoArray[i];
      HINSTANCE hDLL = pViz->m_hDLL;
      delete pViz;
      FREE_LIBRARY(hDLL);
      }
   }


void EnvModel::SetScenario( Scenario *pScenario )
   {
   Notify( EMNT_SETSCENARIO, (INT_PTR) pScenario );

   if ( pScenario == NULL )
      m_scenarioIndex = -1;
   else
      m_scenarioIndex = m_pScenarioManager->GetScenarioIndex( pScenario );

   m_pScenario = pScenario;

   ASSERT( m_scenarioIndex >= -1 );
   }


int EnvModel::GetScenarioNames(std::vector<std::string> &names)
   {
   for (int i = 0; i < m_pScenarioManager->GetCount(); i++)
      {
      std::string name(m_pScenarioManager->GetScenario(i)->m_name);
      names.push_back(name);
      }

   return (int) names.size();
}


void EnvModel::SetConstraint( LPCTSTR constraint )
   {
   if ( constraint == NULL )
      m_constraint.Empty();
   else
      m_constraint = constraint;
   }


void EnvModel::SetPostRunConstraint( LPCTSTR constraint )
   {
   if ( constraint == NULL )
      m_constraint.Empty();
   else
      m_constraint = constraint;
   }


bool EnvModel::SetRunConstraint( void )
   {
   // set constraints if any
   if ( m_constraint.GetLength() > 0 )
      {
      if ( m_constraint.Compare( "0" ) == 0 )  // use current selection
         {
         int queryCount = m_pIDULayer->GetSelectionCount();

         if ( queryCount == 0 )
            {
            ClearTargetPolys();

#ifndef NO_MFC
            int result = AfxMessageBox( "You have specified that you want to run a scenario on a selected area, but the selection set is empty.  Do you want to run on the entire coverage?", MB_YESNO );

            return ( result == IDYES  ? true : false );
#else
	    return true;
#endif
            }

         if ( m_targetPolyArray == NULL )
            SetTargetPolys( NULL, 0 );  // create 
         else
            {
            ASSERT( m_targetPolyCount == m_pIDULayer->GetRecordCount( MapLayer::ALL ) );
            memset( m_targetPolyArray, 0, m_targetPolyCount*sizeof( bool ) );
            }

         for ( int i=0; i < queryCount; i++ )
            {
            int poly = m_pIDULayer->GetSelection( i );
            m_targetPolyArray[ poly ] = true;
            }
         }
      else  // use previously defined constraint
         {
         m_pConstraintQuery = m_pQueryEngine->ParseQuery( m_constraint, 0, "Run Constraint" );

         if ( m_pConstraintQuery == NULL )
            Report::ErrorMsg( "Error parsing constraint query - this constraint will not be applied" );
         else
            {
            if ( m_targetPolyArray == NULL )
               SetTargetPolys( NULL, 0 );  // create 

            ASSERT( m_targetPolyCount == m_pIDULayer->GetRecordCount( MapLayer::ALL ) );
            bool result;
            for ( int i=0; i < m_targetPolyCount; i++ )
               {
               bool ok = m_pConstraintQuery->Run( i, result );
               m_targetPolyArray[ i ] = ( ok && result );
               } 
            }
         }
      }
   else
      ClearTargetPolys();

   return true;
   }



/*
int EnvModel::GetActorValueIndexFromModelIndex( int modelIndex )
   {
   EnvEvaluator *pModel = GetEvaluatorInfo( modelIndex );
   int valueIndex = 0;

   for ( int g=0; g < GetMetagoalCount(); g++ )
      {
      METAGOAL *pGoal = GetMetagoalInfo( g );

      if ( pGoal->decisionUse & DU_SELFINTEREST )
         {
         if ( pGoal->pEvaluator == pModel )
            return valueIndex;

         valueIndex++;
         }
      }

   return -1;
   }
   */


int EnvModel::GetActorValueIndexFromMetagoalIndex( int metagoalIndex )
   {
   int valueIndex = 0;
   if ( metagoalIndex >= GetMetagoalCount() )
      return -1;

   for ( int g=0; g <= metagoalIndex; g++ )
      {
      METAGOAL *pGoal = GetMetagoalInfo( g );

      if ( pGoal->decisionUse & DU_SELFINTEREST )
         {
         if ( g == metagoalIndex )
            return valueIndex;

         valueIndex++;
         }
      }

   return -1;
   }



int EnvModel::GetScarcityIndexFromEvaluatorIndex( int modelIndex )
   {
   EnvEvaluator *pModel = GetEvaluatorInfo( modelIndex );
   int scarcityIndex = 0;

   for ( int g=0; g < GetMetagoalCount(); g++ )
      {
      METAGOAL *pGoal = GetMetagoalInfo( g );

      if ( pGoal->decisionUse & DU_ALTRUISM )
         {
         if ( pGoal->pEvaluator == pModel )
            return scarcityIndex;

         scarcityIndex++;
         }
      }

   return -1;
   }


int EnvModel::GetMetagoalIndexFromEvaluatorIndex( int modelIndex )
   {
   /*
   int metagoalCount = -1;
   for ( int i=0; i <= modelIndex; i++ )
      {
      if ( GetEvaluatorInfo(i)->decisionUse & DU_ALTRUISM || GetEvaluatorInfo(i)->decisionUse & DU_SELFINTEREST )
         metagoalCount++;
      }

   return metagoalCount; */
   EnvEvaluator *pModel = GetEvaluatorInfo( modelIndex );

   for ( int i=0; i < GetMetagoalCount(); i++ )
      {
      METAGOAL *pInfo = GetMetagoalInfo( i );

      if ( pInfo->pEvaluator == pModel )
         return i;
      }

   return -1;
   }



int EnvModel::GetResultsIndexFromEvaluatorIndex( int modelIndex )
   {
   int resultsCount = -1;
   for ( int i=0; i <= modelIndex; i++ )
      {
      if ( GetEvaluatorInfo(i)->m_showInResults )
         resultsCount++;
      }

   return resultsCount;
   }


bool EnvModel::GetConfig(EnvExtension *pExtension, std::string &configStr)
   {
   m_envContext.pEnvExtension = pExtension;
   m_envContext.id = pExtension->m_id;

   bool ok = pExtension->GetConfig(configStr);
   return ok ? true : false;
   }


bool  EnvModel::SetConfig(EnvExtension *pExtension, LPCTSTR configStr)
   {
   m_envContext.pEnvExtension = pExtension;
   m_envContext.id = pExtension->m_id;

   bool ok = pExtension->SetConfig(&m_envContext, configStr);
   return ok ? true : false;
   }

int EnvModel::FindModelProcessIndex( LPCTSTR name )
   {
   int count = GetModelProcessCount();

   for ( int i=0; i < count; i++ )
      {
      EnvModelProcess *pInfo = GetModelProcessInfo( i );
      if ( pInfo->m_name.CompareNoCase( name ) == 0 )
         return i;
      }

   return -1;
   }

EnvModelProcess* EnvModel::FindModelProcessInfo( LPCTSTR name )
   {
   int index = FindModelProcessIndex( name );

   if ( index < 0 )
      return NULL;
   else
      return GetModelProcessInfo( index );
   }


int EnvModel::FindEvaluatorIndex( LPCTSTR name )
   {
   int count = GetEvaluatorCount();

   for ( int i=0; i < count; i++ )
      {
      EnvEvaluator *pInfo = GetEvaluatorInfo( i );
      if ( pInfo->m_name.CompareNoCase( name ) == 0 )
         return i;
      }

   return -1;
   }

EnvEvaluator* EnvModel::FindEvaluatorInfo( LPCTSTR name )
   {
   int index = FindEvaluatorIndex( name );

   if ( index < 0 )
      return NULL;
   else
      return  GetEvaluatorInfo( index );
   }


int EnvModel::GetInputVarCount( int flag )
   {
   int count = 0;

   // flag:  1=aps, 2=eval models, 3=both
   if ( flag & 1 )
      {
      for ( int i=0; i < GetModelProcessCount(); i++ )
         {
         EnvModelProcess *pInfo = GetModelProcessInfo( i );

         if ( pInfo->m_use )
            {
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pInfo->InputVar( pInfo->m_id, &modelVarArray );
            count += varCount;
            }
         }
      }

   if ( flag & 2 )
      {
      for ( int i=0; i < GetEvaluatorCount(); i++ )
         {
         EnvEvaluator *pInfo = GetEvaluatorInfo( i );

         if ( pInfo->m_use )
            {
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pInfo->InputVar( pInfo->m_id, &modelVarArray );
            count += varCount;
            }
         }
      }

   return count;
   }


int EnvModel::GetOutputVarCount( int flag )
   {
   int count = 0;

   // flag:  1=aps, 2=eval models, 3=both
   if ( flag & OVT_MODEL )
      {
      for ( int i=0; i < GetModelProcessCount(); i++ )
         {
         EnvModelProcess *pInfo = GetModelProcessInfo( i );

         if ( pInfo->m_use)
            {
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pInfo->OutputVar( pInfo->m_id, &modelVarArray );

            for (int j = 0; j < varCount; j++)
               {
               //TRACE("%s\n", modelVarArray[j].name);

               if (modelVarArray[j].pVar != NULL)
                  {
                  // exclude data objs?
                  if ((flag & OVT_PDATAOBJ) || (modelVarArray[j].type != TYPE_PDATAOBJ))
                     count++;
                  }
               }
            }
         }
      }

   if ( flag & OVT_EVALUATOR )
      {
      for ( int i=0; i < GetEvaluatorCount(); i++ )
         {
         EnvEvaluator *pInfo = GetEvaluatorInfo( i );

         if ( pInfo->m_use )
            {
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pInfo->OutputVar( pInfo->m_id, &modelVarArray );

            for (int j = 0; j < varCount; j++)
               {
               //TRACE("%s\n", modelVarArray[j].name);

               if (modelVarArray[j].pVar != NULL)
                  {
                  // exclude data objs?
                  if ((flag & OVT_PDATAOBJ) || (modelVarArray[j].type != TYPE_PDATAOBJ))
                     count++;
                  }
               }
            }
         }
      }
   
   if (flag & OVT_APPVAR)
      {
      int totalAppVarCount = GetAppVarCount();
      for (int i = 0; i < totalAppVarCount; i++)
         {
         AppVar *pVar = GetAppVar(i);
         if (pVar->m_avType == AVT_APP && pVar->IsGlobal())
            count++;
         }
      }

   return count;
   }


MODEL_VAR *EnvModel::FindModelVar(LPCTSTR name, int &flag)
   {
   int count = 0;

   // flag:  1=aps, 2=eval models, 3=both
   if (flag & OVT_MODEL)
      {
      for (int i = 0; i < GetModelProcessCount(); i++)
         {
         EnvModelProcess *pInfo = GetModelProcessInfo(i);

         if (pInfo->m_use)
            {
            MODEL_VAR *pVar = pInfo->FindOutputVar(name);;

            if (pVar != nullptr)
               {
               flag = OVT_MODEL;
               return pVar;
               }
            }
         }
      }

   if (flag & OVT_EVALUATOR)
      {
      for (int i = 0; i < GetEvaluatorCount(); i++)
         {
         EnvEvaluator *pInfo = GetEvaluatorInfo(i);

         if (pInfo->m_use)
            {
            MODEL_VAR *pVar = pInfo->FindOutputVar(name);;

            if (pVar != nullptr)
               {
               flag = OVT_EVALUATOR;
               return pVar;
               }
            }
         }
      }

   return nullptr;
   }


int EnvModel::GetOutputVarLabels(int flag, std::vector<std::string> &labels)
   {
   // flag:  1=aps, 2=eval models, 3=both
   if (flag & OVT_MODEL)
      {
      for (int i = 0; i < GetModelProcessCount(); i++)
         {
         EnvModelProcess *pInfo = GetModelProcessInfo(i);

         if (pInfo->m_use )
            {
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR &mv = modelVarArray[j];
               if (mv.pVar != NULL)
                  {
                  if (mv.type == TYPE_PDATAOBJ)
                     {
                     // exclude data objs?
                     if (flag & OVT_PDATAOBJ) 
                        {
                        // get individual columns from the dataobj
                        DataObj *pDataObj = (DataObj*) mv.pVar;

                        int cols = pDataObj->GetColCount();
                        for (int col = 1; col < cols; col++)  // first column is time, so ignore it
                           {
                           std::string label(pInfo->m_name);
                           label += ".";
                           label += mv.name;
                           label += ":";
                           label += pDataObj->GetLabel(col);
                           labels.push_back(label);
                           }
                        }
                     }
                  else // not a data obj
                     {
                     std::string label(pInfo->m_name);
                     label += ".";
                     label += mv.name;
                     labels.push_back(label);
                     }
                  }
               }
            }
         }
      }

   if (flag & OVT_EVALUATOR)
      {
      for (int i = 0; i < GetEvaluatorCount(); i++)
         {
         EnvEvaluator *pInfo = GetEvaluatorInfo(i);

         if (pInfo->m_use )
            {
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR &mv = modelVarArray[j];
               if (mv.pVar != NULL)
                  {
                  // exclude data objs?
                  if ((flag & OVT_PDATAOBJ) || (mv.type != TYPE_PDATAOBJ))
                     {
                     std::string label(pInfo->m_name);
                     label += ".";
                     label += mv.name;
                     labels.push_back(label);
                     }
                  }
               }
            }
         }
      }

   if (flag & OVT_APPVAR)
      {
      int totalAppVarCount = GetAppVarCount();
      for (int i = 0; i < totalAppVarCount; i++)
         {
         AppVar *pVar = GetAppVar(i);
         if (pVar->m_avType == AVT_APP && pVar->IsGlobal())
            labels.push_back(std::string((LPCTSTR) pVar->m_name));
         }
      }

   return (int) labels.size();
   }


//////////////////////////////////////////////////////////////////////////
//
// E N V I S I O N   M O D E L   R O U T I N E S 
//
//////////////////////////////////////////////////////////////////////////

int EnvModel::RunMultiple()
   {
   //if ( m_pScenario )
   //   gpView->m_outputTab.m_scenarioTab.SaveScenario(); //  m_pScenario->Initialize();    // reset random number generator  ???? do we really need to do this?

   m_inMultiRun = true;

   bool showMessages = m_showMessages;
   m_showMessages = false;
   m_envContext.showMessages = false;

   bool useInitialSeed = m_resetInfo.useInitialSeed;
   m_resetInfo.useInitialSeed = false;

   if ( m_pDataManager )
      m_pDataManager->CreateMultiRunDataObjects();

   m_continueToRunMultiple = true;

   for ( int i=0; i < m_iterationsToRun; i++ )
      {
      Run( 1 );   // 1=randomize scenario variables.  This increments m_currentRun

      if ( m_pDataManager )
         m_pDataManager->CollectMultiRunData( i );

      Reset();

      if ( !m_continueToRunMultiple )
         break;

      Notify( EMNT_MULTIRUN, 1, i+1 );
      }

   m_currentMultiRun++;

   m_inMultiRun = false;
   m_showMessages = m_envContext.showMessages = showMessages;
   m_resetInfo.useInitialSeed = useInitialSeed;

   return m_iterationsToRun;
   }

//------------------------------------------------------------------------
// EnvModel::Run( int runFlag )
//
// -- runs the Envsion model
//
//  Basic idea: 
//      for each year....
//          compute evaluative stats

//          for each actor
//             for each of the actor's cells
//                collect relevant policies
//                make a decision about which policie(s) to apply
//                apply the policy
//             next cell
//          next actor
//       next year
//
// input:  int runFlags:  0 = set scenario variables, no randomization (uses paramLocation)
//                        1 = set scenario variable, using randomization if variable is defined as random
//                       -1 = set scenario variable EXCEPT any whose "useInSensitivity" flags are 
//                            set to -1, no randomization
//------------------------------------------------------------------------

int EnvModel::Run( int runFlag )
   {
   TRACE( "Entered EnvModel::Run()\n" );
   
   // set new output path
   CString iduPath = PathManager::GetPath( PM_IDU_DIR );
   CString scenarioName( m_pScenario->m_name );

   // don't allow blanks or slashes in name
   CleanFileName( scenarioName );   // don't allow blanks or slashes in name

   CString exportPath;    // {IduDir}\Outputs\{ScenarioName}\Run{X} during the course of a run
   exportPath.Format( "%sOutputs\\%s\\Run%i\\", (LPCTSTR) iduPath, (LPCTSTR) scenarioName, m_pScenario->m_runCount );

   PathManager::SetPath( PM_OUTPUT_DIR, exportPath );    // {IduDir}\Outputs\{ScenarioName} during the course of a run

   // make sure directory exists
#ifndef NO_MFC
   SHCreateDirectoryEx( NULL, exportPath, NULL );
#else
         mkdir(exportPath,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
   
   CString logFile( exportPath );   //  {IduDir}\Outputs\{ScenarioName}\ during the course of a run

   logFile += "Envision.log";
   Report::OpenFile( logFile );

   CString msg;
   msg.Format(  "Starting scenario: %s (Run %i)", (LPCTSTR) m_pScenario->m_name, this->m_currentRun + this->m_startRunNumber + 1 );
   Report::Log( "---------------------------------------------------------------" );
   Report::Log( msg );
   Report::Log( "---------------------------------------------------------------" );
   
   if ( m_pExprEngine )
      m_pExprEngine->SetCurrentTime( (float) this->m_currentYear );

   QueryEngine::SetCurrentTime(this->m_currentYear);

   // set scenario
   ASSERT( m_pScenario != NULL );
   m_envContext.pScenario = m_pScenario;
   m_pScenario->SetScenarioVars( runFlag );

   bool ok = SetRunConstraint();  // if any, populate m_targetPolyArray
   if ( ! ok )
      return 0;

   m_pIDULayer->m_readOnly = false;

   InitRun();     // sets m_resetInfo.doReset = false, set it to true here by default. 
                  // This also sets m_currentRun to the current run 

   m_envContext.yearOfRun = 0;
   m_currentYear = m_startYear;

   RUN_INFO &ri = m_pDataManager->GetRunInfo( m_currentRun );
   ri.pScenario = m_pScenario;  // let the data manager know which scenario is running.

   // for status bar...
   int cellCount = m_pActorManager->GetManagedCellCount();
   if ( cellCount == 0 )
      cellCount = 1;
     
   m_pIDULayer->m_readOnly = true;

   // always export map at beginning of run 
   if ( m_exportMaps )
      m_pDataManager->ExportRunMap( exportPath );

   if ( m_exportBmps &&  this->m_exportBmpSize > 0 )
      m_pDataManager->ExportRunBmps( exportPath );

   m_runStatus = RS_RUNNING;

   msg.Format( "Year: %i", this->m_currentYear );
   Report::StatusMsg( msg );
   
   try
      {
      while ( m_currentYear < m_endYear )  // m_currentYear is initialized in Reset
         { 
         // check for new messages
#ifndef NO_MFC
         ProcessWinMsg();

         // was the simulation paused?
         if ( m_runStatus == RS_PAUSED )
            {
            break;
            }

         TRACE1( "EnvModel:: Starting year %d\n", m_currentYear );
#endif
         m_envContext.currentYear = m_currentYear;
         m_envContext.yearOfRun   = m_currentYear-m_startYear;

         //UpdateUI( 4 );  // year
         Notify(EMNT_UPDATEUI, 4 );
         Notify(EMNT_STARTSTEP, m_envContext.yearOfRun, m_endYear-m_startYear);

         ResetPolicyStats( false );

         QueryEngine::SetCurrentTime(this->m_currentYear);
         if (m_pExprEngine)
            m_pExprEngine->SetCurrentTime((float) this->m_currentYear);

         // update global AppVars (of type AVT_APP, using pre- timing flag)
         UpdateAppVars( -1, 0, 1 );
         StoreAppVarsToCoverage( 1, true );  // for any idu-based variables, update cell coverage
         ApplyDeltaArray( m_pIDULayer );     // apply any generated deltas
         
         // Run any input visualizers
         //Report::StatusMsg( "Running Input Visualizers (pre)..." );
         //RunVisualizers( false );  // these only run post
         
         // Run the Model Processes (pre)
         ProcessWinMsg();

         Report::StatusMsg( "Running Models(Pre)..." );
         RunModelProcesses( false );
         Report::StatusMsg( "Running Models (Pre)...Completed" );

         // expire or start any relevant policy outcomes based on there outcome status
         CheckPolicyOutcomes();

         // update maintenance costs toward global constraints
         RunGlobalConstraints();   // initializes for this year, applies maintenance costs

         // Apply any scheduled policies
         ApplyScheduledPolicies();

         // Apply any mandatory policies
         ApplyMandatoryPolicies();
         
         // update social network if needed
         if ( m_pSocialNetwork )
            {
            Report::StatusMsg( "Running Social Network" );
            m_pSocialNetwork->Run();
            }

         // iterate through the actors, applying policies
         Report::StatusMsg( "Running Actor Decisionmaking" );
         //UpdateUI( 8, (LONG_PTR) 0 );
         Notify( EMNT_UPDATEUI, 8, (INT_PTR) 0 );

         RunActorLoop();            // updates field-based AppVars
         Report::StatusMsg( "Running Actor Decisionmaking...Completed" );

         ProcessWinMsg();

         // Run the Autonomous Processes (post)
         Report::StatusMsg( "Running Models (Post)..." );
         RunModelProcesses( true );
         Report::StatusMsg( "Running Models (Post)...Completed" );

         ProcessWinMsg();

         // time to do an evaluation?
         int yearOfRun = m_currentYear - m_startYear;
         if ( ( m_evalFreq > 0 ) && ( yearOfRun % m_evalFreq ) == 0 )
            {
            Report::StatusMsg( "Running Evaluators" );
            // generate landscape metrics to guide policy, cultural metaprocesses
            RunEvaluation();
            Report::StatusMsg( "Running Evaluators...Completed" );
            }

         // post update of app vars
         UpdateAppVars( -1, 0, 2 );
         StoreAppVarsToCoverage( 2, true );  // for any idu-based variables, update cell coverage
         ApplyDeltaArray( m_pIDULayer );     // apply any generated deltas

         Report::StatusMsg( "Running Visualizers (post)..." );
         RunVisualizers( true );
         
         // time for generate/retract policies?
         if ( m_policyUpdateEnabled && ( m_policyUpdateFreq > 0 ) && ( yearOfRun % m_policyUpdateFreq ) == 0 )
            RunPolicyMetaprocess();  // run the 

         if ( m_actorUpdateEnabled && ( m_actorUpdateFreq > 0 ) && ( yearOfRun % m_actorUpdateFreq ) == 0 )
            RunCulturalMetaprocess();

         // map export required?         
         if ( ( m_exportMaps && m_exportMapInterval > 0 ) && (yearOfRun % m_exportMapInterval ) == 0 && yearOfRun != 0  && yearOfRun != this->m_yearsToRun-1)
            m_pDataManager->ExportRunMap( exportPath ); //yearOfRun );

         if ( ( m_exportBmps && m_exportBmpInterval > 0 ) && (yearOfRun % m_exportBmpInterval ) == 0 )   // && yearOfRun != 0 )
               m_pDataManager->ExportRunBmps( exportPath );

         // beginning of next year
         m_currentYear++;
         yearOfRun++; // = m_currentYear - m_startYear;

         ASSERT( m_pDataManager );
         clock_t start = clock();
         Report::StatusMsg( "Collecting Data..." );
         m_pDataManager->CollectData( yearOfRun );  // 0-based
         Report::StatusMsg( "Collecting Data...Completed" );
         clock_t finish = clock();
         double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
         m_dataCollectionRunTime += (float) duration;   

         // update various items
         //UpdateUI( 2 );   // map display
         Notify( EMNT_UPDATEUI, 2 );

         if ( m_discardInRunDeltas )
            {
            this->m_pDeltaArray->ClearDeltas();

            int modelCount = GetEvaluatorCount();
            for ( int i=0; i < modelCount; i++ )
               m_emFirstUnseenDelta[i] = 0;

            int apCount = GetModelProcessCount();
            for ( int i=0; i < apCount; i++ )
               m_apFirstUnseenDelta[i] = 0;
            }

         if (m_showRunProgress)
            Notify(EMNT_ENDSTEP, yearOfRun, m_endYear - m_startYear );
         }  // end of:  while ( m_currentYear < m_endYear )

      // done with simulations, let processes know we are done
      EndRun();      // sends EMNT_ENDRUN Notificiations, updates run number

      // Note:  At this point, we have either 
      //        paused a run (m_runStatus==RS_PAUSED) or completed a run (RS_RUNNING) 
      }
   catch ( EnvRuntimeException *pEx )
      {
      CString msg;
      msg.Format( "Run %d was aborted: \n  %s\n", m_currentRun + m_startRunNumber, pEx->what() );

      if ( m_pDataManager )
         {
         // resize the data objects
         m_pDataManager->SetDataSize( m_currentYear-m_startYear+1 );  // + 1 for zero based index
         // dump data collect in this run to disk
         //CString fileName;
         //fileName.Format( "EnvDataDump-Run-%d.edd", m_currentRun );
         //m_pDataManager->SaveRun( fileName );  
         //msg.AppendFormat( "Data collected for this run has been dumped to %s", fileName );
         }

      if ( m_showMessages )
         Report::ErrorMsg( msg );
      else
         Report::InfoMsg( msg );

      pEx->Delete();
      }
   
   if ( m_exportMaps )
      {
      Report::StatusMsg( "Exporting Map at end of run..." );
      m_pDataManager->ExportRunMap( exportPath );
      }

   if ( m_exportBmps )
      {
      Report::StatusMsg( "Exporting Map BMPs at end of run..." );
      m_pDataManager->ExportRunBmps( exportPath );
      }

   if ( m_exportOutputs )
      {
      Report::StatusMsg( "Exporting Model Outputs..." );
      m_pDataManager->ExportRunData( exportPath );
      }

   if ( m_exportDeltas )
      {
      Report::StatusMsg( "Exporting Delta Array..." );
      m_pDataManager->ExportRunDeltaArray( exportPath, -1, -1, -1, false, true, m_exportDeltaFields );
      }

   // run any postrun commandlines
   for ( int i=0; i < (int) m_postRunCmds.GetSize(); i++ )
      {
      system( (LPCTSTR) m_postRunCmds[ i ]  );
      }

   //TRACE( "Leaving EnvModel::Run()\n" );

   CString time;
   time.Format( _T("-Actor Decisionmmaking: %g secs" ), m_actorDecisionRunTime );
   Report::Log( time );

   for ( int i=0; i < m_modelProcessArray.GetCount(); i++ )
      {
      EnvModelProcess *pInfo = m_modelProcessArray.GetAt( i );

      if ( pInfo->m_use )
         {
         time.Format( _T("--%s: %g secs (Auto Process)" ), (LPCTSTR) pInfo->m_name, pInfo->m_runTime );
         time.Replace( _T("%"), _T("percent") );
         Report::Log( time );
         }
      }

   for ( int i=0; i < this->m_evaluatorArray.GetCount(); i++ )
      {
      EnvEvaluator *pInfo = m_evaluatorArray.GetAt( i );

      if ( pInfo->m_use )
         {
         time.Format( _T("--%s: %g secs (Eval Model)" ), (LPCTSTR) pInfo->m_name, pInfo->m_runTime );
         time.Replace( _T("%"), _T("percent") );
         Report::Log( time );
         }
      }
   
   m_runStatus = RS_PRERUN;   // return to pre run status in anticipation of the next run
   
   time.Format( _T("Data Collection: %g secs" ), m_dataCollectionRunTime );
   Report::Log( time );
   Report::CloseFile();
   //gpMain->AddMemoryLine();

//   if ( this->m_discardMultiRunDeltas )
//      m_pDataManager->DiscardDeltaArray( this->m_currentRun );

   Report::StatusMsg( "Run completed..." );
   
   return 1;
   }


void EnvModel::EndRun( void )
   {
   int modelCount = GetEvaluatorCount();
   for ( int i=0; i < modelCount; i++ )
      {
      EnvEvaluator *pInfo = m_evaluatorArray[ i ];
      if ( pInfo->m_use )
         {
         // call the function (eval models should return scores in the range of -3 to +3)

         Notify( EMNT_UPDATEUI, 16, (INT_PTR) (LPCTSTR) pInfo->m_name ); 
         //if ( m_showRunProgress )
         //   gpMain->SetModelMsg( pInfo->m_name );

         m_envContext.pEnvExtension  = pInfo;
         //m_envContext.col    = pInfo->col;
         m_envContext.id     = pInfo->m_id;
         m_envContext.handle = pInfo->m_handle;
         m_envContext.score  = 0.0f;  // CFM - This should not be necessary. models should overwrite
         m_envContext.rawScore = 0.0f;
         m_envContext.pDataObj = NULL;
         m_envContext.firstUnseenDelta = m_emFirstUnseenDelta[i];
         m_envContext.lastUnseenDelta = m_envContext.pDeltaArray ? m_envContext.pDeltaArray->GetSize() : -1;

         clock_t start = clock();

         bool ok = pInfo->EndRun( &m_envContext );

         clock_t finish = clock();
         double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
         pInfo->m_runTime += (float) duration;         

         if ( m_showRunProgress )
            {
            CString msg( pInfo->m_name );
            msg += " - Completed";

            Notify( EMNT_UPDATEUI, 16, (INT_PTR)(LPCTSTR) msg );
            //gpMain->SetModelMsg( msg );
            }

         if ( ! ok )
            {
            CString msg = "The ";
            msg += pInfo->m_name;
            msg += " Evaluative Model returned FALSE during EndRun(), indicating an error.";
            throw new EnvRuntimeException( msg );
            }
         }
      }  // end of:  for( i < modelCount )

   int apCount = GetModelProcessCount();
   for ( int i=0; i < apCount; i++ )
      {
      EnvModelProcess *pInfo = m_modelProcessArray[ i ];
      if ( pInfo->m_use )
         {
         // call the function (eval models should return scores in the range of -3 to +3)
         Notify( EMNT_UPDATEUI, 16, (INT_PTR)(LPCTSTR) pInfo->m_name );
         //   gpMain->SetModelMsg( pInfo->m_name );

         m_envContext.pEnvExtension  = pInfo;
         //m_envContext.col    = pInfo->col;
         m_envContext.id     = pInfo->m_id;
         m_envContext.handle = pInfo->m_handle;
         m_envContext.score  = 0.0f;  // CFM - This should not be necessary. models should overwrite
         m_envContext.rawScore = 0.0f;
         m_envContext.pDataObj = NULL;
         m_envContext.firstUnseenDelta = m_apFirstUnseenDelta[i];
         m_envContext.lastUnseenDelta = m_envContext.pDeltaArray->GetSize();

         clock_t start = clock();

         bool ok = pInfo->EndRun( &m_envContext );

         clock_t finish = clock();
         double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
         pInfo->m_runTime += (float) duration;         

         if ( m_showRunProgress )
            {
            CString msg( pInfo->m_name );
            msg += " - Completed";
            Notify( EMNT_UPDATEUI, 16, (INT_PTR)(LPCTSTR) msg );
            //gpMain->SetModelMsg( msg );
            }

         if ( ! ok )
            {
            CString msg = "The ";
            msg += pInfo->m_name;
            msg += " Evaluative Model returned FALSE during EndRun(), indicating an error.";
            throw new EnvRuntimeException( msg );
            }
         }
      }  // end of:  for( i < modelCount )

   m_pScenario->m_runCount++;
   
   m_pDataManager->EndRun( m_discardInRunDeltas );       // manage end of run for dataobjs

   Notify(EMNT_ENDRUN, m_currentRun, m_endYear - m_startYear);

   if ( m_discardInRunDeltas )
      {
      int modelCount = GetEvaluatorCount();
      for ( int i=0; i < modelCount; i++ )
         m_emFirstUnseenDelta[i] = 0;

      int apCount = GetModelProcessCount();
      for ( int i=0; i < apCount; i++ )
         m_apFirstUnseenDelta[i] = 0;
      }

   return;
   }


void EnvModel::ApplyMandatoryPolicies()
   {
   UINT iduCount = m_pIDULayer->GetRecordCount();   // active records by default
   PolicyScoreArray policyScoreArray;
   policyScoreArray.SetSize( m_pPolicyManager->GetPolicyCount() );

   if ( m_pPolicyManager->GetMandatoryPolicyCount() == 0 )
      return;

   // randomize policy order

   UINT *idus = new UINT[iduCount];
   for (UINT i = 0; i < iduCount; i++)
      idus[i] = i;

   ShuffleArray< UINT >(idus, iduCount, &m_randUnif); 

   //for ( MapLayer::Iterator idu = m_pIDULayer->Begin(); idu != m_pIDULayer->End(); idu++ )
   for (UINT i=0; i < iduCount; i++ )
      {
      UINT idu = idus[i];
      UpdateAppVars( idu, 0, 3 );  // perhaps second arg should be 1?  as is, only updates AppVar, not coverage

      // collect relevant policies for this IDU.  Policies that do not pass any associated constraints will be excluded 
      int mandatoryCount = CollectRelevantPolicies( idu, m_currentYear, PT_MANDATORY, policyScoreArray );  // should apply constraints

      for (int i = 0; i < mandatoryCount; i++)
         {
         Policy *pPolicy = policyScoreArray[i].pPolicy;

         float rn = (float)m_randUnif.RandValue(0, 1);

         if (rn <= pPolicy->m_compliance)
            {
            ApplyPolicy(pPolicy, idu, 0);   // apply mandatory policy, including updating constraint values
            }
         }
      }

   delete[] idus;

   ApplyDeltaArray( m_pIDULayer ); // in case a scheduled policy subdivides want to prevent another acting on the 'defunct' idu. can live with invalidated iterator.
   }    



void EnvModel::ApplyScheduledPolicies()
   {
   int cellCount = m_pIDULayer->GetRecordCount();   // active records by default

   for ( int i=0; i < m_pPolicyManager->GetPolicyScheduleCount(); i++ )
      {
      POLICY_SCHEDULE &ps = m_pPolicyManager->GetPolicySchedule( i );

      if ( ps.year == m_currentYear )
         {
         // get all cell where this policy applies
         for ( MapLayer::Iterator cell = m_pIDULayer->Begin(); cell != m_pIDULayer->End(); cell++ )
            {
            if ( DoesPolicyApply( ps.pPolicy, cell ) )
               {
               ApplyPolicy( ps.pPolicy, cell, 0 );   // apply scheduled policy
               ApplyDeltaArray( m_pIDULayer ); // in case a scheduled policy subdivides want to prevent another acting on the 'defunct' idu. can live with invalidated iterator.
               }
            }
         }
      }
   }


void EnvModel::CheckPolicyOutcomes()
   {
   // expire any policy outcomes that need to be expired, and apply any policy outcomes that were delayed until now
   POSITION pos = m_outcomeStatusMap.GetStartPosition();

   OutcomeStatusArray *pArray = NULL;
   int cell;
   while ( pos != NULL )
      {
      m_outcomeStatusMap.GetNextAssoc( pos, cell, pArray );

      if ( pArray != NULL )
         {
         for ( int i=0; i < pArray->GetSize(); i++ )
            {
            OUTCOME_STATUS &os = pArray->GetAt( i );

            if ( os.targetYear == m_currentYear )  // target time reached for this status event?
               {            
               switch( os.type )
                  {
                  // for persistent and expiring outcomes, check to see if any target value exists.  If so, apply it.
               case ODT_PERSIST:
               case ODT_EXPIRE:
                  if ( ! os.value.IsNull() )
                     AddDelta( cell, os.col, m_currentYear, os.value, DT_POLICY );//dont care if fail/success
                  break;

                  // same thing for delays, at least for now.
               case ODT_DELAY:
                  if ( ! os.value.IsNull() )
                     AddDelta( cell, os.col, m_currentYear, os.value, DT_POLICY );//dont care if fail/success
                  break;
                  }
               }
            }
         }
      }
   }


// run once at the beginning of a Run() step.
// initializes, applies maintenance costs
void EnvModel::RunGlobalConstraints( void )  
   {
   // first, set global constraints to zero for this step
   for ( int i=0; i < m_pPolicyManager->GetGlobalConstraintCount(); i++ )
      {
      GlobalConstraint *pConstraint = m_pPolicyManager->GetGlobalConstraint( i );
      pConstraint->Reset( );  // increments cumulative, resets current to 0
      }
   
   // next, apply maintenance costs to constraints
   if ( m_activeConstraintList.GetCount() > 0 )
      {
      POSITION pos = this->m_activeConstraintList.GetHeadPosition();
      POSITION currentPos;

      while( pos != NULL )
         {
         currentPos = pos;
         CONSTRAINT_APP_INFO *pInfo = m_activeConstraintList.GetNext( pos );
         PolicyConstraint *pConstraint = pInfo->pConstraint;

         // update global
         GlobalConstraint *pGC = pConstraint->m_pGlobalConstraint;
         ASSERT( pGC != NULL );

         // maintenance - subtract any costs from this constraint to the global constraint pool
         pGC->ApplyPolicyConstraint( pConstraint, pInfo->idu, pInfo->applicationArea, false );
         
         // time to expire this one?
         pInfo->periodToDate++;

         int duration = 0;
         pConstraint->GetDuration(pInfo->idu, duration);

         if ( pInfo->periodToDate >= duration )
            {
            delete pInfo;
            pInfo = NULL;

            m_activeConstraintList.RemoveAt( currentPos );
            }
         }
      }  // end of: process active constraints 
   }


void EnvModel::RunActorLoop()
   {
   clock_t start = clock();

   int policyCount = m_pPolicyManager->GetPolicyCount();
   int consideredPolicyCount = 0;
   int appliedPolicyCount = 0;
   int consideredCellCount = 0;
   //int relevantPolicyCount = 0;
   
   PolicyScoreArray tmpPolicyScoreArray;
   tmpPolicyScoreArray.SetSize( policyCount, 0 );

   // reset the internal application count for each policy (cumulative count not affected)
   //m_pPolicyManager->ResetPolicyAppliedCount();

   Notify( EMNT_ACTORLOOP, 0 );

   if ( m_shuffleActorIDUs )
      m_pActorManager->ShuffleActors();

   int actorCount = m_pActorManager->GetActorCount();   // this ensures any actor added this step are not invoked

   int numThreads = omp_get_num_threads();
   omp_set_num_threads( numThreads );

//#pragma omp parallel for shared( consideredPolicyCount, appliedPolicyCount, consideredCellCount)
   for ( int actor=0; actor < actorCount; actor++ )
      {
      Actor *pActor = m_pActorManager->GetActor( actor );
      //Notify( EMNT_ACTORLOOP, 1, actor );

      int cellCount = pActor->GetPolyCount();

      if ( m_shuffleActorIDUs )
         pActor->ShufflePolyIDs();

      // iterate though all the cells managed by this actor
      for ( int i=0; i < cellCount; i++ )
         {
         int cell = pActor->GetPoly( i );

         if ( m_targetPolyArray && m_targetPolyArray[ cell ] == false )
            continue;

         if ( m_areaCutoff > 0 )
            {
            float area;
            m_pIDULayer->GetData( cell, m_colArea, area );
            if ( area < m_areaCutoff )
               continue;
            }

         // is it time to consider a decision on this cell?  (this will be set if the policy is exclusive )
         int nextDecision;
         m_pIDULayer->GetData( cell, m_colNextDecision, nextDecision );

         if ( m_currentYear >= nextDecision )
            {
            // So, this cell is open for decisionmaking (not locked up by an exclusive policy.
            // We next check to see if the actor will make a decision.  This is probablistically determined
            // by generating a random number between 0-pActor->m_decisionFreq, and assuming decisions are uniformly
            // distributed in this range
            double randVal = m_randUnif.RandValue( 0, pActor->GetDecisionFrequency() );
            if ( randVal < 1 )
               {               // the actor has decided to make a decision.  Proceed in selecting a policy.
               UpdateAppVars( cell, 0, 3 );  // perhaps second arg should be 1?  as is, only updates AppVar, not coverage
               
               int relevantPolicyCount = CollectRelevantPolicies( cell, m_currentYear, PT_NON_MANDATORY, tmpPolicyScoreArray ); 
               
               if ( relevantPolicyCount > 0 )
                  {
                  // score using compensatory attributes
                  ScoreRelevantPolicies( pActor, cell, tmpPolicyScoreArray, relevantPolicyCount );

                  // apply the policy to the cell being examined
                  int index = SelectPolicyToApply( cell, tmpPolicyScoreArray, relevantPolicyCount );

                  if ( index >= 0 )
                     {
                     Policy *pPolicy = tmpPolicyScoreArray[ index ].pPolicy;
                     float   score   = tmpPolicyScoreArray[ index ].score;

                //     #pragma omp critical
                        {
                        ASSERT( pPolicy->m_use  );
                        ApplyPolicy( pPolicy, cell, score );
                        ApplyDeltaArray( m_pIDULayer );  // used to be below...
                        }
                        
                     // have the actor remember the policy it applied
                     pActor->m_policyHistoryArray.Add( pPolicy );
               //      #pragma omp atomic
                     appliedPolicyCount++;
                     }

            //      #pragma omp atomic
                  consideredPolicyCount += relevantPolicyCount;
                  }  // if ( relevantPolicyCount > 0 )
               }  // if ( randVal < 1 ) (avtor makes a decision
       //     #pragma omp atomic
            consideredCellCount++;
            }  // end of:  if ( m_currentYear >= nextDecision )
         }  // end of:  for ( i < cellCount );
   //   #pragma omp atomic
      m_consideredCellCount += cellCount;
      }  // end of:  for ( actor < actorCount )

   Notify( EMNT_ACTORLOOP, 2 );

   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
   m_actorDecisionRunTime += (float) duration;         

   //TCHAR buffer[ 128 ];
   //sprintf_s( buffer, 128, _T("Actors-> Time: %i, Cells considered: %i, Policies considered: %i, Policies applied: %i\n"), 
   //   m_currentYear, consideredCellCount, consideredPolicyCount, appliedPolicyCount ); 
   //TRACE( buffer );

   return;
   }


// returns the number of collected policies
int EnvModel::CollectRelevantPolicies( int cell, int year, POLICY_TYPE policyType, PolicyScoreArray &policyScoreArray )
   {
   int policyCount = m_pPolicyManager->GetPolicyCount();
   int relevantPolicyCount = 0;

   //   Policy::SetParserVars( m_pIDULayer, cell );

   for ( int i=0; i < policyCount; i++ )
      {
      Policy *pPolicy = m_pPolicyManager->GetPolicy( i );

      // check various constraints
      if ( pPolicy->m_use == false )
         continue;
      
      if ( pPolicy->m_isScheduled )
         continue;

      if ( pPolicy->m_startDate >= 0 && year < pPolicy->m_startDate )   // prior to start date?
         continue;

      if ( pPolicy->m_endDate >= 0 && year > pPolicy->m_endDate )  // after end date?
         continue;

      if ( pPolicy->m_mandatory && policyType == PT_NON_MANDATORY )
         continue;

      if ( ! pPolicy->m_mandatory && policyType == PT_MANDATORY )
         continue;

      // cehcks agains global constraints
      if ( DoesPolicyApply( pPolicy, cell ) )
         policyScoreArray[ relevantPolicyCount++ ].pPolicy = pPolicy;
      }  // end of:  for ( i < policyCount );

   // if there are any mandatory policies, reduce set to just these.  This will exclude
   // any non-mandatory policies from being applied 
   /*
   int mandatoryPolicyCount = 0; 
   for ( int i=0; i < relevantPolicyCount; i++ )
      {
      Policy *pPolicy = policyScoreArray[i].pPolicy;
      if ( pPolicy->m_mandatory )
         policyScoreArray[ mandatoryPolicyCount++ ].pPolicy = pPolicy;
      }

   if ( mandatoryPolicyCount > 0 )
      return mandatoryPolicyCount;
   else*/
      return relevantPolicyCount;
   }


//-- EnvModel::DoesPolicyApply() --------------------------------------------------
//
// determines is the policy is applicable to the site
//--------------------------------------------------------------------------------------

bool EnvModel::DoesPolicyApply( Policy *pPolicy, int cell )
   {
   // site characteristics
   if ( pPolicy->m_pSiteAttrQuery == NULL )
      return false;

   // check any constraints.  Note in the case of multiple policy constraints
   // if ANY of the constraints are overallocated, the policy is rejected
   int constraintCount = pPolicy->GetConstraintCount();
   int colRepairCost = this->m_pIDULayer->GetFieldCol("repaircost");
   int colRepairYr = this->m_pIDULayer->GetFieldCol("repair_yrs");

   for ( int i=0; i < constraintCount; i++ )
      {
      PolicyConstraint *pConstraint = pPolicy->GetConstraint( i );
      GlobalConstraint *pGC = pConstraint->m_pGlobalConstraint;

      // skip any invalid ones
      if ( pGC == NULL )
         continue;

      // global constraint already overallocated?  then reject
      float availCapacity = pGC->AvailableCapacity();
      if ( availCapacity <= 0 )
         {
         pPolicy->m_rejectedPolicyConstraint++;
         return false;
         }
      
      // budget exists in global constraint, so get the new cost increment
      // of qualifying this policy for further consideration

      // idu area, or max expand area if an outcome include Expand()
      float area = pPolicy->GetMaxApplicationArea( cell ); 

      switch( pGC->m_type )
         {
         case CT_RESOURCE_LIMIT:
            {
            float costIncrement = 0;
            bool found = pConstraint->GetInitialCost( cell, area, costIncrement );

            // Get mainenance info
            float repairCost = 0, repairYrs = 0;
            this->m_pIDULayer->GetData(cell, colRepairCost, repairCost);
            this->m_pIDULayer->GetData(cell, colRepairYr, repairYrs);

            ///CString msg;
            ///msg.Format("Cost: %f, Repair Cost: %f, Repair Yrs: %i, remaining budget: %.1f\n", 
            ///   costIncrement, repairCost, repairYrs, pGC->AvailableCapacity() );
            ///TRACE(msg);

            if ( ! found )
               {
               pPolicy->m_rejectedNoCostInfo++;
               return false;
               }

            bool overAllocated = pGC->DoesConstraintApply( costIncrement );

            if ( overAllocated )
               {
               pPolicy->m_rejectedPolicyConstraint++;
               return false;
               }
            }
            break;

         case CT_MAX_AREA:
            {
            if ( pGC->AvailableCapacity() < area )
               {
               pPolicy->m_rejectedPolicyConstraint++;
               return false;
               }
            }
            break;

         case CT_MAX_COUNT:
            {
            if ( pGC->AvailableCapacity() <= 0 )
               {
               pPolicy->m_rejectedPolicyConstraint++;
               return false;
               }
            }
            break;
         }
      }  // end of: for each constraint

   // passed global constraints, check site attribute constraints
   bool passQuery;
   bool ok = pPolicy->m_pSiteAttrQuery->Run( cell, passQuery );
   ASSERT( ok );
   if ( passQuery == false )
      {
      pPolicy->m_rejectedSiteAttr++;
      return false;
      }

   pPolicy->m_passedCount++;

   return true;
   }

int EnvModel::ScoreRelevantPolicies( Actor *pActor, int cell, PolicyScoreArray &policyScoreArray, int relevantPolicyCount )
   {
   // basic idea:  A score is generated based on:
   //    1) actor score, reflecting self-interested decision-making,
   //    2) an altruistic score, based on landscape feedbacks, and
   //    3) a policy preference score, based on any stated policy preference
   //    4) utility function (if defined)
   //    5) a social network score (if defined)
   // these are then merged into a single multicriteria score based on weighting

   if ( pActor == NULL )
      {
      ENV_ASSERT( 0 );
      throw new EnvRuntimeException( "Invalid Parameters in EnvModel::ScoreRelevantPolicies(.)" );
      }

   static float tolerance = 0.1f;  // actors will not consider policies with combined scores less then this

   float altruismWt      = m_decisionElements & DE_ALTRUISM         ? pActor->GetAltruismWt()      : 0;
   float selfInterestWt  = m_decisionElements & DE_SELFINTEREST     ? pActor->GetSelfInterestWt()  : 0;
   float utilityWt       = m_decisionElements & DE_UTILITY          ? pActor->GetUtilityWt()       : 0;
   float policyPrefWt    = m_decisionElements & DE_GLOBALPOLICYPREF ? this->m_policyPrefWt         : 0;
   float socialNetworkWt = m_decisionElements & DE_SOCIALNETWORK    ? pActor->GetSocialNetworkWt() : 0;

   bool ok = ::NormalizeWeights( altruismWt, selfInterestWt, utilityWt, policyPrefWt, socialNetworkWt );
   if ( ! ok )
      return 0;

   int policyID = -1;
   if ( utilityWt > 0 && m_colUtility >= 0 )
      m_pIDULayer->GetData(cell, m_colUtility, policyID );

   //Policy::SetParserVars( m_pIDULayer, cell );
   int metagoalCount = GetMetagoalCount();
   float *polObjScores = NULL;
   if ( metagoalCount > 0 )
      polObjScores = new float[ metagoalCount ];

   for ( int i=0; i < relevantPolicyCount; i++ )
      {
      Policy *pPolicy = policyScoreArray[ i ].pPolicy;
      bool  mandatory = pPolicy->m_mandatory;

      float cf = 1.0f;
      float score = 0;
      
      if ( mandatory )
         score = pPolicy->m_compliance;  // for mandatory policies
      else
         {
         pPolicy->GetIduObjectiveScores( cell, polObjScores );

         // note: these must be 0-1
         float actorScore   = m_decisionElements & DE_SELFINTEREST     ? GetActorScore( pActor, cell, cf, polObjScores )    : 0.0f;
         float policyScore  = m_decisionElements & DE_ALTRUISM         ? GetAltruismScore( cell, polObjScores )             : 0.0f;
         float utilityScore = ( m_decisionElements & DE_UTILITY && policyID == pPolicy->m_id ) ? 1.0f                       : 0.0f;
         float policyPref   = m_decisionElements & DE_GLOBALPOLICYPREF ? pPolicy->GetIduObjectiveScore( cell, -1 )            : -3;  // -3 to +3
         float policyPrefScore = (float) NormalizeScores( policyPref ); // convert to [0,1]
         float socialNetworkScore = m_decisionElements & DE_SOCIALNETWORK ? GetSocialNetworkScore( pActor, pPolicy ) : 0.0f;

         score = actorScore*selfInterestWt + policyScore*altruismWt + policyPrefScore*policyPrefWt + utilityWt*utilityScore + socialNetworkWt * socialNetworkScore;
         }

      ENV_ASSERT( 0.0f <= score && score <= 1.0f );

      if ( score < tolerance && !mandatory )
         score = 0.0f;

      policyScoreArray[ i ].score = score;
      policyScoreArray[ i ].cf = cf;
      }

   if ( metagoalCount > 0 )
      delete [] polObjScores;

   return relevantPolicyCount;
   }

//-- EnvModel::GetActorScore() ------------------------------------------
//
// return a float in (0,1) that represents the distance in goal space
// between the actors values and the policy intentions
//
// score =  1 when values and policy intentions are identical
// score =  0 when values and policy intentions are as far apart as can be
//
//-----------------------------------------------------------------------

float EnvModel::GetActorScore( Actor *pActor, int cell, float &cf, float *polObjScores )
   {
   // get actor scores computes the alignment between the specified actor's values and the metagoals 
   // used in self-interested decision-making

   // get the number of values/scores (one per model)
   int valueCount = pActor->GetValueCount();
   if( valueCount == 0 )
      return 0.5;

   ENV_ASSERT( valueCount > 0 );
   ENV_ASSERT( valueCount == this->GetActorValueCount() );

   float *a = new float[ valueCount ];  // actors values in goal space
   float *p = new float[ valueCount ];  // policys scores in metagoal space

   int metagoalCount = this->GetMetagoalCount();

   int metagoalIndex = 0;
   int actorValueIndex = 0;

   for ( int i=0; i < metagoalCount; i++ )
      {      
      METAGOAL *pGoal = GetMetagoalInfo( i );

      if ( pGoal->decisionUse & DU_SELFINTEREST )  // implies a policy efficacy slot exists for this value
         {
         // get the actor value for this value
         a[ actorValueIndex ] = pActor->GetValue( actorValueIndex );

         // get the corresponding policy efficacy for this value
         p[ actorValueIndex ] = polObjScores[ metagoalIndex ];

         ENV_ASSERT( -3.0 <= a[actorValueIndex] && a[actorValueIndex] <= 3.0 );
         ENV_ASSERT( -3.0 <= p[actorValueIndex] && p[actorValueIndex] <= 3.0 );

         ++actorValueIndex;
         }

      ASSERT( pGoal->decisionUse != 0 );   // any decision use flag set? (implying that the policy has a efficacy slot for the model)
      ++metagoalIndex;          // then increment the efficacy index slot counter
      }

   float score = 1.0f - GetGoalNorm( a, p, valueCount );

   cf = 1.0f;

   delete [] a;
   delete [] p;
   return score;
   }

double NormalizeScores(double score)
   {
   ENV_ASSERT( -3.0 <= score && score <= 3.0 );
   return (score + 3.0 )/6.0;
   }

float EnvModel::GetAltruismScore( int cell, float *polObjScores )
   {
   // return a float in (0,1) that represents metagoal weighted mean of policy's overall current effectivess.  
   // return 1 when policy efficacies match the scarcities ( policy is most effective at addressing scarcity )
   // return 0 when policy efficacies dont match the scarcities
   // 
   // get the number of scores (one per model that is considered in the scarcity calculations)

   // NOTE:: Doesn't consider "Global" objective

   int metagoalCount = this->GetMetagoalCount();   // includes both self-interested and altruistic
   int scarcityCount = this->GetScarcityCount();   // includes altruistic only

   double a;   // A_BUNDANCE in goal space dimension i
   double e;   // policy's E_FFECTIVENESS in goal space dimension i
   double m;   // M_ETAGOAL in goal space dimension i

   int i, sc;
   double altruismScore  = 0.0;
   double sumMetagoal    = 0.0;
   for ( i=0, sc=0; i < metagoalCount; i++ )
      {
      METAGOAL *pGoal = GetMetagoalInfo( i );

      if ( pGoal->decisionUse & DU_ALTRUISM ) // is this goal used in alteruistic decision-making?
         {
         ENV_ASSERT( pGoal->pEvaluator != NULL );

         int modelIndex = this->GetEvaluatorIndexFromMetagoalIndex( i );
         ENV_ASSERT( 0 <= modelIndex && modelIndex < GetEvaluatorCount() );
         
         a = NormalizeScores( m_landscapeMetrics[modelIndex].scaled );     // convert from (-3, +3) ---> (0, 1)
         e = NormalizeScores( polObjScores[ i ] );  // efficacy of policy to address this scarcity;

         float score = float( e * (((e-a)+1)/2) * (1-a) );

         m = NormalizeScores( pGoal->weight );
         sumMetagoal += m;
         altruismScore += m * score;
         sc++;
         }
      }

   if (sumMetagoal > 0.0)
      altruismScore /= sumMetagoal;

   ENV_ASSERT( sc == scarcityCount );
   ENV_ASSERT( 0.0 <= altruismScore && altruismScore <= 1.0 );

   return (float) altruismScore;
   }

////////float EnvModel::GetAltruismScore( Policy *pPolicy )
////////   {
////////   // return a float in (0,1) that represents the distance in goal space
////////   // between scarcity and the policies the efficacies
////////   //
////////   // priority =  1 when global scores are identical to scarcities ( policy is most effective at addressing scarcity )
////////   // priority =  0 when global scores and scarcities are as far apart as can be
////////
////////   // get the number of scores (one per model that is considered in the scarcity calculations)
////////   //int m = m_evaluatorArray.GetSize();
////////   int metagoalCount = this->GetMetagoalCount();
////////   int scarcityCount = this->GetScarcityCount();
////////   ENV_ASSERT( pPolicy->GetGoalCount() >= metagoalCount );
////////
////////   float *s = new float[ scarcityCount ];  // "goalScarcity" in goal space
////////   float *p = new float[ scarcityCount ];  // policys global scores in goal space
////////   float *a = new float[ scarcityCount ];  // no scarcity point, i.e. -3 in all dimensions
////////   //   i.e. there is no scarcity if s == a
////////
////////   int scarcityIndex = 0;
////////   for ( int i=0; i < metagoalCount; i++ )
////////      {
////////      int modelIndex = this->GetModelIndexFromMetagoalIndex( i );
////////      ENV_ASSERT( 0 <= modelIndex && modelIndex < GetEvaluatorCount() );
////////
////////      if ( m_evaluatorArray[modelIndex]->decisionUse & DU_ALTRUISM )
////////         {
////////         ENV_ASSERT( -3.0 <= m_landscapeMetrics[modelIndex] && m_landscapeMetrics[modelIndex] <= 3.0 );
////////         s[scarcityIndex] = (-1.0f)*GetGsFromAbundance( m_landscapeMetrics[modelIndex], m_evaluatorArray[modelIndex]->metagoal );
////////         p[scarcityIndex] = pPolicy->GetGoalScoreGlobal(i);    // efficacy of policy to address this scarcity
////////         a[scarcityIndex] = 3.0f;
////////         ENV_ASSERT( -3.0 <= s[scarcityIndex] && s[scarcityIndex] <= 3.0 );
////////         ENV_ASSERT( -3.0 <= p[scarcityIndex] && p[scarcityIndex] <= 3.0 );
////////         ++scarcityIndex;
////////         }
////////      }
////////
////////   // first GetGoalNorm() is landscape metric (scaled to metagoal), second is policy efficacy
////////   float score = ( 1.0f - GetGoalNorm( s, p, scarcityCount ) )*GetGoalNorm( s, a, scarcityCount ); //???? why multiply
////////
////////   delete [] s;
////////   delete [] p;
////////   delete [] a;
////////   return (float)score;
////////   }


float EnvModel::GetSocialNetworkScore( Actor *pActor, Policy *pPolicy )
   {
   // Returns a value in [0,1] representing this actor's social network scoring of this policy.
   // Basic idea: Assumes a social network has been defined.  If not, return 0.
   //
   // If a network is defined, the network will have as INPUT landscape eval metrics (scaled to represent
   // scarcity on a [-1,1] interval, -1=abundant, 1=scarce; each landscape metrics is represented by
   // one layer in the network. 
   //
   // The network will have as OUTPUTS activation levels for each ActorGroup for each landscape metric;
   // each layer will have an output for each actor group representing that ActorGroups activation for 
   // the specified metric.
   //
   // To use this information here, we take an approach similar to what is done for comparing actor values 
   // to policy intentions, namely, we collect the social network outputs for the given actor group, and compare
   // those to the policy's intentions. Generally, if the actor's actor group activation is high in the social
   // network and the policy is responsive to that activation, than a high score is returned.  If, on the other hand,
   // the actor's Actor group activation is near zero (in a range [-1,1]), then the actor is neutral towards this policy.
   // If the activation is near -1, the actor will be hostile to any policies that score high in the given metric
   
   // no network defined? then score is 0
   if ( m_pSocialNetwork == NULL )
      return 0;

   // not used in decision-making?  then return 0
   if ( ( this->m_decisionElements & DE_SOCIALNETWORK ) == 0 )
      return 0;

   // network is defined (and used), so compare network outputs with corresponding policy intentions
   // basic idea is to, for each landscape feedback expressed as a social network layer:
   //     //1) Get the value of the scarcity metric from the Eval Model that corresponds to that layer.
   //     1) get the activation level of the Actor's corrresponding ActorGroup from the network.  This reflects
   //           how strong the network influence on the actor is for this particular landscape feedback
   //     2) compare the policy's intention (w.r.t. the given feedback) with the level of activation in the network.
   //           If the network activation is high and the policy is responsive, score the policy high 
   //           If the network activation is low and the policy is not responsive, score the policy high
   //
   //       Network Activation =>  low        low       high        high
   //       Policy Intention   =>  low        high      low         high
   //                  score   =>  low        low       med         high
   // 
   // to implement, multiply the network activation by the policy intention
   
   // iterate through the social network layers.  Note that there is a layer for eval model specified
   // in the network input file.  these have to be reconciled with the appropriate policy intention
   // 
   int scoreCount = 0;

   for ( int i=0; i < m_pSocialNetwork->GetLayerCount(); i++ )
      {
      SNLayer *pLayer = m_pSocialNetwork->GetLayer( i );

      if ( pLayer->m_use == false )
         continue;

      // get activation level for this actor
      SNNode *pActorNode = pLayer->FindNode( pActor->m_pGroup->m_name ); //????  should be a better way
      ASSERT( pActorNode != NULL );
      if ( pActorNode == NULL )
         continue;

      ////float actorActivationLevel = pActorNode->m_activationLevel;      // -1 to +1

      // get policy intention (objective) for this goal.  Note that the policy will have a single global objective, and then 
      // objectives for each metagoal

      // first, get the corresponding evaluative model
      ////EnvEvaluator *pModel = pLayer->m_pModel;  //
      ////
      ////int modelIndex = EnvModel::FindEvaluatorIndex( pModel->m_name );
      ////int metagoalIndex = EnvModel::GetMetagoalIndexFromEvaluatorIndex( modelIndex );
      ////ASSERT( metagoalIndex >= 0 );
      ////
      ////// ObjectiveScores contains arrays of GOAL_SCORES and scroe modifiers for
      ////// a given metagoal associated with a policy
      ////ObjectiveScores *pScores = pPolicy->m_objectiveArray.GetAt( metagoalIndex+1 );      // + 1 because first objective is global
      ////
      ////// get the corresponding policy goal.




      }






   // return a float in (0,1) that represents  metagoal weighted mean of policy's overall current effectivess.  
   // return 1 when policy efficacies match the scarcities ( policy is most effective at addressing scarcity )
   // return 0 when policy efficacies dont match the scarcities
   // 
   // get the number of scores (one per model that is considered in the scarcity calculations)

   // NOTE:: Doesn't consider "Global" objective
/*
   int metagoalCount = this->GetMetagoalCount();   // includes both self-interested and altruistic
   int scarcityCount = this->GetScarcityCount();   // includes altruistic only

   double a;   // A_BUNDANCE in goal space dimension i
   double e;   // policy's E_FFECTIVENESS in goal space dimension i
   double m;   // M_ETAGOAL in goal space dimension i

   int i, sc;
   double altruismScore  = 0.0;
   double sumMetagoal    = 0.0;
   for ( i=0, sc=0; i < metagoalCount; i++ )
      {
      METAGOAL *pGoal = GetMetagoalInfo( i );

      if ( pGoal->decisionUse & DU_ALTRUISM ) // is this goal used in alteruistic decision-making?
         {
         ENV_ASSERT( pGoal->pEvaluator != NULL );

         int modelIndex = this->GetModelIndexFromMetagoalIndex( i );
         ENV_ASSERT( 0 <= modelIndex && modelIndex < GetEvaluatorCount() );
         
         a = NormalizeScores( m_landscapeMetrics[modelIndex].scaled );     // convert from (-3, +3) ---> (0, 1)
         e = NormalizeScores( polObjScores[ i ] );  // efficacy of policy to address this scarcity;

         float score = float( e * (((e-a)+1)/2) * (1-a) );

         m = NormalizeScores( pGoal->weight );
         sumMetagoal += m;
         altruismScore += m * score;
         sc++;
         }
      }

   if (sumMetagoal > 0.0)
      altruismScore /= sumMetagoal;

   ENV_ASSERT( sc == scarcityCount );
   ENV_ASSERT( 0.0 <= altruismScore && altruismScore <= 1.0 );
*/
   return (float) 0; //altruismScore;
   }
      
float EnvModel::GetGoalNorm( float *pPoint1, float *pPoint2, int dimension )
   {
   // normalized distance between two points in the [-3,3]^dimension hypercube
   //
   // summary:
   //
   // let M = sqrt( dimension*6^2 ), the diagonal length of a n-dimensional hypercube with side length of 6 units
   // let d = |pPoint1 - pPoint2| be the euclidian distance between pPoint1 and pPoint2, 0 <= |pPoint1 - pPoint2| <= M
   // norm = d/M
   //
   // score =  0 when d=0
   // score =  1 when d=M
   //
   // ISSUE: Manhattan distance (rather than Euclidean) may better represent Human Decision making.

   double M = sqrt( (double) dimension * 36.0 );
   double norm = 0.0;

   // calculate norm^2
   for ( int i=0; i < dimension; i++ )
      norm += pow( pPoint1[i] - pPoint2[i], 2 );

   norm = ( M - sqrt( norm ) )/M ;
   ENV_ASSERT( 0.0 <= norm && norm <= 1.0 );    // I worry about truncation error causing the norm to be out of range
   return (float)norm;
   }

int SortPolicyScores(const void *elem1, const void *elem2 )
   {
   float score1 = ((POLICY_SCORE*) elem1)->score;
   float score2 = ((POLICY_SCORE*) elem2)->score;

   return ( score1 > score2 ) ? -1 : 1;
   }


int EnvModel::SelectPolicyToApply( int cell, PolicyScoreArray &policyScoreArray, int relevantPolicyCount )
   {
   // coming into this method, the policyScoreArray contains all policies that passed
   // the noncompensatory attribute test, tagged with a score in (0,1) reflecting compensatory attribute assessment.
   //
   // now, we want to sample this score array to figure which, if any, of the policies 
   // we want apply.

   if ( relevantPolicyCount == 0 )
      return -1;

   int policyIndex = 0;

   if ( m_decisionType == DT_MAX )
      {
      float maxScore = 0;
      for ( int i=0; i < relevantPolicyCount; i++ )
         {
         if ( policyScoreArray[ i ].score > maxScore )
            {
            maxScore = policyScoreArray[ i ].score;
            policyIndex = i;
            }
         }

      return policyIndex;
      }

   float cumPVal = (float) relevantPolicyCount;

   // have the total range of possible values - cumPVal become the width of the distribution.  Sample
   // uniformly from this width to select a policy
   float position = (float) m_randUnif.RandValue( 0, cumPVal );      // get a random number in (0,cumPVal)
   float positionSoFar = 0.0f;

   for ( policyIndex=0; policyIndex < relevantPolicyCount; policyIndex++ )
      {
      positionSoFar += policyScoreArray[ policyIndex ].score;

      if ( positionSoFar >= position )
         return policyIndex;
      }

   // no policy sampled, no action taken
   return -1;
   }


//----------------------------------------------------------------------------------
// EnvModel::SelectPolicyOutcome()
//
// -- this methods selects, using a weighted sampling scheme, the specific policy 
//    outcome to use for a specific application of this policy.
//
//   -2 - policy has no outcomes
//   -1 - policy has outcomes, but none selected
//   0 or greater - multioutcome outcome to apply
//-----------------------------------------------------------------------------------

int EnvModel::SelectPolicyOutcome( MultiOutcomeArray &multiOutcomeArray )
   {
   int selectedOutcome = -1;
   int outcomeCount = multiOutcomeArray.GetSize(); //  pPolicy->GetMultiOutcomeCount();

   if ( outcomeCount == 0 )
      return -2;

   float  totalWeight = 0.0f;
 //  int *weights = new int[ outcomeCount + 1 ];   // +1 for "no outcome selected" (wts add to less than 100)
   float *weights = new float[outcomeCount + 1];   // +1 for "no outcome selected" (wts add to less than 100)

   for ( int i = 0; i < outcomeCount; i++ )
      {
      weights[ i ] = multiOutcomeArray.GetMultiOutcome(i).GetProbability();
      totalWeight += weights[ i ];
      }

   if ( totalWeight < 100.0f )
      {
      weights[ outcomeCount ] = 100.0f - totalWeight;
      totalWeight = 100.0f;
      }
   else
      {
      weights[ outcomeCount ] = 0.0f;
      }

   selectedOutcome = m_randUnif.SampleFromWeightedDistribution( weights, outcomeCount+1, totalWeight );

   if ( selectedOutcome == outcomeCount ) // last bin?
      selectedOutcome = -1; // means do nothing

   delete [] weights;
   return selectedOutcome;
   }

//----------------------------------------------------------------------------------------
// apply a policy's outcome to a cell.  This means to apply, through the delta array,
// all the database changes associated with the policy, including 
//  1) the index of the policy applied to the cell
//  2) the score (reflecting the preference for this policy)
//  3) the current calendar year
//  4) the number of times a policy has been applied to this cell
//
//  Also, if there is a probe associated with this policy, update it
//
// if the policy does something (other than subdivide), recode the policy outcome as well
//----------------------------------------------------------------------------------------

void EnvModel::ApplyPolicy( Policy *pPolicy, int idu, float score )
   {
   // get a desired outcome out of the range of possible outcomes prescribed by the policy
   int outcome = SelectPolicyOutcome( pPolicy->m_multiOutcomeArray );
   if ( outcome < 0 )
      {
      pPolicy->m_noOutcomeCount++;
      return;
      }

   pPolicy->m_appliedCount++;    // note: only incremented if an outcome is selected
   pPolicy->m_cumAppliedCount++;

   // Set basic policy info into the cell database.
   // If cell is defunct; then no use continuing execution here;  return;
   //
   if ( AddDelta( idu, m_colPolicy, m_currentYear, pPolicy->m_id, DT_POLICY ) < 0 )
      return;  

   bool readOnly = m_pIDULayer->m_readOnly;
   m_pIDULayer->m_readOnly = false;
   m_pIDULayer->SetData( idu, m_colScore, score );
   m_pIDULayer->m_readOnly = readOnly;

   AddDelta(idu, m_colLastDecision, m_currentYear, m_currentYear, DT_POLICY );

   // if exclusive, set the date the next decision can be made for this cell
   int persistence = pPolicy->GetPersistence();
   if ( pPolicy->m_exclusive )
      {
      int nextDecisionYear = m_currentYear + persistence;
      AddDelta(idu, m_colNextDecision, m_currentYear, nextDecisionYear, DT_POLICY );
      }

   int apps;
   m_pIDULayer->GetData(idu, m_colPolicyApps, apps );
   AddDelta(idu, m_colPolicyApps, m_currentYear, apps+1, DT_POLICY );

   // interpret and apply the policy outcome
   //   outcome == -1 means do nothing
   //   outcome == -42 = subdivide
   pPolicy->m_appliedArea   = 0;
   pPolicy->m_expansionArea = 0;

   if ( pPolicy->GetMultiOutcomeCount() > 0 )  
      {
      // apply the specific multiOutcome selected.  This consists of zero or more outcomes
      const MultiOutcomeInfo &multiOutcome = pPolicy->GetMultiOutcome( outcome );
      ApplyMultiOutcome( multiOutcome, idu, persistence );

      // note: if the outcome contains an Expand function, and the policy has a budget constraints,
      // then we need to make sure the budget gets taken care of by the Expand function handler.
      // we do this by calculating the entire area affected by the policy, and using that
      // area to generate costs   
      m_pIDULayer->GetData(idu, m_colArea, pPolicy->m_appliedArea );   // area of IDU policy is applied to
      }

   // update constraints
   int constraintCount = pPolicy->GetConstraintCount();
   if ( constraintCount > 0 )
      {
      for ( int i=0; i < constraintCount; i++ )
         {
         PolicyConstraint *pConstraint = pPolicy->GetConstraint( i );
         GlobalConstraint *pGC = pConstraint->m_pGlobalConstraint;

         // add to the constraint list if valid and update global constraint
         if ( pGC != NULL )
            {
            float totalArea = pPolicy->m_appliedArea + pPolicy->m_expansionArea;

            // subtract this policy's initial cost from the associated global constraint
            if ( pGC->ApplyPolicyConstraint( pConstraint, idu, totalArea, true ) == false )
               {
               //CString msg;
               //msg.Format( "Error encountered applicy policy constraint '%s' for policy '%s'", (LPCTSTR) pGC->m_name, (LPCTSTR) pPolicy->m_name );
               //AfxMessageBox( msg );
               }

            int duration = 0;
            pConstraint->GetDuration(idu, duration);

            if ( duration > 0 )   // maintenance costs only
               m_activeConstraintList.AddConstraint( pConstraint, idu, pPolicy->m_appliedArea );
            }
         }
      }

   pPolicy->m_cumAppliedArea   += pPolicy->m_appliedArea;  // accumulate area for this run
   pPolicy->m_cumExpansionArea += pPolicy->m_expansionArea;
   }


void EnvModel::ApplyMultiOutcome( const MultiOutcomeInfo &multiOutcome, int cell, int persistence )
   {
   for ( int i=0; i < multiOutcome.GetOutcomeCount(); i++ )
      {
      const OutcomeInfo &outcome = multiOutcome.GetOutcome( i );

      // check to see if there is any persistent outcomes that preclude this outcome from being set
      //  this looks at the column associated with this outcome and sees if anything is recorded in the 
      //  outcomestatusArray for this cell that would block application of this outcome.

      if ( outcome.col >= 0 ) // this means it isn't a function
         {
         ASSERT( outcome.m_pFunction == NULL );

         bool isColBlocked = IsColBlocked( cell, outcome.col );

         if ( ! isColBlocked )      // okay to apply outcome?  then apply it
            {
            bool directiveApplied = false;

            // before applying the outcome,  check to see if there is any outcome directives. 
            OutcomeDirective *pDirective = outcome.m_pDirective;

            VData value = outcome.value;

            if ( outcome.m_pMapExpr != NULL )
               {
               bool ok = outcome.m_pMapExpr->Evaluate( cell );

               if ( ok )
                  value = (float) outcome.m_pMapExpr->GetValue();
               }

            if ( pDirective == NULL )     //  if no directives, just apply it.
               AddDelta( cell, outcome.col, m_currentYear, value, DT_POLICY );

            else  // directives exist, so process them 
               {
               switch( pDirective->type )
                  {
                  case ODT_EXPIRE:
                  case ODT_PERSIST:
                     if ( AddDelta( cell, outcome.col, m_currentYear, value, DT_POLICY ) < 0 )
                        { 
                        directiveApplied = false;
                        break;
                        }
                     AddOutcomeStatus( cell, pDirective->type, outcome.col, m_currentYear + pDirective->period, pDirective->outcome );
                     directiveApplied = true;
                     break;

                 case ODT_DELAY:   // NOTE:  No delta added - this will get processed later in CheckPolicyOutcomes()
                     AddOutcomeStatus( cell, pDirective->type, outcome.col, m_currentYear + pDirective->period, pDirective->outcome );
                     directiveApplied = true;
                     break;
                  }
               }

            // if no directive was applied to the outcome, and the policy is persistent, add an outcomeStatus entry to the cell's OutcomeStatusArray
            if ( ! directiveApplied && persistence > 0 )
               AddOutcomeStatus( cell, ODT_PERSIST, outcome.col, m_currentYear+persistence, VData() );
            }  // end of:  ( ! isColBlocked )
         }  // end of: outcomeCol >= 0 )
      
      else  // it's a function, run it
         {
         if ( outcome.m_pFunction == NULL )
            {
            CString msg( "bad outcome function: " );
            CString function;
            OutcomeInfo *pOutcome = (OutcomeInfo*) &outcome;
            pOutcome->ToString( function, false );
            msg += function;
            Report::ErrorMsg( msg );
            }
         ASSERT( outcome.m_pFunction != NULL );
         outcome.m_pFunction->Run( m_pDeltaArray, cell, m_currentYear, persistence );      // note:  col blocking is checked in Run() when it call ApplyMultiOutcome();
         }
      }  // end of:  for ( i < multiOutcome.GetOutcomeCount )
   }


void EnvModel::AddOutcomeStatus( int cell, OUTCOME_DIRECTIVE_TYPE type, int col, int targetYear, VData outcomeData )
   {
   OutcomeStatusArray *pOutcomeStatusArray = NULL;
   bool found = m_outcomeStatusMap.Lookup( cell, pOutcomeStatusArray );

   if ( ! found )     // is this cell doesn't have an outcomeStatusArray yet, add one
      {
      pOutcomeStatusArray = new OutcomeStatusArray;
      m_outcomeStatusMap.SetAt( cell, pOutcomeStatusArray );
      }

   pOutcomeStatusArray->AddOutcomeStatus( type, col, targetYear, outcomeData );
   }



bool EnvModel::IsColBlocked( int cell, int col )
   {
   if ( col < 0 )
      return false;

   OutcomeStatusArray *pOutcomeStatusArray = NULL;
   bool found = m_outcomeStatusMap.Lookup( cell, pOutcomeStatusArray );

   if ( found && pOutcomeStatusArray )
      {
      // see if this outcome is blocked by a persistent outcome
      for ( int j=0; j < pOutcomeStatusArray->GetSize(); j++ )
         {
         OUTCOME_STATUS &os = pOutcomeStatusArray->GetAt( j );
         switch( os.type )
            {
         case ODT_PERSIST:
            if ( os.col == col && os.targetYear > this->m_currentYear )
               return true;
            break;

         case ODT_DELAY:
         case ODT_EXPIRE:
         default:
            break;
            }
         }
      }

   return false;
   }


int EnvModel::RunPolicyMetaprocess()
   {  
   //if ( m_showRunProgress )
   //   gpMain->SetModelMsg( "PolicyMetaProcess" );

   //m_policyMetaProcess.Run();
   return 1;
   }


int EnvModel::RunCulturalMetaprocess()
   {
   // this method runs the cultural metaprocess that updates actor behavior in response to 
   // metagoals coupled to the landscape evaluation metrics.

   //const float responseRate = 0.01f;  // actors step 1% closer to "scarcity" in goal space

   //int goalCount = m_evaluatorArray.GetSize();

   //// for each actor
   //for ( int j=0; j < m_pActorManager->GetActorCount(); j++ )
   //   {
   //   Actor *pActor = m_pActorManager->GetActor( j );

   //   // adjust actor weights appropriately
   //   // actors values step toward scarcity in goal space
   //   for ( int i=0; i < goalCount; i++ )
   //      {
   //      float s = (-1.0f)*m_evaluatorArray[ i ]->score;  // s is "scarcity"
   //      float weight = pActor->GetValue( i ); // ???WRONG!!!!
   //      ENV_ASSERT( -3.0f <= s      &&      s <= 3.0f );
   //      ENV_ASSERT( -3.0f <= weight && weight <= 3.0f );
   //      
   //      float newWeight = ( s - weight )*responseRate + weight;

   //      pActor->SetValue( i, newWeight );
   //      }
   //   }

   return -1;
   }


// ------------------------------------------------------------------------
// GetGsFromAbundance()
//
// -- compute a goal satifisaction level from an abundance of the goal.
//      inputs:        -3 <= bias <= 3, -3 <= abundanceScore <= 3
//      outputs:       -3 <=  Gs  <= 3  
//    bias =  3 (concerned) satisfied with abundanceScore for only extreamly high values
//    bias = -3 (dont care) satisfied with abundanceScore for all values except extreamly low values
//    if bias is 0, this function will return abundanceScore ( ignoring truncation and all that ... )
//
// ------------------------------------------------------------------------
float EnvModel::GetGsFromAbundance( float abundanceScore, float bias )
   {
   static double base = 3.0;  // dial: amount of variability. Must be greater than 1
   //       for above interpretation to be correct.

   // scale to (0,1)
   double gs = ( (double)abundanceScore + 3.0 )/6.0;

   double  biasPower = pow( base, (double)bias/3.0 );

   gs  = 6.0*pow( gs, biasPower )-3.0;

   return (float)gs;
   }


void EnvModel::SetTargetPolys( int targetPolys[], int size )
   {
   m_targetPolyCount = m_pIDULayer->GetRecordCount( MapLayer::ALL );

   if ( m_targetPolyArray == NULL )
      m_targetPolyArray = new bool[ m_targetPolyCount ];

   memset( (void*) m_targetPolyArray, 0, m_targetPolyCount * sizeof( bool ) );

   for ( int i=0; i < size; i++ )
      m_targetPolyArray[ targetPolys[ i ] ] = true;
   }


void EnvModel::ClearTargetPolys()
   {
   if ( m_targetPolyArray != NULL )
      {
      delete [] m_targetPolyArray;
      m_targetPolyArray = NULL;
      }

   m_targetPolyCount = 0;
   }



// Note: InitModels() should be called AFTER the models records are loaded, but BEFORE
// any policies are evaluated (if, e.g. there are missing scores) or any models are run.
void EnvModel::InitModels()   
   {
   ASSERT( ! m_areModelsInitialized );
   WAIT_CURSOR;

   StoreModelCols();

   //----------------------------------------------------------------
   // ---------- Initialize External Models --------------------------
   //----------------------------------------------------------------
   m_envContext.startYear    = m_startYear;   // year in which the simulation started
   m_envContext.endYear      = m_endYear;     // year in which the simulation ends
   m_envContext.currentYear  = m_startYear;   // current year
   m_envContext.yearOfRun    = 0;
   m_envContext.run          = -1;                    // In a multirun session, runID is incremented after each run.
   //??Check
   //context.showMessages = (m_debug == 1);
   //context.logMsgLevel;           // see flags in evomodel.h
   m_envContext.pDeltaArray = NULL;            // pointer to the current delta array
   m_envContext.handle = -1;                // unique handle of the model
   m_envContext.col = -1;                   // database column to populate, -1 for no column
   m_envContext.firstUnseenDelta = -1;      // models should start scanning the deltaArray here
   m_envContext.lastUnseenDelta = -1;
   m_envContext.targetPolyArray = NULL;      // currently unused
   m_envContext.pEnvExtension = NULL;         // opaque ptr to EnvEvaluator or EnvModelProcess as appropriate; thus deprecates: id, col, score
#ifndef NO_MFC
   m_envContext.pWnd = NULL;
#endif
   m_envContext.score = 0;                 // overall score for this evaluation, -3 to +3 (unused by AP's)
   m_envContext.rawScore = 0;              // model-specific raw score ( not normalized )
   m_envContext.pDataObj = NULL;              // data object returned from the model at each time step (optional, NULL if not used)
   m_envContext.col = -1;

   CString name;

   // Call the init function (if available) for the autonomous processes
   //Report::StatusMsg( "Initializing the Autonomous Processes" );
   try
      {
      for ( int i=0; i < GetModelProcessCount(); i++ )
         {
         EnvModelProcess *pInfo = GetModelProcessInfo(i);
         if ( pInfo->m_use )
            {
            name = pInfo->m_name;
            CString msg = "Initializing ";
            msg += pInfo->m_name;
            Report::StatusMsg(msg);
            Report::Log(msg);
            m_envContext.id = pInfo->m_id;
            //m_envContext.col = pInfo->col;
            m_envContext.handle = pInfo->m_handle;
            m_envContext.pEnvExtension = pInfo;

            pInfo->Init(&m_envContext, (LPCTSTR)pInfo->m_initInfo);

            msg = "Initialization completed for ";
            msg += pInfo->m_name;
            Report::Log(msg);

            // create app vars from any model outputs
            MODEL_VAR *modelVarArray = NULL;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR *pModelVar = modelVarArray + j;

               //CString msg;
               //msg.Format( "%s ModelVar %i of %i, name:%s", (LPCTSTR) pInfo->m_name, 
               //   j, varCount, pModelVar->m_name );
               //Report::Log( msg );

               if (pModelVar->pVar != NULL)
                  {
                  CString name(pInfo->m_name);
                  name += ".";
                  name += pModelVar->name;
                  name.Replace(' ', '_');
                  AppVar *pAppVar = new AppVar(name, pModelVar->description, NULL);
                  pAppVar->m_avType = AVT_OUTPUT;
                  pAppVar->m_pModelVar = pModelVar;
                  pAppVar->m_pEnvProcess = pInfo;
                  pAppVar->SetValue(VData(pModelVar->pVar, pModelVar->type, true));  // stores ptr to variable, so value is always current
                  AddAppVar(pAppVar, false);  // creates corresponding MapExpr - no MapExpr required since it just references a model var
                  }
               }
            }
         }
      }
   catch( std::exception &e )
      {
      CString msg = "Error thrown during Init() when initializing the ";
      msg += name;
      msg += " plugin.  Exception: ";
      msg += e.what();
      Report::ErrorMsg(msg);
      ASSERT(0); 
      }
   catch( ... )
      {
      CString msg = "Error thrown during Init() when initializing the ";
      msg += name;
      msg += " plugin.";
      Report::ErrorMsg(msg);
      ASSERT(0); 
      }

	// Call the init function (if available) for the evaluative models
	try
	   {
		for (int i = 0; i < GetEvaluatorCount(); i++)
		   {
			EnvEvaluator *pInfo = GetEvaluatorInfo(i);
			if (pInfo->m_use)
				{
				name = pInfo->m_name;
				CString msg = "Initializing ";
				msg += pInfo->m_name;
				Report::StatusMsg(msg);
				Report::Log(msg);
				//m_envContext.col = pInfo->col;
				m_envContext.id = pInfo->m_id;
				m_envContext.handle = pInfo->m_handle;
				m_envContext.pEnvExtension = pInfo;
				pInfo->Init(&m_envContext, pInfo->m_initInfo);

            name += ".";
            name += "rawscore";
            AppVar *pAppVar = new AppVar(name, "", NULL);
            pAppVar->m_avType = AVT_RAWSCORE;
            pAppVar->m_pEnvProcess = pInfo;
            pAppVar->SetValue(VData(&(pInfo->m_rawScore), TYPE_FLOAT, true));
            AddAppVar(pAppVar, false);

            name = pInfo->m_name;
            name += ".";
            name += "score";
            pAppVar = new AppVar(name, "", NULL);
            pAppVar->m_avType = AVT_SCORE;
            pAppVar->m_pEnvProcess = pInfo;
            pAppVar->SetValue(VData(&(pInfo->m_score), TYPE_FLOAT, true));
            AddAppVar(pAppVar, false);

            MODEL_VAR *modelVarArray = NULL;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR *pModelVar = modelVarArray + j;

               //CString msg;
               //msg.Format( "%s ModelVar %i of %i, name:%s", (LPCTSTR) pInfo->m_name, 
               //   j, varCount, pModelVar->m_name );
               //Report::Log( msg );
               if (pModelVar->pVar != NULL)
                  {
                  CString name(pInfo->m_name);
                  name += ".";
                  name += pModelVar->name;
                  pAppVar = new AppVar(name, pModelVar->description, NULL);
                  pAppVar->m_avType = AVT_OUTPUT;
                  pAppVar->m_pModelVar = pModelVar;
                  pAppVar->m_pEnvProcess = pInfo;
                  pAppVar->SetValue(VData(pModelVar->pVar, pModelVar->type, true));
                  AddAppVar(pAppVar, false);
                  }
               }  // end of: for j < varCount
            }  // end of: i < model count
			}  // end of: for each model
		}  // end of try
	catch (std::exception &e)
   	{
		CString msg = "Error thrown during Init() when initializing the ";
		msg += name;
		msg += " plugin.  Exception: ";
		msg += e.what();
		Report::ErrorMsg(msg);
		ASSERT(0);
	   }
	catch (...)
	   {
		CString msg = "Error thrown during Init() when initializing the ";
		msg += name;
		msg += " plugin.";
		Report::ErrorMsg(msg);
		ASSERT(0);
	   }

   //Report::StatusMsg( "Initializing the Policy MetaProcess" );
   // Call the init function for the PolicyMetaProcess
   //EnvModelProcess *pInfo = m_pModel->GetMetaProcessInfo(0);
   //pInfo->initRunFn( &m_pModel->m_evoContext );
   //m_pModel->m_policyMetaProcess.Init( m_pIDULayer, gpPolicyManager, &m_model );

   m_areModelsInitialized = true;
   Report::StatusMsg( "" );
   return;
   }
   
   
// EnvModel::InitRun() is called at the begining of each model run (in the Run() method.  Everything that needs to be 
//  initialized before a run should be handled here.
void EnvModel::InitRun()
   {
   int i; // for loop counter
   TCHAR msg[ 265 ];

   // if     n*m_yearsToRun <= m_currentYear < (n+1)*m_yearsToRun, 
   // then   m_endYear = (n+1)*m_yearsToRun
   // where  n = 0,1,2,... 
   m_endYear = ((m_currentYear-m_startYear)/m_yearsToRun)*m_yearsToRun + m_yearsToRun + m_startYear;

   if ( m_runStatus == RS_PRERUN )
      {
      // roll back all change seen by the current deltaArray
      Report::StatusMsg( "Unrolling Delta Array to initial state" );
      UnApplyDeltaArray( m_pIDULayer );  // default arguments - use internal delta array, unapply everything
      Report::StatusMsg( "Unrolling Delta Array Completed" );

      //Notify( EMNT_INITRUN, 0 );   // update mapwnd

      //----------------------------------------------------------------
      //---------------- initialize data objects -----------------------
      //----------------------------------------------------------------
      Report::StatusMsg( "Creating data objects..." );
      m_pDataManager->CreateDataObjects();   // note: this triggers a call to the results tab to create a run 
                                                   // and adds a RUN_INFO to the DataManager's list      
      Report::StatusMsg( "Creating data objects...completed" );

      if ( this->m_discardMultiRunDeltas && this->m_inMultiRun && m_currentRun >= 0 )
         m_pDataManager->DiscardDeltaArray( m_currentRun );

      m_currentRun++;

      // allocate a new delta array for this run
      CreateDeltaArray( m_pIDULayer );

      //ASSERT( m_currentYear == m_startYear );
      m_currentYear = m_startYear;
      m_endYear = m_startYear + m_yearsToRun;

      m_actorDecisionRunTime = 0.0f;
      m_dataCollectionRunTime = 0.0f;

      int emCount  = (int) m_evaluatorArray.GetSize();
      int apCount  = (int) m_modelProcessArray.GetSize();
      int vizCount = (int) m_vizInfoArray.GetSize();

      m_consideredCellCount = 0;

      // clear previous and store new reset information
      m_resetInfo.Clear();

      // make copies of original policies
      for ( i=0; i < m_pPolicyManager->GetPolicyCount(); i++ )
         m_resetInfo.policyArray.Add( new Policy( *m_pPolicyManager->GetPolicy( i ) ) );

      // make copies of original actors.  Note that any "new" actors added during the
      // run are not used here.
      for ( i=0; i < m_pActorManager->GetActorCount(); i++ )
         m_resetInfo.actorArray.Add( new Actor( *m_pActorManager->GetActor( i ) ) );

      // reset any needed policy info
      ResetPolicyStats( true );
 
      // set up any scheduled policies
      m_pPolicyManager->BuildPolicySchedule();

      // clear out any data
      m_pIDULayer->SetColNoData( m_colPolicy );
      m_pIDULayer->SetColNoData( m_colScore );
      m_pIDULayer->SetColNoData( m_colNextDecision );

      // clear out outcomeStatusMap
      POSITION pos = m_outcomeStatusMap.GetStartPosition();
      while( pos != NULL )
         {
         OutcomeStatusArray *pOutcomeStatusArray = NULL;
         int cell;
         m_outcomeStatusMap.GetNextAssoc( pos, cell, pOutcomeStatusArray );
         ASSERT( pOutcomeStatusArray != NULL );
         delete pOutcomeStatusArray;
         }
      m_outcomeStatusMap.RemoveAll();

      // set up EnvContext data for models/ALPS/POP
      m_envContext.pMapLayer       = m_pIDULayer;
      m_envContext.run             = m_currentRun;
      m_envContext.scenarioIndex   = m_scenarioIndex;
      m_envContext.startYear       = m_startYear;
      m_envContext.endYear         = m_endYear;
      m_envContext.currentYear     = m_currentYear;
      m_envContext.yearOfRun       = -1;  // 
      m_envContext.pActorManager   = m_pActorManager;
      m_envContext.pPolicyManager  = m_pPolicyManager;
      m_envContext.pDeltaArray     = m_pDeltaArray;
      m_envContext.pLulcTree       = &m_lulcTree;
      m_envContext.pQueryEngine    = m_pQueryEngine;
      m_envContext.pExprEngine     = m_pExprEngine;
      //???Check
      //m_envContext.showMessages    = m_showMessages = ( gpDoc->m_debug == 1 );
	   m_envContext.exportMapInterval = m_exportMapInterval;
	   m_envContext.logMsgLevel     = m_logMsgLevel > 0 ? m_logMsgLevel : 0xFFFF;
      m_envContext.targetPolyArray = m_targetPolyArray;
      m_envContext.targetPolyCount = m_targetPolyArray == NULL ? 0 : m_pIDULayer->GetPolygonCount();

      // add a new result collection to results panel      
      //gpResultsPanel->AddRun( m_currentRun );
      Notify( EMNT_INITRUN, m_currentRun, m_endYear-m_startYear );

      // set up nonstatic model info
      m_landscapeMetrics.SetSize( emCount, 0 );
      for ( i=0; i < emCount; i++ )
         {
         m_landscapeMetrics[i].raw = 0.0f;
         m_landscapeMetrics[i].scaled = 0.0f;
         }

      m_emFirstUnseenDelta.SetSize( emCount, 0 );
      for ( i=0; i < emCount; i++ )
         m_emFirstUnseenDelta[i] = 0;

      m_apFirstUnseenDelta.SetSize( apCount, 0 );
      for ( i=0; i < apCount; i++ )
         m_apFirstUnseenDelta[i] = 0;

      m_vizFirstUnseenDelta.SetSize( vizCount, 0 );
      for ( i=0; i < vizCount; i++ )
         m_vizFirstUnseenDelta[i] = 0;

      //----------------------------------------------------------------
      //---------------- initialize actors -----------------------------
      //----------------------------------------------------------------
      RandUniform randUnif( 0, 1 );  //long( pActor->GetDecisionFrequency()-0.1f ) );

      for ( i=0; i < m_pActorManager->GetActorCount(); i++ )
         {
         Actor *pActor = m_pActorManager->GetActor( i );
         pActor->InitRun();   // clears actors history array

         // initialize Last Decision field
         int iduCount = pActor->GetPolyCount();

         for ( int i=0; i < iduCount; i++ )
            {
            int idu = pActor->GetPoly( i );
            m_pIDULayer->SetData( i, m_colLastDecision, (int) randUnif.RandValue( 0, pActor->GetDecisionFrequency()-0.1f ) );
            }
         }

      if ( m_pSocialNetwork )
         m_pSocialNetwork->InitRun();

      UpdateAppVars( -1, 0, 3 );  // global vars, don't apply to coverage, pre and post timing

      for ( i=0; i < apCount; i++ )
         {
         EnvModelProcess *pInfo = m_modelProcessArray[ i ];
         if ( pInfo->m_use )
            {
            pInfo->m_runTime = 0.0f;

            m_envContext.pEnvExtension  = pInfo;
            m_envContext.id     = pInfo->m_id;
            m_envContext.handle = pInfo->m_handle;
            //m_envContext.col    = pInfo->col;
            m_envContext.currentYear = this->m_startYear-1;

            lstrcpy( msg, "Initializing " );
            lstrcat( msg, pInfo->m_name );
            Report::StatusMsg( msg );

            bool ok = pInfo->InitRun( &m_envContext, m_envContext.pEnvModel->m_resetInfo.useInitialSeed );

            lstrcpy( msg, "Completed initialization:  " );
            lstrcat( msg, pInfo->m_name );
            Report::StatusMsg( msg );

            if ( !ok )
               {
               CString msg;
               msg.Format( "The APInitRun(.) function of %s returned false.  This autonomous process will not be used.", (PCTSTR) pInfo->m_name );
               Report::ErrorMsg( msg );

               pInfo->m_use = false;
               }
            }
         }

      //----------------------------------------------------------------
      //---------------- initialize models -----------------------------
      //----------------------------------------------------------------
      for ( i=0; i < emCount; i++ )
         {
         EnvEvaluator *pInfo = m_evaluatorArray[ i ];
         if ( pInfo->m_use )
            {
            pInfo->m_runTime = 0.0f;

            m_envContext.pEnvExtension = pInfo;
            //m_envContext.col    = pInfo->col;
            m_envContext.id     = pInfo->m_id;
            m_envContext.handle = pInfo->m_handle;
            m_envContext.currentYear = this->m_startYear-1;

            lstrcpy( msg, "Initializing " );
            lstrcat( msg, pInfo->m_name );
            Report::StatusMsg( msg );

            bool ok = pInfo->InitRun( &m_envContext, m_envContext.pEnvModel->m_resetInfo.useInitialSeed );

            lstrcpy( msg, "Completed initialization:  " );
            lstrcat( msg, pInfo->m_name );
            Report::StatusMsg( msg );

            if ( !ok )
               pInfo->m_use = false;
            }
         }

      //----------------------------------------------------------------
      //---------------- initialize visualizers -----------------------------
      //----------------------------------------------------------------
      //skip for Linux build
#ifndef NO_MFC
      /*
      for ( i=0; i < vizCount; i++ )
         {
         ENV_VISUALIZER *pInfo = m_vizInfoArray[ i ];
         if ( pInfo->m_use )
            {
            //pInfo->m_runTime = 0.0f;
            INITRUNFN initRunFn = pInfo->initRunFn;

            if ( initRunFn != NULL )
               {
               m_envContext.pEnvExtension = pInfo;
               m_envContext.col    = -1;
               m_envContext.id     = pInfo->m_id;
               m_envContext.handle = pInfo->m_handle;
               m_envContext.pWnd   = NULL;

               lstrcpy( msg, "Initializing " );
               lstrcat( msg, pInfo->m_name );
               Report::StatusMsg( msg );

               bool ok = initRunFn( &m_envContext, m_envContext.pEnvModel->m_resetInfo.useInitialSeed );

               lstrcpy( msg, "Completed initialization:  " );
               lstrcat( msg, pInfo->m_name );
               Report::StatusMsg( msg );

               if ( !ok )
                  pInfo->m_use = false;
               }
            }
         } */
#endif

      // initialize global constraints
      for ( int i=0; i < m_pPolicyManager->GetGlobalConstraintCount(); i++ )
         {
         GlobalConstraint *pGC = m_pPolicyManager->GetGlobalConstraint( i );
         pGC->Reset();  // increments cumulative, resets current to 0 and sets m_value
         pGC->ResetCumulative();  // cumulative resets to 0
         }

      m_activeConstraintList.RemoveAll();  // deletes objects and empties list

      // do initial evaluation
      RunEvaluation();   // note: context.currentYear = startYear-1

      m_pDataManager->CollectData( 0 );    // collect initial output data prior to run

      }  // end of:  if ( m_runStatus == RS_PRERUN )

   ASSERT( m_pDataManager != NULL );
   m_pDataManager->SetDataSize( m_yearsToRun + 1 );   // +1 for initial data collection

   //----------------------------------------------------------------
   //-------------------- initialize display ------------------------
   //----------------------------------------------------------------
   //???Check
   //gpView->m_viewPanel.SetRun( m_currentRun );
   //gpView->m_viewPanel.SetYear( m_currentYear );
   //
   //gpView->StartVideoRecorders();
   //gpView->UpdateVideoRecorders();   // initial capture

   // all done initializing this run
   Report::StatusMsg( "All initializations completed..." );
   }



// EnvModel::Reset() - this is called after a run during a MultiRun().  It should do any post-run cleanup 
//   necessary to prepare for another run.  It SHOULDN'T do things managed by InitRun()!
bool EnvModel::Reset()
   {
   if ( m_runStatus != RS_PRERUN )
      {
      ENV_ASSERT( m_pIDULayer  != NULL );
      //ENV_ASSERT( gpResultsPanel != NULL );
      ENV_ASSERT( m_pDeltaArray != NULL );
      ENV_ASSERT( m_pDataManager != NULL );

      // resize the existing current data objects maintained by the 
      // DataManager to match the current year.
      m_pDataManager->SetDataSize( m_currentYear-m_startYear+1 );  // + 1 for zero based index

      // back up all decisions that were made
      //Report::StatusMsg( "Unrolling Delta Array to initial state" );
      //UnApplyDeltaArray( m_pIDULayer );  // default arguments - use internal delta array, unapply everything
      //Report::StatusMsg( "Unrolling Delta Array Completed" );

      //---------- added jpb 8/14/06 -------------------
      //int startDeltas = m_pDeltaArray->GetCount();
      //int endDeltas = m_pDeltaArray->Compact();
      //CString msg;
      //msg.Format( "Compacting DeltaArray at end of run.  Start size=%i, End size=%i\n", startDeltas, endDeltas );
      //TRACE( (LPCTSTR) msg );

      m_pDeltaArray->FreeExtra();

      //------------------------------------------------

      // remove all dynamicly added cells
      ASSERT( 0 == m_pIDULayer->GetDefunctCountByIteration());
      ASSERT( m_pIDULayer->GetRecordCount( MapLayer::ALL ) == m_pIDULayer->GetRecordCount( MapLayer::ACTIVE ) );
      //m_pIDULayer->DynamicClean();  // NOTE:  Shouldn't be necessary!!!

      // reload original policies and actors
      ASSERT( m_resetInfo.policyArray.GetSize() == m_pPolicyManager->GetPolicyCount() );
      for ( int i=0; i < m_resetInfo.policyArray.GetSize(); i++ )
         {
         Policy *pPolicy = m_pPolicyManager->GetPolicy( i );
         Policy *pResetPolicy = m_resetInfo.policyArray[ i ];
         //*pPolicy = *m_resetInfo.policyArray[ i ];   // change jpb 2/19/08 - don't copy whole thing, just reset relevant members
         pPolicy->m_appliedCount = pResetPolicy->m_appliedCount;
         pPolicy->m_cumAppliedCount = pResetPolicy->m_cumAppliedCount;
         pPolicy->m_age = pResetPolicy->m_age;
         }

      ASSERT( m_resetInfo.actorArray.GetSize() == m_pActorManager->GetActorCount() );
      for ( int i=0; i < m_resetInfo.actorArray.GetSize(); i++ )
         {
         Actor *pActor = m_pActorManager->GetActor( i );
         *pActor = *m_resetInfo.actorArray[ i ];
         }

      // reset random number generators
      if ( m_resetInfo.useInitialSeed )
         {
         m_randUnif.SetSeed( m_resetInfo.initialSeedRandUnif );
         m_randNormal.SetSeed( m_resetInfo.initialSeedRandNormal );
         }

      // note:  Data is deleted in InitRun();

#ifdef _DEBUG
      // ----- compare data from STARTLULC to LULC_C -----------
      int levels = m_lulcTree.GetLevels();
      int colLulc = -1;
      switch( levels )
         {
         case 1:  colLulc = m_colLulcA;   break;
         case 2:  colLulc = m_colLulcB;   break;
         case 3:  colLulc = m_colLulcC;   break;
         case 4:  colLulc = m_colLulcD;   break;
         }

      if ( colLulc >= 0 )
         {
         ASSERT( m_colStartLulc >= 0 );
         int totalCells = m_pIDULayer->GetRecordCount( MapLayer::ALL );
         for ( int i=0; i < totalCells; i++ )
            {
            int lulc;
            int startLulc;
            m_pIDULayer->GetData( i, m_colStartLulc, startLulc );
            m_pIDULayer->GetData( i, colLulc, lulc );
            ASSERT( lulc == startLulc );
            }
         }
#endif

      // Clean up
      m_envContext.pEnvExtension = NULL; 
      m_envContext.id  = -1;
      m_envContext.handle = -1;
      m_envContext.col = -1;
      m_envContext.firstUnseenDelta = -1;
      m_envContext.lastUnseenDelta = -1;
      
      m_currentYear = m_startYear;
      m_endYear = m_startYear + m_yearsToRun;
      }

   // ----- Prepare for the next run -----------------------------------------

   /*
   Report::StatusMsg( "Creating data objects..." );
   m_pDataManager->CreateDataObjects( this );   // note: this triggers a call to the results tab to create a run 
   Report::StatusMsg( "Creating data objects...completed" );
   m_currentRun++;  */
   m_runStatus  = RS_PRERUN; 
   // ------------------------------------------------------------------------

   return true;
   }



//-------------------------------------------------------------------
// RunEvaluation()
//
// This method runs the evalative models used by ENVISION.  Note that
//   the evaluative models should return an "abundance level" between
//   -3 and 3
//-------------------------------------------------------------------

bool EnvModel::RunEvaluation( void )
   {
#ifndef NO_MFC
   if ( m_runParallel )
      return RunEvaluationParallel();
#endif

   int scaleError;
   static bool reportErrors = true;  
   Notify( EMNT_RUNEVAL, 0 );

   int modelCount = GetEvaluatorCount();
   // int iCPU = omp_get_num_procs();
   // omp_set_num_threads(iCPU);
   // #pragma omp parallel for
   for ( int i=0; i < modelCount; i++ )
      {
      EnvEvaluator *pInfo = m_evaluatorArray[ i ];
      if ( pInfo->m_use )
         {
         // call the function (eval models should return scores in the range of -3 to +3)
         //???Check
         //if ( m_showRunProgress )
         //   gpMain->SetModelMsg( pInfo->m_name );
         Notify( EMNT_RUNEVAL, 1, (INT_PTR) pInfo );

         m_envContext.pEnvExtension  = pInfo;
         //m_envContext.col    = pInfo->col;
         m_envContext.id     = pInfo->m_id;
         m_envContext.handle = pInfo->m_handle;
         m_envContext.score  = 0.0f;  // CFM - This should not be necessary. models should overwrite
         m_envContext.rawScore = 0.0f;
         m_envContext.pDataObj = NULL;
         m_envContext.firstUnseenDelta = m_emFirstUnseenDelta[i];

         //UpdateUI( 8, (LONG_PTR) m_envContext.pEnvExtension );   // extension name
         //Notify( EMNT_UPDATEUI, 8, (INT_PTR) pInfo );

         clock_t start = clock();

         try
            {
            Report::indentLevel++;
            bool ok = pInfo->Run( &m_envContext );

            clock_t finish = clock();
            double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
            pInfo->m_runTime += (float) duration;         

            Notify( EMNT_RUNEVAL, 2, (INT_PTR) pInfo );
            Report::indentLevel--;

            if ( ! ok )
               {
               CString msg = "The ";
               msg += pInfo->m_name;
               msg += " Evaluative Model returned FALSE during Run(), indicating an error.";
               throw new EnvRuntimeException( msg );
               }
            }
         catch( ... )
            { }

         ApplyDeltaArray( m_pIDULayer );
         m_emFirstUnseenDelta[i] = m_pDeltaArray->GetCount();

         float score = m_envContext.score;
         // koch test
         scaleError = pInfo->ScaleScore( score ); // score passed by reference to this const member fnc

         if ( scaleError && m_showMessages && reportErrors )
            {
            CString msg = "The ";
            msg += pInfo->m_name;
            msg += " evaluation has returned a value that is out of bounds.  ENVISION can continue but this probably means you should fix this evaluator.  Do you want to keep reporting warnings in the evaluators?";

            Report::LogWarning(msg);
            }

         m_landscapeMetrics[i].scaled = pInfo->m_score     = score;
         m_landscapeMetrics[i].raw    = pInfo->m_rawScore  = m_envContext.rawScore;
         pInfo->m_pDataObj = m_envContext.pDataObj;
         }  // end of:  if ( pInfo->m_use )
      else
         {
         m_landscapeMetrics[i].scaled = pInfo->m_score    = 0;
         m_landscapeMetrics[i].raw    = pInfo->m_rawScore = 0;
         pInfo->m_pDataObj = NULL;
         }
      }  // end of:  for( i < modelCount )

   Notify( EMNT_RUNEVAL, 3 );
   return true;
   }


//-------------------------------------------------------------------
// RunEvaluationParallel()
//
// This method runs the evalative models used by ENVISION.  Note that
//   the evaluative models should return an "abundance level" between
//   -3 and 3
//-------------------------------------------------------------------

#ifndef NO_MFC
bool EnvModel::RunEvaluationParallel( void )
{	
	PtrArray< EnvProcess> evalModelInfoArray(false);
	for( int i = 0; i < m_evaluatorArray.GetSize(); i++)
		evalModelInfoArray.Add( m_evaluatorArray.GetAt(i) );

   EnvProcessScheduler::init( evalModelInfoArray );
   int scaleError;
   static bool reportErrors = true;  

   int modelCount = GetEvaluatorCount();
   int i = 0;
   while ( i < modelCount )
      {
      PtrArray<EnvProcess> runnableModels(false);
      EnvProcessScheduler::getRunnableProcesses( evalModelInfoArray, runnableModels );
      while( runnableModels.GetSize() > 0 )
         {
         ProcessWinMsg();
         if( EnvProcessScheduler::countActiveThreads() < EnvProcessScheduler::m_maxThreadNum )
            {
            EnvEvaluator *pInfo = dynamic_cast<EnvEvaluator*>( runnableModels.GetAt(0) );//m_evaluatorArray[ i ];
            runnableModels.RemoveAt(0);
            if( pInfo )
               {
               i++;
               ThreadDataMapIterator itr;

               CSingleLock lock( &EnvProcessScheduler::m_criticalSection );
               lock.Lock();
               itr = EnvProcessScheduler::m_ThreadDataMap.find( pInfo );
               if( itr != EnvProcessScheduler::m_ThreadDataMap.end() )
                  {
                  itr->second.threadState = EnvProcessScheduler::FINISHED;

                  }
               lock.Unlock();

               if ( pInfo->m_use )
                  {
                  // call the function (eval models should return scores in the range of -3 to +3)
                  //if ( m_showRunProgress )
                  //	gpMain->SetModelMsg( pInfo->m_name );
                  EnvContext* pEnvContext = new EnvContext( m_pIDULayer );
                  m_envContext.pEnvExtension  = pInfo;
                  //m_envContext.col    = pInfo->col;
                  m_envContext.id     = pInfo->m_id;
                  m_envContext.handle = pInfo->m_handle;
                  m_envContext.score  = 0.0f;  // CFM - This should not be necessary. models should overwrite
                  m_envContext.rawScore = 0.0f;
                  m_envContext.pDataObj = NULL;
                  m_envContext.firstUnseenDelta = 0;//m_emFirstUnseenDelta[i];
                  m_envContext.lastUnseenDelta = m_envContext.pDeltaArray->GetSize();
                  *pEnvContext = m_envContext;
                  //pEnvContext->pDeltaArray = new DeltaArray( m_pIDULayer );

                  CSingleLock lock( &EnvProcessScheduler::m_criticalSection );
                  lock.Lock();
                  itr = EnvProcessScheduler::m_ThreadDataMap.find( pInfo );
                  if( itr != EnvProcessScheduler::m_ThreadDataMap.end() )
                     {
                     itr->second.threadState = EnvProcessScheduler::RUNNING;
                     itr->second.pEnvContext = pEnvContext;

                     }
                  lock.Unlock();

                  clock_t start = clock();

                  AfxBeginThread( EnvProcessScheduler::workerThreadProc, pEnvContext );
                  //bool ok = pInfo->runFn( &m_envContext );

                  //clock_t finish = clock();
                  //double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
                  //pInfo->m_runTime += (float) duration;         

                  //if ( m_showRunProgress )
                  //{
                  //	CString msg( pInfo->m_name );
                  //	msg += " - Completed";
                  //	gpMain->SetModelMsg( msg );
                  //}

                  //if ( ! ok )
                  //{
                  //	CString msg = "The ";
                  //	msg += pInfo->m_name;
                  //	msg += " Evaluative Model returned FALSE indicating an error.";
                  //	throw new EnvRuntimeException( msg );
                  //}

                  //ApplyDeltaArray( m_pIDULayer );
                  //m_emFirstUnseenDelta[i] = m_pDeltaArray->GetCount();

                  //float score = m_envContext.score;
                  //scaleError = pInfo->ScaleScore( score ); // score passed by reference to this const member fnc

                  //if ( scaleError && m_showMessages && reportErrors )
                  //{
                  //	CString msg = "The ";
                  //	msg += pInfo->m_name;
                  //	msg += " evaluation has returned a value that is out of bounds.  ENVISION can continue but this probably means you should fix this evaluator.  Do you want to keep reporting warnings in the evaluators?";

                  //	if ( Report::LogWarning( msg, MB_YESNO ) == IDNO )
                  //		reportErrors = false;
                  //}

                  //m_landscapeMetrics[i].scaled = pInfo->m_score     = score;
                  //m_landscapeMetrics[i].raw    = pInfo->m_rawScore  = m_envContext.rawScore;
                  //pInfo->m_pDataObj = m_envContext.pDataObj;
                  }  // end of:  if ( pInfo->m_use )
               //else
               //{
               //	m_landscapeMetrics[i].scaled = pInfo->m_score    = 0;
               //	m_landscapeMetrics[i].raw    = pInfo->m_rawScore = 0;
               //	pInfo->m_pDataObj = NULL;
               //}

               }
            }
         }//while( runnableModels.GetSize() > 0 )

      // wait until all threads finish their tasks.
      while( EnvProcessScheduler::countActiveThreads() > 0 )
	  {
		  ProcessWinMsg();
	  }

      // copy delta array of each thread to global delta array
      // reset model info with data in thread
      ThreadDataMapIterator itr = EnvProcessScheduler::m_ThreadDataMap.begin();
      ThreadDataMapIterator itrEnd = EnvProcessScheduler::m_ThreadDataMap.end();
      //for(; itr != itrEnd; itr++ )
      //   {
      //   if( itr->second.pEnvContext != NULL )
      //      {
      //      for( INT_PTR cnt = 0; cnt < itr->second.pEnvContext->pDeltaArray->GetSize(); cnt++ )
      //         {
      //         DELTA& delta = m_pDeltaArray->GetAt(cnt);
      //         m_pDeltaArray->AddDelta(delta.cell, delta.year, delta.year, delta.newValue, delta.type );
      //         }
      //      }
      //   }

      ApplyDeltaArray( m_pIDULayer );

      itr = EnvProcessScheduler::m_ThreadDataMap.begin();
      itrEnd = EnvProcessScheduler::m_ThreadDataMap.end();
      for( ; itr != itrEnd; itr++ )
         //for( size_t k = 0; k < EnvProcessScheduler::m_ThreadDataList.size(); k++ )
         {
         EnvEvaluator* pInfo = NULL;
         int t = -1;
         for( int cnt = 0; cnt < modelCount; cnt++ )
            {
            if( itr->second.id == m_evaluatorArray[cnt] )
               // is this id unique in the list??
               {
               t = cnt;
               pInfo = m_evaluatorArray[cnt];
               break;
               }
            }
         if( pInfo && t >= 0 )
            {	
            if( itr->second.pEnvContext != NULL )
               {
               EnvContext* pEnvContext = itr->second.pEnvContext;
               //m_emFirstUnseenDelta[i] = m_pDeltaArray->GetCount();

               float score = pEnvContext->score;
               scaleError = pInfo->ScaleScore( score ); // score passed by reference to this const member fnc

               if ( scaleError && m_showMessages && reportErrors )
                  {
                  CString msg = "The ";
                  msg += pInfo->m_name;
                  msg += " evaluation has returned a value that is out of bounds.  ENVISION can continue but this probably means you should fix this evaluator.  Do you want to keep reporting warnings in the evaluators?";

                  Report::LogWarning(msg);
                  }

               m_landscapeMetrics[t].scaled = pInfo->m_score     = score;
               m_landscapeMetrics[t].raw    = pInfo->m_rawScore  = pEnvContext->rawScore;
               pInfo->m_pDataObj = pEnvContext->pDataObj;
               /////////////////////////////////////

               //delete itr->second.pEnvContext->pDeltaArray;
               delete itr->second.pEnvContext;
               itr->second.pEnvContext = NULL;

               }
            else if( itr->second.threadState == EnvProcessScheduler::FINISHED )
               {				
               m_landscapeMetrics[t].scaled = pInfo->m_score    = 0;
               m_landscapeMetrics[t].raw    = pInfo->m_rawScore = 0;
               pInfo->m_pDataObj = NULL;
               }
            }			
         }

      }  // end of:  while( i < modelCount )

   return true;
   }
#endif


///////////////////////////////////////////////////////////////////////////////////////////
void EnvModel::RunModelProcesses( bool isPostYear )
   {
#ifndef NO_MFC
   if ( m_runParallel )
      return RunModelProcessesParallel( isPostYear );
#endif

   int count = (int) m_modelProcessArray.GetSize();
   int timing = isPostYear ? 1 : 0;

   Notify( EMNT_RUNAP, 0 );

   for ( INT_PTR i=0; i < count; i++ )
      {
      EnvModelProcess *pInfo = m_modelProcessArray[ i ];
      if ( pInfo->m_use && pInfo->m_timing == timing )
         {
         m_envContext.pEnvExtension  = static_cast<EnvProcess*>(pInfo);
         m_envContext.id = pInfo->m_id;
         m_envContext.handle = pInfo->m_handle;
         //m_envContext.col = pInfo->col;
         m_envContext.firstUnseenDelta = m_apFirstUnseenDelta[i];
         m_envContext.lastUnseenDelta = m_envContext.pDeltaArray->GetSize();

         Notify( EMNT_RUNAP, 1, (INT_PTR) pInfo );

         //UpdateUI( 8, (LONG_PTR) m_envContext.pEnvExtension );   // extension info
         //Notify( EMNT_UPDATEUI, 8, (INT_PTR) m_envContext.pEnvExtension );

         clock_t start = clock();

         try
            {
            CString msg;
            msg.Format("%s starting...", (LPCTSTR)pInfo->m_name);
            Report::Log(msg);
            Report::indentLevel++;

            bool ok = pInfo->Run( &m_envContext );

            clock_t finish = clock();
            double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
            pInfo->m_runTime += (float) duration;
            Report::indentLevel--;


            if ( !ok )
               {
               CString msg = "The ";
               msg += pInfo->m_name;
               msg += " model returned FALSE during Run(), indicating an error.";
               throw new EnvRuntimeException( msg );
               }

            else 
               {
               msg.Format("  %s completed successfully (%.1f seconds)", (LPCTSTR)pInfo->m_name, (float) duration);
               Report::Log(msg);
               }
            }
         catch( ... )
             { }

         ApplyDeltaArray( m_pIDULayer );
         m_apFirstUnseenDelta[i] = m_pDeltaArray->GetCount();

         Notify( EMNT_RUNAP, 2, (INT_PTR) pInfo );
         }
      }

   Notify( EMNT_RUNAP, 3 );
   }



///////////////////////////////////////////////////////////////////////////////////////////
#ifndef NO_MFC
void EnvModel::RunModelProcessesParallel( bool isPostYear )
   {
   PtrArray< EnvProcess> apInfoArray(false);
   for( int i = 0; i < m_modelProcessArray.GetSize(); i++)
      apInfoArray.Add( m_modelProcessArray.GetAt(i) );

   EnvProcessScheduler::init( apInfoArray );

   int count = (int) m_modelProcessArray.GetSize();
   int timing = isPostYear ? 1 : 0;
   char msg[ 256 ];
   memset( msg, 0, 256 );
   int i = 0;
   while( i < count )
      {
      PtrArray<EnvProcess> runnableProcesses(false);
      EnvProcessScheduler::getRunnableProcesses( apInfoArray, runnableProcesses );
      while( runnableProcesses.GetSize() > 0 )
         {
			 ProcessWinMsg();
         if(  EnvProcessScheduler::countActiveThreads() < EnvProcessScheduler::m_maxThreadNum )
            {
            EnvModelProcess* pInfo = dynamic_cast<EnvModelProcess*>(runnableProcesses.GetAt(0));
            runnableProcesses.RemoveAt( 0 );
            if( pInfo )
               {
               i++;
               CSingleLock lock( &EnvProcessScheduler::m_criticalSection );
               lock.Lock();
               ThreadDataMapIterator itr = EnvProcessScheduler::m_ThreadDataMap.find( pInfo );
               if( itr != EnvProcessScheduler::m_ThreadDataMap.end() )
                  {
                  itr->second.threadState = EnvProcessScheduler::FINISHED;
                  }
               lock.Unlock();					

               if ( pInfo->m_use && pInfo->m_timing == timing )
                  {
                  EnvContext* pEnvContext = new EnvContext( m_pIDULayer );
                  m_envContext.pEnvExtension  = static_cast<EnvProcess*>(pInfo);
                  m_envContext.id = pInfo->m_id;
                  m_envContext.handle = pInfo->m_handle;
                  //m_envContext.col = pInfo->col;
                  m_envContext.firstUnseenDelta = 0;//m_apFirstUnseenDelta[i];
                  m_envContext.lastUnseenDelta = m_envContext.pDeltaArray->GetSize();
                  *pEnvContext = m_envContext;
                  //pEnvContext->pDeltaArray = new DeltaArray( m_pIDULayer );

                  CSingleLock lock( &EnvProcessScheduler::m_criticalSection );
                  lock.Lock();
                  ThreadDataMapIterator itr = EnvProcessScheduler::m_ThreadDataMap.find( pInfo );
                  if( itr != EnvProcessScheduler::m_ThreadDataMap.end() )
                     {
                     itr->second.threadState = EnvProcessScheduler::RUNNING;
                     itr->second.pEnvContext = pEnvContext;
                     }
                  lock.Unlock();		

                  /*if ( m_showRunProgress )
                  {
                  lstrcpy( msg, pInfo->m_name );
                  if ( postYear )
                  lstrcat( msg, " (post)" );
                  else
                  lstrcat( msg, " (pre)" );

                  gpMain->SetModelMsg( msg );
                  }*/

                  clock_t start = clock();

                  AfxBeginThread( EnvProcessScheduler::workerThreadProc, pEnvContext );
                  //bool ok = pInfo->runFn( &m_envContext );

                  //clock_t finish = clock();
                  //double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
                  //pInfo->m_runTime += (float) duration;         

                  //if ( !ok )
                  //{
                  //	CString msg = "The ";
                  //	msg += pInfo->m_name;
                  //	msg += " Autonomous Process returned FALSE indicating an error.";
                  //	throw new EnvRuntimeException( msg );
                  //}

                  //ApplyDeltaArray( m_pIDULayer );
                  //m_apFirstUnseenDelta[i] = m_pDeltaArray->GetCount();

                  if ( m_showRunProgress )
                     {
                     lstrcat( msg, " - Completed" );
                     //???Check
                     //gpMain->SetModelMsg( msg );
                     }
                  }
               }
            }
         }// end while( runnableProcesses.GetSize() > 0 )

      // wait until all threads finish their tasks.
      while( EnvProcessScheduler::countActiveThreads() > 0 )
	  {
		  ProcessWinMsg();
	  }

      //copy delta array of each thread to global delta array
      ThreadDataMapIterator itr = EnvProcessScheduler::m_ThreadDataMap.begin();
      ThreadDataMapIterator itrEnd = EnvProcessScheduler::m_ThreadDataMap.end();
      for( ; itr != itrEnd; itr++ )
         {
         if( itr->second.pEnvContext != NULL )
            {
            for( INT_PTR cnt = 0; cnt < itr->second.pEnvContext->pDeltaArray->GetSize(); cnt++ )
               {
               //DELTA& delta = m_pDeltaArray->GetAt(cnt);
               //m_pDeltaArray->AddDelta(delta.cell, delta.year, delta.year, delta.newValue, delta.type );
               }
            //delete itr->second.pEnvContext->pDeltaArray;
            delete itr->second.pEnvContext;
            itr->second.pEnvContext = NULL;
            }
         }
      ApplyDeltaArray( m_pIDULayer );
      } // end while( i < count )
   }

#endif

void EnvModel::RunVisualizers( bool isPostYear )
   {
   /*
   // Note: isPostYear is ALWAYS true for now.
   int count = (int) m_vizInfoArray.GetSize();
   int timing = isPostYear ? 1 : 0;
   CString msg( "Running Visualizers " );

   for ( INT_PTR i=0; i < count; i++ )
      {
      ENV_VISUALIZER *pInfo = m_vizInfoArray[ i ];
      if ( pInfo->m_use && pInfo->runFn != NULL )
         {
         m_envContext.pEnvExtension  = static_cast<ENV_VISUALIZER*>(pInfo);
         m_envContext.id = pInfo->m_id;
         m_envContext.handle = pInfo->m_handle;
         m_envContext.col = -1;
         m_envContext.firstUnseenDelta = m_vizFirstUnseenDelta[i];
         m_envContext.lastUnseenDelta = m_envContext.pDeltaArray->GetSize();
         
         Notify( EMNT_RUNVIZ, 1, (INT_PTR) pInfo );

         clock_t start = clock();

         bool ok = pInfo->runFn( &m_envContext );

         Notify( EMNT_RUNVIZ, 2, (INT_PTR) pInfo );

         clock_t finish = clock();
         double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
         pInfo->m_runTime += (float) duration;         

         if ( !ok )
            {
            CString msg = "The ";
            msg += pInfo->m_name;
            msg += " Visualizer returned FALSE indicating an error.";
            throw new EnvRuntimeException( msg );
            }

         ApplyDeltaArray( m_pIDULayer );   // apply any deltas generated by visualizer
         m_vizFirstUnseenDelta[i] = m_pDeltaArray->GetCount();
         }
      }

    // update windows if RT Visualizer
    //???Check
    //count = gpView->m_vizManager.GetVisualizerCount();
    //
    //for ( int i=0; i < count; i++ )
    //   {
    //   VisualizerWnd *pVizWnd = gpView->m_vizManager.GetVisualizerWnd( i );
    //
    //   ENV_VISUALIZER *pInfo = pVizWnd->m_pVizInfo;
    //   
    //   if ( pVizWnd->m_envContext.run == m_currentRun    // vizualizer using this run?
    //     && pInfo->m_use                                   // in use? 
    //     && ( pInfo->type & VT_RUNTIME )                 // its is a runtime visualizer?
    //     && pInfo->updateWndFn != NULL )                 // with an UpdateWindow()?
    //      {
    //      m_envContext.pEnvExtension  = static_cast<ENV_VISUALIZER*>(pInfo);
    //      m_envContext.id     = pInfo->m_id;
    //      m_envContext.handle = pInfo->m_handle;
    //      m_envContext.col    = -1;
    //      m_envContext.firstUnseenDelta = m_vizFirstUnseenDelta[i];
    //      m_envContext.lastUnseenDelta = m_envContext.pDeltaArray->GetSize();
    //      m_envContext.pWnd = pVizWnd;
    //
    //      if ( m_showRunProgress )
    //         {
    //         msg = pInfo->m_name;
    //         if ( isPostYear )
    //            msg += " (post)";
    //         else
    //            msg += " (pre)";
    //
    //         gpMain->SetModelMsg( msg );
    //         }
    //
    //      clock_t start = clock();
    //
    //      bool ok = pInfo->updateWndFn( &m_envContext, pVizWnd->GetSafeHwnd() );
    //
    //      clock_t finish = clock();
    //      double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
    //      pInfo->m_runTime += (float) duration;         
    //
    //      if ( !ok )
    //         {
    //         CString msg = "The ";
    //         msg += pInfo->m_name;
    //         msg += " Visualizer returned FALSE indicating an error.";
    //         throw new EnvRuntimeException( msg );
    //         }
    //
    //      //ApplyDeltaArray( m_pIDULayer );   // apply any deltas generated by visualizer
    //      //m_vizFirstUnseenDelta[i] = m_pDeltaArray->GetCount();
    //      }
    //   }
    */
    Notify( EMNT_RUNVIZ, 3 );
    }


//-----------------------------------------------------------------------------------------------------
// Delta Array interface for EnvModel.
//
// Basic idea - EnvModel is responsible for creating, adding, applying, and unapplying the delta array
//
// CreateDeltaArray( MapLayer* ) - make a delta array based on the passed in map layer
//
// AddDelta( int cell, int col, int year, VData newValue, DELTA_TYPE type ) - Adds a delta to the model's current DeltaArray
//
// ApplyDeltaArray( DeltaArray *pDeltaArray , int start, int end  )
//   - Starts at "start" in the current delta array and iterates through "end" ]
// updating the cell database with the specified field and value.
//
// ApplyDelta( MapLayer *pLayer, DELTA &delta ) - applies the given delta to the given layer
//
// UnApplyDeltaArray( MapLayer *pLayer, DeltaArray *pDeltaArray /*=NULL*/, int _start /*=-1 -> unapply the whole thing*/ ) const
//   - Unapply the deltaArray to the given MapLayer
//-------------------------------------------------------------------------------------------------


bool EnvModel::CreateDeltaArray( MapLayer *pLayer )
   {
   ASSERT( pLayer );
   DeltaArray *pDeltaArray = new DeltaArray( pLayer, m_startYear );

   if ( m_pDeltaArray )
      pDeltaArray->m_showDeltaMessages = m_pDeltaArray->m_showDeltaMessages;
   else
      {
      //???Check
      //if ( gpDoc->m_debug )
      //   pDeltaArray->m_showDeltaMessages = true;
      //else
      //   pDeltaArray->m_showDeltaMessages = false;
      }

   ASSERT( m_pDataManager );
   if ( m_pDataManager == NULL )
      return false;

   m_pDataManager->m_pDeltaArray = pDeltaArray;
   m_pDataManager->AddDeltaArray( pDeltaArray );

   m_pDeltaArray = pDeltaArray;

   return true;
   }

//---------------------------------------------------------------------------------------
//#define __SYNCHRONOUS__DELTA__
//------------------------------

//--EnvModel::AddDelta-------------------------------------------------------------------------
//    Interface to DeltaArray::AddDelta.  Interprets return value and propagates delta when appropriate.
//
//    --PARAMETERS--
//    cell     = map cell (IDU)
//    col      = column in map database to be changed
//    year     = calendar year in which change occurred
//    newValue = VData object containing value to be applied
//    type     = type of delta indicates where delta originated (see delta.h)
//
//    --RETURN VALUES--
//    error, delta was not added    -->  -1
//    error, redundant delta        -->  -2
//    no issues, delta was added    -->  new size of delta array, propagation included
//-----------------------------------------------------------------------------------------------
INT_PTR EnvModel::AddDelta( int cell, int col, int year, VData newValue, int type )
   {
   INT_PTR index = m_pDeltaArray->AddDelta( cell, col, year, newValue, type );

   // was this delta added?
   if ( index >= 0 )
      { 
      // PROPAGATE hierachically
      //      if ( type == DT_POLICY && (col == m_colLulcA || col == m_colLulcB || col == m_colLulcC) )
      if ( type != DT_POP && (col == m_colLulcA || col == m_colLulcB || col == m_colLulcC || col == m_colLulcD) )
         {
         DELTA  &delta = m_pDeltaArray->GetAt( index );
         ASSERT( delta.type != DT_POP );
         POP(delta);
         }
#ifdef __SYNCHRONOUS__DELTA__
      ApplyDeltaArray();
#endif
      }

   return index;
   }


//--EnvModel::ApplyDeltaArray---------------------------------------------------------------------
//    Iterates through and applies all or a subset of a passed delta array against the passed map layer.
//    This function also sets the oldValue for non-subdivide, non-merge deltas; therefore, until
//    ApplyDeltaArray has been executed during a run, old value will be NULL
//
//    --PARAMETERS--
//    pLayer         = pointer to map layer to be changed
//    pDeltaArray    = pointer to the delta array to be applied; if NULL, use current delta array of EnvModel
//    _start         = index of first delta to apply; if -1, uses the first unapplied of delta array
//    _end           = index of last delta to apply; if -1, uses the count of elements in the delta array (i.e. apply all)
//
//    --RETURN VALUES--
//    always     -->  count of changes applied
//-----------------------------------------------------------------------------------------------

int EnvModel::ApplyDeltaArray( MapLayer *pLayer, DeltaArray *pDeltaArray /*=NULL*/ )
   {
   if ( pDeltaArray == NULL )
      pDeltaArray = m_pDeltaArray;

   if ( pDeltaArray == NULL )
      return 0;

   if ( pDeltaArray->GetSize() == 0 )  // nothing to apply?
      return 0;

   INT_PTR start = pDeltaArray->GetFirstUnapplied( pLayer );
   INT_PTR end   = pDeltaArray->GetCount() - 1;

   return ApplyDeltaArray( pLayer, pDeltaArray, start, end );
   }


int EnvModel::ApplyDeltaArray( MapLayer *pLayer, DeltaArray *pDeltaArray, INT_PTR start, INT_PTR end  )
   {
   if ( pDeltaArray == NULL )
      pDeltaArray = m_pDeltaArray;

   if ( pDeltaArray == NULL )
      return 0;

   if ( pDeltaArray->GetSize() == 0 )  // nothing to apply?
      return 0;

   INT_PTR size = end+1;

   ENV_ASSERT( pLayer != NULL );
   //ENV_ASSERT( pLayer == pDeltaArray->m_pIDULayer );
   ENV_ASSERT( start <= size );

   bool readOnly = pLayer->m_readOnly;
   pLayer->m_readOnly = false;

   int changeCount = 0;

   if ( start == size )  
      return changeCount;

   for ( INT_PTR i=start; i < size; i++ )
      {
      DELTA &delta = pDeltaArray->GetAt(i);

      if ( pLayer->IsDefunct( delta.cell ) )
         {
         m_pDeltaArray->RemoveAt( i );
         size--;
         i--;
         continue;
         }

      if ( delta.col >= 0 )
         {
         pLayer->GetData( delta.cell, delta.col, delta.oldValue );      // sets delta.oldValue here.  WHY????? jpb 7/5/06

         // check for redundant deltas  REMOVED 7/6/06 - slows everything down too much, better to just let the redundant one get applied
         //if ( delta.oldValue == delta.newValue )
         //   {
         //   pDeltaArray->RemoveAt( i );
         //   size--;
         //   i--;
         //   continue;
         //   }
         }

      changeCount += ApplyDelta( pLayer, delta );
      }

   pDeltaArray->SetFirstUnapplied( pLayer, SFU_END );       // set first unapplied member to "count" - looks fully applied

   pLayer->m_readOnly = readOnly;

   return changeCount;
   }


//--EnvModel::ApplyDelta--------------------------------------------------------------------------
//    Applies the passed delta to the layer pointed to by pLayer.  During a run, the delta array is 
//    applied via EnvModel::ApplyDeltaArray; however, this function is called directly by analysis
//    functions to apply one delta at a time.
//
//    --PARAMETERS--
//    pLayer   = pointer to map layer to be changed
//    delta    = delta to be applied, passed by reference
//
//    --RETURN VALUES--
//    (the return value is used to increment the change count in ApplyDeltaArray)
//    delta was NOT applied   -->  0
//    delta WAS applied       -->  1
//-----------------------------------------------------------------------------------------------
int EnvModel::ApplyDelta( MapLayer *pLayer, DELTA &delta )
   {
   // no change?
   if ( delta.oldValue == delta.newValue )
      return 1;

   // Normal Delta
   if ( delta.col >= 0 && delta.cell >= 0 )
      {
      pLayer->SetData( delta.cell, delta.col, delta.newValue );
      return 1;
      }

   // SubDivide
   else if ( delta.type == DT_SUBDIVIDE ) //.col == DELTA::SUBDIVIDE )
      {
      int fieldCount = m_pIDULayer->GetFieldCount();
      VData value;
      int parent = delta.cell;
      PolyArray *pChildArray = (PolyArray*) delta.newValue.val.vPtr;

      // subdivide the map; side-effect is to make the Parent DEFUNCT
      pLayer->Subdivide( parent, *pChildArray );

      // Get the actor and remove the parent cell
      int actor;
      Actor *pActor = NULL;
      pLayer->GetData( parent, m_colActor, actor );
      if ( 0 <= actor && actor < m_pActorManager->GetActorCount() )
         {
         pActor = m_pActorManager->GetActor( actor );
         pActor->RemovePoly( parent );
         }

      int childCount    = (int) pChildArray->GetCount();
      Poly *pParentPoly = pLayer->GetPolygon( parent );
      ASSERT( childCount == pParentPoly->GetChildCount() );
      ASSERT( pLayer->IsDefunct( parent ) ); // This cell should be off

      for ( int child=0; child<childCount; child++ )
         {
         // Get the poly
         Poly *pPoly = pLayer->GetChildPolygon( pParentPoly, child );
         ASSERT( pChildArray->GetAt( child )->m_id == pPoly->m_id );

         // copy attributes of parent cell to all child cells
         for ( int i=0; i<fieldCount; i++ )
            {
            if ( i == m_colArea )
               continue;

            ASSERT( pPoly->m_id >= 0 );
            pLayer->GetData( parent, i, value );
            pLayer->SetData( pPoly->m_id, i, value );
            }

         // Give this cell to the Actor that owns the parent
         if ( pActor )
            {
            Actor *pActor = m_pActorManager->GetActor( actor );
            pActor->AddPoly( pPoly->m_id );
            }

         ASSERT( pLayer->IsDefunct( pPoly->m_id ) == false );  // This cell should be on
         pLayer->ClassifyPoly( -1, pPoly->m_id );    
         }

      // Update the Spatial Index
      pLayer->SubdivideSpatialIndex( parent );

      return 1;
      }

   // Merge
   else if ( delta.type == DT_MERGE ) //.col == DELTA::MERGE )
      {
      ASSERT(0);
      return 0;
      }

   // Increment Col
   else if ( delta.type == DT_INCREMENT_COL  ) //cell == DELTA::INCREMENT_COL )
      {
      for ( MapLayer::Iterator i=pLayer->Begin(); i != pLayer->End(); ++i )
         {
         VData value;
         pLayer->GetData( i, delta.col, value );
         switch( value.type )
            {
            case TYPE_SHORT:
            case TYPE_LONG:
            case TYPE_INT:
               {
               int iValue;
               value.GetAsInt( iValue );
               int incr;
               if ( delta.newValue.GetAsInt( incr ) )
                  iValue += incr;
               pLayer->SetData( i, delta.col, iValue );
               }
               break;

            case TYPE_UINT:
            case TYPE_ULONG:
               {
               UINT iValue;
               value.GetAsUInt( iValue );
               int incr;
               if ( delta.newValue.GetAsInt( incr ) )
                  iValue += incr;
               pLayer->SetData( i, delta.col, VData( iValue ) );
               }
               break;

            case TYPE_FLOAT:
            case TYPE_DOUBLE:
               {
               double dValue;
               value.GetAsDouble( dValue );
               float incr;
               if ( delta.newValue.GetAsFloat( incr ) )
                  dValue += incr;
               if ( value.type == TYPE_FLOAT )
                  pLayer->SetData( i, delta.col, (float) dValue );
               else
                  pLayer->SetData( i, delta.col, dValue );
               }
               break;
            }
         }
      }

   // DELTA of unknown species
   else
      {
      ASSERT(0);
      }

   return 0;
   }


//--EnvModel::UnApplyDeltaArray---------------------------------------------------------------------
//    Iterates through and UNapplies all deltas in the passed delta array against the passed map layer.
//    The function begins at a particular starting point and moves backward to the beginning of the delta array
//
//    --PARAMETERS--
//    pLayer         = pointer to map layer to be changed
//    pDeltaArray    = pointer to the delta array to be applied; if NULL, use current delta array of EnvModel
//    _start         = index at which to begin unapplying deltas; if -1 (or larger than the delta array), starts at firstUnapplied - 1
//
//    --RETURN VALUES--
//    (returns state of delta array before unapplying deltas)
//    no deltas existed in the delta array                  --> -1
//    deltas had been added, but had not been applied       -->  0
//    deltas had been applied up to index n-1               -->  n>0
//-----------------------------------------------------------------------------------------------
INT_PTR EnvModel::UnApplyDeltaArray(MapLayer *pLayer, DeltaArray *pDeltaArray )
   {
   if ( pDeltaArray == NULL )       // if no deltaArray passed in, use the current EnvModel deltaArray
      pDeltaArray = m_pDeltaArray;

   if ( pDeltaArray == NULL )
      return -1;

   ENV_ASSERT( pLayer != NULL );
   //ENV_ASSERT( pLayer == pDeltaArray->m_pIDULayer );     // make sure we are operating on the same map the deltaArray was created with

   INT_PTR firstUnapplied = pDeltaArray->GetFirstUnapplied( pLayer );   // 0 means all unapplied (none applied), -1 means no deltas, >0 means partially or fully applied

   if ( firstUnapplied <= 0 )  // already completely unapplied, don't need to do anything else
      return firstUnapplied;

   INT_PTR start = firstUnapplied-1;    // start = last applied

   return UnApplyDeltaArray( pLayer, pDeltaArray, start );
   }

INT_PTR EnvModel::UnApplyDeltaArray( MapLayer *pLayer, DeltaArray *pDeltaArray, INT_PTR start ) const
   {
   if ( pDeltaArray == NULL )       // if no deltaArray passed in, use the current EnvModel deltaArray
      pDeltaArray = m_pDeltaArray;

   ENV_ASSERT( pDeltaArray != NULL );
   ENV_ASSERT( pLayer != NULL );
   //ENV_ASSERT( pLayer == pDeltaArray->m_pIDULayer );     // make sure we are operating on the same map the deltaArray was created with

   INT_PTR firstUnapplied = pDeltaArray->GetFirstUnapplied( pLayer );   // 0 means all unapplied (none applied), -1 means no deltas, >0 means partially or fully applied

   if ( firstUnapplied <= 0 )  // already completely unapplied, don't need to do anything else
      return firstUnapplied;

   bool readOnly = pLayer->m_readOnly;
   pLayer->m_readOnly = false;

   for ( INT_PTR i=start; i >= 0 ; i-- )
      {
      DELTA &delta = pDeltaArray->GetAt( i );

      if (delta.oldValue == delta.newValue)
         {
         CString trace;
         trace.Format( _T("Redundant delta found: col=%s, cell=%i, value=%s\n"), pLayer->GetFieldLabel( delta.col ), delta.cell, delta.newValue.GetAsString() );
         TRACE( trace );
         }

      // Normal Delta
      if ( delta.col >= 0 && delta.cell >= 0 )
         {
         VData value;
         pLayer->GetData( delta.cell, delta.col, value );

         if ( value.Compare( delta.newValue ) == false )
            { 
            CString msg;
            msg.Format( "The DeltaArray and the CellLayer are inconsistent while UnApplying delta array.  This error occured in cell %i, field %s, at year %i.  Cell layer value is %s, while the new delta value is %s.",
               delta.cell, pLayer->GetFieldLabel( delta.col ), delta.year, value.GetAsString(), delta.newValue.GetAsString() );
            //throw new EnvFatalException( msg );
            //ErrorMsg( msg );
            TRACE( msg ); //?????? jpb 9/20/10
            }
         else
            pLayer->SetData( delta.cell, delta.col, delta.oldValue );
         }

      // Subdivide
      else if ( delta.type == DT_SUBDIVIDE  ) //.col == DELTA::SUBDIVIDE )
         {
         int parent = delta.cell;
         Poly *pParent = pLayer->GetPolygon( parent );

         // Get the actor and add the parent cell
         int actor;
         Actor *pActor = NULL;
         pLayer->GetData( parent, m_colActor, actor );
         if ( 0 <= actor && actor < m_pActorManager->GetActorCount() )
            {
            pActor = m_pActorManager->GetActor( actor );
            pActor->AddPoly( parent );
            }

         int childCount = pParent->GetChildCount();

         CDWordArray  childIdList;  // subsequent use
         childIdList.SetSize(childCount);

         for ( int child=0; child<childCount; child++ )
            {
            // Get the poly
            int childIndex = pParent->GetChild( child );
            //Poly *pPoly = pLayer->GetPolygon( childIndex );

            childIdList.SetAt(child, childIndex /*pPoly->m_id */ ); // used later

            // Take this cell from the Actor that owns the parent
            if ( pActor )
               {
               Actor *pActor = m_pActorManager->GetActor( actor );
               pActor->RemovePoly( childIndex /*pPoly->m_id*/ );
               }

            // This cell should be off 
            //ASSERT( m_pIDULayer->IsDefunct( pPoly->m_id ) );
            }

         // unsubdivide the map
         pLayer->UnSubdivide( parent );
         // 

         // Update the Spatial Index
         // Here use the childId array because previous call to MapLayer::UnSubdivide has deleted the
         // children Poly.
         pLayer->UnSubdivideSpatialIndex( parent, childIdList );

         // This cell should be on
         ASSERT( pLayer->IsDefunct( pParent->m_id ) == false );
         }

      // Merge
      else if ( delta.type == DT_MERGE ) //.col == DELTA::MERGE )
         {
         ASSERT(0);
         }

      // Increment Col
      else if ( delta.type == DT_INCREMENT_COL ) //.cell == DELTA::INCREMENT_COL )
         {
         for ( MapLayer::Iterator i=pLayer->Begin(); i != pLayer->End(); ++i )
            {
            VData value;
            pLayer->GetData( i, delta.col, value );
            switch( value.type )
               {
               case TYPE_INT:
               case TYPE_SHORT:
               case TYPE_LONG:
                  {
                  int iValue;
                  value.GetAsInt( iValue );
                  int incr;
                  if ( delta.newValue.GetAsInt( incr ) )
                     iValue -= incr;
                  pLayer->SetData( i, delta.col, iValue );
                  }
                  break;

               case TYPE_UINT:
               case TYPE_ULONG:
                  {
                  UINT iValue;
                  value.GetAsUInt( iValue );
                  int incr;
                  if ( delta.newValue.GetAsInt( incr ) )
                     iValue -= incr;
                  pLayer->SetData( i, delta.col, VData( iValue ) );
                  }
                  break;

               case TYPE_FLOAT:
               case TYPE_DOUBLE:
                  {
                  double dValue;
                  value.GetAsDouble( dValue );
                  float incr;
                  if ( delta.newValue.GetAsFloat( incr ) )
                     dValue -= incr;
                  if ( value.type == TYPE_FLOAT )
                     pLayer->SetData( i, delta.col, (float) dValue );
                  else 
                     pLayer->SetData( i, delta.col, dValue );
                  }
                  break;
               }
            }
         }

      // DELTA of unknown species
      else
         {
         ASSERT(0);
         }
      }

   ////// TEMPORARY CODE UNTIL UnApplyDeltaArray()'s calls to SpatialINdex;:UnSubdivide are fixed
   ////if ( true == BufferOutcomeFunction::m_enabled )
   ////   m_pIDULayer->CreateSpatialIndex( NULL, 10000, 500, SIM_NEAREST );

   pDeltaArray->SetFirstUnapplied( pLayer, 0 );   // flag that everything has been unapplied

   pLayer->m_readOnly = readOnly;

   return firstUnapplied;                 // return previous firstUnapplied index
   }

//------------------------------------------------------------------------------
// End of DeltaArray functions
//------------------------------------------------------------------------------

void EnvModel::StoreIDUCols()
   {
   // get the columns in the databses to hold alternative information
   ENV_ASSERT( m_pIDULayer != NULL );

   m_colScore = m_pIDULayer->GetFieldCol( "Score" );
   if ( m_colScore < 0 )
      {
      Report::StatusMsg( "SCORE field not found - This field is being added...." );
      m_colScore = m_pIDULayer->m_pDbTable->AddField( "Score", TYPE_FLOAT, true );

      Notify( EMNT_IDUCHANGE, 0 );   // calls gpDoc->SetChanged( CHANGED_COVERAGE );
      }

   m_colArea = m_pIDULayer->GetFieldCol( "AREA" );  // required
   if ( m_colArea < 0 )
      {
      Report::StatusMsg( "AREA field not found - This field is being added...." );
      m_colArea = m_pIDULayer->m_pDbTable->AddField( "Area", TYPE_FLOAT, true );

      Notify( EMNT_IDUCHANGE, 1 );   // calls gpDoc->SetChanged( CHANGED_COVERAGE );

      }

   ENV_ASSERT( m_colArea >= 0 );

   if ( m_lulcTree.GetLevels() >= 1 )
      {
      m_colLulcA = m_pIDULayer->GetFieldCol( m_lulcTree.GetFieldName( 1 ) ); //"LULC_A" );
      if ( m_colLulcA < 0 )
         {
         CString msg;
         msg.Format( "LULC field specified in LULC heirarchy '%s' not found - This field is being added....", (LPCTSTR) m_lulcTree.GetFieldName( 1 )  );
         Report::StatusMsg( msg );
         m_colLulcA = m_pIDULayer->m_pDbTable->AddField( m_lulcTree.GetFieldName( 1 ), TYPE_INT, true );
         Notify( EMNT_IDUCHANGE, 0 );
         }
      }

   if ( m_lulcTree.GetLevels() >= 2 )
      {
      m_colLulcB = m_pIDULayer->GetFieldCol( m_lulcTree.GetFieldName( 2 ) ); //"LULC_B" );
      if ( m_colLulcB < 0 )
         {
         CString msg;
         msg.Format( "LULC field specified in LULC heirarchy '%s' not found - This field is being added....", (LPCTSTR) m_lulcTree.GetFieldName( 2 )  );
         Report::StatusMsg( msg );
         //int width, decimals;
         //GetTypeParams( TYPE_INT, width, decimals );
         m_colLulcB = m_pIDULayer->m_pDbTable->AddField( m_lulcTree.GetFieldName( 2 ), TYPE_INT, /*width, decimals,*/ true );
         Notify( EMNT_IDUCHANGE, 0 );
         }
      }

   if ( m_lulcTree.GetLevels() >= 3 )
      {
      m_colLulcC = m_pIDULayer->GetFieldCol( m_lulcTree.GetFieldName( 3 ) ); //"LULC_C" );  // required
      if ( m_colLulcC < 0 )
         {
         CString msg;
         msg.Format( "LULC field specified in LULC heirarchy '%s' not found - This field is being added....", (LPCTSTR) m_lulcTree.GetFieldName( 3 )  );
         Report::StatusMsg( msg );
         m_colLulcC = m_pIDULayer->m_pDbTable->AddField( m_lulcTree.GetFieldName( 3 ), TYPE_INT );
         Notify( EMNT_IDUCHANGE, 0 );
         }
      }

   if ( m_lulcTree.GetLevels() >= 4 )
      {
      m_colLulcD = m_pIDULayer->GetFieldCol( m_lulcTree.GetFieldName( 4 ) ); //"LULC_D" );  // required
      if ( m_colLulcD < 0 )
         {
         CString msg;
         msg.Format( "LULC field specified in LULC heirarchy '%s' not found - This field is being added....", (LPCTSTR) m_lulcTree.GetFieldName( 4 )  );
         Report::StatusMsg( msg );
         m_colLulcD = m_pIDULayer->m_pDbTable->AddField( m_lulcTree.GetFieldName( 4 ), TYPE_INT );
         Notify( EMNT_IDUCHANGE, 0 );
         }
      }

   m_colStartLulc = m_pIDULayer->GetFieldCol( "STARTLULC" );
   if ( m_colStartLulc < 0 )
      {
      Report::StatusMsg( "STARTLULC field not found - This field is being added...." );
      m_colStartLulc = m_pIDULayer->m_pDbTable->AddField( "STARTLULC", TYPE_INT );
      m_pIDULayer->CopyColData( m_colStartLulc, m_colLulcC );
      Notify( EMNT_IDUCHANGE, 0 );
      }

   m_colPolicy = m_pIDULayer->GetFieldCol( "POLICY" );  // Note: this is the policy offset, not policy ID
   if ( m_colPolicy < 0 )
      {
      Report::StatusMsg( "Policy field not found - This field is being added...." );
      m_colPolicy = m_pIDULayer->m_pDbTable->AddField( "Policy", TYPE_INT );
      Notify( EMNT_IDUCHANGE, 0 );
      }
   m_pIDULayer->SetColNoData( m_colPolicy );

   m_colPolicyApps = m_pIDULayer->GetFieldCol( "POLICYAPPS" );
   if ( m_colPolicyApps < 0 )
      {
      Report::StatusMsg( "PolicyApps field not found - This field is being added...." );
      m_colPolicyApps = m_pIDULayer->m_pDbTable->AddField( "PolicyApps", TYPE_INT );
      Notify( EMNT_IDUCHANGE, 0 );
      }

   m_colActor = m_pIDULayer->GetFieldCol( "ACTOR" );
   if ( m_colActor < 0 && m_pActorManager->m_actorInitMethod == AIM_IDU_GROUPS )
      {
      Report::StatusMsg( "ACTOR field not found - This field is being added...." );
      m_colActor = m_pIDULayer->m_pDbTable->AddField( "Actor", TYPE_INT );
      Notify( EMNT_IDUCHANGE, 0 );
      }

   m_colLastDecision = m_pIDULayer->GetFieldCol( "LASTD" );
   if ( m_colLastDecision < 0 )
      {
      Report::StatusMsg( "Last Decision (LASTD) field not found - This field is being added...." );
      m_colLastDecision = m_pIDULayer->m_pDbTable->AddField( "LastD", TYPE_INT );
      Notify( EMNT_IDUCHANGE, 0 );
      }

   m_colNextDecision = m_pIDULayer->GetFieldCol( "NEXTD" );
   if ( m_colNextDecision < 0 )
      {
      Report::StatusMsg( "Next Decision (NEXTD) field not found - This field is being added...." );
      m_colNextDecision = m_pIDULayer->m_pDbTable->AddField( "NextD", TYPE_INT );
      Notify( EMNT_IDUCHANGE, 0 );
      }

   int colPopDens = m_pIDULayer->GetFieldCol( "POPDENS" );
   if ( colPopDens < 0 )
      {
      Report::StatusMsg( "Population Density (POPDENS) field not found - This field is being added...." );
      colPopDens = m_pIDULayer->m_pDbTable->AddField( "PopDens", TYPE_FLOAT );
      m_pIDULayer->SetColData( colPopDens, VData( 0 ), true );
      Notify( EMNT_IDUCHANGE, 0 );
      }

   int m_colUtility = m_pIDULayer->GetFieldCol( "UTILITYID" );
   if ( m_colUtility < 0 && this->m_decisionElements & DE_UTILITY )
      {
      Report::StatusMsg( "Utility Policy ID (UTILITYID) field not found - This field is being added...." );
      m_colUtility = m_pIDULayer->m_pDbTable->AddField( "UtilityID", TYPE_INT );
      m_pIDULayer->SetColNoData( m_colUtility );
      Notify( EMNT_IDUCHANGE, 0 );
      }

   //???Check
   //if ( gpDoc->IsChanged( CHANGED_COVERAGE ) && m_pIDULayer->m_recordsLoaded > 0 )
   //   {
   //   if ( AfxMessageBox( "The IDU database has had fields added to it.  Do you want to save these changes to disk?", MB_YESNO ) == IDYES )
   //      gpDoc->OnFileSavedatabase();
   //   }

   Report::StatusMsg( "Finished Checking IDU fields" );
   }


void EnvModel::StoreModelCols()
   {
   /*  deprecated!
   // make sure eval model, ap cols are there
   int modelCount = GetEvaluatorCount();
   //ASSERT( modelCount > 0 );

   for ( int i=0; i < modelCount; i++ )
      {
      EnvEvaluator *pInfo = GetEvaluatorInfo( i );

      if ( pInfo->col != -1 )
         {
         int col = m_pIDULayer->GetFieldCol( pInfo->fieldName );
      
         if ( col < 0 )
            {
            CString msg;
            msg.Format( "Evaluative Model, %s, field, %s, is missing; and will be added.  ",
               (PCTSTR) pInfo->m_name, (PCTSTR) pInfo->fieldName);
            Report::InfoMsg( msg );
            m_pIDULayer->m_pDbTable->AddField( pInfo->fieldName, TYPE_FLOAT );
            col = m_pIDULayer->GetFieldCol( pInfo->fieldName );
            m_pIDULayer->SetColNoData( col );
            Notify( EMNT_IDUCHANGE, 0 );
            }
         pInfo->col = col;
         }
      }

   int apCount = GetModelProcessCount();

   for ( int i=0; i < apCount; i++ )
      {
      EnvModelProcess *pInfo = GetModelProcessInfo( i );
      if ( pInfo->col != -1 )
         {
         int col = m_pIDULayer->GetFieldCol( pInfo->fieldName );
      
         if ( col < 0 )
            {
            CString msg;
            msg.Format( "Autonomous Model, %s, field, %s, is missing; and will be added.  ",
               (PCTSTR) pInfo->m_name, (PCTSTR) pInfo->fieldName);
      
            Report::InfoMsg( msg );
            
            m_pIDULayer->m_pDbTable->AddField( pInfo->fieldName, TYPE_FLOAT );
            col = m_pIDULayer->GetFieldCol( pInfo->fieldName );
            m_pIDULayer->SetColNoData( col );
            Notify( EMNT_IDUCHANGE, 0 );
            }
         pInfo->col = col;
         }
      }
   */
   }
 

//====================================

void EnvModel::POP(const DELTA & delta)
   {
   // ignore all deltas that are not from policies
   //ASSERT( delta.type == DT_POLICY );
   //if ( delta.type != DT_POLICY )  continue;
   ASSERT( delta.type != DT_POP );

   const int col = delta.col;
   const int cell = delta.cell;
   const VData & newValue = delta.newValue;
   const int year = delta.year;

   if ( col == m_colLulcD )
      {         
      int lulcD;
      bool ok = newValue.GetAsInt( lulcD );
      ASSERT( ok );

      LulcNode *pNode = m_lulcTree.FindNode( 4, lulcD );  //Get corresponding node in lulcTree
      //ASSERT( pNode != NULL );
      if( pNode == NULL )
         {
         CString msg;
         msg.Format( "POP Error: Unrecognized Tier 4 LULC ID found: '%i'", lulcD ); 
         Report::Log( msg );
         return;
         }

       // Get LulcC
      pNode = pNode->m_pParentNode;
      ASSERT( pNode != NULL );
      if ( pNode != NULL )
         {
         int lulcC = pNode->m_id;
         AddDelta( cell, m_colLulcC, year, lulcC, DT_POP );

         // Get LulcB
         pNode = pNode->m_pParentNode;
         ASSERT( pNode != NULL );
         int lulcB = pNode->m_id;
         AddDelta( cell, m_colLulcB, year, lulcB, DT_POP );

         // Get LulcA
         pNode = pNode->m_pParentNode;
         ASSERT( pNode != NULL );
         int lulcA = pNode->m_id;
         AddDelta( cell, m_colLulcA, year, lulcA, DT_POP );
         }
      }     
   
   else if ( col == m_colLulcC )
      {         
      int lulcC;
      bool ok = newValue.GetAsInt( lulcC );
      ASSERT( ok );

      LulcNode *pNode = m_lulcTree.FindNode( 3, lulcC );  //Get corresponding node in lulcTree
      if( pNode == NULL )
         {
         CString msg;
         msg.Format( "POP Error: Unrecognized Tier 3 LULC ID found: '%i'", lulcC ); 
         Report::Log( msg );
         return;
         }

      // Get LulcB
      pNode = pNode->m_pParentNode;
      ASSERT( pNode != NULL );

      if ( pNode != NULL )
         {
         int lulcB = pNode->m_id;
         AddDelta( cell, m_colLulcB, year, lulcB, DT_POP );

         // Get LulcA
         pNode = pNode->m_pParentNode;
         if ( pNode == NULL )
            return;

         int lulcA = pNode->m_id;
         AddDelta( cell, m_colLulcA, year, lulcA, DT_POP );
         }
      }     

   else if ( col == m_colLulcB )
      {
      int lulcB;
      bool ok = newValue.GetAsInt( lulcB );
      ASSERT( ok );

      LulcNode *pNode = m_lulcTree.FindNode( 2, lulcB );  //Get corresponding node in lulcTree
      if( pNode == NULL )
         {
         CString msg;
         msg.Format( "POP Error: Unrecognized Tier 2 LULC ID found: '%i'", lulcB ); 
         Report::Log( msg );
         return;
         }

      int nodeCount = (int) pNode->m_childNodeArray.GetSize();
      if ( nodeCount > 0 )
         {
         int offset = (int) m_randUnif.RandValue( 0, nodeCount-0.00001 );
         ASSERT( offset < nodeCount );

         // Get LulcC Randomly
         int lulcC = pNode->m_childNodeArray[ offset ]->m_id; 
         AddDelta( cell, m_colLulcC, year, lulcC, DT_POP );
         }

      // Get LulcA
      pNode = pNode->m_pParentNode;
      if ( pNode == NULL )
         return;

      int lulcA = pNode->m_id;
      AddDelta( cell, m_colLulcA, year, lulcA, DT_POP );
      }

   else if ( col == m_colLulcA )
      {
      int lulcA;
      bool ok = newValue.GetAsInt( lulcA );
      ASSERT( ok );

      LulcNode *pNode = m_lulcTree.FindNode( 1, lulcA );  //Get corresponding node in lulcTree
      if( pNode == NULL )
         {
         CString msg;
         msg.Format( "POP Error: Unrecognized Tier 1 LULC ID found: '%i'", lulcA ); 
         Report::Log( msg );
         return;
         }

      // get lulcB value (if level exists)
      int nodeCount = (int) pNode->m_childNodeArray.GetSize();
      if ( nodeCount > 0 )
         {
         int offset = (int) m_randUnif.RandValue( 0, nodeCount-0.00001 );
         ASSERT( offset < nodeCount );

         pNode = pNode->m_childNodeArray[ offset ];
         int lulcB = pNode->m_id;
         AddDelta( cell, m_colLulcB, year, lulcB, DT_POP );

         // get lulcC value
         nodeCount = (int) pNode->m_childNodeArray.GetSize();
         if( nodeCount > 0 )
            {
            offset = (int) m_randUnif.RandValue( 0, nodeCount-0.00001 );
            ASSERT( offset < nodeCount );

            int lulcC = pNode->m_childNodeArray[ offset ]->m_id;
            AddDelta( cell, m_colLulcC, year, lulcC, DT_POP );
            }
         }
      }
   }  


//void EnvModel::ExportRunResults( int run )
//   {
//   // get the location of the cell layer
//   nsPath::CPath path( m_pIDULayer->m_path );
//
//   path.RemoveFileSpec();
//   path.Append( m_pScenario->m_name );
//   path.AddBackslash();
//
//   int flag = 0;
//   if ( this->m_exportMaps )
//      flag = 1;
//   if ( this->m_exportOutputs )
//      flag += 2;
//
//   m_pDataManager->ExportRunData( path, run );
//   }
/*

void EnvModel::ExportDeltaArray( int run )
   {
   // get the location of the cell layer
   nsPath::CPath path( m_pIDULayer->m_path );

   path.RemoveFileSpec();
   path.Append( m_pScenario->m_name );
   path.AddBackslash();

   _mkdir( path ); 

   CString basePath( path );

   CString filename;
   filename.Format( _T("Year_%i.shp" ), year );
   path.Append( filename );

   m_pIDULayer->SaveShapeFile( path );

   // include bmps?
   if ( m_exportBmpSize > 0 )
      {
      if ( m_exportBmpFields.GetLength() > 0 )
         {
         TCHAR buffer[ 256 ];
         lstrcpy( buffer, m_exportBmpFields );
         TCHAR *nextToken;

         TCHAR *field = _tcstok_s( buffer, _T(";,"), &nextToken );

         while( field != NULL )
            {
            int col = m_pIDULayer->GetFieldCol( field );
            if ( col < 0 )
               {
               CString msg;
               msg.Format( "Unable to find field '%s' when exporting end of run Bitmaps", field );
               Report::ErrorMsg( msg );
               }
            else
               {
               path = basePath;
               filename.Format( _T("Year_%i_%s.bmp" ), year, field );
               path.Append( filename );
            
               Raster raster( m_pIDULayer, false ); // don't copy polys, 
               raster.Rasterize( col, m_exportBmpSize, RT_32BIT, 0, NULL, DV_COLOR );
               raster.SaveDIB( path );
               }

            field = _tcstok_s( NULL, ";,", &nextToken );
            }
         }
      else
         {
         int activeCol = m_pIDULayer->GetActiveField();

         if ( activeCol >= 0 )
            {
            path = basePath;
            filename.Format( _T("Year_%i_%s.bmp" ), year, m_pIDULayer->GetFieldLabel( activeCol ) );
            path.Append( filename );

            Raster raster( m_pIDULayer, false ); // don't copy polys, 
            raster.Rasterize( activeCol, m_exportBmpSize, RT_32BIT, 0, NULL, DV_COLOR );
            raster.SaveDIB( path );
            }
         }
      }
   }
*/



void EnvModel::SetIDULayer( MapLayer *pLayer )
   {
   m_pIDULayer = pLayer;

   if ( m_pQueryEngine != NULL )
      delete m_pQueryEngine;

   m_pQueryEngine = new QueryEngine( pLayer );

   ASSERT( m_pExprEngine == NULL );
   m_pExprEngine = new MapExprEngine( pLayer, m_pQueryEngine );

   this->m_envContext.pMapLayer = pLayer;
   this->m_envContext.pExprEngine = m_pExprEngine;
   this->m_envContext.pQueryEngine = m_pQueryEngine;
   }


MapExpr *EnvModel::AddExpr( LPCTSTR name, LPCTSTR expr )
   {
   MapExpr *pMapExpr = m_pExprEngine->AddExpr( name, expr, NULL ); // and in expression evaluator
   return pMapExpr;
   }


bool EnvModel::EvaluateExpr( MapExpr *pMapExpr, bool useQuery, float &result )
   {
   bool ok = m_pExprEngine->EvaluateExpr( pMapExpr, useQuery );
   
   if ( ok )
      result = (float) pMapExpr->GetValue();
   
   return ok;
   }


int EnvModel::AddAppVar(AppVar *pVar, bool useMapExpr)
   {
   int index = (int)m_appVarArray.Add(pVar);    // store AppVar locally

   // is this AppVar's value calculated from an expression?
   if (useMapExpr)
      {
      ASSERT(pVar->m_avType == AVT_APP);    // this should be the only type using expressions

      pVar->m_pMapExpr = AddExpr(pVar->m_name, pVar->m_expr); // and in expression evaluator
      pVar->m_pMapExpr->m_extra = (INT_PTR)pVar;  // store a pointer to the pVar
      }
   else
      pVar->m_pMapExpr = NULL;

   // evaluate the AppVar if called for
   if (pVar->IsGlobal())
      pVar->Evaluate();

   // each app var needs to be registered with the query engine so they can be seen   
   //ASSERT( gpQueryEngine != NULL );
   if (pVar->m_avType == AVT_OUTPUT || pVar->m_avType == AVT_APP)
      {
      TYPE type = pVar->GetValue().GetType();
      switch (type)
         {
         case TYPE_PDOUBLE:
         case TYPE_PFLOAT:
         case TYPE_PINT:
         case TYPE_PUINT:
         case TYPE_PLONG:
         case TYPE_PULONG:
         case TYPE_PSHORT:
            {
            QExternal *pExternal = m_pQueryEngine->AddExternal(pVar->m_name);
            pExternal->SetValue(pVar->GetValue());   // Note: the AppVar value is always a ptr to another value 
            }
         }
      }

   return index;
   }


/*
      
 void EnvModel::CreateModelAppVars( void )
   {
   for ( int i=0; i < GetModelProcessCount(); i++ )
      {
      EnvModelProcess *pInfo = GetModelProcessInfo( i );

      if ( pInfo->outputVarFn != NULL )
         {
         MODEL_VAR *modelVarArray = NULL;
         int varCount = pInfo->outputVarFn( pInfo->m_id, &modelVarArray );

         for ( int j=0; j < varCount; j++ )
            {
            MODEL_VAR *pModelVar = modelVarArray + j;

            //CString msg;
            //msg.Format( "%s ModelVar %i of %i, name:%s", (LPCTSTR) pInfo->m_name, 
            //   j, varCount, pModelVar->m_name );
            //Report::Log( msg );
    
            if ( pModelVar->pVar != NULL )
               {
               CString name( pInfo->m_name );
               name += ".";
               name += pModelVar->m_name;
               name.Replace(' ', '_');
               AppVar *pAppVar = new AppVar( name, pModelVar->description, NULL );
               pAppVar->m_avType = AVT_OUTPUT;
               pAppVar->m_pModelVar = pModelVar;
               pAppVar->m_pEnvProcess = pInfo;
               pAppVar->SetValue( VData( pModelVar->pVar, pModelVar->type, true ) );  // stores ptr to variable, so value is always current
               AddAppVar( pAppVar, false );  // creates corresponding MapExpr - no MapExpr required since it just references a model var
               }
            }
         }
      }

   // repeat for eval models, including score, raw score params
   for ( int i=0; i < GetEvaluatorCount(); i++ )
      {
      EnvEvaluator *pInfo = GetEvaluatorInfo( i );

      CString name( pInfo->m_name );
      name += ".";
      name += "rawscore";
      AppVar *pAppVar = new AppVar( name, "", NULL );
      pAppVar->m_avType = AVT_RAWSCORE;
      pAppVar->m_pEnvProcess = pInfo;
      pAppVar->SetValue( VData( &(pInfo->m_rawScore), TYPE_FLOAT, true ) );
      AddAppVar( pAppVar, false );
 
      name = pInfo->m_name;
      name += ".";
      name += "score";
      pAppVar = new AppVar( name, "", NULL );
      pAppVar->m_avType = AVT_SCORE;
      pAppVar->m_pEnvProcess = pInfo;
      pAppVar->SetValue( VData( &(pInfo->m_score), TYPE_FLOAT, true ) );
      AddAppVar( pAppVar, false );

      if ( pInfo->outputVarFn != NULL )
         {
         MODEL_VAR *modelVarArray = NULL;
         int varCount = pInfo->outputVarFn( pInfo->m_id, &modelVarArray );
         
         for ( int j=0; j < varCount; j++ )
            {
            MODEL_VAR *pModelVar = modelVarArray+j;

            //CString msg;
            //msg.Format( "%s ModelVar %i of %i, name:%s", (LPCTSTR) pInfo->m_name, 
            //   j, varCount, pModelVar->m_name );
            //Report::Log( msg );
            if ( pModelVar->pVar != NULL )
               {
               CString name( pInfo->m_name );
               name += ".";
               name += pModelVar->m_name;
               pAppVar = new AppVar( name, pModelVar->description, NULL );
               pAppVar->m_avType = AVT_OUTPUT;
               pAppVar->m_pModelVar = pModelVar;
               pAppVar->m_pEnvProcess = pInfo;
               pAppVar->SetValue( VData( pModelVar->pVar, pModelVar->type, true ) );
               AddAppVar( pAppVar, false );
               }
            }
         }
      }
   }
*/



int EnvModel::GetAppVarCount( int avtypes /*=0*/ )
   {
   if ( avtypes == 0 )
      return (int) m_appVarArray.GetSize();
   else
      {
      int count = 0;
      for ( int i=0; i < m_appVarArray.GetSize(); i++ )
         {
         if ( m_appVarArray[ i ]->m_avType & avtypes )
            count++;
         }
   
      return count;
      }
   }


// update all map expression-based App Vars with the specified timing to the specified IDU
bool EnvModel::UpdateAppVars( int idu, int applyToCoverage, int timing )
   {
   if ( m_pExprEngine == NULL )
      return true;

   // note - idu < 0 means only evaluate expression that don't refer to a field
   if ( idu >= 0)
      m_pExprEngine->SetCurrentRecord( idu );
   
   int count = GetAppVarCount();
   for ( int i=0; i < count; i++ )
      {
      AppVar *pVar = GetAppVar( i );
      bool evaluate = false;

      if ( pVar->m_timing == 0 ) // don't evaluate?
         continue;

      if ( ( pVar->m_timing & timing ) == 0 )
         continue;

      if ( pVar->m_pMapExpr != NULL )  // does it have a map expression?
         {
         if ( idu < 0 )
            {
            if ( pVar->m_pMapExpr->IsGlobal() )   // only use exprs with no field refs (global)
               evaluate = true;
            }
         else
            {
            if ( pVar->m_pMapExpr->IsFieldBased() )
               evaluate = true;
            }

         if ( evaluate )
            { 
            // evaluate the AppVar's expression to update it's value
            float result;
            EvaluateExpr( pVar->m_pMapExpr, false, result ); // don't use query (not supported for app vars)
         
            // get the value from the expression and store it with the appvar
            pVar->SetValue( VData( result ) );   // this stores a floating point value in the AppVar

            // store in coverage?
            if ( idu > 0 && pVar->GetCol() >= 0 && applyToCoverage > 0 )
               {
               if ( applyToCoverage == 1 ) // use delta
                  this->AddDelta( idu, pVar->GetCol(), m_currentYear, VData( result ), DT_APPVAR );
               else
                  m_pIDULayer->SetData( idu, pVar->GetCol(), result );                  
               }
            }
         }
      }  // end of: i < GetAppVarCount()

   return true;
   }


// update all map expression-based App Vars to the specified IDU
bool EnvModel::StoreAppVarsToCoverage( int timing, bool useDelta )
   {
   if ( m_pExprEngine == NULL )
      return true;

   m_pExprEngine->SetCurrentTime( float( this->m_currentYear ) );
   
   int count = GetAppVarCount();
   int colCount = 0;
   for ( int i=0; i < count; i++ )   // these are guaranteed to be first
      {
      AppVar *pVar = GetAppVar( i );
      if ( ( pVar->m_timing & timing ) && ( pVar->GetCol() >= 0 ) && ( ! pVar->IsGlobal() ) )
         {
         colCount++;
         break;
         }
      }

   if ( colCount == 0 )       // nothing to store?
      return true;

   // iterate through polys, updating as we go
   for ( MapLayer::Iterator cell = m_pIDULayer->Begin(); cell != m_pIDULayer->End(); cell++ )
      {
      m_pExprEngine->SetCurrentRecord( cell );

      for ( int i=0; i < count; i++ )   // these are guaranteed to be first
         {
         AppVar *pVar = GetAppVar( i );

         if ( ( pVar->m_timing & timing ) && ( pVar->GetCol() >= 0 ) && ( ! pVar->IsGlobal() ) )
            {
            bool ok = pVar->Evaluate( cell );
            float value = -99;
            if ( ok )
               value = pVar->GetValue( value );

            float oldValue = -99;
            m_pIDULayer->GetData( cell, pVar->GetCol(), oldValue );

            if ( fabs( oldValue-value ) > 0.000001 )
               {
               if ( useDelta )
                  AddDelta( cell, pVar->GetCol(), m_currentYear, value, DT_APPVAR );
               else
                  m_pIDULayer->SetData( cell, pVar->GetCol(), value );
               }
            }
         }
      }

   return true;
   }



void EnvModel::RemoveAllAppVars()
   {
   for ( int i=0; i < GetAppVarCount(); i++ )
      {
      AppVar *pVar = GetAppVar( i );

      if ( pVar->m_pMapExpr )
         m_pExprEngine->RemoveExpr( pVar->m_pMapExpr );
      }
   
   m_appVarArray.Clear();
   }


bool EnvModel::InitSocialNetwork( void )
   {
   if ( ( m_decisionElements & DE_SOCIALNETWORK ) == 0 )
      return false;
   
   if ( m_pSocialNetwork )
      delete m_pSocialNetwork;

   m_pSocialNetwork = new SocialNetwork(this);
   
   CString path = PathManager::GetPath( PM_PROJECT_DIR );
   //nsPath::CPath path( gpDoc->m_iniFile );
   //path.RemoveFileSpec();
   path.Append( _T("SocialNetwork.xml") );

   return m_pSocialNetwork->Init( path ) ? true : false;
   }


int EnvModel::ChangeIDUActor( EnvContext *pContext, int idu, int groupID, bool randomize )
   {
   if ( m_allowActorIDUChange != true )
      return -1;

   if ( m_pActorManager->m_actorInitMethod != AIM_IDU_GROUPS )  // only ACTOR GROUP defined in ACTOR field allowed to change
      return -2;

   ActorGroup *pGroup =  m_pActorManager->GetActorGroupFromID( groupID );

   if ( pGroup == NULL )
      return -3;

   int index = -1;
   Actor *pOldActor = m_pActorManager->GetActorFromIDU( idu, &index );

   if ( pOldActor->m_pGroup == pGroup )
      return -4;   // no change needed

   // passed various validity tests - proceed to create and assign new actor
   Actor *pNewActor = m_pActorManager->CreateActorFromGroup( pGroup, randomize );  // creates actors, sets values
   if ( pNewActor == NULL )
      return -5;

   // basic idea - replace the existing actor by:
   //   a) adding the actor to the actor manager list
   //   b) removing the idu from the previous actor's idu list
   //   c) adding the idu to the new actors idu list

   if ( pOldActor != NULL )
      pOldActor->RemovePoly( idu );

   //if ( pOldActor->GetPolyCount() == 0 )
   //   m_pActorManager->RemoveActor( pOldActor->m_index );

   m_pActorManager->AddActor( pNewActor );
   pNewActor->AddPoly( idu );

   if ( pContext != NULL )
      this->AddDelta( idu, m_colActor, pContext->currentYear, VData( groupID ), pContext->handle );
   else
      m_pIDULayer->SetData( idu, m_colActor, groupID );
   
   return groupID;   
   }


int EnvModel::RunSensitivityAnalysis()
   {
   m_inMultiRun = true;

   bool showMessages = m_showMessages;
   m_showMessages = false;
   m_envContext.showMessages = false;

   bool useInitialSeed = m_resetInfo.useInitialSeed;
   m_resetInfo.useInitialSeed = false;

   // basic idea.  Run the baseline scenario first.  Then, for each model
   // variable and policy to be included, make another run.  Model variables and
   // policies to be included are indicated with their 
   // "m_includeInSensitivityAnalysis" attribute.
   // the number of runs to make is one + the number of attributes to vary.
   // At the end of the run, the data objects for model outputs are examined
   // and a sensitivity report is generated.
   int oldIterationsToRun = m_iterationsToRun;

   m_iterationsToRun = 1;  // baseline scenario

   int saMetaCount = 0;
   int saModelCount = 0;
   int saProcessCount = 0;
   int saAppVarCount = 0;
   int saPolicyCount = 0;
   CStringArray saNameArray;
   CArray< void*, void* > saPtrArray;

   //========================== METAGOAL SETTINGS ============================
   int metagoalCount = EnvModel::GetMetagoalCount();
   for ( int i=0; i < metagoalCount; i++ )
      {
      METAGOAL *pInfo = GetMetagoalInfo( i );
      if ( pInfo->useInSensitivity )
         {
         saMetaCount++;
         CString name( "Meta." );
         name += pInfo->name;
         saNameArray.Add( name );
         saPtrArray.Add( pInfo );
         m_iterationsToRun++;
         }
      }

   //====================== MODEL/PROCESS USAGE SETTINGS =====================
   int modelCount = GetEvaluatorCount();
   for ( int i=0; i < modelCount; i++ )
	   {
      EnvEvaluator *pModel = EnvModel::GetEvaluatorInfo( i );
      MODEL_VAR *modelVarArray = NULL;
      int varCount = pModel->InputVar( pModel->m_id, &modelVarArray );

      // for each variable exposed by the model, add it to the possible scenario variables
      for ( int j=0; j < varCount; j++ )
         {
         MODEL_VAR &mv = modelVarArray[ j ];
         if ( mv.useInSensitivity && mv.pVar != NULL )
            {
            saModelCount++;
            CString name;
            name.Format( "%s.%s", (LPCTSTR) pModel->m_name, (LPCTSTR) mv.name );
            saNameArray.Add( name );
            saPtrArray.Add( &mv );
            m_iterationsToRun++;
            }
         }
      }
      
   //================= A U T O N O U S  P R O C E S S   V A R I A B L E S ==================
   int apCount = EnvModel::GetModelProcessCount();
   for ( int i=0; i < apCount; i++ )
	   {
      EnvModelProcess *pModel = GetModelProcessInfo( i );
      MODEL_VAR *modelVarArray = NULL;
      int varCount = pModel->InputVar( pModel->m_id, &modelVarArray );

      // for each variable exposed by the model, add it to the possible scenario variables
      for ( int j=0; j < varCount; j++ )
         {
         MODEL_VAR &mv = modelVarArray[ j ];
         if ( mv.useInSensitivity && mv.pVar != NULL )
            {
            saProcessCount++;
            CString name;
            name.Format( "%s.%s", (LPCTSTR) pModel->m_name, (LPCTSTR) mv.name );
            saNameArray.Add( name );
            saPtrArray.Add( &mv );
            m_iterationsToRun++;
            }
         }
      }
      
   //================= A P P L I C A T I O N   V A R I A B L E S ==================
   int apVarCount = GetAppVarCount( (int) AVT_APP );
   int usedAppVarCount = 0;
   for( int i=0; i < apVarCount; i++ )
	   {
      AppVar *pAppVar = GetAppVar( i );

      if ( pAppVar->m_useInSensitivity )
         {
         saAppVarCount++;
         CString name;
         name.Format( "AppVar.%s", (LPCTSTR) pAppVar->m_name );
         saNameArray.Add( name );
         saPtrArray.Add( pAppVar );
         m_iterationsToRun++;
         }
      }
   
   // load all existing policies from policy manager
   for ( int i=0; i < m_pPolicyManager->GetPolicyCount(); i++ )
      {
      Policy *pPolicy = m_pPolicyManager->GetPolicy( i );
      if ( pPolicy->m_useInSensitivity )
         {
         saPolicyCount++;
         CString name;
         name.Format( "Policy.%s", (LPCTSTR) pPolicy->m_name );
         saNameArray.Add( name );
         saPtrArray.Add( pPolicy );
         m_iterationsToRun++;
         }
      }

   // okay, we know how many - get ready to start running.  First we run a baseline.
   // then, we rerun, reverting previous values to the baseline and modifying
   // the next parameter in the list

   if ( m_pDataManager )
      m_pDataManager->CreateMultiRunDataObjects();

   m_continueToRunMultiple = true;

   int countSoFar = 0;
   /*
   for ( int i=0; i < m_iterationsToRun; i++ )
      {
      if ( i == 0 )
         ;  // setup baseline scenario TODO!!!!!
      else if ( IsBetween( i, 0, saMetaCount ) )
         {

         }
      else if ( IsBetween( i, saMetaCount+1, saModelCount ) )
         {
         }
      else if ( IsBetween( i, saModelCount+1, saProcessCount ) )
         {
         }
      else if ( IsBetween( i, saProcessCount+1, saAppVarCount ) )
         {
         }
      else if ( IsBetween( i, saAppVarCount+1, saPolicyCount ) )
         {
         }
      else
         {
         ASSERT( 0 );
         }

      Run( -1);   // no randomization, don't reset current sa variable.  This increments m_currentRun

      if ( m_pDataManager )
         m_pDataManager->CollectMultiRunData( this, i );

      Reset();

      if ( !m_continueToRunMultiple )
         break;

      gpMain->SetMultiRunProgressPos( i+1 );
      }
   */
   Notify( EMNT_MULTIRUN, 0, m_currentMultiRun );
   m_currentMultiRun++;

   m_inMultiRun = false;
   m_showMessages = m_envContext.showMessages = showMessages;
   m_resetInfo.useInitialSeed = useInitialSeed;

   // generate sensitivity outputs

   int itr = m_iterationsToRun;
   m_iterationsToRun = oldIterationsToRun;

   return itr;
   }


void EnvModel::Notify( EM_NOTIFYTYPE type, INT_PTR param  )
   {
   if ( m_notifyProc == NULL )
      return;

   m_notifyProc(type, param, m_extra, (INT_PTR)this );
   }


void EnvModel::Notify( EM_NOTIFYTYPE type, INT_PTR param, INT_PTR extra  )
   {
   if ( m_notifyProc == NULL )
      return;

   m_notifyProc(type, param, extra, (INT_PTR)this);
   }



bool EnvModel::VerifyIDULayer( void )
   {
   ASSERT( m_pIDULayer != NULL );

   float area;
   LulcNode *pNode = NULL;
   int badAreaCount = 0;
   int badLulcCount = 0;
   int zeroLulcCount = 0;
   bool badLulc = false;
   int badFieldCount = 0;

   // check for bad field names
   //for (int i=0; i < pLayer->GetFieldCount(); i++ )
   //   {
   //   LPCTSTR field = pLayer->GetFieldLabel( i );

   //   if ( field == NULL || *field == NULL )
   //      pLayer->m_pDbTable->RemoveField( i );

    //  badFieldCount++;
    //  }


   CUIntArray badCellArray;

   if ( m_pIDULayer == NULL )
      return false;

   int colArea = m_pIDULayer->GetFieldCol( "AREA" );

   for ( MapLayer::Iterator i = m_pIDULayer->Begin(); i != m_pIDULayer->End(); i++ )
      {
      // check Area
      m_pIDULayer->GetData( i, m_colArea, area );
      if ( area <= 0.0f )
         {
         badCellArray.Add( i );
         badAreaCount++;
         }

      // check Lulc
      badLulc = false;
      int lulc = 0;
      int levels = m_lulcTree.GetLevels();

      switch( levels )
         {
         case 4:
            m_pIDULayer->GetData( i, m_colLulcD, lulc );
            break;

         case 3:
            m_pIDULayer->GetData( i, m_colLulcC, lulc );
            break;

         case 2:
            m_pIDULayer->GetData( i, m_colLulcB, lulc );
            break;

         case 1:
            m_pIDULayer->GetData( i, m_colLulcA, lulc );
            break;
         }

      if ( levels > 0 && lulc == 0 )
         {
         zeroLulcCount++;
         // nothing wrong with LULC=0!!!
         //if ( badCellArray.GetSize() == 0 || badCellArray[ badCellArray.GetSize()-1 ] != i )
         //   badCellArray.Add( i );
         //
         //badLulc = true;
         }
      else
         {
         for ( int level = levels; level > 0; level-- )
            {
            pNode = m_lulcTree.FindNode( level, lulc );
            if ( pNode == NULL )
               {
               badLulc = true;
               break;
               }
            else
               lulc = pNode->m_pParentNode->m_id;
            }
         }

      if ( badLulc )
         {
         if ( badCellArray.GetSize() == 0 || badCellArray[ badCellArray.GetSize()-1 ] != i )
            badCellArray.Add( i );
 
         badLulcCount++;
         }
      }  // end of:  for MapLayer::Iterator through cells

   CString msgDetails = _T("");

   if ( zeroLulcCount > 0 )
      {
      CString msg;
      msg.Format( "%d Cells with LULC = 0; ", zeroLulcCount );
      msgDetails += msg;
      }

   if ( badLulcCount > 0 )
      {
      CString msg;
      msg.Format( "%d Cells with inconsistent Lulc Data; ", badLulcCount );
      msgDetails += msg;
      }

   if ( badAreaCount > 0 )
      {
      CString msg;
      msg.Format( "%d Cells with non-positive Area; ", badAreaCount );
      msgDetails += msg;
      }

   if ( badFieldCount > 0 )
      {
      CString msg;
      msg.Format( "%d Fields that have invalide names (removed); ", badFieldCount );
      msgDetails += msg;
      }

   if ( badAreaCount || badLulcCount || zeroLulcCount || badLulc || badFieldCount )
      {
      CString msg = "The MapLayer has: ";
      msg += msgDetails;
      Report::LogWarning( msg );

      for ( int i=0; i < badCellArray.GetSize(); i++ )
         m_pIDULayer->AddSelection( badCellArray[ i ] );

      //m_pIDULayer->m_pMap->RedrawWindow();

      //if ( badFieldCount > 0 )
      //   OnFileSavedatabase();

      return false;
      }

   return true;
   }


bool EnvModel::CheckValidFieldNames( void )
   {
   if ( m_pIDULayer->m_pDbTable == NULL )
      return true;

   for ( int i=0; i < m_pIDULayer->GetFieldCount(); i++ )
      {
      LPCTSTR field = m_pIDULayer->GetFieldLabel( i );
      LPTSTR  p = (LPTSTR) field;
      int invalid = 0;

      if ( p == NULL || *p == NULL )
         {
         CString name;
         name.Format( "_Temp%i", i );
         m_pIDULayer->SetFieldLabel( i, name);
         continue;
         }

      while ( *p != NULL )
         {
         if ( *p != '_' )
            {
            if ( *p < 0 || *p > 255 || ! isalnum( *p ) )
               {
               invalid++;
               *p = '_';
               }
            }

         p++;
         }

      if ( invalid > 0 )
         {
         CString msg;
         msg.Format( "An invalid field name, [%s], was founding in the coverage.  It is being replaced with [%s]", field, m_pIDULayer->GetFieldLabel( i ) );
         Report::ErrorMsg( msg );
         m_pIDULayer->SetFieldLabel( i, field );

         //gpDoc->SetChanged( CHANGED_COVERAGE );
         }
      }
   return true;
   }

