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
// PPNewIniStep1.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "PPNewIniStep1.h"
#include <dirPlaceHolder.h>

//#include <SelectFolderDlg.h>
#include <FolderDlg.h>


// PPNewIniStep1 dialog

IMPLEMENT_DYNAMIC(PPNewIniStep1, CPropertyPage)

PPNewIniStep1::PPNewIniStep1(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PPNewIniStep1::IDD)
   , m_pathIni(_T(""))
   , m_projectName(_T("") )
{
}

PPNewIniStep1::~PPNewIniStep1()
{
}

void PPNewIniStep1::DoDataExchange(CDataExchange* pDX)
{
CPropertyPage::DoDataExchange(pDX);
DDX_Text(pDX, IDC_FILE, m_projectName);
DDX_Text(pDX, IDC_PATH, m_pathIni);
}


BEGIN_MESSAGE_MAP(PPNewIniStep1, CPropertyPage)
   ON_BN_CLICKED(IDC_BROWSE, &PPNewIniStep1::OnBnClickedBrowse)
END_MESSAGE_MAP()


// PPNewIniStep1 message handlers

void PPNewIniStep1::OnBnClickedBrowse()
   {
   DirPlaceholder d;

   UpdateData( TRUE );

   //CDirDialog dlg( _T("Select Folder"), _T("c:\\") );
   //if ( dlg.DoBrowse() )
   //   this->m_pathIni = dlg.m_strPath;

   CFolderDialog dlg( _T( "Select Folder" ), NULL, this, BIF_EDITBOX | BIF_NEWDIALOGSTYLE );
   if( dlg.DoModal() == IDOK  )
      this->m_pathIni = dlg.GetFolderPath();

   UpdateData( FALSE );
   }


BOOL PPNewIniStep1::OnSetActive()
   {
   CPropertySheet* pSheet = (CPropertySheet*) GetParent();   
   pSheet->SetWizardButtons(PSWIZB_NEXT);
   return CPropertyPage::OnSetActive();
   }

