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
// TrajectorySpaceWnd.cpp : implementation file
//

#include "stdafx.h"

#include "EnvWinLibs.h"
#include "Envision.h"
#include "TrajectorySpaceWnd.h"
#include <EnvModel.h>
#include <DataManager.h>
#include "MainFrm.h"
#include <colors.hpp>



// TrajectorySpaceWnd

IMPLEMENT_DYNAMIC(TrajectorySpaceWnd, CWnd)

/*
// TrajectorySpaceWnd message handlers

#include "EnvDoc.h"
#include <FDataObj.h>
#include <Actor.h>
*/

//extern CEnvDoc      *gpDoc;
//extern ActorManager *gpActorManager;
extern EnvModel     *gpModel;
extern CMainFrame   *gpMain;

// TrajectorySpaceWnd

TrajectorySpaceWnd::TrajectorySpaceWnd( int multiRun )
: m_multiRun( multiRun )
{
}

TrajectorySpaceWnd::~TrajectorySpaceWnd()
{ }


BEGIN_MESSAGE_MAP(TrajectorySpaceWnd, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_WM_HSCROLL()
   ON_CBN_SELCHANGE(TSW_LUCLASSCOMBO, OnComboChange)
   ON_CBN_SELCHANGE(TSW_EVALMODELCOMBO, OnComboChange)
   ON_CBN_SELCHANGE(TSW_RUNCOMBO, OnRunComboChange)
END_MESSAGE_MAP()



// TrajectorySpaceWnd message handlers

int TrajectorySpaceWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   ASSERT( gpModel->m_pDataManager != NULL );
   ASSERT( gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES ) != NULL );

   m_plot.CreateEx( WS_EX_CLIENTEDGE, NULL, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, CRect(0,0,0,0), this, TSW_PLOT );

   DataManager *pDM = gpModel->m_pDataManager;
   FDataObj *pMultiRunData = (FDataObj*) pDM->GetMultiRunDataObj( DT_MULTI_EVAL_SCORES ); // landscape goal scores trajectories
                                                                              // rows=runs, col for each evaluative model that reports results
   MULTIRUN_INFO &multiRunInfo = pDM->GetMultiRunInfo( m_multiRun );
   int firstRun = multiRunInfo.runFirst;

   FDataObj *pRunEvalScore = (FDataObj*) pDM->GetDataObj( DT_EVAL_SCORES, firstRun );
   m_totalYears = pRunEvalScore->GetRowCount()-1;    // assumes one row per year

   FDataObj *pLulcTrends = pDM->CalculateLulcTrends( 1, firstRun );
   
   if ( pLulcTrends != NULL )
      {
      // void SetAxisBounds( glReal xMin, glReal xMax, glReal yMin, glReal yMax, glReal zMin, glReal zMax );
      // Note: X-Axis = percent of landscape area in luclass
      //       Y-Axis = time
      //       Z-Axis = evalModel score
      glReal xMin = 0;
      glReal xMax = 50;

      glReal yMin = 0;
      glReal yMax = m_totalYears;

      glReal zMin = -3;
      glReal zMax = 3;

      m_plot.SetAxisBounds( xMin, xMax, yMin, yMax, zMin, zMax );
      m_plot.SetAxisBounds2( xMin, xMax, yMin, yMax, zMin, zMax );
      m_plot.SetAxisTicks( 10, 10, 3.0 );
      m_plot.SetAxisTicks2(10, 10, 3.0 );
      m_plot.SetAxesVisible( true );
      m_plot.SetAxesVisible2( true );
      m_plot.SetFrameType( P3D_FT_DEFAULT );

      PointCollection *pPoints = m_plot.GetPointCollection(0);
      pPoints->SetName( "Run 0" ); 
      pPoints->SetPointProperties( RED, 4, P3D_ST_ISOSAHEDRON );
      pPoints->SetTraj( true );
      pPoints->SetTrajProperties( RED, 4, P3D_LT_PIXEL );

      // PlotWnd3d has a singlepoint collection be default.  Add additional collections for each run
      int runs = multiRunInfo.runLast - firstRun + 1;
      for ( int i=0; i < runs-1; i++ )
         {
         CString label;
         label.Format( "Run %i", i+1 );
         pPoints = m_plot.AddDataPointCollection( label, false, false );
         pPoints->SetPointProperties( RED, 4, P3D_ST_ISOSAHEDRON );
         pPoints->SetTraj( true );
         pPoints->SetTrajProperties( RED, 4, P3D_LT_PIXEL );
         }

      m_year = 0;
      //m_totalYears = gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES )->GetRowCount()-1;

      // set up the ancillary controls
      m_yearSlider.Create( WS_VISIBLE | WS_CHILD | TBS_TOP | TBS_AUTOTICKS , CRect(0,0,0,0), this, TSW_YEARSLIDER );
      m_yearSlider.SetRange( 0, m_totalYears );
      m_yearSlider.SetTicFreq( 1 );
      m_yearSlider.SetPos( m_year );

      m_luClassCombo.Create( WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0,0,0,0), this, TSW_LUCLASSCOMBO ); 
      m_evalModelCombo.Create( WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0,0,0,0), this, TSW_EVALMODELCOMBO );
      m_runCombo.Create( WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0,0,0,0), this, TSW_RUNCOMBO );

      // for each evoland model that produces landscape metrics that are included inthe results, add it to the combo box
      int resultsCount = gpModel->GetResultsCount();
      for ( int i=0; i < resultsCount; i++ )
         {
         int modelIndex = gpModel->GetEvaluatorIndexFromResultsIndex( i );
         EnvEvaluator *pModel = gpModel->GetEvaluatorInfo( modelIndex );

         if ( pModel->m_use )
            m_evalModelCombo.AddString( pModel->m_name );
         }

      m_evalModelCombo.SetCurSel( 0 );

      // add the runs to the run combo
      for ( int i=0; i < runs; i++ )
         {
         CString text;
         text.Format( "%i", i );
         m_runCombo.AddString( text );
         }

      m_runCombo.SetCurSel( 0 );

      // add the LU classes to the luclass combo (get these from the data object
      int luClasses = pLulcTrends->GetColCount();
      for ( int i=1; i < luClasses; i++ )          // first col is "Years", so skip it.
         m_luClassCombo.AddString( pLulcTrends->GetLabel( i ) );

      m_luClassCombo.SetCurSel( 0 );

      // year edit control
      m_yearEdit.Create( WS_VISIBLE | ES_READONLY , CRect(0,0,0,0), this, TSW_YEAREDIT );
      CString str;
      str.Format( "Year: %d", m_year );
      m_yearEdit.SetWindowText( str );

      //RefreshData();
      PostMessage( WM_COMMAND, MAKEWPARAM( (WORD) TSW_LUCLASSCOMBO, (WORD) CBN_SELCHANGE ), (LPARAM)m_luClassCombo.m_hWnd );
      }

   return 0;
   }


/*----------------------------------------------------------
This routine is responsible for loading up the appropriate
points into the ScatterWnd.

The following dataobjects are needed to construct the plot:
   1) LULC Trends
   2) Evaluative score trajectories

The basic idea is to add a line (PointCollection) for each
run of the multirun

Note: X-Axis = percent of landscape area in luclass
      Y-Axis = time
      Z-Axis = evalModel score
------------------------------------------------------------*/

void TrajectorySpaceWnd::RefreshData()
   {
   CWaitCursor c;
   ASSERT( gpModel->m_pDataManager != NULL );
   
   DataManager *pDM = gpModel->m_pDataManager;
   FDataObj *pMultiRunData = (FDataObj*) pDM->GetMultiRunDataObj( DT_MULTI_EVAL_SCORES ); // landscape goal scores trajectories
                                                                              // rows=runs, col for each evaluative model that reports results
   MULTIRUN_INFO &multiRunInfo = pDM->GetMultiRunInfo( m_multiRun );
   int firstRun = multiRunInfo.runFirst;

   FDataObj *pRunEvalScore = (FDataObj*) pDM->GetDataObj( DT_EVAL_SCORES, firstRun );
   m_totalYears = pRunEvalScore->GetRowCount()-1;    // assumes one row per year
   
   ASSERT( IsWindow( m_plot.m_hWnd ) );
   m_plot.ClearData();

   gpMain->SetStatusMsg( "Starting Trajectory Calculations..." );

   // get current combo settings
   int evalModel = this->m_evalModelCombo.GetCurSel();
   int luIndex = this->m_luClassCombo.GetCurSel();
   
   // have all needed data, get variables to plot
   int runs = multiRunInfo.runLast - firstRun + 1;
   int run = firstRun;
   int i=0;

   int selRun = this->m_runCombo.GetCurSel();

   //FDataObj d( 3, m_totalYears );
   while ( run <= multiRunInfo.runLast )
      {
      PointCollection *pPoints = m_plot.GetPointCollection(i);

      if ( selRun == run )
         {
         pPoints->SetTrajColor( GREEN );
         pPoints->SetTrajSize( 8 );
         pPoints->SetPointColor( GREEN );
         pPoints->SetPointType( P3D_ST_SPHERE );
         pPoints->SetPointSize( 8 );
         }
      else
         {
         pPoints->SetTrajColor( RED );
         pPoints->SetTrajSize( 4 );
         pPoints->SetPointType( P3D_ST_SPHERE );
         pPoints->SetPointColor( RED );
         pPoints->SetPointSize( 4 );
         }

      CString msg;
      msg.Format( "Trajectory Calculations: Run %i of %i...", run+1, multiRunInfo.runLast+1 );
      gpMain->SetStatusMsg( msg );

      FDataObj *pLulcTrends = pDM->CalculateLulcTrends( 1, run );

      if ( pLulcTrends != NULL )
         {
         pRunEvalScore = (FDataObj*) pDM->GetDataObj( DT_EVAL_SCORES, run );

         ASSERT( pRunEvalScore != NULL );
         ASSERT( pLulcTrends != NULL );
         
         for ( int j=0; j < m_totalYears; j++ )
            {
            float evalScore     = pRunEvalScore->GetAsFloat( evalModel+1, j );   // evalModel+1 because "time" is the first column in the data object
            float luAreaPercent = pLulcTrends->GetAsFloat( luIndex+1, j );       // luIndex+1 because "time" is the first column in the data object

            // Note: X-Axis = percent of landscape area in luclass
            //       Y-Axis = time
            //       Z-Axis = evalModel score
            m_plot.AddDataPoint( (double)luAreaPercent, (double) j, (double)evalScore, i );
            }

         delete pLulcTrends;
         run++;
         i++;
         }
      }

   m_plot.Invalidate();
   gpMain->SetStatusMsg( "Finished Trajectory Calculations..." );
   }


void TrajectorySpaceWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
   
   static int gutter = 5;
   static int theight = 30;
   static int bheight = 40;
   static int editHeight = 20;
   static int editWidth  = 100;

   CSize sz( (cx - 4*gutter)/3, 400 );
   CPoint pt( gutter, gutter );

   m_luClassCombo.MoveWindow( CRect( pt, sz ) );

   pt.x += gutter + sz.cx;
   m_evalModelCombo.MoveWindow( CRect( pt, sz ) );

   pt.x += gutter + sz.cx;
   m_runCombo.MoveWindow( CRect( pt, sz ) );

   m_plot.MoveWindow( gutter, theight + 2*gutter, cx-2*gutter, cy - theight - bheight - 4*gutter );

   m_yearSlider.MoveWindow(                gutter,                cy - bheight - gutter, cx - 3*gutter - editWidth, bheight );
   m_yearEdit.MoveWindow( cx - gutter - editWidth, cy - gutter - (bheight+editHeight)/2,                 editWidth, editHeight );
   }


void TrajectorySpaceWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
   {
   int pos = m_yearSlider.GetPos();
   
   if ( m_year != pos )
      {
      m_year = pos;
      CString str;
      str.Format( "Year: %d", m_year );
      m_yearEdit.SetWindowText( str );



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


BOOL TrajectorySpaceWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   if ( !CWnd::PreCreateWindow(cs) )
      return FALSE;

   cs.style |= WS_CLIPCHILDREN;
	cs.lpszClass = AfxRegisterWndClass( CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW), NULL);

	return TRUE;
   }

void TrajectorySpaceWnd::OnComboChange()
   {
   // get current combo settings
   int evalModel = m_evalModelCombo.GetCurSel();
   CString modelName;
   m_evalModelCombo.GetLBText( evalModel, modelName );

   int luIndex   = m_luClassCombo.GetCurSel();
   CString luClassName;
   m_luClassCombo.GetLBText( luIndex, luClassName );

   m_plot.SetAxesLabels( luClassName, "Year", modelName );

   RefreshData();
   }

void TrajectorySpaceWnd::OnRunComboChange()
   {
   // have all needed data, get variables to plot
   PointCollectionArray *pColArray = m_plot.GetPointCollectionArray();
   int selRun = m_runCombo.GetCurSel();

   for ( int i=0; i < pColArray->GetSize(); i++ )
      {
      PointCollection *pPoints = m_plot.GetPointCollection(i);

      if ( selRun == i )
         {
         pPoints->SetTrajColor( GREEN );
         pPoints->SetTrajSize( 8 );
         pPoints->SetPointColor( GREEN );
         }
      else
         {
         pPoints->SetTrajColor( RED );
         pPoints->SetTrajSize( 4 );
         pPoints->SetPointColor( RED );
         }
      }

   m_plot.Invalidate();
   }



bool TrajectorySpaceWnd::Replay( CWnd *pWnd, int flag )
   {
   TrajectorySpaceWnd *pTrajectorySpaceWnd = (TrajectorySpaceWnd*)pWnd;
   return pTrajectorySpaceWnd->_Replay( flag );
   }


bool TrajectorySpaceWnd::_Replay( int year )  //  -1 = resetting, 0+ = playing
   {
   m_year = year;
   m_plot.m_endIndex = m_year;

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

   m_plot.Invalidate();
   return true;
   }