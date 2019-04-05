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

#include "BasicQuadTree.h"
#include "BitArray.h"
#include "Maplayer.h"


class LIBSAPI PolyEdge
   {
   public:
      PolyEdge() : m_polyIndex( -1 ), m_vertexIndex( -1 ) {}
      PolyEdge( int polyIndex, int vertexIndex ) : m_polyIndex( polyIndex ), m_vertexIndex( vertexIndex ) {}

      PolyEdge( const PolyEdge &polyEdge ){ *this = polyEdge; }

      PolyEdge &operator=  ( const PolyEdge &polyEdge ){ m_polyIndex = polyEdge.m_polyIndex; m_vertexIndex = polyEdge.m_vertexIndex; return *this; }
      bool      operator== ( const PolyEdge &polyEdge ) const { return ( m_polyIndex == polyEdge.m_polyIndex && m_vertexIndex == polyEdge.m_vertexIndex ); }

   public:
      int m_polyIndex;
      int m_vertexIndex; // edge is [ m_vertexIndex, m_vertexIndex+1 )
   };

typedef CArray< PolyEdge, PolyEdge& > PolyEdgeArray;

class LIBSAPI PolyEdgeIndex : public BasicQuadTree
   {
   public:
      PolyEdgeIndex( const MapLayer *pMapLayer, int depth = 10, int threshold = 10 );
      ~PolyEdgeIndex();

   public:
      void AddPoly( int polyIndex );
      void RemovePoly( int polyIndex );
      void AddEdge( int polyIndex, int vertexIndex );
      void RemoveEdge( int polyIndex, int vertexIndex );
      void RemoveAll();

      bool GetClosestEdge( Vertex vertex, REAL &radius, PolyEdge &polyEdge );
      int  GetThreshold(){ return m_threshold; }

      Vertex LtoV( int x, int y ){ POINT p; p.x=x; p.y=y; return LtoV(p); }
      Vertex LtoV( POINT p )
         { return Vertex( p.x*m_vExtents/m_size + m_vxMin, p.y*m_vExtents/m_size + m_vyMin ); }
      POINT  VtoL( Vertex v )
         { POINT p; p.x = LONG( ( v.x - m_vxMin )*m_size/m_vExtents ); p.y = LONG( ( v.y - m_vyMin )*m_size/m_vExtents ); return p; }

      bool Save( LPCTSTR filename );
      bool Load( LPCTSTR filename );
      bool Save( FILE *fp );
      bool Load( FILE *fp );

   protected:
      bool NodeEdgeIntersect( Node *pNode );
      bool NodeCircleIntersect( const Node *pNode, const Vertex &vertex, REAL radius );

      void GetClosestEdgeInNode( const Node *pNode, Vertex vertex, REAL &radius, PolyEdge &polyEdge );
      REAL PointLineDist( const Vertex &p, const Vertex &a, const Vertex &b );

      void Subdivide( Node *pNode );
      void Merge( Node *pNode );
      void ClearData( Node *pNode );

      // recursive functions
      void AddEdge( Node* pNode );
      void RemoveEdge( Node* pNode );

      void AddEdgeToNode( Node* pNode );
      void RemoveEdgeFromNode( Node* pNode );

      void SaveNode( FILE *fp, Node *pNode );
      void LoadNode( FILE *fp, Node *pNode );

      void PutInt( FILE *fp, int  num )
         { 
         fwrite( (void*) &num, sizeof( int ), 1, fp ); 
         }
      void GetInt( FILE *fp, int &num )
         { 
         int count = (int) fread( &num, sizeof( int ), 1, fp );
         ASSERT( count == 1 );
         }

   protected:
      struct QuadQuery
         {
         QuadQuery( short xSign, short ySign );

         BasicQuadTree::Direction m_dirList[11];
         bool m_testList[11];
         };

   protected:
      BitArray m_polyMask;
      const MapLayer *m_pMapLayer;
      int m_threshold;

      REAL m_vxMin;    // virtual coords
      REAL m_vyMin;
      REAL m_vExtents;

      // temporarily used during search algorithms
      int m_polyIndex;
      int m_vertexIndex;
      const Vertex *m_pFrom;
      const Vertex *m_pTo;
   };
