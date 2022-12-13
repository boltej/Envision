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
#include "EnvLibs.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Map.h : header file
//

#ifndef NO_MFC
#include <afxtempl.h>
#include <atlimage.h>
#endif
#include "Maplayer.h"
#include "PtrArray.h"


//#include <Label.h>


class Poly;
class Map;
//class Raster;
class EnvisionRasterizer;
class CacheManager;

const int LOGICAL_EXTENTS = 65536;  //  4096; 

enum /*DRAW_ORDER*/ { DO_UP=-1, DO_DOWN=-2, DO_TOP=-3, DO_BOTTOM=-4 };


// installable notify function
enum NOTIFY_TYPE  
   {
   NT_LOADVECTOR, NT_LOADGRIDROW, NT_LOADDATA, NT_MOUSEMOVE, NT_LBUTTONDBLCLK,
   NT_LBUTTONDOWN, NT_RBUTTONDOWN, NT_RBUTTONDBLCLK, NT_LBUTTONUP, NT_MOUSEWHEEL,
   NT_EXTENTSCHANGED, NT_WRITEDATA, NT_CLASSIFYING,
   NT_CALCSLOPE, NT_CALCDIST, NT_CALCLENGTHSLOPE, NT_LOADRECORD, NT_SELECT, NT_UPDATEMAP,
   NT_BUILDSPATIALINDEX, NT_QUERYDISTANCE, NT_POPULATERECORDS,
   NT_ACTIVELAYERCHANGED, NT_ACTIVEATTRIBUTECHANGED
   };

typedef int(*MAPPROC)(Map*, NOTIFY_TYPE, int, LONG_PTR, LONG_PTR);  // return 0 to interrupt, anything else to continue

// general purpose text notification - the map will call this function when stuff happens
typedef int(*MAPTEXTPROC)(Map*, NOTIFY_TYPE, LPCTSTR);

typedef void (*SETUPBINSPROC)( Map*, int layerIndexe, int col, bool force );


struct MAPPROCINFO
   {
   MAPPROC proc;
   LONG_PTR extra;

   MAPPROCINFO(MAPPROC p, LONG_PTR ex) : proc(p), extra(ex) { }
   };

struct MAPTEXTPROCINFO
   {
   MAPTEXTPROC proc;
   LONG_PTR extra;
   
   MAPTEXTPROCINFO(MAPTEXTPROC p /*, LONG_PTR ex*/) : proc(p), extra(0) { }
   };


#define VERIFY_LAYER ASSERT( mapLayer < m_mapLayerArray.GetSize() )
#define LAYERPTR     (m_mapLayerArray[ mapLayer ].pLayer)


struct MAP_LAYER_INFO
   {
   MapLayer *pLayer;
   bool      deleteOnDelete;

   MAP_LAYER_INFO() : pLayer(NULL), deleteOnDelete(true) { }
   MAP_LAYER_INFO(MapLayer *_pLayer, bool _deleteOnDelete) : pLayer(_pLayer), deleteOnDelete(_deleteOnDelete) { }
   MAP_LAYER_INFO(const MAP_LAYER_INFO& m) : pLayer(m.pLayer), deleteOnDelete(m.deleteOnDelete) { }
   };

class MapLayerArray : public CArray< MAP_LAYER_INFO, MAP_LAYER_INFO& >
   { };


//////////////////////////////////////////////////////////////////////////////////////////
// Map 

class LIBSAPI Map
   {
   friend class MapLayer;
   
#ifdef _WINDOWS
   friend class MapWindow;
   friend class Raster;
#endif

   // Construction
   public:
      Map();

      virtual ~Map();

      // Attributes
   public:
      // basic data structures
      MapLayerArray m_mapLayerArray;   // CArray of MAP_LAYER_INFO structs
      CArray <MapLayer*> m_drawOrderArray;    // first element gets drawn first, etc.

      MapLayer     *m_pActiveLayer;        // ptr to active layer
      CString       m_name;                // used as label if m_showLabel set
      MAP_UNITS     m_mapUnits;            // MU_UNKNOWN, MU_FEET, MU_METERS
      bool          m_deleteOnDelete; 
      void         *m_pExtraPtr;

   public:
      // layer management
      MapLayer *AddLayer(LAYER_TYPE t = LT_NULL);           // add an empty maplayer
      MapLayer *AddLayer(MapLayer*, bool deleteOnDelete = true);     // add a user-created map layer
      MapLayer *AddShapeLayer(LPCTSTR shapeFile, bool loadDB = true, int extraCols = 0, int records = -1);      // add a layer from a shape file
      MapLayer *AddGridLayer(LPCTSTR gridFile, DO_TYPE, int maxLineLength = -1, int binCount = 10, BCFLAG bcFlag = BCF_GRAY_INCR); // add a layer from a ASCII GRID file
      MapLayer *CloneLayer(MapLayer &layer);

      MapLayer *CreateOverlay(MapLayer *pSourceLayer, int overlayType, int activeCol);
      MapLayer *CreatePointOverlay(MapLayer *pSourceLayer, int col);
      MapLayer *CreateDotDensityPointLayer(MapLayer *pPolyLayer, int col);
      MapLayer* CreateGrid(int rows, int cols, REAL xLLCorner, REAL yLLCorner, REAL cellsize, float initialValue, DO_TYPE type, bool classify) {
         MapLayer* pLayer = new MapLayer(this);
         pLayer->CreateGrid(rows, cols, xLLCorner, yLLCorner, cellsize, initialValue, type, true, classify);
         this->AddLayer(pLayer);
         return pLayer;
         }

      MapLayer *GetLayer(int mapLayer) { VERIFY_LAYER; return LAYERPTR; }
      MapLayer *GetLayer(LPCTSTR name);
      MapLayer *GetLayerFromPath(LPCTSTR path);
      int       GetLayerIndex(MapLayer *pLayer);

      bool RemoveLayer(MapLayer*, bool deallocate = false);
      bool RemoveLayer(int mapLayer, bool deallocate = false) { VERIFY_LAYER; return RemoveLayer(LAYERPTR, deallocate); }

      MapLayer *GetActiveLayer() { return m_pActiveLayer; }
      void SetActiveLayer(MapLayer *pLayer) { int oldIndex = GetLayerIndex(m_pActiveLayer); m_pActiveLayer = pLayer; Notify(NT_ACTIVELAYERCHANGED, GetLayerIndex(pLayer), oldIndex); }
      void SetActiveLayer(int mapLayer) { VERIFY_LAYER; SetActiveLayer(LAYERPTR); }
      int  GetLayerCount() { return (int)m_mapLayerArray.GetSize(); }
      void HideAll();
      void ShowLayer(int mapLayer) { VERIFY_LAYER; m_mapLayerArray[mapLayer].pLayer->Show(); }

      void SetDeleteOnDelete(bool value) { m_deleteOnDelete = value; }

      void Clear();     // delete all polygons 
      
      MAP_UNITS GetMapUnits(void) { return m_mapUnits; }
      LPCTSTR   GetMapUnitsStr(void);

      // draw order management
      MapLayer *GetLayerFromDrawOrder(int i) { return m_drawOrderArray[i]; }
      int GetLayerIndexFromDrawIndex(int drawLayerIndex);
      int GetDrawIndexFromLayerIndex(int layerIndex);
      int ChangeDrawOrder(MapLayer *pLayer, int order);  // see enums above, or specify new index location

      // user interface interaction
      bool InstallNotifyHandler(MAPPROC p, LONG_PTR extra) { m_notifyProcArray.Add(new MAPPROCINFO(p, extra)); return true; }
      bool InstallNotifyTextHandler(MAPTEXTPROC p)         { m_notifyTextProcArray.Add(new MAPTEXTPROCINFO(p)); return true; }
      //LONG_PTR    GetNotifyExtra( void ) { return m_extra; }
      bool RemoveNotifyHandler(MAPPROC p, LONG_PTR extra);
      bool RemoveNotifyTextHandler(MAPTEXTPROC p);

   protected:
      // coordinate system mapping info
      REAL m_vxMin;          // virtual coords for display
      REAL m_vxMax;
      REAL m_vyMin;
      REAL m_vyMax;
      REAL m_vExtent;
      
      REAL m_vxMapMinExt;    // for the entire map
      REAL m_vxMapMaxExt;
      REAL m_vyMapMinExt;
      REAL m_vyMapMaxExt;

#ifndef NO_MFC
      CPoint m_windowOrigin;     // logical space
      CSize  m_windowExtent;
      CPoint m_viewportOrigin;   // device coords
      CSize  m_viewportExtent;
      CRect  m_displayRect;       // device coords
      CRect	 m_windowRect;
#endif

   public:
      CString m_bkgrPath;
      REAL   m_bkgrTop;
      REAL   m_bkgrLeft;
      REAL   m_bkgrRight;
      REAL   m_bkgrBottom;
      
   public:
      float m_zoomFactor;     // smaller=zoom out, bigger=zoom in
      
      void SetMapExtents(REAL vxMin, REAL vxMax, REAL vyMin, REAL vyMax);
      void AddMapExtents(REAL vxMin, REAL vxMax, REAL vyMin, REAL vyMax);
      void InitPolyLogicalPoints();
      void UpdateMapExtents( void );

      void GetViewExtents(REAL &vxMin, REAL &vxMax, REAL &vyMin, REAL &vyMax);
      void SetViewExtents(REAL vxMin, REAL vxMax, REAL vyMin, REAL vyMax);
      void GetMapExtents(REAL &vxMin, REAL &vxMax, REAL &vyMin, REAL &vyMax);
      
#ifndef NO_MFC
      void GetDisplayRect( CRect &rect ) { rect = m_displayRect; }
#endif
      float GetZoom( void ) { return m_zoomFactor; }

      int   Translate(float dx, float dy);
      int   Scale(float xMin, float yMin, float xMax, float yMax);

#ifndef NO_MFC
      // coordinate conversion
      void  LPtoDP(POINT &p);
      void  DPtoLP(POINT &p);
      void  DPtoVP(POINT &p, REAL &vx, REAL &vy) { POINT temp = p; DPtoLP(temp); LPtoVP(temp, vx, vy); }
      bool GetGridCellFromDeviceCoord( POINT &point, int &row, int &col );
#endif
      void  VPtoLP(REAL vx, REAL vy, POINT &lp);
      void  LPtoVP(POINT &p, REAL &vx, REAL &vy);
      int   VDtoLD(REAL d);   // distances
      float VDtoLD_float(REAL d);
      REAL  LDtoVD(int d);
      int  GetAttrAt( CPoint &pt, CString &attrText );

      // misc information
      PtrArray < MAPPROCINFO > m_notifyProcArray;
      PtrArray < MAPTEXTPROCINFO >  m_notifyTextProcArray;

   public:
      int Notify(NOTIFY_TYPE t, int a0, LONG_PTR a1);
      int NotifyText(NOTIFY_TYPE t, LPCTSTR text);

   static SETUPBINSPROC m_setupBinsFn;
   static void SetupBins( Map *pMap, int layerIndex, int nID, bool force = false );
   };


// ------------------------------- Map inlines -----------------------------------//   

inline
MapLayer *Map::AddShapeLayer(LPCTSTR shapeFile, bool loadDB /*=true*/, int extraCols /*=0*/, int records /*=-1*/)
   {
   MapLayer *pLayer = AddLayer(LT_NULL);

   if (pLayer != NULL)
      {
      int retVal = pLayer->LoadShape(shapeFile, loadDB, extraCols, records);

      if (retVal < 0) // failed loading?
         {
         RemoveLayer(pLayer, true);
         return NULL;
         }
      }

   return pLayer;
   }


///inline
///void Map::DrawPoly( int mapLayer, int index, int flash ) 
///   {
///   VERIFY_LAYER;
///   ASSERT( index < LAYERPTR->m_pPolyArray->GetSize() );
///   DrawPoly( LAYERPTR->m_polyArray[ index ], flash );
///   }




inline
void Map::SetMapExtents(REAL vxMin, REAL vxMax, REAL vyMin, REAL vyMax)
{
	m_vxMapMinExt = vxMin;
	m_vxMapMaxExt = vxMax;
	m_vyMapMinExt = vyMin;
	m_vyMapMaxExt = vyMax;
}

inline
void Map::GetMapExtents(REAL &vxMin, REAL &vxMax, REAL &vyMin, REAL &vyMax)
{
	vxMin = m_vxMapMinExt;
	vxMax = m_vxMapMaxExt;
	vyMin = m_vyMapMinExt;
	vyMax = m_vyMapMaxExt;
}


inline
void Map::GetViewExtents(REAL &vxMin, REAL &vxMax, REAL &vyMin, REAL &vyMax)
{
	vxMin = m_vxMin;
	vxMax = m_vxMax;
	vyMin = m_vyMin;
	vyMax = m_vyMax;
}


inline
void Map::SetViewExtents(REAL vxMin, REAL vxMax, REAL vyMin, REAL vyMax)
{
#ifdef NO_MFC
  using namespace std; //for min()/max()
#endif

	m_vxMin = min(vxMin, vxMax);
	m_vxMax = max(vxMin, vxMax);
	m_vyMin = min(vyMin, vyMax);
	m_vyMax = max(vyMin, vyMax);

	if ((m_vxMax - m_vxMin) > (m_vyMax - m_vyMin))
		m_vExtent = m_vxMax - m_vxMin;
	else
		m_vExtent = m_vyMax - m_vyMin;

	// must reinitialize all the polygons...
	InitPolyLogicalPoints();
}



inline
float Map::VDtoLD_float(REAL d)
{
	//-- check for divide-by-zero error --//
	if (m_vExtent == 0)
		return 0;

	return (float)(LOGICAL_EXTENTS * d / m_vExtent);
}


inline
int Map::VDtoLD(REAL d)
{
	//-- check for divide-by-zero error --//
	if (m_vExtent == 0)
		return 0;

	return (int)(LOGICAL_EXTENTS * d / m_vExtent);
}

inline
REAL Map::LDtoVD(int d)
{
	//-- check for divide-by-zero error --//
	if (m_vExtent == 0)
		return 0;

	return m_vExtent * d / LOGICAL_EXTENTS;
}

///inline
///void Map::DrawPoly( int mapLayer, int index, int flash ) 
///   {
///   VERIFY_LAYER;
///   ASSERT( index < LAYERPTR->m_pPolyArray->GetSize() );
///   DrawPoly( LAYERPTR->m_polyArray[ index ], flash );
///   }


inline
LPCTSTR Map::GetMapUnitsStr(void)
{
	switch (m_mapUnits)
	{
	case MU_UNKNOWN:  return _T("");
	case MU_METERS:   return _T("m");
	case MU_FEET:     return _T("ft");
	case MU_DEGREES:  return _T("deg");
	}

	return NULL;
}


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
