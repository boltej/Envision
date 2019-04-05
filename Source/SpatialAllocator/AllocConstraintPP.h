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

#include "Resource.h"
#include <TabPageSSL.h>

#include <EasySize.h>

#include <MapWnd.h>
#include <QueryEngine.h>
#include "afxwin.h"


// AllocConstraintPP dialog

class AllocConstraintPP : public CTabPageSSL //, public MapWndContainer
{
	DECLARE_DYNAMIC(AllocConstraintPP)

   DECLARE_EASYSIZE

public:
	AllocConstraintPP();
	virtual ~AllocConstraintPP();

// Dialog Data
	enum { IDD = IDD_ALLOCCONSTRAINT };

   void LoadFields(Allocation *pAlloc);
   bool SaveFields( Allocation *pAlloc );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

public:
   virtual void SetMapLayer( MapLayer *pLayer );
   void SetQueryEngine(QueryEngine *pQE) { m_pQueryEngine = pQE; }
   void RunQuery( void );
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedEditConstraintQuery();
   afx_msg void OnBnClickedUpdateMap();
   afx_msg void OnBnClickedClearquery();

   //afx_msg void OnCellproperties();
   //afx_msg void OnSelectField(UINT nID);

protected:
   Allocation    *m_pCurrentAllocation;
   QueryEngine   *m_pQueryEngine;
   MapLayer      *m_pMapLayer;
   CString        m_queryStr;

public:
   //CString m_constraintName;
   //afx_msg void OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );
   };
