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

#include <dbtable.h>
#include <Maplayer.h>
#include "afxcmn.h"

// MergeDlg dialog

class MergeDlg : public CDialog
{
	DECLARE_DYNAMIC(MergeDlg)

public:
	MergeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~MergeDlg();

// Dialog Data
	enum { IDD = IDD_MERGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   CComboBox m_targetLayerCombo;
   CCheckListBox m_fields;
   CEdit m_database;

   MapLayer *m_pSpatialJoinLayer;
   DbTable  *m_pJoinTable;
   bool      m_deleteJoinTable;

   CComboBox m_joinField;
   CComboBox m_joinFieldSource;
   CComboBox m_targetFieldToPopulate;
   CComboBox m_mergeIndexField;
   CComboBox m_targetIndexField;
   float m_threshold;

protected:
   void LoadJoinFields(void);
   int  GetCountFieldsToMerge();

public:
   afx_msg void OnBnClickedBrowse();
   virtual BOOL OnInitDialog();

   afx_msg void OnBnClickedOk();
   afx_msg void OnCbnSelchangeLayers();
   CStatic m_status;
   CProgressCtrl m_progress;
   afx_msg void OnBnClickedAddfield();
   };
