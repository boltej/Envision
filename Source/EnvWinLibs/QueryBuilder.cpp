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
// QueryBuilder.cpp : implementation file
//
#include "EnvWinLibs.h"
#pragma hdrstop


#include "QueryBuilder.h"
#include "SpatialOperatorsDlg.h"
#include "queryViewer.h"

#include <queryengine.h>
#include <MapWnd.h>
#include <maplayer.h>
#include <MAP.h>
#include <UNITCONV.H>

CStringArray QueryBuilder::m_queryStringArray;


// QueryBuilder dialog

IMPLEMENT_DYNAMIC(QueryBuilder, CDialog)
QueryBuilder::QueryBuilder(QueryEngine *pDefQueryEngine, MapWindow *pMapWnd, MapLayer *pInitLayer, int record /*=-1*/, CWnd* pParent /*=NULL*/)
: CDialog(QueryBuilder::IDD, pParent)
, m_pQueryEngine( pDefQueryEngine )
, m_pLayer( pDefQueryEngine->m_pMapLayer )
, m_pMapWnd( pMapWnd )
, m_record( record )
, m_runGlobal(FALSE)
, m_clearPrev(TRUE)
, m_queryString(_T(""))
   { 
   if (pInitLayer != NULL)
      m_pLayer = pInitLayer;
   }


QueryBuilder::~QueryBuilder()
{ }


void QueryBuilder::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LIB_QBFIELDS, m_fields);
DDX_Control(pDX, IDC_LIB_QBOPERATORS, m_ops);
DDX_Control(pDX, IDC_LIB_QBVALUES, m_values);
DDX_Control(pDX, IDC_LIB_QBQUERY, m_query);
DDX_Check(pDX, IDC_LIB_QBRUNGLOBAL, m_runGlobal);
DDX_Check(pDX, IDC_LIB_QBCLEARPREV, m_clearPrev);
DDX_Control(pDX, IDC_LIB_QBHISTORY, m_historyCombo);
DDX_Control(pDX, IDC_LIB_QBLAYER, m_layers);
DDX_Text(pDX, IDC_LIB_QBQUERY, m_queryString);
   }


BEGIN_MESSAGE_MAP(QueryBuilder, CDialog)
   ON_CBN_SELCHANGE(IDC_LIB_QBLAYER, OnCbnSelchangeLayers)
   ON_BN_CLICKED(IDC_LIB_QBADD, OnBnClickedAdd)
   ON_CBN_SELCHANGE(IDC_LIB_QBFIELDS, OnCbnSelchangeFields)
   ON_BN_CLICKED(IDC_LIB_QBAND, OnBnClickedAnd)
   ON_BN_CLICKED(IDC_LIB_QBOR, OnBnClickedOr)
   ON_BN_CLICKED(IDC_LIB_QBSPATIALOPS, OnBnClickedSpatialops)
   ON_BN_CLICKED(IDC_LIB_QBQUERYVIEWER, &QueryBuilder::OnBnClickedQueryviewer)
   ON_BN_CLICKED(IDCANCEL, &QueryBuilder::OnBnClickedCancel)
   ON_CBN_SELCHANGE(IDC_LIB_QBHISTORY, &QueryBuilder::OnCbnSelchangeHistory)
   ON_BN_CLICKED(IDC_LIB_QBRUN, &QueryBuilder::OnBnClickedRun)
END_MESSAGE_MAP()


// QueryBuilder message handlers

BOOL QueryBuilder::OnInitDialog()
   {
   CDialog::OnInitDialog();

   //if ( m_pMapWnd == NULL )
   //   {
   //   CWnd *pWnd = this->GetDlgItem( IDOK );
   //   pWnd->SetWindowTextA( "Save" );
   //
   //   pWnd = this->GetDlgItem( IDCANCEL );
   //   pWnd->SetWindowTextA( "Cancel" );
   //   }

   m_query.SetWindowText( this->m_queryString );

   // load layers 
   Map *pMap = m_pLayer->m_pMap;
   int index = -1;

   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );
      m_layers.AddString( pLayer->m_name );

      if ( pLayer == m_pLayer )
         index = i;
      }

   m_layers.SetCurSel( index );

   LoadLayer();

   LoadHistory();

   index = -1;

   if ( index >= 0 )
      m_fields.SetCurSel( index );
   else 
      m_fields.SetCurSel( 0 );

   m_ops.AddString( _T("=") );
   m_ops.AddString( _T("!=") );
   m_ops.AddString( _T("<") );
   m_ops.AddString( _T(">") );
   m_ops.SetCurSel( 0 );

   LoadFieldValues();

   if ( m_record == -1 )
      {
      CheckDlgButton( IDC_LIB_QBRUNGLOBAL, 1 );
      GetDlgItem( IDC_LIB_QBRUNGLOBAL )->EnableWindow( 0 );
      }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void QueryBuilder::LoadLayer( void )
   {
   int layer = m_layers.GetCurSel();

   Map *pMap = m_pLayer->m_pMap;
   m_pLayer = pMap->GetLayer( layer );

   m_fields.ResetContent();

   for ( int i=0; i < m_pLayer->GetFieldInfoCount(1); i++ )
      {
      MAP_FIELD_INFO *pInfo = m_pLayer->GetFieldInfo( i );
      if ( pInfo->mfiType != MFIT_SUBMENU )
         m_fields.AddString( pInfo->fieldname );
      }
   }


void QueryBuilder::LoadFieldValues()
   {
   int index = m_fields.GetCurSel();

   if ( index < 0 )
      return;

   char fieldName[ 64 ];
   m_fields.GetLBText( index, fieldName );
   SetDlgItemText( IDC_LIB_QBDESCRIPTION, _T("") );

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


void QueryBuilder::LoadHistory( void )
   {
   m_historyCombo.ResetContent();

   int count = (int) m_queryStringArray.GetSize();

   for ( int i=count-1; i >= 0; i-- )
      {
      m_historyCombo.AddString( m_queryStringArray.GetAt( i ) );
      }

   if ( count > 0 )
      m_historyCombo.SetCurSel( 0 );
   }


void QueryBuilder::OnBnClickedAdd()
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


void QueryBuilder::OnCbnSelchangeLayers()
   {
   LoadLayer();
   }


void QueryBuilder::OnCbnSelchangeFields()
   {
   LoadFieldValues();
   }


void QueryBuilder::OnBnClickedRun()
   {
   UpdateData( 1 );
   m_pQueryEngine = m_pLayer->GetQueryEngine();
   ASSERT(m_pQueryEngine->m_pMapLayer != m_pLayer);
   //pQE = new QueryEngine( m_pLayer );

   Query *pQuery = m_pQueryEngine->ParseQuery( m_queryString, 0, "Query Setup" );

   if ( pQuery == NULL )
      {
      Report::ErrorMsg( _T("Unable to parse query") );
      return;
      }
   
   CWaitCursor c;
   clock_t start = clock();

   if ( m_runGlobal )
      {
      m_pQueryEngine->SelectQuery( pQuery, m_clearPrev ? true : false );
      if ( m_pMapWnd != NULL )
         m_pMapWnd->RedrawWindow();
      }
   else
      {
      bool result = false; 
      if ( pQuery->Run( m_record, result ) == false )
         {
         Report::ErrorMsg( _T("Error solving query") );
         return;
         }
      }

   clock_t finish = clock();
   float duration = (float)(finish - start) / CLOCKS_PER_SEC;
   //CString msg;
   //msg.Format("Query ran in %.3f seconds");
   
   // get area of selection
   int selCount = m_pLayer->GetSelectionCount();
   float area = 0;
   for ( int i=0; i < selCount; i++ )
      {
      int idu = m_pLayer->GetSelection( i );

      switch( m_pLayer->m_layerType )
         {
         case LT_POLYGON:
            area += m_pLayer->GetPolygon( idu )->GetArea();
            break;

         case LT_LINE:
            area += m_pLayer->GetPolygon( idu )->GetEdgeLength();
            break;
         }
      }

   Map *pMap = m_pLayer->m_pMap;
   MAP_UNITS mu = pMap->GetMapUnits();

   float aggArea = 0;
   if ( mu == MU_FEET )
      aggArea = area / FT2_PER_ACRE;
   else if ( mu == MU_METERS )
      aggArea = area / M2_PER_ACRE;

   float totalArea = m_pLayer->GetTotalArea();
   float pctArea = 100 * area / totalArea;

   CString resultStr;
   switch( m_pLayer->m_layerType )
      {
      case LT_POLYGON:
         {
         if ( mu == MU_FEET || mu == MU_METERS )
            resultStr.Format( "Query [%s] returned %i polygons, with an area of %g sq. %s (%g acres), %4.1f%% of the total study area. [%.3f secs]",
               (LPCTSTR) m_queryString, selCount, area, pMap->GetMapUnitsStr(), aggArea, pctArea, (float) duration );
         else
            resultStr.Format( "Query [%s] returned %i polygons, with an area of %g, %4.1f%% of the total study area. [%.3f secs]",
                  (LPCTSTR) m_queryString, selCount, area, pctArea, (float)duration);
         }
         break;

      case LT_LINE:
         {
         if ( mu == MU_FEET || mu == MU_METERS )
            resultStr.Format( "Query [%s] returned %i lines, with a length of %g sq. %s (%g acres), %4.1f%% of the total study area. [%.3f secs]",
               (LPCTSTR) m_queryString, selCount, area, pMap->GetMapUnitsStr(), aggArea, pctArea, (float)duration);
         else
            resultStr.Format( "Query [%s] returned %i lines, with a length of %g, %4.1f%% of the total study area. [%.3f secs]",
                  (LPCTSTR) m_queryString, selCount, area, pctArea, (float)duration);
         }
         break;

      case LT_POINT:
         {
         resultStr.Format( "Query [%s] returned %i points, %4.1f of the total study area. [%.3f secs]",
               (LPCTSTR) m_queryString, selCount, 100.0f*selCount/m_pLayer->GetRecordCount(), (float)duration);
         }
         break;
      }

   //if (m_pQueryEngine->m_pMapLayer != m_pLayer)
   //   delete pQE;
   
   Report::InfoMsg( resultStr, _T("Query Result") );
   
   m_queryStringArray.Add( m_queryString );
   LoadHistory();
   }


void QueryBuilder::OnOK()    // "Close"
   {
   UpdateData( 1 );

   CDialog::OnOK();
   }


void QueryBuilder::OnBnClickedAnd()
   {
   m_query.ReplaceSel( _T(" and "), TRUE );

   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 );   
   }


void QueryBuilder::OnBnClickedOr()
   {
   m_query.ReplaceSel( _T(" or "), TRUE );

   int end = m_query.GetWindowTextLength();
   m_query.SetSel( end, end, 1 );
   }


void QueryBuilder::OnBnClickedSpatialops()
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


void QueryBuilder::OnBnClickedQueryviewer()
   {
   CString qstr;
   m_query.GetWindowText( qstr );
  
   QueryViewer dlg( qstr, m_pLayer, this );
   dlg.DoModal();
   }


void QueryBuilder::OnBnClickedCancel()
   {
   UpdateData();
   m_query.GetWindowText( m_queryString );
   OnCancel();
   }


void QueryBuilder::OnCbnSelchangeHistory()
   {
   //int index = m_historyCombo.GetCurSel();

   CString query;
   m_historyCombo.GetWindowText( query );

   m_query.SetWindowText( query );
   }


