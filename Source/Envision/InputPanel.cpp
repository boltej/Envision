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
// InputPanel.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "InputPanel.h"


BEGIN_MESSAGE_MAP( InputWnd, CWnd )
   ON_WM_SIZE()
END_MESSAGE_MAP()


BOOL CALLBACK MoveChildrenProc( HWND hwnd, LPARAM lParam );


BOOL CALLBACK MoveInputChildrenProc( HWND hWnd, LPARAM lParam )
   {
   HWND hParent = (HWND) lParam;
   //CRect *pRect = (CRect*) lParam;

   // only true 
   if ( ::GetParent( hWnd ) == hParent )
      {
      CRect rect;
      ::GetClientRect( hParent, &rect );
      ::MoveWindow( hWnd, 0, 0, rect.right, rect.bottom, TRUE );
      }

   return TRUE;
   }



void InputWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize( nType, cx, cy );
   CRect rect( 0, 0, cx, cy );
   ::EnumChildWindows( GetSafeHwnd(), MoveInputChildrenProc, (LPARAM) this->m_hWnd );
   }


// InputPanel

//IMPLEMENT_DYNAMIC(InputPanel, CWnd)

InputPanel::~InputPanel()
{
}


BEGIN_MESSAGE_MAP(InputPanel, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
   ON_WM_PAINT()
END_MESSAGE_MAP()


// InputPanel message handlers

int InputPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
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


void InputPanel::GetViewRect( RECT *rect )
   {
   GetClientRect( rect );

   //RECT tbRect;
   //this->m_viewBar.GetWindowRect( &tbRect );
   //int tbHeight = tbRect.bottom - tbRect.top;

   //rect->top += tbHeight;
   }



InputWnd *InputPanel::AddInputWnd( EnvVisualizer *pViz )
   {
   int tab = m_tabCtrl.GetItemCount();
   
   m_tabCtrl.InsertItem( tab, pViz->m_name );
   m_tabCtrl.SetCurSel( tab );

   CRect tabRect;
   m_tabCtrl.GetItemRect( 0, tabRect );

   CRect rect;
   GetViewRect( &rect );

   rect.bottom -= tabRect.Height();

   InputWnd *pWnd = new InputWnd;      // this is the parent container for the visualizer
   pWnd->m_pViz = pViz;
   pWnd->Create( NULL, "Input Window", WS_CHILD | WS_BORDER | WS_VISIBLE, rect, this, 2000+tab );

   this->m_inputWndArray.Add( pWnd );

   return pWnd;
   }


InputWnd *InputPanel::GetActiveWnd( void )
   {
   int index = m_tabCtrl.GetCurSel();

   if ( index < 0 )
      return NULL;
   else
      return m_inputWndArray[ index ];
   }


void InputPanel::OnSize(UINT nType, int cx, int cy)
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
      for ( int i=0; i < m_inputWndArray.GetSize(); i++ )
         m_inputWndArray[ i ]->MoveWindow( rect );
      } 
   }


BOOL InputPanel::OnEraseBkgnd(CDC* pDC)
   {
   RECT rect;
   GetClientRect( &rect );
   pDC->SelectStockObject( WHITE_BRUSH );
   pDC->Rectangle( &rect );
   return TRUE;
   }


BOOL InputPanel::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
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


void InputPanel::ChangeTabSelection( int index )
   {
   InputWnd *pWnd = NULL;
   int count = (int) m_inputWndArray.GetCount();

   if ( 0 <= index && index < count )
      {
      for ( int i=0; i < count; i++ )
         {
         pWnd = m_inputWndArray[ i ];

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



void InputPanel::OnPaint()
   {
   if ( m_inputWndArray.GetSize() == 0 )
      {
      CPaintDC dc(this); // device context for painting
      CFont font;
      font.CreatePointFont( 90, "Arial" );
      CFont *pFont = dc.SelectObject( &font );
      CString text( "No Input Visualizers specified." );
      dc.TextOut( 15, 15, text );
      dc.SelectObject( pFont );
      }
   }
