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
// CellPropDlg.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include "CellPropDlg.h"
#include "map.h"
#include "maplayer.h"
#include "MapFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CellPropDlg dialog

CRect CellPropDlg::m_windowRect(0,0,0,0);
CellPropDlg::CellPropDlg( MapWindow *pMapWnd, MapLayer *pLayer, int currentCell, CWnd* pParent )
	: CDialog(CellPropDlg::IDD, pParent),
     m_pMapWnd( pMapWnd ),
     m_pLayer( pLayer ),
     m_currentCell( currentCell ),
     m_isModal( false )

   {
	//{{AFX_DATA_INIT(CellPropDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_pParent = pParent;
   }


void CellPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CellPropDlg)
	DDX_Control(pDX, IDC_LIB_FLASHCELL, m_flashCell);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CellPropDlg, CDialog)
	//{{AFX_MSG_MAP(CellPropDlg)
	ON_BN_CLICKED(IDC_LIB_FLASHCELL, OnFlashcell)
	ON_BN_CLICKED(IDC_LIB_NEXTCELL, OnNextcell)
	ON_BN_CLICKED(IDC_LIB_PREVCELL, OnPrevcell)
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
   ON_WM_SIZE()
   ON_WM_SIZING()
END_MESSAGE_MAP()

BEGIN_EASYSIZE_MAP(CellPropDlg)
   EASYSIZE(IDC_LIB_GRID_FRAME,ES_BORDER,ES_BORDER,ES_BORDER,ES_BORDER,0)
   EASYSIZE(IDC_LIB_FLASHCELL,ES_BORDER,ES_KEEPSIZE,ES_BORDER,ES_BORDER,0)
   EASYSIZE(IDC_LIB_PREVCELL,ES_KEEPSIZE,ES_KEEPSIZE,ES_BORDER,ES_BORDER,0)
   EASYSIZE(IDC_LIB_NEXTCELL,ES_KEEPSIZE,ES_KEEPSIZE,ES_BORDER,ES_BORDER,0)
END_EASYSIZE_MAP

/////////////////////////////////////////////////////////////////////////////
// CellPropDlg message handlers

BOOL CellPropDlg::OnInitDialog() 
   {
	CDialog::OnInitDialog();

   INIT_EASYSIZE;

   if ( !m_windowRect.IsRectNull() )
      {
      MoveWindow( m_windowRect );
      }
   else
      CenterWindow();

   CRect rect;
   CWnd *pWnd = GetDlgItem( IDC_LIB_GRID_FRAME );
   GetDlgItem( IDC_LIB_GRID_FRAME )->GetWindowRect( rect );
   ScreenToClient( rect );

   BOOL ok = m_grid.Create( rect, this, 100 ); //, DWORD dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE);
   ASSERT( ok );
   
   // get cell properties
   ASSERT( m_pLayer != NULL );
   LoadCell();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
   }


void CellPropDlg::LoadCell( void )
   {
   m_grid.DeleteAllItems();

   if ( m_pLayer->m_pData == NULL )
      return;

   int fields = m_pLayer->GetFieldCount();

   m_grid.SetRowCount( fields );
   m_grid.SetColumnCount( 2 );

   for ( int row=0; row < fields; row++ )
      {
      CString label = m_pLayer->GetFieldLabel( row );

      MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( label );
      if ( pInfo )
         {
         label = m_pLayer->GetFieldLabel( row );
         label += " (";
         label += pInfo->label;
         label += ")";
         }

      m_grid.SetItemText( row, 0, label );
      CString value;
      m_pLayer->GetData( m_currentCell, row, value );
      m_grid.SetItemText( row, 1, value );
      }

   CRect rect;
   m_grid.GetClientRect( rect );
   m_grid.SetColumnWidth( 0, rect.Width()/2 );
   m_grid.SetColumnWidth( 1, rect.Width() - rect.Width()/2 );

   CString wndLabel;
   wndLabel.Format( _T("Polygon: %i"), m_currentCell );
   SetWindowText( wndLabel );

   m_grid.SortTextItems( 0, true );
   }
	

void CellPropDlg::OnFlashcell() 
   {
   Map *pMap  = m_pLayer->m_pMap;
   int  index = pMap->GetLayerIndex( m_pLayer );
   m_pMapWnd->FlashPoly( index, m_currentCell );  // 0th layer, m_cell cell
   }

void CellPropDlg::OnNextcell() 
   {
   m_currentCell++;

   GetDlgItem( IDC_LIB_PREVCELL )->EnableWindow( TRUE );

   if ( m_currentCell == m_pLayer->GetRecordCount() - 1 )
      GetDlgItem( IDC_LIB_NEXTCELL )->EnableWindow( false );

   LoadCell();
	}


void CellPropDlg::OnPrevcell() 
   {
   m_currentCell--;

   GetDlgItem( IDC_LIB_NEXTCELL )->EnableWindow( TRUE );

   if ( m_currentCell == 0 )
      GetDlgItem( IDC_LIB_PREVCELL )->EnableWindow( false );

   LoadCell();
   }

void CellPropDlg::OnCancel()
   {
   if ( m_isModal )
      CDialog::OnCancel();
   else
      DestroyWindow();

   //CDialog::OnCancel();
   }

void CellPropDlg::OnDestroy()
   {
   CDialog::OnDestroy();

   GetWindowRect( m_windowRect );
   }

void CellPropDlg::PostNcDestroy()
   {
   CDialog::PostNcDestroy();

   if ( m_isModal == false )
      {
      m_pParent->RedrawWindow();
      ((MapFrame*)m_pParent)->m_pCellPropDlg = NULL;
      delete this;
      }
   }

void CellPropDlg::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);

   UPDATE_EASYSIZE;

   if ( IsWindow( m_hWnd ) )
      {
      CWnd *pWnd = GetDlgItem( IDC_LIB_GRID_FRAME );
      if ( pWnd && IsWindow( m_grid.m_hWnd ) )
         {
         CRect rect;
         GetDlgItem( IDC_LIB_GRID_FRAME )->GetWindowRect( &rect );
         ScreenToClient( rect );
         m_grid.MoveWindow( rect );
         m_grid.GetClientRect( rect );
         m_grid.SetColumnWidth( 0, rect.Width()/2 );
         m_grid.SetColumnWidth( 1, rect.Width() - rect.Width()/2 );
         }
      }
   }

void CellPropDlg::OnSizing(UINT fwSide, LPRECT pRect)
   {
   CDialog::OnSizing(fwSide, pRect);

   EASYSIZE_MINSIZE(100,250,fwSide,pRect);
   }
