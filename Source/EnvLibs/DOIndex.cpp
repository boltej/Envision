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

#include "DOIndex.h"
#include "DATAOBJ.H"
#include "PrimeNumber.h"


DOIndex::DOIndex( void )
   : m_pDataObj( NULL )
   , m_size( -1 )
   , m_isBuilt( false )
   { }


DOIndex::DOIndex( DataObj *pDataObj, UINT size /*=-1*/ )
   : m_pDataObj( pDataObj )
   , m_size( size )
   , m_isBuilt( false )
   {
   if ( m_size > 0 )
      m_map.InitHashTable( m_size );
   }

DOIndex& DOIndex::operator = ( const DOIndex &index )
   {
   m_pDataObj  = index.m_pDataObj;   // no storage, so no copy needed
   m_size      = index.m_size;
   m_isBuilt   = index.m_isBuilt;

   POSITION pos = index.m_map.GetStartPosition();
   m_map.RemoveAll();
   m_map.InitHashTable( index.m_map.GetHashTableSize() );
   UINT  key = 0;
   int   value = 0;
   
   while (pos != NULL)
      {
      index.m_map.GetNextAssoc(pos, key, value );
      m_map.SetAt( key, value );
      }

   return *this;
   }


int DOIndex::Build( void )
   {
   if ( m_pDataObj == NULL )
      return -1;
   
   // start  building index.  Basic idea is to build a hash table with keys that can be
   // defined externally and used to look up specific values based on the key.
   int rows = m_pDataObj->GetRowCount();

   if ( m_size < 0 )
      {
      int maxEntries = rows * 14 / 10;  // increase to 140%
      
      PrimeNumber primeNo;

      while ( primeNo.GetPrime() <= maxEntries )
         {
         ++primeNo;
         }

      m_size = primeNo.GetPrime();

      m_map.InitHashTable( m_size );
      }

   for ( int i=0; i < rows; i++ )
      {
      UINT key = GetKey( i );

      //float value = m_pDataObj->GetAsFloat( m_lookupCol, i );
      m_map.SetAt( key, i );
      }
   
   m_isBuilt = true;

   return rows;
   }


