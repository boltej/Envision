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
// SpatialIndex.h: interface for the SpatialIndex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPATIALINDEX_H__02496309_1AB3_4128_AAB8_24DD0FBADF15__INCLUDED_)
#define AFX_SPATIALINDEX_H__02496309_1AB3_4128_AAB8_24DD0FBADF15__INCLUDED_

#include "EnvLibs.h"


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BitArray.h"

class MapLayer;


// The SpatialIndex consists of jagged 2D matrix of  SPATIAL_INFO.
// SPATIAL_INFO provides the basic structure for relating a polygon index (offset) in the spatial index
// with a distance.  As used by class SpatialIndex, it is a variable size struct:  either
// sz=sizeof(SPATIAL_INFO) or sz-sizeof(void*).  PFNC_SPATIAL_INFO_EX is an optional function
// provided to SpatialIndex::BuildIndex that assigns to SPATIAL_INFO::ex.  
// If the index is built without this function, then the SPATIAL_INFO is the smaller size.
// The SpatialIndex members, float Distance(int i, int j), int PolyIndex(int i, int j),
// void * Ex(int i, int j) provide access to SPATIAL_INFO members regardless of size
// 

typedef void * (*PFNC_SPATIAL_INFO_EX)(int polyFrom, int polyTo); 

// SPATIAL_INFO is used as variable sized; member void *ex may not exist.
class SpatialIndex;

struct SPATIAL_INFO
   {
   friend SpatialIndex;

   public:
      int   polyIndex;
      float distance;     // int for speed, could be a float for accuracy
   
   private:
      void * ex;          // ???
   };


enum SI_METHOD { SIM_NEAREST, SIM_CENTROID };


class LIBSAPI SpatialIndex  
{
friend class MapLayer;

public:
	SpatialIndex();
	virtual ~SpatialIndex();

protected:
   MapLayer *m_pLayer;
   MapLayer *m_pToLayer;
   char m_filename[ 256 ];     // name of the index file (valid after ReadIndex();

   int   m_allocatedIndexCount; // same as m_indexCount until dynamic map stuff
   SI_METHOD   m_method;
   float m_maxDistance;

   SPATIAL_INFO **m_index;       // array of pointers to the spatial entries
   int   m_indexCount;           // number of elements in the m_index array, as loaded from disk
   int  *m_entryCount;           // array of sizes of the spatial entry arrays
   bool  m_isBuilt;
   int   m_sizeofSPATIAL_INFO;   // sizeof(SPATIAL_INFO)-sizeof(void*) OR sizeof(SPATIAL_INFO)
   int   m_maxCount;             // largest int in m_entryCount

public:
   bool IsBuilt() { return m_isBuilt; }

   bool HasExtendedSpatialInfoEx(LPCTSTR filename, int & errorCode); // 

protected:

   // compute the distance from polygon i to polygon j
   float Distance(int i, int j) { ASSERT(i<m_indexCount && j<m_entryCount[i] && m_sizeofSPATIAL_INFO != -1);
      return ((SPATIAL_INFO*)((char*)m_index[i] + j*m_sizeofSPATIAL_INFO ))->distance; }

   // return the polygon index for the "j"th entry in the spatial index for the "i"th polygon
   int PolyIndex(int i, int j) { ASSERT( i < m_indexCount && j < m_entryCount[i] && m_sizeofSPATIAL_INFO != -1 );
      return ((SPATIAL_INFO*)((char*)m_index[i] + j*m_sizeofSPATIAL_INFO ))->polyIndex; }
   
   void * Ex(int i, int j) { ASSERT(i<m_indexCount && j<m_entryCount[i] && m_sizeofSPATIAL_INFO == sizeof(SPATIAL_INFO));
      return ((SPATIAL_INFO*)((char*)m_index[i] + j*m_sizeofSPATIAL_INFO ))->ex; }
public:

   int  BuildIndex( MapLayer *pLayer, MapLayer *pToLayer, int maxCount, float maxDistance, SI_METHOD method,
      PFNC_SPATIAL_INFO_EX fillEx=NULL);

   float GetMaxDistance() const { return m_maxDistance; }
   int   GetSize() { return m_indexCount; }
   int   GetCount(){ return m_indexCount; }

   int  WriteIndex( LPCTSTR filename );

   // ReadIndex() notes:  1) pToLayer sould be NULL if indexing on a single layer
   //                     2) records < 0 -> all record valid, > 0 -> read only "records" records should be read
   int  ReadIndex( LPCTSTR filename, MapLayer *pLayer, MapLayer *pToLayer, int records );

   int  GetNeighborArraySize( int index ); // { ASSERT( 0 <= index && index < m_indexCount ); return m_entryCount[ index ]; }
   int  GetNeighborArray( int index, int *neighborArray, float *distanceArray, int maxCount, void ** pEx=NULL );
   bool GetNeighborDistance( int thisPolyIndex, int toPolyIndex, float &distance );

   static int GetSpatialIndexInfo( LPCTSTR filename, float &distance,int &count );

   void Clear( void );

////////////////////////
// dynamic map stuff
public:
   void Subdivide( int parentCell );
   void UnSubdivide( int parentCell,  CDWordArray & childIdList );
   //void Union( int childCell );
   //void UnUnion( int childCell );

protected:
   BitArray m_deadList;
   void BuildList( int forCell, int fromCell, const CDWordArray &minusCells, const CDWordArray &unionCells );

// end dynamic map stuff
////////////////////////
};

#endif // !defined(AFX_SPATIALINDEX_H__02496309_1AB3_4128_AAB8_24DD0FBADF15__INCLUDED_)
