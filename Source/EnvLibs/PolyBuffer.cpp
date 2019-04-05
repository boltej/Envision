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

#include "PolyBuffer.h"

#include <math.h>
#include "PolyClipper.h"

namespace { inline bool ISZERO(double a) { return  ( -0.00001 < a && a < 0.00001 ); } }//#define ISZERO(a)  ( -0.00001 < a && a < 0.00001 )


int PolyBuffer::MStarDiagram::Compare( const void *p0, const void *p1 )
   {
   const MVector *v0 = (const MVector *) p0;
   const MVector *v1 = (const MVector *) p1;

   //
   // v0 and v1 are vectors with their tails at the origin 
   // v0 and v1 are sorted based on the angle between the vector and the positive x-axis measured *clockwise*
   //
   // angles are not calculated, the cross product is used to get the sign of the difference of the angles
   // this method only works if the angle between v0 and v1 is less than pi
   // if angle >= pi, one vector will be in the upper half plane, and the other will be in the lower
   //    this case is handled separately
   //


   // one vector in the open upper half plane, one in the closed lower half plane( angle > pi )
   if ( v0->m_y > 0.0 && v1->m_y <= 0.0 )
      return 1;
   else if ( v0->m_y <= 0.0 && v1->m_y > 0.0 )
      return -1;
   
   // both on the x-axis ( angle == pi )
   else if ( v0->m_y == 0.0 && v1->m_y == 0.0 )
      {
      if ( v0->m_x < 0.0 && v1->m_x > 0.0 )
         return 1;
      else if ( v0->m_x > 0.0 && v1->m_x < 0.0 )
         return -1;
      else
         {
         if ( v0->m_subjectSide == v1->m_subjectSide )
            return 0;
         else
            return v0->m_subjectSide < v1->m_subjectSide ? -1 : 1;
         }
      }

   // both in open upper or open lower half plane ( angle < pi )
   else
      {
      double cross = v0->m_x*v1->m_y - v0->m_y*v1->m_x;
      
      if ( cross > 0.0 )
         return 1;
      else if ( cross < 0.0 )
         return -1;
      else
         {
         if ( v0->m_subjectSide == v1->m_subjectSide )
            return 0;
         else
            return v0->m_subjectSide < v1->m_subjectSide ? -1 : 1;
         }
      }
   }

PolyBuffer::MStarDiagram::~MStarDiagram()
   {
   }

const PolyBuffer::MVector* PolyBuffer::MStarDiagram::BufferAtVertex( const Poly &subjectPoly, int startIndex, int subjectIndex, double &x, double &y, double distance )
   {
   ASSERT( 0 <= startIndex && startIndex < subjectPoly.m_vertexArray.GetCount() );

   x = subjectPoly.m_vertexArray[ startIndex ].x;
   y = subjectPoly.m_vertexArray[ startIndex ].y + distance;

   m_index = -1;
   const MVector *pNextEdge;

   // Advance around secondary edges until subjectIndex is reached
   for (;;)
      {
      pNextEdge = GetNext();
      if ( pNextEdge->m_subjectSide == subjectIndex )
         break;
      else if ( pNextEdge->m_subjectSide < 0 )
         {
         x += pNextEdge->m_x;
         y += pNextEdge->m_y;
         }
      }

   return pNextEdge;
   }

void PolyBuffer::MStarDiagram::Sort()
   {
   qsort( m_vectorArray.GetData(), m_vectorArray.GetCount(), sizeof( MVector ), Compare );
   m_index = -1;
   }

const PolyBuffer::MVector* PolyBuffer::MStarDiagram::GetNext()
   {
   ASSERT( m_vectorArray.GetCount() > 0 );

   m_index = ( m_index + 1 ) % m_vectorArray.GetCount();

   return &m_vectorArray.GetAt( m_index );
   }

const PolyBuffer::MVector* PolyBuffer::MStarDiagram::GetSubject( int subjectIndex, int &starIndex )
   {
   int count = (int) m_vectorArray.GetCount();
   int index = m_index;
   const MVector *pEdge = &m_vectorArray.GetAt( index );

   while ( pEdge->m_subjectSide != subjectIndex )
      {
      index = ( index + 1 ) % count;
      pEdge = &m_vectorArray.GetAt( index );
      }

   starIndex = index;
   return pEdge;
   }

void PolyBuffer::MStarDiagram::ClearSubjectSides()
   {
   int i=0;
   while ( i < m_vectorArray.GetCount() )
      {
      if ( m_vectorArray[i].m_subjectSide >= 0 )
         {
         m_vectorArray.RemoveAt(i);
         continue;
         }
      i++;
      }
   }

//void PolyBuffer::MPolyBuilder::MakePoly( Poly &poly )
//   {
//   poly.ClearPoly();
//   int partCount = m_vertexArrayArray.GetCount();
//   for ( int part=0; part<partCount; part++ )
//      {
//      MVectorArray &partArray = m_vertexArrayArray.GetAt( part );
//
//      poly.m_vertexPartArray.Add( poly.m_vertexArray.GetCount() );
//      for ( int i=0; i<partArray.GetSize(); i++ )
//         {
//         poly.AddVertex( Vertex( partArray[i].m_x, partArray[i].m_y ) );
//         }
//      }
//   }

void PolyBuffer::MPolyBuilder::MakePoly( Poly &poly )
   {
   if ( m_holes.GetVertexCount() > 0 )
      PolyClipper::PolygonClip( GPC_DIFF, m_polygon, m_holes, m_polygon );

   poly = m_polygon;
   }

void PolyBuffer::MPolyBuilder::MakePart( bool island )
   {
   Poly localPoly;
   const PolyBuffer::MVectorArray *pArray = NULL;

   int count = (int) m_vertexArray.GetCount();
   
   if ( count < m_lastPart.GetCount() )
      {
      count = (int) m_lastPart.GetCount();
      ASSERT( count >= 3 ); // a part must have atleast 3 sides!
      
      pArray = &m_lastPart;
      m_areaSum = 1; // arbitrary number > 0
      }
   else
      {
      ASSERT( count >= 3 ); // a part must have atleast 3 sides!
      MVector v(m_vertexArray[0]);
      m_vertexArray.Add(v);
      m_areaSum += m_vertexArray[ count ].m_x * m_vertexArray[ count - 1 ].m_y - m_vertexArray[ count - 1 ].m_x * m_vertexArray[ count ].m_y;

      Test( 1, count-2 );

      pArray = &m_vertexArray;
      }

   if ( island )
      {
      if ( m_areaSum < 0.0 )
         {
         // Add to m_holes
         m_holes.m_vertexPartArray.Add( m_holes.GetVertexCount() );
         for ( int i=0; i<count; i++ )
            {
            m_holes.AddVertex( Vertex( (REAL) pArray->GetAt(i).m_x, (REAL) pArray->GetAt(i).m_y ) );
            }
         }
      }
   else
      {
      localPoly.m_vertexPartArray.Add(0);
      count = (int) pArray->GetCount();
      for ( int i=0; i<count; i++ )
         {
         localPoly.AddVertex( Vertex( (REAL) pArray->GetAt(i).m_x, (REAL) pArray->GetAt(i).m_y ) );
         }

      if ( m_polygon.GetVertexCount() > 0 )
         {
         PolyClipper::PolygonClip( GPC_UNION, m_polygon, localPoly, m_polygon );
         }
      else
         {
         m_polygon = localPoly;
         }
      }

   m_vertexArray.RemoveAll();
   m_lastPart.RemoveAll();
   m_areaSum = 0.0;
   }

int PolyBuffer::MPolyBuilder::Add( double x, double y )
   {
   int count = (int) m_vertexArray.GetCount();

   if ( count > 0 && x == m_vertexArray[ count - 1 ].m_x && y == m_vertexArray[ count - 1 ].m_y )
      {
      return -1;
      }

   MVector v(x, y);
   m_vertexArray.Add(v);

   if ( count > 0 )
      {
      m_areaSum += x * m_vertexArray[ count - 1 ].m_y - m_vertexArray[ count - 1 ].m_x * y;

      if ( count >= 3 )  
         {
         Test( 0, count-2 );
         }
      }

   return count;
   }

void PolyBuffer::MPolyBuilder::Test( int from, int to )
   {
   int count = (int) m_vertexArray.GetCount();
   ASSERT( count >= 3 );

   MVector intersectionPoint;
   ASSERT( to < count - 1 );

   for ( int i = to-1; i >= from; i-- )
      {
      if ( Intersect( m_vertexArray[i], m_vertexArray[i+1], m_vertexArray[ count - 2 ], m_vertexArray[ count - 1 ], intersectionPoint ) )
         {
         // get area of loop
         double area = 0;

         area += m_vertexArray[i+1].m_x * intersectionPoint.m_y - intersectionPoint.m_x * m_vertexArray[i+1].m_y;
         for ( int j=i+1; j<count-2; j++ )
            {
            const MVector &v1 = m_vertexArray[ j ];
            const MVector &v2 = m_vertexArray[ j+1 ];
            area += v2.m_x * v1.m_y - v1.m_x * v2.m_y;
            }
         area += intersectionPoint.m_x * m_vertexArray[ count - 2 ].m_y - m_vertexArray[ count - 2 ].m_x * intersectionPoint.m_y;

         // add part if its an island
         if ( !ISZERO( area ) && area < 0.0 )
            {
            m_holes.m_vertexPartArray.Add( m_holes.GetVertexCount() );

            if ( ! ISZERO( intersectionPoint.m_x - m_vertexArray[i+1].m_x ) || ! ISZERO( intersectionPoint.m_y - m_vertexArray[i+1].m_y ) )
               m_holes.AddVertex( Vertex( (REAL) intersectionPoint.m_x, (REAL) intersectionPoint.m_y ) );
            for ( int j=i+1; j<count-1; j++ )
               m_holes.AddVertex( Vertex( (REAL) m_vertexArray[ j ].m_x, (REAL) m_vertexArray[ j ].m_y ) );
            if ( ! ISZERO( intersectionPoint.m_x - m_vertexArray[count-2].m_x ) || ! ISZERO( intersectionPoint.m_y - m_vertexArray[count-2].m_y ) )
               m_holes.AddVertex( Vertex( (REAL) intersectionPoint.m_x, (REAL) intersectionPoint.m_y ) );
            }
         else
            {
            m_lastPart.RemoveAll();

            if ( ! ISZERO( intersectionPoint.m_x - m_vertexArray[i+1].m_x ) || ! ISZERO( intersectionPoint.m_y - m_vertexArray[i+1].m_y ) )
               m_lastPart.Add( intersectionPoint );
            for ( int j=i+1; j<count-1; j++ )
               m_lastPart.Add( m_vertexArray[ j ] );
            if ( ! ISZERO( intersectionPoint.m_x - m_vertexArray[count-2].m_x ) || ! ISZERO( intersectionPoint.m_y - m_vertexArray[count-2].m_y ) )
               m_lastPart.Add( intersectionPoint );
            }

         // pinch off loop
         m_vertexArray[i+1] = intersectionPoint;
         m_vertexArray[i+2] = m_vertexArray[ count - 1 ];
         m_vertexArray.RemoveAt( i+3, count - i - 3 );
         m_areaSum -= area;

         count = (int) m_vertexArray.GetCount();

         if ( ISZERO( m_vertexArray[count-1].m_x - m_vertexArray[count-2].m_x ) && ISZERO( m_vertexArray[count-1].m_y - m_vertexArray[count-2].m_y ) )
            {
            m_vertexArray.RemoveAt( count-1 );
            count = (int) m_vertexArray.GetCount();
            }
         else if ( ISZERO( m_vertexArray[count-2].m_x - m_vertexArray[count-3].m_x ) && ISZERO( m_vertexArray[count-2].m_y - m_vertexArray[count-3].m_y ) )
            {
            m_vertexArray.RemoveAt( count-1 );
            count = (int) m_vertexArray.GetCount();
            }         
         }
      }
   }

bool PolyBuffer::MPolyBuilder::Intersect( const MVector &v1, const MVector &v2, const MVector &v3, const MVector &v4, MVector &point )
   {
   // modified from Graphic Gems II: lines_intersect:  AUTHOR: Mukesh Prasad

   static double a1, a2, b1, b2, c1, c2; // Coefficients of line eqns.
   static double r1, r2, r3, r4;         // 'Sign' values
   static double denom, offset, num;     // Intermediate values

   // Compute a1, b1, c1, where line joining points 1 and 2
   // is "a1 x  +  b1 y  +  c1  =  0".

   a1 = v2.m_y - v1.m_y;
   b1 = v1.m_x - v2.m_x;
   c1 = v2.m_x * v1.m_y - v1.m_x * v2.m_y;

   // Compute r3 and r4

   r3 = a1 * v3.m_x + b1 * v3.m_y + c1;
   r4 = a1 * v4.m_x + b1 * v4.m_y + c1;

   // Check signs of r3 and r4.  If both c and d lie on
   // same side of line ab, the line segments do not intersect.

   if ( ! ISZERO(r3) && ! ISZERO(r4) && !( ( r3 > 0 ) ^ ( r4 > 0 ) ) )
      return false;

   // Compute a2, b2, c2

   a2 = v4.m_y - v3.m_y;
   b2 = v3.m_x - v4.m_x;
   c2 = v4.m_x * v3.m_y - v3.m_x * v4.m_y;

   // Compute r1 and r2

   r1 = a2 * v1.m_x + b2 * v1.m_y + c2;
   r2 = a2 * v2.m_x + b2 * v2.m_y + c2;

   // Check signs of r1 and r2.  If both point 1 and point 2 lie
   // on same side of second line segment, the line segments do
   // not intersect.

   if ( ! ISZERO(r1) && ! ISZERO(r2) && !( ( r1 > 0 ) ^ ( r2 > 0 ) ) )
      return false;

   // Line segments intersect: compute intersection point. 

   denom = a1 * b2 - a2 * b1;
   if ( denom == 0.0 )
      {
      //ASSERT(0);
      //return ( COLLINEAR );
      return false;
      }
   
   //i// use this code if using integers
   //i//offset = denom < 0 ? - denom / 2 : denom / 2;

   //i// The denom/2 is to get rounding instead of truncating.  It
   //i// is added or subtracted to the numerator, depending upon the
   //i// sign of the numerator.

   //i//num = b1 * c2 - b2 * c1;
   //i//point.m_x = ( num < 0 ? num - offset : num + offset ) / denom;

   //i//num = a2 * c1 - a1 * c2;
   //i//point.m_y = ( num < 0 ? num - offset : num + offset ) / denom;

   point.m_x = ( b1 * c2 - b2 * c1 ) / denom;
   point.m_y = ( a2 * c1 - a1 * c2 ) / denom;

   return true;
   }

void PolyBuffer::MakeBuffer( const Poly &subjectPoly, Poly &resultPoly, float distance )
   {
   MStarDiagram starDiagram;

   MPolyBuilder polyBuilder;

   double x, y, xn, yn;

   /////////////////////////////////////////////////
   // add a circle of radius distance to starDiagram

   int N = 20;
   double h = 2*3.1415926535897932384626/N;
   //double a1 = ( 4.0 - h*h )/( 4.0 + h*h );
   //double a2 = ( 4.0*h )/( 4.0 + h*h );

   x = distance;
   y = 0.0;
   for ( int i=1; i<N; i++ )
      {
      //xn = a1*x - a2*y;
      //yn = a2*x + a1*y;
      
      xn = distance*cos( -h*i );
      yn = distance*sin( -h*i );

      starDiagram.Add( xn - x, yn - y, -1  );

      x = xn;
      y = yn;
      }

   xn = distance;
   yn = 0.0;
   starDiagram.Add( xn - x, yn - y, -1 );

   /////////////////////////////////////
   // For Each Part in subjectPoly

   int parts = (int) subjectPoly.m_vertexPartArray.GetSize();
   ASSERT( parts > 0 ); //must have atleast one part

   int startPart = subjectPoly.m_vertexPartArray[ 0 ];
   int nextPart = 0;

   for ( int part=0; part < parts; part++ )
      {
      if ( part < parts-1 )
         nextPart = subjectPoly.m_vertexPartArray[ part+1 ];
      else
         nextPart = subjectPoly.GetVertexCount();

      ASSERT( nextPart - startPart >= 4 ); // a ring is composed of 4 or more points

      //f//
      //if ( part > 2 )
      //   {
      //   startPart = nextPart;
      //   continue;
      //   }
      //f//

      int startIndex = -1;
      REAL startIndexY = FLT_MIN;

      ///////////////////////////////////
      // add this part as the subjectPoly
      starDiagram.ClearSubjectSides();
      double area = 0.0f;
      for ( int j=startPart; j<nextPart-1; j++ )
         {
         const Vertex &v1 = subjectPoly.m_vertexArray[ j ];
         const Vertex &v2 = subjectPoly.m_vertexArray[ j+1 ];
         starDiagram.Add( v2.x - v1.x, v2.y - v1.y, j );

         const Vertex &v3 = subjectPoly.m_vertexArray[ ( ( j+2 - startPart ) % ( nextPart - startPart - 1) ) + startPart ];
         area += v2.x * v1.y - v1.x * v2.y;

         if ( v1.y > startIndexY )
            {
            startIndex   = j;
            startIndexY = v1.y;
            }
         }

      // TODO: Get rid of this (area can be negative and large)
      if ( area < 0 )
         {
         startPart = nextPart;
         continue;
         }
     
      if (area == 0)  return; //ASSERT( area != 0 );
      ASSERT( startIndex >= 0 );
      
      ///////////////////////
      // Convolution

      starDiagram.Sort();

      const MVector *pEdge;
      const MVector *pNextEdge;
      const MVector *pNextSubjectEdge;
      int   subjectIndex = startIndex;
      int   stopIndex    = ( subjectIndex - 1 - startPart + ( nextPart - startPart - 1 ) ) % ( nextPart - startPart - 1 ) + startPart;


      x = subjectPoly.m_vertexArray[ startIndex ].x;
      y = subjectPoly.m_vertexArray[ startIndex ].y + distance;

      // Find the first edge that is either the subjectIndex of part of the circle
      for (;;)
         {
         pNextEdge = starDiagram.GetNext();
         if ( pNextEdge->m_subjectSide < 0 || pNextEdge->m_subjectSide == subjectIndex )
            break;
         }

            
      // Add First Point
      polyBuilder.Add( x, y );
      bool finishUp = false;

      for (;;)
         {
         pEdge = pNextEdge;

         pNextEdge = starDiagram.GetNext();
         if ( subjectIndex == stopIndex && pEdge->m_subjectSide < 0 )
            int shit = 0;

         if ( subjectIndex == stopIndex )
            finishUp = true;

         if ( finishUp && starDiagram.m_index == 0 )
            break;

         if ( pEdge->m_subjectSide == subjectIndex )
            {
            int starIndex;
            double signedArea;
            bool done = false;

            for (;;)
               {
               x += pEdge->m_x;
               y += pEdge->m_y;

               subjectIndex = ( subjectIndex + 1 - startPart ) % ( nextPart - startPart - 1 ) + startPart;

               polyBuilder.Add( x, y );

               // look for a left turn
               pNextSubjectEdge = starDiagram.GetSubject( subjectIndex, starIndex );
               signedArea = pEdge->m_x * pNextSubjectEdge->m_y - pNextSubjectEdge->m_x * pEdge->m_y;

               if ( ISZERO( signedArea ) )
                  {
                  starDiagram.m_index = starIndex;
                  pEdge = pNextSubjectEdge;
                  pNextEdge = starDiagram.GetNext();

                  if ( subjectIndex == stopIndex )
                     {
                     done = true;
                     break;
                     }
                  }

               else if ( signedArea > 0 )
                  {
                  pNextEdge = starDiagram.BufferAtVertex( subjectPoly, subjectIndex, subjectIndex, x, y, distance );
                  ASSERT( pNextEdge == pNextSubjectEdge );

                  polyBuilder.Add( x, y );

                  starDiagram.m_index = starIndex;
                  pEdge = pNextSubjectEdge;
                  pNextEdge = starDiagram.GetNext();
  
                  if ( subjectIndex == stopIndex )
                     {
                     done = true;
                     break;
                     }
                  }

               else
                  break;
               }

            if ( done )
               break;
            }
         else if ( pEdge->m_subjectSide < 0 )
            {
            x += pEdge->m_x;
            y += pEdge->m_y;

            polyBuilder.Add( x, y );
            }
         }   

      polyBuilder.MakePart( area < 0 );

      startPart = nextPart;
      }

   // Make resultPoly
   polyBuilder.MakePoly( resultPoly );
   }
