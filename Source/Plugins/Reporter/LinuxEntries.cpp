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
// LinuxEntries.cpp : Add functions normally defined in dllmain.cpp
//

#include <EnvEngine/EnvContext.h>

#include "Reporter.h"

#include<memory>
//use a shared pointer here to ensure that memory is 
//properly released when we move out of scope.
std::shared_ptr<Reporter> spCleanup(new Reporter);
Reporter *theReporter=spCleanup.get();

extern "C" void GetExtInfo( ENV_EXT_INFO *pInfo );

/**** TODO: Comment out any unneeded interfaces ****/
// for eval model
//extern "C" bool EMInit( EnvContext *pContext, LPCTSTR initStr );
//extern "C" bool EMInitRun( EnvContext *pContext, bool useInitialSeed );
//extern "C" bool EMRun( EnvContext *pContext );
//extern "C" bool EMEndRun( EnvContext *pContext );
//extern "C" bool EMSetup( EnvContext *pContext, HWND hWnd );
//extern "C" int  EMInputVar( int modelID, MODEL_VAR** modelVar );
//extern "C" int  EMOutputVar( int modelID, MODEL_VAR** modelVar );

// for autonomous processes
extern "C" bool APInit( EnvContext *pEnvContext, LPCTSTR initStr );
extern "C" bool APInitRun( EnvContext *pEnvContext, bool useInitialSeed );
extern "C" bool APRun( EnvContext *pEnvContext );
extern "C" bool APEndRun( EnvContext *pEnvContext );
extern "C" bool APSetup( EnvContext *pContext, HWND hWnd );
extern "C" int  APInputVar( int modelID, MODEL_VAR** modelVar );
extern "C" int  APOutputVar( int modelID, MODEL_VAR** modelVar );

// for visualizers
//extern "C" bool VInit        ( EnvContext*, LPCTSTR initInfo ); 
//extern "C" bool VInitRun     ( EnvContext*, bool ); 
//extern "C" bool VRun         ( EnvContext* );
//extern "C" bool VEndRun      ( EnvContext* );
//extern "C" bool VInitWindow  ( EnvContext*, HWND );
//extern "C" bool VUpdateWindow( EnvContext*, HWND );
//extern "C" bool VSetup       ( EnvContext*, HWND );

/////////////////////////////////////////////////////////////////////////////////////
///////////               E X T E N S I O N     I N F O                 /////////////
/////////////////////////////////////////////////////////////////////////////////////

void GetExtInfo( ENV_EXT_INFO *pInfo ) 
   { 
   // TODO:  update as needed
   pInfo->types = EET_AUTOPROCESS;
   pInfo->description = "Spatial Alloction Model";
   }


/////////////////////////////////////////////////////////////////////////////////////
///////////               A U T O N O M O U S    P R O C E S S          /////////////
/////////////////////////////////////////////////////////////////////////////////////

bool APInit( EnvContext *pEnvContext, LPCTSTR initStr )    { return theReporter->Init( pEnvContext, initStr ); }
bool APInitRun( EnvContext *pEnvContext, bool useInitSeed ){ return theReporter->InitRun( pEnvContext, useInitSeed ); }
bool APRun( EnvContext *pEnvContext )                      { return theReporter->Run( pEnvContext );  }
bool APEndRun( EnvContext *pEnvContext )                   { return theReporter->EndRun( pEnvContext );  }
bool APSetup( EnvContext *pContext, HWND hWnd )            { return theReporter->Setup( pContext, hWnd ); }
int  APInputVar( int id, MODEL_VAR** modelVar )            { return theReporter->InputVar( id, modelVar ); }
int  APOutputVar( int id, MODEL_VAR** modelVar )           { return theReporter->OutputVar( id, modelVar ); }


/////////////////////////////////////////////////////////////////////////////////////
//////////////      E V A L     M O D E L            ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

//bool EMInit( EnvContext *pEnvContext, LPCTSTR initStr )    { return theModel->Init( pEnvContext, initStr ); }
//bool EMInitRun( EnvContext *pEnvContext, bool useInitSeed ){ return theModel->InitRun( pEnvContext, useInitSeed ); }
//bool EMRun( EnvContext *pEnvContext )                      { return theModel->Run( pEnvContext );  }
//bool EMEndRun( EnvContext *pEnvContext )                   { return theModel->EndRun( pEnvContext );  }
//bool EMSetup( EnvContext *pContext, HWND hWnd )            { return theModel->Setup( pContext, hWnd ); }
//int  EMInputVar( int id, MODEL_VAR** modelVar )            { return theModel->InputVar( id, modelVar ); }
//int  EMOutputVar( int id, MODEL_VAR** modelVar )           { return theModel->OutputVar( id, modelVar ); }


/////////////////////////////////////////////////////////////////////////////////////
///////////                        V I S U A L I Z E R                  /////////////
/////////////////////////////////////////////////////////////////////////////////////

//bool VInit        ( EnvContext *pEnvContext, LPCTSTR initInfo )    { return theViz->Init( pEnvContext, initInfo ); }
//bool VInitRun     ( EnvContext *pEnvContext, bool useInitialSeed ) { return theViz->InitRun( pEnvContext, useInitialSeed ); }
//bool VRun         ( EnvContext *pEnvContext )                      { return theViz->Run( pEnvContext );  }
//bool VEndRun      ( EnvContext *pEnvContext )                      { return theViz->EndRun( pEnvContext );  }
//bool VInitWindow  ( EnvContext *pEnvContext, HWND hWnd )           { return theViz->InitWindow( pEnvContext, hWnd );  }
//bool VUpdateWindow( EnvContext *pEnvContext, HWND hWnd )           { return theViz->UpdateWindow( pEnvContext, hWnd );  }
//bool VSetup       ( EnvContext *pEnvContext, HWND hWnd )           { return theViz->Setup( pEnvContext, hWnd );  }

/**** TODO:  Update *.def file exports to match what is exposed here ****/
