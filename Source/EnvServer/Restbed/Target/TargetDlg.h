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
#include "afxwin.h"

#include <Maplayer.h>
#include "target.h"

#include <UGCtrl.h>

#include <ugctelps.h>


#define CELLTYPE_IS_EDITABLE 120

class TargetDlg;

class ReportsGrid : public CUGCtrl
{
public:
   ReportsGrid(void);
   ~ReportsGrid(void);

   bool m_isEditable;
   
   CUGEllipsisType m_ellipsisCtrl;
   int m_ellipsisIndex;

   TargetDlg *m_pParent;

private:
	DECLARE_MESSAGE_MAP()

public:
	//***** Over-ridable Notify Functions *****
	virtual void OnSetup();
	//virtual void OnColChange(int oldcol,int newcol);
	//virtual void OnRowChange(long oldrow,long newrow);
	//virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);

	//mouse and key strokes
	//virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
	//virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);

   //editing
	virtual int OnEditStart(int col, long row,CWnd **edit);
	virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
	virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);

	// notifications
	virtual void OnMenuCommand(int col,long row,int section,int item);
   virtual int OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param);

	//hints
	virtual int OnHint(int col,long row,int section,CString *string);

   int AppendRow( LPCTSTR name, LPCTSTR query );
   virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);

};


class ConstantsGrid : public CUGCtrl
{
public:
   ConstantsGrid(void);
   ~ConstantsGrid(void);

   bool m_isEditable;

   TargetDlg *m_pParent;

private:
	DECLARE_MESSAGE_MAP()

public:
	//***** Over-ridable Notify Functions *****
	virtual void OnSetup();
	//virtual void OnColChange(int oldcol,int newcol);
	//virtual void OnRowChange(long oldrow,long newrow);
	//virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);

	//mouse and key strokes
	//virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
	//virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);

   //editing
	virtual int OnEditStart(int col, long row,CWnd **edit);
	virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
	virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);

	// notifications
	virtual void OnMenuCommand(int col,long row,int section,int item);
   virtual int OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param);

	//hints
	virtual int OnHint(int col,long row,int section,CString *string);

   int AppendRow( LPCTSTR name, LPCTSTR expr );
   virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);

};


class AllocationsGrid : public CUGCtrl
{
public:
   AllocationsGrid(void);
   ~AllocationsGrid(void);

   bool m_isEditable;
   
   CUGEllipsisType m_ellipsisCtrl;
   int m_ellipsisIndex;

   AllocationSet *m_pAllocSet;
   TargetDlg *m_pParent;

private:
	DECLARE_MESSAGE_MAP()

public:
	//***** Over-ridable Notify Functions *****
	virtual void OnSetup();
	//virtual void OnColChange(int oldcol,int newcol);
	//virtual void OnRowChange(long oldrow,long newrow);
	//virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);

	//mouse and key strokes
	//virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
	//virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);

   //editing
	virtual int OnEditStart(int col, long row,CWnd **edit);
	virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
	virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);

	// notifications
	virtual void OnMenuCommand(int col,long row,int section,int item);
   virtual int OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param);

	//hints
	virtual int OnHint(int col,long row,int section,CString *string);

   int AppendRow( LPCTSTR name, LPCTSTR query, LPCTSTR expr );
   virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);

};


class PreferencesGrid : public CUGCtrl
{
public:
   PreferencesGrid(void);
   ~PreferencesGrid(void);

   bool m_isEditable;
   
   CUGEllipsisType m_ellipsisCtrl;
   int m_ellipsisIndex;

   AllocationSet *m_pAllocSet;
   TargetDlg *m_pParent;

private:
	DECLARE_MESSAGE_MAP()

public:
	//***** Over-ridable Notify Functions *****
	virtual void OnSetup();
	//virtual void OnColChange(int oldcol,int newcol);
	//virtual void OnRowChange(long oldrow,long newrow);
	//virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);

	//mouse and key strokes
	//virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
	//virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);

   //editing
	virtual int OnEditStart(int col, long row,CWnd **edit);
	virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
	virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);

	// notifications
	virtual void OnMenuCommand(int col,long row,int section,int item);
   virtual int OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param);

	//hints
	virtual int OnHint(int col,long row,int section,CString *string);

   int AppendRow( LPCTSTR name, LPCTSTR query, LPCTSTR expr );
   virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);
};


// TargetDlg dialog

class TargetDlg : public CDialog
{
	DECLARE_DYNAMIC(TargetDlg)

public:
	TargetDlg(TargetProcess *pTargetProcess, MapLayer *pLayer, CWnd* pParent = NULL);   // standard constructor
	virtual ~TargetDlg();

// Dialog Data
	enum { IDD = IDD_TARGET };

   void MakeDirty() { m_isDirty = true; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   bool LoadTarget( void );
   bool LoadAllocationSet( int i );
   void ClearGrids( bool all );
   void SaveTarget( void );

   TargetProcess *m_pTargetProcess;
   Target        *m_pTarget;
   MapLayer      *m_pLayer;

	DECLARE_MESSAGE_MAP()

   ReportsGrid m_reportsGrid;
   ConstantsGrid m_constantsGrid;

   // the following are arrays, one per allocation set
   PtrArray< AllocationsGrid > m_allocationsGridArray;
   PtrArray< PreferencesGrid > m_preferencesGridArray;
   
   bool m_isDirty;
   CFont m_font;
public:
   CComboBox m_methods;
   CComboBox m_scenarios;
   CComboBox m_colDensity;
   CComboBox m_colCapacity;
   CComboBox m_colAvail;
   CComboBox m_colPctAvail;
   afx_msg void OnBnClickedAddreport();
   afx_msg void OnBnClickedDeletereport();
   afx_msg void OnBnClickedAddconstant();
   afx_msg void OnBnClickedDeleteconstant();
   afx_msg void OnBnClickedAddallocation();
   afx_msg void OnBnClickedDeleteallocation();
   afx_msg void OnBnClickedAddpreference();
   afx_msg void OnBnClickedDeletepreference();
   afx_msg void OnCbnSelchangeScenarios();
   virtual BOOL OnInitDialog();
   virtual void OnOK();
   float m_rate;
   afx_msg void OnBnClickedRemoveallocationset();
   afx_msg void OnBnClickedAddallocationset();
   int m_allocationSetID;
   afx_msg void OnEnChangeAllocationsetid();
   };
