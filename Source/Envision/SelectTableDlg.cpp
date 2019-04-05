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
// SelectTableDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SelectTableDlg.h"
#include <Report.h>
// SelectTableDlg dialog

IMPLEMENT_DYNAMIC(SelectTableDlg, CDialog)

SelectTableDlg::SelectTableDlg( LPCTSTR mdbFile, CWnd* pParent /*=NULL*/)
	: CDialog(SelectTableDlg::IDD, pParent)
   , m_mdbFile( mdbFile )
{ }

SelectTableDlg::~SelectTableDlg()
{ }


void SelectTableDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LIST1, m_tables);
}


BEGIN_MESSAGE_MAP(SelectTableDlg, CDialog)
END_MESSAGE_MAP()


// SelectTableDlg message handlers


BOOL SelectTableDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

#ifndef _WIN64

   CDaoDatabase database;

	try
		{
      // open the database ( non-exclusive, read-only )
		database.Open( m_mdbFile, FALSE, TRUE, "" );

      int count = database.GetTableDefCount();

      for ( int i=0; i < count; i++ )
         {
         CDaoTableDefInfo tbInfo;
         database.GetTableDefInfo( i, tbInfo );

         m_tables.AddString( tbInfo.m_strName );
         }
      }

	catch( CDaoException *p )
		{
		CString msg("Error opening database " );
      msg += m_mdbFile;
      msg += ":  ";
      msg += p->m_pErrorInfo->m_strDescription;
		Report::ErrorMsg( msg );
		}

   database.Close();
#endif // _WIN64

   m_tables.SetCurSel( 0 );

   return TRUE;  // return TRUE unless you set the focus to a control
   }


void SelectTableDlg::OnOK()
   {
   int index = m_tables.GetCurSel();

   if ( index < 0 )
      m_table = _T("");
   else
      m_tables.GetText( index, m_table );

   CDialog::OnOK();
   }
