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


// ChangeTypeDlg dialog

class ChangeTypeDlg : public CDialog
{
	DECLARE_DYNAMIC(ChangeTypeDlg)

public:
	ChangeTypeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~ChangeTypeDlg();

// Dialog Data
	enum { IDD = IDD_CHANGETYPE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void LoadFields();
   void LoadField();
   void EnableCtrls();

	DECLARE_MESSAGE_MAP()
public:
   CComboBox m_layers;
   CComboBox m_fields;
   CString m_oldType;
   CComboBox m_type;
   int m_width;
   CString m_format;

   virtual BOOL OnInitDialog();
   afx_msg void OnCbnSelchangeType();
   afx_msg void OnCbnSelchangeLayers();
   afx_msg void OnCbnSelchangeFields();
   virtual void OnOK();
   BOOL m_useFormat;
   };
