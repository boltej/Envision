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

#include "EnvWinLibs.h"
#include "EnvWinLib_resource.h"

class GraphWnd;


// PPPlotSetup dialog

class WINLIBSAPI PPPlotSetup : public CPropertyPage
{
	DECLARE_DYNAMIC(PPPlotSetup)

public:
	PPPlotSetup( GraphWnd *pGraph );
	virtual ~PPPlotSetup();

   GraphWnd *m_pGraph;

// Dialog Data
	enum { IDD = IDD_LIB_PLOTSETUP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

   bool  Apply( bool redraw=true );
   afx_msg void OnBnClickedFonttitle();
   afx_msg void OnBnClickedFontxaxis();
   afx_msg void OnBnClickedFontyaxis();
   afx_msg void OnEnChangeCaption();
   afx_msg void OnEnChangeTitle();
   afx_msg void OnEnChangeXaxis();
   afx_msg void OnEnChangeYaxis();
   afx_msg void OnBnClickedShowhorz();
   afx_msg void OnBnClickedShowvert();
   afx_msg void OnBnClickedGrooved();
   afx_msg void OnBnClickedRaised();
   afx_msg void OnBnClickedSunken();
   };
