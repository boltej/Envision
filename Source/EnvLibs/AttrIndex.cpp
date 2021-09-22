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
#include "EnvLibs.h"
#pragma hdrstop

#include "AttrIndex.h"
#include "DBTABLE.H"


AttrIndex::~AttrIndex(void)
   {
   Clear();
   }


int AttrIndex::BuildIndex( DbTable *pDbTable, int col )
   {
   if ( m_pDbTable == NULL )
      m_pDbTable = pDbTable;

   // make sure we are dealing with the same layer
   if ( m_pDbTable != pDbTable )
      {
      ASSERT( 0 );
      return -1;
      }

   if ( m_filename.IsEmpty() )
      GetDefaultFilename( m_filename );

   TYPE type = pDbTable->GetFieldType( col );
   ASSERT( type == TYPE_SHORT || type == TYPE_INT || type == TYPE_LONG || type == TYPE_STRING || type == TYPE_DSTRING || type == TYPE_BOOL );

   // make sure no index already exists for this col
   for ( int i=0; i < m_attrInfoArray.GetSize(); i++ )
      if ( m_attrInfoArray[ i ]->col == col )
         return col;

   // no existing col in the index, so add it
   ATTR_INFO_ARRAY *pInfoArray = new ATTR_INFO_ARRAY;
   m_attrInfoArray.Add( pInfoArray );
   pInfoArray->col = col;

   // iterate through the columns data, getting attribute values
   int count = pDbTable->GetRecordCount();
   VData value;

   // use the vdata value to look up the index associated with the vdata in the 
   // ATTR_INFO_ARRAY associated with this column

   int uniqueValues = 0;

   // iterate through each record/polygon in the table being index
   for ( int row=0; row < count; row++ )
      {
      bool ok = pDbTable->GetData( row, col, value );
      ASSERT( ok );

      // have we not seen this value yet?
      // Note that the valueIndex map is key: attribute value (VData), value: index into the infoItems array
      //   (infoItems contains information (value, array of indices of polygons containing this value)
      //   about each attribute)
      int valueIndex;
      if ( pInfoArray->valueIndexMap.Lookup( value, valueIndex ) == FALSE )
         {
         // no, so create a new ATTR_INFO for this attribute value
         ATTR_INFO newInfo(value);
         valueIndex = (int) pInfoArray->infoItems.Add( newInfo );

         // add the value to the value index map
         pInfoArray->valueIndexMap.SetAt( value, valueIndex );

         uniqueValues++;
         }

      // in any case, add the polygon index to the attributes polyIndexArray
      ATTR_INFO &attrInfo = pInfoArray->infoItems.GetAt( valueIndex );
      attrInfo.polyIndexArray.Add( row );
      }

   m_isChanged = false;
   m_colIndexMap.SetAt( col, (int) m_attrInfoArray.GetSize()-1 );
   
   return uniqueValues;
   }


int AttrIndex::WriteIndex( LPCTSTR filename /*=NULL*/ )
   {
   int indexCount = GetIndexCount();

   if ( indexCount == 0 )
      return -1;

   char _filename[ 256 ];
   if ( filename == NULL )
      lstrcpy( _filename, m_filename );
   else
      lstrcpy( _filename, filename );

   FILE *fp;
   fopen_s( &fp, _filename, "wb" );

   if ( fp == NULL )
      {
      CString msg( "Unable to open " );
      msg += _filename;
      msg += " when writing attribute index - The index will not be created. ";
      Report::LogError( msg );
      return -2;
      }

   int byteCount = 0;

   // file format:
   // 1) header includes version, indexCount, record count,  path/name of source file (256 bytes)
   // 2) for each ATTR_INFO_ARRAY,
   //    2a) column
   //    2b) number of unique attribute values stored in the index (this is the number of map items and the number of infoItems)
   //    2c) for each ATTR_INFO stored in the infoItems array, 
   //        3a) the associated VData structure
   //        3b) the number of polygons associated with this value
   //        3c) an array of the polygon indexes associated with this value
   // 4) bytecount and end of file marker

   // 1) header includes version, indexCount, record count,  path/name of source file (256 bytes)
   int version = 1;
   int recordCount   = m_pDbTable->GetRecordCount();

   fwrite( (void*) &version,     sizeof( version ),     1, fp );
   fwrite( (void*) &indexCount,  sizeof( indexCount ),  1, fp );
   fwrite( (void*) &recordCount, sizeof( int ),         1, fp );

   char path[ 256 ];
   int szChar = sizeof( char );
   lstrcpy( path, m_pDbTable->m_databaseName );
   fwrite( (void*) path, szChar, 256, fp );
   byteCount = 3*sizeof( int ) + 256*szChar;

   // 2) for each ATTR_INFO_ARRAY,
   for ( int i=0; i < indexCount; i++ )
      {
      ATTR_INFO_ARRAY *pInfoArray = m_attrInfoArray[ i ];
      ASSERT( pInfoArray != NULL );
      // 2a) column
      fwrite( (void*) &pInfoArray->col, sizeof( int ), 1, fp );
      byteCount += sizeof( int );

      // 2b) number of unique attribute values stored in the index (this is the number of map items and the number of infoItems)
      int valueCount = (int) pInfoArray->infoItems.GetSize();
      ASSERT( valueCount == pInfoArray->valueIndexMap.GetCount() );
      fwrite( (void*) &valueCount, sizeof( int ), 1, fp );
      byteCount += sizeof( int );

      // 2c) for each ATTR_INFO stored in the infoItems array, 
      for ( int j=0; j < valueCount; j++ )
         {
         ATTR_INFO &info = pInfoArray->infoItems[ j ];
         // 3a) the associated VData structure
         fwrite( (void*) &(info.attribute), sizeof( VData ), 1, fp );
         
         // 3b) the number of polygons associated with this value
         int polyCount = (int) info.polyIndexArray.GetSize();
         fwrite( (void*) &polyCount, sizeof( int ), 1, fp );

         // 3c) an array of the polygon indexes associated with this value
         UINT *pBase = info.polyIndexArray.GetData();
         fwrite( (void*) pBase, sizeof( UINT ), polyCount, fp );

         byteCount += sizeof( VData ) + sizeof( int ) + polyCount*sizeof( UINT );
         } 
      }  // end of:  for ( i < indexCount )

   // 4) bytecount and end of file marker
   fwrite( (void*) &byteCount, sizeof( int ), 1, fp );

   int eof = 0xFFFF;
   fwrite( (void*) &eof, sizeof( int ), 1, fp );

   fclose( fp );
   return indexCount;
   }




int AttrIndex::WriteIndexText(LPCTSTR filename /*=NULL*/)
   {
   int indexCount = GetIndexCount();

   if (indexCount == 0)
      return -1;

   char _filename[256];
   if (filename == NULL)
      lstrcpy(_filename, m_filename);
   else
      lstrcpy(_filename, filename);

   FILE* fp;
   fopen_s(&fp, _filename, "wt");

   if (fp == NULL)
      {
      CString msg("Unable to open ");
      msg += _filename;
      msg += " when writing attribute index - The index will not be created. ";
      Report::LogError(msg);
      return -2;
      }

   int byteCount = 0;

   // file format:
   // 1) header includes version, indexCount, record count,  path/name of source file (256 bytes)
   // 2) for each ATTR_INFO_ARRAY,
   //    2a) column
   //    2b) number of unique attribute values stored in the index (this is the number of map items and the number of infoItems)
   //    2c) for each ATTR_INFO stored in the infoItems array, 
   //        3a) the associated VData structure
   //        3b) the number of polygons associated with this value
   //        3c) an array of the polygon indexes associated with this value
   // 4) bytecount and end of file marker

   // 1) header includes version, indexCount, record count,  path/name of source file (256 bytes)
   int version = 1;
   int recordCount = m_pDbTable->GetRecordCount();

   fprintf(fp, "Version: %i\n", version);
   fprintf(fp, "Index Count: %i\n", indexCount);
   fprintf(fp, "Record Count: %i\n", recordCount);

   // 2) for each ATTR_INFO_ARRAY,
   for (int i = 0; i < indexCount; i++)
      {
      ATTR_INFO_ARRAY* pInfoArray = m_attrInfoArray[i];
      ASSERT(pInfoArray != NULL);
      // 2a) column
      fprintf(fp, "Column: %i", pInfoArray->col);

      // 2b) number of unique attribute values stored in the index (this is the number of map items and the number of infoItems)
      int valueCount = (int)pInfoArray->infoItems.GetSize();
      ASSERT(valueCount == pInfoArray->valueIndexMap.GetCount());
      fprintf(fp, "  Value Count: %i\n  Values: \n", valueCount);

      // 2c) for each ATTR_INFO stored in the infoItems array, 
      for (int j = 0; j < valueCount; j++)
         {
         ATTR_INFO& info = pInfoArray->infoItems[j];
         // 3a) the associated VData structure
         fprintf(fp, "     %s ", info.attribute.GetAsString());

         // 3b) the number of polygons associated with this value
         int polyCount = (int)info.polyIndexArray.GetSize();
         fprintf(fp, "(%i):", polyCount);

         // 3c) an array of the polygon indexes associated with this value
         UINT* pBase = info.polyIndexArray.GetData();
         for (int k = 0; k < polyCount; k++)
            fprintf(fp, " %i,", info.polyIndexArray[k]);

         fprintf(fp, "\n");

         }
      }  // end of:  for ( i < indexCount )

   fclose(fp);
   return indexCount;
   }


void AttrIndex::GetDefaultFilename( CString &filename )
   {
   // no associated map layer?
   if ( m_pDbTable == NULL || m_pDbTable->m_databaseName.IsEmpty() )
      {
      filename = "temp.atndx";
      return;
      }

   // mapDbTable exists, use it 
   char _filename[ 128 ];
   lstrcpy( _filename, (LPCTSTR) m_pDbTable->m_databaseName );
   char *dot = strrchr( _filename, '.' );

   if ( dot != NULL )
      lstrcpy( dot, ".atndx" );
   else
      lstrcat( _filename, ".atndx" );
   
   filename = _filename;
   }


int AttrIndex::ReadIndex( DbTable *pDbTable, LPCTSTR filename /*=NULL*/ )
   {
   bool writeWhenDone = false;
   Clear();

   m_pDbTable = pDbTable;

   if ( filename == NULL )
      GetDefaultFilename( m_filename );
   else
      m_filename = filename;

   PCTSTR file = (PCTSTR)m_filename;

   FILE *fp;
   fopen_s( &fp, file, "rb" );    // read binary
   if ( fp == NULL )
      return -2;

   m_pDbTable = pDbTable;
   m_isBuilt = false;
 
   int retVal = -99;
   // 1) header
   int version, indexCount, recordCount;
   if ( fread( (void*) &version,     sizeof( int ),  1, fp ) == 0 ) goto error;
   if ( fread( (void*) &indexCount,  sizeof( int ),  1, fp ) == 0 ) goto error;
   if ( fread( (void*) &recordCount, sizeof( int ),  1, fp ) == 0 ) goto error;

   char path[ 256 ];
   if ( fread( (void*) path, sizeof( char ), 256, fp ) == 0 ) goto error;

   if ( lstrcmpi( path, m_pDbTable->m_databaseName ) != 0 )
      {
#ifndef NO_MFC
      CString msg( "This attribute index was built with a different source DbTable, but contains the same number of records.  Table " );
      msg += m_pDbTable->m_databaseName;
      msg += ", file was built with ";
      msg += path;
      msg += ". Do you want to associate this index with the new source file?" ;
      retVal = AfxGetMainWnd()->MessageBox( msg, "Warning", MB_YESNOCANCEL );
      switch( retVal )
         {
         case IDYES:
            writeWhenDone = true;
            break;

         case IDNO:
            break;

         case IDCANCEL:
            goto error;
         }
#else
      goto error;
#endif
      }
 
   // next, for each index (ATTR_INFO_ARRAY), populate the structure
   m_attrInfoArray.SetSize( indexCount );
   for ( int i=0; i < indexCount; i++ )
      {
      // create a strucutre to populate
      m_attrInfoArray[ i ] = new ATTR_INFO_ARRAY;

      // get the column for this item
      if ( fread( (void*) &(m_attrInfoArray[ i ]->col), sizeof( int ),  1, fp ) == 0 ) goto error;

      // get the number of values stored in the index for this item
      int valueCount; 
      if ( fread( (void*) &valueCount, sizeof( int ),  1, fp ) == 0 ) goto error;

      // for each attribute value, create a corresponding ATTR_INFO struct and populate it
      m_attrInfoArray[ i ]->infoItems.SetSize( valueCount );
      for ( int j=0; j < valueCount; j++ )
         {
         ATTR_INFO &info = m_attrInfoArray[ i ]->infoItems[ j ];
         if ( fread( (void*) &(info.attribute), sizeof( VData ), 1, fp ) == 0 ) goto error;

         int polyCount;
         if ( fread( (void*) &polyCount, sizeof( int ), 1, fp ) == 0 ) goto error;
         info.polyIndexArray.SetSize( polyCount );

         UINT *pBase= info.polyIndexArray.GetData();
         if ( fread( (void*) pBase, sizeof( UINT ), polyCount, fp ) == 0 ) goto error;

         // add mapping between this attribute value and the corresponding index into the infoItems array;
         m_attrInfoArray[ i ]->valueIndexMap.SetAt( info.attribute, j );         
         }

      m_colIndexMap.SetAt( m_attrInfoArray[ i ]->col, i );
      } // end of: for ( i < indexCount )

   int _byteCount;
   if ( fread( (void*) &_byteCount, sizeof( int ), 1, fp ) == 0 ) goto error;

   // compare bytecounts
   int eof;
   if ( fread( (void*) &eof, sizeof( int ), 1, fp ) == 0 ) goto error; //second is eof

   if ( eof != 0xFFFF )
      goto error;

   fclose( fp );

   m_isBuilt = true;

   if ( writeWhenDone )
      WriteIndex( filename );

   m_isChanged = false;

   return indexCount;

error:
   fclose( fp );
   return -3;
   }


void AttrIndex::Clear( void )
   {
   for ( int i=0; i < m_attrInfoArray.GetSize(); i++ )
      delete m_attrInfoArray[ i ];

   m_attrInfoArray.RemoveAll();
   m_colIndexMap.RemoveAll();
   }


bool AttrIndex::ChangeData( int col, int record, VData oldValue, VData newValue )
   {
   int index;
   BOOL found = m_colIndexMap.Lookup( col, index );

   if ( ! found )
      return false;

   // find the attribute info array for this column
   ATTR_INFO_ARRAY *pInfoArray = m_attrInfoArray[ index ];

   // have the right attribute info array (for this column), find the right ATTR_INFO structure for the old, new values
   int oldIndex, newIndex;
   if ( ! pInfoArray->valueIndexMap.Lookup( oldValue, oldIndex ) )
      return false;

   if ( ! pInfoArray->valueIndexMap.Lookup( newValue, newIndex ) )
      return false;

   // remove the old value from the array of polygons
   ATTR_INFO &info = pInfoArray->infoItems[ oldIndex ];
   int polyCount = (int) info.polyIndexArray.GetSize();
   int removeIndex = -1;
   for ( int i=0; i < polyCount; i++ )
      {
      if ( info.polyIndexArray[ i ] == record )
         {
         removeIndex = i;
         break;
         }
      }
   if ( removeIndex >= 0 )
      info.polyIndexArray.RemoveAt( removeIndex );

   // add the new value to the array of polygons
   info = pInfoArray->infoItems[ newIndex ];
   info.polyIndexArray.Add( record );

   return true;
   }


int AttrIndex::GetRecordArray( int col, VData key, CUIntArray &polyArray )
   {
   polyArray.RemoveAll();

   int index;
   if (  ! m_colIndexMap.Lookup( col, index ) )
      return -1;  // no index for this column

   // find the attribute info array for this column
   ATTR_INFO_ARRAY *pInfoArray = m_attrInfoArray[ index ];

   // have the right attribute info array (for this column), find the right ATTR_INFO structure for the specified value
   if ( ! pInfoArray->valueIndexMap.Lookup( key, index ) )
      return -2;  // no value found

   // populate the poly array with the values stored in the index
   polyArray.Copy( pInfoArray->infoItems[ index ].polyIndexArray );
   
   return (int) polyArray.GetSize();
   }

int AttrIndex::GetRecord( int col, VData key, int startRecord /*=0*/ )
   {
   int index;
   if (  ! m_colIndexMap.Lookup( col, index ) )
      return -1;  // no index for this column

   // find the attribute info array for this column
   ATTR_INFO_ARRAY *pInfoArray = m_attrInfoArray[ index ];

   // have the right attribute info array (for this column), find the right ATTR_INFO structure for the specified value
   if ( ! pInfoArray->valueIndexMap.Lookup( key, index ) )
      return -2;  // no value found

   // get the next record in the indexpopulate the poly array with the values stored in the index
   CUIntArray &recordArray = pInfoArray->infoItems[ index ].polyIndexArray;
   if ( recordArray.GetSize() == 0 )
      return -1;

   int i = 0;
   while ( startRecord >= (int) recordArray[ i ] )
      i++;
   
   return recordArray[ i ];
   }
