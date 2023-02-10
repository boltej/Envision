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
// SplashDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include ".\splashdlg.h"
#include <Scenario.h>
#include "IniFileEditor.h"
#include "NewIniFileDlg.h"
#include "EnvDoc.h"

#include <EnvLoader.h>
#include "MapPanel.h"
#include <Path.h>
#include <DirPlaceholder.h>


#include <direct.h>

extern CEnvDoc *gpDoc;
extern ScenarioManager *gpScenarioManager;
extern MapPanel *gpMapPanel;

// SplashDlg dialog

IMPLEMENT_DYNAMIC(SplashDlg, CDialog)
SplashDlg::SplashDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SplashDlg::IDD, pParent)
   , m_name(_T(""))
   , m_filename(_T(""))
   , m_isNewProject( false )
   , m_runIniEditor( false )
   {
   }

SplashDlg::~SplashDlg()
{
}

void SplashDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Text(pDX, IDC_NAME, m_name);
DDX_Control(pDX, IDC_FILE, m_files);
DDX_CBString(pDX, IDC_FILE, m_filename);
DDX_Control(pDX, IDCANCELBUTTON, m_cancelButton);
}


BEGIN_MESSAGE_MAP(SplashDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_FILE, OnCbnSelchangeFile)
   ON_BN_CLICKED(IDCANCELBUTTON, OnCancel)
   ON_COMMAND(30000, &SplashDlg::OnStartup)
   ON_BN_CLICKED(IDC_NEW, &SplashDlg::OnBnClickedNew)
   ON_BN_CLICKED(IDC_OPEN, &SplashDlg::OnBnClickedOpen)
END_MESSAGE_MAP()


// SplashDlg message handlers
BOOL SplashDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   CenterWindow();

   //m_cancelButton.AutoLoad( IDCANCELBUTTON, this );
   m_cancelButton.LoadBitmaps( "CANCELU" );

   // get values from registry
   HKEY key;
   LONG result = RegOpenKeyEx( HKEY_CURRENT_USER,
               "SOFTWARE\\OSU Biosystems Analysis Group\\Envision",
               0, KEY_QUERY_VALUE, &key );

   DWORD showTips = 1;

   if ( result == ERROR_SUCCESS )
      {
      char username[ 128 ];
      DWORD length = 128;
      result = RegQueryValueEx( key, "LastUser", NULL, NULL, (LPBYTE) username, &length );

      if ( result != ERROR_SUCCESS )
         username[ 0 ] = NULL;

      // add most recently used files
      char mru[ 256 ];
      length = 256;
      result = RegQueryValueEx( key, "MRU1", NULL, NULL, (LPBYTE) mru, &length );
      if ( result == ERROR_SUCCESS )
         {
         m_mru1 = mru;
         if ( length > 1 )
            m_files.AddString( mru );
         length = 256;
         result = RegQueryValueEx( key, "MRU2", NULL, NULL, (LPBYTE) mru, &length );
         if ( result == ERROR_SUCCESS )
            {
            m_mru2 = mru;
            if ( length > 1 )
               m_files.AddString( mru );
            length = 256;
            result = RegQueryValueEx( key, "MRU3", NULL, NULL, (LPBYTE) mru, &length );
            if ( result == ERROR_SUCCESS )
               {
               m_mru3 = mru;
               if ( length > 1 )
                  m_files.AddString( mru );
               length = 256;
               result = RegQueryValueEx( key, "MRU4", NULL, NULL, (LPBYTE) mru, &length );
               if ( result == ERROR_SUCCESS )
                  {
                  m_mru4 = mru;
                  if ( length > 1 )
                     m_files.AddString( mru );
                  }
               }
            }
         }
      
      DWORD outputWndStatus = 0;
      length = sizeof( outputWndStatus );
      result = RegQueryValueEx( key, "OutputWndStatus", NULL, NULL, (LPBYTE) &outputWndStatus, &length );

      if ( result == ERROR_SUCCESS )
         gpDoc->m_outputWndStatus = outputWndStatus;

      result = RegQueryValueEx( key, "TipOfTheDay", NULL, NULL, (LPBYTE) &showTips, &length );

      if ( result != ERROR_SUCCESS )
         showTips = 1;
      
      RegCloseKey( key );

      m_name = username;
      SetDlgItemText( IDC_NAME, m_name );
      }


   // always add more files
   //m_files.AddString( "< More Projects... >" );
   //m_files.AddString( "< New Project... >" );
   m_files.SetCurSel( 0 );

   if ( m_files.GetCount() == 0 )
      OpenProject(); //OnCbnSelchangeFile();   // run open file dialog if nothing in MRU list yet

   if ( showTips )
      PostMessage( WM_COMMAND, 30000 );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void SplashDlg::OnOK()
   {
   UpdateData( 1 );

   m_isNewProject = false;

   if ( m_filename.Compare( _T("< New Project... >" )) == 0 )
      CreateNewProject();

   SaveRegistryKeys();

   CDialog::OnOK();
   }

bool SplashDlg::SaveRegistryKeys()
   {
   // store name in registry
   HKEY key;
   LONG result = RegCreateKeyEx( HKEY_CURRENT_USER,
               "SOFTWARE\\OSU Biosystems Analysis Group\\Envision",
               0, // reserved, must be zero
               NULL,
               0, 
               KEY_READ | KEY_WRITE,
               NULL,
               &key,
               NULL ); 

   if ( result != ERROR_SUCCESS )
      {
      DWORD error = GetLastError();
      char buffer[ 256 ];
      FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, error, 0, buffer, 256, NULL );
      Report::WarningMsg( buffer );
      return false;
      }

   char name[ 128 ];
   lstrcpy( name, (LPCSTR) m_name );

   result = RegSetValueEx( key, "LastUser", 0, REG_SZ, (LPBYTE) name, lstrlen(name) + 1);

   if ( m_mru1.CompareNoCase( m_filename ) != 0 )
      {
      m_mru4 = m_mru3;
      m_mru3 = m_mru2;
      m_mru2 = m_mru1;
      m_mru1 = m_filename;
      }

   // eliminate any redundancies
   if ( m_mru1.CompareNoCase( m_mru2 ) == 0 )
      m_mru2.Empty();
   if ( m_mru1.CompareNoCase( m_mru3 ) == 0 )
      m_mru3.Empty();
   if ( m_mru1.CompareNoCase( m_mru4 ) == 0 )
      m_mru4.Empty();

   if ( m_mru2.CompareNoCase( m_mru3 ) == 0 )
      m_mru3.Empty();
   if ( m_mru2.CompareNoCase( m_mru4 ) == 0 )
      m_mru4.Empty();

   if ( m_mru3.CompareNoCase( m_mru4 ) == 0 )
      m_mru4.Empty();

   if ( m_mru2.IsEmpty() )
      {
      m_mru2 = m_mru3;
      m_mru3.Empty();
      }

   if ( m_mru3.IsEmpty() )
      {
      m_mru3 = m_mru4;
      m_mru4.Empty();
      }

   if ( ! m_mru1.IsEmpty() )
      result = RegSetValueEx( key, "MRU1", 0, REG_SZ, (LPBYTE)(LPCSTR) m_mru1, m_mru1.GetLength()+1 );
   else
      result = RegSetValueEx( key, "MRU1", 0, REG_SZ, (LPBYTE)_T(""), 1 );

   if ( ! m_mru2.IsEmpty() )
      result = RegSetValueEx( key, "MRU2", 0, REG_SZ, (LPBYTE)(LPCSTR) m_mru2, m_mru2.GetLength()+1 );
   else
      result = RegSetValueEx( key, "MRU2", 0, REG_SZ, (LPBYTE)_T(""), 1 );

   if ( ! m_mru3.IsEmpty() )
      result = RegSetValueEx( key, "MRU3", 0, REG_SZ, (LPBYTE)(LPCSTR) m_mru3, m_mru3.GetLength()+1 );
   else
      result = RegSetValueEx( key, "MRU3", 0, REG_SZ, (LPBYTE)_T(""), 1 );
   
   if ( ! m_mru4.IsEmpty() )
      result = RegSetValueEx( key, "MRU4", 0, REG_SZ, (LPBYTE)(LPCSTR) m_mru4, m_mru4.GetLength()+1 );
   else
      result = RegSetValueEx( key, "MRU4", 0, REG_SZ, (LPBYTE) _T(""), 1 );

   RegCloseKey( key ); 

   return true;
   }


void SplashDlg::OnCancel()
   {
   CDialog::OnCancel();
   }

bool SplashDlg::CreateNewProject()
   {
   NewIniFileDlg newIniDlg( _T("New Project File Wizard" ) );
   
   if ( newIniDlg.DoModal() == ID_WIZFINISH )
      {
      CWaitCursor c;

      // create default envx file (with default policy file )
      EnvLoader loader;
      loader.LoadLayer( gpMapPanel->m_pMap, _T("IDU"), newIniDlg.m_step2.m_pathLayer, AMLT_SHAPE, 0,0,0, 0, -1, NULL, NULL, true, true );

      // ini file
      CString iniFile( newIniDlg.m_step1.m_pathIni );

      if( iniFile.Right( 1 ).CompareNoCase("\\" ) != 0 &&  iniFile.Right( 1 ).CompareNoCase("/" ) != 0  )
         iniFile += "\\";

      iniFile += newIniDlg.m_step1.m_projectName;
      iniFile += _T(".envx" );
      gpDoc->SaveAs( iniFile );

      m_filename = iniFile;

      if ( newIniDlg.m_finish.m_runIniEditor )
         m_runIniEditor = true;

      m_isNewProject = true;
      return true;
      }
   else  // canceled out of wizard
      return false;
   }         


void SplashDlg::OnCbnSelchangeFile()
   {
   int index = m_files.GetCurSel();
   m_files.GetLBText( index, m_filename );

   //if ( m_filename.Compare( "< More Files... >" ) == 0 )
   //   OpenProject();
   }


bool SplashDlg::OpenProject()
   {
   DirPlaceholder d;

   CFileDialog dlg( 1, "envx", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "Envision Project Files|*.envx|Version 4 INI files|*.ini|All files|*.*||", this );

   if ( dlg.DoModal() == IDOK )
      {
      CString newFilename = dlg.GetPathName();
      if ( newFilename.CompareNoCase( m_mru1 ) == 0 )
         m_files.SetCurSel( 0 );
      else if ( newFilename.CompareNoCase( m_mru2 ) == 0 )
         m_files.SetCurSel( 1 );
      else if ( newFilename.CompareNoCase( m_mru3 ) == 0 )
         m_files.SetCurSel( 2 );
      else if ( newFilename.CompareNoCase( m_mru4 ) == 0 )
         m_files.SetCurSel( 3 );
      else
         {
         int index = m_files.AddString( newFilename );
         m_files.SetCurSel( index );
         }
      return true;
      }
   else
      return false;
   }


void SplashDlg::OnStartup()
   {
   }

void SplashDlg::OnBnClickedNew()
   {
   bool ok = CreateNewProject();

   if ( ok )
      {
      SaveRegistryKeys();
      CDialog::OnOK();
      }
   }

void SplashDlg::OnBnClickedOpen()
   {
   OpenProject();
   }

