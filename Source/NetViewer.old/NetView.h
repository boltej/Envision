#pragma once
#include <afxwin.h>

#include <string>
#include <vector>
#include <map>

#include <COLORS.HPP>
#include <Vdata.h>


class Node
	{
	public:
		std::string id;
		std::string label;

		int x = 0;
		int y = 0;
		RGBA color;

		float reactivity;
		float influence;

		static std::vector<std::string> attrLabels;
		static std::vector<std::string> attrTypes;
		static std::map<std::string, int> attrMap;

		std::vector<VData> attrs;

	};

class Edge
	{
	public:
		std::string id;
		std::string label;

		RGBA color;
		int thickness = 1;

		float trust = 0;
		float signalStrength = 0;

		Node* pFromNode = nullptr;
		Node* pToNode = nullptr;
	};




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

		std::vector<Node*> nodes;
		std::vector<Edge*> edges;

		// logical coords
		int xMin = 0;
		int xMax = 1000;
		int yMin = 0;
		int yMax = 1000;

		// current view, logical coords
		int cxMin = 0;
		int cxMax = 1000;
		int cyMin = 0;
		int cyMax = 1000;

		// 


		bool LoadNetwork(LPCTSTR path);
		Node* FindNode(std::string& id)
			{
			for (Node* node : nodes)
				{ if (node->id == id) return node; }
			return nullptr;
			}

	 protected:
		afx_msg void OnPaint();
		DECLARE_MESSAGE_MAP()

	 public:
		 afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		 afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		 afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		 afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		 afx_msg void OnViewZoomfull();
		 afx_msg void OnUpdateViewZoomfull(CCmdUI* pCmdUI);
	};

