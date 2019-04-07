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
// BarPlotWnd.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include <libs.h>

#include "BarPlotWnd.h"

#include <dataobj.h>
#include <math.h>


// BarPlotWnd

IMPLEMENT_DYNAMIC(BarPlotWnd, GraphWnd)

BarPlotWnd::BarPlotWnd()
{
}

BarPlotWnd::BarPlotWnd( DataObj *pData, bool makeDataLocal /*=false*/ )
   : GraphWnd( pData, makeDataLocal ),
     m_style( BPS_STACKED )
   { }


BarPlotWnd::~BarPlotWnd()
{
}


BEGIN_MESSAGE_MAP(BarPlotWnd, GraphWnd)
	//ON_WM_PAINT()
   ON_WM_CREATE()
   ON_CBN_SELCHANGE(100, OnComboChange)
END_MESSAGE_MAP()


int BarPlotWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (GraphWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   CRect rect( 5, 5, 300, 500 );
   m_combo.Create( WS_CHILD | CBS_DROPDOWNLIST, rect, this, 100 ); 
   
   return 0;
   }

void BarPlotWnd::DrawPlotArea( CDC *pDC )     // note: hDC should be initMapped when this function is called
   {
   DrawFrame( pDC );
   UpdatePlot( pDC, false, 0, -1 );
   }


void BarPlotWnd::LoadCombo()
   {
   m_combo.ShowWindow( SW_SHOW );
   m_combo.ResetContent();
   m_combo.AddString( _T("-- Show All --") );
   
   for ( UINT i=0; i < (UINT) m_lineArray.GetSize(); i++ )
      m_combo.AddString( m_lineArray[ i ]->text );

   m_combo.SetCurSel( 0 );    
   }


void BarPlotWnd::SetBarPlotStyle( BPSTYLE style, int extra /* =-1 */ )
   {
   m_style = style;

   m_combo.ShowWindow( SW_HIDE );

   switch( style )
      {
      case BPS_GROUPED:
         break;

      case BPS_STACKED:
         for ( UINT i=0; i < (UINT) m_lineArray.GetSize(); i++ )
            {
            m_lineArray[ i ]->showLine   = true;
            m_lineArray[ i ]->showMarker = true;
            }
         break;

      case BPS_SINGLE:
         {
         if ( extra < 0 ) 
            extra = 0;

         for ( UINT i=0; i < (UINT) m_lineArray.GetSize(); i++ )
            {
            bool show = ( i == extra ) ? true : false;
            m_lineArray[ i ]->showLine   = show;
            m_lineArray[ i ]->showMarker = show;
            }

         if ( m_lineArray.GetSize() < 2 )
            m_combo.ShowWindow( SW_HIDE );
         else
            m_combo.ShowWindow( SW_SHOW );
         break;
         }
      }

   RedrawWindow();
   }


//-- BarPlotWnd::UpdatePlot() ------------------------------------------
//
//   Draws actual data on the plot
//-------------------------------------------------------------------

bool BarPlotWnd::UpdatePlot( CDC *pDC,  bool doInitMapping, int startIndex/*=0*/, int numCols/*=-1*/ )
   {
   if ( IsIconic() )
      return true;

   int lineCount = (int) m_lineArray.GetSize();

   if ( lineCount == 0 )
      return false;

   HDC hDC = pDC->m_hDC;

   //-- check row count to make sure there is something to draw
   // NOTE:  A row in the data object corresponds to a group of bars on the bar plot
   UINT groupCount = m_lineArray[ 0 ]->pDataObj->GetRowCount();

   if ( groupCount == 0 )  // no rows in any data object?
      return false;

   // there's data, so draw it for each line
   if ( doInitMapping )
      InitMapping( pDC );

   SetBkMode( hDC, TRANSPARENT );

   if ( xAxis.autoScale || yAxis.autoScale )
      {
      lastRowCount = 0;
      DrawAxes( pDC );
      }

   // we assume all lines (represented by stacks in the bar) have the same number of rows (the successive bars)
   // to draw the stacks, we iterate through the rows of data, drawing each stack as we go

   //-- get plot bounding rect
   CPoint plotLowerLeft, plotUpperRight;
   GetPlotBoundingRect( hDC, &plotLowerLeft, &plotUpperRight );  // return LOGICAL coords
   CPoint barLowerLeft, barUpperRight;
   int plotWidth = plotUpperRight.x - plotLowerLeft.x;

   int stacks = (int) m_lineArray.GetSize();
   int fullGroupWidth = plotWidth/groupCount;
   int groupWidth = int(fullGroupWidth * 0.9f);
   int separation = (fullGroupWidth-groupWidth);
   //  ||--[  ]----[  ]----[  ]--|| 

   int barWidth = groupWidth;
   if ( m_style == BPS_GROUPED )
      barWidth = groupWidth / groupCount;

   int selectedStack = m_combo.GetCurSel()-1;  // only used on BPS_SINGLE

   COORD2d zeroPt;   // baseline of plot bars - positives above, negatives below
   zeroPt.x = zeroPt.y = 0;
   this->VPtoLP( zeroPt );

   int stackBaseline = (int) zeroPt.y;   // logical coord of "zero" elevation on the plot

   // iterate through the rows in the data object, corresponding to bar groups on the graph
   for ( UINT group=0; group < groupCount; group++ )
      {
      int stackTop    = stackBaseline;    // current location of top of current stack
      int stackBottom = stackBaseline;    // current location of bottom of current stack
      int stackLeft   = plotLowerLeft.x + separation/2 + group*fullGroupWidth; 
      int barLeft     = stackLeft;
      int barRight    = barLeft;
      int barTop;
      int barBottom;

      for ( int j=stacks-1; j >= 0; j-- )
         {
         if ( m_style == BPS_SINGLE && j != selectedStack )
            continue;

         LINE *pLine = m_lineArray[ j ];
         if ( pLine->showLine == false && pLine->showMarker == false )
            continue;

         pLine->markerStyle = MS_BAR2DHOLLOW;    // hack for now...

         // basic assumption - the data object columns contain information about the
         // height of each bar segment, in virtual coords.  Use these to draw the height of the block.
         //  xCol information is ignored, other than for labels
         float barValue = pLine->pDataObj->GetAsFloat( pLine->dataCol, group );  // virtual coords
        
         COORD2d point( 0, barValue );
         this->VPtoLP( point );
         int barHeight = (int) point.y;   // height of bar in logical coords.

         //draw the bar
         switch( m_style )
            {
            case BPS_GROUPED:
            case BPS_SINGLE:
               barLeft += barWidth;
               barBottom = stackBaseline;
               barTop = barBottom + barHeight;
               break;

            case BPS_STACKED:
               barRight = barLeft + barWidth;
               if ( barHeight > 0 )
                  {
                  barBottom = stackTop;
                  barTop = barBottom + barHeight;
                  stackTop = barTop;
                  }
               else
                  {
                  barTop = stackBottom;
                  barBottom = barTop + barHeight;
                  stackBottom = barBottom;
                  }
               break;
            }

         if ( barHeight != 0 )
            DrawBar( hDC, barLeft, barTop, barRight, barBottom, pLine->markerStyle, pLine->color );
         }

      //if ( pLine->showMarker || pLine->showLine )
      //   DrawLine( hDC, pLine, lowerLeft, upperRight ); //, startIndex, numPoints );
      }

   //-- done with all lines, clean up
   lastRowCount = 0;

   return true;
   }



//-- Bar::DrawBar() ----------------------------------------------
//
//-- Draws a bar on the screen at the logical point (x,y)
//-------------------------------------------------------------------

void BarPlotWnd::DrawBar( HDC hDC, int left, int top, int right, int bottom, MSTYLE style, COLORREF color )
   {
   switch( style )
      {
      case MS_BAR2DSOLID:
         {
         HBRUSH hBrush = CreateSolidBrush( color );
         hBrush = (HBRUSH) SelectObject( hDC, hBrush );
         Rectangle( hDC, left, top, right, bottom );
         DeleteObject( SelectObject( hDC, hBrush ) );
         }
         return;

      case MS_BAR2DHOLLOW:
         {
         HPEN hPen = CreatePen( PS_SOLID, 1, color );
         HBRUSH hBrush = (HBRUSH) SelectObject( hDC, GetStockObject( NULL_BRUSH ) );
         hPen = (HPEN) SelectObject( hDC, hPen );
         Rectangle( hDC, left, top, right, bottom );
         DeleteObject( SelectObject( hDC, hPen ) );
         SelectObject( hDC, hBrush );
         }
         return;

      default:
         break;
      }

   return;
   }

void BarPlotWnd::OnComboChange()
   {
   int selectedStack = m_combo.GetCurSel();

   for ( UINT i=0; i < (UINT) m_lineArray.GetSize(); i++ )
      {
      bool show = false;
      if ( selectedStack == 0  || i == selectedStack-1 )
         show = true;

      m_lineArray[ i ]->showLine   = show;
      m_lineArray[ i ]->showMarker = show;
      }

   RedrawWindow();
   }
