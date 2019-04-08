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
#include <easysize.h>

// AllocSetEditor dialog

class AllocSetEditor : public CDialog
{
   DECLARE_DYNAMIC(AllocSetEditor)
   DECLARE_EASYSIZE

public:
   AllocSetEditor(CWnd* pParent = NULL);   // standard constructor
   AllocSetEditor(SpatialAllocator*, AllocationSet*, CWnd* pParent = NULL);   
   virtual ~AllocSetEditor();

   void SetAllocationSet( AllocationSet *pSet ) { m_pCurrentSet = pSet;  LoadFields(); }  // note:  should check whether anything neeeds to be saved first
   void SetMapLayer(MapLayer *pLayer) { m_pMapLayer = pLayer; }

   // Dialog Data
   enum { IDD = IDD_ALLOCSETDLG };

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void LoadFields(void);

public:
   bool SaveFields( void );   // save current dlg entries to local AllocationSet

   DECLARE_MESSAGE_MAP()

public:
   MapLayer          *m_pMapLayer;
   AllocationSet     *m_pCurrentSet;   // ptr into local AllocationSetArray

   CString           m_allocationSetName;
   CString           m_description;
   CComboBox         m_allocationSetField;
   BOOL              m_inUse;
   BOOL              m_shuffle;
   BOOL              m_useSequences;
   CComboBox         m_sequenceField;
 
   float             m_rate;
   CString           m_timeSeriesValues;
   CString           m_timeSeriesFile;

   virtual BOOL OnInitDialog();
   int m_targetSourceGroupIndex;

protected:
   afx_msg void OnAddNew();
   afx_msg void OnCopy();
   afx_msg void OnDelete();
   afx_msg void OnImportXml();
   afx_msg void OnClose();
   afx_msg void OnSaveXml();

   afx_msg void OnOKAS();

   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void EnableCtrls();
public:
   afx_msg void OnBnClickedBrowse();
   CString m_fromFile;
   CComboBox m_targetBasis;
   afx_msg void OnAddNewAllocation();
   afx_msg void OnEnChangeAsName();
   };
