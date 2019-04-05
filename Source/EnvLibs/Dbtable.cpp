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
// DbTable.cpp: implementation of the DbTable class.
//
//////////////////////////////////////////////////////////////////////

#include "EnvLibs.h"

#include "DBTABLE.H"

#include "Vdataobj.h"
#include "FDATAOBJ.H"
#include "IDATAOBJ.H"

//#ifdef _WINDOWS
//#include "ado2.h"
//#endif

#define USE_SHAPELIB



#ifdef USE_SHAPELIB
#include "shapefil.h"
#endif


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



void GetTypeParams( TYPE type, int &width, int &decimals )
   {
   switch( type )
      {
      case TYPE_STRING:
      case TYPE_DSTRING:
      case TYPE_CSTRING:
         width = 32;
         decimals = 0;
         break;

      case TYPE_CHAR:
      case TYPE_BOOL:
         width = 1;
         decimals = 0;
         break;

      case TYPE_UINT:
      case TYPE_INT:
      case TYPE_ULONG:
      case TYPE_LONG:   
         width = 8;
         decimals = 0;
         break;

      case TYPE_SHORT:
         width=8;
         decimals=0;
         break;

      case TYPE_FLOAT:  
         width = 12;
         decimals = 6;
         break;

      case TYPE_DOUBLE: 
         width = 19;
         decimals = 11;
         break;
 
      default:  ASSERT( 0 );
      }
   }


JoinInfo::~JoinInfo( void ) 
   { 
   if ( m_deleteChildTableOnDelete && m_pChildTable ) 
      delete m_pChildTable; 
   }


int JoinInfo::CreateJoin( DbTable *pParentTable, DbTable *pChildTable, LPCTSTR parentJoinField, LPCTSTR childJoinField, bool saveIndex )
   {
   m_pParentTable = pParentTable;
   m_pChildTable  = pChildTable;
   m_parentJoinField = parentJoinField;
   m_childJoinField = childJoinField;

   m_parentJoinCol = pParentTable->GetFieldCol( parentJoinField );
   m_childJoinCol = pChildTable->GetFieldCol( childJoinField );

   if ( m_parentJoinCol < 0 )
      return -2;

   if ( m_childJoinCol < 0 )
      return -3;

   // have both tables; next build an index to enable fast lookups of the joined table values
   // make an index on the child table (since this is the one will be lookup up values in) 
   int indexCount = m_index.ReadIndex( m_pChildTable );

   // does it contain the specified column
   if ( m_index.ContainsCol( m_childJoinCol ) == false )
      {
      m_index.BuildIndex( m_pChildTable, m_childJoinCol );
      if ( saveIndex )
         m_index.WriteIndex();
      }

   m_childColCount = m_pChildTable->GetColCount();

   return m_childColCount;
   }



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DbTable::DbTable( DO_TYPE type )
:   m_pData( NULL )
  , m_dotType( type )
#ifndef NO_MFC
//  , m_pDatabase( NULL )
#endif
   {  }

/*
DbTable::DbTable( DbTable &table )
:  m_databaseName( table.m_databaseName ),
   m_tableName( table.m_tableName ),
   m_dotType( table.m_dotType )
   {
   for ( int i=0; i < table.m_fieldInfoArray.GetSize(); i++ )
      {
      FIELD_INFO info;
      info.type = table.m_fieldInfoArray[ i ].type;
      info.show = table.m_fieldInfoArray[ i ].show;
   
      m_fieldInfoArray.Add( info);
      }

   switch( m_dotType )
      {
      case DOT_INT:
         m_pData = new IDataObj( *((IDataObj*)table.m_pData) );
         break;

      case DOT_FLOAT:
         m_pData = new FDataObj( *((FDataObj*)table.m_pData) );
         break;

      case DOT_VDATA:
         m_pData = new VDataObj( *((VDataObj*)table.m_pData) );
         break;

      default:
         ASSERT( 0 );
      }
   }
  */


DbTable &DbTable::operator = ( DbTable &table )
   {
   m_databaseName = table.m_databaseName;
   m_tableName = table.m_tableName;
   m_dotType = table.m_dotType;

   for ( int i=0; i < table.m_fieldInfoArray.GetSize(); i++ )
      {
      FIELD_INFO info;
      info.type = table.m_fieldInfoArray[ i ].type;
      info.show = table.m_fieldInfoArray[ i ].show;
   
      m_fieldInfoArray.Add( info);
      }

   switch( m_dotType )
      {
      case DOT_INT:
         m_pData = new IDataObj( *((IDataObj*)table.m_pData) );
         break;

      case DOT_FLOAT:
         m_pData = new FDataObj( *((FDataObj*)table.m_pData) );
         break;

      case DOT_VDATA:
         m_pData = new VDataObj( *((VDataObj*)table.m_pData) );
         break;

      default:
         ASSERT( 0 );
      }

   return *this;
   }
   

DbTable::DbTable( DataObj *pData )  // note: doesn't copy data, just points to original
: m_pData( pData )
#ifndef NO_MFC
//, m_pDatabase( NULL )
#endif
   {
   // update field
   m_fieldInfoArray.RemoveAll();
   int fieldCount = pData->GetColCount();

   m_fieldInfoArray.SetSize( fieldCount );

   for ( int i=0; i < fieldCount; i++ )
      {
      m_fieldInfoArray[ i ].show = true;
      if ( pData->GetRowCount() > 0 )
         m_fieldInfoArray[ i ].type = pData->GetType( i );  // fails if no rows are present...
      else
         m_fieldInfoArray[ i ].type = TYPE_NULL;
      }

   m_dotType = pData->GetDOType();
   }


DbTable::~DbTable()
   {
   Clear();
   }


void DbTable::Clear()
   {
   if ( m_pData != NULL )
      {
      delete m_pData;
      m_pData = NULL;
      }

   m_databaseName.Empty();
   m_tableName.Empty();
   m_dotType = DOT_NULL;

   m_fieldInfoArray.RemoveAll();
   }

#ifndef NO_MFC
#ifndef _WIN64

int DbTable::Load( LPCTSTR databaseName, LPCTSTR sql, BOOL bExclusive, BOOL bReadOnly, LPCTSTR lpszConnect )
   {
   int recordCount = 0;
   m_databaseName = databaseName;

   // get table name from the sql statement (try a few permutations of "From" )
   char *p = strstr( (char*) sql, "from" );
   if ( p == NULL )
      {
      p = strstr( (char*) sql, "FROM" );
      if ( p == NULL )
         p = strstr( (char*) sql, "From" );
      }

   if ( p == NULL )
      {
      CString msg("Unable to find table name in SQL statement: " );
      msg += sql;
      Report::WarningMsg( msg );
      }
   else
      {
      p = strchr( p, ' ' );   // find the next blank
      p++;
      char table[ 64 ];
      int i = 0;
      while ( *p != ' ' && *p != NULL )
         {
         table[ i++ ] = *p;
         p++;
         }

      table[ i ] = NULL;
      m_tableName = table;
      }

   CDaoDatabase database;

	try
		{
		// open the database
		database.Open( databaseName, bExclusive, bReadOnly, lpszConnect );

      // load up the records
      recordCount = LoadRecords( database, sql );
      ASSERT( recordCount >= 0 );
      }

	catch( CDaoException* )
		{
		CString msg("Error opening database " );
      msg += databaseName;
		Report::WarningMsg( msg, "Exception" );
      return -1;
		}

   database.Close();
   
   return recordCount;
   }
   

int DbTable::LoadRecords( CDaoDatabase &database, LPCTSTR sql )
   {
	CDaoRecordset rs( &database );

   if ( m_pData )
      delete m_pData;

   m_pData = new VDataObj;

   m_fieldInfoArray.RemoveAll();
      
	try
		{
		rs.Open( dbOpenSnapshot, sql);
		//rs.MoveFirst();    // move to first record (NOTE (jpb): Not needed and fails when no record exist!)

      // figure out what the table structure of the query results are.
      // For each float field, create a corresponding data object column
      int fieldCount = rs.GetFieldCount( );

      m_pData->SetSize( fieldCount, 0 );  // make a data column for each field
      m_fieldInfoArray.SetSize( fieldCount );
      
      // set the labels --//
      for ( int i=0; i < fieldCount; i++ )
         {
         CDaoFieldInfo info;
         rs.GetFieldInfo( i, info );

         m_pData->AddLabel( (LPCTSTR) info.m_strName );
         
         m_fieldInfoArray[ i ].show = true;

         switch ( info.m_nType )
            {
            case dbBoolean:  m_fieldInfoArray[ i ].type = TYPE_BOOL;    break;
            case dbByte:     m_fieldInfoArray[ i ].type = TYPE_BOOL;    break;
            case dbLong:     m_fieldInfoArray[ i ].type = TYPE_LONG;    break;
            case dbInteger:  m_fieldInfoArray[ i ].type = TYPE_INT;     break;
            case dbSingle:   m_fieldInfoArray[ i ].type = TYPE_FLOAT;   break;
            case dbDouble:   m_fieldInfoArray[ i ].type = TYPE_DOUBLE;  break;
            case dbText:     m_fieldInfoArray[ i ].type = TYPE_DSTRING; break;
            case dbMemo:     m_fieldInfoArray[ i ].type = TYPE_DSTRING; break;
            case dbDate:     m_fieldInfoArray[ i ].type = TYPE_DATE;    break;   //Date/Time; see MFC class COleDateTime

            //dbLongBinary   Long Binary (OLE Object); you might want to use MFC class CByteArray instead of class CLongBinary as CByteArray is richer and easier to use.
            //dbMemo   Memo; see MFC class CString
            default:
               {
               CString msg( _T("Unsupported type in DbTable::LoadRecords() when reading field ") );
               msg += info.m_strName;
               Report::WarningMsg( msg );
               }
            }
         }

      // have basic structure, now populate fields
      COleVariant *values = new COleVariant[ fieldCount ];
      int count = 0;

		while ( rs.IsEOF() == FALSE )
			{
         for ( int i=0; i < fieldCount; i++ )
            {
			   try 
               {
               COleVariant val;
               rs.GetFieldValue( i, val );
               values[ i ] = val;
               //rs.GetFieldValue( i, array[ i ] );
   				}

	   		catch( CDaoException* )
		   		{
			   	CString msg("DAO Exception getting field value." );
				   Report::ErrorMsg( msg, "Exception" );
               return false;
				   }
			   catch ( CMemoryException* )
				   {
				   CString msg("DAO Memory Exception getting field value." );
				   Report::ErrorMsg( msg, "Exception" );
               return false;
				   }
            }  // end of for:

			m_pData->AppendRow( values, fieldCount );
			rs.MoveNext();
         count++;
			}  // end of: while( rs.IsEOF() )

      delete [] values;
		rs.Close();            
		}  // end of: try ( rsTables.Open )

	catch( CDaoException *e )
		{
		CString msg("Database Error opening table: SQL=" );
        msg += sql;
		msg += ":  ";
		msg += e->m_pErrorInfo->m_strDescription;
		Report::ErrorMsg( msg, "Exception" );
      return -1;
		}
	catch( CMemoryException* )
		{
		CString msg("Memory Error opening table: SQL=" );
      msg += sql;
		Report::ErrorMsg( msg, "Exception" );
      return -2;
		}

   return m_pData->GetRowCount();
   }



int DbTable::OpenDB( LPCTSTR databaseName, BOOL bExclusive, BOOL bReadOnly, LPCTSTR lpszConnect )
   {
   int recordCount = 0;
   m_databaseName = databaseName;

   if ( m_pDatabase != NULL )
      delete m_pDatabase;
    
   m_pDatabase = new CDaoDatabase;   // required for x64 apps
   
	try
		{
		// open the database
		m_pDatabase->Open( databaseName, bExclusive, bReadOnly, lpszConnect );
      }

	catch( CDaoException* )
		{
		CString msg("Error opening database " );
      msg += databaseName;
		Report::WarningMsg( msg, "Exception" );
      return -1;
		}
   
   return 1;
   }
   

int DbTable::ReadDB( LPCTSTR sql )
   {
   if ( m_pDatabase == NULL || m_pDatabase->IsOpen() == FALSE )
      return -1;

   // get table name from the sql statement (try a few permutations of "From" )
   char *p = strstr( (char*) sql, "from" );
   if ( p == NULL )
      {
      p = strstr( (char*) sql, "FROM" );
      if ( p == NULL )
         p = strstr( (char*) sql, "From" );
      }

   if ( p == NULL )
      {
      CString msg("Unable to find table name in SQL statement: " );
      msg += sql;
      Report::WarningMsg( msg );
      }
   else
      {
      p = strchr( p, ' ' );   // find the next blank
      p++;
      char table[ 64 ];
      int i = 0;
      while ( *p != ' ' && *p != NULL )
         {
         table[ i++ ] = *p;
         p++;
         }

      table[ i ] = NULL;
      m_tableName = table;
      }
   
   // load up the records
   int recordCount = LoadRecords( *m_pDatabase, sql );

   return recordCount;
   }


int DbTable::CloseDB( void )
   {
   if ( m_pDatabase == NULL )
      return -1;

   if ( m_pDatabase->IsOpen() == false )
      return 0;

   m_pDatabase->Close();

   delete m_pDatabase;
   m_pDatabase = NULL;

   return 1;
   }



#else   // _WIN64 is defined

///// int DbTable::Load( LPCTSTR databaseName, LPCTSTR sql, BOOL bExclusive, BOOL bReadOnly, LPCTSTR lpszConnect )
/////    {
//////   int recordCount = 0;
//////   m_databaseName = databaseName;
//////
//////   // get table name from the sql statement (try a few permutations of "From" )
//////   char *p = strstr( (char*) sql, "from" );
//////   if ( p == NULL )
//////      {
//////      p = strstr( (char*) sql, "FROM" );
//////      if ( p == NULL )
//////         p = strstr( (char*) sql, "From" );
//////      }
//////
//////   if ( p == NULL )
//////      {
//////      CString msg("Unable to find table name in SQL statement: " );
//////      msg += sql;
//////      Report::WarningMsg( msg );
//////      }
//////   else
//////      {
//////      p = strchr( p, ' ' );   // find the next blank
//////      p++;
//////      char table[ 64 ];
//////      int i = 0;
//////      while ( *p != ' ' && *p != NULL )
//////         {
//////         table[ i++ ] = *p;
//////         p++;
//////         }
//////
//////      table[ i ] = NULL;
//////      m_tableName = table;
//////      }
//////
//////   CADODatabase database;   // required for x64 apps
//////
//////   bool useWideChar = VData::GetUseWideChar();
//////   VData::SetUseWideChar( true );
//////
//////   CString strConnection = _T("");
//////   LPCTSTR ext = _tcsrchr( databaseName, '.' );
//////   if ( ext != NULL )
//////      {
//////      if ( lstrcmpi( ext, _T(".mdb") ) == 0 )
//////         {
//////         strConnection = _T("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=");
//////         strConnection += databaseName;
//////         //CString msg( _T("Error will trying to load database ") );
//////         //msg += databaseName;
//////         //msg += _T(": Access .mdb files are not supported on x64 platform" );
//////         //Report::ErrorMsg( msg );
//////         }
//////      else if ( lstrcmpi( ext, _T(".accdb") ) == 0 )
//////         {
//////         strConnection = _T("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=");
//////         strConnection += databaseName;
//////         strConnection += _T(";Persist Security Info=False;" );
//////         }
//////      }   
//////   
//////	try
//////		{
//////		// open the database
//////      database.SetConnectionString( strConnection );
//////      database.SetConnectionMode(CADODatabase::connectModeRead );
//////
//////		database.Open(); //( databaseName, bExclusive, bReadOnly, lpszConnect );
//////
//////      // load up the records
//////      recordCount = LoadRecords( database, sql );
//////      ASSERT( recordCount >= 0 );
//////      }
//////
//////	catch( CADOException* )
//////		{
//////		CString msg("Error opening database " );
//////      msg += databaseName;
//////		Report::WarningMsg( msg, "Exception" );
//////      VData::SetUseWideChar( useWideChar );
//////      return -1;
//////		}
//////
//////   VData::SetUseWideChar( useWideChar );
//////   database.Close();
//////   
//////   return recordCount;
//////   }
//////   
//////
//////int DbTable::LoadRecords( CADODatabase &database, LPCTSTR sql )
//////   {
//////	CADORecordset rs( &database );
//////
//////   if ( m_pData )
//////      delete m_pData;
//////
//////   m_pData = new VDataObj;
//////
//////   m_fieldInfoArray.RemoveAll();
//////      
//////	try
//////		{
//////		rs.Open( sql );  //( dbOpenSnapshot, sql);
//////		//rs.MoveFirst();    // move to first record (NOTE (jpb): Not needed and fails when no record exist!)
//////
//////      // figure out what the table structure of the query results are.
//////      // For each float field, create a corresponding data object column
//////      int fieldCount = rs.GetFieldCount( );
//////
//////      m_pData->SetSize( fieldCount, 0 );  // make a data column for each field
//////      m_fieldInfoArray.SetSize( fieldCount );
//////      
//////      // set the labels --//
//////      for ( int i=0; i < fieldCount; i++ )
//////         {
//////         CADOFieldInfo info;
//////         rs.GetFieldInfo(i, &info );
//////
//////         m_pData->AddLabel( (LPCTSTR) info.m_strName );
//////         
//////         m_fieldInfoArray[ i ].show = true;
//////
//////         switch ( info.m_nType )
//////            {
//////            case CADORecordset::typeBoolean:
//////            case CADORecordset::typeUnsignedTinyInt:
//////            case CADORecordset::typeTinyInt:
//////               m_fieldInfoArray[ i ].type = TYPE_BOOL;    
//////               break;
//////
//////            case CADORecordset::typeBigInt:
//////               m_fieldInfoArray[ i ].type = TYPE_LONG;
//////               break;
//////
//////            case CADORecordset::typeUnsignedBigInt: 
//////               m_fieldInfoArray[ i ].type = TYPE_ULONG;
//////               break;
//////
//////            case CADORecordset::typeSmallInt:
//////            case CADORecordset::typeUnsignedSmallInt:
//////               m_fieldInfoArray[ i ].type = TYPE_SHORT;
//////               break;
//////
//////            case CADORecordset::typeInteger:
//////               m_fieldInfoArray[ i ].type = TYPE_INT;
//////               break;
//////
//////            case CADORecordset::typeUnsignedInt:
//////               m_fieldInfoArray[ i ].type = TYPE_UINT;
//////               break;
//////
//////            case CADORecordset::typeSingle:
//////               m_fieldInfoArray[ i ].type = TYPE_FLOAT;
//////               break;
//////
//////            case CADORecordset::typeDouble:
//////               m_fieldInfoArray[ i ].type = TYPE_DOUBLE;
//////               break;
//////
//////            case CADORecordset::typeChar: 
//////            case CADORecordset::typeVarChar:
//////            case CADORecordset::typeWChar:
//////            case CADORecordset::typeVarWChar: 
//////            case CADORecordset::typeLongVarWChar:
//////            case CADORecordset::typeBinary:
//////            case CADORecordset::typeVarBinary:
//////            case CADORecordset::typeLongVarBinary:
//////                  m_fieldInfoArray[ i ].type = TYPE_DSTRING;
//////                  break;
//////
//////            //case dbDate   Date/Time; see MFC class COleDateTime
//////            //dbLongBinary   Long Binary (OLE Object); you might want to use MFC class CByteArray instead of class CLongBinary as CByteArray is richer and easier to use.
//////            //dbMemo   Memo; see MFC class CString
//////            // CADORecordset::typeEmpty 
//////            // CADORecordset::typeCurrency 
//////            // CADORecordset::typeDecimal 
//////            // CADORecordset::typeNumeric 
//////            // CADORecordset::typeDate 
//////            // CADORecordset::typeDBDate 
//////            // CADORecordset::typeDBTime 
//////            // CADORecordset::typeDBTimeStamp 
//////
//////            default:
//////               {
//////               CString msg( _T("Unsupported type in DbTable::LoadRecords() when reading field ") );
//////               msg += info.m_strName;
//////               Report::WarningMsg( msg );
//////               }
//////            }
//////         }
//////
//////      // have basic structure, now populate fields
//////      COleVariant *varArray = new COleVariant[ fieldCount ];
//////      int count = 0;
//////
//////		while ( rs.IsEOF() == FALSE )
//////			{
//////         for ( int i=0; i < fieldCount; i++ )
//////            {
//////			   try 
//////               {
//////               _variant_t _val;
//////               rs.GetFieldValue( i, _val );
//////               COleVariant val( _val );
//////               varArray[ i ] = val;
//////               //rs.GetFieldValue( i, array[ i ] );
//////   				}
//////
//////	   		catch( CADOException* )
//////		   		{
//////			   	CString msg("ADO Exception getting field value." );
//////				   Report::ErrorMsg( msg, "Exception" );
//////               return false;
//////				   }
//////			   catch ( CMemoryException* )
//////				   {
//////				   CString msg("ADO Memory Exception getting field value." );
//////				   Report::ErrorMsg( msg, "Exception" );
//////               return false;
//////				   }
//////            }  // end of for:
//////
//////			m_pData->AppendRow( varArray, fieldCount );
//////
//////			rs.MoveNext();
//////         count++;
//////			}  // end of: while( rs.IsEOF() )
//////
//////      delete [] varArray;
//////		rs.Close();            
//////		}  // end of: try ( rsTables.Open )
//////
//////	catch( CADOException *e )
//////		{
//////		CString msg("Database Error opening table: SQL=" );
//////      msg += sql;
//////		msg += ":  ";
//////      msg += e->GetErrorMessage();
//////		Report::ErrorMsg( msg, "Exception" );
//////      return -1;
//////		}
//////	catch( CMemoryException* )
//////		{
//////		CString msg("Memory Error opening table: SQL=" );
//////      msg += sql;
//////		Report::ErrorMsg( msg, "Exception" );
//////      return -2;
//////		}
//////
//////   return m_pData->GetRowCount();
//////   }
//////
//////
//////   
//////int DbTable::OpenDB( LPCTSTR databaseName, BOOL bExclusive, BOOL bReadOnly, LPCTSTR lpszConnect )
//////   {
//////   int recordCount = 0;
//////   m_databaseName = databaseName;
//////
//////   if ( m_pDatabase != NULL )
//////      delete m_pDatabase;
//////    
//////   m_pDatabase = new CADODatabase;   // required for x64 apps
//////
//////   CString strConnection = _T("");
//////   LPCTSTR ext = _tcsrchr( databaseName, '.' );
//////   if ( ext != NULL )
//////      {
//////      if ( lstrcmpi( ext, _T(".mdb") ) == 0 )
//////         {
//////         strConnection = _T("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=");
//////         strConnection += databaseName;
//////         //CString msg( _T("Error will trying to load database ") );
//////         //msg += databaseName;
//////         //msg += _T(": Access .mdb files are not supported on x64 platform" );
//////         //Report::ErrorMsg( msg );
//////         }
//////      else if ( lstrcmpi( ext, _T(".accdb") ) == 0 )
//////         {
//////         strConnection = _T("Provider=Microsoft.ACE.OLEDB.12.0;Data Source=");
//////         strConnection += databaseName;
//////         strConnection += _T(";Persist Security Info=False;" );
//////         }
//////      }   
//////   
//////	try
//////		{
//////		// open the database
//////      m_pDatabase->SetConnectionString( strConnection );
//////      m_pDatabase->SetConnectionMode(CADODatabase::connectModeRead );
//////
//////		m_pDatabase->Open(); //( databaseName, bExclusive, bReadOnly, lpszConnect );
//////      }
//////
//////	catch( CADOException* )
//////		{
//////		CString msg("Error opening database " );
//////      msg += databaseName;
//////		Report::WarningMsg( msg, "Exception" );
//////      //VData::m_useWideChar = useWideChar;
//////      return -1;
//////		}
//////
//////   return 1;
//////   }
//////
//////
//////
//////int DbTable::ReadDB( LPCTSTR sql )
//////   {
//////
//////   if ( m_pDatabase == NULL || m_pDatabase->IsOpen() == FALSE )
//////      return -1;
//////
//////   // get table name from the sql statement (try a few permutations of "From" )
//////   char *p = strstr( (char*) sql, "from" );
//////   if ( p == NULL )
//////      {
//////      p = strstr( (char*) sql, "FROM" );
//////      if ( p == NULL )
//////         p = strstr( (char*) sql, "From" );
//////      }
//////
//////   if ( p == NULL )
//////      {
//////      CString msg("Unable to find table name in SQL statement: " );
//////      msg += sql;
//////      Report::WarningMsg( msg );
//////      }
//////   else
//////      {
//////      p = strchr( p, ' ' );   // find the next blank
//////      p++;
//////      char table[ 64 ];
//////      int i = 0;
//////      while ( *p != ' ' && *p != NULL )
//////         {
//////         table[ i++ ] = *p;
//////         p++;
//////         }
//////
//////      table[ i ] = NULL;
//////      m_tableName = table;
//////      }
//////
//////   bool useWideChar = VData::SetUseWideChar( true );
//////
//////   // load up the records
//////   int recordCount = LoadRecords( *m_pDatabase, sql );
//////   ASSERT( recordCount >= 0 );
//////   
//////   VData::SetUseWideChar( useWideChar );
//////   
//////   return recordCount;
//////   }
//////
//////int DbTable::CloseDB( void )
//////   {
//////   if ( m_pDatabase == NULL )
//////      return -1;
//////
//////   if ( m_pDatabase->IsOpen() == false )
//////      return 0;
//////
//////   m_pDatabase->Close();
//////
//////   delete m_pDatabase;
//////   m_pDatabase = NULL;
//////
//////   return 1;
//////   }

#endif   // _WIN64
#endif   // NO_MFC

int DbTable::LoadDataDBF( LPCTSTR path )
   {
#ifndef NO_MFC
   m_databaseName = path;
   m_tableName = path;

   DBFHandle h = DBFOpen(path, "rb+");
   
   int cols = DBFGetFieldCount(h);

   if ( m_pData != NULL )
      delete m_pData;

   m_pData = new VDataObj;
   m_pData->SetSize( cols, 0 );
   m_fieldInfoArray.SetSize( cols );
   //database.top();

   // set field structure
   for ( int col=0; col < cols; col++ )
      {
	   char fname[12];
	   int width, decimals;
	   DBFFieldType ftype = DBFGetFieldInfo(h, col, fname, &width, &decimals);

      m_pData->AddLabel( fname );
      FIELD_INFO &fi = GetFieldInfo( col );
      fi.show = true;

      switch ( ftype )
         {
         case FTLogical:   fi.type = TYPE_BOOL;    break;
         case FTInteger:   fi.type = TYPE_LONG;    break;
         case FTDouble:    fi.type = TYPE_DOUBLE ; break;
         case FTString:    fi.type = TYPE_DSTRING; break;
         //case dbDate   Date/Time; see MFC class COleDateTime
         //dbLongBinary   Long Binary (OLE Object); you might want to use MFC class CByteArray instead of class CLongBinary as CByteArray is richer and easier to use.
         //dbMemo   Memo; see MFC class CString
         default:
            Report::WarningMsg( "Unsupported type in DbTable::LoadDataDBF()" );
         }

      fi.width = width;
      fi.decimals = decimals;
      }

   COleVariant *varArray = new COleVariant[ cols ];

   int count = 0;

   for ( unsigned int i = 0; i < (unsigned int) DBFGetRecordCount(h); i++ )
      {
//	  if( DBFIsRecordDeleted(h) )
//		continue;

      for ( int col=0; col < cols; col++ )
         {
         COleVariant val;

         switch( GetFieldType( col ) )
            {
            case TYPE_BOOL:
               {
               val.ChangeType( VT_BOOL );
               const char *p = DBFReadStringAttribute(h, i, col);
               if ( *p == 'T' || *p == 't' || *p == 'Y' || *p== 'y' )
                  val.boolVal = (VARIANT_BOOL) 1;
               else
                  val.boolVal = (VARIANT_BOOL) 0;
               }
               break;

            case TYPE_LONG:      val = (long) DBFReadIntegerAttribute(h, i, col); break;
            case TYPE_INT:       val = (long) DBFReadIntegerAttribute(h, i, col);   break;
            case TYPE_FLOAT:     val = (float) ((double) DBFReadDoubleAttribute(h, i, col)); break;
            case TYPE_DOUBLE:    val = (double) DBFReadDoubleAttribute(h, i, col);  break;
            case TYPE_DSTRING:   val = DBFReadStringAttribute(h, i, col); break;
            default:
               ;//Report::WarningMsg( "Unsupported type in DBTable::LoadDataDBF()" );
            }

         varArray[ col ] = val;
         }

      m_pData->AppendRow( varArray, cols );
      count++;
		}

   delete [] varArray;

   DBFClose(h);

   return count;
#else
   return -1;
#endif //NO_MFC
   }


int DbTable::SaveDataDBF( LPCTSTR path /*=NULL*/, BitArray *pSaveRecordsArray /*=NULL*/)
   {
#ifndef NO_MFC
   CString _path;
   if ( path == NULL )
      _path = this->m_databaseName;
   else
      _path = path;

   DBFHandle h = DBFCreate(_path);

   if(!h)
	   return -1;

   int fieldCount = (int) m_fieldInfoArray.GetSize();
   ASSERT( fieldCount == m_pData->GetColCount() );

   DBFFieldType dbftype;
   for ( int i=0; i < fieldCount; i++ )
      {
      if ( m_fieldInfoArray[ i ].save )
         {
         switch( m_fieldInfoArray[ i ].type )
            {
            case TYPE_STRING:
            case TYPE_DSTRING:
            case TYPE_CHAR:
               dbftype = FTString;
               break;

            case TYPE_BOOL:
               dbftype = FTLogical;
               break;

            case TYPE_UINT:
            case TYPE_INT:
            case TYPE_ULONG:
            case TYPE_LONG:
            case TYPE_SHORT:
               m_fieldInfoArray[ i ].width = 8;
               m_fieldInfoArray[ i ].decimals = 0;
               dbftype = FTInteger;
               break;

            case TYPE_FLOAT:  
            case TYPE_DOUBLE: 
               dbftype = FTDouble;   
               m_fieldInfoArray[ i ].width=19;
               m_fieldInfoArray[ i ].decimals=11;
               break;
 
            default:  ASSERT( 0 );
            }

         // add field
         // int retVal = fields.add( m_pData->GetLabel( i ), 
         //               type, m_fieldInfoArray[ i ].width, m_fieldInfoArray[ i ].decimals );
         LPCTSTR label = m_pData->GetLabel( i );
         int width = m_fieldInfoArray[ i ].width;
         int decimals = m_fieldInfoArray[ i ].decimals;

         int retVal = DBFAddField(h, label, dbftype, width, decimals );

         ASSERT( retVal >= 0 );
         }
      }


   int recordsWritten = 0;

   for ( int i=0; i < m_pData->GetRowCount(); i++ )
      {
      if ( pSaveRecordsArray == NULL || pSaveRecordsArray->Get( i ) == true )
         {
         int count = 0;
 
         for (int j=0, dbfIdx=-1; j < fieldCount; j++ )
            {
            if ( m_fieldInfoArray[ j ].save == false )
		         continue;
		      else
		         dbfIdx++;
		 
		      DBFFieldType dbftype =  DBFGetFieldInfo(h, dbfIdx, NULL, NULL, NULL);
            switch( dbftype )
               {
               case FTString:
                  {
                  CString value = m_pData->GetAsString( j, i ); 
                  DBFWriteStringAttribute(h, recordsWritten, dbfIdx, value);
                  }
                  break;

               case FTLogical:
                  {
                  VData value;
                  m_pData->Get( j, i, value );
                  bool bVal;
                  value.GetAsBool( bVal );
                  DBFWriteStringAttribute(h, recordsWritten, dbfIdx, bVal ? "T" : "F" );
                  }
                  break;

               case FTInteger:
                  {
                  int value = m_pData->GetAsInt( j, i );
                  DBFWriteIntegerAttribute(h, recordsWritten, dbfIdx, value);
                  }
                  break;

               case FTDouble:
                  {
                  ASSERT(19==m_fieldInfoArray[ j ].width && 11 == m_fieldInfoArray[ j ].decimals);
                  double value = m_pData->GetAsDouble( j, i );
                  //field.assignDouble( value, m_fieldInfoArray[ j ].width, m_fieldInfoArray[ j ].decimals );
			         DBFWriteDoubleAttribute(h, recordsWritten, dbfIdx, value);
                  }
                  break;

               //case 'B' is invalid; in DBF 'F' is for double
  
               default: ASSERT( 0 );
               }

            count++;
            }  // end of:  j < fieldCount;

         ++recordsWritten;
         }
      }  // end of:  i < recordCount

   DBFClose(h);

   return recordsWritten;
#else
   return -1;
#endif //NO_MFC
   }



int DbTable::LoadDataCSV( LPCTSTR path )
   {
   m_databaseName = path;
   m_tableName = path;

   if ( m_pData != NULL )
      delete m_pData;

   m_pData = new VDataObj;
   m_pData->ReadAscii( path, ',' );

   int cols = m_pData->GetColCount();
   m_fieldInfoArray.SetSize( cols );
   
   // set field structure
   for ( int col=0; col < cols; col++ )
      {
      LPCTSTR fname = m_pData->GetLabel( col );
      FIELD_INFO &fi = GetFieldInfo( col );
      fi.show = true;

      VData value;
      m_pData->Get( col, 0, value );

      TYPE type = value.GetType();

      if ( ::IsInteger( type ) )
         fi.type = TYPE_LONG;
      else if ( ::IsReal( type ) )
         fi.type = TYPE_DOUBLE;
      else 
         fi.type = type;

      GetTypeParams( type, fi.width, fi.decimals );
      }

   return m_pData->GetRowCount();
   }


#ifndef NO_MFC
#ifndef _WIN64

int DbTable::SaveDataDB( LPCTSTR databaseName, LPCTSTR tableName, LPCTSTR connectStr )
   {
   ASSERT( m_pData != NULL );

   // note:  connectStr = "" for Access, "DBASE IV;" for dbase
   CDaoDatabase database;

   //char _tableName[ 128 ];
   //lstrcpy( _tableName, tableName );

   //char *p = strrchr( _tableName, '\\' );
   //lstrcpy( p, "\\results.dbf" );

	try
		{
		// open the database ( non-exclusive, read/write )
      database.Create( databaseName );

		//database.Open( databaseName, FALSE, FALSE, connectStr );
      bool ok = CreateTable( database, tableName, connectStr );

      if ( ok )
         SaveRecords( database, tableName );
      }

	catch( CDaoException *e )
		{
		CString msg("Error creating database " );
      msg += databaseName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
		Report::ErrorMsg( msg );
		}

   database.Close();
   return m_pData->GetRowCount();
   }
   

  // helper function
bool DbTable::CreateTable( CDaoDatabase &database, LPCTSTR tableName, LPCTSTR connectStr )
   {
   // delete the specified table from the database
   /*
   try 
      {
      database.DeleteTableDef( "cell.dbf" );  // tableName
      }

   catch( CDaoException *e )
      {
		CString msg("Database Error deleting table " );
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
		m_pMap->MessageBox( msg, "Warning", MB_OK );
      }
*/
   // current table deleted, create a new one corresponding to the data object
   CDaoTableDef td( &database );
   try
      {
      td.Create( tableName, 0, NULL, connectStr );
      }

   catch( CDaoException *e )
      {
		CString msg("Database Error creating table " );
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
		Report::ErrorMsg( msg );
      return false;
      }

   // table created, start adding fields
   int rows = m_pData->GetRowCount();

   for ( int i=0; i < m_pData->GetColCount(); i++ )
      {
      if ( m_fieldInfoArray[ i ].save )
         {
         LPCTSTR fieldName = m_pData->GetLabel( i );
         short type = -1;

         TYPE fieldType = GetFieldType ( i );

         switch( fieldType )
            {
            case TYPE_INT:
            case TYPE_UINT:
            case TYPE_LONG:
            case TYPE_ULONG:     type = dbLong;      break; 
            case TYPE_FLOAT:     type = dbSingle;    break;
            case TYPE_DOUBLE:    type = dbDouble;    break;
            case TYPE_STRING:
            case TYPE_DSTRING:   type = dbText;      break;
            case TYPE_BOOL:      type = dbBoolean;   break;  // Boolean; True=-1, False=0.
            default:
               {
               CString msg;
               msg.Format( "Unsupported field type at col %d", i );
               Report::ErrorMsg( msg );
               continue;
               }
            }

         int size = 0;
         if ( type == dbText )
            {
            // make sure it will hold the longest text
            for ( int j=0; j < rows; j++ )
               {
               CString val = m_pData->GetAsString( i, j );
               if ( val.GetLength() > size )
                  size = val.GetLength();
               }

            size += 10; // for good measure
            if ( size > 255 )
               size = 255;
            }

         try
            {
            td.CreateField( fieldName, type, size, dbUpdatableField | dbFixedField );
            }

         catch( CDaoException *e )
            {
     		   CString msg("Database Error creating field definition " );
            msg += tableName;
            msg += ", ";
            msg += fieldName;
            msg += ": ";
            msg += e->m_pErrorInfo->m_strDescription;
	   	   Report::ErrorMsg( msg, "Exception" );
            continue;
            }
         }  // end of field creation
      } // end of: if (save)

   try
      {
      td.Append();      // add the table to the database table collection
      }

   catch( CDaoException *e )
      {
      CString msg("Database Error appending table :  " );
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
	   Report::ErrorMsg( msg, "Exception" );
      }
   
   return true;
   }



int DbTable::SaveRecords( CDaoDatabase &database, LPCTSTR tableName )
   {
   CDaoTableDef td( &database );

   try{
      td.Open( tableName );
      }
   catch ( CDaoException *e )
      {
  		CString msg("Database Error opening table " );
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
     	Report::ErrorMsg( msg, "Exception" );
      return -1;
      }

   // have the table defition, open the table into a recordset
	CDaoRecordset rs( &database );

	try
		{
		rs.Open( &td, dbOpenTable, dbAppendOnly );

      // start a transaction
      //database.m_pWorkspace->BeginTrans();

      int recordCount = m_pData->GetRowCount();
      COleVariant value;

      for ( int row=0; row < recordCount; row++ )
         {
         rs.AddNew();
         //rs.SetBookmark( rs.GetLastModifiedBookmark( ) );
         //rs.MoveLast();

         // add data to the recordset
         int cols = m_pData->GetColCount();
         int added = 0;

         for ( int col=0; col < cols; col++ )
            {
            if ( m_fieldInfoArray[ col ].save )
               {
               m_pData->Get( col, row, value );
               rs.SetFieldValue( added++, value );
               }
            }

			rs.Update();
         rs.MoveLast();
			}  // end of: for ( row < recordCount )

      //database.m_pWorkspace->CommitTrans();
      rs.Close();
		}  // end of: try ( rsTables.Open )


	catch( CDaoException *e )
		{
		CString msg("Database Error on : " );
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;

		Report::ErrorMsg( msg, "Exception" );
      return -1;
		}
	catch( CMemoryException* )
		{
		CString msg("Memory Error opening table: " );
      msg += tableName;
		Report::ErrorMsg( msg, "Exception" );
      return -2;
		}

   td.Close();

   return m_pData->GetRowCount();
   }

#endif // _WIN64
#endif

bool DbTable::RefreshFieldInfo( void )
   {
   if ( m_fieldInfoArray.GetSize() != GetColCount() )
      {
      m_fieldInfoArray.RemoveAll();
      m_fieldInfoArray.SetSize( GetColCount() );

      for ( int i=0; i < m_fieldInfoArray.GetSize(); i++ )
         {
         m_fieldInfoArray[ i ].type = TYPE_NULL;
         m_fieldInfoArray[ i ].show = false;
         }
      }

   if ( GetRecordCount() == 0 )
      {
      ASSERT( 0 );
      return false;
      }

   for ( int i=0; i < m_fieldInfoArray.GetSize(); i++ )
      m_fieldInfoArray[ i ].type = m_pData->GetType( i );

   return true;
   }

bool DbTable::RemoveAllFields(void)
   {
   m_fieldInfoArray.RemoveAll();
   return true;
   }

int DbTable::AddFields( int count )
   {
   // add fields to field info
   for ( int i=0; i < count; i++ )
     {
       FIELD_INFO info;
       m_fieldInfoArray.Add( info );
     }

   // add to the data object
   return m_pData->AppendCols( count );
   }

int DbTable::AddField( LPCTSTR name, TYPE type, bool addCol /*=true*/ )
   {
   int width, decimals;
   GetTypeParams( type, width, decimals );
   
   return AddField( name, type, width, decimals, addCol );
   }


int DbTable::AddField( LPCTSTR name, TYPE type, int width, int decimals, bool addCol /*=true*/ )
   {
   FIELD_INFO info(type);
   m_fieldInfoArray.Add( info );

   int index = (int) m_fieldInfoArray.GetSize() - 1;
   m_fieldInfoArray[ index ].width    = width;
   m_fieldInfoArray[ index ].decimals = decimals;

   /*switch ( type )
      {
      case TYPE_FLOAT:
         //m_fieldInfoArray[ index ].type    = TYPE_DOUBLE;
         m_fieldInfoArray[ index ].width    = width;
         m_fieldInfoArray[ index ].decimals = decimals;
         break;

      case TYPE_DOUBLE:
         m_fieldInfoArray[ index ].width    = 19;
         m_fieldInfoArray[ index ].decimals = 11;
         break;

      default:
         m_fieldInfoArray[ index ].width    = 10;
         m_fieldInfoArray[ index ].decimals = 0;
         break;
      }
      */
   if ( addCol )
      {
      if ( m_pData == NULL )
         {
         switch( m_dotType )
            {
            case DOT_INT:
               m_pData = new IDataObj;
               break;

            case DOT_VDATA:
               m_pData = new VDataObj;
               break;
               
            case DOT_FLOAT:
               m_pData = new FDataObj;
               break;
               
            default:
               ASSERT( 0 );  // DOT_NULL, DOT_OTHER - illegal
            }
         }

      m_pData->AppendCol( name );
      }

   return index;
   }


int DbTable::InsertField( int insertBefore, LPCTSTR name, TYPE type )
   {
   int width, decimals;
   GetTypeParams( type, width, decimals );
   return InsertField( insertBefore, name, type, width, decimals );
   }


int DbTable::InsertField( int insertBefore, LPCTSTR name, TYPE type, int width, int decimals )
   {
     FIELD_INFO info;
   m_fieldInfoArray.Add( info );

   int col = m_pData->InsertCol( insertBefore, name );
   int cols = (int) m_fieldInfoArray.GetSize();

   for ( int i=cols-1; i > col; i-- )
      {
      m_fieldInfoArray[ i ].type = m_fieldInfoArray[ i-1 ].type;
      m_fieldInfoArray[ i ].show = m_fieldInfoArray[ i-1 ].show;
      m_fieldInfoArray[ i ].save = m_fieldInfoArray[ i-1 ].save;
      }

   m_fieldInfoArray[ col ].type = type;
   m_fieldInfoArray[ col ].show = true;
   m_fieldInfoArray[ col ].save = true;
   m_fieldInfoArray[ col ].width    = width;
   m_fieldInfoArray[ col ].decimals = decimals;
   
   return col;
   }


int DbTable::RemoveField( LPCTSTR name )
   {
   int col = GetFieldCol( name );

   if ( col >= 0 )
      return RemoveField( col );

   else
      return -1;
   }

int DbTable::RemoveField( int col )
   {
   m_fieldInfoArray[ col ].save = false;
   return col;
   }

     
bool DbTable::SwapCols( int col0, int col1 )
   {
   if ( col0 < 0 || col1 < 0 || col0 > this->GetColCount()-1 || col1 > this->GetColCount()-1 )
      return false;
   
   // swap header info (note, with DbTables, there is alway a fieldInfo for each col in the database
   FIELD_INFO &f0 = this->GetFieldInfo( col0 );   // note: this is for DbTable, not MapLayer! 
   FIELD_INFO &f1 = this->GetFieldInfo( col1 );

   FIELD_INFO fTemp = f0;
   f0 = f1;
   f1 = fTemp;

   // swap column labels
   CString hdr0 = this->GetColLabel( col0 );
   CString hdr1 = this->GetColLabel( col1 );

   this->SetColLabel( col0, hdr1 );
   this->SetColLabel( col1, hdr0 );

   // swap data
   int rows = this->GetRecordCount();

   switch( m_dotType )
      {
      case DOT_FLOAT:
         {
         float value;
         for ( int i=0; i < rows; i++ )
            {
            value = m_pData->GetAsFloat( col0, i );
            m_pData->Set( col0, i, m_pData->GetAsFloat( col1, i ) );
            m_pData->Set( col1, i, value );
            }
         }
         break;

      case DOT_VDATA:
         {
         VData value0, value1;
         for ( int i=0; i < rows; i++ )
            {
            m_pData->Get( col0, i, value0 );
            m_pData->Get( col1, i, value1 );
            m_pData->Set( col0, i, value1 );
            m_pData->Set( col1, i, value0 );
            }
         }
         break;

      case DOT_INT:
         {
         int value;
         for ( int i=0; i < rows; i++ )
            {
            value = m_pData->GetAsInt( col0, i );
            m_pData->Set( col0, i, m_pData->GetAsInt( col1, i ) );
            m_pData->Set( col1, i, value );
            }
         }
         break;

      default:
         ASSERT( 0 );
      }

   return true;
   }


void DbTable::SetFieldType( int col, TYPE type, bool convertData )
   {
   int width, decimals;
   GetTypeParams( type, width, decimals );
   SetFieldType( col, type, width, decimals, convertData );
   }


void DbTable::SetFieldType( int col, TYPE type, int width, int decimals, bool convertData )
   {
   ASSERT( m_pData != NULL );
   ASSERT( col < (int) m_fieldInfoArray.GetSize() );
   m_fieldInfoArray[ col ].type = type;

   m_fieldInfoArray[ col ].width = width;

   if ( ::IsString( type ) )
      decimals = 0;
   if ( ::IsInteger( type ) )
      decimals = 0;

   m_fieldInfoArray[ col ].decimals = decimals;

   if ( convertData && m_pData != NULL && GetDOType() == DOT_VDATA )
      {
      for ( INT_PTR i=0; i < m_pData->GetRowCount(); i++ )
         {
         VData value;
         m_pData->Get( col, int( i ), value );

         if ( value.type != TYPE_NULL && value.type != type )
            {
            value.ChangeType( type );
            m_pData->Set( col, int( i ), value );
            }
         }
      }
   }


int DbTable::AddJoinTable( DbTable *pJoinTable, LPCTSTR parentJoinCol, LPCTSTR childJoinCol, bool deleteChildTableOnDelete )
   {
   JoinInfo *pInfo = new JoinInfo;
   int retVal = pInfo->CreateJoin( this, pJoinTable, parentJoinCol, childJoinCol );
   
   pInfo->m_deleteChildTableOnDelete = deleteChildTableOnDelete;

   if ( retVal > 0 )
      this->m_joinTables.Add( pInfo );
   else
      delete pInfo;
    
   return retVal;   
   }
