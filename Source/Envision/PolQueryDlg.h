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

// PolQueryDlg dialog

class PolQueryDlg : public CDialog
{
	DECLARE_DYNAMIC(PolQueryDlg)

public:
	PolQueryDlg(MapLayer *pLayer, CWnd* pParent = NULL);   // standard constructor
	virtual ~PolQueryDlg();

// Dialog Data
	enum { IDD = IDD_POLQUERYBUILDER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void LoadFieldValues();

   MapLayer *m_pLayer;

   DECLARE_MESSAGE_MAP()
public:
   CString m_queryString;

   CComboBox m_ops;
   CComboBox m_values;
   CEdit m_query;
   CComboBox m_fields;

   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnCbnSelchangeFields();
   afx_msg void OnBnClickedAnd();
   afx_msg void OnBnClickedOr();

protected:
   virtual void OnOK();
public:
   afx_msg void OnBnClickedSpatialops();
   afx_msg void OnBnClickedQueryviewer();
   };
