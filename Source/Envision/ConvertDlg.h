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

class MapLayer;
class VDataObj;

// ConvertDlg dialog

class ConvertDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ConvertDlg)

public:
	ConvertDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~ConvertDlg();

// Dialog Data
	enum { IDD = IDD_CONVERTCOVERAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   
   bool GenerateHexFromPoints(); 
   bool GenerateCentroidsFromLayer(); 
   bool GenerateVoroiniFromLayer(); 

   bool GetCentroids( MapLayer *pLayer, VDataObj *pData );


	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedHexfrompoints();
   afx_msg void OnBnClickedCentroidsCsv();
   afx_msg void OnBnClickedBrowsehexcsv();
   
   virtual BOOL OnInitDialog();
   virtual void OnOK();


   CString m_hexCSVInputFile;
   CStatic m_labelHexCSVFile;
   CEdit m_wndHexCSVInput;
   CButton m_wndBrowseHexCSV;
   CStatic m_labelXCol;
   CStatic m_labelYCol;
   CComboBox m_xCols;
   CComboBox m_yCols;
   float m_hexSpacing;
   CComboBox m_layers;
   };
