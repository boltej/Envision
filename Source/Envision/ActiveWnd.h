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
#pragma once

class MapFrame;
class ScatterWnd;
class DataGrid;
class VDataTable;


/*------------------------------------------------------------
 * ActiveWnd - static class that manages the active window
 *
 * Possible Active Window include:
 *   1) MapFrames (main MapPanel or MapFrames embedded in a ResultsMapWnd)
 *   2) DataGrid (contained in the DataPanel)
 *   3) ScatterWnds(contained in the ResultWnds and RtViews)
 *   4) VDataTables( contained in ResultsWnd )
 *
 *  Any time any of these windows become activated, they should
 *  call ActiveWnd::SetActiveWnd();
 *------------------------------------------------------------*/

enum AWTYPE { AWT_UNKNOWN=-1, AWT_MAPFRAME, AWT_DATAGRID, AWT_SCATTERWND, AWT_VDATATABLE, AWT_VISUALIZER, AWT_ANALYTICSVIEWER };

class ActiveWnd
   {
   public:
      ActiveWnd(void);
      ~ActiveWnd(void);

      static bool SetActiveWnd( CWnd *pWnd );

      static AWTYPE GetType();

      static void OnEditCopy();
      static void OnEditCopyLegend();
      static void OnFilePrint();
      static void OnFileExport();

      static void OnUpdateFilePrint( CCmdUI *pCmdUI );
      static void OnUpdateFileExport( CCmdUI *pCmdUI );
      static void OnUpdateEditCopy(CCmdUI *pCmdUI);

      static bool IsMapFrame() { return ( ( GetType() == AWT_MAPFRAME ) ? true : false ); }

      static MapFrame   *GetActiveMapFrame();
      static ScatterWnd *GetActiveScatterWnd();
      static DataGrid   *GetActiveDataGrid();
      static VDataTable *GetActiveVDataTable();

   protected:
      static CWnd *m_pWnd;

   public:
   };
