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
// ResultsPanel.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "EnvDoc.h"
#include <EnvModel.h>
#include <DataManager.h>
#include "DeltaViewer.h"
#include "ResultsPanel.h"
#include "ResultsMapWnd.h"
#include "ResultsGraphWnd.h"
#include "Visualizer.h"
#include "EnvView.h"
#include <SocialNetwork.h>
#include <PathManager.h>
#include <EnvConstants.h>

#include "LulcTransitionResultsWnd.h"
//#include "PolicyPriorityPlot.h"
#include "GoalSpaceWnd.h"
#include "PolicyAppPlot.h"
#include "TrajectorySpaceWnd.h"
#include <Policy.h>
#include "MapPanel.h"

#include <scatterwnd.h>
#include <dynamicscatterwnd.h>
#include "MultirunHistogramPlotWnd.h"
#include <datatable.h>
#include <map.h>
#include <unitconv.h>
#include <FolderDlg.h>
#include <DirPlaceHolder.h>

#include <direct.h>

extern ResultsPanel  *gpResultsPanel;
extern CEnvDoc       *gpDoc;
extern EnvModel      *gpModel;
extern MapLayer      *gpCellLayer;
extern CEnvView      *gpView;
extern PolicyManager *gpPolicyManager;
extern MapPanel      *gpMapPanel;

// ResultsPanel

//ResultInfoArray ResultsPanel::m_resultInfo;
ResultNodeTree ResultsPanel::m_resultTree;

IMPLEMENT_DYNAMIC(ResultsPanel, CWnd)


ResultNode::ResultNode( ResultNode &r )
: m_pParent( r.m_pParent )
, m_category( r.m_category )
, m_run( r.m_run )
, m_extra( r.m_extra )
, m_show( r.m_show )
, m_type( r.m_type )
, m_text( r.m_text )
, m_isExpanded( false )
   {
   for ( int i=0; i < m_children.GetCount(); i++ )
      m_children.Add( new ResultNode( *r.m_children.GetAt( i ) ) );
   }


ResultNode::~ResultNode()
   {
   for ( int i=0; i < m_children.GetSize(); i++ )
      delete m_children.GetAt( i );

   m_children.RemoveAll();
   }
   


ResultNodeTree::ResultNodeTree()
 : m_pRootNode( NULL )
   {
   m_pRootNode = new ResultNode(NULL, VC_ROOT, -1, 0, true, VT_ROOT, _T("Root") );
   }
   

ResultNode *ResultNodeTree::AddNode( ResultNode *pParent, int _run, RESULT_CATEGORY _category, INT_PTR _extra, RESULT_TYPE _type, LPCTSTR _text )
   {
   ResultNode *pNode = new ResultNode( pParent, _category, _run, _extra, true, _type, _text );
   pParent->m_children.Add( pNode );

   return pNode;
   }


ResultsPanel::ResultsPanel()
:  m_pResultsWnd( NULL ),
   m_pTreeWnd( NULL ),
   m_pTree( NULL )
   {
   gpResultsPanel = this;
   }


ResultsPanel::~ResultsPanel()
   {
   if ( m_pResultsWnd != NULL )
      delete m_pResultsWnd;

   if ( m_pTreeWnd != NULL )
      delete m_pTreeWnd;
   }


BEGIN_MESSAGE_MAP(ResultsPanel, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()



int ResultsPanel::AddDynamicGraphNodes( ResultNode *pGraphNode )
   {
   int run = pGraphNode->m_run;

   ResultNode *pEvaluatorNode = AddTreeNode( pGraphNode, run, VC_OTHER, 0, VT_GRAPHS, _T("Eval Model Scores") );

   // add eval model raw scores
   for ( int i=0; i < gpModel->GetResultsCount(); i++ )
      {
      EnvEvaluator *pModel = gpModel->GetResultsEvaluatorInfo( i );
      ASSERT( pModel->m_showInResults );
      AddTreeNode( pEvaluatorNode, run, VC_GRAPH, i, VT_GRAPH_DYNAMIC, pModel->m_name );
      }

   // add node for AppVars
   if ( gpModel->GetAppVarCount( AVT_APP ) > 0 )
      {
      ResultNode *pAppVarNode = AddTreeNode( pGraphNode, run, VC_OTHER, 0, VT_GRAPH_APPVAR, _T("Application Variables") );

      for ( int i=0; i < gpModel->GetAppVarCount(); i++ )
         {
         AppVar *pVar = gpModel->GetAppVar( i );

         if ( pVar->m_avType == AVT_APP )
            AddTreeNode( pAppVarNode, run, VC_GRAPH, (INT_PTR) pVar, VT_GRAPH_APPVAR, pVar->m_name );
         }
      }

   // add Model output variables
   int apCount = gpModel->GetModelProcessCount();
   int extra = 1;  // the column in the DataManager DataObj that contains the model output values

   for ( int i=0; i < apCount; i++ )
      {
      EnvModelProcess *pInfo = gpModel->GetModelProcessInfo( i );
      //if ( pInfo->outputVarFn != NULL )
         {
         MODEL_VAR *modelVarArray = NULL;
         int varCount = pInfo->OutputVar( pInfo->m_id, &modelVarArray );

         if ( varCount > 0 )
            {
            ResultNode *pParentNode = NULL;
            ResultNode *pModelNode = AddTreeNode( pGraphNode, run, VC_MODEL_OUTPUTS, i+1, VT_GRAPH_MODEL_OUTPUTS, pInfo->m_name );
      
            //ResultNode *pLastNode = pModelNode;
            for ( int j=0; j < varCount; j++ )
               {
               MODEL_VAR &mv = modelVarArray[ j ];

               if ( mv.pVar == NULL  )   // this is a label only - then add a parent node for the following 
                  pParentNode = AddTreeNode( pModelNode, run, VC_LEVEL1_NODE, extra, VT_GRAPH_MODEL_OUTPUT, mv.name );
               
               else     // it is a regular model variable.  If there is a currently active parent node and this
                  {     // modelVar is level 0, reset the pParentNode to 0
                  if ( pParentNode != NULL && mv.flags == 0)
                     pParentNode = NULL;

                  if ( pParentNode )
                     AddTreeNode( pParentNode, run, VC_GRAPH, extra, VT_GRAPH_MODEL_OUTPUT, mv.name );
                  else
                     AddTreeNode( pModelNode, run, VC_GRAPH, extra, VT_GRAPH_MODEL_OUTPUT, mv.name );

                  extra++;
                  }
               }
            }
         }
      }

   int emCount = gpModel->GetEvaluatorCount();

   for ( int i=0; i < emCount; i++ )
      {
      EnvEvaluator *pInfo = gpModel->GetEvaluatorInfo( i );
      //if ( pInfo->outputVarFn != NULL )
         {
         MODEL_VAR *modelVarArray = NULL;
         int varCount = pInfo->OutputVar( pInfo->m_id, &modelVarArray );

         if ( varCount > 0 )
            {
            ResultNode *pParentNode = NULL;
            ResultNode *pModelNode = AddTreeNode( pGraphNode, run, VC_MODEL_OUTPUTS, -(i+1), VT_GRAPH_MODEL_OUTPUTS, pInfo->m_name );

            for ( int j=0; j < varCount; j++ )
               {
               MODEL_VAR &mv = modelVarArray[ j ];

               if ( mv.pVar == NULL  )   // this is a label only - then add a parent node for the following 
                  pParentNode = AddTreeNode( pModelNode, run, VC_LEVEL1_NODE, extra, VT_GRAPH_MODEL_OUTPUT, mv.name );
               
               else     // it is a regular model variable.  If there is a currently active parent node and this
                  {     // modelVar is level 0, reset the pParentNode to 0
                  if ( pParentNode != NULL && mv.flags == 0)
                     pParentNode = NULL;

                  if ( pParentNode )
                     AddTreeNode( pParentNode, run, VC_GRAPH, extra, VT_GRAPH_MODEL_OUTPUT, mv.name );
                  else
                     AddTreeNode( pModelNode, run, VC_GRAPH, extra, VT_GRAPH_MODEL_OUTPUT, mv.name );

                  extra++;
                  }
               }
            }
         }
      }   
      
   // dynamic policy info (probe).  Only add for policies in use in the current scenario
   int policyCount  = gpPolicyManager->GetPolicyCount();
   if ( policyCount > 0 )
      {
      ResultNode *pPolicyNode = AddTreeNode( pGraphNode, run, VC_POLICY_STATS, 0, VT_GRAPH_POLICY_STATS, _T("Policy Stats") );

      for ( int i=0; i < policyCount; i++ )
         {
         Policy *pPolicy = gpPolicyManager->GetPolicy( i );

         if ( pPolicy->m_use )   // currently in use?
            {
            AddTreeNode( pPolicyNode, run, VC_GRAPH, (INT_PTR) i+1, VT_GRAPH_POLICY_STATS, pPolicy->m_name );
            }
         }
      }

   // dynamic social network info (probe).  Only add is social networks enabled
   if ( gpModel->m_pSocialNetwork != NULL )
      {
      int layerCount = gpModel->m_pSocialNetwork->GetLayerCount();

      if ( layerCount > 0 )
         {
         ResultNode *pSNNode = AddTreeNode( pGraphNode, run, VC_SOCIAL_NETWORK, 0, VT_GRAPH_SOCIAL_NETWORK, _T("Social Network Metrics") );

         for ( int i=0; i < layerCount; i++ )
            AddTreeNode( pSNNode, run, VC_GRAPH, (INT_PTR) i+1, VT_GRAPH_SOCIAL_NETWORK, gpModel->m_pSocialNetwork->GetLayer( i )->m_name.c_str() );
         }
      }

   return extra-1;
   }

/*
LPCTSTR ResultsPanel::GetResultText( RESULT_TYPE type )
   {
   switch( type )
      {
      case VT_MAP_DYNAMIC:      // for dynamically defined maps
         break;

      case VT_GRAPH_DYNAMIC:    // for dynamically defined graphs
         break; 

      case VT_GRAPH_MODEL_OUTPUT:
         break;

      default:
         for ( int i=0; i < m_resultInfo.GetSize(); i++ )
            if ( m_resultInfo[ i ].type == (int) type )
               return (LPCTSTR) m_resultInfo[ i ].text;
      }

   return NULL;
   }
*/
/*
int ResultsPanel::GetResultCategory( RESULT_TYPE type )
   {
   for ( int i=0; i < m_resultInfo.GetSize(); i++ )
      if ( m_resultInfo[ i ].type == (int) type )
         return m_resultInfo[ i ].category;

   return -1;
   }
*/


// ResultsPanel message handlers

int ResultsPanel::OnCreate(LPCREATESTRUCT lpCreateStruct) 
   {
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   m_pTreeWnd    = new ResultsTree;
   m_pResultsWnd = new ResultsWnd;
   m_splitterWnd.LeftRight( m_pTreeWnd, m_pResultsWnd );
   m_splitterWnd.MoveBar( 800 );
   m_splitterWnd.Create( NULL, NULL, WS_VISIBLE, CRect(0,0,0,0), this, 101 );
   m_pTree = &m_pTreeWnd->m_tree;

	return 0;
   }


void ResultsPanel::OnSize(UINT nType, int cx, int cy) 
   {
	CWnd::OnSize(nType, cx, cy);

   if ( IsWindow( m_splitterWnd.m_hWnd ) )
      m_splitterWnd.MoveWindow( 0, 0, cx, cy, TRUE );
   }


bool ResultsPanel::AddRun( int run )
   {
   RUN_INFO &runInfo = gpModel->m_pDataManager->GetRunInfo( run );

   CString runStr;
   runStr.Format( "Run %i (%s)", run, (LPCTSTR) runInfo.pScenario->m_name );

   // run
   ResultNode *pRunNode = AddTreeNode( NULL, run, VC_RUN, 0, VT_RUN, runStr );
   
   // maps
   ResultNode *pMapNode = AddTreeNode( pRunNode, run, VC_OTHER, 0, VT_MAPS, _T("Maps") );

   ResultNode *pSubNode = NULL;
   
   // add dynamics maps.  The basic idea is that we will iterate through the FieldInfo to populate this menu
   // which any fields for which show in results is turned on will be added to the ResultNodeTree
   int count = gpCellLayer->GetFieldInfoCount( 1 );
   for ( int j=0; j < count; j++ )
      {
      MAP_FIELD_INFO *pInfo = gpCellLayer->GetFieldInfo(j);

      // first, if submenu, see if there are any children that are visible
      if ( pInfo->IsSubmenu() )
         {
         pInfo->SetExtra( 0 );      // assume not to be displayed

         for ( int k=0; k < count; k++ )
            {
            MAP_FIELD_INFO *_pInfo = gpCellLayer->GetFieldInfo( k );

            // if parent matches and child is visible?
            if ( pInfo == _pInfo->pParent && _pInfo->GetExtraBool( 1 ) )
               {
               pInfo->SetExtra( 1 );
               break;
               }
            }

         // should we show this submenu?
         if ( pInfo->GetExtraBool( 1 ) )
            pSubNode = AddTreeNode( pMapNode, run, VC_MAP, j, VT_MAP_DYNAMIC, pInfo->label );
         }  // end of: if ( IsSubMenu )

      else // it is not a submenu
         {
         // is it a top-level item? or nested in a submenu
         // Add the node.  The ItemData will contain a ptr to the corresponding ResultNode

         if ( pInfo->pParent == NULL )  // is this a top level field? (not attached to a subMenu)
            AddTreeNode( pMapNode, run, VC_MAP, j, VT_MAP_DYNAMIC, pInfo->label );

         else  // it is a child, find the parent node and put it as a child under the parent node
            AddTreeNode( pSubNode, run, VC_MAP, j, VT_MAP_DYNAMIC, pInfo->label );
         }
      }  // end of: for ( j < count )

   // add map visualizers
   for ( int j=0; j < gpModel->GetVisualizerCount(); j++ )
      {
      EnvVisualizer *pViz = gpModel->GetVisualizerInfo( j );

      if ( pViz->m_vizType & VZT_POSTRUN_MAP )
         AddTreeNode( pMapNode, run, VC_MAP, j, VT_MAP_VISUALIZER, pViz->m_name );
      }

   // add graphs
   ResultNode *pGraphNode = AddTreeNode( pRunNode, run, VC_OTHER, 0, VT_GRAPHS, _T("Graphs") );
   //m_pTree->SetItemData( lastGraph, MAKELONG( short(-2), (WORD) run ) );
   AddDynamicGraphNodes( pGraphNode );

   // add graph visualizers
   for ( int j=0; j < gpModel->GetVisualizerCount(); j++ )
      {
      EnvVisualizer *pViz = gpModel->GetVisualizerInfo( j );
      if ( pViz->m_vizType & VZT_POSTRUN_GRAPH )
         AddTreeNode( pGraphNode, run, VC_GRAPH, j, VT_GRAPH_VISUALIZER, pViz->m_name );
      }

   int lulcLevels = gpModel->m_lulcTree.GetLevels();

   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_EVAL_SCORES,    _T("Evaluative Scores (Scaled)") );
   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_EVAL_RAWSCORES, _T("Evaluative Scores (Raw)") );
   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_POLICY_APP,     _T("Policy Application Rates") );
   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_POLICY_EFFECTIVENESS_TREND_DYNAMIC, _T("Policy Effectiveness Trends (Phase Plot)") );
   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_POLICY_EFFECTIVENESS_TREND_STATIC,  _T("Policy Effectiveness Trends") );
   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_ACTOR_WTS,  _T("Actor Weight Trajectories") );
   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_ACTOR_COUNTS,  _T("Actor Counts") );

   if ( gpPolicyManager->GetGlobalConstraintCount() > 0 )
      AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_GLOBAL_CONSTRAINTS, _T("Global Constraints") );
   
   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_LULCA_TREND,    _T("Lulc A Trends") );
   if ( lulcLevels > 1 )
      AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_LULCB_TREND, _T("Lulc B Trends") );

   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_POP_BY_LULCA,   _T("Population by LulcA Trends") );
   AddTreeNode( pGraphNode, run, VC_GRAPH, 0, VT_GRAPH_EVALSCORES_BY_LULCA, _T("Evaluative Scores by LulcA") );

   // view tree
   ResultNode *pViewNode = AddTreeNode( pRunNode, run, VC_OTHER, 0, VT_VIEWS, _T("Views") );
   //m_pTree->SetItemData( lastView, MAKELONG( short(-3), (WORD) run ) );
   
   AddTreeNode( pViewNode, run, VC_VIEW, 0, VT_VIEW_GOALSPACE,   _T("Goal Space") );
   AddTreeNode( pViewNode, run, VC_VIEW, 0, VT_VIEW_POLICY_APP,  _T("Policy Application Rates") ); 
   AddTreeNode( pViewNode, run, VC_VIEW, 0, VT_VIEW_LULC_TRANS,  _T("Lulc Transitions") );
   AddTreeNode( pViewNode, run, VC_VIEW, 0, VT_VIEW_DELTA_ARRAY, _T("Delta Array") );

   // add view visualizers
   for ( int j=0; j < gpModel->GetVisualizerCount(); j++ )
      {
      EnvVisualizer *pViz = gpModel->GetVisualizerInfo( j );
      if ( pViz->m_vizType & VZT_POSTRUN_VIEW )
         AddTreeNode( pViewNode, run, VC_VIEW, j, VT_VIEW_VISUALIZER, pViz->m_name );
      }

   // data table tree
   ResultNode *pDataTableNode = AddTreeNode( pRunNode, run, VC_OTHER, 0, VT_DATATABLES, _T("Tables") );
   //m_pTree->SetItemData( lastDataTable, MAKELONG( short(-4), (WORD) run ) );

   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_EVAL_SCORES,    _T("Evaluative Scores (Scaled)") );
   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_EVAL_RAWSCORES, _T("Evaluative Scores (Raw)") );
   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_POLICY_APP,     _T("Policy Application Rates") );
   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_POLICY_EFFECTIVENESS, _T("Policy Effectiveness Trends") );
   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_POLICY_SUMMARY, _T("Summary of Policy Applications") );
   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_POLICIES_BY_ACTOR_AREA, _T("Policy Application (Area) By Actor") );
   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_POLICIES_BY_ACTOR_COUNT, _T("Policy Application (Counts) By Actor") );
   
   if ( gpPolicyManager->GetGlobalConstraintCount() > 0 )
      AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_GLOBAL_CONSTRAINTS, _T("Global Constraints") );
   
   if ( gpPolicyManager->GetPolicyCount() > 0 )
      AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_POLICY_STATS, _T("Policy Stats") );

   //AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_LULC_TRANS,    _T("Lulc Transition Matrix (For Debug)" );
   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_LULCA_TREND,    _T("%Lulc A Trends") );

   if( lulcLevels > 1 )
      AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_LULCB_TREND, _T("%Lulc B Trends" ) );

   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_DATA_POP_BY_LULCA,   _T("Population by LulcA Trends") );
   AddTreeNode( pDataTableNode, run, VC_DATATABLE, 0, VT_GRAPH_EVALSCORES_BY_LULCA, _T("Evaluative Scores by LulcA") );

   PopulateTreeCtrl();

   m_pTree->Expand( pRunNode->m_hItem, TVE_EXPAND );

   // m_pTree->Expand( lastRun, TVE_EXPAND );
   // m_pTree->Expand( lastMap, TVE_EXPAND );
   // m_pTree->Expand( lastGraph, TVE_EXPAND );
   // m_pTree->Expand( lastView, TVE_EXPAND );
   // m_pTree->Expand( lastDataTable, TVE_EXPAND );
   
   return true;
   }


bool ResultsPanel::AddMultiRun( int multiRun )
   {
   MULTIRUN_INFO &multirunInfo = gpModel->m_pDataManager->GetMultiRunInfo( multiRun );

   CString runStr;
   runStr.Format( "MultiRun %i (%s)", multiRun, (LPCTSTR) multirunInfo.pScenario->m_name );

   ResultNode *pMultiRunNode = AddTreeNode( NULL, multiRun, VC_MULTIRUN, 0, VT_MULTIRUN, runStr );
   //m_pTree->SetItemData( lastMultiRun, MAKELONG( short(-1001), (WORD) multiRun ) );  // low,high

   // multirun maps
   ResultNode *pMapNode = AddTreeNode( pMultiRunNode, multiRun, VC_OTHER, 0, VT_MULTIMAPS, _T("Maps") );
   //m_pTree->SetItemData( lastMap, MAKELONG( short(-5), (WORD) multiRun ) );
 
   AddTreeNode( pMapNode, multiRun, VC_MULTIRUN_MAP, 0, VT_MAP_MULTI_SCENARIO_VULNERABILITY, _T("Landscape Vulnerability") );   

   // multirun graph tree
   ResultNode *pGraphNode = AddTreeNode( pMultiRunNode, multiRun, VC_OTHER, 0, VT_MULTIGRAPHS, _T("Graphs") );
   AddTreeNode( pGraphNode, multiRun, VC_MULTIRUN_GRAPH, 0, VT_GRAPH_MULTI_DYNAMIC,         _T("Dynamic MultiRun Graphs") );
   AddTreeNode( pGraphNode, multiRun, VC_MULTIRUN_GRAPH, 0, VT_GRAPH_MULTI_EVAL_SCORES,     _T("Evaluative Scores") );
   AddTreeNode( pGraphNode, multiRun, VC_MULTIRUN_GRAPH, 0, VT_GRAPH_MULTI_EVAL_SCORE_FREQ, _T("Evaluative Scores Frequency Distribution") );
   AddTreeNode( pGraphNode, multiRun, VC_MULTIRUN_GRAPH, 0, VT_GRAPH_MULTI_PARAMETERS,      _T("Scenario Parameter Settings") );
   AddTreeNode( pGraphNode, multiRun, VC_MULTIRUN_GRAPH, 0, VT_GRAPH_MULTI_PARAMETERS_FREQ, _T("Scenario Parameters Setting Frequency Distribution") );

   // multirun views
   ResultNode *pViewNode = AddTreeNode( pMultiRunNode, multiRun, VC_OTHER, 0, VT_MULTIVIEWS, _T("Views") );
   AddTreeNode( pViewNode, multiRun, VC_MULTIRUN_VIEW, 0, VT_VIEW_MULTI_TRAJECTORYSPACE, _T("Scenario Trajectories (Broom in Box)") );

   // multirun data tables
   ResultNode *pDataTableNode = AddTreeNode( pMultiRunNode, multiRun, VC_OTHER, 0, VT_MULTITABLES, _T("Tables") );
   //HTREEITEM lastMap = m_pTree->InsertItem( "Maps", 0, 0, lastMultiRun, TVI_LAST );
   AddTreeNode( pDataTableNode, multiRun, VC_MULTIRUN_DATATABLE, 0, VT_DATA_MULTI_EVAL_SCORES,     _T("Evaluative Scores") );
   AddTreeNode( pDataTableNode, multiRun, VC_MULTIRUN_DATATABLE, 0, VT_DATA_MULTI_EVAL_SCORE_FREQ, _T("Evaluative Scores Frequency Distribution") );
   AddTreeNode( pDataTableNode, multiRun, VC_MULTIRUN_DATATABLE, 0, VT_DATA_MULTI_PARAMETERS,      _T("Scenario Parameter Settings") );
   AddTreeNode( pDataTableNode, multiRun, VC_MULTIRUN_DATATABLE, 0, VT_DATA_MULTI_PARAMETERS_FREQ, _T("Scenario Parameter Settings Frequency Distribution") );

   PopulateTreeCtrl();

   m_pTree->Expand( pMultiRunNode->m_hItem, TVM_EXPAND );

   //m_pTree->Expand( lastMultiRun, TVE_EXPAND );
   //m_pTree->Expand( lastMap, TVE_EXPAND );
   //m_pTree->Expand( lastGraph, TVE_EXPAND );
   //m_pTree->Expand( lastView, TVE_EXPAND );
   //m_pTree->Expand( lastDataTable, TVE_EXPAND );

   return true;
   }


ResultNode *ResultsPanel::AddTreeNode( ResultNode *pParent, int run, RESULT_CATEGORY category, INT_PTR extra, RESULT_TYPE resultType, LPCTSTR label )
   {
   ResultNode *_pParent = pParent;

   if ( pParent == NULL )
      _pParent = m_resultTree.m_pRootNode;

   //HTREEITEM hParentItem;
   //if ( pParent == NULL )
   //   hParentItem = TVI_ROOT;
   //else
   //   hParentItem = _pParent->m_hItem;

   ResultNode *pNode = m_resultTree.AddNode( _pParent, run, category, extra, resultType, label );
   //HTREEITEM hItem = m_pTree->InsertItem( label, 0, 0, hParentItem, TVI_LAST );
   //m_pTree->SetItemData( hItem, (DWORD_PTR) pNode );
   //pNode->m_hItem = hItem;

   return pNode;
   }


int ResultsPanel::PopulateTreeCtrl( void )
   {
   // this method populates the attached CTreeCtrl based on the existing ResultsNodeTree
   // Nodes in the tree are created if the corresponding result passes the filter test

   m_pTree->DeleteAllItems();

   ResultNode *pRootNode = m_resultTree.m_pRootNode;

   SetNodeVisibility( pRootNode, 0 );

   int count = 0;
   for ( int i=0; i < pRootNode->m_children.GetSize(); i++ )
      {
      ResultNode *pNode = pRootNode->m_children[ i ];
      count += PopulateTreeCtrl( pNode, 0 );  // these will be the "Run N" nodes
      }

   return count;
   }


int ResultsPanel::SetNodeVisibility( ResultNode *pResultNode, int level )
   {
   // if ANY of it's children should be shown we show this one regardless of whether it passes
   // the filter
   int shownCount = 0;
   for ( int i=0; i < (int) pResultNode->m_children.GetSize(); i++ )
      {
      int _shownChildren = SetNodeVisibility( pResultNode->m_children[ i ], level+1 );

      shownCount += _shownChildren;
      }

   // should we show this node?
   bool showThis = true;
   if ( level > 0 && m_pTreeWnd->m_filterStr.GetLength() > 0 )  // never filter first, second level
      {
      // do we match the filter?
      CString lower = pResultNode->m_text;
      lower.MakeLower();
      if ( lower.Find( m_pTreeWnd->m_filterStr ) < 0 )
         showThis = false;  // no, see flag this as a not show
      }

   if ( showThis )
      shownCount++;

   pResultNode->m_show = shownCount > 0 ? true : false;

   return shownCount;
   }


// first time through, pResultNode = root 
int ResultsPanel::PopulateTreeCtrl( ResultNode *pResultNode, int level )
   {
   // level 0 = "Run N"
   // level 1 = Map, Graph, Table, View
   // etc...

   if ( level > 0  && pResultNode->m_show == false )  // always show first level (run)
      return 0;

   ASSERT( pResultNode->m_pParent != NULL );

   HTREEITEM hParentItem = 0;
   if ( level == 0 )
      hParentItem = TVI_ROOT;
   else
      hParentItem = pResultNode->m_pParent->m_hItem;

   HTREEITEM hItem = m_pTree->InsertItem( pResultNode->m_text, hParentItem, TVI_LAST );
   m_pTree->SetItemData( hItem, (DWORD_PTR) pResultNode );
   pResultNode->m_hItem = hItem;

   // add children
   int count = 0;
   for ( int i=0; i < (int) pResultNode->m_children.GetSize(); i++ )
      count += PopulateTreeCtrl( pResultNode->m_children[ i ], level + 1 );

    //m_pTree->Expand( hItem, pResultNode->m_isExpanded ? TVE_EXPAND : TVE_COLLAPSE );
   m_pTree->Expand( hItem, TVE_EXPAND );

   return count;
   }


void ResultsPanel::AddMapWnd( ResultNode *pNode )  //RESULT_TYPE type, int run, int fieldInfoOffset )
   {
   CWaitCursor c;
   RECT rect;
   m_pResultsWnd->GetClientRect( &rect );

   RESULT_TYPE type = pNode->m_type;
   int run   = pNode->m_run;
   int fieldInfoOffset = (int) pNode->m_extra;

   RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );
   //MapWnd *pMapWnd = NULL;

   switch( type )
      {
      case VT_MAP_DYNAMIC:         
         {
         ASSERT( fieldInfoOffset >=0 );

         // make sure all current ResultsMaps are rolled back to year=0 (this
         // is required because DeltaArray's must be sync'ed up)
         m_pResultsWnd->ReplayReset();

         MAP_FIELD_INFO *pInfo = gpCellLayer->GetFieldInfo( fieldInfoOffset );

         CString title = "Map";
         if ( pInfo )
            title = pInfo->label;

         title.AppendFormat( " - Run %i (%s)", run, (LPCTSTR) ri.pScenario->m_name );

         ResultsMapWnd *pResultsMapWnd = new ResultsMapWnd( run, fieldInfoOffset, false );
         ASSERT( pResultsMapWnd != NULL );

         int nID = 4000+m_pTree->GetCount();

         try
            {
            pResultsMapWnd->Create( NULL, title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, nID );

            Map *pMap = pResultsMapWnd->m_pMap;

            if ( gpMapPanel->m_mapFrame.m_pMapWnd->GetBkgrImage() != NULL )
               pResultsMapWnd->m_mapFrame.m_pMapWnd->CopyBkgrImage( gpMapPanel->m_mapFrame.m_pMapWnd );

            MapLayer *pLayer = pMap->GetLayer( 0 );

            pLayer->AddFieldInfo( pInfo );  // makes a copy of the MAP_FIELD_INFO

            Map::SetupBins( pMap, 0, 0, true );  // ClassifyData() is called in here
            pResultsMapWnd->RefreshList(); 

            m_pResultsWnd->AddWnd( (CWnd*)pResultsMapWnd, RCW_MAP, &ResultsMapWnd::Replay, &ResultsMapWnd::SetYear );

            //if ( m_pResultsWnd->m_syncMapZoom )
            //   {
            //   m_pResultsWnd->SyncMapZoom( pSourceMap, 
            //   }
            if ( m_pResultsWnd->m_syncMapZoom )
               {
               // sync zoom with main window if syncing turned on
               REAL vxMin, vxMax, vyMin, vyMax;
               gpCellLayer->GetMapPtr()->GetViewExtents( vxMin, vxMax, vyMin, vyMax );
                
               pResultsMapWnd->m_pMap->SetViewExtents( vxMin, vxMax, vyMin, vyMax );  // doesn't raise NT_EXTENTSCHANGED event
               pResultsMapWnd->m_mapFrame.m_pMapWnd->RedrawWindow();
               }
            }
         catch( CException *e )
            {
            TCHAR _msg[ 256 ];
            e->GetErrorMessage( _msg, 256  );
            CString msg("Error creating ResultsMapWnd: " );
            msg += _msg;
      		Report::ErrorMsg( msg );
            }
         catch( ... )
            {
            ASSERT( 0 );
            }
         }
         break;

      //case VT_MAP_MULTI_SCENARIO_VULNERABILITY:
      //   {
      //   UINT_PTR firstUnapplied = gpModel->UnApplyDeltaArray( gpCellLayer );  // reset base map to starting conditions
      //
      //   MapWnd *pMapWnd = new MapWnd;
      //   ASSERT( pMapWnd != NULL );
      //
      //   CString title;
      //   title.Format( "Scenario Vulnerability - MultiRun %i (%s)", run, (LPCTSTR) ri.pScenario->m_name );
      //
      //   int result = pMapWnd->Create( NULL, title,
      //         WS_CHILD | WS_BORDER | WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
      //         rect, m_pResultsWnd, 4000+m_pTree->GetCount() );
      //
      //   m_pResultsWnd->AddWnd( (CWnd*)pMapWnd, RCW_MAP, NULL, NULL );
      //
      //   MapLayer *pLayer = pMapWnd->m_pMap->AddLayer( LT_POLYGON );      // add an empty layer
      //   pLayer->SetNoOutline();
      //
      //   // make a copy of the polygons
      //   pLayer->ClonePolygons( gpCellLayer );      // warning: sets no data value to 0!
      //
      //   // get the needed data
      //   int rows = pLayer->GetPolygonCount( MapLayer::ALL );
      //   FDataObj *pData = gpModel->m_pDataManager->CalculateScenarioVulnerability( run );
      //
      //   pLayer->m_pDbTable = new DbTable( pData );   // for creating a DbTable from a preexisting VDataObj
      //   pLayer->m_pData    = pData;
      //   pLayer->SetNoDataValue( gpCellLayer->GetNoDataValue() );
      //   MapWnd::SetupBins( pMapWnd->m_pMap, 0, 0 );  // ClassifyData() is called in here
      //   pMapWnd->RefreshList();
      //
      //   if ( m_pResultsWnd->m_syncMapZoom )
      //      {
      //      // sync zoom with main window if syncing turned on
      //      float vxMin, vxMax, vyMin, vyMax;
      //      gpCellLayer->GetMapPtr()->GetViewExtents( vxMin, vxMax, vyMin, vyMax );
      //       
      //      pMapWnd->m_pMap->SetViewExtents( vxMin, vxMax, vyMin, vyMax );  // doesn't raise NT_EXTENTSCHANGED event
      //      }
      //   else
      //      {
      //      pMapWnd->m_pMap->UpdateMapExtents();
      //      pMapWnd->m_pMap->ZoomFull();
      //      }
      //
      //   gpModel->ApplyDeltaArray( gpCellLayer, NULL, 0, firstUnapplied == 0 ? 0 : firstUnapplied-1);      // return map to it's starting state
      //   pMapWnd->m_pMap->RedrawWindow();
      //   }
      //   break;

      case VT_MAP_VISUALIZER:
		  //needs to be retested - pcw
         AddVisualizerWnd( pNode );   // extra has offset of visualizer in EnvModel visualizer array
         break;
      }
   }


void ResultsPanel::AddChangeMapWnd( ResultNode *pNode )  //RESULT_TYPE type, int run, int fieldInfoOffset )
   {
   CWaitCursor c;
   RECT rect;
   m_pResultsWnd->GetClientRect( &rect );

   RESULT_TYPE type = pNode->m_type;
   int run   = pNode->m_run;
   int fieldInfoOffset = (int) pNode->m_extra;

   RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );

   switch( type )
      {
      case VT_MAP_DYNAMIC:         
         {
         ASSERT( fieldInfoOffset >=0 );

         // make sure all current ResultsMaps are rolled back to year=0 (this
         // is required because DeltaArray's must be sync'ed up)
         m_pResultsWnd->ReplayReset();

         MAP_FIELD_INFO *pInfo = gpCellLayer->GetFieldInfo( fieldInfoOffset );

         CString title = "Change Map";
         if ( pInfo )
            title = pInfo->label;

         title.AppendFormat( " - Run %i (%s)", run, (LPCTSTR) ri.pScenario->m_name );

         ResultsMapWnd *pMapWnd = new ResultsMapWnd( run, fieldInfoOffset, true );
         ASSERT( pMapWnd != NULL );

         int nID = 4000+m_pTree->GetCount();

         try
            {
            pMapWnd->Create( NULL, title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, nID );

            Map *pMap = pMapWnd->m_pMap;
            MapLayer *pLayer = pMap->GetLayer( 0 );

            pLayer->AddFieldInfo( pInfo );  // makes a copy of the MAP_FIELD_INFO

            Map::SetupBins( pMap, 0, 0, true );  // ClassifyData() is called in here
            pMapWnd->RefreshList(); 

            m_pResultsWnd->AddWnd( (CWnd*)pMapWnd, RCW_MAP, &ResultsMapWnd::Replay, &ResultsMapWnd::SetYear );
            }
         catch( CException *e )
            {
            TCHAR _msg[ 256 ];
            e->GetErrorMessage( _msg, 256  );
            CString msg("Error creating ResultsMapWnd: " );
            msg += _msg;
      		Report::ErrorMsg( msg );
            }
         catch( ... )
            {
            ASSERT( 0 );
            }
         }
         break;

      case VT_MAP_MULTI_SCENARIO_VULNERABILITY:
         {
         //UINT_PTR firstUnapplied = gpModel->UnApplyDeltaArray( gpCellLayer );  // reset base map to starting conditions
         //
         //MapWnd *pMapWnd = new MapWnd;
         //ASSERT( pMapWnd != NULL );
         //
         //CString title;
         //title.Format( "Scenario Vulnerability - MultiRun %i (%s)", run, (LPCTSTR) ri.pScenario->m_name );
         //
         //int result = pMapWnd->Create( NULL, title,
         //      WS_CHILD | WS_BORDER | WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
         //      rect, m_pResultsWnd, 4000+m_pTree->GetCount() );
         //
         //m_pResultsWnd->AddWnd( (CWnd*)pMapWnd, RCW_MAP, NULL, NULL );
         //
         //MapLayer *pLayer = pMapWnd->m_pMap->AddLayer( LT_POLYGON );      // add an empty layer
         //pLayer->SetNoOutline();
         //
         //// make a copy of the polygons
         //pLayer->ClonePolygons( gpCellLayer );      // warning: sets no data value to 0!
         //
         //// get the needed data
         //int rows = pLayer->GetPolygonCount( MapLayer::ALL );
         //FDataObj *pData = gpModel->m_pDataManager->CalculateScenarioVulnerability( run );
         //
         //pLayer->m_pDbTable = new DbTable( pData );   // for creating a DbTable from a preexisting VDataObj
         //pLayer->m_pData    = pData;
         //pLayer->SetNoDataValue( gpCellLayer->GetNoDataValue() );
         //MapWnd::SetupBins( pMapWnd->m_pMap, 0, 0 );  // ClassifyData() is called in here
         //pMapWnd->RefreshList();
         //pMapWnd->m_pMap->UpdateMapExtents();
         //pMapWnd->m_pMap->ZoomFull();
         //
         //gpModel->ApplyDeltaArray( gpCellLayer, NULL, 0, firstUnapplied == 0 ? 0 : firstUnapplied-1);      // return map to it's starting state
         //pMapWnd->m_pMap->RedrawWindow();
         }
         break;
      }
   }

   
void ResultsPanel::AddGraphWnd( ResultNode *pNode )
   {
   CWaitCursor c;
   CWnd *pGraph = NULL;
   CString title;
   bool isMultiRun = false;
   bool useAutoX = true;
   bool useAutoY = true;
   float yMin = -3.0f;
   float yMax = 3.0f;
   REPLAYFN replayFn = NULL;
   SETYEARFN setYearFn = NULL;

   RESULT_TYPE type = pNode->m_type;
   int run   = pNode->m_run;
   RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );

   INT_PTR extra = pNode->m_extra;

   int nID = 5000;

   switch( type )
      {
      case VT_GRAPH_DYNAMIC:         
         {
         ASSERT( extra >=0 );
         
         EnvEvaluator *pModel = gpModel->GetResultsEvaluatorInfo( (int) extra );
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_RAWSCORES, run );
         ASSERT( pDataObj != NULL );
         
         ScatterWnd *pScatter = new ScatterWnd;
         pGraph = (CWnd*) pScatter;
         
         pScatter->SetXCol( pDataObj, 0 );
         pScatter->AppendYCol( pDataObj, (int) extra+1 );
         pScatter->MakeDataLocal();
         yMin = yMax = 0;
         
         title.Format( "%s - Run %i (%s)", (LPCTSTR) pModel->m_name, run, (LPCTSTR) ri.pScenario->m_name );
         }
         break;

      case VT_GRAPH_APPVAR:
         {
         ASSERT( extra != 0 );
         DataManager *pDataManager = gpModel->m_pDataManager;
         DataObj *pDataObj = pDataManager->GetDataObj( DT_MODEL_OUTPUTS, run );
         ASSERT( pDataObj != NULL );
         AppVar *pVar = (AppVar*) extra;

         int col = pDataObj->GetCol( pVar->m_name );
         ASSERT( col >= 0 );

         TYPE type = pDataObj->GetType( col );
         ScatterWnd *pScatter = NULL;
         
         if ( type == TYPE_PDATAOBJ )      // Note: Assumes plugin manages the dataobjs
            {
            DynamicScatterWnd *pDScatter = new DynamicScatterWnd;
            pScatter = (ScatterWnd*) pDScatter;

            // add data objects (rows=year, col for each exposed variable)
            for ( int i=0; i < pDataObj->GetRowCount(); i++ )
               {
               DataObj *pDynData;
               bool ok = pDataObj->Get( col, i, pDynData );

               ASSERT( ok );
               //ASSERT( pDynData != NULL );  NULL is sometimes okay
               pDScatter->AddDataObj( (FDataObj*) pDynData );
               }

            pDScatter->CreateDataObjectFromComposites( DSP_USELASTONLY, false );
            }
         else
            {
            pScatter = new ScatterWnd;
            pScatter->SetXCol( pDataObj, 0 );
            pScatter->AppendYCol( pDataObj, col );
            }

         pGraph = (CWnd*) pScatter;
         pScatter->MakeDataLocal();
         yMin = yMax = 0;

         title.Format( "%s - Run %i (%s)", (LPCTSTR) pVar->m_name, run, (LPCTSTR) ri.pScenario->m_name );
         }
         break;

      case VT_GRAPH_MODEL_OUTPUT:         
         {
         DataManager *pDataManager = gpModel->m_pDataManager;
         DataObj *pDataObj = pDataManager->GetDataObj( DT_MODEL_OUTPUTS, run );
         ASSERT( pDataObj != NULL );
         int col = (int) extra;
         TYPE type = pDataObj->GetType( col );
         ScatterWnd *pScatter = NULL;
         if ( type == TYPE_PDATAOBJ )      // Note: Assumes plugin manages the dataobjs
            {
            DynamicScatterWnd *pDScatter = new DynamicScatterWnd;
            pScatter = (ScatterWnd*) pDScatter;

            // add data objects (rows=year, col for each exposed variable)
            for ( int i=0; i < pDataObj->GetRowCount(); i++ )
               {
               DataObj *pDynData;
               bool ok = pDataObj->Get( col, i, pDynData );

               ASSERT( ok );
               //ASSERT( pDynData != NULL );  NULL is sometimes okay
               ASSERT( pDynData == NULL  || pDynData->GetDOType() == DOT_FLOAT );
               pDScatter->AddDataObj( (FDataObj*) pDynData );
               }

            pDScatter->CreateDataObjectFromComposites( DSP_USELASTONLY, false );
            }
         else
            {
            pScatter = new ScatterWnd;
            pScatter->SetXCol( pDataObj, 0 );
            pScatter->AppendYCol( pDataObj, col );
            }

         pGraph = (CWnd*) pScatter;
         pScatter->MakeDataLocal();

         // reset legend labels to NOT include dataobj name
         for ( int i=0; i < pScatter->GetLineCount(); i++ )
            {
            LINE *pLine = pScatter->GetLineInfo( i );
            LPCTSTR label = pLine->text;
            TCHAR *p = _tcschr( (LPTSTR) label, '.');
            if ( p != NULL )
               pLine->text = p+1;
            }

         yMin = yMax = 0;

         title.Format( "%s - Run %i (%s)", (LPCTSTR) pDataObj->GetLabel( col ), run, (LPCTSTR) ri.pScenario->m_name );
         }
         break;

      case VT_GRAPH_EVAL_SCORES:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         }
         break;

      case VT_GRAPH_EVAL_RAWSCORES:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_RAWSCORES, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         ((ScatterWnd*)pGraph)->SetAutoscaleAll();
         }
         break;


      case VT_GRAPH_POLICY_APP:
         {
         //DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_POLICY_APPS, run );
         DataObj *pDataObj = gpModel->m_pDataManager->CalculatePolicyApps( run );

         if ( pDataObj != NULL )
            {
            pGraph = (CWnd*) new ScatterWnd( pDataObj, true );
            delete pDataObj;
            yMin = yMax = 0;
            }
         }
         break;

      case VT_GRAPH_POLICY_EFFECTIVENESS_TREND_DYNAMIC:
         {
         // NOTE:: ??? 0, 1 goals are hard coded for now!!!!
         DataObj *pDataObj = gpModel->m_pDataManager->CalculatePolicyEffectivenessTrendDynamic( 0, 1, run );
         if ( pDataObj != NULL )
            {
            pGraph = (CWnd*) new ScatterWnd( pDataObj, true );
            delete pDataObj;
            yMin = yMax = 0;
            }
         }
         break;

      case VT_GRAPH_POLICY_EFFECTIVENESS_TREND_STATIC:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->CalculatePolicyEffectivenessTrendStatic( run );
         if ( pDataObj != NULL )
            {
            pGraph = (CWnd*) new ScatterWnd( pDataObj, true );
            delete pDataObj;
            }
         }
         break;

      case VT_GRAPH_GLOBAL_CONSTRAINTS:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_GLOBAL_CONSTRAINTS, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         }
         break;

      case VT_GRAPH_POLICY_STATS:
         {
         ASSERT( extra > 0 );

         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_POLICY_STATS, run );
         ASSERT( pDataObj != NULL );

         ScatterWnd *pScatter = new ScatterWnd;
         pGraph = (CWnd*) pScatter;

         pScatter->SetXCol( pDataObj, 0 );
         int policyIndex = (int) extra - 1;
         Policy *pPolicy = gpPolicyManager->GetPolicy( policyIndex );

         for( int i=0; i < 6; i++ )
            {
            int col = 1 + ( policyIndex*6 ) + i;
            pScatter->AppendYCol( pDataObj, col );
            }

         pScatter->MakeDataLocal();
         yMin = yMax = 0;

         title.Format( "%s - Run %i (%s)", (LPCTSTR) pPolicy->m_name, run, (LPCTSTR) ri.pScenario->m_name );
         }
         break;

      case VT_GRAPH_SOCIAL_NETWORK:
         {
         ASSERT( extra > 0 );

         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_SOCIAL_NETWORK, run );
         ASSERT( pDataObj != NULL );

         ScatterWnd *pScatter = new ScatterWnd;
         pGraph = (CWnd*) pScatter;

         pScatter->SetXCol( pDataObj, 0 );   // time

         int layer = (int) extra - 1;
         SNLayer *pSNLayer = gpModel->m_pSocialNetwork->GetLayer( layer );

         int metricCount = gpModel->m_pSocialNetwork->GetMetricCount();

         for( int i=0; i < metricCount; i++ )
            {
            int col = 1 + ( layer*metricCount ) + i;
            pScatter->AppendYCol( pDataObj, col );
            }

         pScatter->MakeDataLocal();
         yMin = yMax = 0;

         title.Format( "%s - Run %i (%s)", pSNLayer->m_name.c_str(), run, (LPCTSTR)ri.pScenario->m_name);
         }
         break;

      case VT_GRAPH_ACTOR_WTS:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_ACTOR_WTS, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         }
         break;

      case VT_GRAPH_ACTOR_COUNTS:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_ACTOR_COUNTS, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         }
         break;

      case VT_GRAPH_LULCA_TREND:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( 1, run );
         
         if ( pDataObj != NULL )
            {
            pGraph = (CWnd*) new ScatterWnd( pDataObj, true );
   
            float minVal, maxVal, _maxVal=0;
            for ( int i=0; i < pDataObj->GetColCount(); i++ )
               {
               pDataObj->GetMinMax( i, &minVal, &maxVal );
               if ( maxVal > _maxVal ) _maxVal = maxVal;
               }
   
            yMin = 0;
   
            float md = fmod( _maxVal, 10.0f );         
            yMax = _maxVal + 10.0f - md;
   
            delete pDataObj;
            }
         }
         break;

      case VT_GRAPH_LULCB_TREND:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( 2, run );
         if ( pDataObj != NULL )
            {
            pGraph = (CWnd*) new ScatterWnd( pDataObj, true );

            float minVal, maxVal, _maxVal=0;
            for ( int i=0; i < pDataObj->GetColCount(); i++ )
               {
               pDataObj->GetMinMax( i, &minVal, &maxVal );
               if ( maxVal > _maxVal ) _maxVal = maxVal;
               }

            yMin = 0;

            float md = fmod( _maxVal, 10.0f );         
            yMax = _maxVal + 10.0f - md;

            delete pDataObj;
            }
         }
         break;

/*
      case VT_GRAPH_LANDUSE_SUMMARY:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( run );
         ASSERT( pDataObj != NULL );
         pGraph =(CWnd*) new ScatterWnd( pDataObj, true );
         delete pDataObj;
         yMin = 0;
         yMax = 40;
         }
         break;
*/
      case VT_GRAPH_POP_BY_LULCA:
         {
         int col = gpCellLayer->GetFieldCol( "POPDENS" );  // this in in units of #/ha

         float convFactor = 0;
         switch( gpCellLayer->m_mapUnits )
            {
            case MU_METERS:
            case MU_UNKNOWN:
               convFactor = HA_PER_M2;    // 1/10000 hectare per m2
               break;

            case MU_FEET:
               convFactor = HA_PER_M2/ FT2_PER_M2;
               break;
            }             

         DataObj *pDataObj = gpModel->m_pDataManager->CalculateTrendsWeightedByLulcA( col, convFactor, AF_MULTIPLY, run, false );
         ASSERT( pDataObj != NULL );
         pGraph =(CWnd*) new BarPlotWnd( pDataObj, true );

         float popMin, popMax;
         pDataObj->GetMinMax( -1, &popMin, &popMax, 0 );
         delete pDataObj;
         yMin = 0;
         yMax = 40000; //popMax;
         }
         break;

      case VT_GRAPH_EVALSCORES_BY_LULCA:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->CalculateEvalScoresByLulcA( run );
         ASSERT( pDataObj != NULL );
         BarPlotWnd *pBarPlot = new BarPlotWnd( pDataObj, true );
         pBarPlot->SetBarPlotStyle( BPS_STACKED );
         pGraph = (CWnd*) pBarPlot;
         }
         break;

      //case VT_GRAPH_POLICY_PRIORITY:
      //   pScatter = (ScatterWnd*) new PolicyPriorityPlot;
      //   break;

      case VT_GRAPH_MULTI_DYNAMIC:
         {
/*         ASSERT( extra >=0 );
         EnvEvaluator *pModel = gpModel->GetResultsModelInfo( extra );

         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_RAWSCORES, run );
         ASSERT( pDataObj != NULL );


         int multiRun = run;
         FDataObj *pDataObj = (FDataObj*) gpModel->m_pDataManager->GetMultiRunDataObj( DT_MULTI_EVAL_SCORES, multiRun );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         isMultiRun = true;
         delete pDataObj;  */
         }
         break;
     
      case VT_GRAPH_MULTI_EVAL_SCORES:
         {
         int multiRun = run;
         FDataObj *pDataObj = (FDataObj*) gpModel->m_pDataManager->GetMultiRunDataObj( DT_MULTI_EVAL_SCORES, multiRun );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         isMultiRun = true;
         delete pDataObj;
         }
         break;

      case VT_GRAPH_MULTI_EVAL_SCORE_FREQ:
         {
         int multiRun = run;
         FDataObj *pDataObj = (FDataObj*) gpModel->m_pDataManager->CalculateEvalFreq( multiRun ); 
         ASSERT( pDataObj != NULL );
         pGraph =(CWnd*) new MultirunHistogramPlotWnd( multiRun );
         BarPlotWnd &plot = ((MultirunHistogramPlotWnd*) pGraph)->m_plot;
         plot.SetExtents( XAXIS, -3, 3 );
         plot.SetAxisTickIncr( XAXIS, 1, 0 );
         useAutoX = false;
         plot.SetExtents( YAXIS, 0, 200 );
         useAutoY = false;
         isMultiRun = true;
         yMin = yMax = 0;
         delete pDataObj;
         replayFn = MultirunHistogramPlotWnd::Replay;
         //setYearFn = MultirunHistogramPlowWnd::SetYear;
         }
         break;
         
      case VT_GRAPH_MULTI_PARAMETERS:
         {
         int multiRun = run;
         FDataObj *pDataObj = (FDataObj*) gpModel->m_pDataManager->GetMultiRunDataObj( DT_MULTI_PARAMETERS, multiRun );
         ASSERT( pDataObj != NULL );
         pGraph =(CWnd*) new ScatterWnd( pDataObj, false );
         isMultiRun = true;
         yMin = yMax = 0;
         }
         break;

      case VT_GRAPH_MULTI_PARAMETERS_FREQ:
         {
         int multiRun = run;
         FDataObj *pDataObj = (FDataObj*) gpModel->m_pDataManager->CalculateScenarioParametersFreq( multiRun ); 
         ASSERT( pDataObj != NULL );
         pGraph =(CWnd*) new ScatterWnd( pDataObj, true );
         isMultiRun = true;
         yMin = yMax = 0;
         delete pDataObj;
         }
         break;
      }

   if ( pGraph == NULL )
      return;

   RECT rect;
   m_pResultsWnd->GetClientRect( &rect );
   
   LPCTSTR _title = pNode->m_text;  //GetResultText( type );

   if ( _title != NULL )
      {
   if ( isMultiRun )
      {
      MULTIRUN_INFO &ri = gpModel->m_pDataManager->GetMultiRunInfo( run );
         title.Format( "%s - Multiple Run %i (%s)", _title, run, (LPCTSTR) ri.pScenario->m_name );
      }
   else
      title.Format( "%s - Run %i (%s)", _title, run, (LPCTSTR) ri.pScenario->m_name );
      }

   // create the frame window - this will hold the graph window
   ResultsGraphWnd *pResultsGraphWnd = new ResultsGraphWnd( run, false );
   ASSERT( pResultsGraphWnd != NULL );
   pResultsGraphWnd->Create( NULL, title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, nID );
   pResultsGraphWnd->m_pGraphWnd = (GraphWnd*) pGraph;

   // create the graph window
   pGraph->Create( NULL, title, WS_CHILD | WS_VISIBLE, rect, pResultsGraphWnd, 5000 );
   
   // handle special cases
   if ( type == VT_GRAPH_POLICY_EFFECTIVENESS_TREND_DYNAMIC )
      ((ScatterWnd*) pGraph)->SetDynamicDisplayInterval( 250 );

   if ( type == VT_GRAPH_MULTI_EVAL_SCORE_FREQ )
      ((MultirunHistogramPlotWnd*)pGraph)->m_plot.SetBarPlotStyle( BPS_SINGLE );
   
   // handle scaling
   if ( ! useAutoX )
      {
      if ( type == VT_GRAPH_MULTI_EVAL_SCORE_FREQ )
         {
         BarPlotWnd *pPlot = &(((MultirunHistogramPlotWnd*) pGraph)->m_plot );
         COLDATA xColData = pPlot->GetXColData();
         float xMin = xColData.pDataObj->GetAsFloat( xColData.colNum, 0 );
         float xMax = xColData.pDataObj->GetAsFloat( xColData.colNum, xColData.pDataObj->GetRowCount()-1 );
         pPlot->SetExtents( XAXIS, xMin, xMax, true, true );
         }
      else
         {
         GraphWnd *_pGraph = (GraphWnd*) pGraph;
         COLDATA xColData = _pGraph->GetXColData();
         float xMin = xColData.pDataObj->GetAsFloat( xColData.colNum, 0 );
         float xMax = xColData.pDataObj->GetAsFloat( xColData.colNum, xColData.pDataObj->GetRowCount()-1 );
         _pGraph->SetExtents( XAXIS, xMin, xMax, true, true );
         }
      }

   if ( ! useAutoY )
      ((GraphWnd*)pGraph)->SetExtents( YAXIS, yMin, yMax, true, true );

   m_pResultsWnd->AddWnd( (CWnd*) pResultsGraphWnd, RCW_GRAPH, &ResultsGraphWnd::Replay, &ResultsGraphWnd::SetYear  );
   }


void ResultsPanel::AddViewWnd( ResultNode *pNode ) //RESULT_TYPE type, int run, int /*extra*/ )
   {
   CWaitCursor c;
   RECT rect;
   m_pResultsWnd->GetClientRect( &rect );

   RESULT_TYPE type = pNode->m_type;
   int run   = pNode->m_run;
   //int extra = pNode->m_extra;

   bool isMultiRun = false;
   CWnd *pWnd = NULL;

   switch( type )
      {
      case VT_VIEW_GOALSPACE:
         {
         GoalSpaceWnd* pGoalSpaceWnd = new GoalSpaceWnd( run );
         pGoalSpaceWnd->Create( NULL, "", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, 4000+m_pTree->GetCount() );
         m_pResultsWnd->AddWnd( (CWnd*)pGoalSpaceWnd, RCW_VIEW, &GoalSpaceWnd::Replay, NULL );
         pWnd = (CWnd*) pGoalSpaceWnd;
         }
         break;

      case VT_VIEW_POLICY_APP:
         {
         PolicyAppPlot *pApp = new PolicyAppPlot( run );
         pApp->Create( rect, m_pResultsWnd );
         m_pResultsWnd->AddWnd( (CWnd*)pApp, RCW_VIEW, NULL, NULL );
         pWnd = (CWnd*) pApp;
         }
         break;
         
      case VT_VIEW_LULC_TRANS:
         {
         LulcTransitionResultsWnd *pTrans = new LulcTransitionResultsWnd( run ); 
         pTrans->Create( NULL, "", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, 4000+m_pTree->GetCount() );
         m_pResultsWnd->AddWnd( (CWnd*)pTrans, RCW_VIEW, &LulcTransitionResultsWnd::Replay, NULL );
         pWnd = (CWnd*) pTrans;
         }
         break;

      case VT_VIEW_DELTA_ARRAY:
         {
         DeltaViewer dlg( this, run );
         dlg.DoModal();
         }
         return;

      case VT_VIEW_MULTI_TRAJECTORYSPACE:
         {
         TrajectorySpaceWnd *pTraj = new TrajectorySpaceWnd( run );
         int result = pTraj->Create( NULL, "", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, 4000+m_pTree->GetCount() );
         m_pResultsWnd->AddWnd( (CWnd*)pTraj, RCW_VIEW, &TrajectorySpaceWnd::Replay, NULL );
         pWnd = (CWnd*) pTraj;
         isMultiRun = true;
         }
         break;
      case VT_VIEW_VISUALIZER:
         {
         AddVisualizerWnd(pNode);
         return;
         }
      }

   LPCTSTR title = pNode->m_text;   //GetResultText( type );
   CString _title;
   if ( isMultiRun )
      {
      MULTIRUN_INFO &ri = gpModel->m_pDataManager->GetMultiRunInfo( run );
      _title.Format( "%s - Multiple Run %i (%s)", title, run, (LPCTSTR) ri.pScenario->m_name );
      }
   else
      {
      RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );
      _title.Format( "%s - Run %i (%s)", title, run, (LPCTSTR) ri.pScenario->m_name );
      }

   ASSERT( pWnd != NULL );
   pWnd->SetWindowText( _title );
      
   //BOOL ok = pView->Create( NULL, title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, 4000+m_pTree->GetCount() );
   //if ( ok )
   m_pResultsWnd->AddWnd( pWnd, RCW_VIEW, NULL, NULL );
   }


void ResultsPanel::AddDataTableWnd( ResultNode *pNode ) //RESULT_TYPE type, int run, int /*extra*/ )
   {
   CWaitCursor c;
   DataObj *pDataObj = NULL;
   bool deleteDataObj = false;

   RESULT_TYPE type = pNode->m_type;
   int run   = pNode->m_run;
   //int extra = pNode->m_extra;

   switch( type )
      {
      case VT_DATA_EVAL_SCORES:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES, run );
         break;

      case VT_DATA_EVAL_RAWSCORES:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_RAWSCORES, run );
         break;

      case VT_DATA_ACTOR_WTS:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_ACTOR_WTS, run );
         break;

      case VT_DATA_ACTOR_COUNTS:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_ACTOR_COUNTS, run );
         break;
      //case VT_DATA_POLICY_PRIORITY:
      //   pDataObj = gpModel->m_pDataManager->GetDataObj( DT_POLICY_PRIORITIES, run );
      //   break;

      case VT_DATA_POLICY_APP:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyApps( run );
         deleteDataObj = true;
         break;

      case VT_DATA_POLICY_EFFECTIVENESS:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyEffectivenessTrendStatic( run );
         deleteDataObj = true;
         break;

      case VT_DATA_POLICY_SUMMARY:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_POLICY_SUMMARY, run ); //CalculatePolicySummary( run );
         //deleteDataObj = true;
         break;

      case VT_DATA_GLOBAL_CONSTRAINTS:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_GLOBAL_CONSTRAINTS, run );
         break;

      case VT_DATA_POLICY_STATS:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_POLICY_STATS, run );
         break;

      case VT_DATA_SOCIAL_NETWORK:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_SOCIAL_NETWORK, run );
         break;

      case VT_DATA_POLICIES_BY_ACTOR_AREA:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyByActorArea( run );
         deleteDataObj = true;
         break;

      case VT_DATA_POLICIES_BY_ACTOR_COUNT:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyByActorCount( run );
         deleteDataObj = true;
         break;

      case VT_DATA_LULC_TRANS:
         pDataObj = gpModel->m_pDataManager->CalculateLulcTransTable( run );
         deleteDataObj = true;
         break;

      case VT_DATA_LULCA_TREND:
         pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( 1, run );
         deleteDataObj = true;
         break;

      case VT_DATA_LULCB_TREND:
         pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( 2, run );
         deleteDataObj = true;
         break;

/*      case VT_DATA_LANDUSE_SUMMARY:
         pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( run );
         deleteDataObj = true;
         break;
*/
      case VT_DATA_POP_BY_LULCA:
         {
         int col = gpCellLayer->GetFieldCol( "POPDENS" );

         float convFactor = 0;
         switch( gpCellLayer->m_mapUnits )
            {
            case MU_METERS:
            case MU_UNKNOWN:
               convFactor = HA_PER_M2;    // 1/10000 hectare per m2
               break;

            case MU_FEET:
               convFactor = HA_PER_M2/ FT2_PER_M2;
               break;
            }             

         pDataObj = gpModel->m_pDataManager->CalculateTrendsWeightedByLulcA( col, convFactor, AF_MULTIPLY, run, true );
         deleteDataObj = true;
         }
         break;

/*
      case VT_DATA_MVALBYLULCA:
         {
         int col = gpCellLayer->GetFieldCol( "MARKET_VAL" );
         if ( col < 0 )
            return;
         pDataObj = gpModel->m_pDataManager->CalculateTrendsWeightedByLulcA( col, 1.0, AF_MULTIPLY, run );
         deleteDataObj = true;
         }
         break;
*/
      case VT_DATA_MULTI_EVAL_SCORES:
         pDataObj = gpModel->m_pDataManager->GetMultiRunDataObj( DT_MULTI_EVAL_SCORES, run );
         break;

      case VT_DATA_MULTI_EVAL_SCORE_FREQ:
         pDataObj = gpModel->m_pDataManager->CalculateEvalFreq( run );
         break;

      case VT_DATA_MULTI_PARAMETERS:
         pDataObj = gpModel->m_pDataManager->GetMultiRunDataObj( DT_MULTI_PARAMETERS, run );
         break;

      case VT_DATA_MULTI_PARAMETERS_FREQ:
         pDataObj = gpModel->m_pDataManager->CalculateScenarioParametersFreq( run );
         break;

      default:
         ASSERT(0);
         return;
      }
      
   if ( pDataObj != NULL )
      {
      RECT rect;
      m_pResultsWnd->GetClientRect( &rect );
   
      RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );
      
      LPCTSTR title = pNode->m_text;   //GetResultText( type );
      CString _title;
      _title.Format( "%s - Run %i (%s)", title, run, (LPCTSTR) ri.pScenario->m_name );
   
      ASSERT( pDataObj );
      DataTable *pDataTable = new DataTable( pDataObj ); // note: datatables implicitly manage data locally
      pDataTable->Create( _title, rect, m_pResultsWnd, 4000+m_pTree->GetCount() );
   
      m_pResultsWnd->AddWnd( pDataTable, RCW_TABLE, NULL, NULL );
   
      if ( deleteDataObj )
         delete pDataObj;
      }
   }


void ResultsPanel::AddCrossRunGraph( ResultNode *pNode ) //RESULT_TYPE type, int extra )
   {
   CWaitCursor c;
   CWnd *pGraph = NULL;
   CString title;
   bool isMultiRun = false;
   bool useAutoX = true;
   bool useAutoY = true;
   float yMin = -3.0f;
   float yMax = 3.0f;
   REPLAYFN replayFn = NULL;
   SETYEARFN setYearFn = NULL;

   RESULT_TYPE type = pNode->m_type;
   int run   = pNode->m_run;
   INT_PTR extra = pNode->m_extra;
   int nID = 5000;
   switch( type )
      {
      case VT_GRAPH_DYNAMIC:         
         {
         ASSERT( extra >=0 );

         EnvEvaluator *pModel = gpModel->GetResultsEvaluatorInfo( (int) extra );
         DataManager *pDM = gpModel->m_pDataManager;
         int runCount = pDM->GetRunCount();

         // get the first data object
         FDataObj *pRunData = (FDataObj*) gpModel->m_pDataManager->GetDataObj( DT_EVAL_RAWSCORES, 0 );
         int rows = pRunData->GetRowCount();
         int col  = pRunData->GetCol( pModel->m_name );

         FDataObj *pDataObj = new FDataObj( runCount+1, rows, U_YEARS);
         pDataObj->SetLabel( 0, "Time (years)" );

         RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( 0 );
         
         CString label;
         label.Format( "Run 0 (%s)", (LPCTSTR) ri.pScenario->m_name );
         pDataObj->SetLabel( 1, label );
         
         for ( int i=0; i < rows; i++ )
            {
            pDataObj->Set( 0, i, pRunData->GetAsFloat( 0,   i ) );
            pDataObj->Set( 1, i, pRunData->GetAsFloat( col, i ) );
            }

         for ( int run=1; run < runCount; run++ )
            {
            pRunData = (FDataObj*) gpModel->m_pDataManager->GetDataObj( DT_EVAL_RAWSCORES, run );

            RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );
            label.Format( "Run %i (%s)", run, (LPCTSTR) ri.pScenario->m_name );
            pDataObj->SetLabel( run+1, label );

            for ( int i=0; i < rows; i++ )
               pDataObj->Set( run+1, i, pRunData->GetAsFloat( col, i ) );
            }

         ScatterWnd *pScatter = new ScatterWnd;
         pGraph = (CWnd*) pScatter;
         pScatter->SetDataObj( pDataObj, true );
         yMin = yMax = 0;

         delete pDataObj;

         title.Format( "%s - Cross-Run Comparison", (LPCTSTR) pModel->m_name );
         }
         break;

      case VT_GRAPH_MODEL_OUTPUT:
         {
         // note:  'extra' contains the column in the DataManager DataObj that contains the model output values
         //   autonous process variables first, then evaluative model variables
         //
         // note:  for model variables, the data is stored in the DataManager DT_MODEL_OUTPUTS dataobj
         ASSERT( extra >=0 );
         DataManager *pDataManager = gpModel->m_pDataManager;
         int runCount = pDataManager->GetRunCount();

         // get the first data object
         DataObj *pRunData = pDataManager->GetDataObj( DT_MODEL_OUTPUTS, 0 );
         int rows = pRunData->GetRowCount();    // number of years in run
         int col  = (int) extra;

         FDataObj *pDataObj = new FDataObj( runCount+1, rows, U_YEARS);
         pDataObj->SetLabel( 0, "Time (years)" );

         RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( 0 );
         
         CString label;
         label.Format( "Run 0 (%s)", (LPCTSTR) ri.pScenario->m_name );
         pDataObj->SetLabel( 1, label );
         
         for ( int i=0; i < rows; i++ )
            {
            pDataObj->Set( 0, i, pRunData->GetAsFloat( 0,   i ) );
            pDataObj->Set( 1, i, pRunData->GetAsFloat( col, i ) );
            }

         for ( int run=1; run < runCount; run++ )
            {
            pRunData = pDataManager->GetDataObj( DT_MODEL_OUTPUTS, run );

            RUN_INFO &ri = pDataManager->GetRunInfo( run );
            label.Format( "Run %i (%s)", run, (LPCTSTR) ri.pScenario->m_name );
            pDataObj->SetLabel( run+1, label );

            for ( int i=0; i < rows; i++ )
               pDataObj->Set( run+1, i, pRunData->GetAsFloat( col, i ) );
            }

         ScatterWnd *pScatter = new ScatterWnd;
         pGraph = (CWnd*) pScatter;
         pScatter->SetDataObj( pDataObj, true );
         yMin = yMax = 0;

         delete pDataObj;

         title.Format( "%s - Cross-Run Comparison", pRunData->GetLabel( col ) );
         }
         break;

/*
      case VT_GRAPH_EVAL_SCORES:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         }
         break;

      case VT_GRAPH_EVAL_RAWSCORES:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_RAWSCORES, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         ((ScatterWnd*)pGraph)->SetAutoscaleAll();
         }
         break;


      case VT_GRAPH_POLICY_APP:
         {
         //DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_POLICY_APPS, run );
         DataObj *pDataObj = gpModel->m_pDataManager->CalculatePolicyApps( run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, true );
         delete pDataObj;
         yMin = yMax = 0;
         }
         break;

      case VT_GRAPH_POLICY_EFFECTIVENESS_TREND_DYNAMIC:
         {
         // NOTE:: ??? 0, 1 goals are hard coded for now!!!!
         DataObj *pDataObj = gpModel->m_pDataManager->CalculatePolicyEffectivenessTrendDynamic( 0, 1, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, true );
         delete pDataObj;
         yMin = yMax = 0;
         }
         break;

      case VT_GRAPH_POLICY_EFFECTIVENESS_TREND_STATIC:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->CalculatePolicyEffectivenessTrendStatic( run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, true );
         delete pDataObj;
         }
         break;

      case VT_GRAPH_ACTOR_WT_TRAJ:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->GetDataObj( DT_ACTOR_WTS, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, false );
         }
         break;

      case VT_GRAPH_LULCA_TREND:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( 1, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, true );

         float minVal, maxVal, _maxVal=0;
         for ( int i=0; i < pDataObj->GetColCount(); i++ )
            {
            pDataObj->GetMinMax( i, &minVal, &maxVal );
            if ( maxVal > _maxVal ) _maxVal = maxVal;
            }

         yMin = 0;

         float md = fmod( _maxVal, 10.0f );         
         yMax = _maxVal + 10.0f - md;

         delete pDataObj;
         }
         break;

      case VT_GRAPH_LULCB_TREND:
         {
         DataObj *pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( 2, run );
         ASSERT( pDataObj != NULL );
         pGraph = (CWnd*) new ScatterWnd( pDataObj, true );

         float minVal, maxVal, _maxVal=0;
         for ( int i=0; i < pDataObj->GetColCount(); i++ )
            {
            pDataObj->GetMinMax( i, &minVal, &maxVal );
            if ( maxVal > _maxVal ) _maxVal = maxVal;
            }

         yMin = 0;

         float md = fmod( _maxVal, 10.0f );         
         yMax = _maxVal + 10.0f - md;

         delete pDataObj;
         }
         break;

      case VT_GRAPH_POPBYLULCA:
         {
         int col = gpCellLayer->GetFieldCol( "POPDENS" );  // this in in units of #/ha

         float convFactor = 0;
         switch( gpCellLayer->m_mapUnits )
            {
            case MU_METERS:
            case MU_UNKNOWN:
               convFactor = HA_PER_M2;    // 1/10000 hectare per m2
               break;

            case MU_FEET:
               convFactor = HA_PER_M2/ FT2_PER_M2;
               break;
            }             

         DataObj *pDataObj = gpModel->m_pDataManager->CalculateTrendsWeightedByLulcA( col, convFactor, AF_MULTIPLY, run, false );
         ASSERT( pDataObj != NULL );
         pGraph =(CWnd*) new BarPlotWnd( pDataObj, true );

         float popMin, popMax;
         pDataObj->GetMinMax( -1, &popMin, &popMax, 0 );
         delete pDataObj;
         yMin = 0;
         yMax = 40000; //popMax;
         }
         break;

      //case VT_GRAPH_POLICY_PRIORITY:
      //   pScatter = (ScatterWnd*) new PolicyPriorityPlot;
      //   break;
*/     
      }

   if ( pGraph == NULL )
      return;

   RECT rect;
   m_pResultsWnd->GetClientRect( &rect );
   
   LPCTSTR _title = pNode->m_text;   //GetResultText( type );

   // create the frame window - this will hold the graph window 
   ResultsGraphWnd *pResultsGraphWnd = new ResultsGraphWnd( -1, false );
   ASSERT( pResultsGraphWnd != NULL );
   pResultsGraphWnd->Create( NULL, title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, nID );
   pResultsGraphWnd->m_pGraphWnd = (GraphWnd*) pGraph;
   
   pGraph->Create( NULL, title, WS_CHILD | WS_VISIBLE, rect, pResultsGraphWnd, 4000+m_pTree->GetCount() );

   // handle scaling
   if ( ! useAutoX )
      {
      if ( type == VT_GRAPH_MULTI_EVAL_SCORE_FREQ )
         {
         BarPlotWnd *pPlot = &(((MultirunHistogramPlotWnd*) pGraph)->m_plot );
         COLDATA xColData = pPlot->GetXColData();
         float xMin = xColData.pDataObj->GetAsFloat( xColData.colNum, 0 );
         float xMax = xColData.pDataObj->GetAsFloat( xColData.colNum, xColData.pDataObj->GetRowCount()-1 );
         pPlot->SetExtents( XAXIS, xMin, xMax, true, true );
         }
      else
         {
         GraphWnd *_pGraph = (GraphWnd*) pGraph;
         COLDATA xColData = _pGraph->GetXColData();
         float xMin = xColData.pDataObj->GetAsFloat( xColData.colNum, 0 );
         float xMax = xColData.pDataObj->GetAsFloat( xColData.colNum, xColData.pDataObj->GetRowCount()-1 );
         _pGraph->SetExtents( XAXIS, xMin, xMax, true, true );
         }
      }

   if ( ! useAutoY )
      ((GraphWnd*)pGraph)->SetExtents( YAXIS, yMin, yMax, true, true );

   m_pResultsWnd->AddWnd( pResultsGraphWnd, RCW_GRAPH, &ResultsGraphWnd::Replay, &ResultsGraphWnd::SetYear  );
   }


void ResultsPanel::ExportDataTable( ResultNode *pNode ) //RESULT_TYPE type, int run )
   {
   DataObj *pDataObj = NULL;
   bool deleteDataObj = false;
   CString filename;

   RESULT_TYPE type = pNode->m_type;
   int run = pNode->m_run;

   switch( type )
      {
      case VT_GRAPH_EVAL_SCORES:
      case VT_DATA_EVAL_SCORES:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_SCORES, run );
         filename = "EvalScores.csv";
         break;

      case VT_GRAPH_EVAL_RAWSCORES:
      case VT_DATA_EVAL_RAWSCORES:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_EVAL_RAWSCORES, run );
         filename = "EvalRawScores.csv";
         break;

      case VT_GRAPH_ACTOR_WTS:
      case VT_DATA_ACTOR_WTS:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_ACTOR_WTS, run );
         filename = "ActorValues.csv";
         break;

      case VT_GRAPH_ACTOR_COUNTS:
      case VT_DATA_ACTOR_COUNTS:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_ACTOR_COUNTS, run );
         filename = "ActorCounts.csv";
         break;

      case VT_GRAPH_POLICY_APP:
      case VT_DATA_POLICY_APP:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyApps( run );
         filename = "PolicyApps.csv";
         deleteDataObj = true;
         break;

/*
      case VT_GRAPH_POLICY_EFFECTIVENESS_TREND_DYNAMIC:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyEffectivenessTrendDynamic( run );
         filename = "PolicyEffectTrendsDynamic.csv";
         deleteDataObj = true;
         break;
*/
      case VT_GRAPH_POLICY_EFFECTIVENESS_TREND_STATIC:
      case VT_DATA_POLICY_EFFECTIVENESS:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyEffectivenessTrendStatic( run );
         filename = "PolicyEffectTrends.csv";
         deleteDataObj = true;
         break;

      case VT_DATA_POLICY_SUMMARY:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_POLICY_SUMMARY, run ); //CalculatePolicySummary( run );
         filename = "PolicySummary.csv";
         //deleteDataObj = true;
         break;

      case VT_DATA_POLICIES_BY_ACTOR_AREA:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyByActorArea( run );
         filename = "PolicyByActorAreaSummary.csv";
         deleteDataObj = true;
         break;
         
      case VT_DATA_POLICIES_BY_ACTOR_COUNT:
         pDataObj = gpModel->m_pDataManager->CalculatePolicyByActorCount( run );
         filename = "PolicyByActorCountSummary.csv";
         deleteDataObj = true;
         break;

      case VT_GRAPH_GLOBAL_CONSTRAINTS:
      case VT_DATA_GLOBAL_CONSTRAINTS:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_GLOBAL_CONSTRAINTS, run );
         filename = "GlobalConstraints.csv";
         break;

      case VT_GRAPH_POLICY_STATS:
      case VT_DATA_POLICY_STATS:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_POLICY_STATS, run );
         filename = "PolicyStats.csv";
         break;

      case VT_GRAPH_SOCIAL_NETWORK:
      case VT_DATA_SOCIAL_NETWORK:
         pDataObj = gpModel->m_pDataManager->GetDataObj( DT_SOCIAL_NETWORK, run );
         filename = "SocialNetwork.csv";
         break;

      case VT_DATA_LULC_TRANS:
         pDataObj = gpModel->m_pDataManager->CalculateLulcTransTable( run );
         filename = "LulcTransMatrix.csv";
         deleteDataObj = true;
         break;

      case VT_GRAPH_LULCA_TREND:
      case VT_DATA_LULCA_TREND:
         pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( 1, run );
         filename = "LulcATrends.csv";
         deleteDataObj = true;
         break;

      case VT_GRAPH_LULCB_TREND:
      case VT_DATA_LULCB_TREND:
         pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( 2, run );
         filename = "LulcBTrends.csv";
         deleteDataObj = true;
         break;

/*
      case VT_GRAPH_LANDUSE_SUMMARY:
      case VT_DATA_LANDUSE_SUMMARY:
         pDataObj = gpModel->m_pDataManager->CalculateLulcTrends( run );
         filename = "LandUseSummary.csv";
         deleteDataObj = true;
         break;
*/
      case VT_DATA_POP_BY_LULCA:
         {
         int col = gpCellLayer->GetFieldCol( "POPDENS" );
         pDataObj = gpModel->m_pDataManager->CalculateTrendsWeightedByLulcA( col, 1.0/10000.0, AF_MULTIPLY, run );
         filename = "PopByLulcA.csv";
         deleteDataObj = true;
         }
         break;
/*
case VT_DATA_MVALBYLULCA:
         {
         int col = gpCellLayer->GetFieldCol( "MARKET_VAL" );
         pDataObj = gpModel->m_pDataManager->CalculateTrendsWeightedByLulcA( col, 1.0, AF_MULTIPLY, run );
         filename = "MarketValByLulcA.csv";
         deleteDataObj = true;
         }
         break;
*/
      case VT_GRAPH_MULTI_EVAL_SCORES:
      case VT_DATA_MULTI_EVAL_SCORES:
         pDataObj = gpModel->m_pDataManager->GetMultiRunDataObj( DT_MULTI_EVAL_SCORES, run );
         filename = "MultiEvalScores.csv";
         break;

      case VT_GRAPH_MULTI_EVAL_SCORE_FREQ:
      case VT_DATA_MULTI_EVAL_SCORE_FREQ:
         pDataObj = gpModel->m_pDataManager->CalculateEvalFreq( run );
         filename = "MultiEvalScoresFreq.csv";
         break;

      case VT_GRAPH_MULTI_PARAMETERS:
      case VT_DATA_MULTI_PARAMETERS:
         pDataObj = gpModel->m_pDataManager->GetMultiRunDataObj( DT_MULTI_PARAMETERS, run );
         filename = "MultiParameters.csv";
         break;

      case VT_GRAPH_MULTI_PARAMETERS_FREQ:
      case VT_DATA_MULTI_PARAMETERS_FREQ:
         pDataObj = gpModel->m_pDataManager->CalculateScenarioParametersFreq( run );
         filename = "MultiParametersFreq.csv";
         break;

      case VT_GRAPH_MODEL_OUTPUTS:  // export all of a model/ap's outputs
      case VT_DATA_MODEL_OUTPUTS:
         {
         DataManager *pDataManager = gpModel->m_pDataManager;
         pDataObj = pDataManager->GetDataObj( DT_MODEL_OUTPUTS, run );
         ASSERT( pDataObj != NULL );
         // JUST DUMP EVERYTHING FOR NOW - this should be made model specific
         RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );

         filename.Format( "%s_ModelOutputs_Run%i.csv", (LPCTSTR) ri.pScenario->m_name, run );   
         //filename.Format( "Model Outputs - Run %i (%s).csv", run, (LPCTSTR) ri.pScenario->m_name );
         }
         break;

      default:
         Report::ErrorMsg( "Exporting this object is not currently supported..." );
         return;
      }
      
   if ( pDataObj == NULL)
      {
      CString msg( "Unable to export data object " );
      msg += filename;
  
      Report::Log( msg );
      return;
      }

   // ask where to save it
   char *cwd = _getcwd( NULL, 0 );

   CString path = PathManager::GetPath( PM_OUTPUT_DIR );
   path += filename;

   CFileDialog dlg( FALSE, ".csv", path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Envision Data Files|*.csv|All files|*.*||" );

   if ( dlg.DoModal() == IDOK )
      {
      filename = dlg.GetPathName();

      CWaitCursor c;
      pDataObj->WriteAscii( filename, ',' );
      }

   if ( deleteDataObj )
      {
      delete pDataObj;
      }

   _chdir( cwd );
   free( cwd );
   }
   
 

void ResultsPanel::ExportModelOutputs( int run /*=-1*/, bool promptForPath /*=false*/) 
   {
   CString path = PathManager::GetPath( PM_OUTPUT_DIR );

   if ( promptForPath )
      {
      DirPlaceholder d;

      CFolderDialog dlg( _T( "Select Folder" ), path, gpView, BIF_EDITBOX | BIF_NEWDIALOGSTYLE );
      dlg.SetSelectedFolder( path );

      if ( dlg.DoModal() == IDOK )
         {
         path = dlg.GetFolderPath();

         if ( !path.IsEmpty() )
            if ( path[ path.GetLength()-1 ] != '\\' )
               path += "\\";
         }
      }

   gpModel->m_pDataManager->ExportRunData( path, run );
   }


void ResultsPanel::AddVisualizerWnd( ResultNode *pNode )   // extra has offset of visualizer in EnvModel visualizer array
   {
   int index = (int) pNode->m_extra;
   ASSERT( index < gpModel->GetVisualizerCount() );

   EnvVisualizer *pEnvViz = gpModel->GetVisualizerInfo( index );
   ASSERT( pEnvViz != NULL );

   CWaitCursor c;
   RECT rect;
   m_pResultsWnd->GetClientRect( &rect );

   int run = pNode->m_run;
   RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );

   //RESULT_TYPE type = pNode->m_type;
   CString title( "Visualizer: " );
   title += pEnvViz->m_name;
   title.AppendFormat( " - Run %i (%s)", run, (LPCTSTR) ri.pScenario->m_name );

   //VisualizerWnd *pVizWnd = new VisualizerWnd( pEnvViz, run );
   
   //int nID = 4000+m_pTree->GetCount();

   try
      {
      BOOL success;
      VisualizerWnd *pVizWnd = gpView->m_vizManager.CreateWnd( m_pResultsWnd, pEnvViz, run, success,VZT_POSTRUN ); 

	  if (success)
	  {
		  pEnvViz->InitWindow(&(pVizWnd->m_envContext), pVizWnd->GetSafeHwnd());
	  }
      //pVizWnd->Create( NULL, title, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_VISIBLE, rect, m_pResultsWnd, nID );
      //gpView->m_vizManager.AddVisualizer( pVizWnd );
      m_pResultsWnd->AddWnd( (CWnd*) pVizWnd, RCW_VISUALIZER, &VisualizerWnd::RtReplay, &VisualizerWnd::RtSetYear );
      }

   catch( CException *e )
      {
      TCHAR _msg[ 256 ];
      e->GetErrorMessage( _msg, 256  );
      CString msg("Error creating ResultsMapWnd: " );
      msg += _msg;
      Report::ErrorMsg( msg );
      }
   catch( ... )
      {
      ASSERT( 0 );
      }
      
   }
