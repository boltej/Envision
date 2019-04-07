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

#include "plot3d/ScatterPlot3DWnd.h"
#include "plot3d/ShapePreviewWnd.h"

#include "EnvWinLib_resource.h"

//  ScatterPropertiesDlg.h


class ScatterPropertiesDlg : public CDialog
{

public:
	ScatterPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~ScatterPropertiesDlg();

private:
   PointCollectionArray  m_dataArray;
   PointCollectionArray *m_pParentDataArray;

   CComboBox m_dataSetCombo;
   CComboBox m_lineWidthCombo;
   CComboBox m_pointSizeCombo;
   CComboBox m_pointTypeCombo;
   CStatic   m_colorStatic;
   CStatic   m_colorStaticLines;
   CStatic   m_previewStatic;

   ShapePreviewWnd m_previewWnd;

   void UpdateProperties( void );

// Dialog Data
	enum { IDD = IDD_LIB_S3D_DATA_SET_PROPS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedSetColorButton();
   afx_msg void OnBnClickedSetColorButtonLines();
   afx_msg void OnPaint();
   afx_msg void OnCbnSelchangeDataSetCombo();
protected:
   virtual void OnOK();
public:
   afx_msg void OnCbnSelchangeLineWidthCombo();
   afx_msg void OnCbnSelchangePointSizeCombo();
   afx_msg void OnCbnSelchangePointTypeCombo();
};