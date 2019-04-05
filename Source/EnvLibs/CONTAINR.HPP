//===================================================================
// CONTAINR.HPP
//
//   - definitions for container objects
//
//     1) Container (abstract class)
//     2) TDList - Doubly link list of void pointers
//===================================================================
#pragma once
#include "libs.h"


#include "Report.h"

//--------------------------- Constants ---------------------------//


const int CO_FAILURE = 0;
const int CO_SUCCESS = 1;


//---------------------------- Container --------------------------//


class Container
   {
   protected:
      UINT length;                 // number of items in this list

   public:
      //-- constructors --//
      Container();
      Container( Container & c );

      virtual ~Container( void ) { }

      //-- operators --//
      void operator = ( Container & c );    // assignment operator

      //- get/set methods --//
      UINT Length() { return length; }       // get number of items

   };



//------------------------------ Inlines --------------------------//


//-- Container() ----------------------------------------------------
//
//-- constructors
//-------------------------------------------------------------------

inline
Container::Container()
   {
   length = 0;
   }

//--- copy constructor ---//
inline
Container::Container( Container & c )
   {
   length = c.length;
   }

