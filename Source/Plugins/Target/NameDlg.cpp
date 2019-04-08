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
// NameDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Target.h"
#include "NameDlg.h"
#include "afxdialogex.h"

#include "resource.h"


// CNameDlg dialog

IMPLEMENT_DYNAMIC(CNameDlg, CDialogEx)

CNameDlg::CNameDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CNameDlg::IDD, pParent)
   , m_name(_T(""))
   {

}

CNameDlg::~CNameDlg()
{
}

void CNameDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Text(pDX, IDC_CNAME, m_name);
   }


BEGIN_MESSAGE_MAP(CNameDlg, CDialogEx)
END_MESSAGE_MAP()


// CNameDlg message handlers
