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

// RemoveDbColsDlg dialog

class RemoveDbColsDlg : public CDialog
{
	DECLARE_DYNAMIC(RemoveDbColsDlg)

public:
	RemoveDbColsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~RemoveDbColsDlg();

// Dialog Data
	enum { IDD = IDD_REMOVEDBCOL };

   CComboBox m_layers;
   CCheckListBox m_fields;

   virtual BOOL OnInitDialog();
   virtual void OnOK();

   afx_msg void OnCbnSelchangeLayers();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void LoadFields();
	DECLARE_MESSAGE_MAP()
};
