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
#if !defined(AFX_TABPAGESSL_H__619331B3_7DE7_4DB1_A039_2103E87E8E71__INCLUDED_)
#define AFX_TABPAGESSL_H__619331B3_7DE7_4DB1_A039_2103E87E8E71__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// CTabPageSSL declaration

#include "EnvWinLibs.h"

class WINLIBSAPI CTabPageSSL : public CDialog
{
public:
// Construction
	CTabPageSSL ();	// Default Constructor
	CTabPageSSL (UINT nIDTemplate, CWnd* pParent = NULL);	// Standard Constructor
// Destruction
	~CTabPageSSL ();

	// Enable/disable command routing to the parent.
	void EnableRouteCommand(bool bRoute = true) { m_bRouteCommand = bRoute; };
	bool IsRouteCommand() { return m_bRouteCommand; };
	// Enable CmdMsg routing to the parent.
	void EnableRouteCmdMsg(bool bRoute = true) { m_bRouteCmdMsg = bRoute; };
	bool IsRouteCmdMsg() { return m_bRouteCmdMsg; };
	// Enable/Disable Notify routing to the parent.
	void EnableRouteNotify(bool bRoute = true) { m_bRouteNotify = bRoute; };
	bool IsRouteNotify() { return m_bRouteNotify; };

protected:
// Message Handlers
	virtual BOOL OnCommand (WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify (WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual void OnOK (void);
	virtual void OnCancel (void);
	virtual BOOL OnCmdMsg (UINT nID, int nCode, void* pExtra,
		AFX_CMDHANDLERINFO* pHandlerInfo);

// Routing flags
	bool m_bRouteCommand;
	bool m_bRouteCmdMsg;
	bool m_bRouteNotify;
};

#endif // !defined(AFX_TABPAGE_H__619331B3_7DE7_4DB1_A039_2103E87E8E71__INCLUDED_)
