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
// ScatterWnd.cpp : implementation file
//

#include "EnvWinLibs.h"

#include <scatterwnd.h>
#include <fdataobj.h>
#include <math.h>
#include <gdiplus.h>
#include <GEOMETRY.HPP>

const int MARKERSIZE = 4;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// ScatterWnd

IMPLEMENT_DYNAMIC( ScatterWnd, GraphWnd )

ScatterWnd::ScatterWnd()
   : GraphWnd(),
     isTrajPlot( false ),
     trajLag   ( 1 ),
     lastDynamicDisplayIndex( 0 ),
     showCurrentPosBar( false ),
     drawToCurrentPosOnly( false ),
     currentPosValue( 0 ),
     eraseLastBar( false ),
     validRows( -1 ),
     autoscaleAll( false ),
     m_activated( false )
   { 
   m_toolTipContent[ 0 ] = NULL;
   }

ScatterWnd::ScatterWnd( DataObj *pData, bool makeDataLocal /*=false*/ )
   : GraphWnd( pData, makeDataLocal ),
     isTrajPlot( false ),
     trajLag   ( 1 ),
     showCurrentPosBar( false ),
     currentPosValue( 0 ),
     eraseLastBar( false ),
     validRows( -1 ),
     autoscaleAll( false ),
     m_activated( false )
   {
   m_toolTipContent[ 0 ] = NULL;
   }


ScatterWnd::~ScatterWnd()
   { }


BEGIN_MESSAGE_MAP(ScatterWnd, GraphWnd)
	ON_WM_PAINT()
   ON_WM_CREATE()
   ON_WM_SYSCOMMAND()
   ON_WM_RBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_LBUTTONDOWN()
   ON_WM_NCMOUSEMOVE()
   ON_WM_MOUSEMOVE()
   ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnTTNNeedText) 
   //ON_NOTIFY_EX(TTN_NEEDTEXTW, 0, OnTTNNeedText) 
END_MESSAGE_MAP()


BOOL ScatterWnd::OnTTNNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult) 
   {
   TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
   //UINT nID =pNMHDR->idFrom;
 
   if (pTTT->uFlags & TTF_IDISHWND)
      pTTT->lpszText = m_toolTipContent;
 
   return TRUE; 
   } 


/////////////////////////////////////////////////////////////////////////////
// ScatterWnd message handlers

int ScatterWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (GraphWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_ttPosition.x = 200;
   m_ttPosition.y = 200;
   m_toolTip.Create( this, TTS_ALWAYSTIP|TTF_TRACK|TTF_ABSOLUTE|TTF_IDISHWND );

   m_toolTip.SetDelayTime(TTDT_AUTOPOP, -1); 
   m_toolTip.SetDelayTime(TTDT_INITIAL, 0); 
   m_toolTip.SetDelayTime(TTDT_RESHOW, 0);

   TOOLINFO ti;
   ti.cbSize = TTTOOLINFOA_V2_SIZE;  //sizeof( TOOLINFO );
   ti.uFlags = TTF_ABSOLUTE | TTF_IDISHWND | TTF_TRACK;
   ti.hwnd   = this->m_hWnd;
   ti.uId    = (UINT_PTR) this->m_hWnd;
   ti.lpszText = LPSTR_TEXTCALLBACK;
   ti.lParam = 10000;  // ap defined value
   ti.lpReserved = NULL;
   GetClientRect( &ti.rect);
   m_toolTip.SendMessage(TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti); 
   //m_toolTip.Activate(TRUE);

   m_toolTip.SendMessage( TTM_SETMAXTIPWIDTH, 0, (LPARAM) SHRT_MAX );
   m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_AUTOPOP, SHRT_MAX ); // turn of autopop
   m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_INITIAL, 200 );
   m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_RESHOW, 200 );

   m_toolTip.EnableTrackingToolTips(TRUE);
   m_toolTip.EnableToolTips(TRUE);

   //m_toolTip.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti); // Activate tracking - display tooltip
   
   //*** you could change the defualt settings ***
   //m_ToolTip->SetTipBkColor(RGB(0,255,0));
   //m_ToolTip->SetTipTextColor(RGB(255,0,0))


   return 0;
   }

void ScatterWnd::MakeTrajPlot( int lag )
   {
   isTrajPlot = true;
   trajLag    = lag;

   //-- set x col.  Note: this is only to handle labels, scaling 
   //   correctly.  Data for x axis comes from y-axis line data, lagged
   SetXCol( m_lineArray[ 0 ]->pDataObj, m_lineArray[ 0 ]->dataCol );

   SetMarkerIncr ( 0, 1        );
   SetMarkerStyle( 0, MS_POINT ); 
   ShowMarker    ( 0, true     );
   ShowLine      ( 0, false    );
   }


void ScatterWnd::OnPaint() 
   {
	CPaintDC dc(this); // device context for painting

	if ( m_lineArray.GetSize() == 0 )
       return;

   if ( IsIconic() )
       return;

   lastRowCount = 0;

   InitMapping( &dc );
   SetLayout( &dc );

   DrawFrame ( &dc );
   DrawLabels( &dc );
   DrawAxes  ( &dc, autoscaleAll ? false : true );
   UpdatePlot( &dc, false, 0, validRows );    // virtual method for actually drawing data

   eraseLastBar = false;

   return;
   }



//-- ScatterWnd::UpdatePlot() ------------------------------------------
//
//   Draws actual data on the plot
//-------------------------------------------------------------------

bool ScatterWnd::UpdatePlot( CDC *pDC,  bool doInitMapping, int startIndex/*=0*/, int numPoints/*=-1*/ )
   {
   if ( IsIconic() )
      return true;

   UINT lineCount = (UINT) m_lineArray.GetSize();

   if ( lineCount == 0 )
      return false;

   HDC hDC = pDC->m_hDC;

   //-- check row count to make sure there is something to draw
   UINT rows = 0;
   UINT i = 0;

   for ( i=0; i < lineCount; i++ )
      rows += m_lineArray[ i ]->pDataObj->GetRowCount();

   if ( rows == 0 )  // no rows in any data object?
      return false;

   // there's data, so draw it for each line
   if ( doInitMapping )
      InitMapping( pDC );

   SetBkMode( hDC, TRANSPARENT );

   if ( xAxis.autoScale || yAxis.autoScale )
      {
      lastRowCount = 0;
      DrawAxes( pDC, autoscaleAll ? false : true );
      }

   //-- get plot bounding rect
   POINT lowerLeft;
   POINT upperRight;
   GetPlotBoundingRect( hDC, &lowerLeft, &upperRight );  // return LOGICAL coords

   //-- now cycle through line, drawing each one
   for( i=0; i < (UINT) m_lineArray.GetSize(); i++ )
      {
      LINE *pLine = m_lineArray[ i ];

      if ( numPoints < 0 )
         numPoints = pLine->pDataObj->GetRowCount();

      if ( drawToCurrentPosOnly  )  // note: this could probably get move outside the loop
         {
         int _numPoints = 0;
         DataObj *pDataObj = this->xCol.pDataObj;
         int colX = this->xCol.colNum;

         for ( int j=0; j < numPoints; j++ )
            {
            float x = pDataObj->GetAsFloat( colX, j );
            if ( x > currentPosValue )
               break;

            _numPoints++;
            }

         numPoints = _numPoints;
         }

      if ( pLine->m_isEnabled && ( pLine->showMarker || pLine->showLine ) )
         DrawLine( hDC, pLine, lowerLeft, upperRight, startIndex, numPoints );
      }

   //-- done with all lines, clean up
   lastRowCount = rows;

   return true;
   }


//-- for ScatterWnd::DrawLine( hDC, pLine );
//    Note:  lowerLeft, upperRight are in LOGICAL coordinants

bool ScatterWnd::DrawLine( HDC hDC, LINE *pLine, POINT &lowerLeft, POINT &upperRight, int startIndex, int numPoints )
   {
   int lineWidth = pLine->lineWidth;

   // override defaults if needed
   if ( pLine->pDataObj->GetRowCount() > 120 )
      lineWidth = 1;
   else if ( pLine->pDataObj->GetRowCount() > 60 )
      lineWidth = 2;

   HPEN hPen    = CreatePen( pLine->lineStyle, lineWidth, pLine->color );
   HPEN hOldPen = (HPEN) SelectObject( hDC, hPen );

   DataObj *pDataObj = pLine->pDataObj;
   UINT     yColNum  = pLine->dataCol;

   //-- handle auto scaling --//
   float min = yMin;
   float max = yMax;

   if ( this->autoscaleAll )
      {
      int ticks;
      float tickIncr;

      pDataObj->GetMinMax ( yColNum , &min, &max );
      GetAutoScaleParameters( min, max, ticks, tickIncr );
      }

   int rows;
   if ( numPoints < 0 )
      rows = pDataObj->GetRowCount();
   else
      {
      rows = (startIndex+1)+numPoints;
      if ( rows > pDataObj->GetRowCount() )
         rows = pDataObj->GetRowCount();
      }

   int ptCount = rows-startIndex;
   int ptIndex = 0;
   Gdiplus::Point *pts = NULL;
   
   if ( pLine->m_smooth )
      pts = new Gdiplus::Point[ ptCount ];

   // only draw rows from lastRowCount to rows...
   // each row corresponds to a data point in the plot
   for( int row = startIndex+1; row < rows; row++ )
      {
      COORD2d end;
      COORD2d start;
      POINT _start;
      POINT _end;
      bool  clipped;

      //-- get starting data values and rescale
      if ( isTrajPlot && row < trajLag+1 )
         continue;

      if ( row > 0 )
         {
         if ( isTrajPlot )
            start.x = pDataObj->GetAsFloat( yColNum, row-(trajLag+1) );
         else
            start.x = xCol.pDataObj->GetAsFloat( xCol.colNum, row-1 );

         start.y = pDataObj->GetAsFloat( yColNum, row-1 );
         VPtoLP( start, min, max );
         _start.x = (int) start.x; _start.y = (int) start.y;
         }

      //-- get ending data value and scale
      if ( isTrajPlot )
         end.x = pDataObj->GetAsFloat( yColNum, row-trajLag );
      else
         end.x = xCol.pDataObj->GetAsFloat( xCol.colNum, row );

      end.y = pDataObj->GetAsFloat( yColNum, row );

      //if ( drawToCurrentPosOnly && end.x > currentPosValue )
      //   break;

      VPtoLP( end, min, max );

      _end.x = (int) end.x;
      _end.y = (int) end.y;

      clipped = ClipLine( _start, _end, lowerLeft, upperRight );

      if ( ! clipped && pLine->showLine && row > 0 )
         {
         if ( pLine->m_smooth )
            {
            if ( row == startIndex+1 ) // first pt?
               {
               pts[ ptIndex ].X = _start.x;
               pts[ ptIndex ].Y = _start.y;
               ptIndex++;
               }

            pts[ ptIndex ].X = _end.x;
            pts[ ptIndex ].Y = _end.y;
            ptIndex++;
            }
         else
            {
            MoveToEx( hDC, _start.x, _start.y, NULL );
            LineTo( hDC, _end.x,   _end.y );
            }
         }

      if ( ( row % pLine->markerIncr == 0 ) && ( ! clipped ) && ( pLine->showMarker ) )
          {
          DrawMarker( hDC, _end.x, _end.y, pLine->markerStyle, pLine->color, MARKERSIZE );
          }

      }  // end of row for loop

   if ( pLine->m_smooth )
      {
      Gdiplus::Graphics graphics( hDC );
      Gdiplus::Color c;
      c.SetFromCOLORREF( pLine->color );
      Gdiplus::Pen pen( c, 3 );
      graphics.DrawCurve( &pen, pts, ptCount, pLine->m_tension );
      graphics.DrawBeziers( &pen, pts, ptCount );
      delete [] pts;
      }

   //-- done with this line, clean up
   SelectObject( hDC, hOldPen );
   DeleteObject( hPen );

   return true;
   }


bool ScatterWnd::ClipLine( POINT &start, POINT &end, POINT &lowerLeft, POINT &upperRight )
   {
   //-- note, all coords and points are in logical units! --//

   //-- start to the left? --//
   if ( start.x < lowerLeft.x )
      {
      if ( end.x < lowerLeft.x )     // end to the left?
         return true;

      else  // clip it
         start.x = lowerLeft.x;
      }

   //-- start to the right? --//
   else if ( start.x > upperRight.x )
      {
      if ( end.x > upperRight.x )    // end to the right?
         return true;

      else  // clip it
         start.x = upperRight.x;
      }

   //-- end to the left? --//
   if ( end.x < lowerLeft.x )
      end.x = lowerLeft.x;   // clip it

   //-- end to the right? --//
   else if ( end.x > upperRight.x )
      end.x = upperRight.x;  // clip it


   //-- repeat with vertical bounds --//
   //-- start below? --//
   if ( start.y < lowerLeft.y )
      {
      if ( end.y < lowerLeft.y )     // end below?
         return true;

      else  // clip it
         start.y = lowerLeft.y;
      }

   //-- start above? --//
   else if ( start.y > upperRight.y )
      {
      if ( end.y > upperRight.y )    // end above?
         return true;

      else  // clip it
         start.y = upperRight.y;
      }

   //-- end below? --//
   if ( end.y < lowerLeft.y )
      end.y = lowerLeft.y;   // clip it

   //-- end to the right? --//
   else if ( end.y > upperRight.y )
      end.y = upperRight.y;  // clip it

   return false;
   }


// draw the current position bar
void ScatterWnd::UpdateCurrentPos( float value )
   {
   CDC *pDC = GetDC();

   InitMapping( pDC );

   int r2 = pDC->SetROP2( R2_NOT );

   CPen pen( PS_SOLID, 5, RGB( 0, 0xFF, 0xFF ) );  // LT_CYAN, but color doesn't matter
   CPen *pOldPen = pDC->SelectObject( &pen );

   POINT lowerLeft, upperRight;
   GetPlotBoundingRect( pDC->m_hDC, &lowerLeft, &upperRight );

   // convert currentPos from virtual->logical
   COORD2d point;
   point.x = currentPosValue;
   point.y = 0;
   VPtoLP( point );

   // erase old (if it existed)
   if ( eraseLastBar )
      {
      pDC->MoveTo( (int) point.x, lowerLeft.y );
      pDC->LineTo( (int) point.x, upperRight.y );
      }
   else
      eraseLastBar = true;
      
   currentPosValue = value;
   point.x = currentPosValue;
   point.y = 0;
   VPtoLP( point );
   pDC->MoveTo( (int) point.x, lowerLeft.y );
   pDC->LineTo( (int) point.x, upperRight.y );
   
   pDC->SetROP2( r2 );
   pDC->SelectObject( pOldPen );
   ReleaseDC( pDC );
   }


void ScatterWnd::OnSysCommand(UINT nID, LPARAM lParam)
   {   
   GraphWnd::OnSysCommand(nID, lParam);
   }


void ScatterWnd::UpdateLine( float xPrevious, float yPrevious, float x, float y )
   {
   CDC *pDC = GetDC();

   InitMapping( pDC );

   int r2 = pDC->SetROP2( R2_BLACK );

   CPen pen( PS_SOLID, 2, RGB( 240, 0, 0 ) );
   CPen *pOldPen = pDC->SelectObject( &pen );

   POINT lowerLeft, upperRight;
   GetPlotBoundingRect( pDC->m_hDC, &lowerLeft, &upperRight );

   // convert currentPos from virtual->logical
   COORD2d pointCurrent;
   COORD2d pointPrevious;
   pointCurrent.x = x;
   pointCurrent.y = y;

   pointPrevious.x = xPrevious;
   pointPrevious.y = yPrevious;

   VPtoLP( pointPrevious );
   VPtoLP( pointCurrent );

   //currentPosValue = x;
   //point.x = currentPosValue;
   //point.y = y;
   //VPtoLP( point );
   pDC->MoveTo( (int) pointPrevious.x, (int) pointPrevious.y );
   pDC->LineTo( (int) pointCurrent.x, (int) pointCurrent.y );
   
   pDC->SetROP2( r2 );
   pDC->SelectObject( pOldPen );
   ReleaseDC( pDC );
   }


void CALLBACK ScatterTimerProc(
   HWND hWnd,      // handle of CWnd that called SetTimer
   UINT nMsg,      // WM_TIMER
   UINT_PTR nIDEvent,  // timer identification
   DWORD dwTime )  // system time
   {
   ScatterWnd *pScatter = (ScatterWnd*) nIDEvent;
   if ( pScatter->lastDynamicDisplayIndex > 0 )
      {
      // update the plot to the 
      HDC hDC = GetDC( hWnd );
      CDC dc;
      dc.FromHandle( hDC );
      pScatter->UpdatePlot( &dc, true, pScatter->lastDynamicDisplayIndex-1, 1 );     // NOTE: this resets lastRowCount to the number of rows in the data object
      dc.Detach();
      ReleaseDC( hWnd, hDC );

      int rows = pScatter->m_lineArray[ 0 ]->pDataObj->GetRowCount();
      if ( pScatter->lastDynamicDisplayIndex == rows )
         pScatter->lastDynamicDisplayIndex = 0;
      else
         {
         pScatter->lastDynamicDisplayIndex++;
         pScatter->lastRowCount = pScatter->lastDynamicDisplayIndex+1;
         }
      }
   else     // lastDynamicDisplayIndex == 0 
      {
      // erase plot area
      LONG_PTR hBkBr = GetClassLongPtr( hWnd, GCLP_HBRBACKGROUND );
      POINT lowerLeft, upperRight;
      HDC hDC = GetDC( hWnd );
      CDC dc;
      dc.FromHandle( hDC );
      pScatter->InitMapping( &dc );
      pScatter->GetPlotBoundingRect( hDC, &lowerLeft, &upperRight );  // return LOGICAL coords
      HGDIOBJ oldBr = SelectObject( hDC, (HGDIOBJ) hBkBr );
      Rectangle( hDC, lowerLeft.x+1, upperRight.y+1, upperRight.x-1, lowerLeft.y-1 );
      SelectObject( hDC, oldBr );
      dc.Detach();
      ReleaseDC( hWnd, hDC );
      pScatter->lastDynamicDisplayIndex = 1;
      }
   }


void ScatterWnd::OnRButtonDown(UINT nFlags, CPoint point)
   {
   GraphDlg dlg( this );
   dlg.DoModal();

   GraphWnd::OnRButtonDown(nFlags, point);
   }


void ScatterWnd::OnMouseMove(UINT nFlags, CPoint point)
   {
   //CString msg;
   bool lButtonDown = nFlags & MK_LBUTTON;

   if ( lButtonDown )
      m_dragState = 1;  // dragging

   m_ttPosition = point;
   
   CString tipText;
   bool activate = CollectTTLines( tipText );
   if ( activate )
      lstrcpyn( m_toolTipContent, tipText, 511 );

   if ( activate && ! m_activated  )
      {
      CToolInfo ti;
      m_toolTip.GetToolInfo( ti, this, (UINT_PTR) m_hWnd );
      m_toolTip.SendMessage(TTM_TRACKACTIVATE,(WPARAM)TRUE, (LPARAM) &ti );
//      m_toolTip.Activate( TRUE );
      m_activated = true;
      }
   else if ( ! activate && m_activated )
      {
      CToolInfo ti;
      m_toolTip.GetToolInfo( ti, this, (UINT_PTR) m_hWnd );
      m_toolTip.SendMessage(TTM_TRACKACTIVATE,(WPARAM)FALSE, (LPARAM) &ti );
//      m_toolTip.Activate( FALSE );
      m_activated = false;
      }

   ClientToScreen( &point );
   point.Offset( 10, -10 );
   m_toolTip.SendMessage(TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(point.x, point.y) ); // Set pos

   GraphWnd::OnMouseMove(nFlags, point);
   }


bool ScatterWnd::CollectTTLines( CString &tipText )
   {
   tipText.Empty();

   UINT lineCount = (UINT) m_lineArray.GetSize();
   if ( lineCount == 0 )
      return false;

   UINT rows = 0;
   for ( UINT i=0; i < lineCount; i++ )
      {
      if ( m_lineArray[ i ]->pDataObj != NULL )
         rows += m_lineArray[ i ]->pDataObj->GetRowCount();
      }

   if ( rows == 0 )  // no rows in any data object?
      return false;
    
   CDC *pDC = GetDC();
   HDC hDC = pDC->m_hDC;

   InitMapping( pDC );  // set up coordinant system

   //-- get plot bounding rect
   POINT lowerLeft;
   POINT upperRight;
   GetPlotBoundingRect( hDC, &lowerLeft, &upperRight );  // return LOGICAL coords

   CPoint ll( lowerLeft );
   CPoint ur( upperRight );
   pDC->LPtoDP( &ll );
   pDC->LPtoDP( &ur );

   if ( m_ttPosition.x <= ll.x || m_ttPosition.x >= ur.x || m_ttPosition.y >= ll.y || m_ttPosition.y <= ur.y )
      return false;

   int count = 0;

   // based on m_ttPosition, determine if any lines cross this.
   for ( int i=0; i < (int) lineCount; i++ )
      {
      LINE *pLine = m_lineArray[ i ];

      //-- now cycle through line, drawing each one
      for( i=0; i < (int) m_lineArray.GetSize(); i++ )
         {
         LINE *pLine = m_lineArray[ i ];
         float xValue = 0;
         float yValue = 0;
         bool intersect = IsPointOnDataPoint( m_ttPosition, pDC, pLine, lowerLeft, upperRight, xValue, yValue ); 
         if ( intersect )
            {
            count++;
            CString tip;
            TCHAR fmt[ 32 ];
            TCHAR *p = fmt;
            
            if ( count > 1 )
               {
               *fmt = '\n';
               p = fmt + 1;
               }

            if ( yValue > 100000 || yValue < -100000 )
               lstrcpy( p, "%s: %.2f" );
            else
               lstrcpy( p, "%s: %g" );

            tip.Format( fmt, (LPCTSTR) pLine->text, yValue );
            
            tipText += tip;
            }
         }
      }

   return ( count > 0 ) ? true : false;
   }

      
bool ScatterWnd::IsPointOnDataPoint( CPoint &point, CDC *pDC, LINE *pLine, POINT &lowerLeft, POINT &upperRight, float &xValue, float &yValue )
   {
   if ( isTrajPlot )
      return false;

   DataObj *pDataObj = pLine->pDataObj;
   UINT     yColNum  = pLine->dataCol;

   //-- handle auto scaling --//
   float minVal = yMin;
   float maxVal = yMax;

   if ( this->autoscaleAll )
      {
      int ticks;
      float tickIncr;

      pDataObj->GetMinMax( yColNum , &minVal, &maxVal );
      GetAutoScaleParameters( minVal, maxVal, ticks, tickIncr );
      }

   int rows = pDataObj->GetRowCount();
   int ptIndex = 0;

   // each row corresponds to a data point in the plot
   for( int row=0; row < rows-1; row++ )
      {
      COORD2d dataPoint0;
      COORD2d dataPoint1;
      POINT   dataPoints[2];
      
      if ( isTrajPlot )
         {
         dataPoint0.x = pDataObj->GetAsFloat( yColNum, row-(trajLag+1) );  //????
         dataPoint1.x = pDataObj->GetAsFloat( yColNum, row+1-(trajLag+1) );  //????
         }
      else
         {
         dataPoint0.x = xCol.pDataObj->GetAsFloat( xCol.colNum, row );
         dataPoint1.x = xCol.pDataObj->GetAsFloat( xCol.colNum, row+1 );
         }

      dataPoint0.y = pDataObj->GetAsFloat( yColNum, row );
      dataPoint1.y = pDataObj->GetAsFloat( yColNum, row+1 );

      COORD2d dataPointLP0( dataPoint0 );
      COORD2d dataPointLP1( dataPoint1 );
      
      //xValue = dataPoint.x;
      //yValue = dataPoint.y;

      VPtoLP( dataPointLP0, minVal, maxVal );
      VPtoLP( dataPointLP1, minVal, maxVal );
      
      // store the logical coordinates and convert to client
      dataPoints[0].x = (int) dataPointLP0.x;
      dataPoints[0].y = (int) dataPointLP0.y;

      dataPoints[1].x = (int) dataPointLP1.x;
      dataPoints[1].y = (int) dataPointLP1.y;
      
      pDC->LPtoDP( &dataPoints[0] );
      pDC->LPtoDP( &dataPoints[1] );

      // now, _dataPoint is in client coords, look for intersections with line segments
      if (::IsPointOnLine( dataPoints, 2, point, 3 ) )
         {
         // find x, y, values
         CPoint _point( point );  // client of mouse position
         pDC->DPtoLP( &_point );  // to logical

         COORD2d value( (REAL) _point.x, (REAL) _point.y );
         LPtoVP( value );    // to virtual - this gives us X, but a better y is from the intersection point

         xValue = (float) value.x;  // virtual

         // get intersection point; start with logical coords
         if ( xValue == dataPoint0.x )
            yValue = (float) dataPoint0.y;
         else
            {
            float slope = float( (dataPoint1.y - dataPoint0.y)/(dataPoint1.x - dataPoint0.x) );
            yValue = (float)(dataPoint0.y + slope * ( xValue-dataPoint0.x ));
            }

         return true;
         }

      //if ( abs( _dataPoint.x - point.x ) < 5 && abs( _dataPoint.y - point.y ) < 5 )
      //   return true;
      }

   return false;
   }


void ScatterWnd::OnLButtonDown(UINT nFlags, CPoint point)
   { 
   /*
   // if clicking on legend, toggle the legend on/off
   m_startDragPt = point;
   m_dragState = 0;   // not dragging (set in OnMouseMove() )

   CDC *pDC = GetDC();

   InitMapping( pDC );

   CPoint lp( point );

   pDC->DPtoLP( &lp );

   CRect rect;

   // click on legend?
   bool redraw = false;
   for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
      {
      LINE *pLine = m_lineArray[ i ];

      if ( ( pLine->rect.left   < lp.x  && lp.x < pLine->rect.right )
         &&( pLine->rect.bottom < lp.y  && lp.y < pLine->rect.top ) )
         {
         pLine->m_isEnabled = !pLine->m_isEnabled;
         rect = pLine->rect;         
         redraw = true;
         break;
         }
      }

   if ( redraw )
      RedrawWindow();
*/
   GraphWnd::OnLButtonDown(nFlags, point);
   }


void ScatterWnd::OnLButtonUp(UINT nFlags, CPoint point)
   {
   CDC *pDC = GetDC();

   InitMapping( pDC );

   CPoint lp( point );

   pDC->DPtoLP( &lp );

   // are we completing a drag?   
   if ( m_dragState == 1 ) 
      {
      // check if we dragged over legend
      }


   CRect rect;


/*   // click on legend?
   bool redraw = false;
   for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
      {
      LINE *pLine = m_lineArray[ i ];

      if ( ( pLine->rect.left   < lp.x  && lp.x < pLine->rect.right )
         &&( pLine->rect.bottom < lp.y  && lp.y < pLine->rect.top ) )
         {
         pLine->m_isEnabled = !pLine->m_isEnabled;
         rect = pLine->rect;         
         redraw = true;
         break;
         }
      }

   if ( redraw )
      RedrawWindow();
*/

   m_dragState = 0;   // not dragging (set in OnMouseMove() )

   GraphWnd::OnLButtonUp(nFlags, point);
   }

