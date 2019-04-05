//--------------------------------------------------------------------------
//   PROGRAM: tparray.hpp
//
//   PURPOSE: Provides a general dynamic template array class for pointers
//            only
//--------------------------------------------------------------------------

#pragma once

#include "libs.h"

#include "Report.h"
#include "typedefs.h"
#include "stdlib.h"
#include "array.hpp"

const UINT  TA_NOTFOUND = 0xFFFF;


template <class T>
class  TPtrArray : public Array
   {
   protected:
      T *array;               // pointer to array
      void Shrink( void );    // shrink Array to minimum allocation

      BOOL deleteOnDelete;

   public:
      // constructor
      TPtrArray( void );
      TPtrArray( TPtrArray &T );             // use TPtrArray
      TPtrArray( T *array, UINT count );     // use  a "c" Array
      TPtrArray( UINT count, T value );      // fill it with value

      // destructor
      virtual ~TPtrArray( void );

      // operators
      T& operator [] ( UINT pos ) { return Get( pos ); }
      T& operator () ( UINT pos ) { return Get( pos ); }

      // create a T ptr from Array (usage: )
      T *GetBase( void ) { return array; }

      TPtrArray<T> &operator = ( TPtrArray<T> &t );     // assignment
      void operator      += ( TPtrArray<T> &t );        // concatenation
      void operator      += ( T value ) { Append( value ); }

      void SetDeleteOnDelete( BOOL _deleteOnDelete )
                               { deleteOnDelete = _deleteOnDelete; }

      T   &Get( UINT pos );
      void Set( UINT pos, T value );

      UINT Copy( T array[], UINT count );

      UINT Delete( UINT pos, UINT count = 1 );
      UINT Delete( T value );
      UINT Clear ( void );                // clears all element in array
      BOOL Find  ( T value, UINT *location );

      UINT Insert( UINT pos, T value );                // insert value
      UINT Insert( UINT pos, TPtrArray<T> &t );        // or array

      UINT Append( T value );                          // append to end

      void Fill( T value );                            // fill existing array
      void Fill( T value, UINT count );                // fill new array

      UINT Resize( UINT count );                       // resize array

      // print array to cout
      //void Print( void );
   };


//---------------------- TPtrArray Methods ---------------------------//


//-- private function to shrink the size of an allocated TPtrArray --//

template <class T>
void TPtrArray<T>::Shrink( void )
   {
   if ( (size-(length*sizeof( T ))) > AllocIncr ) // room available to shrink?
      {
      size = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

      T *temp = new T[ size/sizeof( T ) ];

      if ( temp )
          {
          for ( UINT i=0; i < length; i++ )
             temp[i] = array[i];

          delete [] array;
          array = temp;
          }
      else
          lpFnErrorHandler( AE_ALLOC );
      }
  }


//-- constructors --------------------------------------------//

//---------------------------------------------------
// default constructor
//---------------------------------------------------

template <class T>
TPtrArray<T>::TPtrArray( void )
   : deleteOnDelete( TRUE )
   {
   length = 0;
   size = AllocIncr;

   array = new T[ AllocIncr / sizeof( T ) ];

   if ( ! array )
      lpFnErrorHandler(AE_ALLOC);
   }


//---------------------------------------------------
// Copy constructor
//---------------------------------------------------

template <class T>
TPtrArray<T>::TPtrArray( TPtrArray< T > &tArray )
   : deleteOnDelete( tArray.deleteOnDelete )
	{
	length = tArray.length;
	size   = tArray.size;

	array = new T [ size/sizeof( T ) ];

   if ( ! array )
       lpFnErrorHandler( AE_ALLOC );

   //-- copy all the pointers (NOTE: DON'T copy the objects pointed to!) --//
   for ( UINT i=0; i < length; i++ )
      array[ i ] = tArray.array[ i ];  
   }


//---------------------------------------------------
// construct from "c" T array
//---------------------------------------------------
template <class T>
TPtrArray<T>::TPtrArray( T *tArray, UINT count )
   : deleteOnDelete( TRUE )
   {
   length = count;
   size = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

   array = new T[ size/sizeof( T ) ];

   if ( ! array )
      lpFnErrorHandler( AE_ALLOC );

   for ( UINT i=0; i < length; i++ )
      array[i] = tArray[i];
   }


//---------------------------------------------------
// fill with a given value
//---------------------------------------------------
template <class T>
TPtrArray<T>::TPtrArray( UINT count, T value )
   : deleteOnDelete( TRUE )
   {
   length = count;
   size = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

   array = new T[ size/sizeof( T ) ];

   if ( ! array )
      lpFnErrorHandler( AE_ALLOC );

   for ( UINT i=0; i < length; i++ )
      array[i] = value;
   }


// destructor
template <class T>
TPtrArray< T >::~TPtrArray( void )
	{
	if (array == NULL )
		return;

	if ( deleteOnDelete )
		{
		for ( UINT i = 0; i < length; i++ )
			if ( array[ i ] != NULL )
				delete array[ i ];
		}

   delete [] array;
   }


//---------------------------------------------------
// Assignment operator
//---------------------------------------------------
template <class T>
TPtrArray<T> &TPtrArray<T>::operator = ( TPtrArray< T > &tArray )
   {
   length = tArray.length;
   size   = tArray.size;

   delete [] array;

   array = new T[ size/sizeof( T ) ];

   if ( ! array )
      {
      lpFnErrorHandler( AE_ALLOC );
      return *this;
      }

   for ( UINT i=0; i < length; i++ )
      array[i] = tArray.array[i];

   return *this;
   }


//---------------------------------------------------
// append operator
//---------------------------------------------------

template <class T>
void TPtrArray<T>::operator += ( TPtrArray< T > &tArray )
    {
    unsigned long totalLength;
    UINT newLength, newSize;

    totalLength = tArray.length + length;

    //if (totallength > UINT_MAX)
    //    lpFnErrorHandler(SE_TOO_LONG);

    newLength  = (UINT) totalLength;

    newSize = ( ( (newLength*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

    T *temp = new T[ newSize ];

    if ( ! temp )
       {
       lpFnErrorHandler( AE_ALLOC );
       return;
       }

    UINT i=0;

    for ( i=0; i < length; i++ )
       temp[i] = array[i];

    delete [] array;
    array = temp;

    for ( i=0; i < tArray.length; i++ )
       array[ length+i ] = tArray.array[i];

    length = newLength;
    size   = newSize;

    Shrink();

    return;
    }



template <class T>
T &TPtrArray< T >::Get( UINT pos )
   {
   if ( pos >= length )
      {
      Report::ErrorMsg( "Position exceeds bounds in TPtrArray::Get()" );
      return array[ 0 ];
      }

   else
      return array[ pos ];
   }


template <class T>
void TPtrArray< T >::Set( UINT pos, T value )
   {
   if ( pos >= length )
      {
      Report::ErrorMsg( "Position exceeds bounds in TPtrArray::Set()" );
      return;
      }

   array[ pos ] = value;
   }



//---------------------------------------------------
// copy from an array
//---------------------------------------------------

template <class T>
UINT TPtrArray<T>::Copy( T tArray[], UINT count )
   {
   length = count;
   size = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;
   
   delete [] array;

   array = new T[ size/sizeof( T ) ];

   if ( ! array )
      lpFnErrorHandler( AE_ALLOC );

   for ( UINT i=0; i < length; i++ )
      array[i] = tArray[i];

   return length;
   }


//---------------------------------------------------
// Delete element(s)
//
//-- return: number of items deleted
//---------------------------------------------------

template <class T>
UINT TPtrArray< T >::Delete( UINT pos, UINT count )
   {
   if ( pos > length )
       return 0;

   if ( pos+count > length )
      count = length - pos;

   if ( deleteOnDelete )
      {
      for ( UINT i=0; i < count; i++ )
         delete array[ pos+i ];
      }

   while ( pos+count < length )
      {
      array[ pos ] = array[ pos+count ];
      pos++;
      }

   length -= count;
   Shrink();

   return count;
   }


template <class T>
UINT TPtrArray< T >::Delete( T value )
   {
   UINT offset;
   
   if ( Find( value, &offset ) == FALSE )
      return TA_NOTFOUND;

   else
      return Delete( offset, 1 );
   }


//-- TPtrArray::Clear() ------------------------------------------------
//
//-- return: number of items deleted
//-------------------------------------------------------------------

template <class T>
UINT TPtrArray< T >::Clear( void )
   {
   UINT _length = length;

   if ( deleteOnDelete )
      for ( UINT i=0; i < length; i++ )
        delete array[ i ];

   length = 0;

   Shrink();

   return _length;
   }


//---------------------------------------------------
// insert value
//---------------------------------------------------

template <class T>
UINT TPtrArray<T>::Insert( UINT pos, T value )
   {
   if ( pos > length )
      return 0;

   UINT i;

   if ( (length*sizeof( T )) == size )    // need more space?
      {
      size  += AllocIncr;
      T *temp  = new T[ size/sizeof( T ) ];

      if ( ! temp )
         {
         lpFnErrorHandler( AE_ALLOC );
         return 0;
         }

      for ( i=0; i < length; i++ )
         temp[i]=array[i];

      delete [] array;
      array = temp;
      }

   for ( i=length+1; i > pos; --i)
       array[i] = array[i-1];

   array[pos] = value;
   ++length;

   return length;
   }


//---------------------------------------------------
// insert TPtrArray
//---------------------------------------------------

template <class T>
UINT TPtrArray<T>::Insert( UINT pos, TPtrArray< T > &tArray )
   {
   //unsigned long totallength = TArray.length + length;
   //if ( totallength > UINT_MAX )
   //   lpFnErrorHandler(SE_TOO_LONG);

   UINT sLength = tArray.length;

   if ( sLength > 0 )
      {
      for ( UINT i = 0; i < sLength; ++i )
         {
         if ( Insert(pos, tArray.array[i]) == 0 )
            return 0;   // error!
         ++pos;
         }
      }
   return length;
   }



//---------------------------------------------------
// Append value
//
//-- DOESN'T copy item pointed to only pointer...
//-- returns length of array after appending...
//---------------------------------------------------

template <class T>
UINT TPtrArray<T>::Append( T value )
   {
   if ( (length*sizeof( T )) == size )    // need more space?
      {
      size  += AllocIncr;
      T *temp  = new T[ size/sizeof( T ) ];

      if ( ! temp )
         {
         lpFnErrorHandler( AE_ALLOC );
         return 0;
         }

      for ( UINT i=0; i < length; i++ )
         temp[i]=array[i];

      if ( array != NULL )
         delete [] array;

      array = temp;
      }

   array[length++] = value;

   return length;
   }


//---------------------------------------------------
// fill array with value
//---------------------------------------------------

template <class T>
void TPtrArray<T>::Fill( T value )
   {
   for ( UINT i=0; i < length; i++ )
      array[i] = value;
   return;
   }


//---------------------------------------------------
// Create (clear) and fill with value
//---------------------------------------------------

template <class T>
void TPtrArray<T>::Fill( T value, UINT count )
   {
   length = count;
   size = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

   delete [] array;

   array = new T[ size/sizeof( T ) ];

   if ( ! array )
      lpFnErrorHandler( AE_ALLOC );

   for ( UINT i=0; i < length; i++ )
      array[i] = value;
   return;
   }


template <class T>
UINT TPtrArray<T>::Resize( UINT count )
   {
   //-- remember old data for a little while --//
   UINT oldLength = length;

   length = count;
   size   = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

   T *newArray = new T[ size/sizeof( T ) ];

   if ( ! newArray )
      {
      lpFnErrorHandler( AE_ALLOC );
      delete [] array;
      array  = NULL;
      length = 0;
      return 0;
      }

   //-- copy any valid data over before deleteing old array --//
   UINT copyLength = min( length, oldLength );
   UINT i = 0;

   for ( i=0; i < copyLength; i++ )
      newArray[ i ] = array[ i ];

   if ( deleteOnDelete == TRUE )
      {
      //-- if fewer elements in new array, delete any excess (old) objects --//
      if ( length < oldLength )
         for ( i=copyLength; i < oldLength; i++ )
            delete array[ i ];

      //-- if more element in new array, set to NULL to avoid problems --//
      else if ( length > oldLength )
         for ( i=copyLength; i < length; i++ )
            newArray[ i ] = NULL;
      }

   delete [] array;
   array = newArray;

   return length;
   }


//---------------------------------------------------
// print array to cout
//---------------------------------------------------
/*-------------------

#include <iostream>

template <class T>
void TPtrArray<T>::Print( void )
  {
   if ( length == 0 ) return;

   for ( UINT i=0; i < (length-1); i++ )
      cout << array[i] << ", ";

   cout << array[i];
   return;
   }

---------*/



template <class T>
BOOL TPtrArray< T >::Find( T value, UINT *offset )
  {
   if ( length == 0 )
      return FALSE;

   for ( UINT i=0; i < length; i++ )
      {
      if ( array[i] == value )
         {
         if ( offset != NULL )
            *offset = i;
         return TRUE;
         }
      }

   return FALSE;
   }

