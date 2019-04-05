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
#include "afxwin.h"
#include "afxcmn.h"

class IniFileEditor;


// PPIniBasic dialog

class PPIniBasic : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPIniBasic)

public:
	PPIniBasic(IniFileEditor* pParent);   // standard constructor
	virtual ~PPIniBasic();

// Dialog Data
	enum { IDD = IDD_INIBASIC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void UpdateCounts();

	DECLARE_MESSAGE_MAP()
public:
   IniFileEditor *m_pParent;

   bool StoreChanges();

   virtual BOOL OnInitDialog();
   
   afx_msg void OnBnClickedImportPolicies();
   afx_msg void OnBnClickedImportActors();
   afx_msg void OnBnClickedImportScenarios();
   afx_msg void OnBnClickedImportLulcTree();


   afx_msg void MakeDirty();
   afx_msg void OnBnClickedRunpolicyeditor();
   afx_msg void OnBnClickedRunactoreditor();
   afx_msg void OnBnClickedRunscenarioeditor();
   afx_msg void OnBnClickedRunlulceditor();
   };
