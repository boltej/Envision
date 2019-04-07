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
// MapFrame.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include "MapFrame.h"
#include "MapWnd.h"
#include "MapListWnd.h"
//#include "ActiveWnd.h"

#include "cellPropDlg.h"

#include <map.h>

#include <afxcview.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MapFrame


IMPLEMENT_DYNAMIC(MapFrame, CWnd)

MapFrame::MapFrame()
: m_pMap( NULL ),
  m_pMapWnd( NULL ),
  m_pMapList( NULL ),
  m_pCellPropDlg( NULL ),
  m_currentPoly( -1 )
   {  }


MapFrame::MapFrame( MapFrame &mapFrame )
:  m_pMapWnd( mapFrame.m_pMapWnd ),
   m_pMapList( NULL ),
   m_currentPoly( -1 )
   {
   if ( m_pMapWnd != NULL )
      delete m_pMapWnd;

   if ( m_pMapList != NULL )
      delete m_pMapList;
   }


MapFrame::~MapFrame()
{
if ( m_pMapWnd != NULL )
   delete m_pMapWnd;

if ( m_pMapList != NULL )
   delete m_pMapList;
}


BEGIN_MESSAGE_MAP(MapFrame, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_WM_ACTIVATE()
	ON_COMMAND(ID_LIB_CELLPROPERTIES, OnCellproperties)
	ON_COMMAND(ID_LIB_SHOWSAMEATTRIBUTE, OnShowSameAttribute)
   ON_COMMAND(ID_LIB_SHOWPOLYGONVERTICES, OnShowPolygonVertices)
   ON_COMMAND(ID_LIB_ZOOMTOPOLY, OnZoomToPoly)
	ON_COMMAND(ID_LIB_RECLASSIFYDATA, OnReclassifydata)
	ON_WM_MOUSEACTIVATE()
   ON_COMMAND_RANGE(40000, 60000, OnSelectField)
   ON_COMMAND_RANGE(30000, 39000, RouteToParent)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// MapFrame message handlers


int MapFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
   {
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   m_pMapList = new MapListWnd;
   m_pMapWnd  = new MapWindow;
   m_pMap     = m_pMapWnd->GetMap(); // new Map;
   m_splitterWnd.LeftRight( m_pMapList, m_pMapWnd );
   m_splitterWnd.Create( NULL, NULL, WS_VISIBLE, CRect(0,0,0,0), this, 101 );

   m_pMapList->m_pMapWnd = m_pMapWnd;

	return 0;
   }


void MapFrame::Update( void )
   {
   for ( int i=0; i < m_pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = m_pMap->GetLayer( i );

      if ( pLayer->IsVisible() && pLayer->m_pData != NULL )
         pLayer->ClassifyData();
      }

   m_pMapList->RedrawHistograms();  // redraws everyting

   CDC *pDC = m_pMapWnd->GetDC();
   m_pMapWnd->DrawMap( *pDC );
   ReleaseDC( pDC );
   }


void MapFrame::OnSize(UINT nType, int cx, int cy) 
   {
	CWnd::OnSize(nType, cx, cy);

   if ( IsWindow( m_splitterWnd.m_hWnd ) )
      {
      m_splitterWnd.MoveWindow( 0, 0, cx, cy, TRUE );
      }	
   }


void MapFrame::OnEditCopy() 
{
if ( m_pMapWnd != NULL )
   m_pMapWnd->CopyMapToClipboard( 0 );
}


void PopulateFieldsMenu( Map *pMap, MapLayer *pLayer, CMenu &mainMenu, bool addAdditionalLayers )
   {
   int drawLayerIndex = -1;
   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      if ( pMap->GetLayerFromDrawOrder( i ) == pLayer )
         {
         drawLayerIndex = i;
         break;
         }

   ASSERT( drawLayerIndex >= 0 );
   CMenu *pAdditionalFieldsMenu = NULL;

   // add any field not covered by field info structure to the bottom of the menu as a new menu
   if ( pLayer->m_includeData && pLayer->GetFieldCount() > 0
     && pLayer->GetFieldInfoCount( 0 ) != pLayer->GetFieldCount() )
      {
      pAdditionalFieldsMenu = new CMenu;
      pAdditionalFieldsMenu->CreatePopupMenu();
      }

   int fields = pLayer->GetFieldCount();

   // load up any field information registrered with the documents field info
   for ( int i=0; i < pLayer->GetFieldInfoCount( 1 ); i++ )
      {
      MAP_FIELD_INFO *pInfo = pLayer->GetFieldInfo( i );
      ASSERT( pInfo != NULL );

      // only use level 0 in the main menu
      if ( pInfo->pParent == NULL )
         {
         UINT flag = MF_STRING;
         UINT id = 0;

         // is this map_field_info a menu type? meaning it is used
         // as text with an associated submenu?  In that case, add it to
         // the main menu and create a submenu that will be populated later
         if ( pInfo->mfiType == MFIT_SUBMENU )
            {
            flag += MF_POPUP;

            if ( pInfo->pMenu != NULL )
               delete pInfo->pMenu;
            
            pInfo->pMenu = new CMenu;
            pInfo->pMenu->CreatePopupMenu();
            //HMENU hMenu = CreatePopupMenu();

            mainMenu.AppendMenu( flag, (UINT_PTR) pInfo->pMenu->m_hMenu,  (LPCTSTR) pInfo->label );
            }
         else  // it is for a field
            {
            // get column number if this is associated with a field
            int col = pLayer->GetFieldCol( pInfo->fieldname );
            if ( col >= 0 )  // column found?  Then add to menu
               {
               if ( col == pLayer->m_activeField )
                  flag |= MF_CHECKED;
               }

            // have field label, which menu to put it on?  if it isn't nested,add it to the 
            // main menu, otherwise figure out which submenu is needed and add it there.
            mainMenu.AppendMenu( flag, 40000 + (1000*drawLayerIndex) + col,  (LPCTSTR) pInfo->label );
            }   // end of: else (its a field menu item)
         }
      }

   // populate any submenus
   for ( int i=0; i < pLayer->GetFieldInfoCount( 1 ); i++ )
      {
      MAP_FIELD_INFO *pInfo = pLayer->GetFieldInfo( i );
      ASSERT( pInfo != NULL );

      // does this field info go on a submenu?
      if ( /*pInfo->level == 1 && */ pInfo->pParent != NULL )
         {
         // various checks
         if ( pInfo->mfiType == MFIT_SUBMENU )
            continue;

         // does the parent container field info/menu actually have a menu?
         //ASSERT ( pInfo->pParent->pMenu != NULL );
         if ( pInfo->pParent->pMenu == NULL )
            continue;
         
         UINT flag = MF_STRING;
         UINT id = 0;

         // get column number if this is associated with a field
         int col = pLayer->GetFieldCol( pInfo->fieldname );
         if ( col >= 0 )  // column found?  Then add to menu
            {
            if ( col == pLayer->m_activeField )
               flag |= MF_CHECKED;
         
            pInfo->pParent->pMenu->AppendMenu( flag, 40000 + (1000*drawLayerIndex) + col,  (LPCTSTR) pInfo->label );
            }   // end of: else (its a field menu item)
         }
      }

   // detach any submenus before deletion
   for ( int i=0; i < pLayer->GetFieldInfoCount( 1 ); i++ )
      {
      MAP_FIELD_INFO *pInfo = pLayer->GetFieldInfo( i );
      ASSERT( pInfo != NULL );

      if ( pInfo->pMenu != NULL )
         {
         pInfo->pMenu->Detach();
         delete pInfo->pMenu;
         pInfo->pMenu = NULL;
         }
      }

   // is there an additional fields menu needed?
   if ( pAdditionalFieldsMenu != NULL )
      {
      for ( int i=0; i < pLayer->GetFieldCount(); i++ )
         {
         CString fieldname = pLayer->GetFieldLabel( i );
        
         UINT flag = MF_STRING;
         if ( i == pLayer->GetActiveField() )
            flag |= MF_CHECKED;
         
         MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( fieldname );

         if ( pInfo == NULL )    // MAP_FIELD_INFO notdefined?
            pAdditionalFieldsMenu->AppendMenu( MF_STRING, 40000 + (1000*drawLayerIndex) + i, fieldname );
         }  

      mainMenu.AppendMenu( MF_SEPARATOR ); 
      mainMenu.AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) pAdditionalFieldsMenu->m_hMenu, "Additional Fields" );

      pAdditionalFieldsMenu->Detach();
      delete pAdditionalFieldsMenu;
      }

   // add additional layers if needed
   if ( addAdditionalLayers && pMap->GetLayerCount() > 1 )
      {
      mainMenu.AppendMenu( MF_SEPARATOR ); 
            
      for ( int k=0; k < pMap->GetLayerCount(); k++ )
         {
         MapLayer *pAddLayer = pMap->GetLayerFromDrawOrder( k );

         if ( pAddLayer != pLayer && pAddLayer->m_includeData )
            {
            CMenu additionalLayersMenu;
            additionalLayersMenu.CreatePopupMenu();
            mainMenu.AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) additionalLayersMenu.m_hMenu, pAddLayer->m_name );

            PopulateFieldsMenu( pMap, pAddLayer, additionalLayersMenu, false );

            additionalLayersMenu.Detach();
            }         
         }
      }
   }

   
BOOL MapFrame::OnNotify(WPARAM ctrlID, LPARAM lParam, LRESULT* pResult) 
   {
	return CWnd::OnNotify(ctrlID, lParam, pResult);
   }


void MapFrame::OnCellproperties() 
   {
   MapLayer *pLayer = m_pMap->GetActiveLayer();

   if ( m_pCellPropDlg == NULL )
      {
      m_pCellPropDlg = new CellPropDlg(m_pMapWnd, pLayer, m_currentPoly, this);
      m_pCellPropDlg->Create( CellPropDlg::IDD, this );
      m_pCellPropDlg->ShowWindow( SW_SHOW );
      }
   else
      {
      m_pCellPropDlg->SetLayer( pLayer );
      m_pCellPropDlg->SetCurrentCell( m_currentPoly );
      m_pCellPropDlg->SetForegroundWindow();
      }
   }


void MapFrame::OnSelectField( UINT nID )
   {
   int activeField = nID;            // active field to be nID, UNLESS ...

   if ( 40000 <= nID && nID <= 60000 )
      {
      // layer/field coding: 40000 + (1000*drawLayerIndex) + fieldIndex
      int drawLayerIndex = ( nID-40000 ) / 1000;
      int col = ( nID-40000 ) - drawLayerIndex*1000;

      int layerIndex = m_pMap->GetLayerIndexFromDrawIndex( drawLayerIndex );
      
      Map::SetupBins( m_pMap, layerIndex, col, false );

	   m_pMap->Notify(NT_ACTIVEATTRIBUTECHANGED,col,0);
      }

   RedrawWindow();
   if ( m_pMapWnd )
      m_pMapWnd->RedrawWindow();

   RefreshList();
   }

void MapFrame::RouteToParent( UINT nID )  // for code between 30000-39000
   {
   ::SendMessage( GetParent()->m_hWnd, WM_COMMAND, MAKEWPARAM( nID, 0 ), 0 );
   }



void MapFrame::OnShowSameAttribute() 
   {
   MapLayer *pLayer = m_pMap->GetActiveLayer();

   if ( pLayer == NULL )
      return;

   int activeCol = pLayer->GetActiveField();
   if ( activeCol < 0 )
      return;

   int   polyArray[ 100 ];
   int count = pLayer->GetNearbyPolys( pLayer->GetPolygon( m_currentPoly ), polyArray, NULL, 100, 1000 );

   CString msg;
   msg.Format ("Found %i matching cells", count );
   MessageBox( msg );

   for ( int i=0; i < count /*polyArray.GetSize()*/; i++ )
      pLayer->AddSelection( polyArray[ i ] );

   m_pMapWnd->RedrawWindow();
   }


void MapFrame::OnReclassifydata() 
   {
   MapLayer *pLayer = m_pMap->GetActiveLayer();

   if ( pLayer == NULL )
      return;

   pLayer->ClassifyData( -1 );

   m_pMapWnd->RedrawWindow();	
   }


void MapFrame::OnShowPolygonVertices() 
   {
   MapLayer *pLayer = m_pMap->GetActiveLayer();
   if ( pLayer == NULL )
      return;

   Poly *pPoly = pLayer->GetPolygon( m_currentPoly );
   m_pMapWnd->DrawPolyVertices( pPoly );
   }


void MapFrame::OnZoomToPoly() 
   {
   MapLayer *pLayer = m_pMap->GetActiveLayer();
   if ( pLayer == NULL )
      return;

   //Poly *pPoly = pLayer->GetPolygon( m_currentPoly );
   m_pMapWnd->ZoomToPoly( pLayer, m_currentPoly );
   }


int MapFrame::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message) 
   {
   //ActiveWnd::SetActiveWnd( this );
	//SetWindowPos( &wndTop, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE );
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
   }

void MapFrame::SetActiveField( int col, bool redraw )
   {
   Map::SetupBins( m_pMap, 0, col, false );
   m_pMap->Notify(NT_ACTIVEATTRIBUTECHANGED,col,0);
   
   if ( redraw )
      {
      RedrawWindow();
      RefreshList();
      }
   }

           

