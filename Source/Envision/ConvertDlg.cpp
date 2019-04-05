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
// ConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "ConvertDlg.h"
#include "afxdialogex.h"

#include <MAP.h>
#include <Maplayer.h>
#include <VDATAOBJ.H>
#include <DirPlaceholder.h>
#include <Report.h>
#include <VoronoiDiagramGenerator.h>

#include "EnvView.h"


extern CEnvView *gpView;



// ConvertDlg dialog

IMPLEMENT_DYNAMIC(ConvertDlg, CDialogEx)

ConvertDlg::ConvertDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(ConvertDlg::IDD, pParent)
   , m_hexCSVInputFile(_T(""))
   , m_hexSpacing(0)
   {

}

ConvertDlg::~ConvertDlg()
{
}




void ConvertDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Text(pDX, IDC_HEXCSV_INPUT, m_hexCSVInputFile);
DDX_Control(pDX, IDC_LABEL_HEXCSVFILE, m_labelHexCSVFile);
DDX_Control(pDX, IDC_HEXCSV_INPUT, m_wndHexCSVInput);
DDX_Control(pDX, IDC_BROWSEHEXCSV, m_wndBrowseHexCSV);
DDX_Control(pDX, IDC_LABEL_XCOL, m_labelXCol);
DDX_Control(pDX, IDC_LABEL_YCOL, m_labelYCol);
DDX_Control(pDX, IDC_XCOL, m_xCols);
DDX_Control(pDX, IDC_YCOL, m_yCols);
DDX_Text(pDX, IDC_HEXRADIUS, m_hexSpacing);
DDV_MinMaxFloat(pDX, m_hexSpacing, 0, 5000);
DDX_Control(pDX, IDC_LAYERS, m_layers);
   }


BEGIN_MESSAGE_MAP(ConvertDlg, CDialogEx)
   ON_BN_CLICKED(IDC_HEXFROMPOINTS, &ConvertDlg::OnBnClickedHexfrompoints)
   ON_BN_CLICKED(IDC_CENTROIDS_CSV, &ConvertDlg::OnBnClickedCentroidsCsv)
   ON_BN_CLICKED(IDC_BROWSEHEXCSV, &ConvertDlg::OnBnClickedBrowsehexcsv)
END_MESSAGE_MAP()


// ConvertDlg message handlers
BOOL ConvertDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   // load layers
   Map *pMap = gpView->m_mapPanel.m_pMap;

   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      m_layers.AddString( pMap->GetLayer( i )->m_name );

   m_layers.SetCurSel( 0 );
   
   return TRUE;
   }

void ConvertDlg::OnOK()
   {
   if ( IsDlgButtonChecked( IDC_HEXFROMPOINTS ) )
      {
      if ( GenerateHexFromPoints() )
         CDialogEx::OnOK(); 
      }

   if ( IsDlgButtonChecked( IDC_CENTROIDS_CSV ) )
      {
      if ( GenerateCentroidsFromLayer() )
         CDialogEx::OnOK();
      }

   if ( IsDlgButtonChecked( IDC_VOROINI ) )
      {
      if ( GenerateVoroiniFromLayer() )
         CDialogEx::OnOK();
      }
   }


void ConvertDlg::OnBnClickedHexfrompoints()
   {
   // TODO: Add your control notification handler code here
   }


void ConvertDlg::OnBnClickedCentroidsCsv()
   {
   // TODO: Add your control notification handler code here
   }


void ConvertDlg::OnBnClickedBrowsehexcsv()
   {
   DirPlaceholder d;
   CFileDialog dlg( TRUE, "csv", "", OFN_HIDEREADONLY, "CSV files|*.csv|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString path = dlg.GetPathName();

      VDataObj input;
      int rows = input.ReadAscii( path, ',' );

      if ( rows <= 0 )
         {
         AfxMessageBox( "Error reading CSV file - invalid format" );
         return;
         }
      
      m_hexCSVInputFile = path;
      UpdateData( 0 );

      m_labelXCol.EnableWindow();
      m_labelYCol.EnableWindow();
      m_xCols.EnableWindow();
      m_yCols.EnableWindow();

      m_xCols.ResetContent();
      m_yCols.ResetContent();

      for ( int i=0; i < input.GetColCount(); i++ )
         {
         m_xCols.AddString( input.GetLabel( i ) );
         m_yCols.AddString( input.GetLabel( i ) );
         }

      m_xCols.SetCurSel( 0 );
      m_yCols.SetCurSel( 0 );
      }
   }


bool ConvertDlg::GenerateHexFromPoints()
   {
   // load input csv file
   UpdateData();

   if ( m_hexCSVInputFile.IsEmpty() )
      {
      AfxMessageBox( "Must specify an input file!" );
      return false;
      }

   VDataObj input;
   int hexCount = input.ReadAscii( m_hexCSVInputFile );
   
   int xCol = m_xCols.GetCurSel();
   int yCol = m_yCols.GetCurSel();

   /*
   // infer rows and cols from data.  Get cols by scanning first row until y coord changes
   // note: we assume that the grid is arranged in col order, meaning the first column 
   // of center points comes first, then the second col, etc...
   //int cols = 1;
   int rows = 1;
   float lastX = 0;
   float lastY = 0;

   for ( int i=0; i < hexCount; i++ )
      {
      float x = input.GetAsFloat( xCol, i ); 
      float y = input.GetAsFloat( yCol, i );

      if ( i == 0 )
         {
         lastX = x;
         lastY = y;
         }

      if ( x != lastX )
         break;

      lastX = x;
      rows++;
      }

   if ( hexCount % rows != 0 )
      {
      AfxMessageBox( "Invalid hex geometry - not a regular grid!" );
      return false;
      }

   int cols = hexCount / rows;
   
   // get spacing
   float y0 = input.GetAsFloat( yCol, 0 );
   float y1 = input.GetAsFloat( yCol, 1 );

   float dy     = (float) fabs( y1-y0 );
   float radius = float( dy / sqrt( 3.0 ) );
   */


   CString msg;
   msg.Format( "%i hex centerpoints loaded (radius=%f)", hexCount, (float) m_hexSpacing );
   AfxMessageBox( msg );

   // dimensions figured, generate polygons
   Map *pMap = gpView->m_mapPanel.m_pMap;
   MapLayer *pLayer = pMap->AddLayer( LT_POLYGON );           // add an empty maplayer

   pLayer->m_name = m_hexCSVInputFile;

   float dy = m_hexSpacing;  // /2;
   float radius = float( dy / sqrt( 3.0 ) );
   ///////


   for ( int i=0; i < hexCount; i++ )
      {
      float xCenter = input.GetAsFloat( xCol, i );
      float yCenter = input.GetAsFloat( yCol, i );

      Poly *pPoly = new Poly;  // memory is managed by map layer once the poly is added
      
      pPoly->AddVertex( Vertex( xCenter+radius,   yCenter ) );
      pPoly->AddVertex( Vertex( xCenter+radius/2, yCenter-dy/2 ) );
      pPoly->AddVertex( Vertex( xCenter-radius/2, yCenter-dy/2 ) );
      pPoly->AddVertex( Vertex( xCenter-radius,   yCenter ) );
      pPoly->AddVertex( Vertex( xCenter-radius/2, yCenter+dy/2 ) );
      pPoly->AddVertex( Vertex( xCenter+radius/2, yCenter+dy/2 ) );
      pPoly->AddVertex( Vertex( xCenter+radius,   yCenter ) );
      
      pLayer->AddPolygon( pPoly, false );   // don't update extents, we'll do this at the end

      if ( i % 100 == 0 )
         {
         CString msg;
         msg.Format( "Generating Hex Poly %i of %i", i, hexCount );
         Report::StatusMsg( msg );
         }
      }

   pLayer->UpdateExtents();

   // populate data
   int colCount = input.GetColCount();
   DataObj  *pDataObj = pLayer->CreateDataTable( hexCount, colCount );
   for( int col=0; col < colCount; col++ )
      {
      LPCTSTR label = input.GetLabel( col );
      TYPE    type  = input.Get( col, 0 ).GetType();

      int width=0, decimals=0;
      ::GetTypeParams( type, width, decimals );

      pLayer->m_pDbTable->SetField( col, label, type, width, decimals, true );

      for ( int row=0; row < hexCount; row++ )
         pDataObj->Set( col, row, input.Get( col, row ) );
      }

   pLayer->SaveShapeFile( "D:\\envision\\StudyAreas\\TampaBay\\idu.shp" );
   gpView->m_mapPanel.m_mapFrame.RefreshList();
   gpView->m_mapPanel.RedrawWindow();

   return true;
   }


bool ConvertDlg::GenerateCentroidsFromLayer()
   {
   // load input csv file
   UpdateData();
   
   DirPlaceholder d;
   CFileDialog dlg( FALSE, "csv", "Centroids.csv", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "CSV Files|*.csv|All files|*.*||");

   if ( dlg.DoModal() == IDCANCEL )
      return false;
   
   int layer = m_layers.GetCurSel();

   Map *pMap = gpView->m_mapPanel.m_pMap;
   MapLayer *pLayer = pMap->GetLayer( layer );

   VDataObj output;
   GetCentroids( pLayer, &output );

   CString filename( dlg.GetPathName() );
   CWaitCursor c;

   output.WriteAscii( filename, ',' );
   return true;
   }
   

bool ConvertDlg::GenerateVoroiniFromLayer()
   {
   // load input csv file
   UpdateData();
   
   DirPlaceholder d;
   CFileDialog dlg( FALSE, "shp", "Voroini.shp", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "Shape Files|*.shp|All files|*.*||");

   if ( dlg.DoModal() == IDCANCEL )
      return false;
   
   int layer = m_layers.GetCurSel();

   Map *pMap = gpView->m_mapPanel.m_pMap;
   MapLayer *pLayer = pMap->GetLayer( layer );

   VDataObj output;

   GetCentroids( pLayer, &output );

   int pts = output.GetRowCount();
   ASSERT( pts == pLayer->GetPolygonCount() );

   // have centroids, generated polygons
   float *xValues = new float[pts];
	float *yValues = new float[pts];

   // NOT IMPLEMENTED YET!!!
	//VoronoiDiagramGenerator vdg;
	//vdg.generateVoronoi( xValues,yValues,pts, -100,100,-100,100,3);
   //
	//vdg.resetIterator();
   //
	//float x1,y1,x2,y2;
   //
	//printf("\n-------------------------------\n");
	//while(vdg.getNext(x1,y1,x2,y2))
	//{
	//	printf("GOT Line (%f,%f)->(%f,%f)\n",x1,y1,x2, y2);
	//	
	//}




   return true;
   }



bool ConvertDlg::GetCentroids( MapLayer *pLayer, VDataObj *pData )
   {
   // set up data object
   bool copyData = IsDlgButtonChecked( IDC_COPYDATA ) ? true : false;
   int cols = 2;

   if ( copyData )
      cols = 2 + pLayer->GetColCount();

   int rows = pLayer->GetRecordCount();
   
   // first, generate centroids
   pData->Clear();
   pData->SetSize( cols, rows );

   // add labels to dataobj "X_CENTROID", "Y_CENTROID", ...
   pData->AddLabel( "X_CENTROID" );
   pData->AddLabel( "Y_CENTROID" );

   if ( copyData )
      {
      for ( int i=0; i < pLayer->GetColCount(); i++ )
         pData->AddLabel( pLayer->GetFieldLabel( i ) );
      }

   // for each polygon, add a row with centroid data to the dataobj
   for ( int i=0; i < rows; i++ )
      {
      Poly *pPoly = pLayer->GetPolygon( i );
      Vertex v( pPoly->GetCentroid() );

      pData->Set( 0, i, (float) v.x );
      pData->Set( 1, i, (float) v.y );

      if ( copyData )
         {
         for ( int j=0; j < pLayer->GetColCount(); j++ )
            {
            VData value;
            pLayer->GetData( i, j, value );   // get from source 
            pData->Set( j+2, i, value );
            }
         }
      }

   return true;
   }
   

