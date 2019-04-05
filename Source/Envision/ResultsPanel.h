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
#include "EnvWinLibs.h"
#include "resultsWnd.h"
#include "resultsTree.h"

#include "StaticSplitterWnd.h"


//---------------------------------------------------------------------------------------------------------
// NOTE: To add a new result viewer you must:
//   1) Create an appropriate RESULT_TYPE enum below
//   2) Add a corresponding ResultsPanel:_m_resultsInfo[] entry in ResultsPanel.cpp
//   3) Provide a DataManager method for generating an appropriate data object.
//   4) Add code in ResultsPanel.cpp in the appropriate AddMap(), AddGraph(), AddView(), AddDataTable(), method
//---------------------------------------------------------------------------------------------------------
enum RESULT_CATEGORY
   {
   VC_ROOT,
   VC_RUN,
   VC_MULTIRUN,
   VC_MAP,
   VC_GRAPH,
   VC_VIEW,  // includes visualizer
   VC_DATATABLE,
   VC_MULTIRUN_MAP,
   VC_MULTIRUN_GRAPH,
   VC_MULTIRUN_VIEW,
   VC_MULTIRUN_DATATABLE,
   VC_MODEL_OUTPUTS,   // Model Parent Nodes
   VC_VISUALIZER,      // visualizer nodes
   VC_POLICY_STATS,
   VC_SOCIAL_NETWORK,
   VC_LEVEL1_NODE,
   VC_OTHER
   };

enum RESULT_TYPE 
   {
   VT_ROOT,
   VT_RUN,
   VT_MULTIRUN,

   // maps
   VT_MAPS, 
   VT_MAP_DYNAMIC,      // for dynamically defined maps
   VT_MAP_VISUALIZER,

   // graphs
   VT_GRAPHS,
   VT_GRAPH_DYNAMIC,    // for dynamically defined graphs
   VT_GRAPH_APPVAR,
   VT_GRAPH_MODEL_OUTPUT,
   VT_GRAPH_MODEL_OUTPUTS,
   VT_GRAPH_EVAL_SCORES, 
   VT_GRAPH_EVAL_RAWSCORES, 
   VT_GRAPH_POLICY_APP, 
   VT_GRAPH_POLICY_EFFECTIVENESS_TREND_DYNAMIC,
   VT_GRAPH_POLICY_EFFECTIVENESS_TREND_STATIC,
   VT_GRAPH_GLOBAL_CONSTRAINTS,
   VT_GRAPH_POLICY_STATS,
   VT_GRAPH_ACTOR_WTS, 
   VT_GRAPH_ACTOR_COUNTS, 
   VT_GRAPH_LULCA_TREND,
   VT_GRAPH_LULCB_TREND,
   VT_GRAPH_POP_BY_LULCA,
   VT_GRAPH_EVALSCORES_BY_LULCA,
   VT_GRAPH_SOCIAL_NETWORK,
   VT_GRAPH_VISUALIZER,

   // views
   VT_VIEWS,
   VT_VIEW_GOALSPACE, 
   VT_VIEW_POLICY_APP,
   VT_VIEW_LULC_TRANS,
   VT_VIEW_DELTA_ARRAY,
   VT_VIEW_VISUALIZER,

   // data tables
   VT_DATATABLES,
   VT_DATA_MODEL_OUTPUTS,
   VT_DATA_EVAL_SCORES, 
   VT_DATA_EVAL_RAWSCORES, 
   VT_DATA_ACTOR_WTS, 
   VT_DATA_ACTOR_COUNTS, 
   VT_DATA_POLICY_APP, 
   VT_DATA_POLICY_EFFECTIVENESS,
   VT_DATA_POLICY_SUMMARY,
   VT_DATA_POLICIES_BY_ACTOR_AREA,
   VT_DATA_POLICIES_BY_ACTOR_COUNT,
   VT_DATA_GLOBAL_CONSTRAINTS,
   VT_DATA_POLICY_STATS,
   VT_DATA_LULC_TRANS,
   VT_DATA_LULCA_TREND,
   VT_DATA_LULCB_TREND,
   VT_DATA_POP_BY_LULCA,
   VT_DATA_EVALSCORES_BY_LULC_A,
   VT_DATA_SOCIAL_NETWORK,
   // multirun maps
   VT_MULTIMAPS,
   VT_MAP_MULTI_SCENARIO_VULNERABILITY,

   // multirun graphs
   VT_MULTIGRAPHS,
   VT_GRAPH_MULTI_DYNAMIC,    
   VT_GRAPH_MULTI_EVAL_SCORES, 
   VT_GRAPH_MULTI_EVAL_SCORE_FREQ,
   VT_GRAPH_MULTI_PARAMETERS,
   VT_GRAPH_MULTI_PARAMETERS_FREQ,
      
   // multirun views
   VT_MULTIVIEWS,
   VT_VIEW_MULTI_TRAJECTORYSPACE,

   // Multirun tables
   VT_MULTITABLES,
   VT_DATA_MULTI_EVAL_SCORES, 
   VT_DATA_MULTI_EVAL_RAWSCORES, 
   VT_DATA_MULTI_EVAL_SCORE_FREQ,
   VT_DATA_MULTI_PARAMETERS,
   VT_DATA_MULTI_PARAMETERS_FREQ,

   // some sort of visualizer
   VT_VISUALIZER,

   VT_UNKNOWN_RESULT
   };   


// ResultNode - a class for storing information about each node in the Postrun results tree
class ResultNode
{
public:
   int         m_run;          // what run is this node associated with
   INT_PTR     m_extra;        // extra data, used for dynamic map information (contains ptr to EnvEvaluator for result)
   bool        m_show;
   CString     m_text;

   RESULT_TYPE m_type;         // seen enum above
   RESULT_CATEGORY m_category;     // 0=run, 1=map, 2=graph, 3=view, 4=datatable, 6=multirun graph, 8=multirun table
   
   HTREEITEM   m_hItem;

   bool        m_isExpanded;

public:
   ResultNode( ResultNode *pParent ) : m_pParent( pParent ), m_category( VC_OTHER ), m_extra( 0 ), m_show( true ),
                     m_type( VT_UNKNOWN_RESULT ), m_text() { }

   ResultNode( ResultNode *pParent, RESULT_CATEGORY _category, int _run, INT_PTR _extra, bool _show, RESULT_TYPE _type, LPCTSTR _text ) 
                  :  m_pParent( pParent ), m_category( _category ), m_run( _run ), m_extra( _extra ), 
                     m_show( _show ), m_type( _type ), m_text( _text ) { }

   ResultNode( ResultNode &r );
   
   ~ResultNode();
   
   // node pointers
   ResultNode *m_pParent;

   CArray< ResultNode*, ResultNode* > m_children;
};


class ResultNodeTree
{
//  root
//    -- run1
//       -- maps
//          -- map1
//          -- ...
//       -- graphs
//          -- standard graph1
//          -- standard graph2
//          -- model1
//             -- var1
//                 -- childVar1
//                 -- childVar2
//             -- var 2
//         ... etc...
//       -- tables
//       -- views
//    -- run2 

public:
   ResultNode *m_pRootNode;  // base of the tree.  Run Nodes get added here

   ResultNodeTree();
   ~ResultNodeTree() { if ( m_pRootNode != NULL ) delete m_pRootNode; }

   ResultNode *AddNode( ResultNode *pParent, int _run, RESULT_CATEGORY _category, INT_PTR _extra, RESULT_TYPE _type, LPCTSTR _text );
};
   

// ResultsPanel - parent window that contains everything associated with the Post-Run Results view,
// including the results tree, the results tree container window, and the ResultWnd that holds
//  all the results

class ResultsPanel : public CWnd
{
	DECLARE_DYNAMIC(ResultsPanel)

public:
	ResultsPanel();
	virtual ~ResultsPanel();

   StaticSplitterWnd m_splitterWnd;
 
   ResultsWnd   *m_pResultsWnd;
   ResultsTree  *m_pTreeWnd;
   CTreeCtrl    *m_pTree;

public:
   CWnd *GetActiveView() { return m_pResultsWnd->m_pActiveWnd; }

   //static ResultInfoArray m_resultInfo;
   static ResultNodeTree m_resultTree;

   //static LPCTSTR GetResultText( RESULT_TYPE );
   //static int     GetResultCategory( RESULT_TYPE );

   bool AddRun( int run );
   bool AddMultiRun( int run );

   void AddMapWnd  ( ResultNode* ); //RESULT_TYPE type, int run, int fieldInfoOffset );
   void AddChangeMapWnd( ResultNode* ); //RESULT_TYPE type, int run, int fieldInfoOffset );
   void AddGraphWnd( ResultNode* ); //RESULT_TYPE type, int run, int extra );
   void AddViewWnd ( ResultNode* ); //RESULT_TYPE type, int run, int extra );
   void AddDataTableWnd( ResultNode* );   //RESULT_TYPE type, int run, int extra );
   void AddMultiRunResult( ResultNode* );   // RESULT_TYPE type, int run );

   void AddCrossRunGraph( ResultNode* ); //RESULT_TYPE type, int extra );

   void AddVisualizerWnd( ResultNode* );

   void ExportDataTable( ResultNode* ); //RESULT_TYPE type, int run );
   void ExportModelOutputs( int run=-1, bool promptForPath=true );  // -1 = all runs

   void RemoveAll( void ) { m_pResultsWnd->RemoveAll(); }

protected:
   ResultNode *AddTreeNode( ResultNode *pParent, int run, RESULT_CATEGORY category, INT_PTR extra, RESULT_TYPE resultType, LPCTSTR label );

   int  AddDynamicGraphNodes( ResultNode *pGraphNode );
	
 public:
    int PopulateTreeCtrl();    // populate nodes on tree control

 protected:  // helper functions for PopulateTreeCtrl()
   int PopulateTreeCtrl( ResultNode*, int level );  // returns number of children shown + whether this one is shown
   int SetNodeVisibility( ResultNode*, int level );

   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
   DECLARE_MESSAGE_MAP()

};


