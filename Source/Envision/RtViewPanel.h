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
#pragma once

#include "RtViewWnd.h"
#include <customtabctrl\customtabctrl.h>

#include <PtrArray.h>

class Scenario;

const int TABS = 1000;

// command ID ranges
const int ID_RTMAP_BEGIN   = 2000;
const int ID_RTMAP_END     = 2999;
const int ID_RTGRAPH_BEGIN = 3000;
const int ID_RTGRAPH_END   = 3999;
const int ID_RTVIEW_BEGIN  = 4000;
const int ID_RTVIEW_END    = 4999;
const int ID_RTVIZ_BEGIN   = 5000;
const int ID_RTVIZ_END     = 5999;

//----------------------------------------------------------------------------------------------------------
// Real Time Views.
//
// Basic idea:
//
//  RtViewPanel class is the main window.  it contains a tab control.  The Tab Control 
//     contains tabs that correspond to different collections (pages) of views.  These tabs contain
//     windows (class RtViewPage) that contain contain all the views for a page.
//
//  The RtViewPanel also tracks run information, to a allow a "Current run" to be displayed.
//  Run information is stored in the RtViewRunInfo class, one entry for each run.  It contains
//  a pointer to the run's DeltaArray among other items//
//
//  RtViewPage class - a CWnd derived class that contains views for a given page of inputs.
//
//  RtViewWnd class - this is the base class for each view.  A given RtViewWnd is defined by its
//     view type (RTV_TYPE).  RtViewWnd is a virtual class that exposes an interface children must implement
//
//  To add new view types, you need to derive a class (say, MyRtView) from RtViewWnd and implement the RtViewWnd
//     interface.  This includes:
//   virtual BOOL Create( void ) { return false; }
////   virtual BOOL UpdateWindow( void ) { return TRUE; }
//   virtual RTV_TYPE GetRtvType() = 0;
//
//  Sequence of events:
//      1) One (and only one) RtViewPanel is created when the Parent EnvView is created.  At that point, there
//         are no pages, no views, etc.
//
//      2) When the Envx file is loaded, any pages/views defined are created. Note that only one set of views
//         are created; these are "pointed to" appropriate run information, delta array, etc as needed. 
//         During RtViewWnd initialization, the view should depict initial state of the system in a manner
//         specific to the view
//
//      3) Users may interactively add a view "before" a run.  When this happens, the RtViewWnd is created,
//         and it's RtViewWnd::InitWindow() method is called.  However, no run information is set - this happens
//         at the beginning of EnvModel::Run(), which calls RtViewPanel::SetRun(), which in turn invokes SetRun()
//         on all the pages and viewWnds that have been defined.  It is up the the RtViewWnd subclasses to 
//         initialize the view to a reasonable state.
//         
//      4) During an invocation of EnvModel::Run()...
//         a) A "run" is added to a RtViewPanel at the beginning of EnvModel Run() by calling 
//            RtViewPanel::AddRun(). This causes a new RtViewRunInfo object to be created and populated.
//         b) RtViewPanel::SetRun() is invoked using the current run, which in turns
//            causes the contained objects SetRun() methods to be invoked.  In the case of RtViewWnd,
//            this causes the run information to be stored and the m_pDeltaArray member to be set to the
//            current EnvModels DeltaArray.
//
//      5) If an RtViewWnd subclass overrides SetRun(), it should call RtViewWnd::SetRun() before doing it's
//         own initialization
//
//      5) Whenever the user changes a run or year, all RtViewWnds are notified of the change.
//----------------------------------------------------------------------------------------------------------

//------------------------------------------------------------
// STATE MODEL FOR RtViewPanel
//
//       EVENT-->   REWIND         START        PAUSE
// STATE              <||           |>           []
//------------------------------------------------------------
// PRERUN          (disabled)    (start?)     (disabled)
// RUNNING         (disabled)    (disabled)     Pause
// PAUSED            Rewind       Start       (disabled)  
// REPLAY            Rewind      (disabled)     Pause
//// STEP_FORWARD      Rewind       Start        (disabled)
//// STEP_BACK
//------------------------------------------------------------

// states
enum RTSTATE {
   PS_UNKNOWN      = 0, 
   PS_PRERUN       = 1,    // about to start a run. Views should be at initial conditions
   PS_RUNNING      = 2,    // running.  Views should reflect current Envision state
   PS_PAUSED       = 4,    // paused during replay.  View should reflect selected delta array
   PS_REPLAY       = 8     // replaying an existing run.  View should reflect selected delta array 
   };


class RtViewRunInfo
{
public:
   RtViewRunInfo( void ) : m_run( -1 ), m_pDeltaArray( NULL ), m_pScenario( NULL ) { }

   int         m_run;               // 0-based run index this info refers to
   DeltaArray *m_pDeltaArray;       // maintained by data manager
   Scenario   *m_pScenario;         // scenarion used for this run
};


class RtViewPage : public CWnd     // contents of a tab
{
DECLARE_DYNAMIC(RtViewPage)

public:
   RtViewPage( void ) : CWnd(), m_index( -1 ), m_activeRun( -1 ), 
      m_currentYear( 0 ), m_totalYearsInRun( 0 ),
      m_pActiveWnd( NULL ), m_pRunInfo( NULL )
      { }

   int m_index;
   int m_activeRun;
   int m_currentYear;
   int m_totalYearsInRun;

   //bool UseDeltaArray( void ) { return m_useDeltaArray; }    // during runs, use map.  During replays, use

   PtrArray< RtViewWnd > m_viewWndArray;
   RtViewWnd     *m_pActiveWnd;
   RtViewRunInfo *m_pRunInfo;

public:
   RtViewWnd *AddView( RTV_TYPE, INT_PTR extra, int run, CRect &rect );   // rect is percentage of screen (left->right, top->bottom)
   RtViewWnd *GetActiveView() { return m_pActiveWnd; }
   void SetRun( int run ) { m_activeRun = run; for ( int i=0; i < (int) m_viewWndArray.GetSize(); i++ ) m_viewWndArray[ i ]->SetRun( run ); }

	DECLARE_MESSAGE_MAP()

   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


// RtViewPanel - main container for a RtViewPage.
// It manages run information and collections 
// of Tabs (one tab for each page of screens).
// Also manages Window event handling and 
// animation related to the contained views

class RtViewPanel : public CWnd
{
DECLARE_DYNAMIC(RtViewPanel)

public:
	RtViewPanel();
	virtual ~RtViewPanel();

// tab management.  Each tab is a run
   CCustomTabCtrl m_tabCtrl;
   void ChangeTabSelection( int index );
 
protected:
   CFont  m_font;

public:
   static int m_nextID;

// run management
protected:
   PtrArray< RtViewPage > m_pageArray;
   PtrArray< RtViewRunInfo > m_runInfoArray;

public:
   void AddRun( int run );       // this gets called at the start of a run with an empty delta array, deltaArray added at end of run 
   RtViewRunInfo *GetActiveRunInfo() { return m_activeRun  >= 0 ? m_runInfoArray[ m_activeRun ] : NULL; }
   RtViewRunInfo *GetRunInfo( int run ) { return m_runInfoArray[ run ]; }
 
   RtViewPage    *GetActivePage()    { return m_activePage >= 0 ? m_pageArray[ m_activePage ] :  NULL; }
   RtViewPage    *GetPage( int index ) { return m_pageArray[ index ]; }
   RtViewPage    *AddPage( LPCTSTR name );      // returns index of added page
   int  GetPageCount( void ) { return (int) m_pageArray.GetSize(); }
   bool UseDeltaArray( void ) { return true; } //IsPaused() || IsReplay(); }

   // state management
protected:
   RTSTATE m_state;     // flag indicating current run status, one of:
                        // PS_UNKNOWN, PS_PRERUN, PS_RUNNING, PS_PAUSED, PS_REPLAY
   int m_currentYear;
   int m_activeRun;
   int m_activePage;

   int m_totalYearsToRun;

   UINT_PTR m_timerID;

public:
   bool SetPage( int page );  // zero-based index of page to set
   void SetYear( int year );  // broadcast to active views to move to the current year
   void SetRun ( int run );   // switch to the current year in a new run, updating UI

   bool IsRunning( void ) { return m_state & PS_RUNNING ? true : false; }
   bool IsPaused ( void ) { return m_state & PS_PAUSED  ? true : false; }
   bool IsReplay ( void ) { return m_state & PS_REPLAY  ? true : false; }
   bool IsPreRun ( void ) { return m_state & PS_PRERUN  ? true : false; }
   
   void SetTimer( void ) { if ( m_timerID < 0 ) m_timerID = CWnd::SetTimer( 1000, 1000, NULL ); }
   void KillTimer( void ) { if ( m_timerID >= 0 ) CWnd::KillTimer( m_timerID ); }
      
// window management
public:
   void GetViewRect( RECT *rect );

   //RtViewWnd *AddWindow( RTV_TYPE, INT_PTR extra, int run );
   RtViewWnd *GetActiveView() { if ( m_activePage >= 0 ) return GetActivePage()->GetActiveView();  return NULL; }
 
	DECLARE_MESSAGE_MAP()

   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
     
protected:
   afx_msg void DeleteWindow();
   afx_msg void MaximizeMinimizeWindow();
   afx_msg void ExportData();

public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnTimer(UINT_PTR nIDEvent);
   afx_msg void OnCommandMap( UINT id );
   afx_msg void OnCommandGraph( UINT id );
   afx_msg void OnCommandViz( UINT id );
   afx_msg void OnUpdateGraph(CCmdUI *pCmdUI);
   afx_msg void OnUpdateMap(CCmdUI *pCmdUI);
   afx_msg void OnUpdateViz(CCmdUI *pCmdUI);
   afx_msg void OnUpdateRewind(CCmdUI *pCmdUI);
   afx_msg void OnUpdateStart(CCmdUI *pCmdUI);
   afx_msg void OnUpdatePause(CCmdUI *pCmdUI);
	afx_msg void OnUpdateTileCascade(CCmdUI* pCmdUI);
   afx_msg void OnCascade();
	afx_msg void OnTile();
	afx_msg void OnRewind();
	afx_msg void OnStart();
	afx_msg void OnPause();

protected:
   //virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
   virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

