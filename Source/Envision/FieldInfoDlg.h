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

#include "resource.h"

#include <maplayer.h>
#include <grid\gridctrl.h>
#include <EditTreeCtrl\EditTreeCtrl.h>

#include "afxcmn.h"

class FieldInfoDlg;

class AttrGrid : public CGridCtrl
{
public:
   FieldInfoDlg *m_pParent;

   //virtual void  EndEditing() { m_pParent->MakeDirty(); }
   //afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point) { m_pParent->MakeDirty(); }
};


class ColorGridCell : public CGridCell
{
public:
   virtual void OnDblClick( CPoint PointCellRelative );
   //virtual void OnClick( CPoint PointCellRelative)  { m_pGrid->m_pParent->MakeDirty(); }
   //virtual void OnClickDown( CPoint PointCellRelative)  { m_pGrid->m_pParent->MakeDirty(); }
   //virtual void OnEndEdit() { m_pGrid->m_pParent->MakeDirty(); }

   DECLARE_DYNCREATE(ColorGridCell)
};

//MyGrid.SetCellType(row, column, RUNTIME_CLASS(CMyGridCell));


class FieldInfoTreeCtrl : public CEditTreeCtrl
{
protected:
   //afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
   // If derived from CEditTreeCtrl:
   virtual bool CanDragItem( TVITEM & item );
   virtual bool CanDropItem( HTREEITEM hDrag, HTREEITEM hDrop, EDropHint hint);

   virtual void DragMoveItem(HTREEITEM hDrag, HTREEITEM hDrop, EDropHint hint, bool bCopy );
   virtual void DisplayContextMenu( CPoint & point);
};



// FieldInfoDlg dialog

class FieldInfoDlg : public CDialog
{
friend class ColorGridCell;

   DECLARE_DYNAMIC(FieldInfoDlg)

public:
   FieldInfoDlg(CWnd* pParent = NULL);   // standard constructor
   virtual ~FieldInfoDlg();

// Dialog Data
   enum { IDD = IDD_FIELDINFO };

   bool LoadFieldInfo();

protected:
   FieldInfoArray m_fieldInfoArray;

   MAP_FIELD_INFO *m_pInfo;      // current pointers
   FIELD_ATTR     *m_pAttr;

   AttrGrid        m_grid;

   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void EnableControls();
   void AllocateAttrGrid();
   void LoadAttributes();
   void LoadFields();
   void LoadFieldTreeCtrl();
   void LoadSubmenuCombo();
   DECLARE_MESSAGE_MAP()

   MapLayer *m_pLayer;      // currently loaded layer
   int       m_layerIndex;  // currently loaded file in the file combo box

   COLORREF m_attrColor;

   bool m_initialized;
   bool m_isInfoDirty;     // the current field info has changed
   bool m_isArrayDirty;

public:
   CComboBox m_files;
   CCheckListBox m_fields;
   FieldInfoTreeCtrl m_fieldTreeCtrl;
   CString m_label;
   CString m_description;
   CString m_source;
   CComboBox m_colors;
   CComboBox m_submenuItems;
   CImageList m_imageList;

   int m_count;
   BOOL m_results;
   BOOL m_decadalMaps;
   BOOL m_siteAttr;
   BOOL m_outcomes;
   BOOL m_save;
   CString m_path;      // most recently saved path
   int m_yearInterval;
   float m_minVal;
   float m_maxVal;

   afx_msg void OnBnClickedSave();
   afx_msg void OnBnClickedLoad();
   afx_msg void OnLbnSelchangeFields();

   virtual BOOL OnInitDialog();
   BOOL Initialize();

   void MakeDirty() { m_isInfoDirty = m_isArrayDirty = true; }

protected:
   virtual void OnOK();

   afx_msg void OnEnChangeLabel();
   afx_msg void OnEnChangeDescription();
   afx_msg void OnEnChangeSource();
   //afx_msg void OnBnClickedLevel0();
   //afx_msg void OnBnClickedLevel1();
   afx_msg void OnCbnSelchangeColors();
   afx_msg void OnEnChangeCount();
   afx_msg void OnEnChangeMin();
   afx_msg void OnEnChangeMax();
   afx_msg void OnBnClickedSiteattr();
   afx_msg void OnBnClickedOutcomes();
   afx_msg void OnBnClickedResults();
   afx_msg void OnBnClickedExport();
   afx_msg void OnBnClickedDecadalmaps();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedAddall();
   afx_msg void OnBnClickedRemove();
   afx_msg void OnLbnCheckFields();
   afx_msg void OnGridEndEdit(NMHDR *pNotifyStruct, LRESULT* pResult);

public:
   afx_msg void OnCbnSelchangeFiles();
   afx_msg void OnBnClickedRemoveall();
   afx_msg void OnBnClickedMoveup();
   afx_msg void OnBnClickedMovedown();
   afx_msg void OnBnClickedUsecategory();
   afx_msg void OnBnClickedUsequantitybins();
   afx_msg void OnBnClickedUsecategorybins();
   afx_msg void OnCbnSelchangeSubmenu();
   afx_msg void OnBnClickedAddsubmenu();
   afx_msg void OnBnClickedAddfieldinfo();
   afx_msg void OnBnClickedRemovefieldinfo();
   afx_msg void OnTvnSelchangedFieldtree(NMHDR *pNMHDR, LRESULT *pResult);
   afx_msg void OnLbnDblclkFields();
   afx_msg void OnTvnKeydownFieldtree(NMHDR *pNMHDR, LRESULT *pResult);
};
