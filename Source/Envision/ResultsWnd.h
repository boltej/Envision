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
#if !defined(AFX_RESULTSWND_H__A76913A7_C847_11D3_95C5_00A076B0010A__INCLUDED_)
#define AFX_RESULTSWND_H__A76913A7_C847_11D3_95C5_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResultsWnd.h : header file
//

#include "MapPanel.h"
#include <afxtempl.h>

class CAVIGenerator;

typedef bool (*REPLAYFN)( CWnd*, int year );  // -1=reset
typedef bool (*SETYEARFN)( CWnd*, int oldYear, int newYear );

enum RCW_TYPE
   {
   RCW_MAP,
   RCW_GRAPH,
   RCW_TABLE,
   RCW_VIEW,
   RCW_VISUALIZER
   };


/////////////////////////////////////////////////////////////////////////////
// ResultsChildWnd
//
// A ResultsChildWnd is a window class that contains post-run results of
// some type.  Windows that render results are generally Child windows
// that live inside a ResultsChildWnd.

class ResultsChildWnd : public CWnd
{
public:
   ResultsChildWnd( CWnd *pWnd, RCW_TYPE type, REPLAYFN pfnReplay, SETYEARFN pfnSetYear );
   virtual ~ResultsChildWnd();

public:
   CWnd *m_pWnd;              // child window contained by this window
   REPLAYFN  m_pfnReplay;
   SETYEARFN m_pfnSetYear;
   RCW_TYPE  m_type;
protected:
	DECLARE_MESSAGE_MAP()

public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
};


/////////////////////////////////////////////////////////////////////////////

class CResultsWndArray : public CArray< ResultsChildWnd*, ResultsChildWnd* >
   {
   public:
      ResultsChildWnd* Add( CWnd *pWnd, RCW_TYPE type, REPLAYFN pfReplay, SETYEARFN pfSetYear )
         {
         ResultsChildWnd *pChild = new ResultsChildWnd( pWnd, type, pfReplay, pfSetYear );
         CArray< ResultsChildWnd*, ResultsChildWnd* >::Add( pChild );
         return pChild;
         }
   };


/////////////////////////////////////////////////////////////////////////////
// ResultsWnd window
//
// Notes:  there is one and only one ResultsWnd.  It is the parent window for all
//   post-run results windows (which all subclass from ResultsChildWnd).
//
// the ResultWnd contains zero or more child windows which it controls.
// These child windows are of type ResultsChildWnd, which contain
// functions for replaying and setting the current year for the child wind
// Its main responsibility is to coordinate these child windows during replay, etc. by

class ResultsWnd : public CWnd
{
// Construction
public:
	ResultsWnd();
	DECLARE_DYNCREATE(ResultsWnd)

// Attributes
public:
   CResultsWndArray m_wndArray;
   int   m_resultCount;
   CWnd *m_pActiveWnd;
   int   m_year;  // current year being played

   bool m_syncMapZoom;

// Operations
public:
   void RemoveAll();    // clear out all maps, etc.

   void  AddWnd( CWnd *pWnd, RCW_TYPE type, REPLAYFN pfReplay, SETYEARFN pfSetYear )
      {
      MakeActive( (CWnd*) m_wndArray.Add( pWnd, type, pfReplay, pfSetYear ) );  // makes a new ResultsChildWnd
      }

   // Replay Interface ( Called from ResultsTree )
   void ReplayReset();
   void ReplayStop();
   void ReplayPlay();
   void SetYear( int oldYear, int year );

   void SetCurrentYear( int year ) { m_year = year; }

   //void SyncMapZoom( Map *pSourceMap, ResultsMapWnd *pResultsMapWnd );


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ResultsWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~ResultsWnd();

   void Tile();
   void Cascade();
   void Clear() { RemoveAll(); MakeActive( NULL ); RedrawWindow(); }
   void MakeActive( CWnd *pWnd );

public:
   //AVI production
   void WriteFrame();
   void InitMovieCapture();
   void CleanupMovieCapture();
   bool m_createMovie;
   CAVIGenerator** m_aviGenerators;
   struct AuxMovieStuff* m_auxMovieStuff;


   // Generated message map functions
protected:
	//{{AFX_MSG(ResultsWnd)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnParentNotify(UINT message, LPARAM lParam);
   afx_msg void OnPaint();   
   afx_msg void OnTimer(UINT_PTR nIDEvent);
};

// this is all just for precalculating and storing things to avoid doing it per-frame
struct AuxMovieStuff
{
   CDC bitmapDC;
   CBitmap bitmap;
   BITMAPINFO bi;
   LPBITMAPINFO lpbi;
   BYTE* bitAddress;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESULTSWND_H__A76913A7_C847_11D3_95C5_00A076B0010A__INCLUDED_)
