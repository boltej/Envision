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
#if !defined(AFX_VDATATABLE_H__7D8BB2EC_A943_44AA_9CA7_D363AB86950B__INCLUDED_)
#define AFX_VDATATABLE_H__7D8BB2EC_A943_44AA_9CA7_D363AB86950B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VDataTable.h : header file
//


#include <grid\gridctrl.h>
#include <vdataobj.h>

/////////////////////////////////////////////////////////////////////////////
// VDataTable window

class WINLIBSAPI VDataTable : public CWnd
{
// Construction
public:
   VDataTable() : m_pData( NULL ) {}
	VDataTable( VDataObj*);

// Attributes
public:
   VDataObj *m_pData;
   CGridCtrl m_grid;

// Operations
public:
   void LoadTable();
   void SetDataSource( VDataObj *pData );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(VDataTable)
	public:
   virtual BOOL Create( LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID );
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~VDataTable();

	// Generated message map functions
protected:
	//{{AFX_MSG(VDataTable)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
   };

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VDATATABLE_H__7D8BB2EC_A943_44AA_9CA7_D363AB86950B__INCLUDED_)
