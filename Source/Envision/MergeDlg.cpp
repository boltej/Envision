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
// MergeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "EnvDoc.h"
#include "MapPanel.h"
#include ".\mergedlg.h"

#include <EnvLoader.h>

#include "mainfrm.h"
#include "selectTableDlg.h"
#include "AddDbColDlg.h"

#include <map.h>

#include <direct.h>

extern CMainFrame *gpMain;
extern CEnvDoc    *gpDoc;
extern MapPanel   *gpMapPanel;


// MergeDlg dialog

IMPLEMENT_DYNAMIC(MergeDlg, CDialog)
MergeDlg::MergeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(MergeDlg::IDD, pParent)
   , m_threshold(0)
   , m_pSpatialJoinLayer( NULL )
   , m_pJoinTable( NULL )
   , m_deleteJoinTable( false )
   { }


MergeDlg::~MergeDlg()
   { 
   if ( m_pJoinTable != NULL && m_deleteJoinTable )
      delete m_pJoinTable;
   }


void MergeDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LAYERS, m_targetLayerCombo);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Control(pDX, IDC_DATABASE, m_database);
DDX_Control(pDX, IDC_TARGETFIELD, m_joinField);
DDX_Control(pDX, IDC_SOURCEFIELD, m_joinFieldSource);
DDX_Text(pDX, IDC_THRESHOLD, m_threshold);
DDV_MinMaxFloat(pDX, m_threshold, 0, 1000);
DDX_Control(pDX, IDC_TARGETFIELDTOPOPULATE, m_targetFieldToPopulate);
DDX_Control(pDX, IDC_MERGEINDEXFIELD, m_mergeIndexField);
DDX_Control(pDX, IDC_TARGETINDEXFIELD, m_targetIndexField);
DDX_Control(pDX, IDC_STATUS, m_status);
DDX_Control(pDX, IDC_PROGRESS, m_progress);
   }


BEGIN_MESSAGE_MAP(MergeDlg, CDialog)
   ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
   ON_BN_CLICKED(IDOK, OnBnClickedOk)
   ON_CBN_SELCHANGE(IDC_LAYERS, &MergeDlg::OnCbnSelchangeLayers)
   ON_BN_CLICKED(IDC_ADDFIELD, &MergeDlg::OnBnClickedAddfield)
END_MESSAGE_MAP()


// MergeDlg message handlers
BOOL MergeDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();
   CheckDlgButton( IDC_OVERWRITE, TRUE );

   int layerCount = gpMapPanel->m_pMap->GetLayerCount();

   CString msg;
   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( i );

      if ( pLayer->m_pDbTable != NULL )
         m_targetLayerCombo.AddString( pLayer->m_name );
      }

   m_targetLayerCombo.SetCurSel( 0 );

   LoadJoinFields();
   return TRUE;
   }


void MergeDlg::LoadJoinFields()
   {
   m_joinField.ResetContent();
   m_targetIndexField.ResetContent();
   m_targetFieldToPopulate.ResetContent();

   int index = m_targetLayerCombo.GetCurSel();

   if ( index < 0 )
      return;

   MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( index );

   m_targetFieldToPopulate.AddString( "<Use Merge Field>" );

   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      {
      m_joinField.AddString( pLayer->GetFieldLabel( i ) );
      m_targetIndexField.AddString( pLayer->GetFieldLabel( i ) );
      m_targetFieldToPopulate.AddString( pLayer->GetFieldLabel( i ) );
      }
   
   m_joinField.SetCurSel( 0 );   
   m_targetIndexField.SetCurSel( 0 );
   m_targetFieldToPopulate.SetCurSel( 0 );

   return; 
   }


void MergeDlg::OnCbnSelchangeLayers()
   {
   LoadJoinFields();
   }



void MergeDlg::OnBnClickedBrowse()
   {
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "DBase files|*.dbf|Access Files|*.mdb|CSV Files|*.csv|Shape Files|*.shp|All files|*.*||");

   if ( dlg.DoModal() != IDOK )
      return;

   CString filename( dlg.GetPathName() );

   // load table
   if ( m_pJoinTable != NULL )
      delete m_pJoinTable;

   bool useWide = VData::SetUseWideChar( true );

   CString ext = dlg.GetFileExt();

   int count = -1;

   if ( ext.CompareNoCase( "mdb" ) == 0 )
      {
      SelectTableDlg sel( filename, this );

      if ( sel.DoModal() == IDOK )
         {
         CString sql( "SELECT * FROM " );
         sql += sel.m_table;

         CWaitCursor c;
         m_pJoinTable = new DbTable;
         m_deleteJoinTable = true;
         ///// count = m_pJoinTable->Load( filename, sql, 0, 1, "" );
         }
      }
   else if ( ext.CompareNoCase( "dbf" ) == 0 )
      {
      CWaitCursor c;
      m_pJoinTable = new DbTable;
      m_deleteJoinTable = true;
      ///// count = m_pJoinTable->LoadDataDBF( filename );
      }
   else if ( ext.CompareNoCase( "csv" ) == 0 )
      {
      CWaitCursor c;
      m_pJoinTable = new DbTable;
      m_deleteJoinTable = true;
      count = m_pJoinTable->LoadDataCSV( filename );
      }

   else if ( ext.CompareNoCase( "shp" ) == 0 )
      {
      CWaitCursor c;
      EnvLoader loader;
      int layer = loader.LoadLayer( gpMapPanel->m_pMap, _T("SpatialJoinLayer"), filename, AMLT_SHAPE, 0,0,0, 0, -1, NULL, NULL, false, true,0 );

      if ( layer >= 0 )
         {
         m_pSpatialJoinLayer = gpMapPanel->m_pMap->GetLayer( layer );
         m_pJoinTable = m_pSpatialJoinLayer->m_pDbTable;
         m_deleteJoinTable = false;
         count = m_pJoinTable->GetRecordCount();
   
         //_mergeIndexField.ResetContent();

//         for ( int i=0; i < m_pSpatialJoinLayer->GetFieldCount(); i++ )
//            m_mergeIndexField.AddString( m_pSpatialJoinLayer->GetFieldLabel( i ) );

//         m_mergeIndexField.SetCurSel( 0 );
         }
      }
   else
      {
      MessageBox( "Unrecognized database type" );
      VData::SetUseWideChar( useWide );
      return;
      }
   
   VData::SetUseWideChar( useWide );
   
   if ( count <= 0 )
      {
      Report::ErrorMsg( "Unable to load file to merge" );
      _chdir( cwd );
      free( cwd );
      return;
      }

   m_database.SetWindowText( filename );

   ASSERT( m_pJoinTable != NULL );
   DataObj *pData = m_pJoinTable->m_pData;
   ASSERT( pData->GetColCount() == m_pJoinTable->GetColCount() );

   CWaitCursor c;
   m_fields.ResetContent();
   for ( int i=0; i < pData->GetColCount(); i++ )
      {
      m_fields.AddString( pData->GetLabel( i ) );
      m_joinFieldSource.AddString( pData->GetLabel( i ) );
      }

   // add merge index field names to combo
   m_mergeIndexField.ResetContent();

   for ( int i=0; i < m_pJoinTable->GetColCount(); i++ )
      m_mergeIndexField.AddString( m_pJoinTable->m_pData->GetLabel( i ) );

   m_mergeIndexField.SetCurSel( 0 );
   
   _chdir( cwd );
   free( cwd );
   }


void MergeDlg::OnBnClickedOk()
   {
   UpdateData( 1 );

   CWaitCursor c;

   bool overwrite = IsDlgButtonChecked( IDC_OVERWRITE ) ? true : false;
   bool attrJoin  = IsDlgButtonChecked( IDC_ATTR ) ? true : false;
   bool spatialJoin  = IsDlgButtonChecked( IDC_SPATIAL ) ? true : false;

   bool popTargetIndexIntoMergeTable = IsDlgButtonChecked( IDC_POPTARGETINDEX ) ? true : false;
   bool popMergeIndexIntoTargetTable  = IsDlgButtonChecked( IDC_POPMERGEINDEX ) ? true : false;
   bool popTargetLayer = IsDlgButtonChecked( IDC_POPTARGETLAYER ) ? true : false;
   
   // Get existing layer to merge with
   CString layerName;
   m_targetLayerCombo.GetWindowText( layerName );
   MapLayer *pTargetLayer = gpMapPanel->m_pMap->GetLayer( layerName );

   DbTable *pTargetTable = pTargetLayer->m_pDbTable;

   char joinField[ 32 ], joinFieldSource[ 32 ], mergeIndexField[ 32 ], targetIndexField[ 32 ];
   int colJoin = -1, colJoinSource = -1, colMergeIndex = -1, colTargetIndex = -1;

   if ( ( popTargetIndexIntoMergeTable || popMergeIndexIntoTargetTable ) && !( attrJoin || spatialJoin ) )
      {
      MessageBox( "Either an attribute join or a spatial join must be specified when populating indexes", "Specify Join Type", MB_OK );
      return;
      }

   //  SET UP ATTRIBUTE JOIN
   if ( attrJoin )
      {
      m_joinField.GetLBText( m_joinField.GetCurSel(), joinField );
      m_joinFieldSource.GetLBText( m_joinFieldSource.GetCurSel(), joinFieldSource );

      colJoin = pTargetTable->GetFieldCol( joinField );
      colJoinSource = m_pJoinTable->GetFieldCol( joinFieldSource );
      }

   // SET UP SPATIAL JOIN 
   if ( spatialJoin )
      {
      if ( m_pSpatialJoinLayer == NULL )
         {
         AfxMessageBox( "No shapefile loaded - you must specify a shape file in the data source to perform a spatial join" );
         return;
         }
      }
      
   // SETUP INDEX JOINS
   if ( popTargetIndexIntoMergeTable )
      {
      CString field;
      m_mergeIndexField.GetWindowText( field );
      int col = m_pJoinTable->GetFieldCol( field );

      if ( col < 0 )
         {
         CString msg;
         msg.Format( "Target Index column [%s] not found in merge database - It will be added... ", (LPCTSTR) mergeIndexField );
         MessageBox( msg, "Adding Field" );

         CWaitCursor c;
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         col = m_pJoinTable->AddField( field, TYPE_INT, width, decimals, true );
         }

      lstrcpy( mergeIndexField, field );         
      colMergeIndex = m_pJoinTable->GetFieldCol( mergeIndexField );
      ASSERT( colMergeIndex >= 0 );
      m_pJoinTable->SetColData( colMergeIndex, VData(  pTargetLayer->GetNoDataValue() ) ); 

      int width, decimals;
      GetTypeParams( TYPE_INT, width, decimals );
      m_pJoinTable->SetFieldType( colMergeIndex, TYPE_INT, width, decimals, true );
      }

   if ( popMergeIndexIntoTargetTable )
      {
      CString field;
      m_targetIndexField.GetWindowText( field );
      int col = pTargetLayer->GetFieldCol( field );

      if ( col < 0 )
         {
         CString msg;
         msg.Format( "Merge Index column [%s] not found in target database - It will be added... ", (LPCTSTR) targetIndexField );
         MessageBox( msg, "Adding Field" );

         CWaitCursor c;
         int width, decimals;
         GetTypeParams( TYPE_INT, width, decimals );
         col = pTargetLayer->m_pDbTable->AddField( field, TYPE_INT, width, decimals, true );
         }

      lstrcpy( targetIndexField, field );         
      colTargetIndex = pTargetLayer->GetFieldCol( targetIndexField );
      ASSERT( colTargetIndex >= 0 );
      pTargetLayer->SetColNoData( colTargetIndex );

      int width, decimals;
      GetTypeParams( TYPE_INT, width, decimals );
      pTargetLayer->SetFieldType( colTargetIndex, TYPE_INT, width, decimals, true );
      }

   int targetRecordCount = pTargetLayer->GetRecordCount();
   int joinRecordCount = m_pJoinTable->GetRecordCount(); 

   if ( ! ( attrJoin || spatialJoin ) && ( targetRecordCount != joinRecordCount ) )
      {
      Report::WarningMsg( "Mismatched record counts...fields not merged" );
      return;
      }

   int colTargetField = m_targetFieldToPopulate.GetCurSel() - 1;
   int countFieldsToMerge = GetCountFieldsToMerge();
   if ( popTargetLayer && countFieldsToMerge > 1 && colTargetField >= 0 )
      {
      MessageBox( "Target Field to populate mush be set to <Use Merge Field> if more than one field being populated.  Please correct before continuing...", "Unable to continue" );
      return;
      } 

   // all set, start joining....
   m_progress.SetRange32( 0, targetRecordCount );

   for ( int i=0; i < m_fields.GetCount(); i++ )
      {
      if ( m_fields.GetCheck( i ) == 1 )
         {
         CString label;
         m_fields.GetText( i, label );
         
         CString status( "Copying records for field " );
         status += label;
         m_status.SetWindowText( status );

         int cellCol = colTargetField;
         if ( cellCol < 0 )
            cellCol = pTargetLayer->GetFieldCol( label );   // colum in target layer to populate

         TYPE type = m_pJoinTable->GetFieldType( i );
         int width = m_pJoinTable->GetFieldInfo( i ).width;
         int decimals = m_pJoinTable->GetFieldInfo( i ).decimals;

         if ( cellCol < 0 ) // column doesn't exist, so added it
            {
            CString msg;
            msg.Format( "The Target field [%s] doesn't exist.  It will be added...", (LPCTSTR) label );
            MessageBox( msg );
            cellCol = pTargetTable->AddField( label, type, width, decimals, true );
            }

         else  // it already exists
            {
            if ( overwrite == false )
               {
               CString msg;
               msg.Format( "The field [%s] already exists in the IDU coverage.  Do you want to overwrite it?", (LPCTSTR) label );
               int retVal = this->MessageBox( msg, "Overwrite Field?", MB_YESNOCANCEL );

               switch( retVal )
                  {
                  case IDYES:
                     break;

                  case IDNO:
                     continue;

                  case IDCANCEL:
                     return;
                  }
               }

            else if ( attrJoin && ( colJoin == cellCol ) )
               {
               MessageBox( _T("You are doing an Attribute Join but indicated you want to overwrite the join field. This will result in an empty join, so the join field will not be overwritten."), _T("Warning"), MB_OK);
               continue;
               }

            VData nullValue;
            pTargetLayer->SetColData( cellCol, nullValue, true );
            int width, decimals;
            GetTypeParams( type, width, decimals );
            pTargetTable->SetFieldType( cellCol, type, width, decimals, false ); // make sure target field is of the correct type
            }

         // have appropriate cell col, start writing
         int count = 0;
         VData vTarget;
         VData vSource;

         for ( int j=0; j < targetRecordCount; j++ )
            {
            if ( j % 100 == 0 )
               m_progress.SetPos( j );

            VData value;

            if ( attrJoin )
               {
               // find corresponding record in the source table that matches the join attribute in the target table
               pTargetLayer->GetData( j, colJoin, vTarget );

               int sourceRecord = m_pJoinTable->FindData( colJoinSource, vTarget );

               if ( sourceRecord >= 0 )   // found match?
                  {
                  m_pJoinTable->GetData( sourceRecord, i, value );
                  pTargetLayer->SetData( j, cellCol, value );

                  //if ( popTargetIndexIntoMergeTable )
                  //   m_pJoinTable->SetData( sourceRecord, colMergeIndex, j ); // put target index into merge layer db

                  //if ( popMergeIndexIntoTargetTable )
                  //   pTargetLayer->SetData( j, colTargetIndex, sourceRecord );   
                  }
               }

            else if ( spatialJoin && popTargetLayer )
               {
               Poly *pPoly = pTargetLayer->GetPolygon( j );
               ASSERT( pPoly != NULL );

               Vertex v = pPoly->GetCentroid();
               int index = -1;
               m_pSpatialJoinLayer->GetPolygonFromCentroid( v.x, v.y, m_threshold, &index );

               if ( index >= 0 )
                  {
                  //if ( popMergeIndexIntoTargetTable )    // in target index database
                  //   pTargetLayer->SetData( j, colTargetIndex, index );

                  //if ( popTargetIndexIntoMergeTable )   // in merge layer database
                  //   m_pSpatialJoinLayer->SetData( index, colMergeIndex, j );                     

                  m_pSpatialJoinLayer->GetData( index, i, value );
                  pTargetLayer->SetData( j, cellCol, value );
                  }
               }

            else
               {
               m_pJoinTable->GetData( j, i, value );     // source data
               pTargetLayer->SetData( j, cellCol, value );
               }
            }  // end of: for ( j < count )
         }  // end of:  if ( checked )
      }  // end of: for ( i < fields )


   if ( spatialJoin && ( popMergeIndexIntoTargetTable || popTargetIndexIntoMergeTable )  )
      {
      if ( popMergeIndexIntoTargetTable )
         {
         //pTargetLayer->SetColNoData( colTargetIndex );

         m_status.SetWindowText( "Populating Target Layer with Merge Layer index" );
         m_progress.SetRange32( 0, targetRecordCount );

         for ( int j=0; j < targetRecordCount; j++ )
            {
            if ( j % 100 == 0 )
               m_progress.SetPos( j );

            Poly *pPoly = pTargetLayer->GetPolygon( j );
            ASSERT( pPoly != NULL );

            Vertex v = pPoly->GetCentroid();
            int index = -1;
            m_pSpatialJoinLayer->GetPolygonFromCentroid( v.x, v.y, m_threshold, &index );

            if ( index >= 0 )
               pTargetLayer->SetData( j, colTargetIndex, index );
            }
         }

      if ( popTargetIndexIntoMergeTable )
         {
         //m_pSpatialJoinLayer->SetColNoData( colMergeIndex );

         m_status.SetWindowText( "Populating Merge Layer with Target Layer index" );
         m_progress.SetRange32( 0, joinRecordCount );

         for ( int j=0; j < joinRecordCount; j++ )
            {
            if ( j % 100 == 0 )
               m_progress.SetPos( j );

            Poly *pPoly = m_pSpatialJoinLayer->GetPolygon( j );
            ASSERT( pPoly != NULL );

            Vertex v = pPoly->GetCentroid();
            int index = -1;
            pTargetLayer->GetPolygonFromCentroid( v.x, v.y, m_threshold, &index );

            if ( index >= 0 )
               m_pSpatialJoinLayer->SetData( j, colMergeIndex, index );
            }
         }
      }  // end of:  if ( spatialJoin && ( popMergeIndexIntoTargetTable || popTargetIndexIntoMergeTable )  )        

   if ( attrJoin && ( popMergeIndexIntoTargetTable || popTargetIndexIntoMergeTable )  )
      {
      if ( popMergeIndexIntoTargetTable )
         {
         m_status.SetWindowText( "Populating Target Layer with Merge Layer index" );
         m_progress.SetRange32( 0, targetRecordCount );
         
         float noDataValue = pTargetLayer->GetNoDataValue();

         for ( int j=0; j < targetRecordCount; j++ )
            {
            if ( j % 100 == 0 )
               m_progress.SetPos( j );

            // find corresponding record in the source table that matches the join attribute in the target table
            VData vTarget;
            pTargetLayer->GetData( j, colJoin, vTarget );

            float value;
            vTarget.GetAsFloat( value );
            if ( value != noDataValue )
               {
               int sourceRecord = m_pJoinTable->FindData( colJoinSource, vTarget );

               if ( sourceRecord >= 0 )   // found match?
                  pTargetLayer->SetData( j, colTargetIndex, sourceRecord );                  
               }
            }
         }

      if ( popTargetIndexIntoMergeTable )
         {
         m_status.SetWindowText( "Populating Merge Layer with Target Layer index" );
         m_progress.SetRange32( 0, joinRecordCount );

         float noDataValue = pTargetLayer->GetNoDataValue();

         for ( int j=0; j < joinRecordCount; j++ )
            {
            if ( j % 100 == 0 )
               m_progress.SetPos( j );
   
            // find corresponding record in the source table that matches the join attribute in the target table
            VData vTarget;
            m_pJoinTable->GetData( j, colJoinSource, vTarget );

            float value;
            vTarget.GetAsFloat( value );
            if ( value != noDataValue )
               {
               int targetRecord = pTargetLayer->m_pDbTable->FindData( colJoin, vTarget );

               if ( targetRecord >= 0 )   // found match?
                  m_pJoinTable->SetData( j, colMergeIndex, targetRecord );
               }
            }
         }
      }
   
   MessageBox( "Merge completed" );

   char *cwd = _getcwd( NULL, 0 );

   bool targetLayerChanged = false;
   bool mergeLayerChanged  = false;

   if ( ! (attrJoin || spatialJoin ) )
      targetLayerChanged = true;

   if ( attrJoin )
      targetLayerChanged = true; 
   
   if ( spatialJoin && ( popTargetLayer || popMergeIndexIntoTargetTable ) )
      targetLayerChanged = true;

   if ( popTargetIndexIntoMergeTable )
      mergeLayerChanged = true;
   
   if( targetLayerChanged )
      {
      CWaitCursor c;

      CFileDialog dlg( FALSE, "dbf", pTargetLayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "DBase Files|*.dbf|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         {
         CString filename( dlg.GetPathName() );

         bool useWide = VData::SetUseWideChar( true );

         m_status.SetWindowText( "Saving Database..." );

         pTargetLayer->SaveDataDB( filename );

         VData::SetUseWideChar( useWide );
 
         gpDoc->SetChanged( 0 );
         }
      else
         gpDoc->SetChanged( CHANGED_COVERAGE );
      }

   if ( mergeLayerChanged )
      {
      CFileDialog dlg( FALSE, "dbf", m_pJoinTable->m_tableName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "DBase Files|*.dbf|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         {
         CString filename( dlg.GetPathName() );

         bool useWide = VData::SetUseWideChar( true );

         m_status.SetWindowText( "Saving Database..." );

         m_pJoinTable->SaveDataDBF( filename );

         VData::SetUseWideChar( useWide );
         }
      }

   _chdir( cwd );
   free( cwd );

   m_status.SetWindowText( "Done..." );

   GetDlgItem( IDCANCEL )->SetWindowText( "Close" );

   //OnOK();
   }

   
void MergeDlg::OnBnClickedAddfield()
   {
   AddDbColDlg dlg;
   dlg.DoModal();
   
   m_targetFieldToPopulate.ResetContent();

   int index = m_targetLayerCombo.GetCurSel();

   if ( index < 0 )
      return;

   MapLayer *pLayer = gpMapPanel->m_pMap->GetLayer( index );

   m_targetFieldToPopulate.AddString( "<Use Merge Field>" );
   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      m_targetFieldToPopulate.AddString( pLayer->GetFieldLabel( i ) );
   
   m_targetFieldToPopulate.SetCurSel( 0 );
   }


int MergeDlg::GetCountFieldsToMerge()
   {
   int count = 0;

   for ( int i=0; i < m_fields.GetCount(); i++ )
      {
      if ( m_fields.GetCheck( i ) == 1 )
         count++;
      }

   return count;
   }