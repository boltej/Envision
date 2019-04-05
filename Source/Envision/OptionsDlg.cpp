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
// OptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "OptionsDlg.h"
#include "EnvDoc.h"
#include <EnvModel.h>
#include "LoadDlg.h"


extern CEnvDoc *gpDoc;
extern EnvModel *gpModel;


// OptionsDlg dialog


//static const TCHAR szTips[]    = _T("TipOfTheDay");
//static const TCHAR szCheckForUpdates[]    = _T("CheckForUpdates");
//static const TCHAR szStartup[] = _T("StartUp");


IMPLEMENT_DYNAMIC(OptionsDlg, CDialog)

OptionsDlg::OptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(OptionsDlg::IDD, pParent)
   , m_showTips(FALSE)
   , m_checkForUpdates(TRUE)
   , m_isDirty( false )
   {
   }


OptionsDlg::~OptionsDlg()
{
}

void OptionsDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Check(pDX, IDC_SHOWTIPS, m_showTips);
DDX_Check(pDX, IDC_CHECKFORUPDATES, m_checkForUpdates);
DDX_Control(pDX, IDC_LIST, m_addIns);
DDX_Control(pDX, IDC_TAB, m_tabCtrl);
}


BEGIN_MESSAGE_MAP(OptionsDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, &OptionsDlg::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_DELETE, &OptionsDlg::OnBnClickedDelete)
   ON_NOTIFY(TCN_SELCHANGE, IDC_TAB, &OptionsDlg::OnTcnSelchangeTab)
   ON_BN_CLICKED(IDC_SHOWTIPS, &OptionsDlg::OnBnClickedShowtips)
   ON_BN_CLICKED(IDC_CHECKFORUPDATES, &OptionsDlg::OnBnClickedCheckforupdates)
END_MESSAGE_MAP()


enum { ANALYSIS=0, VISUALIZATION=1, DATA_MANAGEMENT=2 };


BOOL OptionsDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   //CWinApp* pApp = AfxGetApp();
   //m_showTips = pApp->GetProfileInt(szTips, szStartup, TRUE);
   //m_checkForUpdates = pApp->GetProfileInt(szCheckForUpdates, szStartup, TRUE);

   HKEY key;
   LONG result = RegOpenKeyEx( HKEY_CURRENT_USER,
               "SOFTWARE\\OSU Biosystems Analysis Group\\Envision",
               0, KEY_QUERY_VALUE, &key );

   if ( result == ERROR_SUCCESS )
      {
      DWORD checkForUpdates=1;
      DWORD length = sizeof( DWORD);
      result = RegQueryValueEx( key, "CheckForUpdates", NULL, NULL, (LPBYTE) &checkForUpdates, &length );

      if ( result == ERROR_SUCCESS )
         m_checkForUpdates = checkForUpdates ? true : false;

      DWORD showTips;
      result = RegQueryValueEx( key, "TipOfTheDay", NULL, NULL, (LPBYTE) &showTips, &length );

      if ( result == ERROR_SUCCESS )
         m_showTips = showTips ? true : false;

      RegCloseKey( key );

      UpdateData( 0 );  // update UI
      }


   m_tabCtrl.InsertItem( 0, _T("Modeling/Analysis") );
   m_tabCtrl.InsertItem( 1, _T("Visualization") );
   m_tabCtrl.InsertItem( 2, _T("Database Management"));
   
   LoadAnalysisModules();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void OptionsDlg::OnOK()
   {
   UpdateData( 1 );

   bool ok = true;

   if ( m_isDirty )
      ok = SaveRegistryKeys();

   if ( ok )
      CDialog::OnOK();
   }


void OptionsDlg::OnBnClickedAdd()
   {
   m_isDirty = true;

   LoadDlg dlg;
   if ( dlg.DoModal() == IDOK )
      {
      HINSTANCE hDLL = AfxLoadLibrary( dlg.m_path ); 
      if ( hDLL == NULL )
         {
         CString msg( "Unable to find " );
         msg += dlg.m_path;
         Report::ErrorMsg( msg );
         Report::SystemErrorMsg( GetLastError() );
         return;
         }

      switch( m_tabCtrl.GetCurSel() )
         {
         case ANALYSIS:
            {
            ANALYSISMODULEFN runFn = (ANALYSISMODULEFN) GetProcAddress( hDLL, "AMRun" );
            if ( runFn == NULL )
               {
               CString msg( "Unable to find function 'AMRun()' in " );
               msg += dlg.m_path;
               Report::ErrorMsg( msg );
               AfxFreeLibrary( hDLL );
               return;
               }

            if ( dlg.m_name.IsEmpty() )
               dlg.m_name = dlg.m_path;
      
            EnvAnalysisModule *pInfo = new EnvAnalysisModule;
            pInfo->m_hDLL = hDLL;
            pInfo->m_name = dlg.m_name;
            pInfo->m_path = dlg.m_path;
            pInfo->m_use = true;

            gpDoc->AddAnalysisModule( pInfo );
            this->m_addIns.AddString( pInfo->m_name );
            m_isDirty= true;
            }
            break;

         case VISUALIZATION:
            {
            //if ( runFn == NULL && initFn == NULL /*&& postRunFn == NULL */)
            //   {
            //   CString msg( "Unable to find Visualizer Entry point function ( Init() and/or Run() ) in " );
            //   msg += dlg.m_path;
            //   Report::ErrorMsg( msg );
            //   AfxFreeLibrary( hDLL );
            //   return;
            //   }

            if ( dlg.m_name.IsEmpty() )
               dlg.m_name = dlg.m_path;

            int id = -( m_addIns.GetCount()+1 ); 
            int type = 0;  // WRONG!!!!
                  
            //EnvVisualizer *pInfo = new EnvVisualizer( hDLL, runFn, initFn, initRunFn, endRunFn, initWndFn, updateWndFn, setupFn, 0, type, initInfo, name, path, false );
            //pInfo->use = true;

            //gpModel->AddVisualizer( pInfo );
            //this->m_addIns.AddString( pInfo->m_name );
            //m_isDirty= true;
            }
            break;

         case DATA_MANAGEMENT:
            {
            }
            break;
         }
      }
   }


void OptionsDlg::OnBnClickedDelete()
   {
   int index = m_addIns.GetCurSel();

   if ( MessageBox( _T("Delete this Extension Module"), "Delete?", MB_YESNOCANCEL) == IDYES )
      {
      gpDoc->RemoveAnalysisModule( index );
      m_isDirty = true;
      }
   }


bool OptionsDlg::SaveRegistryKeys()
   {
   UpdateData( 1 );

   //CWinApp* pApp = AfxGetApp();
   //pApp->WriteProfileInt( szTips, szStartup, m_showTips ? 1 : 0 );
   //pApp->WriteProfileInt( szCheckForUpdates, szStartup, m_checkForUpdates ? 1 : 0 );

   HKEY rootKey;
   DWORD result = RegCreateKeyEx( HKEY_CURRENT_USER,
               _T("SOFTWARE\\OSU Biosystems Analysis Group\\Envision"),
               0, // reserved, must be zero
               NULL,
               0, 
               KEY_READ | KEY_WRITE,
               NULL,
               &rootKey,
               NULL ); 

   if ( result != ERROR_SUCCESS )
      return false;

   DWORD checkForUpdates = m_checkForUpdates ? 1 : 0;
   result = RegSetValueEx( rootKey, "CheckForUpdates", 0, REG_DWORD, (LPBYTE) &checkForUpdates, sizeof(DWORD) );

   DWORD tipOfTheDay = m_showTips ? 1 : 0;
   result = RegSetValueEx( rootKey, "TipOfTheDay", 0, REG_DWORD, (LPBYTE) &tipOfTheDay, sizeof(DWORD) );

   
   RegDeleteKey( rootKey, _T("AnalysisModules") );

   HKEY key;
   for ( int i=0; i < gpDoc->GetAnalysisModuleCount(); i++ )
      {
      EnvAnalysisModule *pInfo = gpDoc->GetAnalysisModule( i );
   
      // store name in registry
      CString keyName( _T("SOFTWARE\\OSU Biosystems Analysis Group\\Envision\\AnalysisModules\\") );
      keyName += pInfo->m_name;

      DWORD result = RegCreateKeyEx( HKEY_CURRENT_USER,
               keyName,
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

      RegSetValueEx( key, _T("path"),     0, REG_SZ, (LPBYTE)(LPCTSTR) pInfo->m_path,     pInfo->m_path.GetLength()+1 );
      RegSetValueEx( key, _T("initInfo"), 0, REG_SZ, (LPBYTE)(LPCTSTR) pInfo->m_initInfo, pInfo->m_initInfo.GetLength()+1 );
      RegCloseKey( key );
      }

   RegDeleteKey( rootKey, _T("Visualizers") );

   for ( int i=0; i < gpModel->GetVisualizerCount(); i++ )
      {
      EnvVisualizer *pInfo = gpModel->GetVisualizerInfo( i );
   
      // store name in registry
      CString keyName( _T("SOFTWARE\\OSU Biosystems Analysis Group\\Envision\\Visualizers\\") );
      keyName += pInfo->m_name;

      DWORD result = RegCreateKeyEx( HKEY_CURRENT_USER,
               keyName,
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

      RegSetValueEx( key, _T("name"),     0, REG_SZ,    (LPBYTE)(LPCTSTR) pInfo->m_name,     pInfo->m_name.GetLength()+1 );
      RegSetValueEx( key, _T("type"),     0, REG_DWORD, (LPBYTE) &pInfo->m_vizType,          sizeof( DWORD ) );
      RegSetValueEx( key, _T("path"),     0, REG_SZ,    (LPBYTE)(LPCTSTR) pInfo->m_path,     pInfo->m_path.GetLength()+1 );
      RegSetValueEx( key, _T("initInfo"), 0, REG_SZ,    (LPBYTE)(LPCTSTR) pInfo->m_initInfo, pInfo->m_initInfo.GetLength()+1 );
      
      RegCloseKey( key );
      }
   
   return true;
   }



void OptionsDlg::OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult)
   {
   // enum { ANALYSIS=0, VISUALIZATION=1, DATA_MANAGEMENT=2 };
   int index = TabCtrl_GetCurSel( pNMHDR->hwndFrom );

   switch( index )
      {
      case ANALYSIS:
         LoadAnalysisModules();
         break;

      case VISUALIZATION:
         LoadVisualizationModules();
         break;

      case DATA_MANAGEMENT:
         LoadDataManagementModules();
         break;

      default:
         ASSERT( 0);
      }

   *pResult = 0;
   }


void OptionsDlg::LoadAnalysisModules()
   {
   m_addIns.ResetContent();

   for ( int i=0; i < gpDoc->GetAnalysisModuleCount(); i++ )
      {
      EnvAnalysisModule *pInfo = gpDoc->GetAnalysisModule( i );
      m_addIns.AddString( pInfo->m_name );
      }
   }


void OptionsDlg::LoadVisualizationModules()
   {
   m_addIns.ResetContent();

   for ( int i=0; i < gpModel->GetVisualizerCount(); i++ )
      {
      EnvVisualizer *pInfo = gpModel->GetVisualizerInfo( i );

      if( pInfo->m_inRegistry )
         m_addIns.AddString( pInfo->m_name );
      }
   }


void OptionsDlg::LoadDataManagementModules()
   {
   m_addIns.ResetContent();

   for ( int i=0; i < gpDoc->GetDataModuleCount(); i++ )
      {
      EnvDataModule *pInfo = gpDoc->GetDataModule( i );
      m_addIns.AddString( pInfo->m_name );
      }
   }


void OptionsDlg::OnBnClickedShowtips()
   {
   m_isDirty = true;
   }


void OptionsDlg::OnBnClickedCheckforupdates()
   {
   m_isDirty = true;
   }
