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

#include "stdafx.h"

#include "afxwin.h"
#include "afxcmn.h"
#include "resource.h"
#include <Policy.h>

#include <TabPageSSL.h>

class PolEditor;

// PPPolicyBasic dialog

class PPPolicyBasic : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPPolicyBasic)

public:
	PPPolicyBasic( PolEditor*, Policy *&pPolicy  );
	virtual ~PPPolicyBasic();

// Dialog Data
	enum { IDD = IDD_POLICYBASIC };
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
   Policy    *&m_pPolicy;
   PolEditor  *m_pParent;

   CStatic     m_color;    // the control
   COLORREF    m_colorRef; // the selected color
   CString     m_narrative;
   int         m_persistenceMin;
   int         m_persistenceMax;
   BOOL        m_isScheduled;
   BOOL        m_mandatory;
   BOOL        m_exclusive;
   BOOL        m_isShared;
   BOOL        m_isEditable;
   BOOL        m_useStartDate;
   BOOL        m_useEndDate;
   int         m_startDate;
   int         m_endDate;

public:
   void LoadPolicy();
   bool StoreChanges();

protected:
   void EnableControls();
   void MakeDirty();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSetcolor();
   afx_msg void OnBnClickedIsScheduled();
   afx_msg void OnBnClickedMandatory();
   afx_msg void OnBnClickedPersistcheck();
   afx_msg void OnBnClickedUsestartdate();
   afx_msg void OnBnClickedUseenddate();
   afx_msg void OnBnClickedIsshared();
   afx_msg void OnBnClickedShowhide();
	DECLARE_MESSAGE_MAP()

};
