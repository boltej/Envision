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
#include "afxcmn.h"
#include "resource.h"

#include "pppolicybasic.h"
#include "pppolicysiteAttr.h"
#include "PPPolicyConstraints.h"
#include "pppolicyoutcomes.h"
//#include "pppolicyscores.h"
#include "pppolicyobjscores.h"
#include "PPPolicyScenarios.h"

#include <tabctrlssl.h>
#include "afxwin.h"


// m_isDirty flags
const int DIRTY_PARENT   = 1;
const int DIRTY_BASIC    = 2;
const int DIRTY_SITEATTR = 4;
const int DIRTY_GLOBAL_CONSTRAINTS = 8;
const int DIRTY_POLICY_CONSTRAINTS = 16;
const int DIRTY_OUTCOMES = 32;
const int DIRTY_SCORES   = 64;
const int DIRTY_SCENARIOS = 128;


#define  IDT_TIMER  WM_USER+200



// PolEditor dialog

class PolEditor : public CDialog
{
	DECLARE_DYNAMIC(PolEditor)

public:
	PolEditor(CWnd* pParent = NULL);   // standard constructor
	virtual ~PolEditor();

// Dialog Data
	enum { IDD = IDD_POLICYEDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CTabCtrlSSL m_tabCtrl;
   CComboBox m_policies;

   PPPolicyBasic m_basic;
   PPPolicySiteAttr m_siteAttr;
   PPPolicyConstraints m_constraints;
   PPPolicyOutcomes m_outcomes;
   PPPolicyObjScores m_scores;
   PPPolicyScenarios m_scenarios;

   Policy *FindPolicy( LPCTSTR name );
 
public:
   virtual BOOL OnInitDialog();

   int  ReloadPolicies( bool promptForNewPolicy );
   void MakeLocalCopyOfPolicies();
   void LoadPolicy();
   bool StoreChanges();
   bool SavePolicies();

   void MakeClean( int flag ) { m_isDirty &= ( ~flag ); }
   void MakeDirty( int flag );
   bool IsDirty( int flag ) { return m_isDirty & flag ? true : false; }
   bool IsEditable();
   bool AddNew();
   void ShowSites();
   void ShowMore();
   void ShowLess();


public:
   PolicyArray m_policyArray;
   PtrArray< GlobalConstraint > m_constraintArray;   // local copies of global constraitns stored in policy manager
   
   Policy*  m_pPolicy;
   int      m_currentIndex;
   bool     m_isEditable;

   bool m_showingMore;  // true if showing more, false is showing less
   int  m_fullHeight;
   int  m_shrunkHeight;
   bool m_firstLoad;
   int  m_isDirty;      // set whenever ANY change is made to the CURRENT policy
                        // determines if the user should be prompted to replace existing policies
   bool m_hasBeenDirty; // set whenever m_isDirty is set, but is not unset
   CString m_path;      // path of most recently saved policy file


protected:
   //virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSelchangePolicies();
	afx_msg void OnDelete();
	afx_msg void OnCopy();
	afx_msg void OnPrev();
	afx_msg void OnNext();
   afx_msg void OnAddnew();
   afx_msg void OnMakeDirty();
   afx_msg void OnBnClickedShowhide();
	DECLARE_MESSAGE_MAP()

   virtual void OnOK();
   virtual void OnCancel();
public:
   afx_msg void OnBnClickedEditfieldinfo();
   afx_msg void OnBnClickedSave();
   afx_msg void OnCbnEditchangePolicies();
   afx_msg void OnBnClickedImport();
   afx_msg void OnTimer(UINT_PTR TimerVal);  
   };
