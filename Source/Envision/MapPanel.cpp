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
// MapPanel.cpp : implementation file
//

#include "stdafx.h"

#pragma hdrstop

#include "Envision.h"
#include "MapPanel.h"
#include "ActiveWnd.h"

#include <Actor.h>
#include "EnvView.h"
#include "EnvDoc.h"
#include <EnvModel.h>
#include "mainfrm.h"
#include <EnvPolicy.h>
#include "MapListWnd.h"
#include "PPPolicyResults.h"
#include "DeltaViewer.h"

#include <MapWnd.h>

#include <queryBuilder.h>
#include <MAP.h>
#include <MapFrame.h>
#include <Maplayer.h>

extern CMainFrame    *gpMain;
extern CEnvDoc       *gpDoc;
extern CEnvView      *gpView;
extern PolicyManager *gpPolicyManager;
extern ActorManager  *gpActorManager;
extern MapLayer      *gpCellLayer;
extern EnvModel      *gpModel;

// MapPanel


int MapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );
int MapTextProc( Map *pMap, NOTIFY_TYPE type, LPCTSTR msg );

void SetupBins( Map *pMap, int layerIndex, int nID = -1, bool force = false );


IMPLEMENT_DYNAMIC(MapPanel, CWnd)

MapPanel::MapPanel()
{
}


MapPanel::~MapPanel()
{
}


BEGIN_MESSAGE_MAP(MapPanel, CWnd)
   ON_WM_CREATE()
	ON_WM_SIZE()
  	//ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_SHOWRESULTS, OnShowresults)
	ON_COMMAND(ID_SHOWDELTAS, OnShowdeltas)
	//ON_UPDATE_COMMAND_UI(ID_SHOWRESULTS, OnUpdateShowresults)
	//ON_UPDATE_COMMAND_UI(ID_SHOWDELTAS, OnUpdateShowdeltas)
	//ON_WM_ACTIVATE()
	//ON_COMMAND(ID_CELLPROPERTIES, OnCellproperties)
	//ON_COMMAND(ID_SHOWSAMEATTRIBUTE, OnShowSameAttribute)
   ON_COMMAND(ID_RUNQUERY, OnRunQuery)
   //ON_COMMAND(ID_SHOWPOLYGONVERTICES, OnShowPolygonVertices)
   //ON_COMMAND(ID_ZOOMTOPOLY, OnZoomToPoly)
	//ON_COMMAND(ID_RECLASSIFYDATA, OnReclassifydata)
	//ON_WM_MOUSEACTIVATE()
   //ON_COMMAND_RANGE(40000, 60000, OnSelectField)
END_MESSAGE_MAP()


// MapPanel message handlers


// MapPanel message handlers

int MapPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   // build the outer window for for holding the MapWindow and associated legend window
   m_mapFrame.Create( NULL, _T("MapFrame"), WS_CHILD | WS_VISIBLE, CRect( 0,0,0,0), this, 100, NULL );


   m_pMap = m_mapFrame.m_pMap;

   m_pMap->InstallNotifyHandler( MapProc, (LONG_PTR) &m_mapFrame );
   m_pMap->InstallNotifyTextHandler( MapTextProc );

   Map::m_setupBinsFn = SetupBins;

   ActiveWnd::SetActiveWnd( &m_mapFrame );

   m_pMapWnd = m_mapFrame.m_pMapWnd;
  
   // Map Text definitions       X    Y     position flags,                                           fontSize
   m_pMapWnd->AddMapText( "",   20,   20,   MTP_ALIGNRIGHT | MTP_FROMRIGHT | MTP_FROMTOP,     180 );    // text
   m_pMapWnd->AddMapText( "",   20,   65,   MTP_ALIGNRIGHT | MTP_FROMRIGHT | MTP_FROMTOP,     100 );    // subtext 
   m_pMapWnd->AddMapText( "",   20,   20,   MTP_ALIGNLEFT  | MTP_FROMLEFT  | MTP_FROMBOTTOM,  100 );    // lower right
   m_pMapWnd->AddMapText( "",   20,   20,   MTP_ALIGNLEFT  | MTP_FROMLEFT  | MTP_FROMTOP,     180 );    // upper right

   return 0;
   }


void MapPanel::OnSize(UINT nType, int cx, int cy) 
   {
	CWnd::OnSize(nType, cx, cy);

   if ( IsWindow( m_mapFrame.m_hWnd ) )
      {
      m_mapFrame.MoveWindow( 0, 0, cx, cy, TRUE );
      }	
   }


int MapTextProc( Map *pMap, NOTIFY_TYPE type, LPCTSTR msg )
   {
   gpMain->SetStatusMsg( msg );

   return 1;
   }


int MapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra )
   {
   MapFrame *pMapFrame = (MapFrame*) extra;

   switch( type )
      {
      case NT_LOADVECTOR:
         if ( a0 >= 0 )
            {
            MSG msg;
            BOOL retVal = PeekMessage( &msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE | PM_QS_INPUT ); 

            if ( retVal != 0 && msg.wParam == VK_ESCAPE ) // keyboard message
               {
               Report::WarningMsg( _T("The map loading process was interrupted - saving the database to the same location will corrupt the original database") );
               return 0;
               }
            
            if ( a0%100 == 0 )
               {
               CString msg;
               msg.Format( "Loading Vector: %i", a0 );
               gpMain->SetStatusMsg( msg );
               }
            }
         else
            {
            CString msg;
            msg.Format( "Loaded %i vectors", -a0 );
            gpMain->SetStatusMsg( msg );
            }            
         break;

      case NT_LOADRECORD:
         if ( a0 >= 0 )
            {
            if ( a0%10000 == 0 )
               {
               CString msg;
               msg.Format( "Loading Record: %i", a0 );
               gpMain->SetStatusMsg( msg );
               }
            }
         else
            {
            CString msg;
            msg.Format( "Loaded %i records", -a0 );
            gpMain->SetStatusMsg( msg );
            }            
         break;

      case NT_LOADGRIDROW:
         if ( a0 >= 0 )  // a0 is the row number
            {
            if ( a0%100 == 0 )
               {
               CString msg;
               msg.Format( "Loading row: %i", a0 );
               gpMain->SetStatusMsg( msg );
               }
            }
         else
            {
            CString msg;
            msg.Format( "Loaded %i rows", -a0 );
            gpMain->SetStatusMsg( msg );
            }            
         break;

      case NT_LBUTTONDBLCLK:
         {
         MapLayer *pLayer = pMap->GetActiveLayer();

         if ( pLayer == NULL )
            {
            MessageBeep( 0 );
            gpMain->SetStatusMsg( "No Active Layer set..." );
            }
         else
            {
            switch( pLayer->m_layerType )
               {
               case LT_POLYGON:
               case LT_LINE:
                  {
                  Poly *pPoly = pLayer->FindPolygon( a0, (int) a1 );

                  if ( pPoly == NULL )
                     {
                     MessageBeep( 0 );
                     gpMain->SetStatusMsg( "No polygon selected..." );            
                     break;
                     }

                  CString msg;
                  msg.Format( "%s - PolyID: %d   ", (LPCTSTR) pLayer->m_name, pPoly->m_id );

                  int col = pLayer->GetActiveField();
                  if ( col < 0 )
                     msg += "(No active field)";
                  else
                     {
                     CString value;
                     pLayer->GetDataAtPoint( a0, (int) a1, col, value );
                     msg += pLayer->GetFieldLabel( col );
                     msg += ": ";

                     int colPolicy = gpModel->m_colPolicy;

                     if ( col == colPolicy )
                        {               
                        int i = atoi( value );
                        if ( i >= 0 )
                           {
                           EnvPolicy *pPolicy = gpPolicyManager->GetPolicyFromID( i );

                           msg += pPolicy->m_name;

                           msg += " (";
                           msg += value;
                           msg += ")";
                           }
                        else
                           msg += "No Policy Applied";
                        }
                     else
                        {
                        msg += value;

                        // field info exist?  then get label for attribute
                        MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( pLayer->GetFieldLabel( col ) );

                        if ( pInfo )
                           {
                           FIELD_ATTR *pAttr = pInfo->FindAttribute( value );
                           if ( pAttr )
                              {
                              msg += " (";
                              msg += pAttr->label;
                              msg += ")";
                              }
                           }                            
                        }
                     }
                  gpMain->SetStatusMsg( msg );            
                  }  // end of: pLayer->m_layerType == LT_POLYGON
                  break;

               case LT_GRID:
                  {
                  POINT pt;
                  pt.x = a0;
                  pt.y = (int) a1;

                  pMapFrame->m_pMap->DPtoLP( pt );              // convert to logical
                  REAL vx, vy;
                  pMapFrame->m_pMap->LPtoVP( pt, vx, vy );

                  int row, col;
                  bool ok = pLayer->GetGridCellFromCoord( vx, vy, row, col );

                  if ( ! ok )
                     {
                     MessageBeep( 0 );
                     gpMain->SetStatusMsg( "No cell selected..." );            
                     break;
                     }

                  CString value;
                  pLayer->GetData( row, col, value );

                  CString msg;
                  msg.Format( "%s - row: %i, col: %i, value: %s", (LPCTSTR) pLayer->m_name, row, col, (LPCTSTR) value );
                  gpMain->SetStatusMsg( msg );            
                  }
                  break;

               case LT_POINT:
                  {
                  }
               break;
               }
            }
         }
         break;

      case NT_LBUTTONDOWN:
         {
         MapWindow *pMapWnd = pMapFrame->m_pMapWnd;

         switch( gpView->GetMode() )
            {
            case MM_NORMAL:
               break;
               
            case MM_ZOOMIN:
               if ( IsWindow( pMapWnd->m_hWnd ) )
                  {
                  gpMain->SetStatusMode( _T("Zoom In" ));
                  pMapWnd->StartZoomIn();
                  }
               break;
               
            case MM_ZOOMOUT:
               if ( IsWindow( pMapWnd->m_hWnd ) )
                  {
                  gpMain->SetStatusMode( _T("Zoom Out" ));
                  pMapWnd->StartZoomOut();
                  }
               break;

            case MM_PAN:
               if ( IsWindow( pMapWnd->m_hWnd ) )
                  {
                  gpMain->SetStatusMode( _T("Pan" ));
                  pMapWnd->StartPan();
                  }
               break;

            case MM_SELECT:
               if ( IsWindow( pMapWnd->m_hWnd ) )
                  {
                  gpMain->SetStatusMode( _T("Zoom In" ));
                  pMapWnd->StartSelect();
                  }
               break;

            case MM_QUERYDISTANCE:
               if ( IsWindow( pMapWnd->m_hWnd ) )
                  pMapWnd->StartQueryDistance();
               break;
            }
         }
         break;

         
      case NT_RBUTTONDOWN:   // pop up an context menu.
         {
         MapLayer *pLayer = pMap->GetActiveLayer();

         if ( pLayer == NULL )
            {
            MessageBeep( 0 );
            gpMain->SetStatusMsg( "No Active Layer set..." );
            break;
            }

         int index;
         Poly *pPoly = pLayer->FindPolygon( a0, (int) a1, &index );
         
         pMapFrame->m_currentPoly = index;

         CMenu mainMenu;
         CMenu *pPopup = NULL;

         //CMenu resultsMenu;
         //if ( gpDoc->m_model.m_currentRun > 0 )
         //   resultsMenu.CreatePopupMenu();

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

            int flag = gpDoc->m_model.m_currentRun >= 0 ? MF_STRING : MF_STRING | MF_GRAYED;
            mainMenu.AppendMenu( flag, ID_SHOWRESULTS, "Show Policies for this Site" );
            mainMenu.AppendMenu( flag, ID_SHOWDELTAS, "Show Deltas for this Site" );

            CMenu attrPlotMenu;
            attrPlotMenu.CreatePopupMenu();

            for ( int i=0; i < pLayer->GetFieldInfoCount( 1 ); i++ )
               {
               MAP_FIELD_INFO *pInfo = pLayer->GetFieldInfo( i );

               if ( pInfo->mfiType != MFIT_SUBMENU )
                  {
                  if ( ::IsNumeric( pInfo->type ) )
                     attrPlotMenu.AppendMenu( MF_STRING, ID_ATTRPLOT+i, pInfo->label );
                  }
               }

            mainMenu.AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) attrPlotMenu.m_hMenu, "Plot Attribute Values from most recent Run" );

            mainMenu.AppendMenu( MF_SEPARATOR, -1, "" );
            mainMenu.AppendMenu( MF_STRING, ID_LIB_RECLASSIFYDATA, "Reclassify Data" );
            mainMenu.AppendMenu( MF_STRING, ID_LIB_SHOWSAMEATTRIBUTE, "Show Polygons with the Same Attribute" );
            mainMenu.AppendMenu( MF_STRING, ID_RUNQUERY, "Run &Query on this Polygon" );
            mainMenu.AppendMenu( MF_STRING, ID_LIB_SHOWPOLYGONVERTICES, "Show Polygon &Vertices" );
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
         pMapFrame->m_pMapWnd->ClientToScreen( &point );

         // display and track the menu
         if ( pPopup )
            pPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, pMapFrame, NULL );
         }
         break;

      case NT_LBUTTONUP:
         {
         //switch( gpView->m_mode )
            //{
            // for any of the zoom modes, if we're in a result window, zoom all results
            //case MM_ZOOMIN:
            //case MM_ZOOMOUT:
            //case MM_PAN:
            //   if ( IsWindow( pMapWnd->m_hWnd ) )
            //      pMap->StartPan();
            //   break;
            //}

         gpView->SetMode( MM_NORMAL );
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
         gpMain->SetStatusMsg( msg );
         }
         break;

      case NT_EXTENTSCHANGED:
         {
         REAL vxMin, vxMax, vyMin, vyMax;
         pMap->GetViewExtents( vxMin, vxMax, vyMin, vyMax );
         CString msg1;
         msg1.Format( "View: (%g,%g)-(%g,%g)   ", vxMin, vyMin, vxMax, vyMax );

         //gpMap3d->ChangeViewExtents( vxMin, vxMax, vyMin, vyMax );

         pMap->GetMapExtents( vxMin, vxMax, vyMin, vyMax );
         CString msg2;
         msg2.Format( "Map: (%g,%g)-(%g,%g)", vxMin, vyMin, vxMax, vyMax );
         msg1 += msg2;
         gpMain->SetStatusMsg( msg1 );
         }
         break;

      case NT_CLASSIFYING:
         gpMain->SetStatusMsg( "Classifying data..." );
         break;

      case NT_CALCDIST:
         if ( a0 > 0 )
            {
            if ( a0%100 == 0 )
               {
               CString msg;
               msg.Format( "Computing distance for poly %i", a0 );
               gpMain->SetStatusMsg( msg );
               }
            }
         else
            {
            CString msg;
            msg.Format( "Computed %i distances", -a0 );
            gpMain->SetStatusMsg( msg );
            }            
         break;

      case NT_MOUSEWHEEL:
         {
         if ( a0 > 0 )
            pMapFrame->m_pMapWnd->ZoomOut();
         else
            pMapFrame->m_pMapWnd->ZoomIn();
         break;
         }

      //case NT_BUILDSPATIALINDEX:  // moved to SpatialIndexDlg
      //   {
      //   if ( (a0 % 10 ) == 0 )
      //      {
      //      CString msg;
      //      msg.Format( "Building index for poly %i of %i", a0, a1 );
      //      gpMain->SetStatusMsg( msg );
      //      //gpMain->SetStatusMode( "Building index" );
      //      }
      //   }
      //   break;
      }  // endof switch:

   return 1;
   }



void SetupBins( Map *pMap, int layerIndex, int nID /*=-1*/, bool force /*= false*/ )
   {
   CWaitCursor c;
   ASSERT( gpDoc != NULL );
   ASSERT( gpPolicyManager != NULL );
   ASSERT( gpActorManager != NULL );

   MapLayer *pLayer = pMap->GetLayer( layerIndex );

   if ( pLayer == NULL || pLayer->m_pData == NULL )
      return;

   if ( nID < 0 )
      nID = pLayer->GetActiveField();
   else if ( nID < pLayer->GetFieldCount() )
      pLayer->SetActiveField( nID );
   else
      pLayer->SetActiveField( 0 );

   if ( ! force && pLayer->GetBinCount( nID ) > 0 )
      {
      // bins already set up, just reclassify

      //pLayer->ShowLegend();
      pLayer->ShowBinCount();
      pLayer->ClassifyData( nID );
      //pMap->Invalidate( false );
      return;
      }

   // classify field      
   //pLayer->ClearAllBins();
   if ( layerIndex == 0 )
      {
      // figure out which field it is
      int lulcLevels = gpModel->m_lulcTree.GetLevels(); 
      int colPolicy  = pLayer->GetFieldCol( "POLICY" ); // gpModel->m_colPolicy;

      if ( nID == colPolicy )
         {
         //
         // Bins consist of all policies with the use flag set
         // and all policies that are currently in the POLICY field
         //

         CUIntArray values;
         pLayer->GetUniqueValues( colPolicy, values );    // get unique policy ID's

         int count = gpPolicyManager->GetPolicyCount();
         for ( int i=0; i < count; i++ )
            {
            EnvPolicy *pPolicy = gpPolicyManager->GetPolicy( i );
            bool add = false;
            if ( pPolicy->m_use )
               {
               add = true;
               }
            else
               {
               for ( int j=0; j < values.GetSize(); j++ )
                  {
                  if ( values.GetAt(j) == pPolicy->m_id )
                     {
                     add = true;
                     break;
                     }
                  }
               }

            if ( add )
               {
               COLORREF color = pPolicy->m_color;
               pLayer->AddBin( colPolicy, color, pPolicy->m_name, TYPE_INT, pPolicy->m_id - 0.1f, pPolicy->m_id + 0.1f );
               }
            }

         pLayer->AddNoDataBin( colPolicy );       
    
         //pLayer->SortBins( true );
         }  // end of: if ( nID == colPolicy )

      else if ( nID == pLayer->GetFieldCol( "POLICYAPPS" ) )
         {
         static int maxPolicyApps = 10;
         float minVal, maxVal;
         pLayer->GetDataMinMax( nID, &minVal, &maxVal );

         if ( maxPolicyApps < int( maxVal ) )
            maxPolicyApps = int( maxVal );

         CStringArray labels;
         for ( int i=0; i<=maxPolicyApps; i++ )
            {
            CString str;
            str.Format( "%d", i );
            labels.Add( str );
            }
         pLayer->SetBinColorFlag( BCF_GREEN_INCR );
         pLayer->SetBins( nID, labels );
         }

      // anything else 
      else
         {
         // is it an actor value field?
         bool isValueField = false;
         for ( int i=0; i < gpModel->m_colActorValues.GetSize(); i++ )
            {
            if ( nID == gpModel->m_colActorValues[ i ] )
               {
               isValueField = true;
               pLayer->SetBinColorFlag( BCF_REDGREEN );
               pLayer->SetBins( nID, -3.0f, 3.0f, 12, TYPE_FLOAT );
               break;
               }
            }

         if ( ! isValueField )
            {
            // its not the altern col, is a column we have field info for?
            LPCTSTR label = pLayer->GetFieldLabel();  // get label for active col

            MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( label );
            if ( pInfo != NULL )    // found a field, now use it
               pLayer->SetBins( pInfo );
            else
               {  // pInfo == NULL, check any remaining cases
               
               }
            }  // end of:  else ( not a altern col )
         }  // end of: else ( not POLICY col )
      }

   // any data to display?
   if ( pLayer->GetBinCount( nID ) > 0 )
      {
      pLayer->ShowBinCount();
      pLayer->ClassifyData(); 
      }
   else
      {
      // create bins and classify data
      pLayer->SetBins( nID );
      pLayer->ShowBinCount();
      pLayer->ClassifyData(); 
      }

   //pMap->Invalidate( false );
   }


void MapPanel::OnRunQuery() 
   {
   MapLayer *pLayer = m_pMap->GetActiveLayer();
   QueryBuilder dlg(gpModel->m_pQueryEngine, m_mapFrame.m_pMapWnd, NULL, m_mapFrame.m_currentPoly );
   dlg.DoModal();

   this->m_pMapWnd->RedrawWindow();
   }



void MapPanel::OnShowresults() 
   {
   CPropertySheet dlg( "Site Results", this, 0 );
   PPPolicyResults resultsPage( m_mapFrame.m_currentPoly );

   dlg.AddPage( &resultsPage );
   resultsPage.m_cell = m_mapFrame.m_currentPoly;

   dlg.DoModal();
   }


void MapPanel::OnUpdateShowresults(CCmdUI* pCmdUI) 
   {
   pCmdUI->Enable();
   }


void MapPanel::OnShowdeltas() 
   {
   if ( gpDoc->m_model.m_currentRun <= -1 )
      {
      AfxMessageBox( "No deltas are available until you've completed a run", MB_OK );
      return;
      }
   
   gpCellLayer->ClearSelection();

   gpCellLayer->AddSelection( m_mapFrame.m_currentPoly );
   DeltaViewer dlg( gpMain, -1, true );

   gpCellLayer->ClearSelection();

   dlg.DoModal();
   }


void MapPanel::OnUpdateShowdeltas(CCmdUI* pCmdUI) 
   {
   pCmdUI->Enable( gpDoc->m_model.m_currentRun > -1 ? TRUE : FALSE );
   }



void MapPanel::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
   {
	CWnd::OnActivate(nState, pWndOther, bMinimized);
	
   if ( nState != WA_INACTIVE )
      ActiveWnd::SetActiveWnd( &(this->m_mapFrame) );   
   }
  
