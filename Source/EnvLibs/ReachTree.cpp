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

#include "reachtree.h"

#include <math.h>
#include <stdio.h>
#include "FDATAOBJ.H"
#include "GEOMETRY.HPP"
#include "UNITCONV.H"

#include "QueryEngine.h"



// note:  a maxDelta of <= 0 makes one subnode
int ReachNode::ComputeSubnodeLength( float maxDeltaX  )
   {
   //if ( qArray != NULL )
   //   delete [] qArray;

   // how many to allocate?
   int subnodeCount = 1;
   if ( maxDeltaX > 0 )
      subnodeCount = (int)(( m_length / maxDeltaX ) + 1);

   //qArray = new float[ subnodeCount ];

   //ASSERT( qArray != NULL );
   //for ( int i=0; i < subnodeCount; i++ )
   //   qArray[ i ] = qInit;

   m_deltaX = m_length / subnodeCount;
   return subnodeCount;
   }



ReachNode *ReachNode::GetDownstreamNode( bool skipPhantom )
   {
   if ( skipPhantom == false )
      return this->m_pDown;

   // otherwise, check phantom nodes
   ReachNode *pDown = this->m_pDown;

   while( pDown != NULL && pDown->IsPhantomNode() )
      pDown = pDown->m_pDown;

   return pDown;
   }



ReachTree::ReachTree( void ) 
: m_nodeArray( NULL )
, m_nodeCount( 0 )
, m_phantomCount( 0 )
, m_unbranchedCount( 0 )
, m_badToNodeCount( 0 )
, m_pStreamLayer( NULL )
, m_colReachID( -1 )
, m_rootNodeArray( false )      // memory is managed by m_nodeArray
, m_phantomNodeArray( true )    // memory is managed by this array
   { }


ReachTree::~ReachTree( void ) 
   {
   RemoveAll();
   }


void ReachTree::RemoveAll( void )
   {
   //for ( INT_PTR i=0; i < m_rootNodeArray.GetSize(); i++ )
   //   {
   //   ReachNode *pNode = m_rootNodeArray[ i ];
   //   DeleteNode( pNode );
   //   }

   for ( int i=0; i < m_nodeCount; i++ )
      delete m_nodeArray[ i ];

   if ( m_nodeArray != NULL ) 
      delete [] m_nodeArray;

   m_rootNodeArray.RemoveAll();
   m_phantomNodeArray.RemoveAll();

   m_nodeArray = NULL;

   m_nodeCount = 0;
   m_phantomCount = 0;
   m_unbranchedCount = 0;
   m_badToNodeCount = 0;
   }



// RemoveNode() converts the specified node to a phantom node and takes care of associated bookkeeping
int ReachTree::RemoveNode( ReachNode *pNode, bool deleteUpstreamNodes )
   {
   if ( pNode == NULL )
      return 0;

   int removeCount = 0;

   // take care of this one first
   if ( pNode->IsPhantomNode() )   // if phantom?
      {
      // do nothing - phantom nodes are maintained intact
      }
   else
      { // Non-phantom nodes - we convert this node to a phantom node
        // remove from the array and fix up the rest of the array
      int currentIndex = 0;
      for ( int i=0; i < this->m_nodeCount; i++ )
         {
         currentIndex = i;
         if ( pNode == this->m_nodeArray[ i ] )
            break;
         }

      // currentIndex will be pointing at pNode. Move remaining nodes up
      for ( int i=currentIndex; i < this->m_nodeCount-1; i++ )
         {
         ReachNode *pNextNode = this->m_nodeArray[ i+1 ];
         this->m_nodeArray[ i ] = pNextNode;
         pNextNode->m_reachIndex--;
         }

      this->m_nodeCount--;
      this->m_nodeArray[ m_nodeCount ] = NULL;

      // make this node a phantom node
      pNode->m_polyIndex = pNode->m_reachIndex = PHANTOM_NODE;
      m_phantomNodeArray.Add( pNode );
      removeCount++;
      }
   
   int leftCount = 0;
   int rightCount = 0; 
   if ( deleteUpstreamNodes )
      {
      if ( pNode->m_pLeft != NULL )
         leftCount = RemoveNode( pNode->m_pLeft, true );

      if ( pNode->m_pRight != NULL )
         rightCount = RemoveNode( pNode->m_pRight, true );

      removeCount += leftCount + rightCount;
      }

   return removeCount;
   }


Vertex &ReachTree::GetUpstreamVertex( Poly *pCurrentLine, bool errorCheck )
   {
    int vertexCount = pCurrentLine->GetVertexCount();
    ASSERT( vertexCount > 0 );
    
    
    if (errorCheck)
       { //  Be careful here - we are setting values of pointers...
       if ( pCurrentLine->GetVertex( vertexCount-1 ).z  <= pCurrentLine->GetVertex( 0 ).z )
          {
          Vertex &v = (Vertex&) pCurrentLine->GetVertex( vertexCount-1 );
          v.z  = pCurrentLine->GetVertex( 0 ).z + 1.0f; 
          }
       else
          {
          Vertex &v = (Vertex&) pCurrentLine->GetVertex( 0 );
          v.z  = pCurrentLine->GetVertex( vertexCount-1 ).z + 1.0f;
          }
      }


   if ( pCurrentLine->GetVertex( vertexCount-1 ).z  <= pCurrentLine->GetVertex( 0 ).z )
      {
      const Vertex &vCurrentFirst = pCurrentLine->GetVertex( 0 );
      return (Vertex&) vCurrentFirst;
      }

   else
      { 
      const Vertex &vCurrentFirst = pCurrentLine->GetVertex( vertexCount-1 );
      return (Vertex&) vCurrentFirst;
      }
      
   }

Vertex &ReachTree::GetDownstreamVertex( Poly *pCurrentLine , bool errorCheck)
   {
    int vertexCount = pCurrentLine->GetVertexCount();
    ASSERT( vertexCount > 0 );
 
      
    if ( pCurrentLine->GetVertex( vertexCount-1 ).z  < pCurrentLine->GetVertex( 0 ).z )
       {
       const Vertex &vCurrentLast = pCurrentLine->GetVertex( vertexCount-1 );
       return (Vertex&) vCurrentLast;
       }

    else
       { 
        const Vertex &vCurrentLast = pCurrentLine->GetVertex( 0 );
        return (Vertex&) vCurrentLast;
       }
   }


ReachNode *ReachTree::GetReachNodeFromReachID( int reachID, int *index /*=NULL*/ )
   {
   if ( this->m_colReachID < 0 )
      return NULL;

   for ( int i=0; i < m_nodeCount; i++ )
      {
      ReachNode *pNode = m_nodeArray[ i ];

      if ( pNode->m_reachID == reachID )
         {
         if ( index != NULL )
            *index = i;

         return pNode;
         }
      }

   if ( index != NULL )
      *index = -1;

   return NULL;
   }


ReachNode *ReachTree::GetReachNodeFromPolyIndex( int m_polyIndex, int *index /*=NULL*/ )
   {
   for ( int i=0; i < m_nodeCount; i++ )
      {
      ReachNode *pNode = m_nodeArray[ i ];

      if ( pNode->m_polyIndex == m_polyIndex )
         {
         if ( index != NULL )
            *index = i;

         return pNode;
         }
      }

   if ( index != NULL )
      *index = -1;

   return NULL;
   }


void ReachTree::CreateTreeDescription( CUIntArray &reachIdMap, CUIntArray &subnodeMap )
   {
   reachIdMap.SetSize(m_nodeCount);
   subnodeMap.SetSize(m_nodeCount);

   for ( int i=0; i < m_nodeCount; i++ )   
      {
      ReachNode *pNode = m_nodeArray[ i ];
      reachIdMap.SetAt( i, pNode->m_reachID );
      subnodeMap.SetAt( i, pNode->GetSubnodeCount() );
      }
   }   


//--------------------------------------------------------------------------------------
// ReachTree::BuildTree
//
//  
// The method builds the topology needed to route water through the linear network
// represented by the stream shape file.
//
// return value:
//  number of nodes created, or
// -1 = No stream reach layer has been loaded
// -2 = No vectors in stream coverage
// -3 = Too many downstream nodes - 1 max allowed
// -4 = Topology error
// -5 = Too many unresolved downstream node pointers found while building reach tree         
// -6 = No streams upstream of the root node found while building reach tree
//--------------------------------------------------------------------------------------
 

int ReachTree::BuildTree( MapLayer *pStreamLayer, REACHFACTORYFN pFactoryFn, int colReachID /* =-1 */, Query *pQuery /*=NULL*/ )
   {
   m_pStreamLayer = pStreamLayer;
   m_colReachID = colReachID;

   bool slopeCheck = false;   // this will switch to true if slopes are illegal
   int errorCode = InitBuild( pStreamLayer, pFactoryFn, colReachID, slopeCheck, pQuery );   // creates initial nodes, ordered same as polys in stream layer

   if ( errorCode < 0 )
      return errorCode;

   // UPSTREAM TOPOLOGY

   // now, each polyline has a node, fix up node pointers into correct tree structure
   for ( int i=0; i < m_nodeCount; i++ )
      {      
      ReachNode *pCurrentNode = m_nodeArray[ i ];

      // have a node, get corresponding polyline  
      int m_polyIndex = pCurrentNode->m_polyIndex;

      Poly *pCurrentLine = pStreamLayer->GetPolygon( m_polyIndex );

      int vertexCount = pCurrentLine->GetVertexCount();
      ASSERT( vertexCount > 0 );

      // NOTE:  WE'RE ASSUMING the first vertex is the upstream vertex!!! (well, not exactly) TODO
 
     // Vertex vCurrentFirst = GetUpstreamVertex( pCurrentLine, errorCheck );
     // Vertex vCurrentLast = GetDownstreamVertex( pCurrentLine );

      Vertex vCurrentFirst = pCurrentLine->GetVertex( 0 );
      Vertex vCurrentLast = pCurrentLine->GetVertex( vertexCount-1 );

      // now check all other lines to see if their downstream vertex matches Current upstream vertex
      for ( int j=0; j < m_nodeCount; j++ )
         {
         if ( j == i ) // skip "Current"
            continue;

         Poly *pLine = pStreamLayer->GetPolygon( j );

         // get last vertex (their downstream vertex)
        // Vertex vLineLast = GetDownstreamVertex( pLine );

         const Vertex &vLineLast = pLine->GetVertex( pLine->GetVertexCount() -1 );

         if ( vLineLast.x == vCurrentFirst.x  && vLineLast.y == vCurrentFirst.y )   // match found, so pLine must be upstream from "Current" line
            {
            // get node that corresponds to this poly line
            ReachNode *pNode = m_nodeArray[ j ];

            // store it in the "current" tree node (for the polyline being examined) - Note: Left filled before right!
            if ( pCurrentNode->m_pLeft == NULL )
               pCurrentNode->m_pLeft = pNode;

            else if ( pCurrentNode->m_pRight == NULL )
               pCurrentNode->m_pRight = pNode;

            else
               {
               // both node are occupied, create a new "phantom" node with no reach to 
               // take care of tertiary nodes
               ReachNode *pPhantomNode = new ReachNode;

               // initialize phantom node

               // fix up pointers for this new phantom node.
               ReachNode *pTemp = pCurrentNode->m_pRight;
               pCurrentNode->m_pRight = pPhantomNode; //the right pointer for the current node refers to
                                                      //this new phantom node

               pPhantomNode->m_pLeft = pTemp;         //the phantom node's left pointer refers to what was
                                                      //going to be the current node's right pointer
               
               pPhantomNode->m_pRight = pNode;   //the phantom node's right pointer is the stream that was 
                                                 //the third segment found draining into this current node

               // fix up nodes upstream from the phantom node to point to the phantom node
               pPhantomNode->m_pLeft->m_pDown = pPhantomNode->m_pRight->m_pDown = pPhantomNode;

               pPhantomNode->m_pDown = pCurrentNode; //and the phantom drains into the current node!

               pPhantomNode->m_polyIndex = PHANTOM_NODE;  // indicates a phantom node/reach-note that a -1 indicates
                                                     // the root of the tree
               m_phantomNodeArray.Add( pPhantomNode );
               m_phantomCount++;
               
               //but now we also have a phantom reach with a m_length of 0.  This reach is ignored in the 
               //solve reach code.
               }
            }  // end of:  if ( vLine == vCurrentUp )

         
      // DOWNSTREAM TOPOLOGY

      // now check to see if the pLine's upstream (starting) vertex matches our downstream vertex
      // if it does, then that is the downstream pLine.

        const Vertex &vLineFirst = pLine->GetVertex( 0 );
      //   Vertex vLineFirst = GetUpstreamVertex( pLine );

         if ( vLineFirst.x == vCurrentLast.x  && vLineFirst.y == vCurrentLast.y)   // match found, 
                                                         //so pLine must be downstream from "Current" line
            {
            // get node that corresponds to this poly line (the pLine, not pCurrentLine )
            ReachNode *pNode = m_nodeArray[ j ];

            // store it in the "current" tree node (for the polyline being examined)
            if ( pCurrentNode->m_pDown == NULL )
                pCurrentNode->m_pDown = pNode;

            else
               {
               if ( pCurrentNode->m_pDown->m_polyIndex != PHANTOM_NODE )                
                  return -3; //ErrorMsg( "Too many downstream nodes - 1 max allowed, skipping..." );                  
               }
            }  // end of:  if ( vLine == vCurrentDn )
      
         }  // end of ( for j < lineCount ) - iterating through the "other" polys

      // check to make sure fixups occured, error if they didn't
      if ( pCurrentNode->m_pRight != NULL  && pCurrentNode->m_pLeft == NULL )
         //ErrorMsg( "Topology error...");
         return -4;
    
   
      //if ( pCurrentNode->m_pLeft != NULL && pCurrentNode->m_pRight == NULL )
      //   {
      //   errcount++;    // only one upstream pointer - okay
      //   }

      }  // done with this current poly line, move to next

   // ROOT NODE TOPOLOGY (FOR THE ENTIRE NETWORK)

   // now, look for unresolved nodes, these will be roots of the trees.

   for ( int j=0; j < m_nodeCount; j++ )        // so we must figure out what is upstream of it
      {
      ReachNode *pNode = m_nodeArray[ j ];
                                             
      if ( pNode->m_pDown == NULL )       // there should be one NULL m_pDown, and that is the reach
         m_rootNodeArray.Add( pNode );

      if ( pNode->m_pLeft != NULL && pNode->m_pRight == NULL )
         m_unbranchedCount++;
      }

   CString msg;
   msg.Format( "Success building reach topology for %s. %i Nodes (%i roots, %i phantom)", (LPCTSTR) pStreamLayer->m_name, m_nodeCount, (int) m_rootNodeArray.GetSize(), m_phantomCount );
   Report::Log( msg );   

   return m_nodeCount;
   }
      

int ReachTree::BuildTree( MapLayer *pLayer, REACHFACTORYFN pFactoryFn, int colEdgeID, int colToNode, int colReachID /*=-1*/, Query *pQuery /*= NULL*/ )
   {
   //-------------------------------------------------------------------------------------
   // this method builds the tree from attributes in the association network (stream) coverage.
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
   //         2) Empty entries imply (a) root node (no "To" entry) or leaves (no "From" entry)
   //
   // returns: number of edges in tree(s)
   // ------------------------------------------------------------------------------------

   ASSERT( colToNode >= 0 );
   m_pStreamLayer = pLayer;
   m_colReachID = colReachID;
   int polyCount = pLayer->GetRecordCount();

   bool slopeCheck;

   if ( pQuery != NULL )
      {
      if ( pQuery->GetQueryEngine()->m_pMapLayer != m_pStreamLayer )
         {
         ASSERT( 0 );
         TRACE( "Bad Query in ReachTree - doesn't match stream layer" );
         pQuery = NULL;
         }
      }

   // allocate REACH_INFOs
   int errorCode = InitBuild( pLayer, pFactoryFn, colReachID, slopeCheck, pQuery );

   if ( errorCode < 0 )
      return errorCode;

   // for each node, determine it's connections
   for ( int thisIndex=0; thisIndex < m_nodeCount; thisIndex++ )
      {
      ReachNode *pNode = m_nodeArray[ thisIndex ];

      //pNode->index = thisIndex;

      // determine "to" connection for this edge
      int toNodeIndex = -1;
      int m_polyIndex = pNode->m_polyIndex;

      pLayer->GetData( m_polyIndex, colToNode, toNodeIndex );

      // if this refers to an index, we're done.  Otherwise look up the value to get the index
      if ( colEdgeID >= 0 )
         toNodeIndex = pLayer->FindIndex( colEdgeID, toNodeIndex );

      // we now know where "thisIndex" goes "to", fix up network node ptr
      if ( toNodeIndex >= polyCount )
         m_badToNodeCount++;

      else if ( toNodeIndex >= 0 )
         {
         ReachNode *pDown = NULL;

         if ( pQuery )
            pDown = GetReachNodeFromPolyIndex( toNodeIndex );
         else
            pDown = m_nodeArray[ toNodeIndex ];

         pNode->m_pDown = pDown;  // set thisIndex's down ptr

         if ( pDown->m_pLeft == NULL ) // left node available?
            pDown->m_pLeft = pNode;
         else if ( pDown->m_pRight == NULL ) // right node available?
            pDown->m_pRight = pNode;
         else if (pNode->m_reachID==pDown->m_pLeft->m_reachID) 
            continue;
         else if (pNode->m_reachID==pDown->m_pRight->m_reachID)
            continue;
         else
            {
            // down node's left node and right node already occupied - need phantom node
            ReachNode *pPhantom = new ReachNode;
            pPhantom->m_polyIndex = PHANTOM_NODE; //pDown->index;  // phantom nodes have same ID as their downstream nodes
            pPhantom->m_reachIndex = PHANTOM_NODE;

            // fix up upstream nodes to point to phantom
            pDown->m_pLeft->m_pDown = pPhantom;
            pDown->m_pRight->m_pDown = pPhantom;
            pPhantom->m_pLeft = pDown->m_pLeft;
            pPhantom->m_pRight = pDown->m_pRight;

            pPhantom->m_pDown = pDown;
            pDown->m_pLeft = pPhantom;
            pDown->m_pRight = pNode;

            m_phantomNodeArray.Add( pPhantom );
            m_phantomCount++;
            }
         }
      else  // toNodeIndex < 0 
         pNode->m_pDown = NULL;
      }

   // at this point, all the trees are built and the nodes connected.  store all the roots
   for ( int thisIndex=0; thisIndex < m_nodeCount; thisIndex++ )
      {
      ReachNode *pNode = m_nodeArray[ thisIndex ];

      if ( pNode->IsRootNode() )
         m_rootNodeArray.Add( pNode );

      if ( pNode->m_pRight == NULL && pNode->m_pLeft != NULL )
         m_unbranchedCount++;
      }

   CString msg;
   msg.Format( "Success building reach topology for %s. %i Nodes (%i roots, %i phantom)", (LPCTSTR) pLayer->m_name, m_nodeCount, (int) m_rootNodeArray.GetSize(), m_phantomCount );
   Report::Log( msg );   

   return m_nodeCount;
   }
   

int ReachTree::InitBuild( MapLayer *pLayer, REACHFACTORYFN pFactoryFn, int colReachID, bool &slopeCheck, Query *pQuery )
   {
   m_phantomCount = 0;
   m_unbranchedCount = 0;
   m_badToNodeCount = 0;

   slopeCheck = false; // This will switch to a true if the z values for a line are incorrect
   if ( pLayer == NULL )
      {
      Report::ErrorMsg( "No stream layer provided...aborting..." );
      return -1;
      }
   
   if ( pLayer->m_pData == NULL )
      {
      Report::ErrorMsg( "No stream data is available, probable cause is that the includeData flag in the startup file was turned off...aborting..." );
      return -2;
      }

   // clear any existing reaches
   RemoveAll();

   // start building tree
   // notes: 
   // 1) each polyline in the coverage will have a single upstream tree node
   //
   // cycle through the coverage polylines, making a node for each 
   int polyCount = pLayer->GetRecordCount();
   m_nodeCount = polyCount;

   if ( pQuery != NULL)
      m_nodeCount = pQuery->Select( true ); //, false );
     
   if ( m_nodeCount == 0 )
      return -2;

   // create a temporary array of pointers to binary tree nodes  (one for each line in the coverage)
   if ( m_nodeArray != NULL )
      delete [] m_nodeArray;

   m_nodeArray = new ReachNode*[ m_nodeCount ];   // ONLY nodes with non-phantom reach info

   int errcount = 0;

   // initialize node and reach info
   for ( int i=0; i < m_nodeCount; i++ )
      {
      int m_polyIndex = pQuery ? m_pStreamLayer->GetSelection( i ) : i;

      // allocate a node
      ReachNode *pNode = NULL;
      if ( pFactoryFn )
         pNode = pFactoryFn();
      else
         pNode = new ReachNode;

      // store it temporarily
      m_nodeArray[ i ] = pNode;

      pNode->m_polyIndex  = m_polyIndex;  // for non-phantom node in tree
      pNode->m_reachIndex = i;

      // m_length
      Poly *pLine = pLayer->GetPolygon( m_polyIndex );
      int vertexCount = pLine->GetVertexCount();
      ASSERT( vertexCount > 0 );

      float slope = 0.0001f;
      if ( vertexCount > 0 )
         {
         float totalLength = 0.0f;
         for (int j = 0; j < vertexCount - 1; j++)
            {
            const Vertex &vStart = pLine->GetVertex(j);
            const Vertex &vEnd = pLine->GetVertex(j + 1);

            float length = (float) ( (vStart.x - vEnd.x)*(vStart.x - vEnd.x) + (vStart.y - vEnd.y)*(vStart.y - vEnd.y) );
            length = (float) sqrt(length);

            totalLength += length;
            }

         pNode->m_length = totalLength;

         // slope
         const Vertex &vTop = pLine->GetVertex(0);
         const Vertex &vBottom = pLine->GetVertex(vertexCount - 1);
         slope = (float)((vTop.z - vBottom.z) / totalLength);

         if ( slope <= 0.0f )
            slope = 0.0001f;
         }

      pNode->m_slope = slope;

      // add reachID info?
      if ( colReachID >= 0 )
         {
         int reachID;
         bool ok = pLayer->GetData( m_polyIndex, colReachID, reachID );
         ASSERT( ok );

         pNode->m_reachID = reachID;
         }
      else
         pNode->m_reachID = -1;
      }
   
   return m_nodeCount;
   }



int ReachTree::SaveTopology(LPCTSTR filename)
   {
   FILE *fp;
   fopen_s( &fp, filename, "wt" );

   if ( fp == NULL )
      {
      CString msg;
      msg.Format( "Unable to open file %s", filename );
      Report::ErrorMsg( msg );
      return -1;
      }

   //for ( INT_PTR i=0; i < m_rootNodeArray.GetSize(); i++ )
   //   SaveTree( fp, m_rootNodeArray[i], 0 );
   SaveRecords( fp );

   fclose( fp );

   return 1;
   }



int ReachTree::SaveTree( FILE *fp, ReachNode *pRoot, int level )
   {
   ASSERT (pRoot != NULL );
   
   int left, right, down, storeindex;
   float len;
       
   if ( pRoot->m_pLeft != NULL )      
     left = pRoot->m_pLeft->m_reachIndex;
   else
     left = -999;

   if ( pRoot->m_pRight != NULL )
     right = pRoot->m_pRight->m_reachIndex;
   else 
     right = -999;

   if (pRoot->m_pDown != NULL )
     down = pRoot->m_pDown->m_reachIndex;
   else
     down = -999;

   storeindex = pRoot->m_reachIndex;
   len = pRoot->m_length;

   for ( int i=0; i < level; i++ )
      fputc( ' ', fp);

   fprintf(fp, " %i, %lg, %i, %i, %i\n",  storeindex, len, left, right, down );

   if ( pRoot->m_pLeft != NULL )
      SaveTree(fp, pRoot->m_pLeft, level+1);
         
   if ( pRoot->m_pRight != NULL )
      SaveTree(fp, pRoot->m_pRight, level+1);

   return -1;   
   }


int ReachTree::SaveRecords( FILE *fp  )
   {
   ASSERT ( fp != NULL );
   
   // header
   fprintf(fp, "Index, Length, Left, Right, Down" );
   
   for( int i=0; i < this->GetReachCount(); i++ )
      {
      int left, right, down, storeindex;
      float len;

      ReachNode *pNode = GetReachNode( i );   // doesn't give phantoms
       
      if ( pNode->m_pLeft != NULL )      
        left = pNode->m_pLeft->m_reachIndex;
      else
         left = -999;

      if ( pNode->m_pRight != NULL )
        right = pNode->m_pRight->m_reachIndex;
      else 
        right = -999;

      if ( pNode->m_pDown != NULL )
        down = pNode->m_pDown->m_reachIndex;
      else
        down = -999;

      storeindex = pNode->m_reachIndex;
      len = pNode->m_length;

      fprintf(fp, " %i, %g, %i, %i, %i\n",  storeindex, len, left, right, down );
      }

   return -1;   
   }


// PopulateStreamOrder populates a database with the strahler stream order
int ReachTree::PopulateOrder( int colStreamOrder, bool calcOrder )
   {
   ASSERT( m_pStreamLayer != NULL );

   int maxOrder = 0;

   if ( calcOrder )
      {
      for ( int i=0; i < (int) m_rootNodeArray.GetSize(); i++ )
         {
         int order = CalculateOrder( m_rootNodeArray [ i ] );
         if ( order > maxOrder )
            maxOrder = order;
         }
      }

   if ( colStreamOrder >= 0 )
      {
      for ( int i=0; i < m_nodeCount; i++ )
         {
         ReachNode *pNode = m_nodeArray[ i ];

         int m_polyIndex = pNode->m_polyIndex;
         ASSERT( m_polyIndex >= 0 );
        // ASSERT( pNode->streamOrder > 0 );

         m_pStreamLayer->SetData( m_polyIndex, colStreamOrder, pNode->m_streamOrder );
         }
      }

   return maxOrder;
   }


//--------------------------------------------------------------------------------------
// ReachTree::CalculateStreamOrder
//
//  
// The method calculates the Strahler stream order for every node in a reachtree, except
// for the root node.  The stream order is stored in the node's streamOrder.
//
//
// basic idea - depth first algorithm
//
// k = 1 for leaf nodes - these are first order streams
// When 2 streams of the same order (k) meet, they form a stream of order k+1, however
// when a stream of order k is met by one of lower order  (say, k-1), no change occurs in the downstream order
//
// algorithm:
// 1) starting at the root, find the leftmost branch.  
//     If the left branch is a leaf, assign it a value of 1.
//     otherwise, recurse.
//     Repeat with the right-most branch.
// 
// 2) once the left side and right sides are calculated, determine the order of this reach according to:
//    a) if both left and right branches are the same order, this reaches order is one greater
//    b) else, is they are different orders, this reach is the max of the two orders
//    c) if only one upstream branch exists, this order is the same as the upstream branch.
//--------------------------------------------------------------------------------------

int ReachTree::CalculateOrder( ReachNode *pNode )
   {
#ifdef NO_MFC
     using namespace std;
#endif

   // are we at a leaf?
   if ( pNode->IsLeafNode() )
      {
      pNode->m_streamOrder = 1;
      return 1;
      }

   ASSERT( pNode->m_pLeft != NULL );

   int leftOrder = CalculateOrder( pNode->m_pLeft );
   if ( pNode->m_pRight == NULL )
      {
      pNode->m_streamOrder = leftOrder;
      return leftOrder;
      }

   int rightOrder = CalculateOrder( pNode->m_pRight );

   if ( leftOrder == rightOrder && ! pNode->IsPhantomNode() )
      pNode->m_streamOrder = leftOrder+1;
   else
      pNode->m_streamOrder = max( leftOrder, rightOrder );

   return pNode->m_streamOrder;
   }

   /*
   // get leftmost leaf
   ReachNode *pNode = FindLeftLeaf( pRoot );  // find leftmost leaf of entire tree (no params defaults to m_pRoot )
   pNode->streamOrder = 1;  

   while ( pNode->m_pDown != NULL )
      {
      ReachNode *pDown = pNode->m_pDown;

      // if we're coming from the left and a right branch exists, solve the right branch first

      // are we coming from the left? and right branch exists?

      if (( pDown->m_pLeft == pNode ) && ( pDown->m_pRight != NULL ))
         {
         pNode = FindLeftLeaf( pDown->m_pRight );
         pNode->streamOrder = 1;
         }
      
      else if (( pDown->m_pLeft == pNode ) && ( pDown->m_pRight == NULL ))
         {
         pDown->streamOrder = pNode->streamOrder;
         pNode = pDown;
         }

      else // We have all upstream orders, so can move downstream.
         {
         pNode = pDown;
         
         ASSERT( pNode->m_pLeft->streamOrder != -1 );
//         ASSERT( pNode->m_pRight->streamOrder != -1 );

         if (pNode->m_pRight != NULL)
            {

            if ( pNode->m_pLeft->streamOrder == pNode->m_pRight->streamOrder )
               pNode->streamOrder = pNode->m_pLeft->streamOrder + 1;
            
            else if ( pNode->m_pLeft->streamOrder > pNode->m_pRight->streamOrder )
               pNode->streamOrder = pNode->m_pLeft->streamOrder ;
                  
            else
               pNode->streamOrder = pNode->m_pRight->streamOrder ;
            }
         }

      ASSERT( pNode->streamOrder >= 1 );
      }
         
   return -5;
   }
   */

void ReachTree::CreateSubnodes( SUBNODEFACTORYFN pSubnodeFactoryFn, float maxReachDeltaX )
   {
   int reachCount = m_nodeCount;

   for ( int i=0; i < reachCount; ++i )
      {
      ReachNode *pNode = m_nodeArray[ i ];

      if ( pNode->IsPhantomNode() == false )
         {
         int subnodeCount = pNode->ComputeSubnodeLength( maxReachDeltaX );
         pNode->m_subnodeArray.SetSize(subnodeCount);
         
         for ( int j=0; j < subnodeCount; j++ ) 
            {
            SubNode *pSubnodeData = pSubnodeFactoryFn();
            pNode->m_subnodeArray.SetAt(j, pSubnodeData);
            SetSubnodeLocation( i, j, maxReachDeltaX);
            }
         }
      }
   }


void ReachTree::SetSubnodeLocation(int i, int j, float maxReachDeltaX)
   {
   ReachNode *pNode = m_nodeArray[ i ];

   SubNode *pSubnodeData = pNode->m_subnodeArray[j];
   int subnodeCount = pNode->ComputeSubnodeLength( maxReachDeltaX );

   float deltaX = pNode->m_deltaX;
   ASSERT( deltaX > 0 );

   //Step through vertices.  When total m_length is greater than deltaX, STOP!  
   //Interpolate between these two vertices to find the coordinates of the m_subnode
   int m_polyIndex = pNode->m_polyIndex;

   Poly *pLine = m_pStreamLayer->GetPolygon( m_polyIndex );
   int vertexCount = pLine->GetVertexCount();
   ASSERT( vertexCount > 0 );

   float totalLength = 0.0f;

   Vertex vStart = pLine->GetVertex( 0 );
   Vertex vEnd   = pLine->GetVertex( 1 );

   //First m_subnode is located at the NODE, (vertex 0).
   if (j == 0)
      {
      pSubnodeData->location.x = vStart.x;
      pSubnodeData->location.y = vStart.y;
      pSubnodeData->location.z = vStart.z;
      pNode->m_nodeLocation.x = pSubnodeData->location.x;
      pNode->m_nodeLocation.y = pSubnodeData->location.y;
      pNode->m_nodeLocation.z = pSubnodeData->location.z;
      }
   else
      {     
      for (int g = 0; g < vertexCount - 1; g++)
      {
         //These are the coordinates of the line segement
         vStart = pLine->GetVertex(g);
         vEnd = pLine->GetVertex(g + 1);

         //Use distance formula between the vertices
         float m_length = (float) ( (vStart.x - vEnd.x)*(vStart.x - vEnd.x) + (vStart.y - vEnd.y)*(vStart.y - vEnd.y) );
         m_length = (float)sqrt(m_length);
         totalLength += m_length;

         if (totalLength >= j*deltaX)
         {
            pSubnodeData->location.z = vEnd.z;
            break;
         }
      }
            
      //loop stops when totalLength is between vertices g and g+1
      //Now find location of m_subnode within this segment

      float segmentLength = totalLength - (j)*deltaX;
      ASSERT( segmentLength >=0 );

      float slope, x, y;

      // are the segments NOT vertical?
      if ( (vEnd.x - vStart.x) != 0 )
         {         
         slope = float( (vEnd.y - vStart.y)/(vEnd.x - vStart.x) );

         // The point-slope equation of a line is stated: (y - y1) = slope(x-x1) 
         //(y-y1) and (x-x1) are simply the lengths of 2 sides of a triangle, with
         //segmentLength as the hypotanuse.  These can be reassigned as simply x and y.
         //Using the pythagorean theorem, we can solve for the values of x and y
         //(x^2 + y^2) = (segmentLength)^2;
         //Our second equation is y = mx (from the point-slope form)
         //We have two equations and 2 unknowns!   (x and y)
         //Substitution yields the following equation
         //x = sqrt( (pow(segmentLength,2) )/( pow(slope,2) + 1 )             
                        
         x = (float)sqrt( (segmentLength*segmentLength) /( (slope*slope) + 1 ) );         
         y = (float)fabs( slope*x );
         }
      else  // they are vertical, treat as special case
         {
         x = 0;
         y = segmentLength;
         }
      

      //These values must now be simply added to the existing coordinates to 
      //determine the m_subnode location
      // initialize coords
      if (vStart.x > vEnd.x)
         pSubnodeData->location.x = vEnd.x + x;
      else
         pSubnodeData->location.x = vEnd.x - x;

      if (vStart.y > vEnd.y)
         pSubnodeData->location.y = vEnd.y + y;
      else
         pSubnodeData->location.y = vEnd.y - y;
      }
   }


void ReachTree::InterpolateSubnodes( MapLayer *pCellLayer, MapLayer *pFlowLayer, MapLayer *pWithdrawalsLayer, MapLayer *pStreamLayer, float m_flow_adjust /*= 0.0f*/ )
   {/*
   if ( pCellLayer == NULL )
      { Report::ErrorMsg( "No cell layer specified when interpolating reach subnodes" ); return; }

   ASSERT( pCellLayer->GetType() == LT_POLYGON );
   
   if ( pFlowLayer == NULL )
      { Report::ErrorMsg( "No flow layer specified when interpolating reach subnodes" ); return; }

   ASSERT( pFlowLayer->GetType() == LT_POINT );
   
   if( pWithdrawalsLayer != NULL )
      { ASSERT( pWithdrawalsLayer->GetType() == LT_POINT ); }

   if ( pStreamLayer == NULL )
      { Report::ErrorMsg( "No stream layer specified when interpolating reach subnodes" ); return; }

   ASSERT( pStreamLayer->GetType() == LT_LINE );
   
   const int FLOW_SCALAR = 100;   // 

   // basic idea:
   //
   // 0) compute reach/m_subnode corresponding to the observed point
   // 1) compute the reach drainage area/total watershed area ratios for each reach
   // 2) starting at the root of the tree, distribute observed flow upstream, 
   //    based on drainage areas

   // step 0 - find reach/subnodes associated with observed points
   //           as well as reach/subnodes with withdrawals

   int obsCount   = pFlowLayer->GetRecordCount();   

   if ( obsCount == 0 )
      { Report::ErrorMsg( "No flow observations found when interpolating subnodes" ); return; }

   OBSERVED_VALUE *obsValueArray = new OBSERVED_VALUE[ obsCount ];

   int wdCount = 0;
   WITHDRAWAL *wdArray = NULL;
   int wdCol = -1;
   if ( pWithdrawalsLayer != NULL )
      {
      wdCount = pWithdrawalsLayer->GetRecordCount();

      if ( wdCount == 0 )
         { Report::ErrorMsg( "No withdrawals found when interpolating subnodes" ); return; }

      wdArray = new WITHDRAWAL[ wdCount ];  
      wdCol   = pWithdrawalsLayer->GetFieldCol( "Cfs" );

      if ( wdCol < 0 )
         { Report::ErrorMsg( "No CFS column found in withdrawals when interpolating subnodes" ); return; }
      }

   int reachCount = m_nodeCount;

   if ( reachCount <= 0 )
         { Report::ErrorMsg( "No reaches found when interpolating subnodes" ); return; }
  
   int flowCol = pFlowLayer->GetFieldCol( "Cfs" );
   ASSERT( flowCol >= 0 );

   for ( int i=0; i < obsCount; i++ )
      {
      REAL xObs, yObs;
      bool ok = pFlowLayer->GetPointCoords( i, xObs, yObs );
      ASSERT( ok );

      // iterate through tree, looking for closest node/m_subnode
      int m_reachIndex   = -1;
      int subnodeIndex = -1;
      REAL distance = 1.0e10;
         
      int nearestm_polyIndex = pStreamLayer->GetNearestPoly( xObs, yObs );
      ASSERT( nearestm_polyIndex >= 0 );

      int j = 0;
      ReachNode *pNode = NULL;

      for ( j=0; j < reachCount; j++ )
         {
         ReachNode *_pNode = m_nodeArray[ j ];

         if ( _pNode->m_polyIndex == nearestm_polyIndex )
            {
            pNode = _pNode;
            break;
            }
         }

      ASSERT( pNode != NULL );   // make sure we found a node with the right index

      for ( int k=0; k < pNode->GetSubnodeCount(); k++ )
         {
         REAL x = pNode->m_subnodeArray[ k ]->location.x;
         REAL y = pNode->m_subnodeArray[ k ]->location.y;

         REAL _distance = DistancePtToPt( x, y, xObs, yObs );
            
         if ( _distance < distance )
            {
            distance = _distance;
            m_reachIndex = j;
            subnodeIndex = k;
            }
         }  // end of:  for ( k < subnodeCount )
         
      // store observation results
      float flow;
      ok = pFlowLayer->GetData( i, flowCol, flow );
      ASSERT( ok );

      obsValueArray[ i ].reachIndex   = m_reachIndex;
      obsValueArray[ i ].subnodeIndex = subnodeIndex;
      obsValueArray[ i ].value        = (flow / FT3_M3)* (1.0f + m_flow_adjust);   // convert from CFS to cubic meter/sec
      //m_flow adjust allows user to explore alternate flow regimes....
      //i.e. what would happen to temperature if flow increased by 10%   mmc 6/18/04
      }  // end of:  for ( i < obsCount )
   
   for ( int w=0; w < wdCount; w++ )
      {
      REAL xWD = 0;
      REAL yWD = 0;
      if ( pWithdrawalsLayer != NULL )
         {
         bool ok = pWithdrawalsLayer->GetPointCoords( w, xWD, yWD );
         ASSERT( ok );
         }

      // iterate through tree, looking for closest node/m_subnode
      int m_reachIndex   = -1;
      int subnodeIndex = -1;
      REAL distance = 1.0e10;

      for ( int j=0; j < reachCount; j++ )
         {
         ReachNode *pNode = m_nodeArray[ j ];
         int subnodeCount = pNode->GetSubnodeCount();

         for ( int k=0; k < subnodeCount; k++ )
            {
            REAL x = pNode->m_subnodeArray[ k ]->location.x;
            REAL y = pNode->m_subnodeArray[ k ]->location.y;

            REAL _distance = DistancePtToPt( x, y, xWD, yWD );
            
            if ( _distance < distance )
               {
               distance = _distance;
               m_reachIndex = j;
               subnodeIndex = k;
               }
            }  // end of:  for ( k < subnodeCount )
         }  // end of:  for ( j < reachCount )
               
      // store observation results
      float wdFlow = 0;
      if ( pWithdrawalsLayer != NULL )
         {
         bool ok = pWithdrawalsLayer->GetData( w, wdCol, wdFlow );
         ASSERT( ok );

         wdArray[ w ].reachIndex   = m_reachIndex;
         wdArray[ w ].subnodeIndex = subnodeIndex;
         wdArray[ w ].value        = wdFlow / FT3_M3;   // convert from CFS to cubic meter/sec
         }
      }  // end of:  for ( w < wdCount )

      
   // step 1 - compute reach drainage areas ratios
   float *areaArray   = new float[ reachCount ];  // store the drainage area for each reach
   float totalArea = ComputeReachDrainageAreas( areaArray, pCellLayer, pStreamLayer );
   
   for ( int i=0; i < reachCount; i++ )   // normalize areas to total area
      areaArray[ i ] /= totalArea;

   // also compute cumulate reach area ratios
   float *cumAreaArray = new float[ reachCount ];
   for ( INT_PTR i=0; i < m_rootNodeArray.GetSize(); i++ )   
      ComputeCumulativeReachDrainageAreas( m_rootNodeArray[ i ], cumAreaArray, areaArray, totalArea );

   // now, walk the flow network, upstream to downstream, distributing the flow observations to upstream reaches base
   // based on contributing areas
   float *flowArray = new float[ reachCount ];
   for ( int i=0; i < reachCount; i++ )
      flowArray[ i ] = -1.0f;

   for ( int i=0; i < obsCount; i++ )
      {
      int m_reachIndex = obsValueArray[ i ].reachIndex;
      ReachNode *pNode = m_nodeArray[ m_reachIndex ];
      
      int count = DistributeObservedFlows( pNode, obsValueArray, obsCount, areaArray, cumAreaArray, flowArray, true, wdArray, wdCount);
      }

   delete [] areaArray;
   delete [] cumAreaArray;
   delete [] flowArray;
   delete [] obsValueArray;
   if ( wdArray )
      delete [] wdArray;
   */
   }


int ReachTree::DistributeObservedFlows( ReachNode *pNode, OBSERVED_VALUE *obsValueArray, int obsValueCount, 
                                        float *areaArray, float *cumAreaArray, float *flowArray, bool firstNode,
                                        WITHDRAWAL*wdArray, int wdCount)
   {
   /*
   if ( pNode == NULL )
      return 0;

   int reachCount = 0;

   // if first time through, initialize to observed values
   if ( firstNode )
      {
      int m_reachIndex = pNode->m_reachIndex;

      bool found = false;
      for ( int i=0; i < obsValueCount; i++ )
         {
         if ( m_reachIndex == obsValueArray[ i ].reachIndex )
            {
            found = true;
            flowArray[ m_reachIndex ] = obsValueArray[ i ].value;
            break;
            }
         }
         
      if ( found == false )
         Report::ErrorMsg( "No observed flows" );
      }  // end of:  if ( firstNode )

   // start by getting the current flow value at this node (it should have already been assigned);
   float thisFlow = 0.0f;
   
   if ( pNode->IsPhantomNode() )    // if phantom, flow is ( down reach - its other trib )
      {
      int downIndex  = pNode->m_pDown->m_reachIndex;
      int otherIndex = 0;
      if ( pNode->m_pDown->m_pRight != pNode )  // "other" trib
         otherIndex = pNode->m_pDown->m_pRight->m_reachIndex;
      else
         otherIndex = pNode->m_pDown->m_pLeft->m_reachIndex;
      
      thisFlow = flowArray[ downIndex ] - flowArray[ otherIndex ];
      }
   else
      thisFlow = flowArray[ pNode->m_reachIndex ];

   ASSERT( thisFlow >= 0 );

   //  it this is a headwater - no further action required, other than to distribute the m_subnode flows
   if ( pNode->IsHeadwaterNode() )  // headwater reach, no further upstream processing required
      {
      DistributeSubnodeFlows( pNode, thisFlow, thisFlow );
      return 1;
      }

   // okay, this is a "regular" node, process it and recurse upstream

   // partition the flow, with cases for both lest and right sides.
   //  1) if no observation upstream, distribute according to area ratios and recurse
   //  2) if observation upstream, set to observed values
   
   float leftObs  = -1.0f;
   float rightObs = -1.0f;
   float leftWD   = -1.0f;
   float rightWD  = -1.0f;
   int   leftIndex  = pNode->m_pLeft->m_reachIndex;   // note - these could be phantom nodes (index = -2)

   // special case on right - may not exist
   int   rightIndex = -3;     // indicates right branch doesn't exist
   if ( pNode->m_pRight != NULL ) 
      rightIndex = pNode->m_pRight->m_reachIndex;

   // is there an observation or withdrawal for the left upstream reach?
   if ( leftIndex >= 0)   // skip phantom nodes
      {
      for ( int i=0; i < obsValueCount; i++ )
         {
         if ( obsValueArray[ i ].reachIndex == leftIndex )
            {
            leftObs = obsValueArray[ i ].value;
            break;
            }
         }

      for (int j=0; j < wdCount; j++)
         {
         if ( wdArray && ( wdArray[ j ].reachIndex == leftIndex ) )
            {
            leftWD = wdArray[ j ].value;
            break;
            }   
         }
      
      }

   // is there an observation or withdrawal for the right upstream reach?
   if ( rightIndex >= 0 )  // skip phantom/null nodes
      {
      for ( int i=0; i < obsValueCount; i++ )
         {
         if ( obsValueArray[ i ].reachIndex == rightIndex )
            {
            rightObs = obsValueArray[ i ].value;
            break;
            }
         }
   
      for ( int j=0; j < wdCount; j++ )
         {
         if ( wdArray && ( wdArray[ j ].reachIndex == rightIndex ) )
            {
            rightWD = wdArray[ j ].value;
            break;
            }
         }

      }

   float leftCumArea  = 0.0f;
   float rightCumArea = 0.0f;
   float thisArea = 0;

   if ( leftIndex < 0 )    // left index not a phantom node?
      leftCumArea = cumAreaArray[ pNode->m_pLeft->m_pLeft->m_reachIndex ] + cumAreaArray[ pNode->m_pLeft->m_pRight->m_reachIndex ];
   else
      leftCumArea  = cumAreaArray[ leftIndex ];

   if ( rightIndex == -2 )    // right index a phantom node?
      rightCumArea = cumAreaArray[ pNode->m_pRight->m_pLeft->m_reachIndex ] + cumAreaArray[ pNode->m_pRight->m_pRight->m_reachIndex ];
   else if ( rightIndex == -3 ) // right branch NULL
      rightCumArea = 0;
   else     // "normal" case
      rightCumArea = cumAreaArray[ rightIndex ];

   if ( pNode->IsPhantomNode() == false )
      thisArea = areaArray[ pNode->m_reachIndex ];

   float combinedArea = leftCumArea + rightCumArea + thisArea;

   // set flows based on partitioned areas
   if ( leftIndex >= 0 )
      {
      if ( leftObs >= 0.0f )
         flowArray[ leftIndex ] = leftObs;
      else if ( leftWD >= 0.0f )
         flowArray[ leftIndex ]  = (thisFlow + leftWD) * leftCumArea / combinedArea; 
      else
         flowArray[ leftIndex ]  = thisFlow * leftCumArea / combinedArea;
      }

   if ( rightIndex >= 0 )
      {
      if ( rightObs >= 0.0f )
         flowArray[ rightIndex ] = rightObs;
      else if ( rightWD >= 0.0f )
         flowArray[ rightIndex ]  = (thisFlow + rightWD) * rightCumArea / combinedArea; 
      else
         flowArray[ rightIndex ] = thisFlow * rightCumArea / combinedArea;
      }


   // all done partitioning flows, move upstream and repeat
   if ( leftObs < 0.0f )
      reachCount += DistributeObservedFlows( pNode->m_pLeft,  obsValueArray, obsValueCount, areaArray, cumAreaArray, flowArray, false, wdArray, wdCount);

   if ( rightObs < 0.0f )
      reachCount += DistributeObservedFlows( pNode->m_pRight, obsValueArray, obsValueCount, areaArray, cumAreaArray, flowArray, false, wdArray, wdCount);

   // finally distribute the m_subnode flows   
   if ( pNode->IsPhantomNode() == false )
      {
      float leftFlow = 0;
      if ( leftIndex < 0 )  // left is a phantom node, so sum both it's input flows
         {
         leftFlow = flowArray[ pNode->m_pLeft->m_pLeft->m_reachIndex ] ;

         if ( pNode->m_pLeft->m_pRight != NULL )   // make sure right node exists
            leftFlow += flowArray[ pNode->m_pLeft->m_pRight->m_reachIndex ];
         }
      else
         leftFlow = flowArray[ leftIndex ];

      float rightFlow = 0;
      if ( rightIndex == PHANTOM_NODE )  // is right node a phantom node?
         {
         rightFlow = flowArray[ pNode->m_pRight->m_pLeft->m_reachIndex ];

         if ( pNode->m_pRight->m_pRight )
            rightFlow += flowArray[ pNode->m_pRight->m_pRight->m_reachIndex ];
         }
      else
         {
         if ( pNode->m_pRight )
            rightFlow = flowArray[ rightIndex ];
         else
            rightFlow = 0.0f;
         }
      
      float lateralInflow  = thisFlow - ( leftFlow + rightFlow );
      DistributeSubnodeFlows( pNode, thisFlow, lateralInflow ); 
      reachCount++;
      }

   return reachCount;
   */
return 0;  // temp
   }

/*
void ReachTree::DistributeSubnodeFlows( ReachNode *pNode, float reachFlow, float lateralInflow )
   {
   // Note:  reachFlow is the flow at the end of the reach (i.e. the last m_subnode)
   int subnodeCount = pNode->GetSubnodeCount();
   float deltaX     = pNode->deltaX;
   float *qArray    = pNode->qArray;

   //record lateral inflow value for each reach
   pNode->reachLateralInflow = lateralInflow;

   // partition inflow over entire reach
   float subnodeInflow = lateralInflow / subnodeCount;
   qArray[ 0 ] = reachFlow - lateralInflow+0.5f*subnodeInflow;
   
   pNode->subnodeArray[ 0 ]->lateralInflow = (0.5f*subnodeInflow);

   //If qArray[0] is not at a headwater, lateral inflows for upstream segments must
   //be accounted for
   if ( pNode->m_pLeft != NULL )
      {
      int subCountLeft = pNode->m_pLeft->GetSubnodeCount();
      float leftLat = ((pNode->m_pLeft->reachLateralInflow/subCountLeft)*0.5f); 
      qArray[0] += leftLat;
      pNode->subnodeArray[0]->lateralInflow += leftLat;   
      }

   if ( pNode->m_pRight != NULL )
      {
      int subCountRight = pNode->m_pRight->GetSubnodeCount();
      float rightLat = ((pNode->m_pRight->reachLateralInflow/subCountRight)*0.5f); 
      qArray[0] += rightLat;
      pNode->subnodeArray[0]->lateralInflow += rightLat;
      }

   if ( pNode->IsHeadwaterNode() )  //mmc 10/4/02  Flow rate should not start at zero for headwater reaches
      {
      qArray[ 0 ] = 0.5f * reachFlow;  //Q at headwaters is 50% of reach flow instead of 0%
      //subnodeInflow = 0.0f;
      subnodeInflow = (0.5f * lateralInflow) / GetSubnodeCount();  //remaining 50% of reach flow is distributed
      }
   
   for ( int i=1; i < GetSubnodeCount(); i++ )
      {
      qArray[ i ] = qArray[ 0 ] + (i*subnodeInflow);
      
      //store lateral inflow value for graphing
      SubNode *pSubnodeData = pNode->subnodeArray[ i ];
      pSubnodeData->lateralInflow = subnodeInflow;
      }
   }


float ReachTree::ComputeReachDrainageAreas( float *areaArray, MapLayer *pCellLayer, MapLayer *pStreamLayer )
   {   
   ///////////////////////////////////////////////////////

   //For use with RESTORE cell coverages
   // get the column in the cell layer that has the reach ID in it

   int coverType = 1;   // RESTORE
   
   int colReachID = pCellLayer->GetFieldCol( "HYDRO_ID" );     // for RESTORE coverages
   if ( colReachID < 0 )
      {
      colReachID = pStreamLayer->GetFieldCol( "HYDRO_ID");     // for DEQ coverages
      coverType = 2;
      }

   if ( colReachID < 0 )
      { Report::ErrorMsg( "Unable to find Hydro ID col when computing drainage areas" ); return 0; }

   int colCellArea;
   
   if ( coverType == 1 )
      colCellArea = pCellLayer->GetFieldCol( "AREA" );        // for RESTORE coverages
   if ( coverType == 2 )
      colCellArea = pStreamLayer->GetFieldCol( "Area_Drain");      // for DEQ coverages

   if ( colCellArea < 0 )
      { Report::ErrorMsg( "Unable to find Cell Area col when computing drainage areas" ); return 0; }
   
   ////////////////////////////////////////////////////////

   // initialize area array
   for ( int i=0; i < m_nodeCount; i++ )
      areaArray[ i ] = 0.0f;
      
   float totalArea = 0.0f;


   ///////////////////For use with ODEQ coverages
   if ( coverType == 2 )
      {
      for ( int k=0; k < m_nodeCount; k++ )
         {
         float areaDrain;
         bool ok = pStreamLayer->GetData( k, colCellArea, areaDrain );
         ASSERT( ok );

         areaArray[ k ] = areaDrain;
         totalArea += areaDrain;
         }
      }
   else
      {
      //Code for RESTORE cell coverages
      // for each reach, find the corresponding area and store it in the area array
      for ( int i=0; i < pCellLayer->GetRecordCount(); i++ )
         {
         float cellArea;
         bool ok = pCellLayer->GetData( i, colCellArea, cellArea );
         ASSERT( ok );

         int reachID;
         ok = pCellLayer->GetData( i, colReachID, reachID );
         ASSERT( ok );

         // get reach offset from ID
         int index;
         GetReachNodeFromReachID( reachID, &index );

         if ( index < 0 || index >= m_nodeCount )
            {
            CString msg;
            msg.Format( "Bad reach index found: Index = %i, Reach ID = %i; ignoring...", index, reachID );
            //Report::WarningMsg( msg );
            TRACE( msg );
            }
         else
            {
            // add to appropriate area
            areaArray[ index ] += cellArea;
            totalArea += cellArea;
            }
         }  
      }

   return totalArea;
   }


float ReachTree::ComputeCumulativeReachDrainageAreas( ReachNode *pNode, float *cumAreaArray, float *areaArray, float totalArea )
   {
   float cumArea = 0.0f;

   //Attach distance to mouth to reach information
   float cumlength = 0.0f;
   if (pNode->m_pDown != NULL)  //m_pDown should only be NULL at the root
      {
      cumlength = pNode->m_pDown->distToMouth + pNode->m_length;
      pNode->distToMouth = cumlength;
      }

   // get the cumulative area ratio for the upstream reaches, if any
   if ( pNode->m_pLeft != NULL )
      cumArea += ComputeCumulativeReachDrainageAreas( pNode->m_pLeft, cumAreaArray, areaArray, totalArea );

   if ( pNode->m_pRight != NULL )
      cumArea += ComputeCumulativeReachDrainageAreas( pNode->m_pRight, cumAreaArray, areaArray, totalArea );

   // add the area ratio for this reach
   int m_reachIndex = pNode->m_reachIndex;
   ASSERT( m_reachIndex < m_nodeCount );

   if ( m_reachIndex != PHANTOM_NODE )
      {
      cumArea += areaArray[ m_reachIndex ];
      cumAreaArray[ m_reachIndex ] = cumArea;
      }

   //Store cumulative drainage area for each reach convert back to absolute (not normalized) area
   pNode->areaDrained = cumArea*totalArea;

   return cumArea;
   }
*/


ReachNode *ReachTree::FindLeftLeaf( ReachNode *pStartNode )
   {
   ASSERT( pStartNode != NULL );

   if ( pStartNode->m_pLeft == NULL ) 
      return pStartNode; 
   else 
      return FindLeftLeaf( pStartNode->m_pLeft ); 
   }


static int count=0;

bool ReachTree::BuildDownstreamDistanceMatrix( FDataObj *pDataObj )
   {
   // FDataObj will look like:
   //        Reach1  Reach2   Reach3   Reach4
   // Reach1   0       d21      d31      d41
   // Reach2   d21      0       d32      d42
   // Reach3   d31      d32      0       d43
   // Reach4   d41      d42      d43      0
   //
   // reaches are ordered by their position in the m_nodeArray
   //
   // basic idea. iterate through m_nodeArray. For each reach,  1) traverse up the network,
   //   recording distances as you move up the network (note: this is recursive), then
   //   2) move down the network, recording distances as you go.


   // alternative:  start at the root of the tree.  Recursively move up the tree, computing distance up the tree as you go.

   if ( pDataObj == NULL )
      return false;

   int reachCount = GetReachCount();
   if ( reachCount == 0 )
      return false;

   // initialize dataobject
   pDataObj->SetSize( reachCount, reachCount );
   for ( int i=0; i < reachCount; i++ )
      for ( int j=0; j < reachCount; j++ )
         pDataObj->Set( i, j, -1 );

   // recurse the tree for each node
   count = 0;
   for ( int i=0; i < reachCount; i++ )
      {
      ReachNode *pNode = m_nodeArray[ i ];
      BuildDownstreamDistanceMatrix( pNode, pNode->m_pLeft,  0, pDataObj );
      BuildDownstreamDistanceMatrix( pNode, pNode->m_pRight, 0, pDataObj );
      }

   // fill in remaining values
   for ( int i=0; i < reachCount; i++ )
      {
      for ( int j=0; j < reachCount; j++ )
         {
         if ( i != j )
            {
            float value = pDataObj->GetAsFloat( i, j );

            if ( value < 0.0001f )  // non-zero?
               {
               float d = pDataObj->GetAsFloat( j, i );
               //ASSERT( pDataObj->GetAsFloat( j, i ) < 0 );
               pDataObj->Set( i, j, d );
               }
            }
         else pDataObj->Set( i, j, 0 );
         }
      }
   return true;
   }



bool ReachTree::BuildDownstreamDistanceMatrix( ReachNode *pStartNode, ReachNode *pThisNode, float distanceSoFar, FDataObj *pDataObj )
   {
   // get downstream distances from distanceSoFar, which represents the distance to the ReachNode (pStartNode) that
   // originally invoked this branch of calculations.  Store this distance at the appropriate place in the DataObj and
   // recurse upwards towards the leafs in the tree.
   if ( pStartNode == NULL || pThisNode == NULL )
      return true;

   if ( pThisNode->m_pDown == NULL )
      return true;

   float thisLength = pThisNode->m_length;
   float downLength = pThisNode->m_pDown->m_length;
   float lengthToAdd = ( thisLength + downLength ) / 2;

   // if these nodes aren't phantom nodes (or a root node), record a value
   int startIndex = pStartNode->m_reachIndex;
   int thisIndex  = pThisNode->m_reachIndex;

   if ( startIndex >= 0 && thisIndex >= 0 )
      {
      count++;
      if ( count % 1000 == 0 )
         {
         CString msg;
         int maxValues = (pDataObj->GetRowCount()-1)*pDataObj->GetColCount()/2;
         msg.Format( "Building distance matrix distance %i of %i", count, maxValues );
         Report::StatusMsg( msg );
         }   

      pDataObj->Set( startIndex, thisIndex, distanceSoFar+lengthToAdd );
      }

   // if the startNode is not a phantom node (or root node), keep moving up the tree.  If it is a phantom node, 
   // then there is no reason to continue with it as a starting node
   if ( ! pStartNode->IsPhantomNode() )
      {
      // recurse up from the starting Node
      BuildDownstreamDistanceMatrix( pStartNode, pThisNode->m_pLeft,  distanceSoFar + lengthToAdd, pDataObj );
      BuildDownstreamDistanceMatrix( pStartNode, pThisNode->m_pRight, distanceSoFar + lengthToAdd, pDataObj );      
      }

   return true;
   }




bool ReachTree::BuildFullDistanceMatrix( FDataObj *pDataObj )
   {
   // FDataObj will look like:
   //        Reach1  Reach2   Reach3   Reach4
   // Reach1   0       d21      d31      d41
   // Reach2   d21      0       d32      d42
   // Reach3   d31      d32      0       d43
   // Reach4   d41      d42      d43      0
   //
   // reaches are ordered by their position in the m_nodeArray
   //
   // basic idea. iterate through m_nodeArray. For each reach,  1) traverse up the network,
   //   recording distances as you move up the network (note: this is recursive), then
   //   2) move down the network, recording distances as you go.


   // alternative:  start at the root of the tree.  Recursively move up the tree, computing distance up the tree as you go.

   if ( pDataObj == NULL )
      return false;

   int reachCount = GetReachCount();
   if ( reachCount == 0 )
      return false;

   // initialize dataobject
   pDataObj->SetSize( reachCount, reachCount );
   for ( int i=0; i < reachCount; i++ )
      for ( int j=0; j < reachCount; j++ )
         pDataObj->Set( i, j, -1 );

   // recurse the tree for each node, starting with each the root
   count = 0;
   for ( int i=0; i < (int) this->m_rootNodeArray.GetSize(); i++ )
      {
      ReachNode *pRoot = m_rootNodeArray[ i ];
      BuildFullDistanceMatrix( pRoot,  0, pDataObj );
      }

   /*
   // fill in remaining values
   for ( int i=0; i < reachCount; i++ )
      {
      for ( int j=0; j < reachCount; j++ )
         {
         if ( i != j )
            {
            float value = pDataObj->GetAsFloat( i, j );

            if ( value < 0.0001f )  // non-zero?
               {
               float d = pDataObj->GetAsFloat( j, i );
               //ASSERT( pDataObj->GetAsFloat( j, i ) < 0 );
               pDataObj->Set( i, j, d );
               }
            }
         else pDataObj->Set( i, j, 0 );
         }
      }  */

   return true;
   }



bool ReachTree::BuildFullDistanceMatrix( ReachNode *pThisNode, float distanceSoFar, FDataObj *pDataObj )
   {
   /*
   // get downstream distances from distanceSoFar, which represents the distance to the ReachNode (pStartNode) that
   // originally invoked this branch of calculations.  Store this distance at the appropriate place in the DataObj and
   // recurse upwards towards the leafs in the tree.
   if ( pStartNode == NULL || pThisNode == NULL )
      return true;

   if ( pThisNode->m_pDown == NULL )
      return true;

   float thisLength = pThisNode->m_length;
   float downLength = pThisNode->m_pDown->m_length;
   float lengthToAdd = ( thisLength + downLength ) / 2;

   // if these nodes aren't phantom nodes (or a root node), record a value
   int startIndex = pStartNode->index;
   int thisIndex  = pThisNode->index;

   if ( startIndex >= 0 && thisIndex >= 0 )
      {
      count++;
      if ( count % 1000 == 0 )
         {
         CString msg;
         int maxValues = (pDataObj->GetRowCount()-1)*pDataObj->GetColCount()/2;
         msg.Format( "Building distance matrix distance %i of %i", count, maxValues );
         Report::StatusMsg( msg );
         }   

      pDataObj->Set( startIndex, thisIndex, distanceSoFar+lengthToAdd );
      }

   // if the startNode is not a phantom node (or root node), keep moving up the tree.  If it is a phantom node, 
   // then there is no reason to continue with it as a starting node
   if ( pStartNode->index >= 0 )
      {
      // recurse up from the starting Node
      BuildDownstreamDistanceMatrix( pStartNode, pThisNode->m_pLeft,  distanceSoFar + lengthToAdd, pDataObj );
      BuildDownstreamDistanceMatrix( pStartNode, pThisNode->m_pRight, distanceSoFar + lengthToAdd, pDataObj );      
      }
      */
   return true;
   }





//////////////////
// Iterator

ReachTree::Iterator::Iterator( const ReachTree *pTree, int index, RT_ITERATOR_TYPE type ) 
: m_pReachTree( pTree )
, m_index( index )
, m_type( type )
, m_pCurrentNode( NULL )
   { }

ReachTree::Iterator::Iterator( const Iterator& pi )
   { 
   *this = pi; 
   }

ReachTree::Iterator& ReachTree::Iterator::operator=( const Iterator& pi )
   { 
   m_index      = pi.m_index; 
   m_pReachTree = pi.m_pReachTree;
   m_type       = pi.m_type;
   m_pCurrentNode = pi.m_pCurrentNode;

   return *this; 
   }

bool ReachTree::Iterator::operator==( const Iterator& pi ) 
   { 
   return ( m_index == pi.m_index && ((const ReachTree*)(m_pReachTree)) == pi.m_pReachTree ); 
   }


bool ReachTree::Iterator::operator!=( const Iterator& pi ) 
   { 
   return ! ( *this == pi ); 
   }


void ReachTree::Iterator::Increment() 
   {
   ASSERT( m_index < m_pReachTree->m_nodeCount );

   if ( m_type == ReachTree::UNORDERED )
      {
      m_index++;
      m_pCurrentNode = m_pReachTree->m_nodeArray[ m_index ];
      }
   else
      {
      ASSERT( 0 );
      m_index++;
      m_pCurrentNode = m_pReachTree->m_nodeArray[ m_index ];
      }
   }

void ReachTree::Iterator::Decrement() 
   {
   ASSERT( 0 < m_index );

   m_index--;
   m_pCurrentNode = m_pReachTree->m_nodeArray[ m_index ];
   }


ReachTree::Iterator ReachTree::Begin( RT_ITERATOR_TYPE type ) const
   {
   int index = 0;
   ReachTree::Iterator iterator( this, index, type );

   if ( type == ReachTree::UNORDERED )
      {
      iterator++; // will now be on the first Active row
      }

   return iterator;
   }

ReachTree::Iterator ReachTree::End( RT_ITERATOR_TYPE type ) const
   {
   ReachTree::Iterator iterator( this, m_nodeCount, type );
 
   if ( type == ReachTree::UNORDERED )
      { }

   return iterator;   
   }

//
////////////////////



int ReachTree::PopulateTreeID( int colTreeID /*=-1*/ )
   {
   if ( colTreeID < 0 )
      {
      colTreeID = m_pStreamLayer->GetFieldCol( _T("TreeID") );
      if ( colTreeID < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         colTreeID = m_pStreamLayer->m_pDbTable->AddField( _T("TreeID"), TYPE_INT, width, decimals, true );
         }
      }

   ASSERT( colTreeID >= 0 );

   m_pStreamLayer->SetColNoData( colTreeID );

   int dups = 0;

   for ( int i=0; i < m_rootNodeArray.GetCount(); i++ )
      {
      ReachNode *pNode = m_rootNodeArray[ i ];
      dups += PopulateTreeID( pNode, colTreeID, i );
      }

   return dups;
   }


int ReachTree::PopulateTreeID( ReachNode *pNode, int colTreeID, int treeIndex )
   {
   int dups=0;

   if ( pNode->m_pLeft != NULL )
      dups = PopulateTreeID( pNode->m_pLeft, colTreeID, treeIndex );

   if ( pNode->m_pRight != NULL )
      dups += PopulateTreeID( pNode->m_pRight, colTreeID, treeIndex );

   if ( ! pNode->IsPhantomNode() ) // not a phantom node?
      {
      int value;
      m_pStreamLayer->GetData( pNode->m_polyIndex, colTreeID, value );
      if ( int (m_pStreamLayer->GetNoDataValue()) == value )
         m_pStreamLayer->SetData( pNode->m_polyIndex, colTreeID, treeIndex );
      else
         dups++;
      }

   return dups;
   }




// get the number of reach nodes ABOVE the starting node
//int ReachTree::GetUpstreamReachCount( ReachNode *pStartNode, bool includePhantoms /*=false*/ )
//   {
//   if ( pStartNode == NULL )
//      return 0;
//
//   int count = 0;
//   if ( pStartNode->m_pLeft )    // is the a left branch above us?
//      {
//      // yes, so count nodes in the branch above us
//      count += GetUpstreamReachCount( pStartNode->m_pLeft, includePhantoms );
//
//      // if the left node is a phantom node and were aren't including phantoms,
//      // don't count it
//      if ( includePhantoms || pStartNode->m_pLeft->IsPhantomNode() == false )
//         count++;   // coutnt 
//      }
//
//   if ( pStartNode->m_pRight)
//      {
//      count += GetUpstreamReachCount( pStartNode->m_pRight, includePhantoms );
//
//      if ( includePhantoms || pStartNode->m_pRight->IsPhantomNode() == false )
//         count++;
//      }
//      
//   return count;
//   }

int ReachTree::GetUpstreamReachCount( ReachNode *pStartNode, bool includePhantoms /*=false*/ )
   {
   if ( pStartNode == NULL )
      return 0;

   int count = 0;
   if ( pStartNode->m_pLeft )    // is the a left branch above us?
      {
      // yes, so count nodes in the branch above us
      count += GetUpstreamReachCount( pStartNode->m_pLeft, includePhantoms );
      }

   if ( pStartNode->m_pRight)
      {
      count += GetUpstreamReachCount( pStartNode->m_pRight, includePhantoms );
      }

    if ( includePhantoms || pStartNode->IsPhantomNode() == false )
       count++;
     
   return count;
   }
