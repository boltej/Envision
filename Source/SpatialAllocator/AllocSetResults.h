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

#include "afxcmn.h"
#include "afxwin.h"

#include "Resource.h"
#include "SpatialAllocator.h"
#include "SAllocWnd.h"
#include <Vdataobj.h>
#include <EasySize.h>
#include <grid\gridctrl.h>


// AllocSetResults dialog


class AllocSetResults : public CDialog 
{
   DECLARE_DYNAMIC(AllocSetResults)

   DECLARE_EASYSIZE

public:
   AllocSetResults( CWnd* pParent = NULL);   // standard constructor
   virtual ~AllocSetResults();

   void Run( void );
   void SetAllocationSet( AllocationSet *pAllocSet );
   
   // Dialog Data
   enum { IDD = IDD_ALLOCSETRESULTSDLG };

   AllocationSet* m_pAllocSet;
   VDataObj m_saRunData;

protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void LoadResultsGrid();
   void LoadResultsList( SpatialAllocator *pSA );

   CGridCtrl  m_grid;
   CListBox m_resultsList;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnSize(UINT nType, int cx, int cy);

   //afx_msg void OnCellproperties();
   //afx_msg void OnSelectField(UINT nID);

   DECLARE_MESSAGE_MAP()
public:
 
   virtual BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );

   };

