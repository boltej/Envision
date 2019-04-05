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

#include <Plot3D\ScatterPlot3DWnd.h>



// GoalSpaceWnd

class GoalSpaceWnd : public CWnd
{
public:
	GoalSpaceWnd( int run);
	virtual ~GoalSpaceWnd();

   int m_run;
   int m_year;
   int m_totalYears;

   ScatterPlot3DWnd m_plot;

   CSliderCtrl m_yearSlider;
   CComboBox   m_goalCombo0;
   CComboBox   m_goalCombo1;
   CComboBox   m_goalCombo2;
   CEdit       m_yearEdit;


public:
   static bool Replay( CWnd *pWnd, int flag );
   bool _Replay( int flag );

protected:
   void RefreshData();

   // child Windows
   enum 
   {
      GSW_PLOT = 7452,
      GSW_YEARSLIDER,
      GSW_YEAREDIT,
      GSW_GOALCOMBO1,
      GSW_GOALCOMBO2,
      GSW_GOALCOMBO3     
   };

protected:
	DECLARE_MESSAGE_MAP()
public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnGoalChange();

protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};


