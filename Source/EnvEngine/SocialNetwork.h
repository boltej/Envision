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

#include <json.h>

using json = nlohmann::json;

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
   I_UNKNOWN = 0,
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
   IM_SIGNAL_SENDER_RECEIVER = 2,
   IM_TRUST = 3
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

struct NetStats {
   // simulation stats
   int cycle;
   int convergeIterations;
   // basic network info
   int nodeCount;
   int edgeCount;
   int nodeCountNLA;
   float minNodeReactivity;
   float meanNodeReactivity;
   float maxNodeReactivity;
   float minNodeInfluence;
   float meanNodeInfluence;
   float maxNodeInfluence;
   float minEdgeTransEff;
   float meanEdgeTransEff;
   float maxEdgeTransEff;
   float minEdgeInfluence;
   float meanEdgeInfluence;
   float maxEdgeInfluenc;
   float edgeDensity;
   float totalEdgeSignalStrength;
   // input/output
   float inputActivation;
   float landscapeSignalInfluence;
   float outputActivation;
   float meanLANodeReactivity;
   float maxLANodeReactivity;
   float signalContestedness;
   // network
   float meanDegreeCentrality;
   float meanBetweenessCentrality;
   float meanClosenessCentrality;
   float meanInfWtDegreeCentrality;
   float meanInfWtBetweenessCentrality;
   float meanInfWtClosenessCentrality;
   float normCoordinators;
   float normGatekeeper;
   float normRepresentative;
   float notLeaders;
   float clusteringCoefficient;
   float NLA_Influence;
   float NLA_TransEff;
   float normNLA_Influence;
   float normNLA_TransEff;
   float normEdgeSignalStrength;

   NetStats(void) { Init(); }

   void Init(void) {
      cycle = -1;
      convergeIterations = 0;
      nodeCount = 0;
      edgeCount = 0;
      nodeCountNLA = 0;
      minNodeReactivity = 1.0f;
      meanNodeReactivity = 0.0f;
      maxNodeReactivity = 0.0f;
      minNodeInfluence = 1.0f;
      meanNodeInfluence = 0.0f;
      maxNodeInfluence = 0.0f;
      minEdgeTransEff = 1.0f;
      meanEdgeTransEff = 0.0f;
      maxEdgeTransEff = 0.0f;
      minEdgeInfluence = 1.0f;
      meanEdgeInfluence = 0.0f;
      maxEdgeInfluenc = 0.0f;
      edgeDensity = 0.0f;
      totalEdgeSignalStrength = 0.0f;
      inputActivation = 0.0f;
      landscapeSignalInfluence = 0.0f;
      outputActivation = 0.0f;
      meanLANodeReactivity = 0.0f;
      maxLANodeReactivity = 0.0f;
      signalContestedness = 0.0f;
      meanDegreeCentrality = 0.0f;
      meanBetweenessCentrality = 0.0f;
      meanClosenessCentrality = 0.0f;
      meanInfWtDegreeCentrality = 0.0f;
      meanInfWtBetweenessCentrality = 0.0f;
      meanInfWtClosenessCentrality = 0.0f;
      normCoordinators = 0.0f;
      normGatekeeper = 0.0f;
      normRepresentative = 0.0f;
      notLeaders = 0.0f;
      clusteringCoefficient = 0.0f;
      NLA_Influence = 0.0f;
      NLA_TransEff = 0.0f;
      normNLA_Influence = 0.0f;
      normNLA_TransEff = 0.0f;
      normEdgeSignalStrength = 0.0f;
      }
   };


class SNEdge;
class SNNode;
class SNLayer;
class SNIPModel;
class SocialNetwork;





class TraitContainer
{
   public:
      CArray<float> m_traits;

      void AddTraitsFrom(TraitContainer* pSource) {
         int traitCount = (int) pSource->m_traits.GetSize();
         m_traits.SetSize(traitCount);
         for (int i = 0; i <  traitCount; i++)
            this->m_traits[i] = pSource->m_traits[i];
         }

      void ClearTraits() { m_traits.RemoveAll(); }
      float ComputeSimilarity(TraitContainer*);
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
      m_inEdges.Copy( node.m_inEdges ); 
      m_outEdges.Copy(node.m_outEdges);
      return *this;
      }

//   CArray < SNEdge*, SNEdge* > DecideActive();

protected:
	//float m_landscapeValue;
public:
   // edges - note that these are stored at the layer level, just ptrs here
   CArray<SNEdge*, SNEdge*> m_inEdges;
   CArray<SNEdge*, SNEdge*> m_outEdges;
   //std::vector<std::shared_ptr<SNEdge>>;

   bool IsActive() { return m_state == SNIP_STATE::STATE_ACTIVE; }
   bool IsActivating() { return m_state == SNIP_STATE::STATE_ACTIVATING; }

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
   float m_trust;          // only used in Trust subbmodel
   float m_transEffSender; // : 0,
   float m_transEffSignal; // : 0,
   int m_transTime;      // : transTime,
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

   bool IsActive() { return m_state == SNIP_STATE::STATE_ACTIVE; }
   bool IsActivating() { return m_state == SNIP_STATE::STATE_ACTIVATING; }
};


class SNLayer
{
friend class SocialNetwork;
friend class SNIPModel;
friend class EnvModel;

public:
   std::string m_name;
   std::string m_description;

   int   m_outputNodeCount = 0;

   CStringArray m_traitsLabels;

protected:
   // container
   SocialNetwork* m_pSocialNetwork;
   //EnvEvaluator* m_pModel;

   SNIPModel *m_pSNIPModel = NULL;
   //unique_ptr<SNIPModel> m_pSNIPModel;

   // nodes and edges
   //SNNode* m_pInputNode;
   PtrArray< SNNode > m_nodes;
   PtrArray< SNEdge > m_edges;

   CMap< LPCTSTR, LPCTSTR, SNNode*, SNNode* > m_nodeMap;

   int m_nextNodeIndex = 0;
   int m_nextEdgeIndex = 0;

   // methods
   void ShuffleNodes(SNNode** nodeArray, int nodeCount);
   //void LSReact();

   countTable m_countTable;		//
   countTable m_inCountTable;	//
   countTable m_outCountTable;	//
   countTable m_trustTable;		//
   map< CString, map<CString, float> > m_AdjacencyMatrix;			//
   //double m_degreeCentrality;

 
   // network objects/stores
   // Note that the edge information is stored in an adjacency matrix
   // node information is stored in a nodeInfos array, one per node
   


public:
   SNLayer(SocialNetwork* pNet);
   SNLayer( SNLayer &sl );
   ~SNLayer();

   bool Init();

   SNNode *FindNode( LPCTSTR name );
   SNNode *GetNode( int i )         { return m_nodes[ i ]; }
   //SNNode *GetInputNode( void )     { return m_pInputNode; }  
   int GetNodeCount( void )         { return (int) m_nodes.GetSize(); }
   int GetNodeCount( SNIP_NODETYPE type )  { int count=0; for ( int i=0; i < GetNodeCount(); i++ ) if ( m_nodes[ i ]->m_nodeType == type ) count++; return count; }
   int GetOutputNodeCount( void ) { return m_outputNodeCount; }

   //bool SetInCountTable();	//
   //bool SetOutCountTable();//
   //bool SetCountTable();	//
   //bool SetTrustTable();	//
   //bool SetAdjacencyMatrix();	//
   void GetInteriorNodes(CArray< SNNode*, SNNode* > &out);		//
   void GetInteriorNodes(bool active, CArray< SNNode*, SNNode* > & out);
   void GetInteriorEdges(CArray< SNEdge*, SNEdge* > &out);
   void GetInteriorEdges(bool active, CArray< SNEdge*, SNEdge* > &out);
   //bool GetDegreeCentrality();
   SNEdge *GetEdge( int i ) { return m_edges[ i ]; }
   int GetEdgeCount( void ) { return (int) m_edges.GetSize(); }
   //void ActivateNodes(CArray < SNEdge*, SNEdge* >);
   //void UpdateNetworkStats(int cycle);


   int  Activate( void );
   //void GetContactedNodes(const CArray< SNEdge*, SNEdge* >& ActiveEdges, CArray <SNNode*>& ContactedNodes);
   double SNWeight_Density();	//
   double SNDensity();			//
   //void Bonding(SNNode*);
   void GetOutputEdges(bool active, CArray< SNEdge*, SNEdge* > &out);
 //  bool NodeEdgeCount( SNNode *pNode, int nodeEdgeType );
   

};




class SNIPModel
   {
   friend class SNLayer;

   protected:
      bool  m_use = true;
      SNLayer* m_pSNLayer = NULL;       // associated network layer
      json m_networkJSON; // / networkInfo = null;   // dictionary with description, traits, etc read from JSON file

      std::string m_name;
      std::string m_description;

      // input definition
      SNNode* m_pInputNode = NULL;
      SNIP_INPUT_TYPE m_inputType = I_UNKNOWN;   // "constant", "constant_with_stop","sinusoidal","random","track_output"
      //inputValue = 1;   ///???

      // input signal parameters
      float m_k = 0;        // inputConstant
      float m_k1 = 0;       // inputConstant1
      float m_slope = 0.5;  // 
      float m_stop = 0;     // #inputConstantStop
      float m_amp = 0;      // #inputAmp
      float m_period = 0;   // #inputPeriod
      float m_phase = 0;    // #inputPhaseShift
      float m_lagPeriod = 0;

      // network function parameters
      float m_activationThreshold = 0;
      float m_activationSteepFactB = 1;
      float m_kD = 0;   // signal decay constant
      float m_kF = 0;   // reactivity decay factor
      float m_aggInputSigma = 3;
      float m_transEffMax = 1.2f;
      float m_transEffAlpha = 0;
      float m_transEffBetaTraits0 = 1.0f;
      float m_infSubmodelWt = 0.5f;
      SNIP_INFMODEL_TYPE m_infSubmodel = SNIP_INFMODEL_TYPE::IM_TRUST;

      // runtime variables
      int m_cycles = -1;
      int m_currentCycle = -1;

      // network function parameters
      //SERIES_TYPE m_inputSeriesType;
 
      CArray<std::string, std::string&> m_traitsLabels;

      static RandUniform m_randShuffle;


      // network generation parameters
      RandUniform* m_pRandGenerator = NULL;
      float m_autogenFraction = 0;
      CString autogenBias = "degree";
      int m_autogenTransTimeMax = 0;
      std::string m_autogenTransTimeBias = "none";
      float m_closeToZero = 0.001f;  // edge weights below this value are considered zero

      // network learning parameters
      //m_maxEdgeWtDelta = 0.1;
      //m_learningExpA = 8;

      // network view parameters
      //nodeSize = "reactivity";
      //nodeColor = "influence";
      //nodeLabel = "name";
      //edgeSize = "transEff";
      //edgeColor = "influence";
      //edgeLabel = "none";
      ////showTips = false; global
      //showLandscapeSignalEdges = true;
      //layoutName = "cola";
      //layout = layoutCola;
      //zoom = 1;
      //center = { x: 0, y : 0 };
      //"simulation_period" = 10
      //zoomed = false;

      // event handlers
      //OnLoad = null;
      //OnWatchMsg = null;

      // data created/collected
      vector <float> m_xs;             // generated data for plotting
      vector<float> m_netInputs;      // and setting input, one per cycle
      NetStats m_netStats;
      FDataObj* m_pNetData;
      //m_netReport = [];   // array of netStats, one per 
      //m_watchReport = [];

      int   m_activationIterations = 0;
      int   m_maxIterations = 100;        // maximum iterations until activations set
      int   m_convergeIterations = 0;
      float m_deltaTolerance = 0.0001f;       // convergence criteria for activation

      int m_maxNodeDegree = 1;
      float m_maxNodeInfluence = 1;

      // frequency distribution stats
      //degreeDistributions = [];
      //reactivityDistributions = [];
      //influenceDistributions = [];
      //transEffDistributions = [];
      //signalStrengthDistributions = []

   public:
      SNIPModel(SNLayer* pLayer) : m_pSNLayer(pLayer) { Init(); }
      ~SNIPModel(void);

      // SNIP Methods
      bool Init(void);

      bool LoadSnipNetwork(LPCTSTR path);
      //bool BuildNetwork( void );
      SNNode* BuildNode(SNIP_NODETYPE nodeType, LPCTSTR name, float* traits);
      SNNode* BuildNode(SNIP_NODETYPE nodeType, LPCTSTR name);
      SNEdge* BuildEdge(SNNode* pFromNode, SNNode* pToNode, int transTime, float* traits = NULL);

      bool LoadPreBuildSettings(json& settings);
      bool LoadPostBuildSettings(json& settings);

      SNNode* GetInputNode(void) { return m_pInputNode; }
      void SetInputNodeReactivity(float value) { this->m_pInputNode->m_reactivity = value; }
      void SetEdgeTransitTimes();
      void GenerateInputArray();
      void ResetNetwork();

      int RunSimulation(bool initialize, bool step);

      void InitSimulation();
      void RunCycle(int cycle, bool repeatUntilDone);
      float PropagateSignal(int cycle);
      void ComputeEdgeTransEffs();   // NOTE: Only need to do active edges
      void ComputeEdgeInfluences();
      float ActivateNode(SNNode* pNode, float bias);
      float ProcessLandscapeSignal(float inputLevel, CArray<float, float>& landscapeTraits, CArray<float, float>& nodeTraits);
      float AggregateInputFn(float sumInputSignals);
      float ActivationFn(float inputSignal, float reactivityMovAvg);
      float GetOutputLevel();
      float GetInputLevel(int cycle);  // note: cycle is ZERO based
      float SolveEqNetwork(float bias);

      // not implemented...
      void AddAutogenInputEdges();
      void AddAutogenInputEdge(std::string id, SNNode* pSourceNode, SNNode* pTargetNode, float sourceTraits[], float actorValue);

      void UpdateNetworkStats();

      // convenience functions
      int GetEdgeCount() { return m_pSNLayer->GetEdgeCount(); }
      int GetNodeCount() { return m_pSNLayer->GetNodeCount(); }
      SNEdge* GetEdge(int i) { return m_pSNLayer->GetEdge(i); }
      SNNode* GetNode(int i) { return m_pSNLayer->GetNode(i); }

      // stats
      int GetMaxDegree(bool) {
         int count = GetNodeCount(); int maxDegree = 0;
         for (int i = 0; i < count; i++) {
            int edges = int(GetNode(i)->m_inEdges.GetSize() + GetNode(i)->m_outEdges.GetSize());
            if (edges > maxDegree)
               maxDegree = edges;
            }
         return maxDegree;
         }


      int GetActiveNodeCount() {
         int count = GetNodeCount();
         int activeCount = 0;
         for (int i = 0; i < count; i++)
            if (m_pSNLayer->GetNode(i)->m_state == SNIP_STATE::STATE_ACTIVE)
               activeCount++;
         return activeCount;
         }

   };




class SocialNetwork
{
friend class SNLayer;

public:
   SocialNetwork(EnvModel *pModel) : m_pEnvModel(pModel) {}
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
   int   GetMetricCount( void ) { return m_metricCount; }
   bool  GetMetricLabel( int i, CString &label );
   float GetMetricValue( int layer, int i );

protected:
   bool LoadXml( LPCTSTR filename );
   //void SetInputActivations( void );
   //void ActivateLayers( void );
   //float Scale( float lb, float up, float value );
   //void SetValueVectors(void);
   
   
   int m_metricCount = 0;
   PtrArray< SNLayer > m_layerArray;
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
