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
//  Program: TYPEDEFS.H
//
//  Purpose: typedefs.
//-------------------------------------------------------------------

#if !defined  _TYPEDEFS_H
#define       _TYPEDEFS_H  1

#include "EnvLibs.h"

//#include <ATLComTime.h>

typedef unsigned int   UINT;
typedef unsigned int   FLAG;
typedef unsigned int   MODE;
typedef unsigned long  ULONG;

//-- possible return values --//
//const int ERROR   = -2;
//const int CANCEL  = -1;
//const int FAILURE =  0;
//const int SUCCESS =  1;

#define INIT_FLOAT ( 0.0 )
#define INIT_INT   ( 0 )
#define INIT_LONG  ( 0 )
#define INIT_UINT  ( 0 )

#define BETWEEN( x, y, z ) ( x <= y && y <= z )

//since we fake cpair in non-mfc, create macros for accessing fields
#ifdef NO_MFC
#define CPAIR_PTR_KEY(x) (x)->first
#define CPAIR_PTR_VALUE(x) (x)->second
#else
#define CPAIR_PTR_KEY(x) (x)->key
#define CPAIR_PTR_VALUE(x) (x)->value
#endif

//-------------------- data type information ---------------//

enum TYPE      // various data type flags.
   {
   TYPE_NULL     = 0x0000,
   TYPE_CHAR     = 0x0001,
   TYPE_BOOL     = 0x0002,
   TYPE_SHORT    = 0x0003,
   TYPE_UINT     = 0x0004,
   TYPE_INT      = 0x0005,
   TYPE_ULONG    = 0x0006,
   TYPE_LONG     = 0x0007,
   TYPE_FLOAT    = 0x0008,
   TYPE_DOUBLE   = 0x0009,
   TYPE_DATE     = 0x000A,  // DATE type (float)
   TYPE_VDATA    = 0x000B,
   TYPE_PTR      = 0x0100,  // void ptr
   TYPE_PCHAR    = 0x0101,  // TYPE_PTR | TYPE_CHAR
   TYPE_PBOOL    = 0x0102,  // TYPE_PTR | TYPE_BOOL
   TYPE_PSHORT   = 0x0103,  // TYPE_PTR | TYPE_SHORT
   TYPE_PUINT    = 0x0104,  // TYPE_PTR | TYPE_UINT
   TYPE_PINT     = 0x0105,  // TYPE_PTR | TYPE_INT
   TYPE_PULONG   = 0x0106,  // TYPE_PTR | TYPE_ULONG
   TYPE_PLONG    = 0x0107,  // TYPE_PTR | TYPE_LONG
   TYPE_PFLOAT   = 0x0108,  // TYPE_PTR | TYPE_FLOAT
   TYPE_PDOUBLE  = 0x0109,  // TYPE_PTR | TYPE_DOUBLE
   TYPE_PDATE    = 0x010A,  // TYPE_PTR | TYPE_DATE
   TYPE_STRING   = 0x0111,  // NULL-terminated string, statically allocated
   TYPE_DSTRING  = 0x0121,  // NULL-terminated string, dynamically allocated (new'ed)
   TYPE_CSTRING  = 0x0131,  // pointer to a MFC CString object
   TYPE_STLSTRING= 0x0141,  // pointer to std::string object
   TYPE_PDATAOBJ = 0x0140   // pointer to a data object
   };


bool IsReal   ( TYPE t );
bool IsInteger( TYPE t );
bool IsString ( TYPE t );
bool IsNumeric( TYPE t );
bool IsPtr    ( TYPE t ); 
LPCTSTR GetTypeLabel( TYPE t );
TYPE GetTypeFromPtrType( TYPE t );
TYPE GetPtrTypeFromType( TYPE t );


inline bool IsReal   ( TYPE t ) { return ( t == TYPE_FLOAT || t == TYPE_DOUBLE );  }
inline bool IsInteger( TYPE t ) { return ( t == TYPE_CHAR || t == TYPE_UINT || t == TYPE_INT || t == TYPE_ULONG || t == TYPE_LONG || t == TYPE_BOOL || t == TYPE_SHORT );  }
inline bool IsString ( TYPE t ) { return ( t == TYPE_STRING || t == TYPE_DSTRING );  }
inline bool IsNumeric( TYPE t ) { return ( IsReal( t ) || IsInteger( t ) );  }
inline bool IsPtr    ( TYPE t ) { return t & TYPE_PTR ? true : false; } 
inline TYPE GetTypeFromPtrType( TYPE t ) { return ( t == TYPE_PDATAOBJ ) ? t : TYPE( t & (~TYPE_PTR) ); }
inline TYPE GetPtrTypeFromType( TYPE t ) { return TYPE( t | TYPE_PTR ); }

inline LPCTSTR GetTypeLabel( TYPE t )
   {
   switch ( t )
      {
      case TYPE_SHORT:    return _T("Short" );
      case TYPE_INT:      return _T("Integer");
      case TYPE_UINT:     return _T("Unsigned Integer");
      case TYPE_LONG:     return _T("Long");
      case TYPE_ULONG:    return _T("Unsigned Long");
      case TYPE_FLOAT:    return _T("Float");
      case TYPE_DOUBLE:   return _T("Double");
      case TYPE_BOOL:     return _T("Boolean (Yes/No)");
      case TYPE_NULL:     return _T("NULL");
      case TYPE_CHAR:     return _T("Character(1)");
      case TYPE_STRING:
      case TYPE_DSTRING:  return _T("String");
      case TYPE_DATE:     return _T("Date/Time");
      case TYPE_PSHORT:   return _T("Short (ptr)" );
      case TYPE_PINT:     return _T("Integer (ptr)");
      case TYPE_PUINT:    return _T("Unsigned Integer (ptr)");
      case TYPE_PLONG:    return _T("Long (ptr)");
      case TYPE_PULONG:   return _T("Unsigned Long (ptr)");
      case TYPE_PFLOAT:   return _T("Float (ptr)");
      case TYPE_PDOUBLE:  return _T("Double (ptr)");
      case TYPE_PBOOL:    return _T("Boolean (Yes/No) (ptr)");
      case TYPE_PCHAR:    return _T("Character(1) (ptr)");
      case TYPE_PDATAOBJ: return _T("Data Object (ptr)");
      }
   return _T("Unknown");
   }


//---------------------------- structures -------------------------//


//-- variable-type data structure used for passing variables
//-- whose type is defined at runtime.
class DataObj;

struct LIBSAPI VDATA
   {
   TYPE type;        // the data type for this data

   union             // void data union
      {
      char     vChar;
      bool     vBool;
      UINT     vUInt;
      short    vShort;
      int      vInt;
      ULONG    vULong;
      long     vLong;
      float    vFloat;
      double   vDouble;
      void    *vPtr;
      char    *vString;
      DataObj *vPtrDataObj;
      DATE     vDateTime;
      } val;

   VDATA( void ) : type( TYPE_NULL ) { val.vLong = 0; }
   };

//-- 2D virtual (floating point) coordinant structure --//

typedef double REAL;

struct COORD2d
   {
   REAL x;
   REAL y;

   COORD2d( REAL _x, REAL _y ) { x = _x; y = _y; }
   COORD2d( void ) { x = y = 0.0; }
   COORD2d( COORD2d &d ) { *this = d; }
   
   COORD2d & operator = ( COORD2d &d ) { x = d.x; y = d.y; return *this; }
   bool operator == ( COORD2d &d ) { return ( x == d.x && y == d.y ); }
   void Set(REAL _x, REAL _y) { x = _x; y = _y; }
   };


struct COORD3d : public COORD2d
   {
   REAL z;

   COORD3d( REAL _x,  REAL _y, REAL _z ) : COORD2d( _x, _y ), z( _z ) { }
   COORD3d( void ) { x = y = z = 0.0; }

   bool operator == ( COORD3d &d ) { return ( z == d.z && x == d.x && y == d.y ); }
   };


struct COORD_RECT
   {

   COORD2d ll;
   COORD2d ur;

   COORD_RECT( REAL x0, REAL y0, REAL x1, REAL y1 )
      {

#ifdef NO_MFC
	using namespace std; //for min/max
#endif
     ll.x = min( x0, x1 ); ll.y = min( y0, y1 );  ur.x = max( x0, x1 ), ur.y = max( y0, y1 ); 
      }

   COORD_RECT( COORD_RECT &d ) { *this = d; }

   bool operator == ( COORD_RECT &d ) { return ( ll == d.ll && ur == d.ur ); }

   COORD_RECT &operator = ( COORD_RECT &d ) { ll = d.ll; ur = d.ur; return *this; } 
   };

//platform dependent module loading methods.
#ifndef NO_MFC
#define LOAD_LIBRARY(x) AfxLoadLibrary((x))
#define PROC_ADDRESS(h,s) GetProcAddress((h),(s))
#define FREE_LIBRARY(x) AfxFreeLibrary((x))
#else
#include<dlfcn.h>

inline void* NoMFCLoadLibrary(const char* & path)
{
  const char* PREFIX="lib";
  const char* SUFFIX=".so";
  const char* SUFFIX_DEV="-dev.so";

  std::string toPath=path;
  if(toPath.substr(toPath.size()-4)==std::string(".dll"))
#ifndef _DEBUG
    toPath=PREFIX+toPath.substr(0,toPath.size()-4)+SUFFIX;
#else
     toPath=PREFIX+toPath.substr(0,toPath.size()-4)+SUFFIX_DEV;
#endif

     void* ret=dlopen(toPath.c_str(),RTLD_LAZY);
     if(!ret)
       {
	 //the two \x1b pieces are for displaying red texts
	 //google ANSI color codes if interested.
	 const char* msg=dlerror();
	 std::cerr<<"\x1b[31m"<<"dyld err: "<<msg<<"\x1b[0m"<<std::endl;
       }
     return ret;
}
#define LOAD_LIBRARY(x) NoMFCLoadLibrary(x)
#define PROC_ADDRESS(h,s) dlsym((h),(s))
#define FREE_LIBRARY(x) dlclose((x))
#endif

//platform dependent cursor stuff
#ifndef NO_MFC
#define WAIT_CURSOR CWaitCursor c;
#else
#define WAIT_CURSOR
#endif
#endif
