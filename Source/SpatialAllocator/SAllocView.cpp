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
// SAllocView.cpp : implementation file

#include "stdafx.h"
#include "SpatialAllocator.h"
#include "SAllocView.h"
#include <DirPlaceholder.h>

SAllocView *gpSAllocView = NULL;

const int ID_SPLITTER = 1;


IMPLEMENT_DYNAMIC(SAllocView, CDialogEx)


SAllocView::SAllocView(SpatialAllocator *pSAlloc, MapLayer *pLayer, CRect &initRect, EnvContext *pContext, CWnd* pParent /*=NULL*/)
   : CDialogEx(SAllocView::IDD, pParent)
 , m_pSpatialAllocator(pSAlloc)
 , m_pSAllocWnd(NULL)
 , m_pTreeWnd(NULL)
 , m_pMapContainer( NULL )
 , m_pMapLayer( pLayer )
 , m_initRect( initRect )
 , m_pEnvContext( pContext )
   {
   gpSAllocView = this;

   m_allocationSetArray.DeepCopy( pSAlloc->m_allocationSetArray );
   }


SAllocView::~SAllocView()
   {
   if (m_pSAllocWnd != NULL)
      delete m_pSAllocWnd;

   if (m_pTreeWnd != NULL)
      delete m_pTreeWnd;

   if (m_pMapContainer != NULL)
      delete m_pMapContainer;
   }


BEGIN_MESSAGE_MAP(SAllocView, CDialogEx)
   ON_WM_SIZE()
END_MESSAGE_MAP()


//BEGIN_EASYSIZE_MAP(SAllocView)
//   //         control         left      top        right        bottom     options
//    EASYSIZE(ID_PLACEHOLDER, ES_BORDER, ES_BORDER,   ES_BORDER, ES_BORDER,  0 )
//END_EASYSIZE_MAP

void SAllocView::DoDataExchange(CDataExchange* pDX)
   {
   CDialogEx::DoDataExchange(pDX);
   }


BOOL SAllocView::OnInitDialog()
   {
   CDialogEx::OnInitDialog();

   // create high-level windows
   m_pTreeWnd = new SAllocTree;
   m_pSAllocWnd = new SAllocWnd;
   m_pMapContainer = new MapContainer;

   // set them up
   m_pSAllocWnd->SetMapLayer( m_pMapLayer );
   m_pMapContainer->SetMapLayer( m_pMapLayer );
   m_pTreeWnd->m_pSAllocWnd = m_pSAllocWnd;
   
   m_splitterWnd2.LeftRight( m_pSAllocWnd, m_pMapContainer );
   m_splitterWnd2.MoveBar(540);

   CWnd *pLeft  = m_pTreeWnd; //     new CWnd;
   CWnd *pRight = &m_splitterWnd2;
   
   //CWnd *pRight =  m_pSAllocWnd;   // new CWnd;
   m_splitterWnd.LeftRight(pLeft, pRight);
   m_splitterWnd.MoveBar(160);
   m_splitterWnd.Create(NULL, NULL, WS_VISIBLE, CRect(0, 0, 800, 800), this, ID_SPLITTER);
   

   CWnd *pWnd = this->GetDlgItem( IDC_PLACEHOLDER );
   pWnd->ShowWindow( SW_HIDE );

   this->MoveWindow( &m_initRect );
   //INIT_EASYSIZE;
   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void SAllocView::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
   //UPDATE_EASYSIZE;

   if (IsWindow(m_splitterWnd.m_hWnd))
      {
      m_splitterWnd.MoveWindow(  0,0,cx,cy, TRUE );
      }
   }


bool SAllocView::UpdateSpatialAllocator( SpatialAllocator *pSA )
   {
   // replaces the allocation/allocation sets in the passed-in spatial allocator
   // with copies of the local set (this->m_allocationSetArray).
   // Note that any existing AllocationSets in the passed-in SpatialAllocator
   // will be deleted and replaced with copies of the local AllocationSets

   // first, copy data from dlg to local copy of the allocations
   this->m_pSAllocWnd->SaveFields();

   // then, replace the pSA's allocation sets with new ones based on our local copy
   pSA->ReplaceAllocSets( this->m_allocationSetArray );

   return true;
   }


bool SAllocView::SaveXml( void )
   {
   DirPlaceholder d;
   
   // TODO get path
   CString startPath = m_pSpatialAllocator->m_xmlPath;
   CString ext = _T("xml");
   CString extensions = _T("XML Files|*.xml|All files|*.*||");
   
   CFileDialog fileDlg(FALSE, ext, startPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, extensions);
   if (fileDlg.DoModal() == IDOK)
      {
      CString pathName = fileDlg.GetPathName();

      UpdateSpatialAllocator( m_pSpatialAllocator );   // write current dlg values to internal data structures
      m_pSpatialAllocator->SaveXml( pathName );
      
      return true;
      }

   return false;
   }


bool SAllocView::ImportXml( void )
   {
   // need to verify this works correctly
   DirPlaceholder d;
   
   // TODO get path
   CString startPath = m_pSpatialAllocator->m_xmlPath;
   CString ext = _T("xml");
   CString extensions = _T("XML Files|*.xml|All files|*.*||");
   
   CFileDialog fileDlg(TRUE, ext, startPath, OFN_HIDEREADONLY, extensions);
   if (fileDlg.DoModal() == IDOK)
      {
      CString pathName = fileDlg.GetPathName();

      m_pSpatialAllocator->LoadXml( pathName, &(this->m_allocationSetArray) );

      this->m_pTreeWnd->LoadTreeCtrl();
      
      
      //Change the window's title to the opened file's title.
      //CString fileName = fileDlg.GetFileTitle();
      //
      //SetWindowText(fileName);
      
      return true;
      }

   return false;
   }


