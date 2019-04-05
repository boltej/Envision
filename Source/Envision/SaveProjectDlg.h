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
#pragma once
#include "afxwin.h"


// SaveProjectDlg dialog

class SaveProjectDlg : public CDialogEx
{
	DECLARE_DYNAMIC(SaveProjectDlg)

public:
	SaveProjectDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~SaveProjectDlg();

// Dialog Data
	enum { IDD = IDD_SAVEPROJECT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   CString m_projectPath;
   CString m_lulcPath;
   CString m_policyPath;
   CString m_scenarioPath;
   afx_msg void OnBnClickedBrowseenvx();
   afx_msg void OnBnClickedBrowselulc();
   afx_msg void OnBnClickedBrowsepolicy();
   afx_msg void OnBnClickedBrowsescenario();
   virtual BOOL OnInitDialog();
   virtual void OnOK();

   CButton m_cbLulcNoSave;
   BOOL m_lulcNoSave;
   CButton m_cbPolicyNoSave;
   BOOL m_policyNoSave;
   CButton m_cbScenarioNoSave;
   BOOL m_scenarioNoSave;

   afx_msg void UpdateUI();
   };
