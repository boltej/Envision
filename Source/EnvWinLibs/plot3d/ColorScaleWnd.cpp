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
// ColorScaleWnd.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "plot3d/ColorScaleWnd.h"


// ColorScaleWnd

IMPLEMENT_DYNAMIC(ColorScaleWnd, CWnd)
ColorScaleWnd::ColorScaleWnd()
{
m_colorScale.SetBaseColor( RGB(0,255,0) );
m_font.CreatePointFont( 90, "Arial" );
}

ColorScaleWnd::~ColorScaleWnd()
{
}


BEGIN_MESSAGE_MAP(ColorScaleWnd, CWnd)
   ON_WM_SIZE()
   ON_WM_PAINT()
END_MESSAGE_MAP()



// ColorScaleWnd message handlers


void ColorScaleWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);

   m_height = cy;
   m_width = cx;
   }

void ColorScaleWnd::OnPaint()
   {
   CPaintDC dc(this); // device context for painting
   // TODO: Add your message handler code here
   // Do not call CWnd::OnPaint() for painting messages

   dc.SelectObject( &m_font );

   CSize zSize = dc.GetTextExtent( "Z", 1 );
   //int textHeight = zSize.cy;

   int brushHeight = zSize.cy;
   static int brushWidth  = 30;
   static int num = 25;

   for ( int i=0; i<num; i++ )
      {
      CBrush brush( m_colorScale.GetColor( m_colorScale.GetMax()*(1.0f-float(i)/float(num)) ) );
      dc.FillRect( CRect( 0, i*brushHeight, brushWidth, (i+1)*brushHeight ), &brush );
      }

   dc.SetTextAlign( TA_TOP );

   CString label;
   label.Format( "%g", m_colorScale.GetMax() );
   dc.TextOut( brushWidth, 0, label );

   label.Format( "%g", m_colorScale.GetMin() );
   dc.TextOut( brushWidth, (num-1)*brushHeight , label );

   }

BOOL ColorScaleWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   cs.lpszClass = AfxRegisterWndClass(NULL, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

   return CWnd::PreCreateWindow(cs);
   }
