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
// FindDataDlg.cpp : implementation file
//

#include "stdafx.h"

#include "Envision.h"
#include "FindDataDlg.h"
#include "DataPanel.h"

// FindDataDlg dialog

IMPLEMENT_DYNAMIC(FindDataDlg, CDialog)

FindDataDlg::FindDataDlg( CWnd* pParent /*=NULL*/)
	: CDialog(FindDataDlg::IDD, pParent)
   , m_pGrid( NULL )
   , m_col( -1 )
   , m_row( -1 )
   , m_showingMore( true )
   , m_matchCase(FALSE)
   , m_matchEntire(FALSE)
   , m_findAll( FALSE )
   , m_searchUp(FALSE)
   , m_searchThis(TRUE)
   , m_searchSelected(FALSE)
   , m_searchAll(FALSE)
   , m_findString(_T(""))
   , m_replaceString(_T(""))
   , m_field(_T(""))
   { }

FindDataDlg::~FindDataDlg()
{ }


void FindDataDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);

DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Text(pDX, IDC_FINDSTRING, m_findString);
DDX_Text(pDX, IDC_REPLACESTRING, m_replaceString);
DDX_CBString(pDX, IDC_FIELDS, m_field);
DDX_Check(pDX, IDC_MATCH_CASE, m_matchCase);
DDX_Check(pDX, IDC_MATCH_ENTIRE, m_matchEntire);
DDX_Check(pDX, IDC_FINDALL, m_findAll);
DDX_Check(pDX, IDC_SEARCH_UP, m_searchUp);
DDX_Check(pDX, IDC_SEARCH_THIS, m_searchThis);
DDX_Check(pDX, IDC_SEARCH_SELECTED, m_searchSelected);
DDX_Check(pDX, IDC_SEARCH_ALL, m_searchAll);
}


BEGIN_MESSAGE_MAP(FindDataDlg, CDialog)
   ON_BN_CLICKED(IDOK, &FindDataDlg::OnBnClickedOk)
   ON_BN_CLICKED(IDC_REPLACE, &FindDataDlg::OnBnClickedReplace)
   ON_BN_CLICKED(IDCANCEL, &FindDataDlg::OnBnClickedCancel)
   ON_BN_CLICKED(IDC_MORE_LESS, &FindDataDlg::OnBnClickedMoreLess)
END_MESSAGE_MAP()


void FindDataDlg::SetGrid( DataGrid *pGrid )
   {
   m_pGrid = pGrid;

   m_fields.ResetContent();
   for ( int i=0; i < m_pGrid->GetNumberCols()-1; i++ )
      {
      LPCTSTR hdr = m_pGrid->QuickGetText( i, -1 );

      if( hdr )
         m_fields.AddString( hdr );
      }

   m_fields.SetCurSel( m_pGrid->GetCurrentCol()+1 );
   }


// FindDataDlg message handlers

void FindDataDlg::OnBnClickedOk()
   {
   FindReplace( false );
   }

void FindDataDlg::OnBnClickedReplace()
   {
   FindReplace( true );
   }


void FindDataDlg::FindReplace( bool replace )
   {
   UpdateData( 1 );

   int flags = 0;

   if ( ! m_matchCase )
      flags |= UG_FIND_CASEINSENSITIVE;

   if ( ! m_matchEntire )
      flags |= UG_FIND_PARTIAL;

   if ( m_searchUp )
      flags |= UG_FIND_UP;

   m_pGrid->FindInAllCols( m_searchAll ? 1 : 0 );

   if ( m_searchAll )
      {
      flags |= UG_FIND_ALLCOLUMNS;
      m_col = 0;
      m_row = 0;
      }
   else if ( m_searchSelected )
      {
      m_col = m_pGrid->GetCurrentCol();
      //m_row = m_pGrid->GetCurrentRow();
      }
   else
      {
      m_col = m_fields.GetCurSel();
      //m_row = m_pGrid->GetCurrentRow();
      }

   if ( m_searchUp )
      m_row--;
   else
      m_row++;

   int index = m_pGrid->GetDefDataSource();
   DbTableDataSource *pDS = (DbTableDataSource*) m_pGrid->GetDataSource( index );

   CWaitCursor c;

   pDS->ClearAll();

   if ( m_findAll )
      {
      CArray< DTDS_ROW_COL, DTDS_ROW_COL& > foundRowCols;
      if ( pDS->FindAll( &m_findString, m_col, foundRowCols, flags ) != UG_NA )
         {
         for ( int i=0; i < foundRowCols.GetSize(); i++ )
            {
            pDS->AddSelection( foundRowCols[ i ].col, foundRowCols[ i ].row, true );

            if ( replace )
               m_pGrid->QuickSetText( foundRowCols[ i ].col, foundRowCols[ i ].row, m_replaceString ); 
            }
         }
      }
   else
      {
      if ( pDS->FindNext( &m_findString, &m_col, &m_row, flags ) != UG_NA )
         {
         pDS->AddSelection( m_col, m_row, true );
         
         if ( replace )
            m_pGrid->QuickSetText( m_col, m_row, m_replaceString ); 

         m_pGrid->GotoCell( m_col, m_row );
         }
      }

   m_pGrid->RedrawAll();
   }


void FindDataDlg::OnBnClickedCancel()
   {
   ShowWindow( SW_HIDE );
   //OnCancel();
   }


void FindDataDlg::OnBnClickedMoreLess()
   {
   m_showingMore = ( m_showingMore ? false : true );

   GetDlgItem( IDC_FRAME )->ShowWindow( m_showingMore ? 1 : 0 );
   GetDlgItem( IDC_FIELDS )->ShowWindow( m_showingMore ? 1 : 0 );
   GetDlgItem( IDC_MATCH_CASE )->ShowWindow( m_showingMore ? 1 : 0 );
   GetDlgItem( IDC_MATCH_ENTIRE )->ShowWindow( m_showingMore ? 1 : 0 );
   GetDlgItem( IDC_FINDALL )->ShowWindow( m_showingMore ? 1 : 0 );
   GetDlgItem( IDC_SEARCH_THIS )->ShowWindow( m_showingMore ? 1 : 0 );
   GetDlgItem( IDC_SEARCH_UP )->ShowWindow( m_showingMore ? 1 : 0 );
   GetDlgItem( IDC_SEARCH_SELECTED )->ShowWindow( m_showingMore ? 1 : 0 );
   GetDlgItem( IDC_SEARCH_ALL )->ShowWindow( m_showingMore ? 1 : 0 );

   int sign = m_showingMore ? 1 : -1;

   m_moreRect.OffsetRect ( 0, sign * m_moreLessSize );
   m_findRect.OffsetRect ( 0, sign * m_moreLessSize );
   m_closeRect.OffsetRect( 0, sign * m_moreLessSize );

   GetDlgItem( IDC_MORE_LESS )->MoveWindow( &m_moreRect );
   GetDlgItem( IDOK )->MoveWindow( &m_findRect );
   GetDlgItem( IDCANCEL )->MoveWindow( &m_closeRect );

   CRect dlgRect;
   GetWindowRect( &dlgRect );
   //GetParent()->ScreenToClient( &dlgRect );
   dlgRect.bottom += sign * m_moreLessSize;
   MoveWindow( &dlgRect );

   if ( m_showingMore )
      GetDlgItem( IDC_MORE_LESS )->SetWindowText( "<< Show Less" );
   else
      GetDlgItem( IDC_MORE_LESS )->SetWindowText( "Show More >>" );
   }

BOOL FindDataDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   GetDlgItem( IDC_MORE_LESS )->GetWindowRect( &m_moreRect );
   GetDlgItem( IDOK )->GetWindowRect( &m_findRect );
   GetDlgItem( IDCANCEL )->GetWindowRect( &m_closeRect );

   CRect dlgRect;
   GetWindowRect( &dlgRect );
   int bottomMargin = dlgRect.bottom - m_moreRect.bottom;

   CRect findStringRect;
   GetDlgItem( IDC_FINDSTRING )->GetWindowRect( &findStringRect );

   m_moreLessSize = dlgRect.bottom - findStringRect.bottom - m_moreRect.Height() - 4 - bottomMargin;

   ScreenToClient( &m_moreRect );
   ScreenToClient( &m_findRect );
   ScreenToClient( &m_closeRect );

   if ( ! m_allowReplace )
      {
      GetDlgItem( IDC_REPLACE )->EnableWindow( 0 );
      GetDlgItem( IDC_REPLACESTRING )->EnableWindow( 0 );
      GetDlgItem( IDC_REPLACELABEL )->EnableWindow( 0 );
      }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void FindDataDlg::AllowReplace( bool allowReplace )
   {
   m_allowReplace = allowReplace;

   if ( m_hWnd )
      {
      GetDlgItem( IDC_REPLACE )->EnableWindow( m_allowReplace ? 1 : 0 );
      GetDlgItem( IDC_REPLACESTRING )->EnableWindow( m_allowReplace ? 1 : 0 );
      GetDlgItem( IDC_REPLACELABEL )->EnableWindow( m_allowReplace ? 1 : 0 );
      }

   }