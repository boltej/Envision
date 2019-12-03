// dllmain.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>

#include <flow.h>
//#include "Flow_Grid.h"
#include "Flow_Solute.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE Flow_SoluteDLL = { NULL, NULL };

Flow_Solute *theModel = NULL;

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("Flow_Solute.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(Flow_SoluteDLL, hInstance))
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

		new CDynLinkLibrary(Flow_SoluteDLL);


      ASSERT( theModel == NULL );
      theModel = new Flow_Solute;
      ASSERT( theModel != NULL );

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("Flow_Solute.DLL Terminating!\n");

		// Terminate the library before destructors are called
		AfxTermExtensionModule(Flow_SoluteDLL);
	}
	return 1;   // ok
}

extern "C" float PASCAL EXPORT Init_Solute_Fluxes( FlowContext *pFlowContext );
extern "C" float PASCAL EXPORT Solute_Fluxes( FlowContext *pFlowContext );
extern "C" float PASCAL EXPORT Solute_Fluxes_SRS(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT Solute_Fluxes_HJA(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT Solute_Fluxes_SRS_Managed(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT HBV_N_Fluxes(FlowContext *pFlowContext);
//extern "C" float PASCAL EXPORT Tracer_Application( FlowContext *pFlowContext );

float PASCAL Init_Solute_Fluxes( FlowContext *pFlowContext ) { return theModel->Init_Solute_Fluxes( pFlowContext ); }
float PASCAL Solute_Fluxes( FlowContext *pFlowContext ) { return theModel->Solute_Fluxes( pFlowContext ); }
float PASCAL Solute_Fluxes_SRS(FlowContext *pFlowContext) { return theModel->Solute_Fluxes_SRS(pFlowContext); }
float PASCAL Solute_Fluxes_HJA(FlowContext *pFlowContext) { return theModel->Solute_Fluxes_HJA(pFlowContext); }
float PASCAL Solute_Fluxes_SRS_Managed(FlowContext *pFlowContext) { return theModel->Solute_Fluxes_SRS_Managed(pFlowContext); }
float PASCAL HBV_N_Fluxes(FlowContext *pFlowContext) { return theModel->HBV_N_Fluxes(pFlowContext); }
//float PASCAL Tracer_Application( FlowContext *pFlowContext ) { return theModel->Tracer_Application( pFlowContext ); }