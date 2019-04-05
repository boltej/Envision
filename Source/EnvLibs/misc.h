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

#include <string>
#include <stdio.h>


#include "randgen/Randunif.hpp"

#ifdef Yield
#undef Yield
#endif

#ifndef NO_MFC
inline
void YieldMsg()
   {
   MSG  msg;
   while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
      {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      }
   }
#endif
// e.g MAKELONG type macro
// 
// Usage:
// 
// WORD wLo = 0xAA55U, wHi = 0x55AAU;
// ASSERT (MAKE<DWORD> (wLo, wHi) == 0x55AAAA55);

template<typename DOUBLE, typename SINGLE> 
DOUBLE MAKE (const SINGLE lo, const SINGLE hi)
   {
   static_assert (sizeof (DOUBLE) == (2 * sizeof (SINGLE)), "Unmatched");
   return ((static_cast<DOUBLE> (hi) << (CHAR_BIT * sizeof (SINGLE))) | lo);
   }


template <class T>
void ShuffleArray(T *a, INT_PTR size, RandUniform *pRand )
   {
   //--- Shuffle elements by randomly exchanging each with one other.
   for ( int i=0; i < size-1; i++ )
      {
      int randVal = (int) pRand->RandValue( 0, size-i-0.0001f );

      int r = i + randVal; // Random remaining position.
      T temp = a[ i ];
      a[i] = a[r];
      a[r] = temp;
      }
   }

inline
int MonteCarloDraw( RandUniform *pRand, float probs[], int count, bool shuffleProbs=false )
   {
   if ( shuffleProbs )
      ShuffleArray( probs, count, pRand );

   float randVal = (float) pRand->RandValue();

   float sumSoFar = 0;
   for ( int i=0; i < count; i++ )
      {
      sumSoFar += probs[ i ];

      if ( randVal <= sumSoFar )
         return i;
      }

   return -1; 
   }


inline
int MonteCarloDraw( RandUniform *pRand, CArray< float, float > &probs, bool shuffleProbs=false )
   {
   if ( shuffleProbs )
      ShuffleArray( probs.GetData(), probs.GetCount(), pRand );

   float randVal = (float) pRand->RandValue();

   float sumSoFar = 0;
   for ( int i=0; i < (int) probs.GetCount(); i++ )
      {
      sumSoFar += probs[ i ];

      if ( randVal <= sumSoFar )
         return i;
      }

   return -1; 
   }


inline
bool IsPrime(int number)
   {	// Given:   num an integer > 1
	// Returns: true if num is prime
	// 			false otherwise.	
   for (int i=2; i < number; i++)
	   {
		if (number % i == 0)
			return false;
	   }
	
	return true;	
   }



inline
bool FileToString(LPCTSTR filename, std::string &str)
   {
   FILE *fp;
   errno_t err;
   if ((err = fopen_s(&fp, filename, "rb")) != 0)
      return false;

   std::fseek(fp, 0, SEEK_END);
   str.resize(std::ftell(fp));
   std::rewind(fp);
   std::fread(&str[0], 1, str.size(), fp);
   std::fclose(fp);
   return true;
   }
   


#ifdef __cplusplus
extern "C" {
#endif

void LIBSAPI GetTileParams( int count, int &rows, CArray<int,int> &colArray );
int  LIBSAPI GetTileRects( int count, CRect *parentRect, CArray< CRect, CRect& > &rects );

int  LIBSAPI CleanFileName(LPCTSTR filename);
int  LIBSAPI Tokenize( const TCHAR* str, const TCHAR* delimiters, CStringArray &tokens);
#ifdef __cplusplus
}
#endif
