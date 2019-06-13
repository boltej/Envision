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
//  fdataobj.hpp
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


#if !defined ( _FMATRIX_HPP )
#include "FMATRIX.HPP"
#endif

#pragma warning ( push )
#pragma warning( disable : 4800 )


class  LIBSAPI FDataObj : public DataObj
   {
   protected:
      FloatMatrix matrix;        // matrix of floating pt values
      StatArray   statArray;     // column of statistics

   public:
      //-- constructor --//
      FDataObj();  // U_UNDEFINED   
      FDataObj( UNIT_MEASURE m );  // U_UNDEFINED   
            
      FDataObj( int cols, int allocRows, UNIT_MEASURE m=U_UNDEFINED);  // by dimension (must Append())
      FDataObj( int cols, int allocRows, float initialValue, UNIT_MEASURE m = U_UNDEFINED);
      FDataObj( int col, int allocRows, STATSFLAG*, UNIT_MEASURE m = U_UNDEFINED);

      //-- converstion ctor -- copies form arg, data, which is row-by-row in memory --//
      FDataObj( const FloatMatrix::type_data * data, int cols, int allocRows, UNIT_MEASURE m = U_UNDEFINED);

      //-- copy constructor --//
      FDataObj( FDataObj& ) ;
 
      //-- destructor --//
      virtual ~FDataObj( void ) { }

      virtual DO_TYPE GetDOType() { return DOT_FLOAT; }

      //-- get element --//
      float operator () ( int col, int row ) { return Get( col, row ); }

      float Get( int col, int row );

      virtual bool Get( int col, int row, float &value   ) { value = Get( col, row ); return true; }
      virtual bool Get( int col, int row, double &value  ) { value = Get( col, row ); return true; }
      virtual bool Get( int col, int row, COleVariant &v ) { v = Get( col, row );     return true; }
      virtual bool Get( int col, int row, VData &v       ) { v = Get( col, row );     return true; }
      virtual bool Get( int col, int row, int &value     ) { value = (int) Get( col, row ); return true; }  // allow - jpb 1/24/2014
      virtual bool Get( int col, int row, short &value   ) { value = (short)Get(col, row); return true; }  // allow - jpb 1/24/2014
      virtual bool Get( int /*col*/, int /*row*/, bool &         ) { return false; }
      virtual bool Get( int col, int row, CString &value ) { value = GetAsString( col, row ); return true; }

      //-- set element --//
      virtual bool Set( int col, int row, COleVariant &v ) { v.ChangeType( VT_R4 ); return matrix.Set( row, col, v.fltVal );  }
      virtual bool Set( int col, int row, float v        ) { return matrix.Set( row, col, v ); }
      virtual bool Set( int col, int row, double v       ) { return matrix.Set( row, col, (float) v ); }
      virtual bool Set( int col, int row, int v          ) { return matrix.Set( row, col, (float) v ); }
      virtual bool Set( int col, int row, const VData &v       ) { float value; if ( v.GetAsFloat( value ) ) return matrix.Set( row, col, value ); return false; }
      virtual bool Set( int col, int row, LPCTSTR value ) { float v=(float)atof( value ); return Set( col,row,v ); }

      virtual int Find( int col, VData &value, int startRecord=-1 );

      virtual void Clear( void );

      //- set size: no Append() required, data uninitialized --//
      virtual bool SetSize( int _cols, int _rows );

      virtual VARTYPE GetOleType( int col, int row=0 );
      virtual TYPE    GetType   ( int col, int row=0 );

      virtual float   GetAsFloat ( int col, int row ) { return Get( col, row ); }
      virtual double  GetAsDouble( int col, int row ) { return (double) Get(col, row); }
      virtual CString GetAsString( int col, int row ) { CString s; s.Format( "%g", Get( col, row ) ); return s; } 
      virtual int     GetAsInt   ( int col, int row ) { return (int)Get(col, row); }
      virtual bool    GetAsBool  ( int col, int row ) { return false; }
      virtual UINT    GetAsUInt  ( int col, int row ) { return (UINT) Get( col, row ); }

      virtual bool GetMinMax ( int col, float *minimum, float *maximum, int startRow=0 );

      virtual int  AppendCols( int count );
      virtual int  AppendCol( LPCTSTR label ); //, float *dataArray /*=NULL*/ );
      virtual bool CopyCol( int toCol, int fromCol );
      virtual int  InsertCol( int insertBefore, LPCTSTR label );

      virtual int AppendRow( COleVariant *array, int length );
      virtual int AppendRow( VData *array, int length       );
      virtual int AppendRows( int count ) { return (int) matrix.AppendRows( count ); }
      virtual int DeleteRow( int row ) { return (int) matrix.DeleteRow( (UINT) row ); }

      void   Add( int col, int row, float value ) { Set( col, row, Get( col, row ) + value ); }
      void   Div( int col, int row, float value ) { Set( col, row, Get( col, row ) / value ); }
    
      //-- interpolating Get() (col=column value to return) --//
      float  IGet(float x, int xcol, int ycol, IMETHOD method = IM_LINEAR, int *startRow=NULL, int *leftRowIndex = NULL, int *rightRowIndex = NULL);
      float  IGet( float x, int col=1, IMETHOD method=IM_LINEAR, int *leftRowIndex=NULL, int *rightRowIndex=NULL ) { return IGet( x, 0, col, method, NULL, leftRowIndex, rightRowIndex ); }
      float  IGet( float x, float y, IMETHOD method );
      //float  IGetInternal( float x, int col, IMETHOD method, int &offset, bool increment ); deprecated
   
      //-- concatenation --//
      void operator += ( CArray< float, float > &fArray ) { AppendRow( fArray.GetData(), (int) fArray.GetSize() ); }

      int AppendRow( float *fArray, int length );
      int AppendRow( CArray< float, float > &fArray ) { return AppendRow( fArray.GetData(), (int) fArray.GetSize() ); }

      int SetRow( float *fArray, int setRow, int countCols );

      //-- various gets --//
      virtual int   GetRowCount( void ) { return matrix.GetRows(); }
      virtual int   GetColCount( void ) { return matrix.GetCols(); }
      void  SetColFlags( int col, STATSFLAG flag ) { statArray[ col ].flag = flag; }

      //-- access data directly --//
      float **GetDataPtr( void    ) { return matrix.GetBase(); }
      float  *GetRowPtr ( int row ) { return matrix.GetRowPtr( row ); }

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
      int   ReadUSGSFlow ( LPCTSTR fileName, TCHAR delimiter=',', BOOL showMsg=TRUE );
      int   ReadUSGSFlow (HANDLE hFile,    TCHAR delimiter=',', BOOL showMsg=TRUE  );

      int   WriteAscii( LPCTSTR fileName, TCHAR delimiter=',', int colWidth=0 );
#ifndef NO_MFC
#ifndef _WIN64
      int   ReadXls(LPCTSTR fileName);
#endif
#endif
   };

inline VARTYPE FDataObj::GetOleType( int /*col*/, int /*row=0*/ ) { return VT_R4; }
inline TYPE    FDataObj::GetType   ( int /*col*/, int /*row=0*/ ) { return TYPE_FLOAT; }

#pragma warning ( pop )
