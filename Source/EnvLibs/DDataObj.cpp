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
//  DDataObj.CPP
//
//-- code for the global data object
//-------------------------------------------------------------------

#include "EnvLibs.h"
#pragma hdrstop

#include "DDataObj.h"

#ifndef NO_MFC
#include <io.h>
#endif
#include <limits.h>
#include <math.h>
#include <string.h>

#include "Report.h"
#include "DBTABLE.H"
#include "Path.h"
#include "PathManager.h"



//-- DDataObj::DDataObj() ---------------------------------------------
//
//-- constructors
//-------------------------------------------------------------------

DDataObj::DDataObj()
   : DataObj(U_UNDEFINED),
   matrix(),
   statArray()
   {
   }


DDataObj::DDataObj(UNIT_MEASURE m)
   : DataObj(m),
     matrix  (),
     statArray()
   { }


//-------------------------------------------------------------------
DDataObj::DDataObj( DDataObj &dataObj )
   : DataObj( dataObj ),
     matrix  ( dataObj.matrix ),
     statArray  ( dataObj.statArray )
   { }


//-------------------------------------------------------------------
DDataObj::DDataObj( int _cols, int _rows, UNIT_MEASURE m)
   : DataObj( _cols, _rows,m ),
     matrix  ( _rows, _cols ),     
     statArray()
   {
   DO_STATS stat = { (float) LONG_MAX, (float) LONG_MIN, (float)0.0, (float)0.0, SF_NONE };
   statArray.ReFill( stat, _cols );

   return;
   }

DDataObj::DDataObj( int _cols, int _rows, double initialValue, UNIT_MEASURE m)
   : DataObj( _cols, _rows, m ),
     matrix  ( _rows, _cols, initialValue ),     
     statArray()
   {
   DO_STATS stat = { (float) LONG_MAX, (float) LONG_MIN, (float)0.0, (float)0.0, SF_NONE };
   statArray.ReFill( stat, _cols );

   return;
   }

DDataObj::DDataObj( const DoubleMatrix::type_data * data, int _cols, int _rows, UNIT_MEASURE m)
: DataObj( _cols, _rows, m ),
  matrix  ( data, _rows, _cols ),     
  statArray()
   {
   DO_STATS stat = { (float) LONG_MAX, (float) LONG_MIN, (float)0.0, (float)0.0, SF_NONE };
   statArray.ReFill( stat, _cols );

   return;
   }




//-- DDataObj::DDataObj() ---------------------------------------------
//
//-- stats constructor
//-------------------------------------------------------------------

DDataObj::DDataObj( int _cols, int _rows, STATSFLAG *statFlagArray, UNIT_MEASURE m)
   : DataObj( _cols, _rows, m ),
     matrix  ( _rows, _cols ),
     statArray()
   {
   int i = 0;

   //for ( i=0; i < m_dataCols; i++ )
   //   {
   //   if ( statFlagArray[ i ] & SF_STORE_MEAN )
   //      _cols++;
   //
	//	if ( statFlagArray[ i ] & SF_STORE_VARIANCE )
   //      _cols++;
   //   }

   //matrix.Resize( _rows, _cols );

   for ( i=0; i < _cols; i++ )
      {
      STATSFLAG statFlag = SF_NONE;

		if ( i < m_dataCols )
			statFlag = statFlagArray[ i ];

		DO_STATS stat =
			{ (float) LONG_MAX, (float) LONG_MIN, (float)0.0, float(0.0), statFlag };

		statArray.Append( stat );
		}
   }


double DDataObj::Get( int col, int row )
   {
   if ( col < 0 || col >= this->GetColCount() )
      {
      CString msg;
      msg.Format( "Invalid column index (%i) passed to DDataObj '%s' (cols=%i,rows=%i) when getting a value!",
         col, this->m_name, this->GetColCount(), this->GetRowCount() );
      Report::ErrorMsg( msg );
      return 0;
      }
         
   if ( row < 0 || row >= this->GetRowCount() )
      {
      CString msg;
      msg.Format( "Invalid row index (%i) passed to DDataObj '%s' (cols=%i,rows=%i) when getting a value!",
         col, this->m_name, this->GetColCount(), this->GetRowCount() );
      Report::ErrorMsg( msg );
      return 0;
      }
         
   return matrix.Get( row, col );
   }


//This version of IGet does not assume that the X data is in column 0.  
//Column with X data is specified. Column with Y data is specified
// 
// Note that there are a few cases :
// 
// 1) ‘x’ is “before” all data – leftRowIndex = -1, rightRowIndex = 0;
// 2) ‘x’ is “after” all data – leftRowIndex = #rows - 1, rightRowIndex = -1
// 3) ‘x’ is exactly on a row – leftRowIndex = rightRowIndex = row
// 4) ‘x’ is between two rows : leftRowIndex = first row “before” x, rightRowIndex = first row “after” x

double DDataObj::IGet( double x, int xcol, int ycol, IMETHOD method, int *startRow/*=NULL*/, int *leftRowIndex /*=NULL*/, int *rightRowIndex /*=NULL*/)
   {
   double x1;
   int rows = GetRowCount();
   int row = 0;

   if ( startRow != NULL )
      row = *startRow;

   //-- find appropriate row
   for ( row=0; row < rows; row++ )
      {
      x1 = Get( xcol, row );
      
      if ( startRow != NULL )
         *startRow = row;

      //-- have we found or passed the correct row?
      if ( x1 >= x )
         {
         if (leftRowIndex != NULL)
            *leftRowIndex = row - 1;
         if (rightRowIndex != NULL)
            *rightRowIndex = row;
         break;
         }
      }

	//-- x occured before first row?
   //-- return first element (doesn't wrap)
   if ( row == 0 )
      return Get( ycol, 0 );

   //-- x occurred after last row (never found row)?
   //-- return last element (doesn't wrap)
   if ( row == rows )
      {
      if (rightRowIndex != NULL)
         *rightRowIndex = -1;

      if ( startRow != NULL )
         *startRow = rows;

      return Get(ycol, rows-1);
      }

   //-- was the found row an exact match?
   if ( x1 == x )
      {
      if (leftRowIndex != NULL)
         *leftRowIndex = row;   // note:  rigthRowIndex has already been set

      return Get(ycol, row);
      }

   //-- not an exact match, interpolate between found and previous rows
   double x0 = Get( xcol, row-1 );
   double y0 = Get( ycol, row-1 );
   double y1 = Get( ycol, row   );
   double y=0;
   
   switch( method )
      {
      case IM_NONE:
         y = Get(ycol, row);
         break;

      case IM_CONSTANT_RATE:
         {
         double xfrac = (x - x0) / (x1 - x0);
         double yratio = y1 / y0;
         y = (double)(y0 * pow(yratio, xfrac));
         }
         break;


      case IM_QUADRATIC:
         //-- quadratic not implemented, fall through to linear

      case IM_LINEAR:
      default:
         y = y1 - (x1 - x) * (y1 - y0) / (x1 - x0 ) ;
      }

   return y;
   }

//This version of IGet can interpolate values from a 2D lookup table.   mmc 5/2/2012  
//Assume that x data is in col 0 and y data is in row 0.  Value returned is a lookup from both x and y, both values can be interpolated.

double DDataObj::IGet( double x, double y, IMETHOD method )
   {
   double xget;
   double yget;
   int x0row, x1row, y0col, y1col;
   int rows = GetRowCount();
   int cols = GetColCount();
   int row = 0;
   int col = 0;
   

   //-- find rows above (x0row) and below (x1row) the x value
   for ( row=1; row < rows; row++ )    //start at 1 because row 0 contains the y value lookup values
      {
      xget = Get( 0, row );

      //-- have we found or passed the correct row?
      if ( xget >= x )
         break;
      }
   
   x0row = row-1;
   x1row = row;

   //-- x occured before first row?
   //-- return first element (doesn't wrap)
   if ( row == 1 )
      {
	  x0row = 1;
      x1row = 2;
      }
   //-- x occurred after last row (never found row)?
   //-- return last element (doesn't wrap)
   if ( row == rows )
      {
	  x0row = rows-1;
      x1row = rows;
      }
   //-- was the found row an exact match?
   if ( xget == x )
      {
	  x0row = row;
      x1row = row+1;
      }
   
   //-- next, find cols above (y0col) and below (y1col) the y value
   for ( col=1; col < cols; col++ )   //start at col = 1 because col = 0 contains the x value look up values
      {
      yget = Get( col, 0 );

      //-- have we found or passed the correct col?
      if ( yget >= y )
         break;
      }
   
   y0col = col-1;
   y1col = col;

	//-- y occured before first col?
   //-- return first element (doesn't wrap)
   if ( col == 1 )
      {
	  y0col = 1;
      y1col = 2;
      }
   //-- x occurred after last col (never found row)?
   //-- return last element (doesn't wrap)
   if ( col == cols )
      {
      y0col = cols-1;
      y1col = cols;
      }
   //-- was the found col an exact match?
   if ( yget == y )
      {
	  y0col = col;
      y1col = col+1;
      }
   // Obtain interpolated y values for both the x0row and x1row
	  double y0 = Get(y0col,0);
	  double y1 = Get(y1col, 0);
	  
	  double y0_x0row = Get(y0col,x0row);
	  double y1_x0row = Get(y1col,x0row);
	  double yval_x0row = ((y-y0)/(y1-y0))*(y0_x0row - y1_x0row)+y1_x0row;

	  double y0_x1row = Get(y0col,x1row);
	  double y1_x1row = Get(y1col,x1row);
	  double yval_x1row = ((y-y0)/(y1-y0))*(y0_x1row - y1_x1row)+y1_x1row;

   //Proceed with standard interpolation using new y values (yval_x0row and yval_x1row)
   //-- not an exact match, interpolate between found and previous rows
   double x0 = Get( 0, x0row );
   double x1 = Get( 0, x1row );
   double val;
   
   switch( method )
      {
      case IM_QUADRATIC:
         //-- quadratic not implemented --//

      case IM_LINEAR:
      default:
         val = ((x-x0)/(x1-x0))*(yval_x1row - yval_x0row)+yval_x0row;
      }

   return val;
   }

  
//  This version of iGet will start looking at the offset record.  If increment is true, the method will return offset as the 
//  current record.  If it is false, the method will return the originally passed offset.
//double DDataObj::IGetInternal( double x, int col, IMETHOD method, int &offset, bool increment )
//   {
//   double x1;
//   int rows = GetRowCount();
//   int row = 0;
//
//   //-- find appropriate row
//   for ( row=offset; row < rows; row++ )
//      {
//      x1 = Get( 0, row );
//
//      //-- have we found or passed the correct row?
//      if ( x1 >= x )
//         break;
//      }
//   if (increment)   
//      offset=row;
//
//	//-- x occured before first row?
//   //-- return first element (doesn't wrap)
//   if ( row == 0 )
//      return Get( col, 0 );
//
//   //-- x occurred after last row (never found row)?
//   //-- return last element (doesn't wrap)
//   if ( row == rows )
//      return Get( col, rows-1 );
//
//   //-- was the found row an exact match?
//   if ( x1 == x )
//      return Get( col, row );
//   
//
//   //-- not an exact match, interpolate between found and previous rows
//   double x0 = Get( 0,   row-1 );
//   double y0 = Get( col, row-1 );
//   double y1 = Get( col, row   );
//   double y;
//
//   switch( method )
//      {
//      case IM_QUADRATIC:
//         //-- quadratic not implemented --//
//
//      case IM_LINEAR:
//      default:
//         y = y1 - (x1 - x) * (y1 - y0) / (x1 - x0 ) ;
//      }
//
//   return y;
//   }


int DDataObj::Find( int col, VData &value, int startRecord /*=-1*/ )
   {
   if (startRecord < 0 )
      startRecord = 0;

   for ( int i=startRecord; i < this->GetRowCount(); i++ )
      {
      double v = Get( col, i );
      double fvalue;
      value.GetAsDouble( fvalue );

      if ( v == fvalue )
         return i;
      }

   return -1;
   }





//-- DDataObj::Clear() ----------------------------------------------
//
//-- resets the data object, clearing out the data and labels
//-------------------------------------------------------------------

void DDataObj::Clear( void )
   {
   DataObj::Clear();  // clear parent

   matrix.Clear();
   statArray.Clear();
   }


//-- DDataObj::SetSize() --------------------------------------------
//
//-- resets the dimensions of the data object
//
//-- NOTE: doesn't fix up labels array, stats array!!!!
//-------------------------------------------------------------------

bool DDataObj::SetSize( int _cols, int _rows )
   {
	matrix.Resize( _rows, _cols );   // actual data

	m_dataCols = _cols;  // assumes no stats!!!

   DO_STATS stat = { (float) LONG_MAX, (float) LONG_MIN, (float)0.0, (float)0.0, SF_NONE };
	statArray.ReFill( stat, _cols );

   return TRUE;
   }


//-- DDataObj::AppendRow() ------------------------------------------
//
//   Appends a row of data to the data object.
//-------------------------------------------------------------------

int DDataObj::AppendRow( COleVariant *varArray, int length )
   {
   int cols = (int) GetColCount();
   ASSERT( cols == length );

   if ( cols != length )
      return -1;

   double *array = new double[ cols ];

   // convert variants
   for ( int i=0; i < cols; i++ )
      {
      varArray[ i ].ChangeType( VT_R4 );
      array[ i ] = varArray[ i ].fltVal;
      }

   //-- Append data.
   int stat = matrix.AppendRow( array, length );

   delete [] array;

   return stat;
   }


int DDataObj::AppendRow( VData *varArray, int length )
   {
   int cols = (int) GetColCount();
   ASSERT( cols == length );

   if ( cols != length )
      return -1;

   double *values = new double[ cols ];

   // convert variants
   for ( int i=0; i < cols; i++ )
      {
      double value;
      varArray[ i ].GetAsDouble( value );
      values[ i ] = value;
      }
      
   //-- Append data.
   int stat = matrix.AppendRow( values, length );

   delete [] values;

   return stat;
   }



int DDataObj::AppendRow( double *fArray, int length )
   {
   int cols = GetColCount();
   ASSERT( cols == length );

   if ( cols != length )
      return -1;

   double *_fArray;

   //-- Check stat columns.
   if ( m_dataCols != cols )
      {
      _fArray = new double[ cols ];
      memcpy( _fArray, fArray, m_dataCols * sizeof( double ) );
      }
   else
      _fArray = fArray;

   //-- Append data.
   int stat = matrix.AppendRow( _fArray, length );

	if ( m_dataCols != cols )
		delete [] _fArray;

   //only as needed
	////-- set mins/maxs
	//for ( int i=0; i < m_dataCols; i++ )
	//	{
	//	if ( fArray[ i ] < statArray[ i ].min )
	//		statArray[ i ].min = fArray[ i ];
   //
	//	if ( fArray[ i ] > statArray[ i ].max )
	//		statArray[ i ].max = fArray[ i ];
	//	}

   return stat;
   }



//-- DDataObj::SetRow() ----------------------------------------------
//
//   Sets a row of data in the data object.
//-------------------------------------------------------------------

int DDataObj::SetRow( double *fArray, int setRow, int countCols )
   {
   double *_fArray;
   int cols = GetColCount();
   int rows = GetRowCount();

   if ( setRow >= rows || countCols > cols  )
      return 0;

   //-- Check stat columns.
   if ( m_dataCols != cols )
      {
      _fArray = new double[ cols ];
      memcpy( _fArray, fArray, m_dataCols * sizeof( double ) );
      }
   else
      _fArray = fArray;

   //-- Set data.
   for ( int i = 0; i < countCols; i++ )
      {
      if ( matrix.Set( setRow, i, _fArray[ i ] ) == 0 )
         return 0;
      }

	if ( m_dataCols != cols )
		delete [] _fArray;

   //only as needed
	////-- set mins/maxs --//
	//for ( i = 0; i < m_dataCols; i++ )
	//	{
	//	if ( fArray[ i ] < statArray[ i ].min )
	//		statArray[ i ].min = fArray[ i ];
   //
	//	if ( fArray[ i ] > statArray[ i ].max )
	//		statArray[ i ].max = fArray[ i ];
	//	}

   return 1;
   }



int DDataObj::AppendCols( int count )
   {
   double *dataArray =  new double[ GetRowCount() ];

   int colCount = GetColCount();

   matrix.AppendCol( dataArray, count );

   for ( int i=0; i < count; i++ )
      {
      CString label;
      label.Format( "Col_%i", i+colCount-1 );
      AddLabel( label );
      }

   delete [] dataArray;

   m_dataCols++;

   return GetColCount()-1;
   }


int DDataObj::AppendCol( LPCTSTR label ) //, double *dataArray /*=NULL*/ )
   {
   double *dataArray = new double[ GetRowCount() ];

   for ( int i=0; i < GetRowCount(); i++ )
      dataArray[ i ] = 0.0f;

   matrix.AppendCol( dataArray );

   AddLabel( label );

   delete [] dataArray;
 
   m_dataCols++;
   
   return GetColCount()-1;
   }


int DDataObj::InsertCol( int insertBefore, LPCTSTR label )
   {
   bool alloc = false;

   double *dataArray = new double[ GetRowCount() ];

   int col = matrix.InsertCol( insertBefore, dataArray );

   // take care of labels
   AddLabel( "" );

   for ( int i=GetColCount()-1; i > col; i-- )
      SetLabel( i, GetLabel( i-1 ) );

   SetLabel( col, label );

   delete [] dataArray;

   return col;
   }

bool DDataObj::CopyCol( int toCol, int fromCol )
   {
   if ( toCol < 0 || toCol >= this->GetColCount() )
      return false;

   if ( fromCol < 0 || fromCol >= this->GetColCount() )
      return false;

   for ( int i=0; i < this->GetRowCount(); i++ )
      this->Set( toCol, i, this->GetAsDouble( fromCol, i ) );

   return true;
   }




//-- DDataObj::ReadAscii() ------------------------------------------
//
//--  Opens and loads the file into the data object
//-------------------------------------------------------------------

int DDataObj::ReadAscii( LPCTSTR _fileName, TCHAR delimeter, BOOL showMsg )
   {
   CString fileName;
   PathManager::FindPath( _fileName, fileName );

#ifndef NO_MFC
   HANDLE hFile = CreateFile(fileName, 
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
      TCHAR buffer[ 256 ];
      strcpy_s( buffer, 256, "VDataObj: Couldn't find file " );
      strcat_s( buffer, 256 - strlen(buffer), _fileName);
      Report::WarningMsg( buffer );
      return 0;
      }

   m_path = fileName;

   if ( this->m_name.IsEmpty() )
      this->m_name = fileName;
   
   return ReadAscii( hFile, delimeter, showMsg );
   }


//-------------------------------------------------------------------

int DDataObj::ReadAscii( HANDLE hFile, TCHAR delimiter, BOOL showMsg )
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
      TCHAR *start = p;
      p = _tcschr( p, '\n' );

      if ( p == NULL )
         p = _tcschr( start, '\r' );

      p++;
      }

   //-- start parsing buffer --//
   TCHAR *end = _tcschr( p, '\n' );

   if ( end == NULL )
      end =  _tcschr( p, '\r' );

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

      while ( *test != NULL && *test != '\n' && *test != '\r' ) // && testCount < 240 )
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

      if ( tabCount > commaCount && tabCount > spaceCount )
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
   TCHAR d[ 4 ];
   d[ 0 ] = delimiter;
   d[ 1 ] = '\n';
   d[ 2 ] = '\r';
   d[ 3 ] = NULL;
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

   double *data = new double[ cols ];

   int bytesPerRow = cols * sizeof(double);
   double rAI = bytesRead / (10.0f * bytesPerRow);
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
         data[ i ] =  (double) atof( p ) ;
         p = strtok_s( NULL, d, &next );  // look for delimeter

         //-- invalid line? --//
         if ( p == NULL && i < cols-1 )
            goto finish;
         }

      AppendRow( data, cols );
      }

   //-- go to
finish:

   //-- clean up --//
   delete[] data;
   delete[] buffer;


   return GetRowCount();
   }

#ifndef NO_MFC
#ifndef _WIN64
//-- DDataObj::ReadXls() ------------------------------------------
//
//-- Read the data object from an xls file.  Assumes the first row contains labels
//-------------------------------------------------------------------

int DDataObj::ReadXls(LPCTSTR fileName)
   {
  
   bool useWideChar = VData::SetUseWideChar( false );

   DbTable xls( DOT_VDATA );     // make an empty data table
   // get corresponding dbf file
   TCHAR *xlsName = new TCHAR[ lstrlen( fileName ) + 1 ];
   lstrcpy( xlsName, fileName );

   CString sql( "SELECT * FROM [sheet1$]" );

   int rows = xls.Load( xlsName, sql, FALSE, TRUE, "Excel 8.0;" );

   VData::SetUseWideChar( useWideChar );

   if ( rows <= 0 )
      {
      CString msg( "Error loading file: " );
      msg += fileName;
      Report::ErrorMsg( msg );
      return -1;
      }

	int cols = xls.GetColCount();
   // get the labels from first row
   SetSize( cols, 0 );  // resize matrix, statArray

   for (int col=0; col < cols; col++ )
      {
      CString colName( xls.m_pData->GetLabel( col ) );
      colName.Trim();
      SetLabel(col,colName);
      }

//  We have labels, now gather data row by row
   rows = xls.GetRecordCount();
   double str = 0;

   double *data = new double[cols];
   for (int row = 1; row < rows; row++) //first row is labels
      {
      for ( int col=0; col < cols; col++ )
         xls.GetData(row, col, data[col]);

      AppendRow(data, cols);
      }
   delete data;
   int t = GetColCount();
   return GetRowCount();
   }

#endif // _WIN64
#endif //NO_MFC


//-- DDataObj::WriteAscii() ------------------------------------------
//
//-- Saves the data object to an ascii file
//-------------------------------------------------------------------

int DDataObj::WriteAscii( LPCTSTR fileName, TCHAR delimiter, int colWidth )
   {
   int *colWidthArray = NULL;  // store column widths if colWidth == 0

   if ( _tcschr( fileName, '\\' ) != NULL || _tcschr( fileName, '/' != NULL  ) )
      {
      nsPath::CPath path( fileName );
      path.RemoveFileSpec();
#ifndef NO_MFC
      SHCreateDirectoryEx( NULL, path, NULL );
#else
      mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
      }

   FILE *fp;
   errno_t err = fopen_s( &fp, fileName, "wt" );     // open for writing
   if ( fp == NULL )
      {
      CString msg( "DDataObj: Couldn't open file " );
      msg += fileName;
      msg.Format("%s number %i", (LPCTSTR)msg, (int)err);
      Report::WarningMsg( msg );
      return 0;
      }

   int cols = GetColCount();
   int rows = GetRowCount();
   int col  = 0;
   int row  = 0;

   if ( colWidth == 0 )
      colWidthArray = new int[ cols ];

   bool useLabelWidth = ( m_labelArray.GetSize() > 0 && m_labelArray.GetSize() == (INT_PTR) cols ) ? true : false;
   
   TCHAR buffer[ 256 ];

   //-- put out labels, comma-delimited
   if ( useLabelWidth )
      {
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

         fprintf( fp, "%s%c ", buffer, delimiter );

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
         colWidthArray[ cols-1 ] = lstrlen( buffer ) + 1;
      }
   else if ( colWidth == 0 )
      {
      for ( int i=0; i < cols; i++ )
         colWidthArray[ i ] = 32;
      }
   
   //-- now cycle through rows, putting out actual data
   for ( row=0; row < rows; row++ )
      {
      int _colWidth = colWidth;

      for ( col=0; col < cols-1; col++ )
         {
         if ( colWidth == 0 )  _colWidth = colWidthArray[ col ];

         fprintf( fp, "%lf%c ", Get( col, row ), delimiter );
         }

      if ( colWidth == 0 ) _colWidth = colWidthArray[ cols-1 ];

      double value = Get( cols-1, row );
      fprintf( fp, "%lf\n",  value );
      }

   if ( colWidthArray )
      delete [] colWidthArray;

   fclose( fp );
   return rows;
   }


//-- DDataObj::GetMinMax() ------------------------------------------
//
//-- Retrieves the min, max values of a col
//-------------------------------------------------------------------

bool DDataObj::GetMinMax( int col, double *minimum, double *maximum, int startRow )
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

   *minimum = 1.0e20f;
   *maximum = -1.0e20f;

   int rows = GetRowCount();
   int cols = GetColCount();

   if ( rows <= 0 || (int) startRow >= rows || col >= cols )
      return false;

   // search across all columns?
   if ( col < 0 )
      {
      for ( int i=0; i < cols; i++ )
         {
         double _min, _max;
         if ( GetMinMax( i, &_min, &_max, startRow ) )
            {
            if ( *minimum > _min )
               *minimum = _min;

            if ( *maximum < _max )
               *maximum = _max;
            }
         }

      return true;
      }

   // just the specified column
   *minimum = Get( col, startRow );
   *maximum = Get( col, startRow );

   for ( int i=startRow; i < rows; i++)
      {
      if ( *minimum > Get( col, i ) )
         *minimum = Get( col, i );

      if ( *maximum < Get( col, i ) )
         *maximum = Get( col, i );
      }

   return true;
   }


//-------------------------------------------------------------------
double DDataObj::GetMean( int col, int startRow )
   {
   int rows = GetRowCount();

   if ( rows <= 0 || (int) startRow >= rows )
      return 0;

   double mean = 0.0;

   for ( int i=startRow; i < rows; i++ )
       mean += Get( col, i );

   return ( mean/(rows-startRow) );
   }


//-------------------------------------------------------------------
double DDataObj::GetStdDev( int col, int startRow )
   {
   int rows = GetRowCount();

   if ( rows < 2 || (int) startRow >= rows-1 )
      return 0;

   double sumSquares = 0;

   for ( int i=startRow; i < rows; i++ )
      {
      double value = Get( col, i );
      sumSquares += value * value;
      }

	return sqrt( (sumSquares/(rows-1-startRow)) );
	}

