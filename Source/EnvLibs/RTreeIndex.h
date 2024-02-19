#pragma once

#ifndef NO_MFC
#include <afxtempl.h>
#endif

#include "Maplayer.h"
#include "RTree.h"



typedef RTree<int, REAL, 2> IndexTree;

struct IDU_DIST 
    { 
    int idu; 
    float distance; 
    IDU_DIST(int _idu, float _distance) : idu(_idu), distance(_distance) {}
    };

class LIBSAPI  RTreeIndex
   {

   friend class MapLayer;

   public:
      RTreeIndex() {}
      virtual ~RTreeIndex() {}

      static MapLayer* m_pLayer;
      static MapLayer* m_pToLayer;
      static int m_currentIdu;

      static float m_currentDistance;

      // store search results as (idu,distance) pairs
      static std::vector<IDU_DIST> m_searchResults;

   protected:
      char m_filename[256];     // name of the index file (valid after ReadIndex();
      
      IndexTree m_rTree;
      //bgi::rtree<value, bgi::quadratic<16>> m_rTree;

      int   m_indexCount=-1;           // number of elements in the m_index array, as loaded from disk
      bool  m_isBuilt = false;

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

      int WithinDistance(int idu, float distance, std::vector<IDU_DIST>& results);
      int NextTo(int idu, std::vector<IDU_DIST>& results) { return WithinDistance(idu, 0, results);}


      int  WriteIndex(LPCTSTR filename);

      // ReadIndex() notes:  1) pToLayer sould be NULL if indexing on a single layer
      //                     2) records < 0 -> all record valid, > 0 -> read only "records" records should be read
      int  ReadIndex(LPCTSTR filename, MapLayer* pLayer, MapLayer* pToLayer, int records);

      //int  GetNeighborArraySize(int index); // { ASSERT( 0 <= index && index < m_indexCount ); return m_entryCount[ index ]; }
      //int  GetNeighborArray(int index, int* neighborArray, float* distanceArray, int maxCount, void** pEx = NULL);
      //bool GetNeighborDistance(int thisPolyIndex, int toPolyIndex, float& distance);
      
      void Clear(void);
   };

