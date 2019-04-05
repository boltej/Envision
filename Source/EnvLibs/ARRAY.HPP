//--------------------------------------------------------------------------
//   PROGRAM: array.hpp
//
//   PURPOSE: Provides a general dynamic template array class parent
//--------------------------------------------------------------------------
#pragma once
#include "libs.h"

#define  AE_ALLOC     0    // exception errors
#define  AE_TOO_LONG  1

#if !defined ( _TYPEDEFS_H )
#include "Typedefs.h"
#endif


class   LIBSAPI  Array
   {
   protected:
      UINT size;              // allocated size
      UINT length;            // current length
      void (*lpFnErrorHandler)( int err );   // exception handler
      UINT AllocIncr;

   public:
      // value return methods
      UINT Length( void ) { return length; }
      UINT Size( void )   { return size; }

      Array( void );
      virtual ~Array( void ) { }

      // Assign an exception handler
      void SetErrorHandler(void (*lpFnUserHandler)( int ))
            {  lpFnErrorHandler = lpFnUserHandler; };
   };



//-------------------- Array Methods (abstract) -------------------//

//-- Prototype for default error handler --//
void DefaultHandler( int err );

