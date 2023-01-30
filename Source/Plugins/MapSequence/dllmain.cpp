
// dllmain.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>

#include <EnvEngine\EnvContext.h>

#include "MapSequence.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


MapSequenceProcess  *theProcess = NULL;

static AFX_EXTENSION_MODULE MapSequenceDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
      /**** TODO: update trace string with module name ****/
		TRACE0("MapSequence.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
      /**** TODO: update AfxInitExtensionModule with module name ****/
		if (!AfxInitExtensionModule(MapSequenceDLL, hInstance))
			return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

      /**** TODO: update CDynLinkLibrary constructor with module name ****/
		new CDynLinkLibrary(MapSequenceDLL);
     
      ASSERT( theProcess == NULL );
      theProcess = new MapSequenceProcess;
      ASSERT( theProcess != NULL );
	}
    
	else if (dwReason == DLL_PROCESS_DETACH)
	{
      /**** TODO: Update module name in trace string ****/
		TRACE0("MapSequence.DLL Terminating!\n");

      if ( theProcess != NULL)
         delete theProcess;

		// Terminate the library before destructors are called
      /**** TODO: Update module name in AfxTermExtensionModule ****/
		AfxTermExtensionModule(MapSequenceDLL);
	}
	return 1;   // ok
}

extern "C" void PASCAL EXPORT GetExtInfo( ENV_EXT_INFO *pInfo );


/**** TODO: Comment out any unneeded interfaces ****/
/**** TODO: NOTE: For all interfaces used, make sure there is an entry in the .def file!!! ****/


// for autonomous processes
extern "C" BOOL PASCAL EXPORT APInit( EnvContext *pEnvContext, LPCTSTR initStr );
extern "C" BOOL PASCAL EXPORT APInitRun( EnvContext *pEnvContext, bool useInitialSeed );
extern "C" BOOL PASCAL EXPORT APRun( EnvContext *pEnvContext );
extern "C" BOOL PASCAL EXPORT APEndRun( EnvContext *pEnvContext );
extern "C" BOOL PASCAL EXPORT APSetup( EnvContext *pContext, HWND hWnd );
extern "C" int  PASCAL EXPORT APInputVar( int modelID, MODEL_VAR** modelVar );
extern "C" int  PASCAL EXPORT APOutputVar( int modelID, MODEL_VAR** modelVar );


/////////////////////////////////////////////////////////////////////////////////////
///////////               E X T E N S I O N     I N F O                 /////////////
/////////////////////////////////////////////////////////////////////////////////////

void PASCAL GetExtInfo( ENV_EXT_INFO *pInfo ) 
   { 
   // TODO:  update as needed
   pInfo->types = EET_AUTOPROCESS;
   pInfo->description = "Map Sequence (Autonomous Process)";
   }


/////////////////////////////////////////////////////////////////////////////////////
///////////               A U T O N O M O U S    P R O C E S S          /////////////
/////////////////////////////////////////////////////////////////////////////////////

BOOL PASCAL APInit( EnvContext *pEnvContext, LPCTSTR initStr )    { return theProcess->Init( pEnvContext, initStr ); }
BOOL PASCAL APInitRun( EnvContext *pEnvContext, bool useInitSeed ){ return theProcess->InitRun( pEnvContext, useInitSeed ); }
BOOL PASCAL APRun( EnvContext *pEnvContext )                      { return theProcess->Run( pEnvContext );  }
BOOL PASCAL APEndRun( EnvContext *pEnvContext )                   { return theProcess->EndRun( pEnvContext );  }
BOOL PASCAL APSetup( EnvContext *pContext, HWND hWnd )            { return theProcess->Setup( pContext, hWnd ); }
int  PASCAL APInputVar( int id, MODEL_VAR** modelVar )            { return theProcess->InputVar( id, modelVar ); }
int  PASCAL APOutputVar( int id, MODEL_VAR** modelVar )           { return theProcess->OutputVar( id, modelVar ); }
