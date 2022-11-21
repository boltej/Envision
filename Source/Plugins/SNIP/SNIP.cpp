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

#include "EnvConstants.h"
#include <math.h>
#include <PathManager.h>
#include "SNIP.h"
#include <FDATAOBJ.H>
#include <Vdataobj.h>
#include <GEOMETRY.HPP>
#include <Scenario.h>
#include <colorramps.h>

#include <format>
#include <iostream>
#include <fstream>
#include <set>
#include <unordered_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//to do:  SNDensity is now in the Init function - should probably call from the getMetricValue fx.
RandUniform SNIPModel::m_randShuffle;
RandUniform randNodeInit;



float TraitContainer::ComputeSimilarity(TraitContainer* pOther)
   {
   // use only numeric attributes
   if (m_traits.GetSize() == 0 || pOther->m_traits.GetSize() == 0) {
      Report::ErrorMsg("Error", "Missing traits when computing similarity");
      return 0;
      }

   if (m_traits.GetSize() != pOther->m_traits.GetSize()) {
      Report::ErrorMsg("Error", "traits vectors different lengths when computing similarity");
      return 0;
      }

   int count = 0;
   float traitsDelta = 0;
   float similarity = 0;

   for (int i = 0; i < m_traits.GetSize(); i++)
      {
      //if (isNaN(sTraits[i]) == false && isNaN(rTraits[i]) == false) {
      traitsDelta += (this->m_traits[i] - pOther->m_traits[i]) * (this->m_traits[i] - pOther->m_traits[i]);
      count++;
      //   }
      }

   if (count > 0)
      {
      float dMax = (float)sqrt(count * 4);   // sqrt(sum(((1-(-1))^2)))summed of each dimension
      similarity = 1 - (sqrt(traitsDelta) / dMax);
      }

   return similarity;
   }





SNNode::SNNode(SNIP_NODETYPE nodeType)
   :m_nodeType(nodeType)
   {   
   //m_reactivity = (float) randNodeInit.RandValue( ACT_LOWER_BOUND, ACT_UPPER_BOUND );   
   }

//void SNNode::GetTrust()//get total trust for a single organization
//{
//   double out = 0;
//   for (int i = 0; i < m_edges.GetSize(); i++)
//   {
//      SNEdge *edge = m_edges[i];
//      if (edge->m_pFromNode->m_nodeType == NT_LANDSCAPE_ACTOR)
//      {
//         out += edge->m_pFromNode->m_trust;
//      }
//   m_trust = out;
//   }
//}


/*
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
      m_nodeInstances[i]->m_AgParent = nullptr;
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

*/

/////////////////////////// S N L A Y E R //////////////////////////////////////////

SNLayer::SNLayer(SNIP* pNet)
   : m_pSNIP(pNet)
   //, m_pInputNode(nullptr)
   , m_outputNodeCount(0)
   {
   m_pSNIPModel = new SNIPModel(this);
   }



SNLayer::SNLayer(SNLayer& sl)
   : m_name(sl.m_name)
   , m_pSNIPModel(nullptr)
   ////, m_pModel( sl.m_pModel )
   , m_outputNodeCount(sl.m_outputNodeCount)
   , m_pSNIP(nullptr)  // NOTE - MUST BE SET AFTER CONSTRUCTION;
   //, m_pInputNode( nullptr )      // set below...
   , m_nodes(sl.m_nodes)
   , m_edges(sl.m_edges)
   ////, m_outputArray( sl.m_outputArray )
   {
   m_pSNIPModel = new SNIPModel(this);


   // next, build map
   //CMap< LPCTSTR, LPCTSTR, SNNode*, SNNode* > m_nodeMap;
   // TODO!!!!
   ////GetInteriorNodes(m_interiorNodes);
   ////for (int i = 0; i < m_interiorNodes.GetCount(); i++)
   ////{
   ////   CString orgFrom = m_interiorNodes[i]->m_name;
   ////   for (int j = 0; j < m_interiorNodes.GetCount(); j++)
   ////   {
   ////      CString orgTo = m_interiorNodes[j]->m_name;
   ////      m_AdjacencyMatrix[orgTo][orgFrom] = 0.0;
   ////   }
   ////}
   }

SNLayer::~SNLayer()
   {
   if (m_pSNIPModel != nullptr)
      delete m_pSNIPModel;
   }


bool SNLayer::Init()
   {
   m_pSNIPModel->Init();
   return true;
   }



bool SNLayer::ExportNetworkGraphML(LPCTSTR path)
   {
   // declare output file stream :
   ofstream out;
   out.open(path);

   out << "<?xml version='1.0' encoding='UTF-8' ?>\n";
   out << "<graphml xmlns='http://graphml.graphdrawing.org/xmlns' xmlns:xsi = 'http://www.w3.org/2001/XMLSchema-instance'\n";
   out << "xsi:schemaLocation='http://graphml.graphdrawing.org/xmlns http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd'>\n";

   // start with node, edge properties
   //out << "<key id='react' for='node' attr.name='color' attr.type='string'>\n";
   out << "<key id='n1' for='node' attr.name='color' attr.type='string'/>\n";
   out << "<key id='n2' for='node' attr.name='size' attr.type='float' /> \n";
   out << "<key id='n3' for='node' attr.name='reactivity' attr.type='float'/> \n";
   out << "<key id='n4' for='node' attr.name='influence' attr.type='float'/> \n";

   out << "<key id='e1' for='edge' attr.name='weight' attr.type='float' /> \n";
   out << "<key id='e2' for='edge' attr.name='color' attr.type='string'><default>yellow</default></key>\n";
   out << "<key id='e3' for='edge' attr.name='signal_strength' attr.type='float' /> \n";
   out << "<key id='e4' for='edge' attr.name='trust' attr.type='float' /> \n";
   out << "<key id='e5' for='edge' attr.name='influence' attr.type='float' /> \n";

   out << "<graph id='G' edgedefault='directed'> \n";

   for (int i = 0; i < (int)this->m_nodes.GetSize(); i++)
      {
      SNNode* pNode = this->m_nodes[i];
      string color = "black";
      switch (pNode->m_nodeType)
         {
         case NT_ASSESSOR:          color = "red";  break;
         case NT_NETWORK_ACTOR:     color = "blue";  break;
         case NT_ENGAGER:           color = "cyan";  break;
         case NT_LANDSCAPE_ACTOR:   color = "green";  break;
         }

      out << "<node id='" << pNode->m_name << "'>\n";
      out << " <data key='n1'>" << color << "</data>\n";
      out << " <data key='n2'>" << pNode->m_reactivity << "</data>\n";
      out << " <data key='n3'>" << pNode->m_reactivity << "</data>\n";
      out << " <data key='n4'>" << pNode->m_influence << "</data>\n";
      out << "</node> \n";
      }

   for (int i = 0; i < (int)this->m_edges.GetSize(); i++)
      {
      SNEdge* pEdge = this->m_edges[i];

      string color = "black";

      //switch (pEdge->m_nodeType)
      //   {
      //   case NT_ASSESSOR:          color = "red";  break;
      //   case NT_NETWORK_ACTOR:     color = "blue";  break;
      //   case NT_ENGAGER:           color = "cyan";  break;
      //   case NT_LANDSCAPE_ACTOR:   color = "green";  break;
      //   }

      out << "<edge id='" << pEdge->m_name << "' source='" << pEdge->m_pFromNode->m_name << "' target='" << pEdge->m_pToNode->m_name << "'>\n";
      out << "  <data key='e1'>" << pEdge->m_trust << "</data> \n";  // weight
      out << "  <data key='e2'>" << color << "</data> \n";  // color
      out << "  <data key='e3'>" << pEdge->m_signalStrength << "</data> \n";  // signal strength
      out << "  <data key='e4'>" << pEdge->m_trust << "</data> \n";   // trust
      out << "  <data key='e5'>" << pEdge->m_influence << "</data> \n";  // influence
      out << "</edge> \n";
      }
      
   out << "</graph>\n";
   out << "</graphml>\n";
   out.close();

   return true;
   }


bool SNLayer::ExportNetworkGEXF(LPCTSTR path, LPCTSTR date)
   {
   // declare output file stream :
   ofstream out;
   out.open(path);

   out << "<?xml version='1.0' encoding='UTF-8'?>" << endl;
   out << "<gexf xmlns='http://gexf.net/1.3' xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:schemaLocation='http://gexf.net/1.3 http://gexf.net/1.3/gexf.xsd' version='1.3' xmlns:viz='http://gexf.net/1.3/viz' >" << endl;
   out << "<meta lastmodifieddate='2009-03-20'>" << endl;
   out << "    <creator>Envision</creator>" << endl;
   out << "    <description>SNIP network " << this->m_name << "</description>" << endl;
   out << "</meta>" << endl;
   if ( date == nullptr)
      out << "<graph mode='static' defaultedgetype='directed'>" << endl;
   else
      out << "<graph mode='dynamic' timeformat='date' defaultedgetype='directed'>" << endl;

   out << "<attributes class='node'>" << endl;
   out << "  <attribute id='n1' title='color' type='string'/>" << endl;
   out << "  <attribute id='n2' title='size' type='float' />" << endl;
   out << "  <attribute id='n3' title='reactivity' type='float'/>" << endl;
   out << "  <attribute id='n4' title='influence' type='float'/>" << endl;
   out << "</attributes>" << endl;
   out << "<attributes class='edge'>" << endl;
   out << "  <attribute id='e1' title='weight' type='float' />" << endl;
   out << "  <attribute id='e2' title='color' type='string'/>" << endl;
   out << "  <attribute id='e3' title='signal_strength' type='float' />" << endl;
   out << "  <attribute id='e4' title='trust' type='float' />" << endl;
   out << "  <attribute id='e5' title='influence' type='float' />" << endl;
   out << "</attributes>" << endl;

   out << "<nodes>" << endl;

   int nodes = (int)this->m_nodes.GetSize();

   int nAssessors = this->GetNodeCount(NT_ASSESSOR);
   int nNAs = this->GetNodeCount(NT_NETWORK_ACTOR);
   int nEngagers = this->GetNodeCount(NT_ENGAGER);
   int nLAs = this->GetNodeCount(NT_LANDSCAPE_ACTOR);

   int na = 0, nn = 0, ne = 0, nl = 0;
   int x=0, y=0;
   for (int i = 0; i < nodes; i++)
      {
      SNNode* pNode = this->m_nodes[i];
      // temporary
      if (pNode->m_nodeType == NT_LANDSCAPE_ACTOR)
         continue;

      string color = "black";
      switch (pNode->m_nodeType)
         {
         case NT_ASSESSOR:        color = "red";   x = 20; y = int(20+((60.0f*na)/nAssessors)); na++; break;
         case NT_NETWORK_ACTOR:   color = "blue";  x = 40; y = int(20+((60.0f*nn)/nNAs));       nn++; break;
         case NT_ENGAGER:         color = "cyan";  x = 60; y = int(20+((60.0f*ne)/nEngagers));  ne++; break;
         case NT_LANDSCAPE_ACTOR: color = "green"; x = 80; y = int(20+((60.0f*nl)/nLAs));       nl++; break;
         }

      if ( date == nullptr)
         out << "<node id='" << pNode->m_id << "' label='" << pNode->m_name << "' >" << endl;
      else
         out << "<node id='" << pNode->m_id << "' label='" << pNode->m_name << "' start='" << date << "' end='" << date << "' >" << endl;
      
      out << " <attvalues>\n";
      out << "  <attvalue for='n1' value='" << color << "'/>" << endl;
      out << "  <attvalue for='n2' value='" << pNode->m_reactivity << "'/>" << endl;
      out << "  <attvalue for='n3' value='" << pNode->m_reactivity << "'/>" << endl;
      out << "  <attvalue for='n4' value='" << pNode->m_influence << "'/>" << endl;
      out << " </attvalues>\n";
      
      // node color=reactivity
      // node size = influence
      int r=0, g=0, b=0;
      ::RWBColorRamp(-1,1, pNode->m_reactivity, r, g, b);

      out << " <viz:color r='" << r << "' g='" << g << "' b='" << b << "' a='1.0'/>" << endl;
      out << " <viz:position x='" << x << "' y='" << y << "' z='0.0'/>" << endl;
      out << " <viz:size value='" << (100*pNode->m_influence / this->m_pSNIPModel->m_netStats.maxNodeInfluence) << "'/>" << endl;
      out << " <viz:shape value='disc'/>" << endl;

      out << "</node> \n";
      }

   out << "</nodes>" << endl;
   out << "<edges>" << endl;

   for (int i = 0; i < (int)this->m_edges.GetSize(); i++)
      {
      SNEdge* pEdge = this->m_edges[i];

      // temporary
      if (pEdge->m_pToNode->m_nodeType == NT_LANDSCAPE_ACTOR)
         continue;



      string color = "black";

      //switch (pEdge->m_nodeType)
      //   {
      //   case NT_ASSESSOR:          color = "red";  break;
      //   case NT_NETWORK_ACTOR:     color = "blue";  break;
      //   case NT_ENGAGER:           color = "cyan";  break;
      //   case NT_LANDSCAPE_ACTOR:   color = "green";  break;
      //   }

      out << "<edge id='" << pEdge->m_id << "' source='" << pEdge->m_pFromNode->m_id << "' target='" << pEdge->m_pToNode->m_id << "'>" << endl;
      out << " <attvalues>\n";
      out << "  <attvalue for='e1' value='" << pEdge->m_trust << "'/>" << endl;
      out << "  <attvalue for='e2' value='" << color << "'/>" << endl;  // color
      out << "  <attvalue for='e3' value='" << pEdge->m_signalStrength << "'/>" << endl;  // signal strength
      out << "  <attvalue for='e4' value='" << pEdge->m_trust << "'/>" << endl;   // trust
      out << "  <attvalue for='e5' value='" << pEdge->m_influence << "'/>" << endl;  // influence
      out << " </attvalues>" << endl;

      // color = trust
      // thickness = influence
      int r = 0, g = 0, b = 0;
      ::RWBColorRamp(-1, 1, pEdge->m_trust, r, g, b);

      float thick = 1 + (8*pEdge->m_influence / this->m_pSNIPModel->m_netStats.maxEdgeInfluence);

      out << " <viz:color r='" << r << "' g='" << g << "' b='" << b << "' a='1.0'/>" << endl;
      out << " <viz:thickness value='" <<  thick << "'/>" << endl;
      out << " <viz:shape value='solid' />" << endl;

      out << "</edge> \n";
      }
   out << "</edges>" << endl;
   out << "</graph>" << endl;
   out << "</gexf>";

   out.close();

   return true;
   }



int SNLayer::CheckNetwork()
   {
   int nIslandNodes = 0;
   int nUnknownNodes = 0;

   for (int i = 0; i < (int)this->m_nodes.GetSize(); i++)
      {
      SNNode* pNode = this->m_nodes[i];

      if (pNode->m_inEdges.GetSize() == 0 && pNode->m_outEdges.GetSize() == 0)
         nIslandNodes++;

      if (pNode->m_nodeType == NT_UNKNOWN)
         nUnknownNodes++;
      }

   int nEdgeMissingNodes = 0;
   for (int i = 0; i < (int)this->m_edges.GetSize(); i++)
      {
      SNEdge* pEdge = this->m_edges[i];

      if ( pEdge->m_pToNode == nullptr)
         nEdgeMissingNodes++;
      else if ( FindNode(pEdge->m_pToNode->m_name) == nullptr )
         nEdgeMissingNodes++;

      if (pEdge->m_pFromNode == nullptr)
         nEdgeMissingNodes++;
      else if (FindNode(pEdge->m_pFromNode->m_name) == nullptr)
         nEdgeMissingNodes++;
      }

   if (nIslandNodes > 0)
      Report::Log_i("%i nodes in the network are unconnected to any other nodes", nIslandNodes, RA_NEWLINE, RT_WARNING);

   if (nUnknownNodes > 0)
      Report::Log_i("%i nodes in the network are of unknown type", nUnknownNodes, RA_NEWLINE, RT_WARNING);

   if (nEdgeMissingNodes > 0)
      Report::Log_i("%i edges in the network have missing connections (to/from nodes)", nEdgeMissingNodes, RA_NEWLINE, RT_WARNING);

   return nIslandNodes + nUnknownNodes + nEdgeMissingNodes;
   }




////////////////////////////////////////////////////////////////
// SNIP Model
////////////////////////////////////////////////////////////////


SNIPModel::~SNIPModel()
   {
   if (m_pOutputData)
      delete m_pOutputData;
   }


bool SNIPModel::Init()
   {
   int col = 0;

   m_pOutputData = new FDataObj(42, 0);
   m_pOutputData->SetName(this->m_pSNLayer->m_name.c_str());
   m_pOutputData->SetLabel(col++, "Time");
   m_pOutputData->SetLabel(col++, "cycles");
   m_pOutputData->SetLabel(col++, "convergeIterations");
   m_pOutputData->SetLabel(col++, "nodeCount");
   m_pOutputData->SetLabel(col++, "edgeCount");
   m_pOutputData->SetLabel(col++, "nodeCountNLA");
   m_pOutputData->SetLabel(col++, "minNodeReactivity");
   m_pOutputData->SetLabel(col++, "meanNodeReactivity");
   m_pOutputData->SetLabel(col++, "maxNodeReactivity");
   m_pOutputData->SetLabel(col++, "minNodeInfluence");
   m_pOutputData->SetLabel(col++, "meanNodeInfluence");
   m_pOutputData->SetLabel(col++, "maxNodeInfluence");
   m_pOutputData->SetLabel(col++, "minEdgeTransEff");
   m_pOutputData->SetLabel(col++, "meanEdgeTransEff");
   m_pOutputData->SetLabel(col++, "maxEdgeTransEff");
   m_pOutputData->SetLabel(col++, "minEdgeInfluence");
   m_pOutputData->SetLabel(col++, "meanEdgeInfluence");
   m_pOutputData->SetLabel(col++, "maxEdgeInfluence");
   m_pOutputData->SetLabel(col++, "edgeDensity");
   m_pOutputData->SetLabel(col++, "totalEdgeSignalStrength");
   m_pOutputData->SetLabel(col++, "inputActivation");
   m_pOutputData->SetLabel(col++, "landscapeSignalInfluence");
   m_pOutputData->SetLabel(col++, "outputReactivity");
   m_pOutputData->SetLabel(col++, "meanLANodeReactivity");
   m_pOutputData->SetLabel(col++, "maxLANodeReactivity");
   m_pOutputData->SetLabel(col++, "signalContestedness");
   m_pOutputData->SetLabel(col++, "meanDegreeCentrality");
   m_pOutputData->SetLabel(col++, "meanBetweenessCentrality");
   m_pOutputData->SetLabel(col++, "meanClosenessCentrality");
   m_pOutputData->SetLabel(col++, "meanInfWtDegreeCentrality");
   m_pOutputData->SetLabel(col++, "meanInfWtBetweenessCentrality");
   m_pOutputData->SetLabel(col++, "meanInfWtClosenessCentrality");
   m_pOutputData->SetLabel(col++, "normCoordinators");
   m_pOutputData->SetLabel(col++, "normGatekeeper");
   m_pOutputData->SetLabel(col++, "normRepresentative");
   m_pOutputData->SetLabel(col++, "notLeaders");
   m_pOutputData->SetLabel(col++, "clusteringCoefficient");
   m_pOutputData->SetLabel(col++, "NLA_Influence");
   m_pOutputData->SetLabel(col++, "NLA_TransEff");
   m_pOutputData->SetLabel(col++, "normNLA_Influence");
   m_pOutputData->SetLabel(col++, "normNLA_TransEff");
   m_pOutputData->SetLabel(col++, "normEdgeSignalStrength");

   this->m_pSNLayer->m_pSNIP->AddOutputVar(this->m_pSNLayer->m_name.c_str(), m_pOutputData, "");

   return true;
   }


// new
bool SNIPModel::LoadSnipNetwork(LPCTSTR path, bool includeLandscapeActors)
   {
   CString _path(PathManager::GetPath(PM_PROJECT_DIR));
   _path += path;
   // read a JSON file
   std::ifstream input(_path);
   json j;
   input >> j;

   // interpret into this layer
   m_networkJSON = j["network"];

   this->m_name = m_networkJSON["name"].get<std::string>();
   this->m_description = m_networkJSON["description"].get<std::string>();
   int index = 0;
   for (const auto& t : m_networkJSON["traits"])
      {
      std::string trait = t;
      this->m_traitsLabels.push_back(trait);
      this->m_traitsMap[trait] = index++;
      }

   json &settings = m_networkJSON["settings"];

   json &autogenLandscapeSignals = settings["autogenerate_landscape_signals"];
   float autoGenFraction = autogenLandscapeSignals["fraction"];
   std::string autoGenBias = autogenLandscapeSignals["bias"];
   ASSERT(autoGenFraction < 0.0000001f);  // autogen landscape signals not currently supported

   json& autogenTransTimes = settings["autogenerate_transit_times"];
   this->m_autogenTransTimeMax = autogenTransTimes["max_transit_time"];
   this->m_autogenTransTimeBias = autogenTransTimes["bias"];

   json& _input = settings["input"];
   std::string inputType = _input["type"];
   float inputValue = _input["value"];

   std::string infSubmodel = settings["infSubmodel"];
   this->m_infSubmodelWt = settings["infSubmodelWt"].get<float>();
   this->m_aggInputSigma = settings["agg_input_sigma"].get<float>();
   this->m_transEffMax = settings["trans_eff_max"].get<float>();
   this->m_activationThreshold = settings["reactivity_threshold"].get<float>();
   this->m_activationSteepFactB = settings["reactivity_steepness_factor_B"].get<float>();
   //settings["node_size"];
   //settings["node_color"];
   //settings["node_label"];
   //settings["edge_size"];
   //settings["edge_color"];
   //settings["edge_label"];
   //settings["show_tips"];
   //settings["show_landscape_signal_edges"];
   //settings["layout"];
   //settings["zoom"];
   //settings["center"];

   this->m_infSubmodel = SNIP_INFMODEL_TYPE::IM_SIGNAL_SENDER_RECEIVER;
   if (infSubmodel == "signal_receiver")
      this->m_infSubmodel = SNIP_INFMODEL_TYPE::IM_SIGNAL_RECEIVER;
   else if (infSubmodel == "sender_receiver")
      this->m_infSubmodel = SNIP_INFMODEL_TYPE::IM_SENDER_RECEIVER;
   else if (infSubmodel == "trust")
      this->m_infSubmodel = SNIP_INFMODEL_TYPE::IM_TRUST;

   if (m_networkJSON.contains("settings"))
      this->LoadPreBuildSettings(m_networkJSON["settings"]);

   int traitsCount = (int)this->m_traitsLabels.size();
   float* _traits = new float[traitsCount];

   // build nodes
   for (const auto& node : m_networkJSON["nodes"])
      {
      std::string nname = node["name"];
      std::string ntype = node["type"];
      auto& ntraits = node["traits"];  // array
      // ignore x, y

      SNIP_NODETYPE nodeType = SNIP_NODETYPE::NT_NETWORK_ACTOR;
      if (ntype == "input")
         nodeType = SNIP_NODETYPE::NT_INPUT_SIGNAL;
      else if (ntype == "engager")
         nodeType = SNIP_NODETYPE::NT_ENGAGER;
      else if (ntype == "assessor")
         nodeType = SNIP_NODETYPE::NT_ASSESSOR;
      else if (ntype == "landscape")
         nodeType = SNIP_NODETYPE::NT_LANDSCAPE_ACTOR;

      for (int i = 0; i < traitsCount; i++)
         _traits[i] = ntraits[i];

      // add node to collection
      if ((nodeType != NT_LANDSCAPE_ACTOR || includeLandscapeActors))
         {
         SNNode* pNode = this->BuildNode(nodeType, nname.c_str(), _traits);

         // if input node, set its reactivity
         if (pNode->IsInputSignal())
            {
            this->m_pInputNode = pNode;
            //pNode->m_reactivity = this->GetInputLevel(0);
            }
         }
      // set node data???
      }

   delete[] _traits;

   // build edges
   int errorCount = 0;
   for (const auto& edge : m_networkJSON["edges"])
      {
      std::string from = edge["from"].get<std::string>();
      std::string to = edge["to"].get<std::string>();
      float trust = edge["trust"].get<float>();

      SNNode* pFromNode = this->m_pSNLayer->FindNode(from.c_str());
      SNNode* pToNode = this->m_pSNLayer->FindNode(to.c_str());
      
      if (pFromNode == nullptr)
         {
         if (errorCount < 100)
            {
            string msg = std::format("Unable to find FROM node %s when building network!  This link will be ignored", from.c_str());
            Report::LogWarning(msg.c_str());
            errorCount++;
            }
         continue;
         }
      
      if (pToNode == nullptr)
         {
         if (errorCount < 100)
            {
            string msg = std::format("Unable to find TO node %s when building network!  This link will be ignored", to.c_str());
            Report::LogWarning(msg.c_str());
            errorCount++;
            }
         continue;
         }
      
      //ASSERT(pFromNode != nullptr && pToNode != nullptr);
      int transTime = 1; // ???
      this->BuildEdge(pFromNode, pToNode, transTime, trust, nullptr);   // note: no traits

      auto bidirectional = edge.find("bidirectional");
      if (bidirectional != edge.end() && bidirectional[0].get<bool>() == true)
         this->BuildEdge(pToNode, pFromNode, transTime, trust, nullptr);
      }

   this->LoadPostBuildSettings(m_networkJSON["settings"]);

   this->AddAutogenInputEdges();
   this->SetEdgeTransitTimes();
   this->ResetNetwork();
   this->GenerateInputArray();
   this->SetInputNodeReactivity(1.0f); ///???
   this->m_maxNodeDegree = this->GetMaxDegree(false);

   int nodeCount = GetNodeCount();
   int edgeCount = GetEdgeCount();
   int assessorCount = GetNodeCount(NT_ASSESSOR);
   int engagerCount = GetNodeCount(NT_ENGAGER);
   int laCount = GetNodeCount(NT_LANDSCAPE_ACTOR);
   CString msg;
   msg.Format("Loaded network %s (%i nodes, %i edges, %i assessors, %i engagers, %i landscape actors)", this->m_pSNLayer->m_name.c_str(), nodeCount, edgeCount, assessorCount, engagerCount, laCount);
   Report::LogInfo(msg);

   this->SolveEqNetwork(0);

   //this->InitSimulation();

   this->UpdateNetworkStats();
   return true;
   }


SNNode* SNIPModel::BuildNode(SNIP_NODETYPE nodeType, LPCTSTR name, float* traits)
   {
   SNNode* pNode = new SNNode(nodeType);

   pNode->m_name = name;
   pNode->m_pLayer = this->m_pSNLayer;   // snipModel: this,
   pNode->m_index = this->m_pSNLayer->m_nextNodeIndex;
   pNode->m_id = std::format("n{}", this->m_pSNLayer->m_nextNodeIndex++);
      //label : '',
   pNode->m_state = STATE_ACTIVE;  // : STATE_ACTIVE,
   pNode->m_reactivity = 0;  // : reactivity,
   pNode->m_sumInfs = 0;     // : 0,
   pNode->m_srTotal = 0;     // : 0,
   pNode->m_influence = 0;   // : 0,
   pNode->m_reactivityMultiplier = 1; // : 1,
   //pNode->m_reactivityHistory = ;    // : [reactivity] ,
   //pNode->m_reactivityMovAvg;     // : 0,

   // copy traits
   int traitsCount = (int)this->m_traitsLabels.size();
   ASSERT(pNode->m_traits.IsEmpty());

   for (int i = 0; i < traitsCount; i++)
      pNode->m_traits.Add(traits[i]);

   pNode->m_name = name;
   ////pNode->m_landscapeGoal= landscapeGoal;
   if (nodeType == NT_INPUT_SIGNAL)
      this->m_pInputNode = pNode;

   else if (nodeType == NT_LANDSCAPE_ACTOR)
      this->m_pSNLayer->m_outputNodeCount++;

   this->m_pSNLayer->m_nodes.Add(pNode); //SNNode( nodeType ) );
   this->m_pSNLayer->m_nodeMap[string((LPCTSTR) pNode->m_name)] = pNode;

   return pNode;
   }


SNEdge* SNIPModel::BuildEdge(SNNode* pFromNode, SNNode* pToNode, int transTime, float trust, float* traits)
   {
   ASSERT(pFromNode != nullptr && pToNode != nullptr);

   SNEdge* pEdge = new SNEdge(pFromNode, pToNode);

   pEdge->m_transTime = transTime;
   pEdge->m_name = pFromNode->m_name + "->" + pToNode->m_name;
   pEdge->m_trust = trust;
   pEdge->m_edgeType = ET_NETWORK;

   if (pFromNode->IsInputSignal())
      {
      pEdge->m_edgeType = ET_INPUT;
      pEdge->AddTraitsFrom(pFromNode);  // only on input edges?
      }

   pEdge->m_id = std::format("e{}", this->m_pSNLayer->m_nextEdgeIndex++);

   pEdge->m_signalStrength = 0;   // [-1,1]
   //pEdge->m_signalTraits : signalTraits,  // traits vector for current signal, or null if inactive
   pEdge->m_transEff = 0;  //??? : transEff,
   pEdge->m_transEffSender = 0;
   pEdge->m_transEffSignal = 0;
   pEdge->m_transTime = transTime;
   pEdge->m_activeCycles = 0;
   pEdge->m_state = STATE_ACTIVE;
   pEdge->m_influence = 0;

   pFromNode->m_outEdges.Add(pEdge);
   pToNode->m_inEdges.Add(pEdge);

   if (traits != nullptr)
      pEdge->AddTraitsFromArray(traits, (int) this->m_traitsLabels.size());

   this->m_pSNLayer->m_edges.Add(pEdge);

   return pEdge;
   }


SNNode* SNLayer::FindNode(LPCTSTR name)
   {
   //int nodeCount = this->GetNodeCount();
   //for (int i = 0; i < nodeCount; i++)
   //   {
   //   if (this->GetNode(i)->m_name.CompareNoCase(name) == 0)
   //      return this->GetNode(i);
   //   }
   map< string, SNNode* >::iterator itr;

   SNNode* node1 = nullptr;
   int count = 0;
   for (itr = m_nodeMap.begin(); itr != m_nodeMap.end(); ++itr) {
      count++;
      if (itr->first == "Landscape Signal")
         node1 = itr->second;
      }

   try {
      SNNode* node = m_nodeMap.at(name);
      return node;
      }
   catch(exception e) {
      CString msg;
      msg.Format("Exception encountered finding node %s", name);
      Report::LogError(msg);
      }
   return nullptr;
   }


bool SNLayer::RemoveNode(SNNode* pNode)
   {
   // remove from node array
   m_nodes.Remove(pNode);  // also deletes node

   // remove from nodeMap
   for (std::map<std::string, SNNode*>::iterator it = m_nodeMap.begin(); it != m_nodeMap.end();)
      {
      if ((it->second) == pNode)
         {
         it = m_nodeMap.erase(it);
         break;
         }
      else
         it++;
      }
   // remove any edges that point from/to this node.
   for (int i = 0; i < (int)m_edges.GetSize(); i++)
      {
      SNEdge* pEdge = m_edges[i];
      if (pEdge->m_pFromNode == pNode)
         {
         m_edges.Remove(pEdge);
         for ( int j=0; j < (int) pEdge->m_pFromNode->m_outEdges.GetSize(); j++)
            if (pEdge->m_pFromNode->m_outEdges[j] == pEdge)
               {
               pEdge->m_pFromNode->m_outEdges.RemoveAt(j);
               break;
               }   
                  
         for (int j = 0; j < (int)pEdge->m_pToNode->m_inEdges.GetSize(); j++)
            if (pEdge->m_pToNode->m_inEdges[j] == pEdge)
               {
               pEdge->m_pToNode->m_inEdges.RemoveAt(j);
               break;
               }

         i--;
         }
      }

   return true;
   }



int SNIPModel::FindNodesFromTrait(string &trait, SNIP_NODETYPE ntype, float threshold, vector<SNNode*>& nodes)
   {
   nodes.clear();

   for (int i = 0; i < GetNodeCount(); i++)
      {
      SNNode* pNode = GetNode(i);
      if (pNode->m_nodeType == ntype && this->FindTraitValue(pNode, trait) > threshold )
         nodes.push_back(pNode);
      }
   return (int) nodes.size();
   }



float SNIPModel::FindTraitValue(TraitContainer* pTC, string &name)
   {
   try {
      int index = m_traitsMap[name];
      ASSERT(index < m_traitsLabels.size());
      return pTC->m_traits[index];
      }
   catch (exception) {
      string msg = format("Error: Unable to find trait value '{}'", name);
      Report::ErrorMsg(msg.c_str());
      }
   return 0;
   }






void SNIPModel::AddAutogenInputEdges()
   {
   Report::LogWarning("SNIP: AddAutogenInputEdges not supported!");
   /*
   if (this.autogenFraction > 0) {
      // add landscape signal edges
      let sourceNode = this.cy.nodes(INPUT_SIGNAL)[0];
      let sourceID = sourceNode.data('id');
      let sourceTraits = sourceNode.data('traits');
      let threshold = 0;
      let index = 0;
      let snodes = null;
      let targetID = null;
      switch (this.autogenBias) {
         case "influence":
         case "reactivity":
            snodes = this.cy.nodes().sort(function(a, b) {
               return b.data(this.autogenBias) - a.data(this.autogenBias);  // decreasing order
               });

            threshold = snodes.length * this.autogenFraction;
            for (index = 0; index < threshold; index++) {
               targetID = snodes[index].data('id');
               this.AddAutogenInputEdge('e' + this.nextEdgeIndex, sourceID, targetID, sourceTraits, this.actorValue);
               }
            break;

         case "degree":
            snodes = this.cy.nodes().sort(function(a, b) {
               return b.degree(false) - a.degree(false);  // decreasing order
               });

            threshold = snodes.length * this.autogenFraction;
            for (index = 0; index < threshold; index++) {
               targetID = snodes[index].data('id');
               this.AddAutogenInputEdge('e' + this.nextEdgeIndex, sourceID, targetID, sourceTraits, this.actorValue);
               }
            break;

         case "random":
            this.cy.nodes().forEach(function(node, index) {
               let rand = this.randGenerator();
               if (rand < this.autogenFraction) {
                  if (index != = nodeCount - 1) {  // skip last node (self) ///??????????
                     targetID = this.cy.nodes().data('id'); // = 'n' + index;
                     this.AddAutogenInputEdge('e' + this.nextEdgeIndex, sourceID, targetID, sourceTraits, this.actorValue);
                     }
                  }
               });
            break;
         }
      } */
   return;
   }

// only landscape edges for now
void SNIPModel::AddAutogenInputEdge(std::string id, SNNode* pSourceNode, SNNode* pTargetNode, float sourceTraits[], float actorValue)
   {/*
   Report::LogWarning("SNIP: AddAutogenInputEdge not supported!");
   
   this.cy.add({
       group: 'edges',
       data : {
           id: id,   // FromIndex_ToIndex
           name : 'Landscape Signal',
           label : '',
           source : sourceID,

           target : targetID,
           signalStrength : 0,   // [-1,1]
           signalTraits : sourceTraits,  // traits vector for current signal, or null if inactive
           transEff : actorValue,
           transEffSender : 0,
           transEffSignal : actorValue,
           transTime : 0,
           activeCycles : 0,
           state : STATE_ACTIVE,
           influence : actorValue,
           width : 0.5,
           lineColor : 'lightblue',
           type : ET_INPUT,
           watch : 0,
           opacity : 1,
           show : 0
       }
      });
   this.nextEdgeIndex++;
   */
   }

void SNIPModel::SetEdgeTransitTimes()
   {
   if (this->m_autogenTransTimeMax > 0)
      {
      // add landscape edges (landscape node is last node added)
      if (this->m_autogenTransTimeBias == "influence" || this->m_autogenTransTimeBias == "transEff")
         {
         for (int i = 0; i < this->GetEdgeCount(); i++)
            {
            SNEdge* pEdge = this->GetEdge(i);
            if (pEdge->m_edgeType == SNIP_EDGE_TYPE::ET_NETWORK)    // ignore landscape edges
               {
               //????????????? 
               //float scalar = this->m_autogenTransTimeBias;    // [-1,1] for both transEff, influence (NOTE: NOT QUITE CORRECT FOR TRANSEFF)
               float scalar = 1;
               scalar = (scalar + 1) / 2.0f;       // [0,1]
               scalar = 1 - scalar;            // [1,0]
               float tt = std::round(scalar * this->m_autogenTransTimeMax);
               pEdge->m_transTime = int(tt);
               }
            }
         }
      else if (this->m_autogenTransTimeBias == "random")
         {
         for (int i = 0; i < this->GetEdgeCount(); i++)
            {
            SNEdge* pEdge = this->GetEdge(i);
            if (pEdge->m_edgeType == SNIP_EDGE_TYPE::ET_NETWORK)    // ignore landscape edges
               {
               float rand = (float)this->m_randShuffle.RandValue();  // [0,1]
               float tt = std::round(rand * this->m_autogenTransTimeMax);
               pEdge->m_transTime = int(tt);
               }
            }
         }
      }
   }

void SNIPModel::GenerateInputArray()
   {
   // populate the model's netInputs vector
   m_xs.resize(this->m_cycles);
   float x = -1;
   std::generate(m_xs.begin(), m_xs.end(), [&] { return x += 1; });

   this->m_netInputs.resize(this->m_cycles);

   switch (this->m_inputType)
      {
      case I_CONSTANT:    // constant
         std::generate(m_netInputs.begin(), m_netInputs.end(), [&] { return this->m_k; });
         break;

      case I_CONSTWSTOP:  // constant with stop
      {
      for (int i = 0; i < m_xs.size(); i++)
         m_netInputs[i] = (m_xs[i] < this->m_stop) ? this->m_k1 : 0;
      }
      break;

      case I_SINESOIDAL:  // sinesoidal
      {
      for (int i = 0; i < m_xs.size(); i++)
         m_netInputs[i] = 0.5f + (this->m_amp * sin((2 * PI * m_xs[i] / this->m_period) + this->m_phase));
      }
      break;

      case I_RANDOM:  // random
         //this.netInputs = xs.map(function(x, index, array) { return Math.random(); }); // [0,1] 
         break;

      case I_TRACKOUTPUT: // track output
         std::generate(m_netInputs.begin(), m_netInputs.end(), [&] { return 0.0f; });
         break;

      case I_ENVISION_SIGNAL:
         break;
      }
   }

void SNIPModel::ResetNetwork()
   {
   // initialize all participants (except landscape signals)
   int nodeCount = 0;
   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);

      // apply to ANY_ACTOR (but not input nodes)
      if (pNode->IsInputSignal() == false )
         {
         pNode->m_state = STATE_ACTIVE;  // the node is initially active
         pNode->m_reactivity = 0;
         pNode->m_influence = 0;
         pNode->m_sumInfs = 0;
         pNode->m_srTotal = 0;
         //pNode->m_reactivityHistory', [0]);      //// ???????????
         nodeCount++;
         }
      }

   // ditto with edges
   int edgeCount = 0;
   //let traits = this.inputNode.data('traits');
   for (int i = 0; i < this->GetEdgeCount(); i++)
      {
      SNEdge* pEdge = this->GetEdge(i);

      pEdge->m_state = STATE_ACTIVE;  // the edge is initially inactive
      pEdge->m_activeCycles = 0;
      pEdge->m_transEff = 0;
      pEdge->m_influence = 0;
      pEdge->m_signalStrength = 0;
      //pEdge->m_signalTraits = 0;
      pEdge->AddTraitsFrom(this->m_pInputNode);

      if (pEdge->m_edgeType == ET_INPUT)
         pEdge->m_signalStrength = 1.0f;

      edgeCount++;
      }

   //this->GenerateInputArray();
   //this->m_pInputNode->m_reactivity = 0;  // this->GetInputLevel(0);
   }



int SNIPModel::RunSimulation(bool initialize, bool step)
   {
   if (initialize)
      this->InitSimulation();

   // set up progress bar
   if (this->m_currentCycle == 0)
      {
      //   var $divProgressBar = $('#divProgressBar');
      //   if (divProgressBar) {
      //      $divProgressBar.progress('set active');
      //      $divProgressBar.progress('reset');
      //      $divProgressBar.progress('set total', this.cycles);
      //      $('#divProgressText').text("1/" + this.cycles);
      //      $('#divProgress').show();
      //      }

      //this.UpdateWatchLists();

      // set up array to store outputs
      //this.netOutputs = Array(this.cycles).fill(0);

      //this->UpdateNetworkStats();
      //this->netReport.push(this.netStats);
      }

   // if "step" model, call RunCycle() with repeatUntilDone=false; otherwise, true
   // Note that RunCycle will recurse

   RunCycle(this->m_currentCycle, !step);

   // Simulation is complete for, do a little reporting
   ///////float meanLAReactivity = 0;
   ///////int laCount = 0;
   ///////for (int i = 0; i < this->GetNodeCount(); i++)
   ///////   {
   ///////   SNNode* pNode = this->GetNode(i);
   ///////   if (pNode->IsLandscapeActor())
   ///////      {
   ///////      meanLAReactivity += pNode->m_reactivity;
   ///////      laCount++;
   ///////      //CString msg;
   ///////      //msg.Format("Landscape Actor Node %s: Reactivity = %.2f, Influence = %.2f", pNode->m_name, pNode->m_reactivity, pNode->m_influence);
   ///////      //Report::LogInfo(msg);
   ///////      }
   ///////   }
   ///////
   ///////meanLAReactivity /= laCount;
   ///////CString msg;
   ///////msg.Format("Landscape Actor Node: Mean Reactivity=%.2f from %i actors", meanLAReactivity, laCount);
   ///////Report::LogInfo(msg);

   for (int i = 0; i < this->GetEdgeCount(); i++)
      {
      SNEdge* pEdge = this->GetEdge(i);
      //if (pNode->m_nodeType == NT_LANDSCAPE_ACTOR)
      //   {
      //CString msg;
      //msg.Format("   Edge %s: Active=%i, Trans Eff=%.2f, Signal Strength=%.2f, Influence=%.2f", pEdge->m_name.c_str(), (pEdge->IsActive() ? 1 : 0), pEdge->m_transEff, pEdge->m_influence);
      //Report::LogInfo(msg);
      //   }
      }

   float inputReactivity = this->GetInputNode()->m_reactivity;
   float meanEngagerReactivity = this->GetMeanReactivity(NT_ENGAGER);
   float meanLAReactivity = this->GetMeanReactivity(NT_LANDSCAPE_ACTOR);
   float netReactivity = this->GetMeanReactivity();

   CString msg;
   msg.Format("SNIP simulation complete: Input Signal=%.2f, Mean Engager reactivity=%.2f, Mean LA reactivity=%.2f, Mean Network Reactivity=%.2f",
      inputReactivity, meanEngagerReactivity, meanLAReactivity, netReactivity);
   Report::LogInfo(msg);
   return 0;
   }


void SNIPModel::InitSimulation()
   {
   this->ResetNetwork();  // zero out nodes, edges, makes everything ACTIVE

   this->GenerateInputArray();
   //this->netOutputs = Array(this.cycles).fill(1.0);
   //this->netReport = [];
   //this->watchReport = [];

   // inactivate  all actors (input nodes stay active)
   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);

      switch (pNode->m_nodeType)
         {
         case NT_ASSESSOR:
         case NT_NETWORK_ACTOR:
         case NT_ENGAGER:
         case NT_LANDSCAPE_ACTOR:
            pNode->m_state = STATE_INACTIVE;  // the node is initially inactive
            pNode->m_reactivity = 0;
            pNode->m_influence = 0;
            break;

         case NT_INPUT_SIGNAL:
            pNode->m_state = STATE_ACTIVE;  // the node is iniital inactive
            pNode->m_reactivity = GetInputLevel(0);
            pNode->m_influence = 0;
         }
      }

   for (int i = 0; i < this->GetEdgeCount(); i++)
      {
      SNEdge* pEdge = this->GetEdge(i);
      pEdge->m_signalStrength = 0;
      pEdge->m_state = STATE_INACTIVE;
      //pEdge->ClearTraits();

      //if (pEdge->m_edgeType == ET_INPUT)
      //   {
      //   // any input signal edges - set:
      //   // 1) 'signalStrength' attribute to the input's reactivity
      //   // 2) 'signalTraits' attribute to the inputs' traits array
      //   pEdge->m_signalStrength = pEdge->m_pFromNode->m_reactivity;
      //   pEdge->AddTraitsFrom(pEdge->m_pFromNode);
      //   pEdge->m_state = STATE_ACTIVE;
      //   }
      }

   this->m_currentCycle = 0;
   }



void SNIPModel::RunCycle(int cycle, bool repeatUntilDone)
   {
   if (cycle >= 0)
      this->m_currentCycle = cycle;

   if (this->m_currentCycle >= this->m_cycles) // end of simulation?
      {
      //let $divProgressText = $('#divProgressText');

      //if ($divProgressText.length > 0) {
      //   $divProgressText.text("Simulation Complete after " + snipModel.cycles + " cycles");
      //   setTimeout(HideProgress, 3000);
      //   $('#divOutputs').show();
      //   }
      //this->m_runCount += 1;
      this->m_currentCycle = 0;
      return;
      }

   // first times for this simulation?
   if (cycle <= 0)
      {
      //snipModel.UpdateWatchLists();
      //$('#ulWatchListCalcs').empty();
      }

   // set an inputs to the level called for this cycles
   float inputLevel = this->GetInputLevel(cycle);  // this is the landscape signal.

   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);

      if (pNode->IsInputSignal())
         pNode->m_reactivity = inputLevel;
      }

   //ActivateNetwork(cycle);
   float stepDelta = this->PropagateSignal(cycle);

   ////this->UpdateNetworkStats();

   // update UI
   //let $divProgressText = $('#divProgressText');
   //if ($divProgressText.length > 0) {
   //   $('#divProgressText').text("Run " + snipModel.runCount + " Cycle " + (snipModel.cycle + 1) + ": Network converged in " + snipModel.convergeIterations + " iterations.  Final Delta: " + stepDelta.toPrecision(2));
   //   $('#divProgressBar').progress('set progress', cycle + 1);
   //   }
   ////??????   UpdateUI(snipModel);
   ////??????   UpdateNetworkStatsDisplay(snipModel);

   if (cycle >= 0)
      {
      // remember output
      //float output = this->GetOutputLevel();
      //this->netOutputs[snipModel.cycle] = output;
      ////???????snipModel.UpdateOutputCharts(cycle);
      }

   //snipModel.netReport.push(snipModel.netStats);
   this->m_currentCycle++;

   if (repeatUntilDone == true)
      {
      if (cycle < this->m_cycles)
         RunCycle(cycle + 1, true);
      }
   }


// Part of RunSimulation() - runs for one cycle
float SNIPModel::PropagateSignal(int cycle)
   {
   // basic idea - starting at the landscape signal (cycle 0), each successive cycle
   // propagates the signal throughout the network, neighbor to neighbor

   // for all the nodes that are active, get the set of neighbors that
   // are not (yet) active, and activatate them if appropriate.
   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);

      if (pNode->IsActive())
         {
         // for all edges neighboring this ACTIVE node, if the 
         // edge is currently inactive and pointing away from this node,
         // set it's state to 'activating'  This mean it is preparing to propagate
         // a signal when the edges 'travel time' has elapsed.
         int edgeCount = (int)pNode->m_outEdges.GetSize();
         for (int j = 0; j < edgeCount; j++)
            {
            SNEdge* pEdge = pNode->m_outEdges[j];

            if (pEdge->m_state == STATE_INACTIVE)
               {
               // only include inactive edges that point AWAY from this node
               if (pEdge->Source() == pNode)
                  {
                  // check to see if it needs to be activated, meaning
                  // it has been in an activing state for sufficient cycles
                  pEdge->m_state = STATE_ACTIVATING;  // the edge is activating
                  //CString msg;
                  //msg.Format("   Activing edge %s", pEdge->m_name.c_str());
                  //Report::LogInfo(msg);
                  // update active cycles since edge is connected to active node
                  //pEdge->m_activeCycles++;
                  }
               }
            }  // end of: for (each neighbor edge)
         }  // end of: if ( pNode is active)
      }  // end of: for each node

      // activating edges identified, solve the network for all
      // ACTIVE and ACTIVATING nodes/edges
   float stepDelta = this->SolveEqNetwork(0);
   //console.log("StepDelta in PropSig()", stepDelta)

   // update activeCycles for all active and activating edges
   int edgeCount = GetEdgeCount();

   for (int j = 0; j < edgeCount; j++)
      {
      SNEdge* pEdge = this->GetEdge(j);

      int activeCycles = pEdge->m_activeCycles;
      if (pEdge->IsActive() || pEdge->IsActivating())
         pEdge->m_activeCycles = activeCycles + 1;

      if ((pEdge->m_state == STATE_ACTIVATING) && ((activeCycles + 1) >= pEdge->m_transTime))
         {
         pEdge->m_state = STATE_ACTIVE;
         // set signal strength for the activating edge based on upstream signal minus degradation
         // signal strength is based on the reactivity of the upstream node
         float srcReactivity = pEdge->m_pFromNode->m_reactivity;    // what if it's an input signal?
         pEdge->m_signalStrength = srcReactivity * (1 - this->m_kD);    // confirm for input signals!!!!
         pEdge->AddTraitsFrom(this->m_pInputNode);   // establish edge signal traits ADD INPUT NODE
         if (pEdge->m_pToNode->m_state != STATE_ACTIVE)
            {
            pEdge->m_pToNode->m_state = STATE_ACTIVE;  // set the target node to active as well
            //CString msg;
            //msg.Format("   Node %s activated", pEdge->m_pToNode->m_name);
            //TRACE(msg); // Report::LogInfo(msg);
            }
         }
      }

   // update node reactivity histories
   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);

      if (pNode->IsActive())
         {
         if (pNode->IsInputSignal() == false)
            {
            //float r = pNode->m_reactivity;
            //float rs = pNode->m_reactivityHistory');
            //rs.push(r);
            //const period = 5;
            //if (rs.length > period)
            //   rs.splice(0, 1);
            //var sum = 0;
            //for (var i in rs)
            //   sum += rs[i];
            //var n = period;
            //if (rs.length < period)
            //   n = rs.length;
            //
            //node.data('reactivityMovAvg', sum / n);
            //node.data('reactivityHistory', rs);
            }
         }
      }

   // done updating node activations.  If this network is adaptive, iterate through connections, 
   // adjusting weights according to similarity of activation of connected nodes.  
   // Threshold is the max tolerable difference between activation level of 
   // orgs(representing diff in orgs effort / investment)
   // 
   // Learning rule: Edges change in response to nodes becoming simultanously activated, mitigated by an
   //    speed factor.  To calculate the weight change for and edge, we compute the delta of the two nodes'
   //    activation levels.  For small differences (indicating similar responses to their collective inputs),
   //    we increase the weights proportional to the inverse of the delta.  For larger differences, 
   //    we decrease the weights, proportional to the delta
   // The difference between activation levels  for each edge, modify the weight proptionally by computing a delta where
   //   delta = (threshold - fabs( fromActivation - toActivation)) * ((fromActivation + toActivation) / 2) / (threshold);

   //if (this->m_adaptive)
   //   {
   //   //ApplyLearningRule(this);
   //   }

   return stepDelta;
   }



float SNIPModel::SolveEqNetwork(float  bias)
   {
   // selector = ACTIVE; ANY_ACTOR
   // Solves the network to steady state.  starting condition is whatever state the 
   // network is currently in - this function does not do the initialization.
   int nodeCount = this->GetActiveNodeCount();
   int edgeCount = this->GetActiveEdgeCount();
   int laCount = this->GetActiveNodeCount(NT_LANDSCAPE_ACTOR);

   CString msg;
   msg.Format("Solving equilibrium network for cycle %i (%i active nodes, %i active edges, %i landscape actors), Input=%.2f",
                  this->m_currentCycle, nodeCount, edgeCount, laCount, m_pInputNode->m_reactivity);
   Report::LogInfo(msg);

   // set input nodes to their appropriate values
   float stepDelta = 9999999.0f;
   float deltaTolerance = 0.0001f * (nodeCount - 1);  // -1 since last node is landscape signal
   const int maxIterations = 400;

   // update the influence transmission efficiencies for each edge.  Not that this are independent of reactivities
   this->ComputeEdgeTransEffs();

   // iteratively solve the network but letting it relax to steady state.
   this->m_convergeIterations = 0;

   while (stepDelta > deltaTolerance && this->m_convergeIterations < maxIterations)
      {
      // update all edges so that signal strength, used in the singal-reviever calculatons are equal
      //this.cy.edges().forEach(function(edge) {
      edgeCount = GetEdgeCount();
      for (int i = 0; i < edgeCount; i++)
         {
         // set signal strength for the activating edge based on upstream signal minus degradation
         // signal strength is based on the reactivity of the upstream node
         SNEdge* pEdge = this->GetEdge(i);
         SNNode* pSrcNode = pEdge->Source();
         float srcReactivity = pSrcNode->m_reactivity;    // what if it's an input signal?
         pEdge->m_signalStrength = srcReactivity; // *(1 - this->m_kD);    // confirm for input signals!!!!
         //// NOTE TODO: pEdge->m_signalTraits = this->m_pInputNode->m_traits;   // establish edge signal traits ADD INPUT NODE
         }

      //WatchMsg(null, "Run:" + runCount + ", Iteration:" + convergeIterations);
      // iterate though each node (excluding input), calculating an activation for that node
      // we will repeat this until the accumulate delta goes below a threshold value.
      this->ComputeEdgeInfluences();  // note that these are dependent on reactivitities

      stepDelta = 0.0;
      //this.cy.nodes(ACTIVE).forEach(function(node) {
      nodeCount = this->GetNodeCount();
      for (int i = 0; i < nodeCount; i++)
         {
         SNNode* pNode = this->GetNode(i);
         if (pNode->IsActive() && pNode->IsInputSignal() == false)
            {  // don't compute reactivities for input nodes 
            float oldReactivity = pNode->m_reactivity;   //  [-1,1]
            float newReactivity = this->ActivateNode(pNode, bias); // this updates the node's 'reactivity', 'sumInf', 'srTotal' data
            float delta = newReactivity - oldReactivity;

            //console.log(node.data('name'), " old: ", oldReactivity, " new: ", newReactivity)
            stepDelta += delta * delta;   // change from last cycle?
            }
         }

      // update reactivities
      //cy.nodes(selector).forEach(function (node) {
      //    var newReactivity = node.data('updatedReactivity');
      //    // apply decay
      //    //newReactivity *= (1 - reactivityDecayRate);
      //    node.data('reactivity', newReactivity);
      //});

      //WatchMsg(null, "--Delta this iteration: " + stepDelta.toPrecision(3)); //Fixed(5));
      this->m_convergeIterations++;
      }   // end of: while(network not converged)

   // update node influence levels
   this->m_maxNodeInfluence = 0.1f;
   for (int i = 0; i < nodeCount; i++)
      {
      SNNode* pNode = this->GetNode(i);
      if (pNode->IsActive())
         {
         float influence = 0;
         int count = (int)pNode->m_outEdges.GetSize();
         for (int j = 0; j < count; j++)
            influence += pNode->m_outEdges[j]->m_influence;

         pNode->m_influence = influence;
         if (influence > this->m_maxNodeInfluence) ////// && node.data('type') !== 0)  // exclude signals
            this->m_maxNodeInfluence = influence;
         }
      }

   msg.Format("Converged after %i iterations, Max node influence=%.2f, Mean LA Reactivity=%.2f", this->m_convergeIterations, this->m_maxNodeInfluence, this->GetMeanReactivity(NT_LANDSCAPE_ACTOR));
   Report::StatusMsg(msg);

   // report any watch outputs
   //if (this.OnWatchMsg != = null) {
   //   this.cy.nodes('[?watch]').forEach(function(node) {
   //      _this.OnWatchMsg(node, "--Node " + node.data('id') + ": sumInfs=" + node.data('sumInfs').toFixed(3) +
   //         ",  srTotal=" + node.data('srTotal').toFixed(3) + ", reactivity=" + newReactivity.toFixed(3) + ", delta=" + delta.toFixed(3));
   //      });
   //
   //   this.cy.edges('[?watch]').forEach(function(edge) {
   //      this.OnWatchMsg(edge, "--Edge " + edge.data('id') + ": transEff=" + edge.data('transEff').toFixed(3)
   //         + ", transEffSender=" + edge.data('transEffSender').toFixed(3)
   //         + ", transEffSignal=" + edge.data('transEffSignal').toFixed(3)
   //         + ", influence=" + edge.data('transEffSignal').toFixed(3)
   //         + ", signalStrength=" + edge.data('signalStrength').toFixed(3));
   //
   //      });
   //   }
   //
   //console.log("Show StepDelta: ", stepDelta)

   return stepDelta;
   }


//------------------------------------------------------- //
//---------------- Influence Submodel ------------------- //
//------------------------------------------------------- //

void SNIPModel::ComputeEdgeTransEffs()   // NOTE: Only need to do active edges
   {
   for (int i = 0; i < this->GetEdgeCount(); i++)
      {
      SNEdge* pEdge = this->GetEdge(i);

      if (pEdge->m_state == STATE_ACTIVE)  ////??????
         {
         if (this->m_infSubmodel == IM_SENDER_RECEIVER
            || this->m_infSubmodel == IM_SIGNAL_RECEIVER
            || this->m_infSubmodel == IM_SIGNAL_SENDER_RECEIVER)
            {
            // transmission efficiency depends on the current network model:
            // IM_SENDER_RECEIVER - uses sender node traits
            // IM_SIGNAL_RECEIVER - uses edge signal traits
            // IM_SIGNAL_SENDER_RECEIVER - uses both
            // IM_TRUST - uses single trust measure as edge trans eff
            float transEff = 0;
            SNNode* pSource = pEdge->Source();   // sender node
            SNNode* pTarget = pEdge->Target();   // receiver node
            //float srcType = pTarget->m_nodeType;
            float transEffSignal = 0;
            float transEffSender = 0;
            //float sTraits = [];  // source/signal traits
            //float tTraits = target.data('traits');  // receiver node

            // next, we will calulate two transmission efficiencies,
            // 1) based on relationship between receiver traits and signal traits
            // 2) based on relationship between receiver traits and sender traits.
            // We assume all input edge traits are initialized to the input
            // message traits

            // compute similarity between signal and receiver traits,
            // i.e. the edges 'signalTraits' and the receiver nodes 'traits' array
            float similarity = pEdge->ComputeSimilarity(pTarget);
            //sTraits = edge.data('signalTraits');
            //similarity = this.ComputeSimilarity(sTraits, tTraits);
            transEffSignal = this->m_transEffAlpha + this->m_transEffBetaTraits0 * similarity;   // [0,1]

            similarity = pSource->ComputeSimilarity(pTarget);
            //sTraits = source.data('traits');
            //similarity = this.ComputeSimilarity(sTraits, tTraits);
            transEffSender = this->m_transEffAlpha + this->m_transEffBetaTraits0 * similarity;   // [0,1]

            switch (this->m_infSubmodel) {
               case IM_SIGNAL_RECEIVER:
                  transEff = transEffSignal;
                  break;

               case IM_SENDER_RECEIVER:
                  transEff = transEffSender;
                  break;

               case IM_SIGNAL_SENDER_RECEIVER:
                  transEff = (this->m_infSubmodelWt * transEffSender) + ((1 - this->m_infSubmodelWt) * transEffSignal);
                  break;
               }

            //transEff *= edge.data('transEffMultiplier');
            pEdge->m_transEff = transEff;
            pEdge->m_transEffSignal = transEffSignal;
            pEdge->m_transEffSender = transEffSender;
            }
         else // IM_TRUST
            pEdge->m_transEff = pEdge->m_trust;  // nothing required
         }
      }
   }

// The following two functions compute the influence that flows along each edge connecting two actors.
// Compute edge weights (influence flows) by running the influence submodel on each connecting edge.
// These function assume:
//   1) edge transEff data is current
//   2) node reactivity data is current

void SNIPModel::ComputeEdgeInfluences()
   {
   for (int i = 0; i < GetEdgeCount(); i++)
      {
      SNEdge* pEdge = this->GetEdge(i);

      if (pEdge->m_state == STATE_ACTIVE)  ////??????
         {
         float transEff = pEdge->m_transEff;
         float srcNode = 0;
         float srcReactivity = 0;
         float influence = 0;

         //console.log("sub", this.infSubmodel)
         //console.log("transEff", edge.data('transEff'))

         switch (this->m_infSubmodel)
            {
            case IM_SIGNAL_RECEIVER:
            case IM_TRUST:
               // influence is the signal strength of the edge * transission efficiency
               influence = pEdge->m_signalStrength * transEff;
               break;

            case IM_SENDER_RECEIVER:
            {
            // influence is the reactivity of the source * transmission efficiency
            SNNode* pSrcNode = pEdge->Source();
            srcReactivity = pSrcNode->m_reactivity;
            influence = transEff * srcReactivity;
            }
            break;

            case IM_SIGNAL_SENDER_RECEIVER:
            {
            // blend the above
            SNNode* pSrcNode = pEdge->Source();
            //console.log(edge.data('transEffSender'), edge.data('transEffSignal'), "transeffs, sender - signal")
            float influenceSender = pSrcNode->m_reactivity * pEdge->m_transEffSender;
            float influenceSignal = pEdge->m_signalStrength * pEdge->m_transEffSignal;
            //console.log(influenceSender, influenceSignal, "calculate influece")

            influence = (this->m_infSubmodelWt * influenceSender) + ((1 - this->m_infSubmodelWt) * influenceSignal);
            }
            break;
            }

         pEdge->m_influence = influence;
         //if (this.OnWatchMsg != = null)
         //   this.OnWatchMsg(edge, "Edge " + edge.data('name') + ": Influence=" + influence.toFixed(3));

         //console.log("ComputeEdgeInfluence: ", edge.data('name'), " - ", edge.data('influence'))
         //return influence;
         }
      }
   }


float SNIPModel::ActivateNode(SNNode* pNode, float bias)
   {
   // basic idea - iterate through incoming connections, computing inputs.  These get
   // summed and processed through a transfer function to generate an reactivity between 
   // [-1,1].  Note that edge transEff and influence have already been set for this cycle.

   //if (bias == = null)
   //   bias = 0;

   // Step one - aggregate input signals
   //var incomers = node.incomers('edge');  // Note - incomers are nodes, not edges
   float sumInfs = 0;

   for (int i = 0; i < pNode->m_inEdges.GetSize(); i++)
      {
      SNEdge* pEdge = pNode->m_inEdges[i];

      //let a = incomers[i].data('name')
      //float k = pEdge->m_influence;
      //console.log(a, " - ", k)
      sumInfs += pEdge->m_influence;
      }

   //console.log(node.data('name'), " suminfsTotal :", sumInfs)

   // srTotal is the total aggregated input signal (influence) this actor is experiencing.
   float srTotal = this->AggregateInputFn(sumInfs + bias);
   //console.log(node.data('name'), " srTotal :", srTotal)

   // apply signal degradation factor in incoming influence. NOTE - this was moved to happen when activation occurs
   //srTotal = srTotal * (1 - kD);

   float reactivity = this->ActivationFn(srTotal, pNode->m_reactivityMovAvg);

   //if (isNaN(reactivity)) {
   //   this.ErrorMsg("Bad Reactivity calculation for node " + node.data('name') + ": srTotal=" + srTotal + ", sumInfs=" + sumInfs, ", movAvg=" + node.data('reactivityMovAvg'));
   //   reactivity = 0;
   //   }

   pNode->m_sumInfs = sumInfs;
   pNode->m_srTotal = srTotal;
   pNode->m_reactivity = reactivity;    // update store to new activation level

   //console.log('Activating node ' + node.data('id') + ', SumInfs=' + sumInfs.toFixed(2) + ', srTotal=' + srTotal.toFixed(2) + ', Reactivity=' + reactivity.toFixed(2))

   return reactivity;  // [-1,1]
   }


float SNIPModel::ProcessLandscapeSignal(float inputLevel, CArray<float, float>& landscapeTraits, CArray<float, float>& nodeTraits)
   {
   return inputLevel;
   }


float SNIPModel::AggregateInputFn(float sumInputSignals)
   {
   // normalize input to [0,transEffMax]
   // larger values result in a steeper curve
   // scale input signals to 0-transEffMax, based on the absolute value of the signal
   float signal = 0;
   if (sumInputSignals > 0)
      signal = 1 - pow(this->m_aggInputSigma, -sumInputSignals);
   else if (sumInputSignals < 0)
      signal = -1 + pow(this->m_aggInputSigma, sumInputSignals);

   return signal * this->m_transEffMax;  // srTotal
   }


float SNIPModel::ActivationFn(float inputSignal, float reactivityMovAvg)
   {
   // transform normalized input {-1,1] into reactivity using activation function
   float reactivity = 0;
   if (fabs(inputSignal) > this->m_activationThreshold)
      {
      //const scale = 2.0;
      //const shift = 1.0;
      float b = this->m_activationSteepFactB - this->m_kF * reactivityMovAvg;

      reactivity = (2 * (1 / (1 + exp(-b * inputSignal)))) - 1;
      }

   return reactivity;
   }



float SNIPModel::GetOutputLevel()
   {
   // output level is defined as the average activation level of all the nodes
   // may add alternatives later!!
   float output = 0;
   float count = 0;

   //this.cy.nodes(NT_LANDSCAPE_ACTOR).forEach(function(node) {
   for (int i = 0; i < GetNodeCount(); i++)
      {
      output += GetNode(i)->m_reactivity;
      count++;
      }

   return output / count;
   }


float SNIPModel::GetInputLevel(int cycle)
   {  // note: cycle is ZERO based
   switch (this->m_inputType)
      {
      case I_CONSTANT:  // constant
      case I_CONSTWSTOP:  // constant with stop
      case I_SINESOIDAL:  // sinesoidal
      case I_RANDOM:  // random
         return this->m_netInputs[cycle];

      case I_ENVISION_SIGNAL:
         float score = 0;
         if ( this->m_pEvaluator != nullptr)
            score = this->m_pEvaluator->m_score;  // -3 - +3?
         // do any necessary scaling
         score /= 3;
         return score;


         ////case I_TRACKOUTPUT: 
         ////   { // track output
         ////    //let lagPeriod = parseInt($("#lagPeriod").val());
         ////
         ////    // are we at the start of a simulation
         ////   if (cycle <= this->m_lagPeriod) 
         ////      {
         ////      return this->m_initialValue; //parseFloat($("#initialValue").val());
         ////      }
         ////   else 
         ////      {
         ////      //var slope = parseFloat($("#slope").val());
         ////      float output = 0;
         ////      if (this->m_lagPeriod == 0)
         ////         output = GetOutputLevel();
         ////      else
         ////         output = this->m_netOutputs[cycle - this->m_lagPeriod];
         ////
         ////      float input = this->m_slope * output;
         ////      this->m_netInputs[cycle] = input;
         ////      return input;
         ////      }
         ////   }
      }

   return 0;
   }



void SNIPModel::UpdateNetworkStats()
   {
   int edgeCount = GetEdgeCount();
   int nodeCount = GetNodeCount();
   float localClusterCoeff = 0;

   int cycle = this->m_currentCycle < 0 ? 0 : this->m_currentCycle;
   float inputActivation = this->GetInputLevel(cycle);

   this->m_netStats.Init();
   this->m_netStats.cycle = this->m_currentCycle < 0 ? 0 : this->m_currentCycle;
   this->m_netStats.nodeCount = nodeCount;
   this->m_netStats.edgeCount = edgeCount;
   this->m_netStats.inputActivation = inputActivation;
   this->m_netStats.outputReactivity = this->GetOutputLevel();
   this->m_netStats.clusteringCoefficient = localClusterCoeff;

   int landscapeActorCount = 0;
   int inputSignalCount = 0;
   //let _this = this;

   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);
      if (pNode->IsInputSignal() == false )
         {
         // min reactivity
         float nodeReactivity = pNode->m_reactivity;
         if (nodeReactivity < this->m_netStats.minNodeReactivity)
            this->m_netStats.minNodeReactivity = nodeReactivity;
         // max reactivity
         if (nodeReactivity > this->m_netStats.maxNodeReactivity)
            this->m_netStats.maxNodeReactivity = nodeReactivity;
         // mean reactivity
         this->m_netStats.meanNodeReactivity += nodeReactivity;
         }

      if (pNode->IsLandscapeActor())
         {
         this->m_netStats.meanLANodeReactivity += pNode->m_reactivity;
         landscapeActorCount += 1;
         }

      else if (pNode->IsInputSignal())
         {
         inputSignalCount += 1;
         }
      }

   // netStats.networkNodeCount = netStats.nodeCount - (landscapeActorCount + inputSignalCount)

   this->m_netStats.meanNodeReactivity /= nodeCount;

   if (landscapeActorCount > 0)
      this->m_netStats.meanLANodeReactivity /= landscapeActorCount;

   //this->m_cy.nodes(ANY_ACTOR).forEach(function(node) {
   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);
      float nodeInfluence = pNode->m_influence;
      // min reactivity
      if (nodeInfluence < this->m_netStats.minNodeInfluence)
         this->m_netStats.minNodeInfluence = nodeInfluence;
      // max reactivity
      if (nodeInfluence > this->m_netStats.maxNodeInfluence)
         this->m_netStats.maxNodeInfluence = nodeInfluence;
      // mean reactivity
      this->m_netStats.meanNodeInfluence += nodeInfluence;
      }

   this->m_netStats.meanNodeInfluence /= nodeCount;

   //this->m_cy.nodes(INPUT_SIGNAL).forEach(function(node) {
   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);
      if (pNode->IsInputSignal())
         this->m_netStats.landscapeSignalInfluence += pNode->m_influence;
      }

   // next, do edges
   //this->m_cy.edges(NETWORK_EDGE).forEach(function(edge) {
   for (int i = 0; i < this->GetEdgeCount(); i++)
      {
      SNEdge* pEdge = this->GetEdge(i);
      if (pEdge->m_edgeType == ET_NETWORK)
         {
         // transmission efficiency
         float transEff = pEdge->m_transEff;
         if (transEff < this->m_netStats.minEdgeTransEff)
            this->m_netStats.minEdgeTransEff = transEff;

         if (transEff > this->m_netStats.maxEdgeTransEff)
            this->m_netStats.maxEdgeTransEff = transEff;

         this->m_netStats.meanEdgeTransEff += transEff;

         // influence
         float influence = pEdge->m_influence;

         if (influence < this->m_netStats.minEdgeInfluence)
            this->m_netStats.minEdgeInfluence = influence;

         if (influence > this->m_netStats.maxEdgeInfluence)
            this->m_netStats.maxEdgeInfluence = influence;

         this->m_netStats.meanEdgeInfluence += influence;
         }
      }

   this->m_netStats.meanEdgeTransEff /= edgeCount;
   this->m_netStats.meanEdgeInfluence /= edgeCount;

   // LAReactivity.reduce((a, b) => (a + b)) / LAReactivity.length
   // traditional SA stats
   ///// float cs = this->GetDegreeCentralities(NETWORK_ACTOR, true);
   ///// this->m_netStats.meanDegreeCentrality = cs.length > 0 ? cs.reduce((a, b) = > (a + b)) / cs.length : 0;
   ///// 
   ///// cs = this->m_GetInfWtDegreeCentralities(NETWORK_ACTOR, true);
   ///// this->m_netStats.meanInfWtDegreeCentrality = cs.length > 0 ? cs.reduce((a, b) = > (a + b)) / cs.length : 0;
   ///// 
   ///// cs = this->m_GetBetweennessCentralities(NETWORK_ACTOR, true);
   ///// this->m_netStats.meanBetweennessCentrality = cs.length > 0 ? cs.reduce((a, b) = > (a + b)) / cs.length : 0;
   ///// 
   ///// cs = this->m_GetInfWtBetweennessCentralities(NETWORK_ACTOR, true);
   ///// this->m_netStats.meanInfWtBetweenCentrality = cs.length > 0 ? cs.reduce((a, b) = > (a + b)) / cs.length : 0;
   ///// 
   ///// cs = this->m_GetClosenessCentralities(NETWORK_ACTOR, true);
   ///// this->m_netStats.meanClosenessnessCentrality = cs.length > 0 ? cs.reduce((a, b) = > (a + b)) / cs.length : 0;
   ///// 
   ///// cs = this->m_GetInfWtClosenessCentralities(NETWORK_ACTOR, true);
   ///// this->m_netStats.meanInfWtClosenessnessCentrality = cs.length > 0 ? cs.reduce((a, b) = > (a + b)) / cs.length : 0;
   ///// 
   ///// // Generate final values for netStats
   ///// 
   ///// // motif representation
   ///// let coordinators = this->m_GetMotifs(MOTIF_COORDINATOR);
   ///// let gatekeepers = this->m_GetMotifs(MOTIF_GATEKEEPER);
   ///// let representatives = this->m_GetMotifs(MOTIF_REPRESENTATIVE);
   ///// let leaders = this->m_GetMotifs(MOTIF_LEADER);
   ///// this->m_netStats.normCoordinators = coordinators.length / this->m_netStats.nodeCount;
   ///// this->m_netStats.normGatekeeper = gatekeepers.length / this->m_netStats.nodeCount;
   ///// this->m_netStats.normRepresentative = representatives.length / this->m_netStats.nodeCount;
   ///// this->m_netStats.normLeaders = leaders.length / this->m_netStats.nodeCount;

   // ???
   int edgeCountNLA = 0;
   int edgeCountLSN = 0;
   int edgeCountN = 0;
   float NLA_Influence = 0;
   float NLA_TransEff = 0;
   float totalEdgeSignalStrength = 0;
   int landscapeActorNodeCount = 0;
   float contestt = 0;

   //this->m_cy.edges().forEach(function(edge) {
   for (int i = 0; i < this->GetEdgeCount(); i++)
      {
      SNEdge* pEdge = this->GetEdge(i);

      if (pEdge->Target()->IsLandscapeActor())
         {   // is target of edge a landscape actor?
         edgeCountNLA = edgeCountNLA + 1;
         NLA_Influence = NLA_Influence + pEdge->m_influence;
         NLA_TransEff = NLA_TransEff + pEdge->m_transEff;
         }
      else if (pEdge->Source()->IsInputSignal())
         {
         edgeCountLSN += 1;
         totalEdgeSignalStrength += pEdge->m_signalStrength;
         }

      else {
         edgeCountN += 1;
         totalEdgeSignalStrength += pEdge->m_signalStrength;
         }
      }

   //Contestedness
   //Add the sign of the sender node to the influence. A contested signal will have a contestt value of 0
   // 
   //this->m_cy.nodes(LANDSCAPE_ACTOR).forEach(function(node) {
   for (int i = 0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = this->GetNode(i);
      if (pNode->IsLandscapeActor())
         {
         landscapeActorNodeCount += 1;
         float cont_temp = 0;
         float max_inf = 0;
         //pNode.incomers('edge').forEach(function(edge) {
         for (int j = 0; j < pNode->m_inEdges.GetSize(); j++)
            {
            //let sn = edge.data('source');
            //let nodetraits = _this->m_cy.elements('node#' + sn).data('traits');
            //cont_temp += edge.data("influence") * (nodetraits / Math.abs(nodetraits))
            //
            //   //Possibly add a maximum possible influence value here. If cont_temp at the end is greater than any max influence, then the signal is past the contested threshold
            //   //But we might also not need that as contestedness requires opposite signals of the same magnitude. So as we move farther away from 0 we have
            //   // a less contested signal, it's just higher values mean that the signal is uncontested at a greater level.
            //   if (edge.data("influence") > max_inf) {
            //      max_inf = edge.data("influence");
            //      };
            //
            }

         // contestt += Math.abs(cont_temp)
         }
      }

   this->m_netStats.signalContestedness = contestt / landscapeActorNodeCount;

   this->m_netStats.normNLA_Influence = NLA_Influence / edgeCountNLA;
   this->m_netStats.normNLA_TransEff = NLA_TransEff / edgeCountNLA;

   this->m_netStats.normEdgeSignalStrength = totalEdgeSignalStrength / this->m_netStats.edgeCount;

   // Measure of Edge Density w/r/t Landscape Signals and Landscape Actors
   // Maximum possible edges for LSN == Network Actors, NLA == Network Actors * # Landscape Actors

   //this->m_netStats.edgeMakeupLSN = edgeCountLSN / this->m_netStats.networkNodeCount
   //   this->m_netStats.edgeMakeupNLA = edgeCountNLA / (this->m_netStats.networkNodeCount * lanc)
   //   // Collaboration Metric Currently the Density of the inner network (innie-innie network??)? 
   //   this->m_netStats.networkActorDensity = edgeCountN / (this->m_netStats.networkNodeCount * (this->m_netStats.networkNodeCount - 1));
   //
   ////console.log(this->m_cy.$().betweennessCentrality({ directed: true }));
   //var LAReactivity = []
   //
   //   this->m_cy.nodes(ANY_ACTOR).forEach(function(node) {
   //   if (node.data('type') == = NT_LANDSCAPE_ACTOR) {
   //      ;
   //      LAReactivity.push(node.data("reactivity"));
   //      }
   //   if (node.data('type') == = NT_INPUT_SIGNAL) {
   //      landscapeSignalInfluence = node.data("influence");
   //      }
   //   });
   //
   //
   //
   //this->m_landscapeSignalInfluence = this->m_landscapeSignalInfluence
   //   this->m_netStats.maxLANodeReactivity = Math.max(...LAReactivity);
   //this->m_netStats.meanLANodeReactivity = LAReactivity.reduce((a, b) = > (a + b)) / LAReactivity.length;

   return;
   }


bool SNIPModel::CollectData(EnvContext* pEnvContext)
   {
   UpdateNetworkStats();
   CArray<float, float> data;

   data.Add((float) pEnvContext->currentYear);
   data.Add((float) m_netStats.cycle);
   data.Add((float) m_netStats.convergeIterations);
   data.Add((float) m_netStats.nodeCount);
   data.Add((float) m_netStats.edgeCount);
   data.Add((float) m_netStats.nodeCountNLA);
   data.Add(m_netStats.minNodeReactivity);
   data.Add(m_netStats.meanNodeReactivity);
   data.Add(m_netStats.maxNodeReactivity);
   data.Add(m_netStats.minNodeInfluence);
   data.Add(m_netStats.meanNodeInfluence);
   data.Add(m_netStats.maxNodeInfluence);
   data.Add(m_netStats.minEdgeTransEff);
   data.Add(m_netStats.meanEdgeTransEff);
   data.Add(m_netStats.maxEdgeTransEff);
   data.Add(m_netStats.minEdgeInfluence);
   data.Add(m_netStats.meanEdgeInfluence);
   data.Add(m_netStats.maxEdgeInfluence);
   data.Add(m_netStats.edgeDensity);
   data.Add(m_netStats.totalEdgeSignalStrength);
   data.Add(m_netStats.inputActivation);
   data.Add(m_netStats.landscapeSignalInfluence);
   data.Add(m_netStats.outputReactivity);
   data.Add(m_netStats.meanLANodeReactivity);
   data.Add(m_netStats.maxLANodeReactivity);
   data.Add(m_netStats.signalContestedness);
   data.Add(m_netStats.meanDegreeCentrality);
   data.Add(m_netStats.meanBetweenessCentrality);
   data.Add(m_netStats.meanClosenessCentrality);
   data.Add(m_netStats.meanInfWtDegreeCentrality);
   data.Add(m_netStats.meanInfWtBetweenessCentrality);
   data.Add(m_netStats.meanInfWtClosenessCentrality);
   data.Add(m_netStats.normCoordinators);
   data.Add(m_netStats.normGatekeeper);
   data.Add(m_netStats.normRepresentative);
   data.Add(m_netStats.notLeaders);
   data.Add(m_netStats.clusteringCoefficient);
   data.Add(m_netStats.NLA_Influence);
   data.Add(m_netStats.NLA_TransEff);
   data.Add(m_netStats.normNLA_Influence);
   data.Add(m_netStats.normNLA_TransEff);
   data.Add(m_netStats.normEdgeSignalStrength);
   this->m_pOutputData->AppendRow(data);
 
   return true;
   }

int SNIPModel::GetMaxDegree(bool) 
   {
   int count = GetNodeCount(); int maxDegree = 0;
   for (int i = 0; i < count; i++) {
      int edges = int(GetNode(i)->m_inEdges.GetSize() + GetNode(i)->m_outEdges.GetSize());
      if (edges > maxDegree)
         maxDegree = edges;
      }
   return maxDegree;
   }


int SNIPModel::GetActiveNodeCount(int type)
   {
   int count = GetNodeCount();
   int activeCount = 0;
   for (int i = 0; i < count; i++)
      {
      SNNode* pNode = m_pSNLayer->GetNode(i);
      if ((pNode->m_nodeType == type) && (pNode->m_state == SNIP_STATE::STATE_ACTIVE))
         activeCount++;
      }
   return activeCount;
   }

int SNIPModel::GetActiveEdgeCount() 
   {
   int count = GetEdgeCount();
   int activeCount = 0;
   for (int i = 0; i < count; i++)
      if (m_pSNLayer->GetEdge(i)->m_state == SNIP_STATE::STATE_ACTIVE)
         activeCount++;
   return activeCount;
   }


int SNIPModel::GetNodeCount(int type) 
   {
   int count = GetNodeCount();
   int _count = 0;
   for (int i = 0; i < count; i++)
      {
      SNNode* pNode = m_pSNLayer->GetNode(i);
      if (pNode->m_nodeType == type)
         _count++;
      }
   return _count;
   }


float SNIPModel::GetMeanReactivity(int type)
   {
   int count = GetNodeCount();
   int typeCount = 0;
   float reactivity = 0;
   for (int i = 0; i < count; i++)
      {
      SNNode* pNode = m_pSNLayer->GetNode(i);
      if (pNode->m_nodeType == type)
         {
         typeCount++;
         reactivity += pNode->m_reactivity;
         }
      }
   return reactivity / typeCount;
   }



/*
bool SNIPModel::BuildNetwork()
   {
   // make a default network object
   //SnipModel *pSnipModel = new SnipModel;  // create new SnipModel object

   //Node.js Testing
   //if (divNetwork == "Test") {
   //   snipModel.randGenerator = Math.random();
   //   }
   //else {
   //   snipModel.randGenerator = new Math.seedrandom(networkInfo.name);
   //   }
   //snipModel.networkInfo = networkInfo;

   if (m_networkJSON.contains("settings"))
      this->LoadPreBuildSettings(m_networkJSON["settings"]);

   this->BuildNodes(m_networkJSON["nodes"], m_networkJSON["traits"]);
   this->BuildEdges(m_networkJSON["edges"]);

   // instantiate a cytocspe network object
   //snipModel.cy = cytoscape({
   //    container: $('#' + divNetwork),
   //    minZoom : 0.2,
   //    maxZoom : 4,
   //    elements : {
   //        nodes: nodes,
   //        edges : edges
   //    },
   //    wheelSensitivity : 0.05,
   //    style : SnipModel.GetNetworkStyleSheet(),
   //    layout : snipModel.layout
   //   });

   this->m_pInputNode = this->cy.nodes(INPUT_SIGNAL)[0];
   //InitSimulation();

   // setup up initial network configuration
   this->AddAutogenInputEdges();
   this->SetEdgeTransitTimes();

   this->ResetNetwork();
   this->GenerateInputArray();
   this->SetInputNodeReactivity();
   this->m_maxNodeDegree = this->cy.nodes().maxDegree(false);

   this->SolveEqNetwork(0);

   //this->InitSimulation();

   this->UpdateNetworkStats();

   if (m_networkJSON.contains("settings"))
      this->LoadPostBuildSettings(m_networkJSON["settings"]);

   //this->cy.on('mouseover', 'node', ShowNodeTip);
   //this->cy.on('mouseout', 'node', HideTip);
   //this->cy.on('mouseover', 'edge', ShowEdgeTip);
   //this->cy.on('mouseout', 'edge', HideTip);

   return true;
   }
   */

bool SNIPModel::LoadPreBuildSettings(json& settings)
   {
   if (settings.contains("input"))
      {
      auto& input = settings["input"];
      auto inputType = input["type"].get<std::string>();

      if (inputType == "constant")
         {
         this->m_inputType = I_CONSTANT;
         this->m_k = input["value"].get<float>();
         }
      else if (inputType == "constant_with_stop")
         {
         this->m_inputType = I_CONSTWSTOP;
         this->m_k1 = input["value"].get<float>();
         this->m_stop = input["stop"].get<float>();
         }

      else if (inputType == "sinusoidal")
         {
         this->m_inputType = I_SINESOIDAL;
         this->m_amp = input["amplitude"].get<float>();
         this->m_period = input["period"].get<float>();
         this->m_phase = input["phase_shift"].get<float>();
         }
      else
         Report::LogWarning("SNIP - Unsupported input type encountered");
      //case "random":
      //   break;
      //case "track_output":
      //   break;
      }

   this->m_inputType = I_ENVISION_SIGNAL;  // override network setting

   if (settings.contains("trans_eff_max"))
      this->m_transEffMax = settings["trans_eff_max"].get<float>();

   if (settings.contains("agg_input_sigma"))
      this->m_aggInputSigma = settings["agg_input_sigma"].get<float>();;

   if (settings.contains("reactivity_steepness_factor_B"))
      this->m_activationSteepFactB = settings["reactivity_steepness_factor_B"].get<float>();;

   if (settings.contains("reactivity_threshold"))
      this->m_activationThreshold = settings["reactivity_threshold"].get<float>();

   //if (settings.contains("autogenerate_landscape_signals" in settings) {
   //   if (typeof settings.autogenerate_landscape_signals == = 'boolean') {
   //      if (this->autogenFraction == = true)
   //         this->autogenFraction = 1.0;
   //      else
   //         this->autogenFraction = 0;
   //      }
   //   else {    // it's an object
   //      this->autogenFraction = settings.autogenerate_landscape_signals.fraction;
   //      this->autogenBias = settings.autogenerate_landscape_signals.bias;
   //      }
   //   }

   if (settings.contains("autogenerate_transit_times"))
      {
      this->m_autogenTransTimeMax = settings["autogenerate_transit_times"]["max_transit_time"].get<int>();
      this->m_autogenTransTimeBias = settings["autogenerate_transit_times"]["bias"].get<std::string>();
      }

   //if (settings.contains("layout" in settings) {
   //   this->layout = settings.layout;
   //   this->UpdateNetworkLayout(settings.layout);
   //   }

   if (settings.contains("infSubmodel"))
      {
      std::string ism = settings["infSubmodel"].get<std::string>();
      if (ism == "sender_receiver")
         this->m_infSubmodel = IM_SENDER_RECEIVER;
      else if (ism == "signal_receiver")
         this->m_infSubmodel = IM_SIGNAL_RECEIVER;
      else if (ism == "signal_sender_receiver")
         this->m_infSubmodel = IM_SIGNAL_SENDER_RECEIVER;
      else if (ism == "trust")
         this->m_infSubmodel = IM_TRUST;
      else
         Report::LogWarning("SNIP: Unknown submodel specified");
      }

   if (settings.contains("infSubmodelWt"))
      this->m_infSubmodelWt = settings["infSubmodelWt"].get<float>();

   return true;
   }



bool SNIPModel::LoadPostBuildSettings(json& settings)
   {
   //if ("show_tips" in settings)
   //    this.showTips = settings.show_tips;

    //if ("show_landscape_signal_edges" in settings) {
    //   if (settings.show_landscape_signal_edges == = true)
    //      this.ShowLandscapeSignalEdges(1);
    //   else
    //      this.ShowLandscapeSignalEdges(0);
    //   }

    //if ("zoom" in settings) {
    //    this.cy.zoom(settings.zoom);
    //}
    //if ("center" in settings) {
    //    this.cy.pan(settings.center);
    //}
   if (settings.contains("simulation_period"))
      this->m_cycles = settings["simulation_period"].get<int>();

   return true;
   }



int SNIPModel::ConnectToIDUs(MapLayer* pIDULayer, LPCTSTR mappings, float mappingFrac)
   {
   // read mapping csv
   VDataObj actorProfiles(UNIT_MEASURE::U_UNDEFINED);

   int profileCount = actorProfiles.ReadAscii(mappings, ',');

   // basic idea:  We want to assign each candidate IDU (actor) to a specific actor profile
   // based on ??? and build a network actor->landscape actor edge in SNIP to 
   // represent the onnection
   int colIDUProfileID = -1;
   this->m_pSNLayer->m_pSNIP->CheckCol(pIDULayer, colIDUProfileID, "ProfileID", TYPE_INT, CC_AUTOADD);

   int colX = actorProfiles.GetCol("POINT_X");
   int colY = actorProfiles.GetCol("POINT_Y");
   int colProfileID = actorProfiles.GetCol("ProfileID");

   pIDULayer->SetColData(colProfileID, VData(-1), true);

   int traitsCount = (int)this->m_traitsLabels.size();
   float* _traits = new float[traitsCount];
   memset(_traits, 0, traitsCount * sizeof(float));  // zero out array

   std::set<string> missingTraits;
   int newLACount = 0;
   int unconnectedLACount = 0;
   int newEdgeCount = 0;
   std::unordered_set<SNNode*> newEngagers;
  
   // go through IDU's bulding network nodes/links for IDU's that pass the mapping query
   for (MapLayer::Iterator idu = pIDULayer->Begin(); idu != pIDULayer->End(); idu++)
      {
      bool result = false;
      bool ok = this->m_pMappingQuery->Run(idu, result);

      if (ok && result)
         {
         double rn = m_randShuffle.RandValue(0, 1);  // randomly select based on mapping fraction
         if (rn < mappingFrac)
            {
            // iterate through actor profiles, building landscape actor nodes connections for each actor
            // assign the closest profile to the IDU
            int closest = -1;
            float closestDist = 999999999.0f;
            Vertex c = pIDULayer->GetPolygon(idu)->GetCentroid();

            for (int i = 0; i < profileCount; i++)
               {
               float x = actorProfiles.GetAsFloat(colX, i);
               float y = actorProfiles.GetAsFloat(colY, i);

               float d = ::DistancePtToPt(c.x, c.y, x, y);
               if (d < closestDist)
                  {
                  closest = i;
                  closestDist = d;
                  }
               }

            // have the closest agent profile, next assign to IDU and build connections in SNIP.
            // Note that both nodes, representing landscape actors, and edges connecting these LA nodes
            // to in-network Engager nodes are built 
            pIDULayer->SetData(idu, colIDUProfileID, closest);

            SNNode* pNode = nullptr; // for this IDU

            // iterate though the list of traits, comparing to the trait strength in the matched actor profile.
            // if the trait matches the actor profile, that signals that we want to creating a landscape actor
            // and connect it to an engager actor matching the trait in the SNIP network.
            // Note that this implies that one landscape actor can be connected to many engager network nodes.
            for (auto& trait : this->m_traitsLabels)
               {
               int col = actorProfiles.GetCol(trait.c_str());
               if (col >= 0)
                  {
                  // should a link be created to the network actor(s)? 
                  int link = actorProfiles.GetAsInt(col, closest);
                  if (link > 0)
                     {// make a LA node if we haven't already
                     if (pNode == nullptr)
                        {
                        // we want to add a node to our network for this landscape actor
                        CString nname;
                        nname.Format("LA_%i", (int)idu);
                        pNode = BuildNode(NT_LANDSCAPE_ACTOR, nname, _traits);
                        pNode->m_idu = (int)idu;
                        newLACount++;
                        }

                     // find an engager that has this trait
                     vector<SNNode*> nodes;
                     int ncount = this->FindNodesFromTrait(trait, NT_ENGAGER, 0.2f, nodes);

                     if (ncount > 0)
                        {
                        int ri = (int)m_randShuffle.RandValue(0, ncount);
                        SNNode* pEngagerNode = nodes[ri];   // this is the source node, our LA node is the target

                        // do we already have a connection to this engager?
                        bool found = false;
                        for (int j = 0; j < (int)pNode->m_inEdges.GetSize(); j++)
                           {
                           if (pNode->m_inEdges[j]->m_pFromNode == pEngagerNode)
                              {
                              found = true;
                              break;
                              }
                           }

                        if (!found)
                           {
                           int transTime = 1; // ???
                           float trust = 0.9f;
                           SNEdge* pEdge = BuildEdge(pEngagerNode, pNode, transTime, trust, _traits);
                           newEdgeCount++;
                           ASSERT(pEdge->Source()->m_nodeType == NT_ENGAGER);
                           newEngagers.insert(pEdge->Source());
                           }
                        }
                     else
                        {
                        //CString msg;
                        //msg.Format("  Unable to find engager with trait [%s] to connect to landscape actor %i", trait.c_str(), (int) idu);
                        //Report::LogWarning(msg);
                        //nMissingEngagers++;
                        }
                     }
                  }
               else
                  {
                  if (missingTraits.find(trait) != missingTraits.end())
                     {
                     CString msg;
                     msg.Format(" Unable to locate trait '%s' in the actor profiles when assigning landscape actors to SNIP Network.   This trait will be ignored", trait.c_str());
                     Report::LogWarning(msg);
                     missingTraits.insert(trait);
                     }
                  }  // end of: else (link>0 == false)
               }  // end of:  for each trait

            // if no engagers found, remove LA node from network
            if (pNode && pNode->m_inEdges.GetSize() == 0)
               {
               newLACount++;
               unconnectedLACount++;
               this->m_pSNLayer->RemoveNode(pNode);
               }
            }  // end of: if ( rn < mappingFrac
         }  // end of: if ( ok && result ) - passed query
      }  // end of: for each IDU

   CString msg;
   msg.Format("Added %i Landscape Actors to the SNIP network, with %i connections to %i engagers. %i Landscape Actors were not connectabel to any engagers", newLACount, newEdgeCount, newEngagers.size(), unconnectedLACount);
   Report::LogInfo(msg);

   delete[]_traits;
   return 0;
   }


/*
void SNIPModel::GetMotifs(SNIP_MOTIF motif)
   {

   // Gatekeeper - a gatekeeper is defined by:
   // receives an "out of group" incoming signal and propagates to at least
   // one "in group"
   float similarityThreshold = 0.5f;
   vector<SNNode*> nodes;
   float bc = 0l;
   if (motif == MOTIF_LEADER)
      ;  // bc = this.cy.$().betweennessCentrality({ directed: true });

   //this.cy.nodes(NETWORK_ACTOR).forEach(function(node) {
   for ( int i=0; i < this->GetNodeCount(); i++)
      {
      SNNode* pNode = GetNode(i);
      bool downstreamInGroup = false;
      bool downstreamOutGroup = false;
      bool upstreamOutGroup = false;
      bool upstreamInGroup = false;
      float similarity = 0;

      node.neighborhood('edge').forEach(function(edge) {
         // downstream edge?
         if (edge.source() == = node) {
            similarity = _this.ComputeSimilarity(node.data('traits'), edge.target().data('traits'));
            if (similarity > similarityThreshold)
               downstreamInGroup = true;
            else
               downstreamOutGroup = true;

            }
         else { // upstream edge
            similarity = _this.ComputeSimilarity(node.data('traits'), edge.source().data('traits'));
            if (similarity < similarityThreshold)
               upstreamOutGroup = true;
            else
               upstreamInGroup = true;
            }
         });

      switch (motif) {
         case MOTIF_GATEKEEPER:
            if (downstreamInGroup && upstreamOutGroup)
               nodes.push(node);
            break;

         case MOTIF_COORDINATOR:
            if (downstreamInGroup && upstreamInGroup)
               nodes.push(node);
            break;

         case MOTIF_REPRESENTATIVE:
            if (downstreamOutGroup && upstreamInGroup)
               nodes.push(node);
            break;

         case MOTIF_LEADER:
            const bcN = bc.betweennessNormalized('#' + node.data('id'));
            if (bcN > 0.5) // ????
               nodes.push(node);
            break;
         }
      });

   return nodes;
   }  */

/*
GetDegreeCentralities(selector, normed) {
   let nas = this.cy.nodes(selector);  //NETWORK_ACTOR);
   let values = [];

   let maxDC = 0;
   if (normed) {
      nas.forEach(node = > {
         let outgoers = node.outgoers('edge');
         if (outgoers.length > maxDC)
            maxDC = outgoers.length;
         });
      }

   nas.forEach(node = > {
      if (normed)
         values.push(node.outgoers('edge').length / maxDC);
      else
         values.push(node.outgoers('edge').length);
      });

   return values;
   }

GetInfWtDegreeCentralities(selector, normed) {
   let nas = this.cy.nodes(selector);  //NETWORK_ACTOR);
   let values = [];

   let maxDC = 0;
   if (normed) {
      nas.forEach(node = > {
         let outgoers = node.outgoers('edge');

         outgoers.forEach(edge = > {
            if (edge.data('influence') > maxDC)
               maxDC = edge.data('influence');
            });
         });
      }

   nas.forEach(node = > {
      let outgoers = node.outgoers('edge');
      let dci = 0;
      if (normed)
         outgoers.forEach(edge = > dci += edge.data('influence') / maxDC);
      else
         outgoers.forEach(edge = > dci += edge.data('influence'));

      values.push(dci);
      });

   return values;
   }


GetBetweennessCentralities(selector, normed) {
   let nas = this.cy.nodes(selector);
   let bc = null;
   if (normed)     // NEEDS WORK!  NORM NOT IMPLEMTENTED
      bc = nas.bc({ directed: true });  // betweenness
   else
      bc = nas.bc({ directed: true });  // betweenness

   let values = [];
   let _this = this;
   nas.forEach(node = > {
      let _bc = bc.betweenness(node);
      values.push(_bc);
      });

   return values;
   }

GetInfWtBetweennessCentralities(selector, normed) {
   let nas = this.cy.nodes(selector);
   let bc = null;
   if (normed)
      bc = nas.bc({ weight: (edge) = > edge.data('influence'), directed: true });  // betweenness
   else
      bc = nas.bc({ directed: true });  // betweenness

   let values = [];
   let _this = this;
   nas.forEach(node = > {
      let _bc = bc.betweenness(node);
      values.push(_bc);
      });

return values;
}

GetClosenessCentralities(selector, normed) {
   let nas = this.cy.nodes(selector);
   let values = [];
   let cc = null;
   if (normed)
      cc = nas.ccn({ directed: true });
   else
      cc = nas.cc({ directed: true });

   let _this = this;
   nas.forEach(node = > {
      let value = cc.closeness(node);
      values.push(value);
      });

   return values;
   }

GetInfWtClosenessCentralities(selector, normed) {
   let nas = this.cy.nodes(selector);
   let values = [];
   let cc = null;

   if (normed)
      cc = nas.ccn({ weight: (edge) = > Math.abs(edge.data('influence')), directed: true });  // betweenness
   else
      cc = nas.cc({ directed: true });  // betweenness

   nas.forEach(node = > {
      let value = cc.closeness(node);
      values.push(value);
      });

   return values;
   }

*/































//int SNLayer::Activate( void )
/////////////////////////////////////////////////////////////////////

void SNLayer::GetInteriorNodes(CArray< SNNode*, SNNode* >& out) //get all interior nodes (ie organizations)
   {
   for (int i = 0; i < m_nodes.GetSize(); i++)
      {
      SNNode* node = m_nodes[i];
      if (node->IsInterior())
         out.Add(node);
      }
   }

void SNLayer::GetInteriorNodes(bool active, CArray< SNNode*, SNNode* >& out)   //get organizations that are 'active' or inactive
   {
   for (int i = 0; i < m_nodes.GetSize(); i++)
      {
      SNNode* node = m_nodes[i];
      if (node->IsInterior())
         {
         if (active && node->IsActive())
            out.Add(node);
         else if (active == false && node->IsActive() == false)
            out.Add(node);
         }
      }
   }

void SNLayer::GetInteriorEdges(CArray< SNEdge*, SNEdge* >& out) //get interior edges
   {
   for (int i = 0; i < m_edges.GetSize(); i++)
      {
      SNEdge* edge = m_edges[i];
      if (edge->m_pFromNode->IsInterior())
         out.Add(edge);
      }
   }

void SNLayer::GetInteriorEdges(bool active, CArray< SNEdge*, SNEdge* >& out) //get active/inactive edges
   {
   for (int i = 0; i < m_edges.GetSize(); i++)
      {
      SNEdge* edge = m_edges[i];
      if (edge->m_pFromNode->m_nodeType == NT_NETWORK_ACTOR && edge->m_pToNode->m_nodeType == NT_NETWORK_ACTOR && edge->m_state == STATE_ACTIVE)
         {
         out.Add(edge);
         }
      else if (edge->m_pFromNode->m_nodeType == NT_NETWORK_ACTOR && edge->m_pToNode->m_nodeType == NT_NETWORK_ACTOR && edge->m_state == STATE_ACTIVE)
         {
         out.Add(edge);
         }
      }
   }

void SNLayer::GetOutputEdges(bool active, CArray< SNEdge*, SNEdge* >& out) //get active edges from orgs to actor groups
   {
   for (int i = 0; i < m_edges.GetSize(); i++)
      {
      SNEdge* edge = m_edges[i];
      if (edge->m_pToNode->IsLandscapeActor() && edge->IsActive())
         out.Add(edge);
      }
   }


double SNLayer::SNDensity()//number of edges/max possible number of edges
   {
   CArray< SNNode*, SNNode* > interiorNodes;
   GetInteriorNodes(interiorNodes);
   CArray < SNEdge*, SNEdge* > interiorEdges;
   GetInteriorEdges(interiorEdges);
   double ret = -1;

   int nodeCount = (int)interiorNodes.GetSize();
   if (nodeCount > 1)
      {
      double max_edges = (nodeCount) * (nodeCount - 1);
      ret = ((double)interiorEdges.GetSize()) / max_edges;
      }
   return ret;
   }

double SNLayer::SNWeight_Density() //sum of weights/max possible sum of weights
   {
   double ret = -1;
   CArray< SNNode*, SNNode* > interiorNodes;
   GetInteriorNodes(interiorNodes);
   CArray < SNEdge*, SNEdge* > interiorEdges;
   GetInteriorEdges(interiorEdges);
   int nodeCount = (int)interiorNodes.GetSize();
   if (nodeCount > 1)
      {
      float sum_weights = 0;
      for (int i = 0; i < interiorEdges.GetSize(); i++)
         {
         SNNode* pNode = interiorEdges[i]->m_pFromNode;
         ////sum_weights += interiorEdges[i]->m_weight;
         }
      int max_edges = (nodeCount) * (nodeCount - 1);
      ret = sum_weights / max_edges;
      }
   return ret;
   }

/*

bool SNLayer::SetInCountTable()//in degree for each organization
{
   m_inCountTable.clear();
   CArray < SNEdge*, SNEdge* > interiorEdges;
   GetInteriorEdges(interiorEdges);
   for (int i = 0; i < interiorEdges.GetSize(); i++)
   {
      SNEdge *pEdge = interiorEdges[i];
      if (m_inCountTable.find(pEdge->m_pToNode->m_name) == m_inCountTable.end())
      {
         m_inCountTable[pEdge->m_pToNode->m_name] = 0;
      }
      m_inCountTable[pEdge->m_pToNode->m_name]++;
   }
   return false;
}

bool SNLayer::SetOutCountTable()//out degree for each organization
{
   m_outCountTable.clear();
   CArray < SNEdge*, SNEdge* > interiorEdges;
   GetInteriorEdges(interiorEdges);
   for (int i = 0; i < interiorEdges.GetSize(); i++)
   {
      SNEdge *pEdge = interiorEdges[i];
      if (m_outCountTable.find(pEdge->m_pFromNode->m_name) == m_outCountTable.end())
      {
         m_outCountTable[pEdge->m_pFromNode->m_name] = 0;
      }
      m_outCountTable[pEdge->m_pFromNode->m_name]++;
   }
   return false;
}
*/
/*
void SNLayer::LSReact()  //get input node (landscape signal) and compare it to the threshold of each node it connects to.
{
   double threshold = 0.5;
   CArray < SNEdge*, SNEdge* > ActiveEdges;
   SNNode* InputNode = GetInputNode();
   float LandscapeScore = InputNode->m_activationLevel;

   for (int i = 0; i < InputNode->m_noEdges; i++)
   {
      SNEdge* temp = InputNode->m_edges[i];
      if(temp->m_pToNode->m_nodeType == NT_NETWORK_ACTOR)		//if edge is to an organization
      {
         if (fabs(temp->m_pToNode->m_landscapeGoal - LandscapeScore)*temp->m_weight > threshold)   //if (distance between the orgs goal and the landscape signal)*reactivity of org/importance of metric to the org > threshold
         {
            temp->m_pToNode->m_active = 1;
            for(int j = 0; j < temp->m_pToNode->m_noEdges; j++)
            {
               if((temp->m_pToNode->m_edges[j]->m_pToNode != temp->m_pToNode) && (temp->m_pToNode->m_edges[j]->m_pToNode->m_nodeType == NT_NETWORK_ACTOR)) //if the edge is to another org (should also gather edges to actor groups? could also just keep track of active nodes and gather these connex later)
               {
                  ActiveEdges.Add(temp->m_pToNode->m_edges[j]);		//then add this edge to the ActiveEdges
                  temp->m_pToNode->m_edges[j]->m_active = 1;
               }
            }
         }
      }
   }
   //create unique list of contacted nodes
   CArray <SNNode*> ContactedNodes;
   GetContactedNodes(ActiveEdges, ContactedNodes);
   CArray < SNEdge*, SNEdge* > newEdges;
   //for each contacted node, decide activate? reciprocate? do nothing? - if activate, all its edges become active...put this in a list and repeat.  If reciprocate, strengthen both edges between these orgs.  If do nothing, weaken edge weight from org with active edge
   //
   do
   {
      newEdges.RemoveAll();
      for (int i = 0; i < ContactedNodes.GetCount(); i++)
      {
//			 newEdges.Append(ContactedNodes[i]->DecideActive());
      }
      GetContactedNodes(newEdges, ContactedNodes);
   }while (!newEdges.IsEmpty());
}

void SNLayer::GetContactedNodes(const CArray< SNEdge*, SNEdge* >& ActiveEdges, CArray <SNNode*>& ContactedNodes)
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
*/


/* Each org probabilistically decides to add/delete edge depending on trust and trust in neighborhood (and change in trust for last few time steps?) This is meant to capture bridging capital

for each organization
   calculate trust(time_t) (and trust(time_t) for each actor group)
      compare with trust(time_t-1)
   calculate agg trust for this layer
   calculate change in utility  and decide to make change(s) - track changes: 'bridging edge', new bonding edge, deleted edge with similar/dissimilar org


parts to utility function:
   value similarity
   degree (similarity but also for 'reputation'/'popularity'/'network influence')
   agg trust in neighborhood (for now - just neighbors)
   transitivity


*/

/*
bool SNLayer::GetDegreeCentrality()
{//for each interior node, iterate through m_inCountTable and m_outCountTable and add the edges to find the number of edges the org has that are between itself and another org.
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
*/




/*

bool SNLayer::SetTrustTable()
{
   m_trustTable.clear();
   for (int i = 0; i < m_edges.GetSize(); i++)
   {
      SNEdge *pEdge = m_edges[i];
      if (m_trustTable.find(pEdge->m_pToNode->m_name) == m_trustTable.end())
      {
         m_outCountTable[pEdge->m_pToNode->m_name] = 0;
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



void SNLayer::ShuffleNodes(SNNode** nodeArray, int nodeCount)
   {
   //--- Shuffle elements by randomly exchanging each with one other.
   for (int i = 0; i < nodeCount - 1; i++)
      {
      int randVal = (int)m_pSNIPModel->m_randShuffle.RandValue(0, nodeCount - i - 0.0001f);

      int r = i + randVal; // Random remaining position.
      SNNode* temp = nodeArray[i];
      nodeArray[i] = nodeArray[r];
      nodeArray[r] = temp;
      }
   }


//////////////////////////////////////////////////////////////////////////////
//    S O C I A L    N E T W O R K
//////////////////////////////////////////////////////////////////////////////

//   SNIP() : m_deltaTolerance( 0.0001f ), m_maxIterations( 100 ) { }
SNIP::SNIP(SNIP& sn)
   : m_layerArray(sn.m_layerArray)
   {
   for (int i = 0; i < (int)m_layerArray.GetSize(); i++)
      m_layerArray[i]->m_pSNIP = this;
   }



bool SNIP::Init(EnvContext *pEnvContext, LPCTSTR filename)
   {
   // build network edges base in initStr.
   // Note that:
   //    1) Input nodes are required for landscape feedbacks and any external drivers
   //    2) Output nodes are required for each actor's values (one per actor/value pair)
   //    3) Interior nodes represent the social network

   bool ok = LoadXml(pEnvContext, filename); //, pEnvContext );  // defines basic nodes/network structure, connects inputs and outputs

   for (int i = 0; i < GetLayerCount(); i++)
      {
      SNLayer* pLayer = GetLayer(i);
      pLayer->Init();
      }

   return TRUE;
   }


bool SNIP::InitRun(EnvContext* pEnvContext, bool)
   {
   // reset all weights to their initial states
   for (int i = 0; i < this->GetLayerCount(); i++)
      {
      SNLayer* pLayer = GetLayer(i);
      //pLayer->Init();
      }

   return true;
   }


bool SNIP::Run(EnvContext* pEnvContext)
   {
   MapLayer* pIDULayer = (MapLayer*) pEnvContext->pMapLayer;

   for (int i = 0; i < this->GetLayerCount(); i++)
      {
      SNLayer* pLayer = GetLayer(i);

      // update network if needed
      if (pLayer->m_pSNIPModel->m_colAdapt > 0)
         {
         // for each landscape actor node, modify incoming edges trust values
         // based on IDU informtion
         float adapt = 0;
         for (int j = 0; j < pLayer->GetNodeCount(); j++)
            {
            SNNode* pNode = pLayer->GetNode(j);
            if (pNode->m_idu >= 0 && pNode->IsLandscapeActor())
               {
               // get adaptive factor from map
               pIDULayer->GetData(pNode->m_idu, pLayer->m_pSNIPModel->m_colAdapt, adapt);

               // apply to each edge
               for (int k = 0; k < pNode->m_inEdges.GetSize(); k++)
                  pNode->m_inEdges[k]->m_trust *= adapt;
               }
            }
         }

      Report::Log_s("Running SNIP Model %s", pLayer->m_pSNIPModel->m_name.c_str());
      pLayer->m_pSNIPModel->RunSimulation(true, false);

      // write actor reactivities to associated IDUs
      for (int j = 0; j < pLayer->GetNodeCount(); j++)
         {
         SNNode* pNode = pLayer->GetNode(j);
         if (pNode->m_idu >= 0)
            {
            this->UpdateIDU(pEnvContext, pNode->m_idu, pLayer->m_colReactivity, pNode->m_reactivity);
            }
         }

      if (pLayer->m_exportNetworkInterval > 0)
         {
         CString outpath(PathManager::GetPath(PM_OUTPUT_DIR));
         CString scname;
         Scenario* pScenario = ::EnvGetScenario(pEnvContext->pEnvModel, pEnvContext->scenarioIndex);

         CString path;
         path.Format("%sSNIP_%s_Year%i_%s_Run%i.gexf", (LPCTSTR)outpath, pLayer->m_name.c_str(), pEnvContext->currentYear, (LPCTSTR) pScenario->m_name, pEnvContext->run);

         pLayer->ExportNetworkGEXF(path);
         }
      }

   CollectData(pEnvContext);

   return true;
   }



bool SNIP::CollectData(EnvContext* pEnvContext)
   {
   for (int i = 0; i < this->GetLayerCount(); i++)
      {
      SNLayer* pLayer = GetLayer(i);
      SNIPModel *pModel = pLayer->m_pSNIPModel;

      pModel->CollectData(pEnvContext);
      }

   return true;
   }


bool SNIP::LoadXml(EnvContext *pEnvContext, LPCTSTR filename)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // assoc_net

   // top level is layers
   TiXmlElement* pXmlLayers = pXmlRoot->FirstChildElement("layers");
   if (pXmlLayers == nullptr)
      {
      CString msg;
      msg.Format(_T("Error reading SNIP input file %s: Missing <layers> entry.  This is required to continue..."), filename);
      Report::ErrorMsg(msg);
      return false;
      }

   TiXmlElement* pXmlLayer = pXmlLayers->FirstChildElement("layer");
   if (pXmlLayer == nullptr)
      {
      CString msg;
      msg.Format(_T("Error reading SNIP input file %s: Missing <layer> entry.  This is required to continue..."), filename);
      Report::ErrorMsg(msg);
      return false;
      }

   while (pXmlLayer != nullptr)
      {
      LPTSTR name = nullptr, path = nullptr, mappings = nullptr, mappingQuery=nullptr, reactivityCol=nullptr, adaptCol=nullptr,landscapeSignal=nullptr;
      int exportNetworks = -1;
      float mappingFrac = 0;
      XML_ATTR attrs[] = { // attr             type        address           isReq checkCol
                         { "name",             TYPE_STRING,  &name,            true,  0 },
                         { "path",             TYPE_STRING,  &path,            true,  0 },
                         { "mappings",         TYPE_STRING,  &mappings,        true,  0 },
                         { "mapping_query",    TYPE_STRING,  &mappingQuery,    true,  0 },
                         { "reactivity_col",   TYPE_STRING,  &reactivityCol,   true,  0 },
                         { "adapt_col",        TYPE_STRING,  &adaptCol,        false, 0 },
                         { "landscape_signal", TYPE_STRING,  &landscapeSignal, true,  0 },
                         { "mapping_fraction", TYPE_FLOAT,   &mappingFrac,     true,  0 },
                         { "export_networks",  TYPE_INT,     &exportNetworks,  false,  0 },
                         { nullptr,            TYPE_NULL,    nullptr,          false, 0 } };

      bool ok = TiXmlGetAttributes(pXmlLayer, attrs, filename);

      if (ok)
         {
         // have layer info, allocate layer and start reading
         SNLayer* pLayer = this->AddLayer();
         if (name != nullptr)
            pLayer->m_name = name;

         if (path != nullptr)
            ok = pLayer->m_pSNIPModel->LoadSnipNetwork(path, false);  // ignore landscape actor nodes

         EnvEvaluator *pEvaluator = ::EnvFindEvaluatorInfo(pEnvContext->pEnvModel, landscapeSignal);
         if (pEvaluator == nullptr)
            {
            CString msg;
            msg.Format("Unable to find signal source %s.  This must be fixed to run SNIP...", landscapeSignal);
            Report::WarningMsg(msg);
            }
         else
            {
            pLayer->m_pSNIPModel->m_pEvaluator = pEvaluator;
            }

         CheckCol(pIDULayer, pLayer->m_colReactivity, reactivityCol, TYPE_FLOAT, CC_AUTOADD);
         if ( adaptCol != nullptr && adaptCol[0] != 0)
            CheckCol(pIDULayer, pLayer->m_pSNIPModel->m_colAdapt, adaptCol, TYPE_FLOAT, CC_AUTOADD);

         if (mappingQuery != nullptr)
            {
            pLayer->m_pSNIPModel->m_mappingQuery = mappingQuery;
            pLayer->m_pSNIPModel->m_pMappingQuery = pEnvContext->pQueryEngine->ParseQuery(mappingQuery, 0, "MappingQuery");
            }

         if (mappings != nullptr)
            ok = pLayer->m_pSNIPModel->ConnectToIDUs((MapLayer*)pEnvContext->pMapLayer, mappings, mappingFrac);

         pLayer->m_exportNetworkInterval = exportNetworks;

         // network is fully created at this point, check for any problems
         //pLayer->CheckNetwork();         
         }  // end of: if ( layer is ok )

      pXmlLayer = pXmlLayer->NextSiblingElement("layer");
      }

   return true;
   }



bool SNIP::RemoveLayer(LPCTSTR name)
   {
   for (INT_PTR i = 0; i < this->m_layerArray.GetSize(); i++)
      {
      ////if ( m_layerArray[ i ]->m_name.CompareNoCase( name ) == 0 )
      ////   {
      ////   m_layerArray.RemoveAt( i );
      ////   return true;
      ////   }
      }

   return false;
   }

