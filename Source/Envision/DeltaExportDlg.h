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


// DeltaExportDlg dialog

class DeltaExportDlg : public CDialog
{
	DECLARE_DYNAMIC(DeltaExportDlg)

public:
	DeltaExportDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~DeltaExportDlg();

// Dialog Data
	enum { IDD = IDD_DELTAEXPORT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   CStringArray m_startValuesArray;    // stores strings associated with each field
   CStringArray m_endValuesArray;      // stores strings associated with each field

   CMap<int,int,int,int> m_colToListIndexMap;

   int m_lastField;                    // this is the currently selected item, NOT field index (since fields are sorted alphabetically)
   
public:
   CListBox m_runs;
   CCheckListBox m_fields;
   afx_msg void OnBnClickedAll();
   afx_msg void OnBnClickedNone();
   BOOL m_selectedOnly;
   BOOL m_includeDuplicates;
   BOOL m_selectedDates;
   afx_msg void OnBnClickedSelecteddates();
   int m_startYear;
   int m_endYear;

   virtual BOOL OnInitDialog();
   virtual void OnOK();

   //void SetDefaultPath();

   afx_msg void OnLbnSelchangeRuns();
   BOOL m_useStartValue;
   CString m_startValues;
   BOOL m_useEndValue;
   CString m_endValues;
   afx_msg void OnBnClickedUsestartvalue();
   afx_msg void OnBnClickedUseendvalue();
   afx_msg void OnLbnSelchangeFields();
   CComboBox m_iduIDCombo;
   CString m_iduField;
   };
