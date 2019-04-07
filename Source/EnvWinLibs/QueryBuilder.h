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
#include "EnvWinLibs.h"
#include "EnvWinLib_resource.h"

class MapLayer;
class MapWindow;
class QueryEngine;

// QueryBuilder dialog

class WINLIBSAPI QueryBuilder : public CDialog
{
	DECLARE_DYNAMIC(QueryBuilder)

public:
	QueryBuilder(QueryEngine *pDefQueryEngine, MapWindow *pMapWnd, MapLayer *pInitLayer=NULL, int record = -1, CWnd* pParent = NULL);   // standard constructor
	virtual ~QueryBuilder();

// Dialog Data
	enum { IDD = IDD_LIB_QUERYBUILDER };

   int m_record;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void LoadLayer();
   void LoadFieldValues();
   void LoadHistory();

   MapLayer  *m_pLayer;           // memory managed elsewhere
   MapWindow *m_pMapWnd;          // memory managed elsewhere
   QueryEngine *m_pQueryEngine;   // memory managed elsewhere

   DECLARE_MESSAGE_MAP()
public:
   CString m_queryString;

   static CStringArray m_queryStringArray;

   CComboBox m_layers;
   CComboBox m_ops;
   CComboBox m_values;
   CEdit m_query;
   CComboBox m_fields;
   CComboBox m_historyCombo;

   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnCbnSelchangeFields();
   afx_msg void OnCbnSelchangeLayers();
   afx_msg void OnBnClickedAnd();
   afx_msg void OnBnClickedOr();

protected:
   virtual void OnOK();
public:
   BOOL m_runGlobal;
   BOOL m_clearPrev;
   afx_msg void OnBnClickedSpatialops();
   afx_msg void OnBnClickedQueryviewer();
   afx_msg void OnBnClickedCancel();
   afx_msg void OnCbnSelchangeHistory();
   afx_msg void OnBnClickedRun();
   };
