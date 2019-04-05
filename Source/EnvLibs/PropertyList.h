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

#include "Vdata.h"

class LIBSAPI PropertyList
   {
   public:
      PropertyList(void) { }
      ~PropertyList(void) { }


   protected:
      CMap< CString, LPCTSTR, VData, VData& > m_lookupTable;

   public:
      bool GetPropValue( LPCTSTR propName, VData   &value ) { return m_lookupTable.Lookup( propName, value ) ? true : false; }
      bool GetPropValue( LPCTSTR propName, float   &value );
      bool GetPropValue( LPCTSTR propName, int     &value );
      bool GetPropValue( LPCTSTR propName, bool    &value );
      bool GetPropValue( LPCTSTR propName, CString &value );

      void SetPropValue( LPCTSTR propName, VData   &value ) { m_lookupTable.SetAt( propName, value ); }
      void SetPropValue( LPCTSTR propName, float    value ) { VData v(value); m_lookupTable.SetAt( propName, v ); }
      void SetPropValue( LPCTSTR propName, int      value ) { VData v(value); m_lookupTable.SetAt( propName, v ); }
      void SetPropValue( LPCTSTR propName, bool     value ) { VData v(value); m_lookupTable.SetAt( propName, v ); }
      void SetPropValue( LPCTSTR propName, CString &value ) { VData v(value); m_lookupTable.SetAt( propName, v ); }
   };


inline
bool PropertyList::GetPropValue( LPCTSTR propName, float &value )  
   {
   VData v; 
   if ( m_lookupTable.Lookup( propName, v ) == 0 )
      return false;
   
   return v.GetAsFloat( value ); 
   }


inline
bool PropertyList::GetPropValue( LPCTSTR propName, int &value )  
   {
   VData v; 
   if ( m_lookupTable.Lookup( propName, v ) == 0 )
      return false;
   
   return v.GetAsInt( value ); 
   }


inline
bool PropertyList::GetPropValue( LPCTSTR propName, bool &value )  
   {
   VData v; 
   if ( m_lookupTable.Lookup( propName, v ) == 0 )
      return false;
   
   return v.GetAsBool( value ); 
   }


inline
bool PropertyList::GetPropValue( LPCTSTR propName, CString &value )  
   {
   VData v; 
   if ( m_lookupTable.Lookup( propName, v ) == 0 )
      return false;
   
   return v.GetAsString( value ); 
   }
