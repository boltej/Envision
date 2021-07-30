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
// MapWindow.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include <MapWnd.h>
#include <map.h>

#include "legend.h"
#include "memdc.h"
#include "tipwnd.h"
#include "scale.h"

#include <colors.hpp>
#include <geometry.hpp>
#include <maplayer.h>
#include <queryengine.h>
#include <idataobj.h>

//#include <cximage/ximagif.h>
//#include <cximage/ximabmp.h>
#include "GeoSpatialDataObj.h"
#include "CacheManager.h"
#include <tinyxml.h>

#include <math.h>
#include <limits.h>
#include <memory>
#include <gdiplus.h>
#include <atlimage.h> // for CImage
//using namespace Gdiplus;

#include <randgen/randunif.hpp>


#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

MapWindow *pMapWnd;

//*********************************************************

IMPLEMENT_DYNCREATE(MapWindow, CWnd)


int MapWndNotifyProc(Map*, NOTIFY_TYPE, int, LONG_PTR, LONG_PTR);  // return 0 to interrupt, anything else to continue

int MapWndNotifyProc(Map *pMap, NOTIFY_TYPE ntype, int param0, LONG_PTR, LONG_PTR)
   {
   // return 0 to interrupt, anything else to continue

   if ( ntype == NT_LOADVECTOR )
      {
      if ( param0 < 0  )  // finished loading vector coverage?
         {
         if ( pMap->GetLayerCount() == 1 )
            {
            //float vxMin, vxMax, vyMin, vyMax;
            //pMap->SetMapExtents( pMap->m_vxMin, pMap->m_vxMax, pMap->m_vyMin, pMap->m_vyMax );
            //ZoomFull( false );  // no redraw
            }
         else
            ; //pMap->AddMapExtents( m_vxMin, m_vxMax, m_vyMin, m_vyMax );   // must do this prior to  InitPolyLogicalPoints()
   
         pMap->InitPolyLogicalPoints();   // create logical points for this set   
         }
      }

   return 1;
   }


/////////////////////////////////////////////////////////////////////////////
// MapWindow

MapWindow::MapWindow()
	: m_invertCoords(true)
	, m_mode(MM_NORMAL)
	, m_modeSticky(true)
	, m_rubberBanding(false)
	, m_bkgrColor(RGB(255, 255, 255))
	, m_selectionColor(RGB(255, 255, 0)) // yellow
	, m_selectionHatchStyle(Gdiplus::HatchStyleDiagonalCross)
	, m_labelFont()
	, m_deleteOnDelete(true)
	, m_useGdiPlus(true)
	, m_extra(0)
   , m_pMapCache( NULL )
   , m_pMap( NULL )
	, m_pTipWnd(NULL)
	, m_maxPolyParts(0)
	, m_timerCount(-1)
	, m_showTitle(false)
	, m_showScale(true)
	, m_showAttributeOnHover(false)
	, m_ttPosition()
	, m_ttActivated(false)
	, m_pBkgrImage(NULL)
	, m_showBkgrImage(true)
	, m_bkgrImageUL(0, 0)
	, m_bkgrImageLR(0, 0)
  {
  m_pMap = new Map;
  m_pMapCache = new CacheManager(500);
  m_startPt.x = m_startPt.y = m_lastPt.x = m_lastPt.y = -1;
  m_labelFont.CreatePointFont( 80, "Arial" ); 
  }


MapWindow::~MapWindow()
   {
   //int layerCount = (int) m_mapLayerArray.GetSize();
   //for ( int i=0; i < layerCount; i++ )
   //   {
   //   MapLayer *pLayer = m_mapLayerArray[ i ].pLayer;
   //
   //   if ( m_deleteOnDelete && m_mapLayerArray[ i ].deleteOnDelete )
   //      delete pLayer;
   //
   //   m_mapLayerArray[ i ].pLayer = NULL;
   //   }

   if ( m_pMap )
      delete m_pMap;

   if ( m_pTipWnd )
      delete m_pTipWnd;

   if ( m_pBkgrImage )
      {
      m_pBkgrImage->Destroy();
      delete m_pBkgrImage;
      }
   }



BEGIN_MESSAGE_MAP(MapWindow, CWnd)
   ON_WM_PAINT()
   ON_WM_CREATE()
   ON_WM_LBUTTONDBLCLK()
   ON_WM_MOUSEMOVE()
   ON_WM_TIMER()
   ON_WM_LBUTTONDOWN()
   ON_WM_RBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_ERASEBKGND()
   ON_WM_MOUSEWHEEL()
   ON_WM_SIZE()
   ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnTTNNeedText) 
END_MESSAGE_MAP()


BOOL MapWindow::OnTTNNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult) 
   {
   TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
   //UINT nID =pNMHDR->idFrom;
 
   if (pTTT->uFlags & TTF_IDISHWND)
      pTTT->lpszText = m_toolTipContent;
 
   return TRUE; 
   } 




/////////////////////////////////////////////////////////////////////////////
// MapWindow message handlers

void MapWindow::OnPaint() 
   {
   CPaintDC pdc(this); // device context for painting

   int oldPolyFillMode = pdc.GetPolyFillMode();
   pdc.SetPolyFillMode( WINDING );

   DrawMap( pdc );
  
   //RECT rect;
   //GetClientRect( &rect );   
   //if ( m_showTitle )
   //   dc.TextOut( rect.left+60, rect.bottom-60, m_name );

   pdc.SetPolyFillMode( oldPolyFillMode );
   // Do not call CWnd::OnPaint() for painting messages
   }


void MapWindow::CopyMapToClipboard( int flag  )
   {
   if ( OpenClipboard() == FALSE )
      {
      AfxMessageBox( "Unable to open clipboard..." );
      return;
      }

   if ( EmptyClipboard() == FALSE )
      {
      AfxMessageBox( "Unable to empty clipboard..." );
      return;
      }

   // create a device context and draw the map...
   CDC *pDevDC = GetDC();

   int oldPolyFillMode = pDevDC->GetPolyFillMode();
   pDevDC->SetPolyFillMode( WINDING );

   CMetaFileDC metaDC;
   if ( metaDC.CreateEnhanced( pDevDC, NULL, NULL, "MapWindow\0MapImage\0\0" ) == TRUE )
      {
      //m_invertCoords = false;
      int layerCount = m_pMap->GetLayerCount();

      // store original legend flags
      bool *showLegendArray = new bool[ layerCount ];
      if ( layerCount > 0 ) // draw only the first legend, but only if it exists
         {
         for ( int i=0; i < 1; i++ )// TEMPORARY: GetLayerCount(); i++ )
            {
            showLegendArray[i] = m_pMap->GetLayer(i)->m_showLegend;
            m_pMap->GetLayer(i)->m_showLegend = true;
            }  
         }

      bool useGdiPlus = m_useGdiPlus;
      m_useGdiPlus = false;  // to avoid breaking metafile

      //DrawLegend( metaDC );
      DrawMapToDeviceContext( metaDC, true, flag );
      //m_invertCoords = true;
      HENHMETAFILE hEnhMetaFile = metaDC.CloseEnhanced( );
      SetClipboardData( CF_ENHMETAFILE, hEnhMetaFile );

      m_useGdiPlus = useGdiPlus;

      for (int j=0; j < m_pMap->GetLayerCount(); j++ )
         m_pMap->GetLayer(j)->m_showLegend = showLegendArray[j];

      delete [] showLegendArray;
      }
   else
      AfxMessageBox( "Unable to create metafile" );

   pDevDC->SetPolyFillMode( oldPolyFillMode );

   ReleaseDC( pDevDC );
   CloseClipboard();

   RedrawWindow();
   }


void MapWindow::SaveAsImage( LPCTSTR filename, int imageType, int drawFlag )
   {
   MapLayer *pLayer = m_pMap->GetActiveLayer();
   if ( pLayer == NULL )
      {
      AfxMessageBox( "No Active Layer set" );
      return;
      }

   CDC* pDC = GetDC();

   CDC memDC;
   memDC.CreateCompatibleDC(pDC);

   CRect rect;
   m_pMap->GetDisplayRect( rect );
   //GetClientRect( &rect );

   CBitmap bitmap;
   bitmap.CreateCompatibleBitmap(pDC, rect.right, rect.bottom );

   CBitmap* pOldBitmap = memDC.SelectObject((CBitmap*)&bitmap);

   //--- At this point you can draw on your bitmap using memDC
   CBrush br( RGB( 255, 255, 255 ) );
   CBrush *pOldBrush = memDC.SelectObject( &br );
   memDC.Rectangle( &rect );
   memDC.SelectObject( pOldBrush );
   this->DrawMapToDeviceContext( memDC, true, drawFlag );

   memDC.SelectObject((CBitmap*)pOldBitmap);
   ReleaseDC(pDC);

   CImage image;
   image.Attach(bitmap);
   HRESULT result = image.Save( filename );
   if ( result != 0 )
      Report::SystemErrorMsg( GetLastError() );
   }


void MapWindow::SaveAsEnhancedMetafile( LPCTSTR filename, int drawFlag )
   {
   // create a device context and draw the map...
   CDC *pDevDC = GetDC();

   CMetaFileDC metaDC;

   // store original legend flags
   int layerCount = m_pMap->GetLayerCount();
   bool *showLegendArray = new bool[ layerCount ];

   if ( layerCount > 0) //kbv and jpb:  draw only the first legend, but only if it exists
      {
      for ( int i=0; i < 1; i++ )// TEMPORARY: GetLayerCount(); i++ )
         {
         showLegendArray[i] = m_pMap->GetLayer(i)->m_showLegend;
         m_pMap->GetLayer(i)->m_showLegend = ( drawFlag & MDF_LEGEND ) ? true : false;
         }  
      }

   //switch( emfMode )
   //   {
   //   case ME_MAP:
         if ( metaDC.CreateEnhanced(pDevDC, filename, NULL, "MapWindow\0MapImage\0\0" ) == TRUE )
            { 
            m_showScale = false;
            DrawMapToDeviceContext( metaDC, true );
            HENHMETAFILE hEnhMetaFile = metaDC.CloseEnhanced( );
            m_showScale = true;
            }
   //      break;

   //   case ME_LEGEND:
   //      if ( metaDC.CreateEnhanced( pDevDC, filename, NULL, "MapWindow\0MapLegend\0\0" ) == TRUE )
   //         {
   //         InitMapping( metaDC );
   //
   //         DrawLegend( metaDC );
   //         HENHMETAFILE hEnhMetaFile = metaDC.CloseEnhanced( );
   //         }
   //      break;
   //   }

   for (int j=0; j < layerCount; j++ )
      m_pMap->GetLayer(j)->m_showLegend = showLegendArray[j];

   delete [] showLegendArray;

   ReleaseDC( pDevDC );
   }


bool MapWindow::LoadBkgrImage( void )
   {
   ASSERT( m_pMap != NULL );
   if ( m_pMap == NULL )
      return false;

	if (m_pMap->m_bkgrPath.IsEmpty())
		return false;

   return LoadBkgrImage( m_pMap->m_bkgrPath, m_pMap->m_bkgrLeft, m_pMap->m_bkgrTop,  m_pMap->m_bkgrRight,  m_pMap->m_bkgrBottom );
   }


bool MapWindow::LoadBkgrImage( LPCTSTR url, REAL xLeft, REAL yTop, REAL xRight, REAL yBottom )
   {
   if ( m_pBkgrImage == NULL )
      m_pBkgrImage = new CImage;

   CString filename;
   HRESULT result;

   // is this a WMS entry?
   //if ( _tcsnicmp( url, "wms:", 4 ) == 0 )
   //   {
	//   if (m_pMapCache->hasImage(yTop, yBottom, xLeft, xRight)) {
	//		//use a previously cached image
	//	   m_pBkgrImage = m_pMapCache->getImage(yTop, yBottom, xLeft, xRight);
	//   }
	//   else {
	//	   //go get a new image
	//	   
	//	   GDALWrapper m_gdal;
	//	   GeoSpatialDataObj m_geo;
	//	   GDALDataType  m_DataType = GDT_Unknown;
	//	   CString prjWKT = (LPCTSTR) m_pMap->m_mapLayerArray[0].pLayer->m_projection;
	//	   //cast LPCTSTR to string in order to access spliting & parsing functionality
	//	   std::string tmp_string(url);
   //
	//	   //grab everything after the "wms:"
	//	   std::string url_string = tmp_string.substr(4, string::npos);
   //
	//	   //convert url back to LPCTSTR
	//	   LPCTSTR m_GeoFilename = url_string.c_str();
   //      LPCTSTR m_outGeoFilename = "\\Envision\\StudyAreas\\WW2100\\gdal_baseMap.jpg";
   //
	//	   m_geo.GeoImage2GeoTransWarpImage(m_GeoFilename, m_outGeoFilename, prjWKT);
   //
	//	   filename = m_outGeoFilename;
   //
	//	   result = m_pBkgrImage->Load(filename);
   //
	//	   //store the image in the cache for later
	//	   m_pMapCache->storeImage(m_pBkgrImage, yTop, yBottom, xLeft, xRight);
   //
	//	   if (result == E_FAIL)
	//	      {
	//		   CString msg("Unable to load background image '");
	//		   msg += url;
	//		   msg += "'";
	//		   Report::Log(msg);
	//	      }
	//      }
   //   }
   //else 
      {
	   filename = url;

	   result = m_pBkgrImage->Load(filename);

	   if (result == E_FAIL)
	      {
		   CString msg("Unable to load background image '");
		   msg += url;
		   msg += "'";
		   Report::Log(msg);
	      }
      }

   this->m_bkgrImageUL.x = xLeft;
   this->m_bkgrImageUL.y = yTop;
   this->m_bkgrImageLR.x = xRight;
   this->m_bkgrImageLR.y = yBottom;
   this->m_showBkgrImage = true;
   this->m_bkgrImageFilename = filename;

   return result != E_FAIL;
   }


bool MapWindow::CopyBkgrImage( MapWindow *pSourceMap )
   {
   if ( m_pBkgrImage == NULL )
      m_pBkgrImage = new CImage;

   HRESULT result = m_pBkgrImage->Load( pSourceMap->m_bkgrImageFilename );

   if ( result == E_FAIL )
      {
      CString msg( "Unable to load background image '" );
      msg += pSourceMap->m_bkgrImageFilename;
      msg += "'";
      Report::Log( msg );
      }

   this->m_bkgrImageUL.x = pSourceMap->m_bkgrImageUL.x;
   this->m_bkgrImageUL.y = pSourceMap->m_bkgrImageUL.y;
   this->m_bkgrImageLR.x = pSourceMap->m_bkgrImageLR.x;
   this->m_bkgrImageLR.y = pSourceMap->m_bkgrImageLR.y;
   this->m_showBkgrImage = pSourceMap->m_showBkgrImage;

   return result != E_FAIL;
   }

//------------------------------------------------------------------
// DrawMap() draws the layers contained in the map
//
// Drawing is based on what Bins are associated with the polygons
// be drawn, and assumes the correct bins have been loaded into the
// Polygons.  This is normally done when the active layer is set
// 
// NOTE: 2/26/08 change:
// Moved initMapping out of this code (to caller codes) to allow support
//  for double-buffered (CMemDC) device contexts
//------------------------------------------------------------------

void MapWindow::DrawMap( CDC &pdc ) //, bool initMapping /*=true*/ )
   {   
   // draw map
   InitMapping( pdc );  // set up logical coordinate system
   MemDC dc( &pdc );
  
   DrawBackground( dc );

   DrawMapToDeviceContext( dc, false, 3 );
   }


void MapWindow::DrawBackground( CDC &dc )
   {
   CRect rect;
   m_pMap->GetDisplayRect( rect );
   //GetClientRect( &rect );

   dc.DPtoLP( &rect );

   Gdiplus::Graphics graphics(dc.m_hDC);    // create a graphics surface

   // first, draw background
   Gdiplus::Point p1( rect.left, rect.bottom );
   Gdiplus::Point p2( rect.left, rect.top );
   Gdiplus::Color color1( 0xcc, 0xcc, 0xcc );
   Gdiplus::Color color2( 0xff, 0xff, 0xff );
   
   Gdiplus::LinearGradientBrush linGrBrush( p1, p2, color1, color2 );
   Gdiplus::Rect gRect(rect.left, rect.bottom, abs(rect.right-rect.left)+1, abs(rect.bottom-rect.top)+1);
   graphics.FillRectangle( &linGrBrush, gRect );  

   // if background image defined, show it
   if ( m_showBkgrImage && m_pBkgrImage != NULL )
      {
      POINT ul, lr;  // logical
      m_pMap->VPtoLP( m_bkgrImageUL.x, m_bkgrImageUL.y, ul );
      m_pMap->VPtoLP( m_bkgrImageLR.x, m_bkgrImageLR.y, lr );

      int lwidth = lr.x - ul.x;
      int lheight = ul.y - lr.y;

      Gdiplus::Image *imx = Gdiplus::Bitmap::FromHBITMAP( m_pBkgrImage->operator HBITMAP(), NULL );
      Gdiplus::RectF r( Gdiplus::REAL( ul.x ), Gdiplus::REAL( ul.y ), Gdiplus::REAL( lwidth ), Gdiplus::REAL( -lheight ) );

      graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQuality);
      graphics.DrawImage(imx, r ); //, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, Gdiplus::UnitPixel); 
      
      delete imx;
      }

   graphics.ReleaseHDC( dc.m_hDC );
   }


void MapWindow::DrawMapToDeviceContext( CDC &dc, bool initMapping /*=true*/, int flag /*=3*/ )
   {
   // note:  flag & 1 - draw map (using bin colors)
   //        flag & 2 - draw legend
   //        flag & 4 - use index of polyon for color
   //        flag & 8 - use value of polygon for color
   //        flag & 16 - draw text
   CRect rect;
   m_pMap->GetDisplayRect( rect );
   //GetClientRect( &rect );

   if( initMapping == true )
      InitMapping( dc );  // set up logical coordinate system

   if ( flag & 1 )  // draw map?
      {
      // draw each item...
      for ( int layer=0; layer < (int) m_pMap->m_drawOrderArray.GetSize(); layer++ )
         {
         int oldBkMode = dc.SetBkMode( OPAQUE );

         MapLayer *pLayer = m_pMap->m_drawOrderArray[ layer ];

         if ( pLayer->IsVisible() )
            {
            DRAW_VALUE dv = DV_COLOR;
            if ( flag & 4 ) 
               dv = DV_INDEX;
            else if ( flag & 8 )
               dv = DV_VALUE;

            BinArray *pBinArray = pLayer->GetBinArray( USE_ACTIVE_COL, false );

            if ( m_useGdiPlus && ( pLayer->m_transparency > 0 || ( pBinArray != NULL && pBinArray->m_hasTransparentBins ) ) )
                DrawLayer_Gdiplus( dc, pLayer, false, dv );
            else
                DrawLayer( dc, pLayer, false, dv );
            }

         dc.SetBkMode( oldBkMode );
         }  // end of:  for ( layer < m_mapLayerArray.GetSize()
      
      DrawLabels( dc );

      if ( m_showScale )
         DrawScale( dc );

      DrawSelection( dc );
      }

   // Draw Legend;
   if ( flag & 2 )
      DrawLegend( dc );

 //   if ( flag && 16 )
      DrawMapText( dc, false );
   }


void MapWindow::DrawLayer( CDC &dc, MapLayer *pLayer, bool initMapping /*=true*/, DRAW_VALUE drawValue /*=DV_COLOR*/ )
   {  
   // draws a layer to the device context, using the polygon's ptArray (no remapping done)
   if ( initMapping == true )
      InitMapping( dc );  // set up logical coordinate system

   // initialize device context with defaul pen/brush
   HPEN hNullPen = CreatePen( PS_NULL, 0, BLACK );
   HPEN hOldPen  = (HPEN) SelectObject( dc.m_hDC, (HGDIOBJ) hNullPen );
   
   HBRUSH hWhiteBrush = CreateSolidBrush( WHITE );
   HBRUSH hOldBrush   = (HBRUSH) SelectObject( dc.m_hDC, (HGDIOBJ) hWhiteBrush );

   // device context set up, start drawing polygons
   Bin *pLastBin = NULL;

   // infrastructure to handle polygons with parts
   int maxPolyParts = 0;    // used to keep m_maxPolyParts up-to-date
   int growBy       = 10;   // if partCounts becomes to small, it will grow by this amount
   LPINT partCounts = NULL; // dynamic array of ints, hold the number of vertices in each part of the current poly, passed to CDC::PolyPolygon(.)

   if ( m_maxPolyParts > 0 )
      partCounts = new int[ m_maxPolyParts ];

   BinArray *pBinArray = pLayer->GetBinArray( USE_ACTIVE_COL, false );

   switch( pLayer->GetType() )
      {  
      case LT_POLYGON:
      case LT_LINE:  // also uses polygons
         {
         // set up pens and brushes for drawing polygons/lines
         HPEN   hPolyPen   = NULL;
         HBRUSH hPolyBrush = NULL;

         if ( pLayer->m_outlineColor != NO_OUTLINE && pLayer->m_hatchStyle < 0 )
            {
            hPolyPen = CreatePen( PS_SOLID, pLayer->m_lineWidth, pLayer->m_outlineColor );
            SelectObject( dc.m_hDC, (HGDIOBJ) hPolyPen );
            }
         else if ( pLayer->m_hatchStyle >= 0 )
            {
            hPolyBrush = CreateHatchBrush( pLayer->m_hatchStyle, pLayer->m_hatchColor );
            SelectObject( dc.m_hDC, (HGDIOBJ) hPolyBrush );
            dc.SetBkMode( TRANSPARENT );
            pLastBin = NULL;
            }
         else
            SelectObject( dc.m_hDC, (HGDIOBJ) hNullPen );

         pLastBin = (Bin*) 0xFFFFFF;

         TYPE type = pLayer->GetFieldType( USE_ACTIVE_COL );
        
         // cycle through polygons
         //for ( MapLayer::Iterator i = pLayer->Begin(); i != end; i++ )   // Note: iterator breaks if no dataobj loaded - use IsDefunct() check instead.
         int polyCount = pLayer->GetPolygonCount(); 
         for ( INT_PTR i=0; i < polyCount; i++ )
            {
            if ( pLayer->IsDefunct( (int) i ) )
               continue;
   
            Poly *pPoly = pLayer->m_pPolyArray->GetAt( i );    // get the polygon

            // set up color
            Bin *pBin = NULL;
            if ( drawValue == DV_INDEX )
               pBin = (Bin*) i;
            else  // drawValue = DV_COLOR or DV_VALUE
               pBin = pLayer->GetPolyBin( (int) i ); //pPoly->m_pBin;

            HBRUSH hNewBrush = NULL;
            HPEN hNewPen = NULL;

            // select new brush if needed...
            if ( pBin != pLastBin )
               {
               if ( pLayer->GetType() == LT_POLYGON )
                  {
                  hNewBrush = NULL;

                  if ( drawValue == DV_INDEX )
                     {
                     COLORREF color = COLORREF( i ) ;
                     hNewBrush = (HBRUSH) ::CreateSolidBrush( COLORREF( i ) );
                     }
                  else
                     {
                     if ( ( pLayer->GetBinColorFlag() == BCF_TRANSPARENT  ) || ( pBin && pBin->m_color == -1 ) )
                        hNewBrush = (HBRUSH) GetStockObject( NULL_BRUSH );
                     else if ( pBin == NULL || pBin->m_color == -1 )  // no data for this polygon
                        hNewBrush = (HBRUSH) GetStockObject( NULL_BRUSH );
               
                     //else if ( pLayer->m_hatchStyle != -1 ) // draw with a brush?
                     //   {
                     //   hNewBrush = CreateHatchBrush( pLayer->m_hatchStyle, pLayer->m_hatchColor );
                     //   bkMode = dc.SetBkMode( TRANSPARENT );
                     //   }
                     else if ( drawValue == DV_VALUE && ::IsString( type ) == false )
                        {
                        if ( ::IsInteger( type ) )
                           {
                           int value = (int)( ( pBin->m_minVal + pBin->m_maxVal ) / 2 );
                           COLORREF color = COLORREF( value );
                           hNewBrush = CreateSolidBrush( color );
                           }
                        else
                           ASSERT( 0 );
                           //hNewBrush = CreateSolidBrush( (COLORREF) (float) ((pBin->m_maxVal + pBin->m_minVal) / 2 ) );
                        }
                     else
                        hNewBrush = CreateSolidBrush( pBin->m_color );
                     }

                  hOldBrush = (HBRUSH) SelectObject( dc.m_hDC, (HGDIOBJ) hNewBrush );
                  DeleteObject( (HGDIOBJ) hOldBrush );
                  }

               else // layerType == LT_LINE:
                  {
                  hNewPen = NULL;
                  int width = pLayer->m_lineWidth;

                  if ( pBin != NULL )
                     {
                     // is the line variable with (meaning, width is scaled using the current Bins
                     // maximum value to 
                     if ( pLayer->m_useVarWidth && ::IsNumeric( pBinArray->m_type ) )
                        width = int( pBin->m_maxVal * pLayer->m_maxVarWidth / pBinArray->m_binMax );

                     hNewPen = CreatePen( PS_SOLID, width, pBin->m_color );
                     hOldPen = (HPEN) SelectObject( dc.m_hDC, (HGDIOBJ) hNewPen );
                     DeleteObject( (HGDIOBJ) hOldPen );
                     }
                  else  // NULL bin 
                     {
                     hNewPen = CreatePen( PS_SOLID, width, RGB( 250, 250, 250 ) );
                     hOldPen = (HPEN) SelectObject( dc.m_hDC, (HGDIOBJ) hNewPen );
                     DeleteObject( (HGDIOBJ) hOldPen );
                     }
                 }

               pLastBin = pBin;
               }

            // draw the polygon
            int parts = (int) pPoly->m_vertexPartArray.GetSize();

            if ( parts > 0 )  // handle NULL polygons
               {
               int startPart = pPoly->m_vertexPartArray[ 0 ];
               int vertexCount = 0;
               int nextPart = 0;

               switch ( pLayer->m_layerType )
                  {
                  case LT_POLYGON:
                     {
                     // memory management for partCounts
                     if ( maxPolyParts < parts )
                        maxPolyParts = parts;
                     
                     if ( m_maxPolyParts < parts )
                        {
                        while ( m_maxPolyParts < parts )
                           m_maxPolyParts += growBy;
                           
                        if ( partCounts != NULL )
                           delete [] partCounts;
                        
                        partCounts = new int[ m_maxPolyParts ];
                        }

                     for ( int part=0; part < parts; part++ )
                        {
                        if ( part < parts-1 )
                           {
                           nextPart = pPoly->m_vertexPartArray[ part+1 ];
                           vertexCount =  nextPart - startPart;
                           }
                        else
                           vertexCount = pPoly->GetVertexCount() - startPart;
                                 
                        partCounts[part] = vertexCount;
                        startPart = nextPart;
                        }

                     // Note: documentation for CDC::PolyPolygon(.) says it should not be used for a 1 part poly.
                     if ( parts > 1 )
                        dc.PolyPolygon( pPoly->m_ptArray, partCounts, parts );
                     else
                        dc.Polygon( pPoly->m_ptArray, partCounts[0] );
                     }
                     break;

                  case LT_LINE:
                     {
                     for ( int part=0; part < parts; part++ )
                        {
                        if ( part < parts-1 )  // more that one part, but not last part?
                           {
                           nextPart = pPoly->m_vertexPartArray[ part+1 ];
                           vertexCount =  nextPart - startPart;
                           }
                        else   // one part, or last part
                           vertexCount = pPoly->GetVertexCount() - startPart;

                        dc.Polyline( pPoly->m_ptArray+startPart, vertexCount );
                        startPart = nextPart;
                        }
                     }
                     break;
                  }  // end of: switch( m_layerType )
               }  // end of: if ( parts > 0 )

            // delete created pens and brushes
            if ( hNewPen ) DeleteObject( hNewPen );
            if ( hNewBrush ) DeleteObject( hNewBrush );

            }  // end of:  for ( i < polyArray.GetSize() )

         // delete created pens and brushes
         if ( hPolyPen )   DeleteObject( hPolyPen );
         if ( hPolyBrush ) DeleteObject( hPolyBrush );

         }
         break;   // end of case LT_POLYGON/LT_LINE

      case LT_GRID:
         {
         DataObj *pData = pLayer->m_pData;
         ASSERT( pData != NULL );

         int rows = pData->GetRowCount();
         int cols = pData->GetColCount();

         // get logical cell sizes for drawing
         POINT ll;
         m_pMap->VPtoLP( pLayer->m_vxMin, pLayer->m_vyMin, ll );  // convert the lower left corner to logical points
         float width  = m_pMap->VDtoLD_float( (pLayer->m_vxMax - pLayer->m_vxMin ) / cols );
         float height = m_pMap->VDtoLD_float( (pLayer->m_vyMax - pLayer->m_vyMin ) / rows );

         //CBrush br;
         //br.CreateSolidBrush( RGB( 100, 100, 100 ) );
         //CGdiObject *pOldBrush = dc.SelectObject( &br );

         // ready to go, start drawing...
         for ( int row=0; row < rows; row++ )
            {
            int y0 = ll.y + (int) ( (rows-row-1) * height );
            int y1 = y0 + (int) height + 1;

            for ( int col=0; col < cols; col++ )
               {
               int x0 = ll.x + (int)( col*width );
               int x1 = x0 + (int) width + 1;
                        
               float value;
               bool ok = pData->Get( col, row, value );
               if (  ok )
                  {
                  if ( value != pLayer->GetNoDataValue() )
                     {                                   
                     Bin *pBin = pLayer->GetDataBin( USE_ACTIVE_COL, value );
                     if ( pBin != NULL )
                        dc.FillSolidRect( x0, y0, x1-x0, y1-y0, pBin->m_color );
                     //dc.Rectangle( x0, y0, x1, y1 );
                     }
                  }
               }
            }
         
         // draw rect around whole thing
         //SelectObject( dc.m_hDC, GetStockObject( NULL_BRUSH ) );
         //SelectObject( dc.m_hDC, GetStockObject( BLACK_PEN ) );
         //dc.Rectangle( ll.x, ll.y, ll.x+width*cols, ll.y+height*rows );            
         //dc.SelectObject( pOldBrush );
         }
         break;

      case LT_POINT:
         {
         // drawn points can use classification in two ways:
         // 1) by the size of the point (if m_useVarWidth is set )
         // 2) by the color of the point (normal classification)

         // get the original pen/brush
         CGdiObject *pOldPen   = dc.SelectStockObject( BLACK_PEN );
         CGdiObject *pOldBrush = dc.SelectStockObject( BLACK_BRUSH );

         for ( int i=0; i < pLayer->m_pPolyArray->GetSize(); i++ )
            {
            Poly *pPoly = pLayer->m_pPolyArray->GetAt( i );    // get the polygon representing this point

            // set up color
            Bin *pBin = pLayer->GetPolyBin( i );

            // select new brush if needed...
            if ( pBin != pLastBin )
               {
               HPEN   hNewPen   = 0;
               HBRUSH hNewBrush = 0;

               if ( pBin != NULL )
                  {
                  if ( pBin->m_color == -1 )
                     {
                     hNewPen   = (HPEN)   GetStockObject( NULL_PEN );
                     hNewBrush = (HBRUSH) GetStockObject( NULL_BRUSH );
                     }
                  else
                     {
                     hNewPen   = CreatePen( PS_SOLID, 0, pBin->m_color );
                     hNewBrush = CreateSolidBrush( pBin->m_color );
                     }
                  }
               else  // bin doesn't exist, use fill, outline colors
                  {
                  hNewPen   = CreatePen( PS_SOLID, 0, pLayer->m_outlineColor );
                  hNewBrush = CreateSolidBrush( pLayer->m_fillColor );
                  }

               hOldPen   = (HPEN)   SelectObject( dc.m_hDC, (HGDIOBJ) hNewPen );
               hOldBrush = (HBRUSH) SelectObject( dc.m_hDC, (HGDIOBJ) hNewBrush );

               DeleteObject( (HGDIOBJ) hOldPen );
               DeleteObject( (HGDIOBJ) hOldBrush );
                  
               pLastBin = pBin;
               }

            ASSERT( pPoly->GetVertexCount() == 1 );
            ASSERT( pPoly->m_ptArray != NULL );

            int ptSize = 300;

            if ( pBin != NULL )
               {
               if ( pLayer->m_useVarWidth && IsNumeric( pBinArray->m_type ) )
                  {
                  float scalar = ( (pBin->m_maxVal+pBin->m_minVal)/2) * pLayer->m_maxVarWidth / pBinArray->m_binMax;
                  ptSize = int( ptSize*scalar );
                  }

              if ( ptSize > 0 )
                  {
                  int x0 = pPoly->m_ptArray[ 0 ].x-ptSize;
                  int y0 = pPoly->m_ptArray[ 0 ].y-ptSize;
                  int x1 = pPoly->m_ptArray[ 0 ].x+ptSize;
                  int y1 = pPoly->m_ptArray[ 0 ].y+ptSize;
                  dc.Ellipse( x0, y0, x1, y1 );
                  }
               }
            else  // no bin, use defaults
               {
               int activeField = pLayer->GetActiveField();
               COLORREF color = pLayer->m_fillColor;

               if ( pLayer->m_useVarWidth && IsNumeric( pLayer->GetFieldType( activeField ) ) )
                  {
                  float value;
                  pLayer->GetData( i, activeField, value );
                  float scalar = value / pLayer->m_maxVarWidth;
                  ptSize = int( ptSize*scalar );
                  }

              if ( ptSize > 0 )
                  {
                  int x0 = pPoly->m_ptArray[ 0 ].x-ptSize;
                  int y0 = pPoly->m_ptArray[ 0 ].y-ptSize;
                  int x1 = pPoly->m_ptArray[ 0 ].x+ptSize;
                  int y1 = pPoly->m_ptArray[ 0 ].y+ptSize;
                  dc.Ellipse( x0, y0, x1, y1 );
                  }
               }

            }  // end of: for ( i < polyArray.GetSize() )

         dc.SelectObject( pOldPen );
         dc.SelectObject( pOldBrush );
         }  // end of:  case LT_POINT
         break;
      }

   // memory management for partCounts
   if ( partCounts != NULL )
      delete [] partCounts;

   m_maxPolyParts = maxPolyParts;

   // restore device context to original settings
   SelectObject( dc.m_hDC, (HGDIOBJ) hOldPen );
   SelectObject( dc.m_hDC, (HGDIOBJ) hOldBrush ); 

   DeleteObject( hNullPen );
   DeleteObject( hWhiteBrush );

   if (hOldPen) DeleteObject(hOldPen);
   if (hOldBrush) DeleteObject(hOldBrush);
      
   return;
   }

   
void MapWindow::DrawLayer_Gdiplus( CDC &dc, MapLayer *pLayer, bool initMapping /*=true*/, DRAW_VALUE drawValue /*=DV_COLOR*/ )
   {
   // draws a layer to the device context, using the polygon's ptArray (no remapping done)
   if ( initMapping == true )
      InitMapping( dc );  // set up logical coordinate system

   Gdiplus::Graphics graphics( dc.m_hDC);
   // initialize device context
   std::auto_ptr<Gdiplus::Pen> pPen(NULL);
   std::auto_ptr<Gdiplus::Brush> pBrush(new Gdiplus::SolidBrush(Gdiplus::Color::White));

   // device context set up, start drawing polygons
   Bin *pLastBin = NULL;

   // infrastructure to handle polygons with parts
   int maxPolyParts = 0;    // used to keep m_maxPolyParts up-to-date
   int growBy       = 10;   // if partCounts becomes to small, it will grow by this amount
   LPINT partCounts = NULL; // dynamic array of ints, hold the number of vertices in each part of the current poly, passed to CDC::PolyPolygon(.)

   if ( m_maxPolyParts > 0 )
      partCounts = new int[ m_maxPolyParts ];

   //int maxPolyVertices = 200;
   //Gdiplus::Point* points = new Gdiplus::Point[maxPolyVertices];

   BinArray *pBinArray = pLayer->GetBinArray( USE_ACTIVE_COL, false );
   int alpha = -255*pLayer->GetTransparency()/100 + 255;  // return a number between 0 (transparent) and 255 (opaque)
   
   switch( pLayer->GetType() )
      {
      case LT_POLYGON:
      case LT_LINE:  // also uses polygons
         {
         // set up the Pen for drawing the poly outline
         if ( pLayer->m_outlineColor != NO_OUTLINE && pLayer->m_hatchStyle < 0 )
            {    
            //pPen.reset( new Gdiplus::Pen(Gdiplus::Color(GetRValue(pLayer->m_outlineColor), GetGValue(pLayer->m_outlineColor), GetBValue(pLayer->m_outlineColor)), pLayer->m_lineWidth) );
            pPen.reset( new Gdiplus::Pen(Gdiplus::Color(alpha, GetRValue(pLayer->m_outlineColor), GetGValue(pLayer->m_outlineColor), GetBValue(pLayer->m_outlineColor)), (float)pLayer->m_lineWidth) );
            }
         else if ( pLayer->m_hatchStyle >= 0 )
            {            
             Gdiplus::Color color( alpha, GetRValue(pLayer->m_hatchColor), GetGValue(pLayer->m_hatchColor), GetBValue(pLayer->m_hatchColor) );
             //color.SetFromCOLORREF(pLayer->m_hatchColor);
             pBrush.reset( new Gdiplus::HatchBrush( (Gdiplus::HatchStyle)(pLayer->m_hatchStyle), color, Gdiplus::Color::Transparent ) );             
             pLastBin = NULL;
            }
        else
            {
             pPen.reset(NULL);
            }

        pLastBin = (Bin*) 0xFFFFFF;

        TYPE type = pLayer->GetFieldType( USE_ACTIVE_COL );
        
        // cycle through polygons
        //for ( MapLayer::Iterator i = pLayer->Begin(); i != end; i++ )   // Note: iterator breaks if no dataobj loaded - use IsDefunct() check instead.
        int polyCount = pLayer->GetPolygonCount(); 
        for ( INT_PTR i=0; i < polyCount; i++ )
            {
            if ( pLayer->IsDefunct( (int) i ) )
               continue;
   
            Poly *pPoly = pLayer->m_pPolyArray->GetAt( i );    // get the polygon

            // set up color
            Bin *pBin;
            if ( drawValue == DV_INDEX )
               pBin = (Bin*) i;
            else  // drawValue = DV_COLOR or DV_VALUE
               pBin = pLayer->GetPolyBin( (int) i ); 

            // select new brush if needed...
            if ( pBin != pLastBin )
               {
               if ( pLayer->GetType() == LT_POLYGON )
                  {
                  if ( drawValue == DV_INDEX )
                     {
                     COLORREF color = COLORREF( i ) ;                    
                     Gdiplus::Color c;
                     c.SetFromCOLORREF(color);
                     pBrush.reset( new Gdiplus::SolidBrush(c) );
                     }
                  else
                     {
                     // if layer is transparent or bin has no color
                     if ( pLayer->GetBinColorFlag() == BCF_TRANSPARENT || ( pBin && pBin->m_color == -1 ) )
                        {
                        pBrush.reset( NULL );
                        }
                     else if ( pBin == NULL )  // no data for this polygon
                        {
                        pBrush.reset( NULL );
                        }
               
                     //else if ( pLayer->m_hatchStyle != -1 ) // draw with a brush?
                     //   {
                     //   hNewBrush = CreateHatchBrush( pLayer->m_hatchStyle, pLayer->m_hatchColor );
                     //   bkMode = dc.SetBkMode( TRANSPARENT );
                     //   }
                     else if ( drawValue == DV_VALUE && ::IsString( type ) == false )
                        {
                        if ( ::IsInteger( type ) )
                           {
                           int value = (int)( ( pBin->m_minVal + pBin->m_maxVal ) / 2 );

                           Gdiplus::Color c( alpha, GetRValue( value ), GetGValue( value ), GetBValue( value ) );
                           //COLORREF color = COLORREF( value );
                           //Gdiplus::Color c;
                           //c.SetFromCOLORREF( color );
                           pBrush.reset( new Gdiplus::SolidBrush(c));
                           }
                        else
                           ASSERT( 0 );
                        }
                     else   // data exist, bin exists, use bin color
                        {
                        float opacity = ( 100-pBin->m_transparency )/100.0f;  //0=transparent, 1=opaque 
                        int binAlpha = int( alpha * opacity );

                        Gdiplus::Color c( binAlpha, GetRValue( pBin->m_color ), GetGValue( pBin->m_color ), GetBValue( pBin->m_color ) );
                        //Gdiplus::Color c;
                        //c.SetFromCOLORREF( pBin->m_color );
                        pBrush.reset( new Gdiplus::SolidBrush( c ) );
                        }
                     }
                  }

               else // layerType == LT_LINE:
                  {
                  float width = (float) pLayer->m_lineWidth;

                  if ( pBin != NULL )
                     {
                     if ( pLayer->m_useVarWidth && ::IsNumeric( pBinArray->m_type ) )
                        width =  pBin->m_maxVal * pLayer->m_maxVarWidth / pBinArray->m_binMax;

                     //Gdiplus::Color c;
                     //c.SetFromCOLORREF( pBin->m_color );
                     Gdiplus::Color c( alpha, GetRValue( pBin->m_color ), GetGValue( pBin->m_color ), GetBValue( pBin->m_color ) );
                     pPen.reset( new Gdiplus::Pen( c, width ) );
                     }
                  else  // NULL bin 
                     {
                     pPen.reset( new Gdiplus::Pen( Gdiplus::Color::White, width ) );
                     }
                 }

               pLastBin = pBin;
               }
           
            // draw the polygon
            int parts = (int) pPoly->m_vertexPartArray.GetSize();

            if ( parts > 0 )  // handle NULL polygons
               {
               int startPart = pPoly->m_vertexPartArray[ 0 ];
               int vertexCount = 0;
               int nextPart = 0;

               if ( pLayer->m_layerType == LT_POLYGON || pLayer->m_layerType == LT_LINE )
                  {
                  // memory management for partCounts
                  if ( maxPolyParts < parts )
                     maxPolyParts = parts;
                     
                  if ( m_maxPolyParts < parts )
                     {
                     while ( m_maxPolyParts < parts )
                        m_maxPolyParts += growBy;
                        
                     if ( partCounts != NULL )
                        delete [] partCounts;
                     
                     partCounts = new int[ m_maxPolyParts ];
                     }

                  for ( int part=0; part < parts; part++ )
                     {
                     if ( part < parts-1 )
                        {
                        nextPart = pPoly->m_vertexPartArray[ part+1 ];
                        vertexCount =  nextPart - startPart;
                        }
                     else
                        vertexCount = pPoly->GetVertexCount() - startPart;
                              
                     partCounts[part] = vertexCount;
                     startPart = nextPart;
                     }
                  }
               
               switch ( pLayer->m_layerType )
                  {
                  case LT_POLYGON:
                     {
                     int last = 0;
                     
                     for( int part = 0; part < parts; part++ )
                        {
                        /*
                        if( partCounts[part] > maxPolyVertices )
                           {
                           delete []points;
                           points = new Gdiplus::Point[partCounts[part]];
                           maxPolyVertices = partCounts[part];
                           }
                        for( int j = 0; j < partCounts[part]; j++ )
                           {
                           points[j].X = pPoly->m_ptArray[last].x;
                           points[j].Y = pPoly->m_ptArray[last].y;
                           last++;
                           } */


                        if( pBrush.get() != NULL )
                           {
                           //graphics.FillPolygon( pBrush.get(), points, partCounts[part] );
                           graphics.FillPolygon( pBrush.get(), (Gdiplus::Point*)(pPoly->m_ptArray+last), partCounts[part] );
                           }
                        if( pPen.get() != NULL )
                           {
                           //graphics.DrawPolygon( pPen.get(), points, partCounts[part] );
                           graphics.DrawPolygon( pPen.get(), (Gdiplus::Point*)(pPoly->m_ptArray+last), partCounts[part] );
                           }

                        last += partCounts[ part ];
                        }
                     }
                     break;

                  case LT_LINE:
                     {
                     int last = 0;

                     for ( int part=0; part < parts; part++ )
                        {
                        if ( pPen.get() != NULL )
                           {
                           //graphics.DrawPolygon( pPen.get(), points, vertexCount );
                           //graphics.DrawPolygon( pPen.get(), (Gdiplus::Point*)(pPoly->m_ptArray+last), partCounts[part] );
                           graphics.DrawLines( pPen.get(), (Gdiplus::Point*)(pPoly->m_ptArray+last), partCounts[part] );
                           }

                        last += partCounts[ part ];
                        }
                     }
                     break;

                  }  // end of: switch( m_layerType )
               }  // end of: if ( parts > 0 )
            }  // end of:  for ( i < polyArray.GetSize() )
         }
         break;   // end of case LT_POLYGON/LT_LINE

      case LT_GRID:
         {
         DataObj *pData = pLayer->m_pData;
         ASSERT( pData != NULL );

         int rows = pData->GetRowCount();
         int cols = pData->GetColCount();

         // get logical cell sizes for drawing
         POINT ll;
         m_pMap->VPtoLP( pLayer->m_vxMin, pLayer->m_vyMin, ll );  // convert the lower left corner to logical points
         float width  = m_pMap->VDtoLD_float( (pLayer->m_vxMax - pLayer->m_vxMin ) / cols );
         float height = m_pMap->VDtoLD_float( (pLayer->m_vyMax - pLayer->m_vyMin ) / rows );

         //CBrush br;
         //br.CreateSolidBrush( RGB( 100, 100, 100 ) );
         //CGdiObject *pOldBrush = dc.SelectObject( &br );

         // ready to go, start drawing...
         for ( int row=0; row < rows; row++ )
            {
            int y0 = ll.y + (int) ( (rows-row-1) * height );
            int y1 = y0 + (int) height + 1;

            for ( int col=0; col < cols; col++ )
               {
               int x0 = ll.x + (int)( col*width );
               int x1 = x0 + (int) width + 1;
                        
               float value;
               bool ok = pData->Get( col, row, value );
               if (  ok )
                  {
                  if ( value != pLayer->GetNoDataValue() )
                     {                                   
                     Bin *pBin = pLayer->GetDataBin( USE_ACTIVE_COL, value );
                     if (pBin != NULL)
                        {
                        if (pBin->m_transparency > 0)
                           {
                           //int _alpha = -255 * pBin->m_transparency / 100 + 255;  // return a number between 0 (transparent) and 255 (opaque)
                           //Gdiplus::Color c(alpha, GetRValue(pBin->m_color), GetGValue(pBin->m_color), GetBValue(pBin->m_color));
                           //Gdiplus::SolidBrush solidBrush(c);
                           //graphics.FillRectangle(&solidBrush, x0, y0, x1 - x0, y1 - y0);
                           }
                        else
                           {
                           /* Gdiplus::Color c;
                           c.SetFromCOLORREF( pBin->m_color );*/
                           Gdiplus::Color c(alpha, GetRValue(pBin->m_color), GetGValue(pBin->m_color), GetBValue(pBin->m_color));
                           Gdiplus::SolidBrush solidBrush(c);
                           graphics.FillRectangle(&solidBrush, x0, y0, x1 - x0, y1 - y0);
                           }
                        }
                     }
                  }
               }
            }
         
         // draw rect around whole thing
         //SelectObject( dc.m_hDC, GetStockObject( NULL_BRUSH ) );
         //SelectObject( dc.m_hDC, GetStockObject( BLACK_PEN ) );
         //dc.Rectangle( ll.x, ll.y, ll.x+width*cols, ll.y+height*rows );            
         //dc.SelectObject( pOldBrush );
         }
         break;

      case LT_POINT:
         {
         // drawn points can use classification in two ways:
         // 1) by the size of the point (if m_useVarWidth is set )
         // 2) by the color of the point (normal classification)

         // get the original pen/brush
         std::auto_ptr<Gdiplus::Pen> pCurrPen( new Gdiplus::Pen( Gdiplus::Color::Black, 0 ) );
         std::auto_ptr<Gdiplus::Brush> pCurrBrush( new Gdiplus::SolidBrush( Gdiplus::Color::Black ) );
         
         for ( int i=0; i < pLayer->m_pPolyArray->GetSize(); i++ )
            {
            Poly *pPoly = pLayer->m_pPolyArray->GetAt( i );    // get the polygon representing this point

            // set up color
            Bin *pBin = pLayer->GetPolyBin( i ); //pPoly->m_pBin;

            // select new brush if needed...
            if ( pBin != pLastBin )
               {         
               if ( pBin != NULL )
                  {
                  if ( pBin->m_color == -1 )
                     {
                     pCurrPen.reset();
                     pCurrBrush.reset();
                     }
                  else
                     {
                     Gdiplus::Color c;
                     c.SetFromCOLORREF( pBin->m_color );
                     pCurrPen.reset( new Gdiplus::Pen( c, 0 ) );
                     pCurrBrush.reset( new Gdiplus::SolidBrush( c ) );
                     }
                  }                  
               pLastBin = pBin;
               }

            ASSERT( pPoly->GetVertexCount() == 1 );
            ASSERT( pPoly->m_ptArray != NULL );

            if ( pBin != NULL )
               {
               int ptSize = 30;

               if ( pLayer->m_useVarWidth && IsNumeric( pBinArray->m_type ) )
                  {
                  float scalar = ( (pBin->m_maxVal+pBin->m_minVal)/2) * pLayer->m_maxVarWidth / pBinArray->m_binMax;
                  ptSize = int( ptSize*scalar );
                  }

              if ( ptSize > 0 )
                  {
                  int x0 = pPoly->m_ptArray[ 0 ].x-ptSize;
                  int y0 = pPoly->m_ptArray[ 0 ].y-ptSize;
                  if( pCurrBrush.get() != NULL )
                     {
                     graphics.FillEllipse(pCurrBrush.get(), x0, y0, 2*ptSize, 2*ptSize);
                     }
                  if( pCurrPen.get() != NULL )
                     {
                     graphics.DrawEllipse(pCurrPen.get(), x0, y0, 2*ptSize, 2*ptSize);
                     }
                  }
               }
            }  // end of: for ( i < polyArray.GetSize() )

            }  // end of:  case LT_POINT
            break;
         }

   // memory management for partCounts
   if ( partCounts != NULL )
      delete [] partCounts;

   m_maxPolyParts = maxPolyParts;
   
   return;
   }


void MapWindow::DrawLabels( CDC &dc )
   {
   // draw each label...
   dc.SetTextAlign( TA_CENTER | TA_BASELINE );

   for ( int layer=0; layer < m_pMap->m_drawOrderArray.GetSize(); layer++ )
      {
      MapLayer *pLayer = m_pMap->m_drawOrderArray[ layer ];
      if ( pLayer->IsVisible() && pLayer->m_showLabels )
         {
         // need to create a query?
         if ( pLayer->m_labelQueryStr.GetLength() > 0 )
            {
            if ( pLayer->m_pQueryEngine == NULL )
               pLayer->m_pQueryEngine = new QueryEngine( pLayer );
            
            ASSERT( pLayer->m_pQueryEngine != NULL );

            if ( pLayer->m_pLabelQuery == NULL  )
               pLayer->m_pLabelQuery = pLayer->m_pQueryEngine->ParseQuery( pLayer->m_labelQueryStr, 0, "Label Query" );
            }

         int fontSize = m_pMap->VDtoLD( pLayer->m_labelSize );
         if ( fontSize < 10 )
            continue;

         //if ( fontSize > 200 )
         //   fontSize = 200;

         BinArray *pBinArray = pLayer->GetBinArray( USE_ACTIVE_COL, false );
         MAP_FIELD_INFO *pInfo = NULL;

         if ( pBinArray != NULL )
            pInfo = pBinArray->m_pFieldInfo;

         //CFont *pOldFont = NULL;
         
         CFont *pLabelFont = new CFont;
         pLayer->CreateLabelFont( *pLabelFont );
         CFont *pOldFont = dc.SelectObject( pLabelFont );
         dc.SetTextColor( pLayer->m_labelColor );
         dc.SetBkMode( TRANSPARENT );
         
         switch( pLayer->GetType() )
            {
            case LT_POLYGON:
            case LT_LINE:  // also uses polygons
               {
               int polyCount = pLayer->GetPolygonCount();
               for ( int i=0; i < polyCount; i++ )
                  {
                  if ( pLayer->IsDefunct( i ) )
                     continue;

                  Poly *pPoly = pLayer->m_pPolyArray->GetAt( i );    // get the polygon

                  // do we need to show a label?
                  if ( pPoly->m_vertexArray.GetSize() > 0 && pLayer->m_showLabels )
                     DrawLabel( dc, pLayer, pPoly, i, pInfo );
                  }  // end of:  for ( i < polyArray.GetSize() )
               }
               break;   // end of case LT_POLYGON/LT_LINE

            case LT_POINT:
               {
               for ( int i=0; i < pLayer->m_pPolyArray->GetSize(); i++ )
                  {
                  Poly *pPoly = pLayer->m_pPolyArray->GetAt( i );    // get the polygon representing this point

                  // do we need to show a label?
                  if ( pLayer->m_showLabels )
                     DrawLabel( dc, pLayer, pPoly, i, pInfo );
                  }  // end of: for ( i < polyArray.GetSize() )
               }  // end of:  case LT_POINT
               break;
            }  // end of:  switch( pLayer->GetType() )

         if ( pLabelFont != NULL )
            {
            dc.SelectObject( pOldFont );
            pLabelFont->DeleteObject();
            delete pLabelFont;
            }
         }  // end of: if ( pLayer->IsVisible() && pLayer->m_showLabels )
      }  // end of:  for ( layer < m_mapLayerArray.GetSize()

   dc.SetTextColor( 0 );
   }  // end of: if ( flag & 1 )


void MapWindow::DrawScale( CDC &dc )
   {
   //int originalDC = dc.SaveDC();

   CRect rect;
   m_pMap->GetDisplayRect( rect );
   //GetClientRect( &rect );

   REAL vxMin, vxMax, vyMin, vyMax;
   m_pMap->DPtoVP( CPoint( 0,0 ), vxMin, vyMax );
   m_pMap->DPtoVP( CPoint(rect.right, rect.bottom ), vxMax, vyMin );

   // note: device context is mapped into current virtual space.
   float scaleMax = float( ( vxMax - vxMin )/4);
   
   POINT ll;  // lower left corner of scale box (logical coords)
   REAL vxMid = (vxMax+vxMin)/2;
   REAL vyBottom = (vyMin + ( vyMax-vyMin )*0.05f );
   
   m_pMap->VPtoLP( vxMid, vyBottom, ll );   // half way across screen, bottom 10 percent

   double low, high, _ticks;
   bestscale( 0.0, double( scaleMax ), &low, &high, &_ticks);
   int ticks = int(_ticks);

   if (ticks < 2)
      ticks = 2;
   
   while ( ticks > 8 )
      ticks /= 2;

   int scaleWidth = m_pMap->VDtoLD( REAL( high ) );  // logical

   CPoint lp(  scaleWidth, 0 );
   m_pMap->LPtoDP( lp );

   if ( lp.x < 100 ) // pixels
      ticks = 2;
   else if ( lp.x < 200 && ticks > 3 )
      ticks = 3;
   else if ( lp.x < 300 && ticks > 4 )
      ticks = 4;

   float tickIncr = scaleWidth/(ticks-1.0f);
   
   // have dimensions, draw scale
   CBrush  brWhite( WHITE );
   CBrush  brBlack( BLACK );
   CPen    pnBlack( PS_SOLID, 0, BLACK );
   CPen   *pOldPen   = dc.SelectObject( &pnBlack );
   CBrush *pOldBrush = dc.SelectObject( &brWhite );

   // get point near right, lower part of screen
   CPoint logLR( int(rect.right*0.95f), int(rect.bottom*0.95f) );
   m_pMap->DPtoLP( logLR );
 
   int xLeft  = logLR.x-scaleWidth;
   int xRight = xLeft + int( tickIncr );
   int yBottom = ll.y;
   int yTop = int( yBottom + (LOGICAL_EXTENTS/m_pMap->m_zoomFactor)/100 );

   int save2 = dc.SaveDC();

   const int bdr = int( 70 / m_pMap->m_zoomFactor );

   // draw semitransparent background
   Gdiplus::Graphics graphics( dc.m_hDC );
   Gdiplus::Pen pen( Gdiplus::Color( 128, 0, 0, 0 ), 1.0 );    // Alpha value set
   Gdiplus::SolidBrush brush( Gdiplus::Color( 128, 255, 255, 255) );
   // Floating point coordinates
   //graphics.DrawRectangle( &pen, Gdiplus::RectF( xLeft, yBottom, 100, 100 ));
   graphics.FillRectangle( &brush, xLeft-2*bdr, int(yBottom-bdr-40/m_pMap->m_zoomFactor), ((xRight-xLeft)*(ticks-1))+4*bdr, int( yTop-yBottom+40/m_pMap->m_zoomFactor+2*bdr) );
   graphics.ReleaseHDC( dc.m_hDC );
   
   dc.RestoreDC( save2 );

   // set up text;
   CFont font;
   font.CreatePointFont( 80, "Arial", &dc );
   CFont *pOldFont = dc.SelectObject( &font );
   dc.SetTextAlign( TA_CENTER | TA_TOP );
   dc.SetBkMode( TRANSPARENT );
   dc.TextOut(xLeft, yBottom-4, CString( "0" ) );

   for ( int i=0; i < ticks-1; i++ )
      {
      if ( i % 2 == 0 )
         dc.SelectObject( brWhite );
      else
         dc.SelectObject( brBlack );

      dc.Rectangle( xLeft, yBottom, xRight, yTop );
      CString scale;
      CString units( _T(""));
      float value = float((i+1)*(high/(ticks-1)));

      switch( m_pMap->m_mapUnits )
         {
         case MU_METERS:
            value /= 1000;
            units = _T("km");
            break;

         case MU_FEET:
            units = _T("ft");
            break;
         }

      scale.Format( "%g%s", value, (LPCTSTR) units );
      dc.TextOut( xRight, yBottom-4, scale );
      xLeft = xRight;
      xRight = xLeft + int( tickIncr );
      }

   dc.SelectObject( pOldPen );
   dc.SelectObject( pOldBrush );
   dc.SelectObject( pOldFont );
   
   font.DeleteObject();
   brWhite.DeleteObject();
   brBlack.DeleteObject();
   pnBlack.DeleteObject();

   //dc.RestoreDC( originalDC );
   }


MapText *MapWindow::AddMapText( LPCTSTR text, int x, int y, int posFlags, int fontsize )
   {
   // incoming x, y are DISPLAY UNITS, posFlags indicates what they are relative to
   //int x = px * LOGICAL_EXTENTS / 100;
   //int y = py * LOGICAL_EXTENTS / 100;

   MapText *pText = new MapText( text, x, y, fontsize );
   pText->m_posFlags = posFlags;
   int index = (int) m_mapTextArray.Add( pText );
   return pText;
   } 


void MapWindow::SetMapText( int i, LPCTSTR text, bool redraw )
   {
   MapText *pText = GetMapText( i );
   pText->m_text = text;

   if ( redraw )
      {
      CDC *pDC = GetDC();
      this->InitMapping( *pDC );
      DrawMapText( *pDC, pText, true );
      ReleaseDC( pDC );
      }
   }


void MapWindow::DrawMapText( CDC &dc, bool eraseBkgr )
   {
   for ( int i=0; i < GetMapTextCount(); i++ )
      {
      MapText *pText = GetMapText( i );
      DrawMapText( dc, pText, eraseBkgr );
      }
   }

void MapWindow::DrawMapText( CDC &dc, MapText *pText, bool eraseBkgr )
   {
   if ( eraseBkgr )
      {
      CRect rect;
      pText->GetTextRect( this, rect );
      EraseMapText( dc, rect );
      }

   if ( pText->m_text.IsEmpty() )
      return;

   CPoint pos = pText->GetPos( this );
   dc.DPtoLP( &pos );

   // set up text;
   CFont font;
   font.CreatePointFont( pText->m_fontSize, "Arial", &dc );
   CFont *pOldFont = dc.SelectObject( &font );

   int align = TA_LEFT;
   if ( pText->m_posFlags & MTP_ALIGNCENTER )
      align = TA_CENTER;
   else if ( pText->m_posFlags & MTP_ALIGNRIGHT )
      align = TA_RIGHT;

   dc.SetTextAlign( align | TA_TOP );
   dc.SetBkMode( TRANSPARENT );

   // Get text rect
   pText->m_textSize = dc.GetTextExtent( pText->m_text );
   
   // draw text
   dc.TextOut( pos.x, pos.y, pText->m_text );

   dc.SelectObject( pOldFont );
   font.DeleteObject();
   }


void MapWindow::EraseMapText( CDC &dc, CRect &rect )   // rect is logical coords, up=positive
   {
   int dark  = 0xCC;
   int light = 0xFF;

   float pct = float(rect.bottom)/LOGICAL_EXTENTS;

   int gray = dark + int( pct * ( light-dark ) ); 


   // erase old text?
   COLORREF bkClr = RGB( gray, gray, gray );
   
   if ( rect.IsRectEmpty() != FALSE )
      {
      CBrush br;
      br.CreateSolidBrush( bkClr );
      CPen *pOldPen = (CPen*) dc.SelectStockObject( NULL_PEN );
      CBrush *pOldBr = dc.SelectObject( &br );
      CRect _rect( rect );
      _rect.InflateRect( 4, 4 );
      dc.Rectangle( &_rect );
      dc.SelectObject( pOldBr );
      dc.SelectObject( pOldPen );
      br.DeleteObject();
      }
   }

// returns client coords
CPoint MapText::GetPos( MapWindow *pMapWnd )
   {
   CRect rect;
   pMapWnd->GetClientRect( &rect );

   CPoint pt;

   //  intepret x
   if ( m_posFlags & MTP_FROMRIGHT )  // relative to right?
      pt.x = rect.right - m_x;
   else
      pt.x = m_x;

   // intepret y;
   if ( m_posFlags & MTP_FROMBOTTOM )  // relative to bottom?
      pt.y = rect.bottom - m_y;
   else
      pt.y = m_y;

   return pt;
   }


bool MapText::GetTextRect( MapWindow *pMapWnd, CRect &rect )
   {
   CPoint pt = GetPos( pMapWnd );  // display coords

   pMapWnd->m_pMap->DPtoLP( pt );  // covert to logical;

   rect.SetRect( pt.x, pt.y, pt.x+m_textSize.cx, pt.y-m_textSize.cy );   // note: m_textSize is logical coords

   int align = TA_LEFT;
   if ( m_posFlags & MTP_ALIGNCENTER )
      align = TA_CENTER;
   else if ( m_posFlags & MTP_ALIGNRIGHT )
      align = TA_RIGHT;

   switch( align )
      {
      case TA_LEFT:
         break;

      case TA_CENTER:
         rect.OffsetRect( -m_textSize.cx/2, 0 );
         break;

      case TA_RIGHT:
         rect.OffsetRect( -m_textSize.cx, 0 );
         break;
      }

   return true;
   }


void MapWindow::DrawLegend( CDC &dc )
   {
   for ( int layer=0; layer <  m_pMap->m_mapLayerArray.GetSize(); layer++ )
      {
      MapLayer *pLayer = m_pMap->m_drawOrderArray[ layer ];

      if ( pLayer->IsVisible() && pLayer->m_showLegend == true )
         {
         BinArray *pBinArray = pLayer->GetBinArray( USE_ACTIVE_COL, false );

         if ( pBinArray != NULL )
            {
            if ( pLayer->GetBinColorFlag() != BCF_SINGLECOLOR && pBinArray->GetSize() > 1 )
               {
               Legend legend( pLayer );
               legend.SetDefaults();
               float vRight = legend.SetExtents( &dc );
               legend.Draw( &dc );   

               // draw BinHistogram
               BinHistogram binHisto( pLayer );
               binHisto.SetDefaults( vRight, legend.m_bottom );
               binHisto.Draw( &dc );

               //ScaleIndicator scale( pLayer );
               //scale.SetDefaults( vRight, binHisto.m_bottom );
               //scale.Draw( &dc );
               }
            }
         }
      }  // end of:  for ( layer < m_mapLayerArray.GetSize()
   }


void MapWindow::DrawLabel( CDC &dc, MapLayer *pLayer, Poly *pPoly, int index, MAP_FIELD_INFO *pInfo )
   {
   if ( pLayer->m_pLabelQuery != NULL && pLayer->m_pQueryEngine != NULL )
      {
      bool result = false;
      pLayer->m_pQueryEngine->Run( pLayer->m_pLabelQuery, index, result );

      if ( result == false )
         return;
      }

   CString label;
   pLayer->GetRecordLabel( index, label, pInfo );
   POINT pt;
   m_pMap->VPtoLP( pPoly->m_labelPoint.x, pPoly->m_labelPoint.y, pt );
   int bkMode = dc.SetBkMode( TRANSPARENT );
   dc.TextOut( pt.x, pt.y, label );
   dc.SetBkMode( bkMode );
   }


void MapWindow::DrawText(LPCTSTR str, int x, int y, CFont *pFont /*=NULL*/ )    // NOTE: x, y are screen coordinates
   {
   CDC *pDC = GetDC();

   CGdiObject *pObject = NULL;

   if ( pFont != NULL )
      pObject = pDC->SelectObject( pFont );

   pDC->TextOut( x, y, str, lstrlen( str ) );

   if( pFont != NULL )
      pDC->SelectObject( pObject );

   ReleaseDC( pDC );
   }


void MapWindow::DrawDirections( MapLayer *pDirLayer )
   {
   ASSERT( pDirLayer != NULL );

   if ( pDirLayer->GetType() != LT_GRID )
      return;

   CDC *pDC = GetDC();

   InitMapping( *pDC );

   int rows = pDirLayer->GetRowCount();
   int cols = pDirLayer->GetColCount();
   int dir;
   POINT center;

   float _width = pDirLayer->GetGridCellWidth();
   int width = m_pMap->VDtoLD( _width );
   int arrow = width*3/4;

   if ( arrow < 4 )
      return;

   REAL cx, cy;

   int xe, ye, xp, yp, x0, y0, x1, y1;

   for ( int row=0; row < rows; row++ )
      {
      for ( int col=0; col < cols; col++ )
         {
         pDirLayer->GetData( row, col, dir );

         if ( dir > 0 )
            {
            pDirLayer->GetGridCellCenter( row, col, cx, cy );
            m_pMap->VPtoLP( cx, cy, center );

            switch( dir )
               {
               case S:
                  xe = xp = center.x;
                  ye = center.y+arrow/2;
                  yp = center.y-arrow/2;
                  x0 = xp-arrow/3;
                  x1 = xp+arrow/3;
                  y0 = y1 = yp+arrow/3;
                  break;

               case N:
                  xe = xp = center.x;
                  ye = center.y-arrow/2;
                  yp = center.y+arrow/2;
                  x0 = xp-arrow/3;
                  x1 = xp+arrow/3;
                  y0 = y1 = yp-arrow/3;
                  break;

               case E:
                  ye = yp = center.y;
                  xe = center.x-arrow/2;
                  xp = center.x+arrow/2;
                  y1 = yp-arrow/3;
                  y0 = yp+arrow/3;
                  x0 = x1 = xp-arrow/3;
                  break;

               case W:
                  ye = yp = center.y;
                  xe = center.x+arrow/2;
                  xp = center.x-arrow/2;
                  y1 = yp-arrow/3;
                  y0 = yp+arrow/3;
                  x0 = x1 = xp+arrow/3;
                  break;

               case NE:
                  xe = center.x-arrow/3;
                  ye = center.y-arrow/3;
                  xp = center.x+arrow/3;
                  yp = center.y+arrow/3;
                  x0 = xp-arrow/2;
                  y0 = yp;
                  x1 = xp;
                  y1 = yp-arrow/2;
                  break;

               case SW:
                  xe = center.x+arrow/3;
                  ye = center.y+arrow/3;
                  xp = center.x-arrow/3;
                  yp = center.y-arrow/3;
                  x0 = xp+arrow/2;
                  y0 = yp;
                  x1 = xp;
                  y1 = yp+arrow/2;
                  break;
         
               case SE:
                  xe = center.x-arrow/3;
                  ye = center.y+arrow/3;
                  xp = center.x+arrow/3;
                  yp = center.y-arrow/3;
                  x0 = xp-arrow/2;
                  y0 = yp;
                  x1 = xp;
                  y1 = yp+arrow/2;
                  break;
      
               case NW:
                  xp = center.x-arrow/3;
                  yp = center.y+arrow/3;
                  xe = center.x+arrow/3;
                  ye = center.y-arrow/3;
                  x0 = xp+arrow/2;
                  y0 = yp;
                  x1 = xp;
                  y1 = yp-arrow/2;
                  break;
               }

            pDC->MoveTo( xe, ye );
            pDC->LineTo( xp, yp );
            pDC->LineTo( x0, y0 );
            pDC->MoveTo( xp, yp );
            pDC->LineTo( x1, y1 );
            }
         }
      }

   ReleaseDC( pDC );
   }


void MapWindow::DrawDownNodes( MapLayer *pLayer )
   {
   ASSERT( pLayer != NULL );

   if ( pLayer->GetType() != LT_LINE )
      return;

   CDC *pDC = GetDC();
   InitMapping( *pDC );

   CBrush brush( RGB( 0, 0, 0 ) );
   CPen   pen(  PS_SOLID, 1, RGB( 0, 0, 0) );

   CBrush *pOldBrush = pDC->SelectObject( &brush );
   CPen   *pOldPen   = pDC->SelectObject( &pen );

   // cycle through polygons
   PolyArray *pPolyArray = pLayer->m_pPolyArray;
   for ( int i=0; i < pPolyArray->GetSize(); i++ )
      {
      Poly *pPoly = pPolyArray->GetAt( i );    // get the polygon

      // get last vertices
      int vertexCount = pPoly->GetVertexCount();
      if ( vertexCount > 1 )
         {
         int xMid = ( pPoly->m_ptArray[ vertexCount-1 ].x + pPoly->m_ptArray[ vertexCount-2 ].x ) / 2;
         int yMid = ( pPoly->m_ptArray[ vertexCount-1 ].y + pPoly->m_ptArray[ vertexCount-2 ].y ) / 2;

         pDC->Rectangle( xMid-20, yMid-20, xMid+20, yMid+20 );
         }
      }

   pDC->SelectObject( pOldBrush );
   pDC->SelectObject( pOldPen );

   ReleaseDC( pDC );
   }


void MapWindow::DrawSelection( CDC &dc )
   {
   // draw any polygons (only for now) cell matching the criteria, using
   // a pen the color of the query color, and a NULL brush
   Gdiplus::Graphics graphics(dc.m_hDC);
   Gdiplus::Color color;
   color.SetFromCOLORREF(m_selectionColor);
   
   Gdiplus::Pen pen( color, 2);
   Gdiplus::HatchBrush* pHatchBrush = NULL;

   CBrush br;
   //int bkMode;
   if ( m_selectionHatchStyle != -1 ) 
      {
      pHatchBrush = new Gdiplus::HatchBrush( (Gdiplus::HatchStyle)m_selectionHatchStyle, color, Gdiplus::Color(0, 255, 255, 255) );
      }
   else
      pHatchBrush = NULL;
      
   // infrastructure to handle polygons with parts
   int maxPolyParts = 0;    // used to keep m_maxPolyParts up-to-date
   int growBy       = 10;   // if partCounts becomes to small, it will grow by this amount
   LPINT partCounts = NULL; // dynamic array of ints, hold the number of vertices in each part of the current poly, passed to CDC::PolyPolygon(.)

   if ( m_maxPolyParts > 0 )
      partCounts = new int[ m_maxPolyParts ];

   int maxPolyVertices = 200;
   Gdiplus::Point* points = new Gdiplus::Point[maxPolyVertices];
   // draw each item...
   for ( int layer=0; layer < m_pMap->m_mapLayerArray.GetSize(); layer++ )
      {
      MapLayer *pLayer = m_pMap->m_drawOrderArray[ layer ];

      if ( pLayer->IsVisible())
         {
         switch( pLayer->GetType() )
            {
            case LT_POLYGON:
            case LT_LINE:  // also uses polygons
               {
               PolyArray *pPolyArray = pLayer->m_pPolyArray;

               // cycle through pquery results
               for ( int i=0; i < pLayer->GetSelectionCount(); i++ )
                  {
                  Poly *pPoly = pPolyArray->GetAt( pLayer->GetSelection( i ) );    // get the polygon

                  // draw a polygon for each vertex
                  int parts = (int) pPoly->m_vertexPartArray.GetSize();

                  if ( parts > 0 )  // handle NULL polygons
                     {
                     int startPart = pPoly->m_vertexPartArray[ 0 ];
                     int vertexCount = 0;
                     int nextPart = 0;

                     switch ( pLayer->m_layerType )
                        {
                        case LT_POLYGON:
                           {
                           // memory management for partCounts
                           if ( maxPolyParts < parts )
                              maxPolyParts = parts;
                           if ( m_maxPolyParts < parts )
                              {
                              while ( m_maxPolyParts < parts )
                                 m_maxPolyParts += growBy;
                              if ( partCounts != NULL )
                                 delete [] partCounts;
                              partCounts = new int[ m_maxPolyParts ];
                              }

                           for ( int part=0; part < parts; part++ )
                              {
                              if ( part < parts-1 )
                                 {
                                 nextPart = pPoly->m_vertexPartArray[ part+1 ];
                                 vertexCount =  nextPart - startPart;
                                 }
                              else
                                 vertexCount = pPoly->GetVertexCount() - startPart;

                              partCounts[part] = vertexCount;
                              
                              startPart = nextPart;
                              }

                           int last = 0;
                           for( int part = 0; part < parts; part++)
                              {
                              int num = partCounts[part];
                              if( num > maxPolyVertices )
                                 {
                                 delete []points;
                                 points = new Gdiplus::Point[num];
                                 maxPolyVertices = num;
                                 }
                              for(int i = 0; i < num; i++)
                                 {
                                 points[i].X = pPoly->m_ptArray[last].x;
                                 points[i].Y = pPoly->m_ptArray[last].y;
                                 last++;
                                 } 
                              if( pHatchBrush)
                                 {
                                 graphics.FillPolygon(pHatchBrush, points, num);
                                 }
                              graphics.DrawPolygon(&pen, points, num);  
                              }

                           // documentation for CDC::PolyPolygon(.) says it should not be used for a 1 part poly.
                           }
                           break;

                        case LT_LINE:
                           {
                           for ( int part=0; part < parts; part++ )
                              {
                              if ( part < parts-1 )
                                 {
                                 nextPart = pPoly->m_vertexPartArray[ part+1 ];
                                 vertexCount =  nextPart - startPart;
                                 }
                              else
                                 vertexCount = pPoly->GetVertexCount() - startPart;

                              if( vertexCount > maxPolyVertices )
                                 {
                                 delete []points;
                                 points = new Gdiplus::Point[vertexCount];
                                 maxPolyVertices = vertexCount;
                                 }
                              for( int i = 0; i < vertexCount; i++)
                                 {
                                 points[i].X = pPoly->m_ptArray[startPart+i].x;
                                 points[i].Y = pPoly->m_ptArray[startPart+i].y;
                                 }                             
                              graphics.DrawLines(&pen, points, vertexCount);                             
                             
                              startPart = nextPart;
                              }
                           }
                           break;
                        }  // end of: switch( m_layerType )
                    }  // end of: if ( parts > 0 )
                  }  // end of:  for ( i < polyArray.GetSize() )
               }
               break;   // end of case LT_POLYGON/LT_LINE

            case LT_GRID:
               {
               // for grids, we assume selections ID's are in pairs, corresponding to row, col
               DataObj *pData = pLayer->m_pData;
               ASSERT( pData != NULL );

               int rows = pData->GetRowCount();
               int cols = pData->GetColCount();

               for ( int i=0; i < pLayer->GetSelectionCount(); i += 2 )
                  {
                  int row = pLayer->GetSelection( i );
                  int col = pLayer->GetSelection( i+1 );

                  COORD2d vll, vur;
                  pLayer->GetGridCellRect( row, col, vll, vur );
                  
                  // get logical cell sizes for drawing
                  POINT ll, ur;
                  m_pMap->VPtoLP( vll.x, vll.y, ll );  // convert the lower left corner to logical points
                  m_pMap->VPtoLP( vur.x, vur.y, ur );  // convert the lower left corner to logical points
                  Gdiplus::Rect rect;
                  rect.X = ll.x;
                  rect.Y = ll.y;
                  rect.Width = ur.x - ll.x;
                  rect.Height = ur.y - ll.y;
                  
                  graphics.DrawRectangle( &pen, rect );
                  }
               }
               break;

            case LT_POINT:
               {
               for ( int i=0; i < pLayer->GetSelectionCount(); i++ )
                  {
                  Poly *pPoint = pLayer->m_pPolyArray->GetAt( pLayer->GetSelection( i ) );    // get the polygon
                  graphics.DrawEllipse( &pen, pPoint->m_ptArray[0].x, pPoint->m_ptArray[0].y, 100, 100 );
                  //graphics.FillEllipse( &br, pPoint->m_ptArray[0].x, pPoint->m_ptArray[0].y, 100, 100 );
                  }
               }
               break;
            }  // end of:  switch( pLayer->GetType() )

         }  // end of: if ( pLayer->IsVisible() )
      }  // end of:  for ( layer < m_mapLayerArray.GetSize()

   // memory management for partCounts
   if ( partCounts != NULL )
      delete [] partCounts;
   m_maxPolyParts = maxPolyParts;

   delete [] points;

   if ( pHatchBrush != NULL )
      delete pHatchBrush;
   
   }


void MapWindow::DrawPoly( MapLayer *pLayer, Poly *pPoly, int flash )
   {
   CDC *pDC = GetDC();
  
   InitMapping( *pDC );  // set up logical coordinate system

   CPen    outlinePen;
   CBrush  brush;
   CBrush *pOldBrush = NULL;

   // initialize device context
   if ( m_pMap->m_pActiveLayer->m_outlineColor != NO_OUTLINE && m_pMap->m_pActiveLayer->m_hatchStyle < 0 )
      outlinePen.CreatePen( PS_SOLID, m_pMap->m_pActiveLayer->m_lineWidth, m_pMap->m_pActiveLayer->m_outlineColor );
   else if ( m_pMap->m_pActiveLayer->m_hatchStyle >= 0 )
      {
      brush.CreateHatchBrush( m_pMap->m_pActiveLayer->m_hatchStyle, m_pMap->m_pActiveLayer->m_hatchColor );
      pOldBrush = (CBrush*) pDC->SelectObject( (CGdiObject*) &brush );
      pDC->SetBkMode( TRANSPARENT );
      }
   else
      outlinePen.CreateStockObject( NULL_PEN );//outlinePen.CreatePen( NULL_PEN, 1, BLACK );

   CPen *pOldPen = pDC->SelectObject( &outlinePen );

   if ( flash > 0 )
      pDC->SetROP2( R2_NOT );

   else
      {   // set up color
      Bin *pBin = pLayer->GetPolyBin( pPoly->m_id ); //pPoly->m_pBin;

      if ( pBin != NULL )
         brush.CreateSolidBrush( pBin->m_color );
      else
         brush.CreateSolidBrush( m_bkgrColor );

      pOldBrush = pDC->SelectObject( &brush );
      }

   int oldPolyFillMode = pDC->GetPolyFillMode();
   pDC->SetPolyFillMode( WINDING );

   // draw the polygon
   int parts = (int) pPoly->m_vertexPartArray.GetSize();

   if ( parts > 0 )  // handle NULL polygons
      {
      int startPart = pPoly->m_vertexPartArray[ 0 ];
      int vertexCount = 0;
      int nextPart = 0;

      LPINT partCounts = new int[ parts ];

      for ( int part=0; part < parts; part++ )
         {
         if ( part < parts-1 )
            {
            nextPart = pPoly->m_vertexPartArray[ part+1 ];
            vertexCount =  nextPart - startPart;
            }
         else
            vertexCount = pPoly->GetVertexCount() - startPart;

         partCounts[part] = vertexCount;

         startPart = nextPart;
         }

      // documentation for CDC::PolyPolygon(.) says it should not be used for a 1 part poly.
      if ( parts > 1 )
         pDC->PolyPolygon( pPoly->m_ptArray, partCounts, parts );
      else
         pDC->Polygon( pPoly->m_ptArray, partCounts[0] );

      delete [] partCounts;
      }
   
   // restore device context to original settings
   pDC->SelectObject( pOldPen );

   pDC->SetPolyFillMode( oldPolyFillMode );

   if ( flash <= 0 )
      {
      pDC->SelectObject( pOldBrush );
      brush.DeleteObject();
      }
   
   ReleaseDC( pDC );
   }



void MapWindow::DrawPolyBoundingRect( Poly *pPoly, int flash )
   {
   CDC *pDC = GetDC();
  
   InitMapping( *pDC );  // set up logical coordinate system

   // initialize device context
   //CPen pen;
   //pen.CreatePen( PS_SOLID, 1, BLACK );
   //CPen  *pOldPen = pDC->SelectObject( &pen );

   if ( flash > 0 )
      pDC->SetROP2( R2_NOT );


   // draw a polygon for each vertex
   REAL xMin, yMin, xMax, yMax;
   pPoly->GetBoundingRect( xMin, yMin, xMax, yMax );

   POINT ptMin, ptMax;
   m_pMap->VPtoLP( xMin, yMin, ptMin );
   m_pMap->VPtoLP( xMax, yMax, ptMax );

   //pDC->Rectangle( ptMin.x, ptMin.y, ptMax.x, ptMax.y );
   pDC->MoveTo( ptMin.x, ptMin.y );
   pDC->LineTo( ptMax.x, ptMin.y );
   pDC->LineTo( ptMax.x, ptMax.y );
   pDC->LineTo( ptMin.x, ptMax.y );
   pDC->LineTo( ptMin.x, ptMin.y );
   
   // restore device context to original settings
   //pDC->SelectObject( pOldPen );
   //pDC->SelectObject( pBrush );

   //if ( flash <= 0 )
   //   {
   //   pDC->SelectObject( pOldBrush );
   //   brush.DeleteObject();
   //   }
   
   ReleaseDC( pDC );
   }


void MapWindow::DrawPoint( Poly *pPoly, int flash )
   {
   CDC *pDC = GetDC();
  
   InitMapping( *pDC );  // set up logical coordinate system

   CGdiObject *pOldPen   = pDC->SelectStockObject( BLACK_PEN );
   CGdiObject *pOldBrush = pDC->SelectStockObject( BLACK_BRUSH );

   int r2;

   if ( flash > 0 )
      r2 = pDC->SetROP2( R2_NOT );

   const int ptSize = 30;
   pDC->Ellipse( pPoly->m_ptArray[ 0 ].x-ptSize, pPoly->m_ptArray[ 0 ].y-ptSize, pPoly->m_ptArray[ 0 ].x+ptSize, pPoly->m_ptArray[ 0 ].y+ptSize );

   pDC->SelectObject( pOldPen );
   pDC->SelectObject( pOldBrush );

   if ( flash > 0 )
      pDC->SetROP2( r2 );
   
   ReleaseDC( pDC );
   }

void MapWindow::DrawCircle( REAL x, REAL y, REAL radius )
   {
   CDC *pDC = GetDC();
  
   InitMapping( *pDC );  // set up logical coordinate system

   CGdiObject *pOldPen = pDC->SelectStockObject( BLACK_PEN );
   CGdiObject *pOldBrush = pDC->SelectStockObject( NULL_BRUSH );

   POINT center;
   m_pMap->VPtoLP( x, y, center );
   int rad = m_pMap->VDtoLD( radius );

   pDC->Ellipse( center.x-rad, center.y-rad, center.x+rad, center.y+rad );
   
   pDC->SelectStockObject( BLACK_BRUSH );
   int dot = 30;
   pDC->Ellipse( center.x-dot, center.y-dot, center.x+dot, center.y+dot );

   pDC->SelectObject( pOldPen );
   pDC->SelectObject( pOldBrush );
   
   ReleaseDC( pDC );
   }

void MapWindow::DrawPolyVertices( Poly *pPoly )
   {
   CDC *pDC = GetDC();
  
   InitMapping( *pDC );  // set up logical coordinate system

   Gdiplus::Graphics graphics( pDC->m_hDC );

   Gdiplus::Pen blackPen( Gdiplus::Color::Black, 2);
   Gdiplus::SolidBrush blackBr(Gdiplus::Color::Black);
   Gdiplus::Pen bluePen( Gdiplus::Color::Blue, 2);
   Gdiplus::SolidBrush blueBr(Gdiplus::Color::Blue);

   Gdiplus::Pen pen(Gdiplus::Color::Yellow, 1);
   Gdiplus::SolidBrush br(Gdiplus::Color::Yellow );

   int ptSize = 20;

   for ( int i=0; i < pPoly->GetVertexCount(); i++ )
   {
    graphics.FillEllipse(&br, pPoly->m_ptArray[ i ].x-ptSize, pPoly->m_ptArray[ i ].y-ptSize, 2*ptSize, 2*ptSize );
    graphics.DrawEllipse(&pen, pPoly->m_ptArray[ i ].x-ptSize, pPoly->m_ptArray[ i ].y-ptSize, 2*ptSize, 2*ptSize );
   }


   // draw start of parts
   ptSize *= 2;

   for ( int i=0; i < pPoly->m_vertexPartArray.GetSize(); i++ )
      {
      int index = pPoly->m_vertexPartArray[ i ];
      graphics.FillEllipse(&blackBr, pPoly->m_ptArray[ index ].x-ptSize, pPoly->m_ptArray[ index ].y-ptSize, 2*ptSize, 2*ptSize );
      graphics.DrawEllipse(&blackPen, pPoly->m_ptArray[ index ].x-ptSize, pPoly->m_ptArray[ index ].y-ptSize, 2*ptSize, 2*ptSize );
      }

   // draw solid black circle at starting vertex
    graphics.FillEllipse(&blackBr, pPoly->m_ptArray[ 0 ].x-ptSize, pPoly->m_ptArray[ 0 ].y-ptSize, 2*ptSize, 2*ptSize );
    graphics.DrawEllipse(&blackPen, pPoly->m_ptArray[ 0 ].x-ptSize, pPoly->m_ptArray[ 0 ].y-ptSize, 2*ptSize, 2*ptSize );
   
   // and blue cross at ending 
    Gdiplus::Point p1, p2;
   ptSize *= 2;
   int i = pPoly->GetVertexCount()-1;
   p1.X = pPoly->m_ptArray[ 0 ].x-ptSize;
   p1.Y = pPoly->m_ptArray[ 0 ].y;
   p2.X = pPoly->m_ptArray[ 0 ].x+ptSize;
   p2.Y = pPoly->m_ptArray[ 0 ].y;
   graphics.DrawLine(&bluePen, p1, p2);

   p1.X = pPoly->m_ptArray[ 0 ].x;
   p1.Y = pPoly->m_ptArray[ 0 ].y-ptSize;
   p2.X = pPoly->m_ptArray[ 0 ].x;
   p2.Y = pPoly->m_ptArray[ 0 ].y+ptSize;
   graphics.DrawLine(&bluePen, p1, p2);
   
   ReleaseDC( pDC );
   }



void MapWindow::FlashPoly( int mapLayer, int index, int period /*= 250ms */ )
   {
   if ( m_pMap && m_pMap->m_pActiveLayer )
      {
      m_pMap->m_pActiveLayer = m_pMap->m_mapLayerArray[ mapLayer ].pLayer;   
      
      m_timerCount = 0;
      SetTimer( index, period, NULL );
      }
   }

int MapWindow::OnCreate(LPCREATESTRUCT lpCreateStruct) 
   {
   lpCreateStruct->style |= WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN;

   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   m_ttPosition.x = 200;
   m_ttPosition.y = 200;
   m_toolTip.Create( this, TTS_ALWAYSTIP|TTF_TRACK|TTF_ABSOLUTE|TTF_IDISHWND );

   m_toolTip.SetDelayTime(TTDT_AUTOPOP, 3);  // -1,0,0
   m_toolTip.SetDelayTime(TTDT_INITIAL, 3); 
   m_toolTip.SetDelayTime(TTDT_RESHOW, 3);

   TOOLINFO ti;
   ti.cbSize = TTTOOLINFOA_V2_SIZE;  //sizeof( TOOLINFO );
   ti.uFlags = TTF_ABSOLUTE | TTF_IDISHWND | TTF_TRACK;
   ti.hwnd   = this->m_hWnd;
   ti.uId    = (UINT_PTR) this->m_hWnd;
   ti.lpszText = LPSTR_TEXTCALLBACK;
   ti.lParam = 10001;  // ap defined value
   ti.lpReserved = NULL;
   GetClientRect( &ti.rect);
   m_toolTip.SendMessage(TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti); 
   //m_toolTip.Activate(TRUE);

   m_toolTip.SendMessage( TTM_SETMAXTIPWIDTH, 0, (LPARAM) SHRT_MAX );
   m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_AUTOPOP, SHRT_MAX ); // turn of autopop
   m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_INITIAL, 1200 );  // 200 200
   m_toolTip.SendMessage( TTM_SETDELAYTIME, TTDT_RESHOW, 1200 );

   m_toolTip.EnableTrackingToolTips(TRUE);
   m_toolTip.EnableToolTips(TRUE);
   
   return 0;
   }



void MapWindow::InitMapping( CDC &dc )
   {
   //-- set and flip logical coordinate system --//
   // Note:  This function assumes that the virtual coordinate system
   //  has been mapped into a logical coordinate system
   //  that is LOGICAL_EXTENTS square.  The coordinate
   //  conversion functions below do map conversion
   RECT  rect;
   //GetClientRect ( &rect );
   rect = m_pMap->m_displayRect;

   dc.SetMapMode( MM_ISOTROPIC );

   int yExt = (int) (LOGICAL_EXTENTS / m_pMap->m_zoomFactor);
   if ( m_invertCoords )
      yExt = -yExt;

   dc.SetWindowExt  ( (int) (LOGICAL_EXTENTS / m_pMap->m_zoomFactor ), yExt );  // logical units
   dc.SetViewportExt( rect.right,  rect.bottom );  // physical (device) units
   dc.SetViewportOrg( rect.right/2, rect.bottom/2 );
   dc.SetWindowOrg  ( LOGICAL_EXTENTS/2, LOGICAL_EXTENTS/2 );

   ::GetWindowOrgEx  ( dc.m_hDC, &(m_pMap->m_windowOrigin   ) );
   ::GetWindowExtEx  ( dc.m_hDC, &(m_pMap->m_windowExtent   ) );
   ::GetViewportOrgEx( dc.m_hDC, &(m_pMap->m_viewportOrigin ) );
   ::GetViewportExtEx( dc.m_hDC, &(m_pMap->m_viewportExtent ) );

   //m_windowOrigin   = dc.GetWindowOrg();
   //m_windowExtent   = dc.GetWindowExt();
   //m_viewportOrigin = dc.GetViewportOrg();
   //m_viewportExtent = dc.GetViewportExt();
   }
      


//int MapWindow::Notify( NOTIFY_TYPE t, int a0, LONG_PTR a1 )
//   {
//   int procVal = 0;
//   int retVal  = 1;
//   for ( int i=0; i < m_notifyProcArray.GetSize(); i++ )
//      {
//      MAPPROCINFO *pInfo = m_notifyProcArray[ i ];
//      if ( pInfo->proc != NULL ) 
//         {
//         procVal =  pInfo->proc( this, t, a0, a1, pInfo->extra ); 
//         if ( procVal == 0 )
//            retVal = 0;
//         }
//      }
//
//   return retVal;
//   }
//
//
//int MapWindow::NotifyText( NOTIFY_TYPE t, LPCTSTR text )
//   {
//   int procVal = 0;
//   int retVal  = 1;
//
//   for ( int i=0; i < m_notifyTextProcArray.GetSize(); i++ )
//      {
//      MAPTEXTPROCINFO *pInfo = m_notifyTextProcArray[ i ];
//      if ( pInfo->proc != NULL ) 
//         {
//         procVal = pInfo->proc( this, t, text ); 
//         if ( procVal == 0 )
//            retVal = 0;
//         }
//      }
//
//   return retVal;
//   }
//
//bool MapWindow::RemoveNotifyHandler( MAPPROC p, LONG_PTR extra )
//   {
//   for ( int i=0; i < m_notifyProcArray.GetSize(); i++ )
//      {
//      MAPPROCINFO *pInfo = m_notifyProcArray[ i ];
//      if ( pInfo->proc == p && pInfo->extra == extra ) 
//         {
//         m_notifyProcArray.RemoveAt( i );
//         return true;
//         }
//      }
//
//   return false;
//   }
//
//bool MapWindow::RemoveNotifyTextHandler( MAPTEXTPROC p /*, LONG_PTR extra*/ )
//   {
//   for ( int i=0; i < m_notifyTextProcArray.GetSize(); i++ )
//      {
//      MAPTEXTPROCINFO *pInfo = m_notifyTextProcArray[ i ];
//      if ( pInfo->proc == p /*&& pInfo->extra == extra*/ ) 
//         {
//         m_notifyTextProcArray.RemoveAt( i );
//         return true;
//         }
//      }
//
//   return false;
//   }


void MapWindow::ZoomFull( bool redraw )
   {
   m_pMap->SetViewExtents( m_pMap->m_vxMapMinExt, m_pMap->m_vxMapMaxExt, m_pMap->m_vyMapMinExt, m_pMap->m_vyMapMaxExt );   // also runs InitPolyLogicalPts();

   m_pMap->m_zoomFactor = 0.9f;
 
   m_pMap->Notify( NT_EXTENTSCHANGED, 0, 0 );

   if ( redraw && this->GetSafeHwnd() > 0 )
      RedrawWindow();
   }


void MapWindow::ZoomRect( COORD2d ll, COORD2d ur, bool redraw /*=true*/ )
   {
   REAL xMin = min( ll.x, ur.x );
   REAL xMax = max( ll.x, ur.x );
   REAL yMin = min( ll.y, ur.y );
   REAL yMax = max( ll.y, ur.y );
   
   m_pMap->SetViewExtents( xMin, xMax, yMin, yMax );  // also runs InitPolyLogicalPoints()

   m_pMap->Notify( NT_EXTENTSCHANGED, 0, 0 );

   if ( redraw )
      RedrawWindow();
   }


void MapWindow::ZoomToPoly( MapLayer *pLayer, int polyIndex, bool redraw /*=true*/ )
   {
   if ( polyIndex < 0 || polyIndex >= pLayer->GetPolygonCount() )
      return;

   Poly *pPoly = pLayer->GetPolygon( polyIndex );
   REAL xMin, xMax, yMin, yMax;
   pPoly->GetBoundingRect( xMin, yMin, xMax, yMax );

   ZoomRect( COORD2d( xMin, yMin ), COORD2d( xMax, yMax ), redraw );
   }


void MapWindow::ZoomToSelection( MapLayer *pLayer, bool redraw /* true */ )
   {
   // get bounding rect of query
   if ( pLayer == NULL )
      return;

   int count = pLayer->GetSelectionCount();

   if ( count == 0 )
      return;

   // get containing rect
   COORD2d ll, ur;
   ll.x = ll.y =  999999999.0f;
   ur.x = ur.y = -999999999.0f;

   for ( int i=0; i < count; i++ )
      {
      int idu = pLayer->GetSelection( i );

      Poly *pPoly = pLayer->GetPolygon( idu );

      REAL xMin=0, yMin=0, xMax=0, yMax=0;
      pPoly->GetBoundingRect( xMin, yMin, xMax, yMax );

      if ( xMin < ll.x ) ll.x = xMin;
      if ( yMin < ll.y ) ll.y = yMin;
      if ( xMax > ur.x ) ur.x = xMax;
      if ( yMax > ur.y ) ur.y = yMax;
      }

   this->ZoomRect( ll, ur, redraw );
   }


void MapWindow::SetZoomFactor( float zoomFactor, bool redraw )
   {
   m_pMap->m_zoomFactor = zoomFactor;

   if ( redraw )
      RedrawWindow();

   m_pMap->Notify( NT_EXTENTSCHANGED, 0, 0 );
   }


void MapWindow::OnLButtonDblClk(UINT nFlags, CPoint point) 
   {
   m_pMap->Notify( NT_LBUTTONDBLCLK, point.x, point.y );
   }


void MapWindow::OnRButtonDblClk(UINT nFlags, CPoint point) 
   {
   m_pMap->Notify( NT_RBUTTONDBLCLK, point.x, point.y );
   }
 

void MapWindow::OnRButtonDown(UINT nFlags, CPoint point) 
   {
   m_pMap->Notify( NT_RBUTTONDOWN, point.x, point.y );
   }


void MapWindow::OnTimer(UINT_PTR nIDEvent) 
   {   
   m_timerCount++;

   if ( m_timerCount > 4 )
      KillTimer( nIDEvent);

   else
      {
      LAYER_TYPE type = m_pMap->m_pActiveLayer->GetType();
      Poly *pPoly = m_pMap->m_pActiveLayer->m_pPolyArray->GetAt( nIDEvent );
            
      switch( type )
         {
         case LT_POLYGON:
         case LT_LINE:
            DrawPoly( m_pMap->m_pActiveLayer, pPoly, m_timerCount % 2 );
            break;

         case LT_POINT:
            DrawPoint( pPoly, m_timerCount % 2 );
            break;
         }

      }

   CWnd::OnTimer(nIDEvent);
   }


void MapWindow::OnLButtonDown(UINT nFlags, CPoint point) 
   {
   m_pMap->Notify( NT_LBUTTONDOWN, point.x, point.y );

   switch( m_mode )
      {
      case MM_ZOOMIN:
      case MM_SELECT:
         m_startPt = m_lastPt = point;
         m_rubberBanding = true;
         SetCapture();
         break;

      case MM_QUERYDISTANCE:
      case MM_PAN:
         m_startPt = m_lastPt = point;
         m_rubberBanding = true;
         SetCapture();
/*
         if ( m_pTipWnd == NULL )
            {
            m_pTipWnd = new TipWnd;
            BOOL ok = m_pTipWnd->Create( m_startPt.x, m_startPt.y, this, 1000 );
            ASSERT( ok );
            }
         else
            m_pTipWnd->ShowWindow( SW_SHOW );

         m_pTipWnd->UpdateText( "0.0" ); */
         m_pMap->NotifyText( NT_QUERYDISTANCE, _T("Drag Mouse to query distance" ) );
         break;

      case MM_ZOOMOUT:
         {
         REAL vx, vy;
         m_pMap->DPtoVP( point, vx, vy );      // get center;

         REAL vxMin = vx - m_pMap->m_vExtent;
         REAL vxMax = vx + m_pMap->m_vExtent;
         REAL vyMin = vy - m_pMap->m_vExtent;
         REAL vyMax = vy + m_pMap->m_vExtent;

         m_pMap->SetViewExtents( vxMin, vxMax, vyMin, vyMax );
         if ( m_modeSticky == false )
            m_mode = MM_NORMAL;

         InvalidateRect( NULL, TRUE );
         RedrawWindow();
         }
         break;
      }
   
   CWnd::OnLButtonDown(nFlags, point);
   }

void MapWindow::OnLButtonUp(UINT nFlags, CPoint point) 
   {
   if ( m_rubberBanding )
      {
      CClientDC dc( this );
      dc.SetROP2( R2_NOT );
      CGdiObject *pObj = dc.SelectStockObject( NULL_BRUSH );

      if ( m_mode == MM_QUERYDISTANCE )
         {
         dc.MoveTo( m_startPt.x, m_startPt.y );
         dc.LineTo( m_lastPt.x, m_lastPt.y );
         }
      else
         dc.Rectangle( m_startPt.x, m_startPt.y, m_lastPt.x, m_lastPt.y );

      dc.SelectObject( pObj );
      m_lastPt = point;
      m_rubberBanding = false;
      ReleaseCapture();

      switch ( m_mode )
         {
         case MM_ZOOMIN:
            ZoomAt( m_startPt, m_lastPt );
            break;

         case MM_SELECT:
            if ( m_pMap->m_pActiveLayer != NULL )
               {
               POINT ll = m_startPt;
               POINT ur = point;
               m_pMap->DPtoLP( ll );
               m_pMap->DPtoLP( ur );
               int selectedCount = 0;
               bool clear = nFlags & MK_SHIFT ? false : true;

               // point selection?
               if ( abs( m_startPt.x - point.x ) < 3 && abs( m_startPt.y - point.y ) < 3 )
                  selectedCount = m_pMap->m_pActiveLayer->SelectPoint( ll, clear );

               else  // area selection
                  selectedCount = m_pMap->m_pActiveLayer->SelectArea( ll, ur, clear, false );

               m_pMap->Notify( NT_SELECT, selectedCount, m_pMap->m_pActiveLayer->GetRecordCount() );

               RedrawWindow();
               }
            else
               MessageBeep( 0 );
            break;

         case MM_PAN:
            {
            if ( m_pTipWnd != NULL )
               m_pTipWnd->ShowWindow( SW_HIDE );

            POINT ll = m_startPt;
            POINT ur = point;
            m_pMap->DPtoLP( ll );
            m_pMap->DPtoLP( ur );

            REAL dx = m_pMap->LDtoVD( ll.x - ur.x );
            REAL dy = m_pMap->LDtoVD( ll.y - ur.y );

            m_pMap->m_vxMin += dx;    // virtual coords for display
            m_pMap->m_vxMax += dx;
            m_pMap->m_vyMin += dy;
            m_pMap->m_vyMax += dy;

            m_pMap->SetViewExtents( m_pMap->m_vxMin, m_pMap->m_vxMax, m_pMap->m_vyMin, m_pMap->m_vyMax );
            m_pMap->Notify( NT_EXTENTSCHANGED, 0, 0 );
            RedrawWindow();
            }
            break;

         case MM_QUERYDISTANCE:
            //if ( m_pTipWnd != NULL )
            //   m_pTipWnd->ShowWindow( SW_HIDE );

            break;
         }
      
      if ( m_modeSticky == false )
         m_mode = MM_NORMAL;
      }

   m_pMap->Notify( NT_LBUTTONUP, point.x, point.y );    // send message on to app

   CWnd::OnLButtonUp(nFlags, point);
   }


BOOL MapWindow::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
   {
   //this->CenterAt(pt, false);

   if ( zDelta < 0 )
      this->ZoomOut();
   else
      this->ZoomIn();

   m_pMap->Notify( NT_MOUSEWHEEL, zDelta, 0 );

   return CWnd::OnMouseWheel(nFlags, zDelta, pt);
   }


void MapWindow::OnMouseMove(UINT nFlags, CPoint point) 
   {
   m_pMap->Notify( NT_MOUSEMOVE, point.x, point.y );

   if ( m_rubberBanding )
      {
      CClientDC dc( this );
      dc.SetROP2( R2_NOT );
      CGdiObject *pObj = dc.SelectStockObject( NULL_BRUSH );
    
      if ( m_startPt != m_lastPt )
         {
         if ( m_mode == MM_QUERYDISTANCE || m_mode == MM_PAN)
            {
            dc.MoveTo( m_startPt.x, m_startPt.y );
            dc.LineTo( m_lastPt.x, m_lastPt.y );
            dc.MoveTo( m_startPt.x, m_startPt.y );
            dc.LineTo( point.x, point.y );

            REAL vxStart, vxEnd, vyStart, vyEnd;
            m_pMap->DPtoVP( m_startPt, vxStart, vyStart );
            m_pMap->DPtoVP( point, vxEnd, vyEnd );
            float dx = float( vxEnd-vxStart );
            float dy = float( vyEnd-vyStart );
            CString msg;
            msg.Format( "dx:%g, dy:%g, distance:%g", dx, dy, float( sqrt( dx*dx + dy*dy ) ) );
            m_pMap->NotifyText( NT_QUERYDISTANCE, msg );
            }
         else
            {
            dc.Rectangle( m_startPt.x, m_startPt.y, m_lastPt.x, m_lastPt.y );
            dc.Rectangle( m_startPt.x, m_startPt.y, point.x, point.y );
            }
         }

      dc.SelectObject( pObj );
    
      m_lastPt = point;
      }  // end of: if ( m_rubberbanding)
/*
   if ( m_mode == MM_QUERYDISTANCE || m_mode == MM_PAN )
      {
      if ( m_pTipWnd != NULL )
         {
         float x0, y0, x1, y1;
         DPtoVP( m_startPt, x0, y0 );
         DPtoVP( point, x1, y1 );
            
         float dist = DistancePtToPt( x0, y0, x1, y1 );

         CString tip;
         tip.Format( "%g", dist );
         m_pTipWnd->UpdateText( tip );
         m_pTipWnd->Move( point.x, point.y );
         InvalidateRect( tipRect );
         }
      } */


   if ( this->m_showAttributeOnHover )
      {
      m_ttPosition = point;
   
      CString tipText( "test" );
      if ( m_pMap->GetAttrAt( point, tipText ) >= 0 )
         {
         m_toolTip.ShowWindow( SW_SHOW );
         lstrcpyn( m_toolTipContent, tipText, 511 );//,, tipText, 511 );
         bool activate = true;
         if ( activate && ! m_ttActivated  )
            {
            CToolInfo ti;
            m_toolTip.GetToolInfo( ti, this, (UINT_PTR) m_hWnd );
            m_toolTip.SendMessage(TTM_TRACKACTIVATE,(WPARAM)TRUE, (LPARAM) &ti );
   //         m_toolTip.Activate( TRUE );
            m_ttActivated = true;
            }
         else if ( ! activate && m_ttActivated )
            {
            CToolInfo ti;
            m_toolTip.GetToolInfo( ti, this, (UINT_PTR) m_hWnd );
            m_toolTip.SendMessage(TTM_TRACKACTIVATE,(WPARAM)FALSE, (LPARAM) &ti );
   //         m_toolTip.Activate( FALSE );
            m_ttActivated = false;
            }
   
         ClientToScreen( &point );
         point.Offset( 10, -10 );
         m_toolTip.SendMessage(TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(point.x, point.y) ); // Set pos
         }
      else
         {
         lstrcpy( m_toolTipContent, "" );
         m_toolTip.ShowWindow( SW_HIDE );
         }
      }
   }


void MapWindow::CenterAt( POINT p, bool redraw /*= true */ )
   {
   // get current extents
   REAL vxMin, vxMax, vyMin, vyMax;
   m_pMap->GetViewExtents( vxMin, vxMax, vyMin, vyMax );

   // recenter on clicked point
   REAL vxCenter, vyCenter;
   m_pMap->DPtoVP( p, vxCenter, vyCenter );

   REAL width  = vxMax - vxMin;
   REAL height = vyMax - vyMin;

   vxMin = vxCenter - width/2;
   vxMax = vxMin + width;
   vyMin = vyCenter - height/2;
   vyMax = vyMin + height;
   
   m_pMap->SetViewExtents( vxMin, vxMax, vyMin, vyMax );

   if ( redraw )
      RedrawWindow();

   m_pMap->Notify( NT_EXTENTSCHANGED, 0, 0 );
   }


void MapWindow::ZoomAt( POINT p, float zoomFactor, bool redraw /*=true*/)
   {
   // convert the device coords to virtuals
   CenterAt( p, false );
   SetZoomFactor( m_pMap->m_zoomFactor * zoomFactor, false );
   
   if ( redraw )
      RedrawWindow();

   m_pMap->Notify( NT_EXTENTSCHANGED, 0, 0 );
   }


void MapWindow::ZoomAt( POINT ul, POINT lr, bool redraw/*=true*/ )
   {
   // convert the device coords to virtuals
   REAL vxMin, vxMax, vyMin, vyMax;
   m_pMap->DPtoVP( ul, vxMin, vyMin );
   m_pMap->DPtoVP( lr, vxMax, vyMax );

   REAL xMin = min( vxMin, vxMax );
   REAL xMax = max( vxMin, vxMax );
   REAL yMin = min( vyMin, vyMax );
   REAL yMax = max( vyMin, vyMax );

   m_pMap->SetViewExtents( xMin, xMax, yMin, yMax );

   // redraw and let the app know its been resized...
   if ( redraw )
      RedrawWindow();

   m_pMap->Notify( NT_EXTENTSCHANGED, 0, 0 );
   }


BOOL MapWindow::OnEraseBkgnd(CDC* pDC) 
   {
   RECT rect;
   GetClientRect( &rect );
   rect.bottom++;
   rect.right++;
   //pDC->FillSolidRect( &rect, m_bkgrColor );

   TRIVERTEX        vert[2] ;
   GRADIENT_RECT    gRect;
   vert [0] .x      = 0;
   vert [0] .y      = 0;
   vert [0] .Red    = 0xcc00;
   vert [0] .Green  = 0xcc00;
   vert [0] .Blue   = 0xcc00;
   vert [0] .Alpha  = 0x0000;

   vert [1] .x      = rect.right;
   vert [1] .y      = rect.bottom; 
   vert [1] .Red    = 0xff00;
   vert [1] .Green  = 0xff00;
   vert [1] .Blue   = 0xff00;
   vert [1] .Alpha  = 0x0000;

   gRect.UpperLeft  = 0;
   gRect.LowerRight = 1;
   pDC->GradientFill( vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V );

   return FALSE;
   }


//void MapWindow::HideAll()
//   {
//   for ( int i=0; i < m_pMap->m_mapLayerArray.GetSize(); i++ )
//      m_pMap->m_mapLayerArray[ i ].pLayer->Hide();
//   }


void MapWindow::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
   CPaintDC pDC(this);  
   RECT rect;
   RECT oldWindowRect = m_pMap->m_windowRect;

   GetWindowRect(&m_pMap->m_windowRect);
   GetClientRect(&m_pMap->m_displayRect);  

   //setup gradient vertex colors
   TRIVERTEX        vert[2] ;
   GRADIENT_RECT    gRect;
   vert [0] .Red    = 0xcc00;
   vert [0] .Green  = 0xcc00;
   vert [0] .Blue   = 0xcc00;
   vert [0] .Alpha  = 0x0000;

   vert [1] .Red    = 0xff00;
   vert [1] .Green  = 0xff00;
   vert [1] .Blue   = 0xff00;
   vert [1] .Alpha  = 0x0000;
   gRect.UpperLeft  = 0;
   gRect.LowerRight = 1;

   //resizing will create rects that need to be filled in
   //check top rect
   if(oldWindowRect.top > m_pMap->m_windowRect.top)
      {
      rect = m_pMap->m_displayRect;
      rect.bottom = rect.top + (oldWindowRect.top - m_pMap->m_windowRect.top);

      pDC.FillSolidRect( &rect, RGB(0xff,0xff,0xff) );
      }

   //check bottom rect
   if(oldWindowRect.bottom < m_pMap->m_windowRect.bottom)
      {
      rect = m_pMap->m_displayRect;
      rect.top = rect.bottom - (m_pMap->m_windowRect.bottom - oldWindowRect.bottom);

      pDC.FillSolidRect( &rect, RGB(0xcc,0xcc,0xcc) );
      
      }

   //check left rect
   if(oldWindowRect.left > m_pMap->m_windowRect.left)
      {
      rect = m_pMap->m_displayRect;
      rect.right = rect.left + (oldWindowRect.left - m_pMap->m_windowRect.left);

      vert [0] .x       = rect.left;
      vert [0] .y       = rect.bottom;
      vert [1] .x       = rect.right;
      vert [1] .y       = rect.top;
      pDC.GradientFill( vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V );
      
      }
   //check right rect
   if(oldWindowRect.right < m_pMap->m_windowRect.right)
      {
       rect = m_pMap->m_displayRect;
       rect.left = rect.right - (m_pMap->m_windowRect.right - oldWindowRect.right);

      vert [0] .x       = rect.left;
      vert [0] .y       = rect.bottom;
      vert [1] .x       = rect.right;
      vert [1] .y       = rect.top;
      pDC.GradientFill( vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V );      
      } 

   //invalidate so paint will be called
   Invalidate();
   }

