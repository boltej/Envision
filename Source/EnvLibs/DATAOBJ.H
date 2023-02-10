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
//  dataobj.h
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
//#include "TARRAY.HPP"

#include <unitconv.h>


#include "Vdata.h"
#include "STRARRAY.HPP"
#ifndef NO_MFC
#include <afxole.h>
#else
//stat is linux specific-may need to be cross platformed in the future
#include<sys/stat.h>
#endif

const UINT  DO_ALLOCERROR  = 0xFFFE;

enum DO_TYPE { DOT_VDATA, DOT_FLOAT, DOT_INT, DOT_DOUBLE, DOT_GEOSPATIAL, DOT_NULL, DOT_OTHER };


// CheckCol() flags (can be OR'd together)
enum 
   {
   CC_AUTOADD    = 0x1000,   // automatically adds column if not present without prompting
   CC_MUST_EXIST = 0x2000    // column must exist prior to CheckCol()
   };


enum STATSFLAG
   {
   SF_NONE = 0,          // nothing
   SF_OBSERVATION = 0x0001,     // calculate observational sx, ssx
   SF_DISCRETE_TIME = 0x0002,     // calculate discrete time sx, ssx
   SF_CONTINUOUS_TIME = 0x0004,     // calculate continuous time sx, ssx

   SF_STORE_MEAN = 0x0100,     // maintain trace of means in data obj
   SF_STORE_VARIANCE = 0x0200      // maintain trace of variance in data obj
   };


//struct DO_STATS
//   {
//   float min;
//   float max;
//   float sx;         // sum of x's
//   float ssx;        // sum of squares of x's
//   STATSFLAG  flag;   // type of statistic we collect
//   };
//
//
//class StatArray : public TArray< DO_STATS >
//   {
//   };


enum IMETHOD { IM_LINEAR, IM_QUADRATIC, IM_CONSTANT_RATE, IM_NONE };

// virtual superclass for VDataObj, FDataObj, IDataObj

class  LIBSAPI  DataObj
   {
   protected:
      int   m_dataCols;             // number of data (non-stat) columns
      CString m_name;               // name associated with data object
      CStringArray m_labelArray;     // column label char arrays

   public:
      CString m_path;              // path of loaded file
      UNIT_MEASURE m_primaryAxisUnit;   // x data unit (e.g. U_YEARS)

   public:
      //-- constructor --//
      DataObj(UNIT_MEASURE m);  // =U_UNDEFINED );
      DataObj(int cols, int allocRow, UNIT_MEASURE m); // = U_UNDEFINED);  // by dimension (must Append())
      
      //-- copy constructor --//
      DataObj( DataObj& );   

      //-- destructor --//
      virtual ~DataObj( void );

      //----------------- pure virtual functions ------------------//
      virtual DO_TYPE GetDOType() = 0;

      //-- get element --//
      virtual bool Get( int col, int row, float &value ) = 0;
      virtual bool Get( int col, int row, double &value) = 0;
      virtual bool Get( int col, int row, COleVariant& ) = 0;
      virtual bool Get( int col, int row, VData &      ) = 0;
      virtual bool Get( int col, int row, int &        ) = 0;
      virtual bool Get( int col, int row, short &      ) = 0;
      virtual bool Get( int col, int row, bool &       ) = 0;
      virtual bool Get( int col, int row, DataObj*&  ) { return false; } // only implemented by VDataObj; 
      virtual bool Get( int col, int row, TCHAR& ) { return false; }
      virtual bool Get( int col, int row, CString& )     = 0;

      virtual int Find( int col, VData &value, int startRecord=-1 ) = 0;
      virtual bool Set( int col, int row, COleVariant &value ) = 0;
      virtual bool Set( int col, int row, float value        ) = 0;
      virtual bool Set( int col, int row, double value       ) = 0;
      virtual bool Set( int col, int row, int value          ) = 0;
      virtual bool Set( int col, int row, const VData &value       ) = 0;
      virtual bool Set( int col, int row, LPCTSTR value       ) = 0;

      virtual void Clear( void );

      //- set size: no Append() required, data uninitialized --//
      virtual bool SetSize( int _cols, int _rows ) = 0;

      virtual bool GetMinMax( int col, float *minimum, float *maximum, int startRow=0 );

      bool CheckCol( LPCTSTR field, int &col, int );

      
      //-- concatenation --//
      virtual int  AppendCols( int count  ) = 0;
      virtual int  AppendCol( LPCTSTR name ) = 0;
      virtual bool CopyCol( int toCol, int fromCol ) = 0;
      virtual int  InsertCol( int insertBefore, LPCTSTR label ) = 0;

      virtual int  AppendRow( VData *array, int len ) = 0;   // these always work, subclasses can use specialized types
      virtual int  AppendRow( COleVariant *array, int len ) = 0;
      virtual int  AppendRows( int count ) = 0;
      virtual int  DeleteRow( int row ) = 0;

      //-- various gets --//
      virtual int GetRowCount( void ) = 0;
      virtual int GetColCount( void ) = 0;
      
      virtual VARTYPE GetOleType( int col, int row=0 ) = 0;
      virtual TYPE    GetType   ( int col, int row=0 ) = 0;

      virtual float   GetAsFloat ( int col, int row  ) = 0;
      virtual double  GetAsDouble( int col, int row  ) = 0;
      virtual CString GetAsString( int col, int row  ) = 0; 
      virtual int     GetAsInt   ( int col, int row  ) = 0;
      virtual UINT    GetAsUInt  ( int col, int row  ) = 0;
      virtual bool    GetAsBool  ( int col, int row  ) = 0;
      //virtual TCHAR   GetAsChar  ( int col, int row  ) = 0;

      // get a particular time range from a column of data
      // start, end are INCLUSIVE and represent years
      bool SubsetData(int col, int start, int end, CArray<float, float> &data)
         {
         data.RemoveAll();
         data.SetSize(end - start + 1);
         int index = 0;
         for (int i = start; i <= end; i++)
            data[index++] = this->GetAsFloat(col, i);

         return true;
         }

      //-- column labels --//
      int     GetCol( LPCTSTR  colLabel );
      LPCTSTR GetLabel( int col ) { return (LPCTSTR) m_labelArray[ col ]; }
      int     SetLabel( int col, LPCTSTR label );
      void    AddLabel( LPCTSTR label ) { m_labelArray.Add( label ); }

      //-- get/set name --//
      void  SetName( LPCTSTR buffer ) { m_name = buffer; }
      LPCTSTR GetName( void )         { return (LPCTSTR) m_name; }

      //long  GetSize( void ) { return size; }

      //-- empty data/columns/rows --//
      void ClearRows( void ) { SetSize( GetColCount(), 0 ); }

      //-- File I/O --//
      virtual int ReadAscii ( LPCTSTR fileName, char delimiter=0, BOOL showMsg=TRUE ) = 0;
      //virtual int ReadAscii ( FILE *fp,        char delimiter=',', BOOL showMsg=TRUE ) = 0;
      virtual int WriteAscii( LPCTSTR fileName, char delimiter=',', int colWidth=0    ) = 0;

#ifndef NO_MFC
      int CopyToClipboard( char delimeter='\t');
#endif
   };

inline bool DataObj::GetMinMax( int /*col*/, float* /*minimum*/, float* /*maximum*/, int /*startRow=0*/ )
   { return false; }
