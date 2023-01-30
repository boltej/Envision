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

// EnvView.cpp : implementation of the EnvView class
//

#include "stdafx.h"
#include "Envision.h"
#include ".\EnvView.h"
#include "mainfrm.h"
#include "actorDlg.h"
#include "MapPanel.h"
#include "EnvDoc.h"
#include <EnvModel.h>
#include "SelectActor.h"
#include "wizrun.h"
#include <DataManager.h>
#include <EnvException.h>
#include "ExportDataDlg.h"
#include "deltaviewer.h"
#include "PolicyColorDlg.h"
#include "ScenarioEditor.h"
#include "databaseInfoDlg.h"
#include "ActiveWnd.h"
#include "ActorDlg.h"
#include "LulcEditor.h"

#include <querybuilder.h>
#include <map.h>
#include <MapWnd.h>
#include <VDataTable.h>
#include <FDataObj.h>
#include <direct.h>
#include <Path.h>

#include <ColumnTrace.h>
//#include "VideoRecorderDlg.h"

#include <raster.h>

#include <AlgLib\AlgLib.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

InputPanel    *gpInputPanel   = NULL;
MapPanel      *gpMapPanel     = NULL;
ResultsPanel  *gpResultsPanel = NULL;
RtViewPanel   *gpRtViewsPanel = NULL;
AnalyticsPanel *gpAnalyticsPanel = NULL;
CEnvView      *gpView         = NULL;

extern MapLayer      *gpCellLayer;
extern CEnvDoc       *gpDoc;
extern PolicyManager *gpPolicyManager;
extern ActorManager  *gpActorManager;
extern EnvModel      *gpModel;
extern CMainFrame    *gpMain;
//__EXPORT__  Map3d    *gpMap3d;      // defined in Map3d.h


// CEnvView

IMPLEMENT_DYNCREATE(CEnvView, CView)

BEGIN_MESSAGE_MAP(CEnvView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CEnvView::OnFilePrintPreview)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_COPYLEGEND, OnEditCopyLegend)
	ON_COMMAND(ID_ZOOMIN, OnZoomin)
 	ON_UPDATE_COMMAND_UI(ID_ZOOMIN, OnUpdateIsMapWnd)
	ON_COMMAND(ID_ZOOMOUT, OnZoomout)
 	ON_UPDATE_COMMAND_UI(ID_ZOOMOUT, OnUpdateIsMapWnd)
	ON_COMMAND(ID_ZOOMTOQUERY, OnZoomToQuery)
 	ON_UPDATE_COMMAND_UI(ID_ZOOMTOQUERY, OnUpdateIsMapWnd)
	ON_COMMAND(ID_SELECT, OnSelect)
 	ON_UPDATE_COMMAND_UI(ID_SELECT, OnUpdateIsMapWnd)
   ON_COMMAND(ID_CLEAR_SELECTION, OnClearSelection)
 	ON_UPDATE_COMMAND_UI(ID_CLEAR_SELECTION, OnUpdateIsSelection)
	ON_COMMAND(ID_QUERYDISTANCE, OnQuerydistance)
 	ON_UPDATE_COMMAND_UI(ID_QUERYDISTANCE, OnUpdateIsMapWnd)
	ON_COMMAND(ID_ZOOMFULL, OnZoomfull)
 	ON_UPDATE_COMMAND_UI(ID_ZOOMFULL, OnUpdateIsMapWnd)
	//ON_COMMAND(ID_DBTABLE, OnDbtable)
	//ON_UPDATE_COMMAND_UI(ID_DBTABLE, OnUpdateIsMapWnd)
	ON_COMMAND(ID_PAN, OnPan)
	ON_UPDATE_COMMAND_UI(ID_PAN, OnUpdateIsMapWnd)

	ON_COMMAND(ID_RESULTS_CASCADE, OnResultsCascade)
	ON_UPDATE_COMMAND_UI(ID_RESULTS_CASCADE, OnUpdateResultsTileCascade)
	ON_COMMAND(ID_RESULTS_TILE, OnResultsTile)
	ON_UPDATE_COMMAND_UI(ID_RESULTS_TILE, OnUpdateResultsTileCascade)
	ON_COMMAND(ID_RESULTS_CLEAR, OnResultsClear)
	ON_UPDATE_COMMAND_UI(ID_RESULTS_CLEAR, OnUpdateResultsTileCascade)
	//ON_COMMAND(ID_RESULTS_CAPTURE, OnResultsCapture)
	//ON_UPDATE_COMMAND_UI(ID_RESULTS_CAPTURE, OnUpdateResultsCapture)
   ON_COMMAND(ID_RESULTS_DELTAS, OnResultsDeltas)
   ON_UPDATE_COMMAND_UI(ID_RESULTS_DELTAS, OnUpdateResultsDeltas)

   
   ON_COMMAND(ID_RESULTS_SYNC_MAPS, OnResultsSyncMaps)
	ON_UPDATE_COMMAND_UI(ID_RESULTS_SYNC_MAPS, OnUpdateResultsSyncMaps)
   ON_UPDATE_COMMAND_UI(ID_RTVIEW_MAPS, OnUpdateData)

	ON_COMMAND(ID_ANALYTICS_CASCADE, OnAnalyticsCascade)
	ON_UPDATE_COMMAND_UI(ID_ANALYTICS_CASCADE, OnUpdateAnalyticsTileCascade)
	ON_COMMAND(ID_ANALYTICS_TILE, OnAnalyticsTile)
	ON_UPDATE_COMMAND_UI(ID_ANALYTICS_TILE, OnUpdateAnalyticsTileCascade)
	ON_COMMAND(ID_ANALYTICS_CLEAR, OnAnalyticsClear)
	ON_UPDATE_COMMAND_UI(ID_ANALYTICS_CLEAR, OnUpdateAnalyticsTileCascade)
	ON_COMMAND(ID_ANALYTICS_CAPTURE, OnAnalyticsCapture)
	ON_UPDATE_COMMAND_UI(ID_ANALYTICS_CAPTURE, OnUpdateAnalyticsCapture)
	ON_COMMAND(ID_ANALYTICS_ADD, OnAnalyticsAdd)
	ON_UPDATE_COMMAND_UI(ID_ANALYTICS_ADD, OnUpdateAnalyticsAdd)
   ON_UPDATE_COMMAND_UI(ID_RTVIEW_MAPS, OnUpdateData)


   ON_WM_KEYDOWN()

   ON_COMMAND(ID_VIEW_INPUT, OnViewInput)
   ON_COMMAND(ID_VIEW_MAP, OnViewMap)
   ON_COMMAND(ID_VIEW_TABLE, OnViewTable)
   ON_COMMAND(ID_VIEW_RTVIEWS, OnViewRtViews)
   ON_COMMAND(ID_VIEW_RESULTS, OnViewResults)
   ON_COMMAND(ID_VIEW_ANALYTICS, OnViewAnalytics)
   
   ON_COMMAND(ID_VIEW_POLYGONEDGES, OnViewPolygonedges)
	ON_UPDATE_COMMAND_UI(ID_VIEW_POLYGONEDGES, OnUpdateViewPolygonedges)
   ON_COMMAND(ID_VIEW_SHOWSCALE, OnViewShowMapScale)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWSCALE, OnUpdateViewShowMapScale)
   ON_COMMAND(ID_VIEW_SHOWATTRIBUTE, OnViewShowAttribute)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWATTRIBUTE, OnUpdateViewShowAttribute)

   ON_COMMAND(ID_ANALYSIS_SHOWACTORTERRITORY, OnAnalysisShowactorterritory)
	ON_COMMAND(ID_ANALYSIS_SHOWACTORS, OnAnalysisShowactors)
	ON_WM_MOUSEWHEEL()

   // Data menu
   ON_COMMAND(ID_DATA_LULCINFO, OnDataLulcInfo)
   ON_COMMAND(ID_DATA_IDUINFO, OnDataIduInfo)

   // Analysis menu
   ON_COMMAND(ID_ANALYSIS_SETUP_RUN, OnAnalysisSetupRun)
   ON_COMMAND(ID_ANALYSIS_SETUP_MODELS, OnAnalysisSetupModels)
   ON_COMMAND(ID_EDIT_POLICIES, OnEditPolicies)
   ON_COMMAND(ID_EDIT_ACTORS, OnEditActors)
   ON_COMMAND(ID_EDIT_LULC, OnEditLulc)
   ON_COMMAND(ID_EDIT_SCENARIOS, OnEditScenarios)
   
   ON_COMMAND(ID_ANALYSIS_EXPORTCOVERAGES, OnExportCoverages)
   ON_COMMAND(ID_ANALYSIS_EXPORTOUTPUTS, OnExportOutputs)
   ON_COMMAND(ID_ANALYSIS_EXPORTDELTAS, OnExportDeltas)
   ON_COMMAND(ID_ANALYSIS_EXPORTDELTAFIELDS, OnExportDeltaFields)
 //  ON_COMMAND(ID_ANALYSIS_VIDEORECORDERS, OnVideoRecorders)
 
	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_EXPORTCOVERAGES, OnUpdateExportCoverages)
	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_EXPORTOUTPUTS, OnUpdateExportOutputs)
	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_EXPORTDELTAS, OnUpdateExportDeltas)
	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_EXPORTDELTAFIELDS, OnUpdateExportDeltaFields)
	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_EXPORTINTERVAL,  OnUpdateExportInterval)
	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_STARTYEAR, OnUpdateStartYear)
	ON_UPDATE_COMMAND_UI(ID_ANALYSIS_STOPTIME,  OnUpdateStopTime)
   
   ON_UPDATE_COMMAND_UI(ID_EDIT_POLICIES, OnUpdateData)
   ON_UPDATE_COMMAND_UI(ID_EDIT_ACTORS, OnUpdateData)
   ON_UPDATE_COMMAND_UI(ID_EDIT_LULC, OnUpdateData)
   ON_UPDATE_COMMAND_UI(ID_EDIT_SCENARIOS, OnUpdateData)
   
   ON_COMMAND(ID_STATUSBAR_ZOOMSLIDER, OnZoomSlider)
   ON_COMMAND(ID_STATUSBAR_ZOOMBUTTON, OnZoomButton)
   
   ON_UPDATE_COMMAND_UI(ID_ANALYSIS_SHOWACTORTERRITORY, OnUpdateData)
   ON_UPDATE_COMMAND_UI(ID_FILE_RESET, OnUpdateData)
   ON_COMMAND(ID_SAVE_SHAPEFILE, OnSaveShapefile)
   ON_UPDATE_COMMAND_UI(ID_SAVE_SHAPEFILE, OnUpdateSaveShapefile)
//   ON_COMMAND(ID_INTERACTIVE_CONNECTTOHOST, OnConnectToHost)
//   ON_UPDATE_COMMAND_UI(ID_INTERACTIVE_CONNECTTOHOST, OnUpdateConnectToHost)
//   ON_COMMAND(ID_INTERACTIVE_HOSTINTERACTIVESESSION, OnHostSession)
//   ON_UPDATE_COMMAND_UI(ID_INTERACTIVE_HOSTINTERACTIVESESSION, OnUpdateHostSession)
   ON_COMMAND(ID_ANALYSIS_SHOWDELTALIST, OnAnalysisShowdeltalist)
   ON_UPDATE_COMMAND_UI(ID_ANALYSIS_SHOWDELTALIST, OnUpdateAnalysisShowdeltalist)
   ON_COMMAND(ID_ANALYSIS_SETUPPOLICYMETAPROCESS, OnAnalysisSetuppolicymetaprocess)
   ON_UPDATE_COMMAND_UI(ID_ANALYSIS_SETUPPOLICYMETAPROCESS, OnUpdateAnalysisSetuppolicymetaprocess)
   ON_COMMAND(ID_DATA_STORERUNDATA, OnDataStorerundata)
   ON_COMMAND(ID_DATA_LOADRUNDATA, OnDataLoadrundata)
   ON_COMMAND(ID_RUNQUERY, OnRunquery)
   ON_UPDATE_COMMAND_UI(ID_RUNQUERY, OnUpdateData)
   ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
   ON_UPDATE_COMMAND_UI(ID_EDIT_COPYLEGEND, OnUpdateIsMapWnd)
   ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
   ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
   ON_WM_CLOSE()
   ON_COMMAND(ID_EDIT_SETPOLICYCOLORS, OnEditSetpolicycolors)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnvView construction/destruction

CEnvView::CEnvView()
: m_mode( MM_NORMAL )
, m_policyEditor( this )
, m_activePanel( MAP_PANEL )
, m_pDefaultZoom( NULL )
   {
   gpView = this;  // initially
   }

CEnvView::~CEnvView()
{
}


BOOL CEnvView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CEnvView drawing

void CEnvView::OnDraw(CDC* /*pDC*/)
{
	CEnvDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CEnvView printing


void CEnvView::OnFilePrintPreview()
{
	AFXPrintPreview(this);
}

BOOL CEnvView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CEnvView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CEnvView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CEnvView::OnRButtonUp(UINT nFlags, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CEnvView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
}


// CEnvView diagnostics

#ifdef _DEBUG
void CEnvView::AssertValid() const
{
	CView::AssertValid();
}

void CEnvView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CEnvDoc* CEnvView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CEnvDoc)));
	return (CEnvDoc*)m_pDocument;
}
#endif //_DEBUG



/////////////////////////////////////////////////////////////////////////////
// CEnvView message handlers

int CEnvView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
   {
   // basic idea - CEnvView maintains a set of windows:
   //  1) Map Panel - GIS front end for IDU Display
   //  2) Data Panel - Tabular access to loaded GIS datatables
   //  3) RtViews Panel - for displaying dynamic results with plug-in views during a simulation run
   //  4) Results Panel - where all results are displayed
   //
   // all are create here, only one visible initially
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

   RECT rect;
   GetClientRect( &rect );

   BOOL ok = m_inputPanel.Create( NULL, "Input Panel", WS_CHILD, rect, this, 100 );
   ASSERT ( ok );

   ok = m_mapPanel.Create( NULL, "Map View", WS_CHILD | WS_VISIBLE, rect, this, 101 );
   ASSERT ( ok );

   ok = m_dataPanel.Create( this );
   ASSERT( ok );

   ok = m_viewPanel.Create( NULL, "RtViews Panel", WS_CHILD | WS_CLIPCHILDREN, rect, this, 102 );
   ASSERT( ok );

   ok = m_resultsPanel.Create( NULL, "Results View", WS_CHILD | WS_CLIPCHILDREN, rect, this, 104 );
   ASSERT( ok );

   ok = m_analyticsPanel.Create( NULL, "Analytics View", WS_CHILD | WS_CLIPCHILDREN, rect, this, 106 );
   ASSERT ( ok );

   gpInputPanel = &m_inputPanel;
   gpMapPanel = &m_mapPanel;
   gpResultsPanel = &m_resultsPanel;
   gpRtViewsPanel = &m_viewPanel;
   gpAnalyticsPanel = &m_analyticsPanel;
   
   ActiveWnd::SetActiveWnd( &(gpMapPanel->m_mapFrame) );

   // create modeless dialogs
   //m_policyEditor.Create( IDD_POLICYEDITOR, this );   // initially invisible
 
	return 0;
   }


void CEnvView::OnInitialUpdate()
   {
   CView::OnInitialUpdate();
   }


void CEnvView::OnSize(UINT nType, int cx, int cy) 
   {
	CView::OnSize(nType, cx, cy);
	
   // size the embedded views
   if ( m_inputPanel.m_hWnd ) m_inputPanel.MoveWindow( 0, 0, cx, cy, TRUE );

   if ( m_mapPanel.m_hWnd ) m_mapPanel.MoveWindow( 0, 0, cx, cy, TRUE );

   if ( m_dataPanel.m_hWnd ) m_dataPanel.MoveWindow( 0, 0, cx, cy, TRUE );

   if( m_resultsPanel.m_hWnd ) m_resultsPanel.MoveWindow( 0, 0, cx, cy, TRUE );

   if ( m_viewPanel.m_hWnd ) m_viewPanel.MoveWindow( 0, 0, cx, cy, TRUE );

   if ( m_analyticsPanel.m_hWnd ) m_analyticsPanel.MoveWindow( 0, 0, cx, cy, TRUE );
   }


BOOL CEnvView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
   {
/*
   //-- handle the toolbar control beiing pushed --//
   NMHDR *pNotify = (NMHDR*) lParam;

   switch( pNotify->idFrom )
      {
      case 100:   // m_tab control
         {
         if ( pNotify->code == TCN_SELCHANGE )
            {
            int index = TabCtrl_GetCurSel( pNotify->hwndFrom );
            ChangeTabSelection( index );
            }  // end of:  code == TCN_SELCHANGE
         }
      break;
      }
*/
	return CView::OnNotify(wParam, lParam, pResult);
   }


void CEnvView::OnEditCopy() 
   {
   ActiveWnd::OnEditCopy();
   }


void CEnvView::OnEditCopyLegend() 
   {
   ActiveWnd::OnEditCopyLegend();
   }


void CEnvView::OnZoomin() 
   {
   m_mode = MM_ZOOMIN;
   }

void CEnvView::OnZoomout() 
   {
   m_mode = MM_ZOOMOUT;
   }


void CEnvView::OnZoomfull() 
   {
   // check on the active window to see if it is a map
   MapFrame *pMapFrame = ActiveWnd::GetActiveMapFrame();

   if ( pMapFrame != NULL )
      {
      gpMain->SetStatusZoom( 100 );
      pMapFrame->m_pMapWnd->ZoomFull();
      }
   }

void CEnvView::OnZoomToQuery() 
   {
   // check on the active window to see if it is a map
   MapFrame *pMapFrame = ActiveWnd::GetActiveMapFrame();

   if ( pMapFrame != NULL )
      {
      gpMain->SetStatusZoom( 100 );
      MapLayer *pActiveLayer = pMapFrame->m_pMap->GetActiveLayer();
      pMapFrame->m_pMapWnd->ZoomToSelection( pActiveLayer, true );
      }
   }


void CEnvView::SetZoom( int zoomIndex ) 
   {
   // check on the active window to see if it is a map
   MapFrame *pMapFrame = ActiveWnd::GetActiveMapFrame();

   if ( pMapFrame != NULL )
      {
      ZoomInfo *pInfo = GetZoomInfo( zoomIndex );

      //gpMain->SetStatusZoom( 100 );?????
      pMapFrame->m_pMapWnd->ZoomRect(  COORD2d( pInfo->m_xMin, pInfo->m_yMin ), COORD2d( pInfo->m_xMax, pInfo->m_yMax ), true );

      //if ( 
      }
   }




void CEnvView::OnUpdateIsSelection( CCmdUI *pCmdUI )
   {
   if ( gpCellLayer == NULL )
      {
      pCmdUI->Enable( 0 );
      return;
      }

   // map exists, is there anything selected?
   Map *pMap = gpCellLayer->GetMapPtr();
   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );

      if ( pLayer->GetSelectionCount() > 0 )
         {
         pCmdUI->Enable( 1 );
         return;
         }
      }

   pCmdUI->Enable( 0 );
   }


void CEnvView::OnUpdateIsMapWnd(CCmdUI* pCmdUI) 
   {
   pCmdUI->Enable( ActiveWnd::IsMapFrame() ? 1 : 0 );
   }


void CEnvView::OnClearSelection()
   {
   MapWindow *pMapWnd = this->m_mapPanel.m_pMapWnd;
   Map *pMap = this->m_mapPanel.m_pMap;
   if ( pMap == NULL )
      return;

   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      pMap->GetLayer( i )->ClearSelection();

   pMapWnd->RedrawWindow();
   }


void CEnvView::OnSelect() 
   {
   m_mode = MM_SELECT;
   }


void CEnvView::OnQuerydistance() 
   {
   m_mode = MM_QUERYDISTANCE;
   }


void CEnvView::OnPan() 
   {
   m_mode = MM_PAN;
   }


void CEnvView::OnUpdateResultsTileCascade(CCmdUI* pCmdUI) 
   {
   BOOL enable = FALSE;

   if ( m_resultsPanel.IsWindowVisible() && m_resultsPanel.m_pResultsWnd->m_wndArray.GetSize() > 0 )
      enable = TRUE;

   pCmdUI->Enable( enable );	
   }


void CEnvView::OnResultsCascade() 
   {
   if ( m_resultsPanel.IsWindowVisible() )
      m_resultsPanel.m_pResultsWnd->Cascade();
   }


void CEnvView::OnResultsTile() 
   {
   if ( m_resultsPanel.IsWindowVisible() )
      m_resultsPanel.m_pResultsWnd->Tile();
   }

void CEnvView::OnResultsClear() 
   {
   if ( m_resultsPanel.IsWindowVisible() )
      m_resultsPanel.m_pResultsWnd->Clear();
   }

//void CEnvView::OnResultsCapture() 
//   {
//   bool status = m_resultsPanel.m_pResultsWnd->m_createMovie;
//   m_resultsPanel.m_pResultsWnd->m_createMovie = ( status == true ? false : true );
//   }
//
//
//void CEnvView::OnUpdateResultsCapture( CCmdUI *pCmdUI )
//   {
//   if ( m_resultsPanel.m_pResultsWnd )
//      {
//      bool setCheck = ( m_resultsPanel.m_pResultsWnd->m_createMovie ? true : false );
//      pCmdUI->SetCheck( setCheck ? 1 : 0 );
//      }
//   }

void CEnvView::OnResultsDeltas() 
   {
   // toggle checkbox state
   bool status = m_resultsPanel.m_pResultsWnd->m_showDeltas;
   m_resultsPanel.m_pResultsWnd->m_showDeltas = ( status == true ? false : true );


   // redraw windows

   }


void CEnvView::OnUpdateResultsDeltas( CCmdUI *pCmdUI )
   {
   if ( m_resultsPanel.m_pResultsWnd )
      {
      bool setCheck = ( m_resultsPanel.m_pResultsWnd->m_showDeltas ? true : false );
      pCmdUI->SetCheck( setCheck ? 1 : 0 );
      }
   }

void CEnvView::OnResultsSyncMaps() 
   {
   bool status = m_resultsPanel.m_pResultsWnd->m_syncMapZoom;
   m_resultsPanel.m_pResultsWnd->m_syncMapZoom = ( status == true ? false : true );
   }


void CEnvView::OnUpdateResultsSyncMaps( CCmdUI *pCmdUI )
   {
   if ( m_resultsPanel.m_pResultsWnd )
      {
      pCmdUI->Enable( 1 );

      if ( m_resultsPanel.m_pResultsWnd->m_syncMapZoom )
         pCmdUI->SetCheck( 1 );
      else
         pCmdUI->SetCheck( 0 );
      }
   }



void CEnvView::OnUpdateAnalyticsTileCascade(CCmdUI* pCmdUI) 
   {
   BOOL enable = FALSE;

   if ( m_analyticsPanel.IsWindowVisible() && m_analyticsPanel.GetViewerCount() > 0 )
      enable = TRUE;

   pCmdUI->Enable( enable );	
   }


void CEnvView::OnAnalyticsCascade() 
   {
   if ( m_analyticsPanel.IsWindowVisible() )
      m_analyticsPanel.Cascade();
   }


void CEnvView::OnAnalyticsTile() 
   {
   if ( m_analyticsPanel.IsWindowVisible() )
      m_analyticsPanel.Tile();
   }

void CEnvView::OnAnalyticsClear() 
   {
   if ( m_analyticsPanel.IsWindowVisible() )
      m_analyticsPanel.Clear();
   }

void CEnvView::OnAnalyticsCapture() 
   {
   //bool status = m_analyticsPanel.m_pAnalyticsWnd->m_createMovie;
   //m_analyticsPanel.m_pAnalyticsWnd->m_createMovie = ( status == true ? false : true );
   }


void CEnvView::OnUpdateAnalyticsCapture( CCmdUI *pCmdUI )
   {
   //if ( m_analyticsPanel.m_pAnalyticsWnd )
   //   {
   //   bool setCheck = ( m_analyticsPanel.m_pAnalyticsWnd->m_createMovie ? true : false );
   //   pCmdUI->SetCheck( setCheck ? 1 : 0 );
   //   }
   }


void CEnvView::OnAnalyticsAdd() 
   {
   // get current map and delta array
   AnalyticsViewer *pViewer = m_analyticsPanel.AddViewer();

   MapLayer *pLayer = gpModel->m_pIDULayer;
   DeltaArray *pDA  = gpModel->m_pDeltaArray;

   pViewer->Init( pLayer, pDA );
   }


void CEnvView::OnUpdateAnalyticsAdd( CCmdUI *pCmdUI )
   {
   //if ( m_analyticsPanel.m_pAnalyticsWnd )
   //   {
      pCmdUI->Enable( 1 );

   //   if ( m_analyticsPanel.m_pAnalyticsWnd->m_syncMapZoom )
   //      pCmdUI->SetCheck( 1 );
   //   else
   //      pCmdUI->SetCheck( 0 );
   //   }
   }



void CEnvView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
   {
   switch( this->m_activePanel )
      {
      case ANALYTICS_PANEL:
         {
         AnalyticsViewer *pWnd = this->m_analyticsPanel.GetActiveView();
         
         if ( pWnd )
            pWnd->OnKeyDown( nChar, nRepCnt, nFlags );
         break;
         }
      }

   char lsChar = char(nChar);
   WORD ctrl = HIWORD( (DWORD) GetKeyState( VK_CONTROL ) );

   // CTL + Page Down
   if ( ctrl && nChar == VK_NEXT )
      {
      //ChangeTabSelection( m_tabIndex + 1 );
      }
   
   // CTL + Page Up
   if ( ctrl && nChar == VK_PRIOR )
      {
      //ChangeTabSelection( m_tabIndex - 1 );
      }

   // toggle output window
   if ( ctrl && ( lsChar == 'o' || lsChar == 'O' ) ) 
      gpMain->OnViewOutput();

   ///////////////////////////
   // this is to debug whatever
   if ( ctrl && lsChar == 'R' )
      {
      ///RandLogNormal r(10, 5, 1);
      ///
      ///FILE *fp = fopen("d:/randtest.csv", "w");
      ///
      ///for (int i = 0; i < 10000; i++)
      ///   {
      ///   double v = r.RandValue();
      ///   fprintf(fp, "%f\n", (float)v);
      ///   }
      ///
      ///fclose(fp);


      //MapLayer *pPtLayer = gpCellLayer->GetMapPtr()->GetLayer("2002 Dune Toe Line");
      //MapLayer *pCellLayer = gpCellLayer->GetMapPtr()->GetLayer("CellType");
      //int btCol = pPtLayer->GetFieldCol("BEACHTYPE");
      //
      //for (int i = 0; i < pPtLayer->GetRecordCount(); i++)
      //   {
      //   int beachType = 0;
      //   pPtLayer->GetData(i, btCol, beachType);
      //
      //   if (beachType != 9)
      //      {
      //      REAL x, y;
      //      pPtLayer->GetPointCoords(i, x, y);
      //
      //      // get grid row from celltype for this dunept
      //      int row, col;
      //      pCellLayer->GetGridCellFromCoord(x, y, row, col);
      //
      //      for (int j = 0; j < pCellLayer->GetColCount(); j++)
      //         {
      //         int cellType = 0;
      //         pCellLayer->GetData(row, j, cellType);
      //
      //         if (cellType >= 0)
      //            {
      //            pCellLayer->SetData(row, j, 1);  // type=CT_BEACH
      //            break;
      //            }
      //         }
      //      }
      //   }   
      //
      //CString path = "\\Envision\\StudyAreas\\GraysHarbor\\CellType3.asc";
      //bool ok = pCellLayer->SaveGridFile(path);  
      //
      //CString msg;
      //msg.Format("%i:   Grid - rows=%i, cols=%i", ok?1:0, pCellLayer->GetRowCount(), pCellLayer->GetColCount());
      //Report::Log(msg);
      }

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
   }


void CEnvView::OnViewInput()
   {
   ActivatePanel( 0 );
   }

void CEnvView::OnViewMap()
   {
   ActivatePanel( 1 );
   }

void CEnvView::OnViewTable()
   {
   ActivatePanel( 2 );
   }

void CEnvView::OnViewRtViews()
   {
   ActivatePanel( 3 );
   }

void CEnvView::OnViewResults()
   {
   ActivatePanel( 4 );
   }

void CEnvView::OnViewAnalytics()
   {
   ActivatePanel( 5 );
   }

void CEnvView::ActivatePanel( int index )
   {
   //gpMain->SetRibbonContextCategory( 0 );
   if ( m_activePanel != index )
      {
      gpMain->SetRibbonContextCategory( 0 );

      m_inputPanel.ShowWindow    ( index==INPUT_PANEL     ? SW_SHOW : SW_HIDE );
      m_mapPanel.ShowWindow      ( index==MAP_PANEL       ? SW_SHOW : SW_HIDE );
      m_dataPanel.ShowWindow     ( index==DATA_PANEL      ? SW_SHOW : SW_HIDE );
      m_viewPanel.ShowWindow     ( index==RTVIEW_PANEL    ? SW_SHOW : SW_HIDE );
      m_resultsPanel.ShowWindow  ( index==RESULTS_PANEL   ? SW_SHOW : SW_HIDE );
      m_analyticsPanel.ShowWindow( index==ANALYTICS_PANEL ? SW_SHOW : SW_HIDE );
      m_activePanel = (PANEL_TYPE) index;
      //
      switch( index )
         {
         case INPUT_PANEL:     // input panel
            gpInputPanel->RedrawWindow();
            ActiveWnd::SetActiveWnd( gpInputPanel );
            break;

         case MAP_PANEL:     // map panel
            ActiveWnd::SetActiveWnd( &gpMapPanel->m_mapFrame );
            break;

         case DATA_PANEL:     // data panel
            {
            DataGrid *pGrid = &(m_dataPanel.m_grid);
            ActiveWnd::SetActiveWnd( pGrid );
            }
            break;

         case RTVIEW_PANEL:     // 
            ActiveWnd::SetActiveWnd( m_viewPanel.GetActiveView() );
            gpMain->SetRibbonContextCategory( IDCC_RTVIEWS );
            break;

         case RESULTS_PANEL:  // results
            ActiveWnd::SetActiveWnd( m_resultsPanel.GetActiveView() );
            gpMain->SetRibbonContextCategory( IDCC_RESULTS );
            break;

         case ANALYTICS_PANEL:  // analytics
            ActiveWnd::SetActiveWnd( m_analyticsPanel.GetActiveView() );
            gpMain->SetRibbonContextCategory( IDCC_ANALYTICS );
            break;
         }
      }
   }


void CEnvView::OnViewPolygonedges() 
   {
   MapFrame *pMapFrame = ActiveWnd::GetActiveMapFrame();

   if ( pMapFrame == NULL )
      {
      MessageBeep( 0 );
      return;
      }

   MapLayer *pLayer = pMapFrame->m_pMap->GetActiveLayer();
   if ( pLayer == NULL )
      return;

   long edge = pLayer->GetOutlineColor();

   if ( edge == NO_OUTLINE )
      pLayer->SetOutlineColor( RGB( 127, 127, 127 ) );
   else
      pLayer->SetNoOutline();

   pMapFrame->m_pMapWnd->RedrawWindow();
   }


void CEnvView::OnUpdateViewPolygonedges(CCmdUI* pCmdUI) 
   {
   MapFrame *pMapFrame = ActiveWnd::GetActiveMapFrame();

   if ( pMapFrame == NULL || pMapFrame->m_pMap->GetActiveLayer() == NULL )
      {
      pCmdUI->Enable( FALSE );
      return;
      }

   MapLayer *pLayer = pMapFrame->m_pMap->GetActiveLayer();
   
   if ( pLayer->GetOutlineColor() == NO_OUTLINE )
      {
      pCmdUI->Enable();
      pCmdUI->SetCheck( 0 );
      }
   else
      {
      pCmdUI->Enable();
      pCmdUI->SetCheck( 1 );
      }	
   }


void CEnvView::OnViewShowMapScale() 
   {
   if ( gpMapPanel == NULL || gpMapPanel->m_pMapWnd == NULL )
      return;

   gpMapPanel->m_pMapWnd->ShowScale( gpMapPanel->m_pMapWnd->IsScaleShowing() ? false : true );
   gpMapPanel->m_pMapWnd->RedrawWindow();
   }


void CEnvView::OnUpdateViewShowMapScale(CCmdUI* pCmdUI) 
   {
   if ( gpMapPanel == NULL || gpMapPanel->m_pMapWnd == NULL )
      {
      pCmdUI->Enable( 0 );
      return;
      }
   
   pCmdUI->Enable();
   pCmdUI->SetCheck( gpMapPanel->m_pMapWnd->IsScaleShowing() ? 1 : 0 );
   }


void CEnvView::OnViewShowAttribute() 
   {
   if ( gpMapPanel == NULL || gpMapPanel->m_pMapWnd == NULL )
      return;

   gpMapPanel->m_pMapWnd->ShowAttributeOnHover( gpMapPanel->m_pMapWnd->IsShowAttributeOnHover() ? false : true );

   // set for results maps as well?

   }


void CEnvView::OnUpdateViewShowAttribute(CCmdUI* pCmdUI) 
   {
   if ( gpMapPanel == NULL || gpMapPanel->m_pMapWnd == NULL )
      {
      pCmdUI->Enable( 0 );
      return;
      }
   
   pCmdUI->Enable();
   pCmdUI->SetCheck( gpMapPanel->m_pMapWnd->IsShowAttributeOnHover() ? 1 : 0 );
   }



void CEnvView::OnAnalysisShowactorterritory() 
   {
   SelectActor dlg;
   if ( dlg.DoModal() == IDOK )
      {
      int index = dlg.m_index;
      Actor *pActor = gpActorManager->GetActor( index );

      gpCellLayer->ClearSelection();

      for ( int i=0; i < pActor->GetPolyCount(); i++ )
         {
         ASSERT( ! gpCellLayer->IsDefunct( pActor->GetPoly( i ) ) );
         gpCellLayer->AddSelection( pActor->GetPoly( i ) );
         }

      gpMapPanel->RedrawWindow();
      }	
   }

void CEnvView::OnAnalysisShowactors() 
   {
   ActorDlg dlg;
   dlg.DoModal();	
   }


BOOL CEnvView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
   {
	// TODO: Add your message handler code here and/or call default
   // gpMap3d->OnMouseWheel( nFlags, zDelta, pt );
	
	return CView::OnMouseWheel(nFlags, zDelta, pt);
   }


void CEnvView::OnUpdateData(CCmdUI* pCmdUI) 
   {
	switch ( pCmdUI->m_nID )
	   {
      case ID_FILE_RESET:
         pCmdUI->Enable( /*gpDoc->m_currentRun > 0 ? TRUE :*/ FALSE );
         break;
      case ID_DATA_EXPORTDATA:
         pCmdUI->Enable( gpDoc->m_model.m_currentRun > 0 ? TRUE : FALSE );
         break;
	   default:
         pCmdUI->Enable( gpCellLayer != 0 ? TRUE : FALSE );
         break;
	   }
   }


void CEnvView::OnEditPolicies()
   {
   if ( m_policyEditor.GetSafeHwnd() == 0 )
      return;
   else
      {
      if ( m_policyEditor.IsWindowVisible() )
         m_policyEditor.ShowWindow( SW_HIDE );
      else
         {
         m_policyEditor.ShowWindow( SW_SHOW );

         if ( gpPolicyManager->GetPolicyCount() == 0 )
            m_policyEditor.AddNew();
         }
      }
   }


void CEnvView::OnEditActors()
   {
   ActorDlg dlg;
   dlg.DoModal();
   }


void CEnvView::OnEditLulc()
   {
   LulcEditor dlg;
   dlg.DoModal();
   }

void CEnvView::OnDataLulcInfo()
   {
	//LulcInfoDialog dlg;
	//dlg.DoModal();

   if ( !IsWindow( m_lulcInfoDialog.m_hWnd ) )
      {
      m_lulcInfoDialog.Create( this );
      m_lulcInfoDialog.ShowWindow( SW_SHOW );
      }
   else
      m_lulcInfoDialog.SetFocus();
   }


void CEnvView::OnDataIduInfo()
   {
	DatabaseInfoDlg dlg( gpCellLayer );
   dlg.DoModal();   
   }


void CEnvView::OnAnalysisSetupRun()
   {
   WizRun wizRun( "Setup Run", this );
   wizRun.DoModal();

   // create a new run and run
   if ( wizRun.m_runState == 1 ) // do a single run
      {
      //if ( gpModel->m_currentRun == 0 
      //if ( gpModel->m_currentYear > 0 )
      //   m_outputTab.m_pViewTab->NewRun();

      ///???V5 m_outputTab.m_pViewTab->OnAnalysisRun();
      }

   else if ( wizRun.m_runState == 2 )  // do a multirun
      gpDoc->OnAnalysisRunmultiple();
   }



//void CEnvView::OnVideoRecorders()
//   {
//   VideoRecorderDlg dlg;
//   dlg.DoModal();
//   }


void CEnvView::OnSaveShapefile()
   {
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( FALSE, "shp", gpCellLayer->m_path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "Shape Files|*.shp|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      int count = gpCellLayer->SaveShapeFile( filename, FALSE );

      if ( count <= 0 )
         MessageBox( "Unable to save file", "File Error" );
      }

   _chdir( cwd );
   free( cwd );
   }


void CEnvView::OnUpdateSaveShapefile(CCmdUI *pCmdUI)
   {
   if ( gpCellLayer )
      pCmdUI->Enable( TRUE );
   else
      pCmdUI->Enable( FALSE );
   }

/*
void CEnvView::OnConnectToHost()
   {
   CPropertySheet sheet( "Interaction Session", this, 1 );
   PPClient clientPage;
   PPHost   hostPage;
   sheet.AddPage( &hostPage );
   sheet.AddPage( &clientPage );
   sheet.DoModal();
   }

void CEnvView::OnUpdateConnectToHost(CCmdUI *pCmdUI)
   {
   }

void CEnvView::OnHostSession()
   {
   CPropertySheet sheet( "Interaction Session", this, 0 );
   PPClient clientPage;
   PPHost   hostPage;
   sheet.AddPage( &hostPage );
   sheet.AddPage( &clientPage );
   sheet.DoModal();
   }

void CEnvView::OnUpdateHostSession(CCmdUI *pCmdUI)
   {
   }
*/

void CEnvView::OnAnalysisShowdeltalist()
   {
   DeltaViewer viewer( this );
   viewer.DoModal();
   }

void CEnvView::OnUpdateAnalysisShowdeltalist(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable( gpModel->m_pDataManager->GetDeltaArrayCount() > 0 ? TRUE : FALSE );
   }

void CEnvView::OnAnalysisSetuppolicymetaprocess()
   {
   //gpDoc->m_model.m_policyMetaProcess.Setup(this);
   }

void CEnvView::OnUpdateAnalysisSetuppolicymetaprocess(CCmdUI *pCmdUI)
   {
   if ( gpCellLayer == NULL )
      pCmdUI->Enable( FALSE );
   else
      pCmdUI->Enable( TRUE );
   }


void CEnvView::OnDataStorerundata()
   {
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( FALSE, "edd", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "EnvDataDump Files|*.edd|All files|*.*||" );

   if ( dlg.DoModal() == IDOK )
      {
      CString fileName = dlg.GetPathName();
      if ( ! gpModel->m_pDataManager->SaveRun( fileName ) )
         Report::ErrorMsg( "Error Saving Run." );
      }   

   _chdir( cwd );
   free( cwd );
   }

void CEnvView::OnDataLoadrundata()
   {
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( TRUE, "edd", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "EnvDataDump Files|*.edd|All files|*.*||" );

   if ( dlg.DoModal() == IDOK )
      {
      CWaitCursor c;
      if ( ! gpModel->m_pDataManager->LoadRun( dlg.GetPathName() ) )
         {
         MessageBox( "Error Loading Run.", 0, MB_ICONERROR | MB_OK );
         }
      }

   _chdir( cwd );
   free( cwd );
   }

void CEnvView::OnRunquery()
   {
   MapLayer *pLayer = m_mapPanel.m_pMap->GetActiveLayer();
   QueryBuilder dlg(gpModel->m_pQueryEngine, m_mapPanel.m_pMapWnd, NULL, -1, this );

   this->m_mapPanel.m_pMapWnd->RedrawWindow();
   dlg.DoModal();
   }



void CEnvView::OnUpdateEditCopy(CCmdUI *pCmdUI)
   {
   ActiveWnd::OnUpdateEditCopy( pCmdUI );
   }


void CEnvView::OnFilePrint()
   {
   ActiveWnd::OnFilePrint();
   }


void CEnvView::OnUpdateFilePrint(CCmdUI *pCmdUI)
   {
   ActiveWnd::OnUpdateFilePrint( pCmdUI );
   }


void CEnvView::OnClose()
   {
   //this->m_outputTab.m_scenarioTab.OnBnClickedOk();      // check to see is scenarios need to be saved   
   CView::OnClose();
   }

void CEnvView::OnEditSetpolicycolors()
   {
   PolicyColorDlg dlg;
   dlg.DoModal();
   }


void CEnvView::OnEditScenarios()
   {
   ScenarioEditor dlg;
   dlg.DoModal();
   }




void CEnvView::OnExportCoverages()
   {
   gpDoc->m_model.m_exportMaps = ! gpDoc->m_model.m_exportMaps; //gpMain->m_pExportCoverages->IsChecked() ? true : false;
   }


void CEnvView::OnUpdateExportCoverages(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable();
   pCmdUI->SetCheck( gpDoc->m_model.m_exportMaps ? 1 : 0 );
   }


void CEnvView::OnUpdateExportInterval(CCmdUI *pCmdUI)
   {
   //TCHAR buffer[ 32 ];
   //_itoa_s( gpDoc->m_model.m_exportMapInterval, buffer, 32, 10 );
   //gpMain->SetExportInterval( gpDoc->m_model.m_exportMapInterval ); ////pCmdUI->SetText( buffer );

   pCmdUI->Enable( gpDoc->m_model.m_exportMaps ? 1 : 0 );
   }


void CEnvView::OnExportOutputs()
   {
   gpDoc->m_model.m_exportOutputs = ! gpDoc->m_model.m_exportOutputs;
   }


void CEnvView::OnUpdateExportOutputs(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable();
   pCmdUI->SetCheck( gpDoc->m_model.m_exportOutputs ? 1 : 0 );
   }

void CEnvView::OnExportDeltas()
   {
   gpDoc->m_model.m_exportDeltas = ! gpDoc->m_model.m_exportDeltas;
   }


void CEnvView::OnUpdateExportDeltas(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable();
   pCmdUI->SetCheck( gpDoc->m_model.m_exportDeltas ? 1 : 0 );
   }

void CEnvView::OnExportDeltaFields()
   {
   ;  //gpDoc->m_model.m_exportDeltas = ! gpDoc->m_model.m_exportDeltas;
   }


void CEnvView::OnUpdateExportDeltaFields(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable( gpDoc->m_model.m_exportDeltas ? 1 : 0 ); 
   }


void CEnvView::OnUpdateStartYear(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable(); 
   }

void CEnvView::OnUpdateStopTime(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable(); 
   }




void CEnvView::OnZoomSlider()
   {
   float zoomPct = gpMain->GetZoomSliderPos()/100.0f;

   MapFrame *pMapFrame = ActiveWnd::GetActiveMapFrame();

   if ( pMapFrame && pMapFrame->m_pMapWnd )
      {
      // a larger zoomFactor zooms in, a smaller zoomFactor zooms out.
      // a zoom factor of 1.0 (actually, 0.9) zoom full
      // for a map, the "most zoomed" state is a zoom factor of 100, corresponding to
      // a zoomPct of 1.0.
      // the "least zoomed" state is full zoom, meaning a zoomFactor of 0.9, correspinding
      // to a zoomPct of 0;
      double r = log( 50.0-0.9 );
      float zoomFactor = float( 0.9 + ( exp( r * zoomPct )-1 ) );
      pMapFrame->m_pMapWnd->SetZoomFactor( zoomFactor, true );
      }
   }


void CEnvView::OnZoomButton()
   {
   
   }



void CEnvView::RandomizeLulcLevel( int level /*one-based*/)
   {
   // level 0 = root  (illegal)
   // level 1 = LULC_A  (illegal)
   // level 2 = LULC_B  
   // ...etc..

   LulcTree &lulcTree = gpModel->m_lulcTree;

   int levels = lulcTree.GetLevels();  // doesn't include root level
   ASSERT( level <= levels );
   ASSERT( level > 1 );

   CString &parentField = lulcTree.GetFieldName( level-1 );
   MAP_FIELD_INFO *pInfoParent = gpCellLayer->FindFieldInfo( parentField );
   if ( pInfoParent == NULL )
      return;

   CString &childField = lulcTree.GetFieldName( level );
   MAP_FIELD_INFO *pInfoChild = gpCellLayer->FindFieldInfo( childField );
   if ( pInfoChild == NULL )
      return;

   LulcNode *pNode = lulcTree.GetRootNode();
   
   RandUniform r;

   while( (pNode = lulcTree.GetNextNode()) != NULL )
      {
      if ( pNode->GetNodeLevel() == level )
         {
         FIELD_ATTR *pAttr = pInfoChild->FindAttribute( pNode->m_id );
         if ( pAttr == NULL )
            continue;

         // this is a node we want to get child from.  First, find the color of this node
         LulcNode *pParentNode = pNode->m_pParentNode;
         FIELD_ATTR *pParentAttr = pInfoParent->FindAttribute( pParentNode->m_id );
         if ( pParentAttr == NULL )
            continue;

         COLORREF color = pParentAttr->color;
         int red = GetRValue( color );
         int grn = GetGValue( color );
         int blu = GetBValue( color );

         red += (int) r.RandValue( -25, 25 );
         grn += (int) r.RandValue( -25, 25 );
         blu += (int) r.RandValue( -25, 25 );
         
         pAttr->color = RGB( red, grn, blu );
         }
      }
   }


BOOL CEnvView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
   {
   if ( CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) )
      return TRUE;

   // add rtview panel to command handlers
   return this->m_viewPanel.OnCmdMsg( nID, nCode, pExtra, pHandlerInfo );
   }

/*
bool CEnvView::AddStandardRecorders( void )
   {
   nsPath::CPath path( gpDoc->m_iniFile );
   nsPath::CPath fullPath = path.MakeFullPath();

   fullPath.RemoveFileSpec();
   fullPath.Append( "\\Outputs\\AVIs" );
   SHCreateDirectoryEx( NULL, fullPath, NULL );

   // main window
   CString _path = fullPath + "\\MainWnd.avi";
   VideoRecorder *pVR = new VideoRecorder( _path, gpMain, 30 );
   pVR->m_enabled = false;
   pVR->m_useApplication = true;
   pVR->m_name = "Main Envision Window";
   AddVideoRecorder( pVR );

   // map panel
   _path = CString( fullPath  ) + "\\MapPanel.avi";
   pVR = new VideoRecorder( _path, &m_mapPanel, 30 );
   pVR->m_enabled = false;
   pVR->m_useApplication = true;
   pVR->m_name = "Map Panel";
   AddVideoRecorder( pVR );

   // results panel
   _path = CString( fullPath ) + "\\ResultsPanel.avi";
   pVR = new VideoRecorder( _path, m_resultsPanel.m_pResultsWnd, 30 );
   pVR->m_enabled = false;
   pVR->m_useApplication = true;
   pVR->m_name = "Results Panel";
   AddVideoRecorder( pVR );

   // rtviews panel
   _path = CString( fullPath ) + "\\RtViewsPanel.avi";
   pVR = new VideoRecorder( _path, &m_viewPanel, 30 );
   pVR->m_enabled = false;
   pVR->m_useApplication = true;
   pVR->m_name = "Runtime Views Panel";
   AddVideoRecorder( pVR );

   // input panel
   _path = (CString) fullPath + "\\InputPanel.avi";
   pVR = new VideoRecorder( _path, &m_inputPanel, 30 );
   pVR->m_enabled = false;
   pVR->m_useApplication = true;
   pVR->m_name = "Input Panel";
   AddVideoRecorder( pVR );

   // main map (no legend tree)
   _path = (CString) fullPath + "\\ResultsPanel.avi";
   pVR = new VideoRecorder( _path, m_mapPanel.m_pMapWnd, 30 );
   pVR->m_enabled = false;
   pVR->m_useApplication = true;
   pVR->m_name = "Main Map";
   AddVideoRecorder( pVR );

   return true;
   }



void CEnvView::StartVideoRecorders( void )
   {
   for ( int i=0; i < GetVideoRecorderCount(); i++ )
      {
      VideoRecorder *pVR = GetVideoRecorder( i );

      ASSERT( pVR != NULL );

      if ( pVR->m_enabled && pVR->m_useApplication )
         {
         pVR->StartCapture();
         }
      }
   }


void CEnvView::UpdateVideoRecorders( void )
   {
   for ( int i=0; i < GetVideoRecorderCount(); i++ )
      {
      VideoRecorder *pVR = GetVideoRecorder( i );

      ASSERT( pVR != NULL );

      if ( pVR->m_enabled && pVR->m_useApplication )
         {
         pVR->CaptureFrame();
         }
      }
   }

void CEnvView::StopVideoRecorders( void )
   {
   for ( int i=0; i < GetVideoRecorderCount(); i++ )
      {
      VideoRecorder *pVR = GetVideoRecorder( i );

      ASSERT( pVR != NULL );

      if ( pVR->m_enabled && pVR->m_useApplication )
         {
         pVR->EndCapture();
         }
      }
   }
   */

void CEnvView::UpdateUI( int flags, INT_PTR extra )
   {
   if ( ( gpModel->m_dynamicUpdate & flags ) == 0 )
      return;

   //if ( m_inMultiRun )
   //   return;

   if ( flags & 2 )  // update map.   TODO: Implement dynamic update for multi-run case
      {
      m_mapPanel.m_mapFrame.Update();
      //UpdateVideoRecorders();
      }

   if ( flags & 4 ) // show year
      {
      CString msg;

      if ( gpModel->m_inMultiRun )         
         {
         int currentRun = gpModel->m_currentRun % gpModel->m_iterationsToRun;
         msg.Format( "%i%i:%i", gpModel->m_currentMultiRun, currentRun, gpModel->m_currentYear );
         }
      else
         msg.Format( "%i", gpModel->m_currentYear );

      m_mapPanel.UpdateText( msg );
      }

   if ( flags & 8 )    // process - extra pts to EnvExtension
      {
      if ( extra == 0 )
         m_mapPanel.UpdateSubText( "Actor Decision-making" );
      else
         {
         EnvExtension *pExt = (EnvExtension*) extra;
         m_mapPanel.UpdateSubText( pExt->m_name );
         }
      }

   if ( flags & 16 )  // update model msg
      {
      LPCTSTR name = (LPCTSTR) extra;
      gpMain->SetModelMsg( name );
      }
   }


void CEnvView::EndRun()
   {
   //ENV_ASSERT( m_pIDULayer->GetActiveField() >= 0  && gpMapPanel != NULL);
   m_mapPanel.m_mapFrame.OnSelectField( m_mapPanel.m_pMap->GetActiveLayer()->GetActiveField() );

   int vizCount = m_vizManager.GetVisualizerCount();
    
   for ( int i=0; i < vizCount; i++ )
      {
      VisualizerWnd *pVizWnd = m_vizManager.GetVisualizerWnd( i );
   
      EnvVisualizer *pInfo = pVizWnd->m_pVizInfo;
   
      if ( pInfo->m_use )
         {
         // call the function (eval models should return scores in the range of -3 to +3)
         if ( gpModel->m_showRunProgress )
            gpMain->SetModelMsg( pInfo->m_name );
   
         gpModel->m_envContext.pEnvExtension  = pInfo;
         gpModel->m_envContext.id     = pInfo->m_id;
         gpModel->m_envContext.handle = pInfo->m_handle;
         gpModel->m_envContext.col    = -1;
         gpModel->m_envContext.firstUnseenDelta = gpModel->GetFirstUnseenDeltaViz( i );
         gpModel->m_envContext.lastUnseenDelta = gpModel->m_envContext.pDeltaArray->GetSize();
   
         gpModel->m_envContext.pWnd   = pVizWnd;
   
         clock_t start = clock();
   
         BOOL ok = pInfo->EndRun( &gpModel->m_envContext );
   
         clock_t finish = clock();
         double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
         //pInfo->m_runTime += (float) duration;         
   
         if ( gpModel->m_showRunProgress )
            {
            CString msg( pInfo->m_name );
            msg += " - Completed";
            gpMain->SetModelMsg( msg );
            }
   
         if ( ! ok )
            {
            CString msg = "The ";
            msg += pInfo->m_name;
            msg += " Visualizer returned FALSE indicating an error.";
            throw new EnvRuntimeException( msg );
            }
         }
      }  // end of:  for( i < vizCount )

   //StopVideoRecorders();   // generates AVI files
   return;
   }
