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


class RestThread
   {
   public:
      RestThread() : m_pThread(NULL), m_pService(NULL ) {}

      ~RestThread() { }
      thread *m_pThread;          // RestServer manages memory
      Service *m_pService;        // RestServer manages memory
   };


class RestServer
   {
   public:
      RestServer() {}
      ~RestServer() { Close(); }

      int StartThread();

      void SendServerEvent(EM_NOTIFYTYPE type, INT_PTR param, INT_PTR extra, INT_PTR modelID);

      int Close();  // shuts down, clean up all server threads

      static int m_nextSessionID;

   public:
      PtrArray< RestThread > m_threads;
      // key = sessionID, value = Session ptr
      //map< string, shared_ptr< Session > > m_sessionsMap;
      map< string, EnvModel*> m_sessionsMap;
   };

