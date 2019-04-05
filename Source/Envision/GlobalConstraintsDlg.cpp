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
// GlobalConstraintsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "GlobalConstraintsDlg.h"
#include "afxdialogex.h"

#include "PolEditor.h"
#include <NameDlg.h>

// GlobalConstraintsDlg dialog

IMPLEMENT_DYNAMIC(GlobalConstraintsDlg, CDialog)

GlobalConstraintsDlg::GlobalConstraintsDlg(PolEditor *pPolEditor, CWnd* pParent /*=NULL*/)
	: CDialog(GlobalConstraintsDlg::IDD, pParent)
   , m_pPolEditor( pPolEditor )
   , m_globalConstraintIndex( -1 )
   , m_maxAnnualValueExpr(_T("0"))
{ }

GlobalConstraintsDlg::~GlobalConstraintsDlg()
{ }


void GlobalConstraintsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_GLOBALCONSTRAINTS, m_globalConstraints);
   DDX_Text(pDX, IDC_MAXANNUALVALUE, m_maxAnnualValueExpr);
}


BEGIN_MESSAGE_MAP(GlobalConstraintsDlg, CDialog)
   ON_BN_CLICKED(IDC_ADDGC, &GlobalConstraintsDlg::OnBnClickedAddgc)
   ON_BN_CLICKED(IDC_DELETEGC, &GlobalConstraintsDlg::OnBnClickedDeletegc)
   ON_LBN_SELCHANGE(IDC_GLOBALCONSTRAINTS, &GlobalConstraintsDlg::OnLbnSelchangeGlobalconstraints)
   ON_BN_CLICKED(IDC_RESOURCE_LIMIT, &GlobalConstraintsDlg::OnBnClickedResourceLimit)
   ON_BN_CLICKED(IDC_MAX_AREA, &GlobalConstraintsDlg::OnBnClickedResourceLimit)
   ON_BN_CLICKED(IDC_MAX_COUNT, &GlobalConstraintsDlg::OnBnClickedResourceLimit)
   ON_EN_CHANGE(IDC_MAXANNUALVALUE, &GlobalConstraintsDlg::OnEnChangeMaxAnnualValue)
END_MESSAGE_MAP()


BOOL GlobalConstraintsDlg::OnInitDialog() 
   {   
	CDialog::OnInitDialog();

   for ( int i=0; i < (int) m_pPolEditor->m_constraintArray.GetSize(); i++ )
      m_globalConstraints.AddString( m_pPolEditor->m_constraintArray[ i ]->m_name );

   if  ( m_pPolEditor->m_constraintArray.GetSize() > 0 )
      {
      m_globalConstraints.SetCurSel( 0 );
      LoadGlobalConstraint();
      }
   else
      m_globalConstraints.SetCurSel( 0 );

   EnableControls();   
   return TRUE;
   }


// GlobalConstraintsDlg message handlers
void GlobalConstraintsDlg::MakeDirtyGlobal( void )
   {
   m_pPolEditor->MakeDirty( DIRTY_GLOBAL_CONSTRAINTS );
   }


void GlobalConstraintsDlg::LoadGlobalConstraint()
   {
   m_globalConstraintIndex = m_globalConstraints.GetCurSel();

   if ( m_globalConstraintIndex >= 0 )
      {
      GlobalConstraint *pConstraint = m_pPolEditor->m_constraintArray[ m_globalConstraintIndex ];

      this->m_maxAnnualValueExpr = pConstraint->m_expression;

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



bool GlobalConstraintsDlg::StoreGlobalChanges()
   {
   // NOTE:  These assume any changes to a current constraint not
   //        stored here are already updated elsewhere in response to 
   //        a user edit

   UpdateData( TRUE );  // save and validate member variables

   //GlobalConstraint *pConstraint = m_pParent->m_constraintArray[ m_globalConstraintIndex ];

   return true;
   }


void GlobalConstraintsDlg::OnBnClickedAddgc()
   {
   NameDlg dlg( this );
   dlg.m_title = "Add New Global Constraint";
   dlg.m_name = "<Global Constraint>";
   if ( dlg.DoModal() == IDOK )
      {
      PolicyManager *pPolicyManager = this->m_pPolEditor->m_pPolicy->m_pPolicyManager;

      // add a new global constraint
      m_globalConstraintIndex = (int) m_pPolEditor->m_constraintArray.Add( new GlobalConstraint( pPolicyManager, dlg.m_name, CT_RESOURCE_LIMIT, 0 ) );
      m_globalConstraints.AddString( dlg.m_name );
      m_globalConstraints.SetCurSel( m_globalConstraintIndex );
      LoadGlobalConstraint();
 
      m_pPolEditor->MakeDirty( DIRTY_GLOBAL_CONSTRAINTS );
      EnableControls();
      }
   }


void GlobalConstraintsDlg::OnBnClickedDeletegc()
   {
   if ( AfxMessageBox( "OK to delete this constraint", MB_YESNO ) == IDOK )
      {
      m_pPolEditor->m_constraintArray.RemoveAt( m_globalConstraintIndex );
      m_globalConstraints.DeleteString( m_globalConstraintIndex );
      m_globalConstraints.SetCurSel( --m_globalConstraintIndex );
      LoadGlobalConstraint();
      m_pPolEditor->MakeDirty( DIRTY_GLOBAL_CONSTRAINTS );
      EnableControls();
      }
   }

void GlobalConstraintsDlg::OnLbnSelchangeGlobalconstraints()
   {
   LoadGlobalConstraint();
   }


void GlobalConstraintsDlg::OnBnClickedResourceLimit()
   {
   GlobalConstraint *pConstraint = m_pPolEditor->m_constraintArray[ m_globalConstraintIndex ];

   if ( IsDlgButtonChecked( IDC_RESOURCE_LIMIT ) )
      pConstraint->m_type = CT_RESOURCE_LIMIT;
      
   else if ( IsDlgButtonChecked( IDC_MAX_AREA ) )
      pConstraint->m_type = CT_MAX_AREA;
   
   else if ( IsDlgButtonChecked( IDC_MAX_COUNT ) )
      pConstraint->m_type = CT_MAX_COUNT;
       
   m_pPolEditor->MakeDirty( DIRTY_GLOBAL_CONSTRAINTS );
   }



void GlobalConstraintsDlg::OnEnChangeMaxAnnualValue()
   {
   // this applies to the current global constraint
   GlobalConstraint *pConstraint = m_pPolEditor->m_constraintArray[ m_globalConstraintIndex ];

   GetDlgItemText( IDC_MAXANNUALVALUE, pConstraint->m_expression );

   m_pPolEditor->MakeDirty( DIRTY_POLICY_CONSTRAINTS );
   }
  



void GlobalConstraintsDlg::EnableControls()
   {
   m_globalConstraintIndex = m_globalConstraints.GetCurSel();

   int globalEnabled = m_globalConstraintIndex >= 0 ? 1 : 0;
   
   //GetDlgItem( IDC_ADDGC           )->EnableWindow( editable );
   GetDlgItem( IDC_DELETEGC        )->EnableWindow( globalEnabled );
   GetDlgItem( IDC_MAXVALUE_LABEL  )->EnableWindow( globalEnabled );
   GetDlgItem( IDC_MAXANNUALVALUE  )->EnableWindow( globalEnabled );
   GetDlgItem( IDC_RESOURCE_LIMIT  )->EnableWindow( globalEnabled );
   GetDlgItem( IDC_MAX_AREA        )->EnableWindow( globalEnabled );
   GetDlgItem( IDC_MAX_COUNT       )->EnableWindow( globalEnabled );
   }



