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
// ResizablePage.cpp : implementation file
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000-2002 by Paolo Messina
// (http://www.geocities.com/ppescher - ppescher@yahoo.com)
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

#include "EnvWinLibs.h"
#include "ResizablePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResizablePage

IMPLEMENT_DYNCREATE(CResizablePage, CPropertyPage)

CResizablePage::CResizablePage()
{
}

CResizablePage::CResizablePage(UINT nIDTemplate, UINT nIDCaption)
	: CPropertyPage(nIDTemplate, nIDCaption)
{
}

CResizablePage::CResizablePage(LPCTSTR lpszTemplateName, UINT nIDCaption)
	: CPropertyPage(lpszTemplateName, nIDCaption)
{
}

CResizablePage::~CResizablePage()
{
}


BEGIN_MESSAGE_MAP(CResizablePage, CPropertyPage)
	//{{AFX_MSG_MAP(CResizablePage)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CResizablePage message handlers

void CResizablePage::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
	ArrangeLayout();
}

BOOL CResizablePage::OnEraseBkgnd(CDC* pDC) 
{
	// Windows XP doesn't like clipping regions ...try this!
	EraseBackground(pDC);
	return TRUE;

/*	ClipChildren(pDC);	// old-method (for safety)
	
	return CPropertyPage::OnEraseBkgnd(pDC);
*/
}
