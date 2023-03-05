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
// PPPolicySiteAttr.cpp : implementation file
//
#include "stdafx.h"

#include "pppolicysiteattr.h"
#include <EnvPolicy.h>
#include "spatialOperatorsDlg.h"
#include <EnvModel.h>
#include "PolEditor.h"
#include "MapPanel.h"

#include <queryengine.h>
#include <map.h>
#include <maplayer.h>

extern EnvModel *gpModel;
extern MapPanel *gpMapPanel;

extern PolicyManager *gpPolicyManager;
//extern QueryEngine *PolicyManager::m_pQueryEngine;


// PPPolicySiteAttr dialog

IMPLEMENT_DYNAMIC(PPPolicySiteAttr, CTabPageSSL)

PPPolicySiteAttr::PPPolicySiteAttr( PolEditor *pParent, EnvPolicy *&pPolicy )
	: CTabPageSSL()
   , m_pPolicy( pPolicy )
   , m_pParent( pParent )
{ }

PPPolicySiteAttr::~PPPolicySiteAttr()
{ }

// PPPolicySiteAttr message handlers
void PPPolicySiteAttr::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Control(pDX, IDC_OPERATORS, m_ops);
DDX_Control(pDX, IDC_VALUES, m_values);
DDX_Control(pDX, IDC_QUERY, m_query);
DDX_Control(pDX, IDC_LAYERS, m_layers);
   }


BEGIN_MESSAGE_MAP(PPPolicySiteAttr, CTabPageSSL)
   ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
   ON_CBN_SELCHANGE(IDC_FIELDS, OnCbnSelchangeFields)
   ON_BN_CLICKED(IDC_AND, OnBnClickedAnd)
   ON_BN_CLICKED(IDC_OR, OnBnClickedOr)
   ON_BN_CLICKED(IDC_SPATIALOPS, OnBnClickedSpatialops)
   ON_EN_CHANGE(IDC_QUERY, OnEnChangeQuery)
   ON_BN_CLICKED(IDC_UPDATE, OnBnClickedUpdate)
   ON_CBN_SELCHANGE(IDC_LAYERS, &PPPolicySiteAttr::OnCbnSelchangeLayers)
   ON_BN_CLICKED(IDC_CHECK, &PPPolicySiteAttr::OnBnClickedCheck)
END_MESSAGE_MAP()


// PPPolicySiteAttr message handlers

BOOL PPPolicySiteAttr::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();

   Map *pMap = gpMapPanel->m_pMap;

   int layerCount = pMap->GetLayerCount();

   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );
      m_layers.AddString( pLayer->m_name );
      }

   m_layers.AddString("<Application Variables>");

   if ( layerCount > 0 )
      {
      m_layers.SetCurSel( 0 );
      m_pLayer = pMap->GetLayer( 0 );
      }
   else
      {
      m_layers.SetCurSel( -1 );
      m_pLayer = NULL;
      }

   LoadFieldInfo();

   m_ops.AddString( "=" );
   m_ops.AddString( "!=" );
   m_ops.AddString( "<" );
   m_ops.AddString( ">" );
   m_ops.AddString( ">=" );
   m_ops.AddString( "<=" );
   m_ops.SetCurSel( 0 );

   GetDlgItem( IDC_SITES )->SetWindowText( " " );
   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void PPPolicySiteAttr::LoadFieldInfo()
   {
   m_fields.ResetContent();
   m_values.ResetContent();

   if (m_pLayer == NULL)   // load app vars instead
      {
      for (int i = 0; i < gpModel->GetAppVarCount(); i++)
         {
         AppVar *pVar = gpModel->GetAppVar(i);
         //VData value = pVar->GetValue();
         m_fields.AddString(pVar->m_name);
         }

      if (gpModel->GetAppVarCount() > 0)
         m_fields.SetCurSel(0);
      }

   else // pLayer != NULL
      {
      for (int i = 0; i < m_pLayer->GetFieldInfoCount(1); i++)
         {
         MAP_FIELD_INFO *pInfo = m_pLayer->GetFieldInfo(i);

         if (pInfo->mfiType != MFIT_SUBMENU)
            {
            bool siteAttr = pInfo->GetExtraBool(2);

            if (siteAttr)
               m_fields.AddString(pInfo->fieldname);
            }
         }

      int levels = gpModel->m_lulcTree.GetLevels();
      int index = -1;
      if (levels > 0)
         index = m_fields.FindString(-1, gpModel->m_lulcTree.GetFieldName(1));

      if (index >= 0)
         m_fields.SetCurSel(index);
      else
         m_fields.SetCurSel(0);
      }

   LoadFieldValues();
   }


void PPPolicySiteAttr::LoadPolicy()
   {
   if ( m_pPolicy == NULL )
      return;

   m_query.SetWindowText( m_pPolicy->m_siteAttrStr );
   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 );

   if ( GetDlgItem( IDC_QUERY ) == NULL || GetDlgItem( IDC_QUERY )->m_hWnd == 0 )
      return;

   BOOL enabled = FALSE;
   if ( m_pParent->IsEditable() )
      enabled = TRUE;
      
   GetDlgItem( IDC_QUERY )->EnableWindow( enabled );
   GetDlgItem( IDC_SITES )->SetWindowText( "" );
   }


bool PPPolicySiteAttr::StoreChanges()
   {
   if ( m_pParent->IsDirty( DIRTY_SITEATTR ) )
      CompilePolicy();

   m_pParent->MakeClean( DIRTY_SITEATTR );   
   return true;
   }


void PPPolicySiteAttr::LoadFieldValues()
   {
   int index = m_fields.GetCurSel();
   if ( index < 0 )
      return;
   
   char fieldName[ 64 ];
   m_fields.GetLBText( index, fieldName );
   SetDlgItemText( IDC_DESCRIPTION, "" );

   m_values.ResetContent();

   if (m_pLayer == NULL) // app variable?
      {
      AppVar *pAppVar = gpModel->FindAppVar(fieldName);

      if (pAppVar)
         {
         m_values.AddString(pAppVar->GetValue().GetAsString());
         m_fields.SetCurSel(0);
         }
      }

   else
      {
      int colCellField = gpModel->m_pIDULayer->GetFieldCol(fieldName);
      ASSERT(colCellField >= 0);

      // check field info
      MAP_FIELD_INFO *pInfo = gpModel->m_pIDULayer->FindFieldInfo(fieldName);

      if (pInfo != NULL)  // found field info.  get attributes to populate box
         {
         SetDlgItemText(IDC_DESCRIPTION, pInfo->label);

         for (int i = 0; i < pInfo->attributes.GetSize(); i++)
            {
            FIELD_ATTR &attr = pInfo->GetAttribute(i);

            CString value;
            value.Format("%s {%s}", attr.value.GetAsString(), attr.label);

            m_values.AddString(value);
            }

         m_values.SetCurSel(0);
         }

      else  // no field info exists...
         {
         // If it is a string type, get uniue values from the database
         switch (gpModel->m_pIDULayer->GetFieldType(colCellField))
            {
            case TYPE_STRING:
            case TYPE_DSTRING:
               {
               CStringArray valueArray;
               gpModel->m_pIDULayer->GetUniqueValues(colCellField, valueArray);
               for (int i = 0; i < valueArray.GetSize(); i++)
                  m_values.AddString(valueArray[i]);
               }
            m_values.SetCurSel(0);
            return;

            case TYPE_BOOL:
               m_values.AddString("0 {False}");
               m_values.AddString("1 {True}");
               m_values.SetCurSel(0);
               return;
            }

         // at this point, we have no attributes defined in the database, and the field is not a string
         // field or a boolean field.  No further action required - user has to type an entry into the
         // combo box     
         }
      }
   return;
   }


void PPPolicySiteAttr::OnBnClickedAdd()
   {
   // get text for underlying representation and add it to the query edit control at the current insertion point.
   CString layer, field, op, value, query;
   if ( m_layers.GetCurSel() > 0 )
      {
      m_layers.GetWindowText( layer );
      layer += ".";
      }

   m_fields.GetWindowText( field );
   m_ops.GetWindowText( op );
   m_values.GetWindowText( value );

   query += layer + field + " " + op + " " + value;
   
   m_query.ReplaceSel( query, TRUE );

   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 );

   MakeDirty();
   }


void PPPolicySiteAttr::OnCbnSelchangeFields()
   {
   LoadFieldValues();
   }


void PPPolicySiteAttr::MakeDirty( void )
   {
   m_pParent->MakeDirty( DIRTY_SITEATTR );
   }


bool PPPolicySiteAttr::CompilePolicy()
   {
   // try to compile site query
   UpdateData();

   CString oldSiteAttrQuery( m_pPolicy->m_siteAttrStr );   // remember old query
   Query *pConstraint = m_pPolicy->m_pSiteAttrQuery;             // and constraint
   m_query.GetWindowText( m_pPolicy->m_siteAttrStr );

   gpPolicyManager->CompilePolicy( m_pPolicy, gpModel->m_pIDULayer );

   // bad compilation?
   if ( m_pPolicy->m_pSiteAttrQuery == NULL )
      {
      //CString msg( "Error compiling site attribute query: " );        // changes 8/10/06 (jpb) to allow bad queries
      //msg += m_pPolicy->m_siteAttrStr;
      //msg += "   You can continue, but this policy will not be functional until the query is fixed.";
      //MessageBox( msg );
      //m_pPolicy->m_siteAttrStr = oldSiteAttrQuery;  // restore original query on failed compilation
      //return false;
      }
   
   return true;
   }


void PPPolicySiteAttr::OnBnClickedAnd()
   {
   m_query.ReplaceSel( " and ", TRUE );

   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 ); 

   MakeDirty();
   }


void PPPolicySiteAttr::OnBnClickedOr()
   {
   m_query.ReplaceSel( " or ", TRUE );

   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 );

   MakeDirty();
   }


void PPPolicySiteAttr::OnBnClickedSpatialops()
   {
   SpatialOperatorsDlg dlg;
   if ( dlg.DoModal() == IDOK )
      {
      switch( dlg.m_group )
         {
         case 0: 
            m_query.ReplaceSel( "NextTo( <query> )", TRUE );
            break;

         case 1: 
            {
            CString text;
            text.Format( "Within( <query>, %g )", dlg.m_distance );
            m_query.ReplaceSel( text, TRUE );
            }
            break;

         case 2: 
            {
            CString text;
            text.Format( "WithinArea( <query>, %g, %g )", dlg.m_distance, dlg.m_areaThreshold );
            m_query.ReplaceSel( text, TRUE );
            }
            break;
         }

      CString qstr;
      m_query.SetFocus();
      m_query.GetWindowText( qstr );
      int pos = qstr.Find( "<query>" );
      m_query.SetSel( pos, pos+7, TRUE );

      MakeDirty();
      }
   }

void PPPolicySiteAttr::OnEnChangeQuery()
   {
   MakeDirty();
   }


void PPPolicySiteAttr::OnBnClickedUpdate()
   {
   CString text;
   GetDlgItemText( IDC_QUERY, text );

   m_pPolicy->SetSiteAttrQuery( text );
   gpPolicyManager->CompilePolicy( m_pPolicy, gpModel->m_pIDULayer );

   // bad compilation?
   if ( m_pPolicy->m_pSiteAttrQuery == NULL )
      {
      CString msg( "Error compiling site attribute query: " );
      msg += m_pPolicy->m_siteAttrStr;
      Report::ErrorMsg( msg );
      return;
      }
      
   m_pParent->ShowSites();
   m_pParent->MakeClean( DIRTY_SITEATTR );
   }


void PPPolicySiteAttr::OnCbnSelchangeLayers()
   {
   int index = m_layers.GetCurSel();

   if (index < 0)
      m_pLayer = NULL;
   else if (index == m_layers.GetCount() - 1)   // app variables
      m_pLayer = NULL;
   else
      m_pLayer = gpMapPanel->m_pMap->GetLayer( index );

   LoadFieldInfo();
   }


void PPPolicySiteAttr::OnBnClickedCheck()
   {
   CString query;
   GetDlgItem( IDC_QUERY )->GetWindowText( query );
   Query *pQuery= gpPolicyManager->m_pQueryEngine->ParseQuery( query, 0, "Policy Site Attr Setup" );

   if ( pQuery == NULL )
      {
      CString msg( "Error compiling site attribute query: " );
      msg += query;
      MessageBox( msg, "Unsuccessful Syntax Check", MB_OK );
      }
   else
      MessageBox( "Site Attribute quey successfully parsed", "Success", MB_OK );
   }
