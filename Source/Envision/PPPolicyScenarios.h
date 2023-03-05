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

// PPPolicyScenarios dialog

class PPPolicyScenarios : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPPolicyScenarios)

public:
	PPPolicyScenarios( PolEditor*, EnvPolicy *&pPolicy );
	virtual ~PPPolicyScenarios();

// Dialog Data
	enum { IDD = IDD_POLICY_SCENARIOS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
   EnvPolicy *&m_pPolicy;
   PolEditor *m_pParent;

public:
   void LoadPolicy();
   bool StoreChanges();

protected:
//   void MakeDirty();

	DECLARE_MESSAGE_MAP()
public:
   CCheckListBox m_scenarios;
   virtual BOOL OnInitDialog();

   afx_msg void OnCheckScenario();
   };
