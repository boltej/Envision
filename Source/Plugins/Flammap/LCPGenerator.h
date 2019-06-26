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
/* LCPGenerator.h
**
** Used to generate LCP files during the course of a run.
** It starts with an LCP file for the study area that has the
** correct slope, aspect, and elevation.
**
** It uses a MapLayer, a polygrid lookup, and an LCP lookup to
** convert the VegClass of Envision polygons to the LCP values
** associated with each polygon's VegClass in the grid space
** that the FlamMap DLL uses. This file is written on command.
**
** 2011.03.14 - tjs - adapting from the original
**   LandscapeGenerator and associated code from
**   FlamMapAP::PrepLandscape()
*/

#pragma once
#include <EnvExtension.h>
#include <Maplayer.h>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include "VegClassLCPLookup.h"
#include "PolyGridLookups.h"
#include "FlamMap_DLL.h"

#define FUEL_MOISTURE_MAX_LINES 1024

/*enum LCP_LAYER {
  LCP_ELEVATION,
  LCP_SLOPE,
  LCP_ASPECT,
  LCP_FUEL_MODEL,
  LCP_CANOPY_COVER,
  LCP_STAND_HEIGHT,
  LCP_CANOPY_BASE_HEIGHT,
  LCP_CANOPY_BULK_DENSITY,
  LCP_DUFF_LOADING,
  LCP_COARSE_WOODY_FUELS
};*/

class LCPGenerator {
	private:

	// Header for the FlamMap Init File
	// note the #pragma pack(2)...#pragma pack(pop) is supposed to change the
	// alignment of the struct to be on a 2 byte alignment. This is being done
	// so that the struct can be output to the FlamMap init file in packed form
	// without having to output each element separately

	// Note the values stored in the 100 element matrices are used for generating
	// a legend, so they should not be needed to run the DLL.

	// set memory alignment to 2 so that the header can be treated
	// in a struct as a single block of memory
#pragma pack(push,1)
  struct LCPHeader {
    long   crownFuels;            // 20 if no crown fuels, 21 if crown fuels exist (crown fuels = canopy height, canopy base height, canopy bulk density)
    long   groundFuels;           // 20 if no ground fuels, 21 if ground fuels exist (ground fuels = duff loading, coarse woody)
    long   latitude;              // latitude (negative for southern hemisphere)
    double loEast;                // offset to preserve coordinate precision (legacy from 16-bit OS days)
    double hiEast;                // offset to preserve coordinate precision (legacy from 16-bit OS days)
    double loNorth;               // offset to preserve coordinate precision (legacy from 16-bit OS days)
    double hiNorth;               // offset to preserve coordinate precision (legacy from 16-bit OS days)
    long   loElev;                // minimum elevation
    long   hiElev;                // maximum elevation
    long   numElev;               // number of elevation classes, -1 if > 100
    long   elevationValues[100];  // list of elevation values as longs
    long   loSlope;               // minimum slope
    long   hiSlope;               // maximum slope
    long   numSlope;              // number of slope classes, -1 if > 100
    long   slopeValues[100];      // list of slope values as longs
    long   loAspect;              // minimum aspect
    long   hiAspect;              // maximum aspect
    long   numAspects;            // number of aspect classes, -1 if > 100
    long   aspectValues[100];     // list of aspect values as longs
    long   loFuel;                // minimum fuel model value
    long   hiFuel;                // maximum fuel model value
    long   numFuel;               // number of fuel models -1 if > 100
    long   fuelValues[100];       // list of fuel model values as longs
    long   loCover;               // minimum canopy cover
    long   hiCover;               // maximum canopy cover
    long   numCover;              // number of canopy cover classes, -1 if > 100
    long   coverValues[100];      // list of canopy cover values as longs
    long   loHeight;              // minimum canopy height
    long   hiHeight;              // maximum canopy height
    long   numHeight;             // number of canopy height classes, -1 if > 100
    long   heightValues[100];     // list of canopy height values as longs
    long   loBase;                // minimum canopy base height
    long   hiBase;                // maximum canopy base height
    long   numBase;               // number of canopy base height classes, -1 if > 100
    long   baseValues[100];       // list of canopy base height values as longs
    long   loDensity;             // minimum canopy bulk density
    long   hidensity;             // maximum canopy bulk density
    long   numDensity;            // number of canopy bulk density classes, -1 if >100
    long   densityValues[100];    // list of canopy bulk density values as longs
    long   loDuff;                // minimum duff
    long   hiDuff;                // maximum duff
    long   numDuff;               // number of duff classes, -1 if > 100
    long   duffValues[100];       // list of duff values as longs
    long   loWoody;               // minimum coarse woody
    long   hiWoody;               // maximum coarse woody
    long   numWoodies;            // number of coarse woody classes, -1 if > 100
    long   woodyValues[100];      // list of coarse woody values as longs
    long   numEast;               // number of raster columns
    long   numNorth;              // number of raster rows
    double EastUtm;               // max X
    double WestUtm;               // min X
    double NorthUtm;              // max Y
    double SouthUtm;              // min Y
    long   GridUnits;             // linear unit: 0 = meters, 1 = feet, 2 = kilometers
    double XResol;                // cell size width in GridUnits
    double YResol;                // cell size height in GridUnits
    short  EUnits;                // elevation units: 0 = meters, 1 = feet
    short  SUnits;                // slope units: 0 = degrees, 1 = percent
    short  AUnits;                // aspect units: 0 = Grass categories, 1 = Grass degrees, 2 = azimuth degrees
    short  FOptions;              // fuel model options: 0 = no custom models AND no  conversion file,
                                  // 1 = custom models BUT no conversion file, 2 = no custom models BUT conversion file, 3 = custom models AND conversion file needed
    short  CUnits;                // canopy cover units: 0 = categories (0-4), 1 = percent
    short  HUnits;                // canopy height units: 1 = meters, 2 = feet, 3 = m x 10, 4 = ft x 10
    short  BUnits;                // canopy base height units: 1 = meters, 2 = feet, 3 = m x 10, 4 = ft x 10
    short  PUnits;                // canopy bulk density units: 1 = kg/m^3, 2 = lb/ft^3, 3 = kg/m^3 x 100, 4 = lb/ft^3 x 1000
    short  DUnits;                // duff units: 1 = Mg/ha x 10, 2 = t/ac x 10
    short  WOptions;              // coarse woody options (1 if coarse woody band is present)
    char   ElevFile[256];         // elevation file name
    char   SlopeFile[256];        // slope file name
    char   AspectFile[256];       // aspect file name
    char   FuelFile[256];         // fuel model file name
    char   CoverFile[256];        // canopy cover file name
    char   HeightFile[256];       // canopy height file name
    char   BaseFile[256];         // canopy base file name
    char   DensityFile[256];      // canopy bulk density file name
    char   DuffFile[256];         // duff file name
    char   WoodyFile[256];        // coarse woody file name
    char   Description[512];      // LCP file description
  };
#pragma pack (pop)

	LCPHeader
		*m_pLCPHeader; // the header for the 8 layer LCP file
	short
		*m_pLCPLayers; // The 8 layer LCP "sandwich" FlamMap will use
	int
		m_nGridNumLayers;
	PolyGridLookups
		*m_pPolyGridLkUp;
	VegClassLCPLookup
		*m_pVegClassLCPLookup;

	//source units for canopy characteristics from translation file
	int m_sourceCanopyCoverUnits;
	int m_sourceHeightUnits;
	int m_sourceBaseHeightUnits;
	int m_sourceBulkDensityUnits;

	void SetLayerPt(LCP_LAYER layer, const int x, const int y, const short value);

	void SetFuelModel(const int x, const int y, const short value) {
		SetLayerPt(FUEL, x, y, value);}
	void SetCanopyCover(const int x, const int y, const short value) {
		SetLayerPt(COVER, x, y, value);}
	void SetStandHeight(const int x, const int y, const short value) {
		SetLayerPt(HEIGHT, x, y, value);}
	void SetBaseHeight(const int x, const int y, const short value) {
		SetLayerPt(BASE_HEIGHT, x, y, value);}
	void SetBulkDensity(const int x, const int y, const short value) {
		SetLayerPt(BULK_DENSITY, x, y, value);}

public:
  
	LCPGenerator();

	LCPGenerator( LPCTSTR LCPFName, 
		LPCTSTR VegClassLCPLookupFName, 
		PolyGridLookups *pgLookup);
/*	{
		InitLCPValues(LCPFName);
		InitVegClassLCPLookup(VegClassLCPLookupFName);
		InitPolyGridLookup(pgLookup);
	}*/

	~LCPGenerator();

	BOOL InitLCPValues(LPCTSTR LCPFName);
	BOOL InitVegClassLCPLookup(LPCTSTR LCPLookupFName);
	void InitPolyGridLookup(PolyGridLookups *pgLookup);


	int GetNumRows() {return (int) m_pLCPHeader->numNorth;}
	int GetNumCols() {return (int) m_pLCPHeader->numEast;}
	int GetNumLayers() {return m_nGridNumLayers;}
	double GetCellDim() {return m_pLCPHeader->XResol;}
	double GetEast() {return m_pLCPHeader->EastUtm;}
	double GetWest() {return m_pLCPHeader->WestUtm;}
	double GetNorth() {return m_pLCPHeader->NorthUtm;}
	double GetSouth() {return m_pLCPHeader->SouthUtm;}

	void PrepLandscape(EnvContext *m_pEnvContext);
	void WriteFile(LPCTSTR fName, EnvContext *pEnvContext);
	void WriteReadableHeaderToFile(LPCTSTR fName);
}; // class LCPGenerator
