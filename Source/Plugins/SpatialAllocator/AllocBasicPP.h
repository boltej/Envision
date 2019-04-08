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
#include <EasySize.h>

// AllocBasicPP dialog

class AllocBasicPP : public CTabPageSSL
{
   DECLARE_DYNAMIC(AllocBasicPP)

   DECLARE_EASYSIZE

public:
   AllocBasicPP();   // standard constructor
   virtual ~AllocBasicPP();

   // Dialog Data
   enum { IDD = IDD_ALLOCBASIC };

   void LoadFields(Allocation *pAlloc);
   bool SaveFields(Allocation *pAlloc);
   void SetMapLayer(MapLayer *pLayer) { m_pMapLayer = pLayer; }

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void EnableCtrls();
   afx_msg void OnBnClickedBrowse();

   DECLARE_MESSAGE_MAP()

public:
   Allocation       *m_pCurrentAllocation;
   MapLayer         *m_pMapLayer;
   CString           m_allocationName;
   int               m_allocationCode; 
   float             m_rate;
   CString           m_timeSeriesValues;
   CString           m_timeSeriesFile;
 
   virtual BOOL OnInitDialog();
   int m_targetSourceGroupIndex;
   CString m_fromFile;
   afx_msg void OnEnChangeAllocationname();
   };
