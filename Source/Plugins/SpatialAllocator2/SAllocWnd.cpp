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
// SAllocWnd.cpp : implementation file
//

#include "stdafx.h"
#include "SAllocWnd.h"

#include "AllocSetEditor.h"
#include "AllocEditor.h"
#include "AllocSetResults.h"

#include <EasySize.h>
const int BTN_HEIGHT = 40;

#include "SAllocView.h"
extern SAllocView *gpSAllocView;
extern SpatialAllocator *gpSpatialAllocator;


//#include "ActiveWnd.h"


/////////////////////////////////////////////////////////////////////////////
// SAllocWnd

IMPLEMENT_DYNCREATE(SAllocWnd, CWnd)


BEGIN_MESSAGE_MAP(SAllocWnd, CWnd)
   ON_WM_CREATE()
   ON_WM_PAINT()
   ON_WM_SIZE()
END_MESSAGE_MAP()


BEGIN_EASYSIZE_MAP(SAllocWnd)
   //         control         left      top        right        bottom     options
//    EASYSIZE(IDC_ADDNEW,      ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,  0 )
//    EASYSIZE(IDC_COPY,        ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,  0 )
//    EASYSIZE(IDC_DELETE,      ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,  0 )
//    EASYSIZE(IDC_IMPORT,      ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,  0 )
//    EASYSIZE(IDC_SAVE,        ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,  0 )
//    EASYSIZE(IDOK,            ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,  0 )
//    EASYSIZE(IDCANCEL,        ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,  0 )

END_EASYSIZE_MAP


SAllocWnd::SAllocWnd( )
   : m_pMapLayer( NULL )
   , m_pActiveWnd( NULL )
{ }


SAllocWnd::~SAllocWnd()
{ }


void SAllocWnd::MakeActive(CWnd *pWnd)
   {
   if (pWnd != NULL)
      ASSERT(IsWindow(pWnd->m_hWnd));

   // deactivate all windows
   for (int i = 0; i<m_wndArray.GetCount(); i++)
      {
      CWnd *_pWnd = m_wndArray.GetAt(i);
      _pWnd->ShowWindow( SW_HIDE );
      }

   if (pWnd != NULL)
      pWnd->ShowWindow( SW_SHOW );

   m_pActiveWnd = pWnd;
   }


// all the allocWnd has to do is resize the windows it manage

void SAllocWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
   UPDATE_EASYSIZE;

   CRect tmpRect = CRect(0, 0, cx, cy-BTN_HEIGHT);
   for (int i = 0; i < m_wndArray.GetSize(); i++)
      {
      CWnd *pWnd = m_wndArray[i];
      if ( ::IsWindow(pWnd->GetSafeHwnd()) )
         pWnd->MoveWindow(&tmpRect);
      }

   RedrawWindow();
   }

void SAllocWnd::OnPaint()
{
   CPaintDC dc(this); // device context for painting
   CFont font;
   font.CreatePointFont(90, "Arial");
   CFont *pFont = dc.SelectObject(&font);
   CString text("Click on a tree item to display it in this view");
   dc.TextOut(15, 15, text);
   dc.SelectObject(pFont);
}

int SAllocWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   this->SetWindowText("SAllocWnd");
   AllocSetEditor *pAllocSetEditor = new AllocSetEditor(this);
   AllocEditor *pAllocEditor = new AllocEditor(this);
   AllocSetResults *pAllocSetResults = new AllocSetResults(this);

   pAllocSetEditor->SetMapLayer( m_pMapLayer);
   pAllocEditor->SetMapLayer( m_pMapLayer );
   //pAllocSetResults->SetMapLayer( m_pMapLayer );

   BOOL ok = pAllocSetEditor->Create(AllocSetEditor::IDD, this);
   ok = pAllocEditor->Create(AllocEditor::IDD, this);
   ok = pAllocSetResults->Create(AllocSetResults::IDD, this);

   m_wndArray.Add(pAllocSetEditor); 
   m_wndArray.Add(pAllocEditor); 
   m_wndArray.Add(pAllocSetResults);

   pAllocSetEditor->ShowWindow(SW_SHOW);
   pAllocEditor->ShowWindow(SW_HIDE);
   pAllocSetResults->ShowWindow(SW_HIDE);
   
   pAllocSetEditor->SetWindowText("AllocSetEditor");
   pAllocEditor->SetWindowText("AllocEditor");
   pAllocSetResults->SetWindowText("AllocSetResults");

   // initially, locate button at bottom of window
   CRect rect;
   GetClientRect( &rect );
   rect.top = rect.bottom-BTN_HEIGHT;
   m_btnAddNew.Create( "Add New", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rect, this, IDC_ADDNEW );

   INIT_EASYSIZE;

   return true;
}


void SAllocWnd::Activate( AllocationSet *pSet, bool isResult )
   {
   if ( isResult )
      {
      AllocSetResults *pWnd = (AllocSetResults*) m_wndArray[ 2 ];   // NOTE: ASSUMES ORDER!!!!  BAD!!!!
      MakeActive( pWnd );
      pWnd->SetAllocationSet( pSet );
      //pWnd->UpdateData( 0 );
      }
   else
      {
      AllocSetEditor *pWnd = (AllocSetEditor*) m_wndArray[ 0 ];   // NOTE: ASSUMES ORDER!!!!  BAD!!!!
      MakeActive( pWnd );
      pWnd->SetAllocationSet( pSet );
      pWnd->UpdateData( 0 );
      }
   }


void SAllocWnd::Activate( Allocation *pAlloc)
   {
   AllocEditor *pWnd = (AllocEditor*) m_wndArray[ 1 ];   // NOTE: ASSUMES ORDER!!!!  BAD!!!!

   MakeActive( pWnd );
   pWnd->SetAllocation( pAlloc );
   pWnd->UpdateData( 0 );

   if ( pAlloc == NULL )
      return;

   // calculate allocation scores
   SpatialAllocator *pSA = new SpatialAllocator(*(gpSAllocView->m_pSpatialAllocator ));
   gpSAllocView->UpdateSpatialAllocator( pSA );

   pSA->ScoreIduAllocation( pAlloc, 0 );

  int colAllocScore = m_pMapLayer->GetFieldCol("ALLOC_SCORE" );
   if ( colAllocScore < 0  )
      colAllocScore = m_pMapLayer->m_pDbTable->AddField("ALLOC_SCORE", TYPE_FLOAT );

   // scan delta array for allocation IDs
   int iduCount = m_pMapLayer->GetRecordCount();

   for ( int i=0; i < iduCount; i++ )
      {
      //float score = pAlloc->m_iduScoreArray[ i ].score;
      //m_pMapLayer->SetData( i, colAllocScore, score );
      }

   m_pMapLayer->SetActiveField( colAllocScore );
   m_pMapLayer->SetBins( colAllocScore, BCF_GREEN_INCR );
   }


void SAllocWnd::OnSave( void )
   {
   SaveFields();
   }


bool SAllocWnd::SaveFields()
   {
   AllocSetEditor *pAllocSetEditor = (AllocSetEditor*) m_wndArray[ 0 ];   // NOTE: ASSUMES ORDER!!!!  BAD!!!!
   bool ok1 = pAllocSetEditor->SaveFields();
   
   AllocEditor *pAllocEditor = (AllocEditor*) m_wndArray[ 1 ];   // NOTE: ASSUMES ORDER!!!!  BAD!!!!
   bool ok2 = pAllocEditor->SaveFields();

   return ok1 && ok2;
   }

//bool SAllocWnd::Save( AllocationSet *pSet )
//   {
//   AllocSetEditor *pWnd = (AllocSetEditor*) m_wndArray[ 0 ];   // NOTE: ASSUMES ORDER!!!!  BAD!!!!
//   return pWnd->Save();
//   }
//
//
//bool SAllocWnd::Save( Allocation *pAlloc )
//   {
//   AllocEditor *pWnd = (AllocEditor*) m_wndArray[ 1 ];   // NOTE: ASSUMES ORDER!!!!  BAD!!!!
//   return pWnd->SaveFields();
//   }
//


