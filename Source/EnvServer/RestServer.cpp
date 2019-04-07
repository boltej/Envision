#include "stdafx.h"
#include "RestServer.h"

#include <iostream>
//#include <map>
//#include <set>
#include <chrono>
#include <string>
#include <algorithm>

#include <Path.h>
#include <Report.h>
#include <tixml.h>
#include "EnvServerDlg.h"
#include <EnvModel.h>
#include <DataManager.h>
#include <FDATAOBJ.H>

#include <misc.h>

#include <cstdio>
#include <cstdarg>

#include <iostream>
#include <fstream>

using namespace std;

//#if __cplusplus < 201703L
//#include <memory>
//#endif


using namespace std::chrono;

shared_ptr< Session > gpSession = nullptr;

#define _CRT_SECURE_NO_WARNINGS

// globals
extern CEnvServerDlg *gpMainWnd;

int RestServer::m_nextSessionID = 0;

// prototypes
// API
void InitSessionHandler(const shared_ptr< Session > session);
void GetProjectsHandler(const shared_ptr< Session > session);
void LoadProjectHandler(const shared_ptr< Session > session);
void GetOutputVarsHandler(const shared_ptr< Session > session);
void GetScenariosHandler(const shared_ptr< Session > session);
void GetPluginsHandler(const shared_ptr< Session > session);
void GetPoliciesHandler(const shared_ptr< Session > session);
void SetPoliciesHandler(const shared_ptr< Session > session);
void GetConfigHandler(const shared_ptr< Session > session);
void SetConfigHandler(const shared_ptr< Session > session);
void GetPluginsHandler(const shared_ptr< Session > session);
void GetAppVarsHandler(const shared_ptr< Session > session);
void SetAppVarsHandler(const shared_ptr< Session > session);
void GetDataHandler(const shared_ptr< Session > session);
void RunScenarioHandler(const shared_ptr< Session > session);
void CloseSessionHandler(const shared_ptr< Session > session);

void RegisterEventSrc(const shared_ptr< Session > session);

//////////////////////////
//void register_event_source_handler(const shared_ptr< Session > session);
//void event_stream_handler(void);
//////////////////////////


// internals
void ThreadMainFunction(RestThread*);
string format(const char *fmt, ...);
string readFile(const string &fileName);


//void EventHandler(void);

// globals
extern RestServer restServer;


int RestServer::Close()
   {
   //m_listener.close();
   for (int i = 0; i < m_threads.GetCount(); i++)
      {
      RestThread *pRT = m_threads[i];

      // terminate service
      if (pRT->m_pService != NULL)
         {
         pRT->m_pService->stop();
         delete pRT->m_pService;
         pRT->m_pService = NULL;
         }

      if (pRT->m_pThread != NULL && pRT->m_pThread->joinable())
         {
         pRT->m_pThread->join();
         delete pRT->m_pThread;
         pRT->m_pThread = NULL;
         }
      }
   m_threads.RemoveAll();

   return 0;
   }


int RestServer::StartThread()
   {
   RestThread *pRT = new RestThread;
   
   m_threads.Add(pRT);

   pRT->m_pThread = new thread(ThreadMainFunction,pRT);
    
   return (int) m_threads.GetCount();
   }
   

// note: we run the service on it's own thread so it doesn't interfer with the GUI
void ThreadMainFunction(RestThread *pRT)
   {
   // define and exposed resources (function) associated with an HTTP verb
   auto rInitSession = make_shared< Resource >();
   rInitSession->set_path("/EnvAPI/init-session");
   rInitSession->set_method_handler("POST", InitSessionHandler);

   auto rProjects = make_shared< Resource >();
   rProjects->set_path("/EnvAPI/projects");
   rProjects->set_method_handler("GET", GetProjectsHandler);

   auto rGetOutputs = make_shared< Resource >();
   rGetOutputs->set_path("/EnvAPI/output-vars");
   rGetOutputs->set_method_handler("GET", GetOutputVarsHandler);

   auto rGetScenarios = make_shared< Resource >();
   rGetScenarios->set_path("/EnvAPI/scenarios");
   rGetScenarios->set_method_handler("GET", GetScenariosHandler);

   auto rGetPlugins = make_shared< Resource >();
   rGetPlugins->set_path("/EnvAPI/plugins");
   rGetPlugins->set_method_handler("GET", GetPluginsHandler);

   auto rGetPolicies = make_shared< Resource >();
   rGetPolicies->set_path("/EnvAPI/policies");
   rGetPolicies->set_method_handler("GET", GetPoliciesHandler);

   auto rSetPolicies = make_shared< Resource >();
   rSetPolicies->set_path("/EnvAPI/set-policies");
   rSetPolicies->set_method_handler("POST", SetPoliciesHandler);

   auto rGetAppVars = make_shared< Resource >();
   rGetAppVars->set_path("/EnvAPI/appvars");
   rGetAppVars->set_method_handler("GET", GetAppVarsHandler);

   auto rSetAppVars = make_shared< Resource >();
   rSetAppVars->set_path("/EnvAPI/set-appvars");
   rSetAppVars->set_method_handler("POST", SetAppVarsHandler);

   auto rGetConfig = make_shared< Resource >();
   rGetConfig->set_path("/EnvAPI/config");
   rGetConfig->set_method_handler("GET", GetConfigHandler);

   auto rSetConfig = make_shared< Resource >();
   rSetConfig->set_path("/EnvAPI/set-config");
   rSetConfig->set_method_handler("POST", SetConfigHandler);

   auto rLoadProject = make_shared< Resource >();
   rLoadProject->set_path("/EnvAPI/load-project");
   rLoadProject->set_method_handler("POST", LoadProjectHandler);

   auto rRunScenario = make_shared< Resource >();
   rRunScenario->set_path("/EnvAPI/run-scenario");
   rRunScenario->set_method_handler("POST", RunScenarioHandler);

   auto rGetData = make_shared< Resource >();
   rGetData->set_path("/EnvAPI/data");
   rGetData->set_method_handler("GET", GetDataHandler);
   
   // event source - this will make a keep-open connection
   // for sending events to the client
   auto rRegEventSource = make_shared< Resource >();
   rRegEventSource->set_path("/EnvAPI/event-source");
   rRegEventSource->set_method_handler("GET", RegisterEventSrc);

   auto rCloseSession = make_shared< Resource >();
   rCloseSession->set_path("/EnvAPI/close-session");
   rCloseSession->set_method_handler("POST", CloseSessionHandler);


   //////////////////////
   //auto resource = make_shared< Resource >();
   //resource->set_path("/EnvAPI/stream");
   //resource->set_method_handler("GET", register_event_source_handler);
   //////////////////////////////////////////
     

   // set up a Service that we will publish resources on
   auto settings = make_shared< Settings >();
   settings->set_port(1984);

   Service *pService = new Service;
   pRT->m_pService = pService;

   // publish each resources
   pService->publish(rInitSession);
   pService->publish(rProjects);
   pService->publish(rLoadProject);
   pService->publish(rGetOutputs);
   pService->publish(rGetScenarios);
   pService->publish(rGetPlugins);
   pService->publish(rGetPolicies);
   pService->publish(rSetPolicies);
   pService->publish(rGetAppVars);
   pService->publish(rSetAppVars);
   pService->publish(rGetConfig);
   pService->publish(rSetConfig);
   pService->publish(rRunScenario);
   pService->publish(rGetData);

   ////////////////////////
   //pService->publish(resource);
   //pService->schedule(event_stream_handler, seconds(10));
   ////////////////////////

   pService->publish(rRegEventSource); // test

   // start up the service and wait for requests
   pService->start(settings);

   return; // EXIT_SUCCESS;
   }


void InitSessionHandler(const shared_ptr< Session > pSession)
   {
   string sessionID = format("S_INIT_%i", RestServer::m_nextSessionID++);
   pSession->set_id(sessionID);

   // add an entry for this sessionID in the map of sessions;
   restServer.m_sessionsMap[sessionID] = NULL;

   //gpMainWnd->AddSession(sessionID.c_str());
   
   string data = format( "EnvAPI Initialized|%s", sessionID.c_str() );
   gpMainWnd->Log(data.c_str());
   
   pSession->close(OK, data,
      { { "Content-type", "application/x-www-form-urlencoded"},
        { "Access-Control-Allow-Origin","*" }
      });
   }

void LoadProjectHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain a string with the envx file to load
   const auto request = pSession->get_request();

   int content_length = request->get_header("Content-Length", 0);

   // get the body of the request, which contains the name of the 
   // project file and scenario to run
   pSession->fetch(content_length, [](const shared_ptr< Session > pSession, const Bytes & body)
      {
      CString _body;
      _body.Format("%.*s", (int)body.size(), body.data());

      int pos = 0;
      CString sessionID = _body.Tokenize("|", pos);

      CString envxFile = _body.Tokenize("|", pos);
      nsPath::CPath _envxFile(envxFile);

      CString scn = _body.Tokenize("|\n", pos);
      int _scn = atoi((LPCTSTR)scn);

      // is the content a valid project file?
      if (envxFile.GetLength() > 5 && _envxFile.GetExtension().CompareNoCase("envx") == 0)
         {
         // yes, so launch an Envision Instance with that project
         EnvisionInstance *pInst = gpMainWnd->AddInstance(); // sets the inst ID
         pInst->LoadProject(envxFile);   // launched on a thread, DOESN'T set session
         // NOTE - the above should return quickly since it launches it's own thread
         // Bad if it doesn't - CHECK THIS!!!!

         CString msg;
         msg.Format("LoadProject: loading %s, InstanceID=%i", (LPCTSTR)envxFile, pInst->GetID());
         gpMainWnd->Log(msg);

         string data = format("Loading Envision Project|%i", pInst->GetID());

         pSession->close(OK, data,
            { { "Content-type", "application/x-www-form-urlencoded"},
              { "Access-Control-Allow-Origin","*" }
            });
         }
      });
   }

void GetProjectsHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   //const auto request = pSession->get_request();

   gpMainWnd->Log("GetProjects called");

   string json = readFile(string("/envision/source/EnvServer/projects.json"));

   pSession->close(OK, json,
      { { "Content-type", "application/json"},
        {"Connection","close"},
        { "Access-Control-Allow-Origin","*" } });
   }

void GetOutputVarsHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      EnvModel *pEnvModel = pInst->m_pEnvModel;

      if (pEnvModel != NULL)
         {
         vector<string> labels;
         int varCount = pEnvModel->GetOutputVarLabels(OVT_MODEL|OVT_EVALUATOR|OVT_APPVAR,labels);// return a response without closing the underlying socket connection

         // convert to JSON
         string json("{ \"labels\":[");

         for (int i = 0; i < varCount; i++)
            {
            json += "\"";
            json += labels[i];
            json += "\"";

            if (i < varCount - 1)
               json += ",";
            else
               json += "]}";
            }


         CString msg;
         msg.Format("GetOutputVars: returning %i variables for InstanceID %i, EnvModelID: %I64d", varCount, pInst->GetID(), (INT_PTR) pEnvModel);
         gpMainWnd->Log(msg);

         pSession->close(OK, json,
            { { "Content-type", "application/json"},
              { "Connection","close"},
              { "Access-Control-Allow-Origin","*" } });
         }
      }
   else
      {
      gpMainWnd->Log("GetOutputVars: failed to find model instance!");
      pSession->close(BAD_REQUEST, { {"Connection","close"} });
      }
   }

void GetScenariosHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      EnvModel *pEnvModel = pInst->m_pEnvModel;

      if (pEnvModel != NULL)
         {
         int count = pEnvModel->m_pScenarioManager->GetCount();

         // convert to JSON
         string json("{ \"scenarios\":[");

         for (int i = 0; i < count; i++)
            {
            Scenario *pScenario = pEnvModel->m_pScenarioManager->GetScenario(i);

            json += "{\"name\":\"";
            json += (LPCTSTR)pScenario->m_name;
            json += "\",\"desc\":\"";
            json += (LPCTSTR)pScenario->m_description;
            json += "\"}";


            if (i < count - 1)
               json += ",";
            else
               json += "]}";
            }

         CString msg;
         msg.Format("GetScenario: returning %i scenario for InstanceID %i, EnvModelID: %I64d", count, pInst->GetID(), (INT_PTR)pEnvModel);
         gpMainWnd->Log(msg);

         pSession->close(OK, json,
            { { "Content-type", "application/json"},
              { "Connection","close"},
              { "Access-Control-Allow-Origin","*" } });
         }
      }
   else
      {
      gpMainWnd->Log("GetScenarios: failed to find model instance!");
      pSession->close(BAD_REQUEST, { {"Connection","close"} });
      }
   }

void GetPluginsHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      EnvModel *pEnvModel = pInst->m_pEnvModel;

      if (pEnvModel != NULL)
         {
         // convert to JSON, e.g.
         // { models:[{"name":name, "module":name},...]
         string json("{ \"plugins\":[");

         int apCount = pEnvModel->GetModelProcessCount();
         int evalCount = pEnvModel->GetEvaluatorCount();
         int total = apCount + evalCount;

         int index = 0;

         for (int i = 0; i < apCount; i++)
            {
            EnvModelProcess *pInfo = pEnvModel->GetModelProcessInfo(i);

            nsPath::CPath modulePath(pInfo->m_path);
            CString moduleName( modulePath.GetTitle() );   // e.g. "Target", "SpatialAllocator"
            
            json += "{\"name\":\"";
            json += (LPCTSTR)pInfo->m_name;
            json += "\",\"module\":\"";
            json += (LPCTSTR)moduleName;
            json += "\",\"desc\":\"";
            json += (LPCTSTR)pInfo->m_description;
            json += "\",\"imgURL\":\"";
            json += (LPCTSTR) pInfo->m_imageURL;
            json += "\",\"type\":\"model\" }";

            if (index < total-1)
               json += ",";

            index++;
            }

         for (int i = 0; i < evalCount; i++)
            {
            EnvEvaluator *pInfo = pEnvModel->GetEvaluatorInfo(i);

            nsPath::CPath modulePath(pInfo->m_path);
            CString moduleName(modulePath.GetTitle());   // e.g. "Target", "SpatialAllocator"

            json += "{\"name\":\"";
            json += (LPCTSTR) pInfo->m_name;
            json += "\",\"module\":\"";
            json += (LPCTSTR)moduleName;
            json += "\",\"desc\":\"";
            json += (LPCTSTR)pInfo->m_description;
            json += "\",\"imgURL\":\"";
            json += (LPCTSTR) pInfo->m_imageURL;
            json += "\",\"type\":\"evaluator\" }";

            if (index < total - 1)
               json += ",";

            index++;
            }
         json += "]}";

         CString msg;
         msg.Format("GetPlugins: returning %i models, %i evaluators for InstanceID %i, EnvModelID: %I64d", apCount, evalCount, pInst->GetID(), (INT_PTR)pEnvModel);
         gpMainWnd->Log(msg);

         pSession->close(OK, json,
            { { "Content-type", "application/json"},
              { "Connection","close"},
              { "Access-Control-Allow-Origin","*" } });
         }
      }
   else
      {
      gpMainWnd->Log("GetPlugins: failed to find model instance!");
      pSession->close(BAD_REQUEST, { {"Connection","close"} });
      }
   }

void GetPoliciesHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   string scenarioIndex = request->get_query_parameter("Scenario");
   int _scenarioIndex = atoi(scenarioIndex.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      EnvModel *pEnvModel = pInst->m_pEnvModel;

      if (pEnvModel != NULL)
         {
         // convert to JSON, e.g.
         // { policies:[{"name":name, "module":name},...]
         string json("{ \"policies\":[");

         Scenario *pScenario = pEnvModel->m_pScenarioManager->GetScenario(_scenarioIndex);

         int count = pEnvModel->m_pPolicyManager->GetPolicyCount();

         for (int i = 0; i < count; i++)
            {
            Policy *pPolicy = pEnvModel->m_pPolicyManager->GetPolicy(i);

            POLICY_INFO *pInfo = pScenario->GetPolicyInfoFromID(pPolicy->m_id);

            bool use = pPolicy->m_use;
            if (pInfo != NULL)
               use = pInfo->inUse;

            TCHAR id[16];
            _itoa_s(pPolicy->m_id, id, 16, 10);

            json += "{\"name\":\"";
            json += (LPCTSTR)pPolicy->m_name;
            json += "\",\"id\":\"";
            json += id;
            json += "\",\"desc\":\"";
            json += (LPCTSTR)pPolicy->m_narrative;
            json += "\",\"use\":";
            json += use ? "true" : "false";
            json += "}";

            if (i < count - 1)
               json += ",";
            }
         json += "]}";

         CString msg;
         msg.Format("GetPolicies: returning %i policies for InstanceID %i, EnvModelID: %I64d", count, pInst->GetID(), (INT_PTR)pEnvModel);
         gpMainWnd->Log(msg);

         pSession->close(OK, json,
            { { "Content-type", "application/json"},
              { "Connection","close"},
              { "Access-Control-Allow-Origin","*" } });
         }
      }
   else
      {
      gpMainWnd->Log("GetPolicies: failed to find model instance!");
      pSession->close(BAD_REQUEST, { {"Connection","close"} });
      }
   }

void SetPoliciesHandler(const shared_ptr< Session > pSession)
   {
   // the query parames contain modelID and scenario.  The body of the 
   // has xml with policy info
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      EnvModel *pEnvModel = pInst->m_pEnvModel;

      if (pEnvModel != NULL)
         {
         int content_length = request->get_header("Content-Length", 0);

         // get the body of the request, which contains the xml config string 
         pSession->fetch(content_length, [](const shared_ptr< Session > pSession, const Bytes & body)
            {
            if (body.size() > 5)
               {
               const auto request = pSession->get_request();
               string modelID = request->get_query_parameter("ModelID");
               int _modelID = atoi(modelID.c_str());

               EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);
               EnvModel *pEnvModel = pInst->m_pEnvModel;

               string scenarioIndex = request->get_query_parameter("Scenario");
               int _scenarioIndex = atoi(scenarioIndex.c_str());
               Scenario *pScenario = pEnvModel->m_pScenarioManager->GetScenario(_scenarioIndex);

               if (pScenario != NULL)
                  {
                  TiXmlDocument doc;
                  string _body = String::to_string(body);
                  bool ok = doc.Parse(_body.c_str());
                  if (!ok)
                     {
                     CString msg;
                     msg.Format("Error reading policy");
                     //AfxGetMainWnd()->MessageBox(doc.ErrorDesc(), msg);
                     return;
                     }

                  TiXmlElement *pXmlRoot = doc.RootElement();  // policies
                  TiXmlElement *pXmlPolicy = pXmlRoot->FirstChildElement(_T("policy"));
                  while (pXmlPolicy != NULL)
                     {
                     int id = -1, use = -1;
                     pXmlPolicy->Attribute(_T("id"), &id);
                     pXmlPolicy->Attribute(_T("use"), &use);
                     pXmlPolicy = pXmlPolicy->NextSiblingElement(_T("policy"));

                     POLICY_INFO *pInfo = pScenario->GetPolicyInfoFromID(id);
                     if (pInfo)
                        pInfo->inUse = use ? true : false;
                     else
                        ;
                     }
                  }   // end of: if (pScenario != NULL )
               } // end of: if ( bodys.size() > 5 )

            pSession->close(OK, "Policies updated",
               { { "Content-type", "application/x-www-form-urlencoded"},
                 { "Access-Control-Allow-Origin","*" }
               });

            return;
            });  // end of fetch

         gpMainWnd->Log("SetConfig: failed to find model and/or extension instance!");
         pSession->close(BAD_REQUEST, { { "Content-type", "application/x-www-form-urlencoded"},
                                           { "Access-Control-Allow-Origin","*" }
            });
         }
      }   // end of: pEnvModel != NULL

   return;
   }

void GetAppVarsHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   string scenarioIndex = request->get_query_parameter("Scenario");
   int _scenarioIndex = atoi(scenarioIndex.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      EnvModel *pEnvModel = pInst->m_pEnvModel;

      if (pEnvModel != NULL)
         {
         // is there a corresponding scenario variable?
         Scenario *pScenario = pEnvModel->m_pScenarioManager->GetScenario(_scenarioIndex);

         // convert to JSON, e.g.
         // { policies:[{"name":name, "module":name},...]
         string json("{ \"appvars\":[");

         // get only application-defined appvars
         int count = pEnvModel->GetAppVarCount();
         int appCount = pEnvModel->GetAppVarCount(AVT_APP);
         int ii = 0;
         for (int i = 0; i < count; i++)
            {
            AppVar *pAppVar = pEnvModel->GetAppVar(i);

            string expr = (LPCTSTR) pAppVar->m_expr;
            int use = -1;  // -1 means not defined as scenario var

            if (pAppVar->m_avType == AVT_APP)
               {
               SCENARIO_VAR *pVar = pScenario->GetScenarioVar(pAppVar->m_name);

               if (pVar)
                  {
                  if (pVar->inUse)
                     expr = pVar->defaultValue.GetAsString();

                  use = pVar->inUse ? 1 : 0;
                  }

               TCHAR _use[8];
               _itoa_s(use, _use, 8, 10);

               json += "{\"name\":\"";
               json += (LPCTSTR)pAppVar->m_name;
               json += "\",\"desc\":\"";
               json += (LPCTSTR)pAppVar->m_description;
               json += "\",\"expr\":\"";
               json += (LPCTSTR)pAppVar->m_expr;
               json += "\",\"use\":";
               json += _use;
               json += "}";

               if (ii < appCount - 1)
                  json += ",";

               ii++;
               }
            }

         json += "]}";

         CString msg;
         msg.Format("GetAppVar: returning %i appvars for InstanceID %i, EnvModelID: %I64d", count, pInst->GetID(), (INT_PTR)pEnvModel);
         gpMainWnd->Log(msg);

         pSession->close(OK, json,
            { { "Content-type", "application/json"},
              { "Connection","close"},
              { "Access-Control-Allow-Origin","*" } });
         }
      }
   else
      {
      gpMainWnd->Log("GetAppVars: failed to find model instance!");
      pSession->close(BAD_REQUEST, { {"Connection","close"} });
      }
   }

void SetAppVarsHandler(const shared_ptr< Session > pSession)
   {
   // the query parames contain modelID and scenario.  The body of the 
   // has xml with policy info
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      EnvModel *pEnvModel = pInst->m_pEnvModel;

      if (pEnvModel != NULL)
         {
         int content_length = request->get_header("Content-Length", 0);

         // get the body of the request, which contains the xml config string 
         pSession->fetch(content_length, [](const shared_ptr< Session > pSession, const Bytes & body)
            {
            if (body.size() > 5)
               {
               const auto request = pSession->get_request();
               string modelID = request->get_query_parameter("ModelID");
               int _modelID = atoi(modelID.c_str());

               EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);
               EnvModel *pEnvModel = pInst->m_pEnvModel;

               string scenarioIndex = request->get_query_parameter("Scenario");
               int _scenarioIndex = atoi(scenarioIndex.c_str());
               Scenario *pScenario = pEnvModel->m_pScenarioManager->GetScenario(_scenarioIndex);

               if (pScenario != NULL)
                  {
                  TiXmlDocument doc;
                  string _body = String::to_string(body);
                  bool ok = doc.Parse(_body.c_str());
                  if (!ok)
                     {
                     CString msg;
                     msg.Format("Error reading policy");
                     //AfxGetMainWnd()->MessageBox(doc.ErrorDesc(), msg);
                     return;
                     }

                  TiXmlElement *pXmlRoot = doc.RootElement();  // policies
                  TiXmlElement *pXmlPolicy = pXmlRoot->FirstChildElement(_T("policy"));
                  while (pXmlPolicy != NULL)
                     {
                     int id = -1, use = -1;
                     pXmlPolicy->Attribute(_T("id"), &id);
                     pXmlPolicy->Attribute(_T("use"), &use);
                     pXmlPolicy = pXmlPolicy->NextSiblingElement(_T("policy"));

                     POLICY_INFO *pInfo = pScenario->GetPolicyInfoFromID(id);
                     if (pInfo)
                        pInfo->inUse = use ? true : false;
                     else
                        ;
                     }
                  }   // end of: if (pScenario != NULL )
               } // end of: if ( bodys.size() > 5 )

            pSession->close(OK, "Policies updated",
               { { "Content-type", "application/x-www-form-urlencoded"},
                 { "Access-Control-Allow-Origin","*" }
               });

            return;
            });  // end of fetch

         gpMainWnd->Log("SetConfig: failed to find model and/or extension instance!");
         pSession->close(BAD_REQUEST, { { "Content-type", "application/x-www-form-urlencoded"},
                                           { "Access-Control-Allow-Origin","*" }
            });
         }
      }   // end of: pEnvModel != NULL

   return;
   }



void GetConfigHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID and 
   // name of the plugin whose config is being requested
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      EnvModel *pEnvModel = pInst->m_pEnvModel;
      if (pEnvModel != NULL)
         {
         string pluginName = request->get_query_parameter("Extension");
         if (pluginName.size() > 0)
            {
            // is it a model?
            EnvExtension *pExtension = (EnvExtension*)pEnvModel->FindModelProcessInfo(pluginName.c_str());

            if (pExtension == NULL)
               {
               // or an evaluator?
               pExtension = (EnvExtension*)pEnvModel->FindEvaluatorInfo(pluginName.c_str());
               }

            if (pExtension != NULL)
               {
               std::string configStr;
               pEnvModel->GetConfig(pExtension, configStr);
               
               CString msg( "GetConfig: successful getting configuration file" );
               gpMainWnd->Log(msg);

               pSession->close(OK, configStr.c_str(),
                  { { "Content-type", "application/json"},
                    { "Connection","close"},
                    { "Access-Control-Allow-Origin","*" } });
               }
            }
         }
      }
   else
      {
      gpMainWnd->Log("GetConfig: failed to find model and/or extension instance!");
      pSession->close(BAD_REQUEST, { {"Connection","close"} });
      }
   }

// request should contain an xml-formatted configuration string that is
// then loaded into the given extension instance on the server 
void SetConfigHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID and 
   // name of the plugin whose config is being requested
   const auto request = pSession->get_request();
   int content_length = request->get_header("Content-Length", 0);

   CString msg;
   msg.Format("ContentLength: %i", content_length);
   gpMainWnd->Log(msg);

   // get the body of the request, which contains the xml config string 
   pSession->fetch(content_length, [](const shared_ptr< Session > pSession, const Bytes & body)
      {
      if (body.size() > 5)
         {
         const auto request = pSession->get_request();

         string modelID = request->get_query_parameter("ModelID");
         int _modelID = atoi(modelID.c_str());

         EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

         if (pInst != NULL)
            {
            EnvModel *pEnvModel = pInst->m_pEnvModel;
            if (pEnvModel != NULL)
               {
               string pluginName = request->get_query_parameter("Extension");
               if (pluginName.size() > 0)
                  {
                  // is it a model?
                  EnvExtension *pExtension = (EnvExtension*) pEnvModel->FindModelProcessInfo(pluginName.c_str());

                  if (pExtension == NULL)
                     {
                     // or an evaluator?
                     pExtension = (EnvExtension*)pEnvModel->FindEvaluatorInfo(pluginName.c_str());
                     }

                  if (pExtension != NULL)
                     {
                     CString xmlConfigStr;
                     xmlConfigStr.Format("%.*s", (int)body.size(), body.data());

                     pEnvModel->SetConfig(pExtension, (LPCTSTR)xmlConfigStr);

                     CString msg;
                     msg.Format("SetConfig: InstanceID=%i, Extension=%s", pInst->GetID(), pExtension->m_name);
                     gpMainWnd->Log(msg);

                     string data = format("Configured Plugin %s|%i", pExtension->m_name, pInst->GetID());

                     pSession->close(OK, data.c_str(),
                        { { "Content-type", "application/x-www-form-urlencoded"},
                          { "Access-Control-Allow-Origin","*" }
                        });

                     return;
                     }  // end of: if ( pExtension != NULL )
                  }  // end of: if( pluginName.size() > 0 ) 
               }  // end of: if ( pEnvModel != NULL )
            }  // end of: if ( pInst != NULL )
         }  // end of: if ( bodys.size() > 5 )

      gpMainWnd->Log("SetConfig: failed to find model and/or extension instance!");
      pSession->close(BAD_REQUEST, { { "Content-type", "application/x-www-form-urlencoded"},
                                     { "Access-Control-Allow-Origin","*" }
                                   });
      });  // end of: fetch
   }

void RunScenarioHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   string scenario = request->get_query_parameter("Scenario");
   int _scenario = atoi(scenario.c_str());

   string simulationLength = request->get_query_parameter("Period");
   int _simulationLength = atoi(simulationLength.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);

   if (pInst != NULL)
      {
      pInst->RunScenario(_scenario, _simulationLength );

      CString msg;
      msg.Format("RunScenario: Starting scenario %i on InstanceID %i, EnvModelID: %I64d", _scenario, pInst->GetID(), (INT_PTR) pInst->m_pEnvModel);
      gpMainWnd->Log(msg);

      pSession->close(OK, "Scenario started", 
            { { "Content-type", "application/x-www-form-urlencoded"},
              { "Access-Control-Allow-Origin","*" }
            });
      }
   }

// handle notifications from EnvModel. 
void RestServer::SendServerEvent(EM_NOTIFYTYPE type, INT_PTR param, INT_PTR extra, INT_PTR modelPtr)
   {
   LPCTSTR _type = ::GetNotifyTypeStr(type);
   //const auto message = "data: event " + to_string(type) + "\n\n";
   string msg = format("data: %s \n\n", _type);
   TRACE("type: %s\n", _type);

   switch (type)
      {
      // end of an EnvModel time step - send the data to the corresponding session
      case EMNT_ENDSTEP:
      case EMNT_INITRUN:
      case EMNT_ENDRUN:
      case -1:
         {
         EnvModel *pEnvModel = (EnvModel*)modelPtr;
         const shared_ptr<Session> &pSession = gpMainWnd->GetSessionFromEnvModel(pEnvModel);

         string body;
         switch (type)
            {
            case EMNT_INITRUN:
               body = format("event:init-run\ndata:{\"run\":%i, \"simulationLength\":%i, \"type\":\"start\" }\n\n",
                  (int)param, (int)extra);
               break;

            case EMNT_ENDSTEP:
               body = format("event:end-step\ndata:{\"yearOfRun\":%i, \"simulationLength\":%i }\n\n",
                     (int)param, (int)extra);
               break;

            case EMNT_ENDRUN:
               body = format("event:end-run\ndata:{\"run\":%i, \"simulationLength\":%i }\n\n",
                  (int)param, (int)extra);
               break;

            case -1:
               {
               // Return data as response without closing the underlying socket connection
               LPCTSTR msg = (LPCTSTR)param;
               LPCTSTR type = "";
               switch (extra)
                  {
                  case RT_UNDEFINED: type = ""; break;
                  case RT_INFO:      type = "Info "; break;
                  case RT_WARNING:   type = "Warning: "; break;
                  case RT_ERROR:     type = "Error: "; break;
                  case RT_FATAL:     type = "FATAL: "; break;
                  case RT_SYSTEM:    type = "SYSTEM: "; break;
                  }
               
               CString _msg;
               _msg.Format("%s%s", type, msg);
               
               string body = format("event:log-msg\ndata:\"%s\"\n\n", (LPCTSTR) _msg);
               }
               break;
            }
         
         //string len = format("%i", (int) body.size());
         //string msg = format("data: %s \n\n", _type);

         if (pSession)
            pSession->yield(body);
         }
      }

   //EMNT_IDUCHANGE, EMNT_INIT, EMNT_INITRUN, EMNT_STARTSTEP, EMNT_ENDSTEP, EMNT_RUNAP, EMNT_ACTORLOOP, EMNT_RUNEVAL, EMNT_RUNVIZ, EMNT_ENDRUN, EMNT_UPDATEUI,
   //   EMNT_SETSCENARIO, EMNT_MULTIRUN
   //};
   //switch (type)
   //   {
   //   // end of an EnvModel time step - send the data to the corresponding session
   //   case EMNT_ENDSTEP:
   //   case EMNT_INITRUN:
   //   case EMNT_ENDRUN:
   //      {
   //      EnvModel *pEnvModel = (EnvModel*)modelPtr;
   //      const shared_ptr<Session> &pSession = gpMainWnd->GetSessionFromEnvModel(pEnvModel);
   //
   //      string sid = pSession->get_id();
   //      string sorg = pSession->get_origin();
   //      bool open = pSession->is_open();
   //      
   //      if (open == false)
   //         {
   //         TRACE("SendServerEvent: Session (%s,closed) %s, type: %i, param: %i, extra: %i, modelID: %I64d\n",
   //            sorg.c_str(), sid.c_str(), (int)type, (int)param, (int)extra, modelPtr);
   //         }
   //
   //      if (pSession && pSession->is_open())
   //         {
   //         if (pEnvModel == NULL)
   //            {
   //            TRACE("RestServer: Error loading model %i\n", modelPtr);
   //            return;
   //            }
   //         else
   //            {
   //            //CString msg;
   //            //msg.Format("EnvEvent (EndStep): Step %i/%i completed on EnvModelID: %I64d", (int) param, (int) extra, /*pInst->GetID(),*/ (INT_PTR) pEnvModel);
   //            //gpMainWnd->Log(msg);
   //
   //            // Return data as response without closing the underlying socket connection
   //            string body;
   //            switch (type)
   //               {
   //               case EMNT_ENDSTEP: 
   //                  body = format("event:end-step\ndata:{\"yearOfRun\":%i, \"simulationLength\":%i }\n\n",
   //                        (int)param, (int)extra);
   //                  break;
   //
   //               case EMNT_INITRUN:
   //                  body = format("event:init-run\ndata:{\"run\":%i, \"simulationLength\":%i }\n\n",
   //                     (int)param, (int)extra);
   //                  break;
   //
   //               case EMNT_ENDRUN:
   //                  body = format("event:end-run\ndata:{\"run\":%i, \"simulationLength\":%i }\n\n",
   //                     (int)param, (int)extra);
   //                  break;
   //               }
   //            
   //            string len = format("%i", (int) body.size());
   //            
   //            pSession->yield(OK, body);// ,
   //               //{ { "Cache-Control", "no-cache" },
   //               //  { "Content-Type", "text/event-stream" },
   //               //  { "Content-Length", len.c_str() },
   //               //  { "Connection", "keep-alive" },
   //               //  //{ "Keep-Alive", "timeout=1000, max=1000"},   // timeout-seconds, max=# of addition requests allowed on current connection
   //               //  { "Access-Control-Allow-Origin","*" }
   //               //});
   //            }
   //         }
   //      
   //      break;
   //      }
   //
   //   case -1:  // log message - redirect to client
   //      {
   //      EnvModel *pEnvModel = (EnvModel*)modelPtr;
   //      const shared_ptr<Session> &pSession = gpMainWnd->GetSessionFromEnvModel(pEnvModel);
   //
   //      if (pSession && pSession->is_open())
   //         {
   //         if (pEnvModel == NULL)
   //            {
   //            TRACE("RestServer: Error loading model %i\n", modelPtr);
   //            return;
   //            }
   //         else
   //            {
   //            // Return data as response without closing the underlying socket connection
   //            LPCTSTR msg = (LPCTSTR)param;
   //            LPCTSTR type = "";
   //            switch (extra)
   //               {
   //               case RT_UNDEFINED: type = ""; break;
   //               case RT_INFO:      type = "Info "; break;
   //               case RT_WARNING:   type = "Warning: "; break;
   //               case RT_ERROR:     type = "Error: "; break;
   //               case RT_FATAL:     type = "FATAL: "; break;
   //               case RT_SYSTEM:    type = "SYSTEM: "; break;
   //               }
   //
   //            CString _msg;
   //            _msg.Format("%s%s", type, msg);
   //
   //            string body = format("event:log-msg\ndata:\"%s\"\n\n", (LPCTSTR) _msg);
   //            string len = format("%i", body.size());
   //
   //            pSession->yield(OK, body.c_str(),
   //               { //{ "Content-type", "application/x-www-form-urlencoded" },
   //                 { "Cache-Control", "no-cache" },
   //                 { "Content-Type", "text/event-stream" },
   //                 { "Content-Length", len.c_str() },
   //                 { "Connection", "keep-alive" },
   //                 //{ "Keep-Alive", "timeout=1000, max=1000"},   // timeout-seconds, max=# of addition requests allowed on current connection
   //                 { "Access-Control-Allow-Origin","*" }
   //               }); 
   //            }
   //         }
   //
   //      break;
   //      }
   //   }
   }


// format: ?ModelID=mID&Start=start&End=end&Fields=f0|f1|...|fend

void GetDataHandler(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());

   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);
  
   if (pInst == NULL)
      {
      CString msg;
      msg.Format("GetData: Unknown Model ID (%s)", modelID);
      gpMainWnd->Log(msg);

      pSession->close(BAD_REQUEST, "GetData: Unknown Model ID",
         { { "Content-type", "application/x-www-form-urlencoded"},
           { "Access-Control-Allow-Origin","*" } });
      
      return;
      }

   // have valid model ID, query it for data
   string start = request->get_query_parameter("Start");
   int _start = atoi(start.c_str());
   string end = request->get_query_parameter("End");
   int _end = atoi(end.c_str());

   string fields = "All";
   CStringArray _fields;   // contains parsed list of fields to collect data for
   int fieldCount = -1;

   EnvModel *pEnvModel = pInst->m_pEnvModel;
   ASSERT(pEnvModel != NULL);

   if (request->has_query_parameter("Fields"))
      {
      fields = request->get_query_parameter("Fields");
      fieldCount = ::Tokenize(fields.c_str(), "|", _fields);  // get list of fields
      }
   else
      {
      vector<string> labels;   // Note: excludes DataObj-type output vars
      fieldCount = pEnvModel->GetOutputVarLabels(OVT_MODEL|OVT_EVALUATOR|OVT_APPVAR, labels);  //7=get all (aps, evals, appvars)
      for (int i = 0; i < fieldCount; i++)
         _fields.Add(labels[i].c_str());
      }

   // at this point, _fields contains an array of field name strings
   // create a dataobj with cols=fields, rows= requested time interval
   int rows = _end - _start + 1;
   FDataObj dataObj(fieldCount, rows);  // cols, rows

   if (fieldCount > 0)
      {
      for (int i = 0; i < fieldCount; i++)
         {
         // field will be "model name.fieldname"
         DataObj *pDataObj = pEnvModel->m_pDataManager->GetDataObj(DT_MODEL_OUTPUTS, -1);
         ASSERT(pDataObj != NULL);

         int doFieldCount = pDataObj->GetColCount();
         ASSERT(fieldCount <= doFieldCount);

         int col = pDataObj->GetCol(_fields[i]);
         ASSERT(col >= 0);

         CArray<float, float> data;
         pEnvModel->m_pDataManager->SubsetData(DT_MODEL_OUTPUTS, /*run*/ -1, col, _start, _end, data);

         dataObj.SetLabel(i, _fields[i]);
         for (int j = 0; j < rows; j++)
            dataObj.Set(i, j, data[j]);
         }
      }

   // have requested data, return it to the client as JSON
   // { { "f0 label": [d0,d1,d2] },
   //             ...
   //   { "fn label": [d0,d1,d2] } }
   
   // convert to JSON
   string json("{ \"data\":[");
   
   for (int i = 0; i < fieldCount; i++)
      {
      //json += "{ \"";
      //json += (LPCTSTR)_fields[i];
      //json += "\":[";
      json += "[";
      for (int j = 0; j < rows; j++)
         {
         string value = format("%f", dataObj.GetAsFloat(i, j));
         json += value;

         if (j != rows - 1)
            json += ",";
         }

      //json += "]}";
      json += "]";

      if (i != fieldCount - 1)
         json += ",";
      else
         json += "]}";
      }


   CString msg;
   msg.Format("GetData: Fields %s for InstanceID %i, EnvModelID: %I64d", fields.c_str(), pInst->GetID(), (INT_PTR)pInst->m_pEnvModel);
   gpMainWnd->Log(msg);

   pSession->close(OK, json,
          { { "Content-type", "application/json"},
            { "Access-Control-Allow-Origin","*" } });
   }

     
void CloseSessionHandler(const shared_ptr< Session > pSession)
   {
   string sessionID = pSession->get_id();

   // add an entry for this sessionID in the map of sessions;
   restServer.m_sessionsMap[sessionID] = NULL;

   gpMainWnd->RemoveSession(sessionID.c_str());

   CString msg;
   msg.Format("Close Session: Session ID=%s", sessionID.c_str());
   gpMainWnd->Log(msg);

   string data = format("EnvAPI Session %s closed", sessionID.c_str());
   pSession->close(OK, data,
      { { "Content-type", "application/x-www-form-urlencoded"},
        { "Access-Control-Allow-Origin","*" }
      });
   }


// open a keep-alive connection to the client via 'yield'
// this gets invoked when the client sets a javacript
// EventSource to http:.../EnvAPI/event-source
void RegisterEventSrc(const shared_ptr< Session > pSession)
   {
   // the body of the request should contain the model ID
   const auto request = pSession->get_request();
   string modelID = request->get_query_parameter("ModelID");
   int _modelID = atoi(modelID.c_str());
   
   // Note: EnvisionInstances get created in LoadProjectHandler, so
   // EventSources must be created after a project is loaded
   EnvisionInstance *pInst = gpMainWnd->GetEnvisionInstanceFromModelID(_modelID);
   
   if (pInst != NULL)
      {
      pInst->m_pEventSession = pSession;   // remember this session
      CString msg;
      msg.Format("RegisterEventSrc: InstanceID %i, EnvModel %I64d (%s)", pInst->GetID(), (INT_PTR)pInst->m_pEnvModel, modelID.c_str());
      gpMainWnd->Log(msg);
           
      //gpSession = pSession;

      const auto headers = multimap< string, string >{
         { "Connection", "keep-alive" },
         { "Cache-Control", "no-cache" },
         { "Content-Type", "text/event-stream" },
         { "Access-Control-Allow-Origin", "*" } //Only required for demo purposes.
         };

      pSession->yield(OK, headers, [](const shared_ptr< Session > session)
         {
         session->yield("event:message\ndata:\"starting\"\n\n");
         });
      }
  }


/*

void GetData(const shared_ptr< Session > pSession, EnvModel *pModel, FDataObj &data)
   {
   // DM_TYPES
   //   DT_EVAL_SCORES = 0,     // landscape goal scores trajectories
   //   DT_EVAL_RAWSCORES,      // landscape goal scores trajectories (raw scores)
   //   DT_MODEL_OUTPUTS,       // output variables from eval and autonous process models, + App vars
   //   DT_ACTOR_WTS,           // actor group data
   //   DT_ACTOR_COUNTS,        // actor counts by actor group
   //   DT_POLICY_SUMMARY,      // policy summary information (only initial areas)
   //   DT_GLOBAL_CONSTRAINTS,  // Global Constraints summary outputs
   //   DT_POLICY_STATS,        // info on policy stats (intrumentation)
   //   DT_SOCIAL_NETWORK,      // info on social network outpus
   //   DT_LAST

   // the body of the request should contain thq data type, column, start, and end values
   const auto request = pSession->get_request();

   int content_length = request->get_header("Content-Length", 0);

   // get the body of the request, which contains the nae of the 
   // project file and scenario to run
   pSession->fetch(content_length, [](const shared_ptr< Session > pSession, const Bytes & body)
      {
      CString _body;
      _body.Format("%.*s", (int)body.size(), body.data());

      int pos = 0;
      CString type = _body.Tokenize(" ", pos);
      CString field = _body.Tokenize(" ", pos);
      CString start = _body.Tokenize(" ", pos);
      CString end = _body.Tokenize(" \n", pos);

      int _type = atoi(type);
      int _start = atoi(start);
      int _end = atoi(end);

      EnvModel *pEnvModel = gpMainWnd->GetEnvModelFromSession(pSession);
      if (pEnvModel == NULL)
         {
         TRACE("RestServer: Error loading model during 'GetDataHandler()' \n");
         }
      else
         {
         int col = pEnvModel->m_pIDULayer->GetFieldCol(field);

         if (col >= 0)
            {
            CArray<float, float> data;
            bool ok = pEnvModel->m_pDataManager->SubsetData((DM_TYPE)_type, -1, col, _start, _end, data);

            if (ok)
               {
               INT_PTR nBytes = data.GetSize() * sizeof(float);

               //Byte is typedef for uint8_t (eight-bit)
               Bytes &bytes = (Bytes&)(*(data.GetData()));

               // Return data as response without closing the underlying socket connection
               CString sBytes;
               sBytes.Format("%I64d", nBytes);

               pSession->yield(OK, bytes,
                  { { "Content-type", "application/x-www-form-urlencoded" },
                    { "Content-Length", (LPCTSTR)sBytes },
                    { "Connection", "keep-alive" },
                    //{ "Keep-Alive", "timeout=20, max=50"},
                    { "Access-Control-Allow-Origin","*" }
                  });  // ,
                  //[](const shared_ptr< Session > session)
                  //{
                  ////sessions.push_back(session);
                  //});
               }
            }
         }

      //CString msg;
      //msg.Format("data: %i %i %i %i\n\n", (int)type, (int)param, (int)extra, (int)modelID);
      //
      //// send the message to any open sessions
      //for (auto session : restServer.m_sessions)
      //   {
      //   TRACE("Session %s sending a message to is being closed\n", session->get_id().c_str(), session->get_origin().c_str());
      //
      //   session->yield((LPCTSTR) (msg ));
      //   }
      });  // end of: session.fetch()
   }
   */

string readFile(const string &fileName)
   {
   ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

   ifstream::pos_type fileSize = ifs.tellg();
   ifs.seekg(0, ios::beg);

   vector<char> bytes(fileSize);
   ifs.read(bytes.data(), fileSize);

   return string(bytes.data(), fileSize);
   }

string format(const char *fmt, ...)
   {
   char buf[256];

   va_list args;
   va_start(args, fmt);
   const auto r = std::vsnprintf(buf, sizeof buf, fmt, args);
   va_end(args);

   if (r < 0)
      // conversion failed
      return {};

   const size_t len = r;
   if (len < sizeof buf)
      // we fit in the buffer
      return { buf, len };

#if __cplusplus >= 201703L
   // C++17: Create a string and write to its underlying array
   std::string s(len, '\0');
   va_start(args, fmt);
   std::vsnprintf(s.data(), len + 1, fmt, args);
   va_end(args);

   return s;
#else
   // C++11 or C++14: We need to allocate scratch memory
   auto vbuf = std::unique_ptr<char[]>(new char[len + 1]);
   va_start(args, fmt);
   std::vsnprintf(vbuf.get(), len + 1, fmt, args);
   va_end(args);

   return { vbuf.get(), len };
#endif
   }