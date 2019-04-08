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
// AllocSetResults.cpp : implementation file
//

#include "stdafx.h"
#include "AllocSetResults.h"

#include "SAllocView.h"
#include "..\EnvEngine\EnvModel.h"

extern SAllocView *gpSAllocView;

// AllocSetResults dialog
int SAScoreProc( SpatialAllocator *pSA, int status, int idu, float score, Allocation *pAlloc, float remainingArea, float pctAllocated, LONG_PTR extra );


IMPLEMENT_DYNAMIC(AllocSetResults, CDialog)

AllocSetResults::AllocSetResults(CWnd* pParent /*=NULL*/)
   : CDialog(AllocSetResults::IDD, pParent)
   //, MapWndContainer()
   , m_pAllocSet( NULL)
   , m_saRunData( 5, 0 )
   {
   m_saRunData.SetLabel(0, "IDU");
   m_saRunData.SetLabel(1, "Score");
   m_saRunData.SetLabel(2, "AllocationID");
   m_saRunData.SetLabel(3, "RemainingArea");
   m_saRunData.SetLabel(4, "PctAllocated");
   }

AllocSetResults::~AllocSetResults()
{ }


BEGIN_MESSAGE_MAP(AllocSetResults, CDialog)
   ON_WM_SIZE()
END_MESSAGE_MAP()

BEGIN_EASYSIZE_MAP(AllocSetResults)
   //         control           left           top          right        bottom     options
   EASYSIZE(IDC_AS_HEADER,      ES_BORDER,     ES_BORDER,   ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_GRID,           ES_BORDER,     ES_BORDER,   ES_BORDER,   ES_BORDER,   0)
   EASYSIZE(IDC_RESULTSLIST,    ES_BORDER,     ES_KEEPSIZE, ES_BORDER,   ES_BORDER,   0)
END_EASYSIZE_MAP


// AllocSetResults message handlers
void AllocSetResults::DoDataExchange(CDataExchange* pDX)
   {
   DDX_Control(pDX, IDC_RESULTSLIST, m_resultsList);
   }


BOOL AllocSetResults::OnInitDialog()
   {
   CDialog::OnInitDialog();

   INIT_EASYSIZE;

   CWnd *pWnd = GetDlgItem( IDC_GRID );
   ASSERT( pWnd != NULL );
   
   RECT rect;
   pWnd->GetWindowRect( &rect );
   ScreenToClient( &rect );
   pWnd->ShowWindow( SW_HIDE );

   BOOL ok = m_grid.Create( rect, this, 100, WS_CHILD | WS_TABSTOP | WS_VISIBLE );

   //pWnd = GetDlgItem( IDC_GRID1 );
   //ASSERT( pWnd != NULL );
   //
   //RECT rect;
   //pWnd->GetWindowRect( &rect );
   //ScreenToClient( &rect );
   //pWnd->ShowWindow( SW_HIDE );
   //
   //ok = m_grid1.Create( rect, this, 101, WS_CHILD | WS_TABSTOP | WS_VISIBLE );


   return TRUE;
   }


void AllocSetResults::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);
   UPDATE_EASYSIZE;
  // MapWndContainer::OnSize( nType, cx, cy );

   if ( ::IsWindow( m_grid.GetSafeHwnd() ) )
      {
      CRect rect;
      CWnd  *pPlaceHolder = this->GetDlgItem( IDC_GRID );
      pPlaceHolder->GetWindowRect( &rect );
      pPlaceHolder->ShowWindow( SW_HIDE );
      this->ScreenToClient( &rect );
      m_grid.MoveWindow( &rect, TRUE );
      }
   }


void AllocSetResults::SetAllocationSet( AllocationSet *pAllocSet )
   {
   CString hdr( "Results Summary: ");
   hdr += pAllocSet->m_name;
   //GetDlgItem( IDC_AS_HEADER )->SetWindowText( hdr );

   m_pAllocSet = pAllocSet; 
   Run(); 
   }


void AllocSetResults::Run(void)
   {
   m_saRunData.ClearRows();

   if ( this->m_pAllocSet == NULL )
      return;

   // make a copy of the original spatial allocator
   SpatialAllocator *pSA = new SpatialAllocator( *gpSAllocView->m_pSpatialAllocator );
   
   // Update the new (local) SA to the current dialog values for allocation sets, allocations, etc
   // This deletes any prior copies of pSA's m_allocationSetArray
   gpSAllocView->UpdateSpatialAllocator( pSA );

   // make the current allocationSet the only active set
   for ( int i=0; i < (int) pSA->m_allocationSetArray.GetSize(); i++ )
      {
      if ( gpSAllocView->m_allocationSetArray[ i ] == m_pAllocSet ) // compare to view's allocSetArray!
         pSA->m_allocationSetArray[ i ]->m_inUse = true;
      else
         pSA->m_allocationSetArray[ i ]->m_inUse = false;
      }

   // get ready to do a run
   EnvContext *pContext = gpSAllocView->m_pEnvContext;
   MapLayer   *pLayer = gpSAllocView->m_pMapLayer;
   DeltaArray *pOldArray = pContext->pDeltaArray;
   DeltaArray da( pLayer, 0 );
   pContext->pDeltaArray = &da;
   pContext->pEnvModel->m_pDeltaArray = &da;

   pSA->InstallNotifyHandler(SAScoreProc, (LONG_PTR) this);
   
   pSA->Run( pContext );   // 1) sets Alocation/AllocSet targets, based on pContext->currentYear
                           // 2) scores IDU allocations (each IDU, for each Allocation, stored internally
                           // 3) writes scores to IDUs is pAlloc->m_colScores >= 0 using Delta, NOT SetData
                           // 4) writes ID of "best" Allocation to the pAllocSet->m_col field of the IDU
                           //    using Delta methods
   
   // at this point, the delta array been updated with "Best" Allocation. 
   // update map from delta array
   bool readOnly = pLayer->m_readOnly;
   pLayer->m_readOnly = false;
   int count = 0;

   int colAllocID = pLayer->GetFieldCol("ALLOC_ID");
   if ( colAllocID < 0  )
      colAllocID = pLayer->m_pDbTable->AddField("ALLOC_ID", TYPE_INT);

   MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( m_pAllocSet->m_field );
   if ( pInfo != NULL )
      {
      pInfo->fieldname = "ALLOC_ID";
      pLayer->SetFieldInfo( pInfo );
      }

   pLayer->SetColNoData( colAllocID );

   // scan delta array for allocation IDs
   for ( INT_PTR j = 0; j < da.GetCount(); j++ )
      {
      // does this delta correspond to an AllocID in the allocationSet's column?
      DELTA &d = da.GetAt( j );
      if ( d.col == m_pAllocSet->m_col )
         {
         // yes, so write the value to the ALLOC_ID field in the map
         pLayer->SetData( d.cell, colAllocID, d.newValue );
         count++;
         }
      }

   pLayer->m_readOnly = readOnly;
   
   // set up map
   pLayer->SetActiveField( colAllocID );
   pLayer->SetBins( colAllocID, BCF_MIXED );
   pLayer->ClassifyData();  // setsIDUs to point to correct bin, assumes bins have already been set up
   pLayer->ClearSelection();

   gpSAllocView->RedrawMap();
   
   // update the results table
   LoadResultsGrid();

   LoadResultsList( pSA );

   // clean up
   pContext->pDeltaArray = pOldArray;
   pContext->pEnvModel->m_pDeltaArray = pOldArray;
   delete pSA;

   CString msg;
   msg.Format("%i IDUs updated...", count );
   gpSAllocView->SetMapMsg( msg );
   }


void AllocSetResults::LoadResultsGrid()
   {
   m_grid.DeleteAllItems();

   m_grid.SetColumnCount( 5 );
   m_grid.SetFixedRowCount( 1 );
   m_grid.SetItemText( 0, 0, "Allocation" );
   m_grid.SetItemText( 0, 1, "Score" );
   m_grid.SetItemText( 0, 2, "Remaining Area" );
   m_grid.SetItemText( 0, 3, "Pct Allocated" );
   m_grid.SetItemText( 0, 4, "IDU" );
   m_grid.SetColumnWidth( 0, 50 );
   m_grid.SetColumnWidth( 1, 20 );
   m_grid.SetColumnWidth( 2, 40 );
   m_grid.SetColumnWidth( 3, 30 );
   m_grid.SetColumnWidth( 4, 30 );

   int rows = m_saRunData.GetRowCount();

   m_grid.SetRowCount( rows+1 ); // include fixed (header) row
   m_grid.ExpandColumnsToFit();
   m_grid.SetEditable( FALSE );
   m_grid.EnableSelection( FALSE );

   for ( int i=0; i < rows; i++ )
      {
      m_grid.SetItemText( i+1, 0, m_saRunData.GetAsString( 0, i ) );
      m_grid.SetItemText( i+1, 1, m_saRunData.GetAsString( 1, i ) );
      m_grid.SetItemText( i+1, 2, m_saRunData.GetAsString( 2, i ) );
      m_grid.SetItemText( i+1, 3, m_saRunData.GetAsString( 3, i ) );
      m_grid.SetItemText( i+1, 4, m_saRunData.GetAsString( 4, i ) );

      //m_grid.SetItemState( i+1, 0, m_grid.GetItemState(i+1, 0) | GVIS_READONLY );
      }
   }

void AllocSetResults::LoadResultsList( SpatialAllocator *pSA )
   {
   m_resultsList.ResetContent();

   AllocationSet *pAllocSet = NULL;

   for ( int i=0; i < (int) pSA->m_allocationSetArray.GetSize(); i++ )
      {
      if ( gpSAllocView->m_allocationSetArray[ i ] == m_pAllocSet ) // compare to view's allocSetArray!
         {
         pAllocSet = pSA->m_allocationSetArray[ i ];
         break;
         }
      }

   ASSERT( pAllocSet != NULL );

   m_resultsList.AddString( pAllocSet->m_status );

   for ( int i=0; i < pAllocSet->GetAllocationCount(); i++ )
      {
      CString status = "  ";
      status += pAllocSet->GetAllocation(i)->m_status;
      m_resultsList.AddString( status );
      }
   }


//void AllocSetResults::OnCellproperties() 
//   {
//   MapWndContainer::OnCellproperties();
//   }
//
//void AllocSetResults::OnSelectField( UINT nID) 
//   {
//   m_mapFrame.OnSelectField( nID );
//   }


///////////////////////////////////////////////////////////////////
int SAScoreProc( SpatialAllocator *pSA, int status, int idu, float score, Allocation *pAlloc, float remainingArea, float pctAllocated, LONG_PTR extra )
   {
   VData data[ 5 ];
   CString pct;
   pct.Format( "%.1f", pctAllocated );
   data[ 0 ].AllocateString( pAlloc->m_name );
   data[ 1 ] = (int) score;
   data[ 2 ] = (int) remainingArea;
   data[ 3 ].AllocateString( pct );
   data[ 4 ] = idu;

   AllocSetResults *pThis = (AllocSetResults*) extra;

   pThis->m_saRunData.AppendRow( data, 5 );

   return 1;
   }



BOOL AllocSetResults::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult )
   {
   // TODO: Add your specialized code here and/or call the base class

   if (wParam == (WPARAM) m_grid.GetDlgCtrlID())
      {
      *pResult = 1;

      NM_GRIDVIEW *pMsg = (NM_GRIDVIEW*) lParam;
      int row = pMsg->iRow-1;   // one-based (with header row)

      if ( row > 0 )
         {
         int iduIndex = this->m_saRunData.GetAsInt( 4, row-1 );

         gpSAllocView->m_pMapLayer->ClearSelection();
         gpSAllocView->m_pMapLayer->AddSelection( iduIndex );
         gpSAllocView->RedrawMap();
         }
      }

   return __super::OnNotify( wParam, lParam, pResult );
   }

