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

#include "InfluenceNetwork.h"

#include "tixml.h"

#include <unitconv.h>
#include <math.h>

float INLayer::m_weightThreshold = 0.1f;   // abs value below which a weight is considered 0
float INLayer::m_afSlopeScalar = 6.0f;     // 1-10, determines slope of acivation function (default=6)
float INLayer::m_afTranslate = 0; 
float InfluenceNetwork::m_shapeParam = 1.0f;  // smaller values = wider shape for sinusoid activation function

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
//to do:  SNDensity is now in the Init function - should probably call from the getMetricValue fx.
RandUniform INLayer::m_randShuffle;
RandUniform randNodeInit;

INNode::INNode( NNTYPE nodeType )
: m_nodeType( nodeType )
, m_extra( NULL )
, m_activationLevel( 0 )
, m_x( 0 )
, m_y( 0 )
   {
   m_activationLevel = (float) randNodeInit.RandValue( ACT_LOWER_BOUND, ACT_UPPER_BOUND );   
   }


/////////////////////////// I N L A Y E R //////////////////////////////////////////

INLayer::INLayer( INLayer &sl )
   : m_name( sl.m_name )
   , m_use( sl.m_use )
   , m_activationIterations( sl.m_activationIterations )
   , m_pInfluenceNetwork( NULL )  // NOTE - MUST BE SET AFTER CONSTRUCTION;
   , m_inputNodeArray( sl.m_inputNodeArray )      // set below...
   , m_interiorNodeArray( sl.m_interiorNodeArray )
   , m_outputNodeArray( sl.m_outputNodeArray )
   , m_connectionArray( sl.m_connectionArray )
   , m_layerData(U_UNDEFINED)
   {
   // build map
   for ( int i = 0; i < GetInputNodeCount(); i++ )
      this->m_nodeMap[m_inputNodeArray[i]->m_name] = m_inputNodeArray[ i ];

   for ( int i = 0; i < GetInteriorNodeCount(); i++ )
      this->m_nodeMap[m_interiorNodeArray[i]->m_name] = m_interiorNodeArray[ i ];

   for ( int i = 0; i < GetOutputNodeCount(); i++ )
      this->m_nodeMap[m_outputNodeArray[i]->m_name] = m_outputNodeArray[ i ];
   }

INLayer::~INLayer()
{ }


INNode *INLayer::AddNode( NNTYPE nodeType, LPCTSTR name )
   {
   INNode *pNode = new INNode( nodeType );
   pNode->m_name = name;

   switch ( nodeType )
      {
      case NNT_INPUT:
         m_inputNodeArray.Add( pNode );
         break;

      case NNT_INTERIOR:
         m_interiorNodeArray.Add( pNode );
         break;

      case NNT_OUTPUT:
         m_outputNodeArray.Add( pNode );
         break;
      }


   m_nodeMap.SetAt( name, pNode );

   return pNode;
   }


INConnection *INLayer::AddConnection( INNode *pFromNode, INNode *pToNode, float weight )
   {
   ASSERT( pFromNode != NULL && pToNode != NULL );

   INConnection *pConnection = new INConnection( pFromNode, pToNode );
   m_connectionArray.Add( pConnection );

   pConnection->m_weight = pConnection->m_initWeight = weight;
   pToNode->m_inConnectionArray.Add( pConnection );   // store input connections info at INNode level
   pFromNode->m_outConnectionArray.Add( pConnection );   // store output connections info at INNode level as well

   return pConnection;
   }


INNode *INLayer::FindNode( LPCTSTR name, NNTYPE type /*=NNT_UNKNOWN */ )
   {
   INNode *pNode = NULL;
   BOOL found = m_nodeMap.Lookup( name, pNode );
   
   if ( found )
      return pNode;
   else
      return NULL;
   } 


void INLayer::SetAllConnections( float weight )
   {
   for ( int i=0; i < m_connectionArray.GetSize(); i++ )
      {
      m_connectionArray[ i ]->m_weight = m_connectionArray[ i ]->m_initWeight = weight;
      }
   }

void INLayer::SetAllInteriorNodes( float activation )
   {
   for ( int i=0; i < m_interiorNodeArray.GetSize(); i++ )
      GetInteriorNode( i )->m_activationLevel = activation;
   }

void INLayer::SetAllInputNodes(float activation)
   {
   for (int i = 0; i < m_inputNodeArray.GetSize(); i++)
      GetInputNode(i)->m_activationLevel = activation;
   }


float INLayer::GetEdgeDensity( NNTYPE nodeType /*=NNT_UNKNOWN*/)   //number of edges/max possible number of edges
   {
   int totalEdgeCount = 0;
   int actualEdgeCount = 0;

   if ( nodeType == NNT_INTERIOR || nodeType == NNT_UNKNOWN )
      {
      int nodeCount = GetInteriorNodeCount();
      for ( int i=0; i < nodeCount; i++ )
         {
         INNode *pNode = GetInteriorNode( i );
         int connectionCount = pNode->GetInConnectionCount();
         for ( int j=0; j < connectionCount; j++  )
            {
            totalEdgeCount++;
            if ( fabs( pNode->GetInConnection(j)->m_weight ) >= m_weightThreshold )
               actualEdgeCount++;
            }

         totalEdgeCount += connectionCount;
         }
      }

   if ( nodeType == NNT_OUTPUT || nodeType == NNT_UNKNOWN )
      {
      int nodeCount = GetOutputNodeCount();
      for ( int i=0; i < nodeCount; i++ )
         {
         INNode *pNode = GetOutputNode( i );
         int connectionCount = pNode->GetInConnectionCount();
         for ( int j=0; j < connectionCount; j++  )
            {
            totalEdgeCount++;
            if ( fabs( pNode->GetInConnection(j)->m_weight ) >= m_weightThreshold )
               actualEdgeCount++;
            }

         totalEdgeCount += connectionCount;
         }
      }

   if ( totalEdgeCount > 0  )
      return float( actualEdgeCount ) / totalEdgeCount;
   else
      return 0;
   }


//sum of weights/max possible sum of weights
float INLayer::GetEdgeWeightDensity( NNTYPE nodeType /*=NNT_UNKNOWN*/)
   {
   int totalEdgeCount = 0;
   //int actualEdgeCount = 0;
   float wtAvg = 0;

   if ( nodeType == NNT_INTERIOR || nodeType == NNT_UNKNOWN )
      {
      int nodeCount = GetInteriorNodeCount();
      for ( int i=0; i < nodeCount; i++ )
         {
         INNode *pNode = GetInteriorNode( i );
         int connectionCount = pNode->GetInConnectionCount();
         for ( int j=0; j < connectionCount; j++  )
            {
            totalEdgeCount++;
            if ( fabs( pNode->GetInConnection(j)->m_weight ) >= m_weightThreshold )
               {
               //actualEdgeCount++;
               wtAvg += pNode->GetInConnection(j)->m_weight;
               }
            }

         totalEdgeCount += connectionCount;
         }
      }

   if ( nodeType == NNT_OUTPUT || nodeType == NNT_UNKNOWN )
      {
      int nodeCount = GetOutputNodeCount();
      for ( int i=0; i < nodeCount; i++ )
         {
         INNode *pNode = GetOutputNode( i );
         int connectionCount = pNode->GetInConnectionCount();
         for ( int j=0; j < connectionCount; j++  )
            {
            totalEdgeCount++;
            if ( fabs( pNode->GetInConnection(j)->m_weight ) >= m_weightThreshold )
               {
               //actualEdgeCount++;
               wtAvg += pNode->GetInConnection(j)->m_weight;
               }
            }

         totalEdgeCount += connectionCount;
         }
      }

   if ( totalEdgeCount > 0  )
      return wtAvg / ( 1.0f*totalEdgeCount);

   else
      return 0;
}  


// calculates ratio of edges connected to node vs. edges in the network.  Only
// considers interior nodes
float INLayer::GetDegreeCentrality( INNode *pNode )
   {
   if ( pNode->m_nodeType != NNT_INTERIOR )
      return 0;

   int connections = (int) this->m_connectionArray.GetSize();
   int connectedCount = 0;
   int totalCount = 0;

   // get all the incoming connections
   for ( int i=0; i < connections; i++ )
      {
      INConnection *pConn = m_connectionArray[ i ];

      // only consider interior-to-interior connections
      if ( pConn->m_pFromNode->m_nodeType != NNT_INTERIOR || pConn->m_pToNode->m_nodeType != NNT_INTERIOR )
         continue;

      if ( pConn->m_weight < INLayer::m_weightThreshold )
         continue;

      totalCount++;
      
      if ( pConn->m_pFromNode == pNode|| pConn->m_pToNode == pNode ) 
         connectedCount++;
      }

   return ( float( connectedCount ) / totalCount );
   }



int INLayer::GetDegreeDistribution( CArray<float,float> &distArray )
   {
   // Notes:  Degree is the number of incoming and outgoing connections
   //         associated with a node.
   distArray.RemoveAll();
   int count = GetInteriorNodeCount();
   
   for( int i=0; i < count; i++ )
      {
      INNode *pNode = GetInteriorNode( i );

      int degree = pNode->GetDegree();

      // add bins if needed
      while ((int) distArray.GetSize() <= degree )
         distArray.Add( 0 );

      // add to bins
      distArray[ degree ] += 1.0f;
      }

   // normalize bins?

   for ( int i=0; i < (int) distArray.GetSize(); i++ )
      distArray[ i ] /= count;

   return (int) distArray.GetSize();
   }


void INLayer::ShuffleNodes( INNode **nodeArray, int nodeCount )
   {
   //--- Shuffle elements by randomly exchanging each with one other.
   for ( int i=0; i < nodeCount-1; i++ )
      {
      int randVal = (int) m_randShuffle.RandValue( 0, nodeCount-i-0.0001f );

      int r = i + randVal; // Random remaining position.
      INNode *temp = nodeArray[ i ];
      nodeArray[ i ] = nodeArray[ r ];
      nodeArray[ r ] = temp;
      }
   }




int InfluenceNetwork::AddRandomLayer( int inputCount, int interiorCount, int outputCount, float linkDensity )
   {
   INLayer *pLayer = this->AddLayer();

   for ( int i=0; i < inputCount; i++ )
      {
      CString label;
      label.Format( "Input_%i", i );
      pLayer->AddNode( NNT_INPUT, (LPCTSTR) label );
      }

   for ( int i=0; i < interiorCount; i++ )
      {
      CString label;
      label.Format( "Node_%i", i );
      pLayer->AddNode( NNT_INTERIOR, (LPCTSTR) label );
      }

   for ( int i=0; i < outputCount; i++ )
      {
      CString label;
      label.Format( "Output_%i", i );
      pLayer->AddNode( NNT_OUTPUT, (LPCTSTR) label );
      }

   // input to interior connections
   for ( int i=0; i < inputCount; i++ )
      {
      INNode *pNode = pLayer->GetInputNode(  i );

      for ( int j=0; j < interiorCount; j++ )
         {
         if ( INLayer::m_randShuffle.RandValue(0,1) <= linkDensity )
            pLayer->AddConnection( pNode, pLayer->GetInteriorNode( j ), (float) INLayer::m_randShuffle.RandValue( -1, 1 ) );
         }
      }

   // interior to interior connections
   for ( int i=0; i < interiorCount; i++ )
      {
      INNode *pNode = pLayer->GetInteriorNode( i );

      for ( int j=0; j < interiorCount; j++ )
         {
         if ( i != j && INLayer::m_randShuffle.RandValue(0,1) <= linkDensity )
            pLayer->AddConnection( pNode, pLayer->GetInteriorNode( j ), (float) INLayer::m_randShuffle.RandValue( -1, 1 ) );
         }
      }

   // interior to output connections
   for ( int i=0; i < interiorCount; i++ )
      {
      INNode *pNode = pLayer->GetInteriorNode( i );

      for ( int j=0; j < outputCount; j++ )
         {
         if ( INLayer::m_randShuffle.RandValue(0,1) <= linkDensity )
            pLayer->AddConnection( pNode, pLayer->GetOutputNode( j ), (float) INLayer::m_randShuffle.RandValue( -1, 1 ) );
         }
      }

   return pLayer->GetConnectionCount();
   }


int InfluenceNetwork::AddFullyConnectedLayer(  int inputCount, int interiorCount, int outputCount )
   {
   INLayer *pLayer = this->AddLayer();

   for ( int i=0; i < inputCount; i++ )
      {
      CString label;
      label.Format( "Input_%i", i );
      pLayer->AddNode( NNT_INPUT, (LPCTSTR) label );
      }

   for ( int i=0; i < interiorCount; i++ )
      {
      CString label;
      label.Format( "Node_%i", i );
      pLayer->AddNode( NNT_INTERIOR, (LPCTSTR) label );
      }

   for ( int i=0; i < outputCount; i++ )
      {
      CString label;
      label.Format( "Output_%i", i );
      pLayer->AddNode( NNT_OUTPUT, (LPCTSTR) label );
      }

   // input to interior connections
   for ( int i=0; i < inputCount; i++ )
      {
      INNode *pNode = pLayer->GetInputNode(  i );

      for ( int j=0; j < interiorCount; j++ )
         pLayer->AddConnection( pNode, pLayer->GetInteriorNode( j ), (float) INLayer::m_randShuffle.RandValue( -1, 1 ) );
      }

   // interior to interior connections
   for ( int i=0; i < interiorCount; i++ )
      {
      INNode *pNode = pLayer->GetInteriorNode( i );

      for ( int j=0; j < interiorCount; j++ )
         {
         if ( i != j  )
            pLayer->AddConnection( pNode, pLayer->GetInteriorNode( j ), (float) INLayer::m_randShuffle.RandValue( -1, 1 ) );
         }
      }

   // interior to output connections
   for ( int i=0; i < interiorCount; i++ )
      {
      INNode *pNode = pLayer->GetInteriorNode( i );

      for ( int j=0; j < outputCount; j++ )
         pLayer->AddConnection( pNode, pLayer->GetOutputNode( j ), (float) INLayer::m_randShuffle.RandValue( -1, 1 ) );
      }

   return pLayer->GetConnectionCount();
   }


bool InfluenceNetwork::LoadXml( LPCTSTR filename )
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
      msg.Format( _T("Error reading InfluenceNetwork input file %s: Missing <layers> entry.  This is required to continue..."), filename );
      Report::LogError( msg);
      return false;
      }

   TiXmlElement *pXmlLayer = pXmlLayers->FirstChildElement( "layer" );
   if ( pXmlLayer == NULL )
      {
      CString msg;
      msg.Format( _T("Error reading InfluenceNetwork input file %s: Missing <layer> entry.  This is required to continue..."), filename );
      Report::LogError( msg);
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
         INLayer *pLayer = this->AddLayer();
         if ( name != NULL )
            pLayer->m_name = name;

         // next, get inputs
         TiXmlElement *pXmlInputs = pXmlLayer->FirstChildElement( "inputs" );
         if ( pXmlInputs == NULL )
            {
            CString msg;
            msg.Format( _T("Error reading InfluenceNetwork input file %s: Missing <inputs> entry.  This is required to continue..."), filename );
            Report::LogError( msg);
            this->RemoveLayer( name );
            goto nextLayer;
            }

         TiXmlElement *pXmlInput = pXmlInputs->FirstChildElement( "input" );
         while( pXmlInput != NULL )
            {
            LPCTSTR name = NULL;
            float value  = NULL;
            XML_ATTR attrs[] = {  // attr      type          address  isReq checkCol
                               { "name",      TYPE_STRING,   &name,   true,  0 },
                               { "value",     TYPE_FLOAT,    &value,  true,  0 },
                               { NULL,        TYPE_NULL,     NULL,    false, 0 } };

            bool ok = TiXmlGetAttributes( pXmlInput, attrs, filename );
            if ( ok )
               {
               INNode *pNode = new INNode( NNT_INPUT );
               pNode->m_name = name;
               pLayer->m_inputNodeArray.Add( pNode );
               }

            pXmlInput = pXmlInput->NextSiblingElement( "input" );
            }

         // next read node info
         TiXmlElement *pXmlNodes = pXmlLayer->FirstChildElement( "nodes" );
         if ( pXmlNodes == NULL )
            {
            CString msg;
            msg.Format( _T("Error reading InfluenceNetwork input file %s: Missing <nodes> entry.  This is required to continue..."), filename );
            Report::LogError( msg );
            this->RemoveLayer( name );
            goto nextLayer;
            }

         // start adding interior nodes based on definitions in XML
         TiXmlElement *pXmlNode = pXmlNodes->FirstChildElement( "node" );
         while ( pXmlNode != NULL )
            {
            LPCTSTR name = NULL;
            float value  = NULL;
            XML_ATTR attrs[] = { // attr   type          address    isReq checkCol
                               { "name",   TYPE_STRING,  &name,     true,  0 },
                               { "goal",  TYPE_FLOAT,    &value,    true,  0 },
                               { NULL,     TYPE_NULL,    NULL,      false, 0 } };
         
            bool ok = TiXmlGetAttributes( pXmlNode, attrs, filename );
            if ( ok )
               {
               INNode *pNode = pLayer->AddNode( NNT_INTERIOR, name );
               pNode->m_value = value;
               }
            else
               {
               CString msg;
               msg.Format( _T("InfluenceNetwork: bad <node> encountered reading file %s"), filename );
               Report::LogError( msg );
               }
               
            pXmlNode = pXmlNode->NextSiblingElement( "node" );
            }

         // next, get outputs
         TiXmlElement *pXmlOutputs = pXmlLayer->FirstChildElement( "outputs" );
         if ( pXmlOutputs != NULL )
            {
            TiXmlElement *pXmlOutput = pXmlOutputs->FirstChildElement( "output" );
            while( pXmlOutput != NULL )
               {
               LPCTSTR name = NULL;
               XML_ATTR attrs[] = {  // attr      type          address  isReq checkCol
                               { "name",      TYPE_STRING,   &name,   true,  0 },
                               { NULL,        TYPE_NULL,     NULL,    false, 0 } };

               bool ok = TiXmlGetAttributes( pXmlOutput, attrs, filename );
               if ( ok )
                  {
                  INNode *pNode = new INNode( NNT_OUTPUT );
                  pNode->m_name = name;
                  pLayer->m_outputNodeArray.Add( pNode );
                  }

               pXmlOutput = pXmlOutput->NextSiblingElement( "output" );
               }
            }

         // next read connection info
         TiXmlElement *pXmlConnections = pXmlLayer->FirstChildElement( "connections" );
         if ( pXmlConnections == NULL )
            {
            CString msg;
            msg.Format( _T("Error reading InfluenceNetwork input file %s: Missing <connections> entry.  This is required to continue..."), filename );
            Report::LogError( msg );
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
               INNode *pFromNode=NULL, *pToNode=NULL;

               // find the to-node first
               pToNode = pLayer->FindNode( to );
               if ( pToNode == NULL || pToNode->m_nodeType == NNT_INPUT )
                  {
                  CString msg;
                  msg.Format( _T("InfluenceNetwork: Error reading input file %s: Connection 'to' node '%s' is not a valid node.  This connection will be ignored..."), filename, from );
                  Report::LogError( msg );
                  }
               else   // sucess with "to", get "from" node next
                  { 
                  // from node can be a name or *, meaning all connections at this level
                  //  Note that 'from' nodes must be input or interior (NOT output),
                  //    'to' nodes are interior or output (NOT input)
                  if ( from[ 0 ] == '*' )    // from ALL interior nodes
                     {
                     // connect "from" ALL other interior nodes, 'to' this node
                     for ( int i=0; i < pLayer->GetInteriorNodeCount(); i++ )
                        {
                        pFromNode = pLayer->GetInteriorNode( i );
                        if ( pToNode != pFromNode )
                           pLayer->AddConnection( pFromNode, pToNode, weight );
                        }
                     }
                  else if ( from[ 0 ] == '#' && pToNode->m_nodeType == NNT_INTERIOR )  // 'from' All input nodes "to" interior node
                      {
                      for ( int i=0; i < pLayer->GetInputNodeCount(); i++ )
                         {
                         pFromNode = pLayer->GetInputNode( i );
                         pLayer->AddConnection( pFromNode, pToNode, weight );
                         }
                      }
                  else
                     {
                     pFromNode = pLayer->FindNode( from );
                     if ( pFromNode == NULL )
                        {
                        CString msg;
                        msg.Format( _T("Error reading InfluenceNetwork input file %s: Connection 'from' node '%s' is not a valid node.  This connection will be ignored..."), filename, from );
                        Report::LogError( msg );
                        }
                     else
                        pLayer->AddConnection( pFromNode, pToNode, weight );
                     }
                  }
               }

            pXmlConnection = pXmlConnection->NextSiblingElement( "connection" );
            }

         }  // end of: if ( layer is ok )

nextLayer:
      pXmlLayer = pXmlLayer->NextSiblingElement( "layer" );
      }
     
   return true;
   }


bool InfluenceNetwork::RemoveLayer( LPCTSTR name )
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


void InfluenceNetwork::ActivateLayers( void )
   {
   // activates each layer in the network.
   for ( int i=0; i < this->GetLayerCount(); i++ )
      {
      INLayer *pLayer = GetLayer( i );

      if ( pLayer->m_use == false )
         continue;

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

//double INLayer::GetSimilarity(AgNode , AgNode)

float INNode::GetInputSignal()
   {
   // for this node, goes through all input connections, and sums
   // their total weight*activation. return value is [-inf, +inf]

   int inputCount = GetInConnectionCount();
   float signal = 0;
   //float sumWeights = 0;
   int count = 0;
   
   for (int i = 0; i < inputCount; i++)
      {
      float wt = m_inConnectionArray[i]->m_weight;
      float activation = m_inConnectionArray[ i ]->m_pFromNode->m_activationLevel;

      ASSERT( wt <= 1.0f );
      ASSERT( wt >= -1.0f );

      if ( fabs( wt ) >= INLayer::m_weightThreshold  )
         {
         //wt = (wt+1.0f)/2.0f;   // scale to [0,1]
         signal += (wt * activation);
         //sumWeights += wt;
         count++;
         }
      }
   if ( count == 0  )
      return 0;
   else
      return signal;  ///sumWeights;
   }


 float INNode::Activate( void )
   {
   // basic idea - iterate through incoming connections, computing inputs.  These get
   // summed and processed through a transfer function to generate an activation between 
   // ACT_LOWERBOUND, ACT_UPPERBOUND (note: not correct, [0,1]
   // activation rule - simple for now, assume linear influences - THIS NEEDS TO BE IMPROVED
   if ( m_nodeType == NNT_INPUT )   // don't activate inputs!!!
      return m_activationLevel;

   float inputSignal = GetInputSignal();    

   // transform input into activation using activation function
   // = 1/(1+exp(-a*(b+inputSignal)))
   // input should range from approx. -6 to 6.  Here, we use the logistic to adjust activation range to ~ 0.0024 to 0.997.
   // NOTE:  NEED BETTER TRANSFER FUNCTION
   //m_activationLevel =  1/(1 + exp(-INLayer::m_afSlopeScalar*(INLayer::m_afTranslate + inputSignal)) );   // [0,1]

   // check if input is beyond bounds
   float xBound = PI/InfluenceNetwork::m_shapeParam;

   if ( inputSignal > xBound ||  inputSignal < -xBound )
      return m_activationLevel = 1;

   m_activationLevel = float( 0.5f*sin((inputSignal*InfluenceNetwork::m_shapeParam) - PI/2) + 0.5f );  //  [0,1]
   return m_activationLevel;
   }

 INLayer::INLayer(InfluenceNetwork *pNet) 
    : m_pInfluenceNetwork(pNet)
    , m_use(true)
    , m_activationIterations(0)
    , m_layerData(3, 0, U_UNDEFINED)
    { 
    m_layerData.SetName( "IN Layer Data" );
    m_layerData.SetLabel(0, "Step");
    m_layerData.SetLabel(1, "Input");
    m_layerData.SetLabel(2, "Output");    
    }


 int INLayer::Activate( void )
   {
   // basic idea - iterate randomly through the network, computing activation levels as we go.  Accumulate the
   // "energy" of the network (defined as the sum of the activations of each node), and compute the change from the
   // previous cycle.  Repeat until activations converge 
   int nodeCount = (int) this->GetInteriorNodeCount();
   
   // create an array of pointers to the nodes.  This will be used to "shuffle" the nodes prior to 
   // each invocation of the activition calculation loop.
   INNode **nodeArray = new LPINNode[ nodeCount ];

   for ( int i=0; i < nodeCount; i++ )
      nodeArray[ i ] = GetInteriorNode( i );
   
   float delta = 9999999.0f;
   float lastActivationSum = 0;
   //const float relaxationFactor = 0.2f;
   
   m_activationIterations = 0;
   
   // start iterating, updating the entire network and checking for convergence
   while ( delta > m_pInfluenceNetwork->m_deltaTolerance
      && m_activationIterations < m_pInfluenceNetwork->m_maxIterations )
      {
      //ShuffleNodes( nodeArray, nodeCount );   // randomize nodes
   
      float activationSum = 0;  // sum of activations across all interior nodes

      // for the interior nodes, update their current activation levels based on inputs
      for ( int i=0; i < nodeCount; i++ )
         {
         INNode *pNode = nodeArray[ i ];
         float activation = pNode->Activate();  // compute and set activation level for the node
         activationSum += activation;
         }
   
      activationSum /= nodeCount;             // get average activation level
      delta = (float) fabs( lastActivationSum - activationSum );   // compare to last iteration - are we still changing?
   
      lastActivationSum = activationSum;
      ++m_activationIterations;
      }

   // interior node convergence reached - update output nodes
   for ( int i=0; i < GetOutputNodeCount(); i++ )
      m_outputNodeArray[i]->Activate();

   //iterate through connections, adjusting weights according to similarity of activation of connected nodes.  Threshold is the max tolerable difference between activation level of orgs (representing diff in orgs effort/investment)
   //CArray < SNConnection*, SNConnection* > interiorConnections;
   //GetInteriorConnections(interiorConnections);
   //double threshold = 0.2;
   //
   //for (int i = 0; i < interiorConnections.GetSize(); i++)
   //   {
   //      SNConnection *pconnection  = interiorConnections[i];
   //      pconnection->m_weight += float( (threshold - abs(pconnection->m_pFromNode->m_activationLevel - pconnection->m_pToNode->m_activationLevel))*((pconnection->m_pFromNode->m_activationLevel + pconnection->m_pToNode->m_activationLevel)/2)/(threshold) );
   //   }
   
   delete [] nodeArray;
   
   return m_activationIterations;
   }

// INLayer::Output()
// {
//
//
// }


 
//////////////////////////////////////////////////////////////////////////////
//    I N F L U E N C E    N E T W O R K
//////////////////////////////////////////////////////////////////////////////

//   InfluenceNetwork() : m_deltaTolerance( 0.0001f ), m_maxIterations( 100 ) { }
InfluenceNetwork::InfluenceNetwork( InfluenceNetwork &sn )
   : m_deltaTolerance( sn.m_deltaTolerance )
   , m_maxIterations( sn.m_maxIterations )
   , m_layerArray( sn.m_layerArray )
   {
   for ( int i=0; i < (int) m_layerArray.GetSize(); i++ )
      m_layerArray[ i ]->m_pInfluenceNetwork = this;
   }


bool InfluenceNetwork::Init( LPCTSTR filename )
   {
   // build network connections base in initStr.
   // Note that:
   //    1) Input nodes are required for landscape feedbacks and any external drivers
   //    2) Output nodes are required for each actor's values (one per actor/value pair)
   //    3) Interior nodes represent the social network

   if ( filename != NULL && lstrlen( filename ) > 0 )
      {
      bool ok = LoadXml( filename ); //, pEnvContext );  // defines basic nodes/network structure, connects inputs and outputs
      
      if ( ! ok ) 
         return false;
      }

   return true; 
   }


// get ready to start a dynamic run
bool InfluenceNetwork::InitRun( void )
   {
   // reset all weights to their initial states
   for ( int i=0; i < this->GetLayerCount(); i++ )
      {
      INLayer *pLayer = GetLayer( i );
      
      int connectionCount = pLayer->GetConnectionCount();
      for ( int j=0; j < connectionCount; j++ )
         {
         INConnection *pConnection = pLayer->GetConnection( j );
         pConnection->m_weight = pConnection->m_initWeight;
         }

      pLayer->m_layerData.ClearRows();
      }

   //SetInputActivations();
   ActivateLayers();

   return true; 
   }


bool InfluenceNetwork::Step( int cycle )
   {
   ActivateLayers();

   for ( int i=0; i < this->GetLayerCount(); i++ )
      {
      INLayer *pLayer = GetLayer( i );

      if ( pLayer->m_use == false )
         continue;
      
      float d[ 3 ];
      d[0] = (float) cycle;
      d[1] = pLayer->GetInputNode(0)->m_activationLevel;
      d[2] = pLayer->GetOutputNode(0)->m_activationLevel;
      pLayer->m_layerData.AppendRow( d, 3 );
      }
  
   return true; 
   }

