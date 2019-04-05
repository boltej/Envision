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

#include "resource.h"

#include <TabPageSSL.h>
#include "afxwin.h"


class EnvModelProcess;
class IniFileEditor;


// PPIniProcesses dialog

class PPIniProcesses : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPIniProcesses)

public:
	PPIniProcesses(IniFileEditor* pParent);   // standard constructor
	virtual ~PPIniProcesses();

// Dialog Data
	enum { IDD = IDD_INIPROCESSES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   bool m_isProcessDirty;
   bool m_isProcessesDirty;

   EnvModelProcess *GetAP();


public:
   IniFileEditor *m_pParent;

   CCheckListBox m_aps;
   CString m_label;
   CString m_path;
   int m_timing;
   int m_ID;
   BOOL m_useCol;
   CString m_fieldName;
   CString m_initStr;

   void LoadAP();
   bool StoreChanges();

   afx_msg void OnBnClickedBrowse();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedRemove();

   virtual BOOL OnInitDialog();

   afx_msg void OnLbnSelchangeAps();
   afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
   afx_msg void OnEnChangeLabel();
   afx_msg void OnEnChangePath();
   afx_msg void OnEnChangeID();
   afx_msg void OnEnChangeField();
   afx_msg void OnEnChangeInitStr();
   afx_msg void OnBnClickedTiming();
   afx_msg void OnBnClickedUsecol();

   afx_msg void OnLbnChkchangeAps();

   afx_msg void MakeDirty();
   };
