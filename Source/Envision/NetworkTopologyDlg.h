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
#include <ReachTree.h>
#include <NetworkTree.h>
#include "afxcmn.h"


// NetworkTopologyDlg dialog

class NetworkTopologyDlg : public CDialog
{
	DECLARE_DYNAMIC(NetworkTopologyDlg)

public:
	NetworkTopologyDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~NetworkTopologyDlg();

// Dialog Data
	enum { IDD = IDD_REACHTREE };

public:
   //NetworkTree *m_pTree;
   ReachTree  *m_pTree;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void LoadFields();
   void UpdateInterface();

	DECLARE_MESSAGE_MAP()
public:
   CComboBox m_layers;
   CComboBox m_colNodeID;
   CComboBox m_colToNode;
   CComboBox m_colOrder;
   CComboBox m_colTreeID;
   CComboBox m_colReachIndex;

   CEdit m_flowDirGrid;
   CButton m_populateOrder;
   CButton m_populateTreeID;
   CButton m_populateReachIndex;
   CStatic m_nodesGenerated;
   CStatic m_singleNodeReaches;
   CStatic m_phantomNodesGenerated;
   afx_msg void OnBnClickedGenerate();
   afx_msg void OnCbnSelchangeLayers();
   afx_msg void OnBnClickedPopulateorder();
   afx_msg void OnBnClickedUseattr();
   afx_msg void OnBnClickedUsevertices();
   virtual BOOL OnInitDialog();

   afx_msg void OnBnClickedBrowse();
   CProgressCtrl m_progress;
   CStatic m_progressText;
   };
