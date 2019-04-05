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

#include "Resource.h"

#include <grid\gridctrl.h>



//class AttrGrid : public CGridCtrl
//{
//public:
   
   //virtual void  EndEditing() { m_pParent->MakeDirty(); }
   //afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point) { m_pParent->MakeDirty(); }
//};


// TextToIdDlg dialog

class TextToIdDlg : public CDialog
{
	DECLARE_DYNAMIC(TextToIdDlg)

public:
	TextToIdDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~TextToIdDlg();

// Dialog Data
	enum { IDD = IDD_TEXT_TO_ID };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   CComboBox m_layers;
   CComboBox m_textField;
   CComboBox m_idField;

   CGridCtrl  m_grid;


protected:
   void LoadFields();

public:
   virtual BOOL OnInitDialog();

   afx_msg void OnBnClickedOk();
   afx_msg void OnCbnSelchangeLayers();
   afx_msg void OnCbnChangeTextField();
};
