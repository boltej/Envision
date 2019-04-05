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
// SaveXmlOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "SaveXmlOptionsDlg.h"

#include <dirPlaceholder.h>


// SaveXmlOptionsDlg dialog

IMPLEMENT_DYNAMIC(SaveXmlOptionsDlg, CDialog)

SaveXmlOptionsDlg::SaveXmlOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SaveXmlOptionsDlg::IDD, pParent)
   , m_file(_T(""))
   , m_saveType( 0 )
   {

}

SaveXmlOptionsDlg::~SaveXmlOptionsDlg()
{
}

void SaveXmlOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Text(pDX, IDC_FILE, m_file);
   }


BEGIN_MESSAGE_MAP(SaveXmlOptionsDlg, CDialog)
   ON_BN_CLICKED(IDC_BROWSE, &SaveXmlOptionsDlg::OnBnClickedBrowse)
   ON_BN_CLICKED(IDC_NOSAVE, &SaveXmlOptionsDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_SAVEPRJ, &SaveXmlOptionsDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_SAVEEXTERNAL, &SaveXmlOptionsDlg::OnBnClickedRadio)
END_MESSAGE_MAP()


// SaveXmlOptionsDlg message handlers
BOOL SaveXmlOptionsDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   CheckDlgButton( IDC_NOSAVE, 1 );
   OnBnClickedRadio();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void SaveXmlOptionsDlg::OnBnClickedBrowse()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", m_file, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Xml files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_file = dlg.GetPathName();
      UpdateData( 0 );
      }   
   }

void SaveXmlOptionsDlg::OnOK()
   {
   if ( IsDlgButtonChecked( IDC_NOSAVE ) )
      m_saveType = 0;
   else if ( IsDlgButtonChecked( IDC_SAVEPRJ ) )
      m_saveType = 1;
   else // if ( IsDlgButtonChecked( ID_SAVEEXTERNAL )
      m_saveType = 2;

   CDialog::OnOK();
   }


void SaveXmlOptionsDlg::OnBnClickedRadio()
   {
   if ( IsDlgButtonChecked( IDC_SAVEEXTERNAL ) )
      {
      GetDlgItem( IDC_FILE )->EnableWindow( 0 );
      GetDlgItem( IDC_BROWSE )->EnableWindow( 0 );
      }
   else
      {
      GetDlgItem( IDC_FILE )->EnableWindow( 1 );
      GetDlgItem( IDC_BROWSE )->EnableWindow( 1 );
      }
  
   }
