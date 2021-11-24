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
// ResultsMapWnd.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "ResultsMapWnd.h"
#include "EnvView.h"

#include <map.h>
#include "MapListWnd.h"
#include <VDataObj.h>

#include "EnvDoc.h"
#include <EnvModel.h>
#include <DataManager.h>
#include "MapWnd.h"
#include ".\resultsmapwnd.h"
#include "ActiveWnd.h"
#include "DeltaViewer.h"
#include "PPPolicyResults.h"

#include "mainfrm.h"

extern CEnvDoc    *gpDoc;
extern EnvModel   *gpModel;
extern MapLayer   *gpCellLayer;
extern CMainFrame *gpMain;
extern CEnvView   *gpView;
extern MapPanel   *gpMapPanel;
// ResultsMapWnd
extern int MapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );

ResultsMapWnd::ResultsMapWnd( int run, int fieldInfoOffset, bool isDifference )
 : m_pMap( NULL ),
   m_pMapList( NULL ),
   m_pDeltaArray( NULL ),
   m_run( run ), 
   m_startYear( -1 ),
   m_endYear( -1 ),
   m_iduCol( -1 ),
   m_overlayCol( -1 ),
   m_fieldInfoOffset( fieldInfoOffset ),
   m_isDifference( isDifference )   
   { }


ResultsMapWnd::~ResultsMapWnd()
   {
   CWaitCursor c;

   //if ( m_pMap != NULL )
   //   delete m_pMap;
   //if ( m_pMapList != NULL )
   //   delete m_pMapList;
   }


BEGIN_MESSAGE_MAP(ResultsMapWnd, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_WM_HSCROLL()
   ON_COMMAND_RANGE(1000, 2000, OnSelectOption)
 	ON_COMMAND(ID_SHOWRESULTS, OnShowresults)
 	ON_COMMAND(ID_SHOWDELTAS, OnShowdeltas)
   ON_WM_ACTIVATE()
   ON_WM_MOUSEACTIVATE()
END_MESSAGE_MAP()


int ResultsMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra )
   {
   // Note: "extra" is a pointer to a ResultMapWnd;  MapProc expects the associated MapWnd ptr;
   ResultsMapWnd *pResultsMapWnd = (ResultsMapWnd*) extra;

   MapFrame *pMapFrame = &pResultsMapWnd->m_mapFrame;

   // pass to mapproc for initial processing
   int result = MapProc( pMap, type, a0, a1, (LONG_PTR) pMapFrame );

   // additional processing
   ResultsChildWnd *pResultsChildWnd = (ResultsChildWnd*) pResultsMapWnd->GetParent();
   ResultsWnd *pResultsWnd = (ResultsWnd*) pResultsChildWnd->GetParent();

   if ( pResultsWnd->m_syncMapZoom && type == NT_EXTENTSCHANGED )
      {
      //pResultsWnd->SyncMapZoom( pMap, pResultsMapWnd );
      REAL vxMin, vxMax, vyMin, vyMax;
      pMap->GetViewExtents( vxMin, vxMax, vyMin, vyMax );

      for ( int i=0; i < pResultsWnd->m_wndArray.GetSize(); i++ )
         {
         ResultsChildWnd *pChild = pResultsWnd->m_wndArray[ i ];
         if ( pChild->m_type == RCW_MAP )
            {
            ResultsMapWnd *_pResultsMapWnd = (ResultsMapWnd*) pChild->m_pWnd;

            if ( _pResultsMapWnd != pResultsMapWnd )
               {
               _pResultsMapWnd->m_pMap->SetViewExtents( vxMin, vxMax, vyMin, vyMax );  // doesn't raise NT_EXTENTSCHANGED event
               _pResultsMapWnd->m_mapFrame.m_pMapWnd->RedrawWindow();
               }
            }
         }  // end of: for ( i < resultsWndCount )

      // sync up main window as well
      gpCellLayer->GetMapPtr()->SetViewExtents( vxMin, vxMax, vyMin, vyMax );  // doesn't raise NT_EXTENTSCHANGED event
      gpMapPanel->m_mapFrame.m_pMapWnd->RedrawWindow();
      }  // end of: if ( syncMapZoom && type == NT_EXTENTSCHANGED )

   return result;
   }


void ResultsMapWnd::OnSelectOption( UINT nID )
   {
   if ( nID == 1001 )
      {
      Map::SetupBins( m_pMap, 0, true );  // ClassifyData() is called in here
      m_pMapList->Refresh();
      }
   }

// ResultsMapWnd message handlers

BOOL ResultsMapWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   cs.lpszClass = AfxRegisterWndClass( CS_VREDRAW | CS_HREDRAW, 
		::LoadCursor(NULL, IDC_SIZEWE), reinterpret_cast<HBRUSH>(COLOR_WINDOW), NULL);

   return CWnd::PreCreateWindow(cs);
   }


int ResultsMapWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   MAP_FIELD_INFO *pInfo = gpCellLayer->GetFieldInfo( m_fieldInfoOffset );
   ASSERT( pInfo );

   m_iduCol = gpCellLayer->m_pData->GetCol( pInfo->fieldname );
   ASSERT( m_iduCol >= 0 );

   bool multirun = ( gpModel->m_pDataManager->GetDeltaArrayCount() > 1 );
   
   if ( multirun ) 
      m_pDeltaArray = gpModel->m_pDataManager->GetDeltaArray( m_run );
   else
      m_pDeltaArray = gpModel->m_pDataManager->GetDeltaArray();

   ASSERT( m_pDeltaArray );

   //m_totalYears = m_pDeltaArray->GetMaxYear() + 1;  // + 1 to point to the end of the DeltaArray
   // adjust for non-zero start year
   //m_totalYears -= gpModel->m_startYear;
   m_startYear = gpModel->m_startYear;
   m_endYear   = gpModel->m_endYear;
   m_year = m_startYear;

   // slider
   m_yearSlider.Create( WS_VISIBLE | WS_CHILD | TBS_TOP | TBS_AUTOTICKS, CRect(0,0,0,0), this, RMW_SLIDER );
   m_yearSlider.SetRange( m_startYear, m_endYear );
   m_yearSlider.SetTicFreq( 1 );
   m_yearSlider.SetPos( m_year );
   m_yearSlider.SetPageSize( 1 );

   // edit
   m_yearEdit.Create( WS_VISIBLE | ES_READONLY, CRect(0,0,0,0), this, RMW_EDIT );
   CString str;
   str.Format( "Year: %d", m_year );
   m_yearEdit.SetWindowText( str );

   // map and mapList
   m_mapFrame.Create( NULL, _T("MapFrame"), WS_VISIBLE, CRect( 0,0,0,0 ), this, 101 );
   m_pMap     = m_mapFrame.m_pMap;
   m_pMapList = m_mapFrame.m_pMapList;

   //m_pMap->InstallNotifyHandler( MapProc, (LONG_PTR) &m_mapWnd );  // ResultsMapProc
   //m_pMap->InstallNotifyHandler( ResultsMapProc, (LONG_PTR) this ); defer to post-construction
   m_pMapList->m_pMapWnd = m_mapFrame.m_pMapWnd;

   MapLayer *pLayer = m_pMap->AddLayer( LT_POLYGON );      // add an empty layer
   pLayer->SetNoOutline();

   // make a copy of the polygons
   pLayer->ClonePolygons( gpCellLayer );      // warning: sets no data value to 0!  doesn't copy data, just polys
   pLayer->m_mapUnits = pLayer->m_mapUnits;

   m_pDeltaArray->AddMapLayer( pLayer );      // assocoiate this resultMap with the approriate deltaArray
   // get the needed data
   int rows = pLayer->GetPolygonCount();

   int cols = 1;
   //m_overlayCol = gpCellLayer->m_overlayCol;
   //if ( m_overlayCol >= 0 )  // is there an overlay column?
   //   cols = 2;

   VDataObj *pData = new VDataObj( cols, rows, U_UNDEFINED );
   pData->SetLabel( 0, pInfo->fieldname );

   if ( cols > 1 )
      pData->SetLabel( 1, gpCellLayer->GetFieldLabel( m_overlayCol ) );  // hack for now

   VData data;

   // load data from original (IDU layer) map to the stored one
   for ( int row=0; row < rows; row++ )
      {
      bool ok = gpCellLayer->GetData( row, m_iduCol, data );
      ASSERT( ok );

      ok = pData->Set( 0, row, data );
      ASSERT( ok );

      if ( cols > 1 )
         {
         ok = gpCellLayer->GetData( row, m_overlayCol, data );
         ASSERT( ok );

         ok = pData->Set( 1, row, data );
         ASSERT( ok );
         }
      }

   pLayer->m_pDbTable = new DbTable( pData );   // for creating a DbTable from a preexisting VDataObj
   pLayer->m_pData = pData;
   pLayer->SetNoDataValue( -99 );

   TYPE type = gpCellLayer->GetFieldType( m_iduCol );
   int width, decimals;
   GetTypeParams( type, width, decimals );
   pLayer->SetFieldType( 0, type, width, decimals, false );
   
   // NOTE:  to provide a correct basis for generating the new map, we have to roll the copied data back
   //  to the beginning year.  Do that now.

   // get the current deltaArray for the main IDU coverage
   DeltaArray *pIduDeltaArray = gpModel->m_pDeltaArray;
   ASSERT( pIduDeltaArray != NULL ); 

   // if the IDU coverage is past the beginning, roll back the  just-copied ResultMap layer
   INT_PTR firstUnapplied =  pIduDeltaArray->GetFirstUnapplied( gpCellLayer );
   if ( firstUnapplied > 0 )
      {
      for ( INT_PTR i=pIduDeltaArray->GetFirstUnapplied( gpCellLayer )-1; i >= 0; i-- )
         {
         DELTA &delta = pIduDeltaArray->GetAt( i );

         // if delta changed and the column matches, then unapply it
         if ( ((delta.oldValue.Compare(delta.newValue)) == false ) && (delta.col == m_iduCol || ( m_overlayCol >= 0  && delta.col == m_overlayCol ) ) )
            {
            VData value;
            pLayer->GetData( delta.cell, 0, value );
            ASSERT( value.Compare( delta.newValue ) == true );  // the "new value in the delta should match what's current in the resultMap
            pLayer->SetData( delta.cell, 0, delta.oldValue );   // roll the value back to the previous delta value
            }
         }
      }

    // if difference map, copy starting values and set current value to NULL
   if ( m_isDifference )
      {
      m_startDataArray.SetSize( rows );

      for ( int row=0; row < rows; row++ )
         {
         bool ok = pLayer->GetData( row, 0, data );
         ASSERT( ok );
         m_startDataArray[ row ] = data;

         pLayer->SetData( row, 0, VData() );
         }
      }

   pLayer->ResetBins();
   BinArray *pBinArray = gpCellLayer->GetBinArray( m_iduCol, false );
   if ( pBinArray != NULL )
      pLayer->AddBinArray( new BinArray( *pBinArray ) );

   if( m_overlayCol >= 0 )
      {
      BinArray *pBinArray = gpCellLayer->GetBinArray( m_overlayCol, false );
      if ( pBinArray != NULL )
         pLayer->AddBinArray( new BinArray( *pBinArray ) );
      }
   
   Map::SetupBins( m_pMap, 0, 0 );  // ClassifyData() is called in here
   m_pMapList->Refresh();

   ActiveWnd::SetActiveWnd( &m_mapFrame );
   return 0;
   }


void ResultsMapWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);

   static int gutter     = 5;
   static int bheight    = 40;
   static int editHeight = 20;
   static int editWidth  = 100;

   CSize sz( (cx - 3*gutter)/2, 400 );
   CPoint pt( gutter, gutter );

   if ( IsWindow( m_mapFrame ) )
      m_mapFrame.MoveWindow( 0, 0, cx, cy - bheight - 2*gutter, TRUE );
   
   if ( IsWindow( m_yearSlider ) )
      m_yearSlider.MoveWindow( 0, cy - bheight - gutter, cx - 2*gutter - editWidth, bheight );

   if ( IsWindow( m_yearEdit ) )
      m_yearEdit.MoveWindow( cx - gutter - editWidth, cy - gutter - (bheight+editHeight)/2, editWidth, editHeight );
   }


void ResultsMapWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
   {
   int oldYear = m_year;
   int newYear = m_yearSlider.GetPos();

   if ( oldYear != newYear )
      {
      ResultsChildWnd *pResultsChildWnd = (ResultsChildWnd*) GetParent();
      ResultsWnd *pResultsWnd = (ResultsWnd*) pResultsChildWnd->GetParent();

      pResultsWnd->SetYear( oldYear, newYear );
      }

   CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
   }


 bool ResultsMapWnd::_SetYear( int oldYear, int newYear )
   {
   if ( oldYear != newYear )
      {
      m_year = newYear;

      ShiftMap( oldYear, newYear );

      CString str;
      str.Format( "Year: %d", m_year );

      m_yearEdit.SetWindowText( str );
      m_yearEdit.Invalidate();
      m_yearEdit.UpdateWindow();
      }

   return true;
   }

void ResultsMapWnd::ShiftMap( int fromYear, int toYear )
   {
   ASSERT( m_startYear <= fromYear && fromYear <= m_endYear );
   ASSERT( m_startYear <= toYear   && toYear   <= m_endYear );

   MapLayer *pLayer = m_pMap->GetLayer(0);

   if ( fromYear == toYear )
      return;

   bool isCategorical = pLayer->IsFieldCategorical( 0 );

   INT_PTR from, to;
   TYPE type = pLayer->GetFieldType( 0 );
   
   if ( fromYear < toYear )
      {
      m_pDeltaArray->GetIndexRangeFromYearRange( fromYear, toYear, from, to );

      for (INT_PTR i = from; i < to; i++)
         {
         DELTA& delta = m_pDeltaArray->GetAt(i);
         //ASSERT( ! delta.oldValue.Compare( delta.newValue ) );

         if ((delta.col == m_iduCol) || (m_overlayCol >= 0 && delta.col == m_overlayCol))
            {
            if (m_isDifference)
               {
               // has value changed since beginning?
               //if (delta.newValue.Compare(m_startDataArray[delta.cell]) == true)
               VData oldValue;
               pLayer->GetData(delta.cell, 0, oldValue);

               if ( delta.newValue.Compare(oldValue) == true)
                  pLayer->SetNoData(delta.cell, 0);
               else  // they are different, so populate coverage
                  {
                  bool populated = false;

                  if (isCategorical)
                     {
                     pLayer->SetData(delta.cell, 0, delta.newValue);
                     populated = true;
                     }
                  else if (::IsNumeric(type))
                     {
                     float newValue;
                     bool ok = delta.newValue.GetAsFloat(newValue);
                     if (ok)   // valid float?
                        {
                        float startValue;
                        ok = m_startDataArray[delta.cell].GetAsFloat(startValue);
                        if (ok)
                           {
                           pLayer->SetData(delta.cell, 0, (newValue - startValue));
                           populated = true;
                           }
                        }
                     }

                  if (!populated)
                     pLayer->SetData(delta.cell, 0, VData());
                  }
               }  // end of: isDifference
            else
               {
               // if old and new deltas are different, and delta column is the column for this map, then update map
               if (!delta.oldValue.Compare(delta.newValue))
                  {
                  VData value;
                  pLayer->GetData(delta.cell, 0, value);
                  ASSERT(value.Compare(delta.oldValue) == true);  //!!! JPB These and otehrs should works!!!
                  pLayer->SetData(delta.cell, 0, delta.newValue);
                  }
               }
            }
         }
      }
   else // fromYear > toYear - going backwards
      {
      m_pDeltaArray->GetIndexRangeFromYearRange( fromYear, toYear, from, to );

      for ( INT_PTR i=from; i>to; i-- )
         {
         DELTA &delta = m_pDeltaArray->GetAt( i );
         //ASSERT( ! delta.oldValue.Compare( delta.newValue ) );
         
         if ( m_isDifference && delta.col == m_iduCol )
            {
            if ( delta.oldValue.Compare( m_startDataArray[ delta.cell ] ) == true ) // old value same as start value?
               pLayer->SetNoData( delta.cell, 0 );
            else  // they are different, so populate coverage
               {
               bool populated = false;

               if ( isCategorical )
                  {
                  pLayer->SetData( delta.cell, 0, delta.oldValue );
                  populated = true;
                  }
               else if ( ::IsNumeric( type ) )
                  {
                  float oldValue;
                  bool ok = delta.oldValue.GetAsFloat( oldValue );
                  if ( ok )   // valid float?
                     {
                     float startValue;
                     ok = m_startDataArray[ delta.cell ].GetAsFloat( startValue );
                     if ( ok )
                        {
                        pLayer->SetData( delta.cell, 0, (oldValue-startValue) );
                        populated = true;
                        }
                     }
                  }
               
               if ( ! populated )
                  pLayer->SetData( delta.cell, 0, VData() );
               }
            }  // end of: isDifference
         else
            {
            if ( (! delta.oldValue.Compare(delta.newValue)) && ( delta.col == m_iduCol || (m_overlayCol >= 0 && delta.col == m_overlayCol ) ) )
               {
               VData value;
               pLayer->GetData( delta.cell, 0, value );
               ASSERT( value.Compare( delta.newValue ) == true );
               pLayer->SetData( delta.cell, 0, delta.oldValue );
               }
            }
         }
      }

   pLayer->ClassifyData(); 
   m_pMapList->Refresh();
   m_mapFrame.m_pMapWnd->Invalidate( FALSE );
   m_mapFrame.m_pMapWnd->UpdateWindow();
   }



// static functions
bool ResultsMapWnd::Replay( CWnd *pWnd,int flag )
   {
   ResultsMapWnd *pMapWnd = (ResultsMapWnd*)pWnd;
   return pMapWnd->_Replay( flag );
   }

bool ResultsMapWnd::SetYear( CWnd *pWnd,int oldYear, int newYear )
   {
   ResultsMapWnd *pMapWnd = (ResultsMapWnd*)pWnd;
   return pMapWnd->_SetYear( oldYear, newYear );
   }


// flag:
//  -1=reset to year 0
//

bool ResultsMapWnd::_Replay( int flag )
   {
   //ASSERT( m_yearSlider.GetRangeMin() <= year && year <= m_yearSlider.GetRangeMax() );

   if ( flag >= 0 )
      {
      if ( m_year == m_yearSlider.GetRangeMax() )
         return false;

      m_year++;
      ASSERT( m_year <= m_yearSlider.GetRangeMax() ); 

      m_yearSlider.SetPos( m_year );
      
      ShiftMap( m_year-1, m_year );

      CString str;
      str.Format( "Year: %d", m_year );

      m_yearEdit.SetWindowText( str );
      m_yearEdit.Invalidate();
      m_yearEdit.UpdateWindow();
      }
   else  // flag < 0
      {
      int firstYear = m_yearSlider.GetRangeMin();
      int oldYear   = m_year;

      if ( oldYear != firstYear )
         {
         m_year = firstYear;

         m_yearSlider.SetPos( firstYear );
         ShiftMap( oldYear, firstYear );

         CString str;
         str.Format( "Year: %d", m_year );

         m_yearEdit.SetWindowText( str );
         //m_yearEdit.Invalidate();
         //m_yearEdit.UpdateWindow();
         }
      }
   
   return true;
   }


void ResultsMapWnd::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
   {
   ActiveWnd::SetActiveWnd( &m_mapFrame );
   CWnd::OnActivate(nState, pWndOther, bMinimized);
   }


int ResultsMapWnd::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
   {
   ActiveWnd::SetActiveWnd( &m_mapFrame );
   return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
   }


void ResultsMapWnd::OnShowresults() 
   {
   CPropertySheet dlg( "Site Results", gpMain, 0 );
   PPPolicyResults resultsPage( m_mapFrame.m_currentPoly, m_run );

   dlg.AddPage( &resultsPage );
   resultsPage.m_cell = m_mapFrame.m_currentPoly;

   dlg.DoModal();
   }



void ResultsMapWnd::OnShowdeltas() 
   {
   gpCellLayer->ClearSelection();

   gpCellLayer->AddSelection( m_mapFrame.m_currentPoly );
   DeltaViewer dlg( gpMain, m_run, true );

   gpCellLayer->ClearSelection();

   dlg.DoModal();
   }


//void ResultsMapWnd::OnUpdateShowdeltas(CCmdUI* pCmdUI) 
//   {
//   pCmdUI->Enable( gpDoc->m_model.m_currentRun > 0 ? TRUE : FALSE );
//   }

