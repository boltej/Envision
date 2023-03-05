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
// OutcomeEditor.cpp : implementation file
//

#include "stdafx.h"
#include "OutcomeEditor.h"
#include <EnvModel.h>
#include <EnvPolicy.h>

#include "AddResultDlg.h"
#include "OutcomeConstraintsDlg.h"
#include <maplayer.h>

extern MapLayer *gpCellLayer;

// OutcomeEditor dialog

IMPLEMENT_DYNAMIC(OutcomeEditor, CDialog)

OutcomeEditor::OutcomeEditor( EnvPolicy *pPolicy, CWnd* pParent /*=NULL*/)
	: CDialog(OutcomeEditor::IDD, pParent)
   , m_pPolicy( pPolicy )
   , m_outcome(_T(""))
   , m_probability(0)
   , m_expire(FALSE)
   , m_persist(FALSE)
   , m_delay(FALSE)
   , m_expirePeriod(0)
   , m_persistPeriod(0)
   , m_delayPeriod(0)
   , m_attr(_T(""))
   { }

OutcomeEditor::~OutcomeEditor()
{ }

void OutcomeEditor::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_RESULTS, m_results);
   DDX_Control(pDX, IDC_OUTCOME, m_editOutcome);
   DDX_Text(pDX, IDC_OUTCOME, m_outcome);
   DDX_Text(pDX, IDC_PROBABILITY, m_probability);
   //DDV_MinMaxInt(pDX, m_probability, 0, 100);
   DDX_Check(pDX, IDC_EXPIRE, m_expire);
   DDX_Check(pDX, IDC_PERSIST, m_persist);
   DDX_Check(pDX, IDC_DELAY, m_delay);
   DDX_Text(pDX, IDC_EXPIREPERIOD, m_expirePeriod);
   DDV_MinMaxInt(pDX, m_expirePeriod, 0, 500);
   DDX_Text(pDX, IDC_PERSISTPERIOD, m_persistPeriod);
   DDV_MinMaxInt(pDX, m_persistPeriod, 0, 500);
   DDX_Text(pDX, IDC_DELAYPERIOD, m_delayPeriod);
   DDV_MinMaxInt(pDX, m_delayPeriod, 0, 500);
   DDX_Text(pDX, IDC_ATTR, m_attr);
   DDX_Control(pDX, IDC_OUTCOME, m_editOutcome);
   }

BEGIN_MESSAGE_MAP(OutcomeEditor, CDialog)
   ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
   ON_BN_CLICKED(IDC_DELETE, OnBnClickedDelete)
   ON_EN_CHANGE(IDC_OUTCOME, OnEnChangeOutcome)
   ON_BN_CLICKED(IDOK, &OutcomeEditor::OnBnClickedOk)
   ON_LBN_SELCHANGE(IDC_RESULTS, &OutcomeEditor::OnLbnSelchangeResults)
   ON_EN_CHANGE(IDC_PROBABILITY, &OutcomeEditor::OnEnChangeProbability)
   ON_BN_CLICKED(IDC_DELAY, &OutcomeEditor::OnBnClickedDelay)
   ON_BN_CLICKED(IDC_EXPIRE, &OutcomeEditor::OnBnClickedExpire)
   ON_BN_CLICKED(IDC_PERSIST, &OutcomeEditor::OnBnClickedPersist)
   ON_EN_CHANGE(IDC_EXPIREPERIOD, &OutcomeEditor::OnEnChangeExpireperiod)
   ON_EN_CHANGE(IDC_ATTR, &OutcomeEditor::OnEnChangeAttr)
   ON_EN_CHANGE(IDC_PERSISTPERIOD, &OutcomeEditor::OnEnChangePersistperiod)
   ON_EN_CHANGE(IDC_DELAYPERIOD, &OutcomeEditor::OnEnChangeDelayperiod)
   ON_BN_CLICKED(IDC_CONSTRAINTS, &OutcomeEditor::OnBnClickedConstraints)
END_MESSAGE_MAP()


// OutcomeEditor message handlers

BOOL OutcomeEditor::OnInitDialog()
   {
   CDialog::OnInitDialog();

   LoadResults();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void OutcomeEditor::LoadResults()
   {
   m_results.ResetContent();

   // get outcomes string
   m_editOutcome.GetWindowText( m_outcome );
   m_outcome.Trim();

   if ( m_outcome.IsEmpty() )
      {
      AddResultDlg dlg;

      if ( dlg.DoModal() == IDOK )
         m_outcome = dlg.m_result;
      else
         {
         EndDialog( IDCANCEL );
         return;
         }
      }
   
   MultiOutcomeArray moa;
   bool ok = EnvPolicy::ParseOutcomes( m_pPolicy, m_outcome, moa );
   int count = 0;
   m_probability = -1;

   if ( ok && moa.GetSize() == 1 )
      {
      MultiOutcomeInfo &moi = moa.GetMultiOutcome( 0 );

      count = moi.GetOutcomeCount();

      for ( int i=0; i < count; i++ )
         {
         OutcomeInfo &outcome = moi.GetOutcome( i );

         CString outcomeStr;
         outcome.ToString( outcomeStr, false );

         m_results.AddString ( outcomeStr );
         }

      m_probability = moi.GetProbability();
      }

   EnableControls();

   if ( count > 0 )
      {
      m_results.SetCurSel( 0 );
      LoadResult();
      }

   UpdateData( 0 );
  }


void OutcomeEditor::OnBnClickedAdd()
   {
   AddResultDlg dlg;

   if ( dlg.DoModal() == IDOK )
      {
      int index = m_results.AddString( dlg.m_result );
      m_results.SetCurSel( index );

      LoadResult();
      RefreshOutcome();
      }
   }


void OutcomeEditor::OnBnClickedDelete()
   {
   int index = m_results.GetCurSel();

   if ( index < 0 )
      return;

   m_results.DeleteString( index );

   if ( m_results.GetCount() == 0 )
      m_results.SetCurSel( -1 );
   else
      m_results.SetCurSel( 0 );

   LoadResult();
   RefreshOutcome();
   }


void OutcomeEditor::RefreshOutcome()
   {
   m_outcome = "";

   int count = m_results.GetCount();

   for ( int i=0; i < count; i++ )
      {
      CString result;
      m_results.GetText( i, result );
      m_outcome += result;

      if ( i < count-1 )
         m_outcome += " and ";
      }

   // add probability if there is any text
   CString prob;
   GetDlgItem( IDC_PROBABILITY )->GetWindowText( prob );

   if ( prob.GetLength() > 0 )
      {
      m_outcome += ":";
      m_outcome += prob;
      }

   UpdateData( 0 );
   }


void OutcomeEditor::OnLbnSelchangeResults()
   {
   LoadResult();
   }


void OutcomeEditor::LoadResult()
   {
   m_isDirty = false;

   // Example Lulc_C=66    Conserv=1[e:50,0]

   int index = m_results.GetCurSel();

   if ( index < 0 )
      return;

   CString result;
   m_results.GetText( index, result );

   if ( result.IsEmpty() )
      return;

   MultiOutcomeArray moa;
   bool ok = EnvPolicy::ParseOutcomes( m_pPolicy, result, moa );

   if ( !ok )
      {
      Report::WarningMsg( "Syntax error encountered while parsing result", MB_OK );
      return;
      }

   ASSERT( moa.GetSize() == 1 );
   MultiOutcomeInfo &moi = moa.GetMultiOutcome( 0 );

   ASSERT ( moi.GetOutcomeCount() == 1 );
   OutcomeInfo &outcomeInfo = moi.GetOutcome( 0 );

   m_expire = m_persist = m_delay = FALSE;

   if ( outcomeInfo.m_pDirective )
      {
      switch( outcomeInfo.m_pDirective->type )
         {
         case ODT_EXPIRE:
            m_expire = TRUE;
            m_expirePeriod = outcomeInfo.m_pDirective->period;
            m_attr = outcomeInfo.m_pDirective->outcome.GetAsString();
            break;
            
         case ODT_PERSIST:
            m_persist = TRUE;
            m_persistPeriod = outcomeInfo.m_pDirective->period;
            break;
            
         case ODT_DELAY:
            m_delay = TRUE;
            m_delay = outcomeInfo.m_pDirective->period;
            break;            
         }
      }


/*
   // start parsing
   char *directive = strchr( result, '[' );

   m_expire = m_persist = m_delay = FALSE;
   char *end = strchr( result, ':' );

   if ( directive )
      {
      // parse directive
      directive++;
      while ( *directive == ' ' ) directive++;   // trim leading whitespace

      switch( *directive )
         {
         case 'e':
         case 'E':
            {
            m_expire = TRUE;
            char *p= strchr( directive, ':' );

            if ( p == NULL )
               break;

            p++;
            m_expirePeriod = atoi( p );

            p = strchr( p, ',' );

            if ( p == NULL )
               break;

            p++;
            end = strchr( p, ']' );

            if ( end == NULL )
               break;

            *end = NULL;
            m_attr = p;
            end = strchr( end+1, ':' );
            }
            break;

         case 'p':
         case 'P':
            {
            m_persist = TRUE;
            char *p= strchr( directive, ':' );

            if( p == NULL )
               break;

            p++;
            m_persistPeriod = atoi( p );
            end = strchr( p, ':' );
            }
            break;

         case 'd':
         case 'D':
            {
            m_delay = TRUE;
            char *p= strchr( directive, ':' );

            if ( p == NULL )
               break;

            p++;
            m_delayPeriod = atoi( p );
            end = strchr( p, ':' );
            }
            break;
         }
      }  // end of: if ( directive ) */

    // no probability?
    //if ( end == NULL )
    //  m_probability = 100;
    //else
    //   m_probability = atoi( end+1 );

   UpdateData( 0 );
   EnableControls();
   }


void OutcomeEditor::EnableControls()
   {
   GetDlgItem( IDC_EXPIRE )->EnableWindow( ( m_delay || m_persist ) ? FALSE : TRUE );
   GetDlgItem( IDC_L1EXPIRE )->EnableWindow( m_expire );
   GetDlgItem( IDC_L2EXPIRE )->EnableWindow( m_expire );
   GetDlgItem( IDC_EXPIREPERIOD )->EnableWindow( m_expire );
   GetDlgItem( IDC_ATTR )->EnableWindow( m_expire );

   GetDlgItem( IDC_DELAY )->EnableWindow( ( m_expire || m_persist ) ? FALSE : TRUE );
   GetDlgItem( IDC_L1DELAY )->EnableWindow( m_delay );
   GetDlgItem( IDC_DELAYPERIOD )->EnableWindow( m_delay );

   GetDlgItem( IDC_PERSIST )->EnableWindow( ( m_delay || m_expire ) ? FALSE : TRUE );
   GetDlgItem( IDC_L1PERSIST )->EnableWindow( m_persist );
   GetDlgItem( IDC_L2PERSIST )->EnableWindow( m_persist );
   GetDlgItem( IDC_PERSISTPERIOD )->EnableWindow( m_persist );
   }


void OutcomeEditor::RefreshResult()
   {
   UpdateData( 1 );

   int index = m_results.GetCurSel();

   if ( index < 0 )
      return;

   char result[ 1024 ];
   m_results.GetText( index, result );

   char *directive = strchr( result, '[' );

   if ( directive )
      *directive = NULL;

   // have base query.  if directive defined, add it
   if ( m_expire )
      {
      char dir[ 64 ];
      sprintf_s( dir, 64, "[e:%i,%s]", m_expirePeriod, (LPCTSTR) m_attr );
      lstrcat( result, dir );
      }
   else if ( m_persist )
      {
      char dir[ 64 ];
      sprintf_s( dir, 64, "[p:%i]", m_persistPeriod );
      lstrcat( result, dir );
      }
   else if ( m_delay )
      {
      char dir[ 64 ];
      sprintf_s( dir, 64, "[d:%i]", m_delayPeriod );
      lstrcat( result, dir );
      }

   m_results.InsertString( index, result );
   m_results.DeleteString( index+1 );
   m_results.SetCurSel( index );

   RefreshOutcome();
   }


void OutcomeEditor::OnBnClickedOk()
   {
   m_editOutcome.GetWindowText( m_outcome );

   MultiOutcomeArray moa;
   bool ok = EnvPolicy::ParseOutcomes( m_pPolicy, m_outcome, moa );

   if ( ! ok )
      Report::ErrorMsg( "Syntax error encountered parsing outcome string.  Please correct before saving.", MB_OK );
   else
      OnOK();
   }

void OutcomeEditor::OnBnClickedExpire()
   {
   if ( m_results.m_hWnd )
      {
      MakeDirty();
      RefreshResult();
      EnableControls();
      }
   }

void OutcomeEditor::OnBnClickedPersist()
   {
  if ( m_results.m_hWnd )
      {
      MakeDirty();
      RefreshResult();
      EnableControls();
      }
   }

void OutcomeEditor::OnBnClickedDelay()
   {
   if ( m_results.m_hWnd )
      {
      MakeDirty();
      RefreshResult();
      EnableControls();
      }
   }


void OutcomeEditor::OnEnChangeOutcome()
   {
   if ( m_results.m_hWnd )
      {
      MakeDirty();
      LoadResults();
      //RefreshResult();
      }
   }


void OutcomeEditor::OnEnChangeProbability()
   {
   if ( m_results.m_hWnd )
      {
      MakeDirty();
      RefreshResult();
      }
   }


void OutcomeEditor::OnEnChangeExpireperiod()
   {
   if ( m_results.m_hWnd )
      {
      MakeDirty();
      RefreshResult();
      }
   }

void OutcomeEditor::OnEnChangeAttr()
   {
   if ( m_results.m_hWnd )
      {
      MakeDirty();
      RefreshResult();
      }
   }

void OutcomeEditor::OnEnChangePersistperiod()
   {
   if ( m_results.m_hWnd )
      {
      MakeDirty();
      RefreshResult();
      }
   }


void OutcomeEditor::OnEnChangeDelayperiod()
   {
   if ( m_results.m_hWnd )
      {
      MakeDirty();
      RefreshResult();
      }
   }


void OutcomeEditor::OnBnClickedConstraints()
   {
   MultiOutcomeInfo *pOutcome = NULL;

   OutcomeConstraintsDlg dlg( this, pOutcome, this->m_pPolicy );
   dlg.DoModal();
   }
