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
#include "afxcmn.h"
#include "resource.h"
#include <Scenario.h>
// ScenarioEditor dialog



class ScenarioEditor : public CDialog
{
	DECLARE_DYNAMIC(ScenarioEditor)

public:
	ScenarioEditor(CWnd* pParent = NULL);   // standard constructor
	virtual ~ScenarioEditor();

// Dialog Data
	enum { IDD = IDD_SCENARIOEDITOR };

protected:
   int m_currentIndex;
   Scenario *m_pScenario;     // currently selected scenario
   SCENARIO_VAR *m_pInfo;     // currently selected VAR_INFO (or NULL)
   AppVar   *m_pAppVar;       // currently selected APP_VAR (or NULL)

   CArray< Scenario*, Scenario* > m_scenarioArray; // local copies of scenarios

   bool m_isDirty;
   bool m_isSaved;
   bool m_isVarDirty;

   void MakeDirty() { m_isDirty = true; m_isSaved = false; }
   void MakeVarDirty() { m_isDirty = true; m_isVarDirty = true; m_isSaved = false; }

   HTREEITEM m_hMetagoals;    // root of the metagoals
   HTREEITEM m_hModels;       // root of the eval model variables
   HTREEITEM m_hProcesses;    // root of the auto. processes variables
   HTREEITEM m_hModelUsage;       // root of the eval model variables
   HTREEITEM m_hProcessUsage;    // root of the auto. processes variables
   HTREEITEM m_hAppVars;       // root of the app variables
   void LoadScenario();
   void LoadVarTree();
   void LoadModelVar();
   void EnableControls();
   int  SaveScenarios();

   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   CString m_path;      // path to most recently saved scenario file
   CComboBox m_scenarios;
   CString m_name;
   CString m_originator;
   CString m_scnDescription;
//   int m_altruism;
//   int m_self;
   int m_policyPref;
   CTreeCtrl m_inputVars;
   BOOL m_useVar;
   CComboBox m_distribution;
   CString m_location;
   CString m_scale;  // float
   CStatic m_description;
   CCheckListBox m_policies;
//   CSliderCtrl m_altruismSlider;
//   CSliderCtrl m_selfSlider;
   CSliderCtrl m_policyPrefSlider;
   int m_evalFreq;

   virtual BOOL OnInitDialog();
   afx_msg void OnTvnSelchangedModelvars(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnEnChangeName();
   afx_msg void OnEnChangeOriginator();
   afx_msg void OnEnChangeScnDescription();
   afx_msg void OnCbnSelchangeDistribution();
   afx_msg void OnEnChangeLocation();
   afx_msg void OnEnChangeScale();
   afx_msg void OnLbnChkchangePolicies();
   afx_msg void OnCbnSelchangeScenarios();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedDelete();
   afx_msg void OnBnClickedSave();
   afx_msg void OnBnClickedOk();
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnEnChangeEvalfreq();
   afx_msg void OnBnClickedUsedfaultvalues();
   afx_msg void OnBnClickedUsespecifiedvalues();
   afx_msg void OnBnClickedGridview();
   afx_msg void OnTvnItemexpandedModelvars(NMHDR *pNMHDR, LRESULT *pResult);
   };
