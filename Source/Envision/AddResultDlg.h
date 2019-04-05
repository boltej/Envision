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
#include "afxwin.h"


// AddResultDlg dialog

class AddResultDlg : public CDialog
{
	DECLARE_DYNAMIC(AddResultDlg)

public:
	AddResultDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~AddResultDlg();

   CString result;

// Dialog Data
	enum { IDD = IDD_ADDRESULT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void LoadValues();

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
   void LoadFieldInfo();
   afx_msg void OnCbnSelchangeFields();

protected:
   virtual void OnOK();

public:
   CComboBox m_fields;
   CComboBox m_values;
   CString   m_result;
   CEdit m_bufferFn;
   CEdit m_addPointFn;
   CEdit m_incrementFn;
   CEdit m_expandFn;
   afx_msg void OnBnClickedBuffer();
   afx_msg void OnBnClickedAddpoint();
   afx_msg void OnBnClickedIncrement();
   afx_msg void OnBnClickedField();
   afx_msg void OnBnClickedExpand();
   CButton m_field;
   };
