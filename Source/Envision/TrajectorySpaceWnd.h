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

#include <plot3d\scatterPlot3dWnd.h>
#include <deltaArray.h>


// TrajectorySpaceWnd

class TrajectorySpaceWnd : public CWnd
{
DECLARE_DYNAMIC(TrajectorySpaceWnd)

public:
	TrajectorySpaceWnd( int multiRun );
	virtual ~TrajectorySpaceWnd();

protected:
   int m_multiRun;      // multirun attached to this plot
   int m_run;           // current run in the multirun that is highlighted
   int m_year;          // current year being "sliced"
   int m_totalYears;    

   ScatterPlot3DWnd m_plot;

   CSliderCtrl m_yearSlider;
   CComboBox   m_luClassCombo;
   CComboBox   m_evalModelCombo;
   CComboBox   m_runCombo;
   CEdit       m_yearEdit;

   DeltaArray  *m_pDeltaArray;

public:
   static bool Replay( CWnd *pWnd, int flag );
   bool _Replay( int flag );

protected:
   void RefreshData();

   // child Windows
   enum 
   {
      TSW_PLOT = 7472,
      TSW_YEARSLIDER,
      TSW_YEAREDIT,
      TSW_LUCLASSCOMBO,
      TSW_EVALMODELCOMBO,
      TSW_RUNCOMBO
   };

protected:
	DECLARE_MESSAGE_MAP()
public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnComboChange();
   afx_msg void OnRunComboChange();
   //afx_msg void OnEvaluatorChange();
   //afx_msg void OnRunChange();

protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);




};


