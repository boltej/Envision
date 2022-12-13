// InfoBar.cpp : implementation file
//

#include "pch.h"
#include "NetViewer.h"
#include "InfoBar.h"

#include <colors.hpp>
#include <ColorRamps.h>

// InfoBar

IMPLEMENT_DYNAMIC(InfoBar, CWnd)

InfoBar::InfoBar()
{

}

InfoBar::~InfoBar()
{
}


BEGIN_MESSAGE_MAP(InfoBar, CWnd)
   ON_WM_PAINT()
END_MESSAGE_MAP()



// InfoBar message handlers

void InfoBar::OnPaint()
   {
   CPaintDC dc(this); // device context for painting

   CRect rect;
   this->GetClientRect(&rect);

   int width = rect.Width()/4;
   int height = rect.Height();

   CPen pen(PS_SOLID, 1, BLACK);
   CPen* pOldPen = dc.SelectObject(&pen);
   dc.Rectangle(rect);

   for (int i = 1; i < width - 1; i++)
      {
      RGBA color = ::WRColorRamp(0, width, i);
      COLORREF _color = RGB(color.r, color.g, color.b);
      pen.DeleteObject();
      pen.CreatePen(PS_SOLID, 1, _color);
      dc.SelectObject(&pen);
      dc.MoveTo(i,1);
      dc.LineTo(i,height-1);
      }

   dc.SelectObject(pOldPen);
   pen.DeleteObject();

   // Do not call CWnd::OnPaint() for painting messages
   }

