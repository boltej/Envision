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
// PPNewIniStep2.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "PPNewIniStep2.h"
#include "NewIniFileDlg.h"
#include <dirPlaceHolder.h>

// PPNewIniStep2 dialog

IMPLEMENT_DYNAMIC(PPNewIniStep2, CPropertyPage)

PPNewIniStep2::PPNewIniStep2(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PPNewIniStep2::IDD)
   , m_pathLayer(_T(""))
   {

}

PPNewIniStep2::~PPNewIniStep2()
{
}

void PPNewIniStep2::DoDataExchange(CDataExchange* pDX)
{
CPropertyPage::DoDataExchange(pDX);
DDX_Text(pDX, IDC_FILE, m_pathLayer);
   }


BEGIN_MESSAGE_MAP(PPNewIniStep2, CPropertyPage)
   ON_BN_CLICKED(IDC_BROWSE, &PPNewIniStep2::OnBnClickedBrowse)
END_MESSAGE_MAP()


// PPNewIniStep2 message handlers

void PPNewIniStep2::OnBnClickedBrowse()
   {
   DirPlaceholder d;
   NewIniFileDlg *pDlg = (NewIniFileDlg*) GetParent();

   CFileDialog dlg( TRUE, "shp", pDlg->m_step1.m_pathIni, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Shape files|*.shp|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_pathLayer= dlg.GetPathName();
      UpdateData( 0 );
      }
   }


BOOL PPNewIniStep2::OnSetActive()
   {
   CPropertySheet* psheet = (CPropertySheet*) GetParent();   
   psheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
   return CPropertyPage::OnSetActive();
   }
