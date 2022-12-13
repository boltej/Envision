#pragma once
#include <afxwin.h>

#include <string>
#include <vector>
#include <map>

#include <COLORS.HPP>
#include <Vdata.h>

#include <snip.h>

class NetForm;
class Node;

enum NE_TYPE { NE_NODE, NE_EDGE };

// node types
//enum NV_NODETYPE
//	{
//	NT_UNKNOWN = 0,
//	NT_INPUT_SIGNAL = 1,
//	NT_NETWORK_ACTOR = 2,
//	NT_ASSESSOR = 3,
//	NT_ENGAGER = 4,
//	NT_LANDSCAPE_ACTOR = 5
//	};

class NetElement
	{
	public:
		std::string displayLabel;
		int index = 0;
		bool show = true;
		RGBA color;

		std::vector<VData> attrs;

		bool selected = false;
		virtual NE_TYPE GetType() = 0;
	};


class Edge : public NetElement
	{
	public:
		SNEdge* pSNEdge = nullptr;

		Node* pFromNode = nullptr;
		Node* pToNode = nullptr;

		virtual NE_TYPE GetType() { return NE_EDGE; }
		int thickness = 1;

		static std::vector<std::string> attrIDs;
		static std::vector<std::string> attrLabels;
		static std::vector<std::string> attrTypes;
		static std::map<std::string, int> attrMap;
	};

class Node : public NetElement
	{
	public:
		SNNode* pSNNode = nullptr;

		virtual NE_TYPE GetType() { return NE_NODE; }

		int nodeSize = 1;	// logical coords
		int x = 0;
		int y = 0;

		std::vector<Edge*> inEdges;
		std::vector<Edge*> outEdges;

		static std::vector<std::string> attrIDs;
		static std::vector<std::string> attrLabels;
		static std::vector<std::string> attrTypes;
		static std::map<std::string, int> attrMap;

		int degree = 0;
		int inDegree = 0;
		int outDegree = 0;

		// temps
		int xDC = 0;
		int yDC = 0;
		int nodeSizeDC = 0;
	};


enum DRAW_FLAG { DF_NORMAL, DF_GRAYED, DF_NEIGHBORHOODS, DF_SHORTESTROUTE };
enum DRAW_MODE { DM_SELECT, DM_SELECTRECT, DM_PANNING, DM_ZOOMING};

enum NODESIZE  { NS_REACTIVITY, NS_INFLUENCE, NS_CONNECTIONS };
enum NODECOLOR { NC_REACTIVITY, NC_INFLUENCE, NC_CONNECTIONS };
enum NODELABEL { NL_LABEL, NL_INFLUENCE, NL_REACTIVITY };
enum EDGESIZE  { ES_TRUST, ES_SIGNALSTRENGTH, ES_INFLUENCE };
enum EDGECOLOR { EC_TRUST, EC_SIGNALSTRENGTH, EC_INFLUENCE };
enum EDGELABEL { EL_LABEL, EL_TRUST, EL_SIGNALSTRENGTH, EL_INFLUENCE };

class NetView :public CWnd
    {
	 public:
		 SNIPModel* m_pSNIPModel = nullptr;
		 SNLayer * m_pSNLayer = nullptr;

	 public:
		 ~NetView()
			 {
			 if (m_pSNIPModel != nullptr)
				 delete m_pSNIPModel;

			 for (Node* node : nodes)
			 	 delete node;
			 for (Edge* edge : edges)
			 	 delete edge;
			 }

		NetForm* netForm;
		std::string name;
		std::string desc;

		std::vector<Node*> nodes;
		std::vector<Edge*> edges;

		int xMin = 0;
		int xMax = 1000;
		int yMin = 0;
		int yMax = 1000;

		DRAW_MODE drawMode = DM_SELECT;
		bool showNodeLabels = true;
		bool showEdgeLabels = false;
		bool darkMode = false;
		int edgeType = 0;  // 1=beziers;
		int maxDegree = 0;

		int nodeCount = 0;
		int nodeCountIS = 0;
		int nodeCountASS = 0;
		int nodeCountNA = 0;
		int nodeCountENG = 0;
		int nodeCountLA = 0;

		NODESIZE nodeSizeFlag   = NS_REACTIVITY;
		NODECOLOR nodeColorFlag = NC_INFLUENCE;
		NODELABEL nodeLabelFlag = NL_LABEL;
		EDGESIZE edgeSizeFlag  = ES_TRUST;
		EDGECOLOR edgeColorFlag = EC_SIGNALSTRENGTH;
		EDGELABEL edgeLabelFlag = EL_LABEL;

		bool AttachSNIPModel(SNIPModel* pModel);

		bool LoadNetwork(LPCTSTR path); // deprecated
		Node* FindNode(std::string& id)
			{
			for (Node* node : nodes)
				{ if (node->pSNNode->m_id == id) return node; }
			return nullptr;
			}

		void GetNetworkStats(std::string &);

		void SetDrawMode(DRAW_MODE m);
		bool GetAnchorPoints(CPoint start, CPoint end, float alpha, float frac, CPoint& anchor0, CPoint& anchor1);
		void RotatePoint(CPoint& point, int x0, int y0, float theta);
		void ComputeEdgeArc(Edge* edge, float alpha);

		NetElement *HitTest(UINT, CPoint );
		void SetHoverText(LPCTSTR msg, CPoint pos);
		int SampleNodes(SNIP_NODETYPE nodeType, int count);

	 protected:
		 //void InitPaint(Graphics &graphics);
		 void InitDC(CDC &);
		 void DrawNetwork(CDC &, bool drawBkgr, DRAW_FLAG );
		 void DrawPanels(CDC& dc);
		 void DrawPoint(CDC& dc, CPoint pt, int radius, float aspect);

		 void ClearSelection();

		 afx_msg void OnPaint();
		DECLARE_MESSAGE_MAP()

	 public:
		 afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		 afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		 afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		 afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		 afx_msg void OnZoomfull();
		 //afx_msg void OnUpdateViewZoomfull(CCmdUI* pCmdUI);
		 afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		 afx_msg void OnMouseHover(UINT nFlags, CPoint point);
		 afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
		 afx_msg void OnDarkMode();
		 afx_msg void OnUpdateDarkmode(CCmdUI* pCmdUI);
		 afx_msg void OnShowEdgeLabels();
		 afx_msg void OnUpdateShowEdgeLabels(CCmdUI* pCmdUI);
		 afx_msg void OnShowNodeLabels();
		 afx_msg void OnUpdateShowNodeLabels(CCmdUI* pCmdUI);
		 afx_msg void OnNodeColor();
		 afx_msg void OnNodeSize();
		 afx_msg void OnNodeLabel();
		 afx_msg void OnEdgeColor();
		 afx_msg void OnEdgeSize();
		 afx_msg void OnEdgeLabel();
		 afx_msg void OnPan();
		 afx_msg void OnShowNeighborhood();
		 afx_msg void OnCurvedEdges();
	};

