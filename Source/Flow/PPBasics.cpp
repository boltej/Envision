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
// PPBasics.cpp : implementation file
//

#include "stdafx.h"
#include "Flow.h"
#include "PPBasics.h"
#include "afxdialogex.h"


// PPBasics dialog

IMPLEMENT_DYNAMIC(PPBasics, CPropertyPage)

PPBasics::PPBasics( FlowModel *pFlow )
	: CPropertyPage(PPBasics::IDD)
   , m_pFlow( pFlow )
{

}

PPBasics::~PPBasics()
{
}

void PPBasics::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(PPBasics, CPropertyPage)
   ON_EN_CHANGE(IDC_INPUTPATH, &PPBasics::OnEnChangeInputpath)
END_MESSAGE_MAP()


// PPBasics message handlers


void PPBasics::OnEnChangeInputpath()
   {
   // TODO:  If this is a RICHEDIT control, the control will not
   // send this notification unless you override the CPropertyPage::OnInitDialog()
   // function and call CRichEditCtrl().SetEventMask()
   // with the ENM_CHANGE flag ORed into the mask.

   // TODO:  Add your control notification handler code here
   }


BOOL PPBasics::OnInitDialog()
   {
   CPropertyPage::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }
