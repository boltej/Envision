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

#include "BIN.H"


BinArray& BinArray::operator = ( const BinArray &binArray )
   {
   RemoveAll();

   for ( int i=0; i < binArray.GetSize(); i++ )
      Add( (Bin&) binArray[ i ] );

   m_binMin  = binArray.m_binMin;
   m_binMax  = binArray.m_binMax;
   m_type     = binArray.m_type;
   m_hasTransparentBins = binArray.m_hasTransparentBins;
   m_pFieldInfo = binArray.m_pFieldInfo;

   return *this;
   }


void Bin::SetDefaultLabel( TYPE type, LPCTSTR format, BPOS binPos, BOOL showCount, BOOL singleValue, LPCTSTR units /*=NULL*/ )
   {
   char _units[ 32 ];
   if ( units )
      lstrcpy( _units, units );
   else
      _units[ 0 ] = '\0';
   
   if ( ::IsString( type ) )
      {
      if ( showCount )
         m_label.Format( "%s%s (%d)", m_strVal, _units, m_popSize );
      else  
         {
         m_label = m_strVal;
         m_label += units;
         }

      return;
      }

   // else, its numeric
   char fmt[ 32 ];
   fmt[ 0 ] = '\0';

   if ( singleValue == FALSE )
      {
      switch( binPos )
         {
		   case BP_FIRST: lstrcpy( fmt, "<= " );  break;
         case BP_LAST:  lstrcpy( fmt, "> " );  break;
         default:       lstrcpy( fmt, format ); lstrcat( fmt, " to " );
         }
      }

   lstrcat( fmt, format );
   if ( units != NULL )
      lstrcat( fmt, units );

   if ( showCount )
      {
      lstrcat( fmt, " (%i)" );

      if ( singleValue == FALSE )
         {
         switch( binPos )
            {
            case BP_FIRST: m_label.Format( fmt, m_maxVal, m_popSize );  break;
			   case BP_LAST:  m_label.Format( fmt, m_minVal, m_popSize );  break;
            default:       m_label.Format( fmt, m_minVal, m_maxVal, m_popSize );
            }
         }
      else  // single value == TRUE
         m_label.Format( fmt, (m_minVal+m_maxVal)/2, m_popSize );
      }

   else  // showCount == FALSE
      {
      if ( singleValue == FALSE )
         {
         switch( binPos )
            {
            case BP_FIRST: m_label.Format( fmt, m_maxVal ); break;
            case BP_LAST:  m_label.Format( fmt, m_minVal ); break;
            default:       m_label.Format( fmt, m_minVal, m_maxVal );
            }
         }
      else  // singleValue == TRUE
         m_label.Format( fmt, (m_minVal+m_maxVal)/2 );
      }
	}


void Bin::SetColor( COLORREF color )
   {
   m_color = color;

   //if ( ( (HBRUSH) m_brush ) != NULL )
   //   m_brush.DeleteObject();

   //m_brush.CreateSolidBrush( color );
   }

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// BinArrayArray

BinArrayArray& BinArrayArray::operator=( const BinArrayArray& baa )
   {
   int count = (int) baa.GetCount();
   SetSize( count );

   for ( int i=0; i < count; i++ )
      {
      BinArray *pBinArray = baa.GetAt( i );

      if ( pBinArray == NULL )
         SetAt( i, NULL );
      else
         SetAt( i, new BinArray( *pBinArray ) );
      }

   return *this;
   }


void BinArrayArray::RemoveAll()
   {
   int count = (int) GetCount();
   BinArray *pBinArray = NULL;

   for ( int i=0; i<count; i++ )
      {
      pBinArray = GetAt(i);
      if ( pBinArray != NULL )
         delete pBinArray;
      }
   CArray< BinArray*, BinArray* >::RemoveAll();
   }

void BinArrayArray::Init( int count )
   {
   RemoveAll();
   SetSize( count );
   for ( int i=0; i < count; i++ )
      Add( NULL );
   }
