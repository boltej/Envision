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
#include "Maplayer.h"
#include "PtrArray.h"


// Various DataAggregator::GetData() methods
enum DAMETHOD { DAM_FIRST=0, DAM_MEAN, DAM_AREAWTMEAN, DAM_SUM, DAM_MAJORITYAREA, DAM_MAJORITYCOUNT };

// helper structs/classes
struct POLY_VALUES 
{ 
  int count; 
  float area; 
  POLY_VALUES() :count(0),area(0.0f) {}
};

class LIBSAPI  DataAggregator
{
public:
   DataAggregator(  MapLayer *pLayer ) : m_pLayer( pLayer ), m_polyIndexArray( NULL ), m_colArea( -1 ) { }
   ~DataAggregator() { if ( m_manageInternally && m_polyIndexArray != NULL ) delete [] m_polyIndexArray; }

public:
   void SetPolys( int *polys, int count );
   int  AddPolys( int *polys, int count );      // assumes local management, return total number of polys managed
   void SetAreaCol( int col ) { m_colArea = col; }

   // high level query
   bool GetData( int col, VData &value, DAMETHOD method );
   bool GetData( int col, int   &value, DAMETHOD method );
   bool GetData( int col, float &value, DAMETHOD method );
   bool GetData( int col, bool  &value, DAMETHOD method );

protected:
   MapLayer *m_pLayer;     // associated map laey
   PtrArray< POLY_VALUES > m_polyValueArray;
   CMap< VData, VData&, POLY_VALUES*, POLY_VALUES* > m_valuePtrMap;
   
   int *m_polyIndexArray;
   int  m_polyIndexCount;
   bool m_manageInternally;   // is the arr

   int  m_colArea;

public:
   bool AddPolyValues( VData value, float area );
   bool AddPolyValues( int   value, float area ) { return AddPolyValues( VData( value ), area ); }
   bool AddPolyValues( float value, float area ) { return AddPolyValues( VData( value ), area ); }
   bool AddPolyValues( bool  value, float area ) { return AddPolyValues( VData( value ), area ); }
protected:
   bool GetMajorityCount( VData &value );
   bool GetMajorityCount( int   &value );
   bool GetMajorityCount( float &value );
   bool GetMajorityCount( bool  &value );
    
   bool GetMajorityArea( VData &value );
   bool GetMajorityArea( int   &value );
   bool GetMajorityArea( float &value );
   bool GetMajorityArea( bool  &value );
};

