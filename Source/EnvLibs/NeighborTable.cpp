#include "EnvLibs.h"
#pragma hdrstop


#include "NeighborTable.h"
#include "Report.h"
#include <fstream>
using namespace std;

int NeighborTable::Read(LPCTSTR path)
   {
   int eol = 0xffff;

   ifstream inFile(path, ios::in | ios::binary);

   if (!inFile) {
      CString msg("NeighborTable:  Can't open file ");
      msg += path;
      Report::ErrorMsg(msg);
      return 1;
      }

   while (! inFile.eof() )
      {
      int srcIndex, count, neighbor;

      inFile.read(reinterpret_cast<char*>(&srcIndex), sizeof(int));
      inFile.read(reinterpret_cast<char*>(&count), sizeof(int));
      //inFile >> srcIndex;
      //inFile >> count;

      this->polyIndexes.Add(srcIndex);

      CUIntArray* pNeighbors = new CUIntArray;
      this->neighborsArray.Add(pNeighbors);

      for (int i = 0; i < count; i++)
         {
         inFile.read(reinterpret_cast<char*>(&neighbor), sizeof(int));
         //inFile >> neighbor;
         pNeighbors->Add(neighbor);
         }

      //inFile.read(reinterpret_cast<char*>(&eol), sizeof(int));
      //inFile >> eol;
      }

   return (int)this->polyIndexes.GetSize();
   }


CUIntArray* NeighborTable::GetNeighbors(int polyIndex)
   {
   if (polyIndex >= polyIndexes.GetSize())
      return NULL;

   return this->neighborsArray[polyIndex];
   }

