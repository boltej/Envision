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
#if !defined(AFX_WIZRUN_H__45446545_7C09_11D3_95B9_00A076B0010A__INCLUDED_)
#define AFX_WIZRUN_H__45446545_7C09_11D3_95B9_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WizRun.h : header file
//


#include "PPRunMain.h"
#include "PPRunSetup.h"
#include "PPRunData.h"
//#include "PPRunPolicies.h"
//#include "PPRunActors.h"
#include "PPMetaGoals.h"

/////////////////////////////////////////////////////////////////////////////
// WizRun

class WizRun : public CPropertySheet
{
	DECLARE_DYNAMIC(WizRun)

// Construction
public:
	WizRun(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	WizRun(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

   PPRunMain          m_mainPage;
   PPRunSetup         m_setupPage;
   PPRunData          m_dataPage;
   //PPRunPolicies      m_policiesPage;
   //PPRunActors        m_actorsPage;
   PPMetaGoals        m_metagoalsPage;
   
// Attributes
public:
   int m_runState;  // 0=no run (setup only), 1=single, 2=multiple

// Operations
public:
   int SaveSetup();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(WizRun)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
   virtual ~WizRun() {}

	// Generated message map functions
protected:
	//{{AFX_MSG(WizRun)
	afx_msg void OnApplyNow();
   virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WizRun_H__45446545_7C09_11D3_95B9_00A076B0010A__INCLUDED_)
