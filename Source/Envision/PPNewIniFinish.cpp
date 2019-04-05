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
// PPNewIniFinish.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "PPNewIniFinish.h"
#include "NewIniFileDlg.h"

#include <Path.h>

// PPNewIniFinish dialog

IMPLEMENT_DYNAMIC(PPNewIniFinish, CPropertyPage)

PPNewIniFinish::PPNewIniFinish(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PPNewIniFinish::IDD)
   , m_runIniEditor(FALSE)
   {

}

PPNewIniFinish::~PPNewIniFinish()
{
}

void PPNewIniFinish::DoDataExchange(CDataExchange* pDX)
{
CPropertyPage::DoDataExchange(pDX);
DDX_Check(pDX, IDC_RUNINIEDITOR, m_runIniEditor);
   }


BEGIN_MESSAGE_MAP(PPNewIniFinish, CPropertyPage)
END_MESSAGE_MAP()


// PPNewIniFinish message handlers
BOOL PPNewIniFinish::OnSetActive()
   {
   CPropertySheet* psheet = (CPropertySheet*) GetParent();   
   psheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

   NewIniFileDlg *pParent = (NewIniFileDlg*) GetParent();

   CString msg( _T(" A project file will be created at " ) );
   msg +=  pParent->m_step1.m_pathIni;

   if ( pParent->m_step1.m_pathIni.Right( 1 ).CompareNoCase("\\" ) != 0 &&  pParent->m_step1.m_pathIni.Right( 1 ).CompareNoCase("/" ) != 0  )
      msg += "\\";
   msg += pParent->m_step1.m_projectName;
   msg += ".envx";

   GetDlgItem( IDC_FILES )->SetWindowText( msg );

   return CPropertyPage::OnSetActive();
   }