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
// PolyGridLookups.h
//
// Class declaration for the Polygon to Grid / Grid to Polygon
// lookup table creation.
//
// This is implemented using CompressedRow2DArray objects. A
// CompressedRow2DArray object stores values in a sparse array.
//
// Filling a CompressedRow2DArray object requires that elements assigned
// to the array must be done in row major order and in column order. In other
// words you can put elements into the array in this order:
//
// 0,0 0,2 0,3 1,2 1,5 3,1 3,7
//
// But you cannot put elements into the array in this order:
//
// 0,0 0,2 1,2 0,3
//
// A PolyGridLookups object creates two sparse arrays. One is used for  doing
// lookups for a grid cell to find the polygon portions covered by that cell
// The other is used for doing lookups for a polygon to find the grid cell
// portions covered by that polygon.
// 
//
// The PolyGonGridLookups class separates the user from having to deal with the
// CompressedRow2DArray class at all. For points in the grid, it does all the
// necessary conversion to access the correct indexes used by the
// CompressedRow2DArray.
//
// Class variables are described in this file. Implementation details are
// described in PolyGridLookups.cpp as necessary.
//
// NOTE: Once you are creating the initial lookup table, you must call the
// appropriate function to build the counterpart lookup table.
//
// NOTE: that the values stored in the CompressedRow2DArray objects are for
// the proportion of the gridcell in a polygon. The computation of the
// proportion of a polygon contained in a grid is calculated when necessary
//
// Development history:
// 2009.07.03 - tjs - Implemented and tested with standalone debugging driver
// 2009.07.04 - tjs - Bringing it into the FlamMap DLL class

#pragma once
#include "EnvLibs.h"

#include "CompressedRow2DArray.h"
#include "MapLayer.h"

enum BLD_FROM { BLD_FROM_GRID, BLD_FROM_POLY };

class LIBSAPI PolyGridLookups
{
private:
	CompressedRow2DArray<int>
		*m_pGridToPolyLkUp, // array for polys in a gridcell
		*m_pPolyToGridLkUp; // array for gridcells in a poly

	// pass through to Compressed2DArray
	int
		m_maxElements,		// max number of elements stored in sparse array
		m_defaultVal,      // default value for poly or grid cell
		m_nullVal;         // null value

	int
		m_numPolys,        // ttl number of polys
		m_numGridRows,     // ttl number of grid rows
		m_numGridCols;     // ttl number of grid cols

	BLD_FROM
		m_bldFrom;         // Building lookup from polys or grids?

	bool
		m_objectStatus;	// Was the PolyGridLookups object successfully created

	// Convert gridpoint to sparse array ndx
	int xyToCArrayCoord(const POINT &pt) {
		return(static_cast<int>(pt.x) * m_numGridCols + static_cast<int>(pt.y));
	}

	// Convert sparse array ndx to gridpoint 
	void CArrayCoordToxy(const int &CArrayCoord, POINT *pt) {
		pt->x = CArrayCoord / m_numGridCols;
		pt->y = CArrayCoord % m_numGridCols;
	}

	void InitFromFile(const char *fName);
public:


	// Constructor for creating, but not filling the lookup tables
	PolyGridLookups(
		int numGridRows,
		int numGridCols,
		int numPolys,
		BLD_FROM bldFrom,
		int maxElements, 
		int defaultVal,
		int nullVal);

	// Constructor for creating and filling the lookup tables
	// using a polygon layer and a grid layer
	PolyGridLookups (
		MapLayer *m_pGridMapLayer,
		MapLayer *m_pPolyMapLayer,
		int cellSubDivisions,
		int maxElements,
		int defaultVal,
		int nullVal);

	// Constructor for creating and filling the lookup tables 
	// using a polygon layer, lat/lon for upper left corner,
	// and a boolean arry representing the mask for the grid
	PolyGridLookups (
		double topLeftLat,
		double topLeftLon,
		double latIncrement,
		double lonIncrement,
		bool *gridArr,
		int numGridRows, // same as number of latitudes
		int numGridCols, // same as number of longitudes
		MapLayer *m_pPolyMapLayer,
		int cellSubDivisions,
		int maxElements,
		int defaultVal,
		int nullVal);

	// Constructor for creating the object from a saved version of the object
	PolyGridLookups(const CString fName){InitFromFile(fName.GetString());}
	PolyGridLookups(const char *fName){InitFromFile(fName);}


	~PolyGridLookups();


	// How many polys (grid cells) does a gridcell (poly) overlap
	int GetPolyCntForGridPt(const POINT &pt);
	int GetPolyCntForGridPt(const int &x, const int &y);
	int GetGridPtCntForPoly(const int &poly);

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	// The following methods return vectors of 'values' that correspond to
	// one another. For a polygon, a 'value' is the number of subgrid points within
   // a specified grid cell that intersect the polygon.
   // For example, the poly indexes returned by
	// GetPolyNdxsForGridPt() correspond to the proportions returned by
	// GetPolyProportionsForGridPt().

	// get the indexes for the the polys (gridcells) that a gridcell (poly) overlaps
	int GetPolyNdxsForGridPt(const POINT &pt, int *ndxs);
	int GetPolyNdxsForGridPt(const int &x, const int &y, int *Ndxs);
	int GetGridPtNdxsForPoly(const int &poly, POINT *Ndxs);

	// get the raw count of subcells within the specified grid cell or polygon, broken down by polygon or grid cell
	int GetPolyValsForGridPt(const POINT &pt, int *values);
	int GetPolyValsForGridPt(const int &x, const int &y, int *values);
	int GetGridPtValsForPoly(const int &poly, int *values);

	// get the proportions of overlap
	int GetPolyProportionsForGridPt(const POINT &pt, float *values);        // note: ;values must be preallocated to a length at least equal to the number of intersecting polys/grid cells
	int GetPolyProportionsForGridPt(const int &x, const int &y, float *values);
	int GetGridPtProportionsForPoly(const int &poly, float *values);           

	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// Total raw count of subcells
	int GetPolyValTtlForGridPt(const POINT &pt);
	int GetPolyValTtlForGridPt(const int &x, const int &y);
	int GetGridPtValTtlForPoly(const int &poly);

	// Which poly (gridcell) has the greatest intersection with a gridcell (poly)
	int GetPolyPluralityForGridPt(const POINT &pt);
	int GetPolyPluralityForGridPt(const int &x, const int &y);
	void GetGridPtPluralityForPoly(const int &poly, POINT *pt);
	void GetGridPtPluralityForPoly(const int &poly, int *x, int *y);

	// Set the value of an element in the lookup table
	void SetGridPtElement(const POINT &pt, const int &poly, const int &value);
	void SetGridPtElement(const int &x, const int &y, const int &poly, const int &value);
	void SetPolyElement(const int &poly, const POINT &pt, const int &value);
	void SetPolyElement(const int &poly, const int &x, const int &y, const int &value);

	// Get the value of an element in the table
	int GetGridPtElement(const POINT &pt, const int &poly);
	int GetGridPtElement(const int &x, const int &y, const int &poly);
	int GetPolyElement(const int &poly, const POINT &pt);
	int GetPolyElement(const int &poly, const int &x, const int &y);
   int ConvertCoordtoOtherCoord(const int &x, const int &y);
   // Concatenates the individual grid cells, each of which is a compressed row, into a single compressed sparse matrix row
   bool BuildCSM(PtrArray<CompressedRow2DArray<int> > csmRow);


	// Create the counterpart to the initialized table (used when building
	// the lookup from scratch.
	void CreateGridPtLookupFromPolyLookup();
	void CreatePolyLookupFromGridPtLookup();

	// Access local variables
	int GetNumGridRows() {return m_numGridRows;};
	int GetNumGridCols() {return m_numGridCols;};
	int GetNumPolys() {return m_numPolys;};
	bool ObjectStatus() {return m_objectStatus;};

	bool WriteToFile(const char *fName);

	void OutputGridPtArray(const char *fname) {
		m_pGridToPolyLkUp->DisplayArray(fname);
	}
	void OutputPolyArray(const char *fname) {
		m_pPolyToGridLkUp->DisplayArray(fname);
	}
}; //class PolyGridLookups


