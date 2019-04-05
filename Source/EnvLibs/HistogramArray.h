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

#include "Vdata.h"
#include "randgen/Randunif.hpp"

class MapLayer;


enum HISTO_USE {
   HU_UNIFORM    = 1,
   HU_MEAN       = 2,
   HU_AREAWTMEAN = 4,
   HU_STDDEV     = 8
   };


struct LIBSAPI HISTO_BIN
{
float left;   // left side of bin
float right;  // right side of bin
UINT  count;  // count in bin

HISTO_BIN( float _left, float _right, UINT _count ) : left( _left ), right( _right ), count( _count ) { }
HISTO_BIN() : left( 0 ), right( 0 ), count( 0 ) { }
HISTO_BIN( const HISTO_BIN &hb ) : left( hb.left ), right( hb.right ), count( hb.count ) { }
HISTO_BIN & operator=( HISTO_BIN& hb ) { left = hb.left; right=hb.right, count= hb.count; return *this; }
};


// this represents a single histogram
class LIBSAPI Histogram
{
public:
   CArray< HISTO_BIN, HISTO_BIN&> m_binArray;

public:
   CString m_category;     // variable (column) serving as the basis for this histogram (empty for none)
   int     m_colCategory;  // corresponding column offset
   bool    m_isCategorical;  // is the category categorical
   float   m_minValue;     // for continuous categories only
   float   m_maxValue;     // for continuous categories only
   VData   m_value;        // for categorical categories only
   UINT    m_observations; // total number of observations
   float   m_mean;
   float   m_areaWtMean;
   float   m_stdDev;

   Histogram() : m_colCategory( -1 ), m_isCategorical( true ), m_minValue( 0 ), m_maxValue( 0 ), m_observations( -1 ), m_mean( 0 ), m_areaWtMean( 0 ), m_stdDev( 0 ) { }
};


// a HistogramArray is a collection of histograms that are associated
//  with one variable.  The fieldName/col refers to the variable
//  for which the Histograms are created (i.e. contain counts of the variable)
class LIBSAPI HistogramArray : public CArray< Histogram*, Histogram* >
{
public:
   CString   m_name;          // name of this variable
   CString   m_expr;          // expression used to generate histogram
   CString   m_category;      // field in the MapLayer assocaite with this variable
   int       m_colCategory;   // column for the associated field
   HISTO_USE m_use;           // how to sample - see enum above

   MapLayer *m_pMapLayer;

   double  m_value;
   
   void Clear();
   bool Evaluate( int cell, double &value );

public:
   int LoadXml( LPCTSTR filename );   // returns # of histograms loaded, -1 for error

   HistogramArray(MapLayer *); // LPCTSTR name, LPCTSTR expr, LPCTSTR category, MapLayer *pLayer );
   ~HistogramArray() { Clear(); }

   static RandUniform rn;
   static RandUniform rn1;

};

