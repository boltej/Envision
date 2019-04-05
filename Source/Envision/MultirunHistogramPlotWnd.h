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

#include <Histogram.h>
#include "BarPlotWnd.h"


// MultirunHistogramPlotWnd

class MultirunHistogramPlotWnd : public CWnd
{
DECLARE_DYNAMIC(MultirunHistogramPlotWnd)

public:
	MultirunHistogramPlotWnd( int multirun );
	virtual ~MultirunHistogramPlotWnd();

   int m_multirun;
   int m_year;
   int m_totalYears;

   BarPlotWnd m_plot;

   CSliderCtrl m_yearSlider;
   CEdit       m_yearEdit;
   CComboBox   m_resultCombo;
   
   static bool Replay( CWnd *pWnd, int flag );
   bool _Replay( int flag );

   void ComputeFrequencies( std::vector< float > xVector, int binCount );

   bool m_showLine;     // determines if a interpolation line is shown

protected:
   void RefreshData( void );

	DECLARE_MESSAGE_MAP()
  	afx_msg void OnPaint();

public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnResultChange();

protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);


   // child Windows
   enum 
      {
      HPW_PLOT = 8452,
      HPW_YEARSLIDER,
      HPW_YEAREDIT,
      HPW_RESULTCOMBO,
      };

};