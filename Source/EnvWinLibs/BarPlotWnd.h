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
#pragma once

#include "graphwnd.h"


// BarPlotWnd

// Notes about Bar Plot Styles:
//   1) each BAR GROUP on the plot corresponds to a ROW in the attached data object.
//   2) STACKS for each bar correspond to LINES (DataObj cols) defiends for the plot

enum BPSTYLE 
   { 
   BPS_GROUPED,   // stacks are placed groups adjacent to each other
   BPS_STACKED,   // stacks are placed on top of each other
   BPS_SINGLE     // only on stack is display, selected from combo box (if more than one)
   };


#define MS_BAR2DSOLID   MS_SOLIDSQUARE
#define MS_BAR2DHOLLOW  MS_HOLLOWSQUARE


class WINLIBSAPI BarPlotWnd : public GraphWnd
{
	DECLARE_DYNAMIC(BarPlotWnd)

public:
	BarPlotWnd();
   BarPlotWnd( DataObj *pData, bool makeDataLocal=false );

	virtual ~BarPlotWnd();

   void SetBarPlotStyle( BPSTYLE, int extra=-1 );
   void LoadCombo();


protected:
	DECLARE_MESSAGE_MAP()
   //afx_msg void OnPaint();

   BPSTYLE m_style;     // defaults to BPS_STACKED
   CComboBox m_combo;

   virtual void DrawPlotArea( CDC *pDC );     // note: hDC should be initMapped when this function is called
   virtual bool UpdatePlot( CDC *pDC, bool, int startIndex=0, int numPoints=-1 );

   void DrawBar( HDC hDC, int left, int top, int right, int bottom, MSTYLE style, COLORREF color );

public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnComboChange();
   };


