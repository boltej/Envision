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
// NetworkTopologyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NetworkTopologyDlg.h"

#include "MapPanel.h"

#include <map.h>
#include <maplayer.h>
#include <DirPlaceholder.h>


extern MapPanel *gpMapPanel;
extern MapLayer *gpCellLayer;


int NTMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );


// NetworkTopologyDlg dialog

IMPLEMENT_DYNAMIC(NetworkTopologyDlg, CDialog)

NetworkTopologyDlg::NetworkTopologyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(NetworkTopologyDlg::IDD, pParent)
   , m_pTree( NULL )
   { }


NetworkTopologyDlg::~NetworkTopologyDlg()
{
if ( m_pTree != NULL )
   delete m_pTree;
}


void NetworkTopologyDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LAYERS, m_layers);
DDX_Control(pDX, IDC_COLTONODE, m_colToNode);
DDX_Control(pDX, IDC_COLNODE, m_colNodeID);
DDX_Control(pDX, IDC_COLTREEID, m_colTreeID);
DDX_Control(pDX, IDC_COLORDER2, m_colOrder);
DDX_Control(pDX, IDC_COLREACHINDEX, m_colReachIndex);
DDX_Control(pDX, IDC_POPULATEORDER, m_populateOrder);
DDX_Control(pDX, IDC_POPULATEREACHINDEX, m_populateReachIndex);
DDX_Control(pDX, IDC_LABELNODESGENERATED, m_nodesGenerated);
DDX_Control(pDX, IDC_LABELSINGLENODEREACHES, m_singleNodeReaches);
DDX_Control(pDX, IDC_LABELPHANTOMSGENERATED, m_phantomNodesGenerated);
DDX_Control(pDX, IDC_FLOWDIRGRID, m_flowDirGrid);
DDX_Control(pDX, IDC_PROGRESS, m_progress);
DDX_Control(pDX, IDC_PROGRESSTEXT, m_progressText);
   }


BEGIN_MESSAGE_MAP(NetworkTopologyDlg, CDialog)
   ON_BN_CLICKED(IDC_GENERATE, &NetworkTopologyDlg::OnBnClickedGenerate)
   ON_CBN_SELCHANGE(IDC_LAYERS, &NetworkTopologyDlg::OnCbnSelchangeLayers)
   ON_BN_CLICKED(IDC_POPULATEORDER, &NetworkTopologyDlg::OnBnClickedPopulateorder)
   ON_BN_CLICKED(IDC_USEATTR, &NetworkTopologyDlg::OnBnClickedUseattr)
   ON_BN_CLICKED(IDC_USEVERTICES, &NetworkTopologyDlg::OnBnClickedUsevertices)
   ON_BN_CLICKED(IDC_BROWSE, &NetworkTopologyDlg::OnBnClickedBrowse)
END_MESSAGE_MAP()


BOOL NetworkTopologyDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   // load up source dialog
   Map *pMap = gpMapPanel->m_pMap;

   if ( pMap == NULL )
       return TRUE;

   int layerCount = pMap->GetLayerCount();

   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );

      if ( pLayer->GetType() == LT_LINE )
         m_layers.AddString( pLayer->m_name );
      }

   if ( m_layers.GetCount() == 0 )
      {
      Report::ErrorMsg( _T("No line coverages are available to build network topology on.  Please Add these to your Project (.envx) file and restart Envision to complete this operation" ));
      EndDialog( IDCANCEL );
      return FALSE;
      }

   m_layers.SetCurSel( 0 );
   
   LoadFields();
   if ( gpCellLayer->GetFieldCol( _T( "Rch_Index" ) ) < 0 )
      m_colReachIndex.AddString( _T("Rch_Index") );
   
   for ( int i=0; i < gpCellLayer->GetFieldCount(); i++ )
      m_colReachIndex.AddString( gpCellLayer->GetFieldLabel( i ) );
   
   m_colReachIndex.SetCurSel( 0 );

   CheckDlgButton( IDC_USEATTR, 1 );

   UpdateInterface();
   	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE   
   }


void NetworkTopologyDlg::LoadFields() 
   {
   m_colToNode.ResetContent();
   m_colNodeID.ResetContent();
   m_colOrder.ResetContent();
   m_colTreeID.ResetContent();

   int layerIndex = m_layers.GetCurSel();

   CString layerName;
   m_layers.GetLBText( layerIndex, layerName );

   // load up source dialog
   Map *pMap = gpMapPanel->m_pMap;
   if ( pMap == NULL )
       return;

   MapLayer *pLayer = pMap->GetLayer( layerName );
   ASSERT( pLayer != NULL );
   
   if ( pLayer->GetFieldCol( _T("TreeID")) < 0 )
      m_colTreeID.AddString( _T("TreeID") );
   
   if ( pLayer->GetFieldCol( _T("Order")) < 0 )
      m_colOrder.AddString ( _T("Order") );

   if ( pLayer->GetFieldCol( _T("Rch_Indec")) < 0 )
      m_colOrder.AddString ( _T("Order") );

   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      {
      m_colToNode.AddString  ( pLayer->GetFieldLabel( i ) );
      m_colNodeID.AddString  ( pLayer->GetFieldLabel( i ) );
      m_colOrder.AddString   ( pLayer->GetFieldLabel( i ) );
      m_colTreeID.AddString  ( pLayer->GetFieldLabel( i ) );
      }

   m_colNodeID.SetCurSel( 0 );
   m_colToNode.SetCurSel( 0 );
   m_colOrder.SetCurSel( 0 );
   m_colTreeID.SetCurSel( 0 );

   int col = pLayer->GetFieldCol( _T("FNODE_") );
   if ( col > 0 )
      m_colNodeID.SetCurSel( col );

   col = pLayer->GetFieldCol( _T("TNODE_") );
   if ( col > 0 )
      m_colToNode.SetCurSel( col );

   }


// NetworkTopologyDlg message handlers

void NetworkTopologyDlg::OnBnClickedGenerate()
   {
   int layerIndex = m_layers.GetCurSel();
   CString layerName;
   m_layers.GetLBText( layerIndex, layerName );

   // load up source dialog
   Map *pMap = gpMapPanel->m_pMap;
   if ( pMap == NULL )
       return;

   MapLayer *pLayer = pMap->GetLayer( layerName );
   ASSERT( pLayer != NULL );

   int colToNode   = this->m_colToNode.GetCurSel();
   int colEdge     = this->m_colNodeID.GetCurSel();

   if ( IsDlgButtonChecked( IDC_USEEDGEID ) == 0  )
      colEdge = -1;

   if ( colToNode < 0 )
      {
      AfxMessageBox( _T("You must select a 'To' column" ), MB_OK );
      return;
      }

   m_progressText.SetWindowText( _T("Building Tree..." ) );

   CWaitCursor c;

   if ( m_pTree != NULL )
      delete m_pTree;

   m_pTree = new ReachTree; //NetworkTree;

   //m_pTree->BuildTree( pLayer, colEdge, colToNode );

   CString label;
   label.Format( "# of roots generated: %i", m_pTree->GetRootCount() );
   GetDlgItem( IDC_LABELROOTSGENERATED )->SetWindowText( label );

   label.Format( "# of non-branched reaches: %i", m_pTree->GetUnbranchedCount() );
   GetDlgItem( IDC_LABELSINGLENODEREACHES )->SetWindowText( label );

   label.Format( "# of phantom nodes generated: %i", m_pTree->GetPhantomCount() );
   GetDlgItem( IDC_LABELPHANTOMSGENERATED )->SetWindowText( label );
   
   pMap->InstallNotifyHandler( NTMapProc, (LONG_PTR) this );

   if ( IsDlgButtonChecked( IDC_POPULATEORDER ) )
      {
      m_progressText.SetWindowText( _T("Populating Order..." ) );
      int colOrder = m_colOrder.GetCurSel();
      CString orderCol;
      m_colOrder.GetLBText( colOrder, orderCol );
      colOrder = pLayer->GetFieldCol( orderCol );
          
      if ( colOrder < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         colOrder = pLayer->m_pDbTable->AddField( _T( "Order" ), TYPE_INT, width, decimals, true );
         }

      m_pTree->PopulateOrder( colOrder, true );
      }

   if ( IsDlgButtonChecked( IDC_POPULATETREEID ) )
      {
      m_progressText.SetWindowText( _T("Populating Tree ID..." ) );
      int colTreeID = m_colTreeID.GetCurSel();
      CString treeIDCol;
      m_colTreeID.GetLBText( colTreeID, treeIDCol );
      colTreeID = pLayer->GetFieldCol( treeIDCol );

      if ( colTreeID < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         colTreeID = pLayer->m_pDbTable->AddField( _T( "TreeID" ), TYPE_INT, width, decimals, true );
         }

      m_pTree->PopulateTreeID( colTreeID );
      }

   if ( IsDlgButtonChecked( IDC_POPULATEREACHINDEX ) )
      {
      m_progressText.SetWindowText( _T("Populating Reach Index..." ) );

      int colReachIndex = m_colReachIndex.GetCurSel();
      CString reachIndexField;
      m_colReachIndex.GetLBText( colReachIndex, reachIndexField );
      colReachIndex = pLayer->GetFieldCol( reachIndexField );
      
      if ( colReachIndex < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         colReachIndex = pLayer->m_pDbTable->AddField( _T( "RCH_INDEX" ), TYPE_INT, width, decimals, true );      
         }

      CString flowDirGrid;
      m_flowDirGrid.GetWindowText( flowDirGrid );

      Map *pMap = gpCellLayer->GetMapPtr();

      MapLayer *pGrid = pMap->GetLayerFromPath( flowDirGrid );
      if ( pGrid == NULL )
         pGrid = pMap->AddGridLayer( flowDirGrid, DOT_INT );

      if ( pGrid != NULL )
         gpCellLayer->PopulateReachIndex( pGrid, pLayer, colReachIndex, 0 );
      else
         MessageBox( "Unable to load flow direction grid", MB_OK );
      }

   CString finished;
   finished.Format( "Finished - %i nodes processed", m_pTree->GetReachCount() );
   m_progressText.SetWindowText( finished );

   pMap->RemoveNotifyHandler( NTMapProc, (LONG_PTR) this );

   CString nodesGenerated;
   nodesGenerated.Format( "# of roots generated: %i", m_pTree->GetRootCount() );
   m_nodesGenerated.SetWindowText( nodesGenerated );

   CString singleNodeReaches;
   singleNodeReaches.Format( "# of single node reaches: %i", m_pTree->GetUnbranchedCount() );
   m_singleNodeReaches.SetWindowText( singleNodeReaches );

   CString phantomNodesGenerated;
   phantomNodesGenerated.Format( "# of phantom node generated: %i", m_pTree->GetPhantomCount() );
   m_phantomNodesGenerated.SetWindowText( phantomNodesGenerated );
   }


void NetworkTopologyDlg::OnCbnSelchangeLayers()
   {
   LoadFields();
   }


void NetworkTopologyDlg::OnBnClickedPopulateorder()
   {
   // TODO: Add your control notification handler code here
   }

void NetworkTopologyDlg::OnBnClickedUseattr()
   {
   UpdateInterface();
   }

void NetworkTopologyDlg::OnBnClickedUsevertices()
   {
   UpdateInterface();
   }


void NetworkTopologyDlg::OnBnClickedBrowse()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "flt", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "FLT files|*.flt|BIL files|*.bil|Ascii Grid files|*.asc|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      m_flowDirGrid.SetWindowText( dlg.GetPathName() );
   }


void NetworkTopologyDlg::UpdateInterface( void )
   {
   BOOL useAttr = IsDlgButtonChecked( IDC_USEATTR );

   GetDlgItem( IDC_LABELTONODE )->EnableWindow( useAttr );
   GetDlgItem( IDC_COLTONODE   )->EnableWindow( useAttr );
   GetDlgItem( IDC_USEEDGEID   )->EnableWindow( useAttr );
   GetDlgItem( IDC_LABELEDGEID )->EnableWindow( useAttr );
   GetDlgItem( IDC_LABELNODE   )->EnableWindow( useAttr );
   GetDlgItem( IDC_COLNODE     )->EnableWindow( useAttr );
   }



int NTMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra )
   {
   NetworkTopologyDlg *pDlg = (NetworkTopologyDlg*) extra;

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

