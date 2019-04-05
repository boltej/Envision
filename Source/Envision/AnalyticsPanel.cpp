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
// AnalyticsPanel.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "AnalyticsPanel.h"

#include <misc.h>

//BEGIN_MESSAGE_MAP( AnalyticsPanel, CWnd )
//   ON_WM_SIZE()
//END_MESSAGE_MAP()
//

//BOOL CALLBACK MoveChildrenProc( HWND hwnd, LPARAM lParam );
//
//
//BOOL CALLBACK MoveViewerChildrenProc( HWND hWnd, LPARAM lParam )
//   {
//   HWND hParent = (HWND) lParam;
//   //CRect *pRect = (CRect*) lParam;
//
//   // only true 
//   if ( ::GetParent( hWnd ) == hParent )
//      {
//      CRect rect;
//      ::GetClientRect( hParent, &rect );
//      ::MoveWindow( hWnd, 0, 0, rect.right, rect.bottom, TRUE );
//      }
//
//   return TRUE;
//   }


// AnalyticsPanel

//IMPLEMENT_DYNAMIC(AnalyticsPanel, CWnd)

AnalyticsPanel::~AnalyticsPanel()
{
}


BEGIN_MESSAGE_MAP(AnalyticsPanel, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
   ON_WM_PAINT()
END_MESSAGE_MAP()


// AnalyticsPanel message handlers

int AnalyticsPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_font.CreatePointFont( 80, "Arial" );
   CDC *pDC = GetDC();
   CGdiObject *pOld = pDC->SelectObject( &m_font );
   TEXTMETRIC tm;
   pDC->GetTextMetrics( &tm );
   int height = tm.tmHeight * 4 / 3;
   pDC->SelectObject( pOld );
   ReleaseDC( pDC );

   CRect rect;
   GetViewRect( &rect );
   rect.top = rect.bottom - height;

   BOOL ok = m_tabCtrl.Create( WS_CHILD | WS_VISIBLE | CTCS_AUTOHIDEBUTTONS, rect, this, 1000 );
   ASSERT( ok );

   LOGFONT logFont;
   m_font.GetLogFont( &logFont );
   m_tabCtrl.SetControlFont( logFont );

   return 0;
   }


void AnalyticsPanel::GetViewRect( RECT *rect )
   {
   GetClientRect( rect );

   //RECT tbRect;
   //this->m_viewBar.GetWindowRect( &tbRect );
   //int tbHeight = tbRect.bottom - tbRect.top;

   //rect->top += tbHeight;
   }



AnalyticsViewer *AnalyticsPanel::AddViewer()
   {
   int tab = m_tabCtrl.GetItemCount();
   
   m_tabCtrl.InsertItem( tab, "Analytics Viewer" ); //pViz->name );
   m_tabCtrl.SetCurSel( tab );

   CRect tabRect;
   m_tabCtrl.GetItemRect( 0, tabRect );

   CRect rect;
   GetViewRect( &rect );

   rect.bottom -= tabRect.Height();

   AnalyticsViewer *pWnd = new AnalyticsViewer;      // this is the parent container for the visualizer
   //pWnd->m_pViz = pViz;
   pWnd->Create( NULL, "Analytics Viewer", WS_CHILD | WS_BORDER | WS_VISIBLE, rect, this, 2000+tab );

   this->m_viewerArray.Add( pWnd );

   return pWnd;
   }


AnalyticsViewer *AnalyticsPanel::GetActiveView( void )
   {
   int index = m_tabCtrl.GetCurSel();

   if ( index < 0 )
      return NULL;
   else
      return m_viewerArray[ index ];
   }


void AnalyticsPanel::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);

   CRect rect;
   GetViewRect( &rect );

   CRect tabRect;
   m_tabCtrl.GetWindowRect( &tabRect );
   int tabHeight = tabRect.Height();

   rect.top = rect.bottom - tabHeight;
   m_tabCtrl.MoveWindow( &rect );

   GetViewRect( &rect );
   rect.bottom -= tabHeight;

   if ( m_tabCtrl.GetItemCount() > 0 )
      {
      for ( int i=0; i < m_viewerArray.GetSize(); i++ )
         m_viewerArray[ i ]->MoveWindow( rect );
      } 
   }


BOOL AnalyticsPanel::OnEraseBkgnd(CDC* pDC)
   {
   RECT rect;
   GetClientRect( &rect );
   pDC->SelectStockObject( WHITE_BRUSH );
   pDC->Rectangle( &rect );
   return TRUE;
   }


BOOL AnalyticsPanel::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
   {
   NMHDR *pNotify = (NMHDR*) lParam;

   switch( pNotify->idFrom )
      {
      case 1000:   // m_tabCtrl
         {
         CTC_NMHDR *pNMH = (CTC_NMHDR*) lParam;
         ChangeTabSelection( pNMH->nItem );
         }
         break;
      }

   return CWnd::OnNotify(wParam, lParam, pResult);
   }


void AnalyticsPanel::ChangeTabSelection( int index )
   {
   AnalyticsViewer *pWnd = NULL;
   int count = (int) m_viewerArray.GetCount();

   if ( 0 <= index && index < count )
      {
      for ( int i=0; i < count; i++ )
         {
         pWnd = m_viewerArray[ i ];

         if ( i == index )
            {
            //m_viewBar.m_slider.SetPos( pPanel->m_year, pPanel->m_totalYears );

            pWnd->ShowWindow( SW_SHOW );
            //gpActiveWnd = pTab;
            }
         else
            {
            pWnd->ShowWindow( SW_HIDE );
            }
         }
      }
   }



void AnalyticsPanel::OnPaint()
   {
   if ( m_viewerArray.GetSize() == 0 )
      {
      CPaintDC dc(this); // device context for painting
      CFont font;
      font.CreatePointFont( 90, "Arial" );
      CFont *pFont = dc.SelectObject( &font );
      CString text( "No Analytics Viewers specified." );
      dc.TextOut( 15, 15, text );
      dc.SelectObject( pFont );
      }
   }



void AnalyticsPanel::Tile() 
   {
   int count = (int) m_viewerArray.GetSize();

   if ( count == 0 )
      return;

   CRect rect;
   GetClientRect( rect );

   CArray< CRect, CRect& > rects;
   GetTileRects( count, &rect, rects );

   for ( int i=0; i < count; i++ )
      {
      CWnd *pWnd = m_viewerArray[ i ];
      CRect &_rect = rects[ i ];

      pWnd->MoveWindow( _rect.left, _rect.top, _rect.Width(), _rect.Height() );
      }
   }


void AnalyticsPanel::Cascade() 
   {
   int count = (int) m_viewerArray.GetSize();

   if ( count == 0 )
      return;
   
   RECT rect;
   GetClientRect( &rect );

   int borderHeight = GetSystemMetrics( SM_CYSIZEFRAME );
   int captionHeight = GetSystemMetrics( SM_CYCAPTION );
   int totalHeight = borderHeight + captionHeight + 1;

   int height = rect.bottom - totalHeight*(count-1);
   int width  = rect.right - totalHeight*(count-1);

   int x = 0;
   int y = 0;

   for ( int i=0; i < count; i++ )
      {
      CWnd *pWnd = m_viewerArray[ i ];

      if ( !IsWindow( pWnd->m_hWnd ) )
         ASSERT( IsWindow( pWnd->m_hWnd ) );
      
      pWnd->MoveWindow( x, y, width, height );

      x += totalHeight;
      y += totalHeight;

      pWnd->BringWindowToTop();
      }
   }

