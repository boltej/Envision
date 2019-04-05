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
#include "PolyPtMapper.h"


PolyPointMapper::PolyPointMapper(MapLayer *pPolyLayer,MapLayer *pPtLayer,LPCTSTR filename)
   : m_pPolyLayer(pPolyLayer)
    ,m_pPtLayer(pPtLayer)
{
    if (filename != NULL)
       LoadFromFile(filename); 
}

void PolyPointMapper::BuildIndex()
   {
   // basic idea of index (in memory) is to maintain two maps, one mapping pt indexs (key) to associated polys (value),
   // and the second mapping poly indexes (key) to associated point indexes (values)
   m_ptIndexToPolyIndexMap.clear();
   m_polyIndexToPtIndexMap.clear();

   // start with Polygons
   for ( int i=0; i < m_pPolyLayer->GetPolygonCount(); i++ )
      {
      Poly *pPoly = m_pPolyLayer->GetPolygon( i );

      for ( int j=0; j < m_pPtLayer->GetPolygonCount(); j++ )
         {
         Poly *pPt = m_pPtLayer->GetPolygon( j );

         REAL x, y;
         m_pPtLayer->GetPointCoords( j, x, y );

         Vertex v(x, y);

         if ( pPoly->IsPointInPoly(v) )
            Add( i, j );
         }
      }
   }


void PolyPointMapper::Add( int polyIndex, int ptIndex )
   {
	// should this be make_pair ???
   m_ptIndexToPolyIndexMap.insert( std::pair< int, int >( ptIndex, polyIndex ));
   m_polyIndexToPtIndexMap.insert( std::pair< int, int >( polyIndex, ptIndex ));
   }


int PolyPointMapper::GetPolysFromPointIndex( int ptIndex, CArray< int, int > &values)
   {
   values.RemoveAll();

  /* std::pair <std::unordered_multimap<int,int>::iterator, std::unordered_multimap<int,int>::iterator> ret;
   ret = m_ptIndexToPolyIndexMap.equal_range( ptIndex );*/
   
	auto search = m_ptIndexToPolyIndexMap.find(ptIndex);

	if( search != m_ptIndexToPolyIndexMap.end())
		values.Add(search->second);
   //for (std::unordered_multimap<int,int>::iterator it=ret.first; it != ret.second; ++it )
   //   values.Add( it->second );

   return (int) values.GetSize();
   }


int PolyPointMapper::GetPointsFromPolyIndex( int polyIndex, CArray< int, int > &values)
   {
   values.RemoveAll();

   std::pair <std::unordered_multimap<int,int>::iterator, std::unordered_multimap<int,int>::iterator> ret;
   ret = m_polyIndexToPtIndexMap.equal_range( polyIndex );
   
   for (std::unordered_multimap<int,int>::iterator it=ret.first; it != ret.second; ++it )
      values.Add( it->second );

   return (int) values.GetSize();
   }


bool PolyPointMapper::SaveToFile( LPCTSTR filename )
   {
   // a ppm file contains a header and two main section, corresponding to the ptIndex-based keys,
   // and the polyIndex-based keys.
   // HEADER LAYOUT
   //   long version
   //   long numPolys
   //   long numPts
   //   char polyLayerName[128]
   //   char ptFilename[ 128 ]
   //   char reserved[ 64 ]
   //
   // the first section has the polyIndex mappings.
   //  for each polygon, [ptCount][ptindx0,ptindex1...]

   //  1) an  int array (of length #polys) storing the ptCounts for each polygon
   //  2) an int array for each polygon, list associated ptIndexs

  FILE *fp = NULL;
  fopen_s( &fp, filename, "wb");

  if ( fp == NULL )
     return false;

  // write header
  int version = 1;
  int numPolys = m_pPolyLayer->GetPolygonCount();
  int numPts = m_pPtLayer->GetPolygonCount();
  TCHAR polyLayerName[ 128 ], ptLayerName[ 128 ], reserved[ 64 ];
  memset( polyLayerName, 0, 128 );
  memset( ptLayerName, 0, 128 );
  memset( reserved, 0, 64 );
  _tcscpy_s( polyLayerName, 128, (LPCTSTR) m_pPolyLayer->m_name );
  _tcscpy_s( ptLayerName, 128, (LPCTSTR) m_pPtLayer->m_name );
  fwrite( &version,  sizeof( int ), 1, fp  );
  fwrite( &numPolys, sizeof( int ), 1, fp  );
  fwrite( &numPts,   sizeof( int ), 1, fp  );
  fwrite( polyLayerName, sizeof(TCHAR), 128, fp ); 
  fwrite( ptLayerName, sizeof(TCHAR), 128, fp ); 
  fwrite( reserved, sizeof(TCHAR), 64, fp );

  // write polyIndex section
 // CArray< int, int > values;

 // for ( int i=0; i < numPolys; i++ )
 //    {
 //    int ptCount = this->GetPointsFromPolyIndex( i, values );
 //    fwrite( &ptCount, sizeof(int), 1, fp );
 // //   if ( ptCount > 0 )
 //       fwrite( values.GetData(), sizeof(int), (int) values.GetSize(), fp );
 //    }

 // // write ptIndex section  -- NOT NEEDED...
 // for ( int i=0; i < numPts; i++ )
 //    {
 //    int polyCount = this->GetPolysFromPointIndex( i, values );
 //    fwrite( &polyCount, sizeof(int), 1, fp );
 ////    if ( polyCount > 0 )
 //       fwrite( values.GetData(), sizeof(int), (int) values.GetSize(), fp );
 //    }

	size_t size = m_polyIndexToPtIndexMap.size();
	fwrite(&size, sizeof(size_t), 1, fp);
	for (std::unordered_multimap< int, int >::const_iterator i = m_polyIndexToPtIndexMap.begin(); i != m_polyIndexToPtIndexMap.end(); ++i)
	{
		fwrite(&(i->first), sizeof(int), 1, fp);
		fwrite(&(i->second), sizeof(int), 1, fp);
	}

	size = m_ptIndexToPolyIndexMap.size();
	fwrite(&size, sizeof(size_t), 1, fp);
	for (std::unordered_map< int, int >::const_iterator i = m_ptIndexToPolyIndexMap.begin(); i != m_ptIndexToPolyIndexMap.end(); ++i)
	{
		fwrite(&(i->first), sizeof(int), 1, fp);
		fwrite(&(i->second), sizeof(int), 1, fp);
	}

   fclose( fp );

   return true;
   }


bool PolyPointMapper::LoadFromFile( LPCTSTR filename )
   {
   Clear();

   if( m_pPolyLayer == NULL  || m_pPtLayer == NULL )
      return false;

   FILE *fp = NULL;
   fopen_s( &fp, filename, "rb");

   if ( fp == NULL )
      {
      BuildIndex();
      SaveToFile( filename );
      return true;
      }

   // read header
   int version, numPolys, numPts;
   TCHAR polyLayerName[ 128 ], ptLayerName[ 128 ], reserved[ 64 ];
   fread( &version,  sizeof( int ), 1, fp  );
   fread( &numPolys, sizeof( int ), 1, fp  );
   fread( &numPts,   sizeof( int ), 1, fp  );
   fread( polyLayerName, sizeof(TCHAR), 128, fp ); 
   fread( ptLayerName, sizeof(TCHAR), 128, fp ); 
   fread( reserved, sizeof(TCHAR), 64, fp );

   // read polyIndex section
   size_t polyToPtIndexMapSize;
   fread(&polyToPtIndexMapSize, sizeof(size_t), 1, fp);

   for (size_t i = 0; i < (int)polyToPtIndexMapSize; ++i)
   {
	   int key, value;
	   fread(&key, sizeof(int), 1, fp);
	   fread(&value, sizeof(int), 1, fp);
	   m_polyIndexToPtIndexMap.insert(std::pair< int, int >(key, value));
   }

   size_t ptIndexToPolyMapSize;
   fread(&ptIndexToPolyMapSize, sizeof(size_t), 1, fp);

   for (size_t i = 0; i < (int)ptIndexToPolyMapSize; ++i)
   {
	   int key, value;
	   fread(&key, sizeof(int), 1, fp);
	   fread(&value, sizeof(int), 1, fp);
	   m_ptIndexToPolyIndexMap.insert(std::pair< int, int >(key, value));
   }


//   CArray< int, int > values;

  // for ( int i=0; i < numPolys; i++ )
  //    {
  //    int ptCount;
  //    fread( &ptCount, sizeof(int), 1, fp );
  //    
  // //   if ( ptCount > 0 )
  //       {
  //       int *_values = new int[ ptCount ];
  //       fread( _values, sizeof(int), ptCount, fp );

  //       // add to index...
  //       for ( int j=0; j < ptCount; j++ )
		//	 Add(_values[ j ], j);

  //       delete [] _values;
  //       }
  //    }

  // // read ptIndex section
  // for ( int i=0; i < numPts; i++ )
  //    {
  //    int polyCount;
  //    fread( &polyCount, sizeof(int), 1, fp );
  //    
  ////    if ( polyCount > 0 )
  //       {
  //       int *_values = new int[ polyCount ];
  //       fread( _values, sizeof(int), polyCount, fp );
  // 
  //       // add to index...
  //       for ( int j=0; j < polyCount; j++ )
  //          Add( _values[j], j );
  // 
  //       delete [] _values;
  //       }
  //    }

   fclose( fp );

   return true;
   }
