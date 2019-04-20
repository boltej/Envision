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

// EnvDoc.cpp : implementation of the CEnvDoc class
//

#include "stdafx.h"

#include "resource.h"
#include ".\EnvDoc.h"
#include "EnvView.h"
#include "EvaluatorLearning.h"
#include "mainfrm.h"
#include "InputPanel.h"
#include "MapPanel.h"
#include "maplistwnd.h"
#include <Policy.h>
#include <DataManager.h>
#include "WizRun.h"
#include <EnvException.h>
#include "splashdlg.h"
#include <Scenario.h>
#include "mergedlg.h"
#include "ChangeTypeDlg.h"
#include "adddbcoldlg.h"
#include "RemoveDbColsDlg.h"
#include "multisandboxdlg.h"
#include "FieldInfoDlg.h"
#include "FieldCalculatorDlg.h"
#include "NetworkTopologyDlg.h"
#include "IniFileEditor.h"
#include "WizardDlg.h"
#include "PopulateDistanceDlg.h"
#include "PopulateAdjacencyDlg.h"
#include "HistogramCDF.h"
#include "OptionsDlg.h"
#include "SpatialIndexDlg.h"
#include "ActiveWnd.h"
#include <Actor.h>
#include "histogramdlg.h"
#include "AreaSummaryDlg.h"
#include "LinearDensityDlg.h"
#include "DeltaExportDlg.h"
#include "SaveProjectDlg.h"
#include "EnvExtension.h"
#include "ConvertDlg.h"
#include "Envision.h"
#include "UpdateDlg.h"
#include "SensitivityDlg.h"
#include "envmsg.h"
#include <EnvConstants.h>
#include <EnvLoader.h>

#include <fdataobj.h>
#include <map.h>
#include <maplayer.h>
#include <DBTABLE.H>
#include <queryengine.h>
#include <dirplaceholder.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <path.h>
#include <folderdlg.h>
#include "TextToIdDlg.h"
#include "IntersectDlg.h"
#include "ReclassDlg.h"
#include <tixml.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <GeoSpatialDataObj.h>
#include "EnvisionAPI.h"

//#include "WebServices\soapEnvWebServiceSoapProxy.h"
//#include "WebServices\EnvWebServiceSoap.nsmap"

using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CEnvDoc      *gpDoc       = NULL;
MapLayer     *gpCellLayer = NULL;
EnvModel     *gpModel     = NULL;

extern InputPanel   *gpInputPanel;
extern MapPanel      *gpMapPanel;
extern CEnvView     *gpView;
extern CMainFrame   *gpMain;
extern ResultsPanel *gpResultsPanel;
extern ActorManager *gpActorManager;

extern CEnvApp theApp;
extern ENVSETTEXT_PROC envSetLLMapTextProc;
extern ENVREDRAWMAP_PROC envRedrawMapProc;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//__EXPORT__ Map3d         *gpMap3d;              // defined in Map3d.h

PolicyManager   *gpPolicyManager   = NULL;
ActorManager    *gpActorManager    = NULL;
ScenarioManager *gpScenarioManager = NULL;

// static members
CString CEnvDoc::m_iniFile( "envision.envx" );       // ini file name - default is evoland.ini
CString CEnvDoc::m_databaseName( "" );             // database file name - no default
CStringArray CEnvDoc::m_fieldInfoFileArray;        // field Info file name - no default


//bool showMessage = true;

void SetBoolHtml( CString &html, LPCTSTR label, bool value );

void NotifyProc( EM_NOTIFYTYPE type, INT_PTR, INT_PTR, INT_PTR );
void NotifyProc( EM_NOTIFYTYPE type, INT_PTR param, INT_PTR extra, INT_PTR modelID)
   {
   switch( type )
      {
      case EMNT_IDUCHANGE:
         if ( param == 1 )
            gpDoc->OnDataArea();

         gpDoc->SetChanged( CHANGED_COVERAGE );
         break;

      case EMNT_INITRUN:   // param=current run number (0-based)
         gpMapPanel->m_mapFrame.Update();   
         gpMain->SetRunProgressPos( 0 );
         gpResultsPanel->AddRun( (int) param );
         gpView->m_viewPanel.SetRun( (int) param );
         gpView->m_viewPanel.SetYear( gpModel->m_currentYear );
         gpView->StartVideoRecorders();
         gpView->UpdateVideoRecorders();   // initial capture
         break;

      case EMNT_ENDSTEP:
         gpMain->SetRunProgressPos( (int)  param );
         break;

      case EMNT_INIT:
         break;

      case EMNT_ACTORLOOP:
         if ( param == 0 && gpModel->m_showRunProgress )
            gpMain->SetModelMsg( "Actors");
         break;

      case EMNT_RUNEVAL:
      case EMNT_RUNAP:
      case EMNT_RUNVIZ:
         if ( param == 1 && gpModel->m_showRunProgress )  // in model loop?
            {
            EnvExtension *pInfo = (EnvExtension*) extra;
            gpMain->SetModelMsg( pInfo->m_name );
            gpView->UpdateUI( (int) 8, extra );
            }
         else if ( param == 2 && gpModel->m_showRunProgress )
            {
            EnvExtension *pInfo = (EnvExtension*) extra;
            CString msg( pInfo->m_name );
            msg += " - Completed";
            gpMain->SetModelMsg( msg );
            }
         else if ( param == 3 && gpModel->m_showRunProgress )
            {
            gpMain->SetModelMsg( "" );
            }
         break;

      case EMNT_ENDRUN:
         gpView->EndRun();
         break;

      case EMNT_SETSCENARIO:
         {
         Scenario *pScenario = (Scenario*) param;
         gpMain->SetCaption( pScenario );
         }
         break;

      case EMNT_MULTIRUN:
         if ( param == 0 )
            gpResultsPanel->AddMultiRun( (int) extra );
         else if ( param == 1 )
            gpMain->SetMultiRunProgressPos( (int) extra );
         break;

      case EMNT_UPDATEUI:
         gpView->UpdateUI( (int) param, extra );
         break;
      }
   }


#ifndef _WIN64
void GetDBValue( CDaoRecordset &rs, LPCTSTR field, long &value );
void GetDBValue( CDaoRecordset &rs, LPCTSTR field, int &value );
void GetDBValue( CDaoRecordset &rs, LPCTSTR field, float &value );
void GetDBValue( CDaoRecordset &rs, LPCTSTR field, CString &value );
void GetDBValue( CDaoRecordset &rs, LPCTSTR field, bool &value );
#endif 

/////////////////////////////////////////////////////////////////////////////
// CEnvDoc

IMPLEMENT_DYNCREATE(CEnvDoc, CDocument)

BEGIN_MESSAGE_MAP(CEnvDoc, CDocument)
   // Application menu
   ON_COMMAND(ID_OPTIONS, &CEnvDoc::OnOptions)

   // save
   ON_COMMAND(ID_FILE_SAVEDATABASE, OnFileSavedatabase)
   ON_UPDATE_COMMAND_UI(ID_FILE_SAVEDATABASE, OnUpdateIsDocOpen)

   // import
   ON_UPDATE_COMMAND_UI(ID_IMPORT_POLICIES,  OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_IMPORT_ACTORS,    OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_IMPORT_LULC,      OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_IMPORT_SCENARIOS, OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_IMPORT_MULTIRUNDATASETS, OnUpdateImportMultirunDataset)
   
   ON_COMMAND(ID_IMPORT_POLICIES,   OnImportPolicies)
   ON_COMMAND(ID_IMPORT_ACTORS,     OnImportActors)
   ON_COMMAND(ID_IMPORT_LULC,       OnImportLulc)
   ON_COMMAND(ID_IMPORT_SCENARIOS,  OnImportScenarios)
   ON_COMMAND(ID_IMPORT_MULTIRUNDATASETS, OnImportMultirunDataset)

   // export
   ON_UPDATE_COMMAND_UI(ID_FILE_EXPORT,       OnUpdateFileExport)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_ACTORS,     OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_POLICIES,   OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_LULC,       OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_SCENARIOS,  OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_POLICIESASHTML, OnUpdateIsDocOpen)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_MODELOUTPUTS, OnUpdateExportModelOutputs)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_MULTIRUNDATASETS, OnUpdateExportMultirunDataset)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_DECADALCELLLAYERS, OnUpdateExportDecadalcelllayers)
   ON_UPDATE_COMMAND_UI(ID_EXPORT_DELTAARRAY, OnUpdateExportDeltaArray)

   ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
   ON_COMMAND(ID_EXPORT_ACTORS,        OnExportActors)
   ON_COMMAND(ID_EXPORT_POLICIES,      OnExportPolicies)
   ON_COMMAND(ID_EXPORT_LULC,          OnExportLulc)
   ON_COMMAND(ID_EXPORT_SCENARIOS,     OnExportScenarios)
   ON_COMMAND(ID_EXPORT_POLICIESASHTML,OnExportPoliciesashtmldocument)
   ON_COMMAND(ID_EXPORT_MODELOUTPUTS, OnExportModelOutputs)
   ON_COMMAND(ID_EXPORT_MULTIRUNDATASETS, OnExportMultirunDataset)
   ON_COMMAND(ID_EXPORT_DECADALCELLLAYERS, OnExportDecadalcelllayers)
   ON_COMMAND(ID_EXPORT_FIELDINFO,OnExportFieldInfo)
   ON_COMMAND(ID_EXPORT_FIELDINFOASHTML,OnExportFieldInfoAsHtml)   
   ON_COMMAND(ID_EXPORT_DELTAARRAY,OnExportDeltaArray)   

   // Data menu
   ON_COMMAND(ID_DATA_LULC_FIELDS,        &CEnvDoc::OnDataLulcFields)
   ON_COMMAND(ID_DATA_VERIFYCELLLAYER,    &CEnvDoc::OnDataVerifyCellLayer)
   ON_COMMAND(ID_DATA_INITIALIZEPOLICYEFFICACIES, &CEnvDoc::OnDataInitializePolicyEfficacies)
   ON_COMMAND(ID_DATA_AREA,               &CEnvDoc::OnDataArea)
   ON_COMMAND(ID_DATA_MERGE,              &CEnvDoc::OnDataMerge)
   ON_COMMAND(ID_DATA_CHANGETYPE,         &CEnvDoc::OnDataChangeType)
   ON_COMMAND(ID_DATA_ADDCOL,             &CEnvDoc::OnDataAddCol)
   ON_COMMAND(ID_DATA_CALCULATEFIELD,     &CEnvDoc::OnDataCalculateField)
   ON_COMMAND(ID_DATA_REMOVECOL,          &CEnvDoc::OnDataRemoveCol)
   ON_COMMAND(ID_DATA_BUILDNETWORKTREE,   &CEnvDoc::OnDataBuildnetworktree)
   ON_COMMAND(ID_DATA_DISTANCETO,         &CEnvDoc::OnDataDistanceTo)
   ON_COMMAND(ID_DATA_ADJACENTTO,         &CEnvDoc::OnDataNextTo)
   ON_COMMAND(ID_DATA_PROJECT3D,          &CEnvDoc::OnDataProjectTo3d)
   ON_COMMAND(ID_DATA_ELEVATION,          &CEnvDoc::OnDataElevation)
   ON_COMMAND(ID_DATA_LINEARDENSITY,      &CEnvDoc::OnDataLinearDensity)
   ON_COMMAND(ID_DATA_TEXT_TO_ID,         &CEnvDoc::OnDataTextToID)
   ON_COMMAND(ID_DATA_INTERSECT,          &CEnvDoc::OnDataIntersect)
   ON_COMMAND(ID_DATA_RECLASS,            &CEnvDoc::OnDataReclass)
   ON_COMMAND(ID_DATA_IDUINDEX,           &CEnvDoc::OnDataPopulateIduIndex)
   ON_COMMAND(ID_DATA_HISTOGRAM,          &CEnvDoc::OnDataHistogram)

   // Analysis menu
   ON_COMMAND(ID_MAP_AREASUMMARY,         &CEnvDoc::OnMapAreaSummary)
   ON_COMMAND(ID_MAP_DOTDENSITYMAP,       &CEnvDoc::OnMapAddDotDensityMap)
   ON_COMMAND(ID_MAP_CONVERT,             &CEnvDoc::OnMapConvert)

   // Run menu
   ON_COMMAND(ID_ANALYSIS_SENSITIVITY,    &CEnvDoc::OnSensitivityAnalysis)
   
   // Other

   // Not figured out yet
   
   ON_COMMAND(ID_FILE_LOADMAP, OnFileLoadmap)
   ON_COMMAND(ID_ANALYSIS_RUN, OnAnalysisRun)
   ON_UPDATE_COMMAND_UI(ID_ANALYSIS_RUN, OnUpdateAnalysisRun)

   ON_COMMAND(ID_ANALYSIS_STOP, OnAnalysisStop)
   ON_UPDATE_COMMAND_UI(ID_ANALYSIS_STOP, OnUpdateAnalysisStop)
   ON_COMMAND(ID_ANALYSIS_RESET, OnAnalysisReset)
   ON_UPDATE_COMMAND_UI(ID_ANALYSIS_RESET, OnUpdateAnalysisReset)
   ON_COMMAND(ID_ANALYSIS_RUNMULTIPLE, OnAnalysisRunmultiple)
   ON_UPDATE_COMMAND_UI(ID_ANALYSIS_RUNMULTIPLE, OnUpdateAnalysisRunMultiple)
   
   ON_COMMAND(ID_EDIT_FIELDINFO, &CEnvDoc::OnEditFieldInformation)
   ON_COMMAND(ID_EDIT_INIFILE, &CEnvDoc::OnEditIniFile)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnvDoc construction/destruction

CEnvDoc::CEnvDoc()
 : m_coupleSliders( false ),
   m_isChanged( 0 ),
   m_loadSharedPolicies( 1 ),
   m_errors( 0 ),
   m_warnings( 0 ),
   m_infos( 0 ),
   m_cmdRunScenario( -2 ),
   m_cmdMultiRunCount( 0 ),
   m_cmdNoExit( false ),          // avoid terminating after run/multirun 
   m_outputWndStatus( 0 ),
   m_runEval( false )
   {
   InitDoc();
   }

bool CEnvDoc::InitDoc( void )
   {
   gpDoc = this;

   ASSERT( gpPolicyManager == NULL );
   ASSERT( gpActorManager == NULL );
   ASSERT( gpScenarioManager == NULL );
   
   gpPolicyManager   = m_model.m_pPolicyManager;
   gpActorManager    = m_model.m_pActorManager;
   gpScenarioManager = m_model.m_pScenarioManager;

   gpModel = &m_model;
   gpModel->SetNotifyProc( NotifyProc, 0 );

   Report::logMsgProc    = LogMsgProc;
   Report::statusMsgProc = StatusMsgProc;
   Report::popupMsgProc  = PopupMsgProc;

   ::envSetLLMapTextProc = EnvSetLLMapTextProc;
   ::envRedrawMapProc = EnvRedrawMapProc;

   return true;
   }


CEnvDoc::~CEnvDoc()
   {
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

   //m_outputWndStatus = gpMain->IsOutputOpen() ? 1 : 0;
   result = RegSetValueEx( key, "OutputWndStatus", 0, REG_DWORD, (LPBYTE) &m_outputWndStatus, sizeof( DWORD ) );

   Clear();
   gpDoc = NULL;
   }


void CEnvDoc::Clear()
   {
   //= 16, CHANGED_OTHER=32 };

   
   //if ( gfpLogFile != NULL )
   //   {
   //   fclose( gfpLogFile );
   //   gfpLogFile = NULL;
   //   }
   Report::CloseFile();

   if ( m_model.m_pPolicyManager )
      m_model.m_pPolicyManager->RemoveAllPolicies();

   if (m_model.m_pActorManager)
      m_model.m_pActorManager->RemoveAll();

   if (m_model.m_pScenarioManager)
      m_model.m_pScenarioManager->RemoveAll();
   
   //if ( gpQueryEngine != NULL )
   //   {
   //   delete gpQueryEngine;
   //   gpQueryEngine = NULL;
   //   }

   m_isChanged = 0;
   m_model.m_areModelsInitialized = false;
   
   m_analysisModuleArray.Clear();
   m_dataModuleArray.Clear();
   m_visualizerArray.Clear();
   }


BOOL CEnvDoc::OnNewDocument()
   {
   if (!CDocument::OnNewDocument())
      return FALSE;

   // TODO: add reinitialization code here
   // (SDI documents will reuse this document)

   return TRUE;
   }



/////////////////////////////////////////////////////////////////////////////
// CEnvDoc serialization

void CEnvDoc::Serialize(CArchive& ar)
{
   if (ar.IsStoring())
   {
      // TODO: add storing code here
   }
   else
   {
      // TODO: add loading code here
   }
}

/////////////////////////////////////////////////////////////////////////////
// CEnvDoc diagnostics

#ifdef _DEBUG
void CEnvDoc::AssertValid() const
{
   CDocument::AssertValid();
}

void CEnvDoc::Dump(CDumpContext& dc) const
{
   CDocument::Dump(dc);
}
#endif //_DEBUG

#include <MovingWindow.h>

/////////////////////////////////////////////////////////////////////////////
// CEnvDoc commands
// open up a project (.envx) file.  This file contains information about layers to load

int CEnvDoc::StartUp()
   {
   CWaitCursor c;

   if ( CheckForUpdates() )
      {
      }

   //AddGDALPath();

   bool isNewProject = false;
   bool runIniEditor = false;

   m_iniFile = m_cmdLoadProject;

   if ( this->m_cmdLoadProject.IsEmpty() )
      {
      char dir[ _MAX_PATH ];
      _getcwd( dir, _MAX_PATH );
   
      SplashDlg dlg;
      if ( dlg.DoModal() == IDCANCEL )
         return ( m_exitCode = -1 );
   
      _chdir( dir );

      m_userName = dlg.m_name;
      isNewProject = dlg.m_isNewProject;
      runIniEditor = dlg.m_runIniEditor;
      m_iniFile = dlg.m_filename;
      }
   
   int outputWndStatus = m_outputWndStatus;
   gpMain->ShowOutputPanel( true );

   gpDoc = this;
  
   if ( isNewProject )
      {
      m_loadSharedPolicies = 1;

      gpActorManager->m_actorAssociations = 0;
      gpActorManager->m_actorInitMethod = AIM_NONE;

      m_model.m_logMsgLevel = 0;
      m_model.m_pDataManager->multiRunDecadalMapsModulus = 10;
      m_model.m_dynamicUpdate = 1 + 2 + 4 + 8;
      m_model.m_yearsToRun = 25;

      //BufferOutcomeFunction::m_enabled = false;

      CWaitCursor c;
      ASSERT ( gpMapPanel != NULL );
      m_model.StoreIDUCols();    // requires models needed to be loaded before this point
      m_model.VerifyIDULayer();     // requires StoreCellCols() to be call before this is called, but not models     

      ReconcileDataTypes();      // require map layer to be loaded, plus map field infos?

      // set all layers MAP_UNITS
      gpMapPanel->m_pMap->m_mapUnits = MU_UNKNOWN;

      for ( int i=0; i < gpMapPanel->m_pMap->GetLayerCount(); i++ )
         {
         MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( i ) ;
         pLayer->m_mapUnits = MU_UNKNOWN;
         }

      m_model.m_envContext.pMapLayer = m_model.m_pIDULayer;
      gpDoc->UnSetChanged( CHANGED_ACTORS | CHANGED_POLICIES | CHANGED_SCENARIOS );

      if ( runIniEditor )
         {
         IniFileEditor editor;
         editor.DoModal();
         }
      }  // end of: if ( new project )
   else
      { 
      BOOL ok = OnOpenDocument( m_iniFile );   // calls EnvLoader

      if ( ! ok )
         return m_exitCode;
      }

   // set up bins for loaded layer(s)
   if ( gpMapPanel != NULL )
      {
      Map *pMap = gpMapPanel->m_pMap;

      for ( int i=0; i < pMap->GetLayerCount(); i++ )
         {
         if ( pMap->GetLayer( i )->m_pData != NULL )
            Map::SetupBins( pMap, i, -1 );
         }

      gpMapPanel->m_mapFrame.RefreshList();
      }

   gpMain->SetStatusMsg( _T("") );

   // add a field for each actor value; cf CEnvDoc::CEnvDoc()
   for ( int i=0; i < gpModel->GetActorValueCount(); i++ )
      {
      METAGOAL *pInfo =  gpModel->GetActorValueMetagoalInfo( i );
      CString label( pInfo->name );
      label += " Actor Value";

      CString field;
      field.Format( "ACTOR%s", pInfo->name );
      if (field.GetLength() > 10)
         field.Truncate(10);
      field.Replace( ' ', '_' );

      m_model.m_pIDULayer->AddFieldInfo( field, 0, label, _T(""), _T(""), TYPE_FLOAT, MFIT_QUANTITYBINS, BCF_GREEN_INCR , 0, true );
      }
    
   int defaultScenario = gpScenarioManager->GetDefaultScenario();
   m_model.SetScenario( gpScenarioManager->GetScenario( defaultScenario ) );  /* -- */

   LoadExtensions();

/* --*/
   // update main interface
   gpMain->AddDynamicMenus();

   // No better time than now to execute this.
   //if ( m_spatialIndexDistance > 0 && m_model.m_pIDULayer->LoadSpatialIndex( NULL, (float) m_spatialIndexDistance ) < 0 )
   //   {
   //   SpatialIndexDlg dlg;
   //   dlg.m_maxDistance = m_spatialIndexDistance;

   //   if ( dlg.DoModal() == IDOK )
   //      m_spatialIndexDistance = dlg.m_maxDistance;
   //   }

   // reset EnvModel
   m_model.Reset();

   gpMain->SetCaption( NULL );

   int levels = gpModel->m_lulcTree.GetLevels();

   switch( levels )
      {
      case 1:  m_model.m_pIDULayer->CopyColData( gpModel->m_colStartLulc, gpModel->m_colLulcA );   break;
      case 2:  m_model.m_pIDULayer->CopyColData( gpModel->m_colStartLulc, gpModel->m_colLulcB );   break;
      case 3:  m_model.m_pIDULayer->CopyColData( gpModel->m_colStartLulc, gpModel->m_colLulcC );   break;
      case 4:  m_model.m_pIDULayer->CopyColData( gpModel->m_colStartLulc, gpModel->m_colLulcD );   break;
      }

   // any errors during load?
   CString msg;
   CString error;
   CString warning;
   if ( m_errors > 0 )
      {
      if (m_errors == 1 )
         error = "1 Error ";
      else
         error.Format( "%i Errors ", m_errors );
      }

   if ( m_warnings > 0 )
      {
      if (m_warnings == 1 )
         warning = "1 Warning ";
      else
         warning.Format( "%i Warnings ", m_warnings );
      }
        
   if ( m_errors > 0 && m_warnings > 0 )
      msg.Format( "%s and %s \nwere encountered during startup.", (LPCTSTR) error, (LPCTSTR) warning );
   else if ( m_errors > 0 )
      {
      if ( m_errors == 1 )
         msg = "1 Error was \nencountered during startup.";
      else
         msg.Format( "%i Errors were \nencountered during startup.", m_errors );
      }
   else if ( m_warnings > 0 )
      {
      if ( m_warnings == 1 )
         msg = "1 Warning was encountered during startup";
      else
         msg.Format( "%i Warnings were \nencountered during startup.", m_warnings );
      }

   if ( ! msg.IsEmpty() )
      {
      msg += " Check the Output Panel for details...";

      Report::BalloonMsg( msg, RT_WARNING );
      gpMain->SetStatusMsg( msg );
      }

   if ( m_errors > 0 )
      outputWndStatus = 1;

   gpMain->ShowOutputPanel( outputWndStatus == 1 ? true : false );
   gpMain->RedrawWindow();

   gpCellLayer->ClearSelection();
   gpMapPanel->RedrawWindow(); /* -- */


   // if there are any relevant command line cmds, process them
   if ( this->m_cmdRunScenario >= 0 )
      {
      // load the specified scenario
      if ( this->m_cmdRunScenario == 0 ) // run all scenarios
         {
         int count = (int) gpMain->m_pScenariosCombo->GetCount();
         gpMain->m_pScenariosCombo->SelectItem( count-1 );  // run all is last
         }
      else
         gpMain->m_pScenariosCombo->SelectItem( this->m_cmdRunScenario-1 );  // m_cmdRunScenario is 1-based

      // single run?
      if ( this->m_cmdMultiRunCount <= 1 )
         OnAnalysisRun(); 
      else
         {
         // multirun specified at command line
         this->m_model.m_iterationsToRun = this->m_cmdMultiRunCount;
         OnAnalysisRunmultiple();         
         }

      m_exitCode = this->m_cmdNoExit ? 1 : -10;

      return m_exitCode;
      }

   return 0;
   }


BOOL CEnvDoc::OnOpenDocument(LPCTSTR lpszPathName) 
   {
   if (!CDocument::OnOpenDocument(lpszPathName))
      return FALSE;

   m_iniFile = lpszPathName;

   nsPath::CPath path( lpszPathName );

   CString ext = path.GetExtension();
   int result;
   if ( ext.CompareNoCase( _T( "envx" ) ) == 0 )
      result = OpenDocXml( lpszPathName );
   else
      {
      AfxMessageBox( _T( "Unrecognized file extension - Envision cannot continue." ), MB_OK );
      result = -1;
      }

   if ( result < 0 )
      {
      m_exitCode = result;
      return FALSE;
      }
 
   return TRUE;
   }


int CEnvDoc::SaveFieldInfoXML( MapLayer *pLayer, LPCTSTR filename )
   {
   // write header
   ofstream file;
   file.open( filename );

   // standard header
   file << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl << endl;

   file << "<!-- " << endl;
   file << "==================================================================================" << endl;
   file << "<field> attributes" << endl;
   file << "label:  readable field description" << endl;
   file << "  level:  menu level - 0 = first level menu, 1 = second level menu, " << endl;
   file << "   displayFlag:  bin color flags - " << endl;
   file << "               BCF_MIXED=0, BCF_RED_INCR=1, BCF_GREEN_INCR=2, BCF_BLUE_INCR=3, " << endl;
   file << "               BCF_GRAY_INCR=4, BCF_SINGLECOLOR=5, BCF_TRANSPARENT=6," << endl;
   file << "               BCF_BLUEGREEN=7, BCF_BLUERED=8, BCF_REDGREEN=9," << endl;
   file << "               BCF_RED_DECR=10, BCF_GREEN_DECR=11, BCF_BLUE_DECR=12, " << endl;
   file << "               BCF_GRAY_DECR=13" << endl;
   file << "       mfiType: 0=quantity bins, 1=category bins" << endl;
   file << "       binStyle: 0=linear, 1=equal counts" << endl;
   file << "       min:      minValue of the collection" << endl;
   file << "       max:      maxValue of the collection" << endl;
   file << "       results:  1=show map of this attribute in the PostRun results tab,0=don't show" << endl;
   file << "       decadalMaps:  0=don't include in decadal map out, > 0 = include in decadal outputs" << endl;
   file << "       useInSiteAttr:  1=include attribute in site attribute editor, 0=don't include" << endl;
   file << "       useInOutcomes:  1=include attribute in outcome editor, 0=don't include" << endl;
   file << "==================================================================================" << endl;
   file << "-->" << endl << endl;

   file << "<fields layer=\"" << pLayer->m_name << "\">" << endl;

   file.precision( 3 );

   for ( int i=0; i < pLayer->GetFieldInfoCount(1); i++ )
      {
      // write field information.  These can be of two types, <field> and <submenu>
      MAP_FIELD_INFO *pInfo = pLayer->GetFieldInfo( i );

      if ( pInfo->mfiType == MFIT_SUBMENU )
         {
         if ( pInfo->save == false )
            continue;
   
         file << "\t<submenu label='" << pInfo->label << "' >" << endl;

         // write any associated submenu fields
         for ( int j=0; j < pLayer->GetFieldInfoCount(1); j++ )
            {
            if ( i == j )
               continue;

            // write field information for children of this SubMenu
            MAP_FIELD_INFO *pChildInfo = pLayer->GetFieldInfo( j );
      
            if ( pChildInfo->pParent == pInfo && pChildInfo->save == true )
                  SaveFieldXml( file, pChildInfo, pLayer, 1 );
            }

         // submenu closing tag
         file << "\t</submenu>" << endl;
         }
      else
         {
         if ( pInfo->pParent == NULL )
            SaveFieldXml( file, pInfo, pLayer, 0 );
         }  // end of: else ( NOT SubMenu )
      }  // end of: for ( i < m_pFieldInfoArray->GetCount() )

   file << "</fields>" << endl;

   file.close();

   return pLayer->GetFieldInfoCount( 1 );
   }


bool CEnvDoc::SaveFieldXml( std::ofstream &file, MAP_FIELD_INFO *pInfo, MapLayer *pLayer, int tabCount )
   {
   if ( pLayer->GetFieldCol( pInfo->fieldname ) == -1 )
      {
      CString msg( _T("The field [" ) );
      msg += pInfo->fieldname;
      msg += _T("] was not found in the MapLayer.  Do you want to exclude it from being saved?");
   
      int ok = AfxMessageBox( msg, MB_YESNO );
   
      if ( ok == IDYES )
         pInfo->save = false;
      }
   
   if ( pInfo->save == false )
      return false;
   
   int results  = pInfo->GetExtraBool( 1 ) ? 1 : 0;    // these are in the low word
   int siteAttr = pInfo->GetExtraBool( 2 ) ? 1 : 0;
   int outcomes = pInfo->GetExtraBool( 4 ) ? 1 : 0;
   int mfi      = int( pInfo->mfiType );
   
   int col = pLayer->GetFieldCol( pInfo->fieldname );
   TYPE type = pInfo->type;
   if ( type == TYPE_NULL && col > 0 )
      type = pLayer->GetFieldType( col );
   
   LPCTSTR _type = ::GetTypeLabel( type );

   CString indent = "";
   if ( tabCount > 0 )
      indent = "\t";
   
   file << "\t"     << indent << "<field col=\""  << pInfo->fieldname
      << "\"\n\t\t" << indent << "label=\""        << pInfo->label 
      << "\"\n\t\t" << indent << "displayFlag=\""  << pInfo->displayFlags
      << "\"\n\t\t" << indent << "datatype=\""     << _type
      << "\"\n\t\t" << indent << "mfiType=\""      << mfi
      << "\"\n\t\t" << indent << "binStyle=\""     << pInfo->bsFlag; 
      
   if ( type != TYPE_STRING && type != TYPE_DSTRING )
      file << "\"\n\t\t" << indent << "min=\"" << pInfo->binMin << "\"\n\t\t max=\"" << pInfo->binMax;
   
   file << "\"\n\t\t" << indent << "results=\""    << results
        << "\"\n\t\t" << indent << "decadalMaps=\""  << pInfo->GetExtraHigh()
        << "\"\n\t\t" << indent << "useInSiteAttr=\""<< siteAttr
        << "\"\n\t\t" << indent << "useInOutcomes=\""<< outcomes
        << "\">" << endl;
   
   CString description;
   GetXmlStr( pInfo->description, description );
   file <<  "\t\t" << indent <<"<description>";
   file << "\n\t\t\t" << indent << description;
   file << "\n\t\t" << indent << "</description>" << endl;
   
   CString source;
   GetXmlStr( pInfo->source, source );
   file << "\t\t" << indent << "<source>";
   file << "\n\t\t\t" << indent << source;
   file << "\n\t\t" << indent << "</source>" << endl;
   
   // write attribute values for this field
   file << "\t\t" << indent << "<attributes>" << endl;
   
   for ( int j=0; j < pInfo->attributes.GetCount(); j++ )
      {
      FIELD_ATTR &a = pInfo->GetAttribute( j );
   
      CString label;
      GetXmlStr( a.label, label );
   
      file << "\t\t\t" << indent << "<attr";
   
      if ( pInfo->mfiType == MFIT_CATEGORYBINS )
         file << " value=\"" << a.value.GetAsString() << "\"";
      else
         file << " minVal=\"" << a.minVal << "\" maxVal=\"" << a.maxVal << "\"";
         
      file << " color=\"(" << int (GetRValue(a.color)) << "," << int( GetGValue( a.color )) << "," << int( GetBValue( a.color )) << ")\"";
      if ( a.transparency != 0 )
         file << " transparancy=\"" << a.transparency << "\"";
      file << " label=\"" << label << "\" />" << endl;
      }
   
   file << "\t\t" << indent << "</attributes>" << endl;
   file << "\t" << indent << "</field>\n" << endl;
   
   return true;
   }

/*
bool CEnvDoc::LoadActors()
   {
   CString msg( "Loading Actor File " );
   msg += gpActorManager->m_path;
   gpMain->SetStatusMsg( msg );

   if ( gpActorManager->m_path.IsEmpty() )
      {
      Report::ErrorMsg( "No Actor File Specified - Envision will proceed with no actors..." );
      return false;
      }
   
   gpActorManager->LoadXml( gpActorManager->m_path, false, false );
   gpMain->SetStatusMsg( "Actors successfully loaded!" );
   return true;
   }
    

 bool CEnvDoc::LoadPolicies()
   {
   CString msg( "Loading Policy File " );
   msg += gpPolicyManager->m_path;
   gpMain->SetStatusMsg( msg );
   
   if ( gpPolicyManager->m_path.IsEmpty() )
      {
      Report::ErrorMsg( "No Policy File Specified - Envision will proceed with no policies..." );
      return false;
      }
   
   gpPolicyManager->LoadXml( gpPolicyManager->m_path, false, false );
   gpMain->SetStatusMsg( "Compiling Policies" );
   gpPolicyManager->CompilePolicies( m_model.m_pIDULayer );
   gpMain->SetStatusMsg( "Policies successfully loaded and compiled!" );
   return true;
   }

   */

//====================================================================================
namespace {
   enum { 
      BY_UNKNOWN, 
      BY_IDU_ATTRIBUTE_ACTORGROUP, // ACTORGROUPs assigned to "ACTORGROUP" col; omit 
      BY_LULC_PREDICATE            // Use LULC predicate from Actor DB
   };


int HowAssignActorsToCells()
   {
   /*
      int nRet = BY_UNKNOWN;
      for (int i=0; i < gpActorManager->GetActorGroupCount(); i++ )
      {
         nRet = BY_LULC_PREDICATE;
         ActorGroup *pAG = gpActorManager->GetActorGroup(i);
         if (-1 == pAG->m_lulcClass || -1 == pAG->m_lulcLevel) 
         {
            nRet = BY_IDU_ATTRIBUTE_ACTORGROUP;
            break;
         }
      }
      return nRet; */ return 1;         
   }
}


void CEnvDoc::OnFileLoadmap() 
   {
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "Shape files|*.shp|Grid Files|*.grd|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );

      ASSERT ( gpMapPanel != NULL );
      ASSERT ( gpMapPanel->m_pMap != NULL );

      Map *pMap = gpMapPanel->m_pMap;

      CString ext = dlg.GetFileExt();

      MapLayer *pLayer = NULL;

      if ( ext.CompareNoCase( "shp" ) == 0 )
         pLayer = pMap->AddShapeLayer( filename, true );  // load data as well...

      else if ( ext.CompareNoCase( "grd" ) == 0 )
         pLayer = pMap->AddGridLayer( filename, DOT_INT );

      else
         Report::WarningMsg( "Unsupported Map Type - Only Shape (*.shp) and Grid (*.grd) files are currently supported..." );

      // layer added succesfully?
      if ( pLayer != NULL )
         {
         //ASSERT ( gpMapPanel != NULL );
         //ASSERT ( gpMapPanel->m_pMapTree != NULL );
         //gpMapPanel->m_pMapTree->AddNode( pLayer );            
         //ASSERT ( gpMapPanel->m_pMapList != NULL );
         //gpMapPanel->m_pMapList->AddLayer( pLayer->m_name );
         
         SetModifiedFlag( TRUE );    // let the framework now the internal doc data has been changed
         }

      ASSERT ( gpMapPanel != NULL );
      gpMapPanel->m_mapFrame.RefreshList();
      }

   _chdir( cwd );
   free( cwd );
   }


BOOL CEnvDoc::OnSaveDocument(LPCTSTR lpszPathName) 
   {
   SaveProjectDlg dlg;
   dlg.m_projectPath = lpszPathName;

   if ( dlg.DoModal() == IDOK )
      SaveAs( dlg.m_projectPath );

   return true; //CDocument::OnSaveDocument(lpszPathName);
   }


int CEnvDoc::SaveAs( LPCTSTR _filename  /* if NULL, prompt for path */ )
   {
   ASSERT( gpPolicyManager != NULL );
   ASSERT( gpScenarioManager != NULL );

   char filename[ 256 ];

   if ( _filename == NULL )
      {
      DirPlaceholder d;

      SaveProjectDlg dlg;
      //CFileDialog dlg( FALSE, "envx", m_iniFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
      //   "Project Files|*.envx|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         lstrcpy( filename, dlg.m_projectPath );
      else
         return 0;
      }
   else
      lstrcpy( filename, _filename );
 
   // open the file and write contents
   FILE *fp;
   fopen_s( &fp, filename, "wt" );
   if ( fp == NULL )
      {
      LONG s_err = GetLastError();
      CString msg = "Failure to open " + CString(filename) + " for writing.  ";
      Report::SystemErrorMsg  ( s_err, msg );
      return -1;
      }

   // ready to start writing
   //int noBuffer = BufferOutcomeFunction::m_enabled ? 0 : 1;

   fputs( "<?xml version='1.0' encoding='utf-8' ?>\n\n", fp );
   fputs( "<Envision ver='6.0'>\n\n", fp );

   // settings
   fputs( "<!--\n", fp );
   fputs( "==========================================================================================================\n", fp );
   fputs( "                               S E T T I N G S \n", fp );
   fputs( "==========================================================================================================\n", fp );
   fputs( " actorInitMethod: Specifies how acters are initialized\n", fp );
   fputs( "    0 = no actors\n", fp );
   fputs( "    1 = based on weights specified in the IDU coverage\n", fp );
   fputs( "    2 = based on groups defined in the ACTOR field in the IDU coverage\n", fp );
   fputs( "    3 = based on a spatial querys defined for the actor groups\n", fp );
   fputs( "    4 = use a single, uniform actor\n", fp );
   fputs( "    5 = generate random actors (not fully supported at this time\n", fp );
   fputs( " \n", fp );
   fputs( " actorAssociations:    0=disable, 1=enable \n", fp );
   fputs( " loadSharedPolicies:   0=disable, 1=enable shared policies \n", fp );   
   fputs( " debug:                0=use debug mode, 1=no debug mode \n", fp );
   fputs( " startYear:            0= ignore, otherwise specific start year (e.g. 2012) \n", fp );
   fputs( " startRunNumber:       starting run number (default=0)\n", fp );
   fputs( " logMsgLevel:          0=log everything, 1=log errors, 2=log warnings, 4=log infos, add together as necessary \n", fp );
   //fputs( " noBuffering:          0=disable polygon subdivision during Buffer(), 1=enable subdivision \n", fp );
   fputs( " multiRunDecadalMapsModulus: output frequency (years) for maps during multiruns \n", fp );
   fputs( " defaultPeriod:        default simulation period (years) \n", fp );
   fputs( " multiRunInterations:  default number of runs in a multirun \n", fp );
   fputs( " dynamicUpdate:        flag indicating whether display should be updated dynamically 0=no update,1=update views,2=update main map, 3=update both \n", fp );
   fputs( " spatialIndexDistance: distance to build spatialindex to speed up spatial operation.  0=disable, gt 0 = distance to use\n", fp );
   fputs( " areaCutoff:           minimum area of a polygon at which a label will be shown \n", fp );
   fputs( " deltaAllocationSize:  'chunk' size for allocating deltas.  0=auto \n", fp );
   fputs( " actorDecisionElements: flag specifying what elements actors consider during decision-making. \n", fp );
   fputs( "             It is the sum of: \n", fp );
   fputs( "                1 = self interest \n", fp );
   fputs( "                2 = altruism \n", fp );
   fputs( "                4 = global policy preference \n", fp );
   fputs( "                8 = utility \n", fp );
   fputs( "               16 = network assocations \n", fp );
   fputs( " actorDecisionMethod:   1=probablistic, 2=always select policy with max score \n", fp );
   fputs( " policyPreferenceWt:    weight (0-1) reflecting importance of global policy preferences during actor decision-making. \n", fp );
   fputs( " shuffleActorPolys:     1=randomize actor poly traversal order, 0=fixed order. \n", fp );
   fputs( " parallel:              1=run models/processes in parallel, 0=serial.  Default=0 \n", fp );
   fputs( " collectPolicyData:     0=don't collect policy application data during runs, 1=do  Default=1 \n", fp );
   fputs( " exportMapInterval:     default map export interval during a run (years), -1 = don't export \n", fp );
   fputs( " exportBmpPixelSize:    cell size (map coords) for exporting BMPs of field(s) specified below, -1=don't export BMPs \n", fp );
   fputs( " exportBmpCols:         comma-separated list of field name(s) to export in bmp exports, empty=use active field \n", fp );
   fputs( " exportOutputs:         0=don't export model outputs, 1 = export model outputs at end of run \n", fp );
   fputs( " exportDelta:           0=don't export delta array, 1 = export delta array at end of run \n", fp );
   fputs( " exportDeltaCols:       comma-separated list of field name(s) to export in bmp exports, empty=export all fields \n", fp );
   fputs( " dynamicActors:         0=don't allow dynamic actor, 1=allow dynmaic actors \n", fp );
   fputs( " policyPreferenceWt:    0-1 value, default = 0.33 \n", fp );
   fputs( " shuffleActorPolys:     1=randomize IDU oreder during actor decision-making, 0=don't randomize \n", fp );
   fputs( " parallel:              1=run plugins in parallel, 0=run in series \n", fp );
   fputs( " addReturnsToBudget:    1=add negative costs to budget, 0=ignore negative cost in budgeting \n", fp );
   fputs( " collectPolicyData:     1=collect detailed usage data on policy application (slower), 0=don't (faster) \n", fp );
   fputs( " exportMaps:            1=export maps at the interval specified in 'exportMapInterval' attribute,0=don't (default) \n", fp );
   fputs( " exportMapInterval:     Positive value=frequency with which maps get exported during a run, <=0 disables export \n", fp );
   fputs( " exportBmpPixelSize:    integer value for the size of a pixel in an exported bitmap; <=0 disables bitmap export \n", fp );
   fputs( " exportBmpCols:         comma-separated list of fields in the IDU database to generate export maps from \n", fp );
   fputs( " exportOutputs:         1=export CSV files of run data during a run, 0=don't export run data \n", fp );
   fputs( " exportDeltas:          1=export CSV files of run deltas during a run, 0=don't export run deltas \n", fp );
   fputs( " exportDeltaCols:       comma-separated list of fields to include in delta exports, empty=export all \n", fp );
   fputs( " discardMultiRunDeltas: 1=during multiruns, don't retain deltas between runs, 0=retain deltas during multiruns (memory intensive!) \n", fp );
   fputs( " path:                  comma-separated list of paths to search for relative file specification seearches; blank=only default paths are searched \n", fp );
   fputs( "==========================================================================================================\n", fp );
   fputs( "-->\n\n", fp );

   fputs( "<settings\n", fp );
   fprintf( fp, "\tactorInitMethod            ='%i' \n", gpActorManager->m_actorInitMethod );
   fprintf( fp, "\tactorAssociations          ='0'  \n" );  // HARDCODED FOR NOW!!!!
   fprintf( fp, "\tloadSharedPolicies         ='%i' \n", m_loadSharedPolicies );
   fprintf( fp, "\tdebug                      ='%i' \n", m_model.m_debug );
   fprintf( fp, "\tstartYear                  ='%i' \n", gpModel->m_startYear );
   fprintf( fp, "\tstartRunNumber             ='%i' \n", gpModel->m_startRunNumber );
   fprintf( fp, "\tlogMsgLevel                ='%i' \n", gpModel->m_logMsgLevel );
   //fprintf( fp, "\tnoBuffering                ='%i' \n", noBuffer );
   fprintf( fp, "\tmultiRunDecadalMapsModulus ='%i' \n", m_model.m_pDataManager->multiRunDecadalMapsModulus );
   fprintf( fp, "\tdefaultPeriod              ='%i' \n", gpModel->m_yearsToRun );
   fprintf( fp, "\tmultiRunIterations         ='%i' \n", gpModel->m_iterationsToRun );
   fprintf( fp, "\tdynamicUpdate              ='%i' \n", gpModel->m_dynamicUpdate );
   fprintf( fp, "\tspatialIndexDistance       ='%i' \n", m_model.m_spatialIndexDistance );
   fprintf( fp, "\tareaCutoff                 ='%g' \n", gpModel->m_areaCutoff );
   fprintf( fp, "\tdeltaAllocationSize        ='%i' \n", DeltaArray::m_deltaAllocationSize );
   fprintf( fp, "\tactorDecisionElements      ='%i' \n", gpModel->m_decisionElements );
   fprintf( fp, "\tactorDecisionMethod        ='%i' \n", (int) gpModel->m_decisionType );
   fprintf( fp, "\tdynamicActors              ='%i' \n", gpModel->m_allowActorIDUChange ? 1 : 0 );
   
   //fprintf( fp, "\tlandscapeFeedbackWt        ='%g' \n", gpModel->m_altruismWt );
   //fprintf( fp, "\tactorValueWt               ='%g' \n", gpModel->m_selfInterestWt );
   fprintf( fp, "\tpolicyPreferenceWt         ='%g' \n", gpModel->m_policyPrefWt );
   fprintf( fp, "\tshuffleActorPolys          ='%i' \n", gpModel->m_shuffleActorIDUs ? 1 : 0 );
   fprintf( fp, "\tparallel                   ='%i' \n", gpModel->m_runParallel ? 1 : 0 );
   fprintf( fp, "\taddReturnsToBudget         ='%i' \n", gpModel->m_addReturnsToBudget ? 1 : 0 );
   fprintf( fp, "\tcollectPolicyData          ='%i' \n", gpModel->m_collectPolicyData );
   //fprintf( fp, "\texportMaps                 ='%i' \n", gpModel->m_exportMaps ? 1 : 0 );
   fprintf( fp, "\texportMapInterval          ='%i' \n", gpModel->m_exportMapInterval );
   fprintf( fp, "\texportBmpPixelSize         ='%g' \n", gpModel->m_exportBmpSize ); 
   fprintf( fp, "\texportBmpCols              ='%s' \n", (LPCTSTR) gpModel->m_exportBmpFields );
   fprintf( fp, "\texportOutputs              ='%i' \n", gpModel->m_exportOutputs ? 1 : 0 );
   fprintf( fp, "\texportDeltas               ='%i' \n", gpModel->m_exportDeltas ? 1 : 0 );
   fprintf( fp, "\texportDeltaCols            ='%s' \n", (LPCTSTR) gpModel->m_exportDeltaFields);
   fprintf( fp, "\tdiscardMultiRunDeltas      ='%i' \n", gpModel->m_discardMultiRunDeltas ? 1 : 0 );
   fprintf( fp, "\tpath                       ='%s' \n", (LPCTSTR) gpModel->m_searchPaths );
   
   //LPCTSTR field = NULL;
   //if ( gpModel->m_exportBmpCol > 0 )
   //   field = m_model.m_pIDULayer->GetFieldLabel( gpModel->m_exportBmpCol );
   //fprintf( fp, "\texport_bmp_col             ='%s' \n", field == NULL ? "" : field );

   CString mapUnits;
   switch( gpMapPanel->m_pMap->m_mapUnits )
      {
      case MU_METERS:   mapUnits = "meters";    break;
      case MU_FEET:     mapUnits = "feet";      break;
      case MU_DEGREES:  mapUnits = "degrees";   break;
      default:          mapUnits = "unknown";   break;
      }

   fprintf( fp, "\tmapUnits                   ='%s' \n", LPCTSTR( mapUnits ) );
   fputs( "/>\n\n", fp );

   fputs( "<!--\n", fp );
   fputs( "=====================================================================================================\n", fp );
   fputs( "                                   L A Y E R S -\n", fp );
   fputs( "=====================================================================================================\n", fp );
   fputs( "type:         0=shape, 1=grid \n", fp );
   fputs( "includeData:  0=no (load geometry only), 1-load data as well as geometry \n", fp );
   fputs( "color:        Red, Green Blue triplet \n", fp );
   fputs( "records:      -1=all, any other number loads the first N records only \n", fp );
   fputs( "initField:    field name for field to display initial, leave blank for default \n", fp );
   fputs( "overlayFields: comma-separated list of fields names used to generate overlays \n", fp );
   fputs( "fieldInfoFile: path to xml file contain field info descriptors - blank if <shapefilename.xml> \n", fp );
   fputs( "labelField:   IDU column name that contains labels ('' for no labels) \n", fp );
   fputs( "labelFont:    face name of font (e.g. 'Arial' used to draw label \n", fp );
   fputs( "labelSize:    size (in map units) used to draw labels \n", fp );
   fputs( "labelColor:   color (as 'r,g,b') used for labels, e.g. red is '255,0,0' \n", fp );
   fputs( "labelQuery:   spatial query indicating which IDU's shold be labeled ('' for all) \n", fp );
   fputs( "=====================================================================================================\n", fp );
   fputs( "--> \n", fp );

   if (gpMapPanel->m_pMapWnd->m_bkgrImageFilename.GetLength() > 0)
      {
      fprintf(fp, "<layers bkgr='%s' left='%f' top='%f' right='%f' bottom='%f' >\n",
         (LPCTSTR)gpMapPanel->m_pMapWnd->m_bkgrImageFilename,
         (float)gpMapPanel->m_pMapWnd->m_bkgrImageUL.x,
         (float)gpMapPanel->m_pMapWnd->m_bkgrImageUL.y,
         (float)gpMapPanel->m_pMapWnd->m_bkgrImageLR.x,
         (float)gpMapPanel->m_pMapWnd->m_bkgrImageLR.y);
      }
   else
      fputs( "<layers>\n", fp );

   Map *pMap = m_model.m_pIDULayer->m_pMap;
   
   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );

      int type = -1;
      switch ( pLayer->m_layerType )
         {
         case LT_LINE:
         case LT_POINT:
         case LT_POLYGON:
            type = 0;   // shape
            break;

         case LT_GRID:
            if ( pLayer->m_pData->GetDOType() == TYPE_INT )
               type = 1;
            else if (pLayer->m_pData->GetDOType() == TYPE_FLOAT)
               type = 2;
            break;
         }

      if ( ! pLayer->IsOverlay() && ! pLayer->m_path.IsEmpty() )
         {
         int data = pLayer->m_includeData ? 1 : 0;
         COLORREF color = pLayer->GetOutlineColor();
         int red = GetRValue( color );
         int grn = GetGValue( color );
         int blu = GetBValue( color );
         int recordsLoaded = pLayer->m_recordsLoaded;
         if ( recordsLoaded == 0 )
            recordsLoaded = -1;

         CString name;
         GetXmlStr( pLayer->m_name, name );

 //        CString overlayField(_T(""));
 //        if ( pLayer->m_overlayCol >= 0 )
 //           overlayField = pLayer->GetFieldLabel( pLayer->m_overlayCol );

         fputs( "\t<layer \n", fp );
         fprintf( fp, "\t\tname         ='%s' \n", (LPCTSTR) name );
         fprintf( fp, "\t\tpath         ='%s' \n", (LPCTSTR) pLayer->m_path );
         fprintf( fp, "\t\ttype         ='%i' \n", type );         
         fprintf( fp, "\t\tincludeData  ='%i' \n", data );
         fprintf( fp, "\t\tcolor        ='%i,%i,%i' \n", red, grn, blu );
         fprintf( fp, "\t\trecords      ='%i' \n", recordsLoaded );
         fprintf( fp, "\t\tinitField    ='%s' \n",  (LPCTSTR) pLayer->m_initInfo );
         fprintf( fp, "\t\toverlayFields ='%s' \n", (LPCTSTR) pLayer->m_overlayFields );

         if ( ! pLayer->m_pFieldInfoArray->m_path.IsEmpty() )
            fprintf( fp, "\t\tfieldInfoFile='%s' \n", (LPCTSTR) pLayer->m_pFieldInfoArray->m_path );

         if ( pLayer->m_showLabels )
            {
            color = pLayer->m_labelColor;
            red = GetRValue( color );
            grn = GetGValue( color );
            blu = GetBValue( color );

            fprintf( fp, "\t\tlabelField   ='%s' \n", (LPCTSTR) pLayer->GetFieldLabel( pLayer->m_labelCol ) );
            fprintf( fp, "\t\tlabelFont    ='%s' \n", (LPCTSTR) pLayer->m_labelFont.lfFaceName );
            fprintf( fp, "\t\tlabelSize    ='%f' \n", pLayer->m_labelSize );
            fprintf( fp, "\t\tlabelColor   ='%i,%i,%i' \n",  red, grn, blu );
            fprintf( fp, "\t\tlabelQuery   ='%s' \n", (LPCTSTR) pLayer->m_labelQueryStr );
            }

         fputs( "\t/>\n", fp );
         }
      }

   fputs( "</layers> \n\n", fp );

   
   fputs( "<!--\n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( "                                  V I S U A L I Z E R S \n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( "\n", fp );
   fputs( "name:          string indicatind name of visualizer\n", fp );
   fputs( "path:          path to the visualizer DLL\n", fp );
   fputs( "id:            unique integer identifier for this visualizer \n", fp );
   fputs( "use:           0=don't use this process, 1=use this visualizer \n", fp );
   fputs( "type:          1=Input, 2=RunTime, 4 = postrun - may be or'd together\n", fp );
   fputs( "initInfo:      visualizer -specific string passed to model at initialization\n", fp );
   fputs( "\n", fp );
   fputs( " ALL FIELDS EXCEPT [initInfo] and [name] MUST BE NON-WHITESPACE.\n", fp );
   fputs( "\n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( "-->\n", fp );
   fputs( "<visualizers> \n", fp );

   for ( int i=0; i < gpModel->GetVisualizerCount(); i++ )
      {
      EnvVisualizer *pInfo = gpModel->GetVisualizerInfo( i );

      if ( ! pInfo->m_inRegistry )
         {
         int use = pInfo->m_use ? 1 : 0;

         CString name;
         GetXmlStr( pInfo->m_name, name );

         fputs( "\t<visualizer \n", fp );
         fprintf( fp, "\t\tname         ='%s' \n", (LPCTSTR)name );
         fprintf( fp, "\t\tpath         ='%s' \n", (LPCTSTR)pInfo->m_path );
         fprintf( fp, "\t\tid           ='%i' \n", pInfo->m_id );
         fprintf( fp, "\t\tuse          ='%i' \n", use );
         fprintf( fp, "\t\ttype         ='%i' \n", pInfo->m_vizType );
         fprintf( fp, "\t\tinitInfo     ='%s' \n", (LPCTSTR)pInfo->m_initInfo );
         fputs( "\t/>\n", fp );
         }
      }

   fputs( "\n</visualizers>\n\n", fp );



   fputs( "<!--\n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( "                                  Z O O M S \n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( "\n", fp );
   fputs( "name:          string indicating name of zoom\n", fp );
   fputs( "left, top, right, bottom:  real-world coords for view - xMin, yMax, xMax, yMin\n", fp );
   fputs( "\n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( "-->\n", fp );

   // get default zoom
   if ( gpView->m_pDefaultZoom != NULL )
      fprintf( fp, "<zooms default='%s'> \n", (LPCTSTR) gpView->m_pDefaultZoom->m_name );
   else
      fputs( "<zooms>", fp );

   for ( int i=0; i < gpView->GetZoomCount(); i++ )
      {
      ZoomInfo *pInfo = gpView->GetZoomInfo( i );

      CString name;
      GetXmlStr( pInfo->m_name, name );

      fprintf( fp, "\t<zoom name='%s' left='%f' top='%f' right='%f' bottom='%f' />\n",
         (LPCTSTR) name, pInfo->m_xMin, pInfo->m_yMax, pInfo->m_xMax, pInfo->m_yMin );
      }

   fputs( "</zooms> \n", fp );


   fputs( "<!--\n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( "                                  E V A L U A T O R S \n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( " Note that a single DLL can contain multiple evaluators, each must be specified on \n", fp );
   fputs( "     it's own line.  NOTE:  Once specified, the modelID should not be changed, since the model may use\n", fp );
   fputs( "     this ID internally.\n", fp );
   fputs( "\n", fp );
   fputs( "id:            unique integer identifier for this model \n", fp );
   fputs( "freq:          number indicating how often to run this model, 0=always run\n", fp );
   fputs( "use:           0=don't use this process, 1=use this model \n", fp );
   fputs( "\n", fp );
   fputs( "showInResults: 0=don't show anywhere\n", fp );
   fputs( "               1=show everywhere\n", fp );
   fputs( "               2=show where unscaled (beyond [-3..3]) can be shown.\n", fp );
   //fputs( "fieldName:  name of field in the IDU coverage reserved by this model, blank if not field reserved\n", fp );
   fputs( "initInfo:   model-specific string passed to model at initialization\n", fp );
   fputs( "dependencies: comma-separated list of names of models this model is dependent on\n", fp );
   //fputs( "initRunOnStartup: causes InitRun() to be invoked after initial loading if set to 1, 0=not invoked\n", fp );
   fputs( "\n", fp );
   fputs( " ALL FIELDS EXCEPT [initInfo] and [name] MUST BE NON-WHITESPACE.\n", fp );
   fputs( "\n", fp );
   fputs( "==================================================================================================\n", fp );
   fputs( "-->\n", fp );
   fputs( "<models> \n", fp );

   for ( int i=0; i < gpModel->GetEvaluatorCount(); i++ )
      {
      EnvEvaluator *pInfo = gpModel->GetEvaluatorInfo( i );

      //int useCol = 1;
      //if ( pInfo->fieldName.IsEmpty() )
      //   useCol = 0;

      int use = pInfo->m_use ? 1 : 0;

      CString name;
      GetXmlStr( pInfo->m_name, name );

      fputs( "\t<model \n", fp );
      fprintf( fp, "\t\tname         ='%s' \n", (LPCTSTR) name );
      fprintf( fp, "\t\tpath         ='%s' \n", (LPCTSTR) pInfo->m_path );
      fprintf( fp, "\t\tid           ='%i' \n", pInfo->m_id );
      fprintf( fp, "\t\tuse          ='%i' \n", use );
      fprintf( fp, "\t\tfreq         ='%i' \n", pInfo->m_frequency );
      //fprintf( fp, "\t\tdecisionUse  ='%i' \n", pInfo->decisionUse );
      fprintf( fp, "\t\tshowInResults='%i' \n", pInfo->m_showInResults );
      //fprintf( fp, "\t\tfieldName    ='%s' \n", (LPCTSTR) pInfo->fieldName );
      fprintf( fp, "\t\tinitInfo     ='%s' \n", (LPCTSTR) pInfo->m_initInfo );
      fprintf( fp, "\t\tdependencies ='%s' \n", (LPCTSTR) pInfo->m_dependencyNames );
      //fprintf( fp, "\t\tinitRunOnStartup  ='%i' \n", pInfo->initRunOnStartup );
      //fprintf( fp, "\t\tgain         ='%f' \n", pInfo->gain );
      //fprintf( fp, "\t\toffset       ='%f' \n", pInfo->offset );
      fputs( "\t/>\n", fp );
      }

   fputs( "\n</models>\n\n", fp );
   
   fputs( "<!--\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "                      M O D E L S \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "id:         unique integer identifier for this process \n", fp );
   fputs( "use:        0=don't use this process, 1=use this process \n", fp );
   fputs( "freq:       number indicating how often to run this process, 0=always run\n", fp );
   fputs( "timing:     0=run before eval models, 1=run after eval models \n", fp );
   //fputs( "sandbox:    1=use this process when running sandbox evaluations, 0=ignore during sandboxing \n", fp );
   //fputs( "fieldName:  name of field in the IDU coverage reserved by this process, blank if not field reserved\n", fp );
   fputs( "initInfo:   model-specific string passed to process at initialization\n", fp );
   fputs( "dependencies: comma-separated list of names of models this model is dependent on\n", fp );
   //fputs( "initRunOnStartup: causes InitRun() to be invoked after initial loading if set to 1, 0=not invoked\n", fp );
   fputs( "===================================================================================\n", fp );
  
   fputs( "-->\n", fp );

   fputs( "<models>\n", fp );

   for ( int i=0; i < gpModel->GetModelProcessCount(); i++ )
      {
      EnvModelProcess *pInfo = gpModel->GetModelProcessInfo( i );

      //int useCol = 1;
      //if ( pInfo->fieldName.GetLength() == 0 )
      //   useCol = 0;

      int use = pInfo->m_use ? 1 : 0;

      CString name;
      GetXmlStr( pInfo->m_name, name );

      fputs( "\t<model\n", fp );
      fprintf( fp, "\t\tname         ='%s' \n", (LPCTSTR) name );
      fprintf( fp, "\t\tpath         ='%s' \n", (LPCTSTR) pInfo->m_path );
      fprintf( fp, "\t\tid           ='%i' \n", pInfo->m_id );
      fprintf( fp, "\t\tuse          ='%i' \n", use );
      fprintf( fp, "\t\ttiming       ='%i' \n", pInfo->m_timing );
      fprintf( fp, "\t\tfreq         ='%i' \n", pInfo->m_frequency );
      //fprintf( fp, "\t\tsandbox      ='%i' \n", pInfo->sandbox );
      //fprintf( fp, "\t\tfieldName    ='%s' \n", (LPCTSTR) pInfo->fieldName );
      fprintf( fp, "\t\tinitInfo     ='%s' \n", (LPCTSTR) pInfo->m_initInfo );
      fprintf( fp, "\t\tdependencies ='%s' \n", (LPCTSTR) pInfo->m_dependencyNames );
      //fprintf( fp, "\t\tinitRunOnStartup  ='%i' \n", pInfo->initRunOnStartup );
      fputs( "\t/>\n\n", fp );
      }

   fputs( "</models>\n\n", fp );
/*
   fputs( "<!-- \n", fp );
   fputs( "=============================================================================================\n", fp );
   fputs( "                                 C D F S \n", fp );
   fputs( "=============================================================================================\n", fp );
   fputs( " <name><,>[path]<,><format><,><data>[\n", fp );
   fputs( "       if path empty, read here\n", fp );
   fputs( "       format of {TEXT, N_BIN, xmlschema=...}\n", fp );
   fputs( "       data depends of format; if TEXT, spc separated pairs of score and Pr(SCORE <= score) \n", fp );
   fputs( "=============================================================================================\n", fp );
   fputs( "--> \n", fp );

   for ( int i=0; i < gpModel->GetEvaluatorCount(); i++ )
      {
      EnvEvaluator *pInfo = gpModel->GetEvaluatorInfo( i );

      if ( pInfo->apNpCdf.get()!= NULL )
         {
         string cdf;
         HistogramCDF::writeQuantiles( cdf, *pInfo->apNpCdf, (LPCTSTR) pInfo->m_name );

         //file << cdf << endl;
         }
      }
      */

   fputs( "\n<!--\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "                  A P P L I C A T I O N   V A R I A B L E S  \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "<app_vars> \n", fp );
   fputs( "<app_var name='myname' description='' col='myField' timing='3' /> \n", fp );
   fputs( "</app_vars> \n", fp );
   fputs( "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  \n", fp );
   fputs( "name:          name of app variable (no spaces) (required)\n", fp );
   fputs( "description:   description of variable (required)\n", fp );
   fputs( "value:         expression to evaluate. exclude this if a plugin will manage this AppVar (optional)\n", fp );
   fputs( "col:           field name to populate (optional) - implies this is a local variable.  Leave empty for global variables \n", fp );
   fputs( "timing:        0=no autoupdate.  The value is constant, or a plugin controls this value.\n", fp );
   fputs( "               1=evaluate at the beginning of a time step only\n", fp );
   fputs( "               2=evaluate at the end of a time step only\n", fp );
   fputs( "               3=evaluate both at the beginning and end of a time step.\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "-->\n", fp );

   fputs( "<app_vars>\n", fp );

   for ( int i=0; i < m_model.GetAppVarCount(); i++ )
      {
      AppVar *pVar = m_model.GetAppVar( i );
      
      if ( pVar->m_avType == AVT_APP )
         {
         CString name;
         GetXmlStr( pVar->m_name, name );

         CString descr;
         GetXmlStr( pVar->m_description, descr );

         CString expr;
         GetXmlStr( pVar->m_expr, expr );

         int col = pVar->GetCol();
         CString field = "";
         if ( col >= 0 )
            field = m_model.m_pIDULayer->GetFieldLabel( col );
               
         fputs( "\t<app_var \n", fp );
         fprintf( fp, "\t\tname ='%s' \n", (LPCTSTR) name );
         fprintf( fp, "\t\tdescription ='%s' \n", (LPCTSTR) descr );
         fprintf( fp, "\t\tvalue ='%s' \n", (LPCTSTR) expr );
         if ( col >= 0 )
            fprintf( fp, "\t\tcol ='%s' \n", (LPCTSTR) field );
         fprintf( fp, "\t\ttiming ='%i' \n", pVar->m_timing );
         fputs( "\t/> \n", fp );
         }
      }
   fputs( "</app_vars>\n\n", fp );
   
   fputs( "\n<!--\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "                      L U L C   T R E E \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "-->\n\n", fp );   
   m_model.m_lulcTree.SaveXml( fp, false );

   fputs( "\n<!--\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "                      M E T A G O A L S \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "name:    name of this metagoal \n", fp );
   fputs( "model:   name of associated eval model, if used in altruistic decision-making. \n", fp );
   fputs( "         this must correspond to entry in the <models> section.\n", fp );
   fputs( "decisionUse:   0=don't use in decision,\n", fp );
   fputs( "               1=use in actor self-interest (value) decision only\n", fp );
   fputs( "               2=use in altruistic decision only\n", fp );
   fputs( "               3=use in both\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "-->\n", fp );

   fputs( "<metagoals>\n", fp );

   for ( int i=0; i < gpModel->GetMetagoalCount(); i++ )
      {
      METAGOAL *pInfo = gpModel->GetMetagoalInfo( i );

      CString name;
      GetXmlStr( pInfo->name, name );

      fputs( "\t<metagoal ", fp );
      fprintf( fp, "name ='%s' ", (LPCTSTR) name );

      if ( pInfo->pEvaluator != NULL )
         fprintf( fp, "model ='%s' ", (LPCTSTR) pInfo->pEvaluator->m_name );

      fprintf( fp, "decision_use ='%i' />\n", pInfo->decisionUse );
      }

   fputs( "</metagoals>\n\n", fp );

   fputs( "\n<!--\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "                      P O L I C I E S \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "-->\n\n", fp ); 
   bool useFileRef = gpPolicyManager->m_loadStatus == 1 ? true : false;  // 1=loaded from xml,0=embedded in envx
   gpPolicyManager->SaveXml( fp, /*includeHeader=*/ false, useFileRef );

   //---------------- actors ---------------------------
   useFileRef = gpActorManager->m_loadStatus == 1 ? true : false;
   gpActorManager->SaveXml( fp, false, useFileRef );
 
   fputs( "\n<!--\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "                      S C E N A R I O S \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "-->\n\n", fp );   
   useFileRef = gpScenarioManager->m_loadStatus == 1 ? true : false;
   gpScenarioManager->SaveXml( fp, false, useFileRef );
/*
   fputs( "\n<!--\n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "                      P R O B E S \n", fp );
   fputs( "===================================================================================\n", fp );
   fputs( "-->\n\n", fp );   
   
   fputs( "<probes>\n", fp );

   for ( int i=0; i < gpModel->GetProbeCount(); i++ )
      {
      Probe *pProbe = gpModel->GetProbe( i );

      switch( pProbe->GetType() )
         {
         case PT_POLICY:
            fputs( "\t<probe type='policy' id=", fp );
            fprintf( fp, "'%i' />\n", pProbe->m_pPolicy->m_id );
            break;

         case PT_IDU:
            break;
         }
      }

   fputs( "</probes>\n\n", fp );


   */

   fputs( "\n</Envision>\n", fp );
   fclose( fp );

   this->UnSetChanged( CHANGED_PROJECT );
   SetModifiedFlag( FALSE );   

   return 1; 
   }



// save the cell database
void CEnvDoc::OnFileSavedatabase() 
   {
   // make sure the map is present
   ASSERT ( m_model.m_pIDULayer != NULL );
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( FALSE, "dbf",m_model.m_pIDULayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "DBase Files|*.dbf|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      bool useWide = VData::SetUseWideChar( true );

      CWaitCursor c;
      m_model.m_pIDULayer->SaveDataDB( filename ); // uses DBase/CodeBase

      VData::SetUseWideChar( useWide );
      UnSetChanged( CHANGED_COVERAGE );
      }

   _chdir( cwd );
   free( cwd );
   }


void CEnvDoc::OnUpdateIsDocOpen(CCmdUI* pCmdUI) 
   {
   BOOL enable = TRUE;

   if ( m_model.m_pIDULayer == NULL )  enable = FALSE;
   //else if ( ! IsChanged( CHANGED_COVERAGE ) )
   //   enable = FALSE;

   pCmdUI->Enable( enable );
   }

void CEnvDoc::OnUpdateIsDocChanged(CCmdUI* pCmdUI) 
   {
   BOOL enable = TRUE;

   if ( m_model.m_pIDULayer == NULL )
      enable = FALSE;
   else if ( ! IsChanged( CHANGED_COVERAGE ) )
      enable = FALSE;

   pCmdUI->Enable( enable );
   }



void CEnvDoc::OnOptions()
   {
   OptionsDlg dlg;
   dlg.DoModal();
   }


void CEnvDoc::OnCloseDocument() 
   {
   if ( IsChanged( CHANGED_PROJECT ) 
      && gpMain->MessageBox( "The Project has changed.  Would you like to save it?", "Save Project", MB_YESNO ) == IDYES )
         SaveAs( NULL );
   
   if ( IsChanged( CHANGED_POLICIES ) 
      && gpMain->MessageBox( "The Policies for this project have changed.  Would you like to save them", "Save Policies", MB_YESNO ) == IDYES )
         OnExportPolicies();
   
   if ( IsChanged( CHANGED_ACTORS )
      && gpMain->MessageBox( "The Actors for this project have changed.  Would you like to save them", "Save Actors", MB_YESNO ) == IDYES )
         OnExportActors();
   
   //if ( IsChanged( CHANGED_COVERAGE ) 
   //   && gpMain->MessageBox( "The database for this project has changed.  Would you like to save it", "Save IDU Database", MB_YESNO ) == IDYES )
   //      OnFileSavedatabase();
   
   if ( IsChanged( CHANGED_SCENARIOS )
      && gpMain->MessageBox( "The Scenarios for this project have changed.  Would you like to save them", "Save Scenarios", MB_YESNO ) == IDYES )
         OnExportScenarios();   

   CDocument::OnCloseDocument();
   }


// main entry point for starting model run
void CEnvDoc::OnAnalysisRun() 
   {
   if ( m_model.m_runStatus == RS_RUNNING )
      return;

   // get run info from command bar
   m_model.m_startYear  = gpMain->GetStartYear();
   m_model.m_yearsToRun = gpMain->GetRunLength();  
   m_model.m_endYear    = m_model.m_startYear + m_model.m_yearsToRun;

   //m_model.m_exportMaps = gpMain->GetExportMaps();
   m_model.m_exportMapInterval = gpMain->GetExportMapInterval();
   //m_model.m_exportOutputs = gpMain->GetExportOutputs();
   gpMain->GetExportDeltaFieldList( m_model.m_exportDeltaFields );

   // get the current scenario
   int scenario = gpMain->GetCurrentlySelectedScenario();

   if ( scenario < 0 )  // run all?
      {
      for ( int i=0; i < gpScenarioManager->GetCount(); i++ )
         {
         Scenario *pScenario = gpScenarioManager->GetScenario( i ) ; 
         m_model.SetScenario( pScenario );
         RunModel();
         }

      m_model.SetScenario( NULL );
      }
   else   // a specific scenarion specified
      {
      m_model.SetScenario( gpScenarioManager->GetScenario( scenario ) );
      RunModel();
      }
   }


void CEnvDoc::RunModel()
   {
   if ( m_model.m_runStatus == RS_RUNNING )
      {
      //m_model.m_runStatus = RS_PAUSED;
      return;
      }

   if ( m_model.m_currentYear != m_model.m_startYear )
      OnAnalysisReset();

   if ( m_model.m_areModelsInitialized == false )
      gpModel->InitModels();

   //int cellCount = gpActorManager->GetManagedCellCount();
   //if ( cellCount == 0 )
   //   cellCount = 1;

   gpMain->SetRunProgressRange( 0, m_model.m_yearsToRun );
   gpMain->SetRunProgressPos( 0 );
   
   // any constraints?
   if ( GetConstraintCount() > 0 )
      {
      


      }

   //try
   //   {
   m_model.Run( 0 );      // don't randomize
   //   }
   //catch ( EnvFatalException & ex )
   //   {
   //   CString msg;
   //   msg.Format( "Fatal Error:\n  %s", ex.what() );
   //   Report::ErrorMsg( msg );
   //   }

   //gpMapPanel->RefreshList();
   gpMapPanel->RedrawWindow();
   gpMain->SetModelMsg( "Done" );
   SetChanged( CHANGED_COVERAGE );
   }



void CEnvDoc::RunMultiple()
   {
   if ( m_model.m_runStatus == RS_RUNNING )
      {
      //m_model.m_runStatus = RS_PAUSED;
      return;
      }

   if ( gpModel->m_currentYear != 0 )
      OnAnalysisReset();

   if ( m_model.m_areModelsInitialized == false )
      gpModel->InitModels();

   //int cellCount = gpActorManager->GetManagedCellCount();
   //if ( cellCount == 0 )
   //   cellCount = 1;

   gpMain->SetRunProgressRange( 0, m_model.m_yearsToRun );
   gpMain->SetRunProgressPos( 0 );

   gpMain->SetMultiRunProgressRange( 0, m_model.m_iterationsToRun );
   gpMain->SetMultiRunProgressPos( 0 );

   // any constraints?
   if ( GetConstraintCount() > 0 )
      {
      }

   clock_t start = clock();

   m_model.RunMultiple(); 
   gpResultsPanel->AddMultiRun( m_model.m_currentMultiRun-1 );  // multirun count already incremented at this point
  
   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;   

   //CTime startTime = CTime::GetCurrentTime();
   //CTime endTime = CTime::GetCurrentTime();
   //CTimeSpan elapsedTime = endTime - startTime;
   
   CString msg;
   msg.Format( "Multirun (%s) done: CPU=%i secs", (LPCTSTR) m_model.GetScenario()->m_name, (int) duration );
   gpMain->SetModelMsg( msg );

   gpMapPanel->RedrawWindow();
   gpMain->SetModelMsg( "Done (Multirun)" );
   SetChanged( CHANGED_COVERAGE );
   }


void CEnvDoc::OnAnalysisRunmultiple()
   {
   if ( m_model.m_runStatus == RS_RUNNING )
      return;

   m_model.m_startYear  = gpMain->GetStartYear();
   m_model.m_currentYear = m_model.m_startYear;
   m_model.m_yearsToRun = gpMain->GetRunLength();  
   m_model.m_endYear    = m_model.m_startYear + m_model.m_yearsToRun;

   //m_model.m_exportMaps = gpMain->GetExportMaps();
   m_model.m_exportMapInterval = gpMain->GetExportMapInterval();
   //m_model.m_exportOutputs = gpMain->GetExportOutputs();
   gpMain->GetExportDeltaFieldList( m_model.m_exportDeltaFields );

   // get the current scenario
   int scenario = gpMain->GetCurrentlySelectedScenario();

   if ( scenario < 0 )  // run all?
      {
      for ( int i=0; i < gpScenarioManager->GetCount(); i++ )
         {
         Scenario *pScenario = gpScenarioManager->GetScenario( i ) ; 
         m_model.SetScenario( pScenario );
         RunMultiple();
         }

      m_model.SetScenario( NULL );
      }
   else   // a specific scenario specified
      {
      m_model.SetScenario( gpScenarioManager->GetScenario( scenario ) );
      RunMultiple();
      }
   }


void CEnvDoc::OnUpdateAnalysisRun(CCmdUI* pCmdUI) 
   {
   BOOL prerun  = m_model.m_runStatus == RS_PRERUN  ? 1 : 0;
   BOOL stopped = m_model.m_runStatus == RS_PAUSED  ? 1 : 0;
   BOOL running = m_model.m_runStatus == RS_RUNNING ? 1 : 0;

   if ( m_model.m_pIDULayer == NULL || running )
      {
      pCmdUI->Enable( FALSE );
      return;
      }
   else
      pCmdUI->Enable( TRUE );

   switch( m_model.m_runStatus )
      {
      case RS_RUNNING:
         pCmdUI->SetText( "Pause" );
         break;
         
      case RS_PRERUN:
         pCmdUI->SetText( "Run" );
         break;

      case RS_PAUSED:
//         pCmdUI->SetText( "&Continue" );
         break;
      }
   }


void CEnvDoc::OnUpdateAnalysisRunMultiple(CCmdUI* pCmdUI) 
   {
   if ( m_model.m_pIDULayer == NULL  || m_model.m_runStatus == RS_RUNNING )
      pCmdUI->Enable( FALSE );
   else
      pCmdUI->Enable( TRUE );
   }


void CEnvDoc::OnAnalysisStop()
   {
   //m_model.m_runStatus = RS_STOPPED;

   //if ( m_model.m_continueToRunMultiple )
   //   m_model.m_continueToRunMultiple = false;
   }

void CEnvDoc::OnUpdateAnalysisStop(CCmdUI *pCmdUI)
   {
   //if ( m_model.m_pIDULayer != NULL && ( m_model.m_runStatus == RS_RUNNING || m_model.m_runStatus == RS_PAUSED ) )
   //   pCmdUI->Enable( TRUE );
   //else
      pCmdUI->Enable( FALSE );
   }

void CEnvDoc::OnAnalysisReset()
   {
   m_model.Reset();
   gpMain->SetRunProgressPos(0);
   }

void CEnvDoc::OnUpdateAnalysisReset(CCmdUI *pCmdUI)
   {
   //if ( m_model.m_runStatus == RS_PAUSED )
   //   pCmdUI->Enable( TRUE );
   //else
      pCmdUI->Enable( FALSE );
   }


//void FileMessage( LPCTSTR msg )
//   {
//   fprintf( gfpLogFile, msg );
//   }


void StatusMsg( LPCTSTR msg )
   {
   CMainFrame *pMain = (CMainFrame*) AfxGetMainWnd();

   if ( pMain != NULL )
      pMain->SetStatusMsg( msg );
   }


/*
int CEnvDoc::SelectWatershed( MapLayer *pCellLayer, int reachID, int colReachID )
   {
   pCellLayer->ClearSelection();

   // find the ReachNode with this id
   ReachNode *pNode = m_reachTree.GetReachNodeFromReachID( reachID );

   // no node found...
   if ( pNode == NULL )
      return -1;

   CArray< ReachNode*, ReachNode* > upstreamReachArray;

   FindUpstreamReaches( pNode, upstreamReachArray );

   int upstreamReachCount = upstreamReachArray.GetSize();
   if ( upstreamReachCount == 0 )
      return 0;

   // found all upstream reaches, select all polygons that have each rach as it id
   for ( int i=0; i < pCellLayer->GetRecordCount(); i++ )
      {
      int polyReachID;
      bool ok = pCellLayer->GetData( i, colReachID, polyReachID );

      if ( ! ok ) // field not found, so bail with an error code
         return -2;

      // see if this watershed id is in the upstream
      for ( int j=0; j < upstreamReachCount; j++ )
         {
         if ( upstreamReachArray[ j ]->m_reachInfo.reachID == polyReachID )
            {
            pCellLayer->AddSelection( i );
            break;
            }
         }
      }

   return pCellLayer->GetSelectionCount();
   }


void CEnvDoc::FindUpstreamReaches( ReachNode *pNode, CArray< ReachNode*, ReachNode* > &upstreamReachArray )
   {
   if (  pNode == NULL )
      return;

   upstreamReachArray.Add( pNode );

   FindUpstreamReaches( pNode->m_pLeft, upstreamReachArray );
   FindUpstreamReaches( pNode->m_pRight, upstreamReachArray );
   return;
   }
*/



void CEnvDoc::OnImportPolicies()
   {
   ASSERT( gpPolicyManager != NULL );

   int retVal = AfxMessageBox( _T("Do you want to clear existing policies before importing?"), MB_YESNOCANCEL );

   if ( retVal == IDCANCEL )
      return;

   DirPlaceholder d;
   CString startPath = gpPolicyManager->m_path;
   CString ext = _T("xml");
   CString extensions = _T("XML Files|*.xml|ENVX (Project) Files|*.envx|All files|*.*||");
   
   CFileDialog dlg( TRUE, ext, startPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, extensions);

   int retVal2 = (int) dlg.DoModal();
   
   if ( retVal2 == IDOK )
      {
      if ( retVal == IDYES )
         gpPolicyManager->RemoveAllPolicies();

      CString filename = dlg.GetPathName();
   
      int count = gpPolicyManager->LoadXml( filename, false, true );
      
      if ( count >= 0 )
         {
         gpView->m_policyEditor.ReloadPolicies( true );

         CString msg;
         msg.Format( _T("Successfully loaded %i policies"), count );
         AfxMessageBox( msg, MB_OK );
         }
      else
         AfxMessageBox( _T("Error encountered loading file - no poolicies found or loaded"), MB_OK );
      }
   }


void CEnvDoc::OnImportActors()
   {
   ASSERT( gpActorManager != NULL );

   int retVal = AfxMessageBox( _T("Do you want to clear existing actors before importing?"), MB_YESNOCANCEL );

   if ( retVal == IDCANCEL )
      return;

   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", gpActorManager->m_path, OFN_HIDEREADONLY, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      if ( retVal == IDYES )
         {
         gpActorManager->RemoveAllActorGroups();
         gpActorManager->RemoveAllActors();
         gpActorManager->RemoveAllAssociations();
         }

      CString filename( dlg.GetPathName() );
      int count = gpActorManager->LoadXml( filename, true, false );
      
      if ( count >= 0 )
         {
         CString msg;
         msg.Format( _T("Successfully loaded %i actor definitions"), count );
         AfxMessageBox( msg, MB_OK );
         }
      else
         AfxMessageBox( _T("Error encountered loading file"), MB_OK );
      }
   }


void CEnvDoc::OnImportLulc()
   {
   ASSERT( gpModel != NULL );

   int retVal = AfxMessageBox( _T("Do you want to clear existing land cover classes before importing?"), MB_YESNOCANCEL );

   if ( retVal == IDCANCEL )
      return;

   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", gpModel->m_lulcTree.m_path, OFN_HIDEREADONLY, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      if ( retVal == IDYES )
         gpModel->m_lulcTree.RemoveAll();

      CString filename( dlg.GetPathName() );
      int count = gpModel->m_lulcTree.LoadXml( filename, true, false );
      
      if ( count >= 0 )
         {
         CString msg;
         msg.Format( _T("Successfully loaded %i land use/land cover classes"), count );
         AfxMessageBox( msg, MB_OK );
         }
      else
         AfxMessageBox( _T("Error encountered loading file"), MB_OK );
      }
   }


void CEnvDoc::OnImportScenarios()
   {
   ASSERT( gpScenarioManager != NULL );

   int retVal = AfxMessageBox( _T("Do you want to clear existing scenarios before importing?"), MB_YESNOCANCEL );

   if ( retVal == IDCANCEL )
      return;

   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", gpScenarioManager->m_path, OFN_HIDEREADONLY, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      if ( retVal == IDYES )
         gpScenarioManager->RemoveAll();
      
      CString filename( dlg.GetPathName() );
      int count = gpScenarioManager->LoadXml( filename, true, false );
      
      if ( count >= 0 )
         {
         CString msg;
         msg.Format( _T("Successfully loaded %i scenarios"), count );
         AfxMessageBox( msg, MB_OK );
         }
      else
         AfxMessageBox( _T("Error encountered loading file"), MB_OK );
      }
   }


void CEnvDoc::OnFileExport()
   {
   ActiveWnd::OnFileExport();
   }

void CEnvDoc::OnUpdateFileExport(CCmdUI *pCmdUI)
   {
   ActiveWnd::OnUpdateFileExport( pCmdUI );
   }


void CEnvDoc::OnImportMultirunDataset()
   {
   DirPlaceholder d;

   CFolderDialog dlg( _T( "Select Folder" ), NULL, gpView, BIF_EDITBOX | BIF_NEWDIALOGSTYLE );

   if ( dlg.DoModal() == IDOK )
      {
      CString path = dlg.GetFolderPath();

      if ( !path.IsEmpty() )
         if ( path[ path.GetLength()-1 ] != '\\' )
            path += "\\";

      // Import Multi Run returns the index of the scenario used for the imported runs
      int runScenario = gpModel->m_pDataManager->ImportMultiRunData( path );      // TODO:  fix -1
      }
   }
 

void CEnvDoc::OnUpdateImportMultirunDataset(CCmdUI *pCmdUI)
   {
   if ( gpModel->m_currentMultiRun == 0 )
      pCmdUI->Enable( TRUE );
   else
      pCmdUI->Enable( FALSE );
   }


void CEnvDoc::OnExportModelOutputs()
   {
   DirPlaceholder d;
   gpResultsPanel->ExportModelOutputs( -1, true );
   }
 

void CEnvDoc::OnUpdateExportModelOutputs(CCmdUI *pCmdUI)
   {
   if ( gpModel->m_currentRun > 0 || gpModel->m_currentYear > gpModel->m_startYear )
      pCmdUI->Enable( TRUE );
   else
      pCmdUI->Enable( FALSE );
   }


void CEnvDoc::OnExportMultirunDataset()
   {
   DirPlaceholder d;

   CFolderDialog dlg( _T( "Select Folder" ), NULL, gpView, BIF_EDITBOX | BIF_NEWDIALOGSTYLE );

   if ( dlg.DoModal() == IDOK )
      {
      CString path = dlg.GetFolderPath();

      if ( !path.IsEmpty() )
         if ( path[ path.GetLength()-1 ] != '\\' )
            path += "\\";

      gpModel->m_pDataManager->ExportMultiRunData( path, -1 );      // TODO:  fix -1
      }
   }


void CEnvDoc::OnUpdateExportDeltaArray(CCmdUI *pCmdUI)
   {
   if ( gpModel->m_currentRun > 0 || gpModel->m_currentYear > gpModel->m_startYear )
      pCmdUI->Enable( TRUE );
   else
      pCmdUI->Enable( FALSE );
   }


void CEnvDoc::OnUpdateExportMultirunDataset(CCmdUI *pCmdUI)
   {
   if ( gpModel->m_currentMultiRun > 0 )
      pCmdUI->Enable( TRUE );
   else
      pCmdUI->Enable( FALSE );
   }


void CEnvDoc::OnExportDecadalcelllayers()
   {
   // TODO: Add your command handler code here
   char path[_MAX_PATH];

   if (NULL == _getcwd(path, _MAX_PATH))
      Report::SystemErrorMsg( GetLastError() );

   gpModel->m_pDataManager->ExportRunData( path ); 
   }


void CEnvDoc::OnUpdateExportDecadalcelllayers(CCmdUI *pCmdUI)
   {
   // TODO: Add your command update UI handler code here
   pCmdUI->Enable(gpDoc->m_model.m_currentYear >= gpDoc->m_model.m_endYear);
   }


void CEnvDoc::OnExportActors()
   {
   ASSERT( gpActorManager != NULL );

   DirPlaceholder d;

   CFileDialog dlg( FALSE, "xml", gpActorManager->m_path, OFN_HIDEREADONLY, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      gpActorManager->SaveXml( filename );
      }   
   }

void CEnvDoc::OnExportPolicies()
   {
   ASSERT( gpPolicyManager != NULL );

   DirPlaceholder d;

   CFileDialog dlg( FALSE, "xml", gpPolicyManager->m_path, OFN_HIDEREADONLY, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      gpPolicyManager->SaveXml( filename );
      }   
   }


void CEnvDoc::OnExportLulc()
   {
   ASSERT( gpModel != NULL );

   DirPlaceholder d;

   CFileDialog dlg( FALSE, "xml", gpModel->m_lulcTree.m_path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      gpDoc->m_model.m_lulcTree.SaveXml( filename );
      }   
   }


void CEnvDoc::OnExportScenarios()
   {
   ASSERT( gpScenarioManager != NULL );

   DirPlaceholder d;

   CFileDialog dlg( FALSE, "xml", gpScenarioManager->m_path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      gpScenarioManager->SaveXml( filename );
      }   
   }


void CEnvDoc::OnExportFieldInfo()
   {
   DirPlaceholder d;
 
   CFileDialog dlg( FALSE, "xml", m_model.m_pIDULayer->m_pFieldInfoArray->m_path, OFN_OVERWRITEPROMPT, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString path = dlg.GetPathName();
      CWaitCursor c;
      gpDoc->SaveFieldInfoXML( m_model.m_pIDULayer, path );
      }
   }


void CEnvDoc::OnDataLulcFields()
   {
   // Infrastructure to handle inconsistencies between cellLayer and lulcTree
   bool ok = PopulateLulcFields();

   CString finishedMessage = "Lulc Fields have been populated based on values in the lulcTree file.";
   finishedMessage += " To save these changes, select the File->Save->Database Menu Option.";
   MessageBox( GetActiveWindow(), finishedMessage, "Finished", MB_OK );
   SetChanged( CHANGED_COVERAGE );
   }


// examine each cell, reclassify any special cases, and populate correct LULC_A, LULC_B fields based on LULC_C
bool CEnvDoc::PopulateLulcFields( void )
   {
   int levels = m_model.m_lulcTree.GetLevels();  // 1=LULC_A, 2=LULC_B, etc...

   if ( levels <= 1 )
      return true;  // nothing to do

   switch( levels )
      {
      case 2:
         return PopulateLulcLevel( 1, m_model.m_colLulcB, m_model.m_colLulcA );

      case 3:
         PopulateLulcLevel( 2, m_model.m_colLulcC, m_model.m_colLulcB );
         return PopulateLulcLevel( 1, m_model.m_colLulcB, m_model.m_colLulcA );

      case 4:
         PopulateLulcLevel( 3, m_model.m_colLulcD, m_model.m_colLulcC );
         PopulateLulcLevel( 2, m_model.m_colLulcC, m_model.m_colLulcB );
         return PopulateLulcLevel( 1, m_model.m_colLulcB, m_model.m_colLulcA );
      }

   return false;
   }


bool CEnvDoc::PopulateLulcLevel( int level, int thisCol, int parentCol )
   {
   bool success = true;
   CMap< int, int, int, int > missingValues;

   int totalCells = m_model.m_pIDULayer->GetRecordCount( MapLayer::ALL );

   
   // eg. PopulateLulcLevel( 1, m_model.m_colLulcB, m_model.m_colLulcA );

   for ( int i=0; i < totalCells; i++ )
      {
      // Get more detailed Lulc value
      int lulc;
      m_model.m_pIDULayer->GetData( i, thisCol, lulc );

      //Get corresponding node in lulcTree
      LulcNode *pNode = m_model.m_lulcTree.FindNode( level+1, lulc );

      if ( pNode != NULL )    // if lulc is in the tree
         {
         //Get parent lulc
         pNode = pNode->m_pParentNode;
         ASSERT( pNode != NULL );
         lulc = pNode->m_id;  // this is now the parent LULC
         }
      else                    // if lulc is not in the tree
         {
         int value;
         if ( ! missingValues.Lookup( lulc, value ) )
            {
            missingValues.SetAt( lulc, lulc );
            success = false;
            }
         lulc = -99;
         }

      m_model.m_pIDULayer->SetData( i, parentCol, lulc );
      }

   int errorCount = (int) missingValues.GetCount();
   if ( errorCount > 0 )
      {
      CString msg;
      msg.Format( "Can't find the following Level %i Lulc IDs in the tree:\n  ", level );

      POSITION pos = missingValues.GetStartPosition();
      while (pos != NULL)
         {
         int lulc;
         missingValues.GetNextAssoc( pos, lulc, lulc );
         CString moreMsg;
         moreMsg.Format("%d ", lulc );
         msg += moreMsg;
         }
      msg += "\n";
      msg += "Corresponding values in the LULC hierarchy for all such cells will be set to -99.";
      msg += "  ENVISION can proceed but this is a problem that should be fixed.";
      MessageBox( GetActiveWindow(), msg, "Error", MB_OK | MB_ICONEXCLAMATION );
      }

   return success;
   }


void CEnvDoc::OnDataProjectTo3d()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY, _T( "Ascii DEM Grid Files|*.dem|All files|*.*||") );

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );

      Map *pMap = gpMapPanel->m_pMap;
      MapLayer *pDEM = pMap->AddGridLayer( filename, DOT_INT );

      // layer added succesfully?
      if ( pDEM != NULL )
         m_model.m_pIDULayer->Project3D( pDEM );

      pMap->RemoveLayer( pDEM, true );
      }   
   }


void CEnvDoc::OnDataHistogram()
   {
   HistogramDlg dlg;
   dlg.m_pLayer = m_model.m_pIDULayer;

   dlg.DoModal();
   }


void CEnvDoc::OnMapAreaSummary()
   {
   AreaSummaryDlg dlg( m_model.m_pIDULayer );
   dlg.DoModal();
   }



void CEnvDoc::OnMapAddDotDensityMap()
   {
   Map *pMap = m_model.m_pIDULayer->m_pMap;

   pMap->CreateDotDensityPointLayer( m_model.m_pIDULayer, m_model.m_pIDULayer->GetActiveField() );

   gpMapPanel->m_mapFrame.m_pMapList->Refresh();
   }


void CEnvDoc::OnMapConvert()
   {
   ConvertDlg dlg( gpView );
   dlg.DoModal();
   }



void CEnvDoc::OnSensitivityAnalysis()
   {
   SensitivityDlg dlg( gpView );
   dlg.DoModal();
   }



void CEnvDoc::OnDataDistanceTo()
   {
   PopulateDistanceDlg dlg( gpMapPanel->m_pMap, gpView );
   dlg.DoModal();
   }

void CEnvDoc::OnDataNextTo()
   {
   PopulateAdjacencyDlg dlg( gpMapPanel->m_pMap, gpView );
   dlg.DoModal();
   }


void CEnvDoc::OnDataElevation()
   {
   int col = m_model.m_pIDULayer->GetFieldCol( "elev" );

   if ( col < 0 )
      {
      int result = gpView->MessageBox( "Field 'ELEV' does not exist in the Cell database.  Would you like to add it?", "Add Fields", MB_OKCANCEL );

      if ( result == IDCANCEL )
         return;

      col = m_model.m_pIDULayer->m_pDbTable->AddField( "Elev", TYPE_INT );
      }

   m_model.m_pIDULayer->PopulateElevation( col );
   }



void CEnvDoc::OnDataLinearDensity()
   {
   LinearDensityDlg dlg;
   dlg.DoModal();
   }


void CEnvDoc::OnDataTextToID()
   {
   TextToIdDlg dlg;
   dlg.DoModal();
   }


void CEnvDoc::OnDataIntersect()
   {
   IntersectDlg dlg;
   dlg.DoModal();
   }


void CEnvDoc::OnDataReclass()
   {
   ReclassDlg dlg;
   dlg.DoModal();
   }


void CEnvDoc::OnDataPopulateIduIndex()
   {
   int col = m_model.m_pIDULayer->GetFieldCol( "IDU_INDEX" );

   if ( col < 0 )
      {
      Report::StatusMsg( "Creating IDU_INDEX field..." );
      col = m_model.m_pIDULayer->m_pDbTable->AddField( "IDU_INDEX", TYPE_LONG );
      }

   Report::StatusMsg( "Writing IDU_INDEX values..." );
   for ( MapLayer::Iterator i = m_model.m_pIDULayer->Begin(); i != m_model.m_pIDULayer->End(); i++ )
      m_model.m_pIDULayer->SetData( i, col, i );
   }


void CEnvDoc::OnDataEvaluativeModelLearning()
   {
   // TODO: Add your command handler code here
   EvaluatorLearning eml;
   eml.adUbiOmniHoc( gpView );
   }


int CEnvDoc::OpenDocXml( LPCTSTR filename )
   {
   Clear();

   //InitDoc();

   EnvLoader loader;
   loader.LoadProject( filename, gpMapPanel->m_pMap, &m_model);
   
   gpCellLayer = m_model.m_pIDULayer;

   gpMapPanel->m_pMap->m_pExtraPtr = gpMapPanel->m_pMapWnd;
   gpMapPanel->m_pMapWnd->LoadBkgrImage();
   gpMapPanel->m_pMapWnd->ZoomFull( true );

   if ( gpView->m_policyEditor.m_hWnd == NULL )
      gpView->m_policyEditor.Create( IDD_POLICYEDITOR, gpView );   // initially invisible

   // fix up interface based on loaded information
   gpMain->SetYearInfo( m_model.m_startYear, m_model.m_yearsToRun );
   gpMain->SetExportDeltaFieldList( m_model.m_exportDeltaFields );
   gpMain->SetExportInterval( m_model.m_exportMapInterval );

   if ( gpMapPanel->m_pMap->GetLayerCount() == 0 )
      {
      AfxMessageBox( "No IDU coverage loaded - Envision cannot continue... This is likely caused by a bad <layer> specification in your ENVX file", MB_OK );
      return -1;
      }

   for ( int i=0; i < gpMapPanel->m_pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( i );
      gpView->m_dataPanel.AddMapLayer( pLayer );
      }

   gpCellLayer = m_model.m_pIDULayer;

   ReconcileDataTypes();

   m_model.CheckValidFieldNames( );

   gpView->m_policyEditor.ReloadPolicies( false );
   gpView->AddStandardRecorders();
   gpDoc->UnSetChanged( CHANGED_ACTORS | CHANGED_POLICIES | CHANGED_SCENARIOS | CHANGED_PROJECT );
   return 1;   
   }

/*
int CEnvDoc::SavePolicyRecordsWeb()
   {

   CArray<int> goalIDArray;  // holds the GoalID in DB for models in m_model order
   ModelNumberToDbGoalIDWeb( goalIDArray );

   Policy *pPolicy = NULL;
   bool result;
   int count = 0;

   EnvDataWebService::CEnvDataWebService ws;
   HRESULT hr = ws.SetUrl(m_databaseName);
   if (S_OK != hr) { ASSERT(0); }

   // Delete deleted policies.  Assume PolicyEditor Rules have been enforced already.
   // The delete will Cascade to PolicyGoal table in the service.
   for ( int i=0; i < gpPolicyManager->GetDeletedPolicyCount(); i++ )
      {
      pPolicy = gpPolicyManager->GetDeletedPolicy( i );
      result = false;
      if ( ! pPolicy->m_isAdded )
         hr = ws.DeletePolicy( pPolicy->m_id, &result );  
      if (E_FAIL == hr)  
         Report::WarningMsg(CW2CT(ws.m_fault.m_strFaultString));
      }

   // Iterate over all policies in Envision.
   // Send the Policy to the Server; include the m_isAdded flag.
   // the result indicates disposition.
   // If disposition indicates new policy,
   // then change PolicyGoal table accordingly.
   // Note possibility that a new Goal has been introduced
     
   for ( int i=0; i < gpPolicyManager->GetPolicyCount(); i++ )
      {
      pPolicy = gpPolicyManager->GetPolicy( i );

      // Let the server decide what to do with this policy in the database
      int  permanentDbID = -1;      //////result = false; 

      hr = ws.PutPolicy( pPolicy->m_id, 
         CComBSTR( pPolicy->m_name ), 
         CComBSTR( pPolicy->m_originator ),
         pPolicy->m_persistenceMin, 
         pPolicy->m_persistenceMax,
         pPolicy->m_isScheduled,
         pPolicy->m_mandatory, 
         pPolicy->m_exclusive,
         pPolicy->m_startDate,
         pPolicy->m_endDate,
         CComBSTR( pPolicy->m_siteAttrStr ), 
         CComBSTR( pPolicy->m_outcomeStr ), 
         pPolicy->m_use ,
         CComBSTR( pPolicy->m_narrative ), 
         CComBSTR( pPolicy->m_studyArea ), 
         pPolicy->m_color, 
         pPolicy->m_isShared,
         pPolicy->m_isEditable,
         //isDeleted not a member of pPolicy
         pPolicy->m_isAdded,
         CComBSTR( gpDoc->m_userName ), // checked against originator in DB
         &permanentDbID ) ; ///////&result );
      
      if ( E_FAIL == hr )
         {
         CString msg( "Unable to save policy <" );
         msg += pPolicy->m_name + "> to the web site " + m_databaseName;
         msg +=  CW2CT(ws.m_fault.m_strFaultString);
         Report::WarningMsg( msg );
         }
      if (permanentDbID == -1 ) ////////( result == false )
         continue;

      // policy entered, add policy goal records
      for ( int i=0; i < m_model.GetMetagoalCount(); i++ )
         {
         int goalID = goalIDArray.GetAt(i);
         result = false;
         hr = ws.PutPolicyGoal( permanentDbID, goalID, pPolicy->GetGoalScoreGlobal( i ), &result );
         if (result != true || E_FAIL == hr )  
            {
            CString msg;
            msg.Format( "Unable to save PolicyGoalRecord <%d, %d> to the web site %s",
               pPolicy->m_id, goalID, m_databaseName);
            msg +=  CW2CT(ws.m_fault.m_strFaultString); 
            Report::WarningMsg( msg );
            }
         }
         if ( pPolicy->m_isAdded )
            {
            pPolicy->m_id = permanentDbID;
            pPolicy->m_isAdded = false;
            }
      }
   ++count;

   return count;
   }
   */



void CEnvDoc::OnDataVerifyCellLayer()
   {
   if( m_model.VerifyIDULayer() )
      MessageBox( GetActiveWindow(), "Cell Layer Validated!", "ENVISION", MB_OK );
   else
      ; // MessageBox( GetActiveWindow(), "Errors detected in the Cell Layer!", "ENVISION", MB_OK );
   }


void CEnvDoc::OnDataInitializePolicyEfficacies()
   {
   CWaitCursor c;
   gpMain->SetStatusMsg( "Initializing Policy Efficacies" );

   MultiSandboxDlg dlg;
   dlg.DoModal();

   //gpView->m_outputTab.m_policyTab.Load();
   //gpView->m_outputTab.m_policyTab.RedrawWindow();
   gpMain->SetStatusMsg( "" );

   SetChanged( CHANGED_POLICIES );
   }


void CEnvDoc::OnDataArea()
   {
   CWaitCursor c;
   gpMain->SetStatusMsg( "Initializing Areas" );

   int colArea = m_model.m_pIDULayer->GetFieldCol( "Area" );

   if ( colArea < 0 )
      {
      int retVal = MessageBox( GetActiveWindow(), "Unable to find field 'AREA'.  Would you like to add this to the coverage?", "Add AREA field?", MB_YESNO );

      if ( retVal == IDNO )
         return;
      
      colArea = m_model.m_pIDULayer->m_pDbTable->InsertField( 1, "AREA", TYPE_FLOAT );
      ASSERT( colArea >= 0 );
      }

   for ( MapLayer::Iterator i = m_model.m_pIDULayer->Begin(); i != m_model.m_pIDULayer->End(); i++ )
      {
      Poly *pPoly = m_model.m_pIDULayer->GetPolygon( i );
      float area = pPoly->GetArea();

      if ( area < 0 )
         {
         CString msg;
         msg.Format( "Negative area encountered for polygon %i, area = %f", (int) i, area );
         Report::WarningMsg( msg );
         }

      m_model.m_pIDULayer->SetData( i, colArea, area );

      if ( int( i ) % 1000 == 0 )
         {
         CString msg;
         msg.Format( "Calulating area for polygon %i" );
         gpMain->SetStatusMsg( msg );
         }
      }

   CString finishedMessage( "The AREA field has been populated. To save these changes, select the File->Save->Database Menu Option." );
   MessageBox( GetActiveWindow(), finishedMessage, "Finished", MB_OK );
   SetChanged( CHANGED_COVERAGE );
   }




void CEnvDoc::OnExportPoliciesashtmldocument()
   {
   DirPlaceholder d;

   nsPath::CPath path( m_model.m_pIDULayer->m_path );
   path.RenameExtension( _T("html") );

   CFileDialog dlg( FALSE, "html", path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "Html Files|*.html|All files|*.*||");

   if ( dlg.DoModal() != IDOK )
      return;

   CString filename( dlg.GetPathName() );

   CWaitCursor c;
   
   int count = gpPolicyManager->GetPolicyCount();
   MapFrame *pMapFrame = &gpMapPanel->m_mapFrame;
   Map *pMap = pMapFrame->m_pMap; //m_model.m_pIDULayer->GetMapPtr();
   ASSERT( pMap != NULL );

   // open file and start writing
   ofstream file;
   file.open( filename );

   CString buffer;
   nsPath::CPath _path( filename );
   CString _filename ( _path.GetName() );

   file << _T("<html><head><title>Policy Summary</title></head><body>");
   file << _T("<table width='100%'><tr><td style='background-color:#66CCFF'>" );
   file << _T("<h1>Policy Summary - ") << _filename << _T("</h1></td></tr></table><hr>");

   // links to each policy entry (table of contents)
   for ( int i=0; i < count; i++ )
      {
      Policy *pPolicy = gpPolicyManager->GetPolicy( i );
      file << "<a href='#" << pPolicy->m_name << "' >" <<  pPolicy->m_name << "</a><br/>";
      }

   file << "<hr>";

   //m_model.m_pIDULayer->SetActiveField( m_model.m_pIDULayer->GetFieldCol( gpModel->m_lulcTree.GetFieldName( 1 ) ) );  //"LULC_A" ) );
   //m_model.m_pIDULayer->ClassifyData();

   for ( int i=0; i < count; i++ )
      {
      Policy *pPolicy = gpPolicyManager->GetPolicy( i );
 
      // make sure policy compiled
      if ( pPolicy->m_pSiteAttrQuery == NULL )
         {
         gpPolicyManager->CompilePolicy( pPolicy, m_model.m_pIDULayer );

         // bad compilation?
         if ( pPolicy->m_pSiteAttrQuery == NULL )
            {
            CString msg( "Error compiling site attribute query: " );
            msg += pPolicy->m_siteAttrStr;
            Report::ErrorMsg( msg );
            }
         }

      nsPath::CPath imagepath( filename );

      imagepath.RemoveFileSpec();
      CString imgfile;
      imgfile.Format( "PolicyID%i.png", i );
      imagepath.Append( imgfile );
      
      // generate map
      CString appliesTo;

      if ( pPolicy->m_pSiteAttrQuery == NULL )
         {
         m_model.m_pIDULayer->ClearSelection();
         pMapFrame->RedrawWindow();
         }
      else
         {
         int count = m_model.m_pQueryEngine->SelectQuery( pPolicy->m_pSiteAttrQuery, true ); // redraw= true );
         pMapFrame->m_pMapWnd->RedrawWindow();
         int iduCount = m_model.m_pIDULayer->GetRowCount();
         appliesTo.Format( "Applies to %i of %i sites (%4.2f%%)", count, iduCount, 100.0f * count / iduCount );
         }

      //pMap->SaveAsEnhancedMetafile( imagepath, false );
      pMapFrame->m_pMapWnd->SaveAsImage( imagepath, 0, MDF_ALL );

      // main label
      file << "<a name='" << pPolicy->m_name << "' />";
      file << "<table width='100%' border='0'> <tr> <td colspan='2' style='background-color:#66CCFF' ><h2>" << pPolicy->m_name << "</h2> </td> </tr>";

      file << "<tr><td colspan='2'>" << pPolicy->m_narrative << "</td></tr>";

      // second row - origniator + map 
      file << "<tr> <td width='25%'></td> <td rowspan='2'>"; 
      file << "<img width='400' height='400' src='" << imgfile << "' /> </td></tr>";


      // additional rows
      file << "<tr><td>";

      CString siteAttr;
      pPolicy->GetSiteAttrFriendly( siteAttr, ALL_OPTIONS & ~SHOW_LEADING_TAGS);
      file << "<b>Site Attributes: </b><br>" << siteAttr;

      CString outcomes;
      pPolicy->GetOutcomesFriendly( outcomes );
      file << "<p><b>Outcomes: </b><br>" <<  outcomes;

      CString scores;
      pPolicy->GetScoresFriendly( scores );
      file << "<p><b>Scores (-3 to 3 scale): </b><br>" <<  scores;

      file << "<p><b>Minimum Persistence: </b>";
      buffer.Format( "%i", pPolicy->m_persistenceMin );
      file << buffer;
      file << "<br><b>Maximum Persistence: </b>";
      buffer.Format( "%i", pPolicy->m_persistenceMax );
      file << buffer;

      buffer.Empty();
      SetBoolHtml( buffer, "Exclusive: ", pPolicy->m_exclusive );
      //SetBoolHtml( buffer, "Shared: ", pPolicy->m_isShared );
      //SetBoolHtml( buffer, "Editable: ", pPolicy->m_isEditable );
      SetBoolHtml( buffer, "Mandatory: ", pPolicy->m_mandatory );
      file << buffer;
      
      if ( pPolicy->m_startDate >= 0 )
         {
         buffer.Format( "<br><b>Start Date:</b> %i", pPolicy->m_startDate );
         file << buffer;
         }

      if ( pPolicy->m_endDate > 0 )
         {
         buffer.Format( "<br><b>End Date:</b> %i", pPolicy->m_endDate );
         file << buffer;
         }

      file << "<br><br>" << appliesTo;

      file << "</td></tr>";
      file << "</table><p/><hr>^";

      }

   file << "</body></html>";

   file.close();
   }


void CEnvDoc::OnExportFieldInfoAsHtml()
   {
   DirPlaceholder d;

   TCHAR fieldInfoFile[ 128 ];
   fieldInfoFile[ 0 ] = NULL;
   if ( m_fieldInfoFileArray.GetCount() > 0 )
      {
      nsPath::CPath path( m_fieldInfoFileArray[ 0 ] );
      path.RenameExtension( _T("html") );
      lstrcpy( fieldInfoFile, path );
      }

   CFileDialog dlg( FALSE, "html", fieldInfoFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "Html Files|*.html|Htm Files|*.htm|All files|*.*||");

   if ( dlg.DoModal() != IDOK )
      return;

   CString filename( dlg.GetPathName() );

   // open file and start writing
   ofstream file;
   file.open( filename );

   nsPath::CPath path( filename );

   CString _filename( path.GetName() );
  
   CWaitCursor c;

   // standard header
   file << _T("<html><head><title>Field Information Summary</title></head><body>");
   file << _T("<table width='100%'><tr><td style='background-color:#66CCFF'>" );
   file << _T("<h1>Data Dictionary - ") << _filename << _T("</h1></td></tr></table><hr>");

   // links to each fieldInfo entry
   for ( int i=0; i < m_model.m_pIDULayer->GetFieldInfoCount(1); i++ )
      {
      MAP_FIELD_INFO *pInfo = m_model.m_pIDULayer->GetFieldInfo( i );

      if ( pInfo->mfiType != MFIT_SUBMENU )
         file << "<a href='#" << pInfo->fieldname << "' >" <<  pInfo->label << " (" << pInfo->fieldname << ")</a><br/>";
      }

   file << "<hr>";

   MapFrame *pMapFrame = &gpMapPanel->m_mapFrame;
   Map *pMap = pMapFrame->m_pMap; //m_model.m_pIDULayer->GetMapPtr();
   ASSERT( pMap != NULL );
   //pMap->m_showScale = false;

   // next - summary info for each field info
   for ( int i=0; i < m_model.m_pIDULayer->GetFieldInfoCount(1); i++ )
      {
      MAP_FIELD_INFO *pInfo = m_model.m_pIDULayer->GetFieldInfo( i );

      if ( pInfo->mfiType != MFIT_SUBMENU )
         {
         int col = m_model.m_pIDULayer->GetFieldCol( pInfo->fieldname );
         if ( col < 0 )
            continue;

         nsPath::CPath imagepath( filename );
      
         Map::SetupBins( pMap, 0, col );

         imagepath.RemoveExtension();
         CString img( imagepath );
         img += pInfo->fieldname;
         img += ".png";

         //pMap->SaveAsEnhancedMetafile( img, false );
         pMapFrame->m_pMapWnd->SaveAsImage( img, 0, MDF_ALL );

         imagepath = img;
         
         // main label
         file << "<a name='" << pInfo->fieldname << "' />";
         file << "<table width='100%' border='0'> <tr> <td colspan='3' style='background-color:#66CCFF' ><h2>" << pInfo->label << " (" << pInfo->fieldname << ")</h2> </td> </tr>";

         file << "<tr><td colspan='3'>" << "<b>Description:</b> " << pInfo->description << "</td></tr>";
         file << "<tr><td colspan='3'>" << "<b>Source:</b> " << pInfo->source << "</td></tr>";



         // second row - label + map
         LPCTSTR typeStr = ::GetTypeLabel( pInfo->type );
         file << "<tr> <td colspan='2'>Type: " << typeStr << "</td> <td rowspan='4'>";

         if ( col >= 0 )
            file << "<img width='400' height='400' src='" << imagepath << "' /> </td></tr>";
         else
            file << "</td></tr>";

         bool outcomes = pInfo->GetExtraLow() & WORD( 4 ) ? true : false;
         bool siteAttr = pInfo->GetExtraLow() & WORD( 2 ) ? true : false;
         
         //third row - usage
         file << "<tr> <td>";
         if ( siteAttr )
            file << "Use In Site Attributes";
         file << "</td></tr><tr><td>";

         // fourth row
         if ( outcomes )
            file << "Use In Outcomes";
         file << "</td></tr>";

         // attributes next
         file << "<tr><td colspan='2'><table border='1'> <tr> <td colspan='4' style='background-color:#66CCFF'><h3>Attributes</h3> </td> </tr>";

         if ( pInfo->mfiType == MFIT_QUANTITYBINS )         
            file << "<tr> <td width='10px'>&nbsp;Color&nbsp;</td> <td>&nbsp;Minimum&nbsp;</td><td>&nbsp;Maximum &nbsp;</td><td>&nbsp;Label&nbsp;</td></tr>";
         else  // categorical
            file << "<tr> <td width='10px'>&nbsp;Color&nbsp;</td> <td width='50px' colspan='2'>&nbsp;Value&nbsp;</td><td width='100px'>&nbsp;Label&nbsp;</td></tr>";

         for ( int j=0; j < pInfo->GetAttributeCount(); j++ )
            {
            FIELD_ATTR &a = pInfo->GetAttribute( j );

            if ( pInfo->mfiType == MFIT_QUANTITYBINS )         
               {
               int red = GetRValue( a.color );
               int grn = GetGValue( a.color );
               int blu = GetBValue( a.color );
               file << "<tr> <td width='10px' style='background-color:rgb(" << red << "," << grn << "," << blu << ")' > &nbsp; </td>";
               file << "<td>" << a.minVal << "</td><td>" << a.maxVal << "</td><td>" << a.label << "</td></tr>";
               }
            else
               {
               int red = GetRValue( a.color );
               int grn = GetGValue( a.color );
               int blu = GetBValue( a.color );
               file << "<tr> <td width='10px' style='background-color:rgb(" << red << "," << grn << "," << blu << ")' > &nbsp; </td>";
               file << "<td colspan='2'>" << a.value.GetAsString() << "</td><td>" << a.label << "</td></tr>";
               }
            }
         file << "</table></td></tr></table><p/><hr>^";
         }
      }
   file << "</body></html>";

   file.close();

   //pMap->m_showScale = true;

   }


void CEnvDoc::OnExportDeltaArray()
   {
   DeltaExportDlg dlg;
   dlg.DoModal();
   }









void SetBoolHtml( CString &html, LPCTSTR label, bool value )
   {
   html += "<br><b>";
   html += label;
   html += "</b>";

   if ( value )
      html += "yes";
   else
      html += "no";
   }


void CEnvDoc::OnDataMerge()
   {
   MergeDlg dlg;
   dlg.DoModal();
   }

void CEnvDoc::OnDataChangeType()
   {
   ChangeTypeDlg dlg;
   dlg.DoModal();
   }

void CEnvDoc::OnDataAddCol()
   {
   AddDbColDlg dlg;
   dlg.DoModal();
   }

void CEnvDoc::OnDataRemoveCol()
   {
   RemoveDbColsDlg dlg;
   dlg.DoModal();
   }


void CEnvDoc::OnDataCalculateField()
   {
   FieldCalculatorDlg dlg;
   dlg.DoModal();
   }


void CEnvDoc::OnDataBuildnetworktree()
   {
   NetworkTopologyDlg dlg;
   dlg.DoModal();
   }

void CEnvDoc::OnEditFieldInformation()
   {
   FieldInfoDlg dlg;
   dlg.DoModal();   
   }

void CEnvDoc::OnEditIniFile()
   {
   IniFileEditor dlg;
   dlg.DoModal();   
   }


int CEnvDoc::LoadExtensions()
   {
   DWORD    cbName;                   // size of name string 
   TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
   DWORD    cchClassName = MAX_PATH;  // size of class string 
   DWORD    cSubKeys=0;               // number of subkeys 
   DWORD    cbMaxSubKey;              // longest subkey size 
   DWORD    cchMaxClass;              // longest class string 
   DWORD    cValues;              // number of values for key 
   DWORD    cchMaxValue;          // longest value name 
   DWORD    cbMaxValueData;       // longest value data 
   DWORD    cbSecurityDescriptor; // size of security descriptor 
   FILETIME ftLastWriteTime;      // last write time 

   // get values from registry
   HKEY key;
   DWORD result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,  // HKEY_CURRENT_USER
               "SOFTWARE\\OSU Biosystems Analysis Group\\Envision\\AnalysisModules",
               0, KEY_QUERY_VALUE, &key );

   if ( result == ERROR_SUCCESS )
      {
      // Get the class name and the value count. 
      result = RegQueryInfoKey(
           key,                     // key handle 
           achClass,                // buffer for class name 
           &cchClassName,           // size of class string 
           NULL,                    // reserved 
           &cSubKeys,               // number of subkeys 
           &cbMaxSubKey,            // longest subkey size 
           &cchMaxClass,            // longest class string 
           &cValues,                // number of values for this key 
           &cchMaxValue,            // longest value name 
           &cbMaxValueData,         // longest value data 
           &cbSecurityDescriptor,   // security descriptor 
           &ftLastWriteTime);       // last write time 
    
       // Enumerate the subkeys, until RegEnumKeyEx fails.
       if ( cSubKeys )
          {
          // note: each subkey represents a analysis module.. The key name is the module name.
          // Values stored in the key include "path" and "initInfo"
          for ( DWORD i=0; i < cSubKeys; i++ ) 
            { 
            TCHAR  extModuleName[ 256 ];
            DWORD  extModuleSize = 256;

            cbName = 256;
            result = RegEnumKeyEx(key, i, extModuleName, &extModuleSize, NULL, NULL, NULL, &ftLastWriteTime ); 

            if ( result == ERROR_SUCCESS) 
               {
               TCHAR path[ 256 ];
               TCHAR initInfo[ 256 ];
               DWORD nPath = 256;
               DWORD type = REG_SZ;
               bool use = true;
                  
               result = RegQueryValueEx( key, extModuleName, NULL, &type, (LPBYTE) path, &nPath );
               result = RegQueryValueEx( key, extModuleName, NULL, &type, (LPBYTE) initInfo, &nPath );
               //RegGetValue( key, extModuleName, _T("path"), RRF_RT_REG_SZ, &type, (LPVOID) &path, &nPath );
               //RegGetValue( key, extModuleName, _T("initInfo"), RRF_RT_REG_SZ, &type, (LPVOID) &initInfo, &nPath );

               // try to load it
               HINSTANCE hDLL = AfxLoadLibrary( path ); 
               if ( hDLL == NULL )
                  {
                  CString msg( _T( "Unable to find ") );
                  msg += path;
                  msg += ( _T(" or a dependent DLL") );
                  Report::ErrorMsg( msg );
                  Report::BalloonMsg( msg );
                  use = false;
                  }
               else
                  {
                  //runFn = (ANALYSISMODULEFN) GetProcAddress( hDLL, "AMRun" );
                  //if ( runFn == NULL  )
                  //   {
                  //   CString msg( _T("Unable to find function 'AMRun()' in ") );
                  //   msg += path;
                  //   Report::ErrorMsg( msg );
                  //   AfxFreeLibrary( hDLL );
                  //   hDLL = 0;
                  //   use = false;
                  //   }
                  }
               
               EnvAnalysisModule *pInfo = new EnvAnalysisModule;
               pInfo->m_hDLL = hDLL;
               pInfo->m_name = extModuleName;
               pInfo->m_path = path;
               pInfo->m_initInfo = initInfo;
               pInfo->m_use = use;
               this->m_analysisModuleArray.Add( pInfo );
               }
             }
          }
      }

   RegCloseKey( key );

   //----------------- Visualizers next -------------------------------------
   result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,  // HKEY_CURRENT_USER
               "SOFTWARE\\OSU Biosystems Analysis Group\\Envision\\Visualizers",
               0, KEY_QUERY_VALUE, &key );

   if ( result == ERROR_SUCCESS )
      {
      // Get the class name and the value count. 
      result = RegQueryInfoKey(
           key,                     // key handle 
           achClass,                // buffer for class name 
           &cchClassName,           // size of class string 
           NULL,                    // reserved 
           &cSubKeys,               // number of subkeys 
           &cbMaxSubKey,            // longest subkey size 
           &cchMaxClass,            // longest class string 
           &cValues,                // number of values for this key 
           &cchMaxValue,            // longest value name 
           &cbMaxValueData,         // longest value data 
           &cbSecurityDescriptor,   // security descriptor 
           &ftLastWriteTime);       // last write time 
    
       // Enumerate the subkeys, until RegEnumKeyEx fails.
       if ( cSubKeys )
          {
          // note: each subkey represents a visualizer module.. The key name is the module name.
          // Values stored in the key include "name", "type", "path" and "initInfo"
          for ( DWORD i=0; i < cSubKeys; i++ ) 
            { 
            TCHAR  extModuleName[ 256 ];
            DWORD  extModuleSize = 256;

            cbName = 256;
            result = RegEnumKeyEx(key, i, extModuleName, &extModuleSize, NULL, NULL, NULL, &ftLastWriteTime ); 

            if ( result == ERROR_SUCCESS) 
               {
               TCHAR name[ 256 ];
               TCHAR path[ 256 ];
               TCHAR initInfo[ 256 ];
               DWORD type;
               DWORD nPath = 256;
               DWORD datatype = REG_SZ;
               bool use = true;
                  
               result = RegQueryValueEx( key, _T("name"), NULL, &datatype, (LPBYTE)  name, &nPath );
               result = RegQueryValueEx( key, _T("type"), NULL, &datatype, (LPBYTE) &type, &nPath );
               result = RegQueryValueEx( key, _T("path"), NULL, &datatype, (LPBYTE)  path, &nPath );
               result = RegQueryValueEx( key, _T("initInfo"), NULL, &datatype, (LPBYTE) initInfo, &nPath );

               // try to load it
               HINSTANCE hDLL = AfxLoadLibrary( path ); 
               if ( hDLL == NULL )
                  {
                  CString msg( _T( "Unable to find ") );
                  msg += path;
                  msg += ( _T(" or a dependent DLL") );
                  Report::ErrorMsg( msg );
                  Report::BalloonMsg( msg );
                  use = false;
                  }
               else
                  {
                  //initFn = (INITFN) GetProcAddress( hDLL, "VInit" );
                  //if ( initFn == NULL  )
                  //   {
                  //   CString msg( _T("Unable to find function 'Init()' in ") );
                  //   msg += path;
                  //   Report::ErrorMsg( msg );
                  //   AfxFreeLibrary( hDLL );
                  //   hDLL = 0;
                  //   use = false;
                  //   }
                  }

               EnvVisualizer *pViz = new EnvVisualizer;
               
               pViz->m_hDLL = hDLL;
               pViz->m_vizType = (VIZ_TYPE) type;
               pViz->m_initInfo = initInfo;
               pViz->m_name = name;
               pViz->m_path = path;
               pViz->m_use = use;
               this->m_visualizerArray.Add( pViz );
               }
             }
          }
      }

   RegCloseKey( key );

   return (int) m_analysisModuleArray.GetSize();
   }
     
bool CEnvDoc::CheckForUpdates( void )     // return true to update, false otherwise
   {
   UpdateDlg dlg;

   if ( dlg.Init() )
      {
      if ( ! dlg.m_disable  )
         {
         if ( dlg.DoModal() == IDOK )
            theApp.OnHelpUpdateEnvision();
         }
      }
   
   return true;
   }




int CEnvDoc::ReconcileDataTypes(void)
   {
   ASSERT(m_model.m_pIDULayer);
   int col=-1;

      // Models must have fields TYPE_DOUBLE--overwrite default field type.
   //ASSERT ( m_model.GetEvaluatorCount() > 0 );
   for ( int i=0; i < m_model.GetEvaluatorCount(); i++ ) 
      {
      //if (0 < (col = m_model.GetEvaluatorInfo(i)->col))
      //      m_model.m_pIDULayer->SetFieldType( col, TYPE_DOUBLE, false);
      }
   
   for ( int i=0; i < m_model.GetModelProcessCount(); i++ ) 
      {
      //if (0 < (col = m_model.GetModelProcessInfo(i)->col))
      //   m_model.m_pIDULayer->SetFieldType(col, TYPE_DOUBLE, false);
      }

   // Overwrite  MAP_FIELD_INFO that contradicts the m_model.m_pIDULayer DataType
   // in the case of integral vs floating point.
   int more = IDYES;
   for ( int i=0; i < m_model.m_pIDULayer->GetFieldInfoCount(1); i++ ) 
      {
      MAP_FIELD_INFO *pInfo = m_model.m_pIDULayer->GetFieldInfo( i ); // ... as specified in the Envision .ini file.
      col = m_model.m_pIDULayer->GetFieldCol( pInfo->fieldname );
      if ( col == -1 )
         continue;

      TYPE fiType = pInfo->type;
      TYPE dbType  = m_model.m_pIDULayer->GetFieldType( col );

      if ( IsInteger( fiType ) && IsReal( dbType ) )
         {
         CString msg;
         msg.Format( "Integer/Real data type mismatch:  Field [%s] is indicated as Integer in it's Field Info, but is floating point in the coverage.  Do you want to convert the coverage to Integer type?", (LPCTSTR) pInfo->fieldname );
         int retVal = gpMain->MessageBox( msg, "Data Type Mismatch Detected", MB_YESNOCANCEL );

         switch( retVal )
            {
            case IDYES:
               {
               m_model.m_pIDULayer->SetFieldType( col, TYPE_LONG, true );
               SetChanged( CHANGED_COVERAGE );
               }
               break;

            case IDCANCEL:
               return 0;
            }
         }

      if ( IsReal( fiType ) && IsInteger( dbType ) )
         {
         CString msg;
         msg.Format( "Integer/Real data type mismatch:  Field [%s] is indicated as floating point in it's Field Info, but is Integer in the coverage.  Do you want to convert the coverage to floating point type?", (LPCTSTR) pInfo->fieldname );
         int retVal = gpMain->MessageBox( msg, "Data Type Mismatch Detected", MB_YESNOCANCEL );

         switch( retVal )
            {
            case IDYES:
               {
               m_model.m_pIDULayer->SetFieldType( col, TYPE_DOUBLE, true );
               SetChanged( CHANGED_COVERAGE );
               }
               break;

            case IDCANCEL:
               return 0;
            }
         }
      }

   return 1;
   }


