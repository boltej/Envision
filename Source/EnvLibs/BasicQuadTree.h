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

class LIBSAPI BasicQuadTree
   {
   protected:
      BasicQuadTree( int depth );
      ~BasicQuadTree();

   public:
      int GetDepth(){ return m_depth; }

   protected:
      struct Direction
         {
         short x;
         short y;

         Direction() : x( 0 ), y( 0 ) {}
         Direction( short _x, short _y ) : x( _x ), y( _y ) {}
         };

      struct Node
         {
         Node( int x, int y, int size, Node *pParent );
         ~Node();

         enum CHILD { NE = 0, NW = 1, SW = 2, SE = 3 };

         int  m_x;  //bottom left corner
         int  m_y;
         int  m_size;
         
         void *m_pData;

         Node *m_pChild[4];
         Node *m_pParent;

         void Subdivide();
         void Merge();
         };

      void ClearAll();
      Node* GetNextNode( Node *pNode, short x, short y ){ return GetNextNode( pNode, Direction(x,y) ); }
      Node* GetNextNode( Node *pNode, Direction dir );
      Node* FindNode( int x, int y );

   protected:
      // recursive functions
      Node* FindDown( int x, int y, Node *pNode );
      Node* FindDown( int x, int y, int size, Node *pNode );
      Node* FindUp( int x, int y, int size, Node *pNode );

   protected:
      int m_depth;
      int m_size;  // = 2^(m_depth-1)
      Node *m_pRoot;
   };