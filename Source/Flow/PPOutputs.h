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

#include "Flow.h"
#include <EnvWinLibs.h>


#include <EditTreeCtrl\EditTreeCtrl.h>



class OutputTreeCtrl : public CEditTreeCtrl
{
protected:
   //afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
   // If derived from CEditTreeCtrl:
   virtual bool CanDragItem( TVITEM & item );
   virtual bool CanDropItem( HTREEITEM hDrag, HTREEITEM hDrop, EDropHint hint);

   virtual void DragMoveItem(HTREEITEM hDrag, HTREEITEM hDrop, EDropHint hint, bool bCopy );
   virtual void DisplayContextMenu( CPoint & point);
};




// PPOutputs dialog

class PPOutputs : public CPropertyPage
{
	DECLARE_DYNAMIC(PPOutputs)

public:
	PPOutputs( FlowModel *pFlow );
	virtual ~PPOutputs();

// Dialog Data
	enum { IDD = IDD_OUTPUTS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   FlowModel   *m_pFlow;
   ModelOutput *m_pOutput;    // current output if any
   ModelOutputGroup *m_pGroup;

   PtrArray< ModelOutputGroup > m_modelOutputGroupArray;  // a copy of the one in Flow

public:
   OutputTreeCtrl m_outputTreeCtrl;

   CString m_name;
   BOOL m_inUse;
   CEdit m_varNames;
   CString m_value;
   CString m_query;
   virtual BOOL OnInitDialog();
  
   
   ///////////////////
 
   void LoadOutput();        // fills individual output
   void LoadOutputs();       // fills tree

   void EnableControls();

   bool m_initialized;
   bool m_isOutputDirty;     // the current field info has changed
   bool m_isArrayDirty;

public:

   BOOL Initialize();

   void MakeDirty() { m_isOutputDirty = m_isArrayDirty = true; }

protected:
   virtual void OnOK();

public:
   afx_msg void OnBnClickedSave();
   afx_msg void OnBnClickedLoad();
   afx_msg void OnBnClickedExport();
   afx_msg void OnBnClickedMoveup();
   afx_msg void OnBnClickedMovedown();
   afx_msg void OnBnClickedAddOutput();
   afx_msg void OnBnClickedRemoveOutput();

   afx_msg void OnBnClickedOutputInuse();
   afx_msg void OnBnClickedOutputAwm();
   afx_msg void OnBnClickedOutputSum();
   afx_msg void OnBnClickedOutputPctarea();

   afx_msg void OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnTvnKeydownTree(NMHDR *pNMHDR, LRESULT *pResult);
};

   
