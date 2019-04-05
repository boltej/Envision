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
// DeltaReport.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "DeltaReport.h"
#include <EnvModel.h>

#include <DeltaArray.h>

extern EnvModel *gpModel;


// DeltaReport dialog

IMPLEMENT_DYNAMIC(DeltaReport, CDialog)

DeltaReport::DeltaReport( DeltaArray *pDeltaArray, CWnd* pParent /*=NULL*/)
	: CDialog(DeltaReport::IDD, pParent)
   , m_pDeltaArray( pDeltaArray )
{
ASSERT( m_pDeltaArray != NULL );
}


DeltaReport::~DeltaReport()
{
}

void DeltaReport::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_ERRORLIST, m_errorList);
DDX_Control(pDX, IDC_PROGRESS, m_progress);
   }


BEGIN_MESSAGE_MAP(DeltaReport, CDialog)
   //ON_MESSAGE(WM_USER, CheckErrors)
   ON_BN_CLICKED(IDC_CHECKERRORS, &DeltaReport::OnBnClickedCheckerrors)
END_MESSAGE_MAP()


// DeltaReport message handlers

BOOL DeltaReport::OnInitDialog()
   {
   CDialog::OnInitDialog();

   INT_PTR count= m_pDeltaArray->GetCount();

   CString _count;
   _count.Format( _T("Delta Count: %i"), count );
   GetDlgItem( IDC_COUNT )->SetWindowText( _count );

   CString memory;
   long size = (long) count * sizeof( DELTA );

   if( size > 1000000L )
      memory.Format( _T("Memory Usage: %6.2f MB"), float( size/1000000.0f ) );
   else
      memory.Format( _T("Memory Usage: %i KB"), int( size/1000) );
   GetDlgItem( IDC_MEMORY )->SetWindowText( memory );

   // set up list control
   m_errorList.InsertColumn( 0, "IDU", LVCFMT_LEFT, 60, -1 );
   m_errorList.InsertColumn( 1, "Type", LVCFMT_LEFT, 60, -1 );
   m_errorList.InsertColumn( 2, "Date", LVCFMT_LEFT, 60, -1 );
   m_errorList.InsertColumn( 3, "Starting Value", LVCFMT_LEFT, 120, -1 );
   m_errorList.InsertColumn( 4, "Ending Value", LVCFMT_LEFT, 120, -1 );

   m_errorList.InsertColumn( 5, "Prev IDU", LVCFMT_LEFT, 60, -1 );
   m_errorList.InsertColumn( 6, "Type", LVCFMT_LEFT, 60, -1 );
   m_errorList.InsertColumn( 7, "Date", LVCFMT_LEFT, 60, -1 );
   m_errorList.InsertColumn( 8, "Starting Value", LVCFMT_LEFT, 120, -1 );
   m_errorList.InsertColumn( 9, "Ending Value", LVCFMT_LEFT, 120, -1 );

   m_progress.SetRange32( 0, int( count / 100 ) );
//   this->PostMessage( WM_USER, 0, 0 );

   return TRUE;
   }


LRESULT DeltaReport::CheckErrors(WPARAM wParam, LPARAM lParam)
   {
   UNREFERENCED_PARAMETER(wParam);
   UNREFERENCED_PARAMETER(lParam);
   
   CWaitCursor c;
   LVITEM item;
   item.mask = LVIF_TEXT;

   int items = 0;
   INT_PTR count= m_pDeltaArray->GetCount();

   SetDlgItemText( IDC_STATIC, _T("Error Report started..." ));

   // do error detection - note: an error exists if 
   // delta.oldValue is different than the previous delta
   // for the same idu, columns new value...
   for ( INT_PTR i=count-1; i > 0; i-- )
      {
      // check for new messages
      //MSG  msg;
      //while( PeekMessage( &msg, NULL, NULL, NULL, PM_REMOVE ) )
      //   {
      //   TranslateMessage( &msg );
      //   DispatchMessage( &msg );
      //   }

      if ( ( (count-i) % 100) == 0 )
         m_progress.SetPos( int( (count-i) / 100 ));

      DELTA &delta = m_pDeltaArray->GetAt( i );

      for ( INT_PTR j=i-1; j >= 0; j-- )
         {
         DELTA &prevDelta = m_pDeltaArray->GetAt( j );

         if ( delta.cell == prevDelta.cell && delta.col == prevDelta.col )
            {
            if ( delta.oldValue.Compare( prevDelta.newValue ) == false )
               {
               TCHAR idu[ 16 ], date[8], pidu[ 16 ], pdate[ 8 ];

               item.iItem = items;
               item.iSubItem = 0;   // idu
               _itoa_s( delta.cell, idu, 16, 10 );
               item.pszText = idu;
               m_errorList.InsertItem( &item );

               item.iSubItem = 1;   // type
               item.pszText = (LPTSTR) GetDeltaTypeStr( delta.type );
               m_errorList.SetItem( &item );

               item.iSubItem = 2;   // date
               _itoa_s( delta.year, date, 8, 10 );
               item.pszText = date;
               m_errorList.SetItem( &item );

               item.iSubItem = 3;   // starting value
               item.pszText = (LPTSTR) delta.oldValue.GetAsString();
               m_errorList.SetItem( &item );

               item.iSubItem = 4;   // ending value
               item.pszText = (LPTSTR)delta.newValue.GetAsString();
               m_errorList.SetItem( &item );

               item.iSubItem = 5;   // idu
               _itoa_s( prevDelta.cell, pidu, 16, 10 );
               item.pszText = pidu;
               m_errorList.SetItem( &item );

               item.iSubItem = 6;   // type
               item.pszText = (LPTSTR) GetDeltaTypeStr( prevDelta.type );
               m_errorList.SetItem( &item );

               item.iSubItem = 7;   // date
               _itoa_s( prevDelta.year, pdate, 8, 10 );
               item.pszText = pdate;
               m_errorList.SetItem( &item );

               item.iSubItem = 8;   // starting value
               item.pszText = (LPTSTR) prevDelta.oldValue.GetAsString();
               m_errorList.SetItem( &item );

               item.iSubItem = 9;   // ending value
               item.pszText = (LPTSTR) prevDelta.newValue.GetAsString();
               m_errorList.SetItem( &item );

               items++;

               CString errors;
               errors.Format( _T("%i Errors encountered..."), items+1 );
               SetDlgItemText( IDC_STATIC, errors );
               }

            break;   // no need to look further
            }  // end of:  // previous Matching IDU found
         }
      }

   if ( items == 0 )
      SetDlgItemText( IDC_STATIC, _T("Error Report completed - no errors were found..." ));


   return 1;
   }

void DeltaReport::OnBnClickedCheckerrors()
   {
   CheckErrors(0,0);
   }


LPCTSTR DeltaReport::GetDeltaTypeStr( short type )  // note: this function is duplicated in DeltaViewer.cpp

   {
   switch( type )
      {
      case DT_NULL:           return _T("Null");
      case DT_POLICY:         return _T("Policy");
      case DT_SUBDIVIDE:      return _T("Subdivide");
      case DT_MERGE:          return _T("Merge");
      case DT_INCREMENT_COL:  return _T("IncrementCol");
      case DT_POP:            return _T("POP");
      }

   if ( type >= DT_MODEL && type < DT_PROCESS )
      {
      int index = type-DT_MODEL;
      ASSERT( index < gpModel->GetEvaluatorCount() );
      EnvEvaluator *pInfo = gpModel->GetEvaluatorInfo( index );
      return pInfo->m_name;
      }

   else if ( type >= DT_PROCESS )
      {
      int index = type-DT_PROCESS;
      ASSERT( index < gpModel->GetModelProcessCount() );
      EnvModelProcess *pInfo = gpModel->GetModelProcessInfo( index );
      return pInfo->m_name;
      }

   return _T("Unknown");
   }