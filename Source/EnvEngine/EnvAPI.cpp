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
// EnvEngine.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "EnvAPI.h"

#include <Maplayer.h>
#include <QueryEngine.h>
#include "EnvModel.h"
#include "EnvLoader.h"
#include "Actor.h"
#include "Policy.h"
#include "Scenario.h"
#include <iostream>

void RunModel(EnvModel*);


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void _LogMsgProc(LPCTSTR msg, LPCTSTR hdr, int type);
void _StatusMsgProc(LPCTSTR msg);
int  _PopupMsgProc(LPCTSTR hdr, LPCTSTR msg, int flags);


// called whenever a Report message is invoked (e.g. Report::Log();
void _LogMsgProc(LPCTSTR msg, REPORT_ACTION action, REPORT_TYPE type)
   {
   LPCTSTR msgType;

   switch (type)
      {
      case RT_WARNING:
         msgType = _T("Warning:");
         break;

      case RT_ERROR:
      case RT_FATAL:
      case RT_SYSTEM:
         msgType = _T("Error:");
         break;

      default:
         msgType = _T("Info:");
         break;
      }

   CString _msg;
   _msg.Format("%s: %s", msgType, msg);
   std::cout << msg << std::endl;
   }


void _StatusMsgProc(LPCTSTR msg)
   {
   std::cout << "Status: " << msg << std::endl;
   }

// called whenever a Report message is invoked (e.g. Report::Log();
int _PopupMsgProc(LPCTSTR hdr, LPCTSTR msg, int flags)
   {
   std::cout << hdr << ": " << msg << std::endl;
   return 0;
   }

void RunModel(EnvModel *pModel)
   {
   if (pModel->m_runStatus == RS_RUNNING)
      {
      return;
      }

   if (pModel->m_currentYear != pModel->m_startYear)
      pModel->Reset();

   if (pModel->m_areModelsInitialized == false)
      pModel->InitModels();


   //try
   //   {
   pModel->Run(0);      // don't randomize
                        //   }
                        //catch ( EnvFatalException & ex )
                        //   {
                        //   CString msg;
                        //   msg.Format( "Fatal Error:\n  %s", ex.what() );
                        //   Report::ErrorMsg( msg );
                        //   }


   Report::StatusMsg("All Done!!!");
   }

#ifdef __cplusplus
   extern "C" {
#endif // 




int EnvInitEngine(int initFlags)
      {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      //AddGDALPath();

      return 1;
      }

EnvModel* EnvLoadProject(LPCTSTR envxFile, int initFlags)
    {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    EnvModel *pModel = new EnvModel;   // memory leak?
    Map *pMap = new Map;

    //theApp.AddJob(pModel, pMap);

    Report::logMsgProc = _LogMsgProc;
    Report::statusMsgProc = _StatusMsgProc;
    Report::popupMsgProc = _PopupMsgProc;

    EnvLoader loader;
    int result = loader.LoadProject(envxFile, pMap, pModel);

    if (result < 0)
       return NULL;

    // set up bins for loaded layer(s)
    for (int i = 0; i < pMap->GetLayerCount(); i++)
       {
       if (pMap->GetLayer(i)->m_pData != NULL)
          Map::SetupBins(pMap, i, -1);
       }

    // add a field for each actor value; cf CEnvDoc::CEnvDoc()
    for (int i = 0; i < pModel->GetActorValueCount(); i++)
       {
       METAGOAL *pInfo = pModel->GetActorValueMetagoalInfo(i);
       CString label(pInfo->name);
       label += " Actor Value";

       CString field;
       field.Format("ACTOR%s", (PCTSTR)pInfo->name);
       if (field.GetLength() > 10)
          field.Truncate(10);
       field.Replace(' ', '_');

       pModel->m_pIDULayer->AddFieldInfo(field, 0, label, _T(""), _T(""), TYPE_FLOAT, MFIT_QUANTITYBINS, BCF_GREEN_INCR, 0, true);
       }

    int defaultScenario = pModel->m_pScenarioManager->GetDefaultScenario();
    pModel->SetScenario(pModel->m_pScenarioManager->GetScenario(defaultScenario));

    //LoadExtensions();

    // reset EnvModel
    pModel->Reset();

    int levels = pModel->m_lulcTree.GetLevels();

    switch (levels)
       {
       case 1:  pModel->m_pIDULayer->CopyColData(pModel->m_colStartLulc, pModel->m_colLulcA);   break;
       case 2:  pModel->m_pIDULayer->CopyColData(pModel->m_colStartLulc, pModel->m_colLulcB);   break;
       case 3:  pModel->m_pIDULayer->CopyColData(pModel->m_colStartLulc, pModel->m_colLulcC);   break;
       case 4:  pModel->m_pIDULayer->CopyColData(pModel->m_colStartLulc, pModel->m_colLulcD);   break;
       }

    return pModel;
    }


// scenario is zero-based, -1=run all
int EnvRunScenario(EnvModel *pModel, int scenario, int simulationLength, int runFlags)
    {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (pModel == NULL)
       {
       Report::ErrorMsg("No Envision Model is defined");
       return -10;
       }

    ScenarioManager *pScenarioManager = pModel->m_pScenarioManager;

    pModel->m_yearsToRun = simulationLength;
    pModel->m_endYear = pModel->m_startYear + simulationLength;

    if (scenario < 0)  // run all?
       {
       for (int i = 0; i < pScenarioManager->GetCount(); i++)
          {
          Scenario *pScenario = pScenarioManager->GetScenario(i);
          pModel->SetScenario(pScenario);
          RunModel(pModel);
          }

       pModel->SetScenario(NULL);
       }
    else   // a specific scenario specified
       {
       pModel->SetScenario(pScenarioManager->GetScenario(scenario));
       RunModel(pModel);
       }
    return 1;
   }     // note: scenario is zero-based, use -1 to run all scenarios



int EnvCloseProject(EnvModel *pModel, int closeFlags)
   {
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ((closeFlags & ENVCP_PRESERVE_MAP ) == 0 )
      {
      Map *pMap = pModel->m_pIDULayer->m_pMap;
      delete pMap;
      }

   delete pModel;
   return 0;
   }


int EnvCloseEngine()
    {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //theApp.RemoveJobs();

    return 1;
    }




DELTA& EnvGetDelta(DeltaArray *da, INT_PTR index) { return da->GetAt(index); }
int EnvApplyDeltaArray(EnvModel *pModel) { return pModel->ApplyDeltaArray(pModel->m_pIDULayer); }

int EnvGetEvaluatorCount( EnvModel *pModel ) { return pModel->GetEvaluatorCount(); }
EnvEvaluator* EnvGetEvaluatorInfo(EnvModel *pModel, int i) { return pModel->GetEvaluatorInfo(i); }
EnvEvaluator* EnvFindEvaluatorInfo(EnvModel *pModel, LPCTSTR name) { return pModel->FindEvaluatorInfo(name); }

int EnvGetAutoProcessCount( EnvModel *pModel) { return pModel->GetModelProcessCount(); }
EnvModelProcess* EnvGetAutoProcessInfo(EnvModel *pModel, int i) { return pModel->GetModelProcessInfo(i); }

int EnvChangeIDUActor(EnvContext *pContext, int idu, int groupID, bool randomize) { return pContext->pEnvModel->ChangeIDUActor(pContext, idu, groupID, randomize); }

// policy related methods
int     EnvGetPolicyCount(EnvModel *pModel) { return pModel->m_pPolicyManager->GetPolicyCount(); }
int     EnvGetUsedPolicyCount(EnvModel *pModel) { return pModel->m_pPolicyManager->GetUsedPolicyCount(); }
Policy *EnvGetPolicy(EnvModel *pModel, int i) { return pModel->m_pPolicyManager->GetPolicy(i); }
Policy *EnvGetPolicyFromID(EnvModel *pModel, int id) { return pModel->m_pPolicyManager->GetPolicyFromID(id); }


// Scenario related methods
int       EnvGetScenarioCount(EnvModel *pModel) { return pModel->m_pScenarioManager->GetCount(); }
Scenario *EnvGetScenario(EnvModel *pModel, int i) { return pModel->m_pScenarioManager->GetScenario(i); }
Scenario *EnvGetScenarioFromName(EnvModel *pModel, LPCTSTR name, int *index)
   {
   Scenario *pScenario = pModel->m_pScenarioManager->GetScenario(name);

   if (index != NULL && pScenario != NULL)
      {
      *index = pModel->m_pScenarioManager->GetScenarioIndex(pScenario);
      }

   return pScenario;
   }

//int EnvStandardizeOutputFilename( LPTSTR filename, LPTSTR pathAndFilename, int maxLength )
//   {
//   EnvCleanFileName( filename );
//    
//   CString path = PathManager::GetPath( PM_OUTPUT_DIR );  // {ProjectDir}\Outputs\CurrentScenarioName\
//   path += filename;
//   _tcsncpy( pathAndFilename, (LPCTSTR) path, maxLength );
//
//   return path.GetLength();
//   }

int EnvCleanFileName(LPTSTR filename)
   {
   if (filename == NULL)
      return 0;

   TCHAR *ptr = filename;

   while (*ptr != NULL)
      {
      switch (*ptr)
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

   return (int)_tcslen(filename);
   }


#ifdef __cplusplus
   }
#endif

