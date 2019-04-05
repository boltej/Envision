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
#include "dataGrid.h"

// FindDataDlg dialog

class FindDataDlg : public CDialog
{
	DECLARE_DYNAMIC(FindDataDlg)

public:
	FindDataDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~FindDataDlg();

// Dialog Data
	enum { IDD = IDD_FINDDATA };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   void FindReplace( bool replace );

public:
   void SetGrid( DataGrid *pGrid );
   void AllowReplace( bool );

protected:
   DataGrid *m_pGrid;

   int  m_col;
   long m_row;

   bool  m_allowReplace;
   bool  m_showingMore;
   CRect m_moreRect;       // all these rects are client coords
   CRect m_findRect;
   CRect m_closeRect;
   int   m_moreLessSize;

   BOOL m_matchCase;
   BOOL m_matchEntire;
   BOOL m_findAll;
   BOOL m_searchThis;
   BOOL m_searchSelected;
   BOOL m_searchAll;
   BOOL m_searchUp;
   CComboBox m_fields;
   CString m_findString;
   CString m_replaceString;
   CString m_field;

   afx_msg void OnBnClickedOk();
   afx_msg void OnBnClickedReplace();
   afx_msg void OnBnClickedCancel();
   afx_msg void OnBnClickedMoreLess();
   virtual BOOL OnInitDialog();
   };
