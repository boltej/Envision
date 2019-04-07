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
#if !defined(AFX_CELLPROPDLG_H__BF6D0365_AB11_4D8F_956C_1878407F2E10__INCLUDED_)
#define AFX_CELLPROPDLG_H__BF6D0365_AB11_4D8F_956C_1878407F2E10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// CellPropDlg.h : header file
//

class MapLayer;
class MapWindow;

#include "grid\gridctrl.h"
#include "EasySize.h"

#include "EnvWinLib_resource.h"


/////////////////////////////////////////////////////////////////////////////
// CellPropDlg dialog

class WINLIBSAPI CellPropDlg : public CDialog
{
DECLARE_EASYSIZE

// Construction
public:
   CellPropDlg(MapWindow *pMapWnd, MapLayer *pLayer, int currentCell, CWnd* pParent);   // standard constructor

   CGridCtrl  m_grid;
   MapWindow *m_pMapWnd;
   MapLayer  *m_pLayer;

// Dialog Data
	//{{AFX_DATA(CellPropDlg)
	enum { IDD = IDD_LIB_CELLPROPERTIES };
	CButton	m_flashCell;
	//}}AFX_DATA

protected:
   int   m_currentCell;
   CWnd *m_pParent;

   static CRect m_windowRect;

public:
   bool m_isModal;

public:
   void SetLayer( MapLayer *pLayer ) { m_pLayer = pLayer; }
   void SetCurrentCell( int currentCell ){ m_currentCell = currentCell; LoadCell();  }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CellPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void LoadCell( void );

	// Generated message map functions
	//{{AFX_MSG(CellPropDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnFlashcell();
	afx_msg void OnNextcell();
	afx_msg void OnPrevcell();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   virtual void OnCancel();
public:
   afx_msg void OnDestroy();
protected:
   virtual void PostNcDestroy();
public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CELLPROPDLG_H__BF6D0365_AB11_4D8F_956C_1878407F2E10__INCLUDED_)
