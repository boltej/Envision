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
// Map.cpp : implementation file
//

#include "EnvLibs.h"
#pragma hdrstop

#include "MAP.h"

#include "COLORS.HPP"
#include "GEOMETRY.HPP"
#include "Maplayer.h"
#include "IDATAOBJ.H"

//#include <cximage/ximagif.h>
//#include <cximage/ximabmp.h>
#include <GeoSpatialDataObj.h>
//#include "CacheManager.h"
#include "tinyxml.h"

#include <math.h>
#include <limits.h>
#include <memory>
#ifndef NO_MFC
#include <gdiplus.h>
#include <atlimage.h> // for CImage
#endif
//using namespace Gdiplus;

#include "randgen/Randunif.hpp"

SETUPBINSPROC Map::m_setupBinsFn = NULL;


#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Map *pMap;

//*********************************************************

//IMPLEMENT_DYNCREATE(Map, CWnd)


/////////////////////////////////////////////////////////////////////////////
// Map

Map::Map()
	: m_pActiveLayer(NULL)
	, m_mapUnits(MU_UNKNOWN)
	, m_vxMin(0.0f)
	, m_vxMax(100.0f)
	, m_vyMin(0.0f)
	, m_vyMax(100.0f)
	, m_vExtent(100.0f)
	, m_vxMapMinExt(0.0f)
	, m_vxMapMaxExt(100.0f)
	, m_vyMapMinExt(0.0f)
	, m_vyMapMaxExt(100.0f)
	, m_deleteOnDelete(true)
   , m_pExtraPtr( NULL )
   , m_bkgrTop( 0 )
   , m_bkgrLeft( 0 )
   , m_bkgrRight( 0 )
   , m_bkgrBottom( 0 )
   , m_zoomFactor(0.9f)
#ifndef NO_MFC
   , m_windowOrigin()     // logical space
   , m_windowExtent()
   , m_viewportOrigin()   // device coords
   , m_viewportExtent()
   , m_displayRect()        // device coords
   , m_windowRect() 
#endif
   //m_pMapCache = new CacheManager(500);
   { }

Map::~Map()
   {
   Clear();
   }


void Map::Clear( void )
   {
   int layerCount = (int) m_mapLayerArray.GetSize();
   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = m_mapLayerArray[ i ].pLayer;
      if ( m_deleteOnDelete && m_mapLayerArray[ i ].deleteOnDelete )
         delete pLayer;
      m_mapLayerArray[ i ].pLayer = NULL;
      }

   m_mapLayerArray.RemoveAll();
   }


MapLayer *Map::AddLayer( LAYER_TYPE type )
   {
   MapLayer *pLayer = new MapLayer( this );
   pLayer->m_layerType = type;

   MAP_LAYER_INFO mapInfo( pLayer, true );

   m_mapLayerArray.Add( mapInfo );
   m_drawOrderArray.Add( pLayer );

   if ( m_mapLayerArray.GetSize() == 1 )
      {
      m_pActiveLayer = pLayer;
      SetMapExtents( pLayer->m_vxMin, pLayer->m_vxMax, pLayer->m_vyMin, pLayer->m_vyMax );
      //ZoomFull( false );  // no redraw
      }

   return pLayer;
   }


MapLayer *Map::AddLayer( MapLayer *pLayer, bool deleteOnDelete/*=true*/  )
   {
   MAP_LAYER_INFO mapInfo( pLayer, deleteOnDelete );

   m_mapLayerArray.Add( mapInfo );
   m_drawOrderArray.Add( pLayer );

   if ( m_mapLayerArray.GetSize() == 1 )
      {
      m_pActiveLayer = pLayer;
      SetMapExtents( pLayer->m_vxMin, pLayer->m_vxMax, pLayer->m_vyMin, pLayer->m_vyMax );
      //ZoomFull( false );  // no redraw
      }
   else
      AddMapExtents( pLayer->m_vxMin, pLayer->m_vxMax, pLayer->m_vyMin, pLayer->m_vyMax );   // must do this prior to  InitPolyLogicalPoints()

   return pLayer;
   }


MapLayer *Map::GetLayer( LPCTSTR name )
   {
   for ( int i=0; i < m_mapLayerArray.GetSize(); i++ )
      {
      MapLayer *pLayer = m_mapLayerArray[ i ].pLayer;

      if ( pLayer->m_name.CompareNoCase( name ) == 0 )
         return m_mapLayerArray[ i ].pLayer;
      }

   return NULL;
   }


MapLayer *Map::GetLayerFromPath( LPCTSTR path )
   {
   for ( int i=0; i < m_mapLayerArray.GetSize(); i++ )
      {
      MapLayer *pLayer = m_mapLayerArray[ i ].pLayer;

      if ( pLayer->m_path.CompareNoCase( path ) == 0 )
         return m_mapLayerArray[ i ].pLayer;
      }

   return NULL;
   }


MapLayer *Map::CloneLayer( MapLayer &layer )
   {
   MapLayer *pLayer = new MapLayer( layer );
   return AddLayer( pLayer );
   }


bool Map::RemoveLayer( MapLayer *pLayer, bool deallocate /*=false*/ )
   {
   bool found = false;
   for ( int i=0; i < m_mapLayerArray.GetSize(); i++ )
      {
      if ( m_mapLayerArray[ i ].pLayer == pLayer )
         {
         bool deleteOnDelete = m_mapLayerArray[ i ].deleteOnDelete;

         m_mapLayerArray.RemoveAt( i );

         if ( deallocate && deleteOnDelete )
            delete pLayer;

         if ( m_mapLayerArray.GetSize() > 0 )
            m_pActiveLayer = m_mapLayerArray[ 0 ].pLayer;
         else
            m_pActiveLayer = NULL;
         found = true;
         break;
         }
      }
   for ( int i=0; i < m_drawOrderArray.GetSize(); i++ )
      {
      if ( m_drawOrderArray[ i ] == pLayer )
         {
         m_drawOrderArray.RemoveAt( i );
         break;
         }
      }
   
   return found;
   }

int Map::GetLayerIndexFromDrawIndex( int drawLayerIndex )
   {
   ASSERT( m_drawOrderArray.GetSize() == m_mapLayerArray.GetSize() );
   MapLayer *pDrawLayer = m_drawOrderArray[ drawLayerIndex ];
   for ( int j=0; j < (int) m_mapLayerArray.GetSize(); j++ )
      {
      if ( pDrawLayer == m_mapLayerArray[ j ].pLayer )
         return j;
      }
   return -1;
   }
int Map::GetDrawIndexFromLayerIndex( int layerIndex )
   {
   ASSERT( m_drawOrderArray.GetSize() == m_mapLayerArray.GetSize() );
   MapLayer *pLayer = GetLayer( layerIndex );
   
   for ( int i=0; i < (int) m_drawOrderArray.GetSize(); i++ )
      {
      if ( m_drawOrderArray[ i ] == pLayer )
         return i;
      }

   return -1;
   }



int Map::ChangeDrawOrder( MapLayer *pLayer, int order)
   {
   int curPos = -1;
   int layers = (int) m_drawOrderArray.GetSize();
   for ( int i=0; i < layers; i++ )
      {
      if ( pLayer == m_drawOrderArray[ i ] )
         {
         curPos = i;
         break;
         }
      }

   if ( curPos < 0 )
      return curPos;

   int newPos = -1;

   switch( order )
      {
      case DO_UP:
         if ( curPos == 0 )
            return 0;
         newPos = curPos-1;
         break;

      case DO_DOWN:
         if ( curPos == layers-1 )
            return 0;
         newPos = curPos+1;
         break;

      case DO_TOP:
         newPos = 0;
         break;

      case DO_BOTTOM:
         newPos = layers-1;
         break;

      default:  // index specified
         {
         newPos = order;

         if ( newPos == curPos )
            return newPos;

         if ( curPos < newPos )  // moving down the list (towards the tail)
            {
            for ( int i=curPos; i < newPos; i++ )
               m_drawOrderArray[ i ] = m_drawOrderArray[ i+1 ];
            }
         else   // moving up the list (toward the tail)
            {
            for ( int i=curPos; i > newPos; i-- )
               m_drawOrderArray[ i-1 ] = m_drawOrderArray[ i ];
            }

         m_drawOrderArray[ newPos ] = pLayer;
         return newPos;
         }
      }

   MapLayer *pSwapLayer = pSwapLayer = m_drawOrderArray[ newPos ];
   m_drawOrderArray[ newPos ] = pLayer;
   m_drawOrderArray[ curPos ] = pSwapLayer;

   return newPos;
   }


MapLayer *Map::CreateOverlay( MapLayer *pSourceLayer, int overlayFlags, int activeCol )
   {
   MapLayer *pLayer = new MapLayer( pSourceLayer, overlayFlags );
   pLayer->m_name = pSourceLayer->m_name + " (Overlay)";
   this->AddLayer( pLayer );

   if ( activeCol >= 0 )
      pLayer->SetActiveField( activeCol );
 
   return pLayer;
   }


MapLayer *Map::CreatePointOverlay( MapLayer *pSourceLayer, int col )
   {
   MapLayer *pLayer = this->AddLayer( LT_POINT );
   pLayer->m_pOverlaySource = pSourceLayer;
   pLayer->m_name = pSourceLayer->m_name + " (Overlay)";
   
   int count = pSourceLayer->GetPolygonCount();

   Vertex v;
   for ( int i=0; i < count; i++ )
      {
      Poly *pPoly = pSourceLayer->GetPolygon( i );
      v = pPoly->GetCentroid();
      pLayer->AddPoint( v.x, v.y );
      }

   if ( col >= 0 && pSourceLayer->m_pData )
      {
      ASSERT( col < pSourceLayer->GetColCount() );
      
      pLayer->m_pDbTable    = pSourceLayer->m_pDbTable;
      pLayer->m_pData       = pSourceLayer->m_pData;
      pLayer->SetActiveField( col );
      pLayer->m_pFieldInfoArray->Copy( *(pSourceLayer->m_pFieldInfoArray) );
      }

   return pLayer;
   }


MapLayer *Map::CreateDotDensityPointLayer( MapLayer *pSourceLayer, int col )
   {
   MapLayer *pLayer = this->AddLayer( LT_POINT );
   pLayer->m_name = pSourceLayer->m_name + " (Dot Density)";

   LPCTSTR fieldname = pSourceLayer->GetFieldLabel( col );
   
   IDataObj *pData = (IDataObj*) pLayer->CreateDataTable( 0, 1, DOT_INT );  // 0 rows, 1 col
   pLayer->m_pDbTable->SetField( 0, fieldname, TYPE_INT, 8, 0, true ); 
   
   int polyCount = pSourceLayer->GetPolygonCount();
   float value = 0;
   RandUniform rx;
   RandUniform ry;
   VData _value;
   for ( int i=0; i < polyCount; i++ )
      {
      value = 0;
      bool ok = pSourceLayer->GetData( i, col, value );

      if ( ok && value > 0 )
         {
         Poly *pPoly = pSourceLayer->GetPolygon( i );
         
         int roundedValue = (int) value;
         if ( fmod( value, 1.0f ) >= 0.5f )
            roundedValue++;

         for ( int j=0; j < roundedValue; j++ )
            {
            float x = (float) rx.RandValue( pPoly->m_xMin, pPoly->m_xMax );
            float y = (float) ry.RandValue( pPoly->m_yMin, pPoly->m_yMax );

            int index = pLayer->AddPoint( x, y );
            pLayer->SetData( index, 0, 1 );
               
            //_value = 1; // i;  //value;
            //pLayer->m_pData->AppendRow( &_value, 1 );
            }
         }
      }

   pLayer->AddFieldInfo( fieldname, 0, fieldname, "", "", TYPE_INT, MFIT_CATEGORYBINS, 
               BCF_SINGLECOLOR, BSF_LINEAR, 0, 2, true );
   
   pLayer->SetActiveField( 0 );
   pLayer->SetBins( 0, BCF_SINGLECOLOR, 1 );
   pLayer->ClassifyData();
   pLayer->UpdateExtents();

   return pLayer;
   }


int Map::GetLayerIndex( MapLayer *pLayer )
   {
   for ( int i=0; i < GetLayerCount(); i++ )
      if ( pLayer == GetLayer( i ) )
         return i;
   return -1;
   }

#ifndef NO_MFC
void Map::LPtoDP( POINT &p )
   {
   //if ( m_hWnd == NULL )
      {
      p.x = int(((p.x - m_windowOrigin.x)*m_viewportExtent.cx / (float) m_windowExtent.cx) + m_viewportOrigin.x);
      p.y = int(((p.y - m_windowOrigin.y)*m_viewportExtent.cy / (float) m_windowExtent.cy) + m_viewportOrigin.y);
      }
   //else
   //   {
   //   CDC *pDC = GetDC();
   //   InitMapping( *pDC );
   //   pDC->LPtoDP( &p, 1 );
   //   ReleaseDC( pDC );   
   //   }
   }


void Map::DPtoLP( POINT &p )
   {
   //CDC *pDC = GetDC();
   //if ( pDC != NULL )
   //   {
   //   InitMapping( *pDC );
   //   pDC->DPtoLP( &p, 1 );
   //   ReleaseDC( pDC );
   //   }
   //else
   //   p.x = p.y = 0;

   int dx = p.x;
   int dy = p.y;
   p.x = int( (dx - m_viewportOrigin.x ) * ( (float) m_windowExtent.cx / m_viewportExtent.cx ) + m_windowOrigin.x );
   p.y = int( (dy - m_viewportOrigin.y ) * ( (float) m_windowExtent.cy / m_viewportExtent.cy ) + m_windowOrigin.y ); 
   }

#endif

void Map::LPtoVP( POINT &lp, REAL &vx, REAL &vy )
   {
   // map the max min/max into the LOGICAL_EXTENTS
   // first, figure out which dimension has the larger extents   
   vx = (lp.x*m_vExtent/LOGICAL_EXTENTS) + m_vxMin;
   vy = (lp.y*m_vExtent/LOGICAL_EXTENTS) + m_vyMin;
   }


void Map::VPtoLP( REAL vx, REAL vy, POINT &lp )
   {
   // map the max min/max into the LOGICAL_EXTENTS
   lp.x = (int) ((vx-m_vxMin)*LOGICAL_EXTENTS/m_vExtent);
   lp.y = (int) ((vy-m_vyMin)*LOGICAL_EXTENTS/m_vExtent);
   }

void Map::AddMapExtents(REAL vxMin, REAL vxMax, REAL vyMin, REAL vyMax )
   {
   if ( vxMax > m_vxMapMaxExt ) m_vxMapMaxExt = vxMax;     // check bounds
   if ( vyMax > m_vyMapMaxExt ) m_vyMapMaxExt = vyMax;
   if ( vxMin < m_vxMapMinExt ) m_vxMapMinExt = vxMin;
   if ( vyMin < m_vyMapMinExt ) m_vyMapMinExt = vyMin;
   }


// set the map extents to the extents of all layers
void Map::UpdateMapExtents( void )
   {
   REAL vxMin, vxMax, vyMin, vyMax;
   m_vxMapMinExt = m_vyMapMinExt = (REAL) LONG_MAX;
   m_vxMapMaxExt = m_vyMapMaxExt = (REAL) LONG_MIN;

   for ( int layer=0; layer < m_mapLayerArray.GetSize(); layer++ )
      {
      MapLayer *pLayer = m_mapLayerArray[ layer ].pLayer;
      pLayer->UpdateExtents( false );  // don't init poly pts, sinces these will change with map extents

      pLayer->GetExtents( vxMin, vxMax, vyMin, vyMax );
      AddMapExtents( vxMin, vxMax, vyMin, vyMax );
      }

   // now update poly pts, since these depend n the new map extents
   InitPolyLogicalPoints();
   }




void Map::InitPolyLogicalPoints()
   {
   for ( int i=0; i < m_mapLayerArray.GetSize(); i++ )
      m_mapLayerArray[ i ].pLayer->InitPolyLogicalPoints( this );   // create logical points for this set
   }

#ifndef NO_MFC
bool Map::GetGridCellFromDeviceCoord( POINT &point, int &row, int &col )
   {
   REAL x, y;
   DPtoLP( point );         // convert to logical
   LPtoVP( point, x, y );   // then virtual
   // get active layer
   MapLayer *pLayer = GetActiveLayer();

   if ( pLayer == NULL )
      return false;

   return pLayer->GetGridCellFromCoord( x, y, row, col ); // note: coords are virtual
   }
#endif


int Map::GetAttrAt( CPoint &pt, CString &attrText )
   {
   MapLayer *pLayer = GetActiveLayer();

   if ( pLayer == NULL )
      return -1;

   switch( pLayer->m_layerType )
      {
      case LT_POLYGON:
      case LT_LINE:
         {
         Poly *pPoly = pLayer->FindPolygon( pt.x, pt.y );

         if ( pPoly == NULL )
            return -2;

         int col = pLayer->GetActiveField();
         if ( col < 0 )
            return -3;
         
         attrText.Empty();
         pLayer->GetDataAtPoint( pt.x, pt.y, col, attrText );

         // field info exist?  then get label for attribute
         MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( pLayer->GetFieldLabel( col ) );
         
         if ( pInfo )
            {
            FIELD_ATTR *pAttr = pInfo->FindAttribute( attrText );
            if ( pAttr )
               {
               attrText += " (";
               attrText += pAttr->label;
               attrText += ")";
               }
            }                            
         }
      }

   return 0;
   }


int Map::Notify( NOTIFY_TYPE t, int a0, LONG_PTR a1 )
   {
   int procVal = 0;
   int retVal  = 1;
   for ( int i=0; i < m_notifyProcArray.GetSize(); i++ )
      {
      MAPPROCINFO *pInfo = m_notifyProcArray[ i ];
      if ( pInfo->proc != NULL ) 
         {
         procVal =  pInfo->proc( this, t, a0, a1, pInfo->extra ); 
         if ( procVal == 0 )
            retVal = 0;
         }
      }

   return retVal;
   }


int Map::NotifyText( NOTIFY_TYPE t, LPCTSTR text )
   {
   int procVal = 0;
   int retVal  = 1;

   for ( int i=0; i < m_notifyTextProcArray.GetSize(); i++ )
      {
      MAPTEXTPROCINFO *pInfo = m_notifyTextProcArray[ i ];
      if ( pInfo->proc != NULL ) 
         {
         procVal = pInfo->proc( this, t, text ); 
         if ( procVal == 0 )
            retVal = 0;
         }
      }

   return retVal;
   }


bool Map::RemoveNotifyHandler( MAPPROC p, LONG_PTR extra )
   {
   for ( int i=0; i < m_notifyProcArray.GetSize(); i++ )
      {
      MAPPROCINFO *pInfo = m_notifyProcArray[ i ];
      if ( pInfo->proc == p && pInfo->extra == extra ) 
         {
         m_notifyProcArray.RemoveAt( i );
         return true;
         }
      }
   return false;
   }


bool Map::RemoveNotifyTextHandler( MAPTEXTPROC p /*, LONG_PTR extra*/ )
   {
   for ( int i=0; i < m_notifyTextProcArray.GetSize(); i++ )
      {
      MAPTEXTPROCINFO *pInfo = m_notifyTextProcArray[ i ];
      if ( pInfo->proc == p /*&& pInfo->extra == extra*/ ) 
         {
         m_notifyTextProcArray.RemoveAt( i );
         return true;
         }
      }
   return false;
   }


MapLayer *Map::AddGridLayer( LPCTSTR gridFile, DO_TYPE type, int maxLineWidth, int binCount, BCFLAG bcFlag )
   {
   MapLayer *pLayer = AddLayer( LT_GRID ); 

   if ( pLayer != NULL )
      {
      int result = pLayer->LoadGrid( gridFile, type, maxLineWidth, binCount, bcFlag );
      if ( result < 0 )
         {
         Report::StatusMsg( "AddGridLayer(): Unable to load grid..." );
         RemoveLayer( pLayer, true );
         return NULL;
         }
      else
         { 
         CString msg;
         msg.Format("AddGridLayer(): Added grid... layer=%s", (LPCTSTR)pLayer->m_name);
         Report::StatusMsg(msg);
   
         return pLayer;
         }
      }

   CString msg;
   msg.Format("AddGridLayer(): Failed to add grid... %s", gridFile );
   Report::StatusMsg(msg);

   return NULL;
   }


// static function
void Map::SetupBins( Map *pMap, int layerIndex, int col /*=-1*/, bool force /*= false*/ )
   {
     WAIT_CURSOR;

   if ( m_setupBinsFn )
      {
      m_setupBinsFn( pMap, layerIndex, col, force );
      return;
      }

   MapLayer *pLayer = pMap->GetLayer( layerIndex );

   if ( pLayer == NULL || pLayer->m_pData == NULL )
      return;

   if ( col < 0 )
      col = pLayer->GetActiveField();
   else if ( col < pLayer->GetFieldCount() )
      pLayer->SetActiveField( col );
   else
      pLayer->SetActiveField( 0 );

   if ( ! force && pLayer->GetBinCount( col ) > 0 )
      {
      // bins already set up, just reclassify
      pLayer->ShowBinCount();
      pLayer->ClassifyData( col );
      //pMap->Invalidate( false );
      return;
      }

   // classify field      
   // is it a column we have field info for?
   LPCTSTR label = pLayer->GetFieldLabel();  // get label for active col

   MAP_FIELD_INFO *pInfo = pLayer->FindFieldInfo( label );
   if ( pInfo != NULL )    // found a field, now use it
      pLayer->SetBins( pInfo );

   // any data to display?
   if ( pLayer->GetBinCount( col ) > 0 )
      {
      //pLayer->ShowLegend();
      pLayer->ShowBinCount();
      pLayer->ClassifyData(); 
      }
   else
      {
      // create bins and classify data
      pLayer->SetBins( col );
      pLayer->ShowBinCount();
      pLayer->ClassifyData(); 
      }

   //pMap->Invalidate( false );
   }

