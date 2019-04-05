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

class DataObj;

class LIBSAPI DOIndex
   {
   public:
      DOIndex( void );

      DOIndex( DataObj*, UINT size );
      //~DOIndex(void);

      DOIndex &operator = ( const DOIndex & );

      int  Build( void );
      bool IsBuilt( void ) { return m_isBuilt; }

      //bool LookupRow( int row, float &value ) { UINT key = GetKey( row ); return Lookup( key, value ); }

      BOOL Lookup( UINT key, int &value )
         {
         ASSERT( IsBuilt() );
         return m_map.Lookup( key, value );
         }
      
      virtual UINT GetKey( int row ) = 0;

   public:
      DataObj  *m_pDataObj;
      int       m_size;
      bool      m_isBuilt;

      CMap< UINT, UINT, int, int > m_map;
   };




