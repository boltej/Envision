#include "EnvLibs.h"

#include "IndexedDataObj.h"
#include "FDataObj.H"
#include "IDataObj.H"
#include "Vdataobj.h"

IndexedDataObj::IndexedDataObj() 
   : m_pDataObj( NULL )
   , m_indexCol( 0 )
   { }


IndexedDataObj::~IndexedDataObj()
   {
   if ( m_pDataObj != NULL )
      delete m_pDataObj;
   }

int IndexedDataObj::Create(LPCTSTR path, TYPE type, int indexCol/*=0*/)
   {
   // create data obj
   switch (type)
      {
      case TYPE_INT:
         m_pDataObj = new IDataObj(U_UNDEFINED);
         break;

      case TYPE_FLOAT:
         m_pDataObj = new FDataObj(U_UNDEFINED);
         break;

      case TYPE_VDATA:
         m_pDataObj = new VDataObj(U_UNDEFINED);
         break;

      default:
         ASSERT(0);
         return -1;
      }

   if ( m_pDataObj == NULL )
      return -2;

   m_indexCol = indexCol;

   int rows = m_pDataObj->ReadAscii( path, ',' );

   for( int row=0; row < rows; row++ )
      {
      int id = m_pDataObj->GetAsInt( m_indexCol, row );
      m_indexMap.SetAt( id, row );
      }
   return rows;   
   }

int IndexedDataObj::Lookup(int id, int col, int &value)
   {
   if (m_pDataObj == NULL)
      return -1;

   int row = -1;
   BOOL found = m_indexMap.Lookup(id, row);

   if (found)
      {
      value = m_pDataObj->GetAsInt(col, row);
      return row;
      }
   else
      return -2;
   }

int IndexedDataObj::Lookup(int id, int col, bool &value)
   {
   if (m_pDataObj == NULL)
      return -1;

   int row = -1;
   BOOL found = m_indexMap.Lookup(id, row);

   if (found)
      {
      value = m_pDataObj->GetAsBool(col, row);
      return row;
      }
   else
      return -2;
   }

int IndexedDataObj::Lookup(int id, int col, float &value)
   {
   if (m_pDataObj == NULL)
      return -1;

   int row = -1;
   BOOL found = m_indexMap.Lookup(id, row);

   if (found)
      {
      value = m_pDataObj->GetAsFloat(col, row);
      return row;
      }
   else
      return -2;
   }

int IndexedDataObj::Lookup(int id, int col, CString &value)
   {
   if (m_pDataObj == NULL)
      return -1;

   int row = -1;
   BOOL found = m_indexMap.Lookup(id, row);

   if (found)
      {
      value = m_pDataObj->GetAsString( col, row);
      return row;
      }
   else
      return -2;
   }

int IndexedDataObj::Lookup(int id, int col, std::string &value)
   {
   if (m_pDataObj == NULL)
      return -1;

   int row = -1;
   BOOL found = m_indexMap.Lookup(id, row);

   if (found)
      {
      CString _value = m_pDataObj->GetAsString(col, row);
      value = _value;
      return row;
      }
   else
      return -2;
   }
