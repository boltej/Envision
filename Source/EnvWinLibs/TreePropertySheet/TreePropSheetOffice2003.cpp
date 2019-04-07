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
// TreePropSheetOffice2003.cpp: implementation of the CTreePropSheetOffice2003 class.
//
//////////////////////////////////////////////////////////////////////
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
#include "TreePropSheetOffice2003.h"
#include "PropPageFrameOffice2003.h"

namespace TreePropSheet
{

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CTreePropSheetOffice2003, CTreePropSheetEx)

CTreePropSheetOffice2003::CTreePropSheetOffice2003()
{
}

CTreePropSheetOffice2003::CTreePropSheetOffice2003(UINT nIDCaption,CWnd* pParentWnd /*=NULL*/,UINT iSelectPage /*=0*/)
 : CTreePropSheetEx(nIDCaption, pParentWnd, iSelectPage)
{
}

CTreePropSheetOffice2003::CTreePropSheetOffice2003(LPCTSTR pszCaption,CWnd* pParentWnd /*=NULL*/,UINT iSelectPage /*=0*/)
 : CTreePropSheetEx(pszCaption, pParentWnd, iSelectPage)
{
}

CTreePropSheetOffice2003::~CTreePropSheetOffice2003()
{
}

//////////////////////////////////////////////////////////////////////
// Overrided implementation helpers
//////////////////////////////////////////////////////////////////////

CPropPageFrame* CTreePropSheetOffice2003::CreatePageFrame()
{
  return new CPropPageFrameOffice2003;
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CTreePropSheetOffice2003, CTreePropSheetEx)
	//{{AFX_MSG_MAP(CTreePropSheetOffice2003)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////
// Overridings
/////////////////////////////////////////////////////////////////////

BOOL CTreePropSheetOffice2003::OnInitDialog() 
{
  BOOL bRet = CTreePropSheetEx::OnInitDialog();

  // If in tree mode, update the tree style.
	if( IsTreeViewMode() && GetPageTreeControl() )
  {
    GetPageTreeControl()->ModifyStyle( TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_TRACKSELECT, TVS_FULLROWSELECT|TVS_SHOWSELALWAYS, 0 );
  }

  return bRet;
}


}; // namespace TreePropSheet
