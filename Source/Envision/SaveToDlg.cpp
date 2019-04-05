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
#include "Envision.h"
#include "SaveToDlg.h"
#include "afxdialogex.h"

#include <Path.h>


// SaveToDlg dialog

IMPLEMENT_DYNAMIC(SaveToDlg, CDialog)

BEGIN_MESSAGE_MAP(SaveToDlg, CDialog)
   ON_BN_CLICKED(IDC_BROWSE, &SaveToDlg::OnBnClickedBrowse)
END_MESSAGE_MAP()



SaveToDlg::SaveToDlg(LPCTSTR text, LPCTSTR defPath, CWnd* pParent /*=NULL*/)
	: CDialog(SaveToDlg::IDD, pParent)
   , m_saveThisSession(TRUE)
   , m_saveToDisk(TRUE)
   , m_text( text )
   , m_path(defPath)
   {
   }

SaveToDlg::~SaveToDlg()
{
}

void SaveToDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Check(pDX, IDC_SAVE_THIS_SESSION, m_saveThisSession);
DDX_Check(pDX, IDC_SAVE_TO_FILE, m_saveToDisk);
DDX_Text(pDX, IDC_PATH, m_path);
   }


// SaveToDlg message handlers


BOOL SaveToDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   CString text, text2;
   GetDlgItem( IDC_TEXT )->GetWindowText( text );
   text2.Format( text, (LPCTSTR) m_text, (LPCTSTR) m_text, (LPCTSTR) m_text ); 
   GetDlgItem( IDC_TEXT )->SetWindowText( text2 );

   GetDlgItem( IDC_SAVE_THIS_SESSION )->GetWindowText( text );
   text2.Format( text, (LPCTSTR) m_text ); 
   GetDlgItem( IDC_SAVE_THIS_SESSION )->SetWindowText( text2 );

   GetDlgItem( IDC_SAVE_TO_FILE )->GetWindowText( text );
   text2.Format( text, (LPCTSTR) m_text ); 
   GetDlgItem( IDC_SAVE_TO_FILE )->SetWindowText( text2 );

   return TRUE;  // return TRUE unless you set the focus to a control
   }


void SaveToDlg::OnBnClickedBrowse()
   {
   UpdateData( 1 );

   nsPath::CPath path( m_path );
   CString ext = path.GetExtension();
   CString extensions;

   if (ext.CompareNoCase( _T("envx") ) == 0 )
      extensions = _T("ENVX (Project) Files|*.envx|XML Files|*.xml|All files|*.*||");
   else
      extensions = _T("XML Files|*.xml|ENVX (Project) Files|*.envx|All files|*.*||");

   CFileDialog dlg( FALSE, ext, m_path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, extensions);

   int retVal = (int) dlg.DoModal();
   
   if ( retVal == IDOK )
      {
      m_path = dlg.GetPathName();
      UpdateData( 0 );
      }
   }
