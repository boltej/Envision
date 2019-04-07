
// EnvServer.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CEnvServerApp:
// See EnvServer.cpp for the implementation of this class
//

class CEnvServerApp : public CWinApp
{
public:
	CEnvServerApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CEnvServerApp theApp;
