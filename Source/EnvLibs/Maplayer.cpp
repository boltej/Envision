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
// MapLayer.cpp: implementation of the MapLayer class.
//
//////////////////////////////////////////////////////////////////////

#include "EnvLibs.h"
#pragma hdrstop

#pragma warning( push )
#pragma warning( disable : 4244 )  // warning C4244: '=' : conversion from 'double' to 'REAL', possible loss of data

#include <iostream>

#include "Maplayer.h"
#include "MAP.h"
#include "Path.h"
#include "PathManager.h"
#include <math.h>
#include "GEOMETRY.HPP"
#include "UNITCONV.H"
#include "Report.h"
#include "QueryEngine.h"
#include "Vdataobj.h"
#include "IDATAOBJ.H"
#include "FDATAOBJ.H"
#include "DDATAOBJ.H"
#include "tinyxml.h"
#include <float.h>
#include <omp.h>
#include <tixml.h>
#include <misc.h>

#include "shapefil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


MapLayer *pMapLayer;


FIELD_ATTR *MAP_FIELD_INFO::FindAttribute(VData &value, int *index /*=NULL*/)
   {
   if (::IsInteger(value.type))
      {
      int val = 0;
      value.GetAsInt(val);
      return FindAttribute(val, index);
      }

   else if (::IsReal(value.type))
      {
      float val = 0;
      value.GetAsFloat(val);
      return FindAttribute(val, index);
      }
   else if (::IsString(value.type))
      return FindAttribute(value.val.vString, index);

   else
      return NULL;
   }


FIELD_ATTR *MAP_FIELD_INFO::FindAttribute(float value, int *index /*=NULL*/)
   {
   for (int i = 0; i < this->GetAttributeCount(); i++)
      {
      FIELD_ATTR &attr = attributes[i];

      if (attr.minVal == attr.maxVal)  // categorical
         {
         float _value;
         attr.value.GetAsFloat(_value);
         if (value == _value)
            {
            if (index != NULL)
               *index = i;
            return &attr;
            }
         }
      else     // range
         {
         if (attr.minVal <= value && value <= attr.maxVal)
            {
            if (index != NULL)
               *index = i;
            return &attr;
            }
         }
      }

   return NULL;
   }


FIELD_ATTR *MAP_FIELD_INFO::FindAttribute(int value, int *index /*=NULL*/)
   {
   for (int i = 0; i < this->GetAttributeCount(); i++)
      {
      FIELD_ATTR &attr = attributes[i];

      if (attr.minVal == attr.maxVal)  // categorical
         {
         int _value;
         attr.value.GetAsInt(_value);
         if (value == _value)
            {
            if (index != NULL)
               *index = i;
            return &attr;
            }
         }
      else     // range
         {
         if (attr.minVal <= value && value <= attr.maxVal)
            {
            if (index != NULL)
               *index = i;
            return &attr;
            }
         }
      }

   return NULL;
   }


FIELD_ATTR* MAP_FIELD_INFO::FindAttribute(LPCTSTR value, int* index /*=NULL*/)
   {
   if (value == NULL)
      return NULL;

   for (int i = 0; i < this->GetAttributeCount(); i++)
      {
      FIELD_ATTR& attr = attributes[i];

      CString _value;
      attr.value.GetAsString(_value);
      if (_value.Compare(value) == 0)
         {
         if (index != NULL)
            *index = i;
         return &attr;
         }
      }

   return NULL;
   }



void FieldInfoArray::Copy(FieldInfoArray &f)
   {
   m_path = f.m_path;
   Clear();
   for (int i = 0; i < (int)f.GetSize(); i++)
      {
      MAP_FIELD_INFO *pSourceInfo = f.GetAt(i);
      MAP_FIELD_INFO *pThisInfo = new MAP_FIELD_INFO(*pSourceInfo);
      Add(pThisInfo);
      }

   // fix up any parent ptrs
   for (int i = 0; i < (int)GetSize(); i++)
      {
      MAP_FIELD_INFO *pInfo = GetAt(i);

      if (pInfo->pParent != NULL)
         {
         // at this point, the parent lives in the source field array.
         // find the index of the parent and reset this parent ptr
         for (int j = 0; j < (int)f.GetSize(); j++)
            {
            MAP_FIELD_INFO *pSourceInfo = f.GetAt(j);

            if (pInfo->pParent == pSourceInfo)
               {
               pInfo->pParent = this->GetAt(j);
               break;
               }
            }
         }
      }
   }


int FieldInfoArray::AddFieldInfo(LPCTSTR fieldname, /*int level,*/ LPCTSTR label, LPCTSTR desc, LPCTSTR source, int type, MFI_TYPE mfiType, /*int binCount,*/ int displayFlags, int bsFlag, float binMin, float binMax, bool save)
   {
   // does an entry for this field already exist?
   for (int i = 0; i < GetSize(); i++)
      {
      MAP_FIELD_INFO *pInfo = GetAt(i);

      if (pInfo->fieldname.CompareNoCase(fieldname) == 0)
         {
         CString msg("Field Information for ");
         msg += fieldname;
         msg += " was previously defined...ignoring";
         Report::WarningMsg(msg);
         return  -1;
         }
      }

   return Add(new MAP_FIELD_INFO(fieldname, /*level,*/ label, desc, source, type, mfiType, /*binCount,*/ displayFlags, bsFlag, binMin, binMax, save));
   }


int FieldInfoArray::SetFieldInfo(LPCTSTR fieldname, /*int level,*/ LPCTSTR label, LPCTSTR desc, LPCTSTR source, int type, MFI_TYPE mfiType,
   int displayFlags, int bsFlag/*=0*/, float binMin/*=-1.0f*/, float binMax/*=-1.0f*/, bool save/*=false*/)
   {
   for (int i = 0; i < GetSize(); i++)
      {
      MAP_FIELD_INFO *pInfo = GetAt(i);
      if (lstrcmpi(fieldname, pInfo->fieldname) == 0)
         {
         //pInfo->level = level;
         pInfo->label = label;
         pInfo->description = desc;
         pInfo->source = source;
         pInfo->type = (TYPE)type;
         pInfo->mfiType = mfiType;
         pInfo->displayFlags = displayFlags;
         pInfo->bsFlag = bsFlag;
         pInfo->binMin = binMin;
         pInfo->binMax = binMax;
         pInfo->save = save;
         return i;
         }
      }

   return AddFieldInfo(fieldname, /*level,*/ label, desc, source, type, mfiType, displayFlags, bsFlag, binMin, binMax, save);
   }


MAP_FIELD_INFO *FieldInfoArray::FindFieldInfo(LPCTSTR fieldname, int *pIndex /*=NULL*/) const
   {
   for (int i = 0; i < GetSize(); i++)
      {
      if (lstrcmpi(fieldname, GetAt(i)->fieldname) == 0)
         {
         if (pIndex != NULL)
            *pIndex = i;

         return GetAt(i);
         }
      }

   if (pIndex != NULL)
      *pIndex = -1;

   return NULL;
   }

MAP_FIELD_INFO *FieldInfoArray::FindFieldInfoFromLabel(LPCTSTR label) const
   {
   for (int i = 0; i < GetSize(); i++)
      {
      if (lstrcmpi(label, GetAt(i)->label) == 0)
         return GetAt(i);
      }
   return NULL;
   }


int __cdecl Compare(const void *elem0, const void *elem1);
int __cdecl CompareCoords(const void *arg0, const void *arg1);

void SortStringArray(CStringArray &array);
int __cdecl SortStringProc(const void *elem1, const void *elem2);

WORD   SwapTwoBytes(WORD w);
DWORD  SwapFourBytes(DWORD dw);

WORD   GetBigWord(FILE *fp);
DWORD  GetBigDWORD(FILE *fp);
double GetBigDouble(FILE *fp);

WORD   GetLittleWord(FILE *fp);
DWORD  GetLittleDWORD(FILE *fp);
double GetLittleDouble(FILE *fp);

void   PutBigWord(WORD w, FILE *fp);
void   PutBigDWORD(DWORD dw, FILE *fp);

void   PutLittleWord(WORD w, FILE *fp);
void   PutLittleDWORD(DWORD dw, FILE *fp);
void   PutLittleDouble(double d, FILE *fp);

void SET_FLAG(int *flag, int value) { if (flag != NULL) *flag = value; }
void CHECK_VALID_NUMBER(double value) { ASSERT(_isnan(value) == 0); }


////////////////////////////////////////////////////////////////////////////
// Poly

void Poly::TraceVertices(int levels)
   {
   int count = GetVertexCount();

   if (count > 3)
      {
      for (int i = 0; i < count; i++)
         {
         if (i < levels || i > count - levels - 1)
            {
            //CString msg;
            //msg.Format("Vertex %i: x=%.1f, y=%.1f\n", i, GetVertex(i).x, GetVertex(i).y);
            //TRACE(msg);
            }
         }
      }
   }


Poly& Poly::operator=(const Poly &poly)
   {
   m_id = poly.m_id;
   //m_pBin  = NULL;
   m_extra = poly.m_extra;

   m_xMax = poly.m_xMax;
   m_xMin = poly.m_xMin;
   m_yMax = poly.m_yMax;
   m_yMin = poly.m_yMin;

   if (poly.m_children.GetCount() > 0)
      m_children.Copy(poly.m_children);

   if (poly.m_parents.GetCount() > 0)
      m_parents.Copy(poly.m_parents);      // added jpb 3/24/06

   // take care of copying the m_ptArray.  Note - thre is one logical POINT for each vertex
   int count = this->GetVertexCount();
   if (count == 0)
      m_ptArray = NULL;
   else
      {
      m_ptArray = new POINT[count];
      memcpy(m_ptArray, poly.m_ptArray, count * sizeof(POINT));
      }

   // take care of copying the vertex array
   if (poly.m_vertexArray.GetSize() > 0)
      {
      m_vertexArray.SetSize(poly.m_vertexArray.GetSize());
      m_vertexArray.Copy(poly.m_vertexArray);
      }

   // take care of copying the vertexPart array
   if (poly.m_vertexPartArray.GetSize() > 0)
      {
      m_vertexPartArray.SetSize(poly.m_vertexPartArray.GetSize());
      m_vertexPartArray.Copy(poly.m_vertexPartArray);
      }

   return *this;
   }

Poly::~Poly()
   {
   ClearPoly();
   }


void Poly::ClearPoly()
   {
   m_vertexArray.RemoveAll();
   m_vertexPartArray.RemoveAll();

   if (m_ptArray != NULL)
      {
      delete[] m_ptArray;
      m_ptArray = NULL;
      }

   //m_pBin = NULL;
   }


void Poly::AddVertex(const Vertex &v, bool updatePtArray /*=true*/)
   {
   m_vertexArray.Add((Vertex&)v);

   if (this->GetVertexCount() == 1 && m_vertexPartArray.GetSize() == 0) // first vertex?
      this->m_vertexPartArray.Add(0);

   if (v.x < this->m_xMin)
      this->m_xMin = v.x;
   else if (v.x > this->m_xMax)
      this->m_xMax = v.x;

   if (v.y < this->m_yMin)
      this->m_yMin = v.y;
   else if (v.y > this->m_yMax)
      this->m_yMax = v.y;

   if (updatePtArray)
      {
      if (m_ptArray != NULL)
         delete m_ptArray;

      m_ptArray = new POINT[m_vertexArray.GetSize()];  // allocate point array
      }
   }


void Poly::InitLogicalPoints(Map *pMap)
   {
   int vertexCount = m_vertexArray.GetSize();

   if (m_ptArray == NULL)
      m_ptArray = new POINT[vertexCount];  // allocate point array

   for (int i = 0; i < vertexCount; i++)
      {
      // get world coords
      REAL x = m_vertexArray[i].x;
      REAL y = m_vertexArray[i].y;
      pMap->VPtoLP(x, y, m_ptArray[i]);
      }
   }


void Poly::Translate(float dx, float dy)
   {
   for (int i = 0; i < m_vertexArray.GetSize(); i++)
      {
      m_vertexArray[i].x += dx;
      m_vertexArray[i].y += dy;
      }

   m_xMin += dx;
   m_xMax += dx;
   m_yMin += dy;
   m_yMax += dy;
   }


// set the label point. Note that the label point is in virtual coords
void Poly::SetLabelPoint(SLPMETHOD method)
   {
   switch (method)
      {
      case SLP_CENTROID:
         m_labelPoint = this->GetCentroid();
         break;

      case SLP_ABOVE:
      case SLP_BELOW:
         ;  // not implemented yet...
      }
   }


bool Poly::GetBoundingRect(REAL &xMin, REAL &yMin, REAL &xMax, REAL &yMax) const
   {
   xMin = m_xMin;
   xMax = m_xMax;
   yMin = m_yMin;
   yMax = m_yMax;

   return true;
   }


void Poly::UpdateBoundingBox(void)
   {
   REAL xMin = (REAL)LONG_MAX;
   REAL xMax = (REAL)LONG_MIN;
   REAL yMin = (REAL)LONG_MAX;
   REAL yMax = (REAL)LONG_MIN;

   for (int i = 0; i < m_vertexArray.GetSize(); i++)
      {
      if (m_vertexArray[i].x < xMin) xMin = m_vertexArray[i].x;
      if (m_vertexArray[i].x > xMax) xMax = m_vertexArray[i].x;
      if (m_vertexArray[i].y < yMin) yMin = m_vertexArray[i].y;
      if (m_vertexArray[i].y > yMax) yMax = m_vertexArray[i].y;
      }

   m_xMin = xMin;
   m_xMax = xMax;
   m_yMin = yMin;
   m_yMax = yMax;
   }


bool Poly::GetBoundingZ(float &zMin, float &zMax) const
   {
   int count = GetVertexCount();

   if (count == 0)
      return false;

   zMin = 10e16f;
   zMax = -10e16f;

   for (int i = 0; i < count; i++)
      {
      Vertex &v = ((Poly*)this)->GetVertex(i);

      if (v.z < zMin) zMin = (float)v.z;
      if (v.z > zMax) zMax = (float)v.z;
      }

   return true;
   }


// Calculate  Area of a polygon.
// The Polygon array is assumed to be closed, i.e.,
// Polygon[0] = Polygon[High(Polygon)]  
//
// The algebraic sign of the area is negative for counterclockwise
// ordering of vertices in the X-Y plane, and positive for
// clockwise ordering.  Hence the absolute value
//
// Area calculation is also provided in GetArea()
//	
// This is an implementation of 'Greens Theorem'.	
//
// Reference:  "Centroid of a Polygon" in Graphics Gems IV,
// Paul S. Heckbert (editor), Academic Press, 1994, pp. 3-6.

float Poly::GetArea() const
   {
   double area = 0.0f;

   int parts = m_vertexPartArray.GetSize();

   if (parts == 0)
      return 0;

   int startPart = m_vertexPartArray[0];
   int nextPart = 0;

   for (int part = 0; part < parts; part++)
      {
      if (part < parts - 1)
         nextPart = m_vertexPartArray[part + 1];
      else
         nextPart = GetVertexCount();

      ASSERT((nextPart - startPart) >= 4); // a ring is composed of 4 or more points

      for (int j = startPart; j < nextPart - 1; j++)
         {
         const Vertex &v1 = m_vertexArray[j];
         const Vertex &v2 = m_vertexArray[j + 1];

         double v1x = double(v1.x);
         double v1y = double(v1.y);
         double v2x = double(v2.x);
         double v2y = double(v2.y);

         area += v2x*v1y - v1x *v2y;
         }

      startPart = nextPart;
      }

   return float(area / 2.0);
   }


float Poly::GetEdgeLength() const
   {
   float length = 0.0f;

   int parts = m_vertexPartArray.GetSize();

   if (parts == 0)
      return 0;

   int startPart = m_vertexPartArray[0];
   int nextPart = 0;

   for (int part = 0; part < parts; part++)
      {
      if (part < parts - 1)
         nextPart = m_vertexPartArray[part + 1];
      else
         nextPart = GetVertexCount();

      for (int j = startPart; j < nextPart - 1; j++)
         {
         const Vertex &v1 = m_vertexArray[j];
         const Vertex &v2 = m_vertexArray[j + 1];

         double v1x = double(v1.x);
         double v1y = double(v1.y);
         double v2x = double(v2.x);
         double v2y = double(v2.y);

         length += sqrt((v2x - v1x)*(v2x - v1x) + (v2y - v1y)*(v2y - v1y));
         }

      startPart = nextPart;
      }

   return float(length);
   }


INT_PTR PolyArray::GetPolyIndex(Poly *pPoly)
   {
   INT_PTR count = GetSize();
   for (INT_PTR i = 0; i < count; i++)
      if (GetAt(i) == pPoly)
         return i;

   return -1;
   }


// Calculate Centroid and Area of a polygon.
// The Polygon array is assumed to be closed, i.e.,
// Polygon[0] = Polygon[High(Polygon)]  
//
// The algebraic sign of the area is negative for counterclockwise
// ordering of vertices in the X-Y plane, and positive for
// clockwise ordering.  Hence the absolute value
//
// Area calculation is also provided in GetArea()
//	
// This is an implementation of 'Greens Theorem'.	
//
// Reference:  "Centroid of a Polygon" in Graphics Gems IV,
// Paul S. Heckbert (editor), Academic Press, 1994, pp. 3-6.
//


Vertex MapLayer::GetCentroidFromPolyOffsets(CUIntArray *polyIndexArray)
   {
   double xSum = 0.0f;
   double ySum = 0.0f;
   double aSum = 0.0f;
   //   double term;
   Vertex centroid;
   int i, j;
   VertexArray vertexArray;

   for (int i = 0; i < polyIndexArray->GetSize(); i++)
      {
      Poly *pPoly = GetPolygon(polyIndexArray->GetAt(i));
      Vertex v = pPoly->GetCentroid();
      vertexArray.Add(v);
      }

   int vertexCount = vertexArray.GetCount();
   if (vertexCount < 3)
      centroid = vertexArray[0];
   else
      {
      double ai;

      for (i = vertexCount - 1, j = 0; j < vertexCount; i = j, j++)
         {
         double xi = vertexArray[i].x;
         double xj = vertexArray[j].x;
         double yi = vertexArray[i].y;
         double yj = vertexArray[j].y;

         ai = xi * yj - xj * yi;
         aSum += ai;
         xSum += (xj + xi) * ai;
         ySum += (yj + yi) * ai;
         }

      centroid.x = float(xSum / (3.0 * aSum));
      centroid.y = float(ySum / (3.0 * aSum));
      }

   return centroid;
   }


const Vertex Poly::GetCentroid() const
   {
   int vertexCount = m_vertexArray.GetCount();

   Vertex vStart = m_vertexArray.GetAt(0);
   Vertex centroid;

   if (vertexCount == 1)
      {
      centroid.x = m_vertexArray[0].x;
      centroid.y = m_vertexArray[0].y;
      return centroid;
      }


   double xSum = 0.0f;
   double ySum = 0.0f;
   double aSum = 0.0f;
   //   double term;

   bool isClosed = (vStart.Compare(m_vertexArray[vertexCount - 1]));

   if (isClosed)
      vertexCount--;
   /*

   int parts = m_vertexPartArray.GetSize();
   ASSERT( parts > 0 ); //must have atleast one part

   int startPart = m_vertexPartArray[ 0 ];
   int nextPart = 0;

   for ( int part=0; part < parts; part++ )
      {
      if ( part < parts-1 )
         nextPart = m_vertexPartArray[ part+1 ];
      else
         nextPart = GetVertexCount();

      ASSERT( nextPart - startPart >= 4 ); // a ring is composed of 4 or more points

      for ( int j=startPart; j<nextPart-1; j++ )
         {
         const Vertex &v1 = m_vertexArray[ j ];
         const Vertex &v2 = m_vertexArray[ j+1 ];

         term = v2.x * v1.y - v1.x * v2.y;
         aSum += term;
         xSum += (v2.x + v1.x) * term;
         ySum += (v2.y + v1.y) * term;
         }

      startPart = nextPart;
      }
      */
   register int i, j;
   double ai;

   for (i = vertexCount - 1, j = 0; j < vertexCount; i = j, j++)
      {
      double xi = m_vertexArray[i].x;
      double xj = m_vertexArray[j].x;
      double yi = m_vertexArray[i].y;
      double yj = m_vertexArray[j].y;

      ai = xi * yj - xj * yi;
      aSum += ai;
      xSum += (xj + xi) * ai;
      ySum += (yj + yi) * ai;
      }

   if (aSum != 0.0f)

      {
      centroid.x = float(xSum / (3.0 * aSum));
      centroid.y = float(ySum / (3.0 * aSum));
      }
   else
      {
      centroid.x = m_vertexArray[0].x;
      centroid.y = m_vertexArray[0].y;
      }

   return centroid;
   }


float Poly::CalcSlope(void)
   {
   int vertexCount = m_vertexArray.GetSize();

   ASSERT(vertexCount > 1);

   Vertex vmin = m_vertexArray[0];
   Vertex vmax = m_vertexArray[0];
   float maxz = -1E20f;
   float maxx = -1E20f;
   float maxy = -1E20f;

   float minz = 1E20f;
   float minx = 1E20f;
   float miny = 1E20f;

   for (int i = 0; i < vertexCount; i++)
      {
      if (m_vertexArray[i].z > maxz)
         {
         vmax = m_vertexArray[i];
         maxz = (float)vmax.z;
         maxx = (float)vmax.x;
         maxy = (float)vmax.y;

         }

      if (m_vertexArray[i].z < minz)
         {
         vmin = m_vertexArray[i];
         minz = (float)vmin.z;
         minx = (float)vmin.x;
         miny = (float)vmin.y;
         }

      }

   float distance = ((minx - maxx) * (minx - maxx)) + ((miny - maxy) * (miny - maxy));
   distance = (float)sqrt(distance);

   float slope;

   if (distance > 0.0f)
      slope = (maxz - minz) / distance;
   else
      slope = 0.001f;

   if (slope <= 0.0f)
      slope = 0.001f;

   return slope;
   }


float Poly::CalcElevation(void)
   {
   int vertexCount = m_vertexArray.GetSize();
   ASSERT(vertexCount > 0);

   float z = 0;

   for (int i = 0; i < vertexCount; i++)
      z += m_vertexArray[i].z;

   z /= vertexCount;

   return z;
   }


float Poly::CalcDistance(void)
   {
   int vertexCount = m_vertexArray.GetSize();
   ASSERT(vertexCount > 1);
   Vertex vmin = m_vertexArray[0];
   Vertex vmax = m_vertexArray[0];
   float maxz = -1E20f;
   float maxx = -1E20f;
   float maxy = -1E20f;

   float minz = 1E20f;
   float minx = 1E20f;
   float miny = 1E20f;

   for (int i = 0; i < vertexCount; i++)
      {
      if (m_vertexArray[i].z > maxz)
         {
         vmax = m_vertexArray[i];
         maxz = (float)vmax.z;
         maxx = (float)vmax.x;
         maxy = (float)vmax.y;
         }

      if (m_vertexArray[i].z < minz)
         {
         vmin = m_vertexArray[i];
         minz = (float)vmin.z;
         minx = (float)vmin.x;
         miny = (float)vmin.y;
         }

      }

   float distance = ((minx - maxx) * (minx - maxx)) + ((miny - maxy) * (miny - maxy));
   distance = (float)sqrt(distance);

   return distance;
   }


float Poly::CalcLengthSlope(float area, float slope)  // slope is non-decimal percent, area is HECTARES
   {
   //slope = PERCENT_DEGREE( slope );
   slope = RISERUN_PER_DEGREE(slope); // convert to degrees
   //ASSERT ( slope < 180.0f ); //???
   //ASSERT ( slope >= 0.0f );

   float lengthSlope = (float)(pow(sin(slope * RAD_PER_DEGREE) / 0.0896, 1.3)
      * pow(area / 22.13, 0.4));
   return lengthSlope;
   }


bool Poly::DoesBoundingBoxIntersect(const Poly *pPoly) const
   {
   REAL xMinOther = 0.0f, yMinOther = 0.0f, xMaxOther = 0.0f, yMaxOther = 0.0f;
   REAL xMinThis = 0.0f, yMinThis = 0.0f, xMaxThis = 0.0f, yMaxThis = 0.0f;

   // get the bouding rect for the "other" polygon
   bool ok = pPoly->GetBoundingRect(xMinOther, yMinOther, xMaxOther, yMaxOther);

   ok = this->GetBoundingRect(xMinThis, yMinThis, xMaxThis, yMaxThis);

   // note: there are 2 cases where an intersection does *NOT* occur:
   //
   //   this:              _______
   // --------------------------------------------
   //   other   1) _____                  ( xMaxOther < xMinThis )
   //           2)                  _____ ( xMinOther > xMaxThis )
   //
   // anything else is an intersection
   //
   // repeates with y, and both x and y must intersect for the bounding boxes to intersect

   // check X intersection first
   if ((xMaxOther < xMinThis)    // case 1 
      || (xMinOther > xMaxThis))  // case 2
      return false;  // no reason to look further

   // check y intersection next
   if ((yMaxOther < yMinThis)    // case 1 
      || (yMinOther > yMaxThis))  // case 2
      return false;

   return true;
   }


bool Poly::DoesEdgeIntersectLine(Vertex &v0, Vertex &v1) const
   {
#ifdef NO_MFC
   using namespace std; //min()/max()
#endif

   // if bounding rects don't interect, no need to look further
   Vertex vMin, vMax;
   vMin.x = min(v0.x, v1.x);
   vMin.y = min(v0.y, v1.y);
   vMax.x = max(v0.x, v1.x);
   vMax.y = max(v0.y, v1.y);

   if (!DoRectsIntersect(vMin.x, vMin.y, vMax.x, vMax.y, m_xMin, m_yMin, m_xMax, m_yMax))
      return false;

   int thisVertexCount = GetVertexCount();

   // interate though every edge of the the passed in line segment, check for intersections
   for (int j = 0; j < thisVertexCount - 1; j++)		// polygon segments
      {
      Vertex &thisVertex0 = ((Poly*)this)->GetVertex(j);
      Vertex &thisVertex1 = ((Poly*)this)->GetVertex(j + 1);
      Vertex intersectionVertex;

      bool ok = GetIntersectionPt(thisVertex0, thisVertex1, v0, v1, intersectionVertex);

      if (ok)
         return true;
      }

   return false;
   }



int Poly::GetIntersectionPoints(Vertex &v0, Vertex &v1, VertexArray &intersectionPts) const
   {
#ifdef NO_MFC
   using namespace std; //for min()/max()
#endif

   intersectionPts.RemoveAll();

   // if bounding rects don't interect, no need to look further
   Vertex vMin, vMax;
   vMin.x = min(v0.x, v1.x);
   vMin.y = min(v0.y, v1.y);
   vMax.x = max(v0.x, v1.x);
   vMax.y = max(v0.y, v1.y);

   if (!DoRectsIntersect(vMin.x, vMin.y, vMax.x, vMax.y, m_xMin, m_yMin, m_xMax, m_yMax))
      return 0;

   int thisVertexCount = GetVertexCount();

   // interate though every edge of the the passed in line segment, check for intersections
   for (int j = 0; j < thisVertexCount - 1; j++)		// polygon segments
      {
      Vertex &thisVertex0 = ((Poly*)this)->GetVertex(j);
      Vertex &thisVertex1 = ((Poly*)this)->GetVertex(j + 1);
      Vertex intersectionVertex;

      bool ok = GetIntersectionPt(thisVertex0, thisVertex1, v0, v1, intersectionVertex);

      if (ok)
         intersectionPts.Add(intersectionVertex);
      }

   return intersectionPts.GetSize();
   }



// returns the length of the intersection of pLine (treated as a line) with this polygon (assumes "this" is a closed polygon)
REAL Poly::GetIntersectionLength(const Poly *pLine) const
   {
#ifdef NO_MFC
   using namespace std; //for min()
#endif

   if (DoesBoundingBoxIntersect(pLine) == false)
      return 0.0f;

   int thisVertexCount = GetVertexCount();
   int otherVertexCount = pLine->GetVertexCount();

   //COORD3d  coordCache[ 32 ];      // temporary cache for vertex intersections
   //COORD3d *coordCachePtr[ 32 ];   // temporary cache for vertex intersections

   // check segments on this poly with vertices on the other poly
   REAL length = 0.0f;

   // interate though every edge of the the passed in line segment, check for intersections   
   for (int i = 0; i < otherVertexCount - 1; i++)
      {
      Vertex lineVertex0 = ((Poly*)pLine)->GetVertex(i);
      Vertex lineVertex1 = ((Poly*)pLine)->GetVertex(i + 1);

      VertexArray intersectionPts;
      int pts = GetIntersectionPoints(lineVertex0, lineVertex1, intersectionPts);

      //   1) the line segment is completely inside the polygon: there are no intersections but we add the entire segment length
      //   2) the line segment is completely outside the polygon: there are no intersections and no length is added
      //   3) the line segment crosses the polygon:  we add the portion of the segment length that is inside the polygon

      switch (pts)
         {
         case 0:     // case 1, 2 above
            {
            if (IsPointInPoly(lineVertex0)) // case 1 above
               length += LineLength(lineVertex0.x, lineVertex0.y, lineVertex1.x, lineVertex1.y);  // both inside
            }
            break;
            // the rest cover case 3 above
         case 1:     // one end in, the other end out
            {
            if (IsPointInPoly(lineVertex0)) // "0" end is in , "1" end is out
               length += LineLength(lineVertex0.x, lineVertex0.y, intersectionPts[0].x, intersectionPts[0].y);
            else
               length += LineLength(lineVertex1.x, lineVertex1.y, intersectionPts[0].x, intersectionPts[0].y);
            }
            break;

         case 2:  // both ends in or both end out
            {
            if (IsPointInPoly(lineVertex0)) // both in
               {
               float l0 = LineLength(lineVertex0.x, lineVertex0.y, intersectionPts[0].x, intersectionPts[0].y);
               float l1 = LineLength(lineVertex0.x, lineVertex0.y, intersectionPts[1].x, intersectionPts[1].y);
               length += min(l0, l1);

               l0 = LineLength(lineVertex1.x, lineVertex1.y, intersectionPts[0].x, intersectionPts[0].y);
               l1 = LineLength(lineVertex1.x, lineVertex1.y, intersectionPts[1].x, intersectionPts[1].y);
               length += min(l0, l1);
               }
            else  // both out
               {
               length += LineLength(intersectionPts[0].x, intersectionPts[0].y, intersectionPts[1].x, intersectionPts[1].y);
               }
            }
            break;

         default:     // more complex shapes are not handled for now.
            ASSERT(0);
         }  // end of : switch()

      }  // end of: for ( i < otherVertexCount-1)

   return length;
   }
/*
// build cache of intersection points by interating through this polygon.  First point in cache is the first vertex of this edge
coordCache[ 0 ] = pLine->m_vertexArray[ i ];
int count = 1;    // one coord cached so far

// now, loop throught this poly's edges, looking for intersection pts.
// there are several possible cases:
//   1) the line segment is completely inside the polygon: there are no intersections but we add the entire segment length
//   2) the line segment is completely outside the polygon: there are no intersections and no length is added
//   3) the line segment crosses the polygon:  we add the portion of the segment length that is inside the polygon

for ( int j=0; j < thisVertexCount - 1; j++ )		// polygon segments
   {
   const Vertex &thisVertex0 = GetVertex( j );
   const Vertex &thisVertex1 = GetVertex( j + 1 );
   Vertex intersectionVertex;

   bool ok  = GetIntersectionPt( thisVertex0, thisVertex1, lineVertex0, lineVertex1, intersectionVertex );

   if ( ok )  // case 3 above
      {
      // find out which line segment vertext is inside the polygon
      bool isV0inPoly = IsPointInPoly( lineVertex0 );
      bool isV1inPoly = IsPointInPoly( lineVertex1 );
 ASSERT( count < 32 );

   if ( ok )
      {
      coordCache[ count ] = intersectionVertex;
      count++;
      }
   }  // end of:  for ( j < thisVertexCount-1 )

// added all intersection points, add last end point
coordCache[ count ] = pPoly->m_vertexArray[ i+1 ];
count++;

// sort cache
for ( int j=0; j < count; j++ )
   coordCachePtr[ j ] = coordCache+j;

qsort( (void*) coordCachePtr, (size_t) count, sizeof( COORD3d* ), CompareCoords );

// coords are sorted, now iterate through to check midpoints
for ( int j=0; j < count-1; j++ )
   {
   Vertex midPt;
   midPt.x = ( coordCachePtr[ j ]->x + coordCachePtr[ j+1 ]->x )/2;
   midPt.y = ( coordCachePtr[ j ]->y + coordCachePtr[ j+1 ]->y )/2;
   bool ok = IsPointInPoly( midPt );

   if ( ok )
      {
      REAL segmentLength = (float) DistancePtToPt( coordCachePtr[j]->x, coordCachePtr[j]->y,  coordCachePtr[j+1]->x, coordCachePtr[j+1]->y);
      length += segmentLength;
      }
   }

// done with one segment of the passed in polygon
}  // end of:  for ( i < otherVertexCount-1

// done with all segments of the passed in poly, return
return length;
}*/


int CompareCoords(const void *arg0, const void *arg1)
   {
   COORD3d **pC0 = (COORD3d**)arg0;
   COORD3d **pC1 = (COORD3d**)arg1;

   if ((*pC0)->x != (*pC1)->x)
      return (((*pC1)->x - (*pC0)->x) > 0) ? 1 : -1;

   else
      return (((*pC1)->y - (*pC0)->y) > 0) ? 1 : -1;
   }


bool Poly::IsPointInPoly(Vertex &point) const
   {
   // check bounding rect first
   if (point.x < m_xMin || point.x > m_xMax)
      return false;
   if (point.y < m_yMin || point.y > m_yMax)
      return false;

   int parts = m_vertexPartArray.GetSize();
   if (parts == 0)
      return false;

   int startPart = m_vertexPartArray[0];
   int nextPart = 0;

   // Origin vertex
   Vertex origin(0.0, 0.0);

   //-- intermediate verticies --//
   const Vertex *v0 = &origin;
   const Vertex *v1 = NULL;

   bool isIn = false;

   for (int part = 0; part < parts; part++)
      {
      if (part < parts - 1)
         nextPart = m_vertexPartArray[part + 1];
      else
         nextPart = GetVertexCount();

      ASSERT(nextPart - startPart >= 4); // a ring is composed of 4 or more points

      for (int j = startPart; j < nextPart; j++)
         {
         v1 = (Vertex*)&m_vertexArray[j];
         _IsPointInPoly(v0, v1, point, isIn);
         v0 = v1;
         }

      v1 = &origin;
      _IsPointInPoly(v0, v1, point, isIn);
      v0 = v1;

      startPart = nextPart;
      }

   return isIn;
   }

// integer version
bool Poly::IsPointInPoly(POINT &point) const
   {
   // NOTE: There is not bounding rect in logical coords, could convert.  Would that be more efficient?

   int parts = m_vertexPartArray.GetSize();
   ASSERT(parts > 0); //must have atleast one part

   int startPart = m_vertexPartArray[0];
   int nextPart = 0;

   // Origin vertex
   POINT origin;
   origin.x = 0;
   origin.y = 0;

   //-- intermediate verticies --//
   const POINT *v0 = &origin;
   const POINT *v1 = NULL;

   bool isIn = false;

   for (int part = 0; part < parts; part++)
      {
      if (part < parts - 1)
         nextPart = m_vertexPartArray[part + 1];
      else
         nextPart = GetVertexCount();

      ASSERT(nextPart - startPart >= 4); // a ring is composed of 4 or more points

      for (int j = startPart; j < nextPart; j++)
         {
         v1 = m_ptArray + j; //(Vertex*) &m_vertexArray[ j ];
         _IsPointInPoly(v0, v1, point, isIn);
         v0 = v1;
         }

      v1 = &origin;
      _IsPointInPoly(v0, v1, point, isIn);
      v0 = v1;

      startPart = nextPart;
      }

   return isIn;
   }


//-- Poly::NearestDistanceToPoly(Poly *pPoly ) -----------------------------------------------------
//
//  computes the shortest distance between two polygons, using a 4 part methodology
//
//  0) See if any of the vertex match.  return 0 distance if they do.
//  1) see if any of the segments cross, and return a distance of zero if they do
//  2) see if any of "this" polygons' line segments have perpendiculars that intersect any
//     of the passed in polygons vertices
//  3) see if any of the passed in polygons' line segments have perpendiculars that intersect any
//     of "this" polygons vertices
//  4) if 0, 1, 2 and 3 don't work, return the closest distance between any vertices in the two polygons
//
//  The threshold value indicates a maximum distance to consider (by checking bounding boxes).
//  If the bounding boxes are more than 'threshold' distance, compute an apprximate distance.
//  Specifying any negative value (the default) computes an actual distance in all circumstances
//---------------------------------------------------------------------------------------------------

REAL Poly::NearestDistanceToPoly(Poly *pToPoly, REAL threshold, int *retFlag) const
   {
   int thisVertexCount = GetVertexCount();
   int otherVertexCount = pToPoly->GetVertexCount();

   //CDWordArray &thisPartArray  = m_vertexPartArray;
   CDWordArray &otherPartArray = pToPoly->m_vertexPartArray;

   SET_FLAG(retFlag, -1);

   if (threshold > 0) // check bounding box
      {
      REAL xMinThis, yMinThis, xMaxThis, yMaxThis;
      GetBoundingRect(xMinThis, yMinThis, xMaxThis, yMaxThis);

      REAL xMinOther, yMinOther, xMaxOther, yMaxOther;
      pToPoly->GetBoundingRect(xMinOther, yMinOther, xMaxOther, yMaxOther);

      // have both bounding boxes, compute the distance between the two bounding boxes.
      float distance = DistanceRectToRect(xMinThis, yMinThis, xMaxThis, yMaxThis,
         xMinOther, yMinOther, xMaxOther, yMaxOther);

      if (distance > threshold)
         {
         SET_FLAG(retFlag, 0);
         return distance;
         }
      }
   else
      threshold = FLT_MAX;

   //---- closer than the threshold distance, so do a closer examination ----//

   // Step 0: do any of the vertex's match ( are they the same?)
   for (int i = 0; i < thisVertexCount; i++)
      for (int j = 0; j < otherVertexCount; j++)
         {
         if (m_vertexArray[i].x == pToPoly->m_vertexArray[j].x && m_vertexArray[i].y == pToPoly->m_vertexArray[j].y)
            {
            SET_FLAG(retFlag, 1);
            return (REAL)0;
            }
         }

   // Step 1:  check segments on this poly with vertices on the other poly
   // see if any segments cross         

   float distance = threshold;      // start large, try to find smaller distances
   int currentThisPart = 0;
   int totalThisParts = m_vertexPartArray.GetSize();

   int currentOtherPart = 0;
   int totalOtherParts = otherPartArray.GetSize();

   for (int i = 0; i < thisVertexCount - 1; i++)
      {
      // are we on a part boundary for this polygon? then skip
      if ((currentThisPart < totalThisParts - 1) && (m_vertexPartArray[currentThisPart + 1] - 1 == i))
         {
         ++currentThisPart;
         continue;
         }

      // not on a part boundary, so keep going
      Vertex &thisVertex0 = ((Poly*)this)->GetVertex(i);
      Vertex &thisVertex1 = ((Poly*)this)->GetVertex(i + 1);

      currentOtherPart = 0;
      for (int j = 0; j < otherVertexCount - 1; j++)
         {
         // are we on a part boundary for the other polygon? then skip
         if ((currentOtherPart < totalOtherParts - 1) && (otherPartArray[currentOtherPart + 1] - 1 == j))
            {
            ++currentOtherPart;
            continue;
            }

         const Vertex &otherVertex0 = pToPoly->GetVertex(j);
         const Vertex &otherVertex1 = pToPoly->GetVertex(j + 1);

         // compute slopes, intersect for the two line segments

         // is the "other" line vertical or horizontal?
         bool verticalOther = false, verticalThis = false, horizontalThis = false, horizontalOther = false;
         REAL mOther = -999.0;

         if (otherVertex0.x == otherVertex1.x)
            verticalOther = true;
         else if (otherVertex0.y == otherVertex1.y)
            {
            horizontalOther = true;
            mOther = 0.0f;
            }
         else  // other is not vertical OR horizontal, so compute slope
            mOther = (otherVertex0.y - otherVertex1.y) / (otherVertex0.x - otherVertex1.x);  // it isn't vertical, so calculate the slope of the stream segment

         // check "this" line - vertical of horizontal?
         REAL mThis = -999.0;
         if (thisVertex0.x == thisVertex1.x)   //vertical polygon segment??????????
            verticalThis = true;
         else if (thisVertex0.y == thisVertex1.y)
            {
            horizontalThis = true;
            mThis = 0.0f;
            }
         else
            mThis = (thisVertex0.y - thisVertex1.y) / (thisVertex0.x - thisVertex1.x); //calc slope of polygon segment

         // get intercepts for "this" and "other" vertex
         REAL bOther = -999.0;
         if (verticalOther == false)
            bOther = otherVertex0.y - mOther * otherVertex0.x;         // get intercept of stream line

         REAL bThis = -999.0;
         if (verticalThis == false)
            bThis = thisVertex0.y - mThis * thisVertex0.x;  // get intercept of polygon side

         ASSERT((verticalThis  && horizontalThis) == 0);  // can't be both horizontal and vertical
         ASSERT((verticalOther && horizontalOther) == 0);

         // compute the intersection point
         REAL yMinThis = -999.0, yMaxThis = -999.0, yMinOther = -999.0, yMaxOther = -999.0;

         if (thisVertex0.y < thisVertex1.y)
            {
            yMinThis = thisVertex0.y;
            yMaxThis = thisVertex1.y;
            }
         else
            {
            yMaxThis = thisVertex0.y;
            yMinThis = thisVertex1.y;
            }

         if (otherVertex0.y < otherVertex1.y)
            {
            yMinOther = otherVertex0.y;
            yMaxOther = otherVertex1.y;
            }
         else
            {
            yMaxOther = otherVertex0.y;
            yMinOther = otherVertex1.y;
            }

         REAL xMinThis = -999.0, xMaxThis = -999.0, xMinOther = -999.0, xMaxOther = -999.0;

         if (otherVertex0.x < otherVertex1.x)
            {
            xMinOther = otherVertex0.x;
            xMaxOther = otherVertex1.x;
            }
         else
            {
            xMinOther = otherVertex1.x;
            xMaxOther = otherVertex0.x;
            }

         if (thisVertex0.x < thisVertex1.x)
            {
            xMinThis = thisVertex0.x;
            xMaxThis = thisVertex1.x;
            }
         else
            {
            xMinThis = thisVertex1.x;
            xMaxThis = thisVertex0.x;
            }


         if (verticalThis && verticalOther)   // both vertical?
            {
            if (yMinThis <= yMaxOther && yMinOther <= yMaxThis) // case where they DON'T overlap
               {
               float _distance = (float)fabs(thisVertex0.x - otherVertex0.x);

               if (_distance < distance)
                  distance = _distance;
               }
            }

         else if (verticalThis)  // polygon side is vertical, line segment is not
            {
            // get the intersection coordinate
            REAL yIntersect = mOther*thisVertex0.x + bOther;

            if (horizontalOther)
               {
               if (xMinOther <= xMinThis && xMinThis <= xMaxOther && yMinThis <= yMinOther && yMinOther <= yMaxThis)
                  {
                  SET_FLAG(retFlag, 2);
                  return 0.0;
                  }
               }
            else
               {
               // see if the intersection is in the bounds of this both segments
               if (yMinOther <= yIntersect && yIntersect <= yMaxOther  && yMinThis <= yIntersect && yIntersect <= yMaxThis)
                  {
                  SET_FLAG(retFlag, 3);
                  return 0.0f;
                  }
               }
            }  // end of: else (this vertical, other note)

         else if (verticalOther)    // and NOT vertical this
            {
            // get the intersection coordinate
            REAL yIntersect = mThis*otherVertex0.x + bThis;

            if (horizontalThis)
               {
               if (xMinThis <= xMinOther && xMinOther <= xMaxThis  && yMinOther <= yMinThis && yMinThis <= yMaxOther)
                  {
                  SET_FLAG(retFlag, 4);
                  return 0.0f;
                  }
               }
            else
               {
               // see if the intersection is in the bounds both segments
               if (yMinThis <= yIntersect && yIntersect <= yMaxThis && yMinOther <= yIntersect && yIntersect <= yMaxOther)
                  {
                  SET_FLAG(retFlag, 5);
                  return 0.0f;
                  }
               }
            }

         else   // neither of the line segments are vertical, so proceed...
            {

            // same slopes?  need to check to see if the points are colinear (same y intercepts)
            if (mThis == mOther)
               {
               if (bThis == bOther)  // colinear
                  {
                  if (horizontalThis)
                     {
                     if (xMinThis <= xMaxOther && xMinOther <= xMaxThis)
                        {
                        SET_FLAG(retFlag, 6);
                        return 0;
                        }
                     }
                  else
                     {
                     if (yMinThis <= yMaxOther && yMinOther <= yMaxThis)  // case where they DO overlap
                        {
                        SET_FLAG(retFlag, 7);
                        return 0.0;
                        }
                     }
                  }
               }
            else  // different slopes...do the lines intersect in the bounds of the segments?
               {
               REAL xIntersect = (bOther - bThis) / (mThis - mOther);

               if (xIntersect >= xMinThis && xIntersect >= xMinOther && xIntersect <= xMaxThis && xIntersect <= xMaxOther)
                  {
                  SET_FLAG(retFlag, 8);
                  return 0.0;
                  }
               }
            }  // end of:  else (this not vertical and other not vertical)
         }  // end of:  for ( j < otherVertexCount-1 )
      }  // end of:  for ( i < thisVertexCount-1 )

   // if we get this far, the segments do not cross - move on to step two
   //
   //  2) see if any of "this" polygons' line segments have perpendiculars that intersect any
   //     of the passed in polygons vertices

   //  First calculate distances from vertices in the linear coverage to lines in the polygon coverage
   bool foundPtSegment = false;

   currentThisPart = 0;
   for (int i = 0; i < thisVertexCount - 1; i++)
      {
      // are we on a part boundary for this polygon? then skip
      if ((currentThisPart < totalThisParts - 1) && (m_vertexPartArray[currentThisPart + 1] - 1 == i))
         {
         ++currentThisPart;
         continue;
         }

      // not on a part boundary, so keep going
      Vertex &thisVertexStart = ((Poly*)this)->GetVertex(i);
      Vertex &thisVertexEnd = ((Poly*)this)->GetVertex(i + 1);

      for (int j = 0; j < otherVertexCount; j++)
         {
         const Vertex &otherVertex = pToPoly->GetVertex(j);
         float _distance;

         // get a distances to the "other" vertex (and see if it's perpendicular to this segment)
         bool onSegment = DistancePtToLineSegment(
            otherVertex.x, otherVertex.y,
            thisVertexStart.x, thisVertexStart.y, thisVertexEnd.x, thisVertexEnd.y,
            _distance);

         if (onSegment)
            {
            if (_distance < 0.0f)
               _distance = -_distance;

            if (foundPtSegment == false)
               {
               distance = _distance;
               foundPtSegment = true;
               }
            else if (_distance < distance)
               distance = _distance;
            }
         }  // end of:  for ( j < otherVertexCount )
      }  // end of:  for ( i < thisVertexCount-1 )

   // repeat for segments on other poly and vertices on this one
   currentOtherPart = 0;
   for (int i = 0; i < otherVertexCount - 1; i++)
      {
      // are we on a part boundary for the other polygon? then skip
      if ((currentOtherPart < totalOtherParts - 1) && (otherPartArray[currentOtherPart + 1] - 1 == i))
         {
         ++currentOtherPart;
         continue;
         }

      // not on a part boundary, so continue
      const Vertex &otherVertexStart = pToPoly->GetVertex(i);
      const Vertex &otherVertexEnd = pToPoly->GetVertex(i + 1);

      for (int j = 0; j < thisVertexCount; j++)
         {
         Vertex &thisVertex = ((Poly*)this)->GetVertex(j);
         float _distance;

         bool onSegment = DistancePtToLineSegment(
            thisVertex.x, thisVertex.y,
            otherVertexStart.x, otherVertexStart.y, otherVertexEnd.x, otherVertexEnd.y,
            _distance);

         if (onSegment)
            {
            if (_distance < 0.0f)
               _distance = -_distance;

            if (foundPtSegment == false)
               {
               distance = _distance;
               foundPtSegment = true;
               }
            else if (_distance < distance)
               distance = _distance;
            }
         }  // end of:  for ( j < thisVertexCount )
      }  // end of:  for ( i < otherVertexCount-1 )

   // did we find a perpendicular segment?  if so, we're done...
   if (foundPtSegment == true)
      {
      SET_FLAG(retFlag, 9);
      //return distance;
      }

   // Step 3. no perpendicular segments found, so check all the vertex-to-vertex distance's
   for (int i = 0; i < thisVertexCount; i++)
      {
      for (int j = 0; j < otherVertexCount; j++)
         {
         const Vertex &otherVertex = pToPoly->GetVertex(j);

         // calculate the distance between these two vertices
         float _distance = DistancePtToPt(otherVertex.x, otherVertex.y,
            m_vertexArray[i].x, m_vertexArray[i].y);

         if (_distance < 0.0)
            _distance = -_distance;

         if (_distance < distance)
            {
            distance = _distance;
            SET_FLAG(retFlag, 10);
            }
         }
      }

   //SET_FLAG( retFlag, 10 );
   return distance;
   }

REAL Poly::NearestDistanceToVertex(Vertex *pToVertex) const
   {
   int parts = m_vertexPartArray.GetSize();

   ASSERT(parts > 0); //must have atleast one part

   int startPart = m_vertexPartArray[0];
   int vertexCount = 0;
   int nextPart = 0;

   REAL dist = FLT_MAX;
   REAL _dist;
   REAL L, h;

   for (int part = 0; part < parts; part++)
      {
      if (part < parts - 1)
         {
         nextPart = m_vertexPartArray[part + 1];
         vertexCount = nextPart - startPart;
         }
      else
         vertexCount = GetVertexCount() - startPart;

      ASSERT(vertexCount >= 4); // a ring is composed of 4 or more points

      for (int i = startPart; i < startPart + vertexCount - 1; i++)
         {
         const Vertex &vertex0 = m_vertexArray.GetAt(i);
         const Vertex &vertex1 = m_vertexArray.GetAt(i + 1);

         // Distance from endpoint of segment to vertex ( the other endpoint will be tested in the next loop )
         _dist = sqrt((vertex0.x - pToVertex->x)*(vertex0.x - pToVertex->x) + (vertex0.y - pToVertex->y)*(vertex0.y - pToVertex->y));
         if (_dist < dist)
            dist = _dist;

         // Distance from interior of segment to vertex
         if (vertex0.x != vertex1.x || vertex0.y != vertex1.y) // check for degenerate segment
            {
            L = sqrt((vertex1.x - vertex0.x)*(vertex1.x - vertex0.x) + (vertex1.y - vertex0.y)*(vertex1.y - vertex0.y));
            h = ((pToVertex->x - vertex0.x)*(vertex1.x - vertex0.x) + (pToVertex->y - vertex0.y)*(vertex1.y - vertex0.y)) / L;
            if (0 < h && h < L) // if not, one of the endpoints is closest so dont bother with this calculation
               {
               _dist = fabs(((pToVertex->y - vertex0.y)*(vertex1.x - vertex0.x) - (pToVertex->x - vertex0.x)*(vertex1.y - vertex0.y)) / L);
               if (_dist < dist)
                  dist = _dist;
               }
            }
         }

      startPart = nextPart;
      }

   if (IsPointInPoly(*pToVertex))
      return -dist;
   else
      return dist;
   }

REAL Poly::CentroidDistanceToPoly(Poly *pToPoly) const
   {
   Vertex vThis = GetCentroid();
   Vertex vOther = pToPoly->GetCentroid();

   return (REAL)sqrt((vThis.x - vOther.x)*(vThis.x - vOther.x) + (vThis.y - vOther.y)*(vThis.y - vOther.y));
   }

float Poly::GetAreaInCircle(Vertex center, float radius) const
   {
   static const double pi = 3.1415926535897932384626433832795;
   double radius2 = double(radius)*double(radius);

   // ---- check bounding rect ---- //
   // The following code is based on:
   //   Fast Circle-Rectangle Intersection Checking
   //   by Clifford A. Shaffer
   //   from "Graphics Gems", Academic Press, 1990

   REAL xMin, yMin, xMax, yMax;
   GetBoundingRect(xMin, yMin, xMax, yMax);

   // Translate coordinates, placing center of circle at the origin.
   xMax -= center.x;  yMax -= center.y;
   xMin -= center.x;  yMin -= center.y;

   bool tl = ((xMin * xMin + yMax * yMax) < radius2);  // top left corner in circle?
   bool tr = ((xMax * xMax + yMax * yMax) < radius2);  // top right corner in circle?
   bool br = ((xMax * xMax + yMin * yMin) < radius2);  // bottom right corner in circle?
   bool bl = ((xMin * xMin + yMin * yMin) < radius2);  // bottom left corner in circle?

   // Entire BoundingRect in circle?
   if (tl && tr && br && bl)
      return GetArea();

   // Atleast one corner is outside the circle.
   bool intersect;
   if (xMax < 0)           // BoundingRect to left of circle center
      if (yMax < 0)        // BoundingRect in lower left corner 
         intersect = tr;
      else if (yMin > 0)   // BoundingRect in upper left corner 
         intersect = br;
      else                   // BoundingRect due West of circle 
         intersect = (-xMax < radius);
   else if (xMin > 0)      // BoundingRect to right of circle center
      if (yMax < 0)        // BoundingRect in lower right corner
         intersect = tl;
      else if (yMin > 0)   // BoundingRect in upper right corner
         intersect = bl;
      else                   // BoundingRect due East of circle
         intersect = (xMin < radius);
   else                      // BoundingRect on circle vertical centerline
      if (yMax < 0)        // BoundingRect due South of circle
         intersect = (-yMax < radius);
      else if (yMin > 0)   // BoundingRect due North of circle
         intersect = (yMin < radius);
      else                   // BoundingRect contains circle centerpoint
         intersect = true;

   if (!intersect)
      return 0.0f;

   // ---- Get Area In Circle ---- //
   int parts = m_vertexPartArray.GetSize();
   ASSERT(parts > 0); //must have atleast one part

   int startPart = m_vertexPartArray[0];
   int nextPart = 0;

   Vertex v1;
   Vertex v2;

   bool   v1isIn;
   bool   v2isIn;
   double areaInCircle = 0.0;
   double partArea = 0.0;
   double partAreaInCircle = 0.0;
   double baseTheta = 0.0;
   double theta;
   double totalTheta;

   bool   vertexIn;
   bool   centerInPoly;
   bool   baseThetaSet;

   double x21;
   double y21;
   double t;
   Vertex ov1;
   Vertex ov2;
   int sign;

   for (int part = 0; part < parts; part++)
      {
      if (part < parts - 1)
         nextPart = m_vertexPartArray[part + 1];
      else
         nextPart = GetVertexCount();

      ASSERT(nextPart - startPart >= 4); // a ring is composed of 4 or more points

      // reset variables
      partArea = 0.0;
      partAreaInCircle = 0.0;
      totalTheta = 0.0;
      baseTheta = 0.0;
      vertexIn = false;
      centerInPoly = false;
      baseThetaSet = false;

      // get initial vertex
      v1 = m_vertexArray[startPart];  // copy constructor called

      // translate the copied vertex
      v1.x -= center.x;
      v1.y -= center.y;

      v1isIn = (v1.x*v1.x + v1.y*v1.y < radius2);

      // walk the boundry of this part, 
      //   integrating when inside the circle 
      //   and keeping a running total of the arc angle
      for (int j = startPart + 1; j < nextPart; j++)
         {
         v2 = m_vertexArray[j];  // copy constructor called

         // translate the copied vertex
         v2.x -= center.x;
         v2.y -= center.y;

         v2isIn = (v2.x*v2.x + v2.y*v2.y < radius2);

         partArea += v2.x * v1.y - v1.x * v2.y;

         _IsPointInPoly(&v1, &v2, Vertex(0, 0), centerInPoly);

         if (v1isIn)
            {
            vertexIn = true;
            if (v2isIn)
               {
               partAreaInCircle += v2.x * v1.y - v1.x * v2.y;
               }
            else
               {
               x21 = v2.x - v1.x;
               y21 = v2.y - v1.y;

               // in this case, there are two solutions, t1 < 0 < t2 < 1, want t2
               t = (-(v1.x*x21 + v1.y*y21) + sqrt((v1.x*x21 + v1.y*y21)*(v1.x*x21 + v1.y*y21) - (x21*x21 + y21*y21)*(v1.x*v1.x + v1.y*v1.y - radius2))) / (x21*x21 + y21*y21);
               if (t < 0) t = 0.0;
               if (t > 1.0) t = 1;
               ov2.x = v1.x + t*x21;
               ov2.y = v1.y + t*y21;
               if (ov2.y >= 0.0)
                  theta = acos(ov2.x / radius) - baseTheta;
               else
                  theta = 2 * pi - acos(ov2.x / radius) - baseTheta;

               if (!baseThetaSet)
                  {
                  baseTheta = theta;
                  totalTheta = 2 * pi;  // because we are leaving the circle
                  baseThetaSet = true;
                  }
               else
                  {
                  if (theta < 0.0)
                     theta = 2 * pi + theta;

                  totalTheta += theta; // += because we are leaving the circle
                  }

               partAreaInCircle += ov2.x * v1.y - v1.x * ov2.y;
               }
            }
         else // v1 is out
            {
            if (v2isIn)
               {
               vertexIn = true;

               x21 = v2.x - v1.x;
               y21 = v2.y - v1.y;

               // in this case, there are two solutions, 0 < t1 < 1 < t2, want t1
               t = (-(v1.x*x21 + v1.y*y21) - sqrt((v1.x*x21 + v1.y*y21)*(v1.x*x21 + v1.y*y21) - (x21*x21 + y21*y21)*(v1.x*v1.x + v1.y*v1.y - radius2))) / (x21*x21 + y21*y21);
               ASSERT(0.0 <= t && t < 1.0);
               ov1.x = v1.x + t*x21;
               ov1.y = v1.y + t*y21;
               if (ov1.y >= 0.0)
                  theta = acos(ov1.x / radius) - baseTheta;
               else
                  theta = 2 * pi - acos(ov1.x / radius) - baseTheta;

               if (!baseThetaSet)
                  {
                  baseTheta = theta;
                  ASSERT(totalTheta == 0.0);  // because we are entering the circle
                  baseThetaSet = true;
                  }
               else
                  {
                  if (theta < 0)
                     theta = 2 * pi + theta;
                  if (theta < FLT_EPSILON) // call this zero
                     totalTheta -= 2 * pi;  // because we are leaving the circle
                  else
                     totalTheta -= theta; // -= because we are entering the circle
                  }

               partAreaInCircle += v2.x * ov1.y - ov1.x * v2.y;
               }
            else
               {
               x21 = v2.x - v1.x;
               y21 = v2.y - v1.y;

               // test for intersection
               t = -(v1.y*y21 + v1.x*x21) / (x21*x21 + y21*y21);
               if (0.0 < t && t < 1.0)
                  {
                  ov1.x = v1.x + t*x21;  // point on line segment closest to center
                  ov1.y = v1.y + t*y21;

                  if (ov1.x*ov1.x + ov1.y*ov1.y < radius2)
                     {
                     vertexIn = true;

                     // in this case, there are two solutions, 0 < t1 < t2 < 1, want both
                     t = (-(v1.x*x21 + v1.y*y21) - sqrt((v1.x*x21 + v1.y*y21)*(v1.x*x21 + v1.y*y21) - (x21*x21 + y21*y21)*(v1.x*v1.x + v1.y*v1.y - radius2))) / (x21*x21 + y21*y21);
                     if (t < 0) t = 0;
                     if (t > 1) t = 1;
                     //ASSERT( 0.0 <= t && t <= 1.0 );
                     ov1.x = v1.x + t*x21;
                     ov1.y = v1.y + t*y21;

                     t = (-(v1.x*x21 + v1.y*y21) + sqrt((v1.x*x21 + v1.y*y21)*(v1.x*x21 + v1.y*y21) - (x21*x21 + y21*y21)*(v1.x*v1.x + v1.y*v1.y - radius2))) / (x21*x21 + y21*y21);
                     //ASSERT( 0.0 <= t && t <= 1.0 );
                     if (t < 0) t = 0;
                     if (t > 1) t = 1;

                     ov2.x = v1.x + t*x21;
                     ov2.y = v1.y + t*y21;

                     if (ov1.y >= 0.0)
                        theta = acos(ov1.x / radius) - baseTheta;
                     else
                        theta = 2 * pi - acos(ov1.x / radius) - baseTheta;

                     if (!baseThetaSet)
                        {
                        baseTheta = theta;
                        ASSERT(totalTheta == 0.0);  // because we are entering the circle
                        baseThetaSet = true;
                        }
                     else
                        {
                        if (theta < 0.0)
                           theta = 2 * pi + theta;
                        totalTheta -= theta; // -= because we are entering the circle
                        }

                     if (ov2.y >= 0.0)
                        theta = acos(ov2.x / radius) - baseTheta;
                     else
                        theta = 2 * pi - acos(ov2.x / radius) - baseTheta;

                     if (theta < 0)
                        theta = 2 * pi + theta;
                     if (theta < FLT_EPSILON) // call this zero
                        totalTheta += 2 * pi;  // because we are leaving the circle
                     else
                        totalTheta += theta; // += because we are leaving the circle

                     partAreaInCircle += ov2.x * ov1.y - ov1.x * ov2.y;
                     }
                  }
               }
            }

         v1 = v2;
         v1isIn = v2isIn;
         }

      sign = partArea > 0 ? 1 : -1;

      if (vertexIn) // if atleast one vertex is in the circle
         {
         if (baseThetaSet) // and atleast one is outside
            {
            // now integrate on the arcs between the intersection points
            if (totalTheta < 0) totalTheta = 0;
            if (totalTheta > 2 * pi) totalTheta = 2 * pi;
            if (sign > 0)
               partAreaInCircle += radius2*totalTheta;
            else
               partAreaInCircle -= radius2*(2 * pi - totalTheta);
            }
         }
      else // check for case that circle is inside polygon
         {
         if (centerInPoly)  // center of circle is in the poly, but the boundry does not enter the circle
            partAreaInCircle = 2.0*sign*pi*radius2;  // twice the area
         }

      //ASSERT( sign*partAreaInCircle <= 1.01*sign*partArea );  
      if (sign*partAreaInCircle > sign*partArea)
         {
         //TRACE( "GetAreaInCircle(.): partAreaInCircle = %g, partArea = %g\n", sign*partAreaInCircle, sign*partArea );
         partAreaInCircle = partArea;
         }

      areaInCircle += partAreaInCircle / 2.0;

      startPart = nextPart;
      }

   //ASSERT( areaInCircle >= -0.01*sign*partArea  );
   if (areaInCircle < 0.0)
      {
      //TRACE( "GetAreaInCircle(.): areaInCircle = %g\n", areaInCircle );
      partAreaInCircle = partArea;
      areaInCircle = 0.0;
      }

   return (float)areaInCircle;
   }


PolyArray::PolyArray(const PolyArray &polyArray)
   {
   // Shallow Copy
   Copy(polyArray);

   // clone all the polygons
   for (INT_PTR i = 0; i < polyArray.GetSize(); i++)
      {
      this->GetAt(i) = new Poly(*(polyArray.GetAt(i)));  // no bins, everything else ok
      ASSERT(this->GetAt(i) != NULL);
      }
   }


MapLayer::MapLayer(Map *pMap)
   : m_pMap(pMap),
   m_totalArea(-1.0f),
   m_pQueryEngine(NULL),
   m_pSpatialIndex(NULL),
   m_pAttrIndex(NULL),
   m_layerType(LT_POLYGON),
   m_pPolyArray(NULL),
   //  m_pBinArray( NULL ),
   //  m_attribInfoArray(),
   m_pFieldInfoArray(NULL),
   m_pDbTable(NULL),
   m_pData(NULL),
   m_mapUnits(MU_UNKNOWN),
   m_activeField(-1),
   m_showLegend(false),
   m_expandLegend(false),
   m_showBinCount(false),
   m_showSingleValue(false),
   m_binColorFlag(BCF_MIXED),
   m_isVisible(true),
   m_vxMin((REAL)LONG_MAX),
   m_vxMax((REAL)LONG_MIN),
   m_vyMin((REAL)LONG_MAX),
   m_vyMax((REAL)LONG_MIN),
   m_vzMin((REAL)LONG_MAX),
   m_vzMax((REAL)LONG_MIN),
   m_vExtent(0),
   m_pOverlaySource(NULL),
   m_overlayFlags(OT_NOT_SHARED),
   m_overlayFields(),
   m_includeData(true),
   m_3D(false),
   m_recordsLoaded(-1),
   m_readOnly(false),
   m_noDataValue(-99.0f),
   m_useVarWidth(false),  // for line coverages, specifies variable width line segments based on classified value
   m_minVarWidth(0),
   m_maxVarWidth(20),     // upper scalar for var Width lines, ignored if m_useVarWidth = false;
   m_lineWidth(0),
   m_outlineColor(RGB(127, 127, 127)),
   m_hatchColor(0),
   m_hatchStyle(-1),      // no hatch
   m_transparency(0),     // opaque
   m_showLabels(false),
   m_labelColor(0),
   m_labelCol(-1),
   m_labelSize(120),
   m_labelQueryStr(),
   m_pLabelQuery(NULL),
   m_labelMethod(SLP_CENTROID),
   m_useNoDataBin(true),
   m_changedFlag(CF_NONE),
   m_ascending(false),
   m_useDisplayThreshold(0),
   m_displayThresholdValue(0.0f),
   m_deadCount(0),
   m_projection()
   {
   lstrcpy(m_legendFormat, "%g");

#ifdef _WINDOWS
   memset(&m_labelFont, 0, sizeof(LOGFONT));
   lstrcpy(m_labelFont.lfFaceName, _T("Arial"));
#endif  //_WINDOWS

   m_pPolyArray = new PolyArray;
   m_pFieldInfoArray = new FieldInfoArray;
   }


MapLayer::MapLayer(MapLayer &layer)
   : m_pMap(layer.m_pMap),
   m_totalArea(layer.m_totalArea),
   m_pQueryEngine(NULL),
   m_pSpatialIndex(NULL),
   m_pAttrIndex(NULL),
   m_name(layer.m_name),
   m_path(layer.m_path),
   m_database(layer.m_database),
   m_tableName(layer.m_tableName),
   m_layerType(layer.m_layerType),
   m_pPolyArray(layer.m_pPolyArray),
   //   m_pBinArray ( NULL ),  // set in the body
   m_binArrayArray(layer.m_binArrayArray),
   m_pFieldInfoArray(NULL),
   m_pDbTable(NULL),
   m_pData(NULL),
   m_mapUnits(layer.m_mapUnits),
   m_activeField(layer.m_activeField),
   m_isVisible(layer.m_isVisible),
   m_vxMin(layer.m_vxMin),
   m_vxMax(layer.m_vxMax),
   m_vyMin(layer.m_vyMin),
   m_vyMax(layer.m_vyMax),
   m_vzMin(layer.m_vzMin),
   m_vzMax(layer.m_vzMax),
   m_vExtent(layer.m_vExtent),
   m_includeData(layer.m_includeData),
   m_3D(layer.m_3D),
   m_pOverlaySource(layer.m_pOverlaySource),
   m_overlayFlags(layer.m_overlayFlags),
   m_recordsLoaded(layer.m_recordsLoaded),
   m_readOnly(layer.m_readOnly),
   m_cellWidth(layer.m_cellWidth),
   m_cellHeight(layer.m_cellHeight),
   m_noDataValue(layer.m_noDataValue),
   m_showLegend(layer.m_showLegend),
   m_expandLegend(layer.m_expandLegend),
   m_showBinCount(layer.m_showBinCount),
   m_showSingleValue(layer.m_showSingleValue),
   m_ascending(layer.m_ascending),
   m_binColorFlag(layer.m_binColorFlag),
   m_useVarWidth(layer.m_useVarWidth),
   m_minVarWidth(layer.m_minVarWidth),
   m_maxVarWidth(layer.m_maxVarWidth),
   m_lineWidth(layer.m_lineWidth),
   m_outlineColor(layer.m_outlineColor),
   m_hatchColor(layer.m_hatchColor),
   m_hatchStyle(layer.m_hatchStyle),
   m_transparency(layer.m_transparency),
   m_showLabels(layer.m_showLabels),
   m_labelColor(layer.m_labelColor),
   m_labelCol(layer.m_labelCol),
   m_labelSize(layer.m_labelSize),
   m_labelQueryStr(layer.m_labelQueryStr),
   m_pLabelQuery(layer.m_pLabelQuery),
   m_labelMethod(layer.m_labelMethod),
   m_useNoDataBin(layer.m_useNoDataBin),
   m_changedFlag(CF_NONE),
   m_useDisplayThreshold(layer.m_useDisplayThreshold),
   m_displayThresholdValue(layer.m_displayThresholdValue),
   m_deadCount(layer.m_deadCount),
   m_projection(layer.m_projection)
   {
   lstrcpy(m_legendFormat, layer.m_legendFormat);

   if (layer.m_pDbTable != NULL && !IsOverlay())
      {
      m_pDbTable = new DbTable(*layer.m_pDbTable);
      m_pData = m_pDbTable->m_pData;
      }
   else
      {
      m_pDbTable = NULL;
      m_pData = NULL;
      }

   if (layer.m_pPolyArray != NULL && !IsOverlay())
      m_pPolyArray = new PolyArray(*layer.m_pPolyArray);

#ifdef _WINDOWS
   memcpy(&m_labelFont, &layer.m_labelFont, sizeof(LOGFONT));
#endif  //_WINDOWS

   if (layer.m_pFieldInfoArray != NULL)
      m_pFieldInfoArray = new FieldInfoArray(*(layer.m_pFieldInfoArray));

   m_polyBinArray.SetSize(GetPolygonCount());
   InitPolyBins();

   //if ( m_activeField > 0 )
   //   m_pBinArray = m_binArrayArray.GetAt( m_activeField );

   }

// create a shared layer (overlay type)
MapLayer::MapLayer(MapLayer *pLayer, int overlayFlags)
   : m_pMap(pLayer->m_pMap),
   m_totalArea(pLayer->m_totalArea),
   m_pQueryEngine(NULL),
   m_pSpatialIndex(NULL),
   m_pAttrIndex(NULL),
   m_name(pLayer->m_name),
   m_path(pLayer->m_path),
   m_database(pLayer->m_database),
   m_tableName(pLayer->m_tableName),
   m_layerType(pLayer->m_layerType),
   m_pPolyArray(pLayer->m_pPolyArray),
   //   m_pBinArray ( NULL ),  // set in the body
      //m_polyBinArray( pLayer->m_polyBinArray ),
   m_binArrayArray(pLayer->m_binArrayArray),
   m_pDbTable(NULL),
   m_pData(NULL),
   m_mapUnits(pLayer->m_mapUnits),
   m_activeField(pLayer->m_activeField),
   m_isVisible(pLayer->m_isVisible),
   m_vxMin(pLayer->m_vxMin),
   m_vxMax(pLayer->m_vxMax),
   m_vyMin(pLayer->m_vyMin),
   m_vyMax(pLayer->m_vyMax),
   m_vzMin(pLayer->m_vzMin),
   m_vzMax(pLayer->m_vzMax),
   m_vExtent(pLayer->m_vExtent),
   m_includeData(pLayer->m_includeData),
   m_3D(pLayer->m_3D),
   m_pOverlaySource(pLayer->m_pOverlaySource),
   m_overlayFlags(overlayFlags),
   m_recordsLoaded(pLayer->m_recordsLoaded),
   m_readOnly(pLayer->m_readOnly),
   m_cellWidth(pLayer->m_cellWidth),
   m_cellHeight(pLayer->m_cellHeight),
   m_noDataValue(pLayer->m_noDataValue),
   m_showLegend(pLayer->m_showLegend),
   m_expandLegend(pLayer->m_expandLegend),
   m_showBinCount(pLayer->m_showBinCount),
   m_showSingleValue(pLayer->m_showSingleValue),
   m_ascending(pLayer->m_ascending),
   m_binColorFlag(pLayer->m_binColorFlag),
   m_useVarWidth(pLayer->m_useVarWidth),
   m_minVarWidth(pLayer->m_minVarWidth),
   m_maxVarWidth(pLayer->m_maxVarWidth),
   m_lineWidth(pLayer->m_lineWidth),
   m_outlineColor(pLayer->m_outlineColor),
   m_hatchColor(pLayer->m_hatchColor),
   m_hatchStyle(pLayer->m_hatchStyle),
   m_transparency(pLayer->m_transparency),
   m_showLabels(pLayer->m_showLabels),
   m_labelColor(pLayer->m_labelColor),
   m_labelCol(pLayer->m_labelCol),
   m_labelSize(pLayer->m_labelSize),
   m_labelQueryStr(pLayer->m_labelQueryStr),
   m_pLabelQuery(pLayer->m_pLabelQuery),
   m_labelMethod(pLayer->m_labelMethod),
   m_useNoDataBin(pLayer->m_useNoDataBin),
   m_changedFlag(CF_NONE),
   m_useDisplayThreshold(pLayer->m_useDisplayThreshold),
   m_displayThresholdValue(pLayer->m_displayThresholdValue),
   m_deadCount(pLayer->m_deadCount)
   {
   lstrcpy(m_legendFormat, pLayer->m_legendFormat);
#ifdef _WINDOWS
   memcpy(&m_labelFont, &pLayer->m_labelFont, sizeof(LOGFONT));
#endif  //_WINDOWS

   // is this an overlay layer?
   if (overlayFlags > 0)
      m_pOverlaySource = pLayer;

   if (overlayFlags & OT_SHARED_DATA)
      {
      m_pDbTable = pLayer->m_pDbTable;
      m_pData = pLayer->m_pData;
      }
   else
      {
      if (pLayer->m_pDbTable != NULL)
         {
         m_pDbTable = new DbTable(*(pLayer->m_pDbTable));
         m_pData = m_pDbTable->m_pData;
         }
      else
         {
         m_pDbTable = NULL;
         m_pData = NULL;
         }
      }

   if (overlayFlags & OT_SHARED_GEOMETRY)
      m_pPolyArray = pLayer->m_pPolyArray;
   else
      m_pPolyArray = new PolyArray(*(pLayer->m_pPolyArray));

   if (overlayFlags & OT_SHARED_SPINDEX)
      m_pSpatialIndex = pLayer->m_pSpatialIndex;
   else if (pLayer->m_pSpatialIndex != NULL)
      m_pSpatialIndex = new SpatialIndex(*(pLayer->m_pSpatialIndex));

   if (overlayFlags & OT_SHARED_ATTRINDEX)
      m_pAttrIndex = pLayer->m_pAttrIndex;
   else if (pLayer->m_pAttrIndex != NULL)
      m_pAttrIndex = new AttrIndex(*(pLayer->m_pAttrIndex));

   if (overlayFlags & OT_SHARED_FIELDINFO)
      m_pFieldInfoArray = pLayer->m_pFieldInfoArray;
   else if (pLayer->m_pFieldInfoArray != NULL)
      m_pFieldInfoArray->Copy(*(pLayer->m_pFieldInfoArray));

   //if (overlayFlags & OT_SHARED_QUERYENGINE)
      m_pQueryEngine = pLayer->m_pQueryEngine;

   m_polyBinArray.SetSize(GetPolygonCount());
   InitPolyBins();
   ClassifyData();
   }


MapLayer::~MapLayer()
   {
   if (m_pDbTable && !(m_overlayFlags & OT_SHARED_DATA))
      delete m_pDbTable;

   if (m_pPolyArray != NULL && !(m_overlayFlags & OT_SHARED_GEOMETRY))
      {
      for (int i = 0; i < m_pPolyArray->GetSize(); i++)
         delete m_pPolyArray->GetAt(i);

      delete m_pPolyArray;
      }

   if (m_pSpatialIndex != NULL && !(m_overlayFlags & OT_SHARED_SPINDEX))
      delete m_pSpatialIndex;

   if (m_pAttrIndex != NULL && !(m_overlayFlags & OT_SHARED_ATTRINDEX))
      delete m_pAttrIndex;

   //if (m_pQueryEngine != NULL && !(m_overlayFlags & OT_SHARED_QUERYENGINE))
   //   delete m_pQueryEngine;

   if (m_pFieldInfoArray != NULL && !(m_overlayFlags & OT_SHARED_FIELDINFO))
      delete m_pFieldInfoArray;
   }


bool MapLayer::GetState(MAP_STATE &ms) const
   {
   //ms.m_pBinArray    = NULL;//m_pBinArray;   // CArray of classification bins  FTODO: Impliment
   ms.m_activeField = m_activeField;   // column in data object
   ms.m_isVisible = m_isVisible;

   // legend formatting
   ms.m_showLegend = m_showLegend;
   ms.m_expandLegend = m_expandLegend;
   lstrcpy(ms.m_legendFormat, m_legendFormat);    // sprintf format string
   ms.m_showBinCount = m_showBinCount;          // display bin count info?
   ms.m_showSingleValue = m_showSingleValue;       // one item on each legend line?
   ms.m_ascending = m_ascending;             // sort order
   ms.m_useDisplayThreshold = m_useDisplayThreshold;   // neg = only display values below threshold, 0 =ignore threshold, pos = only display values above threshold
   ms.m_displayThresholdValue = m_displayThresholdValue; // value to use, see above for usage

   // display settings
   ms.m_binColorFlag = m_binColorFlag;         // how to auto-gen colors...
   ms.m_useVarWidth = m_useVarWidth;          // for line coverages, specifies variable width line segments based on classified value
   ms.m_minVarWidth = m_minVarWidth;          // lower scalar for var Width lines, ignored if ms.m_useVarWidth = false;
   ms.m_maxVarWidth = m_maxVarWidth;          // upper scalar for var Width lines, ignored if ms.m_useVarWidth = false;
   ms.m_lineWidth = m_lineWidth;            // line width for drwing lines, poly outlines

   ms.m_outlineColor = m_outlineColor;         // polygon outline for cell (NO_OUTLINE for no outline)
   ms.m_fillColor = m_fillColor;            // only used if BCFLAG=BCF_SINGLECOLOR
   ms.m_hatchColor = m_hatchColor;
   ms.m_hatchStyle = m_hatchStyle;           // -1 for no hatch, or, HS_BDIAGONAL, HS_CROSS, HS_DIAGCROSS, 
                                               //    HS_FDIAGONAL, HS_HORIZONTAL, HS_VERTICAL
   ms.m_transparency = m_transparency;

   ms.m_showLabels = m_showLabels;
   ms.m_labelColor = m_labelColor;           // color to draw labels (NO_LABELS for no label)
   ms.m_labelCol = m_labelCol;
#ifdef _WINDOWS
   memcpy(&ms.m_labelFont, &m_labelFont, sizeof(LOGFONT));
#endif  //_WINDOWS
   ms.m_labelSize = m_labelSize;
   ms.m_labelQueryStr = m_labelQueryStr;
   ms.m_pLabelQuery = m_pLabelQuery;
   ms.m_labelMethod = m_labelMethod;
   ms.m_useNoDataBin = m_useNoDataBin;

   return true;
   }


bool MapLayer::RestoreState(MAP_STATE &ms)
   {
   //m_binArray    = ms.m_binArray;        // CArray of classification bins  FTODO: Impliment this
   m_activeField = ms.m_activeField;   // column in data object
   m_isVisible = ms.m_isVisible;

   // legend formatting
   m_showLegend = ms.m_showLegend;
   m_expandLegend = ms.m_expandLegend;
   lstrcpy(m_legendFormat, ms.m_legendFormat);    // sprintf format string
   m_showBinCount = ms.m_showBinCount;          // display bin count info?
   m_showSingleValue = ms.m_showSingleValue;       // one item on each legend line?
   m_ascending = ms.m_ascending;             // sort order
   m_useDisplayThreshold = ms.m_useDisplayThreshold;   // neg = only display values below threshold, 0 =ignore threshold, pos = only display values above threshold
   m_displayThresholdValue = ms.m_displayThresholdValue; // value to use, see above for usage

   // display settings
   m_binColorFlag = ms.m_binColorFlag;         // how to auto-gen colors...
   m_useVarWidth = ms.m_useVarWidth;          // for line coverages, specifies variable width line segments based on classified value
   m_minVarWidth = ms.m_minVarWidth;          // lower scalar for var Width lines, ignored if ms.m_useVarWidth = false;
   m_maxVarWidth = ms.m_maxVarWidth;          // upper scalar for var Width lines, ignored if ms.m_useVarWidth = false;
   m_lineWidth = ms.m_lineWidth;
   m_outlineColor = ms.m_outlineColor;         // polygon outline for cell (NO_OUTLINE for no outline)
   m_fillColor = ms.m_fillColor;            // only used if BCFLAG=BCF_SINGLECOLOR
   m_hatchColor = ms.m_hatchColor;
   m_hatchStyle = ms.m_hatchStyle;           // -1 for no hatch, or, HS_BDIAGONAL, HS_CROSS, HS_DIAGCROSS, 
                                               //    HS_FDIAGONAL, HS_HORIZONTAL, HS_VERTICAL
   m_transparency = ms.m_transparency;
   m_showLabels = ms.m_showLabels;
   m_labelColor = ms.m_labelColor;           // color to draw labels (NO_LABELS for no label)
   m_labelCol = ms.m_labelCol;
   m_labelSize = ms.m_labelSize;
#ifdef _WINDOWS
   memcpy(&m_labelFont, &ms.m_labelFont, sizeof(LOGFONT));
#endif  //_WINDOWS
   m_labelQueryStr = ms.m_labelQueryStr;
   m_pLabelQuery = ms.m_pLabelQuery;

   m_labelMethod = ms.m_labelMethod;
   m_useNoDataBin = ms.m_useNoDataBin;

   return true;
   }

#ifdef _WINDOWS
void MapLayer::CreateLabelFont(CFont &font)
   {
   int size = m_pMap->VDtoLD(m_labelSize);
   size *= 10;

   /////
   if (size > 2000)
      size = 2000;
   ///

#ifdef _WINDOWS
   m_labelFont.lfHeight = size;
#endif  //_WINDOWS

   font.CreateFontIndirect(&m_labelFont);
   }
#endif


int MapLayer::GetRecordCount(ML_RECORD_SET flag /*= ACTIVE*/) const
   {
   switch (m_layerType)
      {
      case LT_LINE:
      case LT_POINT:
      case LT_GRID:
         ASSERT(m_deadCount == 0);
      case LT_POLYGON:
         if (m_pData == NULL)
            return -1;
         else
            {
            if (flag == ACTIVE)
               return m_pData->GetRowCount() - m_deadCount;
            else if (flag == DEFUNCT)
               return m_deadCount;
            else
               return m_pData->GetRowCount();
            }

      default:
         ASSERT(0);   // invalid type
         break;
      }

   return -2;
   }

int MapLayer::GetPolygonCount(ML_RECORD_SET flag /*= ACTIVE*/) const
   {
   if (flag == ACTIVE)
      return m_pPolyArray->GetSize() - m_deadCount;
   else if (flag == DEFUNCT)
      return m_deadCount;
   else
      return m_pPolyArray->GetSize();
   }

int MapLayer::GetFieldCount(void) const
   {
   switch (m_layerType)
      {
      case LT_POLYGON:
      case LT_LINE:
      case LT_POINT:
      case LT_GRID:
         if (m_pData == NULL)
            return -1;
         else
            return m_pData->GetColCount();

      default:
         ASSERT(0);   // invalid type
         break;
      }

   return -2;
   }


int MapLayer::LoadVectorsAscii(LPCTSTR filename)
   {
   ClearPolygons();   // clear any existing polygons
   ClearData();   // add associated data

   FILE *fp;
   fopen_s(&fp, filename, "r");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -1;
      }

   // start loading
   const int MAXLINEWIDTH = 128;    // max line width ( for now )
   char *buffer = new char[MAXLINEWIDTH];

   Vertex v;
   int count = 0;

   // set extents to bounding values...
   m_vxMin = m_vyMin = (REAL)LONG_MAX;
   m_vxMax = m_vyMax = (REAL)LONG_MIN;

   while (!feof(fp))
      {
      //-- start getting data --//
      fgets(buffer, MAXLINEWIDTH, fp);

      if (*buffer == 'E')      // end of file found (two "ENDS")
         break;

      // first line of a poly def has an id, first vertex points
      Poly *pPoly = new Poly;
      int id;
      double x,y;
      if (sscanf_s(buffer, "%i %lf %lf", &id, &x, &y ) != 3 ) //&v.x, &v.y) != 3)
         {
         delete pPoly;
         delete buffer;
         return -2;
         }

      v.x = (REAL) x;
      v.y = (REAL) y;

      pPoly->m_id = id;
      //pPoly->Add( v );  // center point, don't add...

      fgets(buffer, MAXLINEWIDTH, fp);
      while (*buffer != 'E')    // END found?
         {
         double x, y;
         if (sscanf_s(buffer, "%lf %lf", &x, &y ) != 2 ) //&v.x, &v.y) != 2)
            {
            delete pPoly;
            delete buffer;
            return -3;
            }

         v.x = (REAL) x;
         v.y = (REAL) y;

         //-- add a vertex to the polygon --//
         pPoly->AddVertex(v, false);

         if (v.x > m_vxMax) m_vxMax = v.x;     // check bounds
         if (v.y > m_vyMax) m_vyMax = v.y;
         if (v.x < m_vxMin) m_vxMin = v.x;
         if (v.y < m_vyMin) m_vyMin = v.y;

         fgets(buffer, MAXLINEWIDTH, fp);     // get next line         
         }  // end of: while buffer != 'E' (adding additional vertexes)

      m_pPolyArray->Add(pPoly);  // add the polygon to the Map's collection
      count++;

      m_pMap->Notify(NT_LOADVECTOR, count, 0);
      }  // end of file reached, continue


   if ((m_vxMax - m_vxMin) > (m_vyMax - m_vyMin))
      m_vExtent = m_vxMax - m_vxMin;
   else
      m_vExtent = m_vyMax - m_vyMin;

   InitPolyLogicalPoints();   // create logical points for this set

   int polys = m_pPolyArray->GetSize();
   m_polyBinArray.SetSize(polys);
   InitPolyBins();

   //-- clean up --// 
   delete buffer;
   m_pMap->Notify(NT_LOADVECTOR, -count, 0);

   return count;
   }


int MapLayer::LoadShape(LPCTSTR filename, bool loadDB, int extraCols /* =0 */, int records /*=-1*/)
   {
   ClearPolygons();   // clear any existing polygons
   ClearData();   // add associated data

   FILE *fp;
   fopen_s(&fp, filename, "rb");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -1;
      }

   m_recordsLoaded = records;

   // start loading
   int count = 0;
   int voidCount = 0;
   bool deleteVoidPolys = false;

   fseek(fp, 24, SEEK_SET);
   /*DWORD fileLength =*/ GetBigDWORD(fp);
   /*DWORD version    =*/ GetLittleDWORD(fp);
   DWORD fileType = GetLittleDWORD(fp);

   m_3D = false;
   bool measured = false;

   switch (fileType)
      {
      case 15: // 3d
         m_3D = true;
      case 5:  // 2d
         m_layerType = LT_POLYGON;
         break;

      case 13: // 3d
         m_3D = true;
      case 3:  // 2d
         m_layerType = LT_LINE;
         break;

      case 23: // MPolyline
         m_layerType = LT_LINE;
         measured = true;
         break;

      case 11: // 3d
         m_3D = true;
      case 1:
         m_layerType = LT_POINT;
         break;

      default:
         Report::ErrorMsg("Can only read polygon, line and point shape files");
         fclose(fp);
         return -2;
      }

   if (m_3D)
      {
      m_vzMin = 1.0e31f;    // set unreasonable bounds, to be overwritten below...
      m_vzMax = -1.0e31f;
      }

   // get the bounding box
   m_vxMin = (REAL)GetLittleDouble(fp);
   m_vyMin = (REAL)GetLittleDouble(fp);
   m_vxMax = (REAL)GetLittleDouble(fp);
   m_vyMax = (REAL)GetLittleDouble(fp);

   REAL vxMax = (REAL)-LONG_MAX;
   REAL vxMin = (REAL)LONG_MAX;
   REAL vyMax = (REAL)-LONG_MAX;
   REAL vyMin = (REAL)LONG_MAX;

   CHECK_VALID_NUMBER(m_vxMin);
   CHECK_VALID_NUMBER(m_vyMin);
   CHECK_VALID_NUMBER(m_vxMax);
   CHECK_VALID_NUMBER(m_vyMax);

   // done with header, get records...
   fseek(fp, 100, SEEK_SET);

   UINT lastRecNo = 0;

   while (feof(fp) == 0)
      {
      DWORD recordNumber = GetBigDWORD(fp);

      if (recordNumber != ++lastRecNo)
         {
         CString msg;
         msg.Format("Bad record number found in shape file at record # %i... Do you want to continue?", lastRecNo);
         //if ( AfxMessageBox( msg, MB_YESNO) == IDNO )
         {
         records = count;
         break;
         }
         }

      DWORD contentLength = GetBigDWORD(fp);
      DWORD shapeType = GetLittleDWORD(fp);

      ASSERT((shapeType == fileType) || (shapeType == 0));

      if (shapeType != 5 && shapeType != 3 && shapeType != 15 && shapeType != 13 && shapeType != 1 && shapeType != 11 && shapeType != 23)
         {
#ifndef NO_MFC
         if (shapeType != 0)
            {
            CString msg;
            msg.Format("Invalid or Unsupported shape type (%i) found at record %d.  Press <Yes> to replace with an empty shape, or <No> to skip this shape. \n\n Replace with and empty shape?", shapeType, count);
            //Report::WarningMsg( msg );
            int retVal = AfxMessageBox(msg, MB_YESNO);

            if (retVal == IDNO)
               continue;   // to next record
            }
#endif
         // NULL shape encoutered, insert empty polygon and continue
         contentLength -= 2;
         for (int skip = 0; skip < (int)contentLength; skip++)
            GetLittleWord(fp);

         Poly *pPoly = new Poly;
         pPoly->m_id = count;
         m_pPolyArray->Add(pPoly);  // add the polygon to the Map's collection
         count++;

         continue;   // continue to next record
         }

      // create a poly...
      Poly *pPoly = new Poly;
      pPoly->m_id = count;

      // is it a polygon/line object?
      if (shapeType == 5 || shapeType == 15 || shapeType == 3 || shapeType == 13 || shapeType == 23)
         {
         // polygon bounding box...
         pPoly->m_xMin = (REAL)GetLittleDouble(fp);
         pPoly->m_yMin = (REAL)GetLittleDouble(fp);
         pPoly->m_xMax = (REAL)GetLittleDouble(fp);
         pPoly->m_yMax = (REAL)GetLittleDouble(fp);

         if (pPoly->m_xMin < vxMin) vxMin = pPoly->m_xMin;
         if (pPoly->m_xMax > vxMax) vxMax = pPoly->m_xMax;

         if (pPoly->m_yMin < vyMin) vyMin = pPoly->m_yMin;
         if (pPoly->m_yMax > vyMax) vyMax = pPoly->m_yMax;

         DWORD numParts = GetLittleDWORD(fp);
         DWORD numPoints = GetLittleDWORD(fp);
         //ASSERT( numParts > 0 );

         if (numParts == 0 || numPoints == 0)
            {
            //CString msg;
            //msg.Format( "Void polygon encountered reading shape file %s at record %i.  Do you want to delete void polygons from the coverage?",
            //   filename, count );
            //
            //AfxMessageBox( msg );
            }
         else
            {
            pPoly->m_vertexPartArray.SetSize(numParts);

            for (int i = 0; i < (int)numParts; i++)
               {
               DWORD parts = GetLittleDWORD(fp);
               pPoly->m_vertexPartArray[i] = parts;
               }

            // and start populating it with vertexes
            Vertex v;
            for (int i = 0; i < (int)numPoints; i++)
               {
               v.x = (REAL)GetLittleDouble(fp);
               v.y = (REAL)GetLittleDouble(fp);
               pPoly->AddVertex(v, false);

               CHECK_VALID_NUMBER(v.x);
               CHECK_VALID_NUMBER(v.y);
               }

            if (measured)  // just ignore these
               {
               GetLittleDouble(fp);  // M min
               GetLittleDouble(fp);  // M max

               for (int i = 0; i < (int)numPoints; i++)
                  GetLittleDouble(fp);  // measured point array
               }

            if (shapeType == 15 || shapeType == 13)  // 3d polygons or lines?
               {
               REAL zMin = (REAL)GetLittleDouble(fp);
               REAL zMax = (REAL)GetLittleDouble(fp);

               CHECK_VALID_NUMBER(zMin);
               CHECK_VALID_NUMBER(zMax);

               if (zMin < m_vzMin) m_vzMin = zMin;
               if (zMax > m_vzMax) m_vzMax = zMax;

               for (int i = 0; i < (int)numPoints; i++)
                  {
                  REAL z = (REAL)GetLittleDouble(fp);
                  if (_isnan(z))
                     {
                     pPoly->m_vertexArray[i].z = 0;
                     TRACE("Found NAN: Poly %i, point %i (z)\n", pPoly->m_id, i);
                     }

                  else
                     pPoly->m_vertexArray[i].z = z;
                  }

               //determine if M array is present (offsets based on ESRI docs)
               //static header components + parts
               DWORD totContent = 44 + (4 * numParts);
               //num points
               totContent += 16 * numPoints;
               //z offsets
               totContent += 16 + (8 * numPoints);

               //if totContent is smaller than content length, then there is an M array to parse
               //content length is in 16-bit (2 byte) words; we are measuring in bytes
               if (totContent < contentLength * 2)
                  {
                  //if you get here, then [ totContent-(contentLength*2) = 16+(8*numPoints) ]

                  // skip measures..
                  GetLittleDouble(fp);  // M min
                  GetLittleDouble(fp);  // M max

                  for (int i = 0; i < (int)numPoints; i++)
                     GetLittleDouble(fp);  // M array
                  }
               }
#ifdef _DEBUG
            // check for valid part descriptors (start vertex must be same is end vertex)
            if (m_layerType == LT_POLYGON)
               {
               int start, end;
               end = -1;
               for (int i = 0; i < (int)numParts; i++)
                  {
                  start = end + 1;

                  if (i == (int)numParts - 1) // last part?
                     end = numPoints - 1;
                  else
                     end = pPoly->m_vertexPartArray[i + 1] - 1;

                  const Vertex &vStart = pPoly->GetVertex(start);
                  const Vertex &vEnd = pPoly->GetVertex(end);

                  if (vStart.x != vEnd.x || vStart.y != vEnd.y || vStart.z != vEnd.z)
                     {
                     // add back if numParts == 1
                     if (numParts == 1)
                        pPoly->AddVertex(vStart, false);
                     else
                        {
                        TRACE("Unfixable part (unmatched vertices): Part number %i, Poly id: %i\n", i, pPoly->m_id);
                        TRACE("Vertex Dump:\n  Start= %f, %f, %f\n", (float)vStart.x, (float)vStart.y, (float)vStart.z);
                        TRACE(" End =    %f, %f, %f\n", (float)vEnd.x, (float)vEnd.y, (float)vEnd.z);

                        for (int j = 0; j < (int)numPoints; j++)
                           {
                           for (int k = 0; k < (int)numParts; k++)
                              {
                              if (j == (int)pPoly->m_vertexPartArray[k]) // is this the start of a part?
                                 {
                                 //TRACE("  Part %i\n", j);
                                 break;
                                 }

                              //TRACE("    %.1f, %.1f, %.1f\n", (float)pPoly->m_vertexArray[j].x, (float)pPoly->m_vertexArray[j].y, (float)pPoly->m_vertexArray[j].z);
                              }
                           }
                        }
                     }
                  }

               //ASSERT( vStart.x == vEnd.x );
               //ASSERT( vStart.y == vEnd.y );
               //ASSERT( vStart.z == vEnd.z );
               }
#endif
            }  // end of:  if ( polygon/line)
         }

      else  // point coverage
         {
         ASSERT(shapeType == 1 || shapeType == 11);

         pPoly->m_vertexPartArray.SetSize(1);
         pPoly->m_vertexPartArray[0] = 1;

         // populate a single vertex
         Vertex v;
         v.x = (REAL)GetLittleDouble(fp);
         v.y = (REAL)GetLittleDouble(fp);
         pPoly->AddVertex(v, false);

         ASSERT(v.x >= m_vxMin);
         ASSERT(v.x <= m_vxMax);
         ASSERT(v.y >= m_vyMin);
         ASSERT(v.y <= m_vyMax);

         if (v.x < vxMin) vxMin = v.x;
         if (v.x > vxMax) vxMax = v.x;

         if (v.y < vyMin) vyMin = v.y;
         if (v.y > vyMax) vyMax = v.y;

         pPoly->m_xMin = pPoly->m_xMax = v.x;
         pPoly->m_yMin = pPoly->m_yMax = v.y;

         if (shapeType == 11)  // 3d point?
            {
            pPoly->m_vertexArray[0].z = (REAL)GetLittleDouble(fp);
            GetLittleDouble(fp);    // skip measures
            }
         }  // end of:  if ( polygon/line)

      m_pPolyArray->Add(pPoly);  // add the polygon to the Map's collection
      count++;

      int retVal = m_pMap->Notify(NT_LOADVECTOR, count, 0);

      if (retVal == 0) // escaped out?
         {
         records = count;
         m_recordsLoaded = records;
         }

      if (count == records)    // count is the number of records loaded so far
         break;
      }  // end of: while ( feof() == 0 )  

   // end of file reached, continue

   // update extents
   m_vxMin = vxMin;
   m_vxMax = vxMax;
   m_vyMin = vyMin;
   m_vyMax = vyMax;

   if (m_pMap->GetLayerCount() == 1)
      {
      m_pMap->SetMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);
      //m_pMap->ZoomFull( false );  // no redraw
      }
   else
      m_pMap->AddMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);   // must do this prior to  InitPolyLogicalPoints()

   InitPolyLogicalPoints();   // create logical points for this set   

   //-- clean up --// 
   m_pMap->Notify(NT_LOADVECTOR, -count, 0);

   fclose(fp);

   m_name = filename;
   m_path = filename;
   m_includeData = loadDB;

   if (loadDB)
      {
      // get corresponding dbf file
      char *dbf = new char[lstrlen(filename) + 1];
      lstrcpy(dbf, filename);
      char *p = strrchr(dbf, '.');
      p++;
      lstrcpy(p, _T("dbf"));

      CString sql("SELECT * FROM ");
      sql += dbf;
      m_tableName = dbf;

      // get path of shape file
      p = strrchr(dbf, '\\');

      if (p == NULL)  // no path
         {
#ifndef NO_CODEBASE
         LoadDataDBF(m_tableName, extraCols, records);
#else
#ifdef USE_SHAPELIB
         LoadDataDBF(m_tableName, extraCols, records);
#else
         LoadDataDB(".", "DBASE IV;", sql, extraCols, records);
#endif

#endif
         }
      else
         {
         p++;
         *p = NULL;
#ifndef NO_CODEBASE
         LoadDataDBF(m_tableName, extraCols, records);
#else
#ifdef USE_SHAPELIB
         VData::SetUseWideChar(true);
         LoadDataDBF(m_tableName, extraCols, records);
#else
         LoadDataDB(dbf, "DBASE IV;", sql, extraCols, records);
#endif

#endif
         }

      delete[] dbf;
      }


   // get corresponding prj file
   m_projection.Empty();

   char *prj = new char[lstrlen(filename) + 1];
   lstrcpy(prj, filename);
   char *p = strrchr(prj, '.');
   p++;
   lstrcpy(p, _T("prj"));

   FILE *fpPrj = NULL;
   fopen_s(&fpPrj, prj, "rt");

   if (fpPrj != NULL)
      {
      TCHAR buffer[1024];

      while (fgets(buffer, 255, fpPrj))
         m_projection += buffer;

      fclose(fpPrj);
      }

   delete prj;

   // make BinArrayArray
   if (m_pDbTable)
      {
      int polys = m_pPolyArray->GetSize();
      m_polyBinArray.SetSize(polys);
      InitPolyBins();

      m_binArrayArray.Init(m_pDbTable->GetColCount());
      ASSERT(GetPolygonCount() == GetRecordCount());

      m_pDbTable->m_databaseName = m_tableName;
      m_pDbTable->m_tableName = m_tableName;
      }

   CString msg;
   msg.Format("Loaded shape file %s, %i polygons, %i records\n", filename, GetPolygonCount(), GetRecordCount());
   TRACE(msg);

   //TRACE("End of LoadShape()\n");
   GetPolygon(0)->TraceVertices(1);

   // update extents (only if map layer is missing extents)
   if (m_pMap->m_vxMapMaxExt == m_pMap->m_vxMapMinExt)
      {
      this->UpdateExtents();
      m_pMap->UpdateMapExtents();
      //   m_pMap->ZoomFull();
      }

   return count;
   }


int MapLayer::LoadVectorsCSV(LPCTSTR filename, LPCTSTR data /*=NULL*/)
   {
   ClearPolygons();   // clear any existing polygons
   ClearData();   // add associated data

   FILE *fp;
   fopen_s(&fp, filename, "rb");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -1;
      }

   // start loading
   const int MAXLINEWIDTH = 255;
   TCHAR buffer[MAXLINEWIDTH + 1];

   m_layerType = LT_POLYGON;
   m_3D = false;

   // set extents to bounding values...
   m_vxMin = m_vyMin = (REAL)LONG_MAX;
   m_vxMax = m_vyMax = (REAL)LONG_MIN;

   int currentPolyID = -1;
   int currentPartID = -1;
   Poly *pPoly = NULL;
   int count = 0;

   while (!feof(fp))
      {
      //-- start getting data --//
      fgets(buffer, MAXLINEWIDTH, fp);

      if (*buffer == NULL || *buffer == '\n' || *buffer == '\r')      // end of file found (two "ENDS")
         break;

      // FORMAT MUST BE POLYID, PARTID, X, Y
      int polyID, partID;
      double x, y;

      if (sscanf_s(buffer, "%i,%i,%lf,%lf", &polyID, &partID, &x, &y) != 4)
         return -2;

      Vertex v( (REAL) x, (REAL) y );

      // new polygon definition?  Then make a new polygon
      if (polyID != currentPolyID)
         {
         // if there is an existing polygon, close the vertex list
         if (pPoly != NULL)
            pPoly->CloseVertices();

         currentPolyID = polyID;
         currentPartID = partID;
         pPoly = new Poly;
         pPoly->m_id = polyID;
         m_pPolyArray->Add(pPoly);  // add the polygon to the Map's collection

         currentPartID = -1;
         count++;
         }

      // add vertex, tracking parts as needed
      pPoly->AddVertex(v, false);

      // is this a new part?  If so, add to the parts list
      if (partID != currentPartID)
         {
         pPoly->m_vertexPartArray.Add(pPoly->GetVertexCount() - 1);
         currentPartID = partID;
         }

      if (v.x > m_vxMax) m_vxMax = v.x;     // check bounds
      if (v.y > m_vyMax) m_vyMax = v.y;
      if (v.x < m_vxMin) m_vxMin = v.x;
      if (v.y < m_vyMin) m_vyMin = v.y;

      fgets(buffer, MAXLINEWIDTH, fp);     // get next line         

      m_pMap->Notify(NT_LOADVECTOR, count, 0);
      }  // end of file reached, continue

   pPoly->CloseVertices();

   /*
#ifdef _DEBUG
         // check for valid part descriptors (start vertex must be same is end vertex)
         if ( m_layerType == LT_POLYGON )
            {
            int start, end;
            end = -1;
            for ( int i=0; i < (int) numParts; i++ )
               {
               start = end+1;

               if ( i == (int) numParts-1) // last part?
                  end = numPoints-1;
               else
                  end = pPoly->m_vertexPartArray[i+1]-1;

               const Vertex &vStart = pPoly->GetVertex( start );
               const Vertex &vEnd   = pPoly->GetVertex( end   );

               if ( vStart.x != vEnd.x || vStart.y != vEnd.y || vStart.z != vEnd.z )
                  {
                  // add back if numParts == 1
                  if ( numParts == 1 )
                     pPoly->AddVertex( vStart, false );
                  else
                     {
                     TRACE("Unfixable part (unmatched vertices): Part number %i, Poly id: %i\n", i, pPoly->m_id );
                     TRACE("Vertex Dump:\n  Start= %f, %f, %f\n", (float) vStart.x, (float) vStart.y, (float) vStart.z );
                     TRACE(" End =    %f, %f, %f\n", (float) vEnd.x, (float) vEnd.y, (float) vEnd.z );

                     for ( int j=0; j < (int) numPoints; j++ )
                        {
                        for ( int k=0; k < (int) numParts; k++ )
                           {
                           if ( j == (int) pPoly->m_vertexPartArray[ k ] ) // is this the start of a part?
                              {
                              TRACE( "  Part %i\n", j );
                              break;
                              }

                           TRACE("    %.1f, %.1f, %.1f\n", (float) pPoly->m_vertexArray[j].x, (float) pPoly->m_vertexArray[j].y, (float) pPoly->m_vertexArray[j].z );
                           }
                        }
                     }
                  }
               }

            //ASSERT( vStart.x == vEnd.x );
            //ASSERT( vStart.y == vEnd.y );
            //ASSERT( vStart.z == vEnd.z );
            }
#endif
            */


            /*int retVal = m_pMap->Notify( NT_LOADVECTOR, count, 0 );

               if ( retVal == 0) // escaped out?
                  {
                  records = count;
                  m_recordsLoaded = records;
                  }

               if ( count == records )    // count is the number of records loaded so far
                  break;
               }  // end of: while ( feof() == 0 )

            // end of file reached, continue

            */
            //if ( m_pMap->GetLayerCount() == 1 )
            //   {
            //   m_pMap->SetMapExtents( m_vxMin, m_vxMax, m_vyMin, m_vyMax );
            //   m_pMap->ZoomFull( false );  // no redraw
            //   }
            //else
            //   m_pMap->AddMapExtents( m_vxMin, m_vxMax, m_vyMin, m_vyMax );   // must do this prior to  InitPolyLogicalPoints()
            //
            //InitPolyLogicalPoints();   // create logical points for this set   

            //-- clean up --// 
   m_pMap->Notify(NT_LOADVECTOR, -count, 0);

   fclose(fp);

   m_name = filename;
   m_path = filename;
   m_includeData = data == NULL ? false : true;

   if (data)
      {
      // TODO - load csv
      }


   // get corresponding prj file
   m_projection.Empty();

   char *prj = new TCHAR[lstrlen(filename) + 1];
   lstrcpy(prj, filename);
   char *p = strrchr(prj, '.');
   p++;
   lstrcpy(p, _T("prj"));

   FILE *fpPrj = NULL;
   fopen_s(&fpPrj, prj, "rt");

   if (fpPrj != NULL)
      {
      TCHAR buffer[1024];

      while (fgets(buffer, 255, fpPrj))
         m_projection += buffer;

      fclose(fpPrj);
      }

   delete prj;

   // make BinArrayArray
   if (m_pDbTable)
      {
      int polys = m_pPolyArray->GetSize();
      m_polyBinArray.SetSize(polys);
      InitPolyBins();

      m_binArrayArray.Init(m_pDbTable->GetColCount());
      ASSERT(GetPolygonCount() == GetRecordCount());

      m_pDbTable->m_databaseName = m_tableName;
      m_pDbTable->m_tableName = m_tableName;
      }

   CString msg;
   msg.Format("Loaded CSV shape file %s, %i polygons, %i records\n", filename, GetPolygonCount(), GetRecordCount());
   TRACE(msg);
   return 0;
   }



float MapLayer::GetTotalArea(bool forceRecalc /*=false*/)
   {
   if (m_totalArea >= 0.0f && !forceRecalc)
      return m_totalArea;

   m_totalArea = 0.0f;

   switch (m_layerType)
      {
      case LT_POLYGON:
         {
         int colArea = GetFieldCol("AREA");

         for (Iterator i = Begin(); i != End(); i++)
            {
            float area;

            if (colArea >= 0)
               GetData(i, colArea, area);
            else
               {
               Poly *pPoly = GetPolygon(i);
               area = pPoly->GetArea();
               }

            m_totalArea += area;
            }

         return m_totalArea;
         }
      }

   return m_totalArea = 0.0f;
   }


int MapLayer::ClonePolygons(const MapLayer *pLayerToCopyFrom)
   {
   // Clone both alive and dead polygons
   if (pLayerToCopyFrom->GetType() != LT_POLYGON && pLayerToCopyFrom->GetType() != LT_LINE && pLayerToCopyFrom->GetType() != LT_POINT)
      return -1;

   // clean anthing existing out
   ClearPolygons();
   ClearData();

   // copy polygons
   INT_PTR polyCount = pLayerToCopyFrom->m_pPolyArray->GetSize();
   if (polyCount > 0)
      {
      m_pPolyArray->SetSize(polyCount);

      // clone all the polygons
      for (INT_PTR i = 0; i < polyCount; i++)
         {
         m_pPolyArray->GetAt(i) = new Poly(*pLayerToCopyFrom->m_pPolyArray->GetAt(i));  // no bins, everything else ok
         ASSERT(m_pPolyArray->GetAt(i) != NULL);
         }
      }
   // copy m_defunctPolyList
   m_defunctPolyList.Copy(pLayerToCopyFrom->m_defunctPolyList);

   m_layerType = pLayerToCopyFrom->GetType();
   m_vxMin = pLayerToCopyFrom->m_vxMin;    // virtual coords
   m_vxMax = pLayerToCopyFrom->m_vxMax;
   m_vyMin = pLayerToCopyFrom->m_vyMin;
   m_vyMax = pLayerToCopyFrom->m_vyMax;
   m_vzMin = pLayerToCopyFrom->m_vzMin;
   m_vzMax = pLayerToCopyFrom->m_vzMax;
   m_vExtent = pLayerToCopyFrom->m_vExtent;

   //m_pMap->UpdateMapExtents();  // calls initPolyLogicalPts
   //m_pMap->ZoomFull();

   // grid-only information
   m_cellWidth = m_cellHeight = m_noDataValue = 0.0f;
   m_selection.RemoveAll();

   m_polyBinArray.SetSize(m_pPolyArray->GetSize());
   InitPolyBins();

   // note this function doesn't take care of teh following...
   //  Map *m_pMap;
   //   CString    m_name;       // name of the layer (defaults to path name)
   //   CString    m_path;       // full path name for vector file
   //   CString    m_database;   // full path for database file (no filename for DBase)
   //   CString    m_tableName;  // table name (file name for DBase)

   //   BinArray   m_pBinArray;   // CArray of classification bins
   //   DbTable   *m_pDbTable;   // table of data for this layer
   //   VDataObj  *m_pData;      // pointer to the VDataObj of the m_pDbTable

   //   int    m_activeField;   // column in data object
   //   bool   m_isVisible;

   return polyCount;
   }


int MapLayer::MergePolygons()
   {
   int selCount = GetSelectionCount();

   if (selCount == 0)
      return 0;

   int colArea = GetFieldCol("Area");
   if (colArea < 0)
      return -1;

   int startPolyID = this->GetSelection(0);
   float maxArea;
   GetData(startPolyID, colArea, maxArea);

   for (int i = 0; i < selCount; i++)
      {
      float area;
      int polyID = this->GetSelection(i);
      GetData(polyID, colArea, area);

      if (area > maxArea)
         {
         startPolyID = polyID;
         maxArea = area;
         }
      }

   // have max area polygon, merge others in.
   for (int i = 0; i < selCount; i++)
      {
      float area;
      int polyID = this->GetSelection(i);
      GetData(polyID, colArea, area);

      if (area > maxArea)
         {
         startPolyID = polyID;
         maxArea = area;
         }
      }

   return selCount;
   }


int MapLayer::ConvertToPoly( bool addToMap, bool classify )
   {
   if (m_layerType != LT_GRID)
      return -1;

   ClearPolygons();       // clear any existing polygons (shouldn't be any!)

   int rows = this->GetRowCount();
   int cols = this->GetColCount();

   DO_TYPE doType = this->m_pDbTable->GetDOType();
   VDataObj *pData = new VDataObj(3, rows*cols, U_UNDEFINED); 
   
   pData->SetLabel(0, "Value");
   pData->SetLabel(1, "Row");
   pData->SetLabel(2, "Col");

   this->m_layerType = LT_POLYGON;

   int record = 0;
   for (int i = 0; i < rows; i++)
      {
      for (int j = 0; j < cols; j++)
         {
         COORD2d ll, ur;
         this->GetGridCellRect(i, j, ll, ur);

         // make a polygon representing this cell
         Poly *pPoly = new Poly;

         // add for corners
         pPoly->AddVertex(Vertex(ll.x, ll.y));
         pPoly->AddVertex(Vertex(ll.x, ur.y));
         pPoly->AddVertex(Vertex(ur.x, ur.y));
         pPoly->AddVertex(Vertex(ur.x, ll.y));

         this->AddPolygon(pPoly, false);

         VData value;
         this->GetData(i, j, value);

         switch (doType)
            {
            case DOT_VDATA:   pData->Set(0,record, value);  break;
            case DOT_DOUBLE:  pData->Set(0,record, value.GetDouble()); break;
            case DOT_FLOAT:   pData->Set(0,record, value.GetFloat());  break;
            case DOT_INT:     pData->Set(0,record, value.GetInt()); break;
            }

         pData->Set(1, record, i);
         pData->Set(2, record, j);

         record++;
         }
      }

   CString msg;
   msg.Format("PolyCount: %i, Records: %i", this->GetPolygonCount(), this->GetRecordCount());
   Report::Log(msg);
   ClearData();   // and any associated data

   m_layerType = LT_POLYGON;

   // attach data to a dbtable object 
   m_pDbTable = new DbTable(pData);
   m_pData = pData;
   // first layer?
   if (addToMap && m_pMap)
      {
      if (m_pMap->GetLayerCount() == 1)
         {
         m_pMap->SetMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);
         //m_pMap->ZoomFull( false );  // no redraw
         }
      else
         m_pMap->AddMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);
      }

   m_name = "PolyGrid";
   m_path = "PolyGrid.shp";

   // do default grid classification
   //if (classify && m_pMap)
   //   {
   //   //m_pMap->Notify( NT_CLASSIFYING, 0, 0 );
   //   float minVal, maxVal;
   //
   //   GetDataMinMax(-1, &minVal, &maxVal);
   //   Report::StatusMsg("start set bins");
   //   SetBins(0, minVal, maxVal, 10, _type);
   //   Report::StatusMsg("stop set bins");
   //   }

   return true;
   }


/*int MapLayer::CreateGrid( MapLayer *pLayer, float cellsize, float initialValue, DO_TYPE type ) // Create from an existing maplayer
   {
   float xLLCorner = pLayer->m_vxMin;
   float yLLCorner = pLayer->m_vyMin;

   int cols = ( pLayer->m_vxMax - pLayer->m_vxMin ) / cellsize;
   int rows = ( pLayer->m_vyMax - pLayer->m_vyMin ) / cellsize;

   return CreateGrid( rows, cols, xLLCorner, yLLCorner, cellsize, initialValue, type );
   }
   */

int MapLayer::CreateGrid(int rows, int cols, REAL xLLCorner,REAL yLLCorner, REAL cellWidth, REAL cellHeight, float initialValue, DO_TYPE type, bool addToMap, bool classify)
   {
   ClearPolygons();       // clear any existing polygons
   ClearData();   // and any associated data

   m_layerType = LT_GRID;

   // allocate data object
   TYPE _type = TYPE_NULL;
   switch (type)
      {
      case DOT_INT:
         m_pData = new IDataObj(cols, rows, U_UNDEFINED);
         _type = TYPE_INT;
         break;

      case DOT_FLOAT:
         m_pData = new FDataObj(cols, rows, U_UNDEFINED);
         _type = TYPE_FLOAT;
         break;

      default:
         m_pData = new VDataObj(cols, rows, U_UNDEFINED);
         _type = TYPE_STRING;
         break;
      }

   SetCellSize(cellWidth, cellHeight);  // same size (square) ???

   COORD2d ll(xLLCorner, yLLCorner);
   SetLowerLeft(ll);
   SetNoDataValue(-9999);

#pragma omp parallel for 
   for (int row = 0; row < rows; row++)
      for (int col = 0; col < cols; col++)
         m_pData->Set(col, row, initialValue);

   // attach data to a dbtable object 
   m_pDbTable = new DbTable(m_pData);

   // data loaded, clean up
   m_vxMax = m_vxMin + m_pData->GetColCount() * m_cellWidth;
   m_vyMax = m_vyMin + m_pData->GetRowCount() * m_cellHeight;

   // first layer?
   if (addToMap && m_pMap)
      {
      if (m_pMap->GetLayerCount() == 1)
         {
         m_pMap->SetMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);
         //m_pMap->ZoomFull( false );  // no redraw
         }
      else
         m_pMap->AddMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);
      }

   m_name = "Grid";
   m_path = "Grid.grd";

   // add ONE binarray
   m_binArrayArray.Init(1);

   // do default grid classification
   if (classify && m_pMap)
      {
      //m_pMap->Notify( NT_CLASSIFYING, 0, 0 );
      float minVal, maxVal;

      GetDataMinMax(-1, &minVal, &maxVal);
      Report::StatusMsg("start set bins");
      SetBins(0, minVal, maxVal, 10, _type);
      Report::StatusMsg("stop set bins");
      }
   return TRUE;
   }

int MapLayer::CreateGrid(int rows, int cols, REAL xLLCorner, REAL yLLCorner, REAL cellsize, float initialValue, DO_TYPE type, bool addToMap, bool classify)
   {
   int ok = 0;

   ok = CreateGrid(rows, cols, xLLCorner, yLLCorner, cellsize, cellsize, initialValue, type, addToMap, classify);

   return ok;
   }


/*int MapLayer::CreateGridFromPoly( MapLayer *pLayer, float cellsize, DO_TYPE type, bool addToMap, bool classify )
   {
   BOOL ok = this->CreateGrid( pLayer, cellsize, m_noDataValue, type, addToMap, classify );

   if( ok )
      return FALSE;

   }
   */


   //-- MapLayer::LoadGrid() -------------------------------------------
   //
   //-- loads an ArcInfo-exported ascii grid file
   //-------------------------------------------------------------------

int MapLayer::LoadGrid(LPCTSTR filename, DO_TYPE type, int maxLineWidth, int binCount /*=10*/, BCFLAG bcFlag /*=BCF_GRAY*/)
   {
   ClearPolygons();       // clear any existing polygons
   ClearData();   // and any associated data

   m_layerType = LT_GRID;
   m_binColorFlag = bcFlag;

   nsPath::CPath path(filename);

   CString ext = path.GetExtension();

   int result = -1;

   if (ext.CompareNoCase("bil") == 0)
      result = LoadGridBIL(filename);
   else if (ext.CompareNoCase("asc") == 0)
      result = LoadGridAscii(filename, type, maxLineWidth);
   else if (ext.CompareNoCase("flt") == 0)
      result = LoadGridFLT(filename, type);

   if (result < 0)
      return -1;

   // data loaded, clean up
   m_vxMax = m_vxMin + m_pData->GetColCount() * m_cellWidth;
   m_vyMax = m_vyMin + m_pData->GetRowCount() * m_cellHeight;

   // first layer?
   if (m_pMap->GetLayerCount() == 1)
      {
      m_pMap->SetMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);
      //m_pMap->ZoomFull( false );  // no redraw
      }
   else
      m_pMap->AddMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);   // must do this prior to  InitPolyLogicalPoints()

   m_name = filename;
   m_path = filename;

   TYPE _type = TYPE_STRING;
   if (type == DOT_FLOAT)
      _type = TYPE_FLOAT;
   else if (type == DOT_INT)
      _type = TYPE_INT;

   if (binCount > 0)
      {
      //m_pMap->Notify( NT_CLASSIFYING, 0, 0 );

      // do default grid classification

      // add ONE binarray
      m_binArrayArray.Init(1);

      float minVal, maxVal;
      GetDataMinMax(-1, &minVal, &maxVal);

      Report::StatusMsg("start set bins");


      SetBins(0, minVal, maxVal, 10, _type);
      Report::StatusMsg("stop set bins");

      m_activeField = 0;
      }

   return 1;
   }



int MapLayer::LoadGridBIL(LPCTSTR filename)
   {
   nsPath::CPath path(filename);
   path.RenameExtension(".hdr");

   PCTSTR file = (PCTSTR)path;

   FILE *fp;
   fopen_s(&fp, file, "rt");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += (LPCTSTR)path;;
      Report::ErrorMsg(msg);
      return -1;
      }

   char buffer[128];

   //-- get header information -----------------------
   //
   //-- NOTE: Header looks like: (starts in col 0)
   //
   //   BYTEORDER M
   //   LAYOUT BIL
   //   NROWS 100
   //   NCOLS 63
   //   NBANDS 1
   //   NBITS 8
   //   ULXMAP 330000
   //   ULYMAP 6500000
   //   XDIM 400
   //   YDIM 400
   //-------------------------------------------------
   char byteOrder = 'I';
   int   nCols, nRows, nBits = 8, nBands = 1, bandRowBytes = -1, skipBytes = 0;

   double ulxMap, ulyMap, xDim, yDim;
   while (fgets(buffer, 128, fp) != NULL)
      {
      TCHAR *param = strchr(buffer, ' ');

      if (_strnicmp(buffer, "BYTEORDER", 9) == 0)
         byteOrder = *(param + 1);
      else if (_strnicmp(buffer, "LAYOUT", 6) == 0)
         ASSERT(_strnicmp(param + 1, "BIL", 3) == 0);
      else if (_strnicmp(buffer, "NROWS", 5) == 0)
         nRows = atoi(param + 1);
      else if (_strnicmp(buffer, "NCOLS", 5) == 0)
         nCols = atoi(param + 1);
      else if (_strnicmp(buffer, "NBANDS", 6) == 0)
         nBands = atoi(param + 1);
      else if (_strnicmp(buffer, "NBITS", 5) == 0)
         nBits = atoi(param + 1);
      else if (_strnicmp(buffer, "ULXMAP", 6) == 0)
         ulxMap = atof(param + 1);
      else if (_strnicmp(buffer, "ULYMAP", 6) == 0)
         ulyMap = atof(param + 1);
      else if (_strnicmp(buffer, "XDIM", 4) == 0)
         xDim = atof(param + 1);
      else if (_strnicmp(buffer, "YDIM", 4) == 0)
         yDim = atof(param + 1);
      else if (_strnicmp(buffer, "BANDROWBYTES", 12) == 0)
         bandRowBytes = atoi(param + 1);
      else if (_strnicmp(buffer, "SKIPBYTES", 9) == 0)
         skipBytes = atoi(param + 1);
      else
         continue;
      }
   fclose(fp);

   if (bandRowBytes < 0)
      {
      float _bandRowBytes = (nCols*nBits) / 8.0f;
      bandRowBytes = (nCols*nBits) / 8;
      if (_bandRowBytes - bandRowBytes > 0.001)
         bandRowBytes++;
      }

   // have basic hdr info, proceed with reading data file
   fopen_s(&fp, filename, "rb");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -1;
      }

   // allocate data object
   m_pData = new IDataObj(nCols, nRows, U_UNDEFINED);
   double xLLCorner = ulxMap - xDim / 2;
   double yLLCorner = ulyMap - (nRows - 1)*yDim - yDim / 2;

   COORD2d ll((float)xLLCorner, (float)yLLCorner);
   SetCellSize((float)xDim, (float)yDim);
   SetLowerLeft(ll);
   SetDataDim(nCols, nRows);
   SetNoDataValue(-99);

   // ready to start reading...
   int totalRowBytes = nBands * bandRowBytes;
   int totalBytes = totalRowBytes * nRows;

   BYTE *data = new BYTE[skipBytes + totalBytes];
   size_t size = fread(data, sizeof(BYTE), skipBytes + totalBytes, fp);
   if (size != skipBytes + totalBytes)
      {
      CString msg("Error reading file:  ");
      msg += filename;
      Report::ErrorMsg(msg);
      delete[] data;
      return -2;
      }

   fclose(fp);

   int value;
   int count = 0;
   int position = skipBytes;

   for (int row = 0; row < nRows; row++)
      {
      m_pMap->Notify(NT_LOADGRIDROW, row, 0);

      position = skipBytes + (row*totalRowBytes);
      count = 0;

      for (int col = 0; col < nCols; col++)
         {
         switch (nBits)
            {
            case 1:
            case 4:
               {
               BYTE _value = data[position];
               value = (int)_value;
               position++;
               ASSERT(0);   // this code is incorrect - needs fixing
               }
               break;

            case 8:
               {
               BYTE _value = data[position];
               value = (int)_value;
               position++;
               }
               break;

            case 16:
               {
               short int _value = *((short int*)(data + position));
               value = (int)_value;
               position += 2;
               }
               break;

            case 32:
               value = *((int*)(data + position));
               position += 4;
               break;
            }

         m_pData->Set(col, row, value);
         }  // end of:  for ( col < nCols )
      }  // end of:  for ( row < nRows )

   // attach data to a dbtable object 
   m_pDbTable = new DbTable(m_pData);

   //-- clean up --// 
   delete[] data;
   m_pMap->Notify(NT_LOADGRIDROW, -nRows, 0);

   return nRows;
   }


int MapLayer::LoadGridFLT(LPCTSTR filename, DO_TYPE type)
   {
   nsPath::CPath path(filename);
   path.RenameExtension(".hdr");

   PCTSTR file = (PCTSTR)path;

   FILE *fp;
   fopen_s(&fp, file, "rt");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += (LPCTSTR)path;;
      Report::ErrorMsg(msg);
      return -1;
      }

   char buffer[128];

   //-- get header information -----------------------
   //
   //-- NOTE: Header looks like: (starts in col 0)
   //
   //  ncols 139 
   //  nrows 145 
   //  xllcorner -108010 
   //  yllcorner 486980
   //  cellsize 27.4 
   //  NODATA_value -9999 
   //  byteorder LSBFIRST
   //-------------------------------------------------
   char byteOrder = 'I';
   int   nCols, nRows;

   double xllCorner, yllCorner, cellSize, noDataValue = -9999;
   while (fgets(buffer, 128, fp) != NULL)
      {
      TCHAR *param = strchr(buffer, ' ');

      if (_strnicmp(buffer, "BYTEORDER", 9) == 0)
         byteOrder = *(param + 1);
      else if (_strnicmp(buffer, "NROWS", 5) == 0)
         nRows = atoi(param + 1);
      else if (_strnicmp(buffer, "NCOLS", 5) == 0)
         nCols = atoi(param + 1);
      else if (_strnicmp(buffer, "XLLCORNER", 9) == 0)
         xllCorner = atof(param + 1);
      else if (_strnicmp(buffer, "YLLCORNER", 9) == 0)
         yllCorner = atof(param + 1);
      else if (_strnicmp(buffer, "CELLSIZE", 8) == 0)
         cellSize = atof(param + 1);
      else if (_strnicmp(buffer, "NODATA", 6) == 0)
         noDataValue = atof(param + 1);
      else
         continue;
      }
   fclose(fp);

   // have basic hdr info, proceed with reading data file
   fopen_s(&fp, filename, "rb");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -1;
      }

   // allocate data object
   switch (type)
      {
      case DOT_INT:
         m_pData = new IDataObj(nCols, nRows, U_UNDEFINED);
         break;

      case DOT_FLOAT:
         m_pData = new FDataObj(nCols, nRows, U_UNDEFINED);
         break;

      default:
         return -1;
      }

   SetCellSize((float)cellSize, (float)cellSize);
   COORD2d ll((float)xllCorner, (float)yllCorner);
   SetLowerLeft(ll);
   SetDataDim(nCols, nRows);
   SetNoDataValue(noDataValue);

   float *data = new float[nRows*nCols];
   size_t size = fread(data, sizeof(float), nRows*nCols, fp);
   if (size != nRows*nCols)
      {
      CString msg("Error reading file:  ");
      msg += filename;
      Report::ErrorMsg(msg);
      delete[] data;
      return -2;
      }

   fclose(fp);

   int position = 0;

   for (int row = 0; row < nRows; row++)
      {
      m_pMap->Notify(NT_LOADGRIDROW, row, 0);

      for (int col = 0; col < nCols; col++)
         {
         float value = data[position];
         m_pData->Set(col, row, value);
         position++;
         }  // end of:  for ( col < nCols )
      }  // end of:  for ( row < nRows )

   // attach data to a dbtable object 
   m_pDbTable = new DbTable(m_pData);

   //-- clean up --// 
   delete[] data;
   m_pMap->Notify(NT_LOADGRIDROW, -nRows, 0);

   return nRows;
   }



int MapLayer::LoadGridAscii(LPCTSTR filename, DO_TYPE type, int maxLineWidth/*=-1*/)
   {
   FILE *fp;
   fopen_s(&fp, filename, "rb");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -1;
      }

   char *buffer = new char[100];

   //-- get header information -----------------------
   //
   //-- NOTE: Header looks like: (starts in col 0)
   //
   //   NCOLS 200
   //   NROWS 120
   //   XLLCORNER 60
   //   YLLCORNER -10
   //   CELLSIZE 0.5
   //   NODATA_VALUE -99.9
   //-------------------------------------------------
   int   nCols, nRows, xLLCorner, yLLCorner;
   float cellSize, noDataValue;
   char varname[32];

   char *err = fgets(buffer, 100, fp);
   ASSERT(err != NULL);
   sscanf_s(buffer, "%s %d", varname, 32, &nCols);

   err = fgets(buffer, 100, fp);
   ASSERT(err != NULL);
   sscanf_s(buffer, "%s %d", varname, 32, &nRows);

   err = fgets(buffer, 100, fp);
   ASSERT(err != NULL);
   sscanf_s(buffer, "%s %d", varname, 32, &xLLCorner);

   err = fgets(buffer, 100, fp);
   ASSERT(err != NULL);
   sscanf_s(buffer, "%s %d", varname, 32, &yLLCorner);

   err = fgets(buffer, 100, fp);
   ASSERT(err != NULL);
   sscanf_s(buffer, "%s %f", varname, 32, &cellSize);

   fgets(buffer, 100, fp);
   sscanf_s(buffer, "%s %f", varname, 32, &noDataValue);

   // allocate data object
   switch (type)
      {
      case DOT_INT:
         m_pData = new IDataObj(nCols, nRows, U_UNDEFINED);
         break;

      case DOT_FLOAT:
         m_pData = new FDataObj(nCols, nRows, U_UNDEFINED);
         break;

      case DOT_VDATA:
         m_pData = new VDataObj(nCols, nRows, U_UNDEFINED);
         break;

      default:
         Report::ErrorMsg("Unsupported grid data type specified in LoadGrid()");
         return -2;
      }

   SetCellSize(cellSize, cellSize);  // same size (square) ???
   COORD2d ll((float)xLLCorner, (float)yLLCorner);
   SetLowerLeft(ll);
   //SetDataDim     ( nCols, nRows );
   SetNoDataValue(noDataValue);

   delete buffer;

   // if not specified, figure out line width 
   if (maxLineWidth <= 0)
      {
      fpos_t pos;
      if (fgetpos(fp, &pos) != 0)
         {
         Report::ErrorMsg(" Error setting file position");
         return -1;
         }

      int count = 0;
      int c = fgetc(fp);
      while (c != '\n' && c != '\r')
         {
         c = fgetc(fp);
         count++;
         }

      fsetpos(fp, &pos);
      maxLineWidth = count * 2;
      }

   buffer = new char[maxLineWidth];


   //-- have header, proceed with grid data --//
   int row = 0;
   for (row = 0; row < nRows; row++)
      {
      m_pMap->Notify(NT_LOADGRIDROW, row, 0);

      fgets(buffer, maxLineWidth, fp);
      char *next;

      char *p = strtok_s(buffer, " \n", &next);

      for (int col = 0; col < nCols; col++)
         {
         if (p == NULL)
            {
            char msg[128];
            sprintf_s(msg, 128, "Premature end-of-line encountered at row %d, col %d (of %d) (Line Width=%i)", row, col, nCols, maxLineWidth);
            Report::ErrorMsg(msg);
            return FALSE;
            }

         switch (type)
            {
            case DOT_INT:
               {
               int value = atoi(p);
               m_pData->Set(col, row, value);
               }
               break;

            default:
               {
               float value = (float)atof(p);
               m_pData->Set(col, row, value);
               }
               break;
            }

         p = strtok_s(NULL, " \n", &next);  // parse next value

         }  // end of:  for ( col < nCols )

      }  // end of:  for ( row < nRows )

   // attach data to a dbtable object 
   m_pDbTable = new DbTable(m_pData);

   //-- clean up --// 
   delete buffer;
   m_pMap->Notify(NT_LOADGRIDROW, -row, 0);

   fclose(fp);

   return nRows;
   }



int MapLayer::SaveShapeFile(LPCTSTR filename, bool selectedPolysOnly, int saveDefunctPolysFlag, int saveEmptyPolysFlag)
   {
   // NOTE: saveDefunctPolysFlag: 0=ask,1=save,2=don't save
   // check for defunct poly's
   if (m_deadCount == 0)
      saveDefunctPolysFlag = 1;  // saves everything

   if (m_deadCount > 0 && saveDefunctPolysFlag == 0)
      {
#ifdef _DEBUG
      int defunctCount = 0;
      for (int k = 0; k < int(this->m_defunctPolyList.GetSize()); k++)
         if (m_defunctPolyList.Get(k))
            defunctCount++;

      ASSERT(m_deadCount == defunctCount);
#endif
#ifndef NO_MFC
      int retVal = ::MessageBox(AfxGetMainWnd()->m_hWnd, "This map layer contains defunct polygons that have been replaced with new (child) polygons.  Do you want to save both the original (parent) and new (child) polygons?",
         "Save Defunct Polygons?", MB_YESNO);

      if (retVal == IDYES)
         saveDefunctPolysFlag = 1;
      else
         saveDefunctPolysFlag = 2;
#else
      //default arbitrarily picked
      saveDefunctPolysFlag = 1;
#endif
      }

   // if these are goint to be saved, make sure a database column exists to store parentID info
   if (saveDefunctPolysFlag == 1)
      {
      // note: parentID: 
      //       nodata   = not defunct
      //       parentID = this is a child polygon with the given parent ID
      //       -1       = defunct parent
      int colParentID = this->GetFieldCol("ParentID");

      if (colParentID < 0)  // missing? then add it
         {
         int width, decimals;
         GetTypeParams(TYPE_INT, width, decimals);
         colParentID = this->m_pDbTable->AddField("ParentID", TYPE_INT, width, decimals, true);
         }

      m_pDbTable->GetFieldInfo(colParentID).save = true;

      ASSERT(colParentID >= 0);

      bool readOnly = m_readOnly;
      m_readOnly = false;

      this->SetColNoData(colParentID);

      for (int i = 0; i < this->m_pPolyArray->GetSize(); i++)
         {
         Poly *pPoly = this->GetPolygon(i);

         // is it a defunct parent?
         if (this->IsDefunct(i))
            SetData(i, colParentID, -1);

         // is it a child
         else if (pPoly->GetParentCount() > 0)
            SetData(i, colParentID, pPoly->GetParent(0));

         // otherwise, let nodata value stand
         }

      m_readOnly = readOnly;
      }

   FILE *fp;
   fopen_s(&fp, filename, "wb");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -1;
      }

   // open shx file as well
   char *shx = new char[lstrlen(filename) + 1];
   lstrcpy(shx, filename);
   char *pindex = strrchr(shx, '.');
   pindex++;
   lstrcpy(pindex, _T("shx"));

   FILE *fpi;
   fopen_s(&fpi, shx, "wb");

   delete[] shx;

   if (fpi == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -2;
      }

   //   return 1;

      // get file type
   int fileType = -1;

   switch (GetType())
      {
      case LT_LINE:
         fileType = 3;
         break;

      case LT_POLYGON:
         fileType = 5;
         break;

      case LT_POINT:
         fileType = 1;
      }

   if (fileType < 0)
      {
      Report::ErrorMsg("Unsupported file type - only 2D and 3D polygon, point and line types are saveable...");
      return -1;
      }

   if (m_3D)
      fileType += 10;

   // get file size
   int fileSize = 50;  // for header

   int recordsToWrite = 0;
   if (selectedPolysOnly)
      recordsToWrite = GetSelectionCount();
   else
      {
      if (saveDefunctPolysFlag == 1)
         recordsToWrite = GetRecordCount(MapLayer::ALL);
      else
         recordsToWrite = GetRecordCount(MapLayer::ACTIVE);
      }

   fileSize += 4 * recordsToWrite;    // record number + content length

   int count = selectedPolysOnly ? GetSelectionCount() : GetRecordCount(MapLayer::ALL);
   int records = count;
   int recordsWritten = 0;

   for (int i = 0; i < count; i++)
      {
      int index = selectedPolysOnly ? GetSelection(i) : i;

      Poly *pPoly = GetPolygon(index);

      // empty vertex?
      if (pPoly->GetVertexCount() == 0)
         {
#ifndef NO_MFC
         if (saveEmptyPolysFlag == 0) // ask?
            {
            int retVal = AfxMessageBox(_T("An empty shape was encountered during save - do you want to remove empty shapes?"), MB_YESNO);
            saveEmptyPolysFlag = (retVal == IDYES) ? 2 : 1;
            }
#endif
         if (saveEmptyPolysFlag == 2)   // don't save
            {
            fileSize -= 4;
            records--;
            continue;
            }
         }

      switch (GetType())
         {
         case LT_POLYGON:
         case LT_LINE:
            {
            // is this a polygon we want to save (not defunct)
            if (saveDefunctPolysFlag == 1 || !IsDefunct(index))
               {
               fileSize += 2;    // shape type
               fileSize += 16;    // bounding box
               fileSize += 4;    // number of part/points

               int numParts = pPoly->m_vertexPartArray.GetSize();
               fileSize += 2 * numParts;    // 2 for part   

               int numPoints = pPoly->GetVertexCount();
               fileSize += 8 * numPoints;

               if (m_3D)
                  {
                  fileSize += 8;    // z range
                  fileSize += 4 * numPoints;  // z values

                  fileSize += 8;    // M range
                  fileSize = 4 * numPoints;
                  }
               }
            else
               {
               fileSize -= 4;
               records--;
               }
            }
            break;

         case LT_POINT:
            fileSize += 2;    // shape type
            fileSize += 8;    // x,y coords

            if (m_3D)
               fileSize += 8;    // z,m

            break;
         }
      }  // end of:  for ( i < numRecords )

   // have file size, start writing file   
   WriteShapeHeader(fp, fileSize, fileType);
   WriteShapeHeader(fpi, 50 + 4 * records, fileType);

   // file header finished, now start writing record information
   fseek(fp, 100, SEEK_SET);
   fseek(fpi, 100, SEEK_SET);

   int offset = 50;   // this is the offset of the record in the shp file (in 16-bit words), stored in the index file

   for (int i = 0; i < count; i++)
      {
      //---- record header ---------//
      int index = selectedPolysOnly ? GetSelection(i) : i;

      // is this a polygon we want to save (not defunct)
      if (saveDefunctPolysFlag == 1 || !IsDefunct(index))
         {
         Poly *pPoly = GetPolygon(index);

         // empty vertex?
         if (pPoly->GetVertexCount() == 0 && saveEmptyPolysFlag == 2)  // note: 0=ask,1=save,2=don't save
            continue;

         ++recordsWritten;

         // first, do record header
         PutBigDWORD(i + 1, fp);             // record number

         // get content length of this record (16-bit words)
         int contentLength = 2;  // shape type (bytes)

         switch (GetType())
            {
            case LT_POLYGON:
            case LT_LINE:
               contentLength += 16;    // bounding box
               contentLength += 4;     // num parts + num Points
               contentLength += pPoly->m_vertexPartArray.GetSize() * 2;    // parts array
               contentLength += pPoly->GetVertexCount() * 8;               // points array

               if (m_3D)
                  {
                  contentLength += 8;  // z range
                  contentLength += 4 * pPoly->GetVertexCount();   // each z value
                  contentLength += 8;  // m range
                  contentLength += 4 * pPoly->GetVertexCount();   // each m value
                  }
               break;

            case LT_POINT:
               contentLength += m_3D ? 16 : 8;   // pts are 2 doubles = 8 words (4 words each)
               break;
            }

         PutBigDWORD(contentLength, fp);   // content length (number of 16-bit words)

         PutBigDWORD(offset, fpi);         // write index info as well
         PutBigDWORD(contentLength, fpi);

         offset += (2 + 2 + contentLength);   // update offset for next go around - include recordNumber + contentLength + content

         //------- now, do the record contents --------------
         // type
         PutLittleDWORD(fileType, fp);

         switch (GetType())
            {
            case LT_POLYGON:
            case LT_LINE:
               {
               // bounding box
               REAL xMin, yMin, xMax, yMax;
               pPoly->GetBoundingRect(xMin, yMin, xMax, yMax);
               PutLittleDouble(xMin, fp);
               PutLittleDouble(yMin, fp);
               PutLittleDouble(xMax, fp);
               PutLittleDouble(yMax, fp);

               // number of parts
               int numParts = pPoly->m_vertexPartArray.GetSize();
               PutLittleDWORD(numParts, fp);

               int numPoints = pPoly->GetVertexCount();
               PutLittleDWORD(numPoints, fp);

               for (int k = 0; k < numParts; k++)
                  PutLittleDWORD(pPoly->m_vertexPartArray[k], fp);

               for (int k = 0; k < numPoints; k++)
                  {
                  const Vertex &vertex = pPoly->GetVertex(k);
                  PutLittleDouble(vertex.x, fp);
                  PutLittleDouble(vertex.y, fp);
                  }

               // done with 2D shapes.  For 3D shapes, add the following
               if (m_3D)
                  {
                  // z range
                  float zMin, zMax;
                  pPoly->GetBoundingZ(zMin, zMax);
                  PutLittleDouble(zMin, fp);
                  PutLittleDouble(zMax, fp);

                  // z Array
                  for (int k = 0; k < numPoints; k++)
                     PutLittleDouble(pPoly->GetVertex(k).z, fp);

                  // M range
                  PutLittleDouble(0.0, fp);
                  PutLittleDouble(0.0, fp);

                  // M Array
                  for (int k = 0; k < numPoints; k++)
                     PutLittleDouble(0.0, fp);
                  }  // end of:  if ( m_3d )
               }
               break;   // end of:  case LT_POLY, LT_LINE

            case LT_POINT:
               PutLittleDouble(pPoly->GetVertex(0).x, fp);
               PutLittleDouble(pPoly->GetVertex(0).y, fp);
               if (m_3D)
                  {
                  PutLittleDouble(pPoly->GetVertex(0).z, fp);
                  PutLittleDouble(0.0, fp);     // measure
                  }
               break;
            }
         }  // end of: if ( not defunct )
      }  // end of:  for ( i < numRecords )

   fclose(fp);
   fclose(fpi);
   ASSERT(recordsWritten == recordsToWrite);

   // get corresponding dbf file
   char *dbf = new char[lstrlen(filename) + 1];
   lstrcpy(dbf, filename);
   char *p = strrchr(dbf, '.');
   p++;
   lstrcpy(p, _T("dbf"));

   // get path of shape file
   p = strrchr(dbf, '\\');

   BitArray *pSaveRecordsArray = NULL;

   if (saveDefunctPolysFlag == 2 || selectedPolysOnly) // don't save defunct records
      {
      int records = GetRecordCount(MapLayer::ALL);
      pSaveRecordsArray = new BitArray(records, selectedPolysOnly ? false : true);

      // take care of defunct first
      if (saveDefunctPolysFlag == 2)
         for (int k = 0; k < records; k++)
            pSaveRecordsArray->Set(k, this->IsDefunct(k) ? false : true);

      if (selectedPolysOnly)
         {
         int queryCount = this->GetSelectionCount();
         for (int k = 0; k < queryCount; k++)
            {
            int idu = this->GetSelection(k);
            pSaveRecordsArray->Set(idu, true);
            }
         }
      }

   //if ( p == NULL )  // no path
#if !defined(_WIN64) && !defined(NO_MFC)
   SaveDataDB(filename, pSaveRecordsArray);
#else
   m_pDbTable->SaveDataDBF(filename, pSaveRecordsArray);
#endif
   //else
   //   {
   //   p++;

   //   char dbfilename[ 128 ];
   //   lstrcpy( dbfilename, p );
   //   *p = NULL;
   //   SaveDataDB( dbfilename, pSaveRecordsArray );
   //   }

   delete dbf;

   if (pSaveRecordsArray)
      delete pSaveRecordsArray;

   // if projection is defined, save it as well
   char *prj = new char[lstrlen(filename) + 4];
   lstrcpy(prj, filename);
   pindex = strrchr(prj, '.');
   pindex++;
   lstrcpy(pindex, _T("prj"));

   FILE *fpp;
   fopen_s(&fpp, prj, "wt");

   if (fpp)
      {
      fputs((LPCTSTR) this->m_projection, fpp);
      fclose(fpp);
      }

   delete[] prj;


   /*
   // write corresponding index file

   // get corresponding shx file
   char *shx = new char[ lstrlen( filename ) + 1 ];
   lstrcpy( shx, filename );
   char *pindex = strrchr( shx, '.' );
   pindex++;
   lstrcpy( pindex, _T( "shx" ) );

   FILE *fpi;
   fopen_s( &fpi, shx, "wb" );

   delete [] shx;

   fseek( fpi, 0, SEEK_SET );

   if ( fpi == NULL )
      {
      CString msg( "Unable to find file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return -1;
      }

//   return 1;

   // get file size
   fileSize = 50;  // This is the file size of the header, not the main file
   //count = selectedPolysOnly ? GetSelectionCount() : GetRecordCount( MapLayer::ALL );
   fileSize += recordsToWrite * 4;  //  The total size of the index file is 50 (for the header) + 4 * the number of records

   PutBigDWORD( 9994, fpi );      // file code
   PutBigDWORD( 0, fpi );         // 20 bytes of padding
   PutBigDWORD( 0, fpi );
   PutBigDWORD( 0, fpi );
   PutBigDWORD( 0, fpi );
   PutBigDWORD( 0, fpi );

   PutBigDWORD( fileSize, fpi );     // file length (in 16-bit words)
   PutLittleDWORD( 1000, fpi );      // version

   PutLittleDWORD( fileType, fpi );

   PutLittleDouble( m_vxMin, fpi );      // xmin
   PutLittleDouble( m_vyMin, fpi );      // ymin
   PutLittleDouble( m_vxMax, fpi );      // xmax
   PutLittleDouble( m_vyMax, fpi );      // ymax
   PutLittleDouble( m_vzMin, fpi );      // zmin
   PutLittleDouble( m_vzMax, fpi );      // zmax
   PutLittleDouble( 0.0, fpi );          // mmin
   PutLittleDouble( 0.0, fpi );          // mmax

   fseek( fpi, 100, SEEK_SET );

   // file header finished, now start writing record information

   int offset = 50;

   PutBigDWORD( offset, fpi );  //  There are 50 16 bit words in the first record (the header) so...

   int numPolys = selectedPolysOnly ? GetSelectionCount() : GetPolygonCount();

   for ( int i=0; i < numPolys; i++ )
      {
      int index = selectedPolysOnly ? GetSelection( i ) : i;

      if ( saveDefunctPolysFlag == 1 || ! IsDefunct( offset ) )
         {
         Poly *pPoly = GetPolygon( index );

         // empty vertex?
         if ( pPoly->GetVertexCount() == 0 && saveEmptyPolysFlag == 2 )   // don't save
            continue;

         int contentLength = 2;   // shape type

         switch( GetType() )
            {
            case LT_POLYGON:
            case LT_LINE:
               {
               // get content length of this record (16-bit words)
               contentLength += 16;    // bounding box
               contentLength += 4;     // num parts + num Points
               contentLength += pPoly->m_vertexPartArray.GetSize() * 2;    // parts array
               contentLength += pPoly->GetVertexCount() * 8;               // points array

               if ( m_3D )
                  {
                  contentLength += 8;  // z range
                  contentLength += 4 * pPoly->GetVertexCount();   // each z value
                  contentLength += 8;  // m range
                  contentLength += 4 * pPoly->GetVertexCount();   // each m value
                  }
               break;

            case LT_POINT:
               contentLength += m_3D ? 16 : 8;   // pts are 2 doubles = 8 words (4 words each)
               break;
            }


         offset += contentLength + 4;
         PutBigDWORD( contentLength, fpi );   // content length

         PutBigDWORD( offset, fpi );
         }

      } //end of record count, index file is finished...

   fclose( fpi );
   */
   //return fileSize;
   return recordsWritten;
   }



bool MapLayer::WriteShapeHeader(FILE *fp, int fileSize, int fileType)
   {
   fseek(fp, 0, SEEK_SET);
   PutBigDWORD(9994, fp);      // file code

   PutBigDWORD(0, fp);         // 20 bytes of padding
   PutBigDWORD(0, fp);
   PutBigDWORD(0, fp);
   PutBigDWORD(0, fp);
   PutBigDWORD(0, fp);

   PutBigDWORD(fileSize, fp);     // file length (in 16-bit words)
   PutLittleDWORD(1000, fp);      // version

   PutLittleDWORD(fileType, fp);

   PutLittleDouble(m_vxMin, fp);      // xmin
   PutLittleDouble(m_vyMin, fp);      // ymin
   PutLittleDouble(m_vxMax, fp);      // xmax
   PutLittleDouble(m_vyMax, fp);      // ymax
   PutLittleDouble(m_vzMin, fp);      // zmin
   PutLittleDouble(m_vzMax, fp);      // zmax
   PutLittleDouble(0.0, fp);          // mmin
   PutLittleDouble(0.0, fp);          // mmax
   return true;
   }


//-- MapLayer::SaveGridFile() -------------------------------------
//
//-- save a grid layer in ArcInfo-exported ascii grid file format
//-------------------------------------------------------------------

bool MapLayer::SaveGridFile(LPCTSTR filename) //, int maxLineLength )
   {
   FILE *fp;
   fopen_s(&fp, filename, "wt");

   if (fp == NULL)
      return false;

   char buffer[128];

   //-- set header information -----------------------
   //
   //-- NOTE: Header looks like: (starts in col 0)
   //
   //   NCOLS 200
   //   NROWS 120
   //   XLLCORNER 60
   //   YLLCORNER -10
   //   CELLSIZE 0.5
   //   NODATA_VALUE -99.9
   //-------------------------------------------------
   int cols, rows;
   GetDataDim(cols, rows);
   COORD2d ll;
   GetLowerLeft(ll);

   REAL width, height;
   GetCellSize(width, height);

   sprintf_s(buffer, 128, "NCOLS %d\n", cols);
   fputs(buffer, fp);

   sprintf_s(buffer, 128, "NROWS %d\n", rows);
   fputs(buffer, fp);

   sprintf_s(buffer, 128, "XLLCORNER %d\n", (int)ll.x);
   fputs(buffer, fp);

   sprintf_s(buffer, 128, "YLLCORNER %d\n", (int)ll.y);
   fputs(buffer, fp);

   sprintf_s(buffer, 128, "CELLSIZE %f\n", width);  // assumes square cells
   fputs(buffer, fp);

   sprintf_s(buffer, 128, "NODATA_VALUE %f\n", GetNoDataValue());
   fputs(buffer, fp);

   //-- have header, proceed with grid data --//
   for (int row = 0; row < rows; row++)
      {
      for (int col = 0; col < cols; col++)
         {
         CString str(m_pData->GetAsString(col, row));
         fputs((LPCTSTR)str, fp);

         if (col != cols - 1)
            fputs(", ", fp);
         else
            fputc('\n', fp);
         }  // end of:  for ( col < nCols )
      }  // end of:  for ( row < nRows )

   fclose(fp);

   return true;
   }



void MapLayer::SetGridToNoData(void)
   {
   ASSERT(m_layerType == LT_GRID);

   int rows = GetRecordCount();
   int cols = GetFieldCount();

   //-- have header, proceed with grid data --//
   for (int row = 0; row < rows; row++)
      for (int col = 0; col < cols; col++)
         m_pData->Set(col, row, m_noDataValue);

   //SetBins( m_noDataValue, m_noDataValue, 1 );
   ClassifyData();

   return;
   }


DataObj *MapLayer::CreateDataTable(int rows, int cols, DO_TYPE type /*= DOT_VDATA*/)
   {
   if (m_pDbTable != NULL)
      delete m_pDbTable;

   //ASSERT(m_pDbTable == NULL);
   //ASSERT(m_pData == NULL);

   // allocate data object
   switch (type)
      {
      case DOT_INT:
         m_pData = new IDataObj(cols, rows, U_UNDEFINED);
         break;

      case DOT_FLOAT:
         m_pData = new FDataObj(cols, rows, U_UNDEFINED);
         break;

      default:
         m_pData = new VDataObj(cols, rows, U_UNDEFINED);
         break;
      }

   float noDataValue = this->GetNoDataValue();

   for (int row = 0; row < rows; row++)
      for (int col = 0; col < cols; col++)
         {
         if (type == DOT_INT)
            m_pData->Set(col, row, (int)noDataValue);
         else // if ( type == DOT_FLOAT )
            m_pData->Set(col, row, noDataValue);
         }

   // attach data to a dbtable object - creates field info as well
   m_pDbTable = new DbTable(m_pData);

   m_binArrayArray.Init(cols);

   return m_pData;
   }


void MapLayer::SetDataDim(int nCols, int nRows)
   {
   ASSERT(m_layerType == LT_GRID);
   m_pData->SetSize((INT_PTR)nCols, (INT_PTR)nRows);
   UpdateExtents();

   m_pMap->UpdateMapExtents();
   }


int MapLayer::AddPolygon(Poly *pPoly, bool updateExtents /*=true*/)
   {
   if (m_layerType != LT_POLYGON && m_layerType != LT_POINT)
      return -1;

   m_pPolyArray->Add(pPoly);
   pPoly->SetLabelPoint(this->m_labelMethod);
   if (updateExtents)
      UpdateExtents(true);

   m_polyBinArray.Add(NULL);

   return (int)m_pPolyArray->GetSize();
   }


bool MapLayer::RemovePolygon(Poly *pPoly, bool updateExtents /*=true*/)
   {
   if (m_layerType != LT_POLYGON && m_layerType != LT_POINT)
      return false;

   int index = m_pPolyArray->GetPolyIndex(pPoly);

   if (index < 0)
      return false;

   MakePolyDefunct(index);

   if (updateExtents)
      UpdateExtents(true);

   return true;
   }


int MapLayer::AddPoint(REAL x, REAL y)
   {
   if (m_layerType != LT_POINT)
      return -1;

   Poly *pPoly = new Poly;
   pPoly->AddVertex(Vertex(x, y));
   pPoly->InitLogicalPoints(m_pMap);
   pPoly->SetLabelPoint(this->m_labelMethod);

   // adjust layer extents if necessary
   if (x < m_vxMin) m_vxMin = x;
   if (x > m_vxMax) m_vxMax = x;
   if (y < m_vyMin) m_vyMin = y;
   if (y > m_vyMax) m_vyMax = y;

   if ((m_vxMax - m_vxMin) > (m_vyMax - m_vyMin))
      m_vExtent = m_vxMax - m_vxMin;
   else
      m_vExtent = m_vyMax - m_vyMin;

   // set extents for the point
   const Vertex &v = pPoly->m_vertexArray[0];
   pPoly->m_xMin = v.x;
   pPoly->m_xMax = v.x;
   pPoly->m_yMin = v.y;
   pPoly->m_yMax = v.y;

   m_polyBinArray.Add(NULL);
   int index = m_pPolyArray->Add(pPoly);

   if (this->m_pData != NULL)
      {
      int fieldCount = this->GetFieldCount();
      VData *data = new VData[fieldCount];
      this->m_pData->AppendRow(data, fieldCount);
      delete[] data;
      }

   return index;
   }


bool MapLayer::GetPointCoords(int index, REAL &x, REAL &y) const
   {
   if (m_layerType != LT_POINT)
      return false;

   if (index >= m_pPolyArray->GetSize())
      return false;

   Poly *pPoly = GetPolygon(index);

   x = pPoly->GetVertex(0).x;
   y = pPoly->GetVertex(0).y;

   return true;
   }


/*
MapLayer *MapLayer::Polygonize(LPCTSTR __bins)
   {
   // create polygons from a raster.  Polygons are defined 
   // by contiguous cell that satisfy a particulr constraint
   // e.g "0:0.1-1.0;1:1.0-2.0"

   struct _BIN {
      int polyVal;
      float minVal;
      float maxVal;
      };

   CStringArray tokens;
   ::Tokenize(__bins, ";", tokens);

   CArray< _BIN, _BIN > bins;
   bins.SetSize(tokens.GetCount());

   for (int i = 0; i < tokens.GetCount(); i++)
      {
      int pos = 0;
      bins[i].polyVal = atoi((LPCTSTR)tokens[i].Tokenize(":", pos));

      bins[i].minVal = atof((LPCTSTR)tokens[i].Tokenize("-", pos));
      bins[i].maxVal = atof((LPCTSTR)tokens[i].Tokenize("-", pos));
      }


   // have to start with a raster
   if (m_layerType != LT_GRID)
      return NULL;

   // make a matrix to hold polyids
   int rows = this->m_pData->GetRowCount();
   int cols = this->m_pData->GetColCount();

   IntMatrix polyIDGrid(rows, cols, -1);

   int currentPolyID = 0;
   int currentBin = -1;

   // idea is to move across the grid looking for undefined grid cells.
   // when found, expand to neighbors that match a particular 
   // constraint, tagging each with the ID of the associated polygon.
   // Keep track of which polygon matches each constraint class.

   for (int row = 0; row < rows; row++)
      {
      for (int col = 0; col < cols; col++)
         {
         if (polyIDGrid(row, col) == -1)  // haven't seen this one yet, process it
            {
            // classify cell;
            float value = 0;
            this->GetData(row, col, value);

            if (value == this->m_noDataValue)
               {
               polyIDGrid(row, col) = this->m_noDataValue;
               continue;
               }

            currentBin = -1;
            for (int i = 0; i < bins.GetSize(); i++)
               {
               if (value >= bins[i].minVal && value < bins[i].maxVal)
                  {
                  currentBin = i;
                  break;
                  }
               }
            if (currentBin < 0)
               polyIDGrid(row, col) = this->m_noDataValue;
            else
               {
               _MarkPoly(polyIDGrid, row, col, bins[currentBin].minVal, bins[currentBin].maxVal, bins[currentBin].polyVal);
               Poly *pPoly = _BuildPolyFromGrid(polyIDGrid, row, col, bins[currentBin].polyVal);
               }
            }  // end of: cell == -1
         }  // end of: for col
      }  // end of: for row

         // poly IDs in raster generate polygons.  Generate actual polygons.


   }

int MapLayer::_MarkPoly(IntMatrix &polyIDGrid, int row, int col, float minVal, float maxVal, int polyVal)
   {
   int count = 0;

   // check out of bounds conditions
   if (row < 0 || row >= polyIDGrid.GetRowCount())
      return 0;

   if (col < 0 || col >= polyIDGrid.GetColCount())
      return 0;

   float value = 0;
   this->GetData(row, col + 1, value);

   if (value < minVal || value >= maxVal)
      return 0;

   // tag current cell with poly index
   polyIDGrid(row, col) = polyVal;
   count++;

   // try to expand (recursively) in four directions

   // to the right
   float value = 0;
   if (polyIDGrid(row, col + 1) < 0)
      count += _MarkPoly(polyIDGrid, row, col + 1, minVal, maxVal, polyVal);

   // down
   if (polyIDGrid(row + 1, col) < 0)
      count += _MarkPoly(polyIDGrid, row + 1, col, minVal, maxVal, polyVal);

   // to the left
   if (polyIDGrid(row, col - 1) < 0)
      count += _MarkPoly(polyIDGrid, row, col - 1, minVal, maxVal, polyVal);

   // up
   if (polyIDGrid(row - 1, col) < 0)
      count += _MarkPoly(polyIDGrid, row - 1, col, minVal, maxVal, polyVal);

   return count;
   }


Poly *MapLayer::_BuildPolyFromGrid(IntMatrix &polyIDGrid, int row, int col, int polyVal)
   {
   // starting at the specified row, col, first look 

   // to the right
   //float value = 0;
   //if (polyIDGrid(row, col + 1) < 0)
   //   count += _MarkPoly(polyIDGrid, row, col + 1, minVal, maxVal, polyVal);
   //
   //// down
   //if (polyIDGrid(row + 1, col) < 0)
   //   count += _MarkPoly(polyIDGrid, row + 1, col, minVal, maxVal, polyVal);
   //
   //// to the left
   //if (polyIDGrid(row, col - 1) < 0)
   //   count += _MarkPoly(polyIDGrid, row, col - 1, minVal, maxVal, polyVal);
   //
   //// up
   //if (polyIDGrid(row - 1, col) < 0)
   //   count += _MarkPoly(polyIDGrid, row - 1, col, minVal, maxVal, polyVal);


   return NULL;
   }
   */


bool MapLayer::ConvertToLine( LPCTSTR fields, LPCTSTR binSpec)
   {
   if (this->m_layerType == LT_LINE)
      return true;

   if (this->m_layerType != LT_POINT)
      return false;

   CStringArray _fields;
   ::Tokenize(fields, ";,", _fields);

   int col = this->GetFieldCol(_fields[0]);  // first field used for 
   if (col < 0)
      return false;

   //  binSpec: e.g "0:0.1-1.0;1:1.0-2.0"

   CUIntArray polyData;

   // decode and set up bin structure

   struct _BIN {
      int polyVal;
      float minVal;
      float maxVal;
      };

   class _BinArray : public CArray< _BIN, _BIN > {};

   _BinArray bins;

   if (binSpec && lstrlen(binSpec) > 0)
      {
      CStringArray tokens;
      ::Tokenize(binSpec, ";,", tokens);

      bins.SetSize(tokens.GetCount());

      for (int i = 0; i < tokens.GetCount(); i++)
         {
         int pos = 0;
         bins[i].polyVal = atoi((LPCTSTR)tokens[i].Tokenize(":", pos));

         bins[i].minVal = atof((LPCTSTR)tokens[i].Tokenize("-", pos));
         bins[i].maxVal = atof((LPCTSTR)tokens[i].Tokenize("-", pos));
         }
      }
   else   // no spec, assume an int
      {
      CUIntArray values;
      this->GetUniqueValues(col, values);

      bins.SetSize(values.GetCount());

      for (int i = 0; i < values.GetCount(); i++)
         {
         int pos = 0;
         bins[i].polyVal = values[i];

         bins[i].minVal = values[i] - 0.5f;
         bins[i].maxVal = values[i] + 0.5f;
         }
      }

   // start at first point, creating polyline while bins match
   
   PolyArray pa;
   Poly *pPoly = NULL;
   int index = 0;
   int lastBin = -2;
   VData data[1];
   int pts = this->GetRowCount();

   // iterate through points
   for (int i=0; i < pts; i++)
      {
      // classify the point
      float value;
      this->GetData(i, col, value);

      int thisBin = -1;
      for (int j = 0; j < bins.GetSize(); j++)
         {
         if (value >= bins[j].minVal && value < bins[j].maxVal)
            {
            thisBin = j;
            break;
            }
         }

      // get the point
      Poly *pThisPoint = this->GetPolygon(i);

      // we have a few options:
      // 1) The last bin matches this bin.  In this case,
      //    add this vertex to the current polyline
      // 2) the bins don't match.  In this case, add the vertext 
      //    to the current polyline, start a new
      //    polyline and add the vertex.

      if ( pPoly )
         pPoly->AddVertex(pThisPoint->GetVertex(0), false);

      if (thisBin != lastBin)  // time to start a new polyline?
         {
         if (pPoly != NULL)  // save the current poly and binned value
            {
            if (lastBin < 0)
               polyData.Add((int)this->m_noDataValue);
            else
               polyData.Add(bins[lastBin].polyVal);
            pa.Add(pPoly); // add to the polyline array
            }
         
         // start a new poly
         pPoly = new Poly;
         pPoly->AddVertex(pThisPoint->GetVertex(0), false);
         lastBin = thisBin;
         }
     

      // last one?
      if (i == pts-1)
         {
         if (pPoly != NULL)
            {
            if ( thisBin < 0 )
               polyData.Add((int)this->m_noDataValue);
            else
               polyData.Add(bins[thisBin].polyVal);

            pa.Add(pPoly);
            }
         }
      }
   
   // at this point, all polys created and stored
   // copy to MapLayer
   ASSERT(pa.GetCount() == polyData.GetCount());
   this->ClearPolygons();
   for (int i = 0; i < pa.GetCount(); i++)
      {
      Poly *pPoly = pa[i];
      if (pPoly->GetVertexCount() > 1)
         this->AddPolygon(pPoly);
      else
         delete pPoly;
      }

   // add data
   this->m_pDbTable->RemoveAllFields();
   this->m_pDbTable->m_pData->Clear();
   this->m_pDbTable->AddField(_fields[0], TYPE_INT);
   VData d[1];
   for (int i = 0; i < pa.GetCount(); i++)
      {
      Poly *pPoly = pa[i];
      if (pPoly->GetVertexCount() > 1)
         {
         d[0] = polyData[i];
         m_pDbTable->m_pData->AppendRow(d, 1);
         }
      }

   this->m_layerType = LT_LINE;

   return true;
   }


bool  MapLayer::GetDataDim(int &cols, int &rows) const
   {
   if (m_pData == NULL)
      return false;

   cols = m_pData->GetColCount();
   rows = m_pData->GetRowCount();
   return true;
   }


void  MapLayer::SetLowerLeft(COORD2d ll)
   {
   m_vxMin = ll.x;
   m_vyMin = ll.y;

   // data loaded, clean up
   m_vxMax = m_vxMin + m_pData->GetColCount() * m_cellWidth;
   m_vyMax = m_vyMin + m_pData->GetRowCount() * m_cellHeight;

   if (m_pMap->GetLayerCount() == 1)
      {
      m_pMap->SetMapExtents(m_vxMin, m_vxMax, m_vyMin, m_vyMax);
      //m_pMap->ZoomFull( false );  // no redraw
      }
   }

int MapLayer::LoadDataDBF(LPCTSTR databaseName, int extraCols /* =0 */, int records /*=-1*/, CUIntArray *pColsToLoad /*=NULL*/)
   {
   DBFHandle h = DBFOpen(databaseName, "rb");  // jpb - removed '+'

   if (h == NULL)
      {
      CString msg(_T("Error opening database "));
      msg += databaseName;
      msg += _T("... it may not be present, you may not have access, it may be locked, or may be read only");
      Report::ErrorMsg(msg, _T("File Error"));
      return -1;
      }

   int cols = DBFGetFieldCount(h);

   if (pColsToLoad)
      cols = pColsToLoad->GetSize();

   if (m_pData != NULL)
      delete m_pData;

   m_pData = new VDataObj(U_UNDEFINED);  // by dimension (must Append())
   m_pData->SetSize(INT_PTR(cols + extraCols), 0);
   m_pDbTable = new DbTable(m_pData); // make a corresponding DbTable and attach to this data object
   //m_pDbTable->SetFieldInfoSize( cols + extraCols  );

   // set field structure
   for (int col = 0; col < cols; col++)
      {
      char fname[16];
      fname[0] = NULL;

      int width, decimals;
      DBFFieldType ftype = DBFGetFieldInfo(h, col, fname, &width, &decimals);

      if (lstrlen(fname) == 0 || fname[0] == ' ')
         lstrcpy(fname, "_UNNAMED_");

      m_pData->AddLabel(fname);
      FIELD_INFO &fi = m_pDbTable->GetFieldInfo(col);
      fi.show = true;

      switch (ftype)
         {
         case FTLogical:   fi.type = TYPE_BOOL;    break;
         case FTInteger:   fi.type = TYPE_LONG;    break;
         case FTDouble:    fi.type = TYPE_DOUBLE; break;
         case FTString:    fi.type = TYPE_DSTRING; break;
            //case dbDate   Date/Time; see MFC class COleDateTime
            //dbLongBinary   Long Binary (OLE Object); you might want to use MFC class CByteArray instead of class CLongBinary as CByteArray is richer and easier to use.
            //dbMemo   Memo; see MFC class CString
         default:
            Report::WarningMsg("Unsupported type in MapLayer::LoadDataDBF()");
         }

      fi.width = width;
      fi.decimals = decimals;
      }

   // add extra fields
   for (int i = 0; i < extraCols; i++)
      {
      CString label;
      label.Format("EXTRA_%i", i + 1);

      m_pData->AddLabel((LPCTSTR)label);
      int width, decimals;
      GetTypeParams(TYPE_LONG, width, decimals);
      m_pDbTable->SetFieldType(cols + i, TYPE_LONG, width, decimals, false);  // make long by default
      m_pDbTable->GetFieldInfo(cols + i).save = false;   // make saving extra cols off by default
      }

   COleVariant *varArray = new COleVariant[cols + extraCols];

   int count = 0;

   int dbRecordCount = DBFGetRecordCount(h);

   for (int i = 0; i < dbRecordCount; i++)
      {
      //	  if( DBFIsRecordDeleted(h) )
      //		continue;

      if (records > 0 && count == records)
         break;

      for (int col = 0; col < cols; col++)
         {
         COleVariant val;

         switch (m_pDbTable->GetFieldType(col))
            {
            case TYPE_BOOL:
               {
               val.ChangeType(VT_BOOL);
               const char *p = DBFReadStringAttribute(h, i, col);
               if (*p == 'T' || *p == 't' || *p == 'Y' || *p == 'y')
                  val.boolVal = (VARIANT_BOOL)1;
               else
                  val.boolVal = (VARIANT_BOOL)0;
               }
               break;

            case TYPE_LONG:      val = (long)DBFReadIntegerAttribute(h, i, col); break;
            case TYPE_SHORT:
            case TYPE_UINT:
            case TYPE_INT:       val = (long)DBFReadIntegerAttribute(h, i, col);   break;
            case TYPE_FLOAT:     val = (float)((double)DBFReadDoubleAttribute(h, i, col)); break;
            case TYPE_DOUBLE:    val = (double)DBFReadDoubleAttribute(h, i, col);  break;
            case TYPE_DSTRING:   val = DBFReadStringAttribute(h, i, col); break;
            default:
               ; //Report::WarningMsg( "Unsupported type in MapLayer::LoadDataDBF()" );
            }

         varArray[col] = val;
         }

      m_pMap->Notify(NT_LOADRECORD, count, 0);
      m_pData->AppendRow(varArray, cols + extraCols);
      //((VDataObj*) m_pData)->AppendRow( array );
      count++;
      }

   delete[] varArray;

   DBFClose(h);

   //delete timer;
   //char b[100];
   //sprintf(b, "LoadDataDBF in: %f\n", timer_seconds);
   //TRACE0(b);
   //timer_seconds = 0;

   return count;
   }


#if !defined(_WIN64) && !defined(NO_MFC)
int MapLayer::LoadDataDB(LPCTSTR databaseName, LPCTSTR connectStr, LPCTSTR sql, int extraCols /* =0 */, int records /*=-1*/)
   {
   // note:  connectStr = "" for Access, "DBASE IV;" for dbase
   int recordCount = 0;

   CDaoDatabase database;

   try
      {
      // open the database ( non-exclusive, read/write )
      database.Open(databaseName, FALSE, FALSE, connectStr);
      recordCount = LoadRecords(database, sql, extraCols, records);
      ASSERT(recordCount >= 0);
      }

   catch (CDaoException *p)
      {
      CString msg("Error opening database ");
      msg += databaseName;
      msg += ":  ";
      msg += p->m_pErrorInfo->m_strDescription;
      MessageBox(NULL, msg, "Exception", MB_OK);
      }

   database.Close();
   m_database = databaseName;

   return recordCount;
   }


int MapLayer::LoadRecords(CDaoDatabase &database, LPCTSTR sql, int extraCols /* =0 */, int records /*=-1*/)
   {
   CDaoRecordset rs(&database);

   if (m_pData)
      delete m_pData;

   //switch( type )
   //   {
   //   case DOT_INT:
   //      m_pData = new IDataObj;
   //      break;

   //   case DOT_FLOAT:
   //      m_pData = new FDataObj;
   //      break;

   //   default:
   m_pData = new VDataObj;
   //      break;
   //   }

   try
      {
      rs.Open(dbOpenSnapshot, sql);
      rs.MoveFirst();    // move to first record

      // figure out what the table structure of the query results are.
      // For each float field, create a corresponding data object column
      int fieldCount = rs.GetFieldCount();

      m_pData->SetSize(fieldCount + extraCols, 0);  // make a data column for each field
      m_pDbTable = new DbTable(m_pData); // make a corresponding DbTable and attach to this data object

      // set the labels --//
      for (int i = 0; i < fieldCount; i++)
         {
         CDaoFieldInfo info;
         rs.GetFieldInfo(i, info);

         m_pData->AddLabel((LPCTSTR)info.m_strName);

         switch (info.m_nType)
            {
            case dbBoolean:  m_pDbTable->SetFieldType(i, TYPE_BOOL, false);   break;
            case dbByte:     m_pDbTable->SetFieldType(i, TYPE_CHAR, false);   break;
            case dbLong:     m_pDbTable->SetFieldType(i, TYPE_LONG, false);   break;
            case dbInteger:  m_pDbTable->SetFieldType(i, TYPE_INT, false);    break;
            case dbSingle:   m_pDbTable->SetFieldType(i, TYPE_FLOAT, false);  break;
            case dbDouble:   m_pDbTable->SetFieldType(i, TYPE_DOUBLE, false); break;
            case dbText:     m_pDbTable->SetFieldType(i, TYPE_STRING, false); break;
            case dbDate:     m_pDbTable->SetFieldType(i, TYPE_FLOAT, false); break;
            default:
               {
               CString msg;
               msg.Format("Unsupported type in MapLayer::LoadRecords() reading file %s, field %s", (LPCTSTR)m_path, (LPCTSTR)info.m_strName);
               Report::WarningMsg(msg);
               }
            }

         m_pDbTable->ShowField(i, true);
         //m_attribInfoArray[ i ].show = true;
         }

      // add extra fields
      for (int i = 0; i < extraCols; i++)
         {
         CString label;
         label.Format("EXTRA_%i", i + 1);

         m_pData->AddLabel((LPCTSTR)label);
         m_pDbTable->SetFieldType(fieldCount + i, TYPE_LONG, false);  // make long by default
         m_pDbTable->GetFieldInfo(fieldCount + i).save = false;   // make saving extra cols off by default
         }


      // have basic structure, now populate fields
      COleVariant *varArray = new COleVariant[fieldCount + extraCols];
      int count = 0;

      while (rs.IsEOF() == FALSE)
         {
         for (int i = 0; i < fieldCount; i++)
            {
            try
               {
               rs.GetFieldValue(i, varArray[i]);
               }

            catch (CDaoException*)
               {
               CString msg("DAO Exception getting field value.");
               MessageBox(NULL, msg, "Exception", MB_OK);
               return false;
               }
            catch (CMemoryException*)
               {
               CString msg("DAO Memory Exception getting field value.");
               MessageBox(NULL, msg, "Exception", MB_OK);
               return false;
               }
            }  // end of for:

         for (int i = 0; i < extraCols; i++)
            varArray[fieldCount + i] = 0L;

         ((VDataObj*)m_pData)->AppendRow(varArray, fieldCount + extraCols);
         rs.MoveNext();
         count++;
         m_pMap->Notify(NT_LOADRECORD, count, 0);

         if (count == records)
            break;
         }  // end of: while( rs.IsEOF() )

      delete[] varArray;
      rs.Close();
      }  // end of: try ( rsTables.Open )

   catch (CDaoException *p)
      {
      CString msg("Database Error opening table: SQL=");
      msg += sql;
      msg += ":  ";
      msg += p->m_pErrorInfo->m_strDescription;

      MessageBox(NULL, msg, "Exception", MB_OK);
      return -1;
      }
   catch (CMemoryException*)
      {
      CString msg("Memory Error opening table: SQL=");
      msg += sql;
      MessageBox(NULL, msg, "Exception", MB_OK);
      return -2;
      }

   return m_pData->GetRowCount();
   }


int MapLayer::SaveDataColDB(int col, LPCTSTR databaseName, LPCTSTR tableName, LPCTSTR connectStr) const
   {
   ASSERT(m_pData != NULL);
   ASSERT(col < m_pData->GetColCount());

   // note:  connectStr = "" for Access, "DBASE IV;" for dbase
   CDaoDatabase database;

   try
      {
      // open the database ( non-exclusive, read/write )
      database.Open(databaseName, FALSE, FALSE, connectStr);

      // get a recordset corresponding to the table
      CDaoTableDef td(&database);

      try {
         td.Open(tableName);
         }
      catch (CDaoException*)
         {
         CString msg("Database Error opening table ");
         msg += tableName;
         MessageBox(NULL, msg, "Exception", MB_OK);
         return -2;
         }

      // have the table defition, open the table into a recordset
      CDaoRecordset rs(&database);

      try
         {
         rs.Open(&td);

         // start a transaction
         database.m_pWorkspace->BeginTrans();
         int recordCount = m_pData->GetRowCount();
         COleVariant value;

         rs.MoveFirst();
         for (int row = 0; row < recordCount; row++)
            {
            rs.Edit();
            m_pData->Get(col, row, value);
            rs.SetFieldValue(col, value);
            rs.Update();
            rs.MoveNext();
            }

         database.m_pWorkspace->CommitTrans();
         rs.Close();
         }  // end of: try ( rs.Open )

      catch (CDaoException*)
         {
         CString msg("Database Error opening/writing table: ");
         msg += tableName;
         MessageBox(NULL, msg, "Exception", MB_OK);
         return -3;
         }
      td.Close();
      }

   catch (CDaoException*)
      {
      CString msg("Database Error opening database: ");
      msg += databaseName;
      MessageBox(NULL, msg, "Exception", MB_OK);
      return -1;
      }

   catch (CMemoryException*)
      {
      CString msg("Memory Error opening database: ");
      msg += databaseName;
      MessageBox(NULL, msg, "Exception", MB_OK);
      return -2;
      }

   database.Close();
   return m_pData->GetRowCount();
   }

int MapLayer::SaveDataDB() const
   {
   //CString    m_database;   // full path for database file (no filename for DBase)
   //CString    m_tableName;  // table name (file name for DBase)
   return SaveDataDB(m_database, m_tableName, "DBASE IV;", false);
   }


int MapLayer::SaveDataDB(LPCTSTR databaseName, LPCTSTR tableName, LPCTSTR connectStr, bool /*selectedPolysOnly*/) const
   {
   ASSERT(m_pData != NULL);

   if (lstrcmpi(connectStr, "DBase IV;") == 0)
      {
      m_pDbTable->SaveDataDBF(databaseName);
      return m_pData->GetRowCount();
      }

   // note:  connectStr = "" for Access, "DBASE IV;" for dbase
   CDaoDatabase database;

   try
      {
      // open the database ( non-exclusive, read/write )
      database.Create(databaseName);

      //database.Open( databaseName, FALSE, FALSE, connectStr );
      bool ok = CreateTable(database, tableName, connectStr);

      if (ok)
         SaveRecords(database, tableName);
      }

   catch (CDaoException *e)
      {
      CString msg("Error creating database ");
      msg += databaseName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
      MessageBox(NULL, msg, "Exception", MB_OK);
      }

   database.Close();
   return m_pData->GetRowCount();
   }


// helper function
bool MapLayer::CreateTable(CDaoDatabase &database, LPCTSTR tableName, LPCTSTR connectStr) const
   {
   // delete the specified table from the database
   //char *p = (char*) tableName;
   char p[16];
   lstrcpy(p, "cell");
   /*
   try
      {
      database.DeleteTableDef( "cell.dbf" );  // tableName
      }

   catch( CDaoException *e )
      {
      CString msg("Database Error deleting table " );
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
      MessageBox( NULL,  msg, "Warning", MB_OK );
      }
*/
// current table deleted, create a new one corresponding to the data object
   CDaoTableDef td(&database);
   try
      {
      td.Create(tableName, 0, NULL, connectStr);
      }

   catch (CDaoException *e)
      {
      CString msg("Database Error creating table ");
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
      MessageBox(NULL, msg, "Exception", MB_OK);
      return false;
      }

   // table created, start adding fields
   int rows = m_pData->GetRowCount();

   FILE *fp;
   fopen_s(&fp, "fields.txt", "wt");

   for (int i = 0; i < m_pData->GetColCount(); i++)
      {
      LPCTSTR fieldName = m_pData->GetLabel(i);
      fprintf(fp, "%s\n", fieldName);

      short type = -1;
      //int   fieldType = m_pData->GetType( i );

      TYPE fieldType = m_pDbTable->GetFieldType(i);

      switch (fieldType)
         {
         case TYPE_SHORT:
         case TYPE_INT:
         case TYPE_UINT:
         case TYPE_LONG:
         case TYPE_ULONG:     type = dbLong;      break;
         case TYPE_FLOAT:     type = dbSingle;    break;
         case TYPE_DOUBLE:    type = dbDouble;    break;
         case TYPE_STRING:
         case TYPE_DSTRING:   type = dbText;      break;
         case TYPE_BOOL:      type = dbBoolean;   break;  // Boolean; True=-1, False=0.
         default:
            {
            CString msg;
            msg.Format("Unsupported field type at col %d", i);
            Report::ErrorMsg(msg);
            continue;
            }
         }

      int size = 0;
      if (type == dbText)
         {
         // make sure it will hold the longest text
         for (int j = 0; j < rows; j++)
            {
            CString val = m_pData->GetAsString(i, j);
            if (val.GetLength() > size)
               size = val.GetLength();
            }

         size += 10; // for good measure
         if (size > 255)
            size = 255;
         }

      try
         {
         td.CreateField(fieldName, type, size, dbUpdatableField | dbFixedField);
         }

      catch (CDaoException *e)
         {
         CString msg("Database Error creating field definition ");
         msg += tableName;
         msg += ", ";
         msg += fieldName;
         msg += ": ";
         msg += e->m_pErrorInfo->m_strDescription;
         MessageBox(NULL, msg, "Exception", MB_OK);
         continue;
         }
      }  // end of field creation

   try
      {
      td.Append();      // add the table to the database table collection
      }

   catch (CDaoException *e)
      {
      CString msg("Database Error appending table :  ");
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
      MessageBox(NULL, msg, "Exception", MB_OK);
      }

   fclose(fp);
   return true;
   }



int MapLayer::SaveRecords(CDaoDatabase &database, LPCTSTR tableName) const
   {
   CDaoTableDef td(&database);

   try {
      td.Open(tableName);
      }
   catch (CDaoException *e)
      {
      CString msg("Database Error opening table ");
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;
      MessageBox(NULL, msg, "Exception", MB_OK);
      return -1;
      }

   // have the table defition, open the table into a recordset
   CDaoRecordset rs(&database);

   try
      {
      rs.Open(&td, dbOpenTable, dbAppendOnly);

      // start a transaction
      //database.m_pWorkspace->BeginTrans();

      int recordCount = m_pData->GetRowCount();
      COleVariant value;

      for (int row = 0; row < recordCount; row++)
         {
         rs.AddNew();
         //rs.SetBookmark( rs.GetLastModifiedBookmark( ) );
         //rs.MoveLast();

         // add data to the recordset
         int cols = m_pData->GetColCount();
         ASSERT(cols == rs.GetFieldCount());

         for (int col = 0; col < cols; col++)
            {
            m_pData->Get(col, row, value);
            rs.SetFieldValue(col, value);
            }

         rs.Update();
         rs.MoveLast();
         }  // end of: for ( row < recordCount )

      //database.m_pWorkspace->CommitTrans();
      rs.Close();
      }  // end of: try ( rsTables.Open )


   catch (CDaoException *e)
      {
      CString msg("Database Error on : ");
      msg += tableName;
      msg += ": ";
      msg += e->m_pErrorInfo->m_strDescription;

      MessageBox(NULL, msg, "Exception", MB_OK);
      return -1;
      }
   catch (CMemoryException*)
      {
      CString msg("Memory Error opening table: ");
      msg += tableName;
      MessageBox(NULL, msg, "Exception", MB_OK);
      return -2;
      }

   td.Close();

   return m_pData->GetRowCount();
   }


#endif // _WIN64



int MapLayer::SaveDataAscii(LPCTSTR filename, bool includeID, bool includeLabels, LPCTSTR colToWrite) const
   {
   FILE *fp;
   fopen_s(&fp, filename, "wt");

   if (fp == NULL)
      {
      CString msg("Unable to find file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return -1;
      }

   // start writing
   int recordCount = m_pData->GetRowCount();

   if (recordCount != m_pPolyArray->GetSize())
      {
      Report::ErrorMsg("Database record count different that polygon count...Aborting", "Error", MB_OK);
      return -1;
      }

   char *id = new char[64]; // max size...
   int colID = -1;

   if (colToWrite != NULL)
      colID = m_pData->GetCol(colToWrite);

   int colCount = m_pData->GetColCount();

   for (int i = 0; i < recordCount; i++)
      {
      m_pMap->Notify(NT_WRITEDATA, i, 0);

      if (includeID)
         {
         _itoa_s(m_pPolyArray->GetAt(i)->m_id, id, 64, 10);
         fputs(id, fp);
         fputs(", ", fp);
         }

      if (colToWrite == NULL)
         {
         if (includeLabels && i == 0)
            {
            for (int k = 0; k < colCount; k++)
               {
               LPCTSTR str = m_pData->GetLabel(k);
               {
               fputs(str, fp);
               fputs(",", fp);
               }
               }
            fputc('\n', fp);
            }

         for (int j = 0; j < colCount; j++)
            {
            CString data = m_pData->GetAsString(j, i);   // col, row
            fputs(data, fp);

            if (j != colCount - 1)
               fputs(",", fp);
            else
               fputc('\n', fp);
            }
         }
      else  // col == NULL
         {
         if (includeLabels && i == 0)
            {
            LPCTSTR str = m_pData->GetLabel(colID);
            fputs(str, fp);
            fputs(",", fp);
            fputc('\n', fp);
            }
            
         CString data = m_pData->GetAsString(colID, i);   // col, row
         fputs(data, fp);
         fputc('\n', fp);
         }
      }  // end of: for ( i < recordCount ) 

   delete id;
   fclose(fp);

   m_pMap->Notify(NT_WRITEDATA, -recordCount, 0);

   return recordCount;
   }


void MapLayer::SetActiveField(int col)
   {
   if (col < 0 || col >= GetColCount())
      return;

   m_activeField = col;
   }



BinArray *MapLayer::GetBinArray(int col, bool add)
   {
   if (col == USE_ACTIVE_COL)
      col = m_activeField;

   if (col < 0)
      return NULL;

   if (col >= m_binArrayArray.GetCount())
      {
      //  ASSERT( 0 );
      return NULL;
      }

   if (m_binArrayArray.GetData() == NULL)
      m_binArrayArray.Init(m_pDbTable->GetColCount());

   BinArray *pBinArray = m_binArrayArray.GetAt(col);
   if (pBinArray == NULL && add)
      {
      pBinArray = new BinArray;
      m_binArrayArray.SetAt(col, pBinArray);
      }

   return pBinArray;
   }

const BinArray *MapLayer::GetBinArray(int col) const
   {
   if (col == USE_ACTIVE_COL)
      col = m_activeField;

   if (col < 0 || col >= m_binArrayArray.GetCount() || m_binArrayArray.GetData() == NULL)
      {
      //  ASSERT( 0 );
      return NULL;
      }

   return m_binArrayArray.GetAt(col);
   }


void MapLayer::InitPolyLogicalPoints(Map *pMap /*=NULL*/)
   {
   if (pMap == NULL)
      pMap = m_pMap;

   ASSERT(pMap != NULL);

   for (int i = 0; i < m_pPolyArray->GetSize(); i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);
      pPoly->InitLogicalPoints(pMap);

      if (pPoly->GetVertexCount() > 0)
         pPoly->SetLabelPoint(m_labelMethod);
      }
   }
int MapLayer::InitData(int cols, int rows, float value)
   {
   ASSERT(m_layerType != LT_GRID);  // Note:  this code assume a dataobj of type float - needs to be updated if ever called

   if (m_pDbTable)    // clear out any existing data
      delete m_pDbTable;

   COleVariant *data = new COleVariant[cols];  // create temp array of COleVariants

   for (int i = 0; i < cols; i++)    // load up column info
      {
      data[i].vt = VT_R4;
      data[i].fltVal = value;
      }

   m_pData = new VDataObj(cols, 0, U_UNDEFINED);   // create a new data object to store data

   for (int i = 0; i < rows; i++)
      m_pData->AppendRow(data, cols);       // add as many rows as needecd

   m_pDbTable = new DbTable(m_pData);

   delete[] data;

   return rows;
   }


void MapLayer::ClearPolygons()
   {
   for (int i = 0; i < m_pPolyArray->GetSize(); i++)
      delete m_pPolyArray->GetAt(i);

   m_pPolyArray->RemoveAll();

   m_deadCount = 0;
   m_defunctPolyList.SetAll(false);
   }



void MapLayer::ClearData()
   {
   if (m_pDbTable != NULL)
      delete m_pDbTable;

   m_pDbTable = NULL;
   m_pData = NULL;

   m_binArrayArray.RemoveAll();
   }



void MapLayer::ClearRows(void)
   {
   if (m_pDbTable == NULL)
      return;

   if (m_pData == NULL)
      return;

   m_pData->ClearRows();   // preserves colums
   }


int MapLayer::GetBinCount(int col)
   {
   if (col < 0)
      col = this->m_activeField;

   if (col >= GetFieldCount())
      return -1;

   // verify bins
   int baaSize = (int)m_binArrayArray.GetSize();
   if (col >= baaSize)
      {
      int binsToAdd = GetFieldCount() - baaSize;
      for (int i = 0; i < binsToAdd; i++)
         {
         BinArray *pBinArray = new BinArray;
         m_binArrayArray.Add(pBinArray);
         }
      }

   // verified continue looking for binarray
   BinArray *pBinArray = GetBinArray(col, false);

   if (pBinArray == NULL)
      return 0;
   else
      return pBinArray->GetCount();
   }


// general purpose method - figures out the best bin structure based on available information
void MapLayer::SetBins(int col, BCFLAG bcFlag /*=BCF_UNDEFINED*/, int maxBins /*=-1*/)
   {
   if (col < 0)
      col = GetActiveField();

   if (col < 0)
      return;

   MAP_FIELD_INFO *pInfo = FindFieldInfo(GetFieldLabel(col));

   if (pInfo)
      {
      this->SetBins(pInfo);
      return;
      }

   TYPE type = this->GetFieldType(col);
   switch (type)
      {
      case TYPE_BOOL:
      case TYPE_CHAR:
      case TYPE_STRING:
      case TYPE_DSTRING:
         {
         WAIT_CURSOR;

         if (bcFlag == BCF_UNDEFINED)
            bcFlag = BCF_MIXED;
         SetBinColorFlag(bcFlag);
         SetUniqueBins(col, type);
         }
         break;

      case TYPE_SHORT:
      case TYPE_INT:
      case TYPE_LONG:
      case TYPE_ULONG:
      case TYPE_UINT:
         {
         WAIT_CURSOR;

         if (bcFlag == BCF_UNDEFINED)
            bcFlag = BCF_GRAY_INCR;
         SetBinColorFlag(bcFlag);
         SetUniqueBins(col, type);
         }
         break;

      case TYPE_FLOAT:
      case TYPE_DOUBLE:
         {
         WAIT_CURSOR;

         if (bcFlag == BCF_UNDEFINED)
            bcFlag = BCF_GRAY_INCR;
         SetBinColorFlag(bcFlag);
         float minVal, maxVal;
         GetDataMinMax(col, &minVal, &maxVal);
         if (maxBins < 0)
            maxBins = 10;
         SetBins(col, minVal, maxVal, maxBins, type, BSF_LINEAR);
         }
         break;

      default:
         ASSERT(0);
      }  // end of:  switch( pInfo->type )
   }


void MapLayer::SetBins(MAP_FIELD_INFO *pInfo)
   {
   int col = GetFieldCol(pInfo->fieldname);
   if (col < 0)
      return;

   // any bins?
   BinArray *pBinArray = GetBinArray(col, true);
   pBinArray->RemoveAll();

   WAIT_CURSOR;

   pBinArray->m_pFieldInfo = pInfo;
   SetBinColorFlag((BCFLAG)pInfo->displayFlags);
   int attrCount = pInfo->attributes.GetCount();

   switch (pInfo->type)
      {
      case TYPE_CHAR:
      case TYPE_STRING:
      case TYPE_DSTRING:
         {
         if (attrCount > 0)
            {
            for (int i = 0; i < attrCount; i++)
               {
               FIELD_ATTR &a = pInfo->GetAttribute(i);
               Bin *pBin = this->AddBin(col, a.color, a.value.GetAsString(), a.label);
               pBin->m_pFieldAttr = &a;
               if (a.transparency > 0)
                  {
                  pBin->SetTransparency(a.transparency);
                  pBinArray->m_hasTransparentBins = true;
                  }
               }
            if (this->m_useNoDataBin)
               AddNoDataBin(col);
            }
         else
            SetUniqueBins(col);
         }
         break;

      case TYPE_BOOL:
      case TYPE_INT:
      case TYPE_SHORT:
      case TYPE_LONG:
      case TYPE_ULONG:
      case TYPE_UINT:
         if (attrCount <= 0)
            SetUniqueBins(col);
         else
            {
            for (int i = 0; i < attrCount; i++)
               {
               FIELD_ATTR &a = pInfo->GetAttribute(i);
               if (pInfo->mfiType == MFIT_CATEGORYBINS)
                  {
                  int value;
                  a.value.GetAsInt(value);
                  a.minVal = value;
                  a.maxVal = value;
                  }

               Bin *pBin = this->AddBin(col, a.color, a.label, pInfo->type, a.minVal, a.maxVal);
               pBin->m_pFieldAttr = &a;
               if (a.transparency > 0)
                  {
                  pBin->SetTransparency(a.transparency);
                  pBinArray->m_hasTransparentBins = true;
                  }
               }

            if (this->m_useNoDataBin)
               AddNoDataBin(col);
            }
         break;

      case TYPE_FLOAT:
      case TYPE_DOUBLE:
         {
         if (attrCount > 0)
            {
            for (int i = 0; i < attrCount; i++)
               {
               FIELD_ATTR &a = pInfo->GetAttribute(i);
               Bin *pBin = this->AddBin(col, a.color, a.label, pInfo->type, a.minVal, a.maxVal);
               pBin->m_pFieldAttr = &a;
               if (a.transparency > 0)
                  {
                  pBin->SetTransparency(a.transparency);
                  pBinArray->m_hasTransparentBins = true;
                  }
               }

            if (this->m_useNoDataBin)
               AddNoDataBin(col);
            }
         else
            {
            int binCount = pInfo->GetAttributeCount();

            if (pInfo->binMax > pInfo->binMin && binCount > 0)   // bin min/max values set?
               SetBins(col, pInfo->binMin, pInfo->binMax, binCount, pInfo->type, (BSFLAG)pInfo->bsFlag);
            else
               {
               float minVal, maxVal;
               GetDataMinMax(col, &minVal, &maxVal);

               int count = 10;
               if (binCount > 0)
                  count = binCount;

               SetBins(col, minVal, maxVal, count, pInfo->type, (BSFLAG)pInfo->bsFlag);
               }
            }
         }
         break;
      }  // end of:  switch( pInfo->type )
   }


int CompareInt(const void *elem0, const void *elem1);

void MapLayer::SetBins(int col, float binMin, float binMax, int binCount, TYPE type, BSFLAG bsFlag, BinArray *pExtBinArray, LPCTSTR units)
   {
   if (col == USE_ACTIVE_COL)
      col = m_activeField;

   BinArray *pBinArray = NULL;

   if (pExtBinArray == NULL)
      pBinArray = GetBinArray(col, true);
   else
      pBinArray = pExtBinArray;

   //if (pBinArray == NULL)
   //   pBinArray = new BinArray;

   //-- resize the existing array to the new size.  Note:  If the old
   //   BinArray is shorter than the new array, extra elements will not
   //   be pointing to anything in particular.
   pBinArray->SetSize(binCount);
   pBinArray->m_type = TYPE_FLOAT;

   pBinArray->m_binMin = binMin;
   pBinArray->m_binMax = binMax;

   switch (bsFlag)
      {
      case BSF_LINEAR:
         {
         //-- okay, new bin array set up, now build up values and labels --//
         float binWidth = (binMax - binMin) / binCount;

         for (int i = 0; i < binCount; i++)
            {
            Bin &bin = pBinArray->ElementAt(i);
            bin.m_minVal = binMin + (i * binWidth);
            bin.m_maxVal = binMin + ((i + 1) * binWidth);

            if (i > 0 && ::IsInteger(type))
               bin.m_minVal += 1;

            if (i == binCount - 1) // last one?
               bin.m_maxVal *= 1000;

            bin.m_popSize = 0;
            }
         }
         break;

      case BSF_EQUALCOUNTS:
         {
         int recordCount = GetRecordCount();
         int countPerBin = recordCount / binCount;

         int *indexArray = new int[recordCount];
         int index = 0;
         for (Iterator i = Begin(); i != End(); i++)     // default iterates through ACTIVE bins
            {
            ASSERT(index < recordCount);
            indexArray[index++] = i;
            }
         //ASSERT( index == recordCount );

         pMapLayer = this;
         m_ascending = true;

         // sort index array
         qsort((void*)indexArray, recordCount, sizeof(int), Compare);

         // indexArray is now sorted, use it to figure out bin boundaries
         TYPE type = GetFieldType(m_activeField);
         ASSERT(type == TYPE_FLOAT || type == TYPE_INT || type == TYPE_SHORT || type == TYPE_UINT || type == TYPE_DOUBLE || type == TYPE_LONG);

         // Adjust recordCount and countPerBin to ignore the NoDataValues at the end.
         if (type == TYPE_FLOAT || type == TYPE_DOUBLE)
            {
            VData vValue;
            float fValue;
            bool ok;
            float noDataValue = pMapLayer->GetNoDataValue();
            int activeField = pMapLayer->GetActiveField();

            pMapLayer->GetData(indexArray[recordCount - 1], activeField, vValue);
            ok = vValue.GetAsFloat(fValue);
            ASSERT(ok);

            if (fValue == noDataValue) // else there are no NoDataValues
               {
               int i;
               int high = recordCount;
               for (int low = -1; high - low > 1; )
                  {
                  i = (high + low) / 2;

                  pMapLayer->GetData(indexArray[i], activeField, vValue);
                  ok = vValue.GetAsFloat(fValue);
                  ASSERT(ok);

                  if (fValue == noDataValue)
                     high = i;
                  else
                     low = i;
                  }

               ASSERT(0 <= high && high < recordCount);
               pMapLayer->GetData(indexArray[high], activeField, vValue);
               ok = vValue.GetAsFloat(fValue);
               ASSERT(ok);

               ASSERT(fValue == noDataValue);
               recordCount = high;
               countPerBin = recordCount / binCount;
               }
            }

         for (int i = 0; i < binCount; i++)
            {
            Bin &bin = pBinArray->ElementAt(i);

            VData v0, v1;
            int index0 = i*countPerBin;
            int index1;
            if (i < binCount - 1)
               index1 = ((i + 1)*countPerBin) - 1;
            else  // last bin
               index1 = recordCount - 1;

            pMapLayer->GetData(indexArray[index0], m_activeField, v0);
            pMapLayer->GetData(indexArray[index1], m_activeField, v1);

            switch (type)
               {
               case TYPE_FLOAT:
                  bin.m_minVal = v0.val.vFloat;
                  bin.m_maxVal = v1.val.vFloat;
                  break;

               case TYPE_DOUBLE:
                  bin.m_minVal = (float)v0.val.vDouble;
                  bin.m_maxVal = (float)v1.val.vDouble;
                  break;

               case TYPE_LONG:
                  bin.m_minVal = (float)v0.val.vLong;
                  bin.m_maxVal = (float)v1.val.vLong;
                  break;

               case TYPE_INT:
                  bin.m_minVal = (float)v0.val.vInt;
                  bin.m_maxVal = (float)v1.val.vInt;
                  break;

               case TYPE_UINT:
                  bin.m_minVal = (float)v0.val.vUInt;
                  bin.m_maxVal = (float)v1.val.vUInt;
                  break;
               }

            bin.m_popSize = 0;
            }

         delete[] indexArray;
         }  // end of: case ( EQUAL_COUNTS )
         break;
      }  // end of switch

   ClassifyData();

   for (int i = 0; i < binCount; i++)      // set labels (except last nodata bin)
      {
      Bin &bin = pBinArray->ElementAt(i);
      BPOS binPos;
      if (i == 0)          binPos = BP_FIRST;
      else if (i == binCount - 1) binPos = BP_LAST;
      else                        binPos = BP_MID;

      bin.SetDefaultLabel(TYPE_FLOAT, m_legendFormat, binPos, m_showBinCount, m_showSingleValue, units);

      if (m_binColorFlag == BCF_SINGLECOLOR)
         bin.SetColor(m_fillColor);
      else
         {
         COLORREF color = GetColor(i, binCount, m_binColorFlag);
         bin.SetColor(color);
         }
      }

   if (this->m_useNoDataBin)
      AddNoDataBin(col);

   return;
   }



int CompareInt(const void *elem0, const void *elem1)
   {
   int index0 = *((int*)elem0);
   int index1 = *((int*)elem1);

   int activeField = pMapLayer->GetActiveField();

   VData v0, v1;
   pMapLayer->GetData(index0, activeField, v0);
   pMapLayer->GetData(index1, activeField, v1);

   float f0, f1;
   v0.GetAsFloat(f0);
   v1.GetAsFloat(f1);

   return (f0 < f1) ? -1 : 1;
   /*
      int activeField = pMapLayer->GetActiveField();

      // get field for the indexes
      TYPE type = pMapLayer->GetFieldType( activeField );

      ASSERT( type == TYPE_FLOAT || type == TYPE_INT || type == TYPE_DOUBLE || type == TYPE_LONG );

      VData v0, v1;
      pMapLayer->GetData( index0, activeField, v0 );
      pMapLayer->GetData( index1, activeField, v1 );
      int compare = 0;

      switch( type )
         {
         case TYPE_FLOAT:
            compare = ( v0.val.vFloat < v1.val.vFloat ) ? -1 : 1;
            break;

         case TYPE_DOUBLE:
            compare = ( v0.val.vDouble < v1.val.vDouble ) ? -1 : 1;
            break;

         case TYPE_LONG:
            compare = v0.val.vLong - v1.val.vLong;
            break;

         case TYPE_INT:
            compare = v0.val.vInt - v1.val.vInt;
            break;

         case TYPE_UINT:
            compare = v0.val.vUInt - v1.val.vUInt;
            break;

         default:
            ASSERT( 0 );
            break;

         // case VT_?? string
         // compare = _stricmp( * ( char** ) arg1, * ( char** ) arg2 );
         // break;
         }

      if ( pMapLayer->m_ascending == false )
         compare = -compare;

      return compare;  */
   }



void MapLayer::SetBins(int col, CStringArray &valueArray, BinArray *pExtBinArray/*=NULL*/, TYPE type/*=TYPE_STRING*/, LPCTSTR /*units=NULL*/)
   {
   BinArray *pBinArray = NULL;

   if (pExtBinArray == NULL)
      pBinArray = GetBinArray(col, true);
   else
      pBinArray = pExtBinArray;

   //-- resize the existing array to the new size.  Note:  If the old
   //   BinArray is shorter than the new array, extra elements will not
   //   be pointing to anything in particular.
   int binCount = valueArray.GetSize(); // add one for no data
   pBinArray->SetSize(binCount);
   pBinArray->m_type = type;  //BT_STRING;
   pBinArray->m_binMin = 0.0f;
   pBinArray->m_binMax = 0.0f;
   pBinArray->m_hasTransparentBins = false;

   // check the type of the active field.  If it is numeric, or layer is a grid, maintain numeric info
   if (::IsNumeric(type) || m_layerType == LT_GRID)  //??? need to check active field for numeric
      {
      pBinArray->m_binMin = (float)LONG_MAX;
      pBinArray->m_binMax = (float)LONG_MIN;

      for (int i = 0; i < binCount; i++)
         {
         float value = (float)atof(valueArray[i]);

         if (value < pBinArray->m_binMin)
            pBinArray->m_binMin = value;

         if (value > pBinArray->m_binMax)
            pBinArray->m_binMax = value;
         }
      }

   for (int i = 0; i < binCount; i++)
      {
      Bin &bin = pBinArray->ElementAt(i);
      bin.m_minVal = 0.0f;
      bin.m_maxVal = 0.0f;
      bin.m_strVal = valueArray[i];
      bin.m_popSize = 0;

      if (::IsNumeric(type) || m_layerType == LT_GRID)  //??? need to check active field for numeric
         {
         float value = (float)atof(valueArray[i]);
         bin.m_minVal = value - 0.00001f;
         bin.m_maxVal = value + 0.00001f;
         }
      }

   ClassifyData();

   for (int i = 0; i < binCount; i++)
      {
      Bin &bin = pBinArray->ElementAt(i);
      BPOS binPos;
      if (i == 0)          binPos = BP_FIRST;
      else if (i == binCount - 1) binPos = BP_LAST;
      else                        binPos = BP_MID;

      bin.SetDefaultLabel(TYPE_STRING, m_legendFormat, binPos, m_showBinCount, m_showSingleValue);

      if (m_binColorFlag == BCF_SINGLECOLOR)
         bin.SetColor(m_fillColor);
      else
         bin.SetColor(GetColor(i, binCount, m_binColorFlag));
      }

   if (this->m_useNoDataBin)
      AddNoDataBin(col);

   return;
   }


void MapLayer::AddNoDataBin(int col)
   {
   Bin bin;
   bin.m_minVal = m_noDataValue;
   bin.m_maxVal = m_noDataValue;
   bin.m_popSize = 0;
   bin.m_color = -1; //RGB( 250, 250, 250 );    // white
   bin.m_label = "No Data";
   bin.m_transparency = 100;
   BinArray *pBinArray = GetBinArray(col, true);

   pBinArray->Add(bin);
   }



int MapLayer::SetUniqueBins(int col, TYPE type, BinArray *pExtBinArray /*=NULL*/, LPCTSTR units /*=NULL*/)
   {
   LAYER_TYPE layerType = GetType();

   if (layerType == LT_GRID)
      col = -1;

   CStringArray values;
   GetUniqueValues(col, values, 300);

   if (layerType == LT_GRID)
      type = TYPE_FLOAT;

   SetBins(col, values, pExtBinArray, type, units);

   if (type != TYPE_STRING)
      SortBins(col, false);

   return values.GetSize() + 1;
   }


TYPE fBinType;
bool fIncreasingOrder;
int SortBinProc(const void *elem1, const void *elem2);


void MapLayer::SortBins(int col, bool increasingOrder)
   {
   int binCount = GetBinCount(col);

   if (binCount <= 0)
      return;

   BinArray *pBinArray = GetBinArray(col, false);
   if (pBinArray == NULL)
      return;

   // have bins, now sort them...
   //const Bin** GetData( ) const
   Bin **newBinArray = new Bin*[binCount];   // an array of pointers to bins

   for (int i = 0; i < binCount; i++)
      newBinArray[i] = &pBinArray->GetAt(i);

   fIncreasingOrder = increasingOrder;
   fBinType = pBinArray->m_type;

   // sort the pointers
   qsort((void*)newBinArray, binCount - 1, sizeof(Bin*), SortBinProc);  // binCount-1, because the nodata bin will be the last one, not sorted.

   // copy into new array
   BinArray tempBinArray;
   for (int i = 0; i < binCount; i++)
      tempBinArray.Add(*(newBinArray[i]));  // make a copy of the binArray

   pBinArray->RemoveAll();

   for (int i = 0; i < binCount; i++)
      pBinArray->Add(tempBinArray[i]);

   delete[] newBinArray;
   }


int SortBinProc(const void *elem1, const void *elem2)
   {
   Bin *pBin0 = *((Bin**)elem1);
   Bin *pBin1 = *((Bin**)elem2);

   if (::IsNumeric(fBinType))
      {
      float dif = pBin0->m_minVal - pBin1->m_minVal;

      if (fIncreasingOrder)
         dif = -dif;

      if (dif > 0)
         return 1;
      else
         return -1;
      }
   else if (::IsString(fBinType))
      {
      int result = lstrcmpi(pBin0->m_strVal, pBin1->m_strVal);

      if (fIncreasingOrder)
         result = -result;

      return result;
      }

   return 0;
   }


//-- Map::ClassifyData() --------------------------------------------
//
//-- classifies the layers data based on passed in values.  Sets
//   bin pointers for polygons (stored in m_polyBinArray) and generates 
//   population counts for each bin.  Assumes bins have already
//   been specified and create using SetBin() family of methods
//-------------------------------------------------------------------

bool MapLayer::ClassifyData(int col /* default=-1, ignored for grids */, int /* scheme */)
   {
   int colArea = GetFieldCol("AREA");

   if (m_layerType == LT_GRID)
      col = 0;
   else if (col < 0)
      col = GetActiveField();

   if (col < 0)
      return false;

   if (m_pData == NULL)       // no data to work with!
      return false;

   BinArray *pBinArray = GetBinArray(col, false);
   if (pBinArray == NULL)
      {
      InitPolyBins();   // set all to NULL
      return false;
      }

   // set bin counts to zero
   for (int i = 0; i < pBinArray->GetSize(); i++)
      {
      pBinArray->GetAt(i).m_popSize = 0;
      pBinArray->GetAt(i).m_area = 0;
      }

   // for each record (polygon/line/grid), figure out which bin it is in
   Bin *pBin = NULL;

   if (pBinArray->GetSize() == 0)  // no bins?
      {
      this->SetBins(col);    // then automatically set up bins and return
      return false;
      }

   //int colArea = GetFieldCol( "Area" );

   // bins already exist, so classify Polygons
   switch (m_layerType)
      {
      case LT_POLYGON:
      case LT_LINE:
      case LT_POINT:
         {
         TYPE type = GetFieldType(col);

         for (int i = 0; i < m_pPolyArray->GetSize(); i++)
            {
            Poly *pPoly = m_pPolyArray->GetAt(i);    // get the polygon

            bool displayPoly = CheckDisplayThreshold(i);    // display this one?

            VData v;
            m_pData->Get(col, i, v);

            if (displayPoly == false || v.IsNull())
               pBin = NULL;

            else  // displayPoly is true, so get the appropriate bin
               {
               switch (type)
                  {
                  case TYPE_STRING:
                  case TYPE_DSTRING:
                     {
                     CString attribValue = m_pData->GetAsString(col, i);
                     pBin = GetDataBin(col, attribValue);
                     //ASSERT( pBin != NULL );
                     if (pBin)
                        pBin->m_popSize++;
                     }
                     break;

                  case TYPE_FLOAT:
                  case TYPE_DOUBLE:
                     {
                     float attribValue = m_pData->GetAsFloat(col, i);   // assumes polys are ordered consist with dataobj
                     if (m_useNoDataBin == false && attribValue == GetNoDataValue())
                        pBin = NULL;
                     else
                        {
                        pBin = GetDataBin(col, attribValue);
                        //ASSERT( pBin != NULL );
                        if (pBin)
                           pBin->m_popSize++;
                        }
                     }
                     break;

                  case TYPE_UINT:
                  case TYPE_INT:
                  case TYPE_ULONG:
                  case TYPE_LONG:
                  case TYPE_BOOL:
                     {
                     int attribValue = m_pData->GetAsInt(col, i);   // assumes polys are ordered consist with dataobj
                     if (m_useNoDataBin == false && attribValue == GetNoDataValue())
                        pBin = NULL;
                     else
                        {
                        pBin = GetDataBin(col, attribValue);
                        //ASSERT( pBin != NULL );
                        if (pBin)
                           pBin->m_popSize++;
                        }
                     }
                     break;

                  default:
                     Report::ErrorMsg("Invalid Type encountered in ClassifyData()");
                     goto outside_for;

                  }  // end of: switch

               if (pBin && m_layerType == LT_POLYGON)
                  {
                  float area;
                  if (colArea >= 0)
                     GetData(pPoly->m_id, colArea, area);
                  else
                     area = pPoly->GetArea();

                  pBin->m_area += area;
                  }
               }  // end of:  // else   displayPoly == true

            //pPoly->m_pBin = (Bin*) pBin;
            m_polyBinArray[i] = pBin;
            }  // end of: for i < getsize()
      outside_for:
         ;
         }
         break;   // end of: case LT_POLYGON|LT_LINE

      case LT_GRID:
         {
         int rows = m_pData->GetRowCount();
         int cols = m_pData->GetColCount();

         for (int row = 0; row < rows; row++)
            {
            for (int _col = 0; _col < cols; _col++)
               {
               float value;
               bool ok = m_pData->Get(_col, row, value);
               ASSERT(ok);

               if (value != m_noDataValue)
                  {
                  pBin = GetDataBin(_col, value);

                  if (pBin != NULL)     //                  ASSERT( pBin != NULL );
                     pBin->m_popSize++;
                  }
               }  // end of: for ( _col < cols )
            }  // end of: for ( row < rows )
         }
         break;

      default:
         return false;
      }

   return true;
   }


/*
bool MapLayer::ClassifyGridData( int )
   {
   if ( m_pData == NULL )       // no data to work with!
      return false;

   BinArray *pBinArray = GetBinArray( 0, false );
   if ( pBinArray == NULL )
      {
      InitPolyBins();   // set all to NULL
      return false;
      }

   // set bin counts to zero
   for ( int i=0; i < pBinArray->GetSize(); i++ )
      {
      pBinArray->GetAt( i ).m_popSize = 0;
      pBinArray->GetAt( i ).m_area = 0;
      }

   // for each record (polygon/line/grid), figure out which bin it is in
   Bin *pBin = NULL;

   if ( pBinArray->GetSize() == 0 )  // no bins?
      {
      this->SetBins( col );    // then automatically set up bins and return
      return false;
      }

   //int colArea = GetFieldCol( "Area" );

   // bins already exist, so classify Polygons
   switch( m_layerType )
      {
      case LT_POLYGON:
      case LT_LINE:
      case LT_POINT:
         {
         TYPE type = GetFieldType( col );

         for ( int i=0; i < m_pPolyArray->GetSize(); i++ )
            {
            Poly *pPoly = m_pPolyArray->GetAt( i );    // get the polygon

            bool displayPoly = CheckDisplayThreshold( i );    // display this one?

            VData v;
            m_pData->Get( col, i, v );

            if ( displayPoly == false || v.IsNull() )
               pBin = NULL;

            else  // displayPoly is true, so get the appropriate bin
               {
              switch( type )
                  {
                  case TYPE_STRING:
                  case TYPE_DSTRING:
                     {
                     CString attribValue = m_pData->GetAsString( col, i );
                     pBin = GetDataBin( col, attribValue );
                     //ASSERT( pBin != NULL );
                     if ( pBin )
                        pBin->m_popSize++;
                     }
                     break;

                  case TYPE_FLOAT:
                  case TYPE_DOUBLE:
                     {
                     float attribValue = m_pData->GetAsFloat( col, i );   // assumes polys are ordered consist with dataobj
                     if ( m_useNoDataBin == false && attribValue == GetNoDataValue() )
                        pBin = NULL;
                     else
                        {
                        pBin = GetDataBin( col, attribValue );
                        //ASSERT( pBin != NULL );
                        if ( pBin )
                           pBin->m_popSize++;
                        }
                     }
                     break;

                  case TYPE_UINT:
                  case TYPE_INT:
                  case TYPE_ULONG:
                  case TYPE_LONG:
                  case TYPE_BOOL:
                     {
                     int attribValue = m_pData->GetAsInt( col, i );   // assumes polys are ordered consist with dataobj
                     if ( m_useNoDataBin == false && attribValue == GetNoDataValue() )
                        pBin = NULL;
                     else
                        {
                        pBin = GetDataBin( col, attribValue );
                        //ASSERT( pBin != NULL );
                        if ( pBin )
                           pBin->m_popSize++;
                        }
                     }
                     break;

                  default:
                     Report::ErrorMsg( "Invalid Type encountered in ClassifyData()" );
                     goto outside_for;

                  }  // end of: switch

               if ( pBin && m_layerType == LT_POLYGON )
                  {
                  float area;
                  if ( colArea >= 0 )
                     GetData( pPoly->m_id, colArea, area );
                  else
                     area = pPoly->GetArea();

                  pBin->m_area += area;
                  }
               }  // end of:  // else   displayPoly == true

            //pPoly->m_pBin = (Bin*) pBin;
            m_polyBinArray[ i ] = pBin;
            }  // end of: for i < getsize()
outside_for:
            ;
         }
         break;   // end of: case LT_POLYGON|LT_LINE

      case LT_GRID:
         {
         int rows = m_pData->GetRowCount();
         int cols = m_pData->GetColCount();

         for ( int row=0; row < rows; row++ )
            {
            for ( int _col=0; _col < cols; _col++ )
               {
               float value;
               bool ok = m_pData->Get( _col, row, value );
               ASSERT( ok );

               if ( value != m_noDataValue )
                  {
                  pBin = GetDataBin( _col, value );

                  if ( pBin != NULL )     //                  ASSERT( pBin != NULL );
                     pBin->m_popSize++;
                  }
               }  // end of: for ( _col < cols )
            }  // end of: for ( row < rows )
         }
         break;

      default:
         return false;
      }

   return true;
   }
*/


bool MapLayer::ClassifyPoly(int col, int index)
   {
   if (col < 0)
      col = m_activeField;

   ASSERT(index >= 0);
   ASSERT(index < m_pPolyArray->GetSize());

   Poly *pPoly = m_pPolyArray->GetAt(index);    // get the polygon
   ASSERT(pPoly != NULL);

   int colArea = GetFieldCol("Area");
   float area = 0.0f;
   if (colArea >= 0)
      m_pData->Get(colArea, index, area);


   //if ( pPoly->m_pBin != NULL )
   //   {
   //   pPoly->m_pBin->m_popSize--;
   //   pPoly->m_pBin->m_area -= area;
   //   }
   ASSERT(m_polyBinArray.GetSize() == m_pPolyArray->GetSize());

   if (m_polyBinArray[index] != NULL)
      {
      m_polyBinArray[index]->m_popSize--;
      m_polyBinArray[index]->m_area -= area;
      }


   Bin *pBin = NULL;

   switch (GetFieldType(col))
      {
      case TYPE_STRING:
      case TYPE_DSTRING:
         {
         CString attribValue = m_pData->GetAsString(col, index);
         pBin = GetDataBin(col, attribValue);
         }
         break;

      case TYPE_FLOAT:
      case TYPE_DOUBLE:
      case TYPE_INT:
      case TYPE_LONG:
      case TYPE_SHORT:
      case TYPE_UINT:
      case TYPE_ULONG:
         {
         float attribValue = m_pData->GetAsFloat(col, index);   // assumes polys are ordered consist with dataobj

         if (m_useNoDataBin == false && attribValue == GetNoDataValue())
            pBin = NULL;
         else
            pBin = GetDataBin(col, attribValue);
         }
         break;
      }  // end of: switch

   if (pBin != NULL)  //   ASSERT( pBin != NULL );
      {
      pBin->m_popSize++;
      pBin->m_area += area;
      }

   ASSERT(pBin != NULL);

   m_polyBinArray[index] = pBin;
   //pPoly->m_pBin = pBin; 
   return true;

   }


COLORREF GetColor(int count, int maxCount, BCFLAG bcFlag)
   {
   //if ( bcFlag == BCF_SINGLECOLOR ) // managed prior to GetColor calls
   //   return (COLORREF) m_fillColor;

   //if ( bcFlag == BCF_TRANSPARENT )  // covered in Map::DrawMap().
   //   return (COLORREF) -1;

   count = maxCount - count;

   float ratio = (float)count / (float)maxCount;
   long red, green, blue;

   const int darkPrimary = 210;
   const int lightPrimary = 250;
   const int darkSecondary = 0;
   const int lightSecondary = 227;

   switch (bcFlag)
      {
      case BCF_MIXED:
         {
         float oneThird = 1.0f / 3.0f;
         float oneNinth = 1.0f / 9.0f;
         float one27th = 1.0f / 27.0f;

         if (count == 3) // 3 is a pathological case for mixed
            ratio = count / 6.0f;

         red = (long)((fmod(ratio, oneThird) / oneThird) * 255);
         green = (long)((fmod(ratio, oneNinth) / oneNinth) * 255);
         blue = (long)((fmod(ratio, one27th) / one27th) * 255);

         if (red > 250 && green > 250 && blue > 250)
            red = green = blue = lightSecondary;
         }
         break;

      case BCF_RED_INCR:
      case BCF_RED_DECR:
         red = long(darkPrimary + ratio*(lightPrimary - darkPrimary));
         green = blue = long(darkSecondary + ratio * (lightSecondary - darkSecondary));
         if (bcFlag == BCF_RED_DECR)
            {
            red = 255 - red;
            green = 255 - green;
            blue = 255 - blue;
            }
         break;

      case BCF_GREEN_INCR:
      case BCF_GREEN_DECR:
         green = long(darkPrimary + ratio*(lightPrimary - darkPrimary));
         red = blue = long(darkSecondary + ratio * (lightSecondary - darkSecondary));
         if (bcFlag == BCF_GREEN_DECR)
            {
            red = 255 - red;
            green = 255 - green;
            blue = 255 - blue;
            }
         break;

      case BCF_BLUE_INCR:
      case BCF_BLUE_DECR:
         blue = long(darkPrimary + ratio*(lightPrimary - darkPrimary));
         red = green = long(darkSecondary + ratio * (lightSecondary - darkSecondary));
         if (bcFlag == BCF_BLUE_DECR)
            {
            red = 255 - red;
            green = 255 - green;
            blue = 255 - blue;
            }
         break;

      case BCF_REDGREEN:  // bluered  grn->blue, blue->red, red-<grn
         {
         blue = 0;
         int half = maxCount / 2;   // maxCount = 5, half = 2;
         count--;

         if (count < half)   // red?  ramp between 100, 255
            {
            ratio = (float)count / (float)half;
            green = long(darkPrimary + ratio*(lightPrimary - darkPrimary));
            blue = red = long(darkSecondary + ratio * (lightSecondary - darkSecondary));
            }
         else if (count == half)
            return RGB(lightSecondary, lightSecondary, lightSecondary);//-1L;
         else
            {
            ratio = (float)(maxCount - count - 1) / (float)half;
            red = long(darkPrimary + ratio*(lightPrimary - darkPrimary));
            green = blue = long(darkSecondary + ratio * (lightSecondary - darkSecondary));
            }
         }
         break;

      case BCF_BLUEGREEN:
         {
         const float k = 0.5f;
         blue = long(255 / (1 + exp(-k*(count - maxCount / 3))));
         green = long(255 / (1 + exp(-k*((maxCount - count) - maxCount / 3))));
         red = 0;
         }
         break;

      case BCF_BLUERED:
         {
         green = 0;
         int half = maxCount / 2;   // maxCount = 5, half = 2;
         count--;

         if (count < half)   // red?  ramp between 100, 255
            {
            ratio = (float)count / (float)half;
            red = long(darkPrimary + ratio*(lightPrimary - darkPrimary));
            green = blue = long(darkSecondary + ratio * (lightSecondary - darkSecondary));
            }
         else if (count == half)
            return RGB(lightSecondary, lightSecondary, lightSecondary);//-1L;
         else
            {
            ratio = (float)(maxCount - count - 1) / (float)half;
            blue = long(darkPrimary + ratio*(lightPrimary - darkPrimary));
            red = green = long(darkSecondary + ratio * (lightSecondary - darkSecondary));
            }
         }
         break;

      case BCF_GRAY_INCR:
      case BCF_GRAY_DECR:
         int intensity = (int)(ratio * lightSecondary);
         if (bcFlag == BCF_GRAY_INCR)
            red = green = blue = intensity;
         else
            red = green = blue = 255 - intensity;
         break;
      }

   return RGB(red, green, blue);
   }



int MapLayer::Reclass(int col, CArray< VData, VData& > &oldValues, CArray< VData, VData& > &newValues)
   {
   if (oldValues.GetSize() != newValues.GetSize())
      return -1;

   int count = 0;
   for (Iterator i = Begin(); i != End(); i++)
      {
      VData value;
      this->GetData(i, col, value);

      for (INT_PTR j = 0; j < oldValues.GetSize(); j++)
         {
         if (value == oldValues[j])
            {
            this->SetData(i, col, newValues[j]);
            count++;
            break;
            }
         }
      }

   return count;
   }





//-- MapLayer::CheckDisplayThreshold() -----------------------------------
//
//  Checks to see if the value given for the specified record passes the
//  display threshold test.
//
//  returns true  if test is passed, and the poly should be displayed
//          false if test false, and the poly should have a NULL bin
//------------------------------------------------------------------------  

bool MapLayer::CheckDisplayThreshold(int rec)
   {
   // not using thresholds?
   if (m_useDisplayThreshold == 0)
      return true;

   int displayCol;
   bool displayOnlyLarger;

   if (m_useDisplayThreshold > 0)
      {
      displayCol = m_useDisplayThreshold - 1;
      displayOnlyLarger = true;
      }
   else
      {
      displayCol = -m_useDisplayThreshold - 1;
      displayOnlyLarger = false;
      }

   float displayColValue;
   bool ok = GetData(rec, displayCol, displayColValue);
   ASSERT(ok);

   if (displayOnlyLarger)
      return  displayColValue >= m_displayThresholdValue ? true : false;
   else
      return displayColValue <= m_displayThresholdValue ? true : false;
   }



Bin *MapLayer::GetDataBin(int col, VData &value)
   {
   if (col == USE_ACTIVE_COL)
      col = m_activeField;

   BinArray *pBinArray = GetBinArray(col, false);

   if (pBinArray == NULL)
      return NULL;

   int binCount = pBinArray->GetSize();

   if (binCount == 0)
      return NULL;

   if (m_useNoDataBin && value == GetNoDataValue())
      return &(pBinArray->GetAt(pBinArray->GetSize() - 1));     // nodata bin is always last one.

   for (int i = 0; i < binCount - 1; i++)
      {
      Bin *pBin = &(pBinArray->GetAt(i));

      if (::IsNumeric(pBinArray->m_type))
         {
         float _value;
         bool ok = value.GetAsFloat(_value);
         if (ok && (_value >= pBin->m_minVal && _value <= pBin->m_maxVal))
            return pBin;
         }
      else if (::IsString(pBinArray->m_type))
         {
         CString _value;
         bool ok = value.GetAsString(_value);
         if (ok && (_value.Compare(pBin->m_strVal) == 0))
            return pBin;
         }
      else
         ASSERT(0);

      }  // end of:  for ( i < binCount )

   if (m_useNoDataBin)
      return &(pBinArray->GetAt(binCount - 1));
   else
      return &(pBinArray->GetAt(binCount - 2));
   }




Bin *MapLayer::GetDataBin(int col, float value)
   {
   if (col == USE_ACTIVE_COL)
      col = m_activeField;

   BinArray *pBinArray = GetBinArray(col, false);

   if (pBinArray == NULL)
      return NULL;

   int binCount = pBinArray->GetSize();

   if (binCount == 0)
      return NULL;

   if (m_useNoDataBin && value == GetNoDataValue())
      return &(pBinArray->GetAt(pBinArray->GetSize() - 1));     // nodata bin is always last one.

   for (int i = 0; i < binCount - 1; i++)
      {
      Bin *pBin = &(pBinArray->GetAt(i));

      if (::IsNumeric(pBinArray->m_type))
         {
         if (value >= pBin->m_minVal && value <= pBin->m_maxVal)
            return pBin;
         }
      else if (::IsString(pBinArray->m_type))
         {
         CString svalue;
         svalue.Format("%g", value);
         if (svalue.Compare(pBin->m_strVal) == 0)
            return pBin;
         }
      else
         ASSERT(0);
      }  // end of:  for ( i < binCount )

   if (m_useNoDataBin)
      return &(pBinArray->GetAt(binCount - 1));
   else
      return &(pBinArray->GetAt(binCount - 2));
   }


Bin *MapLayer::GetDataBin(int col, int value)
   {
   if (col == USE_ACTIVE_COL)
      col = m_activeField;

   BinArray *pBinArray = GetBinArray(col, false);

   if (pBinArray == NULL)
      return NULL;

   int binCount = pBinArray->GetSize();

   if (binCount == 0)
      return NULL;

   //if ( binCount == 1 )
   //   return &m_pBinArray->GetAt( 0 );

   if (m_useNoDataBin && value == GetNoDataValue())
      return &(pBinArray->GetAt(pBinArray->GetSize() - 1));     // nodata bin is always last one.

   int _binCount = binCount;
   if (m_useNoDataBin)
      _binCount--;

   for (int i = 0; i < _binCount; i++)
      {
      Bin *pBin = &(pBinArray->GetAt(i));

      if (::IsNumeric(pBinArray->m_type))
         {
         if (value >= pBin->m_minVal && value <= pBin->m_maxVal)
            return pBin;
         }
      else if (::IsString(pBinArray->m_type))
         {
         CString svalue;
         svalue.Format("%i", value);
         if (svalue.Compare(pBin->m_strVal) == 0)
            return pBin;
         }
      else
         ASSERT(0);

      }  // end of:  for ( i < binCount )

   // if we get this far, the bin wasn't found.  
   return NULL;
   }


Bin *MapLayer::GetDataBin(int col, LPCTSTR value)
   {
   if (col == USE_ACTIVE_COL)
      col = m_activeField;

   BinArray *pBinArray = GetBinArray(col, false);

   if (pBinArray == NULL)
      return NULL;

   //ASSERT(  ::IsString( pBinArray->m_type ) );

   int binCount = pBinArray->GetSize();

   if (binCount == 0)
      return NULL;

   for (int i = 0; i < binCount - 1; i++)
      {
      Bin *pBin = &pBinArray->GetAt(i);

      if (lstrcmp(value, pBin->m_strVal) == 0)
         return pBin;
      }

   return &pBinArray->GetAt(binCount - 1);
   }


bool MapLayer::GetSortedPolyArray(PolyArray &polyArray, int col, bool ascending)
   {
   int polyCount = m_pPolyArray->GetSize();

   ASSERT(m_pData->GetRowCount() == polyCount);
   ASSERT(col < m_pData->GetColCount());

   // now, create an arra of indexes.  This is what will 
   // actually be sorted.
   int *indexArray = new int[polyCount];

   int oldActiveField = m_activeField;
   m_activeField = col;
   m_ascending = ascending;

   for (int i = 0; i < polyCount; i++)
      indexArray[i] = i;

   // sort the index array
   pMapLayer = this;
   qsort((void*)indexArray, polyCount, sizeof(int), Compare);

   // restore previous active field;
   m_activeField = (int)oldActiveField;

   // resize and load up polyArray
   polyArray.SetSize(polyCount);

   for (int i = 0; i < polyCount; i++)
      polyArray[i] = m_pPolyArray->GetAt(indexArray[i]);

   delete[] indexArray;

   return true;
   }


int Compare(const void *elem0, const void *elem1)
   {
   int index0 = *((int*)elem0);
   int index1 = *((int*)elem1);

   int activeField = pMapLayer->GetActiveField();

   // get field for the indexes
   TYPE type = pMapLayer->GetFieldType(activeField);

   ASSERT(type == TYPE_FLOAT || type == TYPE_INT || type == TYPE_DOUBLE || type == TYPE_LONG);

   VData v0, v1;
   pMapLayer->GetData(index0, activeField, v0);
   pMapLayer->GetData(index1, activeField, v1);
   int compare = 0;

   switch (type)
      {
      case TYPE_FLOAT:
         {
         float noDataValue = pMapLayer->GetNoDataValue();
         if (v0.val.vFloat == noDataValue && v1.val.vFloat == noDataValue)
            compare = 0;
         else if (v0.val.vFloat == noDataValue)
            compare = 1;
         else if (v1.val.vFloat == noDataValue)
            compare = -1;
         else
            compare = (v0.val.vFloat < v1.val.vFloat) ? -1 : 1;
         }
         break;

      case TYPE_DOUBLE:
         {
         double noDataValue = (double)pMapLayer->GetNoDataValue();
         if (v0.val.vDouble == noDataValue && v1.val.vDouble == noDataValue)
            compare = 0;
         else if (v0.val.vDouble == noDataValue)
            compare = 1;
         else if (v1.val.vDouble == noDataValue)
            compare = -1;
         else
            compare = (v0.val.vDouble < v1.val.vDouble) ? -1 : 1;
         }
         break;

      case TYPE_LONG:
         compare = v0.val.vLong - v1.val.vLong;
         break;

      case TYPE_INT:
         compare = v0.val.vInt - v1.val.vInt;
         break;

      case TYPE_UINT:
         compare = v0.val.vUInt - v1.val.vUInt;
         break;

      case TYPE_SHORT:
         compare = v0.val.vShort - v1.val.vShort;
         break;

      default:
         ASSERT(0);
         break;

         // case VT_?? string
         // compare = _stricmp( * ( char** ) arg1, * ( char** ) arg2 );
         // break;
      }

   if (pMapLayer->m_ascending == false)
      compare = -compare;

   return compare;
   }


Poly *MapLayer::FindPoint(int x, int y, int *pIndex)  const  // assumes x, y are device coords
   {
#ifndef NO_MFC
   if (m_layerType != LT_POINT)
      return NULL;

   POINT pt;
   pt.x = x;
   pt.y = y;

   m_pMap->DPtoLP(pt);              // convert to logical

   for (int i = 0; i < m_pPolyArray->GetSize(); i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);

      //if ( IsPointInPoly( pPoly->m_ptArray, pPoly->GetVertexCount(), pt ) )
      POINT &pp = pPoly->m_ptArray[0];

      if (abs(pt.x - pp.x) < 200 && abs(pt.y - pp.y) < 200)
         {
         if (pIndex != NULL)
            *pIndex = i;

         return pPoly;
         }
      }

   if (pIndex != NULL)
      *pIndex = -1;

#endif
   return NULL;
   }


Poly *MapLayer::FindPolygon(int x, int y, int *pIndex) const   // assumes x, y are device coords
   {
#ifndef NO_MFC
   if (m_layerType != LT_POLYGON && m_layerType != LT_LINE && m_layerType != LT_POINT)
      return NULL;

   POINT pt;
   pt.x = x;
   pt.y = y;

   m_pMap->DPtoLP(pt);              // convert to logical

   if (m_layerType == LT_POLYGON)
      {
      //Vertex point;
      //m_pMap->LPtoVP( pt, point.x, point.y );
      for (int i = 0; i < m_pPolyArray->GetSize(); i++)
         {
         // dont test if pPoly is dead
         if (IsDefunct(i))
            continue;

         Poly *pPoly = m_pPolyArray->GetAt(i);

         //if ( IsPointInPoly( pPoly->m_ptArray, pPoly->GetVertexCount(), pt ) )
         //if ( pPoly->IsPointInPoly( point ) )
         if (pPoly->IsPointInPoly(pt))
            {
            if (pIndex != NULL)
               *pIndex = i;
            return pPoly;
            }
         }
      }
   else if (m_layerType == LT_LINE)// m_layerType == LT_LINE
      {
      for (int i = 0; i < m_pPolyArray->GetSize(); i++)
         {
         Poly *pPoly = m_pPolyArray->GetAt(i);

         if (IsPointOnLine(pPoly->m_ptArray, pPoly->GetVertexCount(), pt, 200))
            {
            if (pIndex != NULL)
               *pIndex = i;

            return pPoly;
            }
         }
      }

   else
      {
      return FindPoint(x, y, pIndex);
      }

   if (pIndex != NULL)
      *pIndex = -1;
#endif

   return NULL;
   }


Poly *MapLayer::GetPolygonFromCoord(REAL x, REAL y, int *pIndex /*=NULL*/) const
   {
   Vertex v(x, y);

   for (int i = 0; i < m_pPolyArray->GetSize(); i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);

      if (pPoly->IsPointInPoly(v))
         {
         if (pIndex != NULL)
            *pIndex = i;

         return pPoly;
         }
      }

   if (pIndex != NULL)
      *pIndex = -1;

   return NULL;
   }


Poly *MapLayer::GetPolyInBoundingBox(Vertex vMin, Vertex vMax, int *pIndex /*=NULL*/) const
   {
   // Note: assumes rows are ordered from top down, vcoord system in increasing upward
   ASSERT(m_layerType == LT_LINE);

	REAL cellWidth = this->GetGridCellWidth();
	REAL cellHeight = this->GetGridCellHeight();

   for (int i = 0; i < m_pPolyArray->GetSize(); i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);

      if (IsPolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), vMin.x, vMin.y, vMax.x, vMax.y))
         {
         if (pIndex != NULL)
            *pIndex = i;

         return pPoly;
         }
      }

   if (pIndex != NULL)
      *pIndex = -1;

   return NULL;
   }


Poly *MapLayer::GetPolygonFromCentroid(REAL x, REAL y, float tolerance, int *pIndex)
   {
   for (int i = 0; i < m_pPolyArray->GetSize(); i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);

      Vertex vCentroid = pPoly->GetCentroid();

      if (::DistancePtToPt(x, y, vCentroid.x, vCentroid.y) <= tolerance)
         {
         if (pIndex != NULL)
            *pIndex = i;

         return pPoly;
         }
      }

   if (pIndex != NULL)
      *pIndex = -1;

   return NULL;
   }



Poly* MapLayer::GetDataAtPoint(int x, int y, int col, CString &value) const
   {
   int row;
   Poly *pPoly = FindPolygon(x, y, &row);

   if (pPoly == NULL)
      return NULL;
   else
      {
      if (m_pData == NULL)
         return NULL;
      else
         {
         bool useWide = VData::SetUseWideChar(true);
         value = m_pData->GetAsString(col, row);
         VData::SetUseWideChar(useWide);
         return pPoly;
         }
      }
   }


bool MapLayer::IsFieldCategorical(int col)
   {
   TYPE type = GetFieldType(col);
   if (::IsString(type))
      return true;

   if (type == TYPE_FLOAT || type == TYPE_DOUBLE)
      return false;

   int binCount = this->GetBinCount(col);
   if (binCount == 0)
      return false;

   Bin &bin = this->GetBin(col, 0);

   if (bin.m_minVal == bin.m_maxVal)
      return true;

   return false;
   /*
      LPCTSTR fieldName = GetFieldLabel( col );
      MAP_FIELD_INFO *pInfo = this->FindFieldInfo( fieldName );

      if ( pInfo == NULL )
         return false;

      if( pInfo->GetAttributeCount() == 0 )
         return false;

      FIELD_ATTR &attr = pInfo->GetAttribute( 0 );
      if ( attr.minVal == attr.maxVal )
         return true;
      else
         return false; */
   }

LPCTSTR MapLayer::GetFieldLabel(int col) const
   {
   if (m_pData == NULL)
      return "";

   else
      {
      if (col < 0)
         col = m_activeField;

      LPCTSTR p = NULL;
      if (col < 0)    // no active field, return name of layer
         p = m_name;

      else
         p = m_pData->GetLabel(col);

      if (p)
         return p;
      else
         return "";
      }
   }


void MapLayer::SetFieldLabel(int col, LPCTSTR label)
   {
   if (m_pData == NULL)
      return;

   if (col < 0)
      col = m_activeField;

   m_pData->SetLabel(col, label);
   }



void MapLayer::LoadFieldStrings(CStringArray &strArray)
   {
   strArray.RemoveAll();

   for (int i = 0; i < (int)m_pData->GetColCount(); i++)
      strArray.Add(m_pData->GetLabel(i));
   }


void MapLayer::UpdateExtents(bool initPolyPts /*=true*/)
   {
   switch (m_layerType)
      {
      case LT_POLYGON:
      case LT_LINE:
      case LT_POINT:
         {
         m_vxMin = m_vyMin = (float)LONG_MAX;
         m_vxMax = m_vyMax = (float)LONG_MIN;

         for (int i = 0; i < m_pPolyArray->GetSize(); i++)
            {
            Poly *pPoly = m_pPolyArray->GetAt(i);

            for (int j = 0; j < pPoly->GetVertexCount(); j++)
               {
               const Vertex &v = pPoly->GetVertex(j);

               if (v.x > m_vxMax) m_vxMax = v.x;     // check bounds
               if (v.y > m_vyMax) m_vyMax = v.y;
               if (v.x < m_vxMin) m_vxMin = v.x;
               if (v.y < m_vyMin) m_vyMin = v.y;
               }
            }

         if ((m_vxMax - m_vxMin) > (m_vyMax - m_vyMin))
            m_vExtent = m_vxMax - m_vxMin;
         else
            m_vExtent = m_vyMax - m_vyMin;

         if (initPolyPts)
            InitPolyLogicalPoints();   // create logical points for this set
         }
         break;

      case LT_GRID:
         m_vxMax = m_vxMin + m_pData->GetColCount() * m_cellWidth;
         m_vyMax = m_vyMin + m_pData->GetRowCount() * m_cellHeight;

         if ((m_vxMax - m_vxMin) > (m_vyMax - m_vyMin))
            m_vExtent = m_vxMax - m_vxMin;
         else
            m_vExtent = m_vyMax - m_vyMin;

         break;

      default:
         ASSERT(0);
      }
   }


void MapLayer::ClearAllBins(bool resetPolys /*=false*/)
   {
   m_binArrayArray.RemoveAll();

   if (resetPolys)
      {
      //for ( int i=0; i < m_pPolyArray->GetSize(); i++ )
      //   m_pPolyArray->GetAt( i )->m_pBin = NULL;

      InitPolyBins();
      }
   }


MAP_FIELD_INFO *MapLayer::FindFieldInfo(LPCTSTR fieldname)
   {
   return m_pFieldInfoArray->FindFieldInfo(fieldname);
   }

MAP_FIELD_INFO *MapLayer::FindFieldInfoFromLabel(LPCTSTR label)
   {
   return m_pFieldInfoArray->FindFieldInfoFromLabel(label);
   }



bool MapLayer::GetDataMinMax(int col, float *pMin, float *pMax) const
   {
   ASSERT(m_pData != NULL);

   if (m_layerType == LT_GRID)
      return GetGridDataMinMax(pMin, pMax);

   int cols = m_pData->GetColCount();
   ASSERT(col < cols);

   if (col >= 0)
      return _GetDataMinMax(col, pMin, pMax) ? true : false;

   else
      {
      float minVal, maxVal;
      *pMin = (float)LONG_MAX;
      *pMax = (float)LONG_MIN;

      for (col = 0; col < cols; col++)
         {
         _GetDataMinMax(col, &minVal, &maxVal);

         if (minVal < *pMin) *pMin = minVal;
         if (maxVal > *pMax) *pMax = maxVal;
         }
      }

   return true;
   }


// helper function for GetDataMinMax()
bool MapLayer::_GetDataMinMax(int col, float *pMin, float *pMax) const
   {
   *pMin = (float)LONG_MAX;
   *pMax = (float)LONG_MIN;

   int rows = m_pData->GetRowCount();

   for (int i = 0; i < rows; i++)
      {
      float val = m_pData->GetAsFloat(col, i);

      if (val != m_noDataValue)
         {
         if (*pMin > val)
            *pMin = val;

         if (*pMax < val)
            *pMax = val;
         }
      }

   return TRUE;
   }


bool MapLayer::GetGridDataMinMax(float *pMin, float *pMax) const
   {
   ASSERT(m_pData != NULL);

   *pMin = (float) 1.0e10;
   *pMax = (float)-1.0e10;

   int rows = m_pData->GetRowCount();
   int cols = m_pData->GetColCount();

   for (int row = 0; row < rows; row++)
      for (int col = 0; col < cols; col++)
         {
         float val = m_pData->GetAsFloat(col, row);

         if (*pMin > val && val != m_noDataValue)
            *pMin = val;

         if (*pMax < val && val != m_noDataValue)
            *pMax = val;
         }

   return true;
   }



// big/little endian conversion routines


WORD SwapTwoBytes(WORD w)
   {
   register WORD tmp;
   tmp = (w & 0x00FF);
   tmp = ((w & 0xFF00) >> 0x08) | (tmp << 0x08);
   return(tmp);
   }

DWORD SwapFourBytes(DWORD dw)
   {
   register DWORD tmp;
   tmp = (dw & 0x000000FF);
   tmp = ((dw & 0x0000FF00) >> 0x08) | (tmp << 0x08);
   tmp = ((dw & 0x00FF0000) >> 0x10) | (tmp << 0x08);
   tmp = ((dw & 0xFF000000) >> 0x18) | (tmp << 0x08);
   return(tmp);
   }

WORD GetBigWord(FILE *fp)
   {
   register WORD w;
   w = (WORD)(fgetc(fp) & 0xFF);
   w = ((WORD)(fgetc(fp) & 0xFF)) | (w << 0x08);
   return(w);
   }


WORD GetLittleWord(FILE *fp)
   {
   register WORD w;
   w = (WORD)(fgetc(fp) & 0xFF);
   w = ((WORD)(fgetc(fp) & 0xFF) << 0x08);
   return(w);
   }


DWORD GetBigDWORD(FILE *fp)
   {
   register DWORD dw;
   dw = (DWORD)(fgetc(fp) & 0xFF);
   dw = ((DWORD)(fgetc(fp) & 0xFF)) | (dw << 0x08);
   dw = ((DWORD)(fgetc(fp) & 0xFF)) | (dw << 0x08);
   dw = ((DWORD)(fgetc(fp) & 0xFF)) | (dw << 0x08);
   return(dw);
   }

DWORD GetLittleDWORD(FILE *fp)
   {
   register DWORD dw;
   /*int count =*/ fread(&dw, sizeof(DWORD), 1, fp);
   //ASSERT( count == 1 );
   return (dw);
   }

double GetLittleDouble(FILE *fp)
   {
   ASSERT(feof(fp) == 0);
   double dw;
   int count = (int)fread(&dw, sizeof(double), 1, fp);
   ASSERT(count == 1);
   return(dw);
   }


void PutBigWord(WORD w, FILE *fp)
   {
   fputc((w >> 0x08) & 0xFF, fp);
   fputc(w & 0xFF, fp);
   }


void PutLittleWord(WORD w, FILE *fp)
   {
   fputc(w & 0xFF, fp);
   fputc((w >> 0x08) & 0xFF, fp);
   }


void PutBigDWORD(DWORD dw, FILE *fp)
   {
   fputc((dw >> 0x18) & 0xFF, fp);
   fputc((dw >> 0x10) & 0xFF, fp);
   fputc((dw >> 0x08) & 0xFF, fp);
   fputc(dw & 0xFF, fp);
   }


void PutLittleDWORD(DWORD dw, FILE *fp)
   {
   //fputc( dw & 0xFF, fp );
   //fputc( (dw >> 0x08) & 0xFF, fp );
   //fputc( (dw >> 0x10) & 0xFF, fp );
   //fputc( (dw >> 0x18) & 0xFF, fp );
   fwrite((void*)&dw, sizeof(DWORD), 1, fp);
   }

void PutLittleDouble(double d, FILE *fp)
   {
   fwrite((void*)&d, sizeof(double), 1, fp);
   }



// Get an array of unique values for the specified data column. 
// Works for any type that can be converted to string
int MapLayer::GetUniqueValues(int col, CStringArray &valueArray, int maxCount) const
   {
   bool found;

   if (col == -1)
      {
      for (int i = 0; i < m_pData->GetColCount(); i++)
         GetUniqueValues(i, valueArray, maxCount);
      }
   else
      {
      for (int row = 0; row < m_pData->GetRowCount(); row++)
         {
         if (maxCount > 0 && valueArray.GetSize() >= maxCount)
            {
#ifndef NO_MFC
            CString msg;
            msg.Format(_T("The number of unique attributes is large.  Only the first %i unique values will be reported"), maxCount);
            Report::LogWarning(msg);
            //int retVal = AfxMessageBox(msg, MB_YESNO);

            //if (retVal == IDYES)
            //   maxCount = LONG_MAX;
            //else
               return valueArray.GetSize();
#else
            //arbitrary choice
            maxCount = LONG_MAX;
#endif
            }

         found = false;
         CString value = m_pData->GetAsString(col, row);

         // check to see if it has not been found yet
         for (int j = 0; j < valueArray.GetSize(); j++)
            {
            // string already found?
            if (valueArray[j].Compare(value) == 0)
               {
               found = true;
               break;
               }
            }

         if (found == false)
            valueArray.Add(value);
         }
      }

   SortStringArray(valueArray);

   return valueArray.GetSize();
   }


void SortStringArray(CStringArray &sarray)
   {
   int count = sarray.GetSize();
   char **strings = new char*[count];

   // allocate temporary strings
   for (int i = 0; i < count; i++)
      {
      strings[i] = new char[sarray[i].GetLength() + 1];
      lstrcpy(strings[i], sarray[i]);
      }

   // sort the pointers
   qsort((void*)strings, count, sizeof(char*), SortStringProc);

   for (int i = 0; i < count; i++)
      {
      sarray[i] = strings[i];
      delete[] strings[i];
      }

   delete[] strings;
   }


int SortStringProc(const void *elem1, const void *elem2)
   {
   char *str0 = *((char**)elem1);
   char *str1 = *((char**)elem2);
   return lstrcmp(str0, str1);
   }


// Get an array of unique values for the specified data column. 
// Works for any type that can be converted to UINT
int MapLayer::GetUniqueValues(int col, CUIntArray &valueArray, int maxCount) const
   {
   bool found;

   if (col == -1)
      {
      for (int i = 0; i < m_pData->GetColCount(); i++)
         GetUniqueValues(i, valueArray, maxCount);
      }
   else
      {
      for (int row = 0; row < m_pData->GetRowCount(); row++)
         {
         found = false;
         UINT value = (UINT)m_pData->GetAsInt(col, row);

         // check to see if it has not been found yet
         for (int j = 0; j < valueArray.GetSize(); j++)
            {
            // value already found?
            if (valueArray[j] == value)
               {
               found = true;
               break;
               }
            }

         if (found == false)
            valueArray.Add(value);
         }
      }

   return valueArray.GetSize();
   }



// Get an array of unique values for the specified data column. 
// Works for any type
int MapLayer::GetUniqueValues(int col, CArray< VData, VData& > &valueArray, int maxCount) const
   {
   bool found;

   if (col == -1)
      {
      for (int i = 0; i < m_pData->GetColCount(); i++)
         GetUniqueValues(i, valueArray, maxCount);
      }
   else
      {
      for (int row = 0; row < m_pData->GetRowCount(); row++)
         {
         found = false;
         VData value;
         bool ok = m_pData->Get(col, row, value);
         ASSERT(ok);

         if ((value.type == TYPE_STRING || value.type == TYPE_DSTRING) && value.val.vString == NULL)
            value = "";

         // check to see if it has not been found yet
         for (int j = 0; j < valueArray.GetSize(); j++)
            {
            // value already found?
            if (valueArray[j].Compare(value) == true)
               {
               found = true;
               break;
               }
            }

         if (found == false)
            valueArray.Add(value);
         }
      }

   //SortStringArray( valueArray );

   return valueArray.GetSize();
   }


int MapLayer::Project3D(MapLayer *pGrid, bool interpolate)
   {

   if (pGrid->m_layerType != LT_GRID)
      {
      Report::ErrorMsg("Projection Grid is not a grid layer");
      return -1;
      }

   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON)
      {
      Report::ErrorMsg("Source grid is not a polygon or line coverage");
      return -2;
      }

   // correct layer types certified, now start converting vertex's
   for (int i = 0; i < m_pPolyArray->GetSize(); i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);

      int vertexCount = pPoly->GetVertexCount();

      for (int j = 0; j < vertexCount; j++)
         {
         Vertex &v = (Vertex&)pPoly->GetVertex(j);
         // get the elevations of the nearest neighbors from the grid
         float elev = 0;
         int row, col;
         pGrid->GetGridCellFromCoord(v.x, v.y, row, col);
         if (interpolate == false)
            {
            pGrid->GetData(row, col, elev);
            v.z = elev;
            }
         else
            {
            POINT map[9] = { {-1,-1}, { 0,-1}, { 1,-1},    // top row
                               {-1, 0}, { 0, 0}, { 1, 0},    // center row
                               {-1, 1}, { 0, 1}, { 1, 1} };  // bottom row
            float sum_wdz = 0.0f;
            float sum_wd = 0.0f;
            for (int i = 0; i < 9; i++)
               {
               int _row = row + map[i].x;
               int _col = col + map[i].y;
               // skip over edges
               if (_row < 0) continue;
               if (_col < 0) continue;
               if (_row >= pGrid->GetRowCount()) continue;
               if (_col >= pGrid->GetColCount()) continue;

               // not an edge so get data and process it
               float elev;
               REAL x, y;
               pGrid->GetData(_row, _col, elev);           // elevation
               if (elev > 0.0f)
                  {
                  pGrid->GetGridCellCenter(_row, _col, x, y); // center
                  float distance = DistancePtToPt(v.x, v.y, x, y);

                  if (distance == 0.0f)    //Special case if point falls exactly in center of grid cell
                     distance = 0.001f;

                  float wd = 1.0f / (distance*distance);
                  sum_wd += wd;
                  sum_wdz += wd*elev;
                  }
               }  // end of for ( i < 9 )

            if (sum_wd == 0.0f)
               {
#ifndef NO_MFC
               int retVal = MessageBox(NULL,
                  "Source coverage is outside the clipped DEM, Elevation will be set to zero.  Do you want to continue?",
                  "Warning", MB_YESNO);

               if (retVal == IDNO)
                  return -3;
               else
#endif
                  v.z = 0.0f;

               }
            else
               v.z = sum_wdz / sum_wd;

            }  // end of else

         if (_isnan(v.z))
            {
            TRACE("Invalid (NAN) z calculated - setting to 0");
            v.z = 0.0f;
            }
         }  // end of:  for ( j < vertexCount )
      }  // end of:  for ( i < m_pPolyArray->GetSize() );


   m_3D = true;

   return m_pPolyArray->GetSize();
   }



bool MapLayer::GetGridCellFromCoord(REAL x, REAL y, int &row, int &col)
   {
   // Note:  returned row is from TOP down (e.g. top of screen is row 0)
   ASSERT(m_layerType == LT_GRID);

   if (y - m_vyMin < 0)
      return false;

   if (x - m_vxMin < 0)
      return false;

   row = (int)((y - m_vyMin) / m_cellHeight); // - 1; jpb, 11/4/99
   col = (int)((x - m_vxMin) / m_cellWidth); // - 1;

   row = m_pData->GetRowCount() - row - 1; // invert to be top down

   if (row < 0) return false;
   if (row >= m_pData->GetRowCount()) return false;

   if (col < 0) return false;
   if (col >= m_pData->GetColCount()) return false;

   return true;
   }


Poly *MapLayer::GetAncestorPolygon(Poly *pPoly)
   {
   // get polygon for passed cell and check that it exists
   ASSERT(pPoly != NULL);

   // if cell has parents, recurse to root parent
   if (pPoly->GetParentCount() > 0)
      {
      ASSERT(pPoly->GetParentCount() == 1);

      while (pPoly->GetParentCount() > 0)
         {
         int parentIndex = pPoly->GetParent(0);
         pPoly = this->GetPolygon(parentIndex);
         }
      }

   return pPoly;
   }


void MapLayer::GetGridCellCenter(int row, int col, REAL &x, REAL &y) const
   {
   // Note: assumes rows are ordered from top down, vcoord system in increasing upward
   ASSERT(m_layerType == LT_GRID);

   x = m_vxMin + (col * m_cellWidth) + m_cellWidth / 2.0f;
   //y = m_vyMin + ( row * m_cellHeight ) + m_cellHeight/2.0f;
   // changed (Bolte, 7/18/02)
   int rows = GetRowCount();
   y = m_vyMin + ((rows - row - 1) * m_cellHeight) + m_cellHeight / 2.0f;
   }

void MapLayer::GetGridCellRect(int row, int col, COORD2d &ll, COORD2d &ur) const
   {
   // Note: assumes rows are ordered from top down, vcoord system in increasing upward
   ASSERT(m_layerType == LT_GRID);
   REAL x, y;
   GetGridCellCenter(row, col, x, y);

   float width = GetGridCellWidth();
   float height = GetGridCellHeight();

   ll.x = x - width / 2;
   ll.y = y - height / 2;
   ur.x = ll.x + width;
   ur.y = ll.y + height;
   }


int MapLayer::CalcSlopes(int colToSet)
   {
   //   ASSERT( m_layerType == LT_POLYGON );
   ASSERT(m_pData != NULL);
   ASSERT(colToSet < m_pData->GetColCount());
   ASSERT(m_pData->GetRowCount() == m_pPolyArray->GetSize());

   int polyCount = m_pPolyArray->GetSize();

   for (int i = 0; i < polyCount; i++)
      {
      float slope = m_pPolyArray->GetAt(i)->CalcSlope();
      m_pData->Set(colToSet, i, slope);
      m_pMap->Notify(NT_CALCSLOPE, i, 0);
      }

   return polyCount;
   }

int MapLayer::Create3dShape(MapLayer *pDEMLayer)
   {
   int polyCount = GetPolygonCount(MapLayer::ALL);
   for (int i = 0; i < polyCount - 1; i++)
      {
      Poly *pCurrent = GetPolygon(i);
      for (int j = 0; j < pCurrent->GetVertexCount() - 1; j++)
         {
         Vertex &vFirst = (Vertex&)pCurrent->GetVertex(j);
         int col, row;
         bool ok = pDEMLayer->GetGridCellFromCoord(vFirst.x, vFirst.y, row, col);
         float elev;
         ok = pDEMLayer->GetData(row, col, elev);
         vFirst.z = elev;
         }
      }

   return -1;
   }


int MapLayer::PopulateElevation(int colToSet)
   {
   ASSERT(m_layerType != LT_GRID);
   ASSERT(m_pData != NULL);
   ASSERT(colToSet < m_pData->GetColCount());
   ASSERT(m_pData->GetRowCount() == m_pPolyArray->GetSize());

   int polyCount = m_pPolyArray->GetSize();

   for (int i = 0; i < polyCount; i++)
      {
      Poly *pPoly = GetPolygon(i);
      float elev = pPoly->CalcElevation();
      m_pData->Set(colToSet, i, elev);
      }

   return polyCount;
   }



int MapLayer::CalcLengthSlopes(int slopeCol, int colToSet)
   {
   ASSERT(m_layerType == LT_POLYGON);
   ASSERT(m_pData != NULL);
   ASSERT(colToSet < m_pData->GetColCount());
   ASSERT(m_pData->GetRowCount() == m_pPolyArray->GetSize());

   int polyCount = m_pPolyArray->GetSize();

   for (int i = 0; i < polyCount; i++)
      {
      float slope = m_pData->GetAsFloat(slopeCol, i);  // slope is nondecimal percent
      float area = m_pData->GetAsFloat(0, i);  // area is m2
      area = area * HA_PER_M2;   // convert to hectares....

      float lengthSlope = m_pPolyArray->GetAt(i)->CalcLengthSlope(slope, area);  // unitless
      m_pData->Set(colToSet, i, lengthSlope);
      }

   return polyCount;
   }



bool MapLayer::IsDataValid(int row, int col) const
   {
   ASSERT(row < m_pData->GetRowCount());
   ASSERT(col < m_pData->GetColCount());
   ASSERT(row >= 0);
   ASSERT(col >= 0);

   float data;
   GetData(row, col, data);

   return (data == m_noDataValue ? false : true);
   }


void MapLayer::SetAllData(float value, bool includeNoData)
   {
   ASSERT(m_layerType == LT_GRID);

   int rows = GetRowCount();
   int cols = GetColCount();

   for (int row = 0; row < rows; row++)
      for (int col = 0; col < cols; col++)
         {
         if (includeNoData == false) // need to check noData regions?
            {
            float currentValue;
            bool ok = m_pData->Get(col, row, currentValue);
            ASSERT(ok);

            if (currentValue != m_noDataValue)
               SetData(row, col, value);
            }
         else
            SetData(row, col, value);
         }
   }

bool MapLayer::SetColData(int col, VData value, bool includeNoData)
   {
   if (m_pDbTable == NULL)
      return false;

   int rows = GetRowCount(MapLayer::ALL);
   int cols = GetColCount();

   if (col < 0 || col >= cols)
      return false;

   for (int row = 0; row < rows; row++)
      {
      if (includeNoData == false) // need to check noData regions?
         {
         float currentValue;
         m_pData->Get(col, row, currentValue);

         if (currentValue != m_noDataValue)
            SetData(row, col, value);
         }
      else
         SetData(row, col, value);
      }

   return true;
   }


int MapLayer::SelectPoint(COORD2d pt, bool clear /* =true */)
   {
   POINT _pt;
   m_pMap->VPtoLP(pt.x, pt.y, _pt);

   return SelectPoint(_pt, clear);
   }


int MapLayer::SelectPoint(POINT pt, bool clear /* =true */)   // Note: pt is a LOGICAL coord
   {
   if (clear)
      ClearSelection();

   // start checking polygons
   switch (m_layerType)
      {
      case LT_POLYGON:
      case LT_LINE:
         {
         int polyCount = m_pPolyArray->GetSize();

         for (int i = 0; i < polyCount; i++)
            {
            Poly *pPoly = m_pPolyArray->GetAt(i);
            int isInPoly = IsPointInPoly(pPoly->m_ptArray, pPoly->GetVertexCount(), pt);

            if (isInPoly)
               {
               AddSelection(i);
               break;
               }
            }  // end of:  for( i < polyCount )
         }
         break;

      case LT_POINT:
      case LT_GRID:
         ASSERT(0);   // not implemented (yet)
         break;
      }

   return GetSelectionCount();
   }


int MapLayer::SelectArea(COORD2d _ll, COORD2d _ur, bool clear, bool insideOnly)   // Note: _llm _ur are real world coords
   {
   POINT ll, ur;
   m_pMap->VPtoLP(_ll.x, _ll.y, ll);
   m_pMap->VPtoLP(_ur.x, _ur.y, ur);

   return SelectArea(ll, ur, clear, insideOnly);
   }



int MapLayer::SelectArea(POINT _ll, POINT _ur, bool clear, bool insideOnly)   // Note: _llm _ur are LOGICAL coords
   {
#ifdef NO_MFC
   using namespace std; //min()/max()
#endif

   if (clear)
      ClearSelection();

   POINT ll, ur;
   ll.x = min(_ll.x, _ur.x);      // make sure ponts are oriented correctly
   ll.y = min(_ll.y, _ur.y);
   ur.x = max(_ll.x, _ur.x);
   ur.y = max(_ll.y, _ur.y);

   // start checking polygons
   switch (m_layerType)
      {
      case LT_POLYGON:
      case LT_LINE:
         {
         int polyCount = m_pPolyArray->GetSize();

         for (int i = 0; i < polyCount; i++)
            {
            Poly *pPoly = m_pPolyArray->GetAt(i);

            if (insideOnly)
               {
               bool isOutside = false;
               for (int j = 0; j < pPoly->GetVertexCount(); j++)
                  {
                  POINT *v = pPoly->m_ptArray + j;

                  if (v->x < ll.x || v->x > ur.x || v->y < ll.y || v->y > ur.y)
                     {
                     isOutside = true;
                     break;
                     }

                  if (!isOutside)
                     AddSelection(i);
                  }
               }
            else
               {
               for (int j = 0; j < pPoly->GetVertexCount(); j++)
                  {
                  POINT *v = pPoly->m_ptArray + j;

                  if (ll.x <= v->x && v->x <= ur.x && ll.y <= v->y && v->y <= ur.y)
                     {
                     AddSelection(i);
                     break;
                     }
                  }
               }
            }  // end of:  for( i < polyCount )
         }
         break;

      case LT_POINT:
      case LT_GRID:
         ASSERT(0);
         break;
      }

   return GetSelectionCount();
   }


bool MapLayer::IsSelected(int polyIndex)
   {
   for (int i = 0; i < (int)m_selection.GetSize(); i++)
      {
      if (m_selection[i] == polyIndex)
         return true;
      }

   return false;
   }



// gets the first record number (offset) with the specified value
int MapLayer::FindIndex(int col, int value, int startIndex /*=0*/) const
   {
   if (m_pData == NULL)
      return -1;

   int _value;
   for (int i = startIndex; i < GetRecordCount(MapLayer::ALL); i++)
      {
      if (IsDefunct(i))
         continue;

      if (GetData(i, col, _value) == false)
         return -2;

      if (_value == value)
         return i;
      }

   return -3;
   }


bool MapLayer::FindMinValue(int &minRow, int &minCol) const
   {
   if (m_layerType != LT_GRID)
      return false;

   ASSERT(m_pData != NULL);

   float minVal = 1.0e10f;

   int rows = m_pData->GetRowCount();
   int cols = m_pData->GetColCount();

   for (int row = 0; row < rows; row++)
      for (int col = 0; col < cols; col++)
         {
         float val = m_pData->GetAsFloat(col, row);

         if (val < minVal && val != m_noDataValue)
            {
            minRow = row;
            minCol = col;
            minVal = val;
            }
         }

   return true;
   }


bool MapLayer::IsPolyInMap(Poly *pPoly, float edgeDistance /*=0*/) const
   {
   REAL xMinPoly = 0.0f, yMinPoly = 0.0f, xMaxPoly = 0.0f, yMaxPoly = 0.0f;
   REAL vxMinLayer = 0.0f, vxMaxLayer = 0.0f, vyMinLayer = 0.0f, vyMaxLayer = 0.0f;

   pPoly->GetBoundingRect(xMinPoly, yMinPoly, xMaxPoly, yMaxPoly);

   GetExtents(vxMinLayer, vxMaxLayer, vyMinLayer, vyMaxLayer);

   if ((xMaxPoly < vxMinLayer - edgeDistance)    // case 1 
      || (xMinPoly > vxMaxLayer + edgeDistance))  // case 2
      return false;  // no reason to look further

   // check y intersection next
   if ((yMaxPoly < vyMinLayer - edgeDistance)    // case 1 
      || (yMinPoly > vyMaxLayer + edgeDistance))  // case 2
      return false;

   return true;
   }


AttrIndex *MapLayer::GetAttrIndex(int col)
   {
   // first time?
   if (m_pAttrIndex == NULL)
      {
      m_pAttrIndex = new AttrIndex;
      m_pAttrIndex->ReadIndex(this->m_pDbTable);
      }

   // does it contain the specified column?
   if (m_pAttrIndex->ContainsCol(col) == false)
      {
      m_pAttrIndex->BuildIndex(this->m_pDbTable, col);
      m_pAttrIndex->WriteIndex();
      }

   return m_pAttrIndex;
   }


int MapLayer::CompareSpatialIndexMethod(SI_METHOD *method)const
   {
   // returns 0 if method matches; -1 if no index; 1 if index but no match
   if (m_pSpatialIndex == NULL)
      return -1;
   if (m_pSpatialIndex->m_method == *method)
      return 0;
   *method = m_pSpatialIndex->m_method;
   return 1;
   }



int MapLayer::LoadSpatialIndex(LPCTSTR filename /*=NULL*/, float maxDistance /*=500*/, SI_METHOD method /*=SIM_NEAREST*/, MapLayer *pToLayer /*=NULL*/, PFNC_SPATIAL_INFO_EX fillEx/*=NULL*/)
   {
   // 1 for successful load, < 0 for failure

   if (m_pSpatialIndex == NULL)
      m_pSpatialIndex = new SpatialIndex;
   else
      m_pSpatialIndex->Clear();

   // see if a spatial index currently exists on disk
   char _filename[128];

   if (filename != NULL)
      lstrcpy(_filename, filename);
   else
      {
      if (m_path.IsEmpty())
         lstrcpy(_filename, "temp.spndx");
      else
         {
         lstrcpy(_filename, (LPCTSTR)m_path);
         char *dot = strrchr(_filename, '.');

         if (dot != NULL)
            lstrcpy(dot, ".spndx");
         else
            lstrcat(_filename, ".spndx");
         }
      }

   // have file name established, see if it exists
   float distance;
   int count;
   int retVal = m_pSpatialIndex->GetSpatialIndexInfo(_filename, distance, count);

   if (retVal > 0) // file exists, check size and check for extra data
      {
      int error = 0;
      if (distance >= maxDistance
         && (count == GetRecordCount() || (m_recordsLoaded > 0 && count >= m_recordsLoaded))
         && (fillEx == NULL || m_pSpatialIndex->HasExtendedSpatialInfoEx(_filename, error))) // is the current spatial index okay?
         {
         retVal = m_pSpatialIndex->ReadIndex(_filename, this, pToLayer, count /*GetRecordCount( MapLayer::ALL)*/);    // yes, so just read it from the disk

         if (retVal < 0 || m_pSpatialIndex->m_method != method)
            return -1;
         }

      else  // exists, but wrong parameters
         return -2;
      }

   else  // file doesn't exist
      return -3;

   return 1;
   }


//--------------------------------------------------------------------------------------------------------------------------------
// int MaSpLayer::CreateSpatialIndex()
//
// creates a spatial index for use prior to call GetNearbyPolys()
//
// Before creating an index, it checks to see if a valid index already exists, and loads it if possible.
//
// Parameters:
//    LPCTSTR filename   - name of the index to use, or NULL.  If NULL, then the method assumes the index name is "temp.spndx"
//                        if there is no path information for the "this" MapLayer;  if path info
//                        exists, then the spatial index name is the name of "this" layer, with a ".spndx" extension.
//    int maxCount      - max number of "nearby" polys to store
//    float maxDistance - max distance to "look" for nearby polys
//    SI_METHOD method  - currently  SIM_NEAREST, SIM_CENTROID - methods for determining proximity
//    MapLayer *pToLayer-layer to look to, or NULL for internal (self-referenced) index
//---------------------------------------------------------------------------------------------------------------------------------

int MapLayer::CreateSpatialIndex(LPCTSTR filename, int maxCount, float maxDistance, SI_METHOD method, MapLayer *pToLayer /*=NULL*/, PFNC_SPATIAL_INFO_EX fillEx /*=NULL*/)
   {
   if (m_pSpatialIndex == NULL)
      m_pSpatialIndex = new SpatialIndex;
   else
      m_pSpatialIndex->Clear();

   // see if a spatial index currently exists on disk
   char _filename[128];

   if (filename != NULL)
      lstrcpy(_filename, filename);
   else
      {
      if (m_path.IsEmpty())
         lstrcpy(_filename, "temp.spndx");
      else
         {
         lstrcpy(_filename, (LPCTSTR)m_path);
         char *dot = strrchr(_filename, '.');

         if (dot != NULL)
            lstrcpy(dot, ".spndx");
         else
            lstrcat(_filename, ".spndx");
         }
      }

   // have file name established, see if it exists
   float distance;
   int count;
   int retVal = m_pSpatialIndex->GetSpatialIndexInfo(_filename, distance, count);

   if (retVal > 0) // file exists, check size and check for extra data
      {
      int error = 0;
      if (distance >= maxDistance
         && (count == GetRecordCount() || (m_recordsLoaded > 0 && count >= m_recordsLoaded))
         && (fillEx == NULL || m_pSpatialIndex->HasExtendedSpatialInfoEx(_filename, error))) // is the current spatial index okay?
         {
         retVal = m_pSpatialIndex->ReadIndex(_filename, this, pToLayer, count /*GetRecordCount( MapLayer::ALL)*/);    // yes, so just read it from the disk

         if (retVal < 0 || m_pSpatialIndex->m_method != method)
            {
            //MessageBox( GetActiveWindow(), "Need to Build the Spatial Index.  This will take a while.", "Warning", MB_OK );
            WAIT_CURSOR;
            retVal = m_pSpatialIndex->BuildIndex(this, pToLayer, maxCount, maxDistance, method, fillEx);
            if (retVal > 0)
               m_pSpatialIndex->WriteIndex(_filename);
            }
         }

      else  // otherwise, rebuild it
         {
         //MessageBox( GetActiveWindow(), "Need to Build the Spatial Index.  This will take a while.", "Warning", MB_OK );
         WAIT_CURSOR;
         retVal = m_pSpatialIndex->BuildIndex(this, pToLayer, maxCount, maxDistance, method, fillEx);

         if (retVal > 0)
            m_pSpatialIndex->WriteIndex(_filename);
         }
      }

   else  // file doesn't exist, so create index and save to disk
      {
      //MessageBox( GetActiveWindow(), "Need to Build the Spatial Index.  This will take a while.", "Warning", MB_OK );
      WAIT_CURSOR;
      retVal = m_pSpatialIndex->BuildIndex(this, pToLayer, maxCount, maxDistance, method, fillEx);

      if (retVal > 0)
         m_pSpatialIndex->WriteIndex(_filename);
      }

   return retVal;
   }

int   _neighbors[2048];
float _distances[2048];
void* _ex[2048];

int MapLayer::GetNearbyPolysFromIndex(Poly *pPoly, int *neighbors, float *distances, int maxCount, float maxDistance,
   SI_METHOD /*method=SIM_NEAREST*/, MapLayer* /*pToLayer =NULL*/, void** ex /*= NULL*/) const
   {
   if (m_pSpatialIndex == NULL)
      return -1;

   //if ( m_recordsLoaded > 0 )
   //   return -1;

   int thisIndex = pPoly->m_id;

   if (thisIndex >= m_pSpatialIndex->GetCount())
      return 0;

   ASSERT(thisIndex < m_pSpatialIndex->GetCount());

   int size = m_pSpatialIndex->GetNeighborArraySize(thisIndex);

   /// added jpb for testing
   if (size > 2048)
      size = 2048;
   // make some termporary arrays
   //int   *_neighbors = new int  [ size ];  // NOTE: SLOWDOWN HERE!!!!
   //float *_distances = new float[ size ];
   //void* *_ex = NULL;
   //if ( ex != NULL )
   //   _ex = new void*[ size ];

   void* *__ex__ = NULL;
   if (ex != NULL)
      __ex__ = _ex;

   // get the values from the index
   int retVal = m_pSpatialIndex->GetNeighborArray(thisIndex, _neighbors, _distances, size, __ex__);

   if (retVal < 0)
      {
      //delete [] _neighbors;  // jpb
      //delete [] _distances;
      //if ( ex != NULL )
      //   delete [] _ex;
      return retVal;
      }

   // iterate through, getting any <= maxdistance
   int count = 0;
   for (int i = 0; i < size; i++)
      {
      if (_distances[i] <= maxDistance)
         {
         ASSERT(0 <= count && count < maxCount);
         neighbors[count] = _neighbors[i];

         if (distances != NULL)
            distances[count] = _distances[i];

         if (ex != NULL)
            ex[count] = _ex[i];

         count++;
         }
      else
         break; // BECAUSE _distances are SORTED.

      if (count >= maxCount)
         break;
      }

   //delete [] _neighbors;
   //delete [] _distances;  jpb
   //if ( ex != NULL )
   //   delete [] _ex;

   return count;  // actual number populated
   }

int MapLayer::GetNearbyPolys(Poly *pPoly, int *neighbors, float *distances, int maxCount, float maxDistance,
   SI_METHOD method/*=SIM_NEAREST*/, MapLayer *pToLayer /*=NULL*/) const
   {
   if (m_pSpatialIndex != NULL && m_pSpatialIndex->IsBuilt() && m_pSpatialIndex->m_pLayer == this && (pToLayer == NULL || m_pSpatialIndex->m_pToLayer == pToLayer))
      return GetNearbyPolysFromIndex(pPoly, neighbors, distances, maxCount, maxDistance, method, pToLayer);

   if (pToLayer == NULL)
      pToLayer = (MapLayer*) this;

   int recordCount = pToLayer->GetRecordCount(MapLayer::ALL);
   int count = 0;

   REAL xMin, xMax, yMin, yMax;
   pPoly->GetBoundingRect(xMin, yMin, xMax, yMax);

   ASSERT(xMin <= xMax);
   ASSERT(yMin <= yMax);

   REAL xMinTo, xMaxTo, yMinTo, yMaxTo;

   // iterate through the poly's in the to layer
   //int iCPU = omp_get_num_procs();
   //omp_set_num_threads(iCPU);

   //#pragma omp parallel for
   for (int i = 0; i < recordCount; i++)
      {
      Poly *pToPoly = pToLayer->GetPolygon(i);

      if (pToPoly == pPoly) // skip this polygon
         continue;

      // are we even close?
      bool ok = pToPoly->GetBoundingRect(xMinTo, yMinTo, xMaxTo, yMaxTo);
      ASSERT(ok);

      bool passX = false;
      bool passY = false;

      ASSERT(xMinTo <= xMaxTo);
      ASSERT(yMinTo <= yMaxTo);

      // fundamentally, if the closest distance between the two poly's (D) is less
      // than the max distance (M), then the poly's are neighbors, e.g. D < M 
      //
      // To speed the search, we first look at the bounding rects.  The bounding rect represents
      // the maximum size of the polygons.  Therefore, if D for the bounding rects is greater than
      // M, then we can be assured that these polygons are not neighbors.  We apply this test as a
      // first cut.  Any poly's that pass are then examined vertex by vertex to get the closest distance

      // For the bounding rect, we further simplify the analysis.  D for overlapping rects is 0,
      // and so the poly passes, Rect's overlap if:
      //   1) one rect overlaps another, or
      //   2) the "left" edge of one is within D of the right edge of the other

      // check condition 1 for both poly's
      if (BETWEEN(xMin, xMinTo, xMax)    // Note: BETWEEN is inclusive ( x <= y <= z )
         || BETWEEN(xMin, xMaxTo, xMax)
         || BETWEEN(xMinTo, xMin, xMaxTo)
         || BETWEEN(xMinTo, xMax, xMaxTo))
         passX = true;

      // condition 2
      else if (((xMinTo > xMax) && (xMinTo - xMax <= maxDistance))
         || ((xMin > xMaxTo) && (xMin - xMaxTo <= maxDistance)))
         passX = true;

      // repeat for y's
      if (BETWEEN(yMin, yMinTo, yMax)
         || BETWEEN(yMin, yMaxTo, yMax)
         || BETWEEN(yMinTo, yMin, yMaxTo)
         || BETWEEN(yMinTo, yMax, yMaxTo))
         passY = true;

      // condition 2
      else if (((yMinTo > yMax) && (yMinTo - yMax <= maxDistance))
         || ((yMin > yMaxTo) && (yMin - yMaxTo <= maxDistance)))
         passY = true;

      if (passX && passY)   // passes both?
         {
         // we are close, so check more detail
         float distance;
         switch (method)
            {
            case SIM_NEAREST:
               distance = pPoly->NearestDistanceToPoly(pToPoly, 2 * maxDistance);  // ???? need to check threshold useage!!!
               break;

            case SIM_CENTROID:
               distance = pPoly->CentroidDistanceToPoly(pToPoly);
               break;
            }

         if (distance <= maxDistance)
            {
            neighbors[count] = i;

            if (distances != NULL)
               distances[count] = distance;

            count++;

            if (count >= maxCount)
               return count;
            }
         }
      }  // end of for

   return count;
   }


int MapLayer::GetNearestPoly(REAL x, REAL y) const
   {
   float nearestDistance = 1.0e18f;
   int   nearestIndex = -1;

   for (Iterator i = Begin(); i != End(); i++)
      {
      Poly *pPoly = GetPolygon(i);

      int vertexCount = pPoly->GetVertexCount();

      for (int j = 0; j < vertexCount - 1; j++)
         {
         const Vertex &start = pPoly->GetVertex(j);
         const Vertex &end = pPoly->GetVertex(j + 1);

         float distance = 0.0f;
         /*bool onSegment = */DistancePtToLineSegment(x, y, start.x, start.y, end.x, end.y, distance);

         //if ( onSegment )
         //   return i;
         if (distance < 0.000001f)
            return i;

         if (fabs(distance) < nearestDistance)
            {
            nearestDistance = (float)fabs(distance);
            nearestIndex = i;
            }
         }
      }

   return nearestIndex;
   }


int MapLayer::GetNearestPoly(Poly *pPoly, float maxDistance, SI_METHOD method/*=SIM_NEAREST*/, MapLayer *pToLayer /*=NULL*/) const
   {
   if (pToLayer == NULL)
      pToLayer = (MapLayer*) this;

   int nearestIndex = 0;
   float nearestDistance = 1.0e12f;

   REAL xMin, xMax, yMin, yMax;
   pPoly->GetBoundingRect(xMin, yMin, xMax, yMax);

   ASSERT(xMin <= xMax);
   ASSERT(yMin <= yMax);

   REAL xMinTo, xMaxTo, yMinTo, yMaxTo;

   // iterate through the poly's in the to layer
   for (Iterator i = pToLayer->Begin(); i != pToLayer->End(); i++)
      {
      Poly *pToPoly = pToLayer->GetPolygon(i);

      if (pToPoly == pPoly) // skip this polygon
         continue;

      // are we even close?
      bool ok = pToPoly->GetBoundingRect(xMinTo, yMinTo, xMaxTo, yMaxTo);
      ASSERT(ok);

      bool passX = false;
      bool passY = false;

      ASSERT(xMinTo <= xMaxTo);
      ASSERT(yMinTo <= yMaxTo);

      // fundamentally, if the closest distance between the two poly's (D) is less
      // than the max distance (M), then the poly's are neighbors, e.g. D < M 
      //
      // To speed the search, we first look at the bounding rects.  The bounding rect represents
      // the maximum size of the polygons.  Therefore, if D for the bounding rects is greater than
      // M, then we can be assured that these polygons are not neighbors.  We apply this test as a
      // first cut.  Any poly's that pass are then examined vertex by vertex to get the closest distance

      // For the bounding rect, we further simplify the analysis.  D for overlapping rects is 0,
      // and so the poly passes, Rect's overlap if:
      //   1) one rect overlaps another, or
      //   2) the "left" edge of one is within D of the right edge of the other

      // check condition 1 for both poly's
      if (BETWEEN(xMin, xMinTo, xMax)
         || BETWEEN(xMin, xMaxTo, xMax)
         || BETWEEN(xMinTo, xMin, xMaxTo)
         || BETWEEN(xMinTo, xMax, xMaxTo))
         passX = true;

      // condition 2
      else if (((xMinTo > xMax) && (xMinTo - xMax <= maxDistance))
         || ((xMin > xMaxTo) && (xMin - xMaxTo <= maxDistance)))
         passX = true;

      // repeat for y's
      if (BETWEEN(yMin, yMinTo, yMax)
         || BETWEEN(yMin, yMaxTo, yMax)
         || BETWEEN(yMinTo, yMin, yMaxTo)
         || BETWEEN(yMinTo, yMax, yMaxTo))
         passY = true;

      // condition 2
      else if (((yMinTo > yMax) && (yMinTo - yMax <= maxDistance))
         || ((yMin > yMaxTo) && (yMin - yMaxTo <= maxDistance)))
         passY = true;

      if (passX && passY)   // passes both?
         {
         // we are close, so check more detail
         float distance;
         switch (method)
            {
            case SIM_NEAREST:
               distance = pPoly->NearestDistanceToPoly(pToPoly, 2 * maxDistance);  // ???? need to check threshold useage!!!
               break;

            case SIM_CENTROID:
               distance = pPoly->CentroidDistanceToPoly(pToPoly);
               break;
            }

         if (distance < nearestDistance)
            {
            nearestIndex = i;
            nearestDistance = distance;
            }
         }
      }

   return nearestIndex;
   }


float MapLayer::ComputeAdjacentLength(Poly *pThisPoly, Poly *pSourcePoly)
   {
   // starting with pThisPoly's vertices, find the first common vertext
   int thisVertexCount = pThisPoly->GetVertexCount();
   int sourceVertexCount = pSourcePoly->GetVertexCount();

   int thisVertexIndex = -1;
   int sourceVertexIndex = -1;

   for (int i = 0; i < thisVertexCount; i++)
      {
      const Vertex &vThis = pThisPoly->GetVertex(i);

      sourceVertexIndex = -1;

      for (int j = 0; j < sourceVertexCount; j++)
         {
         const Vertex &vSource = pSourcePoly->GetVertex(j);

         if (vThis.x == vSource.x && vThis.y == vSource.y)
            {
            thisVertexIndex = i;
            sourceVertexIndex = j;
            break;
            }
         }
      }

   // if there is an vertex in common, we should have it at this point.
   if (thisVertexIndex == -1 || sourceVertexIndex == -1)
      return 0.0f;

   // assume that....
   //
   //       ----->
   //   *-------*-----*
   //   |             |
   //   |    this     |
   //   *-----*--*----* <--- thisVertexIndex, sourceVertexIndex
   //   |             |
   //   |   source    |
   //   *-----*-------*
   //      <------

   float distance = 0.0f;

   while (1)
      {
      thisVertexIndex++;      // increment indexes
      sourceVertexIndex++;

      if (thisVertexIndex >= thisVertexCount || sourceVertexIndex < 0)
         return distance;

      const Vertex &vThis = pThisPoly->GetVertex(thisVertexIndex);
      const Vertex &vSource = pSourcePoly->GetVertex(sourceVertexIndex);

      if (vThis.x != vSource.x || vThis.y != vSource.y)
         return distance;

      float segmentDistance = DistancePtToPt(vThis.x, vThis.y, vSource.x, vSource.y);

      distance += segmentDistance;
      }

   return distance;
   }



//-------------------- Watershed functions --------------//

float MapLayer::GetDownFlowPathDistance(int row, int col, MapLayer *pStreamGrid) const
   {
   ASSERT(m_layerType == LT_GRID);
   ASSERT(pStreamGrid->m_layerType == LT_GRID);

   float cellwidth = GetGridCellWidth();

   // check for stopping condition... (non-zero indicates river boundary)
   if (pStreamGrid->IsCellInStream(row, col))
      return cellwidth / 2;  // why cellwidth/2 ????  Note:  Cells near grid border have high values.  Why???

   // try to follow the path.  if FollowFlowPath() fails, we are at a border moving off the grid
   int newRow = row;
   int newCol = col;

   bool ok = FollowFlowPath(newRow, newCol);  // gets next cell coords along flowpath, returns false
                                                // if we new row is off a border
   if (newRow != row && newCol != col)
      cellwidth *= 1.414f;

   if (!ok)    // at a border?
      return cellwidth;   // should probably use a large value....

   // otherwise, recurse and proceed      
   float flowPath = GetDownFlowPathDistance(newRow, newCol, pStreamGrid);

   return flowPath + cellwidth;
   }


float MapLayer::GetDownFlowPathGradient(int row, int col, MapLayer *pStreamGrid, MapLayer *pDEMGrid, int &count) const
   {
   ASSERT(m_layerType == LT_GRID);
   float currentElev = 0;
   float downElev = 0;
   float gradAverage = 0.0001f;
   float cellwidth = GetGridCellWidth();
   pDEMGrid->GetData(row, col, currentElev);
   count++;

   // check for stopping condition... (non-zero indicates river boundary)
   if (pStreamGrid->IsCellInStream(row, col))
      return gradAverage;  // why cellwidth/2 ????  Note:  Cells near grid border have high values.  Why???

   // try to follow the path.  if FollowFlowPath() fails, we are at a border moving off the grid
   int newRow = row;
   int newCol = col;

   bool ok = FollowFlowPath(newRow, newCol);  // gets next cell coords along flowpath, returns false
                                                // if we new row is off a border
   if (newRow != row && newCol != col)
      cellwidth *= 1.414f;

   pDEMGrid->GetData(newRow, newCol, downElev);

   float  gradient = (currentElev - downElev) / cellwidth;
   if (gradient == 0.0f)
      gradient = 0.0001f;

   if (!ok)    // at a border?
      return gradient;   // should probably use a large value....

   // otherwise, recurse and proceed      
   gradAverage = GetDownFlowPathGradient(newRow, newCol, pStreamGrid, pDEMGrid, count);

   return (gradAverage + gradient);
   }




//------------------------------------------------------------------------------------------------
// Given a row and col, follow the flowpath to a stream and determine which side of the stream
// the original row and col were on
//------------------------------------------------------------------------------------------------

int MapLayer::GetSideOfStream(int row, int col, MapLayer *pStreamLayer) const
   {
   ASSERT(pStreamLayer->m_layerType == LT_GRID);

   int side = -1;

   int previousRow = row;
   int previousCol = col;

   while (!pStreamLayer->IsCellInStream(row, col))
      {
      previousRow = row;
      previousCol = col;
      bool ok = FollowFlowPath(row, col);
      if (!ok)
         break;
      }

   int landDir, streamDir;
   bool ok1 = GetData(row, col, streamDir);
   if (streamDir == GetNoDataValue()) // if the centroid of the cell is outside of the watershed area, the flowpath will move off grid...
      return side = -1;

   /*bool ok2 =*/ GetData(previousRow, previousCol, landDir);

   //  At this point, streamDir is the stream flow direction and landDir is the flow direction of the cell
   //  flowing into the streamDir cell.

   ASSERT(ok1 == true);
   int newRowUp = row;
   int newColUp = col;

   //  If the two flow directions are equal, we need the flow direction of the stream cell 
   //  that is just upstream of the current stream cell.

   if (landDir == streamDir)
      {
      bool ok3 = GetUpstreamCell(row, col, pStreamLayer, newRowUp, newColUp);
      if (ok3)
         /*bool ok4 =*/ GetData(newRowUp, newColUp, streamDir);
      }

   int landPower = FlowDirToPower(landDir); // convert from flow direction into 2^power (get the power part)
   int streamPower = FlowDirToPower(streamDir);
   int relationship = -1;
   int angle = abs(landPower - streamPower);
   //   bool landCardinal = false;
   //   bool streamCardinal = false;

   //   if (landPower == 0 || landPower == 2 || landPower == 4 || landPower == 6)
   //      landCardinal = true;
   //   if (streamPower == 0 || streamPower == 2 || streamPower == 4 || streamPower == 6)
   //      streamCardinal = true;

   // These cases occur when the landDir is a cardinal direction (N E S W) or when the landDir and
   // streamDir form an acute angle.  In this case, the side of the stream is unambiguous  

   if (angle == 4)//the two directions face each other.  This should never occur
      relationship = 0;
   if (angle == 2 || angle == 6) // unambigous
      relationship = 1;
   if (angle == 0 || angle == 3 || angle == 5 || angle == 1 || angle == 7) // ambiguous - look one stream cell upstream
      relationship = 2;

   switch (relationship)
      {
      case 0:
         {
         side = -1;
         break;
         }
      case 1:
         {
         int streamTemp = streamDir;
         for (int i = 0; i < 3; i++)
            {
            streamTemp *= 2;
            if (streamTemp > 128)
               streamTemp = 1;
            if (landDir == streamTemp)
               {
               side = 1; // left side of stream, looking downstream
               break;
               }
            }
         if (side == -1)
            side = 2; // right side of the stream, looking downstream

         break;
         }

      case 2:
         {

         bool ok3 = GetUpstreamCell(row, col, pStreamLayer, newRowUp, newColUp);
         if (ok3)
            /*bool ok4 =*/ GetData(newRowUp, newColUp, streamDir);
         int streamTemp = streamDir;
         for (int i = 0; i < 3; i++)
            {
            streamTemp *= 2;
            if (streamTemp > 128)
               streamTemp = 1;
            if (landDir == streamTemp)
               {
               side = 1;
               break;
               }
            }
         if (side == -1)
            side = 2;
         break;
         }
         break;


      }  // end of:  switch( change )


   return side;

   }

int MapLayer::FlowDirToPower(int flowDirection) const
   {
   int power = -1;
   switch (flowDirection)
      {
      case 1:
         power = 0;
         break;
      case 2:
         power = 1;
         break;
      case 4:
         power = 2;
         break;
      case 8:
         power = 3;
         break;
      case 16:
         power = 4;
         break;
      case 32:
         power = 5;
         break;
      case 64:
         power = 6;
         break;
      case 128:
         power = 7;
         break;
      }
   return power;
   }



// FollowFlowPath() takes a cell coordinate is input, and returns the coordinate 
// of the next cell down the flow path, returning true if successful, false if we're off the
// DEM grid (nodata) or if the downstream neighbor
//
// NOTE:  Assumes this is a DEM derived flowdir grid!!!

bool MapLayer::FollowFlowPath(int &row, int &col) const
   {
   ASSERT(m_layerType == LT_GRID);
   // get the flow path direction for this cell

   int dir;
   bool ok = GetData(row, col, dir);
   if (!ok)
      return false;

   if (dir == GetNoDataValue())
      return false;

   ok = GetNeighborCoords(dir, row, col);

   return ok;
   }


// GetNeighborCoords() - takes a direction and a cell coord, returns the neighbors' cell coord
// if the neighbor cell exists - returns false if a bad direction is passed, or if the neighbor is 
// off the grid.

bool MapLayer::GetNeighborCoords(int direction, int &row, int &col) const
   {
   ASSERT(m_layerType == LT_GRID);

   switch (direction)
      {
      case N:     row--;         break;
      case NE:    row--; col++;  break;
      case E:            col++;  break;
      case SE:    row++; col++;  break;
      case S:     row++;         break;
      case SW:    row++; col--;  break;
      case W:            col--;  break;
      case NW:    row--; col--;  break;

      default: return false; // no direction data, return false with original coords unchanged
      }

   // take care of boundaries
   bool retVal = true;
   if (row < 0)
      {
      row = 0;
      retVal = false;
      }

   else if (row >= GetRecordCount())
      {
      row = GetRecordCount() - 1;
      retVal = false;
      }

   if (col < 0)
      {
      col = 0;
      retVal = false;
      }

   else if (col >= GetFieldCount())
      {
      col = GetFieldCount();
      retVal = false;
      }

   return retVal;
   }


// MapLayer::FindPourPoint( int &row, int &col, MapLayer *pStreamLayer, bool firstTime )
//
// Takes an initial guess, and follows down flow paths until a pour point is reached.
// pStreamLayer can be NULL, in which case the pour point is NOT checked to see if it is on the 
// river coverage
//
// NOTE:  Assumes this is a DEM coverage


bool MapLayer::FindPourPoint(int &row, int &col, MapLayer *pStreamLayer /*=NULL*/, bool firstTime /*=true*/) const
   {
   ASSERT(m_layerType == LT_GRID);

   // look for an existing pour point file
/*
   char file[ 128 ];
   lstrcpy( file, m_name );

   char *pExt = strrchr( file, '.' );
   if ( pExt == NULL )
      {
      lstrcat( file, "." );
      pExt = file + lstrlen( file );
      }
   else
      pExt++;

   lstrcpy( pExt, "pp" );
   FILE *fp = fopen(  file, "rt" );
   if ( fp != NULL )
      {
      int _row, _col;
      fscanf( fp, "%i %i", &_row, &_col );
      fclose( fp );
      row = _row;
      col = _col;
      return true;
      }
*/
// no pour point file founds, continue looking
   int counter = 0;

   // try to find a starting point
   if (firstTime)
      {
      int rows = GetRecordCount();
      int cols = GetFieldCount();

      for (row = rows / 2; row < rows; row++)
         for (col = cols / 2; col < cols; col++)
            {
            int dir;
            bool ok = GetData(row, col, dir);
            ASSERT(ok);

            if (dir != GetNoDataValue())
               goto next;
            }

      ASSERT(0);

   next:
      ;
      }
   int lastRow = row;
   int lastCol = col;

   // loop until FollowFlowPath() fails, indicating border or no-data flow cell found
   while (FollowFlowPath(row, col) && counter < 3000)
      {
      counter++;
      lastRow = row;
      lastCol = col;
      }

   if (counter > 2999)
      Report::WarningMsg("Counter exceeded in FindPourPoint(), aborting...");

   // restore the last coordinates before failure
   row = lastRow;
   col = lastCol;

   // make sure we're on a river cell
   bool found = true;

   if (pStreamLayer != NULL)
      {
      int river;
      bool ok = pStreamLayer->GetData(row, col, river);
      ASSERT(ok);

      found = (river != 0);
      }

   if (found)
      {
#ifndef NO_MFC
      PourPointDlg dlg;
      dlg.m_row = row;
      dlg.m_col = col;
      if (dlg.DoModal() == IDOK)
         {
         row = dlg.m_row;
         col = dlg.m_col;

         FindPourPoint(row, col, pStreamLayer, false);
         }
      else // IDCANCEL, accept pourpoint
#endif 
         {
         // write to disk
         char file[128];
         lstrcpy(file, m_name);

         char *pExt = strrchr(file, '.');
         if (pExt == NULL)
            {
            lstrcat(file, ".");
            pExt = file + lstrlen(file);
            }
         else
            pExt++;

         lstrcpy(pExt, "pp");
         FILE *fp;
         fopen_s(&fp, file, "wt");
         fprintf(fp, "%i %i", row, col);
         fclose(fp);
         }
      }
   else
      Report::ErrorMsg("Unable to find pour point...");

   return found;
   }



bool MapLayer::GetStreamIntersectionPoint(int &row, int &col, MapLayer *pStreamLayer) const
   {
   // goes downstream, looking for a branch point
   ASSERT(m_layerType == LT_GRID);

   while (pStreamLayer->IsCellInStream(row, col) == false)
      {
      bool ok = FollowFlowPath(row, col);

      if (!ok) // did we hit a border?
         return false;
      }

   return true;
   }


int MapLayer::FindUpstreamBranchPoints(int row, int col, MapLayer *pStreamLayer, RowColArray &upstreamBranchCoordArray) const
   {
   RowColArray upstreamCoordArray;

   int count = GetUpstreamCells(row, col, pStreamLayer, upstreamCoordArray);

   switch (count)
      {
      case 0:  // headwater, stop looking
         break;

      case 1:  // in a stream reach
         FindUpstreamBranchPoints(upstreamCoordArray[0].row, upstreamCoordArray[0].col, pStreamLayer, upstreamBranchCoordArray);
         break;

      default:
         {
         upstreamBranchCoordArray.Add(row, col);

         for (int i = 0; i < count; i++)
            FindUpstreamBranchPoints(upstreamCoordArray[i].row, upstreamCoordArray[i].col, pStreamLayer, upstreamBranchCoordArray);
         }
         break;

      }

   return upstreamBranchCoordArray.GetSize();
   }


int MapLayer::GetUpstreamCells(int row, int col, MapLayer *pStreamLayer, RowColArray &coordArray) const
   {
   // look in all directions for cells that are:
   //     1) upstream (flowing into this cell as determined by the directions layer)
   // and 2) are a river cell ( as determined by a non-zero, valid value in the river layer)
   coordArray.RemoveAll();

   static int dirArray[] = { NE, N, NW, W, SW, S, SE, E };

   // check cell in eac direction, add to array if necessary
   int _row, _col;

   for (int i = 0; i < 8; i++)
      {
      _row = row;
      _col = col;
      if (IsCellUpstream(dirArray[i], _row, _col, pStreamLayer))  // send coords of this cells, 
         coordArray.Add(_row, _col); // return coords of neighbor cell in the specified direction
      }

   return coordArray.GetSize();
   }

bool MapLayer::GetUpstreamCell(int row, int col, MapLayer *pStreamLayer, int &rowUp, int &colUp) const
   {
   // check cell in each direction, add to array if necessary
   int _row, _col;
   static int dirArray[] = { NE, N, NW, W, SW, S, SE, E };
   for (int i = 0; i < 8; i++)
      {
      _row = row;
      _col = col;
      if (IsCellUpstream(dirArray[i], _row, _col, pStreamLayer))  // send coords of this cells, 
         {
         rowUp = _row;
         colUp = _col;
         return true;
         }

      }
   return false;
   }


bool MapLayer::IsCellUpstream(int neighborDir, int &row, int &col, MapLayer *pStreamLayer) const
   {
   int myRow = row;
   int myCol = col;

   // get the coords of the neighbor cell in the specified direction
   if (GetNeighborCoords(neighborDir, row, col) == false)  // sends my coords, returns neighbor coords
      return false;  //  hit a border

   int neighborRow = row;
   int neighborCol = col;

   // next see if the neighbor cell is a river
   if (pStreamLayer->IsCellInStream(neighborRow, neighborCol) == false)
      return false;

   // it is in the river, check to see if the neighbor cell is flowing toward the starting cell
   row = neighborRow;
   col = neighborCol;
   FollowFlowPath(row, col);

   if (myRow == row && myCol == col)
      {
      row = neighborRow;
      col = neighborCol;
      return true;
      }
   else
      return false;
   }


bool MapLayer::IsCellInStream(int row, int col) const
   {
   int value;
   bool ok = GetData(row, col, value);
   ASSERT(ok);

   if (value == GetNoDataValue())
      return false;

   return (value > 0);      // okay for negative nodata values
   }

//---------------------------------------------------------------------------------
// MapLayer::IsCellUpslope()
//
// Determines if a cell in the specified direction is upslope.  If pStreamLayer != NULL,
// excludes cells in a stream from being considered upslope
//---------------------------------------------------------------------------------

bool MapLayer::IsCellUpslope(int neighborDir, int &row, int &col, MapLayer *pStreamLayer /*=NULL*/) const
   {
   int myRow = row;
   int myCol = col;

   // get the coords of the neighbor cell in the specified direction
   if (GetNeighborCoords(neighborDir, row, col) == false)  // sends my coords, returns neighbor coords
      return false;  //  hit a border

   int neighborRow = row;
   int neighborCol = col;

   // if its in the river, exclude it
   if (pStreamLayer != NULL && pStreamLayer->IsCellInStream(neighborRow, neighborCol))
      return false;

   // check to see if the neighbor cell is flowing toward the starting cell
   row = neighborRow;
   col = neighborCol;
   FollowFlowPath(row, col);

   if (myRow == row && myCol == col)
      {
      row = neighborRow;
      col = neighborCol;
      return true;
      }
   else
      return false;
   }


//-------------------------------------------------------------------------------------
// MapLayer::GetUpslopeCells()
//
// Get all upslope cell immediate adjacent to the cell at row, col.
//
// If pStreamLayer != NULL, exclude any  cells that are in a stream
//-------------------------------------------------------------------------------------

int MapLayer::GetUpslopeCells(int row, int col, RowColArray &coordArray, MapLayer *pStreamLayer /*=NULL*/) const
   {
   // look in all directions for cells that are:
   //     1) upstream (flowing into this cell as determined by the directions layer)
   coordArray.RemoveAll();

   static int dirArray[] = { NE, N, NW, W, SW, S, SE, E };

   // check cell in each direction, add to array if necessary
   int _row, _col;

   for (int i = 0; i < 8; i++)
      {
      _row = row;
      _col = col;
      if (IsCellUpslope(dirArray[i], _row, _col, pStreamLayer))  // send coords of this cells, 
         coordArray.Add(_row, _col); // return coords of neighbor cell in the specified direction
      }

   return coordArray.GetSize();
   }


bool MapLayer::GetUpslopeCell(int row, int col, int &rowUp, int &colUp, MapLayer *pStreamLayer /*=NULL*/) const
   {
   // look in all directions for cells that are:
   //     1) upstream (flowing into this cell as determined by the directions layer)

   static int dirArray[] = { NE, N, NW, W, SW, S, SE, E };

   // check cell in each direction, add to array if necessary
   int _row, _col;

   for (int i = 0; i < 8; i++)
      {
      _row = row;
      _col = col;
      if (IsCellUpslope(dirArray[i], _row, _col, pStreamLayer))  // send coords of this cells, 
         {
         rowUp = _row;
         colUp = _col;
         return true;
         }
      }
   return false;

   }

//--------------------------------------------------------------------------------------
// MapLayer::DelineateWatershedFromPoint()
//
// delineate the subwatersheds, based on a pour point
// Note:  "this" layer must be a DEM layer
//--------------------------------------------------------------------------------------

int MapLayer::DelineateWatershedFromPoint(int row, int col, MapLayer *pWatershedLayer, int watershedID) const
   {
   ASSERT(pWatershedLayer->GetType() == LT_GRID);

   // start at row, col, tag all points up stream
   RowColArray coordArray;

   int count = GetUpslopeCells(row, col, coordArray);

   if (count == 0)
      return 0;

   else
      {
      int upCount = 0;

      for (int i = 0; i < upCount; i++)
         {
         ROW_COL &cell = coordArray[i];

         pWatershedLayer->SetData(cell.row, cell.col, watershedID);

         // recurse for rest of watershed
         upCount += DelineateWatershedFromPoint(cell.row, cell.col, pWatershedLayer, watershedID);
         }

      count += upCount;
      }

   return count;
   }


//--------------------------------------------------------------------------------------
// MapLayer::DelineateWatershedsFromStream()
//
// Delineate ALL watersheds based on stream reach, as defined by branch points
//
//  returns total number of cell mapped into watersheds, with watershedID incremented
//  by the number of watersheds delineated
//--------------------------------------------------------------------------------------

int MapLayer::DelineateWatershedsFromStream(int startRow, int startCol, MapLayer *pWatershedLayer,
   MapLayer *pStreamLayer, int &watershedID) const
   {
   ASSERT(pWatershedLayer != NULL);
   ASSERT(pStreamLayer != NULL);
   ASSERT(pWatershedLayer->GetType() == LT_GRID);
   ASSERT(pStreamLayer->GetType() == LT_GRID);

   // start at pourRow, pourCol, tag all points upslope (but not upstream)
   RowColArray coordArray;

   int countFromHere = 0;

   // find all points outside of stream that are upslope from this point
   int upCount = GetUpslopeCells(startRow, startCol, coordArray, pStreamLayer);

   // for each one, get all upstream cells
   for (int i = 0; i < upCount; i++)
      countFromHere += DelineateWatershedFromPoint(coordArray[i].row, coordArray[i].col, pWatershedLayer, watershedID);

   // repeat for next upstream point, looking for terminals and branch points
   upCount = GetUpstreamCells(startRow, startCol, pStreamLayer, coordArray);

   switch (upCount)
      {
      case 0:     // terminal cell in stream
         return countFromHere;

      case 1:     // still on this stream segment, delineate with same watershedID
         {
         int count = DelineateWatershedsFromStream(coordArray[0].row, coordArray[0].col,
            pWatershedLayer, pStreamLayer, watershedID);
         countFromHere += count;
         return countFromHere;
         }

      default: // more than one upstream cell, so we're at a branch point in the stream coverage.
               // delineate each upstream reach with an incremented watershedID
         {
         int count = 0;
         for (int i = 0; i < upCount; i++)
            {
            count += DelineateWatershedsFromStream(coordArray[i].row, coordArray[i].col,
               pWatershedLayer, pStreamLayer, ++watershedID);
            }
         countFromHere += count;
         return countFromHere;
         }
      }
   }



int MapLayer::DelineateStreams(float maxUpslopeArea, MapLayer *pStreamLayer)
   {
   ASSERT(pStreamLayer != NULL);
   ASSERT(pStreamLayer->GetType() == LT_GRID);

   // first, locate the pour point.  initial guess is in middle of the grid
   int row = GetRowCount() / 2;
   int col = GetColCount() / 2;

   bool found = FindPourPoint(row, col);
   ASSERT(found);

   DelineateStreamsFromPoint(row, col, maxUpslopeArea, pStreamLayer);

   pStreamLayer->ClearAllBins();
   pStreamLayer->SetBinColorFlag(BCF_GRAY_INCR);
   float minVal;
   float maxVal;
   pStreamLayer->GetGridDataMinMax(&minVal, &maxVal);
   pStreamLayer->SetBins(USE_ACTIVE_COL, minVal, maxVal, (int)(maxVal - minVal) + 1, TYPE_FLOAT);
   pStreamLayer->ClassifyData();

   return 1;
   }


int MapLayer::DelineateStreamsFromPoint(int row, int col, float maxUpslopeArea, MapLayer *pStreamLayer,
   int branchCount /*=0*/)
   {
   // set stream coverage to zero/nodata values
   int rows = GetRowCount();
   int cols = GetColCount();

   if (branchCount == 0)
      {
      branchCount++;
      for (int i = 0; i < rows; i++)
         for (int j = 0; j < cols; j++)
            {
            int value;
            GetData(i, j, value);

            if (value == (int)GetNoDataValue())
               pStreamLayer->SetNoData(i, j);
            else
               pStreamLayer->SetData(i, j, 0);
            }
      }

   // have pour point, now start working upstream
   RowColArray coordArray;

   // find all points that are upslope from this point
   int upCount = GetUpslopeCells(row, col, coordArray);

   if (upCount == 0)
      return 0;

   CArray <int, int& > streamCells;

   for (int i = 0; i < upCount; i++)
      {
      // iterate throught each upslope cell, looking for sufficient drainage area
      row = coordArray[i].row;
      col = coordArray[i].col;

      float upslopeArea = GetTotalUpslopeArea(row, col);

      // mark this cell as a stream if there is sufficient upstream area
      if (upslopeArea > maxUpslopeArea)
         streamCells.Add(i);
      }

   if (streamCells.GetSize() > 1)
      branchCount++;

   for (int i = 0; i < streamCells.GetSize(); i++)
      {
      row = coordArray[streamCells[i]].row;
      col = coordArray[streamCells[i]].col;

      pStreamLayer->SetData(row, col, 1);

      upCount += DelineateStreamsFromPoint(row, col, maxUpslopeArea, pStreamLayer, branchCount);
      }

   return upCount;
   }


int MapLayer::GetTotalUpslopeCells(int row, int col) const
   {
   RowColArray coordArray;

   int upCount = GetUpslopeCells(row, col, coordArray);
   int size = coordArray.GetSize();
   ASSERT(upCount == size);

   int count = upCount;

   for (int i = 0; i < upCount; i++)
      count += GetTotalUpslopeCells(coordArray[i].row, coordArray[i].col);

   return count;
   }



float MapLayer::GetTotalUpslopeArea(int row, int col) const
   {
   int upCount = GetTotalUpslopeCells(row, col);

   return upCount * (m_cellWidth * m_cellHeight);
   }

/*

  //This is John's original DEMtoFlowDir - kellie manipulated it below.  To return to the old version
  //uncomment this code (and commment out the version below)


int MapLayer::DEMtoFlowDir( MapLayer *pFlowLayer )
   {
   ASSERT( m_layerType == LT_GRID );
   ASSERT( pFlowLayer != NULL );
   ASSERT( pFlowLayer->GetType() == LT_GRID );

   // iterate through each cell, determining flow direction based on neighbor elevations
   int rows = GetRowCount();
   int cols = GetColCount();

   for ( int row=0; row < rows; row++ )
      {
      for ( int col=0; col < cols; col++ )
         {
         int dir = GetCellFlowDir( row, col );

         if ( dir < 0 )
            pFlowLayer->SetNoData( row, col );
         else
            pFlowLayer->SetData( row, col, dir );

         }  // end of:  for ( col < cols )
      }  // end of:  for ( row < rows )

   // entire grid computed, set the legend and go...
   const int a=30;
   pFlowLayer->ClearBins();
   pFlowLayer->AddBin( RGB( 0, 0, 0 ), "0", BT_STRING, 0.0f, 0.1f );
   pFlowLayer->AddBin( RGB( 110-a, 110-a, 170-50 ), "E", BT_STRING, 0.5f, 1.5f );
   pFlowLayer->AddBin( RGB( 130-a, 130-a, 180-50 ), "SE", BT_STRING, 1.51f, 2.5f );
   pFlowLayer->AddBin( RGB( 150-a, 150-a, 190-50 ), "S", BT_STRING, 3.0f, 5.0f );
   pFlowLayer->AddBin( RGB( 170-a, 170-a, 200-50 ), "SW", BT_STRING, 7.0f, 9.0f );
   pFlowLayer->AddBin( RGB( 190-a, 190-a, 210-50 ), "W", BT_STRING, 15.0f, 17.0f );
   pFlowLayer->AddBin( RGB( 210-a, 210-a, 220-50 ), "NW", BT_STRING, 31.0f, 33.0f );
   pFlowLayer->AddBin( RGB( 230-a, 230-a, 230-50 ), "N", BT_STRING, 63.0f, 65.0f );
   pFlowLayer->AddBin( RGB( 250-a, 250-a, 240-50 ), "NE", BT_STRING, 127.0f, 129.0f );

   return 1;
   }
*/

//--------------------------------------------------------------------------------------
// MapLayer::DEMtoFlowDir()
//
// Calculate a flow direction grid and estimate elevationally undefined flow directions
//
//  
//--------------------------------------------------------------------------------------
int MapLayer::DEMtoFlowDir(MapLayer *pFlowLayer)
   {
   ASSERT(m_layerType == LT_GRID);
   ASSERT(pFlowLayer != NULL);
   ASSERT(pFlowLayer->GetType() == LT_GRID);

   // iterate through each cell, determining flow direction based on neighbor elevations
   int rows = GetRowCount();
   int cols = GetColCount();

   for (int row = 0; row < rows; row++)
      {
      for (int col = 0; col < cols; col++)
         {
         int dir = GetCellFlowDir(row, col);

         if (dir < 0)
            pFlowLayer->SetNoData(row, col);
         else
            pFlowLayer->SetData(row, col, dir);

         }  // end of:  for ( col < cols )
      }  // end of:  for ( row < rows )


   // now go back through the newly created flow dir grid to look for 0 values (which are flat
   // areas with undefined flow direction.)  Flow directions for these flat areas are taken from
   // neighboring cells with defined flow directions.  (see FlatAreaFlowDir)


   //John - it would be nice to calculate this all in a single loop, but we need to have all
   //the flow directions calculated first, so I could not see how to do it.  Anyway, it executes
   //quickly all the same.  If you see a better way, let me know.


   int dir;
   int   dirEstimate;
   for (int _row = 0; _row < rows; _row++)
      {
      for (int _col = 0; _col < cols; _col++)
         {

         int  ok = pFlowLayer->GetData(_row, _col, dir); //Get the previously calculated
                                                           //flow directions
         ASSERT(ok);

         if (dir == 0)                                    //If flow dir is undefined
            {
            dirEstimate = FlatAreaFlowDir(_row, _col, pFlowLayer);  //make an estimate
            pFlowLayer->SetData(_row, _col, dirEstimate);  //and reset the undefined
            }                                                //lue to the estimate

         }  // end of:  for ( col < cols )
      }  // end of:  for ( row < rows )

   // entire grid computed, set the legend and go...
   const int a = 30;
   pFlowLayer->ClearAllBins();
   pFlowLayer->AddBin(0, RGB(0, 0, 0), "0", TYPE_STRING, 0.0f, 0.1f);
   pFlowLayer->AddBin(0, RGB(110 - a, 110 - a, 170 - 50), "E", TYPE_STRING, 0.5f, 1.5f);
   pFlowLayer->AddBin(0, RGB(130 - a, 130 - a, 180 - 50), "SE", TYPE_STRING, 1.51f, 2.5f);
   pFlowLayer->AddBin(0, RGB(150 - a, 150 - a, 190 - 50), "S", TYPE_STRING, 3.0f, 5.0f);
   pFlowLayer->AddBin(0, RGB(170 - a, 170 - a, 200 - 50), "SW", TYPE_STRING, 7.0f, 9.0f);
   pFlowLayer->AddBin(0, RGB(190 - a, 190 - a, 210 - 50), "W", TYPE_STRING, 15.0f, 17.0f);
   pFlowLayer->AddBin(0, RGB(210 - a, 210 - a, 220 - 50), "NW", TYPE_STRING, 31.0f, 33.0f);
   pFlowLayer->AddBin(0, RGB(230 - a, 230 - a, 230 - 50), "N", TYPE_STRING, 63.0f, 65.0f);
   pFlowLayer->AddBin(0, RGB(250 - a, 250 - a, 240 - 50), "NE", TYPE_STRING, 127.0f, 129.0f);
   pFlowLayer->AddNoDataBin(0);

   return 1;
   }


// returns -1 for nodata, 0 for pit or flat area, > 0 for direction
int MapLayer::GetCellFlowDir(int row, int col) const
   {
   float myElev, distance, test;

   // start out with this cell as the lowest cell
   bool ok = GetData(row, col, myElev);
   ASSERT(ok);

   if (myElev == m_noDataValue)
      return -1;

   int lowestDir = 0;
   float lowestTest = 0.0f;
   //bool onEdge = false;

   for (int dir = E; dir <= NE; dir *= 2)
      {
      if (dir == E || dir == S || dir == W || dir == N)

         distance = 1.0f;
      else
         distance = 1.41214f;

      int _row = row;
      int _col = col;

      ok = GetNeighborCoords(dir, _row, _col);

      if (!ok)  // off grid, ignore it
         continue;

      float neighborElev;
      ok = GetData(_row, _col, neighborElev);
      ASSERT(ok);
      test = (myElev - neighborElev) / distance;
      if (neighborElev == m_noDataValue)
         {
         //onEdge = true;
         if (lowestDir == 0)
            lowestDir = dir;
         }
      else //not on edge
         //filling the DEM still leaves the cells undefined.  We need to make a guess as to where
         //the water will go when two or more adjacent cells have equal elevation (and the rest of the 
         //adjacent cells are higher) J&D provide what seems like reasonable soln.  
         //I partially implemented this as MapLayer::FlatAreaFlowDir (see below).
         if (test > lowestTest)
            {
            // Jenson Domingue suggest a distance weighted drop as opposed to a simple drop
            // It makes sense.  Divide the drop by the distance from centroids (1 for
            // a non-corner, root2 for a corner)  This is why we are testing with 'test' rather
            // my myElev and neighborElev.

            lowestTest = test;
            lowestDir = dir;

            }

      }  // end of:  for ( dir < 129 )  and we know the lowest elevation neighbor

   // did we find a downstream cell?
   return lowestDir;  //return the direction of the flow path (1 through 128)
   }


//--------------------------------------------------------------------------------------
// MapLayer::FlatAreaFlowDir
//
// Calculate a flow directions for cells that have no neighbors with lower elevations.  The 
// standard method cannot calculate a flow direction for this case (it's elevationally undefined)
// This method looks for neighbor cells with defined flow directions and arbitrarily assigns
// the neighbors flow direction to the undefined cell.  In this way, flow directions are calculated
// for large flat areas, moving from the borders of the areas inward.  

// It is an arbitrary solution, taken from Jenson and Domingue. The key is that water will 
// continue to move downslope.
//--------------------------------------------------------------------------------------

int MapLayer::FlatAreaFlowDir(int row, int col, MapLayer *pFlowLayer)
   {
   int myDir, dir, neighborDir;
   // start out with this cell as the lowest cell
   bool ok = GetData(row, col, myDir);
   ASSERT(ok);

   for (dir = E; dir <= NE; dir *= 2)
      {
      int _row = row;
      int _col = col;

      ok = pFlowLayer->GetNeighborCoords(dir, _row, _col);

      ok = pFlowLayer->GetData(_row, _col, neighborDir);
      ASSERT(ok);

      if (neighborDir > 0)
         {
         myDir = neighborDir;
         }


      }  // end of:  for ( dir < 129 )  and we know the lowest elevation neighbor

   // did we find a downstream cell?
   return myDir;  //return the direction of the flow path (1 through 128)
   }


//--------------------------------------------------------------------------------------
// MapLayer::DEMtoFilledDEM
//
// This method is the loop structure used to fill pits using the method FillDEMCell
//--------------------------------------------------------------------------------------

int MapLayer::DEMtoFilledDEM(MapLayer *pFilledDEMLayer)
   {
   ASSERT(m_layerType == LT_GRID);
   ASSERT(pFilledDEMLayer != NULL);
   ASSERT(pFilledDEMLayer->GetType() == LT_GRID);

   // iterate through each cell, determining flow direction based on neighbor elevations
   int rows = GetRowCount();
   int cols = GetColCount();

   for (int row = 0; row < rows; row++)
      {
      for (int col = 0; col < cols; col++)
         {
         float fillElev = FillDEMCell(row, col);
         pFilledDEMLayer->SetData(row, col, fillElev);
         }  // end of:  for ( col < cols )
      }  // end of:  for ( row < rows )


   return 1;
   }


// returns -1 for nodata, 0 for pit or flat area, > 0 for direction

//--------------------------------------------------------------------------------------
// MapLayer::FillDEMCell
//
// Searches for sinks and raises them to the lowest neighboring elevation.
//  
//--------------------------------------------------------------------------------------

float MapLayer::FillDEMCell(int row, int col)
   {
   float myElev;
   float fillElev = 0;
   // start out with this cell as the lowest cell
   bool ok = GetData(row, col, myElev);
   ASSERT(ok);

   if (myElev == m_noDataValue)
      return m_noDataValue;

   for (int dir = E; dir <= NE; dir *= 2)
      {
      int _row = row;
      int _col = col;

      ok = GetNeighborCoords(dir, _row, _col);

      if (!ok)  // off grid, ignore it
         continue;

      float neighborElev;
      ok = GetData(_row, _col, neighborElev);
      ASSERT(ok);

      if (fillElev == 0)
         fillElev = neighborElev;

      if (neighborElev <= myElev) //the cell is NOT a sink

         return myElev;             //so return the original elevation

      else                          //The cell IS a sink
         {
         if (fillElev > neighborElev)  //Check for the LOWEST neighbor elevation.


            fillElev = neighborElev;// Raise cell elevation to the lowest neighboring elevation

         }

      }  // end of:  for ( dir < 129 )  and we know the lowest elevation neighbor

   // did we find a downstream cell?
   return fillElev;  //return the direction of the flow path (1 through 128)
   }


int MapLayer::FindAdjacentRegion(int row, int col, RowColArray &coordArray) const
   {
   ASSERT(m_layerType == LT_GRID);

   float myValue;
   bool ok = GetData(row, col, myValue);

   // no data?
   if (myValue == m_noDataValue)
      return 0;

   for (int dir = E; dir <= NE; dir *= 2)
      {
      int _row = row;
      int _col = col;

      ok = GetNeighborCoords(dir, _row, _col);

      if (!ok)  // off grid, ignore it
         continue;

      float neighborValue;
      ok = GetData(row, col, neighborValue);
      ASSERT(ok);

      if (myValue == neighborValue)  // do the have the same attribute?
         {
         // have we already visted this cell?
         int index = coordArray.Find(_row, _col);

         if (index < 0)  // not in list already
            {
            coordArray.Add(_row, _col);
            FindAdjacentRegion(_row, _col, coordArray);
            }
         }
      }  // end of:  for ( all directions )

   return coordArray.GetSize();
   }



// NOT IMPLEMENTED!!!
/*
int MapLayer::GridToPoly( MapLayer *pPolyLayer )
   {

   ASSERT( m_layerType == LT_GRID );

   // reinitialize polygon layer
   pPolyLayer->m_layerType = LT_POLYGON;
   pPolyLayer->m_pPolyArray->RemoveAll();
   //pPolyLayer->m_binArray = m_binArray;

   if ( pPolyLayer->m_pDbTable != NULL )
      delete pPolyLayer->m_pDbTable;

   pPolyLayer->m_pDbTable = NULL;
   pPolyLayer->m_pData    = NULL;

   pPolyLayer->m_activeField = 0;   // column in data object
   pPolyLayer->m_isVisible = true;

   pPolyLayer->m_vxMin = m_vxMin;
   pPolyLayer->m_vxMax = m_vxMax;
   pPolyLayer->m_vyMin = m_vyMin;
   pPolyLayer->m_vyMax = m_vyMax;
   pPolyLayer->m_vzMin = m_vzMin;
   pPolyLayer->m_vzMax = m_vzMax;
   pPolyLayer->m_vExtent = m_vExtent;

   pPolyLayer->m_selection.RemoveAll();

   // make a temporary int matrix to designate what cells have already been vectorized
   int rows = GetRowCount();
   int cols = GetColCount();
   int id = 0;

   // NOTE:  ASSUMES INTEGER GRID!!!
   IntMatrix *visited = new IntMatrix( rows, cols, -1 );
   int value;
   int noDataValue = (int) GetNoDataValue();

   for ( int row=0; row < rows; row++ )
      {
      for ( int col=0; col < cols; col++ )
         {
         if ( visited->Get( row, col ) >= 0 )  // already visited? then skip it.
            continue;

         bool ok = GetData( row, col, value );
         ASSERT( ok );

         if ( value == noDataValue )         // nothing there? then skip it
            {
            visited->Set( row, col, 0 );
            continue;
            }

         // all set to start a new polygon....
         Poly *pPoly = new Poly;
         pPoly->m_id = id++;

         // is it a single pixel?
         int dir = 1;
         bool singlePixel = true;

         // E  = 1; S  = 4; W  = 16; N  = 64;
         do {
            int nrow = row;
            int ncol = col;
            if ( GetNeighborCoords( dir, nrow, ncol ) > 0 )
               {
               int nvalue;
               GetData( nrow, ncol, nvalue );
               if ( nvalue != value )
                  {
                  singlePixel = false;
                  break;
                  }
               }

            dir *= 4;
            } while ( dir < 128 );

         if ( singlePixel )
            {
            float x, y;
            GetGridCellCenter( row, col, x, y );
            x -= m_cellWidth/2;
            y -= m_cellHeight/2;
            pPoly->AddVertex( Vertex( x, y ) );                            // upper left?
            pPoly->AddVertex( Vertex( x+m_cellWidth, y ) );                // upper right?
            pPoly->AddVertex( Vertex( x+m_cellWidth, y+m_cellHeight ) );   // lower right?
            pPoly->AddVertex( Vertex( x, y+m_cellHeight ) );               // lower left ?
            pPoly->AddVertex( Vertex( x, y ) );                            // upper left? (closes poly)
            continue;
            }

         // not a single pixel, find the region border
         CUIntArray chain;    // hold direction chain for border  N, S, E, W
         int p0Row = row;
         int p0Col = col;
         int p1Row, p1Col, pn1Row, pn1Col, pnRow, pnCol;

         int prevDir = 3;   // previous move direction from previous border element to the current border element
         int nextDir = prevDir;
         int count   = 0;

         // search 3x3 neighborhood in an anticlockwise direction, beginning the search in the pixel
         // positioned in the direction (dir+3) mod 4

loop:
         if ( count == 1 )
            {
            p1Row = row;
            p1Col = col;


         // get anticlockwise direction (based on 4-connectivity)
         for ( int i=0; i < 4; i++ )
            {
            ASSERT( i != 3 );    // don't look more than three directions
            nextDir = (nextDir+3) % 4; // move anti-clockwise

            switch( nextDir )
               {
               case 0:  dir = E; break;
               case 1:  dir = N; break;
               case 2:  dir = W; break;
               case 3:  dir = S; break;
               default: ASSERT( 0 );
               }

            int nrow = row;
            int ncol = col;
            if ( GetNeighborCoords( dir, nrow, ncol ) )
               {
               int nvalue;
               GetData( nrow, ncol, nvalue );
               if ( nvalue == value )     // found a neighbor on the border in this region
                  {
                  prevDir = nextDir;      // update direction path
                  chain.Add( dir );       // add direction to chain
                  break;
                  }
               }
            }  // end of: for ( i < 4 )

         // if the current boundary element is the same as the second border element




         ExtendGridPoly( pPoly, row, col, value );

         {
         //-- add a vertex to the polygon --//
         //pPoly->AddVertex( v );

         //if ( v.x > m_vxMax ) m_vxMax = v.x;     // check bounds
         //if ( v.y > m_vyMax ) m_vyMax = v.y;
         //if ( v.x < m_vxMin ) m_vxMin = v.x;
         //if ( v.y < m_vyMin ) m_vyMin = v.y;
         }  // end of: while buffer != 'E' (adding additional vertexes)

         m_pPolyArray->Add( pPoly );  // add the polygon to the Map's collection
         }
      }

   delete visited;


   if ( ( m_vxMax-m_vxMin ) > ( m_vyMax - m_vyMin ) )
      m_vExtent = m_vxMax - m_vxMin;
   else
      m_vExtent = m_vyMax - m_vyMin;

   InitPolyLogicalPoints();   // create logical points for this set


   return 0;
   }
*/

void MapLayer::ExtendGridPoly(Poly *pPoly, int row, int col, int /*value*/)
   {

   // the fact that we are here implies adding the UL vertex.
   Vertex v;
   REAL x, y;
   GetGridCellCenter(row, col, x, y);
   v.x = x - m_cellWidth;
   v.y = y - m_cellHeight;
   pPoly->AddVertex(v);   // add UL


   }


//------------------------------------------------------------------------------
// MapLayer::PopulateNearestDistanceToPoly() 
//
// this method iterates through this layer's polygons, getting the distance from
// each of these polygons to the "To" polygons passed in, and stores it in the
// specified column
//------------------------------------------------------------------------------

int MapLayer::PopulateNearestDistanceToPoly(Poly *pToPoly, int col, REAL threshold, LPCTSTR label /*=NULL*/)
   {
   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON)
      return -1;

   if (col < 0)
      return -3;

   int width, decimals;
   GetTypeParams(TYPE_FLOAT, width, decimals);
   SetFieldType(col, TYPE_FLOAT, width, decimals, false);
   if (label != NULL)
      SetFieldLabel(col, label);

   int count = GetRecordCount(MapLayer::ALL);

   for (int i = 0; i < count; i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);
      float distance = pPoly->NearestDistanceToPoly(pToPoly, threshold);

      SetData(i, col, distance);
      }

   return count;
   }



int MapLayer::PopulateFlowPathDistance(MapLayer *pFlowDir, MapLayer *pStreamGrid, int method)
   {
   //  The watershed layer should be a polygon.  For each flowpathdistance (a grid), we need to 1.  Determine which
   // subbasin it is in and 2. sum up that distance (and count the # of grid cells in the watershed)
   int polygonCount = GetRecordCount(MapLayer::ALL);
   //FDataObj *basinFPDistance = new FDataObj;
   for (int i = 0; i < polygonCount; i++)
      {
      CArray <float, float& > fpDist;
      Poly *pThisPoly = m_pPolyArray->GetAt(i);
      float flowPathDistance = 0.0f;
      int flowPathCount = 0;
      int rows = pFlowDir->GetRowCount();
      int cols = pFlowDir->GetColCount();
      for (int row = 0; row < rows; row++)
         {
         for (int col = 0; col < cols; col++)
            {
            REAL x = 0;
            REAL y = 0;
            pFlowDir->GetGridCellCenter(row, col, x, y);
            Vertex point;
            point.x = x;
            point.y = y;
            if (pThisPoly->IsPointInPoly(point))
               {

               float distance = pFlowDir->GetDownFlowPathDistance(row, col, pStreamGrid);
               if (method == 1)// we just want the average fpdistance
                  {
                  flowPathDistance += distance;
                  flowPathCount++;
                  }
               if (method == 2)// we want to maintain the entire distribution
                  {
                  fpDist.Add(distance);
                  }
               }
            }  // end of:  for ( col < cols )
         }  // end of:  for ( row < rows )
//      basinFPDistance.Add(fpDist);

      float averageFPDistance = flowPathDistance / flowPathCount;
      m_pMap->Notify(NT_CALCDIST, i, polygonCount);
      SetData(i, m_pData->GetCol("EXTRA_1"), averageFPDistance);
      } // end of this polygon, now go on to the next one!
//   basinFPDistance.WriteAscii("c:\\temp\\fpdist.csv");
 //  delete basinFPDistance; 
   return 1;
   }




//------------------------------------------------------------------------------
// MapLayer::PopulateNearestDistanceToCoverage() 
//
// this method iterates through this layer's polygons, getting the distance from
// each of these polygons to the closest polygon in the "To" layer passed in,
// and stores it in the specified column
//------------------------------------------------------------------------------

int MapLayer::PopulateNearestDistanceToCoverage(MapLayer *pToLayer, int col, REAL threshold, Query *pToQuery /*= NULL*/, Query *pThisQuery/*=NULL*/, LPCTSTR label/*=NULL*/, int colNearestToIndex /*=-1*/, int colNearestToValue /*=-1*/, int colNearestSourceValue /*=-1*/)
   {
   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON && m_layerType != LT_POINT)
      return -1;

   if (pToLayer->GetType() != LT_LINE && pToLayer->GetType() != LT_POLYGON && pToLayer->GetType() != LT_POINT)
      return -2;

   if (col < 0)
      return -3;

   int width, decimals;
   GetTypeParams(TYPE_FLOAT, width, decimals);
   SetFieldType(col, TYPE_FLOAT, width, decimals, false);
   if (label != NULL)
      SetFieldLabel(col, label);

   int thisCount = GetPolygonCount(MapLayer::ALL);

   int toCount = pToLayer->GetPolygonCount(MapLayer::ALL);

   int iCPU = omp_get_num_procs();
   omp_set_num_threads(iCPU);
   int computedSoFar = 0;

   // iterate through each polygon in "this" coverage, getting nearest distantance in "to" coverage

   //#pragma omp parallel for
   for (int i = 0; i < thisCount; i++)
      {
      int thisPolyIndex = i;

      float nearestDistance = 1.0e16f;

      Poly *pThisPoly = m_pPolyArray->GetAt(i);

      if (pThisQuery != NULL)
         {
         bool result = false;
         bool ok = pThisQuery->Run(i, result);

         if (ok == false || result == false)
            {
#pragma omp atomic
            computedSoFar++;
            continue;  // skip is this polygon not in query set
            }
         }

      int nearestIndex = -1;
      VData nearestValue;

      // look to each poly in the "to" coverage
      bool abortLoop = false;
      //      #pragma omp parallel for
      for (int j = 0; j < toCount; j++)
         {
         // #pragma omp flush (abortLoop)
         if (!abortLoop)
            {
            float distance = nearestDistance;

            Poly *pToPoly = pToLayer->GetPolygon(j);

            if (pToQuery != NULL)
               {
               bool result = false;
               bool ok = pToQuery->Run(i, result);

               if (ok == false || result == false)
                  continue;  // skip if this polygon not in query set
               }

            if (IsPolyInMap(pToPoly))
               distance = pThisPoly->NearestDistanceToPoly(pToPoly, threshold);   // note: Doesn't use spatial index

            if (distance < nearestDistance)
               {
               nearestDistance = distance;
               nearestIndex = j;
               }

            if (nearestDistance <= 0.0f)   // no need to keep going...
               break;
            //{
            //abortLoop = true;
            //#pragma omp flush (abortLoop)
            //}
            }  // end of inner (j) loop

         SetData(thisPolyIndex, col, nearestDistance);

         if (colNearestToIndex >= 0)
            SetData(thisPolyIndex, colNearestToIndex, nearestIndex);

         if (colNearestToValue >= 0)
            {
            VData nearestValue;
            pToLayer->GetData(j, colNearestSourceValue, nearestValue);
            SetData(thisPolyIndex, colNearestToValue, nearestValue);
            }
         }  // end of inner (j) loop

      m_pMap->Notify(NT_CALCDIST, computedSoFar, thisCount);
#pragma omp atomic
      computedSoFar++;
      }  //end of outer (i) loop

   return thisCount;
   }


//------------------------------------------------------------------------------
// MapLayer::PopulateNumberNearbyPolys() 
//
// this method iterates through this layer's polygons, the number of polygons
// that are within a threshold distance
//------------------------------------------------------------------------------

int MapLayer::PopulateNumberNearbyPolys(MapLayer *pToLayer, int col, REAL threshold, Query *pToQuery /*=NULL*/, Query *pThisQuery/*=NULL*/, LPCTSTR label /*=NULL*/, REAL nearThreshold /*=-1*/)
   {
   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON && m_layerType != LT_POINT)
      return -1;

   if (pToLayer->GetType() != LT_LINE && pToLayer->GetType() != LT_POLYGON && pToLayer->GetType() != LT_POINT)
      return -2;

   if (col < 0)
      return -3;

   int width, decimals;
   GetTypeParams(TYPE_FLOAT, width, decimals);
   SetFieldType(col, TYPE_FLOAT, width, decimals, false);
   if (label != NULL)
      SetFieldLabel(col, label);

   int thisCount = GetPolygonCount(MapLayer::ALL);
   int toCount = pToLayer->GetPolygonCount(MapLayer::ALL);

   int iCPU = omp_get_num_procs();
   omp_set_num_threads(iCPU);
   int computedSoFar = 0;
   float distance = 0.0f;
   // iterate through each polygon in "this" coverage, getting nearest distantance in "to" coverage

#pragma omp parallel for
   for (int i = 0; i < thisCount; i++)
      {
      int thisPolyIndex = i;
      Poly *pThisPoly = m_pPolyArray->GetAt(i);

      if (pThisQuery != NULL)
         {
         bool result = false;
         bool ok = m_pQueryEngine->Run(pThisQuery, i, result);

         if (ok == false || result == false)
            continue;  // skip is this polygon not in query set
         }

      int nearestIndex = -1;
      int numberNearPolys = 0;
      // look to each poly in the "to" coverage
#pragma omp parallel for reduction(+:numberNearPolys)
      for (int j = 0; j < toCount; j++)
         {
         Poly *pToPoly = pToLayer->GetPolygon(j);

         if (pToQuery != NULL)
            {
            bool result = false;
            bool ok = pToLayer->m_pQueryEngine->Run(pThisQuery, i, result);

            if (ok == false || result == false)
               continue;  // skip is this polygon not in query set
            }

         if (IsPolyInMap(pToPoly))
            {
            distance = pThisPoly->NearestDistanceToPoly(pToPoly, threshold);
            if (distance < nearThreshold)
               numberNearPolys++;
            }
         }  // end of inner (j) loop
      //m_pMap->Notify( NT_CALCDIST, computedSoFar++, thisCount );
      SetData(thisPolyIndex, col, numberNearPolys);
      }  // end of outer (i) loop
   return thisCount;
   }


//------------------------------------------------------------------------------
// MapLayer::PopulateNearestDistanceToSelectedCoverage() 
//
// this method iterates through this layer's polygons, getting the distance from
// each of these polygons to the closest polygon in the "To" layer passed in,
// and stores it in the specified column.  The search is restricted to "To" polys
// that have the speified attribute value in the specified column
//------------------------------------------------------------------------------

/*

int MapLayer::PopulateNearestDistanceToSelectedCoverage( MapLayer *pToLayer, int col, REAL threshold, int colToSelectFrom, VData &value, LPCTSTR label  )
   {
   if ( m_layerType != LT_LINE && m_layerType != LT_POLYGON )
      return -1;

   if ( pToLayer->GetType() != LT_LINE && pToLayer->GetType() != LT_POLYGON )
      return -2;

   if ( col < 0 )
      return -3;

   SetFieldType( col, TYPE_FLOAT );
   if ( label != NULL )
      SetFieldLabel( col, label );

   int thisCount = GetRecordCount( MapLayer::ALL );
   int toCount   = pToLayer->GetRecordCount( MapLayer::ALL );

   for ( int i=0; i < thisCount; i++ )
      {
      float nearestDistance = 1.0e16f;
      Poly *pThisPoly = m_pPolyArray->GetAt( i );

      for ( int j=0; j < toCount; j++ )
         {
         //int isTrue = 0;
         VData toValue;
         pToLayer->GetData( j, colToSelectFrom, toValue );
         if ( toValue == value )
            {
            Poly *pToPoly = pToLayer->GetPolygon( j );
            float distance = pThisPoly->NearestDistanceToPoly( pToPoly, threshold );

            if ( distance < nearestDistance )
               nearestDistance = distance;
            }
         }
      m_pMap->Notify( NT_CALCDIST, i, thisCount );
      SetData( i, col, nearestDistance );
      }

   return thisCount;
   }
*/

int MapLayer::PopulateLengthOfIntersection(MapLayer *pToLayer, int col, bool selectedOnlyTo /*=false*/, bool selectedOnlyThis /*=false*/, LPCTSTR label /*=NULL*/)
   {
   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON)
      return -1;

   if (pToLayer->GetType() != LT_LINE && pToLayer->GetType() != LT_POLYGON)
      return -2;

   if (col < 0)
      return -3;

   int width, decimals;
   GetTypeParams(TYPE_FLOAT, width, decimals);
   SetFieldType(col, TYPE_FLOAT, width, decimals, false);

   if (label != NULL)
      SetFieldLabel(col, label);


   int toCount = 0, thisCount = 0;

   if (selectedOnlyThis)
      thisCount = this->GetSelectionCount();
   else
      thisCount = this->GetPolygonCount(MapLayer::ALL);

   if (selectedOnlyTo)
      toCount = pToLayer->GetSelectionCount();
   else
      toCount = pToLayer->GetPolygonCount(MapLayer::ALL);

   for (int i = 0; i < thisCount; i++)
      {
      int thisPolyIndex = i;

      float length = 0.0f;

      Poly *pThisPoly = NULL;

      if (selectedOnlyThis)
         {
         thisPolyIndex = GetSelection(i);
         pThisPoly = GetPolygon(thisPolyIndex);
         }
      else
         pThisPoly = m_pPolyArray->GetAt(i);

      for (int j = 0; j < toCount; j++)
         {
         Poly *pToPoly = NULL;

         if (selectedOnlyTo)
            {
            int toPolyIndex = pToLayer->GetSelection(j);
            pToPoly = pToLayer->GetPolygon(toPolyIndex);
            }
         else
            pToPoly = pToLayer->GetPolygon(j);

         float polyLength = pThisPoly->GetIntersectionLength(pToPoly);
         length += polyLength;
         }

      m_pMap->Notify(NT_CALCDIST, i, thisCount);

      SetData(i, col, length);
      }

   return thisCount;
   }







//------------------------------------------------------------------------------
// MapLayer::PopulateLinearNetworkDensity() 
//
// this method iterates through this layer's polygons, the length of the linear 
// features that that are within a threshold distance, normalized to search area
//------------------------------------------------------------------------------

int MapLayer::PopulateLinearNetworkDensity(MapLayer *pToLayer, int col, REAL distance, bool selectedOnlyTo /*=false*/, bool selectedOnlyThis /*=false*/)
   {
   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON && m_layerType != LT_POINT)
      return -1;

   if (pToLayer->GetType() != LT_LINE)
      return -2;

   if (col < 0)
      return -3;

   int width, decimals;
   GetTypeParams(TYPE_FLOAT, width, decimals);
   SetFieldType(col, TYPE_FLOAT, width, decimals, false);

   int thisCount = 0;
   if (selectedOnlyThis)
      thisCount = GetSelectionCount();
   else
      thisCount = GetPolygonCount(MapLayer::ALL);

   int toCount = 0;
   if (selectedOnlyTo)
      toCount = pToLayer->GetSelectionCount();
   else
      toCount = pToLayer->GetPolygonCount(MapLayer::ALL);

   //int iCPU = omp_get_num_procs();
   //omp_set_num_threads(iCPU);
   int computedSoFar = 0;

   // create a "circle" poly to use as a template
   double theta = 0; // radians
   int arcs = 16;

   Poly pTemplate;
   for (int j = 0; j <= arcs; j++)
      {
      theta = double(j * (2 * PI / arcs));   // radians

      float x = distance * cos(theta);
      float y = distance * sin(theta);
      pTemplate.AddVertex(Vertex(x, y));
      }

   // bounding box
   pTemplate.m_xMin = pTemplate.m_yMin = -distance;
   pTemplate.m_xMax = pTemplate.m_yMax = distance;

   // iterate through each polygon in "this" coverage, getting nearest distantance in "to" coverage
   //#pragma omp parallel for
   for (int i = 0; i < thisCount; i++)
      {
      int thisPolyIndex = i;
      Poly *pThisPoly = NULL;
      if (selectedOnlyThis)
         {
         thisPolyIndex = GetSelection(i);
         pThisPoly = GetPolygon(thisPolyIndex);
         }
      else
         pThisPoly = m_pPolyArray->GetAt(i);

      // create a polygon representing a radius from this polygons centroid
      Vertex v = pThisPoly->GetCentroid();

      Poly circle(pTemplate);
      circle.Translate(v.x, v.y);

      float length = 0;
      // look to each poly in the "to" coverage
      //#pragma omp parallel for reduction(+:numberNearPolys)
      for (int j = 0; j < toCount; j++)
         {
         Poly *pToPoly = NULL;

         if (selectedOnlyTo)
            {
            int toPolyIndex = pToLayer->GetSelection(j);
            pToPoly = pToLayer->GetPolygon(toPolyIndex);
            }
         else
            pToPoly = pToLayer->GetPolygon(j);

         if (IsPolyInMap(pToPoly, distance))
            {
            float polyLength = circle.GetIntersectionLength(pToPoly);
            length += polyLength;
            }
         }  // end of inner (j) loop

      m_pMap->Notify(NT_POPULATERECORDS, computedSoFar++, thisCount);
      float density = length / (PI * distance * distance);
      SetData(thisPolyIndex, col, density);
      }  // end of outer (i) loop

   return thisCount;
   }



// finds the index of the corresponding network edge index the polygon
// flows into.  requires a flow direction grid as an input.
// return value:  the number of polygons in "this" layer that flow into the reach network.

int MapLayer::GetReachIndex(Poly *pPoly, MapLayer *pFlowDirGrid, MapLayer *pNetworkLayer, int method)
   {
   ASSERT(m_layerType == LT_POLYGON);
   ASSERT(pFlowDirGrid != NULL);
   ASSERT(pNetworkLayer != NULL);
   ASSERT(pFlowDirGrid->m_layerType == LT_GRID);
   ASSERT(pNetworkLayer->m_layerType == LT_LINE);

   const Vertex centroid(pPoly->GetCentroid());

   int row, col;
   bool found = pFlowDirGrid->GetGridCellFromCoord(centroid.x, centroid.y, row, col);

   if (!found) // centroid of pPoly not on grid - so just return
      return -1;

   found = false;
   int edgeCount = pNetworkLayer->GetPolygonCount();
   float cellWidth = pFlowDirGrid->GetGridCellWidth() / 2;
   float cellHeight = pFlowDirGrid->GetGridCellHeight() / 2;
   REAL cx, cy;
   while (!found)
      {
      // look for intersection with network layer
      // basic idea:
      //   see if the line corresponding to the flow direction vector for the current grid cell
      //     crosses a network edge.  If it does, return the index of the network edge.  Otherwise,
      //     follow the flow direction grid to the next grid cell and repeat.

      // first, generate a line corresponding to the flow direction vector;
      int flowDir;
      bool ok = pFlowDirGrid->GetData(row, col, flowDir);
      ASSERT(ok);

      pFlowDirGrid->GetGridCellCenter(row, col, cx, cy);

      Vertex v0, v1;
      switch (flowDir)
         {
         case E:   // east-west
         case W:
            v0.x = cx - cellWidth;
            v1.x = cx + cellWidth;
            v0.y = v1.y = cy;
            break;

         case N:  // north-south
         case S:
            v0.y = cy - cellHeight;
            v1.y = cy + cellHeight;
            v0.x = v1.x = cx;
            break;

         case SE:   // southeast-northwest
         case NW:
            v0.x = cx + cellWidth;    // se
            v0.y = cy - cellHeight;
            v1.x = cx - cellWidth;    // nw
            v1.y = cy + cellHeight;
            break;

         case NE:  // north-south
         case SW:
            v0.x = cx - cellWidth;    // sw
            v0.y = cy - cellHeight;
            v1.x = cx + cellWidth;    // ne
            v1.y = cy + cellHeight;
            break;

         default: // no or bad data - skip.
            return -4;
         }

      // have line, see if it intersects any edge in the network coverage
      for (int i = 0; i < edgeCount; i++)
         {
         Poly *pEdge = pNetworkLayer->GetPolygon(i);

         if (pEdge->DoesEdgeIntersectLine(v0, v1))
            return i;
         }

      // no interesction found for this cell, move downstream
      int dir;
      pFlowDirGrid->GetData(row, col, dir);
      ok = pFlowDirGrid->FollowFlowPath(row, col);

      if (ok)
         {
         // check to be sure this isn't a dead end, where the flow direction for the two cell point to eachother.
         int dir180 = 0;
         switch (dir)
            {
            case E:  dir180 = W;   break;
            case SE: dir180 = NW;  break;
            case S:  dir180 = N;   break;
            case SW: dir180 = NE;  break;
            case W:  dir180 = E;   break;
            case NW: dir180 = SE;  break;
            case N:  dir180 = S;   break;
            case NE: dir180 = SW;  break;
            }

         pFlowDirGrid->GetData(row, col, dir);
         if (dir == dir180)
            return -5;
         }

      if (!ok)  // off grid?
         return -2;
      }  // end of: while ( ! found )

   return -3;
   }


float MapLayer::GetCentroidFlowPathDistance(Poly *pPoly, MapLayer *pFlowDirGrid, MapLayer *pToLayer) const
   {
   Vertex centroid = pPoly->GetCentroid();
   int row = 0;
   int col = 0;

   pFlowDirGrid->GetGridCellFromCoord(centroid.x, centroid.y, row, col);

   float flowPathDistance = pFlowDirGrid->GetDownFlowPathDistance(row, col, pToLayer);

   return flowPathDistance;
   }




// Populates a column in a polygon coverage the index of the correspnding network edge index the polygon
// flows into.  requires a flow direction grid as an input.
// return value:  the number of polygons in "this" layer that flow into the reach network.

int MapLayer::PopulateReachIndex(MapLayer *pFlowDirGrid, MapLayer *pNetworkLayer, int colRchIndex, int method)
   {
   ASSERT(m_layerType == LT_POLYGON);
   ASSERT(pFlowDirGrid != NULL);
   ASSERT(pNetworkLayer != NULL);
   ASSERT(pFlowDirGrid->m_layerType == LT_GRID);
   ASSERT(pNetworkLayer->m_layerType == LT_LINE);

   if (colRchIndex < 0)
      {
      int width, decimals;
      GetTypeParams(TYPE_INT, width, decimals);
      colRchIndex = this->m_pDbTable->AddField(_T("RCH_INDEX"), TYPE_INT, width, decimals, true);
      }

   int thisCount = GetRecordCount(MapLayer::ALL);
   int foundCount = 0;
   this->SetColNoData(colRchIndex);

   for (int i = 0; i < thisCount; i++)
      {
      Poly *pPoly = GetPolygon(i);

      int reachIndex = GetReachIndex(pPoly, pFlowDirGrid, pNetworkLayer, method);
      if (reachIndex >= 0)
         {
         SetData(i, colRchIndex, reachIndex);
         foundCount++;
         }

      m_pMap->Notify(NT_POPULATERECORDS, i, thisCount);
      }

   return foundCount;
   }



int MapLayer::PopulateFlowPathDistanceFromCentroid(MapLayer *pFlowDirGrid, MapLayer *pToGrid, int colToSet)
   {
   if (m_layerType == LT_POLYGON)
      {
      int thisCount = GetRecordCount(MapLayer::ALL);

      for (int i = 0; i < thisCount; i++)
         {
         Poly *pPoly = GetPolygon(i);

         float distance = GetCentroidFlowPathDistance(pPoly, pFlowDirGrid, pToGrid);

         SetData(i, colToSet, distance);
         }

      return 1;
      }

   return 0;
   }

int MapLayer::PopulateSideOfStream(MapLayer *pFlowDirGrid, MapLayer *pToGrid, int colToSet)
   {
   if (m_layerType == LT_POLYGON)
      {
      int thisCount = GetRecordCount(MapLayer::ALL);

      for (int i = 0; i < thisCount; i++)
         {
         Poly *pPoly = GetPolygon(i);
         Vertex centroid = pPoly->GetCentroid();
         int colOrder = GetFieldCol("ORDER");

         int order = GetData(i, colOrder, order);
         int side = -1;
         if (order == 1)
            side = 1;
         else
            {
            int row = 0;
            int col = 0;
            pFlowDirGrid->GetGridCellFromCoord(centroid.x, centroid.y, row, col);
            side = pFlowDirGrid->GetSideOfStream(row, col, pToGrid);
            }

         SetData(i, colToSet, side);
         }
      }
   return 1;
   }

/*
int MapLayer::PopulateSelectedLengthOfIntersection( MapLayer *pToLayer, int col, int colToSelectFrom, LPCTSTR label  )
   {
   return -1;
   }
*/

int MapLayer::Translate(float dx, float dy)
   {
   // only poly and line coverages (for now)
   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON)
      return -1;

   int polyCount = GetRecordCount(MapLayer::ALL);

   for (int i = 0; i < polyCount; i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);

      int vertexCount = pPoly->GetVertexCount();

      for (int j = 0; j < vertexCount; j++)
         {
         Vertex &v = (Vertex&)pPoly->GetVertex(j);
         v.x += dx;
         v.y += dy;
         }

      pPoly->InitLogicalPoints(m_pMap);
      pPoly->SetLabelPoint(m_labelMethod);
      }

   return polyCount;
   }


bool MapLayer::GetZExtents(float &zMin, float &zMax)
   {
   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON)
      return false;

   if (m_3D == false)
      {
      zMin = zMax = 0.0f;
      return false;
      }

   if (m_vzMin != m_vzMax)
      {
      zMin = m_vzMin;
      zMax = m_vzMax;
      return true;
      }

   zMin = (float)  10e20;
   zMax = (float)-10e20;

   for (Iterator i = Begin(); i != End(); i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);

      int vertexCount = pPoly->GetVertexCount();

      for (int j = 0; j < vertexCount; j++)
         {
         const Vertex &v = pPoly->GetVertex(j);

         if (v.z < zMin)  zMin = v.z;
         if (v.z > zMax)  zMax = v.z;
         }
      }

   m_vzMin = zMin;
   m_vzMax = zMax;
   return true;
   }

bool MapLayer::GetZExtents(double &zMin, double &zMax)
   {
   if (m_layerType != LT_LINE && m_layerType != LT_POLYGON)
      return false;

   if (m_3D == false)
      {
      zMin = zMax = 0.0f;
      return false;
      }

   if (m_vzMin != m_vzMax)
      {
      zMin = m_vzMin;
      zMax = m_vzMax;
      return true;
      }

   zMin = (double)  10e20;
   zMax = (double)-10e20;

   for (Iterator i = Begin(); i != End(); i++)
      {
      Poly *pPoly = m_pPolyArray->GetAt(i);

      int vertexCount = pPoly->GetVertexCount();

      for (int j = 0; j < vertexCount; j++)
         {
         const Vertex &v = pPoly->GetVertex(j);

         if (v.z < zMin)  zMin = v.z;
         if (v.z > zMax)  zMax = v.z;
         }
      }

   m_vzMin = zMin;
   m_vzMax = zMax;
   return true;
   }

bool MapLayer::GetRecordLabel(int rec, CString &label, MAP_FIELD_INFO *pFieldInfo)
   {
   if (m_labelCol < 0)
      return false;

   VData value;
   GetData(rec, m_labelCol, value);

   if (pFieldInfo != NULL)
      {
      Bin *pBin = GetDataBin(m_labelCol, value);

      if (pBin != NULL && pBin->m_pFieldAttr != NULL)
         {
         label = pBin->m_pFieldAttr->label;
         return true;
         }
      }

   return value.GetAsString(label);
   }



// NOTE: FOLLOWING CODE IS INCOMPLETE!!!!
/*
int MapLayer::DissolvePolys( int rec0, int rec1 )
   {
   ASSERT( rec0 < GetRecordCount() && rec0 >= 0 );
   ASSERT( rec1 < GetRecordCount() && rec1 >= 0 );

   Poly *pPoly0 = GetPolygon( rec0 );
   Poly *pPoly1 = GetPolygon( rec1 );

   // find first point in common
   int vertexCount0 = pPoly0->GetVertexCount();
   int vertexCount1 = pPoly1->GetVertexCount();
   int i,j;
   int first0 = -1;  // index of first vertex found on poly 0
   int first1 = -1;  // index of first vertex found on poly 0
   int last0 = -1;   // index of last vertex found on poly 0
   int last1 = -1;   // index of last vertex found on poly 0
   bool found = false;

   for ( i=0; i < vertexCount0; i++ )
      {
      // get ith vertex on first poly
      const Vertex &v0 = pPoly0->GetVertex( i );

      // see if jth vertex on second poly is a match
      for ( j=0; j < vertexCount1; j++ );
         {
         const Vertex &v1 = pPoly1->GetVertex( j );

         if ( v0.x == v1.x && v0.y == v1.y ) // same coord?  (Note that == operator is overloaded to look at X, Y, Z
            {
            first0 = i;
            first1 = j;
            found = true;
            break;
            }
         }  // end of: for ( j < vertexCount1 )
      }  // end of: for ( i < vertexCount0;

   if ( ! found )
      return -1;

   // found first, keep looking for last
   //  Note:  vertices are ALWAY in clockwise order;  therefore, moving through increasing
   //  common vertex indices in poly0 will be moving through decreasing indices in poly1
   last0 = i;
   last1 = j;
   while ( found )// keep looking (found==true means we are still finding common vertices)
      {
      last0++;  // move to next vertex on poly0
      last1--;
      ASSERT( last0 < vertexCount0 );
      ASSERT( last1 >= 0 );
      const Vertex &v0 = pPoly0->GetVertex( last0 );
      const Vertex &v1 = pPoly1->GetVertex( last1 );

      if ( v0.x != v1.x || v0.y != v1.y ) // different coord?
         {
         last0--;    // back up
         last1++;
         found = false;
         break;
         }
      }

   ASSERT( found == false );

   // okay, we have the start and ending vertex.  combine into one

   // MORE NEEDED HERE!!!
   return 0;
   }
*/


//////////////////////////////////////////////////////////////////////////////
// dynamic map

void MapLayer::Subdivide(int parent, PolyArray &childrenGeometry)
   {
   int count = childrenGeometry.GetCount();
   if (count == 0)
      {
      ASSERT(0);
      return;
      }

   CUIntArray parentList;
   parentList.Add(parent);

   CUIntArray childList;

   for (int i = 0; i < count; i++)
      {
      Poly *pNewChild = AddNewPoly(*childrenGeometry[i], parentList);
      ASSERT(pNewChild);
      ASSERT(pNewChild->m_id == childrenGeometry[i]->m_id);

      childList.Add(pNewChild->m_id);
      MakePolyActive(pNewChild->m_id);  // Active is opposite of Defunct
      }

   Poly *pParent = GetPolygon(parent);
   pParent->GetChildren().Copy(childList);
   MakePolyDefunct(parent);
   }

void MapLayer::Subdivide(int parent)
   {
   ASSERT(0 <= parent && parent < GetPolygonCount(ALL));

   Poly *pParent = GetPolygon(parent);
   int childCount = pParent->GetChildCount();
   ASSERT(childCount >= 2);

   for (int i = 0; i < childCount; i++)
      {
      //Poly *pChild = GetPolygon( pParent->GetChild(i) );
      int childID = pParent->GetChild(i);
      MakePolyActive(childID); //pChild->m_id ); // Active is opposite of Defunct
      }

   MakePolyDefunct(parent);
   }

void MapLayer::UnSubdivide(int parent)
   {
   // This function does the following:
   //    * deletes all children of polygons of the subdivide
   //    * deletes the clidren's records in the database
   //    * removes the children from the parent's child poly list
   //    * trims the defunctPoly array
   //    * removes the defunct flag from the parent polygon   
   ASSERT(0 <= parent && parent < GetPolygonCount(ALL));

   Poly *pParent = GetPolygon(parent);
   int childCount = pParent->GetChildCount();
   ASSERT(childCount >= 2);

   VDataObj *pData = (VDataObj*)m_pData;
   int polyCount = GetPolygonCount(MapLayer::ALL);

   for (int i = childCount - 1; i >= 0; i--)
      {
      Poly *pChild = GetPolygon(pParent->GetChild(i));

      // note: child should be last one added
      // further note:  We could do this all at once, but it's nice to check to make sure they are coming off in the right order.
      ASSERT(this->GetPolygon(polyCount - 1) == pChild);
      delete pChild;
      pData->DeleteRow(polyCount - 1);

      polyCount--;
      }

   m_pPolyArray->SetSize(polyCount);      // get rid of the deleted poly's
   m_defunctPolyList.SetLength(polyCount);

   MakePolyActive(parent);
   }


Poly* MapLayer::AddNewPoly(Poly &polyGeometry, const CUIntArray &parentList)
   {
   // This function does the following:
   //    * A new polygon, copy constructed from the arg, polyGeometry,
   //      is appended to end of m_pPolyArray->
   //    * polyGeometry.m_id is set to the m_id of the new polygon
   //    * the new polygon is flagged as dead
   //    * A new record is created in m_pData with all fields set to GetNoDataValue()

   ASSERT(polyGeometry.m_vertexArray.GetCount() > 3);
   ASSERT(polyGeometry.m_vertexPartArray.GetCount() > 0);
   ASSERT(parentList.GetSize() < 2);

   int newPolyID = GetRecordCount(MapLayer::ALL);
   ASSERT(newPolyID == GetPolygonCount(MapLayer::ALL));

   // Create a New Polygon
   Poly *pPoly = new Poly(polyGeometry);

   pPoly->m_id = newPolyID;
   polyGeometry.m_id = newPolyID;

   //PolyArray parentPolyList; 
   //for ( int i=0; i < parentList.GetCount(); i++ )
   //   parentPolyList.Add( m_pPolyArray->GetAt( parentList[i] ) );

   pPoly->GetParents().Copy(parentList);   //parentPolyList );

   pPoly->InitLogicalPoints(GetMapPtr());
   pPoly->SetLabelPoint(m_labelMethod);

   int count = pPoly->m_vertexArray.GetCount();
   float xMin, yMin, xMax, yMax;

   xMin = yMin = FLT_MAX;
   xMax = yMax = FLT_MIN;

   for (int i = 0; i < count; i++)
      {
      const Vertex &v = pPoly->m_vertexArray[i];

      if (v.x < xMin) xMin = (float)v.x;
      if (v.y < yMin) yMin = (float)v.y;
      if (v.x > xMax) xMax = (float)v.x;
      if (v.y > yMax) yMax = (float)v.y;
      }

   pPoly->m_xMin = xMin;
   pPoly->m_xMax = xMax;
   pPoly->m_yMin = yMin;
   pPoly->m_yMax = yMax;

   m_pPolyArray->Add(pPoly); // Add to end

   // flag new poly as dead
   m_defunctPolyList.SetLength(newPolyID + 1);
   m_defunctPolyList[newPolyID] = true; //false;
   m_deadCount++;

   // create new record
   int fieldCount = GetFieldCount();
   VData *varArray = new VData[fieldCount];

   int colArea = GetFieldCol("Area");
   for (int i = 0; i < fieldCount; i++)
      {
      if (i == colArea)
         {
         float area = pPoly->GetArea();
         if (area < 0.0f)
            {
            ASSERT(0);
            area = 0.0f;
            }
         //ASSERT( area >= 0.0f );
         varArray[i] = area;
         }
      else
         varArray[i] = GetNoDataValue();
      }

   ((VDataObj*)m_pData)->AppendRow(varArray, fieldCount);

   ClassifyPoly(-1, newPolyID);

   delete[] varArray;

   return pPoly;
   }


void MapLayer::MakePolyActive(int index)
   {
   ASSERT(0 <= index && UINT(index) < m_defunctPolyList.GetSize());

   BitArray::Bit bit = m_defunctPolyList[index];

   if (bit) // if poly is defunct
      {
      bit = false; // make poly active
      m_deadCount--;
      }
   }

void MapLayer::MakePolyDefunct(int index)
   {
   if ( m_defunctPolyList.GetSize() == 0 )
      return;

   ASSERT(0 <= index && UINT(index) < m_defunctPolyList.GetSize());

   BitArray::Bit bit = m_defunctPolyList[index];

   if (!bit) // if poly is active
      {
      bit = true;
      m_deadCount++;
      }
   }


void MapLayer::DynamicClean()
   {
   ASSERT(GetPolygonCount(MapLayer::ALL) == GetRecordCount(MapLayer::ALL));

   Poly *pPoly;
   Poly *pParent;
   int   parentCount;

   int recordCount = GetRecordCount(MapLayer::ALL);
   int row = recordCount - 1;
   int firstAddedPoly = -1;

   for (; row >= 0; row--)
      {
      pPoly = GetPolygon(row);
      parentCount = pPoly->GetParentCount();

      // does this poly have any parents?
      if (parentCount == 0) // no
         {
         firstAddedPoly = row + 1;
         break;
         }
      else  // parents exists, so process parents
         {
         for (int i = 0; i < parentCount; i++)
            {
            pParent = GetPolygon(pPoly->GetParent(i));
            pParent->GetChildren().RemoveAll();
#pragma message( "__TODO__ check MapLayer::DynamicClean code!!!!" )
            MakePolyDefunct(i);  // ?????? TODO - is this right???
            }
         }
      }

#ifdef _DEBUG
   for (; row >= 0; row--)
      {
      pPoly = GetPolygon(row);
      ASSERT(pPoly->GetParentCount() == 0);
      }
#endif

   for (int i = firstAddedPoly; i < recordCount; i++)
      delete m_pPolyArray->GetAt(i);

   m_pPolyArray->RemoveAt(firstAddedPoly, recordCount - firstAddedPoly);
   ((VDataObj*)m_pData)->SetSize(m_pData->GetColCount(), firstAddedPoly);

   m_defunctPolyList.SetLength(firstAddedPoly);
   m_defunctPolyList.SetAll(false);
   }



bool MapLayer::GetUpstreamPourInPoint(int &rowUp, int &colUp, int subBasin, MapLayer *pStreamLayer) const
   {
   // check cell in each direction, add to array if necessary
   int _row, _col;
   static int dirArray[] = { NE, N, NW, W, SW, S, SE, E };
   for (int i = 0; i < 8; i++)
      {
      _row = rowUp;
      _col = colUp;
      if (IsCellUpstream(dirArray[i], _row, _col, pStreamLayer))  // send coords of this cells, 
         {
         int thisValue = -1;
         pStreamLayer->GetData(_row, _col, thisValue);
         if (thisValue == subBasin)
            {
            rowUp = _row;
            colUp = _col;
            return true;
            }
         else
            return false;
         }
      }
   return false;
   }

bool MapLayer::FollowFlowPathSubBasin(int &row, int &col, int subBasin, MapLayer *pWatershed) const
   {
   ASSERT(m_layerType == LT_GRID);
   // get the flow path direction for this cell

   float dir = -1.0f;
   bool ok = GetData(row, col, dir);
   if (!ok)
      return false;

   if (dir == GetNoDataValue())
      return false;
   GetNeighborCoords(dir, row, col);
   int shed;
   ok = pWatershed->GetData(row, col, shed);
   if (shed != subBasin)
      return false;

   return true;
   }


bool MapLayer::IsDefunct(int index) const
   {
   ASSERT(index >= 0);
   if (m_defunctPolyList.GetSize() > (UINT)index)
      return m_defunctPolyList[index];
   else
      return false; // by default m_defunctPolyList is empty and all polys are alive
                    // That's silly; you can avoid the condition test if you fill it by default 
   }

int MapLayer::GetDefunctCountByIteration() const
   {
   int count = 0;
   for (int i = 0; i < int(m_defunctPolyList.GetSize()); ++i)
      if (m_defunctPolyList[i])
         ++count;
   return count;
   }


bool MapLayer::SwapCols(int col0, int col1)
   {
   // note: field info swap not required, since they are not defined in order = col order

   // swap binArrays.  Note that there are one binArray for each db col in the BinArrayArray.
   BinArray *pBin0 = m_binArrayArray[col0];
   BinArray *pBin1 = m_binArrayArray[col1];

   m_binArrayArray[col0] = pBin1;
   m_binArrayArray[col1] = pBin0;

   // swap dbTables
   return m_pDbTable->SwapCols(col0, col1);
   }


bool MapLayer::CheckCol(int &col, LPCTSTR label, TYPE type, int flags)
   {
   col = this->GetFieldCol(label);

   if (col < 0)
      {
      if (flags & CC_MUST_EXIST)
         {
         CString msg(_T("Unable to find column ["));
         msg += label;
         msg += _T("] in the ");
         msg += m_name;
         msg += _T(" database.  This is a required column that must be populated prior to using this model");

         Report::ErrorMsg(msg, "Missing Field");
         return false;
         }

      if (flags & CC_AUTOADD)
         {
         int width, decimals;
         GetTypeParams(type, width, decimals);
         col = m_pDbTable->AddField(label, type, width, decimals, true);
         CString msg("Adding column [");
         msg += label;
         msg += "] to layer '";
         msg += m_name;
         msg += "'";
         Report::Log(msg);
         }
      else
         {
#ifndef NO_MFC
         CString msg(_T("Unable to find column ["));
         msg += label;
         msg += _T("] in the ");
         msg += m_name;
         msg += _T(" database.  Would you like to add it?");

         if (Report::WarningMsg(msg, "Add Missing Field?", MB_YESNO) == IDYES)
            {
            int width, decimals;
            GetTypeParams(type, width, decimals);
            col = m_pDbTable->AddField(label, type, width, decimals, true);
            }
#endif
         }
      }

   TYPE _type = this->GetFieldType(col);

   if (_type != type)
      {
      bool mismatch = false;

      if (IsReal(_type) && IsInteger(type)) mismatch = true;
      if (IsReal(type) && IsInteger(_type)) mismatch = true;
      if (IsString(_type) && IsNumeric(type)) mismatch = true;
      if (IsString(type) && IsNumeric(_type)) mismatch = true;

      //not sure if the following block does, or if we need a linux equivalent
#ifndef NO_MFC
      HINSTANCE hInst = ::AfxGetInstanceHandle();
      TCHAR name[256];
      strcpy_s(name, "Undefined");
      ::GetModuleFileName(hInst, name, 255);
      //::AfxGetAppName();
#endif
      if (mismatch)
         {
#ifndef NO_MFC
         CString msg("The specified database field '");
         msg += label;
         msg += "' has a different type (";
         msg += ::GetTypeLabel(_type);
         msg += ") in the database than was specified by a calling process [";
         msg += name;
         msg += "] (";
         msg += ::GetTypeLabel(type);
         msg += ").  Do you want to change the type in the database to that specified by the calling process?";

         int retVal = AfxMessageBox(msg, MB_YESNO);

         if (retVal == IDYES)
            this->SetFieldType(col, type, true);
#endif
         }
      }

   return col >= 0;
   }


//------------------------------------------------------------------------------------------------------------------------------------
// GetExpandPolysFromAttr() generates a list of all polys in a patch around the given polygon (idu) that satisfy a specified query.
//  The index of the patch polygons is returned in 'expandArray', the first of which is the polygon index (kernal) that was passed in.
//
// Inputs:
//   idu = index of the kernal (nucleus) polygon
//   colValue = the column storing the value to be compared to the value
//   value = the value to be compared to teh value in the polygons 'colValue' field
//   op = one of LT, LE, EQ, GT, GE, operator used to compare specified value with column value
//   colArea = column containing poly area.  if -1, areas are ignored, including maxExpandArea; only the expand array is populated
//   maxExpandArea = upper limit expansion size, if a positive value.  If negative, it is ignored and the max patch size is not limited
//
// returns: 
//   1) area of the expansion area (NOT including the nucleus polygon) and 
//   2) an array of polygon indexes considered for the patch (DOES include the nucelus polygon).  Zero or Positive indexes indicate they
//      were included in the patch, negative values indicate they were considered but where not included in the patch
//------------------------------------------------------------------------------------------------------------------------------------

float MapLayer::GetExpandPolysFromAttr(int idu, int colValue, VData &value, OPERATOR op, int colArea, float maxExpandArea, CArray< int, int > &expandArray)
   {
   // basic idea - expand in concentric circles aroud the idu (subject to constraints)
   // until necessary area is found.  We do this keeping track of candidate expansion IDUs
   // in m_expansionArray, starting with the "nucleus" IDU and iteratively expanding outward.
   // for the expansion array, we track the index in the expansion array of location of
   // the lastExpandedIndex and the number of unprocessed IDUs that are expandable (poolSize).
   //int neighbors[ 64 ];
   CArray< int, int >_neighbors;

   bool addToPool = false;
   INT_PTR lastExpandedIndex = 0;
   int poolSize = 1;       // number of unprocessed IDUs that are expandable

   // note: values in m_expansionArray are -(index+1)
   expandArray.RemoveAll();
   expandArray.Add(idu);  // the first one (nucleus) is added whether it passes the query or not
                            // note: this means the nucleus IDU gets treated the same as any
                            // added IDU's passing the test, meaning the expand outcome gets applied
                            // to the nucleus as well as the neighbors.  However, the nucleus
                            // idu is NOT included in the "areaSoFar" calculation, and so does not
                            // contribute to the area count to the max expansion area
   float areaSoFar = 0;

   // We collect all the idus that are next to the idus already processed that we haven't seen already
   while (poolSize > 0 && (colArea < 0 || maxExpandArea < 0 || areaSoFar < maxExpandArea))
      {
      INT_PTR countSoFar = expandArray.GetSize();

      // iterate from the last expanded IDU on the list to the end of the current list,
      // adding any neighbor IDU's that satisfy the expansion criteria.

      for (INT_PTR i = lastExpandedIndex; i < countSoFar; i++)
         {
         int nextIDU = expandArray[i];

         if (nextIDU >= 0)   // is this an expandable IDU?
            {
            Poly *pPoly = GetPolygon(nextIDU);  // -1 );    // why -1?
            int count = GetNextToPolys(nextIDU, _neighbors);

            for (int i = 0; i < count; i++)
               {
               float area = AddExpandPoly(_neighbors[i], expandArray, colValue, value, op, colArea, addToPool, areaSoFar, maxExpandArea);

               if (addToPool)  // add to the expansion array AND it is an expandable IDU?
                  {
                  poolSize++;
                  areaSoFar += area;
                  ASSERT(maxExpandArea < 0 || areaSoFar <= maxExpandArea);
                  // have we reached the max area?
                  //if ( colArea >= 0 &&  maxExpandArea > 0 && area >= maxExpandArea )
                  //   goto finished;
                  }
               }

            poolSize--;    // remove the "i"th idu just processed from the pool of unexpanded IDUs 
                           // (if it was in the pool to start with)
            }  // end of: if ( nextIDU > 0 ) 
         }  // end of: for ( i < countSoFar ) - iterated through last round of unexpanded IDUs

      lastExpandedIndex = countSoFar;
      }

   //finished:
      // at this point, the expansion area has been determined (it's stored in the expandArrays) 
   return areaSoFar;  // note: areaSoFar only includes expansion area
   }


// returns area of the idu if colArea >= 0.  Adds 
float MapLayer::AddExpandPoly(int idu, CArray< int, int > &expandArray, int colValue, VData &value, OPERATOR op, int colArea, bool &addToPool, float areaSoFar, float maxArea)
   {
   // have we seen this on already?
   for (int i = 0; i < expandArray.GetSize(); i++)
      {
      int expansionIndex = expandArray[i];

      if ((expansionIndex < 0 && -(expansionIndex + 1) == idu)    // seen but not an expandable IDU
         || (expansionIndex >= 0 && expansionIndex == idu))      // seen and is an expandable IDU
         {
         addToPool = false;
         return 0;
         }
      }

   // so, we haven't seen this one before. Is the constraint satisified?
   addToPool = false;

   // does it pass a query?
   VData thisValue;
   GetData(idu, colValue, thisValue);

   float _v, _tv;
   value.GetAsFloat(_v);
   thisValue.GetAsFloat(_tv);

   // Note: "value" is the idu refernece value (e.g. TWL). thisValue is the idu value being considered for expansion
   switch (op)
      {
      case EQ:   // equal
         if (value == thisValue)
            addToPool = true;
         break;

      case GT:
         if (value > thisValue)
            addToPool = true;
         break;

      case LT:
         if (value < thisValue)
            addToPool = true;
         break;

      case GE:
         if (value >= thisValue)
            addToPool = true;
         break;

      case LE:
         if (value <= thisValue)
            addToPool = true;
         break;

      default:
         addToPool = false;
      }


   //if ( pAlloc->m_pExpandQuery )
   //   {
   //   bool result = false;
   //   bool ok = pAlloc->m_pExpandQuery->Run( idu, result );
   //   if ( ! ok || ! result )
   //      addToPool = false;
   //   }

   float area = 0;
   if (colArea >= 0)
      {
      // would this IDU cause the area limit to be exceeded?
      GetData(idu, colArea, area);
      if (addToPool && (maxArea > 0) && ((areaSoFar + area) > maxArea))
         addToPool = false;
      }

   if (addToPool)
      expandArray.Add(idu/*+1*/);
   else
      expandArray.Add(-(idu + 1)); // Note: what gets added to the expansion array is -(index+1)

   return area;
   }



//------------------------------------------------------------------------------------------------------------------------------------
// GetExpandPolysFromQuery() generates a list of all polys in a patch around the given polygon (idu) that satisfy a specified query.
//  The index of the patch polygons is returned in 'expandArray', the first of which is the polygon index (kernal) that was passed in.
//
// Inputs:
//   idu = index of the kernal (nucleus) polygon
//   pPatchQuery = the column storing the value to be compared to the value
//   colArea = column containing poly area.  if -1, areas are ignored, including maxExpandArea; only the expand array is populated
//   maxExpandArea = upper limit expansion size, if a positive value.  If negative, it is ignored and the max patch size is not limited
//
// returns: 
//   1) area of the expansion area (NOT including the nucleus polygon) and 
//   2) an array of polygon indexes considered for the patch (DOES include the nucelus polygon).  Zero or Positive indexes indicate they
//      were included in the patch, negative values indicate they were considered but where not included in the patch
//------------------------------------------------------------------------------------------------------------------------------------

float MapLayer::GetExpandPolysFromQuery(int startingIDU, Query *pPatchQuery, int colArea, float maxExpandArea, /* bool *visited,*/ CArray< int, int >& expandArray)
   {
   // basic idea - expand in concentric circles aroud the idu (subject to constraints)
   // until necessary area is found.  We do this keeping track of candidate expansion IDUs
   // in m_expansionArray, starting with the "nucleus" IDU and iteratively expanding outward.
   // for the expansion array, we track the index in the expansion array of location of
   // the lastExpandedIndex and the number of unprocessed IDUs that are expandable (poolSize).
   //int neighbors[ 64 ];
   CArray< int, int >_neighbors;

   bool addToPool = false;
   INT_PTR lastExpandedIndex = 0;   // this is the index in the expandArray of the last expanded IDU
   int poolSize = 1;       // number of unprocessed IDUs that are expandable

   // note: values in m_expansionArray are -(index+1)
   //int startingIDU = this->GetSelection(startingIndex);

   expandArray.RemoveAll();
   expandArray.Add(startingIDU);  // the first one (nucleus) is added whether it passes the query or not
                                  // note: this means the nucleus IDU gets treated the same as any
                                  // added IDU's passing the test, meaning the expand outcome gets applied
                                  // to the nucleus as well as the neighbors.  However, the nucleus
                                  // idu is NOT included in the "areaSoFar" calculation, and so does not
                                  // contribute to the area count to the max expansion area
   float areaSoFar = 0;

   // We collect all the idus that are next to the idus already processed that we haven't seen already
   while (poolSize > 0 && (colArea < 0 || maxExpandArea < 0 || areaSoFar < maxExpandArea))
      {
      INT_PTR countSoFar = expandArray.GetSize();

      // iterate from the last expanded IDU on the list to the end of the current list,
      // adding any neighbor IDU's that satisfy the expansion criteria.

      for (INT_PTR i = lastExpandedIndex; i < countSoFar; i++)
         {
         int nextIDU = expandArray[i];  // initially, this is the kernal IDU

         if (nextIDU >= 0)   // is this an expandable IDU?
            {
            // get neighbors
            Poly* pPoly = GetPolygon(nextIDU);  // -1 );    // why -1?
            int count = GetNextToPolys(nextIDU, _neighbors);

            // iterate through neighbors, adding any IDUs that satisfies the query
            for (int i = 0; i < count; i++)
               {
               // should we add the neighbor to the expanded IDU list?infind any additional expansion IDUs in the neighbors.
               float area = AddExpandPolyFromQuery(_neighbors[i], expandArray, pPatchQuery, colArea, addToPool, areaSoFar, maxExpandArea);

               if (addToPool)  // add to the expansion array AND it is an expandable IDU?
                  {
                  poolSize++;
                  areaSoFar += area;
                  ASSERT(maxExpandArea < 0 || areaSoFar <= maxExpandArea);
                  // have we reached the max area?
                  //if ( colArea >= 0 &&  maxExpandArea > 0 && area >= maxExpandArea )
                  //   goto finished;
                  }
               }

            poolSize--;    // remove the "i"th idu just processed from the pool of unexpanded IDUs 
                           // (if it was in the pool to start with)
            }  // end of: if ( nextIDU > 0 ) 
         }  // end of: for ( i < countSoFar ) - iterated through last round of unexpanded IDUs

      lastExpandedIndex = countSoFar;
      }

   //finished:
      // at this point, the expansion area has been determined (it's stored in the expandArrays) 
   return areaSoFar;  // note: areaSoFar only includes expansion area
   }


// returns area of the idu if colArea >= 0.  Adds the given IDU to the expandArray if it hasn't been seen yet, otherwise just sets addToPool to false.
float MapLayer::AddExpandPolyFromQuery(int idu, CArray< int, int >& expandArray, Query *pQuery, int colArea, bool& addToPool, float areaSoFar, float maxArea)
   {
   //int selectedIDU = this->GetSelection(selectedIndex);

   // have we seen this on already? If so, no need to go further, just return.
   // Note that expandArray contains IDU indexes, not selected indexes
   for (int i = 0; i < expandArray.GetSize(); i++)
      {
      int expansionIndex = expandArray[i];

      if ((expansionIndex < 0 && -(expansionIndex + 1) == idu)   // seen but not an expandable IDU
         || (expansionIndex >= 0 && expansionIndex == idu))      // seen and is an expandable IDU
         {
         addToPool = false;
         return 0;
         }
      }

   // so, we haven't seen this one before. Is the constraint satisified?
   addToPool = false;

   // does it pass a query?
   bool result = false;
   bool ok = pQuery->Run(idu, result);

   if (ok && result)  // yes, so add it to the pool
      addToPool = true;

   //if ( pAlloc->m_pExpandQuery )
   //   {
   //   bool result = false;
   //   bool ok = pAlloc->m_pExpandQuery->Run( idu, result );
   //   if ( ! ok || ! result )
   //      addToPool = false;
   //   }

   float area = 0;
   if (colArea >= 0)
      {
      // would this IDU cause the area limit to be exceeded?
      GetData(idu, colArea, area);
      if (addToPool && (maxArea > 0) && ((areaSoFar + area) > maxArea))
         addToPool = false;
      }

   if (addToPool)
      expandArray.Add(idu/*+1*/);
   else
      expandArray.Add(-(idu + 1)); // Note: what gets added to the expansion array is -(index+1)

   return area;
   }



int MapLayer::GetPatchSizes(int colValue, VData &value, OPERATOR op, int colArea, Query *pAreaToConsider, CArray< float, float > &patchSizeArray)
   {
   patchSizeArray.RemoveAll();

   int iduCount = GetRecordCount();

   bool *visited = new bool[iduCount];
   memset(visited, 0, iduCount * sizeof(bool));      // initial all IDU's are unvisited

   if (pAreaToConsider != NULL)
      {
      if (pAreaToConsider->GetQueryEngine()->GetMapLayer() != this)
         return -1;

      iduCount = pAreaToConsider->Select(true); //, false );
      }

   int idu;
   for (int i = 0; i < iduCount; i++)
      {
      if (pAreaToConsider != NULL)
         idu = this->GetSelection(i);
      else
         idu = i;

      if (!visited[idu])
         {
         // Get the patch.  Returns:
         //   1) area of the expansion area (NOT including the nucleus polygon) and 
         //   2) an array of polygon indexes considered for the patch (DOES include the nucelus polygon).  Zero or Positive indexes indicate they
         //      were included in the patch, negative values indicate they were considered but where not included in the patch
         CArray< int, int > expandArray;
         float expandArea = this->GetExpandPolysFromAttr(idu, colValue, value, op, colArea, -1, expandArray);

         float iduArea = 0;
         this->GetData(idu, colArea, iduArea);
         float patchArea = iduArea + expandArea;

         patchSizeArray.Add(patchArea);

         for (int j = 0; j < (int)expandArray.GetSize(); j++)
            {
            int expandIDU = expandArray[j];
            if (expandIDU < 0)
               expandIDU = (-expandIDU) - 1;

            visited[expandIDU] = true;
            }
         }
      }

   delete[] visited;

   return (int)patchSizeArray.GetSize();
   }


int MapLayer::GetPatchSizes(Query* pPatchQuery, int colArea, CArray< float, float > &patchSizeArray)
   {
   patchSizeArray.RemoveAll();

   if (pPatchQuery == NULL)
      return -1;

   if (pPatchQuery->GetQueryEngine()->GetMapLayer() != this)
      return -2;

   int iduCount = this->GetRecordCount(); // pPatchQuery->Select(true); //false;  // clear prior query
   
   bool* visited = new bool[iduCount];   // Note: only IDUs that match selection
   for (int i = 0; i < iduCount; i++)
      visited[i] = true;
   
   // initial all IDU's are visited initially.  But, we'll mask any that satify the query
   
   int selCount = pPatchQuery->Select(true); //false;  // clear prior query
   for (int i = 0; i < selCount; i++)
      {
      int idu = this->GetSelection(i);
      visited[idu] = false;
      }

   this->ClearSelection();
   
   // note: i is the index of the *selected* IDU
   for (int i = 0; i < iduCount; i++)
      {
      if (!visited[i])
         {
         int idu = i;
         // Get the patch.  Returns:
         //   1) area of the expansion area (NOT including the nucleus polygon) and 
         //   2) an array of polygon indexes considered for the patch (DOES include the nucelus polygon).  Zero or Positive indexes indicate they
         //      were included in the patch, negative values indicate they were considered but where not included in the patch
         CArray< int, int > expandArray;
         float expandArea = this->GetExpandPolysFromQuery(idu, pPatchQuery, colArea, -1, /*visited,*/ expandArray);

         float iduArea = 0;
         this->GetData(idu, colArea, iduArea);
         float patchArea = iduArea + expandArea;

         patchSizeArray.Add(patchArea);

         for (int j = 0; j < (int)expandArray.GetSize(); j++)
            {
            int expandIDU = expandArray[j];
            if (expandIDU < 0)
               expandIDU = (-expandIDU) - 1;

            visited[expandIDU] = true;
            }
         }
      }

   delete[] visited;
   return patchSizeArray.GetSize();
   }




bool MapLayer::IsNextTo(int poly0, int poly1)
   {
   Poly *p0 = GetPolygon(poly0);
   Poly *p1 = GetPolygon(poly1);

   REAL xMin0, yMin0, xMax0, yMax0;
   REAL xMin1, yMin1, xMax1, yMax1;

   p0->GetBoundingRect(xMin0, yMin0, xMax0, yMax0);
   p1->GetBoundingRect(xMin1, yMin1, xMax1, yMax1);

   bool intersect = DoRectsIntersect(xMin0, yMin0, xMax0, yMax0,
      xMin1, yMin1, xMax1, yMax1);

   if (intersect == false)
      return false;

   for (int i = 0; i < p0->GetVertexCount(); i++)
      {
      Vertex &v0 = (Vertex&)p0->GetVertex(i);

      for (int j = 0; j < p1->GetVertexCount(); j++)
         {
         Vertex &v1 = (Vertex&)p1->GetVertex(j);

         if (v0 == v1)
            return true;
         }
      }

   return false;
   }


int MapLayer::GetNextToPolys(int poly, CArray< int, int > &nextToArray)
   {
   nextToArray.RemoveAll();

   for (int i = 0; i < GetPolygonCount(); i++)
      {
      if (i == poly)
         continue;

      if (IsNextTo(poly, i))
         nextToArray.Add(i);
      }

   return (int)nextToArray.GetSize();
   }





int MapLayer::LoadFieldInfoXml(LPCTSTR filename, bool addExtraFields)
   {
   // have xml string, start parsing
   CString path;
   PathManager::FindPath(filename, path);
      
   TiXmlDocument doc;
   bool ok = doc.LoadFile(path);

   if (!ok)
      {
      CString msg;
      msg.Format("Error reading FieldInfo input file, %s", (PCTSTR)filename);
      Report::ErrorMsg(msg);
      //AfxGetMainWnd()->MessageBox(doc.ErrorDesc(), msg);
      return false;
      }

   MAP_FIELD_INFO *pFieldInfo = NULL;

   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();

   this->m_pFieldInfoArray->Clear();
   this->m_pFieldInfoArray->m_path = filename;

   // load any field/submenu tags defined in the XML
   int loadSuccess = LoadFieldInfoXml(pXmlRoot, NULL, filename);

   // now add any missing, but required field names
   if (addExtraFields)
      {
      MAP_FIELD_INFO *pFieldInfo = NULL;
      if ((pFieldInfo = FindFieldInfo("POLICY")) == NULL)
         {
         int index = AddFieldInfo("POLICY", 0, "Policy applied to this cell", _T(""), _T(""), TYPE_INT, MFIT_CATEGORYBINS, /*0,*/ BCF_MIXED, 0, -1.f, -1.f, true);
         GetFieldInfo(index)->SetExtra(1);     // show in results
         }

      if ((pFieldInfo = FindFieldInfo("POLICYAPPS")) == NULL)
         {
         int index = AddFieldInfo("POLICYAPPS", 0, "Total Policys applied to this cell", _T(""), _T(""), TYPE_INT, MFIT_QUANTITYBINS, /*0,*/ BCF_MIXED, 0, -1.f, -1.f, true);
         GetFieldInfo(index)->SetExtra(1);
         }
      }

   return loadSuccess;
   }


int MapLayer::LoadFieldInfoXml(TiXmlElement *pXmlElement, MAP_FIELD_INFO *pParent, LPCTSTR filename)
   {
   int loadSuccess = 1;
   // iterate through 
   TiXmlNode *pXmlFieldNode = NULL;
   while (pXmlFieldNode = pXmlElement->IterateChildren(pXmlFieldNode))
      {
      if (pXmlFieldNode->Type() != TiXmlNode::ELEMENT)
         continue;

      // get the classification
      TiXmlElement *pXmlFieldElement = pXmlFieldNode->ToElement();

      CString elementName = pXmlFieldElement->Value();
      int index = -1;
      // is it a submenu definition?
      if (elementName.CompareNoCase(_T("submenu")) == 0)
         {
         LPTSTR label = NULL;

         // lookup fields
         XML_ATTR attrs[] = { // attr         type           address  isReq checkCol
                           { "label",         TYPE_STRING,   &label,  true,  0 },
                           { NULL,            TYPE_NULL,     NULL,    false, 0 } };

         bool ok = TiXmlGetAttributes(pXmlFieldElement, attrs, filename);
         if (!ok)
            {
            loadSuccess = 0;
            continue;
            }

         index = AddFieldInfo(label, 0, label, "", "", 0, MFIT_SUBMENU, 0, 0, -1.0f, -1.0f, true);
         MAP_FIELD_INFO *pSubMenu = GetFieldInfo(index);
         if (LoadFieldInfoXml(pXmlFieldElement, pSubMenu, filename) == 0)
            loadSuccess = 0;
         }

      else if (elementName.CompareNoCase(_T("field")) == 0)
         {
         LPTSTR col = NULL;
         LPTSTR label = NULL;
         int    level = 0;
         LPTSTR mfiType = NULL;
         int    displayFlag = 0;
         int    binStyle = 0;
         float  minVal = 0;
         float  maxVal = 0;
         bool   results = false;
         int    decadalMaps = 0;
         bool   siteAttr = false;
         bool   outcomes = false;
         LPTSTR datatype = NULL;

         // lookup fields
         XML_ATTR fieldAttrs[] = { // attr          type           address       isReq checkCol
                         { "col",           TYPE_STRING,   &col          , true,  0 },
                         { "label",         TYPE_STRING,   &label        , true,  0 },
                         { "level",         TYPE_INT,      &level        , false, 0 }, /*deprecated*/
                         { "mfiType",       TYPE_STRING,   &mfiType      , false, 0 },
                         { "displayFlag",   TYPE_INT,      &displayFlag  , true,  0 },
                         { "binStyle",      TYPE_INT,      &binStyle     , true,  0 },
                         { "min",           TYPE_FLOAT,    &minVal       , false, 0 },
                         { "max",           TYPE_FLOAT,    &maxVal       , false, 0 },
                         { "results",       TYPE_BOOL,     &results      , true,  0 },
                         { "decadalMaps",   TYPE_INT,      &decadalMaps  , true,  0 },
                         { "useInSiteAttr", TYPE_BOOL,     &siteAttr     , true,  0 },
                         { "useInOutcomes", TYPE_BOOL,     &outcomes     , true,  0 },
                         { "datatype",      TYPE_STRING,   &datatype     , true,  0 },
                         { NULL,            TYPE_NULL,     NULL,           false, 0 } };

         bool ok = TiXmlGetAttributes(pXmlFieldElement, fieldAttrs, filename, this);

         if (!ok)
            {
            continue;
            }

         MFI_TYPE _mfiType = MFIT_CATEGORYBINS;
         if (mfiType != NULL)
            _mfiType = (MFI_TYPE)atoi(mfiType);

         int colIndex = GetFieldCol(col);
         TYPE type = TYPE_NULL;

         if (datatype == NULL)
            {
            if (colIndex < 0)
               {
               CString msg;
               msg.Format("The column (%s) in the database referred to in %s is missing", col, (LPCTSTR)filename);
               }
            else
               type = m_pDbTable->GetFieldInfo(colIndex).type;
            }
         else
            {
            if (lstrcmpi(datatype, "long") == 0)
               type = TYPE_LONG;
            else if (lstrcmpi(datatype, "double") == 0)
               type = TYPE_DOUBLE;
            else if (lstrcmpi(datatype, "float") == 0)
               type = TYPE_DOUBLE;
            else if (lstrcmpi(datatype, "short") == 0)
               type = TYPE_SHORT;
            else if (lstrcmpi(datatype, "int") == 0)
               type = TYPE_INT;
            else if (lstrcmpi(datatype, "integer") == 0)
               type = TYPE_INT;
            else if (lstrcmpi(datatype, "boolean") == 0)
               type = TYPE_BOOL;
            else if (lstrcmpi(datatype, "string") == 0)
               type = TYPE_DSTRING;
            else
               {
               CString msg;
               msg.Format("Unrecognized type (%s) reading field info for field [%s]", datatype, col);
               Report::ErrorMsg(msg);
               type = TYPE_LONG;
               }
            }

         index = AddFieldInfo(col, 0 /*level*/, label, "", "", type, _mfiType, displayFlag, binStyle, minVal, maxVal, true);

         MAP_FIELD_INFO *pFieldInfo = NULL;

         // fill out extra data for the field.  byte 1=results, byte 2=useInSiteAttr, byte3=useInOutcome, HIWORD=decadalMaps
         if (index >= 0)
            pFieldInfo = GetFieldInfo(index);
         else
            pFieldInfo = FindFieldInfo(col);    // info previously defined...

         if (pFieldInfo)
            {
            if (results) pFieldInfo->SetExtra(1);
            if (siteAttr) pFieldInfo->SetExtra(2);
            if (outcomes) pFieldInfo->SetExtra(4);
            pFieldInfo->SetExtra(MAKELONG(0, short(decadalMaps)));

            if (pParent != NULL)
               {
               pFieldInfo->pParent = pParent;
               //pFieldInfo->level = 1;
               }

            // load description
            TiXmlNode *pXmlDescNode = pXmlFieldNode->FirstChild("description");
            if (pXmlDescNode)
               {
               TiXmlElement *pDesc = pXmlDescNode->ToElement();
               pFieldInfo->description = pDesc->GetText();
               }

            // load source information
            TiXmlNode *pXmlSourceNode = pXmlFieldNode->FirstChild("source");
            if (pXmlSourceNode)
               {
               TiXmlElement *pSource = pXmlSourceNode->ToElement();
               pFieldInfo->source = pSource->GetText();
               }

            // load attribute information
            TiXmlNode *pXmlAttrsNode = pXmlFieldNode->FirstChild("attributes");

            if (pXmlAttrsNode != NULL)
               {
               TiXmlNode *pXmlAttrNode = NULL;

               while (pXmlAttrNode = pXmlAttrsNode->IterateChildren(pXmlAttrNode))
                  {
                  if (pXmlAttrNode->Type() != TiXmlNode::ELEMENT)
                     continue;

                  // get the classification
                  TiXmlElement *pXmlAttrElement = pXmlAttrNode->ToElement();

                  const TCHAR *value = pXmlAttrElement->Attribute("value");
                  const TCHAR *label = pXmlAttrElement->Attribute("label");
                  const TCHAR *color = pXmlAttrElement->Attribute("color");
                  const TCHAR *minVal = pXmlAttrElement->Attribute("minVal");
                  const TCHAR *maxVal = pXmlAttrElement->Attribute("maxVal");
                  const TCHAR *transparency = pXmlAttrElement->Attribute("transparency");

                  if (label == NULL || color == NULL)
                     {
                     CString msg("Envision: Misformed <attr> element reading ");
                     msg += filename;
                     msg += " A required attribute is missing.";
                     Report::ErrorMsg(msg);
                     loadSuccess = false;
                     continue;
                     }

                  // convert non-string values
                  VData _value;
                  if (value)
                     {
                     _value = value;
                     _value.ChangeType(type);
                     }

                  int red = -1, green = -1, blue = -1, alpha = -1;
                  int count = 0;
                  TCHAR *ptr = (LPTSTR)color;
                  while (*ptr == ' ' || *ptr == '(')
                     ptr++;

                  // should be negative or a number - verify
                  if (!isdigit(*ptr))
                     {
                     CString msg;
                     msg.Format("Error reading color information for field '%s' attribute '%s'.  Invalid color format specified.",
                        col, label);
                     Report::WarningMsg(msg);
                     }
                  else
                     {
                     int count = sscanf_s(ptr, "%i,%i,%i", &red, &green, &blue);

                     if (count != 3)
                        {
                        CString msg("Misformed <attr> color element reading");
                        msg += filename;
                        Report::ErrorMsg(msg);
                        loadSuccess = false;
                        }
                     }

                  int _transparency = 0;
                  if (transparency != NULL)
                     {
                     _transparency = atoi(transparency);
                     }

                  float _minVal = 0;
                  float _maxVal = 0;

                  if (minVal != NULL)
                     _minVal = (float)atof(minVal);

                  if (maxVal != NULL)
                     _maxVal = (float)atof(maxVal);

                  pFieldInfo->AddAttribute(_value, label, RGB(red, green, blue), _minVal, _maxVal, _transparency);
                  }  // while (iterating attr tags)
               }  // end of: if ( pFieldInfo != NULL )
            }
         }
      }  // end of: while (iterating through nodes)

   return loadSuccess;
   }




//////////////////////////////////////////////////////////////////////////////////
// Iterator
//////////////////////////////////////////////////////////////////////////////////


MapLayer::Iterator::Iterator(const MapLayer *pLayer, int index, ML_RECORD_SET flag)
   : m_pLayer(pLayer),
   m_index(index),
   m_qrIndex(0),
   m_flag(flag)
   {
   if (flag == MapLayer::SELECTION)
      {
      if (pLayer->GetSelectionCount() > 0)
         {
         m_qrIndex = index;
         m_index = pLayer->GetSelection(m_qrIndex);
         }
      else
         {
         m_qrIndex = -1;
         m_index = -1;
         }
      }
   }

MapLayer::Iterator::Iterator(const Iterator& pi)
   {
   *this = pi;
   }

MapLayer::Iterator& MapLayer::Iterator::operator=(const Iterator& pi)
   {
   m_index = pi.m_index;
   m_qrIndex = pi.m_qrIndex;
   m_pLayer = pi.m_pLayer;
   m_flag = pi.m_flag;

   return *this;
   }

bool MapLayer::Iterator::operator==(const Iterator& pi)
   {
   return (m_index == pi.m_index && ((const MapLayer*)(m_pLayer)) == pi.m_pLayer);
   }

bool MapLayer::Iterator::operator!=(const Iterator& pi)
   {
   return !(*this == pi);
   }

void MapLayer::Iterator::Increment()
   {
   ASSERT(m_index < m_pLayer->GetPolygonCount(MapLayer::ALL));

   if (m_flag == MapLayer::ACTIVE)
      {
      do
         {
         m_index++;
         } while (m_index < m_pLayer->GetPolygonCount(MapLayer::ALL) && m_pLayer->IsDefunct(m_index));
      }

   else if (m_flag == MapLayer::DEFUNCT)
      {
      do
         {
         m_index++;
         } while (m_index < m_pLayer->GetPolygonCount(MapLayer::ALL) && (!m_pLayer->IsDefunct(m_index)));
      }

   else if (m_flag == MapLayer::SELECTION)
      {
      if (m_qrIndex < m_pLayer->GetSelectionCount() - 1)
         m_index = m_pLayer->GetSelection(++m_qrIndex);
      }

   else
      m_index++;
   }

void MapLayer::Iterator::Decrement()
   {
   ASSERT(0 < m_index);

   if (m_flag == MapLayer::ACTIVE)
      {
      do
         {
         m_index--;
         } while (m_pLayer->IsDefunct(m_index) && m_index > 0);
      }

   else if (m_flag == MapLayer::DEFUNCT)
      {
      do
         {
         m_index--;
         } while ((!m_pLayer->IsDefunct(m_index)) && m_index > 0);
      }

   else if (m_flag == MapLayer::SELECTION)
      {
      if (m_qrIndex > 0)
         m_index = m_pLayer->GetSelection(--m_qrIndex);
      }

   else
      m_index--;
   }


MapLayer::Iterator MapLayer::Begin(ML_RECORD_SET flag /*= ACTIVE*/) const
   {
   // returns an Iterator that points to the first non-dead polygon
   // starting from the beginning
   int index = 0;
   MapLayer::Iterator iterator(this, index, flag);

   if (flag == MapLayer::ACTIVE)
      {
      if (IsDefunct(index))
         iterator++; // will now be on the first Active row
      }

   else if (flag == MapLayer::DEFUNCT)
      {
      if (!IsDefunct(index))
         iterator++; // will now be on the first Active row
      }

   else if (flag == MapLayer::SELECTION)
      {
      if (this->GetSelectionCount() > 0)
         {
         iterator.m_qrIndex = 0;
         iterator.m_index = GetSelection(0);
         }
      else
         {
         iterator.m_qrIndex = -1;
         iterator.m_index = -1;
         }
      }

   return iterator;
   }

MapLayer::Iterator MapLayer::End(ML_RECORD_SET flag /*= ACTIVE*/) const
   {
   // returns an Iterator that points to the first non-dead polygon
   // starting from the end

   if (flag == MapLayer::SELECTION)
      {
      int index = this->GetSelectionCount() - 1;

      MapLayer::Iterator iterator(this, index, flag);
      return iterator;
      }

   int recordCount = GetRecordCount(MapLayer::ALL);
   int index = recordCount;
   MapLayer::Iterator iterator(this, index, flag);

   //if ( flag == MapLayer::ACTIVE )
   //   {
   //   if ( IsDefunct( index ) )
   //      iterator--; // will now be on the first Active row
   //   }
   //else if ( flag == MapLayer::DEFUNCT )
   //   {
   //   if ( ! IsDefunct( index ) )
   //      iterator--; // will now be on the first Active row
   //   }

   return iterator;
   }
//
////////////////////



//==========================================================================
#ifndef NO_MFC
PourPointDlg::PourPointDlg(CWnd* pParent /*=NULL*/)
   : CDialog(PourPointDlg::IDD, pParent)
   {
   //{{AFX_DATA_INIT(PourPointDlg)
   m_col = 0;
   m_row = 0;
   //}}AFX_DATA_INIT
   }


#if !defined IDC_COL
#define IDC_COL 0
#endif

#if !defined IDC_ROW
#define IDC_ROW 0
#endif

void PourPointDlg::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(PourPointDlg)
   DDX_Text(pDX, IDC_COL, m_col);
   DDX_Text(pDX, IDC_ROW, m_row);
   //}}AFX_DATA_MAP
   }


BEGIN_MESSAGE_MAP(PourPointDlg, CDialog)
   //{{AFX_MSG_MAP(PourPointDlg)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PourPointDlg message handlers

BOOL PourPointDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
   }
#endif //NO_MFC

//#pragma warning( pop )
