// dllmain.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>

#include <Flow\flow.h>
#include "ModflowAdapter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE ModflowAdapterDLL = { NULL, NULL };

ModflowAdapter *theModel = NULL;


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("ModflowAdapter.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(ModflowAdapterDLL, hInstance))
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

		new CDynLinkLibrary(ModflowAdapterDLL);

            /*** TODO:  instantiate any models/processes ***/
      ASSERT( theModel == NULL );
      theModel = new ModflowAdapter;
      ASSERT( theModel != NULL );

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("ModflowAdapter.DLL Terminating!\n");

         if ( theModel != NULL)
            delete theModel;

		// Terminate the library before destructors are called
		AfxTermExtensionModule(ModflowAdapterDLL);
	}
	return 1;   // ok
}

extern "C" BOOL PASCAL EXPORT Init( FlowContext *pFlowContext, LPCTSTR initStr );
extern "C" BOOL PASCAL EXPORT InitRun( FlowContext *pFlowContext );
extern "C" BOOL PASCAL EXPORT Run( FlowContext *pFlowContext );
extern "C" BOOL PASCAL EXPORT EndRun( FlowContext *pFlowContext );


BOOL PASCAL Init( FlowContext *pFlowContext, LPCTSTR initStr ) { return theModel->Init( pFlowContext, initStr ); }
BOOL PASCAL InitRun( FlowContext *pFlowContext ) { return theModel->InitRun( pFlowContext ); }
BOOL PASCAL Run( FlowContext *pFlowContext ) { return theModel->Run( pFlowContext ); }
BOOL PASCAL EndRun( FlowContext *pFlowContext ) { return theModel->EndRun( pFlowContext ); }



