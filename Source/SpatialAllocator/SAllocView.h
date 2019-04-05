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
#ifndef __SALLOCVIEW__H
#define __SALLOCVIEW__H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "resource.h"

#include "SpatialAllocator.h"

#include "SAllocView.h"
#include "SAllocWnd.h"
#include "SAllocTree.h"
#include "MapContainer.h"

#include <StaticSplitterWnd.h>

//#include <EasySize.h>
#include "afxwin.h"


/////////////////////////////////////////////////////////////////////////////
// SAllocView window - includes splitter window with map legend, MapWindow

class SAllocView : public CDialogEx
{
   // Construction
   DECLARE_DYNAMIC(SAllocView)
   //DECLARE_EASYSIZE
public:
   SAllocView(CWnd* pParent = NULL);	// standard constructor
   SAllocView(SpatialAllocator *pSAlloc, MapLayer *pLayer, CRect &initRect, EnvContext*, CWnd* pParent=NULL );

   virtual ~SAllocView();

   bool SaveXml( void );
   bool ImportXml( void );
   bool UpdateSpatialAllocator( SpatialAllocator *pSA );   // overwrites the original allocations/allocation sets

   void RedrawMap() { m_pMapContainer->RedrawMap(); }
   void SetMapMsg(LPCTSTR msg) { m_pMapContainer->SetStatusMsg( msg ); }

   // Dialog Data
//  enum { IDD = AFX_IDD_EMPTYDIALOG };
   enum { IDD = IDD_SPATIALALLOCATOR };

   StaticSplitterWnd m_splitterWnd;
   StaticSplitterWnd m_splitterWnd2;

   SAllocWnd       *m_pSAllocWnd;
   SAllocTree      *m_pTreeWnd;
   MapContainer    *m_pMapContainer;

public:
   //CWnd *GetActiveView() { return m_pSAllocWnd->m_pActiveWnd; }

   SpatialAllocator *m_pSpatialAllocator;
   MapLayer         *m_pMapLayer;
   PtrArray< AllocationSet > m_allocationSetArray; // local copy of the current AllocationSet
                                                   // this is the one the Property Pages should interact with
   AllocationSet *m_pCurrentSet;                   // ptr to the current (in the local copy)
   Allocation    *m_pCurrentAllocation;            // ditto
   EnvContext    *m_pEnvContext;                   // The one passed in by Envision

   bool AddAllocSet(int id);

protected:
   virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

protected: 
   virtual BOOL OnInitDialog();
   afx_msg void OnSize(UINT nType, int cx, int cy);

   DECLARE_MESSAGE_MAP()
public:
   CStatic m_placeholder;

protected:
   CRect m_initRect;
   };

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before thevious line.

#endif