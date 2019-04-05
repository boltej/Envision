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
#include "EnvLibs.h"

/*
usage:

int main ()
   { 
   const int no_of_primes = 100;
   //------------------------------
   //  here is our prime class
   //------------------------------
   PrimeNumber  prm_num;
   cout << "generate " << no_of_primes << " prime numbers " << "\n";

   while ( prm_num.get_nth_prime() <= no_of_primes )
      {
      cout << "prime number " << prm_num.get_nth_prime() 
	   << "\t" << prm_num << "\n";
	   // get the next prime with the overloaded ++ operator 
	   ++prm_num;
      }

   cout << " end of program " << "\n";
   return (0);
   }
*/


class LIBSAPI PrimeNumber
{
protected:
   int     m_primeCount;
   int     m_primeNumber;

 public : 
    // constructor
    PrimeNumber ( int num_in = 1, int prime_in = 2 ) 
      {
	   m_primeCount   = num_in;
	   m_primeNumber  = prime_in;
      }

    // get the next prime number by overloading the ++ operator
    PrimeNumber &operator++();

    // accessor member functions
    int GetNthPrime() { return ( m_primeCount ); }

    int GetPrime() { return ( m_primeNumber ); 	}
};
