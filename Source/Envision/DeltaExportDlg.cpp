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
// DeltaExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "DeltaExportDlg.h"
#include "afxdialogex.h"

#include <DataManager.h>
#include "EnvDoc.h"
#include <EnvModel.h>
#include <Scenario.h>

#include <Maplayer.h>
#include <Path.h>
#include <DirPlaceholder.h>



extern MapLayer *gpCellLayer;
extern CEnvDoc  *gpDoc;
extern EnvModel *gpModel;
extern ScenarioManager *gpScenarioManager;


// DeltaExportDlg dialog

/*
void DeltaBrowser::OnBrowse( void )
   {
   DirPlaceholder d;

   CString path;
   GetWindowText( path );

   CFileDialog dlg( FALSE, "csv", path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "CSV Files|*.csv|All Files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      path = dlg.GetPathName();
      SetWindowText( path );
      }
   } */


IMPLEMENT_DYNAMIC(DeltaExportDlg, CDialog)

DeltaExportDlg::DeltaExportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(DeltaExportDlg::IDD, pParent)
   , m_selectedOnly(FALSE)
   , m_includeDuplicates(FALSE)
   , m_selectedDates(FALSE)
   , m_startYear(0)
   , m_endYear(-1)
   , m_useStartValue(FALSE)
   , m_startValues(_T(""))
   , m_lastField( -1 )
   , m_iduField(_T(""))
   { }

DeltaExportDlg::~DeltaExportDlg()
{
}

void DeltaExportDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_RUNS, m_runs);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Check(pDX, IDC_SELECTEDONLY, m_selectedOnly);
DDX_Check(pDX, IDC_INCLUDEDUPLICATES, m_includeDuplicates);
DDX_Check(pDX, IDC_SELECTEDDATES, m_selectedDates);
DDX_Text(pDX, IDC_STARTYEAR, m_startYear);
DDX_Text(pDX, IDC_ENDYEAR, m_endYear);
DDX_Check(pDX, IDC_USESTARTVALUE, m_useStartValue);
DDX_Text(pDX, IDC_STARTVALUES, m_startValues);
DDX_Check(pDX, IDC_USEENDVALUE, m_useEndValue);
DDX_Text(pDX, IDC_ENDVALUES, m_endValues);
DDX_Control(pDX, IDC_IDUFIELD, m_iduIDCombo);
DDX_CBString(pDX, IDC_IDUFIELD, m_iduField);
   }


BEGIN_MESSAGE_MAP(DeltaExportDlg, CDialog)
   ON_BN_CLICKED(IDC_ALL, &DeltaExportDlg::OnBnClickedAll)
   ON_BN_CLICKED(IDC_NONE, &DeltaExportDlg::OnBnClickedNone)
   ON_BN_CLICKED(IDC_SELECTEDDATES, &DeltaExportDlg::OnBnClickedSelecteddates)
   ON_LBN_SELCHANGE(IDC_RUNS, &DeltaExportDlg::OnLbnSelchangeRuns)
   ON_BN_CLICKED(IDC_USESTARTVALUE, &DeltaExportDlg::OnBnClickedUsestartvalue)
   ON_BN_CLICKED(IDC_USEENDVALUE, &DeltaExportDlg::OnBnClickedUseendvalue)
   ON_LBN_SELCHANGE(IDC_FIELDS, &DeltaExportDlg::OnLbnSelchangeFields)
END_MESSAGE_MAP()


// DeltaExportDlg message handlers

BOOL DeltaExportDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   int runs = gpModel->m_pDataManager->GetRunCount();

   for ( int i=0; i < runs; i++ )
      {
      //RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( i );

      CString msg;
      msg.Format( "Run %i", i );
      m_runs.AddString( msg );
      }

   m_runs.SetCurSel( runs-1 );

   m_iduIDCombo.AddString( "<Use IDU Offset>" );

   // fields
   for ( int i=0; i < gpCellLayer->GetFieldCount(); i++ )
      {
      LPCTSTR field = gpCellLayer->GetFieldLabel( i );
      m_fields.AddString( field );
      }

   for ( int i=0; i < gpCellLayer->GetFieldCount(); i++ )
      {
      m_fields.SetCheck( i, 1 );

      CString field;
      m_fields.GetText( i, field );
      int col = gpCellLayer->GetFieldCol( field );
      ASSERT( col >= 0 );
      m_colToListIndexMap[ col ] = i;

      if ( ::IsInteger( gpCellLayer->GetFieldType( col ) ) )
         m_iduIDCombo.AddString( field );
      }      

   int curSel = m_iduIDCombo.FindString( -1, "<Use IDU Offsets>" );
   m_iduIDCombo.SetCurSel( curSel );

   //SetDefaultPath();

   GetDlgItem( IDC_STARTYEAR )->EnableWindow( FALSE );
   GetDlgItem( IDC_ENDYEAR )->EnableWindow( FALSE );

   m_startValuesArray.SetSize( gpCellLayer->GetFieldCount() );
   m_endValuesArray.SetSize( gpCellLayer->GetFieldCount() );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void DeltaExportDlg::OnOK()
   {
   UpdateData( 1 );
   OnLbnSelchangeFields();  // get any last changes
   
   DirPlaceholder d;

   int run = m_runs.GetCurSel();
   if ( run < 0 )
      return;
   
   nsPath::CPath path = gpDoc->m_iniFile;
   //path.RemoveExtension();
   path.RemoveFileSpec();
   CString file;
   file.Format( "DeltaArray_Run%i.csv", run );

   path.Append( file );
   //m_path.SetWindowText( file ); 

   CFileDialog dlg( FALSE, "csv", (LPCTSTR) path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "csv|*.csv|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      file = dlg.GetPathName();
   else
      return;

   DeltaArray *pDA = gpModel->m_pDataManager->GetDeltaArray( run );
   ASSERT( pDA != NULL );
   MapLayer *pLayer = (MapLayer*) pDA->GetBaseLayer();

   int fieldCount = m_fields.GetCount();

   //CString ext = path.GetExtension();
   VDataObj data( 3, 0 );

   bool includeAllFields = true;

   for ( int j=0; j < fieldCount; j++ )
      {
      if ( m_fields.GetCheck( j ) == 0 )
         {
         includeAllFields = false;
         break;
         }
      }

   bool *includeField = new bool[ fieldCount ];
   for ( int j=0; j < fieldCount; j++ )
      {
      CString field;
      m_fields.GetText( j, field );
      int col = pLayer->GetFieldCol( field );

      includeField[ col ] = ( m_fields.GetCheck( j ) == 0 ) ? false : true;
      }   

   VData **startVDatasArray = new VData*[ fieldCount ];  // order is order of fields in maplayer
   VData **endVDatasArray   = new VData*[ fieldCount ];

   for ( int j=0; j < fieldCount; j++ )
      {
      int listIndex = -1;
      int col = j;
      BOOL ok = m_colToListIndexMap.Lookup( col, listIndex );
      ASSERT( ok );

      // start values first
      if ( m_startValuesArray[ listIndex ].GetLength() > 0 )      // each element is a comma-separated string, in list box (alphabetical) order
         {
         TCHAR buffer[ 128 ];
         lstrcpy( buffer, m_startValuesArray[ listIndex ] );

         // count commas
         int count = 0;
         LPTSTR next = buffer;
         while( *next != NULL )
            {
            if ( *next == ',' )
               count++;
            next++;
            }

         VData *vDataArray = new VData[ count+1 ];
         int i=0;
         TCHAR *token = _tcstok_s( buffer, _T(",\n"), &next );
         while ( token != NULL )
            {
            vDataArray[ i ].Parse( token );
            i++;
            token = _tcstok_s( NULL, _T( ",\n" ), &next );
            }

         startVDatasArray[ j ] = vDataArray;
         }
      else
         startVDatasArray[ j ] = NULL;

      // end values next
      if ( m_endValuesArray[ listIndex ].GetLength() > 0 )      // each element is a comma-separated string, in list box (alphabetical) order
         {
         TCHAR buffer[ 128 ];
         lstrcpy( buffer, m_endValuesArray[ listIndex ] );

         // count commas
         int count = 0;
         LPTSTR next = buffer;
         while( *next != NULL )
            {
            if ( *next == ',' )
               count++;
            next++;
            }

         VData *vDataArray = new VData[ count+1 ];
         int i=0;
         TCHAR *token = _tcstok_s( buffer, _T(",\n"), &next );
         while ( token != NULL )
            {
            vDataArray[ i ].Parse( token );
            i++;
            token = _tcstok_s( NULL, _T( ",\n" ), &next );
            }

         endVDatasArray[ j ] = vDataArray;
         }
      else
         endVDatasArray[ j ] = NULL;
      }

   // at this point, startVDatasArray and endVDatasArray hold VData values to check for


   INT_PTR size = pDA->GetSize();

   CWaitCursor c;

   FILE *fp;
   PCTSTR filename = (PCTSTR)file;
   fopen_s( &fp, filename, "wt" );     // open for writing

   if ( fp == NULL )
      {
      char buffer[ 512 ];
      strcpy_s( buffer, 512, "Couldn't open file " );
      strcat_s( buffer, 512, file );
      AfxMessageBox( buffer, MB_OK );
      return;
      }

   int colIDU = -1;
   if ( isalpha( m_iduField[ 0 ] ) )
      colIDU = pLayer->GetFieldCol( m_iduField );

   if ( colIDU > 0 )
      fprintf( fp, _T("year,%s,col,field,oldValue,newValue,type\n" ), (LPCTSTR) m_iduField );
   else
      fputs( _T("year,idu,col,field,oldValue,newValue,type\n" ), fp );

   INT_PTR count = 0;   

   for ( INT_PTR i=0; i < size; i++ )
      {
      DELTA &delta = pDA->GetAt( i );
      
      // selected only?
      if ( m_selectedOnly && ( pLayer->IsSelected( delta.cell ) == false ) )
         continue;

      // avoid duplicates?
      if ( i > 0 && m_includeDuplicates == false && delta.oldValue.Compare( delta.newValue ) == true )
         continue;

      // in year range?
      if ( m_selectedDates && ( delta.year < m_startYear || delta.year > m_endYear ) )
            continue;

      // checked field?
      if ( ! includeAllFields && includeField[ delta.col ] == false ) // not checked?
         continue;

      // is there field-specific values to look for?
      bool endFound = false;
      if ( endVDatasArray[ delta.col ] != NULL )
         {
         VData *vDataArray = endVDatasArray[ delta.col ];

         for ( int k=0; vDataArray[ k ].GetType() != VT_NULL; k++ )
            {
            VData &data = vDataArray[ k ];

            if ( data == delta.newValue )
               {
               endFound = true;
               break;
               }
            }

         if ( endFound == false )
            continue;
         }

      bool startFound = false;
      if ( endVDatasArray[ delta.col ] != NULL )
         {
         VData *vDataArray = endVDatasArray[ delta.col ];

         for ( int k=0; vDataArray[ k ].GetType() != VT_NULL; k++ )
            {
            VData &data = vDataArray[ k ];

            if ( data == delta.oldValue )
               {
               startFound = true;
               break;
               }
            }

         if ( startFound == false )
            continue;
         }

      // passes, write
      count++;
      CString oldValue( delta.oldValue.GetAsString() );
      CString newValue( delta.newValue.GetAsString() );

      int idu = delta.cell;
      if ( colIDU >= 0 )
         pLayer->GetData( idu, colIDU, idu );

      fprintf_s( fp, _T("%i,%i,%i,%s,%s,%s,%i\n"),
         delta.year, idu, delta.col, gpCellLayer->GetFieldLabel( delta.col ), (LPCTSTR) oldValue, (LPCTSTR) newValue, delta.type );
      }


   for ( int i=0; i < fieldCount; i++ )
      {
      if ( startVDatasArray[ i ] != NULL )
         delete [] startVDatasArray[ i ];

      if ( endVDatasArray[ i ] != NULL )
         delete [] endVDatasArray[ i ];
      }

   // clean up
   delete [] startVDatasArray;
   delete [] endVDatasArray;

   fclose( fp );
   CString msg;
   msg.Format( _T("Successfully wrote %i delta array items to %s" ), count, (LPCTSTR) path );

   MessageBox( msg, "Success", MB_OK );
   Report::InfoMsg( msg );

   delete [] includeField;

   //CDialog::OnOK();
   }

/*
void DeltaExportDlg::SetDefaultPath()
   {
   int run = m_runs.GetCurSel();

   RUN_INFO &ri = gpModel->m_pDataManager->GetRunInfo( run );
   
   nsPath::CPath path = gpDoc->m_iniFile;
   path.RemoveExtension();
   //path.Append( ri.pScenario->m_name );

   CString file( path );
   file.Format( "_DeltaArray_Run%i.csv", run );

   //path.Append( file );
   m_path.SetWindowText( file ); 
   }
*/

void DeltaExportDlg::OnBnClickedAll()
   {
   for ( int i=0; i < m_fields.GetCount(); i++ )
      m_fields.SetCheck( i, 1 );   
   }


void DeltaExportDlg::OnBnClickedNone()
   {
   for ( int i=0; i < m_fields.GetCount(); i++ )
      m_fields.SetCheck( i, 0 );   
   }


void DeltaExportDlg::OnBnClickedSelecteddates()
   {
   BOOL enable = IsDlgButtonChecked( IDC_SELECTEDDATES );
   GetDlgItem( IDC_STARTYEAR )->EnableWindow( enable );
   GetDlgItem( IDC_ENDYEAR )->EnableWindow( enable );
   }



void DeltaExportDlg::OnLbnSelchangeRuns()
   {
   //SetDefaultPath();
   }


void DeltaExportDlg::OnBnClickedUsestartvalue()
   {
   // TODO: Add your control notification handler code here
   }

void DeltaExportDlg::OnBnClickedUseendvalue()
   {
   // TODO: Add your control notification handler code here
   }


void DeltaExportDlg::OnLbnSelchangeFields()
   {
   UpdateData( 1 );

   if ( m_lastField >= 0 )    // m_lastField is currently selected list item
      {
      if ( m_useStartValue )
         m_startValuesArray[ m_lastField ] = m_startValues;

      if ( m_useEndValue )
         m_endValuesArray[ m_lastField ] = m_endValues;
      }

   m_lastField = m_fields.GetCurSel();

   if ( m_lastField >= 0 )
      {
      if ( m_startValuesArray[ m_lastField ].GetLength() == 0 )
         {
         m_startValues.Empty();
         m_useStartValue = FALSE;
         }
      else
         {
         m_startValues = m_startValuesArray[ m_lastField ];
         m_useStartValue = TRUE;
         }

      if ( m_endValuesArray[ m_lastField ].GetLength() == 0 )
         {
         m_endValues.Empty();
         m_useEndValue = FALSE;
         }
      else
         {
         m_endValues = m_endValuesArray[ m_lastField ];
         m_useEndValue = TRUE;
         }
      }
   }
