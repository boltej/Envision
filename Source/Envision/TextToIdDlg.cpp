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
// TextToIdDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "TextToIdDlg.h"
#include "afxdialogex.h"

#include "EnvDoc.h"
//#include "MapWnd.h"
#include ".\TextToIdDlg.h"

#include "mainfrm.h"

#include <direct.h>
#include <Maplayer.h>
#include <Map.h>


extern CMainFrame *gpMain;
extern CEnvDoc    *gpDoc;
//extern MapWnd     *gpMapPanel;

extern MapLayer *gpCellLayer;

// TextToIdDlg dialog

IMPLEMENT_DYNAMIC(TextToIdDlg, CDialog)

TextToIdDlg::TextToIdDlg(CWnd* pParent /*=NULL*/)
	: CDialog(TextToIdDlg::IDD, pParent)
{

}

TextToIdDlg::~TextToIdDlg()
{
}

void TextToIdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LAYERS, m_layers);
DDX_Control(pDX, IDC_TEXTFIELD, m_textField);
DDX_Control(pDX, IDC_IDFIELD, m_idField);
}


BEGIN_MESSAGE_MAP(TextToIdDlg, CDialog)
   ON_BN_CLICKED(IDOK, OnBnClickedOk)
   ON_CBN_SELCHANGE(IDC_LAYERS, &TextToIdDlg::OnCbnSelchangeLayers)
   ON_CBN_SELCHANGE(IDC_TEXTFIELD, &TextToIdDlg::OnCbnChangeTextField)
END_MESSAGE_MAP()


// TextToIdDlg message handlers



// MergeDlg message handlers
BOOL TextToIdDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   Map *pMap = gpCellLayer->GetMapPtr();
   ASSERT( pMap != NULL );

   int layerCount = pMap->GetLayerCount();

   CString msg;
   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );

      if ( pLayer->m_pDbTable != NULL )
         m_layers.AddString( pLayer->m_name );
      }

   m_layers.SetCurSel( 0 );

   
   CWnd *pAttrWnd = GetDlgItem( IDC_ATTR );
   ASSERT( pAttrWnd != NULL );
   
   RECT rect;
   pAttrWnd->GetWindowRect( &rect );
   ScreenToClient( &rect );
   pAttrWnd->ShowWindow( SW_HIDE );

   BOOL ok = m_grid.Create( rect, this, 150, WS_CHILD | WS_TABSTOP | WS_VISIBLE );
   //m_grid.m_pParent = this;
   ASSERT( ok );

   LoadFields();
   return TRUE;
   }


void TextToIdDlg::LoadFields()
   {
   m_idField.ResetContent();
   m_textField.ResetContent();

   int index = m_layers.GetCurSel();

   if ( index < 0 )
      return;

   Map *pMap = gpCellLayer->GetMapPtr();
   MapLayer *pLayer = pMap->GetLayer( index );

   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      {
      TYPE type = pLayer->GetFieldType( i );

      if ( type == TYPE_STRING || type == TYPE_DSTRING )
         m_textField.AddString( pLayer->GetFieldLabel( i ) );
         
      if ( ::IsNumeric( type ) )
         m_idField.AddString( pLayer->GetFieldLabel( i ) );
      }
   
   m_textField.SetCurSel( 0 );   
   m_idField.SetCurSel( 0 );

   OnCbnChangeTextField();

   return; 
   }


void TextToIdDlg::OnCbnSelchangeLayers()
   {
   LoadFields();
   }


void TextToIdDlg::OnCbnChangeTextField()
   {
   m_grid.DeleteAllItems();

   CString textField;
   m_textField.GetWindowTextA( textField );

   Map *pMap = gpCellLayer->GetMapPtr();
   CString layerName;
   m_layers.GetWindowText( layerName );
   MapLayer *pLayer = pMap->GetLayer( layerName );
   ASSERT( pLayer != NULL );

   int colText = pLayer->GetFieldCol( textField );
   if ( colText < 0 )
      return;

   CStringArray values;
   int count = pLayer->GetUniqueValues( colText, values );
   ASSERT( count == values.GetSize() );
   
   m_grid.SetColumnCount( 2 );
   m_grid.SetFixedRowCount( 1 );
   m_grid.SetItemText( 0, 0, "Text Attribute" );
   m_grid.SetItemText( 0, 1, "ID Value" );
   m_grid.SetColumnWidth( 0, 60 );
   m_grid.SetColumnWidth( 1, 20 );

   m_grid.SetRowCount( count+1 ); // include fixed row
   m_grid.ExpandColumnsToFit();
   m_grid.SetEditable();
   m_grid.EnableSelection( FALSE );

   for ( int i=0; i < count; i++ )
      {
      TCHAR istr[ 32 ];
      sprintf_s( istr, 32, "%i", i+1 );

      m_grid.SetItemText( i+1, 0, values[ i ] );
      m_grid.SetItemText( i+1, 1, istr );

      m_grid.SetItemState( i+1, 0, m_grid.GetItemState(i+1, 0) | GVIS_READONLY );
      }
   }


void TextToIdDlg::OnBnClickedOk()
   {
   UpdateData( 1 );

   CWaitCursor c;

   Map *pMap = gpCellLayer->GetMapPtr();
   
   // Get existing layer to merge with
   CString layerName;
   m_layers.GetWindowText( layerName );
   MapLayer *pLayer = pMap->GetLayer( layerName );
   ASSERT( pLayer != NULL );

   DbTable *pTable = pLayer->m_pDbTable;
   int recordCount = pTable->GetRecordCount();

   CString textField;
   m_textField.GetWindowText( textField );

   CString idField;
   m_idField.GetWindowText( idField );

   int textIndex = pLayer->GetFieldCol( textField );
   ASSERT( textIndex >= 0 );

   int idIndex = pLayer->GetFieldCol( idField );
   if ( idIndex < 0 )
      {
      CString msg;
      msg.Format( "The field [%s] does not exist in the coverage.  Do you want to add it?", (LPCTSTR) idField );
      int retVal = this->MessageBox( msg, "Add Field?", MB_YESNOCANCEL );

      switch( retVal )
         {
         case IDYES:
            {
            int width, decimals;
            GetTypeParams( TYPE_INT, width, decimals );
            idIndex = pLayer->m_pDbTable->AddField( idField, TYPE_INT, width, decimals, true );
            }
            break;

         case IDNO:
         case IDCANCEL:
            return;
         }
      }

   if ( idIndex < 0 )
      return;

   // special case - in place (same column) conversion.
   CStringArray textArray;
   if ( idIndex == textIndex )
      {
      if ( MessageBox( "The text field and the ID field are the same field.  IDs will overwrite the text field and the field type will be converted to an integer type.  Is this okay?",
                       "Overwrite text field?", MB_YESNO ) != IDYES )
         return;

      for ( int i=0; i < recordCount; i++ )
         {
         CString value;
         pLayer->GetData( i, textIndex, value );
         textArray.Add( value );
         }
      }

   VData nullValue;
   pLayer->SetColData( idIndex, nullValue, true );
   int width, decimals;
   GetTypeParams( TYPE_INT, width, decimals );
   pLayer->SetFieldType( idIndex, TYPE_INT, width, decimals, false ); // make sure target field is of the correct type

   int noDataValue = (int) pLayer->GetNoDataValue();

   CStringArray values;
   CMap< CString, LPCTSTR, int, int > idMap;

   // get values for mapping text to ID
   for ( int i=0; i < m_grid.GetRowCount()-1; i++ )
      {
      CString value = m_grid.GetItemText( i+1, 0 );
      values.Add( value );
      
      CString idStr = m_grid.GetItemText( i+1, 1 );
      int id = noDataValue;
      if ( idStr.GetLength() > 0 )
         id = atoi( idStr );

      idMap.SetAt( value, id );
      }

   // now, update IDUs
   for ( int i=0; i < recordCount; i++ )
      {
      CString value;

      if ( idIndex == textIndex )
         value = textArray[ i ];
      else
         pLayer->GetData( i, textIndex, value );

      int id;
      BOOL ok = idMap.Lookup( value, id );

      if ( ok )
         pLayer->SetData( i, idIndex, id );
      else
         pLayer->SetNoData( i, idIndex );
 
      if ( i % 100 == 0 )
         {
         CString msg;
         msg.Format( "Processing record %i...",  i );
         gpMain->SetStatusMsg( msg );
         }
      }  // end of:  i < recordCount

   bool fieldInfoAdded = false;

   if ( IsDlgButtonChecked( IDC_CREATEFIELDINFO ) )
      {
      MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( idField );
      if ( pInfo == NULL )
         {
         pInfo = new MAP_FIELD_INFO( idField, /*0,*/ idField, "", "", TYPE_INT, MFIT_CATEGORYBINS,
            BCF_MIXED, 0, -1,-1, true );
         fieldInfoAdded = true;
         }

      pInfo->RemoveAttributes();

      for (int i=0; i < values.GetSize(); i++ )
         {
         int id = noDataValue;
         BOOL ok = idMap.Lookup( values[ i ], id );
         if ( ok )
            {
            COLORREF color = GetColor( i, (int) values.GetSize(), BCF_MIXED );
            pInfo->AddAttribute( VData( id ), values[i], color, float(id), float(id), 0 );
            }
         }

      if ( fieldInfoAdded )
         {
         pLayer->AddFieldInfo( pInfo );
         delete pInfo;
         }
      }

   int retVal = MessageBox( "Population process completed.  Save Database?", "Save Database?", MB_YESNO );
   if ( retVal == IDYES )
      {
      char *cwd = _getcwd( NULL, 0 );

      CFileDialog dlg( FALSE, "dbf", pLayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "DBase Files|*.dbf|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         {
         CString filename( dlg.GetPathName() );

         bool useWide = VData::SetUseWideChar( true );

         pLayer->SaveDataDB( filename ); // uses DBase/CodeBase

         VData::SetUseWideChar( useWide );
 
         gpDoc->SetChanged( 0 );
         
         _chdir( cwd );
         free( cwd );
         }
      else
         gpDoc->SetChanged( CHANGED_COVERAGE );
      }

   if ( fieldInfoAdded )
      {
      CString msg;
      msg.Format( "Field Information has been generated for field [%s].  Would you like to save this to disk?", (LPCTSTR) idField );

      retVal = MessageBox( msg, "Save Field Information?", MB_YESNO );
      
      if ( retVal == IDYES )
         {
         char *cwd = _getcwd( NULL, 0 );

         CFileDialog dlg( FALSE, "xml", pLayer->m_pFieldInfoArray->m_path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
               "XML Files|*.xml|All files|*.*||");

         if ( dlg.DoModal() == IDOK )
            {
            CString filename( dlg.GetPathName() );

            gpDoc->SaveFieldInfoXML( pLayer, filename );

            _chdir( cwd );
            free( cwd );
            }
         }
      }
   //OnOK();
   }

