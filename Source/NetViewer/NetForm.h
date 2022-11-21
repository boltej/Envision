#pragma once


class Node;
class Edge;

const int panelWidth = 300;


//static INT_PTR NetFormDlgProc(HWND, UINT, WPARAM, LPARAM);


// NetForm form view

class NetForm : public CWnd
{
	DECLARE_DYNCREATE(NetForm)

public:
	NetForm();           // protected constructor used by dynamic creation
	virtual ~NetForm();

public:
	CFont fontName;
	CFont fontDesc;
	CStatic m_netName;
	CStatic m_netDescription;
	CMFCPropertyGridCtrl m_nodePropGrid;
	CMFCPropertyGridCtrl m_edgePropGrid;

	void SetNetworkName(std::string&);
	void SetNetworkDescription(std::string&);

	void AddNodePropertyPage(Node* node);
	void AddEdgePropertyPage(Edge* edge);
	void RemoveNodePropertyPage(Node* node);
	void RemoveEdgePropertyPage(Edge* edge);
	void RemoveAllProperties() { m_nodePropGrid.RemoveAll(); m_edgePropGrid.RemoveAll(); }

protected:
	//virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	};


