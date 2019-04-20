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
// MultirunHistogramPlotWnd.cpp : implementation file
//

#include "stdafx.h"
#pragma hdrstop

#include "MultirunHistogramPlotWnd.h"
#include "ResultsWnd.h"
#include <EnvModel.h>
#include <DataManager.h>
#include <fdataobj.h>
#include <vector>

extern EnvModel *gpModel;

const int binCount = 12;

// MultirunHistogramPlotWnd

IMPLEMENT_DYNAMIC(MultirunHistogramPlotWnd, CWnd)

MultirunHistogramPlotWnd::MultirunHistogramPlotWnd( int multirun )
: m_year( 0 ),
  m_multirun( multirun ),
  m_totalYears( 0 ),
  m_showLine( true ),
  m_plot()
   { }


MultirunHistogramPlotWnd::~MultirunHistogramPlotWnd()
{
}


BEGIN_MESSAGE_MAP(MultirunHistogramPlotWnd, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_WM_HSCROLL()
   ON_CBN_SELCHANGE(HPW_RESULTCOMBO, OnResultChange)
END_MESSAGE_MAP()


int MultirunHistogramPlotWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_plot.CreateEx( WS_EX_CLIENTEDGE, NULL, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, CRect(0,0,0,0), this, HPW_PLOT );

   m_year = 0;
   m_totalYears = gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES )->GetRowCount()-1;

   m_yearSlider.Create( WS_VISIBLE | WS_CHILD | TBS_TOP | TBS_AUTOTICKS /*| WS_BORDER*/, CRect(0,0,0,0), this, HPW_YEARSLIDER );
   m_yearSlider.SetRange( 0, m_totalYears );
   m_yearSlider.SetTicFreq( 1 );
   m_yearSlider.SetPos( m_year );

   m_resultCombo.Create( WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0,0,0,0), this, HPW_RESULTCOMBO ); 

   m_yearEdit.Create( WS_VISIBLE | ES_READONLY /*| WS_BORDER*/, CRect(0,0,0,0), this, HPW_YEAREDIT );
   CString str;
   str.Format( "Year: %d", m_year );
   m_yearEdit.SetWindowText( str );

   // set up plot dataobj structure
   int resultsCount = gpModel->GetResultsCount();
   m_resultCombo.AddString( "--Show All--" );

   // create a dataobj for the histogram
   FDataObj *pData = new FDataObj( resultsCount+1, binCount, 0.0f, U_UNDEFINED);
   pData->SetLabel( 0, "Bin" );
   for ( int i=0; i < resultsCount; i++ )
      {
      int modelIndex = gpModel->GetEvaluatorIndexFromResultsIndex( i );
      pData->SetLabel( i+1, gpModel->GetEvaluatorInfo( modelIndex )->m_name );
      m_resultCombo.AddString( gpModel->GetEvaluatorInfo( modelIndex )->m_name );
      }

   const float vMin = -3;
   const float vMax = +3;
   for ( int i=0; i < binCount; i++ )
      {
      float mid = float( vMin + ((i+0.5)*(vMax-vMin)/binCount) );
      pData->Set( 0, i, mid );
      }

   m_plot.SetDataObj( pData, true );
   delete pData;   

   m_resultCombo.SetCurSel( 0 );
   m_plot.SetBarPlotStyle( BPS_SINGLE, 0 );

   RefreshData();
   OnResultChange();

   m_plot.RefreshPlotArea();
   return 0;
   }


void MultirunHistogramPlotWnd::RefreshData()
   {
   // based on the current year, get the new frequency distributions

   int resultsCount = gpModel->GetResultsCount();

   MULTIRUN_INFO &mi = gpModel->m_pDataManager->GetMultiRunInfo( m_multirun );
   int firstRun = mi.runFirst;
   int lastRun  = mi.runLast;

   FDataObj *pPlotData = m_plot.pLocDataObj;
   ASSERT( pPlotData != NULL );
   ASSERT( pPlotData->GetColCount() == resultsCount+1 );
   ASSERT( pPlotData->GetRowCount() == binCount );

   // zero out data object columns
   for ( int i=1; i <= resultsCount; i++ )
      for ( int j=0; j < binCount; j++ )
         pPlotData->Set( i, j, 0.0f );

   // compute frequencies by inerating though DataObjects acroos the runs in this multirun
   const float vMin = -3;
   const float vMax = +3;
   int year = m_year;
   if( year < 0 )
      year = 0;

   for ( int run=firstRun; run <= lastRun; run++ )
      {
      FDataObj *pScoreData = (FDataObj*) gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES, run );
      ASSERT( pScoreData != NULL );

      // iterate through models (eval scores)
      for ( int result=0; result < resultsCount; result++ )
         {
         // rows=year, col for each model that reports results (first col is year)
         float score = pScoreData->GetAsFloat( result+1, year );

         // figure out which bin it belongs in
         int bin = int( ( score - vMin ) * binCount / ( vMax - vMin ));
         if ( bin < 0 ) bin = 0;
         if ( bin > binCount-1 ) bin = binCount-1;

         // update the frequency;
         pPlotData->Add( result+1, bin, 1 );
         }
      }

   return;
   }


void MultirunHistogramPlotWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
   
   static int gutter = 5;
   static int theight = 30;
   static int bheight = 40;
   static int editHeight = 20;
   static int editWidth  = 100;

   CSize sz( (cx - 4*gutter)/3, 400 );
   CPoint pt( gutter, gutter );

   m_resultCombo.MoveWindow( CRect( pt, sz ) );

   pt.x += gutter + sz.cx;
//   m_goalCombo1.MoveWindow( CRect( pt, sz ) );

   pt.x += gutter + sz.cx;
//   m_goalCombo2.MoveWindow( CRect( pt, sz ) );

   m_plot.MoveWindow( gutter, theight + 2*gutter, cx-2*gutter, cy - theight - bheight - 4*gutter );

   m_yearSlider.MoveWindow(                gutter,                cy - bheight - gutter, cx - 3*gutter - editWidth, bheight );
   m_yearEdit.MoveWindow( cx - gutter - editWidth, cy - gutter - (bheight+editHeight)/2,                 editWidth, editHeight );
   }


void MultirunHistogramPlotWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
   {
   int pos = m_yearSlider.GetPos();
   
   if ( m_year != pos )
      {
      m_year = pos;
      CString str;
      str.Format( "Year: %d", m_year );
      m_yearEdit.SetWindowText( str );
      }

   ((ResultsWnd*) GetParent())->SetCurrentYear( m_year );

   CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
   }


BOOL MultirunHistogramPlotWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   if ( !CWnd::PreCreateWindow(cs) )
      return FALSE;

   cs.style |= WS_CLIPCHILDREN;
	cs.lpszClass = AfxRegisterWndClass( CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW), NULL);

	return TRUE;
   }


void MultirunHistogramPlotWnd::OnResultChange()
   {
   int result = m_resultCombo.GetCurSel();

   if ( result == 0 )
      m_plot.SetBarPlotStyle( BPS_STACKED );
   else
      m_plot.SetBarPlotStyle( BPS_SINGLE, result-1 );

   m_plot.RefreshPlotArea();
   }



// MultirunHistogramPlotWnd message handlers

void MultirunHistogramPlotWnd::ComputeFrequencies( std::vector<float> xVector, int binCount )
   {
   THistogram< float, double > histogram( binCount );
   histogram.Compute( xVector );       	// computing the histogram

   std::vector< double > centers = histogram.GetCenterContainers();
   std::vector< UINT   > freqs   = histogram.GetHistogram();  //GetNormalizedHistogram();

   FDataObj dataObj( 2, binCount, 0.0f, U_UNDEFINED);

   for ( int i=0; i < binCount; i++ )
      {
      dataObj.Set( 0, i, (float) centers[ i ] );
      dataObj.Set( 1, i, (float) freqs[ i ] );
      }

   // add data to plot
   m_plot.SetXCol( &dataObj, 0 );
   m_plot.AppendYCol( &dataObj, 1 );
   m_plot.MakeDataLocal();

   if ( m_showLine )
      {

      }
   }


bool MultirunHistogramPlotWnd::Replay( CWnd *pWnd, int flag )
   {
   MultirunHistogramPlotWnd *pPlotWnd = (MultirunHistogramPlotWnd*)pWnd;
   return pPlotWnd->_Replay( flag );
   }


bool MultirunHistogramPlotWnd::_Replay( int year )  //  -1 = resetting, 0+ = playing
   {
   m_year = year;

   if ( year >= 0 )
      {
      if ( m_year == m_yearSlider.GetRangeMax() )
         return false;

      m_yearSlider.SetPos( m_year );

      CString str;
      str.Format( "Year: %d", m_year+1 );
      m_yearEdit.SetWindowText( str );
      }
   else
      {
      int firstYear = m_yearSlider.GetRangeMin();
      m_yearSlider.SetPos( firstYear );

      CString str;
      str.Format( "Year: %d", m_year+1 );
      m_yearEdit.SetWindowText( str );
      }

   RefreshData();
   m_plot.RefreshPlotArea();
 
   return true;
   }
