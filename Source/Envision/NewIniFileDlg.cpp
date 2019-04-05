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
h// NewIniFileDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "NewIniFileDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// NewIniFileDlg

IMPLEMENT_DYNAMIC(NewIniFileDlg, CPropertySheet)

NewIniFileDlg::NewIniFileDlg(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}


NewIniFileDlg::NewIniFileDlg(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
   {
   AddPage( &m_step1 );
   AddPage( &m_step2 );
//   AddPage( &m_step3 );
   AddPage( &m_finish );

   SetWizardMode();
   }


BEGIN_MESSAGE_MAP(NewIniFileDlg, CPropertySheet)
   ON_COMMAND (ID_APPLY_NOW, OnApplyNow)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// NewIniFileDlg message handlers


void NewIniFileDlg::OnApplyNow() 
   {
	//SaveSetup();
   }
