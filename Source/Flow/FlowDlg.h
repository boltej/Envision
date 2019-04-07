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

#include <TreePropertySheet\TreePropSheetEx.h>
using namespace TreePropSheet;

#include "PPBasics.h"
#include "PPOutputs.h"

// FlowDlg

class FlowDlg : public CTreePropSheetEx
{
	DECLARE_DYNAMIC(FlowDlg)

public:
	FlowDlg(FlowModel *pFlow, LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~FlowDlg();

   PPBasics  m_basicPage;                
   
   // PPInputsPage         m_inputsPage;       // climate, tables
   // PPMethodsPage        m_methodsPage;      // global methods, flux handlers
   // PPReservoirPage      m_reservoirPage;    // reservoir, control points? (may need a separte page for control pts?)
   // PPParamEstPage       m_paramEstPage;     // parameter Estimation
   // PPControlPointsPage  m_controlPtsPage
   // PPExtraSVsPage       m_extraSVPage;
   PPOutputs m_outputsPage;   // add videoCapture?

protected:
	DECLARE_MESSAGE_MAP()
public:
   //virtual BOOL OnInitDialog();
   };


