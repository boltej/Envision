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
#include "stdafx.h"

#pragma hdrstop
#include "RtViewWnd.h"
#include "RtViewPanel.h"
#include <EnvModel.h>

#include <DeltaArray.h>
#include <MapWnd.h>
#include <Maplayer.h>
#include <Vdataobj.h>


extern MapLayer *gpCellLayer;
extern EnvModel *gpModel;


///////////////////////////////////////////////////////////////////////////////////////////
// RtViewWnd

IMPLEMENT_DYNAMIC(RtViewWnd, CWnd)

RtViewWnd::RtViewWnd( RtViewPage *pParent, int run, RTV_TYPE type, CRect &rect )
: m_rtvType( type )
, m_pPage( pParent )
, m_pDeltaArray( NULL )
, m_run( run )
, m_year( 0 )
, m_xLeft( rect.left )       // these are all measured as percent of the screen (e.g. "50")
, m_xRight( rect.right )
, m_yTop( rect.top )
, m_yBottom( rect.bottom )
   { 
   m_pPanel = (RtViewPanel*) pParent->GetParent();
   }

RtViewWnd::~RtViewWnd()
   {
   }


BEGIN_MESSAGE_MAP(RtViewWnd, CWnd)
      ON_WM_SIZE()
END_MESSAGE_MAP()


void RtViewWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
   }


bool RtViewWnd::SetRun( int run )      // called at the invocation of each EnvModel::Run()   
   {
   // This will set this RtViewWnd's run to the current run
   m_run  = run;

   ASSERT( this->m_pPanel != NULL );

   //RtViewPanel::m_runInfoArray is always empty (never modified).
   //Contact Vir or Patrick for more info.

//   RtViewRunInfo *pRunInfo = this->m_pPanel->GetRunInfo( run );

//   this->m_pDeltaArray = pRunInfo->m_pDeltaArray;

   return true;
   }



///////////////////////////////////////////////////////////////////////////////////////////
// RtVisualizer

IMPLEMENT_DYNAMIC(RtVisualizer, RtViewWnd)


BEGIN_MESSAGE_MAP(RtVisualizer, RtViewWnd)
      ON_WM_SIZE()
	  ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

//BOOL RtVisualizer::UpdateWindow( void )
//   {
//   if ( m_pVisualizerWnd && m_pVisualizerWnd->GetSafeHwnd() > 0 )
//      {
//      EnvContext &ctx = m_pVisualizerWnd->m_envContext;
//      ctx.currentYear = this->m_year;
//      ctx.yearOfRun = this->m_year - ctx.startYear;
//      m_pVisualizerWnd->UpdateWindow();
//      }
//
//   return TRUE;
//   }

BOOL RtVisualizer::OnEraseBkgnd(CDC* pDC)
   {
   return TRUE;
   }

void RtVisualizer::OnSize(UINT nType, int cx, int cy)
   {
   if ( m_pVisualizerWnd && m_pVisualizerWnd->GetSafeHwnd() > 0 )
       m_pVisualizerWnd->MoveWindow( 0, 0, cx, cy, TRUE );
   
   //CWnd::OnSize(nType, cx, cy);
   }




///////////////////////////////////////////////////////////////////////////////////////////
// RtMap

IMPLEMENT_DYNAMIC(RtMap, RtViewWnd)

BEGIN_MESSAGE_MAP(RtMap, RtViewWnd)
      ON_WM_SIZE()
END_MESSAGE_MAP()




// virtual methods - overrides of RtViewWnd

bool RtMap::InitWindow( void )
   {
   // Basic idea:
   //  Map a map window that live inside the RtViewWnd and populate the map with appropriate data
   //
   //  Notes: 1 ) m_layer
   if ( m_hWnd == 0 )   // must be a window already!
      return FALSE;

   int id = 100 + (int) this->m_pPage->m_viewWndArray.GetSize();

   CRect rect;
   m_pPage->GetClientRect( &rect );
   
   rect.left   = rect.right  * m_xLeft / 100;
   rect.right  = rect.right  * m_xRight / 100;
   rect.top    = rect.bottom * m_yTop / 100;
   rect.bottom = rect.bottom * m_yBottom / 100;

   m_mapFrame.Create( NULL, _T("MapWnd"), WS_CHILD | WS_VISIBLE, rect, this, id );
   m_pMap     = m_mapFrame.m_pMap;
   m_pMapList = m_mapFrame.m_pMapList;
   m_pMapList->m_pMapWnd = m_mapFrame.m_pMapWnd;

   MapLayer *pLayer = m_pMap->AddLayer( LT_POLYGON );      // add an empty layer
   pLayer->SetNoOutline();

   // make a copy of the source map polygons
   Map *pSourceMap = gpCellLayer->GetMapPtr();

   MapLayer *pSourceLayer = pSourceMap->GetLayer( m_layer );

   pLayer->ClonePolygons( pSourceLayer );      // warning: sets no data value to 0!

   //m_pDeltaArray->AddMapLayer( pLayer );      // assocoiate this resultMap with the approriate deltaArray
   // get the needed data
   int rows = pLayer->GetPolygonCount();

   int cols = 1;
   //m_overlayCol = gpCellLayer->m_overlayCol;
   //if ( m_overlayCol >= 0 )  // is there an overlay column?
   //   cols = 2;

   LPCTSTR fieldname = pSourceLayer->GetFieldLabel( m_col );
   VDataObj *pData = new VDataObj( cols, rows );
   pData->SetLabel( 0, fieldname );

   if ( cols > 1 )
      pData->SetLabel( 1, pSourceLayer->GetFieldLabel( m_overlayCol ) );  // hack for now

   VData data;

   // load data from original (IDU layer) map to the stored one
   for ( int row=0; row < rows; row++ )
      {
      bool ok = pSourceLayer->GetData( row, m_col, data );
      ASSERT( ok );

      ok = pData->Set( 0, row, data );
      ASSERT( ok );

      if ( cols > 1 )
         {
         ok = pSourceLayer->GetData( row, m_overlayCol, data );
         ASSERT( ok );
      
         ok = pData->Set( 1, row, data );
         ASSERT( ok );
         }
      }

   pLayer->m_pDbTable = new DbTable( pData );   // for creating a DbTable from a preexisting VDataObj
   pLayer->m_pData = pData;
   pLayer->SetNoDataValue( -99 );

   TYPE type = pSourceLayer->GetFieldType( m_col );
   int width, decimals;
   GetTypeParams( type, width, decimals );
   pLayer->SetFieldType( 0, type, width, decimals, false );
   
   // NOTE:  to provide a correct basis for generating the new map, we have to roll the copied data back
   //  to the beginning year.  Do that now.

   // get the current deltaArray for the main IDU coverage.  Note that these get created
   // in EnvModel::InitRun(), called at the beginning of EnvModel::Run()    .
   m_pDeltaArray = gpModel->m_pDeltaArray;

   if ( m_pDeltaArray != NULL )
      {
      m_pDeltaArray->AddMapLayer( pLayer );   // ??? CORRECT????

      // if the IDU coverage is past the beginning, roll back the  just-copied ResultMap layer
      INT_PTR firstUnapplied =  m_pDeltaArray->GetFirstUnapplied( pSourceLayer );
      if ( firstUnapplied > 0 )
         {
         for ( INT_PTR i = m_pDeltaArray->GetFirstUnapplied( pSourceLayer )-1; i >= 0; i-- )
            {
            DELTA &delta = m_pDeltaArray->GetAt( i );

            // if delta changed and the column matches, then unapply it
            if ( ((delta.oldValue.Compare(delta.newValue)) == false ) && (delta.col == m_col /*|| ( m_overlayCol >= 0  && delta.col == m_overlayCol )*/ ) )
               {
               VData value;
               pLayer->GetData( delta.cell, 0, value );
               ASSERT( value.Compare( delta.newValue ) == true );  // the "new value in the delta should match what's current in the resultMap
               pLayer->SetData( delta.cell, 0, delta.oldValue );   // roll the value back to the previous delta value
               }
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
   BinArray *pBinArray = pSourceLayer->GetBinArray( m_col, false );
   if ( pBinArray != NULL )
      pLayer->AddBinArray( new BinArray( *pBinArray ) );

   if( m_overlayCol >= 0 )
      {
      BinArray *pBinArray = pSourceLayer->GetBinArray( m_overlayCol, false );
      if ( pBinArray != NULL )
         pLayer->AddBinArray( new BinArray( *pBinArray ) );
      }
   
   Map::SetupBins( m_pMap, 0, 0 );  // ClassifyData() is called in here
   m_pMapList->Refresh();

   //ActiveWnd::SetActiveWnd( &m_mapFrame );
   return 1;
   }


//void RtMap::SetRun ( int run  ){} // { RtViewWnd::SetRun( run ); if ( m_pVisualizerWnd != NULL ) m_pVisualizerWnd->m_envContext.run = run; }
bool RtMap::SetYear( int year )
   {
   if ( m_year == year )
      return true;  // nothing needed

   int oldYear = m_year;
   m_year = year;


   if ( m_pPanel->UseDeltaArray() )
      {
      ShiftMap( oldYear, year );
      }
   else
      {
      }

   return true;
   }



// internal method only.  Callers should use SetYear() only
void RtMap::ShiftMap( int fromYear, int toYear )
   {
   int firstYear = m_pDeltaArray->GetFirstYear();
   int lastYear   = m_pDeltaArray->GetLastYear();

   if ( fromYear < firstYear )
      fromYear = firstYear;

   if ( fromYear > lastYear )
      fromYear = lastYear;

   if ( toYear < firstYear )
      toYear = firstYear;

   if ( toYear > lastYear )
      toYear = lastYear;

   if ( fromYear == toYear )
      return;   // nothing to do

   MapLayer *pLayer = m_pMap->GetLayer( m_layer );
   
   bool isCategorical = pLayer->IsFieldCategorical( 0 );

   INT_PTR from, to;
   TYPE type = pLayer->GetFieldType( 0 );
   
   if ( fromYear < toYear )
      {
      m_pDeltaArray->GetIndexRangeFromYearRange( fromYear, toYear, from, to );

      for ( INT_PTR i=from; i < to; i++ )
		   {
         DELTA &delta = m_pDeltaArray->GetAt(i);
         //ASSERT( ! delta.oldValue.Compare( delta.newValue ) );

         if ( m_isDifference && delta.col == m_col )
            {
            if ( delta.newValue.Compare( m_startDataArray[ delta.cell ] ) == true )
               pLayer->SetNoData( delta.cell, 0 );
            else  // they are different, so populate coverage
               {
               bool populated = false;

               if ( isCategorical )
                  {
                  pLayer->SetData( delta.cell, 0, delta.newValue );
                  populated = true;
                  }
               else if ( ::IsNumeric( type ) )
                  {
                  float newValue;
                  bool ok = delta.newValue.GetAsFloat( newValue );
                  if ( ok )   // valid float?
                     {
                     float startValue;
                     ok = m_startDataArray[ delta.cell ].GetAsFloat( startValue );
                     if ( ok )
                        {
                        pLayer->SetData( delta.cell, 0, (newValue-startValue) );
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
            // if old and new deltas are different, and delta column is the column for this map, then update map
            if ( (! delta.oldValue.Compare(delta.newValue)) && ( delta.col == m_col || (m_overlayCol >= 0 && delta.col == m_overlayCol ) ) )
               {
               VData value;
               pLayer->GetData( delta.cell, 0, value );
               ASSERT( value.Compare( delta.oldValue ) == true );  //!!! JPB These and otehrs should works!!!
               pLayer->SetData( delta.cell, 0, delta.newValue );
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
         
         if ( m_isDifference && delta.col == m_col )
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
            if ( (! delta.oldValue.Compare(delta.newValue)) && ( delta.col == m_col || (m_overlayCol >= 0 && delta.col == m_overlayCol ) ) )
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
   m_mapFrame.m_pMapWnd->Invalidate( false );
   m_mapFrame.m_pMapWnd->UpdateWindow();
   }







/*
BOOL RtMap::UpdateWindow( void )
   {
   if ( m_mapFrame.GetSafeHwnd() > 0 )
      {
      //EnvContext &ctx = m_pVisualizerWnd->m_envContext;
      //ctx.currentYear = this->m_year;
      //ctx.yearOfRun = this->m_year - ctx.startYear;
      m_mapFrame.UpdateWindow();
      }

   return TRUE;
   }

*/
void RtMap::OnSize(UINT nType, int cx, int cy)
   {
   if ( m_mapFrame.GetSafeHwnd() > 0 )
       m_mapFrame.MoveWindow( 0, 0, cx, cy, TRUE );
   
   //CWnd::OnSize(nType, cx, cy);
   }

