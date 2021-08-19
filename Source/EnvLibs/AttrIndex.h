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
#pragma once
#include "EnvLibs.h"

#include "Vdata.h"

#ifndef NO_MFC
#include <afxtempl.h>
#endif

typedef CMap<int, int, int, int> IntMap;

class DbTable;


// ATTR_INFO structure stores all the polygon indexes associated with a given attribute value
struct ATTR_INFO
   {
   VData   attribute;
   CUIntArray polyIndexArray;

   ATTR_INFO() : attribute( VT_NULL ) { }
   ATTR_INFO( const VData &v ) : attribute( v ) { } 
   ATTR_INFO( const ATTR_INFO &v ) : attribute( v.attribute ), polyIndexArray()
      { polyIndexArray.Copy( v.polyIndexArray ); } 

   ATTR_INFO& operator=(const ATTR_INFO& v)  { attribute = v.attribute; polyIndexArray.Copy(v.polyIndexArray); return *this; }
   //void operator = ( ATTR_INFO &v ) { attribute = v.attribute; polyIndexArray.Copy( v.polyIndexArray ); }
   bool operator == (const ATTR_INFO& v ) const { return attribute == v.attribute; }
   };


// ATTR_INFO_ARRAY structure stores arrays of attribute/index sets for a given column in the database
struct ATTR_INFO_ARRAY
   {
   int col;                // field column associated with this attrbute index info
   
   CMap< VData, VData&, int, int > valueIndexMap;  // maps attribute values to index into infoItems array
   CArray< ATTR_INFO, ATTR_INFO& > infoItems;      // stores ATTR_INFO for each attribute value in the column
   ATTR_INFO_ARRAY() { }
 /*  ATTR_INFO_ARRAY(const ATTR_INFO_ARRAY &a)  {
      col = a.col;  infoItems.Copy(a.infoItems);
      CMap< VData, VData&, int, int> valueIndexMap;  for (int i = 0; i < a.valueIndexMap.GetSize(); i++) {
         }
      }

   ATTR_INFO_ARRAY& operator =(const ATTR_INFO_ARRAY &a) { col = a.col; infoItems.Copy(a.infoItems); return *this; }*/

   };

//inline

//not sure what this is for; disable for non-mfc
#ifndef NO_MFC
template <> AFX_INLINE UINT AFXAPI HashKey( VData& v )
{
return (DWORD)(((DWORD_PTR)v.val.vInt)>>4);
}
#endif


// this class provide a persistent index to one or more columns in a DbTable.  
// Each columns info is stored in an ATTR_INFO_ARRAY, where the actual indexing occurs.

class LIBSAPI AttrIndex
{
friend class MapLayer;

public:
   AttrIndex(void) : m_isBuilt( false ), m_isChanged( false ), m_pDbTable( NULL ) { }
   AttrIndex(AttrIndex &a) : m_pDbTable(a.m_pDbTable), m_filename(a.m_filename), m_isBuilt(a.m_isBuilt), m_isChanged(a.m_isChanged) { m_attrInfoArray.Copy(a.m_attrInfoArray); IntMap::CPair* pCurVal; pCurVal = a.m_colIndexMap.PGetFirstAssoc(); while (pCurVal != NULL) { m_colIndexMap.SetAt(CPAIR_PTR_KEY(pCurVal), CPAIR_PTR_VALUE(pCurVal)); } };
   ~AttrIndex(void);

protected:
   DbTable  *m_pDbTable;
   CString   m_filename;

   CArray< ATTR_INFO_ARRAY*, ATTR_INFO_ARRAY* >  m_attrInfoArray;
   CMap< int, int, int, int > m_colIndexMap;  // maps column# to m_attrInfoArray index
   
   bool  m_isBuilt;
   bool  m_isChanged;

public:
   bool IsBuilt() { return m_isBuilt; }
   int  BuildIndex( DbTable *pDbTable, int col );
   
   int  WriteIndex( LPCTSTR filename=NULL );
   int  ReadIndex( DbTable *pDbTable, LPCTSTR filename=NULL );

   bool GetIndexArray( int col, VData attr, CUIntArray& );
   int  GetIndexCount() { return (int) m_attrInfoArray.GetSize(); }
   bool ContainsCol( int col ) { int index; return m_colIndexMap.Lookup( col, index ) ? true : false; }

   bool ChangeData( int col, int record, VData oldValue, VData newValue );

   int  GetRecordArray( int col, VData key, CUIntArray &recordArray );
   int  GetRecord( int col, VData key, int startRecord=0 );

   int  Lookup( int col, VData key, float &value );   // need more of these!
   

   void Clear( void );

   void GetDefaultFilename( CString &filename );
   };
