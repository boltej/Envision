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
// DbTable.h: interface for the DbTable class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "EnvLibs.h"

#include "Report.h"

#ifndef NO_MFC
#include <afxtempl.h>
#endif

#include "DATAOBJ.H"
#include "BitArray.h"
#include "PtrArray.h"
#include "AttrIndex.h"   // for join tables, to enable quick lookup

#ifdef _WINDOWS
//#include "ado2.h"
#endif

#if !(defined _WIN64 || defined NO_MFC) 
#include <afxdao.h>
#endif



#pragma warning( disable : 4995 )   // "CDaoDatabase deprecated"

class DbTable;

struct FIELD_INFO
   {
   TYPE type;  // VData type
   int  width;
   int  decimals;
   bool show;
   bool save;

   FIELD_INFO() : type( TYPE_NULL ), width( 12 ), decimals( 3 ), show( true ), save( true ) { }
   FIELD_INFO( TYPE t ) : type( t ), width( 12 ), decimals( 3 ), show( true ), save( true ) { }
   FIELD_INFO( const FIELD_INFO &fi ) { *this = fi; }
   FIELD_INFO &operator = ( const FIELD_INFO &fi ) { type = fi.type; width = fi.width; decimals = fi.decimals; show = fi.show; save = fi.save; return *this; } 
   };


// JoinInfo defines a joined table relationship.  The basic idea is:
//  1) There is parent (main) table, and a joined child table
//  2) The two tables are joined by a a column in the parent table,
//     whose record values are used to look up corresponding rows in the child table (one-to-one relationship)
//  3) to join the tables, an index is created on the child table/column to allow fast
//     lookups of records based on the value in common between the two tables
//  4) The parent DbTable is essentially "extended"  to include the columns of the child table

class LIBSAPI JoinInfo
{
public:
   DbTable *m_pParentTable;  
   DbTable *m_pChildTable;

   CString m_parentJoinField;
   CString m_childJoinField;
   int m_parentJoinCol;
   int m_childJoinCol;

   AttrIndex m_index;      // index on the child table join col

   int m_childColCount;

   bool m_deleteChildTableOnDelete;

   JoinInfo( void ) : m_pParentTable( NULL ), m_pChildTable( NULL ), m_parentJoinCol( -1 ), 
      m_childJoinCol( -1 ), m_childColCount( -1 ), m_deleteChildTableOnDelete( false ) { }

   ~JoinInfo( void );

   int CreateJoin( DbTable *pParentTable, DbTable *pChildTable, LPCTSTR parentJoinField, LPCTSTR childJoinField, bool saveIndex=true );   
};


void LIBSAPI GetTypeParams( TYPE type, int &width, int &decimals );


class  LIBSAPI  DbTable  
{
public:
   CString m_databaseName;
   CString m_tableName;

   DataObj *m_pData;      // holds the underlying data
   DO_TYPE m_dotType;    // DOT_FLOAT, DOT_VDATA, DOT_INT, etc...(see dataobj.h)

   // methods
 	DbTable( DO_TYPE type=DOT_VDATA);
   DbTable( DbTable &dbTable ) { *this = dbTable; }
   DbTable( DataObj *pData );   // for creating a DbTable from a preexisting VDataObj

   DbTable &operator = (DbTable &dbTable );

   DO_TYPE GetDOType() { return m_dotType; }

	virtual ~DbTable();

   void Clear();

   // load a table from a dataset (open,read,save)
   int Load( LPCTSTR databaseName, LPCTSTR sql, 
      BOOL bExclusive = FALSE, BOOL bReadOnly = FALSE, LPCTSTR lpszConnect = _T("")  );

protected:
#ifndef NO_MFC
#ifdef _WIN64
//   CADODatabase *m_pDatabase;
#else
   CDaoDatabase *m_pDatabase;
#endif
#endif

public:
   int OpenDB( LPCTSTR databaseName,  BOOL bExclusive, BOOL bReadOnly, LPCTSTR lpszConnect );
   int ReadDB( LPCTSTR sql );
   int CloseDB( void );

   int LoadDataDBF( LPCTSTR path );
   int LoadDataCSV( LPCTSTR path );

   int SaveDataDB( LPCTSTR databaseName, LPCTSTR tableName, LPCTSTR connectStr );
   int SaveDataDBF( LPCTSTR path=NULL, BitArray *pSaveArray=NULL );  // DBase only

   int GetRecordCount() { ASSERT( m_pData != NULL );  return m_pData->GetRowCount(); }

   // data manipulation
   void SetData( int rec, int col, int value );
   void SetData( int rec, int col, float value );
   void SetData( int rec, int col, double value );
   void SetData( int rec, int col, COleVariant &value );
   void SetData( int rec, int col, VData &value );

   void SetColData( int rec, VData &value );

   bool GetData( int rec, int col, float &value );
   bool GetData( int rec, int col, double &value );
   bool GetData( int rec, int col, COleVariant &value );
   bool GetData( int rec, int col, CString &value );
   bool GetData( int rec, int col, bool &value );
   bool GetData( int rec, int col, int &value );
   bool GetData( int rec, int col, VData &value );

   int FindData( int col, VData &value, int startRecord=-1 )    // find the first record that matches the passed in VData;
      { if ( m_pData ) return m_pData->Find( col, value, startRecord ); else return -1; }

   // field (column) operations
   int  GetColCount( bool includeJoins=true );
   void ShowField( int col, bool show=true );
   int  GetFieldCol( LPCTSTR label );
   
   LPCTSTR GetColLabel( int col ) { return (LPCTSTR) m_pData->GetLabel( col ); }
   int     SetColLabel( int col, LPCTSTR label ) { return m_pData->SetLabel( col, label ); }
   
   bool IsFieldVisible( int col );
   void SetFieldType( int col, TYPE type, bool convertData );
   void SetFieldType( int col, TYPE type, int width, int decimals, bool convertData );
   TYPE GetFieldType ( int col );
#ifndef NO_MFC
   void SetField ( int col, LPCTSTR label, COleVariant &v, bool show );
#endif
   void SetField ( int col, LPCTSTR label, TYPE type, int width, int decimals, bool show );
   void SetField ( int col, LPCTSTR label, FIELD_INFO& );
   void SetFieldInfoSize( int count ) { m_fieldInfoArray.SetSize( INT_PTR(count) ); }
   bool RefreshFieldInfo( void );
   FIELD_INFO &GetFieldInfo( int i ) { return m_fieldInfoArray[ i ]; }
   bool SwapCols( int col0, int col1 );
   bool CopyCol( int toCol, int fromCol ) { return m_pData->CopyCol( toCol, fromCol ); }

   int  AddFields( int count );   // uninitialized

   int  AddField( LPCTSTR name, TYPE type, bool addCol=true );
   int  AddField( LPCTSTR name, TYPE type, int width, int decimals, bool addCol=true );
   int  InsertField( int insertBefore, LPCTSTR name, TYPE type );
   int  InsertField( int insertBefore, LPCTSTR name, TYPE type, int width, int decimals );
   int  RemoveField( LPCTSTR name );
   int  RemoveField( int col );
   bool RemoveAllFields();

   int  AddJoinTable( DbTable *pJoinTable, LPCTSTR parentJoinCol, LPCTSTR childJoinCol, bool deleteChildTable );

protected:

   CArray< FIELD_INFO, FIELD_INFO& > m_fieldInfoArray;

   // Join information
   PtrArray< JoinInfo > m_joinTables;

   JoinInfo *GetJoinInfoFromCol( int col, int &childCol );
   JoinInfo *GetJoinedDataInfo( int rec, int col, int &childCol, int &childRecord );

   bool GetJoinedData( int rec, int col, float &value );
   bool GetJoinedData( int rec, int col, double &value );
   bool GetJoinedData( int rec, int col, COleVariant &value );
   bool GetJoinedData( int rec, int col, CString &value );
   bool GetJoinedData( int rec, int col, bool &value );
   bool GetJoinedData( int rec, int col, int &value );
   bool GetJoinedData( int rec, int col, VData &value );

#ifndef NO_MFC
#ifndef _WIN64
   int  LoadRecords( CDaoDatabase &db, LPCTSTR sql );
   bool CreateTable( CDaoDatabase &database, LPCTSTR tableName, LPCTSTR connectStr );
   int  SaveRecords( CDaoDatabase &database, LPCTSTR tableName );
#else
//   int  LoadRecords( CADODatabase &db, LPCTSTR sql );
//   bool CreateTable( CADODatabase &database, LPCTSTR tableName, LPCTSTR connectStr );
//   int  SaveRecords( CADODatabase &database, LPCTSTR tableName );
#endif
#endif
};


/* ---------------- SetData() methods ----------*/

inline
void DbTable::SetData( int rec, int col, int value )
   {
   ASSERT( m_pData != NULL );
   m_pData->Set( col, rec, value );   
   }

inline
void DbTable::SetData( int rec, int col, float value )
   {
   ASSERT( m_pData != NULL );
   m_pData->Set( col, rec, value );   
   }

inline
void DbTable::SetData( int rec, int col, double value )
   {
   ASSERT( m_pData != NULL );
   m_pData->Set( col, rec, value );   
   }

#ifndef NO_MFC
inline
void DbTable::SetData( int rec, int col, COleVariant &value )
   {
   ASSERT( m_pData != NULL );
   m_pData->Set( col, rec, value );
   }
#endif

inline
void DbTable::SetData( int rec, int col, VData &value )
   {
   ASSERT( m_pData != NULL );
   m_pData->Set( col, rec, value );
   }


inline
void DbTable::SetColData( int col, VData &value )
   {
   int rows = GetRecordCount();
   int cols = GetColCount();

   if ( col >= cols )
      return;

   for ( int row=0; row < rows; row++ )
       SetData( row, col, value );
   }




/* ---------------- GetData() methods ----------*/
  
inline
bool DbTable::GetData( int rec, int col, float &value )
   {
   ASSERT( m_pData != NULL );
   
   if ( col >= m_pData->GetColCount() )
       return GetJoinedData( rec, col, value );

   return m_pData->Get( col, rec, value );
   }

  
inline
bool DbTable::GetData( int rec, int col, double &value )
   {
   ASSERT( m_pData != NULL );
   
   if ( col >= m_pData->GetColCount() )
       return GetJoinedData( rec, col, value );

   return m_pData->Get( col, rec, value );
   }


inline
bool DbTable::GetData( int rec, int col, VData &value )
   {
   ASSERT( m_pData != NULL );
   
   if ( col >= m_pData->GetColCount() )
       return GetJoinedData( rec, col, value );

   return m_pData->Get( col, rec, value );
   }

#ifndef NO_MFC
inline
bool DbTable::GetData( int rec, int col, COleVariant &value )
   {
   ASSERT( m_pData != NULL );
   
   if ( col >= m_pData->GetColCount() )
       return GetJoinedData( rec, col, value );

   return m_pData->Get( col, rec, value );
   }
#endif

inline
bool DbTable::GetData( int rec, int col, CString &value )
   {
   //if ( col > m_pData->GetColCount()-1 )
   //   {
   //   CString msg;
   //   msg.Format( "Col out of bounds in DBTable::GetData() (%i of %i)", col, (int) m_pData->GetColCount() );
   //   Report::LogError( msg );
   //   }
   
   ASSERT( m_pData != NULL );
   
   if ( col >= m_pData->GetColCount() )
       return GetJoinedData( rec, col, value );

   value = m_pData->GetAsString( col, rec );
   return true;
   }

inline
bool DbTable::GetData( int rec, int col, bool &value )
   {
   ASSERT( m_pData != NULL );
   
   if ( col >= m_pData->GetColCount() )
       return GetJoinedData( rec, col, value );

   return m_pData->Get( col, rec, value );
   }


inline
bool DbTable::GetData( int rec, int col, int &value )
   {
   ASSERT( m_pData != NULL );
   
   if ( col >= m_pData->GetColCount() )
       return GetJoinedData( rec, col, value );

   return m_pData->Get( col, rec, value );
   }


inline
bool DbTable::GetJoinedData( int rec, int col, float &value )
   {
   int childCol=-1, childRecord=-1;
   JoinInfo *pInfo = GetJoinedDataInfo( rec, col, childCol, childRecord );
   return pInfo->m_pChildTable->GetData( childRecord, childCol, value );
   }


inline
bool DbTable::GetJoinedData( int rec, int col, double &value )
   {
   int childCol=-1, childRecord=-1;
   JoinInfo *pInfo = GetJoinedDataInfo( rec, col, childCol, childRecord );
   return pInfo->m_pChildTable->GetData( childRecord, childCol, value );
   }

#ifndef NO_MFC
inline
bool DbTable::GetJoinedData( int rec, int col, COleVariant &value )
   {
   int childCol=-1, childRecord=-1;
   JoinInfo *pInfo = GetJoinedDataInfo( rec, col, childCol, childRecord );
   return pInfo->m_pChildTable->GetData( childRecord, childCol, value );
   }
#endif

inline
bool DbTable::GetJoinedData( int rec, int col, CString &value )
   {
   int childCol=-1, childRecord=-1;
   JoinInfo *pInfo = GetJoinedDataInfo( rec, col, childCol, childRecord );
   return pInfo->m_pChildTable->GetData( childRecord, childCol, value );
   }


inline
bool DbTable::GetJoinedData( int rec, int col, bool &value )
   {
   int childCol=-1, childRecord=-1;
   JoinInfo *pInfo = GetJoinedDataInfo( rec, col, childCol, childRecord );
   return pInfo->m_pChildTable->GetData( childRecord, childCol, value );
   }


inline
bool DbTable::GetJoinedData( int rec, int col, int &value )
   {
   int childCol=-1, childRecord=-1;
   JoinInfo *pInfo = GetJoinedDataInfo( rec, col, childCol, childRecord );
   return pInfo->m_pChildTable->GetData( childRecord, childCol, value );
   }


inline
bool DbTable::GetJoinedData( int rec, int col, VData &value )
   {
   int childCol=-1, childRecord=-1;
   JoinInfo *pInfo = GetJoinedDataInfo( rec, col, childCol, childRecord );
   return pInfo->m_pChildTable->GetData( childRecord, childCol, value );
   }


inline
JoinInfo *DbTable::GetJoinedDataInfo( int rec, int col, int &childCol, int &childRecord )
   {
   childCol = childRecord = -1;
   JoinInfo *pInfo = GetJoinInfoFromCol( col, childCol );

   if ( pInfo == NULL )
      return NULL;
   
   // get lookup value from parent table
   VData key;
   if ( m_pData->Get( col, rec, key ) == false )
      return NULL;

   // look up value in child table by finding first joined record and extracting the value
   childRecord = pInfo->m_index.GetRecord( pInfo->m_childJoinCol, key );
   
   if ( childRecord < 0 )
      return NULL;

   return pInfo;
   }


/// more overloads to come




/* ------------------ Other inline methods -------------------*/

inline
JoinInfo *DbTable::GetJoinInfoFromCol( int col, int &childCol )
   {
   col -= m_pData->GetColCount();
   if ( col < 0 )
      return NULL;

   int joinCount = (int) m_joinTables.GetSize();
   JoinInfo *pInfo = NULL;
   int jiIndex = -1;
 
   while ( col >= 0 )
      {
      jiIndex++;
      if ( jiIndex >= (int) m_joinTables.GetSize() )
         break;

      pInfo = m_joinTables[ jiIndex ];

      if ( col < pInfo->m_childColCount )      // are wi in the right table?
         {
         childCol = col;
         return pInfo;
         }
      else
         col -= pInfo->m_childColCount;
      }

   childCol = -1;
   return NULL;
   }


inline
void DbTable::ShowField( int col, bool show )
   {
   ASSERT( col < (int) m_fieldInfoArray.GetSize() );
   m_fieldInfoArray[ col ].show = show;
   }
   
inline
int DbTable::GetFieldCol( LPCTSTR label )
   {
   ASSERT ( m_pData != NULL );
   return m_pData->GetCol( label );
   }

inline
bool DbTable::IsFieldVisible( int col )
   {
   ASSERT( col < (int) m_fieldInfoArray.GetSize() );
   return m_fieldInfoArray[ col ].show;
   }

inline
TYPE DbTable::GetFieldType ( int col )
   {
   ASSERT( m_pData != NULL );
   ASSERT( col < (int) m_fieldInfoArray.GetSize() );
   return m_fieldInfoArray[ col ].type;
   }
   

inline
void DbTable::SetField( int col, LPCTSTR label, FIELD_INFO &fi )
   {
   SetField ( col, label, fi.type, fi.width, fi.decimals, fi.show );
   }

#ifndef NO_MFC
inline
void DbTable::SetField( int col, LPCTSTR label, COleVariant &v, bool show )
   {
   m_pData->SetLabel( col, label );
   m_fieldInfoArray[ col ].type = VData::MapOleType( v.vt );
   m_fieldInfoArray[ col ].show = show;

   for ( int i=0; i < m_pData->GetRowCount(); i++ )
      SetData( i, col, v );
   }
#endif

inline
void DbTable::SetField ( int col, LPCTSTR label, TYPE type, int width, int decimals, bool show )
   {
   if ( width <= 0 )
      width = 10;

   m_pData->SetLabel( col, label );
   m_fieldInfoArray[ col ].type     = type;
   m_fieldInfoArray[ col ].width    = width;
   m_fieldInfoArray[ col ].decimals = decimals;
   m_fieldInfoArray[ col ].show     = show;

#ifndef NO_MFC
   COleVariant v;
   for ( int i=0; i < m_pData->GetRowCount(); i++ )
      SetData( i, col, v );
   }
#else
 for ( int i=0; i < m_pData->GetRowCount(); i++ )
      SetData( i, col, 0 );
   }
#endif

inline
int DbTable::GetColCount( bool includeJoins /*=true*/ )
   {
   if ( m_pData == NULL )
      return 0;

   if ( includeJoins == false )
      return m_pData->GetColCount();
   else
      {
      int count = m_pData->GetColCount();

      for ( int i=0; i < (int) m_joinTables.GetSize(); i++ )
         count += m_joinTables[ i ]->m_pChildTable->GetColCount();

      return count;
      }
   }

