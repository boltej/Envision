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

#include "EnvWinLibs.h"
#include "EnvWinLib_resource.h"


// NameDlg dialog

class WINLIBSAPI NameDlg : public CDialog
{
	DECLARE_DYNAMIC(NameDlg)

public:
	NameDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~NameDlg();

// Dialog Data
	enum { IDD = IDD_LIB_NAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   // Generated message map functions
   virtual BOOL OnInitDialog();

public:
   CString m_name;
   CString m_title;
   };
