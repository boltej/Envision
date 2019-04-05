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

#include "Maplayer.h"


//////////////////////////////////////////////////////////////////////////////////
// NetworkTree class
//
//  contains a tree representation for a network topology.
//
//  Note:  This differs from ReachTree in that:
//    1) No subnodes are maintained
//    2) One instance of the NetworkTree class can hold multiple tree, each with it's own root node
//      
//////////////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------------------------
// class NetworkNode represents a node in the binary tree represent the network
//
// a node exists for each edge in the network coverage, plus a root node.  Thus, a node
// is associated with the index (array offset) of the downstream edge in coverage
//------------------------------------------------------------------------------------------

class LIBSAPI NetworkNode        // class of the node in the B tree
   {
   public:
      NetworkNode *m_pLeft;         // pointer to left upstream node

      NetworkNode *m_pRight;        // pointer to right upstream node
      NetworkNode *m_pDown;         // pointer to the downstream node

      int m_edgeIndex;              // index of the associated (downstream) edge in the network coverage.
                                    //     note: phantom ID's are the negative of their base (downstream) edge index
      int  m_order;                 // order of the node in the network
      bool m_isPhantom;

      void *m_pData;

      //-- constructor --//
      NetworkNode()
         : m_pLeft( NULL ), m_pRight( NULL ), m_pDown( NULL ), 
           m_edgeIndex( -1 ), m_isPhantom( false ), m_order( -1 ), m_pData( NULL )
         { }

      ~NetworkNode();

      //float GetOutputFlow( void );  
      int  GetDownstreamVectorIndex( void ) { if ( m_pDown != NULL ) return m_pDown->m_edgeIndex; else return -1; }
      bool IsPhantomNode() { return m_isPhantom; }
      bool IsRootNode()    { return m_pDown == NULL; }
      bool IsLeafNode()    { return m_pLeft == NULL && m_pRight == NULL; }
      bool IsBranch()      { return ( m_pLeft != NULL && m_pRight != NULL ); }
   };


// a NetworkNodeArray is an array of nodes that represent roots of the Network trees
class LIBSAPI NetworkNodeArray : public CArray< NetworkNode*, NetworkNode* >
   {
   public:
      // ~NetworkNodeArray() { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i );  RemoveAll(); }
   };



class LIBSAPI NetworkTree
   {
   protected:
      MapLayer *m_pLayer;           // corresponding line coverage
      NetworkNodeArray m_roots;     // pointer to the root of the tree

      int m_phantomNodeCount;   // number of phantom nodes
      int m_unbranchedCount;
      int m_badToNodeCount;
      int m_totalNodeCount;
      int m_rootNodeCount;

   protected:
      int m_colToNode;
      int m_colEdgeID;

   public:
      NetworkTree( void );
      ~NetworkTree( void );

      //int BuildTree( MapLayer *pLayer, int colReachID=-1 );
      int BuildTree( MapLayer *pLayer, int colEdgeID, int colToNode );  // assumes topology present in datatable
      int PopulateOrder( int colOrder=-1 );
      int PopulateTreeID( int colTreeID=-1 );

      NetworkNode *GetRoot( int i ) { return m_roots[ i ]; }
      int GetRootCount() { return (int) m_roots.GetSize(); }
      int GetPhantomCount() { return m_phantomNodeCount; }
      int GetUnbranchedCount() { return m_unbranchedCount; }
      int GetBadToNodeCount() { return m_badToNodeCount; }

      NetworkNode *FindLeftLeaf( NetworkNode *pStartNode );

   protected:
      int  PopulateTreeID( NetworkNode  *pNode, int colTreeID, int treeIndex );
      void PopulateOrder( int colOrder, NetworkNode *pNode, int orderSoFar, int &maxOrder );
      void DeleteTrees();

   };
