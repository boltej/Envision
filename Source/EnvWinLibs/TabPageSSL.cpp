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
#include "EnvWinLibs.h"
#include "TabPageSSL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction

CTabPageSSL::CTabPageSSL () {
#ifndef _AFX_NO_OCC_SUPPORT
	AfxEnableControlContainer ();
#endif // !_AFX_NO_OCC_SUPPORT
	m_bRouteCommand = false;
	m_bRouteCmdMsg = false;
	m_bRouteNotify = false;
}

CTabPageSSL::CTabPageSSL (UINT nIDTemplate, CWnd* pParent /*=NULL*/)
	: CDialog(nIDTemplate, pParent) {
#ifndef _AFX_NO_OCC_SUPPORT
	AfxEnableControlContainer ();
#endif // !_AFX_NO_OCC_SUPPORT
	m_bRouteCommand = false;
	m_bRouteCmdMsg = false;
	m_bRouteNotify = false;
}

/////////////////////////////////////////////////////////////////////////////
// Destruction

CTabPageSSL::~CTabPageSSL () {
}

/////////////////////////////////////////////////////////////////////////////
// Message Handlers

void CTabPageSSL::OnOK (void) {
	//
	// Prevent CDialog::OnOK from calling EndDialog.
	//
}

void CTabPageSSL::OnCancel (void) {
	//
	// Prevent CDialog::OnCancel from calling EndDialog.
	//
}

BOOL CTabPageSSL::OnCommand (WPARAM wParam, LPARAM lParam)
   {
	// Call base class OnCommand to allow message map processing
	BOOL bReturn = CDialog::OnCommand (wParam, lParam);

	if (true == m_bRouteCommand)
	   {
		//
		// Forward WM_COMMAND messages to the dialog's parent.
		//
		return (BOOL) GetParent()->SendMessage(WM_COMMAND, wParam, lParam);
	   }

	return bReturn;
   }

BOOL CTabPageSSL::OnNotify (WPARAM wParam, LPARAM lParam, LRESULT* pResult)
   {
	BOOL bReturn = CDialog::OnNotify(wParam, lParam, pResult);

	if (true == m_bRouteNotify)
   	{
		//
		// Forward WM_NOTIFY messages to the dialog's parent.
		//
		return (BOOL) GetParent ()->SendMessage (WM_NOTIFY, wParam, lParam);
	   }

	return bReturn;
   }

BOOL CTabPageSSL::OnCmdMsg (UINT nID, int nCode, void* pExtra,
	AFX_CMDHANDLERINFO* pHandlerInfo) {
	BOOL bReturn = CDialog::OnCmdMsg (nID, nCode, pExtra, pHandlerInfo);

#ifndef _AFX_NO_OCC_SUPPORT
	if (true == m_bRouteCmdMsg)
	{
		//
		// Forward ActiveX control events to the dialog's parent.
		//
		if (nCode == CN_EVENT)
			return GetParent ()->OnCmdMsg (nID, nCode, pExtra, pHandlerInfo);
	}
#endif // !_AFX_NO_OCC_SUPPORT

	return bReturn;
}
