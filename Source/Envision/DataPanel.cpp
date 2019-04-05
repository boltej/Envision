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
// DataPanel.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "mainfrm.h"
#include "DataPanel.h"
#include "QueryBuilder.h"
#include "activeWnd.h"

#include "mappanel.h"
#include <map.h>
#include <mapLayer.h>
#include <DirPlaceholder.h>
#include <colors.hpp>
#include <EnvModel.h>

extern CMainFrame *gpMain;
extern MapPanel   *gpMapPanel;
extern EnvModel *gpModel;



// DataPanel

IMPLEMENT_DYNCREATE(DataPanel, CDialog)

DataPanel::DataPanel()
: CDialog()
, m_isDirty( false )
{
}


DataPanel::~DataPanel()
   {
   for ( int i=0; i < m_dataSourceArray.GetSize(); i++ )
      delete m_dataSourceArray.GetAt( i );
   }


void DataPanel::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_DATASOURCES, m_dataSources);
   }

BEGIN_MESSAGE_MAP(DataPanel, CDialog)
   ON_CBN_SELCHANGE(IDC_DATASOURCES, &DataPanel::OnCbnSelchangeDatasources)
   ON_BN_CLICKED(IDC_SAVE, &DataPanel::OnBnClickedSave)
   ON_BN_CLICKED(IDC_SAVEAS, &DataPanel::OnBnClickedSaveas)
   ON_BN_CLICKED(IDC_EDIT, &DataPanel::OnBnClickedEdit)
   ON_BN_CLICKED(IDC_SEARCH, &DataPanel::OnBnClickedSearch)
   ON_BN_CLICKED(IDC_QUERY, &DataPanel::OnBnClickedQuery)
   ON_WM_ACTIVATE()
   ON_BN_CLICKED(IDC_REFRESH, &DataPanel::OnBnClickedRefresh)
END_MESSAGE_MAP()


// DataPanel diagnostics

#ifdef _DEBUG
void DataPanel::AssertValid() const
{
	CDialog::AssertValid();
}

#ifndef _WIN32_WCE
void DataPanel::Dump(CDumpContext& dc) const
{
	CDialog::Dump(dc);
}
#endif
#endif //_DEBUG


// DataPanel message handlers
BOOL DataPanel::Create(CWnd *pParent)
   {
   BOOL retVal = CDialog::Create( DataPanel::IDD, pParent );
   ShowWindow( SW_HIDE );

   BOOL ok = m_grid.AttachGrid( this, IDC_GRID );
   ASSERT( ok );

   m_layoutManager.Attach(this);
   m_layoutManager.AddChild( IDC_DSLABEL, 1 ); // attach top left to container
   m_layoutManager.AddChild( IDC_DATASOURCES, 1 );
   m_layoutManager.AddChild( IDC_EDIT, 0 );
   m_layoutManager.AddChild( IDC_SEARCH, 0 );
   m_layoutManager.AddChild( IDC_QUERY, 0 );
   m_layoutManager.AddChild( IDC_REFRESH, 0 );
   m_layoutManager.AddChild( IDC_SAVE, 0 );
   m_layoutManager.AddChild( IDC_SAVEAS, 0 );
   m_layoutManager.AddChild( &m_grid, 0 );

   const int spacing = 6;
   
   // fix label
   m_layoutManager.SetConstraint( IDC_DSLABEL, OX_LMS_LEFT, OX_LMT_SAME, spacing, 0 );

   // various buttons
   m_layoutManager.SetConstraint( IDC_SAVEAS, OX_LMS_RIGHT, OX_LMT_SAME, -spacing, 0 );
   m_layoutManager.SetConstraint( IDC_SAVE, OX_LMS_RIGHT, OX_LMT_OPPOSITE, -spacing, IDC_SAVEAS );
   m_layoutManager.SetConstraint( IDC_REFRESH, OX_LMS_RIGHT, OX_LMT_OPPOSITE, -spacing, IDC_SAVE );
   m_layoutManager.SetConstraint( IDC_QUERY, OX_LMS_RIGHT, OX_LMT_OPPOSITE, -spacing, IDC_REFRESH );
   m_layoutManager.SetConstraint( IDC_SEARCH, OX_LMS_RIGHT, OX_LMT_OPPOSITE, -spacing, IDC_QUERY );
   m_layoutManager.SetConstraint( IDC_EDIT, OX_LMS_RIGHT, OX_LMT_OPPOSITE, -spacing, IDC_SEARCH );

   // datasources combo box
   m_layoutManager.SetConstraint( IDC_DATASOURCES, OX_LMS_LEFT, OX_LMT_OPPOSITE, spacing, IDC_DSLABEL );
   m_layoutManager.SetConstraint( IDC_DATASOURCES, OX_LMS_RIGHT, OX_LMT_OPPOSITE, -spacing, IDC_EDIT );

   // grid
   m_layoutManager.SetConstraint( IDC_GRID, OX_LMS_TOP, OX_LMT_OPPOSITE, 2*spacing, IDC_DSLABEL ); 
   m_layoutManager.SetConstraint( IDC_GRID, OX_LMS_LEFT, OX_LMT_SAME, spacing, 0 );
   m_layoutManager.SetConstraint( IDC_GRID, OX_LMS_RIGHT, OX_LMT_SAME, -spacing, 0 );
   m_layoutManager.SetConstraint( IDC_GRID, OX_LMS_BOTTOM, OX_LMT_SAME, -spacing, 0 );

   m_layoutManager.RedrawLayout();

   //GetDlgItem( IDC_SAVE )->EnableWindow( FALSE );

   return retVal;
   }


int DataPanel::AddMapLayer( MapLayer *pLayer )
   {
   if ( pLayer == NULL || pLayer->m_pDbTable->m_pData == NULL )
      return -1;

   DbTableDataSource *pDS = new DbTableDataSource( pLayer->m_pDbTable );
   m_dataSourceArray.Add( pDS );

   m_dataSources.AddString( pLayer->m_name );
   pDS->m_index = m_grid.AddDataSource( pDS );

   // is this the first one?
   if ( m_dataSourceArray.GetSize() == 1 )
      {
      // then load the data into the grid
      m_dataSources.SetCurSel( 0 );
      LoadDataTable();
      }

   return pDS->m_index;
   }


void DataPanel::OnCbnSelchangeDatasources()
   {
   CWaitCursor c;
   CString msg( _T("Loading table ") );
   CString dataSource;
   m_dataSources.GetWindowText( dataSource );    // this will have the envision name for the layer
   msg += dataSource;
   gpMain->SetStatusMsg( msg );

   LoadDataTable();
   RedrawWindow();

   if ( m_findDlg.m_hWnd != 0 )
      m_findDlg.SetGrid( &m_grid );

   gpMain->SetStatusMsg( _T("Table loaded") );
   }

void DataPanel::OnBnClickedSave()
   {
   DirPlaceholder d;

   int index = m_dataSources.GetCurSel();
   ASSERT( index >= 0 );

   DbTableDataSource *pDS = m_dataSourceArray[ index ];
   DbTable *pTable = pDS->m_pDbTable;
   bool useWide = VData::SetUseWideChar( true );

   CWaitCursor c;
   pTable->SaveDataDBF();

   VData::SetUseWideChar( useWide );
   }


void DataPanel::OnBnClickedSaveas()
   {
   DirPlaceholder d;

   int index = m_dataSources.GetCurSel();
   ASSERT( index >= 0 );

   DbTableDataSource *pDS = m_dataSourceArray[ index ];
   DbTable *pTable = pDS->m_pDbTable;

   CFileDialog dlg( FALSE, "dbf", pTable->m_databaseName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "DBase Files|*.dbf|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      bool useWide = VData::SetUseWideChar( true );

      CWaitCursor c;
      pTable->SaveDataDBF( filename ); // uses DBase/CodeBase

      VData::SetUseWideChar( useWide );
      }
   }


void DataPanel::OnBnClickedEdit()
   {
   // currently editing?
   if ( m_grid.m_isEditable )
      {
      int retVal = AfxMessageBox( _T("Do you want to save your edits?"), MB_YESNOCANCEL );

      switch( retVal )
         {
         case IDYES:
            OnBnClickedSaveas();
            break;

         case IDCANCEL:
            return;

         case IDNO:
            break;
         }

      // stop editing
      SetDlgItemText( IDC_EDIT, _T("Start Editing") );
      m_grid.m_isEditable = false;
      GetDlgItem( IDC_SAVE )->EnableWindow( TRUE );
      m_findDlg.AllowReplace( false );
      }
   else
      {
      SetDlgItemText( IDC_EDIT, _T("Stop Editing") );
      m_grid.m_isEditable = true;
      m_findDlg.AllowReplace( true );
      }
   }


void DataPanel::OnBnClickedSearch()
   {
   if ( m_findDlg.m_hWnd == 0 )
      {
      m_findDlg.Create( IDD_FINDDATA, this );
      m_findDlg.SetGrid( &m_grid );
      m_findDlg.AllowReplace( m_grid.m_isEditable );
      }
   else
      {
      m_findDlg.ShowWindow( SW_SHOW );
      m_findDlg.AllowReplace( m_grid.m_isEditable );
      }
   }


void DataPanel::OnBnClickedQuery()
   {
   int index = this->m_dataSources.GetCurSel();
   DbTableDataSource *pDS = this->m_dataSourceArray[ index ];
   Map *pMap = gpMapPanel->m_pMap;
   MapLayer *pLayer = NULL;

   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      if ( pMap->GetLayer( i )->m_pDbTable == pDS->m_pDbTable )
         {
         pLayer = pMap->GetLayer( i );
         break;
         }
      }

   ASSERT( pLayer != NULL );

   QueryBuilder dlg(gpModel->m_pQueryEngine, NULL, NULL, -1, this );
   dlg.DoModal();
 
   CWaitCursor c;

   pDS->ClearAll();

   for ( int i=0; i < pLayer->GetSelectionCount(); i++ )
      {
      int index = pLayer->GetSelection( i );
      pDS->AddSelection( -1, index, true );
      }
  
   m_grid.RedrawAll();
   }


void DataPanel::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
   {
   CDialog::OnActivate(nState, pWndOther, bMinimized);

   ActiveWnd::SetActiveWnd( &m_grid );
   }


void DataPanel::OnBnClickedRefresh()
   {
   LoadDataTable();
   }


void DataPanel::ClearGrid()
   {
   int ids = m_grid.GetDefDataSource();

   if ( ids != 0 )
      {
      DbTableDataSource *pDS = (DbTableDataSource*) m_grid.GetDataSource( ids );
      pDS->ClearAll();
      }

   m_grid.ResetAll();
   }


int DataPanel::LoadDataTable()
   {
   CWaitCursor c;

   ClearGrid();
   int index = m_dataSources.GetCurSel();

   if ( index >= 0 )
      {
      DbTableDataSource *pDS = m_dataSourceArray.GetAt( index );
      m_grid.SetDefDataSource( pDS->m_index );
      m_grid.SetGridUsingDataSource( pDS->m_index );
      m_grid.AdjustComponentSizes();
      m_grid.RedrawAll();
      }

   return 0;
   }


//////////////////////////////////////////////////////////////////////////////////////
//  DbTableDataSource methods
//////////////////////////////////////////////////////////////////////////////////////

DbTableDataSource::DbTableDataSource( DbTable *pDbTable ) 
: CUGDataSource()
, m_pDbTable( pDbTable )
, m_highlightColor( LTGRAY )
, m_selectionColor( YELLOW )
   {
   }


int DbTableDataSource::GetCell(int col, long row, CUGCell *cell )
   {
   if ( m_pDbTable == NULL )
      return UG_NA;

   if ( row >= m_pDbTable->GetRecordCount() )
      return UG_NA;

   if ( col >= m_pDbTable->GetColCount() )
      return UG_NA;

   DataObj *pDataObj = m_pDbTable->m_pData;
   ASSERT( pDataObj != NULL );
   
   if ( row == -1 )
      {
      if ( col >= 0 )
         cell->SetText( pDataObj->GetLabel( col ) );
      else
         cell->SetText( "" );
      }
   else if ( col == -1 )
      {
      TCHAR buf[ 16 ];
      _itoa_s( row, buf, 15, 10 );
      cell->SetText( buf );
      }
   else
      cell->SetText( pDataObj->GetAsString( col, row ) );

   // bkgrColor
   if ( col >= 0 && row >= 0 )
      {
      long index = GetPropIndex( col, row );
      DTDS_PROPS value;
      if ( m_selectionSet.Lookup( index, value ) )
         cell->SetBackColor( value.bkgrColor );
      }

   return UG_SUCCESS;
   }

int DbTableDataSource::SetCell(int col, long row, CUGCell *cell )
   {
   if ( m_pDbTable == NULL )
      return UG_NA;

   if ( row >= m_pDbTable->GetRecordCount() )
      return UG_NA;

   if ( col >= m_pDbTable->GetColCount() || col == -1 )
      return UG_NA;

   DataObj *pDataObj = m_pDbTable->m_pData;
   ASSERT( pDataObj != NULL );
   if ( row == -1 )
      {
      TCHAR *label = (TCHAR*) cell->GetText();

      if ( label && lstrlen( label ) > 0 )
         pDataObj->SetLabel( col, label );
      }

   else
      {
      TYPE type = m_pDbTable->GetFieldType( col );

      switch( type )
         {
         case TYPE_NULL:
            break;
            
         case TYPE_CHAR:
            pDataObj->Set( col, row, (int) *(cell->GetText()) );
            break;
            
         case TYPE_BOOL:
            pDataObj->Set( col, row, cell->GetBool() );
            break;

         case TYPE_UINT:
         case TYPE_INT:
         case TYPE_SHORT:
            {
            int value;
            cell->GetNumber( &value );
            pDataObj->Set( col, row, value );
            }
            break;

         case TYPE_LONG:
         case TYPE_ULONG:
            {
            long value;
            cell->GetNumber( &value );
            pDataObj->Set( col, row, value );
            }
            break;
            
         case TYPE_FLOAT:
         case TYPE_DOUBLE:
            {
            float value;
            cell->GetNumber( &value );
            pDataObj->Set( col, row, value );
            }
            break;

         case TYPE_STRING:
         case TYPE_DSTRING:
            pDataObj->Set( col, row, cell->GetText() );
            break;

         default:
            ASSERT( 0 );
         }
      }

   //if ( col >= 0 && row >= 0 )
   //   {
   //   long index = GetPropIndex( col, row );
      
   //   int value;
   //   if ( m_selectionSet.Lookup( index, value );
   //      m_propsMatrix[ index ].bkgrColor = cell->GetBackColor();
   //   }

   return UG_SUCCESS;
   }


long DbTableDataSource::GetPropIndex( int col, long row )
   {
   int cols  = m_pDbTable->GetColCount();
   long index = row*cols + col;
   return index;
   }

int DbTableDataSource::FindAll( CString *string, int col, CArray< DTDS_ROW_COL, DTDS_ROW_COL& > &foundRowCols, int flags )
   {
   // col=col to start search in.  if flags includes UG_FIND_ALLCOLUMNS, then col is ignored
   // 
   foundRowCols.RemoveAll();
   flags &= ( ~UG_FIND_UP );     // always ignore this flag

   int _col = col;
   long row = 0;

   if ( flags & UG_FIND_ALLCOLUMNS )
      {
      flags &= (~UG_FIND_ALLCOLUMNS );

      for ( int icol=0; icol < m_pDbTable->GetColCount(); icol++ )
         {
         CString msg( _T("Searching column " ) );
         msg += m_pDbTable->m_pData->GetLabel( icol );
         gpMain->SetStatusMsg( msg );

         row = 0;
         while( FindNext( string, &icol, &row, flags ) != UG_NA )
            {
            foundRowCols.Add( DTDS_ROW_COL( row, icol ) );
            row++;
            }
         }
      }
   else
      {
      while ( FindNext( string, &_col, &row, flags ) != UG_NA )
         {
         foundRowCols.Add( DTDS_ROW_COL( row, _col ) );
         row++;
         }
      }
   
   CString msg;
   msg.Format( "Found %i matches", foundRowCols.GetSize() );
   gpMain->SetStatusMsg( msg );

   return (int) foundRowCols.GetSize();
   }


int DbTableDataSource::FindNext(CString *string, int *fcol, long *frow, int flags)
   {
   // *string – The string to look for.
   // *fcol - A column in the custom data source to be searched.
   // *frow - A pointer to a long integer value (0-based) that identifies the row which contains the string.
   // flags - Determines the type of search and its direction (see below).
   //
   // Possible flag values are:
   //
   // UG_FIND_PARTIAL (1) Search for partial string allowed. 
   // UG_FIND_CASEINSENSITIVE (2) Search is case insensitive.
   // UG_FIND_UP (4) Search in upwards direction.
   // UG_FIND_ALLCOLUMNS (8) Search all columns, not just the one specified by the *col parameter.
   // If you pass a flag value of UG_FIND_ALLCOLUMNS then the *col parameter is set to the 
   //   column where the string was found. 
   int cols = m_pDbTable->GetColCount();
   int rows = m_pDbTable->GetRecordCount();

   bool findPartial         = flags & UG_FIND_PARTIAL         ? true : false;
   bool findCaseInsensitive = flags & UG_FIND_CASEINSENSITIVE ? true : false;
   bool findUp              = flags & UG_FIND_UP ? true : false;

   if ( findCaseInsensitive )
      string->MakeLower();

   int col = 0;
   int row = *frow;
   int startCol = *fcol;
   int endCol   = *fcol+1;

   if ( flags & UG_FIND_ALLCOLUMNS )
      { startCol = 0; endCol = cols; }

   for ( col=startCol; col < endCol; col++ )
      {
      while ( row >= 0 && row < rows )
      //for ( row=0; row < rows; row++ )
         {
         CString value;
         m_pDbTable->GetData( row, col, value );

         if ( findCaseInsensitive )
            value.MakeLower();
         
         if ( findPartial )
            {
            if ( value.Find( *string ) >= 0 )
               {
               *fcol = col;
               *frow = row;
               return UG_SUCCESS;
               }
            }            
         else // must have full match
            {
            if ( value.Compare( *string ) == 0 )
               {
               *fcol = col;
               *frow = row;
               return UG_SUCCESS;
               }
            }

         if ( findUp )
            --row;
         else
            ++row;
         }  // end of: while ( row >=0 && row < rows )
      }  // end of:  for ( col < cols )
   
   return UG_NA;
   }


void DbTableDataSource::AddSelection( int col, long row, bool highlightRow )
   {
   if ( col >= 0 )
      {
      long index = GetPropIndex( col, row );
      m_selectionSet.SetAt( index, DTDS_PROPS( m_selectionColor ) );
      }

   int cols  = m_pDbTable->GetColCount();

   if ( highlightRow )
      {
      for ( int hcol=0; hcol < cols; hcol++ )
         {
         if ( hcol != col )
            {
            long index = GetPropIndex( hcol, row );
            DTDS_PROPS props( m_highlightColor );

            // don't overwrite previously selected item
            if ( ! m_selectionSet.Lookup( index, props ) )
               m_selectionSet.SetAt( index, props );
            }
         }
      }
   }


void DbTableDataSource::ClearSelection( int col, long row )
   {
   long index = GetPropIndex( col, row );
   m_selectionSet.RemoveKey( index );
   }


void DbTableDataSource::ClearAll( void )
   {
   m_selectionSet.RemoveAll();
   }

