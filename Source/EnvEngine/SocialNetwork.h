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
#include <TMATRIX.HPP>
#include <randgen/Randunif.hpp>
#include <vector>
#include <map>
#include <string>
// vector -> carray; map -> cmap; to change from stl to mfc

using namespace std;

typedef map<CString, int> countTable;
typedef countTable::iterator countTableItr;

class EnvModel;

//---------------------------------------------------------------------------------------------------------------------------------------
// SocialNetwork is a directed multigraph network implementation for representing social networks.
//
// Basic idea:  A network consist of nodes and edges.  Nodes represent entities in the system
// that operate at some level of activation.  They are connected to other node through 
// Edges. SNEdge have weights between -1 and +1
//
// An SNLayer is created for each landscape feedback (evaluative metric).  Each SNLayer will have as a 
// SINGLE input the landscape feedback score, scaled to [-1,1] and inverted (1=high scarcity, -1=high abundance) 
// It will have one output for EACH ACTOR GROUP.  These outputs represent how actively the actor group's 
// social network responds to scarcity of the given landscape feedback.
//---------------------------------------------------------------------------------------------------------------------------------------


// node types

//const int ACT_LOWER_BOUND = -1;
//const int ACT_UPPER_BOUND =  1;





// constants
enum SNIP_INPUT_TYPE {
   I_CONSTANT = 1,
   I_CONSTWSTOP = 2,
   I_SINESOIDAL = 3,
   I_RANDOM = 4,
   I_TRACKOUTPUT = 5
   };


const int SEB_TRANSEFF = 0;
const int SEB_INFLUENCE = 1;
const int SNB_DEGREE = 0;
const int SNB_REACTIVITY = 1;

enum SNIP_STATE {
   STATE_INACTIVE = 0,
   STATE_ACTIVATING = 1,  // edges onl
   STATE_ACTIVE = 2,
   STATE_DELETED = 3
   };

enum SNIP_INFMODEL_TYPE {
   IM_SENDER_RECEIVER = 0,
   IM_SIGNAL_RECEIVER = 1,
   IM_SIGNAL_SENDER_RECEIVER = 2
   };

// node types
enum SNIP_NODETYPE {
   NT_UNKNOWN = -1,
   NT_INPUT_SIGNAL = 0,
   NT_NETWORK_ACTOR = 1,
   NT_LANDSCAPE_ACTOR = 2
   };

enum SNIP_EDGE_TYPE {
   ET_INPUT = 0,
   ET_NETWORK = 1
   };

// cytoscape selectors
// edge type selectors
//const INPUT_EDGE = '[type=0]';
//const NETWORK_EDGE = '[type=1]';
//
//// nod selectors
//const INPUT_SIGNAL = '[type=0]';
//const NETWORK_ACTOR = '[type=1]';
//const LANDSCAPE_ACTOR = '[type=2]';
//const ANY_ACTOR = '[type=1],[type=2]';
//
//// node, edge "state" selectors
//const INACTIVE = '[state=0]';
//const ACTIVATING = '[state=1]';
//const ACTIVE = '[state=2]';


// Node Status
// On LOAD    | Input  | Network  | LA       |
// state      | Active | Inactive | Inactive |
// reactivity |  1.0   | 0.0      | 0.0      |

// During RUN | Input  | Network  | LA       |
// state      | Active | Both     | Both     |
// reactivity |  1.0   | calculated          |

//const int DEBUG = 0;




class SNEdge;
class SocialNetwork;


class TraitContainer
{
   public:
      CArray<float> m_traitArray;

      void AddTraitsFrom(TraitContainer* pSource) {
         int traitCount = (int) pSource->m_traitArray.GetSize();
         m_traitArray.SetSize(traitCount);
         for (int i = 0; i <  traitCount; i++)
            this->m_traitArray[i] = pSource->m_traitArray[i];
         }

      void ClearTraits() { m_traitArray.RemoveAll(); }
};



// social network node class
class SNNode : public TraitContainer
{
friend class SocialNetwork;
friend class SNLayer;

public:
   // data
   CString  m_name;
   SNIP_NODETYPE  m_nodeType;

   CArray< SNEdge*, SNEdge* > m_edgeArray;

   SNLayer* m_pLayer;   // snipModel: this,
   int m_index;         // : this.nextNodeIndex,
   std::string m_id;    // : 'n' + this.nextNodeIndex,
   //label : '',
   SNIP_STATE m_state;  // : STATE_ACTIVE,
   float m_reactivity;  // : reactivity,
   float m_sumInfs;     // : 0,
   float m_srTotal;     // : 0,
   float m_influence;   // : 0,
   float m_reactivityMultiplier; // : 1,
   float m_reactivityHistory;    // : [reactivity] ,
   float m_reactivityMovAvg;     // : 0,

public:
   // constructor
   SNNode(SNIP_NODETYPE nodeType );
   SNNode( SNNode &node ) { *this=node; }
   SNNode( void ) : m_nodeType( NT_UNKNOWN ), m_reactivity( 0 ) { }
   
   ~SNNode() { }
   SNNode &operator = (SNNode &node ) 
      { 
      m_nodeType = node.m_nodeType;
      //m_extra = node.m_extra; 
      m_reactivity = node.m_reactivity;
      m_edgeArray.Copy( node.m_edgeArray ); 
      return *this; 
      }



//   CArray < SNEdge*, SNEdge* > DecideActive();

protected:
	//float m_landscapeValue;
public:
   // edges - note that these are stored at the layer level, just ptrs here
   //CArray<SNNode*, SNNode*> m_inEdges;
   //CArray<SNNode*, SNNode*> m_outEdges;

   //float getLandscapeMetric(){return m_landscapeValue;}
   //void setLandscapeMetric(float value){m_landscapeValue = value;}
   //bool CalculateLandscapeValue(SNNode* node);
   //float Activate( void );
};


typedef SNNode* LPSNNode;


class SNEdge : public TraitContainer
{
friend class SNLayer;

public:
   SNNode* m_pFromNode;
   SNNode* m_pToNode;

   std::string m_name;

   //bool m_active;


   std::string m_id;  // : 'e' + this.nextEdgeIndex,   // FromIndex_ToIndex
   float m_signalStrength; // : 0,   // [-1,1]
   float m_signalTraits;   // : signalTraits,  // traits vector for current signal, or null if inactive
   float m_transEff;       // : transEff,
   float m_transEffSender; // : 0,
   float m_transEffSignal; // : 0,
   float m_transTime;      // : transTime,
   int m_activeCycles;     // : 0,
   SNIP_STATE m_state;     // : STATE_ACTIVE,
   float m_influence;      // : 0,
   SNIP_EDGE_TYPE m_edgeType;

public:
   // constructors
   SNEdge( SNNode *pFromNode, SNNode *pToNode ) 
      : m_pFromNode( pFromNode )
      , m_pToNode( pToNode )
      , m_transTime(-1)
      { } 

   SNEdge( SNEdge &cn ) { *this = cn; }

   SNEdge( void ) : m_pFromNode(NULL), m_pToNode(NULL), m_transTime(-1) { }

   SNEdge &operator = ( SNEdge &cn ) 
      { 
      //m_weight    = cn.m_weight; 
      m_pFromNode = cn.m_pFromNode; 
      m_pToNode   = cn.m_pToNode; 
      m_transTime = cn.m_transTime;
      return *this;
      }

   bool IsFrom( SNNode *pNode ) { return pNode == m_pFromNode; }
   bool IsTo  ( SNNode *pNode ) { return pNode == m_pToNode; }

   SNNode* Source() { return m_pFromNode; }
   SNNode* Target() { return m_pToNode; }

};


class SNLayer
{
friend class SocialNetwork;

public:
   std::string m_name;
   std::string m_description;

   bool    m_use;
   int     m_activationIterations;
   int     m_outputNodeCount;

   CStringArray m_traitsLabels;

   // SNIP parameters
   int m_cycles;
   int m_currentCycle;
   int m_adaptive;  // 0=non-adaptive; 1=adaptive
   int m_convergeIterations = 0;

   // autogen parameters
   int m_autogenTransTimeMax;
   std::string m_autogenTransTimeBias;

   // transmission efficiency submodel
   SNIP_INFMODEL_TYPE m_infSubmodel;
   float m_infSubmodelWt; // = 0.5;   // 0 =  signal only; 1=sender only
   float m_transEffMax;
   //float m_transEffAlpha;
   //float m_transEffBetaTraits0;

   float m_aggInputSigma;
   float m_activationSteepFactB;
   float m_activationThreshold;
   float m_kD;

   // temps for building network
   int m_nextNodeIndex;
   int m_nextEdgeIndex;

protected:
   // container
   SocialNetwork* m_pSocialNetwork;
   EnvEvaluator* m_pModel;

   // nodes and edges
   SNNode* m_pInputNode;
   PtrArray< SNNode > m_nodeArray;
   PtrArray< SNEdge > m_edgeArray;

   CMap< LPCTSTR, LPCTSTR, SNNode*, SNNode* > m_nodeMap;

   // SNIP Methods
   void SetInputNodeReactivity(float value) { this->m_pInputNode->m_reactivity = value; }
   void SetEdgeTransitTimes();
   void GenerateInputArray();
   void ResetNetwork();
   void InitSimulation();
   void RunCycle(int cycle, bool repeatUntilDone);
   float PropagateSignal(int cycle);
   void ComputeEdgeTransEffs();   // NOTE: Only need to do active edges


   float SolveEqNetwork(int) { return 0; }

   // TODO!!!!
   float GetInputLevel(int cycle) { return 1; }
   float GetOutputLevel() { return 1; }

   // not implemented...
   void AddAutogenInputEdges();
   void AddAutogenInputEdge(std::string id, SNNode* pSourceNode, SNNode* pTargetNode, float sourceTraits[], float actorValue);






   // output variables (actor groups X values )
   //TMatrix< float > m_outputArray;



   //float m_actorValue;     // used for landscape signal transmission efficiency

   
   // autogen params
   ////float m_autoGenFraction;
   ////std::string m_autoGenBias;
   ////
   ////int m_maxAutoGenTransTime;
   ////std::string m_autoGenTransTimeBias;
   ////
   ////// input
   ////std::string inputType;
   ////float inputValue;

   // model parameters






   // methods
   void ShuffleNodes(SNNode** nodeArray, int nodeCount);
   void LSReact();

   static RandUniform m_randShuffle;

   countTable m_countTable;		//
   countTable m_inCountTable;	//
   countTable m_outCountTable;	//
   countTable m_trustTable;		//
   map< CString, map<CString, float> > m_AdjacencyMatrix;			//
   //double m_degreeCentrality;

  /*
   // network objects/stores
   // Note that the edge information is stored in an adjacency matrix
   // node information is stored in a nodeInfos array, one per node
   cy = null;       // cytoscape network
   networkInfo = null;   // dictionary with description, traits, etc read from JSON file

   // input definition
   inputNode = null;
   inputType = "";   // "constant", "constant_with_stop","sinusoidal","random","track_output"
   inputValue = 1;   ///???

   // input signal parameters
   k = 0;        // #inputConstant
   k1 = 0;       // #inputConstant1
   slope = 0.5;  //?
   stop = 0;     // #inputConstantStop
   amp = 0;      // #inputAmp
   period = 0;   // #inputPeriod
   phase = 0;    // #inputPhaseShift
   lagPeriod = 0;

   // network function parameters
   inputSeriesType = 1;
   kD = 0;
   kF = 0;

   // network generation parameters
   randGenerator = null;
   autogenFraction = 0;
   autogenBias = "degree";
   autogenTransTimeMax = 0;
   autogenTransTimeBias = "none";
   nextNodeIndex = 0;
   nextEdgeIndex = 0;
   closeToZero = 0.001;  // edge weights below this value are considered zero

   // network learning parameters
   maxEdgeWtDelta = 0.1;
   learningExpA = 8;

   // network view parameters
   nodeSize = "reactivity";
   nodeColor = "influence";
   nodeLabel = "name";
   edgeSize = "transEff";
   edgeColor = "influence";
   edgeLabel = "none";
   //showTips = false; global
   showLandscapeSignalEdges = true;
   layoutName = "cola";
   layout = layoutCola;
   zoom = 1;
   center = { x: 0, y : 0 };
   //"simulation_period" = 10
   //zoomed = false;

   // event handlers
   //OnLoad = null;
   OnWatchMsg = null;

   // data created/collected
   xs = [];             // generated data for plotting
   netInputs = [];      // and setting input, one per cycle
   netOutputs = []; //Array(cycles).fill(1.0),
   netStats = {};
   netReport = [];   // array of netStats, one per 
   watchReport = [];

   maxNodeDegree = 1;
   maxNodeInfluence = 1;

   // frequency distribution stats
   degreeDistributions = [];
   reactivityDistributions = [];
   influenceDistributions = [];
   transEffDistributions = [];
   signalStrengthDistributions = []
   */


public:
   SNLayer( SocialNetwork *pNet ) : m_pSocialNetwork( pNet ), m_use( true ), m_pModel(NULL),
      m_activationIterations( 0 ), m_pInputNode( NULL ), m_outputNodeCount( 0 ) { }

   SNLayer( SNLayer &sl );
   ~SNLayer();

   bool LoadSnipNetwork(LPCTSTR path);


   SNNode *BuildNode( SNIP_NODETYPE nodeType, LPCTSTR name, float *traits);
   SNNode *BuildNode( SNIP_NODETYPE nodeType, LPCTSTR name);
   SNEdge  *BuildEdge( SNNode *pFromNode, SNNode *pToNode, int transTime, float *traits=NULL );
   SNNode *FindNode( LPCTSTR name );
   SNNode *GetNode( int i )         { return m_nodeArray[ i ]; }
   SNNode *GetInputNode( void )     { return m_pInputNode; }  
   int GetNodeCount( void )         { return (int) m_nodeArray.GetSize(); }
   int GetNodeCount( SNIP_NODETYPE type )  { int count=0; for ( int i=0; i < GetNodeCount(); i++ ) if ( m_nodeArray[ i ]->m_nodeType == type ) count++; return count; }
   int GetOutputNodeCount( void ) { return m_outputNodeCount; }

   bool SetInCountTable();	//
   bool SetOutCountTable();//
   bool SetCountTable();	//
   bool SetTrustTable();	//
   bool SetAdjacencyMatrix();	//
   void GetInteriorNodes(CArray< SNNode*, SNNode* > &out);		//
   void GetInteriorNodes(bool active, CArray< SNNode*, SNNode* > & out);
   void GetInteriorEdges(CArray< SNEdge*, SNEdge* > &out);
   void GetInteriorEdges(bool active, CArray< SNEdge*, SNEdge* > &out);
   bool GetDegreeCentrality();
   SNEdge *GetEdge( int i ) { return m_edgeArray[ i ]; }
   int GetEdgeCount( void ) { return (int) m_edgeArray.GetSize(); }
   void ActivateNodes(CArray < SNEdge*, SNEdge* >);


   bool BuildNetwork( void );
   int  Activate( void );
   void GetContactedNodes(const CArray< SNEdge*, SNEdge* >& ActiveEdges, CArray <SNNode*>& ContactedNodes);
   double SNWeight_Density();	//
   double SNDensity();			//
   void Bonding(SNNode*);
   void GetOutputEdges(bool active, CArray< SNEdge*, SNEdge* > &out);
 //  bool NodeEdgeCount( SNNode *pNode, int nodeEdgeType );
   


};


class SocialNetwork
{
friend class SNLayer;

public:
   SocialNetwork(EnvModel *pModel) : m_pEnvModel(pModel), m_deltaTolerance( 0.0001f ), m_maxIterations( 100 ) { }
   SocialNetwork( SocialNetwork &sn );

   ~SocialNetwork( void ) { }
   
   EnvModel *m_pEnvModel;

   bool Init   ( LPCTSTR filename );
   bool InitRun( void );
   bool Run    ( void );
   //bool Setup  ( EnvContext *pContext, HWND hWnd );
   
   SNLayer *AddLayer( void ) { if ( m_layerArray.Add( new SNLayer( this ) ) >= 0 ) return m_layerArray[ m_layerArray.GetUpperBound() ]; else return NULL; }
   bool     RemoveLayer( LPCTSTR name );
   SNLayer *GetLayer( int i ) { return m_layerArray[ i ]; }
   int      GetLayerCount( void ) { return (int) m_layerArray.GetSize(); }
   //map<CString, AgNode> m_agMap;


   // metrics (these need to be implemented!!!)
   int   GetMetricCount( void ) { return 1; }
   bool  GetMetricLabel( int i, CString &label );
   float GetMetricValue( int layer, int i );

protected:
   bool LoadXml( LPCTSTR filename );
   void SetInputActivations( void );
   void ActivateLayers( void );
   float Scale( float lb, float up, float value );
   void SetValueVectors(void);
   PtrArray< SNLayer > m_layerArray;

   //for testing
   vector < double > test_values;

   int   m_maxIterations;        // maximum iterations until activations set
   float m_deltaTolerance;       // convergence criteria for activation
};

/*

inline
float SocialNetwork::Scale( float lb, float ub, float value )
   {
   float range0 = ub - lb ;
   float range1 = ACT_UPPER_BOUND - ACT_LOWER_BOUND;

   value = ACT_LOWER_BOUND + (value - lb ) * range1/range0;
   return value;
   }
   */
//class AgNode
//{
//
//public:
//	AgNode();
//	CString m_name;
//	vector < SNNode* > m_nodeInstances; 
//	bool AddInstance(SNNode*);
//	void ClearInstances();
//	vector < double > GetValues();
//
//};
