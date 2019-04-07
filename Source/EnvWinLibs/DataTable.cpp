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
// DataTable.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include <DataTable.h>
#include <dataobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DataTable

DataTable::DataTable( DataObj *pData )
: m_pData( pData )
{
ASSERT( m_pData != NULL );
}

DataTable::~DataTable()
{
}


BEGIN_MESSAGE_MAP(DataTable, CWnd)
	//{{AFX_MSG_MAP(DataTable)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// DataTable message handlers

void DataTable::LoadTable()
   {
   m_grid.DeleteAllItems();
   int cols = m_pData->GetColCount();
   int rows = m_pData->GetRowCount();

   m_grid.SetColumnCount( cols );

   m_grid.SetFixedRowCount( 1 );
   m_grid.SetRowCount( rows + 1 ); // the " + 1" is for the fixed row

   for( int col=0; col < cols; col++ )
      {
      LPCTSTR label = m_pData->GetLabel( col );
      m_grid.SetItemText( 0, col, label );
      }

   VData v;

   for ( int row=0; row < rows; row++ )
      {
      for ( int col=0; col < cols; col++ )
         {
         bool ok = m_pData->Get( col, row, v );
         ASSERT( ok );
         ok = m_grid.SetItemText( row+1, col, v.GetAsString() ) ? TRUE : FALSE;
         ASSERT( ok );
        }
      }
   }


int DataTable::OnCreate(LPCREATESTRUCT lpCreateStruct) 
   {
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   RECT rect;
   GetClientRect( &rect );
   
   BOOL ok = m_grid.Create( rect, this, 100 ); //, DWORD dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE);
   ASSERT( ok );
   
   // get cell properties
   ASSERT( m_pData != NULL );
   LoadTable();

	return 0;
   }
   
void DataTable::OnSize(UINT nType, int cx, int cy) 
   {
	CWnd::OnSize(nType, cx, cy);

   if ( IsWindow( m_grid.m_hWnd ) )
      {
      m_grid.MoveWindow( 0, 0, cx, cy, TRUE );
      }
   }


BOOL DataTable::Create( LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID ) 
   {	
   return CWnd::Create(NULL, lpszWindowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            rect, pParentWnd, nID, NULL);
   }
