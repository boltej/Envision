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
// BasemapDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "BasemapDlg.h"
#include "afxdialogex.h"


// BasemapDlg dialog

IMPLEMENT_DYNAMIC(BasemapDlg, CDialogEx)

BasemapDlg::BasemapDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(BasemapDlg::IDD, pParent)
   , m_localPath(_T(""))
   , m_wmsPath(_T(""))
   {

}

BasemapDlg::~BasemapDlg()
{
}

void BasemapDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Text(pDX, IDC_LOCALPATH, m_localPath);
DDX_Text(pDX, IDC_WMSPATH, m_wmsPath);
DDX_Control(pDX, IDC_WMSLIST, m_wmsList);
DDX_Control(pDX, IDC_WMSLAYERS, m_layerList);
   }


BEGIN_MESSAGE_MAP(BasemapDlg, CDialogEx)
   ON_BN_CLICKED(IDC_BROWSE, &BasemapDlg::OnBnClickedBrowse)
   ON_BN_CLICKED(IDC_ADDWMS, &BasemapDlg::OnBnClickedAddwms)
END_MESSAGE_MAP()


// BasemapDlg message handlers

BOOL BasemapDlg::OnInitDialog()
   {
   CDialogEx::OnInitDialog();



   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void BasemapDlg::OnBnClickedBrowse()
   {
   // TODO: Add your control notification handler code here
   }


void BasemapDlg::OnBnClickedAddwms()
   {
   // TODO: Add your control notification handler code here
   }



void BasemapDlg::OnOK()
   {
   // TODO: Add your specialized code here and/or call the base class

   CDialogEx::OnOK();
   }
