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
// RemoveDbColsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RemoveDbColsDlg.h"

#include "EnvDoc.h"
#include "MapPanel.h"
#include <map.h>
#include <maplayer.h>

#include <direct.h>


extern CEnvDoc    *gpDoc;
extern MapPanel   *gpMapPanel;

// RemoveDbColsDlg dialog

IMPLEMENT_DYNAMIC(RemoveDbColsDlg, CDialog)

RemoveDbColsDlg::RemoveDbColsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(RemoveDbColsDlg::IDD, pParent)
{ }

RemoveDbColsDlg::~RemoveDbColsDlg()
{ }

void RemoveDbColsDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LAYERS, m_layers);
DDX_Control(pDX, IDC_FIELDS, m_fields);
}


BEGIN_MESSAGE_MAP(RemoveDbColsDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_LAYERS, &RemoveDbColsDlg::OnCbnSelchangeLayers)
END_MESSAGE_MAP()


// RemoveDbColsDlg message handlers

// MergeDlg message handlers
BOOL RemoveDbColsDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   int layerCount = gpMapPanel->m_pMap->GetLayerCount();

   CString msg;
   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( i );

      if ( pLayer->m_pDbTable != NULL )
         m_layers.AddString( pLayer->m_name );
      }
   m_layers.SetCurSel( 0 );

   LoadFields();
   return TRUE;
   }


void RemoveDbColsDlg::LoadFields()
   {
   m_fields.ResetContent();

   int index = m_layers.GetCurSel();

   if ( index < 0 )
      return;

   MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( index );

   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      m_fields.AddString( pLayer->GetFieldLabel( i ) );
   
   m_fields.SetCurSel( 0 );   

   return; 
   }


void RemoveDbColsDlg::OnCbnSelchangeLayers()
   {
   LoadFields();
   }


void RemoveDbColsDlg::OnOK()
   {
   CWaitCursor c;

   // Get existing layer to merge with
   int index = m_layers.GetCurSel();

   if ( index < 0 )
      return;

   MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( index );

   DbTable *pTable = pLayer->m_pDbTable;

   for ( int i=0; i < m_fields.GetCount(); i++ )
      {
      if ( m_fields.GetCheck( i ) == 1 )
         {
         CString field;
         m_fields.GetText( i, field );
         pTable->RemoveField( field );
         }
      }

   MessageBox( "Column removal completed" );

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
      }
   else
      gpDoc->SetChanged( CHANGED_COVERAGE );

   _chdir( cwd );
   free( cwd );
   }


