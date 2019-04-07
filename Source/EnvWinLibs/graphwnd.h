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
#if !defined(AFX_GRAPHWND_H__02F082FC_035F_4F2D_98FF_8D801C7CD663__INCLUDED_)
#define AFX_GRAPHWND_H__02F082FC_035F_4F2D_98FF_8D801C7CD663__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// graphwnd.h : header file
//
#include "EnvWinLibs.h"

#include <typedefs.h>
#include <ptrarray.h>
#include <strarray.hpp>
#include <Report.h>        // handles error messages
#include <string.h>
#include <colors.hpp>
#include <marker.hpp>
#include <ppplotsetup.h>
#include <ppplotaxes.h>
#include <ppplotlines.h>

class DataObj;
class FDataObj;
class CWndManager;

//------------------------- constants -----------------------------//

//-- frameStyle constants --//
enum { FS_GROOVED=120, FS_RAISED, FS_SUNKEN, FS_NORMAL };

//-- window Logical coordinants ( mins are 0 ) --//
const int XLOGMAX  = 2560;
const int YLOGMAX  = 2560;

//-- data window offsets (logical coords?) --/
//const int XDATAOFFSET_LEFT  = 150;
//const int XDATAOFFSET_RIGHT = 300;
//const int YDATAOFFSET       = 150;

//const int LEGEND_HEIGHT  = 30;      // default values
//const int LABEL_HEIGHT   = YDATAOFFSET/4;
//const int TITLE_HEIGHT   = YDATAOFFSET/3;

//-- orientation flags --//
const int OR_HORZ  = 0;
const int OR_VERT  = 2700;

//-- label text flags --//
enum MEMBER { TITLE, XAXIS, YAXIS };

//-- Bar plot equivalents --//
#define MS_BAR2DSOLID   MS_SOLIDSQUARE
#define MS_BAR2DHOLLOW  MS_HOLLOWSQUARE

//#define MS_BAR3DSOLID   MS_SOLIDCIRCLE  // not implemented
//#define MS_BAR3DHOLLOW  MS_HOLLOWCIRCLE
const LONG PLOTWNDCOLORREF = LTGRAY;
const int   TICSIZE  = 10;
const int   BORDERSIZE = 20;  // logical 
const float LEGEND_SPACING = 0.20f;  // pct

//-- text information ( title, labels, legends ) --//
struct LABEL
   {
   CString   text;            // text
   COLORREF  color;           // color to draw with
   RECT      rect;            // rects containing text
   //int       orientation;     // OR_HORZ, OR_VERT stored in logfont structure
   LOGFONT   logFont;

   LABEL( void ) : color( BLACK ), rect() { }//, orientation( OR_HORZ ) {}
   ~LABEL( void ) { }
   };


struct LINE
   {
   CString   text;            // text
   DataObj  *pDataObj;        // associated data object
   UINT      dataCol;         // corresponding column in the data object
   RECT      rect;            // rects containing text
   //int       orientation;     // OR_HORZ, OR_VERT
   COLORREF  color;           // color to draw with
   bool      showLine;
   bool      showMarker;
   FLAG      lineStyle;  // line style (see windows.h)
   MSTYLE    markerStyle;
   COLORREF  markerColor;
   int       lineWidth;       // line width
   int       markerIncr;      // marker display increment
   float     scaleMin;        // scale factors
   float     scaleMax;
   bool      m_isEnabled;
   bool      m_smooth;
   float     m_tension;
//   LOGFONT   logfont;         // font for this legend.  Size is in LOGICAL coords, converted to screen as needed

   LINE( void ) :  pDataObj( NULL ), dataCol( 0 ), rect(),
                  //orientation( OR_HORZ ),
                  color      ( 0 ),
                  showLine   ( true ),
                  showMarker ( true ),
                  lineStyle  ( PS_DASH ),
                  markerStyle( MS_CROSS ),
                  markerColor( 0 ),
                  lineWidth  ( 8 ),
                  markerIncr ( 5 ),
                  scaleMin   ( 0 ),
                  scaleMax   ( 0 ),
                  m_isEnabled( true ),
                  m_smooth   ( false ),
                  m_tension  ( 0.5f )
                  { }
         
   ~LINE( void ) {}
   };


//-- x, y Axis information --//
struct AXIS
   {
   int      digits;           // number of digits to the right of the decimal
   float    major;            // major tic increment
   float    minor;            // minor tic increment
   COLORREF color;            // color to draw with
   bool     showLabel;
   bool     showTics;
   bool     showScale;
   bool     autoScale;
   bool     autoIncr;

   AXIS( void ) : digits( 0 ), major( float(0) ), minor( float(0) ), color( BLACK ),
                  showLabel( true ), showTics( true ), showScale( true ),
                  autoScale( true ), autoIncr( true ) { }
   };

struct COLDATA
   {
   DataObj *pDataObj;     // ptr to the data object
   UINT     colNum;       // column of data in the dataobj
   };



/////////////////////////////////////////////////////////////////////////////
// GraphWnd window

class WINLIBSAPI GraphWnd : public CWnd
{
DECLARE_DYNAMIC( GraphWnd )

//friend class GraphAxisDlg;
//friend class GraphLineDlg;
//friend class GraphPlotDlg;

// Construction
public:
	GraphWnd();
   GraphWnd( DataObj*, bool makeDataLocal=false );

// Attributes
protected:
   //--- structs ---//
   PtrArray < LINE > m_lineArray;   // line (and legend) info

public:
   LABEL label[ 3 ];            // labels (for title, xaxis, yaxis )
   AXIS  xAxis;                 // axis info
   AXIS  yAxis;

public:
   int m_chartOffsetLeft;        // logical coords (default=150)
   int m_chartOffsetRight;       // logical coords (default=300)
   int m_chartOffsetTop;         // logical coords (default=150)
   int m_chartOffsetBottom;      // logical coords (default=150)
  
   int m_legendFontHeight;
   int m_legendRowHeight;   // text height for one row - logical coords(default= 30)
   int m_labelHeight;    // text height for labels (logical coords)
   int m_titleHeight;    // text height for title (logical coords)

   CRect m_legendRect;
   CRect m_chartRect;

   int    m_dragState;      // 0= not dragging, 1= dragging mouse
   CPoint m_startDragPt;    // device coords
   
   bool m_isLegendFrameVisible;

protected:
   StringArray xAxisLabelArray;   // axis labels (optional)
   StringArray yAxisLabelArray;

   CWndManager *pTableMngr;

   //--- data variable definitions---//
   float xMin;             // virtual coordinants (for whle plot)!
   float xMax;
   float yMin;
   float yMax;

public:
   UINT  frameStyle;       // 
   bool  showXGrid;
   bool  showYGrid;

protected:
   //-- data object info --//
   COLDATA  xCol;          // hold dataobj, location of xcol in dataobject

public:
   FDataObj *pLocDataObj;  // pLocDataObj is deleted by destructor this object (Note:  This is always a FDataObj, not DataObj)

protected:
   //-- miscellanous others --//              
   UINT lastRowCount;      // initially 0, then set at each update
   bool allowZoom;         // unable mouse zoom functions
   int  dateYear;          // for displaying date one x axis
   int  start;             // index of data to start drawing from (-1 mean use all) - currently not used
   int  end;               // index of data to end drawing (-1 means use all) - currently not used

   //--- LOGFONT variables ---//
   LOGFONT m_logFont;      // used to change system font
   int     InitializeFont( LOGFONT &lf ) {  memcpy( &lf, &m_logFont, sizeof( LOGFONT ) ); return sizeof( LOGFONT ); }


protected:
   //----------------- protected methods -----------------------//
   bool Initialize  ( void );
   bool BuildSysMenu( void );

   int  IsPointOnLegend( POINT &click );

   //-- initial mapping for the data, plot window window --//
   void InitMapping( CDC* );
   void DrawRubberRect( CDC*, RECT &rect, int thickness );

   //-- convert between logical-virtual coordinants --//
   void VPtoLP( COORD2d &point );
   void VPtoLP( COORD2d &point, float minimum, float maximum );
   void LPtoVP( COORD2d &point );

   //-- internal drawing methods --//
   void SetFont( CDC *pDC );
   void SetLayout( CDC *pDC );
   virtual void DrawTitle ( CDC *pDC );
   virtual void DrawLabels( CDC *pDC );
   virtual void DrawAxes  ( CDC *pDC, bool drawYAxisLabels=true );
   virtual void DrawFrame ( CDC *pDC );
   virtual void DrawPlotArea(CDC *pDC ) { }

public:
   virtual void RefreshPlotArea();

   void RemoveLines() { m_lineArray.RemoveAll(); }

// Operations
public:
   virtual bool UpdatePlot( CDC *pDC, bool, int startIndex=0, int numPoints=-1 ) { return true; } // should be overridden by children to draw frame + data

   void GetFrameRect( POINT &lowerLeft, POINT &upperRight );  // logical coordinates

   //-- extents --//
   void SetExtents( MEMBER axis, float minimum,  float maximum, bool computeTickIncr=true, bool redraw=false );
   void GetExtents( MEMBER axis, float *minimum, float *maximum );
   int  GetAutoScaleParameters( float &minimum, float &maximum, int &ticks, float &tickIncr  );     // returns number of digits in scale

   // get bouding rect of actual plot in logical coords
   void GetPlotBoundingRect( HDC hDC, LPPOINT lowerLeft, LPPOINT upperRight );
   int  GetIdealLegendSize( CDC *pDC, CSize &sz );

   //-- legends --//
   bool SetLegendText( int lineNo, LPCTSTR newText );

   void SetLegendColor( int lineNo, int newColor )
               { m_lineArray[ lineNo ]->color = newColor; }

   void SetLegendRect( int lineNo, RECT rect )
               { SetRect( (LPRECT) &( m_lineArray[ lineNo ]->rect ), rect.left, rect.top, rect.bottom, rect.right ); }

   //-- labels --//
   void SetLabelRect( MEMBER axis, RECT rect )
               { SetRect( (LPRECT) &label[axis].rect, rect.left, rect.top, rect.bottom, rect.right ); }

   bool SetLabelText( MEMBER axis, LPCTSTR newText );
   int  GetLabelText( MEMBER axis, LPSTR titleText, int maxCount );

   void SetLabelColor( MEMBER axis, int newColor )
               { label[ axis ].color = newColor; }

   //void SetLabelHeight ( int height ) { labelHeight  = height; }
   //void SetLegendHeight( int height ) { legendHeight = height; }

   void SetAxisTickIncr( MEMBER axis, float major,  float minor  );
   void GetAxisTickIncr( MEMBER axis, float *major, float *minor );

   void SetAxisDigits( MEMBER axis, int numDigits );
   int  GetAxisDigits( MEMBER axis );

   void ShowAxisScale( MEMBER axis, bool flag );

   COLDATA GetXColData( void ) { return xCol; }
   void SetXCol   ( DataObj *pDataObj, UINT xColNum );
   UINT AppendYCol( DataObj *pDataObj, UINT yColNum );
   void ClearYCols( void ) { m_lineArray.Clear(); }

   void SetDataObj( DataObj*, bool makeDataLocal );

   bool AppendAxisLabel( MEMBER axis, const char * const label );

   bool IsOpen( void ) { return (bool) ! IsIconic(); }

   //-- date display --//
   bool IsShowDate   ( void )     { return dateYear >= 0; }
   void ShowXAxisDate( int year ) { dateYear = year; }

   //-- line characteristics --//
   bool EnableLine    ( int lineNo, bool enable );
   bool ShowLine      ( int lineNo, bool show );
   bool SetMarkerStyle( int lineNo, MSTYLE mStyle );
   bool SetMarkerIncr ( int lineNo, int incr  );
   bool SetMarkerColor( int lineNo, COLORREF color );
   bool ShowMarker    ( int lineNo, bool show );
   void ShowMarkers   ( bool show );
   bool SetSmooth     ( int lineNo=-1, float tension=0.5f);

   //-- force complete redraw --//
   void ResetLastRow( void ) { lastRowCount = 0; }

   void RedrawPlotArea( void );

   //-- copy all external data in a local data object --//
   bool MakeDataLocal( void );

   int GetLineCount() { return (int) m_lineArray.GetSize(); }   // line (and legend) info
   LINE *GetLineInfo( int line ) { return m_lineArray[ line ]; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(GraphWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~GraphWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(GraphWnd)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};


//-- LPtoVP() -------------------------------------------------------
//
//-- Scales a logical coordinant to a virtual coordinant.
//-------------------------------------------------------------------

inline
void GraphWnd::LPtoVP( COORD2d &point )
   {
   point.x = (xMax-xMin)
     *(point.x-m_chartOffsetLeft)/(XLOGMAX-m_chartOffsetLeft-m_chartOffsetRight )
               + xMin;

   point.y = (yMax-yMin)
      *(point.y-m_chartOffsetTop)/(YLOGMAX-m_chartOffsetTop-m_chartOffsetBottom ) 
               + yMin;
   }



class GraphDlg : public CPropertySheet
{
DECLARE_DYNAMIC(GraphDlg)

public:
   GraphDlg( GraphWnd *pParent );
   
protected:
   PPPlotSetup m_setupPage;
   PPPlotAxes  m_axesPage;
   PPPlotLines m_linePage;

public:
	//{{AFX_MSG(GraphDlg)
	afx_msg void OnApplyNow();
   virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHWND_H__02F082FC_035F_4F2D_98FF_8D801C7CD663__INCLUDED_)

