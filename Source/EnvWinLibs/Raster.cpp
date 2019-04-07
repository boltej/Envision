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
#include "EnvWinLibs.h"
#include "Raster.h"
#include "Maplayer.h"
#include "MAP.h"
#include "DirPlaceholder.h"

#include <limits.h>
#include <io.h>
#include <share.h>



Raster::Raster( MapLayer *pLayer, MapLayer *pIndexLayer, COORD_RECT *pRasterRect, bool copyPolys )
: m_pLayer( NULL )
, m_pSourceLayer( pLayer )
, m_pIndexLayer( pIndexLayer )
, m_pMapWnd( NULL )
, m_pRasterRect( NULL )
, m_pStartRect( NULL )
, m_pBits( NULL )
, m_rows( -1 )
, m_cols( -1 )
, m_cellSize( -1 )
, m_rtype( RT_NONE )
, m_bmpContainsValues( false )
, m_noDataValue( -9999.0f )
, m_pIndexRaster( NULL )
, m_index( NULL )
, m_col( -1 )
, m_pFieldInfo( NULL )
, m_notifyProc( NULL )
, m_extra( 0 )
   {
   Map *pMap = new Map;
   m_pMapWnd = new MapWindow;
   m_pMapWnd->SetMap( pMap );

   if ( pRasterRect != NULL )
      m_pRasterRect = new COORD_RECT( *pRasterRect );

   if ( copyPolys )
      m_pLayer = new MapLayer( pLayer, OT_SHARED_DATA );  // don't copy data, but do copy polys because we need to manage these seperately.
   else
      {
      m_pLayer = pLayer;      // Note:  if polys aren't copied, then the source layer is the same as m_pLayer
      pMap->AddLayer( pLayer, false );  // false because the source map is managing this layer
      }

   //m_pMap->ZoomFull( false );
   }


Raster::~Raster(void)
   {
   if ( m_pMapWnd )
      delete m_pMapWnd;

   if ( m_pIndexRaster != NULL )
      delete m_pIndexRaster;

   if ( m_index != NULL )
      delete [] m_index;

   if ( (HBITMAP) m_dibSection )
      m_dibSection.DeleteObject();

   if ( m_pRasterRect != NULL )
      delete m_pRasterRect;

   if ( m_pStartRect != NULL )
      delete m_pStartRect;

   m_indexToIValueMap.RemoveAll();
   m_indexToFValueMap.RemoveAll();
   m_colorToIValueMap.RemoveAll();
   m_colorToFValueMap.RemoveAll();
   }


// Notes:
// createIndexFlags should be eitehr CI_RASTER to create just a raster index, 
// or CI_POLYINDEX to create/use a poly index.  if CI_POLYINDEX is used,
// an index will be created eitehr from an index raster (indexPath == NULL)
// or loaded from an existing file (indexPath != NULL).  in the latter case
// if the index file does not exist, it will be created and saved to disk.

bool Raster::Rasterize( int col, float cellSize, RASTER_TYPE rtype, int createIndexFlags, LPCTSTR indexPath, DRAW_VALUE drawValue )
   {
   if ( col < 0 )
      return false;

   TYPE type = m_pLayer->GetFieldType( col );
   ASSERT( drawValue != DV_VALUE || ::IsInteger( type ) );   // DV_VALUE implies integer data type

   bool ok = _Rasterize( col, cellSize, rtype, drawValue );

   if ( ok )
      {
      //CString msg; 
      //msg.Format( _T("Created Raster:  rows=%i, cols=%i"), m_rows, m_cols );
      //Report::InfoMsg( msg );
      }
   else
      Report::ErrorMsg( _T("Raster: failed to create raster" ) );

   if ( createIndexFlags & CI_RASTER )
      CreateIndexRaster();

   if ( createIndexFlags & CI_POLYINDEX )
      {
      CWaitCursor c;

      if ( indexPath == NULL || indexPath[ 0 ] == NULL )
         ok = CreateIndex() > 0 ? true : false;
      else
         {
         ok = LoadIndex( indexPath );

         if ( ! ok )
            {
            CString msg;
            msg.Format( "Raster: The specified index file (%s) does not exist.  Would you like to create it?", indexPath );
            int retVal = AfxMessageBox( msg, MB_YESNO );

            if ( retVal == IDYES )
               {
               ok = CreateIndex() > 0 ? true : false;

               if ( ok )
                  SaveIndex( indexPath );

               if ( ok )
                  AfxMessageBox( "Raster poly index created and saved...", MB_OK );
               else
                  AfxMessageBox( "An error was encountered - Raster poly index not created/saved...", MB_OK );
               }
            }
         }
      }

   return ok;
   }


bool Raster::_Rasterize( int col, float cellSize, RASTER_TYPE rtype, DRAW_VALUE drawValue )
   {
   if ( m_pMapWnd == NULL || m_pLayer == NULL )
      return false;

   m_rtype       = rtype;
   m_cellSize    = cellSize;
   m_noDataValue = m_pLayer->GetNoDataValue();

   int colorCount = 0;  // number of distinct colors to represent data
   int activeCol = m_pLayer->GetActiveField();

   GetRequiredRowsCols( m_pLayer, cellSize, m_pRasterRect, m_rows, m_cols );

   if ( m_pLayer->GetType() == LT_GRID )
      {
      Notify( NT_RASTERIZING, -1, m_rows );

      // set up map to display the correct column
      m_col = col = 0;
      ///m_valueType = m_pLayer->GetFieldType( col );
      ///m_pLayer->SetActiveField( col );
      ///TYPE type = m_pLayer->GetFieldType( col );
      int maxBins = -1;

      DO_TYPE doType = m_pLayer->m_pDbTable->GetDOType();
      if ( doType == DOT_FLOAT )
         {
         if ( m_rtype == RT_8BIT )
            maxBins = 256;
         else
            maxBins = 256*256;  // arbitrary, must be less the INT_MAX
         }
      
      // figure out bitmap info
      if ( m_pFieldInfo != NULL )
         colorCount = m_pFieldInfo->GetAttributeCount();
      else
         colorCount = maxBins;

      // set up bins
      m_pLayer->SetBins( col, BCF_GRAY_INCR, maxBins );
      m_pLayer->ClassifyData(); 
      }
   else
      {
      if ( col >= 0 )   // Note: col=-1 means were are generating an index raster
         {
         Notify( NT_RASTERIZING, -1, m_rows );

         // set up map to display the correct column
         m_col = col;
         m_valueType = m_pLayer->GetFieldType( col );
         m_pLayer->SetActiveField( col );
         TYPE type = m_pLayer->GetFieldType( col );
         int maxBins = -1;
         if ( type == TYPE_FLOAT || type == TYPE_DOUBLE )
            {
            if ( m_rtype == RT_8BIT )
               maxBins = 256;
            else
               maxBins = 256*256;  // arbitrary, must be less the INT_MAX
            }
         
         // figure out bitmap info
         if ( m_pFieldInfo != NULL )
            colorCount = m_pFieldInfo->GetAttributeCount();
         else
            colorCount = maxBins;

         // set up bins
         m_pLayer->SetBins( col, BCF_GRAY_INCR, maxBins );
         m_pLayer->ClassifyData(); 
         }
      else  // generating an index - one color per record
         {
         Notify( NT_INDEXING, -1, m_rows );

         colorCount = m_pLayer->GetRecordCount();
         m_valueType = TYPE_INT;
         }
      }  // end of ( layerType != LT_GRID )

   int bitCount = 8;
   int _colorCount = colorCount;
   if ( m_rtype == RT_32BIT )
      {
      bitCount = 32;
      _colorCount = 0;
      }

   BITMAPINFO bmInfo;
   bmInfo.bmiHeader.biSize = sizeof( bmInfo.bmiHeader );
   bmInfo.bmiHeader.biWidth = 10; //m_cols;
   bmInfo.bmiHeader.biHeight = 10; //m_rows;
   bmInfo.bmiHeader.biPlanes = 1;
   bmInfo.bmiHeader.biBitCount = bitCount;   // 256 colors/attributes max
   bmInfo.bmiHeader.biCompression = BI_RGB;
   bmInfo.bmiHeader.biSizeImage = 0;      // 0 for no comprression
   bmInfo.bmiHeader.biXPelsPerMeter = 0;
   bmInfo.bmiHeader.biYPelsPerMeter = 0;
   bmInfo.bmiHeader.biClrUsed = _colorCount;
   bmInfo.bmiHeader.biClrImportant = _colorCount;
  
   m_pBits = NULL;

   CDC dc;
   dc.CreateCompatibleDC( NULL );  // create a memory device context

   // we use a DIB because it contains the full color information.  This is used only to create 
   // a compatible bitmap
   HBITMAP hDibSection = CreateDIBSection( (HDC) dc, &bmInfo, DIB_RGB_COLORS, (void**) &m_pBits, NULL, 0 );
   ASSERT( hDibSection != NULL );

   // remember original display dimensions
   if ( m_pStartRect != NULL )
      delete m_pStartRect;

   REAL xMin, xMax, yMin, yMax;
   m_pLayer->GetMapPtr()->GetViewExtents( xMin, xMax, yMin, yMax );
   m_pStartRect = new COORD_RECT( xMin, yMin, xMax, yMax );
   
   // draw the layer onto the bitmap

   m_pMapWnd->m_pMap->m_displayRect.left = 0;
   m_pMapWnd->m_pMap->m_displayRect.top = 0;
   m_pMapWnd->m_pMap->m_displayRect.right = m_cols;
   m_pMapWnd->m_pMap->m_displayRect.bottom = m_rows;
   
   //m_pMap->InitPolyLogicalPoints();

   HBITMAP hOldBitmap = (HBITMAP) dc.SelectObject( (HBITMAP) hDibSection ); // select to provide template for compatible bitmap
   HBITMAP hCompBitmap = CreateCompatibleBitmap( dc, m_cols, m_rows );  // needed for drawing on
   dc.SelectObject( hCompBitmap );

   COLORREF outColor = m_pLayer->GetOutlineColor();
   m_pLayer->SetNoOutline();
   
   m_pMapWnd->ZoomFit( false );  // false = no redraw
   if ( this->m_pRasterRect != NULL )
      m_pMapWnd->ZoomRect( m_pRasterRect, false );
    
   if ( col < 0 ) // drawing an index
      drawValue = DV_INDEX;

   // draw background
   CBrush bkbr;
   bkbr.CreateSolidBrush( INT_MAX-1 );
   CBrush *pOldBrush = dc.SelectObject( &bkbr );
   dc.Rectangle( 0, 0, m_cols, m_rows );
   dc.SelectObject( pOldBrush );
   
   m_pMapWnd->DrawLayer( dc, m_pLayer, true, drawValue );
   
   if ( m_pLayer != m_pSourceLayer )   // polys weren't copied, so the source layers need to be reset to previous values
      {
      m_pLayer->SetOutlineColor( outColor );
      m_pSourceLayer->InitPolyLogicalPoints();
      }

   if ( drawValue == DV_VALUE || drawValue == DV_INDEX )
      m_bmpContainsValues = true;
   else
      m_bmpContainsValues = false;
    
   // clean up
   dc.SelectObject( hOldBitmap );
   DeleteObject( hDibSection );

   // take care of any type-specific details
   if ( m_rtype == RT_32BIT && ! m_bmpContainsValues )
      CreateColorToValueMap();

   else if ( m_rtype == RT_8BIT && ! m_bmpContainsValues )
      CreateIndexToValueMap();

   // restore map to prior state
   m_pLayer->SetActiveField( activeCol );
   m_pLayer->GetMapPtr()->SetViewExtents( m_pStartRect->ll.x,
               m_pStartRect->ur.x, m_pStartRect->ll.y, m_pStartRect->ur.y );
   m_pMapWnd->RedrawWindow();

   if( HBITMAP(m_dibSection) != NULL )
      m_dibSection.DeleteObject();

   m_dibSection.SetBitmap( hCompBitmap );
   DeleteObject( hDibSection );

   m_pBits = (LPBYTE) m_dibSection.GetDIBits();
   m_bytesPerLine = m_dibSection.BytesPerLine( m_cols, m_rtype==RT_8BIT ? 8 : 32 );

   // set the colors of the dib if necessary (note: 32bit dibs contain color values in the bitmap - no color table.
   if ( m_rtype == RT_8BIT && col >= 0 )
      {
      int colorTableSize = m_dibSection.GetColorTableSize(); 
   
      LPRGBQUAD colorTable = m_dibSection.GetColorTable();
      if ( m_pFieldInfo != NULL )
         {
         for ( int i=0; i < m_pFieldInfo->GetAttributeCount(); i++ )
            {
            FIELD_ATTR &attr = m_pFieldInfo->GetAttribute( i );
            colorTable[ i ].rgbBlue  = GetBValue( attr.color );
            colorTable[ i ].rgbGreen = GetGValue( attr.color );
            colorTable[ i ].rgbRed   = GetRValue( attr.color );
            }
         }
      else
         {
         BinArray *pBinArray = m_pLayer->GetBinArray( col, false );
         if ( pBinArray != NULL )
            {
            for ( int i=0; i < pBinArray->GetSize(); i++ )
               {
               Bin &bin = pBinArray->GetAt( i );
               colorTable[ i ].rgbBlue  = GetBValue( bin.m_color );
               colorTable[ i ].rgbGreen = GetGValue( bin.m_color );
               colorTable[ i ].rgbRed   = GetRValue( bin.m_color );
               }
            }
         else
            {
            // don't initializ table
            }
         }
      }

   if ( col >= 0 )
      Notify( NT_RASTERIZING, -2, m_rows );
   else
      Notify( NT_INDEXING, -2, m_rows );
   
   return true;
   }

/*
struct CIT_INFO {
   CWinThread *pThread;
   Raster *pRaster;
   UINT col;
   UINT row;
   UINT startCol;
   UINT endCol;
   int polyID;
   };

UINT Raster::CreateIndexThread( LPVOID pParam )
   {
   CIT_INFO *pInfo = (CIT_INFO*) pParam;
   pInfo->pThread = AfxGetThread();
   pInfo->pThread->m_bAutoDelete = FALSE;

   for ( pInfo->col=pInfo->startCol; pInfo->col <= pInfo->endCol; pInfo->col++ )
      {
      //int polyID;
      pInfo->pRaster->m_pIndexRaster->GetData( pInfo->row, pInfo->col, pInfo->polyID );
      //if ( polyID >= 0 && polyID < count )
      pInfo->pRaster->m_index[ pInfo->polyID ].Add( pInfo->row, pInfo->col );
      }

   return 0;   // indicate success
   }
*/

Raster *Raster::CreateIndexRaster( void )
   {
   if ( m_pIndexLayer == NULL)
      m_pIndexLayer = m_pSourceLayer;

   if ( m_pIndexLayer == NULL )
      return NULL;

   if ( m_pIndexRaster != NULL )
      {
      delete m_pIndexRaster;
      m_pIndexRaster = NULL;
      }

   if ( m_index != NULL )
      {
      delete [] m_index;
      m_index = NULL;
      }
   
   m_pIndexRaster = new Raster( m_pIndexLayer, NULL, m_pRasterRect, false );    // don't copy poly's - we can just reference the source layer polys/data
   if ( m_pIndexRaster == NULL )
      return NULL;

   m_pIndexRaster->_Rasterize( -1, m_cellSize, RT_32BIT, DV_INDEX );  // rasterize using index instead of column   
   
   return m_pIndexRaster;
   }


int Raster::CreateIndex( void )
   {
   // generated raster will have the index of the poly as it RGBQUAD
   int count = m_pSourceLayer->GetPolygonCount();
   ASSERT( count == m_pSourceLayer->GetRecordCount() );
   if ( m_pIndexRaster == NULL )
      CreateIndexRaster();

   if ( m_index != NULL )
      delete m_index;

   m_index = new RowColArray[ count ];

   if ( m_index == NULL )
      {
      delete m_pIndexRaster;
      m_pIndexRaster = NULL;
      return -5;
      }
   
   /*
   SYSTEM_INFO sysinfo;
   GetSystemInfo(&sysinfo);
   int numCores = sysinfo.dwNumberOfProcessors;
   int colsPerCore = m_cols / numCores;

   CIT_INFO *citInfoArray = new CIT_INFO[ numCores ];
   int col = 0;
   for ( int i=0; i < numCores; i++ )
      {
      citInfoArray[ i ].startCol = col;

      if ( i != numCores )
         citInfoArray[ i ].endCol = col+colsPerCore-1;
      else
         citInfoArray[ i ].endCol = m_cols-1;

      citInfoArray[ i ].pRaster = this;
      citInfoArray[ i ].row = -1;
      col += colsPerCore;
      }
   
   HANDLE *hArray = new HANDLE[ numCores ];

   for ( UINT row=0; row < m_rows; row++ )
      {
      Notify( NT_INDEXING, (int) row, (LONG_PTR) m_rows );

      for ( int i=0; i < numCores; i++ )
         {
         citInfoArray[ i ].row = row;
         CWinThread *pThread = AfxBeginThread( CreateIndexThread, &(citInfoArray[ i ]) );
         hArray[ i ] = pThread->m_hThread;
         }

      DWORD retVal = WaitForMultipleObjects( numCores, hArray, TRUE, INFINITE );
      ASSERT( retVal == WAIT_OBJECT_0 );
      }

   delete citInfoArray;
   delete hArray; */
      
   for ( UINT row=0; row < m_rows; row++ )
      {
      Notify( NT_INDEXING, (int) row, (LONG_PTR) m_rows );

      for ( UINT col=0; col < m_cols; col++ )
         {
         int polyID;
         m_pIndexRaster->GetData( row, col, polyID );
         if ( polyID >= 0 && polyID < count )
            m_index[ polyID ].Add( row, col );
         }
      }

   return m_rows*m_cols/count;
   }


bool Raster::SaveIndex( LPCTSTR filename /*=NULL*/ )
   {
   DirPlaceholder d;

   CString path;

   if ( filename == NULL || filename[ 0 ] == NULL )
      {
      CFileDialog dlg( FALSE, "rndx", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "Raster Index Files|*.rndx|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         path = dlg.GetPathName();
      }
   else
      path = filename;
   
   FILE *fp;
   PCTSTR file = (PCTSTR)path;
   errno_t err = fopen_s( &fp, file, "wb" );

   if ( err != 0 )
      return false;

   CWaitCursor c;
   fwrite( "RNDX0 ", 6, 1, fp );   
   fwrite( &m_cellSize, sizeof( float ), 1, fp );
   fwrite( &m_cellSize, sizeof( float ), 1, fp );
   fwrite( &m_rows, sizeof( int ), 1, fp );
   fwrite( &m_cols, sizeof( int ), 1, fp );
   
   char buffer[ 256 ];
   lstrcpy( buffer, m_pIndexLayer->m_path );
   fwrite( buffer, 256, 1, fp );

   int polyCount = m_pIndexLayer->GetPolygonCount();
   fwrite( &polyCount, sizeof( int ), 1, fp );

   // write ROWCOL count for each polygon
   for ( int i=0; i < polyCount; i++ )
      {
      RowColArray &rca = m_index[ i ];
      int count = (int) rca.GetCount();
      fwrite( &count, sizeof( int ), 1, fp );

      for ( int j=0; j < rca.GetCount(); j++ )
         {
         ROW_COL &rc = rca.GetAt( j );
         fwrite( &rc.row, sizeof( UINT ), 1, fp );
         fwrite( &rc.col, sizeof( UINT ), 1, fp );
         }
      }

   fclose( fp );
   return true;
   }


bool Raster::LoadIndex( LPCTSTR filename /*=NULL*/ )
   {
   DirPlaceholder d;

   CString path;

   if ( filename == NULL || filename[ 0 ] == NULL )
      {
      CFileDialog dlg( TRUE, "rndx", "", OFN_HIDEREADONLY,
            "Raster Index Files|*.rndx|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         path = dlg.GetPathName();
      }
   else
      path = filename;
   /*
   // get file size
   int fh;
   unsigned int nbytes = BUFSIZ;

   // Open a file 
   if( _sopen_s( &fh, path, _O_BINARY | _O_RDONLY, _SH_DENYNO, _S_IREAD ) == 0 )
      {
      nbytes = _filelength( fh );
      close( fh );
      }
   */

   FILE *fp;
   PCTSTR file = (PCTSTR)path;
   errno_t err = fopen_s( &fp, file, "rb" );

   if ( err != 0 )
      return false;

   CWaitCursor c;
   char buffer[ 256 ];

   size_t sz = fread( buffer, sizeof(char), 6, fp ); 
   if ( sz != 6 || strncmp( buffer, "RNDX0 ", 6 ) != 0 )
      {
      AfxMessageBox( "Error reading raster index - specificed file is not a valid raster index file" );
      return false;
      }

   float cellSize;
   fread( &cellSize, sizeof( float ), 1, fp );  // two of these...
   fread( &cellSize, sizeof( float ), 1, fp );

   UINT rows, cols;
   fread( &rows, sizeof( UINT ), 1, fp );
   fread( &cols, sizeof( UINT ), 1, fp );
   
   UINT _rows, _cols;
   if ( GetRequiredRowsCols( m_pLayer, cellSize, m_pRasterRect, _rows, _cols ) )
      {
      if ( _rows != rows || _cols != cols )
         {
         CString msg;
         msg.Format( "Source rows/cols (%i,%i) does not match Raster Index (%i,%i).  Indexing can not continue with this raster index file", 
                  _rows, _cols, rows, cols );
         AfxMessageBox( msg, MB_OKCANCEL );
         fclose( fp );
         return false;
         }
      }

   char sourceLayer[ 256 ];
   fread( sourceLayer, 256, 1, fp );

   if ( m_pIndexLayer->m_path.CompareNoCase( sourceLayer ) != 0 )
      {
      CString msg;
      msg.Format( "Source path (%s) does not match path used to generate Raster Index (%s).  Do you want to continue anyway?", 
                  m_pIndexLayer->m_path, sourceLayer );
      int retVal = AfxMessageBox( msg, MB_OKCANCEL );
      if ( retVal == IDCANCEL )
         {
         fclose( fp );
         return false;
         }
      }
            
   int polyCount;
   fread( &polyCount, sizeof( int ), 1, fp );

   if ( m_pIndexLayer->GetPolygonCount() != polyCount )
      {
      CString msg;
      msg.Format( "Polygon count mismatch between Index layer (%i) and Raster index file (%i).  The index is not valid for this layer", 
         m_pIndexLayer->GetPolygonCount(), polyCount );
      AfxMessageBox( msg, MB_OK );
      return false;
      }

   if ( m_index != NULL )
      delete m_index;


   Notify( NT_LOADINGINDEX, -1, polyCount );

   m_index = new RowColArray[ polyCount ];

   // read ROWCOL count for each polygon
   for ( int i=0; i < polyCount; i++ )
      {
      Notify( NT_LOADINGINDEX, i, polyCount );

      RowColArray &rca = m_index[ i ];
      int count;
      fread( &count, sizeof( int ), 1, fp );

      rca.SetSize( count );

      for ( int j=0; j < count; j++ )
         {
         ROW_COL &rc = rca.GetAt( j );
         fread( &rc.row, sizeof( UINT ), 1, fp );
         fread( &rc.col, sizeof( UINT ), 1, fp );
         }
      }

   Notify( NT_LOADINGINDEX, -2, polyCount );
   
   fclose( fp );
   return true;
   }


bool Raster::GetRequiredRowsCols( MapLayer *pLayer, float cellSize, COORD_RECT *pRect, UINT &rows, UINT &cols )
   {
   if ( pLayer == NULL )
      return false;

   if ( cellSize <= 0 )
      return false;

   REAL vxMin, vxMax, vyMin, vyMax;
   if ( pRect == NULL )
      pLayer->GetExtents( vxMin, vxMax, vyMin, vyMax );
   else
      {
      vxMin = min( pRect->ll.x, pRect->ur.x );
      vxMax = max( pRect->ll.x, pRect->ur.x );
      vyMin = min( pRect->ll.y, pRect->ur.y );
      vyMax = max( pRect->ll.y, pRect->ur.y );
      }
   
   cols = int( ( vxMax - vxMin ) / cellSize );
   rows = int( ( vyMax - vyMin ) / cellSize );

   return true;
   }


bool Raster::GetIndexHeader( LPCTSTR filename, float &cellSize, CString &layerName, int &polyCount, int &rows, int &cols )
   {
   DirPlaceholder d;

   CString path;

   if ( filename == NULL || filename[ 0 ] == NULL )
      {
      CFileDialog dlg( TRUE, "rndx", "", OFN_HIDEREADONLY,
            "Raster Index Files|*.rndx|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         path = dlg.GetPathName();
      }
   else
      path = filename;
   
   FILE *fp;
   PCTSTR file = (PCTSTR)path;
   errno_t err = fopen_s(&fp, file, "rb");

   if ( err != 0 )
      return false;

   CWaitCursor c;
   char buffer[ 256 ];

   size_t sz = fread( buffer, sizeof(char), 6, fp ); 
   if ( sz != 6 || strncmp( buffer, "RNDX0 ", 6 ) != 0 )
      {
      AfxMessageBox( "Error reading raster index - specificed file is not a valid raster index file" );
      return false;
      }

   fread( &cellSize, sizeof( float ), 1, fp );  // two fo these...
   fread( &cellSize, sizeof( float ), 1, fp );

   fread( &rows, sizeof( int ), 1, fp );
   fread( &cols, sizeof( int ), 1, fp );
   
   char sourceLayer[ 256 ];
   fread( sourceLayer, 256, 1, fp );
   layerName = sourceLayer;

   fread( &polyCount, sizeof( int ), 1, fp );

   fclose( fp );
   return true;
   }


int Raster::GetPolyIndex( UINT row, UINT col )
   {
   if ( m_pIndexRaster == NULL )
      return -1;

   int value;
   m_pIndexRaster->GetData(  row, col, value );

   return value;
   }


int Raster::CreateIndexToValueMap( void )
   {
   if ( m_rtype != RT_8BIT ) return -1;

   int valueCount = -2;

   if ( ::IsInteger( m_valueType ) )
      {
      m_indexToIValueMap.RemoveAll();

      if ( m_pFieldInfo != NULL )
         {
         // for each bitmap colre index table entry, find assocaited attribute value
         for ( int i=0; i < m_pFieldInfo->GetAttributeCount(); i++ )
            {
            FIELD_ATTR &attr = m_pFieldInfo->GetAttribute( i );
            int value;
            attr.value.GetAsInt( value );
            m_indexToIValueMap.Add( value );
            }
         }
      else
         {
         BinArray *pBinArray = m_pLayer->GetBinArray( m_col, false );
         for ( int i=0; i < pBinArray->GetSize(); i++ )
            {
            Bin &bin = pBinArray->GetAt( i );               
            int value = int (bin.m_maxVal+ bin.m_minVal )/2;
            m_indexToIValueMap.Add( value );
            }
         }

      valueCount = (int) m_indexToIValueMap.GetSize();
      }

   else if ( ::IsReal( m_valueType ) )
      {
      m_indexToFValueMap.RemoveAll();
   
      // for each bitmap colre index table entry, find assocaited attribute value
      if ( m_pFieldInfo != NULL )
         {
         for ( int i=0; i < m_pFieldInfo->GetAttributeCount(); i++ )
            {
            FIELD_ATTR &attr = m_pFieldInfo->GetAttribute( i );
            float value;
            attr.value.GetAsFloat( value );
            m_indexToFValueMap.Add( value );
            }
         }
      else
         {
         BinArray *pBinArray = m_pLayer->GetBinArray( m_col, false );
         for ( int i=0; i < pBinArray->GetSize(); i++ )
            {
            Bin &bin = pBinArray->GetAt( i );               
            float value = (bin.m_maxVal+ bin.m_minVal )/2;
            m_indexToFValueMap.Add( value );
            }
         }

      valueCount = (int) m_indexToFValueMap.GetSize();
      }
   else 
      {
      ASSERT( 0 );
      return false;  // text not current supported
      }

   return valueCount;
   }


int Raster::CreateColorToValueMap( void )
   {
   if ( m_rtype != RT_32BIT ) return -1;

   int valueCount = -2;

   if ( ::IsInteger( m_valueType ) )
      {
      m_colorToIValueMap.RemoveAll();

      if ( m_pFieldInfo != NULL )
         {
         for ( int i=0; i < m_pFieldInfo->GetAttributeCount(); i++ )
            {
            FIELD_ATTR &attr = m_pFieldInfo->GetAttribute( i );
            int value;
            attr.value.GetAsInt( value );
            m_colorToIValueMap.SetAt( attr.color, value );
            }
         }
      else
         {
         BinArray *pBinArray = m_pLayer->GetBinArray( m_col, false );
         for ( int i=0; i < pBinArray->GetSize(); i++ )
            {
            Bin &bin = pBinArray->GetAt( i );               
            int value = int (bin.m_maxVal+ bin.m_minVal )/2;
            m_colorToIValueMap.SetAt( bin.m_color, value );
            }
         }

      valueCount = (int) m_colorToIValueMap.GetSize();
      }

   else if ( ::IsReal( m_valueType ) )
      {
      m_colorToFValueMap.RemoveAll();

      if ( m_pFieldInfo )
         {
         for ( int i=0; i < m_pFieldInfo->GetAttributeCount(); i++ )
            {
            FIELD_ATTR &attr = m_pFieldInfo->GetAttribute( i );
            float value;
            attr.value.GetAsFloat( value );
            m_colorToFValueMap.SetAt( attr.color, value );
            }
         }
      else
         {
         BinArray *pBinArray = m_pLayer->GetBinArray( m_col, false );
         for ( int i=0; i < pBinArray->GetSize(); i++ )
            {
            Bin &bin = pBinArray->GetAt( i );               
            float value = (bin.m_maxVal+ bin.m_minVal )/2;
            m_colorToFValueMap.SetAt( bin.m_color, value );
            }
         }
      
      valueCount = (int) m_colorToFValueMap.GetSize();
      }

   return valueCount;
   }


bool Raster::ReplaceColorsWithValues( void )
   {
   if ( m_pFieldInfo == NULL )
      return false;

   if ( (HBITMAP) m_dibSection == NULL )
      return false;

   if ( ::IsReal( m_valueType ) && m_rtype != RT_32BIT )
      return false;
   
   if ( m_bmpContainsValues )
      return true;   // no further action required
    
   m_pBits = (LPBYTE) m_dibSection.GetDIBits();

   if ( m_rtype == RT_32BIT )  // color values
      {      
      for ( UINT row=0; row < m_rows; row++ )
         {
         LPBYTE lpStartOfLine = m_pBits + row * m_bytesPerLine;
      
         for ( UINT col=0; col < m_cols; col++ )
            {
            LPBYTE pStartOfPixel = lpStartOfLine + col * 4;  // 4 bytes per pixel for 32bit BMP
            RGBQUAD *pRGB = (RGBQUAD*) pStartOfPixel;

            COLORREF color = RGB( pRGB->rgbRed, pRGB->rgbGreen, pRGB->rgbBlue );

            if ( ::IsInteger( m_valueType ) )
               {
               int value = m_colorToIValueMap[ color ];
               int *pValue = (int*) pStartOfPixel;
               *pValue = value;
               }
            else if ( ::IsReal( m_valueType ) )
               {
               float value = m_colorToFValueMap[ color ];
               float *pValue = (float*) pStartOfPixel;
               *pValue = value;
               }
            else
               ASSERT( 0 );   // only floats, int currently supported
            }
         }
      }  // end of: for ( m_rtype == RT_32BIT )

   else if ( m_rtype == RT_8BIT )
      {
      int colorTableSize = m_dibSection.GetColorTableSize(); 
   
      LPRGBQUAD colorTable = m_dibSection.GetColorTable();
      for ( int i=0; i < m_pFieldInfo->GetAttributeCount(); i++ )
         {
         FIELD_ATTR &attr = m_pFieldInfo->GetAttribute( i );

         if ( ::IsInteger( m_valueType ) )
            {
            int value;
            attr.value.GetAsInt( value );

            int *pValue = (int*)(colorTable + i );
            *pValue = value;
            }
         else if ( ::IsReal( m_valueType ) )
            {
            float value;
            attr.value.GetAsFloat( value );

            float *pValue = (float*)(colorTable + i );
            *pValue = value;
            }
         else
            ASSERT( 0 );
         }
      }

   m_bmpContainsValues = true;

   return true;
   }


bool Raster::SaveDIB( LPCTSTR filename )
   {   
   DirPlaceholder d;

   CString path;

   if ( filename == NULL || filename[ 0 ] == NULL )
      {
      CFileDialog dlg( FALSE, "rndx", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "Raster Index Files|*.rndx|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         path = dlg.GetPathName();
      }
   else
      path = filename;
   

   if ( (HBITMAP) m_dibSection == NULL )
      return false;

   BOOL retVal = m_dibSection.Save( path );

   return retVal ? true : false;
   }


bool Raster::GetData( UINT row, UINT col, int &value )
   {
   if ( (HBITMAP) m_dibSection == NULL )
      return false;

   if ( row < 0 || row >= m_rows )
      return false;

   if ( col < 0 || col >= m_cols )
      return false;

   m_pBits = (LPBYTE) m_dibSection.GetDIBits();

   LPBYTE pStartOfLine = m_pBits + row * m_bytesPerLine;

   if ( m_rtype == RT_32BIT )
      {
      LPBYTE pStartOfPixel = pStartOfLine + col * sizeof( RGBQUAD );

      if ( m_bmpContainsValues )
         {
         RGBQUAD *pRGBQuad = (RGBQUAD*) pStartOfPixel;
         COLORREF color = FROMRGB( pRGBQuad );
         int red = GetRValue( color );
         int grn = GetGValue( color );
         int blu = GetBValue( color );

         if ( ::IsInteger( m_valueType ) ) 
            value = color;
         else
            ASSERT( 0 );
         }
            
      else  // doesn't contain values
         {
         LPRGBQUAD pRGB = (LPRGBQUAD) pStartOfPixel;
         COLORREF color = FROMRGB( pRGB );
         if ( ::IsInteger( m_valueType ) )
            value = m_colorToIValueMap[ color ];
         else if ( ::IsReal( m_valueType ) )
            {
            float fvalue = m_colorToFValueMap[ color ];
            value = int( fvalue );
            }
         else
            ASSERT( 0 );
         }
     
      return true;
      }

   else if ( m_rtype == RT_8BIT )
      {
      LPBYTE pStartOfPixel = pStartOfLine + col;

      if ( m_bmpContainsValues )
         value = int( *pStartOfPixel );
      else
         {
         BYTE index = *pStartOfPixel;
         value = m_indexToIValueMap[ (INT_PTR) index ];
         }
      }

   return true;
   }


bool Raster::GetData( UINT row, UINT col, float &value )
   {
   if ( (HBITMAP) m_dibSection == NULL )
      return false;

   if ( row < 0 || row >= m_rows )
      return false;

   if ( col < 0 || col >= m_cols )
      return false;

   m_pBits = (LPBYTE) m_dibSection.GetDIBits();

   LPBYTE pStartOfLine = m_pBits + row * m_bytesPerLine;

   if ( m_rtype == RT_32BIT )
      {
      LPBYTE pStartOfPixel = pStartOfLine + col * sizeof( RGBQUAD );

      if ( m_bmpContainsValues )
         {
         if ( ::IsInteger( m_valueType ) )
            {
            COLORREF color = FROMRGB( (RGBQUAD*) pStartOfPixel );
            int ivalue = (int) color;
            value = (float) ivalue;
            }
         else
            ASSERT( 0 );
         }
            
      else  // doesn't contain values
         {
         LPRGBQUAD pRGB = (LPRGBQUAD) pStartOfPixel;
         COLORREF color = FROMRGB( pRGB );
         if ( ::IsInteger( m_valueType ) )
            value = (float) m_colorToIValueMap[ color ];
         else if ( ::IsReal( m_valueType ) )
            value = m_colorToFValueMap[ color ];
         else
            ASSERT( 0 );
         }
     
      return true;
      }

   else if ( m_rtype == RT_8BIT )
      {
      LPBYTE pStartOfPixel = pStartOfLine + col;
      ASSERT( m_bmpContainsValues == false );

      BYTE index = *pStartOfPixel;
      value = (float) m_indexToIValueMap[ (INT_PTR) index ];
      }

   return true;
   }


bool Raster::GetData( int polyIndex, int &value, POLY_METHOD polyMethod )
   {
   if ( polyIndex < 0 || polyIndex > m_pSourceLayer->GetPolygonCount() )
      {
      ASSERT( 0 );
      return false;
      }

   value = (int) m_noDataValue;

   INT_PTR count = m_index[ polyIndex ].GetCount();
   if ( count == 0 )
      return true;
       
   value = 0;

   switch ( polyMethod )
      {
      case PM_SUM:
      case PM_AVG:
         {
         // iterate though pixels associated with this poly
         for ( INT_PTR i=0; i < count; i++ )
            {
            ROW_COL &rc = m_index[ polyIndex ].GetAt( i );

            int _value;
            if ( GetData( rc.row, rc.col, _value ) )
               value += _value;
            }

         if ( polyMethod == PM_AVG )
            value /= ((int) count);
         }
         break;

      case PM_MAJORITY:
         {
         // iterate though pixels associated with this poly
         CMap< int,int,int,int> m_valueCountMap;
         
         for ( INT_PTR i=0; i < count; i++ )
            {
            ROW_COL &rc = m_index[ polyIndex ].GetAt( i );  // get coords for the pixel

            int _value;
            if ( GetData( rc.row, rc.col, _value ) )
               {
               int countSoFar = 0;
               m_valueCountMap.Lookup( _value, countSoFar );
               m_valueCountMap.SetAt( _value, ++countSoFar );
               }
            }

         value = (int) m_noDataValue;
         int maxCount = 0;
         POSITION pos = m_valueCountMap.GetStartPosition();
         while (pos != NULL )
            {
            int _value, count;
            m_valueCountMap.GetNextAssoc( pos, _value, count );
            if ( count > maxCount )
               {
               value = _value;
               maxCount = count;
               }
            }
         }
      break;

      default:
         ASSERT( 0 );
      }

   return true;
   }


bool Raster::GetData( int polyIndex, float &value, POLY_METHOD polyMethod )
   {
   if ( polyIndex < 0 || polyIndex > m_pSourceLayer->GetPolygonCount() )
      {
      ASSERT( 0 );
      return false;
      }

   value = m_noDataValue;

   INT_PTR count = m_index[ polyIndex ].GetCount();
   if ( count == 0 )
      return true;
       
   value = 0;
      
   switch ( polyMethod )
      {
      case PM_SUM:
      case PM_AVG:
         { 
         for ( INT_PTR i=0; i < count; i++ )
            {
            ROW_COL &rc = m_index[ polyIndex ].GetAt( i );

            float _value;
            if ( GetData( rc.row, rc.col, _value ) )
               value += _value;
            }

         if ( polyMethod == PM_AVG )
            value /= count;
         }
         break;

      case PM_MAJORITY:
         {
         // iterate though pixels associated with this poly
         CMap< float,float,int,int> m_valueCountMap;
         
         for ( INT_PTR i=0; i < count; i++ )
            {
            ROW_COL &rc = m_index[ polyIndex ].GetAt( i );  // get coords for the pixel

            float _value;
            if ( GetData( rc.row, rc.col, _value ) )
               {
               int countSoFar = 0;
               m_valueCountMap.Lookup( _value, countSoFar );
               m_valueCountMap.SetAt( _value, ++countSoFar );
               }
            }

         value = m_noDataValue;
         int maxCount = 0;
         POSITION pos = m_valueCountMap.GetStartPosition();
         while (pos != NULL )
            {
            float _value;
            int count;
            m_valueCountMap.GetNextAssoc( pos, _value, count );
            if ( count > maxCount )
               {
               value = _value;
               maxCount = count;
               }
            }
         }
      break;

      default:
         ASSERT( 0 );
      }

   return true;
   }


bool Raster::GetPixelColor( UINT row, UINT col, COLORREF &color )
   {
   if ( (HBITMAP) m_dibSection == NULL )
      return false;

   if ( row < 0 || row >= m_rows )
      return false;

   if ( col < 0 || col >= m_cols )
      return false;

   if ( m_bmpContainsValues )
      return false;

   m_pBits = (LPBYTE) m_dibSection.GetDIBits();

   LPBYTE pStartOfLine = m_pBits + row * m_bytesPerLine;

   if ( m_rtype == RT_32BIT )
      {
      LPBYTE pStartOfPixel = pStartOfLine + col * sizeof( RGBQUAD );

      RGBQUAD *pRGBQuad = (RGBQUAD*) pStartOfPixel;
      color = FROMRGB( pRGBQuad );
      }

   else if ( m_rtype == RT_8BIT )
      {
      LPBYTE pStartOfPixel = pStartOfLine + col;

      int value = int( *pStartOfPixel );
      color = (COLORREF) value;
      }

   return true;
   }






// only works with numeric datatypes
bool Raster::CopyToSourceLayer( int col, POLY_METHOD polyMethod )
   {
   if ( m_pSourceLayer == NULL )
      return false;

   TYPE type = m_pSourceLayer->GetFieldType( col );

   if ( ! ::IsNumeric( type ) )
      return false;

   switch( type )
      {
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_LONG:
      case TYPE_ULONG:
         {
         for ( int i=0; i < m_pSourceLayer->GetRecordCount(); i++ )
            {
            int value;
            this->GetData( i, value, polyMethod );
            m_pSourceLayer->SetData( i, col, value );
            }
         }
         break;

      case TYPE_FLOAT:
      case TYPE_DOUBLE:
         {
         for ( int i=0; i < m_pSourceLayer->GetRecordCount(); i++ )
            {
            float value;
            this->GetData( i, value, polyMethod );
            m_pSourceLayer->SetData( i, col, value );
            }
         }
         break;

      default:
         return false;
      }

   return true;
   }


int Raster::GetPolyRowColArray( INT_PTR polyIndex, RowColArray **pRowColArray )
   {
   if ( ! IsIndexed() )
      return -1;

   if ( polyIndex < 0 || polyIndex >= m_pSourceLayer->GetRecordCount() )
      return -2;

   *pRowColArray = m_index + polyIndex;

   return (int) (*pRowColArray)->GetSize();
   }
