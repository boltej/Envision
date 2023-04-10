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
// MapLayer.h: interface for the MapLayer class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include "EnvLibs.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "BIN.H"
#include "DBTABLE.H"
#include "Typedefs.h"
#include "SpatialIndex.h"
#include "AttrIndex.h"
#include "NeighborTable.h"
//#include "RTreeIndex.h"

#include <vector>

class Query;
class QueryEngine;
class MapExprEngine;
class TiXmlElement;
class RTreeIndex;

/////////////////////////////////////////////////////////////////////////////
// Misc critters...

class Map;
class Map3d;
class DbTable;

const int USE_ACTIVE_COL = -1;


enum BCFLAG {
   BCF_UNDEFINED = -1, BCF_MIXED = 0, BCF_RED_INCR = 1, BCF_GREEN_INCR = 2, BCF_BLUE_INCR = 3,
   BCF_GRAY_INCR = 4, BCF_SINGLECOLOR = 5, BCF_TRANSPARENT = 6,
   BCF_BLUEGREEN = 7, BCF_BLUERED = 8, BCF_REDGREEN = 9,
   BCF_RED_DECR = 10, BCF_GREEN_DECR = 11, BCF_BLUE_DECR = 12,
   BCF_GRAY_DECR = 13
   };


enum BSFLAG { BSF_LINEAR = 0, BSF_EQUALCOUNTS = 1 };

enum { CF_NONE = 0, CF_GEOMETRY = 1, CF_DATA = 2 };

// SetLabelPoint() method
enum SLPMETHOD { SLP_CENTROID, SLP_ABOVE, SLP_BELOW };

enum MAP_UNITS { MU_UNKNOWN = -1, MU_METERS = 0, MU_FEET = 1, MU_DEGREES = 2 };

enum {
   OT_NOT_SHARED = 0, OT_SHARED_DATA = 1, OT_SHARED_GEOMETRY = 2, OT_SHARED_SPINDEX = 4, OT_SHARED_ATTRINDEX = 8, OT_SHARED_FIELDINFO = 16, OT_SHARED_QUERYENGINE = 32,
   OT_SHARED_ALL = 0xFFFFFFFF
   };

enum OPERATOR { EQ, GT, LT, GE, LE };


COLORREF LIBSAPI GetColor(int count, int maxCount, BCFLAG bcFlag);


struct LIBSAPI FIELD_ATTR
   {
   VData    value;
   CString  label;
   COLORREF color;
   float    minVal;
   float    maxVal;
   int      transparency; // 0=opaque, 100=transparenc

   FIELD_ATTR() : value(), label(), color(0), minVal(0), maxVal(0), transparency(0) { }

   FIELD_ATTR(VData &_value, LPCTSTR _label, COLORREF _color, float _minVal, float _maxVal, int _transparency)
      : value(_value), label(_label), color(_color), minVal(_minVal), maxVal(_maxVal), transparency(_transparency) {}

   FIELD_ATTR(const FIELD_ATTR &a) : value(a.value), label(a.label), color(a.color),
      minVal(a.minVal), maxVal(a.maxVal), transparency(a.transparency) { }
   };




//---------------------------------------------------------------------------------------
// MAP_FIELD_INFO defines field information for the "Field" menu for a coverage
// Notes:
//    1) Typically, this is mapped to a database field.  Additional types include
//       a) Submenu (mfitype = MFIT_SUBMENU) - in this case, fieldname will refer
//          the label of the parent item, label wil be the text to display, and 
//          the pParent ptr will be set.
//---------------------------------------------------------------------------------------

enum MFI_TYPE     // MAP_FIELD_INFO bin types
   {
   MFIT_QUANTITYBINS,   // based on ranges of values between binMin, binMax
   MFIT_CATEGORYBINS,   // based on unique values (binMax = binMin = 0.0f - ignored)
   MFIT_SUBMENU
   };

struct LIBSAPI MAP_FIELD_INFO
   {
   public:
      CString  fieldname;
      CString  label;
      CString  description;
      CString  source;
      MFI_TYPE mfiType;       // MFIT_QUANTITYBINS for quantity (continuous) bins, MFIT_CATEGORYBINS for categories (ints and string, generally)
      TYPE     type;          // data type for this field
      int      displayFlags;  // bin color flags.
      CString  parentName;    // "fieldname" of parent MAP_FIELD_INFO
      MAP_FIELD_INFO *pParent;  // NULL if no parent
      CMenu    *pMenu;          // NULL if no parent, otherwise ptr to parent menu
      int      bsFlag;        // 0=linear, 1=equalCounts
      float    binMin;        // minimum value for this field (not defined for BT_STRING, BT_DSTRING fields, -1 if not defined for other types)
      float    binMax;        // maximum value for this field (not defined for BT_STRING, BT_DSTRING fields, -1 if not defined for other types)
      int      alwaysDraw;    // (reserved for future use) >0 means always draw (whether active or not), <= 0 means don't draw unless active
      int      drawFlag;      // (reserved for future use) 0 means draw as default (polygons for poly layers, lines fo line layers, etc), 1=draws as points (at centroids)
      int      transparency;  // transparency level (0-100) for this layer (default is 0)
      bool     save;          // persist this to disk when saving?

   private:
      LONG_PTR  extra;     // HIWORD = decadal maps interval, LOWORD = show in results=1, show in site attr=2, show in outcomes=4

   public:
      CArray< FIELD_ATTR, FIELD_ATTR& > attributes;

   public:
      MAP_FIELD_INFO() : fieldname(), label(), description(), source(), type(TYPE_NULL), mfiType(MFIT_QUANTITYBINS), displayFlags(0), bsFlag(0),
         binMin(0), binMax(0), pParent(NULL), pMenu(NULL), alwaysDraw(0), drawFlag(0), transparency(0), save(true), extra(0) { }

      MAP_FIELD_INFO(MAP_FIELD_INFO &m) { *this = m; }

      MAP_FIELD_INFO(LPCTSTR _fieldname, LPCTSTR _label, LPCTSTR _desc, LPCTSTR _source, int _type, MFI_TYPE _mfiType, int _displayFlags, int _bsFlag, float _binMin, float _binMax, bool _save)
         : fieldname(_fieldname), label(_label), description(_desc), source(_source), type((TYPE)_type), mfiType(_mfiType), /*binCount( _binCount ),*/ displayFlags(_displayFlags), bsFlag(_bsFlag),
         binMin(_binMin), binMax(_binMax), pParent(NULL), pMenu(NULL), alwaysDraw(0), drawFlag(0), transparency(0), extra(0), save(_save) { }

      void operator = (MAP_FIELD_INFO &m)
         {
         fieldname = m.fieldname;
         label = m.label;
         description = m.description;
         source = m.source;
         mfiType = m.mfiType;
         type = m.type;
         displayFlags = m.displayFlags;
         parentName = m.parentName;
         pParent = m.pParent;
         pMenu = NULL;
         bsFlag = m.bsFlag;
         binMin = m.binMin;
         binMax = m.binMax;
         alwaysDraw = m.alwaysDraw;
         drawFlag = m.drawFlag;
         transparency = m.transparency;
         extra = m.extra;
         save = m.save;

         attributes.RemoveAll();
         for (int i = 0; i < m.attributes.GetSize(); i++)
            AddAttribute(m.attributes[i].value, m.attributes[i].label, m.attributes[i].color, m.attributes[i].minVal, m.attributes[i].maxVal, m.attributes[i].transparency);
         }

      void SetExtra(DWORD value) { extra |= value; }
      void UnsetExtra(DWORD value) { extra &= ~(LONG_PTR)value; }
      bool GetExtraBool(DWORD value) { return ((extra & value) ? true : false); }
      WORD GetExtraLow() { return LOWORD(extra); }
      WORD GetExtraHigh() { return HIWORD(extra); }

      // manage attributes
      int AddAttribute(VData &value, LPCTSTR label, COLORREF color, float minVal, float maxVal, int transparency)
         {
         FIELD_ATTR newAttr(value, label, color, minVal, maxVal, transparency);
         return (int)attributes.Add(newAttr);
         }

      int AddAttribute(FIELD_ATTR &a) { return (int)attributes.Add(a); }

      void RemoveAttributes() { attributes.RemoveAll(); }

      int GetAttributeCount() { return (int)attributes.GetCount(); }

      FIELD_ATTR &GetAttribute(int i) { return attributes.GetAt(i); }

      FIELD_ATTR *FindAttribute(VData &value, int *index = NULL);
      FIELD_ATTR *FindAttribute(int value, int *index = NULL);
      FIELD_ATTR *FindAttribute(float value, int *index = NULL);
      FIELD_ATTR *FindAttribute(LPCTSTR value, int *index = NULL);
      FIELD_ATTR *FindAttributeFromLabel(LPCTSTR value, int *index = NULL);

      void SwapAttributes(int index0, int index1) { FIELD_ATTR f = GetAttribute(index0); attributes[index0] = attributes[index1];  attributes[index1] = f; }

      bool IsSubmenu(void) { return mfiType == MFIT_SUBMENU; }
      void SetParent(MAP_FIELD_INFO *_pParent) { pParent = NULL; parentName.Empty(); if (_pParent != NULL) { pParent = _pParent; parentName = pParent->fieldname; } };
   };


//--------- class FieldInfoArray ------------------------------------------------------------------
//
// stores information about fields (columns) in this maplayer
//-------------------------------------------------------------------------------------------------

class LIBSAPI FieldInfoArray : public CArray< MAP_FIELD_INFO*, MAP_FIELD_INFO* >
   {
   public:
      CString m_path;   // path to the field information file (empty for no file)

      FieldInfoArray() {}
      FieldInfoArray(FieldInfoArray &f) { Copy(f); }
      ~FieldInfoArray(void) { Clear(); }

      void Copy(FieldInfoArray &f); //  { m_path = f.m_path; Clear(); for ( int i=0; i < f.GetSize(); i++ ) Add( new MAP_FIELD_INFO( *(f.GetAt( i )))); }
      void Clear() { for (int i = 0; i < GetSize(); i++) delete GetAt(i);  RemoveAll(); }

      FieldInfoArray &operator = (FieldInfoArray &f) { Copy(f); return *this; }
      int AddFieldInfo(LPCTSTR fieldname, LPCTSTR label, LPCTSTR desc, LPCTSTR source, int type, MFI_TYPE mfiType, /*int binCount,*/ int displayFlags, int bsFlag = 0, float binMin = -1.0f, float binMax = -1.0f, bool save = false);
      int AddFieldInfo(MAP_FIELD_INFO *pInfo) { if (pInfo != NULL) return (int) this->Add(new MAP_FIELD_INFO(*pInfo)); else return -1; }

      int SetFieldInfo(LPCTSTR fieldname, LPCTSTR label, LPCTSTR desc, LPCTSTR source, int type, MFI_TYPE mfiType, /*int binCount,*/ int displayFlags, int bsFlag = 0, float binMin = -1.0f, float binMax = -1.0f, bool save = false);

      void Swap(int index0, int index1) { MAP_FIELD_INFO *pTemp = GetAt(index0); SetAt(index0, GetAt(index1)); SetAt(index1, pTemp); }

      MAP_FIELD_INFO *FindFieldInfo(LPCTSTR fieldname, int *pIndex = NULL)const;
      MAP_FIELD_INFO *FindFieldInfoFromLabel(LPCTSTR label)const;
   };



enum LAYER_TYPE { LT_POLYGON, LT_LINE, LT_POINT, LT_GRID, LT_NULL };

const long NO_OUTLINE = 0xff000000;

const int E = 1;
const int SE = 2;
const int S = 4;
const int SW = 8;
const int W = 16;
const int NW = 32;
const int N = 64;
#undef NE
const int NE = 128;


struct ROW_COL
   {
   UINT row;
   UINT col;

   ROW_COL() : row((UINT)-1), col((UINT)-1) { }
   ROW_COL(UINT _row, UINT _col) : row(_row), col(_col) { }
   ROW_COL(const ROW_COL &rc) : row(rc.row), col(rc.col) { }
   ROW_COL & operator = (ROW_COL &rc) { row = rc.row; col = rc.col; return *this; }
   };



struct Vertex : public COORD3d
   {
   Vertex(REAL x, REAL y) : COORD3d(x, y, 0.0) { }
   Vertex(REAL x, REAL y, REAL z) : COORD3d(x, y, z) {}
   Vertex() : COORD3d(REAL(0.0), REAL(0.0), REAL(0.0)) { }
   Vertex(const Vertex& v) { *this = v; }

   Vertex& operator =  (const Vertex& v) { x = v.x; y = v.y; z = v.z; return *this; }
   bool    operator == (const Vertex& v) { return (x == v.x && y == v.y && z == v.z); }

   bool Compare(const Vertex& v) { return (x == v.x && y == v.y && z == v.z); }
   };


class RowColArray : public CArray< ROW_COL, ROW_COL& >
   {
   public:
      int Add(int row, int col)
         {
         ROW_COL newRC(row, col);
         return (int)CArray< ROW_COL, ROW_COL& >::Add(newRC);
         }

      int Find(int row, int col)
         {
         for (int i = 0; i < GetSize(); i++)
            if (GetAt(i).row == row && GetAt(i).col == col)
               return i;
         return -1;
         }
   };


struct MAP_STATE
   {
   BinArray   *m_pBinArray;   // CArray of classification bins
   int    m_activeField;   // column in data object
   bool   m_isVisible;

   // legend formatting
   bool   m_showLegend;
   bool   m_expandLegend;          // does the MapList start out with an expanded legend?
   char   m_legendFormat[32];    // sprintf format string
   bool   m_showBinCount;          // display bin count info?
   bool   m_showSingleValue;       // one item on each legend line?
   bool   m_ascending;             // sort order
   int    m_useDisplayThreshold;   // neg = only display values below threshold, 0 =ignore threshold, pos = only display values above threshold
   float  m_displayThresholdValue; // value to use, see above for usage

   // display settings
   BCFLAG m_binColorFlag;         // how to auto-gen colors...
   bool   m_useVarWidth;          // for line coverages, specifies variable width line segments based on classified value
   int    m_minVarWidth;          // lower scalar for var Width lines, ignored if m_useVarWidth = false;
   int    m_maxVarWidth;          // upper scalar for var Width lines, ignored if m_useVarWidth = false;
   int    m_lineWidth;            // line width for drwing lines, poly outlines
   long   m_outlineColor;         // polygon outline for cell (NO_OUTLINE for no outline)
   long   m_fillColor;            // only used if BCFLAG=BCF_SINGLECOLOR
   long   m_hatchColor;
   int    m_hatchStyle;           // -1 for no hatch, or, HS_BDIAGONAL, HS_CROSS, HS_DIAGCROSS, 
                                         //    HS_FDIAGONAL, HS_HORIZONTAL, HS_VERTICAL
   int     m_transparency;

   bool    m_showLabels;
   long    m_labelColor;
   int     m_labelCol;
#ifdef _WINDOWS
   LOGFONT m_labelFont;
#endif
   float   m_labelSize;
   CString m_labelQueryStr;
   Query  *m_pLabelQuery;
   SLPMETHOD m_labelMethod;
   bool   m_useNoDataBin;
   };

typedef CArray< Vertex, Vertex& > VertexArray;

/////////////////////////////////////////////////////////////////////////////////////
// Poly - encapsulating a polygon

class Poly;

class LIBSAPI PolyArray : public CArray< Poly*, Poly* >
   {
   public:
      PolyArray(void) { }
      PolyArray(const PolyArray &);
      INT_PTR GetPolyIndex(Poly *pPoly);
   };


class LIBSAPI Poly
   {
   public:
      //-- data members --//
      // note: for closed polygons, the last vertex must be the same as the first one.
      VertexArray m_vertexArray;
      CDWordArray m_vertexPartArray;

      REAL m_xMin;   // bounding box
      REAL m_xMax;
      REAL m_yMin;
      REAL m_yMax;

      int    m_id;
      //Bin   *m_pBin;
      POINT *m_ptArray;    // logical points
      Vertex m_labelPoint; // virtual coords.
      long   m_extra;      // user-defined

   // dynamic map stuff
   private:
      CUIntArray m_parents;
      CUIntArray m_children;

   public:
      CUIntArray&       GetParents() { return m_parents; }
      CUIntArray&       GetChildren() { return m_children; }
      const CUIntArray& GetParents()           const { return m_parents; }
      const CUIntArray& GetChildren()          const { return m_children; }

      int               GetParentCount()       const { return (int)m_parents.GetCount(); }
      int               GetParent(int index) { return (int)m_parents.GetAt(index); }
      int               GetChildCount()        const { return (int)m_children.GetCount(); }
      int               GetChild(int index) { return (int)m_children.GetAt(index); }

      // end dynamic map stuff

   public:

      //--- methods ---//
      // constructor
      Poly(void)
         : m_vertexArray(),
         m_vertexPartArray(),
         m_id(-1),
         //m_pBin( NULL ), 
         m_ptArray(NULL),
         m_extra(0),
         m_xMin(float(LONG_MAX)),
         m_xMax(float(LONG_MIN)),
         m_yMin(float(LONG_MAX)),
         m_yMax(float(LONG_MIN))
         { }

      Poly(const Poly &p) { *this = p; }
      Poly& operator=(const Poly &poly);

      // destructor
      ~Poly();
      void ClearPoly();

      // methods
      void  InitLogicalPoints(Map*);
      float CalcSlope(void);
      float CalcElevation(void);
      float CalcDistance(void);  //straight line distance between beginning and end of the reach
      float CalcLengthSlope(float area, float slope); // slope is RISE/RUN coming in, area is HECTARES

      // vertex managment
      int   GetVertexCount() const { return (int)m_vertexArray.GetSize(); }
      Vertex &GetVertex(int i) { return m_vertexArray[i]; }
      void  Translate(float dx, float dy);

      void  AddVertex(const Vertex &v, bool updatePtArray = true); // updatePtArray should always be true

      bool  DoesBoundingBoxIntersect(const Poly *pPoly) const;
      bool  DoesEdgeIntersectLine(Vertex &v0, Vertex &v1) const;
      int   GetIntersectionPoints(Vertex &v0, Vertex &v1, VertexArray &intersectionPts) const;

      bool  GetBoundingRect(double &xMin, double &yMin, double &xMax, double &yMax) const;
      bool  GetBoundingRectF(float &xMin, float& yMin, float& xMax, float& yMax) const;

      bool  GetBoundingZ(float &zMin, float &zMax) const;
      REAL  NearestDistanceToPoly(Poly *pToPoly, REAL threshold = -1, int *retFlag = NULL) const;
      REAL  NearestDistanceToVertex(Vertex *pToVertex) const; // returns negative distance if vertex is inside the poly
      REAL  CentroidDistanceToPoly(Poly *pToPoly) const;
      REAL  GetIntersectionLength(const Poly *pPoly) const;
      bool  IsPointInPoly(Vertex &point) const;
      bool  IsPointInPoly(POINT &point) const;
      const Vertex GetCentroid() const;
      float GetArea() const;
      float GetEdgeLength() const;
      float GetAreaInCircle(Vertex center, float radius) const;
      void  SetLabelPoint(SLPMETHOD method);
      int   CloseVertices() { return 0; }
      void  UpdateBoundingBox(void);

      void TraceVertices(int levels);

   private:
      // helper function for IsPointInPoly(.)
      void _IsPointInPoly(const Vertex *v0, const Vertex *v1, const Vertex &point, bool &isIn) const
         {
         if ((((v0->y <= point.y) && (point.y < v1->y)) ||
            ((v1->y <= point.y) && (point.y < v0->y))) &&
            (point.x < (v1->x - v0->x) * (point.y - v0->y) / (v1->y - v0->y) + v0->x))
            isIn = !isIn;
         }
      void _IsPointInPoly(const POINT *v0, const POINT *v1, const POINT &point, bool &isIn) const
         {
         if ((((v0->y <= point.y) && (point.y < v1->y)) ||
            ((v1->y <= point.y) && (point.y < v0->y))) &&
            (point.x < (v1->x - v0->x) * float(point.y - v0->y) / (v1->y - v0->y) + v0->x))
            isIn = !isIn;
         }
      // helper function for GetAreaInCircle
      int _CircleRectCheck(float xMin, float yMin, float xMax, float yMax, Vertex center, float radius);
   };


//--------------------------------- MapLayer -----------------------------------
//
// implements a map layer.  Data can be polygon, line, point or grid.
// for polygons and lines, features are stored in the m_pPolyArray, and data is stored in m_pData.
// for grids,  data and extents are stored in m_pData.
//------------------------------------------------------------------------------

class  LIBSAPI  MapLayer
   {
   friend class Map;
   friend class Map3d;
   friend class MapWindow;

   public:
      // dynamic map flags
      enum ML_RECORD_SET { ACTIVE = 1, DEFUNCT = 2, ALL = 3, SELECTION = 4 }; // Note:  ALL == ACTIVE | DEFUNCT

      Map *m_pMap;             // Map this layer is associated with

      CString    m_name;       // name of the layer (defaults to path name)
      CString    m_path;       // full path name for vector file
      CString    m_database;   // full path for database file (no filename for DBase)
      CString    m_tableName;  // table name (file name for DBase)

      LAYER_TYPE m_layerType;  // LT_POLYGON, LT_LINE, LT_POINT, LT_GRID
      PolyArray *m_pPolyArray; // empty for grid, defiend for polygons, lines, points
      DbTable   *m_pDbTable;   // table of data for this layer
      DataObj   *m_pData;      // pointer to the DataObj of the m_pDbTable (virtual class)
      MAP_UNITS  m_mapUnits;   // dimensional units used for this map

      int    m_activeField;    // column in data object that corresponds to the first attribute drawn when displaying this map layer
      bool   m_isVisible;      // indicates whether this map layer is visible

      // overlay management
      MapLayer *m_pOverlaySource; // indicates this is an overlay layer using another layer's data and binArray, NULL otherwise
      int       m_overlayFlags;   // OT_NOT_SHARED, OT_SHARED_DATA, OT_SHARED_GEOMETRY, etc... see OT_xxx flags above
      CString   m_overlayFields;  // for source layer only, what fields are overlayed?

      CString   m_initInfo;      // an application-specific initialization string associated with this layer;

      bool   m_includeData;
      bool   m_3D;
      int    m_recordsLoaded; // -1 for all, count for something less than all...
      bool   m_readOnly;
      int    m_output=0;        // where this map is exported in results

   protected:
      // Bin managment
      // Basic idea is that every layer has an array of BinArrays, one for each column in the database.
      // This gets created whenever a DbTable is added (m_binArrayArray), and initialized to the 
      // number of columns on the DbTable, and each binArray ptr is set to NULL.  Whenever a dbTable 
      // column is added or removed, the m_binArrayArray is adjusted accordingly.  Whenever SetBins() 
      // is called, m_binArrayArray is checked for an existing array, which is deleted if it is non-NULL and
      // a new BinArray is allocated and populated.  m_pBinArray is a pointer to the currently active
      // BinArray stored in the m_binArray      
      CArray< Bin*, Bin* > m_polyBinArray;   // pointers to bins fore polygons in this layer
      BinArrayArray m_binArrayArray;   // collection of binArrays for each bin

      int  InitPolyBins(void);
      Bin *GetPolyBin(int i) { if (i >= m_polyBinArray.GetSize()) return NULL; return m_polyBinArray[i]; }

   protected:
      // array of flags for Field information
      REAL m_vxMin;    // virtual coords
      REAL m_vxMax;
      REAL m_vyMin;
      REAL m_vyMax;
      REAL m_vzMin;
      REAL m_vzMax;
      REAL m_vExtent;

      float m_totalArea;

      // grid-only information
      REAL m_cellWidth;
      REAL m_cellHeight;
      float m_noDataValue;

      // query results information
      QueryEngine* m_pQueryEngine;     // memory managed here
      CArray< int, int > m_selection;

      MapExprEngine *m_pMapExprEngine;     // memory managed here


      NeighborTable* m_pNeighborTable;
      SpatialIndex *m_pSpatialIndex;
      AttrIndex    *m_pAttrIndex;
      RTreeIndex   *m_pRTreeIndex;

   protected:
      // miscellaneous protected functions
      bool _GetDataMinMax(int col, float *pMin, float *pMax) const;  // helper for GetDataMinMax();
      bool CheckDisplayThreshold(int rec);

   public:
      void  SetNoDataValue(float noDataValue) { m_noDataValue = noDataValue; }
      float GetNoDataValue(void) const { return m_noDataValue; }
      bool  IsDataValid(int row, int col) const;

   public:
      // legend formatting
      bool   m_showLegend;
      bool   m_expandLegend;          // does the MapList start out with an expanded legend?
      char   m_legendFormat[32];    // sprintf format string
      bool   m_showBinCount;          // display bin count info?
      bool   m_showSingleValue;       // one item on each legend line?
      bool   m_ascending;             // sort order
      int    m_useDisplayThreshold;   // neg = only display values below threshold, 0 =ignore threshold, pos = only display values above threshold
      float  m_displayThresholdValue; // value to use, see above for usage

      // field information management
      bool CheckCol(int &col, LPCTSTR label, TYPE type, int flags);

      int LoadFieldInfoXml(LPCTSTR _filename, bool addExtraFields);
   protected:
      int LoadFieldInfoXml(TiXmlElement *pXmlElement, MAP_FIELD_INFO *pParent, LPCTSTR filename);

   public:
      int AddFieldInfo(MAP_FIELD_INFO *pInfo) { return m_pFieldInfoArray->AddFieldInfo(pInfo); } // note: this allocates a new copy of the passed in MAP_FIELD_INFO
      int AddFieldInfo(LPCTSTR fieldname, int level, LPCTSTR label, LPCTSTR desc, LPCTSTR source, int type, MFI_TYPE mfiType, int displayFlags, int bsFlag = 0, float binMin = -1.0f, float binMax = -1.0f, bool save = false)
         {
         return m_pFieldInfoArray->AddFieldInfo(fieldname, /*level,*/ label, desc, source, type, mfiType, displayFlags, bsFlag, binMin, binMax, save);
         }

      int SetFieldInfo(LPCTSTR fieldname, /*int level,*/ LPCTSTR label, LPCTSTR desc, LPCTSTR source, int type, MFI_TYPE mfiType, int displayFlags, int bsFlag = 0, float binMin = -1.0f, float binMax = -1.0f, bool save = false)
         {
         return m_pFieldInfoArray->SetFieldInfo(fieldname, /*level,*/ label, desc, source, type, mfiType, displayFlags, bsFlag, binMin, binMax, save);
         }
      int SetFieldInfo(MAP_FIELD_INFO *pInfo) { return SetFieldInfo(pInfo->fieldname, /*pInfo->level,*/ pInfo->label, pInfo->description, pInfo->source, pInfo->type, pInfo->mfiType, pInfo->displayFlags, pInfo->bsFlag, pInfo->binMin, pInfo->binMax, pInfo->save); }

      int GetFieldInfoCount(int flags);   // flags=0 returns just fields, 1=return all // { return (int) m_pFieldInfoArray->GetSize(); }

      FieldInfoArray *m_pFieldInfoArray;

      MAP_FIELD_INFO *FindFieldInfo(LPCTSTR field);           // field is the name of the database field 
      MAP_FIELD_INFO *FindFieldInfoFromLabel(LPCTSTR label);  // label is the extended field descriptor
      MAP_FIELD_INFO *GetFieldInfo(int offset) { return m_pFieldInfoArray->GetAt(offset); }
      FieldInfoArray *GetFieldInfoArray(void) { return m_pFieldInfoArray; }

   public:
      // display settings
      BCFLAG m_binColorFlag;         // how to auto-gen colors...
      bool   m_useVarWidth;          // for line coverages, specifies variable width line segments based on classified value
                                     // for point coverages, specifies points should be scaled to classified attribute values
      int    m_minVarWidth;          // lower scalar for var Width lines, points; ignored if m_useVarWidth = false;
      int    m_maxVarWidth;          // upper scalar for var Width lines, points; ignored if m_useVarWidth = false;
      int    m_lineWidth;            // line width for drawing lines, poly outlines
      long   m_outlineColor;         // polygon outline for cell (NO_OUTLINE for no outline)
      long   m_fillColor;            // only used if BCFLAG=BCF_SINGLECOLOR
      long   m_hatchColor;
      int    m_hatchStyle;           // -1 for no hatch, or, HS_BDIAGONAL, HS_CROSS, HS_DIAGCROSS, 
                                     //    HS_FDIAGONAL, HS_HORIZONTAL, HS_VERTICAL
      int    m_transparency;         // 0=opaque, 100=transparent

      // label info
   public:
      bool      m_showLabels;
#ifdef _WINDOWS
      LOGFONT   m_labelFont;          // font to draw labels, NULL for no labels
#endif  //_WINDOWS
      long      m_labelColor;         // color to draw labels (NO_LABELS for no label)
      int       m_labelCol;           // -1 for no label col
      float     m_labelSize;          // virtual coordinates
      CString   m_labelQueryStr;
      Query    *m_pLabelQuery;
      SLPMETHOD m_labelMethod;
      CString   m_projection;         // contains the projection field contents, if defined.

      bool   m_useNoDataBin;          // automatically create a no data bin?

   protected:
      int   m_changedFlag;            // CF_NONE, CF_GEOMETRY, CF_DATA 

   public:
      int  GetChangedFlag() { return m_changedFlag; }
      void SetChangedFlag(int flag) { m_changedFlag |= flag; }

   protected:
      MapLayer(MapLayer&);          // copy constructor
      MapLayer(Map*);

   public:
      MapLayer(MapLayer*, int overlayFlags);  // clone method - see overlay flags above

   protected:
      virtual ~MapLayer();

   public:
      // general
      Map *GetMapPtr(void) const { return m_pMap; }
      bool GetState(MAP_STATE&) const;
      bool RestoreState(MAP_STATE&);

      bool IsOverlay(void) { return m_pOverlaySource != NULL ? true : false; }
      MapLayer *GetOverlaySource(void) { return m_pOverlaySource; }

      // general data managment/information
      void  ClearPolygons();  // deletes all polygons
      void  ClearData();  // deletes all associated data
      int   GetRecordCount(ML_RECORD_SET flag = ACTIVE) const;
      int   GetFieldCount(void) const;

      int   GetRowCount(ML_RECORD_SET flag = ACTIVE) const { return GetRecordCount(flag); }
      int   GetColCount(void) const { return GetFieldCount(); }
      bool  SwapCols(int col0, int col1);
      float GetTotalArea(bool forceRecalc = false);

      AttrIndex *GetAttrIndex(int col);

      LAYER_TYPE GetType(void) const { return m_layerType; }

      // vector management
      int   LoadVectorsAscii(LPCTSTR filename);
      int   LoadShape(LPCTSTR filename, bool loadDB = true, int extraCols = 0, int records = -1);
      int   LoadVectorsCSV(LPCTSTR filename, LPCTSTR data = NULL);
      int   SaveShapeFile(LPCTSTR filename, bool selectedPolysOnly = false, int saveDefunctPolysFlag = 0, int saveEmptyPolys = 0); // NOTE: saveDefunctPolysFlag: 0=ask,1=save,2=don't save
      bool  SaveProjection(LPCTSTR filename=nullptr);

   protected:
      bool   WriteShapeHeader(FILE *fp, int fileSize, int fileType);

   public:
      void  GetExtents(REAL &vxMin, REAL &vxMax, REAL &vyMin, REAL &vyMax) const;
      //void  GetExtents      (double &vxMin, double &vxMax, double &vyMin, double &vyMax) const;
      bool  GetZExtents(float &zMin, float &zMax);  // also sets them internally
      bool  GetZExtents(double &zMin, double &zMax);  // also sets them internally
      void  UpdateExtents(bool initPolyPts = true);
      void  InitPolyLogicalPoints(Map *pMap = NULL);   // default map is owner map 
      Poly *FindPolygon(int x, int y, int *pIndex = NULL) const;   // assumes x, y are device coords
      Poly *GetPolygonFromCoord(REAL x, REAL y, int *pIndex = NULL) const;
      Poly *GetPolyInBoundingBox(Vertex vMin, Vertex vMax, int *pIndex /*=NULL*/) const; // returns a PolyLine in a given b. box
      Poly *GetPolygonFromCentroid(REAL x, REAL y, float tolerance, int *pIndex);
      Poly *FindPoint(int x, int y, int *pIndex) const;   // assumes x, y are device coords, works on point coverages
      bool  IsPolyInMap(Poly *pPoly, float edgeDistance = 0.0f) const;
      int   GetNearestPoly(REAL x, REAL y) const;
      int   GetNearestPoly(Poly *pPoly, float maxDistance, SI_METHOD method = SIM_NEAREST, MapLayer *pToLayer = NULL) const;

      int   GetNearbyPolys(Poly *pPoly, int *neighbors, float *distances, int maxCount, float maxDistance,
         SI_METHOD method = SIM_NEAREST, MapLayer *pToLayer = NULL);

      int   GetNearbyPolysFromIndex(Poly *pPoly, int *neighbors, float *distances, int maxCount, float maxDistance,
         SI_METHOD method = SIM_NEAREST, MapLayer *pToLayer = NULL, void** ex = NULL);

      int   GetNearbyPolysFromNeighborTable(Poly* pPoly, int* neighbors, int maxCount) const;


      float ComputeAdjacentLength(Poly *pThisPoly, Poly *pSourcePoly);

      SpatialIndex *GetSpatialIndex() { return m_pSpatialIndex; }
      int   LoadSpatialIndex(LPCTSTR filename = NULL, float maxDistance = 500, SI_METHOD method = SIM_NEAREST, MapLayer *pToLayer = NULL, PFNC_SPATIAL_INFO_EX fillEx = NULL);     // 1 for successful load, < 0 for failure
      int   CreateSpatialIndex(LPCTSTR filename = NULL, int maxCount = 10000, float maxDistance = 500, SI_METHOD method = SIM_NEAREST, MapLayer *pToLayer = NULL, PFNC_SPATIAL_INFO_EX fillEx = NULL);
      float GetSpatialIndexMaxDistance() const { ASSERT(m_pSpatialIndex); return m_pSpatialIndex != NULL ? m_pSpatialIndex->GetMaxDistance() : -1.0f; }
      int   CompareSpatialIndexMethod(SI_METHOD *method)const;// returns 0 if method matches; -1 if no index; 1 if index but no match and sets arg method to actual method
      //int   MergePolys( Poly *polysToMerge, int polyCount, MapLayer *pToLayer, bool includeData );

      int   CreateRTreeIndex(MapLayer* pToLayer = NULL);



      int   LoadNeighborTable();

      //Poly *GetPolygonFromCoord( float x, float y, int *pIndex=NULL );
      int   AddPolygon(Poly *pPoly, bool updateExtents = true);
      bool  RemovePolygon(Poly *pPoly, bool updateExtents = true);
      Poly *GetPolygon(int i) const { return m_pPolyArray->GetAt(i); }
      int   GetPolygonCount(ML_RECORD_SET flag = ACTIVE) const;
      Poly *GetParentPolygon(Poly *pPoly, int parentIndex) { return GetPolygon(pPoly->GetParent(parentIndex)); }
      Poly *GetAncestorPolygon(Poly *pPoly);
      Poly *GetChildPolygon(Poly *pPoly, int childIndex) { return GetPolygon(pPoly->GetChild(childIndex)); }

      int   CalcSlopes(int colToSet);
      int   CalcLengthSlopes(int slopeCol, int colToSet);
      int   ClonePolygons(const MapLayer *pLayerToCopyFrom);
      int   MergePolygons(); // not implemented
      int   PopulateNearestDistanceToPoly(Poly *pToPoly, int col, REAL threshold, LPCTSTR label = NULL);
      int   PopulateNearestDistanceToPolyIndex(Poly *pToPoly, int col, REAL threshold, int maxCount, LPCTSTR label = NULL);

      //int   PopulateNearestDistanceToCoverage( MapLayer *pToLayer, int col, REAL threshold, bool selectedOnlyTo=false, bool selectedOnlyThis=false, LPCTSTR label=NULL, int colNearestToIndex=-1 );
      int   PopulateNearestDistanceToCoverage(MapLayer *pToLayer, int col, REAL threshold, Query *pToQuery = NULL, Query *pThisQuery = NULL, LPCTSTR label = NULL, int colNearestToIndex = -1, int colNearestToValue = -1, int colNearestSourceValue = -1);
      //int   PopulateNumberNearbyPolys( MapLayer *pToLayer, int col, REAL threshold, bool selectedOnlyTo=false, bool selectedOnlyThis=false, LPCTSTR label=NULL, REAL nearThreshold=10 );
      int   PopulateNumberNearbyPolys(MapLayer *pToLayer, int col, REAL threshold, Query *pToQuery = NULL, Query *pThisQuery = NULL, LPCTSTR label = NULL, REAL nearThreshold = 10);

      int   PopulateLengthOfIntersection(MapLayer *pToLayer, int col, bool selectedOnlyTo = false, bool selectedOnlyThis = false, LPCTSTR label = NULL);
      //int   PopulateSelectedLengthOfIntersection( MapLayer *pToLayer, int col, int colToSelectFrom, LPCTSTR label=NULL  );  // Not Implimented
      //int   PopulateNearestDistanceToSelectedCoverage( MapLayer *pToLayer, int col, REAL threshold, int colToSelectFrom, VData &value, LPCTSTR label=NULL );
      int   PopulateLinearNetworkDensity(MapLayer *pToLayer, int col, REAL distance, bool selectedOnlyTo /*=false*/, bool selectedOnlyThis /*=false*/);

      int   PopulateReachIndex(MapLayer *pFlowDirGrid, MapLayer *pNetworkLayer, int colRchIndex, int method = 0);   // 'method' currently unused
      int   PopulateFlowPathDistanceFromCentroid(MapLayer *pFlowDirectionGrid, MapLayer *pToLayer, int colToSet);
      int   PopulateSideOfStream(MapLayer *pFlowDirectionLayer, MapLayer *pToLayer, int colToSet);
      int   PopulateFlowPathDistance(MapLayer *pFlowDirGrid, MapLayer *pStreamGrid, int method);

      int   PopulateElevation(int colToSet);
      int   Translate(float dx, float dy);
      Vertex GetCentroidFromPolyOffsets(CUIntArray *m_polyIndexArray);
      //int   Scale( float xMin, float yMin, float xMax, float yMax );
      //int   DissolvePolys( int rec0, int rec1 );

      // point management
      int   AddPoint(REAL x, REAL y);   // only point, no data added
      bool  GetPointCoords(int i, REAL &x, REAL &y) const;

      bool ConvertToLine(LPCTSTR fieldToCopy, LPCTSTR binSpec );
      int  ConvertToPoly( bool addToMap, bool classify );


      // grid management
      int   CreateGrid(int rows, int cols, REAL xLLCorner, REAL yLLCorner, REAL cellsize, float initialValue, DO_TYPE type, bool addToMap, bool classify);
      int   CreateGrid(int rows, int cols, REAL xLLCorner, REAL yLLCorner, REAL cellWidth, REAL cellHeight, float initialValue, DO_TYPE type, bool addToMap, bool classify);
      int   CreateGrid(MapLayer *pLayer, REAL cellsize, float initialValue, DO_TYPE, bool addtoMap, bool classify); // Create from an existing maplayer
      //int   CreateGridFromPoly( MapLayer *pLayer, float cellsize, DO_TYPE, bool addtoMap, bool classify );

      int   LoadGrid(LPCTSTR filename, DO_TYPE type, int maxLineWidth = -1, int binCount = 10, BCFLAG binColorFlag = BCF_GRAY_INCR);  // use binCount=0 for no classification
      int   LoadGridAscii(LPCTSTR filename, DO_TYPE type, int maxLineWidth = -1);
      int   LoadGridBIL(LPCTSTR filename);
      int   LoadGridFLT(LPCTSTR filename, DO_TYPE type1);
      bool  GetGridCellFromCoord(REAL x, REAL y, int &row, int &col); // note: coords are virtual
      void  GetGridCellCenter(int row, int col, REAL &x, REAL &y) const;
      void  GetGridCellRect(int row, int col, COORD2d &ll, COORD2d &ur) const;
      REAL GetGridCellWidth(void) const { return m_cellWidth; }
      REAL GetGridCellHeight(void) const { return m_cellHeight; }
      void  SetGridToNoData(void);
      bool  SaveGridFile(LPCTSTR filename);
      bool  FindMinValue(int &row, int &col) const;
      int   FindAdjacentRegion(int row, int col, RowColArray &coordArray) const; // find all cells nth-adjacent to row, col with same attribute

      bool  GetLowerLeft(REAL &xLL, REAL &yLL) const { xLL = m_vxMin; yLL = m_vyMin; return m_layerType == LT_GRID; }
      bool  GetLowerLeft(COORD2d &ll) const { ll.x = m_vxMin; ll.y = m_vyMin; return m_layerType == LT_GRID; }
      void  SetLowerLeft(COORD2d ll);
      void  SetLowerLeft(REAL x, REAL y) { COORD2d ll; ll.x = x; ll.y = y; SetLowerLeft(ll); }
      bool  GetNeighborCoords(int direction, int &row, int &col) const;

    /*  void  SetCellSize(float width, float height) { m_cellWidth = width; m_cellHeight = height; }
      bool  GetCellSize(float &width, float &height) const { width = m_cellWidth; height = m_cellHeight; return m_layerType == LT_GRID; }*/

	  void  SetCellSize(REAL width, REAL height) { m_cellWidth = width; m_cellHeight = height; }
	  bool  GetCellSize(REAL &width, REAL &height) const { width = m_cellWidth; height = m_cellHeight; return m_layerType == LT_GRID; }
      void  SetDataDim(int cols, int rows);
      bool  GetDataDim(int &cols, int &rows) const;

      // conversion functions
      //int   GridToPoly  ( MapLayer *pPolyLayer );  // not implemented yet
      int   DEMtoFlowDir(MapLayer *pFlowLayer);  // "this" grid must be a DEM grid

      // grid-based watershed functions

      // FlowDir Grid layer functions (this layer must be a Flow Direction Grid!)
      float GetDownFlowPathDistance(int row, int col, MapLayer *pStreamLayer) const;
      float GetDownFlowPathGradient(int row, int col, MapLayer *pStreamLayer, MapLayer *pDEMLayer, int &count) const;
      float GetCentroidFlowPathDistance(Poly *pPoly, MapLayer *pFlowDirectionLayer, MapLayer *pToLayer) const;

      int   GetReachIndex(Poly *pPoly, MapLayer *pFlowDirGrid, MapLayer *pNetworkLayer, int method = 0);  // 'method' currently unused

      bool  FollowFlowPath(int &row, int &col) const;
      bool  FindPourPoint(int &row, int &col, MapLayer *pStreamLayer = NULL, bool firstTime = true) const;
      int   FindUpstreamBranchPoints(int row, int col, MapLayer *pStreamLayer, RowColArray &upstreamBranchCoordArray) const;
      bool  GetStreamIntersectionPoint(int &row, int &col, MapLayer *pStreamLayer) const;
      int   GetUpstreamCells(int row, int col, MapLayer *pStreamLayer, RowColArray &coordArray) const;
      bool  IsCellUpstream(int neighborDir, int &row, int &col, MapLayer *pStreamLayer) const;

      int   GetUpslopeCells(int row, int col, RowColArray &coordArray, MapLayer *pStreamLayer = NULL) const;
      bool  GetUpslopeCell(int row, int col, int &rowUp, int &colUp, MapLayer *pStreamLayer = NULL) const;

      bool  GetUpstreamCell(int row, int col, MapLayer *pStreamLayer, int &rowUp, int &colUp) const;
      int   GetTotalUpslopeCells(int row, int col) const;
      float GetTotalUpslopeArea(int row, int col) const;
      bool  IsCellUpslope(int neighborDir, int &row, int &col, MapLayer *pStreamLayer = NULL) const;
      int   DelineateWatershedFromPoint(int row, int col, MapLayer *pWatershedLayer, int watershedID) const;
      int   DelineateWatershedsFromStream(int startRow, int startCol, MapLayer *pWatershedLayer,
         MapLayer *pStreamLayer, int &watershedID) const;
      int   GetSideOfStream(int row, int col, MapLayer *pStreamLayer) const;
      int   FlowDirToPower(int flowDirection) const;

      int   DelineateStreams(float maxUpslopeArea, MapLayer *pStreamLayer);
      int   DelineateStreamsFromPoint(int row, int col, float maxUpslopeArea, MapLayer *pStreamLayer, int branchCount = 0);

      bool  FollowFlowPathSubBasin(int &row, int &col, int subBasin, MapLayer *pWatershed) const;
      bool GetUpstreamPourInPoint(int &rowUp, int &colUp, int subBasin, MapLayer *pStreamLayer) const;


      // DEM layer functions (this layer must be a DEM grid
      int   GetCellFlowDir(int row, int col) const;
      int   FlatAreaFlowDir(int row, int col, MapLayer *pFlowLayer);
      float FillDEMCell(int row, int col);
      int   DEMtoFilledDEM(MapLayer *pFlowLayer);
      int   Create3dShape(MapLayer *pDemLayer);

      int   Project3D(MapLayer *pGrid, bool interpolates = true);

      // Grid Stream layer functions (this layer must be a stream Grid!)
      bool  IsCellInStream(int row, int col) const;

      // database management
      int  LoadDataDBF(LPCTSTR database, int extraCols = 0, int records = -1, CUIntArray *pColsToLoad = NULL);
      int  SaveDataAscii(LPCTSTR filename, bool includeID, bool includeLabels, LPCTSTR colToWrite = NULL) const;

#ifndef NO_MFC
      int  LoadDataAscii(LPCTSTR filename);
      int  LoadDataDB(LPCTSTR database, LPCTSTR connectStr, LPCTSTR sql, int extraCols = 0, int records = -1 /*, CUIntArray *pColsToLoad=NULL*/);
      int  SaveDataDB(void) const;
      int  SaveDataDB(LPCTSTR path, BitArray *pSaveArray = NULL) const { return m_pDbTable->SaveDataDBF(path, pSaveArray); }
      int  SaveDataDB(LPCTSTR databaseName, LPCTSTR tableName, LPCTSTR connectStr, bool selectedOnly = false) const;
      int  SaveDataColDB(int col, LPCTSTR databaseName, LPCTSTR tableName, LPCTSTR connectStr) const;
#endif
      int  InitData(int cols, int rows, float value = 0);
      void SetField(int col, LPCTSTR label, COleVariant &v, bool show = true);

      // query management
      QueryEngine* GetQueryEngine() { return m_pQueryEngine; }
      MapExprEngine* GetMapExprEngine() { return m_pMapExprEngine; }
      int  AddSelection(int i) { return (int)m_selection.Add(i); }
      void SetSelection(int index, int i) { m_selection.SetAt(index, i); }
      void ClearSelection(void) { m_selection.RemoveAll(); }
      void SetSelectionSize(int size) { m_selection.SetSize((INT_PTR)size); }
      int  GetSelectionCount(void) const { return (int)m_selection.GetSize(); }
      int  GetSelection(int i) const { return (int)m_selection[(INT_PTR)i]; }
      int  SelectArea(POINT ll, POINT ur, bool clear, bool insideOnly);  // ll, ur are logical coords
      int  SelectArea(COORD2d ll, COORD2d ur, bool clear, bool insideOnly);  // ll, ur are logical coords
      int  SelectPoint(POINT pt, bool clear = true);           // pt is a logical coord
      int  SelectPoint(COORD2d pt, bool clear = true);           // pt is a logical coord
      int  SelectValue(int col, int value, bool all = true);
      bool IsSelected(int i);

      int FindIndex(int col, int value, int startIndex = 0) const; // gets the first record number (offset) with the specified value

      // get an array of Field labels...
      void  LoadFieldStrings(CStringArray &strArray);
      bool  GetSortedPolyArray(PolyArray& polyArray, int col, bool ascending = true);
      Poly *GetDataAtPoint(int x, int y, int col, CString &value) const;

      // data access
      DataObj  *CreateDataTable(int rows, int cols, DO_TYPE type = DOT_VDATA);
      bool HasData(void) const { return m_pData != NULL ? true : false; }
      bool HasData(int rec, int col) { float value = m_noDataValue; GetData(rec, col, value);  return (value == m_noDataValue ? false : true); }
      void SetData(int rec, int col, float value);
      void SetData(int rec, int col, double value);
      void SetData(int rec, int col, COleVariant &value);
      void SetData(int rec, int col, int value);
      void SetData(int rec, int col, const VData &value);
      void SetData(int rec, int col, LPCTSTR value);
      void SetNoData(int rec, int col) { SetData(rec, col, GetNoDataValue()); }
      void SetAllData(float value, bool includeNoData);
      //bool SetColData( int col, float value, bool includeNoData );    // deprecated, use VData version below
      bool SetColData(int col, VData value, bool includeNoData);
      bool SetColNoData(int col) { return SetColData(col, m_noDataValue, true); }
      bool SetColNull(int col) { return SetColData(col, VData(), true); }
      void AddData(int rec, int col, float incr) { float value = 0; GetData(rec, col, value); SetData(rec, col, value + incr); }

      bool CopyColData(int toCol, int fromCol) { return m_pDbTable->CopyCol(toCol, fromCol); }
      void ClearRows(void);

      // GetData(..) return true on success.
      bool GetData(int rec, int col, COleVariant &value) const;
      bool GetData(int rec, int col, float &value) const;
      bool GetData(int rec, int col, double &value) const;
      bool GetData(int rec, int col, VData &value) const;
      bool GetData(int rec, int col, int &value) const;
      bool GetData(int rec, int col, short &value) const;
      bool GetData(int rec, int col, CString &value) const;
      bool GetData(int rec, int col, bool &value) const;
      bool GetData(int rec, int col, char& value) const;

      bool GetDataMinMax(int col, float *pMin, float *pMax) const;  // note: col=-1 gets min/max for ALL columns
      bool GetGridDataMinMax(float *pMin, float *pMax) const;      // specially for grids

      // Field information
      void    SetActiveField(int col);
      int     GetActiveField(void) const { return m_activeField; }
      LPCTSTR GetFieldLabel(int col = -1) const;
      void    SetFieldLabel(int col, LPCTSTR label);
      int     GetFieldCol(LPCTSTR label) const;
      void    ShowField(int col, bool show);
      bool    IsFieldVisible(int col) const;
      TYPE    GetFieldType(int col) const;
      void    SetFieldType(int col, TYPE, bool convertData);
      void    SetFieldType(int col, TYPE, int width, int decimals, bool convertData);
      bool    IsFieldCategorical(int col);

      // display management
      bool IsVisible() const { return m_isVisible; }
      void Show() { m_isVisible = true; }
      void Hide() { m_isVisible = false; }

      long GetOutlineColor(void) const { return m_outlineColor; }
      void SetOutlineColor(long color) { m_outlineColor = color; }
      void SetNoOutline(void) { m_outlineColor = NO_OUTLINE; }

      COLORREF GetFillColor(void) { return m_fillColor; }
      void SetSolidFill(long color) { m_fillColor = color; m_binColorFlag = BCF_SINGLECOLOR; }

      int  GetTransparency(void) { return m_transparency; }
      void SetTransparency(int value) { m_transparency = value; } // 0=opaque, 100 = transparent

      // -1 for no hatch, or, HS_BDIAGONAL, HS_CROSS, HS_DIAGCROSS, 
      void SetHatch(int style, int color) { m_hatchColor = color; m_hatchStyle = style; }
      long GetHatchColor() const { return m_hatchColor; }
      int  GetHatchStyle() const { return m_hatchStyle; }

      void ClearHatch() { m_hatchStyle = -1; }
      void ShowLabels(void) { m_showLabels = true; }
      void HideLabels(void) { m_showLabels = false; }
      long GetLabelColor(void) const { return m_labelColor; }
      void SetLabelColor(long color) { m_labelColor = color; }
      void SetLabelField(int col) { m_labelCol = col; }
      void SetLabelSize(float size) { m_labelSize = size; }

#ifdef _WINDOWS
      void SetLabelFont(LPCTSTR faceName) { lstrcpy(m_labelFont.lfFaceName, faceName); }
      void CreateLabelFont(CFont &font);
#else
      //dummy entries
      void SetLabelFont(LPCTSTR faceName) {};
      template<class T>
      void CreateLabelFont(T &font) {};

#endif
      bool GetRecordLabel(int rec, CString&, MAP_FIELD_INFO*);

      void ShowLegend(bool show = true) { m_showLegend = show; }
      void ShowBinCount(bool show = true) { m_showBinCount = show; }
      void SortBins(int col, bool increasingOrder);

      int  GetLineWidth() { return m_lineWidth; }
      void SetLineWidth(int width) { m_lineWidth = width; }
      void UseVarWidth(int minWidthPixels, int maxWidthPixels) { m_useVarWidth = true; m_maxVarWidth = maxWidthPixels; m_minVarWidth = minWidthPixels; }
      void ClearVarWidth(void) { m_useVarWidth = false; }

      // classification stuff
      void SetBinColorFlag(BCFLAG flag) { m_binColorFlag = flag; }
      BCFLAG GetBinColorFlag(void) const { return m_binColorFlag; }
      BinArray *GetBinArray(int col, bool add);   // returns if one exists, adds one otherwise
      const BinArray *GetBinArray(int col) const; //accessor for const MapLayer*;

      int  AddBinArray(BinArray *pBinArray) { return (int)m_binArrayArray.Add(pBinArray); }


      void SetBins(int col, BCFLAG bcFlag = BCF_UNDEFINED, int maxBins = -1);  // minimal information known
      void SetBins(MAP_FIELD_INFO *pInfo);
      void SetBins(int col, float binMin, float binMax, int binCount, TYPE type, BSFLAG flag = BSF_LINEAR, BinArray *pExtBinArray = NULL, LPCTSTR units = NULL);
      void SetBins(int col, CStringArray &, BinArray *pExtBinArray = NULL, TYPE type = TYPE_STRING, LPCTSTR units = NULL);  // for string bins
      void SetBins(MapLayer *pSourceLayer, int sourceCol);

      int  SetUniqueBins(int col, TYPE type = TYPE_STRING, BinArray *pExtBinArray = NULL, LPCTSTR units = NULL);

      int  GetBinCount(int col);
      void ResetBins(void) { ClearAllBins(); m_binArrayArray.Init(m_pDbTable->GetColCount()); }
      void AddNoDataBin(int col);

      Bin &GetBin(int col, int i) { BinArray *pBinArray = GetBinArray(col, false);  ASSERT(pBinArray); return pBinArray->GetAt(i); }
      //const accessor for Bin.
      Bin GetBin(int col, int i) const { const BinArray *pBinArray = GetBinArray(col);  ASSERT(pBinArray); return pBinArray->GetAt(i); }

      int GetUniqueValues(int col, CStringArray&, int maxCount = -1) const;  // col=-1 for unique values over all columns
      int GetUniqueValues(int col, CUIntArray&, int maxCount = -1) const;    // col=-1 for unique values over all columns
      int GetUniqueValues(int col, std::vector<int>&, int maxCount = -1) const;    // col=-1 for unique values over all columns
      int GetUniqueValues(int col, CArray< VData, VData& >&, int maxCount = -1) const;   // col=-1 for unique values over all columns

      bool ClassifyPoly(int col, int index);
      bool ClassifyData(int col = -1, int classifyType = 0);   // uses existing bin structure
      //bool ClassifyGridData( int /* scheme */ );
      Bin *GetDataBin(int col, VData &value);
      Bin *GetDataBin(int col, float value);
      Bin *GetDataBin(int col, int value);
      Bin *GetDataBin(int col, LPCTSTR value);

      int Reclass(int col, CArray< VData, VData& > &oldValues, CArray< VData, VData& > &newValues);

      void ClearAllBins(bool resetPolys = false);
      Bin *AddBin(int col, COLORREF color, LPCTSTR label, TYPE binType, float binMin, float binMax);
      Bin *AddBin(int col, COLORREF color, LPCTSTR value, LPCTSTR label);      // for string bins

   protected:
      float AddExpandPoly(int idu, CArray< int, int > &expandArray, int colValue, VData &value, OPERATOR op, int colArea, bool &addToPool, float areaSoFar, float maxArea);
      float AddExpandPolyFromQuery(int idu, CArray< int, int >& expandArray, Query* pQuery, int colArea, bool& addToPool, float areaSoFar, float maxArea);

   public:
      float GetExpandPolysFromAttr(int idu, int colValue, VData &value, OPERATOR op, int areaCol, float maxExpandArea, CArray< int, int > &expandArray);
      float GetExpandPolysFromQuery(int idu, Query* pPatchQuery, int colArea, float maxExpandArea, /* bool* visited,*/ CArray< int, int >& expandArray);

      bool IsNextTo(int poly0, int poly1);
      int  GetNextToPolys(int poly, CArray< int, int > &nextToArray);

      int GetPatchSizes(Query *pPatchQuery, int colArea, CArray< float, float >& patchSizeArray);
      int GetPatchSizes(int colValue, VData& value, OPERATOR op, int colArea, Query* pAreaToConsider, CArray< float, float >& patchSizeArray);

      ////////////////////
      // dynamic map stuff
   public:
      // Subdivide Operation
      void Subdivide(int parent, PolyArray &childrenGeometry); // note: union of polygons in childrenGeometry should equal the parent
      void Subdivide(int parent);
      void UnSubdivide(int parent);

      void SubdivideSpatialIndex(int cell) { m_pSpatialIndex->Subdivide(cell); }
      void UnSubdivideSpatialIndex(int cell, CDWordArray & childIdList) { m_pSpatialIndex->UnSubdivide(cell, childIdList); }

      // Merge Operation ( TODO: implement )
      //void Merge( const CDWordArray &parentList, const Poly *pChildGeometry );  // note: union of polygons in parentList should equal the pChildGeometry
      //void Merge( const CDWordArray &parentList );
      //void UnMerge( const CDWordArray &parentList );

      //void MergeSpatialIndex( const CDWordArray &parentList ){ }
      //void UnMergeSpatialIndex( const CDWordArray &parentList ){ }

      bool IsDefunct(int index) const;
      void DynamicClean();

      int GetDefunctCountByIteration() const;

   private:
      BitArray m_defunctPolyList; // 0 = alive, 1 = dead
      int      m_deadCount;

   private:
      void MakePolyActive(int index);
      void MakePolyDefunct(int index);
      Poly* AddNewPoly(Poly &polyGeometry, const CUIntArray &parentList); // returns ptr to the new Poly

   // end dynamic map stuff
   ////////////////////////

   //////////////////
   // Iterator
   public:
      class Iterator;
      friend class Iterator;

      class LIBSAPI Iterator
         {
         friend class MapLayer;

         private:
            Iterator(const MapLayer *pLayer, int index, ML_RECORD_SET flag);

         public:
            Iterator(const Iterator& pi);

         public:
            // make it behave like an int
            operator int() { return m_index; }

            Iterator& operator =  (const Iterator& pi);

            bool      operator == (const Iterator& pi);
            bool      operator != (const Iterator& pi);

            Iterator& operator ++ () { Increment(); return *this; }
            Iterator& operator ++ (int) { Increment(); return *this; }
            Iterator& operator -- () { Decrement(); return *this; }
            Iterator& operator -- (int) { Decrement(); return *this; }

         private:
            void Increment();
            void Decrement();

         private:
            int m_index;  // keep this as the first data member and then the first 32 bits of this class is an int
                          // it will then behave correctlly even without type checking (as when passed to printf)
            int m_qrIndex;   // the index into the layer's m_selection array
            const MapLayer *m_pLayer;
            ML_RECORD_SET m_flag;
         };

      Iterator Begin(ML_RECORD_SET flag = ACTIVE) const;
      Iterator End(ML_RECORD_SET flag = ACTIVE) const;

      // End Iterator
      ////////////////////

   protected:
#ifndef NO_MFC
#ifndef _WIN64
      int  LoadRecords(CDaoDatabase &database, LPCTSTR sql, int extraCols = 0, int records = -1);
      bool CreateTable(CDaoDatabase &database, LPCTSTR tableName, LPCTSTR connectStr) const;
      int  SaveRecords(CDaoDatabase &database, LPCTSTR tableName) const;
#endif
#endif
      void ExtendGridPoly(Poly *pPoly, int row, int col, int value);
   };


//////--------- inline functions ---------------------------

inline
void MapLayer::GetExtents(REAL &vxMin, REAL &vxMax, REAL &vyMin, REAL &vyMax) const
   {
   vxMin = (float)m_vxMin;
   vxMax = (float)m_vxMax;
   vyMin = (float)m_vyMin;
   vyMax = (float)m_vyMax;
   }


//inline
//void MapLayer::GetExtents(double &vxMin, double &vxMax, double &vyMin, double &vyMax) const
//   {
//   vxMin = (double)m_vxMin;
//   vxMax = (double)m_vxMax;
//   vyMin = (double)m_vyMin;
//   vyMax = (double)m_vyMax;
//   }


inline
void MapLayer::SetData(int rec, int col, float value)
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());
   //ASSERT( m_readOnly == false );

   if (!m_readOnly)
      {
      COleVariant v(value);
      SetData(rec, col, v);
      }
   }

inline
void MapLayer::SetData(int rec, int col, double value)
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   //ASSERT(m_readOnly == false);
   if (!m_readOnly)
      {
      COleVariant v(value);
      SetData(rec, col, v);
      }
   }


inline
void MapLayer::SetData(int rec, int col, int value)
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   //ASSERT(m_readOnly == false);
   if (!m_readOnly)
      {
      VData v(value);
      SetData(rec, col, v);
      }
   }


inline
void MapLayer::SetData(int rec, int col, const VData &value)
   {
   //if ( col == 10 && rec == 0 )
   //   ASSERT( 0 );

   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   //ASSERT(m_readOnly == false);
   if (!m_readOnly)
      {
      ASSERT(m_pData != NULL);
      m_pData->Set(col, rec, value);
      }
   }

inline
void MapLayer::SetData(int rec, int col, COleVariant &value)
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   //ASSERT(m_readOnly == false);
   if (!m_readOnly)
      {
      ASSERT(m_pData != NULL);
      m_pData->Set(col, rec, value);
      }
   }

inline
void MapLayer::SetData(int rec, int col, LPCTSTR value)
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   //ASSERT(m_readOnly == false);
   if (!m_readOnly)
      {
      ASSERT(m_pData != NULL);
      m_pData->Set(col, rec, value);
      }
   }


inline
bool MapLayer::GetData(int rec, int col, COleVariant &value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   return m_pData->Get(col, rec, value);
   }

inline
bool MapLayer::GetData(int rec, int col, float &value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   COleVariant v;
   GetData(rec, col, v);
   v.ChangeType(VT_R4);
   value = v.fltVal;
   return true;
   }

inline
bool MapLayer::GetData(int rec, int col, double &value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   COleVariant v;
   GetData(rec, col, v);
   v.ChangeType(VT_R8);
   value = v.dblVal;
   return true;
   }

inline
bool MapLayer::GetData(int rec, int col, VData &value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec >= 0 && rec < m_pData->GetRowCount());
   ASSERT(col >= 0 && col < m_pData->GetColCount());

   //if ( m_pData == NULL ) return false;
   //if ( rec >= m_pData->GetRowCount() ) return false;
   //if ( col >= m_pData->GetColCount() ) return false;
   bool ok = m_pData->Get(col, rec, value);
   ASSERT(ok);
   return true;
   }


inline
bool MapLayer::GetData(int rec, int col, CString &value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec < m_pData->GetRowCount());
   ASSERT(col < m_pData->GetColCount());
   value = m_pData->GetAsString(col, rec);
   return true;
   }


inline
bool MapLayer::GetData(int rec, int col, int &value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec < m_pData->GetRowCount());
   ASSERT(col < m_pData->GetColCount());
   m_pData->Get(col, rec, value);      // NULL type returns false
   return true;
   }

inline
bool MapLayer::GetData(int rec, int col, short &value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec < m_pData->GetRowCount());
   ASSERT(col < m_pData->GetColCount());
   m_pData->Get(col, rec, value);      // NULL type returns false
   return true;
   }

inline
bool MapLayer::GetData(int rec, int col, bool &value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec < m_pData->GetRowCount());
   ASSERT(col < m_pData->GetColCount());
   m_pData->Get(col, rec, value);      // NULL type returns false
   return true;
   }

inline
bool MapLayer::GetData(int rec, int col, char& value) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(rec < m_pData->GetRowCount());
   ASSERT(col < m_pData->GetColCount());
   m_pData->Get(col, rec, value);      // NULL type returns false
   return true;
   }


inline
void MapLayer::ShowField(int col, bool show)
   {
   ASSERT(col < m_pDbTable->GetColCount());
   m_pDbTable->ShowField(col, show);
   }


inline
int MapLayer::GetFieldCol(LPCTSTR label) const
   {
   if (m_pData == NULL)
      return -2;

   CString _label(label);
   _label.Trim();

   return m_pData->GetCol(_label);
   }

inline
bool MapLayer::IsFieldVisible(int col) const
   {
   ASSERT(col < m_pDbTable->GetColCount());
   return m_pDbTable->IsFieldVisible(col);
   }

inline
TYPE MapLayer::GetFieldType(int col) const
   {
   if (col < 0)
      col = this->GetActiveField();
   if (col < 0)
      return TYPE_NULL;

   ASSERT(m_pDbTable != NULL);
   ASSERT(col < m_pDbTable->GetColCount());

   TYPE type = m_pDbTable->GetFieldType(col);

   if (type == TYPE_NULL && GetRowCount() > 0)
      {
      VData v;
      GetData(0, col, v);
      type = v.type;
      }

   return type;
   }

inline
void MapLayer::SetFieldType(int col, TYPE type, bool convertData)
   {
   int width, decimals;
   GetTypeParams(type, width, decimals);
   SetFieldType(col, type, width, decimals, convertData);
   }

inline
void MapLayer::SetFieldType(int col, TYPE type, int width, int decimals, bool convertData)
   {
   ASSERT(m_pDbTable != NULL);
   ASSERT(col < m_pDbTable->GetColCount());
   m_pDbTable->SetFieldType(col, type, width, decimals, convertData);

   MAP_FIELD_INFO *pInfo = FindFieldInfo(GetFieldLabel(col));
   if (pInfo != NULL)
      pInfo->type = type;
   }

inline
void MapLayer::SetField(int col, LPCTSTR label, COleVariant &v, bool show)
   {
   TYPE type = VData::MapOleType(v.vt);

   int width = 0;
   int decimals = 0;
   GetTypeParams(type, width, decimals);

   m_pData->SetLabel(col, label);
   m_pDbTable->SetFieldType(col, type, width, decimals, true);
   m_pDbTable->ShowField(col, show);

   for (int i = 0; i < m_pData->GetRowCount(); i++)
      SetData(i, col, v);
   }


inline
Bin *MapLayer::AddBin(int col, COLORREF color, LPCTSTR label, TYPE binType, float binMin, float binMax)
   {
   Bin bin;
   bin.SetLabel(label);
   bin.SetColor(color);
   bin.m_minVal = binMin;
   bin.m_maxVal = binMax;
   if (::IsInteger(binType))
      bin.m_strVal.Format(_T("%i"), (int)((binMin + binMax) / 2));
   else if (::IsReal(binType))
      bin.m_strVal.Format(_T("%g"), (binMin + binMax) / 2);

   BinArray *pBinArray = GetBinArray(col, true);

   if (pBinArray == NULL && col == 0)
      {
      pBinArray = new BinArray;
      m_binArrayArray.Add(pBinArray);
      }

   int index = (int)pBinArray->Add(bin);

   if (binMin < pBinArray->m_binMin)
      pBinArray->m_binMin = binMin;

   if (binMax > pBinArray->m_binMax)
      pBinArray->m_binMax = binMax;

   pBinArray->m_type = binType;

   return &(pBinArray->GetAt(index));
   }

inline
Bin *MapLayer::AddBin(int col, COLORREF color, LPCTSTR value, LPCTSTR label)
   {
   Bin bin;
   bin.SetLabel(label);
   bin.SetColor(color);
   bin.m_minVal = 0.0f;
   bin.m_maxVal = 0.0f;
   bin.m_strVal = value;

   BinArray *pBinArray = GetBinArray(col, true);
   int index = (int)pBinArray->Add(bin);
   pBinArray->m_type = TYPE_STRING;

   return &(pBinArray->GetAt(index));
   }

inline
int MapLayer::InitPolyBins(void)
   {
   int polys = (int)m_polyBinArray.GetSize();
   if (polys > 0)
      {
      Bin **pBase = m_polyBinArray.GetData();
      memset(pBase, 0, polys * sizeof(Bin*));
      }

   return polys;
   }


/////////////////////////////////////////////////////////////////////////////
// PourPointDlg dialog

#ifndef NO_MFC    
#if !defined IDD_POURPOINT
#define IDD_POURPOINT 0
#endif

class PourPointDlg : public CDialog
   {
   // Construction
   public:
      PourPointDlg(CWnd* pParent = NULL);   // standard constructor

      // Dialog Data
      //{{AFX_DATA(PourPointDlg)
      enum { IDD = IDD_POURPOINT };
      int		m_col;
      int		m_row;
      //}}AFX_DATA


      // Overrides
      // ClassWizard generated virtual function overrides
      //{{AFX_VIRTUAL(PourPointDlg)
   protected:
      virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
      //}}AFX_VIRTUAL

      // Implementation
   protected:

      // Generated message map functions
      //{{AFX_MSG(PourPointDlg)
      virtual BOOL OnInitDialog();
      //}}AFX_MSG
      DECLARE_MESSAGE_MAP()
   };
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

inline
FIELD_ATTR *MAP_FIELD_INFO::FindAttributeFromLabel(LPCTSTR label, int *index /*=NULL*/)
   {
   for (int i = 0; i < this->GetAttributeCount(); i++)
      {
      FIELD_ATTR &attr = attributes[i];

      if (attr.label.CompareNoCase(label) == 0)
         {
         if (index != NULL)
            *index = i;
         return &attr;
         }
      }

   return NULL;
   }


inline
int MapLayer::GetFieldInfoCount(int flags)
   {
   // flags=0 returns just fields, 1=return all
   if (flags == 1)
      return (int)m_pFieldInfoArray->GetSize();

   int count = 0;
   for (int i = 0; i < (int)m_pFieldInfoArray->GetSize(); i++)
      {
      MAP_FIELD_INFO *pInfo = m_pFieldInfoArray->GetAt(i);

      if (pInfo->mfiType != MFIT_SUBMENU)
         count++;
      }

   return count;
   }

inline
void MapLayer::SetBins(MapLayer *pSourceLayer, int sourceCol)
   {
   this->m_binArrayArray.RemoveAll();

   BinArray *pSourceBinArray = pSourceLayer->GetBinArray(sourceCol, false);

   if (pSourceBinArray)
      {
      BinArray *pBinArray = new BinArray(*(pSourceLayer->GetBinArray(sourceCol, false)));
      if (pBinArray)
         this->m_binArrayArray.Add(pBinArray);
      }
   }

