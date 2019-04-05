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
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"

#include <DeltaArray.h>
#include <vector>

#include <EasySize.h>

#include "SelectIduDiag.h"


// DeltaViewer dialog

// magic #s for threadstate management

struct FilterThreadParams
{
	std::vector<DELTA*> *pDeltaBuffer;

	CListCtrl* pListCtrl;

	int run;

	bool showSelectedIDUs;
	bool showPolicies;
	bool showLulc;
	bool showEvaluators;
   bool showProcesses;
	bool showOther;

   CArray< bool, bool > showModelArray;   // for eval models
   CArray< bool, bool > showAPArray;      // for auto processes
   CArray< bool, bool > showFieldArray;   // fields

   ::CMutex* ctrlMutex;
	bool* ctrlStop;

	CArray <int, int> *selectedIDUs;
};


class DeltaViewer : public CDialog
{
	DECLARE_DYNAMIC(DeltaViewer)

   DECLARE_EASYSIZE

   friend BOOL SelectIduDiag::OnInitDialog();
   friend BOOL SelectIduDiag::UpdateData( BOOL bSaveAndValidate );
   friend void SelectIduDiag::OnBnClickedAddcurrentidu();

public:
	DeltaViewer(CWnd* pParent=NULL, int selectedDeltaArrayIndex=-1, bool selectedOnly=false );   // standard constructor
	virtual ~DeltaViewer();

// Dialog Data
	enum { IDD = IDD_DELTAVIEWEREX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void Load();
   LPCTSTR GetDeltaTypeStr( short type );


   int m_selectedDeltaArray;
   
   int m_selectedDeltaIDU;

   bool m_selectedOnly;   // show only selected IDU's at startup

   CArray <int, int> m_selectedIDUs;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnLbnSelchangeRuns();
   afx_msg void OnNMClickTree(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnNMCustomdraw( NMHDR* pNMHDR, LRESULT* pResult );

protected:
   CListCtrl m_listView;
   CComboBox m_runs;
   //CString m_iduCountStatic;

   /// New stuff
   CTreeCtrl m_tree;
   CCheckListBox m_fields;

   HTREEITEM m_hItemPolicies;
   HTREEITEM m_hItemLulc;
   HTREEITEM m_hItemEvaluators;
   HTREEITEM m_hItemAutoProcs;
   HTREEITEM m_hItemOther;

   int m_run;

   int m_lastRowYear;
   int m_highlighted;

   ///

public:
   //afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
   afx_msg void OnBnClicked();
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);

   // members for multithreading
protected:
	// THIS MEMBER IS GUARDED BY A LOCK FOR
	// MULTITHREADING. DO _NOT_ USE WITHOUT
	// LOCKING FIRST
	bool m_ctrlStop;
	::CMutex* m_ctrlMutex;

	CWinThread* pThread;
	
	std::vector<DELTA*> m_deltaBuffer;
	static UINT FilterByOpts( LPVOID pParam );

public:
	afx_msg void OnBnClickedSelectidubtn();
   afx_msg void OnBnClickedSummary();
   afx_msg void OnBnClickedUnselectallfields();
afx_msg void OnBnClickedSelectallfields();
   afx_msg void OnLbnSelchangeFields();
};
