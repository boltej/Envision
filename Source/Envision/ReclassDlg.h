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

class MapLayer;


// ReclassDlg dialog

class ReclassDlg : public CDialog
{
	DECLARE_DYNAMIC(ReclassDlg)

public:
	ReclassDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~ReclassDlg();

// Dialog Data
	enum { IDD = IDD_RECLASS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   MapLayer *m_pLayer;

   void LoadFieldInfo();
   
	DECLARE_MESSAGE_MAP()
public:
   CComboBox m_layers;
   CComboBox m_fields;
   //CEdit m_pairs;
   afx_msg void OnCbnSelchangeLayer();
   afx_msg void OnCbnSelchangeField();
   afx_msg void OnBnClickedExtract();
   virtual BOOL OnInitDialog();
   virtual void OnOK();
   CString m_fieldName;
   CString m_reclassPairs;
   afx_msg void OnBnClickedLoadfromfile();
   afx_msg void OnEnChangeReclassvalues();
   };
