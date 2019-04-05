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
// AreaQueryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "AreaQueryDlg.h"
#include ".\areaquerydlg.h"
#include <maplayer.h>

extern MapLayer *gpCellLayer;


// AreaQueryDlg dialog

IMPLEMENT_DYNAMIC(AreaQueryDlg, CDialog)
AreaQueryDlg::AreaQueryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AreaQueryDlg::IDD, pParent)
{
}

AreaQueryDlg::~AreaQueryDlg()
{
}

void AreaQueryDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Control(pDX, IDC_VALUE, m_value);
}


BEGIN_MESSAGE_MAP(AreaQueryDlg, CDialog)
END_MESSAGE_MAP()


// AreaQueryDlg message handlers

BOOL AreaQueryDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   for ( int i=0; i < gpCellLayer->GetFieldCount(); i++ )
      m_fields.AddString( gpCellLayer->GetFieldLabel( i ) );

   m_fields.SetCurSel( 0 );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void AreaQueryDlg::OnOK()
   {
   //int index = m_fields.GetCurSel();
   CString label;
   m_fields.GetWindowText( label );

   m_col = gpCellLayer->GetFieldCol( label );
   ASSERT( m_col >= 0 );

   TYPE type = gpCellLayer->GetFieldType( m_col );

   CString value;
   m_value.GetWindowText( value );

   switch( type )
      {
      case TYPE_CHAR:
      case TYPE_BOOL:
      case TYPE_UINT:
      case TYPE_INT:
      case TYPE_ULONG:
      case TYPE_LONG:
      case TYPE_SHORT:
         m_dataValue = atoi( value );
         break;

      case TYPE_FLOAT:
         m_dataValue = (float) atof( value );
         break;

      case TYPE_DOUBLE:
         m_dataValue = atof( value );
         break;

      case TYPE_STRING:
      case TYPE_DSTRING:
         m_dataValue = value;
         break;

      default: 
         ASSERT( 0 );
      }

   CDialog::OnOK();
   }
