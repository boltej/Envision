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
#include <grid\gridctrl.h>

class IniFileEditor;



// PPIniSettings dialog

class PPIniSettings : public CTabPageSSL
{
	DECLARE_DYNAMIC(PPIniSettings)

public:
	PPIniSettings( IniFileEditor* );   // standard constructor
	virtual ~PPIniSettings();
   
   IniFileEditor *m_pParent;

   // Dialog Data
	enum { IDD = IDD_INISETTINGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
	DECLARE_MESSAGE_MAP()
   afx_msg void OnBnClickedMapoutputs();

   BOOL m_loadShared;
   BOOL m_debug;
   BOOL m_mapOutputs;
   BOOL m_noBuffer;
   int  m_interval;         // default simulation period (years)
   int  m_defaultPeriod;
   //CListCtrl m_vars;

public:
   bool StoreChanges();

   virtual BOOL OnInitDialog();
   
   afx_msg void MakeDirty();
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedRemove();
   CGridCtrl m_varGrid;
   int m_mapUnits;
   };
