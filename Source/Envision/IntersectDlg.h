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

#include <Typedefs.h>

class MapLayer;
class Raster;

// IntersectDlg dialog

enum { MAJORITY, SUM, AVG, PERCENTAGES };


class IntersectDlg : public CDialogEx
{
	DECLARE_DYNAMIC(IntersectDlg)

public:
	IntersectDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~IntersectDlg();

// Dialog Data
	enum { IDD = IDD_INTERSECT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void EnableControls();

	DECLARE_MESSAGE_MAP()
public:
   CComboBox m_targetLayer;
   CComboBox m_targetField;
   CComboBox m_sourceLayer;
   CComboBox m_mergeField;
   float m_gridSize;
   CStatic m_units;
   CStatic m_memory;
   CStatic m_status;
   CProgressCtrl m_progress;
   CString m_query;
   CString m_sourceRasterPath;
   CString m_indexRasterPath;
   CEdit m_rndxFilename;   
   int m_partRows;
   int m_partCols;

   int m_currentPart;


   void LoadTargetFields();
   void LoadMergeFields();   

   bool PopulateTargetLayer( MapLayer *pTargetLayer, MapLayer *pMergeLayer, int colTarget, int colMerge, TYPE targetType, TYPE mergeType, int rule, Raster *pRaster );
   bool PopulateTargetLayerPoly( MapLayer *pTargetLayer, MapLayer *pMergeLayer, int colTarget, int colMerge, TYPE targetType, TYPE mergeType, int rule, Raster *pRaster, int polyIndex );

   
   afx_msg void OnCbnSelchangeTargetlayer();
   afx_msg void OnCbnSelchangeIntersectlayer();
   afx_msg void OnEnChangeGridsize();
   afx_msg void OnBnClickedBrowse();
   virtual void OnOK();
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedQuerybuilder();

   afx_msg void OnBnClickedBrowserndx();
   afx_msg void OnBnClickedBrowsesourcebmp();
   afx_msg void OnBnClickedBrowseindexbmp();
   afx_msg void OnBnClickedUserndx();

   };
