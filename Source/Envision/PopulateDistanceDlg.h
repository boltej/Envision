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
#include "afxwin.h"
#include "afxcmn.h"

// PopulateDistanceDlg dialog

class PopulateDistanceDlg : public CDialog
{
	DECLARE_DYNAMIC(PopulateDistanceDlg)

public:
	PopulateDistanceDlg( Map *pMap, CWnd* pParent = NULL);   // standard constructor
	virtual ~PopulateDistanceDlg();

// Dialog Data
	enum { IDD = IDD_DISTANCEDLG };

private:
   Map *m_pMap;

   QueryEngine *m_pSourceQueryEngine;
   QueryEngine *m_pTargetQueryEngine;   // IDU
   
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   CComboBox m_targetLayerCombo;
   CComboBox m_sourceLayerCombo;
   CComboBox m_targetFieldCombo;
   CComboBox m_indexFieldCombo;
   CComboBox m_valueFieldCombo;
   CComboBox m_sourceValueFieldCombo;
   virtual BOOL OnInitDialog();
   afx_msg void OnCbnSelchangePddTargetlayer();
   afx_msg void OnCbnSelchangePddSourcelayer();
   afx_msg void OnCalculate();
   afx_msg void OnExit();
   CEdit m_qTarget;
   CEdit m_qSource;
   afx_msg void OnBnClickedPopulatenearestindex();
   afx_msg void OnBnClickedPopulatenearestvalue();
   float m_thresholdDistance;
   afx_msg void OnBnClickedPopulatecount();
   CEdit m_editDistance;
   CProgressCtrl m_progress;
   CStatic m_status;
   afx_msg void OnBnClickedBrowse();
   };
