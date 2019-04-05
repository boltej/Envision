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

#include "resource.h"
#include <map.h>


// PopulateAdjacencyDlg dialog

class PopulateAdjacencyDlg : public CDialog
{
	DECLARE_DYNAMIC(PopulateAdjacencyDlg)

public:
	PopulateAdjacencyDlg(Map *pMap, CWnd* pParent = NULL);   // standard constructor
	virtual ~PopulateAdjacencyDlg();

// Dialog Data
	enum { IDD = IDD_ADJACENCY };

private:
   Map *m_pMap;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   CComboBox m_sourceLayerCombo;
   CComboBox m_targetLayerCombo;
   CComboBox m_targetFieldCombo;
   virtual BOOL OnInitDialog();
   afx_msg void OnCbnSelchangePapTargetlayer();
   afx_msg void OnCalculate();
   afx_msg void OnExit();
   CEdit m_query;
};
