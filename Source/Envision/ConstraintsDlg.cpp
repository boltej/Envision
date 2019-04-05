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
// ConstraintsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "ConstraintsDlg.h"
#include "afxdialogex.h"

#include "EnvDoc.h"
#include <QueryBuilder.h>
#include <Maplayer.h>

extern CEnvDoc  *gpDoc;
extern MapLayer *gpCellLayer;
extern EnvModel *gpModel;

// ConstraintsDlg dialog

IMPLEMENT_DYNAMIC(ConstraintsDlg, CDialog)

ConstraintsDlg::ConstraintsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ConstraintsDlg::IDD, pParent)
{

}

ConstraintsDlg::~ConstraintsDlg()
{
}

void ConstraintsDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_CONSTRAINTS, m_constraints);
   }


BEGIN_MESSAGE_MAP(ConstraintsDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, &ConstraintsDlg::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_REMOVE, &ConstraintsDlg::OnBnClickedRemove)
   ON_BN_CLICKED(IDC_EDIT, &ConstraintsDlg::OnBnClickedEdit)
END_MESSAGE_MAP()


// ConstraintsDlg message handlers


BOOL ConstraintsDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   LoadConstraints();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void ConstraintsDlg::LoadConstraints()
   {
   m_constraints.ResetContent();

   for ( int i=0; i < gpDoc->GetConstraintCount(); i++ )
      {
      CString &constraint = gpDoc->GetConstraint( i );
      m_constraints.AddString( constraint );
      }

   m_constraints.SetCurSel( 0 );
   }


void ConstraintsDlg::OnOK()
   {
   gpDoc->RemoveConstraints();

   for ( int i=0; i < m_constraints.GetCount(); i++ )
      {
      CString constraint;
      m_constraints.GetText( i, constraint );
      if ( ! constraint.IsEmpty() )
         gpDoc->AddConstraint( constraint ); 
      }

   CDialog::OnOK();
   }


void ConstraintsDlg::OnBnClickedAdd()
   {
   QueryBuilder dlg(gpModel->m_pQueryEngine, NULL, NULL, -1, this );
   if ( dlg.DoModal() == IDCANCEL )
      {
      CString constraint = dlg.m_queryString;
      m_constraints.AddString( constraint );
      }
   }


void ConstraintsDlg::OnBnClickedRemove()
   {
   int index = m_constraints.GetCurSel();

   if ( index < 0 )
      return;

   m_constraints.DeleteString( index );
   m_constraints.SetCurSel( 0 );
   }


void ConstraintsDlg::OnBnClickedEdit()
   {
   int index = m_constraints.GetCurSel();

   if ( index < 0 )
      return;

   QueryBuilder dlg(gpModel->m_pQueryEngine, NULL, NULL, -1, this );
   
   CString constraint;
   m_constraints.GetText( index, constraint );
   dlg.m_queryString = constraint;
   if ( dlg.DoModal() == IDCANCEL )
      {
      constraint = dlg.m_queryString;
      m_constraints.DeleteString( index );
      m_constraints.InsertString( index, constraint );
      m_constraints.SetCurSel( index );
      }
   }
