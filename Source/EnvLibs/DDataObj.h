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
//  DDataObj.hpp
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
#include "DATAOBJ.H"
#include "TARRAY.HPP"
#include "DMatrix.h"

#pragma warning ( push )
#pragma warning( disable : 4800 )




class  LIBSAPI DDataObj : public DataObj
   {
   protected:
      DoubleMatrix matrix;        // matrix of floating pt values
      StatArray   statArray;     // column of statistics

   public:
      //-- constructor --//
      DDataObj(UNIT_MEASURE m);
            
      DDataObj( int cols, int allocRows, UNIT_MEASURE m);  // by dimension (must Append())
      DDataObj( int cols, int allocRows, double initialValue, UNIT_MEASURE m);
      DDataObj( int col, int allocRows, STATSFLAG*, UNIT_MEASURE m);

      //-- converstion ctor -- copies form arg, data, which is row-by-row in memory --//
      DDataObj( const DoubleMatrix::type_data * data, int cols, int allocRows, UNIT_MEASURE m);

      //-- copy constructor --//
      DDataObj( DDataObj& ) ;
 
      //-- destructor --//
      virtual ~DDataObj( void ) { }

      virtual DO_TYPE GetDOType() { return DOT_DOUBLE; }

      //-- get element --//
      double operator () ( int col, int row ) { return Get( col, row ); }

      double Get( int col, int row );

      virtual bool Get( int col, int row, float &value   ) { value = (float) Get( col, row ); return true; }
      virtual bool Get( int col, int row, double &value  ) { value = Get( col, row ); return true; }
      virtual bool Get( int col, int row, COleVariant &v ) { v = Get( col, row );     return true; }
      virtual bool Get( int col, int row, VData &v       ) { v = Get( col, row );     return true; }
      virtual bool Get( int col, int row, int &value     ) { value = (int) Get( col, row ); return true; }  // allow - jpb 1/24/2014
      virtual bool Get( int col, int row, short &value   ) { value = (short) Get(col, row); return true; }
      virtual bool Get( int /*col*/, int /*row*/, bool &         ) { return false; }
      virtual bool Get( int col, int row, CString &value ) { value = GetAsString( col, row ); return true; }

      //-- set element --//
      virtual bool Set( int col, int row, COleVariant &v ) { v.ChangeType( VT_R8 ); return matrix.Set( row, col, v.dblVal );  }
      virtual bool Set( int col, int row, float v        ) { return matrix.Set( row, col, (double) v ); }
      virtual bool Set( int col, int row, double v       ) { return matrix.Set( row, col, v ); }
      virtual bool Set( int col, int row, int v          ) { return matrix.Set( row, col, (double) v ); }
      virtual bool Set( int col, int row, const VData &v       ) { double value; if ( v.GetAsDouble( value ) ) return matrix.Set( row, col, value ); return false; }
      virtual bool Set( int col, int row, LPCTSTR value ) { double v=atof( value ); return Set( col,row,v ); }

      virtual int Find( int col, VData &value, int startRecord=-1 );

      virtual void Clear( void );

      //- set size: no Append() required, data uninitialized --//
      virtual bool SetSize( int _cols, int _rows );

      virtual VARTYPE GetOleType( int col, int row=0 );
      virtual TYPE    GetType   ( int col, int row=0 );

      virtual float   GetAsFloat ( int col, int row  ) { return (float) Get( col, row ); }
      virtual double  GetAsDouble( int col, int row )  { return Get(col, row); }
      virtual CString GetAsString( int col, int row  ) { CString s; s.Format( "%lg", Get( col, row ) ); return s; } 
      virtual int     GetAsInt   ( int col, int row  ) { return (int) Get( col, row ); }
      virtual bool    GetAsBool  (int col, int row)     { return false; }
      virtual UINT    GetAsUInt  ( int col, int row  ) { return (UINT) Get( col, row ); }

      virtual bool GetMinMax ( int col, double *minimum, double *maximum, int startRow=0 );

      virtual int  AppendCols( int count );
      virtual int  AppendCol( LPCTSTR label ); //, float *dataArray /*=NULL*/ );
      virtual bool CopyCol( int toCol, int fromCol );
      virtual int  InsertCol( int insertBefore, LPCTSTR label );

      virtual int AppendRow( COleVariant *array, int length );
      virtual int AppendRow( VData *array, int length       );
      virtual int AppendRows( int count ) { return (int) matrix.AppendRows( count ); }
      virtual int DeleteRow( int row ) { return (int) matrix.DeleteRow( (UINT) row ); }

      void   Add( int col, int row, double value ) { Set( col, row, Get( col, row ) + value ); }
      void   Div( int col, int row, double value ) { Set( col, row, Get( col, row ) / value ); }
    
      //-- interpolating Get() (col=column value to return) --//
      double  IGet(double x, int xcol, int ycol, IMETHOD method = IM_LINEAR, int *startRow=NULL, int *leftRowIndex = NULL, int *rightRowIndex = NULL);
      double  IGet( double x, int col=1, IMETHOD method=IM_LINEAR, int *leftRowIndex=NULL, int *rightRowIndex=NULL ) { return IGet( x, 0, col, method, NULL, leftRowIndex, rightRowIndex ); }
      double  IGet( double x, double y, IMETHOD method );
      //double  IGetInternal( double x, int col, IMETHOD method, int &offset, bool increment ); deprecated
   
      //-- concatenation --//
      void operator += ( CArray< double, double > &fArray ) { AppendRow( fArray.GetData(), (int) fArray.GetSize() ); }

      int AppendRow( double *fArray, int length );
      int AppendRow( CArray< double, double > &fArray ) { return AppendRow( fArray.GetData(), (int) fArray.GetSize() ); }

      int SetRow( double *fArray, int setRow, int countCols );

      //-- various gets --//
      virtual int   GetRowCount( void ) { return matrix.GetRows(); }
      virtual int   GetColCount( void ) { return matrix.GetCols(); }
      void  SetColFlags( int col, STATSFLAG flag ) { statArray[ col ].flag = flag; }

      //-- access data directly --//
      double **GetDataPtr( void    ) { return matrix.GetBase(); }
      double  *GetRowPtr ( int row ) { return matrix.GetRowPtr( row ); }

      //-- empty data/columns/rows --//
      void ClearRows( void ) { SetSize( GetColCount(), 0 ); }

      //-- statistical information --//
      double  GetMean   ( int col, int startRow=0 );
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

inline VARTYPE DDataObj::GetOleType( int /*col*/, int /*row=0*/ ) { return VT_R8; }
inline TYPE    DDataObj::GetType   ( int /*col*/, int /*row=0*/ ) { return TYPE_DOUBLE; }

#pragma warning ( pop )
