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

#include "SpatialAllocator.h"
#include <QueryEngine.h>

#include <EasySize.h>

// AllocExpandPP dialog

class AllocExpandPP : public CTabPageSSL
{
   DECLARE_DYNAMIC(AllocExpandPP)

   DECLARE_EASYSIZE

public:
   AllocExpandPP();   // standard constructor
   virtual ~AllocExpandPP();

   // Dialog Data
   enum { IDD = IDD_ALLOCEXPAND };

   void LoadFields(Allocation *pAlloc);
   bool SaveFields(Allocation *pAlloc);

public:
   void SetMapLayer(MapLayer *pLayer);
   void SetQueryEngine(QueryEngine *pQE) { m_pQueryEngine = pQE; }
   void RunQuery();


protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
   Allocation       *m_pCurrentAllocation;
   MapLayer         *m_pMapLayer;
   BOOL              m_allowExpansion;
   BOOL              m_limitToQuery;
   CString           m_expandQuery;
   BOOL              m_limitSize;
   CString           m_expandMaxSize;
   QueryEngine      *m_pQueryEngine;



   virtual BOOL OnInitDialog();
   DECLARE_MESSAGE_MAP()
   afx_msg void OnBnClickedUseconstraintquery();
   afx_msg void OnBnClickedUpdateMap();
   afx_msg void OnBnClickedClearQuery();
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void EnableCtrls();
   afx_msg void OnBnClickedQueryEditor();
   };
