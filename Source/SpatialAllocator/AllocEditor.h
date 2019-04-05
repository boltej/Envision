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

#include "Resource.h"
#include "SpatialAllocator.h"
#include "SAllocWnd.h"

#include <tabctrlssl.h>

#include "AllocBasicPP.h"
#include "AllocExpandPP.h"
#include "AllocConstraintPP.h"
#include "AllocPrefsPP.h"


#include <EasySize.h>

// AllocEditor dialog
class SAETabCtrl : public CTabCtrlSSL
{
public:
   virtual void OnActivatePage (int nIndex, int nPageID);
   //virtual void OnDeactivatePage (int nIndex, int nPageID);
};



class AllocEditor : public CDialog
{
   DECLARE_DYNAMIC(AllocEditor)

   DECLARE_EASYSIZE

public:
   AllocEditor( CWnd* pParent = NULL);   // standard constructor
   virtual ~AllocEditor();

   void LoadAllocation();
   void SetAllocation( Allocation *pAlloc )
      {
      // note:  should check whether anything neeeds to be saved first
      m_pAlloc = pAlloc; LoadAllocation();
      }
   void SetMapLayer(MapLayer *pLayer) { m_pMapLayer = pLayer; }
   void SetQueryEngine(QueryEngine *pQE) { m_pQueryEngine = pQE; }

   bool SaveFields();
   
   // Dialog Data
   enum { IDD = IDD_ALLOCDLG };

   Allocation* m_pAlloc;
   QueryEngine *m_pQueryEngine;

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   SAETabCtrl m_tabCtrl;

   AllocBasicPP      m_basic;
   AllocExpandPP     m_expand;
   AllocConstraintPP m_constraint;
   AllocPrefsPP      m_prefs;
   MapLayer         *m_pMapLayer;

   virtual BOOL OnInitDialog();
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnDelete();
   afx_msg void OnCopy();
   afx_msg void OnAddNew();
   DECLARE_MESSAGE_MAP()

   virtual void OnOK();
   virtual void OnCancel();
public:
  // afx_msg void OnBnClickedEditfieldinfo();
   afx_msg void OnBnClickedSaveXml();
   afx_msg void OnBnClickedImport();
   afx_msg void OnBnClickedBtngrp();
   
   CStatic m_allocHeader;
   };

