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
// GoalSpaceWnd.cpp : implementation file
//

#include "stdafx.h"
#include ".\goalspacewnd.h"

#include <EnvModel.h>
#include <DataManager.h>
#include "EnvDoc.h"
#include <FDataObj.h>
#include <Actor.h>

extern CEnvDoc      *gpDoc;
extern ActorManager *gpActorManager;
extern EnvModel     *gpModel;

// GoalSpaceWnd

GoalSpaceWnd::GoalSpaceWnd( int run )
: m_run( run )
{ }


GoalSpaceWnd::~GoalSpaceWnd()
{ }


BEGIN_MESSAGE_MAP(GoalSpaceWnd, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_WM_HSCROLL()
   ON_CBN_SELCHANGE(GSW_GOALCOMBO1, OnGoalChange)
   ON_CBN_SELCHANGE(GSW_GOALCOMBO2, OnGoalChange)
   ON_CBN_SELCHANGE(GSW_GOALCOMBO3, OnGoalChange)
END_MESSAGE_MAP()



// GoalSpaceWnd message handlers



int GoalSpaceWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   ASSERT( gpModel->m_pDataManager != NULL );
   ASSERT( gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES ) != NULL );

   m_plot.CreateEx( WS_EX_CLIENTEDGE, NULL, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, CRect(0,0,0,0), this, GSW_PLOT );

   m_plot.SetAxisBounds( -3.0, 3.0, -3.0, 3.0, -3.0, 3.0 );
   m_plot.SetAxisBounds2( -3.0, 3.0, -3.0, 3.0, -3.0, 3.0 );
   m_plot.SetAxisTicks( 3.0, 3.0, 3.0 );
   m_plot.SetAxisTicks2( 3.0, 3.0, 3.0 );
   m_plot.SetAxesVisible( true );
   m_plot.SetAxesVisible2( true );
   m_plot.SetFrameType( P3D_FT_DEFAULT );

   PointCollection *pCol = m_plot.GetPointCollection(0);
   pCol->SetName( "Landscape Metric" ); 
   pCol->SetPointProperties( RGB( 0, 255, 0 ), 9, P3D_ST_ISOSAHEDRON );

   pCol = m_plot.AddDataPointCollection( "Actor Values", true, false );
   pCol->SetPointProperties( RGB( 255, 0, 0 ), 4 );

   m_year = 0;
   m_totalYears = gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES )->GetRowCount()-1;

   m_yearSlider.Create( WS_VISIBLE | WS_CHILD | TBS_TOP | TBS_AUTOTICKS /*| WS_BORDER*/, CRect(0,0,0,0), this, GSW_YEARSLIDER );
   m_yearSlider.SetRange( 0, m_totalYears );
   m_yearSlider.SetTicFreq( 1 );
   m_yearSlider.SetPos( m_year );

   m_goalCombo0.Create( WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0,0,0,0), this, GSW_GOALCOMBO1 ); 
   m_goalCombo1.Create( WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0,0,0,0), this, GSW_GOALCOMBO2 );
   m_goalCombo2.Create( WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, CRect(0,0,0,0), this, GSW_GOALCOMBO3 );

   int resultsCount = gpModel->GetResultsCount();

   for ( int i=0; i < resultsCount; i++ )
      {
      int modelIndex = gpModel->GetEvaluatorIndexFromResultsIndex( i );
      EnvEvaluator *pModel = gpModel->GetEvaluatorInfo( modelIndex );

      if ( pModel->m_use )
         {
         m_goalCombo0.AddString( pModel->m_name );
         m_goalCombo1.AddString( pModel->m_name );
         m_goalCombo2.AddString( pModel->m_name );
         }
      }

   if ( resultsCount >= 3 )
      {
      m_goalCombo0.SetCurSel( 0 );
      m_goalCombo1.SetCurSel( 1 );
      m_goalCombo2.SetCurSel( 2 );
      }
   else
      {
      MessageBox( "There are less than 3 models.  The GoalSpaceWnd can not be created.", 0, MB_OK | MB_ICONERROR );
      return -1;
      }

   m_yearEdit.Create( WS_VISIBLE | ES_READONLY /*| WS_BORDER*/, CRect(0,0,0,0), this, GSW_YEAREDIT );
   CString str;
   str.Format( "Year: %d", m_year );
   m_yearEdit.SetWindowText( str );

   RefreshData();
   OnGoalChange();

   return 0;
   }

void GoalSpaceWnd::RefreshData()
   {
   ASSERT( gpModel->m_pDataManager != NULL );
   ASSERT( gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES ) != NULL );
   ASSERT( IsWindow( m_plot.m_hWnd ) );

   m_plot.ClearData();

   float x,y,z;
   int   g0, g1, g2;
   
   g0 = m_goalCombo0.GetCurSel();
   g1 = m_goalCombo1.GetCurSel();
   g2 = m_goalCombo2.GetCurSel();

   // Landscape Metrics
   gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES )->Get( g0 + 1, m_year, x );
   gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES )->Get( g1 + 1, m_year, y );
   gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES )->Get( g2 + 1, m_year, z );

   m_plot.AddDataPoint( (double)x, (double)y, (double)z );

   //// Actor Values
   //int goalCount  = gpModel->GetEvaluatorCount();
   //int groupCount = gpActorManager->GetActorGroupCount();

   //for ( int i=0; i<groupCount; i++ )
   //   {
   //   gpModel->m_pDataManager->GetDataObj( DT_ACTOR_WTS )->Get( i*goalCount+g0+1, m_year, x );
   //   gpModel->m_pDataManager->GetDataObj( DT_ACTOR_WTS )->Get( i*goalCount+g1+1, m_year, y );
   //   gpModel->m_pDataManager->GetDataObj( DT_ACTOR_WTS )->Get( i*goalCount+g2+1, m_year, z );
   //   m_plot.AddDataPoint( (double)x, (double)y, (double)z, 1 );
   //   }

   m_plot.Invalidate();
   //m_plot.Invalidate();
   }

void GoalSpaceWnd::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
   
   static int gutter = 5;
   static int theight = 30;
   static int bheight = 40;
   static int editHeight = 20;
   static int editWidth  = 100;

   CSize sz( (cx - 4*gutter)/3, 400 );
   CPoint pt( gutter, gutter );

   m_goalCombo0.MoveWindow( CRect( pt, sz ) );

   pt.x += gutter + sz.cx;
   m_goalCombo1.MoveWindow( CRect( pt, sz ) );

   pt.x += gutter + sz.cx;
   m_goalCombo2.MoveWindow( CRect( pt, sz ) );

   m_plot.MoveWindow( gutter, theight + 2*gutter, cx-2*gutter, cy - theight - bheight - 4*gutter );


   m_yearSlider.MoveWindow(                gutter,                cy - bheight - gutter, cx - 3*gutter - editWidth, bheight );
   m_yearEdit.MoveWindow( cx - gutter - editWidth, cy - gutter - (bheight+editHeight)/2,                 editWidth, editHeight );
   }

void GoalSpaceWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
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


BOOL GoalSpaceWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   if ( !CWnd::PreCreateWindow(cs) )
      return FALSE;

   cs.style |= WS_CLIPCHILDREN;
	cs.lpszClass = AfxRegisterWndClass( CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW), NULL);

	return TRUE;
   }

void GoalSpaceWnd::OnGoalChange()
   {
   int g0 = m_goalCombo0.GetCurSel();
   int g1 = m_goalCombo1.GetCurSel();
   int g2 = m_goalCombo2.GetCurSel();

   m_plot.SetAxesLabels( gpModel->GetEvaluatorInfo( g0 )->m_name, gpModel->GetEvaluatorInfo( g1 )->m_name, gpModel->GetEvaluatorInfo( g2 )->m_name );

   RefreshData();
   }

bool GoalSpaceWnd::Replay( CWnd *pWnd, int flag )
   {
   GoalSpaceWnd *pGoalSpaceWnd = (GoalSpaceWnd*)pWnd;
   return pGoalSpaceWnd->_Replay( flag );
   }

bool GoalSpaceWnd::_Replay( int flag )
   {
   //ASSERT( m_yearSlider.GetRangeMin() <= year && year <= m_yearSlider.GetRangeMax() );

   if ( flag >= 0)
      {
      if ( m_year == m_yearSlider.GetRangeMax() )
         return false;

      m_year++;
      ASSERT( m_year <= m_yearSlider.GetRangeMax() ); 

      m_yearSlider.SetPos( m_year );

      CString str;
      str.Format( "Year: %d", m_year );
      m_yearEdit.SetWindowText( str );

      RefreshData();
      }
   else
      {
      int firstYear = m_yearSlider.GetRangeMin();
      int oldYear   = m_year;

      if ( oldYear != firstYear )
         {
         m_year = firstYear;

         m_yearSlider.SetPos( firstYear );

         CString str;
         str.Format( "Year: %d", m_year );
         m_yearEdit.SetWindowText( str );
         
         RefreshData();
         }
      }
   
   return true;
   }