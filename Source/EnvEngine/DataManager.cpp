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
#ifndef NO_MFC
#include <direct.h>
#else
#include<unistd.h>
#endif
#include "DataManager.h"
//#include "envdoc.h"
#include "EnvConstants.h"
#include "EnvPolicy.h"
#include "Actor.h"
#include "DeltaArray.h"
#include "ColumnTrace.h"
#include "Scenario.h"
//#include "mainfrm.h"
//#include "SocialNetwork.h"

#include <Maplayer.h>
#include <MAP.h>
#include <Report.h>
#include <Path.h>
#include <PathManager.h>
#include <misc.h>
#ifdef _WINDOWS
//#include <raster.h>
#endif
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

using std::vector;
using std::string;
using std::ostringstream;

#pragma warning(suppress: 6031)


//LU_STATUS GetLulcStatus( int cell, int lulcC, int colDistStr );

FDataObj* DataObjMatrix::GetDataObj(int run, int year)
   {
   if (run >= this->GetSize())
      return nullptr;

   DataObjArray* pDOA = this->GetAt(year);

   if (pDOA == nullptr)
      return nullptr;

   if (year >= pDOA->GetSize())
      return nullptr;

   return (FDataObj*)pDOA->GetAt(year);
   }


bool DataObjMatrix::SetDataObj(int run, int year, FDataObj* pDataObj)
   {
   if (pDataObj == nullptr)
      return false;  

   // do we need to add a run? (a row)
   if (run >= this->GetSize())
      {
      ASSERT(run == this->GetSize());
      this->Add(new DataObjArray);
      }

   // do we need to add a new year?
   DataObjArray* pDOA = this->GetAt(run);
   ASSERT(pDOA != nullptr);

   if (year >= pDOA->GetSize())
      {
      ASSERT(year == pDOA->GetSize());
      pDOA->Add(new FDataObj(*pDataObj));
      }
   else
      {
      ASSERT(pDOA->GetAt(year) != nullptr);
      delete pDOA->GetAt(year);
      pDOA->SetAt(year, new FDataObj(*pDataObj));
      }

   return true;
   }


DataManager::DataManager(EnvModel* pModel)
   : m_pEnvModel(pModel)
   , m_collectActorData(false)
   , m_collectLandscapeScoreData(true)
   , m_pDeltaArray(nullptr)
   , multiRunDecadalMapsModulus(0)
   , m_modelDataObjMatrices(nullptr)
   {
   for (int i = 0; i < DT_LAST; i++)
      m_currentDataObjs[i] = nullptr;
   }

DataManager::~DataManager(void)
   {
   RemoveAllData();

   m_pEnvModel = nullptr;
   }


DataObj* DataManager::GetDataObj(DM_TYPE type, int run /*=-1*/)
   {
   DataObj* pData = nullptr;
   if (run < 0)
      pData = m_currentDataObjs[type];
   else
      pData = m_dataObjs[type].GetAt(run);

   return pData;
   }


DataObj* DataManager::GetMultiRunDataObj(DM_MULTITYPE type, int run /*=-1*/)
   {
   if (this->m_collectMultiRunOutput== false)
      return nullptr;

   int _type = (int)type;
   if (run < 0)
      return m_currentMultiRunDataObjs[_type];
   else if (this - m_collectMultiRunOutput)
      return m_multiRunDataObjs[_type].GetAt(run);
   else
      return nullptr;
   }


// get a particular time range from a column of data
// start, end are INCLUSIVE and represent years
bool DataManager::SubsetData(DM_TYPE type, int run, int col, int start, int end, CArray<float, float>& data)
   {
   DataObj* pData = GetDataObj(type, run);

   if (pData == nullptr)
      return false;

   data.RemoveAll();
   data.SetSize(end - start + 1);
   int index = 0;
   for (int i = start; i <= end; i++)
      data[index++] = pData->GetAsFloat(col, i);

   return true;
   }


void DataManager::RemoveAllData(void)
   {
   // NOTE: DataObjs are removed by the DataObjArray destructor
   for (int i = 0; i < m_deltaArrayArray.GetSize(); i++)
      if (m_deltaArrayArray[i] != nullptr)
         delete m_deltaArrayArray[i];

   m_deltaArrayArray.RemoveAll();
   m_pDeltaArray = nullptr;

   for (int i = 0; i < DT_LAST; i++)
      {
      m_currentDataObjs[i] = nullptr;
      m_dataObjs[i].RemoveAllData();
      }

   for (int i = 0; i < DT_MULTI_LAST; i++)
      {
      if (this->m_collectMultiRunOutput)
         {
         m_currentMultiRunDataObjs[i] = nullptr;
         m_multiRunDataObjs[i].RemoveAllData();
         }
      }

   m_runInfoArray.RemoveAll();
   m_multirunInfoArray.RemoveAll();

   if (this->m_collectMultiRunOutput && m_modelDataObjMatrices != nullptr)
      {
      for (int i = 0; i < this->m_pEnvModel->GetResultsCount(); i++)
         if (m_modelDataObjMatrices[i] != nullptr)
            delete m_modelDataObjMatrices[i];

      delete[] m_modelDataObjMatrices;
      m_modelDataObjMatrices = nullptr;
      }
   }


bool DataManager::CreateDataObjects() 
   {
   if (m_pEnvModel == nullptr)
      return false;

   int rows = m_pEnvModel->m_yearsToRun + 1;  // +1 for initial data
   int resultsCount = this->m_pEnvModel->GetResultsCount();

   //--------------- landscape scores for each goal -------------
   if (m_collectLandscapeScoreData)
      {
      Report::StatusMsg("Creating data objects...Landscape scores");

      FDataObj* pEvalScoresData = new FDataObj(resultsCount + 1, rows, U_YEARS);
      pEvalScoresData->SetName("Landscape Evaluative Statistics (Scaled)");
      pEvalScoresData->SetLabel(0, "Time (years)");
      for (int i = 0; i < resultsCount; i++)
         {
         int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
         pEvalScoresData->SetLabel(i + 1, this->m_pEnvModel->GetEvaluatorInfo(modelIndex)->m_name);
         }

      m_currentDataObjs[DT_EVAL_SCORES] = pEvalScoresData;
      m_dataObjs[DT_EVAL_SCORES].Add(pEvalScoresData);

      FDataObj* pRawScoresData = new FDataObj(resultsCount + 1, rows, U_YEARS);
      pRawScoresData->SetName("Landscape Evaluative Statistics (Raw Scores)");
      pRawScoresData->SetLabel(0, "Time (years)");
      for (int i = 0; i < resultsCount; i++)
         {
         int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
         pRawScoresData->SetLabel(i + 1, this->m_pEnvModel->GetEvaluatorInfo(modelIndex)->m_name);
         }

      m_currentDataObjs[DT_EVAL_RAWSCORES] = pRawScoresData;
      m_dataObjs[DT_EVAL_RAWSCORES].Add(pRawScoresData);
      }

   // model output variables (autonomous processes, eval models, appvars)
   int outVarCount = m_pEnvModel->GetOutputVarCount(OVT_MODEL | OVT_EVALUATOR | OVT_PDATAOBJ);
   int outputAppVarCount = 0;
   int totalAppVarCount = m_pEnvModel->GetAppVarCount();
   for (int i = 0; i < totalAppVarCount; i++)
      {
      AppVar* pVar = m_pEnvModel->GetAppVar(i);
      if (pVar->m_avType == AVT_APP && pVar->IsGlobal())
         outputAppVarCount++;
      }

   int totalOutputCount = outVarCount + outputAppVarCount;

   if (totalOutputCount > 0)
      {
      Report::StatusMsg("Creating data objects...Output Variables");

      VDataObj* m_pModelOutputsData = new VDataObj(totalOutputCount + 1, rows, U_YEARS);
      m_pModelOutputsData->SetName("ModelOutput");
      m_pModelOutputsData->SetLabel(0, "Year");
      int col = 1;
      for (int i = 0; i < this->m_pEnvModel->GetModelProcessCount(); i++)
         {
         EnvModelProcess* pInfo = this->m_pEnvModel->GetModelProcessInfo(i);

         if (pInfo->m_use)
            {
            MODEL_VAR* modelVarArray = nullptr;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR& mv = modelVarArray[j];
               if (mv.pVar != nullptr)
                  {
                  CString label(pInfo->m_name);
                  label += ".";
                  label += mv.name;
                  m_pModelOutputsData->SetLabel(col, label);
                  col++;
                  }
               }
            }
         }

      for (int i = 0; i < this->m_pEnvModel->GetEvaluatorCount(); i++)
         {
         EnvEvaluator* pInfo = this->m_pEnvModel->GetEvaluatorInfo(i);
         if (pInfo->m_use)
            {
            MODEL_VAR* modelVarArray = nullptr;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR& mv = modelVarArray[j];
               if (mv.pVar != nullptr)
                  {
                  CString label(pInfo->m_name);
                  label += ".";
                  label += mv.name;
                  m_pModelOutputsData->SetLabel(col, label);
                  col++;
                  }
               }
            }
         }

      for (int i = 0; i < totalAppVarCount; i++)
         {
         AppVar* pVar = m_pEnvModel->GetAppVar(i);

         if (pVar->m_avType == AVT_APP && pVar->IsGlobal())
            m_pModelOutputsData->SetLabel(col++, pVar->m_name);
         }

      m_currentDataObjs[DT_MODEL_OUTPUTS] = m_pModelOutputsData;
      m_dataObjs[DT_MODEL_OUTPUTS].Add(m_pModelOutputsData);
      }  // end of:  if ( outputCount > 0 )

   // results model data objs.  Note: We just allocate an array of pointers to DataObjMatrices here.  Any 
   //  needed DataObjMatrices are allocated during CollectData();
   Report::StatusMsg("Creating data objects...Model data objects");

   if (m_modelDataObjMatrices == nullptr)
      m_modelDataObjMatrices = new DataObjMatrix * [resultsCount];

   for (int i = 0; i < resultsCount; i++)
      m_modelDataObjMatrices[i] = nullptr;

   //----------- average actor values -----------------
   if (m_collectActorData)
      {
      Report::StatusMsg("Creating data objects...Actor Values");

      // get the number of actor values in use in valueCount
      int valueCount = this->m_pEnvModel->GetActorValueCount();

      // the data object stored will have "valueCount" columns for each actor group, and a row for each year
      int groupCount = m_pEnvModel->m_pActorManager->GetActorGroupCount();
      FDataObj* pActorWtData = new FDataObj(valueCount * groupCount + 1, rows, U_YEARS);
      pActorWtData->SetName("Actor Value Trajectories");
      pActorWtData->SetLabel(0, "Time (years)");
      int col = 1;
      CString label;
      for (int j = 0; j < groupCount; j++)
         {
         ActorGroup* pGroup = m_pEnvModel->m_pActorManager->GetActorGroup(j);

         for (int i = 0; i < valueCount; i++)
            {
            METAGOAL* pInfo = this->m_pEnvModel->GetActorValueMetagoalInfo(i);
            CString label = pInfo->name;
            label += "-";

            if (pGroup && pGroup->m_name)
               label += pGroup->m_name;

            pActorWtData->SetLabel(col++, label);
            }
         }

      m_currentDataObjs[DT_ACTOR_WTS] = pActorWtData;
      m_dataObjs[DT_ACTOR_WTS].Add(pActorWtData);

      //  actor counts      
      Report::StatusMsg("Creating data objects...Actor Counts");
      FDataObj* pActorCountData = new FDataObj(groupCount + 1, rows, U_YEARS);
      pActorCountData->SetName("Actor Counts");
      pActorCountData->SetLabel(0, "Time (years)");
      col = 1;
      for (int j = 0; j < groupCount; j++)
         {
         ActorGroup* pGroup = m_pEnvModel->m_pActorManager->GetActorGroup(j);
         label = pGroup->m_name;
         pActorCountData->SetLabel(col++, label);
         }

      m_currentDataObjs[DT_ACTOR_COUNTS] = pActorCountData;
      m_dataObjs[DT_ACTOR_COUNTS].Add(pActorCountData);
      }


   //------------- policy summary information ---------------
   // note: this gets populatede at the end of a run only, but we set it up here.

   // allocate a data object
   //Report::StatusMsg( "Creating data objects...Policy summaries" );

   int policyCount = m_pEnvModel->m_pPolicyManager->GetPolicyCount();
   VDataObj* pPolicySummaryData = new VDataObj(8, policyCount, U_UNDEFINED);
   pPolicySummaryData->SetName("Policy Results Summary");
   //pPolicySummaryData->SetLabel( 0, "Policy Name" );
   //pPolicySummaryData->SetLabel( 1, "Policy Index" );
   //pPolicySummaryData->SetLabel( 2, "Potential Area" );


   pPolicySummaryData->SetName("Policy Results Summary");
   pPolicySummaryData->SetLabel(0, "Policy");
   pPolicySummaryData->SetLabel(1, "Applied Count");
   pPolicySummaryData->SetLabel(2, "Potential Area (%)");
   pPolicySummaryData->SetLabel(3, "Actual Area (%)");
   pPolicySummaryData->SetLabel(4, "Actual Area");
   pPolicySummaryData->SetLabel(5, "% Potential Area Used");
   pPolicySummaryData->SetLabel(6, "Expansion Area");
   pPolicySummaryData->SetLabel(7, "Total Cost");
   // add probe stuff

   VData value;

   // initialize columns
   for (int j = 0; j < policyCount; j++)
      {
      EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(j);

      if (pPolicy->m_use)
         value = 0;
      else
         value = "---";
      pPolicySummaryData->Set(0, j, VData(m_pEnvModel->m_pPolicyManager->GetPolicy(j)->m_name));
      pPolicySummaryData->Set(1, j, value);
      pPolicySummaryData->Set(2, j, value);
      pPolicySummaryData->Set(3, j, value);
      pPolicySummaryData->Set(4, j, value);
      pPolicySummaryData->Set(5, j, value);
      pPolicySummaryData->Set(6, j, value);
      pPolicySummaryData->Set(7, j, value);
      }  // end of: for ( i < policyCount )

   this->m_pEnvModel->m_pIDULayer->ClearSelection();
   m_currentDataObjs[DT_POLICY_SUMMARY] = pPolicySummaryData;
   m_dataObjs[DT_POLICY_SUMMARY].Add(pPolicySummaryData);

   // global Constraints
   int gcCount = m_pEnvModel->m_pPolicyManager->GetGlobalConstraintCount();
   FDataObj* pGCData = new FDataObj((gcCount * 4) + 1, rows, U_YEARS);
   pGCData->SetName("Global Constraints Summary");
   pGCData->SetLabel(0, "Time (years)");
   int col = 1;
   for (int i = 0; i < gcCount; i++)
      {
      GlobalConstraint* pGC = m_pEnvModel->m_pPolicyManager->GetGlobalConstraint(i);
      CString label(pGC->m_name);
      label += ": Total Costs";
      pGCData->SetLabel(col++, label);

      label = pGC->m_name;
      label += ": Initial Costs";
      pGCData->SetLabel(col++, label);

      label = pGC->m_name;
      label += ": Maintenance Costs";
      pGCData->SetLabel(col++, label);

      label = pGC->m_name;
      label += ": Total Budget";
      pGCData->SetLabel(col++, label);
      }

   m_currentDataObjs[DT_GLOBAL_CONSTRAINTS] = pGCData;
   m_dataObjs[DT_GLOBAL_CONSTRAINTS].Add(pGCData);

   //----- policy stats -----------------------------
   FDataObj* pPolicyStatsData = new FDataObj((policyCount * 6) + 1, rows, U_YEARS);
   pPolicyStatsData->SetName("Policy Stats");
   pPolicyStatsData->SetLabel(0, "Time (years)");
   col = 1;
   for (int i = 0; i < policyCount; i++)
      {
      EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(i);

      CString label = pPolicy->m_name;
      label += ": Rejected Count (Policy Constraint)";
      pPolicyStatsData->SetLabel(col++, label);

      label = pPolicy->m_name;
      label += ": Rejected Count (No Cost Info)";
      pPolicyStatsData->SetLabel(col++, label);

      label = pPolicy->m_name;
      label += ": Rejected Count (Site Attr)";
      pPolicyStatsData->SetLabel(col++, label);

      label = pPolicy->m_name;
      label += ": Passed Count";
      pPolicyStatsData->SetLabel(col++, label);

      label = pPolicy->m_name;
      label += ": Applied Count";
      pPolicyStatsData->SetLabel(col++, label);

      label = pPolicy->m_name;
      label += ": No Outcome Count";
      pPolicyStatsData->SetLabel(col++, label);
      }

   m_currentDataObjs[DT_POLICY_STATS] = pPolicyStatsData;
   m_dataObjs[DT_POLICY_STATS].Add(pPolicyStatsData);
   /*
   //----- social network -----------------------------
   FDataObj *pSocialNetworkData = nullptr;
   if ( m_pEnvModel->m_pSocialNetwork != nullptr )
      {
      int layerCount  = m_pEnvModel->m_pSocialNetwork->GetLayerCount();
      int metricCount = m_pEnvModel->m_pSocialNetwork->GetMetricCount();
      pSocialNetworkData = new FDataObj( 1+layerCount*metricCount, rows, U_YEARS);
      pSocialNetworkData->SetName( "Social Network Metrics" );
      pSocialNetworkData->SetLabel( 0, "Time (years)" );
      col = 1;
      for ( int l=0; l < layerCount; l++ )
         {
         for ( int i=0; i < metricCount; i++ )
            {
            CString metric;
            m_pEnvModel->m_pSocialNetwork->GetMetricLabel( i, metric );

            CString label( m_pEnvModel->m_pSocialNetwork->GetLayer( l )->m_name.c_str() );
            label += ".";
            label += metric;
            pSocialNetworkData->SetLabel( col++, label );
            }
         }
      }

   m_currentDataObjs[ DT_SOCIAL_NETWORK ] = pSocialNetworkData;
   m_dataObjs[ DT_SOCIAL_NETWORK ].Add( pSocialNetworkData );
   */
   RUN_INFO ri(m_pEnvModel->GetScenario(), m_pEnvModel->GetScenario()->m_runCount, m_pEnvModel->m_startYear, m_pEnvModel->m_endYear);
   m_runInfoArray.Add(ri);

   //-- All done.....
   return true;
   }


void DataManager::SetDataSize(int rows)
   {
   for (int i = 0; i < DT_LAST; i++)
      {
      if (DM_TYPE(i) != DT_POLICY_SUMMARY)    // exclude policy summary, since it is not a time series
         {
         DataObj* pData = m_currentDataObjs[(DM_TYPE)i];

         if (pData != nullptr)
            {
            if (pData->GetRowCount() != rows)
               {
               int cols = pData->GetColCount();
               pData->SetSize(cols, rows);
               }
            }
         }
      }
   }

bool DataManager::AppendRows(int rows)
   {
   for (int i = 0; i < DT_LAST; i++)
      {
      DataObj* pData = m_currentDataObjs[(DM_TYPE)i];

      if (pData != nullptr)
         pData->AppendRows(rows);
      }

   return true;
   }


bool DataManager::CollectData(int yearOfRun)
   {
   int row = yearOfRun;
   int currentYear = m_pEnvModel->m_currentYear;

   int resultsCount = this->m_pEnvModel->GetResultsCount();
   int valueCount = this->m_pEnvModel->GetActorValueCount();

   //--------------------------------------------------------
   //------- First, collect model results -------------------
   //--------------------------------------------------------

   // scaled
   FDataObj* pEvalScoresData = (FDataObj*)m_currentDataObjs[DT_EVAL_SCORES];
   if (pEvalScoresData != nullptr)
      {
      pEvalScoresData->Set(0, row, (float)currentYear); // yearOfRun );
      for (int i = 0; i < resultsCount; i++)
         {
         int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
         pEvalScoresData->Set(i + 1, row, m_pEnvModel->GetLandscapeMetric(modelIndex));
         }
      }

   // raw scores
   FDataObj* pRawScoresData = (FDataObj*)m_currentDataObjs[DT_EVAL_RAWSCORES];
   if (pRawScoresData != nullptr)
      {
      pRawScoresData->Set(0, row, (float)currentYear); //yearOfRun );
      for (int i = 0; i < resultsCount; i++)
         {
         int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
         pRawScoresData->Set(i + 1, row, m_pEnvModel->GetLandscapeMetricRaw(modelIndex));
         }
      }

   // model output variables (autonomous processes followed by eval models)
   int outVarCount = this->m_pEnvModel->GetOutputVarCount(OVT_MODEL | OVT_EVALUATOR | OVT_PDATAOBJ);
   int outputAppVarCount = 0;
   int totalAppVarCount = m_pEnvModel->GetAppVarCount();
   for (int i = 0; i < totalAppVarCount; i++)
      {
      AppVar* pVar = m_pEnvModel->GetAppVar(i);
      if (pVar->m_avType == AVT_APP && pVar->IsGlobal())
         outputAppVarCount++;
      }

   int totalOutputCount = outVarCount + outputAppVarCount;

   if (totalOutputCount > 0)
      {
      VDataObj* m_pModelOutputsData = (VDataObj*)m_currentDataObjs[DT_MODEL_OUTPUTS];
      ASSERT(m_pModelOutputsData != nullptr);
      m_pModelOutputsData->Set(0, row, (float)currentYear); // yearOfRun );

      int col = 1;
      for (int i = 0; i < this->m_pEnvModel->GetModelProcessCount(); i++)
         {
         EnvModelProcess* pInfo = this->m_pEnvModel->GetModelProcessInfo(i);
         if (pInfo->m_use)
            {
            MODEL_VAR* modelVarArray = nullptr;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR& mv = modelVarArray[j];
               //VData v( mv.pVar, mv.type );
               if (mv.pVar != nullptr)
                  {
                  m_pModelOutputsData->Set(col, row, VData(mv.pVar, mv.type, false));  // convert to non-ptr type
                  col++;
                  }
               }
            }
         }

      for (int i = 0; i < this->m_pEnvModel->GetEvaluatorCount(); i++)
         {
         EnvEvaluator* pInfo = this->m_pEnvModel->GetEvaluatorInfo(i);
         if (pInfo->m_use)
            {
            MODEL_VAR* modelVarArray = nullptr;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR& mv = modelVarArray[j];
               //VData v( mv.pVar, mv.type );
               if (mv.pVar != nullptr)
                  {
                  m_pModelOutputsData->Set(col, row, VData(mv.pVar, mv.type, false));
                  col++;
                  }
               }
            }
         }

      int totalAppVarCount = m_pEnvModel->GetAppVarCount(0);
      for (int i = 0; i < totalAppVarCount; i++)
         {
         AppVar* pVar = m_pEnvModel->GetAppVar(i);

         if (pVar->m_avType == AVT_APP && pVar->IsGlobal())
            {
            m_pModelOutputsData->Set(col, row, pVar->GetValue());
            col++;
            }
         }
      }  // end of:  if ( outputCount > 0 )


   // data objects
   // note: DataObjMatrix is a matrix of FDataObj ptrs (representing DataObjs from one eval model)
   //       Each DataObjMatrix row is a run, each col is a point in time
   //       m_modelDataObjs is an array of DataObjectMatrices, one for each eval model that generates results
   ASSERT(m_modelDataObjMatrices != nullptr);
   for (int i = 0; i < resultsCount; i++)
      {
      int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);

      EnvEvaluator* m_pModelInfo = m_pEnvModel->GetEvaluatorInfo(modelIndex);

      if (m_pModelInfo->m_pDataObj != nullptr)
         {
         DataObjMatrix* pDOM = nullptr;
         if (m_modelDataObjMatrices[i] == nullptr)
            {
            pDOM = new DataObjMatrix;
            m_modelDataObjMatrices[i] = pDOM;
            }
         else
            pDOM = m_modelDataObjMatrices[i];

         // the row of this DOM corresponds to the current run.  the column corresponds to the current year.
         int run = m_pEnvModel->m_currentRun;
         pDOM->SetDataObj(run, yearOfRun, m_pModelInfo->m_pDataObj);
         }  // end of: id ( pDataObj != nullptr )
      }


   //------------ actor weights --------------  
   // note:  These are the average weights across the actor population
   FDataObj* pActorWtData = (FDataObj*)m_currentDataObjs[DT_ACTOR_WTS];
   ActorManager* pActorManager = m_pEnvModel->m_pActorManager;

   if (pActorWtData != nullptr)
      {
      int actorCount = pActorManager->GetActorCount();
      int groupCount = pActorManager->GetActorGroupCount();

      if (actorCount > 0)
         {
         int actorValueCount = pActorManager->GetActorGroup(0)->GetValueCount();
         //ASSERT( actorValueCount == valueCount );

         float* wts = new float[valueCount * groupCount];
         memset((void*)wts, 0, valueCount * groupCount * sizeof(float));  // zero it out

         for (int i = 0; i < actorCount; i++)
            {
            Actor* pActor = pActorManager->GetActor(i);

            if (pActor && pActor->GetPolyCount() > 0)
               {
               int group = pActorManager->GetActorGroupIndexFromID(pActor->GetID());
               ASSERT(0 <= group && group < groupCount);

               for (int j = 0; j < valueCount; j++)
                  {
                  ASSERT(0 <= group * valueCount + j && group * valueCount + j < valueCount* groupCount);
                  wts[group * valueCount + j] += pActor->GetValue(j);
                  }
               }
            }

         for (int i = 0; i < groupCount; i++)
            {
            int value = 0;
            for (int j = 0; j < valueCount; j++)
               {
               ASSERT(0 <= i * resultsCount + j && i * valueCount + j < valueCount* groupCount);
               }
            }

         pActorWtData->Set(0, row, (float)currentYear); // yearOfRun );
         int col = 1;
         for (int i = 0; i < groupCount; i++)
            {
            int value = 0;
            for (int j = 0; j < valueCount; j++)
               pActorWtData->Set(col++, row, wts[i * valueCount + j]);
            }

         delete[] wts;
         }  // end of: if ( actorCount > 0 )
      }  // end of: if ( pActorWtsData != nullptr )


   FDataObj* pActorCountData = (FDataObj*)m_currentDataObjs[DT_ACTOR_COUNTS];
   if (pActorCountData != nullptr)
      {
      int groupCount = pActorManager->GetActorGroupCount();
      int actorCount = pActorManager->GetActorCount();

      int* counts = new int[groupCount];
      memset((void*)counts, 0, groupCount * sizeof(int));

      for (int i = 0; i < actorCount; i++)
         {
         Actor* pActor = pActorManager->GetActor(i);

         if (pActor->GetPolyCount() > 0)
            {
            int group = pActorManager->GetActorGroupIndexFromID(pActor->GetID());
            ASSERT(0 <= group && group < groupCount);
            counts[group]++;
            }
         }

      pActorCountData->Set(0, row, (float)currentYear);   // yearOfRun );
      int col = 1;
      for (int i = 0; i < groupCount; i++)
         {
         ActorGroup* pGroup = pActorManager->GetActorGroup(i);
         pActorCountData->Set(col++, row, counts[i]);
         }

      delete[] counts;
      }  // end of: if ( pActorWtsData != nullptr )


   // policy application summary (only collect and end of run)
   if (yearOfRun == m_pEnvModel->m_endYear - m_pEnvModel->m_startYear)
      {
      VDataObj* pPolicySummaryData = (VDataObj*)m_currentDataObjs[DT_POLICY_SUMMARY];
      ASSERT(pPolicySummaryData != nullptr);

      for (int j = 0; j < m_pEnvModel->m_pPolicyManager->GetPolicyCount(); j++)
         {
         EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(j);
         if (pPolicy->m_use)
            {
            float potentialAreaPct = 100 * pPolicy->m_potentialArea / this->m_pEnvModel->m_pIDULayer->GetTotalArea();
            float appliedAreaPct = 100 * pPolicy->m_cumAppliedArea / this->m_pEnvModel->m_pIDULayer->GetTotalArea();
            float potentialAreaUsedPct = 0;
            if (pPolicy->m_potentialArea > 0.00001f)
               potentialAreaUsedPct = 100 * pPolicy->m_cumAppliedArea / pPolicy->m_potentialArea;

            pPolicySummaryData->Set(1, j, (int)pPolicy->m_cumAppliedCount);      // Applied Count

            if (m_pEnvModel->m_collectPolicyData)
               pPolicySummaryData->Set(2, j, (int)potentialAreaPct);             // Potential Area (%)

            pPolicySummaryData->Set(3, j, (int)appliedAreaPct);               // Actual Area (%)
            pPolicySummaryData->Set(4, j, (int)pPolicy->m_cumAppliedArea);    // Actual Area 

            if (m_pEnvModel->m_collectPolicyData)
               pPolicySummaryData->Set(5, j, (int)potentialAreaUsedPct);         // % Potential Area Used

            pPolicySummaryData->Set(6, j, (int)pPolicy->m_cumExpansionArea);     // Expansion Area
            pPolicySummaryData->Set(7, j, (int)pPolicy->m_cumCost);              // costs associated with constraints
            }
         }
      }

   // global constraints
   FDataObj* pGlobalConstraintsData = (FDataObj*)m_currentDataObjs[DT_GLOBAL_CONSTRAINTS];
   if (pGlobalConstraintsData != nullptr)
      {
      pGlobalConstraintsData->Set(0, row, currentYear); // yearOfRun );
      int gcCount = m_pEnvModel->m_pPolicyManager->GetGlobalConstraintCount();

      int col = 1;
      for (int i = 0; i < gcCount; i++)
         {
         GlobalConstraint* pGC = m_pEnvModel->m_pPolicyManager->GetGlobalConstraint(i);
         pGlobalConstraintsData->Set(col++, row, pGC->m_currentValue);
         pGlobalConstraintsData->Set(col++, row, pGC->m_currentInitValue);
         pGlobalConstraintsData->Set(col++, row, pGC->m_currentMaintenanceValue);
         pGlobalConstraintsData->Set(col++, row, pGC->m_value);
         }
      }

   // policy stats
   FDataObj* pPolicyStatsData = (FDataObj*)m_currentDataObjs[DT_POLICY_STATS];
   if (pPolicyStatsData != nullptr)
      {
      pPolicyStatsData->Set(0, row, (float)currentYear); // yearOfRun );
      int policyCount = m_pEnvModel->m_pPolicyManager->GetPolicyCount();

      int col = 1;
      for (int i = 0; i < policyCount; i++)
         {
         EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(i);

         pPolicyStatsData->Set(col++, row, pPolicy->m_rejectedPolicyConstraint);
         pPolicyStatsData->Set(col++, row, pPolicy->m_rejectedNoCostInfo);
         pPolicyStatsData->Set(col++, row, pPolicy->m_rejectedSiteAttr);
         pPolicyStatsData->Set(col++, row, pPolicy->m_passedCount);
         pPolicyStatsData->Set(col++, row, pPolicy->m_appliedCount);
         pPolicyStatsData->Set(col++, row, pPolicy->m_noOutcomeCount);
         }
      }
  
   return true;
   }


void DataManager::EndRun(bool discardDeltaArray)
   {
   //int row = yearOfRun;

   // model output variables (autonomous processes followed by eval models)
   int outVarCount = this->m_pEnvModel->GetOutputVarCount(OVT_MODEL | OVT_EVALUATOR | OVT_PDATAOBJ);  // total for models and processes
   int outputAppVarCount = 0;
   int totalAppVarCount = m_pEnvModel->GetAppVarCount();
   for (int i = 0; i < totalAppVarCount; i++)
      {
      AppVar* pVar = m_pEnvModel->GetAppVar(i);
      if (pVar->m_avType == AVT_APP && pVar->IsGlobal())
         outputAppVarCount++;
      }

   int totalOutputCount = outVarCount + outputAppVarCount;

   if (totalOutputCount > 0)
      {
      VDataObj* m_pModelOutputsData = (VDataObj*)m_currentDataObjs[DT_MODEL_OUTPUTS];
      ASSERT(m_pModelOutputsData != nullptr);
      //m_pModelOutputsData->Set( 0, row, (float) yearOfRun );

      int col = 1;
      for (int i = 0; i < this->m_pEnvModel->GetModelProcessCount(); i++)
         {
         EnvModelProcess* pInfo = this->m_pEnvModel->GetModelProcessInfo(i);
         if (pInfo->m_use)
            {
            MODEL_VAR* modelVarArray = nullptr;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR& mv = modelVarArray[j];

               // need to make a copy of any data objects, since the originals are managed by
               // the plugin
               if (mv.type == TYPE_PDATAOBJ)
                  {
                  // get the original data obj ptr (stored in the m_pModelOutputsData)
                  DataObj* pData = nullptr;
                  m_pModelOutputsData->Get(col, 0, pData);

                  DataObj* pDataCopy = nullptr;

                  ASSERT(pData != nullptr);
                  switch (pData->GetDOType())
                     {
                     case DOT_FLOAT:
                     {
                     FDataObj* _pData = (FDataObj*)pData;
                     pDataCopy = new FDataObj(*_pData);
                     }
                     break;

                     case DOT_INT:
                     {
                     IDataObj* _pData = (IDataObj*)pData;
                     pDataCopy = new IDataObj(*_pData);
                     }
                     break;

                     case DOT_VDATA:
                     {
                     VDataObj* _pData = (VDataObj*)pData;
                     pDataCopy = new VDataObj(*_pData);
                     }
                     break;
                     }

                  if (pDataCopy != nullptr)
                     {
                     // update all the ptr records in the m_pModelOutputsData for this
                     m_modelOutputDataObjArray.Add(pDataCopy);

                     for (int j = 0; j < m_pModelOutputsData->GetRowCount(); j++)
                        {
                        m_pModelOutputsData->Set(col, j, VData(pDataCopy, TYPE_PDATAOBJ, false));   // store copied dataobj ptr in each row
                        }
                     }
                  }  // end of: if ( mv.type == PDATAOBJ )

               //VData v( mv.pVar, mv.type );
               //m_pModelOutputsData->Set( col, row, VData( mv.pVar, mv.type, false ) );  // convert to non-ptr type
               if (mv.pVar != nullptr)
                  col++;
               }  // end of: for ( j < varCount )
            }
         }

      for (int i = 0; i < this->m_pEnvModel->GetEvaluatorCount(); i++)
         {
         EnvEvaluator* pInfo = this->m_pEnvModel->GetEvaluatorInfo(i);
         if (pInfo->m_use)
            {
            MODEL_VAR* modelVarArray = nullptr;
            int varCount = pInfo->OutputVar(pInfo->m_id, &modelVarArray);

            for (int j = 0; j < varCount; j++)
               {
               MODEL_VAR& mv = modelVarArray[j];

               // need to make a copy of any data objects, since the originals are managed by
               // the plugin
               if (mv.type == TYPE_PDATAOBJ)
                  {
                  // get the original data obj ptr (stored in the m_pModelOutputsData)
                  DataObj* pData = nullptr;
                  m_pModelOutputsData->Get(col, 0, pData);

                  DataObj* pDataCopy = nullptr;

                  ASSERT(pData != nullptr);
                  switch (pData->GetDOType())
                     {
                     case DOT_FLOAT:
                     {
                     FDataObj* _pData = (FDataObj*)pData;
                     pDataCopy = new FDataObj(*_pData);   // DataManager manages this memory
                     }
                     break;

                     case DOT_INT:
                     {
                     IDataObj* _pData = (IDataObj*)pData;
                     pDataCopy = new IDataObj(*_pData);   // DataManager manages this memory
                     }
                     break;

                     case DOT_VDATA:
                     {
                     VDataObj* _pData = (VDataObj*)pData;
                     pDataCopy = new VDataObj(*_pData);  // DataManager manages this memory
                     }
                     break;
                     }

                  if (pDataCopy != nullptr)
                     {
                     m_modelOutputDataObjArray.Add(pDataCopy);

                     // update all the ptr records in the m_pModelOutputsData for this
                     for (int k = 0; k < m_pModelOutputsData->GetRowCount(); k++)
                        {
                        m_pModelOutputsData->Set(col, k, VData(pDataCopy, TYPE_PDATAOBJ, false));   // store copied dataobj ptr in each row
                        }
                     }
                  }  // end of: if ( mv.type == PDATAOBJ )

               if (mv.pVar != nullptr)
                  col++;
               }
            }
         }

      int totalAppVarCount = m_pEnvModel->GetAppVarCount(0);
      for (int i = 0; i < totalAppVarCount; i++)
         {
         AppVar* pVar = m_pEnvModel->GetAppVar(i);

         if (pVar->m_avType == AVT_APP && pVar->IsGlobal())
            {
            // stil to do
            //m_pModelOutputsData->Set( col, row, pVar->GetValue() );
            col++;
            }
         }
      }  // end of:  if ( outputCount > 0 )


   if (discardDeltaArray)
      {
      int count = GetDeltaArrayCount();
      ASSERT(count > 0);
      DiscardDeltaArray(count - 1);
      }
   }


bool DataManager::CreateMultiRunDataObjects()
   {
   int rows = m_pEnvModel->m_iterationsToRun;

   if (this->m_collectMultiRunOutput)
      {
      int resultsCount = this->m_pEnvModel->GetResultsCount();

      //--------------- landscape scores for each goal -------------
      FDataObj* pData = new FDataObj(resultsCount + 1, rows, U_UNDEFINED);
      pData->SetName("Multirun Landscape Evaluative Statistics");
      pData->SetLabel(0, "Run");
      for (int i = 0; i < resultsCount; i++)
         {
         int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
         pData->SetLabel(i + 1, this->m_pEnvModel->GetEvaluatorInfo(modelIndex)->m_name);
         }

      m_currentMultiRunDataObjs[DT_MULTI_EVAL_SCORES] = pData;
      m_multiRunDataObjs[DT_MULTI_EVAL_SCORES].Add(pData);

      //-------------- landscape raw scores for each goal -----------
      pData = new FDataObj(resultsCount + 1, rows, U_UNDEFINED);
      pData->SetName("Multirun Landscape Evaluative Statistics (Raw)");
      pData->SetLabel(0, "Run");
      for (int i = 0; i < resultsCount; i++)
         {
         int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
         pData->SetLabel(i + 1, this->m_pEnvModel->GetEvaluatorInfo(modelIndex)->m_name);
         }

      m_currentMultiRunDataObjs[DT_MULTI_EVAL_RAWSCORES] = pData;
      m_multiRunDataObjs[DT_MULTI_EVAL_RAWSCORES].Add(pData);


      //------------- variable settings for the scenario --------
      Scenario* pScenario = m_pEnvModel->GetScenario();
      ASSERT(pScenario != nullptr);

      int varCount = pScenario->GetScenarioVarCount(int(V_META | V_MODEL | V_AP), false);
      pData = new FDataObj(varCount + 1, rows, U_UNDEFINED);
      pData->SetName("Multirun Scenario Settings");
      pData->SetLabel(0, "Run");
      for (int i = 0; i < varCount; i++)
         pData->SetLabel(i + 1, pScenario->GetScenarioVar(i).name);

      m_currentMultiRunDataObjs[DT_MULTI_PARAMETERS] = pData;
      m_multiRunDataObjs[DT_MULTI_PARAMETERS].Add(pData);

      }
   int first = m_pEnvModel->m_currentRun;
   MULTIRUN_INFO mi(first, first + rows - 1, m_pEnvModel->GetScenario());
   m_multirunInfoArray.Add(mi);

   return true;
   }


bool DataManager::CollectMultiRunData(int run)   // note: run - zero-based
   {
   if (this->m_collectMultiRunOutput == false)
      return false;

   int row = run;
   int resultsCount = this->m_pEnvModel->GetResultsCount();

   // note: these are collected at the END of a run...

   //-------------- scores ------------------- 
   FDataObj* pData = (FDataObj*)m_currentMultiRunDataObjs[DT_MULTI_EVAL_SCORES];
   if (pData != nullptr)
      {
      pData->Set(0, row, (float)run);
      for (int i = 0; i < resultsCount; i++)
         {
         int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
         float score = m_pEnvModel->GetLandscapeMetric(modelIndex);
         pData->Set(i + 1, row, score);
         }
      }

   //----------- raw scores -------------------
   pData = (FDataObj*)m_currentMultiRunDataObjs[DT_MULTI_EVAL_RAWSCORES];
   if (pData != nullptr)
      {
      pData->Set(0, row, (float)run);
      for (int i = 0; i < resultsCount; i++)
         {
         int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
         float rawScore = m_pEnvModel->GetLandscapeMetricRaw(modelIndex);
         pData->Set(i + 1, row, rawScore);
         }
      }

   //------------- variable settings for the scenario --------
   pData = (FDataObj*)m_currentMultiRunDataObjs[DT_MULTI_PARAMETERS];
   if (pData != nullptr)
      {
      Scenario* pScenario = m_pEnvModel->GetScenario();
      ASSERT(pScenario != nullptr);
      int varCount = pScenario->GetScenarioVarCount(int(V_META | V_MODEL | V_AP), false);

      pData->Set(0, row, (float)run);
      for (int i = 0; i < varCount; i++)
         {
         SCENARIO_VAR& vi = pScenario->GetScenarioVar(i);
         float value;
         switch (vi.type)
            {
            case TYPE_BOOL:   value = *((bool*)vi.pVar) == false ? 0.0f : 1.0f;        break;
            case TYPE_UINT:   value = (float)*((UINT*)vi.pVar);                       break;
            case TYPE_INT:    value = (float)*((int*)vi.pVar);                        break;
            case TYPE_SHORT:  value = (float)*((short*)vi.pVar);                       break;
            case TYPE_LONG:   value = (float)*((long*)vi.pVar);                       break;
            case TYPE_FLOAT:  value = *((float*)vi.pVar);                              break;
            case TYPE_DOUBLE: value = (float)*((double*)vi.pVar);                     break;
            default: ASSERT(0);
            }

         pData->Set(i + 1, row, value);
         }
      }

   // thats all for now!

   return true;
   }

bool DataManager::ClearRunData(int run)
   {
   if (this->m_collectMultiRunOutput == false)
      {
      // NOTE: DataObjs are removed by the DataObjArray destructor
      for (int i = 0; i < m_deltaArrayArray.GetSize(); i++)
         if (i != run && m_deltaArrayArray[i] != nullptr)  // keep the current one around to unroll in next InitRun()
            {
            delete m_deltaArrayArray[i];
            m_deltaArrayArray[i] = nullptr;
            }

      //m_pDeltaArray = nullptr;

      
      //for (int i = 0; i < DT_LAST; i++)
      //   {
      //   m_currentDataObjs[i] = nullptr;
      //   //m_dataObjs[i].RemoveAllData();
      //   if (m_dataObjs[i][run] != nullptr)
      //      {
      //      delete m_dataObjs[i][run];
      //      m_dataObjs[i][run] = nullptr;
      //      }
      //   }
      //
      //for (int i = 0; i < DT_MULTI_LAST; i++)
      //   {
      //   m_currentMultiRunDataObjs[i] = nullptr;
      //   m_multiRunDataObjs[i].RemoveAllData();
      //   }
      //
      //if (m_modelDataObjMatrices != nullptr)
      //   {
      //   for (int i = 0; i < this->m_pEnvModel->GetResultsCount(); i++)
      //      if (m_modelDataObjMatrices[i] != nullptr)
      //         {
      //         delete m_modelDataObjMatrices[i];
      //         m_modelDataObjMatrices[i] = nullptr;
      //         }
      //
      //   //delete[] m_modelDataObjMatrices;
      //   //m_modelDataObjMatrices = nullptr;
      //   }
      }
   return true;
   }



// -- FDataObj *DataManager::CalculateLulcTrends(.) ----------------------------------------------------
//
// allocate data object.  The data object will have a column for year, and one column for each LULC class
// if there are n years of data there will be n+1 rows
//    row i, 0 <= i <= n-1, will have the data corresponding to the map state at the begining of year i.
//    row n will have the data corresponding to the map state at the end of year n-1.
//
//  This DataObj cotains state info, so length = number of simulated years + 1
// ------------------------------------------------------------------------------------------------------

FDataObj* DataManager::CalculateLulcTrends(int level, int run /*=-1*/)
   {
   DeltaArray* pDeltaArray = this->GetDeltaArray(run);
   ASSERT(pDeltaArray != nullptr);

   RUN_INFO& ri = this->GetRunInfo(run);

   int startYear = ri.startYear;
   int endYear = ri.endYear;
   int years = endYear - startYear;

   CString label = this->m_pEnvModel->m_lulcTree.GetFieldName(level);
   label += " Trends (pct)";

   // allocate a data object
   int lulcCount = this->m_pEnvModel->m_lulcTree.GetNodeCount(level);

   FDataObj* pData = new FDataObj(lulcCount + 1, years + 1, U_YEARS); // +1 for values at the end of the last year
   pData->SetName(label);
   pData->SetLabel(0, "Time (years)");

   // get the LULC nodes
   for (int i = 0; i < lulcCount; i++)
      {
      LulcNode* pNode = this->m_pEnvModel->m_lulcTree.FindNodeFromIndex(level, i);
      CString label;
      label.Format("%s (%i)", (PCTSTR)pNode->m_name, pNode->m_id);
      pData->SetLabel(i + 1, label);
      }

   // allocate an array to track
   float* trendArray = new float[lulcCount];  // will be performing lots of additions. Is truncations error is an issue?
   for (int i = 0; i < lulcCount; i++)
      trendArray[i] = 0.0f;

   // create a map to map lulcA values into offsets into the pTrendArray
   CMap< int, int, int, int > lulcMap;
   for (int i = 0; i < lulcCount; i++)
      {
      LulcNode* pNode = this->m_pEnvModel->m_lulcTree.FindNodeFromIndex(level, i);
      lulcMap.SetAt(pNode->m_id, i);
      }

   // Calculate initial %Lulc.  The ColumnTrace contains only the rows data for the specified column
   int colLulc;
   switch (level)
      {
      case 1:  colLulc = this->m_pEnvModel->m_colLulcA;  break;
      case 2:  colLulc = this->m_pEnvModel->m_colLulcB;  break;
      case 3:  colLulc = this->m_pEnvModel->m_colLulcC;  break;
      case 4:  colLulc = this->m_pEnvModel->m_colLulcD;  break;
      }

   ColumnTrace lulcCol(m_pEnvModel, colLulc, run);  // Note: the column trace is rolled back to inital conditions inthe constructor
   float totalArea = 0.0f;
   int   lulcIndex = -1;

   lulcCol.SetCurrentYear(0);

   // roll back map to starting condition
   // nullptr = unapply internal (current) deltaArray
   // -1 = unapply the whole thing
   UINT_PTR firstUnapplied = m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, nullptr);

   // interate through cells to get starting value
   for (MapLayer::Iterator i = this->m_pEnvModel->m_pIDULayer->Begin(); i != this->m_pEnvModel->m_pIDULayer->End(); i++)
      {
      int lulc;
      lulcCol.Get(i, lulc);

      if (lulc > 0)
         {
         bool ok = lulcMap.Lookup(lulc, lulcIndex);    // get the index for this lulc class in the trendArray
         if (ok) //ASSERT( ok );   
            {
            float area;
            this->m_pEnvModel->m_pIDULayer->GetData(i, this->m_pEnvModel->m_colArea, area);
            trendArray[lulcIndex] += area;
            totalArea += area;
            }
         }
      }

   // record data for year 0
   pData->Set(0, 0, startYear);
   for (int i = 0; i < lulcCount; i++)
      pData->Set(i + 1, 0, 100.0f * trendArray[i] / totalArea);

   // data object set up, now start interating through the delta array, processing changes
   int year = startYear;
   int index = 0;
   int row = 1;

   for (int year = startYear; year < endYear; year++)
      {
      INT_PTR from = -1;
      INT_PTR to = -1;
      pDeltaArray->GetIndexRangeFromYear(year, from, to);

      // mine data from delta array
      for (INT_PTR i = from; i < to; i++)
         {
         DELTA delta = pDeltaArray->GetAt(i);

         m_pEnvModel->ApplyDelta(this->m_pEnvModel->m_pIDULayer, delta);

         // is it an lulcA change?
         if (delta.col == colLulc)
            {
            // get the area
            float area;
            int value;
            this->m_pEnvModel->m_pIDULayer->GetData(delta.cell, this->m_pEnvModel->m_colArea, area);

            // get starting lulcA value and subtract from appropriate slot in trendArray
            delta.oldValue.GetAsInt(value);
            bool ok = lulcMap.Lookup(value, lulcIndex);
            //ASSERT( ok );
            trendArray[lulcIndex] -= area;

            // get ending lulcA value and add to the appropriate slot in the trendArray
            delta.newValue.GetAsInt(value);
            ok = lulcMap.Lookup(value, lulcIndex);    // get the index for this lulc class in the trendArray
            //ASSERT( ok );   
            trendArray[lulcIndex] += area;
            }
         }

      // record
      pData->Set(0, row, year + 1); // we are now at the end of year "year" which is the same as the beginning of year "year+1"
      for (int j = 0; j < lulcCount; j++)
         {
         pData->Set(j + 1, row, 100.0f * trendArray[j] / totalArea);
         }

      row++; // done with this year, move to next
      }

   delete[] trendArray;

   // done mining data.  Next step is to return the current map to the state it was in when we entered this function.

   pDeltaArray->SetFirstUnapplied(this->m_pEnvModel->m_pIDULayer, SFU_END);      // deltas have no concept of the delta array they belong to, so ApplyDelta does not set FirstUnapplied
   m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, pDeltaArray);   // unapply all of this delta array
   m_pEnvModel->ApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, nullptr, 0, firstUnapplied == 0 ? 0 : firstUnapplied - 1);      // return map to it's starting state

   return pData;
   }

// compute a frequency distribution for evalative scores
FDataObj* DataManager::CalculateEvalFreq(int multiRun /*= -1*/)
   {
   if (this->m_collectMultiRunOutput == false)
      return nullptr;

   int resultsCount = this->m_pEnvModel->GetResultsCount();
   const int binCount = 12;

   // create a dataobj for the histogram
   FDataObj* pData = new FDataObj(resultsCount + 1, binCount, 0.0f, U_UNDEFINED);
   pData->SetLabel(0, "Bin");
   for (int i = 0; i < resultsCount; i++)
      {
      int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
      pData->SetLabel(i + 1, this->m_pEnvModel->GetEvaluatorInfo(modelIndex)->m_name);
      }

   const float vMin = -3;
   const float vMax = +3;
   for (int i = 0; i < binCount; i++)
      {
      //pData->Set( 0, i, i+1 );      // bins are 1-based

      float mid = float(vMin + ((i + 0.5) * (vMax - vMin) / binCount));
      pData->Set(0, i, mid);
      }

   FDataObj* pSource = (FDataObj*)GetMultiRunDataObj(DT_MULTI_EVAL_SCORES, multiRun);
   ASSERT(pSource != nullptr);
   ASSERT(pData != nullptr);

   int rows = pSource->GetRowCount();
   float value;

   for (int i = 0; i < resultsCount; i++)
      {
      for (int j = 0; j < rows; j++)
         {
         bool ok = pSource->Get(i + 1, j, value);
         ASSERT(ok);

         // figure out which bin it belongs in
         int bin = int((value - vMin) * binCount / (vMax - vMin));
         if (bin < 0) bin = 0;
         if (bin > binCount - 1) bin = binCount - 1;

         // update the frequency;
         pData->Add(i + 1, bin, 1);
         }
      }

   return pData;
   }

// -- FDataObj *DataManager::CalculatePolicyApps(.) -----------------------------------------------------
//
// allocate data object.  The data object will have a column for year, and one column for each policy 
//   in the policyManagers policy set.
// There will be one row for each year.
//   row i, will have the number of policy applications that occured during year i
//
//  This DataObj has no time=0 values, so length = number of simulated years
// ------------------------------------------------------------------------------------------------------

IDataObj* DataManager::CalculatePolicyApps(int run /*= -1*/)
   {
   DeltaArray* pDeltaArray = this->GetDeltaArray(run);

   if (pDeltaArray == nullptr)
      return nullptr;

   RUN_INFO& ri = this->GetRunInfo(run);

   int startYear = ri.startYear;
   int endYear = ri.endYear;
   int years = endYear - startYear;

   //int startYear = pDeltaArray->GetAt( 0 ).year;
   //int endYear   = pDeltaArray->GetAt( pDeltaArray->GetSize()-1 ).year + 1;
   //int years     = endYear - startYear;

   int policyCount = m_pEnvModel->m_pPolicyManager->GetPolicyCount();

   // allocate a data object
   IDataObj* pData = new IDataObj(policyCount + 1, years, U_YEARS);
   pData->SetName("Actor Policy Application Rates");
   pData->SetLabel(0, "Time (years)");
   for (int i = 0; i < policyCount; i++)
      {
      EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(i);
      pData->SetLabel(i + 1, pPolicy->m_name);
      }

   int* countArray = new int[policyCount]; // gets set to zeros in the for loop

   UINT_PTR firstUnapplied = m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer);    // move map to starting conditions

   for (int year = startYear; year < endYear; year++)
      {
      INT_PTR from = -1;
      INT_PTR to = -1;
      pDeltaArray->GetIndexRangeFromYear(year, from, to);

      for (int i = 0; i < policyCount; i++)
         countArray[i] = 0;

      // mine data from delta array
      for (INT_PTR i = from; i < to; i++)
         {
         DELTA& delta = pDeltaArray->GetAt(i);

         // apply the delta to the map
         m_pEnvModel->ApplyDelta(this->m_pEnvModel->m_pIDULayer, delta);

         if (delta.col == this->m_pEnvModel->m_colPolicy)
            {
            int policyID;
            delta.newValue.GetAsInt(policyID);

            int policyIndex = m_pEnvModel->m_pPolicyManager->GetPolicyIndexFromID(policyID);
            ASSERT(0 <= policyIndex && policyIndex < policyCount);
            countArray[policyIndex] += 1;
            }
         }

      // record data
      pData->Set(0, year - startYear, year + 1);
      for (int j = 0; j < policyCount; j++)
         pData->Set(j + 1, year - startYear, (float)countArray[j]);
      }

   delete[] countArray;

   // map has this runs deltas applied, remove them and restore the original
   pDeltaArray->SetFirstUnapplied(this->m_pEnvModel->m_pIDULayer, SFU_END);      // deltas have no concept of the delta array they belong to, so ApplyDelta does not set FirstUnapplied
   m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, pDeltaArray);
   m_pEnvModel->ApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, nullptr, 0, firstUnapplied == 0 ? 0 : firstUnapplied - 1);      // return map to it's starting state

   return pData;
   }

// -- FDataObj *DataManager::CalculatePolicyEffectivenessTrendDynamic(.) -------------------------------------------------
//
// allocate data object.  
// The data object will have a column for the mean policy effectiveness for the ecological goal, 
//   and one column for the mean effectiveness for the ecological goal.  
// In each case, the score is weighted by the application rate of the policy
// There will be one row for each year.
//
// -----------------------------------------------------------------------------------------------------------------------
FDataObj* DataManager::CalculatePolicyEffectivenessTrendDynamic(int modelIndexX, int modelIndexY, int run /*= -1*/)
   {
   DeltaArray* pDeltaArray = this->GetDeltaArray(run);
   if (pDeltaArray == nullptr)
      return nullptr;

   int metagoalIndexX = this->m_pEnvModel->GetMetagoalIndexFromEvaluatorIndex(modelIndexX);
   int metagoalIndexY = this->m_pEnvModel->GetMetagoalIndexFromEvaluatorIndex(modelIndexY);

   int startYear = pDeltaArray->GetAt(0).year;
   int endYear = pDeltaArray->GetAt(pDeltaArray->GetSize() - 1).year + 1;
   int years = endYear - startYear;

   int policyCount = m_pEnvModel->m_pPolicyManager->GetPolicyCount();

   // allocate a data object (2 columns, with a row for each year)
   FDataObj* pData = new FDataObj(2, years, U_YEARS);
   pData->SetName("Policy Effectiveness Trend");
   pData->SetLabel(0, this->m_pEnvModel->GetEvaluatorInfo(modelIndexX)->m_name);
   pData->SetLabel(1, this->m_pEnvModel->GetEvaluatorInfo(modelIndexY)->m_name);

   UINT_PTR firstUnapplied = m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer); // roll map back to starting condition

   for (int year = startYear; year < endYear; year++)
      {
      // for each year, get the effectiveness of the applied policies for the two specified models, 
      // normalized by the number of applications of that policy (or maybe this should be by area of policy application?)

      INT_PTR from = -1;
      INT_PTR to = -1;
      pDeltaArray->GetIndexRangeFromYear(year, from, to);

      float modelXResults = 0;
      float modelYResults = 0;

      int totalPolicyApps = 0;

      // mine data from delta array
      for (INT_PTR i = from; i < to; i++)
         {
         DELTA& delta = pDeltaArray->GetAt(i);

         m_pEnvModel->ApplyDelta(this->m_pEnvModel->m_pIDULayer, delta);

         if (delta.col == this->m_pEnvModel->m_colPolicy)
            {
            int policyID;
            delta.newValue.GetAsInt(policyID);
            int policyIndex = m_pEnvModel->m_pPolicyManager->GetPolicyIndexFromID(policyID);
            ASSERT(0 <= policyIndex && policyIndex < policyCount);

            EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(policyIndex);
            ASSERT(pPolicy != nullptr);

            float effectivenessX = pPolicy->GetGoalScore(metagoalIndexX);
            float effectivenessY = pPolicy->GetGoalScore(metagoalIndexY);

            modelXResults += effectivenessX;
            modelYResults += effectivenessY;
            totalPolicyApps++;
            }
         }

      if (totalPolicyApps == 0)
         modelXResults = modelYResults = 0;
      else
         {
         modelXResults /= totalPolicyApps;
         modelYResults /= totalPolicyApps;
         }

      // record data
      pData->Set(0, year - startYear, modelXResults);
      pData->Set(1, year - startYear, modelYResults);

      }  // end of: for ( year <= endyear )

   // map has this runs deltas applied, remove them and restore the original
   pDeltaArray->SetFirstUnapplied(this->m_pEnvModel->m_pIDULayer, SFU_END);      // deltas have no concept of the delta array they belong to, so ApplyDelta does not set FirstUnapplied
   m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, pDeltaArray);
   m_pEnvModel->ApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, nullptr, 0, firstUnapplied == 0 ? 0 : firstUnapplied - 1);      // return map to it's starting state

   return pData;
   }

// -- FDataObj *DataManager::CalculatePolicyEffectivenessTrendStatic(.) ------------------------------------------
//
// allocate data object.  
// The data object will have a column for mean policy effectiveness for the ecological goal, 
//    and one column for the mean effectiveness for the ecological goal.  
// In each case, the score is weighted by the application rate of the policy
// There will be one row for each year.
//
// Note: rowCount = number of simulated years (no initial conditions collected)
// ---------------------------------------------------------------------------------------------------------------
FDataObj* DataManager::CalculatePolicyEffectivenessTrendStatic(int run /*= -1*/)
   {
   DeltaArray* pDeltaArray = this->GetDeltaArray(run);
   if (pDeltaArray == nullptr)
      return nullptr;

   RUN_INFO& ri = this->GetRunInfo(run);

   int startYear = ri.startYear;
   int endYear = ri.endYear;
   int years = endYear - startYear;

   //int startYear = pDeltaArray->GetAt( 0 ).year;
   //int endYear   = pDeltaArray->GetAt( pDeltaArray->GetSize()-1 ).year + 1;
   //int years     = endYear - startYear;

   int policyCount = m_pEnvModel->m_pPolicyManager->GetPolicyCount();

   // allocate a data object (one column for year, + one column for each policy effectiveness (model) )
   int goalCount = this->m_pEnvModel->GetMetagoalCount();
   //int resultsCount = this->m_pEnvModel->GetResultsCount();

   // move map back to starting conditions
   UINT_PTR firstUnapplied = m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer);

   FDataObj* pData = new FDataObj(goalCount + 1, years, U_YEARS);
   pData->SetName("Policy Effectiveness Trend");
   pData->SetLabel(0, "Time (Years)");
   for (int i = 0; i < goalCount; i++)
      {
      //int modelIndex = this->m_pEnvModel->GetModelIndexFromMetagoalIndex( i );
      //pData->SetLabel( i+1, this->m_pEnvModel->GetEvaluatorInfo( modelIndex )->m_name );
      METAGOAL* pInfo = this->m_pEnvModel->GetMetagoalInfo(i);
      pData->SetLabel(i + 1, pInfo->name);
      }

   float* modelResults = new float[goalCount];

   for (int year = startYear; year < endYear; year++)
      {
      // for each year, get the effectiveness of the applied policies for each model, 
      // normalized by the number of applications of that policy (or maybe this should be by area of policy application?)
      INT_PTR from = -1;
      INT_PTR to = -1;
      pDeltaArray->GetIndexRangeFromYear(year, from, to);

      for (int i = 0; i < goalCount; i++)
         modelResults[i] = 0.0f;

      int totalPolicyApps = 0;

      // mine data from delta array
      for (INT_PTR j = from; j < to; j++)
         {
         DELTA& delta = pDeltaArray->GetAt(j);

         m_pEnvModel->ApplyDelta(this->m_pEnvModel->m_pIDULayer, delta);

         if (delta.col == this->m_pEnvModel->m_colPolicy)
            {
            int policyID;
            delta.newValue.GetAsInt(policyID);
            int policyIndex = m_pEnvModel->m_pPolicyManager->GetPolicyIndexFromID(policyID);
            ASSERT(0 <= policyIndex && policyIndex < policyCount);

            EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(policyIndex);
            ASSERT(pPolicy != nullptr);

            for (int i = 0; i < goalCount; i++)
               {
               //int modelIndex = this->m_pEnvModel->GetModelIndexFromMetagoalIndex( i );
               float effectiveness = pPolicy->GetGoalScore(i); //modelIndex );
               modelResults[i] += effectiveness;
               }

            totalPolicyApps++;
            }
         }

      // record data
      pData->Set(0, year - startYear, year);

      for (int i = 0; i < goalCount; i++)
         {
         if (totalPolicyApps == 0)
            modelResults[i] = 0;
         else
            modelResults[i] /= totalPolicyApps;

         pData->Set(i + 1, year - startYear, modelResults[i]);
         }
      }  // end of: for ( year <= endyear )

   delete[] modelResults;

   // map has this runs deltas applied, remove them and restore the original
   pDeltaArray->SetFirstUnapplied(this->m_pEnvModel->m_pIDULayer, SFU_END);      // deltas have no concept of the delta array they belong to, so ApplyDelta does not set FirstUnapplied
   m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, pDeltaArray);
   m_pEnvModel->ApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, nullptr, 0, firstUnapplied == 0 ? 0 : firstUnapplied - 1);      // return map to it's starting state

   return pData;
   }

// -- VDataObj *DataManager::CalculatePolicySummary() ------------------------------------------
//
// allocate data object.  
// The data object will have a column for
//  0) name of policy
//  1) times policy was applied
//  2) potential area of application
//  3) actual area of application
//  4) for each eval model, the change in effectness/policy application area over the run
//  5) for each actor value, the average value for the actors that selected this policy
//
// Note: rowCount = number of simulated years (no initial conditions collected)
// ---------------------------------------------------------------------------------------------

VDataObj* DataManager::CalculatePolicySummary(int run /*= -1*/)
   {
   // compute column count
   int resultsCount = 0;  //this->m_pEnvModel->GetResultsCount();
   int valueCount = this->m_pEnvModel->GetActorValueCount();
   int colCount = 6 + resultsCount + valueCount + 1;

   DeltaArray* pDeltaArray = this->GetDeltaArray(run);
   if (pDeltaArray == nullptr)
      return nullptr;

   RUN_INFO& ri = this->GetRunInfo(run);

   int startYear = ri.startYear;
   int endYear = ri.endYear;
   int years = endYear - startYear;

   //int startYear = pDeltaArray->GetAt( 0 ).year;
   //int endYear   = pDeltaArray->GetAt( pDeltaArray->GetSize()-1 ).year + 1;
   //int years     = endYear - startYear;

   //int policyCount = m_pEnvModel->m_pPolicyManager->GetPolicyCount();
   if (run < 0)
      run = this->GetRunCount() - 1;

   Scenario* pScenario = ri.pScenario;
   int policyCount = pScenario->GetPolicyCount(false);  // only "In Use" policies
   int usedPolicyCount = pScenario->GetPolicyCount(true);

   // allocate a data object
   VDataObj* pData = new VDataObj(colCount, usedPolicyCount, U_UNDEFINED);
   pData->SetName("Policy Results Summary");
   pData->SetLabel(0, "Policy");
   pData->SetLabel(1, "Applied Count");
   pData->SetLabel(2, "Potential Area (%)");
   pData->SetLabel(3, "Actual Area (%)");
   pData->SetLabel(4, "Actual Area");
   pData->SetLabel(5, "% Potential Area Used");
   pData->SetLabel(6, "Expansion Area");

   // initialize columns
   usedPolicyCount = 0;
   CMap<int, int, int, int> usedPolicyIndexMap;

   for (int j = 0; j < policyCount; j++)
      {
      POLICY_INFO& pi = pScenario->GetPolicyInfo(j);

      if (pi.inUse)
         {
         pData->Set(0, usedPolicyCount, VData(m_pEnvModel->m_pPolicyManager->GetPolicy(j)->m_name));
         pData->Set(1, usedPolicyCount, 0.0f);
         pData->Set(2, usedPolicyCount, 0.0f);
         pData->Set(3, usedPolicyCount, 0.0f);
         pData->Set(4, usedPolicyCount, 0);
         pData->Set(5, usedPolicyCount, 0.0f);
         pData->Set(6, usedPolicyCount, (int)pi.pPolicy->m_cumAppliedArea);

         usedPolicyIndexMap.SetAt(j, usedPolicyCount);
         usedPolicyCount++;
         }
      }

   // add labels for evaluative metrics
   for (int i = 0; i < resultsCount; i++)
      {
      int modelIndex = this->m_pEnvModel->GetEvaluatorIndexFromResultsIndex(i);
      CString label = this->m_pEnvModel->GetEvaluatorInfo(modelIndex)->m_name;
      label += " (change/policy area)";
      pData->SetLabel(i + 7, label);

      int usedPolicyIndex = 0;
      for (int j = 0; j < policyCount; j++)
         {
         POLICY_INFO& pi = pScenario->GetPolicyInfo(j);

         if (pi.inUse)
            {
            pData->Set(i + 6, usedPolicyIndex, 0);
            usedPolicyIndex++;
            }
         }
      }

   // add labels for actorvalues
   for (int i = 0; i < valueCount; i++)
      {
      METAGOAL* pGoal = this->m_pEnvModel->GetActorValueMetagoalInfo(i);
      CString label = "Avg Actor Value (" + pGoal->name + ")";
      pData->SetLabel(i + 7 + resultsCount, label);

      int usedPolicyIndex = 0;
      for (int j = 0; j < policyCount; j++)
         {
         POLICY_INFO& pi = pScenario->GetPolicyInfo(j);

         if (pi.inUse)
            {
            pData->Set(i + 7 + resultsCount, usedPolicyIndex, 0.0f);
            usedPolicyIndex++;
            }
         }
      }

   int colPolicyID = this->m_pEnvModel->m_colPolicy;
   int colActor = this->m_pEnvModel->m_colActor;

   // basic strategy:
   //  first, iterate through the delta array, looking for policy applications.  for each application
   //    determine which policy and which cell was applied.  Compute the applied area IF the policy has not
   //    been applied before.  Store the results in the policyCellArray, for each cell, for each policy
   CUIntArray* policyCellArray = new CUIntArray[usedPolicyCount];  // an array of arrays of the cells each policy is applied in.

   // set map to starting state
   UINT_PTR firstUnapplied = m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer);

   // start iterating through the deltas.
   UINT_PTR deltaCount = pDeltaArray->GetSize();
   for (UINT_PTR i = 0; i < deltaCount; i++)
      {
      DELTA& delta = pDeltaArray->GetAt(i);
      m_pEnvModel->ApplyDelta(this->m_pEnvModel->m_pIDULayer, delta); // apply it

      // if this delta was from a policy process, extract area info from it
      if (delta.col == colPolicyID)  // was it for the specification of a specific policy?
         {
         int policyID;
         delta.newValue.GetAsInt(policyID);  // get index of policy applied
         int policyIndex = m_pEnvModel->m_pPolicyManager->GetPolicyIndexFromID(policyID);
         int cell = delta.cell;                   // and the cell it was applied to

         // get the corresponding Used index 
         int usedPolicyIndex;
         bool ok = usedPolicyIndexMap.Lookup(policyIndex, usedPolicyIndex);
         ASSERT(ok);

         // have we seen this policy in this cell before?
         CUIntArray& cellArray = policyCellArray[usedPolicyIndex];
         bool found = false;
         for (int j = 0; j < cellArray.GetSize(); j++)
            {
            if (cellArray[j] == cell)
               {
               found = true;
               break;
               }
            }

         if (!found) // cell hasn't been seen for this policy, so add it
            cellArray.Add(cell);

         // increment the policy's "Applied Count" field
         int count = pData->GetAsInt(1, usedPolicyIndex);
         pData->Set(1, usedPolicyIndex, count + 1);
         }
      }  // end of:  for ( i < deltaCount )

   // so at this point, we know which policies were applied in which cells.  compute corresponding areas.

   // first, get total area of ALL cells
   float cellArea = 0.0f;
   float totalArea = this->m_pEnvModel->m_pIDULayer->GetTotalArea();
   char text[128];

   // iterate through policies, getting area each policy was applied to
   int usedPolicyIndex = 0;
   for (int i = 0; i < policyCount; i++)
      {
      if (pScenario->GetPolicyInfo(i).inUse == false)
         continue;

      EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(i);

      // set total potential area for this policy (Note: potentialAreaDataObj = 3cols X policyCount rows)
      VDataObj* pPotentialAreaData = (VDataObj*)this->GetDataObj(DT_POLICY_SUMMARY, run);
      float potentialArea = pPotentialAreaData->GetAsFloat(2, i);

      if (potentialArea / totalArea > 0.0001f)
         sprintf_s(text, 128, "%6.2f%%", 100 * potentialArea / totalArea);
      else
         sprintf_s(text, 128, "%f%%", 100 * potentialArea / totalArea);

      pData->Set(2, usedPolicyIndex, text);

      // set applied cell area for this policy
      CUIntArray& cellArray = policyCellArray[usedPolicyIndex];
      float appliedArea = 0.0f;
      for (int j = 0; j < cellArray.GetSize(); j++)
         {
         this->m_pEnvModel->m_pIDULayer->GetData(cellArray[j], this->m_pEnvModel->m_colArea, cellArea);
         appliedArea += cellArea;
         }

      if (appliedArea / totalArea > 0.0001f)
         sprintf_s(text, 128, "%6.2f%%", 100 * appliedArea / totalArea);
      else
         sprintf_s(text, 128, "%f%%", 100 * appliedArea / totalArea);

      pData->Set(3, usedPolicyIndex, text);

      sprintf_s(text, 128, "%12.0f", appliedArea);
      pData->Set(4, usedPolicyIndex, text);

      if (potentialArea < 0.01f)
         pData->Set(5, usedPolicyIndex, "-n/a-");
      else
         {
         if (appliedArea / potentialArea > 0.001f)
            sprintf_s(text, 128, "%6.2f%%", 100 * appliedArea / potentialArea);
         else
            sprintf_s(text, 128, "%f%%", 100 * appliedArea / potentialArea);

         pData->Set(5, usedPolicyIndex, text);
         }

      // look at values of actors  applying this policy next.  To do this, for each value
      // iterate through the actors selecting this policy and summarize their values
      for (int k = 0; k < valueCount; k++)
         {
         float cumValue = 0;

         // figure out the actors and they values that resulted in this policy being applied
         int cellCount = 0;
         for (int j = 0; j < cellArray.GetSize(); j++)
            {
            int actor;
            this->m_pEnvModel->m_pIDULayer->GetData(cellArray[j], colActor, actor);

            if (actor >= 0)
               {
               Actor* pActor = m_pEnvModel->m_pActorManager->GetActor(actor);
               float value = pActor->GetValue(k);
               cumValue += value;
               cellCount++;
               }
            }

         if (cellArray.IsEmpty())
            pData->Set(7 + resultsCount + k, usedPolicyIndex, "-n/a-");
         else
            {
            sprintf_s(text, 128, "%6.2f%%", float(cumValue / cellCount));
            pData->Set(7 + resultsCount + k, usedPolicyIndex, text);
            }
         }

      usedPolicyIndex++;
      }  // end of: for ( i < policyCount )

   delete[] policyCellArray;

   // map has this runs deltas applied, remove them and restore the original
   pDeltaArray->SetFirstUnapplied(this->m_pEnvModel->m_pIDULayer, SFU_END);      // deltas have no concept of the delta array they belong to, so ApplyDelta does not set FirstUnapplied
   m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, pDeltaArray);
   m_pEnvModel->ApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, nullptr, 0, firstUnapplied == 0 ? 0 : firstUnapplied - 1);      // return map to it's starting state

   return pData;
   }


VDataObj* DataManager::CalculatePolicyByActorArea(int run /* = -1 */)
   {
   DeltaArray* pDeltaArray = this->GetDeltaArray(run);
   if (pDeltaArray == nullptr)
      return nullptr;

   if (run < 0)
      run = this->GetRunCount() - 1;

   RUN_INFO& ri = GetRunInfo(run);
   Scenario* pScenario = ri.pScenario;
   int policies = m_pEnvModel->m_pPolicyManager->GetPolicyCount();
   int actors = m_pEnvModel->m_pActorManager->GetActorGroupCount();

   // allocate a data object
   VDataObj* pData = new VDataObj(actors + 1, policies, U_UNDEFINED);
   pData->SetName("Policies By Actor Summary (Area)");

   pData->SetLabel(0, "Policy");
   for (int j = 0; j < actors; j++)
      {
      ActorGroup* pGroup = m_pEnvModel->m_pActorManager->GetActorGroup(j); //Policy *pPolicy =  m_pEnvModel->m_pPolicyManager->GetPolicy( j );
      pData->SetLabel(j + 1, pGroup->m_name);
      }

   for (int j = 0; j < policies; j++)
      {
      EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(j);
      pData->Set(0, j, pPolicy->m_name);
      }

   for (int j = 0; j < actors; j++)
      for (int k = 0; k < policies; k++)
         pData->Set(j + 1, k, 0);

   // NOTE ASSUMES STATIC ACTORS!!!!!
   int policyApps = 0;
   int duplicatePolicyCount = 0;

   for (INT_PTR i = 0; i < pDeltaArray->GetSize(); i++)
      {
      DELTA& d = pDeltaArray->GetAt(i);

      if (d.col == this->m_pEnvModel->m_colPolicy)  // this is the policy ID
         {
         policyApps++;

         float area;
         this->m_pEnvModel->m_pIDULayer->GetData(d.cell, this->m_pEnvModel->m_colArea, area);

         int actor;
         this->m_pEnvModel->m_pIDULayer->GetData(d.cell, this->m_pEnvModel->m_colActor, actor);

         int policyID;
         d.newValue.GetAsInt(policyID);
         int policyIndex = m_pEnvModel->m_pPolicyManager->GetPolicyIndexFromID(policyID);

         if (d.newValue == d.oldValue)
            duplicatePolicyCount++;

         int currentArea = pData->GetAsInt(actor + 1, policyIndex);
         currentArea += (int)area;
         pData->Set(actor + 1, policyIndex, currentArea);
         }
      }

   CString msg;
   msg.Format("Generated Policy By Actor Report - %i total policy applications, %i duplicates", policyApps, duplicatePolicyCount);
   Report::Log(msg);

   return pData;
   }

VDataObj* DataManager::CalculatePolicyByActorCount(int run /* = -1 */)
   {
   DeltaArray* pDeltaArray = this->GetDeltaArray(run);
   if (pDeltaArray == nullptr)
      return nullptr;

   if (run < 0)
      run = this->GetRunCount() - 1;

   RUN_INFO& ri = GetRunInfo(run);
   Scenario* pScenario = ri.pScenario;
   int policies = m_pEnvModel->m_pPolicyManager->GetPolicyCount();
   int actors = m_pEnvModel->m_pActorManager->GetActorGroupCount();

   // allocate a data object
   VDataObj* pData = new VDataObj(actors + 1, policies, U_UNDEFINED);
   pData->SetName("Policies By Actor Summary (Count)");

   pData->SetLabel(0, "Policy");
   for (int j = 0; j < actors; j++)
      {
      ActorGroup* pGroup = m_pEnvModel->m_pActorManager->GetActorGroup(j); //Policy *pPolicy =  m_pEnvModel->m_pPolicyManager->GetPolicy( j );
      pData->SetLabel(j + 1, pGroup->m_name);
      }

   for (int j = 0; j < policies; j++)
      {
      EnvPolicy* pPolicy = m_pEnvModel->m_pPolicyManager->GetPolicy(j);
      pData->Set(0, j, pPolicy->m_name);
      }

   for (int j = 0; j < actors; j++)
      for (int k = 0; k < policies; k++)
         pData->Set(j + 1, k, 0);

   // NOTE ASSUMES STATIC ACTORS!!!!!
   int policyApps = 0;
   int duplicatePolicyCount = 0;

   for (INT_PTR i = 0; i < pDeltaArray->GetSize(); i++)
      {
      DELTA& d = pDeltaArray->GetAt(i);

      if (d.col == this->m_pEnvModel->m_colPolicy)  // this is the policy ID
         {
         policyApps++;

         int actor;
         this->m_pEnvModel->m_pIDULayer->GetData(d.cell, this->m_pEnvModel->m_colActor, actor);

         int policyID;
         d.newValue.GetAsInt(policyID);   // ID

         int policyIndex = m_pEnvModel->m_pPolicyManager->GetPolicyIndexFromID(policyID);

         if (d.newValue == d.oldValue)
            duplicatePolicyCount++;

         int currentCount = pData->GetAsInt(actor + 1, policyIndex);
         currentCount++;
         pData->Set(actor + 1, policyIndex, currentCount);
         }
      }

   CString msg;
   msg.Format("Generated Policy By Actor Report - %i total policy applications, %i duplicates", policyApps, duplicatePolicyCount);
   Report::Log(msg);

   return pData;
   }

// -- FDataObj *DataManager::CalculateLulcTransTable(.) ----------------------------------------------------------
//
// The data object will have a column and row for each lucl class
// ---------------------------------------------------------------------------------------------------------------

FDataObj *DataManager::CalculateLulcTransTable( int run /*= -1*/, int level /*= 1*/, int year /*= -1*/, bool areaBasis /*= true*/, bool includeAlps /*= true*/ )
   {
   int         lulcCount   = m_pEnvModel->m_lulcTree.GetNodeCount( level );
   bool        errorInLulc = false;
   DeltaArray *pDeltaArray = GetDeltaArray( run );

   if ( pDeltaArray == nullptr )
      return nullptr;

   CMap< int, int, int, int > lulcMap;

   ASSERT( pDeltaArray != nullptr );
   ASSERT( lulcCount > 0 );

   FDataObj *pData = new FDataObj( lulcCount, lulcCount, U_UNDEFINED );
   pData->SetName( "Lulc Transition Table" );

   int lulcCol;
   switch ( level )
      {
      case 1:
         lulcCol = m_pEnvModel->m_colLulcA;
         break;
      case 2:
         lulcCol = m_pEnvModel->m_colLulcB;
         break;
      case 3:
         lulcCol = m_pEnvModel->m_colLulcC;
         break;
      case 4:
         lulcCol = m_pEnvModel->m_colLulcD;
         break;
      default:
         ASSERT(0);
      }

   LulcNode *pNode = m_pEnvModel->m_lulcTree.GetRootNode();
   int index = 0;
   while ( pNode != nullptr )
      {
      if ( level == pNode->GetNodeLevel() )
         {
         CString label;
         label.Format( "(%i) %s ", pNode->m_id, (PCTSTR) pNode->m_name );
         pData->SetLabel( index, label );
         lulcMap.SetAt( pNode->m_id, index );
         index++;
         }

      pNode = m_pEnvModel->m_lulcTree.GetNextNode();
      }

   ASSERT( index == lulcCount );

   // initilize all cells to zero 
   for ( int col=0; col<lulcCount; col++ )
      {
      for ( int row=0; row<lulcCount; row++ )
         pData->Set(col,row,0.0f);
      }

   // set map to initial conditions
   UINT_PTR firstUnapplied = m_pEnvModel->UnApplyDeltaArray( this->m_pEnvModel->m_pIDULayer );
   
   // populate transition table cells
   INT_PTR from;
   INT_PTR to;

   if ( year == -1 ) // meaning throughout the run
      {
      from = 0;
      to   = pDeltaArray->GetCount();
      }
   else  // meaning for a specific year
      {
      pDeltaArray->GetIndexRangeFromYear( year, from, to );
      if ( from != 0 )
         m_pEnvModel->ApplyDeltaArray( this->m_pEnvModel->m_pIDULayer, pDeltaArray, 0, from-1 );   // roll map forward to just prior to given year
      }

   // iterate through the specified year(s) delta's
   for ( INT_PTR i=from; i<to; i++ )
      {
      DELTA &delta = pDeltaArray->GetAt(i);
      m_pEnvModel->ApplyDeltaArray( this->m_pEnvModel->m_pIDULayer, pDeltaArray, i, i );

      if ( delta.col == lulcCol )
         {
         int fromLulc;
         int toLulc;
         bool ok; 
         ok = delta.oldValue.GetAsInt( fromLulc );
         ASSERT(ok);
         ok = delta.newValue.GetAsInt( toLulc );
         ASSERT(ok);

         if ( fromLulc != toLulc )  // ignore redundant changes
            {
            int row;
            int col;
            bool ok1 = lulcMap.Lookup( fromLulc, row );
            bool ok2 = lulcMap.Lookup( toLulc, col );
            if ( ok1 && ok2 )
               {
               //if ( !( ( includeAlps == FALSE ) && ( delta.type == DT_SUCCESSION ) ) )
               //   {
                  float v = pData->GetAsFloat( col, row );
                  if ( areaBasis )
                     {
                     float area;
                     bool ok = this->m_pEnvModel->m_pIDULayer->GetData( delta.cell, this->m_pEnvModel->m_colArea, area );
                     ASSERT( ok );
                     v += area;
                     }
                  else
                     {
                     v += 1.0f;
                     }
                  pData->Set(col, row, v);
               //   }
               }
            else if ( errorInLulc == false )
               {
               errorInLulc = true;
               CString msg;
               msg.Format( "Calculating Lulc Transition Table for run %i, idu %i, year %i, level %i.  Undefined LULC value - From: %i  To: %i... Ignoring",
                     run, delta.cell, year, level, fromLulc, toLulc );
               Report::Log( msg );
               }
            }
         }
      }  // end of: for ( i < from; i < to; i++ )

   // done interating through delta for this year, roll map back and reset to initial conditions
   UINT_PTR _firstUnapplied = pDeltaArray->GetFirstUnapplied( this->m_pEnvModel->m_pIDULayer );

   if ( _firstUnapplied > 0 )
      m_pEnvModel->UnApplyDeltaArray( this->m_pEnvModel->m_pIDULayer, pDeltaArray, _firstUnapplied-1 );

   m_pEnvModel->ApplyDeltaArray( this->m_pEnvModel->m_pIDULayer, nullptr, 0, firstUnapplied == 0 ? 0 : firstUnapplied-1 );      // return map to it's starting state

   return pData;
   }


FDataObj* DataManager::CalculateScenarioParametersFreq(int multiRun /*= -1 */)
   {
   if (this->m_collectMultiRunOutput == false)
      return nullptr;

   FDataObj* pData = (FDataObj*)this->GetMultiRunDataObj(DT_MULTI_PARAMETERS, multiRun);
   ASSERT(pData != nullptr);

   return CalculateFreqTable(pData, 20, true);   // 20 bins, ignore first col
   }


FDataObj* DataManager::CalculateFreqTable(FDataObj* pSourceData, int binCount, bool ignoreFirstCol /*=true*/)
   {
   int cols = pSourceData->GetColCount();
   int rows = pSourceData->GetRowCount();

   if (ignoreFirstCol)
      --cols;

   // new data object will have a column for each column in the original dataobj, and a row for each bin

   // create a dataobj for the histogram
   FDataObj* pData = new FDataObj(cols + 1, binCount, 0.0f, U_UNDEFINED);
   CString name(pSourceData->GetName());
   name += " - Frequency Table";
   pData->SetName(name);

   pData->SetLabel(0, "Bin");
   for (int i = 0; i < cols; i++)
      {
      int fromCol = ignoreFirstCol ? i + 1 : i;
      pData->SetLabel(i + 1, pSourceData->GetLabel(fromCol));
      }

   for (int i = 0; i < binCount; i++)
      pData->Set(0, i, i);

   // figure out bin ranges based on mins/maxs of data
   float* minVal = new float[cols];
   float* maxVal = new float[cols];

   // get min/max values for each column for scaling
   for (int i = 0; i < cols; i++)
      {
      int fromCol = ignoreFirstCol ? i + 1 : i;
      float _minVal, _maxVal;
      pSourceData->GetMinMax(fromCol, &_minVal, &_maxVal);
      minVal[i] = _minVal;
      maxVal[i] = _maxVal;
      }

   float value;
   for (int i = 0; i < cols; i++)
      {
      int fromCol = ignoreFirstCol ? i + 1 : i;

      for (int j = 0; j < rows; j++)
         {
         bool ok = pSourceData->Get(fromCol, j, value);
         ASSERT(ok);

         // figure out which bin it belongs in
         int bin = int((value - minVal[i]) * binCount / (maxVal[i] - minVal[i]));
         if (bin < 0) bin = 0;
         if (bin > binCount - 1) bin = binCount - 1;

         // update the frequency;
         pData->Add(i + 1, bin, 1);
         }
      }

   return pData;
   }

bool DataManager::SaveRun(LPCTSTR fileName, int run /*= -1*/)
   {
   if ((run != -1 && run < 0 && run >= GetDeltaArrayCount()) || fileName == nullptr)
      {
      ASSERT(0);
      return false;
      }

   DeltaArray* pDeltaArray = GetDeltaArray(run);

   FILE* fp;
   fopen_s(&fp, fileName, "wb");
   if (fp == nullptr)
      {
      CString msg = "Error opening the file ";
      msg += fileName;
      Report::ErrorMsg(msg);
      return false;
      }

   // Save Version Number
   int version = 1;
   if (fwrite(&version, sizeof(version), 1, fp) != 1)
      return false;

   // Save DeltaArray
   if (pDeltaArray && !pDeltaArray->Save(fp))
      return false;

   // Save Data Objects
   for (int i = 0; i < DT_LAST; i++)
      {
      if (!SaveDataObj(fp, *m_currentDataObjs[i]))
         return false;
      }

   return true;
   }

bool DataManager::LoadRun(LPCTSTR fileName)
   {
   FILE* fp;
   fopen_s(&fp, fileName, "rb");
   if (fp == nullptr)
      {
      CString msg = "Error opening the file ";
      msg += fileName;
      Report::ErrorMsg(msg);
      return false;
      }

   // Load Version Number
   int version;
   if (fread(&version, sizeof(version), 1, fp) != 1)
      return false;

   // Load DeltaArray
   ASSERT(this->m_pEnvModel->m_pIDULayer);
   m_pDeltaArray = new DeltaArray(this->m_pEnvModel->m_pIDULayer, m_pEnvModel->m_startYear);
   int run = (int)m_deltaArrayArray.Add(m_pDeltaArray);
   if (!m_pDeltaArray->Load(fp))
      return false;

   // Load Data Objects   
   for (int i = 0; i < DT_LAST; i++)
      {
      ASSERT(m_currentDataObjs[i] == nullptr);
      FDataObj* pData = new FDataObj(U_YEARS);
      LoadDataObj(fp, *pData);
      m_currentDataObjs[i] = pData;
      int r = (int)m_dataObjs[i].Add(pData);
      ASSERT(r == run);
      }

   CString runName = fileName;
   int index = runName.Find(".edd");
   if (index != -1)
      runName.Delete(index, runName.GetLength());

   // this works but the cellLayer and the resultMapWnds are out of sync in the case of a single run

   //???Check
   //gpResultsPanel->AddRun( run );

   return true;
   }

bool DataManager::ExportRunDeltaArray(LPCTSTR _path, int run, int startYear, int endYear, bool selectedOnly, bool includeDuplicates, LPCTSTR fieldList /*=nullptr*/)
   {
   //--------------------------------------------------
   // Get the appropriate DeltaArray to use; not necessarily the current one.
   ASSERT(run < m_deltaArrayArray.GetSize());

   if (this->m_collectMultiRunOutput && run != -1)
      return false;
   
   DeltaArray* pDeltaArray = (run > -1) ? m_deltaArrayArray.GetAt(run) : m_pDeltaArray;

   if (pDeltaArray == nullptr)
      return false;

   if (run < 0)
      run = (int)this->m_runInfoArray.GetCount() - 1;

   RUN_INFO& ri = this->m_runInfoArray[run];

   nsPath::CPath path;
   if (_path == nullptr)
      {
      //path = gpDoc->m_iniFile;
      //path.RemoveFileSpec();
      //path.AddBackslash();
      PathManager::GetPath(PM_PROJECT_DIR);  // always terminated with a '/'
      }
   else
      {
      path = _path;
      path.AddBackslash();
      }

   CString scname = ri.pScenario->m_name;
   int scrun = ri.scenarioRun;
   CleanFileName(scname);

   CString file;
   file.Format("DeltaArray_%s_Run%i.csv", (LPCTSTR)scname, scrun);
   path.Append(file);

   bool includeAllFields = (fieldList == nullptr || fieldList[0] ==NULL) ? true : false;
   bool selectedDates = (startYear >= 0 && endYear >= startYear) ? true : false;
   int fieldCount = this->m_pEnvModel->m_pIDULayer->GetFieldCount();

   bool* includeField = nullptr;

   if (!includeAllFields)
      {
      includeField = new bool[fieldCount];
      memset(includeField, 0, fieldCount * sizeof(bool)); // by default, don't include

      // parse fieldList
      LPTSTR next = nullptr;
      TCHAR* buffer = new TCHAR[lstrlen(fieldList) + 1];
      lstrcpy(buffer, fieldList);

      TCHAR* token = _tcstok_s(buffer, _T(",;"), &next);
      while (token != nullptr)
         {
         int col = this->m_pEnvModel->m_pIDULayer->GetFieldCol(token);

         if (col >= 0)
            includeField[col] = true;

         token = _tcstok_s(nullptr, _T(",;"), &next);
         }

      delete[] buffer;
      }

   INT_PTR size = pDeltaArray->GetSize();

   WAIT_CURSOR;

   FILE* fp;
   fopen_s(&fp, path, "wt");     // open for writing

   if (fp == nullptr)
      {
#ifndef NO_MFC
      char buffer[512];
      strcpy_s(buffer, 512, "Couldn't open file ");
      strcat_s(buffer, 512, path);
      AfxMessageBox(buffer, MB_OK);
#endif
      return false;
      }

   fputs(_T("year,idu,col,field,oldValue,newValue,type\n"), fp);

   INT_PTR count = 0;

   for (INT_PTR i = 0; i < size; i++)
      {
      DELTA& delta = pDeltaArray->GetAt(i);

      // selected only?
      if (selectedOnly && (this->m_pEnvModel->m_pIDULayer->IsSelected(delta.cell) == false))
         continue;

      // avoid duplicates?
      if (i > 0 && includeDuplicates == false && delta.oldValue.Compare(delta.newValue) == true)
         continue;

      // in year range?
      if (selectedDates && (delta.year < startYear || delta.year > endYear))
         continue;

      // include field?
      if (!includeAllFields && includeField[delta.col] == false) // not checked?
         continue;

      // passes, write
      count++;
      CString oldValue(delta.oldValue.GetAsString());
      CString newValue(delta.newValue.GetAsString());

      fprintf_s(fp, _T("%i,%i,%i,%s,%s,%s,%i\n"),
         delta.year, delta.cell, delta.col, this->m_pEnvModel->m_pIDULayer->GetFieldLabel(delta.col), (LPCTSTR)oldValue, (LPCTSTR)newValue, delta.type);
      }

   // clean up
   if (includeField != nullptr)
      delete[] includeField;

   fclose(fp);
   CString msg;
   msg.Format(_T("Successfully wrote %i delta array items to %s"), (int)count, (LPCTSTR)path);
   Report::Log(msg);
   return true;
   }

bool DataManager::ExportRunData(LPCTSTR _path, int run /*=-1*/)   /// Note: run is 0-based, doesn't include m_startRunNumber
   {
   //if (this->m_collectMultiRunOutput && run != -1)
   //   return false;

   if (run < 0)
      run = (int)this->m_runInfoArray.GetCount() - 1;

   RUN_INFO& ri = this->m_runInfoArray[run];

   int scrun = ri.scenarioRun;
   CString scname = ri.pScenario->m_name;

   CleanFileName(scname);

   nsPath::CPath exportPath;
   if (_path == nullptr)
      exportPath = PathManager::GetPath(PM_OUTPUT_DIR);
   else
      exportPath = _path;

   CString filename;

   // write data objects
   DataObj* pData = nullptr;

   for (int j = 0; j < DT_LAST; j++)
      {
      DataObjArray& dataObjArray = m_dataObjs[j];

      if (dataObjArray.GetSize() > 0)
         {
         pData = dataObjArray[run];
         if (pData != nullptr)
            {
            CString dataObjName(pData->GetName());
            CleanFileName(dataObjName);  // replace illegal chars with '_'

            filename.Format("%s_%s_Run%i.csv", (LPCTSTR)dataObjName, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);
            ExportDataObject(pData, exportPath, filename);

            // if there are any sub-dataobjs, export those too.
            int cols = pData->GetColCount();
            for (int col = 0; col < cols; col++)
               {
               VData v;
               pData->Get(col, 0, v);

               if (v.GetType() == TYPE_PDATAOBJ)
                  {
                  DataObj* pSubData = v.val.vPtrDataObj;
                  ASSERT(pSubData != nullptr);

                  CString subDOName(pSubData->GetName());
                  if (subDOName.IsEmpty())
                     subDOName = dataObjName + "_unnamed";

                  filename.Format("%s_%s_Run%i.csv", (LPCTSTR)subDOName, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);

                  CleanFileName(filename);

                  ExportDataObject(pSubData, exportPath, filename);
                  }
               }
            }
         }
      }

   int lulcLevels = this->m_pEnvModel->m_lulcTree.GetLevels();

   if (lulcLevels > 0)
      {
      pData = CalculateLulcTrends(1, run);

      if (pData)
         {
         CString dataObjName(pData->GetName());
         CleanFileName(dataObjName);  // replace illegal chars with '_'

         filename.Format("%s_%s_Run%i.csv", (LPCTSTR)dataObjName, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);
         ExportDataObject(pData, exportPath, filename);
         delete pData;
         }
      }

   if (lulcLevels > 1)
      {
      pData = CalculateLulcTrends(2, run);
      if (pData)
         {
         CString dataObjName(pData->GetName());
         CleanFileName(dataObjName);  // replace illegal chars with '_'

         filename.Format("%s_%s_Run%i.csv", (LPCTSTR)dataObjName, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);
         ExportDataObject(pData, exportPath, filename);
         delete pData;
         }
      }

   if (lulcLevels > 2)
      {
      pData = CalculateLulcTrends(3, run);
      if (pData)
         {
         CString dataObjName(pData->GetName());
         CleanFileName(dataObjName);  // replace illegal chars with '_'

         filename.Format("%s_%s_Run%i.csv", (LPCTSTR)dataObjName, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);
         ExportDataObject(pData, exportPath, filename);
         delete pData;
         }
      }

   pData = CalculatePolicyApps(run);

   if (pData)
      {
      CString dataObjName(pData->GetName());
      CleanFileName(dataObjName);  // replace illegal chars with '_'

      filename.Format("%s_%s_Run%i.csv", (LPCTSTR)dataObjName, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);
      ExportDataObject(pData, exportPath, filename);
      delete pData;
      }

   pData = this->CalculatePolicyEffectivenessTrendStatic(run);
   if (pData)
      {
      CString dataObjName = pData->GetName();
      CleanFileName(dataObjName);  // replace illegal chars with '_'

      filename.Format("%s_%s_Run%i.csv", (LPCTSTR)dataObjName, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);
      ExportDataObject(pData, exportPath, filename);
      delete pData;
      }

   pData = this->CalculateLulcTransTable();
   if (pData)
      {
      filename.Format("LULC_Trans_Table_%s_Run%i.csv", (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);
      ExportDataObject(pData, exportPath, filename);
      delete pData;
      }

   return true;
   }


bool DataManager::ExportRunMap(LPCTSTR path, int run /*=-1*/)    // assume path is terminated with a '\'
   {
   if (run < 0)
      run = (int)this->m_runInfoArray.GetCount() - 1;

   RUN_INFO& ri = this->m_runInfoArray[run];

   int scrun = ri.scenarioRun;
   CString scname = ri.pScenario->m_name;

   scname.Replace(' ', '_');    // blanks
   scname.Replace('\\', '_');   // illegal filename characters 
   scname.Replace('/', '_');
   scname.Replace(':', '_');
   scname.Replace('*', '_');
   scname.Replace('"', '_');
   scname.Replace('?', '_');
   scname.Replace('<', '_');
   scname.Replace('>', '_');
   scname.Replace('|', '_');


   Map* pMap = m_pEnvModel->m_pIDULayer->GetMapPtr();

   for (int i = 0; i < pMap->GetLayerCount(); i++)
      {
      MapLayer* pMapLayer = pMap->GetLayer(i);

      if (pMapLayer->m_output)
         {
         CString layerName = pMapLayer->m_name;
         layerName.Replace(' ', '_');

         CString filename;
         filename.Format(_T("%s%s_Year%i_%s_Run%i.shp"), path, (LPCTSTR)layerName, m_pEnvModel->m_currentYear, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber);

         CString msg("Exporting map: ");
         msg += filename;
         Report::StatusMsg(msg);

         pMapLayer->SaveShapeFile(filename);
         //m_pEnvModel->m_pIDULayer->SaveShapeFile(filename);
         }
      }
   return true;
   }


bool DataManager::ExportRunBmps(LPCTSTR path, int run /*=-1*/)    // assume path is terminated with a '\'
   {
   if (run < 0)
      run = (int)this->m_runInfoArray.GetCount() - 1;

   RUN_INFO& ri = this->m_runInfoArray[run];

   int scrun = ri.scenarioRun;
   CString scname = ri.pScenario->m_name;

   if (m_pEnvModel->m_exportBmpSize > 0)
      {
      scname.Replace(' ', '_');    // blanks
      scname.Replace('\\', '_');   // illegal filename characters 
      scname.Replace('/', '_');
      scname.Replace(':', '_');
      scname.Replace('*', '_');
      scname.Replace('"', '_');
      scname.Replace('?', '_');
      scname.Replace('<', '_');
      scname.Replace('>', '_');
      scname.Replace('|', '_');

      if (m_pEnvModel->m_exportBmpFields.GetLength() > 0)
         {
         TCHAR buffer[256];
         lstrcpy(buffer, m_pEnvModel->m_exportBmpFields);
         TCHAR* nextToken = nullptr;

         TCHAR* field = _tcstok_s(buffer, _T(";,"), &nextToken);

         while (field != nullptr)
            {
            int col = m_pEnvModel->m_pIDULayer->GetFieldCol(field);
            if (col < 0)
               {
               CString msg;
               msg.Format("Unable to find field '%s' when exporting end of run Bitmaps", field);
               Report::ErrorMsg(msg);
               }
            else
               {
               CString filename;
               filename.Format(_T("%s%s_Run%i_Year_%i_%s.bmp"), path, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber, m_pEnvModel->m_currentYear, field);

               CString msg("Exporting image: ");
               msg += filename;
               Report::StatusMsg(msg);

#ifdef _WINDOWS            
               //               Raster raster( m_pEnvModel->m_pIDULayer, false ); // don't copy polys, 
               //               raster.Rasterize( col, m_pEnvModel->m_exportBmpSize, RT_32BIT, 0, nullptr, DV_COLOR );
               //               raster.SaveDIB( filename );
#endif
               }

            field = _tcstok_s(nullptr, ";,", &nextToken);
            }
         }
      else
         {
         int activeCol = m_pEnvModel->m_pIDULayer->GetActiveField();

         if (activeCol >= 0)
            {
            CString filename;

            filename.Format(_T("%s%s_Run%iYear_%i_%s.bmp"), path, (LPCTSTR)scname, scrun + m_pEnvModel->m_startRunNumber, m_pEnvModel->m_currentYear, m_pEnvModel->m_pIDULayer->GetFieldLabel(activeCol));
            //filename.Format( _T("%sYear_%i_%s.bmp" ), path, m_currentYear, m_pIDULayer->GetFieldLabel( activeCol ) );
            CString msg("Exporting image: ");
            msg += filename;
            Report::StatusMsg(msg);

#ifdef _WINDOWS
            //            Raster raster( m_pEnvModel->m_pIDULayer, false ); // don't copy polys, 
            //            raster.Rasterize( activeCol, m_pEnvModel->m_exportBmpSize, RT_32BIT, 0, nullptr, DV_COLOR );
            //            raster.SaveDIB( filename );
#endif
            }
         }
      }
   return true;
   }



int DataManager::ImportMultiRunData(LPCTSTR path, int multirun /* =-1 */)
   {

   char curDir[_MAX_PATH];      // remeber the current directory

#ifndef NO_MFC
   CWaitCursor c;
   _getcwd(curDir, _MAX_PATH);

#else
   getcwd(curDir, _MAX_PATH);
#endif

   // clear out any existing datasets
   RemoveAllData();  // resets everything in DataManager

   char filename[_MAX_PATH];
   bool ok;

   // 
   Report::StatusMsg("Import multirun datasets...");

   // first, get the multirun dataobjs and read from the specified directory
   FDataObj* pData = new FDataObj(U_YEARS);
   pData->SetName("Multirun Landscape Evaluative Statistics");
   strcpy_s(filename, _MAX_PATH, path);
   strcat_s(filename, _MAX_PATH, "Multirun Landscape Evaluative Statistics.dat");
   pData->ReadAscii(filename, '\t');
   m_multiRunDataObjs[DT_MULTI_EVAL_SCORES].Add(pData);
   m_currentMultiRunDataObjs[DT_MULTI_EVAL_SCORES] = pData;

   // multirun raw scores
   pData = new FDataObj(U_YEARS);
   pData->SetName("Multirun Landscape Evaluative Statistics (Raw)");
   strcpy_s(filename, _MAX_PATH, path);
   strcat_s(filename, _MAX_PATH, "Multirun Landscape Evaluative Statistics (Raw).dat");
   pData->ReadAscii(filename, '\t');
   m_multiRunDataObjs[DT_MULTI_EVAL_RAWSCORES].Add(pData);
   m_currentMultiRunDataObjs[DT_MULTI_EVAL_RAWSCORES] = pData;

   // scenario settings  
   pData = new FDataObj(U_YEARS);
   pData->SetName("Multirun Scenario Settings");
   strcpy_s(filename, _MAX_PATH, path);
   strcat_s(filename, _MAX_PATH, "Multirun Scenario Settings.dat");
   pData->ReadAscii(filename, '\t');
   m_multiRunDataObjs[DT_MULTI_PARAMETERS].Add(pData);
   m_currentMultiRunDataObjs[DT_MULTI_PARAMETERS] = pData;

   // get the single run datasets
   // files are in directories named Run_XXX, where XXX is the run number.
   char scenario[256];
   lstrcpy(scenario, path);
   int position = lstrlen(scenario) - 1;
   if (scenario[position] == '\\')     // truncate terminal '\'
      scenario[position--] = 0;
   while (scenario[position] != '\\' && position > 0)
      position--;
   if (position != 0)
      position++;

   // position should now point to the start of the scenario name
   Scenario* pScenario = m_pEnvModel->m_pScenarioManager->GetScenario(scenario + position);
   ASSERT(pScenario != nullptr);
   pScenario->SetScenarioVars(SET_WITH_RANDOMIZATION);

   // find index of scenario used in multi run for UI setup and scenario configuration 
   int runScenario = -1;
   for (int i = 0; i < m_pEnvModel->m_pScenarioManager->GetCount(); i++)
      {
      if (m_pEnvModel->m_pScenarioManager->GetScenario(i)->m_name == pScenario->m_name)
         runScenario = i;
      }

   // get info for single run loop
   int runFirst = 0;    // for now...
   int runLast = 0;
   int run = 0;
   CString runPath;

   //----- single runs stuff ------//
   while (1)
      {
      if (run > 50) break;

      CString msg;
      msg.Format("Loading Run %i", run);
      Report::StatusMsg(msg);

      // load the delta array
      runPath.Format("%sRun_%i\\deltaArray.dab", path, run);

      DeltaArray* pDA = new DeltaArray(this->m_pEnvModel->m_pIDULayer, m_pEnvModel->m_startYear);
      ok = pDA->Load(runPath);

      if (!ok)
         break;

      //FixSubdividePointers(pDA);

      pDA->SetFirstUnapplied(this->m_pEnvModel->m_pIDULayer, SFU_END);   // In a true run, FirstUnapplied = delta count for each delta array
      m_deltaArrayArray.Add(pDA);
      m_pDeltaArray = pDA;
      m_pEnvModel->m_pDeltaArray = pDA;

      // add data objects

      // 1) Landscape Eval Statistics
      runPath.Format("%sRun_%i\\Landscape Evaluative Statistics.dat", path, run);
      pData = new FDataObj(U_YEARS);
      pData->SetName("Landscape Evaluative Statistics");
      pData->ReadAscii(runPath, '\t');
      m_currentDataObjs[DT_EVAL_SCORES] = pData;
      m_dataObjs[DT_EVAL_SCORES].Add(pData);

      runPath.Format("%sRun_%i\\Landscape Evaluative Statistics (Raw).dat", path, run);
      pData = new FDataObj(U_YEARS);
      pData->SetName("Landscape Evaluative Statistics (Raw)");
      pData->ReadAscii(runPath, '\t');
      m_currentDataObjs[DT_EVAL_RAWSCORES] = pData;
      m_dataObjs[DT_EVAL_RAWSCORES].Add(pData);

      // 2) Actor Value Trajectories
      runPath.Format("%sRun_%i\\Actor Value Trajectories.dat", path, run);
      pData = new FDataObj(U_YEARS);
      pData->SetName("Actor Value Trajectories");
      pData->ReadAscii(runPath, '\t');
      m_currentDataObjs[DT_ACTOR_WTS] = pData;
      m_dataObjs[DT_ACTOR_WTS].Add(pData);

      // 3) Policy Initial Area Results Summary
      runPath.Format("%sRun_%i\\Policy Initial Area Results Summary.dat", path, run);
      pData = new FDataObj(U_YEARS);
      pData->SetName("Policy Initial Area Results Summary");
      pData->ReadAscii(runPath, '\t');
      m_currentDataObjs[DT_POLICY_SUMMARY] = pData;
      m_dataObjs[DT_POLICY_SUMMARY].Add(pData);

      // 4) Global Constraints
      runPath.Format("%sRun_%i\\Global Constraints Summary.dat", path, run);
      pData = new FDataObj(U_YEARS);
      pData->SetName("Global Constraints Summary");
      pData->ReadAscii(runPath, '\t');
      m_currentDataObjs[DT_GLOBAL_CONSTRAINTS] = pData;
      m_dataObjs[DT_GLOBAL_CONSTRAINTS].Add(pData);

      // 5) Policy Stats
      runPath.Format("%sRun_%i\\Policy Stats.dat", path, run);
      pData = new FDataObj(U_YEARS);
      pData->SetName("Policy Stats");
      pData->ReadAscii(runPath, '\t');
      m_currentDataObjs[DT_POLICY_STATS] = pData;
      m_dataObjs[DT_POLICY_STATS].Add(pData);

      // run info
      RUN_INFO ri(pScenario, 0, 0, 0);
      m_runInfoArray.Add(ri);  // NOTE:  startYear, endYEar not specified!!!!
      //???Check
      //gpResultsPanel->AddRun( run );
      run++;
      runLast++;
      }

   run--;
   runLast--;

   // set up the multiRunInfo
   MULTIRUN_INFO mri(runFirst, runLast, pScenario);
   m_multirunInfoArray.Add(mri);
   //???Check
   //gpResultsPanel->AddMultiRun( 0 );

   // everything taken care of in DataManager, now let other objects know.
   m_pEnvModel->m_currentMultiRun = 1;
   m_pEnvModel->m_currentRun = runLast;
   m_pEnvModel->SetScenario(pScenario);

   // import has set objects up as if run had occurred; reset to prepare for next run (as in 
   // EnvModel-->Run)
   m_pEnvModel->Reset();


#ifndef NO_MFC
   _chdir(curDir);
#else
   chdir(curDir);
#endif

   return runScenario;
   }

//------------------------------------------------------------------------------
// TEMPORARY FIX - RECREATES POINTERS TO SUBDIVIDED POLYGONS WHEN IMPORTING MULTI RUN
//    children polygons were not persisted with export
#include <PolyBuffer.h>
#include <PolyClipper.h>

bool DataManager::FixSubdividePointers(DeltaArray* pDA)
   {
   int count = 0;
   UINT_PTR deltas = pDA->GetCount();

   // loop through delta array, looking for subdivision deltas
   for (UINT_PTR d = 0; d < deltas; d++)
      {
      DELTA& deltaCheck = pDA->GetAt(d);

      if (this->m_pEnvModel->m_pIDULayer->IsDefunct(deltaCheck.cell))
         {
         pDA->RemoveAt(d);
         deltas--;
         d--;
         continue;
         }

      // not a subdivision? skip.
      if (deltaCheck.type != DT_SUBDIVIDE) //.col != DELTA::SUBDIVIDE)
         {
         count += m_pEnvModel->ApplyDelta(this->m_pEnvModel->m_pIDULayer, deltaCheck);
         continue;
         }

      // find policy offset that caused subdivision
      int back = 1;
      int policyID;

      while (pDA->GetAt(d - back).col != 50 && back <= 4)
         back++;

      if (back > 4)
         this->m_pEnvModel->m_pIDULayer->GetData(deltaCheck.cell, this->m_pEnvModel->m_colPolicy, policyID);
      else
         pDA->GetAt(d - back).newValue.GetAsInt(policyID);

      // hard coded distance and outcome strings based on offset in policy table
      float distance;
      int issue = 0;
      switch (policyID)
         {
         case 20:
         case 21:
         case 72:
         case 73:
         {distance = 100; break; }
         case 74:
         case 76:
         case 88:
         case 89:
         case 90:
         case 91:
         {distance = 46; break; }
         case 78:
         case 81:
         case 86:
         case 87:
         {distance = 30; break; }
         case 85:
         {distance = 122; break; }
         }

      CString qryString;
      switch (policyID)
         {
         case 20:
         case 21:
         {
         qryString = "Lulc_A = 7 {Water} and Lulc_C != 32"; // ??? BAD!!!!  Shouldn't hardcode LULC_A
         break;
         }
         case 72:
         case 73:
         case 74:
         case 76:
         case 78:
         case 81:
         case 85:
         case 86:
         case 87:
         case 88:
         case 89:
         case 90:
         case 91:
         {
         qryString = "Lulc_C = 30 or Lulc_C=31 {buffer stream or river}";
         break;
         }
         }

      // create query from query string
      //QueryEngine *queryEng = new QueryEngine(this->m_pEnvModel->m_pIDULayer);
      QueryEngine* queryEng = this->m_pEnvModel->m_pIDULayer->GetQueryEngine();

      Query* qry = queryEng->ParseQuery(qryString, 0, "FixSubdividePointers()");
      if (qry == nullptr)
         {
         ASSERT(0);
         }

      //--------------------------------------------------------------   
      //--------------------------------------------------------------
      // perform buffer operation - code from BufferOutcomeFunction (minus assertions)
      Poly* pPoly = this->m_pEnvModel->m_pIDULayer->GetPolygon(deltaCheck.cell);
      Vertex center = pPoly->GetCentroid();

      int* neighbors = nullptr;
      int count = 0;
      int n = 0;

      do
         {
         if (n > 0)
            delete[] neighbors;
         n += 100;
         neighbors = new int[n];
         count = this->m_pEnvModel->m_pIDULayer->GetNearbyPolysFromIndex(pPoly, neighbors, nullptr, n, 0);
         } while (n <= count);

         if (count < 0)
            {
            delete[] neighbors;
            ASSERT(0);
            }

         Poly bufferPoly;
         Poly bufferUnion;

         for (int i = 0; i < count; i++)
            {
            bool result;
            if (queryEng->Run(qry, neighbors[i], result) == false)  //WOW 08Feb2006
               {
               delete[] neighbors;
               ASSERT(0);
               }

            if (result)
               {
               Poly* pPoly = this->m_pEnvModel->m_pIDULayer->GetPolygon(neighbors[i]);
               ASSERT(pPoly);

               PolyBuffer::MakeBuffer(*pPoly, bufferPoly, distance);

               if (bufferUnion.GetVertexCount() > 0)
                  {
                  PolyClipper::PolygonClip(GPC_UNION, bufferUnion, bufferPoly, bufferUnion);
                  }
               else
                  {
                  bufferUnion = bufferPoly;
                  }
               }
            }

         delete[] neighbors;

         if (bufferUnion.GetVertexCount() > 0)
            {
            Poly* pPoly = this->m_pEnvModel->m_pIDULayer->GetPolygon(deltaCheck.cell);

            Poly* pInside = new Poly;
            Poly* pOutside = new Poly;

            PolyClipper::PolygonClip(GPC_INT, *pPoly, bufferUnion, *pInside);
            PolyClipper::PolygonClip(GPC_DIFF, *pPoly, bufferUnion, *pOutside);

            // TODO: Seem to get negative areas too much
            //       There may be a bug!!!!!!
            if (pInside->GetVertexCount() == 0 || pInside->GetArea() <= 0)
               {
               delete pInside;
               delete pOutside;

               //return false;

               pDA->RemoveAt(d);
               deltas--;
               d--;
               continue;
               }
            if (pOutside->GetVertexCount() == 0 || pOutside->GetArea() <= 0)
               {
               delete pInside;
               delete pOutside;

               /*int outcome = m_pEnvModel->SelectPolicyOutcome( m_insideBufferMultiOutcomeArray );
               if (outcome >= 0)
                  {
                  const MultiOutcomeInfo& bMultiOutcome = m_insideBufferMultiOutcomeArray.GetMultiOutcome( outcome );
                  m_pEnvModel->ApplyMultiOutcome( bMultiOutcome, cell, persistence );
                  }
               return true;*/

               pDA->RemoveAt(d);
               deltas--;
               d--;
               continue;
               }

            PolyArray replacementList;
            replacementList.Add(pInside);
            replacementList.Add(pOutside);
            //--------------------------------------------------------------   
            //--------------------------------------------------------------   

               // point delta to children polygons
            PolyArray* pArray = new PolyArray(replacementList); // Shallow Copy

            int dynamicPolyCount = this->m_pEnvModel->m_pIDULayer->GetRecordCount(MapLayer::ALL);

            for (int i = 0; i < pArray->GetCount(); i++)
               {
               pArray->GetAt(i)->m_id = dynamicPolyCount;
               dynamicPolyCount++;
               }

            deltaCheck.newValue = (void*)pArray;
            count += m_pEnvModel->ApplyDelta(this->m_pEnvModel->m_pIDULayer, deltaCheck);
            }

         //delete queryEng;
      }

   // reset map to starting state
   pDA->SetFirstUnapplied(this->m_pEnvModel->m_pIDULayer, SFU_END);      // deltas have no concept of the delta array they belong to, so ApplyDelta does not set FirstUnapplied
   m_pEnvModel->UnApplyDeltaArray(this->m_pEnvModel->m_pIDULayer, pDA);

   return true;
   }

bool DataManager::ExportMultiRunData(LPCTSTR _path, int multirun /*=-1*/)
   {
   if ( this->m_collectMultiRunOutput == false)
      return false;
   
   WAIT_CURSOR;

   // test passed multi-run parameter, then get first and last run numbers from array
   if (multirun == -1)
      multirun = int(m_multirunInfoArray.GetSize()) - 1;

   if (multirun < 0)
      return false;

   int runFirst = m_multirunInfoArray[multirun].runFirst;
   int runLast = m_multirunInfoArray[multirun].runLast;

   // append scenario name to path and create directory
   CString path = PathManager::GetPath(PM_PROJECT_DIR);
   path += "Outputs\\";
   path += m_multirunInfoArray[multirun].pScenario->m_name;
   path += "\\";
   CleanFileName(path);

#ifndef NO_MFC
   SHCreateDirectoryEx(nullptr, path, nullptr);
#else
   mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

   //int retVal = _mkdir( path );
   //
   //if ( retVal != 0 )
   //{  //TODO: add code to handle directory overwrite
   //  CString msg;
   //   msg = "Unable to create directory: ";
   //   msg += path;
   //   Report::ErrorMsg(msg);
   //   return false;
   //}

   //------ multirun stuff ----//
   Report::StatusMsg("Writing multirun datasets...");
   for (int i = 0; i < DT_MULTI_LAST; i++)
      {
      // first, get the multirun dataobj and write to the specified directory
      DataObjArray& multiDataObjArray = m_multiRunDataObjs[i];
      DataObj* pData = multiDataObjArray[multirun];
      ExportDataObject(pData, path);
      }

   DataObj* pData = this->CalculateScenarioParametersFreq(multirun);
   ExportDataObject(pData, path);
   delete pData;

   //pData = this->CalculateScenarioVulnerability( multirun );
   //ExportDataObject( pData, path );
   //delete pData;

   pData = CalculateEvalFreq(multirun);
   ExportDataObject(pData, path);
   delete pData;

   //pData = this->CalculateDecadalLUStatusVariance( multirun );
   //ExportDataObject( pData, path );
   //delete pData;

   //----- single runs stuff ------//
   for (int run = runFirst; run <= runLast; run++)
      {
      CString runPath;
      runPath.Format("%s\\Run_%i\\", (PCTSTR)path, run + m_pEnvModel->m_startRunNumber);  // need to revisit!
      ExportRunData(runPath, run);
      }

   return true;
   }

bool DataManager::ExportDataObject(DataObj* pData, LPCTSTR _path, LPCTSTR filename /*=nullptr*/)
   {
   nsPath::CPath path;
   if (_path != nullptr)
      {
      path = _path;
      path.AddBackslash();
      }

   if (filename)
      path.Append(filename);
   else
      {
      path.Append(pData->GetName());
      path.AddExtension("csv");
      }

   CString msg("Exporting file: ");
   msg += path;
   Report::StatusMsg(msg);

   Report::Log(msg);
   pData->WriteAscii(path, ',');

   msg += "...completed";
   Report::StatusMsg(msg);

   return true;
   }


bool DataManager::SaveDataObj(FILE* fp, DataObj& dataObj)
   {
   CString str;

   str = dataObj.GetName();

   if (!SaveCString(fp, str))
      return false;

   int rowCount = dataObj.GetRowCount();
   int colCount = dataObj.GetColCount();

   if (fwrite(&rowCount, sizeof(rowCount), 1, fp) != 1)
      return false;
   if (fwrite(&colCount, sizeof(colCount), 1, fp) != 1)
      return false;

   for (int i = 0; i < colCount; i++)
      {
      str = dataObj.GetLabel(i);
      if (!SaveCString(fp, str))
         return false;
      }

   switch (dataObj.GetDOType())
      {
      case DOT_FLOAT:
      {
      float value;
      for (int row = 0; row < rowCount; row++)
         for (int col = 0; col < colCount; col++)
            {
            dataObj.Get(col, row, value);
            if (fwrite(&value, sizeof(value), 1, fp) != 1)
               return false;
            }
      }
      break;

      case DOT_INT:
      {
      int value;
      for (int row = 0; row < rowCount; row++)
         for (int col = 0; col < colCount; col++)
            {
            dataObj.Get(col, row, value);
            if (fwrite(&value, sizeof(value), 1, fp) != 1)
               return false;
            }
      }
      break;

      case DOT_VDATA:
      {
      VData value;
      for (int row = 0; row < rowCount; row++)
         for (int col = 0; col < colCount; col++)
            {
            dataObj.Get(col, row, value);
            if (fwrite(&value, sizeof(value), 1, fp) != 1)
               return false;
            }
      }
      break;

      default:
      {
      ASSERT(0);
      return false;
      }
      }

   return true;
   }

bool DataManager::LoadDataObj(FILE* fp, DataObj& dataObj)
   {
   CString str;
   if (!LoadCString(fp, str))
      return false;
   dataObj.SetName(str);

   int rowCount;
   int colCount;

   if (fread(&rowCount, sizeof(rowCount), 1, fp) != 1)
      return false;
   if (fread(&colCount, sizeof(colCount), 1, fp) != 1)
      return false;

   dataObj.SetSize(colCount, rowCount);

   for (int i = 0; i < colCount; i++)
      {
      if (!LoadCString(fp, str))
         return false;
      dataObj.SetLabel(i, str);
      }

   switch (dataObj.GetDOType())
      {
      case DOT_FLOAT:
      {
      float value;
      for (int row = 0; row < rowCount; row++)
         for (int col = 0; col < colCount; col++)
            {
            if (fread(&value, sizeof(value), 1, fp) != 1)
               return false;
            dataObj.Set(col, row, value);
            }
      }
      break;

      case DOT_INT:
      {
      int value;
      for (int row = 0; row < rowCount; row++)
         for (int col = 0; col < colCount; col++)
            {
            if (fread(&value, sizeof(value), 1, fp) != 1)
               return false;
            dataObj.Set(col, row, value);
            }
      }
      break;

      case DOT_VDATA:
      {
      VData value;
      for (int row = 0; row < rowCount; row++)
         for (int col = 0; col < colCount; col++)
            {
            if (fread(&value, sizeof(value), 1, fp) != 1)
               return false;
            dataObj.Set(col, row, value);
            }
      }
      break;

      default:
      {
      ASSERT(0);
      return false;
      }
      }

   return true;
   }

bool DataManager::SaveCString(FILE* fp, CString& string)
   {
   int length = string.GetLength();

   if (fwrite(&length, sizeof(length), 1, fp) != 1)
      return false;

   char c;
   for (int i = 0; i < length; i++)
      {
      c = (char)string.GetAt(i);
      if (fwrite(&c, sizeof(char), 1, fp) != 1)
         return false;
      }

   return true;
   }

bool DataManager::LoadCString(FILE* fp, CString& string)
   {
   string = "";
   int length;

   if (fread(&length, sizeof(length), 1, fp) != 1)
      return false;

   char c;
   for (int i = 0; i < length; i++)
      {
      if (fread(&c, sizeof(char), 1, fp) != 1)
         return false;
      string += c;
      }

   return true;
   }

/*
int DataManager::CleanFileName ( CString &filename )
   {
   LPSTR ptr = filename.GetBuffer();

   while ( *ptr != nullptr )
      {
      switch( *ptr )
         {
         case ' ':   *ptr = '_';    break;  // blanks
         case '\\':  *ptr = '_';    break;  // illegal filename characters
         case '/':   *ptr = '_';    break;
         case ':':   *ptr = '_';    break;
         case '*':   *ptr = '_';    break;
         case '"':   *ptr = '_';    break;
         case '?':   *ptr = '_';    break;
         case '<':   *ptr = '_';    break;
         case '>':   *ptr = '_';    break;
         case '|':   *ptr = '_';    break;
         }
     ptr++;
      }

   filename.ReleaseBuffer();
   return filename.GetLength();
   }
*/
