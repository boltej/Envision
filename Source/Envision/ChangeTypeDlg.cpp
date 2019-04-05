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
// ChangeTypeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "Envision.h"
#include "ChangeTypeDlg.h"
#include "afxdialogex.h"

#include "EnvDoc.h"

#include <Maplayer.h>
#include <MAP.h>
#include <DirPlaceholder.h>

extern MapLayer *gpCellLayer;
extern CEnvDoc *gpDoc;

// ChangeTypeDlg dialog

IMPLEMENT_DYNAMIC(ChangeTypeDlg, CDialog)

ChangeTypeDlg::ChangeTypeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ChangeTypeDlg::IDD, pParent)
   , m_oldType(_T(""))
   , m_width(0)
   , m_format( VData::m_formatString )
   , m_useFormat(FALSE)
   { }

ChangeTypeDlg::~ChangeTypeDlg()
{ }

void ChangeTypeDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LAYERS, m_layers);
DDX_Control(pDX, IDC_FIELD, m_fields);
DDX_Text(pDX, IDC_CURRENTTYPE, m_oldType);
DDX_Control(pDX, IDC_TYPE, m_type);
DDX_Text(pDX, IDC_WIDTH, m_width);
DDX_Text(pDX, IDC_FORMAT, m_format);
DDX_Check(pDX, IDC_USEFORMAT, m_useFormat);
   }


BEGIN_MESSAGE_MAP(ChangeTypeDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_TYPE, &ChangeTypeDlg::OnCbnSelchangeType)
   ON_CBN_SELCHANGE(IDC_LAYERS, &ChangeTypeDlg::OnCbnSelchangeLayers)
   ON_CBN_SELCHANGE(IDC_FIELDS, &ChangeTypeDlg::OnCbnSelchangeFields)
END_MESSAGE_MAP()


// ChangeTypeDlg message handlers


BOOL ChangeTypeDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   m_type.AddString( "Character" );
   m_type.AddString( "Boolean" );
   m_type.AddString( "Short" );   
   m_type.AddString( "Integer" );
   m_type.AddString( "Long" );
   m_type.AddString( "Float" );
   m_type.AddString( "Double" );
   m_type.AddString( "String" );
   m_type.SetItemData( 0, (DWORD_PTR) TYPE_CHAR );
   m_type.SetItemData( 1, (DWORD_PTR) TYPE_BOOL );
   m_type.SetItemData( 2, (DWORD_PTR) TYPE_SHORT );
   m_type.SetItemData( 3, (DWORD_PTR) TYPE_INT );
   m_type.SetItemData( 4, (DWORD_PTR) TYPE_LONG );
   m_type.SetItemData( 5, (DWORD_PTR) TYPE_FLOAT );
   m_type.SetItemData( 6, (DWORD_PTR) TYPE_DOUBLE );
   m_type.SetItemData( 7, (DWORD_PTR) TYPE_STRING );

   m_type.SetCurSel( 2 );

   Map *pMap = gpCellLayer->GetMapPtr();

   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      m_layers.AddString( pMap->GetLayer( i )->m_name );

   m_layers.SetCurSel( 0 );

   LoadFields();

   EnableCtrls();
 
   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void ChangeTypeDlg::LoadFields( void )
   {
   m_fields.ResetContent();

   int layer = m_layers.GetCurSel();

   if ( layer < 0 )
      return;

   Map *pMap = gpCellLayer->GetMapPtr();
   MapLayer *pLayer = pMap->GetLayer( layer );

   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      m_fields.AddString( pLayer->GetFieldLabel( i ) );

   m_fields.SetCurSel( 0 );

   LoadField();
   }

void ChangeTypeDlg::LoadField( void )
   {
   SetDlgItemText( IDC_CURRENTTYPE, _T("") );
   
   int layer = m_layers.GetCurSel();

   if ( layer < 0 )
      return;

   Map *pMap = gpCellLayer->GetMapPtr();
   MapLayer *pLayer = pMap->GetLayer( layer );

   CString field;
   m_fields.GetWindowText( field );

   int col = pLayer->GetFieldCol( field );
   if ( col < 0 )
      return;

   TYPE type = pLayer->GetFieldType( col );
   CString msg( "Existing type is: " );

   msg += ::GetTypeLabel( type );
   SetDlgItemText( IDC_CURRENTTYPE, msg );

   int index = -1;
   switch( type )
      {
      case TYPE_CHAR:    index = 0;  break;
      case TYPE_BOOL:    index = 1;  break;
      case TYPE_SHORT:   index = 2;  break;
      case TYPE_INT:     index = 3;  break;
      case TYPE_LONG:    index = 4;  break;
      case TYPE_FLOAT:   index = 5;  break;
      case TYPE_DOUBLE:  index = 6;  break;
      case TYPE_STRING:  index = 7;  break;
      case TYPE_DSTRING: index = 7;  break;
      }

   ASSERT( type >= 0 );
   m_type.SetCurSel( index );
   EnableCtrls();
   }


void ChangeTypeDlg::EnableCtrls()
   {
   UpdateData( 1 );

   int index = m_type.GetCurSel();
   DWORD_PTR data = m_type.GetItemData( index );
   TYPE type = (TYPE) data;   // this is the convert TO type;

   switch( type )
      {
      case TYPE_DSTRING:  // string
      case TYPE_STRING:  
         GetDlgItem( IDC_WIDTH )->EnableWindow( TRUE );
         break;

      default:
         GetDlgItem( IDC_WIDTH )->EnableWindow( FALSE );
      }  

   int layer = m_layers.GetCurSel();
   Map *pMap = gpCellLayer->GetMapPtr();
   MapLayer *pLayer = pMap->GetLayer( layer );

   CString field;
   m_fields.GetWindowText( field );

   int col = pLayer->GetFieldCol( field );
   TYPE oldType = pLayer->GetFieldType( col );

   BOOL enableUseFormat = ( ( oldType == TYPE_FLOAT || oldType == TYPE_DOUBLE ) && IsString( type ) ) ? TRUE : FALSE;
   BOOL enableFormat = ( enableUseFormat && m_useFormat ) ? TRUE : FALSE;

   GetDlgItem( IDC_USEFORMAT )->EnableWindow( enableUseFormat );
   GetDlgItem( IDC_FORMAT )->EnableWindow( enableFormat );
   }


void ChangeTypeDlg::OnCbnSelchangeType()
   {
   EnableCtrls();
   }

void ChangeTypeDlg::OnCbnSelchangeLayers()
   {
   LoadFields();
   }

void ChangeTypeDlg::OnCbnSelchangeFields()
   {
   LoadField();
   }


void ChangeTypeDlg::OnOK()
   {
   UpdateData( 1 );

   int layer = m_layers.GetCurSel();

   if ( layer < 0 )
      return;

   Map *pMap = gpCellLayer->GetMapPtr();
   MapLayer *pLayer = pMap->GetLayer( layer );

   CString field;
   m_fields.GetWindowText( field );

   int col = pLayer->GetFieldCol( field );
   if ( col < 0 )
      return;

   int index = m_type.GetCurSel();
   TYPE type = (TYPE) m_type.GetItemData( index );

   ASSERT( type != TYPE_NULL );

   if ( ! ::IsString( type ) && type == pLayer->GetFieldType( col ) )
      return;

   int width, decimals;
   GetTypeParams( type, width, decimals );
   if ( ::IsString( type ) )
      width = m_width;

   // set up formatting if needed for Setting field type
   CString oldFormat( VData::m_formatString );
   if ( m_useFormat )
      {
      VData::m_useFormatString = true;
      VData::SetFormatString( m_format );
      }

   pLayer->SetFieldType( col, type, width, decimals, true );

   if ( m_useFormat )
      {
      VData::m_useFormatString = false;
      VData::SetFormatString( oldFormat );
      }

   DirPlaceholder d;
   CFileDialog dlg( FALSE, "dbf", pLayer->m_tableName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "DBase Files|*.dbf|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );

      bool useWide = VData::SetUseWideChar( true );

      pLayer->SaveDataDB( filename );

      VData::SetUseWideChar( useWide );
      gpDoc->SetChanged( 0 );
      }
   }
