// dllmain.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>

#include <flow.h>
#include "Flow_Grid.h"
//#include <EnvEngine\EnvContext.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE Flow_GridDLL = { NULL, NULL };

Flow_Grid *theModel = NULL;


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("Flow_Grid.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(Flow_GridDLL, hInstance))
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

		new CDynLinkLibrary(Flow_GridDLL);


      ASSERT( theModel == NULL );
      theModel = new Flow_Grid;
      ASSERT( theModel != NULL );

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("Flow_Grid.DLL Terminating!\n");

		// Terminate the library before destructors are called
		AfxTermExtensionModule(Flow_GridDLL);
	}
	return 1;   // ok
}

extern "C" float PASCAL EXPORT Grid_Fluxes(FlowContext *pFlowContext); 
extern "C" float PASCAL EXPORT SRS_5_Layer(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT SRS_6_Layer(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT HJA_6_Layer(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT UpdateTreeGrowth(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT UpdateHBVTreeGrowth(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT UpdateSmallWatershedTreeGrowth(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT Init_Grid_Fluxes( FlowContext *pFlowContext );
extern "C" float PASCAL EXPORT Init_FIRE_Grid_Fluxes(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT Solute_Fluxes( FlowContext *pFlowContext );
extern "C" float PASCAL EXPORT PrecipFluxHandler( FlowContext *pFlowContext );
extern "C" float PASCAL EXPORT Tracer_Application( FlowContext *pFlowContext );
float PASCAL Grid_Fluxes( FlowContext *pFlowContext ) { return theModel->Grid_Fluxes( pFlowContext ); }
float PASCAL SRS_5_Layer(FlowContext *pFlowContext) { return theModel->SRS_5_Layer(pFlowContext); }
float PASCAL SRS_6_Layer(FlowContext *pFlowContext) { return theModel->SRS_6_Layer(pFlowContext); }
float PASCAL HJA_6_Layer(FlowContext *pFlowContext) { return theModel->HJA_6_Layer(pFlowContext); }
float PASCAL UpdateTreeGrowth(FlowContext *pFlowContext) { return theModel->UpdateTreeGrowth(pFlowContext); }
float PASCAL UpdateHBVTreeGrowth(FlowContext *pFlowContext) { return theModel->UpdateHBVTreeGrowth(pFlowContext); }
float PASCAL UpdateSmallWatershedTreeGrowth(FlowContext *pFlowContext) { return theModel->UpdateSmallWatershedTreeGrowth(pFlowContext); }
float PASCAL Init_Grid_Fluxes( FlowContext *pFlowContext ) { return theModel->Init_Grid_Fluxes( pFlowContext ); }
float PASCAL Init_FIRE_Grid_Fluxes(FlowContext *pFlowContext) { return theModel->Init_FIRE_Grid_Fluxes(pFlowContext); }
float PASCAL Solute_Fluxes( FlowContext *pFlowContext ) { return theModel->Solute_Fluxes( pFlowContext ); }
float PASCAL PrecipFluxHandler( FlowContext *pFlowContext ) { return theModel->PrecipFluxHandler( pFlowContext ); }
float PASCAL Tracer_Application( FlowContext *pFlowContext ) { return theModel->Tracer_Application( pFlowContext ); }