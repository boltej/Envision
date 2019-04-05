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

#include "ARRAY.HPP"


Array::Array()
   {
   //-- initialize error handler --//
   //void (*(Array::lpFnErrorHandler))( int ) = DefaultHandler;
   lpFnErrorHandler = DefaultHandler;

   //-- class-global constant initialization --//
   AllocIncr = 256;   // 128 bytes per allocation 
   }

//-- default exception handler --//
void DefaultHandler( int err )
   {
   switch (err)
     {
     case AE_ALLOC :
        //error_msg("memory allocation failure");
        break;
     case AE_TOO_LONG :
        //error_msg( "maximum size exceeded" );
     break;
     }
   return;
   }


