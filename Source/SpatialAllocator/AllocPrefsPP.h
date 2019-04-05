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

#include <EasySize.h>

#include <MapWnd.h>
#include <QueryEngine.h>


class AllocPrefsPP : public CTabPageSSL
{
	DECLARE_DYNAMIC(AllocPrefsPP)

   DECLARE_EASYSIZE

public:
	AllocPrefsPP();
	virtual ~AllocPrefsPP();

// Dialog Data
	enum { IDD = IDD_ALLOCPREFS };

   bool SaveFields( Allocation *pAlloc );
   void LoadFields(Allocation *pAlloc);    // fills combo

   void LoadPreference();     // set cur sel to dlg fields

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

public:
   Allocation        *m_pAlloc;
   
   CComboBox         m_preferences;
   CString           m_name;
   CString           m_queryStr;
   CString           m_weightExpr;

   int m_currentIndex;  // current pref selection index

public:
   QueryEngine *m_pQueryEngine;
   MapLayer    *m_pMapLayer;

   virtual void SetMapLayer( MapLayer *pLayer );
   void RunQuery( void );

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedEditPrefQuery();
   afx_msg void OnBnClickedUpdateMap();

   // afx_msg void OnCellproperties();
   // afx_msg void OnSelectField(UINT nID);

   //afx_msg void OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );
   afx_msg void OnBnClickedAddpref();
   afx_msg void OnBnClickedRemovepref();
   afx_msg void OnBnClickedClearquery();
   };
