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
#pragma once

///=======================================================================================
// The random path generation procedure of Srivastava:
//   as in GSLIB::anneal.f  

// History: 
//   *The range of the indices has been changed to have base-0 not base-1 to be like
//    C/C++ and not FORTRAN.  Tested by mrg on 10-24-2003.
//   *Made first(.) idiot proof. Will always return a value in [0,nxyz)
//   *Added default constructor and the SetMod function.
//
// Usage:  
//   *Construct one of these; 
//   *Use first(rand) to get first.
//   *Use next() to get the next index; using next overwrites the stored index.
//

#include "EnvLibs.h"


class LIBSAPI SriRandomPath 
   {
   long nxyz, modlus, index;

   public:
      SriRandomPath( long nx, long ny, long nz=1 ) : nxyz( nx*ny*nz ), index( -1 ) 
         { ASSERT( nxyz > 0 ); onctor(); }
      SriRandomPath( long N ) : nxyz( N ), index( -1 ) 
         { ASSERT( nxyz > 0 ); onctor(); }
      SriRandomPath() : nxyz( 0 ), modlus( -1 ), index( -1 ) 
         { }

      void SetMod( long N ){ nxyz = N; ASSERT( nxyz > 0 ); onctor(); }

      long first( double drandn ) 
         {
         if ( drandn < 0 )
            drandn = 0;
         index = (long)(drandn*double(nxyz)); 
         return next(); 
         }

      long next()   
         {
         ASSERT( index  >= 0 );
         ASSERT( modlus >= 0 );
         while (1) 
            {
            index = (5*index+1) % modlus;
            if ( index >= nxyz )   
               continue;
            return index;
            }
         }  

   protected:  
      void onctor()  
         { 
         modlus = 1;
         while(modlus <= nxyz) 
            modlus*=2;
         }
   };

