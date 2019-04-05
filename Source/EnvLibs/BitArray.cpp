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
// BitArray.cpp

#include "EnvLibs.h"
#pragma hdrstop

#include "BitArray.h"

BitArray::BitArray()
:  m_sizeBits( 0 ),
   m_sizeWords( 0 ),
   m_pWordArray( NULL )
   {
   }

BitArray::BitArray( UINT sizeBits, bool bitValue /*= false*/ )
:  m_sizeBits( 0 ),
   m_sizeWords( 0 ),
   m_pWordArray( NULL )
   {
   SetLength( sizeBits );   
   SetAll( bitValue );
   }

BitArray::~BitArray(void)
   {
   if ( m_pWordArray )
      delete [] m_pWordArray;
   }

void BitArray::Copy( const BitArray& ba )
   {
   SetLength( ba.m_sizeBits );
   ASSERT( m_sizeWords == ba.m_sizeWords );

   if ( m_sizeBits > 0 )
      memcpy( (void*) m_pWordArray, (const void*) ba.m_pWordArray, sizeof( ba.m_pWordArray ) );
   }

void BitArray::Set( UINT bitPos, bool bitValue  )
   {
   ASSERT( bitPos < m_sizeBits );

   UINT index = bitPos/BitsPerWord;
   DWORD  mask  = DWORD(1) << bitPos % BitsPerWord;

   if ( bitValue )
      m_pWordArray[ bitPos/BitsPerWord ] |= DWORD(1) << bitPos % BitsPerWord;
   else
      m_pWordArray[ bitPos/BitsPerWord ] &= ~( DWORD(1) << bitPos % BitsPerWord );

   }

void BitArray::SetAll( bool bitValue )
   {
   int value(0);

   if ( bitValue )
      value = int(~0);

   if ( m_sizeWords > 0 )
      memset( m_pWordArray, value, CharsPerWord*m_sizeWords ); 
   }

void BitArray::SetLength( UINT sizeBits, bool bitValue /*= false*/ )
   {
#ifdef _DEBUG
   if ( m_pWordArray == NULL )
      {
      ASSERT( m_sizeBits == 0 && m_sizeWords == 0 );
      }
#endif

   UINT sizeWords = ( sizeBits + BitsPerWord - 1 ) / BitsPerWord; // round up

   if( sizeWords == 0 )
      {
      if ( m_pWordArray != NULL )
         delete m_pWordArray;

      m_pWordArray = NULL;
      m_sizeBits   = 0;
      m_sizeWords  = 0;
      return;
      }
      
   DWORD *pArray = new DWORD[ sizeWords ];
   ASSERT( pArray );

   // shortSizeBits is the SMALLER of the current size ( m_sizeBits ) and the desired size( sizeBits );
   UINT shortSizeBits  = m_sizeBits  < sizeBits  ? m_sizeBits  : sizeBits;
   UINT shortSizeWords = m_sizeWords < sizeWords ? m_sizeWords : sizeWords;

   if ( shortSizeWords > 0 )  // is there any thing to copy?
      {
      memcpy( (void*) pArray, (const void*) m_pWordArray, (shortSizeWords-1)*CharsPerWord );

      DWORD word = 0;
      if ( bitValue )
         word = DWORD(~0);

      DWORD mask = 0;
      for ( UINT i = (shortSizeWords - 1)*BitsPerWord; i<shortSizeBits; i++ )
         mask |= DWORD(1) << i;

      pArray[ shortSizeWords - 1 ] = ( mask & m_pWordArray[ shortSizeWords - 1 ] ) | ( word & ~mask );
      }

   if ( sizeWords > 0 )
      {
      int value(0);
      if ( bitValue )
         value = int(~0);

      if ( shortSizeWords < sizeWords )
         memset( (void*) (pArray + shortSizeWords), value, (sizeWords-shortSizeWords)*CharsPerWord );
      }

   delete m_pWordArray;

   m_sizeBits   = sizeBits;
   m_sizeWords  = sizeWords;
   m_pWordArray = pArray;
   }

bool BitArray::Get( UINT bitPos ) const
   {
   ASSERT( bitPos < m_sizeBits );
   return Test( bitPos );
   }

BitArray::Bit BitArray::Get( UINT bitPos )
   {
   ASSERT( bitPos < m_sizeBits );
   return Bit( this, bitPos );
   }

bool BitArray::Test( UINT bitPos ) const
   {
   ASSERT( bitPos < m_sizeBits );

   UINT index = bitPos/BitsPerWord;
   DWORD  mask  = DWORD(1) << bitPos % BitsPerWord;
   DWORD  val   = ( m_pWordArray[ bitPos/BitsPerWord ] & ( DWORD(1) << bitPos % BitsPerWord ) );

   return ( ( m_pWordArray[ bitPos/BitsPerWord ] & ( DWORD(1) << bitPos % BitsPerWord ) ) != 0 );
   }

CString BitArray::GetString() const
   {
   CString str;

   for ( UINT i=0; i < m_sizeBits; i++ )
      {
      if ( Get(i) )
         str += '1';
      else
         str += '0';
      }

   return str;
   }

