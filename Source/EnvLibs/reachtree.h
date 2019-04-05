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
// ReachTree : implements a tree structure for representing stream reach information and topology
//
//////////////////////////////////////////////////////////////////////

#if !defined( _REACHTREE_H )
#define _REACHTREE_H

#include "EnvLibs.h"

#include "Maplayer.h"
#include "PtrArray.h" 

class FDataObj;
class Query;
class ReachNode;
class SubNode;

struct OBSERVED_VALUE { int reachIndex; int subnodeIndex; float value; };

struct WITHDRAWAL { int reachIndex; int subnodeIndex; float value;  }; 
 
typedef ReachNode* (*REACHFACTORYFN)( void );
typedef SubNode* (*SUBNODEFACTORYFN)( void );

// node index values - normally the reach id
//const int ROOT_NODE    = -1;
const int PHANTOM_NODE = -2;
const int NULL_NODE    = -99;   // undefined node
 
class SubNode
{
public:
   COORD3d location;	// x,y,z location of this subnode

   //constructor
   SubNode( void ) { }

   virtual ~SubNode( void ) { }

   static SubNode *CreateInstance() { return new SubNode; }
};


//class SubnodeArray : public CArray< SubNode*, SubNode* >
//{
//public:
//   ~SubnodeArray() { for ( int i=0; i < GetSize(); i++ ) delete GetAt( i ); }
//};


// class for btree implmentation
class LIBSAPI ReachNode             // class of the node in the B tree
   {
   public:
      //-- constructor --//
      ReachNode( void ) : m_pLeft( NULL ), m_pRight( NULL ), m_pDown( NULL ), 
         m_reachID( -1 ), m_polyIndex( NULL_NODE ), m_reachIndex( -1 ), 
         m_length( 0 ), m_slope( 0 ), m_deltaX( 0 ), m_streamOrder( -1 ),
         m_pData( NULL )
         { } 

      virtual ~ReachNode() { }

      ReachNode *m_pLeft;           // pointer to left upstream node
      ReachNode *m_pRight;          // pointer to right upstream node
      ReachNode *m_pDown;           // pointer to the downstream node

      int   m_polyIndex;  // offset for this reach in the stream layer (-1 for phantom node)
      int   m_reachIndex; // offset in the reach info array
      int   m_reachID;    // grid code (unique id) for this reach, or -1

      float m_length;     // length (UNITS)???
      float m_slope;      // slope of the stream segment
      float m_deltaX;     // m

      //float reachLateralInflow;
      //float distToMouth;
      //float areaDrained;
      int    m_streamOrder;
      //float  qInit;
      //float *qArray;

      COORD3d  m_nodeLocation;

      void *m_pData;      // application-defined pointer to arbirary data

      PtrArray< SubNode> m_subnodeArray;
   
      int ComputeSubnodeLength( float maxDeltaX );     // set subnode length internally; returns number of subnodes that this represents
	   //int CreateConstantLengthSubnodeArray( float DeltaX ) ;//This one produces a constant length dx
 
      //float GetOutputFlow( void );   
      int  GetSubnodeCount() { return (int) m_subnodeArray.GetSize(); }
      bool IsPhantomNode()   { return m_polyIndex == PHANTOM_NODE; }
      bool IsRootNode()      { return m_pDown == NULL; } //m_reachInfo.index == ROOT_NODE; }
      bool IsHeadwaterNode() { return IsLeafNode(); }
      bool IsLeafNode()      { return m_pLeft == NULL && m_pRight == NULL; }
      bool MakePhantomNode() { m_polyIndex = PHANTOM_NODE; m_length = 0; return true; }
      ReachNode *GetDownstreamNode( bool skipPhantom );
      //ReachNode *GetLeftNode( bool skipPhantom );
      //ReachNode *GetRightNode( bool skipPhantom );
      
      static ReachNode *CreateInstance() { return new ReachNode; }
   };



// basic idea - Each reach is represented by a ReachNode that represents the reach segment
// BELOW the node.  An extra ReachNode representing the root of the tree is included - it has
// no corresponding line in the map coverage, and it's poly/reach index is =-1.
//
// Notes about "phantom" nodes:  A phantom node is used when three (or more) upstream reaches converage to the 
// ReachNode.  In that case the ReachNode at the confluence will be "fixed up" by:
// 1) creating a Phantom Node (with length=0), 
// 2) setting the new nodes m_reachInfo.index to PHANTOM_NODE
// 3) pointing the confluence's "right" pointer to the phantom node,
// 4) Pointing the phantom nodes "down" ptr to the confluence node
// 5) pointing the phantom nodes "right" and "left" (upstream) ptrs to the remaining
//    upstream confluences
//    |  |    |
//     \  \  /
//      \   0  <--- phantom node
//       \ /  <--- 0-length reach
//        *  <--- confluence node
//
// Notes about "root" node.  
// 1) The root node's "left" ptr should always be populated.
// 2) the root node's "right" ptr may or may not be occupied (will be if root node is a confluence)
// 3) the root node's "down" ptr is always NULL



class   LIBSAPI ReachTree
   {
   enum RT_ITERATOR_TYPE { UNORDERED=1, LEFT=2 };

   public:
      //ReachNode  *m_pRoot;           // pointer to the root of the tree
      ReachNode **m_nodeArray;         // ONLY nodes with non-phantom reach info
      int         m_nodeCount;         // number of nodes stored in tree (not including phantom nodes).
      int         m_phantomCount;      // number of phantom nodes
      int         m_unbranchedCount;   // number of single node reaches
      int         m_badToNodeCount;

      MapLayer   *m_pStreamLayer;   // associated maplayer
      int         m_colReachID;     // column with reachID info, or -1
   
      PtrArray< ReachNode > m_rootNodeArray;
      PtrArray< ReachNode > m_phantomNodeArray;

   public:
      //-- constructors --//
      ReachTree( void );

      //-- destructor --//
      ~ReachTree( void );
      
      ReachNode *GetRootNode( int i ) { if ( i >= GetRootCount() ) return NULL; return m_rootNodeArray[ i ]; }
      int GetRootCount() { return (int) m_rootNodeArray.GetSize(); }
      int GetPhantomCount() { return m_phantomCount; }
      int GetUnbranchedCount()  { return m_unbranchedCount; }

      bool IsLeaf( ReachNode *pNode ) { return ( pNode->m_pLeft == NULL && pNode->m_pRight == NULL ); }
      bool IsRoot( ReachNode *pNode ) { return ( pNode->m_pDown == NULL ); }

      ReachNode *FindLeftLeaf( ReachNode *pStartNode );

      int GetReachCount() { return m_nodeCount-1; }
      int GetSubnodeCount() { int count=0; for ( int i=0; i < m_nodeCount-1; i++ ) count += m_nodeArray[ i ]->GetSubnodeCount(); return count; }

      int GetUpstreamReachCount( ReachNode *pStartNode, bool includePhantoms=false );
      ReachNode *GetReachNodeFromReachID( int reachID, int *index=NULL );
      ReachNode *GetReachNodeFromPolyIndex( int polyIndex, int *index=NULL );
      ReachNode *GetReachNode( int index ) { return m_nodeArray[ index ]; }
      void CreateTreeDescription( CUIntArray &reachIdMap, CUIntArray &subnodeMap );

      //void DeleteNode( ReachNode *pNode );
      int RemoveNode( ReachNode *pNode, bool deleteUpstreamNodes );

      void RemoveAll( void );

      int BuildTree( MapLayer *pStreamLayer, REACHFACTORYFN pReachFactoryFn, int colReachID=-1, Query *pQuery=NULL );  // build from vertices.  specify colReachID if you want the reachID's to be set to something other than the index of the corresponding polygon in the reach file
      int BuildTree( MapLayer *pStreamLayer, REACHFACTORYFN pReachFactoryFn, int colEdgeID, int colToNode, int colReachID=-1, Query *pQuery=NULL ); // build from attributes

      Vertex &GetDownstreamVertex( Poly *pCurrentLine , bool errorCheck=false);
      Vertex &GetUpstreamVertex( Poly *pCurrentLine , bool errorCheck=false);

      void CreateSubnodes(  SUBNODEFACTORYFN pSubnodeFactoryFn, float maxReachDeltaX );
      void SetSubnodeLocation( int i, int j, float maxReachDeltaX );

      int  SaveTopology( LPCTSTR filename );
      int  PopulateOrder( int colOrder, bool calcOrder=true );
      int  CalculateOrder( ReachNode *pRoot );

      int  PopulateTreeID( int colTreeID );      
     
      //void InterpolateSubnodes( MapLayer *pCellLayer, MapLayer *pObservedLayer, MapLayer *pWithdrawalsLayer, MapLayer *pStreamLayer, float flowX );
      void InterpolateSubnodes( MapLayer *pCellLayer, MapLayer *pObservedLayer, MapLayer *pWithdrawalsLayer, MapLayer *pStreamLayer,float m_flow_adjust = 0.0f);

      bool BuildFullDistanceMatrix( FDataObj *pDataObj );  // pass in empty FDataObj, on return it will be filled.
      bool BuildDownstreamDistanceMatrix( FDataObj *pDataObj );  // pass in empty FDataObj, on return it will be filled.

   protected:
      int   InitBuild( MapLayer *pLayer, REACHFACTORYFN pFactoryFn, int colReachID, bool &slopeCheck, Query *pQuery);
      int   SaveTree(FILE *fp, ReachNode *pRoot, int level);
      int   SaveRecords(FILE *fp);
      void  InitializeSubnodes( void );
      float ComputeReachDrainageAreas( float *areaArray, MapLayer *pCellLayer, MapLayer *pStreamLayer );
      float ComputeCumulativeReachDrainageAreas( ReachNode *pNode, float *cumAreaArray, float *areaArray );
      float ComputeCumulativeReachDrainageAreas( ReachNode *pNode, float *cumAreaArray, float *areaArray, float totalArea );
      //int   DistributeObservedFlows( ReachNode *pNode, OBSERVED_VALUE *obsValueArray, int obsValueCount, 
      //                                  float *areaArray, float *cumAreaArray, float *flowArray, bool obsNode,
      //                                  WITHDRAWAL *wdArray, int wdCount, float flowX);

      int   DistributeObservedFlows( ReachNode *pNode, OBSERVED_VALUE *obsValueArray, int obsValueCount, 
                                        float *areaArray, float *cumAreaArray, float *flowArray, bool firstNode,
                                        WITHDRAWAL*wdArray, int wdCount);

      void  DistributeSubnodeFlows( ReachNode *pNode, float reachFlow, float lateralInflow );
      bool  BuildFullDistanceMatrix( ReachNode *pStartNode, float distanceSoFar, FDataObj *pDataObj );
      bool  BuildDownstreamDistanceMatrix( ReachNode *pStartNode, ReachNode *pThisNode, float distanceSoFar, FDataObj *pDataObj );
      int   PopulateTreeID( ReachNode *pNode, int colTreeID, int treeIndex );



   ///////////////  Iterator
   public:
      class Iterator;
      friend class Iterator;

      class LIBSAPI Iterator
         {
         friend class ReachTree;

         private:
            Iterator( const ReachTree *pReachTree, int index, RT_ITERATOR_TYPE flag );

         public:
            Iterator( const Iterator& pi );
         
         public:
            // make it behave like an int
            operator int() { return m_index; }

            Iterator& operator =  ( const Iterator& pi );

            bool      operator == ( const Iterator& pi );
            bool      operator != ( const Iterator& pi );

            Iterator& operator ++ ( )     { Increment(); return *this; }
            Iterator& operator ++ ( int ) { Increment(); return *this; }
            Iterator& operator -- ( )     { Decrement(); return *this; }
            Iterator& operator -- ( int ) { Decrement(); return *this; }

         private:
            void Increment();
            void Decrement();

         private:
            ReachNode *m_pCurrentNode;
            int m_index;  // keep this as the first data member and then the first 32 bits of this class is an int
                          // it will then behave correctlly even without type checking (as when passed to printf)
            RT_ITERATOR_TYPE m_type;  
            const ReachTree *m_pReachTree;
         };

      Iterator Begin( RT_ITERATOR_TYPE type=UNORDERED ) const;
      Iterator End( RT_ITERATOR_TYPE type=UNORDERED ) const;

   // End Iterator
   ////////////////////



   };


//inline
//void ReachTree::DeleteNode( ReachNode *pNode )   // note: doesn't remove from array
//   {
//   if ( pNode == NULL )
//      return;
//
//   if ( pNode->m_pLeft != NULL )
//      DeleteNode( pNode->m_pLeft );
//
//   if ( pNode->m_pRight != NULL )
//      DeleteNode( pNode->m_pRight );
//
//   delete pNode;
//   }


#endif