#pragma once

#include <map>
#include <PtrArray.h>
#include <EnvModel.h>


#include <restbed>

using namespace std;
using namespace restbed;


/*
==========================================================
RestServer manages a collection of REST services
that allow a client to interact with this envision server
to load projects, runs scenarios, etc.

Basic Idea:

1) Client GET:projects   -> returns list of project available on server

2) Client POST:init-session -> sets sessionID, AddSession() to mainwnd,
                               returns sessionID to client. closes session

3) Client POST:load-project  -> Server loads the project into new EnvModel 
                                instance synchronously.
                                Adds an new EnvisionInstance to mainwnd
                                returns Model ID (e.g. 0,1,...)
                                closes session

4) Client GET:output-vars    -> requires ModelID query parameter in request
                                returns list of model output variable labels

5) Client GET:event-source  -> requires ModelID query parameter,
                               finds the associated EnvisionInstance, set's
                               the EnvisionInstance's session ptr to this one,
                               makes the session keep-alive
                               This allows the RestServer to notify the client 
                               whenever the server recieves an EnvModel event.
                               
6) Client POST:run-scenario  -> takes a ModelID and Scenario(0-based) to run
                                Launches a new thread and runs a scenario
                                closes session

7) Client POST:close-session  -> takes a sessionID, closes it, deleting
                                 associated resources

*/

enum STATUS { ST_INITIALIZED, ST_PROJECTLOADED, ST_RUNNING, ST_PAUSED, ST_STOPPED };

//--------------------------------------------------------------------
// An EnvisionInstance represents one instance of an Envision Model.
// It supports functionality for a) loading, and b) running an Envision
// model, managing the interface between the REST services and the
// Envision API.
// When loading and running an Envision model, each function is run 
// on its own thread.
//--------------------------------------------------------------------

class EnvisionInstance
   {
   friend class CEnvServerDlg;

   public:
      CString m_projectFile;  // envx associated with this instance
      EnvModel *m_pEnvModel;  // EnvEngine instance ptr (NULL until model loaded)
      int m_scenario;         // zero-based (-1=run all)
      int m_simulationLength;

      time_t m_start;   // start time;
      shared_ptr<Session> m_pEventSession;  // EventSource session

   protected:
      int m_id;   // unique identifier (incrementing m_count)
      std::thread *m_pThread;  //  currently execting thread
      STATUS m_status;
      time_t m_stop;
      double m_duration;   // seconds

      static int m_instanceCount;

      // API Entry points
   public:
      int LoadProject(LPCTSTR envxFile);  // Runs ::EnvLoadProject
      int RunScenario(int scenario, int simulationLength);      // Runs ::EnvRunScenario()

   // Thread functions.
   protected:
      int _RunScenarioThreadFn();
      int _LoadProjectThreadFn();

   public:
      EnvisionInstance();
      ~EnvisionInstance();

      int GetID() { return m_id; }

      void Join();
   };

class RestThread
   {
   public:
      RestThread() : m_pThread(NULL), m_pService(NULL ) {}

      ~RestThread() { }
      thread *m_pThread;          // RestServer manages memory
      Service *m_pService;        // RestServer manages memory
   };


class RestSession
   {
   public:
      int m_sessionID;
      std::string m_clientUrl;  
      shared_ptr<Session> m_pEventSession;  // EventSource session

      EnvisionInstance *m_pEnvisionInstance;  // memory managed elsewhere
   };


class RestServer
   {
   public:
      RestServer() {}
      ~RestServer() { Close(); }

      int StartThread();

      void SendServerEvent(EM_NOTIFYTYPE type, INT_PTR param, INT_PTR extra, INT_PTR modelID);

      int Close();  // shuts down, clean up all server threads
      int InitSession(string &origin)
         {
         int sessionID = m_nextSessionID++;

         RestSession *pRestSession = new RestSession;
         pRestSession->m_sessionID = sessionID;
         pRestSession->m_clientUrl = origin;
         pRestSession->m_pEventSession = nullptr;
         pRestSession->m_pEnvisionInstance = nullptr;

         // add an entry for this sessionID in the map of sessions;
         m_sessionsMap[sessionID] = pRestSession;
         return sessionID;
         }

      void EndSession(int sessionID);   // removes session freom session list

      EnvisionInstance *AddModelInstance();

      EnvModel *GetEnvModelFromSession(const shared_ptr<Session> &);
      const shared_ptr<Session> & GetSessionFromEnvModel(EnvModel*);
      EnvisionInstance *GetEnvisionInstanceFromModelID(int modelID);

      static int m_nextSessionID;
           
   public:
      PtrArray< RestThread > m_threads;

      // track active sessions, based on sessionID given client when init-session called
      // key = sessionID, value = RestSession
      map< int, RestSession*> m_sessionsMap;

      PtrArray<EnvisionInstance> m_instArray;

      CStringArray m_datasetsArray;

   };

