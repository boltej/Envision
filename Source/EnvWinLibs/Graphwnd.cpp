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
// graphwnd.cpp : implementation file
//

#include "EnvWinLibs.h"


#ifdef _WINDOWS

#include "graphwnd.h"
#include <scale.h>
#include <date.hpp>
#include <fdataobj.h>
#include <datatable.h>
#include <strarray.hpp>

#include <cwndmngr.h>
#include <dibapi.h>

#include <math.h>
#include <stdlib.h>
#include <graphwnd.h>
#include <colors.hpp>
#include <GEOMETRY.HPP>


  WINLIBSAPI   COLORREF gColorRef[];

//static bool  dragging   = false;
//static POINT startPos   = { 0, 0 };
//static POINT lastPos    = { 0, 0 };
static bool  isPrinting = false;    // turns off initmapping when true


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// GraphWnd

IMPLEMENT_DYNAMIC( GraphWnd, CWnd )

GraphWnd::GraphWnd()
: CWnd()
, m_lineArray()
, xAxisLabelArray()
, yAxisLabelArray()
, pTableMngr    ( NULL )
, frameStyle    ( FS_SUNKEN )
, showXGrid     ( true )
, showYGrid     ( true )
, pLocDataObj   ( NULL )
, lastRowCount  ( 0 )
, allowZoom     ( false )
, dateYear      ( -1 )
, start         ( -1 )
, end           ( -1 )
, m_chartOffsetLeft  ( 300 )
, m_chartOffsetRight ( 500 )
, m_chartOffsetTop   ( 300 )
, m_chartOffsetBottom( 200 )
, m_legendFontHeight( 100 )
, m_legendRowHeight( 60 )
, m_labelHeight ( 80 )    //   = YDATAOFFSET/4;
, m_titleHeight ( 100 )    // TITLE_HEIGHT   = YDATAOFFSET/3;
, m_legendRect  ()
, m_chartRect   ()
, m_dragState   ( 0 )      // 0= not dragging, 1= dragging mouse
, m_startDragPt () 
, m_isLegendFrameVisible( false )
   {
   Initialize();
   }

GraphWnd::GraphWnd( DataObj *pDataObj, bool makeDataLocal /*=false*/ )
: CWnd()
, m_lineArray()
, xAxisLabelArray()
, yAxisLabelArray()
, pTableMngr    ( NULL )
, frameStyle    ( FS_SUNKEN )
, showXGrid     ( true )
, showYGrid     ( true )
, pLocDataObj   ( NULL )
, lastRowCount  ( 0 )
, allowZoom     ( false )
, dateYear      ( -1 )
, start         ( -1 )
, end           ( -1 )
, m_chartOffsetLeft  ( 300 )
, m_chartOffsetRight ( 500 )
, m_chartOffsetTop   ( 300 )
, m_chartOffsetBottom( 200 )
, m_legendFontHeight( 100 )
, m_legendRowHeight( 60 )
, m_labelHeight ( 80 )    //   = YDATAOFFSET/4;
, m_titleHeight ( 100 )    // TITLE_HEIGHT   = YDATAOFFSET/3;
, m_legendRect  ()
, m_chartRect   ()
, m_dragState   ( 0 )      // 0= not dragging, 1= dragging mouse
, m_startDragPt () 
, m_isLegendFrameVisible( false )
   {
   Initialize();
   ASSERT( pDataObj != NULL );

   SetDataObj( pDataObj, makeDataLocal );
   }


void GraphWnd::SetDataObj( DataObj* pDataObj, bool makeDataLocal )
   {   
   SetXCol( pDataObj, 0 );
   for ( int i=1; i <= pDataObj->GetColCount()-1; i++ )
       AppendYCol( pDataObj, i );

   if ( makeDataLocal )
      MakeDataLocal();
   }


GraphWnd::~GraphWnd()
   {
   if ( pTableMngr )
      delete pTableMngr;

   if ( pLocDataObj )
      delete pLocDataObj;
   }


BEGIN_MESSAGE_MAP(GraphWnd, CWnd)
   ON_WM_PAINT()
   ON_WM_SYSCOMMAND()
   ON_WM_CREATE()
   ON_WM_ERASEBKGND()
   ON_WM_LBUTTONUP()
   ON_WM_LBUTTONDOWN()
   ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// GraphWnd message handlers

int GraphWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   BuildSysMenu();

   return 0;
   }


bool GraphWnd::Initialize( void )
   {
   //--- Set default values ---//
   xMin  = yMin = 0.0;             // set default min's, max's
   xMax  = yMax = 100.0;
   xCol.pDataObj = NULL;           // set default column for x-data
   lastRowCount = 0;

   //-- initialize logFont --//
   m_logFont.lfHeight         = 0;                   // character height
   m_logFont.lfWidth          = 0;                   // average width
   m_logFont.lfEscapement     = 0;                   // text angle
   m_logFont.lfOrientation    = 0;                   // individual character angle
   m_logFont.lfWeight         = 700;                 // average pixels/1000
   m_logFont.lfItalic         = false;               // flag != 0 if italic
   m_logFont.lfUnderline      = false;               // flag != 0 if underline
   m_logFont.lfStrikeOut      = false;               // flag != 0 if strikeout
   m_logFont.lfCharSet        = ANSI_CHARSET;        // character set: ANSI, OEM
   m_logFont.lfOutPrecision   = OUT_STROKE_PRECIS;   // mapping percision-unused
   m_logFont.lfClipPrecision  = CLIP_STROKE_PRECIS;  // clip percision-unused
   m_logFont.lfQuality        = PROOF_QUALITY;       // draft or proof quality
   m_logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS; // flags for font style
   lstrcpy((LPTSTR) m_logFont.lfFaceName, "Arial");   // typeface name

   //-- initialize Legends --//
   //-- first, set legend rects (logical coords, 1280x1280) --//
   int xLeft  = XLOGMAX-m_chartOffsetRight + 10;
   int xRight = XLOGMAX;

   //-- initialize labels rects --//
   int yBottom = YLOGMAX - m_chartOffsetBottom + 5;
   int yTop    = yBottom + m_labelHeight;

   //-- title --//
   SetRect( (LPRECT) &label[ TITLE ].rect, 0, YLOGMAX-m_labelHeight*3/2, XLOGMAX, YLOGMAX-m_labelHeight/2 ); // left, top, right, bottom

   //-- xAxis --//
   xLeft   = m_chartOffsetLeft;
   xRight  = XLOGMAX - m_chartOffsetRight;
   yBottom = m_chartOffsetBottom - 10 - m_labelHeight;
   yTop    = yBottom + m_labelHeight;
   SetRect( (LPRECT) &label[ XAXIS ].rect, xLeft, yBottom, xRight, yTop );

   //-- yAxis --//
   xLeft   = m_chartOffsetLeft - 10 - m_labelHeight;
   xRight  = xLeft + m_labelHeight;
   yBottom = m_chartOffsetBottom;
   yTop    = YLOGMAX -m_chartOffsetTop;
   SetRect( (LPRECT) &label[ YAXIS ].rect,  xLeft, yBottom, xRight, yTop );

   //-- initialize label text --//
   SetLabelText( TITLE, "" );
   SetLabelText( XAXIS, "" );
   SetLabelText( YAXIS, "" );

   //-- initialize label orientation --//
   //label[ TITLE ].orientation = OR_HORZ;
   //label[ XAXIS ].orientation = OR_HORZ;
   //label[ YAXIS ].orientation = OR_VERT;

   //-- initialize label color --//
   label[ TITLE ].color = BLACK;
   label[ XAXIS ].color = BLACK;
   label[ YAXIS ].color = BLACK;

   memcpy( &(label[ TITLE ].logFont), &m_logFont, sizeof( LOGFONT ) );
   memcpy( &(label[ XAXIS ].logFont), &m_logFont, sizeof( LOGFONT ) );
   memcpy( &(label[ YAXIS ].logFont), &m_logFont, sizeof( LOGFONT ) );

   //-- initialize axes --//
   xAxis.digits = 1;    xAxis.major = 1;   xAxis.minor = 0.5;
   yAxis.digits = 1;    yAxis.major = 1;   yAxis.minor = 0.5;

   return true;
   }



bool GraphWnd::BuildSysMenu( void )
   {
   //-- add "Zoom Out" to system menu --//
   HMENU hSysMenu = ::GetSystemMenu( m_hWnd, 0 );

   if ( hSysMenu )
      {
      AppendMenu( hSysMenu, MF_SEPARATOR, NULL, NULL );
      AppendMenu( hSysMenu, MF_STRING, 102, "&Table" );
      AppendMenu( hSysMenu, MF_STRING, 100, "&Print" );
      AppendMenu( hSysMenu, MF_SEPARATOR, NULL, NULL );
      AppendMenu( hSysMenu, MF_STRING, 101, "&Zoom Out" );
      DrawMenuBar();
      return true;
      }

   return false;
   }



void GraphWnd::OnSysCommand(UINT nID, LPARAM lParam)
   {
   switch( nID )
      {
      case 100:   // print
         { /*
         isPrinting = true;
         UINT _frameStyle = frameStyle; frameStyle = FS_NORMAL;

         Printer printer( hWnd );
         HDC hDC = printer.CreateDC( false );
         printer.SetMapping( hDC, MM_ANISOTROPIC, XLOGMAX, -YLOGMAX );
         printer.SetMargin( LEFT,   40 );
         printer.SetMargin( RIGHT,  40 );
         printer.SetMargin( TOP,    40 );
         printer.SetMargin( BOTTOM, 40 );
         printer.StartDoc();
         Paint( hDC );
         printer.NewFrame();
         printer.EndDoc();
         printer.DeleteDC();

         frameStyle = _frameStyle;
         isPrinting = false; */
         }
         break;

      case 101:     // zoom out
         SetExtents( XAXIS, 0, 0 );
         SetExtents( YAXIS, 0, 0 );
         RedrawWindow();
         break;

      case 102:   // table
         {
         int lineCount = (int) m_lineArray.GetSize();

         for ( int i=0; i < lineCount; i++ )
            {
            DataTable *pDataTable = NULL;
            DataObj *pDataObj = m_lineArray[ i ]->pDataObj;

            // if this is first occurance of this dataobj, show table
            bool newDataObj = true;
            for ( int j=0; j < i; j++ )
               {
               //-- has it occured before?  then skip this one --//
               if ( m_lineArray[ j ]->pDataObj == pDataObj )
                  {
                  newDataObj = false;
                  break;
                  }
               }

            if ( newDataObj )
               {
               int offset = 0;
               if ( pTableMngr )
                  offset = int( pTableMngr->GetSize() ) * 30;

               pDataTable = new DataTable( pDataObj );
               CRect rect( 100+offset, 100+offset, 400+offset, 400+offset );
               pDataTable->Create( pDataObj->GetName(), rect, GetParent(), 9000+offset ); 

               if ( pTableMngr == NULL )
                  pTableMngr = new CWndManager;

               pTableMngr->Add( pDataTable );
               }
            }  // end of: for ( i < lineCount )
         }  // end of:  if ( wParam == 102 )  // "table " selected
           break;
      }
   
   CWnd::OnSysCommand(nID, lParam);
   }

void GraphWnd::OnPaint() 
{
   CPaintDC dc(this); // device context for painting
   
   InitMapping ( &dc );
   SetLayout   ( &dc );
   DrawLabels  ( &dc );
   DrawAxes    ( &dc );
   DrawPlotArea( &dc );
   }


void GraphWnd::SetLayout( CDC *pDC )
   {
   // first, do we need a title?
   bool showTitle = true;
   if ( label[ TITLE ].text.IsEmpty() || m_titleHeight <= 0 )
      showTitle = false;

   CSize size;
   m_legendFontHeight = GetIdealLegendSize( pDC, size );

   int actualWidth = size.cx;
   m_legendRowHeight = size.cy;

   int lineCount = (int) m_lineArray.GetSize();

   // size chart offsets
   m_chartOffsetTop = showTitle ? m_titleHeight + 4*BORDERSIZE : 4*BORDERSIZE;

   // m_chartOffsetLeft no change
   //m_chartOffsetBottom no change
   m_chartOffsetBottom = 200;

   if ( size.cx > XLOGMAX / 2 )  // legend can ever take more than half the window
      size.cx = XLOGMAX / 2;

   m_chartOffsetRight = size.cx + 2 * BORDERSIZE;

   // make chart rect
   m_chartRect.left   = m_chartOffsetLeft;
   m_chartRect.right  = XLOGMAX - m_chartOffsetRight;
   m_chartRect.top    = YLOGMAX - m_chartOffsetTop; 
   m_chartRect.bottom = m_chartOffsetBottom + m_legendRowHeight/2;
   
   // size legend
   m_legendRect.left   = XLOGMAX - m_chartOffsetRight + BORDERSIZE;
   m_legendRect.right  = m_legendRect.left + actualWidth;
   m_legendRect.top    = YLOGMAX/2 + ( lineCount*m_legendRowHeight )/2;
   m_legendRect.bottom = m_legendRect.top - lineCount*m_legendRowHeight;

   m_legendRect.bottom -= m_legendRowHeight;


   ////
   //m_legendRect.MoveToXY( 0, YLOGMAX );
   //


   // lines next
   for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
      {
      LINE *pLine = m_lineArray[ i ];

      pLine->rect.top    = m_legendRect.top - i*m_legendRowHeight;
      pLine->rect.bottom = pLine->rect.top - m_legendRowHeight;
      pLine->rect.left   = m_legendRect.left;
      pLine->rect.right  = m_legendRect.right;
      }
   }


/*

LRESULT Graph::LButtonDblClkHandler( WPARAM, LPARAM lParam )
   {
#if !defined _MFC

   //-- convert mouse position to logical coord --//
   POINT click;
#if defined __FLAT__
   POINTS pos = MAKEPOINTS( lParam );
   POINTSTOPOINT( click, pos );
#else
   click = MAKEPOINT( lParam );
#endif
   HDC hDC     = ::GetDC( hWnd );
   InitMapping( hDC );
   DPtoLP( hDC, (LPPOINT) &click, 1 );
   ::ReleaseDC( hWnd, hDC );

   //-- is it in the left margin? --//
   if ( ( click.x < m_chartOffsetLeft ) && ( click.y > YDATAOFFSET )
     && ( click.y < ( YLOGMAX-YDATAOFFSET ) ) )
      {
      GraphAxisDlg dlg( this, YAXIS );
      dlg.Run( hWnd );
      return true;
      }

   //-- bottom margin --//
   else if ( ( click.y < YDATAOFFSET ) && ( click.x > m_chartOffsetLeft )
          && ( click.x < ( XLOGMAX-m_chartOffsetRight ) ) )
      {
      GraphAxisDlg dlg( this, XAXIS );
      dlg.Run( hWnd );
      return true;
      }

   //-- on a legend? --//
   int lineNo =  IsPointOnLegend( click );
   if ( lineNo >= 0 )
      {
      GraphLineDlg dlg( this, lineNo );
      dlg.Run( hWnd );
      return true;
      }

   //-- in the plot region? --//
   else if ( ( click.y > YDATAOFFSET )
          && ( click.y < ( YLOGMAX-YDATAOFFSET ) )
          && ( click.x > m_chartOffsetLeft )
          && ( click.x < ( XLOGMAX-m_chartOffsetRight ) ) )
      {
      GraphPlotDlg dlg( this );
      dlg.Run( hWnd );
      return true;
      }
#endif  // !defined _MFC
   return true;
   }


LRESULT Graph::LButtonUpHandler( WPARAM, LPARAM lParam )
   {
#if !defined _MFC

   if ( allowZoom == false )
      return false;

   //-- convert mouse position to logical coord --//
   POINT pos;
#if defined __FLAT__
   POINTS poss = MAKEPOINTS( lParam );
   POINTSTOPOINT( pos, poss );
#else
   pos = MAKEPOINT( lParam );
#endif

   //-- is the mouse been dragged more than the tolerance? --//
   if ( ( abs( pos.x-startPos.x ) < 4 ) && ( abs( pos.y-startPos.y ) < 4 ) )
      return false;

   HDC hDC = GetDC( hWnd );
   InitMapping( hDC );
   DPtoLP( hDC, (LPPOINT) &pos, 1 );     // convert both points to LOGICAL
   DPtoLP( hDC, (LPPOINT) &startPos, 1 );
   ReleaseDC( hWnd, hDC );

   //-- is at least one point in the plotting window? --//
   if ( ( ( m_chartOffsetLeft <= pos.x )
       && ( pos.x <= XLOGMAX-m_chartOffsetRight )
       && ( YDATAOFFSET <= pos.y )
       && ( pos.y <= YLOGMAX-YDATAOFFSET ) )
     ||
        ( ( m_chartOffsetLeft <= startPos.x )
       && ( startPos.x  <= XLOGMAX-m_chartOffsetRight )
       && ( YDATAOFFSET <= startPos.y )
       && ( startPos.y <= YLOGMAX-YDATAOFFSET ) ) )
      ;
   else
      return false;

   //-- at least one point in plot region, continue --//

   //-- clip points --//
   if ( pos.x < m_chartOffsetLeft            ) pos.x = m_chartOffsetLeft;
   if ( pos.x > XLOGMAX - m_chartOffsetRight ) pos.x = XLOGMAX - m_chartOffsetRight;

   if ( pos.y < YDATAOFFSET           ) pos.y = YDATAOFFSET;
   if ( pos.y > YLOGMAX - YDATAOFFSET ) pos.y = YLOGMAX - YDATAOFFSET;

   if ( startPos.x < m_chartOffsetLeft            ) startPos.x = m_chartOffsetLeft;
   if ( startPos.x > XLOGMAX - m_chartOffsetRight ) startPos.x = XLOGMAX - m_chartOffsetRight;

   if ( startPos.y < YDATAOFFSET           ) startPos.y = YDATAOFFSET;
   if ( startPos.y > YLOGMAX - YDATAOFFSET ) startPos.y = YLOGMAX - YDATAOFFSET;

   //-- calculate zoom rectangle in virtual units --//
   COORD2d _pos;
   _pos.x = (float) pos.x;
   _pos.y = (float) pos.y;
   LPtoVP( _pos );

   COORD2d _startPos;
   _startPos.x = (float)startPos.x;
   _startPos.y = (float)startPos.y;
   LPtoVP( _startPos );

   //-- rescale plot --//
   SetExtents( XAXIS, min( _pos.x, _startPos.x ), max( _pos.x, _startPos.x ) );
   SetExtents( YAXIS, min( _pos.y, _startPos.y ), max( _pos.y, _startPos.y ) );
   Repaint();

#endif  // !defined _MFC
   return true;
   }


LRESULT Graph::MouseMoveHandler( WPARAM, LPARAM lParam )
   {
#if !defined _MFC
   if ( dragging && allowZoom )
      {
      POINT pos;
#if defined __FLAT__
      POINTS poss = MAKEPOINTS( lParam );
      POINTSTOPOINT( pos, poss );
#else
      pos = MAKEPOINT( lParam );
#endif

      if ( startPos.x != pos.x && startPos.y != pos.y )
         {
         //-- erase last rect --//
         HDC hDC = ::GetDC( hWnd );

         SetROP2( hDC, R2_NOT );
         SelectObject( hDC, GetStockObject( NULL_BRUSH ) );

         Rectangle( hDC, startPos.x, startPos.y, lastPos.x, lastPos.y );

         //-- draw new one --//
         Rectangle( hDC, startPos.x, startPos.y, pos.x, pos.y );

#if defined __FLAT__
         POINTS pos = MAKEPOINTS( lParam );
         POINTSTOPOINT( lastPos, pos );
#else
         lastPos = MAKEPOINT( lParam );
#endif
         SetCursor( LoadCursor( ghSysResDLL, "C_ZOOMIN" ) );

         ::ReleaseDC( hWnd, hDC );
         }
      else
         lastPos = startPos;
      }
#endif  // !defined _MFC
   return true;
   }

*/

void GraphWnd::OnLButtonDown(UINT nFlags, CPoint point)
   {
   // if clicking on legend, toggle the legend on/off
   m_startDragPt = point;
   m_dragState = 0;   // not dragging (set in OnMouseMove() )

   CDC *pDC = GetDC();

   InitMapping( pDC );

   CPoint lp( point );

   pDC->DPtoLP( &lp );

   // click on legend?
   bool redraw = false;
   for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
      {
      LINE *pLine = m_lineArray[ i ];

      if ( ( pLine->rect.left   <= lp.x  && lp.x <= pLine->rect.right )
         &&( pLine->rect.bottom <= lp.y  && lp.y <= pLine->rect.top ) )
         {
         pLine->m_isEnabled = !pLine->m_isEnabled;
         redraw = true;
         break;
         }
      }

   if ( redraw )
      RedrawWindow();
   }


void GraphWnd::OnLButtonUp(UINT nFlags, CPoint point)
   {
   CDC *pDC = GetDC();
   InitMapping( pDC );

   CPoint lp( point );   // note: point is in window coords, convert to screen coords
   this->ScreenToClient( &lp );

   pDC->DPtoLP( &lp );

   switch( m_dragState )
      {
      case 0:   // not dragging
         {
         if ( ::IsPointOnRect( m_legendRect, lp, 10 ) )
            {
            if ( m_isLegendFrameVisible )
               {
               DrawRubberRect( pDC, m_legendRect, 6 );
               m_isLegendFrameVisible = false;
               }
            }
         }
         break;

      case 1:  // dragging    
         {
         // check if we dragged over legend
         }
         break;
      }

   m_dragState = 0;   // not dragging (set in OnMouseMove() )
   }


void GraphWnd::OnMouseMove(UINT nFlags, CPoint point)
   {
   //CString msg;
   bool lButtonDown = nFlags & MK_LBUTTON;

   if ( lButtonDown )
      m_dragState = 1;  // dragging

   ScreenToClient( &point );

   CDC *pDC = GetDC();
   InitMapping( pDC );

   CPoint lp( point );
   pDC->DPtoLP( &lp );

   if ( m_dragState == 0 )   // not dragging
      {
      bool onLegendFrame = ::IsPointOnRect( m_legendRect, lp, 10 );

      if (  onLegendFrame && m_isLegendFrameVisible == false )
         {
         DrawRubberRect( pDC, m_legendRect, 6 );
         m_isLegendFrameVisible = true;
         }
      else if ( ! onLegendFrame && m_isLegendFrameVisible )
         {
         DrawRubberRect( pDC, m_legendRect, 6 );  // erase line
         m_isLegendFrameVisible = false;
         }
      }
   }


int GraphWnd::GetLabelText( MEMBER _label, LPSTR titleText, int maxCount )
   {
   strncpy_s( titleText, maxCount+1, label[ _label ].text, maxCount );
   titleText[ strlen( (LPCTSTR) label[ _label ].text ) ] = '\0';

   return lstrlen( titleText );
   }


void GraphWnd::SetXCol( DataObj *pDataObj, UINT xColNum )
   {
   xCol.pDataObj = pDataObj;
   xCol.colNum   = xColNum;
   SetLabelText( XAXIS, (char*) pDataObj->GetLabel( xColNum ) );
   }


UINT GraphWnd::AppendYCol( DataObj *pDataObj, UINT yColNum )
   {
   LINE *pLine = new LINE;

   char *pLabel = (char*) pDataObj->GetLabel( yColNum );

   //-- Set line text --//
   ASSERT( pLabel != NULL );
   pLine->text = pLabel;

   pLine->pDataObj = pDataObj;
   pLine->dataCol  = yColNum;

   int markerCount = MS_END+1;

   
   //-- rect --//
   int offset = (int) m_lineArray.GetSize(); // initially 0

   //pLine->orientation = OR_HORZ;
   pLine->color       = gColorRef[ offset % 16 ];   // see colors.hpp for a listing of
   pLine->showLine    = true;
   pLine->showMarker  = true;
   pLine->markerStyle = (MSTYLE) ( (2*offset) % markerCount );
   pLine->lineStyle   = PS_SOLID;
   pLine->lineWidth   = 10;
   pLine->markerIncr  = 4;
   pLine->scaleMin = pLine->scaleMax = 0.0;

   return (UINT) m_lineArray.Add( pLine );    // add column to plot
   }


void GraphWnd::InitMapping( CDC *pDC )
   {
   if ( isPrinting )
      return;

   RECT  rect;
   pDC->SetMapMode( MM_ANISOTROPIC );

   //-- set viewport mapping --//
   GetClientRect ( &rect );

   //-- set and flip logical coordinate system --//
   pDC->SetWindowExt  ( XLOGMAX, -YLOGMAX );
   pDC->SetViewportExt( rect.right, rect.bottom );
   pDC->SetViewportOrg( rect.left, rect.bottom );
   pDC->SetWindowOrg  ( 0, 0 );
   }

void GraphWnd::RefreshPlotArea( void )
   {
   CDC *pDC = GetDC();

   InitMapping( pDC );
   DrawPlotArea( pDC );

   ReleaseDC( pDC );
   }


void GraphWnd::DrawTitle( CDC *pDC )
   {
   if ( ! label[ TITLE ].text.IsEmpty() )
      {
      pDC->SetBkMode( TRANSPARENT );
      pDC->SetTextAlign( TA_CENTER | TA_TOP );
      
      label[ TITLE ].logFont.lfEscapement  = OR_HORZ;
      label[ TITLE ].logFont.lfOrientation = OR_HORZ;
      label[ TITLE ].logFont.lfHeight = m_titleHeight;    // character height
      label[ TITLE ].logFont.lfWidth  = 0;              // average width (use default)

      HFONT hFontNew = CreateFontIndirect( (LOGFONT FAR *) &(label[ TITLE ].logFont) );
      HFONT hFontOld = (HFONT) ::SelectObject( pDC->m_hDC, hFontNew );

      pDC->SetTextColor( label[ TITLE ].color );

      int xMidPt = ( label[TITLE].rect.left + label[TITLE].rect.right ) / 2;

      pDC->TextOut( xMidPt, label[TITLE].rect.top, (LPCTSTR) label[TITLE].text, label[ TITLE ].text.GetLength() );
      
      SelectObject( pDC->m_hDC, hFontOld );
      DeleteObject( hFontNew );
      }
   }


//-- GraphWnd::DrawLabels() --------------------------------------------
//
//  Draws all labels on the graph.   hDC must have been InitMapped().
//-------------------------------------------------------------------

void GraphWnd::DrawLabels( CDC *pDC )
   {
   DrawTitle( pDC );

   HFONT hFontNew;
   HFONT hFontOld;

   pDC->SetBkMode( TRANSPARENT );
   pDC->SetTextAlign( TA_CENTER | TA_TOP );

   //-- Draw the titles if defined
   int i = 0;

   for ( i=XAXIS; i <= YAXIS; i++ )
      {
      if ( ! label[ i ].text.IsEmpty() )
         {
         int orientation = OR_HORZ;
         if ( i == YAXIS )
            orientation = OR_VERT;

         label[ i ].logFont.lfEscapement  = orientation;
         label[ i ].logFont.lfOrientation = orientation;
         label[ i ].logFont.lfHeight = m_labelHeight;    // character height
         label[ i ].logFont.lfWidth  = 0;              // average width (use default)
         label[ i ].logFont.lfItalic = 1;
         label[ i ].logFont.lfStrikeOut = 0;
         label[ i ].logFont.lfUnderline = 0;

         hFontNew = CreateFontIndirect( (LOGFONT FAR *) &(label[ i ].logFont ));
         hFontOld = (HFONT) pDC->SelectObject( hFontNew );

         pDC->SetTextColor( label[ i ].color );

         if ( orientation == OR_HORZ )
            {
            int xMidPt = ( label[i].rect.left + label[i].rect.right ) / 2;
            pDC->TextOut( xMidPt, label[i].rect.top, (LPCTSTR) label[i].text, label[i].text.GetLength() );
            }

         else
            {
            int yMidPt = ( label[i].rect.top + label[i].rect.bottom ) / 2;
            pDC->TextOut( label[i].rect.left-40,  yMidPt, (LPCTSTR) label[i].text, label[i].text.GetLength() );
            }

         pDC->SelectObject( hFontOld );
         DeleteObject( hFontNew );
         }
      }

   LOGFONT legendFont;
   InitializeFont( legendFont );
   legendFont.lfHeight = m_legendFontHeight; 

   pDC->SetBkMode( TRANSPARENT );
   pDC->SetTextAlign( TA_LEFT | TA_TOP );

   // set up legend logfont
   hFontNew = CreateFontIndirect( (LOGFONT FAR *) &legendFont );
   hFontOld = (HFONT) pDC->SelectObject( hFontNew );

   legendFont.lfItalic = 1;
   legendFont.lfWeight = 300;
   HFONT hFontItalic = CreateFontIndirect( (LOGFONT FAR *) &legendFont );

   for ( i=0; i < (int) m_lineArray.GetSize(); i++ )
      {
      LINE *pLine = m_lineArray[ i ];

      if ( ! pLine->text.IsEmpty() && ( pLine->showLine || pLine->showMarker ) )
         {
         // draw line
         HPEN hPen = CreatePen( pLine->lineStyle, pLine->lineWidth, pLine->color );
         HPEN hOldPen = (HPEN) pDC->SelectObject( hPen );
         HBRUSH hBrush = CreateSolidBrush( pLine->color );
         HBRUSH hOldBrush = (HBRUSH) pDC->SelectObject( hBrush );

         int y = pLine->rect.top-m_legendRowHeight/2;
         pDC->MoveTo( pLine->rect.left, y );
         pDC->LineTo( pLine->rect.left + m_legendRowHeight, y );

         // draw marker
         if ( pLine->showMarker )
            DrawMarker ( pDC->m_hDC, pLine->rect.left + m_legendRowHeight/2, y,
                  pLine->markerStyle, pLine->color, m_legendRowHeight/10 );

         pDC->SelectObject( hOldPen );
         DeleteObject( hPen );
         pDC->SelectObject( hOldBrush );
         DeleteObject( hBrush );

         HFONT hStartFont;
         if ( pLine->m_isEnabled )
            hStartFont = (HFONT) ::SelectObject( pDC->m_hDC, hFontNew );
         else
            hStartFont = (HFONT)::SelectObject( pDC->m_hDC, hFontItalic );
         
         // draw line label
         pDC->SetTextColor( pLine->color );
         
         pDC->TextOut( pLine->rect.left+m_legendRowHeight+10, pLine->rect.top,
               (LPCTSTR) pLine->text, pLine->text.GetLength() );
         
         pDC->SelectObject( hStartFont );
         }
      }

   CPen pen;
   pen.CreatePen( PS_SOLID, 10, LTGRAY );
   CPen *pOldPen = pDC->SelectObject( &pen );
   //pDC->Rectangle( &m_legendRect );
   pDC->SelectObject( pOldPen );

   pDC->SelectObject( hFontOld );
   DeleteObject( hFontNew );
   DeleteObject( hFontItalic );
   }



//-- Graph::DrawAxes() ----------------------------------------------
//
//-- Draws x and y Axes on the graph
//-------------------------------------------------------------------

void GraphWnd::DrawAxes( CDC *pDC, bool drawYAxisLabels /*=true*/ )
   {
   //-- now, do tick marks and labels --//
   LOGFONT logFont;
   this->InitializeFont( logFont );

   //-- set font --//
   logFont.lfEscapement  = OR_HORZ;
   logFont.lfOrientation = OR_HORZ;
   logFont.lfHeight      = m_labelHeight*3/4;   // character height
   logFont.lfWidth       = 0;               // average width (use default)
   logFont.lfItalic      = 1;

   HFONT hFont    = CreateFontIndirect( (LOGFONT FAR *) &logFont );
   HFONT hFontOld = (HFONT) SelectObject( pDC->m_hDC, hFont );

   pDC->SetBkMode( TRANSPARENT );
   pDC->SetTextColor( label[ XAXIS ].color );
   pDC->SetTextAlign( TA_CENTER | TA_TOP );

   //-- set scaling factors --//
   UINT rows = 0;
   for ( UINT i=0; i < (UINT) m_lineArray.GetSize(); i++ )
      rows += m_lineArray[ i ]->pDataObj->GetRowCount();

   if ( xAxis.autoScale && rows > 1 )
      {
      xMin = 0;
      xMax = 0;
      int ticks;
      float tickIncr;

      //-- get min and max --//
      xCol.pDataObj->GetMinMax( xCol.colNum, &xMin, &xMax );
      xAxis.digits = GetAutoScaleParameters( xMin, xMax, ticks, tickIncr );   // autoscale x values

      if ( xAxis.autoIncr && xAxisLabelArray.Length() <= 1 )
         SetAxisTickIncr( XAXIS, tickIncr, 0 );

      //-- axis value labels defined? --//
      else if ( xAxisLabelArray.Length() > 1 )
         xAxis.major = ( xMax - xMin ) / ( xAxisLabelArray.Length() - 1 );
      }

   if ( yAxis.autoScale && ( rows > 1 ) && ( m_lineArray.GetSize() > 0 )  && drawYAxisLabels )
      {
      yMin = (float) 1.0e38;
      yMax = (float) -1.0e38;

      //-- cycle through lines, geting mins, maxs --//
      for ( UINT line=0; line < (UINT) m_lineArray.GetSize(); line++ )
         {
         float _min = 0;
         float _max = 0;

         if ( m_lineArray[ line ]->m_isEnabled )
            {
            DataObj *pDataObj = m_lineArray[ line ]->pDataObj;
            pDataObj->GetMinMax( m_lineArray[ line ]->dataCol, &_min, &_max );

            if ( _min < yMin ) yMin = _min;
            if ( _max > yMax ) yMax = _max;
            }
         }

      int ticks;
      float tickIncr;

      if ( yMin == (float) 1.0e38 )
         {
         yMin = 0;
         yMax = 1;
         }

      yAxis.digits = GetAutoScaleParameters( yMin, yMax, ticks, tickIncr );

      if ( yAxis.autoIncr )
         SetAxisTickIncr( YAXIS, tickIncr, 0 );
      }

   if ( yAxis.minor >= yAxis.major )
      yAxis.minor = yAxis.major / 2;

   int count = 0;

   //-- draw x axis --//
   for ( float xTic=xMin; xTic <= (xMax+xAxis.major/2); xTic += xAxis.major )
      {
      COORD2d point;
      point.x = xTic;
      point.y = yMin;

      VPtoLP( point );
      pDC->MoveTo( (int) point.x, m_chartOffsetBottom-TICSIZE );
      pDC->LineTo( (int) point.x, m_chartOffsetBottom );

      if ( showXGrid && ( xTic > xMin ) && ( xTic < (xMax-(xAxis.major/2) )))
         {
         HPEN hPen = CreatePen( PS_DOT, 0, LTGRAY );
         hPen = (HPEN) SelectObject( pDC->m_hDC, hPen );
         pDC->LineTo( (int) point.x, YLOGMAX-m_chartOffsetTop );
         DeleteObject( SelectObject( pDC->m_hDC, hPen ) );
         }

      //-- do labels if present, otherwise format output --//
      if ( xAxis.showScale )
         {
         char buffer[ 128 ];

         if ( xAxisLabelArray.Length() > 0 )
            {
            strncpy_s( buffer, 128, xAxisLabelArray[ count++ ], 127 );
            buffer[ 127 ] = '\0';
            }

         else if ( IsShowDate() ) // is it a date - generate one
            {
            int year = dateYear;
            int month, day;

            BOOL ok = GetCalDate( (int) xTic, &year, &month, &day, true );
            year = year % 100;

            if ( ok )
               sprintf_s( buffer, 128, "%i/%i/%i", month, day, year );
            else
               buffer[ 0 ] = NULL;
            }

         else
            {
            char format[ 10 ];
            strcpy_s( format, 10, "%." );
            _itoa_s ( xAxis.digits, format+2, 8, 10 );

            if ( xMax > 1.0e6 )
               strcat_s( format, 10, "e" );
            else
               strcat_s( format, 10, "f" );
            //strcpy_s( format, "%f" );
            sprintf_s( buffer, 128, format, xTic );
            }

         pDC->TextOut( (int) point.x, m_chartOffsetBottom-2*TICSIZE, (LPSTR) buffer, lstrlen( buffer ) );
         }
      }  // end of:  if ( xAxis.showScale )

   //-- then, y-axis --//
   pDC->SetTextAlign( TA_BASELINE | TA_RIGHT );
   pDC->SetTextColor( label[ YAXIS ].color );

   if ( drawYAxisLabels )
      {
      for ( float yTic=yMin; yTic <= (yMax+yAxis.major/2); yTic += yAxis.major )
         {
         COORD2d point;
         point.x = xMin;
         point.y = yTic;

         VPtoLP( point );
         pDC->MoveTo( m_chartOffsetLeft-TICSIZE, (int) point.y );
         pDC->LineTo( m_chartOffsetLeft,         (int) point.y );

         if ( showYGrid && ( yTic > yMin ) && ( yTic < (yMax-(yAxis.major/2) )))
            {
            HPEN hPen = CreatePen( PS_DOT, 0, LTGRAY );
            hPen = (HPEN) SelectObject( pDC->m_hDC, hPen );
            pDC->LineTo( XLOGMAX-m_chartOffsetRight, (int) point.y );
            DeleteObject( SelectObject( pDC->m_hDC, hPen ) );
            }

         if ( yAxis.showScale )
            {
            char buffer[ 128 ];
            char format[ 10 ];
            strcpy_s( format, 10, "%." );
            _itoa_s ( yAxis.digits, format+2, 8, 10 );

            if ( yMax > 1.0e8 )
               strcat_s( format, 10, "e" );
            else
               strcat_s( format, 10, "f" );

            sprintf_s( buffer, 128, format, yTic );

            pDC->TextOut( (int) m_chartOffsetLeft-2*TICSIZE, (int) point.y, (LPSTR) buffer, lstrlen( buffer ) );
            }
         }
      }
   //-- clean up device context --//
   SelectObject( pDC->m_hDC, hFontOld );
   DeleteObject( hFont );
   }


int GraphWnd::GetAutoScaleParameters( float &xmin, float &xmax, int &ticks, float &tickIncr )
   {
   // test trival case where range is essentially 0
   if ( fabs( xmin - xmax ) < 1.0e-10 )
      {
      if ( fabs( xmin ) < 1.0e-7 )      // are they zero?
         {
         xmin = -1;
         xmax = 1;
         }
      else
         {
         float temp = xmin;
         xmin = temp * 0.9f;
         xmax = temp * 1.1f;
         }
      }

   float _xmin = min( xmin, xmax );
   float _xmax = max( xmin, xmax );

   double low, high, _ticks;

   bestscale(_xmin, _xmax, &low, &high, &_ticks);
   ticks = int(_ticks);

   if (ticks < 2)
      ticks = 2;
   
   // jpb - adjustment because ticks seem to high
   while ( ticks > 10 )
      ticks /= 2;

   xmin = float(low);
   xmax = float(high);

   tickIncr = (xmax-xmin)/(ticks-1.0f);

   // compute significant digits (to the right of the decimal point).  These are tied to the precision of the scale
   TCHAR buffer[ 64 ];
   int count = _stprintf_s( buffer, 64, "%f", tickIncr );

   int lsd = count-1;   
   while( lsd >= 0 && buffer[ lsd ] == '0' )   // strip rightmost '0's
      lsd--;

   TCHAR *pDecimal = _tcschr( buffer, '.' );
   int decimal = 0;
   if ( pDecimal )
      while ( _istdigit( buffer[ decimal ] ) ) 
         decimal++;
   else
      decimal = -1;

   int digits = lsd-decimal;
   ASSERT( digits >= 0 );
   /*
   float fdigits = (float) log10( tickIncr );
   int digits;
   if ( fdigits > 0 )    // tickIncr > 1?
      digits = 1;
   else
      digits = int( (-fdigits)+0.9f ) + 1; */

   return digits;
   }


void GraphWnd::GetFrameRect( POINT &lowerLeft, POINT &upperRight )  // logical coordinates
   {
   lowerLeft.x  = m_chartOffsetLeft;
   lowerLeft.y  = m_chartOffsetBottom;

   upperRight.x = XLOGMAX - m_chartOffsetRight;
   upperRight.y = YLOGMAX - m_chartOffsetTop;
   }


// assume HDC has been InitMapped()
void GraphWnd::DrawFrame( CDC *pDC )
   {
   POINT lowerLeft;
   POINT upperRight;

   GetFrameRect( lowerLeft, upperRight );

   //-- enlarge rect by one pixel --//
   pDC->LPtoDP( &lowerLeft, 1 );
   lowerLeft.x--;     // enlarge
   lowerLeft.y++;
   pDC->DPtoLP( &lowerLeft, 1 );

   pDC->LPtoDP( &upperRight, 1 );
   upperRight.x++;     // enlarge
   upperRight.y--;
   pDC->DPtoLP(  &upperRight, 1 );

   pDC->SelectObject( GetStockObject( WHITE_BRUSH) );
   pDC->Rectangle( lowerLeft.x, lowerLeft.y, upperRight.x, upperRight.y );

   pDC->SelectObject( GetStockObject( NULL_BRUSH) );

   if ( frameStyle == FS_GROOVED || frameStyle == FS_RAISED )
      {
      //-- shift right, down one pixel--//
      pDC->LPtoDP( &lowerLeft, 1 );
      lowerLeft.x++; 
      lowerLeft.y++;
      pDC->DPtoLP( &lowerLeft, 1 );

      pDC->LPtoDP( &upperRight, 1 );
      upperRight.x++;
      upperRight.y++;
      pDC->DPtoLP( &upperRight, 1 );

      //-- set up pens, brushes --//
      if ( frameStyle == FS_GROOVED )
         pDC->SelectObject( GetStockObject( WHITE_PEN ) );
      else
         pDC->SelectObject( GetStockObject( BLACK_PEN ) );

      //-- draw hilite lines --//
      pDC->Rectangle( lowerLeft.x, lowerLeft.y, upperRight.x, upperRight.y );

      //-- shift rect back --//
      pDC->LPtoDP( &lowerLeft, 1 );
      lowerLeft.x--;     // shift
      lowerLeft.y--;
      pDC->DPtoLP( &lowerLeft, 1 );

      pDC->LPtoDP( &upperRight, 1 );
      upperRight.x--;     // shift
      upperRight.y--;
      pDC->DPtoLP( &upperRight, 1 );

      if ( frameStyle == FS_GROOVED )
         pDC->SelectObject( GetStockObject( BLACK_PEN ) );
      else
         pDC->SelectObject( GetStockObject( WHITE_PEN ) );

      pDC->Rectangle( lowerLeft.x, lowerLeft.y, upperRight.x, upperRight.y );
      }

   else if ( frameStyle == FS_SUNKEN )
      {
      //-- draw "inner" frame (start with black) --//

      pDC->SelectObject( GetStockObject( BLACK_PEN ) );

      pDC->MoveTo( lowerLeft.x,  lowerLeft.y  );   // lower left
      pDC->LineTo( lowerLeft.x,  upperRight.y );   // upper left
      pDC->LineTo( upperRight.x, upperRight.y );   // upper right

      //-- change to a white pen --//
      pDC->SelectObject( GetStockObject( WHITE_PEN ) );
      pDC->LineTo( upperRight.x, lowerLeft.y );    // lower right
      pDC->LineTo( lowerLeft.x,  lowerLeft.y );    // lower left

      //-- inflate rect again 1 pixel--//
      pDC->LPtoDP( &lowerLeft, 1 );
      lowerLeft.x--; 
      lowerLeft.y++;
      pDC->DPtoLP( &lowerLeft, 1 );

      pDC->LPtoDP( &upperRight, 1 );
      upperRight.x++;
      upperRight.y--;
      pDC->DPtoLP( &upperRight, 1 );

      //-- draw "outer" frame --//

      HPEN hPen = CreatePen( PS_SOLID, 0, GRAY );
      pDC->SelectObject( hPen );
      pDC->MoveTo( lowerLeft.x,  lowerLeft.y );   // lower left
      pDC->LineTo( lowerLeft.x,  upperRight.y );   // upper left
      pDC->LineTo( upperRight.x, upperRight.y );   // upper right
      
      //-- change to a light gray pen --//

      //-- do our own dithering --//
      pDC->LPtoDP( (LPPOINT) &lowerLeft, 1 );
      pDC->LPtoDP( (LPPOINT) &upperRight, 1 );

      HDC _hDC = ::GetDC( m_hWnd );  // get a text-mapped device context
      for ( int y = upperRight.y; y < lowerLeft.y; y++ )
          SetPixel( _hDC, upperRight.x, y, ( y%2 ? LTGRAY : WHITE ) );

      for ( int x = upperRight.x; x >= lowerLeft.x; x-- )
          SetPixel( _hDC, x, lowerLeft.y, ( x%2 ? LTGRAY : WHITE ) );
      ::ReleaseDC( m_hWnd, _hDC );

      pDC->SelectObject( GetStockObject( BLACK_PEN ) );
      DeleteObject( hPen );
      }

   else // ( FS_NORMAL )
      {
      pDC->SelectObject( GetStockObject( BLACK_PEN ) );
      pDC->Rectangle( lowerLeft.x, lowerLeft.y, upperRight.x, upperRight.y );
      }

   return;
   }


int GraphWnd::IsPointOnLegend( POINT &click )
   {
   //-- note: click is in logical units (from InitMapped hdc ) --//

   for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
      {
      if ( ( click.x > m_lineArray[i]->rect.left )
        && ( click.x < m_lineArray[i]->rect.right )
        && ( click.y > m_lineArray[i]->rect.bottom )
        && ( click.y < m_lineArray[i]->rect.top ) )
         return i;
      }

   return -1;
   }


void GraphWnd::SetExtents( MEMBER axis, float minimum, float maximum, bool computeTickIncr /*=true*/, bool redraw /*=false*/ )
   {
   if ( axis == XAXIS )
      {
      if ( minimum == maximum )
         xAxis.autoScale = true;
      else
         {
         xMin = minimum;
         xMax = maximum;
         xAxis.autoScale = false;
         }
      }
   else
      {
      if ( minimum == maximum )
         yAxis.autoScale = true;
      else
         {
         yMin = minimum;
         yMax = maximum;
         yAxis.autoScale = false;
         }
      }
   
   if ( computeTickIncr )
      {
      float tickIncr;
      int ticks;
      int digits = GetAutoScaleParameters( minimum, maximum, ticks, tickIncr );
      SetAxisTickIncr( axis, tickIncr, 0 );
      SetAxisDigits( axis, digits );
      }

   if ( redraw )
      RedrawWindow();
   }


void GraphWnd::GetExtents( MEMBER axis, float *minimum, float *maximum )
   {
   if ( axis == XAXIS )
      { *minimum = xMin; *maximum = xMax; }
   else
      { *minimum = yMin; *maximum = yMax; }
   }



int GraphWnd::GetIdealLegendSize( CDC *pDC, CSize &sz )
   {
   // Assumes hDC has already been InitMapped()
   /*
    * this gets the size of the legend font (in logical units*10) needed to "fit" the legend in legend area
    *.The basic idea is to find a balnce between font size and available space.
    * We start with a font size of 11pts.  If the legend "fits" in a reason rectangle, were done.
    * if it doesn't fit, repeat with smaller/larger fonts until it fits
    return 10*pt size.  sz is populated with the width and height of a single legend row
  */
   
   int lineCount = (int) m_lineArray.GetSize();
   
   CRect rect;
   rect.bottom = YLOGMAX * 1 / 10;
   rect.top    = YLOGMAX * 9 / 10;
   rect.right  = XLOGMAX - 10;
   rect.left   = XLOGMAX - m_chartOffsetRight;   

   int maxFromHeight = (YLOGMAX*8/10) * 8/10;      // second factor is for buffer between successive lines

   LOGFONT logFont;
   InitializeFont( logFont );
   
   // select a font of known dimensions
   logFont.lfHeight = 100;
   CFont font;
   font.CreateFontIndirect( &logFont );
   CFont *pOldFont = pDC->SelectObject( &font );

   // get max from width
   int maxWidth = 0;
   int maxHeight = 0;
   for ( int i=0; i < lineCount; i++ )
      {
      CSize sz = pDC->GetOutputTextExtent( m_lineArray[ i ]->text );
      if ( sz.cx > maxWidth )
         maxWidth = sz.cx;

      if ( sz.cy > maxHeight )
         maxHeight = sz.cy;
      }

   pDC->SelectObject( pOldFont );

   float scalar = rect.Width() / float( maxWidth ); // >1 when legend too small, <1 when legend too big

   int height = int( 100 *scalar );
   if ( height < 60 )
      height = 60;
   else if ( height > 80 )
      height = 80;

   logFont.lfHeight = height;
   CFont font2;
   font2.CreateFontIndirect( &logFont );
   pOldFont = pDC->SelectObject( &font2 );

   // get max from width
   maxWidth  = 0;
   maxHeight = 0;
   for ( int i=0; i < lineCount; i++ )
      {
      CSize sz = pDC->GetOutputTextExtent( m_lineArray[ i ]->text );
      if ( sz.cx > maxWidth )
         maxWidth = sz.cx;

      if ( sz.cy > maxHeight )
         maxHeight = sz.cy;
      }

   pDC->SelectObject( pOldFont );

   // temp   
   sz.cx = 3*maxWidth/2;
   sz.cy = int( maxHeight * ( 1+LEGEND_SPACING) );
   return height;
   }


bool GraphWnd::SetLegendText( int lineNo, LPCTSTR newText )
   {
   m_lineArray[ lineNo ]->text = newText;
   return true;
   }


bool GraphWnd::SetLabelText( MEMBER member, LPCTSTR newText )
   {
   label[ member ].text = newText;
   return true;
   }


void GraphWnd::SetAxisTickIncr( MEMBER axis, float major, float minor )
   {
   if ( axis == XAXIS )
      { 
      xAxis.major = major;  
      xAxis.minor = minor; 
      //xAxis.autoIncr = false;
      }
   else
      { 
      yAxis.major = major;  
      yAxis.minor = minor; 
      //yAxis.autoIncr = false;
      }
   }


void GraphWnd::GetAxisTickIncr( MEMBER axis, float *major, float *minor )
   {
   if ( axis == XAXIS )
      { *major = xAxis.major;  *minor = xAxis.minor; }
   else
      { *major = yAxis.major;  *minor = yAxis.minor; }
   }


void GraphWnd::SetAxisDigits( MEMBER axis, int numDigits )
   {
   if ( axis == XAXIS )
      xAxis.digits = numDigits;
   else
      yAxis.digits = numDigits;
   }
      

int GraphWnd::GetAxisDigits( MEMBER axis )
   {
   if ( axis == XAXIS )
      return xAxis.digits;
   else
      return yAxis.digits;
   }


void GraphWnd::ShowAxisScale( MEMBER axis, bool flag )
   {
   if ( axis == XAXIS )
      xAxis.showScale = flag;
   else
      yAxis.showScale = flag;
   }


//-- VPtoLP() -------------------------------------------------------
//
//-- Scales a virtual coordinant to a logical coordinant for use in
//   a DrawWindow device context. This assures sufficient resolution
//   in moving from float to int based display units
//-------------------------------------------------------------------

void GraphWnd::VPtoLP( COORD2d &point )
   {
   if ( xMax-xMin == 0.0 ) return;
   if ( yMax-yMin == 0.0 ) return;

   point.x = (XLOGMAX-m_chartOffsetLeft-m_chartOffsetRight)
             * ( (point.x-xMin) / (xMax-xMin) ) + m_chartOffsetLeft;

   point.y = (YLOGMAX-m_chartOffsetTop-m_chartOffsetBottom) 
             * ( (point.y-yMin) / (yMax-yMin) ) + m_chartOffsetBottom;
   }


void GraphWnd::VPtoLP( COORD2d &point, float _yMin, float _yMax )
   {
   if ( xMax  - xMin  == 0.0 ) return;
   if ( _yMax - _yMin == 0.0 ) return;

   point.x = (XLOGMAX-m_chartOffsetLeft-m_chartOffsetRight) 
             * ( (point.x-xMin) / (xMax-xMin) ) + m_chartOffsetLeft;

   point.y = (YLOGMAX-m_chartOffsetTop-m_chartOffsetBottom) 
             * ( (point.y-_yMin) / (_yMax-_yMin) ) + m_chartOffsetBottom;
   }


bool GraphWnd::EnableLine( int lineNo, bool enable )
   {
   if ( lineNo >= (int) m_lineArray.GetSize() )
      return false;

   if ( lineNo < 0 )
      for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
         {
         m_lineArray[ i ]->m_isEnabled = enable;
         }

   else
      m_lineArray[ lineNo ]->m_isEnabled = enable;
   
   return true;
   }


bool GraphWnd::ShowLine( int lineNo, bool flag )
   {
   if ( lineNo >= (int) m_lineArray.GetSize() )
      return false;

   if ( lineNo < 0 )
      for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
         {
         m_lineArray[ i ]->showLine = flag;
         }

   else
      m_lineArray[ lineNo ]->showLine = flag;
   
   return true;
   }


bool GraphWnd::SetMarkerStyle( int lineNo, MSTYLE mStyle )
   {
   if ( lineNo >= (int) m_lineArray.GetSize() )
      return false;

   if ( lineNo < 0 )
      for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
         {
         m_lineArray[ i ]->showMarker = true;
         m_lineArray[ i ]->markerStyle = mStyle;
         }

   else
      {
      m_lineArray[ lineNo ]->showMarker  = true;
      m_lineArray[ lineNo ]->markerStyle = mStyle;
      }

   return true;
   }


bool GraphWnd::SetMarkerIncr( int lineNo, int incr )
   {
   if ( lineNo >= (int) m_lineArray.GetSize() )
      return false;

   if ( lineNo < 0 )
      for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
         {
         m_lineArray[ i ]->showMarker = true;
         m_lineArray[ i ]->markerIncr = incr;
         }

   else
      {
      m_lineArray[ lineNo ]->showMarker = true;
      m_lineArray[ lineNo ]->markerIncr = incr;
      }

   return true;
   }



bool GraphWnd::SetMarkerColor( int lineNo, COLORREF color )
   {
   if ( lineNo >= (int) m_lineArray.GetSize() )
      return false;

   if ( lineNo < 0 )
      for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
         {
         m_lineArray[ i ]->showMarker = true;
         m_lineArray[ i ]->markerColor = color;
         }
   else
      {
      m_lineArray[ lineNo ]->showMarker = true;
      m_lineArray[ lineNo ]->markerColor = color;
      }

   return true;
   }


bool GraphWnd::ShowMarker( int lineNo, bool flag )
   {
   if ( lineNo >= (int) m_lineArray.GetSize() )
      return false;

   if ( lineNo < 0 )
      for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
            m_lineArray[ i ]->showMarker = flag;
   else
      m_lineArray[ lineNo ]->showMarker = flag;
   return true;
   }


void GraphWnd::ShowMarkers( bool flag )
   {
   int length = (int) m_lineArray.GetSize();

   for ( int i=0; i < length; i++ )
      m_lineArray[ i ]->showMarker = flag;
   }


bool GraphWnd::SetSmooth( int lineNo /*=-1*/, float tension /*=0.5f*/)
   {
   if ( lineNo >= (int) m_lineArray.GetSize() )
      return false;

   if ( lineNo < 0 )
      for ( int i=0; i < (int) m_lineArray.GetSize(); i++ )
         {
         m_lineArray[ i ]->m_smooth = true;
         m_lineArray[ i ]->m_tension = tension;
         }

   else
      {
      m_lineArray[ lineNo ]->m_smooth = true;
      m_lineArray[ lineNo ]->m_tension = tension;
      }

   return true;
   }




bool GraphWnd::AppendAxisLabel( MEMBER axis, const char * const label )
   {
   switch( axis )
      {
      case XAXIS:
         {
         xAxisLabelArray.AddString( label );
         return true;
         }

      case YAXIS:
         {
         yAxisLabelArray.AddString( label );
         return true;
         }
      }
   return false;
   }


bool GraphWnd::MakeDataLocal( void )
   {
   if ( pLocDataObj )
      return true;

   int cols = int( m_lineArray.GetSize() + 1 );  // +1 for x data
   int rows = xCol.pDataObj->GetRowCount();

   pLocDataObj = new FDataObj( cols, 0 );  //rows );

   if ( pLocDataObj == NULL )
      return false;

   float *data = new float[ cols ];
   int i = 0;

   for ( i=0; i < rows; i++ )
      {
      data[ 0 ] = xCol.pDataObj->GetAsFloat( xCol.colNum, i );

      for ( int j=1; j < cols; j++ )
         {
         LINE *pLine = m_lineArray[ j-1 ];
         UINT col    = pLine->dataCol;

         data[ j ] = pLine->pDataObj->GetAsFloat( col, i );
         }

      pLocDataObj->AppendRow( data, cols );
      }

   delete [] data;

   //-- local data object set up, reset x, y dataobj ptrs --//
   for ( i=0; i < cols-1; i++ )
      {
      LINE    *pLine    = m_lineArray[ i ];
      DataObj *pDataObj = pLine->pDataObj;

      pLocDataObj->SetLabel( i+1, (char*) pDataObj->GetLabel( pLine->dataCol ) );
      m_lineArray[ i ]->pDataObj = pLocDataObj;
      m_lineArray[ i ]->dataCol  = i+1;
      }

   pLocDataObj->SetLabel( 0, (char*) xCol.pDataObj->GetLabel( xCol.colNum ) );
   xCol.pDataObj = pLocDataObj;
   xCol.colNum   = 0;

   if ( pTableMngr )
      delete pTableMngr;

   return true;
   }


/// return logical coords
void GraphWnd::GetPlotBoundingRect( HDC hDC, LPPOINT lowerLeft, LPPOINT upperRight )
   {
   lowerLeft->x  = m_chartOffsetLeft;
   lowerLeft->y  = m_chartOffsetBottom;

   upperRight->x = XLOGMAX - m_chartOffsetRight;
   upperRight->y = YLOGMAX - m_chartOffsetTop;

   //-- offset by one pixel
   LPtoDP( hDC, (LPPOINT) lowerLeft, 1 );
   lowerLeft->x--;
   lowerLeft->y++;
   DPtoLP( hDC, (LPPOINT) lowerLeft, 1 );

   LPtoDP( hDC, (LPPOINT) upperRight, 1 );
   upperRight->x++;
   upperRight->y--;
   DPtoLP( hDC, (LPPOINT) upperRight, 1 );
   }

BOOL GraphWnd::OnEraseBkgnd(CDC* pDC)
   {
   RECT rect;
   GetClientRect( &rect );

   TRIVERTEX        vert[2] ;
   GRADIENT_RECT    gRect;
   vert [0] .x      = 0;
   vert [0] .y      = 0;
   vert [0] .Red    = 0xcc00;
   vert [0] .Green  = 0xcc00;
   vert [0] .Blue   = 0xcc00;  //ff00;
   vert [0] .Alpha  = 0x0000;

   vert [1] .x      = rect.right;
   vert [1] .y      = rect.bottom; 
   vert [1] .Red    = 0xff00;
   vert [1] .Green  = 0xff00;
   vert [1] .Blue   = 0xff00;
   vert [1] .Alpha  = 0x0000;

   gRect.UpperLeft  = 0;
   gRect.LowerRight = 1;
   pDC->GradientFill( vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V );

   return 1; //CWnd::OnEraseBkgnd(pDC);
   }


void GraphWnd::DrawRubberRect( CDC *pDC, RECT &rect, int width )
   {
   CPen pen;
   pen.CreatePen( PS_SOLID, width, (COLORREF) 0 );
   CPen *pOldPen = pDC->SelectObject( &pen );

   int drawMode = pDC->SetROP2( R2_NOT );

   pDC->MoveTo( rect.left,  rect.top );
   pDC->LineTo( rect.right, rect.top );
   pDC->LineTo( rect.right, rect.bottom );
   pDC->LineTo( rect.left,  rect.bottom );
   pDC->LineTo( rect.left,  rect.top );

   pDC->SetROP2( drawMode );
   pDC->SelectObject( pOldPen );
   }


///////////// G R A P H D L G class /////////////////////////

IMPLEMENT_DYNAMIC(GraphDlg, CPropertySheet)

BEGIN_MESSAGE_MAP(GraphDlg, CPropertySheet)
   ON_COMMAND (ID_APPLY_NOW, OnApplyNow)
END_MESSAGE_MAP()


GraphDlg::GraphDlg( GraphWnd *pParent ) 
 : CPropertySheet ( "Plot Setup", pParent ),
   m_setupPage( pParent ),
   m_axesPage( pParent ),
   m_linePage( pParent )
   {
   m_setupPage.m_psp.dwFlags |= PSP_PREMATURE;
   m_axesPage.m_psp.dwFlags  |= PSP_PREMATURE;
   m_linePage.m_psp.dwFlags  |= PSP_PREMATURE;

   AddPage( &m_setupPage );
   AddPage( &m_axesPage );
   AddPage( &m_linePage );
   }


BOOL GraphDlg::OnInitDialog() 
   {
   BOOL retVal = CPropertySheet::OnInitDialog();

   //m_mainPage.SetModified( TRUE );

   // hide the ok button
   CWnd *pOK = GetDlgItem( IDOK );

   if ( pOK )
      pOK->ShowWindow( SW_HIDE );

   return retVal;
   }



void GraphDlg::OnApplyNow() 
   {
   m_setupPage.Apply( true );
   m_axesPage.Apply( true );
   //m_linePage.Apply( true );
   }



#endif  // _WINDOWS
