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
// PPOutputs.cpp : implementation file
//

#include "stdafx.h"
#include "Flow.h"
#include "PPOutputs.h"
#include <afxdialogex.h>


#include <NameDlg.h>



bool OutputTreeCtrl::CanDragItem(TVITEM & item) 
   {
   return true;
   }


bool OutputTreeCtrl::CanDropItem(HTREEITEM hDrag, HTREEITEM hDrop, EDropHint hint)
   {
   // basic rules 
   //  1) You can drop a non-submenu MAP_FIELD_INFO anywhere
   //  2) You can only drop a submenu item at the root level

   MAP_FIELD_INFO *pDragInfo = (MAP_FIELD_INFO*) this->GetItemData( hDrag );
   MAP_FIELD_INFO *pDropInfo = (MAP_FIELD_INFO*) this->GetItemData( hDrop );

   CString msg;
   CString _hint;

   switch( hint )
      {
      case DROP_BELOW:   _hint=_T("Below");   break;
      case DROP_ABOVE:   _hint=_T("Above");   break;
      case DROP_CHILD:   _hint=_T("Child");   break;
      case DROP_NODROP:  _hint=_T("No Drop");   break;
      }
   msg.Format( _T("drop target: %s, hint=%s"), (LPCTSTR) pDropInfo->label, (LPCTSTR) _hint );
   //pPPOutputs->SetDlgItemText( IDC_TYPE, msg );

   switch( hint )
      {
      case DROP_CHILD:   // only allow drop child events on submenu
         return pDropInfo->IsSubmenu() ? true : false;

      case DROP_BELOW:   // these are always ok
      case DROP_ABOVE:
         return true;
         
      case DROP_NODROP:  _hint=_T("No Drop");   break;
         return false;
      }
       
   return false;
   }


void OutputTreeCtrl::DragMoveItem(HTREEITEM hDrag, HTREEITEM hDrop, EDropHint hint, bool bCopy )
   {
   // let the parent control handle the move
   CEditTreeCtrl::DragMoveItem( hDrag, hDrop, hint, false );     // never copy, always move
   
   HTREEITEM hNew = this->GetSelectedItem();

   // fix up the pointers for the moved items.
   MAP_FIELD_INFO *pNewInfo = (MAP_FIELD_INFO*) this->GetItemData( hNew ); 

   // the drag item need to have it's parent pointer fixed up
   HTREEITEM hParent = this->GetParentItem( hNew );
   if ( hParent == NULL )
      pNewInfo->SetParent( NULL );
   else
      {
      MAP_FIELD_INFO *pParent = (MAP_FIELD_INFO*) this->GetItemData( hParent );
      pNewInfo->SetParent( pParent );
      }
   }


void OutputTreeCtrl::DisplayContextMenu(CPoint & point)
   {
//   CPoint pt(point);
//   ScreenToClient(&pt);
//   UINT flags;
//   HTREEITEM hItem = HitTest(pt, &flags);
//   bool bOnItem = (flags & TVHT_ONITEM) != 0;
//
//   CMenu menu;
//   VERIFY(menu.CreatePopupMenu());
//
//   VERIFY(menu.AppendMenu( MF_STRING, ID_ADD_SUBMENU, _T("New Submenu\tShift+INS")));
//
//   // maybe the menu is empty...
//   if(menu.GetMenuItemCount() > 0)
//      menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);
   }



///////////////////////////////////////////////////////////////////////////////////////
// PPOutputs dialog
///////////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(PPOutputs, CPropertyPage)

PPOutputs::PPOutputs( FlowModel *pFlow )
	: CPropertyPage(PPOutputs::IDD)
   , m_pFlow( pFlow )
   , m_pOutput( NULL )
   , m_name(_T(""))
   , m_inUse(FALSE)
   , m_value(_T(""))
   , m_query(_T(""))
   {

}

PPOutputs::~PPOutputs()
{
}

void PPOutputs::DoDataExchange(CDataExchange* pDX)
{
CPropertyPage::DoDataExchange(pDX);
DDX_Text(pDX, IDC_OUTPUTNAME, m_name);
DDX_Check(pDX, IDC_OUTPUT_INUSE, m_inUse);
DDX_Control(pDX, IDC_VARNAMES, m_varNames);
DDX_Text(pDX, IDC_OUTPUT_VALUE, m_value);
DDX_Text(pDX, IDC_OUTPUT_QUERY, m_query);
DDX_Control(pDX, IDC_OUTPUT_TREE, m_outputTreeCtrl);
}


BEGIN_MESSAGE_MAP(PPOutputs, CPropertyPage)
   ON_BN_CLICKED(IDC_OUTPUT_INUSE, &PPOutputs::OnBnClickedOutputInuse)
   ON_BN_CLICKED(IDC_OUTPUT_AWM, &PPOutputs::OnBnClickedOutputAwm)
   ON_BN_CLICKED(IDC_OUTPUT_SUM, &PPOutputs::OnBnClickedOutputSum)
   ON_BN_CLICKED(IDC_OUTPUT_PCTAREA, &PPOutputs::OnBnClickedOutputPctarea)
   ON_NOTIFY(TVN_SELCHANGED, IDC_OUTPUT_TREE, &PPOutputs::OnTvnSelchangedTree)
   ON_NOTIFY(TVN_KEYDOWN, IDC_OUTPUT_TREE, &PPOutputs::OnTvnKeydownTree)

END_MESSAGE_MAP()



BOOL PPOutputs::OnInitDialog()
   {
   CPropertyPage::OnInitDialog();

   Initialize();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


BOOL PPOutputs::Initialize()
   {
   // make a local copy of the ModelOutputs
   m_modelOutputGroupArray.RemoveAll();
   
   for ( int i=0; i < m_pFlow->GetModelOutputGroupCount(); i++ )
      m_modelOutputGroupArray.Add( new ModelOutputGroup( *(m_pFlow->GetModelOutputGroup( i )) ) );

   LoadOutputs();
   LoadOutput();

   m_isOutputDirty = false;
   m_isArrayDirty = false;

   return TRUE;
   }


void PPOutputs::LoadOutputs( void )
   {
   m_outputTreeCtrl.DeleteAllItems();
 
   HTREEITEM hRoot  = m_outputTreeCtrl.GetRootItem();   // root
   HTREEITEM hFirst = NULL;

   for ( int i=0; i < (int) m_modelOutputGroupArray.GetSize(); i++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ i ];
      ASSERT( pGroup != NULL );

      // this gets inserted at the root level
      HTREEITEM hNewItem = m_outputTreeCtrl.InsertItem( pGroup->m_name );
      m_outputTreeCtrl.SetItemData( hNewItem, (DWORD_PTR) pGroup );

      if ( i == 0 )
         hFirst = hNewItem;

      // add individal outputs
      for ( int j=0; j < (int) pGroup->GetSize(); j++ )
         {
         ModelOutput *pOutput = pGroup->GetAt( j );

         HTREEITEM hOutput = m_outputTreeCtrl.InsertItem( pOutput->m_name );
         m_outputTreeCtrl.SetItemData( hOutput, (DWORD_PTR) pOutput );
         }
      }

   if ( hFirst )
      m_outputTreeCtrl.SelectItem( hFirst ); 
   }



void PPOutputs::LoadOutput()
   {
   m_pOutput = NULL;
   m_pGroup  = NULL;

   HTREEITEM hSelectedItem = m_outputTreeCtrl.GetSelectedItem();

   if ( hSelectedItem == NULL )
      return;

   // is this a group?
   if ( m_outputTreeCtrl.GetParentItem( hSelectedItem ) == m_outputTreeCtrl.GetRootItem() )
      {
      m_pGroup = (ModelOutputGroup*) m_outputTreeCtrl.GetItemData( hSelectedItem );
      m_name = m_pGroup->m_name;
      }

   else  // it is a ModelOutput
      {
      m_pOutput = (ModelOutput*) m_outputTreeCtrl.GetItemData( hSelectedItem );

      ASSERT( m_pOutput != NULL );
      m_name  = m_pOutput->m_name;
      m_inUse = (BOOL) m_pOutput->m_inUse;

      // domain
      switch( m_pOutput->m_modelDomain )
         {
         case MOD_HRU:
            CheckDlgButton( IDC_DOMAIN_HRUS, 1 );
            m_varNames.SetWindowText( _T("hruVolMelt, hruVolSwe, hruPrecip, hruRainfall, hruSnowfall, hruAirTemp, hruET") );
            break;

         case MOD_HRULAYER:    
            CheckDlgButton( IDC_DOMAIN_HRULAYERS, 1);
            m_varNames.SetWindowText( "" );
            break;

         case MOD_REACH:
         default:
            m_varNames.SetWindowText( _T("reachOutflow") );
            CheckDlgButton( IDC_DOMAIN_REACHES, 1 );     
            break;
         }

      m_value = m_pOutput->m_exprStr;
      
      switch( m_pOutput->m_modelType )
         {
         case MOT_AREAWTMEAN: CheckDlgButton( IDC_OUTPUT_AWM, 1 );     break;
         case MOT_PCTAREA:    CheckDlgButton( IDC_OUTPUT_PCTAREA, 1);  break;
         case MOT_SUM:
         default:             CheckDlgButton( IDC_OUTPUT_SUM, 1 );     break;
         }

      m_query = m_pOutput->m_queryStr;
      }

   UpdateData( 0 );
   EnableControls();
   return;
   }


void PPOutputs::EnableControls()
   {
   bool isGroup = m_pGroup != NULL;
   int  show    = isGroup ? 0 : 1;
   
   GetDlgItem( IDC_OUTPUT_INUSE )->ShowWindow( show );
   GetDlgItem( IDC_STATIC_DOMAIN )->ShowWindow( show );
   GetDlgItem( IDC_DOMAIN_HRUS )->ShowWindow( show );
   GetDlgItem( IDC_DOMAIN_HRULAYERS )->ShowWindow( show );
   GetDlgItem( IDC_DOMAIN_REACHES )->ShowWindow( show );
   GetDlgItem( IDC_VARNAMES )->ShowWindow( show );
   GetDlgItem( IDC_STATIC_VALUE )->ShowWindow( show );
   GetDlgItem( IDC_OUTPUT_VALUE )->ShowWindow( show );
   GetDlgItem( IDC_STATIC_TYPE )->ShowWindow( show );
   GetDlgItem( IDC_OUTPUT_SUM )->ShowWindow( show );
   GetDlgItem( IDC_OUTPUT_AWM )->ShowWindow( show );
   GetDlgItem( IDC_OUTPUT_PCTAREA )->ShowWindow( show );
   GetDlgItem( IDC_STATIC_QUERY )->ShowWindow( show );
   GetDlgItem( IDC_OUTPUT_QUERY )->ShowWindow( show );  
   
   GetDlgItem( IDC_OUTPUT_INUSE )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_STATIC_DOMAIN )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_DOMAIN_HRUS )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_DOMAIN_HRULAYERS )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_DOMAIN_REACHES )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_VARNAMES )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_STATIC_VALUE )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_OUTPUT_VALUE )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_STATIC_TYPE )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_OUTPUT_SUM )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_OUTPUT_AWM )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_OUTPUT_PCTAREA )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_STATIC_QUERY )->EnableWindow( isGroup ? 0 : 1 );
   GetDlgItem( IDC_OUTPUT_QUERY )->EnableWindow( isGroup ? 0 : 1 );  
   }


void PPOutputs::OnBnClickedSave()
   {
   /*
   CString filename;
   m_files.GetLBText( m_layerIndex, filename );

   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( FALSE, _T("xml"), filename, OFN_OVERWRITEPROMPT, _T("XML Files|*.xml|All files|*.*||"));

   if ( dlg.DoModal() == IDOK )
      {
      m_path = dlg.GetPathName();

      CWaitCursor c;

      // make sure order matches what is in the tree
      FieldInfoArray temp;

      // iterate through tree
      HTREEITEM hItem = m_outputTreeCtrl.GetRootItem();

      // process root-level items.  
      while ( hItem != NULL )
         {
         MAP_FIELD_INFO *pInfo = (MAP_FIELD_INFO*) m_outputTreeCtrl.GetItemData( hItem );
         pInfo = new MAP_FIELD_INFO( *pInfo );
         pInfo->SetParent( NULL );
         temp.Add( pInfo );

         // any children to process?
         if ( m_outputTreeCtrl.ItemHasChildren( hItem ) )
            {
            HTREEITEM hChildItem = m_outputTreeCtrl.GetChildItem( hItem );

            while (hChildItem != NULL)
               {
               MAP_FIELD_INFO *pChildInfo = (MAP_FIELD_INFO*) m_outputTreeCtrl.GetItemData( hChildItem );
               pChildInfo = new MAP_FIELD_INFO( *pChildInfo );
               pChildInfo->SetParent( pInfo );
               temp.Add( pChildInfo );

               hChildItem = m_outputTreeCtrl.GetNextItem( hChildItem, TVGN_NEXT);
               }
            }  // end processing children nodes
            
         hItem = m_outputTreeCtrl.GetNextSiblingItem( hItem );
         }
      
      // sync everything up
      //m_outcomeArray.Copy( temp );
      m_pLayer->m_pFieldInfoArray->Copy( temp ); //m_outcomeArray );
      
      gpDoc->SaveFieldInfoXML( m_pLayer, m_path );

      //LoadFieldTreeCtrl();    // reload trss, since ptrs are changed.

      m_isInfoDirty = m_isArrayDirty = false;
      }

   _chdir( cwd );
   free( cwd );
   */
   }





void PPOutputs::OnBnClickedAddOutput()
   {
/*
   int added = 0;
   int count = m_fields.GetCount();

   for ( int i=0; i < count; i++ )
      {
      if ( m_fields.GetCheck( i ) > 0 )
         {
         MFI_TYPE mfiType = MFIT_CATEGORYBINS;

         int col = (int) m_fields.GetItemData( i );
         TYPE type = m_pLayer->m_pDbTable->GetFieldType( col );

         CString fieldname;
         m_fields.GetText( i, fieldname );

         switch( type )
            {
            case TYPE_FLOAT:
            case TYPE_DOUBLE:
               mfiType = MFIT_QUANTITYBINS;
               break;
            }

         m_outcomeArray.AddFieldInfo( fieldname, fieldname, _T(""), _T(""), type, mfiType, 0, 0, -1.0f, -1.0f, true );
         added++;
         }
      }

   if ( added > 0 )
      {
      HTREEITEM hItem = m_outputTreeCtrl.GetSelectedItem();
      LoadOutcomes();
      LoadFieldTreeCtrl();

      m_outputTreeCtrl.SelectItem( hItem );
      MakeDirty();
      }
   */
   }


void PPOutputs::OnBnClickedRemoveOutput()
   {
   /*
   HTREEITEM hItem = m_outputTreeCtrl.GetSelectedItem();

   if ( hItem )
      {
      MAP_FIELD_INFO *pInfo = (MAP_FIELD_INFO*) m_outputTreeCtrl.GetItemData( hItem );

      ASSERT( pInfo != NULL );
      int index = -1;
      m_outcomeArray.FindFieldInfo( pInfo->fieldname, &index );
      m_outcomeArray.RemoveAt( index );

      m_outputTreeCtrl.DeleteItem( hItem );
      LoadOutcomes();
      LoadOutcome();
      MakeDirty();
      }
      */
   }


void PPOutputs::OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult)
   {
   //LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
 
   LoadOutput();

   *pResult = 0;
   }



void PPOutputs::OnTvnKeydownTree(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
   *pResult = 0;
   }


void PPOutputs::OnBnClickedOutputInuse()
   {
   // TODO: Add your control notification handler code here
   }


void PPOutputs::OnBnClickedOutputAwm()
   {
   // TODO: Add your control notification handler code here
   }


void PPOutputs::OnBnClickedOutputSum()
   {
   // TODO: Add your control notification handler code here
   }



void PPOutputs::OnBnClickedOutputPctarea()
   {
   // TODO: Add your control notification handler code here
   }

void PPOutputs::OnOK()
   {
   }
