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
#include "stdafx.h"
#pragma hdrstop

#include "SocialNetwork.h"
#include "Actor.h"
#include <math.h>

using namespace std;

//extern ActorManager *gpActorManager;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//to do:  SNDensity is now in the Init function - should probably call from the getMetricValue fx.
RandUniform SNLayer::m_randShuffle;
RandUniform randNodeInit;

SNNode::SNNode( NNTYPE nodeType )
: m_nodeType( nodeType )
, m_extra( NULL )
, m_activationLevel( 0 )
, m_noConnections( -1 )
   {
   m_activationLevel = (float) randNodeInit.RandValue( ACT_LOWER_BOUND, ACT_UPPER_BOUND );   
   }

void SNNode::GetTrust()//get total trust for a single organization
{
   double out = 0;
   for (int i = 0; i < m_connectionArray.GetSize(); i++)
   {
      SNConnection *connection = m_connectionArray[i];
      if (connection->m_pFromNode->m_nodeType == NNT_OUTPUT)
      {
         out += connection->m_pFromNode->m_trust;
      }
   m_trust = out;
   }
}

//////////////////////////////////////////////////////////////////////////////
//    S O C I A L    N E T W O R K
//////////////////////////////////////////////////////////////////////////////

//   SocialNetwork() : m_deltaTolerance( 0.0001f ), m_maxIterations( 100 ) { }
SocialNetwork::SocialNetwork( SocialNetwork &sn )
   : m_deltaTolerance( sn.m_deltaTolerance )
   , m_maxIterations( sn.m_maxIterations )
   , m_layerArray( sn.m_layerArray )
   {
   for ( int i=0; i < (int) m_layerArray.GetSize(); i++ )
      m_layerArray[ i ]->m_pSocialNetwork = this;
   }




bool SocialNetwork::Init( LPCTSTR filename )
   {
   // build network connections base in initStr.
   // Note that:
   //    1) Input nodes are required for landscape feedbacks and any external drivers
   //    2) Output nodes are required for each actor's values (one per actor/value pair)
   //    3) Interior nodes represent the social network

   bool ok = LoadXml( filename ); //, pEnvContext );  // defines basic nodes/network structure, connects inputs and outputs
   double SNDensity;

   for ( int i=0; i < GetLayerCount(); i++ )
      {
      SNLayer *pLayer = GetLayer( i );
      SNDensity = pLayer->SNDensity();
      if ( pLayer->m_use == false )
         continue;

      for (int j=0; j < pLayer->GetNodeCount(); j++ )
         {
         SNNode *pNode = pLayer->GetNode( j );

         LPTSTR type = _T("Input");
         if ( pNode->m_nodeType == NNT_INTERIOR )
            {
            type = _T("Interior");
            m_agMap[pNode->m_name].AddInstance(pNode);
            }
         else if ( pNode->m_nodeType == NNT_OUTPUT )
            type = _T( "Output" );

         CString label;
         label.Format( _T("%s:%s (%s)"), (LPCTSTR) pLayer->m_name, (LPCTSTR) pNode->m_name, type );

         //AddOutputVar( label, pNode->m_activationLevel, "" );
         }

      CString label;
      label.Format( _T("%s:Activation Iterations)"), (LPCTSTR) pLayer->m_name );
      //AddOutputVar( label, pLayer->m_activationIterations, "" );
      }

   // assign name of interior nodes to AgNode using keys in m_agMap
   for (auto itr = m_agMap.begin(); itr != m_agMap.end(); itr++)
      {
      (*itr).second.m_name = (*itr).first;
      }
   vector < double > test_values = (*m_agMap.begin()).second.GetValues();
   return TRUE; 
   }


bool SocialNetwork::InitRun( void )
   {
   // reset all weights to their initial states
   for ( int i=0; i < this->GetLayerCount(); i++ )
      {
      SNLayer *pLayer = GetLayer( i );

      int connectionCount = pLayer->GetConnectionCount();
      for ( int j=0; j < connectionCount; j++ )
         {
         SNConnection *pConnection = pLayer->GetConnection( j );
         pConnection->m_weight = pConnection->m_initWeight;
         }
      }

   SetInputActivations();
   ActivateLayers();

   return TRUE; 
   }


bool SocialNetwork::Run( void )
   {
   SetInputActivations();
   ActivateLayers();
   return TRUE; 
   }

////////////////////  A G G R E G A T E   O B J E C T  //////////////////////////////
//used for storing organizations' values across all layers
AgNode::AgNode()
{
   m_name = "";
}

bool AgNode::AddInstance(SNNode* instance)
{
   instance->m_AgParent = this;
   m_nodeInstances.push_back(instance);
   return true;
}

void AgNode::ClearInstances()
{
   for (int i = 0; i < m_nodeInstances.size(); i++)
   {
      m_nodeInstances[i]->m_AgParent = NULL;
   }
   m_nodeInstances.clear();
}

vector < double > AgNode::GetValues()
{
   vector < double > values;
   values.resize(m_nodeInstances.size(), 0.0);
   for (int i = 0; i < m_nodeInstances.size(); i++)
   {
      values[i] = m_nodeInstances[i]->GetInputWeight();
   }
   return values;
}



/////////////////////////// S N L A Y E R //////////////////////////////////////////

SNLayer::SNLayer( SNLayer &sl )
   : m_name( sl.m_name )
   , m_use( sl.m_use )
   , m_pModel( sl.m_pModel )
   , m_activationIterations( sl.m_activationIterations )
   , m_outputNodeCount( sl.m_outputNodeCount )
   , m_pSocialNetwork( NULL )  // NOTE - MUST BE SET AFTER CONSTRUCTION;
   , m_pInputNode( NULL )      // set below...
   , m_nodeArray( sl.m_nodeArray )
   , m_connectionArray( sl.m_connectionArray )
   , m_outputArray( sl.m_outputArray )
   {
   // find input node
   for ( int i=0; i < (int) m_nodeArray.GetSize(); i++ )
      {
      if ( m_nodeArray[ i ]->m_nodeType == NNT_INPUT )  // one and only
         {
         m_pInputNode = m_nodeArray[ i ];
         break;
         }
      }

   ASSERT( m_pInputNode != NULL );

   // next, build map
   //CMap< LPCTSTR, LPCTSTR, SNNode*, SNNode* > m_nodeMap;
   // TODO!!!!
   GetInteriorNodes(m_interiorNodes);
   for (int i = 0; i < m_interiorNodes.GetCount(); i++)
   {
      CString orgFrom = m_interiorNodes[i]->m_name;
      for (int j = 0; j < m_interiorNodes.GetCount(); j++)
      {
         CString orgTo = m_interiorNodes[j]->m_name;
         m_AdjacencyMatrix[orgTo][orgFrom] = 0.0;
      }
   }
}

SNLayer::~SNLayer()
{
   
}

SNNode *SNLayer::AddNode( NNTYPE nodeType, LPCTSTR name, float landscapeGoal )
   {
   SNNode *pNode = new SNNode( nodeType );
   int index = (int) m_nodeArray.Add( pNode ); //SNNode( nodeType ) );

   pNode->m_name = name;
   pNode->m_landscapeGoal= landscapeGoal;
   if ( nodeType == NNT_INPUT )
      m_pInputNode = pNode;

   else if ( nodeType == NNT_OUTPUT )
      m_outputNodeCount++;

   return pNode;
   }


SNConnection *SNLayer::AddConnection( SNNode *pFromNode, SNNode *pToNode, float weight )
   {
   ASSERT( pFromNode != NULL && pToNode != NULL );

   SNConnection *pConnection = new SNConnection( pFromNode, pToNode );
   
   int index = (int) m_connectionArray.Add( pConnection );

   //SNConnection *pConnection = &(m_connectionArray[ index ]);
   pConnection->m_weight = pConnection->m_initWeight = weight;

   pFromNode->m_connectionArray.Add( pConnection );
   pToNode->m_connectionArray.Add( pConnection );

   return pConnection;
   }

SNConnection *SNLayer::AddConnection( SNNode *pFromNode, SNNode *pToNode)
   {
   ASSERT( pFromNode != NULL && pToNode != NULL );

   SNConnection *pConnection = new SNConnection( pFromNode, pToNode );
   
   int index = (int) m_connectionArray.Add( pConnection );

   //SNConnection *pConnection = &(m_connectionArray[ index ]);
//   pConnection->m_weight = pConnection->m_initWeight = weight;

   pFromNode->m_connectionArray.Add( pConnection );
   pToNode->m_connectionArray.Add( pConnection );

   return pConnection;
   }
SNNode *SNLayer::FindNode( LPCTSTR name )
   {
   SNNode *pNode = NULL;

   int nodeCount = (int) m_nodeArray.GetSize();
   for ( int i=0; i < nodeCount; i++ )
      {
      if ( m_nodeArray[ i ]->m_name.CompareNoCase( name ) == 0 )
         return m_nodeArray[ i ];
      }

   //bool ok = m_nodeMap.Lookup( name, pNode );
   //if ( ok )
   //   return pNode;
   //else
      return NULL;
   }



bool SNLayer::BuildNetwork( void )
   {
   // we assume that all nodes have been added, and this method connects all the nodes with Connections
   m_connectionArray.RemoveAll();
   int nodeCount = (int) m_nodeArray.GetSize();

   for ( int i=0; i < nodeCount; i++ )
      {
      for ( int j=0; j < nodeCount; j++ )
         {
         if ( i != j )     // no self connections
            {
            AddConnection( m_nodeArray[ i ], m_nodeArray[ j ] );   // this creates a connection and ::Korejwa - this was commented out as AddConnection( &m_nodeArray[ i ], ... but would not compile.  changed to m_nodeArray.
            }
         }
      }

   return true;
   }


//int SNLayer::Activate( void )


void SNLayer::GetInteriorNodes(CArray< SNNode*, SNNode* > & out) //get all interior nodes (ie organizations)
{
   for (int i = 0; i < m_nodeArray.GetSize(); i++)
   {
      SNNode *node = m_nodeArray[i];
      if (node->m_nodeType == NNT_INTERIOR)
      {
         out.Add(node);
      }
   }
}

void SNLayer::GetInteriorNodes(bool active, CArray< SNNode*, SNNode* > & out)   //get organizations that are 'active' or inactive
{
   for (int i = 0; i < m_nodeArray.GetSize(); i++)
   {
      SNNode *node = m_nodeArray[i];
      if(active==0)
      {
         if (node->m_nodeType == NNT_INTERIOR && node->m_active==0)
         {
            out.Add(node);
         }
      }
      else if(active == 1)
      {
         if (node->m_nodeType == NNT_INTERIOR && node->m_active==1)
         {
            out.Add(node);
         }
      }
   }
}
void SNLayer::GetInteriorConnections(CArray< SNConnection*, SNConnection* > &out) //get interior connections
{
   for (int i = 0; i < m_connectionArray.GetSize(); i++)
   {
      SNConnection *connection = m_connectionArray[i];
      if (connection->m_pFromNode->m_nodeType == NNT_INTERIOR && connection->m_pToNode->m_nodeType == NNT_INTERIOR)
      {
         out.Add(connection);
      }
   }
}

void SNLayer::GetInteriorConnections(bool active, CArray< SNConnection*, SNConnection* > &out) //get active/inactive connections
{
	for (int i = 0; i < m_connectionArray.GetSize(); i++)
	{
		SNConnection *connection = m_connectionArray[i];
		if (connection->m_pFromNode->m_nodeType == NNT_INTERIOR && connection->m_pToNode->m_nodeType == NNT_INTERIOR && connection->m_active==1)
		{
			out.Add(connection);
		}
		else if (connection->m_pFromNode->m_nodeType == NNT_INTERIOR && connection->m_pToNode->m_nodeType == NNT_INTERIOR && connection->m_active==0)
		{
			out.Add(connection);
		}
	}
}

void SNLayer::GetOutputConnections(bool active, CArray< SNConnection*, SNConnection* > &out) //get active connections from orgs to actor groups
{
	for (int i = 0; i < m_connectionArray.GetSize(); i++)
	{
		SNConnection *connection = m_connectionArray[i];
		if (connection->m_pFromNode->m_nodeType == NNT_INTERIOR && connection->m_pToNode->m_nodeType == NNT_OUTPUT && connection->m_active==1)
		{
			out.Add(connection);
		}
	}
}


double SNLayer::SNDensity()//number of edges/max possible number of edges
{
   CArray< SNNode*, SNNode* > interiorNodes;
   GetInteriorNodes(interiorNodes);
   CArray < SNConnection*, SNConnection* > interiorConnections;
   GetInteriorConnections(interiorConnections);
   double ret = -1;

   int nodeCount = (int) interiorNodes.GetSize();
   if (nodeCount > 1)
   {
      double max_connections = (nodeCount) * (nodeCount-1);
      ret = ((double)interiorConnections.GetSize()) / max_connections;  
   }
   return ret;
}  

double SNLayer::SNWeight_Density() //sum of weights/max possible sum of weights
{
   double ret = -1;
   CArray< SNNode*, SNNode* > interiorNodes;
   GetInteriorNodes(interiorNodes);
   CArray < SNConnection*, SNConnection* > interiorConnections;
   GetInteriorConnections(interiorConnections);
   int nodeCount = (int) interiorNodes.GetSize();
   if (nodeCount > 1)
   {
      float sum_weights = 0;
      for (int i = 0; i < interiorConnections.GetSize(); i ++)
      {
         SNNode *pNode = interiorConnections[i]->m_pFromNode;
         sum_weights += interiorConnections[i]->m_weight;
      }
      int max_connections = (nodeCount) * (nodeCount - 1);
      ret = sum_weights / max_connections;
   }
   return ret;
}


bool SNLayer::SetInCountTable()//in degree for each organization
{
   m_inCountTable.clear();
   CArray < SNConnection*, SNConnection* > interiorConnections;
   GetInteriorConnections(interiorConnections);
   for (int i = 0; i < interiorConnections.GetSize(); i++)
   {
      SNConnection *pConnection = interiorConnections[i];
      if (m_inCountTable.find(pConnection->m_pToNode->m_name) == m_inCountTable.end())
      {
         m_inCountTable[pConnection->m_pToNode->m_name] = 0;
      }
      m_inCountTable[pConnection->m_pToNode->m_name]++;
   }
   return false;
}

bool SNLayer::SetOutCountTable()//out degree for each organization
{
   m_outCountTable.clear();
   CArray < SNConnection*, SNConnection* > interiorConnections;
   GetInteriorConnections(interiorConnections);
   for (int i = 0; i < interiorConnections.GetSize(); i++)
   {
      SNConnection *pConnection = interiorConnections[i];
      if (m_outCountTable.find(pConnection->m_pFromNode->m_name) == m_outCountTable.end())
      {
         m_outCountTable[pConnection->m_pFromNode->m_name] = 0;
      }
      m_outCountTable[pConnection->m_pFromNode->m_name]++;
   }
   return false;
}

void SNLayer::LSReact()  //get input node (landscape signal) and compare it to the threshold of each node it connects to.  
{
   double threshold = 0.5;
   CArray < SNConnection*, SNConnection* > ActiveEdges;
   SNNode* InputNode = GetInputNode();
	float LandscapeScore = InputNode->m_activationLevel;

	for (int i = 0; i < InputNode->m_noConnections; i++)
	{	
		SNConnection* temp = InputNode->m_connectionArray[i];
		if(temp->m_pToNode->m_nodeType == NNT_INTERIOR)		//if connection is to an organization
		{
			if (fabs(temp->m_pToNode->m_landscapeGoal - LandscapeScore)*temp->m_weight > threshold)   //if (distance between the orgs goal and the landscape signal)*reactivity of org/importance of metric to the org > threshold
			{
				temp->m_pToNode->m_active = 1;
				for(int j = 0; j < temp->m_pToNode->m_noConnections; j++)
				{
					if((temp->m_pToNode->m_connectionArray[j]->m_pToNode != temp->m_pToNode) && (temp->m_pToNode->m_connectionArray[j]->m_pToNode->m_nodeType == NNT_INTERIOR)) //if the connection is to another org (should also gather connections to actor groups? could also just keep track of active nodes and gather these connex later)
					{
						ActiveEdges.Add(temp->m_pToNode->m_connectionArray[j]);		//then add this connection to the ActiveEdges
						temp->m_pToNode->m_connectionArray[j]->m_active = 1;
					}
				}
			}
		}
	}
	//create unique list of contacted nodes
	CArray <SNNode*> ContactedNodes;
	GetContactedNodes(ActiveEdges, ContactedNodes);
	CArray < SNConnection*, SNConnection* > newConnections;
	//for each contacted node, decide activate? reciprocate? do nothing? - if activate, all its edges become active...put this in a list and repeat.  If reciprocate, strengthen both edges between these orgs.  If do nothing, weaken edge weight from org with active edge
	//
	do
	{
		newConnections.RemoveAll();
		for (int i = 0; i < ContactedNodes.GetCount(); i++)
		{
//			 newConnections.Append(ContactedNodes[i]->DecideActive());
		}
		GetContactedNodes(newConnections, ContactedNodes);
	}while (!newConnections.IsEmpty());
}

void SNLayer::GetContactedNodes(const CArray< SNConnection*, SNConnection* >& ActiveEdges, CArray <SNNode*>& ContactedNodes)
{
   ContactedNodes.RemoveAll();
   for (int i = 0; i < ActiveEdges.GetCount(); i++)
   {
      SNNode* temp = ActiveEdges[i]->m_pToNode;
      if(!temp->m_active)
      {
         int j;
         for (j = 0; j < ContactedNodes.GetCount(); j++)
         {
            if (temp == ContactedNodes[j])
               break;
         }
         if (j >= ContactedNodes.GetCount())
         {
            ContactedNodes.Add(temp);
         }
      }      
   }
}



/* Each org probabilistically decides to add/delete edge depending on trust and trust in neighborhood (and change in trust for last few time steps?) This is meant to capture bridging capital

for each organization
   calculate trust(time_t) (and trust(time_t) for each actor group)
      compare with trust(time_t-1)
   calculate agg trust for this layer 
   calculate change in utility  and decide to make change(s) - track changes: 'bridging connection', new bonding connection, deleted edge with similar/dissimilar org


parts to utility function:
   value similarity
   degree (similarity but also for 'reputation'/'popularity'/'network influence')
   agg trust in neighborhood (for now - just neighbors)
   transitivity


*/

bool SNLayer::GetDegreeCentrality()
{//for each interior node, iterate through m_inCountTable and m_outCountTable and add the connections to find the number of connections the org has that are between itself and another org.   
   int max = 0;

   CArray< SNNode*, SNNode* > interiorNodes;
   GetInteriorNodes(interiorNodes);
   for (int i = 0; i < interiorNodes.GetSize(); i++)
   {
      int count = 0;
      count += m_inCountTable[interiorNodes[i]->m_name];
      count += m_outCountTable[interiorNodes[i]->m_name];
      if (count > max) { max = count; }
   }
   m_degreeCentrality = max;
   int diffs = 0;
   for (int i = 0; i < interiorNodes.GetSize(); i++)
   {//degree centrality is the sum of the differences between the greatest node degree and the rest of the nodes degrees divided by the theoretical maximum = (N-1)*(N-2) = star network.
      int count = max;
      count -=  m_inCountTable[interiorNodes[i]->m_name];
      count -= m_outCountTable[interiorNodes[i]->m_name];
      diffs += count;
   }
   m_degreeCentrality = (double) diffs / ((double)(interiorNodes.GetSize()-1) * (interiorNodes.GetSize() - 2));
   return false;
}


void SNLayer::ActivateNodes(CArray < SNConnection*, SNConnection* > ActiveConnections)
{
   //for each node, consider all incoming active edges - degree of from nodes, trust of from nodes, value similarity of from nodes.  Can ignore, reciprocate, or choose to become active.
   CArray < SNNode*, SNNode*> InactiveNodes;
   GetInteriorNodes(0, InactiveNodes);
   //for each of these inactiveNodes, collect active edges and then decide to ignore, reciprocate, or activate
   for(int i = 0; i < InactiveNodes.GetCount(); i++)
   {
      SNNode* tempNode;
      tempNode = InactiveNodes[i];
   }
}



/*

bool SNLayer::SetTrustTable()
{
   m_trustTable.clear();
   for (int i = 0; i < m_connectionArray.GetSize(); i++)
   {
      SNConnection *pConnection = m_connectionArray[i];
      if (m_trustTable.find(pConnection->m_pToNode->m_name) == m_trustTable.end())
      {
         m_outCountTable[pConnection->m_pToNode->m_name] = 0;
      }

   }
}

*/

/*to do - return arbitrary list of nodes
for (int i=0; int< max; i++)
{
   list<SNNode *> subArray;
   SNNode *pNode = nodeArray[i];
   apply test - if it works
      subArray.push_back(pNode);
}
return subArray;
*/

//to do: distribution of links.  distribution of weights.  average weights.  trust measure - per organization, per community, and for the entire layer.  clustering method for identifying communities and cliques.  
//probability of forming a new link for orgs and for actor groups.  {do we want to have multiple links for each actor group (each actor group value measure)?}



void SNLayer::ShuffleNodes( SNNode **nodeArray, int nodeCount )
   {
   //--- Shuffle elements by randomly exchanging each with one other.
   for ( int i=0; i < nodeCount-1; i++ )
      {
      int randVal = (int) m_randShuffle.RandValue( 0, nodeCount-i-0.0001f );

      int r = i + randVal; // Random remaining position.
      SNNode *temp = nodeArray[ i ];
      nodeArray[ i ] = nodeArray[ r ];
      nodeArray[ r ] = temp;
      }
   }


bool SocialNetwork::LoadXml( LPCTSTR filename )
   {
   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   if ( ! ok )
      {      
      Report::ErrorMsg( doc.ErrorDesc() );
      return false;
      }
 
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // assoc_net
   
   // top level is layers
   TiXmlElement *pXmlLayers = pXmlRoot->FirstChildElement( "layers" );
   if ( pXmlLayers == NULL )
      {
      CString msg;
      msg.Format( _T("Error reading SocialNetwork input file %s: Missing <layers> entry.  This is required to continue..."), filename );
      Report::ErrorMsg( msg );
      return false;
      }

   TiXmlElement *pXmlLayer = pXmlLayers->FirstChildElement( "layer" );
   if ( pXmlLayer == NULL )
      {
      CString msg;
      msg.Format( _T("Error reading SocialNetwork input file %s: Missing <layer> entry.  This is required to continue..."), filename );
      Report::ErrorMsg( msg );
      return false;
      }

   while ( pXmlLayer != NULL )
      {
      LPTSTR name = NULL;
      XML_ATTR attrs[] = { // attr          type        address    isReq checkCol
                         { "name",      TYPE_STRING,   &name,     true,  0 },
                         { NULL,        TYPE_NULL,     NULL,      false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlLayer, attrs, filename );

      if ( ok )
         {
         // have layer info, allocate layer and start reading
         SNLayer *pLayer = this->AddLayer();
         if ( name != NULL )
            pLayer->m_name = name;

         // next read node info
         TiXmlElement *pXmlNodes = pXmlLayer->FirstChildElement( "nodes" );
         if ( pXmlNodes == NULL )
            {
            CString msg;
            msg.Format( _T("Error reading SocialNetwork input file %s: Missing <nodes> entry.  This is required to continue..."), filename );
            Report::ErrorMsg( msg );
            this->RemoveLayer( name );
            goto nextLayer;
            }

         // add input node for this layer - only one (corresponds to eval model output)
		   SNNode *pInputNode = pLayer->AddNode(NNT_INPUT, pLayer->m_name, 0);//the last argument here should be 'landscape goal'
         pLayer->m_pModel = m_pEnvModel->FindEvaluatorInfo( pLayer->m_name );
         if ( pLayer->m_pModel == NULL )
            {
            CString msg;
            msg.Format( "SocialNetwork Error: the layer name '%s' does not correspond to any Evaluative Model.  This layer will be ignored", (LPCTSTR) pLayer->m_name );
            Report::ErrorMsg( msg );
            pLayer->m_use = false;
            }

         // add output nodes for this layer - one for each actor group
         int actorGroupCount = (int) m_pEnvModel->m_pActorManager->GetActorGroupCount();
         for ( int i=0; i < actorGroupCount; i++ )
            pLayer->AddNode( NNT_OUTPUT, m_pEnvModel->m_pActorManager->GetActorGroup( i )->m_name, 5.0 );//third argument should be 'landscape goal'

         // start adding interior nodes based on definitions in XML
         TiXmlElement *pXmlNode = pXmlNodes->FirstChildElement( "node" );
         while ( pXmlNode != NULL )
            {
            LPCTSTR name = NULL;
			float goal = NULL;
            XML_ATTR attrs[] = { // attr   type          address    isReq checkCol
                               { "name",   TYPE_STRING,  &name,     true,  0 },
							          { "goal", TYPE_FLOAT,    &goal,    true,  0 },
                               { NULL,     TYPE_NULL,    NULL,      false, 0 } };

            bool ok = TiXmlGetAttributes( pXmlNode, attrs, filename );
            if ( ok )
               pLayer->AddNode( NNT_INTERIOR, name, goal );
               
            pXmlNode = pXmlNode->NextSiblingElement( "node" );
            }

         // next read connection info
         TiXmlElement *pXmlConnections = pXmlLayer->FirstChildElement( "connections" );
         if ( pXmlConnections == NULL )
            {
            CString msg;
            msg.Format( _T("Error reading SocialNetwork input file %s: Missing <connections> entry.  This is required to continue..."), filename );
            Report::ErrorMsg( msg );
            this->RemoveLayer( name );
            goto nextLayer;
            }

         // start adding connections based on definitions in XML
         TiXmlElement *pXmlConnection = pXmlConnections->FirstChildElement( "connection" );
         while ( pXmlConnection != NULL )
            {
            LPCTSTR from = NULL;
            LPCTSTR to   = NULL;
            float weight = 0;
            XML_ATTR attrs[] = {  // attr      type          address  isReq checkCol
                               { "from",      TYPE_STRING,   &from,   true,  0 },
                               { "to",        TYPE_STRING,   &to,     true,  0 },
                               { "weight",    TYPE_FLOAT,    &weight, true,  0 },
                               { NULL,        TYPE_NULL,     NULL,    false, 0 } };

            bool ok = TiXmlGetAttributes( pXmlConnection, attrs, filename );
            if ( ok )
               {
               if ( from != NULL && from[ 0 ] == '*' )
                  from = pLayer->m_name.GetString();

               SNNode *pFromNode = pLayer->FindNode( from );
               if ( pFromNode == NULL )
                  {
                  CString msg;
                  msg.Format( _T("Error reading SocialNetwork input file %s: Connection 'from' node '%s' is not a valid node.  This connection will be ignored..."), filename, from );
                  Report::ErrorMsg( msg );
                  }

               SNNode *pToNode = pLayer->FindNode( to );
               if ( pToNode == NULL )
                  {
                  CString msg;
                  msg.Format( _T("Error reading SocialNetwork input file %s: Connection 'to' node '%s' is not a valid node.  This connection will be ignored..."), filename, from );
                  Report::ErrorMsg( msg );
                  }

               if ( pFromNode != NULL && pToNode != NULL )
                  pLayer->AddConnection( pFromNode, pToNode, weight );
               }

            pXmlConnection = pXmlConnection->NextSiblingElement( "connection" );
            }

         }  // end of: if ( layer is ok )

nextLayer:
      pXmlLayer = pXmlLayer->NextSiblingElement( "layer" );
      }
	  
   return true;
   }


bool SocialNetwork::RemoveLayer( LPCTSTR name )
   {
   for ( INT_PTR i=0; i < this->m_layerArray.GetSize(); i++ )
      {
      if ( m_layerArray[ i ]->m_name.CompareNoCase( name ) == 0 )
         {
         m_layerArray.RemoveAt( i );
         return true;
         }
      }

   return false;
   }


void SocialNetwork::SetInputActivations( void )
   {
   for ( int i=0; i < this->GetLayerCount(); i++ )
      {
      SNLayer *pLayer = GetLayer( i );

      if ( pLayer->m_use == false )
         continue;

      ASSERT( pLayer->m_pModel != NULL );

      // get input node
      SNNode *pNode = pLayer->GetNode( 0 );
      ASSERT( pNode->m_nodeType == NNT_INPUT );

     pLayer->SetInCountTable();

      float activation = pLayer->m_pModel->m_score;    // -3 to +3, +3=abundant
      pNode->m_activationLevel = -Scale( -3, 3, activation );   // invert to represent scarcity rather than abundance
      }
   }

void SocialNetwork::SetValueVectors( void )
{
	map<string, vector<float>> OrgValues;
	for ( int i=0; i < this->GetLayerCount(); i++ )
	{
	  SNLayer *pLayer = GetLayer( i );
	  if ( pLayer->m_use == false )
	     continue;
	  ASSERT( pLayer->m_pModel != NULL );

	  for ( int j = 0; j < pLayer->GetNodeCount(); j++)
	  {
		  SNNode *pNode = pLayer->GetNode(j);
		  if (pNode->m_nodeType == NNT_OUTPUT || pNode->m_nodeType == NNT_INPUT)
			  continue;

		  vector<float> tempOrgValues (this->GetLayerCount());
			  
		  /* need to actually simply write code to read xml.  */
		  /* could change these later via another function */
	  
	  }
	}
}

void SocialNetwork::ActivateLayers( void )
   {
   for ( int i=0; i < this->GetLayerCount(); i++ )
      {
      SNLayer *pLayer = GetLayer( i );

      if ( pLayer->m_use == false )
         continue;

      ASSERT( pLayer->m_pModel != NULL );
      pLayer->Activate();
      }
   }

///////////////////////utility components///////////////////////////////
//1: similarity
//2:  degree
//3:  Agg trust
//4:  transitivity
//5:  lower trust -> more disimilar or topographically distant

//1: similarity

//double SNLayer::GetSimilarity(AgNode , AgNode)



double SNNode::GetInputWeight()
{
   double ans = -99.0;
   for (int i = 0; i < m_connectionArray.GetCount(); i++)
   {
      if (m_connectionArray[i]->m_pFromNode->m_nodeType == NNT_INPUT)
      {
         ans = m_connectionArray[i]->m_weight;
      }
   }
   return ans;
}




bool SocialNetwork::GetMetricLabel( int i, CString &label )
{//density, centrality, clustering, distribution of ties, rate of change in nodes, ties, density.  nodes: number of connections, sum of weights of connections, average weight of connections.
   switch( i )
      {
      case 0:  // density
         label = _T("Density");
         return true;

      }

   return false;
   }  


float SocialNetwork::GetMetricValue( int layer, int i )
   {
   switch( i )
      {
      case 0:        // density
         {
         SNLayer *pLayer = this->GetLayer( layer );
         //return pLayer->GetDensity( layer );
         }
      }

   return 0;
   }



 float SNNode::Activate( void )
   {
   // basic idea - iterate through incoming connections, computing inputs.  These get
   // summed and processed through a transfer function to generate an activation between 
   // ACT_LOWERBOUND, ACT_UPPERBOUND
   // activation rule - simple for now, assume linear influences - THIS NEEDS TO BE IMPROVED
   if ( m_nodeType == NNT_INPUT )   // don't activate inputs!!!
      return m_activationLevel;

   int connectionCount = (int) m_connectionArray.GetSize();
   
   float input = 0;
   //landscapeScore*weight is bias term, reflecting the relative importance of the orgs own assessment compared with the importance of other orgs.
   float bias = 0; 

   for ( int i=0; i < connectionCount; i++ )
      {
      SNConnection *pConnection = m_connectionArray[ i ];
	  if (pConnection->m_pFromNode->m_nodeType == NNT_INPUT)
		{
			bias = pConnection->m_weight * pConnection->m_pFromNode->m_activationLevel;
			continue;
	  }
      float connectionStrength = pConnection->m_weight * pConnection->m_pFromNode->m_activationLevel;
   
      input += connectionStrength;
      }
   if (connectionCount-1)
   {
		input = 3*input/connectionCount;  //input should be scaled from -3 to 3
   }
   input += bias;

   // transform input into activation using activation function
   m_activationLevel =  1/(1 + exp(-input));  // input should range from -6 to 6.  Here, we use the logistic to adjust activation range to ~ 0.0024 to 0.997. NOTE:  NEED BETTER TRANSFER FUNCTION
   

   return m_activationLevel;
   }


 int SNLayer::Activate( void )
   {
   // basic idea - iterate randomly through the network, computing activation levels as we go.  Accumulate the
   // "energy" of the network (defined as the sum of the activations of each node), and compute the change from the
   // previous cycle.  Activation is complete When the activations converge 
   int nodeCount = (int) m_nodeArray.GetSize();

   // create an array of pointers to the nodes.  This will be used to "shuffle" the nodes prior to 
   // each invocation of the activition calculation loop.
   SNNode **nodeArray = new LPSNNode[ nodeCount-1 ];
   int count = 0;

   for ( int i=0; i < nodeCount; i++ )
      {
      SNNode *pNode = m_nodeArray[ i ];

      if ( pNode->m_nodeType != NNT_INPUT )
         {
         nodeArray[ count ] = pNode;
         count++;
         }
      }
   
   ASSERT( count == nodeCount-1 );
   
   float delta = 9999999.0f;
   float lastActivationSum = 0;
   //const float relaxationFactor = 0.2f;

   m_activationIterations = 0;

   while ( delta > m_pSocialNetwork->m_deltaTolerance && m_activationIterations < m_pSocialNetwork->m_maxIterations )
      {
      float activationSum = 0;

      //ShuffleNodes( nodeArray, nodeCount );

      for ( int i=0; i < count; i++ )
         {
         SNNode *pNode = nodeArray[ i ];
         ASSERT( pNode->m_nodeType != NNT_INPUT );
         activationSum += pNode->Activate();
         }

      activationSum /= count;             // get average
      delta = fabs( lastActivationSum - activationSum );

      lastActivationSum = activationSum;
      ++m_activationIterations;
      }
   //iterate through connections, adjusting weights according to similarity of activation of connected nodes.  Threshold is the max tolerable difference between activation level of orgs (representing diff in orgs effort/investment)
   CArray < SNConnection*, SNConnection* > interiorConnections;
   GetInteriorConnections(interiorConnections);
   double threshold = 0.2;

   for (int i = 0; i < interiorConnections.GetSize(); i++)
		{
			SNConnection *pconnection  = interiorConnections[i];
			pconnection->m_weight += float( (threshold - fabs(pconnection->m_pFromNode->m_activationLevel - pconnection->m_pToNode->m_activationLevel))*((pconnection->m_pFromNode->m_activationLevel + pconnection->m_pToNode->m_activationLevel)/2)/(threshold) );
		}

   delete [] nodeArray;
   
   return m_activationIterations;
   }

// SNLayer::Output()
// {
//
//
// }
