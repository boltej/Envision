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
// ScatterLegendWnd.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "plot3d/ScatterLegendWnd.h"

#include "plot3d/ScatterPlot3DWnd.h"

// ScatterLegendWnd

IMPLEMENT_DYNAMIC(ScatterLegendWnd, CWnd)
ScatterLegendWnd::ScatterLegendWnd()
   : m_pDataArray( NULL ),
     m_selection( -1 ),
     m_pParent( NULL ),
     m_length(0)
   {
   m_font.CreatePointFont( 90, "Arial" );
   }

ScatterLegendWnd::~ScatterLegendWnd()
   {
   ClearPreviewWndArray();
   }

void ScatterLegendWnd::ClearPreviewWndArray( void )
   {
   for ( int i=0; i< m_previewWndArray.GetCount(); i++ )
      {
      ShapePreviewWnd *pWnd = m_previewWndArray.GetAt(i);
      if ( IsWindow( pWnd->m_hWnd ) )
         pWnd->DestroyWindow();
      delete pWnd;
      }

   m_previewWndArray.RemoveAll();
   }

BEGIN_MESSAGE_MAP(ScatterLegendWnd, CWnd)
   ON_WM_PAINT()
   ON_WM_RBUTTONDOWN()
   ON_WM_CREATE()
   ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


int ScatterLegendWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_pParent = (ScatterPlot3DWnd*) GetParent();

   return 0;
   }

void ScatterLegendWnd::SetPointCollectionArray( PointCollectionArray *pCollection )
   { 
   m_pDataArray = pCollection; 
   ClearPreviewWndArray();

   for ( int i=0; i<m_pDataArray->GetCount(); i++ )
      {
      ShapePreviewWnd *pPreviewWnd = new ShapePreviewWnd();
      PointCollection *pCol = m_pDataArray->GetAt(i);
      
      pPreviewWnd->SetPointCollection( pCol );
      pPreviewWnd->SetAllowMouseRotation( false );

      BOOL ok = pPreviewWnd->Create(NULL, NULL, WS_VISIBLE, CRect(0,0,0,0), this, 5555+i );
      if ( ok )
         m_previewWndArray.Add( pPreviewWnd );
      else
         delete pPreviewWnd;
      }
   }


// ScatterLegendWnd message handlers


BOOL ScatterLegendWnd::PreCreateWindow(CREATESTRUCT& cs)
   {

   if ( !CWnd::PreCreateWindow(cs) )
		return false;

   cs.style |= WS_CLIPCHILDREN;

	cs.lpszClass = AfxRegisterWndClass( CS_VREDRAW | CS_HREDRAW,
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

   return true;
   }

void ScatterLegendWnd::OnPaint()
   {
   CPaintDC dc(this); // device context for painting
   
   if ( m_pDataArray != NULL )
      {
      
      dc.SelectObject( &m_font );
            
      CSize zSize = dc.GetTextExtent( "Z", 1 );
      int textHeight = zSize.cy;
      m_length = max( textHeight, m_previewLength );;
      int textOffset = ( m_length - textHeight )/2;

      if ( m_selection >= 0 )
         {
         CRect rect;
         GetClientRect( &rect );

         CBrush brush( RGB(100,200,255) );
         
         int y = m_selection * ( m_length + m_gutter ) + m_gutter;
            dc.FillRect( CRect( 0, y-m_gutter/2, rect.Width(), y+m_length+m_gutter/2 ), &brush );
         }

      CPoint pt1( m_gutter, m_gutter );
      CPoint pt2( 2*m_gutter + m_length + m_length/2, m_gutter + textOffset );

      int dataCount    = (int) m_pDataArray->GetCount();
      int previewCount = (int) m_previewWndArray.GetCount();

      if ( dataCount != previewCount )
         SetPointCollectionArray( m_pDataArray );

      for ( int i=0; i< dataCount; i++ )
         {
         PointCollection *pCol = m_pDataArray->GetAt(i);
         ShapePreviewWnd *pPreviewWnd = m_previewWndArray.GetAt(i);

         ASSERT( IsWindow( pPreviewWnd->m_hWnd ) );
         pPreviewWnd->MoveWindow( pt1.x, pt1.y, m_length + m_length/2, m_length );
     
         dc.TextOut( pt2.x, pt2.y, pCol->GetName() );

         pt1.Offset( 0, m_length + m_gutter );
         pt2.Offset( 0, m_length + m_gutter );

         pPreviewWnd->Invalidate();
         }

      }
   }

CSize ScatterLegendWnd::GetIdealSize()
   {
   CDC *pDC = GetDC();

   CSize size(0,0);

   if ( m_pDataArray != NULL )
      {
      
      pDC->SelectObject( &m_font );
            
      CSize zSize = pDC->GetTextExtent( "Z", 1 );
      int textHeight = zSize.cy;
      m_length = max( textHeight, m_previewLength );

      size.cy = LONG( m_pDataArray->GetCount()*( m_length + m_gutter ) + m_gutter );

      int width = 0;

      for ( int i=0; i< m_pDataArray->GetCount(); i++ )
         {
         PointCollection *pCol = m_pDataArray->GetAt(i);

         CSize extent = pDC->GetTextExtent( pCol->GetName() );

         if ( extent.cx > width )
            width = extent.cx;
         }

      size.cx = width + m_length + m_length/2 + 3*m_gutter;

      }

   ReleaseDC( pDC );

   return size;
   }
void ScatterLegendWnd::OnRButtonDown(UINT nFlags, CPoint point)
   {
   ClientToScreen( &point );

//   m_pParent->PopUpMenu( point );

   CWnd::OnRButtonDown(nFlags, point);
   }


void ScatterLegendWnd::OnLButtonDown(UINT nFlags, CPoint point)
   {
   //int sel = -1;
   //int bottom = 0;

   //int pt = point.y;

   //for ( int i = 0; i < m_pDataArray->GetCount(); i++ )
   //   {
   //   sel++;
   //   bottom += m_length + m_gutter;
   //   if ( pt < bottom )
   //      break;
   //   
   //   }

   //m_selection = sel;
   //Invalidate();

   CWnd::OnLButtonDown(nFlags, point);
   }
