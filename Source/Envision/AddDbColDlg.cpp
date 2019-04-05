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
// AddDbColDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include ".\adddbcoldlg.h"
#include "EnvDoc.h"

#include <maplayer.h>
#include <map.h>

extern MapLayer *gpCellLayer;
extern CEnvDoc *gpDoc;


// AddDbColDlg dialog

IMPLEMENT_DYNAMIC(AddDbColDlg, CDialog)
AddDbColDlg::AddDbColDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AddDbColDlg::IDD, pParent)
{
}

AddDbColDlg::~AddDbColDlg()
{
}

void AddDbColDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_TYPE, m_type);
DDX_Control(pDX, IDC_WIDTH, m_width);
DDX_Control(pDX, IDC_LAYERS, m_layers);
   }


BEGIN_MESSAGE_MAP(AddDbColDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
   ON_CBN_SELCHANGE(IDC_TYPE, OnCbnSelchangeType)
   ON_CBN_SELCHANGE(IDC_LAYERS, &AddDbColDlg::OnCbnSelchangeLayers)
END_MESSAGE_MAP()


// AddDbColDlg message handlers

void AddDbColDlg::OnBnClickedAdd()
   {
   int layer = m_layers.GetCurSel();
   ASSERT( layer >= 0 );
   Map *pMap = gpCellLayer->GetMapPtr();

   MapLayer *pLayer = pMap->GetLayer( layer );
   ASSERT( pLayer != NULL );

   // name
   CString name;
   GetDlgItemText( IDC_NAME, name );

   if ( name.IsEmpty() )
      {
      MessageBeep(0);
      return;
      }

   if ( name.GetLength() > 12 )
      {
      MessageBox( "Name can't be longer than 12 characters" );
      return;
      }

   if ( pLayer->GetFieldCol( name ) >= 0 )
      {
      MessageBox( "Column already exists - please specify a different column name" );
      return;
      }

   // type
   int item = m_type.GetCurSel();
   TYPE type = (TYPE) m_type.GetItemData( item );

   int width, decimals;
   GetTypeParams( type, width, decimals );

   if ( ::IsString( type ) )
      width = GetDlgItemInt( IDC_WIDTH );

   CWaitCursor c;

   int col = pLayer->m_pDbTable->AddField( name, type, width, decimals, true );

   if ( col >= 0 )
      {
      MessageBox( "Successfully added column" );
      gpDoc->SetChanged( CHANGED_COVERAGE );
      }
   else
      MessageBox( "Unable to add column" );
   }


BOOL AddDbColDlg::OnInitDialog()
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

   EnableCtrls();
 
   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void AddDbColDlg::EnableCtrls()
   {
   int index = m_type.GetCurSel();
   DWORD_PTR data = m_type.GetItemData( index );
   TYPE type = (TYPE) data;

   switch( type )
      {
      case TYPE_DSTRING:  // string
      case TYPE_STRING:  
         m_width.EnableWindow( TRUE );
         break;

      default:
         m_width.EnableWindow( FALSE );
      }  
   }

void AddDbColDlg::OnCbnSelchangeType()
   {
   EnableCtrls();
   }


void AddDbColDlg::OnCbnSelchangeLayers()
   {

   }
