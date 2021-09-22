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
//-------------------------------------------------------------------
//  VDATAOBJ.CPP
//
//-- code for the global data object
//-------------------------------------------------------------------

#include <EnvLibs.h>
#include <stdexcept>
#pragma hdrstop

#ifndef NO_MFC
#include <io.h>
#endif

#include <math.h>
#include <exception>
#include "Vdataobj.h"
#include "misc.h"

#include "Path.h"
#include "PathManager.h"


#define USE_SHAPELIB

#ifdef USE_SHAPELIB
#include "shapefil.h"
#endif


//-- VDataObj::VDataObj() ---------------------------------------------
//
//-- constructors
//-------------------------------------------------------------------

VDataObj::VDataObj( UNIT_MEASURE m )
   : DataObj(m),
     matrix  ()     
   { }


//-------------------------------------------------------------------
VDataObj::VDataObj( VDataObj &dataObj)
   : DataObj( dataObj),
     matrix  ( dataObj.matrix )     
   { }


//-------------------------------------------------------------------
VDataObj::VDataObj( int _cols, int _rows, UNIT_MEASURE m)
   : DataObj( _cols, _rows, m ),
     matrix( _rows, _cols )
   { }



//-- VDataObj::~VDataObj() --------------------------------------------
//
//-- destructor
//-------------------------------------------------------------------

VDataObj::~VDataObj( void )
   { /*
   for ( int row=0; row < this->GetRowCount(); row++ )
      {
      for ( int col=0; col < this->GetColCount(); col++ )
         {
         VData v;
         Get( col, row, v );

         if ( v.GetType() == TYPE_DATAOBJ && v.val.vPtrDataObj != NULL )
            delete v.val.vPtrDataObj;
         }
      } */   
   }


VData & VDataObj::Get( int col, int row )
   {
   int cols = GetColCount();
   int rows = GetRowCount();

   if ( col < 0 || col >= cols )
      {
      CString msg;
      msg.Format( "Invalid column index (%i) passed to VDataObj '%s' (cols=%i,rows=%i) when getting a value!",
         col, this->m_name, cols, rows );
      Report::ErrorMsg( msg );
//#ifdef NO_MFC
      throw std::out_of_range("VDataObj: bad column index");
//#else
//      VData v();
//      return v;  // not a good idea!
//#endif
      }
         
   if ( row < 0 || row >= rows )
      {
      CString msg;
      msg.Format( "Invalid row index (%i) passed to VDataObj '%s' (cols=%i,rows=%i) when getting a value!",
         col, this->m_name, cols, rows );
      Report::ErrorMsg( msg );
//#ifdef NO_MFC
	  throw std::out_of_range("VDataObj: bad row index");
//#else
//      VData v();
//      return v;  // not a good idea!
//#endif
      }
         
   return matrix.Get( row, col ); 
   }



bool VDataObj::Get( int col, int row, COleVariant &var )
   {
   VData &v = Get( col, row );

   v.ConvertToVariant( var );

   if ( v.type == TYPE_NULL )
      return false;
   else
      return true;
   }


bool VDataObj::Get( int col, int row, VData &v )
   {
   int cols = GetColCount();
   int rows = GetRowCount();
   if ( col >= cols || row >= rows )
      return false;

   v = Get( col, row );
   return true;
   }

      
bool VDataObj::Get( int col, int row, float &var )
   { 
   VData &v = Get( col, row ); 

   switch ( v.type )
      {
      case TYPE_FLOAT:
         var = v.val.vFloat;
         return true;

      case TYPE_DOUBLE:
         var = (float) v.val.vDouble;
         return true;

      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_ULONG:
      case TYPE_LONG:
      case TYPE_SHORT:
         {
         int ivalue;
         Get( col, row, ivalue );
         var = (float) ivalue;
         return true;
         }

      case TYPE_NULL:
         var = 0.0f;
         return false;

      case TYPE_STRING:
      case TYPE_DSTRING:
         var = (float) atof( v.val.vString );
         return true;
      }

   return false;
   }
      
bool VDataObj::Get( int col, int row, double &var )
   { 
   VData &v = Get( col, row ); 

   switch ( v.type )
      {
      case TYPE_FLOAT:
         var = (double)v.val.vFloat;
         return true;

      case TYPE_DOUBLE:
         var =  v.val.vDouble;
         return true;

      case TYPE_DATE:
         var = v.val.vDateTime;
         return true;

      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_ULONG:
      case TYPE_LONG:
      case TYPE_SHORT:
         {
         int ivalue;
         Get( col, row, ivalue );
         var = (double) ivalue;
         return true;
         }

      case TYPE_NULL:
         var = 0.0;
         return false;

      case TYPE_STRING:
      case TYPE_DSTRING:
         var = atof( v.val.vString );
         return true;
      }

   return false;
   }
                        
 

bool VDataObj::Get( int col, int row, int &var )
   { 
   VData &v = Get( col, row ); 
			
   switch ( v.type )
      {
      case TYPE_CHAR:
         var = (int) v.val.vChar;
         return true;

      case TYPE_FLOAT:
         var = (int) v.val.vFloat;
         return true;

      case TYPE_DOUBLE:
         var = (int) v.val.vDouble;
         return true;

      case TYPE_INT:
         var = v.val.vInt;
         return true;

      case TYPE_SHORT:
         var = (int) v.val.vShort;
         return true;

      case TYPE_BOOL:
         var = v.val.vBool == true ? 1 : 0;
         return true;

      case TYPE_LONG:
         var = v.val.vLong;
         return true;

      case TYPE_UINT:
         var = v.val.vUInt;
         return true;

      case TYPE_ULONG:
         var = v.val.vULong;
         return true;

      case TYPE_STRING:
      case TYPE_DSTRING:
         if ( v.val.vString != NULL )
            {
            var = atoi( v.val.vString );
            return true;
            }
         else
            return false;

      case TYPE_NULL:
         var = 0;
         return false;
      }

   var = 0;
   return false;
   }
            
bool VDataObj::Get(int col, int row, short &var)
   {
   VData &v = Get(col, row);

   switch (v.type)
      {
      case TYPE_CHAR:
         var = (short)v.val.vChar;
         return true;

      case TYPE_FLOAT:
         var = (short)v.val.vFloat;
         return true;

      case TYPE_DOUBLE:
         var = (short)v.val.vDouble;
         return true;

      case TYPE_INT:
         var = (short) v.val.vInt;
         return true;

      case TYPE_SHORT:
         var = v.val.vShort;
         return true;

      case TYPE_BOOL:
         var = v.val.vBool == true ? 1 : 0;
         return true;

      case TYPE_LONG:
         var = (short) v.val.vLong;
         return true;

      case TYPE_UINT:
         var = (short) v.val.vUInt;
         return true;

      case TYPE_ULONG:
         var = (short) v.val.vULong;
         return true;

      case TYPE_STRING:
      case TYPE_DSTRING:
         if (v.val.vString != NULL)
            {
            var = (short) atoi(v.val.vString);
            return true;
            }
         else
            return false;

      case TYPE_NULL:
         var = 0;
         return false;
      }

   var = 0;
   return false;
   }


bool VDataObj::Get( int col, int row, bool &var )
   {
   VData &v = Get( col, row ); 

   switch( v.type )
      {
      case TYPE_BOOL:
         var = v.val.vBool;
         return true;

      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_LONG:
      case TYPE_ULONG:
      case TYPE_SHORT:
         {
         int ivalue;
         Get( col, row, ivalue );
         var = ( ivalue == 0 ? false : true );
         }
         return true;

      case TYPE_NULL:
      default:
         var = false;
         return false;
      }
   }

bool VDataObj::Get( int col, int row, DataObj* &pDataObj )
   {
   VData &v = Get( col, row ); 

   if ( v.type != TYPE_PDATAOBJ )
      {
      pDataObj = NULL;
      return false;
      }

   pDataObj = v.val.vPtrDataObj;
   return true;
   }


bool VDataObj::Get( int col, int row, TCHAR &var )
   {
   VData &v = Get( col, row ); 

   switch( v.type )
      {
      case TYPE_BOOL:
         var = v.val.vBool ? 'T' : 'F';
         return true;

      case TYPE_CHAR:
         var = v.val.vChar;
         break;

      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_LONG:
      case TYPE_ULONG:
      case TYPE_SHORT:
         var = (TCHAR) v.val.vInt; 
         return true;

      case TYPE_STRING:
      case TYPE_DSTRING:
         if ( v.val.vString != NULL )
            var = *(v.val.vString);
         else
            var = 0;
         return true;

      case TYPE_NULL:
      default:
         break;
      }

   var = 0;
   return false;
   }


int VDataObj::Find( int col, VData &value, int startRecord /*=-1*/ )
   {
   if ( startRecord < 0 )
      startRecord = 0;

   for ( int i=startRecord; i < this->GetRowCount(); i++ )
      {
      VData &v = Get( col, i );

      if ( v.Compare( value ) )
         return i;
      }

   return -1;
   }
    

//-- VDataObj::Clear() ----------------------------------------------
//
//-- resets the data object, clearing out the data and labels
//-------------------------------------------------------------------

void VDataObj::Clear( void )
   {
   DataObj::Clear();

   matrix.Clear();
   }


//-- VDataObj::SetSize() --------------------------------------------
//
//-- resets the dimensions of the data object
//
//-- NOTE: doesn't fix up labels array!!!!
//-------------------------------------------------------------------

bool VDataObj::SetSize( int _cols, int _rows )
   {
	matrix.Resize( _rows, _cols );   // actual data

	m_dataCols = _cols;
   return TRUE;
   }


//-- VDataObj::AppendRow() ------------------------------------------
//
//   Appends a row of data to the data object.
//-------------------------------------------------------------------

int VDataObj::AppendRow( COleVariant *varArray, int length )
   {
   int cols = (int) GetColCount();
   ASSERT( cols == length );

   if ( cols != length )
      return -1;

   VData *values = new VData[ cols ];

   // convert variants
   for ( int i=0; i < cols; i++ )
      values[ i ].ConvertFromVariant( varArray[ i ] );

   //-- Append data.
   int stat = matrix.AppendRow( values, length );

   delete [] values;

   return stat;
   }

int VDataObj::AppendRow(VData *values, int length)
{ 
	return matrix.AppendRow(values, length); 
}

int VDataObj::AppendRow( CArray< VData, VData > &values ) 
{ 
	return matrix.AppendRow( values.GetData(), (int) values.GetSize() ); 
}

int VDataObj::AppendRows( int count ) { 
	return (int) matrix.AppendRows( (UINT) count ); 
}


bool VDataObj::Set( int col, int row, COleVariant &value )
   {
   VData v;
   if ( v.ConvertFromVariant( value ) )
      {
      matrix.Set( row, col, v );
      return true;
      }
   else
      return false;
	}


bool VDataObj::Set( int col, int row, float value )
   {
   VData v( value );
   matrix.Set( row, col, v );

   return true;
   }

bool VDataObj::Set( int col, int row, double value )
   {
   VData v( value );
   matrix.Set( row, col, v );

   return true;
   }

bool VDataObj::Set( int col, int row, int value )
   {
   VData v( value );
   matrix.Set( row, col, v );

   return true;
   }



float VDataObj::GetAsFloat( int col, int row )
   {
   //if ( col > matrix.GetColCount()-1 )
   //   ErrorMsg( "Col out of bounds in VDataObj::GetAsFloat()" );

   COleVariant var;
   Get( col, row, var );
   var.ChangeType( VT_R4 );
   return var.fltVal;
   }

double VDataObj::GetAsDouble( int  col, int row)
   {
   COleVariant var;
   Get( col, row, var );
   var.ChangeType( VT_R8 );
   return var.dblVal;
   }


int VDataObj::GetAsInt( int col, int row )
   {
   //if ( col > matrix.GetColCount()-1 )
   //   ErrorMsg( "Col out of bounds in VDataObj::GetAsInt()" );

   COleVariant var;
   Get( col, row, var );
   var.ChangeType( VT_I4 );
   return var.lVal;
   }

bool VDataObj::GetAsBool(int col, int row)
   {
   //if ( col > matrix.GetColCount()-1 )
   //   ErrorMsg( "Col out of bounds in VDataObj::GetAsInt()" );
   bool var;
   bool ok = Get(col, row, var);

   if ( !ok )
      return false;

   return var;
   }


UINT VDataObj::GetAsUInt( int col, int row )
   {
   //if ( col > matrix.GetColCount()-1 )
   //   ErrorMsg( "Col out of bounds in VDataObj::GetAsInt()" );

   COleVariant var;
   Get( col, row, var );
   var.ChangeType( VT_UI4 );
   return var.lVal;
   }


CString VDataObj::GetAsString( int col, int row )
   {
   //if ( col > matrix.GetColCount()-1 )
   //   {
   //   CString msg;
   //   msg.Format( "Col out of bounds in VDataObj::GetAsString() (%i of %i)", col, (int) matrix.GetColCount() );
   //   ErrorMsg( msg );
    //  }

   //COleVariant v;
   //Get( col, row, v );
   //return CCrack::strVARIANT( v );
   
   VData &v = Get( col, row );
   return v.GetAsString();
   }




//-- VDataObj::ReadAscii() ------------------------------------------
//
//--  Opens and loads the file into the data object
//-------------------------------------------------------------------

int VDataObj::ReadAscii( LPCTSTR _fileName, TCHAR delimiter, BOOL showMsg )
   {
   CString fileName;
   PathManager::FindPath( _fileName, fileName );

#ifndef NO_MFC
   HANDLE hFile = hFile = CreateFile(fileName, 
      GENERIC_READ, // open for reading
      0, // do not share
      NULL, // no security
      OPEN_EXISTING, // existing file only
      FILE_ATTRIBUTE_NORMAL, // normal file
      NULL); // no attr. template

   //FILE *fp;
   //fopen_s( &fp, fileName, "rt" );     // open for "read"

#else
   FILE* hFile=fopen(fileName,"r");
#endif


   if ( hFile == INVALID_HANDLE_VALUE && showMsg )
//   if ( fp == NULL && showMsg )
      {
      CString msg("VDataObj: Couldn't find file " );
      msg += _fileName;
      Report::WarningMsg( msg );
      return 0;
      }

   m_path = fileName;

   if ( this->m_name.IsEmpty() )
      this->m_name = fileName;

   return _ReadAscii( hFile, delimiter, showMsg );
   }


//-------------------------------------------------------------------

int VDataObj::_ReadAscii( HANDLE hFile, TCHAR delimiter, BOOL showMsg )
   {
   Clear();   // reset data object, strings

#ifndef NO_MFC
   LARGE_INTEGER _fileSize;
   BOOL ok = GetFileSizeEx( hFile, &_fileSize );
   ASSERT( ok );

   DWORD fileSize = (DWORD) _fileSize.LowPart;

   //long fileSize = _filelength ( _fileno( fp ) );
   
   TCHAR *buffer  = new TCHAR[ fileSize+2 ];
   memset( buffer, 0, (fileSize+2) * sizeof( TCHAR ) );

   //fread ( buffer, fileSize, 1, fp );
   //fclose( fp );
   DWORD bytesRead = 0;
   ReadFile( hFile, buffer, fileSize, &bytesRead, NULL );
   CloseHandle( hFile );
#else
   //this is linux specific; rewrite for other desired systems
   long f_d=fileno((FILE*)hFile);
   struct stat st;
   fstat(f_d,&st);
   off_t fileSize=st.st_size;
   TCHAR *buffer=new TCHAR[fileSize+2];
   size_t bytesRead=fread(buffer,sizeof(TCHAR),fileSize,(FILE*)hFile)*sizeof(TCHAR);
   fclose((FILE*)hFile);
#endif


   if ( bytesRead == 0 )
      {
      delete [] buffer;
      return 0;
      }

   buffer[ fileSize ] = NULL;

   //-- skip any leading comments --//
   TCHAR *p = buffer;
   while ( *p == ';' )
      {
      p = strchr( p, '\n' );
      p++;
      }

   //-- start parsing buffer --//
   TCHAR *end = strchr( p, '\n' );

   if ( *(end-1) == 13 )  // \r
      end--;

   //-- NULL-terminate then get column count --//
   *end    = NULL;
   int  _cols = 1;

   // if delimiter == NULL, then autodetect by scanning the frist cset of charactores
   if ( delimiter == NULL )
      {
      //int testCount  = 0;
      int tabCount   = 0;
      int commaCount = 0;
      int spaceCount = 0;

      TCHAR *test = buffer;

      while ( *test != NULL && *test != '\n' ) // && testCount < 240 )
         {
         switch( *test )
            {
            case '\t':  tabCount++;    break;
            case ',':   commaCount++;  break;
            case ' ':   spaceCount++;  break;
            }
         
         test++;
         //testCount++;
         }

      delimiter = ',';

      if ( ( tabCount > commaCount ) && ( tabCount > spaceCount ) )
         delimiter = '\t';
      else if ( ( spaceCount > tabCount ) && ( spaceCount > commaCount ) )
         delimiter = ' ';
      }

   // count the nuber of delimiters
   while ( *p != NULL )
      {
      if ( *p++ == delimiter )
         _cols++;
      }

   SetSize( _cols, 0 );  // resize matrix, statArray

   //-- set labels from first line (labels are delimited) --//
   TCHAR d[ 3 ];
   d[ 0 ] = delimiter;
   d[ 1 ] = '\n';
   d[ 2 ] = NULL;
   TCHAR *next;

   p = strtok_s( buffer, d, &next );
   _cols = 0;

   while ( p != NULL )
      {
      while ( *p == ' ' ) p++;   // strip leading blanks

      SetLabel( _cols++, p );
      p = strtok_s( NULL, d, &next );   // look for delimiter or newline
      }

   //-- ready to start parsing data --//
   int cols = GetColCount();

   VData *data = new VData[ cols ];
   memset( data, 0, cols * sizeof( VData ) );

   // reset ptr to next line
   p = end+1;

   // strip any leading newlines
   while ( *p == '\r' || *p == '\n' )
      p++;

   INT_PTR rowcount = 0;

   float bytesPerRow = float(  cols * sizeof(VData) );
   float rAI = bytesRead / (10.0f * bytesPerRow);
   UINT rowAllocIncr = (UINT)rAI;

   if ((int)rowAllocIncr < 50)
      rowAllocIncr = 50;

   matrix.SetRowAllocationIncr(rowAllocIncr);

   while ( p != NULL && *p != EOF && *p != NULL)
      {
      // get a row of data
      for ( int i=0; i < cols; i++ )
         {
         // p is the current position pointer.  Before parsing, find the end of the string and NULL it out
         if ( *p == delimiter )  // nothing to parse?
            *p = NULL;
         else
            {
            next = p+1;
            while ( *next != delimiter 
                 && *next != '\r' 
                 && *next != '\n' 
                 && *next != NULL )
               next++;

            // NULL out the delimiter
            *next = NULL;
            }

         // at the start of this loop, p should point to the current token,
         // and next should point to the first char past the end of the current token
         data[ i ].Parse( p );
         //if ( i == 0 )
         //   TRACE1( "%s\n", p );
         //else
         //   TRACE1( "---%s\n", p );
         
         // move to the next token
         p = next+1;
         }

      AppendRow( data, cols );
      rowcount++;

      if (*p == NULL)
         break;

      p++;
      }

   //-- clean up --//
   delete [] data;
   delete [] buffer;

   return GetRowCount();
   }


//-- VDataObj::WriteAscii() ------------------------------------------
//
//-- Saves the data object to an ascii file
//-------------------------------------------------------------------

int VDataObj::WriteAscii( LPCTSTR fileName, TCHAR delimiter, int colWidth )
   {
   int *colWidthArray = NULL;  // store column widths if colWidth == 0

   if ( _tcschr( fileName, '\\' ) != NULL || _tcschr( fileName, '/' != NULL  ) )
      {
      nsPath::CPath path( fileName );
      path.RemoveFileSpec();
#ifndef NO_MFC
      SHCreateDirectoryEx( NULL, path, NULL );
#else
      mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
      }

   FILE *fp;
   fopen_s( &fp, fileName, "wt" );     // open for writing

   if ( fp == NULL )
      {
      CString msg( "VDataObj: Couldn't open file " );
      msg += fileName;
      Report::WarningMsg( msg );
      return 0;
      }

   int cols = GetColCount();
   int rows = GetRowCount();
   int col  = 0;
   int row  = 0;

   if ( colWidth == 0 )
      colWidthArray = new int[ cols ];
        
   TCHAR buffer[ 256 ];
   
   //-- put out labels, comma-delimited
   for ( col=0; col < cols-1; col++ )
      {
      lstrcpy( buffer, GetLabel( col ) );

      // strip out delimiter from names
      TCHAR *p = buffer;
      while (*p != NULL)
         {
         if (*p == delimiter)
            *p = ' ';
         ++p;
         }

      fprintf( fp, "%s%c", buffer, delimiter );

      if ( colWidth == 0 )
         colWidthArray[ col ] = lstrlen( GetLabel( col ) ) + 1;
      }

   //-- last label (no delimiter)
   lstrcpy( buffer, GetLabel( cols-1 ) );

   // strip out delimiter from names
   TCHAR *p = buffer;
   while (*p != NULL)
      {
      if (*p == delimiter)
         *p = ' ';
      ++p;
      }

   fprintf( fp, "%s\n", buffer );

   if ( colWidth == 0 )
      colWidthArray[ cols-1 ] = lstrlen( GetLabel( cols-1 ) ) + 1;

   //-- now cycle through rows, putting out actual data
   for ( row=0; row < rows; row++ )
      {
      int _colWidth = colWidth;

      for ( col=0; col < cols; col++ )
         {
         if ( colWidth == 0 )  _colWidth = colWidthArray[ col ];

         VData v;
         this->Get( col, row, v );

         //if ( row == 0 && v.GetType() == TYPE_PDATAOBJ )
         //   {
         //   DataObj *pDataObj = v.val.vPtrDataObj;
         //   ASSERT( pDataObj != NULL );
         //
         //   // basic idea - if the value is a data obj, write it separately
         //   // based on filename + column label
         //   CPath doFilename( fileName );
         //   doFilename.RemoveExtension();
         //
         //   CString _doFilename( doFilename );
         //   _doFilename += "_" ;
         //
         //   LPTSTR m_name = (LPTSTR) pDataObj->GetName();
         //   if ( m_name == NULL || *m_name == NULL || *m_name == ' ' )
         //      _doFilename += "unnamed";
         //   else
         //      {
         //      CleanFileName(m_name);
         //      _doFilename += m_name;
         //      }
         //
         //   _doFilename += ".csv";
         //
         //   pDataObj->WriteAscii( _doFilename, delimiter, colWidth );
         //   }

         fputs( (LPCTSTR) GetAsString( col, row ), fp );
         
         if ( col < cols-1 )
            fputc( delimiter, fp ); 
         else
            fputc( '\n', fp ); //;
         //fprintf( fp, "%s%c ", (LPCTSTR) GetAsString( col, row ), delimiter );
         //fprintf( fp, "%*g%c ", _colWidth, Get( col, row ), delimiter );
         }

      //if ( colWidth == 0 ) _colWidth = colWidthArray[ cols-1 ];

      //fprintf( fp, "%*s\n", _colWidth, (LPCTSTR) GetAsString( cols-1, row ) );
      //fprintf( fp, "%s\n", (LPCTSTR) GetAsString( cols-1, row ) );
      }

   if ( colWidthArray )
      delete [] colWidthArray;

   fclose( fp );
   return rows;
   }


//-- VDataObj::GetMinMax() ------------------------------------------
//
//-- Retrieves the min, max values of a col
//-------------------------------------------------------------------

bool VDataObj::GetMinMax( int col, float *minimum, float *maximum, int startRow )
   {   
   *minimum = 0;
   *maximum = 0;

   int rows = GetRowCount();

   if ( rows <= 0 || (int) startRow >= rows || col >= GetColCount() )
      return FALSE;

   *minimum = GetAsFloat( col, startRow );
   *maximum = GetAsFloat( col, startRow );

   for ( int i=startRow; i < rows; i++)
      {
      if ( *minimum > GetAsFloat( col, i ) )
         *minimum = GetAsFloat( col, i );

      if ( *maximum < GetAsFloat( col, i ) )
         *maximum = GetAsFloat( col, i );
      }

   return TRUE;
   }


//-------------------------------------------------------------------
float VDataObj::GetMean( int col, int startRow )
   {
   int rows = GetRowCount();

   if ( rows <= 0 || (int) startRow >= rows )
      return 0;

   float mean = 0.0;

   for ( int i=startRow; i < rows; i++ )
       mean += GetAsFloat( col, i );

   return ( mean/(rows-startRow) );
   }


//-------------------------------------------------------------------
double VDataObj::GetStdDev( int col, int startRow )
   {
   int rows = GetRowCount();

   if ( rows < 2 || (int) startRow >= rows-1 )
      return 0;

   double sumSquares = 0;

   for ( int i=startRow; i < rows; i++ )
      {
      float value = GetAsFloat( col, i );
      sumSquares += value * value;
      }

	return sqrt( (sumSquares/(rows-1-startRow)) );
	}


float VDataObj::GetSum( int col, int startRow )
   {
   int rows = GetRowCount();

   if ( rows < 2 || (int) startRow >= rows-1 )
      return 0;

   float sum = 0;

   for ( int i=startRow; i < rows; i++ )
      {
      float value = GetAsFloat( col, i );
      sum += value;
      }

	return sum;
	}



int VDataObj::AppendCols( int count )
   {
   VData *dataArray =  new VData[ GetRowCount() ];

   int colCount = GetColCount();

   matrix.AppendCol( dataArray, count );

   for ( int i=0; i < count; i++ )
      {
      CString label;
      label.Format( "Col_%i", i+colCount-1 );
      AddLabel( label );
      }

   delete [] dataArray;

   return GetColCount()-1;
   }



int VDataObj::AppendCol( LPCTSTR label ) //, VData *dataArray /*=NULL*/ )
   {
   VData *dataArray = new VData[ GetRowCount() ];

   matrix.AppendCol( dataArray );
   delete[] dataArray;

   AddLabel( label );

   m_dataCols++;

   return GetColCount()-1;
   }


int VDataObj::InsertCol( int insertBefore, LPCTSTR label )//, VData *dataArray /*=NULL*/ )
   {
   VData *dataArray = new VData[ GetRowCount() ];

   int col = matrix.InsertCol( insertBefore, dataArray );

   // take care of labels
   AddLabel( "" );

   for ( int i=GetColCount()-1; i > col; i-- )
      SetLabel( i, GetLabel( i-1 ) );

   SetLabel( col, label );

   delete [] dataArray;

   m_dataCols++;

   return col;
   }


bool VDataObj::CopyCol( int toCol, int fromCol )
   {
   if ( toCol < 0 || toCol >= this->GetColCount() )
      return false;

   if ( fromCol < 0 || fromCol >= this->GetColCount() )
      return false;

   VData v;
   for ( int i=0; i < this->GetRowCount(); i++ )
      {
      this->Get( fromCol, i, v );
      this->Set( toCol, i, v );
      }

   return true;
   }


int VDataObj::ReadDBF( LPCTSTR databaseName )
   {
	DBFHandle h = DBFOpen(databaseName, "rb");  // jpb - removed '+'

	if ( h == NULL )
	   {
	   CString msg( _T("Error opening database " ) );
      msg += databaseName;
      msg += _T("... it may not be present, you may not have access, it may be locked, or may be read only" );
		Report::ErrorMsg( msg, _T("File Error") );
      return -1;
      }

   int cols = DBFGetFieldCount(h);

   this->SetSize( cols, 0 );
   
   // set field structure
   CArray< DBFFieldType, DBFFieldType > ftArray;
   for ( int col=0; col < cols; col++ )
      {
	   char fname[16];
      fname[ 0 ] = NULL;

	   int width, decimals;
	   DBFFieldType ftype = DBFGetFieldInfo(h, col, fname, &width, &decimals);

      ftArray.Add( ftype );

      if ( lstrlen( fname ) == 0 || fname[0] == ' ' )
         lstrcpy( fname, "_UNNAMED_" );

      this->AddLabel( fname );
   
      //switch ( ftype )
      //   {
      //   case FTLogical:   fi.type = TYPE_BOOL;    break;
      //   case FTInteger:   fi.type = TYPE_LONG;    break;
      //   case FTDouble:    fi.type = TYPE_DOUBLE ; break;
      //   case FTString:    fi.type = TYPE_DSTRING; break;
      //   //case dbDate   Date/Time; see MFC class COleDateTime
      //   //dbLongBinary   Long Binary (OLE Object); you might want to use MFC class CByteArray instead of class CLongBinary as CByteArray is richer and easier to use.
      //   //dbMemo   Memo; see MFC class CString
      //   default:
      //      Report::WarningMsg( "Unsupported type in MapLayer::LoadDataDBF()" );
      //   }

      //fi.width = width;
      //fi.decimals = decimals;
      }

   COleVariant *varArray = new COleVariant[ cols ];

   int count = 0;
   int dbRecordCount = DBFGetRecordCount( h );

   for ( int i = 0; i < dbRecordCount; i++ )
      {
//	  if( DBFIsRecordDeleted(h) )
//		continue;
      for ( int col=0; col < cols; col++ )
         {
         COleVariant val;

         switch( ftArray[ col ] )
            {
      //   case FTLogical:   fi.type = TYPE_BOOL;    break;
      //   case FTInteger:   fi.type = TYPE_LONG;    break;
      //   case FTDouble:    fi.type = TYPE_DOUBLE ; break;
      //   case FTString:    fi.type = TYPE_DSTRING; break;
            case FTLogical:
               {
               val.ChangeType( VT_BOOL );
               const char *p = DBFReadStringAttribute(h, i, col);
               if ( *p == 'T' || *p == 't' || *p == 'Y' || *p== 'y' )
                  val.boolVal = (VARIANT_BOOL) 1;
               else
                  val.boolVal = (VARIANT_BOOL) 0;
               }
               break;

            case FTInteger:
               val = (long) DBFReadIntegerAttribute(h, i, col); 
               break;

            case FTDouble:
               val = (float) ((double) DBFReadDoubleAttribute(h, i, col)); 
               break;

            case FTString:
               val = DBFReadStringAttribute(h, i, col); 
               break;

            default:
               Report::WarningMsg( "Unsupported type in VDataObj::ReadDBF()" );
            }

         varArray[ col ] = val;
         }

      this->AppendRow( varArray, cols );
 		count++;
		}

   delete [] varArray;

   DBFClose(h);

   return count;
   }
