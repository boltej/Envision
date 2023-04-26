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
#include "Lulctree.h"
#include "afxwin.h"
//#include "EnvDoc.h"


#include <EasySize.h>
#include "afxcolorbutton.h"


// LulcEditor dialog

class LulcEditor : public CDialog
{
   DECLARE_EASYSIZE
   DECLARE_DYNAMIC(LulcEditor)

public:
	LulcEditor(CWnd* pParent = NULL);   // standard constructor
	virtual ~LulcEditor();

// Dialog Data
	enum { IDD = IDD_LULCEDITOR };

protected:
	LulcTree  *m_pLulcTree;
   CButton    m_expandButton;
   CTreeCtrl  m_treeCtrl;
   bool       m_expanded;
   CRect      m_windowRect;
   CEdit      m_label;
   CEdit      m_attrValue;

   bool       m_isDirty;

   void AddToCtrl( std::vector<LulcNode*> *pChildArray,  HTREEITEM hParent );
	HTREEITEM FindItem( HTREEITEM hItem, int level, int id );
	bool TestNode( LulcNode *pNode, int level, int id ){ return pNode->m_id == id && pNode->GetNodeLevel() == level;}
	void CollapseAll( HTREEITEM hItem );
   void ExpandAll( HTREEITEM hItem );

public:
   CString m_path;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnNMClickLulctreectrl(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnTvnSelchangedLulctreectrl(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnBnClickedExpand();
protected:
   virtual void OnCancel();
   virtual void OnOK();
public:
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedRemove();
   afx_msg void OnBnClickedExport();
   afx_msg void OnEnChangeLabel();
   afx_msg void OnEnChangeId();
   };
