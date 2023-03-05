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
#include "resource.h"
#include <EnvPolicy.h>


// SiteAttrDlg dialog

class SiteAttrDlg : public CDialog
{
	DECLARE_DYNAMIC(SiteAttrDlg)

public:
	SiteAttrDlg(EnvPolicy *pPolicy, CWnd* pParent = NULL);   // standard constructor
	virtual ~SiteAttrDlg();

// Dialog Data
	enum { IDD = IDD_SITEATTR };

protected:
   EnvPolicy *m_pPolicy;

   void LoadFieldValues();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnCbnSelchangeFields();
   CComboBox m_fields;
   CComboBox m_ops;
   CComboBox m_values;
   CEdit m_query;
   afx_msg void OnBnClickedOk();
   afx_msg void OnBnClickedAnd();
   afx_msg void OnBnClickedOr();
   afx_msg void OnBnClickedSpatialops();
};
