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
#include "PolyEdgeIndex.h"

#include <math.h>
#include <float.h>


PolyEdgeIndex::QuadQuery::QuadQuery( short xSign, short ySign )
   {
   m_dirList[0].x =  0;           m_dirList[6].x =  xSign;
   m_dirList[0].y = -ySign;       m_dirList[6].y = -ySign;

   m_dirList[1].x = -xSign;       m_dirList[7].x =  0;
   m_dirList[1].y = -ySign;       m_dirList[7].y = -2*ySign;

   m_dirList[2].x = -xSign;       m_dirList[8].x = -xSign;
   m_dirList[2].y =  0;           m_dirList[8].y = -2*ySign;

   m_dirList[3].x = -xSign;       m_dirList[9].x = -2*xSign;
   m_dirList[3].y =  ySign;       m_dirList[9].y = -ySign;

   m_dirList[4].x =  0;           m_dirList[10].x = -2*xSign;
   m_dirList[4].y =  ySign;       m_dirList[10].y = 0;

   m_dirList[5].x =  xSign;
   m_dirList[5].y =  0;

   for ( int i=0; i<11; i++ )
      m_testList[i] = false;

   }

PolyEdgeIndex::PolyEdgeIndex( const MapLayer *pMapLayer, int depth /*= 10*/, int threshold /*= 10*/ )
:  BasicQuadTree( depth ),
   m_pMapLayer( pMapLayer ),
   m_polyIndex( -1 ),
   m_vertexIndex( -1 ),
   m_pFrom( NULL ),
   m_pTo( NULL ),
   m_threshold( 10 )
   {
   m_pRoot->m_pData = (void*)new PolyEdgeArray();
   m_polyMask.SetLength( pMapLayer->GetRecordCount( MapLayer::ALL ) );

   REAL xMax, yMax;
   pMapLayer->GetExtents( m_vxMin, xMax, m_vyMin, yMax );
   m_vExtents = xMax - m_vxMin > yMax - m_vyMin ? xMax - m_vxMin : yMax - m_vyMin;
   }

PolyEdgeIndex::~PolyEdgeIndex()
   {
   ClearData( m_pRoot );
   }

void PolyEdgeIndex::RemoveAll()
   {
   ClearData( m_pRoot );
   BasicQuadTree::ClearAll();

   m_pRoot->m_pData = (void*)new PolyEdgeArray();
   m_polyMask.SetAll( false );
   }

void PolyEdgeIndex::AddPoly( int polyIndex )
   {
   ASSERT( m_pMapLayer );
   ASSERT( 0 <= polyIndex && polyIndex < m_pMapLayer->GetRecordCount( MapLayer::ALL ) );

   int recordCount = m_pMapLayer->GetRecordCount( MapLayer::ALL );
   ASSERT( recordCount >= 0 );

   if ( m_polyMask.GetLength() < (UINT) recordCount )
      m_polyMask.SetLength( recordCount );

   if ( ! m_polyMask[ polyIndex ] )
      {
      m_polyMask[ polyIndex ] = true;
      Poly *pPoly = m_pMapLayer->GetPolygon( polyIndex );
         
      int parts = (int) pPoly->m_vertexPartArray.GetSize();
      ASSERT( parts > 0 ); //must have atleast one part

      int startPart = pPoly->m_vertexPartArray[ 0 ];
      int nextPart = 0;

      for ( int part=0; part < parts; part++ )
         {
         if ( part < parts-1 )
            nextPart = pPoly->m_vertexPartArray[ part+1 ];
         else
            nextPart = pPoly->GetVertexCount();

         ASSERT( nextPart - startPart >= 4 ); // a ring is composed of 4 or more points

         for ( int j=startPart; j<nextPart-1; j++ )
            {
            AddEdge( polyIndex, j );
            }

         startPart = nextPart;
         }
      }
   }

void PolyEdgeIndex::RemovePoly( int polyIndex )
   {
   ASSERT( m_pMapLayer );
   ASSERT( 0 <= polyIndex && polyIndex < m_pMapLayer->GetRecordCount( MapLayer::ALL ) );

   int recordCount = m_pMapLayer->GetRecordCount( MapLayer::ALL );
   ASSERT( recordCount >= 0 );

   if ( m_polyMask.GetLength() < (UINT) recordCount )
      m_polyMask.SetLength( recordCount );

   if ( m_polyMask[ polyIndex ] )
      {
      m_polyMask[ polyIndex ] = false;
      Poly *pPoly = m_pMapLayer->GetPolygon( polyIndex );
      
      int parts = (int) pPoly->m_vertexPartArray.GetSize();
      ASSERT( parts > 0 ); //must have atleast one part

      int startPart = pPoly->m_vertexPartArray[ 0 ];
      int nextPart = 0;

      for ( int part=0; part < parts; part++ )
         {
         if ( part < parts-1 )
            nextPart = pPoly->m_vertexPartArray[ part+1 ];
         else
            nextPart = pPoly->GetVertexCount();

         ASSERT( nextPart - startPart >= 4 ); // a ring is composed of 4 or more points

         for ( int j=startPart; j<nextPart-1; j++ )
            {
            RemoveEdge( polyIndex, j );
            }

         startPart = nextPart;
         }
      }
   }

void PolyEdgeIndex::AddEdge( int polyIndex, int vertexIndex )
   {
   ASSERT( 0 <= polyIndex && polyIndex < m_pMapLayer->GetPolygonCount( MapLayer::ALL ) );
   Poly *pPoly = m_pMapLayer->GetPolygon( polyIndex );
   ASSERT( vertexIndex < pPoly->GetVertexCount() - 1 );

   m_polyIndex   = polyIndex;
   m_vertexIndex = vertexIndex;
   m_pFrom       = &pPoly->m_vertexArray[ vertexIndex ];
   m_pTo         = &pPoly->m_vertexArray[ vertexIndex + 1 ];

   AddEdge( m_pRoot );

   m_polyIndex   = -1;
   m_vertexIndex = -1;
   m_pFrom       = NULL;
   m_pTo         = NULL;
   }

void PolyEdgeIndex::RemoveEdge( int polyIndex, int vertexIndex )
   {   
   Poly *pPoly = m_pMapLayer->GetPolygon( polyIndex );
   ASSERT( vertexIndex < pPoly->GetVertexCount() - 1 );

   m_polyIndex   = polyIndex;
   m_vertexIndex = vertexIndex;
   m_pFrom       = &pPoly->m_vertexArray[ vertexIndex ];
   m_pTo         = &pPoly->m_vertexArray[ vertexIndex + 1 ];

   RemoveEdge( m_pRoot );

   m_polyIndex   = -1;
   m_vertexIndex = -1;
   m_pFrom       = NULL;
   m_pTo         = NULL;
   }

void PolyEdgeIndex::AddEdge( Node *pNode )
   {
   if ( pNode->m_pChild[0] == NULL )
      {
      AddEdgeToNode( pNode );
      return;
      }

   for ( int i=0; i<4; i++ )
      {
      Node *pChild = pNode->m_pChild[i];
      if ( NodeEdgeIntersect( pChild ) )
         AddEdge( pNode->m_pChild[i] );
      }
   }

void PolyEdgeIndex::RemoveEdge( Node* pNode )
   {
   if ( pNode->m_pChild[0] == NULL )
      {
      RemoveEdgeFromNode( pNode );
      return;
      }

   int count = 0;
   PolyEdgeArray *pEdgeArray = NULL;
   for ( int i=0; i<4; i++ )
      {
      if ( NodeEdgeIntersect( pNode->m_pChild[i] ) )
         RemoveEdge( pNode->m_pChild[i] );

      if ( pNode->m_pChild[i]->m_pData )
         {
         ASSERT( pNode->m_pChild[i]->m_pChild[0] == NULL );
         pEdgeArray = (PolyEdgeArray*) pNode->m_pChild[i]->m_pData;
         count += (int) pEdgeArray->GetCount();
         }
      else
         count += m_threshold+1;
      }

   // Do we need to merge?   
   if ( ( count <= m_threshold ) )
      {
      Merge( pNode );
      }
   }

void PolyEdgeIndex::AddEdgeToNode( Node* pNode )
   {
   ASSERT( pNode );
   ASSERT( m_pFrom );
   ASSERT( m_pTo );
   ASSERT( m_polyIndex   != -1 );
   ASSERT( m_vertexIndex != -1 );
   ASSERT( pNode->m_pData );

   PolyEdgeArray *pEdgeArray = (PolyEdgeArray*) pNode->m_pData;
   int count = (int) pEdgeArray->GetCount();
   bool add = true;
   PolyEdge edgeToAdd( m_polyIndex, m_vertexIndex );

   for ( int i=0; i<count; i++ )
      {
      if ( pEdgeArray->GetAt(i) == edgeToAdd )
         add = false;
      }

   if ( add )
      pEdgeArray->Add( edgeToAdd );

   if ( ( pEdgeArray->GetCount() >= m_threshold ) && ( pNode->m_size > 1 ) )
      {
      Subdivide( pNode );
      }
   }

void PolyEdgeIndex::RemoveEdgeFromNode( Node* pNode )
   {
   ASSERT( pNode );
   ASSERT( m_pFrom );
   ASSERT( m_pTo );
   ASSERT( m_polyIndex   != -1 );
   ASSERT( m_vertexIndex != -1 );
   ASSERT( pNode->m_pData );

   PolyEdgeArray *pEdgeArray = (PolyEdgeArray*) pNode->m_pData;
   int count = (int) pEdgeArray->GetCount();
   bool found = false;
   for ( int i=0; i<count; i++ )
      {
      PolyEdge &edge = pEdgeArray->GetAt(i);
      if ( edge.m_polyIndex == m_polyIndex && edge.m_vertexIndex == m_vertexIndex )
         {
         found = true;
         pEdgeArray->RemoveAt(i);
         break;
         }
      }
//   ASSERT( found );
   }

void PolyEdgeIndex::Subdivide( Node *pNode )
   {
   pNode->Subdivide();
   for ( int j=0; j<4; j++ )
      {
      pNode->m_pChild[j]->m_pData = (void*)new PolyEdgeArray();
      }

   // move edges from current array to the childrens arrays
   PolyEdgeArray *pEdgeArray = (PolyEdgeArray*) pNode->m_pData;
   const Vertex *pFrom = m_pFrom;
   const Vertex *pTo   = m_pTo;

   int count = (int) pEdgeArray->GetCount();
   for ( int i=0; i<count; i++ )
      {
      PolyEdge &edge = pEdgeArray->GetAt(i);

      Poly *pPoly = m_pMapLayer->GetPolygon( edge.m_polyIndex );
      ASSERT( edge.m_vertexIndex < pPoly->GetVertexCount() - 1 );

      m_pFrom = &pPoly->m_vertexArray[ edge.m_vertexIndex ];
      m_pTo   = &pPoly->m_vertexArray[ edge.m_vertexIndex + 1 ];

      #ifdef _DEBUG
         bool gotIt = false;
      #endif

      for ( int j=0; j<4; j++ )
         {         
         if ( NodeEdgeIntersect( pNode->m_pChild[j] ) )
            {
            #ifdef _DEBUG
               gotIt = true;
            #endif
            PolyEdgeArray *pChildEdgeArray = (PolyEdgeArray*) pNode->m_pChild[j]->m_pData;
            PolyEdge edge{};(edge.m_polyIndex, edge.m_vertexIndex);
            pChildEdgeArray->Add( edge );
            }
         }
      #ifdef _DEBUG
         ASSERT( gotIt );
      #endif
      }

   // delete the current array ( all data now in children )
   delete pEdgeArray;
   pNode->m_pData = 0;

   m_pFrom = pFrom;
   m_pTo   = pTo;
   }

void PolyEdgeIndex::Merge( Node *pNode )
   {
   ASSERT( pNode->m_pData == NULL );
   ASSERT( pNode->m_pChild[0]->m_pData != NULL );

   // Make the PolyEdgeArray of the first child the edge array of the parent
   PolyEdgeArray *pParentEdgeArray = (PolyEdgeArray*) pNode->m_pChild[0]->m_pData;
   pNode->m_pData = (void*) pParentEdgeArray;
   pNode->m_pChild[0]->m_pData = NULL;

   // add the edges from the other children
   PolyEdgeArray *pChildEdgeArray = NULL;
   int  childCount;
   int  parentCount;
   bool gotIt;
   for ( int i=1; i<4; i++ )
      {
      ASSERT( pNode->m_pChild[i]->m_pData );
      pChildEdgeArray = (PolyEdgeArray*) pNode->m_pChild[i]->m_pData;
      childCount = (int) pChildEdgeArray->GetCount();
      for ( int j=0; j<childCount; j++ )
         {
         PolyEdge &childEdge = pChildEdgeArray->GetAt(j);
         gotIt = false;
         
         parentCount = (int) pParentEdgeArray->GetCount();
         for ( int k=0; k<parentCount; k++ )
            {
            PolyEdge &parentEdge = pParentEdgeArray->GetAt(k);
            if ( parentEdge == childEdge )
               {
               gotIt = true;
               break;
               }
            }
         if ( !gotIt )
            {
            pParentEdgeArray->Add( childEdge );
            }
         }
      delete pChildEdgeArray;
      pNode->m_pChild[i]->m_pData = NULL;
      }

   ASSERT( pParentEdgeArray->GetCount() <= m_threshold );
   pNode->Merge();
   }

void PolyEdgeIndex::ClearData( Node *pNode )
   {
   if ( pNode->m_pChild[0] )
      {
      ASSERT( pNode->m_pData == 0 );
      for ( int i=0; i<4; i++ )
         {
         ClearData( pNode->m_pChild[i] );
         }
      }
   else
      {
      ASSERT( pNode->m_pData != NULL );
      PolyEdgeArray *pArray = (PolyEdgeArray*) pNode->m_pData;
      delete pArray;
      pNode->m_pData = NULL;
      }
   }

bool PolyEdgeIndex::GetClosestEdge( Vertex vertex, REAL &radius, PolyEdge &polyEdge )
   {
   radius = FLT_MAX;
   POINT point = VtoL( vertex );
   Node *pQNode = FindNode( point.x, point.y );
   if ( pQNode == NULL )
      return false;

   Node *pTNode = pQNode->m_pParent;

   // Stage 1
   //   Examine the quad (Q) containing the query point
   GetClosestEdgeInNode( pQNode, vertex, radius, polyEdge );

   //gpFWnd->FillNode( pQNode, RGB( 255, 0, 0 ) );

   if ( pTNode )
      {
      Node *pNode = NULL;

      short xSign = 1;
      short ySign = 1;
      
      if ( pQNode->m_x > pTNode->m_x )
         xSign = -1;
      if ( pQNode->m_y > pTNode->m_y )
         ySign = -1;

      int child;
      if ( xSign > 0 )
         if ( ySign > 0 )
            child = Node::SW;
         else
            child = Node::NW;
      else
         if ( ySign > 0 )
            child = Node::SE;
         else
            child = Node::NE;

      QuadQuery query( xSign, ySign );
      
      // Stage 2
      //   Step up to Q's parent (T) and examine the brothers of Q
      for ( int i=0; i<4; i++ )
         {
         if ( i == child )
            continue;

         pNode = pTNode->m_pChild[i];
         ASSERT( pNode );

         if ( NodeCircleIntersect( pNode, vertex, radius ) )
            {
            //gpFWnd->FillNode( pNode, RGB( 0, 255, 0 ) );
            GetClosestEdgeInNode( pNode, vertex, radius, polyEdge );
            }
         }

      // should have found an edge by now
      ASSERT( radius < FLT_MAX );

      // Stage 3 and 4
      //   Examine quads of size equal to or greater than T 
      for ( int i=0; i<11; i++ )
         {
         if ( query.m_testList[i] )
            continue;

         pNode = GetNextNode( pTNode, query.m_dirList[i] );

         if ( pNode )
            {
            if ( NodeCircleIntersect( pNode, vertex, radius ) )
               {
               //gpFWnd->FillNode( pNode, RGB( 0, 0, 255 ) );

               query.m_testList[i] = true;  
            
               if ( pNode->m_size > pTNode->m_size )
                  {
                  // remove overlaping quads from the query list
                  for ( int j=i; j<11; j++ )
                     {
                     int lx = pTNode->m_x + query.m_dirList[j].x;
                     int ly = pTNode->m_y + query.m_dirList[j].y;

                     if ( pTNode->m_x <= lx && lx < pTNode->m_x && 
                        pTNode->m_y <= ly && ly < pTNode->m_y    )
                        query.m_testList[j] = true;
                     }
                  }

               GetClosestEdgeInNode( pNode, vertex, radius, polyEdge );
               }
            }  //if ( pNode )
         } // for ( int i=0; i<11; i++ )
      } // if ( pTNode )

   //ASSERT( radius < FLT_MAX );
#ifdef _DEBUG
   if ( radius == FLT_MAX )
      {
      PolyEdgeArray *pEdgeArray = (PolyEdgeArray*) m_pRoot->m_pData;
      ASSERT( pEdgeArray );
      ASSERT( pEdgeArray->GetCount() == 0 );
      }
#endif

   return true;
   }

void PolyEdgeIndex::GetClosestEdgeInNode( const Node *pNode, Vertex vertex, REAL &radius, PolyEdge &polyEdge )
   {
   ASSERT( pNode );

   if ( pNode->m_pChild[0] != NULL )
      {
      for ( int i=0; i<4; i++ )
         {
         Node *pChild = pNode->m_pChild[i];

         if ( NodeCircleIntersect( pChild, vertex, radius ) )
            GetClosestEdgeInNode( pChild, vertex, radius, polyEdge );
         }
      }
   else
      {
      PolyEdgeArray *pEdgeArray = (PolyEdgeArray*) pNode->m_pData;
      ASSERT( pEdgeArray );
      int count = (int) pEdgeArray->GetCount();

      //gpFWnd->FillNode( (Node*)pNode, RGB( 100, 255, 100 ) );

      REAL tempDist;

      for ( int i=0; i<count; i++ )
         {
         const PolyEdge &edge = pEdgeArray->GetAt(i);
         const Poly *pPoly = m_pMapLayer->GetPolygon( edge.m_polyIndex );

         ASSERT( 0 <= edge.m_vertexIndex );
         ASSERT( edge.m_vertexIndex + 1 < pPoly->GetVertexCount() );
         const Vertex &v1 = pPoly->m_vertexArray.GetAt( edge.m_vertexIndex );
         const Vertex &v2 = pPoly->m_vertexArray.GetAt( edge.m_vertexIndex + 1 );

         tempDist = PointLineDist( vertex, v1, v2 );
         if ( tempDist < radius )
            {
            radius = tempDist;
            polyEdge = edge;
            }
         }
      }
   }

REAL PolyEdgeIndex::PointLineDist( const Vertex &p, const Vertex &a, const Vertex &b )
   {
   REAL L  =   b.x - a.x;
   REAL R  =   b.y - a.y;
   REAL a2 = ( p.y - a.y )*L - ( p.x - a.x )*R;

   REAL d1 = (REAL) sqrt( a2*a2/( L*L + R*R ) );
   
   REAL t = ( p.x - a.x )*L + ( p.y - a.y )*R;

   if ( t<0 )
      {
      L = p.x - a.x;
      R = p.y - a.y;
      return (REAL) sqrt( L*L + R*R );
      }
   else
      {
      t = ( b.x - p.x )*L + ( b.y - p.y )*R;
      if ( t<0 )
         {
         L = p.x - b.x;
         R = p.y - b.y;
         return (REAL) sqrt( L*L + R*R );
         }
      }

   return d1;
   }

bool PolyEdgeIndex::NodeCircleIntersect( const Node *pNode, const Vertex &vertex, REAL radius )
   {
   ASSERT( pNode );

   // coords of quad rectangle relative to center of circle
   REAL xMin = pNode->m_x*m_vExtents/m_size + m_vxMin - vertex.x;
   REAL xMax = ( pNode->m_x + pNode->m_size )*m_vExtents/m_size + m_vxMin - vertex.x;
   REAL yMin = pNode->m_y*m_vExtents/m_size + m_vyMin - vertex.y;
   REAL yMax = ( pNode->m_y + pNode->m_size )*m_vExtents/m_size + m_vyMin - vertex.y;

   double radius2 = radius * radius;

   if (xMax < 0)           // R to left of circle center 
      if (yMax < 0)        // R in lower left corner 
         return ((xMax * xMax + yMax * yMax) < radius2);
      else if (yMin > 0)   // R in upper left corner 
         return ((xMax * xMax + yMin * yMin) < radius2);
      else                 // R due West of circle 
         return( -xMax < radius);
   else if (xMin > 0)      // R to right of circle center 
      if (yMax < 0)        // R in lower right corner 
         return ((xMin * xMin + yMax * yMax) < radius2);
      else if (yMin > 0)   // R in upper right corner 
         return ((xMin * xMin + yMin * yMin) < radius2);
      else                 // R due East of circle 
         return (xMin < radius);
   else                    // R on circle vertical centerline 
      if (yMax < 0)        // R due South of circle 
         return ( -yMax < radius);
      else if (yMin > 0)   // R due North of circle 
         return (yMin < radius);
      else                 // R contains circle centerpoint 
         return true;
   }

bool PolyEdgeIndex::NodeEdgeIntersect( Node* pNode )
   {
   ASSERT( pNode );
   ASSERT( m_pFrom );
   ASSERT( m_pTo );

   REAL xMin = pNode->m_x*m_vExtents/m_size + m_vxMin;
   REAL xMax = ( pNode->m_x + pNode->m_size )*m_vExtents/m_size + m_vxMin;
   REAL yMin = pNode->m_y*m_vExtents/m_size + m_vyMin;
   REAL yMax = ( pNode->m_y + pNode->m_size )*m_vExtents/m_size + m_vyMin;

   REAL x1 = m_pFrom->x;
   REAL y1 = m_pFrom->y;
   REAL x2 = m_pTo->x;
   REAL y2 = m_pTo->y;

   if ( xMin <= x1 && x1 < xMax && yMin <= y1 && y1 < yMax )
      return true;
   if ( xMin <= x2 && x2 < xMax && yMin <= y2 && y2 < yMax )
      return true;

   if ( x1 != x2 ) 
      {
      if ( ( x1 < xMin && xMin < x2 ) || ( x2 < xMin && xMin < x1 ) )
         {
         // Test Left
         REAL y = ( x2*y1 - x1*y2 + ( y2 - y1 )*xMin ) / ( x2 - x1 );
         if ( yMin <= y && y < yMax )
            return true;
         }
      if ( ( x1 < xMax && xMax < x2 ) || ( x2 < xMax && xMax < x1 ) )
         {
         // Test Right
         REAL y = ( x2*y1 - x1*y2 + ( y2 - y1 )*xMax ) / ( x2 - x1 );
         if ( yMin <= y && y < yMax )
            return true;
         }
      }

   if ( y1 != y2 ) 
      {
      if ( ( y1 < yMin && yMin < y2 ) || ( y2 < yMin && yMin < y1 ) )
         {
         // Test Bottom
         REAL x = ( x2*y1 - x1*y2 + ( x1 - x2 )*yMin ) / ( y1 - y2 );
         if ( xMin <= x && x < xMax )
            return true;
         }
      if ( ( y1 < yMax && yMax < y2 ) || ( y2 < yMax && yMax < y1 ) )
         {
         // Test Top
         REAL x = ( x2*y1 - x1*y2 + ( x1 - x2 )*yMax ) / ( y1 - y2 );
         if ( xMin <= x && x < xMax )
            return true;
         }
      }

   return false;
   }

bool PolyEdgeIndex::Save( LPCTSTR filename )
   {   
   FILE *fp;
   fopen_s( &fp, filename, "wb" );

   if ( fp == NULL )
      {
#ifndef NO_MFC
      CString msg( "Unable to find file " );
      msg += filename;
      MessageBox( GetActiveWindow(), msg, "Error", MB_ICONERROR | MB_OK );
#endif
      return false;
      }
  
   bool ok = Save( fp );

   fclose( fp );

   return ok;
   }

bool PolyEdgeIndex::Load( LPCTSTR filename )
   {   
   FILE *fp;
   fopen_s( &fp, filename, "rb" );

   if ( fp == NULL )
      {
#ifndef NO_MFC
      CString msg( "Unable to find file " );
      msg += filename;
      MessageBox( GetActiveWindow(), msg, "Error", MB_ICONERROR | MB_OK );
#endif
      return false;
      }
  
   bool ok = Load( fp );

   fclose( fp );

   return ok;
   }

bool PolyEdgeIndex::Save( FILE *fp )
   {
   PutInt( fp, m_threshold );
   PutInt( fp, m_depth );

   SaveNode( fp, m_pRoot );

   return true;
   }

bool PolyEdgeIndex::Load( FILE *fp )
   {
   int threshold, depth;
   GetInt( fp, threshold );
   GetInt( fp, depth );

   if ( m_threshold != threshold || m_depth != depth )
      {
      ASSERT(0);
      return false;
      }

   ClearData( m_pRoot );
   if ( m_pRoot->m_pChild[0] != NULL )
      {
      for ( int i=0; i<4; i++ )
         delete m_pRoot->m_pChild[i];
      }

   LoadNode( fp, m_pRoot );

   return true;
   }

void PolyEdgeIndex::SaveNode( FILE *fp, Node *pNode )
   {
   ASSERT( pNode );

   PutInt( fp, pNode->m_x    );
   PutInt( fp, pNode->m_y    );
   PutInt( fp, pNode->m_size );

   int data = pNode->m_pData ? 1 : 0;
   PutInt( fp, data );

   if ( data )
      {
      PolyEdgeArray *pEdgeArray = (PolyEdgeArray*) pNode->m_pData;
      int count = (int) pEdgeArray->GetCount();
      PutInt( fp, count );

      for ( int i=0; i<count; i++ )
         {
         PolyEdge &edge = pEdgeArray->GetAt(i);
         PutInt( fp, edge.m_polyIndex   );
         PutInt( fp, edge.m_vertexIndex );
         }
      }

   int children = pNode->m_pChild[0] ? 1 : 0;
   PutInt( fp, children );

   if ( children )
      {
      ASSERT( pNode->m_pData == NULL );
      for ( int i=0; i<4; i++ )
         SaveNode( fp, pNode->m_pChild[i] );
      }
   }

void PolyEdgeIndex::LoadNode( FILE *fp, Node *pNode )
   {
   ASSERT( pNode );

   GetInt( fp, pNode->m_x );
   GetInt( fp, pNode->m_y    );
   GetInt( fp, pNode->m_size );

   int data = 0;
   GetInt( fp, data );

   if ( data )
      {
      PolyEdgeArray *pEdgeArray = new PolyEdgeArray();
      pNode->m_pData = (void*) pEdgeArray;

      int count = 0;
      GetInt( fp, count );

      pEdgeArray->SetSize( count );

      for ( int i=0; i<count; i++ )
         {
         PolyEdge &edge = pEdgeArray->GetAt(i);
         GetInt( fp, edge.m_polyIndex   );
         GetInt( fp, edge.m_vertexIndex );
         }
      }

   int children = 0;
   GetInt( fp, children );

   if ( children )
      {
      pNode->Subdivide();
      for ( int i=0; i<4; i++ )
         LoadNode( fp, pNode->m_pChild[i] );
      }
   }
