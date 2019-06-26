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
#ifndef TERRBIODIV_H
#define TERRBIODIV_H

#include <EnvExtension.h>

#include <Vdataobj.h>
#include <maplayer.h>
#include <map>
#include <Fdataobj.h>
#include <vector>
#include <PolyGridLookups.h>

using namespace std;

// enumeration for Cover Types
enum CoverType
	{ 
		CT_PK		= 205,   // Subalpine parkland
		CT_AW		= 255,   // Trembling Aspen / Willow
		CT_OA		= 300,   // Oregon White Oak
		CT_OP		= 305,   // Oregon white oak / Ponderosa pine
		CT_DF		= 400,   // Douglas Fir 
		CT_DFMX	= 405,   // Douglas-fir mix
		CT_DFWF	= 410,   // Douglas Fir / White Fir
		CT_GFES	= 425,   // Grand fir / Engelmann spruce
		CT_SFDF	= 435,   // Pacific silver fir / Douglas-fir
		CT_WLLP	= 440,   // Western larch / Lodgepole pine
      CT_TSHE	= 445,   // Western Hemlock
		CT_GF		= 450,   // Grand Fir
		CT_WF		= 455,   // White Fir
		CT_RF		= 505,	// Shasta red fir
		CT_RFWF	= 510,   // Red Fir/ White Fir
		CT_ESAF	= 600,   // Engelmann spruce / Subalpine fir
		CT_MH		= 605,   // Mountain Hemlock
		CT_PICO	= 705,   // Lodgepole Pine
		CT_LPWI	= 710,   // Lodgepole pine / Wildland-Urban Interface
		CT_MXPI	= 750,   // Mixed Pine		
		CT_PIPO	= 755,   // Ponderosa Pine
		CT_PPLP	= 760,   // Ponderosa pine / Lodgepole pine
		CT_WP		= 765,	// Western white pine
		CT_PIJE	= 770,   // Jeffery Pine
		CT_JU		= 900    // Western Juniper
   }; 

// enumeration for Stem Size
enum StemSize
	{ 
		S_BARREN			= 0,
		S_DEVELOPMENT,
      S_MEADOW,
      S_SHRUB,
	   S_SEEDLING_SAPLING,
      S_POLE,
      S_SMALL_TREE,
      S_MEDIUM_TREE,
      S_LARGE_TREE,
      S_GIANT_TREE };

// enumeration for Canopy Cover
enum CanopyCover
	{  
		C_NONE = 0,
      C_LOW,
      C_MEDIUM,
      C_HIGH,
      C_POST_DISTURBANCE 
};

// enumeration for Layering
enum Layering
	{ 
		L_NONE = 0, 
		L_SINGLE = 1,  
		L_MULTI = 2 
};

enum Region
	{ 
		R_BLUE_MOUNTAINS = 7,
		R_OREGON_EAST_CASCADES = 9,
		R_SOUTHWEST_OREGON = 11
};

enum PVT
{
	PVT_REG9_WESTHEMWET = 1,
	PVT_REG9_WESTHEMINTERMED = 2,
	PVT_REG9_WESTHEMCOLD = 3,
	PVT_REG9_MTNHEMLOCKINTERMED = 6,
	PVT_REG9_MIXCONFMOIST = 7,  
	PVT_REG9_MIXCONFCOLDDRY = 17,
	PVT_REG9_MTNHEMLOCK = 18
};

enum ILAP_SPECIES
{
	AM = 0,				// American  Marten
	BBWO = 1,			// Black-backed Woodpecker								
	NOGO = 2,			// Northern Goshawk
	PIWO = 3,			// Pileated Woodpecker
	RNSAP = 4,			// Red-naped Sapsucker
	NSO = 5,				// Spotted Owl
	WEBL= 6,				// Western Bluebird
	WHWO= 7,				// White-headed Woodpecker
	MD	= 8				// Mule Deer
};

// Class exclusively defined to be able to use the protected constructor
// of the MapLayer class. 
class FAGrid : public MapLayer 
{
public:
   // FAGrid constructor allows to use protected constructor of MapLayer class.
   FAGrid(Map *pMap);
   // Default destructor.
   ~FAGrid(){};
};


class PVTKey
{
public:
	PVTKey() : m_speciescode(-1), m_region(-1), m_pvt(-1) { }
	PVTKey(int speciesCode, int region, int pvt) { m_speciescode = speciesCode; m_region = region; m_pvt = pvt; }
	int m_speciescode; 
	int m_region;
	int m_pvt;	
};

class PVTMapCompare
{
public:
	bool operator() (const PVTKey& lhs, const PVTKey& rhs) const
	{
		if (lhs.m_speciescode < rhs.m_speciescode)
			return true;

		if (lhs.m_speciescode == rhs.m_speciescode && lhs.m_region < rhs.m_region)
			return true;

		if (lhs.m_speciescode == rhs.m_speciescode && lhs.m_region == rhs.m_region && lhs.m_pvt < rhs.m_pvt)
			return true;

		return false;
	}
};


// Defines the initialization routines for the DLL. 
// TerrBiodiv is an Envision Evaluative Model and is used to calculate 
// terrestrial habitat suitability for different animal species as well as 
// coarse filter modelts for the FPF (Forest People Fire) Central Oregon Project. 
// The calculation of the suitability for Downy Brome is based on a 
// logistic regression model published by Lovtang & Riegel ("Predicting 
// the Occurrence of Bromus tectorum in Central Oregon") and calculated 
// on IDU level. Information on the habitat suitability index for Spotted
// Owl was provided by Tom Spies and is a composite of nest site suitability
// and landscape suitability, which refers to the suitability of the habitat
// to provide feasible forage for Spotted Owl.

class TerrBiodiv : public EnvEvaluator
{
public:
   // Default constructor, initialize member variables.
   TerrBiodiv( void );
   // Default destructor, free memory.
   ~TerrBiodiv(void);

//   virtual bool Run( EnvContext*, bool useAddDelta);
//	int OutputVar(int id, MODEL_VAR** modelVar);

protected:
   Map *m_pMap;												// Object to handle map pointer required to create grids.
   MapLayer *m_pPolyMapLayer;								// MapLayer object, to hold values of IDU.shp.
	FAGrid *m_pGridMapLayer;								// MapLayer object, used to initialize Poly <-> Grid lookup table.     
   PolyGridLookups     *m_pPolyGridLkUp;				// Object for handling of the Polygon <-> Grid relationship.
   
   CString  msg;												// String for error handling.  
   CString failureID;										// String for error handling.

   LPCSTR m_initStr;

	bool m_useAddDelta;

	int
		m_cellDim,												// Size of the grid cells of the newly created grid
		m_numSubGridCells,									// Number of subgridcells per grid cell of the newly created grid 

		m_speciesCount,

		// needed to prevent duplicate of AddOutputVar entries in Post-Run results
		m_muleDeerID,
		m_muleDeerIndex,
		m_muleDeerVarCount,

		m_HSISpottedOwlID,
		m_HSISpottedOwlIndex,
		m_HSISpottedOwlCount,

		m_ILAPmodelID,
		m_ILAPmodelIndex,
		m_ILAPmodelCount,

		m_downyBromeID,
		m_downyBromeIndex,
		m_downyBromeCount,

		m_HCSpottedOwlID,
		m_HCSpottedOwlIndex,
		m_HCSpottedOwlCount,

		// columns for most models
		m_colIDU,
		m_colPVT,												// Column number in the IDU table that holds information on Potential Vegetation Type (PVT)
		m_colArea,												// Column number in the IDU table that holds information on AREA (m^2)
		m_colRegion,											// Column number in the IDU table that holds information on REGION
		m_colCoverType,										// Column number in the IDU table that holds information on COVER TYPE
		m_colCanopyCover,										// Column number in the IDU table that holds information on CANOPY COVER
		m_colStemSize,											// Column number in the IDU table that holds information on STEM SIZE
		m_colCanopyLayers,									// Column number in the IDU table that holds information on CANOPY LAYER

		// columns for Mule Deer
		m_colMuleDeerForaging,								// Column number in the IDU table that holds Mule Deer Foraging Score
		m_colMuleDeerHiding,									// Column number in the IDU table that holds Mule Deer Hiding Score
		m_colMuleDeerThermal,								// Column number in the IDU table that holds Mule Deer Thermal Cover Score
		m_colMuleDeerLandscape,								// Column number in the IDU table that holds Mule Deer Landscape Score 
		m_colPurshia,											// Column number in the IDU table that holds information on PURSCHIA COVER    

		// columns for Bird Species
		m_colAmericMarten,									// Column number in the IDU table that holds results on American Marten Suitability    
		m_colBlackBackWoodpeck,								// Column number in the IDU table that holds results on Black Backed Woodpecker Suitability  
		m_colNorthernGoshawk,								// Column number in the IDU table that holds results on Northern Goshawk Suitability
		m_colPileatedWoodpeck,								// Column number in the IDU table that holds results on Pileated Woodpecker Suitability
		m_colRedNappedSapsuck,								// Column number in the IDU table that holds results on Red Napped Sapsucker Suitability
		m_colSpottedOwl,										// Column number in the IDU table that holds results on Spotted Owl Suitability
		m_colWesternBluebird,								// Column number in the IDU table that holds results on Western Bluebird Suitability
		m_colWhiteHeadWoodpeck,								// Column number in the IDU table that holds results on White Headed Woodpecker Suitability
      m_colTSF,                                    // Column number in the IDU table that holds the number of years since a time of fire occurence
      m_colPriorVeg,                              // Column number in the IDU table that holds the VEGCLASS of the previous time step
      //		m_colMuleDeer,											// Column number in the IDU table that holds results on Mule Deer Suitability

		// column for Downy Brome
		m_colDistanceRoads,								   // Column number in the IDU table that holds distance to nearest road (m)
		m_colTotalTreesPerHa,								// Column number in the IDU table that holds total number of trees per hectare (per Ha)
		m_colTotalJuniperTreesPerHa,						// Column number in the IDU table that holds total number of Jumiper trees per hectare (per Ha)
		m_colMinTempInMay,									// Column number in the IDU table that holds Minimum Air Temperature in May (C)
		m_colPrecipInMarch,									// Column number in the IDU table that holds Precipitation in March (cm)
		m_colDownyBrome,										// Column number in the IDU table that holds results on Downy Brome Suitability probability

		// columns for VFO Northern Spotted Owl
		m_colNWForPlan,										// Column number in the IDU table for Northwest Forest Plan 1,2,4,5,6 are NSO Habitat
		m_colHCSpottOwl,										// Column number in the IDU table that holds results on VFO Northern Spotted Owl Habitat Classes

		// columns for HSI Northern Spotted Owl
		m_colHSINestSiteScoreOwl,							// Column number in the IDU table that holds the results of the nest site value for HSO Northern Spotted Owl
		m_colHSILandscapeSiteScoreOwl,					// Column number in the IDU table that holds the results of the landscape value for HSO Northern Spotted Owl
		m_colHSIScoreOwl;										// Column number in the IDU table that holds the composite HSI score
    
		float
			m_DownyBromeSuitability,							// Suitability Index for Downy Brome in Maplayer (cheatgrass)
			m_DownyBromeNoSuitabilityArea,					// Total Area (km^2) of No Suitability for Downy Brome (> 0 and <= .1 probability)
			m_DownyBromeLowSuitabilityArea,					// Total Area (km^2) of Low Suitability for Downy Brome (> 1 and <= .3 probability)
			m_DownyBromeModerateSuitabilityArea,			// Total Area (km^2) of Moderate Suitability for Downy Brome ( > .3 and <= .7 probability)
			m_DownyBromeHighSuitabilityArea,					// Total Area (km^2) of High Suitability for Downy Brome ( > .7 and <= 1.0  probability)

			m_upperBound,
			m_lowerBound,
			m_rawScore,

			//  Mule Deer model
			m_landscapeAvgScore,										// Mule Deer Landscape Score (= Foraging + Hiding + Thermal Cover ) stored in Maplayer
			m_foragingScore,											// Mule Deer Foragin Score stored in MapLayer
			m_hidingScore,												// Mule Deer Hiding Score stored in MapLayer
			m_thermalScore,											// Mule Deer Thermal Cover Score stored in MapLayer
			m_lowSuitabilityMuleDeerArea,							// Total area of low suitability for Mule Deer (km2) ( Landscape Habitat Score > 0 and <= 13.33 )
			m_moderateSuitabilityMuleDeerArea,					// Total area of moderate suitability for Mule Deer (km2) ( Landscape Habitat Score > 0 and <= 13.33 )
			m_highSuitabilityMuleDeerArea,						// Total area of high suitability for Mule Deer (km2) ( Landscape Habitat Score > 0 and <= 13.33 )

			m_threshElevation,										// Maximum threshold for Moutain Hemlock NSO elevation = 1800 m
			m_radiusOfHabitat,										// Maximum distance from nest site for NSO = 1 km = 1000 m

			m_nonHabitatHSIOwlSuitabilityArea,					// Total area of Non-Habitat for HSI Northern Spotted Owl (km2)
			m_moderateHabitatHSIOwlSuitabilityArea,			// Total area of Moderate Habitat for HSI Northern Spotted Owl (km2)
			m_goodHabitatHSIOwlSuitabilityArea,					// Total area of Good Habitat for HSI Northern Spotted Owl (km2)

			m_noAmericanMartenSuitabilityArea,					// Total area of no suitability for American Marten (km2)
			m_lowAmericanMartenSuitabilityArea,					// Total area of low suitability for American Marten (km2)
			m_moderateAmericanMartenSuitabilityArea,			// Total area of moderate suitability for American Marten (km2)
			m_highAmericanMartenSuitabilityArea,				// Total area of high suitability for American Marten (km2)

			m_noBBWoodpeckerSuitabilityArea,						// Total area of no suitability for Black-backed Woodpecker (km2)
			m_lowBBWoodpeckerSuitabilityArea,					// Total area of low suitability for Black-backed Woodpecker (km2)
			m_moderateBBWoodpeckerSuitabilityArea,				// Total area of moderate suitability for Black-backed Woodpecker (km2)
			m_highBBWoodpeckerSuitabilityArea,					// Total area of high suitability for Black-backed Woodpecker (km2)

			m_noNorthernGoshawkSuitabilityArea,					// Total area of low suitability for Northtern Goshawk (km2)
			m_lowNorthernGoshawkSuitabilityArea,				// Total area of low suitability for Northtern Goshawk (km2)
			m_moderateNorthernGoshawkSuitabilityArea,			// Total area of modeerate suitability for Northtern Goshawk (km2)
			m_highNorthernGoshawkSuitabilityArea,				// Total area of high suitability for Northtern Goshawk (km2)		

			m_noPileatedWoodpeckerSuitabilityArea,				// Total area of nosuitability for PileatedWoodpecker (km2)
			m_lowPileatedWoodpeckerSuitabilityArea,			// Total area of low suitability for PileatedWoodpecker (km2)
			m_moderatePileatedWoodpeckerSuitabilityArea,		// Total area of moderate suitability for PileatedWoodpecker (km2)
			m_highPileatedWoodpeckerSuitabilityArea,			// Total area of high suitability for PileatedWoodpecker (km2)

			m_noRedNSapsuckerSuitabilityArea,					// Total area of no suitability for Red-naped/Red-breasted Sapsucker (km2)
			m_lowRedNSapsuckerSuitabilityArea,					// Total area of low suitability for Red-naped/Red-breasted Sapsucker (km2)
			m_moderateRedNSapsuckerSuitabilityArea,			// Total area of moderate suitability for Red-naped/Red-breasted Sapsucker (km2)
			m_highRedNSapsuckerSuitabilityArea,					// Total area of high suitability for Red-naped/Red-breasted Sapsucker (km2)		

			m_noSpottedOwlSuitabilityArea,						// Total area of no suitability for Spotted Owl (km2)
			m_lowSpottedOwlSuitabilityArea,						// Total area of low suitability for Spotted Owl (km2)
			m_moderateSpottedOwlSuitabilityArea,				// Total area of moderate suitability for Spotted Owl (km2)
			m_highSpottedOwlSuitabilityArea,						// Total area of high suitability for Spotted Owl (km2)	

			m_noWesternBluebirdSuitabilityArea,					// Total area of no suitability for Western Bluebird (km2)
			m_lowWesternBluebirdSuitabilityArea,				// Total area of low suitability for Western Bluebird (km2)
			m_moderateWesternBluebirdSuitabilityArea,			// Total area of mdoerate suitability for Western Bluebird (km2)
			m_highWesternBluebirdSuitabilityArea,				// Total area of high suitability for Western Bluebird (km2)

			m_noWHWoodpeckerSuitabilityArea,						// Total area of no suitability for White-headed Woodpecker (km2)
			m_lowWHWoodpeckerSuitabilityArea,					// Total area of low suitability for White-headed Woodpecker (km2)
			m_moderateWHWoodpeckerSuitabilityArea,				// Total area of moderate suitability for White-headed Woodpecker (km2)
			m_highWHWoodpeckerSuitabilityArea,					// Total area of high suitability for White-headed Woodpecker (km2)

			m_nonHabitatVFOOwlArea,									// Total area of Non-Habitat for FVO Northern Spotted Owl (km2)
			m_moderateHabitatVFOOwlArea,							// Total area of Moderate Habitat for FVO Northern Spotted Owl (km2)
			m_goodHabitatVFOOwlArea;								// Total area of Good Habitat for FVO Northern Spotted Owl (km2)


	FDataObj m_muleDeerSuitabilityArray;
	FDataObj m_VFOSpottedOwlSuitabilityArray;
	FDataObj m_downyBromeSuitabilityArray;
	FDataObj m_HSISpottedOwlSuitabilityArray;

	FDataObj m_AmericanMartenSuitabilityArray;
	FDataObj m_BBWoodpeckerSuitabilityArray;
	FDataObj m_NorthernGoshawkSuitabilityArray;
	FDataObj m_PileatedWoodpeckerSuitabilityArray;
	FDataObj m_RedNSapsuckerSuitabilityArray;
	FDataObj m_SpottedOwlSuitabilityArray;
	FDataObj m_WesternBluebirdSuitabilityArray;
	FDataObj m_WHWoodpeckerSuitabilityArray;

	// Lookup tables that are read in
	VDataObj m_ILAPSizeLUT;
	VDataObj m_ILAPCanopyCoverLUT;
	VDataObj m_ILAPCanopyLayerLUT;
	VDataObj m_ILAPCoverTypeLUT;

	VDataObj m_muleDeerPVTLUT;
	VDataObj m_ILAPPVTLUT;

	PVTKey m_pvtHashKey;

	CMap< int, int, int, int > m_coverTypeColIndexMap;
	map< int, int > m_owlHabitatScoreMap;
	map< PVTKey, int, PVTMapCompare> m_muleDeerPVTMap;
	map< PVTKey, int, PVTMapCompare> m_ILAPPVTMap;


protected:

	bool _Run(EnvContext *pEnvContext);
	bool LoadILAPSizeTable(LPTSTR sizeScoreTable);
	bool LoadILAPCanopyCoverTable(LPTSTR canopyCoverScoreTable);
	bool LoadILAPCanopyLayerTable(LPTSTR canopyLayerScoreTable);
	bool LoadILAPCoverTypeTable(LPTSTR coverTypeScoreTable);
	bool LoadPVTTable(LPTSTR pvtScoreTable, VDataObj &pvtLUT, map<PVTKey, int, PVTMapCompare> &pvtMap);

	bool CreateILAPCoverTypeMap();

	bool CalculateILAPSpeciesSuitability(EnvContext *pEnvContext);
   bool LoadILAPLookupTable( EnvContext *pEnvContext, LPCTSTR initStr );
   
	//bool WriteILAPLookupData( EnvContext *pEnvContext );

	bool WriteILAPSuitabilityAreas(EnvContext *pEnvContext, float noSpeciesSuitabilityArea, float lowSpeciesSuitabilityArea, float moderateSpeciesSuitabilityArea, float highSpeciesSuitabilityArea, FDataObj &ILAPSpeciesSuitabilityArray);
  
   // Initialize the Polygon <-> Grid relationship.
   // Uses the PolyGridLookups class to set up the spatial correlation between the IDU shapefile and a grid,
   // that is created based on the extent of the shapefile, a cellsize given by the variable m_cellDim and 
   // a number of subgridcells specified by the variable m_numSubGridCells. Furthermore, the calculated 
   // PolyGridLookup relation is written to a binary file (*.pgl), in case such a file does not yet exist.
   bool LoadPolyGridLookup( EnvContext *pContext );

   // Calculates the nest site habitat suitability index and then the landscape habitat suitability index and 
   // subsequently combines these two compartiments into the total habitat suitability index for spotted owl. 
   bool CalculateHSINorthenSpottedOwlSuitability( EnvContext *pContext );
 
   // Calculates the habitat suitability for Downy Brome on IDU level. Important: Only the odds of the logistic
   // regression equation is calculated (p/(1-p) --> ratio of probatility that downy brome is present (p) vs. 
   // downy brome is not present (1-p)), which means that values range from 0 to + infinity and are not comparable
   // to the Spotted Owl HSI that ranges from 0 to 1.
   bool CalculateDownyBromeSuitability( EnvContext *pContext );
  
   // Calculates the suitability score of habitat for Mule Deer, based on habitat model provided by Anita Morzillo.
   // Suitability score is based on a window approach considering the three suitability layers foraging habitat score,
   // hiding habitat score, and thermal cover score.
   bool CalculateMuleDeerSuitability(EnvContext *pContext );

	int CalcMuleDeerForagingHabitatScore(int coverType, int sizeClass, int canopyCover, int canopyLayers, float purshiaCover);
	int CalcMuleDeerHidingHabitatScore(int coverType, int sizeClass, int canopyCover, int canopyLayers); 
	int CalcMuleDeerThermalCoverScore(int coverType, int sizeClass, int canopyCover, int canopyLayers);
	int CalcMuleDeerLandscapeScore(EnvContext *pContext, int iduCount);

   // Combine the HSIN and HSIL values for Spottet Owl calculated on the grid cell level to a HSI value and 
   // calculate an "area weighted mean" of the resulting HSI at IDU level. The precision of the "area weighting
   // is determined by the number of subgridcells given by the variable m_numSubGridCells. 
	bool CalculateVFONorthernSpottedOwl( EnvContext *pEnvContext );
  
   void SetGridCellSize( int val ) { m_cellDim = val; };
   int  GetGridCellSize( void ) { return m_cellDim; };
   void SetNumberSubGridCells( int val ) { m_numSubGridCells = val; };
   int  GetNumberSubGridCells( void ) { return m_numSubGridCells; };
  
	bool RunHSINorthernSpottedOwl( EnvContext *pEnvContext );
   bool RunMuleDeerHSI( EnvContext *pEnvContext );
   bool RunILAPHabitatModels( EnvContext *pEnvContext );
   bool RunDownyBrome( EnvContext *pEnvContext );
   bool RunHCNorthernSpottedOwl( EnvContext *pEnvContext );

public:
   
	// Initialize and carry out procedure to generate baseyear conditions.
   // Get IDU.shp MapLayer, request dynamic memory for m_pMapExprEngine,
   // call LoadXml() to read the expressions from xml file , compile 
   // expressions derived from xml file and call Evaluate().
   bool Init( EnvContext *pContext, LPCTSTR initStr );
	bool InitRun(EnvContext *pContext, bool useInitSeed);

	bool LoadMuleDeerXML(LPCTSTR initStr);
	bool LoadILAPXML(LPCTSTR initStr);

   // Carry out procedure by calling CalculateSuitability().
   bool Run( EnvContext *pContext );
	bool Run(EnvContext *pContext, bool useAddDelta);
};

#endif /* TERRBIODIV_H */