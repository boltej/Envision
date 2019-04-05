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

#include <FDataObj.h>

// LulcTransitionResultsWnd

class LulcTransitionResultsWnd : public CWnd
{
public:
   LulcTransitionResultsWnd( int run);
   virtual ~LulcTransitionResultsWnd();

   int m_run;
   int m_year;
   int m_totalYears;

   CWnd       *m_pWnd;
   FDataObj   *m_pData;

   CComboBox   m_lulcLevelCombo;
   CComboBox   m_timeIntervalCombo;
   CComboBox   m_plotListCombo;

   CSliderCtrl m_yearSlider;
   CEdit       m_yearEdit;

   bool m_changeTimeIntervalFlag;

public:
   static bool Replay( CWnd *pWnd, int flag );
   bool _Replay( int flag );

protected:
   void RefreshData();

   // child Windows
   enum 
   {
      LTPW_PLOT = 8452,
      LTPW_LULCLEVELCOMBO,
      LTPW_TIMEINTERVALCOMBO,
      LTPW_PLOTLISTCOMBO,
      LTPW_YEARSLIDER,
      LTPW_YEAREDIT,     
   };

protected:
   DECLARE_MESSAGE_MAP()
public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnLulcLevelChange();
   afx_msg void OnTimeIntervalChange();
   afx_msg void OnPlotListChange();

protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};


