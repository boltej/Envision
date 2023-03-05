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

#include <EnvPolicy.h>

#include <TabPageSSL.h>

class PolEditor;
class MapLayer;

// PPPolicySiteAttr dialog

class PPPolicySiteAttr : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPPolicySiteAttr)

public:
	PPPolicySiteAttr( PolEditor*, EnvPolicy *&pPolicy );
	virtual ~PPPolicySiteAttr();

   // Dialog Data
	enum { IDD = IDD_SITEATTR };
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
   EnvPolicy *&m_pPolicy;
   PolEditor *m_pParent;
   MapLayer  *m_pLayer;

public:
   CComboBox m_fields;
   CComboBox m_ops;
   CComboBox m_values;
   CComboBox m_layers;
   CEdit m_query;

public:
   void LoadPolicy();
   bool StoreChanges();
   void LoadFieldInfo();

protected:
   void LoadFieldValues();
   bool CompilePolicy();
   void MakeDirty();

protected:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnCbnSelchangeFields();
   afx_msg void OnBnClickedAnd();
   afx_msg void OnBnClickedOr();
   afx_msg void OnBnClickedSpatialops();
   afx_msg void OnEnChangeQuery();
   afx_msg void OnBnClickedUpdate();
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnCbnSelchangeLayers();
   afx_msg void OnBnClickedCheck();
   };
