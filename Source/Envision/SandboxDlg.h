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

#include <EnvPolicy.h>
#include "Sandbox.h"
#include "resource.h"
#include "afxwin.h"

// SandboxDlg dialog

class SandboxDlg : public CDialog
{
	DECLARE_DYNAMIC(SandboxDlg)

public:
	SandboxDlg( EnvPolicy *pPolicy, CWnd* pParent = NULL );   // standard constructor
	virtual ~SandboxDlg();

// Dialog Data
	enum { IDD = IDD_SANDBOXDLG };

protected:
   EnvPolicy  *m_pPolicy;      // policy whose efficacies are to be calculated
   Sandbox *m_pSandbox;
   int      m_yearsToRun;
   int      m_percentCoverage;
   bool     m_runCompleted;

public:
   bool    *m_useMetagoalArray;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
   // This dialog must be modal.
   virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL){ return CDialog::Create(lpszTemplateName, pParentWnd); }

public:
   virtual BOOL OnInitDialog();
   virtual BOOL DestroyWindow();

protected:
   virtual void OnOK();
   virtual void OnCancel();
public:
   CCheckListBox m_metagoals;
};
