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

#include "BasicQuadTree.h"

///////////////////////////////////////////////////////////////////
// Node

BasicQuadTree::Node::Node( int x, int y, int size, Node *pParent )
:  m_x( x ),
   m_y( y ),
   m_size( size ),
   m_pData( NULL ),
   m_pParent( pParent )
   {
   ASSERT( size > 0 );
   m_pChild[0] = NULL;
   m_pChild[1] = NULL;
   m_pChild[2] = NULL;
   m_pChild[3] = NULL;
   }

BasicQuadTree::Node::~Node()
   {
   for ( int i=0; i<4; i++ )
      if ( m_pChild[i] != NULL )
         delete m_pChild[i];
   }

void BasicQuadTree::Node::Subdivide()
   {
   ASSERT( m_size >= 2 );

   #ifdef _DEBUG
      for ( int i=0; i<4; i++ ) { ASSERT( m_pChild[i] == NULL ); }
   #endif

   int size = m_size/2;

   m_pChild[SW] = new Node( m_x       , m_y     , size, this );
   m_pChild[NW] = new Node( m_x       , m_y+size, size, this );
   m_pChild[SE] = new Node( m_x + size, m_y     , size, this );
   m_pChild[NE] = new Node( m_x + size, m_y+size, size, this );
   }

void BasicQuadTree::Node::Merge()
   {
   for ( int i=0; i<4; i++ )
      {
      delete m_pChild[i];
      m_pChild[i] = NULL;
      }
   }

///////////////////////////////////////////////////////////////////
// BasicQuadTree

BasicQuadTree::BasicQuadTree( int depth )
   {
   m_depth = depth;
   m_size  = 1;
   for ( int i=0; i<depth-1; i++ )
      m_size *= 2;
   
   m_pRoot = new Node( 0, 0, m_size, NULL );
   }

BasicQuadTree::~BasicQuadTree()
   {
   if ( m_pRoot != NULL )
      delete m_pRoot;
   }

void BasicQuadTree::ClearAll()
   {
   m_pRoot->Merge();
   }

BasicQuadTree::Node* BasicQuadTree::GetNextNode( Node *pNode, Direction dir )
   {
   int size = pNode->m_size;
   int x    = pNode->m_x + dir.x*size;
   int y    = pNode->m_y + dir.y*size;

   if ( x < 0 || m_size <= x || y < 0 || m_size <= y )
      return NULL;
   
   // Go Up
   pNode = FindUp( x, y, size, pNode );

   // Go Down
   pNode = FindDown( x, y, size, pNode );
   
   return pNode;
   }

BasicQuadTree::Node* BasicQuadTree::FindNode( int x, int y )
   {
   return FindDown( x, y, m_pRoot );
   }

BasicQuadTree::Node* BasicQuadTree::FindDown( int x, int y, Node *pNode )
   {
   if( x < pNode->m_x || pNode->m_x + pNode->m_size <= x || 
       y < pNode->m_y || pNode->m_y + pNode->m_size <= y    )
      return NULL;

   if ( pNode->m_pChild[0] == NULL )
      {
      return pNode;
      }

   if ( x < pNode->m_x + pNode->m_size/2 )
      {
      // West
      if ( y < pNode->m_y + pNode->m_size/2 )
         {
         // SouthWest
         return FindDown( x, y, pNode->m_pChild[ Node::SW ] );
         }
      else
         {
         return FindDown( x, y, pNode->m_pChild[ Node::NW ] );
         }
      }
   else
      {
      // East
      if ( y < pNode->m_y + pNode->m_size/2 )
         {
         // SouthEast
         return FindDown( x, y, pNode->m_pChild[ Node::SE ] );
         }
      else
         {
         // NorthEast
         return FindDown( x, y, pNode->m_pChild[ Node::NE ] );
         }
      }
   }

BasicQuadTree::Node* BasicQuadTree::FindDown( int x, int y, int size, Node *pNode )
   {
   ASSERT( pNode->m_x <= x && x < pNode->m_x + pNode->m_size );
   ASSERT( pNode->m_y <= y && y < pNode->m_y + pNode->m_size );
   ASSERT( size <= pNode->m_size );

   if ( pNode->m_pChild[0] == NULL || pNode->m_size == size )
      {
      return pNode;
      }

   if ( x < pNode->m_x + pNode->m_size/2 )
      {
      // West
      if ( y < pNode->m_y + pNode->m_size/2 )
         {
         // SouthWest
         return FindDown( x, y, size, pNode->m_pChild[ Node::SW ] );
         }
      else
         {
         return FindDown( x, y, size, pNode->m_pChild[ Node::NW ] );
         }
      }
   else
      {
      // East
      if ( y < pNode->m_y + pNode->m_size/2 )
         {
         // SouthEast
         return FindDown( x, y, size, pNode->m_pChild[ Node::SE ] );
         }
      else
         {
         // NorthEast
         return FindDown( x, y, size, pNode->m_pChild[ Node::NE ] );
         }
      }
   }

BasicQuadTree::Node* BasicQuadTree::FindUp( int x, int y, int size, Node *pNode )
   {
   if ( pNode->m_x <= x && x < pNode->m_x + pNode->m_size &&
        pNode->m_y <= y && y < pNode->m_y + pNode->m_size &&
        size <= pNode->m_size                                 )
      {
      return pNode;
      }
   else
      {
      ASSERT( pNode->m_pParent );
      return FindUp( x, y, size, pNode->m_pParent );
      }
   }