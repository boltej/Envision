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

#include "EnvLibs.h"
#pragma hdrstop

#include <stdexcept>
using std::runtime_error;

#include "Vdata.h"
//#include <crack.h>

const int VDATA_BUFFER_SIZE = 256;
TCHAR VData::buffer[ VDATA_BUFFER_SIZE ];

bool VData::m_useWideChar = true;
bool VData::m_useFormatString = false;
TCHAR VData::m_formatString[ 16 ];


//-- variable-type data structure used for passing variables
//-- whose type is defined at runtime.

VData::VData( LPCTSTR v, bool alloc )
   {
   if ( alloc == true ) 
      {
      type = TYPE_DSTRING;
      AllocateString( v );
      }
   else
      {
      type = TYPE_STRING;
      val.vString = (TCHAR*) v;
      }
   }

VData::VData( void *pVar, TYPE _type, bool storeAsPtr )
   {
   type = GetTypeFromPtrType( _type );  // no change for non-ptr types
   if ( storeAsPtr )
      type = GetPtrTypeFromType( _type );

   switch( type )
      {
      case TYPE_DSTRING:
         this->AllocateString( (LPCTSTR) pVar );
         break;

      case TYPE_NULL:      val.vPtr    = NULL;  break;
      case TYPE_CHAR:      val.vChar   = *((TCHAR*)  pVar); break;
      case TYPE_BOOL:      val.vBool   = *((bool*)  pVar); break;
      case TYPE_UINT:      val.vUInt   = *((UINT*)  pVar); break;
      case TYPE_INT:       val.vInt    = *((int*)   pVar); break;
      case TYPE_SHORT:     val.vShort =  *((short int*)   pVar); break;
      case TYPE_ULONG:     val.vULong  = *((ULONG*) pVar); break;
      case TYPE_LONG:      val.vLong   = *((LONG*)  pVar); break;
      case TYPE_FLOAT:     val.vFloat  = *((float*) pVar); break;
      case TYPE_DOUBLE:    val.vDouble = *((double*)pVar); break;
      case TYPE_STRING:    val.vString = (TCHAR*) pVar;    break;
      
      // pointer types handled below
      case TYPE_PTR:
      case TYPE_PSHORT:   
      case TYPE_PINT:     
      case TYPE_PUINT:    
      case TYPE_PLONG:    
      case TYPE_PULONG:   
      case TYPE_PFLOAT:   
      case TYPE_PDOUBLE:  
      case TYPE_PBOOL:    
      case TYPE_PCHAR:    
      case TYPE_PDATAOBJ: 
            val.vPtr = pVar;
            break;      
      
      default:
         memcpy( val.vPtr, pVar, sizeof( val ) );
      }
   }


void VData::SetAsDString( LPCTSTR value )
   {
   if ( TYPE_DSTRING == type && val.vString != NULL ) 
      delete [] val.vString;  // delete this' previously alloc'd memory 
    
   type = TYPE_DSTRING; 
   AllocateString( value );
   }

 
VData& VData::operator = (LPCTSTR v) 
   { 
   if (TYPE_DSTRING==type && val.vString != NULL )
         delete [] val.vString;  // delete this' previously alloc'd memory 

   type = TYPE_STRING; 
   val.vString = (TCHAR*) v; 
   return *this; 
   }

VData& VData::operator = (const VData &v )
   {
   if ( type == TYPE_DSTRING && val.vString != NULL )
         delete [] val.vString;  //delete this' previously alloc'd memory 

   type = v.type;
   if ( type == TYPE_DSTRING )
      AllocateString( v.val.vString );
   else
      val = v.val;
      
   return *this;
   }


VData &VData::operator += ( double v )
   {
   switch( type )
      {
      case TYPE_SHORT:
         val.vShort += (short int) v;
         break;

      case TYPE_INT:
         val.vInt += (int) v;
         break;

      case TYPE_UINT:
         val.vUInt += (int) v;
         break;

      case TYPE_LONG:
         val.vLong += (long) v;
         break;
      
      case TYPE_ULONG:
         val.vLong += (long) v;
         break;
      
      case TYPE_FLOAT:
         val.vFloat += (float) v;
         break;
      
      case TYPE_DOUBLE:
         val.vDouble += v;
         break;
      }
   
   return *this;
   }
         

VData &VData::operator += ( int v )
   {
   switch( type )
      {
      case TYPE_SHORT:
         val.vShort += (short int) v;
         break;

      case TYPE_INT:
         val.vInt += v;
         break;

      case TYPE_UINT:
         val.vUInt += v;
         break;

      case TYPE_LONG:
         val.vLong += (long) v;
         break;
      
      case TYPE_ULONG:
         val.vLong += (long) v;
         break;
      
      case TYPE_FLOAT:
         val.vFloat += (float) v;
         break;
      
      case TYPE_DOUBLE:
         val.vDouble += (double) v;
         break;
      }
   
   return *this;
   }



VData &VData::operator -= ( double v )
   {
   switch( type )
      {
      case TYPE_SHORT:
         val.vShort -= (short int) v;
         break;

      case TYPE_INT:
         val.vInt -= (int) v;
         break;

      case TYPE_UINT:
         val.vUInt -= (int) v;
         break;

      case TYPE_LONG:
         val.vLong -= (long) v;
         break;
      
      case TYPE_ULONG:
         val.vLong -= (long) v;
         break;
      
      case TYPE_FLOAT:
         val.vFloat -= (float) v;
         break;
      
      case TYPE_DOUBLE:
         val.vDouble -= v;
         break;
      }
   
   return *this;
   }
         

VData &VData::operator -= ( int v )
   {
   switch( type )
      {
      case TYPE_SHORT:
         val.vShort -= (short int) v;
         break;

      case TYPE_INT:
         val.vInt -= v;
         break;

      case TYPE_UINT:
         val.vUInt -= v;
         break;

      case TYPE_LONG:
         val.vLong -= (long) v;
         break;
      
      case TYPE_ULONG:
         val.vLong -= (long) v;
         break;
      
      case TYPE_FLOAT:
         val.vFloat -= (float) v;
         break;
      
      case TYPE_DOUBLE:
         val.vDouble -= (double) v;
         break;
      }
   
   return *this;
   }

VData &VData::operator *= ( double v )
   {
   switch( type )
      {
      case TYPE_SHORT:
         val.vShort *= (short int) v;
         break;

      case TYPE_INT:
         val.vInt *= (int) v;
         break;

      case TYPE_UINT:
         val.vUInt *= (int) v;
         break;

      case TYPE_LONG:
         val.vLong *= (long) v;
         break;
      
      case TYPE_ULONG:
         val.vLong *= (long) v;
         break;
      
      case TYPE_FLOAT:
         val.vFloat *= (float) v;
         break;
      
      case TYPE_DOUBLE:
         val.vDouble *= v;
         break;
      }
   
   return *this;
   }
         

VData &VData::operator *= ( int v )
   {
   switch( type )
      {
      case TYPE_SHORT:
         val.vShort *= (short int) v;
         break;

      case TYPE_INT:
         val.vInt *= v;
         break;

      case TYPE_UINT:
         val.vUInt *= v;
         break;

      case TYPE_LONG:
         val.vLong *= (long) v;
         break;
      
      case TYPE_ULONG:
         val.vLong *= (long) v;
         break;
      
      case TYPE_FLOAT:
         val.vFloat *= (float) v;
         break;
      
      case TYPE_DOUBLE:
         val.vDouble *= (double) v;
         break;
      }
   
   return *this;
   }



VData &VData::operator /= ( double v )
   {
   switch( type )
      {
      case TYPE_SHORT:
         val.vShort /= (short int) v;
         break;

      case TYPE_INT:
         val.vInt /= (int) v;
         break;

      case TYPE_UINT:
         val.vUInt /= (int) v;
         break;

      case TYPE_LONG:
         val.vLong /= (long) v;
         break;
      
      case TYPE_ULONG:
         val.vLong /= (long) v;
         break;
      
      case TYPE_FLOAT:
         val.vFloat /= (float) v;
         break;
      
      case TYPE_DOUBLE:
         val.vDouble /= v;
         break;
      }
   
   return *this;
   }
         

VData &VData::operator /= ( int v )
   {
   switch( type )
      {
      case TYPE_SHORT:
         val.vShort /= (short int) v;
         break;

      case TYPE_INT:
         val.vInt /= v;
         break;

      case TYPE_UINT:
         val.vUInt /= v;
         break;

      case TYPE_LONG:
         val.vLong /= (long) v;
         break;
      
      case TYPE_ULONG:
         val.vLong /= (long) v;
         break;
      
      case TYPE_FLOAT:
         val.vFloat /= (float) v;
         break;
      
      case TYPE_DOUBLE:
         val.vDouble /= (double) v;
         break;
      }
   
   return *this;
   }



VData::~VData( void )
   {
   if ( type == TYPE_DSTRING && val.vString != NULL )
      delete [] val.vString;
   }


LPCTSTR VData::AllocateString( LPCTSTR str )
   {
   // The programmer must ensure that no memory leaks--
   // If this is a reallocation; then the previous allocation
   // must already be deleted !!!     
   type = TYPE_DSTRING;
   if ( str == NULL )
      val.vString = NULL;
   else
      {   
      val.vString = new TCHAR[ lstrlen( str )+1 ];
      lstrcpy( val.vString, str );
      }

   return val.vString;
   }


bool VData::GetAsBool( bool &value ) const
   {
   switch( type )
      {
      case TYPE_CHAR:
         value = ( val.vChar == 0 ) ? false : true;
         return true;
         
      case TYPE_BOOL:
         value = val.vBool;
         return true;

      case TYPE_UINT:
         value = ( val.vUInt == 0 ) ? false : true;
         return true;

      case TYPE_INT:
         value = ( val.vInt == 0 ) ? false : true;
         return true;

      case TYPE_SHORT:
         value = ( val.vShort == 0 ) ? false : true;
         return true;

      case TYPE_ULONG:
         value = ( val.vULong == 0 ) ? false : true;
         return true;

      case TYPE_LONG:
         value = ( val.vLong == 0 ) ? false : true;
         return true;

      case TYPE_FLOAT:
         value = ( val.vFloat == 0 ) ? false : true;
         return true;

      case TYPE_DOUBLE:
         value = ( val.vDouble == 0 ) ? false : true;
         return true;

      // pointer types handled below
      case TYPE_PSHORT:
         value = *((short*) val.vPtr) == 0 ? false : true;
         return true;

      case TYPE_PINT:     
         value = *((int*) val.vPtr) == 0 ? false : true;
         return true;

      case TYPE_PUINT:    
         value = *((UINT*) val.vPtr) == 0 ? false : true;
         return true;

      case TYPE_PLONG:    
         value = *((LONG*) val.vPtr) == 0 ? false : true;
         return true;

      case TYPE_PULONG:   
         value = *((ULONG*) val.vPtr) == 0 ? false : true;
         return true;

      case TYPE_PFLOAT:   
         value = *((float*) val.vPtr) == 0 ? false : true;
         return true;

      case TYPE_PDOUBLE:  
         value = *((double*) val.vPtr) == 0 ? false : true;
         return true;

      case TYPE_PBOOL:    
         value = *((bool*) val.vPtr) == 0 ? false : true;
         return true;

      case TYPE_PCHAR:    
         value = *((TCHAR*) val.vPtr) == 0 ? false : true;
         return true;

      default:
         value = false;
         return false;
      }
   }


bool VData::GetAsInt( int &value ) const
   {
   switch( type )
      {
      case TYPE_CHAR:
         value = val.vChar;
         return true;
         
      case TYPE_BOOL:
         value = val.vBool ? 1 : 0;
         return true;

      case TYPE_UINT:
         value = (int) val.vUInt;
         return true;

      case TYPE_INT:
         value = val.vInt;
         return true;

      case TYPE_SHORT:
         value = (int) val.vShort;
         return true;

      case TYPE_ULONG:
         value = (int) val.vULong;
         return true;

      case TYPE_LONG:
         value = (int) val.vLong;
         return true;

      case TYPE_FLOAT:
         value = (int) val.vFloat;
         return true;

      case TYPE_DOUBLE:
         value = (int) val.vDouble;
         return true;

      case TYPE_STRING:
      case TYPE_DSTRING:
         {
         if ( val.vString == NULL )
            {
            value = 0;
            return true;
            }

         TCHAR *end;
         value = (int) strtol( val.vString, &end, 10 );

         if ( *end == NULL || *end == ' ' )
            return true;
         else
            return false;
         }

      // pointer types handled below
      case TYPE_PSHORT:
         value = int( *((short*) val.vPtr) );
         return true;

      case TYPE_PINT:     
         value = *((int*) val.vPtr);
         return true;

      case TYPE_PUINT:    
         value = int( *((UINT*) val.vPtr) );
         return true;

      case TYPE_PLONG:    
         value = int( *((LONG*) val.vPtr) );
         return true;

      case TYPE_PULONG:   
         value = int( *((ULONG*) val.vPtr) );
         return true;

      case TYPE_PFLOAT:   
         value = int( *((float*) val.vPtr) );
         return true;

      case TYPE_PDOUBLE:  
         value = int( *((double*) val.vPtr) );
         return true;

      case TYPE_PBOOL:    
         value = *((bool*) val.vPtr) == false ? 0 : 1;
         return true;

      case TYPE_PCHAR:    
         value = int( *((TCHAR*) val.vPtr) );
         return true;

      default:
         value = 0;
         return false;
      }
   }


bool VData::GetAsUInt( UINT &value ) const
   {
   switch( type )
      {
      case TYPE_CHAR:
         value = val.vChar;
         return true;
         
      case TYPE_BOOL:
         value = val.vBool ? 1 : 0;
         return true;

      case TYPE_UINT:
         value = val.vUInt;
         return true;

      case TYPE_INT:
         value = (UINT) val.vInt;
         return true;

      case TYPE_SHORT:
         value = (UINT) val.vShort;
         return true;

      case TYPE_ULONG:
         value = (UINT) val.vULong;
         return true;

      case TYPE_LONG:
         value = (UINT) val.vLong;
         return true;

      case TYPE_FLOAT:
         value = (UINT) val.vFloat;
         return true;

      case TYPE_DOUBLE:
         value = (UINT) val.vDouble;
         return true;

      case TYPE_STRING:
      case TYPE_DSTRING:
         {
         if ( val.vString == NULL )
            {
            value = 0;
            return true;
            }

         TCHAR *end;
         value = (int) strtol( val.vString, &end, 10 );

         if ( *end == NULL || *end == ' ' )
            return true;
         else
            return false;
         }

      // pointer types handled below
      case TYPE_PSHORT:
         value = UINT( *((short*) val.vPtr) );
         return true;

      case TYPE_PINT:     
         value = *((int*) val.vPtr);
         return true;

      case TYPE_PUINT:    
         value = UINT( *((UINT*) val.vPtr) );
         return true;

      case TYPE_PLONG:    
         value = UINT( *((LONG*) val.vPtr) );
         return true;

      case TYPE_PULONG:   
         value = UINT( *((ULONG*) val.vPtr) );
         return true;

      case TYPE_PFLOAT:   
         value = UINT( *((float*) val.vPtr) );
         return true;

      case TYPE_PDOUBLE:  
         value = UINT( *((double*) val.vPtr) );
         return true;

      case TYPE_PBOOL:    
         value = *((bool*) val.vPtr) == false ? 0 : 1;
         return true;

      case TYPE_PCHAR:    
         value = UINT( *((TCHAR*) val.vPtr) );
         return true;

      default:
         value = 0;
         return false;
      }
   }


bool VData::GetAsDouble( double &value ) const
   {
   switch( type )
      {
      case TYPE_CHAR:
         value = 0.0;
         return false;
         
      case TYPE_BOOL:
         value = val.vBool ? 1.0 : 0.0;
         return true;

      case TYPE_UINT:
         value = (double) val.vUInt;
         return true;

      case TYPE_INT:
         value = (double) val.vInt;
         return true;

      case TYPE_SHORT:
         value = (double) val.vShort;
         return true;

      case TYPE_ULONG:
         value = (double) val.vULong;
         return true;

      case TYPE_LONG:
         value = (double) val.vLong;
         return true;

      case TYPE_FLOAT:
         value = (double) val.vFloat;
         return true;

      case TYPE_DOUBLE:
         value = val.vDouble;
         return true;

      case TYPE_STRING:
      case TYPE_DSTRING:
         {
         if ( val.vString == NULL )
            {
            value = 0.0;
            return true;
            }

         TCHAR *end;
         value = strtod( val.vString, &end );

         if ( *end == NULL || *end == ' ' )
            return true;
         else
            return false;
         }

      // pointer types handled below
      case TYPE_PSHORT:
         value = double( *((short*) val.vPtr) );
         return true;

      case TYPE_PINT:     
         value = double( *((int*) val.vPtr) );
         return true;

      case TYPE_PUINT:    
         value = double( *((UINT*) val.vPtr) );
         return true;

      case TYPE_PLONG:    
         value = double( *((LONG*) val.vPtr) );
         return true;

      case TYPE_PULONG:   
         value = double( *((ULONG*) val.vPtr) );
         return true;

      case TYPE_PFLOAT:   
         value = double( *((float*) val.vPtr) );
         return true;

      case TYPE_PDOUBLE:  
         value = *((double*) val.vPtr);
         return true;

      case TYPE_PBOOL:    
         value = *((bool*) val.vPtr) == false ? 0 : 1;
         return true;

      case TYPE_PCHAR:    
         value = double( *((TCHAR*) val.vPtr) );
         return true;
         
      default:
         value = 0.0;
         return false;
      }
   }


bool VData::GetAsFloat( float &value )const 
   {
   switch( type )
      {
      case TYPE_CHAR:
         value = 0.0;
         return false;
         
      case TYPE_BOOL:
         value = val.vBool ? 1.0f : 0.0f;
         return true;

      case TYPE_UINT:
         value = (float) val.vUInt;
         return true;

      case TYPE_INT:
         value = (float) val.vInt;
         return true;

      case TYPE_SHORT:
         value = (float) val.vShort;
         return true;

      case TYPE_ULONG:
         value = (float) val.vULong;
         return true;

      case TYPE_LONG:
         value = (float) val.vLong;
         return true;

      case TYPE_FLOAT:
         value = val.vFloat;
         return true;

      case TYPE_DOUBLE:
         value = (float) val.vDouble;
         return true;

      case TYPE_STRING:
      case TYPE_DSTRING:
         {
         if ( val.vString == NULL )
            {
            value = 0.0f;
            return true;
            }

         TCHAR *end;
         value = (float) strtod( val.vString, &end );

         if ( *end == NULL || *end == ' ' )
            return true;
         else
            return false;
         }

      // pointer types handled below
      case TYPE_PSHORT:
         value = float( *((short*) val.vPtr) );
         return true;

      case TYPE_PINT:     
         value = float( *((int*) val.vPtr) );
         return true;

      case TYPE_PUINT:    
         value = float( *((UINT*) val.vPtr) );
         return true;

      case TYPE_PLONG:    
         value = float( *((LONG*) val.vPtr) );
         return true;

      case TYPE_PULONG:   
         value = float( *((ULONG*) val.vPtr) );
         return true;

      case TYPE_PFLOAT:   
         value = *((float*) val.vPtr);
         return true;

      case TYPE_PDOUBLE:  
         value = float( *((double*) val.vPtr) );
         return true;

      case TYPE_PBOOL:    
         value = *((bool*) val.vPtr) == false ? 0.0f : 1.0f;
         return true;

      case TYPE_PCHAR:    
         value = float( *((TCHAR*) val.vPtr) );
         return true;
         
      default:
         value = 0.0f;
         return false;
      }
   }


bool VData::GetAsChar( TCHAR &value ) const 
   {
   TYPE _type = GetTypeFromPtrType( type );  // no effect on non-ptr types

   switch( _type )
      {
      case TYPE_CHAR:
         if ( IsPtr( type ) )
            value = *((TCHAR*) val.vPtr);
         else
            value = val.vChar;
         return true;
         
      case TYPE_BOOL:
         if( IsPtr( type ) )
            value = *((bool*)val.vPtr) ? 'T' : 'F';
         else
            value = val.vBool ? 'T' : 'F';
         return true;

      case TYPE_UINT:
      case TYPE_INT:
      case TYPE_ULONG:
      case TYPE_LONG:
      case TYPE_SHORT:
         {
         int _value;
         GetAsInt( _value );
         value = _value == 1 ? 'T' : 'F';
         return true;
         }

      case TYPE_STRING:
      case TYPE_DSTRING:
         if ( val.vString == NULL )
            value = 0;
         else
            value = val.vString[ 0 ];
         return true;

      default:
         value = 'F';
         return false;
      }
   }


bool VData::GetAsString( CString &value ) const 
   {
   LPCTSTR valStr = GetAsString();
   if ( valStr == NULL )
      {
      value.Empty();
      return false;
      }
   else
      {
      value = valStr;
      return true;
      }
   }



LPCTSTR VData::GetAsString() const
   {
   switch ( type )
      {
      case TYPE_NULL:
         lstrcpy( buffer, "" );
         return buffer;

      case TYPE_CHAR:
         buffer[ 0 ] = val.vChar;
         buffer[ 1 ] = NULL;
         return buffer;

      case TYPE_BOOL:
         if ( val.vBool )
            lstrcpy( buffer, "1" );
         else
            lstrcpy( buffer, "0" );
         return buffer;

      case TYPE_UINT:
         _itoa_s( val.vUInt, buffer, VDATA_BUFFER_SIZE, 10 );
         return buffer;

      case TYPE_INT:
         _itoa_s( val.vInt, buffer, VDATA_BUFFER_SIZE, 10 );
         return buffer;

      case TYPE_SHORT:
         _itoa_s( int ( val.vShort ), buffer, VDATA_BUFFER_SIZE, 10 );
         return buffer;

      case TYPE_ULONG:
         _itoa_s( val.vULong, buffer, VDATA_BUFFER_SIZE, 10 );
         return buffer;

      case TYPE_LONG:
         _itoa_s( val.vLong, buffer, VDATA_BUFFER_SIZE, 10 );
         return buffer;

      case TYPE_FLOAT:
         if ( m_useFormatString )
            sprintf_s( buffer, VDATA_BUFFER_SIZE, (LPCTSTR) m_formatString, val.vFloat );
         else
            sprintf_s( buffer, VDATA_BUFFER_SIZE, "%f", val.vFloat );
         return buffer;

      case TYPE_DOUBLE:
         if ( m_useFormatString )
            sprintf_s( buffer, VDATA_BUFFER_SIZE, m_formatString, val.vDouble );
         else
            sprintf_s( buffer, VDATA_BUFFER_SIZE, "%lf", val.vDouble );
         return buffer;

      case TYPE_PTR:
         lstrcpy( buffer, "PTR" );    
         return buffer;

      // pointer types handled below
      case TYPE_PSHORT:
      case TYPE_PINT:     
      case TYPE_PUINT:    
      case TYPE_PLONG:    
      case TYPE_PULONG:   
         {
         int value;
         GetAsInt( value );
         VData v( value );
         return v.GetAsString();
         }

      case TYPE_PFLOAT:
      case TYPE_PDOUBLE:  
         {
         double value;
         GetAsDouble( value );
         VData v( value );
         return v.GetAsString();
         }

      case TYPE_PBOOL:
         {
         bool value;
         GetAsBool( value );
         VData v( value );
         return v.GetAsString();
         }

      case TYPE_PCHAR:
         return (LPCTSTR) val.vPtr; 

      case TYPE_PDATAOBJ:
         lstrcpy( buffer, "DataObj" );
         return buffer;

      case TYPE_STRING:     // NULL-terminated string, statically allocated
      case TYPE_DSTRING:    // NULL-terminated string, dynamically allocated (new'ed)
            return (LPCTSTR) val.vString;
      }

   return NULL;
   }

bool VData::ConvertToVariant( COleVariant &var )
   {
   switch ( type )
      {
      case TYPE_CHAR:
         var.vt   = VT_UI1;
         var.bVal = val.vChar;
         return true;

      case TYPE_BOOL:
         var.vt  = VT_BOOL;
         var.boolVal = ( val.vBool == true ? -1 : 0 );
         return true;

      case TYPE_UINT:
         var.vt   = VT_I4;
         var.uintVal = val.vUInt;
         return true;

      case TYPE_INT:
         var.vt   = VT_I4;
         var.intVal = val.vInt;
         return true;

      case TYPE_SHORT:
         var.vt   = VT_I2;
         var.iVal = val.vShort;
         return true;

      case TYPE_LONG:
         var.vt   = VT_I4;
         var.lVal = val.vLong;
         return true;

      case TYPE_ULONG:
         var.vt   = VT_I4;
         var.ulVal = val.vULong;
         return true;

      case TYPE_FLOAT:
         var.vt   = VT_R4;
         var.fltVal = val.vFloat;
         return true;

      case TYPE_DOUBLE:
         var.vt   = VT_R8;
         var.dblVal = val.vDouble;
         return true;

      case TYPE_DATE:
         var.vt   = VT_DATE;
         var.date = val.vDateTime;
         return true;

      case TYPE_PTR:
         //lstrcpy( buffer, "PTR" );
         //return buffer;

      case TYPE_STRING:     // NULL-terminated string, statically allocated
      case TYPE_DSTRING:
         //var.vt = VT_I4; // VT_BYREF|VT_UI1;  // 9/20/07 jpb
         //var.pbVal = (unsigned TCHAR*) val.vString;
         if ( m_useWideChar )
            var.SetString( (LPCTSTR) val.vString, VT_BSTRT );
         else
            var = (LPCTSTR) val.vString;
         return true;
      }

   return false;
   }



bool VData::ConvertFromVariant( COleVariant &var )
   {
   switch ( var.vt )
      {
      case VT_EMPTY:
      case VT_NULL:
         type = TYPE_NULL;
         val.vInt = 0;
         return true;

      case VT_BOOL:
         type = TYPE_BOOL;
         val.vBool = ( var.boolVal == 0 ) ? false : true;
         return true;

      case VT_UI1:
         type = TYPE_CHAR;
         val.vChar = var.bVal;
         return true;

      case VT_I4:
         type = TYPE_INT;
         val.vInt = var.lVal;
         return true;

      case VT_I2:
         type = TYPE_SHORT;
         val.vShort = var.iVal;
         return true;

      case VT_R4:
         type = TYPE_FLOAT;
         val.vFloat = var.fltVal;
         return true;

      case VT_R8:
         type = TYPE_DOUBLE;
         val.vDouble = var.dblVal;
         return true;

      case VT_DATE:
         type = TYPE_DATE;
         val.vDateTime = var.date;
         return true;

      //case TYPE_PTR:
         //lstrcpy( buffer, "PTR" );
         //return buffer;

      case VT_BSTR:
         {
         if ( type == TYPE_DSTRING && val.vString != NULL )
            delete [] val.vString;

         type = TYPE_DSTRING;

         if ( m_useWideChar )
            WideToChar((LPCTSTR) var.byref, &val.vString );
         else
            {
            TCHAR *p = (LPSTR) var.byref;
            AllocateString( p );
            }

         return true;
         }

      default:
         TRACE0( "Unknowm Variant type in VData::ConvertFromVariant()" );
         //ASSERT( 0 ); 
      }

   return false;
   }

bool VData::operator < ( const VData &v ) const 
   {
   if ( IsString( type ) && IsString( v.type ) )
      return ( lstrcmp( this->val.vString, v.val.vString ) < 0 ? true : false );

   else
      {
      double v0, v1;
      bool ok0 = this->GetAsDouble( v0 );
      bool ok1 = v.GetAsDouble( v1 );

      ASSERT( ok0 && ok1 );
      if ( ! ok0 || ! ok1 )
         return false;

      return ( (v0-v1) < 0 ? true : false );
      }
   }

bool VData::operator <= ( const VData &v ) const 
   {
   if ( IsString( type ) && IsString( v.type ) )
      return ( lstrcmp( this->val.vString, v.val.vString ) <= 0 ? true : false );

   else
      {
      double v0, v1;
      bool ok0 = this->GetAsDouble( v0 );
      bool ok1 = v.GetAsDouble( v1 );

      ASSERT( ok0 && ok1 );
      if ( ! ok0 || ! ok1 )
         return false;

      return ( (v0-v1) <= 0 ? true : false );
      }
   }


bool VData::operator > ( const VData &v ) const 
   {
   if ( IsString( type ) && IsString( v.type ) )
      return ( lstrcmp( this->val.vString, v.val.vString ) > 0 ? true : false );

   else
      {
      double v0, v1;
      bool ok0 = this->GetAsDouble( v0 );
      bool ok1 = v.GetAsDouble( v1 );

      ASSERT( ok0 && ok1 );
      if ( ! ok0 || ! ok1 )
         return false;

      return ( (v0-v1) > 0 ? true : false );
      }
   }


bool VData::operator >= ( const VData &v ) const 
   {
   if ( IsString( type ) && IsString( v.type ) )
      return ( lstrcmp( this->val.vString, v.val.vString ) >= 0 ? true : false );

   else
      {
      double v0, v1;
      bool ok0 = this->GetAsDouble( v0 );
      bool ok1 = v.GetAsDouble( v1 );

      ASSERT( ok0 && ok1 );
      if ( ! ok0 || ! ok1 )
         return false;

      return ( (v0-v1) >= 0 ? true : false );
      }
   }


bool VData::Compare( const VData &v ) const
   {
   switch( type )
      {
      case TYPE_NULL:
         return type == v.type;     // NULL match if both are TYPE_NULL

      case TYPE_CHAR:               // types have to match
         if ( type != v.type )
            return false;
         else
            return val.vChar == v.val.vChar;

      case TYPE_BOOL:
         if ( type != v.type )
            return false;
         else
            return val.vBool == v.val.vBool;

      case TYPE_UINT:
         {
         int value;
         bool ok = v.GetAsInt( value );
         if ( ok )
            return val.vUInt == value;
         else
            return false;
         }
      
      case TYPE_INT:
         {
         int value;
         bool ok = v.GetAsInt( value );
         if ( ok )
            return val.vInt == value;
         else
            return false;
         }

      case TYPE_SHORT:
         {
         int value;
         bool ok = v.GetAsInt( value );
         if ( ok )
            return int( val.vShort ) == value;
         else
            return false;
         }
               
      case TYPE_LONG:
         {
         int value;
         bool ok = v.GetAsInt( value );
         if ( ok )
            return val.vLong == value;
         else
            return false;
         }
         
      case TYPE_ULONG:
         {
         int value;
         bool ok = v.GetAsInt( value );
         if ( ok )
            return val.vULong == value;
         else
            return false;
         }
         
      case TYPE_FLOAT:
         {
         float value;
         bool ok = v.GetAsFloat( value );
         if ( ok )
            return val.vFloat == value;
         else
            return false;
         }

      case TYPE_DOUBLE:
         {
         double value;
         bool ok = v.GetAsDouble( value );
         if ( ok )
            return val.vDouble == value;
         else
            return false;
         }
         
      case TYPE_PTR:
      case TYPE_PDATAOBJ:
      case TYPE_PSHORT:
      case TYPE_PINT:     
      case TYPE_PUINT:    
      case TYPE_PLONG:    
      case TYPE_PULONG:   
      case TYPE_PFLOAT:
      case TYPE_PDOUBLE:  
      case TYPE_PBOOL:
      case TYPE_PCHAR:
         if ( type != v.type )
            return false;
         else
            return val.vPtr == v.val.vPtr;

     case TYPE_STRING:   
     case TYPE_DSTRING:
         {
         if ( v.type != TYPE_STRING && v.type != TYPE_DSTRING )
            return false;
         else
            {
            if ( val.vString != NULL && v.val.vString != NULL )
               return lstrcmp( val.vString, v.val.vString ) == 0;
            else if ( val.vString == NULL && v.val.vString == NULL )
               return true;
            else
               return false;
            }
         }
      }

   return false;
   }



bool VDataLess::operator()(const VData & v1, const VData & v2)  const 
{
   if (v1.type != v2.type || v1.type == TYPE_NULL || v2.type == TYPE_NULL)
   {
      throw runtime_error("VDataLess fails on different or invalid types");
   }


   TYPE type = v1.type;
   switch( type )
   {

   case TYPE_CHAR:               
      return (v1.val.vChar < v2.val.vChar);

   case TYPE_BOOL:
      return (v1.val.vBool == FALSE &&  v2.val.vBool == TRUE);

   case TYPE_UINT:
      {
         UINT value;
         bool ok = v2.GetAsUInt( value );
         return v1.val.vUInt < value;
      }

   case TYPE_INT:
      {
         int value;
         bool ok = v2.GetAsInt( value );
         return v1.val.vInt < value;
      }

   case TYPE_SHORT:
      {
         int value;
         bool ok = v2.GetAsInt( value );
         return v1.val.vShort < short( value );
      }

   case TYPE_LONG:
      {
         int value;
         bool ok = v2.GetAsInt( value );
         return v1.val.vLong < value;
      }

   case TYPE_ULONG:
      {
         UINT value;
         bool ok = v2.GetAsUInt( value );
         return v1.val.vULong < value;
      }

   case TYPE_FLOAT:
      {
         float value;
         bool ok = v2.GetAsFloat( value );
         return v1.val.vFloat < value;
      }

   case TYPE_DOUBLE:
      {
         double value;
         bool ok = v2.GetAsDouble( value );
         return v1.val.vDouble < value;
      }

   case TYPE_PTR:
      return v1.val.vPtr < v2.val.vPtr;

   case TYPE_STRING:   
   case TYPE_DSTRING:
      {
         if ( v1.val.vString != NULL && v2.val.vString != NULL )
            return lstrcmp( v1.val.vString, v2.val.vString ) < 0;
         else if ( v1.val.vString == NULL && v2.val.vString != NULL )
            return true;
         else if ( v1.val.vString != NULL && v2.val.vString == NULL )
            return false;
         return false;
      }
   }

   return false;
}



int WideToChar( LPCTSTR wc, TCHAR** c )
   {
   // count the characters in the wide string
   int count = 0;
   TCHAR *p = (TCHAR*) wc;
   while ( *p != NULL )
      {
      p += 2;
      count++;
      }

   if ( count == 0 )
      {
      *c = NULL;
      return 0;
      }

   // allocate string
   p = new TCHAR[ count+1 ];
   *c = p;

   for ( int i=0; i < count; i++ )
      {
      *p = *(wc+(i*2));
      p++;
      }

   *p = NULL;
   return count;
   }


TYPE VData::Parse( LPCTSTR str )
   {
   if ( str == NULL || lstrlen( str ) == 0 )
      return ( type = TYPE_NULL );

   bool alpha  = false;
   bool number = false;
   bool dot    = false;

   int len = lstrlen( str );
   TCHAR *p = (TCHAR*) str;

   while ( *p == ' ' || *p == 10 || *p == 13 )  // strip leading whitespace
      p++;

   // is it a letter or '_'? );
   if ( __iscsymf( *p ) )
      alpha = true;

   else if ( isdigit( *p ) || *p == '+' || *p == '-' )
      {
      number = true;
      dot = _tcschr( p, '.' ) != NULL ? true : false;
      }

   if ( TYPE_DSTRING == type && val.vString != NULL )
      delete [] val.vString;

   if ( alpha ) 
      AllocateString( str );

   else if ( number )
      {
      if ( dot )
         {
         this->type = TYPE_FLOAT;
         this->val.vFloat = (float) atof( str );
         }
      else
         {
         this->type = TYPE_INT;
         this->val.vInt = atoi( str );
         }
      }
   else
      type = TYPE_NULL;

   return GetType();
   }


bool VData::ChangeType( TYPE newType )
   {
   if ( type == newType )  // no type change needed
      return true;

   // if dynamic string, remember ptr for deallocating
   TCHAR *dstr = NULL;
   if ( type == TYPE_DSTRING )
      dstr = val.vString;

   bool ok = false;

   switch( newType )
      {
      case TYPE_NULL:   type = TYPE_NULL; return true;

      case TYPE_CHAR:
         {
         TCHAR v;
         ok = GetAsChar( v );
         if ( ok )
            {
            val.vChar = v;
            type = newType;
            }
         break;
         }

      case TYPE_BOOL:
         {
         bool v;
         ok = GetAsBool( v );
         if ( ok )
            {
            val.vBool = v;
            type = newType;
            }
         break;
         }

      case TYPE_SHORT:
         {
         int v;
         ok = GetAsInt( v );
         if ( ok )
            {
            val.vShort = (short) v;
            type = newType;
            }
         break;
         }
         
      case TYPE_UINT:
      case TYPE_INT:
         {
         int v;
         ok = GetAsInt( v );
         if ( ok )
            {
            val.vInt = v;
            type = newType;
            }
         break;
         }
         
      case TYPE_ULONG:
      case TYPE_LONG:
         {
         int v;
         ok = GetAsInt( v );
         if ( ok )
            {
            val.vLong = v;
            type = newType;
            }
         break;
         }

      case TYPE_FLOAT:
         {
         float v;
         ok = GetAsFloat( v );
         if ( ok )
            {
            val.vFloat = v;
            type = newType;
            }
         break;
         }

      case TYPE_DOUBLE:
         {
         double v;
         ok = GetAsDouble( v );
         if ( ok )
            {
            val.vDouble = v;
            type = newType;
            }
         break;
         }
         
      case TYPE_PTR:
         type = newType;
         return true;

      case TYPE_STRING:
      case TYPE_DSTRING:
         {
         LPCTSTR v = GetAsString();    // always succeeds
         AllocateString( v );
         return true;
         }
      }

   if ( ok && dstr )
      delete [] dstr;

   return ok;
   }


bool VData::SetUseWideChar( bool useWideChar )
   {
   bool oldVal = m_useWideChar; 
   m_useWideChar = useWideChar; 
   return oldVal;
   }


bool VData::GetUseWideChar( void )
   {
   return m_useWideChar;
   }


void VData::SetFormatString( LPCTSTR string )
   {
   lstrcpy( m_formatString, string );
   }
