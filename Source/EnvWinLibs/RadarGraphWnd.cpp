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
// RadarGraphWnd.cpp : implementation file
//

#include "EnvWinLibs.h"

#include <RadarGraphWnd.h>
#include <fdataobj.h>
#include <math.h>
#include <unitconv.h>

#include ".\RadarGraphWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// RadarGraphWnd

IMPLEMENT_DYNAMIC( RadarGraphWnd, GraphWnd )

RadarGraphWnd::RadarGraphWnd( )
   : GraphWnd(),
     m_showInitialState( false ),
     m_lastDynamicDisplayIndex ( -1 ),
     m_initialStateColor( BLACK ),
     m_currentStateColor( RED ),
     m_pDataObj( NULL ),
     m_row( -1 )
   {
   }

RadarGraphWnd::RadarGraphWnd( FDataObj *pData, float minValueArray[], float maxValueArray[], bool makeDataLocal )
   : GraphWnd(),
     m_showInitialState( false ),
     m_lastDynamicDisplayIndex ( -1 ),
     m_initialStateColor( BLACK ),
     m_currentStateColor( RED ),
     m_row( -1 )
   {
   if ( makeDataLocal )
      m_pDataObj = new FDataObj( *pData );
   else
      m_pDataObj = pData;

   m_isDataLocal = makeDataLocal;

   for ( int i=0; i < m_pDataObj->GetColCount(); i++ )
      {
      AppendCol( m_pDataObj, i, minValueArray[ i ], maxValueArray[ i ] );
      }
   }


RadarGraphWnd::~RadarGraphWnd()
   {
   if ( m_isDataLocal && m_pDataObj != NULL )
      delete m_pDataObj;
   }

void RadarGraphWnd::AppendCol( FDataObj *pDataObj, UINT colNum, float minimum, float maximum )
   {
   LINE *pLine = new LINE;
   m_lineArray.Add( pLine );
   pLine->pDataObj = (DataObj*) pDataObj;
   pLine->dataCol  = colNum;
   //pLine->markerIncr = 5;
   pLine->scaleMin = minimum;
   pLine->scaleMax = maximum;
   }

BOOL RadarGraphWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   cs.lpszClass = AfxRegisterWndClass( CS_VREDRAW | CS_HREDRAW, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

   return GraphWnd::PreCreateWindow(cs);
   }

BEGIN_MESSAGE_MAP(RadarGraphWnd, CWnd)
	ON_WM_PAINT()
   ON_WM_CREATE()
   ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// RadarGraphWnd message handlers

int RadarGraphWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (GraphWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   return 0;
   }

BOOL RadarGraphWnd::OnEraseBkgnd(CDC* pDC)
   {
   return GraphWnd::OnEraseBkgnd( pDC );
   }


void RadarGraphWnd::OnPaint() 
   {
	CPaintDC dc(this); // device context for painting
	
   OnEraseBkgnd( &dc );

   //if ( m_pDataObj == NULL || m_pDataObj->GetColCount() == 0 )
   //   return;

   if ( m_lineArray.GetSize() == 0 )
      return;

   if ( IsIconic() )
       return;

   lastRowCount = 0;

   InitMapping( &dc );
   
   DrawPlot( dc );
   DrawTitle( &dc );
   return;
   }


void RadarGraphWnd::DrawPlot( CDC &dc )
   {
   int axes = (int) m_lineArray.GetSize();
   float rotation = 2 * PI / axes;

   POINT center;
   center.x = ( XLOGMAX + m_chartOffsetLeft - m_chartOffsetRight )/2;
   center.y = YLOGMAX /2;

   float radius = (float) min( XLOGMAX-m_chartOffsetLeft-m_chartOffsetRight, YLOGMAX-m_chartOffsetTop-m_chartOffsetBottom);
   radius /= 2;

   CFont font;
   font.CreatePointFont( 200, "Arial" );
   
   dc.SetBkMode( TRANSPARENT );
   dc.SetTextAlign( TA_LEFT | TA_TOP );
   dc.SetTextColor( BLACK );

   CFont *pOldFont = dc.SelectObject( &font );

   // get out edges of plot
   CPoint *ptArray = new CPoint[ axes ];
   CPoint *labelPtArray = new CPoint[ axes ];
   for ( int i=0; i < axes; i++ )
      {
      LINE *pLine = m_lineArray[i];

      float theta = rotation * i;
      float cosTheta = (float) cos( theta );
      float sinTheta = (float) sin( theta );
      float x = center.x + radius * cosTheta;
      float y = center.y + radius * sinTheta;

      ptArray[ i ].x = (int) x;
      ptArray[ i ].y = (int) y;

      // figure out placement for labels - first calculated extension point out from array.
      x = center.x + radius * 1.04f * cosTheta;
      y = center.y + radius * 1.04f * sinTheta;

      // get size of label for this axes
      LPCTSTR label = pLine->pDataObj->GetLabel( pLine->dataCol );
      CSize sz = dc.GetTextExtent( label, lstrlen( label ) );

      // figure out where the center of the string should be
      labelPtArray[ i ].x = (long) ( x + cosTheta * sz.cx / 2 );
      labelPtArray[ i ].y = (long) ( y + sinTheta * sz.cy / 2 );
      }

   // draw the frame polygon
   dc.SelectStockObject( WHITE_BRUSH );
   dc.SelectStockObject( BLACK_PEN );
   dc.Polygon( ptArray, axes );
   
   // draw the starting polygon
   //if( m_pDataObj->GetRowCount() > 0 )
      {
      CPoint *startArray = new CPoint[ axes ];
      for ( int i=0; i < axes; i++ )
         {
         LINE *pLine = m_lineArray[ i ];
         ASSERT( pLine->pDataObj );
         ASSERT( pLine->pDataObj->GetRowCount() > 0 );

         float v = pLine->pDataObj->GetAsFloat( pLine->dataCol, 0 );
         float theta = rotation * i;
         float range = pLine->scaleMax - pLine->scaleMin;
         float scalar = (v-pLine->scaleMin)/range;
         float x = (float) (center.x + radius * scalar * cos( theta ));
         float y = (float) (center.y + radius * scalar * sin( theta ));

         startArray[ i ].x = (int) x;
         startArray[ i ].y = (int) y;
         }

      CBrush brush( RGB( 0xCC, 0xCC, 0xFF ) );
      CPen pen( PS_SOLID, 1, BLUE );

      dc.SelectObject( &brush );
      dc.SelectObject( &pen );
      dc.Polygon( startArray, axes );
      dc.SelectStockObject( NULL_BRUSH );
      dc.SelectStockObject( NULL_PEN );
      delete [] startArray;
      }

   // draw the axes now
   dc.SelectStockObject( BLACK_PEN );
   dc.SelectStockObject( NULL_BRUSH );

   for ( int i=0; i < axes; i++ )
      {
      LINE *pLine = m_lineArray[i];

      dc.MoveTo( center );
      dc.LineTo( ptArray[ i ] );

      // draw labels next

      // are we "above" the axis?
      if ( labelPtArray[ i ].y > ptArray[ i ].y )
         dc.SetTextAlign( TA_BASELINE | TA_CENTER );
      else
         dc.SetTextAlign( TA_TOP | TA_CENTER );

      LPCTSTR label = pLine->pDataObj->GetLabel( pLine->dataCol );
      dc.TextOut( labelPtArray[ i ].x, labelPtArray[ i ].y, label, lstrlen( label ) );
      }

   // draw the last row
   CPen pen;
   pen.CreatePen( PS_SOLID, 4, m_currentStateColor );
   CPen *pOldPen = dc.SelectObject( &pen );
   
   int lastRow = m_row;
   if ( m_row == -1 )
      {
      ASSERT( m_pDataObj );
      lastRow = m_pDataObj->GetRowCount()-1;
      }

   if ( lastRow >= 0 && m_lineArray.GetSize() > 0 )
      {
      LINE *pLine;
      pLine = m_lineArray[ 0 ];
      ASSERT( pLine->pDataObj );
      ASSERT( pLine->pDataObj->GetRowCount() > lastRow );

      // get first point
      float v = pLine->pDataObj->GetAsFloat( pLine->dataCol, lastRow );
      float range = pLine->scaleMax - pLine->scaleMin;
      float scalar = (v-pLine->scaleMin)/range;
      float x = center.x + radius * scalar;
      float y = (float) center.y;
      CPoint pt( (int) x, (int) y );
      dc.MoveTo( pt );

      for ( int i=1; i < axes; i++ )
         {
         pLine = m_lineArray[ i ];
         ASSERT( pLine->pDataObj );
         ASSERT( pLine->pDataObj->GetRowCount() > lastRow );

         v = pLine->pDataObj->GetAsFloat( pLine->dataCol, lastRow );
         float range = pLine->scaleMax - pLine->scaleMin;
         float scalar = (v-pLine->scaleMin)/range;
         float theta = rotation * i;
         x = (float) (center.x + radius * scalar * cos( theta ) );
         y = (float) (center.y + radius * scalar * sin( theta ) );
         dc.LineTo( (int) x, (int) y );
         }

      dc.LineTo( pt );
      }

   dc.SelectObject( pOldFont );
   dc.SelectObject( pOldPen );
   delete [] ptArray;
   delete [] labelPtArray;
   }


//-- RadarGraphWnd::UpdatePlot() ------------------------------------------
//
//   Draws actual data on the plot
//-------------------------------------------------------------------

bool RadarGraphWnd::UpdatePlot( CDC *pDC )
   {
   if ( IsIconic() )
      return true;

   InitMapping( pDC );

   DrawPlot( *pDC );
   return true;
   }



void RadarGraphWnd::OnSysCommand(UINT nID, LPARAM lParam)
   {   
   GraphWnd::OnSysCommand(nID, lParam);
   }
