// LogWnd.cpp : implementation file
//
#include "EnvWinLibs.h"
#pragma hdrstop

#include "LogWnd.h"


// LogWnd

IMPLEMENT_DYNAMIC(LogWnd, CWnd)

LogWnd::LogWnd()
{

}

LogWnd::~LogWnd()
{
}


BEGIN_MESSAGE_MAP(LogWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()



// LogWnd message handlers



int LogWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	this->GetClientRect(rect);

	m_logCtrl.Create(WS_CHILD | WS_VISIBLE,
		rect,this,0x118);

	return 0;
	}


void LogWnd::OnSize(UINT nType, int cx, int cy)
	{
	CWnd::OnSize(nType, cx, cy);
	//CRect rect;
	//this->GetClientRect(rect);

	this->m_logCtrl.SetWindowPos(NULL, 0, 0, cx, cy, 0);
	//m_logCtrl.MoveWindow(rect, 1);
	// TODO: Add your message handler code here
	}
