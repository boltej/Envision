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
#if !defined(AFX_NewIniFileDlg_H__45446545_7C09_11D3_95B9_00A076B0010A__INCLUDED_)
#define AFX_NewIniFileDlg_H__45446545_7C09_11D3_95B9_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewIniFileDlg.h : header file
//


#include "PPNewIniStep1.h"
#include "PPNewIniStep2.h"
#include "PPNewIniStep3.h"
#include "PPNewIniFinish.h"

/////////////////////////////////////////////////////////////////////////////
// NewIniFileDlg

class NewIniFileDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(NewIniFileDlg)

// Construction
public:
	NewIniFileDlg(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	NewIniFileDlg(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

   PPNewIniStep1      m_step1;
   PPNewIniStep2      m_step2;
   //PPNewIniStep3      m_step3;
   PPNewIniFinish     m_finish;
   
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(NewIniFileDlg)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
   virtual ~NewIniFileDlg() {}

	// Generated message map functions
protected:
	//{{AFX_MSG(NewIniFileDlg)
	afx_msg void OnApplyNow();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NewIniFileDlg_H__45446545_7C09_11D3_95B9_00A076B0010A__INCLUDED_)
