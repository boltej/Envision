// dllmain.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>

#include <..\Plugins\Flow\flow.h>
#include "LPJGuess.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE LPJDLL = { NULL, NULL };

LPJGuess *theModel = NULL;


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("LPJGuess.DLL Initializing!\n");

		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(LPJDLL, hInstance))
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

		new CDynLinkLibrary(LPJDLL);

		/*** TODO:  instantiate any models/processes ***/
		ASSERT(theModel == NULL);
		theModel = new LPJGuess;
		ASSERT(theModel != NULL);

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("LPJGuess.DLL Terminating!\n");

		if (theModel != NULL)
			delete theModel;

		// Terminate the library before destructors are called
		AfxTermExtensionModule(LPJDLL);
	}
	return 1;   // ok
}

/////////////////////////////////////////////////////////////////////////////////////////////
// DECLARATIONS
/////////////////////////////////////////////////////////////////////////////////////////////


// Models
extern "C" float PASCAL EXPORT Framework(FlowContext *pFlowContext);


/////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////////////////////////////


// Model implementation
float PASCAL Framework(FlowContext *pFlowContext) { return theModel->Framework(pFlowContext); }




