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
//  vdataobj.h
//  Purpose:    Provides general dynamic data storage classes
//
//-- allocate out of "new'ed memory block.  Data is laid        
//-- out in sequential blocks, row by row (i.e. all row 0,   
//-- all row 1, all row N).  Can get a ptr to the start of a row
//-- of data, but not a column, since colums are discontinuous. 
//-- therefore, if an object needs to get access to blocks of   
//-- data, it should put the blocks in rows rather than columns.
//
//-- Access arguments are always ordered ( col, row )...
//-------------------------------------------------------------------

#include "EnvLibs.h"

#if !defined _VDATAOBJ_H
#define      _VDATAOBJ_H 1

#if !defined _DATAOBJ_H
#include "DATAOBJ.H"
#endif

#if !defined ( _VDATA_H )
#include <VData.h>
#endif

#if !defined ( _TMATRIX_HPP )
#include "TMATRIX.HPP"
#endif



//-- interpolation methods for Get() --//
class  LIBSAPI VDataObj : public DataObj
   {
   protected:
      TMatrix< VData > matrix;            // matrix of Variant values

   public:
      //-- constructor --//
      VDataObj(UNIT_MEASURE m=UNIT_MEASURE::U_UNDEFINED);
      VDataObj( int cols, int allocRows, UNIT_MEASURE m=UNIT_MEASURE::U_UNDEFINED);  // by dimension (must Append())
      
      //-- copy constructor --//
      VDataObj( VDataObj& );   

      //-- destructor --//
      virtual ~VDataObj( void );

      virtual DO_TYPE GetDOType() { return DOT_VDATA; }

      //-- get element --//
      VData & operator () ( int col, int row ) { return Get( col, row ); }

      VData &Get( int col, int row );

      virtual bool Get( int col, int row, float &value );
      virtual bool Get( int col, int row, double &value);
      virtual bool Get( int col, int row, COleVariant& );
      virtual bool Get( int col, int row, VData &);
      virtual bool Get( int col, int row, int &);
      virtual bool Get( int col, int row, short &);
      virtual bool Get( int col, int row, bool & );
      virtual bool Get( int col, int row, DataObj*& ); 
      virtual bool Get( int col, int row, TCHAR& ); 
      virtual bool Get( int col, int row, CString &value ) { value = GetAsString( col, row ); return true; }

      virtual int Find( int col, VData &value, int startRecord /*=-1*/ );

      virtual bool Set( int col, int row, COleVariant &value );
      virtual bool Set( int col, int row, float value );
      virtual bool Set( int col, int row, double value );
      virtual bool Set( int col, int row, int value );
      virtual bool Set( int col, int row, const VData &value ) { matrix.Set( row, col, value ); return true; }
      virtual bool Set( int col, int row, LPCTSTR value ) { VData v; v.SetAsDString( value ); return Set( col, row, v ); }

      //-- various gets --//
      virtual int GetRowCount( void ) { return matrix.GetRows(); }
      virtual int GetColCount( void ) { return matrix.GetCols(); }

      virtual VARTYPE GetOleType( int col, int row=0 ) { COleVariant v; Get( col, row, v ); return v.vt; }
      virtual TYPE    GetType   ( int col, int row=0 ) { VData &v = Get( col, row ); return v.type; }

      virtual float   GetAsFloat ( int col, int row );
      virtual double  GetAsDouble( int  col, int row);
      virtual CString GetAsString( int col, int row ); 
      virtual int     GetAsInt   ( int col, int row );
      virtual bool    GetAsBool  (int col, int row);
      virtual UINT    GetAsUInt  ( int col, int row );

      //- set size: no Append() required, data uninitialized --//
      virtual bool SetSize  ( int _cols, int _rows );
      virtual int  AppendCols( int count );
      virtual int  AppendCol( LPCTSTR label );  // VData *dataArray=NULL );
      virtual int  InsertCol( int insertBefore, LPCTSTR label ); //, VData *dataArray=NULL );
      virtual bool CopyCol( int toCol, int fromCol );
      virtual int  AppendRow( COleVariant *values, int length );
		virtual int  AppendRow( VData *values, int length );  
		virtual int  AppendRow( CArray< VData, VData > &values );
		virtual int  AppendRows( int count );
      virtual int  DeleteRow( int row ) { return (int) matrix.DeleteRow( (UINT) row ); }

      virtual void Clear( void );

      //-- concatenation --//
      void operator += ( CArray< COleVariant, COleVariant& > &values ) { AppendRow( values.GetData(), (int) values.GetSize() ); }

      //int SetRow( COleVariant *array, int setRow, int countCols );

      //-- access data directly --//
      //COleVariant **GetDataPtr( void )    { return matrix.GetBase(); }
      //COleVariant  *GetRowPtr( int row ) { return matrix.GetRowPtr( row ); }

      //-- empty data/columns/rows --//
      void ClearRows( void ) { SetSize( GetColCount(), 0 ); }

      bool   GetMinMax( int col, float *minimum, float *maximum, int startRow );
      float  GetMean( int col, int startRow );
      double GetStdDev( int col, int startRow );
      float  GetSum( int col, int startRow );

      //-- File I/O --//
      virtual int ReadAscii ( LPCTSTR fileName, TCHAR delimiter=0, BOOL showMsg=TRUE );
      virtual int _ReadAscii( HANDLE hFile,    TCHAR delimiter=0, BOOL showMsg=TRUE );
      virtual int WriteAscii( LPCTSTR fileName, TCHAR delimiter=',', int colWidth=0 );

      int ReadDBF( LPCTSTR filename );

   };

 #endif
