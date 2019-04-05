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
#include "afxwin.h"
#if !defined(AFX_PPRUNMAIN_H__A76913A2_C847_11D3_95C5_00A076B0010A__INCLUDED_)
#define AFX_PPRUNMAIN_H__A76913A2_C847_11D3_95C5_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PPRunMain.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// PPRunMain dialog

class PPRunMain : public CPropertyPage
{
	DECLARE_DYNCREATE(PPRunMain)

// Construction
public:
	PPRunMain();
	~PPRunMain();

// Dialog Data
	enum { IDD = IDD_RUN };

   int  m_iterations;
   BOOL m_dynamicUpdateViews;
   BOOL m_dynamicUpdateMap;
   BOOL m_useInitialSeed;
   BOOL m_exportMap;
   int  m_exportInterval;
   BOOL m_exportBmps;
   CString  m_fieldList;
   int  m_pixelSize;
   BOOL m_exportOutputs;
//   int  m_evalFrequency;

// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnRun();

   //void LoadScenarios();
   void UpdateUI();

	DECLARE_MESSAGE_MAP()

public:
   //afx_msg void OnBnClickedMultirun();
   //afx_msg void OnCbnSelchangeScenarios();
   //CComboBox m_scenarios;
   //afx_msg void OnBnClickedRunsingle();
   //afx_msg void OnBnClickedRunmultiple();

   afx_msg void OnBnClickedExportmap();
   afx_msg void OnBnClickedExportBmps();
   };

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPRUNMAIN_H__A76913A2_C847_11D3_95C5_00A076B0010A__INCLUDED_)
