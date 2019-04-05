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

// ResultsGraphWnd
#include <ScatterWnd.h>


class ResultsGraphWnd : public CWnd
{
public:
	ResultsGraphWnd( int run, bool isDifference );
   virtual ~ResultsGraphWnd();

   static bool Replay( CWnd *pWnd, int flag );
   bool _Replay( int flag );

   static bool SetYear( CWnd *pWnd, int oldYear, int newYear );
   bool _SetYear( int oldYear, int newYear );

public:
   GraphWnd *m_pGraphWnd;  // pointers to the two contained window
   int       m_graphType;  // 0=scatter, 1=bar

protected:
   int m_year;
   int m_startYear;
   int m_endYear;
   int m_run;

protected:
   void Shift( int fromYear, int toYear );

protected:
	DECLARE_MESSAGE_MAP()
public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
   afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
   afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	};






