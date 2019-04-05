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
#if !defined(AFX_CLASSIFYLAYERDLG_H__1F0032A4_21AA_11D3_95A7_00A076B0010A__INCLUDED_)
#define AFX_CLASSIFYLAYERDLG_H__1F0032A4_21AA_11D3_95A7_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClassifyLayerDlg.h : header file
//

class MapLayer;
#include "resource.h"
#include <afxcoll.h>
#include "afxwin.h"

#include <grid\gridctrl.h>


class BinColorGridCell : public CGridCell
{
   public:
      virtual void OnDblClick( CPoint PointCellRelative );

   DECLARE_DYNCREATE(BinColorGridCell)
};



/////////////////////////////////////////////////////////////////////////////
// ClassifyLayerDlg dialog

class ClassifyLayerDlg : public CDialog
{
friend class BinColorGridCell;

// Construction
public:
	ClassifyLayerDlg(MapLayer*, CWnd* pParent = NULL);   // standard constructor

   MapLayer *m_pLayer;
   //CStringArray m_rangeStringArray;

// Dialog Data
	//{{AFX_DATA(ClassifyLayerDlg)
	enum { IDD = IDD_CLASSIFYLAYER };
	CStatic	m_fieldType;
	CButton	m_uniqueValues;
	CSpinButtonCtrl	m_binCountSpin;
	CComboBox	m_dataField;
	CComboBox	m_colorRamp;
	CListCtrl	m_binList;
	CEdit	m_binCount;

   CGridCtrl m_grid;

	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ClassifyLayerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   void UpdateInterface( void );
   void LoadBins( void );

	// Generated message map functions
	//{{AFX_MSG(ClassifyLayerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnGeneratebins();
	virtual void OnOK();
	afx_msg void OnSelchangeDatafield();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedSpecminval();
   afx_msg void OnBnClickedSpecmaxval();
   CEdit m_minVal;
   CEdit m_maxVal;
   CButton m_specMinVal;
   CButton m_specMaxVal;
   afx_msg void OnBnClickedUniquevalues();
   };

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLASSIFYLAYERDLG_H__1F0032A4_21AA_11D3_95A7_00A076B0010A__INCLUDED_)
