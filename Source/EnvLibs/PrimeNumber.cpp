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

#include "PrimeNumber.h"


//--------------------------------------------------------------------
// overload the output << operator of stream i/o to print prime number 
// use a reference to an ostream and this class 'prime' and return an 
// ostream reference
//--------------------------------------------------------------------
//ostream& operator << (ostream& strm_ref, prime &prm_in)
//{
//   return strm_ref << prm_in.get_prime();
//}  

//--------------------------------------------------------------------
// here where we create an new prime number upon invocation
// the overloaded ++ operator returns a new prime number
// To the user of the class it is as easy as adding one to integer.
//--------------------------------------------------------------------

PrimeNumber& PrimeNumber::operator++()
   {  
   ++m_primeCount;  // how many prime number so far?
   // test for 2
   if ( m_primeNumber == 2 )
      {
   	++m_primeNumber;
	   return (*this);
      }

   for( ; ; )
      {
      m_primeNumber += 2;
      int prime_found = 1;
      
      int max_times = ( m_primeNumber / 2 );
      
      for ( int i = 3; i < max_times; ++i )
	      {
	      if ( ( ( m_primeNumber / i ) * i ) == m_primeNumber )
	         {
		      prime_found = 0;
		      break;
	         }
	      }
      
      if ( prime_found ) 
	      break;
      }

   return ( *this );
   }
