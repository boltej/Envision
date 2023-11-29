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
// LulcTransitionResultsWnd.cpp : implementation file
//

#include "stdafx.h"
#include ".\LulcTransitionResultsWnd.h"

#include <EnvModel.h>
#include <DataManager.h>
#include "EnvDoc.h"
#include <FDataObj.h>
#include <Actor.h>

#include <Plot3D/Plot3DWnd.h>
#include <Plot3D/BarPlot3DWnd.h>

extern CEnvDoc* gpDoc;
extern ActorManager* gpActorManager;
extern EnvModel* gpModel;

// LulcTransitionResultsWnd

LulcTransitionResultsWnd::LulcTransitionResultsWnd(int run)
   : m_run(run),
   m_pData(NULL),
   m_pWnd(NULL),
   m_changeTimeIntervalFlag(false)
   {
   }

LulcTransitionResultsWnd::~LulcTransitionResultsWnd()
   {
   if (m_pData != NULL)
      delete m_pData;

   if (m_pWnd != NULL)
      delete m_pWnd;
   }


BEGIN_MESSAGE_MAP(LulcTransitionResultsWnd, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_WM_HSCROLL()
   ON_CBN_SELCHANGE(LTPW_LULCLEVELCOMBO, OnLulcLevelChange)
   ON_CBN_SELCHANGE(LTPW_TIMEINTERVALCOMBO, OnTimeIntervalChange)
   ON_CBN_SELCHANGE(LTPW_PLOTLISTCOMBO, OnPlotListChange)
END_MESSAGE_MAP()


// LulcTransitionResultsWnd message handlers

BOOL LulcTransitionResultsWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   if (!CWnd::PreCreateWindow(cs))
      return FALSE;

   cs.style |= WS_CLIPCHILDREN;
   cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
      ::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW), NULL);

   return TRUE;
   }

int LulcTransitionResultsWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   ASSERT(gpModel->m_pDataManager != NULL);

   m_lulcLevelCombo.Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, LTPW_LULCLEVELCOMBO);
   m_timeIntervalCombo.Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, LTPW_TIMEINTERVALCOMBO);
   m_plotListCombo.Create(WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0, 0, 0, 0), this, LTPW_PLOTLISTCOMBO);

   m_lulcLevelCombo.AddString("Lulc A");
   m_lulcLevelCombo.AddString("Lulc B");
   m_lulcLevelCombo.AddString("Lulc C");
   m_lulcLevelCombo.SetCurSel(0);

   m_timeIntervalCombo.AddString("Entire Run");
   m_timeIntervalCombo.AddString("Yearly");
   m_timeIntervalCombo.SetCurSel(0);

   m_plotListCombo.AddString("Plot");
   m_plotListCombo.AddString("List");
   m_plotListCombo.SetCurSel(0);

   m_year = 0;
   m_totalYears = gpModel->m_pDataManager->GetDeltaArray()->GetMaxYear();

   m_yearSlider.Create(WS_VISIBLE | WS_CHILD | TBS_TOP | TBS_AUTOTICKS /*| WS_BORDER*/, CRect(0, 0, 0, 0), this, LTPW_YEARSLIDER);
   m_yearSlider.SetRange(0, m_totalYears);
   m_yearSlider.SetTicFreq(1);
   m_yearSlider.SetPos(m_year);
   m_yearSlider.EnableWindow(false);

   m_yearEdit.Create(WS_VISIBLE | ES_READONLY /*| WS_BORDER*/, CRect(0, 0, 0, 0), this, LTPW_YEAREDIT);
   CString str;
   str.Format("Year: %d", m_year);
   m_yearEdit.SetWindowText(str);

   OnPlotListChange();

   return 0;
   }

void LulcTransitionResultsWnd::RefreshData()
   {
   ASSERT(gpModel->m_pDataManager != NULL);
   ASSERT(IsWindow(m_pWnd->m_hWnd));
   int level = m_lulcLevelCombo.GetCurSel() + 1;
   int timeInterval = m_timeIntervalCombo.GetCurSel();

   if (m_pData != NULL)
      delete m_pData;

   //if ( timeInterval == 0 ) // Entire Run
   //   m_pData = gpModel->m_pDataManager->CalculateLulcTransTable( -1, level, -1 );
   //else
   //   m_pData = gpModel->m_pDataManager->CalculateLulcTransTable( -1, level, m_year );

   if (m_pData == NULL)
      return;

   if (m_plotListCombo.GetCurSel() == 0) // if viewing plot
      {
      BarPlot3DWnd* pPlot = (BarPlot3DWnd*)m_pWnd;
      pPlot->SetDataObj(m_pData, false, !m_changeTimeIntervalFlag /*true*/);
      }
   else // if viewing list
      {
      CListCtrl* pList = (CListCtrl*)m_pWnd;
      pList->DeleteAllItems();

      // Delete all of the columns.
      int columns = pList->GetHeaderCtrl()->GetItemCount();
      for (int i = 0; i < columns; i++)
         pList->DeleteColumn(0);

      pList->InsertColumn(0, "From", LVCFMT_LEFT, 200, 1);
      pList->InsertColumn(1, "To", LVCFMT_LEFT, 200, 2);
      pList->InsertColumn(2, "Times", LVCFMT_LEFT, 60, 3);
      //pList->InsertColumn( 3, "Total Area", LVCFMT_LEFT, 100, 3 );

      int count = m_pData->GetColCount();
      ASSERT(m_pData->GetRowCount() == count);
      float value;
      CString str;
      int index = 0;

      for (int row = 0; row < count; row++)
         for (int col = 0; col < count; col++)
            {
            m_pData->Get(col, row, value);
            if (value > 0.0f)
               {
               pList->InsertItem(index, m_pData->GetLabel(row));     // From
               pList->SetItemText(index, 1, m_pData->GetLabel(col));  // To

               str.Format("%g", value);
               pList->SetItemText(index, 2, str);

               index++;
               //str.Format( "%.6g", pTrans->totalArea );
               //pList->SetItemText(i, 3, str );
               }
            }

      }

   m_changeTimeIntervalFlag = false;
   //m_plot.Invalidate();
   }

void LulcTransitionResultsWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);

   static int gutter = 5;
   static int theight = 30;
   static int bheight = 40;
   static int editHeight = 20;
   static int editWidth = 100;

   CSize sz((cx - 4 * gutter) / 3, 400);
   CPoint pt(gutter, gutter);

   m_lulcLevelCombo.MoveWindow(CRect(pt, sz));

   pt.x += gutter + sz.cx;
   m_timeIntervalCombo.MoveWindow(CRect(pt, sz));

   pt.x += gutter + sz.cx;
   m_plotListCombo.MoveWindow(CRect(pt, sz));

   m_pWnd->MoveWindow(gutter, theight + 2 * gutter, cx - 2 * gutter, cy - theight - bheight - 4 * gutter);

   m_yearSlider.MoveWindow(gutter, cy - bheight - gutter, cx - 3 * gutter - editWidth, bheight);
   m_yearEdit.MoveWindow(cx - gutter - editWidth, cy - gutter - (bheight + editHeight) / 2, editWidth, editHeight);
   }

void LulcTransitionResultsWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
   {
   int pos = m_yearSlider.GetPos();

   if (m_year != pos)
      {
      m_year = pos;
      CString str;
      str.Format("Year: %d", m_year);
      m_yearEdit.SetWindowText(str);

      RefreshData();
      }

   //CString msg;
   //if ( nSBCode == TB_THUMBTRACK )
   //   {
   //   msg.Format(" nPos = %d", (int)pos );
   //   MessageBox(msg);
   //   }

   CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
   }

void LulcTransitionResultsWnd::OnLulcLevelChange()
   {
   RefreshData();
   }

void LulcTransitionResultsWnd::OnTimeIntervalChange()
   {
   //MessageBox( "Not Implemented Yet" );
   //m_timeIntervalCombo.SetCurSel( 0 );

   m_changeTimeIntervalFlag = true;

   if (m_timeIntervalCombo.GetCurSel() == 0) // Entire Run
      m_yearSlider.EnableWindow(false);
   else
      {
      m_yearSlider.EnableWindow(true);

      // 
      // Code below is a nice hack but the damnd linker dosnt like it!
      //

      //if ( m_plotListCombo.GetCurSel() == 0 ) // if viewing plot
      //   {
      //   // Get Max Height over the entire run
      //   float m = 0.0f;
      //   float test = 0.0f;
      //   int level = m_lulcLevelCombo.GetCurSel() + 1;
      //   FDataObj *pData;
      //   
      //   for ( int y=0; y<m_totalYears; y++ )
      //      {
      //      pData = gpModel->m_pDataManager->CalculateLulcTransTable( -1, level, y );

      //      for ( int i=0; i<pData->GetRowCount(); i++ )
      //         for ( int j=0; j<pData->GetColCount(); j++ )
      //            {
      //            pData->Get(j,i,test);
      //            if ( test>m )
      //               m = test;
      //            }

      //      delete pData;
      //      }

      //   BarPlot3DWnd *pPlot = (BarPlot3DWnd*)m_pWnd;
      //   pPlot->GetAxis( P3D_ZAXIS )->AutoScale( 0.0, glReal(m) );
      //   }
      }

   RefreshData();
   }

void LulcTransitionResultsWnd::OnPlotListChange()
   {
   CRect rect(0, 0, 0, 0);
   if (m_pWnd)
      m_pWnd->GetWindowRect(rect);
   ScreenToClient(rect);

   CWnd* pOldWnd = m_pWnd;

   if (m_plotListCombo.GetCurSel() == 0) // if switched to viewing plot
      {
      BarPlot3DWnd* pPlot = new BarPlot3DWnd();
      pPlot->SetAxesLabels2("From", "To", "");
      pPlot->SetAxesVisible2(true);
      pPlot->GetAxis(P3D_XAXIS)->SetLabelColor(RGB(255, 0, 0));
      pPlot->GetAxis(P3D_XAXIS2)->SetLabelColor(RGB(255, 0, 0));
      pPlot->GetAxis(P3D_XAXIS2)->ShowTicks(false);
      pPlot->GetAxis(P3D_XAXIS2)->ShowTickLabels(false);

      pPlot->GetAxis(P3D_YAXIS)->SetLabelColor(RGB(0, 0, 255));
      pPlot->GetAxis(P3D_YAXIS2)->SetLabelColor(RGB(0, 0, 255));
      pPlot->GetAxis(P3D_YAXIS2)->ShowTicks(false);
      pPlot->GetAxis(P3D_YAXIS2)->ShowTickLabels(false);

      pPlot->GetAxis(P3D_ZAXIS2)->ShowTicks(false);
      pPlot->GetAxis(P3D_ZAXIS2)->ShowTickLabels(false);

      pPlot->CreateEx(WS_EX_CLIENTEDGE, NULL, NULL, WS_CHILD /*| WS_BORDER*/, rect, this, LTPW_PLOT);

      m_pWnd = (CWnd*)pPlot;
      }
   else // if switched to List
      {
      CListCtrl* pList = new CListCtrl();
      pList->CreateEx(WS_EX_CLIENTEDGE, WS_VISIBLE | WS_CHILD | LVS_REPORT, CRect(0, 0, 0, 0), this, LTPW_PLOT);
      pList->MoveWindow(rect);
      m_pWnd = (CWnd*)pList;
      }

   RefreshData();

   if (pOldWnd)
      delete pOldWnd;

   m_pWnd->ShowWindow(SW_SHOW);
   }

bool LulcTransitionResultsWnd::Replay(CWnd* pWnd, int flag)
   {
   LulcTransitionResultsWnd* pResultsWnd = (LulcTransitionResultsWnd*)pWnd;
   return pResultsWnd->_Replay(flag);
   }

bool LulcTransitionResultsWnd::_Replay(int flag)
   {
   //ASSERT( m_yearSlider.GetRangeMin() <= year && year <= m_yearSlider.GetRangeMax() );

   if (flag >= 0)
      {
      if (m_year == m_yearSlider.GetRangeMax())
         return false;

      m_year++;
      ASSERT(m_year <= m_yearSlider.GetRangeMax());

      m_yearSlider.SetPos(m_year);

      CString str;
      str.Format("Year: %d", m_year);
      m_yearEdit.SetWindowText(str);

      RefreshData();
      }
   else
      {
      int firstYear = m_yearSlider.GetRangeMin();
      int oldYear = m_year;

      if (oldYear != firstYear)
         {
         m_year = firstYear;

         m_yearSlider.SetPos(firstYear);

         CString str;
         str.Format("Year: %d", m_year);
         m_yearEdit.SetWindowText(str);

         RefreshData();
         }
      }

   return true;
   }