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
// ResultsTree.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include ".\resultstree.h"
#include "ResultsPanel.h"
#include "resource.h"
#include "EnvView.h"
#include <EnvModel.h>
#include <DataManager.h>
#include <FolderDlg.h>
#include <DirPlaceHolder.h>

extern CEnvView     *gpView;
extern ResultsPanel *gpResultsPanel;
extern EnvModel     *gpModel;

// ResultsTreeCtrl

IMPLEMENT_DYNAMIC(ResultsTreeCtrl, CTreeCtrl)
ResultsTreeCtrl::ResultsTreeCtrl()
{
}

ResultsTreeCtrl::~ResultsTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(ResultsTreeCtrl, CTreeCtrl)
   ON_WM_RBUTTONDOWN()
   ON_WM_LBUTTONDBLCLK()
   ON_WM_KEYDOWN()
END_MESSAGE_MAP()


// ResultsTreeCtrl message handlers


void ResultsTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
   {
   //CTreeCtrl::OnRButtonDown(nFlags, point);

   UINT uFlags;
   HTREEITEM hItem = HitTest( point, &uFlags);

   if ((hItem != NULL) && (TVHT_ONITEM & uFlags))
      {
      // what kind of item is it?
      // Note: item data LOWORD=category, HIWORD=run for parent nodes, RESULT_TYPE for children nodes

      //DWORD_PTR itemData = GetItemData( hItem );
      //short category = LOWORD( itemData );
      ResultNode *pNode = (ResultNode*) GetItemData( hItem );

      ASSERT( pNode != NULL );

      CMenu menu;
      menu.CreatePopupMenu();

      switch( pNode->m_category )
         {
         case VC_RUN:         // its a run
         case VC_MULTIRUN:    // or a multirun  
            m_lastItem = hItem;
            menu.AppendMenu( MF_STRING, 40110, "Export Data" );
            break;

         case VC_MODEL_OUTPUTS:     // root node for a models output(s)
            m_lastItem = hItem;
            menu.AppendMenu( MF_STRING, 40120, "Export Data" );
            if ( gpModel->m_currentRun > 0 )
               menu.AppendMenu( MF_STRING, 40121, "Export Data for Multiple Runs" );
            break;

         case VC_OTHER:       // map/graph/view/table root
            return;

         case VC_MAP:     // map
         case VC_MULTIRUN_MAP:
            m_lastItem = hItem;
            menu.AppendMenu( MF_STRING, 40100, "Add Map" );
            menu.AppendMenu( MF_STRING, 40101, "Add Change Map" );
            if ( gpModel->m_currentRun > 0 )
               {
               menu.AppendMenu( MF_STRING, 40102, "Compare Maps Across Multiple Runs" );
               menu.AppendMenu( MF_STRING, 40103, "Compare Change Maps Across Multiple Runs" );
               }
            else
               {
               menu.AppendMenu( MF_STRING | MF_GRAYED, 40102, "Compare Graphs Across Multiple Runs" );
               menu.AppendMenu( MF_STRING | MF_GRAYED, 40103, "Compare Change Graphs Across Multiple Runs" );
               }

            menu.AppendMenu( MF_STRING, 40110, "Export Data" );
            break;

         case VC_GRAPH:     // graph
         case VC_MULTIRUN_GRAPH:
            m_lastItem = hItem;
            menu.AppendMenu( MF_STRING, 41100, "Add Graph" );
            //menu.AppendMenu( MF_STRING, 41101, "Add Cumulative Graph" );

            if ( gpModel->m_currentRun > 0 )
               {
               menu.AppendMenu( MF_STRING, 41105, "Compare Graphs Across Multiple Runs" );
               //menu.AppendMenu( MF_STRING, 41106, "Compare Cumulative Graphs Across Multiple Runs" );
               }
            else
               {
               menu.AppendMenu( MF_STRING | MF_GRAYED, 41105, "Compare Graphs Across Multiple Runs" );
               //menu.AppendMenu( MF_STRING | MF_GRAYED, 41106, "Compare Cumulative Graphs Across Multiple Runs" );
               }

            menu.AppendMenu( MF_STRING, 41110, "Export Data" );
            break;

         case VC_VIEW:     // view
         case VC_MULTIRUN_VIEW:     // view
            m_lastItem = hItem;
            menu.AppendMenu( MF_STRING, 42100, "Add View" );
            break;

         case VC_DATATABLE:     // DataTable
         case VC_MULTIRUN_DATATABLE:
            m_lastItem = hItem;
            menu.AppendMenu( MF_STRING, 43100, "Add DataTable" );
            menu.AppendMenu( MF_STRING, 43110, "Export Data" );
            break;
         }

      // convert points to screen coords
      ClientToScreen( &point );

      // display and track the menu
      menu.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, this, NULL );
      }
   }


BOOL ResultsTreeCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
   {
   if ( HIWORD( wParam ) == 0 )
      {
      ResultNode *pNode = (ResultNode*) GetItemData( m_lastItem );
      int run = pNode->m_run;     //(int) HIWORD( parentItemData );
      RESULT_TYPE type = pNode->m_type;
      //int extra  = (int) pNode->m_extra;
      //DWORD_PTR itemData = GetItemData( m_lastItem );
      //short category = LOWORD( itemData );
      //int resultInfoOffset = (int) HIWORD( itemData );
      //int type = (int) gpResultsPanel->m_resultInfo[ resultInfoOffset ].type;
      //int extra = (int) gpResultsPanel->m_resultInfo[ resultInfoOffset ].extra;

      int cmd = LOWORD( wParam );

      switch( cmd )
         {
         case 40100:  // Add map/graph/view/datatable
         case 41100:
         case 42100:
         case 43100:
            {
            // get run info from parent node
            //HTREEITEM hParentItem = GetParentItem( m_lastItem );
            //DWORD_PTR parentItemData = GetItemData( hParentItem );

            switch( pNode->m_category )
               {
               case VC_MAP:
               case VC_MULTIRUN_MAP:
                  gpResultsPanel->AddMapWnd( pNode ); //type, run, extra );
                  break;

               case VC_GRAPH:
               case VC_MULTIRUN_GRAPH:
                  gpResultsPanel->AddGraphWnd( pNode ); // type, run, extra );
                  break;

               case VC_VIEW:  
               case VC_MULTIRUN_VIEW:
                  gpResultsPanel->AddViewWnd ( pNode ); // type, run, extra );  
                  break;

               case VC_DATATABLE:  
               case VC_MULTIRUN_DATATABLE:
                  gpResultsPanel->AddDataTableWnd( pNode ); //type, run, extra );
                  break;
               }
            }
            break;
   
         case 40101:    // "Add Change Map"
            gpResultsPanel->AddChangeMapWnd( pNode );
            break;

         case 40102:    // "Compare Maps Across Multiple Runs"
         case 40103:    // "Compare Difference Maps Across Multiple Runs"
            {
            DataManager *pDM = gpModel->m_pDataManager;
            int runCount = pDM->GetRunCount();
            for ( int i=0; i < runCount; i++ )
               {
               pNode->m_run = i;
               if ( cmd == 40102 )
                  gpResultsPanel->AddMapWnd( pNode ); // type, run, extra );
               else
                  gpResultsPanel->AddChangeMapWnd( pNode );
               }
            }
            gpResultsPanel->m_pResultsWnd->Tile();
            break;

         case 41105: // compare results across multiple runs
            {
            switch( pNode->m_category )
               {
               //case 1:
               //case 5:  gpResultsPanel->AddMap  ( (RESULT_TYPE) type, run, extra );  break;
               case VC_GRAPH:
               //case 6:
                  // note: type is the VT_GRAPH_XXX type, extra is ptr to a EnvEvaluator structure
                  gpResultsPanel->AddCrossRunGraph(  pNode ); //type, extra );
                  break;
               //case 3:  
               //case 7:  gpResultsPanel->AddView ( (RESULT_TYPE) type, run, extra );  break;
               //case 4:  
               //case 8:  gpResultsPanel->AddDataTable( (RESULT_TYPE) type, run, extra );  break;
               }
            }
            break;

         case 40110: // export data (from a RUN)
         case 41110:  
         case 42110:
         case 43110:
         case 40120:  // export data (from MODEL OUTPUTS)
         case 40121:  // export multirun data (from MODEL OUTPUTS)
            {
            switch( pNode->m_category )
               {
               // model outputs node:
               case VC_MODEL_OUTPUTS:
                  if ( cmd == 40120 )
                     gpResultsPanel->ExportDataTable( pNode );
                  else if ( cmd == 40121 )
                     gpResultsPanel->ExportModelOutputs( -1, true );
                  break;

               // maps
               case VC_MAP:
               case VC_MULTIRUN_MAP:
                  gpResultsPanel->ExportDataTable( pNode ); //type, run );
                  break;

               // graph
               case VC_GRAPH:  
               case VC_MULTIRUN_GRAPH:
                  gpResultsPanel->ExportDataTable( pNode ); // type, run );
                  break;

               // view???
               // datatable
               case VC_DATATABLE:  
               case VC_MULTIRUN_DATATABLE:  
                  gpResultsPanel->ExportDataTable( pNode ); // type, run );
                  break;

               case VC_RUN:    // export run data
               case VC_MULTIRUN:    // export multirun data
                  {
                  DirPlaceholder d;

                  CFolderDialog dlg( _T( "Select Folder" ), NULL, gpView, BIF_EDITBOX | BIF_NEWDIALOGSTYLE );

                  if ( dlg.DoModal() == IDOK )
                     {
                     CString path = dlg.GetFolderPath();

                     if ( !path.IsEmpty() )
                        if ( path[ path.GetLength()-1 ] != '\\' )
                           path += "\\";

                     if ( pNode->m_category == VC_RUN )
                        gpModel->m_pDataManager->ExportRunData( path, run );
                     else
                        gpModel->m_pDataManager->ExportMultiRunData( path, run );
                     }
                  }
               }  // end of: switch( category )
            }  // end of: switch( //export data )
            break;
         }
      }

   gpResultsPanel->m_pResultsWnd->Tile();
   return CTreeCtrl::OnCommand(wParam, lParam);
   }


void ResultsTreeCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
   {
   UINT uFlags;
   HTREEITEM hItem = HitTest( point, &uFlags);

   if ((hItem != NULL) && (TVHT_ONITEM & uFlags))
      OpenItem( hItem );

   CTreeCtrl::OnLButtonDblClk(nFlags, point);
   }


void ResultsTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
   {
   if ( nChar == VK_RETURN )
      {
      HTREEITEM hItem = GetSelectedItem();

      if ( hItem != NULL )
         OpenItem( hItem );
      }

   CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
   }


bool ResultsTreeCtrl::OpenItem( HTREEITEM hItem )
   {
   // what kind of item is it?
   ResultNode *pNode = (ResultNode*) GetItemData( hItem );

   RESULT_TYPE type = pNode->m_type;
   //int extra = pNode->m_extra;
   int run = pNode->m_run;
   //DWORD_PTR itemData = GetItemData( hItem );
   //short category = LOWORD( itemData );
   //int resultInfoOffset = (int) HIWORD( itemData );
   //int type = (int) gpResultsPanel->m_resultInfo[ resultInfoOffset ].type;
   //int extra = (int) gpResultsPanel->m_resultInfo[ resultInfoOffset ].extra;
   
   //HTREEITEM hParentItem = GetParentItem( hItem );
   //DWORD_PTR parentItemData = GetItemData( hParentItem );
   //int run = (int) HIWORD( parentItemData );
   m_lastItem = hItem;
   switch( pNode->m_category )
      {
      case VC_MAP:
      case VC_MULTIRUN_MAP:
         gpResultsPanel->AddMapWnd( pNode ); // type, run, extra );
         break;   // extra is gpDoc->m_fieldInfo offset
     
      case VC_GRAPH:
      case VC_MULTIRUN_GRAPH:
         gpResultsPanel->AddGraphWnd( pNode ); // type, run, extra );
         break;

      case VC_VIEW:  
      case VC_MULTIRUN_VIEW:
         gpResultsPanel->AddViewWnd( pNode ); // type, run, extra );
         break;

      case VC_DATATABLE:
      case VC_MULTIRUN_DATATABLE:
         gpResultsPanel->AddDataTableWnd( pNode ); // type, run, extra );
         break;
      }

   gpResultsPanel->m_pResultsWnd->Tile();
   
   return true;
   }

// ResultsTree

IMPLEMENT_DYNCREATE(ResultsTree, CWnd)
ResultsTree::ResultsTree()
: m_buttonHeight( 32 )
, m_editHeight( 24 )
{
}

ResultsTree::~ResultsTree()
   {
   //DeleteObject( m_hRewind );
   //DeleteObject( m_hPause  );
   //DeleteObject( m_hPlay );
   //m_font.DeleteObject();
   //m_filterFont.DeleteObject();
   //m_noFilterFont.DeleteObject();
   }


BEGIN_MESSAGE_MAP(ResultsTree, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
   ON_COMMAND_RANGE( 200, 300, OnButton )
   ON_EN_CHANGE(190, OnChangeFilter )
	ON_EN_SETFOCUS(190, OnSetFocusFilter )
   ON_NOTIFY(TVN_ITEMEXPANDED, 100, &ResultsTree::OnTvnItemexpanded)
//   ON_WM_CTLCOLOR()
   ON_WM_DESTROY()
END_MESSAGE_MAP()



// ResultsTree message handlers


int ResultsTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_font.CreatePointFont( 95, "Arial" );
   m_filterFont.CreatePointFont( 100, "Arial" );

   LOGFONT logFont;
   m_filterFont.GetLogFont( &logFont );
   logFont.lfItalic = TRUE;
   m_noFilterFont.CreateFontIndirect( &logFont );

   RECT rect;
   GetClientRect( &rect );
   int width  = rect.right;
   int height = rect.bottom;

   rect.bottom -= ( m_buttonHeight + m_editHeight );

   m_tree.Create( WS_CHILD | WS_VISIBLE | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_HASLINES, rect, this, 100 );

   // add filter
   rect.top = rect.bottom + 1;
   rect.bottom = rect.top + m_editHeight;
   m_filter.Create( WS_CHILD | WS_VISIBLE | WS_BORDER, rect, this, 190 );  // edit
   m_filter.SetFont( &m_filterFont );
   //m_filter.SetFont( &m_noFilterFont, 0 );
   //m_filter.SetWindowText( "--Search--" );

   // [ << ][ :: ][ > ]
   // [  Options...   ]
   m_hRewind = (HBITMAP) LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDB_REWIND ), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );
   m_hPause  = (HBITMAP) LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDB_PAUSE  ), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );
   m_hPlay   = (HBITMAP) LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDB_PLAY   ), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS );

   //m_iRewind = (HICON) LoadIcon( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDI_REWIND ) );

   rect.right = width/3;
   rect.top = rect.bottom + 1;
   rect.bottom = rect.top + m_buttonHeight;
   m_rewind.Create( "Rewind", WS_CHILD | WS_VISIBLE | BS_BITMAP | BS_CENTER, rect, this, 200 );
   m_rewind.SetBitmap( m_hRewind );
   m_rewind.SetTooltip( _T( "Reset the result windows to the start of the simulations" ) );

   //m_rewind.SetIcon( m_iRewind );
   
   rect.left = rect.right+1;
   rect.right = rect.left + width/3; 
   m_pause.Create( "Pause",  WS_CHILD | WS_VISIBLE | BS_BITMAP, rect, this, 201 );
   m_pause.SetBitmap( m_hPause );
   m_pause.SetTooltip( _T( "Pause the current replay of the simulations" ) );

   rect.left = rect.right+1;
   rect.right = width;
   m_play.Create( "Run",  WS_CHILD | WS_VISIBLE | BS_BITMAP, rect, this, 202 );
   m_play.SetBitmap( m_hPlay );
   m_play.SetTooltip( _T( "Start/Resume the replay of the simulations" ) );

   
   //rect.left = 0;
   //rect.right = width;
   //rect.top = rect.bottom+1;
   //rect.bottom = height;
   //m_options.Create( "Options...",  WS_CHILD | WS_VISIBLE, rect, this, 210 );

   m_tree.SetFont( &m_font );
   //m_options.SetFont( &m_font );
   
   return 0;
   }


void ResultsTree::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);

   RECT rect;
   GetClientRect( &rect );
   int width  = rect.right;
   int height = rect.bottom;

   // tree control
   rect.bottom -=  ( m_buttonHeight + m_editHeight );
   m_tree.MoveWindow( &rect );
   
   // filter
   rect.top = rect.bottom + 1;
   rect.bottom = rect.top + m_editHeight;
   m_filter.MoveWindow( &rect );

   // buttons
   rect.right = width/3;
   rect.top = rect.bottom + 1;
   rect.bottom = rect.top + m_buttonHeight;
   m_rewind.MoveWindow( &rect );

   rect.left = rect.right+1;
   rect.right = rect.left + width/3; 
   m_pause.MoveWindow( &rect );
   
   rect.left = rect.right+1;
   rect.right = width;
   m_play.MoveWindow( &rect );
   
   //rect.left = 0;
   //rect.right = width;
   //rect.top = rect.bottom+1;
   //rect.bottom = height;
   //m_options.MoveWindow( &rect );
   }


void ResultsTree::OnButton( UINT nID )
   {
   // 202
   //MessageBox( "yep" );

   switch ( nID )
      {
      case 200:  //rewind
         gpResultsPanel->m_pResultsWnd->ReplayReset();
         break;

      case 201:  // pause
         gpResultsPanel->m_pResultsWnd->ReplayStop();
         break;

      case 202:  // play
         gpResultsPanel->m_pResultsWnd->ReplayPlay();
         break;

      //case 210:   // options
      //   break;
      }
   }


void ResultsTree::OnTvnItemexpanded(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

   HTREEITEM hItem = pNMTreeView->itemNew.hItem;

   ResultNode *pResultNode = (ResultNode*) gpResultsPanel->m_pTree->GetItemData( hItem );

   if ( pNMTreeView->action == TVE_COLLAPSE )
      pResultNode->m_isExpanded = false;      
   else if (pNMTreeView->action == TVE_EXPAND )
      pResultNode->m_isExpanded = true;

   *pResult = 0;
   }


 void ResultsTree::OnDestroy()
   {
   CWnd::OnDestroy();

   DeleteObject( m_hRewind );
   DeleteObject( m_hPause  );
   DeleteObject( m_hPlay );

   // Free the space allocated for the background brush
   m_font.DeleteObject();
   m_filterFont.DeleteObject();
   m_noFilterFont.DeleteObject();
   }  


 void ResultsTree::OnChangeFilter( void )
   {
   m_filter.GetWindowText(m_filterStr );

   m_filterStr.MakeLower();

   if ( m_filterStr.CompareNoCase( "--search--") == 0 )
      m_filterStr.Empty();
   //else if ( m_filterStr.IsEmpty() )
   //   m_filter.SetWindowText( "--Search--" );

   //if ( m_filterStr.IsEmpty() )
   //   m_filter.SetFont( &m_noFilterFont );
   //else
   //   m_filter.SetFont( &m_filterFont );
   
   if ( gpResultsPanel == NULL || gpResultsPanel->m_pTree == NULL )
      return;

   //gpResultsPanel->m_pTree->RedrawWindow();   
   gpResultsPanel->PopulateTreeCtrl();
   }

void ResultsTree::OnSetFocusFilter()
   {
   //m_filter.SetSel( 0, m_filter.GetWindowTextLength() );
   }


 // called whenever filter control is redrawn
 HBRUSH ResultsTree::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
   {
   //switch (nCtlColor) 
   //   {
   //   case CTLCOLOR_EDIT:
   //   case CTLCOLOR_MSGBOX:
   //      {
   //      //if ( pWnd == (CWnd*) &m_filter )
   //      
   //      HBRUSH hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);        
   //      
   //      if ( m_filterStr.IsEmpty() )
   //         pDC->SetTextColor(RGB(180, 180, 180) );
   //      else
   //         pDC->SetTextColor( 0 );  // black
   //
   //      return hbr;
   //      }
   //
   //   default:
         return CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
   //   }
   }


