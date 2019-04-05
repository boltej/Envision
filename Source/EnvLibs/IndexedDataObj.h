#pragma once

#include <Typedefs.h>
#include <string>

#include <DATAOBJ.H>


class LIBSAPI IndexedDataObj
   {
   public:
      IndexedDataObj();
      ~IndexedDataObj();

      int Create( LPCTSTR path, TYPE type, int indexCol = 0);

      int GetRow( int id ) { m_currentRow=-1; m_indexMap.Lookup( id, m_currentRow ); return m_currentRow; }

      int Lookup(int id, int col, int &value);
      int Lookup(int id, int col, float &value);
      int Lookup(int id, int col, bool &value);
      int Lookup(int id, int col, CString &value);
      int Lookup(int id, int col, std::string &value);

      DataObj *m_pDataObj;

   protected:
      int m_indexCol;
      int m_currentRow;
      CMap< int, int, int, int> m_indexMap;  // key=ID, value=row  
   };
   