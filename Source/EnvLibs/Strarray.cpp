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

#include "STRARRAY.HPP"
#include <string.h>


#define SA_ALLOCINCR  ( 16 )


//
////-- copy constructor --//
//StringArray::StringArray( StringArray& stringArray )
//   {
//   length      = stringArray.length;
//   allocLength = stringArray.allocLength;
//
//   array = new char*[ stringArray.allocLength ];
//
//   for ( UINT i=0; i < stringArray.Length(); i++ )
//      {
//      array[ i ] = NULL;
//      SetString( i, stringArray[ i ] );
//      }
//   }
//
//
//StringArray::~StringArray( void )
//   {
//   Clear();
//   }
//
//
//void StringArray::Clear( void )
//   {
//   if ( array )
//      {
//      for ( UINT i=0; i < length; i++ )
//         delete [] array[ i ];
//
//      if ( length > 0 )
//         delete [] array;
//      }
//
//   array  = NULL;
//   length = allocLength = 0;
//   }
//
//
//void StringArray::Delete( UINT pos )
//   {
//   if ( ! array || pos >= length )
//      return;
//
//   if ( length == 1 )
//      {
//      Clear();
//      return;
//      }
//
//   while ( pos+1 < length )
//      {
//      array[ pos ] = array[ pos+1 ];
//      pos++;
//      }
//
//   length--;
//
//   //memory corruption error, Oct 2000, D. Ernst
//   //allocLength can go to zero
//   //allocLength -= SA_ALLOCINCR;
//   allocLength = length + 1;
//
//   char **pTemp = new char* [ allocLength ];
//   memcpy( pTemp, array, length*sizeof( char* ) );
//
//   delete [] array;
//   array = pTemp;
//   }
//
//
//void StringArray::AddString( const char * const string )
//   {
//   //-- insufficient ptr allocation? --//
//   if ( length >= allocLength )
//      {
//      allocLength += SA_ALLOCINCR;
//      char **pTemp = new char* [ allocLength ];
//
//      if ( array )
//         {
//         memcpy( pTemp, array, length*sizeof( char* ) );  // copy array
//         delete [] array;
//         }
//
//      array = pTemp;
//      }
//
//   array[ length ] = new char[ strlen( string ) + 1 ];
//
//   //-- allocation taken care of, now copy string --//
//   strcpy( array[ length ], string );
//
//   length++;
//
//   return;
//   }
//
//
//BOOL StringArray::SetString( UINT offset, const char * const string )
//   {
//   if ( offset >= length )
//      {
//      while ( offset > length )
//         AddString( "" );
//
//      AddString( string );
//
//      return FALSE;
//      }
//
//   // same string?
//   if ( array[ offset ] != NULL )
//      {
//      if ( array[ offset ] == string ) // same string?  then do nothing
//         return TRUE;
//      else
//         delete [] array[ offset ];    // otherwise, delete existing string
//      }
//
//   array[ offset ] = new char[ strlen( string ) + 1 ];
//
//   //-- allocation taken care of, now copy string --//
//   strcpy( array[ offset ], string );
//
//   return TRUE;
//   }
//
//
//
//const char * const StringArray::GetString( UINT offset )
//   {
//   if ( offset >= length )
//      return NULL;
//
//   else
//      return (const char * const) array[ offset ];
//   }
//
//














