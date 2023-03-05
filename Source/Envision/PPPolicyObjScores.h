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
#include "afxcmn.h"

#include <EnvPolicy.h>

#include "UGCtrl.h"
#include "ugctelps.h"
#include <TabPageSSL.h>

#define CELLTYPE_IS_EDITABLE 120

class PPPolicyObjScores;


class ScoreGrid : public CUGCtrl
{
public:
   ScoreGrid(void);
   ~ScoreGrid(void);

   bool m_isEditable;
   
   CUGEllipsisType m_ellipsisCtrl;
   int m_ellipsisIndex;

   PPPolicyObjScores *m_pParent;

private:
	DECLARE_MESSAGE_MAP()

public:
	//***** Over-ridable Notify Functions *****
	virtual void OnSetup();
	virtual void OnColChange(int oldcol,int newcol);
	virtual void OnRowChange(long oldrow,long newrow);
	virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);

	//mouse and key strokes
	virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
	virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);

   //editing
	virtual int OnEditStart(int col, long row,CWnd **edit);
	virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
	virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);

	// notifications
	virtual void OnMenuCommand(int col,long row,int section,int item);
   virtual int OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param);

	//hints
	virtual int OnHint(int col,long row,int section,CString *string);

   int AppendRow();
   virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);

};


class ModifierGrid : public CUGCtrl
{
public:
   ModifierGrid(void);
   ~ModifierGrid(void);

   bool m_isEditable;

   CUGEllipsisType m_ellipsisCtrl;
   int m_ellipsisIndex;

   PPPolicyObjScores *m_pParent;

private:
	DECLARE_MESSAGE_MAP()

public:
	//***** Over-ridable Notify Functions *****
	virtual void OnSetup();
	virtual void OnColChange(int oldcol,int newcol);
	virtual void OnRowChange(long oldrow,long newrow);
	virtual void OnCellChange(int oldcol,int newcol,long oldrow,long newrow);

	//mouse and key strokes
	virtual void OnLClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);
	virtual void OnRClicked(int col,long row,int updn,RECT *rect,POINT *point,int processed);

	//editing
	virtual int OnEditStart(int col, long row,CWnd **edit);
	virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
	virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);

	//notifications
	virtual void OnMenuCommand(int col,long row,int section,int item);
   virtual int OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param);

	//hints
	virtual int OnHint(int col,long row,int section,CString *string);

   int AppendRow();
   virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);

};



class PolEditor;

// PPPolicyObjScores dialog

class PPPolicyObjScores : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPPolicyObjScores)

public:
	PPPolicyObjScores( PolEditor*, EnvPolicy *&policy );
	virtual ~PPPolicyObjScores();

   // Dialog Data
	enum { IDD = IDD_POLICYOBJSCORES };
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
   EnvPolicy   *&m_pPolicy;     // ref to pointer to current policy
   PolEditor *m_pParent;

   CComboBox m_policies;
   ScoreGrid m_scoreGrid;
   ModifierGrid m_modifierGrid;

public:
   void LoadPolicy();
   void LoadObjective( int i );
   void ClearObjectives();
   bool StoreChanges();
   void MakeDirty();

protected:
   bool StoreObjChanges();

protected:
	virtual BOOL OnInitDialog();
   //afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnDestroy();
   //afx_msg void OnBnClickedComputescores();
   DECLARE_MESSAGE_MAP()
public:
   //CComboBox m_objectives;
   //afx_msg void OnCbnSelchangeObjectiives();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedRemove();
   afx_msg void OnBnClickedModadd();
   afx_msg void OnBnClickedModremove();
   };
