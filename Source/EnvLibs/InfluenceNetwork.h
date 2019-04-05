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

#include <PtrArray.h>
#include <randgen/Randunif.hpp>

#include <FDATAOBJ.H>


//---------------------------------------------------------------------------------------------------------------------------------------
// InfluenceNetwork is a directed multigraph network implementation for representing influence networks.
//
// Basic idea:  A network consist of nodes and edges.  Nodes represent entities in the system
// that operate at some level of activation.  They are connected to other node through 
// Connections. INConnection have weights between -1 and +1
//
// An INLayer is created for each landscape feedback (evaluative metric).  Each INLayer has a 
// SINGLE input, the landscape feedback score, scaled to [-1,1] and inverted (1=high scarcity, -1=high abundance) 
// It will have one output for EACH ACTOR GROUP.  These outputs represent how actively the actor group's 
// social network responds to scarcity of the given landscape feedback.
//---------------------------------------------------------------------------------------------------------------------------------------


// node types
enum NNTYPE { NNT_UNKNOWN=0, NNT_INPUT, NNT_OUTPUT, NNT_INTERIOR };

const int ACT_LOWER_BOUND =  0;
const int ACT_UPPER_BOUND =  1;


class INConnection;
class InfluenceNetwork;

// an INNode is a node in the network, representing one network participant
class LIBSAPI INNode
{
friend class InfluenceNetwork;
friend class INLayer;

public:
   // constructor
   INNode( NNTYPE nodeType );
   INNode( INNode &node ) { *this=node; }
   INNode( void ) : m_nodeType( NNT_UNKNOWN ), m_activationLevel( 0 ), 
         m_value( 0 ), m_extra( NULL ), m_x( 0 ), m_y( 0 ) { }
   
   INNode &operator = (INNode &node ) 
      { 
      m_nodeType = node.m_nodeType;
      m_extra = node.m_extra; 
      m_activationLevel = node.m_activationLevel; //
      m_value = node.m_value;
      m_x = node.m_x;
      m_y = node.m_y;

      m_inConnectionArray.Copy( node.m_inConnectionArray ); 
      m_outConnectionArray.Copy( node.m_outConnectionArray ); 
      return *this; 
      }

   ~INNode() { m_name = "_unnamed"; }

   // data
   CString  m_name;
   NNTYPE   m_nodeType;  // NNT_UNKNOWN, NNT_INPUT, NNT_OUTPUT, NNT_INTERIOR
   LONG_PTR m_extra;     // used by ANWnd to point to the corresponding NODE object

   float m_value;    // associated actor value
   float m_activationLevel;    // current activation level ([0,1])

   float m_x;  // [0,1]
   float m_y;  // [0,1]

   // connections - note that these are stored at the layer level, just ptrs here.
   // Note that we only store input connections at the node level, meaning
   // all of these connections will have their downstream ("to") ptrs = this.
   
protected:
   CArray< INConnection*, INConnection* > m_inConnectionArray;
   CArray< INConnection*, INConnection* > m_outConnectionArray;

public:
   INConnection *GetInConnection(int i) { return m_inConnectionArray[ i ]; }
   int GetInConnectionCount() { return (int) m_inConnectionArray.GetSize(); }

   INConnection *GetOutConnection(int i) { return m_outConnectionArray[ i ]; }
   int GetOutConnectionCount() { return (int) m_outConnectionArray.GetSize(); }

   int GetDegree() { return GetInConnectionCount() + GetOutConnectionCount(); }

   float GetInputSignal();
   float Activate( void );
};


typedef INNode* LPINNode;

// an INConnection is an edge in the network.  

class LIBSAPI INConnection
{
friend class INLayer;

public:
   // constructors
   INConnection( INNode *pFromNode, INNode *pToNode ) 
      : m_weight( 0 ), m_active( 0 ), m_defined( false ), m_initWeight( 0 ), 
        m_pFromNode( pFromNode ), m_pToNode( pToNode ), m_extra( NULL )
      { } 

   INConnection( INConnection &cn ) { *this = cn; }

   INConnection( void ) : m_active( 0 ), m_defined( false ), m_weight( 0 ), m_pFromNode( NULL ), m_pToNode( NULL ) { }

   INConnection &operator = ( INConnection &cn ) 
      { 
      m_weight     = cn.m_weight; 
      m_initWeight = cn.m_initWeight;
      m_active     = cn.m_active;
      m_defined    = cn.m_defined;
      m_pFromNode  = cn.m_pFromNode; 
      m_pToNode    = cn.m_pToNode; 
      return *this; 
      }

   bool IsFrom( INNode *pNode ) { return pNode == m_pFromNode; }
   bool IsTo  ( INNode *pNode ) { return pNode == m_pToNode; }

   float m_weight;         // [-1,1]
   float m_initWeight;
   LONG_PTR m_extra;       // application-specific value

   bool m_active;          // if this node active
   bool m_defined;         // defined in the input file

   INNode *m_pFromNode;
   INNode *m_pToNode;
};


// A layer is a collection of INNodes and INConnections
// An INlayer has input nodes, interior nodes, and output nodes.
// input node representing inputs into the network, typically
// a landscape signal.  Input connections to interior nodes
// are one way, from the input node to interior node.
// The Layer inputs are propagated through the layer through
// interior nodes, each of which represents a participant in the network.
// A layer can have [0,n] output nodes, where each output node represents
// an entity with a particular topological connectivity with the network
// participants.  Output connection are one way, from the interior nodes
// to the output nodes.
  

class LIBSAPI INLayer
{
friend class InfluenceNetwork;

public:
   INLayer( InfluenceNetwork *pNet ); 

   INLayer( INLayer &sl );
   ~INLayer();

   // parent container
   InfluenceNetwork *m_pInfluenceNetwork;

   // nodes and connections
   PtrArray< INNode > m_inputNodeArray;      // input nodes - signals to be propagated through the network
   PtrArray< INNode > m_interiorNodeArray;   // network participants 
   PtrArray< INNode > m_outputNodeArray;     // listeners detecting network output

   PtrArray< INConnection > m_connectionArray;  // a list of all the connections in the layer
        
   CMap< LPCTSTR, LPCTSTR, INNode*, INNode* > m_nodeMap;   // dictionary for looking up Nodes from their names
   
   // layer properties
   static float m_weightThreshold;   // abs value below which a weight is considered 0
   static float m_afSlopeScalar;     // 1-10, determines slope of acivation function (default=6)
   static float m_afTranslate;       // translates the activation function

   // node management
   INNode *AddNode( NNTYPE nodeType, LPCTSTR name );
   INNode *GetInputNode   (int i) { return m_inputNodeArray[ i ]; }
   INNode *GetInteriorNode(int i) { return m_interiorNodeArray[ i ]; }
   INNode *GetOutputNode  (int i) { return m_outputNodeArray[ i ]; }

   int GetInputNodeCount   ( void ) { return (int) m_inputNodeArray.GetSize(); }
   int GetInteriorNodeCount( void ) { return (int) m_interiorNodeArray.GetSize(); }
   int GetOutputNodeCount  ( void ) { return (int) m_outputNodeArray.GetSize(); }

   INNode *FindNode( LPCTSTR name, NNTYPE type=NNT_UNKNOWN );

   void SetAllInteriorNodes( float activation );
   void SetAllInputNodes( float activation );

   // connection management
   INConnection *AddConnection( INNode *pFromNode, INNode *pToNode, float weight );
   INConnection *GetConnection(int i) { return m_connectionArray[ i ]; }
   int GetConnectionCount() { return (int) m_connectionArray.GetSize(); }
   void SetAllConnections( float weight );

   // Data access
   FDataObj *GetLayerData( void ) { return &m_layerData; }

   // Network metrics for this layer
   float GetEdgeDensity( NNTYPE type=NNT_UNKNOWN );   //number of edges/max possible number of edges
   float GetEdgeWeightDensity( NNTYPE nodeType=NNT_UNKNOWN);
   float GetDegreeCentrality( INNode *pNode );
   int   GetDegreeDistribution( CArray<float,float> &distArray );
   
   int  Activate( void );
   void GetOutputConnections(bool active, CArray< INConnection*, INConnection* > &out);
 //  bool NodeConnectionCount( INNode *pNode, int nodeConnectionType );
   
   CString m_name;
   bool    m_use;
   int     m_activationIterations;
   
protected:
   // methods
   void ShuffleNodes( INNode **nodeArray, int nodeCount );

   static RandUniform m_randShuffle;

   FDataObj m_layerData;
};


class LIBSAPI InfluenceNetwork
{
friend class INLayer;

public:
   InfluenceNetwork() : m_deltaTolerance( 0.0001f ), m_maxIterations( 100 ) { }
   InfluenceNetwork( InfluenceNetwork &sn );

   ~InfluenceNetwork( void ) { }
   
   bool Init   ( LPCTSTR filename );
   bool InitRun( void );
   bool Step ( int cycle );
   //bool Setup  ( EnvContext *pContext, HWND hWnd );

   int      AddRandomLayer( int inputCount, int interiorCount, int outputCount, float linkDensity /*0-1*/ );
   int      AddFullyConnectedLayer(  int inputCount, int interiorCount, int outputCount );

   INLayer *AddLayer( void ) { if ( m_layerArray.Add( new INLayer( this ) ) >= 0 ) return m_layerArray[ m_layerArray.GetUpperBound() ]; else return NULL; }
   bool     RemoveLayer( LPCTSTR name );
   void     RemoveAllLayers() { m_layerArray.RemoveAll(); }

   INLayer *GetLayer( int i ) { return m_layerArray[ i ]; }
   int      GetLayerCount( void ) { return (int) m_layerArray.GetSize(); }

   // metrics (these need to be implemented!!!)
   int   GetMetricCount( void ) { return 1; }
   bool  GetMetricLabel( int i, CString &label );
   float GetMetricValue( int layer, int i );

   static float m_shapeParam;   // abs value below which a weight is considered 0


protected:
   bool LoadXml( LPCTSTR filename );
   //void SetInputActivations( void );
   void ActivateLayers( void );
   float Scale( float lb, float ub, float value );
   void SetValueVectors(void);

   // network layers - 
   PtrArray< INLayer > m_layerArray;

   int   m_maxIterations;        // maximum iterations until activations set
   float m_deltaTolerance;       // convergence criteria for activation
};


inline
float InfluenceNetwork::Scale( float lb, float ub, float value )
   {
   float range0 = ub - lb ;
   float range1 = ACT_UPPER_BOUND - ACT_LOWER_BOUND;

   value = ACT_LOWER_BOUND + (value - lb ) * range1/range0;
   return value;
   }

