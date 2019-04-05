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
// This MFC Samples source code demonstrates using MFC Microsoft Office Fluent User Interface 
// (the "Fluent UI") and is provided only as referential material to supplement the 
// Microsoft Foundation Classes Reference and related electronic documentation 
// included with the MFC C++ library software.  
// License terms to copy, use or distribute the Fluent UI are available separately.  
// To learn more about our Fluent UI licensing program, please visit 
// http://msdn.microsoft.com/officeui.
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// Envision.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "Envision.h"
#include "MainFrm.h"

#include "EnvDoc.h"
#include "EnvView.h"

#include <PtrArray.h>

#include <MAP.h>
#include <Maplayer.h>
#include <QueryEngine.h>
#include <Query.h>
#include <tixml.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern CEnvDoc *gpDoc;

enum CLI_CMDS { CMD_LOADPROJECT=0, CMD_EXPORT, CMD_QUERY, CMD_OUTPUT, CMD_FROMFILE, CMD_RUN, CMD_MULTIRUN, CMD_NOEXIT }; 


// CEnvApp

BEGIN_MESSAGE_MAP(CEnvApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CEnvApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinApp::OnFilePrintSetup)
   ON_COMMAND(ID_HELP_UPDATEENVISION, &CEnvApp::OnHelpUpdateEnvision)
END_MESSAGE_MAP()




struct CLI 
   {
   int command; 
   CString argument;

   CLI( void ) : command( 0 ) { }
   CLI( int cmd, LPCTSTR arg ) : command( cmd ), argument( arg ) { }
   };



class EnvCommandLineInfo : public CCommandLineInfo
{
public:
  EnvCommandLineInfo() : CCommandLineInfo() { }

  PtrArray <CLI> m_cmds;

  CLI *FindCmd( int cmd ) { for ( INT_PTR i=0; i < m_cmds.GetSize(); i++ ) if ( m_cmds[ i ]->command == cmd ) return m_cmds[ i ]; return NULL; }

public:
  virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast);
};



// CEnvApp construction

CEnvApp::CEnvApp()
{

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CEnvApp object

CEnvApp theApp;


// CEnvApp initialization

BOOL CEnvApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
   //afxAmbientActCtx = FALSE;

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
 	SetRegistryKey(_T("OSU Biosystems Analysis Group"));
   LoadStdProfileSettings(0);  // Load standard INI file options

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CEnvDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CEnvView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// Enable DDE Execute open
	//EnableShellOpen();
	//RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	//CCommandLineInfo cmdInfo;
	//ParseCommandLine(cmdInfo);


   //////////////////////
   EnvCommandLineInfo cmdInfo;
   ParseCommandLine(cmdInfo);
     
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

   if ( ProcessEnvCmdLineArgs( cmdInfo ) == false )
      return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	// Enable drag/drop open
	//m_pMainWnd->DragAcceptFiles();
   
   if ( gpDoc->StartUp() < 0  )
      {
      switch( gpDoc->m_exitCode )
         {
         case -1:
            //msg = _T("Exiting from splash screen");
            break;

         case -2:
            AfxMessageBox( "The specified Project can't be found or can't be read.  Envision requires a valid Project (.envx) file to proceed.  Plses fix this problem and retry...", MB_OK );
            break;

         case -3:
            AfxMessageBox( "A map layer file specified in the Project (.envx) file was not found or could not be loaded.  The Project file must be fixed before proceeding...", MB_OK );
            break;

         case -4:
            AfxMessageBox( "No IDU layer was found.  Please specify an input IDU coverage in the Project (.envx) file to proceed...", MB_OK );
            break;

         case -10:  // /r (run scenario) specified as command line - no further action needed
			 return true;  // break;

         default:
            AfxMessageBox( "An unspecified error was encountered during startup.  Envision is exiting...", MB_OK );
         }
      return FALSE;
      }

	return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CEnvApp::OnAppAbout()
   {
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
   }


void CEnvApp::OnHelpUpdateEnvision()
   {
   // launch an update bat file external to the process

   // first, close and open docs
   if ( AfxMessageBox( "Updating Envision will close the current application and install the latest version of Envision from the Envision website.  Any changes from this session that have not been saved will be lost.  Do you want to continue?", MB_YESNO ) != IDYES )
      return;

   // write script file
   TCHAR _path[ MAX_PATH ];  // e.g. _path = "D:\Envision\src\x64\Debug\Envision.exe"
   GetModuleFileName( NULL, _path, MAX_PATH );

   for ( int i=0; i < lstrlen( _path ); i++ )
         _path[ i ] = tolower( _path[ i ] );

   // special cases - contains "Debug" or "Release"? (on a developer machine?)
   TCHAR *debug   = _tcsstr( _path, _T("debug") );
   TCHAR *release = _tcsstr( _path, _T("release") );
   TCHAR *end     = NULL;
   if ( debug || release )         
      {
      end = _tcsstr( _path, _T("envision") );
      if ( end != NULL )
         {
         end += 8 * sizeof( TCHAR );
         *end = NULL;
         }
      }

   else     // installed as an application
      {
      end = _tcsrchr( _path, '\\' );
      if ( end != NULL )
         *end = NULL;
      }

   CString scriptPath( _path );
   scriptPath += "\\update_ftp.scr";

   CString lcdPath( "lcd " );    // string for local (client) directory to install files
   lcdPath += _path; 

   CString batPath( _path );
   batPath += "\\EnvisionUpdater.bat";
   
   CString msg( "Creating script file for update: " );
   msg += scriptPath;
   Report::Log( msg );

   ///
   //lcdPath = "lcd \\Envision\\temp";
   ///
   
   FILE *fp = NULL;
   PCTSTR filename = (PCTSTR)scriptPath;
   if ( fopen_s( &fp, filename, "wt" ) == 0 )
      {
      fputs( lcdPath                 , fp );
      fputs( "\nbinary\n"            , fp );
#if defined( _WIN64 )
      fputs( "cd x64\n"              , fp );  // subdir on ftp server with correct files
#else
      fputs( "cd Win32\n"            , fp );  // subdir on ftp server with correct files
#endif
      fputs( "mget *.exe\n"          , fp );
      fputs( "mget *.dll\n"          , fp );
      fputs( "mget *.txt\n"          , fp );
      fputs( "mget *.bat\n"          , fp );
      fputs( "mget *.ver\n"          , fp );
      fputs( "quit"                  , fp );
      fclose( fp );

      CString msg( "Executing batch file for update: " );
      msg += batPath;
      Report::Log( msg );
      
      // exit this process and run a batch file to update.  (need to pass environment?)
      _execl( batPath, "EnvisionUpdater.bat" );
      }
   }



// CEnvApp customization load/save methods

void CEnvApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CEnvApp::LoadCustomState()
{
}

void CEnvApp::SaveCustomState()
{
}

// CEnvApp message handlers


bool CEnvApp::ProcessEnvCmdLineArgs( EnvCommandLineInfo &cmdInfo )
   {
   // return false to exit Envision, true to continue loading Envision
   //TCHAR *cmdLine = this->m_lpCmdLine;
   if ( cmdInfo.m_cmds.GetSize() == 0 )
      return true;

   CLI *pCmdLoadProject = cmdInfo.FindCmd( CMD_LOADPROJECT );
   CLI *pCmdExport      = cmdInfo.FindCmd( CMD_EXPORT );
   CLI *pCmdQuery       = cmdInfo.FindCmd( CMD_QUERY );
   CLI *pCmdOutput      = cmdInfo.FindCmd( CMD_OUTPUT ); 
   CLI *pCmdFromFile    = cmdInfo.FindCmd( CMD_FROMFILE );
   CLI *pCmdRun         = cmdInfo.FindCmd( CMD_RUN );
   CLI *pCmdMultiRun    = cmdInfo.FindCmd( CMD_MULTIRUN );
   CLI *pCmdNoExit      = cmdInfo.FindCmd( CMD_NOEXIT );

   // /r:scenarioIndex (-1 for all scenarios)
   if ( pCmdLoadProject )
      {
      gpDoc->m_cmdLoadProject = pCmdLoadProject->argument;  // (Note: this is one-based, 0=run all)
      }

   if ( pCmdFromFile )
      {
      // have xml string, start parsing
      TiXmlDocument doc;
      bool ok = doc.LoadFile( pCmdFromFile->argument );

      if ( ! ok )
         {
         CString msg;
         msg.Format("Error reading configuration file, %s: %s", (LPCTSTR) pCmdFromFile->argument, doc.ErrorDesc() );
         AfxMessageBox( msg );
         return false;
         }

      // start interating through the nodes
      TiXmlElement *pXmlRoot = doc.RootElement();     // <command_info>
      
      TiXmlElement *pXmlCmd = pXmlRoot->FirstChildElement();

      while ( pXmlCmd != NULL )
         {
         CString cmd( pXmlCmd->Value() );

         //------ export command ----------------
         if ( cmd.CompareNoCase( _T("export" )) == 0 )     // export command?
            {
            LPCTSTR layer = NULL;
            LPCTSTR query = NULL;
            LPCTSTR output = NULL;
            
            XML_ATTR attrs[] = 
               { // attr      type          address  isReq checkCol
               { "layer",     TYPE_STRING,   &layer,  true,   0 },
               { "query",     TYPE_STRING,   &query,  true,   0 },
               { "output",    TYPE_STRING,   &output, true,   0 },
               { NULL,        TYPE_NULL,     NULL,    false,  0 } };
  
            bool ok = TiXmlGetAttributes( pXmlCmd, attrs, pCmdFromFile->argument );
            if ( layer == NULL || query == NULL  || output == NULL )
               return false;

            Map map;
            MapLayer *pLayer = map.AddShapeLayer( layer, true );
            if ( pLayer == NULL )
               return false;

            QueryEngine qe( pLayer );
            Query *pQuery = qe.ParseQuery( query, 0, "Envision Command Line" );

            qe.SelectQuery( pQuery, false ); //, false );
            pLayer->SaveShapeFile( output, true );
            }

         //else if ( cmd.CompareNoCase( _T( "some other command" )
         //   ;

         pXmlCmd = pXmlCmd->NextSiblingElement();
         }  // end of: while ( pXmlCmd != NULL )

      return false;
      }  // end of: cmd = conigure from file

   // /r:scenarioIndex (-1 for all scenarios)
   if ( pCmdRun )
      {
      if ( isdigit( pCmdRun->argument[0] ) == 0 )
         {
         AfxMessageBox( "Bad parameter passed to command line /r:scnIndex" );
         return false;
         }

      int index = atoi( pCmdRun->argument );

      gpDoc->m_cmdRunScenario = index;  // (Note: this is one-based, 0=run all)
      }

   // /m:scenarioIndex (-1 for all scenarios)
   if ( pCmdMultiRun )
      {
      if ( isdigit( pCmdMultiRun->argument[0] ) == 0 )
         {
         AfxMessageBox( "Bad parameter passed to command line /m:scenarioIndex" );
         return false;
         }

      //int index = atoi( pCmdRun->argument );
      //gpDoc->m_cmdRunScenario = index;  // (Note: this is one-based, 0=run all)

      int runCount = atoi( pCmdMultiRun->argument );
      gpDoc->m_cmdMultiRunCount = runCount;
      }

   if (pCmdNoExit)
      gpDoc->m_cmdNoExit = true;

   return true;
   }


void EnvCommandLineInfo::ParseParam(const TCHAR* pszParam, BOOL isSwitch, BOOL bLast)
   {
   // /c:cmdInfo.xml
   // /r:scnIndex    run using specified scenario.  scenarios are one-based.  0=run ALL scenarios
   // /q:
   // /o:
   // /e: 

   if ( pszParam == NULL )
     return;

   LPTSTR command = (LPTSTR) pszParam;

   if ( isSwitch )
      {
      LPTSTR arg = command+2;   // assumes format is  /r:arg or -r:arg (NOT -rarg)

      switch( *command  )
         {
         case 'e':
            m_cmds.Add( new CLI( CMD_EXPORT, arg ) );
            break;
   
         case 'q':
            m_cmds.Add( new CLI( CMD_QUERY, arg ) );
            break;
   
         case 'o':
            m_cmds.Add( new CLI( CMD_OUTPUT, arg ) );
            break;
   
         case 'c':
            m_cmds.Add( new CLI( CMD_FROMFILE, arg ) );
            break;
   
         case 'r':         // run a scenario
            m_cmds.Add( new CLI( CMD_RUN, arg ) );
            break;
 
         case 'm':         // run a multirun scenario
            m_cmds.Add( new CLI( CMD_MULTIRUN, arg ) );
            break;

		 case 'n':         // no exit from command line run/multirun scenario
			 m_cmds.Add(new CLI(CMD_NOEXIT, arg));
			 break;
		 
		 default:
            break;
         }
      }
   else     // it's a parameter (must be envx file nmae
      {
      m_cmds.Add( new CLI( CMD_LOADPROJECT, command ) );
      }
   }

