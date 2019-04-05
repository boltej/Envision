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
// BitArray.h  A BLATANT AND INSUFFICIENT REINVENTION OF THE WHEEL, std::vector<bool>.

#pragma once
#include "EnvLibs.h"

class LIBSAPI BitArray
   {
   public:
      BitArray();
      BitArray( UINT sizeBits, bool bitValue = false );
      BitArray(const BitArray& ba){ *this = ba; };
      ~BitArray();

   private:
   
      BitArray& operator=( const BitArray& ba ){ Copy( ba ); return *this; }

   public:

      class Bit // helper class that is used like a reference to a bit
         {
         friend class BitArray;

         private:
            Bit( BitArray *pBitArray, UINT bitPos ) : m_pBitArray( pBitArray ), m_bitPos( bitPos ) {}

         BitArray *m_pBitArray;
         UINT    m_bitPos;

         public:
            Bit& operator=( bool bitVal )
               {
               m_pBitArray->Set( m_bitPos, bitVal );
               return (*this);
               }

            Bit& operator=( const Bit& bitRef )
               {
               m_pBitArray->Set( m_bitPos, bool(bitRef) );
               return (*this);
               }

            bool operator~() const
               {
               return ( ! m_pBitArray->Test( m_bitPos ) );
               }

            operator bool() const
               {
               return ( m_pBitArray->Test( m_bitPos ) );
               }
         };

   public:
      void Set( UINT bitPos, bool bitValue );
      void SetAll( bool bitValue );
      void SetLength( UINT sizeBits, bool bitValue = false ); // added bits are set to bitValue

      bool Get( UINT bitPos ) const;
      Bit  Get( UINT bitPos );

      UINT GetSize() const { return m_sizeBits; }   // returns number of bits in the array
      UINT GetLength() const { return m_sizeBits; } // returns number of bits in the array

      void Copy( const BitArray& ba );

      CString GetString() const;  // returns a string like "1011100100001"

      short GetShort( int i ) { return *(((short*)m_pWordArray)+i); }
      long  GetLong( int i ) { return *(m_pWordArray+i); }

      bool operator[]( UINT bitPos ) const
         {
         return Get( bitPos );
         }

      Bit operator[]( UINT bitPos )
         {
         return Get( bitPos );
         }

   private:
      UINT m_sizeBits;
      UINT m_sizeWords;

      DWORD *m_pWordArray;

      enum
         {
         BitsPerWord  = CHAR_BIT * sizeof( DWORD ),
         CharsPerWord = sizeof( DWORD )
         };

   private:
      bool Test( UINT bitPos ) const;
   };


