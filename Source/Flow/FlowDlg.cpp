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
// FlowDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Flow.h"
#include "FlowDlg.h"

#include "PPBasics.h"



// FlowDlg

IMPLEMENT_DYNAMIC(FlowDlg, CTreePropSheetEx)


FlowDlg::FlowDlg(FlowModel *pFlow, LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CTreePropSheetEx(pszCaption, pParentWnd, iSelectPage)
   , m_basicPage( pFlow )
   , m_outputsPage( pFlow )
   {
   SetTreeViewMode(TRUE, FALSE, FALSE ); 
   
   AddPage( &m_basicPage );
   AddPage( &m_outputsPage );
   }

FlowDlg::~FlowDlg()
{
}


BEGIN_MESSAGE_MAP(FlowDlg, CTreePropSheetEx)
END_MESSAGE_MAP()


// FlowDlg message handlers


//BOOL FlowDlg::OnInitDialog()
//   {
//   BOOL bResult = CTreePropSheetEx::OnInitDialog();
//
//   return bResult;
//   }
//