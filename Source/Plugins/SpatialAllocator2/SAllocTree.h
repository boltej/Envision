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

#include "SAllocWnd.h"

#include <PtrArray.h>

// SAllocTreeCtrl


struct NODE_INFO
   {
   NODE_INFO( AllocationSet *_pSet, bool _isResult, HTREEITEM _hItem ) { pSet = _pSet; isResult=_isResult; pAlloc = NULL; hItem=_hItem; }
   NODE_INFO( Allocation *_pAlloc, HTREEITEM _hItem )  { pAlloc = _pAlloc; isResult=false; pSet = NULL; hItem = _hItem; }
   
   AllocationSet *pSet;
   Allocation *pAlloc;
   bool isResult;
   HTREEITEM hItem;

   private:
      NODE_INFO() { }  // illegal
   };



class SAllocTreeCtrl : public CTreeCtrl
{
friend class SAllocTree;

   DECLARE_DYNAMIC(SAllocTreeCtrl)

public:
   SAllocTreeCtrl();
   virtual ~SAllocTreeCtrl();

   HTREEITEM m_lastItem;

protected:
   DECLARE_MESSAGE_MAP()

   PtrArray< NODE_INFO > m_nodeInfoArray;  // one of these for each tree item
   
public:
   //virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
   //afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
   //afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
   };

// SAllocTree

class SAllocTree : public CWnd
{
   DECLARE_DYNCREATE(SAllocTree)

public:
   SAllocTree();
   virtual ~SAllocTree();

   void LoadTreeCtrl(void);
   void InsertAllocationSet(AllocationSet *pSet);
   void InsertAllocation(Allocation *pAlloc);
   void RemoveAllocationSet(AllocationSet *pSet);
   void RemoveAllocation(Allocation *pAlloc);

   NODE_INFO *FindNodeInfo( Allocation* );
   NODE_INFO *FindNodeInfo( AllocationSet* );

   void RefreshItemLabel( Allocation* );
   void RefreshItemLabel( AllocationSet* );

   SAllocTreeCtrl m_tree;
   SAllocWnd      *m_pSAllocWnd;

   //int  m_buttonHeight;
   //int  m_editHeight;
   //
   //CFont m_font;

protected:
   DECLARE_MESSAGE_MAP()

public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnSize(UINT nType, int cx, int cy);
};

