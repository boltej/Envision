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
// PolicyAppPlot.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include ".\policyappplot.h"

#include <EnvModel.h>
#include <DataManager.h>

extern EnvModel      *gpModel;
extern PolicyManager *gpPolicyManager;


// PolicyAppPlot dialog

IMPLEMENT_DYNAMIC(PolicyAppPlot, CDialog)
PolicyAppPlot::PolicyAppPlot( int run )
: CDialog(PolicyAppPlot::IDD, NULL),
   m_run( run ),
   m_pPlot( NULL ),
   m_pData( NULL )
   {
   }

PolicyAppPlot::~PolicyAppPlot()
   {
   if ( m_pPlot )
      delete m_pPlot;
   }

void PolicyAppPlot::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_PA_EFFECTIVENESS, m_targetMetagoalCombo);
   DDX_Control(pDX, IDC_PA_TOLERANCE, m_toleranceCombo);
   }


BEGIN_MESSAGE_MAP(PolicyAppPlot, CDialog)
   ON_WM_SIZE()
   ON_CBN_SELCHANGE(IDC_PA_EFFECTIVENESS, OnChangeEffectiveness)
   ON_CBN_SELCHANGE(IDC_PA_TOLERANCE, OnChangeTolerance)
END_MESSAGE_MAP()

BEGIN_EASYSIZE_MAP(PolicyAppPlot)
   EASYSIZE(IDC_PA_STATIC1,ES_BORDER,ES_BORDER,ES_KEEPSIZE,ES_KEEPSIZE,0)
   EASYSIZE(IDC_PA_STATIC2,ES_KEEPSIZE,ES_BORDER,ES_BORDER,ES_KEEPSIZE,0)
   EASYSIZE(IDC_PA_EFFECTIVENESS,IDC_PA_STATIC1,ES_BORDER,IDC_PA_STATIC2,ES_KEEPSIZE,0)
   EASYSIZE(IDC_PA_TOLERANCE,ES_KEEPSIZE,ES_BORDER,ES_BORDER,ES_KEEPSIZE,0)
   EASYSIZE(IDC_PA_PLOTFRAME,ES_BORDER,ES_BORDER,ES_BORDER,ES_BORDER,0)
END_EASYSIZE_MAP

// PolicyAppPlot message handlers

BOOL PolicyAppPlot::Create( const RECT &rect, CWnd* pParentWnd )
   {
   if ( !CDialog::Create( IDD, pParentWnd ) )
      return false;

   MoveWindow( &rect );

   ShowWindow( SW_SHOW );

   return true;
   }

BOOL PolicyAppPlot::OnInitDialog()
   {
   CDialog::OnInitDialog();

   INIT_EASYSIZE;

   DataObj *pDataObj = gpModel->m_pDataManager->CalculatePolicyApps( m_run );
   if ( pDataObj != NULL )
      {
      m_pPlot = new ScatterWnd( pDataObj, true );
      delete pDataObj;

      RECT rect;
      GetDlgItem( IDC_PA_PLOTFRAME )->GetWindowRect( &rect );
      ScreenToClient( &rect );
      m_pPlot->CreateEx( WS_EX_CLIENTEDGE, NULL, NULL, WS_VISIBLE | WS_CHILD, rect, this, 111 );

      m_targetMetagoalCombo.AddString( "No Filter" );
      int metagoalCount = gpModel->GetMetagoalCount();
      for ( int i=0; i < metagoalCount; i++ )
         {
         //int modelIndex = EnvModel::GetModelIndexFromMetagoalIndex( i );
         //m_targetMetagoalCombo.AddString( gpModel->GetEvaluatorInfo( modelIndex )->name );
         METAGOAL *pInfo = gpModel->GetMetagoalInfo( i );
         m_targetMetagoalCombo.AddString( pInfo->name );
         }

      m_targetMetagoalCombo.SetCurSel( 0 );

      for ( int i=-3; i <= 3; i++ )
         {
         CString str;
         str.Format( "%d", i );
         m_toleranceCombo.AddString( str );
         }

      m_toleranceCombo.SetCurSel( 3 );
      m_toleranceCombo.EnableWindow( false );
      }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void PolicyAppPlot::RefreshPlot()
   {
   int targetMetagoal = m_targetMetagoalCombo.GetCurSel() - 1;
   CString strTolerance;
   m_toleranceCombo.GetWindowText( strTolerance );

   int tolerance = atoi( strTolerance );
   ASSERT( -3 <= tolerance && tolerance <= 3 );

   int policyCount = gpPolicyManager->GetPolicyCount();

   ASSERT( policyCount == m_pPlot->GetLineCount() );

   for ( int i=0; i<policyCount; i++ )
      {
      m_pPlot->ShowLine( i, true );
      m_pPlot->ShowMarker( i, true );
      if ( targetMetagoal >= 0 )  // targetResult = -1 if no filter
         {
         Policy *pPolicy = gpPolicyManager->GetPolicy(i);
         float score = pPolicy->GetGoalScore( targetMetagoal );
         if ( score < (float)tolerance )
            {
            m_pPlot->ShowLine( i, false );
            m_pPlot->ShowMarker( i, false );
            }
         }
      }

   Invalidate();
   UpdateWindow();
   }

void PolicyAppPlot::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);

   UPDATE_EASYSIZE;

   if ( IsWindow( m_hWnd ) )
      {
      CWnd *pWnd = GetDlgItem( IDC_PA_PLOTFRAME );
      if ( pWnd && m_pPlot != NULL && IsWindow( m_pPlot->m_hWnd ) )
         {
         RECT rect;
         GetDlgItem( IDC_PA_PLOTFRAME )->GetWindowRect( &rect );
         ScreenToClient( &rect );
         m_pPlot->MoveWindow( &rect );
         }
      }
   }

void PolicyAppPlot::OnCancel()
   {
   DestroyWindow();

   //CDialog::OnCancel();
   }

void PolicyAppPlot::OnOK()
   {
   //DestroyWindow();

   //CDialog::OnOK();
   }

void PolicyAppPlot::OnChangeEffectiveness()
   {
   if ( m_targetMetagoalCombo.GetCurSel() == 0 )
      m_toleranceCombo.EnableWindow( false );
   else
      m_toleranceCombo.EnableWindow( true );


   RefreshPlot();
   }

void PolicyAppPlot::OnChangeTolerance()
   {
   RefreshPlot();
   }
