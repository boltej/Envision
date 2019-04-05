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
// SandboxDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include <EnvModel.h>
#include ".\sandboxdlg.h"

extern EnvModel *gpModel;

// SandboxDlg dialog

IMPLEMENT_DYNAMIC(SandboxDlg, CDialog)
SandboxDlg::SandboxDlg( Policy *pPolicy, CWnd* pParent /*=NULL*/ )
:  CDialog(SandboxDlg::IDD, pParent),
   m_pPolicy( pPolicy ),
   m_pSandbox( NULL ),
   m_yearsToRun( 20 ),
   m_percentCoverage( 100 ),
   m_runCompleted( false ),
   m_useMetagoalArray( NULL )
   {
   }

SandboxDlg::~SandboxDlg()
   {
   ASSERT( m_pSandbox == NULL );

   if ( m_useMetagoalArray != NULL )
      delete [] m_useMetagoalArray;
   }

void SandboxDlg::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_YEARSTORUNEDIT, m_yearsToRun);
   DDX_Text(pDX, IDC_PERCENTCOVERAGEEDIT, m_percentCoverage);
   DDX_Control(pDX, IDC_METAGOALS, m_metagoals);
   }


BEGIN_MESSAGE_MAP(SandboxDlg, CDialog)
END_MESSAGE_MAP()


// SandboxDlg message handlers

BOOL SandboxDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   ASSERT( m_pSandbox == NULL );

   SetWindowText( m_pPolicy->m_name );

   // add metagoals to the check list
   int count = gpModel->GetMetagoalCount();    // only worry about metagoals for policies
   for ( int i=0; i < count; i++ )
      {
      EnvEvaluator *pInfo = gpModel->GetMetagoalEvaluatorInfo( i );
      float effectiveness = m_pPolicy->GetGoalScore( i );
      CString str;
      str.Format( "%s (current value: %g)", (LPCTSTR) pInfo->m_name, effectiveness );
      m_metagoals.AddString( str );
      m_metagoals.SetCheck( i, 1 );
      }

   UpdateData( false );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

BOOL SandboxDlg::DestroyWindow()
   {
   return CDialog::DestroyWindow();
   }

void SandboxDlg::OnOK() // apply
   {
   if ( ! m_runCompleted )
      {
      UpdateData( TRUE );

      CWaitCursor c;
      
      if ( gpModel->m_currentRun > 0 ) // assume that this means the init run has been called for the models
         m_pSandbox = new Sandbox( gpModel, m_yearsToRun, float(m_percentCoverage)/100.0f, false );
      else
         m_pSandbox = new Sandbox( gpModel, m_yearsToRun, float(m_percentCoverage)/100.0f, true );
      
      int metagoalCount = gpModel->GetMetagoalCount();
      ASSERT( metagoalCount == m_metagoals.GetCount() );

      ASSERT( m_useMetagoalArray == NULL );
      m_useMetagoalArray = new bool[ metagoalCount ];
      for ( int i=0; i < metagoalCount; i++ )
         m_useMetagoalArray[ i ] = m_metagoals.GetCheck( i ) ? true : false;

      m_pSandbox->CalculateGoalScores( m_pPolicy, m_useMetagoalArray );

      // update labels in checklist box
      m_metagoals.ResetContent();
      for ( int i=0; i < metagoalCount; i++ )
         {
         EnvEvaluator *pInfo = gpModel->GetMetagoalEvaluatorInfo( i );
         float effectiveness = m_pPolicy->GetGoalScore( i );
         CString str;
         str.Format( "%s (current value: %g)", (LPCTSTR) pInfo->m_name, effectiveness );
         m_metagoals.AddString( str );
         m_metagoals.SetCheck( i, m_useMetagoalArray[ i ] ? 1 : 0 );
         }

      SetDlgItemText( IDOK, "&Apply" );
      MessageBox( "New effectiveness scores computed - select <Apply> to keep these scores", MB_OK );
      m_runCompleted = true;

      delete m_pSandbox;
      m_pSandbox = NULL;
      }
   else  // runCompleted = true
      {
      CDialog::OnOK();
      }
   }

void SandboxDlg::OnCancel()
   {
   CDialog::OnCancel();
   }
