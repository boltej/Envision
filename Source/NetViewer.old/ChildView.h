
// ChildView.h : interface of the CChildView class
//


#pragma once

#include "NetView.h"
//#include <ugctrl.h>

//using namespace std;

// CChildView window

class CChildView : public CWnd
{
// Construction
public:
	CChildView();

	virtual ~CChildView(){}

// Attributes
public:

	//CUGCtrl m_grid;

	NetView netView;

// Operations
public:

// Overrides
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:

	// Generated message map functions
protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileOpen();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	};

