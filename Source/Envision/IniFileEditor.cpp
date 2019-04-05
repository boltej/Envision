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
// IniFileEditor.cpp : implementation file
//

#include "stdafx.h"
#include "IniFileEditor.h"
#include "EnvDoc.h"
#include <Scenario.h>
#include <Actor.h>
#include <Policy.h>
#include <map.h>
#include <maplayer.h>
#include "SaveProjectDlg.h"
#include <iostream>

extern ScenarioManager *gpScenarioManager;
extern ActorManager    *gpActorManager;
extern CEnvDoc         *gpDoc;
extern MapLayer        *gpCellLayer;


// IniFileEditor dialog

IMPLEMENT_DYNAMIC(IniFileEditor, CDialog)

IniFileEditor::IniFileEditor(CWnd* pParent /*=NULL*/)
	: CDialog(IniFileEditor::IDD, pParent)
   , m_basic( this )
   , m_settings( this )
   , m_layers( this )
   , m_metagoals( this )
   , m_models( this )
   , m_processes( this )
   , m_isDirty( 0 )
   , m_tabCtrl()
{ }


IniFileEditor::~IniFileEditor()
{ }


void IniFileEditor::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_TAB, m_tabCtrl);
}


BEGIN_MESSAGE_MAP(IniFileEditor, CDialog)
   ON_BN_CLICKED(IDC_SAVEAS, &IniFileEditor::OnBnClickedSaveas)
   ON_BN_CLICKED(IDC_SAVE, &IniFileEditor::OnBnClickedSave)
END_MESSAGE_MAP()


// IniFileEditor message handlers

BOOL IniFileEditor::OnInitDialog()
   {
   CDialog::OnInitDialog();

   CenterWindow();
   m_isDirty = false;
   
   GetDlgItem( IDC_PATH )->SetWindowText( CEnvDoc::m_iniFile );
   
  // Setup the tab control
   int nPageID = 0;
   m_basic.Create( PPIniBasic::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Basics"), nPageID++, &m_basic );

   m_settings.Create( PPIniSettings::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Settings"), nPageID++, &m_settings );

   m_layers.Create( PPIniLayers::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Map Layers" ), nPageID++, &m_layers );

   m_metagoals.Create( PPIniMetagoals::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Metagoals"), nPageID++, &m_metagoals );

   m_models.Create( PPIniModels::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Evaluative Models"), nPageID++, &m_models );

   m_processes.Create( PPIniProcesses::IDD, this );
   m_tabCtrl.AddSSLPage(_T("Autonomous Processess"), nPageID++, &m_processes );

   GetDlgItem( IDC_SAVE )->EnableWindow( 0 );

   return TRUE;
   }


bool IniFileEditor::StoreChanges()
   {
   UpdateData( TRUE );  // update local variables

   // Validate entered data
   bool ok;

   if ( IsDirty( INI_BASIC ) )
      {
      ok = m_basic.StoreChanges();
      if ( ! ok )
         return false;
      }

   if ( IsDirty( INI_SETTINGS ) )
      {
      ok = m_settings.StoreChanges();
      if ( ! ok )
         return false;
      }

   if ( IsDirty( INI_LAYERS ) )
      {
      ok = m_layers.StoreChanges();
      if ( ! ok )
         return false;
      }

   if ( IsDirty( INI_METAGOALS ) )
      {
      ok = m_metagoals.StoreChanges();
      if ( ! ok )
         return false;
      }

   if ( IsDirty( INI_MODELS ) )
      {
      ok = m_models.StoreChanges();
      if ( ! ok )
         return false;
      }


   if ( IsDirty( INI_PROCESSES ) )
      {
      ok = m_processes.StoreChanges();
      if ( ! ok )
         return false;
      }

   m_isDirty = 0;
   return true;
   }


bool IniFileEditor::SaveAs()
   {
   StoreChanges();

   char *cwd = _getcwd( NULL, 0 );
   bool result = false;

   SaveProjectDlg dlg;
   //CFileDialog dlg( FALSE, "envx", CEnvDoc::m_iniFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
   //   "ENVX Files|*.envx|All files|*.*||");
   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.m_projectPath );
      result = gpDoc->SaveAs( filename ) != 0 ? true : false;
      }

   _chdir( cwd );
   free( cwd );

   return result;
   }


void IniFileEditor::OnOK()
   {
   bool goAhead = true;
//   if ( m_isDirty )
 //     goAhead = StoreChanges();

   m_isDirty = 0;
   return CDialog::OnOK();
   }


// Close
void IniFileEditor::OnCancel()
   {
   if ( m_isDirty )
      {
      int result = MessageBox( "Do you want to save changes to this Project file?", "Save Changes?", MB_YESNOCANCEL );

      switch( result )
         {
         case IDYES:
            SaveAs();
            break;

         case IDNO:
            break;

         case IDCANCEL:
            return;
         }
      }

   m_isDirty = 0;
   CDialog::OnCancel();
   }

void IniFileEditor::OnBnClickedSaveas()
   {
   SaveAs();
   m_isDirty = 0;
   }

void IniFileEditor::OnBnClickedSave()
   {
   gpDoc->SaveAs( gpDoc->m_iniFile );
   m_isDirty = 0;
   }
