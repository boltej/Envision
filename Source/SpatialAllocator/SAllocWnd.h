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
#if !defined(AFX_SAllocWnd_H__A76913A7_C847_11D3_95C5_00A076B0010A__INCLUDED_)
#define AFX_SAllocWnd_H__A76913A7_C847_11D3_95C5_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SAllocWnd.h : header file
//

#include <MapPanel.h>
#include <afxtempl.h>
#include <EasySize.h>

#include "SpatialAllocator.h"


// this class is a container for the right-hand side
// AllocationSetEditor and AllocationEditor


class SAllocWnd : public CWnd
{
   // Construction
public:
   SAllocWnd();
   virtual ~SAllocWnd();

   DECLARE_DYNCREATE(SAllocWnd)
   DECLARE_EASYSIZE

   // Attributes
public:
   PtrArray< CWnd > m_wndArray;

protected:
   MapLayer *m_pMapLayer;
   CWnd     *m_pActiveWnd;

   // Operations
public:
   void  AddWnd(CWnd *pWnd ) { MakeActive((CWnd*)m_wndArray.Add(pWnd));  }

   // Implementation
public:
   void MakeActive(CWnd *pWnd);
   void Activate( AllocationSet *pSet, bool isResult );
   void Activate( Allocation *pAlloc );
   //void AddNew(AllocationSet *pSet);
   //void AddNew(Allocation *pAlloc);
   //bool Save( AllocationSet *pSet );
   //bool Save( Allocation *pAlloction );
   bool SaveFields( void );

   void SetMapLayer(MapLayer *pLayer) { m_pMapLayer = pLayer; }
   // Generated message map functions
protected:
   DECLARE_MESSAGE_MAP()

public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  // afx_msg void OnParentNotify(UINT message, LPARAM lParam);
   afx_msg void OnPaint();
   afx_msg void OnSize(UINT nType, int cx, int cy);
 
   afx_msg void OnSave( void );
   
   void OnCellproperties( MapWindow *pMapWnd, int currentCell, CWnd *pParent );

   CButton m_btnAddNew;



};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAllocWnd_H__A76913A7_C847_11D3_95C5_00A076B0010A__INCLUDED_)
