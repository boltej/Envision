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

#include "MapLayer.h"
#include <unordered_map>


// Useage:
// 1) allocate PolyPtMapper object;
// 2) call SetLayers to set the pointers to the two map layers
// 3) call LoadFromFile() to load from disk, or call BuildIndex() to build from ctratch
// 4) call the GetXXX methods to get points associated with polys and vice versa

class LIBSAPI PolyPointMapper
{
public:
   // constructors/destructors
   PolyPointMapper() : m_pPolyLayer(NULL), m_pPtLayer(NULL) {}

   PolyPointMapper(MapLayer *pPolyLayer,MapLayer *pPtLayer, LPCTSTR filename=NULL);
   
   ~PolyPointMapper() { Clear(); }

   // public methods
   void BuildIndex( void );
   bool SaveToFile( LPCTSTR filename );
   bool LoadFromFile( LPCTSTR filename );
   void Clear( void ) { m_ptIndexToPolyIndexMap.clear(); m_polyIndexToPtIndexMap.clear(); }

   void SetLayers( MapLayer *pPolyLayer, MapLayer *pPtLayer, bool build )
   {
   m_pPolyLayer = pPolyLayer;
   m_pPtLayer = pPtLayer;

   if ( build )
      BuildIndex();
   }

   void Add( int polyIndex, int ptIndex );
   int  GetPointsFromPolyIndex( int polyIndex, CArray< int, int > &);
   int  GetPolysFromPointIndex( int ptIndex, CArray< int, int > &);

protected:
 /*  std::unordered_multimap< int, int > m_ptIndexToPolyIndexMap;*/
   std::unordered_map< int, int > m_ptIndexToPolyIndexMap;
   std::unordered_multimap< int, int > m_polyIndexToPtIndexMap;

   MapLayer *m_pPolyLayer;
   MapLayer *m_pPtLayer;
};

