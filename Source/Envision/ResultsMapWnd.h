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


// ResultsMapWnd

class Map;

#include "MapListWnd.h"
#include "StaticSplitterWnd.h"
#include "MapPanel.h"

#include <DeltaArray.h>
#include <map.h>

int ResultsMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );

class ResultsMapWnd : public CWnd
{
public:
	ResultsMapWnd( int run, int fieldInfoOffset, bool isDifference );
	virtual ~ResultsMapWnd();

   static bool Replay( CWnd *pWnd, int flag );
   bool _Replay( int flag );

   static bool SetYear( CWnd *pWnd, int oldYear, int newYear );
   bool _SetYear( int oldYear, int newYear );

public:
   MapFrame     m_mapFrame;
   Map         *m_pMap;         // pointers to the two contained windows
                                // note:  this layer contains a copy of the original data
protected:
   MapListWnd  *m_pMapList;
   DeltaArray  *m_pDeltaArray;  // pointer to the delta array for this run.
                                // note: this is not a copy, it's the original
   CSliderCtrl m_yearSlider;
   CEdit       m_yearEdit;

   enum { RMW_SLIDER = 8987, RMW_EDIT  };

   int m_year;
   int m_startYear;
   int m_endYear;
   int m_iduCol;    // column index of the IDU database
   int m_overlayCol;

   int m_run;
   int m_fieldInfoOffset;
   bool m_isDifference;

   CArray< VData, VData& > m_startDataArray; // only used for differnece maps

public:
   void RefreshList( void ) { if ( m_pMapList != NULL ) m_pMapList->Refresh(); }

protected:
   void ShiftMap( int fromYear, int toYear );
   void OnSelectOption( UINT nID );

protected:
	DECLARE_MESSAGE_MAP()
public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
   afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
   afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnShowresults();
	afx_msg void OnShowdeltas();
   };


