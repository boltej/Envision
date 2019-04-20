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
//  IDATAOBJ.CPP
//
//-- code for the global data object
//-------------------------------------------------------------------

#include "EnvLibs.h"
#pragma hdrstop

#include "Report.h"
#include "Path.h"
#include "PathManager.h"
#include "IDATAOBJ.H"

#ifndef NO_MFC
#include <io.h>
#endif
#include <climits>
#include <cmath>
#include <cstring>



//-- IDataObj::FDataObj() ---------------------------------------------
//
//-- constructors
//-------------------------------------------------------------------

IDataObj::IDataObj(UNIT_MEASURE m)
   : DataObj(m),
     matrix  ()
   { }


//-------------------------------------------------------------------
IDataObj::IDataObj( IDataObj &dataObj)
   : DataObj( dataObj),
     matrix  ( dataObj.matrix )
   { }


//-------------------------------------------------------------------
IDataObj::IDataObj( int _cols, int _rows, UNIT_MEASURE m)
   : DataObj( _cols, _rows, m ),
     matrix  ( _rows, _cols )
   { }


IDataObj::IDataObj( int _cols, int _rows, int initValue, UNIT_MEASURE m)
   : DataObj( _cols, _rows, m ),
     matrix  ( _rows, _cols, initValue )
   { }


//-- IDataObj::IGet() ------------------------------------------------
//
//-- interpolating Get() method.  Assumes X data is in col 0, Y data
//   is sorted in ascending order
//-------------------------------------------------------------------
/*
float IDataObj::IGet( float x, int col, IMETHOD method )
   {
   float x1;
   int rows = GetRowCount();
   int row = 0;

   //-- find appropriate row
   for ( row=0; row < rows; row++ )
      {
      x1 = Get( 0, row );

      //-- have we found or passed the correct row?
      if ( x1 >= x )
         break;
      }

	//-- x occured before first row?
   //-- return first element (doesn't wrap)
   if ( row == 0 )
      return Get( col, 0 );

   //-- x occurred after last row (never found row)?
   //-- return last element (doesn't wrap)
   if ( row == rows )
      return Get( col, rows-1 );

   //-- was the found row an exact match?
   if ( x1 == x )
      return Get( col, row );

   //-- not an exact match, interpolate between found and previous rows
   float x0 = Get( 0,   row-1 );
   float y0 = Get( col, row-1 );
   float y1 = Get( col, row   );
   float y;

   switch( method )
      {
      case IM_QUADRATIC:
         //-- quadratic not implemented --//

      case IM_LINEAR:
      default:
         y = y1 - (x1 - x) * (y1 - y0) / (x1 - x0 ) ;
      }

   return y;
   }

*/

int IDataObj::Find( int col, VData &value, int startRecord /*=-1*/ )
   {
   if (startRecord < 0 )
      startRecord = 0;

   for ( int i=startRecord; i < this->GetRowCount(); i++ )
      {
      int v = Get( col, i );
      int ivalue;
      value.GetAsInt( ivalue );

      if ( v == ivalue )
         return i;
      }

   return -1;
   }


//-- IDataObj::Clear() ----------------------------------------------
//
//-- resets the data object, clearing out the data and labels
//-------------------------------------------------------------------

void IDataObj::Clear( void )
   {
   DataObj::Clear();  // clear parent

   matrix.Clear();
   }


//-- IDataObj::SetSize() --------------------------------------------
//
//-- resets the dimensions of the data object
//
//-- NOTE: doesn't fix up labels array, stats array!!!!
//-------------------------------------------------------------------

bool IDataObj::SetSize( int _cols, int _rows )
   {
	matrix.Resize( _rows, _cols );   // actual data

   return TRUE;
   }


//-- IDataObj::AppendRow() ------------------------------------------
//
//   Appends a row of data to the data object.
//-------------------------------------------------------------------

int IDataObj::AppendRow( COleVariant *varArray, int length )
   {
   int cols = (int) GetColCount();
   ASSERT( cols == length );

   if ( cols != length )
      return -1;

   int *values = new int[ cols ];

   // convert variants
   for ( int i=0; i < cols; i++ )
      {
      varArray[ i ].ChangeType( VT_I4 );
      values[ i ] = varArray[ i ].intVal;
      }

   //-- Append data.
   int stat = matrix.AppendRow( values, cols );

   delete [] values;

   return stat;
   }


int IDataObj::AppendRow( VData *varArray, int length )
   {
   int cols = (int) GetColCount();
   ASSERT( cols == length );

   if ( cols != length )
      return -1;

   int *values = new int[ cols ];

   // convert variants
   for ( int i=0; i < cols; i++ )
      {
      int value;
      varArray[ i ].GetAsInt( value );
      values[ i ] = value;
      }
      
   //-- Append data.
   int stat = matrix.AppendRow( values, length );

   delete [] values;

   return stat;
   }


int IDataObj::AppendRow( int *iArray, int length )
   {
   int cols = GetColCount();
   ASSERT( cols == length );

   if ( cols != length )
      return -1;

   //-- Append data.
   return matrix.AppendRow( iArray, length );
   }


//-- IDataObj::SetRow() ----------------------------------------------
//
//   Sets a row of data in the data object.
//-------------------------------------------------------------------

int IDataObj::SetRow( int *iArray, int setRow, int countCols )
   {
   int cols = GetColCount();
   int rows = GetRowCount();

   if ( setRow >= rows || countCols > cols  )
      return 0;

   //-- Set data.
   for ( int i = 0; i < countCols; i++ )
      {
      if ( matrix.Set( setRow, i, iArray[ i ] ) == 0 )
         return 0;
      }

   return 1;
   }



int IDataObj::AppendCols( int count )
   {
   int *dataArray =  new int[ GetRowCount() ];

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


int IDataObj::AppendCol( LPCTSTR label ) //, float *dataArray /*=NULL*/ )
   {
   int *dataArray = new int[ GetRowCount() ];

   for ( int i=0; i < GetRowCount(); i++ )
      dataArray[ i ] = 0;

   matrix.AppendCol( dataArray );
   delete[] dataArray;

   AddLabel( label );

   m_dataCols++;

   return GetColCount()-1;
   }


int IDataObj::InsertCol( int insertBefore, LPCTSTR label )
   {
   int *dataArray = new int[ GetRowCount() ];

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

bool IDataObj::CopyCol( int toCol, int fromCol )
   {
   if ( toCol < 0 || toCol >= this->GetColCount() )
      return false;

   if ( fromCol < 0 || fromCol >= this->GetColCount() )
      return false;

   for ( int i=0; i < this->GetRowCount(); i++ )
      this->Set( toCol, i, this->GetAsInt( fromCol, i ) );

   return true;
   }




//-- IDataObj::ReadAscii() ------------------------------------------
//
//--  Opens and loads the file into the data object
//-------------------------------------------------------------------

int IDataObj::ReadAscii( LPCTSTR _fileName, TCHAR delimeter, BOOL showMsg )
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
   FILE* hFile=fopen(_fileName,"r");
#endif

   if ( hFile == INVALID_HANDLE_VALUE && showMsg )
//   if ( fp == NULL && showMsg )
      {
      TCHAR buffer[ 128 ];
      strcpy_s( buffer, 128, "VDataObj: Couldn't find file " );
      strcat_s( buffer, 128 - strlen(buffer), _fileName);
      Report::WarningMsg( buffer );
      return 0;
      }

   m_path = fileName;

   if ( this->m_name.IsEmpty() )
      this->m_name = fileName;

   return ReadAscii( hFile, delimeter, showMsg );
   }


//-------------------------------------------------------------------

int IDataObj::ReadAscii( HANDLE hFile, TCHAR delimiter, BOOL showMsg )
   {
   Clear();   // reset data object, strings

#ifndef NO_MFC
   LARGE_INTEGER _fileSize;
   BOOL ok = GetFileSizeEx( hFile, &_fileSize );
   ASSERT( ok );

   DWORD fileSize = (DWORD) _fileSize.LowPart;

   //long fileSize = _filelength ( _fileno( fp ) );
   
   TCHAR *buffer  = new TCHAR[ fileSize+2 ];
   memset( buffer, 0, (fileSize+2)*sizeof( TCHAR ) );

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

   //-- skip any leading comments --//
   TCHAR *p = buffer;
   while ( *p == ';' )
      {
      p = strchr( p, '\n' );
      p++;
      }

   //-- start parsing buffer --//
   TCHAR *end = strchr( p, '\n' );

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

      if ( tabCount < commaCount && tabCount < spaceCount )
         delimiter = '\t';
      else if ( spaceCount < tabCount && spaceCount < commaCount )
         delimiter = ' ';
      }

   // count the nuber of delimiters


   while ( *p != NULL )
      {
      if ( *p++ == delimiter )    // count the number of delimiters
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
      p = strtok_s( NULL, d, &next );   // look for delimeter or newline
      }

   //-- ready to start parsing data --//
   int cols = GetColCount();

   int *data = new int[ cols ];

   int bytesPerRow = cols * sizeof(int);
   float rAI = bytesRead / (10.0f * bytesPerRow);
   UINT rowAllocIncr = (UINT)rAI;

   if ((int)rowAllocIncr < 50)
      rowAllocIncr = 50;

   matrix.SetRowAllocationIncr(rowAllocIncr);

   //d[ 1 ] = NULL;
   p = strtok_s( end+1, d, &next );  // look for delimiter

   while ( p != NULL )
      {
      //-- get a row of data --//
      for ( int i=0; i < cols; i++ )
         {
         data[ i ] = atoi( p ) ;
         p = strtok_s( NULL, d, &next );  // look for delimeter

         //-- invalid line? --//
         if ( p == NULL && i < cols-1 )
            goto finish;
         }

      AppendRow( data, _cols );
      }

   //-- go to
finish:

   //-- clean up --//
   delete[] data;
   delete[] buffer;

   return GetRowCount();
   }


//-- IDataObj::WriteAscii() ------------------------------------------
//
//-- Saves the data object to an ascii file
//-------------------------------------------------------------------

int IDataObj::WriteAscii( LPCTSTR fileName, TCHAR delimiter, int colWidth )
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
      TCHAR buffer[ 128 ];
      strcpy_s( buffer, 128, "DataObj: Couldn't open file " );
      strcat_s( buffer, 128, fileName );
      Report::WarningMsg( buffer );
      return 0;
      }

   int cols = GetColCount();
   int rows = GetRowCount();
   int col  = 0;
   int row  = 0;

   if ( colWidth == 0 )
      colWidthArray = new int[ cols ];

   //-- put out labels, comma-delimited
   for ( col=0; col < cols-1; col++ )
      {
      fprintf( fp, "%s%c ", GetLabel( col ), delimiter );

      if ( colWidth == 0 )
         colWidthArray[ col ] = lstrlen( GetLabel( col ) ) + 1;
      }

   //-- last label (no delimiter)
   fprintf( fp, "%s\n", GetLabel( cols-1 ) );

   if ( colWidth == 0 )
      colWidthArray[ cols-1 ] = lstrlen( GetLabel( cols-1 ) ) + 1;

   //-- now cycle through rows, putting out actual data
   for ( row=0; row < rows; row++ )
      {
      int _colWidth = colWidth;

      for ( col=0; col < cols-1; col++ )
         {
         if ( colWidth == 0 )  _colWidth = colWidthArray[ col ];

         fprintf( fp, "%i%c ", Get( col, row ), delimiter );
         }

      if ( colWidth == 0 ) _colWidth = colWidthArray[ col ];

      fprintf( fp, "%*i\n", _colWidth, Get( col, row ) );
      }

   if ( colWidthArray )
      delete [] colWidthArray;

   fclose( fp );
   return rows;
   }


//-- IDataObj::GetMinMax() ------------------------------------------
//
//-- Retrieves the min, max values of a col
//-------------------------------------------------------------------

bool IDataObj::GetMinMax( int col, float *minimum, float *maximum, int startRow )
   {
   //if ( col >= statArray.Length() )
   //   return FALSE;
   //
   //else
   //   {
   //   *minimum = statArray[ col ].min;
   //   *maximum = statArray[ col ].max;
   //   return TRUE;
   //   }

   *minimum = 0;
   *maximum = 0;

   int rows = GetRowCount();

   if ( rows <= 0 || (int) startRow >= rows || col >= GetColCount() )
      return FALSE;

   *minimum = (float) Get( col, startRow );
   *maximum = (float) Get( col, startRow );

   for ( int i=startRow; i < rows; i++)
      {
      if ( *minimum > (float) Get( col, i ) )
         *minimum = (float) Get( col, i );

      if ( *maximum < (float) Get( col, i ) )
         *maximum = (float) Get( col, i );
      }

   return TRUE;
   }


//-------------------------------------------------------------------
float IDataObj::GetMean( int col, int startRow )
   {
   int rows = GetRowCount();

   if ( rows <= 0 || (int) startRow >= rows )
      return 0;

   float mean = 0.0;

   for ( int i=startRow; i < rows; i++ )
       mean += Get( col, i );

   return ( mean/(rows-startRow) );
   }


//-------------------------------------------------------------------
double IDataObj::GetStdDev( int col, int startRow )
   {
   int rows = GetRowCount();

   if ( rows < 2 || (int) startRow >= rows-1 )
      return 0;

   double sumSquares = 0;

   for ( int i=startRow; i < rows; i++ )
      {
      float value = (float) Get( col, i );
      sumSquares += value * value;
      }

	return sqrt( (sumSquares/(rows-1-startRow)) );
	}

