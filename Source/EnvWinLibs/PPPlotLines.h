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

#include "afxwin.h"

class GraphWnd;


// PPPlotLines dialog

class WINLIBSAPI PPPlotLines : public CPropertyPage
{
	DECLARE_DYNAMIC(PPPlotLines)

public:
	PPPlotLines( GraphWnd *pGraph );
	virtual ~PPPlotLines();

   GraphWnd *m_pGraph;
   int       m_lineNo;


// Dialog Data
	enum { IDD = IDD_LIB_PLOTLINES };

protected:
   void    LoadLineInfo( void );

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
   bool m_isDirty;

   void EnableControls( void );

public:
   afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
   virtual BOOL OnInitDialog();
   afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);

   CListBox m_lines;

   afx_msg void OnLbnSelchangeLine();
   afx_msg void OnEnChangeLinelabel();
   afx_msg void OnCbnSelchangeLinecolor();
   afx_msg void OnCbnSelchangeLinestyle();
   afx_msg void OnCbnSelchangeLinewidth();
   afx_msg void OnCbnSelchangeMarker();
   afx_msg void OnEnChangeMarkerincr();
   afx_msg void OnBnClickedShowline();
   afx_msg void OnBnClickedSmooth();
   afx_msg void OnBnClickedShowmarker();
   //CComboBox m_color;
   //CComboBox m_lineStyle;
   //CComboBox m_lineWidth;
   //CComboBox m_markerSymbol;
   afx_msg void OnEnChangeLegendheight();
   afx_msg void OnEnChangeTension();
   float m_tension;
   };
