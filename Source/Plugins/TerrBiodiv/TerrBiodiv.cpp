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
// TerrBiodiv.cpp : Defines the initialization routines for the DLL.
//
#include "stdafx.h"
#include "TerrBiodiv.h"
#include <EnvConstants.h>
#include <Path.h>
#include <PathManager.h>
#include <UNITCONV.H>
#include "AlgLib\ap.h"
#include <tixml.h>

//#include <EnvDoc.h>

using namespace alglib_impl;
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension* Factory(EnvContext *pContext)
   {
   return (EnvExtension*) new TerrBiodiv;
   }



#define FailAndReturn(FailMsg) \
   {msg = failureID + (_T(FailMsg)); \
      Report::ErrorMsg(msg); \
      return false; }

// Exclusively defined to be able to use the protected Constructor of the MapLayer class. 
FAGrid::FAGrid(Map *pMap)
   : MapLayer(pMap)
   {
   }

TerrBiodiv::TerrBiodiv(void) 
   : EnvEvaluator()
,
m_cellDim(-1),												// Size of the grid cells of the newly created grid
m_numSubGridCells(-1),									// Number of subgridcells per grid cell of the newly created grid 
m_speciesCount(-1),
m_useAddDelta(false),

// columns for most models
m_colPVT(-1),												// Column number in the IDU table that holds information on Potential Vegetation Type (PVT)
m_colArea(-1),												// Column number in the IDU table that holds information on AREA (m^2)
m_colRegion(-1),											// Column number in the IDU table that holds information on REGION
m_colCoverType(-1),										// Column number in the IDU table that holds information on COVER TYPE
m_colCanopyCover(-1),									// Column number in the IDU table that holds information on CANOPY COVER
m_colStemSize(-1),										// Column number in the IDU table that holds information on STEM SIZE
m_colCanopyLayers(-1),									// Column number in the IDU table that holds information on CANOPY LAYER

// evaluative model scores - not used in this implementation
m_upperBound(0.0f),
m_lowerBound(0.0f),
m_rawScore(0.0f),

// columns for Mule Deer
m_colMuleDeerForaging(-1),								// Column number in the IDU table that holds Mule Deer Foraging Score
m_colMuleDeerHiding(-1),								// Column number in the IDU table that holds Mule Deer Hiding Score
m_colMuleDeerThermal(-1),								// Column number in the IDU table that holds Mule Deer Thermal Cover Score
m_colMuleDeerLandscape(-1),							// Column number in the IDU table that holds Mule Deer Landscape Score (= Foraging + Hiding + Thermal Cover )
m_colPurshia(-1),											// Column number in the IDU table that holds information on PURSCHIA COVER    

// columns for Bird Species
m_colAmericMarten(-1),									// Column number in the IDU table that holds results on American Marten Suitability    
m_colBlackBackWoodpeck(-1),							// Column number in the IDU table that holds results on Black Backed Woodpecker Suitability  
m_colNorthernGoshawk(-1),								// Column number in the IDU table that holds results on Northern Goshawk Suitability
m_colPileatedWoodpeck(-1),								// Column number in the IDU table that holds results on Pileated Woodpecker Suitability
m_colRedNappedSapsuck(-1),								// Column number in the IDU table that holds results on Red Napped Sapsucker Suitability
m_colSpottedOwl(-1),										// Column number in the IDU table that holds results on Spotted Owl Suitability
m_colWesternBluebird(-1),								// Column number in the IDU table that holds results on Western Bluebird Suitability
m_colWhiteHeadWoodpeck(-1),							// Column number in the IDU table that holds results on White Headed Woodpecker Suitability
m_colTSF(-1),                                   // Column number in the IDU table that holds the number of years since a time of fire occurence
m_colPriorVeg(-1),                              // Column number in the IDU table that holds the VEGCLASS of the previous time step
//	m_colMuleDeer(-1),										// Column number in the IDU table that holds results on Mule Deer Suitability

   // columns for Downy Brome
   m_colDistanceRoads(-1),									// Column number in the IDu table that holds distance to nearest road (m)
   m_colTotalTreesPerHa(-1),								// Column number in the IDU table that holds total number of trees per hectare (per Ha)
   m_colTotalJuniperTreesPerHa(-1),						// Column number in the IDU table that holds total number of Juniper trees per hectare (per Ha)
   m_colMinTempInMay(-1),									// Column number in the IDU table that holds Minimum Air Temperature in May (C)
   m_colPrecipInMarch(-1),									// Column number in the IDU table that holds Precipitation in March (cm)
   m_colDownyBrome(-1),										// Column number in the IDU table that holds results on Downy Brome Suitability probability

   // columns for VFO Northern Spotted Owl
   m_colNWForPlan(-1),										// Column number in the IDU table for Northwest Forest Plan 1,2,4,5,6 are NSO Habitat
   m_colHCSpottOwl(-1),										// Column number in the IDU table that holds results on VFO Northern Spotted Owl Habitat Classes

   // columns for HSI Northern Spotted Owl
   m_colHSINestSiteScoreOwl(-1),							// Column number in the IDU table that holds the results of the nest site value for HSO Northern Spotted Owl
   m_colHSILandscapeSiteScoreOwl(-1),					// Column number in the IDU table that holds the results of the landscape value for HSO Northern Spotted Owl
   m_colHSIScoreOwl(-1), 									// Column number in the IDU table that holds the composite HSI score

   // values for Downy Brome
   m_DownyBromeSuitability(0.0f),						// Suitability Index for Downy Brome in Maplayer (cheatgrass)
   m_DownyBromeNoSuitabilityArea(0.0f),				// Total Area (km^2) of No Suitability for Downy Brome (> 0 and <= .1 probability)
   m_DownyBromeLowSuitabilityArea(0.0f),				// Total Area (km^2) of Low Suitability for Downy Brome (> .1 and <= .3 probability)
   m_DownyBromeModerateSuitabilityArea(0.0f),		// Total Area (km^2) of Moderate Suitability for Downy Brome ( > .3 and <= .7 probability)
   m_DownyBromeHighSuitabilityArea(0.0f),				// Total Area (km^2) of High Suitability for Downy Brome ( > .7 and <= 1.0  probability)

   // values for Mule Deer
   m_landscapeAvgScore(0.0f),								// Mule Deer Landscape Score (= Foraging + Hiding + Thermal Cover ) stored in Maplayer
   m_foragingScore(0.0f),									// Mule Deer Foragin Score stored in MapLayer
   m_hidingScore(0.0f),										// Mule Deer Hiding Score stored in MapLayer
   m_thermalScore(0.0f),									// Mule Deer Thermal Cover Score stored in MapLayer

   m_lowSuitabilityMuleDeerArea(0.0f),					// Total area of low suitability for Mule Deer (km2) ( Landscape Habitat Score > 0 and <= 13.33 )
   m_moderateSuitabilityMuleDeerArea(0.0f),			// Total area of moderate suitability for Mule Deer (km2) ( Landscape Habitat Score > 0 and <= 13.33 )
   m_highSuitabilityMuleDeerArea(0.0f),				// Total area of high suitability for Mule Deer (km2) ( Landscape Habitat Score > 0 and <= 13.33 )

   m_threshElevation(0.0f),								// Maximum threshold for Moutain Hemlock NSO elevation = 1800 m
   m_radiusOfHabitat(0.0f),								// Maximum distance from nest site for NSO = 1 km = 1000 m

   // values for HSI Northern Spotted Owl
   m_nonHabitatHSIOwlSuitabilityArea(0.0f),					// Total area of Non-Habitat for HSI Northern Spotted Owl (km2)
   m_moderateHabitatHSIOwlSuitabilityArea(0.0f),			// Total area of Moderate Habitat for HSI Northern Spotted Owl (km2)
   m_goodHabitatHSIOwlSuitabilityArea(0.0f),					// Total area of Good Habitat for HSI Northern Spotted Owl (km2)

   m_ILAPSizeLUT(U_UNDEFINED),
   m_ILAPCanopyCoverLUT(U_UNDEFINED),
   m_ILAPCanopyLayerLUT(U_UNDEFINED),
   m_ILAPCoverTypeLUT(U_UNDEFINED),
   m_muleDeerPVTLUT(U_UNDEFINED),
   m_ILAPPVTLUT(U_UNDEFINED),

   // values for ILAP model 
   m_noAmericanMartenSuitabilityArea(0.0f),					// Total area of no suitability for American Marten (km2)
   m_lowAmericanMartenSuitabilityArea(0.0f),					// Total area of low suitability for American Marten (km2)
   m_moderateAmericanMartenSuitabilityArea(0.0f),			// Total area of moderate suitability for American Marten (km2)
   m_highAmericanMartenSuitabilityArea(0.0f),				// Total area of high suitability for American Marten (km2)

   m_noBBWoodpeckerSuitabilityArea(0.0f),						// Total area of no suitability for Black-backed Woodpecker (km2)
   m_lowBBWoodpeckerSuitabilityArea(0.0f),					// Total area of low suitability for Black-backed Woodpecker (km2)
   m_moderateBBWoodpeckerSuitabilityArea(0.0f),				// Total area of moderate suitability for Black-backed Woodpecker (km2)
   m_highBBWoodpeckerSuitabilityArea(0.0f),					// Total area of high suitability for Black-backed Woodpecker (km2)	

   m_noNorthernGoshawkSuitabilityArea(0.0f),					// Total area of no suitability for Northtern Goshawk (km2)
   m_lowNorthernGoshawkSuitabilityArea(0.0f),				// Total area of low suitability for Northtern Goshawk (km2)
   m_moderateNorthernGoshawkSuitabilityArea(0.0f),			// Total area of moderate suitability for Northtern Goshawk (km2)
   m_highNorthernGoshawkSuitabilityArea(0.0f),				// Total area of high suitability for Northtern Goshawk (km2)

   m_noPileatedWoodpeckerSuitabilityArea(0.0f),				// Total area of no suitability for PileatedWoodpecker (km2)
   m_lowPileatedWoodpeckerSuitabilityArea(0.0f),			// Total area of low suitability for PileatedWoodpecker (km2)
   m_moderatePileatedWoodpeckerSuitabilityArea(0.0f),		// Total area of moderate suitability for PileatedWoodpecker (km2)
   m_highPileatedWoodpeckerSuitabilityArea(0.0f),			// Total area of high suitability for PileatedWoodpecker (km2)

   m_noRedNSapsuckerSuitabilityArea(0.0f),					// Total area of no suitability for Red-naped/Red-breasted Sapsucker (km2)
   m_lowRedNSapsuckerSuitabilityArea(0.0f),					// Total area of low suitability for Red-naped/Red-breasted Sapsucker (km2)
   m_moderateRedNSapsuckerSuitabilityArea(0.0f),			// Total area of moderate suitability for Red-naped/Red-breasted Sapsucker (km2)
   m_highRedNSapsuckerSuitabilityArea(0.0f),					// Total area of high suitability for Red-naped/Red-breasted Sapsucker (km2)

   m_lowSpottedOwlSuitabilityArea(0.0f),						// Total area of low suitability for Spotted Owl (km2)
   m_moderateSpottedOwlSuitabilityArea(0.0f),				// Total area of moderate suitability for Spotted Owl (km2)
   m_highSpottedOwlSuitabilityArea(0.0f),						// Total area of high suitability for Spotted Owl (km2	

   m_noWesternBluebirdSuitabilityArea(0.0f),					// Total area of no suitability for Western Bluebird (km2)
   m_lowWesternBluebirdSuitabilityArea(0.0f),				// Total area of low suitability for Western Bluebird (km2)
   m_moderateWesternBluebirdSuitabilityArea(0.0f),			// Total area of mdoerate suitability for Western Bluebird (km2)
   m_highWesternBluebirdSuitabilityArea(0.0f),				// Total area of high suitability for Western Bluebird (km2)

   m_noWHWoodpeckerSuitabilityArea(0.0f),						// Total area of no suitability for White-headed Woodpecker (km2)
   m_lowWHWoodpeckerSuitabilityArea(0.0f),					// Total area of low suitability for White-headed Woodpecker (km2)
   m_moderateWHWoodpeckerSuitabilityArea(0.0f),				// Total area of moderate suitability for White-headed Woodpecker (km2)
   m_highWHWoodpeckerSuitabilityArea(0.0f),					// Total area of high suitability for White-headed Woodpecker (km2)

   m_nonHabitatVFOOwlArea(0.0f),									// Total area of Non-Habitat for FVO Northern Spotted Owl (km2)
   m_moderateHabitatVFOOwlArea(0.0f),							// Total area of Moderate Habitat for FVO Northern Spotted Owl (km2)
   m_goodHabitatVFOOwlArea(0.0f),								// Total area of Good Habitat for FVO Northern Spotted Owl (km2)

   //
   m_muleDeerID(-1),
   m_muleDeerIndex(0),
   m_muleDeerVarCount(0),

   m_HSISpottedOwlID(-1),
   m_HSISpottedOwlIndex(0),
   m_HSISpottedOwlCount(0),

   m_ILAPmodelID(-1),
   m_ILAPmodelIndex(0),
   m_ILAPmodelCount(0),

   m_downyBromeID(-1),
   m_downyBromeIndex(0),
   m_downyBromeCount(0),

   m_HCSpottedOwlID(-1),
   m_HCSpottedOwlIndex(0),
   m_HCSpottedOwlCount(0),

   // various grids
   m_pGridMapLayer(NULL),
   m_pPolyGridLkUp(NULL)
   {
   // define error string
   failureID = (_T("Error: "));
   }

TerrBiodiv::~TerrBiodiv(void)
   {
   // grid map layers for nothern spotted owl model
   if (m_pGridMapLayer) delete m_pGridMapLayer;
   // poly to grid lookup table
   if (m_pPolyGridLkUp) delete m_pPolyGridLkUp;
   }

bool TerrBiodiv::Init(EnvContext *pEnvContext, LPCSTR initStr)
   {
   TRACE(_T("Starting TerrBiodiv.cpp Init.\n"));
   ASSERT(pEnvContext != NULL); ASSERT(initStr != NULL);

   // get pointer to IDU MapLayer
   m_pPolyMapLayer = (MapLayer *)pEnvContext->pMapLayer;

   // get Map pointer
   m_pMap = m_pPolyMapLayer->GetMapPtr();

   // map layers for nothern spotted owl model
   m_pGridMapLayer = new FAGrid(m_pMap);

   // set values for column numbers in IDU shapefile
   // get column numbers from IDU table for use of various models
   CheckCol(pEnvContext->pMapLayer, m_colPVT, _T("PVT"), TYPE_INT, CC_MUST_EXIST);
   CheckCol(pEnvContext->pMapLayer, m_colCoverType, _T("CoverType"), TYPE_INT, CC_MUST_EXIST);
   CheckCol(pEnvContext->pMapLayer, m_colCanopyCover, _T("CANOPY"), TYPE_INT, CC_MUST_EXIST);
   CheckCol(pEnvContext->pMapLayer, m_colStemSize, _T("SIZE"), TYPE_INT, CC_MUST_EXIST);
   CheckCol(pEnvContext->pMapLayer, m_colCanopyLayers, _T("LAYERS"), TYPE_INT, CC_MUST_EXIST);
   CheckCol(pEnvContext->pMapLayer, m_colArea, _T("Area"), TYPE_DOUBLE, CC_MUST_EXIST);
   CheckCol(pEnvContext->pMapLayer, m_colRegion, _T("REGION"), TYPE_INT, CC_MUST_EXIST);
   CheckCol(pEnvContext->pMapLayer, m_colIDU, _T("IDU_INDEX"), TYPE_INT, CC_MUST_EXIST);

   // choose eval model, in order to add corresponding output variables, via context identifier specified in .envx file
   switch (pEnvContext->id)
      {
      // HSI Mule Deer
      case 2:
         {
         m_muleDeerID = 2;
         // get column numbers required for the Mule Deer model
         CheckCol(pEnvContext->pMapLayer, m_colMuleDeerForaging, _T("MD_For"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colMuleDeerHiding, _T("MD_Hide"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colMuleDeerThermal, _T("MD_Therm"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colMuleDeerLandscape, _T("MD_AvgSc"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colPurshia, _T("PurshiaCov"), TYPE_DOUBLE, CC_MUST_EXIST);

         m_muleDeerSuitabilityArray.SetName("Total area for each Mule Deer Suitability Category [km2]");
         m_muleDeerSuitabilityArray.SetSize(4, 0);
         m_muleDeerSuitabilityArray.SetLabel(0, "Time (year)");
         m_muleDeerSuitabilityArray.SetLabel(1, "Low (0-13) Mule Deer Suitability Area [km2]");
         m_muleDeerSuitabilityArray.SetLabel(2, "Moderate (+13-26) Mule Deer Suitability Area [km2]");
         m_muleDeerSuitabilityArray.SetLabel(3, "High (+26) Mule Deer Suitability Area [km2]");
         m_muleDeerIndex = AddOutputVar(_T("Mule Deer Suitability [km2]"), &m_muleDeerSuitabilityArray, _T("Area [km2] for Mule Deer"));
         m_muleDeerVarCount = 1;

         bool ok = LoadMuleDeerXML(initStr);

         if (!ok)
            return false;
         }
         break;

         // HSI Northern Spotted Owl
      case 3:
         {
         m_HSISpottedOwlID = 3;
         //get column numbers HSI Northern Spotted Owl model
         CheckCol(pEnvContext->pMapLayer, m_colNWForPlan, _T("NWForPlan"), TYPE_INT, CC_MUST_EXIST);
         CheckCol(pEnvContext->pMapLayer, m_colHSINestSiteScoreOwl, _T("HSIN"), TYPE_DOUBLE, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colHSILandscapeSiteScoreOwl, _T("HSIL"), TYPE_DOUBLE, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colHSIScoreOwl, _T("HSI"), TYPE_DOUBLE, CC_AUTOADD);

         m_HSISpottedOwlSuitabilityArray.SetName("Total area for each HSI Spotted Owl Suitability Category [km2]");
         m_HSISpottedOwlSuitabilityArray.SetSize(4, 0);
         m_HSISpottedOwlSuitabilityArray.SetLabel(0, "Time (year)");
         m_HSISpottedOwlSuitabilityArray.SetLabel(1, "Non-Habitat HSI Spotted Owl Suitability Area [km2]");
         m_HSISpottedOwlSuitabilityArray.SetLabel(2, "Moderate HSI Spotted Owl Suitability Area [km2]");
         m_HSISpottedOwlSuitabilityArray.SetLabel(3, "Good HSI Spotted Owl Suitability Area [km2]");
         m_HSISpottedOwlIndex = AddOutputVar(_T("HSI Northern Spotted Owl Suitability [km2]"), &m_HSISpottedOwlSuitabilityArray, _T("Area [km2] for Northern Spotted Owl"));
         m_HSISpottedOwlCount = 1;
         }
         break;

         // ILAP Habitat Models
      case 4:
         {
         m_ILAPmodelID = 4;
         // get column numbers for the different binary[0 / 1] ILAP habitat models
         CheckCol(pEnvContext->pMapLayer, m_colAmericMarten, _T("AmMarten"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colBlackBackWoodpeck, _T("BBWoodpeck"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colNorthernGoshawk, _T("NorGoshawk"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colPileatedWoodpeck, _T("PiWoodpeck"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colRedNappedSapsuck, _T("RNSapsuck"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colSpottedOwl, _T("SpottedOwl"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colWesternBluebird, _T("WeBluebird"), TYPE_INT, CC_AUTOADD);
         CheckCol(pEnvContext->pMapLayer, m_colWhiteHeadWoodpeck, _T("WHWoodpeck"), TYPE_INT, CC_AUTOADD);

         CheckCol(pEnvContext->pMapLayer, m_colNWForPlan, _T("NWForPlan"), TYPE_INT, CC_MUST_EXIST);
         CheckCol(pEnvContext->pMapLayer, m_colTSF, _T("TSF"), TYPE_INT, CC_MUST_EXIST);
         CheckCol(pEnvContext->pMapLayer, m_colPriorVeg, _T("PRIOR_VEG"), TYPE_INT, CC_MUST_EXIST);


         m_AmericanMartenSuitabilityArray.SetName("Total area for each American Marten Suitability Category [km2]");
         m_AmericanMartenSuitabilityArray.SetSize(5, 0);
         m_AmericanMartenSuitabilityArray.SetLabel(0, "Time (year)");
         m_AmericanMartenSuitabilityArray.SetLabel(1, "No American Marten Suitability Area [km2]");
         m_AmericanMartenSuitabilityArray.SetLabel(2, "Low American Marten Suitability Area [km2]");
         m_AmericanMartenSuitabilityArray.SetLabel(3, "Moderate American Marten Suitability Area [km2]");
         m_AmericanMartenSuitabilityArray.SetLabel(4, "High American Marten Suitability Area [km2]");
         m_ILAPmodelIndex = AddOutputVar(_T("American Marten Suitability [km2]"), &m_AmericanMartenSuitabilityArray, _T("Area [km2] for American Marten"));

         m_BBWoodpeckerSuitabilityArray.SetName("Total area for each Black-backed Woodpeacker Suitability Category [km2]");
         m_BBWoodpeckerSuitabilityArray.SetSize(5, 0);
         m_BBWoodpeckerSuitabilityArray.SetLabel(0, "Time (year)");
         m_BBWoodpeckerSuitabilityArray.SetLabel(1, "No Black-backed Woodpeacker Suitability Area [km2]");
         m_BBWoodpeckerSuitabilityArray.SetLabel(2, "Low Black-backed Woodpeacker Suitability Area [km2]");
         m_BBWoodpeckerSuitabilityArray.SetLabel(3, "Moderate Black-backed Woodpeacker Suitability Area [km2]");
         m_BBWoodpeckerSuitabilityArray.SetLabel(4, "High Black-backed Woodpeacker Suitability Area [km2]");
         AddOutputVar(_T("Black-backed Woodpecker Suitability [km2]"), &m_BBWoodpeckerSuitabilityArray, _T("Area [km2] for Black-backed Woodpecker"));

         m_NorthernGoshawkSuitabilityArray.SetName("Total area for each Northern Goshawk Suitability Category [km2]");
         m_NorthernGoshawkSuitabilityArray.SetSize(5, 0);
         m_NorthernGoshawkSuitabilityArray.SetLabel(0, "Time (year)");
         m_NorthernGoshawkSuitabilityArray.SetLabel(1, "No Northern Goshawk Suitability Area [km2]");
         m_NorthernGoshawkSuitabilityArray.SetLabel(2, "Low Northern Goshawk Suitability Area [km2]");
         m_NorthernGoshawkSuitabilityArray.SetLabel(3, "Moderate Northern Goshawk Suitability Area [km2]");
         m_NorthernGoshawkSuitabilityArray.SetLabel(4, "High Northern Goshawk Suitability Area [km2]");
         AddOutputVar(_T("Northern Goshawk Suitability [km2]"), &m_NorthernGoshawkSuitabilityArray, _T("Area [km2] for Northern Goshawk"));

         m_PileatedWoodpeckerSuitabilityArray.SetName("Total area for each Pileated Woodpecker Suitability Category [km2]");
         m_PileatedWoodpeckerSuitabilityArray.SetSize(5, 0);
         m_PileatedWoodpeckerSuitabilityArray.SetLabel(0, "Time (year)");
         m_PileatedWoodpeckerSuitabilityArray.SetLabel(1, "No Pileated Woodpecker Suitability Area [km2]");
         m_PileatedWoodpeckerSuitabilityArray.SetLabel(2, "Low Pileated Woodpecker Suitability Area [km2]");
         m_PileatedWoodpeckerSuitabilityArray.SetLabel(3, "Moderate Pileated Woodpecker Suitability Area [km2]");
         m_PileatedWoodpeckerSuitabilityArray.SetLabel(4, "High Pileated Woodpecker Suitability Area [km2]");
         AddOutputVar(_T("Pileated Woodpecker Suitability [km2]"), &m_PileatedWoodpeckerSuitabilityArray, _T("Area [km2] for Pileated Woodpecker"));

         m_RedNSapsuckerSuitabilityArray.SetName("Total area for each Red-Naped Sapsucker Suitability Category [km2]");
         m_RedNSapsuckerSuitabilityArray.SetSize(5, 0);
         m_RedNSapsuckerSuitabilityArray.SetLabel(0, "Time (year)");
         m_RedNSapsuckerSuitabilityArray.SetLabel(1, "No Red-Naped Sapsucker Suitability Area [km2]");
         m_RedNSapsuckerSuitabilityArray.SetLabel(2, "Low Red-Naped Sapsucker Suitability Area [km2]");
         m_RedNSapsuckerSuitabilityArray.SetLabel(3, "Moderate Red-Naped Sapsucker Suitability Area [km2]");
         m_RedNSapsuckerSuitabilityArray.SetLabel(4, "High Red-Naped Sapsucker Suitability Area [km2]");
         AddOutputVar(_T("Red-Naped Sapsucker Suitability [km2]"), &m_RedNSapsuckerSuitabilityArray, _T("Area [km2] for Red-Naped Sapsucker"));

         m_SpottedOwlSuitabilityArray.SetName("Total area for each Spotted Owl Suitability Category [km2]");
         m_SpottedOwlSuitabilityArray.SetSize(5, 0);
         m_SpottedOwlSuitabilityArray.SetLabel(0, "Time (year)");
         m_SpottedOwlSuitabilityArray.SetLabel(1, "No Spotted Owl Suitability Area [km2]");
         m_SpottedOwlSuitabilityArray.SetLabel(2, "Low Spotted Owl Suitability Area [km2]");
         m_SpottedOwlSuitabilityArray.SetLabel(3, "Moderate Spotted Owl Suitability Area [km2]");
         m_SpottedOwlSuitabilityArray.SetLabel(4, "High Spotted Owl Suitability Area [km2]");
         AddOutputVar(_T("Spotted Owl Suitability [km2]"), &m_SpottedOwlSuitabilityArray, _T("Area [km2] for Spotted Owl"));

         m_WesternBluebirdSuitabilityArray.SetName("Total area for each Western Bluebird Suitability Category [km2]");
         m_WesternBluebirdSuitabilityArray.SetSize(5, 0);
         m_WesternBluebirdSuitabilityArray.SetLabel(0, "Time (year)");
         m_WesternBluebirdSuitabilityArray.SetLabel(1, "No Western Bluebird Suitability Area [km2]");
         m_WesternBluebirdSuitabilityArray.SetLabel(2, "Low Western Bluebird Suitability Area [km2]");
         m_WesternBluebirdSuitabilityArray.SetLabel(3, "Moderate Western Bluebird Suitability Area [km2]");
         m_WesternBluebirdSuitabilityArray.SetLabel(4, "High Western Bluebird Suitability Area [km2]");
         AddOutputVar(_T("Western Bluebird Suitability [km2]"), &m_WesternBluebirdSuitabilityArray, _T("Area [km2] for Western Bluebird"));

         m_WHWoodpeckerSuitabilityArray.SetName("Total area for each White-Headed Woodpecker Suitability Category [km2]");
         m_WHWoodpeckerSuitabilityArray.SetSize(5, 0);
         m_WHWoodpeckerSuitabilityArray.SetLabel(0, "Time (year)");
         m_WHWoodpeckerSuitabilityArray.SetLabel(1, "No White-Headed Woodpecker Suitability Area [km2]");
         m_WHWoodpeckerSuitabilityArray.SetLabel(2, "Low White-Headed Woodpecker Suitability Area [km2]");
         m_WHWoodpeckerSuitabilityArray.SetLabel(3, "Moderate White-Headed Woodpecker Suitability Area [km2]");
         m_WHWoodpeckerSuitabilityArray.SetLabel(4, "High White-Headed Woodpecker Suitability Area [km2]");
         AddOutputVar(_T("White-Headed Woodpecker Suitability [km2]"), &m_WHWoodpeckerSuitabilityArray, _T("Area [km2] for White-Headed Woodpecker"));

         bool ok = LoadILAPXML(initStr);

         if (!ok)
            return false;

         m_ILAPmodelCount = m_speciesCount;
         }
         break;

         // Downy Brome
      case 5:
         {
         m_downyBromeID = 5;
         // get column numbers for Downy Brome model
         CheckCol(pEnvContext->pMapLayer, m_colDistanceRoads, _T("D_ROADS"), TYPE_DOUBLE, CC_MUST_EXIST);
         CheckCol(pEnvContext->pMapLayer, m_colTotalTreesPerHa, _T("TreesPha"), TYPE_DOUBLE, CC_MUST_EXIST);
         CheckCol(pEnvContext->pMapLayer, m_colTotalJuniperTreesPerHa, _T("JunipPha"), TYPE_DOUBLE, CC_MUST_EXIST);
         // Minimum Temperature in May in degree Celsius
         CheckCol(pEnvContext->pMapLayer, m_colMinTempInMay, _T("cli_tmin05"), TYPE_DOUBLE, CC_MUST_EXIST);
         // Precipitation in March in centimeter
         CheckCol(pEnvContext->pMapLayer, m_colPrecipInMarch, _T("cli_ppt03_"), TYPE_DOUBLE, CC_MUST_EXIST);

         CheckCol(pEnvContext->pMapLayer, m_colDownyBrome, _T("HS_DownyB"), TYPE_DOUBLE, CC_AUTOADD);

         m_downyBromeSuitabilityArray.SetName("Total area for each Downy Brome Suitability Category [km2]");
         m_downyBromeSuitabilityArray.SetSize(5, 0);
         m_downyBromeSuitabilityArray.SetLabel(0, "Time (year)");
         m_downyBromeSuitabilityArray.SetLabel(1, "No Downy Brome Suitability Area [km2]");
         m_downyBromeSuitabilityArray.SetLabel(2, "Low Downy Brome Suitability Area [km2]");
         m_downyBromeSuitabilityArray.SetLabel(3, "Moderate Downy Brome Suitability Area [km2]");
         m_downyBromeSuitabilityArray.SetLabel(4, "High Downy Brome Suitability Area [km2]");
         m_downyBromeIndex = AddOutputVar(_T("Downy Brome Suitability [km2]"), &m_downyBromeSuitabilityArray, _T("Area [km2] for Downy Brome"));
         m_downyBromeCount = 1;
         }
         break;

         // VFO (Habitat Classes) Northern Spotted Owl
      case 6:
         {
         m_HCSpottedOwlID = 6;
         //get column numbers for VFO Northern Spotted Owl model
         CheckCol(pEnvContext->pMapLayer, m_colNWForPlan, _T("NWForPlan"), TYPE_INT, CC_MUST_EXIST);
         CheckCol(pEnvContext->pMapLayer, m_colHCSpottOwl, _T("HCSpottOwl"), TYPE_INT, CC_AUTOADD);

         m_VFOSpottedOwlSuitabilityArray.SetName("Total area for each VFO Northern Spotted Owl Habitat Classes [km2]");
         m_VFOSpottedOwlSuitabilityArray.SetSize(4, 0);
         m_VFOSpottedOwlSuitabilityArray.SetLabel(0, "Time (year)");
         m_VFOSpottedOwlSuitabilityArray.SetLabel(1, "Non-Habitat Class Area [km2]");
         m_VFOSpottedOwlSuitabilityArray.SetLabel(2, "Moderate Habitat Class Area [km2]");
         m_VFOSpottedOwlSuitabilityArray.SetLabel(3, "Good Habitat Class Area [km2]");
         m_HCSpottedOwlIndex = AddOutputVar(_T("VFO Northern Spotted Owl Suitability [km2]"), &m_VFOSpottedOwlSuitabilityArray, _T("Area [km2] for VFO Morthern Spotted Owl"));
         m_HCSpottedOwlCount = 1;
         }
         break;
      }

   m_useAddDelta = false;
   _Run(pEnvContext);

   TRACE(_T("Completed TerrBiodiv.cpp Init() successfully.\n"));
   return TRUE;
   }

bool TerrBiodiv::InitRun(EnvContext *pEnvContext, bool useInitSeed)
   {
   m_muleDeerSuitabilityArray.ClearRows();
   m_HSISpottedOwlSuitabilityArray.ClearRows();
   m_downyBromeSuitabilityArray.ClearRows();
   m_VFOSpottedOwlSuitabilityArray.ClearRows();
   m_AmericanMartenSuitabilityArray.ClearRows();
   m_BBWoodpeckerSuitabilityArray.ClearRows();
   m_NorthernGoshawkSuitabilityArray.ClearRows();
   m_PileatedWoodpeckerSuitabilityArray.ClearRows();
   m_RedNSapsuckerSuitabilityArray.ClearRows();
   m_SpottedOwlSuitabilityArray.ClearRows();
   m_WesternBluebirdSuitabilityArray.ClearRows();
   m_WHWoodpeckerSuitabilityArray.ClearRows();

   return true;
   }

bool TerrBiodiv::Run(EnvContext *pEnvContext)
   {
   m_useAddDelta = true;
   _Run(pEnvContext);

   return true;
   }

bool TerrBiodiv::_Run(EnvContext *pEnvContext)
   {
   TRACE(_T("Starting TerrBiodiv.cpp Run with bool argument.\n"));
   ASSERT(pEnvContext != NULL);

   switch (pEnvContext->id)
      {
      // HSI Mule Deer
      case 2:
         {
         SetGridCellSize(90);
         // number of subgridcells per grid cell
         SetNumberSubGridCells(9);
         RunMuleDeerHSI(pEnvContext);
         }
         break;

         // HSI Northern Spotted Owl
      case 3:
         {
         // set size of grid cells to 90 meters
         SetGridCellSize(90);
         // number of subgridcells per grid cell
         SetNumberSubGridCells(9);
         m_radiusOfHabitat = 1000.0;
         RunHSINorthernSpottedOwl(pEnvContext);
         }
         break;

         // ILAP Habitat Models
      case 4:
         {
         RunILAPHabitatModels(pEnvContext);
         }
         break;

         //Downy Brome
      case 5:
         {
         RunDownyBrome(pEnvContext);
         }
         break;

         // HC (Habitat Classes) Northern Spotted Owl
      case 6:
         {
         // set threshold value for HC NSO model
         m_threshElevation = 1800.0;
         // set threshold value for HC NSO model
         m_radiusOfHabitat = 1000.0;
         RunHCNorthernSpottedOwl(pEnvContext);
         }
         break;
      }

   TRACE(_T("Completed TerrBiodiv.cpp Run with bool successfully.\n"));
   return TRUE;
   }

bool TerrBiodiv::LoadMuleDeerXML(LPCTSTR filename)
   {

   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      CString msg;
      msg.Format("TerrBiodiv::LoadMuleDeerXML Error reading input file %s:  %s", filename, doc.ErrorDesc());
      Report::ErrorMsg(msg);
      return false;
      }

   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // <terraBiodiv>

   //----------------------------- <muledeer_model> ---------------------------------
   TiXmlElement *pXmlMuleDeerModel = pXmlRoot->FirstChildElement("muledeer_model");

   if (pXmlMuleDeerModel != NULL)
      {
      LPTSTR pvtTable = NULL;

      XML_ATTR setAttrsMuleDeer[] = {
         // attr                      type           address                                     isReq  checkCol        
         { "pvt_table", TYPE_STRING, &pvtTable, true, 0 },
         { NULL, TYPE_NULL, NULL, false, 0 } };

      ok = TiXmlGetAttributes(pXmlMuleDeerModel, setAttrsMuleDeer, filename, NULL);

      if (!ok)
         {
         CString msg;
         msg.Format(_T("TerrBiodiv::LoadMuleDeerXML Misformed root element reading <muledeer_model> attributes in input file %s"), filename);
         Report::ErrorMsg(msg);
         return false;
         }

      if (LoadPVTTable(pvtTable, m_muleDeerPVTLUT, m_muleDeerPVTMap) == false)
         return false;
      }
   return true;
   }

bool TerrBiodiv::LoadILAPXML(LPCTSTR filename)
   {
   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // <terraBiodiv>

   //----------------------------- <ilap_model> ---------------------------------

   TiXmlElement *pXmlModel = pXmlRoot->FirstChildElement("ilap_model");

   if (pXmlModel == NULL)
      {
      CString msg("LAP Model: missing <ilap_model> element in input file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   if (pXmlModel != NULL)
      {
      LPTSTR sizeTable = NULL;
      LPTSTR canopyCoverTable = NULL;
      LPTSTR canopyLayerTable = NULL;
      LPTSTR coverTypeTable = NULL;
      LPTSTR pvtTable = NULL;

      XML_ATTR attrs[] = {
         // attr                      type           address						isReq  checkCol        
         { "size_table",				TYPE_STRING,	&sizeTable,						true,		0 },
         { "canopyCover_table",		TYPE_STRING,	&canopyCoverTable,			true,		0 },
         { "canopyLayer_table",		TYPE_STRING,	&canopyLayerTable,			true,		0 },
         { "coverType_table",			TYPE_STRING,	&coverTypeTable,				true,		0 },
         { "pvt_table",					TYPE_STRING,	&pvtTable,						true,		0 },
         { NULL,							TYPE_NULL,		NULL,								false,	0 } };

      ok = TiXmlGetAttributes(pXmlModel, attrs, filename, NULL);

      if (!ok)
         {
         CString msg;
         msg.Format(_T("TerrBiodiv: Misformed root element reading <ilap_model> attributes in input file %s"), filename);
         Report::ErrorMsg(msg);
         return false;
         }

      if (LoadILAPSizeTable(sizeTable) == false)
         return false;

      if (LoadILAPCanopyCoverTable(canopyCoverTable) == false)
         return false;

      if (LoadILAPCanopyLayerTable(canopyLayerTable) == false)
         return false;

      if (LoadILAPCoverTypeTable(coverTypeTable) == false)
         return false;

      if (LoadPVTTable(pvtTable, m_ILAPPVTLUT, m_ILAPPVTMap) == false)
         return false;

      }
   return true;
   }

bool TerrBiodiv::LoadILAPSizeTable(LPTSTR sizeScoreTable)
   {
   int rows = m_ILAPSizeLUT.ReadAscii(sizeScoreTable);

   if (rows <= 0)
      {
            {
            CString msg;
            msg.Format("TerrBiodiv::LoadILAPSizeTable could not load Size .csv file \n");
            Report::InfoMsg(msg);
            return false;
            }
      }

   if (rows > 0)
      {
      // get ILAP Size table column numbers for column headers
      int colSpecie = m_ILAPSizeLUT.GetCol("ILAP Species Code");
      int colMeadow = m_ILAPSizeLUT.GetCol("Meadow");
      int colShrub = m_ILAPSizeLUT.GetCol("Shrub");
      int colSeedlingSapling = m_ILAPSizeLUT.GetCol("Seedling/Sapling");
      int colPole = m_ILAPSizeLUT.GetCol("Pole");
      int colSmallTree = m_ILAPSizeLUT.GetCol("Small Tree");
      int colMediumTree = m_ILAPSizeLUT.GetCol("Medium tree");
      int colLargeTree = m_ILAPSizeLUT.GetCol("Large tree");
      int colGiantTree = m_ILAPSizeLUT.GetCol("Giant tree");

      if (colSpecie < 0 || colMeadow < 0 || colShrub < 0 || colSeedlingSapling < 0 || colPole < 0 || colSmallTree < 0
         || colMediumTree < 0 || colLargeTree < 0 || colGiantTree < 0)
         {
         CString msg;
         msg.Format("TerrBiodiv::LoadILAPSizeTable: One or more column headings are incorrect in ILAP Size .csv file\n");
         Report::ErrorMsg(msg);
         return false;
         }
      m_speciesCount = rows;
      }
   return true;
   }

bool TerrBiodiv::LoadILAPCanopyCoverTable(LPTSTR canopyCoverScoreTable)
   {
   int rows = m_ILAPCanopyCoverLUT.ReadAscii(canopyCoverScoreTable);

   if (rows <= 0)
      {
            {
            CString msg;
            msg.Format("TerrBiodiv::LoadILAPCanopyCoverTable could not load Size .csv file \n");
            Report::InfoMsg(msg);
            return false;
            }
      }

   // get ILAP Canopy Cover table column numbers for column headers
   int colSpecie = m_ILAPCanopyCoverLUT.GetCol("ILAP Species Code");
   int colCCLow = m_ILAPCanopyCoverLUT.GetCol("Low");
   int colCCMed = m_ILAPCanopyCoverLUT.GetCol("Medium");
   int colCCHigh = m_ILAPCanopyCoverLUT.GetCol("High");
   int colCCPosDisturb = m_ILAPCanopyCoverLUT.GetCol("PostDisturb");

   if (colSpecie < 0 || colCCLow < 0 || colCCMed < 0 || colCCHigh < 0 || colCCPosDisturb < 0)
      {
      CString msg;
      msg.Format("TerrBiodiv::LoadILAPCanopyCoverTable: One or more column headings are incorrect in ILAP Canopy Cover .csv file\n");
      Report::ErrorMsg(msg);
      return false;
      }
   return true;
   }

bool TerrBiodiv::LoadILAPCanopyLayerTable(LPTSTR canopyLayerScoreTable)
   {
   int rows = m_ILAPCanopyLayerLUT.ReadAscii(canopyLayerScoreTable);

   if (rows <= 0)
      {
            {
            CString msg;
            msg.Format("TerrBiodiv::LoadILAPCanopyLayerTable could not load Size .csv file \n");
            Report::InfoMsg(msg);
            return false;
            }
      }

   // get ILAP Canopy Layer table column numbers for column headers
   int colSpecie = m_ILAPCanopyLayerLUT.GetCol("ILAP Species Code");
   int colCLSingle = m_ILAPCanopyLayerLUT.GetCol("Single");
   int colCLMulti = m_ILAPCanopyLayerLUT.GetCol("Multi");

   if (colSpecie < 0 || colCLSingle < 0 || colCLMulti < 0)
      {
      CString msg;
      msg.Format("TerrBiodiv::LoadILAPCanopyLayerTable: One or more column headings are incorrect in ILAP Canopy Layer .csv file\n");
      Report::ErrorMsg(msg);
      return false;
      }
   return true;
   }

bool TerrBiodiv::LoadILAPCoverTypeTable(LPTSTR coverTypeScoreTable)
   {
   int rows = m_ILAPCoverTypeLUT.ReadAscii(coverTypeScoreTable);

   if (rows <= 0)
      {
            {
            CString msg;
            msg.Format("TerrBiodiv::LoadILAPCoverTypeTable could not load Size .csv file \n");
            Report::InfoMsg(msg);
            return false;
            }
      }

   CreateILAPCoverTypeMap();

   return true;
   }

bool TerrBiodiv::CreateILAPCoverTypeMap()
   {
   m_coverTypeColIndexMap.RemoveAll();

   // NOTE:  CSV file must match this order EXACTLY
   // and this order also matches the enumerated type in TerrBiodiv.h

   int index = 2;
   m_coverTypeColIndexMap.SetAt(CT_PK, index++);
   m_coverTypeColIndexMap.SetAt(CT_AW, index++);
   m_coverTypeColIndexMap.SetAt(CT_OA, index++);
   m_coverTypeColIndexMap.SetAt(CT_OP, index++);
   m_coverTypeColIndexMap.SetAt(CT_DF, index++);
   m_coverTypeColIndexMap.SetAt(CT_DFMX, index++);
   m_coverTypeColIndexMap.SetAt(CT_DFWF, index++);
   m_coverTypeColIndexMap.SetAt(CT_GFES, index++);
   m_coverTypeColIndexMap.SetAt(CT_SFDF, index++);
   m_coverTypeColIndexMap.SetAt(CT_WLLP, index++);
   m_coverTypeColIndexMap.SetAt(CT_TSHE, index++);
   m_coverTypeColIndexMap.SetAt(CT_GF, index++);
   m_coverTypeColIndexMap.SetAt(CT_WF, index++);
   m_coverTypeColIndexMap.SetAt(CT_RF, index++);
   m_coverTypeColIndexMap.SetAt(CT_RFWF, index++);
   m_coverTypeColIndexMap.SetAt(CT_ESAF, index++);
   m_coverTypeColIndexMap.SetAt(CT_MH, index++);
   m_coverTypeColIndexMap.SetAt(CT_PICO, index++);
   m_coverTypeColIndexMap.SetAt(CT_LPWI, index++);
   m_coverTypeColIndexMap.SetAt(CT_MXPI, index++);
   m_coverTypeColIndexMap.SetAt(CT_PIPO, index++);
   m_coverTypeColIndexMap.SetAt(CT_PPLP, index++);
   m_coverTypeColIndexMap.SetAt(CT_WP, index++);
   m_coverTypeColIndexMap.SetAt(CT_PIJE, index++);
   m_coverTypeColIndexMap.SetAt(CT_JU, index++);

   return true;
   }

bool TerrBiodiv::LoadPVTTable(LPTSTR pvtScoreTable, VDataObj &pvtLUT, map<PVTKey, int, PVTMapCompare> &pvtMap)
   {
   int rows = pvtLUT.ReadAscii(pvtScoreTable);

   if (rows <= 0)
      {
            {
            CString msg;
            msg.Format("TerrBiodiv::LoadPVTTable could not load PVT .csv file \n");
            Report::InfoMsg(msg);
            return false;
            }
      }

   // get ILAP Canopy Cover table column numbers for column headers
   int colSpecie = pvtLUT.GetCol("Species Code");
   int colRegion = pvtLUT.GetCol("Region");
   int colPVT = pvtLUT.GetCol("PVT");
   int colScore = pvtLUT.GetCol("Score");

   if (colSpecie < 0 || colRegion < 0 || colPVT < 0)
      {
      CString msg;
      msg.Format("TerrBiodiv::LoadPVTTable: One or more column headings are incorrect in PVT .csv file\n");
      Report::ErrorMsg(msg);
      return false;
      }

   int recordCount = pvtLUT.GetRowCount();

   for (int row = 0; row < recordCount; row++)
      {
      int pvtScore = -1;
      // Building hash key for pvt 
      pvtLUT.Get(colSpecie, row, m_pvtHashKey.m_speciescode);
      pvtLUT.Get(colRegion, row, m_pvtHashKey.m_region);
      pvtLUT.Get(colPVT, row, m_pvtHashKey.m_pvt);

      pvtLUT.Get(colScore, row, pvtScore);

      // storing key/value pair in map
      pvtMap.insert(std::pair<PVTKey, int>(m_pvtHashKey, pvtScore));
      }

   return true;
   }

bool TerrBiodiv::LoadPolyGridLookup(EnvContext *pEnvContext)
   {
   if (m_pPolyGridLkUp != NULL)
      return true;
   TRACE(_T("Starting TerrBiodiv.cpp LoadPolyGridLookup.\n"));
   ASSERT(pEnvContext != NULL);

   REAL
      xMin = -1.0f,
      xMax = -1.0f,
      yMin = -1.0f,
      yMax = -1.0f;

   long
      n = -1,
      rows = -1,
      cols = -1,
      maxEls = -1,
      gridRslt = -1;

   // use extent of polygon layer to define extent of grid layer
   m_pPolyMapLayer->GetExtents(xMin, xMax, yMin, yMax);
   rows = (int)ceil((yMax - yMin) / m_cellDim);
   cols = (int)ceil((xMax - xMin) / m_cellDim);

   maxEls = (rows * cols);
   n = m_pPolyMapLayer->GetPolygonCount();

   CString msg;
   msg.Format("TerraBiodiv: Generating Polygrid - %d Rows, %d Cols, CellDim: %d  NumElements: %d", rows, cols, m_cellDim, maxEls);
   Report::Log(msg);

   // create grids, based on the extent of polygon layer, for important intermediate and output values
   gridRslt = m_pGridMapLayer->CreateGrid(rows, cols, xMin, yMin, (float)m_cellDim, -9999, DOT_INT, FALSE, FALSE);

   // if pgl file exists, use it, otherwise create and persist it
   CPath pglPath(PathManager::GetPath(PM_IDU_DIR));

   CString filename;
   int gridSize = GetGridCellSize();
   filename.Format("TerraBiodiv%im.pgl", gridSize);
   pglPath.Append(filename);

   // if .pgl file exists: read poly - grid - lookup table from file
   // else (if .pgl file does NOT exists): create poly - grid - lookup table and write to .pgl file
   //struct stat stFileInfo;
   //if( stat( pglFile.GetString(), &stFileInfo) == 0 )
   if (pglPath.Exists())
      {
      msg = "  Loading PolyGrid from File ";
      msg += pglPath;
      Report::Log(msg);

      m_pPolyGridLkUp = new PolyGridLookups(pglPath);

      // if existing file has wrong dimension, create NEW poly - grid - lookup table and write to .pgl file
      if ((m_pPolyGridLkUp->GetNumGridCols() != cols) || (m_pPolyGridLkUp->GetNumGridRows() != rows))
         {
         msg = "Invalid dimensions: Recreating File ";
         msg += pglPath;
         Report::Log(msg);
         m_pPolyGridLkUp = new PolyGridLookups(m_pGridMapLayer, m_pPolyMapLayer, m_numSubGridCells, maxEls, 0, -9999);
         m_pPolyGridLkUp->WriteToFile(pglPath);
         }
      }
   else  // doesn't exist, create it
      {
      msg = "  Generating PolyGrid ";
      msg += pglPath;
      Report::Log(msg);
      m_pPolyGridLkUp = new PolyGridLookups(m_pGridMapLayer, m_pPolyMapLayer, m_numSubGridCells, maxEls, 0, -9999);
      m_pPolyGridLkUp->WriteToFile(pglPath);
      }

   return true;
   } // bool TerrBiodiv::LoadPolyGridLookup

bool TerrBiodiv::CalculateHSINorthenSpottedOwlSuitability(EnvContext * pEnvContext)
   {
   TRACE(_T("Starting TerrBiodiv.cpp CalculateHSINorthernSpottedOwlSuitability().\n"));
   ASSERT(pEnvContext != NULL);
   ASSERT(m_pPolyGridLkUp != NULL);

   // get dimension of grid
   int xDim = m_pPolyGridLkUp->GetNumGridRows();
   int yDim = m_pPolyGridLkUp->GetNumGridCols();

   int numberOfLandscapeCells = (int)floor(m_radiusOfHabitat / GetGridCellSize());

   float landscapeScore = 0.0f;
   float nestSiteScore = 0.0f;
   float habitatSuitabilityIndex = 0.0f;
   float total_area = 0.0f;
   float landscape_area = 0.0f;

   // set suitability values
   m_nonHabitatHSIOwlSuitabilityArea = 0.0f;
   m_moderateHabitatHSIOwlSuitabilityArea = 0.0f;
   m_goodHabitatHSIOwlSuitabilityArea = 0.0f;

   int row = 0;
   int col = 0;

   for (row = 0; row < xDim; row++)
      {
      for (col = 0; col < yDim; col++)
         {
         int size = m_pPolyGridLkUp->GetPolyCntForGridPt(row, col);
         int *polyIndxs = new int[size];
         int polyCount = m_pPolyGridLkUp->GetPolyNdxsForGridPt(row, col, polyIndxs);
         float *polyFractionalArea = new float[polyCount];

         m_pPolyGridLkUp->GetPolyProportionsForGridPt(row, col, polyFractionalArea);

         int pvt = -1;
         int stemSize = -1;
         int canopyCover = -1;
         int canopyLayers = -1;
         int vNWForPlan = -1;
         int region = -1;

         float idu_area = 0.0f;
         landscapeScore = 0.0f;
         nestSiteScore = 0.0f;
         habitatSuitabilityIndex = 0.0f;

         for (int idu = 0; idu < polyCount; idu++)
            {
            // habitat score is dependent upon both nestSite index and landscapeScore
            // NSO habitat can only occur in the Northwest Forest Plan  
            m_pPolyMapLayer->GetData(polyIndxs[idu], m_colArea, idu_area);
            total_area += idu_area;

            m_pPolyMapLayer->GetData(polyIndxs[idu], m_colRegion, region);

            if (region == R_OREGON_EAST_CASCADES) // || R_SOUTHWEST_OREGON )
               {
               m_pPolyMapLayer->GetData(polyIndxs[idu], m_colNWForPlan, vNWForPlan);

               if (vNWForPlan == 1 || vNWForPlan == 2 || vNWForPlan == 4 || vNWForPlan == 5 || vNWForPlan == 6)
                  {
                  m_pPolyMapLayer->GetData(polyIndxs[idu], m_colPVT, pvt);
                  m_pPolyMapLayer->GetData(polyIndxs[idu], m_colStemSize, stemSize);
                  m_pPolyMapLayer->GetData(polyIndxs[idu], m_colCanopyCover, canopyCover);
                  m_pPolyMapLayer->GetData(polyIndxs[idu], m_colCanopyLayers, canopyLayers);

                  if (canopyLayers == L_MULTI)
                     {
                     if (canopyCover == C_MEDIUM)
                        {
                        if (stemSize == S_MEDIUM_TREE || stemSize == S_LARGE_TREE || stemSize == S_GIANT_TREE)
                           nestSiteScore = 0.5f;
                        }
                     else if (canopyCover == C_HIGH)
                        {
                        if (stemSize == S_SMALL_TREE)
                           nestSiteScore = 0.5f;
                        else if (stemSize == S_MEDIUM_TREE || stemSize == S_LARGE_TREE || stemSize == S_GIANT_TREE)
                           {
                           if (pvt == PVT_REG9_WESTHEMWET || pvt == PVT_REG9_WESTHEMINTERMED || pvt == PVT_REG9_WESTHEMCOLD || pvt == PVT_REG9_MIXCONFMOIST || pvt == PVT_REG9_MIXCONFCOLDDRY || pvt == PVT_REG9_MTNHEMLOCK)
                              nestSiteScore = 1.0f;
                           }
                        }
                     }
                  } // end NWForPlan
               } // end region 


               // Calculate landscape score portion of the Spotted Owl Suitability
            landscape_area = 0.0f;
            pvt = -1;
            canopyCover = -1;
            int cellCount = 0;
            float pvtCount = 0.0f;
            float canopyCoverCount = 0.0f;

            float pvtAvg = 0.0f;
            float canopyCoverAvg = 0.0f;

            int rowWithin1km = row - numberOfLandscapeCells;
            int colWithin1km = col - numberOfLandscapeCells;

            int rowMax = row + numberOfLandscapeCells;
            int colMax = col + numberOfLandscapeCells;

            if (rowMax <= xDim && rowWithin1km >= 0 && colMax <= yDim && colWithin1km >= 0)
               {
               if (nestSiteScore > 0.0f)
                  {
                  //	#pragma omp parallel for 
                  for (rowWithin1km; rowWithin1km < rowMax; rowWithin1km++)
                     {
                     for (colWithin1km; colWithin1km < colMax; colWithin1km++)
                        {
                        // is this cell within 1km radius
                        // explicit multiply is faster than calling pow function
                        int radiusSqr = (rowWithin1km - row)*GetGridCellSize()*(rowWithin1km - row)*GetGridCellSize() + (colWithin1km - col)*GetGridCellSize()*(colWithin1km - col)*GetGridCellSize();
                        //  include only cells within 1km radius
                        if (radiusSqr < (int)m_radiusOfHabitat *(int)m_radiusOfHabitat)
                           {
                           // do not include the nesting cell
                           if (rowWithin1km != row || colWithin1km != col)
                              {
                              cellCount++;

                              int countWithinCell = m_pPolyGridLkUp->GetPolyCntForGridPt(rowWithin1km, colWithin1km);
                              int *idusWithinCellArray = new int[countWithinCell];
                              countWithinCell = m_pPolyGridLkUp->GetPolyNdxsForGridPt(rowWithin1km, colWithin1km, idusWithinCellArray);
                              float *iduFractionWithinCellArray = new float[countWithinCell];
                              m_pPolyGridLkUp->GetPolyProportionsForGridPt(rowWithin1km, colWithin1km, iduFractionWithinCellArray);

                              for (int iduWithinCell = 0; iduWithinCell < countWithinCell; iduWithinCell++)
                                 {
                                 // habitat score is dependent upon both nestSite index and landscapeScore
                                 // NSO habitat can only occur in the Northwest Forest Plan  							
                                 int iduIndex = idusWithinCellArray[iduWithinCell];
                                 m_pPolyMapLayer->GetData(iduIndex, m_colRegion, region);

                                 if (region == R_OREGON_EAST_CASCADES)
                                    {
                                    m_pPolyMapLayer->GetData(iduIndex, m_colNWForPlan, vNWForPlan);

                                    if (vNWForPlan == 1 || vNWForPlan == 2 || vNWForPlan == 4 || vNWForPlan == 5 || vNWForPlan == 6)
                                       {
                                       m_pPolyMapLayer->GetData(iduIndex, m_colPVT, pvt);
                                       m_pPolyMapLayer->GetData(iduIndex, m_colCanopyCover, canopyCover);
                                       if (pvt == PVT_REG9_WESTHEMWET || pvt == PVT_REG9_WESTHEMINTERMED || pvt == PVT_REG9_WESTHEMCOLD || pvt == PVT_REG9_MIXCONFMOIST || pvt == PVT_REG9_MIXCONFCOLDDRY || pvt == PVT_REG9_MTNHEMLOCK)
                                          {
                                          landscape_area += iduFractionWithinCellArray[iduWithinCell];
                                          canopyCoverCount += iduFractionWithinCellArray[iduWithinCell] * canopyCover;
                                          }
                                       } // end NW Forest Plan
                                    } // end region 
                                 }  // end idus within 1km surrounding the nest cell
                              } // end excluding the nest cell
                           } // end cells within 1km radius
                        } // end columns within 1km surrounding the nest cell
                     } // end rows within 1km surrounding the nest cell
                  if (cellCount != 0)
                     {
                     pvtAvg = landscape_area / cellCount;
                     canopyCoverAvg = canopyCoverCount / cellCount;
                     }
                  if ((canopyCoverAvg >= C_HIGH) && (pvtAvg >= 0.60))
                     landscapeScore = 1.0f;
                  else if ((canopyCoverAvg >= C_MEDIUM) && (pvtAvg >= 0.30))
                     landscapeScore = 0.5f;
                  } // if cell of interest has 1km radius 
               } // end nest cell score greater than 0
            UpdateIDU(pEnvContext, polyIndxs[idu], m_colHSINestSiteScoreOwl, nestSiteScore, m_useAddDelta ? ADD_DELTA : SET_DATA);
            UpdateIDU(pEnvContext, polyIndxs[idu], m_colHSILandscapeSiteScoreOwl, landscapeScore, m_useAddDelta ? ADD_DELTA : SET_DATA);
            float habitatScore = (float)sqrt(pow(nestSiteScore, 2.0) * landscapeScore);
            UpdateIDU(pEnvContext, polyIndxs[idu], m_colHSIScoreOwl, habitatScore, m_useAddDelta ? ADD_DELTA : SET_DATA);

            if (habitatScore >= 0.0f && habitatScore <= 0.30)
               m_nonHabitatHSIOwlSuitabilityArea += idu_area;
            else if (habitatScore > 0.30 && habitatScore < 1)
               m_moderateHabitatHSIOwlSuitabilityArea += idu_area;
            else if (habitatScore >= 1)
               m_goodHabitatHSIOwlSuitabilityArea += idu_area;

            } // end nest cell IDUs
         delete[] polyIndxs;
         } //end columns of grid
      } // end rows of grid

      // put Suitability Categories into Post-Run Graph
   float HSISpottedOwlHabitatLevels[4];
   float time = (float)pEnvContext->currentYear;

   HSISpottedOwlHabitatLevels[0] = time;
   HSISpottedOwlHabitatLevels[1] = m_nonHabitatHSIOwlSuitabilityArea / 1E6F;
   HSISpottedOwlHabitatLevels[2] = m_moderateHabitatHSIOwlSuitabilityArea / 1E6F;
   HSISpottedOwlHabitatLevels[3] = m_goodHabitatHSIOwlSuitabilityArea / 1E6F;

   m_HSISpottedOwlSuitabilityArray.AppendRow(HSISpottedOwlHabitatLevels, 4);

   //set new scores
   m_lowerBound = 0.0f;
   m_upperBound = 1.0f;
   m_rawScore = 0.0f;

   // reset suitability values
   m_nonHabitatHSIOwlSuitabilityArea = 0.0f;
   m_moderateHabitatHSIOwlSuitabilityArea = 0.0f;
   m_goodHabitatHSIOwlSuitabilityArea = 0.0f;

   return TRUE;
   }

int TerrBiodiv::CalcMuleDeerForagingHabitatScore(int coverType, int stemSize, int canopyCover, int canopyLayers, float purshiaCover)
   {
   int
      totalForagingScore = 0,
      coverTypeScore = 0,
      stemSizeScore = 0,
      canopyCoverScore = 0,
      canopyLayersScore = 0,
      purshiaCoverScore = 0;

   //cover type
   if (coverType > 99 && coverType < 150 && coverType > 200)
      coverTypeScore = 1;

   //size class
   switch (stemSize)
      {
      case S_DEVELOPMENT:
         stemSizeScore = 3;
         break;
      case S_MEADOW:
         stemSizeScore = 4;
         break;
      case S_SHRUB:
      case S_SEEDLING_SAPLING:
         stemSizeScore = 4;
         break;
      case S_POLE:
         stemSizeScore = 2;
         break;
      case S_SMALL_TREE:
      case S_MEDIUM_TREE:
      case S_LARGE_TREE:
      case S_GIANT_TREE:
         stemSizeScore = 1;
         break;
      default:
         stemSizeScore = 0;
         break;
      }

   //canopy cover
   switch (canopyCover)
      {
      case C_NONE:
         canopyCoverScore = 10;
         break;
      case C_LOW:
         canopyCoverScore = 6;
         break;
      case C_MEDIUM:
         canopyCoverScore = 2;
         break;
      case C_HIGH:
         canopyCoverScore = 1;
         break;
      default:
         canopyCoverScore = 0;
         break;
      }

   //canopy layers
   switch (canopyLayers)
      {
      case L_NONE:
      case L_SINGLE:
         canopyLayersScore = 1;
         break;
      default:
         canopyLayersScore = 0;
         break;
      }

   //PUTR cover
   if ((purshiaCover >= 0.0f) && (purshiaCover <= 10.0f))
      purshiaCoverScore = 2;
   else if ((purshiaCover > 10.0f) && (purshiaCover <= 30.0f))
      purshiaCoverScore = 4;
   else
      purshiaCoverScore = 0;

   //calculate Foraging Habitat Score by summing up all scores
   totalForagingScore = coverTypeScore + stemSizeScore + canopyCoverScore + canopyLayersScore + purshiaCoverScore;

   /* if (totalForagingScore >= 5 && totalForagingScore <= 20)
       return totalForagingScore;
    else
       return 0;*/
   return 2 * totalForagingScore;
   }

int TerrBiodiv::CalcMuleDeerHidingHabitatScore(int coverType, int stemSize, int canopyCover, int canopyLayers)
   {
   int
      totalHidingScore = 0,
      coverTypeScore = 0,
      stemSizeScore = 0,
      canopyCoverScore = 0,
      canopyLayersScore = 0;

   //cover type
   switch (coverType)
      {
      case CT_PIPO:				// ponderosa pine
      case CT_PICO:				// lodgepole pine
      case CT_PIJE:				// jeffery pine
      case CT_DF:					// doulgas fir
         coverTypeScore = 1;
         break;
      case CT_JU:				// western juniper
      case CT_OA:				// oregon white oak
         coverTypeScore = 2;
         break;
      default:
         coverTypeScore = 0;
      }

   //size class
   switch (stemSize)
      {
      case S_DEVELOPMENT:
         stemSizeScore = 1;
         break;
      case S_MEADOW:
      case S_SHRUB:
      case S_SEEDLING_SAPLING:
      case S_POLE:
      case S_SMALL_TREE:
      case S_MEDIUM_TREE:
         stemSizeScore = 3;
         break;
      case S_LARGE_TREE:
      case S_GIANT_TREE:
         stemSizeScore = 2;
         break;
      default:
         stemSizeScore = 0;
         break;
      }

   //canopy cover
   switch (canopyCover)
      {
      case C_LOW:
         canopyCoverScore = 2;
         break;
      case C_MEDIUM:
      case C_HIGH:
         canopyCoverScore = 4;
         break;
      default:
         canopyCoverScore = 0;
         break;
      }

   //canopy layers
   switch (canopyLayers)
      {
      case L_SINGLE:
      case L_MULTI:
         canopyLayersScore = 1;
         break;
      default:
         canopyLayersScore = 0;
         break;
      }

   //calculate total Hiding Habitat Score by summing up all scores
   totalHidingScore = coverTypeScore + stemSizeScore + canopyCoverScore + canopyLayersScore;

   /*if (totalHidingScore >= 2 && totalHidingScore <= 10)
      return totalHidingScore;
   else
      return 0;*/
   return totalHidingScore;
   }

int TerrBiodiv::CalcMuleDeerThermalCoverScore(int coverType, int stemSize, int canopyCover, int canopyLayers)
   {
   int
      totalThermalScore = 0,
      coverTypeScore = 0,
      stemSizeScore = 0,
      canopyCoverScore = 0,
      canopyLayersScore = 0;

   //cover type
   switch (coverType)
      {
      case CT_PIPO:				// ponderosa pine
      case CT_PICO:				// lodgepole pine
      case CT_PIJE:				// jeffery pine
      case CT_DF:					// doulgas fir
      case CT_JU:					// western juniper
      case CT_OA:					// oregon white oak
         coverTypeScore = 1;
         break;
      default:
         coverTypeScore = 0;
      }

   //size class
   switch (stemSize)
      {
      case S_DEVELOPMENT:
      case S_POLE:
      case S_SMALL_TREE:
      case S_MEDIUM_TREE:
         stemSizeScore = 1;
         break;
      case S_LARGE_TREE:
      case S_GIANT_TREE:
         stemSizeScore = 2;
         break;
      default:
         stemSizeScore = 0;
         break;
      }

   //canopy cover
   switch (canopyCover)
      {
      case C_LOW:
         canopyCoverScore = 2;
         break;
      case C_MEDIUM:
         canopyCoverScore = 4;
         break;
      case C_HIGH:
         canopyCoverScore = 6;
         break;
      default:
         canopyCoverScore = 0;
         break;
      }

   //canopy layers
   switch (canopyLayers)
      {
      case L_SINGLE:
         canopyLayersScore = 1;
         break;
      case L_MULTI:
         canopyLayersScore = 2;
         break;
      default:
         canopyLayersScore = 0;
         break;
      }


   //calculate total Thermal Cover Score by summing up all scores
   totalThermalScore = coverTypeScore + stemSizeScore + canopyCoverScore + canopyLayersScore;

   /* if (totalThermalScore >= 0 && totalThermalScore <= 11)
       return  totalThermalScore;
    else
       return 0;*/
   return  totalThermalScore;
   }


bool TerrBiodiv::CalculateMuleDeerSuitability(EnvContext * pEnvContext)
   {
   //combine three scores to landscape habitat score for mule deer

   TRACE(_T("Starting TerrBiodiv.cpp CalculateMuleDeerSuitability \n"));
   ASSERT(pEnvContext != NULL);
   ASSERT(m_pPolyGridLkUp != NULL);

   int xDim = -1;
   int yDim = -1;

   // area of Grid Cell
   float gridArea = 0.0F;
   float total_area = 0.0f;

   m_lowSuitabilityMuleDeerArea = 0.0f;
   m_moderateSuitabilityMuleDeerArea = 0.0f;
   m_highSuitabilityMuleDeerArea = 0.0f;

   int foragingHabitatScore = 0;
   int hidingHabitatScore = 0;
   int thermalCoverScore = 0;
   int landscapeHabitatScore = 0;

   gridArea = (float)pow(GetGridCellSize(), 2.0);

   // get dimension of grid
   xDim = m_pPolyGridLkUp->GetNumGridRows();
   yDim = m_pPolyGridLkUp->GetNumGridCols();

   // compute suitability 
   for (MapLayer::Iterator idu = m_pPolyMapLayer->Begin(); idu < m_pPolyMapLayer->End(); idu++)
      {
      int region = -1;
      int pvt = -1;
      int coverType = -1;
      int stemSize = -1;
      int canopyCover = -1;
      int canopyLayers = -1;

      float shrubCover = -1.0;

      foragingHabitatScore = 0;
      hidingHabitatScore = 0;
      thermalCoverScore = 0;
      landscapeHabitatScore = 0;

      float idu_area = 0.0f;
      float areaWeightedIDUTotalScore = 0.0F;
      float areaWeightedGridTotalScore = 0.0F;

      // landscape habitat score for mule deer only defined for the modeling zones
      // Oregon East Cascades (= 9), Blue Mountains (= 7), and Oregon West Cascades (= so far not included in dataset)
      m_pPolyMapLayer->GetData(idu, m_colArea, idu_area);


      m_pPolyMapLayer->GetData(idu, m_colRegion, region);
      m_pPolyMapLayer->GetData(idu, m_colPVT, pvt);

      PVTKey lookupKey(0, region, pvt);
      std::map<PVTKey, int>::const_iterator search = m_muleDeerPVTMap.find(lookupKey);

      m_pPolyMapLayer->GetData(idu, m_colCoverType, coverType);
      m_pPolyMapLayer->GetData(idu, m_colStemSize, stemSize);
      m_pPolyMapLayer->GetData(idu, m_colCanopyCover, canopyCover);
      m_pPolyMapLayer->GetData(idu, m_colCanopyLayers, canopyLayers);
      m_pPolyMapLayer->GetData(idu, m_colPurshia, shrubCover);
      m_pPolyMapLayer->GetData(idu, m_colArea, idu_area);

      int matrixScore = 0;
      // Calculate the three different scores for Mule Deer habitats per IDU
      // filtering out Non-Capable areas such as lakes and at this time agriculture
      if (region == 99 || pvt == 99 || (coverType > 99 && coverType < 150 && coverType > 200) || alglib::fp_less_eq(shrubCover, -999.0f))
         {
         landscapeHabitatScore = -1;
         matrixScore = 0;
         }
      else
         {
         int iduIndex = -1;
         m_pPolyMapLayer->GetData(idu, m_colIDU, iduIndex);
         hidingHabitatScore = CalcMuleDeerHidingHabitatScore(coverType, stemSize, canopyCover, canopyLayers);
         foragingHabitatScore = CalcMuleDeerForagingHabitatScore(coverType, stemSize, canopyCover, canopyLayers, shrubCover);
         thermalCoverScore = CalcMuleDeerThermalCoverScore(coverType, stemSize, canopyCover, canopyLayers);
         landscapeHabitatScore = foragingHabitatScore + hidingHabitatScore + thermalCoverScore;
         }

      if (landscapeHabitatScore >= 39)
         {
         if (search != m_muleDeerPVTMap.end())
            {
            switch (search->second)
               {
               case 3:
                  matrixScore = 9;
                  m_highSuitabilityMuleDeerArea += idu_area;
                  break;
               case 2:
                  matrixScore = 8;
                  m_highSuitabilityMuleDeerArea += idu_area;
                  break;
               case 1:
                  matrixScore = 6;
                  m_moderateSuitabilityMuleDeerArea += idu_area;
                  break;
               default:
                  matrixScore = 0;
                  break;
               }
            }
         }
      else if (landscapeHabitatScore > 33 && landscapeHabitatScore <= 38)
         {
         if (search != m_muleDeerPVTMap.end())
            {
            switch (search->second)
               {
               case 3:
                  matrixScore = 7;
                  m_highSuitabilityMuleDeerArea += idu_area;
                  break;
               case 2:
                  matrixScore = 5;
                  m_moderateSuitabilityMuleDeerArea += idu_area;
                  break;
               case 1:
                  matrixScore = 3;
                  m_lowSuitabilityMuleDeerArea += idu_area;
               default:
                  matrixScore = 0;
                  break;
               }
            }
         }
      else if (landscapeHabitatScore >= 0 && landscapeHabitatScore <= 33)
         {
         if (search != m_muleDeerPVTMap.end())
            {
            switch (search->second)
               {
               case 3:
                  matrixScore = 4;
                  m_moderateSuitabilityMuleDeerArea += idu_area;
                  break;
               case 2:
                  matrixScore = 2;
                  m_lowSuitabilityMuleDeerArea += idu_area;
                  break;
               case 1:
                  matrixScore = 1;
                  m_lowSuitabilityMuleDeerArea += idu_area;
               default:
                  matrixScore = 0;
                  break;
               }
            }
         }

      /*	if (landscapeHabitatScore >= 7 && landscapeHabitatScore <= 18)
         {
            m_lowSuitabilityMuleDeerArea += idu_area;
            if (search != m_muleDeerPVTMap.end())
               matrixScore = 1 * search->second;
         }
         else if (landscapeHabitatScore >= 19 && landscapeHabitatScore <= 30)
         {
            m_moderateSuitabilityMuleDeerArea += idu_area;
            if (search != m_muleDeerPVTMap.end())
               matrixScore = 3 + search->second;
         }
         else  if (landscapeHabitatScore >= 31 && landscapeHabitatScore <= 41)
         {
            m_highSuitabilityMuleDeerArea += idu_area;
            if (search != m_muleDeerPVTMap.end())
               matrixScore = 2 * 3 + search->second;
         }
         else if (landscapeHabitatScore > 41)
         {
            matrixScore = 0;
         }*/

      UpdateIDU(pEnvContext, idu, m_colMuleDeerForaging, foragingHabitatScore, m_useAddDelta ? ADD_DELTA : SET_DATA);
      UpdateIDU(pEnvContext, idu, m_colMuleDeerHiding, hidingHabitatScore, m_useAddDelta ? ADD_DELTA : SET_DATA);
      UpdateIDU(pEnvContext, idu, m_colMuleDeerThermal, thermalCoverScore, m_useAddDelta ? ADD_DELTA : SET_DATA);
      //	UpdateIDU(pEnvContext, idu, m_colMuleDeerLandscape, landscapeHabitatScore, m_useAddDelta ? ADD_DELTA : SET_DATA);
      UpdateIDU(pEnvContext, idu, m_colMuleDeerLandscape, matrixScore, m_useAddDelta ? ADD_DELTA : SET_DATA);

      } // end IDUs			

   // put Suitability Categories into Post-Run Graphs
   float muleDeerSuitabilityLevels[4];
   float time = (float)pEnvContext->currentYear;

   muleDeerSuitabilityLevels[0] = time;
   muleDeerSuitabilityLevels[1] = m_lowSuitabilityMuleDeerArea / 1E6F;
   muleDeerSuitabilityLevels[2] = m_moderateSuitabilityMuleDeerArea / 1E6F;
   muleDeerSuitabilityLevels[3] = m_highSuitabilityMuleDeerArea / 1E6F;

   m_muleDeerSuitabilityArray.AppendRow(muleDeerSuitabilityLevels, 4);

   //set new scores
   m_lowerBound = 0.0f;
   m_upperBound = 1.0f;
   m_rawScore = 0.0f;

   // reset suitability values
   m_lowSuitabilityMuleDeerArea = 0.0f;
   m_moderateSuitabilityMuleDeerArea = 0.0f;
   m_highSuitabilityMuleDeerArea = 0.0f;

   return TRUE;
   }

bool TerrBiodiv::CalculateVFONorthernSpottedOwl(EnvContext *pEnvContext)
   {

   TRACE(_T("Starting TerrBiodiv.cpp CalculateFVONorthernSpottedOwl.\n"));
   ASSERT(pEnvContext != NULL);

   int coverType = -1;
   int canopyCover = -1;
   int stemSize = -1;
   int vNWForPlan = -1;
   int NSOHabitatScore = 0;

   float idu_area = 0.0f;
   float total_area = 0.0f;

   m_nonHabitatVFOOwlArea = 0.0f;
   m_moderateHabitatVFOOwlArea = 0.0f;
   m_goodHabitatVFOOwlArea = 0.0f;

   // compute suitability for cover types other than Mountain Hemlock (MH)
   for (MapLayer::Iterator idu = m_pPolyMapLayer->Begin(); idu < m_pPolyMapLayer->End(); idu++)
      {
      m_pPolyMapLayer->GetData(idu, m_colArea, idu_area);
      total_area += idu_area;
      NSOHabitatScore = 0;
      //get value from NorthWest Forest Plan from IDU layer		
      m_pPolyMapLayer->GetData(idu, m_colNWForPlan, vNWForPlan);

      if (vNWForPlan == 1 || vNWForPlan == 2 || vNWForPlan == 4 || vNWForPlan == 5 || vNWForPlan == 6)
         {
         // get various values form IDU layer
         m_pPolyMapLayer->GetData(idu, m_colCoverType, coverType);
         m_pPolyMapLayer->GetData(idu, m_colCanopyCover, canopyCover);
         m_pPolyMapLayer->GetData(idu, m_colStemSize, stemSize);
         m_pPolyMapLayer->GetData(idu, m_colArea, idu_area);

         //set scores and accumulate area  
         switch (stemSize)
            {
            case S_SMALL_TREE:
               {
               if ((coverType == CT_DF) || (coverType == CT_DFWF) ||
                  (coverType == CT_WF) || (coverType == CT_GF) ||
                  (coverType == CT_RFWF))
                  {
                  if (canopyCover == C_MEDIUM || canopyCover == C_HIGH)
                     {
                     NSOHabitatScore = 1;
                     m_moderateHabitatVFOOwlArea += idu_area;
                     }
                  }
               } // end SMALL_TREE
               break;

            case S_MEDIUM_TREE:
            case S_LARGE_TREE:
            case S_GIANT_TREE:
               {
               if ((coverType == CT_DF) || (coverType == CT_DFWF) ||
                  (coverType == CT_WF) || (coverType == CT_GF) ||
                  (coverType == CT_RFWF))
                  {
                  if (canopyCover == C_MEDIUM)
                     {
                     NSOHabitatScore = 1;
                     m_moderateHabitatVFOOwlArea += idu_area;
                     }
                  else if (canopyCover == C_HIGH)
                     {
                     NSOHabitatScore = 2;
                     m_goodHabitatVFOOwlArea += idu_area;
                     }
                  }
               if ((coverType == CT_MXPI))
                  {
                  if (canopyCover == C_HIGH)
                     {
                     NSOHabitatScore = 1;
                     m_moderateHabitatVFOOwlArea += idu_area;
                     }
                  }
               } // end MEDIUM_TREE, LARGE_TREE, GIANT_TREE			
               break;

            default:
               NSOHabitatScore = 0;
               m_nonHabitatVFOOwlArea += idu_area;
               break;

            } // end switch

         m_owlHabitatScoreMap.insert(std::pair<int, int>(idu, NSOHabitatScore));
         UpdateIDU(pEnvContext, idu, m_colHCSpottOwl, NSOHabitatScore, m_useAddDelta ? ADD_DELTA : SET_DATA);
         } //end if vNWForPlan

      }  // for idu


      // compute suitability value for cover type Mountain Hemlock (MH), which is based on suitability for other cover types
   for (MapLayer::Iterator idu = m_pPolyMapLayer->Begin(); idu < m_pPolyMapLayer->End(); idu++)
      {
      float
         elevation = -1.0,
         distances[500];

      int
         neighbors[500],
         maxPolys = 500;

      //get values from idu layer
      m_pPolyMapLayer->GetData(idu, m_colCoverType, coverType);
      m_pPolyMapLayer->GetData(idu, m_colCanopyCover, canopyCover);
      m_pPolyMapLayer->GetData(idu, m_colStemSize, stemSize);
      m_pPolyMapLayer->GetData(idu, m_colNWForPlan, vNWForPlan);
      m_pPolyMapLayer->GetData(idu, m_colArea, idu_area);
      m_pPolyMapLayer->GetData(idu, pEnvContext->pMapLayer->GetFieldCol("ELEVATION"), elevation);

      int _idu = idu;

      // if Mountain Hemlock with elevation < 1800m and in NW Forest Plan except 3
      if (coverType == CT_MH && (vNWForPlan == 1 || vNWForPlan == 2 || vNWForPlan == 4 || vNWForPlan == 5 || vNWForPlan == 6) && (elevation < m_threshElevation))
         {
         NSOHabitatScore = 0;
         //get IDUs within specified distance (= 1km)
         int nCount = m_pPolyMapLayer->GetNearbyPolys(m_pPolyMapLayer->GetPolygon(idu), neighbors, distances, maxPolys, m_radiusOfHabitat);

         // iterate through IDUs within specified distance (= 1km)
         bool goodNeighborFound = false; 
         for (int j = 0; j < nCount; j++)
            {
            int habitatClassScore = -1;
            //if (m_owlHabitatScoreMap.find(idu) != m_owlHabitatScoreMap.end())
            //   habitatClassScore = m_owlHabitatScoreMap.find(idu)->second;
            if (m_owlHabitatScoreMap.find(neighbors[j]) != m_owlHabitatScoreMap.end())
               habitatClassScore = m_owlHabitatScoreMap.find(neighbors[j])->second;
            //		m_pPolyMapLayer->GetData(neighbors[j], m_colHCSpottOwl, habitatClassScore);
                  // if high suitability in one of the idus within specified radius, then...
            if (habitatClassScore == 2)
               {
               switch (stemSize)
                  {
                  case S_SMALL_TREE:
                  case S_MEDIUM_TREE:
                  case S_LARGE_TREE:
                  case S_GIANT_TREE:
                     if (canopyCover == C_MEDIUM || canopyCover == C_HIGH)
                         goodNeighborFound = true;
                     break;
                  }  // end switch					
               } // end if

            if ( goodNeighborFound )
               break;   // no need to keep looking at more neighbors
            
            }	// end of: for each neighbor

         if ( goodNeighborFound )
            {
            NSOHabitatScore = 1;
            m_moderateHabitatVFOOwlArea += idu_area;
            }
         else
            {
            NSOHabitatScore = 0;
            m_nonHabitatVFOOwlArea += idu_area;
            }

         UpdateIDU(pEnvContext, idu, m_colHCSpottOwl, NSOHabitatScore, m_useAddDelta ? ADD_DELTA | FORCE_UPDATE : SET_DATA);
         } // end coverType is Mountain Hemlock
      } // end idu

      // put Suitability Categories into Post-Run Graph
   float VFOSpottedOwlHabitatLevels[4];
   float time = (float)pEnvContext->currentYear;

   VFOSpottedOwlHabitatLevels[0] = time;
   VFOSpottedOwlHabitatLevels[1] = m_nonHabitatVFOOwlArea / 1E6F;
   VFOSpottedOwlHabitatLevels[2] = m_moderateHabitatVFOOwlArea / 1E6F;
   VFOSpottedOwlHabitatLevels[3] = m_goodHabitatVFOOwlArea / 1E6F;

   m_VFOSpottedOwlSuitabilityArray.AppendRow(VFOSpottedOwlHabitatLevels, 4);

   //set new scores
   m_lowerBound = 0.0f;
   m_upperBound = 1.0f;
   m_rawScore = 0.0f;

   // reset suitability values
   m_nonHabitatVFOOwlArea = 0.0f;
   m_moderateHabitatVFOOwlArea = 0.0f;
   m_goodHabitatVFOOwlArea = 0.0f;

   TRACE(_T("Completed TerrBiodiv.cpp CalculateVFONorthernSpottedOwl() successfully.\n\n"));
   return true;
   }

bool TerrBiodiv::CalculateDownyBromeSuitability(EnvContext *pEnvContext)
   {
   TRACE(_T("Starting TerrBiodiv.cpp CalculateDownyBromeSuitability.\n"));
   ASSERT(pEnvContext != NULL);

   float
      distRoad = -1.0F,
      totTrees = -1.0F,
      totJuniper = -1.0F,
      minMayTemp = -273.0F,
      precipMarch = -99.0F,
      poly_area = 0.0F,
      total_area = 0.0F;

   m_DownyBromeNoSuitabilityArea = 0.0f;
   m_DownyBromeLowSuitabilityArea = 0.0f;
   m_DownyBromeModerateSuitabilityArea = 0.0f;
   m_DownyBromeHighSuitabilityArea = 0.0f;

   // compute suitability value for each idu
   const MapLayer *pLayer = pEnvContext->pMapLayer;
   for (MapLayer::Iterator Poly = pLayer->Begin(); Poly < pLayer->End(); Poly++)
      {
      m_pPolyMapLayer->GetData(Poly, m_colArea, poly_area);
      total_area += poly_area;

      //get values from idu layer	
      m_pPolyMapLayer->GetData(Poly, m_colDistanceRoads, distRoad);
      m_pPolyMapLayer->GetData(Poly, m_colTotalTreesPerHa, totTrees);
      m_pPolyMapLayer->GetData(Poly, m_colTotalJuniperTreesPerHa, totJuniper);
      m_pPolyMapLayer->GetData(Poly, m_colMinTempInMay, minMayTemp);
      m_pPolyMapLayer->GetData(Poly, m_colPrecipInMarch, precipMarch);
      m_pPolyMapLayer->GetData(Poly, m_colArea, poly_area);

      // calculate prob of logistic regression model for Downy Brome
      float prob = 0.0F;

      if (totTrees < 0.0F) totTrees = 0.0F;
      if (totJuniper < 0.0F) totJuniper = 0.0F;

      if (alglib::fp_neq(minMayTemp, -273.0) && alglib::fp_neq(precipMarch, -99.0) && alglib::fp_neq(distRoad, -1.0))
         {
         double x = 0.0274 + 0.0015 * distRoad + 0.0034 * (totTrees / ACRE_PER_HA) - 0.0202 * (totJuniper / ACRE_PER_HA) - 1.0116 * minMayTemp + 0.2146 * precipMarch;
         prob = 1.0f / (1.0f + float(exp(x)));
         }

      m_DownyBromeSuitability = prob;
      //		m_pPolyMapLayer->SetData(Poly, m_colDownyBrome, m_DownyBromeSuitability);

      if ((prob >= 0.0f) && (prob <= 0.10f))
         m_DownyBromeNoSuitabilityArea += poly_area;
      if ((prob > 0.10f) && (prob <= 0.30f))
         m_DownyBromeLowSuitabilityArea += poly_area;
      else if ((prob > 0.30f) && (prob <= 0.70f))
         m_DownyBromeModerateSuitabilityArea += poly_area;
      else if ((prob > 0.70f) && (prob <= 1.0f))
         m_DownyBromeHighSuitabilityArea += poly_area;


      UpdateIDU(pEnvContext, Poly, pEnvContext->pMapLayer->GetFieldCol("HS_DownyB"), m_DownyBromeSuitability, m_useAddDelta ? ADD_DELTA : SET_DATA);
      } //end idu

   // put Suitability Categories into Post-Run Graph
   float downyBromeSuitabilityLevels[5];
   float time = (float)pEnvContext->currentYear;

   downyBromeSuitabilityLevels[0] = time;
   downyBromeSuitabilityLevels[1] = m_DownyBromeNoSuitabilityArea / 1E6F;
   downyBromeSuitabilityLevels[2] = m_DownyBromeLowSuitabilityArea / 1E6F;
   downyBromeSuitabilityLevels[3] = m_DownyBromeModerateSuitabilityArea / 1E6F;
   downyBromeSuitabilityLevels[4] = m_DownyBromeHighSuitabilityArea / 1E6F;

   m_downyBromeSuitabilityArray.AppendRow(downyBromeSuitabilityLevels, 5);

   ////set new scores
   m_lowerBound = 0.0f;
   m_upperBound = 1.0f;
   m_rawScore = 0.0f;

   // reset suitability values
   m_DownyBromeNoSuitabilityArea = 0.0f;
   m_DownyBromeLowSuitabilityArea = 0.0f;
   m_DownyBromeModerateSuitabilityArea = 0.0f;
   m_DownyBromeHighSuitabilityArea = 0.0f;

   TRACE(_T("Completed TerrBiodiv.cpp CalculateDownyBromeSuitability successfully.\n"));
   return true;
   } // end TerrBiodiv::CalculateDownyBromeSuitability

bool TerrBiodiv::CalculateILAPSpeciesSuitability(EnvContext *pEnvContext)
   {
   TRACE(_T("Starting TerrBiodiv.cpp CalculateILAPSpeciesSuitability.\n"));
   ASSERT(pEnvContext != NULL);

   int stemSize = -1;
   int canopyCover = -1;
   int canopyLayer = -1;
   int coverType = -1;
   int region = -1;
   int pvt = -1;
   int habitat = 0;
   int vNWForPlan = -1;
   int tsf = -1;
   int priorVeg = -1;

   float idu_area = 0.0f;
   float	total_area = 0.0f;

   m_noAmericanMartenSuitabilityArea = m_lowAmericanMartenSuitabilityArea = m_moderateAmericanMartenSuitabilityArea = m_highAmericanMartenSuitabilityArea = 0.0f;						// Total area of suitability for American Marten (km2)
   m_noBBWoodpeckerSuitabilityArea = m_lowBBWoodpeckerSuitabilityArea = m_moderateBBWoodpeckerSuitabilityArea = m_highBBWoodpeckerSuitabilityArea = 0.0f;							// Total area of suitability for Black-backed Woodpecker (km2)
   m_noNorthernGoshawkSuitabilityArea = m_lowNorthernGoshawkSuitabilityArea = m_moderateNorthernGoshawkSuitabilityArea = m_highNorthernGoshawkSuitabilityArea = 0.0f;				// Total area of suitability for Northtern Goshawk (km2)
   m_noPileatedWoodpeckerSuitabilityArea = m_lowPileatedWoodpeckerSuitabilityArea = m_moderatePileatedWoodpeckerSuitabilityArea = m_highPileatedWoodpeckerSuitabilityArea = 0.0f;	// Total area of suitability for PileatedWoodpecker (km2)
   m_noRedNSapsuckerSuitabilityArea = m_lowRedNSapsuckerSuitabilityArea = m_moderateRedNSapsuckerSuitabilityArea = m_highRedNSapsuckerSuitabilityArea = 0.0f;						// Total area of suitability for Red-naped/Red-breasted Sapsucker (km2)
   m_noSpottedOwlSuitabilityArea = m_lowSpottedOwlSuitabilityArea = m_moderateSpottedOwlSuitabilityArea = m_highSpottedOwlSuitabilityArea = 0.0f;									// Total area of suitability for Spotted Owl (km2)
   m_noWesternBluebirdSuitabilityArea = m_lowWesternBluebirdSuitabilityArea = m_moderateWesternBluebirdSuitabilityArea = m_highWesternBluebirdSuitabilityArea = 0.0f;				// Total area of suitability for Western Bluebird (km2)
   m_noWHWoodpeckerSuitabilityArea = m_lowWHWoodpeckerSuitabilityArea = m_moderateWHWoodpeckerSuitabilityArea = m_highWHWoodpeckerSuitabilityArea = 0.0f;							// Total area of suitability for White-headed Woodpecker (km2)

   // compute suitability, habitat and non-habitat, and area of suitability categories for each idu
   const MapLayer *pLayer = pEnvContext->pMapLayer;
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      idu_area = 0.0f;
      m_pPolyMapLayer->GetData(idu, m_colArea, idu_area);
      int vegClassScore = -1;
      int pvtScore = 0;
      total_area += idu_area;

      //get values from idu layer
      stemSize = 0;
      canopyCover = 0;
      canopyLayer = 0;
      coverType = 0;
      pvt = 0;
      region = 0;
      vNWForPlan = -1;
      tsf = -1;
      priorVeg = -1;



      m_pPolyMapLayer->GetData(idu, m_colStemSize, stemSize);
      m_pPolyMapLayer->GetData(idu, m_colCanopyCover, canopyCover);
      m_pPolyMapLayer->GetData(idu, m_colCanopyLayers, canopyLayer);
      m_pPolyMapLayer->GetData(idu, m_colCoverType, coverType);
      m_pPolyMapLayer->GetData(idu, m_colPVT, pvt);
      m_pPolyMapLayer->GetData(idu, m_colRegion, region);

      // create indices into lookup tables
      int colSpeciesCode = 1;
      int colSizeIndex = stemSize + 2;
      int colCanopyCoverIndex = canopyCover + 2;
      int colCanopyLayerIndex = canopyLayer + 2;
      int colCoverTypeIndex = -1;

      bool ok = m_coverTypeColIndexMap.Lookup(coverType, colCoverTypeIndex);


      //	if (ok)
      //	{
      for (int species = 0; species < m_speciesCount; species++)
         {
         int sizeScore = m_ILAPSizeLUT.GetAsInt(colSizeIndex, species);
         int canopyCoverScore = m_ILAPCanopyCoverLUT.GetAsInt(colCanopyCoverIndex, species);
         int canopyLayerScore = m_ILAPCanopyLayerLUT.GetAsInt(colCanopyLayerIndex, species);
         int coverTypeScore;

         if (!ok)
            coverTypeScore = 0;
         else
            coverTypeScore = m_ILAPCoverTypeLUT.GetAsInt(colCoverTypeIndex, species);

         vegClassScore = sizeScore + canopyCoverScore + canopyLayerScore;
         habitat = 0;

         if (vegClassScore == 3 && coverTypeScore == 1)
            {
            habitat = 1;
            }

         /*	if (habitat == 0)
               switch (species)
            {
               case AM:
                  m_noAmericanMartenSuitabilityArea += idu_area;
                  break;
               case BBWO:
                  m_noBBWoodpeckerSuitabilityArea += idu_area;
                  break;
               case NOGO:
                  m_noNorthernGoshawkSuitabilityArea += idu_area;
                  break;
               case PIWO:
                  m_noPileatedWoodpeckerSuitabilityArea += idu_area;
                  break;
               case RNSAP:
                  m_noRedNSapsuckerSuitabilityArea += idu_area;
                  break;
               case NSO:
                  m_noSpottedOwlSuitabilityArea += idu_area;
                  break;
               case WEBL:
                  m_noWesternBluebirdSuitabilityArea += idu_area;
                  break;
               case WHWO:
                  m_noWHWoodpeckerSuitabilityArea += idu_area;
                  break;
               default:
                  break;
            }*/

         PVTKey lookupKey(species, region, pvt);

         std::map<PVTKey, int>::const_iterator search = m_ILAPPVTMap.find(lookupKey);
         if (search != m_ILAPPVTMap.end())
            {
            switch (species)
               {
               case AM:
                  {
                  int value = search->second;
                  if (habitat == 0)
                     {
                     value = 0;
                     m_noAmericanMartenSuitabilityArea += idu_area;
                     }
                  else if (habitat == 1 && value == 1)  m_lowAmericanMartenSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 2)  m_moderateAmericanMartenSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 3)  m_highAmericanMartenSuitabilityArea += idu_area;
                  UpdateIDU(pEnvContext, idu, m_colAmericMarten, value, m_useAddDelta ? ADD_DELTA : SET_DATA);
                  }
                  break;
               case BBWO:
                  {
                  m_pPolyMapLayer->GetData(idu, m_colTSF, tsf);
                  m_pPolyMapLayer->GetData(idu, m_colPriorVeg, priorVeg);

                  int priorVegSize = -1;
                  if (priorVeg != -1)
                     priorVegSize = priorVeg / 1000 % 10;

                  int value = search->second;

                  if (vegClassScore < 3 && coverTypeScore == 1)
                     if (tsf <= 10 && tsf != -1)
                        if (priorVegSize >= 6)
                           if (canopyCover != 3)
                              if (canopyLayer != 2)
                                 if (stemSize >= 3)
                                    habitat = 1;

                  if (habitat == 0)
                     {
                     value = 0;
                     m_noBBWoodpeckerSuitabilityArea += idu_area;
                     }
                  else if (habitat == 1 && value == 1) m_lowBBWoodpeckerSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 2) m_moderateBBWoodpeckerSuitabilityArea += idu_area;

                  else if (habitat == 1 && value == 3)
                     {
                     if (tsf > 10 || tsf == -1)
                        {
                        value = 2;
                        m_moderateBBWoodpeckerSuitabilityArea += idu_area;
                        }
                     else
                        m_highBBWoodpeckerSuitabilityArea += idu_area;
                     }

                  UpdateIDU(pEnvContext, idu, m_colBlackBackWoodpeck, value, m_useAddDelta ? ADD_DELTA : SET_DATA);
                  }
                  break;
               case NOGO:
                  {
                  int value = search->second;
                  if (habitat == 0)
                     {
                     value = 0;
                     m_noNorthernGoshawkSuitabilityArea += idu_area;
                     }
                  else if (habitat == 1 && value == 1) m_lowNorthernGoshawkSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 2) m_moderateNorthernGoshawkSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 3) m_highNorthernGoshawkSuitabilityArea += idu_area;
                  UpdateIDU(pEnvContext, idu, m_colNorthernGoshawk, value, m_useAddDelta ? ADD_DELTA : SET_DATA);
                  }
                  break;
               case PIWO:
                  {
                  int value = search->second;
                  if (habitat == 0)
                     {
                     value = 0;
                     m_noPileatedWoodpeckerSuitabilityArea += idu_area;
                     }
                  else if (habitat == 1 && value == 1) m_lowPileatedWoodpeckerSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 2) m_moderatePileatedWoodpeckerSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 3) m_highPileatedWoodpeckerSuitabilityArea += idu_area;
                  UpdateIDU(pEnvContext, idu, m_colPileatedWoodpeck, value, m_useAddDelta ? ADD_DELTA : SET_DATA);
                  }
                  break;
               case RNSAP:
                  {
                  int value = search->second;
                  if (habitat == 0)
                     {
                     value = 0;
                     m_noRedNSapsuckerSuitabilityArea += idu_area;
                     }
                  else if (habitat == 1 && value == 1) m_lowRedNSapsuckerSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 2) m_moderateRedNSapsuckerSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 3) m_highRedNSapsuckerSuitabilityArea += idu_area;
                  UpdateIDU(pEnvContext, idu, m_colRedNappedSapsuck, value, m_useAddDelta ? ADD_DELTA : SET_DATA);
                  }
                  break;
               case NSO:
                  {
                  int value = search->second;
                  m_pPolyMapLayer->GetData(idu, m_colNWForPlan, vNWForPlan);

                  if (habitat == 0 || vNWForPlan == 3)
                     {
                     value = 0;
                     m_noSpottedOwlSuitabilityArea += idu_area;
                     }
                  else if (habitat == 1 && value == 1) m_lowSpottedOwlSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 2) m_moderateSpottedOwlSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 3) m_highSpottedOwlSuitabilityArea += idu_area;
                  UpdateIDU(pEnvContext, idu, m_colSpottedOwl, value, m_useAddDelta ? ADD_DELTA : SET_DATA);
                  }
                  break;
               case WEBL:
                  {
                  int value = search->second;
                  if (habitat == 0)
                     {
                     value = 0;
                     m_noWesternBluebirdSuitabilityArea += idu_area;
                     }
                  else if (habitat == 1 && value == 1) m_lowWesternBluebirdSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 2) m_moderateWesternBluebirdSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 3) m_highWesternBluebirdSuitabilityArea += idu_area;
                  UpdateIDU(pEnvContext, idu, m_colWesternBluebird, value, m_useAddDelta ? ADD_DELTA : SET_DATA);
                  }
                  break;
               case WHWO:
                  {
                  int value = search->second;
                  if (habitat == 0)
                     {
                     value = 0;
                     m_noWHWoodpeckerSuitabilityArea += idu_area;
                     }
                  else if (habitat == 1 && value == 1) m_lowWHWoodpeckerSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 2) m_moderateWHWoodpeckerSuitabilityArea += idu_area;
                  else if (habitat == 1 && value == 3) m_highWHWoodpeckerSuitabilityArea += idu_area;
                  UpdateIDU(pEnvContext, idu, m_colWhiteHeadWoodpeck, value, m_useAddDelta ? ADD_DELTA : SET_DATA);
                  }
                  break;

               default:
                  break;
               }
            }
         }
      //	}
      } //end idu

      //// put Suitability Categories into Post-Run Graph

   WriteILAPSuitabilityAreas(pEnvContext, m_noAmericanMartenSuitabilityArea, m_lowAmericanMartenSuitabilityArea, m_moderateAmericanMartenSuitabilityArea, m_highAmericanMartenSuitabilityArea, m_AmericanMartenSuitabilityArray);
   WriteILAPSuitabilityAreas(pEnvContext, m_noBBWoodpeckerSuitabilityArea, m_lowBBWoodpeckerSuitabilityArea, m_moderateBBWoodpeckerSuitabilityArea, m_highBBWoodpeckerSuitabilityArea, m_BBWoodpeckerSuitabilityArray);
   WriteILAPSuitabilityAreas(pEnvContext, m_noNorthernGoshawkSuitabilityArea, m_lowNorthernGoshawkSuitabilityArea, m_moderateNorthernGoshawkSuitabilityArea, m_highNorthernGoshawkSuitabilityArea, m_NorthernGoshawkSuitabilityArray);
   WriteILAPSuitabilityAreas(pEnvContext, m_noPileatedWoodpeckerSuitabilityArea, m_lowPileatedWoodpeckerSuitabilityArea, m_moderatePileatedWoodpeckerSuitabilityArea, m_highPileatedWoodpeckerSuitabilityArea, m_PileatedWoodpeckerSuitabilityArray);
   WriteILAPSuitabilityAreas(pEnvContext, m_noRedNSapsuckerSuitabilityArea, m_lowRedNSapsuckerSuitabilityArea, m_moderateRedNSapsuckerSuitabilityArea, m_highRedNSapsuckerSuitabilityArea, m_RedNSapsuckerSuitabilityArray);
   WriteILAPSuitabilityAreas(pEnvContext, m_noSpottedOwlSuitabilityArea, m_lowSpottedOwlSuitabilityArea, m_moderateSpottedOwlSuitabilityArea, m_highSpottedOwlSuitabilityArea, m_SpottedOwlSuitabilityArray);
   WriteILAPSuitabilityAreas(pEnvContext, m_noWesternBluebirdSuitabilityArea, m_lowWesternBluebirdSuitabilityArea, m_moderateWesternBluebirdSuitabilityArea, m_highWesternBluebirdSuitabilityArea, m_WesternBluebirdSuitabilityArray);
   WriteILAPSuitabilityAreas(pEnvContext, m_noWHWoodpeckerSuitabilityArea, m_lowWHWoodpeckerSuitabilityArea, m_moderateWHWoodpeckerSuitabilityArea, m_highWHWoodpeckerSuitabilityArea, m_WHWoodpeckerSuitabilityArray);

   //set new scores
   m_lowerBound = 0.0f;
   m_upperBound = 1.0f;
   m_rawScore = 0.0f;

   // reset suitability values

   m_noAmericanMartenSuitabilityArea = m_lowAmericanMartenSuitabilityArea = m_moderateAmericanMartenSuitabilityArea = m_highAmericanMartenSuitabilityArea = 0.0f;					// Total area of suitability for American Marten (km2)
   m_noBBWoodpeckerSuitabilityArea = m_lowBBWoodpeckerSuitabilityArea = m_moderateBBWoodpeckerSuitabilityArea = m_highBBWoodpeckerSuitabilityArea = 0.0f;							// Total area of suitability for Black-backed Woodpecker (km2)
   m_noNorthernGoshawkSuitabilityArea = m_lowNorthernGoshawkSuitabilityArea = m_moderateNorthernGoshawkSuitabilityArea = m_highNorthernGoshawkSuitabilityArea = 0.0f;				// Total area of suitability for Northtern Goshawk (km2)
   m_noPileatedWoodpeckerSuitabilityArea = m_lowPileatedWoodpeckerSuitabilityArea = m_moderatePileatedWoodpeckerSuitabilityArea = m_highPileatedWoodpeckerSuitabilityArea = 0.0f;	// Total area of suitability for PileatedWoodpecker (km2)
   m_noRedNSapsuckerSuitabilityArea = m_lowRedNSapsuckerSuitabilityArea = m_moderateRedNSapsuckerSuitabilityArea = m_highRedNSapsuckerSuitabilityArea = 0.0f;						// Total area of suitability for Red-naped/Red-breasted Sapsucker (km2)
   m_noSpottedOwlSuitabilityArea = m_lowSpottedOwlSuitabilityArea = m_moderateSpottedOwlSuitabilityArea = m_highSpottedOwlSuitabilityArea = 0.0f;									// Total area of suitability for Spotted Owl (km2)
   m_noWesternBluebirdSuitabilityArea = m_lowWesternBluebirdSuitabilityArea = m_moderateWesternBluebirdSuitabilityArea = m_highWesternBluebirdSuitabilityArea = 0.0f;				// Total area of suitability for Western Bluebird (km2)
   m_noWHWoodpeckerSuitabilityArea = m_lowWHWoodpeckerSuitabilityArea = m_moderateWHWoodpeckerSuitabilityArea = m_highWHWoodpeckerSuitabilityArea = 0.0f;							// Total area of suitability for White-headed Woodpecker (km2)

   TRACE(_T("Completed TerrBiodiv.cpp CalculateILAPSpeciesSuitability successfully.\n"));
   return true;

   } //  end TerrBiodiv::CalculateILAPSpeciesSuitability

bool TerrBiodiv::WriteILAPSuitabilityAreas(EnvContext *pEnvContext, float noSpeciesSuitabilityArea, float lowSpeciesSuitabilityArea, float moderateSpeciesSuitabilityArea, float highSpeciesSuitabilityArea, FDataObj &ILAPSpeciesSuitabilityArray)
   {
   float ILAPSpeciesSuitabilityLevels[5];
   float time = (float)pEnvContext->currentYear;

   ILAPSpeciesSuitabilityLevels[0] = time;
   ILAPSpeciesSuitabilityLevels[1] = noSpeciesSuitabilityArea / 1E6F;
   ILAPSpeciesSuitabilityLevels[2] = lowSpeciesSuitabilityArea / 1E6F;
   ILAPSpeciesSuitabilityLevels[3] = moderateSpeciesSuitabilityArea / 1E6F;
   ILAPSpeciesSuitabilityLevels[4] = highSpeciesSuitabilityArea / 1E6F;

   ILAPSpeciesSuitabilityArray.AppendRow(ILAPSpeciesSuitabilityLevels, 5);

   return true;
   }

bool TerrBiodiv::RunHSINorthernSpottedOwl(EnvContext *pEnvContext)
   {
   // initialize the Polygon <-> Grid relationship: relates grid to IDU polygon layer
   TRACE(_T("Generate Poly < - > Grid Lookup Table.\n"));
   bool lpgl = LoadPolyGridLookup(pEnvContext);
   if (!lpgl)
      FailAndReturn("LoadPolyGridLookup() returned false in TerrBiodiv.cpp Init");

   // calculate HSI for spotted owl (1) on grid cell level, (2) get average HSI values on polygon level
   TRACE(_T("Calculate grid level suitability for spotted owl.\n"));
   bool cs = CalculateHSINorthenSpottedOwlSuitability(pEnvContext);
   if (!cs)
      FailAndReturn("CalculateHSINorthenSpottedOwlSuitability() returned false in TerrBiodiv.cpp Init");

   return TRUE;
   }

bool TerrBiodiv::RunMuleDeerHSI(EnvContext *pEnvContext)
   {
   // initialize the Polygon <-> Grid relationship: relates grid to IDU polygon layer
   TRACE(_T("Generate Poly < - > Grid Lookup Table.\n"));
   bool lpgl = LoadPolyGridLookup(pEnvContext);
   if (!lpgl)
      FailAndReturn("LoadPolyGridLookup() returned false in TerrBiodiv.cpp Init");

   // calculate the suitability for mule deer
   TRACE(_T("Calculate grid level suitability for mule deer.\n"));
   /* bool csmd = CalculateSuitabilityMuleDeer( pEnvContext );*/
   bool csmd = CalculateMuleDeerSuitability(pEnvContext);
   if (!csmd)
      FailAndReturn("CalculateSuitabilityMuleDeer() returned false in TerrBiodiv.cpp Init");

   return TRUE;
   }

bool TerrBiodiv::RunILAPHabitatModels(EnvContext *pEnvContext)
   {
   // load .csv table with relationship between numerical code fuer VDDT results (numerical code by Sulzmann) and habitat suitability for different species.
   TRACE(_T("Load ILAP habitat suitability lookup table.\n"));
   bool lilt = CalculateILAPSpeciesSuitability(pEnvContext);
   //   bool lilt = LoadILAPLookupTable( pEnvContext, m_initStr );
   if (!lilt)
      //FailAndReturn("LoadILAPLookupTable() returned false in TerrBiodiv.cpp Init");
      FailAndReturn("CalculateILAPSpeciesSuitability() returned false in TerrBiodiv.cpp Init");

   return TRUE;
   }

bool TerrBiodiv::RunDownyBrome(EnvContext *pEnvContext)
   {
   // calculate the suitability for downy brome on polygon level
   TRACE(_T("Calculate polygon level suitability for downy brome.\n"));
   bool cps = CalculateDownyBromeSuitability(pEnvContext);
   if (!cps)
      FailAndReturn("CalculateDownyBromeSuitability() returned false in TerrBiodiv.cpp Init");

   return TRUE;
   }

bool TerrBiodiv::RunHCNorthernSpottedOwl(EnvContext *pEnvContext)
   {
   // calculate habitat classes for northern spotted owl
   TRACE(_T("Calculate VFO (habitat classes) for Northern Spotted Owl.\n"));
   //  bool cps = CalculateHCNorthernSpottedOwl( pEnvContext );
   bool cps = CalculateVFONorthernSpottedOwl(pEnvContext);
   if (!cps)
      FailAndReturn("CalculateVFONorthernSpottedOwl() returned false in TerrBiodiv.cpp Init");

   return TRUE;
   }