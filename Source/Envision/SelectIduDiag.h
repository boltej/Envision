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
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"

// SelectIduDiag dialog

class SelectIduDiag : public CDialog
{
	DECLARE_DYNAMIC(SelectIduDiag)

public:
	SelectIduDiag(CWnd* pParent = NULL);   // standard constructor
	virtual BOOL OnInitDialog();
	virtual ~SelectIduDiag();
	virtual BOOL UpdateData( BOOL bSaveAndValidate = TRUE );

// Dialog Data
	enum { IDD = IDD_SELECTIDU };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CString m_strIduText;
	CListCtrl m_listView;

	CArray <int, int> m_selectedIDUsTemporary;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedRemoveidu();
public:
	afx_msg void OnBnClickedAddidutext();
public:
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnBnClickedAddcurrentidu();
};
