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
// PolQueryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include ".\PolQueryDlg.h"
#include "envdoc.h"
#include <EnvModel.h>
#include "SpatialOperatorsDlg.h"
#include <queryViewer.h>

#include <queryengine.h>
#include <maplayer.h>

extern CEnvDoc     *gpDoc;
extern EnvModel    *gpModel;
//extern QueryEngine *gpQueryEngine;


// PolQueryDlg dialog

IMPLEMENT_DYNAMIC(PolQueryDlg, CDialog)
PolQueryDlg::PolQueryDlg(MapLayer *pLayer, CWnd* pParent /*=NULL*/)
: CDialog(PolQueryDlg::IDD, pParent)
, m_pLayer( pLayer )
   { }

PolQueryDlg::~PolQueryDlg()
{ }

void PolQueryDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Control(pDX, IDC_OPERATORS, m_ops);
DDX_Control(pDX, IDC_VALUES, m_values);
DDX_Control(pDX, IDC_QUERY, m_query);
}


BEGIN_MESSAGE_MAP(PolQueryDlg, CDialog)
   ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
   ON_CBN_SELCHANGE(IDC_FIELDS, OnCbnSelchangeFields)
   ON_BN_CLICKED(IDC_AND, OnBnClickedAnd)
   ON_BN_CLICKED(IDC_OR, OnBnClickedOr)
   ON_BN_CLICKED(IDC_SPATIALOPS, OnBnClickedSpatialops)
   ON_BN_CLICKED(IDC_QUERYVIEWER, &PolQueryDlg::OnBnClickedQueryviewer)

END_MESSAGE_MAP()


// PolQueryDlg message handlers

BOOL PolQueryDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   m_query.SetWindowText( this->m_queryString );

   for ( int i=0; i < m_pLayer->GetFieldInfoCount(1); i++ )
      {
      MAP_FIELD_INFO *pInfo = m_pLayer->GetFieldInfo( i );
      if ( pInfo->mfiType != MFIT_SUBMENU )
         m_fields.AddString( pInfo->fieldname );
      }

   int index = m_fields.FindString( -1, gpModel->m_lulcTree.GetFieldName( 1 ) );
   m_fields.SetCurSel( index );

   m_ops.AddString( _T("=") );
   m_ops.AddString( _T("!=") );
   m_ops.AddString( _T("<") );
   m_ops.AddString( _T(">") );
   m_ops.SetCurSel( 0 );

   LoadFieldValues();

    return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void PolQueryDlg::LoadFieldValues()
   {
   int index = m_fields.GetCurSel();

   if ( index < 0 )
      return;

   char fieldName[ 64 ];
   m_fields.GetLBText( index, fieldName );
   SetDlgItemText( IDC_DESCRIPTION, _T("") );

   m_values.ResetContent();
   int colCellField = m_pLayer->GetFieldCol( fieldName );
   ASSERT( colCellField >= 0 );

   // check field info
   MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( fieldName );

   if ( pInfo != NULL )  // found field info.  get attributes to populate box
      {
      for ( int i=0; i < pInfo->attributes.GetSize(); i++ )
         {
         FIELD_ATTR &attr = pInfo->GetAttribute( i );

         CString value;
         value.Format( "%s {%s}", attr.value.GetAsString(), attr.label );

         m_values.AddString( value );
         }

      m_values.SetCurSel( 0 );
      }

   else  // no field info exists...
      {
      // If it is a string type, get uniue values from the database
      switch( m_pLayer->GetFieldType( colCellField ) )
         {
         case TYPE_STRING:
         case TYPE_DSTRING:
            {
            CStringArray valueArray;
            m_pLayer->GetUniqueValues( colCellField, valueArray );
            for ( int i=0; i < valueArray.GetSize(); i++ )
               m_values.AddString( valueArray[ i ] );
            }
            m_values.SetCurSel( 0 );
            return;

         case TYPE_BOOL:
            m_values.AddString( _T("0 {False}") );
            m_values.AddString( _T("1 {True}") );
            m_values.SetCurSel( 0 );
            return;
         }

      // at this point, we have no attributes defined in the database, and the field is not a string
      // field or a boolean field.  No further action required - user has to type an entry into the
      // combo box     
      }
   return;
   }


void PolQueryDlg::OnBnClickedAdd()
   {
   // get text for underlying representation and add it to the query edit control at the current insertion point.
   CString field, op, value, query;
   m_fields.GetWindowText( field );
   m_ops.GetWindowText( op );
   m_values.GetWindowText( value );

   query += field + _T(" ") + op + _T(" ") + value;
   
   m_query.ReplaceSel( query, TRUE );

   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 );
   }


void PolQueryDlg::OnCbnSelchangeFields()
   {
   LoadFieldValues();
   }


void PolQueryDlg::OnOK()
   {
   UpdateData();
   m_query.GetWindowText( this->m_queryString );

   CDialog::OnOK();
   }


void PolQueryDlg::OnBnClickedAnd()
   {
   m_query.ReplaceSel( _T(" and "), TRUE );

   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 );   
   }


void PolQueryDlg::OnBnClickedOr()
   {
   m_query.ReplaceSel( _T(" or "), TRUE );

   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 );
   }


void PolQueryDlg::OnBnClickedSpatialops()
   {
   SpatialOperatorsDlg dlg;
   if ( dlg.DoModal() == IDOK )
      {
      switch( dlg.m_group )
         {
         case 0: 
            m_query.ReplaceSel( _T("NextTo( <query> )"), TRUE );
            break;

         case 1: 
            {
            CString text;
            text.Format( _T("Within( <query>, %g )"), dlg.m_distance );
            m_query.ReplaceSel( text, TRUE );
            }
            break;

         case 2: 
            {
            CString text;
            text.Format( _T("WithinArea( <query>, %g, %g )"), dlg.m_distance, dlg.m_areaThreshold );
            m_query.ReplaceSel( text, TRUE );
            }
            break;
         }

      CString qstr;
      m_query.SetFocus();
      m_query.GetWindowText( qstr );
      int pos = qstr.Find( _T("<query>") );
      m_query.SetSel( pos, pos+7, TRUE );
      }
   }


void PolQueryDlg::OnBnClickedQueryviewer()
   {
   CString qstr;
   m_query.GetWindowText( qstr );
  
   QueryViewer dlg( qstr, m_pLayer, this );
   dlg.DoModal();
   }
