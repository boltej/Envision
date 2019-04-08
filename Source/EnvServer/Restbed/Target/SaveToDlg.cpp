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
// SaveToDlg.cpp : implementation file
//

#include "stdafx.h"

#include "resource.h"

#include "Target.h"
#include "SaveToDlg.h"
#include "afxdialogex.h"

#include <direct.h>



// SaveToDlg dialog

IMPLEMENT_DYNAMIC(SaveToDlg, CDialogEx)

SaveToDlg::SaveToDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(SaveToDlg::IDD, pParent)
   , m_saveToFile(FALSE)
   , m_saveToSession(FALSE)
   , m_path(_T(""))
   {

}

SaveToDlg::~SaveToDlg()
{
}

void SaveToDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Check(pDX, IDC_SAVETOFILE, m_saveToFile);
DDX_Check(pDX, IDC_SAVESESSION, m_saveToSession);
DDX_Text(pDX, IDC_PATH, m_path);
   }


BEGIN_MESSAGE_MAP(SaveToDlg, CDialogEx)
   ON_BN_CLICKED(IDC_BROWSEXML, &SaveToDlg::OnBnClickedBrowsexml)
END_MESSAGE_MAP()


// SaveToDlg message handlers


void SaveToDlg::OnOK()
   {
   // TODO: Add your specialized code here and/or call the base class

   CDialogEx::OnOK();
   }


void SaveToDlg::OnBnClickedBrowsexml()
   {
   UpdateData( 1 );

   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( TRUE, NULL, m_path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                  "XML files|*.xml|All files|*.*||");
      
   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      m_path = filename;

      UpdateData( 0 );
      }

   _chdir( cwd );
   free( cwd );
   }
