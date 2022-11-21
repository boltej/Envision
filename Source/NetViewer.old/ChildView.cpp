
// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "NetViewer.h"
#include "ChildView.h"
#include "MainFrm.h"


//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif


// CChildView

CChildView::CChildView()
{
}

//CChildView::~CChildView()
//{
//}


BEGIN_MESSAGE_MAP(CChildView, CWnd)
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
		}


	netView.InvalidateRect(NULL);
	netView.UpdateWindow();
	// TODO: Add your command handler code here
	}



int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	this->GetClientRect(&rect);

	netView.Create(NULL, "NetView", WS_CHILD | WS_BORDER | WS_VISIBLE, rect, this, 100);

	return 0;
	}


void CChildView::OnSize(UINT nType, int cx, int cy)
	{
	CWnd::OnSize(nType, cx, cy);

	netView.MoveWindow(0, 0, cx, cy, TRUE);
	}
