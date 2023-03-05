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
#include "resource.h"

#include <EnvPolicy.h>

#include <TabPageSSL.h>

class PolEditor;


// PPPolicyOutcomes dialog

class PPPolicyOutcomes : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPPolicyOutcomes)

public:
	PPPolicyOutcomes( PolEditor*, EnvPolicy *&pPolicy );
	virtual ~PPPolicyOutcomes();

// Dialog Data
	enum { IDD = IDD_POLICYOUTCOME };
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   
protected:
   EnvPolicy *&m_pPolicy;
   PolEditor *m_pParent;

   CListBox m_outcomeList;
   CEdit    m_outcomes;

public:
   void LoadPolicy();
   void RefreshOutcomes();

   bool StoreChanges();

protected:
//   void MakeDirty();

protected:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedEdit();
   afx_msg void OnLbnSelchangeOutcomelist();
   afx_msg void OnEnChangeOutcomes();
	DECLARE_MESSAGE_MAP()

public:
   afx_msg void OnBnClickedDelete();
   afx_msg void OnBnClickedUpdate();
   };

