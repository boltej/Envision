#include "pch.h"
#include "NetForm.h"

#include "NetView.h"

#include "resource.h"

IMPLEMENT_DYNCREATE(NetForm, CWnd)

NetForm::NetForm()
   {
   };

NetForm::~NetForm()
   {
   }



//INT_PTR NetFormDlgProc(HWND, UINT, WPARAM, LPARAM)
//   {
//   return 1;
//   }


BEGIN_MESSAGE_MAP(NetForm, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


void NetForm::SetNetworkName(std::string &name)
	{
	m_netName.SetWindowText(name.c_str());
	}

void NetForm::SetNetworkDescription(std::string&desc)
	{
	m_netDescription.SetWindowText(desc.c_str());

	}

int NetForm::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	const int margin = 3;
	CRect rect;
	GetClientRect(&rect);
	rect.right = panelWidth-margin;
	
	// network name
	rect.left = margin; // left margin
	rect.top = margin;	// top margin
	rect.bottom = rect.top + 100;  // should be based on font size
	m_netName.Create("Network", WS_CHILD | WS_VISIBLE, rect, this, 1000);

	// net stats
	rect.top = rect.bottom + margin;
	rect.bottom = rect.top + 200;
	m_netDescription.Create("", WS_CHILD | WS_VISIBLE, rect, this, 1001);
	
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 600;

	// property grid
	m_nodePropGrid.Create(WS_CHILD | WS_VISIBLE, rect, this, 1010);
	m_edgePropGrid.Create(WS_CHILD, rect, this, 1011);

	// set fonts
	fontName.CreatePointFont(160, "Arial");
	fontDesc.CreatePointFont(120, "Arial");
	m_netName.SetFont(&fontName);
	m_netDescription.SetFont(&fontDesc);

	m_nodePropGrid.ShowWindow(SW_SHOW);
	m_edgePropGrid.ShowWindow(SW_SHOW);

	return 0;
	}


void NetForm::AddNodePropertyPage(Node* node)
	{
	std::string label = std::format("Node: {}", node->label.c_str());
	CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(label.c_str(), node->index);

	// construct a COleVariant object.
	//COleVariant var3DLook((short)VARIANT_FALSE, VT_BOOL);
	//pGroup->AddSubItem(new CMFCPropertyGridProperty(_T("3D Look"), var3DLook, _T("Specifies the dialog's font will be nonbold and controls will have a 3D border")));
	//CMFCPropertyGridProperty* pProp = new CMFCPropertyGridProperty(_T("Border"),
	//	_T("Dialog Frame"), _T("One of: None, Thin, Resizable, or Dialog Frame"));
	//pProp->AddOption(_T("None"));
	//pProp->AddOption(_T("Thin"));
	//pProp->AddOption(_T("Resizable"));
	//pProp->AddOption(_T("Dialog Frame"));
	//pProp->AllowEdit(FALSE);

	// count edges
	int size = node->inDegree;
	std::string ins = std::format("{} incomers: ", size);
	for (int i = 0; i < size - 1; i++)
		{ ins += node->inEdges[i]->label; ins += ", "; }
	if (size > 0)
		ins += node->inEdges[size - 1]->label;


	size = node->outDegree;
	std::string outs = std::format("{} outgoers: ", size);
	for (int i = 0; i < size - 1; i++)
		{ outs += node->outEdges[i]->label; outs += ", "; }
	if (size > 0)
		outs += node->outEdges[size - 1]->label;

	// build group w/ properties
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("ID"),				COleVariant(node->id.c_str()), "ID of the node", 1));   // 1
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("Label"),			COleVariant(node->label.c_str()), "Label (title) of the node", 2));    // 2
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("In Edges"),		COleVariant(ins.c_str()), "In Edges for this node", 3));			 // 3
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("Out Edges"),	COleVariant(outs.c_str()), "Out Edges for this node", 4));		 // 4 
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("Reactivity"),	COleVariant(node->reactivity), "Reactivity of the node(-1, 1)", 5));	 // 5
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("Influence"),	COleVariant(node->influence), "Influence of the node(-1???, 1)", 6)); // 6

	pGroup->AdjustButtonRect();
	//pGroup1->AllowEdit();
	pGroup->Enable();
	pGroup->Show();
	//pGroup->Redraw();

	m_nodePropGrid.AddProperty(pGroup, 1);
	}



void NetForm::AddEdgePropertyPage(Edge* edge)
	{
	std::string label = std::format("Edge: {}", edge->label.c_str());
	CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(label.c_str(), edge->index);

	// build group w/ properties
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("ID"), COleVariant(edge->id.c_str()), "ID of the edge", 1));   // 1
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("Label"), COleVariant(edge->label.c_str()), "Label (title) of the edge", 2));    // 2
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("From Node"), COleVariant(edge->pFromNode->label.c_str()), "FROM node for this edge", 3));			 // 3
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("To Node"), COleVariant(edge->pToNode->label.c_str()), "TO node for this edge", 4));		 // 4 
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("Signal Strength"), COleVariant(edge->signalStrength), "Signal Strength (-1???, 1)", 5));	 // 5
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("Trust"), COleVariant(edge->trust), "Trust Level of the edge(-1???, 1)", 6)); // 6
	pGroup->AddSubItem(new CMFCPropertyGridProperty(CString("Influence"), COleVariant(edge->influence), "Influence of the edge(-1???, 1)", 7));

	pGroup->AdjustButtonRect();
	//pGroup1->AllowEdit();
	pGroup->Enable();
	pGroup->Show();
	//pGroup->Redraw();

	m_edgePropGrid.AddProperty(pGroup, 1);
	}


void NetForm::RemoveNodePropertyPage(Node* node)
	{
	CMFCPropertyGridProperty* p = m_nodePropGrid.FindItemByData(node->index, 1);
	m_nodePropGrid.DeleteProperty(p);
	}

void NetForm::RemoveEdgePropertyPage(Edge* edge)
	{
	CMFCPropertyGridProperty* p = m_edgePropGrid.FindItemByData(edge->index, 1);
	m_edgePropGrid.DeleteProperty(p);
	}


BOOL NetForm::OnEraseBkgnd(CDC* pDC)
	{
	CBrush backBrush(::GetSysColor(COLOR_3DFACE));

	// Save old brush
	CBrush* pOldBrush = pDC->SelectObject(&backBrush);

	CRect rect;
	pDC->GetClipBox(&rect);     // Erase the area needed

	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
	pDC->SelectObject(pOldBrush);
	return TRUE;
	}


/*
void NetForm::SetNodeProperties(Node* node)
	{	
	m_nodePropGrid.GetProperty(1)->SetValue(COleVariant(node->id.c_str()));
	m_nodePropGrid.GetProperty(2)->SetValue(COleVariant(node->label.c_str()));

	int size = (int)node->inEdges.size();
	
	std::string ins = std::format("({})", size);
	for (int i = 0; i < size - 1; i++)
		{
		ins += node->inEdges[i]->label;
		ins += ", ";
		}
	if (size > 0 )
		ins += node->inEdges[size-1]->label;

	std::string outs;
	size = (int)node->outEdges.size();
	for (int i = 0; i < size - 1; i++)
		{
		outs += node->outEdges[i]->label;
		outs += ", ";
		}
	if (size > 0)
		outs += node->outEdges[size-1]->label;

	m_nodePropGrid.GetProperty(3)->SetValue(COleVariant(ins.c_str()));
	m_nodePropGrid.GetProperty(4)->SetValue(COleVariant(outs.c_str()));
	m_nodePropGrid.GetProperty(5)->SetValue(COleVariant(node->reactivity));
	m_nodePropGrid.GetProperty(6)->SetValue(COleVariant(node->influence));
	m_edgePropGrid.ShowWindow(SW_HIDE);
	m_nodePropGrid.ShowWindow(SW_SHOW);
	
	}

void NetForm::SetEdgeProperties(Edge* edge)
	{
	m_edgePropGrid.GetProperty(1)->SetValue(COleVariant(edge->id.c_str()));
	m_edgePropGrid.GetProperty(2)->SetValue(COleVariant(edge->label.c_str()));
	m_edgePropGrid.GetProperty(3)->SetValue(COleVariant(edge->pFromNode->label.c_str()));
	m_edgePropGrid.GetProperty(4)->SetValue(COleVariant(edge->pToNode->label.c_str()));
	m_edgePropGrid.GetProperty(5)->SetValue(COleVariant(edge->signalStrength));
	m_edgePropGrid.GetProperty(6)->SetValue(COleVariant(edge->trust));
	m_edgePropGrid.GetProperty(7)->SetValue(COleVariant(edge->influence));

	m_nodePropGrid.ShowWindow(SW_HIDE);
	m_edgePropGrid.ShowWindow(SW_SHOW);

	}
	*/
