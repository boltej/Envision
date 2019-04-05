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
#include "StdAfx.h"
#include "ActiveWnd.h"

#include "MapPanel.h"
#include <MapFrame.h>
#include <MapWnd.h>
//#include "ResultsWnd.h"
#include "DataGrid.h"

#include <map.h>
#include <scatterWnd.h>
#include <vdatatable.h>
#include <dataobj.h>
#include <fdataobj.h>
#include <dirPlaceholder.h>


CWnd *ActiveWnd::m_pWnd = NULL;

extern MapPanel  *gpMapPanel; 
extern MapLayer  *gpIDULayer;



ActiveWnd::ActiveWnd(void)
   {
   }

ActiveWnd::~ActiveWnd(void)
   {
   }

bool ActiveWnd::SetActiveWnd( CWnd *pWnd )
   {
   m_pWnd = pWnd;

   if ( pWnd == NULL )
      return true;
   
   AWTYPE type = GetType();
   
   if ( type == AWT_UNKNOWN )
      {
      ASSERT( 0 );
      m_pWnd = NULL;
      return false;
      }
   
   return true;
   }


MapFrame *ActiveWnd::GetActiveMapFrame()
   {
   if ( GetType() == AWT_MAPFRAME )
      return (MapFrame*) m_pWnd;
   else
      return NULL;
   }

ScatterWnd *ActiveWnd::GetActiveScatterWnd()
   {
   if ( GetType() == AWT_SCATTERWND )
      return (ScatterWnd*) m_pWnd;
   else
      return NULL;
   }

DataGrid *ActiveWnd::GetActiveDataGrid()
   {
   if ( GetType() == AWT_DATAGRID )
      return (DataGrid*) m_pWnd;
   else
      return NULL;
   }

VDataTable *ActiveWnd::GetActiveVDataTable()
   {
   if ( GetType() == AWT_VDATATABLE )
      return (VDataTable*) m_pWnd;
   else
      return NULL;
   }

AWTYPE ActiveWnd::GetType()
   {
   if ( m_pWnd == NULL )
      return AWT_UNKNOWN;

   CRuntimeClass *rt = m_pWnd->GetRuntimeClass();

   if ( lstrcmpi( rt->m_lpszClassName, "MapFrame" ) == 0 )
      return AWT_MAPFRAME;

   if ( lstrcmpi( rt->m_lpszClassName, "CWnd" ) == 0 )  // NOTE: Should be "DataGrid"!!!!!
      return AWT_DATAGRID;

   if ( lstrcmpi( rt->m_lpszClassName, "ScatterWnd" ) == 0 )
      return AWT_SCATTERWND;

   if ( lstrcmpi( rt->m_lpszClassName, "VDataTable" ) == 0 )
      return AWT_VDATATABLE;

   if ( lstrcmpi( rt->m_lpszClassName, "RtVisualizer" ) == 0 )
	   return AWT_VISUALIZER;

   if ( lstrcmpi( rt->m_lpszClassName, "VisualizerWnd" ) == 0 )
	   return AWT_VISUALIZER;
   
   if ( lstrcmpi( rt->m_lpszClassName, "AnalyticsViewer" ) == 0 )
	   return AWT_ANALYTICSVIEWER;

   ASSERT( 0 );
   return AWT_UNKNOWN;
   }



void ActiveWnd::OnEditCopy( void )
   {
   AWTYPE type = GetType();
   
   switch ( type )
      {
      case AWT_MAPFRAME:
         {
         MapFrame *pMapFrame = (MapFrame*) m_pWnd;
         Map *pMap = pMapFrame->m_pMapWnd->GetMap();
         if ( pMapFrame->m_pMapWnd )
            pMapFrame->m_pMapWnd->CopyMapToClipboard( 1 );
         }
         break;

      case AWT_DATAGRID:
         {
         DataGrid *pGrid = (DataGrid*) m_pWnd;
         pGrid->CopySelected();
         }
         break;
         
      case AWT_SCATTERWND:
         {
         ScatterWnd *pScatter = (ScatterWnd*) m_pWnd;
         pScatter->pLocDataObj->CopyToClipboard();
         }
         break;

      case AWT_VDATATABLE:
         {
         VDataTable *pTable = (VDataTable*) m_pWnd;
         pTable->m_grid.OnEditCopy();
         }
         break;

      case AWT_ANALYTICSVIEWER:
         {
         }
         break;

      case AWT_VISUALIZER:
         {
         //if ( gpMapPanel && gpMapPanel->m_pMapWnd )
         //   gpMapPanel->m_pMapWnd->CopyMapToClipboard( 0 );
         }
         break;

      default:
         ASSERT( 0 );
      }
   }


void ActiveWnd::OnUpdateEditCopy(CCmdUI *pCmdUI)
   {
   AWTYPE type = GetType();

   switch( type )
      {
      case AWT_MAPFRAME:
      case AWT_DATAGRID:
      case AWT_SCATTERWND:
      case AWT_VDATATABLE:
      case AWT_ANALYTICSVIEWER:
         pCmdUI->Enable( 1 );
         return;
      }

   pCmdUI->Enable( 0 );
   }


void ActiveWnd::OnEditCopyLegend()
   {
   if ( m_pWnd == NULL )
      return;

   AWTYPE type = GetType();

   if ( type == AWT_MAPFRAME )
      {
      MapFrame *pMapFrame = (MapFrame*) m_pWnd;
      pMapFrame->m_pMapWnd->CopyMapToClipboard( 2 );
      }
   }


void ActiveWnd::OnFilePrint()
   {
   AWTYPE type = GetType();

   switch( type )
      {
      case AWT_MAPFRAME:
         {
         MapFrame *pMapFrame = (MapFrame*) m_pWnd;
         //pMapWnd->m_pMap->Print();
         break;
         }

      case AWT_DATAGRID:
         {
         DataGrid *pGrid = (DataGrid*) m_pWnd;
         //pGrid->prin
         }
         break;

      case AWT_SCATTERWND:
         break;

      case AWT_VDATATABLE:
         {
         VDataTable *pTable = (VDataTable*) m_pWnd;
         pTable->m_grid.Print();
         }
         break;

      case AWT_VISUALIZER:
         break;
      }
      
   return;
   }

void ActiveWnd::OnUpdateFilePrint( CCmdUI *pCmdUI )
   {
   AWTYPE type = GetType();

   CString label( _T("Print" ) );
   BOOL enable = TRUE;

   switch ( type )
      {
      case AWT_MAPFRAME:
         label += _T(" Map..." );
         break;

      case AWT_DATAGRID:
         label += _T(" Data Grid...");
         break;

      case AWT_SCATTERWND:
         label += _T(" Plot..." );
         break;

      case AWT_VDATATABLE:
         label += _T(" Table..." );
         break;
      
      case AWT_VISUALIZER:
         label += _T(" Visualizer..." );
         break;
      
      case AWT_ANALYTICSVIEWER:
         label += _T(" Analytics..." );
         break;
      
      default:
         enable = FALSE;
      }

   pCmdUI->SetText( label );
   pCmdUI->Enable( enable );
   }



void ActiveWnd::OnFileExport()
   {
   AWTYPE type = GetType();

   CString filename;
   DataObj *pData = NULL;
   
   switch ( type )
      {
      case AWT_MAPFRAME:
         {
         MapFrame *pMapFrame = (MapFrame*) m_pWnd;
         MapLayer *pLayer = pMapFrame->m_pMap->GetActiveLayer();
         pData = pLayer->m_pData;
         filename = pLayer->m_name;
         }
         break;

      case AWT_DATAGRID:
         //DataGrid *pGrid = (DataGrid*) m_pWnd;
         //??? TODO
         break;
         
      case AWT_SCATTERWND:
         {
         ScatterWnd *pScatter = (ScatterWnd*) m_pWnd;
         pData = pScatter->pLocDataObj;
         pScatter->GetWindowText( filename );
         }
         break;

      case AWT_VDATATABLE:
         {
         VDataTable *pTable = (VDataTable*) m_pWnd;
         pData = pTable->m_pData;
         filename = pTable->m_pData->GetName();
         }
         break;

      case AWT_VISUALIZER:
         {
         ASSERT ( 0 );
         }
         break;

      case AWT_ANALYTICSVIEWER:
         {
         ASSERT ( 0 );
         }
         break;


      default:
         ASSERT( 0 );
      }

   DirPlaceholder d;

   CFileDialog dlg( FALSE, "csv", filename, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "CSV Files|*.csv|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );
      CWaitCursor c;
      if ( pData )
         pData->WriteAscii( filename, ',' );
      }
   }
 

void ActiveWnd::OnUpdateFileExport( CCmdUI *pCmdUI )
   {
   AWTYPE type = GetType();

   CString label( _T( "Export" ) );
   BOOL enable = TRUE;

   switch ( type )
      {
      case AWT_MAPFRAME:
         label += _T(" Map..." );
         break;

      case AWT_DATAGRID:
         label += _T(" Data Grid...");
         break;

      case AWT_SCATTERWND:
         label += _T(" Plot..." );
         break;

      case AWT_VDATATABLE:
         label += _T(" Table..." );
         break;
      
      case AWT_ANALYTICSVIEWER:
         label += _T(" Analytics..." );
         break;

      default:
         enable = FALSE;
      }

   pCmdUI->SetText( label );
   pCmdUI->Enable( enable );
   }
