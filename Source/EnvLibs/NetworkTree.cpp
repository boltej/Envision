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

#include "NetworkTree.h"

#ifndef NO_MFC
#include <afxtempl.h>
#endif

NetworkNode::~NetworkNode()
   {/*
   if ( m_pLeft != NULL )
      {
      if ( ! _CrtIsValidHeapPointer( (const void *) m_pLeft ) )
         {
         CString msg;
         msg.Format( "Invalid left pointer for network node: edge: %i, order: %i", m_edgeIndex, m_order );
         TRACE( msg );
         }
      else
         delete m_pLeft;
      }
   
   if ( m_pRight != NULL )
      {
      if ( ! _CrtIsValidHeapPointer( (const void *) m_pRight ) )
         {
         CString msg;
         msg.Format( "Invalid right pointer for network node: edge: %i, order: %i", m_edgeIndex, m_order );
         TRACE( msg );
         }
      else
         delete m_pRight;
      }*/
   }


NetworkTree::NetworkTree(void)
:  m_pLayer( NULL )
,  m_phantomNodeCount( 0 )
,  m_unbranchedCount( 0 )
,  m_totalNodeCount( 0 )
,  m_rootNodeCount( 0 )
,  m_colToNode( -1 )
   {  }


NetworkTree::~NetworkTree(void)
   {
   DeleteTrees();
   }


void NetworkTree::DeleteTrees()
   {
   for( int i=0; i < this->m_roots.GetSize(); i++ )
      {
      NetworkNode *pNode = m_roots[ i ];
      //delete pNode;     // deletes all upstream nodes as well
     }

   m_roots.RemoveAll();
   m_phantomNodeCount = 0;
   m_unbranchedCount = 0;
   m_totalNodeCount = 0;
   m_rootNodeCount = 0;
   }



int NetworkTree::BuildTree( MapLayer *pLayer, int colEdgeID, int colToNode )
   {
   //-------------------------------------------------------------------------------------
   // this method builds the tree from attributes in the association network coverage.
   // it assumes a table structure like the following:
   //
   //   2(ID=366)     0 (ID=292)
   //  -----------+---------------
   //              \
   //               \ 1 (ID-=185)
   //                \
   //
   //  -----------------------------------------------------------
   //  | Index  |EdgeID |  ToID |   etc...
   //  |--------|-------|-------|---------------------------------
   //  |   0    |  292  |  366  | this one is upstream from 2
   //  |   1    |  185  |  366  | this one is upstrem from 2
   //  |   2    |  366  |       | .....................
   //  |--------|-------|-------|---------------------------------
   //  where: FromID field entries point to upstream EdgeIDs,
   //         ToID field entries point to downstream EdgeIDs
   //  
   //  Notes: 1) if colEdgeID < 0, the entries in the FromNode and ToNode columns are assumed
   //            to be indexes, not EdgeID entries
   //         2) Only one of colFromID and colToID have to be specified, but if both are specified, that works
   //         3) Empty entries imply (a) root node (no "To" entry) or leaves (no "From" entry)
   //
   // returns: number of edges in tree(s)
   // ------------------------------------------------------------------------------------

   ASSERT( colToNode >= 0 );

   DeleteTrees();

   if( pLayer->GetPolygonCount() != pLayer->GetRecordCount() )
      return -1;

   m_pLayer = pLayer;

   m_phantomNodeCount = 0;
   m_unbranchedCount = 0;
   m_badToNodeCount = 0;

   // allocate the necessary NetworkNodes
   NetworkNode *nodeArray = new NetworkNode[ pLayer->GetPolygonCount() ];

   // for each node, determine it's connections
   int records = pLayer->GetRowCount();
   ASSERT( records == pLayer->GetPolygonCount() );

   for ( int thisIndex=0; thisIndex < records; thisIndex++ )
      {
      NetworkNode *pNode = nodeArray + thisIndex;
      pNode->m_edgeIndex = thisIndex;

      // determine "to" connection for this edge
      int toNodeIndex = -1;
      pLayer->GetData( thisIndex, colToNode, toNodeIndex );

      if ( colEdgeID >= 0 )
         toNodeIndex = pLayer->FindIndex( colEdgeID, toNodeIndex );

      // we now know where "thisIndex" goes "to", fix up network node ptr
      if ( toNodeIndex >= records )
         m_badToNodeCount++;

      else if ( toNodeIndex >= 0 )
         {
         NetworkNode *pDown = nodeArray + toNodeIndex;

         pNode->m_pDown = pDown;  // set thisIndex's down ptr

         if ( pDown->m_pLeft == NULL ) // left node available?
            pDown->m_pLeft = pNode;
         else if ( pDown->m_pRight == NULL ) // right node available?
            pDown->m_pRight = pNode;
         else
            {
            // down node's left node and right node already occupied - need phantom node
            NetworkNode *pPhantom = new NetworkNode;
            pPhantom->m_edgeIndex = pDown->m_edgeIndex;  // phantom nodes have same ID as their downstream nodes
            pPhantom->m_isPhantom = true;

            // fix up upstream nodes to point to phantom
            pDown->m_pLeft->m_pDown = pPhantom;
            pDown->m_pRight->m_pDown = pPhantom;
            pPhantom->m_pLeft = pDown->m_pLeft;
            pPhantom->m_pRight = pDown->m_pRight;

            pPhantom->m_pDown = pDown;
            pDown->m_pLeft = pPhantom;
            pDown->m_pRight = pNode;

            m_phantomNodeCount++;
            }
         }
      else  // toNodeIndex < 0 
         pNode->m_pDown = NULL;
      }

   // at this point, all the trees are built and the nodes connected.  store all the roots
   for ( int thisIndex=0; thisIndex < pLayer->GetRowCount(); thisIndex++ )
      {
      NetworkNode *pNode = nodeArray + thisIndex;

      if ( pNode->IsRootNode() )
         m_roots.Add( pNode );

      if ( ! pNode->IsBranch() )
         m_unbranchedCount++;
      }

   m_rootNodeCount = (int) m_roots.GetSize();
   m_totalNodeCount = pLayer->GetRecordCount() + m_phantomNodeCount;

   return m_totalNodeCount;
   }


int NetworkTree::PopulateTreeID( int colTreeID /*=-1*/ )
   {
   if ( colTreeID < 0 )
      {
      colTreeID = m_pLayer->GetFieldCol( _T("TreeID") );
      if ( colTreeID < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         colTreeID = m_pLayer->m_pDbTable->AddField( _T("TreeID"), TYPE_INT, width, decimals, true );
         }
      }

   ASSERT( colTreeID >= 0 );

   m_pLayer->SetColNoData( colTreeID );

   int dups = 0;

   for ( int i=0; i < m_roots.GetCount(); i++ )
      {
      NetworkNode *pNode = m_roots[ i ];
      dups += PopulateTreeID( pNode, colTreeID, i );
      }

   return dups;
   }


int NetworkTree::PopulateTreeID( NetworkNode *pNode, int colTreeID, int treeIndex )
   {
   int dups=0;

   if ( pNode->m_pLeft != NULL )
      dups = PopulateTreeID( pNode->m_pLeft, colTreeID, treeIndex );

   if ( pNode->m_pRight != NULL )
      dups += PopulateTreeID( pNode->m_pRight, colTreeID, treeIndex );

   if ( ! pNode->IsPhantomNode() ) // not a phantom node?
      {
      int value;
      m_pLayer->GetData( pNode->m_edgeIndex, colTreeID, value );
      if ( int (m_pLayer->GetNoDataValue()) == value )
         m_pLayer->SetData( pNode->m_edgeIndex, colTreeID, treeIndex );
      else
         dups++;
      }

   return dups;
   }


int NetworkTree::PopulateOrder( int colOrder/*=-1*/ )
   {
   if ( colOrder < 0 )
      {
      colOrder = m_pLayer->GetFieldCol( _T("Order") );
      if ( colOrder < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         colOrder = m_pLayer->m_pDbTable->AddField( _T("Order"), TYPE_INT, width, decimals, true );
         }
      }

   ASSERT( colOrder >= 0 );

   m_pLayer->SetColNoData( colOrder );
   
   int maxOrder = 0;

   for ( int i=0; i < m_roots.GetSize(); i++ )
      PopulateOrder( colOrder, m_roots[ i ], 0, maxOrder );

   return maxOrder;
   }


void NetworkTree::PopulateOrder( int colOrder, NetworkNode *pNode, int orderSoFar, int &maxOrder )
   {
   if ( ! pNode->IsPhantomNode() )
      {
      m_pLayer->SetData( pNode->m_edgeIndex, colOrder, orderSoFar );
      pNode->m_order = orderSoFar;
      }

   if ( orderSoFar > maxOrder )
      maxOrder = orderSoFar;

   if ( pNode->m_pLeft != NULL )
      PopulateOrder( colOrder, pNode->m_pLeft, orderSoFar+1, maxOrder );

   if ( pNode->m_pRight != NULL )
      PopulateOrder( colOrder, pNode->m_pRight, orderSoFar+1, maxOrder );
   }


NetworkNode *NetworkTree::FindLeftLeaf( NetworkNode *pStartNode )
   {
   ASSERT( pStartNode != NULL );

   if ( pStartNode->m_pLeft == NULL ) 
      return pStartNode; 
   else 
      return FindLeftLeaf( pStartNode->m_pLeft ); 
   }

