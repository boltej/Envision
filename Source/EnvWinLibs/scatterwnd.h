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
#if !defined(AFX_SCATTERWND_H__443F401A_870E_44FE_85A0_AB9113540C0E__INCLUDED_)
#define AFX_SCATTERWND_H__443F401A_870E_44FE_85A0_AB9113540C0E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <graphwnd.h>



// scatterwnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ScatterWnd window


class WINLIBSAPI ScatterWnd : public GraphWnd
{
friend void CALLBACK EXPORT ScatterTimerProc( HWND, UINT, UINT_PTR, DWORD );

DECLARE_DYNAMIC( ScatterWnd )

// Construction
public:
	ScatterWnd( void );
	ScatterWnd( DataObj*, bool makeDataLocal=false );

// Attributes
protected:
   bool  isTrajPlot;   // is this a "special" trajectory plot?
   int   trajLag;      // number of points to look back
   bool  eraseLastBar;
   bool  showCurrentPosBar;
   bool  drawToCurrentPosOnly;
   float currentPosValue;     // virtual coordinates
   int   lastDynamicDisplayIndex;
   int   validRows;
   bool  autoscaleAll;        // show all traces autoscaled, don't show axis...

   CToolTipCtrl m_toolTip;
   TCHAR  m_toolTipContent[ 512 ];
   CPoint m_ttPosition;    // client coords
   bool   m_activated;

   bool CollectTTLines( CString &tipText );
   bool IsPointOnDataPoint( CPoint &point, CDC *pDC, LINE *pLine, POINT &lowerLeft, POINT &upperRight, float &xValue, float &yValue );

   bool ClipLine( POINT &start, POINT &end, POINT &lowerLeft, POINT &upperRight );
   void DrawSmoothLine( HDC hDC, LINE *pLine, POINT &lowerLeft, POINT &upperRight, int startIndex, int numPoints, int rows, DataObj *, int yColNum );
   
// Operations
public:
   virtual bool UpdatePlot( CDC*, bool, int startIndex=0, int numPoints=-1 );   // draw background + DrawLine()
   virtual bool DrawLine  ( HDC, LINE*, POINT&, POINT&, int startIndex, int numPoints ); // draw individual lines

   //-- note: This MUST be called AFTER setting data objects! --//
   void SetAutoscaleAll( bool flag=true ) { autoscaleAll=flag; }
   void MakeTrajPlot( int lag );
   void ShowCurrentPosBar( bool show ) { showCurrentPosBar = show; }
   void DrawToCurrentPosOnly( bool enable ) {  drawToCurrentPosOnly = enable; }
   
   void UpdateCurrentPos( float value );

   void UpdateLine( float xPrevious, float yPrevious, float x, float y );

   void SetDynamicDisplayInterval( int milliseconds ) { lastDynamicDisplayIndex = 0; SetTimer( (UINT_PTR) this, milliseconds, ScatterTimerProc ); }
   void StopDynamicDisplay() { KillTimer( 7000 ); }

   int  GetValidRows(){ return validRows; }
   void SetValidRows( int _validRows ){ validRows = _validRows; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ScatterWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~ScatterWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(ScatterWnd)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   BOOL OnTTNNeedText(UINT id, NMHDR* pTTTStruct,  LRESULT* pResult); 

   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
   afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
   };

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCATTERWND_H__443F401A_870E_44FE_85A0_AB9113540C0E__INCLUDED_)
