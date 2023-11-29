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
// Visualizer.cpp : implementation file
//
#include "stdafx.h"
#include "Envision.h"

#include "Visualizer.h"

#include "EnvDoc.h"
#include <EnvModel.h>
#include <DataManager.h>
#include "ActiveWnd.h"

class ActorManager;
class PolicyManager;

extern CEnvDoc* gpDoc;
extern EnvModel* gpModel;
extern MapLayer* gpCellLayer;
extern ActorManager* gpActorManager;
extern PolicyManager* gpPolicyManager;


IMPLEMENT_DYNAMIC(VisualizerWnd, CWnd)
// static functions

// flag =-1 means set to beginning


// VisualizerWnd

VisualizerWnd::VisualizerWnd(EnvVisualizer* pViz, int run)
   : m_pVizInfo(pViz)
   , m_envContext(gpCellLayer)
   , m_useage(0)
   , m_pRTProcessor(NULL)
   {
   ASSERT(pViz != NULL);
   m_envContext.currentYear = 0;
   m_envContext.yearOfRun = 0;
   m_envContext.runID = run;
   }


VisualizerWnd::~VisualizerWnd()
   {
   }


BEGIN_MESSAGE_MAP(VisualizerWnd, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_WM_ACTIVATE()
   ON_WM_MOUSEACTIVATE()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// VisualizerWnd message handlers

BOOL VisualizerWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   cs.lpszClass = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW,
      ::LoadCursor(NULL, IDC_SIZEWE), reinterpret_cast<HBRUSH>(COLOR_WINDOW), NULL);

   return CWnd::PreCreateWindow(cs);
   }


int VisualizerWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   DeltaArray* pDeltaArray = NULL;

   // set up the EnvContext
   // NOTE this need to be bitwise comparisons just in case the visualizer is used elsewhere
   // (such as VZT_RUNTIME)
   if (m_pVizInfo->m_type & VZT_POSTRUN_GRAPH
      || m_pVizInfo->m_type & VZT_POSTRUN_MAP
      || m_pVizInfo->m_type & VZT_POSTRUN_VIEW)
      {
      bool multirun = (gpModel->m_pDataManager->GetDeltaArrayCount() > 1);

      if (multirun)
         pDeltaArray = gpModel->m_pDataManager->GetDeltaArray(this->GetRun());
      else
         pDeltaArray = gpModel->m_pDataManager->GetDeltaArray();    // NOTE - THIS INLY MAKES SENSE FOR A PO
      }

   m_envContext.extra = (INT_PTR)this->m_hWnd;
   m_envContext.endYear = gpModel->m_endYear;
   m_envContext.startYear = gpModel->m_startYear;
   m_envContext.firstUnseenDelta = 0;
   m_envContext.lastUnseenDelta = 0;

   m_envContext.handle = m_pVizInfo->m_handle;
   m_envContext.id = m_pVizInfo->m_id;
   m_envContext.logMsgLevel = gpModel->m_logMsgLevel > 0 ? gpModel->m_logMsgLevel : 0xFFFF;
   m_envContext.pActorManager = gpActorManager;
   m_envContext.pDataObj = NULL;
   m_envContext.pDeltaArray = pDeltaArray;
   m_envContext.pEnvModel = gpModel;
   m_envContext.pEnvExtension = (EnvExtension*)m_pVizInfo;
   m_envContext.pLulcTree = &(gpModel->m_lulcTree);
   m_envContext.pMapLayer = gpCellLayer;
   m_envContext.pPolicyManager = gpPolicyManager;
   m_envContext.ptrAddDelta = NULL;  // not allowed for Visualizers
   m_envContext.showMessages = gpModel->m_showMessages;
   m_envContext.targetPolyArray = gpModel->m_targetPolyArray;
   m_envContext.targetPolyCount = gpModel->m_targetPolyCount;
   m_envContext.currentYear = gpModel->m_currentYear;
   m_envContext.yearOfRun = 0;
   //m_envContext.run = m_run;
   m_envContext.pWnd = this;
   //m_totalYears = 0;

   //if ( pDeltaArray != NULL )
   //   m_totalYears = pDeltaArray->GetMaxYear() + 1;  // + 1 to point to the end of the DeltaArray

   ActiveWnd::SetActiveWnd(this);
   return 0;
   }

BOOL VisualizerWnd::OnEraseBkgnd(CDC* pDC)
   {
   return TRUE;
   }

BOOL VisualizerWnd::InitWindow(EnvContext* pContext /*=NULL*/)
   {
   if (pContext == NULL)
      pContext = &m_envContext;
   else
      {
      pContext->pWnd = this;
      pContext->runID = -1;
      }

   return m_pVizInfo->InitWindow(pContext, GetSafeHwnd());
   }


BOOL VisualizerWnd::UpdateWindow(EnvContext* pContext /*=NULL*/)
   {
   if (pContext == NULL)
      pContext = &m_envContext;
   else
      pContext->pWnd = this;

   return m_pVizInfo->UpdateWindow(pContext, GetSafeHwnd());
   }

BOOL VisualizerWnd::PostrunReplay(int year)
   {
   if (m_pRTProcessor)
      return m_pRTProcessor->PostrunReplay(year);
   return false;
   }
BOOL VisualizerWnd::PostrunSetYear(int oldYear, int newYear)
   {
   if (m_pRTProcessor)
      return m_pRTProcessor->PostrunSetYear(oldYear, newYear);
   return false;
   }

void VisualizerWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
   UpdateWindow();      // this calls and updateWindow fn defeind for for the attached EnvVisualizer
   }


/*
 bool VisualizerWnd::_SetYear( int oldYear, int newYear )
   {
   if ( oldYear != newYear )
      {
      m_year = newYear;

      //Shift( oldYear, newYear );

      //CString str;
      //str.Format( "Year: %d", m_year );

      //m_yearEdit.SetWindowText( str );
      //m_yearEdit.Invalidate();
      //m_yearEdit.UpdateWindow();
      }

   return true;
   }
*/

/*
void VisualizerWnd::Shift( int fromYear, int toYear )
   {
   ASSERT( 0 <= fromYear && fromYear <= m_totalYears );
   ASSERT( 0 <= toYear   && toYear   <= m_totalYears );
   /*
   MapLayer *pLayer = m_pMap->GetLayer(0);

   if ( fromYear == toYear )
      return;

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

         if ( m_isDifference && delta.col == m_iduCol )
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
            if ( (! delta.oldValue.Compare(delta.newValue)) && ( delta.col == m_iduCol || (m_overlayCol >= 0 && delta.col == m_overlayCol ) ) )
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
   m_pMap->Invalidate( false );
   m_pMap->UpdateWindow();

   }
   */


void VisualizerWnd::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
   {
   ActiveWnd::SetActiveWnd(this);
   CWnd::OnActivate(nState, pWndOther, bMinimized);
   }


int VisualizerWnd::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
   {
   ActiveWnd::SetActiveWnd(this);
   return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
   }

