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

#include <EnvExtension.h>
#include <PtrArray.h>


class VizManager;

//interface class for any object that should be called for PostRun/Results animations
class IVizPostRunHooks
{
public:
	virtual BOOL PostrunReplay(int year) = 0;
	virtual BOOL PostrunSetYear(int oldYear, int newYear) = 0;
};

// VisualizerWnd 
//
// This provides a container window for visualizers.  The visualizer window
//  consists of several windows:
//  1) the container window for the EnvVisualizer,
//  2) a slider bar and edit control for representing/changing the current display period.
//

class VisualizerWnd : public CWnd
{
friend class VizManager;

DECLARE_DYNAMIC(VisualizerWnd)

protected:
	VisualizerWnd( EnvVisualizer *pViz, int run );    // use VizManager::CreateWnd()

public:
   virtual ~VisualizerWnd();

public:
   EnvVisualizer *m_pVizInfo;
   IVizPostRunHooks* m_pRTProcessor;

   EnvContext m_envContext;
   int m_useage; //what view invoked this window?
public:
   BOOL InitWindow  ( EnvContext *pContext=NULL );    // NULL means use internal
   BOOL UpdateWindow( EnvContext *pContext=NULL );
   
   virtual BOOL PostrunReplay(int year);
   virtual BOOL PostrunSetYear(int oldYear, int newYear);

   int GetRun( void ) { return m_envContext.run; }
   
   DECLARE_MESSAGE_MAP()

public:
   //static calls for results run updates
	static bool RtReplay(CWnd* pWnd, int year) { return ( ((VisualizerWnd*)pWnd)->PostrunReplay(year) ) ? true : false; };  // -1=reset
	static bool RtSetYear(CWnd* pWnd, int oldYear, int newYear) { return (((VisualizerWnd*)pWnd)->PostrunSetYear(oldYear, newYear) ) ? true : false; }

public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);

protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
   afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
   afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


class VizManager
{
public:
   VizManager( void ) : m_nextID( 80000 ) { }

   int AddVisualizer( VisualizerWnd *pWnd ) { return (int) m_visualizers.Add( pWnd ); }
   int GetVisualizerCount( void ) { return (int) m_visualizers.GetSize(); }
   VisualizerWnd *GetVisualizerWnd( int i ) { return m_visualizers[ i ]; } 

   VisualizerWnd *CreateWnd( CWnd *pParent, EnvVisualizer *pInfo, int run, BOOL &success, int useage );
   
protected:
   PtrArray< VisualizerWnd > m_visualizers;
   
   int m_nextID;
};


inline
VisualizerWnd *VizManager::CreateWnd( CWnd *pParent, EnvVisualizer *pInfo, int run, BOOL &success,int useage )
   {
   VisualizerWnd *pWnd = new VisualizerWnd( pInfo, run );
   pWnd->m_useage = useage;

   RECT rect;
   pParent->GetClientRect( &rect );
   
   DWORD style = 0;


   //different uses provide slightly different setups for visualizations;
   //seperate out style flags base on useage case
   switch (useage)
   {
   case VZT_INPUT:
   case VZT_RUNTIME:
	   style = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	   break;
   case VZT_POSTRUN:
   case VZT_POSTRUN_GRAPH:
   case VZT_POSTRUN_MAP:
   case VZT_POSTRUN_VIEW:
	   style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE;
	   break;
   }
   success = pWnd->Create(NULL, pInfo->m_name, style,
                           rect, pParent, m_nextID++ );

   m_visualizers.Add( pWnd );
  
   pWnd->m_envContext.pWnd = pWnd;
   pWnd->m_envContext.run  = run;
   pWnd->m_envContext.yearOfRun = 0;

   return pWnd;
   }