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

#include "EnvWinLib_resource.h"

// PPPlotAxes dialog

class GraphWnd;


class WINLIBSAPI PPPlotAxes : public CPropertyPage
{
	DECLARE_DYNAMIC(PPPlotAxes)

public:
	PPPlotAxes( GraphWnd *);
	virtual ~PPPlotAxes();

   GraphWnd *m_pGraph;
   int       m_axis;
   CEdit     m_scaleMin;
   CEdit     m_scaleMax;
   CEdit     m_ticMinor;
   CEdit     m_ticMajor;
   CEdit     m_decimals;
   CButton m_autoScale;
   CButton m_autoIncr;
   CComboBox  m_axes;

// Dialog Data
	enum { IDD = IDD_LIB_PLOTAXES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
   virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
   virtual BOOL OnInitDialog();

   bool  Apply( bool redraw=true );
   afx_msg void OnEnChangeScalemin();
   afx_msg void OnEnChangeScalemax();
   afx_msg void OnEnChangeTicmajor();
   afx_msg void OnEnChangeTicminor();
   afx_msg void OnEnChangeTicdecimals();
   };
