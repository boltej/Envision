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

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Envision.h"

#include "MainFrm.h"
#include "EnvDoc.h"
#include "EnvView.h"
#include "ActiveWnd.h"
#include <EnvModel.h>
#include <Scenario.h>
#include "ScenarioEditor.h"
#include "ConstraintsDlg.h"

#include <map.h>
#include <maplayer.h>
#include <Path.h>
#include <NameDlg.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CMainFrame *gpMain = NULL;

extern ScenarioManager *gpScenarioManager;
extern ActorManager    *gpActorManager;
extern PolicyManager   *gpPolicyManager;
extern CEnvDoc         *gpDoc;
extern CEnvView        *gpView;
extern MapPanel        *gpMapPanel;
extern MapLayer        *gpCellLayer;
extern EnvModel        *gpModel;

BEGIN_MESSAGE_MAP(CLogPane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
   ON_WM_CLOSE()
END_MESSAGE_MAP()

int CLogPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

    CRect rect;
   this->GetClientRect(rect);
   m_logCtrl.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER, rect, this, 0x118);

   m_font.CreatePointFont(90, _T("Courier New"));
   m_logCtrl.SetFont(&m_font);

   TCHAR exePath[ MAX_PATH ];
   GetModuleFileName( 0, exePath, MAX_PATH );
   
   nsPath::CPath cPath(exePath);
   cPath.RemoveFileSpec();
 
   SYSTEMTIME tm;
   ::GetSystemTime( &tm );
   CString msg;

   LPTSTR target;
#if defined( _WIN64 )
   target = _T( "(x64)" );
#else
   target = _T( "(Win32)" );
#endif
   msg.Format( _T("ENVISION %s started at %i/%i/%i %i:%i:%i from %s"), target, (int)tm.wMonth, (int)tm.wDay, (int)tm.wYear, (int)tm.wHour, (int)tm.wMinute, (int)tm.wSecond, (LPCTSTR) cPath );
   Log(msg);

	return 0;
}

void CLogPane::OnClose()
   {
   // TODO: Add your message handler code here and/or call default
   this->m_font.DeleteObject();

   CDockablePane::OnClose();
   }


void CLogPane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

   if ( this->GetSafeHwnd() != nullptr)
      ::MoveWindow( m_logCtrl, 0,0, cx, cy, TRUE );
}



void CLogPane::AddLogLine(LPCTSTR msg, LOG_TYPES type, COLORREF color)
   {
   CString _msg(msg);
   _msg.Replace("%", "%%");
   CString __msg;
   SYSTEMTIME tm;
   ::GetSystemTime(&tm);
   __msg.Format(_T("%i/%i/%i %i:%i:%i: %s"), (int)tm.wMonth, (int)tm.wDay, (int)tm.wYear, (int)tm.wHour, (int)tm.wMinute, (int)tm.wSecond, (LPCTSTR)_msg);
   Log(__msg);
   }




// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnUpdateApplicationLook)
	ON_COMMAND(ID_FILE_PRINT, &CMainFrame::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CMainFrame::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CMainFrame::OnFilePrintPreview)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW, &CMainFrame::OnUpdateFilePrintPreview)
   ON_COMMAND(ID_VIEW_OUTPUT, &CMainFrame::OnViewOutput)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUT, &CMainFrame::OnUpdateViewOutput)
	ON_COMMAND_RANGE(ID_AUTONOMOUS_PROCESS, ID_AUTONOMOUS_PROCESS+MAX_APS, &CMainFrame::OnCommandAP)
	ON_COMMAND_RANGE(ID_EVAL_MODEL, ID_EVAL_MODEL+MAX_EMS, &CMainFrame::OnCommandEM)
   ON_COMMAND_RANGE(ID_SCENARIOS, ID_SCENARIOS+MAX_SCENARIOS, &CMainFrame::OnSetScenario)
   ON_COMMAND_RANGE(ID_CONSTRAINTS, ID_CONSTRAINTS+MAX_CONSTRAINTS, &CMainFrame::OnSetConstraint)
   ON_COMMAND_RANGE(ID_EXT_ANALYSISMODULE, ID_EXT_ANALYSISMODULE+MAX_EXT_ANALYSISMODULES, &CMainFrame::OnCommandExtModule)
   ON_COMMAND_RANGE(ID_ZOOMS, ID_ZOOMS+MAX_ZOOMS, &CMainFrame::OnSetZoom)
   ON_UPDATE_COMMAND_UI_RANGE(ID_SCENARIOS, ID_SCENARIOS+MAX_SCENARIOS, &CMainFrame::OnUpdateScenarios)
   ON_UPDATE_COMMAND_UI_RANGE(ID_ZOOMS, ID_ZOOMS+MAX_ZOOMS, &CMainFrame::OnUpdateZooms)
   //ON_MESSAGE( AFX_WM_ON_CLOSEPOPUPWINDOW, &CMainFrame::OnCloseBalloon )

//   ON_COMMAND(ID_ANALYSIS_EXPORTCOVERAGES, &CMainFrame::OnAnalysisExportCoverages)
//	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_EXPORTCOVERAGES, &CMainFrame::OnUpdateAnalysisExportCoverages)
   
//   ON_COMMAND(ID_ANALYSIS_EXPORTOUTPUTS, &CMainFrame::OnAnalysisExportOutputs)
//	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_EXPORTOUTPUTS, &CMainFrame::OnUpdateAnalysisExportOutputs)
   
//   ON_COMMAND(ID_ANALYSIS_EXPORTINTERVAL, &CMainFrame::OnAnalysisExportInterval)
//	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_EXPORTINTERVAL, &CMainFrame::OnUpdateAnalysisExportInterval)
   
END_MESSAGE_MAP()

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
: m_pStatusMsg( NULL )
, m_pStatusMode( NULL )
, m_pZoomButton( NULL )
, m_pZoomSlider( NULL )
, m_pProgressBar( NULL )
{
	// TODO: add member initialization code here
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_OFF_2007_BLUE);
   gpMain = this;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook);

	m_wndRibbonBar.Create(this);
	m_wndStatusBar.Create(this);

  	InitializeRibbon();
   InitializeStatusBar();

   
	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

   CreateDockingWindows();

   //m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);
	//DockPane(&m_wndProperties);

	//Enable enhanced windows management dialog
	//EnableWindowsDialog(ID_WINDOW_MANAGER, IDS_WINDOWS_MANAGER, TRUE);


   //m_splitterWnd.LeftRight( m_pMapList, m_pMap );
   //m_splitterWnd.Create( NULL, NULL, WS_VISIBLE, CRect(0,0,0,0), this, 101 );


	return 0;
   }


BOOL CMainFrame::CreateDockingWindows()
   {
   CreateOutputWnd();

	// Create properties window
/*	CString strPropertiesWnd;
	bNameValid = strPropertiesWnd.LoadString(IDS_PROPERTIES_WND);
	ASSERT(bNameValid);
	if (!m_wndProperties.Create(strPropertiesWnd, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_PROPERTIESWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Properties window\n");
		return FALSE; // failed to create
	}
*/
	SetDockingWindowIcons(theApp.m_bHiColorIcons);
	return TRUE;
}


BOOL CMainFrame::CreateOutputWnd()
   {
   if ( m_wndOutput.m_hWnd != 0 )
      return TRUE;

   // Create output window
	if (!m_wndOutput.Create( _T("Output"), this, CRect(0, 0, 100, 100), FALSE, ID_VIEW_OUTPUTWND,
            WS_CHILD | WS_VISIBLE /*| WS_CLIPSIBLINGS | WS_CLIPCHILDREN*/ | CBRS_BOTTOM  /* | CBRS_FLOAT_MULTI */ ))
	   {
		TRACE0("Failed to create Output window\n");
		return FALSE; // failed to create
	   }
   
	// Enable the next two lines to allow the output pane to be draggable
   //m_wndOutput.EnableDocking(CBRS_ALIGN_BOTTOM);
	//EnableDocking(CBRS_ALIGN_BOTTOM);
	DockPane(&m_wndOutput);

   return TRUE;
   }


void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{
//	HICON hFileViewIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_FILE_VIEW_HC : IDI_FILE_VIEW), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
//	m_wndFileView.SetIcon(hFileViewIcon, FALSE);

//	HICON hClassViewIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_CLASS_VIEW_HC : IDI_CLASS_VIEW), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
//	m_wndModelObjectView.SetIcon(hClassViewIcon, FALSE);

	HICON hOutputBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_OUTPUT_WND_HC : IDI_OUTPUT_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	m_wndOutput.SetIcon(hOutputBarIcon, FALSE);

//	HICON hPropertiesBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_PROPERTIES_WND_HC : IDI_PROPERTIES_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
//	m_wndProperties.SetIcon(hPropertiesBarIcon, FALSE);

//	UpdateMDITabbedBarsIcons();
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}


void CMainFrame::InitializeRibbon()
   {
	BOOL bNameValid;

	CString strTemp;
	bNameValid = strTemp.LoadString(IDS_RIBBON_FILE);
	ASSERT(bNameValid);

	// Load panel images:
	m_PanelImages.SetImageSize(CSize(16, 16));
	m_PanelImages.Load(IDB_BUTTONS);

	// Init main button:
	m_MainButton.SetImage(IDB_MAIN);
	m_MainButton.SetText(_T("\nf"));
	m_MainButton.SetToolTipText(strTemp);

	m_wndRibbonBar.SetApplicationButton(&m_MainButton, CSize (45, 45));

   AddMainPanel();
   AddHomeCategory();
   AddDataCategory();
   AddMapCategory();
   AddRunCategory();

   AddContextTab_Results();
   AddContextTab_Analytics();

	// Add elements to the right side of tabs:
	CMFCRibbonButton* pVisualStyleButton = new CMFCRibbonButton(-1, _T("Style"), -1, -1);
	pVisualStyleButton->SetMenu(IDR_THEME_MENU, FALSE /* No default command */, TRUE /* Right align */);

	bNameValid = strTemp.LoadString(IDS_RIBBON_STYLE_TIP);
	ASSERT(bNameValid);
	pVisualStyleButton->SetToolTipText(strTemp);
	bNameValid = strTemp.LoadString(IDS_RIBBON_STYLE_DESC);
	ASSERT(bNameValid);
	pVisualStyleButton->SetDescription(strTemp);
	m_wndRibbonBar.AddToTabs(pVisualStyleButton);

	// Add quick access toolbar commands:
	CList<UINT, UINT> lstQATCmds;

	lstQATCmds.AddTail(ID_FILE_NEW);
	lstQATCmds.AddTail(ID_FILE_OPEN);
	lstQATCmds.AddTail(ID_FILE_SAVE);
	lstQATCmds.AddTail(ID_FILE_PRINT_DIRECT);

	m_wndRibbonBar.SetQuickAccessCommands(lstQATCmds);
	m_wndRibbonBar.AddToTabs(new CMFCRibbonButton(ID_APP_ABOUT, _T("\na"), m_PanelImages.ExtractIcon (0)));
   }


void CMainFrame::AddMainPanel()
   {
	CMFCRibbonMainPanel* pMainPanel = m_wndRibbonBar.AddMainCategory( _T("File"), IDB_FILESMALL, IDB_FILELARGE);

   // Main (file) Menu from Application Button
	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_NEW,     _T("New"), 0, 0));
	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_OPEN,    _T("Open"), 1, 1));
	
   CMFCRibbonButton *pBtnSave = new CMFCRibbonButton(ID_FILE_SAVE, _T("Save"), 2, 2);

   // Save submenu
   pBtnSave->AddSubItem( new CMFCRibbonLabel(_T("Save Envision Documents")));
   pBtnSave->AddSubItem( new CMFCRibbonButton( ID_FILE_SAVE,           _T("Save Project (.envx)"),           2,  2 ));
   pBtnSave->AddSubItem( new CMFCRibbonButton( ID_FILE_SAVEDATABASE,   _T("Save IDU Database"),         18, 18 ));
   pBtnSave->AddSubItem( new CMFCRibbonButton( ID_SAVE_SHAPEFILE,      _T("Save IDU Shape File"),   19, 19 ));
   pMainPanel->Add( pBtnSave );

   // save as
	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_SAVE_AS, _T("Save As"), 3, 3));

   pMainPanel->Add(new CMFCRibbonSeparator(TRUE));

   // import
   CMFCRibbonButton *pBtnImport = new CMFCRibbonButton(ID_FILE_IMPORT, _T("Import"), 16, 16);

   // import submenu
   pBtnImport->AddSubItem( new CMFCRibbonLabel(_T("Import Envision Items")));
   pBtnImport->AddSubItem( new CMFCRibbonButton( ID_IMPORT_POLICIES,           _T("Import Policies"),  23, 23 ));
   pBtnImport->AddSubItem( new CMFCRibbonButton( ID_IMPORT_ACTORS,             _T("Import Actors"),    21, 21 ));
   pBtnImport->AddSubItem( new CMFCRibbonButton( ID_IMPORT_LULC,               _T("Import LULC"),      15, 15 ));
   pBtnImport->AddSubItem( new CMFCRibbonButton( ID_IMPORT_SCENARIOS,          _T("Import Scenarios"), 14, 14 ));
   
   pBtnImport->AddSubItem( new CMFCRibbonLabel(_T("Import Envision Datasets")));
   pBtnImport->AddSubItem( new CMFCRibbonButton( ID_IMPORT_MULTIRUNDATASETS, _T("Import Multirun Dataset"), 18, 18 ));
   pMainPanel->Add( pBtnImport );

   // export
   CMFCRibbonButton *pBtnExport = new CMFCRibbonButton(ID_FILE_EXPORT, _T("Export"), 17, 17 );

   // export submenu
   pBtnExport->AddSubItem( new CMFCRibbonLabel(_T("Export Envision Items")));
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_POLICIES,       _T("Export Policies as XML"),  23, 23 ));
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_POLICIESASHTML, _T("Export Policies as HTML"), 24, 24 ));
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_ACTORS,         _T("Export Actors as XML"),    21, 21 ));
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_LULC,           _T("Export LULC as XML"),      15, 15 ));
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_SCENARIOS,      _T("Export Scenarios as XML"), 14, 14 ));

   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_FIELDINFO,      _T("Export Field Information as XML"), 27, 27 ));
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_FIELDINFOASHTML,_T("Export Field Information as HTML"), 28, 28 ));
      
   pBtnExport->AddSubItem( new CMFCRibbonLabel(_T("Export Datasets"))); 
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_MODELOUTPUTS,     _T("Export Model Output Datasets"), 18, 18 ));   
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_MULTIRUNDATASETS, _T("Export Multirun Datasets"), 18, 18 ));
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_DECADALCELLLAYERS,_T("Export Decadal Maps"), 19, 19 ));
   pBtnExport->AddSubItem( new CMFCRibbonButton( ID_EXPORT_DELTAARRAY,       _T("Export DeltaArray"), 25, 25 ));
   
   pMainPanel->Add( pBtnExport );

   CMFCRibbonButton* pBtnPrint = new CMFCRibbonButton(ID_FILE_PRINT, _T("Print"), 6, 6);
	pBtnPrint->SetKeys(_T("p"), _T("w"));

   // print submenu
	pBtnPrint->AddSubItem(new CMFCRibbonLabel(_T("Preview and Print the Document")));
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_DIRECT,  _T("Quick Print"),   7,  7, TRUE));
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_PREVIEW, _T("Print Preview"), 8,  8, TRUE));
	pBtnPrint->AddSubItem(new CMFCRibbonButton(ID_FILE_PRINT_SETUP,   _T("Print Setup"),  11, 11, TRUE));
	pMainPanel->Add(pBtnPrint);
	pMainPanel->Add(new CMFCRibbonSeparator(TRUE));

	pMainPanel->Add(new CMFCRibbonButton(ID_FILE_CLOSE, _T("Close"), 9, 9));

   pMainPanel->AddRecentFilesList( _T("Recent Documents") );

   pMainPanel->AddToBottom( new CMFCRibbonMainPanelButton(ID_OPTIONS, _T("Envision Options"), 26));
   pMainPanel->AddToBottom( new CMFCRibbonMainPanelButton(ID_HELP_UPDATEENVISION, _T("Update Envision"), 30));
   pMainPanel->AddToBottom(new CMFCRibbonMainPanelButton(ID_APP_EXIT, _T("Exit"), 29));
   }


void CMainFrame::AddHomeCategory()
   {

   //================ Main Ribbon =======================================================

   //--------------- FIRST CATEGORY - HOME ---------------------------- 
	// Add "Home" category with "Clipboard" panel:
	CMFCRibbonCategory* pCategoryHome = m_wndRibbonBar.AddCategory( _T("Home"), IDB_WRITESMALL, IDB_WRITELARGE);

   // Create and add a "View" panel:
   CMFCRibbonPanel *pPanelView = AddViewPanel( pCategoryHome );
   pPanelView->Add( new CMFCRibbonCheckBox(ID_VIEW_STATUS_BAR,   _T("Status Bar")));
	pPanelView->Add( new CMFCRibbonCheckBox(ID_VIEW_OUTPUT,       _T("Output Panel")));
	pPanelView->Add( new CMFCRibbonCheckBox(ID_VIEW_POLYGONEDGES, _T("Polygon Edges")));

	// Create "Clipboard" panel:
	CMFCRibbonPanel * pPanelClipboard = pCategoryHome->AddPanel( _T("Clipboard"), m_PanelImages.ExtractIcon(27));

	pPanelClipboard->Add(new CMFCRibbonButton(ID_EDIT_PASTE, _T("Paste"), 6, 6));
	pPanelClipboard->Add(new CMFCRibbonButton(ID_EDIT_CUT,   _T("Cut"), 7, 7));
	pPanelClipboard->Add(new CMFCRibbonButton(ID_EDIT_COPY,  _T("Copy"), 8, 8));
	//pPanelClipboard->Add(new CMFCRibbonButton(ID_EDIT_SELECT_ALL, _T("Select All"), 9,9));
	pPanelClipboard->Add(new CMFCRibbonButton(ID_EDIT_COPYLEGEND, _T("Copy Map Legend"), 10, 10));

   // Create "Setup" panel:
	CMFCRibbonPanel * pPanelSetup = pCategoryHome->AddPanel( _T("Setup"), m_PanelImages.ExtractIcon(27));

	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_INIFILE,         _T("Edit Project"),   11, 11));
	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_POLICIES,        _T("Edit Policies"),   13, 13));
	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_ACTORS,          _T("Edit Actors"),    14, 14));
	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_LULC,            _T("Edit LULC"),      12, 12));
	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_SCENARIOS,       _T("Edit Scenarios"), 15, 15));
	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_SETPOLICYCOLORS, _T("Set Policy Colors"), 16, 16));
	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_FIELDINFO,       _T("Edit Field Information"), 17, 17));

   m_pModulesButton = new CMFCRibbonButton(ID_EXT_ANALYSISMODULE,  _T("Analysis Modules"), 18, 18);
   pPanelSetup->Add( m_pModulesButton );
   }

void CMainFrame::AddDataCategory()
   {
   //---------------- SECOND CATEGORY - DATA PREP -------------------------
	CMFCRibbonCategory* pCategoryData = m_wndRibbonBar.AddCategory( _T("Data Preparation"), IDB_DATASMALL, IDB_DATALARGE);

   AddViewPanel( pCategoryData );

	// Create "Fields" panel:
	CMFCRibbonPanel* pPanelFields = pCategoryData->AddPanel(   _T("Database Structure"), m_PanelImages.ExtractIcon(27));

	pPanelFields->Add(new CMFCRibbonButton(ID_DATA_ADDCOL,     _T("Add Field(s)"), 11, 11));
   pPanelFields->Add(new CMFCRibbonButton(ID_DATA_REMOVECOL,  _T("Remove Field(s)"), 12, 12));
   pPanelFields->Add(new CMFCRibbonButton(ID_DATA_MERGE,      _T("Merge Datasets"), 13, 13));
   pPanelFields->Add(new CMFCRibbonButton(ID_DATA_CHANGETYPE, _T("Change Type"), 29, 29));

	// Create "Populate" panel:
	CMFCRibbonPanel* pPanelPopulate = pCategoryData->AddPanel( _T("Populate Fields"), m_PanelImages.ExtractIcon(27));

	pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_CALCULATEFIELD, _T("Calculate Field"), 14, 14));
	pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_LULC_FIELDS,    _T("LULC Hierarchy"), 15, 15));
	pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_DISTANCETO,     _T("Distance To..."), 16, 16));      // EnvDoc
	pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_ADJACENTTO,     _T("Adjacent To..."), 17, 17));      // EnvDoc
	pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_AREA,           _T("AREA Field"), 18, 18));
   pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_HISTOGRAM,      _T("Create Histograms"), 24, 24));       // EnvDoc
	pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_ELEVATION,      _T("Set Elevations"), 19, 19));           // EnvDoc
   pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_LINEARDENSITY,  _T("Linear Density"), 25, 25));      // EnvDoc
   pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_TEXT_TO_ID,     _T("Text to ID"), 26, 26));          // EnvDoc
   pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_INTERSECT,      _T("Intersect Layers"), 27, 27));           // EnvDoc
   pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_RECLASS,        _T("Reclass Values"), 31, 31));           // EnvDoc
   pPanelPopulate->Add(new CMFCRibbonButton(ID_DATA_IDUINDEX,       _T("Populate IDU_INDEX"), 32, 32));           // EnvDoc
   
	// Create "Other" panel:
	CMFCRibbonPanel* pPanelOther = pCategoryData->AddPanel( _T("Other"), m_PanelImages.ExtractIcon(27));

	pPanelOther->Add(new CMFCRibbonButton(ID_DATA_IDUINFO,                 _T("IDU Layer Info"), 20, 20));     // EnvView
   pPanelOther->Add(new CMFCRibbonButton(ID_DATA_IDUDOC ,                 _T("Document IDUs"), 5,5));     // EnvDoc
	pPanelOther->Add(new CMFCRibbonButton(ID_DATA_VERIFYCELLLAYER,         _T("Verify IDU Layer"), 21, 21));   // EnvDoc
	pPanelOther->Add(new CMFCRibbonButton(ID_DATA_PROJECT3D,               _T("Project to 3D"), 22, 22));      // EnvView
	pPanelOther->Add(new CMFCRibbonButton(ID_DATA_BUILDNETWORKTREE,        _T("Build Network Tree"), 23, 23)); // EnvDoc
	//pPanelOther->Add(new CMFCRibbonButton(ID_DATA_EVALUATIVEMODELLEARNING, _T("Eval Model Learning (move?)"), 1, 1)); // EnvDoc
	//pPanelOther->Add(new CMFCRibbonButton(ID_DATA_INITIALIZEPOLICYEFFICACIES, _T("Initialize Policy Efficacies (move?)"), 1, 1)); //EnvDoc
   }

void CMainFrame::AddMapCategory()
   { 
   //---------------- THIRD CATEGORY - MAP -------------------------
	CMFCRibbonCategory* pCategoryMap = m_wndRibbonBar.AddCategory( _T("Map"), IDB_MAPSMALL, IDB_MAPLARGE);

	// Create "View" panel:
   CMFCRibbonPanel *pPanelView = AddViewPanel( pCategoryMap );
   pPanelView->Add( new CMFCRibbonCheckBox(ID_VIEW_STATUS_BAR,   _T("Status Bar")));
	pPanelView->Add( new CMFCRibbonCheckBox(ID_VIEW_OUTPUT,       _T("Output Panel")));
	pPanelView->Add( new CMFCRibbonCheckBox(ID_VIEW_POLYGONEDGES, _T("Polygon Edges")));
   
	// Create "Navigate" panel:
	CMFCRibbonPanel* pPanelZoom = pCategoryMap->AddPanel( _T("Map Display"), m_PanelImages.ExtractIcon(27));

	pPanelZoom->Add(new CMFCRibbonButton(ID_ZOOMIN,       _T("Zoom In"),   11, 11)); // EnvView
   pPanelZoom->Add(new CMFCRibbonButton(ID_ZOOMOUT,      _T("Zoom Out"),  12, 12)); // EnvView
   pPanelZoom->Add(new CMFCRibbonButton(ID_ZOOMFULL,     _T("Zoom Full"), 13, 13)); // EnvView
   pPanelZoom->Add(new CMFCRibbonButton(ID_ZOOMTOQUERY,  _T("Zoom Query"),24, 24)); // EnvView
	pPanelZoom->Add(new CMFCRibbonButton(ID_PAN,          _T("Pan"),       14, 14)); // EnvView

   m_pZoomsCombo = new CMFCRibbonComboBox( ID_ZOOMS, 0, 120, "Zoom to:", -1 ); 
   pPanelZoom->Add( m_pZoomsCombo );
   pPanelZoom->Add( new CMFCRibbonCheckBox(ID_VIEW_SHOWSCALE,      _T("Show Map Scale")));
	pPanelZoom->Add( new CMFCRibbonCheckBox(ID_VIEW_SHOWATTRIBUTE,  _T("Show Attribute on Hover")));
   
   //pPanelZoom->Add( new CMFCRibbonLabel( _T("Zooms")));
   //pPanelZoom->Add( new CMFCRibbonLabel( _T("         ")));   

	// Create "Selection" panel:
	CMFCRibbonPanel *pPanelSelection = pCategoryMap->AddPanel( _T("Selection"), m_PanelImages.ExtractIcon(27));
   
   pPanelSelection->Add(new CMFCRibbonButton(ID_SELECT,          _T("Select"),          15, 15));   // EnvView
   pPanelSelection->Add(new CMFCRibbonButton(ID_CLEAR_SELECTION, _T("Clear Selection"), 16, 16));   // EnvView
   pPanelSelection->Add(new CMFCRibbonButton(ID_RUNQUERY,        _T("Query"),           18, 18));   // EnvView

   // Create "Other" panel:
   CMFCRibbonPanel *pPanelOther = pCategoryMap->AddPanel( _T("Other"), m_PanelImages.ExtractIcon(27));

   pPanelOther->Add(new CMFCRibbonButton(ID_QUERYDISTANCE,      _T("Measure"),         17, 17));   // EnvView
	pPanelOther->Add(new CMFCRibbonButton(ID_EDIT_FIELDINFO,     _T("Edit Field Information"), 19, 19));
   pPanelOther->Add(new CMFCRibbonButton(ID_MAP_AREASUMMARY,    _T("Area Summaries"), 21, 21));
   pPanelOther->Add(new CMFCRibbonButton(ID_MAP_DOTDENSITYMAP,  _T("Add Dot Density Map"), 22, 22));
   pPanelOther->Add(new CMFCRibbonButton(ID_MAP_CONVERT,        _T("Import/Export"), 23, 23));
   }


void CMainFrame::AddRunCategory()
   {
   //---------------- FOURTH CATEGORY - RUN -------------------------
	CMFCRibbonCategory* pCategoryRun = m_wndRibbonBar.AddCategory( _T("Run"), IDB_ANALYSIS_SMALL, IDB_ANALYSIS_LARGE);

	// Create "View" panel:
   AddViewPanel( pCategoryRun);

	// Create "Setup" panel:
	CMFCRibbonPanel *pPanelSetup = pCategoryRun->AddPanel( _T("Setup"), m_PanelImages.ExtractIcon(27));

	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_POLICIES,           _T("Edit Policies"),  11, 11));    // EnvView
	pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_ACTORS,             _T("Edit Actors"),    12, 12));    // EnvView
   pPanelSetup->Add(new CMFCRibbonButton(ID_EDIT_SCENARIOS,          _T("Edit Scenarios"), 13, 13));   // EnvView
   pPanelSetup->Add(new CMFCRibbonButton(ID_ANALYSIS_SETUP_RUN,      _T("Setup Runs"),     14, 14));             // EnvView
   m_pModelsButton = new CMFCRibbonButton(ID_ANALYSIS_SETUP_MODELS,  _T("Setup Models"),   15, 15);
   pPanelSetup->Add( m_pModelsButton );           // EnvView
   pPanelSetup->Add(new CMFCRibbonButton(ID_ANALYSIS_SENSITIVITY,    _T("Sensitivity Analysis"), 22, 22));             // EnvDoc

	// Create "Run" panel:
	CMFCRibbonPanel* pPanelRun = pCategoryRun->AddPanel( _T("Run Scenario"), m_PanelImages.ExtractIcon(27));

   //m_pScenariosButton = new CMFCRibbonButton(ID_SCENARIOS,_T("Scenario"), 12, 12);
   //pPanelRun->Add( m_pScenariosButton );

	pPanelRun->Add(new CMFCRibbonButton(ID_ANALYSIS_RUN,         _T("Single Run"),    16, 16));
   pPanelRun->Add(new CMFCRibbonButton(ID_ANALYSIS_RUNMULTIPLE, _T("Multiple Runs"), 17, 17));

   // create start/end year controls
   m_pStartYear = new CMFCRibbonEdit( ID_ANALYSIS_STARTYEAR, 52, _T("Starting Year: "), -1);
	m_pStartYear->SetEditText(_T("0"));

   m_pEndYear = new CMFCRibbonEdit( ID_ANALYSIS_STOPTIME,40, _T("Run for (years): "), -1);
	m_pEndYear->EnableSpinButtons(0, 100);
	m_pEndYear->SetEditText(_T("25"));

   // Scenarios combo
   pPanelRun->Add( new CMFCRibbonLabel( _T("Scenario to Run")));
   m_pScenariosCombo = new CMFCRibbonComboBox( ID_SCENARIOS, 0, -1, 0, -1 ); //_T("Scenarios"), 12 );
   pPanelRun->Add( m_pScenariosCombo );
   //pPanelRun->Add( new CMFCRibbonLabel( _T("         ")));   
	pPanelRun->Add( m_pStartYear );

   // Constraints combo
   pPanelRun->Add( new CMFCRibbonLabel( _T("Constrain to")));
   m_pConstraintsCombo = new CMFCRibbonComboBox( ID_CONSTRAINTS, 0, -1, 0, -1 ); //_T("Scenarios"), 12 );
   pPanelRun->Add( m_pConstraintsCombo );
	pPanelRun->Add( m_pEndYear );
   
   // create and populate the "Export Options" panel 
	CMFCRibbonPanel* pPanelRunControl = pCategoryRun->AddPanel( _T("Export Options"), m_PanelImages.ExtractIcon(26));

 	m_pExportCoverages = new CMFCRibbonCheckBox(ID_ANALYSIS_EXPORTCOVERAGES, _T("Export IDU Coverage"));


   m_pExportInterval  = new CMFCRibbonEdit( ID_ANALYSIS_EXPORTINTERVAL, 40, _T("Export Interval (yrs): "), -1);
	m_pExportInterval->EnableSpinButtons(0, 100);
   m_pExportOutputs  = new CMFCRibbonCheckBox(ID_ANALYSIS_EXPORTOUTPUTS, _T("Export Model Outputs"));
   m_pExportDeltas   = new CMFCRibbonCheckBox(ID_ANALYSIS_EXPORTDELTAS, _T("Export Delta Array"));
   m_pExportDeltaFields = new CMFCRibbonEdit( ID_ANALYSIS_EXPORTDELTAFIELDS, 120, _T("Fields: "), -1);
   m_pVideoRecorders = new CMFCRibbonButton( ID_ANALYSIS_VIDEORECORDERS, _T( "Video Recorders" ), 21, 21 );


   //pPanelRunControl->Add( new CMFCRibbonLabel( _T("Export Options")));

   pPanelRunControl->Add( m_pExportCoverages  );
   pPanelRunControl->Add( m_pExportOutputs    );
   pPanelRunControl->Add( m_pExportDeltas     );
   pPanelRunControl->Add( m_pExportInterval   );
   pPanelRunControl->Add( new CMFCRibbonLabel( _T(" ")));
   pPanelRunControl->Add( m_pExportDeltaFields  );
   pPanelRunControl->Add( m_pVideoRecorders );
      
   // Create "Results" panel
	CMFCRibbonPanel* pPanelResults = pCategoryRun->AddPanel( _T("Results"), m_PanelImages.ExtractIcon(27));

	pPanelResults->Add(new CMFCRibbonButton(ID_ANALYSIS_SHOWDELTALIST, _T("View Delta List"),  18, 18));
	pPanelResults->Add(new CMFCRibbonButton(ID_EXPORT_DELTAARRAY, _T("Export Delta List"),  19, 19));
	pPanelResults->Add(new CMFCRibbonButton(ID_EXPORT_MODELOUTPUTS, _T("Export Model Results"),  20, 20));
   }


CMFCRibbonPanel *CMainFrame::AddViewPanel( CMFCRibbonCategory *pCategory )
   {
	CMFCRibbonPanel* pPanelView = pCategory->AddPanel(_T("View"), m_PanelImages.ExtractIcon (7));

   pPanelView->Add(new CMFCRibbonButton(ID_VIEW_INPUT,     _T("Input Panel"), 0, 0 ));   
	pPanelView->Add(new CMFCRibbonButton(ID_VIEW_MAP,       _T("Main Map"), 1, 1 ));
   pPanelView->Add(new CMFCRibbonButton(ID_VIEW_TABLE,     _T("Data Table"), 2, 2 ));
   pPanelView->Add(new CMFCRibbonButton(ID_VIEW_RTVIEWS,   _T("Runtime Views"), 3, 3 ));
   pPanelView->Add(new CMFCRibbonButton(ID_VIEW_RESULTS,   _T("PostRun Results"), 4, 4));
   pPanelView->Add(new CMFCRibbonButton(ID_VIEW_ANALYTICS, _T("Analytics"), 5, 5));

   return pPanelView;
   }


void CMainFrame::AddContextTab_Results()
   {
	CMFCRibbonCategory* pCategory = m_wndRibbonBar.AddContextCategory(_T("Manage Results"), _T("Results Tools"), 
         IDCC_RESULTS, AFX_CategoryColor_Red, IDB_RESULTS_SMALL, IDB_RESULTS_LARGE);

	//pCategory->SetKeys(_T("jp"));
   CMFCRibbonPanel* pPanelArrange = pCategory->AddPanel(_T("Arrange"), m_PanelImages.ExtractIcon(20));

   pPanelArrange->Add(new CMFCRibbonButton( ID_RESULTS_TILE, _T("Tile"), 1, 1 ));
   pPanelArrange->Add(new CMFCRibbonButton( ID_RESULTS_CASCADE, _T("Cascade"), 0, 0 ));
   pPanelArrange->Add(new CMFCRibbonButton( ID_RESULTS_CLEAR, _T("Clear Windows"), 2, 2 ));
   pPanelArrange->Add(new CMFCRibbonCheckBox(ID_RESULTS_SYNC_MAPS, _T("Synchronize Maps Zooms")));
   
   CMFCRibbonPanel* pPanelTools = pCategory->AddPanel(_T("Tools"), m_PanelImages.ExtractIcon(20));
   //pPanelTools->Add(new CMFCRibbonCheckBox(ID_RESULTS_CAPTURE, _T("Capture Video During Play")));
   pPanelTools->Add(new CMFCRibbonCheckBox(ID_RESULTS_DELTAS, _T("Show Changed IDUs Only")));
   pPanelTools->Add( new CMFCRibbonLabel( _T("Constrain to")));
   m_pPostRunConstraintsCombo = new CMFCRibbonComboBox( ID_POSTRUN_CONSTRAINTS, 0, -1, 0, -1 ); //_T("Scenarios"), 12 );
   pPanelTools->Add( m_pPostRunConstraintsCombo );
   }


void CMainFrame::AddContextTab_RtViews()
   {
   CMFCRibbonCategory* pCategory = m_wndRibbonBar.AddContextCategory( _T("Runtime Views"), _T("Runtime View Tools"), 
      IDCC_RTVIEWS, AFX_CategoryColor_Blue, IDB_RTVIEWS_SMALL, IDB_RTVIEWS_LARGE);

	//pCategory->SetKeys(_T("jp"));
   CMFCRibbonPanel* pPanelViews = pCategory->AddPanel(_T("Views"), m_PanelImages.ExtractIcon(20));

   //rtview context tab consists of:
   // Panel:  views
   //           - maps: combo of maps
   //           - graphs: combo of graphs
   //         windows
   //            - Tile, Cascade, Delete
   //         Replay
   //            - rewind,pause etc...

   //------------- Maps drop down menu----------------------
   CMFCRibbonButton *pMapsBtn = new CMFCRibbonButton(ID_RTVIEW_MAPS, _T("Maps"), 0, 0 );
   pMapsBtn->SetDefaultCommand(0);

   // load up any field information registered with the documents field info
   Map *pMap = gpMapPanel->m_pMap;

   int id = 0;
   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );
      pMapsBtn->AddSubItem( new CMFCRibbonLabel(pLayer->m_name)); 

      for ( int j=0; j < pLayer->GetFieldInfoCount( 1 ); j++ )
         {
         MAP_FIELD_INFO *pInfo = pLayer->GetFieldInfo( j );
   
         ASSERT( pInfo != NULL );
         if ( pInfo->mfiType != MFIT_SUBMENU )
            {
            int col = pLayer->GetFieldCol( pInfo->fieldname );
            if ( col >= 0 )
               {
               LONG cmd = ( 200 * i ) + col;
               pMapsBtn->AddSubItem( new CMFCRibbonButton( ID_RTMAP_BEGIN+cmd, pInfo->label ));
               }
            id++;
            }
         }
      }

   pPanelViews->Add(pMapsBtn);

   //------- graphs drop down button--------------   

   // these are the same graphs as in the results window
	CMFCRibbonButton* pGraphsBtn = new CMFCRibbonButton(ID_RTVIEW_GRAPHS, _T("Graphs"), 1, 1);
   pGraphsBtn->SetDefaultCommand(0);

   ResultsPanel &resultsPanel = gpView->m_resultsPanel;
   //for ( int i=0; i < resultsPanel.m_resultInfo.GetSize(); i++ )
   //   {
   //   RESULT_INFO  &ri =resultsPanel.m_resultInfo[ i ];

   //   if  ( ri.category == 2)
   //      pGraphsBtn->AddSubItem(new CMFCRibbonButton( ID_RTGRAPH_START+ri.type, ri.text ));
   //   }
	pPanelViews->Add( pGraphsBtn );

   //-------- Views dropdown button ----------------

	CMFCRibbonButton* pViewsBtn = new CMFCRibbonButton(ID_RTVIEW_VIEWS, _T("Views"), 2, 2);
   pViewsBtn->SetDefaultCommand(0);

   // load up registered rtviews
   /*
   int id = 0;
   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );
      pMapsBtn->AddSubItem( new CMFCRibbonLabel(pLayer->m_name)); 

      for ( int j=0; j < pLayer->GetFieldInfoCount(); j++ )
         {
         MAP_FIELD_INFO *pInfo = pLayer->GetFieldInfo( j );
         int col = pLayer->GetFieldCol( pInfo->fieldname );
         if ( col >= 0 )
            {
            // only use level 0 in the main menu
            if ( pInfo->level == 0 )
               pMapsBtn->AddSubItem( new CMFCRibbonButton( ID_RTMAP_START+id, pInfo->label ));
            }
         id++;
         }
      }
      */

   pPanelViews->Add( pViewsBtn );

   //------------ Visualizers -------------
   CMFCRibbonButton *pRtVizBtn = new CMFCRibbonButton(ID_RTVIEW_VISUALIZERS, _T("Visualizers"), 0, 0 );
   pRtVizBtn->SetDefaultCommand(0);

   int vizCount = gpModel->GetVisualizerCount();

   for ( int i=0; i < vizCount; i++ )
      {
      EnvVisualizer *pInfo = gpModel->GetVisualizerInfo( i );

      if ( pInfo->m_vizType & VZT_RUNTIME )
         {         
         //pRtViewBtn->AddSubItem( new CMFCRibbonLabel(pLayer->m_name)); 
         pRtVizBtn->AddSubItem( new CMFCRibbonButton( ID_RTVIZ_BEGIN + i, pInfo->m_name ));
         id++;
         }
      }

   pPanelViews->Add(pRtVizBtn);


   //------------ next panel - Arrange ----------
   CMFCRibbonPanel* pPanelArrange = pCategory->AddPanel(_T("Arrange"), m_PanelImages.ExtractIcon(20));

   pPanelArrange->Add(new CMFCRibbonButton( ID_RTVIEW_TILE, _T("Tile"), 5, 5 ));
   pPanelArrange->Add(new CMFCRibbonButton( ID_RTVIEW_CASCADE, _T("Cascade"), 6, 6 ));
   pPanelArrange->Add(new CMFCRibbonButton( ID_RTVIEW_CLOSE, _T("Close"), 7, 7 ));

   // Next Panel - Controls
   CMFCRibbonPanel* pPanelControls = pCategory->AddPanel(_T("Controls"), m_PanelImages.ExtractIcon(20));
   pPanelControls->Add( new CMFCRibbonButton( ID_RTVIEW_REWIND, _T("Rewind"), 9, 9 ));
   pPanelControls->Add( new CMFCRibbonButton( ID_RTVIEW_START, _T("Start"), 10, 10 ));
   pPanelControls->Add( new CMFCRibbonButton( ID_RTVIEW_PAUSE, _T("Pause"), 11, 11 ));
   //pPanelControls->Add( new CMFCRibbonButton( ID_RTVIEW_END, _T("End"), 12, 12 ));
   }


void CMainFrame::AddContextTab_Analytics()
   {
	CMFCRibbonCategory* pCategory = m_wndRibbonBar.AddContextCategory(_T("Analytics"), _T("Analytics Tools"), 
         IDCC_ANALYTICS, AFX_CategoryColor_Blue, IDB_RESULTS_SMALL, IDB_RESULTS_LARGE);

	//pCategory->SetKeys(_T("jp"));
   CMFCRibbonPanel* pPanelArrange = pCategory->AddPanel(_T("Arrange"), m_PanelImages.ExtractIcon(20));

   pPanelArrange->Add(new CMFCRibbonButton( ID_ANALYTICS_TILE, _T("Tile"), 1, 1 ));
   pPanelArrange->Add(new CMFCRibbonButton( ID_ANALYTICS_CASCADE, _T("Cascade"), 0, 0 ));
   pPanelArrange->Add(new CMFCRibbonButton( ID_ANALYTICS_CLEAR, _T("Clear Windows"), 2, 2 ));
   //pPanelArrange->Add(new CMFCRibbonCheckBox(ID_RESULTS_SYNC_MAPS, _T("Synchronize Maps Zooms")));
   
   

   CMFCRibbonPanel* pPanelTools = pCategory->AddPanel(_T("Tools"), m_PanelImages.ExtractIcon(20));
   pPanelTools->Add(new CMFCRibbonButton( ID_ANALYTICS_ADD, _T("Add"), 1, 1 ));
   pPanelTools->Add(new CMFCRibbonCheckBox(ID_ANALYTICS_CAPTURE, _T("Capture Video During Play")));
   //pPanelTools->Add( new CMFCRibbonLabel( _T("Constrain to")));
   //m_pPostRunConstraintsCombo = new CMFCRibbonComboBox( ID_POSTRUN_CONSTRAINTS, 0, -1, 0, -1 ); //_T("Scenarios"), 12 );
   //pPanelTools->Add( m_pPostRunConstraintsCombo );
   }



/*
void CMainFrame::AddContextTab_DataTable()
   {
	CMFCRibbonCategory* pCategory = m_wndRibbonBar.AddContextCategory( _T("Arrange"), _T("Data Table Tools"), 
         IDCC_RESULTS, AFX_CategoryColor_Red, IDB_RESULTS_SMALL, IDB_RESULTS_LARGE);

	//pCategory->SetKeys(_T("jp"));
   CMFCRibbonPanel* pPanelArrange = pCategory->AddPanel(_T("Arrange"), m_PanelImages.ExtractIcon(20));

   pPanelArrange->Add(new CMFCRibbonButton( IDD_TILE, _T("Tile"), 1, 1 ));
   pPanelArrange->Add(new CMFCRibbonButton( IDD_CASCADE, _T("Cascade"), 0, 0 ));
   }
*/


void CMainFrame::ActivateRibbonContextCategory(UINT uiCategoryID)
   {
	if (m_wndRibbonBar.GetHideFlags() == 0)
	   {
		m_wndRibbonBar.ActivateContextCategory(uiCategoryID);
	   }
   }

void CMainFrame::SetRibbonContextCategory(UINT uiCategoryID)
   {
	BOOL bRecalc = m_wndRibbonBar.HideAllContextCategories();

	if (uiCategoryID != 0)
	   {
		m_wndRibbonBar.ShowContextCategories(uiCategoryID);
		bRecalc = TRUE;
	   }

	if (bRecalc)
	   {
		m_wndRibbonBar.RecalcLayout();
		m_wndRibbonBar.RedrawWindow();

		SendMessage(WM_NCPAINT, 0, 0);
	   }
   }


void CMainFrame::AddModels()
   {
   ASSERT( m_pModelsButton != NULL );

   m_pModelsButton->RemoveAllSubItems();

   m_pModelsButton->AddSubItem( new CMFCRibbonLabel(_T("Autonomous Processes")));

   for ( int i=0; i < gpModel->GetModelProcessCount(); i++ )
      {
      //if ( gpModel->GetModelProcessInfo( i )->setupFn != NULL )
      //   {
      //   CString str( gpModel->GetModelProcessInfo( i )->name );
      //   str += " Properties...";   
      //   m_pModelsButton->AddSubItem( new CMFCRibbonButton( ID_AUTONOMOUS_PROCESS+i, str, 18, -1, 1 ) );
      //   }
      }

   m_pModelsButton->AddSubItem( new CMFCRibbonLabel(_T("Evaluative Models")));

   for ( int i=0; i < gpModel->GetEvaluatorCount(); i++ )
      {
      //if ( gpModel->GetEvaluatorInfo( i )->setupFn != NULL )
      //   {
      //   CString str( gpModel->GetEvaluatorInfo( i )->name );
      //   str += " Properties...";   
      //   m_pModelsButton->AddSubItem( new CMFCRibbonButton( ID_EVAL_MODEL+i, str, 19, -1, 1 ) );
      //   }
      }
   }


void CMainFrame::AddAnalysisModules()
   {
   ASSERT( m_pModulesButton != NULL );

   m_pModulesButton->RemoveAllSubItems();

   m_pModulesButton->AddSubItem( new CMFCRibbonLabel(_T("Analysis Modules")));

   for ( int i=0; i < gpDoc->GetAnalysisModuleCount(); i++ )
      {
      EnvAnalysisModule *pInfo = gpDoc->GetAnalysisModule( i );

      if ( pInfo->m_use )
         {
         CString str( "Run " );
         str += pInfo->m_name;
         m_pModulesButton->AddSubItem( new CMFCRibbonButton( ID_EXT_ANALYSISMODULE+i, str, 18, -1, 1 ) );
         }
      }
   }


void CMainFrame::AddZooms( int curSelection )
   {
   m_pZoomsCombo->RemoveAllItems();


   for ( int i=0; i < gpView->GetZoomCount(); i++ )
      {
      ZoomInfo *pInfo = gpView->GetZoomInfo( i );

      m_pZoomsCombo->AddItem( pInfo->m_name);
      }

   m_pZoomsCombo->AddItem( _T("Capture Current View") );

   m_pZoomsCombo->SelectItem( curSelection );
   }



void CMainFrame::AddScenarios()
   {

   //m_pScenariosButton->RemoveAllSubItems();

   for ( int i=0; i < gpScenarioManager->GetCount(); i++ )
      {
      Scenario *pScenario = gpScenarioManager->GetScenario( i );
      //m_pScenariosButton->AddSubItem( new CMFCRibbonButton( ID_SCENARIOS+i, pScenario->m_name, 19, -1, 1 ) );

      m_pScenariosCombo->AddItem(pScenario->m_name);
      }

   m_pScenariosCombo->AddItem( _T("Run All Scenarios") );

   m_pScenariosCombo->SelectItem(0);
   }


void CMainFrame::AddConstraints()
   {
   m_pConstraintsCombo->RemoveAllItems();
   m_pPostRunConstraintsCombo->RemoveAllItems();

   m_pConstraintsCombo->AddItem( _T("No Constraints (run everywhere)") );
   m_pPostRunConstraintsCombo->AddItem( _T("No Constraints (run everywhere)") );

   for ( int i=0; i < gpDoc->GetConstraintCount(); i++ )
      {
      m_pConstraintsCombo->AddItem( gpDoc->GetConstraint( i ) );
      m_pPostRunConstraintsCombo->AddItem( gpDoc->GetConstraint( i ) );
      }

   m_pConstraintsCombo->EnableDropDownListResize( 1 );
   m_pPostRunConstraintsCombo->EnableDropDownListResize( 1 );
 
   m_pConstraintsCombo->AddItem( _T("--Use Current Selection--") );
   m_pConstraintsCombo->AddItem( _T("--Add/Edit Constraints--") );
   m_pConstraintsCombo->SelectItem(0);

   m_pConstraintsCombo->AddItem( _T("--Use Current Selection--") );
   m_pPostRunConstraintsCombo->AddItem( _T("--Add/Edit Constraints--") );
   m_pPostRunConstraintsCombo->SelectItem(0);
   }


void CMainFrame::InitializeStatusBar()
   {
   // message pane
   m_pStatusMsg = new CMFCRibbonStatusBarPane(ID_STATUSBAR_PANE1, _T(""), TRUE);
   m_pStatusMsg->SetAlmostLargeText( _T("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM") );
	m_wndStatusBar.AddElement(m_pStatusMsg, _T("") );
   m_wndStatusBar.AddSeparator();

   // mode pane
   m_pStatusMode = new CMFCRibbonStatusBarPane(ID_STATUSBAR_PANE2, _T("Mode"), TRUE);
	m_wndStatusBar.AddExtendedElement( m_pStatusMode, _T(""));
   m_pStatusMode->SetAlmostLargeText( _T("MMMMMMMMMMMMMMM") );
   m_wndStatusBar.AddSeparator();

   // multirun progress bar
   m_pMultirunProgressBar = new CMFCRibbonProgressBar( ID_STATUSBAR_MULTIRUNPROGRESSBAR, 80 );
   m_pMultirunProgressBar->SetInfiniteMode(FALSE);
   m_pMultirunProgressBar->SetRange(0,100);
   m_pMultirunProgressBar->SetPos(0,true);
   m_wndStatusBar.AddExtendedElement( m_pMultirunProgressBar, _T(""));
   m_pMultirunProgressBar->SetVisible( TRUE );

   // progress bar
   m_pProgressBar = new CMFCRibbonProgressBar( ID_STATUSBAR_PROGRESSBAR, 80 );
   m_pProgressBar->SetInfiniteMode(FALSE);
   m_pProgressBar->SetRange(0,100);
   m_pProgressBar->SetPos(0,true);
   m_wndStatusBar.AddExtendedElement( m_pProgressBar, _T(""));
   m_pProgressBar->SetVisible( TRUE );

   // Create a ribbon slider for the "zoom" control and add to the status bar.
   m_pZoomButton = new CMFCRibbonButton( ID_STATUSBAR_ZOOMBUTTON, _T("100%") );
   m_wndStatusBar.AddExtendedElement( m_pZoomButton, _T("100%"));
   m_pZoomButton->SetToolTipText( _T("Zoom Button - click to open the Zoom dialog box") );

   m_pZoomSlider = new CMFCRibbonSlider( ID_STATUSBAR_ZOOMSLIDER, 100 );
   // Set the various properties of the slider.
   m_pZoomSlider->SetZoomButtons(true);
   m_pZoomSlider->SetRange(0, 100);
   m_pZoomSlider->SetPos(0, TRUE);
   m_wndStatusBar.AddExtendedElement( m_pZoomSlider, "" );
   m_pZoomSlider->SetToolTipText( _T("Increase or decrease the zoom level"));
   }


void CMainFrame::SetStatusZoom(int percent)
   {
   if ( m_pZoomSlider )
      m_pZoomSlider->SetPos( percent );

   if ( m_pZoomButton )
      {
      CString strPercent;
      strPercent.Format( "%d%%", percent );
      m_pZoomButton->SetText( strPercent );
      m_pZoomButton->Redraw();
      }
   }


void CMainFrame::SetCaption( Scenario *pScenario )
   {
   CString caption( "Envision " );

   nsPath::CPath filename( gpDoc->m_iniFile );
   CString title = filename.GetTitle();
   caption += title;

   if ( pScenario != NULL )
      {
      caption += " (";
      caption += pScenario->m_name;
      caption += ")";
      }

   SetWindowText( caption );
   }


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

void CMainFrame::OnFilePrint()
{
	if (IsPrintPreview())
	{
		PostMessage(WM_COMMAND, AFX_ID_PREVIEW_PRINT);
	}
}

void CMainFrame::OnFilePrintPreview()
{
	if (IsPrintPreview())
	{
		PostMessage(WM_COMMAND, AFX_ID_PREVIEW_CLOSE);  // force Print Preview mode closed
	}
}

void CMainFrame::OnUpdateFilePrintPreview(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(IsPrintPreview());
}


void CMainFrame::OnCommandAP(UINT id)
   {
   EnvModelProcess *pInfo = gpModel->GetModelProcessInfo( id-ID_AUTONOMOUS_PROCESS );            
   ASSERT( pInfo != NULL );
   
   EnvContext *pContext = &(gpDoc->m_model.m_envContext);
   pContext->id = pInfo->m_id;
   pContext->pMapLayer = gpCellLayer;
   
   //pInfo->setupFn( pContext, gpView->GetSafeHwnd() ); 
   }

void CMainFrame::OnCommandEM(UINT id)
   {
   EnvEvaluator *pInfo = gpModel->GetEvaluatorInfo( id-ID_EVAL_MODEL );            
   ASSERT( pInfo != NULL );
   //ASSERT( pInfo->setupFn != NULL );

   EnvContext *pContext = &(gpDoc->m_model.m_envContext);
   pContext->id = pInfo->m_id;
   pContext->pMapLayer = gpCellLayer;
   
   //pInfo->setupFn( pContext, gpView->GetSafeHwnd() ); 
   }


void CMainFrame::OnCommandExtModule(UINT id)
   {
   EnvAnalysisModule *pInfo = gpDoc->GetAnalysisModule( id-ID_EXT_ANALYSISMODULE );            
   ASSERT( pInfo != NULL );
   //ASSERT( pInfo->runFn != NULL );

   EnvContext e( gpCellLayer );
   e.pActorManager  = gpActorManager;
   e.pEnvExtension = pInfo;
   e.pLulcTree      = &gpModel->m_lulcTree;
   e.pPolicyManager = gpPolicyManager;
   e.ptrAddDelta    = NULL;
   //pInfo->Run( &e, gpView->m_hWnd, pInfo->m_initInfo ); 
   }



void CMainFrame::OnSetZoom(UINT /*id*/)
   {
   // zoom selected. Could be an existing selction or "add current view".
   // If the latter, get a name and store it.  Otherwise, refresh the current view.
   int zoomIndex = GetCurrentlySelectedZoom();

   // get current map
   Map *pMap = NULL;
   if ( ActiveWnd::IsMapFrame() )
      {
      MapFrame *pMapFrame = ActiveWnd::GetActiveMapFrame();
      ASSERT( pMapFrame != NULL );
      pMap = pMapFrame->m_pMap;
      }
   else
      {
      pMap = gpView->m_mapPanel.m_pMap;
      }

   // "Add New Zoom"
   if ( zoomIndex < 0 )
      {
      NameDlg dlg( gpView );

      if ( dlg.DoModal() == IDCANCEL )
         return;

      ASSERT( pMap != NULL );
      REAL xMin, xMax, yMin, yMax;
      pMap->GetViewExtents( xMin, xMax, yMin, yMax );

      int zoomIndex = gpView->AddZoom( dlg.m_name, xMin, yMin, xMax, yMax );

      // add it to the view combo and select
      AddZooms( zoomIndex );
      }
   else  // it is one of the existing zooms.  Zoom the active window to the zoom.
      gpView->SetZoom( zoomIndex );
   }


int CMainFrame::GetCurrentlySelectedZoom()
   {
   int zoom = m_pZoomsCombo->GetCurSel();
   ASSERT( zoom < gpView->GetZoomCount()+1 );
   if ( zoom >= gpView->GetZoomCount() )
      zoom = -1;  // capture current view

   return zoom;
   }


void CMainFrame::OnSetScenario(UINT id)
   {
   int scenario = GetCurrentlySelectedScenario();
   gpDoc->m_model.SetScenario( gpScenarioManager->GetScenario( scenario ) );
   }


int CMainFrame::GetCurrentlySelectedScenario()
   {
   int scenario = m_pScenariosCombo->GetCurSel();
   ASSERT( scenario < gpScenarioManager->GetCount()+1 );
   if ( scenario == gpScenarioManager->GetCount() )
      scenario = -1;

   return scenario;
   }


void CMainFrame::OnSetConstraint(UINT id)
   {
   ASSERT( id == ID_CONSTRAINTS || id == ID_POSTRUN_CONSTRAINTS );

   int count = -1;
   int index = -1;

   switch( id )
      {
      case ID_CONSTRAINTS:
         {
         count = (int) m_pConstraintsCombo->GetCount();
         index = m_pConstraintsCombo->GetCurSel();

         if ( index == 0 )
            gpDoc->m_model.SetConstraint( NULL );   // run everywhere
      
         else if ( index == count-1 ) // last item (add/edit constraints)
            {
            ConstraintsDlg dlg;
            dlg.DoModal();
            }
         else if ( index == count-2 ) // second to last item (use current selection)
            {
            gpDoc->m_model.SetConstraint( "0" );
            }
         else
            {
            CString constraint =  m_pConstraintsCombo->GetText();
            gpDoc->m_model.SetConstraint( constraint );
            }
         }
         break;

      case ID_POSTRUN_CONSTRAINTS:
         {
         count = (int) m_pConstraintsCombo->GetCount();
         index = m_pConstraintsCombo->GetCurSel();

         if ( index == 0 )
            gpDoc->m_model.SetPostRunConstraint( NULL );   // run everywhere
      
         else if ( index == count-1 ) // last item (add/edit constraints)
            {
            ConstraintsDlg dlg;
            dlg.DoModal();
            }
         else if ( index == count-2 ) // second to last item (use current selection)
            {
            gpDoc->m_model.SetPostRunConstraint( "0" ) ;
            }
         else
            {
            CString constraint =  m_pPostRunConstraintsCombo->GetText();
            //gpDoc->SetPostRunConstraint( constraint );
            }
         }
         break;
      }

   }


void CMainFrame::OnViewOutput()
   {
   int isVisible = m_wndOutput.IsVisible();

   isVisible = isVisible ? 0 : 1; // flip it

   ShowPane(&m_wndOutput, isVisible, FALSE, TRUE);
	RecalcLayout ();

   if ( gpDoc) 
      gpDoc->m_outputWndStatus = isVisible ? 1 : 0;  
   }


void CMainFrame::OnUpdateViewOutput( CCmdUI *pCmdUI )
   {
   bool setCheck = false;

   if ( m_wndOutput.m_hWnd != 0 && m_wndOutput.IsVisible() )
      setCheck = true;

   pCmdUI->SetCheck( setCheck ? 1 : 0 );
   }

//LRESULT CMainFrame::OnCloseBalloon( WPARAM, LPARAM )
//   {
//   AfxMessageBox( "Closing Balloon" );
//
//   return 1;
//   }


/*
void CMainFrame::OnAnalysisExportOutputs()
   {
   //bool exportOutputs = gpDoc->m_model.m_exportOutputs;
   //gpDoc->m_model.m_exportOutputs = ! exportOutputs;
   
   bool checked = this->m_pExportOutputs->IsChecked() ? true : false;
   gpDoc->m_model.m_exportOutputs = checked;
   }


void CMainFrame::OnUpdateAnalysisExportOutputs( CCmdUI *pCmdUI )
   {
   bool setCheck = false;

   if ( gpDoc->m_model.m_exportOutputs )
      setCheck = true;

   pCmdUI->SetCheck( setCheck ? 1 : 0 );
   }


void CMainFrame::OnAnalysisExportCoverages()
   {
   //bool exportOutputs = gpDoc->m_model.m_exportOutputs;
   //gpDoc->m_model.m_exportOutputs = ! exportOutputs;
   
   //bool checked = this->m_pExportOutputs->IsChecked() ? true : false;
   //gpDoc->m_model.m_exportOutputs = checked;
   }


void CMainFrame::OnUpdateAnalysisExportCoverages( CCmdUI *pCmdUI )
   {
   bool setCheck = false;

   if ( gpDoc->m_model.m_exportMapInterval > 0 )
      setCheck = true;

   pCmdUI->SetCheck( setCheck ? 1 : 0 );
   }

void CMainFrame::OnAnalysisExportInterval()
   {
   //bool exportOutputs = gpDoc->m_model.m_exportOutputs;
   //gpDoc->m_model.m_exportOutputs = ! exportOutputs;
   
   //bool checked = this->m_pExportOutputs->IsChecked() ? true : false;
   //gpDoc->m_model.m_exportOutputs = checked;
   }


void CMainFrame::OnUpdateAnalysisExportInterval( CCmdUI *pCmdUI )
   {
   pCmdUI->Enable( this->m_pExportCoverages->IsChecked() );
   }
   */


void CMainFrame::AddDynamicMenus( void )
   {
   AddAnalysisModules();
   AddModels();
   AddScenarios();
   AddZooms( 0 );
   AddConstraints();
   AddContextTab_RtViews();
   }

   
void CMainFrame::OnUpdateScenarios(CCmdUI* pCmdUI)
   {/*
   // main button?
   if ( pCmdUI->m_nID == ID_SCENARIOS )
      {
      CString scenario( "No Scenario" );

      if ( gpDoc != NULL && gpDoc->m_model.GetScenario() != NULL )      
         pCmdUI->SetText( gpDoc->m_model.GetScenario()->m_name );

      else
         pCmdUI->SetText( _T( "No Scenario" ) );
      }
   else
      return; */
   }
   

void CMainFrame::OnUpdateZooms(CCmdUI* pCmdUI)
   {/*
   // main button?
   if ( pCmdUI->m_nID == ID_SCENARIOS )
      {
      CString scenario( "No Scenario" );

      if ( gpDoc != NULL && gpDoc->m_model.GetScenario() != NULL )      
         pCmdUI->SetText( gpDoc->m_model.GetScenario()->m_name );

      else
         pCmdUI->SetText( _T( "No Scenario" ) );
      }
   else
      return; */
   }
   

BOOL CMainFrame::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
   {
   //if ( wParam == AFX_WM_ON_CLOSEPOPUPWINDOW )
   //   {
   //   //AfxMessageBox( "OnNotify" );
   //   }

   return CFrameWndEx::OnNotify(wParam, lParam, pResult);
   }


void CMainFrame::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
   {
   char lsChar = char(nChar);
   WORD ctrl = HIWORD( (DWORD) GetKeyState( VK_CONTROL ) );

   // toggle output window
   if ( ctrl && ( lsChar == 'o' || lsChar == 'O' ) ) 
      {
      OnViewOutput();
      return;
      }

	CFrameWndEx::OnKeyDown(nChar, nRepCnt, nFlags);
   }


void CMainFrame::ShowOutputPanel( bool show /*=true*/ )
   {
   ShowPane( &m_wndOutput, show ? 1 : 0, FALSE, TRUE ); 
   RecalcLayout ();

   if ( gpDoc) 
      gpDoc->m_outputWndStatus = show==true ? 1 : 0;  
   }
