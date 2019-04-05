//--------------------------------------------------------------------------
//   PROGRAM: tarray.hpp
//
//   PURPOSE: Provides a general dynamic template array class
//--------------------------------------------------------------------------

#if !defined _TARRAY_HPP
#define      _TARRAY_HPP 1

#include "libs.h"


#if !defined ( _ARRAY_HPP )
#include "ARRAY.HPP"
#endif

#if !defined _SERIAL_HPP
#include "SERIAL.HPP"
#endif


#if !defined min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

template <class T>
class  TArray : public Array , public Serial
	{
	protected:
		T *array;               // pointer to array
      void Shrink( void );    // shrink Array to minimum allocation

   public:
      // constructor
      TArray( void );
      TArray( TArray<T> &tArray );                // use TArray
      TArray( T *array, UINT count );     // use  a "c" Array
      TArray( UINT count, T value );     // fill it with value
      TArray( UINT count );               // fill it with garbage (careful!)

      // destructor
		virtual ~TArray( void ) { delete [] array; }

      // operators
      T& operator [] (UINT pos) { return array[ pos ]; }
      T& operator () (UINT pos) { return array[ pos ]; }

      // create a T ptr from Array (usage: )
      T *GetBase( void ) { return array; }

      TArray &operator = ( TArray<T> &tArray );             // assignment
      void operator   += ( TArray<T> &tArray );             // concatenation
      void operator   += ( T value ) { Append( value ); }

      T   &Get( UINT pos ) { return array[ pos ]; }
      void Set( UINT pos, T &value ) { array[ pos ] = value; }

      UINT Copy( T array[], UINT count );

      UINT Delete( UINT pos, UINT count=1 );
      UINT DeleteData( T );
      UINT Clear( void );              // clears all element in array

      UINT Insert( UINT pos, T value );         // insert value
      UINT Insert( UINT pos, TArray<T> &tArray );   // or array

      UINT Append( T value );                   // append to end

      void Fill( T value );                     // fill existing array
      void ReFill( T value, UINT count );       // fill new array
      UINT Resize( UINT count );                 // allocate new array

      //-- Serial virtual methods
      virtual BOOL Serialize( void );

   };


//---------------------- TArray Methods ---------------------------//


//-- private function to shrink the size of an allocated TArray --//

template <class T>
void TArray<T>::Shrink( void )
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
TArray<T>::TArray( void ) : Serial()
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
TArray<T>::TArray( TArray< T > &tArray ) : Serial()
    {
    length = tArray.length;
    size   = tArray.size;

    array = new T[ size/sizeof( T ) ];

    if ( ! array )
        lpFnErrorHandler(AE_ALLOC);

    for ( UINT i=0; i < length; i++ )
       array[i] = tArray.array[i];
    }



//---------------------------------------------------
// construct from "c" T array
//---------------------------------------------------
template <class T>
TArray<T>::TArray( T *tArray, UINT count ) : Serial()
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
TArray<T>::TArray( UINT count, T value ) : Serial()
   {
    length = count;
    size = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

    array = new T[ size/sizeof( T ) ];

    if ( ! array )
       lpFnErrorHandler( AE_ALLOC );

    for ( UINT i=0; i < length; i++ )
       array[i] = value;
   }


//---------------------------------------------------
// fill with 0
//---------------------------------------------------

template <class T>
TArray<T>::TArray( UINT count ) : Serial()
   {
    length = count;
    size = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

    array = new T[ size/sizeof( T ) ];

    if ( ! array )
       lpFnErrorHandler( AE_ALLOC );

    memset( array, 0, size );
    }




//---------------------------------------------------
// Assignment operator
//---------------------------------------------------
template <class T>
TArray<T> & TArray<T>::operator = ( TArray< T > &tArray )
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
void TArray<T>::operator += ( TArray< T > &tArray )
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

    UINT i = 0;

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


//---------------------------------------------------
// copy from an array
//---------------------------------------------------

template <class T>
UINT TArray<T>::Copy( T tArray[], UINT count )
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
UINT TArray<T>::Delete( UINT pos, UINT count )
   {
   if ( pos > length )
       return 0;

   if ( pos+count > length )
      count = length - pos;
   
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
UINT TArray<T>::DeleteData( T data )
   {
   for ( UINT i=0; i < length; i++)
      if ( ! memcmp( &array[ i ], &data, sizeof( T ) ) )
         {
         Delete( i, 1 );  // delete this index
         return TRUE;
         }

   return FALSE;
   }





//-- TArray::Clear() ------------------------------------------------
//
//-- return: number of items deleted
//-------------------------------------------------------------------

template <class T>
UINT TArray<T>::Clear( void )
   {
   UINT _length = length;

   length = 0;

   Shrink();

   return _length;
   }



//---------------------------------------------------
// insert value
//---------------------------------------------------

template <class T>
UINT TArray<T>::Insert( UINT pos, T value )
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
         temp[ i ] = array[ i ];

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
// insert TArray
//---------------------------------------------------

template <class T>
UINT TArray<T>::Insert( UINT pos, TArray< T > &tArray )
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
//---------------------------------------------------

template <class T>
UINT TArray<T>::Append( T value )
   {
   int tSize = sizeof( T );

   if ( ( tSize * (length+1) ) > size )    // need more space?
      {
      size  += AllocIncr;
      T *temp  = new T[ size / tSize ];

      size = (size/tSize) * tSize;

      if ( ! temp )
         {
         lpFnErrorHandler( AE_ALLOC );
         return 0;
         }

      for ( UINT i=0; i < length; i++ )
         temp[ i ] = array[ i ];

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
void TArray<T>::Fill( T value )
   {
   for ( UINT i=0; i < length; i++ )
      array[i] = value;
   return;
   }


//---------------------------------------------------
// Create (clear) and fill with value
//---------------------------------------------------

template <class T>
void TArray<T>::ReFill( T value, UINT count )
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


//---------------------------------------------------
// Resize the array, trashing existing data 
//---------------------------------------------------

template <class T>
UINT TArray<T>::Resize( UINT count )
   {
#ifdef NO_MFC
     using namespace std; //for min
#endif

   //-- remeber old data for a little while --//
   UINT oldLength = length;

   length = count;
   size   = ( ( (length*sizeof( T )) + AllocIncr ) / AllocIncr ) * AllocIncr;

   int newLength = size/sizeof( T );

   T *newArray = new T[ newLength ];

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

   for ( UINT i=0; i < copyLength && i < (UINT) newLength; i++ )
      newArray[ i ] = array[ i ];

   delete [] array;
   array = newArray;

   return length;
   }



template <class T>
BOOL TArray<T>::Serialize( void )
   {
   if ( Serial::Serialize() == FALSE )  // take care of parent stuff
      return FALSE;

   if ( IsReading() )
      {
      UINT count;            // current length
      if ( fread( (void*) &count, sizeof( UINT ), 1, fp ) != 1 )
         return FALSE;

      Resize( count );

      return ( fread( (void*) &array, sizeof( T ), count, fp ) == count );
      }

   else  // writing
      {
      //-- first write the length of the array --//
      fwrite( (const void*) &length, sizeof( length ), 1, fp );

      //-- now, write all it data --//
      fwrite( (const void*) array, sizeof( T ), length, fp );

      return TRUE;
      }
   }


//---------------------------------------------------
// print array to cout
//---------------------------------------------------
/*-------------------

#include <iostream>

template <class T>
void TArray<T>::Print( void )
  {
   if ( length == 0 ) return;

   for ( UINT i=0; i < (length-1); i++ )
      cout << array[i] << ", ";

   cout << array[i];
   return;
   }

---------*/



#endif

