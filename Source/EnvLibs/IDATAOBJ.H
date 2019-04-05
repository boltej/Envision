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
//  idataobj.hpp
//  Purpose:    Provides general dynamic data storage classes
//
//-- allocate out of "new'ed memory block.  Data is laid        
//-- out in one sequential block, row by row (i.e. all row 0,   
//-- all row 1, all row N).  Can get a ptr to the start of a row
//-- of data, but not a column, since colums are discontinuous. 
//-- therefore, if an object needs to get access to blocks of   
//-- data, it should put the blocks in rows rather than columns.
//
//-- Access arguments are always ordered ( col, row )...        
//-------------------------------------------------------------------

#pragma once
#include "EnvLibs.h"

#if !defined ( _DATAOBJ_H )
#include "DATAOBJ.H"
#endif

#if !defined ( _TARRAY_HPP )
#include "TARRAY.HPP"
#endif


#if !defined ( _IMATRIX_HPP )
#include "IMATRIX.HPP"
#endif



class LIBSAPI IDataObj : public DataObj
   {
   protected:
      IntMatrix matrix;        // matrix of floating pt values

   public:
      //-- constructor --//
      IDataObj( void );   
      IDataObj( int cols, int allocRows );  // by dimension (must Append())
      IDataObj( int cols, int allocRows, int initValue );  

      //-- copy constructor --//
      IDataObj( IDataObj& ) ;
 
      //-- destructor --//
      virtual ~IDataObj( void ) { }

      DO_TYPE GetDOType() { return DOT_INT; }

      //-- get element --//
      int operator () ( int col, int row ) { return Get( col, row ); }

      int Get( int col, int row ) { return matrix.Get( row, col ); }

      virtual bool Get( int col, int row, float &value   ) { value = (float) Get( col, row ); return true; }
      virtual bool Get( int col, int row, double &value  ) { value = (double) Get( col, row ); return true; }
      virtual bool Get( int col, int row, COleVariant &v ) { v = (long) Get( col, row ); return true; }
      virtual bool Get( int col, int row, VData &v       ) { v = Get( col, row ); return true; }
      virtual bool Get( int col, int row, int &v         ) { v = Get( col, row ); return true; }
      virtual bool Get( int col, int row, short &v       ) { v = (short) Get( col, row); return true; }
      virtual bool Get( int col, int row, bool &v        ) { v = Get( col, row ) == 0 ? false : true; return true; }
      virtual bool Get( int col, int row, CString &value ) { value = GetAsString( col, row ); return true; }

      int Find( int col, VData &value, int startRecord /*=-1*/ );

      //-- set element --//
      virtual bool Set( int col, int row, COleVariant &v ) { v.ChangeType( VT_I4 ); matrix.Set( row, col, v.intVal ); return true; }
      virtual bool Set( int col, int row, float v        ) { matrix.Set( row, col, (int) v ); return true; }
      virtual bool Set( int col, int row, double v       ) { matrix.Set( row, col, (int) v ); return true; }
      virtual bool Set( int col, int row, int v          ) { matrix.Set( row, col, v ); return true; }
      virtual bool Set( int col, int row, const VData &v       ) { matrix.Set( row, col, v.GetInt() ); return true; }
      virtual bool Set( int col, int row, LPCTSTR value ) { int v=atoi( value ); return Set( col,row,v ); }

      virtual void Clear( void );

      //- set size: no Append() required, data uninitialized --//
      virtual bool SetSize( int _cols, int _rows );

      virtual VARTYPE GetOleType( int col, int row=0 );
      virtual TYPE    GetType   ( int col, int row=0 );

      virtual float   GetAsFloat ( int col, int row  ) { return (float) Get( col, row ); }
      virtual double  GetAsDouble( int col, int row  ) { return (double) Get( col, row); }
      virtual CString GetAsString( int col, int row  ) { CString s; s.Format( "%i", Get( col, row ) ); return s; }
      virtual int     GetAsInt   ( int col, int row  ) { return Get( col, row ); }
      virtual bool    GetAsBool (int col, int row) { int value=Get( col, row ); return value == 1 ? true : false;  }
      virtual UINT    GetAsUInt  ( int col, int row  ) { return (UINT) Get( col, row ); }

      virtual bool GetMinMax ( int col, float *minimum, float *maximum, int startRow=0 );

      virtual int  AppendCols( int count );
      virtual int  AppendCol( LPCTSTR name );
      virtual bool CopyCol( int toCol, int fromCol );
      virtual int  InsertCol( int insertBefore, LPCTSTR label );

      virtual int AppendRow( COleVariant *varArray, int length );
      virtual int AppendRow( VData *varArray, int length );
      virtual int AppendRows( int count ) { return (int) matrix.AppendRows( (UINT) count ); }
      virtual int DeleteRow( int row ) { return (int) matrix.DeleteRow( (UINT) row ); }

      void   Add( int col, int row, float value ) { Set( col, row, Get( col, row ) + value ); }

      //-- interpolating Get() (col=column value to return) --//
      //float  IGet( float x, int col=1, IMETHOD method=IM_LINEAR );

      //-- concatenation --//
      void operator += ( CArray< int, int > &iArray ) { AppendRow( iArray.GetData(), (int) iArray.GetSize() ); }

      int AppendRow( int *iArray, int length );
      int SetRow( int *iArray, int setRow, int countCols );

      //-- various gets --//
      virtual int   GetRowCount( void ) { return matrix.GetRows(); }
      virtual int   GetColCount( void ) { return matrix.GetCols(); }

      //-- access data directly --//
      int **GetDataPtr( void    ) { return matrix.GetBase(); }
      int  *GetRowPtr ( int row ) { return matrix.GetRowPtr( row ); }

      //-- empty data/columns/rows --//
      void ClearRows( void ) { SetSize( GetColCount(), 0 ); }

      //-- statistical information --//
      float  GetMean   ( int col, int startRow=0 );
      double GetStdDev ( int col, int startRow=0 );

      //double GetSums      ( int col ) { return statArray[ col ].sx;  }
      //double GetSumSquares( int col ) { return statArray[ col ].ssx; }

      //-- File I/O --//
      int   ReadAscii ( LPCTSTR fileName, TCHAR delimiter=0, BOOL showMsg=TRUE );
      int   ReadAscii ( HANDLE hFile,    TCHAR delimiter=0, BOOL showMsg=TRUE );
      int   WriteAscii( LPCTSTR fileName, TCHAR delimiter=',', int colWidth=0 );
   };

inline VARTYPE IDataObj::GetOleType( int /*col*/, int /*row=0*/ ) { return VT_I4; }
inline TYPE    IDataObj::GetType   ( int /*col*/, int /*row=0*/ ) { return TYPE_INT; }
