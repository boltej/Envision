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
// LulcInfoDialog.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "LulcInfoDialog.h"
#include ".\lulcinfodialog.h"

#include <stdio.h>

extern CEnvDoc      *gpDoc;
extern EnvModel *gpModel;

// LulcInfoDialog dialog

IMPLEMENT_DYNAMIC(LulcInfoDialog, CDialog)
LulcInfoDialog::LulcInfoDialog(CWnd* pParent /*=NULL*/)
:  CDialog(LulcInfoDialog::IDD, pParent),
m_expanded( FALSE ),
m_windowRect(0,0,0,0)
   {
   m_pLulcTree = &gpModel->m_lulcTree;
   //ASSERT(m_pLulcTree);
   }

LulcInfoDialog::~LulcInfoDialog()
   {
   }

void LulcInfoDialog::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_EXPAND, m_expandButton);
   DDX_Control(pDX, IDC_LULCC, m_lulcC);
   DDX_Control(pDX, IDC_LULCB, m_lulcB);
   DDX_Control(pDX, IDC_LULCA, m_lulcA);
   DDX_Control(pDX, IDC_LULCTREECTRL, m_treeCtrl);
   }

BEGIN_MESSAGE_MAP(LulcInfoDialog, CDialog)
   ON_WM_SIZE()
   ON_NOTIFY(NM_CLICK, IDC_LULCTREECTRL, OnNMClickLulctreectrl)
   ON_NOTIFY(TVN_SELCHANGED, IDC_LULCTREECTRL, OnTvnSelchangedLulctreectrl)
   ON_EN_CHANGE(IDC_LULCC, OnEnChangeLulcc)
   ON_EN_CHANGE(IDC_LULCB, OnEnChangeLulcb)
   ON_EN_CHANGE(IDC_LULCA, OnEnChangeLulca)
   ON_BN_CLICKED(IDC_EXPAND, OnBnClickedExpand)
END_MESSAGE_MAP()


// LulcInfoDialog message handlers

BOOL LulcInfoDialog::OnInitDialog()
   {
   CDialog::OnInitDialog();

   if ( !m_windowRect.IsRectNull() )
      {
      //CRect rect( m_windowRect );
      //CWnd *pWnd = GetParent();
      //if ( pWnd != NULL )
      //   pWnd->ScreenToClient( rect );
      //MoveWindow( rect );
      MoveWindow( m_windowRect );
      }
   else
      CenterWindow();

   // get pointers to the controls
   //m_pTreeCtrl = (CTreeCtrl*) GetDlgItem( IDC_LULCTREECTRL );
   //m_pLulcA   = (CEdit*) GetDlgItem( IDC_LULCA );
   //m_pLulcB   = (CEdit*) GetDlgItem( IDC_LULCB );
   //m_pLulcC   = (CEdit*) GetDlgItem( IDC_LULCC );

   // size the TreeCtrl
   CRect rect;
   GetClientRect( rect );
   rect.DeflateRect( 10, 70, 10, 10 );
   if ( IsWindow( m_treeCtrl.m_hWnd ) )
      m_treeCtrl.MoveWindow( rect );

   // populate the tree control
   AddToCtrl( &m_pLulcTree->GetRootNode()->m_childNodeArray, NULL );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void LulcInfoDialog::AddToCtrl( LulcNodeArray *pChildArray,  HTREEITEM hParent  )
   {
   for ( int i=0; i<pChildArray->GetCount(); i++ )
      {
      LulcNode *pNode = pChildArray->GetAt( i );

      //insert the node
      HTREEITEM hNode = m_treeCtrl.InsertItem( pNode->m_name, hParent);
      m_treeCtrl.SetItemData( hNode, (DWORD_PTR) pNode );

      //insert all the nodes children
      LulcNodeArray *pChildArray = &pNode->m_childNodeArray;
      AddToCtrl( pChildArray, hNode );
      }
   }

void LulcInfoDialog::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);

   if ( IsWindow( m_treeCtrl.m_hWnd ) )
      m_treeCtrl.MoveWindow( 10, 70, cx-20, cy-80 );
   }

void LulcInfoDialog::OnNMClickLulctreectrl(NMHDR *pNMHDR, LRESULT *pResult)
   {
   m_treeCtrl.SetFocus();

   *pResult = 0;
   }

void LulcInfoDialog::OnTvnSelchangedLulctreectrl(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

   if ( GetFocus() == &m_treeCtrl )
      {    
      HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
      LulcNode *pNode = (LulcNode*) m_treeCtrl.GetItemData( hItem );

      int level = pNode->GetNodeLevel();
      CString str;

      if ( level == 3 )
         {
         str.Format("%d", pNode->m_id );
         m_lulcC.SetWindowText(str);
         pNode = pNode->m_pParentNode;
         level = 2;
         }
      else
         m_lulcC.SetWindowText("");
      if ( level == 2 )
         {
         str.Format("%d", pNode->m_id );
         m_lulcB.SetWindowText(str);
         pNode = pNode->m_pParentNode;
         level = 1;
         }
      else
         m_lulcB.SetWindowText("");
      if ( level == 1 )
         {
         str.Format("%d", pNode->m_id );
         m_lulcA.SetWindowText(str);
         pNode = pNode->m_pParentNode;
         }
      else
         ASSERT(0);
      }

   *pResult = 0;
   }

void LulcInfoDialog::OnEnChangeLulcc()
   {
   if ( GetFocus() == &m_lulcC /*m_pLulcC*/ ) 
      {
      char text[64];
      m_lulcC.GetWindowText( text, 64 );
      int id;
      id = atoi(text);

      HTREEITEM hItemToSelect = FindItem( NULL, 3, id );  

      CollapseAll( NULL );
      if ( hItemToSelect != NULL )
         {
         m_treeCtrl.SelectItem( hItemToSelect );
         WriteParentsAbove( hItemToSelect );
         }
      else  //typed nonsense
         {
         m_lulcA.SetWindowText( "" );
         m_lulcB.SetWindowText( "" );
         }
      }
   }

void LulcInfoDialog::OnEnChangeLulcb()
   {
   if ( GetFocus() == &m_lulcB ) 
      {
      char text[64];
      m_lulcB.GetWindowText( text, 64 );
      int id;
      id = atoi(text);

      m_lulcC.SetWindowText( "" );
      CollapseAll( NULL );

      HTREEITEM hItemToSelect = FindItem( NULL, 2, id );  

      if ( hItemToSelect != NULL )
         {
         m_treeCtrl.SelectItem( hItemToSelect );
         WriteParentsAbove( hItemToSelect );
         }
      else  //typed nonsense
         m_lulcA.SetWindowText( "" );
      }
   }

void LulcInfoDialog::OnEnChangeLulca()
   {
   if ( GetFocus() == &m_lulcA ) 
      {
      char text[64];
      m_lulcA.GetWindowText( text, 64 );
      int id;
      id = atoi(text);

      m_lulcC.SetWindowText( "" );
      m_lulcB.SetWindowText( "" );
      CollapseAll( NULL );

      HTREEITEM hItemToSelect = FindItem( NULL, 1, id );
      if ( hItemToSelect != NULL )
         m_treeCtrl.SelectItem( hItemToSelect );
      }
   }


HTREEITEM LulcInfoDialog::FindItem( HTREEITEM hItem, int level, int id )
   {
   if ( hItem == NULL )
      hItem = m_treeCtrl.GetRootItem();

   HTREEITEM retItem = NULL;

   while ( hItem != NULL )
      {
      // is this the one?
      LulcNode *pNode = (LulcNode*) m_treeCtrl.GetItemData( hItem );
      if ( TestNode( pNode, level, id ) )
         return hItem;

      // test children if there are any
      if ( m_treeCtrl.GetChildItem( hItem ) != NULL )
         retItem = FindItem( m_treeCtrl.GetChildItem( hItem ), level, id );
      if ( retItem != NULL )
         break;

      hItem = m_treeCtrl.GetNextSiblingItem( hItem );
      }

   return retItem;

   }


void LulcInfoDialog::WriteParentsAbove( HTREEITEM hItem )
   {
   hItem = m_treeCtrl.GetParentItem( hItem );
   LulcNode *pNode = (LulcNode*) m_treeCtrl.GetItemData( hItem );

   CString msg;
   switch ( pNode->GetNodeLevel() )
      {
      case 2:
         msg.Format( "%d", pNode->m_id );
         m_lulcB.SetWindowText( msg );
         pNode = pNode->m_pParentNode;
      case 1:
         msg.Format( "%d", pNode->m_id );
         m_lulcA.SetWindowText( msg );
         break;
      }
   }

void LulcInfoDialog::CollapseAll( HTREEITEM hItem )
   {
   if ( hItem == NULL )
      hItem = m_treeCtrl.GetRootItem();


   while ( hItem != NULL )
      {
      m_treeCtrl.Expand( hItem, TVE_COLLAPSE );
      if ( m_treeCtrl.GetChildItem( hItem ) != NULL )
         CollapseAll( m_treeCtrl.GetChildItem( hItem ) );

      hItem = m_treeCtrl.GetNextSiblingItem( hItem );
      }

   m_treeCtrl.SelectItem( NULL );
   }


void LulcInfoDialog::OnBnClickedExpand()
   {
   HTREEITEM hItem = m_treeCtrl.GetRootItem();

   if ( m_expanded )
      {
      m_expanded = FALSE;
      CollapseAll( hItem );
      m_expandButton.SetWindowText("Expand");
      }
   else
      {
      m_expanded = TRUE;
      ExpandAll( hItem );
      m_treeCtrl.EnsureVisible( hItem );
      m_expandButton.SetWindowText("Collapse");
      }

   }

void LulcInfoDialog::ExpandAll( HTREEITEM hItem )
   {
   while ( hItem != NULL )
      {
      m_treeCtrl.Expand( hItem, TVE_EXPAND );
      if ( m_treeCtrl.GetChildItem( hItem ) != NULL )
         ExpandAll( m_treeCtrl.GetChildItem( hItem ) );

      hItem = m_treeCtrl.GetNextSiblingItem( hItem );
      }

   m_treeCtrl.SelectItem( NULL );
   }

BOOL LulcInfoDialog::Create(CWnd* pParentWnd)
   {
   // TODO: Add your specialized code here and/or call the base class

   return CDialog::Create(IDD_LULCINFO, pParentWnd);
   }

void LulcInfoDialog::OnCancel()
   {
   //CDialog::OnCancel();
   DestroyWindow();
   }

BOOL LulcInfoDialog::DestroyWindow()
   {
   GetWindowRect( m_windowRect );

   return CDialog::DestroyWindow();
   }
