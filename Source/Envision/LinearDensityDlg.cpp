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
// LinearDensityDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "LinearDensityDlg.h"
#include "afxdialogex.h"

#include <MAP.h>
#include <Maplayer.h>
#include <DirPlaceholder.h>
#include <QueryEngine.h>
#include "EnvDoc.h"
#include <EnvModel.h>

extern MapLayer    *gpCellLayer;
//extern QueryEngine *gpQueryEngine;
extern CEnvDoc     *gpDoc;
extern EnvModel    *gpModel;


int LDMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );


// LinearDensityDlg dialog

IMPLEMENT_DYNAMIC(LinearDensityDlg, CDialog)

LinearDensityDlg::LinearDensityDlg(CWnd* pParent /*=NULL*/)
	: CDialog(LinearDensityDlg::IDD, pParent)
   , m_distance(1000)
   , m_thisQuery(_T(""))
   , m_toQuery(_T(""))
   , m_useToQuery(FALSE)
   , m_useThisQuery(FALSE)
   {

}

LinearDensityDlg::~LinearDensityDlg()
{
}

void LinearDensityDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_TOLAYER, m_layers);
DDX_Control(pDX, IDC_FIELD, m_fields);
DDX_Text(pDX, IDC_DISTANCE, m_distance);
DDX_Control(pDX, IDC_PROGRESS, m_progress);
DDX_Text(pDX, IDC_THISQUERY, m_thisQuery);
DDX_Text(pDX, IDC_TOQUERY, m_toQuery);
DDX_Check(pDX, IDC_USETOQUERY, m_useToQuery);
DDX_Check(pDX, IDC_USETHISQUERY, m_useThisQuery);
   }


BEGIN_MESSAGE_MAP(LinearDensityDlg, CDialog)
   ON_BN_CLICKED(IDC_BROWSE, &LinearDensityDlg::OnBnClickedBrowse)
END_MESSAGE_MAP()


// LinearDensityDlg message handlers


BOOL LinearDensityDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   Map *pMap = gpCellLayer->m_pMap;

   int count = 0;

   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );

      if ( pLayer->m_layerType == LT_LINE )
         {
         m_layers.AddString( pLayer->m_name );
         count++;
         }
      }

   if ( count == 0 )
      MessageBox( "No linear feature layers are current loaded.  Use <Browse> to load a linear feature layer" );
   else
      m_layers.SetCurSel( 0 );

   for ( int i=0; i < gpCellLayer->GetFieldCount(); i++ )
      m_fields.AddString( gpCellLayer->GetFieldLabel( i ) );

   m_fields.SetCurSel( 0 );

   SetDlgItemText( IDC_UNITS, pMap->GetMapUnitsStr() );


   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void LinearDensityDlg::OnBnClickedBrowse()
   {
   DirPlaceholder d;
   
   CFileDialog dlg( TRUE, _T("shp"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Shape files|*.shp|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );

      Map *pMap = gpCellLayer->m_pMap;

      MapLayer *pLayer = pMap->AddShapeLayer( filename, true, 0, -1 );

      if ( pLayer != NULL )
         m_layers.AddString( pLayer->m_name );
      else
         MessageBox( _T("Error adding map layer" ) );
      }      
   }


void LinearDensityDlg::OnOK()
   {
   UpdateData( 1 );

   CString layerName, fieldName;

   m_layers.GetWindowText( layerName );
   m_fields.GetWindowText( fieldName );

   Map *pMap = gpCellLayer->m_pMap;

   MapLayer *pToLayer = pMap->GetLayer( layerName );
   ASSERT( pToLayer != NULL );

   int col = gpCellLayer->GetFieldCol( fieldName );
   if ( col < 0 )
      {
      CString msg( _T("The target column [") );
      msg += fieldName;
      msg += _T("] does not exist in the IDU coverage - would you like to add it?" );
      int retVal = MessageBox( msg, _T("Add Field?"), MB_YESNO );

      if ( retVal == IDCANCEL || retVal == IDNO )
         return;

      int width, decimals;
      GetTypeParams( TYPE_FLOAT, width, decimals );
      col = gpCellLayer->m_pDbTable->AddField( fieldName, TYPE_FLOAT, width, decimals, true );
      gpCellLayer->SetColNoData( col );
      }

   ASSERT( col >= 0 );

   m_toQuery.Trim();
   m_thisQuery.Trim();

   if ( m_toQuery.IsEmpty() )
      m_useToQuery = false;
   if ( m_thisQuery.IsEmpty() )
      m_useThisQuery = false;
   
   if ( m_useToQuery )
      {
      MapLayer *pOldLayer = gpModel->m_pQueryEngine->m_pMapLayer;
      gpModel->m_pQueryEngine->m_pMapLayer = pToLayer;
      Query *pToQuery = gpModel->m_pQueryEngine->ParseQuery( m_toQuery, 0, "Linear Density Setup" );
      if ( pToQuery )
         {
         Report::ErrorMsg( "Unable to parse query" );
         gpModel->m_pQueryEngine->m_pMapLayer = pOldLayer;
         return;
         }
         
      //if ( gpCellLayer->GetSpatialIndex() == NULL )
      //   gpCellLayer->CreateSpatialIndex( NULL, 10000, m_distance, SIM_NEAREST );

      gpModel->m_pQueryEngine->SelectQuery( pToQuery, true ); //, false );
      gpModel->m_pQueryEngine->m_pMapLayer = pOldLayer;
      }

   if ( m_useThisQuery )
      {
      MapLayer *pOldLayer = gpModel->m_pQueryEngine->m_pMapLayer;
      gpModel->m_pQueryEngine->m_pMapLayer = gpCellLayer;
      Query *pThisQuery = gpModel->m_pQueryEngine->ParseQuery( m_thisQuery, 0, "Linear Density Setup" );
      if ( pThisQuery == NULL  )
         {
         Report::ErrorMsg( "Unable to parse query" );
         gpModel->m_pQueryEngine->m_pMapLayer = pOldLayer;
         return;
         }
         
      //if ( gpCellLayer->GetSpatialIndex() == NULL )
      //   gpCellLayer->CreateSpatialIndex( NULL, 10000, 500, SIM_NEAREST );

      gpModel->m_pQueryEngine->SelectQuery( pThisQuery, true); //, false );
      gpModel->m_pQueryEngine->m_pMapLayer = pOldLayer;
      }
   
   pMap->InstallNotifyHandler( LDMapProc, (LONG_PTR) this );
   CWaitCursor c;

   gpCellLayer->PopulateLinearNetworkDensity( pToLayer, col, m_distance, m_useToQuery ? true : false, m_useThisQuery ? true : false );

   // save database?   
   int result = MessageBox( _T("Density calculations complete.  Would you like save the results in the database at this time?"), _T("Save Results?"), MB_YESNO );

   if ( result == IDYES )
      {
      DirPlaceholder d;

      CFileDialog dlg( FALSE, "dbf", gpCellLayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "DBase Files|*.dbf|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         {
         CString filename( dlg.GetPathName() );
         gpCellLayer->SaveDataDB( filename ); // uses DBase
         }
      else
         gpDoc->SetChanged( CHANGED_COVERAGE );

      }
   else
      gpDoc->SetChanged( CHANGED_COVERAGE );

     
   pMap->RemoveNotifyHandler( LDMapProc, (LONG_PTR) this );
   }


int LDMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra )
   {
   LinearDensityDlg *pDlg = (LinearDensityDlg*) extra;

   switch( type )
      {
      case NT_POPULATERECORDS:
         if ( a0 == 0 )
            pDlg->m_progress.SetRange32( 0, int( a1/100 ) );

         if ( a0%100 == 0 )
            pDlg->m_progress.SetPos( a0/100 );

         break;
      }

   return 1;
   }

