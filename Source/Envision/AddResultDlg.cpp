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
// AddResultDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AddResultDlg.h"

#include <maplayer.h>
#include <EnvModel.h>

extern MapLayer *gpCellLayer;
extern EnvModel  *gpModel;


// AddResultDlg dialog

IMPLEMENT_DYNAMIC(AddResultDlg, CDialog)

AddResultDlg::AddResultDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AddResultDlg::IDD, pParent)
{

}

AddResultDlg::~AddResultDlg()
{
}

void AddResultDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Control(pDX, IDC_VALUES, m_values);
DDX_Control(pDX, IDC_BUFFEREDIT, m_bufferFn);
DDX_Control(pDX, IDC_ADDPOINTEDIT, m_addPointFn);
DDX_Control(pDX, IDC_INCREMENTEDIT, m_incrementFn);
DDX_Control(pDX, IDC_EXPANDEDIT, m_expandFn);
DDX_Control(pDX, IDC_FIELD, m_field);
}


BEGIN_MESSAGE_MAP(AddResultDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_FIELDS, &AddResultDlg::OnCbnSelchangeFields)
   ON_BN_CLICKED(IDC_BUFFER, &AddResultDlg::OnBnClickedBuffer)
   ON_BN_CLICKED(IDC_ADDPOINT, &AddResultDlg::OnBnClickedAddpoint)
   ON_BN_CLICKED(IDC_INCREMENT, &AddResultDlg::OnBnClickedIncrement)
   ON_BN_CLICKED(IDC_EXPAND, &AddResultDlg::OnBnClickedExpand)
   ON_BN_CLICKED(IDC_FIELD, &AddResultDlg::OnBnClickedField)
END_MESSAGE_MAP()


// AddResultDlg message handlers

BOOL AddResultDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();
   LoadFieldInfo();

   m_bufferFn.SetWindowText( "Buffer( <distance>, <query>, <Inside Buffer Outcome>, <Outside Buffer Outcome>" );
   m_addPointFn.SetWindowText( "AddPoint( <layerName>, <x>, <y> )" );
   m_incrementFn.SetWindowText( "Increment( <fieldName>, <incrementValue> )" );
   m_expandFn.SetWindowText( "Expand( <constraint query>, <max area>, <outcome> )" );

   CheckDlgButton( IDC_FIELD, 1 );
   m_bufferFn.EnableWindow( 0 );
   m_addPointFn.EnableWindow( 0 );
   m_incrementFn.EnableWindow( 0 );
   m_expandFn.EnableWindow( 0 );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void AddResultDlg::LoadFieldInfo()
   {
   m_fields.ResetContent();
   m_values.ResetContent();

   for ( int i=0; i < gpCellLayer->GetFieldInfoCount(1); i++ )
      {
      MAP_FIELD_INFO *pInfo = gpCellLayer->GetFieldInfo( i );

      if ( pInfo->mfiType != MFIT_SUBMENU )
         {
         bool outcome = pInfo->GetExtraBool( 4 );
  
         if ( outcome )
            m_fields.AddString( pInfo->fieldname );
         }
      }

   int index = m_fields.FindString( -1, gpModel->m_lulcTree.GetFieldName( 1 ) );
   m_fields.SetCurSel( index );

   LoadValues();
   }


void AddResultDlg::OnCbnSelchangeFields()
   {
   LoadValues();
   }


void AddResultDlg::LoadValues()
   {
   int index = m_fields.GetCurSel();
   if ( index < 0 )
      return;

   char fieldName[ 64 ];
   m_fields.GetLBText( index, fieldName );
   SetDlgItemText( IDC_FIELDDESCRIPTION, "" );

   m_values.ResetContent();
   int colCellField = gpCellLayer->GetFieldCol( fieldName );
   ASSERT( colCellField >= 0 );

   // check field info
   MAP_FIELD_INFO *pInfo = gpCellLayer->FindFieldInfo( fieldName );

   if ( pInfo != NULL )  // found field info.  get attributes to populate box
      {
      SetDlgItemText( IDC_FIELDDESCRIPTION, pInfo->label );

      for ( int i=0; i < pInfo->attributes.GetSize(); i++ )
         {
         FIELD_ATTR &attr = pInfo->GetAttribute( i );

         CString value;
         value.Format( "%s {%s}", attr.value.GetAsString(), attr.label );

         m_values.AddString( value );
         }

      m_values.SetCurSel( 0 );
      }
   else
      {  // no entries in the database.  If it is a string type, get uniue values from the database
      switch( gpCellLayer->GetFieldType( colCellField ) )
         {
         case TYPE_STRING:
         case TYPE_DSTRING:
            {
            CStringArray valueArray;
            gpCellLayer->GetUniqueValues( colCellField, valueArray );
            for ( int i=0; i < valueArray.GetSize(); i++ )
               m_values.AddString( valueArray[ i ] );
            }
            m_values.SetCurSel( 0 );
            return;

         case TYPE_BOOL:
            m_values.AddString( "0 {False}" );
            m_values.AddString( "1 {True}" );
            m_values.SetCurSel( 0 );
            return;
         }

      // at this point, we have no attributes defined in the database, and the field is not a string
      // field or a boolean field.  No further action required - user has to type an entry into the
      // combo box     
      }
   }


void AddResultDlg::OnOK()
   {
   if ( IsDlgButtonChecked( IDC_BUFFER ) )
      m_bufferFn.GetWindowText( m_result );

   else if ( IsDlgButtonChecked( IDC_ADDPOINT ) )
      m_addPointFn.GetWindowText( m_result );

   else if ( IsDlgButtonChecked( IDC_INCREMENT ) )
      m_incrementFn.GetWindowText( m_result );

   else if ( IsDlgButtonChecked( IDC_EXPAND ) )
      m_expandFn.GetWindowText( m_result );

   else
      {
      CString field, value;
      m_fields.GetWindowText( field );
      m_values.GetWindowText( value );
      m_result = field + "=" + value;
      }

   CDialog::OnOK();
   }

void AddResultDlg::OnBnClickedBuffer()
   {
   m_bufferFn.EnableWindow( 1 );
   m_addPointFn.EnableWindow( 0 );
   m_incrementFn.EnableWindow( 0 );
   m_expandFn.EnableWindow( 0 );
   m_fields.EnableWindow( 0 );
   m_values.EnableWindow( 0 );
   }

void AddResultDlg::OnBnClickedAddpoint()
   {
   m_bufferFn.EnableWindow( 0 );
   m_addPointFn.EnableWindow( 1 );
   m_incrementFn.EnableWindow( 0 );
   m_expandFn.EnableWindow( 0 );
   m_fields.EnableWindow( 0 );
   m_values.EnableWindow( 0 );
   }

void AddResultDlg::OnBnClickedIncrement()
   {
   m_bufferFn.EnableWindow( 0 );
   m_addPointFn.EnableWindow( 0 );
   m_incrementFn.EnableWindow( 1 );
   m_expandFn.EnableWindow( 0 );
   m_fields.EnableWindow( 0 );
   m_values.EnableWindow( 0 );
   }

void AddResultDlg::OnBnClickedExpand()
   {
   m_bufferFn.EnableWindow( 0 );
   m_addPointFn.EnableWindow( 0 );
   m_incrementFn.EnableWindow( 0 );
   m_expandFn.EnableWindow( 1 );
   m_fields.EnableWindow( 0 );
   m_values.EnableWindow( 0 );
   }

void AddResultDlg::OnBnClickedField()
   {
   m_bufferFn.EnableWindow( 0 );
   m_addPointFn.EnableWindow( 0 );
   m_incrementFn.EnableWindow( 0 );
   m_expandFn.EnableWindow( 0 );
   m_fields.EnableWindow( 1 );
   m_values.EnableWindow( 1 );
   }
