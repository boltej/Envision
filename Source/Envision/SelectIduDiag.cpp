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
// SelectIduDiag.cpp : implementation file
//


#include "stdafx.h"
#include "stdio.h"
#include "stdlib.h"

#include "Envision.h"
#include "SelectIduDiag.h"

#include "Envdoc.h"
#include <EnvPolicy.h>
#include <EnvModel.h>
#include <deltaArray.h>
#include <DataManager.h>

#include "resource.h"

#include "DeltaViewer.h"

extern MapLayer      *gpCellLayer;
extern CEnvDoc       *gpDoc;
extern EnvModel      *gpModel;

// SelectIduDiag dialog

IMPLEMENT_DYNAMIC(SelectIduDiag, CDialog)

SelectIduDiag::SelectIduDiag(CWnd* pParent /*=NULL*/)
	: CDialog(SelectIduDiag::IDD, pParent)
{
}

BOOL SelectIduDiag::OnInitDialog()
{
	CDialog::OnInitDialog();

	DeltaViewer *phDeltaViewer = (DeltaViewer*)this->GetParent();
	m_listView.SetItemCount(int( phDeltaViewer->m_selectedIDUs.GetCount()));
	m_listView.InsertColumn(0, "IDU", 0, 70);

	for(int i = 0; i < phDeltaViewer->m_selectedIDUs.GetCount(); i++)
	{
		int idu = phDeltaViewer->m_selectedIDUs.GetAt(i);
		char t[9];
		_snprintf_s(t, 9, _TRUNCATE, "%d", idu);
		m_listView.InsertItem(i, t);
	}

	m_selectedIDUsTemporary.Append(phDeltaViewer->m_selectedIDUs);

	return TRUE;
}

BOOL SelectIduDiag::UpdateData( BOOL bSaveAndValidate )
{
	CDialog::UpdateData(bSaveAndValidate);

	if( bSaveAndValidate )
	{
		DeltaViewer *phDeltaViewer = (DeltaViewer*)this->GetParent();
		phDeltaViewer->m_selectedIDUs.RemoveAll();
		phDeltaViewer->m_selectedIDUs.Append(m_selectedIDUsTemporary);
	}

	return TRUE;
}

SelectIduDiag::~SelectIduDiag()
{
}

void SelectIduDiag::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IDULIST, m_listView);
	DDX_Text(pDX, IDC_IDUTEXT, m_strIduText);
}

BEGIN_MESSAGE_MAP(SelectIduDiag, CDialog)
	ON_BN_CLICKED(IDC_REMOVEIDU, &SelectIduDiag::OnBnClickedRemoveidu)
	ON_BN_CLICKED(IDC_ADDIDUTEXT, &SelectIduDiag::OnBnClickedAddidutext)
	ON_BN_CLICKED(IDOK, &SelectIduDiag::OnBnClickedOk)
	ON_BN_CLICKED(IDC_ADDCURRENTIDU, &SelectIduDiag::OnBnClickedAddcurrentidu)
END_MESSAGE_MAP()


// SelectIduDiag message handlers

void SelectIduDiag::OnBnClickedRemoveidu()
{
	CArray <int, int> to_remove;
	
	POSITION pos = m_listView.GetFirstSelectedItemPosition();
	while (pos)
	{
		int nItem = m_listView.GetNextSelectedItem(pos);
		to_remove.Add(nItem);
	}

	// go backwards to avoid reindexing problems
	for(INT_PTR i = to_remove.GetCount() - 1; i >= 0 ; i--)
	{
		m_listView.DeleteItem(to_remove[i]);
		m_selectedIDUsTemporary.RemoveAt(to_remove[i]);
	}
}

void SelectIduDiag::OnBnClickedAddidutext()
{
	CString t;
	GetDlgItemText(IDC_IDUTEXT, t);

	if(t == "")
	{
		MessageBox("No IDU entered!", NULL, MB_ICONERROR | MB_OK);
		return;
	}

	// convert to int
	char *end;
	long idu = strtol(t.GetString(), &end, 10) ;

	if(idu == 0 && t != "0")
	{
		MessageBox("Invalid input. IDUs must be numeric.", NULL, MB_ICONERROR | MB_OK);
		return;
	}

	// check to make sure IDU exists in the map
	if(idu > gpCellLayer->GetRecordCount(MapLayer::ALL))
	{
		MessageBox("The IDU entered does not exist in this map.", NULL, MB_ICONERROR | MB_OK);
		return;
	}
	else
	{
		m_listView.InsertItem(m_listView.GetItemCount(), t);
		m_selectedIDUsTemporary.Add(idu);

		SetDlgItemText(IDC_IDUTEXT, "");
	}
}

void SelectIduDiag::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	UpdateData(true); // This wasn't being called from OnOK, for some reason.
	OnOK();
}

void SelectIduDiag::OnBnClickedAddcurrentidu()
{
	DeltaViewer *phDeltaViewer = (DeltaViewer*)this->GetParent();
	if(phDeltaViewer->m_selectedDeltaIDU == -1)
	{
		MessageBox("No delta was selected. Please select a delta or enter the IDU manually.", NULL, MB_ICONERROR | MB_OK);
	}
	else
	{
		char t[9];
		_snprintf_s(t, 9, _TRUNCATE, "%d", phDeltaViewer->m_selectedDeltaIDU);

		m_listView.InsertItem(m_listView.GetItemCount(), t);
		m_selectedIDUsTemporary.Add(phDeltaViewer->m_selectedDeltaIDU);
	}
}
