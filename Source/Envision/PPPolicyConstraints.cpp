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
// PPPolicyConstraints.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "PPPolicyConstraints.h"
#include "afxdialogex.h"
#include "PolEditor.h"
#include "GlobalConstraintsDlg.h"
#include <NameDlg.h>


extern PolicyManager *gpPolicyManager;

// PPPolicyConstraints dialog

IMPLEMENT_DYNAMIC(PPPolicyConstraints, CPropertyPage)

PPPolicyConstraints::PPPolicyConstraints( PolEditor *pPolEditor, Policy *&pPolicy )
: CTabPageSSL(PPPolicyConstraints::IDD)
, m_pPolicy( pPolicy )
, m_pParent( pPolEditor )
, m_policyConstraintIndex( -1 )

, m_initCostExpr(_T("0"))
, m_maintenanceCostExpr(_T("0"))
, m_durationExpr( "0" )
   { }

PPPolicyConstraints::~PPPolicyConstraints()
{ }

void PPPolicyConstraints::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
DDX_Control(pDX, IDC_POLICYCONSTRAINTS, m_policyConstraints);
DDX_Control(pDX, IDC_GCCOMBO, m_assocGC);
DDX_Text(pDX, IDC_LOOKUPTABLE, m_lookupExpr);
DDX_Text(pDX, IDC_INITIALCOST, m_initCostExpr);
DDX_Text(pDX, IDC_MAINTENANCECOST, m_maintenanceCostExpr);
DDX_Text(pDX, IDC_DURATION, m_durationExpr);
}


BEGIN_MESSAGE_MAP(PPPolicyConstraints, CTabPageSSL)
   ON_BN_CLICKED(IDC_ADDPOLICYCONSTRAINT, &PPPolicyConstraints::OnBnClickedAddpolicyconstraint)
   ON_BN_CLICKED(IDC_DELETEPOLICYCONSTRAINT, &PPPolicyConstraints::OnBnClickedDeletepolicyconstraint)
   ON_LBN_SELCHANGE(IDC_POLICYCONSTRAINTS, &PPPolicyConstraints::OnLbnSelchangePolicyconstraints)
   ON_BN_CLICKED(IDC_UNIT_AREA, &PPPolicyConstraints::OnBnClickedUnitArea)
   ON_BN_CLICKED(IDC_ABSOLUTE, &PPPolicyConstraints::OnBnClickedUnitArea)
   ON_CBN_SELCHANGE(IDC_GCCOMBO, &PPPolicyConstraints::OnCbnSelchangeGccombo)
   ON_EN_CHANGE(IDC_INITIALCOST, &PPPolicyConstraints::OnEnChangeInitCost)
   ON_EN_CHANGE(IDC_MAINTENANCECOST, &PPPolicyConstraints::OnEnChangeMaintenanceCost)
   ON_EN_CHANGE(IDC_DURATION, &PPPolicyConstraints::OnEnChangeDuration)
   ON_EN_CHANGE(IDC_LOOKUPTABLE, &PPPolicyConstraints::OnEnChangeLookupTable)
   ON_BN_CLICKED(IDC_ADDGLOBALCONSTRAINT, &PPPolicyConstraints::OnBnClickedAddglobalconstraint)
END_MESSAGE_MAP()


BOOL PPPolicyConstraints::OnInitDialog() 
   {   
	CTabPageSSL::OnInitDialog();

   LoadGCCombo();
	return TRUE;
   }


void PPPolicyConstraints::LoadGCCombo( void )
   {
   m_assocGC.ResetContent();

   for ( int i=0; i < (int) m_pParent->m_constraintArray.GetSize(); i++ )
      m_assocGC.AddString( m_pParent->m_constraintArray[ i ]->m_name );

   m_assocGC.SetCurSel( -1 );
   }


void PPPolicyConstraints::MakeDirtyPolicy( void )
   {
   m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
   }


void PPPolicyConstraints::LoadPolicy()
   {
   if ( m_pPolicy == NULL )
      return;

   // reset everything
   this->m_policyConstraints.ResetContent();

   m_lookupExpr.Empty();
   m_initCostExpr.Empty();
   m_maintenanceCostExpr.Empty();
   m_durationExpr = "0";

   // load any existing constraints
   for ( int i=0; i < m_pPolicy->GetConstraintCount(); i++ )
      m_policyConstraints.AddString( m_pPolicy->GetConstraint( i )->m_name );

   m_policyConstraintIndex = -1;
   if ( m_pPolicy->GetConstraintCount() > 0 )
      {
      m_policyConstraintIndex = 0;
      m_policyConstraints.SetCurSel( 0 );
      LoadPolicyConstraint();
      }

   UpdateData( FALSE );  // loads the controls with updated data
   EnableControls();
   }

void PPPolicyConstraints::LoadPolicyConstraint()
   {
   m_policyConstraintIndex = m_policyConstraints.GetCurSel();

   if ( m_policyConstraintIndex >= 0 )
      {
      PolicyConstraint *pConstraint = m_pPolicy->GetConstraint( m_policyConstraintIndex );

      this->m_lookupExpr   = pConstraint->m_lookupStr;
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
      for ( int i=0; i < (int) m_pParent->m_constraintArray.GetSize(); i++ ) 
         {
         GlobalConstraint *pGC = m_pParent->m_constraintArray[ i ];
         m_assocGC.AddString( pGC->m_name );

         if ( m_policyConstraintIndex >= 0 && pGC == pConstraint->m_pGlobalConstraint )
            assocIndex = i;
         }
      
      m_assocGC.SetCurSel( assocIndex );
      }

   UpdateData( FALSE );  // update controls based on member variable values
   EnableControls();
   }


bool PPPolicyConstraints::StorePolicyChanges()
   {

   UpdateData( TRUE );  // save and validate member variables
   return true;
   }

void PPPolicyConstraints::OnBnClickedAddpolicyconstraint()
   {
   NameDlg dlg( this );
   dlg.m_title = "Add New Policy Constraint";
   dlg.m_name = "<Policy Constraint>";
   if ( dlg.DoModal() == IDOK )
      {
      // add a new policy constraint
      PolicyConstraint *pConstraint = new PolicyConstraint( this->m_pPolicy, dlg.m_name, CB_UNIT_AREA, "0", "0", 0, NULL );

      m_assocGC.SetCurSel( 0 );

      if ( m_pParent->m_constraintArray.GetSize() > 0 )
         {
         GlobalConstraint *pGlobalConstraint = m_pParent->m_constraintArray[ 0 ];

         pConstraint->m_pGlobalConstraint = pGlobalConstraint;
         pConstraint->m_gcName = pGlobalConstraint->m_name;
         }
      else
         {
         pConstraint->m_pGlobalConstraint = NULL;
         pConstraint->m_gcName.Empty();
         }

      m_policyConstraintIndex = m_pPolicy->AddConstraint( pConstraint );
      
      m_policyConstraints.AddString( dlg.m_name );
      m_policyConstraints.SetCurSel( m_policyConstraintIndex );
      LoadPolicyConstraint();
 
      m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
      EnableControls();
      }
   }


void PPPolicyConstraints::OnBnClickedDeletepolicyconstraint()
   {
   if ( AfxMessageBox( "OK to delete this constraint", MB_YESNO ) == IDOK )
      {
      m_pPolicy->DeleteConstraint( m_policyConstraintIndex );
      m_policyConstraints.DeleteString( m_policyConstraintIndex );
      m_policyConstraints.SetCurSel( --m_policyConstraintIndex );
      LoadPolicyConstraint();
      m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
      EnableControls();
      }
   }


void PPPolicyConstraints::OnLbnSelchangePolicyconstraints()
   {
   LoadPolicyConstraint();
   }


void PPPolicyConstraints::OnBnClickedUnitArea()
   {
   if ( m_pPolicy == NULL )
      return;

   PolicyConstraint *pConstraint = m_pPolicy->GetConstraint( m_policyConstraintIndex );
      
   if ( IsDlgButtonChecked( IDC_UNIT_AREA ) )
      pConstraint->m_basis = CB_UNIT_AREA;
      
   else if ( IsDlgButtonChecked( IDC_ABSOLUTE ) )
      pConstraint->m_basis = CB_ABSOLUTE;

   m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
   }


void PPPolicyConstraints::OnCbnSelchangeGccombo()
   {
   // update the current constraint to point to the new global constraint
   PolicyConstraint *pConstraint = m_pPolicy->GetConstraint( m_policyConstraintIndex );

   int gcIndex = m_assocGC.GetCurSel();

   if ( gcIndex >= 0 )
      pConstraint->m_pGlobalConstraint = m_pParent->m_constraintArray[ gcIndex ];
   else
      pConstraint->m_pGlobalConstraint = NULL;

   m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
   }


void PPPolicyConstraints::OnEnChangeInitCost()
   {
   // this applies to policy constraints
   PolicyConstraint *pConstraint = m_pPolicy->GetConstraint( m_policyConstraintIndex );

   GetDlgItemText( IDC_INITIALCOST, pConstraint->m_initialCostExpr );
   
   m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
   }


void PPPolicyConstraints::OnEnChangeMaintenanceCost()
   {
   // this applies to policy constraints
   PolicyConstraint *pConstraint = m_pPolicy->GetConstraint( m_policyConstraintIndex );

   GetDlgItemText( IDC_MAINTENANCECOST, pConstraint->m_maintenanceCostExpr );
   
   m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
   }


void PPPolicyConstraints::OnEnChangeDuration()
   {
   // this applies to policy constraints
   PolicyConstraint *pConstraint = m_pPolicy->GetConstraint( m_policyConstraintIndex );

   GetDlgItemText( IDC_DURATION, pConstraint->m_durationExpr );
   
   m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
   }


void PPPolicyConstraints::OnEnChangeLookupTable()
   {
   // this applies to policy constraints
   PolicyConstraint *pConstraint = m_pPolicy->GetConstraint( m_policyConstraintIndex );

   GetDlgItemText( IDC_LOOKUPTABLE, pConstraint->m_lookupStr );
   
   m_pParent->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
   }


void PPPolicyConstraints::EnableControls()
   {
   BOOL editable = m_pParent->IsEditable() ? TRUE : FALSE;

   //m_globalConstraintIndex = m_globalConstraints.GetCurSel();
   m_policyConstraintIndex = m_policyConstraints.GetCurSel();

   //BOOL globalEnabled = m_globalConstraintIndex >= 0 ? TRUE : FALSE;
   BOOL policyEnabled = m_policyConstraintIndex >= 0 ? TRUE : FALSE;
   BOOL resourceLimit = FALSE;

   if ( policyEnabled )
      {
      PolicyConstraint *pConstraint = m_pPolicy->GetConstraint( m_policyConstraintIndex );

      GlobalConstraint *pGC = pConstraint->m_pGlobalConstraint;
      if ( pGC != NULL && pGC->m_type == CT_RESOURCE_LIMIT )
         resourceLimit = TRUE;
      }
   
   //GetDlgItem( IDC_ADDGC           )->EnableWindow( editable );
   //GetDlgItem( IDC_DELETEGC        )->EnableWindow( editable && globalEnabled );
   //GetDlgItem( IDC_MAXVALUE_LABEL  )->EnableWindow( editable && globalEnabled );
   //GetDlgItem( IDC_MAXANNUALVALUE  )->EnableWindow( editable && globalEnabled );
   //GetDlgItem( IDC_RESOURCE_LIMIT  )->EnableWindow( editable && globalEnabled );
   //GetDlgItem( IDC_MAX_AREA        )->EnableWindow( editable && globalEnabled );
   //GetDlgItem( IDC_MAX_COUNT       )->EnableWindow( editable && globalEnabled );
      
   GetDlgItem( IDC_ADDPOLICYCONSTRAINT   )->EnableWindow( editable && policyEnabled ); //m_globalConstraints.GetCount() > 0 );
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




void PPPolicyConstraints::OnBnClickedAddglobalconstraint()
   {
   GlobalConstraintsDlg dlg( m_pParent, this );
   if ( dlg.DoModal() == IDOK )
      {
      LoadGCCombo();
      }
   }
