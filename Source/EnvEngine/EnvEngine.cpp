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
// EnvEngine.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "EnvEngine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#ifndef NO_MFC
//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" bool PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CEnvEngineApp

BEGIN_MESSAGE_MAP(CEnvEngineApp, CWinApp)
END_MESSAGE_MAP()


// CEnvEngineApp construction

CEnvEngineApp::CEnvEngineApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CEnvEngineApp object

CEnvEngineApp theApp;


// CEnvEngineApp initialization

BOOL CEnvEngineApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

#endif   // NO_MFC
