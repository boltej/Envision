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
// RtViewPanel.cpp : implementation file
//

#include "stdafx.h"
#pragma hdrstop

#include "Envision.h"
#include "RtViewPanel.h"

#include "envdoc.h"
#include "envview.h"
#include "ActiveWnd.h"
//#include "resource.h"
#include <Scenario.h>
//#include "areaquerydlg.h"

#include <maplayer.h>
#include <misc.h>

//#include "VPMapWnd.h"
//#include "VPPopulationScatter.h"
//#include "VPScatterWnds.h"
//#include "VPEvalRadar.h"

extern CEnvDoc  *gpDoc;
extern CEnvView *gpView;
extern MapLayer *gpCellLayer;
extern EnvModel *gpModel;

int RtViewPanel::m_nextID = 90000;

////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------
//------------- R T V I E W P A G E --------------------------------------- 
//-------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(RtViewPage, CWnd)

BEGIN_MESSAGE_MAP(RtViewPage, CWnd)
   ///ON_WM_SIZE()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL RtViewPage::OnEraseBkgnd(CDC* pDC)
   {
   RECT rect;
   GetClientRect( &rect );
   pDC->SelectStockObject( WHITE_BRUSH );
   pDC->Rectangle( &rect );
   return TRUE;
   }


// add a window of the specified type to the ACTIVE tab.
RtViewWnd *RtViewPage::AddView( RTV_TYPE rtvType, INT_PTR extra, int run, CRect &viewRect )
   {
   RtViewWnd *pView = NULL;

   RECT rect;
   GetClientRect( &rect );

   switch( rtvType )
      {
      case RTVT_MAP:  // built-in types - for maps, only show those with field info defined
         {
         int col = extra % 200;
         int layer = int( extra / 200 );

         // create the view
         RtMap *pRtMap = new RtMap( this, run, viewRect, layer, col );

         rect.left   = rect.right  * pRtMap->m_xLeft   / 100;
         rect.right  = rect.right  * pRtMap->m_xRight  / 100 ;
         rect.top    = rect.bottom * pRtMap->m_yTop    / 100;
         rect.bottom = rect.bottom * pRtMap->m_yBottom / 100;

         CString name;
         pRtMap->Create( NULL, name, WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, rect, this, RtViewPanel::m_nextID++ );
         pView = pRtMap;
         }
         break;

      case RTVT_SCATTER:   
      case RTVT_RADAR:     
      case RTVT_TABLE:     
      case RTVT_VIEW:
         // not implemented at this time
         return NULL;

      case RTVT_VISUALIZER: // custom types
         {
         EnvVisualizer *pVizInfo = (EnvVisualizer*) extra;  
         ASSERT( pVizInfo != NULL );

         // create the view
         RtVisualizer *pRtVisualizer = new RtVisualizer( this, run, pVizInfo, viewRect );
         pRtVisualizer->Create( NULL, pVizInfo->m_name, CS_HREDRAW | CS_VREDRAW | WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, rect, this, RtViewPanel::m_nextID++ );
		 
         // Note: No InitWindow needed for RtViz

         // create visualizer window (child of the RtViewWnd)
         BOOL success;
         VisualizerWnd *pVizWnd = gpView->m_vizManager.CreateWnd( pRtVisualizer, pVizInfo, run, success,VT_RUN );
         pVizWnd->InitWindow();

         pRtVisualizer->m_pVisualizerWnd = pVizWnd;

         pView = pRtVisualizer;
         break;
         }
      }

   if ( pView != NULL )
      {
      m_viewWndArray.Add( pView );

      pView->InitWindow();
      m_pActiveWnd = pView;
      }

   return pView;
   }






///////////////////////////////////////////////////////////////////////////////////////////
// RtViewPanel

IMPLEMENT_DYNAMIC(RtViewPanel, CWnd)

RtViewPanel::RtViewPanel()
: m_state( PS_PAUSED )
, m_currentYear( -1 )
, m_activeRun( -1 )
, m_activePage( -1 )
//, m_totalYearsToRun( 0 )
, m_timerID( -1 )
//, m_pActiveWnd( NULL )
  { }


RtViewPanel::~RtViewPanel()
   {
   //for ( int i=0; i < this->m_viewTabArray.GetSize(); i++ )
   //   delete m_viewTabArray[ i ];

   //m_viewTabArray.RemoveAll();
   }


BEGIN_MESSAGE_MAP(RtViewPanel, CWnd)
   ON_WM_CREATE()
   ON_UPDATE_COMMAND_UI_RANGE( ID_RTMAP_BEGIN,   ID_RTMAP_END,   &RtViewPanel::OnUpdateMap )
   ON_UPDATE_COMMAND_UI_RANGE( ID_RTGRAPH_BEGIN, ID_RTGRAPH_END, &RtViewPanel::OnUpdateGraph )
   ON_UPDATE_COMMAND_UI_RANGE( ID_RTVIZ_BEGIN,   ID_RTVIZ_END,   &RtViewPanel::OnUpdateViz )

   ON_UPDATE_COMMAND_UI(ID_RTVIEW_CASCADE,      &RtViewPanel::OnUpdateTileCascade)
	ON_UPDATE_COMMAND_UI(ID_RTVIEW_TILE,         &RtViewPanel::OnUpdateTileCascade)
   ON_UPDATE_COMMAND_UI(ID_RTVIEW_MAPS,         &RtViewPanel::OnUpdateMap)
   ON_UPDATE_COMMAND_UI(ID_RTVIEW_GRAPHS,       &RtViewPanel::OnUpdateGraph)
   ON_UPDATE_COMMAND_UI(ID_RTVIEW_VISUALIZERS,  &RtViewPanel::OnUpdateViz)
	ON_UPDATE_COMMAND_UI(ID_RTVIEW_REWIND,       &RtViewPanel::OnUpdateRewind)
   ON_UPDATE_COMMAND_UI(ID_RTVIEW_START,        &RtViewPanel::OnUpdateStart)
   ON_UPDATE_COMMAND_UI(ID_RTVIEW_PAUSE,        &RtViewPanel::OnUpdatePause)  // pause

   ON_COMMAND_RANGE( ID_RTMAP_BEGIN,   ID_RTMAP_END,   &RtViewPanel::OnCommandMap )
   ON_COMMAND_RANGE( ID_RTGRAPH_BEGIN, ID_RTGRAPH_END, &RtViewPanel::OnCommandGraph )
   ON_COMMAND_RANGE( ID_RTVIZ_BEGIN,   ID_RTVIZ_END,   &RtViewPanel::OnCommandViz )

 	ON_COMMAND(ID_RTVIEW_CASCADE,           &RtViewPanel::OnCascade)
	ON_COMMAND(ID_RTVIEW_TILE,              &RtViewPanel::OnTile)
	ON_COMMAND(ID_RTVIEW_REWIND,            &RtViewPanel::OnRewind)
	ON_COMMAND(ID_RTVIEW_START,             &RtViewPanel::OnStart)
	ON_COMMAND(ID_RTVIEW_PAUSE,             &RtViewPanel::OnPause)

   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
   ON_WM_TIMER()
END_MESSAGE_MAP()


// RtViewPanel message handlers

int RtViewPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_font.CreatePointFont( 80, "Arial" );
   CDC *pDC = GetDC();
   CGdiObject *pOld = pDC->SelectObject( &m_font );
   TEXTMETRIC tm;
   pDC->GetTextMetrics( &tm );
   int height = tm.tmHeight * 4 / 3;
   pDC->SelectObject( pOld );
   ReleaseDC( pDC );

   CRect rect;
   GetViewRect( &rect );
   rect.top = rect.bottom - height;

   BOOL ok = m_tabCtrl.Create( WS_CHILD | WS_VISIBLE | CTCS_AUTOHIDEBUTTONS, rect, this, TABS );
   ASSERT( ok );

   LOGFONT logFont;
   m_font.GetLogFont( &logFont );
   m_tabCtrl.SetControlFont( logFont );

   //m_tab.Create( NULL, "RtViewTab", WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPCHILDREN, rect, this, 1001 );

   int setupLeft = rect.left;
   int gutter = rect.top;
   rect.left = gutter;
   rect.top = rect.CenterPoint().y - 12;
   rect.right = rect.left + 24;
   rect.bottom = rect.top + 24;

   //AddRun( 0 ); //gpDoc->m_model.m_currentRun );
   return 0;
   }


RtViewPage *RtViewPanel::AddPage( LPCTSTR name )
   {
   RtViewPage *pPage = new RtViewPage;

   m_pageArray.Add( pPage );

   pPage->SetRun( m_activeRun );

   CRect rect;
   this->GetViewRect( &rect );
   int index = this->GetPageCount()-1;
   pPage->Create( NULL, name, WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 1000+index );

   m_tabCtrl.InsertItem( index, name, (LPARAM) pPage );
   
   if ( index == 0 ) // first page?  Then make it active
      this->m_activePage = 0;

   return pPage;
   }


void RtViewPanel::AddRun( int run )
   {
   // note - does NOT make this run active, or change the current display!
   ASSERT( run == gpDoc->m_model.m_currentRun );
   
   int tab = m_tabCtrl.GetItemCount();

   CString label;
   label.Format( "Run %i: ", run );
   
   Scenario *pScenario = gpModel->GetScenario();

   if ( pScenario == NULL )
      label += "No Scenario";
   else
      label += pScenario->m_name;

   m_tabCtrl.InsertItem( tab, label );
   //m_tabCtrl.SetCurSel( tab );
   //ChangeTabSelection( tab );

   RtViewRunInfo *pInfo = new RtViewRunInfo;
   pInfo->m_run = run;
   pInfo->m_pScenario = gpModel->GetScenario();
   pInfo->m_pDeltaArray = NULL;   // gpModel->m_pDeltaArray;
   
   m_runInfoArray.Add( pInfo );

   SetRun( run );
   }


void RtViewPanel::SetYear( int year )
   {
   m_currentYear = year;

   RtViewPage *pPage = GetActivePage();

   if ( pPage != NULL )
      {
      for ( int i=0; i < (int) pPage->m_viewWndArray.GetSize(); i++ )
         {
         RtViewWnd *pView = pPage->m_viewWndArray[ i ];
         pView->SetYear( year );
         }
      }
   }


void RtViewPanel::SetRun( int run )
   {
   m_activeRun = run;

   for ( int i=0; i < this->GetPageCount(); i++ )
      this->GetPage( i )->SetRun( run );
   }


void RtViewPanel::GetViewRect( RECT *rect )
   {
   GetClientRect( rect );

   //RECT tbRect;
   //this->m_viewBar.GetWindowRect( &tbRect );
   //int tbHeight = tbRect.bottom - tbRect.top;

   //rect->top += tbHeight;
   }


/*
void RtViewPanel::NewRun()
   {
   CWaitCursor c;
   //gpDoc->OnAnalysisReset();
   AddRun( gpDoc->m_model.m_currentRun );
   }

*/

void RtViewPanel::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);

   CRect rect;
   GetViewRect( &rect );

   CRect tabRect;
   m_tabCtrl.GetWindowRect( &tabRect );
   int tabHeight = tabRect.Height();

   rect.top = rect.bottom - tabHeight;
   m_tabCtrl.MoveWindow( &rect );

   GetViewRect( &rect );
   rect.bottom -= tabHeight;

   if ( m_tabCtrl.GetItemCount() > 0 )
      {
      for ( int i=0; i < (int) m_pageArray.GetSize(); i++ )
         {
         // resize all associated RtViewPages
         for ( int i=0; i < m_pageArray.GetSize(); i++ )
            m_pageArray[ i ]->MoveWindow( rect );
         }
      } 
   }


void RtViewPanel::DeleteWindow()
   {/*
   CWaitCursor c;

   ViewPanel *pPanel = GetActivePanel();
   if ( pPanel == NULL || pPanel->m_pActiveView == NULL )
      return;

   pPanel->DeleteView();
   pPanel->RedrawWindow(); */
   }
   
/*
void RtViewPanel::MaximizeMinimizeWindow()
   {
   /*
   CWaitCursor c;

   ViewPanel *pPanel = GetActivePanel();
   if ( pPanel == NULL || pPanel->m_pActiveView == NULL )
      return;

   pPanel->MaximizeMinimize();

   EnableControls(); 
   }*/
   


void RtViewPanel::ExportData()
   {
   /*
   ViewPanel *pPanel = GetActivePanel();
   if ( pPanel == NULL || pPanel->m_pActiveView == NULL )
      return;

   VIEW_INFO *pInfo = pPanel->GetActiveViewInfo();

   if ( pInfo && pInfo->pInterface->GetDataObjPtr() )
      {
      DataObj *pDataObj = pInfo->pInterface->GetDataObjPtr();

      CString filename( pDataObj->GetName() );

      char *cwd = _getcwd( NULL, 0 );

      CFileDialog dlg( FALSE, "csv", filename, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
               "CSV Files|*.csv|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         {
         CString path( dlg.GetPathName() );

         CWaitCursor c;
         pDataObj->WriteAscii( path );
         }

      _chdir( cwd );
      free( cwd );
      }*/
   }


BOOL RtViewPanel::OnEraseBkgnd(CDC* pDC)
   {
   RECT rect;
   GetClientRect( &rect );
   pDC->SelectStockObject( WHITE_BRUSH );
   pDC->Rectangle( &rect );
   return TRUE;
   }



BOOL RtViewPanel::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
   {
   NMHDR *pNotify = (NMHDR*) lParam;

   switch( pNotify->idFrom )
      {
      case TABS:   // m_tabCtrl
         {
         CTC_NMHDR *pNMH = (CTC_NMHDR*) lParam;
         ChangeTabSelection( pNMH->nItem );
         }
         break;
      }

   return CWnd::OnNotify(wParam, lParam, pResult);
   }


void RtViewPanel::ChangeTabSelection( int index )
   {
   // Note: Tabs correspond to RtViewPages
   SetPage( index );
   }



bool RtViewPanel::SetPage( int index )
   {
   if ( index >= GetPageCount() || index < 0 )
      return false; 

   RtViewPage *pOldPage = GetActivePage();

   if ( pOldPage )
      pOldPage->ShowWindow( SW_HIDE );

   m_activePage = index;

   RtViewPage *pPage = GetActivePage();
   if ( pPage )
      pPage->ShowWindow( SW_SHOW );

   return true;
   }


void RtViewPanel::OnCommandMap( UINT id )
   {
   RtViewPage *pPage = GetActivePage();

   if ( pPage == NULL )
      pPage = this->AddPage( "My View" );

   // decode id
   UINT param = id - ID_RTMAP_BEGIN;

   CRect rect( 25, 25, 75, 75 );

   pPage->AddView( RTVT_MAP, param, this->m_activeRun, rect );
   }


void RtViewPanel::OnCommandGraph( UINT id )
   {
   RtViewPage *pPage = GetActivePage();

   if ( pPage == NULL )
      pPage = this->AddPage( "My View" );

   // decode id
   UINT param = id - ID_RTGRAPH_BEGIN;

   //pPage->AddView( RTVT_MAP, param, this->m_activeRun );
   }


void RtViewPanel::OnCommandViz( UINT id )
   {
   RtViewPage *pPage = GetActivePage();

   if ( pPage == NULL )
      pPage = this->AddPage( "My View" );

   int index = id - ID_RTVIZ_BEGIN;
   EnvVisualizer *pInfo = gpModel->GetVisualizerInfo( index );

   if ( pPage != NULL )
      {
      CRect rect( 25, 25, 75, 75 );
      pPage->AddView( RTVT_VISUALIZER, (INT_PTR) pInfo, -1, rect );
      }

   //RedrawWindow();
   }


void RtViewPanel::OnUpdateMap(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable( gpCellLayer != NULL ? 1 : 0 );
   }


void RtViewPanel::OnUpdateGraph(CCmdUI *pCmdUI)
   {
   //pCmdUI->Enable( gpCellLayer != NULL ? 1 : 0 );
   }


void RtViewPanel::OnUpdateViz(CCmdUI *pCmdUI)
   {
   pCmdUI->Enable(gpModel->GetVisualizerCount() > 0 ? 1 : 0 );
   }


void RtViewPanel::OnUpdateRewind(CCmdUI *pCmdUI)
   {
   BOOL enable = IsPaused() || IsReplay();
   pCmdUI->Enable( enable );
   }

void RtViewPanel::OnUpdateStart(CCmdUI *pCmdUI)
   {
   BOOL enable = IsPaused();
   pCmdUI->Enable( enable );
   }

void RtViewPanel::OnUpdatePause(CCmdUI *pCmdUI)
   {
   BOOL enable = IsRunning() || IsReplay();
   pCmdUI->Enable( enable );
   }

void RtViewPanel::OnUpdateTileCascade(CCmdUI* pCmdUI) 
   {
   BOOL enable = FALSE;
   
   RtViewPage *pPage = this->GetActivePage();

   

   //if ( this->m_rtViewWndArray.GetSize() > 0 )
   //   enable = TRUE;

   pCmdUI->Enable( enable );	
   }


void RtViewPanel::OnCascade() 
   {
   //CascadeWindows( m_hWnd, MDITILE_SKIPDISABLED | MDITILE_ZORDER, NULL, 0, NULL );
   }


void RtViewPanel::OnTile() 
   {
   //TileWindows( m_hWnd, MDITILE_HORIZONTAL, NULL, 0, NULL );
   }



void RtViewPanel::OnRewind() 
   {
   ASSERT( gpDoc->m_model.m_runStatus != RS_RUNNING );
   ASSERT( IsPaused() || IsReplay() );

   if ( IsPaused() || IsReplay() )
      {
      m_currentYear = 0;
      KillTimer();

      // reset to beginning or run and then pause
      RtViewPage *pPage = GetActivePage();

      if ( pPage != NULL )
         {
         for ( int i=0; i < (int) pPage->m_viewWndArray.GetSize(); i++ )
            {
            RtViewWnd *pView = pPage->m_viewWndArray[ i ];

            pView->SetYear( 0 );
            pView->UpdateWindow();
            }
         }
      }

   m_state = PS_PAUSED;
   }


void RtViewPanel::OnStart() 
   {
   ASSERT( IsPaused() );

   SetTimer();
   m_state = PS_RUNNING;
   }


void RtViewPanel::OnPause() 
   {
   ASSERT( IsRunning() || IsReplay() );

   KillTimer();
   m_state = PS_PAUSED;
   }



void RtViewPanel::OnTimer(UINT_PTR nIDEvent)
   {
   if ( IsPaused() )
      return;

   m_currentYear++;

   // end of run reached?
   if ( m_currentYear >= m_totalYearsToRun )
      {
      OnPause();
      return;
      }

   // update views to current year
  RtViewPage *pPage = GetActivePage();

  if ( pPage != NULL )
     {
     ASSERT( pPage->m_activeRun == m_activeRun );

     for ( int i=0; i < (int) pPage->m_viewWndArray.GetSize(); i++ )
        {
        RtViewWnd *pView = pPage->m_viewWndArray[ i ];
        pView->SetYear( m_currentYear );
        pView->UpdateWindow();
        }
     }
  }
