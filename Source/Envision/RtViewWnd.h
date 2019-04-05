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

#include <afxtempl.h>
#include <EnvContext.h>

#include "Visualizer.h"

#include "MapFrame.h"


class RtViewPanel;
class RtViewPage;
class MapList;

// RTView Types
enum RTV_TYPE {
   RTVT_MAP        = -1,  // built-in types
   RTVT_SCATTER    = -2,
   RTVT_RADAR      = -3,
   RTVT_TABLE      = -4,
   RTVT_VIEW       = -5,
   RTVT_VISUALIZER = 1     // custom types 
   };


// id's of various view types
enum {
   RTV_POPULATION,
   RTV_LULC_A,
   RTV_EVALSCORES,
   RTV_APPLIEDPOLICYSCORES, 
   RTV_APPLIEDPOLICYCOUNTS,
   RTV_AREAQUERY
   };


class RtViewWnd : public CWnd
{
DECLARE_DYNAMIC(RtViewWnd)

public:
	RtViewWnd( RtViewPage *pParent, int run, RTV_TYPE type, CRect &rect );
	virtual ~RtViewWnd();

   bool InitRun( void );      // called at the invocation of each EnvModel::Run()   
   int GetRun ( void ) { return m_run;  }
   int GetYear( void ) { return m_year; }

   //-------------------------------------------------//
   //--- these should be overridden by subclasses ----//
   //-------------------------------------------------//
   
   // for replays, post-run support.  For these, the current RtViewRuninfo class should be used
   virtual bool SetYear( int year )  { m_year = year; return true; } 
   virtual bool SetRun ( int run  ); //  { m_run  = run;  return true; } 
   
   // for real-time updates, based on current state
   virtual BOOL UpdateWindow( void ) { return TRUE; }

   // for all 
   virtual bool InitWindow( void ) = 0; //   { return false; }  // called immediately after window is created
   virtual RTV_TYPE GetRtvType() = 0;   
   //-------------------------------------------------//
   
protected:
   RTV_TYPE     m_rtvType;       // defined by subclasses
   RtViewPage  *m_pPage;         // associated page (parent container)
   RtViewPanel *m_pPanel;        // associated panel
   DeltaArray  *m_pDeltaArray;   // delta array this view is associated with
   int          m_run;           // current run for this view        
   int          m_year;          // current year the view

	DECLARE_MESSAGE_MAP()

public:
   int m_xLeft;       // these are all measured as percent of the screen (e.g. "50")
   int m_xRight;
   int m_yTop;
   int m_yBottom;

   afx_msg void OnSize(UINT nType, int cx, int cy);
};


//------------------------------------------------------------------------
// RtVisualizer - a specialized RtViewWnd that knows about
// visualizers
//------------------------------------------------------------------------

class RtVisualizer : public RtViewWnd
{
DECLARE_DYNAMIC(RtVisualizer)

public:
   RtVisualizer( RtViewPage *pParent, int run, EnvVisualizer *pInfo, CRect &rect )
      : RtViewWnd( pParent, run, RTVT_VISUALIZER, rect )
      , m_pInfo( pInfo )
      , m_pVisualizerWnd( NULL )
   { }

protected:
   EnvVisualizer *m_pInfo;    // points to structure maintained by CEnvDoc

public:
   // overrides
   virtual bool SetYear( int year ) { RtViewWnd::SetYear( year ); if ( m_pVisualizerWnd != NULL ) { m_pVisualizerWnd->m_envContext.currentYear = year; m_pVisualizerWnd->m_envContext.yearOfRun = year-m_pVisualizerWnd->m_envContext.startYear; } return true; }
   virtual bool SetRun ( int run  ) { RtViewWnd::SetRun( run ); if ( m_pVisualizerWnd != NULL ) m_pVisualizerWnd->m_envContext.run = run; return true; }
   //virtual BOOL UpdateWindow( void );

   virtual bool InitWindow( void ) { return TRUE; }
   virtual RTV_TYPE GetRtvType( void ) { return RTVT_VISUALIZER; }

   EnvVisualizer *GetVisualizerInfo( int i ) { return m_pInfo; }
   VisualizerWnd  *m_pVisualizerWnd;
   
 	DECLARE_MESSAGE_MAP()

   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

//------------------------------------------------------------------------
// RtMap - a specialized RtViewWnd that knows about
// maps
//------------------------------------------------------------------------

class RtMap : public RtViewWnd
{
DECLARE_DYNAMIC(RtMap)

public:
   RtMap( RtViewPage *pParent, int run, CRect &rect,  int layer, int col )
      : RtViewWnd( pParent, run, RTVT_MAP, rect )
      , m_mapFrame()
      , m_layer( layer )
      , m_col( col )
      , m_overlayCol( -1 )
      , m_isDifference( false )
      , m_pMap( NULL )
      , m_pMapList( NULL )
   { }

protected:
   int m_layer;   // map layer index for source map
   int m_col;     // source maplayer col
   int m_overlayCol;
   
   bool m_isDifference;
   CArray< VData, VData& > m_startDataArray;

   Map        *m_pMap;
   MapListWnd *m_pMapList;

public:
   // overrides - these should be overridden by subclasses
   virtual bool InitWindow( void );
   virtual bool SetYear( int year );
   virtual bool SetRun ( int run  ) { return true; } // { RtViewWnd::SetRun( run ); if ( m_pVisualizerWnd != NULL ) m_pVisualizerWnd->m_envContext.run = run; }
   //virtual BOOL UpdateWindow( void );
   virtual RTV_TYPE GetRtvType( void ) { return RTVT_MAP; }
   
protected:
   MapFrame m_mapFrame;

   void ShiftMap( int fromYear, int toYear );

public:
 	DECLARE_MESSAGE_MAP()

   afx_msg void OnSize(UINT nType, int cx, int cy);

};

