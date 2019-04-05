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

#include "EnvModel.h"
#include <PtrArray.h>
#include <TMATRIX.HPP>
#include <randgen/Randunif.hpp>
#include <vector>
#include <map>
// vector -> carray; map -> cmap; to change from stl to mfc

using namespace std;
typedef map<CString, int> countTable;
typedef countTable::iterator countTableItr;

//---------------------------------------------------------------------------------------------------------------------------------------
// SocialNetwork is a directed multigraph network implementation for representing social networks.
//
// Basic idea:  A network consist of nodes and edges.  Nodes represent entities in the system
// that operate at some level of activation.  They are connected to other node through 
// Connections. SNConnection have weights between -1 and +1
//
// An SNLayer is created for each landscape feedback (evaluative metric).  Each SNLayer will have as a 
// SINGLE input the landscape feedback score, scaled to [-1,1] and inverted (1=high scarcity, -1=high abundance) 
// It will have one output for EACH ACTOR GROUP.  These outputs represent how actively the actor group's 
// social network responds to scarcity of the given landscape feedback.
//---------------------------------------------------------------------------------------------------------------------------------------


// node types
enum NNTYPE { NNT_UNKNOWN=0, NNT_INPUT, NNT_OUTPUT, NNT_INTERIOR };

const int ACT_LOWER_BOUND = -1;
const int ACT_UPPER_BOUND =  1;


class SNConnection;
class SocialNetwork;
class AgNode;

class SNNode
{
friend class SocialNetwork;
friend class SNLayer;

public:
   // constructor
   SNNode( NNTYPE nodeType );
   SNNode( SNNode &node ) { *this=node; }
   SNNode( void ) : m_nodeType( NNT_UNKNOWN ), m_activationLevel( 0 ), m_extra( NULL ), m_AgParent( NULL ) { }
   
   ~SNNode()
   {
	   m_name = "gotcha";
   }
   SNNode &operator = (SNNode &node ) 
      { 
      m_nodeType = node.m_nodeType;
      m_extra = node.m_extra; 
      m_activationLevel = node.m_activationLevel; //
      m_connectionArray.Copy( node.m_connectionArray ); 
      return *this; 
      }

   // data
   CString  m_name;
   NNTYPE   m_nodeType;
   LONG_PTR m_extra;       // used by ANWnd to point to the corresponding NODE object
   INT_PTR m_noConnections;
   AgNode* m_AgParent;
//  INT_PTR m_inConnections;
// INT_PTR m_outConnections;
   float m_landscapeGoal;  //an orgs desired landscape value 
   double m_reactivity;		//a measure of how tolerant an org is concerning deviation from the actual landscape value and its desired landscape value: how quickly it will react.
   float m_activationLevel;    //
   double m_trust;	//
   float LandscapeActivate( void );   //determine if node becomes active
   void GetTrust();
   double GetInputWeight();
   bool m_active;
  
//   CArray < SNConnection*, SNConnection* > DecideActive();

protected:
	float m_landscapeValue;
public:
   // connections - note that these are stored at the layer level, just ptrs here
   CArray< SNConnection*, SNConnection* > m_connectionArray;
   float getLandscapeMetric(){return m_landscapeValue;}
   void setLandscapeMetric(float value){m_landscapeValue = value;}
   bool CalculateLandscapeValue(SNNode* node);
   float Activate( void );
};


typedef SNNode* LPSNNode;


class SNConnection
{
friend class SNLayer;

public:
   // constructors
   SNConnection( SNNode *pFromNode, SNNode *pToNode ) 
      : m_weight( 0 ), m_active( 0 ), m_initWeight( 0 ), 
        m_pFromNode( pFromNode ),
        m_pToNode( pToNode ),
        m_extra( NULL )
      { } 

   SNConnection( SNConnection &cn ) { *this = cn; }

   SNConnection( void ) : m_active( 0 ), m_weight( 0 ), m_pFromNode( NULL ), m_pToNode( NULL ) { }

   SNConnection &operator = ( SNConnection &cn ) 
      { 
      m_weight    = cn.m_weight; 
      m_pFromNode = cn.m_pFromNode; 
      m_pToNode   = cn.m_pToNode; 
	  m_active    = cn.m_active;
      return *this; 
      }

   bool IsFrom( SNNode *pNode ) { return pNode == m_pFromNode; }
   bool IsTo  ( SNNode *pNode ) { return pNode == m_pToNode; }

   float m_weight;   // -1 - +1
   float m_initWeight;
   LONG_PTR m_extra;       // used by ANWnd to store index in weight dataobject

   bool m_active;
   SNNode *m_pFromNode;
   SNNode *m_pToNode;
};


class SNLayer
{
friend class SocialNetwork;

public:
   SNLayer( SocialNetwork *pNet ) : m_pSocialNetwork( pNet ), m_use( true ), m_pModel( NULL ), 
      m_activationIterations( 0 ), m_pInputNode( NULL ), m_outputNodeCount( 0 ) { }

   SNLayer( SNLayer &sl );
   ~SNLayer();

   SNNode *AddNode( NNTYPE nodeType, LPCTSTR name, float value );
   SNNode *AddNode( NNTYPE nodeType, LPCTSTR name);
   SNConnection  *AddConnection( SNNode *pFromNode, SNNode *pToNode, float weight );
   SNConnection *AddConnection(SNNode *pFromNode, SNNode *pToNode);
   SNNode *FindNode( LPCTSTR name );
   SNNode *GetNode( int i )         { return m_nodeArray[ i ]; }
   SNNode *GetInputNode( void )     { return m_pInputNode; }  
   int GetNodeCount( void )         { return (int) m_nodeArray.GetSize(); }
   int GetNodeCount( NNTYPE type )  { int count=0; for ( int i=0; i < GetNodeCount(); i++ ) if ( m_nodeArray[ i ]->m_nodeType == type ) count++; return count; }
   int GetOutputNodeCount( void ) { return m_outputNodeCount; }
   PtrArray< SNNode > m_interiorNodes;

   bool SetInCountTable();	//
   bool SetOutCountTable();//
   bool SetCountTable();	//
   bool SetTrustTable();	//
   bool SetAdjacencyMatrix();	//
   void GetInteriorNodes(CArray< SNNode*, SNNode* > &out);		//
   void GetInteriorNodes(bool active, CArray< SNNode*, SNNode* > & out);
   void GetInteriorConnections(CArray< SNConnection*, SNConnection* > &out);
   void GetInteriorConnections(bool active, CArray< SNConnection*, SNConnection* > &out);
   bool GetDegreeCentrality();
   SNConnection *GetConnection( int i ) { return m_connectionArray[ i ]; }
   int GetConnectionCount( void ) { return (int) m_connectionArray.GetSize(); }
   void ActivateNodes(CArray < SNConnection*, SNConnection* >);

   countTable m_countTable;		//
   countTable m_inCountTable;	//
   countTable m_outCountTable;	//
   countTable m_trustTable;		//
   map< CString, map<CString, float> > m_AdjacencyMatrix;			//
   double m_degreeCentrality;
   PtrArray< SNConnection > m_interiorConnections;

   bool BuildNetwork( void );
   int  Activate( void );
   void GetContactedNodes(const CArray< SNConnection*, SNConnection* >& ActiveEdges, CArray <SNNode*>& ContactedNodes);
   double SNWeight_Density();	//
   double SNDensity();			//
   void Bonding(SNNode*);
   void GetOutputConnections(bool active, CArray< SNConnection*, SNConnection* > &out);
 //  bool NodeConnectionCount( SNNode *pNode, int nodeConnectionType );
   

   CString m_name;
   bool    m_use;
   EnvEvaluator *m_pModel;
   int     m_activationIterations;
   int     m_outputNodeCount;


protected:
   // container
   SocialNetwork *m_pSocialNetwork;

   // nodes and connections
   //CArray< SNNode, SNNode& > m_nodeArray;
   //CArray< SNConnection, SNConnection& > m_connectionArray;

   SNNode *m_pInputNode;
   PtrArray< SNNode > m_nodeArray;
   PtrArray< SNConnection > m_connectionArray;
        
   CMap< LPCTSTR, LPCTSTR, SNNode*, SNNode* > m_nodeMap;
   
   // output variables (actor groups X values )
   TMatrix< float > m_outputArray;
   
   // methods
   void ShuffleNodes( SNNode **nodeArray, int nodeCount );
   void LSReact(  );

   static RandUniform m_randShuffle;
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
   map<CString, AgNode> m_agMap;


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


inline
float SocialNetwork::Scale( float lb, float ub, float value )
   {
   float range0 = ub - lb ;
   float range1 = ACT_UPPER_BOUND - ACT_LOWER_BOUND;

   value = ACT_LOWER_BOUND + (value - lb ) * range1/range0;
   return value;
   }

class AgNode
{

public:
	AgNode();
	CString m_name;
	vector < SNNode* > m_nodeInstances; 
	bool AddInstance(SNNode*);
	void ClearInstances();
	vector < double > GetValues();

};
