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

// AddDbColDlg dialog

class AddDbColDlg : public CDialog
{
	DECLARE_DYNAMIC(AddDbColDlg)

public:
	AddDbColDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~AddDbColDlg();

// Dialog Data
	enum { IDD = IDD_ADDDBCOL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void EnableCtrls();

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedAdd();
   virtual BOOL OnInitDialog();
   CComboBox m_type;
   afx_msg void OnCbnSelchangeType();
   CEdit m_width;
   CComboBox m_layers;
   afx_msg void OnCbnSelchangeLayers();
   };
