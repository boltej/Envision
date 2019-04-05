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

// usage:  PtrClass< "Class"> - pass the type, arrays store ptointer to the type

template < class T >
class PtrArray : public CArray< T*, T* >
   {
   public:
      bool m_deleteOnDelete;
      PtrArray( bool deleteOnDelete=true) : m_deleteOnDelete( deleteOnDelete ) { }
      PtrArray( PtrArray<T> &ptrArray ) { *this = ptrArray; }

      ~PtrArray() { RemoveAll(); }

      INT_PTR DeepCopy( PtrArray<T> &source ) { *this = source; return CArray< T*, T* >::GetSize(); }

      PtrArray<T> & operator = ( PtrArray<T> &source )
         {
         m_deleteOnDelete = source.m_deleteOnDelete;

         for ( INT_PTR i=0; i < source.GetSize(); i++ ) 
            this->Add( new T( *source.GetAt( i ) ) ); 
         
         return *this;
         }
         
      void Clear() { RemoveAll(); }

      virtual void RemoveAll()
         {
         if ( m_deleteOnDelete ) 
            for ( INT_PTR i=0; i < CArray< T*, T* >::GetSize(); i++ )
               {
               T* p = CArray< T*, T* >::GetAt( i );
               delete p; 
               }

         CArray< T*, T* >::RemoveAll(); 
         }

      virtual void RemoveAt( INT_PTR i )
         {
         if ( m_deleteOnDelete )
            delete CArray< T*, T* >::GetAt( i );

         CArray<T*,T*>::RemoveAt( i );
         }

      void RemoveAtNoDelete( INT_PTR i )
         {
         CArray<T*,T*>::RemoveAt( i );
         }

      bool Remove( T* pItem )
         {
         for ( INT_PTR i=0; i < CArray< T*, T* >::GetSize(); i++ )
            {
            if ( CArray< T*, T* >::GetAt( i ) == pItem )
               {
               CArray< T*, T* >::RemoveAt( i );
               return true;
               }
            }
         
         return false;
         }

      int Find( T* pItem ) 
         {
         for (INT_PTR i = 0; i < CArray< T*, T* >::GetSize(); i++)
            {
            if (CArray< T*, T* >::GetAt(i) == pItem)
               return i;
            }

         return -1;
         }
   };
