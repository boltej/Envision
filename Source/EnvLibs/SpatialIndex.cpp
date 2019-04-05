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
// SpatialIndex.cpp: implementation of the SpatialIndex class.
//
//////////////////////////////////////////////////////////////////////

#include "EnvLibs.h"

#include "SpatialIndex.h"
#include "Maplayer.h"
#include "MAP.h"
#include <math.h>

#include <omp.h>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
//=======================================================================================
// for qsort inplace of buckets
namespace 
   { 
   extern "C" int SI_spndx_compare( const void *arg1, const void *arg2 )
      {
      float d1 = ((SPATIAL_INFO*)arg1)->distance;
      float d2 = ((SPATIAL_INFO*)arg2)->distance;
      if (d1 < d2)
         return -1;
      else if (d1 > d2)
         return 1;
      return 0;
      }
   }
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SpatialIndex::SpatialIndex()
: m_pLayer( NULL ),
  m_pToLayer( NULL ),
  m_indexCount( -1 ),
  m_index( NULL ),
  m_entryCount( NULL ),
  m_isBuilt( false ),
  m_sizeofSPATIAL_INFO ( -1 ),
  m_maxCount( 0 ),
  m_allocatedIndexCount( -1 )
   { }

SpatialIndex::~SpatialIndex()
   {
   Clear();
   }


void SpatialIndex::Clear( void )
   {
   if ( m_index != NULL && m_entryCount != NULL && m_indexCount > 0 )
      {
      ASSERT( m_index != NULL );
      ASSERT( m_entryCount != NULL );

      for ( int i=0; i < m_indexCount; i++ )
         {
         if ( m_index[ i ] != NULL )
            delete [] (char*) m_index[ i ];  // cast because of m_sizeofSPATIAL_INFO
         }

      delete [] m_index;
      delete [] m_entryCount;
      }

   m_indexCount = 0;
   m_index      = NULL;
   m_entryCount = NULL;
   m_pLayer     = m_pToLayer = NULL;
   m_isBuilt    = false;
   m_sizeofSPATIAL_INFO = -1;
   m_maxCount   = 0;
   }


int SpatialIndex::BuildIndex( MapLayer *pLayer, MapLayer *pToLayer, int maxCount, float maxDistance, SI_METHOD method,
                             PFNC_SPATIAL_INFO_EX fillEx)
   {
   Clear();

   m_pLayer = pLayer;

   if ( pLayer == NULL )
      return -1;

   if ( pToLayer == NULL )
      m_pToLayer = pLayer;
   else
      m_pToLayer = pToLayer;

   // allocate the index pointers
   m_indexCount  = pLayer->GetRecordCount( MapLayer::ALL );
   m_index  = new SPATIAL_INFO* [ m_indexCount ];
 
   m_maxDistance = maxDistance;
   m_method      = method;

   if ( m_index == NULL )
      return -2;
   
   memset( (void*) m_index, 0, m_indexCount*sizeof( SPATIAL_INFO* ) );      // NULL out the memory

   // allocate the size array
   m_entryCount = new int[ m_indexCount ];
   if ( m_entryCount == NULL )
      return -3;

   m_allocatedIndexCount = m_indexCount;

   // allocate a temporary array to store information
   // Note the accomodation of SPATIAL_INFO ex tended info
   int   *neighborArray = new int  [ maxCount ];
   float *distanceArray = new float[ maxCount ];
   void **vex = NULL;
   m_sizeofSPATIAL_INFO = sizeof(SPATIAL_INFO)-sizeof(void*);
   if (NULL != fillEx) 
      {
      vex  = new void*[ maxCount ];
      m_sizeofSPATIAL_INFO = sizeof(SPATIAL_INFO);
      }

   int   fcount = 0;
   char * offset = NULL;
   SPATIAL_INFO * _si_ = NULL;
   
   //int iCPU = omp_get_num_procs();
   //omp_set_num_threads(iCPU);

   int loopcount = 0;

   //#pragma omp parallel for
   for ( int i=0; i < m_indexCount; i++ )
      {
      Poly *pPoly = pLayer->GetPolygon( i );
      ASSERT( pPoly != NULL );

#ifndef NO_MFC
      MSG  msg;
      while( PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE ) )
         {
         TranslateMessage( &msg );
         DispatchMessage( &msg );
         }
#else
      // There may be the need for a Linux messaging event loop here, depending on 
      //what the WINDOWS one above is used for.
#endif
      if ( m_pLayer->m_pMap->Notify( NT_BUILDSPATIALINDEX, i, m_indexCount ) == 0 )
         return -2;

      //If centroid method is used on a line coverage, set maxDistance to 90% of polygon straight line length  mmc 4/17/03
      if (pLayer->m_layerType == LT_LINE && method == SIM_CENTROID)
         {
         float polyLength = (float)sqrt( pow( ( pPoly->m_xMax - pPoly->m_xMin ), 2.0f ) + pow( ( pPoly->m_yMax - pPoly->m_yMin ), 2.0f ) );
         maxDistance = 0.9f * polyLength;
         }

      int count = pLayer->GetNearbyPolys( pPoly, neighborArray, distanceArray, maxCount, maxDistance, method, pToLayer );

      if ( count > m_maxCount )
         m_maxCount = count;

      if ( count == maxCount )
         fcount++;

      m_entryCount[ i ] = count;
      
      if ( count > 0 )
         {       
         m_index[ i ] = (SPATIAL_INFO*) new char [m_sizeofSPATIAL_INFO*count];
         offset = (char*)m_index [ i ];

         for ( int j=0; j < count; j++ )
            {
            _si_ = (SPATIAL_INFO*) offset; 
            _si_->distance = distanceArray[ j ];
            _si_->polyIndex = neighborArray[ j ];
            if(NULL != fillEx)
               _si_->ex = fillEx(i,j);

            offset += m_sizeofSPATIAL_INFO;
            }

         qsort(m_index[i], count, m_sizeofSPATIAL_INFO, SI_spndx_compare);
         }

      // else, m_index[i] = NULL from memset above
      loopcount++;
      }

   delete [] neighborArray;
   delete [] distanceArray;
   delete [] vex;

   m_isBuilt = true;

   if ( fcount > 0 )
      TRACE( "Warning: %d polygons could have more neighbors then are listed in the spatial index", fcount );

   return m_indexCount;
   }


int SpatialIndex::WriteIndex( LPCTSTR filename )
   {
   if ( m_indexCount <= 0 )
      return -1;

   FILE *fp;
   fopen_s( &fp, filename, "wb" );    // write binary

   if ( fp == NULL )
      return -2;

   int byteCount = 0;

   // file format:
   // 1) header includes version, m_indexCount, m_method, maxDistance, from/to records (v2+)
   // 2) path/name of source file, 128 bytes (v2+)
   // 3) path/name of to file, 128 bytes (v2+)
   // 5) array of entrySizes (ints) - m_indexCount long
   // 6) matrix of SPATIAL_INFO's
   // 7) bytecount and end of file marker

   // 1) header
   int version = 3; //2
   int recordCount   = m_pLayer->GetRecordCount( MapLayer::ALL );
   int toRecordCount = m_pToLayer->GetRecordCount( MapLayer::ALL );

   fwrite( (void*) &version,       sizeof( version ),       1, fp );
   fwrite( (void*) &m_indexCount,  sizeof( m_indexCount ),  1, fp );
   fwrite( (void*) &m_method,      sizeof( m_method ),      1, fp );
   fwrite( (void*) &m_maxDistance, sizeof( m_maxDistance ), 1, fp );
   fwrite( (void*) &m_sizeofSPATIAL_INFO, sizeof( int ), 1, fp ); // ***NEW VERSION 3***
   fwrite( (void*) &recordCount,   sizeof( int ), 1, fp );
   fwrite( (void*) &toRecordCount, sizeof( int ), 1, fp );

   char path[ 128 ]; // MAXPATH
   int szChar = sizeof( char );
   lstrcpy( path, m_pLayer->m_path );
   fwrite( (void*) path, szChar, 128, fp );

   lstrcpy( path, m_pToLayer->m_path );
   fwrite( (void*) path, szChar, 128, fp );


   byteCount = 16 + 2*szChar;


   // 2) array of entrySizes
   fwrite( (void*) m_entryCount, sizeof( int ), m_indexCount, fp );
   byteCount += m_indexCount * sizeof( int );

   // 3) matrix of SPATIAL_INFO's
   for ( int i=0; i < m_indexCount; i++ )
      {
      if ( m_entryCount[ i ] > 0 )
         {
         fwrite( (void*) m_index[ i ], m_sizeofSPATIAL_INFO, m_entryCount[ i ], fp );
         byteCount += m_entryCount[ i ] * m_sizeofSPATIAL_INFO;
         }
      }

   // 4) bytecount and end of file marker
   fwrite( (void*) &byteCount, sizeof( int ), 1, fp );

   int eof = 0xFFFF;
   fwrite( (void*) &eof, sizeof( int ), 1, fp );

   fclose( fp );
   return m_indexCount;
   }


//function call to replace goto error calls
inline int ReadIndex_Error (FILE* fp)
{
  fclose(fp);
  return -3;
}

int SpatialIndex::ReadIndex( LPCTSTR filename, MapLayer *pLayer, MapLayer *pToLayer, int includeRecordCount  )
   {
   bool writeWhenDone = false;

   Clear();
   lstrcpy( m_filename, filename );

   FILE *fp;
   fopen_s( &fp, filename, "rb" );    // read binary
   int i;

   if ( fp == NULL )
      {
      CString msg( "Spatial Index - unable to open file ");
      msg += filename;
      Report::Log( msg );
      return -2;
      }

   m_pLayer = pLayer;

   if ( pToLayer != NULL )
      m_pToLayer = pToLayer;
   else
      m_pToLayer = pLayer;

   m_isBuilt = false;
 
   // 1) header
   int version;
   if ( fread( (void*) &version,       sizeof( version ),       1, fp ) == 0 ) ReadIndex_Error(fp);
   if ( fread( (void*) &m_indexCount,  sizeof( m_indexCount ),  1, fp ) == 0 ) ReadIndex_Error(fp);
   if ( fread( (void*) &m_method,      sizeof( m_method ),      1, fp ) == 0 ) ReadIndex_Error(fp);
   if ( fread( (void*) &m_maxDistance, sizeof( m_maxDistance ), 1, fp ) == 0 ) ReadIndex_Error(fp);

   if  ( version >= 3 ) 
      {
      if ( fread( (void*) &m_sizeofSPATIAL_INFO, sizeof( m_sizeofSPATIAL_INFO ), 1, fp ) == 0 ) ReadIndex_Error(fp);
      }
   else
      m_sizeofSPATIAL_INFO = sizeof(SPATIAL_INFO) - sizeof(void*);

   if ( version >= 2 )
      {
      int recordCount, toRecordCount;
      if ( fread( (void*) &recordCount,   sizeof( int ), 1, fp ) == 0 ) ReadIndex_Error(fp);
      if ( fread( (void*) &toRecordCount, sizeof( int ), 1, fp ) == 0 ) ReadIndex_Error(fp);

      ASSERT( recordCount == m_indexCount );

      // make sure, if including all records, that the layer matches the record
      if ( includeRecordCount < 0 && recordCount != m_pLayer->GetRecordCount( MapLayer::ALL ) )
         {
         Report::WarningMsg( "Spatial Index: Invalid source record count in index - ignoring index" );
         ReadIndex_Error(fp);
         }

      // if not including all records, make sure there are enough in the index
      if ( includeRecordCount > 0 && recordCount < includeRecordCount )
         {
         Report::WarningMsg( "Spatial Index: Insufficient records in index to satisfy included record count - ignoring index" );
         ReadIndex_Error(fp);
         }

      if ( includeRecordCount < 0 && toRecordCount != m_pToLayer->GetRecordCount( MapLayer::ALL ) )
         {
         Report::WarningMsg( "Spatial Index: Invalid 'to' record count in index - ignoring index" );
         ReadIndex_Error(fp);
         }

      // if not including all records, make sure there are enough in the index
      if ( includeRecordCount > 0 && toRecordCount < includeRecordCount )
         {
         Report::WarningMsg( "Spatial Index: Insufficient 'to' records in index to satisfy included record count - ignoring index" );
         ReadIndex_Error(fp);
         }

      char path[ 128 ];
      if ( fread( (void*) path, sizeof( char ), 128, fp ) == 0 ) ReadIndex_Error(fp);

      int retVal = -99;
      if ( lstrcmpi( path, m_pLayer->m_path ) != 0 )
         {
#ifndef NO_MFC
         CString msg( "Spatial Index: This spatial index was built with a different source file, but contains the same number of polygons.  Layer " );
         msg += m_pLayer->m_path;
         msg += ", file was built with ";
         msg += path;
         msg += ". Do you want to associate this index with the new source file?" ;
         retVal = AfxGetMainWnd()->MessageBox( msg, "Warning", MB_YESNOCANCEL );
         switch( retVal )
            {
            case IDYES:
               writeWhenDone = true;
               break;

            case IDNO:
               break;

            case IDCANCEL:
               ReadIndex_Error(fp);
            }
#else
	 //use error action
	 ReadIndex_Error(fp);
#endif
         }
         
      if ( fread( (void*) path, sizeof( char ), 128, fp ) == 0 ) ReadIndex_Error(fp);

      if ( lstrcmpi( path, m_pToLayer->m_path ) != 0 && retVal == -99 )
         {
#ifndef NO_MFC
         CString msg( "Spatial Index: This spatial index was built with a different source file, but contains the same number of polygons.  Layer " );
         msg += m_pToLayer->m_path;
         msg += ", file was built with ";
         msg += path;
         msg += ". Do you want to associate this index with the new source file?" ;
         retVal = AfxGetMainWnd()->MessageBox( msg, "Warning", MB_YESNOCANCEL );
         switch( retVal )
            {
            case IDYES:
               writeWhenDone = true;
               break;

            case IDNO:
               break;

            case IDCANCEL:
               ReadIndex_Error(fp);
            }
#else
	 //should have option, but for now set write when done
	 writeWhenDone=true;
	 //	 ReadIndex_Error(fp);

#endif
         }
      }



   // 2) array of entrySizes
   int totalIndexCount = m_indexCount;

   if ( includeRecordCount > 0 )
      m_indexCount = includeRecordCount;

   int unusedRecordCount = totalIndexCount-m_indexCount;

   m_allocatedIndexCount = m_indexCount;

   // allocate an array of ints holds the size of the the array in each records entry in the index
   if ( ( m_entryCount = new int[ m_indexCount ] ) == NULL ) ReadIndex_Error(fp);

   // allocate and aray of SPATIAL_INFO ptrs to hold the SPATIAL_INFO arrays
   if ( ( m_index = new SPATIAL_INFO*[ m_indexCount ] ) == NULL ) ReadIndex_Error(fp);

   // read the needed entryCount records into this SpatialIndex
   if ( fread( (void*) m_entryCount, sizeof( int ), m_indexCount, fp ) == 0 ) ReadIndex_Error(fp);

   // if there is any non-included entrys, find out if any are empty
   int nullEntryCount = 0;
   if ( unusedRecordCount > 0 )
      {
      int *unusedEntryArray = new int[ unusedRecordCount  ];
      if ( fread( (void*) unusedEntryArray, sizeof( int ), unusedRecordCount, fp ) == 0 ) ReadIndex_Error(fp);

      for ( int i=0; i < unusedRecordCount; i++ )
         if ( unusedEntryArray[ i ] == 0 )
            nullEntryCount++;

      delete [] unusedEntryArray;
      }

   // now pointing to the start of the SPATIAL_INFO , start reading those....

   // 3) matrix of SPATIAL_INFO's
   for ( i=0; i < m_indexCount; i++ )
      {
      m_pLayer->m_pMap->Notify( NT_BUILDSPATIALINDEX, i, m_indexCount );

      int count = m_entryCount[ i ];

      if ( count > m_maxCount )
         m_maxCount = count;

      if ( count > 0 )
         {
         if ( (m_index[ i ] = (SPATIAL_INFO*)new char [m_sizeofSPATIAL_INFO * count] ) == NULL ) ReadIndex_Error(fp);
         if ( fread( (void*) m_index[ i ], m_sizeofSPATIAL_INFO, count, fp ) == 0 ) ReadIndex_Error(fp);
         }
      else
         m_index[ i ] = NULL;
      }

   if ( unusedRecordCount > 0 )
      if ( fseek( fp, (unusedRecordCount-nullEntryCount)*m_sizeofSPATIAL_INFO, SEEK_CUR ) != 0 ) ReadIndex_Error(fp);

   // make sure we're at the eof marker
   int eof;
   if ( fread( (void*) &eof, sizeof( int ), 1, fp ) == 0 ) ReadIndex_Error(fp); // first is bytecount
   if ( fread( (void*) &eof, sizeof( int ), 1, fp ) == 0 ) ReadIndex_Error(fp); //second is eof

   if ( eof != 0xFFFF )
      ReadIndex_Error(fp);

   fclose( fp );

   m_isBuilt = true;

   if ( writeWhenDone )
      WriteIndex( filename );

   return m_indexCount;

// error:
//    fclose( fp );
//    return -3;
   }

   
int SpatialIndex::GetNeighborArraySize( int index )
   {
   if ( m_deadList.GetLength() > 0 )
      {
      if ( m_deadList[ index ] )
         {
         ASSERT(0);
         return -1;
         }
      }

   if ( index < 0 || index > m_indexCount-1 )
      {
      ASSERT(0);
      return -m_indexCount;
      }

   int count = m_entryCount[ index ];
   int nSize = 0;

   // iterate though all entries in the array
   for ( int j=0; j < count; j++ )
      {
      int polyIndex = PolyIndex(index, j);   // get the polygon offset for this entry

      if ( polyIndex < m_pToLayer->GetPolygonCount() )  // in case the full map wasn't loaded!
         nSize++;
      }

   return nSize;
   }


int SpatialIndex::GetNeighborArray( int index, int *neighborArray, float *distanceArray, int maxCount, void ** ex )
   {
   if ( m_deadList.GetLength() > 0 )
      {
      if ( m_deadList[ index ] )
         {
         ASSERT(0);
         return -1;
         }
      }

   if ( index < 0 || index > m_indexCount-1 )
      {
      ASSERT(0);
      return -m_indexCount;
      }

   int count = maxCount;

   if ( m_entryCount[ index ] < count )
      count = m_entryCount[ index ];

   int nSize = 0;

   // iterate though all entries in the array
   for ( int j=0; j < count; j++ )
      {
      int polyIndex = PolyIndex(index, j);   // get the polygon offset for this entry

      if ( polyIndex < m_pToLayer->GetPolygonCount() )  // in case the full map wasn't loaded!
         {
         nSize++;
         neighborArray[ nSize-1 ] = polyIndex;   // get the polygon offset for this entry 
         ASSERT( m_deadList.GetLength() == 0 || m_deadList[ polyIndex ] == false );

         if ( distanceArray != NULL )
            distanceArray[ nSize-1 ] = Distance(index, j); 

         if (NULL != ex)
            ex [ nSize-1 ] = Ex(index, j);
         }
      }

   return nSize;
   }


bool SpatialIndex::GetNeighborDistance( int thisPolyIndex, int toPolyIndex, float &distance )
   {
   if ( thisPolyIndex > m_indexCount-1 )
      return false;

   for ( int j=0; j < m_entryCount[ thisPolyIndex ]; j++ )
      {
      if ( PolyIndex(thisPolyIndex, j) == toPolyIndex )
         {
         distance = Distance(thisPolyIndex, j);
         return true;
         }
      }

   return false;
   }

bool SpatialIndex::HasExtendedSpatialInfoEx(LPCTSTR filename, int & errorCode)
   {
   errorCode = 0;

   FILE *fp;
   fopen_s( &fp, filename, "rb" );    // read binary

   if ( fp == NULL )
      {
      errorCode = -2;
      return false;
      }

   // 1) header
   int version, indexCount, method, sizeofSPATIAL_INFO;
   float maxDistance;

   if ( fread( (void*) &version,     sizeof( version ),       1, fp ) == 0 ) goto error;
   if ( fread( (void*) &indexCount,  sizeof( indexCount ),  1, fp ) == 0 ) goto error;
   if ( fread( (void*) &method,      sizeof( method ),      1, fp ) == 0 ) goto error;
   if ( fread( (void*) &maxDistance, sizeof( maxDistance ), 1, fp ) == 0 ) goto error;

   if  ( version >= 3 ) 
      {
      if ( fread( (void*) &sizeofSPATIAL_INFO, sizeof( sizeofSPATIAL_INFO ), 1, fp ) == 0 ) goto error;
      }
   else
      {
      sizeofSPATIAL_INFO = sizeof(SPATIAL_INFO) - sizeof(void*);
      }
   return (sizeofSPATIAL_INFO == sizeof(SPATIAL_INFO));

error:
   fclose( fp );
   errorCode = -1;
   return false;
   }


int SpatialIndex::GetSpatialIndexInfo( LPCTSTR filename, float &distance, int &indexCount )
   {
   FILE *fp;
   fopen_s( &fp, filename, "rb" );

   if ( fp == NULL )
      {
      CString msg( "Spatial Index: Unable to find file ");
      msg += filename;
      Report::Log( msg );
      return -1;
      }

   int version, count, method;
   float maxDistance;
   if ( fread( (void*) &version,     sizeof( version ),     1, fp ) == 0 ) goto error;
   if ( fread( (void*) &count,       sizeof( count ),  1, fp ) == 0 ) goto error;
   if ( fread( (void*) &method,      sizeof( method ),      1, fp ) == 0 ) goto error;
   if ( fread( (void*) &maxDistance, sizeof( maxDistance ), 1, fp ) == 0 ) goto error;

   fclose( fp );

   distance = maxDistance;
   indexCount = count;
   return 1;

error:
   fclose( fp );
   return -1;
   }

// BuildList(.) -- Elementry List modification function
//    This function builds a row in m_index from an existing row with a few modification
//       forCell    = list for this cell will be overwritten by the one built by this function
//                    if forCell > m_indexCount then forCell = m_indexCount + 1
//       fromCell   = list from this cell is the existing row that is to be modified
//       minusCells = any cell in this list will be not be in the final row
//       unionCells = any cell in this list will be in the final row
void SpatialIndex::BuildList( int forCell, int fromCell, const CDWordArray &minusCells, const CDWordArray &unionCells )
   {
   if ( forCell > m_indexCount )
      {
      ASSERT(0);
      return;
      }
   else if ( forCell == m_indexCount )  // reallocate
      {
      ASSERT( forCell == m_indexCount );
      ASSERT( m_indexCount <= m_allocatedIndexCount );

      if ( m_indexCount == m_allocatedIndexCount )
         {
         m_allocatedIndexCount += m_allocatedIndexCount/10;

         if ( m_deadList.GetLength() < (UINT) m_allocatedIndexCount )
            m_deadList.SetLength( m_allocatedIndexCount );

         // allocate the index
         SPATIAL_INFO* *pNew = new SPATIAL_INFO* [ m_allocatedIndexCount ];
         ASSERT( pNew );

         // allocate the size array
         int *pNewEntry = new int[ m_allocatedIndexCount ];
         ASSERT( pNewEntry );

         // Copy existing
         memcpy( (void*) pNew,      (const void*) m_index,      m_indexCount*sizeof( SPATIAL_INFO* ) );
         memcpy( (void*) pNewEntry, (const void*) m_entryCount, m_indexCount*sizeof( int ) );

         // NULL out the rest of the memory
         memset( (void*) (pNew + m_indexCount     ), 0, (m_allocatedIndexCount-m_indexCount)*sizeof( SPATIAL_INFO* ) );
         memset( (void*) (pNewEntry + m_indexCount), 0, (m_allocatedIndexCount-m_indexCount)*sizeof( int ) );

         // delete old
         delete [] m_index;
         delete [] m_entryCount;
         m_index = pNew;
         m_entryCount = pNewEntry;
         }
      }

   int minusCount = (int) minusCells.GetCount();
   int unionCount = (int) unionCells.GetCount();
   int fromCount  = m_entryCount[ fromCell ];
   int toCount    = 0;
   
   SPATIAL_INFO * _si_   = NULL;
   SPATIAL_INFO * newRow = (SPATIAL_INFO*) new char [ m_sizeofSPATIAL_INFO*( fromCount + unionCount ) ]; // could be too big
   char * offset = (char*) newRow;
   
   bool skip;
   int neigh;

   // list of fromCell minus minusCells
   for ( int j=0; j < fromCount; j++ )
      {
      neigh = PolyIndex( fromCell, j );

      skip = false;                 
      for ( int i=0; i<minusCount; i++ )
         {
         if ( minusCells[i] == neigh )
            {
            skip = true;
            break;
            }
         }

      if ( skip )        // skip minus cell that's in fromCell's list of spatial-neighbors
         continue;
      
      _si_ = (SPATIAL_INFO*) offset;
      _si_->polyIndex = neigh;

      // Distance between fromCell and neigh
      if ( forCell == fromCell )
         {
         _si_->distance  = Distance( fromCell, j );
         
         //if(NULL != fillEx)
         //   _si_->ex = fillEx(i,j);
         if ( m_sizeofSPATIAL_INFO == sizeof(SPATIAL_INFO) )
            _si_->ex = 0;
         }
      else
         {
         Poly *pPoly1 = m_pLayer->GetPolygon( fromCell );
         Poly *pPoly2 = m_pLayer->GetPolygon( neigh );
         _si_->distance = (float) pPoly1->NearestDistanceToPoly( pPoly2 );
         //if(NULL != fillEx)
         //   _si_->ex = fillEx(i,j);
         if ( m_sizeofSPATIAL_INFO == sizeof(SPATIAL_INFO) )
            _si_->ex = 0;
         }

      offset += m_sizeofSPATIAL_INFO;
      toCount++;
      }

   // unionCells
   for ( int j=0; j<unionCount; j++ )
      {
      neigh = unionCells.GetAt(j);

      _si_ = (SPATIAL_INFO*) offset;
      _si_->polyIndex = neigh;

      Poly *pPoly1 = m_pLayer->GetPolygon( fromCell );
      Poly *pPoly2 = m_pLayer->GetPolygon( neigh );
      _si_->distance = (float) pPoly1->NearestDistanceToPoly( pPoly2 );
      //if(NULL != fillEx)
      //   _si_->ex = fillEx(i,j);
      if ( m_sizeofSPATIAL_INFO == sizeof(SPATIAL_INFO) )
         _si_->ex = 0;

      offset += m_sizeofSPATIAL_INFO;
      toCount++;
      }

   if ( forCell >= m_indexCount )
      {
      ASSERT( forCell == m_indexCount );
      m_indexCount++;
      }
   else
      {
      ASSERT( m_index[ forCell ] != NULL );
      delete [] (char*) m_index[ forCell ]; // cast because of m_sizeofSPATIAL_INFO
      m_index[ forCell ] = NULL;
      }

   ASSERT( m_index[ forCell ] == NULL );
   m_index[ forCell ] = (SPATIAL_INFO*) new char [ m_sizeofSPATIAL_INFO*toCount ];
   memcpy( (void*) m_index[ forCell ], (const void*) newRow, m_sizeofSPATIAL_INFO*toCount );
   
   m_entryCount[ forCell ] = toCount;

   qsort( m_index[ forCell ], toCount, m_sizeofSPATIAL_INFO, SI_spndx_compare );
   delete [] (char*) newRow;
   }

void SpatialIndex::Subdivide( int parentCell )
   {
   Poly *pPoly = m_pLayer->GetPolygon( parentCell );

   CDWordArray unionList;
   CDWordArray minusList;
   int childCount = pPoly->GetChildCount();
   
   // Build a list for every child consisting of the children and the parent's extant list (of neighbors). 
   int childId;
   for ( int i=0; i<childCount; i++ )
      {
      unionList.RemoveAll();

      for ( int j=0; j<childCount; j++ )
         {
         if ( i == j )
            continue;

         unionList.Add( pPoly->GetChild(j) );
         }

      childId = pPoly->GetChild(i);
      BuildList( childId, parentCell, minusList, unionList );
      m_deadList.Set( childId, false );
      }

   // Rebuild the list for every cell in the parent's list, 
   // minus-ing away the parent and union-adding the children.
   //
   int parentsNeighCount = m_entryCount[ parentCell ];
   unionList.RemoveAll();
   minusList.RemoveAll();

   minusList.Add( parentCell );
   for ( int j=0; j<childCount; j++ )
      {
      unionList.Add( pPoly->GetChild(j));
      }

   for ( int i=0; i<parentsNeighCount; i++ )
      {
      int rebuildCell = PolyIndex( parentCell, i );
      BuildList( rebuildCell, rebuildCell, minusList, unionList );
      }

   // kill the parent's entry
   m_deadList.Set( parentCell, true );
   }

void SpatialIndex::UnSubdivide( int parentCell,  CDWordArray & childIdList )
   {   
   // As called from UnApplyDeltaArray(.), this UnSubdivide(.) fails 07Mar2006
   //
   Poly *pPoly = m_pLayer->GetPolygon( parentCell );

   CDWordArray unionList;
   CDWordArray minusList;
   int childCount = pPoly->GetChildCount();

   ASSERT(childCount == childIdList.GetSize());
   
   // Kill child entries
   int childId;
   for ( int i=0; i<childCount; i++ )
      {
      childId = childIdList.GetAt(i); // pPoly->GetChild(i)->m_id;
      m_deadList.Set( childId, true );
      minusList.Add( childId );
      }
   // N.B. minusList == childIdList

   unionList.Add( parentCell );

   // Rebuild the parent's neighbor's list for each (neighbor) cell in the parent's list 
   int parentsNeighCount = m_entryCount[ parentCell ];

   for ( int i=0; i<parentsNeighCount; i++ )
      {
      int rebuildCell = PolyIndex( parentCell, i );
      BuildList( rebuildCell, rebuildCell, minusList, unionList );
      }

   // Turn the parents entry back on
   m_deadList.Set( parentCell, false );

   // remove the child entries
   for ( int j = childCount-1; j >= 0; j-- )
      {
      int index = childIdList.GetAt(j); //pPoly->GetChild(j)->m_id;
      ASSERT( index == m_indexCount - 1 );

      delete [] (char*) m_index[ index ];  // cast because of m_sizeofSPATIAL_INFO
      m_index[ index ] = NULL;
      m_indexCount--;
      }
   }
