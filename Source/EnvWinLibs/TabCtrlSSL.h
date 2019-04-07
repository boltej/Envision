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
#if !defined(AFX_TABCTRLSSL_H__75BE48A7_864C_11D5_9F04_000102FB9990__INCLUDED_)
#define AFX_TABCTRLSSL_H__75BE48A7_864C_11D5_9F04_000102FB9990__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TabCtrlEx.h : header file
//

#include <afxtempl.h>
#include "TabPageSSL.h"

#ifdef _DEBUG
#pragma pack (push, 1)

typedef struct {
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATEEX;

#pragma pack (pop)
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlSSL window

class WINLIBSAPI CTabCtrlSSL : public CTabCtrl {
public:
// Construction
	CTabCtrlSSL ();
// Destruction
	virtual ~CTabCtrlSSL (void);
// Page Functions
	int		AddSSLPage (LPCTSTR pszTitle, int nPageID, CTabPageSSL* pTabPage);
	int		AddSSLPage (LPCTSTR pszTitle, int nPageID,	LPCTSTR pszTemplateName);
	int		AddSSLPage (LPCTSTR pszTitle, int nPageID, int nTemplateID);
	BOOL	RemoveSSLPage (int nIndex);
	int		GetSSLPageCount (void);
	BOOL	GetSSLPageTitle (int nIndex, CString& strTitle);
	BOOL	SetSSLPageTitle (int nIndex, LPCTSTR pszTitle);
	int		GetSSLPageID (int nIndex);
	int		SetSSLPageID (int nIndex, int nPageID);
	BOOL	ActivateSSLPage (int nIndex);
	int		GetSSLActivePage (void);
	CWnd*	GetSSLPage (int nIndex);
	int		GetSSLPageIndex (int nPageID);
   void SetResizable( bool resizable ) { m_resizable = resizable; }

protected:
    struct TabDelete 
       {
       CTabPageSSL*   pTabPage;
       BOOL        bDelete;
       };

   CArray<TabDelete, TabDelete> m_tabs;
	CArray<HWND, HWND> m_hFocusWnd;
	CArray<int, int> m_nPageIDs;

   bool m_resizable;

	int AddPage (LPCTSTR pszTitle, int nPageID, TabDelete tabDelete);

   virtual BOOL OnInitPage (int nIndex, int nPageID);
	virtual void OnActivatePage (int nIndex, int nPageID);
	virtual void OnDeactivatePage (int nIndex, int nPageID);
	virtual void OnDestroyPage (int nIndex, int nPageID);
	virtual BOOL OnCommand (WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify (WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnCmdMsg (UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

#ifdef _DEBUG
	BOOL CheckDialogTemplate (LPCTSTR pszTemplateName);
#endif // _DEBUG
	void ResizeDialog (int nIndex, int cx, int cy);
	// Generated message map functions
protected:
	afx_msg void OnDestroy (void);
	afx_msg void OnSetFocus (CWnd* pOldWnd);
	afx_msg void OnKillFocus (CWnd* pNewWnd);
	afx_msg void OnSelChanging (NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChange (NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABCTRLSSL_H__75BE48A7_864C_11D5_9F04_000102FB9990__INCLUDED_)
