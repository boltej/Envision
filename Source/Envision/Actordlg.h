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
#if !defined(AFX_ACTORDLG_H__1E4F2D67_0493_4AFF_9CD4_FE706C10B8DC__INCLUDED_)
#define AFX_ACTORDLG_H__1E4F2D67_0493_4AFF_9CD4_FE706C10B8DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActorDlg.h : header file
//

#include "resource.h"
#include <Actor.h>

/////////////////////////////////////////////////////////////////////////////
// ActorDlg dialog

class ActorDlg : public CDialog
{
// Construction
public:
	ActorDlg(CWnd* pParent = NULL);   // standard constructor

   ActorGroupArray m_actorGroupArray;      // array of pointers to copies of the actor groups

// Dialog Data
	enum { IDD = IDD_ACTOR };

   int m_actorInitMethod;
   ActorGroup *m_pActorGroup;

	CComboBox	m_actorGroups;

   CString m_groupName;
   CString m_query;
   int m_id;
   int m_freq;
   //CStatic	m_color;

   CListBox	m_objectives;
	CSliderCtrl	m_wtSlider;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ActorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support   
	//}}AFX_VIRTUAL

   afx_msg void OnHScroll(UINT nSBCode,UINT nPos,CScrollBar* pScrollBar );

// Implementation
protected:
   bool m_isSetDirty;
   bool m_isDirty;
   int  m_objective;

   void MakeDirty();
   void LoadActorGroup();
   void SetObjectiveWeight();
   void SetApplyButton();
   void StoreChanges();
   void SaveActorsToDoc();

   void EnableControls();

	// Generated message map functions
	//{{AFX_MSG(ActorDlg)
	virtual BOOL OnInitDialog();
   afx_msg void OnAim();
	afx_msg void OnEditupdateActors();
	afx_msg void OnSelchangeObjectives();
	afx_msg void OnSelchangeActors();
	afx_msg void OnSelchangeLulcclass();
	afx_msg void OnSelchangeLulclevel();
	afx_msg void OnDelete();
	afx_msg void OnCopy();
	afx_msg void OnApply();
	afx_msg void OnPrev();
	afx_msg void OnNext();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnNMReleasedcaptureWtslider(NMHDR *pNMHDR, LRESULT *pResult);
protected:
   virtual void OnOK();
public:
   afx_msg void OnNMCustomdrawWtslider(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnEnChangeName();
   afx_msg void OnEnChangeQuery();
   afx_msg void OnBnClickedDefinequery();
   afx_msg void OnAdd();
   afx_msg void OnEnChangeId();
   afx_msg void OnEnChangeFreq();
   afx_msg void OnBnClickedExport();
   };

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTORDLG_H__1E4F2D67_0493_4AFF_9CD4_FE706C10B8DC__INCLUDED_)
