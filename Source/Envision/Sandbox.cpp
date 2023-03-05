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
#include "StdAfx.h"
#include ".\sandbox.h"

#include <EnvException.h>
#include <maplayer.h>
#include <SriRandomPath.h>
#include "EnvDoc.h"
#include "mainFrm.h"
#include <Scenario.h>
#include <Actor.h>
#include <EnvPolicy.h>
#include <DeltaArray.h>
#include "Evaluatorlearning.h"


extern ActorManager  *gpActorManager;
extern PolicyManager *gpPolicyManager;

extern MapLayer   *gpCellLayer;   // get rid of this!!??
extern CEnvDoc    *gpDoc;
extern CMainFrame *gpMain;


Sandbox::Sandbox( EnvModel *pModel, int years /*= 10*/, float percentCoverage /*= 1.0f*/, bool callInitRun /*= false*/ ) 
   : EnvModel( *pModel )
   , m_percentCoverage( percentCoverage )
   {
   m_yearsToRun = years;
   m_evalFreq   = pModel->m_evalFreq;
   m_policyUpdateEnabled = false;
   m_actorUpdateEnabled  = false;
   m_dynamicUpdate       = 0;
   m_showMessages        = false;
   m_showRunProgress     = true; //false; jpb changed 2/5/05

   ASSERT(gpCellLayer);
   m_pDeltaArray             = new DeltaArray( gpCellLayer, pModel->m_startYear );
   m_envContext.pDeltaArray = m_pDeltaArray;
 
   int emCount = GetEvaluatorCount();
   int apCount = GetModelProcessCount();

   m_emFirstUnseenDelta.SetSize( emCount, 0 );
   for ( int i=0; i<emCount; i++ )
      m_emFirstUnseenDelta[i] = 0;

   m_apFirstUnseenDelta.SetSize( apCount, 0 );
   for ( int i=0; i<apCount; i++ )
      m_apFirstUnseenDelta[i] = 0;

   if ( ! m_areModelsInitialized )
      InitModels();

      // initialize landscape metrics
   m_landscapeMetrics.SetSize( emCount, 0 );
   for ( int i=0; i <emCount; i++ )
      {
      m_landscapeMetrics[i].scaled = 0.0f;
      m_landscapeMetrics[i].raw    = 0.0f;
      }

   if ( callInitRun )
      {
      InitRunModels();
      RunEvaluation();      // initial Evaluation
      }
   
   m_initialMetrics.SetSize( emCount );
   for ( int i=0; i < emCount; i++ )
      m_initialMetrics.SetAt( i, GetLandscapeMetric(i) );
   }


Sandbox::~Sandbox(void)
   {
   Reset();

   if ( m_pDeltaArray != NULL )
      delete m_pDeltaArray;
   }


bool Sandbox::Reset( void )
   {
   ASSERT( gpCellLayer  != NULL );
   static bool showMessages = true;

   m_currentYear = 0;

   // back up all decisions that were made
   UnApplyDeltaArray( gpCellLayer );   

   m_pDeltaArray->ClearDeltas();

   int emCount = GetEvaluatorCount();
   int apCount = GetModelProcessCount();

   m_emFirstUnseenDelta.SetSize( emCount, 0 );
   for ( int i=0; i<emCount; i++ )
      m_emFirstUnseenDelta[i] = 0;

   m_apFirstUnseenDelta.SetSize( apCount, 0 );
   for ( int i=0; i<apCount; i++ )
      m_apFirstUnseenDelta[i] = 0;

   return true;
   }

void Sandbox::InitRunModels()
   {
   int emCount = (int) m_evaluatorArray.GetSize();
   int apCount = (int) m_modelProcessArray.GetSize();

   // initilize envContext
   m_envContext.pMapLayer     = gpCellLayer;
   m_envContext.run           = m_currentRun;
   m_envContext.startYear     = m_startYear;
   m_envContext.endYear       = m_endYear;
   m_envContext.currentYear   = m_currentYear;
   m_envContext.yearOfRun     = 0;
   m_envContext.pActorManager = gpActorManager;
   m_envContext.pPolicyManager= gpPolicyManager;
   m_envContext.pLulcTree     = &m_lulcTree;
   m_envContext.showMessages  = m_showMessages;

   // initialize EMs
   for ( int i=0; i < emCount; i++ )
      {
      EnvEvaluator *pInfo = m_evaluatorArray[ i ];
      if ( pInfo->m_use )
         {
         //INITRUNFN initRunFn = pInfo->initRunFn;
         //
         //if ( initRunFn != NULL )
            {
            //m_envContext.col = pInfo->col;
            m_envContext.id = pInfo->m_id;
            m_envContext.pEnvExtension = pInfo;
            BOOL ok = pInfo->InitRun( &m_envContext, m_envContext.pEnvModel->m_resetInfo.useInitialSeed );
            if ( !ok )
               pInfo->m_use = false;
            }
         }
      }

   // initialize APs
   for ( int i=0; i < apCount; i++ )
      {
      EnvModelProcess *pInfo = m_modelProcessArray[ i ];
      if ( pInfo->m_use )
         {
         //INITRUNFN initRunFn = pInfo->initRunFn;
         //
         //if ( initRunFn != NULL )
            {
            m_envContext.id = pInfo->m_id;
            //m_envContext.col = pInfo->col;
            m_envContext.pEnvExtension = pInfo;
            BOOL ok = pInfo->InitRun( &m_envContext, m_envContext.pEnvModel->m_resetInfo.useInitialSeed );
            if ( !ok )
               {
               CString msg;
               msg.Format( "The APInitRun(.) function of %s returned false.  This autonomous process will not be used.", pInfo->m_name );
               MessageBox( GetActiveWindow(), msg, "Sandbox" , MB_ICONERROR | MB_OK );

               pInfo->m_use = false;
               }
            }
         }
      }
   }

void Sandbox::PushModels()
   {
 /////////////  // push EMs
 /////////////  for ( int i=0; i < GetEvaluatorCount(); i++ )
 /////////////     {
 /////////////     if ( GetEvaluatorInfo(i)->m_use )
 /////////////        {
 /////////////        //PUSHFN pushFn = GetEvaluatorInfo(i)->pushFn;
 /////////////        //
 /////////////        //m_envContext.pExtensionInfo = GetEvaluatorInfo( i );
 /////////////        //
 /////////////        //if ( pushFn != NULL )
 /////////////        //   pushFn( &m_envContext );
 /////////////        }
 /////////////     }
 /////////////
 /////////////  // push APs
 /////////////  for ( int i=0; i < m_modelProcessArray.GetSize(); i++ )
 /////////////     {
 /////////////     //EnvModelProcess *pInfo = m_modelProcessArray[ i ];
 /////////////     //m_envContext.id = pInfo->m_id;
 /////////////     //m_envContext.col = pInfo->col;
 /////////////     //m_envContext.pExtensionInfo = pInfo;
 /////////////     //
 /////////////     //if ( pInfo->use && pInfo->sandbox && pInfo->pushFn != NULL )
 /////////////     //   pInfo->pushFn( &m_envContext );
 /////////////     //}
   }

void Sandbox::PopModels()
   {
///   // pop EMs
///   for ( int i=0; i < GetEvaluatorCount(); i++ )
///      {
///      if ( GetEvaluatorInfo(i)->use )
///         {
///         POPFN popFn = GetEvaluatorInfo(i)->popFn;
///         m_envContext.pExtensionInfo = GetEvaluatorInfo( i );
///
///         if ( popFn != NULL )
///            popFn( &m_envContext );
///         }
///      }
///
///   // pop APs
///   for ( int i=0; i < m_modelProcessArray.GetSize(); i++ )
///      {
///      EnvModelProcess *pInfo = m_modelProcessArray[ i ];
///      m_envContext.id = pInfo->m_id;
///      m_envContext.col = pInfo->col;
///      m_envContext.pExtensionInfo = pInfo;
///
///      if ( pInfo->use && pInfo->sandbox && pInfo->popFn != NULL )
///         pInfo->popFn( &m_envContext );
///      }
   }

void Sandbox::CalculateGoalScores( EnvPolicy *pPolicy,  bool *useMetagoalArray /*=NULL*/ )
   {
   // this is the high-level entry point for running the sandbox to calculate goal score
   // for the specified policy and metagoals
   //
   // categorical imperative
   //
   // apply the policy to every cell that has an actor and that satisfies the policies site constraint
   // evaluate the landscape
   // ...
   //
   //
   // Push Models
   //TRACE( "    push\n" );
   //PushModels();  - obsolete - remove jpb 2/15/06

   SriRandomPath rPath( gpCellLayer->GetRecordCount() );

   int cells = int( m_percentCoverage*float( gpCellLayer->GetPolygonCount() ) );
   //int cell = rPath.first( m_randUnif.GetUnif01() );
   int actor = -1;

   //CUIntArray appliedCellArray;  // array of polys where policy was applied, used for ALPS restriction
   m_currentYear = 0;

   // set up env extra data for models/ALPS/POP
   m_envContext.pMapLayer     = gpCellLayer;
   m_envContext.run           = m_currentRun;
   m_envContext.currentYear   = m_currentYear;
   m_envContext.yearOfRun     = m_currentYear - m_startYear;
   m_envContext.pActorManager = gpActorManager;
   m_envContext.pPolicyManager= gpPolicyManager;
   m_envContext.pDeltaArray   = m_pDeltaArray;
   m_envContext.pLulcTree     = &m_lulcTree;
   m_envContext.showMessages  = m_showMessages = ( m_debug > 0 );
   m_envContext.logMsgLevel   = m_logMsgLevel > 0 ? m_logMsgLevel : 0xFFFF;

   // set up any scheduled policies
   gpPolicyManager->BuildPolicySchedule();

   // take care of any schedule policies
   int yearsToRun = m_yearsToRun;
   for ( int year=0; year < yearsToRun; year++ )
      {
      m_currentYear = year;
      ApplyScheduledPolicies();
      }

   m_currentYear = 0;

   //---- Step 1. Apply Policy where applicable -------
   for ( int i=0; i < cells; i++ )
      {
      int cell=i;
      bool ok    = gpCellLayer->GetData( cell, m_colActor, actor );
      if ( ok && actor >= 0 ) // only consider cells that have an actor
         {
         if ( DoesPolicyApply( pPolicy, cell ) )
            {
            //appliedCellArray.Add( cell );
            ApplyPolicy( pPolicy, cell, 0 );
            }
         }
      //cell = rPath.next();
      }

   ApplyDeltaArray( gpCellLayer );   // for any policy applications

   // Step 2 - run autonomous processes
   ASSERT( m_pScenario != NULL );
   m_pScenario->SetScenarioVars( false );
   //InitRun();     // sets m_resetInfo.doReset = false, set it to true here by default
   m_runStatus = RS_RUNNING;

   gpMain->SetStatusMsg( "Year 0" );
   gpMain->SetRunProgressRange( 0, m_yearsToRun );

   // Year Loop
   int apCount = (int) m_modelProcessArray.GetSize();

   for ( int year=0; year < yearsToRun; year++ )
      {
      MSG  msg;
      while( PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE ) )
         {
         TranslateMessage( &msg );
         DispatchMessage( &msg );
         }

      gpMain->SetRunProgressPos( year );
      m_envContext.currentYear = m_currentYear;
      m_envContext.yearOfRun   = m_currentYear - m_startYear;

      // call APs
      RunModelProcesses( false );    // pre-year
      RunModelProcesses( true );     // post-year

      if ( m_evalFreq > 0 && (m_currentYear %m_evalFreq ) )
         RunEvaluation();

      m_currentYear++;
      } // for ( int year=0; year<m_yearsToRun; year++ )


   for ( int g=0; g < pPolicy->GetGoalCount(); g++ )
      {
      if ( useMetagoalArray == NULL || useMetagoalArray[ g ] == true )
         {
         ASSERT(this->GetMetagoalCount() == pPolicy->GetGoalCount() );
         int modelIndex = this->GetEvaluatorIndexFromMetagoalIndex( g );
         if ( modelIndex >= 0 )
            {
            float endMetric   = GetLandscapeMetric( modelIndex );
            float startMetric = m_initialMetrics.GetAt( modelIndex );
            float newscore    = endMetric - startMetric;

            if ( newscore < -3.0f )
               newscore = -3.0f;
            else if ( newscore > 3.0f )
               newscore = 3.0f;
            }
  
         //GOAL_SCORE &gs = pPolicy->m_goalScores.GetAt(g);
         //gs.globalScore = newscore;
         //gs.localScore  = newscore;
         }
      }

   // Pop Eval Models
   //TRACE( "    pop\n" );
   //PopModels();

   //TRACE( "    reset\n" );
   Reset();    // UnApply's deltaArray
   }




   //--------------------------------------
   // SLIGHTLY DIFFERENT USE CASE FROM ABOVE--LEARN MODEL BEHAVIOR VIA emStats
   void Sandbox::CalculateGoalScores(  CArray<EnvPolicy *>  & schedPolicy, EnvPolicy *pPolicy, EvaluatorLearningStats * emStats,  bool * useModelArray)
      {
      ASSERT(useModelArray != NULL);
      int i, g, idx;
      CString msg;
      msg.Format("%d:%s:Sandbox.Learning", pPolicy->m_id, pPolicy->m_name );
      TRACE( "%s\n", msg );
      gpMain->SetStatusMsg(msg);

      SriRandomPath rPath( gpCellLayer->GetRecordCount() );

      int cells = int( m_percentCoverage*float( gpCellLayer->GetPolygonCount() ) );
      int cell = rPath.first( m_randUnif.GetUnif01() );
      int actor = -1;

      // set up env extra data for models/ALPS/POP
      m_envContext.pMapLayer      = gpCellLayer;
      m_envContext.run            = m_currentRun;
      m_envContext.currentYear    = m_currentYear;
      m_envContext.yearOfRun      = m_currentYear - m_startYear;
      m_envContext.pActorManager  = gpActorManager;
      m_envContext.pPolicyManager = gpPolicyManager;
      m_envContext.pDeltaArray    = m_pDeltaArray;
      m_envContext.pLulcTree      = &m_lulcTree;
      m_envContext.showMessages   = m_showMessages = ( m_debug > 0 );
      m_envContext.logMsgLevel    = m_logMsgLevel > 0 ? m_logMsgLevel : 0xFFFF;

      // set up any scheduled policies
      //
      gpPolicyManager->ClearPolicySchedule();
      for ( i=0; i < schedPolicy.GetCount(); ++i )
         {
         EnvPolicy * pPolicy = schedPolicy.GetAt(i);
         gpPolicyManager->AddPolicySchedule(pPolicy->m_startDate, pPolicy);
         }

      for ( m_currentYear = 0; m_currentYear < m_yearsToRun;  m_currentYear++ )
         {
         ApplyScheduledPolicies();
         }

      m_currentYear = 0;

      //---- Step 1. Apply Policy where applicable -------
      for ( i=0; i < cells; i++ )
         {
         bool ok    = gpCellLayer->GetData( cell, m_colActor, actor );
         if ( ok && actor >= 0 ) // only consider cells that have an actor
            {
            if ( DoesPolicyApply( pPolicy, cell ) )
               {
               ApplyPolicy( pPolicy, cell, -1 );
               }
            }
         cell = rPath.next();
         }
      ApplyDeltaArray( gpCellLayer );   // for any policy applications
      // Step 2 - run autonomous processes
      ASSERT( m_pScenario != NULL );
      m_pScenario->SetScenarioVars( false );
      m_runStatus = RS_RUNNING;

      gpMain->SetRunProgressRange( 0, m_yearsToRun );

      // Year Loop
      int apCount = (int) m_modelProcessArray.GetSize();

      for ( m_currentYear=0; m_currentYear < emStats->m_yearsToRun; m_currentYear++ )
         {
         m_envContext.currentYear = m_currentYear;
         m_envContext.yearOfRun   = m_currentYear - m_startYear;

         MSG  msg;
         while( PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE ) )
            {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
            }
         gpMain->SetRunProgressPos( m_currentYear );

         RunModelProcesses( false );
         RunModelProcesses( true );

         // Evaluation Year
         if (1 == emStats->DoEvalYearBase0(m_currentYear))
            {
            VERIFY( RunEvaluation() );

            for ( g=0, idx=0; g < EnvModel::GetEvaluatorCount(); g++ )
               {
               if ( useModelArray[ g ] == true )
                  {
                  float score = GetLandscapeMetric(g);// - m_initialMetrics.GetAt(g);
                  if ( score < -3.0f )
                     score = -3.0f;
                  else if ( score > 3.0f )
                     score = 3.0f;

                  emStats->operator()(idx++, score); // emStats perhaps without a full metagoal list
                  }
               }
            TRACE( emStats->Trace() );
            }
         } 
      TRACE( "    reset\n" );
      Reset();
      gpMain->SetStatusMsg("Done");
      gpMain->SetRunProgressPos( 0 );

      }

