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
#if !defined(AFX_PPRUNSETUP_H__3C45B339_F359_42BE_B268_E43D6BEA3224__INCLUDED_)
#define AFX_PPRUNSETUP_H__3C45B339_F359_42BE_B268_E43D6BEA3224__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PPRunSetup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PPRunSetup dialog

class PPRunSetup : public CPropertyPage
{
	DECLARE_DYNCREATE(PPRunSetup)

// Construction
public:
	PPRunSetup();
	~PPRunSetup();

// Dialog Data
	//{{AFX_DATA(PPRunSetup)
	enum { IDD = IDD_RUNSETUP };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PPRunSetup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

protected:
   CCheckListBox m_models;
   CCheckListBox m_aps;

public:
   int SetModelsToUse(void);

public:
   //bool m_culturalEnabled;
   //int  m_culturalFrequency;
   //bool m_policyEnabled;
   //int  m_policyFrequency;

protected:
//   void EnableControls(void);
public:
//   afx_msg void OnBnClicked();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PPRUNSETUP_H__3C45B339_F359_42BE_B268_E43D6BEA3224__INCLUDED_)
