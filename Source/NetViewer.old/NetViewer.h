
// NetViewer.h : main header file for the NetViewer application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CNetViewerApp:
// See NetViewer.cpp for the implementation of this class
//

class CNetViewerApp : public CWinApp
{
public:
	CNetViewerApp() noexcept;


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CNetViewerApp theApp;
