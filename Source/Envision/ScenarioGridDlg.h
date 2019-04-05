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


#include <ugctrl.h>
#include <ugctelps.h>

#define CELLTYPE_IS_EDITABLE 120


class ScenarioGridDlg;
class Policy;

class ScenarioGrid : public CUGCtrl
{
public:
   ScenarioGrid(void);
   ~ScenarioGrid(void);

   bool m_isEditable;
   
   ScenarioGridDlg *m_pParent;
   //CUGEllipsisType m_ellipsisCtrl;
   //int m_ellipsisIndex;

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
 	virtual void OnTH_LClicked(int col,long row,int updn,RECT *rect,POINT *point,BOOL processed=0);

   //editing
	virtual int OnEditStart(int col, long row,CWnd **edit);
	virtual int OnEditVerify(int col,long row,CWnd *edit,UINT *vcKey);
	virtual int OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag);

	// notifications
	virtual void OnMenuCommand(int col,long row,int section,int item);
   virtual int OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param);



	//hints
	virtual int OnHint(int col,long row,int section,CString *string);

   int AddPolicy( Policy* );
   virtual void OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed);
};


// ScenarioGridDlg dialog

class ScenarioGridDlg : public CDialog
{
	DECLARE_DYNAMIC(ScenarioGridDlg)

public:
	ScenarioGridDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~ScenarioGridDlg();

   ScenarioGrid m_scenarioGrid;

   bool m_isDirty;

// Dialog Data
	enum { IDD = IDD_SCENARIOGRID };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
 	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
   };
