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
// LulcEditor.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "LulcEditor.h"
#include "EnvDoc.h"
#include <DirPlaceholder.h>
#include "SaveToDlg.h"
#include <stdio.h>
#include <Path.h>

extern CEnvDoc      *gpDoc;
extern EnvModel     *gpModel;

// LulcEditor dialog

IMPLEMENT_DYNAMIC(LulcEditor, CDialog)

LulcEditor::LulcEditor(CWnd* pParent /*=NULL*/)
	: CDialog(LulcEditor::IDD, pParent)
   , m_expanded( FALSE )
   {
   m_pLulcTree = &gpModel->m_lulcTree;
   }


LulcEditor::~LulcEditor()
{
}

void LulcEditor::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_EXPAND, m_expandButton);
DDX_Control(pDX, IDC_LULCTREECTRL, m_treeCtrl);
DDX_Control(pDX, IDC_LABEL, m_label);
DDX_Control(pDX, IDC_ID, m_attrValue);
   }


BEGIN_MESSAGE_MAP(LulcEditor, CDialog)
   ON_WM_SIZE()
   ON_NOTIFY(NM_CLICK, IDC_LULCTREECTRL, OnNMClickLulctreectrl)
   ON_NOTIFY(TVN_SELCHANGED, IDC_LULCTREECTRL, OnTvnSelchangedLulctreectrl)
   ON_BN_CLICKED(IDC_EXPAND, OnBnClickedExpand)
   ON_BN_CLICKED(IDC_ADD, &LulcEditor::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_REMOVE, &LulcEditor::OnBnClickedRemove)
   ON_BN_CLICKED(IDC_EXPORT, &LulcEditor::OnBnClickedExport)
   ON_EN_CHANGE(IDC_LABEL, &LulcEditor::OnEnChangeLabel)
   ON_EN_CHANGE(IDC_ID, &LulcEditor::OnEnChangeId)
END_MESSAGE_MAP()


BEGIN_EASYSIZE_MAP(LulcEditor)
   //            ctrl            left           top          right      bottom            
   EASYSIZE( IDC_LULCTREECTRL,  ES_BORDER,    ES_BORDER,   ES_BORDER,   ES_BORDER,   0)
   EASYSIZE( IDOK,              ES_KEEPSIZE,  ES_BORDER,   ES_BORDER,   ES_KEEPSIZE, 0 )
   EASYSIZE( IDCANCEL,          ES_KEEPSIZE,  ES_BORDER,   ES_BORDER,   ES_KEEPSIZE, 0 )
   EASYSIZE( IDC_EXPAND,        ES_KEEPSIZE,  ES_BORDER,   ES_BORDER,   ES_KEEPSIZE, 0 )
   EASYSIZE( IDC_ADD,           ES_KEEPSIZE,  ES_BORDER,   ES_BORDER,   ES_KEEPSIZE, 0 )
   EASYSIZE( IDC_REMOVE,        ES_KEEPSIZE,  ES_BORDER,   ES_BORDER,   ES_KEEPSIZE, 0 )
   EASYSIZE( IDC_EXPORT,        ES_KEEPSIZE,  ES_BORDER,   ES_BORDER,   ES_KEEPSIZE, 0 )
   EASYSIZE( IDC_STATICLABEL,   ES_BORDER,    ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,   0 )
   EASYSIZE( IDC_LABEL,         ES_BORDER,    ES_KEEPSIZE, ES_BORDER,   ES_BORDER,   0 )
   EASYSIZE( IDC_STATICID,      ES_BORDER,    ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,   0 )
   EASYSIZE( IDC_ID,            ES_BORDER,    ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,   0 )
END_EASYSIZE_MAP


BOOL LulcEditor::OnInitDialog()
   {
   m_isDirty = false;

   CDialog::OnInitDialog();

   INIT_EASYSIZE;

   // populate the tree control
   AddToCtrl( &m_pLulcTree->GetRootNode()->m_childNodeArray, NULL );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void LulcEditor::AddToCtrl( std::vector<LulcNode*> *pChildArray,  HTREEITEM hParent  )
   {
   for ( int i=0; i<pChildArray->size(); i++ )
      {
      LulcNode *pNode = pChildArray->at( i );

      // insert the node
      HTREEITEM hNode = m_treeCtrl.InsertItem( pNode->m_name, hParent);
      m_treeCtrl.SetItemData( hNode, (DWORD_PTR) pNode );

      // insert all the nodes children
      std::vector<LulcNode*> *pChildArray = &pNode->m_childNodeArray;
      AddToCtrl( pChildArray, hNode );
      }
   }


void LulcEditor::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);
   UPDATE_EASYSIZE;
   }

void LulcEditor::OnNMClickLulctreectrl(NMHDR *pNMHDR, LRESULT *pResult)
   {
   m_treeCtrl.SetFocus();

   *pResult = 0;
   }


void LulcEditor::OnTvnSelchangedLulctreectrl(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

   if ( GetFocus() == &m_treeCtrl )
      {    
      HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
      LulcNode *pNode = (LulcNode*) m_treeCtrl.GetItemData( hItem );

      m_label.SetWindowText( pNode->m_name );

      TCHAR id[ 16 ];
      _itoa_s( pNode->m_id, id, 16, 10 );
      m_attrValue.SetWindowText( id );
      }

   *pResult = 0;
   }

/*
void LulcEditor::OnEnChangeLulca()
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

*/
HTREEITEM LulcEditor::FindItem( HTREEITEM hItem, int level, int id )
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


void LulcEditor::CollapseAll( HTREEITEM hItem )
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


void LulcEditor::OnBnClickedExpand()
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

void LulcEditor::ExpandAll( HTREEITEM hItem )
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


void LulcEditor::OnCancel()
   {
   if ( m_isDirty )
      {
      int retVal = MessageBox( "Do you want to discard change you've made?", "", MB_OKCANCEL );
      if ( retVal == IDCANCEL )
         return;
      }

   CDialog::OnCancel();
   }


void LulcEditor::OnOK()
   {
   if ( m_isDirty )
      {
      int retVal = MessageBox( _T("Do you want to save changes?"), _T("Save Changes"), MB_YESNOCANCEL);

      switch( retVal )
         {
         case IDYES:
            OnBnClickedExport();
            break;

         case IDNO:
            break;

         case IDCANCEL:
            return;
         }
      }

   CDialog::OnOK();
   }



void LulcEditor::OnBnClickedAdd()
   {
   // TODO: Add your control notification handler code here
   }


void LulcEditor::OnBnClickedRemove()
   {
   // TODO: Add your control notification handler code here
   }


void LulcEditor::OnBnClickedExport()
   {
   DirPlaceholder d;
   CString startPath = m_pLulcTree->m_path;  
      
   if ( startPath.IsEmpty() )
      startPath = gpDoc->m_iniFile;

   SaveToDlg dlg( "LULC", startPath );

   int retVal = (int) dlg.DoModal();

   if ( retVal == IDOK )
      {
      if ( dlg.m_saveThisSession )
         gpDoc->SetChanged( CHANGED_LULC );
   
      if ( dlg.m_saveToDisk )
         {
         m_path = dlg.m_path;

         nsPath::CPath path( m_path );
         CString ext = path.GetExtension();

         if (ext.CompareNoCase( _T("envx") ) == 0 )
            gpDoc->OnSaveDocument( m_path );
         else
            m_pLulcTree->SaveXml( m_path );

         gpDoc->UnSetChanged( CHANGED_LULC );
         }

      m_isDirty = 0;      
      }
   }




void LulcEditor::OnEnChangeLabel()
   {
   CString label;
   m_label.GetWindowText( label );

   HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
   LulcNode *pNode = (LulcNode*) m_treeCtrl.GetItemData( hItem );

   pNode->m_name = label;
   m_treeCtrl.SetItemText( hItem, label );
   m_isDirty = true;
   }


void LulcEditor::OnEnChangeId()
   {
   CString id;
   m_attrValue.GetWindowText( id );

   HTREEITEM hItem = m_treeCtrl.GetSelectedItem();
   LulcNode *pNode = (LulcNode*) m_treeCtrl.GetItemData( hItem );

   pNode->m_id = atoi( id );
   m_isDirty = true;
   }
