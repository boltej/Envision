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

#include <PtrArray.h>



class ScenarioPage
{
public:
   CString m_name;       // caption for this window
   CWnd   *m_pWnd;       // container for controls on this page
   PtrArray< CWnd > m_controlArray;
};

// ScenarioWizard

class ScenarioWizard : public CWnd
{
	DECLARE_DYNAMIC(ScenarioWizard)

public:
	ScenarioWizard();
	virtual ~ScenarioWizard();

protected:
	DECLARE_MESSAGE_MAP()

   PtrArray< ScenarioPage > m_scenarioPageArray;
   int m_currentPage;

public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
   
   //afx_msg void OnEditCopy();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
   afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
};


