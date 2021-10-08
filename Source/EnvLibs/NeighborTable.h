
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "PtrArray.h"


class LIBSAPI NeighborTable
{
public:
   NeighborTable() {}

   int Read(LPCTSTR path);

   CUIntArray* GetNeighbors(int polyIndex);

protected:
   CUIntArray polyIndexes;

   PtrArray<CUIntArray> neighborsArray;

};

