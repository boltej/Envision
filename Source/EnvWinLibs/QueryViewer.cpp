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
// QueryViewer.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include "QueryViewer.h"

#include "maplayer.h"
#include "queryengine.h"


// QueryViewer dialog

IMPLEMENT_DYNAMIC(QueryViewer, CDialog)

QueryViewer::QueryViewer( LPCTSTR query, MapLayer *pLayer, CWnd* pParent /*=NULL*/)
	: CDialog(QueryViewer::IDD, pParent)
   , m_pLayer( pLayer )
   , m_query( query )
{ }


QueryViewer::~QueryViewer()
{ }


void QueryViewer::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_LIB_QVQUERY, m_query);
   DDX_Control(pDX, IDC_LIB_QVTREE, m_tree);
   DDX_Control(pDX, IDC_DEBUGINFO, m_debugInfo);
   }


BEGIN_MESSAGE_MAP(QueryViewer, CDialog)
END_MESSAGE_MAP()


// QueryViewer message handlers

void QueryViewer::OnOK()
   {
   UpdateData( 1 );

   m_tree.DeleteAllItems();

   QueryEngine qEngine( m_pLayer );

   qEngine.m_debug = true;

   Query *pQuery = qEngine.ParseQuery( m_query, 0, "Query Viewer Setup" );

   qEngine.m_debug = false;

   HTREEITEM hItem = m_tree.InsertItem( _T( "Query" ) );

   QNode *pNode = pQuery->m_pRoot;

   AddNode( hItem, pNode );

   m_tree.Expand( hItem, TVE_EXPAND );

   m_debugInfo.SetWindowText(qEngine.m_debugInfo);
   }


void QueryViewer::AddNode( HTREEITEM hParent, QNode *pNode )
   {
   CString label;
   pNode->GetLabel( label, 0 );

   CString _label;
   _label.Format("%s (%i)", (LPCTSTR) label, pNode->m_nodeType );

   HTREEITEM hItem = m_tree.InsertItem( _label, hParent );

   if ( pNode->m_pLeft != NULL )
      AddNode( hItem, pNode->m_pLeft );

   if ( pNode->m_pRight != NULL )
      AddNode( hItem, pNode->m_pRight );

   m_tree.Expand( hItem, TVE_EXPAND );
   }

