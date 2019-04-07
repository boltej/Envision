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

#include "EnvWinLibs.h"
#include "Libs.h"
#include "NameDlg.h"
#include "afxdialogex.h"


// NameDlg dialog

IMPLEMENT_DYNAMIC(NameDlg, CDialog)

NameDlg::NameDlg(CWnd* pParent /*=NULL*/)
	: CDialog(NameDlg::IDD, pParent)
   , m_name(_T(""))
   , m_title( _T("Enter name") )
   {

}

NameDlg::~NameDlg()
{
}

void NameDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Text(pDX, IDC_LIB_NAME, m_name);
   }


BEGIN_MESSAGE_MAP(NameDlg, CDialog)
END_MESSAGE_MAP()


// NameDlg message handlers

BOOL NameDlg::OnInitDialog() 
   {      
	CDialog::OnInitDialog();
   SetWindowText( m_title );

   //HWND hWnd;
   //GetDlgItem( IDC_LIB_NAME, &hWnd);
   //::PostMessage(hWnd, WM_SETFOCUS, 0, 0);

	return TRUE;
   }
