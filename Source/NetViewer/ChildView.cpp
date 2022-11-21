// This MFC Samples source code demonstrates using MFC Microsoft Office Fluent User Interface 
// (the "Fluent UI") and is provided only as referential material to supplement the 
// Microsoft Foundation Classes Reference and related electronic documentation 
// included with the MFC C++ library software.  
// License terms to copy, use or distribute the Fluent UI are available separately.  
// To learn more about our Fluent UI licensing program, please visit 
// https://go.microsoft.com/fwlink/?LinkId=238214.
//
// Copyright (C) Microsoft Corporation
// All rights reserved.

// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "NetViewer.h"
#include "ChildView.h"
#include "MainFrm.h"

#include "resource.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
{
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
	ON_WM_PAINT()
	ON_COMMAND(ID_FILE_OPEN, &CChildView::OnFileOpen)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

void CChildView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CWnd::OnPaint() for painting messages
}


void CChildView::OnFileOpen()
	{
	CFileDialog fOpenDlg(TRUE, _T("gexf"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, _T("GEXF Files(*.gexf)|*.gexf|All files (*.*)|*.*||"), this);

	// Initializes m_pOFN structure
	fOpenDlg.m_pOFN->lpstrTitle = _T("Network Files");

	// Call DoModal
	if (fOpenDlg.DoModal() == IDOK)
		{
		LPCTSTR path = fOpenDlg.GetOFN().lpstrFile;
		bool ok = netView.LoadNetwork(path);
		netForm.SetNetworkName(netView.name);
		netForm.SetNetworkDescription(netView.desc);

		//TRACKMOUSEEVENT e{};
		//e.dwFlags = TME_HOVER;
		//e.hwndTrack = netView.GetSafeHwnd();
		//e.dwHoverTime = HOVER_DEFAULT;
		//TrackMouseEvent(&e);
		}

	netView.Invalidate();
	}


int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	this->GetClientRect(&rect);

	CRect formRect(rect);
	formRect.right = panelWidth;
	CRect viewRect(rect);
	viewRect.left = panelWidth;


	netForm.Create(NULL, "NetForm", WS_CHILD | WS_VISIBLE, formRect, this, 100);
	netView.Create(NULL, "NetView", WS_CHILD | WS_VISIBLE | WS_BORDER, viewRect, this, 101);

	netView.netForm = &netForm;
	
	return 0;
	}


void CChildView::OnSize(UINT nType, int cx, int cy)
	{
	CWnd::OnSize(nType, cx, cy);

	netForm.MoveWindow(0, 0, panelWidth, cy, TRUE);
	netView.MoveWindow(panelWidth, 0, cx - panelWidth, cy, TRUE);
	}