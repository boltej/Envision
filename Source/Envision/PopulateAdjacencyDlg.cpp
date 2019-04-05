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
// PopulateAdjacencyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PopulateAdjacencyDlg.h"

#include "envdoc.h"
#include "mainfrm.h"
#include <EnvModel.h>

#include <maplayer.h>
#include <math.h>
#include <queryengine.h>

#include <direct.h>


//extern QueryEngine *gpModel->m_pQueryEngine;
extern MapLayer    *gpCellLayer;
extern CEnvDoc     *gpDoc;
extern CMainFrame  *gpMain;
extern EnvModel    *gpModel;

// PopulateAdjacencyDlg dialog

IMPLEMENT_DYNAMIC(PopulateAdjacencyDlg, CDialog)

PopulateAdjacencyDlg::PopulateAdjacencyDlg(Map *pMap, CWnd* pParent /*=NULL*/)	: CDialog(PopulateAdjacencyDlg::IDD, pParent)
   {
   ASSERT( pMap != NULL );

   m_pMap = pMap;
   }

PopulateAdjacencyDlg::~PopulateAdjacencyDlg()
   {
   }

void PopulateAdjacencyDlg::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_PAP_SOURCELAYER, m_sourceLayerCombo);
   DDX_Control(pDX, IDC_PAP_TARGETLAYER, m_targetLayerCombo);
   DDX_Control(pDX, IDC_PAP_TARGETFIELD, m_targetFieldCombo);
   DDX_Control(pDX, IDC_QUERY, m_query);
   }


BEGIN_MESSAGE_MAP(PopulateAdjacencyDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_PAP_TARGETLAYER, OnCbnSelchangePapTargetlayer)
   ON_BN_CLICKED(IDC_PDD_CALCULATE, OnCalculate)
   ON_BN_CLICKED(IDC_PDD_EXIT, OnExit)

END_MESSAGE_MAP()


// PopulateAdjacencyDlg message handlers

BOOL PopulateAdjacencyDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   int layerCount = (int) m_pMap->m_mapLayerArray.GetCount();

   CString msg;
   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = m_pMap->m_mapLayerArray.GetAt(i).pLayer;

      if ( pLayer->GetType() == LT_POLYGON )
         {
         m_targetLayerCombo.AddString( pLayer->m_name );
         m_sourceLayerCombo.AddString( pLayer->m_name );
         }
      }

   m_sourceLayerCombo.SetCurSel( 0 );
   m_targetLayerCombo.SetCurSel( 0 );

   CStringArray strArray;
   MapLayer *pLayer = m_pMap->m_mapLayerArray.GetAt(0).pLayer;
   pLayer->LoadFieldStrings( strArray );

   int fieldCount = (int) strArray.GetCount();
   for ( int i=0; i < fieldCount; i++ )
      m_targetFieldCombo.AddString( strArray.GetAt(i) );
   
   m_targetFieldCombo.SetCurSel( 0 );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void PopulateAdjacencyDlg::OnCbnSelchangePapTargetlayer()
   {
   int index = m_targetLayerCombo.GetCurSel();
   CString layerName;
   m_targetLayerCombo.GetLBText( index, layerName );
   MapLayer *pLayer = m_pMap->GetLayer( layerName );
   ASSERT( pLayer != NULL );

   CStringArray strArray;
   pLayer->LoadFieldStrings( strArray );

   m_targetFieldCombo.ResetContent();

   int fieldCount = (int) strArray.GetCount();
   for ( int i=0; i < fieldCount; i++ )
      m_targetFieldCombo.AddString( strArray.GetAt(i) );

   m_targetFieldCombo.SetCurSel( 0 );
   }


void PopulateAdjacencyDlg::OnCalculate()
   {
   CString layerName;
   m_sourceLayerCombo.GetWindowText( layerName );
   MapLayer *pSourceLayer = m_pMap->GetLayer( layerName );

   if ( pSourceLayer == NULL )
      {
      Report::WarningMsg( "You must select a layer to calculate the adjacencies from" );
      return;
      }
   
   m_targetLayerCombo.GetWindowText( layerName );
   MapLayer *pTargetLayer = m_pMap->GetLayer( layerName );

   if ( pTargetLayer == NULL )
      {
      Report::WarningMsg( "You must select a target layer to populate with adcency indexes from teh source layer" );
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
         gpMain->SetStatusMsg( "Adding field to database..." );
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         col = pTargetLayer->m_pDbTable->AddField( field, TYPE_INT, width, decimals, true );
         }
      else
         return;
      }

   LAYER_TYPE sourceType = pSourceLayer->GetType();
   LAYER_TYPE targetType = pTargetLayer->GetType();

   // process query (if one is specified)
   CString query;
   m_query.GetWindowText( query );

   query.Trim();
   bool selectedOnly = true;
   if ( query.IsEmpty() )
      selectedOnly = false;
   else
      {
      MapLayer *pOldLayer = gpModel->m_pQueryEngine->m_pMapLayer;
      gpModel->m_pQueryEngine->m_pMapLayer = pTargetLayer;
      Query *pQuery = gpModel->m_pQueryEngine->ParseQuery( query, 0, "Populate Adjacency Setup" );
      if ( pQuery == NULL )
         {
         Report::ErrorMsg( "Unable to parse query" );
         gpModel->m_pQueryEngine->m_pMapLayer = pOldLayer;
         return;
         }

      if ( gpCellLayer->GetSpatialIndex() == NULL )
         gpCellLayer->CreateSpatialIndex( NULL, 10000, 500, SIM_NEAREST );

      gpModel->m_pQueryEngine->SelectQuery( pQuery, true );
      gpModel->m_pQueryEngine->m_pMapLayer = pOldLayer;
      }

   CWaitCursor c;

   // at this point,
   //   source layer = szuline
   //   target layer = fcurrent
   //   targetfield = ???

   // basic idea - for each idu in the target layer that is part of the query, 
   //   1) find all polys in the source layer that are adjacent to the target idu,
   //   2) compute the longest distance of adjacency from the set of source polys
   //   3) store the index of the source polygon in the target field of the target layer

   // Step 1.
   int adjIndexArray[ 100 ];
   float distArray[ 100 ];

   pTargetLayer->SetColData( col, -1, true );

   if ( selectedOnly )
      {
      for ( int i=0; i < pTargetLayer->GetSelectionCount(); i++ )
         {
         int index = pTargetLayer->GetSelection( i );     // get the index of the selected polygon
         Poly *pTargetPoly = pTargetLayer->GetPolygon( index );

         int count = pTargetLayer->GetNearbyPolys( pTargetPoly, adjIndexArray, distArray, 100, 0.0f, SIM_NEAREST, pSourceLayer );
         
         // adjIndexArray now contains all adjacent polygons - find longest adjacency 
         int   longestIndex = -1;
         float longestEdge  = -1.0f;

         for ( int j=0; j < count; j++ )
            {
            float adjLength = pTargetLayer->ComputeAdjacentLength( pTargetPoly, pSourceLayer->GetPolygon( adjIndexArray[ j ] ) );

            if ( adjLength > longestEdge )
               {
               longestEdge = adjLength;
               longestIndex = adjIndexArray[ j ];
               }
            }  // end of: for ( j < count ); iterating through found source polys

         // longest adjacent edge found - store it in the target field
         pTargetLayer->SetData( index, col, longestIndex );
         }  // end of: for ( i < pTargetLayer->GetSelectionCount() ) 
      }
   else
      {}

   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( FALSE, "dbf", pTargetLayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "DBase Files|*.dbf|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      pTargetLayer->SaveDataDB( filename ); // uses DBase/CodeBase
      gpDoc->SetChanged( 0 );
      }
   else
      gpDoc->SetChanged( CHANGED_COVERAGE );

   _chdir( cwd );
   free( cwd );
   }


void PopulateAdjacencyDlg::OnExit()
   {
   OnOK();
   }
