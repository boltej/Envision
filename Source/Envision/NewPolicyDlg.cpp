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
// NewPolicyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "NewPolicyDlg.h"
#include ".\newpolicydlg.h"


// NewPolicyDlg dialog

IMPLEMENT_DYNAMIC(NewPolicyDlg, CDialog)
NewPolicyDlg::NewPolicyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(NewPolicyDlg::IDD, pParent)
   , m_description(_T(""))
   , m_isActorPolicy(FALSE)
   {
}

NewPolicyDlg::~NewPolicyDlg()
{
}

void NewPolicyDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Text(pDX, IDC_DESCRIPTION, m_description);
DDX_Radio(pDX, IDC_ACTORPOLICY, m_isActorPolicy);
}


BEGIN_MESSAGE_MAP(NewPolicyDlg, CDialog)
END_MESSAGE_MAP()


// NewPolicyDlg message handlers

BOOL NewPolicyDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   CheckDlgButton( IDC_ACTORPOLICY, 1 );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void NewPolicyDlg::OnOK()
   {
   UpdateData( TRUE );
   CDialog::OnOK();
   }
