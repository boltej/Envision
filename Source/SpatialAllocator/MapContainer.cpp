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
// MapContainer.cpp : implementation file
//

#include "stdafx.h"
#include "SpatialAllocator.h"
#include "MapContainer.h"
#include "afxdialogex.h"


// MapContainer dialog

IMPLEMENT_DYNAMIC(MapContainer, CWnd)

MapContainer::MapContainer(CWnd* pParent /*=NULL*/)
	: CWnd()
   //, m_pPlaceHolder( NULL )
   , m_mapFrame()
   , m_pMapLayer( NULL )
   , m_currentCell( -1 )
   , m_status()
{
}

MapContainer::~MapContainer()
{
}

//void MapContainer::DoDataExchange(CDataExchange* pDX)
//{
//	CDialog::DoDataExchange(pDX);
//}


BEGIN_MESSAGE_MAP(MapContainer, CWnd)
   ON_WM_SIZE()
   ON_WM_CREATE()
   ON_COMMAND(ID_LIB_CELLPROPERTIES, &MapContainer::OnCellproperties)
   ON_COMMAND_RANGE(40000, 60000, &MapContainer::OnSelectField)
END_MESSAGE_MAP()



const int BORDER_SIZE=4;
const int STATUS_SIZE=24;

int MapContainer::OnCreate( LPCREATESTRUCT lpCreateStruct )
   {
   if ( CWnd::OnCreate( lpCreateStruct ) == -1 )
      return -1;

   this->CreateMapWnd();
   return 0;
   }
   
void MapContainer::OnSize( UINT nType, int cx, int cy )
   {
   CWnd::OnSize( nType, cx, cy );
//   UPDATE_EASYSIZE;

   if ( ::IsWindow( m_mapFrame.GetSafeHwnd() ) )
      {
      CRect rect;
      this->GetClientRect( &rect );
      rect.DeflateRect( 0,0,0, STATUS_SIZE );
      m_mapFrame.MoveWindow( &rect, TRUE );
      }

   if ( ::IsWindow( m_status.GetSafeHwnd() ) )
      {
      CRect rect;
      this->GetClientRect( &rect );
      rect.top = rect.bottom - STATUS_SIZE;
      rect.bottom = rect.top + STATUS_SIZE;
      rect.left = BORDER_SIZE;
      rect.right -= BORDER_SIZE;
      m_status.MoveWindow( &rect, TRUE );
      }
   }


// MapContainer message handlers
void MapContainer::OnCellproperties() 
   {
   CellPropDlg dlg( m_mapFrame.m_pMapWnd, m_pMapLayer, m_currentCell, this );
   dlg.m_isModal = true;
   dlg.DoModal();
   }


void MapContainer::OnSelectField( UINT nID) 
   {
   this->m_mapFrame.OnSelectField( nID );
   }


void MapContainer::SetMapLayer( MapLayer *pLayer )
   {
   m_pMapLayer = pLayer;
   }


void MapContainer::CreateMapWnd( )
   {
   m_mapFrame.Create(NULL, "MapWnd", WS_CHILD | WS_VISIBLE, CRect( 0,0,20,20 ), this, 40000 );
   
   Map *pMap = m_mapFrame.m_pMap;
   pMap->AddLayer( m_pMapLayer, false );
   pMap->UpdateMapExtents();
   m_mapFrame.m_pMapWnd->ZoomFull();
   //RedrawMap();

   // still needed?
   pMap->InstallNotifyHandler(SAMapProc, (LONG_PTR) this);
   //pMap->InstallNotifyTextHandler( MapTextProc );

   m_status.Create("", WS_CHILD | WS_VISIBLE | SS_LEFT, CRect( 0,0,20,20), this, 40001 );

   SetStatusMsg( "No IDUs selected...  Double-click or drag on map to zoom...");
   }





//////////////////////////////////////////////////////////////////////////////

// Map event listener
int SAMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra )
   {
   static int zoomMode = 0;   //0=zoomed out, 1=zoomed in,
   static int dragMode = 0;   //0=not dragging, 1=dragging,
   static CPoint mouseStart;

   MapContainer *pDlg = (MapContainer*) extra;
   MapWindow *pMapWnd = pDlg->m_mapFrame.m_pMapWnd;
   MapLayer *pLayer = pMap->GetActiveLayer();

   switch( type )
      {
      case NT_LBUTTONDBLCLK:
         {
         if ( pLayer == NULL )
            MessageBeep( 0 );
         else
            {
            switch( pLayer->m_layerType )
               {
               case LT_POLYGON:
               case LT_LINE:
                  {
                  int index=-1;
                  REAL vx, vy;
                  pMap->DPtoVP( CPoint( a0, (int) a1 ), vx, vy );
                  Poly *pPoly = pLayer->GetPolygonFromCoord(  vx, vy, &index );

                  if ( pPoly == NULL )
                     {
                     MessageBeep( 0 );
                     break;
                     }

                  if ( zoomMode == 0 && index > -1 )   // zoomed out?
                     {
                     zoomMode = 1;

                     REAL vWidth, vHeight, vxMin, vxMax, vyMin, vyMax;
                     pLayer->GetExtents( vxMin, vxMax, vyMin, vyMax );

                     vWidth = (vxMax - vxMin)/20;
                     vHeight = (vyMax - vyMin)/20;

                     pMapWnd->ZoomRect( COORD2d( vx-vWidth, vy-vHeight ), COORD2d( vx+vWidth, vy+vHeight ), true );
                     }
                  else
                     {
                     pMapWnd->ZoomFull();
                     zoomMode = 0;
                     }

                   }  // end of: pLayer->m_layerType == LT_POLYGON
                  break;

               }
            }
         }
         break;

      case NT_LBUTTONDOWN:
         {
         mouseStart.x = a0;
         mouseStart.y = (int) a1;
         dragMode = 1;   // start drag

         REAL vx, vy;
         int index = -1;
         pMap->DPtoVP( CPoint( a0, (int) a1 ), vx, vy );
         Poly *pPoly = pLayer->GetPolygonFromCoord(  vx, vy, &index );

         if ( pPoly )
            {
            VData v;
            pLayer->GetData(index, pLayer->GetActiveField(), v );
            
            CString msg;
            msg.Format( "Poly Index %i: %s", index, v.GetAsString() );
            pDlg->SetStatusMsg( msg );
            }
         }
         break;

      case NT_LBUTTONUP:
         {
         if ( dragMode == 1 )
            {
            if (abs( mouseStart.x-a0 ) > 10  && abs( mouseStart.y-(int) a1  )> 10 )
               {
               COORD2d ll, ur; 
               pMap->DPtoVP( mouseStart, ll.x, ll.y );
               pMap->DPtoVP( CPoint( a0, (int) a1 ), ur.x, ur.y );

               pMapWnd->ZoomRect( ll, ur, true ); 
               zoomMode = 1;
               }

            //dragMode = 0;
            }
         }
         break;

      case NT_RBUTTONDOWN:   // pop up an context menu.
         {
         if ( pLayer == NULL )
            {
            MessageBeep( 0 );
            break;
            }

         int index=-1;
         //Poly *pPoly = pLayer->FindPolygon( a0, (int) a1, &index );

         REAL vx, vy;
         pMap->DPtoVP( CPoint( a0, (int) a1 ), vx, vy );
         Poly *pPoly = pLayer->GetPolygonFromCoord(  vx, vy, &index );

         pDlg->m_currentCell = index;

         CString msg;
         msg.Format( "x:%i, y:%i, index:%i   --- vx:%g, vy:%g", a0, a1, index, (float) vx, (float) vy );
         pDlg->SetStatusMsg( msg );
         
         //pMapFrame->m_currentPoly = index;

         CMenu mainMenu;
         CMenu *pPopup = NULL;

         mainMenu.CreatePopupMenu();
         pPopup = &mainMenu;
         //  if NOT over polygons, make a list of available fields
         if ( pPoly == NULL )
            PopulateFieldsMenu( pMap, pLayer, mainMenu, true );

         else  // over a polygon, load menu from menu resource
            {
            // have polygon, pop up context menu
            //mainMenu.LoadMenu( IDR_MAPCONTEXTMENU );   // load it from a menu resource
            //pPopup = mainMenu.GetSubMenu( 0 );
            mainMenu.AppendMenu( MF_STRING, ID_LIB_CELLPROPERTIES, "Properties for this Site" );
            mainMenu.AppendMenu( MF_SEPARATOR, -1, "" );
            mainMenu.AppendMenu( MF_STRING, ID_LIB_ZOOMTOPOLY, "Zoom to this Polygon" );
            
            CMenu layerMenu;
            layerMenu.CreatePopupMenu();
            pPopup->AppendMenu( MF_SEPARATOR ); 
            pPopup->AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) layerMenu.m_hMenu, "Fields" );

            PopulateFieldsMenu( pMap, pLayer, layerMenu, true );
            }

         ASSERT_VALID( pPopup );

         // convert points to screen coords
         CPoint point( a0, (int) a1 );
         pMapWnd->ClientToScreen( &point );

         // display and track the menu (Note: the window attached to this
         // menu is the MapContainer pContainer, e.g. a dialog box
         if ( pPopup )
            pPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, pDlg, NULL );
         }
         break;

      case NT_MOUSEMOVE:
         {
         POINT pt;
         pt.x = a0;
         pt.y = (int) a1;
         pMap->DPtoLP( pt );
         REAL x, y;
         pMap->LPtoVP( pt, x, y );

         char msg[ 64 ];
         sprintf_s( msg, 64, "(%f, %f)", x, y );
         //pp->m_status.SetWindowText( msg );
         }
         break;


      case NT_MOUSEWHEEL:
         {
         if ( a0 > 0 )
            pMapWnd->ZoomOut();
         else
            pMapWnd->ZoomIn();
         break;

         zoomMode = 1;
         }
      }  // endof switch:

   return 1;
   }


