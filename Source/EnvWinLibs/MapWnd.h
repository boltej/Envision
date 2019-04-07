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
#if !defined(AFX_MAPWND_H__27CF1BB8_178F_11D3_95A0_00A076B0010A__INCLUDED_)
#define AFX_MAPWND_H__27CF1BB8_178F_11D3_95A0_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapWnd.h : header file
//

class Map;
class MapLayer;
class Poly;
class CellPropDlg;

#include "MapListWnd.h"
#include "StaticSplitterWnd.h"
#include "CellPropDlg.h"
#include "TIPWND.H"

#include "CacheManager.h"


//void WINLIBSAPI PopulateFieldsMenu( Map *pMap, MapLayer *pLayer, CMenu &mainMenu, bool addAdditionalLayers );

//typedef void (*SETUPBINSPROC)( Map*, int layerIndexe, int col, bool force );



enum MTPOS 
   {
   MTP_FROMLEFT = 1, MTP_FROMRIGHT = 2, MTP_FROMTOP = 4, MTP_FROMBOTTOM = 8,
   MTP_ALIGNLEFT = 16, MTP_ALIGNCENTER = 32, MTP_ALIGNRIGHT = 64
   };

class WINLIBSAPI MapText
   {
   public:
      CString m_text;
      int m_x;          // display coord
      int m_y;          // display coord
      int m_fontSize;   // 10ths of a point

      int m_posFlags;  // relative to: 1=left, 2=right, 4=top, 8=bottom;  16=TA_CENTER, 32=TA_RIGHT

      CSize m_textSize;   // largest background rect, logical coords

      MapText(void) : m_x(0), m_y(0), m_fontSize(240), m_posFlags(0) { }
      MapText(LPCTSTR text, int x, int y, int fontSize) : m_text(text), m_x(x), m_y(y),
         m_fontSize(fontSize), m_posFlags(0) { }

      CPoint GetPos(MapWindow *pMapWnd );      // display coords
      bool GetTextRect(MapWindow *pMapWnd, CRect &rect);    // logical coords
   };


enum MAP_MODE     { MM_NORMAL, MM_ZOOMIN, MM_ZOOMOUT, MM_PAN, MM_INFO, MM_SELECT, MM_QUERYDISTANCE };

enum MAP_DRAWFLAGS  { MDF_MAP = 1, MDF_LEGEND = 2, MDF_ALL = 3 };
enum DRAW_VALUE { DV_COLOR, DV_VALUE, DV_INDEX };


class WINLIBSAPI MapWindow : public CWnd
   {
   DECLARE_DYNCREATE(MapWindow)

   friend class MapListWnd;
   friend class MapListBox;
   friend class MapText;
   friend class Raster;
   //friend class EnvisionRasterizer;

   // Construction
   public:
      MapWindow( /*Map *pMap*/ );

      CacheManager *m_pMapCache;

      // Attributes
   protected:
      Map         *m_pMap;
    	TipWnd      *m_pTipWnd;

   public:
      void SetMap( Map *pMap ) { m_pMap = pMap; }
      Map *GetMap() { return m_pMap; };

      void ShowScale(bool show) { m_showScale = show; }
      bool IsScaleShowing() { return m_showScale; }

      void ShowAttributeOnHover(bool show) { m_showAttributeOnHover = show; m_toolTip.ShowWindow(show ? 1 : 0); }
      bool IsShowAttributeOnHover() { return m_showAttributeOnHover; }

      bool LoadBkgrImage();  // uses map's info
      bool LoadBkgrImage(LPCTSTR url, REAL xLeft, REAL yTop, REAL xRight, REAL yBottom);
      bool CopyBkgrImage(MapWindow *pSourceMap);
      bool IsBkgrImageVisible(void) { return m_showBkgrImage && m_pBkgrImage != NULL; }
      void ShowBkgrImage(bool show) { m_showBkgrImage = show; }
      CImage *GetBkgrImage(void) { return m_pBkgrImage; }

      bool GetGridCellFromDeviceCoord(POINT &point, int &row, int &col);

      // display control
      //void UpdateMapExtents();
      void DrawPoly(MapLayer *pLayer, Poly *pPoly, int flash = 0); // for drawing an individual polygon (not for drawing map!)
      void DrawPoly(int mapLayer, int index, int flash = 0) { MapLayer *pLayer = m_pMap->GetLayer( mapLayer ); DrawPoly( pLayer, pLayer->m_pPolyArray->GetAt(index), flash); }
      void DrawPolyBoundingRect(Poly *pPoly, int flash = 0); // for drawing an individual polygon (not for drawing map!)
      void DrawPoint(Poly *pPoly, int flash = 0);
      void DrawCircle(REAL x, REAL y, REAL radius);
      void DrawPolyVertices(Poly *pPoly);
      void FlashPoly(int mapLayer, int pPolyIndex, int period = 250);  // make mapLayer the active one... period is msecs
      void ZoomIn(bool redraw = true) { m_pMap->m_zoomFactor *= 1.1f; if (redraw) RedrawWindow(); m_pMap->Notify(NT_EXTENTSCHANGED, 0, 0); }
      void ZoomOut(bool redraw = true) { m_pMap->m_zoomFactor /= 1.1f; if (redraw) RedrawWindow(); m_pMap->Notify(NT_EXTENTSCHANGED, 0, 0); }
      void ZoomFull(bool redraw = true); // { m_zoomFactor = 0.9f; RedrawWindow(); }
      void ZoomFit(bool redraw = true) { m_pMap->m_zoomFactor = 1.0f;  if (redraw) RedrawWindow();  m_pMap->Notify(NT_EXTENTSCHANGED, 0, 0); }
      void ZoomAt(POINT p, float zoomFactor, bool redraw=true);
      void ZoomAt(POINT ul, POINT lr, bool redraw = true);  // device coords (e.g. from mouse)
      void ZoomRect(COORD2d ll, COORD2d ur, bool redraw = true); // virtual coords
      void ZoomRect(COORD_RECT *pRect, bool redraw = true) { ZoomRect(pRect->ll, pRect->ur, redraw); } // virtual coords
      void ZoomToPoly(MapLayer *, int polyIndex, bool redraw = true);
      void ZoomToSelection( MapLayer *, bool redraw = true );
      void SetZoomFactor(float zoomFactor, bool redraw=true);
      void CenterAt( POINT p, bool redraw=true );

      void CopyMapToClipboard(int flag);   // flag=1 Copy Map;  flag=2 Copy Legend;  flag=3  Copy Both
      void SaveAsEnhancedMetafile(LPCTSTR filename, int drawFlag); //MAP_DRAWFLAG=MDF_MAP );
      void SaveAsImage(LPCTSTR filename, int imageType, int drawFlag /*= MDF_MAP */);
      void ShowTitle(bool show) { m_showTitle = show; }

      // map labels - text placed on the map
   protected:
      PtrArray< MapText > m_mapTextArray;
      void EraseMapText(CDC &dc, CRect &rect);
      void DrawMapText(CDC &dc, MapText *pText, bool eraseBkgr);

   public:
      MapText *AddMapText(LPCTSTR text, int x, int y, int posFlags, int fontsize); // { int index = (int) m_mapTextArray.Add( new MapText( text, x, y, fontsize ) ); return m_mapTextArray[ index ]; } 
      int      GetMapTextCount(void) { return (int)m_mapTextArray.GetSize(); }
      MapText *GetMapText(int i) { return m_mapTextArray[i]; }
      void     SetMapText(int i, LPCTSTR text, bool redraw);

      void StartZoomIn(bool sticky = false)  { m_mode = MM_ZOOMIN;  m_modeSticky = sticky; }
      void StartZoomOut(bool sticky = false) { m_mode = MM_ZOOMOUT; m_modeSticky = sticky; }
      void StopZoom()                       { m_mode = MM_NORMAL;  m_modeSticky = false; }
      void StartPan(bool sticky = false)    { m_mode = MM_PAN;     m_modeSticky = sticky; }
      void StopPan()                       { m_mode = MM_NORMAL;  m_modeSticky = false; }
      void StartSelect(void)               { m_mode = MM_SELECT;  m_modeSticky = false; }
      void StartQueryDistance(void)        { m_mode = MM_QUERYDISTANCE; m_modeSticky = false; }

   protected:
      bool     m_invertCoords;   // true for screen display, false for clipboard
      MAP_MODE m_mode;           // MM_NORMAL, MM_ZOOM, MM_PAN, MM_INFO, etc...
      bool     m_modeSticky;     // is the mode persistent after LBUTTONUP 
      CPoint   m_startPt;        // temporary for rubber rects
      CPoint   m_lastPt;         // ""
      bool     m_rubberBanding;  // are we in the middle of drawing a rubber band rect?
      bool     m_deleteOnDelete;
      bool     m_useGdiPlus;

      // misc information
      int      m_timerCount;
      COLORREF m_bkgrColor;
      COLORREF m_selectionColor;
      int      m_selectionHatchStyle;// -1 for no hatch, or HS_BDIAGONAL, HS_CROSS, HS_DIAGCROSS, 
      //      HS_FDIAGONAL, HS_HORIZONTAL, HS_VERTICAL
      bool     m_showTitle;
      bool     m_showScale;
      CFont    m_labelFont;
      LONG_PTR m_extra;          // extra data passed to the map proc

      bool     m_showAttributeOnHover;
      CToolTipCtrl m_toolTip;
      TCHAR    m_toolTipContent[512];
      CPoint   m_ttPosition;    // client coords
      bool     m_ttActivated;

      int      m_maxPolyParts;     // maximum number of poly parts, used and maintained by DrawMap(.)

   public:
      CString  m_bkgrImageFilename;
      CImage * m_pBkgrImage;
      bool     m_showBkgrImage;
      Vertex   m_bkgrImageUL;  // virtual coords
      Vertex   m_bkgrImageLR;

      CStringArray m_wmsServers;

      // Operations
   protected:
      void DrawBackground(CDC &dc);
      void DrawMapToDeviceContext(CDC&, bool initMapping = true, int drawFlag = 3);   // used internally
      void DrawLayer(CDC &dc, MapLayer *pLayer, bool initMapping = true, DRAW_VALUE drawValue = DV_COLOR);
      void DrawLayer_Gdiplus(CDC &dc, MapLayer *pLayer, bool initMapping = true, DRAW_VALUE drawValue = DV_COLOR);

      void DrawLabels(CDC &dc);
      void DrawScale(CDC &dc);
      void DrawMapText(CDC &dc, bool eraseBkgr);

   public:
      void DrawMap(CDC&);  // drawing to screen
      void DrawMapGL();

      void DrawDirections(MapLayer*);
      void DrawDownNodes(MapLayer*);
      void DrawText(LPCTSTR, int x, int y, CFont *pFont = NULL); // SCREEN coords!

   protected:
      void DrawLabel(CDC &dc, MapLayer *pLayer, Poly *pPoly, int index, MAP_FIELD_INFO *pInfo);
      void DrawLegend(CDC&);
      void DrawSelection(CDC&);
      void InitMapping(CDC&);

   public:
      // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(Map)
      //}}AFX_VIRTUAL

      // Implementation
   public:
      virtual ~MapWindow();

      // Generated message map functions
   protected:
      //{{AFX_MSG(Map)
      afx_msg void OnPaint();
      afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
      afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
      afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
      afx_msg void OnMouseMove(UINT nFlags, CPoint point);
      afx_msg void OnTimer(UINT_PTR nIDEvent);
      afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
      afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
      afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
      afx_msg BOOL OnEraseBkgnd(CDC* pDC);
      afx_msg BOOL OnTTNNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
      //}}AFX_MSG
      DECLARE_MESSAGE_MAP()

   public:
      afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
      afx_msg void OnSize(UINT nType, int cx, int cy);
   };


#endif // !defined(AFX_MAPWND_H__27CF1BB8_178F_11D3_95A0_00A076B0010A__INCLUDED_)
