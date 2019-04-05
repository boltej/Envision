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
// LoadDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "LoadDlg.h"

#include <dirplaceholder.h>


// LoadDlg dialog

IMPLEMENT_DYNAMIC(LoadDlg, CDialog)

LoadDlg::LoadDlg(CWnd* pParent /*=NULL*/)
	: CDialog(LoadDlg::IDD, pParent)
   , m_name(_T(""))
   , m_path(_T(""))
   {

}

LoadDlg::~LoadDlg()
{
}

void LoadDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Text(pDX, IDC_NAME, m_name);
DDX_Text(pDX, IDC_PATH, m_path);
   }


BEGIN_MESSAGE_MAP(LoadDlg, CDialog)
   ON_BN_CLICKED(IDC_BROWSE, &LoadDlg::OnBnClickedBrowse)
END_MESSAGE_MAP()


// LoadDlg message handlers

void LoadDlg::OnBnClickedBrowse()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "dll", m_path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "DLL files|*.dll|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_path = dlg.GetPathName();

      if ( m_name.IsEmpty() )
         m_name = m_path;

      UpdateData( 0 );
      }

   }

void LoadDlg::OnOK()
   {
   UpdateData( 1 );
   CDialog::OnOK();
   }

BOOL LoadDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }
