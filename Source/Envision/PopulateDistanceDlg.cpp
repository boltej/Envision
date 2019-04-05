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
// PopulateDistanceDlg.cpp : implementation file
//

#include "stdafx.h"
#include ".\populatedistancedlg.h"
#include "envdoc.h"
#include "mainfrm.h"

#include <maplayer.h>
#include <math.h>
#include <queryengine.h>
#include <DirPlaceholder.h>

#include <direct.h>


extern MapLayer    *gpCellLayer;
extern CEnvDoc     *gpDoc;
extern CMainFrame  *gpMain;


int PDMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );

// PopulateDistanceDlg dialog

IMPLEMENT_DYNAMIC(PopulateDistanceDlg, CDialog)
PopulateDistanceDlg::PopulateDistanceDlg( Map *pMap, CWnd* pParent /*=NULL*/ )
	: CDialog(PopulateDistanceDlg::IDD, pParent)
   , m_thresholdDistance(10)
   , m_pSourceQueryEngine( NULL )
   , m_pTargetQueryEngine( NULL )
   {
   ASSERT( pMap != NULL );
   m_pMap = pMap;
   }

PopulateDistanceDlg::~PopulateDistanceDlg()
   { }


void PopulateDistanceDlg::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_PDD_TARGETLAYER, m_targetLayerCombo);
   DDX_Control(pDX, IDC_PDD_SOURCELAYER, m_sourceLayerCombo);
   DDX_Control(pDX, IDC_PDD_TARGETFIELD, m_targetFieldCombo);
   DDX_Control(pDX, IDC_PDD_INDEXFIELD,  m_indexFieldCombo);
   DDX_Control(pDX, IDC_PDD_VALUEFIELD,  m_valueFieldCombo);
   DDX_Control(pDX, IDC_PDD_SOURCEVALUEFIELD,  m_sourceValueFieldCombo);
   DDX_Control(pDX, IDC_QUERYTARGET, m_qTarget);
   DDX_Control(pDX, IDC_QUERYSOURCE, m_qSource);
   DDX_Text(pDX, IDC_DISTANCE, m_thresholdDistance);
   DDV_MinMaxFloat(pDX, m_thresholdDistance, 0, 100000);
   DDX_Control(pDX, IDC_DISTANCE, m_editDistance);
   DDX_Control(pDX, IDC_PROGRESS, m_progress);
   DDX_Control(pDX, IDC_STATUSTEXT, m_status);
   }


BEGIN_MESSAGE_MAP(PopulateDistanceDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_PDD_TARGETLAYER, OnCbnSelchangePddTargetlayer)
   ON_CBN_SELCHANGE(IDC_PDD_SOURCELAYER, OnCbnSelchangePddSourcelayer)
   ON_BN_CLICKED(IDC_PDD_CALCULATE, OnCalculate)
   ON_BN_CLICKED(IDC_PDD_EXIT, OnExit)
   ON_BN_CLICKED(IDC_POPULATENEARESTINDEX, &PopulateDistanceDlg::OnBnClickedPopulatenearestindex)
   ON_BN_CLICKED(IDC_POPULATENEARESTVALUE, &PopulateDistanceDlg::OnBnClickedPopulatenearestvalue)
   ON_BN_CLICKED(IDC_POPULATECOUNT, &PopulateDistanceDlg::OnBnClickedPopulatecount)
   ON_BN_CLICKED(IDC_BROWSE, &PopulateDistanceDlg::OnBnClickedBrowse)
END_MESSAGE_MAP()


// PopulateDistanceDlg message handlers

BOOL PopulateDistanceDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   int layerCount = (int) m_pMap->m_mapLayerArray.GetCount();

   m_targetLayerCombo.AddString( gpCellLayer->m_name );
   m_targetLayerCombo.SetCurSel( 0 );

   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = m_pMap->m_mapLayerArray.GetAt(i).pLayer;

      if ( pLayer->GetType() == LT_POLYGON || pLayer->GetType() == LT_LINE || pLayer->GetType() == LT_POINT )
         m_sourceLayerCombo.AddString( pLayer->m_name );
      }

   m_sourceLayerCombo.SetCurSel( 0 );

   CStringArray strArray;
   gpCellLayer->LoadFieldStrings( strArray );

   int fieldCount = (int) strArray.GetCount();
   for ( int i=0; i < fieldCount; i++ )
      {
      m_targetFieldCombo.AddString( strArray.GetAt(i) );
      m_indexFieldCombo.AddString( strArray.GetAt(i) );
      m_valueFieldCombo.AddString( strArray.GetAt(i) );
      m_sourceValueFieldCombo.AddString( strArray.GetAt(i) );
      }

   m_targetFieldCombo.SetCurSel( 0 );
   m_indexFieldCombo.SetCurSel( 0 );
   m_valueFieldCombo.SetCurSel( 0 );
   m_sourceValueFieldCombo.SetCurSel( 0 );
   
   CheckDlgButton( IDC_POPULATENEARESTINDEX, 0 );
   m_indexFieldCombo.EnableWindow( 0 );
   
   CheckDlgButton( IDC_POPULATENEARESTVALUE, 0 );
   m_valueFieldCombo.EnableWindow( 0 );
   m_sourceValueFieldCombo.EnableWindow( 0 );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void PopulateDistanceDlg::OnCbnSelchangePddTargetlayer()
   {
/*   int index = m_fromLayerCombo.GetCurSel();
   CString layerName;
   m_fromLayerCombo.GetLBText( index, layerName );
   MapLayer *pLayer = m_pMap->GetLayer( layerName );
   ASSERT( pLayer != NULL );
*/   
   }


void PopulateDistanceDlg::OnCbnSelchangePddSourcelayer()
   {
   int index = m_sourceLayerCombo.GetCurSel();
   CString layerName;
   m_sourceLayerCombo.GetLBText( index, layerName );
   MapLayer *pLayer = m_pMap->GetLayer( layerName );
   ASSERT( pLayer != NULL );

   m_sourceValueFieldCombo.ResetContent();

   CStringArray strArray;
   pLayer->LoadFieldStrings( strArray );

   int fieldCount = (int) strArray.GetCount();
   for ( int i=0; i < fieldCount; i++ )
      m_sourceValueFieldCombo.AddString( strArray.GetAt(i) );

   m_sourceValueFieldCombo.SetCurSel( 0 );
   }



void PopulateDistanceDlg::OnCalculate()
   {
   // Note: Target is generally the IDU layer.  Source may be IDU or something else
   UpdateData( 1 );

   CString layerName;
   m_targetLayerCombo.GetWindowText( layerName );
   MapLayer *pTargetLayer = m_pMap->GetLayer( layerName );

   if ( pTargetLayer == NULL )
      {
      Report::WarningMsg( "You must select a target layer to calculate the distances from" );
      return;
      }
   
   m_sourceLayerCombo.GetWindowText( layerName );
   MapLayer *pSourceLayer = m_pMap->GetLayer( layerName );

   if ( pSourceLayer == NULL )
      {
      Report::WarningMsg( "You must select a source layer to calculate the distances from" );
      return;
      }
   
   CString field;
   m_targetFieldCombo.GetWindowText( field );

   int col = pTargetLayer->GetFieldCol( field );

   if ( col < 0 )
      {
      CString msg;
      msg.Format( "The field %s is not in the Layer.  Would you like to add it?", field );
      if ( IDOK == MessageBox( msg, 0, MB_OKCANCEL ) )
         {
         CWaitCursor c;
         m_status.SetWindowText( "Adding field to database..." );
         int width, decimals;
         GetTypeParams( TYPE_FLOAT, width, decimals );
         col = pTargetLayer->m_pDbTable->AddField( field, TYPE_FLOAT, width, decimals, true );
         }
      else
         return;
      }

   LAYER_TYPE targetType = pTargetLayer->GetType();
   LAYER_TYPE sourceType = pSourceLayer->GetType();

   // process query (if one is specified)
   CString qTarget, qSource;
   m_qTarget.GetWindowText( qTarget );
   m_qSource.GetWindowText( qSource );

   qTarget.Trim();
   qSource.Trim();

   bool selectedOnlyTarget = true;
   bool selectedOnlySource   = true;

   if ( qTarget.IsEmpty() )
      selectedOnlyTarget = false;
   if ( qSource.IsEmpty() )
      selectedOnlySource = false;
   
   Query *pSourceQuery = NULL;
   Query *pTargetQuery = NULL;

   if ( selectedOnlySource )
      {
      if ( m_pSourceQueryEngine != NULL )
         delete m_pSourceQueryEngine;

      m_pSourceQueryEngine = new QueryEngine( pSourceLayer );
      pSourceQuery = m_pSourceQueryEngine->ParseQuery( qSource, 0, "Populate Distance Setup" );
      if ( pSourceQuery == NULL )
         {
         Report::ErrorMsg( "Unable to parse source query" );
         delete m_pSourceQueryEngine;
         m_pSourceQueryEngine = NULL;
         return;
         }

      //m_pSourceQueryEngine->SelectQuery( pQuery, true, false );
      }
      
   if ( selectedOnlyTarget )
      {
      if ( m_pTargetQueryEngine != NULL )
         delete m_pTargetQueryEngine;

      m_pTargetQueryEngine = new QueryEngine( pTargetLayer );
      pTargetQuery = m_pTargetQueryEngine->ParseQuery( qTarget, 0, "Populate Distance Setup" );
      if ( pTargetQuery == NULL )
         {
         Report::ErrorMsg( "Unable to parse target query" );
         delete m_pTargetQueryEngine;
         m_pTargetQueryEngine = NULL;
         return;
         }

      //m_pTargetQueryEngine->SelectQuery( pQuery, true, false );
      }
   
   pTargetLayer->m_pMap->InstallNotifyHandler( PDMapProc, (LONG_PTR) this );
   CWaitCursor c;

   //if ( gpCellLayer->GetSpatialIndex() == NULL )  // note: this isn't used!!!
   //   gpCellLayer->CreateSpatialIndex( NULL, 10000, 5000, SIM_NEAREST );
   
   int colNearestToIndex = -1;
   if ( IsDlgButtonChecked( IDC_POPULATENEARESTINDEX ) )
      {
      CString fieldname;
      m_indexFieldCombo.GetWindowText( fieldname );
      colNearestToIndex = pTargetLayer->GetFieldCol( fieldname );

      if ( colNearestToIndex < 0 )
         {
         CString msg;
         msg.Format( "The field %s is not in the Layer.  Would you like to add it?", field );
         if ( IDOK == MessageBox( msg, 0, MB_OKCANCEL ) )
            {
            CWaitCursor c;
            m_status.SetWindowText( "Adding field to database..." );
            colNearestToIndex = pTargetLayer->m_pDbTable->AddField( field, TYPE_LONG, true );
            }
         else
            return;
         }
      }

   int colNearestToValue = -1;
   int colNearestSourceValue = -1;
   if ( IsDlgButtonChecked( IDC_POPULATENEARESTVALUE ) )
      {
      //colNearestToValue = m_valueFieldCombo.GetCurSel();
      //colNearestSourceValue = m_sourceValueFieldCombo.GetCurSel();   
      CString fieldTo;
      m_valueFieldCombo.GetWindowText( fieldTo );
      colNearestToValue = pTargetLayer->GetFieldCol( fieldTo );

      if ( colNearestToValue < 0 )
         {
         CString msg;
         msg.Format( "The field %s is not in the Layer.  Would you like to add it?", fieldTo );
         if ( IDOK == MessageBox( msg, 0, MB_OKCANCEL ) )
            {
            CWaitCursor c;
            m_status.SetWindowText( "Adding field to database..." );
            colNearestToValue = pTargetLayer->m_pDbTable->AddField( fieldTo, TYPE_LONG, true );
            }
         else
            return;
         }

      CString fieldSource;
      m_sourceValueFieldCombo.GetWindowText( fieldSource );
      colNearestSourceValue = pSourceLayer->GetFieldCol( fieldSource );

      if ( colNearestSourceValue < 0 )
         {
         CString msg;
         msg.Format( "The field %s is not in the Layer.  Please specify an existing field", fieldSource );
         MessageBox( msg, 0, MB_OK );
         return;
         }
      }

   m_status.SetWindowText( "Populating distances..." );
   
   if ( IsDlgButtonChecked( IDC_POPULATECOUNT ) )
      pTargetLayer->PopulateNumberNearbyPolys( pSourceLayer, col, m_thresholdDistance+1, pSourceQuery, pTargetQuery, NULL, m_thresholdDistance );
   else
      pTargetLayer->PopulateNearestDistanceToCoverage( pSourceLayer, col, 50000, pSourceQuery, pTargetQuery, NULL, colNearestToIndex, colNearestToValue, colNearestSourceValue );

   int result = MessageBox( _T("Distance calculations complete.  Would you like save the results in the database file at this time?"), _T("Save Results?"), MB_YESNO );

   if ( result == IDYES )
      {
      char *cwd = _getcwd( NULL, 0 );

      CFileDialog dlg( FALSE, "dbf", pTargetLayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "DBase Files|*.dbf|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         {
         CString filename( dlg.GetPathName() );
         pTargetLayer->SaveDataDB( filename ); // uses DBase
         }
      else
         gpDoc->SetChanged( CHANGED_COVERAGE );

      _chdir( cwd );
      free( cwd );
      }
   else
      gpDoc->SetChanged( CHANGED_COVERAGE );

   if ( m_pSourceQueryEngine != NULL )
      {
      //pSourceLayer->ClearSelection();
      delete m_pSourceQueryEngine;
      m_pSourceQueryEngine = NULL;
      }

   if ( m_pTargetQueryEngine != NULL )
      {
      //pTargetLayer->ClearSelection();
      delete m_pTargetQueryEngine;
      m_pTargetQueryEngine = NULL;
      }
      
   pTargetLayer->m_pMap->RemoveNotifyHandler( PDMapProc, (LONG_PTR) this );
   
   m_status.SetWindowText( "Done" );
   }


void PopulateDistanceDlg::OnExit()
   {
   OnOK();
   }

void PopulateDistanceDlg::OnBnClickedPopulatenearestindex()
   {
   if ( IsDlgButtonChecked( IDC_POPULATENEARESTINDEX ) )
      m_indexFieldCombo.EnableWindow( 1 );
   else
      m_indexFieldCombo.EnableWindow( 0 );
   }

void PopulateDistanceDlg::OnBnClickedPopulatenearestvalue()
   {
   if ( IsDlgButtonChecked( IDC_POPULATENEARESTVALUE ) )
      {
      m_valueFieldCombo.EnableWindow( 1 );
      m_sourceValueFieldCombo.EnableWindow( 1 );
      }
   else
      {
      m_valueFieldCombo.EnableWindow( 0 );
      m_sourceValueFieldCombo.EnableWindow( 0 );
      }
   }



void PopulateDistanceDlg::OnBnClickedPopulatecount()
   {
   if ( IsDlgButtonChecked( IDC_POPULATECOUNT ) )
      m_editDistance.EnableWindow( 1 );
   else
      m_editDistance.EnableWindow( 0 );
   }



void PopulateDistanceDlg::OnBnClickedBrowse()
   {
   DirPlaceholder d;
   
   CFileDialog dlg( TRUE, _T("shp"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Shape files|*.shp|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );

      Map *pMap = gpCellLayer->m_pMap;

      MapLayer *pLayer = pMap->AddShapeLayer( filename, true, 0, -1 );

      if ( pLayer != NULL )
         m_sourceLayerCombo.AddString( pLayer->m_name );
      else
         MessageBox( _T("Error adding map layer" ) );

      int count = pMap->GetLayerCount();
      m_sourceLayerCombo.SetCurSel( count-1 );
      }
   }



int PDMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra )
   {
   PopulateDistanceDlg *pDlg = (PopulateDistanceDlg*) extra;

   switch( type )
      {
      case NT_BUILDSPATIALINDEX:
         {      
         if ( a0 == 0 )
            pDlg->m_progress.SetRange32( 0, int( a1/100 ) );

         if ( a0%100 == 0 )
            pDlg->m_progress.SetPos( a0/100 );
         }
         break;

     case NT_CALCDIST:
         {
         if ( a0 == 0 )
            pDlg->m_progress.SetRange32( 0, int( a1/100 ) );

         if ( a0%100 == 0 )
            pDlg->m_progress.SetPos( a0/100 );
         }
         break;
      }

   return 1;
   }