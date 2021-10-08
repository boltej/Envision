#pragma once

#ifndef NO_MFC
#include <afxtempl.h>
#endif

#include "RTree.h"

#include "Maplayer.h"

typedef RTree<int, int, 2, float> IndexTree;

class LIBSAPI  RTreeIndex
   {

   friend class MapLayer;

   public:
      RTreeIndex();
      virtual ~RTreeIndex() {}

   protected:
      MapLayer* m_pLayer;
      MapLayer* m_pToLayer;
      char m_filename[256];     // name of the index file (valid after ReadIndex();
      
      IndexTree m_rTree;

      int   m_indexCount;           // number of elements in the m_index array, as loaded from disk
      bool  m_isBuilt;

   public:
      bool IsBuilt() { return m_isBuilt; }

   protected:
      // compute the distance from polygon i to polygon j
      float Distance(int i, int j) {
         ///ASSERT(i < m_indexCount&& j < m_entryCount[i] && m_sizeofSPATIAL_INFO != -1);
         ///return ((SPATIAL_INFO*)((char*)m_index[i] + j * m_sizeofSPATIAL_INFO))->distance;
         return 0;
         }

   public:
      int  BuildIndex(MapLayer* pLayer);
      
      int   GetSize() { return m_indexCount; }
      int   GetCount() { return m_indexCount; }

      int  WriteIndex(LPCTSTR filename);

      // ReadIndex() notes:  1) pToLayer sould be NULL if indexing on a single layer
      //                     2) records < 0 -> all record valid, > 0 -> read only "records" records should be read
      int  ReadIndex(LPCTSTR filename, MapLayer* pLayer, MapLayer* pToLayer, int records);

      //int  GetNeighborArraySize(int index); // { ASSERT( 0 <= index && index < m_indexCount ); return m_entryCount[ index ]; }
      //int  GetNeighborArray(int index, int* neighborArray, float* distanceArray, int maxCount, void** pEx = NULL);
      //bool GetNeighborDistance(int thisPolyIndex, int toPolyIndex, float& distance);
      
      void Clear(void);
   };

