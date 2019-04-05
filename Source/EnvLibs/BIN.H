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
//-------------------------------------------------------------------
//  BIN.H
//
//-- provides categories (bins) for classification
//-------------------------------------------------------------------
#pragma once
#include "EnvLibs.h"

#ifndef NO_MFC
#include <afxtempl.h>
#endif
#include "Typedefs.h"

enum BPOS { BP_FIRST = -1, BP_MID = 0, BP_LAST = 1 };
//enum BINTYPE { BT_NUMERIC, BT_STRING };

struct FIELD_ATTR;
struct MAP_FIELD_INFO;


class  LIBSAPI Bin
   {
   public:
      float m_minVal;     // for numeric bins only
      float m_maxVal;     // for numeric bins only
      CString m_strVal;   // for string bins only
      int   m_popSize;    // number of individual in this bin
      float m_area;       // area of polygons associated with this bin

      FIELD_ATTR *m_pFieldAttr;  // 
      COLORREF m_color;
      int      m_transparency;  // 0=opaque, 100 = transparent
      CString  m_label;      // (optional) label
      TYPE     m_type;

      //-- constructor --//
      Bin( void ) : m_minVal( 0 ), m_maxVal( 1 ), m_strVal(), m_popSize( 0 ), 
                    m_color( 0L ), m_transparency( 0 ), m_label( "" ), m_type( TYPE_NULL ), m_area( 0 ), m_pFieldAttr( NULL ) { }

      Bin( const Bin &r ) { *this = r; }
      
      Bin& operator=( const Bin& r )
         {
         m_minVal  = r.m_minVal;
         m_maxVal  = r.m_maxVal;
         m_strVal  = r.m_strVal;
         m_popSize = r.m_popSize;
         m_color   = r.m_color;
         m_transparency = r.m_transparency;
         m_label   = r.m_label;
         m_type    = r.m_type;
         m_area    = r.m_area;
         m_pFieldAttr = r.m_pFieldAttr;

         return *this;
         }

      void SetLabel( LPCTSTR label ) { m_label = label; }
      void SetDefaultLabel( TYPE type, LPCTSTR format, BPOS, BOOL showCount, BOOL singleValue, LPCTSTR units=NULL );

      void SetTransparency( int transparency ) { m_transparency = transparency; }  // 0=opaque, 255=transparent
      void SetColor( COLORREF );    // creates an appropriate solid brush
   };



class LIBSAPI BinArray : public CArray< Bin, Bin& >
   {
   public:
      float   m_binMin;
      float   m_binMax;
      TYPE    m_type; 
      bool    m_hasTransparentBins;

      MAP_FIELD_INFO *m_pFieldInfo;

      //-- constructor --//
      BinArray( void )
         : CArray< Bin, Bin& >(), m_binMin( 0 ), m_binMax( 0 ),
           m_type( TYPE_NULL ), m_hasTransparentBins( false ), m_pFieldInfo( NULL ) {}

      BinArray( const BinArray &b ) { *this = b; }
      BinArray& operator = ( const BinArray& );
      
      void RemoveAll( void ) { CArray< Bin, Bin& >::RemoveAll(); m_hasTransparentBins = false; }
   };

class LIBSAPI BinArrayArray : public CArray< BinArray*, BinArray* >
   {
   public:
      BinArrayArray(){}
      ~BinArrayArray(){ RemoveAll(); }
      BinArrayArray( const BinArrayArray& baa ){ *this = baa; }

      BinArrayArray& operator=( const BinArrayArray& baa ); // deep copy

      void RemoveAll();
      void Init( int count );
   };

