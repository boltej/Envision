// ProjectPathDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EnvServer.h"
#include "ProjectPathDlg.h"
#include "afxdialogex.h"


// ProjectPathDlg dialog

IMPLEMENT_DYNAMIC(ProjectPathDlg, CDialogEx)

ProjectPathDlg::ProjectPathDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ENVXFILE, pParent)
   , m_path(_T(""))
   {

}

ProjectPathDlg::~ProjectPathDlg()
{
}

void ProjectPathDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialogEx::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_EDIT1, m_path);
   }


BEGIN_MESSAGE_MAP(ProjectPathDlg, CDialogEx)
   ON_BN_CLICKED(IDC_BROWSE, &ProjectPathDlg::OnBnClickedBrowse)
   ON_BN_CLICKED(IDOK, &ProjectPathDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// ProjectPathDlg message handlers


void ProjectPathDlg::OnBnClickedBrowse()
   {




   }


void ProjectPathDlg::OnBnClickedOk()
   {
   // TODO: Add your control notification handler code here
   CDialogEx::OnOK();
   }
