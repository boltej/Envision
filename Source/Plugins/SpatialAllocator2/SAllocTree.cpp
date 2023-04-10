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
// SAllocTree.cpp : implementation file
//

#include "stdafx.h"

#include "SAllocTree.h"
#include "SAllocView.h"

extern SAllocView *gpSAllocView;

// SAllocTreeCtrl

IMPLEMENT_DYNAMIC(SAllocTreeCtrl, CTreeCtrl)
SAllocTreeCtrl::SAllocTreeCtrl()
{
}

SAllocTreeCtrl::~SAllocTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(SAllocTreeCtrl, CTreeCtrl)
   ON_NOTIFY_REFLECT(TVN_SELCHANGED, &SAllocTreeCtrl::OnTvnSelchanged)
END_MESSAGE_MAP()


// SAllocTreeCtrl message handlers

//void SAllocTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
//{
//   UINT uFlags;
//   HTREEITEM hItem = HitTest(point, &uFlags);
//
//   if ((hItem != NULL) && (TVHT_ONITEM & uFlags))
//      OpenItem(hItem);
//
//   CTreeCtrl::OnLButtonDblClk(nFlags, point);
//}



//void SAllocTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//{
//   if (nChar == VK_RETURN)
//   {
//      HTREEITEM hItem = GetSelectedItem();
//
//      if (hItem != NULL)
//         OpenItem(hItem);
//   }
//
//   CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
//}
//


void SAllocTreeCtrl::OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

   // get selected item
   NODE_INFO *pInfo = (NODE_INFO*)pNMTreeView->itemNew.lParam;

   // what was the old item?
   NODE_INFO *pOldInfo = NULL;
   if (pNMTreeView->itemOld.hItem > 0)
      pOldInfo = (NODE_INFO*)pNMTreeView->itemOld.lParam;

   SAllocTree *pParent = (SAllocTree*)GetParent();

   if (pOldInfo != NULL)
      {
      // are we leaving an allocation?
      if (pOldInfo->pAlloc != NULL)
         pParent->m_pSAllocWnd->SaveFields();
         // or an allocationSet?
      if (pOldInfo->pSet != NULL && pOldInfo->isResult == false )
         pParent->m_pSAllocWnd->SaveFields();
      }

   // is it an allocation set?
   if ( pInfo->pSet != NULL )
      pParent->m_pSAllocWnd->Activate(pInfo->pSet, pInfo->isResult );

   else if ( pInfo->pAlloc != NULL )
      pParent->m_pSAllocWnd->Activate(pInfo->pAlloc);

   *pResult = 0;
   }

// SAllocTree

IMPLEMENT_DYNCREATE(SAllocTree, CWnd)
SAllocTree::SAllocTree()
{
}

SAllocTree::~SAllocTree()
{
}


BEGIN_MESSAGE_MAP(SAllocTree, CWnd)
   ON_WM_CREATE()
   ON_WM_SIZE()
END_MESSAGE_MAP()



// SAllocTree message handlers


int SAllocTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   RECT rect;
   GetClientRect(&rect);

   m_tree.Create(WS_CHILD | WS_VISIBLE | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_HASLINES ,
      rect, this, 100);

   LoadTreeCtrl();

   return 0;
   }


void SAllocTree::OnSize(UINT nType, int cx, int cy)
   {
   m_tree.MoveWindow(CRect(0, 0, cx, cy));
   }


void SAllocTree::LoadTreeCtrl(void)
   {
   m_tree.DeleteAllItems();

   // note: this is the local copy of the View's allocation set, not the originals,
   // so the pointers stored with the NODE_INFOs should be interpreted as such
   for (int i = 0; i < (int)gpSAllocView->m_allocationSetArray.GetSize(); i++)
      {
      AllocationSet *pSet = gpSAllocView->m_allocationSetArray.GetAt(i);
      ASSERT(pSet != NULL);

      HTREEITEM hRoot = m_tree.GetRootItem();   // NULL at this point, since there are no entries

      // this gets inserted at the root level
      HTREEITEM hItemSet = m_tree.InsertItem(pSet->m_name);   // parent=hRoot, TVI_LAST
      NODE_INFO *pNodeInfo = new NODE_INFO( pSet, false, hItemSet );
      this->m_tree.m_nodeInfoArray.Add( pNodeInfo );
      m_tree.SetItemData(hItemSet, (DWORD_PTR) pNodeInfo );


      // insert individual allocations
      for (int j = 0; j < pSet->GetAllocationCount(); j++)
         {
         Allocation *pAlloc = pSet->GetAllocation(j);

         HTREEITEM hItemAlloc = m_tree.InsertItem(pAlloc->m_name, hItemSet);
         pNodeInfo = new NODE_INFO( pAlloc, hItemAlloc );
         this->m_tree.m_nodeInfoArray.Add( pNodeInfo );
         m_tree.SetItemData(hItemAlloc, (DWORD_PTR) pNodeInfo );
         }  // end of:  else ( attached to a submenu )

      // add final "result" node
      HTREEITEM hItemResult = m_tree.InsertItem( "Results", hItemSet);
      pNodeInfo = new NODE_INFO( pSet, true, hItemResult );
      this->m_tree.m_nodeInfoArray.Add( pNodeInfo );
      m_tree.SetItemData(hItemResult, (DWORD_PTR) pNodeInfo );
      }
   }


void SAllocTree::InsertAllocationSet(AllocationSet *pSet)
   {
   HTREEITEM hItemSet = m_tree.InsertItem(pSet->m_name);
   NODE_INFO *pNodeInfo = new NODE_INFO( pSet, false, hItemSet );
   this->m_tree.m_nodeInfoArray.Add( pNodeInfo );
   m_tree.SetItemData(hItemSet, (DWORD_PTR) pNodeInfo );
   }


void SAllocTree::InsertAllocation( Allocation *pAlloc )
   {
   // first, find assocaited allocationset
   HTREEITEM hSet = m_tree.GetRootItem();    // this will be the first allocation
   bool found = false;

   while( hSet != NULL )
      {
      NODE_INFO *pNodeInfo = (NODE_INFO*) m_tree.GetItemData( hSet );
       
      if ( pNodeInfo->pSet == pAlloc->m_pAllocSet )
         {
         found = true;
         break;
         }
      
      hSet = m_tree.GetNextItem(hSet, TVGN_NEXT);
      }
   
   if ( ! found )
      return;

   // have correct AllocSet node, add allocation to it.

   HTREEITEM hChildItem = m_tree.GetChildItem(hSet);
   HTREEITEM hLastItem;
   int itemCount = 0;

   while (hChildItem != NULL)
      {
      hLastItem = hChildItem; 
      hChildItem = m_tree.GetNextItem(hChildItem, TVGN_NEXT);
      itemCount++;
      }
   
   HTREEITEM hItemAlloc = NULL;

   if ( itemCount == 1 )  // fisrt allocation, only a "result" node present
      {
      hItemAlloc = m_tree.InsertItem(pAlloc->m_name, hSet, TVI_FIRST ); 
      }
   else
      {
      hLastItem = m_tree.GetNextItem( hLastItem, TVGN_PREVIOUS );  // last Allocation (before "results")
      hItemAlloc = m_tree.InsertItem(pAlloc->m_name, hSet, hLastItem);
      }

   NODE_INFO *pNodeInfo = new NODE_INFO(pAlloc,hItemAlloc);
   this->m_tree.m_nodeInfoArray.Add(pNodeInfo);
   m_tree.SetItemData(hItemAlloc, (DWORD_PTR)pNodeInfo);

   m_tree.SetFocus();
   m_tree.SelectItem(hItemAlloc);
   m_pSAllocWnd->Activate(pAlloc);
   }


void SAllocTree::RemoveAllocationSet( AllocationSet *pSet )
   {  
   HTREEITEM hRoot = m_tree.GetRootItem();
   
   if (m_tree.ItemHasChildren( hRoot) )
      {
      HTREEITEM hChildItem = m_tree.GetChildItem( hRoot );

      while (hChildItem != NULL) // the children of the root are AllocationSets
         {
         NODE_INFO *pNodeInfo = (NODE_INFO*) m_tree.GetItemData( hChildItem );

         if ( pNodeInfo->pSet == pSet )
            {
            this->m_tree.m_nodeInfoArray.Remove( pNodeInfo );
            m_tree.DeleteItem( hChildItem );
            break;
            }

         hChildItem = m_tree.GetNextItem(hChildItem, TVGN_NEXT);
         }
      }

   m_pSAllocWnd->Activate( NULL, false );
   m_tree.SelectItem( hRoot );
   // still need to select new set
   }


void SAllocTree::RemoveAllocation( Allocation *pAlloc )
   {
   HTREEITEM hSet = m_tree.GetRootItem();   // first allocation set

   // iterate through allocation sets, then allocations
   bool found = false;
   while ( hSet != NULL )
      {
      // iterate through this set's children (allocations)
      if ( m_tree.ItemHasChildren( hSet ) )
         {
         HTREEITEM hAlloc = m_tree.GetChildItem( hSet );

         while (hAlloc != NULL)
            {
            NODE_INFO *pNodeInfo = (NODE_INFO*) m_tree.GetItemData( hAlloc );

            if ( pNodeInfo->pAlloc == pAlloc )
               {
               this->m_tree.m_nodeInfoArray.Remove( pNodeInfo );
               m_tree.DeleteItem( hAlloc );
               found = true;
               break;
               }

            hAlloc = m_tree.GetNextItem(hAlloc, TVGN_NEXT);
            }
         }  // end of: if ( m_tree.ItemHasChildren( hSetItem ) ) 

      if ( found )
         break;

      hSet = m_tree.GetNextItem(hSet, TVGN_NEXT);
      }  // end of: while ( hSetItem != NULL )
   
   m_pSAllocWnd->Activate( NULL );
   m_tree.SelectItem( hSet );
   // still need to select new allocation
   }


NODE_INFO *SAllocTree::FindNodeInfo( Allocation *pAlloc )
   {
   for ( int i=0; i < (int) this->m_tree.m_nodeInfoArray.GetSize(); i++ )
      {
      if ( m_tree.m_nodeInfoArray[ i ]->pAlloc == pAlloc )
         return m_tree.m_nodeInfoArray[ i ];
      }
   return NULL;
   }

NODE_INFO *SAllocTree::FindNodeInfo( AllocationSet *pSet )
   {
   for ( int i=0; i < (int) this->m_tree.m_nodeInfoArray.GetSize(); i++ )
      {
      if ( m_tree.m_nodeInfoArray[ i ]->pSet == pSet && m_tree.m_nodeInfoArray[ i ]->isResult == false )
         return m_tree.m_nodeInfoArray[ i ];
      }
   return NULL;
   }


void SAllocTree::RefreshItemLabel( Allocation *pAlloc )
   {
   NODE_INFO *pInfo = FindNodeInfo( pAlloc );

   if ( pInfo != NULL )
      m_tree.SetItemText( pInfo->hItem, pAlloc->m_name );
   }


void SAllocTree::RefreshItemLabel( AllocationSet *pSet )
   {
   NODE_INFO *pInfo = FindNodeInfo( pSet );

   if ( pInfo != NULL )
      m_tree.SetItemText( pInfo->hItem, pSet->m_name );
   }