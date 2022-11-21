#pragma once
#include <afxwin.h>

#include <string>
#include <vector>
#include <map>

#include <COLORS.HPP>
#include <Vdata.h>

class NetForm;
class Node;

enum NE_TYPE { NE_NODE, NE_EDGE };



class NetElement
	{
	public:
		std::string id;
		std::string label;
		std::string displayLabel;
		int index = 0;
		RGBA color;

		std::vector<VData> attrs;

		bool selected = false;
		virtual NE_TYPE GetType() = 0;
	};


class Edge : public NetElement
	{
	public:
		virtual NE_TYPE GetType() { return NE_EDGE; }
		int thickness = 1;

		float trust = 0;
		float influence = 0;
		float signalStrength = 0;

		Node* pFromNode = nullptr;
		Node* pToNode = nullptr;

		static std::vector<std::string> attrIDs;
		static std::vector<std::string> attrLabels;
		static std::vector<std::string> attrTypes;
		static std::map<std::string, int> attrMap;
	};

class Node : public NetElement
	{
	public:
		virtual NE_TYPE GetType() { return NE_NODE; }

		std::string id;
		int nodeSize=1;	// logical coords

		std::vector<Edge*> inEdges;
		std::vector<Edge*> outEdges;


		int x=0;
		int y=0;

		float reactivity;
		float influence;

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
		 ~NetView()
			 {
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

		NODESIZE nodeSizeFlag   = NS_REACTIVITY;
		NODECOLOR nodeColorFlag = NC_INFLUENCE;
		NODELABEL nodeLabelFlag = NL_LABEL;
		EDGESIZE edgeSizeFlag  = ES_TRUST;
		EDGECOLOR edgeColorFlag = EC_SIGNALSTRENGTH;
		EDGELABEL edgeLabelFlag = EL_LABEL;

		bool LoadNetwork(LPCTSTR path);
		Node* FindNode(std::string& id)
			{
			for (Node* node : nodes)
				{ if (node->id == id) return node; }
			return nullptr;
			}

		void SetDrawMode(DRAW_MODE m);
		bool GetAnchorPoints(CPoint start, CPoint end, float alpha, float frac, CPoint& anchor0, CPoint& anchor1);
		void RotatePoint(CPoint& point, int x0, int y0, float theta);
		void ComputeEdgeArc(Edge* edge, float alpha);

		NetElement *HitTest(UINT, CPoint );
		void SetHoverText(LPCTSTR msg, CPoint pos);

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

