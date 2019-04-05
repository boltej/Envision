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
#include "afxcmn.h"


// OptionsDlg dialog

class OptionsDlg : public CDialog
{
	DECLARE_DYNAMIC(OptionsDlg)

public:
	OptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~OptionsDlg();

// Dialog Data
	enum { IDD = IDD_OPTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   BOOL m_showTips;
   BOOL m_checkForUpdates;
   bool m_isDirty;

protected:
   virtual void OnOK();
   bool SaveRegistryKeys();

   void LoadAnalysisModules();
   void LoadVisualizationModules();
   void LoadDataManagementModules();

public:
   CListBox m_addIns;
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedDelete();
   afx_msg void OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult);
   CTabCtrl m_tabCtrl;
   afx_msg void OnBnClickedShowtips();
   afx_msg void OnBnClickedCheckforupdates();
   };
