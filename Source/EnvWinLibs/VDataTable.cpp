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
// VDataTable.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "VDataTable.h"

#include <vdataobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// VDataTable

VDataTable::VDataTable( VDataObj *pData )
: m_pData( pData )
{
ASSERT( m_pData != NULL );
}

VDataTable::~VDataTable()
{
}


BEGIN_MESSAGE_MAP(VDataTable, CWnd)
	//{{AFX_MSG_MAP(VDataTable)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// VDataTable message handlers

void VDataTable::SetDataSource( VDataObj *pData )
   {
   m_pData = pData;
   LoadTable();
   }


void VDataTable::LoadTable()
   {
   m_grid.DeleteAllItems();
   int cols = m_pData->GetColCount();
   int rows = m_pData->GetRowCount();

   m_grid.SetColumnCount( cols );

   m_grid.SetFixedRowCount( 1 );
   m_grid.SetRowCount( rows );

   //for( int col=0; col < cols; col++ )
   //   {
   //   LPCTSTR label = m_pData->GetLabel( col );
   //   m_grid.SetItemText( 0, col, label );
   //   }

/*   for ( int row=0; row < rows; row++ )
      {
      for ( int col=0; col < cols; col++ )
         {
         VData &v = m_pData->Get( col, row );
         m_grid.SetItemText( row+1, col, v.GetAsString() );
        }
      }*/
   }


int VDataTable::OnCreate(LPCREATESTRUCT lpCreateStruct) 
   {
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
   RECT rect;
   GetClientRect( &rect );
   
   BOOL ok = m_grid.Create( rect, this, 100 ); //, DWORD dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE);
   ASSERT( ok );

   m_grid.SetVirtualMode( TRUE );
   
   // get cell properties
   if ( m_pData != NULL )
      LoadTable();

	return 0;
   }
   
void VDataTable::OnSize(UINT nType, int cx, int cy) 
   {
	CWnd::OnSize(nType, cx, cy);

   if ( IsWindow( m_grid.m_hWnd ) )
      {
      m_grid.MoveWindow( 0, 0, cx, cy, TRUE );
      }
   }


BOOL VDataTable::Create( LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID ) 
   {	
	return CWnd::Create(NULL, lpszWindowName, WS_CHILD | WS_OVERLAPPEDWINDOW | WS_VISIBLE, rect, pParentWnd, nID, NULL);
   }


BOOL VDataTable::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
   {
   if (wParam == (WPARAM) m_grid.GetDlgCtrlID())
      {
      *pResult = 1;
      GV_DISPINFO *pDispInfo = (GV_DISPINFO*)lParam;

      if ( GVN_GETDISPINFO == pDispInfo->hdr.code )
         {
         if ( m_pData == NULL )
            return TRUE;

         int row = pDispInfo->item.row;
         int col = pDispInfo->item.col;

         if ( row == 0 )
            pDispInfo->item.strText = m_pData->GetLabel( col );
         else
            {
            VData &v = m_pData->Get( col, row-1 );
            pDispInfo->item.strText = v.GetAsString();
            }

         return TRUE;
         }

      else if (GVN_ODCACHEHINT == pDispInfo->hdr.code)
         {
         GV_CACHEHINT *pCacheHint = (GV_CACHEHINT*)pDispInfo;
         TRACE(_T("Cache hint received for cell range %d,%d - %d,%d\n"),
                  pCacheHint->range.GetMinRow(), 
                  pCacheHint->range.GetMinCol(),
                  pCacheHint->range.GetMaxRow(), 
                  pCacheHint->range.GetMaxCol());
         }
      }
    
   return CWnd::OnNotify(wParam, lParam, pResult);
   }
