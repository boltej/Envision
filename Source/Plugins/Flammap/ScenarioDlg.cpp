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
// ScenarioDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FlammapAP.h"
#include "ScenarioDlg.h"
#include "afxdialogex.h"


// CScenarioDlg dialog

IMPLEMENT_DYNAMIC(CScenarioDlg, CDialog)

CScenarioDlg::CScenarioDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScenarioDlg::IDD, pParent)
	, m_id(0)
	, m_name(_T(""))
{
	m_pScenarioArray = NULL;
	selectedLoc = -1;
}

CScenarioDlg::~CScenarioDlg()
{
}

void CScenarioDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SCENARIO_ID, m_id);
	DDX_Text(pDX, IDC_EDIT_SCENARIO_NAME, m_name);
}


BEGIN_MESSAGE_MAP(CScenarioDlg, CDialog)
END_MESSAGE_MAP()


// CScenarioDlg message handlers


void CScenarioDlg::OnOK()
{
	UpdateData();
	for (int s = 0; s < m_pScenarioArray->GetSize(); s++)
	{
		FMScenario *scn = m_pScenarioArray->GetAt(s);
		if (scn->m_id == m_id && s != selectedLoc)
		{
			CString msg;
			msg.Format("Error: A scenario with Scenrio ID of %d already exists", m_id);
			AfxMessageBox(msg, MB_OK);
			return;
		}
	}

	CDialog::OnOK();
}
