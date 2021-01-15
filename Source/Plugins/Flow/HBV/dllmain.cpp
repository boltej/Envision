// dllmain.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxwin.h>
#include <afxdllx.h>

#include <flow.h>
#include "HBV.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE HBVDLL = { NULL, NULL };

HBV *theModel = NULL;


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("HBV.DLL Initializing!\n");

		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(HBVDLL, hInstance))
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

		new CDynLinkLibrary(HBVDLL);

		/*** TODO:  instantiate any models/processes ***/
		ASSERT(theModel == NULL);
		theModel = new HBV;
		ASSERT(theModel != NULL);

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("HBV.DLL Terminating!\n");

		if (theModel != NULL)
			delete theModel;

		// Terminate the library before destructors are called
		AfxTermExtensionModule(HBVDLL);
	}
	return 1;   // ok
}

/////////////////////////////////////////////////////////////////////////////////////////////
// DECLARATIONS
/////////////////////////////////////////////////////////////////////////////////////////////


// Models
extern "C" float PASCAL EXPORT HBV_Basic(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT HBV_WithIrrigation(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT HBV_WithQuickflow(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT HBV_WithRadiationDrivenSnow(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT HBV_OnlyGroundwater(FlowContext *pFlowContext);

// deprecated models
extern "C" float PASCAL EXPORT HBV_Global(FlowContext *pFlowContext); // deprecated
extern "C" float PASCAL EXPORT HBV_Global_Radiation(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT HBV_Agriculture(FlowContext *pFlowContext);

// exchange handlers
extern "C" float PASCAL EXPORT HBV_Horizontal(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT HBV_Vertical(FlowContext *pFlowContext);
//extern "C" float PASCAL EXPORT HBV_HorizontalwSnow( FlowContext *pFlowContext );
//extern "C" float PASCAL EXPORT HBV_VerticalwSnow( FlowContext *pFlowContext );

// flux handlers
extern "C" float PASCAL EXPORT ETFluxHandler(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT PrecipFluxHandler(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT PrecipFluxHandlerNoSnow(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT SnowFluxHandler(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT RunoffFluxHandler(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT UplandFluxHandler(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT RuninFluxHandler(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT N_Deposition_FluxHandler(FlowContext *pFlowContext);
extern "C" float PASCAL EXPORT N_ET_FluxHandler(FlowContext *pFlowContext);

extern "C" float PASCAL EXPORT ExchangeFlowToCalvin(FlowContext * pFlowContext);

// state variable handlers
extern "C" float PASCAL EXPORT N_Transformations(FlowContext *pFlowContext);

// misc
extern "C" BOOL PASCAL EXPORT StreamTempRun(FlowContext *pFlowContext);
extern "C" BOOL PASCAL EXPORT StreamTempInit(FlowContext *pFlowContext, LPCTSTR);
extern "C" BOOL PASCAL EXPORT StreamTempReg(FlowContext *pFlowContext);
//

/////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////////////////////////////


// Model implementation
float PASCAL HBV_Basic(FlowContext *pFlowContext) { return theModel->HBV_Basic(pFlowContext); }
float PASCAL HBV_WithIrrigation(FlowContext *pFlowContext) { return theModel->HBV_WithIrrigation(pFlowContext); }
float PASCAL HBV_OnlyGroundwater(FlowContext *pFlowContext) { return theModel->HBV_OnlyGroundwater(pFlowContext); }
float PASCAL HBV_WithQuickflow(FlowContext *pFlowContext) { return theModel->HBV_WithQuickflow(pFlowContext); }
float PASCAL HBV_WithRadiationDrivenSnow(FlowContext *pFlowContext) { return theModel->HBV_WithRadiationDrivenSnow(pFlowContext); }

// deprecated methods
float PASCAL HBV_Global(FlowContext *pFlowContext) { return theModel->HBV_Basic(pFlowContext); }  // deprecated
float PASCAL HBV_Global_Radiation(FlowContext *pFlowContext) { return theModel->HBV_WithRadiationDrivenSnow(pFlowContext); }  // deprecated
float PASCAL HBV_Agriculture(FlowContext *pFlowContext) { return theModel->HBV_WithQuickflow(pFlowContext); }     // deprecated

																												  // exchange handlers
float PASCAL HBV_Horizontal(FlowContext *pFlowContext) { return theModel->HBV_Horizontal(pFlowContext); }
float PASCAL HBV_Vertical(FlowContext *pFlowContext) { return theModel->HBV_Vertical(pFlowContext); }
//float PASCAL HBV_HorizontalwSnow( FlowContext *pFlowContext ) { return theModel->HBV_HorizontalwSnow( pFlowContext ); }
//float PASCAL HBV_VerticalwSnow( FlowContext *pFlowContext ) { return theModel->HBV_VerticalwSnow( pFlowContext ); }

// flux handlers
float PASCAL ETFluxHandler(FlowContext *pFlowContext) { return theModel->ETFluxHandler(pFlowContext); }
float PASCAL PrecipFluxHandler(FlowContext *pFlowContext) { return theModel->PrecipFluxHandler(pFlowContext); }
float PASCAL PrecipFluxHandlerNoSnow(FlowContext *pFlowContext) { return theModel->PrecipFluxHandlerNoSnow(pFlowContext); }
float PASCAL SnowFluxHandler(FlowContext *pFlowContext) { return theModel->SnowFluxHandler(pFlowContext); }
float PASCAL RunoffFluxHandler(FlowContext *pFlowContext) { return theModel->RunoffFluxHandler(pFlowContext); }
float PASCAL UplandFluxHandler(FlowContext *pFlowContext) { return theModel->UplandFluxHandler(pFlowContext); }
float PASCAL RuninFluxHandler(FlowContext *pFlowContext) { return theModel->RuninFluxHandler(pFlowContext); }
float PASCAL N_Deposition_FluxHandler(FlowContext *pFlowContext) { return theModel->N_Deposition_FluxHandler(pFlowContext); }
float PASCAL N_ET_FluxHandler(FlowContext *pFlowContext) { return theModel->N_ET_FluxHandler(pFlowContext); }


float PASCAL ExchangeFlowToCalvin(FlowContext* pFlowContext) { return theModel->ExchangeFlowToCalvin(pFlowContext); }

// state variable handlers
float PASCAL N_Transformations(FlowContext *pFlowContext) { return theModel->N_Transformations(pFlowContext); }



// not sure what these are doing here...
BOOL PASCAL StreamTempInit(FlowContext *pFlowContext, LPCTSTR initStr) { return theModel->StreamTempInit(pFlowContext, initStr); }
BOOL PASCAL StreamTempRun(FlowContext *pFlowContext) { return theModel->StreamTempRun(pFlowContext); }
BOOL PASCAL StreamTempReg(FlowContext *pFlowContext) { return theModel->StreamTempReg(pFlowContext); }
//
//BOOL PASCAL CalcDailyUrbanWater(FlowContext *pFlowContext) { return theModel->CalcDailyUrbanWater(pFlowContext); }
//BOOL PASCAL CalcDailyUrbanWaterInit(FlowContext *pFlowContext) { return theModel->CalcDailyUrbanWaterInit(pFlowContext); }
//BOOL PASCAL CalcDailyUrbanWaterRun(FlowContext *pFlowContext) { return theModel->CalcDailyUrbanWaterRun(pFlowContext); }

