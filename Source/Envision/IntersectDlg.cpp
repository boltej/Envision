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
// IntersectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "IntersectDlg.h"
#include "afxdialogex.h"

#include "EnvDoc.h"

#include <EnvLoader.h>

#include <Maplayer.h>
#include <MAP.h>
#include <Raster.h>
#include <QueryEngine.h>


extern MapLayer *gpCellLayer;
extern CEnvDoc  *gpDoc;


int RNotify( Raster*, RASTER_NOTIFY_TYPE, int, LONG_PTR, LONG_PTR );  // return 0 to interrupt, anything else to continue

int RNotify( Raster*, RASTER_NOTIFY_TYPE type, int a, LONG_PTR b, LONG_PTR extra )
   {
   IntersectDlg *pDlg = (IntersectDlg*) extra;
   switch( type )
      {/*
      case NT_RASTERIZING:
         if ( a == -1 )
            {
            pDlg->m_status.SetWindowText( "Rasterizing started" );
            int range = (int) b;
            pDlg->m_progress.SetRange32( 0, range );
            }
         else if ( a == -2 )
            pDlg->m_status.SetWindowText( "Rasterizing completed" );
         break;*/

      case NT_INDEXING:
         if ( a == -1 )
            {
            pDlg->m_status.SetWindowText( "Indexing started" );
            int range = (int) b;
            pDlg->m_progress.SetRange32( 0, range );
            }
         else if ( a == -2 )
            pDlg->m_status.SetWindowText( "Indexing completed" );
         else
            {
            CString msg;
            if ( a % 100 == 0 )
               {
               CString msg;
               msg.Format( "Indexing row %i of %i", a, (int) b );
               pDlg->m_status.SetWindowText( msg );
               }
            pDlg->m_progress.SetPos( a );
            }
         break;

      case NT_LOADINGINDEX:
         if ( a == -1 )
            {
            pDlg->m_status.SetWindowText( "Loading Index" );
            int range = (int) b;
            pDlg->m_progress.SetRange32( 0, range );
            }
         else if ( a == -2 )
            pDlg->m_status.SetWindowText( "Index Load completed" );
         else
            {
            if ( a % 100 == 0 )
               {
               CString msg;
               msg.Format( "Loading record %i of %i", a, (int) b );
               pDlg->m_status.SetWindowText( msg );
               }
            pDlg->m_progress.SetPos( a );
            }
         break;
      }

   return 1;
   }


// IntersectDlg dialog

IMPLEMENT_DYNAMIC(IntersectDlg, CDialogEx)

IntersectDlg::IntersectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IntersectDlg::IDD, pParent)
   , m_gridSize(30)
   , m_query(_T(""))
   , m_sourceRasterPath(_T(""))
   , m_indexRasterPath(_T(""))
   , m_partRows(1)
   , m_partCols(1)
   { }

IntersectDlg::~IntersectDlg()
{
}

void IntersectDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Control(pDX, IDC_TARGETLAYER, m_targetLayer);
DDX_Control(pDX, IDC_TARGETFIELD, m_targetField);
DDX_Control(pDX, IDC_INTERSECTLAYER, m_sourceLayer);
DDX_Control(pDX, IDC_MERGEFIELD, m_mergeField);
DDX_Text(pDX, IDC_GRIDSIZE, m_gridSize);
DDX_Control(pDX, IDC_UNITS, m_units);
DDX_Control(pDX, IDC_MEMORY, m_memory);
DDX_Control(pDX, IDC_STATUS, m_status);
DDX_Control(pDX, IDC_PROGRESS, m_progress);
DDX_Text(pDX, IDC_QUERY, m_query);
DDX_Text(pDX, IDC_SOURCERASTERPATH, m_sourceRasterPath);
DDX_Text(pDX, IDC_INDEXRASTERPATH, m_indexRasterPath);
DDX_Control(pDX, IDC_SOURCERASTERPATH2, m_rndxFilename);
DDX_Text(pDX, IDC_ROWS, m_partRows);
DDX_Text(pDX, IDC_COLS, m_partCols);
   }


BEGIN_MESSAGE_MAP(IntersectDlg, CDialogEx)
   ON_CBN_SELCHANGE(IDC_TARGETLAYER, &IntersectDlg::OnCbnSelchangeTargetlayer)
   ON_CBN_SELCHANGE(IDC_INTERSECTLAYER, &IntersectDlg::OnCbnSelchangeIntersectlayer)
   ON_EN_CHANGE(IDC_GRIDSIZE, &IntersectDlg::OnEnChangeGridsize)
   ON_BN_CLICKED(IDC_BROWSE, &IntersectDlg::OnBnClickedBrowse)
   ON_BN_CLICKED(IDC_QUERYBUILDER, &IntersectDlg::OnBnClickedQuerybuilder)
   ON_BN_CLICKED(IDC_BROWSERNDX, &IntersectDlg::OnBnClickedBrowserndx)
   ON_BN_CLICKED(IDC_BROWSESOURCEBMP, &IntersectDlg::OnBnClickedBrowsesourcebmp)
   ON_BN_CLICKED(IDC_BROWSEINDEXBMP, &IntersectDlg::OnBnClickedBrowseindexbmp)
   ON_BN_CLICKED(IDC_USERNDX, &IntersectDlg::OnBnClickedUserndx)

END_MESSAGE_MAP()


// IntersectDlg message handlers

BOOL IntersectDlg::OnInitDialog()
   {
   CDialogEx::OnInitDialog();
   
   Map *pMap = gpCellLayer->GetMapPtr();
   ASSERT( pMap != NULL );

   int layerCount = pMap->GetLayerCount();

   CString msg;
   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );

      if ( pLayer->m_pDbTable != NULL && pLayer->GetType() == LT_POLYGON )
         {
         m_targetLayer.AddString( pLayer->m_name );
         m_sourceLayer.AddString( pLayer->m_name );
         }
      }

   m_targetLayer.SetCurSel( 0 );
   m_sourceLayer.SetCurSel( 0 );

   LoadTargetFields();
   LoadMergeFields(); 

   m_units.SetWindowText( gpCellLayer->m_pMap->GetMapUnitsStr() );
   OnEnChangeGridsize();
   m_status.SetWindowText( "" );

   CheckDlgButton( IDC_MAJORITY, 1 );
   EnableControls();

   return TRUE;
   }


void IntersectDlg::OnCbnSelchangeTargetlayer()
   {
   LoadTargetFields();
   }


void IntersectDlg::OnCbnSelchangeIntersectlayer()
   {
   LoadMergeFields();
   }


void IntersectDlg::OnEnChangeGridsize()
   {
   BOOL trans = true;
   int size = GetDlgItemInt( IDC_GRIDSIZE, &trans, TRUE );

   if ( trans )
      {
      // compute extenst
      Map *pMap = gpCellLayer->GetMapPtr();
      ASSERT( pMap != NULL );

      CString layer;
      m_targetLayer.GetWindowText( layer );

      MapLayer *pLayer = pMap->GetLayer( layer );
      ASSERT( pLayer != NULL );

      REAL xMin, xMax, yMin, yMax;
      pLayer->GetExtents( xMin, xMax, yMin, yMax );
      
      int width  = int( ( xMax - xMin ) / size );   // pixels
      int height = int( ( yMax - yMin ) / size );

      float memory = float( width * height * sizeof( int ) * 3 / 1.0e6 );  // mb

      CString msg;
      msg.Format( "Est. Memory Required: %.2f MB", memory );
      m_memory.SetWindowText( msg );

      msg.Format( "%s (%ix%i)", gpCellLayer->m_pMap->GetMapUnitsStr(), width, height );
      m_units.SetWindowText( msg );
      
      }
   }


void IntersectDlg::OnBnClickedBrowse()
   {
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Shape Files|*.shp|All files|*.*||");

   if ( dlg.DoModal() != IDOK )
      return;

   Map *pMap = gpCellLayer->GetMapPtr();

   CString path( dlg.GetPathName() );
   CString filename( dlg.GetFileTitle() );
   CWaitCursor c;
   EnvLoader loader;
   int layer = loader.LoadLayer( pMap, filename, path, AMLT_SHAPE, 0,0,0, 0, -1, NULL, NULL, false, true );

   if ( layer >= 0 )
      {
      int count= m_sourceLayer.AddString( filename );
      m_sourceLayer.SetCurSel( count );
      LoadMergeFields();
      }

   _chdir( cwd );
   free( cwd );
   }



void IntersectDlg::LoadTargetFields()
   {
   m_targetField.ResetContent();

   CString layer;
   m_targetLayer.GetWindowText( layer );

   Map *pMap = gpCellLayer->GetMapPtr();
   ASSERT( pMap != NULL );

   MapLayer *pLayer = pMap->GetLayer( layer );
   ASSERT( pLayer != NULL );

   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      m_targetField.AddString( pLayer->GetFieldLabel( i ) );

   m_targetField.SetCurSel( 0 );
   }


void IntersectDlg::LoadMergeFields()
   {   
   m_mergeField.ResetContent();

   CString layer;
   m_sourceLayer.GetWindowText( layer );

   Map *pMap = gpCellLayer->GetMapPtr();
   ASSERT( pMap != NULL );

   MapLayer *pLayer = pMap->GetLayer( layer );
   ASSERT( pLayer != NULL );

   for ( int i=0; i < pLayer->GetFieldCount(); i++ )
      {
      TYPE type = pLayer->GetFieldType( i );
      if ( ::IsNumeric( type ) )
         m_mergeField.AddString( pLayer->GetFieldLabel( i ) );
      }

   m_mergeField.SetCurSel( 0 );
   }


void IntersectDlg::OnOK()
   {
   UpdateData( 1 );

   int rule = MAJORITY;
   if ( IsDlgButtonChecked( IDC_SUM ) )
      rule = SUM;
   else if ( IsDlgButtonChecked( IDC_AVERAGE ) )
      rule = AVG;
   else if ( IsDlgButtonChecked( IDC_PERCENTFIELDS ) )
      rule = PERCENTAGES;

   if ( rule == PERCENTAGES )
      {
      MessageBox ("Percentages not currently supported", "Error", MB_OK );
      return;
      }

   Map *pMap = gpCellLayer->GetMapPtr();

   CString layer;
   m_sourceLayer.GetWindowText( layer );
   MapLayer *pSourceLayer = pMap->GetLayer( layer );
   ASSERT( pSourceLayer != NULL );

   m_targetLayer.GetWindowText( layer );
   MapLayer *pTargetLayer = pMap->GetLayer( layer );
   ASSERT( pTargetLayer != NULL );

   CString mergeField;
   m_mergeField.GetWindowText( mergeField );
   int colMerge = pSourceLayer->GetFieldCol( mergeField );
   ASSERT( colMerge >= 0 );

   CString targetField;
   m_targetField.GetWindowText( targetField );
   int colTarget = pTargetLayer->GetFieldCol( targetField );
   if ( colTarget < 0 )
      {
      TYPE type    = pSourceLayer->GetFieldType( colMerge );
      int width    = pSourceLayer->m_pDbTable->GetFieldInfo( colMerge ).width;
      int decimals = pSourceLayer->m_pDbTable->GetFieldInfo( colMerge ).decimals;

      colTarget = pTargetLayer->m_pDbTable->AddField( targetField, type, width, decimals, true );
      }

   ASSERT( colTarget >= 0 );

   // figure out appropriate type for target field.
   TYPE targetType = pTargetLayer->GetFieldType( colTarget );
   TYPE mergeType  = pSourceLayer->GetFieldType( colMerge );

   if ( ( rule == SUM || rule == AVG ) && ! ::IsNumeric( mergeType ) )
      {
      MessageBox( "Cannot apply SUM or AVG rules to non-numeric source types", "Invalid Types", MB_OK );
      return;
      }

   if ( rule == AVG )   // AVG rule implies the target field to be populated is FLOAT
      {
      targetType = TYPE_DOUBLE;
      int width, decimals;
      GetTypeParams( TYPE_DOUBLE, width, decimals );
      pTargetLayer->SetFieldType( colTarget, TYPE_DOUBLE, width, decimals, false );
      }

    DRAW_VALUE drawValue = DV_COLOR;
   if ( ::IsInteger( mergeType ) )
      drawValue = DV_VALUE;

   int ciFlag = CI_RASTER;
   CString indexName;
   if ( IsDlgButtonChecked( IDC_USERNDX ) && m_rndxFilename.GetWindowTextLength() > 0 )
      {
      m_rndxFilename.GetWindowText( indexName );
      ciFlag += CI_POLYINDEX;
      }

   m_currentPart = 0;
   pTargetLayer->ClearSelection();

   if ( m_partRows <= 0 )
      m_partRows = 1;
   if ( m_partCols <= 0 )
      m_partCols = 1;

   if ( ( m_partCols * m_partRows ) > 1 )
      {
      REAL xMin, xMax, yMin, yMax;
      pTargetLayer->GetExtents( xMin, xMax, yMin, yMax );   // virtual
      float rowHeight = (float)( fabs( yMax - yMin ) / m_partRows );
      float rowWidth  = (float)( fabs( xMax - xMin ) / m_partCols );

      for ( int r=0; r < m_partRows; r++ )
         {
         for ( int c=0; c < m_partCols; c++ )
            {
            m_currentPart++;

            REAL _xMin = xMin + (rowWidth*c);
            REAL _xMax = _xMin + rowWidth;

            REAL _yMin = yMin+(rowHeight*r);
            REAL _yMax = _yMin + rowHeight;

            COORD2d ll( _xMin, _yMin );
            COORD2d ur( _xMax, _yMax );
            pTargetLayer->SelectArea( ll, ur, true, false );

            // inflate the rect to fully contain the selected polygons
            int count = pTargetLayer->GetSelectionCount();
            for ( int i=0; i < count; i++ )
               {
               int idu = pTargetLayer->GetSelection( i );
               Poly *pPoly = pTargetLayer->GetPolygon( idu );

               REAL pxMin, pxMax, pyMin, pyMax;         
               pPoly->GetBoundingRect( pxMin, pyMin, pxMax, pyMax );

               if ( pxMin < _xMin ) _xMin = pxMin;
               if ( pxMax > _xMax ) _xMax = pxMax;

               if ( pyMin < _yMin ) _yMin = pyMin;
               if ( pyMax > _yMax ) _yMax = pyMax;
               }

            COORD_RECT rect( _xMin, _yMin, _xMax, _yMax );
            Raster raster( pSourceLayer, pTargetLayer, &rect, false );  // note: pTargetLayer is used to create the index, since we want it's polygons
            //raster.InstallNotifyHandler( RNotify, (LONG_PTR) this );

            if ( ! raster.Rasterize( colMerge, m_gridSize, RT_32BIT, ciFlag, indexName, drawValue ) )
               {
               Report::ErrorMsg( "Unable to create raster - you may need to use a larger grid size", "Error", MB_OK );
               return;
               }

            PopulateTargetLayer( pTargetLayer, pSourceLayer, colTarget, colMerge, targetType, mergeType, rule, &raster );
            pTargetLayer->ClearSelection();

            if ( IsDlgButtonChecked( IDC_SAVESOURCE  ) )
               {
               CString filename( m_sourceRasterPath );
               CString ext;
               ext.Format( "_%i.bmp", m_currentPart );
               filename.Replace( ".bmp", ext );
               raster.SaveDIB( filename );
               }

            if ( IsDlgButtonChecked( IDC_SAVEINDEX ) )
               {
               Raster *pIndex = raster.GetIndexRaster();
               if ( pIndex != NULL )
                  {
                  CString filename( m_indexRasterPath );
                  CString ext;
                  ext.Format( "_%1.bmp", m_currentPart );
                  filename.Replace( ".bmp", ext );
                  pIndex->SaveDIB( filename );
                  }
               }
            }
         }
      }  // end of:  if ( partCount > 1 )
   else
      {  // no parts, rasterize in one swoop
      Raster raster( pSourceLayer, pTargetLayer, NULL, false );  // note: pTargetLayer is used to create the index, since we want it's polygons
      raster.InstallNotifyHandler( RNotify, (LONG_PTR) this );

      if ( ! raster.Rasterize( colMerge, m_gridSize, RT_32BIT, ciFlag, indexName, drawValue ) )
         {
         Report::ErrorMsg( "Unable to create raster - you may need to use a larger grid size or use parts", "Error", MB_OK );
         return;
         }

      if ( IsDlgButtonChecked( IDC_SAVESOURCE  ) )
         raster.SaveDIB( m_sourceRasterPath );
   
      if ( IsDlgButtonChecked( IDC_SAVEINDEX ) )
         {
         Raster *pIndex = raster.GetIndexRaster();
         if ( pIndex != NULL )
            pIndex->SaveDIB( m_indexRasterPath );
         }

      if ( IsDlgButtonChecked( IDC_SAVERNDX ) )
         raster.SaveIndex();

      // raster (and index) created, run through polys and get raster results
      m_status.SetWindowText( "Writing data to target layer" );

      PopulateTargetLayer( pTargetLayer, pSourceLayer, colTarget, colMerge, targetType, mergeType, rule, &raster );
      }

   m_status.SetWindowText( "Completed..." );
   }

/*



void IntersectDlg::OnOK()
   {
   UpdateData( 1 );

   int rule = MAJORITY;
   if ( IsDlgButtonChecked( IDC_SUM ) )
      rule = SUM;
   else if ( IsDlgButtonChecked( IDC_AVERAGE ) )
      rule = AVG;
   else if ( IsDlgButtonChecked( IDC_PERCENTFIELDS ) )
      rule = PERCENTAGES;

   if ( rule == PERCENTAGES )
      {
      MessageBox ("Percentages not currently supported", "Error", MB_OK );
      return;
      }

   Map *pMap = gpCellLayer->GetMapPtr();

   CString layer;
   m_sourceLayer.GetWindowText( layer );
   MapLayer *pSourceLayer = pMap->GetLayer( layer );
   ASSERT( pSourceLayer != NULL );

   m_targetLayer.GetWindowText( layer );
   MapLayer *pTargetLayer = pMap->GetLayer( layer );
   ASSERT( pTargetLayer != NULL );

   CString mergeField;
   m_mergeField.GetWindowText( mergeField );
   int colMerge = pSourceLayer->GetFieldCol( mergeField );
   ASSERT( colMerge >= 0 );

   CString targetField;
   m_targetField.GetWindowText( targetField );
   int colTarget = pTargetLayer->GetFieldCol( targetField );
   if ( colTarget < 0 )
      {
      TYPE type = pSourceLayer->GetFieldType( colMerge );
      colTarget = pTargetLayer->m_pDbTable->AddField( targetField, type, true );
      }

   ASSERT( colTarget >= 0 );

   // figure out appropriate type for target field.
   TYPE targetType = pTargetLayer->GetFieldType( colTarget );
   TYPE mergeType  = pSourceLayer->GetFieldType( colMerge );

   if ( ( rule == SUM || rule == AVG ) && ! ::IsNumeric( mergeType ) )
      {
      MessageBox( "Cannot apply SUM or AVG rules to non-numeric source types", "Invalid Types", MB_OK );
      return;
      }

   if ( rule == AVG )   // AVG rule implies the target field to be populated is FLOAT
      {
      targetType = TYPE_DOUBLE;
      pTargetLayer->SetFieldType( colTarget, TYPE_DOUBLE, false );
      }

   //Raster raster( pSourceLayer, pTargetLayer );  // note: pTargetLayer is used to create the index, since we want it's polygons
   //raster.InstallNotifyHandler( RNotify, (LONG_PTR) this );

   DRAW_VALUE drawValue = DV_COLOR;
   if ( ::IsInteger( mergeType ) )
      drawValue = DV_VALUE;

   int ciFlag = CI_RASTER;
   CString indexName;
   if ( IsDlgButtonChecked( IDC_USERNDX ) && m_rndxFilename.GetWindowTextLength() > 0 )
      {
      m_rndxFilename.GetWindowText( indexName );
      ciFlag += CI_POLYINDEX;
      }

      
   int count = pTargetLayer->GetPolygonCount();
   bool isQuery = false;

   if ( ! m_query.IsEmpty() )
      {
      QueryEngine qe( pTargetLayer );
      Query *pQuery = qe.ParseQuery( m_query );
      count = qe.SelectQuery( pQuery, true, false );
      isQuery = true;
      }
   
   m_progress.SetRange32( 0, count );
   m_progress.SetPos( 0 );
   m_status.SetWindowText( "Starting classification" );






   
   for ( int i=0; i < count; i++ )
      {
      if ( i % 100 == 0 )
         m_progress.SetPos( i );

      int idu = i;
      if ( isQuery )
         idu = pTargetLayer->GetSelection( i );

      Poly *pPoly = pTargetLayer->GetPolygon( idu );
      float xMin, xMax, yMin, yMax;
      pPoly->GetBoundingRect( xMin, yMin, xMax, yMax );
      COORD_RECT rect(xMin, yMin, xMax, yMax );

      Raster raster( pSourceLayer, pTargetLayer, &rect, false );

      if ( ! raster.Rasterize( colMerge, m_gridSize, RT_32BIT, ciFlag, NULL, drawValue ) )
         {
         ErrorMsg( "Unable to create raster - you may need to use a larger grid size", "Error", MB_OK );
         return;
         }

      // else, interpret raster
      PopulateTargetLayerPoly( pTargetLayer, pSourceLayer, colTarget, colMerge, targetType, mergeType, rule, &raster, idu );
      
      if ( IsDlgButtonChecked( IDC_SAVESOURCE  ) )
         raster.SaveDIB( m_sourceRasterPath );
   
      if ( IsDlgButtonChecked( IDC_SAVEINDEX ) )
         {
         Raster *pIndex = raster.GetIndexRaster();
         if ( pIndex != NULL )
            pIndex->SaveDIB( m_indexRasterPath );
         }

      //if ( IsDlgButtonChecked( IDC_SAVERNDX ) )
      //   raster.SaveIndex();
      }

   m_status.SetWindowText( "Completed..." );
   }
   
   */





struct POLYINFO { float fvalue; int ivalue; int *valueCountArray; int pixelCount; };

bool IntersectDlg::PopulateTargetLayer( MapLayer *pTargetLayer, MapLayer *pMergeLayer, int colTarget, int colMerge, TYPE targetType, TYPE mergeType, int rule, Raster *pRaster )
   {
   ASSERT( pRaster != NULL );
   Raster *pIndexRaster = pRaster->GetIndexRaster();
   ASSERT( pIndexRaster != NULL );

   int rows = pRaster->m_rows;
   int cols = pRaster->m_cols;
   float noDataValue = pMergeLayer->GetNoDataValue();
   
   int polyCount = pTargetLayer->GetPolygonCount();
   //if ( pTargetLayer->GetSelectionCount() > 0 )
   //   polyCount = pTargetLayer->GetSelectionCount();

   CMap< int, int, int, int > valueIndexMap;   // stores values as key, index of valueCountArray as map
   int valueCount = 0;
   CUIntArray valueArray;

   if ( rule == MAJORITY )
      {
      ASSERT( ::IsInteger( mergeType ) );

      valueCount = pMergeLayer->GetUniqueValues( colMerge, valueArray );
      ASSERT( valueCount > 0 );

      for ( int i=0; i < valueCount; i++ )
         valueIndexMap.SetAt( valueArray[ i ], i );
      }

   POLYINFO *polyInfoArray = new POLYINFO[ polyCount ];
   for ( int i=0; i < polyCount; i++ )
      {
      polyInfoArray[ i ].fvalue = 0;
      polyInfoArray[ i ].ivalue = 0;
      polyInfoArray[ i ].pixelCount = 0;

      if ( valueCount > 0 )
         {
         polyInfoArray[ i ].valueCountArray = new int[ valueCount ];
         for ( int j=0; j < valueCount; j++ )
            polyInfoArray[ i ].valueCountArray[ j ] = 0;
         }
      else
         polyInfoArray[ i ].valueCountArray = NULL;
      }

   // iterate through rasters, populating polyInfo as needed
   m_progress.SetRange32( 0, rows );
   m_progress.SetPos( 0 );
   CString msg;
   msg.Format( "Extracting values from raster for part %i of %i", m_currentPart, m_partRows*m_partCols );
   m_status.SetWindowText( msg );

   for ( int row=0; row < rows; row++ )
      {
      if ( row % 100 == 0 )
         m_progress.SetPos( row );

      for ( int col=0; col < cols; col++ )
         {
         int polyIndex;
         bool ok = pIndexRaster->GetData( row, col, polyIndex );

         ASSERT( ok );

         if ( polyIndex >= 0 && polyIndex < polyCount )
            {
            POLYINFO &pi = polyInfoArray[ polyIndex ];

            if ( ::IsInteger( mergeType ) )
               {
               int mergeValue;
               ok = pRaster->GetData( row, col, mergeValue );

               if ( ok )
                  {
                  switch( rule )
                     {
                     case MAJORITY:
                        {
                        int valueIndex;
                        BOOL found = valueIndexMap.Lookup( mergeValue, valueIndex );
                        if ( found )
                           pi.valueCountArray[ valueIndex ] += 1;
                        }
                        break;

                     case SUM:
                     case AVG:
                        pi.ivalue += mergeValue;
                        break;
                     }
                  pi.pixelCount++;
                  }
               }  // end of: if IsInteger( mergeType )
            
            else  // mergeType is float type
               {
               float mergeValue;
               ok = pRaster->GetData( row, col, mergeValue );

               if ( ok && mergeValue != noDataValue ) // noData's not include in calcs
                  {
                  switch( rule )
                     {
                     case MAJORITY:
                        ASSERT( 0 );   // shouldnt ever get here

                     case SUM:
                     case AVG:
                        pi.fvalue += mergeValue;
                        break;
                     }

                  pi.pixelCount++;
                  }
               }  // else mergetype == float

            }  // end of: if ( polyIndex in range )
         }  // end of: col < cols
      }  // end of: row < rows

   // done iterating thout raster, clean up
   m_progress.SetRange32( 0, polyCount );
   m_progress.SetPos( 0 );
   msg.Format( "Writing results to target layer for part %i of %i", m_currentPart, m_partRows*m_partCols );
   m_status.SetWindowText( msg );

   bool isQuery = false;
   if ( pTargetLayer->GetSelectionCount() > 0 )
      {
      isQuery = true;
      polyCount = pTargetLayer->GetSelectionCount();
      }
   int previousValue=-99;
   for ( int i=0; i < polyCount; i++ )
      {
      int idu = i;
      if ( isQuery )
         idu = pTargetLayer->GetSelection( i );

      POLYINFO &pi = polyInfoArray[ idu ];

      if ( i % 100 == 0 )
         m_progress.SetPos( i );

      switch( rule )
         {
         case MAJORITY:
            {
            int highestCount = 0;
            int highestIndex = -1;
            for ( int j=0; j < valueCount; j++ )
               {
               if ( pi.valueCountArray[ j ] > highestCount )
                  {
                  highestCount = pi.valueCountArray[ j ];
                  highestIndex = j;
                  }
               }
            
            if ( highestIndex >= 0 )
               {
               pTargetLayer->SetData( idu, colTarget, (int) valueArray[ highestIndex ] );
               previousValue=(int) valueArray[ highestIndex ];
               }
            else  
               //pTargetLayer->SetNoData( idu, colTarget );
               pTargetLayer->SetData( idu, colTarget, previousValue );
            
            delete pi.valueCountArray;
            }
            break;

         case SUM:
         case AVG:
            {
            VData value;
            if ( ::IsInteger( mergeType ) )
               {
               value = pi.ivalue;
               if ( rule == AVG )
                  value = pi.ivalue/pi.pixelCount;
               }
            else
               {
               value = pi.fvalue;
               if ( rule == AVG )
                  value = pi.fvalue/pi.pixelCount;
               }

            value.ChangeType( targetType );
            pTargetLayer->SetData( idu, colTarget, value );
            }
         }  // end of: switch( rule )
      }  // end of: for ( i < polyCount );
   
   delete polyInfoArray;
   return true;
   }





bool IntersectDlg::PopulateTargetLayerPoly( MapLayer *pTargetLayer, MapLayer *pMergeLayer, int colTarget, int colMerge, TYPE targetType, TYPE mergeType, int rule, Raster *pRaster, int polyIndex )
   {
   ASSERT( pRaster != NULL );
   Raster *pIndexRaster = pRaster->GetIndexRaster();
   ASSERT( pIndexRaster != NULL );

   int rows = pRaster->m_rows;
   int cols = pRaster->m_cols;
   float noDataValue = pMergeLayer->GetNoDataValue();

   int polyCount = pTargetLayer->GetPolygonCount();
   CMap< int, int, int, int > valueIndexMap;   // stores values as key, index of valueCountArray as map
   int valueCount = 0;
   CUIntArray valueArray;

   if ( rule == MAJORITY )
      {
      ASSERT( ::IsInteger( mergeType ) );

      valueCount = pMergeLayer->GetUniqueValues( colMerge, valueArray );
      ASSERT( valueCount > 0 );

      for ( int i=0; i < valueCount; i++ )
         valueIndexMap.SetAt( valueArray[ i ], i );
      }

   POLYINFO polyInfo;

   polyInfo.fvalue = 0;
   polyInfo.ivalue = 0;
   polyInfo.pixelCount = 0;

   if ( valueCount > 0 )
      {
      polyInfo.valueCountArray = new int[ valueCount ];
      for ( int j=0; j < valueCount; j++ )
         polyInfo.valueCountArray[ j ] = 0;
      }
   else
      polyInfo.valueCountArray = NULL;

   for ( int row=0; row < rows; row++ )
      {
      for ( int col=0; col < cols; col++ )
         {
         int _polyIndex;
         bool ok = pIndexRaster->GetData( row, col, _polyIndex );
         ASSERT( ok );

         if ( _polyIndex == polyIndex )
            {
            if ( ::IsInteger( mergeType ) )
               {
               int mergeValue;
               ok = pRaster->GetData( row, col, mergeValue );

               if ( ok )
                  {
                  switch( rule )
                     {
                     case MAJORITY:
                        {
                        int valueIndex;
                        BOOL found = valueIndexMap.Lookup( mergeValue, valueIndex );
                        if ( found )
                           polyInfo.valueCountArray[ valueIndex ] += 1;
                        }
                        break;

                     case SUM:
                     case AVG:
                        polyInfo.ivalue += mergeValue;
                        break;
                     }
                  polyInfo.pixelCount++;
                  }
               }  // end of: if IsInteger( mergeType )
            
            else  // mergeType is float type
               {
               float mergeValue;
               ok = pRaster->GetData( row, col, mergeValue );

               if ( ok && mergeValue != noDataValue ) // noData's not include in calcs
                  {
                  switch( rule )
                     {
                     case MAJORITY:
                        ASSERT( 0 );   // shouldnt ever get here

                     case SUM:
                     case AVG:
                        polyInfo.fvalue += mergeValue;
                        break;
                     }

                  polyInfo.pixelCount++;
                  }
               }  // else mergetype == float

            }  // end of: if ( polyIndex in range )
         }  // end of: col < cols
      }  // end of: row < rows

   switch( rule )
      {
      case MAJORITY:
         {
         int highestCount = 0;
         int highestIndex = -1;
         for ( int j=0; j < valueCount; j++ )
            {
            if ( polyInfo.valueCountArray[ j ] > highestCount )
               {
               highestCount = polyInfo.valueCountArray[ j ];
               highestIndex = j;
               }
            }

         if ( highestIndex >= 0 )
            pTargetLayer->SetData( polyIndex, colTarget, (int) valueArray[ highestIndex ] );
         else  
            pTargetLayer->SetNoData( polyIndex, colTarget );

         delete polyInfo.valueCountArray;
         }
         break;

      case SUM:
      case AVG:
         {
         VData value;
         if ( ::IsInteger( mergeType ) )
            {
            value = polyInfo.ivalue;
            if ( rule == AVG )
               value = polyInfo.ivalue/polyInfo.pixelCount;
            }
         else
            {
            value = polyInfo.fvalue;
            if ( rule == AVG )
               value = polyInfo.fvalue/polyInfo.pixelCount;
            }

         value.ChangeType( targetType );
         pTargetLayer->SetData( polyIndex, colTarget, value );
         }  // end of: switch( rule )
      }  // end of: for ( i < polyCount );
   
   return true;
   }














void IntersectDlg::OnBnClickedQuerybuilder()
   {
   // TODO: Add your control notification handler code here
   }


void IntersectDlg::OnBnClickedBrowserndx()
   {
   char *cwd = _getcwd( NULL, 0 );

   CString filename = gpCellLayer->m_path;
   filename.Replace( ".shp", ".rndx" );

   CFileDialog dlg( TRUE, "rndx", filename, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Raster Index Files|*.rndx|All files|*.*||");

   if ( dlg.DoModal() != IDOK )
      return;

   CString path = dlg.GetPathName();
   m_rndxFilename.SetWindowText( path );

   float cellSize;
   CString layerName;
   int polyCount=0, rows=-1, cols=-1;
   bool ok = Raster::GetIndexHeader( path, cellSize, layerName, polyCount, rows, cols );
   if ( ok )
      {
      int targetPolyCount = 0;

      Map *pMap = gpCellLayer->GetMapPtr();

      CString layer;
      m_targetLayer.GetWindowText( layer );
      MapLayer *pTargetLayer = pMap->GetLayer( layer );

      if ( pTargetLayer != NULL )
         targetPolyCount = pTargetLayer->GetPolygonCount();

      if ( polyCount != targetPolyCount )
         {
         CString msg;
         msg.Format( "Polygon count mismatch between Target layer (%i) and Raster index (%i).  The index is not valid for this layer", targetPolyCount, polyCount );
         MessageBox( msg, "Warning", MB_OK );
         }

      if ( pTargetLayer != NULL )
         {
         if ( layerName.CompareNoCase( pTargetLayer->m_path ) != 0 )
            {
            CString msg;
            msg.Format( "Target path (%s) does not match path used to generate Raster Index (%s)", pTargetLayer->m_path, layerName );
            MessageBox( msg, "Warning", MB_OK );
            }
         }

      SetDlgItemInt( IDC_GRIDSIZE, (int) cellSize );
      OnEnChangeGridsize();
      CheckDlgButton( IDC_USERNDX, 1 );
      }

   _chdir( cwd );
   free( cwd );
   }


void IntersectDlg::OnBnClickedBrowsesourcebmp()
   {
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( TRUE, "bmp", "source.bmp", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Bitmap Files|*.bmp|All files|*.*||");

   if ( dlg.DoModal() != IDOK )
      return;

   CWnd *pWnd = GetDlgItem( IDC_SOURCERASTERPATH );
   pWnd->SetWindowText( dlg.GetPathName() );

   _chdir( cwd );
   free( cwd );
   }


void IntersectDlg::OnBnClickedBrowseindexbmp()
   {
   char *cwd = _getcwd( NULL, 0 );

   CFileDialog dlg( TRUE, "bmp", "index.bmp", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Bitmap Files|*.bmp|All files|*.*||");

   if ( dlg.DoModal() != IDOK )
      return;

   CWnd *pWnd = GetDlgItem( IDC_INDEXRASTERPATH );
   pWnd->SetWindowText( dlg.GetPathName() );

   _chdir( cwd );
   free( cwd );
   }


void IntersectDlg::OnBnClickedUserndx()
   {
   UpdateData( 1 );
   if ( IsDlgButtonChecked( IDC_USERNDX ) )
      {
      if ( m_rndxFilename.GetWindowTextLength() == 0 )
         OnBnClickedBrowserndx();
      }

   EnableControls();     
   }


void IntersectDlg::EnableControls( void )
   { 
   if ( IsDlgButtonChecked( IDC_USERNDX ) )
      m_rndxFilename.EnableWindow( 1 );
   else
      m_rndxFilename.EnableWindow( 0 );

   }
