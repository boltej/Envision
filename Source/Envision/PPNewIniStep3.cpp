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
// PPNewIniStep3.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "PPNewIniStep3.h"
#include <DirPlaceholder.h>

// PPNewIniStep3 dialog

IMPLEMENT_DYNAMIC(PPNewIniStep3, CPropertyPage)

PPNewIniStep3::PPNewIniStep3(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PPNewIniStep3::IDD)
   , m_pathPolicy(_T(""))
{

}

PPNewIniStep3::~PPNewIniStep3()
{
}

void PPNewIniStep3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_FILE, m_pathPolicy);

}


BEGIN_MESSAGE_MAP(PPNewIniStep3, CPropertyPage)
   ON_BN_CLICKED(IDC_BROWSE, &PPNewIniStep3::OnBnClickedBrowse)
END_MESSAGE_MAP()


// PPNewIniStep3 message handlers

// PPNewIniStep2 message handlers

void PPNewIniStep3::OnBnClickedBrowse()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "shp", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Shape files|*.shp|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_pathPolicy = dlg.GetPathName();
      UpdateData( 0 );
      }
   }


BOOL PPNewIniStep3::OnSetActive()
   {
   CPropertySheet* psheet = (CPropertySheet*) GetParent();   
   psheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

   return CPropertyPage::OnSetActive();
   }
