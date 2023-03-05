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
#include "afxwin.h"

class EnvPolicy;


// OutcomeEditor dialog

class OutcomeEditor : public CDialog
{
	DECLARE_DYNAMIC(OutcomeEditor)

public:
	OutcomeEditor(EnvPolicy *pPolicy, CWnd* pParent = NULL);   // standard constructor
	virtual ~OutcomeEditor();

// Dialog Data
	enum { IDD = IDD_OUTCOMEEDITOR };

   EnvPolicy   *m_pPolicy;

   bool      m_isDirty;

   CString   m_outcome;
   CEdit     m_editOutcome;
   float     m_probability;
   BOOL      m_expire;
   CString   m_attr;
   BOOL      m_persist;
   BOOL      m_delay;
   int       m_expirePeriod;
   int       m_persistPeriod;
   int       m_delayPeriod;
   CListBox  m_results;

protected:
   void LoadResults();
   void LoadResult();
   void RefreshOutcome();
   void RefreshResult();
   void EnableControls();

   void MakeDirty() { m_isDirty = true; }

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedDelete();
   afx_msg void OnEnChangeOutcome();
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedOk();
   afx_msg void OnLbnSelchangeResults();
   afx_msg void OnEnChangeProbability();

   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedDelay();
   afx_msg void OnBnClickedExpire();
   afx_msg void OnBnClickedPersist();
   afx_msg void OnEnChangeExpireperiod();
   afx_msg void OnEnChangeAttr();
   afx_msg void OnEnChangePersistperiod();
   afx_msg void OnEnChangeDelayperiod();
   afx_msg void OnBnClickedConstraints();
   };
