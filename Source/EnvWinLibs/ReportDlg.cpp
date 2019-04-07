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
// Report.cpp : implementation file
//

#include "winlibs.h"
#include "Report.h"
#include "afxdialogex.h"
#include "afxDesktopAlertWnd.h"

///////////////////////////////////////////////////////////////////////////

// ReportBox dialog

IMPLEMENT_DYNAMIC(ReportBox, CDialogEx)

ReportBox::ReportBox(CWnd* pParent /*=NULL*/)
	: CDialogEx(ReportBox::IDD, pParent)
   , m_msg(_T(""))
   , m_output(FALSE)
   , m_noShow(FALSE)
   {

}

ReportBox::~ReportBox()
{
}

void ReportBox::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Text(pDX, IDC_LIB_MSG, m_msg);
DDX_Check(pDX, IDC_LIB_OUTPUT, m_output);
DDX_Check(pDX, IDC_LIB_NOMSGS, m_noShow);
   }


BEGIN_MESSAGE_MAP(ReportBox, CDialogEx)
END_MESSAGE_MAP()


// ReportBox message handlers


void ReportBox::OnOK()
   {
   UpdateData( 1 );

   CDialogEx::OnOK();
   }


BOOL ReportBox::OnInitDialog()
   {
   CDialogEx::OnInitDialog();

   this->SetWindowText( this->m_hdr );

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }
