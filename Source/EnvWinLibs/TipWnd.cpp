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
// TipWnd.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include <TipWnd.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TipWnd
//#define TIPWND_CLASSNAME _T("ZTipWnd")

///LPCTSTR TipWnd::m_wndClass = NULL;
///
///
///TipWnd::TipWnd()
///: m_border( 2 ),
///  m_fontSize( 80 )
///   {
///   if ( m_wndClass == NULL )
///      m_wndClass = AfxRegisterWndClass( 0, 0, 0, 0 );
///   }
///
///TipWnd::~TipWnd()
///{
///}
///
///
///BEGIN_MESSAGE_MAP(TipWnd, CWnd)
///   //{{AFX_MSG_MAP(TipWnd)
///   ON_WM_PAINT()
///   //}}AFX_MSG_MAP
///END_MESSAGE_MAP()
///
///
////////////////////////////////////////////////////////////////////////////////
///// TipWnd message handlers
///
///BOOL TipWnd::Create( int x, int y, CWnd* pParentWnd, UINT nID /* DWORD style= WS_CHILD | WS_BORDER | WS_VISIBLE */ )
///   {
///   // note: x, y, are in client coords of the parent.  convert ot screen coords before creating popup   
///   CFont font;
///   font.CreatePointFont( m_fontSize, "Arial", NULL );
///   CDC *pDC = pParentWnd->GetDC();
///   CFont *pOldFont = pDC->SelectObject( &font );
///   CSize size = pDC->GetTextExtent( "Z" );
///   pParentWnd->ReleaseDC( pDC );
///
///   RECT rect;
///   rect.left = 0;
///   rect.top = 0;
///   rect.right = size.cx * 20;
///   rect.bottom = size.cy + m_border*2;
///
///   m_pAttachedWnd = pParentWnd;
///   //m_pAttachedWnd->ClientToScreen( &rect );
///
///   //return CWnd::CreateEx( WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TIPWND_CLASSNAME, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 
///   //CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL );
///   //int retVal = CWnd::CreateEx( NULL, m_wndClass, "", WS_POPUP | WS_BORDER | WS_VISIBLE, rect, pParentWnd, nID );
///   //int retVal = CWnd::CreateEx( NULL, NULL, "", WS_CHILD | WS_BORDER | WS_VISIBLE, rect, pParentWnd, nID );
///   int retVal = CWnd::Create( NULL, "Tip", WS_CHILD | WS_BORDER | WS_VISIBLE, rect, pParentWnd, nID, NULL );
///
///   return retVal;
///   }
///
///
///void TipWnd::UpdateText(LPCTSTR text)
///   {
///   m_text = text;
///   RedrawWindow();
///   }
///
///
///void TipWnd::OnPaint() 
///   {
///   CPaintDC dc(this); // device context for painting
///
///   CFont font;
///   font.CreatePointFont( m_fontSize, "Arial", NULL );
///
///   CFont *pOldFont = dc.SelectObject( &font );
///
///   CBrush bkgr;
///   bkgr.CreateSolidBrush( RGB( 0xFF, 0xFF, 0xC0 ) );
///   
///   CPen *pOldPen = (CPen*) dc.SelectStockObject( NULL_PEN );
///   CBrush *pOldBrush = dc.SelectObject( &bkgr );
///
///   RECT rect;
///   GetClientRect( &rect );
///
///   dc.Rectangle( &rect );
///
///   dc.SetBkMode( TRANSPARENT );
///   dc.SetTextAlign( TA_CENTER | TA_TOP );
///
///   dc.TextOut( ( rect.right+rect.left )/2, m_border, m_text );
///   
///   dc.SelectObject( pOldFont );
///   dc.SelectObject( pOldPen );
///   dc.SelectObject( pOldBrush );
///   // Do not call CWnd::OnPaint() for painting messages
///   }
///
///void TipWnd::Move(int x, int y)
///   {
///   // note: x, y, are in client coords of the parent.  convert ot screen coords before creating popup
///   POINT p;
///   p.x = x;
///   p.y = y;
///
///   RECT rect;
///   GetWindowRect( &rect );
///
///   int width  = rect.right  - rect.left;
///   int height = rect.bottom - rect.top;
///
///   //m_pAttachedWnd->ClientToScreen( &p );
///   MoveWindow( p.x, p.y, width, height, TRUE );
///   //RedrawWindow();
///   }
///


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE

//static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(TipWnd,CWnd)
BEGIN_MESSAGE_MAP(TipWnd,CWnd)
   ON_WM_PAINT()
   ON_MESSAGE(WM_SETTEXT, OnSetText)
   ON_WM_TIMER()
END_MESSAGE_MAP()

TipWnd::TipWnd()
{
   m_szMargins = CSize(4,4);
   // create font ? use system tooltip font
   CNonClientMetrics ncm;
   m_font.CreateFontIndirect(&ncm.lfStatusFont);
}

TipWnd::~TipWnd()
{
}

// Create window. pt is upper-left or upper-right corner depending on 
// nStyle.
//
int TipWnd::Create(CPoint pt, CWnd* pParentWnd, UINT nStyle, UINT nID)
{
   m_nStyle = nStyle;
   return CreateEx(0,
      NULL,
      NULL,
      WS_POPUP|WS_VISIBLE,
      CRect(pt,CSize(0,0)),
      pParentWnd,
      nID);
}

// Someone changed the text: resize to fit new text
//
LRESULT TipWnd::OnSetText(WPARAM wp, LPARAM lp)
{
   CDC *pDC = this->GetDC();
   CFont* pOldFont = pDC->SelectObject(&m_font);
   CRect rc;
   GetWindowRect(&rc);
   int x = (m_nStyle & JUSTIFYRIGHT) ? rc.right : rc.left;
   int y = rc.top;
   pDC->DrawText(CString((LPCTSTR)lp), &rc, DT_CALCRECT);
   rc.InflateRect(m_szMargins);
   if (m_nStyle & JUSTIFYRIGHT)
      x -= rc.Width();
   SetWindowPos(NULL,x,y,rc.Width(),rc.Height(), SWP_NOZORDER|SWP_NOACTIVATE);
   return Default();
}

// Paint the text. Use system colors
//
void TipWnd::OnPaint()
{
   CPaintDC dc(this);
   CRect rc;
   GetClientRect(&rc);
   CString s;
   GetWindowText(s);
   CBrush b(GetSysColor(COLOR_INFOBK)); // use tooltip bg color
   dc.FillRect(&rc, &b);

   // draw text
   dc.SetBkMode(TRANSPARENT);
   CFont* pOldFont = dc.SelectObject(&m_font);
   dc.SetTextColor(GetSysColor(COLOR_INFOTEXT)); // tooltip text color
   dc.DrawText(s, &rc, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
   dc.SelectObject(pOldFont);
}

// Register class if needed
//
BOOL TipWnd::PreCreateWindow(CREATESTRUCT& cs) 
{
   static CString sClassName;
   if (sClassName.IsEmpty())
      sClassName = AfxRegisterWndClass(0);
   cs.lpszClass = sClassName;
   cs.style = WS_POPUP|WS_BORDER;
   cs.dwExStyle |= WS_EX_TOOLWINDOW;
   return CWnd::PreCreateWindow(cs);
}

// TipWnd is intended to be used on the stack,
// not heap, so don't auto-delete.
//
void TipWnd::PostNcDestroy()
{
   // don't delete this
}

// Show window with delay. No delay means show now.
//
void TipWnd::ShowDelayed(UINT msec)
{
   if (msec==0)
      // no delay: show it now
      OnTimer(1);
   else
      // delay: set time
      SetTimer(1, msec, NULL);
}

// Cancel text?kill timer and hide window
//
void TipWnd::Cancel()
{
   KillTimer(1);
   ShowWindow(SW_HIDE);
}

// Timer popped: display myself and kill timer
//
void TipWnd::OnTimer(UINT_PTR nIDEvent)
{
   ShowWindow(SW_SHOWNA);
   Invalidate();
   UpdateWindow();
   KillTimer(1);
}
