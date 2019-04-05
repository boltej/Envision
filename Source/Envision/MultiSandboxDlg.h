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
#include "sandbox.h"


// MultiSandboxDlg dialog

class MultiSandboxDlg : public CDialog
{
	DECLARE_DYNAMIC(MultiSandboxDlg)

public:
	MultiSandboxDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~MultiSandboxDlg();

// Dialog Data
	enum { IDD = IDD_MULTISANDBOXDLG };


protected:
   Sandbox *m_pSandbox;
   int      m_yearsToRun;
   int      m_percentCoverage;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
   // This dialog must be modal.
   virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL){ return CDialog::Create(lpszTemplateName, pParentWnd); }

public:
   virtual BOOL OnInitDialog();

protected:
   virtual void OnOK();
   virtual void OnCancel();
public:
   CCheckListBox m_metagoals;
   CCheckListBox m_policies;
};
