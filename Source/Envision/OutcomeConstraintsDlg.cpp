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
// OutcomeConstraintsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "OutcomeConstraintsDlg.h"
#include "afxdialogex.h"
#include "EnvDoc.h"
#include <NameDlg.h>


extern PolicyManager *gpPolicyManager;
extern CEnvDoc       *gpDoc;


// OutcomeConstraintsDlg dialog

IMPLEMENT_DYNAMIC(OutcomeConstraintsDlg, CDialogEx)

OutcomeConstraintsDlg::OutcomeConstraintsDlg(CWnd* pParent, MultiOutcomeInfo *pOutcome, EnvPolicy *pPolicy )
	: CDialogEx(OutcomeConstraintsDlg::IDD, pParent)
   , m_isDirtyGlobal( false )
   , m_isDirtyOutcome( false )
   , m_pPolicy( pPolicy )
   , m_pOutcome( NULL )
   , m_globalConstraintIndex( -1 )
   , m_outcomeConstraintIndex( -1 )
   , m_maxAnnualValueExpr(_T("0"))
   , m_initCostExpr(_T("0"))
   , m_maintenanceCostExpr(_T("0"))
   , m_durationExpr( _T("0"))
{ 
m_pOutcome = new MultiOutcomeInfo( *pOutcome );   
}


OutcomeConstraintsDlg::~OutcomeConstraintsDlg()
{
if ( m_pOutcome != NULL )
   delete m_pOutcome;
}


// OutcomeConstraintsDlg message handlers
void OutcomeConstraintsDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Control(pDX, IDC_GLOBALCONSTRAINTS, m_globalConstraints);
DDX_Control(pDX, IDC_POLICYCONSTRAINTS, m_outcomeConstraints);
DDX_Control(pDX, IDC_GCCOMBO, m_assocGC);
DDX_Text(pDX, IDC_INITIALCOST, m_initCostExpr);
DDX_Text(pDX, IDC_MAINTENANCECOST, m_maintenanceCostExpr);
DDX_Text(pDX, IDC_DURATION, m_durationExpr);
DDX_Text(pDX, IDC_MAXANNUALVALUE, m_maxAnnualValueExpr);
}


BEGIN_MESSAGE_MAP(OutcomeConstraintsDlg, CDialogEx)
   ON_BN_CLICKED(IDC_ADDGC, &OutcomeConstraintsDlg::OnBnClickedAddgc)
   ON_BN_CLICKED(IDC_DELETEGC, &OutcomeConstraintsDlg::OnBnClickedDeletegc)
   ON_BN_CLICKED(IDC_ADDPOLICYCONSTRAINT, &OutcomeConstraintsDlg::OnBnClickedAddOutcomeConstraint)
   ON_BN_CLICKED(IDC_DELETEPOLICYCONSTRAINT, &OutcomeConstraintsDlg::OnBnClickedDeleteOutcomeConstraint)
   ON_LBN_SELCHANGE(IDC_POLICYCONSTRAINTS, &OutcomeConstraintsDlg::OnLbnSelchangeOutcomeConstraints)
   ON_LBN_SELCHANGE(IDC_GLOBALCONSTRAINTS, &OutcomeConstraintsDlg::OnLbnSelchangeGlobalconstraints)
   ON_BN_CLICKED(IDC_RESOURCE_LIMIT, &OutcomeConstraintsDlg::OnBnClickedResourceLimit)
   ON_BN_CLICKED(IDC_MAX_AREA, &OutcomeConstraintsDlg::OnBnClickedResourceLimit)
   ON_BN_CLICKED(IDC_MAX_COUNT, &OutcomeConstraintsDlg::OnBnClickedResourceLimit)
   ON_BN_CLICKED(IDC_UNIT_AREA, &OutcomeConstraintsDlg::OnBnClickedUnitArea)
   ON_BN_CLICKED(IDC_ABSOLUTE, &OutcomeConstraintsDlg::OnBnClickedUnitArea)
   ON_CBN_SELCHANGE(IDC_GCCOMBO, &OutcomeConstraintsDlg::OnCbnSelchangeGccombo)
   ON_EN_CHANGE(IDC_MAXANNUALVALUE, &OutcomeConstraintsDlg::OnEnChangeMaxAnnualValue)
   ON_EN_CHANGE(IDC_INITIALCOST, &OutcomeConstraintsDlg::OnEnChangeInitCost)
   ON_EN_CHANGE(IDC_MAINTENANCECOST, &OutcomeConstraintsDlg::OnEnChangeMaintenanceCost)
   ON_EN_CHANGE(IDC_DURATION, &OutcomeConstraintsDlg::OnEnChangeDuration)
END_MESSAGE_MAP()


BOOL OutcomeConstraintsDlg::OnInitDialog() 
   {   
	CDialogEx::OnInitDialog();
   
   // make a local copy of the global constraints
   for ( int i=0; i < gpPolicyManager->GetGlobalConstraintCount(); i++ )
      {
      GlobalConstraint *pConstraint = gpPolicyManager->GetGlobalConstraint( i );
      m_constraintArray.Add( new GlobalConstraint( *pConstraint ) );
      }
  
   LoadOutcome();
	return TRUE;
   }


void OutcomeConstraintsDlg::LoadOutcome()
   {
   if ( m_pOutcome == NULL )
      return;

   // global constraints first
   this->m_globalConstraints.ResetContent();

   for ( int i=0; i < (int) m_constraintArray.GetSize(); i++ )
      {
      GlobalConstraint *pConstraint = m_constraintArray[ i ];
      m_globalConstraints.AddString( pConstraint->m_name );
      }

   if ( m_globalConstraintIndex >= 0  )
      {
      m_globalConstraints.SetCurSel( m_globalConstraintIndex );
      LoadGlobalConstraint();
      }
      
   // load policy-level constraints
   this->m_outcomeConstraints.ResetContent();

   for ( int i=0; i < m_pOutcome->GetConstraintCount(); i++ )
      m_outcomeConstraints.AddString( m_pOutcome->GetConstraint( i )->m_name );

   m_outcomeConstraintIndex = -1;
   if ( m_pOutcome->GetConstraintCount() > 0 )
      {
      m_outcomeConstraintIndex = 0;
      m_outcomeConstraints.SetCurSel( 0 );
      LoadOutcomeConstraint();
      }

   UpdateData( FALSE );  // loads the controls with updated data
   EnableControls();
   }


void OutcomeConstraintsDlg::LoadGlobalConstraint()
   {
   m_globalConstraintIndex = m_globalConstraints.GetCurSel();

   if ( m_globalConstraintIndex >= 0 )
      {
      GlobalConstraint *pConstraint = m_constraintArray[ m_globalConstraintIndex ];

      this->m_maxAnnualValueExpr = pConstraint->GetExpression();

      switch( pConstraint->m_type )
         {
         case CT_RESOURCE_LIMIT:
            CheckDlgButton( IDC_RESOURCE_LIMIT, TRUE );
            break;

        case CT_MAX_AREA:
            CheckDlgButton( IDC_MAX_AREA, TRUE );
            break;

        case CT_MAX_COUNT:
            CheckDlgButton( IDC_MAX_COUNT, TRUE );
            break;
         }      
      }

   UpdateData( 0 );  // write values to controls
   EnableControls();
   }


void OutcomeConstraintsDlg::LoadOutcomeConstraint()
   {
   m_outcomeConstraintIndex = m_outcomeConstraints.GetCurSel();

   if ( m_outcomeConstraintIndex >= 0 )
      {
      OutcomeConstraint *pConstraint = m_pOutcome->GetConstraint( m_outcomeConstraintIndex );

      this->m_initCostExpr = pConstraint->m_initialCostExpr;
      this->m_maintenanceCostExpr = pConstraint->m_maintenanceCostExpr;
      this->m_durationExpr = pConstraint->m_durationExpr;

      switch( pConstraint->m_basis )
         {
         case CB_UNIT_AREA:
            CheckDlgButton( IDC_UNIT_AREA, TRUE );
            break;

        case CB_ABSOLUTE:
            CheckDlgButton( IDC_ABSOLUTE, TRUE );
            break;

        default:
           ASSERT( 0 );
         }

      // global constraint
      m_assocGC.ResetContent();
      int assocIndex = -1;
      for ( int i=0; i < (int) m_constraintArray.GetSize(); i++ ) 
         {
         GlobalConstraint *pGC = m_constraintArray[ i ];
         m_assocGC.AddString( pGC->m_name );

         if ( m_outcomeConstraintIndex >= 0 && pGC == pConstraint->m_pGlobalConstraint )
            assocIndex = i;
         }
      
      m_assocGC.SetCurSel( assocIndex );
      }

   UpdateData( 0 );
   EnableControls();
   }


// OutcomeConstraintsDlg message handlers
void OutcomeConstraintsDlg::OnBnClickedAddgc()
   {
   NameDlg dlg( this );
   dlg.m_title = "Add New Global Constraint";
   dlg.m_name = "<Global Constraint>";
   if ( dlg.DoModal() == IDOK )
      {
      // add a new global constraint
      m_globalConstraintIndex = (int) m_constraintArray.Add( new GlobalConstraint( gpPolicyManager, dlg.m_name, CT_RESOURCE_LIMIT, 0 ) );
      m_globalConstraints.AddString( dlg.m_name );
      m_globalConstraints.SetCurSel( m_globalConstraintIndex );
      LoadGlobalConstraint();
 
      MakeDirtyGlobal();
      EnableControls();
      }
   }


void OutcomeConstraintsDlg::OnBnClickedDeletegc()
   {
   if ( AfxMessageBox( "OK to delete this constraint", MB_YESNO ) == IDOK )
      {
      m_constraintArray.RemoveAt( m_globalConstraintIndex );
      m_globalConstraints.DeleteString( m_globalConstraintIndex );
      m_globalConstraints.SetCurSel( --m_globalConstraintIndex );
      LoadGlobalConstraint();
      MakeDirtyGlobal();
      EnableControls();
      }
   }


void OutcomeConstraintsDlg::OnBnClickedAddOutcomeConstraint()
   {
   NameDlg dlg( this );
   dlg.m_title = "Add New Policy Constraint";
   dlg.m_name = "<Policy Constraint>";
   if ( dlg.DoModal() == IDOK )
      {
      // add a new global constraint
      OutcomeConstraint *pConstraint = new OutcomeConstraint(  m_pPolicy, dlg.m_name, CB_UNIT_AREA, "0", "0", 0, NULL );

      m_assocGC.SetCurSel( 0 );
      int gcIndex = 0;
      if ( m_globalConstraintIndex > 0 )
         gcIndex = m_globalConstraintIndex;

      GlobalConstraint *pGlobalConstraint = m_constraintArray[ gcIndex ];

      pConstraint->m_pGlobalConstraint = pGlobalConstraint;
      pConstraint->m_gcName = pGlobalConstraint->m_name;

      m_outcomeConstraintIndex = m_pOutcome->AddConstraint( pConstraint );
      
      m_outcomeConstraints.AddString( dlg.m_name );
      m_outcomeConstraints.SetCurSel( m_outcomeConstraintIndex );
      LoadOutcomeConstraint();
 
      MakeDirtyOutcome();
      EnableControls();
      }
   }


void OutcomeConstraintsDlg::OnBnClickedDeleteOutcomeConstraint()
   {
   if ( AfxMessageBox( "OK to delete this constraint", MB_YESNO ) == IDOK )
      {
      m_pOutcome->DeleteConstraint( m_outcomeConstraintIndex );
      m_outcomeConstraints.DeleteString( m_outcomeConstraintIndex );
      m_outcomeConstraints.SetCurSel( --m_outcomeConstraintIndex );
      LoadOutcomeConstraint();
      MakeDirtyOutcome();
      EnableControls();
      }
   }


void OutcomeConstraintsDlg::OnLbnSelchangeOutcomeConstraints()
   {
   LoadOutcomeConstraint();
   }


void OutcomeConstraintsDlg::OnLbnSelchangeGlobalconstraints()
   {
   LoadGlobalConstraint();
   }


void OutcomeConstraintsDlg::OnBnClickedResourceLimit()
   {
   if ( m_pOutcome == NULL )
      return;

   GlobalConstraint *pConstraint = m_constraintArray[ m_globalConstraintIndex ];

   if ( IsDlgButtonChecked( IDC_RESOURCE_LIMIT ) )
      pConstraint->m_type = CT_RESOURCE_LIMIT;
      
   else if ( IsDlgButtonChecked( IDC_MAX_AREA ) )
      pConstraint->m_type = CT_MAX_AREA;
   
   else if ( IsDlgButtonChecked( IDC_MAX_COUNT ) )
      pConstraint->m_type = CT_MAX_COUNT;
       
   MakeDirtyGlobal();
   }


void OutcomeConstraintsDlg::OnBnClickedUnitArea()
   {
   if ( m_pOutcome == NULL )
      return;

   OutcomeConstraint *pConstraint = m_pOutcome->GetConstraint( m_outcomeConstraintIndex );
      
   if ( IsDlgButtonChecked( IDC_UNIT_AREA ) )
      pConstraint->m_basis = CB_UNIT_AREA;
      
   else if ( IsDlgButtonChecked( IDC_ABSOLUTE ) )
      pConstraint->m_basis = CB_ABSOLUTE;

   MakeDirtyOutcome();
   }


void OutcomeConstraintsDlg::OnCbnSelchangeGccombo()
   {
   // no action required
   MakeDirtyOutcome();
   }


void OutcomeConstraintsDlg::OnEnChangeMaxAnnualValue()
   {
   // this applies to the current global constraint
   GlobalConstraint *pConstraint = m_constraintArray[ m_globalConstraintIndex ];

   GetDlgItemText( IDC_MAXANNUALVALUE, pConstraint->m_expression );

   MakeDirtyOutcome();
   }


void OutcomeConstraintsDlg::OnEnChangeInitCost()
   {
   // this applies to policy constraints
   OutcomeConstraint *pConstraint = m_pOutcome->GetConstraint( m_outcomeConstraintIndex );

   GetDlgItemText( IDC_INITIALCOST, pConstraint->m_initialCostExpr );
   
   MakeDirtyOutcome();
   }


void OutcomeConstraintsDlg::OnEnChangeMaintenanceCost()
   {
   // this applies to policy constraints
   OutcomeConstraint *pConstraint = m_pOutcome->GetConstraint( m_outcomeConstraintIndex );

   GetDlgItemText( IDC_MAINTENANCECOST, pConstraint->m_maintenanceCostExpr );
   
   MakeDirtyOutcome();
   }


void OutcomeConstraintsDlg::OnEnChangeDuration()
   {
   // this applies to policy constraints
   OutcomeConstraint *pConstraint = m_pOutcome->GetConstraint( m_outcomeConstraintIndex );

   GetDlgItemText( IDC_DURATION, pConstraint->m_durationExpr );
   MakeDirtyOutcome();
   }


void OutcomeConstraintsDlg::EnableControls()
   {
   BOOL editable = TRUE;

   m_globalConstraintIndex = m_globalConstraints.GetCurSel();
   m_outcomeConstraintIndex = m_outcomeConstraints.GetCurSel();

   BOOL globalEnabled = m_globalConstraintIndex >= 0 ? TRUE : FALSE;
   BOOL policyEnabled = m_outcomeConstraintIndex >= 0 ? TRUE : FALSE;
   BOOL resourceLimit = IsDlgButtonChecked( IDC_RESOURCE_LIMIT );

   GetDlgItem( IDC_ADDGC           )->EnableWindow( editable );
   GetDlgItem( IDC_DELETEGC        )->EnableWindow( editable && globalEnabled );
   GetDlgItem( IDC_MAXVALUE_LABEL  )->EnableWindow( editable && globalEnabled );
   GetDlgItem( IDC_MAXANNUALVALUE  )->EnableWindow( editable && globalEnabled );
   GetDlgItem( IDC_RESOURCE_LIMIT  )->EnableWindow( editable && globalEnabled );
   GetDlgItem( IDC_MAX_AREA        )->EnableWindow( editable && globalEnabled );
   GetDlgItem( IDC_MAX_COUNT       )->EnableWindow( editable && globalEnabled );
      
   GetDlgItem( IDC_ADDPOLICYCONSTRAINT   )->EnableWindow( editable && m_globalConstraints.GetCount() > 0 );
   GetDlgItem( IDC_DELETEPOLICYCONSTRAINT)->EnableWindow( editable && policyEnabled );
   GetDlgItem( IDC_GC_LABEL              )->EnableWindow( editable && policyEnabled );
   GetDlgItem( IDC_GCCOMBO               )->EnableWindow( editable && policyEnabled );
   GetDlgItem( IDC_INITCOST_LABEL        )->EnableWindow( editable && policyEnabled && resourceLimit );
   GetDlgItem( IDC_INITIALCOST           )->EnableWindow( editable && policyEnabled && resourceLimit );
   GetDlgItem( IDC_MAINTENANCECOST_LABEL )->EnableWindow( editable && policyEnabled && resourceLimit );
   GetDlgItem( IDC_MAINTENANCECOST       )->EnableWindow( editable && policyEnabled && resourceLimit );
   GetDlgItem( IDC_DURATION_LABEL        )->EnableWindow( editable && policyEnabled && resourceLimit );
   GetDlgItem( IDC_DURATION              )->EnableWindow( editable && policyEnabled && resourceLimit );
   GetDlgItem( IDC_YEAR_LABEL            )->EnableWindow( editable && policyEnabled && resourceLimit );   
   GetDlgItem( IDC_UNIT_AREA             )->EnableWindow( editable && policyEnabled );
   GetDlgItem( IDC_ABSOLUTE              )->EnableWindow( editable && policyEnabled );
   }


void OutcomeConstraintsDlg::OnOK()
   {
   if ( m_isDirtyGlobal )
      {
      // replace the policy managers global constraints with these ones
      gpPolicyManager->ReplaceGlobalConstraints( m_constraintArray );
         
      m_isDirtyGlobal = false; 
      
      gpDoc->SetChanged( CHANGED_POLICIES );
      }
   
   if ( m_isDirtyOutcome )
      {
      // replace the current outcome's constraints
      //OutcomeConstraint *pConstraint = m_pOutcome->GetConstraint( m_outcomeConstraintIndex );

      //pConstraint->m_duration = GetDlgItemInt( IDC_DURATION );
      }

   CDialogEx::OnOK();
   }
