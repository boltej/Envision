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
#pragma once
#include "EnvWinLibs.h"

#include "DIBSection.h"
#include <MapLayer.h>
#include "MapWnd.h"

struct MAP_FIELD_INFO;

//------------------------------------------------------------------------------
// Raster class - used for generating rasters from MapLayers.
//
//  Features:
//
//  1) Rasters can be created at arbitrary resolution, depend on available memory
//  2) Rasters can optionally be indexed, meaning each raster pixel "knows" which
//     source polygon it is associated with
//  3) Rasters can be queried for value information associated with the MapLayer/column
//     they where rasterized from (no index required)
//  4) The raster can be queried for data associated with the source polygon (indexed
//     rasters only)
//  5) Rasters can be created at either 8-bit of 32-bit resolution, depending on the
//     number of source column attributes
//
//  Usage:
//
//  1) Create a raster using Raster::Raster( MapLayer *pLayer, bool copyPolys=false );
//        - pLayer is the MapLayer to be rasterized
//        - copyPolys can generally be defaulted to false.  True makes a local copy of the 
//               source polygons, only needed if you want to leave the source map
//               undisturbed in terms of its zoom level and active layer
//
//  2) rasterize the MapLayer using Raster::Rasterize( int col, float cellSize, RASTER_TYPE type, int createIndex )
//        - col is the source layer column to rasterize
//        - cellSize is the size (in virtual coords) of a pixel in the raster
//        - type is the bit-depth of the raster - RT_32BIT or RT_8BIT
//        - createIndex - creates an index that allows source polygons to be looked up
//               for the created raster. 
//             CI_RASTER - creates just an index raster.  use if calling any of the Raster::GetData( row,col, ? ) methods.
//             CI_POLYINDEX - creates a poly index.  use if calling any of the Raster::GetData( polyIndex, ? ), 
//                      GetPolyIndex(), or GetPolyRowColArray() methods
//  3) call one of the polymorphic Raster::GetData() methods to recover data from the raster, or 
//        associated with the source polygons.
//
//  Notes:
// 
//  1) RT_8BIT untested at this point.  The constructor rasterRect arg is not current supported
//
//  2) If the layer/col to be rasterized has a field info, that is used to determine
//     color information. Otherwise, it is determined via MapLayer::SetBins() based on 
//     the data type of the rasterized column
//
//  3) The rasterization process results in a DIB that is initially populated with
//     the colors that represent the attribute being rasterized.  Additionally, internal
//     mappings are created that allow the source data to be recovered from the map.
//     Raster::ReplaceColorsWithValues() puts the actual values into the DIB.
//
//  4) Using float types with Raster::ReplaceColorsWithValues() is only supported on
//     RT_32BIT rasters
//------------------------------------------------------------------------------------
class Raster;


enum RASTER_TYPE { RT_NONE, RT_8BIT, RT_32BIT };
enum POLY_METHOD { PM_SUM, PM_AVG, PM_MAJORITY };
enum RASTER_NOTIFY_TYPE  { NT_RASTERIZING, NT_INDEXING, NT_LOADINGINDEX };
enum { CI_RASTER=1, CI_POLYINDEX=2 };

typedef int (*RASTERPROC)( Raster*, RASTER_NOTIFY_TYPE, int, LONG_PTR, LONG_PTR );  // return 0 to interrupt, anything else to continue


class WINLIBSAPI Raster
   {
   public:
      Raster( MapLayer*, MapLayer *pIndexLayer=NULL, COORD_RECT *pRasterRect=NULL, bool copyPolys=false );  // copyPolys=true if you don't want to disturb the original map.
                                                  // setting to false means you will have to reset the zoom of the original
                                                  // map after rasterizing, but uses less memory
      ~Raster( void );
   
      bool Rasterize( int col, float cellSize, RASTER_TYPE type, int createIndex, LPCTSTR indexPath, DRAW_VALUE drawValue=DV_COLOR );

      bool IsIndexed( void) { return m_pIndexRaster != NULL; }
      Raster *GetIndexRaster( void ) { return m_pIndexRaster; }

      int  GetPolyIndex( UINT row, UINT col );
      int  GetPolyRowColArray( INT_PTR polyIndex, RowColArray **pRowColArray );

      bool CopyToSourceLayer( int col, POLY_METHOD );
      bool ReplaceColorsWithValues( void );
      bool SaveDIB( LPCTSTR filename=NULL );
      bool SaveIndex( LPCTSTR filename=NULL );
      
      static bool GetIndexHeader( LPCTSTR filename, float &cellSize, CString &layerName, int &polyCount, int &row, int &cols );
      static bool GetRequiredRowsCols( MapLayer*, float cellSize, COORD_RECT *m_pRasterRect, UINT &rows, UINT &cols );

      bool GetData( UINT row, UINT col, int &value );    // get data from the raster.  These return values, not colors
      bool GetData( UINT row, UINT col, float &value );

      bool GetData( int polyIndex, int &value, POLY_METHOD );  // get raster data based on a polygon index.  These return values, not color
      bool GetData( int polyIndex, float &value, POLY_METHOD );

      bool GetPixelColor( UINT row, UINT col, COLORREF &color );

      UINT  m_rows;
      UINT  m_cols;
      float m_cellSize;

      CDIBSection &GetDIB() { return m_dibSection; }

      // user interface interaction
      RASTERPROC InstallNotifyHandler( RASTERPROC p, LONG_PTR extra ) { RASTERPROC temp = m_notifyProc; m_notifyProc = p; m_extra = extra; return temp; }
      LONG_PTR GetNotifyExtra( void ) { return m_extra; }

   protected:
      RASTER_TYPE m_rtype;
      bool  m_bmpContainsValues;  // does the bitmap (32bit) or colortable (8bit) contain values (contains colors by default)
      float m_noDataValue;

      MapLayer  *m_pLayer;        // the layer associated with this Raster
      MapLayer  *m_pSourceLayer;  // source layer for rasterization.  Note:  if this is the same as m_pLayer, then m_pLayer is "owned" by the source map
      MapLayer  *m_pIndexLayer;   // layer used to generate index.  NULL means use source layer
      MapWindow *m_pMapWnd;       // internal map used for copy
      TYPE       m_valueType;     // type of data stored in values - based on source layer/col type

      RASTERPROC m_notifyProc;
      LONG_PTR m_extra;          // extra data passed to the map proc
      int Notify( RASTER_NOTIFY_TYPE t, int a0, LONG_PTR a1 ) { if ( m_notifyProc != NULL ) return m_notifyProc( this, t, a0, a1, m_extra ); else return -1; }

      COORD_RECT *m_pRasterRect;
      COORD_RECT *m_pStartRect;

      CDIBSection m_dibSection;  // dibSection representation of the raster

      LPBYTE m_pBits;            // actual bitmap data
      int m_bytesPerLine;  
      
      int m_col;                       // required for layer/field being rasterized
      MAP_FIELD_INFO *m_pFieldInfo;    // required for layer/field being rasterized

      // mappings for recovering values
      CArray< int, int > m_indexToIValueMap;                        // 8-bit only - lookup table for index values
      CArray< float, float > m_indexToFValueMap;                    // 8-bit only - lookup table for index values
      CMap< COLORREF, COLORREF, int, int > m_colorToIValueMap;      // 32bit only
      CMap< COLORREF, COLORREF, float, float > m_colorToFValueMap;  // 32bit only

      // indexing
      Raster *m_pIndexRaster;
      RowColArray *m_index;      // length=#poly's, each element is an array of RowCol raster pairs corresponding to the given source polygon 
      Raster *CreateIndexRaster( void );  // creates an index raster that contains the index of the source layers's polygons in the raster
      int CreateIndex( void );  // populates m_index from index raster
      bool LoadIndex( LPCTSTR filename=NULL );

      // helper methods
      bool _Rasterize( int col, float cellSize, RASTER_TYPE type, DRAW_VALUE drawValue );
      int  CreateIndexToValueMap( void );
      int  CreateColorToValueMap( void );

      static UINT CreateIndexThread( LPVOID pParam );

   };

