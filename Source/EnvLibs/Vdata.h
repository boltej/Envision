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
#if !defined _VDATA_H
#define _VDATA_H

#include "EnvLibs.h"

#include "Typedefs.h"
#ifndef NO_MFC
#include <wtypes.h>
#endif

int WideToChar( LPCTSTR wc, char **c );


class  LIBSAPI VData : public VDATA
   {
   public:
      static TCHAR buffer[ 256 ];
      static bool m_useFormatString;
      static TCHAR m_formatString[ 16 ];
      
   protected:
      static bool m_useWideChar;

   public:
      static TYPE MapOleType( VARTYPE type );

      VData( void      ) { type = TYPE_NULL;  val.vDouble = 0; }
      VData( bool v    ) { type = TYPE_BOOL;  val.vBool   = v; }
      VData( short v   ) { type = TYPE_SHORT; val.vShort  = v; }
      VData( int  v    ) { type = TYPE_INT;   val.vInt    = v; }
      VData( UINT v    ) { type = TYPE_UINT;  val.vUInt   = v; }
      VData( LONG v    ) { type = TYPE_LONG;  val.vLong   = v; }
      VData( ULONG v   ) { type = TYPE_ULONG; val.vLong   = v; }
      VData( float v   ) { type = TYPE_FLOAT; val.vFloat  = v; }
      VData( double v  ) { type = TYPE_DOUBLE;val.vDouble = v; }
      VData( void*  v  ) { type = TYPE_PTR;   val.vPtr = v; }
//      VData( DATE v   )  { type = TYPE_DATE;  val.vDate = v; }
      VData( DataObj* v) { type = TYPE_PDATAOBJ; val.vPtrDataObj = v; }
      VData( LPCTSTR v, bool alloc=true ); // The arg, alloc, selects TYPE_DSTRING or TYPE_STRING.

      VData( bool   *v ) { type = TYPE_PBOOL;   val.vPtr = v; }
      VData( short  *v ) { type = TYPE_PSHORT;  val.vPtr = v; }
      VData( int    *v ) { type = TYPE_PINT;    val.vPtr = v; }
      VData( UINT   *v ) { type = TYPE_PUINT;   val.vPtr = v; }
      VData( LONG   *v ) { type = TYPE_PLONG;   val.vPtr = v; }
      VData( ULONG  *v ) { type = TYPE_PULONG;  val.vPtr = v; }
      VData( float  *v ) { type = TYPE_PFLOAT;  val.vPtr = v; }
      VData( double *v ) { type = TYPE_PDOUBLE; val.vPtr = v; }

      VData( void *pVar, TYPE type, bool storeAsPtr );     // give an address, stores as a value or as an address)

      ~VData( void );

      VData( const VData &v  ) { *this=v; }
      VData& operator = (const VData &v );
      VData& operator = (void  *p) { type = TYPE_PTR;    val.vPtr    = p; return *this; }
      VData& operator = (short  v) { type = TYPE_SHORT;  val.vShort  = v; return *this; }
      VData& operator = (int    v) { type = TYPE_INT;    val.vInt    = v; return *this; }
      VData& operator = (UINT   v) { type = TYPE_UINT;   val.vUInt   = v; return *this; }
      VData& operator = (LONG   v) { type = TYPE_LONG;   val.vLong   = v; return *this; }
      VData& operator = (ULONG  v) { type = TYPE_ULONG;  val.vULong = v; return *this; }
      VData& operator = (bool   v) { type = TYPE_BOOL;   val.vBool   = v; return *this; }
      VData& operator = (float  v) { type = TYPE_FLOAT;  val.vFloat  = v; return *this; }
      VData& operator = (double v) { type = TYPE_DOUBLE; val.vDouble = v; return *this; }
//      VData& operator = (DATE   v) { type = TYPE_DATE;   val.vDate   = v; return *this; }
      
      // ptr types
      VData& operator = (short*  v) { type = TYPE_PSHORT;   val.vPtr = v; return *this; }
      VData& operator = (int*    v) { type = TYPE_PINT;     val.vPtr = v; return *this; }
      VData& operator = (UINT*   v) { type = TYPE_PUINT;    val.vPtr = v; return *this; }
      VData& operator = (LONG*   v) { type = TYPE_PLONG;    val.vPtr = v; return *this; }
      VData& operator = (ULONG*  v) { type = TYPE_PULONG;   val.vPtr = v; return *this; }
      VData& operator = (bool*   v) { type = TYPE_PBOOL;    val.vPtr = v; return *this; }
      VData& operator = (float*  v) { type = TYPE_PFLOAT;   val.vPtr = v; return *this; }
      VData& operator = (double* v) { type = TYPE_PDOUBLE;  val.vPtr = v; return *this; }
      VData& operator = (DataObj*v) { type = TYPE_PDATAOBJ; val.vPtrDataObj = v; return *this; }
      VData& operator = (LPCTSTR v);  // note:  no allocation, to dynamically allocate, use AllocateString()

      // Assign arg to this; conserve type of this.
      bool SetKeepType(const VData & v){ VData t(v); if (t.ChangeType(type)) {*this=t; return true;} return false;}
      bool SetKeepType(void  *p) { VData t(p); if (t.ChangeType(type)) {*this=t; return true;} return false;}
      bool SetKeepType(short  v) { VData t(v); if (t.ChangeType(type)) {*this=t; return true;} return false;}
      bool SetKeepType(int    v) { VData t(v); if (t.ChangeType(type)) {*this=t; return true;} return false;}
      bool SetKeepType(bool   v) { VData t(v); if (t.ChangeType(type)) {*this=t; return true;} return false;}
      bool SetKeepType(float  v) { VData t(v); if (t.ChangeType(type)) {*this=t; return true;} return false;}
      bool SetKeepType(double v) { VData t(v); if (t.ChangeType(type)) {*this=t; return true;} return false;}
//      bool SetKeepType(DATE   v) { VData t(v); if (t.ChangeType(type)) {*this=t; return true;} return false;}
      bool SetKeepType(LPCTSTR v) { VData t(v); if (t.ChangeType(type)) {*this=t; return true;} return false;}

      bool operator == (const VData *v  ) const { return Compare( (const VData) *v ); }
      bool operator == (const VData & v ) const { return Compare( v ); }
      bool operator != (const VData *v  ) const { return ! Compare( (const VData) *v ); }
      bool operator != (const VData & v ) const { return ! Compare( v ); }
      bool operator <  (const VData &v  ) const ;
      bool operator <= (const VData &v  ) const ;
      bool operator >  (const VData &v  ) const ;
      bool operator >= (const VData &v  ) const ;

      VData & operator += ( double v );
      VData & operator += ( int v );
      VData & operator -= ( double v );
      VData & operator -= ( int v );
      VData & operator *= ( double v );
      VData & operator *= ( int v );
      VData & operator /= ( double v );
      VData & operator /= ( int v );

   //private:
   public:
      LPCTSTR AllocateString( LPCTSTR str );
   public:
      LPCTSTR GetAsString() const;
      TYPE    GetType() const { return type; }

      bool Compare( const VData& ) const;
      void SetNull( void ) { type=TYPE_NULL; val.vLong = 0; }
      bool IsNull( void ) { return type == TYPE_NULL ? true : false; }

      // unsafe get()'s - only use if you know the type
      int    GetInt()    const { ASSERT( type == TYPE_INT );    return val.vInt; }        
      long   GetLong()   const { ASSERT( type == TYPE_LONG );   return val.vLong; }
      float  GetFloat()  const { ASSERT( type == TYPE_FLOAT );  return val.vFloat; }
      double GetDouble() const { ASSERT( type == TYPE_DOUBLE ); return val.vDouble; }
      DATE   GetDate()   const { ASSERT( type == TYPE_DATE );   return val.vDateTime; }

      // "modern" get()'s
      bool    GetAsBool  ( bool &value    ) const;
      bool    GetAsString( CString &value ) const;
      bool    GetAsInt   ( int &value     ) const;
      bool    GetAsUInt  ( UINT &value    ) const;
      bool    GetAsDouble( double &value  ) const;
      bool    GetAsFloat ( float  &value  ) const;
      bool    GetAsChar  ( TCHAR &value    ) const;

      // set()'s
      void SetAsDString( LPCTSTR value );

      // conversion routines
      bool    ConvertToVariant  ( COleVariant &var );
      bool    ConvertFromVariant( COleVariant &var );

      // convert it from a string
      TYPE    Parse( LPCTSTR );

      // change type
      bool    ChangeType( TYPE newType );

      static void SetFormatString( LPCTSTR string );  // { lstrcpy( m_formatString, string ); }
      static bool SetUseWideChar( bool useWideChar ); // { bool oldVal = m_useWideChar; m_useWideChar = useWideChar; return oldVal; }
      static bool GetUseWideChar( void ); // { return m_useWideChar; }
};


// USE WITH STANDARD LIBRARY map<VData, T> or <algorithm> or
struct LIBSAPI VDataLess
{
	bool operator()(const VData & v1, const VData & v2) const;
};


inline
TYPE VData::MapOleType( VARTYPE type )
   {
   switch ( type )
      {
      case VT_BOOL:
      case VT_UI1:   return TYPE_BOOL;
      case VT_I4:    return TYPE_INT;
      case VT_R4:    return TYPE_FLOAT;
      case VT_R8:    return TYPE_DOUBLE;
      case VT_BSTR:  return TYPE_DSTRING;
      }

   return TYPE_NULL;
   }


#endif
