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

class MapLayer;

// FieldCalculatorDlg dialog

class FieldCalculatorDlg : public CDialog
{
	DECLARE_DYNAMIC(FieldCalculatorDlg)

public:
	FieldCalculatorDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~FieldCalculatorDlg();

// Dialog Data
	enum { IDD = IDD_CALCULATEFIELD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   MapLayer *m_pLayer;

   void LoadFieldInfo();

public:
   CComboBox m_layers;
   CComboBox m_fields;
   CEdit m_formula;
   CListBox m_fields1;
   CListBox m_functions;
   CListBox m_operators;
   CEdit m_query;
   afx_msg void OnCbnSelchangeLayers();
   afx_msg void OnCbnSelchangeLayers1();
   afx_msg void OnLbnDblclkFields1();
   afx_msg void OnLbnDblclkFunctions();
   afx_msg void OnLbnDblclkOps();
   virtual BOOL OnInitDialog();
protected:
   virtual void OnOK();
   };
