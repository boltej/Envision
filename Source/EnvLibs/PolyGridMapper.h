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
// PolyGridMapper.h
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
// A PolyGridMapper object creates two sparse arrays. One is used for  doing
// lookups for a grid cell to find the polygon portions covered by that cell
// The other is used for doing lookups for a polygon to find the grid cell
// portions covered by that polygon.
// 
//
// The PolyGonGridLookups class separates the user from having to deal with the
// CompressedRow2DArray class at all. For ROW_COLs in the grid, it does all the
// necessary conversion to access the correct indexes used by the
// CompressedRow2DArray.
//
// Class variables are described in this file. Implementation details are
// described in PolyGridMapper.cpp as necessary.
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

#include <stdio.h>

enum BLD_FRM { BLD_FROM_GRID, BLD_FROM_POLY };

class LIBSAPI PolyGridMapper
{
private:
   CompressedRow2DArray<int>
      *m_pGridToPolyLkUp,        // array for polys in a gridcell
      *m_pPolyToGridLkUp;        // array for gridcells in a poly

   // pass through to Compressed2DArray
   int m_maxElements;            // max number of elements stored in sparse array
   int m_defaultVal;             // default value for poly or grid cell
   int m_nullVal;                // null value
   int m_numPolys;               // total number of polys
   int m_numGridRows;            // total number of grid rows
   int m_numGridCols;            // total number of grid cols
   int m_cellSubdivisions;       // number of row or col subdivisions used when generating grid 

   CString m_polyFile;
   CString m_gridFile;

   BLD_FRM m_bldFrom;           // Building lookup from polys or grids?
   int m_version;
   bool m_objectStatus;            // Was the PolyGridMapper object successfully created
   MapLayer *m_pPolyLayer;
   MapLayer *m_pGridLayer;

   // Convert gridROW_COL to sparse array ndx
   int xyToCArrayCoord(const ROW_COL pt) { return(static_cast<int>(pt.row) * m_numGridCols + static_cast<int>(pt.col)); }

   // Convert sparse array ndx to gridROW_COL 
   void CArrayCoordToxy(const int &CArrayCoord, ROW_COL *pt) { pt->row = CArrayCoord / m_numGridCols; pt->col = CArrayCoord % m_numGridCols; }

   void LoadFromFile(LPCTSTR fName);


private:
   // Constructor for creating, but not filling the lookup tables
   PolyGridMapper(int numGridRows,int numGridCols,int numPolys,BLD_FRM bldFrom );

public:
   // Constructor for creating and filling the lookup tables
   // using a polygon layer and a grid layer
  // PolyGridMapper (MapLayer *m_pPolyMapLayer, MapLayer *m_pGridMapLayer, int cellSubDivisions );
	PolyGridMapper (MapLayer *m_pPolyMapLayer, MapLayer *m_pGridMapLayer, int cellSubDivisions, LPCTSTR filename=NULL);

   // Constructor for creating and filling the lookup tables 
   // using a polygon layer, lat/lon for upper left corner,
   // and a boolean arry representing the mask for the grid
   PolyGridMapper (
      double topLeftLat,
      double topLeftLon,
      double latIncrement,
      double lonIncrement,
      bool *gridArr,
      int numGridRows, // same as number of latitudes
      int numGridCols, // same as number of longitudes
      MapLayer *m_pPolyMapLayer,
      int cellSubDivisions );

   // Constructor for creating the object from a saved version of the object
   PolyGridMapper(const char *fName); 

   // destructor
   ~PolyGridMapper();

   // accessors
   LPCTSTR GetPolyPath() { return m_polyFile; }
   LPCTSTR GetGridPath() { return m_gridFile; }
   int     GetVersion() { return m_version; }
   void ToString( CString &str, int flags ) { if ( flags & 1 ) m_pGridToPolyLkUp->ToString( str, "%i"); else m_pPolyToGridLkUp->ToString( str, "%ld"); return; }
   int GetNumGridRows() {return m_numGridRows;};
   int GetNumGridCols() {return m_numGridCols;};
   int GetNumPolys() {return m_numPolys;};
   bool ObjectStatus() {return m_objectStatus;};
   BLD_FRM GetBuildFrom( void ) { return m_bldFrom; }

   // How many polys (grid cells) does a gridcell (poly) overlap
//   int GetPolyCntForGridPt(const ROW_COL &pt);
   int GetPolyCntForGridPt(const ROW_COL &pt);
   int GetPolyCntForGridPt(const int row, const int col);
   int GetGridPtCntForPoly(const int poly);

   // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
   // The following methods return vectors of 'values' that correspond to
   // one another. For a polygon, a 'value' is the number of subgrid ROW_COLs within
   // a specified grid cell that intersect the polygon.
   // For example, the poly indexes returned by
   // GetPolyNdxsForGridPt() correspond to the proportions returned by
   // GetPolyProportionsForGridPt().

   // get the indexes for the the polys (gridcells) that a gridcell (poly) overlaps
   int GetPolyNdxsForGridPt(const ROW_COL &pt, int *ndxs);
   int GetPolyNdxsForGridPt(const int row, const int col, int *Ndxs);
   int GetGridPtNdxsForPoly(const int poly, ROW_COL *Ndxs);

   // get the raw count of subcells within the specified grid cell or polygon, broken down by polygon or grid cell
   int GetPolyValsForGridPt(const ROW_COL &pt, int *values);
   int GetPolyValsForGridPt(const ROW_COL &pt, CArray<int> &values);

   int GetPolyValsForGridPt(const int row, const int col, int *values);
   int GetPolyValsForGridPt(const int row, const int col, CArray<int> &values);

   int GetGridPtValsForPoly(const int poly, int *values);
   int GetGridPtValsForPoly(const int poly, CArray<int> &values);

   // get the proportions of overlap
   int GetPolyProportionsForGridPt(const ROW_COL &pt, float *values);        // note: ;values must be preallocated to a length at least equal to the number of intersecting polys/grid cells
   int GetPolyProportionsForGridPt(const ROW_COL &pt, CArray<float> &values);        // note: ;values must be preallocated to a length at least equal to the number of intersecting polys/grid cells

   int GetPolyProportionsForGridPt(const int row, const int col, float *values);
   int GetPolyProportionsForGridPt(const int row, const int col, CArray<float> &values);

   int GetGridPtProportionsForPoly(const int poly, float *values);
   int GetGridPtProportionsForPoly(const int poly, CArray<float> &values);

   // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   // Total raw count of subcells
   int GetPolyValTtlForGridPt(const ROW_COL &pt);
   int GetPolyValTtlForGridPt(const int row, const int col);
   int GetGridPtValTtlForPoly(const int poly);

   // Which poly (gridcell) has the greatest intersection with a gridcell (poly)
   int GetPolyPluralityForGridPt(const ROW_COL &pt);
   int GetPolyPluralityForGridPt(const int row, const int col);
   void GetGridPtPluralityForPoly(const int poly, ROW_COL *pt);
   void GetGridPtPluralityForPoly(const int poly, int *x, int *y);

   // Set the value of an element in the lookup table
   void SetGridPtElement(const ROW_COL &pt, const int poly, const int value);
   void SetGridPtElement(const int row, const int col, const int poly, const int value);
   void SetPolyElement(const int poly, const ROW_COL &pt, const int value);
   void SetPolyElement(const int poly, const int row, const int col, const int value);

   // Get the value of an element in the table
   int GetGridPtElement(const ROW_COL &pt, const int poly);
   int GetGridPtElement(const int row, const int col, const int poly);
   int GetPolyElement(const int poly, const ROW_COL &pt);
   int GetPolyElement(const int poly, const int row, const int col);
   int ConvertCoordtoOtherCoord(const int row, const int col);
   // Concatenates the individual grid cells, each of which is a compressed row, into a single compressed sparse matrix row
   bool BuildCSM(PtrArray<CompressedRow2DArray<int> > csmRow);

   int GetCellSubdivisions() { return m_cellSubdivisions; }
   //void SetCellSubdivisions( int count ) { m_cellSubdivisions = count; }

   // Create the counterpart to the initialized table (used when building
   // the lookup from scratch.
   void CreateGridPtLookupFromPolyLookup();
   void CreatePolyLookupFromGridPtLookup();


   bool WriteToFile(const char *fName);

   void OutputGridPtArray(const char *fname) {
      m_pGridToPolyLkUp->DisplayArray(fname);
   }
   void OutputPolyArray(const char *fname) {
      m_pPolyToGridLkUp->DisplayArray(fname);
   }
}; //class PolyGridMapper


