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
// See PolyGridMapper.h for class and method descriptions

#include "EnvLibs.h"

//#include <afxdll_.h		

#include "CompressedRow2DArray.h"
#include "PolyGridMapper.h"
#include "Report.h"
#include <PathManager.h>
#include "PtrArray.h"

#include <map>
#include <iterator>
#include <algorithm>

#include <omp.h>

using namespace std;

const int FILE_VERSION = 1;

PolyGridMapper::PolyGridMapper(LPCTSTR fName)
{
	LoadFromFile(fName); 

	if( m_objectStatus == false)
	{
		CString msg;
		msg.Format(_T("PolyGridMapper: unable to open file: %s! Aborting...\n"), fName);
		Report::ErrorMsg(msg);
	}

	return;
}

PolyGridMapper::PolyGridMapper(int numGridRows, int numGridCols, int numPolys, BLD_FRM bldFrom)
	: m_numGridRows(numGridRows)
	, m_numGridCols(numGridCols)
	, m_numPolys(numPolys)
	, m_bldFrom(bldFrom)
	, m_maxElements(numGridRows * numGridCols)
	, m_defaultVal(0)
	, m_nullVal(-9999)
	, m_version(FILE_VERSION)
	, m_pPolyToGridLkUp(NULL)
	, m_pGridToPolyLkUp(NULL)
	, m_objectStatus(true)
	, m_pPolyLayer(NULL)
	, m_pGridLayer(NULL)
	{
	// Initialize class variables

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
		} else if (m_bldFrom == BLD_FROM_POLY)
			{
			m_pPolyToGridLkUp = new CompressedRow2DArray<int>(
				m_numPolys,
				m_numGridRows * m_numGridCols,
				m_maxElements,
				m_defaultVal,
				m_nullVal);
			m_pGridToPolyLkUp = NULL;
			} else
			{
			CString msg;
			msg.Format(_T("PolyGridMapper ERROR: Invalid bldFrom! Aborting...\n"));
			Report::ErrorMsg(msg);
			m_objectStatus = false;
			}

	} // PolyGridMapper::PolyGridMapper(int numGridRows...)

PolyGridMapper::PolyGridMapper(MapLayer *pPolyMapLayer, MapLayer *pGridMapLayer, int cellSubdivisions, LPCTSTR fName)
	: m_defaultVal(0)
	, m_nullVal(-9999)
	, m_cellSubdivisions(cellSubdivisions)
	, m_numGridRows(pGridMapLayer->m_pData->GetRowCount())
	, m_numGridCols(pGridMapLayer->m_pData->GetColCount())
	, m_numPolys(pPolyMapLayer->GetPolygonCount())
	, m_version(FILE_VERSION)
	, m_maxElements(-1)
	, m_bldFrom(BLD_FROM_GRID)
	, m_polyFile(pPolyMapLayer->m_path)
	, m_gridFile(pGridMapLayer->m_path)
	, m_pPolyLayer(pPolyMapLayer)
	, m_pGridLayer(pGridMapLayer)
	, m_pPolyToGridLkUp(NULL)
	, m_pGridToPolyLkUp(NULL)
	, m_objectStatus(false)
	{
	if (m_pGridLayer->m_layerType != LT_GRID || (m_pPolyLayer->m_layerType != LT_POLYGON && m_pPolyLayer->m_layerType != LT_LINE))
		{
		m_objectStatus = false;
		return;
		}

	if (fName != NULL)
		LoadFromFile(fName);

	// unable to load from file so build it
	if (m_objectStatus == false)
		{
		PtrArray<CompressedRow2DArray<int>>  arrayOfGridCellPtrs;
		INT_PTR numGridCells = m_numGridRows * m_numGridCols;
		arrayOfGridCellPtrs.SetSize(numGridCells);

		m_maxElements = m_numGridRows * m_numGridCols;
		REAL subCellWidth = m_pGridLayer->GetGridCellWidth() / static_cast<double>(cellSubdivisions);
		REAL subCellHeight = m_pGridLayer->GetGridCellHeight() / static_cast<double>(cellSubdivisions);

		REAL cellWidth = m_pGridLayer->GetGridCellWidth();
		REAL cellHeight = m_pGridLayer->GetGridCellHeight();

		// basic idea - iterate though the grid rows and cols, looking for polys that
		// intersect each grid cell.
		// Note: 'rows' start at the top and increase going downward

		for (int row = 0; row < m_numGridRows; row++)
			{
			CString msg;
			msg.Format("PolyGridMapper Build for %s (%ix%i): %i%%", fName, m_numGridRows, this->m_numGridCols, (100 * row / m_numGridRows));
			Report::StatusMsg(msg);

#pragma omp parallel for
			for (int col = 0; col < m_numGridCols; col++)   // and columns
				{
				REAL xCellCtr = 0.0;
				REAL yCellCtr = 0.0;

				m_pGridLayer->GetGridCellCenter(row, col, xCellCtr, yCellCtr);

				REAL cellLeft = xCellCtr - (cellWidth / 2.0 );
				REAL cellBottom = yCellCtr - (cellHeight / 2.0 );

				// create a map, with a slot for each poly found intersecting the current grid cell.  
				// It counts, for each polygon, how many subgrid ROW_COLs are in that polygon

				map<long, long> polygonWt;

				// Traverse the subcells within the parent cell
				REAL xSubCellCentroid = xCellCtr - (cellWidth / 2.0) + (subCellWidth / 2.0);
				int xSubCellNdx = 0;

				Vertex vMin;      // subgrid cell vertices (ll, ur)
				Vertex vMax;
				int pNdx = -1;
				Poly *pPoly = NULL;

				for (int subCol = 0; subCol < cellSubdivisions; subCol++)  // outer loop - through x subcells (cols)
					{
					for (int subRow = 0; subRow < cellSubdivisions; subRow++)    // inner loop - through y subcells (rows)
						{
						// line coverage?
						if (m_pPolyLayer->m_layerType == LT_LINE)
							{
							vMin.x = cellLeft + subCol*subCellWidth;
							vMax.x = vMin.x + subCellWidth;

							vMin.y = cellBottom + subRow*subCellHeight;
							vMax.y = vMin.y + subCellHeight;

							pPoly = m_pPolyLayer->GetPolyInBoundingBox(vMin, vMax, &pNdx);
							} else   // polygon coverge
							pPoly = m_pPolyLayer->GetPolygonFromCoord(cellLeft + (subCol*subCellWidth) + (subCellWidth / 2.0), cellBottom + (subRow*subCellHeight) + (subCellHeight / 2.0), &pNdx);

							// did we find a poly?  then add it to our count
							if (pPoly)
								{
								ASSERT(pPoly->m_id >= 0);
								ASSERT(pPoly->m_id < m_pPolyLayer->GetPolygonCount());
								polygonWt[pPoly->m_id]++;
								}
						}
					}

				// Add the tallies for the row/col to the lookup table 
				size_t count = polygonWt.size();

				CompressedRow2DArray<int> *pGridCellToPolyLkUp = new CompressedRow2DArray<int>(
					1,
					cellSubdivisions * cellSubdivisions,
					cellSubdivisions * cellSubdivisions,
					m_defaultVal,
					m_nullVal);

				for (auto it = polygonWt.begin(); it != polygonWt.end(); it++)
					{
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

		Report::StatusMsg("Combining Arrays...");

		// Build the final Compressed Sparse Matrix from the individual vectors
		BuildCSM(arrayOfGridCellPtrs);

		// The grid lookup table is complete, now create the poly lookup table
		Report::StatusMsg("Finishing up...");
		CreatePolyLookupFromGridPtLookup();

		Report::StatusMsg("Polygrid Mapper successfully created");

		/*clock_t finish = clock();
		float duration = (float)(finish - start) / CLOCKS_PER_SEC;

		msg.Format("Polygrid lookup file generated in %i seconds", (int)duration);
		Report::Log(msg); */

		m_objectStatus = true;

      if(fName != NULL)
         WriteToFile(fName);
		}

	return;
	} // PolyGridMapper::PolyGridMapper (MapLayer *m_pGridMapLayer,...


PolyGridMapper::PolyGridMapper(double topLeftLat, double topLeftLon, double latIncrement, double lonIncrement, bool *gridArr, int numGridRows, /* same as number of latitudes */ int numGridCols, /*same as number of longitudes*/ MapLayer *m_pPolyMapLayer, int cellSubdivisions)
	{
	// Initialize class variables
	this->m_numGridRows = numGridRows;
	this->m_numGridCols = numGridCols;
	this->m_numPolys = m_pPolyMapLayer->GetPolygonCount();
	this->m_bldFrom = BLD_FROM_GRID;
	this->m_version = FILE_VERSION;
	this->m_maxElements = numGridRows * numGridCols;
	this->m_defaultVal = 0;
	this->m_nullVal = -9999;
	this->m_cellSubdivisions = cellSubdivisions;

	// Grid to poly compressed array
	m_pGridToPolyLkUp = new CompressedRow2DArray<int>(m_numGridRows*m_numGridCols, m_numPolys, m_maxElements, m_defaultVal, m_nullVal);

	m_pPolyToGridLkUp = NULL;

	// subcell increments and center offsets
	double
		subcellLatIncrement = latIncrement / cellSubdivisions,
		subcellLonIncrement = lonIncrement / cellSubdivisions,
		subcellLatCtrOffset = subcellLatIncrement / 2,  // used for subcell center
		subcellLonCtrOffset = subcellLonIncrement / 2;  // used for subcell center

	// to keep track of how many subcells intersect with the polygons
	map < long, long > polygonWt;

	// Traverse cell rows
	for (int gridLatNdx = 0; gridLatNdx < numGridRows; gridLatNdx++)
		{
		double crntCellLat = topLeftLat + gridLatNdx * latIncrement;

		// Traverse cell cols
		for (int gridLonNdx = 0; gridLonNdx < numGridCols; gridLonNdx++)
			{
			double crntCellLon = topLeftLon + gridLonNdx * lonIncrement;

			if (!gridArr[gridLatNdx * numGridCols + gridLonNdx])  // don't process masked out cells
				continue;

			polygonWt.clear();

			// Traverse subcell rows
			for (int subgridLatNdx = 0; subgridLatNdx < cellSubdivisions; subgridLatNdx++)
				{
				REAL crntSubCellCtrLat = crntCellLat + subcellLatCtrOffset + subgridLatNdx * subcellLatIncrement;

				// Traverse subcell cols
				for (int subgridLonNdx = 0; subgridLonNdx < cellSubdivisions; subgridLonNdx++)
					{
					double crntSubCellCtrLon = crntCellLon + subcellLonCtrOffset + subgridLonNdx * subcellLonIncrement;

					int pNdx;
					Poly *pPoly = m_pPolyMapLayer->GetPolygonFromCoord((REAL)crntSubCellCtrLon, (REAL)crntSubCellCtrLat, &pNdx);

					if (pPoly)
						polygonWt[pPoly->m_id]++;
					} // for(int subgridLatNdx=0
				} // for(int subgridLatNdx=0

			map<long, long>::iterator it;
			for (it = polygonWt.begin(); it != polygonWt.end(); it++)
				SetGridPtElement(gridLatNdx, gridLonNdx, it->first, it->second);
			} // for(int gridLonNdx=0
		} // for(int gridLatNdx=0

	CreatePolyLookupFromGridPtLookup();
	m_objectStatus = true;
	} // PolyGridMapper::PolyGridMapper (double topLeftLat,...


PolyGridMapper::~PolyGridMapper()
	{
	if (m_pGridToPolyLkUp != NULL)
		delete m_pGridToPolyLkUp;

	if (m_pPolyToGridLkUp != NULL)
		delete m_pPolyToGridLkUp;
	}


// Concatenates the individual grid cells, each of which is a compressed row,  into a single compressed sparse matrix row 
bool PolyGridMapper::BuildCSM(PtrArray< CompressedRow2DArray<int> > csmRows)
	{
	m_pGridToPolyLkUp = new CompressedRow2DArray<int>(m_numGridRows * m_numGridCols, m_numPolys, m_maxElements, m_defaultVal, m_nullVal);

	size_t numGridCells = csmRows.GetSize();

	int maxPolyID = -1;

	for (int i = 0; i < numGridCells; i++)
		{
		size_t tmp = m_pGridToPolyLkUp->m_values.size();

		if (csmRows[i]->m_rowStartNdx[0] == m_nullVal)
			{
			m_pGridToPolyLkUp->m_rowStartNdx[i] = -1;
			m_pGridToPolyLkUp->m_rowEndNdx[i] = -1;
			} else
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

void PolyGridMapper::LoadFromFile(LPCTSTR _fName)
	{
	CString msg;

	m_pGridToPolyLkUp = NULL;
	m_pPolyToGridLkUp = NULL;
	m_objectStatus = false;

	// abort function for reads in this method

#define abend() { CString msg; msg.Format(_T("PolyGridMapper: Failure reading file '%s'"), _fName ); Report::Log(msg); m_objectStatus = false; return; }

	FILE *fp;

	CString fName;
	PathManager::FindPath(_fName, fName);

	fopen_s( &fp, fName, "rb");

	if (fp != NULL)
		{ 
		TCHAR polyFile[256], gridFile[256], extra[256];
		if (fread((void*)&m_version, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)&m_defaultVal, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)&m_nullVal, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)&m_maxElements, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)&m_numPolys, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)&m_numGridRows, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)&m_numGridCols, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)&m_bldFrom, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)&m_cellSubdivisions, sizeof(int), 1, fp) <= 0) abend();
		if (fread((void*)polyFile, sizeof(TCHAR), 256, fp) <= 0) abend();
		if (fread((void*)gridFile, sizeof(TCHAR), 256, fp) <= 0) abend();
		if (fread((void*)extra, sizeof(TCHAR), 256, fp) <= 0) abend();

		if ((m_pGridToPolyLkUp = new CompressedRow2DArray<int>(fp)) == NULL) abend();
		if ((m_pPolyToGridLkUp = new CompressedRow2DArray<int>(fp)) == NULL) abend();

		fclose(fp);
		m_polyFile = polyFile;
		m_gridFile = gridFile;
	#undef abend

		m_objectStatus = true;
		}

	return;
	} // PolyGridMapper::PolyGridMapper(const char *fName)


//int PolyGridMapper::GetPolyCntForGridPt(const ROW_COL &pt) 
//   {
//   int sparse_array_index;
//   int poly_count;
//
//   ASSERT(m_pGridToPolyLkUp != NULL);
//   sparse_array_index = xyToCArrayCoord(pt);
//   poly_count = m_pGridToPolyLkUp->GetColCnt(sparse_array_index);
//   return(poly_count);
//   } // int PolyGridMapper::GetPolyCntForGridPt(const ROW_COL &pt)

int PolyGridMapper::GetPolyCntForGridPt(const ROW_COL &pt)
	{
	int sparse_array_index;
	int poly_count;

	ASSERT(m_pGridToPolyLkUp != NULL);
	sparse_array_index = xyToCArrayCoord(pt);
	poly_count = m_pGridToPolyLkUp->GetColCnt(sparse_array_index);
	return(poly_count);
	}


int PolyGridMapper::GetPolyCntForGridPt(const int row, const int col)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	ROW_COL myXY;

	myXY.row = row;
	myXY.col = col;
	return this->GetPolyCntForGridPt(myXY);
	}


int PolyGridMapper::GetGridPtCntForPoly(const int polyIndex)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	return m_pPolyToGridLkUp->GetColCnt(polyIndex);
	}


int PolyGridMapper::GetPolyNdxsForGridPt(const ROW_COL &pt, int *ndxs)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	return m_pGridToPolyLkUp->GetRowNdxs(xyToCArrayCoord(pt), ndxs);
	}


int PolyGridMapper::GetPolyNdxsForGridPt(const int row, const int col, int *ndxs)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);

	ROW_COL myXY( row, col );
	return this->GetPolyNdxsForGridPt(myXY, ndxs);
	}


int PolyGridMapper::GetGridPtNdxsForPoly(const int poly, ROW_COL *Ndxs)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);

	int lkUpColCnt = GetGridPtCntForPoly(poly);
	int *intNdxs = NULL;

	if (lkUpColCnt > 0)
		{
		intNdxs = new int[lkUpColCnt];

		// get the coords in CompressedRowArr space, then
		// convert them to the xy ROW_COLs in grid space
		m_pPolyToGridLkUp->GetRowNdxs(poly, intNdxs);

		for (int i = 0; i < lkUpColCnt; i++)
			{
			CArrayCoordToxy(intNdxs[i], &(Ndxs[i]));
			}

		delete[] intNdxs;
		} // if

	return lkUpColCnt;
	}


int PolyGridMapper::GetPolyValsForGridPt(const ROW_COL &pt, int *values)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	return m_pGridToPolyLkUp->GetRowValues(xyToCArrayCoord(pt), values);
	}

int PolyGridMapper::GetPolyValsForGridPt(const ROW_COL &pt, CArray<int> &values)
   {
   values.RemoveAll();

   int count = GetPolyCntForGridPt(pt);
   if (count <= 0)
      return 0;

   int *_values = new int[count];
   m_pGridToPolyLkUp->GetRowValues(xyToCArrayCoord(pt), _values);

   for (int i = 0; i < count; i++)
      values.Add(_values[i]);

   delete [] _values;
   return count;
   }

int PolyGridMapper::GetPolyValsForGridPt(const int row, const int col, int *values)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	ROW_COL myXY(row, col);
	return this->GetPolyValsForGridPt(myXY, values);
	}

int PolyGridMapper::GetPolyValsForGridPt(const int row, const int col, CArray<int> &values )
   {
   return GetPolyValsForGridPt( ROW_COL( row, col ), values );
   }

int PolyGridMapper::GetGridPtValsForPoly(const int polyIndex, int *values)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	return m_pPolyToGridLkUp->GetRowValues(polyIndex, values);
	}

int PolyGridMapper::GetGridPtValsForPoly(const int polyIndex, CArray<int> &values)
   {
   ASSERT(m_pPolyToGridLkUp != NULL);
   int count = GetGridPtCntForPoly( polyIndex );

   if ( count <= 0 )
      return 0;

   int *_values = new int[ count ];

   m_pPolyToGridLkUp->GetRowValues(polyIndex, _values);

   for ( int i=0; i < count; i++ )
      values.Add( _values[i] );

   delete [] _values;

   return count;
   }



int PolyGridMapper::GetPolyProportionsForGridPt(const ROW_COL &pt, float *values)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);

	int lkUpColCnt = GetPolyCntForGridPt(pt);

	int *iValues, ttlValue;

	if (lkUpColCnt > 0)
		{
		iValues = new int[lkUpColCnt];
		m_pGridToPolyLkUp->GetRowValues(xyToCArrayCoord(pt), iValues);
		ttlValue = m_pGridToPolyLkUp->GetRowSum(xyToCArrayCoord(pt));

		for (int i = 0; i < lkUpColCnt; i++)
			values[i] = static_cast<float>(iValues[i]) / static_cast<float>(ttlValue);

		delete[] iValues;
		} // if

	return lkUpColCnt;
	}

int PolyGridMapper::GetPolyProportionsForGridPt(const ROW_COL &pt, CArray< float> &values)
   {
   values.RemoveAll();

   ASSERT(m_pGridToPolyLkUp != NULL);

   int lkUpColCnt = GetPolyCntForGridPt(pt);

   if ( lkUpColCnt <= 0  )
      return 0;

   int *iValues = new int[ lkUpColCnt ];
   m_pGridToPolyLkUp->GetRowValues(xyToCArrayCoord(pt), iValues);
   int ttlValue = m_pGridToPolyLkUp->GetRowSum(xyToCArrayCoord(pt));

   for (int i = 0; i < lkUpColCnt; i++)
      values.Add( static_cast<float>(iValues[i]) / static_cast<float>(ttlValue) );
   
   delete[] iValues;
   return lkUpColCnt;
   }


int PolyGridMapper::GetPolyProportionsForGridPt(const int row, const int col, float *values)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	ROW_COL myXY(row, col);
	return this->GetPolyProportionsForGridPt(myXY, values);
	}

int PolyGridMapper::GetPolyProportionsForGridPt(const int row, const int col, CArray< float > &values)
   {
   ASSERT(m_pGridToPolyLkUp != NULL);
   ROW_COL myXY(row, col);
   return this->GetPolyProportionsForGridPt(myXY, values);
   }


int PolyGridMapper::GetGridPtProportionsForPoly(const int poly, float *values)    // values should be pre-allocated to length (#of grid cells in this poly?)
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
	}



int PolyGridMapper::GetGridPtProportionsForPoly(const int poly, CArray<float> &values)    // values should be pre-allocated to length (#of grid cells in this poly?)
   {
   values.RemoveAll();

   ASSERT(m_pPolyToGridLkUp != NULL);
   int lkUpColCnt = GetGridPtCntForPoly(poly);

   if ( lkUpColCnt <= 0 ) // are there any grid cells in this polygon
      return 0;

   int *iValues = new int[lkUpColCnt];      // allocate a slot for each grid cell
   m_pPolyToGridLkUp->GetRowValues(poly, iValues);    // get raster values for each grid cell for this polygon

   int ttlValue = m_pPolyToGridLkUp->GetRowSum(poly);     // sum them

   for (int i = 0; i < lkUpColCnt; i++)
         values.Add( static_cast<float>(iValues[i]) / static_cast<float>(ttlValue) );

   delete[] iValues;
   return lkUpColCnt;
   }

int PolyGridMapper::GetPolyValTtlForGridPt(const ROW_COL &pt)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	return m_pGridToPolyLkUp->GetRowSum(xyToCArrayCoord(pt));
	}


int PolyGridMapper::GetPolyValTtlForGridPt(const int row, const int col)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	ROW_COL myXY(row, col);
	return this->GetPolyValTtlForGridPt(myXY);
	}


int PolyGridMapper::GetGridPtValTtlForPoly(const int poly)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	return m_pPolyToGridLkUp->GetRowSum(poly);
	}


int PolyGridMapper::GetPolyPluralityForGridPt(const ROW_COL &pt)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	return m_pGridToPolyLkUp->GetNdxOfRowMax(xyToCArrayCoord(pt));
	}


int PolyGridMapper::GetPolyPluralityForGridPt(const int row, const int col)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	ROW_COL myXY(row, col);
	return this->GetPolyPluralityForGridPt(myXY);
	}


void PolyGridMapper::GetGridPtPluralityForPoly(const int poly, ROW_COL *pt)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	CArrayCoordToxy(m_pPolyToGridLkUp->GetNdxOfRowMax(poly), pt);
	}


void PolyGridMapper::GetGridPtPluralityForPoly(const int poly, int *row, int *col)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	ROW_COL myXY;

	CArrayCoordToxy(m_pPolyToGridLkUp->GetNdxOfRowMax(poly), &myXY);
	*row = myXY.row;
	*col = myXY.col;
	}


void PolyGridMapper::SetGridPtElement(const ROW_COL &pt, const int poly, const int value)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	m_pGridToPolyLkUp->SetElement(xyToCArrayCoord(pt), poly, value);
	}


int PolyGridMapper::ConvertCoordtoOtherCoord(const int row, const int col)
	{
	ROW_COL myXY(row, col);
	return xyToCArrayCoord(myXY);
	}

void PolyGridMapper::SetGridPtElement(const int row, const int col, const int poly, const int value)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);

	ROW_COL myXY(row, col);
	this->SetGridPtElement(myXY, poly, value);
	}


void PolyGridMapper::SetPolyElement(const int poly, const ROW_COL &pt, const int value)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	m_pPolyToGridLkUp->SetElement(poly, xyToCArrayCoord(pt), value);
	}


void PolyGridMapper::SetPolyElement(const int poly, const int row, const int col, const int value)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	ROW_COL myXY(row, col);

	this->SetPolyElement(poly, myXY, value);
	}


int PolyGridMapper::GetGridPtElement(const ROW_COL &pt, const int poly)
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	return m_pGridToPolyLkUp->GetElement(xyToCArrayCoord(pt), poly);
	}


int PolyGridMapper::GetGridPtElement(const int row, const int col, const int poly)
	{
	ROW_COL myXY;

	ASSERT(m_pGridToPolyLkUp != NULL);
	myXY.row = row;
	myXY.col = col;
	return this->GetGridPtElement(myXY, poly);
	}


int PolyGridMapper::GetPolyElement(const int poly, const ROW_COL &pt)
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	return m_pPolyToGridLkUp->GetElement(poly, xyToCArrayCoord(pt));
	}


int PolyGridMapper::GetPolyElement(const int poly, const int row, const int col)
	{
	ROW_COL myXY;

	ASSERT(m_pPolyToGridLkUp != NULL);
	myXY.row = row;
	myXY.col = col;
	return this->GetPolyElement(poly, myXY);
	}


void PolyGridMapper::CreateGridPtLookupFromPolyLookup()
	{
	ASSERT(m_pPolyToGridLkUp != NULL);
	ASSERT(m_pGridToPolyLkUp == NULL);
	m_pGridToPolyLkUp = m_pPolyToGridLkUp->Transpose();
	}


void PolyGridMapper::CreatePolyLookupFromGridPtLookup()
	{
	ASSERT(m_pGridToPolyLkUp != NULL);
	ASSERT(m_pPolyToGridLkUp == NULL);
	m_pPolyToGridLkUp = m_pGridToPolyLkUp->Transpose();
	}


bool PolyGridMapper::WriteToFile(const char *fName)
	{
	FILE *oFile = NULL;  //fopen(fName,"wb");
	if (fopen_s(&oFile, fName, "wb") != 0)
		{
		CString msg(" PolyGridMapper: Could not open file ");
		msg += fName;
		Report::ErrorMsg(msg);
		return false;
		}

	fwrite((void*)&m_version, sizeof(int), 1, oFile);
	fwrite((void*)&m_defaultVal, sizeof(int), 1, oFile);
	fwrite((void*)&m_nullVal, sizeof(int), 1, oFile);
	fwrite((void*)&m_maxElements, sizeof(int), 1, oFile);
	fwrite((void*)&m_numPolys, sizeof(int), 1, oFile);
	fwrite((void*)&m_numGridRows, sizeof(int), 1, oFile);
	fwrite((void*)&m_numGridCols, sizeof(int), 1, oFile);
	fwrite((void*)&m_bldFrom, sizeof(m_bldFrom), 1, oFile);
	fwrite((void*)&m_cellSubdivisions, sizeof(int), 1, oFile);

	TCHAR buffer[256];
	memset(buffer, 0, sizeof(TCHAR) * 256);
	_tcscpy_s(buffer, 256, (LPCTSTR)m_polyFile);
	fwrite((void*)buffer, sizeof(TCHAR), 256, oFile);

	memset(buffer, 0, sizeof(TCHAR) * 256);
	_tcscpy_s(buffer, 256, (LPCTSTR)m_gridFile);
	fwrite((void*)buffer, sizeof(TCHAR), 256, oFile);

	memset(buffer, 0, sizeof(TCHAR) * 256);
	fwrite((void*)buffer, sizeof(TCHAR), 256, oFile);

	ASSERT(m_pGridToPolyLkUp != NULL);
	m_pGridToPolyLkUp->WriteToFile(oFile);

	ASSERT(m_pPolyToGridLkUp != NULL);
	m_pPolyToGridLkUp->WriteToFile(oFile);

	fclose(oFile);

	return true;
	}


