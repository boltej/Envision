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
#define NOMINMAX
#include "resource.h"
#include "afxwin.h"
#include "afxcmn.h"
#undef NOMINMAX

#include "EvaluatorLearning.h"

#include <map>
#include <set>
using std::set;
using std::map;

class EvaluatorLearnDlg : public CDialog
{
	DECLARE_DYNAMIC(EvaluatorLearnDlg)

public:
	EvaluatorLearnDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~EvaluatorLearnDlg();

// Dialog Data
	enum { IDD = IDD_EVAL_MODEL_LEARN_DLG };

   map< int, Policy * > m_policySet;  // <PolID, Policy*> to be consumed by users of this
   set<int>          m_idxModelInfo; // indices in EnvModel::m_modelInfoArray
   set<Scenario *>   m_scenarioSet;


protected:
   void EvaluatorLearnDlg::UpdatePolicyListControl();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   virtual BOOL OnInitDialog() ;
   virtual void OnOK(  );



	DECLARE_MESSAGE_MAP()
public:
   //afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   CListBox m_scenarioListBox;
   CListCtrl m_policyListCtrl;
   afx_msg void OnLbnSelchangeLbIddEvaluatorLearnDlg();
   CListBox m_modelListBox;
};
