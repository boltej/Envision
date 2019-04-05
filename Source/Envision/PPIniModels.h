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

#include <TabPageSSL.h>
#include <cubicSpline.h>
#include "afxwin.h"

class IniFileEditor;
class EnvEvaluator;


// PPIniModels dialog

class PPIniModels : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPIniModels)

public:
	PPIniModels(IniFileEditor *pParent);   // standard constructor
	virtual ~PPIniModels();

// Dialog Data
	enum { IDD = IDD_INIMODELS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
   
   void LoadModel();
   int  SaveModel();

   bool m_isModelDirty;

public:
   bool StoreChanges();

public:
   IniFileEditor *m_pParent;
   EnvEvaluator *m_pInfo;

   CCheckListBox m_models;
   CString m_label;
   CString m_path;
   int m_id;
   //BOOL m_useSelfInterested;
   //BOOL m_useAltruism;
   BOOL m_showInResults;
   BOOL m_showUnscaled;
   BOOL m_useCol;
   CString m_field;
   CString m_initFnInfo;
   BOOL m_cdf;
   float m_minRaw;
   float m_maxRaw;
   CString m_rawUnits;
   CubicSplineWnd m_spline;

   afx_msg void OnBnClickedBrowse();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedRemove();
   afx_msg void OnBnClickedUsecol();
   afx_msg void OnBnClickedCdf();
   afx_msg void OnEnChangeLabel();

   afx_msg void OnLbnChkchangeModels();

   virtual BOOL OnInitDialog();

   afx_msg void MakeDirty();

   afx_msg void OnLbnSelchangeModels();
   };
