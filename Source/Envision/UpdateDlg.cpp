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
// UpdateDlg.cpp : implementation file
//

#include "stdafx.h"
#pragma hdrstop

#include "Envision.h"
#include "UpdateDlg.h"
#include "afxdialogex.h"

#include <PathManager.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "WebServices\soapEnvWebServiceSoapProxy.h"
#include "WebServices\EnvWebServiceSoap.nsmap"

#include "EnvDoc.h"

extern CEnvDoc *gpDoc;


// UpdateDlg dialog

IMPLEMENT_DYNAMIC(UpdateDlg, CDialogEx)

UpdateDlg::UpdateDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(UpdateDlg::IDD, pParent)
   , m_disable(FALSE)
   {

}

UpdateDlg::~UpdateDlg()
{
m_updateFont.DeleteObject();
}

void UpdateDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Control(pDX, IDC_VERSION, m_versionInfo);
DDX_Check(pDX, IDC_DISABLE, m_disable);
DDX_Control(pDX, IDC_STUDYAREAS, m_studyAreas);
DDX_Control(pDX, IDC_UPDATETEXT, m_updateText);
}


BEGIN_MESSAGE_MAP(UpdateDlg, CDialogEx)
END_MESSAGE_MAP()


// UpdateDlg message handlers
bool UpdateDlg::Init( void )
   {
#if defined( _WIN64 )
   char *build = "x64";
#else
   char *build = "Win32";
#endif   
   EnvWebServiceSoapProxy service;

   _ns1__GetEnvisionVersion g;
   _ns1__GetEnvisionVersionResponse r;
   g.build = build;
 
   if ( service.GetEnvisionVersion( &g, &r ) != SOAP_OK )
      {
      //Report::Log( "Unable to connect to Envision Update Service, updating services will be disabled for this session" );
      return false;
      }
   
   int webVersion = (int) r.GetEnvisionVersionResult;    // e.g. 6101

   int webMajVer = webVersion / 1000;
   int webMinVer = webVersion % 1000;

   // get local version from version.txt
   // first, ENVISION.EXE executable directory
   TCHAR _exePath[ MAX_PATH ];  // Envision exe directory e.g. _path = "D:\Envision\src\x64\Debug\Envision.exe"
   GetModuleFileName( NULL, _exePath, MAX_PATH );

   CPath exePath( _exePath, epcTrim | epcSlashToBackslash );
   exePath.RemoveFileSpec();           // e.g. c:\Envision - directory for executables

   // try to open version file.  Note that for internal use, envision.ver will be in Envision or Envision\x64
   CPath versionFile( exePath );   // d:\envision\x64\debug for internal builds

   // is this a debug/release build (as opposed to an instlled version)?  If so, don't check for updates.
   CString strPath( exePath );
   strPath.MakeLower();
   bool isDebugOrRelease = strPath.Find( "debug" ) >= 0 || strPath.Find( "release" ) >= 0 ? true : false;

   // if an internal build, adjust for envision.ver location
   if ( isDebugOrRelease )
      {
#if defined( _WIN64 )
      versionFile = "\\Envision\\src\\x64";
#else
      versionFile = "\\Envision\\src";
#endif 
      //return false;
      }

   // first try envision.ver.
   versionFile.AddBackslash();
   versionFile.Append( "Envision.ver" );

   FILE *fp = NULL;
   fopen_s( &fp, versionFile, "rt" );

   if ( fp == NULL )
      {
      CPath versionFile2( exePath );
      versionFile2.AddBackslash();
      versionFile2.Append( "version.txt" );
      fopen_s( &fp, versionFile2, "rt" );

      if ( fp == NULL )
         {
         CString msg;
         msg.Format( "Unable to open version file '%s', updating services will be disabled for this session.", (LPCTSTR) versionFile );
         Report::Log( msg );
         return false;
         }
      }

   int majVer, minVer;
   fscanf_s( fp, "%i.%i", &majVer, &minVer );
   fclose( fp );

   int version = majVer*1000 + minVer;
   if ( version >= webVersion )    // is the local version more recent than the web version? if so, no update necessary
      {
      CString msg;
      msg.Format( "Envision Update Services reports your version (%i.%i) is current.", majVer, minVer );
      Report::Log( msg );
      return false;
      }

   _ns1__GetEnvisionSetupDateTime g1;
   _ns1__GetEnvisionSetupDateTimeResponse r1;
   g1.build = build;
  
   if ( service.GetEnvisionSetupDateTime( &g1, &r1 ) != SOAP_OK )
      return false;

   // number of seconds elapsed since midnight, January 1, 1970. 
   time_t setupTime = r1.GetEnvisionSetupDateTimeResult;  // UTC time setup file was last modified
   
   // get ENVISION.EXE executable path
   TCHAR path[ MAX_PATH ];  // e.g. _path = "D:\Envision\src\x64\Debug\Envision.exe"
   GetModuleFileName( NULL, path, MAX_PATH );

   // get time stamps for file
   struct _tstat buf;
   int result = _tstat( path, &buf );
   
   // Check if statistics are valid:
   if( result != 0 )
      return false;   
   
   time_t exeTime = buf.st_mtime;

   tm tmSetup;
   tm tmExe;
   localtime_s( &tmSetup, &setupTime );
   localtime_s( &tmExe, &exeTime );

   m_updateMsg.Format( "Checking for updates: Your Envision version (%i.%i) was built %i/%i/%i; the website version (%i.%i) was created %i/%i/%i.",
      majVer, minVer, tmExe.tm_mon+1, tmExe.tm_mday, tmExe.tm_year+1900,
      webMajVer, webMinVer, tmSetup.tm_mon+1, tmSetup.tm_mday, tmSetup.tm_year+1900 );

   Report::Log( m_updateMsg );

   if ( setupTime <= ( exeTime ) ) // + (7*60*60*24) ) )  // allow seven day gap
      {
      return false;
      }

   m_updateMsg.Format( "Your version (%i.%i) was built %i/%i/%i; a new version (%i.%i) created %i/%i/%i is available.  Do you want to update?",
      majVer, minVer, tmExe.tm_mon+1, tmExe.tm_mday, tmExe.tm_year+1900,
      webMajVer, webMinVer, tmSetup.tm_mon+1, tmSetup.tm_mday, tmSetup.tm_year+1900 );

   CString envPath( exePath );
   int index = envPath.Find( "Envision" );
   
   if ( index > 0 )
      {
      CPath envPath2( envPath.Left(  index + lstrlen( "Envision" ) ) );
      envPath2.AddBackslash();
      envPath2.Append( "StudyAreas\\" );
   
      CPath envPath3( envPath2 );
      envPath2.Append( "*.*" );
   
      // now pointing to studyareas folder, see if there are any subdirectories with envx files.
      // enumerate subdirectories
      WIN32_FIND_DATA findFileData;
      HANDLE hFind = FindFirstFile( envPath2, &findFileData );
   
      int count = 0;
      if ( hFind != INVALID_HANDLE_VALUE )
         {
         do {
            // Find first file will always return "." and ".." as the first two directories.
            if ( lstrcmp(findFileData.cFileName, ".") != 0  && lstrcmp( findFileData.cFileName, "..") != 0
               && findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
               {
               // Build up our file path using the StudyAreas folder and the subDir we just found
               // [sDir] and the file/foldername we just found:

               StudyAreaInfo *pInfo = new StudyAreaInfo;

               pInfo->m_path = envPath3;
               pInfo->m_path += findFileData.cFileName;
   
               // look for version info
               // TODO
               pInfo->m_name = findFileData.cFileName;
               pInfo->m_localVer = "v0.0";
               pInfo->m_webVer = "v0.0";

               m_saArray.Add( pInfo );
               }  // end of: if ( subdirectory )
   
            } while ( FindNextFile( hFind, &findFileData ) != 0 ); //Find the next file.
   
         FindClose(hFind); //Always, Always, clean things up!
         }  // end of: if ( hFind != INVALID_FILE_HANDLE )
      }

      // check registry to see if the dialog is diabled
   
   // get values from registry
   HKEY key;
   LONG result2 = RegOpenKeyEx( HKEY_CURRENT_USER,
               "SOFTWARE\\OSU Biosystems Analysis Group\\Envision",
               0, KEY_QUERY_VALUE, &key );

   if ( result2 == ERROR_SUCCESS )
      {
      DWORD checkForUpdates;
      DWORD length = sizeof( DWORD);
      result = RegQueryValueEx( key, "CheckForUpdates", NULL, NULL, (LPBYTE) &checkForUpdates, &length );

      if ( result == ERROR_SUCCESS )
         m_disable = checkForUpdates ? false : true;
      else
         m_disable = false;

      RegCloseKey( key );
      }

   return true;
   }


BOOL UpdateDlg::OnInitDialog()
   {
   CDialogEx::OnInitDialog();

   m_updateFont.CreatePointFont( 140, "Arial" );
   m_updateText.SetFont( &m_updateFont, false );
   m_versionInfo.SetWindowText( m_updateMsg );

   // set up installed study areas
   CRect rect;
   m_studyAreas.GetClientRect(&rect);
   
   m_studyAreas.InsertColumn( 0, _T("Study Area"), LVCFMT_LEFT, rect.right/2 );
   m_studyAreas.InsertColumn( 1, _T("Current Version"), LVCFMT_LEFT, rect.right/4 );
   m_studyAreas.InsertColumn( 2, _T("Web Version"), LVCFMT_LEFT, rect.right/4 );
   
   // insert study areas - need to search Envision study area directories.  We assume it is located "below" the "Envision" path
   for ( int i=0; i < (int) m_saArray.GetSize(); i++ )
      {
      StudyAreaInfo *pInfo = m_saArray[ i ];

      /// Insert the first item
      LVITEM lvi;
      lvi.mask =  LVIF_TEXT;
      lvi.iItem = i;
      lvi.iSubItem = 0;
      lvi.pszText = (LPSTR)(LPCTSTR) pInfo->m_name;
      m_studyAreas.InsertItem(&lvi);
      
      // Set subitem 1
      lvi.iSubItem =1;
      lvi.pszText = (LPSTR)(LPCTSTR) pInfo->m_localVer;
      m_studyAreas.SetItem(&lvi);
      // Set subitem 2
      lvi.iSubItem =2;
      lvi.pszText = (LPSTR)(LPCTSTR) pInfo->m_webVer;
      m_studyAreas.SetItem(&lvi);
      }  // end of: if ( subdirectory )

   return TRUE;
   }


void UpdateDlg::OnCancel()
   {
   SaveUpdateStatusToReg();
   
   CDialogEx::OnCancel();
   }


void UpdateDlg::OnOK()   
   {
   SaveUpdateStatusToReg();

   CDialogEx::OnOK();
   }


void UpdateDlg::SaveUpdateStatusToReg( void )
   {
   UpdateData( 1 );     // updates m_disable

   // store flag in registry
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
      }
   else
      {
      DWORD checkForUpdates = m_disable ? 0 : 1;
      result = RegSetValueEx( key, "CheckForUpdates", 0, REG_DWORD, (LPBYTE) &checkForUpdates, sizeof(DWORD) );
      RegCloseKey( key ); 
      }
   }
