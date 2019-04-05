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
// See PolyGridLookups.h for class and method descriptions

#include "EnvLibs.h"

//#include <afxdll_.h>
#pragma hdrstop

#include "CompressedRow2DArray.h"
#include "PolyGridLookups.h"
#include "Report.h"
#include <PathManager.h>
#include "PtrArray.h"

#include <map>
#include <iterator>
#include <algorithm>

#include <omp.h>

using namespace std;


PolyGridLookups::PolyGridLookups(
   int numGridRows,
   int numGridCols,
   int numPolys,
   BLD_FROM bldFrom,
   int maxElements,
   int defaultVal,
   int nullVal) {

   // Initialize class variables
   this->m_numGridRows = numGridRows;
   this->m_numGridCols = numGridCols;
   this->m_numPolys = numPolys;
   this->m_bldFrom = bldFrom;
   this->m_maxElements = maxElements;
   this->m_defaultVal = defaultVal;
   this->m_nullVal = nullVal;

   m_pPolyToGridLkUp = NULL;
   m_pGridToPolyLkUp = NULL;

   // If building from grid, instantiate the gridToPoly lookup table,
   // otherwise instantiate the polyToGrid lookup table
   if (m_bldFrom == BLD_FROM_GRID)
      {
      m_pGridToPolyLkUp = new CompressedRow2DArray<int>(
         m_numGridRows * m_numGridCols,
         m_numPolys,
         m_maxElements,
         m_defaultVal,
         m_nullVal);
      m_pPolyToGridLkUp = NULL;
      }
   else if (m_bldFrom == BLD_FROM_POLY)
      {
      m_pPolyToGridLkUp = new CompressedRow2DArray<int>(
         m_numPolys,
         m_numGridRows * m_numGridCols,
         m_maxElements,
         m_defaultVal,
         m_nullVal);
      m_pGridToPolyLkUp = NULL;
      }
   else
      {
      CString msg;
      msg.Format(_T("PolyGridLookups ERROR: Invalid bldFrom! Aborting...\n"));
      Report::ErrorMsg(msg);
      m_objectStatus = false;
      }

   } // PolyGridLookups::PolyGridLookups(int numGridRows...)

   // Concatenates the individual grid cells, each of which is a compressed row,  into a single compressed sparse matrix row 
bool PolyGridLookups::BuildCSM(PtrArray< CompressedRow2DArray<int> > csmRows)
   {
   m_pGridToPolyLkUp = new CompressedRow2DArray<int>(
      m_numGridRows * m_numGridCols,
      m_numPolys,
      m_maxElements,
      m_defaultVal,
      m_nullVal);

   size_t numGridCells = csmRows.GetSize();

   int maxPolyID = -1;

   for (int i = 0; i < numGridCells; i++)
      {
      size_t tmp = m_pGridToPolyLkUp->m_values.size();

      if (csmRows[i]->m_rowStartNdx[0] == m_nullVal)
         {
         m_pGridToPolyLkUp->m_rowStartNdx[i] = -1;
         m_pGridToPolyLkUp->m_rowEndNdx[i] = -1;
         }
      else
         {
         m_pGridToPolyLkUp->m_rowStartNdx[i] = (int)m_pGridToPolyLkUp->m_values.size();
         std::move(csmRows[i]->m_values.begin(), csmRows[i]->m_values.end(), std::back_inserter(m_pGridToPolyLkUp->m_values));
         std::move(csmRows[i]->m_colNdx.begin(), csmRows[i]->m_colNdx.end(), std::back_inserter(m_pGridToPolyLkUp->m_colNdx));
         m_pGridToPolyLkUp->m_rowEndNdx[i] = (int)m_pGridToPolyLkUp->m_values.size() - 1;

         m_pGridToPolyLkUp->m_rowColIndex = csmRows[i]->m_rowColIndex;
         m_pGridToPolyLkUp->SetCrntRow(m_pGridToPolyLkUp->m_rowColIndex);

         int maxPoly = *std::max_element(csmRows[i]->m_colNdx.begin(), csmRows[i]->m_colNdx.end());
         if (maxPoly > maxPolyID)
            maxPolyID = maxPoly;
         if (maxPolyID >= m_numPolys)
            maxPolyID = m_numPolys - 1;

         m_pGridToPolyLkUp->SetCrntCol(maxPolyID);
         }
      }

   return true;
   }

PolyGridLookups::PolyGridLookups(
   MapLayer *m_pGridMapLayer,
   MapLayer *m_pPolyMapLayer,
   int cellSubDivisions,
   int maxElements,
   int defaultVal,
   int nullVal)
   {
   this->m_maxElements = maxElements;
   this->m_defaultVal = defaultVal;
   this->m_nullVal = nullVal;

   this->m_numGridRows = m_pGridMapLayer->m_pData->GetRowCount();
   this->m_numGridCols = m_pGridMapLayer->m_pData->GetColCount();
   this->m_numPolys = m_pPolyMapLayer->GetPolygonCount();
   this->m_bldFrom = BLD_FROM_GRID;

   //msg.Format(_T("PolyGridLookups.cpp: numGridRows, numGridCols, numPolys: %d %d %d"),numGridRows, numGridCols, numPolys);
   //Report::InfoMsg(msg);

  /* CString msg;
   msg.Format(_T("Building PolyGridLookups from Maplayers '%s' (grid: rows=%d, cols=%d) and '%s' (polygons:%d) "), m_pGridMapLayer->m_name, m_numGridRows, m_numGridCols,
      m_pPolyMapLayer->m_name, m_numPolys);
   Report::Log(msg);*/

   PtrArray<CompressedRow2DArray<int>>  arrayOfGridCellPtrs;
   INT_PTR numGridCells = m_numGridRows * m_numGridCols;
   arrayOfGridCellPtrs.SetSize(numGridCells);

   m_pPolyToGridLkUp = NULL;

    REAL subCellWidth = m_pGridMapLayer->GetGridCellWidth() /
      static_cast<float>(cellSubDivisions);

   REAL subCellHeight = m_pGridMapLayer->GetGridCellHeight() /
      static_cast<float>(cellSubDivisions);

   // clock_t start = clock();

   // For each of the cells within the grid
   for (int row = 0; row < m_numGridRows; row++)
      {
#pragma omp parallel for
      for (int col = 0; col < m_numGridCols; col++)   // and columns
         {
         REAL xCellCtr = 0.0f;
         REAL yCellCtr = 0.0f;

         m_pGridMapLayer->GetGridCellCenter(row, col, xCellCtr, yCellCtr);

         // create a map, with a slot for each poly found intersecting the current grid cell.  
         // It counts, for each polygon, how many subgrid points are in that polygon

         map<long, long> polygonWt;

         // Traverse the subcells within the parent cell
         REAL xSubCellCentroid = xCellCtr - m_pGridMapLayer->GetGridCellWidth() / 2.0f + subCellWidth / 2.0f;
         int xSubCellNdx = 0;

         do {
            Vertex vMinCentroid;
            Vertex vMaxCentroid;

            if (m_pPolyMapLayer->m_layerType == LT_LINE)
               {
               vMinCentroid.x = xSubCellCentroid - subCellWidth / 2.0f;
               vMaxCentroid.x = xSubCellCentroid + subCellWidth / 2.0f;
               }

            REAL ySubCellCentroid = yCellCtr - m_pGridMapLayer->GetGridCellHeight() / 2.0f + subCellHeight / 2.0f;
            int ySubCellNdx = 0;

            do {

               if (m_pPolyMapLayer->m_layerType == LT_LINE)
                  {
                  vMinCentroid.y = ySubCellCentroid - subCellHeight / 2.0f;
                  vMaxCentroid.y = ySubCellCentroid + subCellHeight / 2.0f;
                  }

               // Find the poly for the subcell and increment its count
               int pNdx = -1;
               Poly *pPoly;

               if (m_pPolyMapLayer->m_layerType == LT_LINE)
                  {
                  pPoly = m_pPolyMapLayer->GetPolyInBoundingBox(vMinCentroid, vMaxCentroid, &pNdx);
                  }
               else
                  pPoly = m_pPolyMapLayer->GetPolygonFromCoord((float)xSubCellCentroid, (float)ySubCellCentroid, &pNdx);

               if (pPoly) {
                  polygonWt[pPoly->m_id]++;
                  }

               ySubCellCentroid += subCellHeight;
               } while (++ySubCellNdx < cellSubDivisions);

               xSubCellCentroid += subCellWidth;
            } while (++xSubCellNdx < cellSubDivisions);

            // Add the tallies for the row/col to the lookup table 
            size_t count = polygonWt.size();

            CompressedRow2DArray<int> *pGridCellToPolyLkUp = new CompressedRow2DArray<int>(
               1,
               cellSubDivisions * cellSubDivisions,
               cellSubDivisions * cellSubDivisions,
               m_defaultVal,
               m_nullVal);

            for (auto it = polygonWt.begin(); it != polygonWt.end(); it++) {
               pGridCellToPolyLkUp->SetElement(0, it->first, it->second);
               /*    SetGridPtElement(
                      row,
                      col,
                      it->first,
                      it->second);*/
               }
            pGridCellToPolyLkUp->m_rowColIndex = row * (int)m_numGridCols + col;
            INT_PTR index = row * (int)m_numGridCols + col;


            if (count == 0)
               pGridCellToPolyLkUp->m_rowStartNdx[0] = m_nullVal;


            arrayOfGridCellPtrs.SetAt(index, pGridCellToPolyLkUp);
         } // for(int col=0;col<m_NumGridCols;col++)
      } // for(int row=0;row<m_NumGridRows;row++)

   // Build the final Compressed Sparse Matrix from the individual vectors
   BuildCSM(arrayOfGridCellPtrs);

   // The grid lookup table is complete, now create the poly lookup table
   CreatePolyLookupFromGridPtLookup();

   /*clock_t finish = clock();
   float duration = (float)(finish - start) / CLOCKS_PER_SEC;

   msg.Format("Polygrid lookup file generated in %i seconds", (int)duration);
   Report::Log(msg); */

   m_objectStatus = true;

   return;

   } // PolyGridLookups::PolyGridLookups (MapLayer *m_pGridMapLayer,...

PolyGridLookups::PolyGridLookups(double topLeftLat, double topLeftLon, double latIncrement, double lonIncrement, bool *gridArr, int numGridRows, /* same as number of latitudes */ int numGridCols, /*same as number of longitudes*/ MapLayer *m_pPolyMapLayer, int cellSubDivisions, int maxElements, int defaultVal, int nullVal)
   {
   // Initialize class variables
   this->m_numGridRows = numGridRows;
   this->m_numGridCols = numGridCols;
   this->m_numPolys = m_pPolyMapLayer->GetPolygonCount();
   this->m_bldFrom = BLD_FROM_GRID;
   this->m_maxElements = maxElements;
   this->m_defaultVal = defaultVal;
   this->m_nullVal = nullVal;

   // Grid to poly compressed array
   m_pGridToPolyLkUp = new CompressedRow2DArray<int>(
      m_numGridRows * m_numGridCols,
      m_numPolys,
      m_maxElements,
      m_defaultVal,
      m_nullVal);

   m_pPolyToGridLkUp = NULL;

   // subcell increments and center offsets
   double
      subcellLatIncrement = latIncrement / cellSubDivisions,
      subcellLonIncrement = lonIncrement / cellSubDivisions,
      subcellLatCtrOffset = subcellLatIncrement / 2,  // used for subcell center
      subcellLonCtrOffset = subcellLonIncrement / 2;  // used for subcell center

   // to keep track of how many subcells intersect with the polygons
   map<long, long>
      polygonWt;

   // Traverse cell rows
   for (int gridLatNdx = 0; gridLatNdx < numGridRows; gridLatNdx++) {
      double crntCellLat = topLeftLat + gridLatNdx * latIncrement;

      // Traverse cell cols
      for (int gridLonNdx = 0; gridLonNdx < numGridCols; gridLonNdx++) {
         double crntCellLon = topLeftLon + gridLonNdx * lonIncrement;

         if (!gridArr[gridLatNdx * numGridCols + gridLonNdx])  // don't process masked out cells
            continue;

         polygonWt.clear();

         // Traverse subcell rows
         for (int subgridLatNdx = 0; subgridLatNdx < cellSubDivisions; subgridLatNdx++) {
            double crntSubCellCtrLat = crntCellLat + subcellLatCtrOffset + subgridLatNdx * subcellLatIncrement;

            // Traverse subcell cols
            for (int subgridLonNdx = 0; subgridLonNdx < cellSubDivisions; subgridLonNdx++) {
               double crntSubCellCtrLon = crntCellLon + subcellLonCtrOffset + subgridLonNdx * subcellLonIncrement;

               int pNdx;
               Poly *pPoly = m_pPolyMapLayer->GetPolygonFromCoord((float)crntSubCellCtrLon, (float)crntSubCellCtrLat, &pNdx);

               if (pPoly)
                  polygonWt[pPoly->m_id]++;

               } // for(int subgridLatNdx=0
            } // for(int subgridLatNdx=0

         map<long, long>::iterator it;
         for (it = polygonWt.begin(); it != polygonWt.end(); it++) {

            SetGridPtElement(
               gridLatNdx,
               gridLonNdx,
               it->first,
               it->second);
            }

         } // for(int gridLonNdx=0
      } // for(int gridLatNdx=0


   CreatePolyLookupFromGridPtLookup();
   m_objectStatus = true;

   } // PolyGridLookups::PolyGridLookups (double topLeftLat,...

void PolyGridLookups::InitFromFile(const char *_fName) {

   CString msg;

   m_pGridToPolyLkUp = NULL;
   m_pPolyToGridLkUp = NULL;
   m_objectStatus = false;

   // abort function for reads in this method

#define abend() { \
		CString \
			msg; \
		msg.Format(_T("PolyGridLookups::PolyGridLookups(const char *fName) read failed! Aborting...\n")); \
		Report::Log(msg); \
		m_objectStatus = false; \
		return; \
	}

   FILE *iFile;

   CString fName;
   PathManager::FindPath(_fName, fName);

   if (fopen_s(&iFile, fName, "rb") != 0)
      {
      msg.Format(_T("Error: PolyGridLookups::PolyGridLookups(const char *fName) unable to open file: %s! Aborting...\n"),
         fName);
      Report::ErrorMsg(msg);
      m_objectStatus = false;
      return;
      }

   if (fread((void *)&m_defaultVal, sizeof(int), 1, iFile) != 1)
      abend();

   if (fread((void *)&m_nullVal, sizeof(int), 1, iFile) != 1)
      abend();

   if (fread((void *)&m_maxElements, sizeof(int), 1, iFile) != 1)
      abend();

   if (fread((void *)&m_numPolys, sizeof(int), 1, iFile) != 1)
      abend();

   if (fread((void *)&m_numGridRows, sizeof(int), 1, iFile) != 1)
      abend();

   if (fread((void *)&m_numGridCols, sizeof(int), 1, iFile) != 1)
      abend();

   if (fread((void *)&m_bldFrom, sizeof(int), 1, iFile) != 1)
      abend();

   if ((m_pGridToPolyLkUp = new CompressedRow2DArray<int>(iFile)) == NULL)
      abend();

   if ((m_pPolyToGridLkUp = new CompressedRow2DArray<int>(iFile)) == NULL)
      abend();

   fclose(iFile);

#undef abend

   m_objectStatus = true;

   } // PolyGridLookups::PolyGridLookups(const char *fName)

PolyGridLookups::~PolyGridLookups() {
   if (m_pGridToPolyLkUp != NULL)
      delete m_pGridToPolyLkUp;
   if (m_pPolyToGridLkUp != NULL)
      delete m_pPolyToGridLkUp;
   } // PolyGridLookups::~PolyGridLookups()

int PolyGridLookups::GetPolyCntForGridPt(const POINT &pt) {
   int sparse_array_index;
   int poly_count;

   ASSERT(m_pGridToPolyLkUp != NULL);
   sparse_array_index = xyToCArrayCoord(pt);
   poly_count = m_pGridToPolyLkUp->GetColCnt(sparse_array_index);
   return(poly_count);
   } // int PolyGridLookups::GetPolyCntForGridPt(const POINT &pt)

int PolyGridLookups::GetPolyCntForGridPt(const int &x, const int &y) {
   POINT
      myXY;

   ASSERT(m_pGridToPolyLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   return this->GetPolyCntForGridPt(myXY);

   } // int PolyGridLookups::GetPolyCntForGridPt(const POINT &pt)

int PolyGridLookups::GetGridPtCntForPoly(const int &poly) {
   ASSERT(m_pPolyToGridLkUp != NULL);
   return m_pPolyToGridLkUp->GetColCnt(poly);
   } // int PolyGridLookups::GetGridPtCntForPoly(const int &poly)


int PolyGridLookups::GetPolyNdxsForGridPt(const POINT &pt, int *ndxs) {
   CString
      msg;

   ASSERT(m_pGridToPolyLkUp != NULL);
   return m_pGridToPolyLkUp->GetRowNdxs(xyToCArrayCoord(pt), ndxs);
   } // void PolyGridLookups::GetPolyNdxsForGridPt(const POINT &pt, int *Ndxs)

int PolyGridLookups::GetPolyNdxsForGridPt(const int &x, const int &y, int *ndxs) {
   POINT
      myXY;

   ASSERT(m_pGridToPolyLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   return this->GetPolyNdxsForGridPt(myXY, ndxs);
   } //int PolyGridLookups::GetPolyNdxsForGridPt(const int &x, const int &y, int *ndxs) {

int PolyGridLookups::GetGridPtNdxsForPoly(const int &poly, POINT *Ndxs) {
   ASSERT(m_pPolyToGridLkUp != NULL);

   int
      lkUpColCnt = GetGridPtCntForPoly(poly),
      *intNdxs;

   if (lkUpColCnt > 0) {
      intNdxs = new int[lkUpColCnt];

      // get the coords in CompressedRowArr space, then
      // convert them to the xy points in grid space
      m_pPolyToGridLkUp->GetRowNdxs(poly, intNdxs);

      for (int i = 0; i < lkUpColCnt; i++) {
         CArrayCoordToxy(intNdxs[i], &(Ndxs[i]));
         }

      delete[] intNdxs;
      } // if

   return lkUpColCnt;
   } // int PolyGridLookups::GetGridPtNdxsForPoly(const int &poly, POINT *Ndxs)

int PolyGridLookups::GetPolyValsForGridPt(const POINT &pt, int *values) {
   ASSERT(m_pGridToPolyLkUp != NULL);
   return m_pGridToPolyLkUp->GetRowValues(xyToCArrayCoord(pt), values);
   } // void PolyGridLookups::GetPolyValsForGridPt(const POINT &pt, int *values)

int PolyGridLookups::GetPolyValsForGridPt(const int &x, const int &y, int *values) {
   POINT
      myXY;

   ASSERT(m_pGridToPolyLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   return this->GetPolyValsForGridPt(myXY, values);
   } // int PolyGridLookups::GetPolyValsForGridPt(const int &x, const int &y, int *values) {

int PolyGridLookups::GetGridPtValsForPoly(const int &poly, int *values) {
   ASSERT(m_pPolyToGridLkUp != NULL);
   return m_pPolyToGridLkUp->GetRowValues(poly, values);
   } // void PolyGridLookups::GetGridPtValsForPoly(const int &poly, int *values);

int PolyGridLookups::GetPolyProportionsForGridPt(const POINT &pt, float *values) {
   ASSERT(m_pGridToPolyLkUp != NULL);

   int
      lkUpColCnt = GetPolyCntForGridPt(pt);

   int
      *iValues,
      ttlValue;

   if (lkUpColCnt > 0) {
      iValues = new int[lkUpColCnt];
      m_pGridToPolyLkUp->GetRowValues(xyToCArrayCoord(pt), iValues);
      ttlValue = m_pGridToPolyLkUp->GetRowSum(xyToCArrayCoord(pt));

      for (int i = 0; i < lkUpColCnt; i++)
         values[i] = static_cast<float>(iValues[i]) / static_cast<float>(ttlValue);

      delete[] iValues;
      } // if

   return lkUpColCnt;
   } // void PolyGridLookups::GetPolyProportionsForGridPt(const POINT &pt, float *values) {

int PolyGridLookups::GetPolyProportionsForGridPt(const int &x, const int &y, float *values) {
   POINT
      myXY;

   ASSERT(m_pGridToPolyLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   return this->GetPolyProportionsForGridPt(myXY, values);
   } // int PolyGridLookups::GetPolyProportionsForGridPt(const int &x, const int &y, float *values) {

int PolyGridLookups::GetGridPtProportionsForPoly(const int &poly, float *values)    // values should be pre-allocated to length (#of grid cells in this poly?)
   {
   ASSERT(m_pPolyToGridLkUp != NULL);
   int lkUpColCnt = GetGridPtCntForPoly(poly);

   if (lkUpColCnt > 0)    // are there any grid cells in this polygon
      {
      int *iValues = new int[lkUpColCnt];      // allocate a slot for each grid cell
      m_pPolyToGridLkUp->GetRowValues(poly, iValues);    // get raster values for each grid cell for this polygon

      int ttlValue = m_pPolyToGridLkUp->GetRowSum(poly);     // sum them

      for (int i = 0; i < lkUpColCnt; i++)
         values[i] = static_cast<float>(iValues[i]) / static_cast<float>(ttlValue);

      delete[] iValues;
      } // if

   return lkUpColCnt;
   } // void PolyGridLookups::GetGridPtProportionsForPoly(const int &poly, float *values)

int PolyGridLookups::GetPolyValTtlForGridPt(const POINT &pt) {
   ASSERT(m_pGridToPolyLkUp != NULL);
   return m_pGridToPolyLkUp->GetRowSum(xyToCArrayCoord(pt));
   } // int PolyGridLookups::GetPolyValTtlForGridPt(const POINT &pt)

int PolyGridLookups::GetPolyValTtlForGridPt(const int &x, const int &y) {
   POINT
      myXY;

   ASSERT(m_pGridToPolyLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   return this->GetPolyValTtlForGridPt(myXY);
   } // int PolyGridLookups::GetPolyValTtlForGridPt(const int &x, const int &y) {

int PolyGridLookups::GetGridPtValTtlForPoly(const int &poly) {
   ASSERT(m_pPolyToGridLkUp != NULL);
   return m_pPolyToGridLkUp->GetRowSum(poly);
   } // int PolyGridLookups::GetGridPtValTtlForPoly(const int &poly)

   //
int PolyGridLookups::GetPolyPluralityForGridPt(const POINT &pt) {
   ASSERT(m_pGridToPolyLkUp != NULL);
   return m_pGridToPolyLkUp->GetNdxOfRowMax(xyToCArrayCoord(pt));
   } // int PolyGridLookups::GetPolyPluralityForGridPt(const POINT &pt);

int PolyGridLookups::GetPolyPluralityForGridPt(const int &x, const int &y) {
   POINT
      myXY;

   ASSERT(m_pGridToPolyLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   return this->GetPolyPluralityForGridPt(myXY);
   } // int PolyGridLookups::GetPolyPluralityForGridPt(const int &x, const int &y);

void PolyGridLookups::GetGridPtPluralityForPoly(const int &poly, POINT *pt) {
   ASSERT(m_pPolyToGridLkUp != NULL);
   CArrayCoordToxy(m_pPolyToGridLkUp->GetNdxOfRowMax(poly), pt);
   } // void PolyGridLookups::GetGridPtPluralityForPoly(const int &poly, POINT *pt);

void PolyGridLookups::GetGridPtPluralityForPoly(const int &poly, int *x, int *y) {
   POINT
      myXY;

   ASSERT(m_pPolyToGridLkUp != NULL);
   CArrayCoordToxy(m_pPolyToGridLkUp->GetNdxOfRowMax(poly), &myXY);
   *x = myXY.x;
   *y = myXY.y;
   } // void PolyGridLookups::GetGridPtPluralityForPoly(const int &poly, int *x, int *y);

void PolyGridLookups::SetGridPtElement(const POINT &pt, const int &poly, const int &value) {
   ASSERT(m_pGridToPolyLkUp != NULL);
   m_pGridToPolyLkUp->SetElement(xyToCArrayCoord(pt), poly, value);
   } // void PolyGridLookups::SetGridPtElement(const POINT &pt, const int &poly, const int &value)

int PolyGridLookups::ConvertCoordtoOtherCoord(const int &x, const int &y) {
   POINT
      myXY;

   myXY.x = x;
   myXY.y = y;

   return xyToCArrayCoord(myXY);
   } // void PolyGridLookups::SetGridPtElement(const int &x, const int &y, const int &poly, const int &value)

void PolyGridLookups::SetGridPtElement(const int &x, const int &y, const int &poly, const int &value) {
   POINT
      myXY;

   ASSERT(m_pGridToPolyLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   this->SetGridPtElement(myXY, poly, value);
   } // void PolyGridLookups::SetGridPtElement(const int &x, const int &y, const int &poly, const int &value)

void PolyGridLookups::SetPolyElement(const int &poly, const POINT &pt, const int &value) {
   ASSERT(m_pPolyToGridLkUp != NULL);
   m_pPolyToGridLkUp->SetElement(poly, xyToCArrayCoord(pt), value);
   } // void PolyGridLookups::SetPolyElement(const int &poly, const POINT &pt, const int &value)

void PolyGridLookups::SetPolyElement(const int &poly, const int &x, const int &y, const int &value) {
   POINT
      myXY;

   ASSERT(m_pPolyToGridLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   this->SetPolyElement(poly, myXY, value);
   } // void PolyGridLookups::SetPolyElement(const int &poly, const int &x, const int &y, const int &value)


int PolyGridLookups::GetGridPtElement(const POINT &pt, const int &poly) {
   ASSERT(m_pGridToPolyLkUp != NULL);
   return m_pGridToPolyLkUp->GetElement(xyToCArrayCoord(pt), poly);
   } // int GetGridPtElement(const POINT &pt, const int &poly)

int PolyGridLookups::GetGridPtElement(const int &x, const int &y, const int &poly) {
   POINT
      myXY;

   ASSERT(m_pGridToPolyLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   return this->GetGridPtElement(myXY, poly);
   } // int GetGridPtElement(const int &x, const int &y, const int &poly);

int PolyGridLookups::GetPolyElement(const int &poly, const POINT &pt) {
   ASSERT(m_pPolyToGridLkUp != NULL);
   return m_pPolyToGridLkUp->GetElement(poly, xyToCArrayCoord(pt));
   } // int GetPolyElement(const int &poly, const POINT &pt);

int PolyGridLookups::GetPolyElement(const int &poly, const int &x, const int &y) {
   POINT
      myXY;

   ASSERT(m_pPolyToGridLkUp != NULL);
   myXY.x = x;
   myXY.y = y;
   return this->GetPolyElement(poly, myXY);
   } // int GetPolyElement(const int &poly, const int &x, const int &y);

void PolyGridLookups::CreateGridPtLookupFromPolyLookup() {
   ASSERT(m_pPolyToGridLkUp != NULL);
   ASSERT(m_pGridToPolyLkUp == NULL);
   m_pGridToPolyLkUp = m_pPolyToGridLkUp->Transpose();
   } // void PolyGridLookups::CreateGridPtLookupFromPolyLookup()

void PolyGridLookups::CreatePolyLookupFromGridPtLookup() {
   ASSERT(m_pGridToPolyLkUp != NULL);
   ASSERT(m_pPolyToGridLkUp == NULL);
   m_pPolyToGridLkUp = m_pGridToPolyLkUp->Transpose();
   } // void PolyGridLookups::CreatePolyLookupFromGridPtLookup()

bool PolyGridLookups::WriteToFile(const char *fName)
   {
   CString msg;
   bool rtrnStatus = false;

   FILE *oFile = NULL;  //fopen(fName,"wb");
   int errNo = fopen_s(&oFile, fName, "wb");

   /* if (fopen_s(&iFile, fName, "rb") != 0) ASSERT(0);
    if (iFile == NULL)*/

   if (errNo != 0)
      {
      CString msg(" PolyGridLookups::WriteToFile ERROR: Could not open file ");
      msg += fName;
      Report::ErrorMsg(msg);
      return rtrnStatus;
      }

   fwrite((void *)&m_defaultVal, sizeof(int), 1, oFile);
   fwrite((void *)&m_nullVal, sizeof(int), 1, oFile);
   fwrite((void *)&m_maxElements, sizeof(int), 1, oFile);
   fwrite((void *)&m_numPolys, sizeof(int), 1, oFile);
   fwrite((void *)&m_numGridRows, sizeof(int), 1, oFile);
   fwrite((void *)&m_numGridCols, sizeof(int), 1, oFile);
   fwrite((void *)&m_bldFrom, sizeof(m_bldFrom), 1, oFile);

   ASSERT(m_pGridToPolyLkUp != NULL);
   m_pGridToPolyLkUp->WriteToFile(oFile);

   ASSERT(m_pPolyToGridLkUp != NULL);
   m_pPolyToGridLkUp->WriteToFile(oFile);

   fclose(oFile);

   rtrnStatus = true;
   return rtrnStatus;

   } // bool PolyGridLookups::WriteToFile(const char *fName) {


