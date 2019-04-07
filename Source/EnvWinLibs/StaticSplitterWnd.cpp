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
// StaticSplitterWnd.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "StaticSplitterWnd.h"
#include ".\staticsplitterwnd.h"


// StaticSplitterWnd

IMPLEMENT_DYNAMIC(StaticSplitterWnd, CWnd)
StaticSplitterWnd::StaticSplitterWnd()
:  m_pLeft( NULL ),
   m_pRight( NULL ),
   m_barWidth( 3 ),
   m_bar( 150 ),
   m_inBarMove( false ),
   m_mouseBarOffset( 0 )
   {
   }

StaticSplitterWnd::~StaticSplitterWnd()
   {
   }

BOOL StaticSplitterWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   if ( !CWnd::PreCreateWindow(cs) )
      return false;

   cs.style |= WS_CLIPCHILDREN;

   cs.lpszClass = AfxRegisterWndClass( CS_VREDRAW | CS_HREDRAW, 
		::LoadCursor(NULL, IDC_SIZEWE), reinterpret_cast<HBRUSH>(COLOR_WINDOW), NULL);

   return true;
   }

int StaticSplitterWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_pLeft->CreateEx( WS_EX_CLIENTEDGE, NULL, NULL, WS_VISIBLE | WS_BORDER | WS_CHILD, CRect(0,0,0,0), this, 2001 );
   m_pRight->CreateEx( WS_EX_CLIENTEDGE, NULL, NULL, WS_VISIBLE | WS_BORDER | WS_CHILD, CRect(0,0,0,0), this, 2002 );

   return 0;
   }

void StaticSplitterWnd::MoveBar( int newLocation )
   {
   ASSERT( 0 <= newLocation );
   if ( newLocation < 0 )
      return;

   m_bar = newLocation;
   }

void StaticSplitterWnd::LeftRight( CWnd *pLeft, CWnd *pRight )
   {
   ASSERT( m_pLeft == NULL );
   ASSERT( m_pRight == NULL );

   m_pLeft = pLeft;
   m_pRight = pRight;
   }

    

BEGIN_MESSAGE_MAP(StaticSplitterWnd, CWnd)
   ON_WM_SIZE()
   ON_WM_CREATE()
   ON_WM_MOUSEMOVE()
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

// StaticSplitterWnd message handlers

void StaticSplitterWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);

   Draw();
   }

void StaticSplitterWnd::OnMouseMove(UINT nFlags, CPoint point)
   {
   if ( m_inBarMove )
      {
      CRect rect;
      GetClientRect( rect );

      int newLocation = point.x - m_mouseBarOffset;

      if ( newLocation < 4 )
         newLocation = 4;
      if ( newLocation > rect.Width() - 4 - m_barWidth )
         newLocation = rect.Width() - 4 - m_barWidth;

      m_bar = newLocation;
      Draw();
      }

   CWnd::OnMouseMove(nFlags, point);
   }

void StaticSplitterWnd::OnLButtonDown(UINT nFlags, CPoint point)
   {
   ASSERT( m_inBarMove == false );

   m_inBarMove = true;
   SetCapture();

   m_mouseBarOffset = point.x - m_bar;

   CWnd::OnLButtonDown(nFlags, point);
   }

void StaticSplitterWnd::OnLButtonUp(UINT nFlags, CPoint point)
   {
   if ( m_inBarMove )
      {
      ASSERT( GetCapture() == this );
      m_inBarMove = false;
      ReleaseCapture();
      }

   CWnd::OnLButtonUp(nFlags, point);
   }

void StaticSplitterWnd::Draw()
   {
   CRect rect;
   GetClientRect( rect );

   int cx = rect.Width();
   int cy = rect.Height();

   if ( m_pLeft != NULL && m_pRight != NULL && IsWindow( m_pLeft->m_hWnd ) && IsWindow( m_pRight->m_hWnd ) )
      {
      m_pLeft->MoveWindow( 0, 0, m_bar, cy );
      m_pRight->MoveWindow( m_bar + m_barWidth, 0, cx - m_bar - m_barWidth, cy );
      }
   }
