#include "EnvLibs.h"
#include "RTreeIndex.h"


//#include "boost/geometry/geometry.hpp"
//#include "boost/geometry/geometries/polygon.hpp"
//#include "boost/geometry/index/rtree.hpp"
//using namespace boost::geometry;


MapLayer* RTreeIndex::m_pLayer = nullptr;
MapLayer* RTreeIndex::m_pToLayer= nullptr;
int RTreeIndex::m_currentIdu = -1;
float RTreeIndex::m_currentDistance = 0;
std::vector<IDU_DIST> RTreeIndex::m_searchResults;

/*

typedef RTree<ValueType, int, 2, float> MyTree;
MyTree tree;

int i, nhits;
cout << "nrects = " << nrects << "\n";

for (i = 0; i < nrects; i++)
   {
   tree.Insert(rects[i].min, rects[i].max, i); // Note, all values including zero are fine in this version
   }

nhits = tree.Search(search_rect.min, search_rect.max, MySearchCallback);

cout << "Search resulted in " << nhits << " hits\n";

// Iterator test
int itIndex = 0;
MyTree::Iterator it;
for (tree.GetFirst(it);
   !tree.IsNull(it);
   tree.GetNext(it))
   {
   int value = tree.GetAt(it);

   int boundsMin[2] = { 0,0 };
   int boundsMax[2] = { 0,0 };
   it.GetBounds(boundsMin, boundsMax);
   cout << "it[" << itIndex++ << "] " << value << " = (" << boundsMin[0] << "," << boundsMin[1] << "," << boundsMax[0] << "," << boundsMax[1] << ")\n";
   }

// Iterator test, alternate syntax
itIndex = 0;
tree.GetFirst(it);
while (!it.IsNull())
   {
   int value = *it;
   ++it;
   cout << "it[" << itIndex++ << "] " << value << "\n";
   }

// test copy constructor
MyTree copy = tree;

// Iterator test
itIndex = 0;
for (copy.GetFirst(it);
   !copy.IsNull(it);
   copy.GetNext(it))
   {
   int value = copy.GetAt(it);

   int boundsMin[2] = { 0,0 };
   int boundsMax[2] = { 0,0 };
   it.GetBounds(boundsMin, boundsMax);
   cout << "it[" << itIndex++ << "] " << value << " = (" << boundsMin[0] << "," << boundsMin[1] << "," << boundsMax[0] << "," << boundsMax[1] << ")\n";
   }

// Iterator test, alternate syntax
itIndex = 0;
copy.GetFirst(it);
while (!it.IsNull())
   {
   int value = *it;
   ++it;
   cout << "it[" << itIndex++ << "] " << value << "\n";
   }

return 0;

// Output:
//
// nrects = 4
// Hit data rect 1
// Hit data rect 2
// Search resulted in 2 hits
// it[0] 0 = (0,0,2,2)
// it[1] 1 = (5,5,7,7)
// it[2] 2 = (8,5,9,6)
// it[3] 3 = (7,1,9,2)
// it[0] 0
// it[1] 1
// it[2] 2
// it[3] 3
// it[0] 0 = (0,0,2,2)
// it[1] 1 = (5,5,7,7)
// it[2] 2 = (8,5,9,6)
// it[3] 3 = (7,1,9,2)
// it[0] 0
// it[1] 1
// it[2] 2
// it[3] 3
}


*/

int RTreeIndex::BuildIndex(MapLayer* pLayer)
   {
   int polyCount = pLayer->GetPolygonCount();
   // iterate through the maplayer, adding a MBR for each to the index
   for (int i=0; i < polyCount; i++ )
      {
      if (i % 1000 == 0)
         {
         CString msg;
         msg.Format("Building RTree (%i/%i)", i, polyCount);
         Report::StatusMsg(msg);
         }

      Poly* pPoly = pLayer->GetPolygon(i);

      REAL aMins[2], aMaxs[2];
      pPoly->GetBoundingRect(aMins[0], aMins[1], aMaxs[0], aMaxs[1]);

      m_rTree.Insert(aMins, aMaxs, i); // Note, all values including zero are fine in this version
      }
   this->m_pLayer = pLayer;
   this->m_pToLayer = pLayer;
   this->m_isBuilt = true;
   this->m_indexCount = polyCount;
   return polyCount;
   }



bool _WithinDistance(const int& idu)
   {
   Poly* pPoly = RTreeIndex::m_pLayer->GetPolygon(RTreeIndex::m_currentIdu);
   Poly *pToPoly = RTreeIndex::m_pLayer->GetPolygon(idu);
   
   int retFlag = 0;
   REAL d = pPoly->NearestDistanceToPoly(pToPoly, -1, &retFlag);

   if (d < RTreeIndex::m_currentDistance)
      RTreeIndex::m_searchResults.push_back(IDU_DIST(idu, float(d)));

   return true;
   }

bool CompareDist(IDU_DIST a1, IDU_DIST a2)
   {
   return (a1.distance < a2.distance);
   }


int RTreeIndex::WithinDistance(int idu, float distance, std::vector<IDU_DIST> &results )
   {
   REAL aMins[2], aMaxs[2];
   RTreeIndex::m_searchResults.clear();

   Poly* pPoly = this->m_pLayer->GetPolygon(idu);
   pPoly->GetBoundingRect(aMins[0], aMins[1], aMaxs[0], aMaxs[1]);

   aMins[0] -= distance;
   aMins[1] -= distance;
   aMaxs[0] += distance;
   aMaxs[1] += distance;

   RTreeIndex::m_currentIdu = idu;
   RTreeIndex::m_currentDistance = distance;
   RTreeIndex::m_searchResults.clear();

   // note: this call populates the static m_searchResults Array
   m_rTree.Search(aMins, aMaxs, _WithinDistance);

   // sort the results (closest first)
   std::sort(RTreeIndex::m_searchResults.begin(), 
             RTreeIndex::m_searchResults.end(),
             CompareDist);
   
   for (int i = 0; i < RTreeIndex::m_searchResults.size(); i++)
       results.push_back(RTreeIndex::m_searchResults[i]);
   //results = RTreeIndex::m_searchResults;
   return (int) results.size();
   }


