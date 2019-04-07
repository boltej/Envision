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
// PropPageFrameBordered.cpp: implementation of the CPropPageFrameBordered class.
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2004 by Yves Tkaczyk
// (http://www.tkaczyk.net - yves@tkaczyk.net)
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// Documentation: http://www.codeproject.com/property/treepropsheetex.asp
// CVS tree:      http://sourceforge.net/projects/treepropsheetex
//
/////////////////////////////////////////////////////////////////////////////

#include "EnvWinLibs.h"
#include "PropPageFrameBordered.h"
#include "ThemeLibEx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

namespace TreePropSheet
{

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPropPageFrameBordered::CPropPageFrameBordered()
{
}

CPropPageFrameBordered::~CPropPageFrameBordered()
{
}

/////////////////////////////////////////////////////////////////////
// Overridings
/////////////////////////////////////////////////////////////////////

BOOL CPropPageFrameBordered::Create(DWORD dwWindowStyle, const RECT &rect, CWnd *pwndParent, UINT nID)
{
  if (GetThemeLib().IsAvailable() && GetThemeLib().IsThemeActive() && GetThemeLib().IsAppThemed() )
	{
    // Delegate to base class.
    return CPropPageFrameEx::Create(dwWindowStyle, rect, pwndParent, nID);
  }
	else
	{
	  return CWnd::CreateEx(
      WS_EX_CLIENTEDGE|WS_EX_TRANSPARENT,
      AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW, AfxGetApp()->LoadStandardCursor(IDC_ARROW), GetSysColorBrush(COLOR_3DFACE)),
		  _T("PropPageFrameBordered"),
		  dwWindowStyle, rect, pwndParent, nID);
  }
}

/////////////////////////////////////////////////////////////////////
// CPropPageFrameEx overrides
/////////////////////////////////////////////////////////////////////

void CPropPageFrameBordered::DrawBackground(CDC* pDC)
{
  if (GetThemeLib().IsAvailable() && GetThemeLib().IsThemeActive() && GetThemeLib().IsAppThemed() )
	{
    // Delegate to base class.
    CPropPageFrameEx::DrawBackground( pDC );
  }
	else
	{
    // Draw our own background.
		CWnd::OnEraseBkgnd( pDC );
		CRect	rect;
		GetClientRect(rect);
    ::FillRect(pDC->m_hDC, &rect, ::GetSysColorBrush( COLOR_BTNFACE ) );
	}
}

};  // namespace TreePropSheet