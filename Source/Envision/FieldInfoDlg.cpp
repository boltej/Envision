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
// FieldInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "FieldInfoDlg.h"
#include "EnvDoc.h"
#include <EnvLoader.h>

#include <map.h>
#include <maplayer.h>
#include <path.h>

#include <NameDlg.h>



extern MapLayer *gpCellLayer;
extern CEnvDoc  *gpDoc;

FieldInfoDlg *pFieldInfoDlg;


IMPLEMENT_DYNCREATE(ColorGridCell, CGridCell)

void ColorGridCell::OnDblClick( CPoint pt)
   {
   int row = (int) m_lParam;

   FIELD_ATTR &a = pFieldInfoDlg->m_pInfo->GetAttribute( row-1 );
   
   CColorDialog dlg;
   dlg.m_cc.rgbResult = a.color;
   dlg.m_cc.Flags |= CC_RGBINIT;

   if ( dlg.DoModal() != IDOK )
      return;

   a.color = dlg.m_cc.rgbResult;
   pFieldInfoDlg->m_grid.SetItemBkColour( row, 0, a.color );
   pFieldInfoDlg->m_grid.RedrawCell( row, 0 );
   pFieldInfoDlg->MakeDirty();

   return;
   }


bool FieldInfoTreeCtrl::CanDragItem(TVITEM & item) 
   {
   return true;
   }


bool FieldInfoTreeCtrl::CanDropItem(HTREEITEM hDrag, HTREEITEM hDrop, EDropHint hint)
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
      case DROP_BELOW:   _hint="Below";   break;
      case DROP_ABOVE:   _hint="Above";   break;
      case DROP_CHILD:   _hint="Child";   break;
      case DROP_NODROP:  _hint="No Drop";   break;
      }
   msg.Format( "drop target: %s, hint=%s", pDropInfo->label, _hint );
   pFieldInfoDlg->SetDlgItemText( IDC_TYPE, msg );

   switch( hint )
      {
      case DROP_CHILD:   // only allow drop child events on submenu
         return pDropInfo->IsSubmenu() ? true : false;

      case DROP_BELOW:   // these are always ok
      case DROP_ABOVE:
         return true;
         
      case DROP_NODROP:  _hint="No Drop";   break;
         return false;
      }
       
   return false;
   }


void FieldInfoTreeCtrl::DragMoveItem(HTREEITEM hDrag, HTREEITEM hDrop, EDropHint hint, bool bCopy )
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


void FieldInfoTreeCtrl::DisplayContextMenu(CPoint & point)
   {
   CPoint pt(point);
   ScreenToClient(&pt);
   UINT flags;
   HTREEITEM hItem = HitTest(pt, &flags);
   bool bOnItem = (flags & TVHT_ONITEM) != 0;

   CMenu menu;
   VERIFY(menu.CreatePopupMenu());

   VERIFY(menu.AppendMenu( MF_STRING, ID_ADD_SUBMENU, _T("New Submenu\tShift+INS")));

   // maybe the menu is empty...
   if(menu.GetMenuItemCount() > 0)
      menu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);
   }




// FieldInfoDlg dialog

IMPLEMENT_DYNAMIC(FieldInfoDlg, CDialog)

FieldInfoDlg::FieldInfoDlg(CWnd* pParent /*=NULL*/)
   : CDialog(FieldInfoDlg::IDD, pParent)
   , m_fieldInfoArray()
   , m_label(_T(""))
   , m_description( _T(""))
   , m_source( _T(""))
   , m_count(0)
   , m_results(FALSE)
   , m_decadalMaps(FALSE)
   , m_siteAttr(FALSE)
   , m_outcomes(FALSE)
   , m_yearInterval(-1)
   , m_minVal(0)
   , m_maxVal(0)
   , m_pInfo( NULL )
   , m_pAttr( NULL )
   , m_initialized( false )
   , m_isInfoDirty( false )
   , m_isArrayDirty( false )
   {
   pFieldInfoDlg = this;
   }


FieldInfoDlg::~FieldInfoDlg()
{
pFieldInfoDlg = NULL;
}


void FieldInfoDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Text(pDX, IDC_LABEL, m_label);
DDX_Text(pDX, IDC_DESC, m_description);
DDX_Text(pDX, IDC_SOURCE, m_source);
DDX_Text(pDX, IDC_COUNT, m_count);
DDX_Check(pDX, IDC_RESULTS, m_results);
DDX_Check(pDX, IDC_DECADALMAPS, m_decadalMaps);
DDX_Check(pDX, IDC_SITEATTR, m_siteAttr);
DDX_Check(pDX, IDC_OUTCOMES, m_outcomes);
DDX_Check(pDX, IDC_EXPORT, m_save);
DDX_Text(pDX, IDC_YEARINTERVAL, m_yearInterval);
DDX_Text(pDX, IDC_MIN, m_minVal);
DDX_Text(pDX, IDC_MAX, m_maxVal);
DDX_Control(pDX, IDC_COLORS, m_colors);
DDX_Control(pDX, IDC_FILES, m_files);
DDX_Control(pDX, IDC_FIELDTREE, m_fieldTreeCtrl);
DDX_Control(pDX, IDC_SUBMENU, m_submenuItems);
   }


BEGIN_MESSAGE_MAP(FieldInfoDlg, CDialog)
   ON_BN_CLICKED(IDC_SAVE, &FieldInfoDlg::OnBnClickedSave)
   ON_BN_CLICKED(IDC_LOAD, &FieldInfoDlg::OnBnClickedLoad)
   //ON_LBN_SELCHANGE(IDC_FIELDS, &FieldInfoDlg::OnLbnSelchangeFields)
   ON_EN_CHANGE(IDC_LABEL, &FieldInfoDlg::OnEnChangeLabel)
   ON_EN_CHANGE(IDC_DESC, &FieldInfoDlg::OnEnChangeDescription)
   ON_EN_CHANGE(IDC_SOURCE, &FieldInfoDlg::OnEnChangeSource)
   ON_CBN_SELCHANGE(IDC_COLORS, &FieldInfoDlg::OnCbnSelchangeColors)
   ON_EN_CHANGE(IDC_COUNT, &FieldInfoDlg::OnEnChangeCount)
   ON_EN_CHANGE(IDC_MIN, &FieldInfoDlg::OnEnChangeMin)
   ON_EN_CHANGE(IDC_MAX, &FieldInfoDlg::OnEnChangeMax)
   ON_BN_CLICKED(IDC_SITEATTR, &FieldInfoDlg::OnBnClickedSiteattr)
   ON_BN_CLICKED(IDC_OUTCOMES, &FieldInfoDlg::OnBnClickedOutcomes)
   ON_BN_CLICKED(IDC_RESULTS, &FieldInfoDlg::OnBnClickedResults)
   ON_BN_CLICKED(IDC_EXPORT, &FieldInfoDlg::OnBnClickedExport)
   ON_BN_CLICKED(IDC_DECADALMAPS, &FieldInfoDlg::OnBnClickedDecadalmaps)
   ON_EN_CHANGE(IDC_YEARINTERVAL, &FieldInfoDlg::OnBnClickedDecadalmaps)
   ON_BN_CLICKED(IDC_ADD, &FieldInfoDlg::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_ADDALL, &FieldInfoDlg::OnBnClickedAddall)
   ON_BN_CLICKED(IDC_REMOVE, &FieldInfoDlg::OnBnClickedRemove)
   ON_CLBN_CHKCHANGE(IDC_FIELDS, &FieldInfoDlg::OnLbnCheckFields)
   ON_NOTIFY(GVN_ENDLABELEDIT, 100, &FieldInfoDlg::OnGridEndEdit)
   ON_CBN_SELCHANGE(IDC_FILES, &FieldInfoDlg::OnCbnSelchangeFiles)
   ON_BN_CLICKED(IDC_REMOVEALL, &FieldInfoDlg::OnBnClickedRemoveall)
   ON_BN_CLICKED(IDC_MOVEUP, &FieldInfoDlg::OnBnClickedMoveup)
   ON_BN_CLICKED(IDC_MOVEDOWN, &FieldInfoDlg::OnBnClickedMovedown)
   ON_BN_CLICKED(IDC_USEQUANTITYBINS, &FieldInfoDlg::OnBnClickedUsequantitybins)
   ON_BN_CLICKED(IDC_USECATEGORYBINS, &FieldInfoDlg::OnBnClickedUsecategorybins)
   ON_CBN_SELCHANGE(IDC_SUBMENU, &FieldInfoDlg::OnCbnSelchangeSubmenu)
   ON_BN_CLICKED(IDC_ADDSUBMENU, &FieldInfoDlg::OnBnClickedAddsubmenu)
   ON_BN_CLICKED(IDC_ADDFIELDINFO, &FieldInfoDlg::OnBnClickedAddfieldinfo)
   ON_BN_CLICKED(IDC_REMOVEFIELDINFO, &FieldInfoDlg::OnBnClickedRemovefieldinfo)
   ON_NOTIFY(TVN_SELCHANGED, IDC_FIELDTREE, &FieldInfoDlg::OnTvnSelchangedFieldtree)
   ON_LBN_DBLCLK(IDC_FIELDS, &FieldInfoDlg::OnLbnDblclkFields)
   ON_NOTIFY(TVN_KEYDOWN, IDC_FIELDTREE, &FieldInfoDlg::OnTvnKeydownFieldtree)
   ON_COMMAND( ID_ADD_SUBMENU, &FieldInfoDlg::OnBnClickedAddsubmenu )
END_MESSAGE_MAP()


BOOL FieldInfoDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();
   m_initialized = true;

   CWnd *pAttrWnd = GetDlgItem( IDC_ATTRIBUTES );
   ASSERT( pAttrWnd != NULL );
   
   RECT rect;
   pAttrWnd->GetWindowRect( &rect );
   ScreenToClient( &rect );
   pAttrWnd->ShowWindow( SW_HIDE );

   BOOL ok = m_grid.Create( rect, this, 100, WS_CHILD | WS_TABSTOP | WS_VISIBLE );
   m_grid.m_pParent = this;
   ASSERT( ok );

   Map *pMap = gpCellLayer->m_pMap;

   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );

      nsPath::CPath path( pLayer->m_pFieldInfoArray->m_path );

      if ( path.GetStr().IsEmpty() )  // no file loaded?
         path = pLayer->m_path;

      if ( path.GetExtension().CompareNoCase( "xml" ) != 0 )
         path.RenameExtension( "xml" );

      m_files.AddString( path );
      }
   
   if ( m_files.GetCount() > 0 )
      {
      m_files.SetCurSel( 0 );
      m_layerIndex = 0;
      m_pLayer = pMap->GetLayer( 0 );
      }
   else
      {
      m_files.SetCurSel( -1 );
      m_layerIndex = -1;
      m_pLayer = NULL;
      }

    // make image lit for control
    //m_imageList.Create( IDB_FIELDINFOEDITOR, 16, 0, 1);
    //CBitmap bm;
    //bm.LoadBitmap(IDB_BITMAP1);
    //m_Images.Add(&bm, RGB(255, 255, 255));
    //bm.DeleteObject();
    //bm.LoadBitmap(IDB_BITMAP2);
    //m_Images.Add(&bm, RGB(255, 255, 255));
    //
    //CTreeCtrl& ctrl = GetTreeCtrl();

    //m_fieldTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);
    //m_fieldTreeCtrl.SetImageList(&m_imageList, TVSIL_STATE);
    
   Initialize();
   return TRUE;
   }


BOOL FieldInfoDlg::Initialize()
   {
   if ( m_layerIndex < 0 )
      return FALSE;

   //m_fieldTreeCtrl.EnableMultiSelect( false );

   m_pLayer = gpCellLayer->m_pMap->GetLayer( m_layerIndex );
   m_fieldInfoArray.Copy( *m_pLayer->GetFieldInfoArray() );
  
   LoadFields();
   LoadFieldTreeCtrl();
   LoadSubmenuCombo();

   LoadFieldInfo();

   m_isInfoDirty = false;
   m_isArrayDirty = false;

   return TRUE;
   }


void FieldInfoDlg::OnCbnSelchangeFiles()
   {
   if ( m_isArrayDirty )
      {
      int retVal = MessageBox( "You have made changes to the field information.  Do you want to save these changes?", "Save Changes", MB_YESNOCANCEL );

      switch ( retVal )
         {
         case IDYES:
            OnBnClickedSave();
            break;

         case IDNO:
            break;

         default:
            return;
         }
      }

   m_layerIndex = m_files.GetCurSel();
   m_pLayer = gpCellLayer->m_pMap->GetLayer( m_layerIndex );

   Initialize();
   }


void FieldInfoDlg::EnableControls( )
   {
   // enable/disable/clear current info 
   bool isField   = ( m_pInfo && ! m_pInfo->IsSubmenu() ) ? true : false;
   bool isSubmenu = ( m_pInfo &&   m_pInfo->IsSubmenu() ) ? true : false;
   
   BOOL enable = isField ? 1 : 0;

   GetDlgItem( IDC_LABEL   )->EnableWindow( ( m_pInfo != NULL ) ? 1 : 0 );
   GetDlgItem( IDC_DESC    )->EnableWindow( enable );
   GetDlgItem( IDC_SOURCE  )->EnableWindow( enable );
   GetDlgItem( IDC_SUBMENU )->EnableWindow( enable );

   GetDlgItem( IDC_ATTRIBUTES )->EnableWindow( enable );
   GetDlgItem( IDC_COUNT )->EnableWindow( enable );
   GetDlgItem( IDC_RESULTS )->EnableWindow( enable );
   GetDlgItem( IDC_DECADALMAPS )->EnableWindow( enable );
   GetDlgItem( IDC_SITEATTR )->EnableWindow( enable );
   GetDlgItem( IDC_OUTCOMES )->EnableWindow( enable );
   GetDlgItem( IDC_EXPORT )->EnableWindow( enable );
   GetDlgItem( IDC_YEARINTERVAL )->EnableWindow( enable );
   GetDlgItem( IDC_MIN )->EnableWindow( enable );
   GetDlgItem( IDC_MAX )->EnableWindow( enable );
   GetDlgItem( IDC_ADD )->EnableWindow( enable );
   GetDlgItem( IDC_ADDALL )->EnableWindow( enable );
   GetDlgItem( IDC_REMOVE )->EnableWindow( enable );
   GetDlgItem( IDC_COLORS )->EnableWindow( enable );
      
   m_grid.EnableWindow( enable );

   if ( m_pInfo != NULL )
      {
      int colIndex = m_pLayer->GetFieldCol( m_pInfo->fieldname );
      if ( colIndex >= 0 )
         {
         TYPE type = m_pLayer->GetFieldType( colIndex );
         if ( IsString( type ) )
            {
            GetDlgItem( IDC_MIN )->EnableWindow( FALSE );
            GetDlgItem( IDC_MAX )->EnableWindow( FALSE );
            }

         //if ( type == TYPE_FLOAT || type == TYPE_DOUBLE )
         //   GetDlgItem( IDC_ADDALL )->EnableWindow( FALSE );
         }
      }
   }


bool FieldInfoDlg::LoadFieldInfo()
   {
   m_grid.DeleteAllItems();

   m_label = _T("");
   m_description = _T("");
   m_source = _T("");
   m_count = 0;
   m_results = FALSE;
   m_decadalMaps = FALSE;
   m_siteAttr = FALSE;
   m_outcomes = FALSE;
   m_yearInterval = 0;
   m_minVal = 0;
   m_maxVal= 0;
   m_save = TRUE;
   m_pInfo = NULL;

   m_colors.SetCurSel( 0 );
   m_submenuItems.SetCurSel( 0 );

   UpdateData( 0 );
   SetDlgItemText( IDC_TYPE, "" );

   HTREEITEM hSelectedItem = m_fieldTreeCtrl.GetSelectedItem();

   if ( hSelectedItem == NULL )
      return false;

   m_pInfo = (MAP_FIELD_INFO*) m_fieldTreeCtrl.GetItemData( hSelectedItem );
   ASSERT( m_pInfo != NULL );
   
   m_label = m_pInfo->label;
   m_description = m_pInfo->description;
   m_source = m_pInfo->source;
   m_count = m_pInfo->GetAttributeCount();

   int col = m_pLayer->GetFieldCol( m_pInfo->fieldname );
   CString typeStr( "Unable to find column" );
   if ( col >= 0 )
      {
      TYPE type = m_pLayer->GetFieldType( col );
      typeStr = "Data Type: ";
      typeStr += GetTypeLabel( type );

      if ( ::IsNumeric( type ) )
         {
         float minVal, maxVal;
         m_pLayer->m_pData->GetMinMax( col, &minVal, &maxVal );

         CString valStr;
         valStr.Format( "     Range: %g - %g", minVal, maxVal );
         typeStr += valStr;
         }
      }

   GetDlgItem( IDC_TYPE )->SetWindowText( typeStr );

   m_colors.SetCurSel( m_pInfo->displayFlags );

   m_decadalMaps = m_pInfo->GetExtraHigh() > 0 ? TRUE : FALSE;

   m_results  = m_pInfo->GetExtraBool( 1 );
   m_siteAttr = m_pInfo->GetExtraBool( 2 );
   m_outcomes = m_pInfo->GetExtraBool( 4 );
   m_save     = m_pInfo->save ? 1 : 0;

   m_yearInterval = m_pInfo->GetExtraHigh();
   m_minVal = m_pInfo->binMin;
   m_maxVal = m_pInfo->binMax;

   m_submenuItems.SetCurSel( 0 );

   if ( m_pInfo->pParent != NULL )
      {
      int index = m_submenuItems.FindString( 0, m_pInfo->pParent->label );
      m_submenuItems.SetCurSel( index );
      }

   // load attributes
   LoadAttributes();
   EnableControls();

   UpdateData( 0 );

   return true;
   }


void FieldInfoDlg::LoadFields( void )
   {
   m_fields.ResetContent();

   for ( int i=0; i < m_pLayer->GetColCount(); i++ )
      {
      LPCTSTR field = m_pLayer->GetFieldLabel( i );
      MAP_FIELD_INFO *pInfo = m_fieldInfoArray.FindFieldInfo( field );

      if ( pInfo == NULL ) // no inf0 exists yet?
         {
         int index = m_fields.AddString( field );
         m_fields.SetItemData( index, i );
         }
      }
   
   m_fields.SetCurSel( 0 );
   }


void FieldInfoDlg::LoadFieldTreeCtrl( void )
   {
   m_fieldTreeCtrl.DeleteAllItems();

   for ( int i=0; i < (int) m_fieldInfoArray.GetSize(); i++ )
      {
      MAP_FIELD_INFO *pInfo = m_fieldInfoArray[ i ];
      ASSERT( pInfo != NULL );

      // top-level menu?
      if ( pInfo->pParent == NULL )
         {
         HTREEITEM hRoot = m_fieldTreeCtrl.GetRootItem();   // root
         
         // this gets inserted at the root level
         HTREEITEM hNewItem = m_fieldTreeCtrl.InsertItem( pInfo->fieldname );
         m_fieldTreeCtrl.SetItemData( hNewItem, (DWORD_PTR) pInfo );
         }
      else  // it attached to a submenu.  if so, find where to add it in tree.
         {
         // find parent
         HTREEITEM hItem = m_fieldTreeCtrl.GetRootItem();   // root
         //HTREEITEM hItem = m_fieldTreeCtrl.GetChildItem( hRoot );     // first child of the root

         // iterate through root-level children looking for this items parent
         MAP_FIELD_INFO *_pInfo = NULL;
         while ( hItem != NULL )
            {
            _pInfo =  (MAP_FIELD_INFO *) m_fieldTreeCtrl.GetItemData( hItem );

            if ( _pInfo == pInfo->pParent ) // found the right parent
               break;

            hItem = m_fieldTreeCtrl.GetNextItem( hItem, TVGN_NEXT);
            }

         if ( _pInfo == NULL )
            Report::ErrorMsg( "Missing parent menu!!!" );
         else
            {  // attach to parent
            HTREEITEM hNewItem = m_fieldTreeCtrl.InsertItem( pInfo->fieldname, hItem );
            m_fieldTreeCtrl.SetItemData( hNewItem, (DWORD_PTR) pInfo );
            }
         }  // end of:  else ( attached to a submenu )
      }

   // Note: Fields are in alphabetical order, not actual order
   //CString field;
   //for ( int i=0; i < m_fields.GetCount(); i++ )
   //   {
   //   m_fields.GetText( i, field );
   //   MAP_FIELD_INFO *pInfo = m_fieldInfoArray.FindFieldInfo( field );
   //   m_fields.SetCheck( i, pInfo ? 1 : 0 );       // is there a field info defined for this field?
   //   }

   //m_fieldTreeCtrl.Select( );
   }


void FieldInfoDlg::LoadSubmenuCombo( void )
   {
   m_submenuItems.ResetContent();

   m_submenuItems.AddString( "(none)" );
   m_submenuItems.SetItemDataPtr( 0, NULL );

   for ( int i=0; i < (int) m_fieldInfoArray.GetSize(); i++ )
      {
      MAP_FIELD_INFO *pInfo = m_fieldInfoArray[ i ];
      
      if ( pInfo->mfiType == MFIT_SUBMENU )
         {
         int index = m_submenuItems.AddString( pInfo->label );
         m_submenuItems.SetItemDataPtr( index, pInfo );
         }
      }

   m_submenuItems.SetCurSel( 0 );
   }


void FieldInfoDlg::LoadAttributes()
   {
   m_grid.DeleteAllItems();

   if ( m_pInfo == NULL )
      return;

   int rows = (int) m_pInfo->attributes.GetSize();

   // set checkbox
   switch( m_pInfo->mfiType )
      {
      case MFIT_QUANTITYBINS:
         CheckDlgButton( IDC_USEQUANTITYBINS, 1 );
         CheckDlgButton( IDC_USECATEGORYBINS, 0 );
         GetDlgItem( IDC_MIN )->EnableWindow( 1 );
         GetDlgItem( IDC_MAX )->EnableWindow( 1 );
         GetDlgItem( IDC_COUNT )->EnableWindow( 1 );

         m_grid.SetColumnCount( 4 );
         m_grid.SetFixedRowCount( 1 );

         m_grid.SetItemText( 0, 0, "Color" );
         m_grid.SetItemText( 0, 1, "Min Value" );
         m_grid.SetItemText( 0, 2, "Max Value" );
         m_grid.SetItemText( 0, 3, "Description" );
         m_grid.SetColumnWidth( 0, 14 );
         m_grid.SetColumnWidth( 1, 20 );
         m_grid.SetColumnWidth( 2, 20 );
         break;

      case MFIT_CATEGORYBINS:
         CheckDlgButton( IDC_USEQUANTITYBINS, 0 );
         CheckDlgButton( IDC_USECATEGORYBINS, 1 );
         GetDlgItem( IDC_MIN )->EnableWindow( 0 );
         GetDlgItem( IDC_MAX )->EnableWindow( 0 );
         GetDlgItem( IDC_COUNT )->EnableWindow( 0 );

         m_grid.SetColumnCount( 3 );
         m_grid.SetFixedRowCount( 1 );

         m_grid.SetItemText( 0, 0, "Color" );
         m_grid.SetItemText( 0, 1, "Value" );
         m_grid.SetItemText( 0, 2, "Description" );
         m_grid.SetColumnWidth( 0, 14 );
         m_grid.SetColumnWidth( 1, 20 );
         break;
      }

   m_grid.SetRowCount( rows + 1 ); // the " + 1" is for the fixed row
   m_grid.ExpandColumnsToFit();
   m_grid.SetEditable();
   m_grid.EnableSelection( FALSE );

   for ( int i=0; i < m_pInfo->attributes.GetSize(); i++ )
      {
      FIELD_ATTR &a = m_pInfo->GetAttribute( i );
      m_grid.SetItemBkColour( i+1, 0, a.color );

      m_grid.SetCellType(i+1, 0, RUNTIME_CLASS(ColorGridCell));
      m_grid.SetItemData(i+1, 0, i+1);

      if ( m_pInfo->mfiType == MFIT_QUANTITYBINS )
         {
         char min[ 16 ], max[ 16 ];
         sprintf_s( min, 16, "%g", a.minVal );
         sprintf_s( max, 16, "%g", a.maxVal );

         m_grid.SetItemText( i+1, 1, min );
         m_grid.SetItemText( i+1, 2, max );
         m_grid.SetItemText( i+1, 3, a.label );

         m_grid.SetItemState( i+1, 0, m_grid.GetItemState(i+1, 0) | GVIS_READONLY );
         }
      else    // category bins
         {
         LPCTSTR value = a.value.GetAsString();
         m_grid.SetItemText( i+1, 1, value );
         m_grid.SetItemText( i+1, 2, a.label );

         m_grid.SetItemState( i+1, 0, m_grid.GetItemState(i+1, 0) | GVIS_READONLY );
         //m_grid.SetItemState( i+1, 1, m_grid.GetItemState(i+1, 0) | GVIS_READONLY );
         }
      }
   }


void FieldInfoDlg::OnOK()
   {
   if ( m_isArrayDirty )
      {
      int retVal = MessageBox( "You have made changes to the field information.  Do you want to save these changes?", "Save Changes", MB_YESNOCANCEL );

      switch ( retVal )
         {
         case IDYES:
            OnBnClickedSave();
            break;

         case IDNO:
            break;

         default:
            return;
         }
      }

   CDialog::OnOK();
   }




//----------------------- M E S S A G E   H A N D L E R S ----------------------------//

void FieldInfoDlg::OnEnChangeLabel()
   {
   if ( m_initialized == false )
      return;

   UpdateData( 1 );
   m_pInfo->label = m_label;
   MakeDirty();
   }

void FieldInfoDlg::OnEnChangeDescription()
   {
   if ( m_initialized == false )
      return;

   UpdateData( 1 );
   m_pInfo->description = m_description;
   MakeDirty();
   }

void FieldInfoDlg::OnEnChangeSource()
   {
   if ( m_initialized == false )
      return;

   UpdateData( 1 );
   m_pInfo->source = m_source;
   MakeDirty();
   }


void FieldInfoDlg::OnCbnSelchangeColors()
   {
   if ( m_initialized == false )
      return;

   m_pInfo->displayFlags = m_colors.GetCurSel();
   MakeDirty();
   }


void FieldInfoDlg::OnEnChangeCount()
   {
   if ( m_initialized == false )
      return;

   //UpdateData( 1 );
   //m_pInfo->binCount = this->m_count;
   MakeDirty();
   }


void FieldInfoDlg::OnEnChangeMin()
   {
   if ( m_initialized == false )
      return;

   //UpdateData( 1 );
   m_pInfo->binMin = m_minVal;
   MakeDirty();
   }


void FieldInfoDlg::OnEnChangeMax()
   {
   if ( m_initialized == false )
      return;

   //UpdateData( 1 );
   m_pInfo->binMax = m_maxVal;
   MakeDirty();
   }

void FieldInfoDlg::OnBnClickedSiteattr()
   {
   if ( m_initialized == false )
      return;

   UpdateData( 1 );

   if ( m_siteAttr )
      m_pInfo->SetExtra( 2 );
   else
      m_pInfo->UnsetExtra( 2 );

   MakeDirty();
   }

void FieldInfoDlg::OnBnClickedOutcomes()
   {
   if ( m_initialized == false )
      return;

   UpdateData( 1 );

   if ( m_outcomes )
      m_pInfo->SetExtra( 4 );
   else
      m_pInfo->UnsetExtra( 4 );

   MakeDirty();
   }

void FieldInfoDlg::OnBnClickedResults()
   {
   if ( m_initialized == false )
      return;

   UpdateData( 1 );

   if ( m_results )
      m_pInfo->SetExtra( 1 );
   else
      m_pInfo->UnsetExtra( 1 );

   MakeDirty();
   }

void FieldInfoDlg::OnBnClickedExport()
   {
   if ( m_initialized == false )
      return;

   UpdateData( 1 );
   m_pInfo->save = m_save ? true : false;
   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedDecadalmaps()
   {
   if ( m_initialized == false )
      return;

   UpdateData( 1 );

   WORD interval = 0;
   WORD flags = m_pInfo->GetExtraLow();

   if ( m_decadalMaps )
      interval = (WORD) this->m_yearInterval;

   m_pInfo->SetExtra( MAKELONG( flags, interval ) );

   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedAdd()
   {
   if ( m_initialized == false || m_pInfo == NULL )
      return;

   m_pInfo->AddAttribute( VData( 0 ), "<Description>", RGB( 255,255,255 ), 0, 1, 0 );

   LoadAttributes();
   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedAddall() // generate
   {
   CWaitCursor c;

   if ( m_initialized == false )
      return;

   if ( m_pInfo == NULL )
      return;

   UpdateData( 1 );

   int col = m_pLayer->GetFieldCol( m_pInfo->fieldname );
   ASSERT( col >= 0 );

   TYPE type = m_pLayer->GetFieldType( col );

   if ( IsDlgButtonChecked( IDC_USEQUANTITYBINS ) )
      m_pInfo->mfiType = MFIT_QUANTITYBINS;
   else
      m_pInfo->mfiType = MFIT_CATEGORYBINS;

   m_pLayer->SetBinColorFlag( (BCFLAG) this->m_colors.GetCurSel() );

   bool oldShowBinCount = m_pLayer->m_showBinCount;
   m_pLayer->ShowBinCount( false );
   
   switch( type )
      {
      case TYPE_BOOL:      // same regardles of bin type
         m_pInfo->attributes.RemoveAll();
         m_pInfo->AddAttribute( VData( true ), "True",   RGB( 100, 100, 100 ), 0.5f, 100.0f, 0 );
         m_pInfo->AddAttribute( VData( false ), "False", RGB( 255, 255, 255 ), -100.0f, 0.499f, 0 );
         break;

      case TYPE_FLOAT:
      case TYPE_DOUBLE:
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_SHORT:
      case TYPE_LONG:
      case TYPE_ULONG:
         {
         BinArray binArray;
         binArray.m_type = type; 

         switch( m_pInfo->mfiType )
            {
            case MFIT_QUANTITYBINS:
               m_pLayer->SetBins( col, m_minVal, m_maxVal, m_count, type, BSF_LINEAR, &binArray );
               break;

            case MFIT_CATEGORYBINS:
               m_pLayer->SetUniqueBins( col, type, &binArray );
               break;

            default:
               ASSERT(0);
            }
         
         for ( int i=0; i< binArray.GetSize(); i++ )
            {
            if ( m_pLayer->GetBinColorFlag() == BCF_SINGLECOLOR )
               binArray[i].SetColor( m_pLayer->GetFillColor() );
            else
               {
               COLORREF color = GetColor( i, (int) binArray.GetSize(), m_pLayer->GetBinColorFlag() );
               binArray[i].SetColor( color );
               }
 
            FIELD_ATTR a;
            a.color = binArray[ i ].m_color;
            a.label = binArray[ i ].m_label;

            switch( m_pInfo->mfiType )
               {
               case MFIT_QUANTITYBINS:
                  a.minVal = binArray[ i ].m_minVal;
                  a.maxVal = binArray[ i ].m_maxVal;

                  if ( i == binArray.GetSize()-1 ) // last one?
                     a.maxVal *= 1000;
                  break;
            
               case MFIT_CATEGORYBINS:
                  a.value = binArray[ i ].m_maxVal;
                  a.value.ChangeType( type );
                  break;
               }

            m_pInfo->AddAttribute( a );
            }
         }
         break;

      case TYPE_STRING:
      case TYPE_DSTRING:
      default:
         {
         BinArray binArray;
         binArray.m_type = type; 

         switch( m_pInfo->mfiType )
            {
            case MFIT_QUANTITYBINS:
               ASSERT( 0 );   // shouldn't ever get here
               break;

            case MFIT_CATEGORYBINS:
               m_pLayer->SetUniqueBins( col, type, &binArray );
               break;

            default:
               ASSERT(0);
            }
         
         for ( int i=0; i < binArray.GetSize(); i++ )
            {
            if ( m_pLayer->GetBinColorFlag() == BCF_SINGLECOLOR )
               binArray[i].SetColor( m_pLayer->GetFillColor() );
            else
               {
               COLORREF color = GetColor( i, (int) binArray.GetSize(), m_pLayer->GetBinColorFlag() );
               binArray[i].SetColor( color );
               }
 
            FIELD_ATTR a;
            a.minVal = binArray[ i ].m_minVal;
            a.maxVal = binArray[ i ].m_maxVal;
            a.value.AllocateString( binArray[ i ].m_strVal );
            a.color = binArray[ i ].m_color;
            a.label = binArray[ i ].m_label;
            m_pInfo->AddAttribute( a );
            }
         }  // end of: default case
      }  // end of: switch( type )

   m_pLayer->m_showBinCount = oldShowBinCount;

   LoadAttributes();
   MakeDirty();
   }


void FieldInfoDlg::OnLbnCheckFields()
   {
   /*
   if ( m_initialized == false )
      return;

   int sel = m_fields.GetCurSel();

   CString field;
   m_fields.GetText( sel, field );

   int col = m_pLayer->GetFieldCol( field );
   ASSERT( col >= 0 );

   int checked = m_fields.GetCheck( sel );

   MAP_FIELD_INFO *pInfo = m_fieldInfoArray.FindFieldInfo( field );

   if ( checked )    // turning on an unchecked entry?
      {
      if ( pInfo )
         pInfo->save = true;
      else
         {
         MFI_TYPE mfiType = MFIT_CATEGORYBINS;
         TYPE type = m_pLayer->m_pDbTable->GetFieldType( col );

         switch( type )
            {
            case TYPE_FLOAT:
            case TYPE_DOUBLE:
               mfiType = MFIT_QUANTITYBINS;
               break;
            }

         m_fieldInfoArray.AddFieldInfo( m_pLayer->GetFieldLabel( col ), m_pLayer->GetFieldLabel( col ), _T(""), _T(""), type, mfiType, 0, 0, -1.0f, -1.0f, true );
         }

      LoadFieldInfo();
      }
   else  // unchecking an existing pInfo
      {
      ASSERT( pInfo != NULL );
      pInfo->save = false;
      //MessageBox( "Removing previously defined Field Information is not currently supported.  Please edit the XML file directly." );
      }
   
   MakeDirty();
   */
   }

// FieldInfoDlg message handlers

void FieldInfoDlg::OnBnClickedSave()
   {
   CString filename;
   m_files.GetLBText( m_layerIndex, filename );

   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( FALSE, "xml", filename, OFN_OVERWRITEPROMPT, "XML Files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_path = dlg.GetPathName();

      CWaitCursor c;

      // make sure order matches what is in the tree
      FieldInfoArray temp;

      // iterate through tree
      HTREEITEM hItem = m_fieldTreeCtrl.GetRootItem();

      // process root-level items.  
      while ( hItem != NULL )
         {
         MAP_FIELD_INFO *pInfo = (MAP_FIELD_INFO*) m_fieldTreeCtrl.GetItemData( hItem );
         pInfo = new MAP_FIELD_INFO( *pInfo );
         pInfo->SetParent( NULL );
         temp.Add( pInfo );

         // any children to process?
         if ( m_fieldTreeCtrl.ItemHasChildren( hItem ) )
            {
            HTREEITEM hChildItem = m_fieldTreeCtrl.GetChildItem( hItem );

            while (hChildItem != NULL)
               {
               MAP_FIELD_INFO *pChildInfo = (MAP_FIELD_INFO*) m_fieldTreeCtrl.GetItemData( hChildItem );
               pChildInfo = new MAP_FIELD_INFO( *pChildInfo );
               pChildInfo->SetParent( pInfo );
               temp.Add( pChildInfo );

               hChildItem = m_fieldTreeCtrl.GetNextItem( hChildItem, TVGN_NEXT);
               }
            }  // end processing children nodes
            
         hItem = m_fieldTreeCtrl.GetNextSiblingItem( hItem );
         }
      
      // sync everything up
      //m_fieldInfoArray.Copy( temp );
      m_pLayer->m_pFieldInfoArray->Copy( temp ); //m_fieldInfoArray );
      
      gpDoc->SaveFieldInfoXML( m_pLayer, m_path );

      //LoadFieldTreeCtrl();    // reload trss, since ptrs are changed.

      m_isInfoDirty = m_isArrayDirty = false;
      }

   _chdir( cwd );
   free( cwd );
   }


void FieldInfoDlg::OnBnClickedLoad()
   {
   if ( m_pLayer == NULL )
      return;

   if ( m_isArrayDirty )
      {
      int retVal = MessageBox( "You have made changes to the field information.  Do you want to save these changes?", "Save Changes", MB_YESNOCANCEL );

      switch ( retVal )
         {
         case IDYES:
            OnBnClickedSave();
            break;

         case IDNO:
            break;

         default:
            return;
         }
      }

   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( TRUE, "xml", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "XML files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_path = dlg.GetPathName();
      EnvLoader loader;
      m_pLayer->LoadFieldInfoXml( m_path, false );

      Initialize();
      }

   _chdir( cwd );
   free( cwd );
   }


//void FieldInfoDlg::OnLbnSelchangeFields()
//   {
//   LoadFieldInfo();
//   }


void FieldInfoDlg::OnGridEndEdit(NMHDR *pNotifyStruct, LRESULT* pResult)
   {
   NM_GRIDVIEW* pItem = (NM_GRIDVIEW*) pNotifyStruct;

   // we must be in a description field, so save the description in the associated attribute
   //ASSERT( pItem->iColumn == 2 || pItem->iColumn == 3 );
   FIELD_ATTR &a = m_pInfo->GetAttribute( pItem->iRow-1 );
   CString val = m_grid.GetItemText( pItem->iRow, pItem->iColumn );

   switch( m_pInfo->mfiType )
      {
      case MFIT_QUANTITYBINS:
         if ( pItem->iColumn == 1 )  // min value
            a.minVal = (float) atof( val );
         else if ( pItem->iColumn == 2 )
            a.maxVal = (float) atof( val );
         else if ( pItem->iColumn == 3 )
            a.label = val;
         break;

      case MFIT_CATEGORYBINS:
         if ( pItem->iColumn == 1 )
            a.value.SetKeepType( val );
         else if ( pItem->iColumn == 2 )
            a.label = val;
         break;
      }

   *pResult = 0;
   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedRemove()
   {
   if ( m_initialized == false )
      return;

   CCellID cell = m_grid.GetFocusCell();
   int row = cell.row;

   if ( row > 0 )
      {
      if ( MessageBox( "Delete the selected Attribute?", "Remove Attribute", MB_YESNO ) == IDYES )
         {
         m_grid.DeleteRow( row );
         m_grid.RedrawWindow();
         m_pInfo->attributes.RemoveAt( row-1 );
         }
      }
   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedRemoveall()
   {
   if ( m_initialized == false )
      return;

   if ( MessageBox( "Delete all Attributes?", "Remove Attributes", MB_YESNO ) != IDYES )
      return;

   m_pInfo->attributes.RemoveAll();

   LoadAttributes();
   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedMoveup()
   {
   if ( m_initialized == false )
      return;

   CCellID cell = m_grid.GetFocusCell();
   int row = cell.row;

   if ( row > 0 )
      {
      if ( row == 1 )
         {
         MessageBeep( 0 );
         return;
         }

      m_pInfo->SwapAttributes( row-1, row-2 );
      this->LoadAttributes();

      MakeDirty();
      }

   return;
   }


void FieldInfoDlg::OnBnClickedMovedown()
   {
   if ( m_initialized == false )
      return;

   CCellID cell = m_grid.GetFocusCell();
   int row = cell.row;
   int rows = m_grid.GetRowCount();

   if ( row > 0 )
      {
      if ( row == rows )
         {
         MessageBeep( 0 );
         return;
         }

      m_pInfo->SwapAttributes( row-1, row );
      this->LoadAttributes();

      MakeDirty();
      }

   return;
   }


void FieldInfoDlg::OnBnClickedUsequantitybins()
   {
   if ( m_pInfo )
      {
      m_pInfo->mfiType = MFIT_QUANTITYBINS;
      LoadAttributes();
      }

   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedUsecategorybins()
   {
   if ( m_pInfo )
      {
      m_pInfo->mfiType = MFIT_CATEGORYBINS;
      LoadAttributes();
      }

   MakeDirty();
   }


void FieldInfoDlg::OnCbnSelchangeSubmenu()
   {
   int index = m_submenuItems.GetCurSel();

   if ( index > 0 )
      {
      MAP_FIELD_INFO *pInfo = (MAP_FIELD_INFO*) m_submenuItems.GetItemData( index );
      m_pInfo->SetParent( pInfo );
      }
   else
      m_pInfo->SetParent( NULL );

   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedAddsubmenu()
   {
   NameDlg dlg;

   if ( dlg.DoModal() == IDOK )
      {
      MAP_FIELD_INFO *pInfo = new MAP_FIELD_INFO( dlg.m_name, dlg.m_name, "", "", TYPE_NULL, MFIT_SUBMENU, 0, 0, 0, 0, 1 );
      m_fieldInfoArray.Add( pInfo );

      //m_pInfo->SetParent( pInfo ); ////
      HTREEITEM hNewItem = m_fieldTreeCtrl.InsertItem( dlg.m_name );
      m_fieldTreeCtrl.SetItemData( hNewItem, (DWORD_PTR) pInfo );

      LoadSubmenuCombo();

      if (  m_pInfo && m_pInfo->pParent != NULL )
         {
         int index = m_submenuItems.FindString( 0, m_pInfo->pParent->label );
         m_submenuItems.SetCurSel( index );
         }
      }

   MakeDirty();
   }


void FieldInfoDlg::OnBnClickedAddfieldinfo()
   {
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

         m_fieldInfoArray.AddFieldInfo( fieldname, fieldname, _T(""), _T(""), type, mfiType, 0, 0, -1.0f, -1.0f, true );
         added++;
         }
      }

   if ( added > 0 )
      {
      HTREEITEM hItem = m_fieldTreeCtrl.GetSelectedItem();
      LoadFields();
      LoadFieldTreeCtrl();

      m_fieldTreeCtrl.SelectItem( hItem );
      MakeDirty();
      }
   }


void FieldInfoDlg::OnBnClickedRemovefieldinfo()
   {
   HTREEITEM hItem = m_fieldTreeCtrl.GetSelectedItem();

   if ( hItem )
      {
      MAP_FIELD_INFO *pInfo = (MAP_FIELD_INFO*) m_fieldTreeCtrl.GetItemData( hItem );

      ASSERT( pInfo != NULL );
      int index = -1;
      m_fieldInfoArray.FindFieldInfo( pInfo->fieldname, &index );
      m_fieldInfoArray.RemoveAt( index );

      m_fieldTreeCtrl.DeleteItem( hItem );
      LoadFields();
      LoadFieldInfo();
      MakeDirty();
      }
   }


void FieldInfoDlg::OnTvnSelchangedFieldtree(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
 
   LoadFieldInfo();

   *pResult = 0;
   }


void FieldInfoDlg::OnLbnDblclkFields()
   {
   int index = m_fields.GetCurSel();

   if ( index < 0 )
      return;

   MFI_TYPE mfiType = MFIT_CATEGORYBINS;

   int col = (int) m_fields.GetItemData( index );
   TYPE type = m_pLayer->m_pDbTable->GetFieldType( col );

   CString fieldname;
   m_fields.GetText( index, fieldname );

   switch( type )
      {
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
         mfiType = MFIT_QUANTITYBINS;
         break;
      }

   m_fieldInfoArray.AddFieldInfo( fieldname, fieldname, _T(""), _T(""), type, mfiType, 0, 0, -1.0f, -1.0f, true );
   
   HTREEITEM hItem = m_fieldTreeCtrl.GetSelectedItem();
   LoadFields();
   LoadFieldTreeCtrl();

   m_fieldTreeCtrl.SelectItem( hItem );
   MakeDirty();
   }


void FieldInfoDlg::OnTvnKeydownFieldtree(NMHDR *pNMHDR, LRESULT *pResult)
   {
   LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
   *pResult = 0;
   }
