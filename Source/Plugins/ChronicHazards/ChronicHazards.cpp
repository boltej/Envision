// ChronicHazards.cpp : Defines the initialization routines for the DLL.
//
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

You should have received a copy of the GNU General Publi c License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/

#include "stdafx.h"
#pragma hdrstop

#include "ChronicHazards.h"

#include <Map.h>
#include <PathManager.h>
#include <EnvConstants.h>
#include <cmath>
#include "ChronicHazards.h"

#include <tixml.h>
#include <vector>
#include <Maplayer.h>
#include <AlgLib\AlgLib.h>
#include <UNITCONV.H>
#include <DATE.HPP>
#include <randgen\Randunif.hpp>
#include <randgen\Randnorm.hpp>
#include <randgen\RandExp.h>
#include <EnvModel.h>
#include <complex>
#include "secant.h"
#include "bisection.h"
#include "GEOMETRY.HPP"
#include "DeltaArray.h"
#include "GDALWrapper.h"
#include "GeoSpatialDataObj.h"
#include <time.h> 


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const float g = 9.81f;                  // gravity m/sec*sec
const double MHW = 2.1;                  // NAVD88 mean high water meters 
const double MLLW = -0.46;               // NAVD88 mean lower low water meters (t Westport)
const double EXLW_MLLW = 1.1;            // NAVD88 Extreme low water below MLLW meters 
const double MILESPERMETER = 0.000621371;   // Miles per Meter

float PolicyInfo::m_budgetAllocFrac = 0.5f;


FILE *fpPolicySummary = NULL;


TWLDATAFILE ChronicHazards::m_datafile;
COSTS ChronicHazards::m_costs;




//inline bool FailAndReturn(FailMsg) 
//            { msg = failureID + (_T(FailMsg)); 
//      Report::ErrorMsg(msg); 
//      return false; }

//=====================================================================================================
// ChronicHazards
//=====================================================================================================


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new ChronicHazards; }



ChronicHazards::ChronicHazards(void)
   : EnvModelProcess(0, 0),
   m_runFlags(CF_NONE),
   // Layers and Grids                  
   m_pIDULayer(NULL),                  // ptr to IDU Layer                  
   m_pRoadLayer(NULL),                  // ptr to Road Layer
   m_pBldgLayer(NULL),                  // ptr to Building Layer
   m_pInfraLayer(NULL),               // ptr to Infrastructure Building Layer
   m_pElevationGrid(NULL),               // ptr to Minimum Elevation Grid

   m_pOrigDuneLineLayer(NULL),            // ptr to Original Dune Line Layer
   m_pNewDuneLineLayer(NULL),            // ptr to Changed Dune Line Layer

                                       //   m_pTsunamiInunLayer(NULL)               // ptr to Tsunami Inundaiton Layer

   m_pPolygonGridLkUp(NULL),            // provides mapping between IDU layer and DEM grid
   m_pPolylineGridLkUp(NULL),            // provides mapping between roads layer and DEM grid
   m_pIduBuildingLkup(NULL),            // provides mapping between IDU layer, building (point) layer
   m_pPolygonInfraPointLkup(NULL),         // provides mapping between IDU layer, infrastructure (point) layer

                                          // Habitat Grids
   m_pBayBathyGrid(NULL),
   m_pEelgrassGrid(NULL),
   m_pTidalBathyGrid(NULL),


   // Flooded Grid is either 0 = not flooded, or value = TWL (Bathtub flood model) OR value = WaterHeight (Bates Flood Model)
   // Eroded Grid is 0 = not eroded,  1 = event eroded, 2 = chronically eroded
   m_pFloodedGrid(NULL),
   m_pCumFloodedGrid(NULL),
   m_pErodedGrid(NULL),
   m_pBayTraceLayer(NULL),

   // Bates Flooding Model
   m_pNewElevationGrid(NULL),
   m_pWaterElevationGrid(NULL),            // ptr to Flooded Water Depth Grid
   m_pDischargeGrid(NULL),               // ptr to Volumetric Flow rate Grid   // Lookup tables
   m_pManningMaskGrid(NULL),            // ptr to Bates Model Mannings Coefficient Grid (value = -1 if not running Bates model in region)

   m_pInletLUT(NULL),
   m_pBayLUT(NULL),
   m_pProfileLUT(NULL),
   m_pSAngleLUT(NULL),
   m_pTransectLUT(NULL),

   // Setup variables
   //m_cityDir(NULL),
   m_debugOn(false),
   m_debug(0),
   m_numBayPts(0),
   m_numBayStations(0),
   m_numBldgs(0),
   m_numDays(-1),
   m_numDunePts(0),
   //m_numOceanShoresPts(0),
   //m_numJettyPts(0),
   //m_numWestportPts(0),
   m_numProfiles(0),
   m_numTransects(0),
   m_numYears(-1),
   m_simulationCount(0),
   m_maxDuneIndx(-1),
   /*m_maxTraceIndx(-1),
   m_minTraceIndx(99999),*/
   m_maxTransect(-1),
   m_minTransect(99999),
   m_randIndex(0),
   m_totalBudget(0),
   m_removeFromHazardZoneRatio(0.5f),
   m_raiseRelocateSafestSiteRatio(0.4f),

   // Policy Scenario Variables
   m_climateScenarioID(0),               // 0=BaseSLR, 1=LowSLR, 2=MedSLR, 3=HighSLR, 4=WorstCaseSLR
   m_runEelgrassModel(0),               // Run Eelgrass Model:  0=Off, 1=On
   m_runConstructBPSPolicy(0),            // 0=Off, 1=On
   m_runConstructSafestPolicy(0),         // 0=Off, 1=On
   m_runConstructSPSPolicy(0),            // 0=Off, 1=On
   m_runMaintainBPSPolicy(0),            // 0=Off, 1=On
   m_runMaintainSPSPolicy(0),            // 0=Off, 1=On
   m_runNourishByTypePolicy(0),         // 0=Off, 1=On
   m_runNourishSPSPolicy(0),            // 0=Off, 1=On
   m_runRaiseInfrastructurePolicy(0),      // 0=Off, 1=On
   m_runRelocateSafestPolicy(0),         // 0=Off, 1=On   
   m_runRemoveBldgFromHazardZonePolicy(0),      // 0=Off, 1=On
   m_runRemoveInfraFromHazardZonePolicy(0),      // 0=Off, 1=On
   m_runSpatialBayTWL(0),               // 0=Off, 1=On

                                       // XML read class members for swtiching
   m_exportMapInterval(-1),            // -1=Off, Interval number of yrs to write 
   m_findSafeSiteCell(0),               // 0=Off, 1=On
   m_runBayFloodModel(0),               // Run Bay Flooding Model: 0=Off, 1=On
   m_writeDailyBouyData(0),               // 0=Off, 1=On
   m_writeRBF(0),                     // 0=Off, postive value=sample interval

   m_bfeCount(0),
   m_eroCount(0),
   m_floodCountBPS(0),
   m_floodCountSPS(0),
   m_maintFreqBPS(3),
   //   m_maintFreqSPS(3),
   m_nourishFactor(5),
   m_nourishFreq(5),
   m_nourishPercentage(20),
   m_ssiteCount(0),
   m_useMaxTWL(1),
   m_windowLengthEroHzrd(5),
   m_windowLengthFloodHzrd(5),
   m_windowLengthTWL(),

   m_accessThresh(90.f),
   m_avgBackSlope(0.08f),
   m_avgFrontSlope(0.16f),
   m_avgDuneCrest(8.0),
   m_avgDuneHeel(6.0),
   m_avgDuneToe(5.0),
   m_avgDuneWidth(50.0f),
   m_avgWL0(0.0f),
   m_constTD(10.0f),
   m_delta(0.0f),
   m_minDistToProtecteBldg(200.0f),
   /*m_duneCrestSPS(0.0f),
   m_duneHeelSPS(0.0f),
   m_duneToeSPS(0.0f),*/
   m_idpyFactor(0.4f),
   m_inletFactor(0.f),
   m_maxArea(7000000.0f),
   m_maxBPSHeight(8.5f),
   m_minBPSHeight(2.0f),
   m_minBPSRoughness(0.55f),
   m_radiusOfErosion(10.0f),
   m_rebuildFactorSPS(0.0),
   m_safetyFactor(1.5f),
   m_slopeBPS(0.5f),
   m_slopeThresh(0.0f),
   m_shorelineLength(10.0f),
   m_topWidthSPS(0.0f),

   m_numRows(-1),                     // Number of rows in DEM, Flooded, Eroded grid
   m_numCols(-1),                     // Number of columns in DEM, Flooded, Eroded grid   
   m_cellWidth(0.0),                  // cell Width (m) in DEM, Flooded, Eroded grid
   m_cellHeight(0.0),                  // cell Height (m) in DEM, Flooded, Eroded grid

                                       // Habitat in the Bay
   m_numBayRows(-1),                  // Number of rows in Bay Bathy, Eelgrass grid
   m_numBayCols(-1),                  // Number of columns in Bay Bathy, Eelgrass grid   
   m_bayCellWidth(0.0),               // cell Width (m) in Bay Bathy, Eelgrass grid
   m_bayCellHeight(0.0),               // cell Height (m) in Bay Bathy, Eelgrass grid

                                       // Habitat on the edges of the Bay in inlet rivers
   m_numTidalRows(-1),                  // Number of rows in Tidal Bathy grid
   m_numTidalCols(-1),                  // Number of columns in Tidal Bathy grid   
   m_tidalCellWidth(0.0),               // cell Width (m) in Tidal Bathy grid
   m_tidalCellHeight(0.0),               // cell Height (m) in Tidal Bathy grid

                                       // Bates Flooding Model
   m_runBatesFlood(0),                  // 0=Off, 1=On 
   m_timeStep(.1f),                  // default timeStep (hours) for Bates Flooding Model; can be set using environemnt variable
   m_batesFloodDuration(6.0f),                  // default time duration for Bates Flooding Model; can be set using environment variable
   m_ManningCoeff(0.025f),

   m_erodedBldgCount(0),
   m_floodedBldgCount(0),
   m_floodedInfraCount(0),
   m_noBldgsRemovedFromHazardZone(0),
   m_noConstructedBPS(0),
   m_noConstructedDune(0),
   //   m_numCntyNourishProjects(0),
   //   m_numCtnyNourishPrjts(0),

   m_avgCountyAccess(0.0f),
   //m_avgOceanShoresAccess(0.0f),
   //m_avgWestportAccess(0.0f),
   //m_avgJettyAccess(0.0f),

   //   m_eroFreq,
   //   m_floodFreq, 
   m_addedHardenedShoreline(0.0),
   m_addedHardenedShorelineMiles(0.0),
   m_addedRestoredShoreline(0.0),
   m_addedRestoredShorelineMiles(0.0),
   m_constructCostBPS(0.0),
   m_eelgrassArea(0.0),
   m_eelgrassAreaSqMiles(0.0),
   m_erodedRoad(0.0),
   m_erodedRoadMiles(0.0),
   m_floodedArea(0.0),
   m_floodedAreaSqMiles(0.0),
   m_floodedRailroadMiles(0.0),
   m_floodedRoad(0.0),
   m_floodedRoadMiles(0.0),
   m_intertidalArea(0.0),
   m_intertidalAreaSqMiles(0.0),
   m_maintCostBPS(0.0),
   m_maintCostSPS(0.0),
   m_maintVolumeSPS(0.0),
   //  m_maxTWL(0.0),
   //  m_meanErosion(0.0),
   m_meanTWL(0.0),
   m_nourishVolumeBPS(0.0),
   m_nourishVolumeSPS(0.0),
   m_nourishCostBPS(0.0),
   m_nourishCostSPS(0.0),
   m_percentHardenedShoreline(0.0),
   m_percentRestoredShoreline(0.0),
   m_percentHardenedShorelineMiles(0.0),
   m_percentRestoredShorelineMiles(0.0),
   m_restoreCostSPS(0.0),
   m_totalHardenedShoreline(0.0),
   m_totalHardenedShorelineMiles(0.0),
   m_totalRestoredShoreline(0.0),
   m_totalRestoredShorelineMiles(0.0),

   // IDU coverage columns
   m_colArea(-1),
   m_colBaseFloodElevation(-1),
   m_colBeachfront(-1),
   m_colCPolicy(-1),
   m_colEasementYear(-1),
   m_colFID(-1),
   m_colFloodFreq(-1),
   m_colFracFlooded(-1),
   m_colIDUCityCode(-1),
   m_colIDUEroded(-1),
   m_colIDUFlooded(-1),
   m_colIDUFloodZone(-1),
   m_colIDUFloodZoneCode(-1),
   m_colImpValue(-1),
   m_colLandValue(-1),
   m_colLength(-1),
   m_colMaxElevation(-1),
   m_colMinElevation(-1),
   m_colNDU(-1),
   m_colNEWDU(-1),
   m_colNorthingBottom(-1),
   m_colNorthingTop(-1),
   m_colNumStruct(-1),
   m_colPopCap(-1),
   m_colPopDensity(-1),
   m_colPopulation(-1),
   m_colPropEroded(-1),
   m_colRemoveBldgYr(-1),
   m_colRemoveSC(-1),
   m_colSafeLoc(-1),
   m_colSafeSite(-1),
   m_colSafeSiteYear(-1),
   m_colSafestSiteCol(-1),
   m_colSafestSiteRow(-1),
   m_colTaxlotID(-1),
   m_colTsunamiHazardZone(-1),
   m_colType(-1),
   m_colValue(-1),
   m_colZone(-1),

   //  Dune Line coverage columns     
   //  m_colFreeboard(-1),   
   //  m_colInlet(-1),    
   //  m_colKDstd(-1),    
   //  m_colSA(-1),    
   //  m_colStkTWL(-1),    
   //  m_colTWL(-1),    
   m_colA(-1),
   m_colAddYearBPS(-1),
   m_colAddYearSPS(-1),
   m_colAlpha(-1),
   m_colAlpha2(-1),
   m_colAmtFlooded(-1),
   m_colAvgKD(-1),
   //   m_colAvgDuneC(-1),
   //   m_colAvgDuneT(-1),
   //   m_colAvgEro(-1),
   m_colBackshoreElev(-1),
   m_colBackSlope(-1),
   m_colBchAccess(-1),
   m_colBchAccessFall(-1),
   m_colBchAccessSpring(-1),
   m_colBchAccessSummer(-1),
   m_colBchAccessWinter(-1),
   m_colBeachType(-1),
   m_colBeachWidth(-1),
   m_colConstructVolumeSPS(-1),
   m_colFrontSlope(-1),
   m_colLengthBPS(-1),
   m_colLengthSPS(-1),
   m_colRemoveYearBPS(-1),
   m_colCalDate(-1),
   m_colCalDCrest(-1),
   m_colCalDToe(-1),
   m_colCalEroExt(-1),
   m_colCostBPS(-1),
   m_colCostMaintBPS(-1),
   m_colCostSPS(-1),
   m_colCPolicyL(-1),
   m_colDuneBldgDist(-1),
   m_colDuneBldgIndx(-1),
   m_colDuneCrest(-1),
   m_colDuneEroFreq(-1),
   m_colDuneFloodFreq(-1),
   m_colDuneHeel(-1),
   m_colDuneWidth(-1),
   m_colDuneIndx(-1),
   m_colDuneToe(-1),
   m_colDuneToeSPS(-1),
   m_colEastingCrest(-1),
   m_colEastingCrestProfile(-1),
   //   m_colEastingCrestSPS(-1),
   m_colEastingShoreline(-1),
   m_colEastingToe(-1),
   m_colEastingToeBPS(-1),
   m_colEastingToeSPS(-1),
   m_colFlooded(-1),
   m_colForeshore(-1),
   m_colGammaBerm(-1),
   m_colGammaRough(-1),
   m_colHb(-1),
   m_colHdeep(-1),
   m_colHeelWidthSPS(-1),
   m_colHeightBPS(-1),
   m_colHeightSPS(-1),
   m_colIDDCrest(-1),
   m_colIDDCrestFall(-1),
   m_colIDDCrestSpring(-1),
   m_colIDDCrestSummer(-1),
   m_colIDDCrestWinter(-1),
   m_colIDDToe(-1),
   m_colIDDToeFall(-1),
   m_colIDDToeSpring(-1),
   m_colIDDToeSummer(-1),
   m_colIDDToeWinter(-1),
   m_colKD(-1),
   m_colLatitudeToe(-1),
   m_colLittCell(-1),
   m_colLongitudeToe(-1),
   m_colMaintVolumeSPS(-1),
   m_colMaintYearBPS(-1),
   m_colMaintYearSPS(-1),
   m_colMvAvgIDPY(-1),
   m_colMvAvgTWL(-1),
   m_colMvMaxTWL(-1),
   m_colNorthingCrest(-1),
   m_colNorthingToe(-1),
   m_colNourishFreqBPS(-1),
   m_colNourishFreqSPS(-1),
   m_colNourishYearBPS(-1),
   m_colNourishYearSPS(-1),
   m_colNourishVolBPS(-1),
   m_colNourishVolSPS(-1),
   m_colOrigBeachType(-1),
   m_colPrevDuneCr(-1),
   m_colPrevSlope(-1),
   m_colProfileIndx(-1),
   m_colScomp(-1),
   m_colShoreface(-1),
   m_colShorelineAngle(-1),
   m_colShorelineChange(-1),
   m_colTD(-1),
   m_colThreshDate(-1),
   m_colTopWidthSPS(-1),
   m_colTranIndx(-1),
   m_colTranSlope(-1),
   m_colTS(-1),
   m_colWb(-1),
   m_colWDirection(-1),
   m_colWidthBPS(-1),
   m_colWidthSPS(-1),
   m_colWHeight(-1),
   m_colWPeriod(-1),
   m_colXAvgKD(-1),
   m_colYrAvgLowSWL(-1),
   m_colYrAvgSWL(-1),
   m_colYrAvgTWL(-1),
   m_colYrMaxDoy(-1),
   m_colYrMaxIGSwash(-1),
   m_colYrMaxIncSwash(-1),
   m_colYrMaxSetup(-1),
   m_colYrMaxSwash(-1),
   m_colYrMaxSWL(-1),
   m_colYrFMaxTWL(-1),
   m_colYrMaxTWL(-1),

   // cols for debugging
   m_colBWidth(-1),
   m_colComputedSlope(-1),
   m_colComputedSlope2(-1),
   m_colDeltaX(-1),
   m_colDuneBldgEast(-1),
   m_colDuneBldgIndx2(-1),
   m_colNewEasting(-1),
   m_colNumIDPY(-1),
   m_colRunupFlag(-1),
   m_colSTKRunup(-1),
   m_colSTKR2(-1),
   m_colSTKTWL(-1),
   m_colStructR2(-1),
   m_colTAWSlope(-1),
   m_colTypeChange(-1),

   // Bay Trace coverage columns
   m_colBayTraceIndx(-1),
   m_colBayYrAvgSWL(-1),
   m_colBayYrAvgTWL(-1),
   m_colBayYrMaxSWL(-1),
   m_colBayYrMaxTWL(-1),

   // Road Layer Columns
   m_colRoadFlooded(-1),
   m_colRoadLength(-1),
   m_colRoadType(-1),

   // Building Layer coverage columns
   //   m_colBldgDuneDist(-1),
   //   m_colBldgDuneIndx(-1),
   //   m_colBldgMaxElev(-1), 
   m_colBldgBFE(-1),
   m_colBldgBFEYr(-1),
   m_colBldgCityCode(-1),
   m_colBldgEroded(-1),
   m_colBldgEroFreq(-1),
   m_colBldgFloodFreq(-1),
   m_colBldgFlooded(-1),
   //m_colBldgFloodHZ(-1),
   m_colBldgFloodYr(-1),
   m_colBldgIDUIndx(-1),
   m_colBldgNDU(-1),
   m_colBldgNEWDU(-1),
   m_colBldgRemoveYr(-1),
   m_colBldgSafeSite(-1),
   m_colBldgSafeSiteYr(-1),
   m_colBldgTsunamiHZ(-1),
   m_colNumBldgs(-1),

   // Infrastructure Layer coverage columns
   m_colInfraBFE(-1),
   m_colInfraBFEYr(-1),
   m_colInfraCount(-1),
   m_colInfraCritical(-1),
   m_colInfraDuneIndx(-1),
   m_colInfraEroded(-1),
   m_colInfraFlooded(-1),
   m_colInfraFloodYr(-1),
   m_colInfraValue(-1)

   //  m_ImpactDaysPerYr(90)
   //   m_pAvgTWL( NULL )
   //   m_pEroFreq( NULL )
   //   m_pFloodFreq( NULL )

   //   m_pRnSimulation(NULL)
   { }

ChronicHazards::~ChronicHazards(void)
   {
   if (m_pPolygonGridLkUp != NULL) delete m_pPolygonGridLkUp;
   if (m_pPolylineGridLkUp != NULL) delete m_pPolylineGridLkUp;
   if (m_pIduBuildingLkup != NULL) delete m_pIduBuildingLkup;
   if (m_pPolygonInfraPointLkup != NULL) delete m_pPolygonInfraPointLkup;

   if (m_pTransectLUT != NULL) delete m_pTransectLUT;
   if (m_pProfileLUT != NULL) delete m_pProfileLUT;
   if (m_pSAngleLUT != NULL) delete m_pSAngleLUT;
   if (m_pInletLUT != NULL) delete m_pInletLUT;

   /*if (m_pAvgTWL != NULL) delete m_pAvgTWL;
   if (m_pEroFreq != NULL) delete m_pEroFreq;
   if (m_pFloodFreq != NULL) delete m_pFloodFreq;*/
   }


bool ChronicHazards::Init(EnvContext *pEnvContext, LPCTSTR initStr)
   {
   // read config files and set internal variables based on that
   LoadXml(initStr);

   // get Map pointer
   m_pMap = pEnvContext->pMapLayer->GetMapPtr();

   // ****** Load MapLayers of the Map ****** //

   // IDU Layer
   m_pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   // Roads Layer
   m_pRoadLayer = m_pMap->GetLayer("Roads");

   // Buildings Layer
   m_pBldgLayer = m_pMap->GetLayer("Buildings");
   if ( m_pBldgLayer != NULL )
      m_numBldgs = m_pBldgLayer->GetRowCount();

   // Infrastructure Layer
   m_pInfraLayer = m_pMap->GetLayer("Infrastructure Locations");

   // Original Dune line layer
   m_pOrigDuneLineLayer = m_pMap->GetLayer("Dune Points");
   // Tidal Bathymetry

   //m_pBayTraceLayer = m_pMap->GetLayer("Bay Trace Pts");
   //m_pBayTraceLayer->Hide();

   // Elevation
   m_pElevationGrid = m_pMap->GetLayer("Elevation");
   if (m_pElevationGrid != NULL)
      {
      m_pElevationGrid->Hide();
      m_numRows = m_pElevationGrid->GetRowCount();
      m_numCols = m_pElevationGrid->GetColCount();
      m_pElevationGrid->GetCellSize(m_cellWidth, m_cellHeight);
      }

   // Bay Bathymetry
   m_pBayBathyGrid = m_pMap->GetLayer("Bay Bathy");
   if (m_pBayBathyGrid != NULL)
      {
      m_pBayBathyGrid->Hide();
      m_numBayRows = m_pBayBathyGrid->GetRowCount();
      m_numBayCols = m_pBayBathyGrid->GetColCount();
      m_pBayBathyGrid->GetCellSize(m_bayCellWidth, m_bayCellHeight);
      }

   // Bates Model Mask
   if (m_runFlags & CH_MODEL_FLOODING)
      {
      m_pManningMaskGrid = m_pMap->GetLayer("Manning Mask");
      m_pManningMaskGrid->Hide();
      m_pFloodedGrid = m_pMap->CloneLayer(*m_pElevationGrid);
      m_pFloodedGrid->m_name = "Flooded Grid";
      m_pCumFloodedGrid = m_pMap->CloneLayer(*m_pElevationGrid);
      m_pCumFloodedGrid->m_name = "Cumulative Flooded Grid";
      }

   // Tidal Bathymetry
   m_pTidalBathyGrid = m_pMap->GetLayer("Tidal Bathy");
   if (m_pTidalBathyGrid != NULL)
      {
      m_pTidalBathyGrid->Hide();
      m_numTidalRows = m_pTidalBathyGrid->GetRowCount();
      m_numTidalCols = m_pTidalBathyGrid->GetColCount();
      m_pTidalBathyGrid->GetCellSize(m_tidalCellWidth, m_tidalCellHeight);
      }

   // Tsunami Inundation generate
   // --not needed but could be use as sanity check
   //  m_pTsunamiInunLayer = m_pMap->GetLayer("Tsunami Inundation Zone");

   // Check and store relevant columns (in IDU coverage)

   CheckCol(m_pIDULayer, m_colArea, "Area", TYPE_FLOAT, CC_MUST_EXIST);

   if (m_runFlags & CH_REPORT_BUILDINGS )
      {
      CheckCol(m_pIDULayer, m_colZone, "ZONE_CODE", TYPE_INT, CC_MUST_EXIST);
      CheckCol(m_pIDULayer, m_colPopDensity, "PopDens", TYPE_DOUBLE, CC_MUST_EXIST);
      CheckCol(m_pIDULayer, m_colPopulation, "Population", TYPE_DOUBLE, CC_MUST_EXIST);
      CheckCol(m_pIDULayer, m_colPopCap, "POP_CAP", TYPE_FLOAT, CC_AUTOADD);
      }

   ////////CheckCol(m_pIDULayer, m_colNorthingBottom, "Bottom", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pIDULayer, m_colNorthingTop, "Top", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////
   ////////CheckCol(m_pIDULayer, m_colLength, "Length", TYPE_FLOAT, CC_MUST_EXIST);
   ////////CheckCol(m_pIDULayer, m_colIDUAddBPSYr, "ADD_YR_BPS", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colNDU, "NDU", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colNEWDU, "NewDU", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colBeachfront, "BEACHFRONT", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colIDUCityCode, "CITY_CODE", TYPE_INT, CC_AUTOADD);
   ////////
   ////////// IDU Attributes for Safest Site Policy
   ////////
   ////////CheckCol(m_pIDULayer, m_colMinElevation, "MIN_ELEV", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colMaxElevation, "MAX_ELEV", TYPE_FLOAT, CC_AUTOADD);
   ////////
   ////////// These can be calculated once in Init by turning on the findSafeSiteCell flag in GHC_Hazards.xml
   ////////// They are the DEM cell location of the highest elevation in an IDU
   ////////CheckCol(m_pIDULayer, m_colSafestSiteRow, "SAFE_ROW", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colSafestSiteCol, "SAFE_COL", TYPE_FLOAT, CC_AUTOADD);
   ////////
   ////////// 1/0 Field to indicate Safe Site is available, 0 if taken  // not implemented = 0 if closer to coast, to avoid construction of new buildings
   ////////CheckCol(m_pIDULayer, m_colSafeSite, "SAFE_SITE", TYPE_INT, CC_MUST_EXIST);
   ////////m_pIDULayer->SetColData(m_colSafeSite, VData(1), true);
   ////////
   ////////CheckCol(m_pIDULayer, m_colRemoveBldgYr, "REMBLDGYR", TYPE_INT, CC_AUTOADD);
   ////////m_pIDULayer->SetColData(m_colRemoveBldgYr, VData(0), true);
   ////////
   ////////// change to MUST_EXIST
   ////////CheckCol(m_pIDULayer, m_colBaseFloodElevation, "STATIC_BFE", TYPE_FLOAT, CC_MUST_EXIST);   // read
   ////////CheckCol(m_pIDULayer, m_colBaseFloodElevation, "STATIC_BFE", TYPE_FLOAT, CC_MUST_EXIST);   // readflood
   ////////CheckCol(m_pIDULayer, m_colIDUFloodZone, "FLD_ZONE", TYPE_STRING, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colIDUFloodZoneCode, "FLD_HZ", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colTsunamiHazardZone, "TSUNAMI_HZ", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colLandValue, "LANDVALUE", TYPE_LONG, CC_MUST_EXIST);
   ////////
   ////////// 1/0 field if IDU was annually flooded or eroded
   ////////CheckCol(m_pIDULayer, m_colIDUFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pIDULayer, m_colIDUEroded, "ERODED", TYPE_INT, CC_AUTOADD);
   ////////
   ////////CheckCol(m_pIDULayer, m_colFracFlooded, "FRACFLOOD", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pIDULayer->SetColData(m_colFracFlooded, VData(0.0f), true);
   ////////
   ////////CheckCol(m_pIDULayer, m_colEasementYear, "EASEMNT_YR", TYPE_INT, CC_AUTOADD);
   ////////m_pIDULayer->SetColData(m_colEasementYear, VData(0.0f), true);
   ////////
   ////////// ****** Finished Columns for IDU Coverage ****** //
   ////////
   //////////These attributes come from LIDAR and other data sets in the original shape file
   ////////// MUST_EXIST 
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneIndx, "DUNEINDX", TYPE_LONG, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colTranSlope, "TANB", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneToe, "DT", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneCrest, "DUNECREST", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colBeachWidth, "BEACHWDTH", TYPE_FLOAT, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colNorthingToe, "NORTHING", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colEastingToe, "EASTING", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colNorthingCrest, "NORTHNGCR", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colEastingCrest, "EASTNGCR", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colEastingShoreline, "EASTNGSH", TYPE_DOUBLE, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colShorelineChange, "SCRate", TYPE_FLOAT, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colBeachType, "BEACHTYPE", TYPE_INT, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colGammaRough, "GAMMAROUGH", TYPE_FLOAT, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneHeel, "DUNEHEEL", TYPE_FLOAT, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colFrontSlope, "FRONTSLOPE", TYPE_FLOAT, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colBackSlope, "BACKSLOPE", TYPE_FLOAT, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneWidth, "DUNEWIDTH", TYPE_FLOAT, CC_MUST_EXIST);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colScomp, "COMPSLOPE", TYPE_FLOAT, CC_AUTOADD);
   ////////
   ////////
   ////////
   ////////// short term shoreline change rate
   //////////  CheckCol(m_pOrigDuneLineLayer, m_colEPR, "EPR", TYPE_FLOAT, CC_AUTOADD);
   ////////
   ////////// Shoreline locations to tally specific metrics or set differnt actions
   ////////CheckCol(m_pOrigDuneLineLayer, m_colLittCell, "LOC", TYPE_INT, CC_AUTOADD);
   ////////
   ////////// These attributes need values to be determined
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colShorelineAngle, "GAMMABETA", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colShorelineAngle, VData(0.0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colGammaBerm, "GAMMABERM", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colGammaBerm, VData(1.0f), true);
   ////////
   //////////   CheckCol(m_pOrigDuneLineLayer, m_colEastingMHW, "EASTINGMHW", TYPE_DOUBLE, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colLongitudeToe, "LONGITUDE", TYPE_DOUBLE, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colLatitudeToe, "LATITUDE", TYPE_DOUBLE, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colOrigBeachType, "ORBCHTYPE", TYPE_INT, CC_AUTOADD);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colPrevSlope, "PREVTANB", TYPE_DOUBLE, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colPrevDuneCr, "PREVDUNECR", TYPE_DOUBLE, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colPrevRow, "PREVROW", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colPrevCol, "PREVCOL", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colEastingCrestProfile, "PEASTINGCR", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colEastingCrestProfile, VData(0.0), true);
   ////////
   ////////// These attributes are filled in on the fly from the Lookup tables to connect up the various layers
   ////////
   ////////// Index to lookup 
   ////////CheckCol(m_pOrigDuneLineLayer, m_colTranIndx, "TRANINDX", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colProfileIndx, "PROFINDX", TYPE_INT, CC_AUTOADD);
   ////////
   ////////// Bruun Rule Slope (measured MHW - depth at 25m contour)
   ////////CheckCol(m_pOrigDuneLineLayer, m_colShoreface, "SHOREFACE", TYPE_FLOAT, CC_AUTOADD);
   ////////
   ////////// Foreshore Slope (measured in cross-shore profile within +- 0.5m vertically of MHW)
   ////////CheckCol(m_pOrigDuneLineLayer, m_colForeshore, "FORESHORE", TYPE_FLOAT, CC_AUTOADD);
   ////////
   ////////// Index of protected building
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneBldgIndx, "BLDGINDX", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colDuneBldgIndx, VData(-1), true);
   ////////// Distance (m) to protected building
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneBldgDist, "BLDGDIST", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colDuneBldgDist, VData(5000.0), true);
   ////////
   ////////
   ////////// ******   These attributes are calculated during an annual run and represent the annual maximum or average ******
   ////////
   ////////// Yearly Maxmimum Total Water Level (TWL)
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrMaxTWL, "MAX_TWL_YR", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrMaxTWL, VData(0.0f), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrFMaxTWL, "MAXFTWL_YR", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrFMaxTWL, VData(0.0f), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrAvgTWL, "AVG_TWL_YR", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrAvgTWL, VData(0.0f), true);
   ////////
   ////////// Moving Average of TWL over a designated window
   ////////CheckCol(m_pOrigDuneLineLayer, m_colMvAvgTWL, "MVAVG_TWL", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colMvAvgTWL, VData(0.0f), true);
   ////////
   ////////// Maximum TWL within a designated moving window
   ////////CheckCol(m_pOrigDuneLineLayer, m_colMvMaxTWL, "MVMAX_TWL", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colMvMaxTWL, VData(0.0f), true);
   ////////
   ////////// Annual Data corresponding to the wave characteristics which have the yearly maximum TWL
   ////////CheckCol(m_pOrigDuneLineLayer, m_colWPeriod, "WPERIOD", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colWHeight, "WHEIGHT", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colWDirection, "WDIRECTION", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colHdeep, "HDEEP", TYPE_FLOAT, CC_AUTOADD);
   ////////
   ////////// Day of Year (doy) which has the yearly Maximum TWL
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrMaxDoy, "MAXDOY", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrMaxDoy, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrMaxSWL, "MAXSWL", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrMaxSWL, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrAvgSWL, "AVG_SWL_YR", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrAvgSWL, VData(0.0f), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrAvgLowSWL, "AVGLWSWLYR", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrAvgLowSWL, VData(0.0f), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrMaxSetup, "MAXSETUP", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrMaxSetup, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrMaxIGSwash, "MAXIGSWASH", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrMaxIGSwash, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrMaxIncSwash, "MXINCSWASH", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrMaxIncSwash, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colYrMaxSwash, "MAXSWASH", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colYrMaxSwash, VData(0.0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colWb, "WB", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colHb, "HB", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colTS, "TS", TYPE_DOUBLE, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colAlpha, "ALPHA", TYPE_DOUBLE, CC_AUTOADD);
   ////////
   ////////
   ////////// Erosion attributes
   ////////
   ////////// Annual K&D erosion extent
   ////////CheckCol(m_pOrigDuneLineLayer, m_colKD, "EROSION", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colKD, VData(0.0f), true);
   ////////
   ////////// K&D 2-yr average erosion extent
   ////////CheckCol(m_pOrigDuneLineLayer, m_colAvgKD, "AVGKD", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colAvgKD, VData(0.0f), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colXAvgKD, "X_AVGKD", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colXAvgKD, VData(0.0f), true);
   ////////
   ////////
   ////////// 0=Not Flooded, value=Yearly Maximum TWL
   ////////CheckCol(m_pOrigDuneLineLayer, m_colFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colFlooded, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colAmtFlooded, "AMT_FLOOD", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colAmtFlooded, VData(0.0), true);
   ////////
   ////////// Frequency of K&D erosion event or flooded event
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneEroFreq, "ERO_FREQ", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colDuneEroFreq, VData(0.0f), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneFloodFreq, "FLOOD_FREQ", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colDuneFloodFreq, VData(0.0f), true);
   ////////
   ////////
   ////////// ***** Impact Days ***** //
   ////////
   ////////// These attributes are calculated annually and represent the number of days per year the dune is impacted
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDToe, "IDDUNETOE", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDToe, VData(0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDCrest, "IDDUNECR", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDCrest, VData(0), true);
   ////////
   ////////// These attributes are calculated annually and represent the number of days per season the dune toe or dune crest is impacted
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDToeWinter, "IDDTWINTER", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDToeWinter, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDToeSpring, "IDDTSPRING", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDToeSpring, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDToeSummer, "IDDTSUMMER", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDToeSummer, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDToeFall, "IDDTFALL", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDToeFall, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDCrestWinter, "IDDCWINTER", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDCrestWinter, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDCrestSpring, "IDDCSPRING", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDCrestSpring, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDCrestSummer, "IDDCSUMMER", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDCrestSummer, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colIDDCrestFall, "IDDCFALL", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colIDDCrestFall, VData(0), true);
   ////////
   ////////// Moving Average of Impact Days Per Year
   ////////CheckCol(m_pOrigDuneLineLayer, m_colMvAvgIDPY, "MVAVG_IDPY", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colMvAvgIDPY, VData(0.0f), true);
   ////////
   ////////
   ////////// ****** Protection Structures ****** //
   ////////
   ////////// ****** Hard Protection Structures ****** //
   ////////CheckCol(m_pOrigDuneLineLayer, m_colEastingToeBPS, "EASTINGBPS", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colEastingToeBPS, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colAddYearBPS, "ADD_YR_BPS", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colAddYearBPS, VData(0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colMaintYearBPS, "MAIN_YRBPS", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colMaintYearBPS, VData(0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colRemoveYearBPS, "REM_YR_BPS", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colRemoveYearBPS, VData(0), true);
   ////////
   ////////
   ////////// Hard Protection Structure Nourishment attributes
   ////////CheckCol(m_pOrigDuneLineLayer, m_colNourishVolBPS, "NOURVOLBPS", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colNourishYearBPS, "NOURYRBPS", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colNourishYearBPS, VData(0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colNourishFreqBPS, "NOURFRQBPS", TYPE_INT, CC_AUTOADD);
   ////////
   ////////
   ////////// ****** Soft Protection Structures ****** //
   ////////CheckCol(m_pOrigDuneLineLayer, m_colEastingToeSPS, "EASTINGSPS", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colEastingToeSPS, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colAddYearSPS, "ADD_YR_SPS", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colAddYearSPS, VData(0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colMaintYearSPS, "MAIN_YRSPS", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colMaintYearSPS, VData(0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colEastingCrestSPS, "ECRESTSPS", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colEastingCrestSPS, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colDuneToeSPS, "DUNETOESPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colDuneToeSPS, VData(0.0f), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colHeelWidthSPS, "HWIDTHSPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colHeelWidthSPS, VData(0.0f), true);
   ////////
   //////////   CheckCol(m_pOrigDuneLineLayer, m_colEastingCrestSPS, "EASTCRESTSPS", TYPE_DOUBLE, CC_AUTOADD);
   ////////
   ////////// Soft Protection Structure Sand Volume attributes
   ////////CheckCol(m_pOrigDuneLineLayer, m_colConstructVolumeSPS, "BLDVOLSPS", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colConstructVolumeSPS, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colMaintVolumeSPS, "RBLDVOLSPS", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colMaintVolumeSPS, VData(0.0), true);
   ////////
   ////////
   ////////// Soft Protection Structure Nourishment attributes
   ////////CheckCol(m_pOrigDuneLineLayer, m_colNourishVolSPS, "NOURVOLSPS", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colNourishYearSPS, "NOURYRSPS", TYPE_INT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colNourishYearSPS, VData(0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colNourishFreqSPS, "NOURFRQSPS", TYPE_INT, CC_AUTOADD);
   ////////
   ////////// Costs of Protection Structures
   ////////CheckCol(m_pOrigDuneLineLayer, m_colCostBPS, "COST_BPS", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colCostBPS, VData(0.0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colCostMaintBPS, "MNTCOSTBPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colCostMaintBPS, VData(0.0), true);
   ////////
   ////////CheckCol(m_pOrigDuneLineLayer, m_colCostSPS, "COST_SPS", TYPE_DOUBLE, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colCostSPS, VData(0.0), true);
   //////////CheckCol(m_pOrigDuneLineLayer, m_colNoursihCostSPS, "NOURCOSTBPS", TYPE_DOUBLE, CC_AUTOADD);
   //////////m_pOrigDuneLineLayer->SetColData(m_colNourishCostSPS, VData(0.0), true);
   //////////CheckCol(m_pOrigDuneLineLayer, m_colRebuildCostSPS, "RBLDCOSTBPS", TYPE_DOUBLE, CC_AUTOADD);
   //////////m_pOrigDuneLineLayer->SetColData(m_colRebuildCostSPS, VData(0.0), true);
   ////////
   ////////// Height of Protection Structures
   ////////CheckCol(m_pOrigDuneLineLayer, m_colHeightBPS, "HEIGHT_BPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colHeightBPS, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colHeightSPS, "HEIGHT_SPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colHeightSPS, VData(0.0), true);
   ////////
   ////////// Length of Protection Structures
   ////////CheckCol(m_pOrigDuneLineLayer, m_colLengthBPS, "LENGTH_BPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colLengthBPS, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colLengthSPS, "LENGTH_SPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colLengthSPS, VData(0.0), true);
   ////////
   ////////// Width of Protection Structures
   ////////CheckCol(m_pOrigDuneLineLayer, m_colWidthBPS, "WIDTH_BPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colWidthBPS, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colWidthSPS, "WIDTH_SPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colWidthSPS, VData(0.0), true);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colTopWidthSPS, "TOPWDTHSPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colTopWidthSPS, VData(0.0), true);
   ////////
   ////////
   ////////// 1/0 Indication that Beachtype changed 
   ////////// ??? track annually
   ////////CheckCol(m_pOrigDuneLineLayer, m_colTypeChange, "TYPECHANGE", TYPE_INT, CC_AUTOADD);
   ////////
   /////////*CheckCol(m_pOrigDuneLineLayer, m_colDuneWidthSPS, "DWIDTH_SPS", TYPE_FLOAT, CC_AUTOADD);
   ////////m_pOrigDuneLineLayer->SetColData(m_colDuneWidthSPS, VData(0.0), true);*/
   ////////
   ////////// Beach Accessibility attributes, annually and seasonally
   ////////CheckCol(m_pOrigDuneLineLayer, m_colBchAccess, "ACCESS", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colBchAccessWinter, "ACCESS_W", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colBchAccessSpring, "ACCESS_SP", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colBchAccessSummer, "ACCESS_SU", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colBchAccessFall, "ACCESS_F", TYPE_FLOAT, CC_AUTOADD);
   ////////
   ////////// currently unused
   ////////
   /////////*CheckCol(m_pOrigDuneLineLayer, m_colAvgDuneC, "AVG_DUNEC", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colAvgDuneT, "AVG_DUNET", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colAvgEro, "AVG_ERO", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colCPolicyL, "Policy", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colThreshDate, "THR_DATE", TYPE_INT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colCalDCrest, "CAL_DCREST", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colCalDToe, "CAL_DTOE", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colCalEroExt, "CAL_EROEXT", TYPE_FLOAT, CC_AUTOADD);
   ////////CheckCol(m_pOrigDuneLineLayer, m_colCalDate, "CAL_DATE", TYPE_INT, CC_AUTOADD);*/
   //CheckCol(m_pOrigDuneLineLayer, m_colBackshoreElev, "MIN_BACK_E", TYPE_FLOAT, CC_AUTOADD);
   //

   // Bay Trace columns
   if (m_pBayTraceLayer)
      {
      CheckCol(m_pBayTraceLayer, m_colBayTraceIndx, "TRACEINDX", TYPE_INT, CC_MUST_EXIST);
      CheckCol(m_pBayTraceLayer, m_colBayYrMaxTWL, "MAX_TWL_YR", TYPE_FLOAT, CC_AUTOADD);
      m_pBayTraceLayer->SetColData(m_colBayYrMaxTWL, VData(0.0f), true);
      CheckCol(m_pBayTraceLayer, m_colBayYrAvgTWL, "AVG_TWL_YR", TYPE_FLOAT, CC_AUTOADD);
      m_pBayTraceLayer->SetColData(m_colBayYrAvgTWL, VData(0.0f), true);
      CheckCol(m_pBayTraceLayer, m_colBayYrMaxSWL, "MAX_SWL_YR", TYPE_FLOAT, CC_AUTOADD);
      m_pBayTraceLayer->SetColData(m_colBayYrMaxSWL, VData(0.0f), true);
      CheckCol(m_pBayTraceLayer, m_colBayYrAvgSWL, "AVG_SWL_YR", TYPE_FLOAT, CC_AUTOADD);
      m_pBayTraceLayer->SetColData(m_colBayYrAvgSWL, VData(0.0f), true);
      }

   // Road Layer columns
   //if (m_pRoadLayer)
   //   {
   //   CheckCol(m_pRoadLayer, m_colRoadFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
   //   m_pRoadLayer->SetColData(m_colRoadFlooded, VData(0), true);
   //   CheckCol(m_pRoadLayer, m_colRoadType, "TYPE", TYPE_INT, CC_MUST_EXIST);
   //   CheckCol(m_pRoadLayer, m_colRoadLength, "LENGTH", TYPE_DOUBLE, CC_MUST_EXIST);
   //   }
   //
   //// Building layers columns
   //if (m_pBldgLayer)
   //   {
   //   CheckCol(m_pBldgLayer, m_colBldgIDUIndx, "IDUINDX", TYPE_INT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgIDUIndx, VData(-1), true);
   //   CheckCol(m_pBldgLayer, m_colBldgNDU, "NDU", TYPE_INT, CC_AUTOADD);
   //   CheckCol(m_pBldgLayer, m_colBldgNEWDU, "NEWDU", TYPE_INT, CC_AUTOADD);
   //   CheckCol(m_pBldgLayer, m_colBldgBFE, "BFE", TYPE_FLOAT, CC_AUTOADD);
   //   //CheckCol(m_pBldgLayer, m_colBldgFloodHZ, "FLDHZ", TYPE_INT, CC_AUTOADD);
   //   //m_pBldgLayer->SetColData(m_colBldgFloodHZ, VData(0), true);
   //
   //   CheckCol(m_pBldgLayer, m_colBldgFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgFlooded, VData(0), true);
   //   CheckCol(m_pBldgLayer, m_colBldgEroded, "ERODED", TYPE_INT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgEroded, VData(0), true);
   //
   //   CheckCol(m_pBldgLayer, m_colBldgRemoveYr, "REMOVE_YR", TYPE_INT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgRemoveYr, VData(0), true);
   //
   //   CheckCol(m_pBldgLayer, m_colBldgSafeSiteYr, "SFESITE_YR", TYPE_INT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgSafeSiteYr, VData(0), true);
   //
   //   CheckCol(m_pBldgLayer, m_colBldgBFEYr, "BFE_YR", TYPE_INT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgBFEYr, VData(0), true);
   //   CheckCol(m_pBldgLayer, m_colBldgSafeSite, "SAFE_SITE", TYPE_INT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgSafeSite, VData(1), true);
   //   CheckCol(m_pBldgLayer, m_colBldgFloodYr, "FLOOD_YR", TYPE_INT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgFloodYr, VData(0), true);
   //   CheckCol(m_pBldgLayer, m_colBldgEroFreq, "ERO_FREQ", TYPE_FLOAT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgEroFreq, VData(0.0f), true);
   //   CheckCol(m_pBldgLayer, m_colBldgFloodFreq, "FLOOD_FREQ", TYPE_FLOAT, CC_AUTOADD);
   //   m_pBldgLayer->SetColData(m_colBldgFloodFreq, VData(0.0f), true);
   //
   //   CheckCol(m_pBldgLayer, m_colBldgCityCode, "CITY_CODE", TYPE_INT, CC_AUTOADD);
   //   CheckCol(m_pBldgLayer, m_colBldgValue, "BLDGVALUE", TYPE_INT, CC_MUST_EXIST);
   //   }
   //
   //// Infrastructure Layer columns
   //if (m_pInfraLayer)
   //   {
   //   CheckCol(m_pInfraLayer, m_colInfraCritical, "CRITICAL", TYPE_INT, CC_MUST_EXIST);
   //   CheckCol(m_pInfraLayer, m_colInfraDuneIndx, "DUNEINDX", TYPE_INT, CC_AUTOADD);
   //   m_pInfraLayer->SetColData(m_colInfraDuneIndx, VData(-1), true);
   //   CheckCol(m_pInfraLayer, m_colInfraFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
   //   m_pInfraLayer->SetColData(m_colInfraFlooded, VData(0), true);
   //   CheckCol(m_pInfraLayer, m_colInfraEroded, "ERODED", TYPE_INT, CC_AUTOADD);
   //   m_pInfraLayer->SetColData(m_colInfraEroded, VData(0), true);
   //   CheckCol(m_pInfraLayer, m_colInfraBFE, "BFE", TYPE_FLOAT, CC_AUTOADD);
   //   m_pInfraLayer->SetColData(m_colInfraBFE, VData(0.0), true);
   //   CheckCol(m_pInfraLayer, m_colInfraBFEYr, "BFE_YR", TYPE_INT, CC_AUTOADD);
   //   m_pInfraLayer->SetColData(m_colInfraBFEYr, VData(0), true);
   //   CheckCol(m_pInfraLayer, m_colInfraFloodYr, "FLOOD_YR", TYPE_INT, CC_AUTOADD);
   //   m_pInfraLayer->SetColData(m_colInfraFloodYr, VData(0), true);
   //   CheckCol(m_pInfraLayer, m_colInfraValue, "InfraValue", TYPE_FLOAT, CC_MUST_EXIST);
   //
   //   SetInfrastructureValues();
   //
   //   }

   ASSERT(pEnvContext != NULL);

   // define input (scenario) variables
   AddInputVar("Climate Scenario ID", m_climateScenarioID, "0=BaseSLR, 1=LowSLR, 2=MedSLR, 3=HighSLR, 4=WorstCaseSLR");
   AddInputVar("Track Habitat", m_runEelgrassModel, "0=Off, 1=On");

   AddInputVar(_T("Construct BPS"), m_runConstructBPSPolicy, "0=Off, 1=On");
   AddInputVar(_T("Maintain BPS"), m_runMaintainBPSPolicy, "0=Off, 1=On");
   AddInputVar(_T("Nourish By Type"), m_runNourishByTypePolicy, "0=Off, 1=On");

   AddInputVar(_T("Construct SPS"), m_runConstructSPSPolicy, "0=Off, 1=On");
   AddInputVar(_T("Maintain SPS"), m_runMaintainSPSPolicy, "0=Off, 1=On");
   AddInputVar(_T("Nourish SPS"), m_runNourishSPSPolicy, "0=Off, 1=On");

   //AddInputVar(_T("Construct on Safest Site"), m_runConstructSafestPolicy, "0=Off, 1=On");
   AddInputVar(_T("Remove Bldg From Hazard Zone"), m_runRemoveBldgFromHazardZonePolicy, "0=Off, 1=On");
   AddInputVar(_T("Remove Infra From Hazard Zone"), m_runRemoveInfraFromHazardZonePolicy, "0=Off, 1=On");
   AddInputVar(_T("Raise or Relocate to Safest Site"), m_runRelocateSafestPolicy, "0=Off, 1=On");
   AddInputVar(_T("Raise Infrastructure"), m_runRaiseInfrastructurePolicy, "0=Off, 1=On");


   // set up policy information for budgeting
   //m_policyInfoArray.SetSize(PI_COUNT);
   //
   //m_policyInfoArray[PI_CONSTRUCT_BPS].Init(_T("Construct BPS"), (int)PI_CONSTRUCT_BPS, &m_runConstructBPSPolicy, true);
   //m_policyInfoArray[PI_MAINTAIN_BPS].Init(_T("Maintain BPS"), (int)PI_MAINTAIN_BPS, &m_runMaintainBPSPolicy, true);
   //m_policyInfoArray[PI_NOURISH_BY_TYPE].Init(_T("Nourish By Type"), (int)PI_NOURISH_BY_TYPE, &m_runNourishByTypePolicy, true);
   //
   //m_policyInfoArray[PI_CONSTRUCT_SPS].Init(_T("Construct SPS"), (int)PI_CONSTRUCT_SPS, &m_runConstructSPSPolicy, true);
   //m_policyInfoArray[PI_MAINTAIN_SPS].Init(_T("Maintain SPS"), (int)PI_MAINTAIN_SPS, &m_runMaintainSPSPolicy, true);
   //m_policyInfoArray[PI_NOURISH_SPS].Init(_T("Nourish SPS"), (int)PI_NOURISH_SPS, &m_runNourishSPSPolicy, true);
   //
   ////m_policyInfoArray[PI_CONSTRUCT_SAFEST_SITE].Init(_T("Construct on Safest Site"), (int)PI_CONSTRUCT_SAFEST_SITE, &m_runConstructSafestPolicy, false);
   //m_policyInfoArray[PI_REMOVE_BLDG_FROM_HAZARD_ZONE].Init(_T("Remove Bldg From Hazard Zone"), (int)PI_REMOVE_BLDG_FROM_HAZARD_ZONE, &m_runRemoveBldgFromHazardZonePolicy, true);
   //m_policyInfoArray[PI_REMOVE_INFRA_FROM_HAZARD_ZONE].Init(_T("Remove Infra From Hazard Zone"), (int)PI_REMOVE_INFRA_FROM_HAZARD_ZONE, &m_runRemoveInfraFromHazardZonePolicy, true);
   //m_policyInfoArray[PI_RAISE_RELOCATE_TO_SAFEST_SITE].Init(_T("Raise or Relocate to Safest Site"), (int)PI_RAISE_RELOCATE_TO_SAFEST_SITE, &m_runRelocateSafestPolicy, true);
   //m_policyInfoArray[PI_RAISE_INFRASTRUCTURE].Init(_T("Raise Infrastructure"), (int)PI_RAISE_INFRASTRUCTURE, &m_runRaiseInfrastructurePolicy, true);

   // make an output data object to store results
   //int cols = 1 + ((PI_COUNT + 1) * 7);
   //m_policyCostData.SetName("Policy Cost Metrics");
   //m_policyCostData.SetSize(cols, 0);
   //m_policyCostData.SetLabel(0, _T("Time"));
   //int col = 1;
   //for (int i = 0; i <= PI_COUNT; i++)
   //   {
   //   CString base;
   //   if (i < PI_COUNT)
   //      {
   //      PolicyInfo &pi = m_policyInfoArray[i];
   //      base = pi.m_label;
   //      }
   //   else
   //      base = "Total";
   //
   //   CString label;
   //   label = base + ".Starting Budget";
   //   m_policyCostData.SetLabel(col++, (LPCTSTR)label);
   //
   //   label = base + ".Year End Budget";
   //   m_policyCostData.SetLabel(col++, (LPCTSTR)label);
   //
   //   label = base + ".Budget Spent";
   //   m_policyCostData.SetLabel(col++, (LPCTSTR)label);
   //
   //   label = base + ".Unmet Demand";
   //   m_policyCostData.SetLabel(col++, (LPCTSTR)label);
   //
   //   label = base + ".Mov Avg Unmet Demand";
   //   m_policyCostData.SetLabel(col++, (LPCTSTR)label);
   //
   //   label = base + ".Mov Avg Budget Spent";
   //   m_policyCostData.SetLabel(col++, (LPCTSTR)label);
   //
   //   label = base + ".Alloc Frac";
   //   m_policyCostData.SetLabel(col++, (LPCTSTR)label);
   //   }


   m_debugOn = (m_debug == 1) ? true : false;

   ///////CheckCol(m_pOrigDuneLineLayer, m_colDuneBldgEast, "BLDGEAST", TYPE_DOUBLE, CC_AUTOADD);
   ///////CheckCol(m_pOrigDuneLineLayer, m_colDuneBldgIndx2, "BLDGINDX2", TYPE_INT, CC_AUTOADD);
   ///////
   ///////if (m_debugOn)
   ///////   {
   ///////   // columns for debugging Dune line 
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colA, "AN", TYPE_DOUBLE, CC_AUTOADD);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colBWidth, "DELTAX_2", TYPE_DOUBLE, CC_AUTOADD);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colComputedSlope, "CSLOPE", TYPE_DOUBLE, CC_AUTOADD);
   ///////   m_pOrigDuneLineLayer->SetColData(m_colComputedSlope, VData(0.0), true);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colComputedSlope2, "CSLOPENEW", TYPE_DOUBLE, CC_AUTOADD);
   ///////   m_pOrigDuneLineLayer->SetColData(m_colComputedSlope2, VData(0.0), true);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colDeltaX, "DELTAX", TYPE_FLOAT, CC_AUTOADD);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colNewEasting, "NEASTING", TYPE_DOUBLE, CC_AUTOADD);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colNumIDPY, "IDPY_CNT", TYPE_INT, CC_AUTOADD);
   ///////   m_pOrigDuneLineLayer->SetColData(m_colNumIDPY, VData(0), true);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colSTKTWL, "STKTWL", TYPE_FLOAT, CC_AUTOADD);
   ///////   m_pOrigDuneLineLayer->SetColData(m_colSTKTWL, VData(0.0), true);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colRunupFlag, "RUNUPFLAG", TYPE_INT, CC_AUTOADD);
   ///////   m_pOrigDuneLineLayer->SetColData(m_colRunupFlag, VData(0), true);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colTAWSlope, "TAWSLOPE", TYPE_FLOAT, CC_AUTOADD);
   ///////
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colSTKRunup, "STKRunup", TYPE_FLOAT, CC_AUTOADD);
   ///////   m_pOrigDuneLineLayer->SetColData(m_colSTKRunup, VData(0.0), true);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colSTKR2, "STKR2", TYPE_FLOAT, CC_AUTOADD);
   ///////   m_pOrigDuneLineLayer->SetColData(m_colSTKR2, VData(0.0), true);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colStructR2, "StructR2", TYPE_FLOAT, CC_AUTOADD);
   ///////   m_pOrigDuneLineLayer->SetColData(m_colStructR2, VData(0.0), true);
   ///////   CheckCol(m_pOrigDuneLineLayer, m_colTD, "TD", TYPE_FLOAT, CC_AUTOADD);
   ///////   }

   CString filename;
   CString fullFilename = PathManager::GetPath(PM_PROJECT_DIR);

   if (m_cityDir.IsEmpty())
      fullFilename += "PolyGridMaps\\";
   else
      fullFilename += m_cityDir + "\\PolyGridMaps\\";

   CString msg("Looking for polygrid lookups in ");
   msg += fullFilename;
   Report::Log(msg);

   // Load from file if found, otherwise create PolyPointMap for IDUs 
   ///////////CString iduBldgFile = fullFilename + "IDUBuildings.ppm";
   ///////////m_pIduBuildingLkup = new PolyPointMapper(m_pIDULayer, m_pBldgLayer, iduBldgFile);
   /////////////   m_pIduBuildingLkup->BuildIndex();
   ///////////
   ///////////// Load from file if found, otherwise create PolyPointMap for IDUs 
   ///////////CString iduInfraFile = fullFilename + "IDUInfrastructure.ppm";
   ///////////m_pPolygonInfraPointLkup = new PolyPointMapper(m_pIDULayer, m_pInfraLayer, iduInfraFile);
   /////////////m_pPolygonInfraPointLkup->BuildIndex();
   ///////////
   /////////////  Load from file if found, otherwise create PolyGridMap for IDUs based upon cell sizes of the Elevation Layer
   ///////////CString IDUPglFile = fullFilename + "IDUGridLkup.pgm";
   ///////////m_pPolygonGridLkUp = new PolyGridMapper(m_pIDULayer, m_pElevationGrid, 1, IDUPglFile);
   ///////////// Check built lookup matches grid 
   ///////////if ((m_pPolygonGridLkUp->GetNumGridCols() != m_numCols) || (m_pPolygonGridLkUp->GetNumGridRows() != m_numRows))
   ///////////   Report::ErrorMsg("ChronicHazards: IDU Grid Lookup row or column mismatch, rebuild lookup");
   ///////////else if (m_pPolygonGridLkUp->GetNumPolys() != m_pIDULayer->GetPolygonCount())
   ///////////   Report::ErrorMsg("ChronicHazards: IDU Grid Lookup IDU count mismatch, rebuild lookup");
   ///////////
   ///////////// Load from file if found, otherwise create PolyGridMap for Roads based upon cell sizes of the Elevation Layer
   ///////////CString roadsPglFile = fullFilename + "RoadGridLkup.pgm";
   ///////////m_pPolylineGridLkUp = new PolyGridMapper(m_pRoadLayer, m_pElevationGrid, 1, roadsPglFile);
   ///////////// Check built lookup matches grid
   ///////////if ((m_pPolylineGridLkUp->GetNumGridCols() != m_numCols) || (m_pPolylineGridLkUp->GetNumGridRows() != m_numRows))
   ///////////   Report::ErrorMsg("ChronicHazards: Road Grid Lookup row or column mismatch, rebuild lookup");
   ////////////*else if (m_pPolylineGridLkUp->GetNumPolys() != m_pRoadLayer->GetPolygonCount())
   ///////////Report::ErrorMsg("ChronicHazards: Road Grid Lookup road count mismatch, rebuild lookup");*/

   // Load Northing SWAN Lookup
   CString iduPath = PathManager::GetPath(PM_IDU_DIR);
   CString fullPath = iduPath + "TWL INPUTS\\" + m_transectLookupFile;
   m_pTransectLUT = new DDataObj;
   m_numTransects = m_pTransectLUT->ReadAscii(fullPath);

   // Load Northing Profile Lookup (includes shoreface slope lookup for each profile)
   //PathManager::FindPath("ProfileLookup.csv", fullPath);
   //////fullPath = iduPath + "TWL INPUTS\transect_lookup.csv";
   //////m_pProfileLUT = new DDataObj;
   //////m_numProfiles = m_pProfileLUT->ReadAscii(fullPath);

   // Load Shoreline Angle Lookup
   //////////PathManager::FindPath("ShorelineAngleLookup.csv", fullPath);
   //////////m_pSAngleLUT = new DDataObj;
   //////////int numAngles = m_pSAngleLUT->ReadAscii(fullPath);
   //////////int colSA = m_pSAngleLUT->GetCol("Shoreline Angle");
   //////////
   //////////PathManager::FindPath("BayLookup.csv", fullPath);
   //////////m_pBayLUT = new DDataObj;
   //////////m_numBayStations = m_pBayLUT->ReadAscii(fullPath);

   /* int colNorthing = m_pProfileLUT->GetCol("Northing");
   int colTransectNum = m_pProfileLUT->GetCol("Profile Number");*/

   /*  int colNorthing = m_pTransectLUT->GetCol("Northing");
   int colTransectNum = m_pTransectLUT->GetCol("Transect Number"); */

   //   int colBruunSlope = m_pProfileLUT->GetCol("Bruun Slope");
   //   int colForeshoreSlope = m_pProfileLUT->GetCol("Foreshore Slope");

   // Associate SWAN transects and cross shore profiles to duneline by NORTHING value
   // Initialize previous beachtype
   if (m_pOrigDuneLineLayer != NULL)
      {
      for (MapLayer::Iterator point = m_pOrigDuneLineLayer->Begin(); point < m_pOrigDuneLineLayer->End(); point++)
         {
         REAL xCoord = 0.0;
         REAL yCoord = 0.0;

         m_pOrigDuneLineLayer->GetPointCoords(point, xCoord, yCoord);

         int beachType = -1;
         m_pOrigDuneLineLayer->GetData(point, m_colBeachType, beachType);

         // Determine SWAN Lookup transect index
         int rowIndex = -1;

         double tIndex = IGet(m_pTransectLUT, yCoord, 0, 1, IM_LINEAR, 0, &rowIndex);
         int transectIndex = m_pTransectLUT->GetAsInt(1, rowIndex);
         /*float index = m_pTransectLUT->IGet(yCoord, 1, IM_LINEAR);
         int transectIndex = (int)round(index);*/

         if (transectIndex < m_minTransect)
            m_minTransect = transectIndex;

         if (transectIndex > m_maxTransect)
            m_maxTransect = transectIndex;

         //m_pOrigDuneLineLayer->SetData(point, m_colTranIndx, transectIndex);

         // Determine ShorelineAngle 
         //   double shorelineAngle = m_pSAngleLUT->IGet((double)yCoord, 2, IM_LINEAR);
         //   m_pOrigDuneLineLayer->SetData(point, m_colShorelineAngle, shorelineAngle);

         int duneIndx = -1;
         m_pOrigDuneLineLayer->GetData(point, m_colDuneIndx, duneIndx);
         //index = -1;
         // Determine beach profile index
         //index = m_pProfileLUT->IGet(yCoord, 1, IM_LINEAR);

         //   rowIndex = -1;
         //   IGet(m_pProfileLUT, yCoord, 0, 1, IM_LINEAR, 0, &rowIndex); 
         //   int profileIndex = m_pProfileLUT->GetAsInt(1, rowIndex);

         // merge datasets using dune shape and up to date elevations
         /*   float duneToe = 0.0;
         int row = -1;
         int col = -1;
         m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, row, col);
         m_pElevationGrid->GetData(row, col, duneToe);
         m_pOrigDuneLineLayer->SetData(point, m_colDuneToe, duneToe);*/

         float duneCrest = 0.0;

         /*REAL xCoordCr = 0.0;
         m_pOrigDuneLineLayer->GetData(point, m_colEastingCrest, xCoordCr);
         m_pElevationGrid->GetGridCellFromCoord(xCoordCr, yCoord, row, col);
         m_pElevationGrid->GetData(row, col, duneCrest);
         m_pOrigDuneLineLayer->SetData(point, m_colDuneCrest, duneCrest);*/

         //   m_pOrigDuneLineLayer->SetData(point, m_colProfileIndx, profileIndex);
         //   m_pOrigDuneLineLayer->SetData(point, m_colShoreface, bruunRuleSlope);

         //   float foreshoreSlope = m_pProfileLUT->GetAsFloat(colForeshoreSlope, profileIndex);
         //   m_pOrigDuneLineLayer->SetData(point, m_colForeshore, foreshoreSlope);


         // find Maximum dune line index for display purposes
         if (duneIndx > m_maxDuneIndx)
            m_maxDuneIndx = duneIndx;

         //      float duneCrest = 0.0f;
         m_pOrigDuneLineLayer->GetData(point, m_colDuneCrest, duneCrest);
         m_pOrigDuneLineLayer->SetData(point, m_colPrevDuneCr, duneCrest);

         int startRow = -1;
         int startCol = -1;
         m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

         m_pOrigDuneLineLayer->SetData(point, m_colPrevRow, startRow);
         m_pOrigDuneLineLayer->SetData(point, m_colPrevCol, startCol);

         // Initialize original beachtype with existing beachtype
         beachType = 0;
         m_pOrigDuneLineLayer->GetData(point, m_colBeachType, beachType);
         m_pOrigDuneLineLayer->SetData(point, m_colOrigBeachType, beachType);

         int loc = 0;
         m_pOrigDuneLineLayer->GetData(point, m_colLittCell, loc);
         if (loc == 1)
            {
            //m_numJettyPts++;
            //m_numOceanShoresPts++;

            }
         if (loc == 2)
            {
            //m_numJettyPts++;
            //m_numWestportPts++;
            }


         // Convert NORTHING and EASTING to LONGITUDE and LATITUDE for map csv display
         CalculateUTM2LL(m_pOrigDuneLineLayer, point);

         } // end duneline points
      }  // end of: if ( m_pOrigDuneLineLayer )

   // Read in the Bay coverage if running Bay Flooding Model
   /*bool runBayFloodModel = (m_runBayFloodModel == 1) ? true : false;
   if (runBayFloodModel)
      {
      CString bayTraceFile = NULL;
      if (CString(m_cityDir).IsEmpty())
      bayTraceFile = "BayPts.csv";
      else
      bayTraceFile = "BayPts_" + m_cityDir + ".csv";

      PathManager::FindPath(bayTraceFile, fullPath);
      m_pInletLUT = new DDataObj;
      m_numBayPts = m_pInletLUT->ReadAscii(fullPath);
      }*/

   // Associate IDU <-> Duneline indexing
   if (m_pOrigDuneLineLayer != NULL)
      {
      int polyCount = m_pIDULayer->GetRowCount();
      for (int idu = 0; idu < polyCount; idu++)
         {
         Poly* pPoly = m_pIDULayer->GetPolygon(idu);
         /* float northing = pPoly->m_yMax;*/
         Vertex centroid = pPoly->GetCentroid();
         REAL northing = centroid.y;
         float minDiff = 9999.0F;
         int duneIndx = -1;

         for (MapLayer::Iterator point = m_pOrigDuneLineLayer->Begin(); point < m_pOrigDuneLineLayer->End(); point++)
            {
            REAL xCoord = 0.0;
            REAL yCoord = 0.0;

            m_pOrigDuneLineLayer->GetPointCoords(point, xCoord, yCoord);
            float tmpDiff = float(abs(yCoord - northing));

            if (tmpDiff < minDiff)
               {
               minDiff = tmpDiff;
               m_pOrigDuneLineLayer->GetData(point, m_colDuneIndx, duneIndx);
               }
            }
         m_pIDULayer->SetData(idu, m_colBeachfront, duneIndx);
         }  // end of: for ( idu < polyCount )
      }
   // New Dune line layer starts off as copy of Original with changes of Label and Color
   if (m_pOrigDuneLineLayer != NULL)
      {
      m_pNewDuneLineLayer = m_pMap->CloneLayer(*m_pOrigDuneLineLayer);
      m_pNewDuneLineLayer->m_name = "Active Dune Toe Line";

      // Change the color of the active dune line
      int colIndex = m_pNewDuneLineLayer->GetFieldCol("DUNEINDX");

      // Rebin and classify for display
      m_pNewDuneLineLayer->AddBin(colIndex, RGB(179, 89, 0), "All Points", TYPE_INT, 0, (float)m_maxDuneIndx + 1);
      m_pNewDuneLineLayer->AddNoDataBin(colIndex);
      m_pNewDuneLineLayer->ClassifyData();
      }

   // Setup outputs for annual statistics
   this->AddOutputVar("Mean TWL (m)", m_meanTWL, "");

   this->AddOutputVar("Number of BPS Construction Projects", m_noConstructedBPS, "");
   this->AddOutputVar("BPS Construction Cost ($)", m_constructCostBPS, "");
   this->AddOutputVar("Total Length of Shoreline Hardened (m)", m_totalHardenedShoreline, "");
   this->AddOutputVar("Total Length of Shoreline Hardened (miles)", m_totalHardenedShorelineMiles, "");
   this->AddOutputVar("Added Length of Shoreline Hardened (m)", m_addedHardenedShoreline, "");
   this->AddOutputVar("Added Length of Shoreline Hardened (miles)", m_addedHardenedShorelineMiles, "");
   this->AddOutputVar("Percent of Shoreline Hardened (m)", m_percentHardenedShoreline, "");
   this->AddOutputVar("Percent of Shoreline Hardened (miles)", m_percentHardenedShorelineMiles, "");
   this->AddOutputVar("BPS Maintenance Cost ($)", m_maintCostBPS, "");
   this->AddOutputVar("BPS Nourishment Cost ($)", m_nourishCostBPS, "");
   this->AddOutputVar("BPS Nourishment Volume (m3)", m_nourishVolumeBPS, "");

   this->AddOutputVar("Number of SPS Construction Projects", m_noConstructedDune, "");
   this->AddOutputVar("SPS Construction Cost ($)", m_restoreCostSPS, "");
   this->AddOutputVar("SPS Construction Volume of Sand (m3)", m_constructVolumeSPS, "");
   this->AddOutputVar("SPS Maintenance Cost ($)", m_maintCostSPS, "");
   this->AddOutputVar("SPS Maintenance Volume (m3)", m_maintVolumeSPS, "");
   this->AddOutputVar("SPS Nourishment Cost ($)", m_nourishCostSPS, "");
   this->AddOutputVar("SPS Nourishment Volume (m3)", m_nourishVolumeSPS, "");
   this->AddOutputVar("Total Length of SPS Constructed (m)", m_totalRestoredShoreline, "");
   this->AddOutputVar("Total Length of SPS Constructed (miles)", m_totalRestoredShorelineMiles, "");
   this->AddOutputVar("Added Length of SPS Constructed (m)", m_addedRestoredShoreline, "");
   this->AddOutputVar("Added Length of SPS Constructed (miles)", m_addedRestoredShorelineMiles, "");
   this->AddOutputVar("Percent of SPS Constructed (m)", m_percentRestoredShoreline, "");
   this->AddOutputVar("Percent of SPS Constructed (miles)", m_percentRestoredShorelineMiles, "");

   this->AddOutputVar("Number of Buildings Removed From Hazard Zone (100-yr)", m_noBldgsRemovedFromHazardZone, "");

   this->AddOutputVar("% Beach Accessibility (County Wide)", m_avgCountyAccess, "");
   //   this->AddOutputVar("Number of Buildings Eroded", m_noBldgsEroded, "");

   if (m_runFlags && CH_MODEL_FLOODING)
      {
      this->AddOutputVar("Flooded Area (sq meter)", m_floodedArea, "");
      this->AddOutputVar("Flooded Area (sq miles)", m_floodedAreaSqMiles, "");
      this->AddOutputVar("Flooded Road (m)", m_floodedRoad, "");
      this->AddOutputVar("Flooded Road (miles)", m_floodedRoadMiles, "");
      this->AddOutputVar("Flooded Railroad (miles)", m_floodedRailroadMiles, "");
      }

   if (m_runFlags && CH_MODEL_EROSION)
      {
      this->AddOutputVar("Eroded Road (m)", m_erodedRoad, "");
      this->AddOutputVar("Eroded Road (miles)", m_erodedRoadMiles, "");
      }


   // Flooded and Eroded Buildings counted in Reporter
   /*  this->AddOutputVar("Flooded Buildings", m_floodedBldgCount, "");
   this->AddOutputVar("Eroded Buildings", m_erodedBldgCount, "");*/

   this->AddOutputVar("Policy Cost Allocation Frac", PolicyInfo::m_budgetAllocFrac, "");
   this->AddOutputVar("Policy Cost Summary", &m_policyCostData, "");

   if ( m_runFlags & CH_MODEL_TWL )
      LoadRBFs(); // Load/create radial basis functions for TWL model

   if ( m_runFlags & CH_MODEL_EROSION)
      {
   // Read in the erosion and EVT tables
      for (int i = 0; i < m_numProfiles; i++)
         {
         CString path(PathManager::GetPath(PM_PROJECT_DIR));

         DDataObj *pBathyData = new DDataObj;

         bool success = true;

         // first read in bathy (cross-shore profiles) files

         CString bathyPath = path + "Erosion Inputs\\bathy\\";
         CString bathyFile = m_bathyFiles;
         CString extension;
         extension.Format("_%i.csv", i);
         bathyFile += extension;
         PathManager::FindPath(bathyFile, fullPath);

         CString msg("ChronicHazards: processing bathy input file ");
         msg += bathyFile;
         //     Report::StatusMsg(msg);

         // series of Bathymetry lookup tables
         int numBathyRows = pBathyData->ReadAscii(bathyFile);
         if (numBathyRows < 0)
            success = false;

         if (success)
            this->m_BathyDataArray.Add(pBathyData);
         else
            delete pBathyData;

         // read in surf zone width profiles

         //  DDataObj *pSurfZoneData = new DDataObj;

         //  success = true;

         //  CString szPath = path + "Erosion Inputs\\surfzone\\";
         //  CString surfZoneFile = m_surfZoneFiles;
         //
         //  surfZoneFile += extension;
         //  PathManager::FindPath(surfZoneFile, fullPath);

         //  msg = "ChronicHazards: processing surf zone width input file ";
         //  msg += surfZoneFile;
         ////  Report::Log(msg);

         //  //need to figure out where to put data
         //  int surfZoneRows = pSurfZoneData->ReadAscii(surfZoneFile); //series of Surf Zone Width lookup tables
         //  if (surfZoneRows < 0)
         //     success = false;

         //     if (success)
         //        this->m_SurfZoneDataArray.Add(pSurfZoneData);
         //     else
         //        delete pSurfZoneData;

         }  // end, we have read in all files required for erosion models
           
      //CString msg;
      msg.Format("ChronicHazards: Processed %i cross shore profiles", (int)m_BathyDataArray.GetSize());
      Report::Log(msg);

      /*this->m_twlTime = 0;
      this->m_floodingTime = 0;
      this->m_erosionTime = 0;*/

      //srand(time(NULL));

      // Initialize Moving Window Dune line statistics data structures
      // Each dune pt has a its own set of moving windows
      for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
         {
         MovingWindow* ipdyMovingWindow = new MovingWindow(m_windowLengthTWL);
         m_IDPYArray.Add(ipdyMovingWindow);

         MovingWindow* twlMovingWindow = new MovingWindow(m_windowLengthTWL);
         m_TWLArray.Add(twlMovingWindow);

         MovingWindow* eroKDMovingWindow = new MovingWindow(2);
         m_eroKDArray.Add(eroKDMovingWindow);
         }
      }

   if (m_findSafeSiteCell)
      {
      for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
         {
         /*MovingWindow *eroFreqMovingWindow = new MovingWindow(m_windowLengthEroHzrd);
         m_eroFreqArray.Add(eroFreqMovingWindow);

         MovingWindow *floodFreqMovingWindow = new MovingWindow(m_windowLengthFloodHzrd);
         m_floodFreqArray.Add(floodFreqMovingWindow);*/

         // Find the highest elevation in the IDU
         // using the DEM clipped to the Tsunami Inundation Layer and IDU lookup
         float maxElevation = 0.0f;
         int safestSiteRow = -1;
         int safestSiteCol = -1;

         int numCells = m_pPolygonGridLkUp->GetGridPtCntForPoly(idu);

         if (numCells != 0)
            {
            ROW_COL *indices = new ROW_COL[numCells];
            float *values = new float[numCells];

            m_pPolygonGridLkUp->GetGridPtNdxsForPoly(idu, indices);

            for (int i = 0; i < numCells; i++)
               {
               float value = 0.0f;
               m_pElevationGrid->GetData(indices[i].row, indices[i].col, value);
               if (value > maxElevation)
                  {
                  maxElevation = value;
                  safestSiteRow = indices[i].row;
                  safestSiteCol = indices[i].col;
                  }
               }
            delete[] indices;
            delete[] values;
            }

         m_pIDULayer->SetData(idu, m_colMaxElevation, maxElevation);
         m_pIDULayer->SetData(idu, m_colSafestSiteRow, safestSiteRow);
         m_pIDULayer->SetData(idu, m_colSafestSiteCol, safestSiteCol);
         }
      }

   // Associate Moving Windows for Erosion and Flooding Frequency to a building
   // Set year of safest site for buildings equal the start year of the run
   if (m_runFlags & CH_MODEL_FLOODING)
      {
      for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
         {
         m_floodBldgFreqArray.Add(new MovingWindow(m_windowLengthFloodHzrd));
         m_floodInfraFreqArray.Add(new MovingWindow(m_windowLengthFloodHzrd));
         // m_pBldgLayer->SetData(bldg, m_colBldgSafeSiteYr, pEnvContext->startYear);
         }
      }

   if (m_runFlags & CH_MODEL_EROSION)
      {
      // Associate Moving Windows for Erosion and Flooding frequency to critical infrastructure layer
      for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
         {
         m_eroInfraFreqArray.Add(new MovingWindow(m_windowLengthEroHzrd));
         m_eroBldgFreqArray.Add(new MovingWindow(m_windowLengthEroHzrd));
         }
      }

   //  // ********************************************************************************
   //  // this code should be put into InitRun when implement choosing "random" simulaiton
   //
   //  //   CString fullPath;
   //  CString buoyFile = m_datafile.timeseries_filename;
   //  CString extension;

   //  /* if(m_randomize)
   //  {
   //  extension.Format("_%i.csv", m_randIndex);
   //  buoyFile += extension;
   //  }*/

   //  PathManager::FindPath(buoyFile, fullPath);

   //  // Read daily buoy observations/simulations for the entire Envision time period
   //  m_days = m_buoyObsData.ReadAscii(fullPath);
   //  int cols = -1;

   //  if (m_days <= 0)
   //  {
   //     CString msg;
   //     msg.Format("ChronicHazards: InitRun could not load buoy csv file \n");
   //     Report::InfoMsg(msg);
   //     return false;
   //  }
   //  else
   //  {
   //     m_years = m_days / 365;
   //     CString msg;
   //     msg.Format("ChronicHazards: The buoy data %s contains %i years of daily data", buoyFile, m_years);
   //     Report::Log(msg);
   //     /*  Report::Log(buoyFile);
   //     cols = m_buoyObsData.GetColCount();*/
   //  }

   //  //bool writeDailyBouyData = (m_writeDailyBouyData == 1) ? true : false;

   //  // if randomize, reread in
   ////  m_RBFDailyDataArray.Clear();

   //  const int outputCount = 8;
   //  double outputs[outputCount];
   //  
   //  CString path(PathManager::GetPath(PM_PROJECT_DIR));
   // 
   //  if (writeDailyBouyData)
   //  {
   //     m_minTransect = 0;
   //     m_maxTransect = m_numTransects - 1;
   //  }

   //  for (int i = m_minTransect; i <= m_maxTransect; i++)
   //  {
   //     bool success = true;
   //     FDataObj *pRBFDaily = new FDataObj;
   //     pRBFDaily->SetSize(outputCount, m_days);
   //     //   extension.Format("_%i", m_randIndex);

   //     CString folder = m_datafile.timeseries_file + extension + "\\";

   //     //PathManager::FindPath(folder, path);
   //     folder = path + folder;

   //     CString file;
   //     file.Format("DailyData_%i.csv", i);     //series of SWAN lookup tables
   //     folder += file;

   //     // generate DailyData_%i.csv files using the rbfs and the buoy (simulation) data timeseries
   //     if (writeDailyBouyData)
   //     {
   //        LookupTable *pLUT = m_swanLookupTableArray.GetAt(i - m_minTransect);   // zero-based

   //        pRBFDaily->SetLabel(0, "Height_L");
   //        pRBFDaily->SetLabel(1, "Height_H");
   //        pRBFDaily->SetLabel(2, "Period_L");
   //        pRBFDaily->SetLabel(3, "Period_H");
   //        pRBFDaily->SetLabel(4, "Direction_L");
   //        pRBFDaily->SetLabel(5, "Direction_H");
   //        pRBFDaily->SetLabel(6, "Depth_L");
   //        pRBFDaily->SetLabel(7, "Depth_H");

   //        // read in the whole simulation time period
   //        for (int row = 0; row < m_days; row++)
   //        {
   //           float height, period, direction, swl;

   //           m_buoyObsData.Get(0, row, height);
   //           m_buoyObsData.Get(1, row, period);
   //           m_buoyObsData.Get(2, row, direction);
   //           m_buoyObsData.Get(3, row, swl);

   //           int count = pLUT->m_rbf.GetAt(height, period, direction, outputs);

   //           //series of Bathymetry lookup tables
   //           pRBFDaily->Set(0, row, (float)outputs[HEIGHT_L]);
   //           pRBFDaily->Set(1, row, (float)outputs[HEIGHT_H]);
   //           pRBFDaily->Set(2, row, (float)outputs[PERIOD_L]);
   //           pRBFDaily->Set(3, row, (float)outputs[PERIOD_H]);
   //           pRBFDaily->Set(4, row, (float)outputs[DIRECTION_L]);
   //           pRBFDaily->Set(5, row, (float)outputs[DIRECTION_H]);
   //           pRBFDaily->Set(6, row, (float)outputs[DEPTH_L]);
   //           pRBFDaily->Set(7, row, (float)outputs[DEPTH_H]);
   //        }

   //        pRBFDaily->WriteAscii(folder);

   //        CString msg("ChronicHazards: processing input file ");
   //        msg += folder;
   //        Report::Log(msg);

   //        delete pLUT;

   //     }
   //     else
   //     {
   //       int rowcount = pRBFDaily->ReadAscii(folder);

   //        if (rowcount < 0)
   //           success = false;
   //        /*  else
   //        {
   //        CString msg("ChronicHazards: processing daily data input file ");
   //        msg += folder;
   //        Report::Log(msg);
   //        }*/
   //     }

   //     if (success)
   //        this->m_RBFDailyDataArray.Add(pRBFDaily);
   //     else
   //        delete pRBFDaily;
   //  }

   //  //Set all values to zero
   //  //m_BPSMaintenance = 0;
   //  //m_armorCost = 0;
   //  //m_nourishCostBPS = 0;
   //  //m_nourishVolumeBPS = 0;

   //  msg.Format("ChronicHazards: Processed DailyData input files for %i transects", (int)m_RBFDailyDataArray.GetSize());
   //  Report::Log(msg);
   //  //m_randIndex += 1; // add to xml, only want when we are running probabalistically


   // Read in the Bay coverage if running Bay Flooding Model
   /*bool runBayFloodModel = (m_runBayFloodModel == 1) ? true : false;
   if (runBayFloodModel)
   {
   PathManager::FindPath("BayPts.csv", fullPath);
   m_pInletLUT = new DDataObj;
   m_numBayPts = m_pInletLUT->ReadAscii(fullPath);
   }*/

   //FindClosestDunePtToBldg(pEnvContext);
   if (m_runFlags & CH_MODEL_FLOODING || m_runFlags & CH_MODEL_EROSION)
      {
      FindProtectedBldgs();
      }

   return TRUE;
   }


bool ChronicHazards::LoadRBFs()
   {
   FILE* fpSave = NULL;
   CString iduPath(PathManager::GetPath(PM_IDU_DIR));

   CString savePath = m_twlInputDir + "SWAN_RBF_Params.rbf";
   
   m_randIndex = m_simulationCount;

   bool writeRBFfile = (m_writeRBF > 0) ? true : false;

   // Parameterizing the RBF's from data and save to disk
   if (writeRBFfile)
      {
      if (fopen_s(&fpSave, savePath, "wb") != 0)
         {
         Report::ErrorMsg("ChronicHazards: Unable to open RBF serialization file for writing");
         return false;
         }
      }
   else // are we loading from disk?
      {
      if (fopen_s(&fpSave, savePath, "rb") != 0)
         {
         Report::ErrorMsg("ChronicHazards: Unable to open RBF serialization file for reading");
         return false;
         }
      }

   bool writeDailyBouyData = (m_writeDailyBouyData == 1) ? true : false;

   if (writeRBFfile || writeDailyBouyData)
      {
      m_minTransect = 0;
      m_maxTransect = m_numTransects - 1;
      }

   // Either read in the rbf file or generate the rbf file
   for (int i = m_minTransect; i <= m_maxTransect; i++)
      {
      int ypNum = m_pTransectLUT->GetAsInt(0, i);

      // create a lookup table
      LookupTable* pLookupTable = new LookupTable;

      bool success = true;

      // generate RBFs from data files
      if (writeRBFfile)
         {
         CString path(iduPath);
         path += "TWL Inputs\\Lookup Tables\\";

         CString file;

         file.Format("SWAN_lookup_Yp_%04i.csv", ypNum); // i);  //series of SWAN lookup tables (note ONE-based)
         path += file;

         CString msg("Chronic Hazards: Generating RBF parameters for input file ");
         msg += file;
         Report::StatusMsg(msg);

         int rows = pLookupTable->m_data.ReadAscii(path);//series of SWAN lookup tables
         if (rows < 0)
            success = false;

         // create RBF
         pLookupTable->m_rbf.Create(8);  // number of outputs
         pLookupTable->m_rbf.SetML(30, 5, 0.005);   // radius, layers, lambdaV , add to 

                                                    // set the data into the RBF
         pLookupTable->m_rbf.SetPoints(pLookupTable->m_data);  // rows are input points, col0=x, col1=y, col2=z, col3=f0, col4=f1 etc...

         // build the RBF
         pLookupTable->m_rbf.Build();

         std::string str;
         pLookupTable->m_rbf.Save(str);

         int len = (int)str.length();
         //CString m;
         //m.Format("... Success (RBF size: %i)", len);
         //Report::Log(m, RA_APPEND);

         fwrite((void*)&len, sizeof(int), 1, fpSave);
         fwrite((void*)str.c_str(), sizeof(TCHAR), len, fpSave);
         }
      else
         {
         // read the first int, it has the size of the serialization string
         int length;
         size_t count = fread(&length, sizeof(int), 1, fpSave);

         if (count != 1)
            Report::ErrorMsg("ChronicHazards: Error reading RBF parameter header");

         TCHAR* buffer = new TCHAR[length];
         count = fread(buffer, sizeof(TCHAR), length, fpSave);

         if (int(count) != length)
            Report::ErrorMsg("ChronicHazards: Error reading RBF parameter");

         pLookupTable->m_rbf.Load(buffer);

         int outputs = pLookupTable->m_rbf.GetOutputCount();
         ASSERT(outputs == 8);

         delete[] buffer;
         }

      if (success)
         this->m_swanLookupTableArray.Add(pLookupTable);
      else
         delete pLookupTable;
      }  // end of for ( i < numTransects )

   fclose(fpSave);

   CString msg;
   msg.Format("ChronicHazards: Processed %i SWAN input files", (int)m_swanLookupTableArray.GetSize());
   Report::LogInfo(msg);

   return true;
   }

bool ChronicHazards::ReadDailyBayData(CString timeSeriesFolder)
   {
   // if randomize, reread in
   //  m_DailyBayDataArray.Clear();

   for (int i = 0; i < m_numBayStations; i++)
      {
      bool success = true;
      FDataObj *pBayDaily = new FDataObj;
      pBayDaily->SetSize(1, m_numDays);

      CString timeSeriesFile;
      timeSeriesFile.Format("%s\\DailyBayData_%i.csv", timeSeriesFolder, i);

      int numRows = pBayDaily->ReadAscii(timeSeriesFile);

      if (numRows < 0)
         success = false;
      /*  else
      {
      CString msg("ChronicHazards: processing daily bay data input file ");
      msg += folder;
      Report::Log(msg);
      }*/

      if (success)
         this->m_DailyBayDataArray.Add(pBayDaily);
      else
         delete pBayDaily;
      }

   CString msg;
   msg.Format("ChronicHazards: Processed DailyBayData input files for %i locations", (int)m_DailyBayDataArray.GetSize());
   Report::Log(msg);

   return true;
   }


int ChronicHazards::SetInfrastructureValues()
   {
   if (m_pBldgLayer == NULL || m_pInfraLayer == NULL)
      return -1;

   for (int i = 0; i < m_pInfraLayer->GetRecordCount(); i++)
      {
      Poly *pPolyI = m_pInfraLayer->GetPolygon(i);

      float closestDist = 100000000;
      int closestBldg = -1;

      for (int j = 0; j < m_pBldgLayer->GetRecordCount(); j++)
         {
         Poly *pPolyB = m_pBldgLayer->GetPolygon(j);

         float d = ::DistancePtToPt(pPolyI->GetVertex(0), pPolyB->GetVertex(0));

         if (d < closestDist && d < 20)
            {
            closestDist = d;
            closestBldg = j;
            }
         }

      if (closestBldg >= 0)
         {
         float value;
         m_pBldgLayer->GetData(closestBldg, m_colBldgValue, value);
         m_pInfraLayer->SetData(i, m_colInfraValue, value);
         }
      }

   return 1;
   }

// LEGACY :: Not called 
int ChronicHazards::ReducebuoyObsData(int period)
   {
   // This method takes full buoyObsData and reduces to monthly max values.
   // We scan through the rows of hourly buoy data, collecting max values for 
   // each month and and writing those to the reducedData dataobj as monthly maxs.
   //
   // NOTE: We assume 12 30-day months = 1 year, so we only read the first
   //       360 days worth of hourly data from the buoyObsData dataobj.
   // NOTE: reducedData must have the same # cols as the buoy data
   const int hoursPerMonth = 24 * 30;  // 24 hrs * 30 days

   int hoursPerPeriod = 0;
   switch (period)
      {
      case PERIOD_HOURLY:     hoursPerPeriod = 1;     break;
      case PERIOD_MONTHLY:    hoursPerPeriod = hoursPerMonth;  break;
      case PERIOD_ANNUAL:     hoursPerPeriod = hoursPerMonth * 12;  break;
      default:
         ASSERT(0);
         return 0;
      }

   int cols = m_buoyObsData.GetColCount();
   ASSERT(cols == m_reducedbuoyObsData.GetColCount());

   m_reducedbuoyObsData.ClearRows();

   int rows = 24 * 360;  // rows of observed data, equal hours per year, assumes 12 30-day months
   ASSERT(rows <= m_buoyObsData.GetRowCount());

   double *data = new double[cols];
   ::memset((void*) data, 0, cols * sizeof(double));

   // Iterate through the hourly buoy observations
   for (int h = 0; h < rows; h++)
      {
      // get the obs and compare to current maximums
      //for ( int j=0; j < cols; j++ )
      //   {
      //   float value = m_buoyObsData.GetAsFloat( j, h );
      //   if ( value > data[ j ] )
      //      data[ j ] = value;
      //   }

      // col 0 - Height (m)
      float height = m_buoyObsData.GetAsFloat(0, h);
      if (height > data[0])
         data[0] = height;

      // col 1 - period  (use avg???)
      float period = m_buoyObsData.GetAsFloat(1, h);
      data[1] += period;

      // col 2 - direction  (use avg???)
      float dir = m_buoyObsData.GetAsFloat(2, h);
      data[2] += dir;

      // col 3 - WL
      float bwl = m_buoyObsData.GetAsFloat(3, h);
      if (bwl > data[3])
         data[3] = bwl;

      // are we at the end of the current period?  If so, store max values and reset for next period
      if ((h % hoursPerPeriod) == (hoursPerPeriod - 1))  // are we on the last day of the period?
         {
         // normalize where needed (these are averages, not maxs)
         data[1] /= hoursPerPeriod;
         data[2] /= hoursPerPeriod;

         m_reducedbuoyObsData.AppendRow(data, cols);
         ::memset(data, 0, cols * sizeof(double));
         }
      }  // end of: (for each hourly observation)

   delete[] data;
   return m_reducedbuoyObsData.GetRowCount();

   } // end ReducebuoyObsData(int period)

bool ChronicHazards::InitRun(EnvContext *pEnvContext, bool useInitialSeed)
   {
   // Determine which climate scenario is chosen
   switch (m_climateScenarioID)
      {
      case 0:
         m_climateScenarioStr = "Base";
         break;
      case 1:
         m_climateScenarioStr = "Low";
         break;
      case 2:
         m_climateScenarioStr = "Med";
         break;
      case 3:
         m_climateScenarioStr = "High";
         break;
      case 4:
         m_climateScenarioStr = "Extreme";
         break;
      default:
         m_climateScenarioStr = "Base";
         break;
      }

   // add the path to the climate scenarios folder, e.g. '/Tillamook/TWL Inputs/ClimateScenarios/med/'
   // to the PathManager
   CString climateScenarioPath = m_twlInputDir + "Climate_Scenarios/" + m_climateScenarioStr + "/";
   PathManager::AddPath(climateScenarioPath);
   
   // build climate scenario file name for sea level rise projections 
   CString slrDataFile;
   slrDataFile.Format(_T("%s%s_slr.csv"), (LPCTSTR) climateScenarioPath, (LPCTSTR) m_climateScenarioStr);

   CString fullPath;
   PathManager::FindPath(slrDataFile, fullPath);
   int sEPRows = m_slrData.ReadAscii(fullPath);
   CString Rep = "ChronicHazards: Read in Seal Level Rise data: " + slrDataFile;
   Report::Log(Rep);

   // Change the simulation number from 1 to generated number to randomize 

   // /*this->m_twlTime = 0;
   // this->m_floodingTime = 0;
   // this->m_erosionTime = 0;*/

   // //srand(time(NULL));


   srand(static_cast<unsigned int>(time(NULL)));

   /* if(m_randomize)
   {
   extension.Format("_%i.csv", m_randIndex);
   buoyFile += extension;
   }*/

   // Change the simulation number from 1 to generated number to randomize ****HERE****
   CString simulationPath;
   simulationPath.Format("%sSimulation_%i", climateScenarioPath, 1);
   //   simulationPath.Format("%sSimulation_%i", climateScenarioPath, m_randIndex);

   CString buoyFile;
   buoyFile.Format("%s\\%s\\BuoyData_%s.csv", m_twlInputDir, m_climateScenarioStr);
   //PathManager::FindPath(buoyFile, fullPath);

   // Read daily buoy observations/simulations for the entire Envision time period
   m_numDays = m_buoyObsData.ReadAscii(fullPath);
   int cols = -1;

   if (m_numDays <= 0)
      {
      CString msg;
      msg.Format("ChronicHazards: InitRun could not load buoy csv file \n");
      Report::InfoMsg(msg);
      return false;
      }
   else
      {
      m_numYears = m_numDays / 365;
      CString msg;
      msg.Format("ChronicHazards: The buoy data %s contains %i years of daily data", (LPCTSTR) buoyFile, m_numYears);
      Report::Log(msg);
      /*  Report::Log(buoyFile);
      cols = m_buoyObsData.GetColCount();*/
      }

   CString timeSeriesFolder;
   timeSeriesFolder.Format("%s\\DailyData_%s", (LPCTSTR) simulationPath, (LPCTSTR) m_climateScenarioStr);
   PathManager::FindPath(timeSeriesFolder, fullPath);

   WriteDailyData(fullPath);

   //////CString bayTimeSeriesFolder;
   //////bayTimeSeriesFolder.Format("%s\\DailyBayData_%s", (LPCTSTR) simulationPath, (LPCTSTR) m_climateScenarioStr);
   //////PathManager::FindPath(bayTimeSeriesFolder, fullPath);

   //////bool runSpatialBayTWL = (m_runSpatialBayTWL == 1) ? true : false;
   //////if (runSpatialBayTWL)
   //////   ReadDailyBayData(fullPath);

   // initialize policyInfo for this run (Note - m_isActive is already set by the scenario variable)
   int costCount = 0;
   for (int i = 0; i < PI_COUNT; i++)
      {
      if (m_policyInfoArray[i].HasBudget())
         costCount++;
      }

   float initialBudget = m_totalBudget / float(costCount);
   PolicyInfo::m_budgetAllocFrac = 0.5f;

   for (int i = 0; i < PI_COUNT; i++)
      {
      PolicyInfo &pi = m_policyInfoArray[i];

      pi.m_unmetDemand = 0;
      pi.m_availableBudget = 0;
      pi.m_spentBudget = 0;
      pi.m_unmetDemand = 0;
      pi.m_movAvgBudgetSpent.Clear();
      pi.m_movAvgUnmetDemand.Clear();
      pi.m_allocFrac = 0.5;

      if (pi.HasBudget())
         pi.m_startingBudget = pi.m_availableBudget = initialBudget;
      else
         pi.m_startingBudget = pi.m_availableBudget = 0;
      }

   // reset policy cost data obj for output
   m_policyCostData.ClearRows();

   // reset the cumulative flood map
   if ( m_runFlags & CH_MODEL_FLOODING)
      m_pCumFloodedGrid->SetAllData(0, false);

   //// Read in the Bay coverage if running Bay Flooding Model
   /*bool runBayFloodModel = (m_runBayFloodModel == 1) ? true : false;
   if (runBayFloodModel)
   {
   PathManager::FindPath("BayPts.csv", fullPath);
   m_pInletLUT = new DDataObj;
   m_numBayPts = m_pInletLUT->ReadAscii(fullPath);
   }*/

   CString policySummaryPath(PathManager::GetPath(PM_OUTPUT_DIR));
   CString scname;
   Scenario *pScenario = ::EnvGetScenario(pEnvContext->pEnvModel, pEnvContext->scenarioIndex);

   CString path;
   path.Format("%sPolicySummary_%s_Run%i.csv", (LPCTSTR)policySummaryPath, (LPCTSTR)pScenario->m_name, pEnvContext->run);
   fopen_s(&fpPolicySummary, path, "wt");
   fputs("Year,Policy,Locator,Unit Cost,Total Cost, Avail Budget,Param1,Param2,PassCostConstraint", fpPolicySummary);

   return TRUE;

   } // end InitRun(EnvContext *pEnvContext, bool useInitialSeed)

bool ChronicHazards::WriteDailyData(CString timeSeriesFolder)
   {
   bool writeDailyBouyData = (m_writeDailyBouyData == 1) ? true : false;

   // if randomize, reread in
   //  m_RBFDailyDataArray.Clear();

   const int outputCount = 8;
   double outputs[outputCount];

   // CString path(PathManager::GetPath(PM_PROJECT_DIR));

   if (writeDailyBouyData)
      {
      m_minTransect = 0;
      m_maxTransect = m_numTransects - 1;
      }

   // ??? Need to update to get correct transects?
   for (int i = m_minTransect; i <= m_maxTransect; i++)
      {
      bool success = true;
      FDataObj* pRBFDaily = new FDataObj;
      pRBFDaily->SetSize(outputCount, m_numDays);
      //   extension.Format("_%i", m_randIndex);
      //  CString folder = m_datafile.timeseries_file + extension + "\\";
      //PathManager::FindPath(folder, path);
      // folder = path + folder;

      CString timeSeriesFile;
      timeSeriesFile.Format(_T("%s\\DailyData_%i.csv"), timeSeriesFolder, i);     //series of SWAN lookup tables
                                                                              // generate DailyData_%i.csv files using the rbfs and the buoy (simulation) data timeseries
      if (writeDailyBouyData)
         {
         pRBFDaily->SetLabel(0, "Height_L");
         pRBFDaily->SetLabel(1, "Height_H");
         pRBFDaily->SetLabel(2, "Period_L");
         pRBFDaily->SetLabel(3, "Period_H");
         pRBFDaily->SetLabel(4, "Direction_L");
         pRBFDaily->SetLabel(5, "Direction_H");
         pRBFDaily->SetLabel(6, "Depth_L");
         pRBFDaily->SetLabel(7, "Depth_H");

         // read in the whole simulation time period, typically 90 years
         LookupTable* pLUT = m_swanLookupTableArray.GetAt(i - m_minTransect);   // zero-based

         for (int row = 0; row < m_numDays; row++)
            {
            float height, period, direction, swl;

            m_buoyObsData.Get(1, row, height);
            m_buoyObsData.Get(2, row, period);
            m_buoyObsData.Get(3, row, direction);
            m_buoyObsData.Get(4, row, swl);

            int count = pLUT->m_rbf.GetAt(height, period, direction, outputs);

            // added this because rbf isn't doing well with small ( < 1) values
            // for wave heights; later this becomes a problem with a negative wave height
            if (outputs[HEIGHT_L] < 0.0f)
               outputs[HEIGHT_L] = 0.0f;

            if (outputs[HEIGHT_H] < 0.0f)
               outputs[HEIGHT_H] = 0.0f;

            // Series of Bathymetry lookup tables
            pRBFDaily->Set(0, row, (float)outputs[HEIGHT_L]);
            pRBFDaily->Set(1, row, (float)outputs[HEIGHT_H]);
            pRBFDaily->Set(2, row, (float)outputs[PERIOD_L]);
            pRBFDaily->Set(3, row, (float)outputs[PERIOD_H]);
            pRBFDaily->Set(4, row, (float)outputs[DIRECTION_L]);
            pRBFDaily->Set(5, row, (float)outputs[DIRECTION_H]);
            pRBFDaily->Set(6, row, (float)outputs[DEPTH_L]);
            pRBFDaily->Set(7, row, (float)outputs[DEPTH_H]);
            }

         pRBFDaily->WriteAscii(timeSeriesFile);

         CString msg("ChronicHazards: processing input files for ");
         msg += timeSeriesFolder;
         Report::Log(msg);

         //delete pLUT;
         }
      else
         {
         int numRows = pRBFDaily->ReadAscii(timeSeriesFile);

         if (numRows < 0)
            success = false;
         /*  else
         {
         CString msg("ChronicHazards: processing daily data input file ");
         msg += folder;
         Report::Log(msg);
         }*/
         }

      if (success)
         this->m_RBFDailyDataArray.Add(pRBFDaily);
      else
         delete pRBFDaily;
      }

   CString msg;
   msg.Format("ChronicHazards: Processed DailyData input files for %i transects", (int)m_RBFDailyDataArray.GetSize());
   Report::Log(msg);
   //m_randIndex += 1; // add to xml, only want when we are running probabalistically

   return true;

   } // end WriteDailyData(CString timeSeriesFolder)



bool ChronicHazards::Run(EnvContext *pEnvContext)
   {
   TRACE(_T("ChronicHazards: Starting Run\n"));
   ASSERT(pEnvContext != NULL);

   const float tolerance = 1.0f;    // tolerance value

   if ( m_pNewDuneLineLayer != NULL )
      m_pNewDuneLineLayer->m_readOnly = false;

   ExportMapLayers(pEnvContext, pEnvContext->currentYear);

   // Reset annual duneline attributes
   //ResetAnnualVariables();      // reset annual variables for hte start of a new year
   //ResetAnnualBudget();      // reset budget allocations for the year

   m_numDunePts = m_pNewDuneLineLayer->GetRowCount();
   int verCount = m_maxDuneIndx + 1;

   float shorelineLength = 0.0f;

   // are these needed?
   int periodMaxTWL = -1;
   int tranIndxMaxTWL = -1;
   //  int periodMaxTWLL = -1;
   int tranIndxMaxTWLL = -1;

   // Determine Yearly Maximum TWL and impact days per year 
   // based upon daily TWL calculations at each shoreline location
   CalculateYrMaxTWL(pEnvContext);


   // update timings
   /* finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   this->m_twlTime += duration;*/

   // Report::StatusMsg("ChronicHazards: Running Erosion Model...");

   // start = clock();
   ///////   CalculateErosionExtents(pEnvContext);

   // ****** Calculate Chronic and Event-based Erosion Extents ****** //

   // At this point, annual calculations and summaries are finished (Maximum yearly TWL and impact days) for each shoreline location
   // Next calculate chronic and event based erosion extents for each shoreline location
   int VertexCount = 0;
   float percentArmored = 0;
   INT_PTR ptrArrayIndex = 0;

   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      int beachType = 0;
      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);

      // Do NOT calculate erosion for beachtypes of Bay (9) , River (10) , Rocky Headland (11) , Undefined (0)
      if (beachType >= 1 && beachType <= 8)
         {
         // These characteristics do not change through the time period of this model
         int duneIndx = -1;
         int tranIndx = -1;
         int profileIndx = -1;
         float shorelineChangeRate = 0.0f;
         float shorelineAngle = 0.0f;
         double A = 0.0;

         // Get static Duneline characteristics 
         m_pNewDuneLineLayer->GetData(point, m_colDuneIndx, duneIndx);
         m_pNewDuneLineLayer->GetData(point, m_colTranIndx, tranIndx);
         m_pNewDuneLineLayer->GetData(point, m_colProfileIndx, profileIndx);
         m_pNewDuneLineLayer->GetData(point, m_colShorelineChange, shorelineChangeRate);
         m_pNewDuneLineLayer->GetData(point, m_colShorelineAngle, shorelineAngle);
         //   m_pNewDuneLineLayer->GetData(point, m_colA, A);

         // A is a parameter that governs the overall steepness of the profile
         // varies primarily with grain size or fall velocity
         // currently set to a constant for the whole shoreline
         // eventually want to estimate by ditting a linear regression 
         // between the foreshore slope and the best-fit shape parameter
         // at each profile
         // Currently set to a constant
         A = 0.08;

         // Dynamically changine Duneline characteristics
         float tanb1 = 0.0f;
         float duneToe = 0.0f;
         float duneCrest = 0.0f;
         float beachwidth = 0;

         // Get dynamic Duneline characteristics 
         m_pNewDuneLineLayer->GetData(point, m_colTranSlope, tanb1);
         m_pNewDuneLineLayer->GetData(point, m_colDuneToe, duneToe);
         m_pNewDuneLineLayer->GetData(point, m_colDuneCrest, duneCrest);
         m_pNewDuneLineLayer->GetData(point, m_colBeachWidth, beachwidth);

         // Accumulated annually 
         int IDDToeCount = 0;
         int IDDToeCountWinter = 0;
         int IDDToeCountSpring = 0;
         int IDDToeCountSummer = 0;
         int IDDToeCountFall = 0;

         int IDDCrestCount = 0;
         int IDDCrestCountWinter = 0;
         int IDDCrestCountSpring = 0;
         int IDDCrestCountSummer = 0;
         int IDDCrestCountFall = 0;

         // Impact days per year on dune toe
         m_pNewDuneLineLayer->GetData(point, m_colIDDToe, IDDToeCount);
         m_pNewDuneLineLayer->GetData(point, m_colIDDToeWinter, IDDToeCountWinter);
         m_pNewDuneLineLayer->GetData(point, m_colIDDToeSpring, IDDToeCountSpring);
         m_pNewDuneLineLayer->GetData(point, m_colIDDToeSummer, IDDToeCountSummer);
         m_pNewDuneLineLayer->GetData(point, m_colIDDToeFall, IDDToeCountFall);

         // Impact days per year on dune crest
         m_pNewDuneLineLayer->GetData(point, m_colIDDCrest, IDDCrestCount);
         m_pNewDuneLineLayer->GetData(point, m_colIDDCrestWinter, IDDCrestCountWinter);
         m_pNewDuneLineLayer->GetData(point, m_colIDDCrestSpring, IDDCrestCountSpring);
         m_pNewDuneLineLayer->GetData(point, m_colIDDCrestSummer, IDDCrestCountSummer);
         m_pNewDuneLineLayer->GetData(point, m_colIDDCrestFall, IDDCrestCountFall);


         // Get Bathymetry Data (cross-shore profile)
         DDataObj *pBathy = m_BathyDataArray.GetAt(profileIndx);   // zero-based
         ASSERT(pBathy != NULL);

         int rowMHW = -1;
         double eastingMHW = IGet(pBathy, MHW, 0, 1, IM_LINEAR, 0, &rowMHW, true);

         // The foreshore and Bruun Rule slopes are calculated once using the 
         // cross-shore profile at the beginning of the simulation

         float BruunRuleSlope = 0;
         float foreshoreSlope = 0;

         if (pEnvContext->yearOfRun == 0)
            {
            // The foreshore slope is estimated to be the slope of the profile within
            // +/- 0.5m vertically of MHW
            // The forehsore slope is used to determine all initial equilibrium profile 
            // characteristics
            int rowtmp = -1;

            double easting2_6 = IGet(pBathy, 2.6, 0, 1, IM_LINEAR, rowMHW, &rowtmp, true);
            double easting1_6 = IGet(pBathy, 1.6, 0, 1, IM_LINEAR, rowMHW, &rowtmp, false);
            double distance = (easting2_6 - easting1_6);
            float foreshoreSlope = 1.0f / float(distance);
            m_pNewDuneLineLayer->SetData(point, m_colForeshore, foreshoreSlope);
            rowtmp = -1;
            double easting25m = IGet(pBathy, -25, 0, 1, IM_LINEAR, 0, &rowtmp, true);

            // The Bruun Rule slope is estimated to be the slope of the profile
            // between the 25m contour and MHW
            // The Bruun Rule slope (shoreface slope) is used to estimate
            // chronic erosion extents
            distance = (easting25m - eastingMHW);
            float BruunRuleSlope = (float)((-27.1) / distance);
            m_pNewDuneLineLayer->SetData(point, m_colShoreface, BruunRuleSlope);
            }

         m_pNewDuneLineLayer->GetData(point, m_colShoreface, BruunRuleSlope);
         m_pNewDuneLineLayer->GetData(point, m_colForeshore, foreshoreSlope);

         ///???? needed ???
         int CalibrationDate = 0;
         //int iduType = duneTypeArray[ duneIndx ];

         double xCoord = 0.0;
         double yCoord = 0.0;

         //float avgTWL = 0.0f;
         //m_pNewDuneLineLayer->GetData(point, m_colAvgTWL, avgTWL);           //average TWL for the year

         // ????
         //      m_pNewDuneLineLayer->GetData(point, m_colCalDate, CalibrationDate);

         // Based upon yearly Maximum TWL
         float yrMaxTWL = 0.0f;
         float Hdeep = 0.0f;
         float Period = 0.0f;
         int doy = -1;

         // Yearly Maximum TWL - This represents the largest storm of the year
         //   m_pNewDuneLineLayer->GetData(point, m_colYrEMaxTWL, yrEMaxTWL);
         m_pNewDuneLineLayer->GetData(point, m_colYrMaxTWL, yrMaxTWL);
         /*if (pEnvContext->yearOfRun < m_windowLengthTWL -1)
         m_pNewDuneLineLayer->GetData(point, m_colYrAvgTWL, yrMaxTWL);
         else
         m_pNewDuneLineLayer->GetData(point, m_colMvMaxTWL, yrMaxTWL);*/

         m_meanTWL += yrMaxTWL;

         // SWH at yearly Maximum TWL   
         m_pNewDuneLineLayer->GetData(point, m_colHdeep, Hdeep);

         // Wave Period at yearly Maximum TWL                  
         m_pNewDuneLineLayer->GetData(point, m_colWPeriod, Period);

         // Day of Year of yearly Maximum TWL
         m_pNewDuneLineLayer->GetData(point, m_colYrMaxDoy, doy);


         float waveHeight = 0.0;
         m_pNewDuneLineLayer->GetData(point, m_colWHeight, waveHeight);

         //Now we track ShorelineChange year by year
         /*  float ShorelineChange = 0;

         if ((pEnvContext->currentYear) > 2010 && EPR < 0)
         ShorelineChange = -EPR;
         */

         // Erosion extents are calculated based upon the yearly Maximum TWL 

         //Deepwater wave length (m)
         double L = (g * (Period * Period)) / (2.0 * PI);

         double Cgo = (0.5 * g * Period) / (2.0 * PI);

         // Breaking height  = 0.78 * breaking depth
         //double Hb = ((0.563 * Hdeep) / pow((Hdeep / L), (1.0 / 5.0)));
         double Hb = (0.78 / pow(g, 1. / 5.)) * pow(Cgo*Hdeep*Hdeep, 2. / 5.);

         // Breaking wave water depth
         // hb = Hb / gamma where gamma is a breaker index of 0.78  Kriebel and Dean
         double hbl = Hb / 0.78;

         // Need the negative value since profile is water height relative to land
         float nhbl = (float)-hbl;

         int rowHB = -1;
         double eastingHB = IGet(pBathy, nhbl, 0, 1, IM_LINEAR, rowMHW, &rowHB, false);

         // Calculate the new surf zone width referenced to the MHW
         double Wb = eastingMHW - eastingHB;

         // Equation 31 
         // Constant storm duration, this could be altered       units: hrs
         double TD = m_constTD;
         double C1 = 320.0;

         // using Jeremy Mull's Thesis equations:

         // Equation 12
         //double TS = ((C1*pow(Hb, (3.0 / 2.0)) / (pow(g, (1.0 / 2.0)) * pow(A, 3.0))) * pow((1.0 + hbl / (duneToe - MHW) + foreshoreSlope*Wb / hbl), -1.0)) / 3600.0;

         //// Equation 7
         //double R_inf_KD = ((yrMaxTWL - MHW)* (Wb - hbl / foreshoreSlope)) / (duneCrest - duneToe + hbl - (yrMaxTWL - MHW) / 2.0);

         // exponential time scale Kriebel and Dean, eq 31  units: hrs
         double TS = ((C1*pow(Hb, (3.0 / 2.0)) / (pow(g, (1.0 / 2.0)) * pow(A, 3.0))) * pow((1.0 + hbl / (duneCrest - MHW) + foreshoreSlope * Wb / hbl), -1.0)) / 3600.0;

         //// Equation 25
         double R_inf_KD = ((yrMaxTWL - MHW) * (Wb - hbl / foreshoreSlope)) / ((duneCrest - MHW) + hbl - (yrMaxTWL - MHW) / 2.0);
         //   double R_inf_KD = (yrMaxTWL* (Wb - hbl / foreshoreSlope)) / (duneCrest + hbl - yrMaxTWL / 2.0);


         // Ratio of the erosion time scale to the storm duration K&D eq 11
         double betaKD = 2.0 * PI * TS / TD;

         // Phase lag of maximum erosion response
         double phase1 = Maths::RootFinding::secant(betaKD, 1, 0, 0, 0, PI / 2.0f, PI);

         // Time of maximum erosion 
         double tm1 = TD * phase1 / PI;

         // Rmax / equailibrium response, K&D eq 13
         // Alpha is the characteristics rate parameter of the system = 1 / Ts ,  K&D eq 5
         double alpha = 0.5 * (1.0f - cos(2.0 * PI * (tm1 / TD)));

         if (isNan<double>(alpha))
            alpha = 0.02;

         if (alpha > .4)
            alpha = 0.4;

         //multiply by alpha to get 
         R_inf_KD *= alpha;

         if (isNan<double>(R_inf_KD))
            R_inf_KD = 0;

         if ((beachType != BchT_SANDY_DUNE_BACKED && beachType != BchT_UNDEFINED) || R_inf_KD < 0)
            R_inf_KD = 0;
         // Restrict maximum yearly event-based erosion
         if (R_inf_KD > 25)
            R_inf_KD = 25;

         float slr = 0.0f;
         float xBruun = 0.0f;

         // Bruun Rule Calculation to incorporate SLR into the shoreline change rate
         if ((pEnvContext->currentYear) > 2010)
            {
            float prevslr = 0.0f;
            int year = (pEnvContext->currentYear) - 2010;
            int index = 0;
            if ( slrTimestep == 0 ) // daily
               index = 
             
            m_slrData.Get(0, year - 1, prevslr);

            // yearly or daily?

            float toeRise = slr - prevslr;
            if (beachType == BchT_SANDY_DUNE_BACKED)
               {
               // Beach slope and beachwidth remain static as dune erodes landward and equilibrium is reached 
               // while the dune toe elevation rises at the rate of SLR
               if (toeRise > 0.0f)
                  m_pNewDuneLineLayer->SetData(point, m_colDuneToe, (duneToe + toeRise));
               }

            // Determine influence of SLR on chronic erosion, characterized using a Bruun rule calculation
            if (toeRise > 0.0f)
               xBruun = toeRise / BruunRuleSlope;
            }

         // 
         int Loc = 0;
         m_pNewDuneLineLayer->GetData(point, m_colLittCell, Loc);

         // For Hard Protection Structures
         // change the beachwidth and slope to reflect the new shoreline position
         // beach is assumed to narrow at the rate of the total chronic erosion, reflected through 
         // dynamic beach slopes
         // beaches further narrow when maintaining and constructing BPS at a 1:2 slope
         if (beachType == BchT_RIPRAP_BACKED)
            {
            // Need to account for negative shoreline change rate 
            // shorelineChangeRate > 0.0f ? SCRate = shorelineChangeRate : SCRate;

            // Negative shoreline change rate indicates eroding shoreline
            // Currently ignoring progradating shoreline
            float SCRate = 0.0f;

            if (shorelineChangeRate < 0.0f)
               SCRate = (float)-shorelineChangeRate;
            else
               SCRate = 0.0f;


            // Negative shoreline change rate narrows beach
            if (xBruun > 0.0f)
               beachwidth = beachwidth - SCRate - xBruun;
            else
               beachwidth = beachwidth - SCRate;

            if ((duneToe / beachwidth) > 0.1f)
               {
               beachwidth = duneToe / 0.1f;
               }

            tanb1 = duneToe / beachwidth;

            // Determine a minimum slope for which the
            // beach can erode

            //if (tanb1 > m_slopeThresh)
            //   tanb1 = m_slopeThresh;

            // slope is steepened due to beachwidth narrowing
            m_pNewDuneLineLayer->SetData(point, m_colTranSlope, tanb1);

            // keeping beachwidth static, translate MHW by same amount
            /*m_pNewDuneLineLayer->GetData(point, m_colEastingMHW, x);
            m_pNewDuneLineLayer->SetData(point, m_colEastingMHW, x + dx);*/
            }

         // Set new position of the Dune toe point
         Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(point);
         if (beachType == BchT_SANDY_DUNE_BACKED)
            {
            if (pPoly->GetVertexCount() > 0)
               {
               xCoord = pPoly->GetVertex(0).x;

               //Move the point to the extent of erosion. For now, all three types of erosion are included.   
               float dx = 0.0f;

               // Currently only move dunetoe line for eroding shoreline (not prograding)
               /*float SCRate = 0.0f;

               shorelineChangeRate > 0.0f ? SCRate = shorelineChangeRate : SCRate;*/

               // If shoreline is eroding, dune toe line moves landward
               if (shorelineChangeRate < 0.0f)
                  dx = abs(shorelineChangeRate) + xBruun;
               else
                  dx = xBruun;

               pPoly->Translate(dx, 0.0f);
               pPoly->InitLogicalPoints(m_pMap);

               float x = 0.0f;
               m_pNewDuneLineLayer->GetData(point, m_colEastingToe, x);
               m_pNewDuneLineLayer->SetData(point, m_colEastingToe, x + dx);
               m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, x);
               m_pNewDuneLineLayer->SetData(point, m_colEastingCrest, x + dx);

               // Translate MHW by same amount
               /*m_pNewDuneLineLayer->GetData(point, m_colEastingMHW, x);
               m_pNewDuneLineLayer->SetData(point, m_colEastingMHW, x + dx);*/

               //convert northing and easting to longitude and latitude for csv display
               CalculateUTM2LL(m_pNewDuneLineLayer, point);

               /*float xc = 0.0f;
               m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, xc);
               m_pNewDuneLineLayer->SetData(point, m_colEastingCrest, xc + dx);*/
               }
            }

         if (beachType == BchT_RIPRAP_BACKED)
            {
            float BPSCost;
            m_pNewDuneLineLayer->GetData(point, m_colCostBPS, BPSCost);
            m_maintCostBPS += BPSCost * m_costs.BPSMaint;
            }

         //float Ero = (float)R_inf_KD;

         int yr = pEnvContext->yearOfRun;

         MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(point);
         ipdyMovingWindow->AddValue((float)(IDDToeCount + IDDCrestCount));

         // for debugging
         if (m_debugOn)
            {
            int totalIDPY = 0;
            m_pNewDuneLineLayer->GetData(point, m_colNumIDPY, totalIDPY);
            totalIDPY += (IDDToeCount + IDDCrestCount);
            m_pNewDuneLineLayer->SetData(point, m_colNumIDPY, totalIDPY);
            }


         // Retrieve the average imapact days per year within the designated window
         float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();
         m_pNewDuneLineLayer->SetData(point, m_colMvAvgIDPY, movingAvgIDPY);

         MovingWindow *twlMovingWindow = m_TWLArray.GetAt(point);
         twlMovingWindow->AddValue(yrMaxTWL);
         // Retrieve the maxmum TWL within the designated window
         float movingMaxTWL = twlMovingWindow->GetMaxValue();
         m_pNewDuneLineLayer->SetData(point, m_colMvMaxTWL, movingMaxTWL);

         // Retrieve the average TWL within the designated window
         float movingAvgTWL = twlMovingWindow->GetAvgValue();
         m_pNewDuneLineLayer->SetData(point, m_colMvAvgTWL, movingAvgTWL);

         MovingWindow *eroKDMovingWindow = m_eroKDArray.GetAt(point);
         eroKDMovingWindow->AddValue((float)R_inf_KD);

         ptrArrayIndex++;

         float avgIDPY_dtoe = IDDToeCount / 365.0f;
         float avgIDPY_dcrest = IDDCrestCount / 365.0f;

         float beachAccess = (1 - avgIDPY_dtoe - avgIDPY_dcrest) * 100;
         float beachAccessWinter = (1 - (IDDToeCountWinter / 89.0f) - (IDDCrestCountWinter / 89.0f)) * 100;
         float beachAccessSpring = (1 - (IDDToeCountSpring / 93.0f) - (IDDCrestCountSpring / 93.0f)) * 100;
         float beachAccessSummer = (1 - (IDDToeCountSummer / 94.0f) - (IDDCrestCountSummer / 94.0f)) * 100;
         float beachAccessFall = (1 - (IDDToeCountFall / 89.0f) - (IDDCrestCountFall / 89.0f)) * 100;

         float reltwl = yrMaxTWL - duneToe;
         //   float sumEro = Ero;
         float sumduneT = avgIDPY_dtoe;
         float sumduneC = avgIDPY_dcrest;
         //     float sumTWL = twlav;
         float sumrelTWL = reltwl;

         //if (beachAccess < 0)
         //   beachAccess = 0;

         if (beachAccess > m_accessThresh)
            m_avgCountyAccess += 1;

         if (Loc == 1)
            {
            //if (beachAccess > m_accessThresh)
            //   m_avgOceanShoresAccess += 1;
            }

         if (Loc == 2)
            {
            //if (beachAccess > m_accessThresh)
            //   m_avgWestportAccess += 1;
            }


         // m_hardenedShoreline = (percentArmored / float(shorelineLength)) * 100;
         // m_pNewDuneLineLayer->m_readOnly = false;

         //         m_pNewDuneLineLayer->SetData(point, m_colAvgDuneT, avgduneTArray[ duneIndx ]);
         //         m_pNewDuneLineLayer->SetData(point, m_colAvgDuneC, avgduneCArray[ duneIndx ]);
         //         m_pNewDuneLineLayer->SetData(point, m_colAvgEro, avgEroArray[ duneIndx ]);
         //         m_pNewDuneLineLayer->SetData(point, m_colCPolicyL, 0);
         m_pNewDuneLineLayer->SetData(point, m_colBchAccess, beachAccess);
         m_pNewDuneLineLayer->SetData(point, m_colBchAccessWinter, beachAccessWinter);
         m_pNewDuneLineLayer->SetData(point, m_colBchAccessSpring, beachAccessSpring);
         m_pNewDuneLineLayer->SetData(point, m_colBchAccessSummer, beachAccessSummer);
         m_pNewDuneLineLayer->SetData(point, m_colBchAccessFall, beachAccessFall);

         m_pNewDuneLineLayer->SetData(point, m_colWb, Wb);
         m_pNewDuneLineLayer->SetData(point, m_colTS, TS);
         m_pNewDuneLineLayer->SetData(point, m_colHb, Hb);
         m_pNewDuneLineLayer->SetData(point, m_colKD, R_inf_KD);
         m_pNewDuneLineLayer->SetData(point, m_colAlpha, alpha);
         m_pNewDuneLineLayer->SetData(point, m_colShorelineChange, shorelineChangeRate);
         m_pNewDuneLineLayer->SetData(point, m_colBeachWidth, beachwidth);

         } // end erosion calculation
      } // end dune line points

        /******************** Run Scenario Policy to construct on safest site before tallying statistics  ********************/

   m_meanTWL /= m_numDunePts;

   // Scenarios indicate policy for new buildings to be constructed on the safest site
   /*bool runConstructSafestPolicy = (m_runConstructSafestPolicy == 1) ? true : false;
   if (runConstructSafestPolicy)
   ConstructOnSafestSite(pEnvContext, false);
   */

   ///////   if (m_runEelgrassModel > 0)
   ///////      RunEelgrassModel(pEnvContext);
   bool runEelgrassModel = (m_runEelgrassModel == 1) ? true : false;
   int count = 0;
   if (runEelgrassModel)
      {
      float avgSWL = 0.0f;
      float avgLowSWL = 0.0f;

      MapLayer::Iterator pt = m_pNewDuneLineLayer->Begin();
      m_pNewDuneLineLayer->GetData(pt, m_colYrAvgSWL, avgSWL);
      m_pNewDuneLineLayer->GetData(pt, m_colYrAvgLowSWL, avgLowSWL);

      float avgWL = (avgSWL + avgLowSWL) / 2.0f;

      if (pEnvContext->yearOfRun == 0)
         {
         m_avgWL0 = avgWL;
         }

      m_delta = avgWL - m_avgWL0;

      //m_eelgrassArea = GenerateEelgrassMap(count);
      //m_eelgrassAreaSqMiles = m_eelgrassArea * 3.8610216e-07;
      //
      //int intertidalCount = 0;
      //for (int row = 0; row < m_numTidalRows; row++)
      //   {
      //   for (int col = 0; col < m_numTidalCols; col++)
      //      {
      //
      //      float elevation = 0.0f;
      //      m_pTidalBathyGrid->GetData(row, col, elevation);
      //      if (elevation != -9999.0f)
      //         {
      //         if (elevation >= avgLowSWL && elevation <= avgSWL)
      //            intertidalCount++;
      //         }
      //      }
      //   }

      //m_intertidalArea = intertidalCount * m_cellHeight * m_cellWidth;
      //m_intertidalAreaSqMiles = m_intertidalArea * 3.8610216e-07;

      }



   /************************   Generate Flooding Grid ************************/

   count = 0;

   clock_t start = clock();

   // Choose which overland flooding model to run
   //m_floodedArea = RunFloodAreas(pEnvContext);
   //
   //m_floodedAreaSqMiles = m_floodedArea * 3.8610216e-07;
   //   m_pFloodedGrid->SetTransparency(50);

   // update timings
   clock_t finish = clock();
   float duration = (float)(finish - start) / CLOCKS_PER_SEC;

   CString msg;
   msg.Format("Flood Model ran in %i seconds", (int)duration);
   Report::Log(msg);


   /************************   Generate Erosion Grid ************************/
   count = 0;
   GenerateErosionMap(count);

   msg.Format("ChronicHazards: %i Flooding and Erosion Maps generated", pEnvContext->currentYear);
   Report::Log(msg);




   /******************** Use generated Grid Maps to Calculate Statistics ********************/

   // statistics to output or statistics which in turn trigger policies 
   ComputeBuildingStatistics();

   ComputeInfraStatistics();

   // Calculate Shoreline Averages
   m_avgCountyAccess /= (m_numDunePts / 100.0f);
   //m_avgOceanShoresAccess /= (m_numOceanShoresPts / 100.0f);
   //m_avgWestportAccess /= (m_numWestportPts / 100.0f);
   //m_avgJettyAccess /= (m_numJettyPts / 100.0f);


   /******************** Run Scenario dependent policies  ********************/


   // Get new IDUBuildingsLkup every year !!!!!!
   m_pIduBuildingLkup->BuildIndex();

   for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
      {
      //Associate new buildings with communities
      // get Buildings in this IDU
      CArray< int, int > ptArray;

      int iduCityCode = 0;
      m_pIDULayer->GetData(idu, m_colIDUCityCode, iduCityCode);

      int numBldgs = m_pIduBuildingLkup->GetPointsFromPolyIndex(idu, ptArray);

      for (int i = 0; i < numBldgs; i++)
         {
         int bldgIndex = ptArray[i];

         /*int bldgCityCode = 0;
         m_pBldgLayer->GetData(ptArray[i], m_colBldgCityCode, bldgCityCode);*/

         m_pBldgLayer->SetData(bldgIndex, m_colBldgCityCode, iduCityCode);
         } // end each new building
      }

   RunPolicies(pEnvContext);

   TallyCostStatistics(pEnvContext->currentYear);
   TallyDuneStatistics(pEnvContext->currentYear);
   TallyRoadStatistics();

   //  m_pMap->ChangeDrawOrder(m_pOrigDuneLineLayer, DO_BOTTOM);
   m_pMap->ChangeDrawOrder(m_pNewDuneLineLayer, DO_BOTTOM);

   //m_pNewDuneLineLayer->m_readOnly = true;

   // last year of run
   if (pEnvContext->endYear == pEnvContext->currentYear + 1)
      ExportMapLayers(pEnvContext, pEnvContext->endYear);

   return true;

   } // end Run(EnvContext *pEnvContext)

bool ChronicHazards::EndRun(EnvContext *pEnvContext)
   {
   fclose(fpPolicySummary);

   return TRUE;
   }

/*

bool ChronicHazards::ResetAnnualVariables()
   {
   m_pNewDuneLineLayer->m_readOnly = false;
   m_pNewDuneLineLayer->SetColData(m_colYrFMaxTWL, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colYrMaxTWL, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colYrAvgTWL, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colYrAvgSWL, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colYrAvgLowSWL, VData(0.0f), true);

   m_pNewDuneLineLayer->SetColData(m_colFlooded, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colAmtFlooded, VData(0.0), true);

   m_pNewDuneLineLayer->SetColData(m_colYrMaxDoy, VData(-1), true);
   m_pNewDuneLineLayer->SetColData(m_colHdeep, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colWPeriod, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colWDirection, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colWHeight, VData(0.0f), true);

   m_pNewDuneLineLayer->SetColData(m_colYrMaxSetup, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colYrMaxSWL, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colYrMaxIGSwash, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colYrMaxIncSwash, VData(0.0f), true);
   m_pNewDuneLineLayer->SetColData(m_colYrMaxSwash, VData(0.0f), true);

   m_pNewDuneLineLayer->SetColData(m_colIDDToe, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colIDDToeWinter, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colIDDToeSpring, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colIDDToeSummer, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colIDDToeFall, VData(0), true);

   m_pNewDuneLineLayer->SetColData(m_colIDDCrest, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colIDDCrestWinter, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colIDDCrestSpring, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colIDDCrestSummer, VData(0), true);
   m_pNewDuneLineLayer->SetColData(m_colIDDCrestFall, VData(0), true);

   // Reset annual attributes for Building, Infrastructure and Roads
   m_pBldgLayer->SetColData(m_colBldgFlooded, VData(0), true);

   m_pInfraLayer->SetColData(m_colInfraFlooded, VData(0), true);
   //   m_pInfraLayer->SetColData(m_colInfraEroded, VData(0), true);
   m_pRoadLayer->SetColData(m_colRoadFlooded, VData(0), true);

   // Initialize or Reset annual statistics for the whole coastline/study area

   // Reset annual County Wide variables to zero   

   m_meanTWL = 0.0f;
   m_floodedArea = 0.0f;         // square meters, m_floodedAreaSqMiles (sq Miles) is calculated from floodedArea
                                 //m_maxTWL = 0.0f;

                                 // Hard Protection Metrics
   m_noConstructedBPS = 0;
   m_constructCostBPS = 0.0;
   m_maintCostBPS = 0.0;
   m_nourishCostBPS = 0.0;
   m_nourishVolumeBPS = 0.0;
   m_totalHardenedShoreline = 0.0;         // meters
   m_totalHardenedShorelineMiles = 0.0;   // miles
   m_addedHardenedShoreline = 0.0;         // meters
   m_addedHardenedShorelineMiles = 0.0;   // miles


                                          // Soft Protection Metrics
   m_noConstructedDune = 0;
   m_restoreCostSPS = 0.0;
   m_maintCostSPS = 0.0;
   m_nourishCostSPS = 0.0;
   m_constructVolumeSPS = 0.0;
   m_maintVolumeSPS = 0.0;
   m_nourishVolumeSPS = 0.0;
   m_addedRestoredShoreline = 0.0;         // meters
   m_addedRestoredShorelineMiles = 0.0;   // miles
   m_totalRestoredShoreline = 0.0;         // meters
   m_totalRestoredShorelineMiles = 0.0;   // miles


   m_noBldgsRemovedFromHazardZone = 0;
   //m_noBldgsEroded = 0;
   // Beach accesibility
   m_avgCountyAccess = 0.0f;
   m_avgOceanShoresAccess = 0.0f;
   m_avgWestportAccess = 0.0f;
   m_avgJettyAccess = 0.0f;

   m_eelgrassArea = 0.0;
   m_eelgrassAreaSqMiles = 0.0;

   m_intertidalArea = 0.0;
   m_intertidalAreaSqMiles = 0.0;

   m_erodedRoad = 0.0;
   m_erodedRoadMiles = 0.0;
   m_floodedArea = 0.0;
   m_floodedAreaSqMiles = 0.0;
   m_floodedRailroadMiles = 0.0;
   m_floodedRoad = 0.0;
   m_floodedRoadMiles = 0.0;

   // m_erodedBldgCount = 0;
   // m_floodedBldgCount = 0;

   return true;
   }
*/

/*

bool ChronicHazards::ResetAnnualBudget()
   {
   // allocate budgets for the year.  Basic idea:
   // 1) There is no carry-over - budgets are reset every year.
   // 2) Budgets are redistributed at the beginning of each year according to the cumulative unmet demand.
   //    A portion is reserved as base funding, the rest is distributed proportional to unmet cumulative demand

   // first, figure out what the total unmet demand from the last year was
   float totalUnmetDemand = 0;
   float totalSpentBudget = 0;
   int costCount = 0;

   for (int i = 0; i < PI_COUNT; i++)
      {
      PolicyInfo &pi = m_policyInfoArray[i];
      if (pi.HasBudget())
         {
         totalUnmetDemand += pi.m_movAvgUnmetDemand.GetAvgValue();
         totalSpentBudget += pi.m_movAvgBudgetSpent.GetAvgValue();
         costCount++;
         }
      }

   if (costCount <= 0)
      return true;

   // basic idea: next years budget is based on:
   //   1) the proportion of this years budget going to this policy
   //   2) the proportion of the unmetDemand attributed to this policy

   PolicyInfo::m_budgetAllocFrac = 0.5f;
   if (totalSpentBudget > 0 && totalUnmetDemand > 0)
      PolicyInfo::m_budgetAllocFrac = totalSpentBudget / (totalSpentBudget + totalUnmetDemand);

   float initialBudget = m_totalBudget / costCount;
   float totalBudgetAllocated = 0;
   float minBudgetAlloc = 100000;
   int usingMinBudgetCount = 0;

   for (int i = 0; i < PI_COUNT; i++)
      {
      PolicyInfo &pi = m_policyInfoArray[i];

      if (pi.HasBudget())
         {
         // portion based on this years spending levels
         float budget0 = -1;
         if (totalSpentBudget > 0)
            budget0 = m_totalBudget * (pi.m_movAvgBudgetSpent.GetAvgValue() / totalSpentBudget);

         // portion based on moving average of unmet demand
         float budget1 = -1;
         if (totalUnmetDemand > 0)
            budget1 = m_totalBudget * (pi.m_movAvgUnmetDemand.GetAvgValue() / totalUnmetDemand);

         // normalize budget 1, 2 to sum to m_totalBudget if below budget
         // if ( (budget0 + budget1) < m_totalBudget )
         //    {
         //    budget0 = m_totalBudget * budget0 / (budget1 + budget0);
         //    budget1 = m_totalBudget - budget0;
         //    }

         // four cases to consider:
         float budgetAlloc = 0;
         if (totalSpentBudget <= 0 && totalUnmetDemand <= 0)
            budgetAlloc = initialBudget;

         else if (totalSpentBudget <= 0)  // only based on unmet demand
            budgetAlloc = budget1;

         else if (totalUnmetDemand <= 0)   // based only on this years spending
            budgetAlloc = budget0;

         else     // based on both
            budgetAlloc = (PolicyInfo::m_budgetAllocFrac*budget0)
            + ((1.0f - PolicyInfo::m_budgetAllocFrac)*budget1);

         // set allocation ration for this policy
         if ((pi.m_spentBudget + pi.m_unmetDemand) > 0)
            pi.m_allocFrac = pi.m_spentBudget / (pi.m_spentBudget + pi.m_unmetDemand);
         else
            pi.m_allocFrac = 0;

         if (budgetAlloc < minBudgetAlloc)
            {
            usingMinBudgetCount++;
            budgetAlloc = minBudgetAlloc;
            pi.m_usingMinBudget = true;
            }
         else
            {
            pi.m_usingMinBudget = false;
            totalBudgetAllocated += budgetAlloc;   // don't count for min budgets
            }

         pi.m_startingBudget = pi.m_availableBudget = budgetAlloc;
         }
      else
         pi.m_startingBudget = pi.m_availableBudget = 0;

      pi.m_spentBudget = 0;
      pi.m_unmetDemand = 0;
      }

   // normalize allocated budget m_totalBudget if below budget.  Note that
   // policies recieving the minimum were not included in the total budget
   // allocated and we don't normalize them.
   float totalBudgetAvail = m_totalBudget - (usingMinBudgetCount * minBudgetAlloc);
   float ratio = totalBudgetAvail / totalBudgetAllocated;

   for (int i = 0; i < PI_COUNT; i++)
      {
      PolicyInfo &pi = m_policyInfoArray[i];

      if (pi.HasBudget() && pi.m_usingMinBudget == false)
         {
         float budgetAlloc = pi.m_startingBudget;
         float adjAlloc = budgetAlloc * ratio;
         pi.m_startingBudget = pi.m_availableBudget = adjAlloc;
         }
      }

   return true;
   }

   */

//bool ChronicHazards::LoadPolyGridLookup()
//{
//   if (m_pPolygonGridLkUp f!= NULL)
//      return true;
//   TRACE(_T("ChronicHazards: Starting LoadPolyGridLookup \n"));
//
//   float
//      cellWidth = 0.0f,
//      cellHeight = 0.0f;
//
//   float
//      xMin = -1.0F,
//      xMax = -1.0F,
//      yMin = -1.0F,
//      yMax = -1.0F;
//
//   long
//      n = -1,
//      maxEls = -1,
//      gridRslt = -1;
//
//   MapLayer *pGridMapLayer = NULL;
//
//   if (m_pMap != NULL)
//   {
//      pGridMapLayer = m_pMap->AddLayer(LT_GRID);
//      ASSERT(pGridMapLayer != NULL);
//   }
//
//   else
//   {
//      // get Map pointer
//      m_pMap = m_pIDULayer->GetMapPtr();
//      ASSERT(m_pMap != NULL);
//   }
//
//   //  DEM layer characteristics
//   m_pElevationGrid->GetExtents(xMin, xMax, yMin, yMax);
//   m_pElevationGrid->GetCellSize(cellWidth, cellHeight);
//
//   float noDataValue = m_pElevationGrid->GetNoDataValue();
//
//   m_numRows = m_pElevationGrid->GetRowCount();
//   m_numCols = m_pElevationGrid->GetColCount();
//
//   maxEls = (m_numRows * m_numCols);
//   n = m_pIDULayer->GetPolygonCount();
//
//   CString msg;
//   msg.Format("ChronicHazards: Polygrid - %i Rows, %i Cols, CellWidth: %f CellHeight: %f NumElements: %d", m_numRows, m_numCols, cellWidth, cellHeight, maxEls);
//   Report::Log(msg);
//
//   clock_t start = clock();
//
//   // create grids, based on the extent of polygon layer, for important intermediate and output values
//   gridRslt = pGridMapLayer->CreateGrid(m_numRows, m_numCols, xMin, yMin, cellWidth, cellHeight, -9999, DOT_INT, FALSE, FALSE);
//
//   // if pgl file exists, use it, otherwise create and persist it
//   CPath pglFile(PathManager::GetPath(PM_IDU_DIR));
//
//   CString filename;
//   pglFile.Append("PolyGridLookups\\");
//   filename.Format("PolyFloodedLkup.pgl");
//   pglFile.Append(filename);
//
//   // if .pgl file exists: read poly - grid - lookup table from file
//   // else (if .pgl file does NOT exists): create poly - grid - lookup table and write to .pgl file
//   if (pglFile.Exists())
//   {
//      msg = "  Loading PolyGrid from File ";
//      msg += pglFile;
//      Report::Log(msg);
//
//      m_pPolygonGridLkUp = new PolyGridMapper(pglFile);
//
//      // if existing file has wrong dimension, create NEW poly - grid - lookup table and write to .pgl file
//      if ((m_pPolygonGridLkUp->GetNumGridCols() != m_numCols) || (m_pPolygonGridLkUp->GetNumGridRows() != m_numRows))
//      {
//         msg = "Invalid dimensions: Recreating File ";
//         msg += pglFile;
//         Report::Log(msg);
//         m_pPolylineGridLkUp = new PolyGridMapper(m_pRoadLayer, m_pElevationGrid, 1);
//         //      m_pPolygonGridLkUp = new PolyGridLookups(pGridMapLayer, m_pIDULayer, 1, maxEls, 0, -9999);
//         m_pPolygonGridLkUp->WriteToFile(pglFile);
//      }
//   }
//   else  // doesn't exist, create it
//   {
//      msg = "  Generating PolyGrid ";
//      msg += pglFile;
//      Report::Log(msg);
//      m_pPolylineGridLkUp = new PolyGridMapper(m_pRoadLayer, m_pElevationGrid, 1);
//      //    m_pPolygonGridLkUp = new PolyGridLookups(pGridMapLayer, m_pIDULayer, 1, maxEls, 0, -9999);
//      m_pPolygonGridLkUp->WriteToFile(pglFile);
//   }
//
//   clock_t finish = clock();
//   float duration = (float)(finish - start) / CLOCKS_PER_SEC;
//
//   msg.Format("PolyGridLookup generated in %i seconds", (int)duration);
//   Report::Log(msg);
//
//   return true;
//} // BOOL ChronicHazards::LoadPolyGridLookup


//bool ChronicHazards::LoadPolyGridLookup(MapLayer *pGridLayer, MapLayer *pPolyLayer, CString fullFilename, PolyGridLookups *&pPolyGridLookup)
//{
//   if (pPolyGridLookup != NULL)
//      return true;
//
//   if (pGridLayer == NULL)
//      return false;
//
//   if (pPolyLayer == NULL)
//      return false;
//
//   if (CString(fullFilename).IsEmpty())
//      return false;
//
//   TRACE(_T("ChronicHazards: Starting LoadPolyGridLookup \n"));
//
//   float
//      cellWidth = 0.0f,
//      cellHeight = 0.0f;
//
//   float
//      xMin = -1.0f,
//      xMax = -1.0f,
//      yMin = -1.0f,
//      yMax = -1.0f;
//
//   float noDataValue = -9999.f;
//
//   // grid layer for polyGridLookup
//   MapLayer *pGridMapLayer = NULL;
//
//   if (pGridLayer->GetType() == LT_GRID)
//   {
//      // Grid layer characteristics
//      pGridLayer->GetExtents(xMin, xMax, yMin, yMax);
//      pGridLayer->GetCellSize(cellWidth, cellHeight);
//      noDataValue = pGridLayer->GetNoDataValue();
//
//      //added
//      m_numRows = pGridLayer->GetRowCount();
//      m_numCols = pGridLayer->GetColCount();
//      pGridMapLayer = pGridLayer;
//   }
//   // Polygon layer characteristics
//   else if (pGridLayer->GetType() == LT_POLYGON)
//   {
//      float xExtent = xMax - xMin;
//      float yExtent = yMax - yMin;
//      // input!!
//      cellHeight = cellWidth = 90;
//      m_numRows = (int)ceil(yExtent / cellHeight);
//      m_numCols = (int)ceil(xExtent / cellWidth);
//      noDataValue = pGridLayer->GetNoDataValue();
//
//      // get Map pointer
//      m_pMap = pPolyLayer->GetMapPtr();
//      ASSERT(m_pMap != NULL);
//      pGridMapLayer = m_pMap->AddLayer(LT_GRID);
//      ASSERT(pGridMapLayer != NULL);
//
//      // create grids, based on the extent of polygon layer, for important intermediate and output values
//      int gridRslt = pGridMapLayer->CreateGrid(m_numRows, m_numCols, xMin, yMin, cellWidth, cellHeight, noDataValue, DOT_INT, FALSE, FALSE);
//   }
//
//   int maxEls = (m_numRows * m_numCols);
//   int n = pPolyLayer->GetPolygonCount();
//
//   CString msg;
//   msg.Format("ChronicHazards: Polygrid - %i Rows, %i Cols, CellWidth: %f CellHeight: %f NumElements: %d", m_numRows, m_numCols, cellWidth, cellHeight, maxEls);
//   Report::Log(msg);
//
//   clock_t start = clock();
//
//   CString pglPath = NULL;
//
//   int status = PathManager::FindPath(fullFilename, pglPath);    // look along Envision paths
//
//   // if pgl file exists, use it, otherwise create poly grid lookup and persist it
//   // else (if .pgl file does NOT exists): create poly - grid - lookup table and write to .pgl file
//   if (status >= 0)
//   {
//      msg = "ChronicHazards: Loading PolyGrid from File ";
//      msg += pglPath;
//      Report::Log(msg);
//
//      pPolyGridLookup = new PolyGridLookups(pglPath);
//
//      //// if existing file has wrong dimension, create NEW poly - grid - lookup table and write to .pgl file
//      // if ((pPolyGridLookup->GetNumGridCols() != m_numCols) || (pPolyGridLookup->GetNumGridRows() != m_numRows))
//      //{
//      //msg = "ChronicHazards: Invalid dimensions: Recreating File ";
//      //msg += pglPath;
//      //Report::Log(msg);
//      //pPolyGridLookup = new PolyGridLookups(pGridMapLayer, pPolyLayer, 1, maxEls, 0, -9999);
//      //pPolyGridLookup->WriteToFile(pglPath);
//      //}
//   } else  // doesn't exist, create it
//   {
//      pglPath = fullFilename;
//      msg = "ChronicHazards: Generating PolyGrid ";
//      msg += pglPath;
//      Report::Log(msg);
//
//      pPolyGridLookup = new PolyGridLookups(pGridMapLayer, pPolyLayer, 1, maxEls, 0, (int)noDataValue);
//      pPolyGridLookup->WriteToFile(pglPath);
//   }
//
//   clock_t finish = clock();
//   float duration = (float)(finish - start) / CLOCKS_PER_SEC;
//
//   msg.Format("ChronicHazards: PolyGridLookup generated in %i seconds", (int)duration);
//   Report::Log(msg);
//
//   return true;
//} // BOOL ChronicHazards::LoadPolyGridLookup

// Runs the flooding model returning the number of grid cells flooded and the area flooded
///////double ChronicHazards::GenerateBathtubFloodMap(int &floodedCount)
///////   {
///////   //return 0;
///////
///////   if (m_pFloodedGrid == NULL)
///////      {
///////      m_pFloodedGrid = m_pMap->CloneLayer(*m_pElevationGrid);
///////      m_pFloodedGrid->m_name = "Flooded Grid";
///////
///////      m_pCumFloodedGrid = m_pMap->CloneLayer(*m_pElevationGrid);
///////      m_pCumFloodedGrid->m_name = "Cumulative Flooded Grid";
///////      }
///////   m_pFloodedGrid->SetAllData(0.0, false);
///////   m_pCumFloodedGrid->SetAllData(0.0, false);
///////
///////   // Determine the area of the grid that got flooded
///////
///////   // Value: 0 if not flooded, TWL if flooded
///////   float flooded = 0.0f;
///////
///////   // Counter of number of grid cells in flooded grid that are flooded 
///////   // used to determine area flooded
///////
///////   // area = grid cells that are flooded multiplied by the number of grid cells 
///////   double floodedArea = 0.0f;
///////
///////   //clock_t start = clock();
///////
///////   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
///////      {
///////      int beachType = 0;
///////      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
///////
///////      int duneIndx = -1;
///////      m_pNewDuneLineLayer->GetData(point, m_colDuneIndx, duneIndx);
///////
///////      REAL xCoord = 0.0;
///////      REAL yCoord = 0.0;
///////
///////      int startRow = -1;
///////      int startCol = -1;
///////      float minElevation = 0.0f;
///////      float yearlyMaxTWL = 0.0f;
///////      float yearlyAvgTWL = 0.0f;
///////      double duneCrest = 0.0;
///////
///////      if (beachType == BchT_BAY)
///////         {
///////         // get twl of inlet seed point
///////         m_pNewDuneLineLayer->GetData(point, m_colYrFMaxTWL, yearlyMaxTWL);
///////
///////         //if (pEnvContext->yearOfRun < m_windowLengthTWL - 1)
///////         //   m_pNewDuneLineLayer->GetData(point, m_colYrAvgTWL, yearlyMaxTWL);
///////         //else
///////         //   m_pNewDuneLineLayer->GetData(point, m_colMvMaxTWL, yearlyMaxTWL);
///////
///////            // traversing along bay shoreline, determine if twl is more than the elevation 
///////
///////            //for (int inletRow = 0; inletRow < m_numBayPts; inletRow++)
///////            //{
///////            //   // bay trace can be beachtype of bay or river
///////            //   // do not calculate flooding for river type
///////            //   /*beachType = m_pInletLUT->GetAsInt(3, inletRow);            
///////            //   if (beachType == BchT_BAY)
///////            //   {
///////            //      xCoord = m_pInletLUT->GetAsDouble(1, inletRow);
///////            //      yCoord = m_pInletLUT->GetAsDouble(2, inletRow);
///////            //      int bayFloodedCount = 0;
///////            //      bayFloodedCount = m_pInletLUT->GetAsInt(4, inletRow);
///////
///////         int colNorthing;
///////         int colEasting;
///////
///////         CheckCol(m_pBayTraceLayer, colNorthing, "NORTHING", TYPE_DOUBLE, CC_MUST_EXIST);
///////         CheckCol(m_pBayTraceLayer, colEasting, "EASTING", TYPE_DOUBLE, CC_MUST_EXIST);
///////
///////         // traversing along bay shoreline, determine is more than the elevation 
///////         for (MapLayer::Iterator bayPt = m_pBayTraceLayer->Begin(); bayPt < m_pBayTraceLayer->End(); bayPt++)
///////            {
///////            m_pBayTraceLayer->GetData(bayPt, colEasting, xCoord);
///////            m_pBayTraceLayer->GetData(bayPt, colNorthing, yCoord);
///////
///////            m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
///////            
///////            //      m_pInletLUT->Set(4, inletRow, startRow);
///////            //      m_pInletLUT->Set(5, inletRow, startCol);
///////            //
///////            //      /*m_pElevationGrid->SetData(5255, 4373, 3.8f);
///////            //      m_pElevationGrid->SetData(5256, 4372, 3.8f);
///////            //      m_pElevationGrid->SetData(5260, 4369, 3.8f);
///////
///////
///////                  // within grid ?
///////            if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
///////               {
///////               m_pElevationGrid->GetData(startRow, startCol, minElevation);
///////               m_pFloodedGrid->GetData(startRow, startCol, flooded);
///////
///////               bool isFlooded = (flooded > 0.0f) ? true : false;
///////
///////               //   m_pInletLUT->Set(5, inletRow, yearlyMaxTWL);
///////         //         m_pInletLUT->Set(6, inletRow, minElevation);
///////
///////                  // for the cell corresponding to bay point coordinate
///////               if (!isFlooded && (yearlyMaxTWL > duneCrest) && (yearlyMaxTWL > minElevation) && (minElevation >= 0.0))
///////                  {
///////                  m_pFloodedGrid->SetData(startRow, startCol, yearlyMaxTWL);
///////                  floodedCount++;
///////                  floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL); //, minElevation);
///////            //      m_pInletLUT->Set(4, inletRow, ++bayFloodedCount);                     
///////                  int dummybreak = 10;
///////                  }
///////               //   }
///////               } // end inner bay type 
///////            } // end bay inlet pt
///////         } // end bay inlet flooding 
///////      else if (beachType != BchT_UNDEFINED && beachType != BchT_RIVER)
///////         {
///////         m_pNewDuneLineLayer->GetData(point, m_colDuneCrest, duneCrest);
///////         //m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, xCoord);
///////         //m_pNewDuneLineLayer->GetData(point, m_colNorthingCrest, yCoord);
///////
///////           // use Northing location of Dune Toe and Easting location of the Dune Crest
///////         m_pOrigDuneLineLayer->GetPointCoords(point, xCoord, yCoord);
///////         m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, xCoord);
///////
///////         m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
///////         m_pNewDuneLineLayer->GetData(point, m_colYrFMaxTWL, yearlyMaxTWL);
///////         //yearlyMaxTWL -= 0.23f;
///////
///////         // Limit flooding to swl + setup for dune backed beaches (no swash)
///////         float swl = 0.0;
///////         m_pNewDuneLineLayer->GetData(point, m_colYrMaxSWL, swl);
///////         float eta = 0.0;
///////         m_pNewDuneLineLayer->GetData(point, m_colYrMaxSetup, eta);
///////         float STK_IG = 0.0;
///////
///////         yearlyMaxTWL = swl + 1.1f*(eta + STK_IG / 2.0f);
///////
///////         //if (duneIndx == 3191)
///////         //{
///////         // within grid ?
///////         if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
///////            {
///////            m_pElevationGrid->GetData(startRow, startCol, minElevation);
///////
///////            m_pFloodedGrid->GetData(startRow, startCol, flooded);
///////
///////            bool isFlooded = (flooded > 0.0f) ? true : false;
///////
///////            //   if (duneIndx == 1950 || duneIndx == 1951 || duneIndx == 1952 || duneIndx == 1953)
///////            //   {
///////            //      yearlyMaxTWL -= 0.49f;
///////            //   }
///////
///////               //if (duneIndx > 3020 && duneIndx < 3100)
///////               //{
///////               //   yearlyMaxTWL -= 0.49f;
///////               //}
///////               // for the cell corresponding to dune line coordinate
///////            if (!isFlooded && (yearlyMaxTWL > duneCrest) && (yearlyMaxTWL > minElevation) && (minElevation >= 0))
///////               {
///////               m_pFloodedGrid->SetData(startRow, startCol, yearlyMaxTWL);
///////               floodedCount++;
///////               //   floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL, minElevation);
///////               floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL);
///////               }
///////            }
///////         //}
///////         } // end shoreline pt
///////      } // end all duneline pts
///////
///////   m_pFloodedGrid->ResetBins();
///////   m_pFloodedGrid->AddBin(0, RGB(255, 255, 255), "Not Flooded", TYPE_FLOAT, -0.1f, 0.09f);
///////   m_pFloodedGrid->AddBin(0, RGB(0, 0, 255), "Flooded", TYPE_FLOAT, 0.1f, 100.0f);
///////   m_pFloodedGrid->ClassifyData();
///////   //  m_pFloodedGrid->Show();
///////   m_pFloodedGrid->Hide();
///////
///////   //clock_t start = clock();
///////
///////   // ***************    Lookup IDUs associated with FloodedGrid and set IDU attributes accordingly   ****************
///////   ComputeFloodedIDUStatistics();
///////
///////   //for (MapLayer::Iterator point = m_pBldgLayer->Begin(); point < m_pBldgLayer->End(); point++)
///////   //{
///////   //   double xCoord = 0.0f;
///////   //   double yCoord = 0.0f;
///////
///////   //   int flooded = 0;
///////
///////   //   int startRow = -1;
///////   //   int startCol = -1;
///////
///////   //   m_pBldgLayer->GetPointCoords(point, xCoord, yCoord);
///////   //   m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
///////   // 
///////   //   // within grid ?
///////   //   if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
///////   //   {
///////   //      m_pFloodedGrid->GetData(startRow, startCol, flooded);
///////   //      bool isFlooded = (flooded == 1) ? true : false;
///////
///////   //      if (isFlooded)
///////   //      {
///////   //         m_floodedBldgCount++;
///////   //         // m_pBldgLayer->SetData(point, m_colFlooded, flooded);
///////   //      }
///////   //   }
///////   //}
///////
///////   //for (int row = 0; row < m_numRows; row++)
///////   //{
///////   //   for (int col = 0; col < m_numCols; col++)
///////   //   {
///////   //      int flooded = 0;
///////   //      int noBuildings = 0;
///////   //      int size = m_pPolygonGridLkUp->GetPolyCntForGridPt(row, col);
///////   //      m_pFloodedGrid->GetData(row, col, flooded);
///////
///////   //      //
///////   //      bool isFlooded = (flooded == 1) ? true : false;
///////
///////   //      if (isFlooded)
///////   //      {
///////   //         // m_pBuildingGrid->GetData(row, col, buildings);
///////   //         int tempbreak = 10;
///////   //      }
///////   //      int *polyIndxs = new int[size];
///////   //      int polyCount = m_pPolygonGridLkUp->GetPolyNdxsForGridPt(row, col, polyIndxs);
///////   //      float *polyFractionalArea = new float[polyCount];
///////
///////   //      m_pPolygonGridLkUp->GetPolyProportionsForGridPt(row, col, polyFractionalArea);
///////
///////   //      for (int idu = 0; idu < polyCount; idu++)
///////   //      {
///////   //         m_floodFreq.AddValue((float)isFlooded);
///////   //         m_pIDULayer->SetData(polyIndxs[idu], m_colFlooded, (int)isFlooded);
///////   //         //   if (pEnvContext->yearOfRun >= m_avgTime)
///////   //         //{
///////   //         //m_pNewDuneLineLayer->SetData(polyIndxs[idu], m_colFloodFreq, m_floodFreq.GetValue() * m_avgTime);
///////   //         //}
///////
///////   //         //Number of buildings that are flooded
///////   //         //         m_pIDULayer->SetData(polyIndxs[idu], m_colnumFbuildings, ++noBuildings);
///////   //      }
///////
///////   //      delete[] polyIndxs;
///////   //   } //end columns of grid
///////   //} // end rows of grid
///////
///////
///////   //clock_t finish = clock();
///////   //float duration = (float)(finish - start) / CLOCKS_PER_SEC;
///////   //
///////   //msg.Format("ChronicHazards: Flooded grid generated in %i seconds", (int)duration);
///////   //Report::Log(msg);
///////   //
///////   floodedArea = m_cellWidth * m_cellHeight * floodedCount;
///////   return floodedArea;
///////
///////   }
///////
///////
///////// Recursively called method that visits a grid cells, eight neighboring grid cells and determines whether they are flooded
///////// returns the number of flooded grid cells
///////int ChronicHazards::VisitNeighbors(int row, int col, float twl, float &minElevation)
///////   {
///////   float noDataValue = m_pElevationGrid->GetNoDataValue();
///////   float flooded = 0.0f;
///////   int cellCount = 0;
///////   //float minElevation = 0.0f;
///////
///////   // Bathtub Flooding Model
///////   // walk in 8 directions from grid cell position, determine if TWL exceeds dune crest height and elevation 
///////   for (int direction = 0; direction < 8; direction++)
///////      {
///////      //int trow = row;
///////      //int tcol = col;
///////
///////      switch (direction)
///////         {
///////            case 0:  //NORTH
///////               row--;      //Step to the north one grid cell 
///////               break;
///////
///////            case 1:  //NORTHEAST
///////               row--;      //Step to the north one grid cell
///////               col++;      //Step to the east one grid cell
///////               break;
///////
///////            case 2:  //EAST
///////               col++;      //Step to the east one grid cell
///////               break;
///////
///////            case 3:  //SOUTHEAST
///////               row++;      //Step to the south one grid cell
///////               col++;      //Step to the east one grid cell
///////               break;
///////
///////            case 4:  //SOUTH
///////               row++;      //Step to the south one grid cell
///////               break;
///////
///////            case 5:  //SOUTHWEST
///////               row++;      //Step to the south one grid cell
///////               col--;      //Step to the west one grid cell
///////               break;
///////
///////            case 6:  //WEST
///////               col--;      //Step to the west one grid cell
///////               break;
///////
///////            case 7:  //NORTHWEST
///////               row--;      //Step to the north one grid cell
///////               col--;      //Step to the west one grid cell
///////               break;
///////         } // end switch
///////
///////         // get elevation, if value is OK and within grid, determine flooded
///////      if ((row >= 0 && row < m_numRows) && (col >= 0 && col < m_numCols))
///////         {
///////         //if ((row < 0 || row >= m_numRows) || (col < 0 || col >= m_numCols))
///////         //   return cellCount;
///////
///////            //if (minElevation == noDataValue)
///////            //   return cellCount;
///////
///////         m_pElevationGrid->GetData(row, col, minElevation);
///////         if (minElevation != noDataValue && minElevation >= 0)
///////            //if (minElevation >= 0)
///////            {
///////            m_pFloodedGrid->GetData(row, col, flooded);
///////
///////            bool isFlooded = (flooded > 0.0f) ? true : false;
///////
///////            if (!isFlooded && twl > minElevation)
///////               {
///////               cellCount++;
///////               m_pFloodedGrid->SetData(row, col, twl);
///////               cellCount += VisitNeighbors(row, col, twl, minElevation);
///////               }
///////            } // end good value
///////         } // valid location with elevation in grid
///////      } // end direction 
///////
///////   return cellCount;
///////
///////   }
//////////////
//////////////int ChronicHazards::VisitNeighbors(int row, int col, float twl)
///////   {
///////   float noDataValue = m_pElevationGrid->GetNoDataValue();
///////   float minElevation = 0.0f;
///////   int cellCount = 0;
///////   float flooded = 0.0f;
///////
///////   // Bathtub Flooding Model
///////   // walk in 8 directions from grid cell position, determine if TWL exceeds dune crest height and elevation 
///////   for (int direction = 0; direction < 8; direction++)
///////      {
///////      switch (direction)
///////         {
///////            case 0:  //NORTH
///////               row--;      //Step to the north one grid cell 
///////               break;
///////
///////               //case 1:  //NORTHEAST
///////               //   row--;      //Step to the north one grid cell
///////               //   col++;      //Step to the east one grid cell
///////               //   break;
///////
///////            case 2:  //EAST
///////               col++;      //Step to the east one grid cell
///////               break;
///////
///////               //case 3:  //SOUTHEAST
///////               //   row++;      //Step to the south one grid cell
///////               //   col++;      //Step to the east one grid cell
///////               //   break;
///////
///////            case 4:  //SOUTH
///////               row++;      //Step to the south one grid cell
///////               break;
///////
///////               //case 5:  //SOUTHWEST
///////               //   row++;      //Step to the south one grid cell
///////               //   col--;      //Step to the west one grid cell
///////               //   break;
///////
///////            case 6:  //WEST
///////               col--;      //Step to the west one grid cell
///////               break;
///////
///////               //case 7:  //NORTHWEST
///////               //   row--;      //Step to the north one grid cell
///////               //   col--;      //Step to the west one grid cell
///////               //   break;
///////         } // end switch
///////
///////           // get elevation, if value is OK and within grid, determine flooded
///////      if (row < 0 || row >= m_numRows || col < 0 || col >= m_numCols)
///////         return cellCount;
///////
///////      m_pElevationGrid->GetData(row, col, minElevation);
///////
///////      //if (twl < minElevation)
///////      //   return cellCount;
///////
///////      if (minElevation == noDataValue || minElevation <= 0)
///////         return cellCount;
///////
///////      m_pFloodedGrid->GetData(row, col, flooded);
///////      bool isFlooded = (flooded > 0.0f) ? true : false;
///////
///////      if (!isFlooded && twl > minElevation)
///////         {
///////         cellCount++;
///////         m_pFloodedGrid->SetData(row, col, twl);
///////         cellCount += VisitNeighbors(row, col, twl);
///////
///////         //cellCount += VisitNeighbors(row - 1, col, twl);         // NORTH            
///////         //cellCount += VisitNeighbors(row, col + 1, twl);         // EAST               
///////         //cellCount += VisitNeighbors(row + 1, col, twl);         // SOUTH            
///////         //cellCount += VisitNeighbors(row, col - 1, twl);         // WEST
///////
///////         //cellCount += VisitNeighbors(row - 1, col + 1, twl);      // NORTHEAST
///////         //cellCount += VisitNeighbors(row - 1, col - 1, twl);      // NORTHWEST
///////         //cellCount += VisitNeighbors(row + 1, col + 1, twl);      // SOUTHEAST
///////         //cellCount += VisitNeighbors(row + 1, col - 1, twl);      // SOUTHWEST
///////         } // end good value
///////      }
///////   return cellCount;
///////   }
//////////////
//////////////int ChronicHazards::VNeighbors(int row, int col, float twl)
///////   {
///////   float noDataValue = m_pElevationGrid->GetNoDataValue();
///////   float minElevation = 0.0f;
///////   int cellCount = 0;
///////   float flooded = 0.0f;
///////
///////   // Bathtub Flooding Model
///////   // walk in 8 directions from grid cell position, determine if TWL exceeds dune crest height and elevation 
///////   for (int direction = 0; direction < 8; direction++)
///////      {
///////      switch (direction)
///////         {
///////            case 0:  //NORTH
///////               row--;      //Step to the north one grid cell 
///////               break;
///////
///////            case 1:  //NORTHEAST
///////               row--;      //Step to the north one grid cell
///////               col++;      //Step to the east one grid cell
///////               break;
///////
///////            case 2:  //EAST
///////               col++;      //Step to the east one grid cell
///////               break;
///////
///////            case 3:  //SOUTHEAST
///////               row++;      //Step to the south one grid cell
///////               col++;      //Step to the east one grid cell
///////               break;
///////
///////            case 4:  //SOUTH
///////               row++;      //Step to the south one grid cell
///////               break;
///////
///////            case 5:  //SOUTHWEST
///////               row++;      //Step to the south one grid cell
///////               col--;      //Step to the west one grid cell
///////               break;
///////
///////            case 6:  //WEST
///////               col--;      //Step to the west one grid cell
///////               break;
///////
///////            case 7:  //NORTHWEST
///////               row--;      //Step to the north one grid cell
///////               col--;      //Step to the west one grid cell
///////               break;
///////         } // end switch
///////
///////      if ((row >= 0 && row < m_numRows) && (col >= 0 && col < m_numCols))
///////         {
///////         //if ((row < 0 || row >= m_numRows) || (col < 0 || col >= m_numCols))
///////         //return cellCount;
///////         //
///////         ///if (minElevation == noDataValue)
///////         //return cellCount;
///////
///////         m_pElevationGrid->GetData(row, col, minElevation);
///////
///////         if (minElevation != noDataValue && minElevation >= 0)
///////            //if (minElevation >= 0)
///////            {
///////            m_pFloodedGrid->GetData(row, col, flooded);
///////
///////            bool isFlooded = (flooded > 0.0f) ? true : false;
///////
///////            if (!isFlooded && twl > minElevation)
///////               {
///////               cellCount++;
///////               m_pFloodedGrid->SetData(row, col, twl);
///////               cellCount += VisitNeighbors(row, col, twl, minElevation);
///////               }
///////            } // end good value
///////         } // valid location with elevation in grid
///////      } // end direction 
///////
///////   return cellCount;
///////   }
///////

float ChronicHazards::GenerateErosionMap(int &erodedCount)
   {
   if (m_pErodedGrid == NULL)
      {
      m_pErodedGrid = m_pMap->CloneLayer(*m_pElevationGrid);
      m_pErodedGrid->m_name = "Eroded Grid";
      }
   m_pErodedGrid->SetAllData(0, false);

   // Determine the area of the grid that got Eroded


   // Integer that indicates 1 if Eroded and 0 if not Eroded
   int eroded = 0;

   // Counter of number of grid cells in Eroded grid that are Eroded 
   // used to determine area Eroded

   // area = grid cells that are Eroded multiplied by the number of grid cells 
   float ErodedArea = 0.0f;

   // clock_t start = clock();

   INT_PTR ptrArrayIndex = 0;

   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      REAL xCoord = 0.0;
      REAL yCoord = 0.0;

      REAL xStartCoord = 0.0;
      REAL yStartCoord = 0.0;

      int row = -1;
      int col = -1;
      int maxCol = -9999;

      float eventErosion = 0.0f;
      float shorelineChange = 0.0f;

      m_pNewDuneLineLayer->GetPointCoords(point, xCoord, yCoord);
      m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, row, col);

      m_pNewDuneLineLayer->GetData(point, m_colKD, eventErosion);
      float totalErosion = eventErosion + shorelineChange;
      //
      MovingWindow *eroKDMovingWindow = m_eroKDArray.GetAt(point);

      float eroKD2yrExtent = eroKDMovingWindow->GetAvgValue();
      REAL xAvgKD = xCoord + eroKD2yrExtent;

      m_pNewDuneLineLayer->SetData(point, m_colAvgKD, eroKD2yrExtent);

      REAL endXCoord = xCoord + totalErosion;

      while (xCoord < endXCoord)
         {
         if ((row >= 0 && row < m_numRows) && (col >= 0 && col < m_numCols))
            {
            m_pNewDuneLineLayer->SetData(point, m_colXAvgKD, xAvgKD);

            m_pErodedGrid->SetData(row, col, 1);
            erodedCount++;
            col++;
            m_pErodedGrid->GetGridCellCenter(row, col, xCoord, yCoord);
            }
         else
            xCoord = endXCoord;
         }

      m_pNewDuneLineLayer->GetPointCoords(point, xCoord, yCoord);
      m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, row, col);

      m_pOrigDuneLineLayer->GetPointCoords(point, xStartCoord, yCoord);
      while (xCoord > xStartCoord)
         {
         int eroded = 0;
         if ((row >= 0 && row < m_numRows) && (col >= 0 && col < m_numCols))
            {
            m_pErodedGrid->GetData(row, col, (int)eroded);
            bool isEroded = (eroded == 1) ? true : false;
            if (!isEroded)
               m_pErodedGrid->SetData(row, col, 2);
            col--;
            m_pErodedGrid->GetGridCellCenter(row, col, xCoord, yCoord);
            }
         else xCoord = xStartCoord;
         }
      ptrArrayIndex++;
      }

   m_pErodedGrid->ResetBins();
   m_pErodedGrid->AddBin(0, RGB(255, 255, 255), "Not Eroded", TYPE_FLOAT, -0.1f, 0.1f);
   m_pErodedGrid->AddBin(0, RGB(255, 255, 255), "Chronic Eroded", TYPE_FLOAT, 1.1f, 2.1f);
   m_pErodedGrid->AddBin(0, RGB(249, 192, 62), "Event Eroded", TYPE_FLOAT, 0.9f, 1.1f);
   m_pErodedGrid->ClassifyData();
   // m_pErodedGrid->Show();
   m_pErodedGrid->Hide();

   ComputeIDUStatistics();



   /*clock_t finish = clock();
   float duration = (float)(finish - start) / CLOCKS_PER_SEC;*/

   //msg.Format("ChronicHazards: Eroded grid generated in %i seconds", (int)duration);
   //Report::Log(msg);

   //   ErodedArea = ErodedCount * (cellWidth * cellHeight) ;
   return ErodedArea;
   }

void ChronicHazards::ComputeBuildingStatistics()
   {
   // determine how many new buildings were added
   int numNewBldgs = m_pBldgLayer->GetRowCount() - m_numBldgs;
   m_numBldgs += numNewBldgs;

   // create moving windows for new buildings
   for (int i = 0; i < numNewBldgs; i++)
      {
      MovingWindow *eroFreqMovingWindow = new MovingWindow(m_windowLengthEroHzrd);
      m_eroBldgFreqArray.Add(eroFreqMovingWindow);

      MovingWindow *floodFreqMovingWindow = new MovingWindow(m_windowLengthFloodHzrd);
      m_floodBldgFreqArray.Add(floodFreqMovingWindow);

      } // end add MovingWindows for new buildings

   if (m_pFloodedGrid != NULL)
      {
      for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
         {
         MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldg);

         REAL xCoord = 0.0;
         REAL yCoord = 0.0;

         float flooded = 0.0f;

         int startRow = -1;
         int startCol = -1;

         /*int bldgIndex = -1;
         m_pBldgLayer->GetData(bldg, m_colBldgDuneIndx, bldgIndex);*/

         int duCount = 0;
         m_pBldgLayer->GetData(bldg, m_colBldgNDU, duCount);

         bool isDeveloped = (duCount > 0) ? true : false;

         m_pBldgLayer->GetPointCoords(bldg, xCoord, yCoord);
         m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

         // within grid ?
         if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
            {
            //// Determine if building has been eroded by Bruun erosion
            //for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
            //{
            //   int duneBldgIndex = -1;
            //   m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, duneBldgIndex);

            //   CArray<int, int> iduIndices;

            //   if (bldg == duneBldgIndex)
            //   {
            //      double xDunePt = 0.0f;
            //      double yDunePt = 0.0f;
            //      m_pNewDuneLineLayer->GetPointCoords(point, xDunePt, yDunePt);
            //      if (xDunePt >= xCoord)
            //      {
            //         m_pBldgLayer->SetData(bldg, m_colBldgEroded, 1);

            //         int sz = m_pIduBuildingLkup->GetPolysFromPointIndex(bldg, iduIndices);
            //         for (int i = 0; i < sz; i++)
            //         {
            //            int iduIndex = iduIndices[ 0 ];
            //            /*m_pIDULayer->SetData(iduIndex, m_colPopDensity, 0);
            //            m_pIDULayer->SetData(iduIndex, m_colPopCap, 0);
            //            m_pIDULayer->SetData(iduIndex, m_colPopulation, 0);
            //            m_pIDULayer->SetData(iduIndex, m_colLandValue, 0);
            //            m_pIDULayer->SetData(iduIndex, m_colImpValue, 0);*/
            //         }
            //      }
            //   }
            //} // end DuneLine iterate

            m_pFloodedGrid->GetData(startRow, startCol, flooded);
            bool isFlooded = (flooded > 0.0f) ? true : false;
            int removeYr = 0;
            m_pBldgLayer->GetData(bldg, m_colBldgRemoveYr, removeYr);

            if (isFlooded && removeYr == 0)
               {
               if (isDeveloped)
                  m_floodedBldgCount += duCount;
               else
                  m_floodedBldgCount++;

               m_pBldgLayer->SetData(bldg, m_colBldgFlooded, 1);
               floodMovingWindow->AddValue(1.0f);
               float floodFreq = floodMovingWindow->GetFreqValue();
               m_pBldgLayer->SetData(bldg, m_colBldgFloodFreq, floodFreq);

               }
            }
         }

      } // end FloodedGrid

        // if Building is eroded it is flagged and removed from future counts
   if (m_pErodedGrid != NULL)
      {
      //INT_PTR ptrArrayIndex = 0;

      for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
         {
         MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(bldg);
         //   ptrArrayIndex++;

         REAL xCoord = 0.0;
         REAL yCoord = 0.0;

         int eroded = 0;
         int erodedBldg = 0;

         int startRow = -1;
         int startCol = -1;

         int duCount = 0;
         m_pBldgLayer->GetData(bldg, m_colBldgNDU, duCount);

         bool isDeveloped = (duCount > 0) ? true : false;

         // need to get all bldg from column before
         m_pBldgLayer->GetPointCoords(bldg, xCoord, yCoord);
         m_pErodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

         // has building previously eroded by chronic erosion
         m_pBldgLayer->GetData(bldg, m_colBldgEroded, erodedBldg);

         // reset event eroded buildings
         if (erodedBldg > 0)
            m_pBldgLayer->SetData(bldg, m_colBldgEroded, 0);

         // within grid ?
         if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
            {
            m_pErodedGrid->GetData(startRow, startCol, (int)eroded);

            bool isEventEroded = (eroded == 1) ? true : false;
            bool isChronicEroded = (eroded == 2) ? true : false;

            // Determine if building has been eroded by event erosion 
            // Tally buildings eroded by total erosion 
            int removeYr = 0;
            m_pBldgLayer->GetData(bldg, m_colBldgRemoveYr, removeYr);
            if (isEventEroded && removeYr == 0)
               {
               float eroFreq = 0.0f;
               eroMovingWindow->AddValue(1.0f);
               m_pBldgLayer->SetData(bldg, m_colBldgEroded, 1);
               //   m_pBldgLayer->GetData(bldg, m_colBldgEroFreq, eroFreq);
               //   m_pBldgLayer->SetData(bldg, m_colBldgEroFreq, ++eroFreq);
               //   if (isDeveloped)
               //      m_erodedBldgCount += duCount;
               //   else
               //      m_erodedBldgCount++;
               }

            if (isChronicEroded && removeYr == 0)
               {
               m_pBldgLayer->SetData(bldg, m_colBldgEroded, 2);
               //   m_noBldgsEroded++;
               }

            //if (isBldgEroded)
            //{
            //   /*if (isDeveloped)
            //      m_erodedBldgCount += duCount;
            //   else
            //      m_erodedBldgCount++;*/
            //}
            }
         }
      } // end erodedGrid
   } // end ComputeBuildingStatistics


void ChronicHazards::TallyCostStatistics(int currentYear)
   {
   int cols = 1 + ((PI_COUNT + 1) * 7);
   ASSERT(cols == m_policyCostData.GetColCount());

   CArray<float, float> data;
   data.SetSize(cols);

   data[0] = (float)currentYear;

   int col = 1;

   float totals[7];
   for (int i = 0; i < 7; i++)
      totals[i] = 0;

   for (int i = 0; i < PI_COUNT; i++)
      {
      PolicyInfo &pi = m_policyInfoArray[i];
      pi.m_movAvgBudgetSpent.AddValue(pi.m_spentBudget);
      pi.m_movAvgUnmetDemand.AddValue(pi.m_unmetDemand);
      data[col++] = pi.m_startingBudget;
      data[col++] = pi.m_availableBudget;
      data[col++] = pi.m_spentBudget;
      data[col++] = pi.m_unmetDemand;
      data[col++] = pi.m_movAvgUnmetDemand.GetAvgValue();
      data[col++] = pi.m_movAvgBudgetSpent.GetAvgValue();
      data[col++] = pi.m_allocFrac;

      totals[0] += pi.m_startingBudget;
      totals[1] += pi.m_availableBudget;
      totals[2] += pi.m_spentBudget;
      totals[3] += pi.m_unmetDemand;
      totals[4] += pi.m_movAvgUnmetDemand.GetAvgValue();
      totals[5] += pi.m_movAvgBudgetSpent.GetAvgValue();
      totals[6] += pi.m_allocFrac;
      }

   data[col++] = totals[0];
   data[col++] = totals[1];
   data[col++] = totals[2];
   data[col++] = totals[3];
   data[col++] = totals[4];
   data[col++] = totals[5];
   data[col++] = totals[6];

   m_policyCostData.AppendRow(data);
   }


void ChronicHazards::TallyDuneStatistics(int currentYear)
   {
   //if (m_pFloodedGrid != NULL)
   //{
   //   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
   //   {
   //      double xCoord = 0.0f;
   //      double yCoord = 0.0f;

   //      int flooded = 0;

   //      int startRow = -1;
   //      int startCol = -1;

   //      m_pNewDuneLineLayer->GetPointCoords(point, xCoord, yCoord);
   //      m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

   //      // within grid ?
   //      if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
   //      {
   //         m_pFloodedGrid->GetData(startRow, startCol, flooded);
   //         bool isFlooded = (flooded == 1) ? true : false;

   //         if (isFlooded)
   //         {
   //            m_pNewDuneLineLayer->SetData(point, m_colFlooded, flooded);
   //         }
   //      }
   //   }
   //}

   int n = 0;
   int n_old = 0;

   int m = 0;
   int m_old = 0;

   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      int beachType = -1;
      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);

      if (beachType == BchT_RIPRAP_BACKED)
         {
         int yrAdded = 0;
         m_pNewDuneLineLayer->GetData(point, m_colAddYearBPS, yrAdded);
         // count added this year
         if (yrAdded == currentYear)
            n++;
         else
            // all the rest including existing BPS
            n_old++;
         }
      // count constructed dunes
      else if (beachType == BchT_SANDY_DUNE_BACKED)
         {
         int yrAdded = 0;
         m_pNewDuneLineLayer->GetData(point, m_colAddYearSPS, yrAdded);
         // count added this year
         if (yrAdded == currentYear)
            m++;
         else if (yrAdded != 0)
            // all the rest of constructed dunes, does not include existing SDB beaches
            m_old++;
         }
      }

   // determine length of BPS structures
   m_addedHardenedShoreline = m_cellHeight * n;
   m_totalHardenedShoreline = m_addedHardenedShoreline + (m_cellHeight * n_old);
   m_percentHardenedShoreline = (m_totalHardenedShoreline / float(m_shorelineLength)) * 100;

   m_addedHardenedShorelineMiles = m_addedHardenedShoreline * MILESPERMETER;
   m_totalHardenedShorelineMiles = m_totalHardenedShoreline * MILESPERMETER;
   m_percentHardenedShorelineMiles = m_percentHardenedShoreline * MILESPERMETER;

   // determine length of constructed dunes
   m_addedRestoredShoreline = m_cellHeight * m;
   m_totalRestoredShoreline = m_addedRestoredShoreline + (m_cellHeight * m_old);
   m_percentRestoredShoreline = (m_totalRestoredShoreline / float(m_shorelineLength)) * 100;

   m_addedRestoredShorelineMiles = m_addedRestoredShoreline * MILESPERMETER;
   m_totalRestoredShorelineMiles = m_totalRestoredShoreline * MILESPERMETER;
   m_percentRestoredShorelineMiles = m_percentRestoredShoreline * MILESPERMETER;

   }

void ChronicHazards::ComputeFloodedIDUStatistics()
   {
   m_pIDULayer->m_readOnly = false;

   if (m_pFloodedGrid != NULL)
      {
      for (int idu = 0; idu < m_pPolygonGridLkUp->GetNumPolys(); idu++)
         {
         float fracFlooded = 0.0;
         int count = m_pPolygonGridLkUp->GetGridPtCntForPoly(idu);

         if (count > 0)
            {
            ROW_COL *indexes = new ROW_COL[count];
            float *values = new float[count];

            m_pPolygonGridLkUp->GetGridPtNdxsForPoly(idu, indexes);
            m_pPolygonGridLkUp->GetGridPtProportionsForPoly(idu, values);

            for (int i = 0; i < count; i++)
               {

               int startRow = indexes[i].row;
               int startCol = indexes[i].col;

               if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
                  {

                  float flooded = 0.0;
                  m_pFloodedGrid->GetData(startRow, startCol, flooded);

                  bool isFlooded = (flooded > 0.0f) ? true : false;

                  if (isFlooded)
                     {
                     fracFlooded += values[i];
                     }
                  }
               }

            m_pIDULayer->SetData(idu, m_colFracFlooded, fracFlooded);

            delete[] indexes;
            delete[] values;
            }
         }
      }
   m_pIDULayer->m_readOnly = true;

   }  // end ComputeFloodedIDUStatistics

void ChronicHazards::ComputeIDUStatistics()
   {
   m_pIDULayer->m_readOnly = false;

   // Lookup IDUs associated with ErodedGrid and set IDU attributes accordingly
   for (int row = 0; row < m_numRows; row++)
      {
      for (int col = 0; col < m_numCols; col++)
         {
         int eroded = 0;
         int noBuildings = 0;

         int size = m_pPolygonGridLkUp->GetPolyCntForGridPt(row, col);

         m_pErodedGrid->GetData(row, col, (int)eroded);
         bool isIDUEroded = (eroded == 2) ? true : false;

         if (isIDUEroded)
            {
            // m_pBuildingGrid->GetData(row, col, buildings);

            int *polyIndxs = new int[size];
            int polyCount = m_pPolygonGridLkUp->GetPolyNdxsForGridPt(row, col, polyIndxs);
            float *polyFractionalArea = new float[polyCount];

            m_pPolygonGridLkUp->GetPolyProportionsForGridPt(row, col, polyFractionalArea);
            for (int idu = 0; idu < polyCount; idu++)
               {
               m_pIDULayer->SetData(polyIndxs[idu], m_colIDUEroded, eroded);
               m_pIDULayer->SetData(polyIndxs[idu], m_colPopDensity, 0);
               }
            delete[] polyIndxs;
            delete[] polyFractionalArea;
            }

         } //end columns of grid
      } // end rows of 

   m_pIDULayer->m_readOnly = true;
   }

void ChronicHazards::TallyRoadStatistics()
   {
   if (m_pRoadLayer != NULL)
      {

      /************************   Calculate Flooded Road Statistics ************************/

      if (m_pFloodedGrid != NULL)
         {
         float noDataValue = m_pFloodedGrid->GetNoDataValue();

         for (int row = 0; row < m_numRows; row++)
            {
            for (int col = 0; col < m_numCols; col++)
               {

               int polyCount = m_pPolylineGridLkUp->GetPolyCntForGridPt(row, col);

               /*if (polyCount > 0)
               {*/
               int *indices = new int[polyCount];
               int count = m_pPolylineGridLkUp->GetPolyNdxsForGridPt(row, col, indices);

               /*if (count > 0)
               {*/
               for (int i = 0; i < polyCount; i++)
                  {
                  REAL xCenter = 0.0;
                  REAL yCenter = 0.0;

                  COORD2d ll;
                  COORD2d ur;
                  m_pFloodedGrid->GetGridCellRect(row, col, ll, ur);


                  m_pFloodedGrid->GetGridCellCenter(row, col, xCenter, yCenter);
                  REAL xMin0 = xCenter - (m_cellWidth / 2.0);
                  REAL yMin0 = yCenter - (m_cellHeight / 2.0);
                  REAL xMax0 = xCenter + (m_cellWidth / 2.0);
                  REAL yMax0 = yCenter + (m_cellHeight / 2.0);

                  int roadType = -1;
                  Poly *pPoly = m_pRoadLayer->GetPolygon(indices[i]);
                  m_pRoadLayer->GetData(indices[i], m_colRoadType, roadType);

                  float flooded = 0.0f;
                  m_pFloodedGrid->GetData(row, col, flooded);
                  bool isFlooded = (flooded > 0.0f) ? true : false;

                  if (isFlooded && flooded != noDataValue)
                     {

                     if (roadType != 6)
                        {
                        if (roadType == 7)
                           {
                           double lengthOfRailroad = 0.0;
                           lengthOfRailroad = DistancePolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), xMin0, yMin0, xMax0, yMax0);
                           m_floodedRailroadMiles += float(lengthOfRailroad);
                           m_pRoadLayer->SetData(indices[i], m_colRoadFlooded, 1);

                           }
                        else if (roadType != 5)
                           {
                           double lengthOfRoad = 0.0;
                           lengthOfRoad = DistancePolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), ll.x, ll.y, ur.x, ur.y);
                           /*if (lengthOfRoad > 0 && m_debugOn)
                           {
                           CString thisMsg;
                           thisMsg.Format("Grid Cell: row=%i,  col==%i, xMin0=%7.3f,yMin0=%7.3f,xMax0=%7.3f,yMax0=%7.3f,",row,col,xMin0,yMin0,xMax0,yMax0);
                           Report::Log(thisMsg);
                           }*/
                           m_floodedRoad += float(lengthOfRoad);
                           m_pRoadLayer->SetData(indices[i], m_colRoadFlooded, 1);

                           }
                        }
                     }
                  //      }

                  //   }
                  }
               delete[] indices;
               }
            }
         }
      // meters to miles: m *0.00062137 = miles
      m_floodedRoadMiles = m_floodedRoad * MILESPERMETER;
      m_floodedRailroadMiles *= MILESPERMETER;


      /************************   Calculate Eroded Road Statistics ************************/

      if (m_pErodedGrid != NULL)
         {
         for (int row = 0; row < m_numRows; row++)
            {
            for (int col = 0; col < m_numCols; col++)
               {
               int eroded = 0;
               m_pErodedGrid->GetData(row, col, (int)eroded);
               bool isEroded = (eroded == 2) ? true : false;

               if (isEroded)
                  {
                  int polyCount = m_pPolylineGridLkUp->GetPolyCntForGridPt(row, col);

                  if (polyCount > 0)
                     {
                     int *indices = new int[polyCount];
                     int count = m_pPolylineGridLkUp->GetPolyNdxsForGridPt(row, col, indices);

                     double lengthOfRoad = 0.0;
                     //   double lengthOfRailroad = 0.0;

                     REAL xCenter = 0.0;
                     REAL yCenter = 0.0;

                     m_pErodedGrid->GetGridCellCenter(row, col, xCenter, yCenter);
                     REAL xMin0 = xCenter - (m_cellWidth / 2.0);
                     REAL yMin0 = yCenter - (m_cellHeight / 2.0);
                     REAL xMax0 = xCenter + (m_cellWidth / 2.0);
                     REAL yMax0 = yCenter + (m_cellHeight / 2.0);

                     for (int i = 0; i < polyCount; i++)
                        {
                        int roadType = -1;
                        Poly *pPoly = m_pRoadLayer->GetPolygon(indices[i]);
                        m_pRoadLayer->GetData(indices[i], m_colRoadType, roadType);

                        /*   if (roadType == 7)
                        {
                        lengthOfRailroad = DistancePolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), xMin0, yMin0, xMax0, yMax0);
                        m_erodedRailroad += float(lengthOfRailroad);
                        }
                        else
                        {*/
                        lengthOfRoad = DistancePolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), xMin0, yMin0, xMax0, yMax0);
                        m_erodedRoad += float(lengthOfRoad);
                        //   }
                        }
                     delete[] indices;
                     }
                  }
               }
            }
         }
      // meters to miles: m *0.00062137 = miles
      m_erodedRoadMiles = m_erodedRoad * MILESPERMETER;
      //   m_erodedRailroadMiles = m_erodedRailroad * MILESPERMETER;
      }  // end RoadLayer exists

   }

void ChronicHazards::ComputeInfraStatistics()
   {

   /************************   Calculate Flooded Infrastructure Statistics ************************/


   if (m_pFloodedGrid != NULL)
      {
      for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
         {
         REAL xCoord = 0.0;
         REAL yCoord = 0.0;

         float flooded = 0.0f;

         int startRow = -1;
         int startCol = -1;

         int infraIndex = -1;
         m_pInfraLayer->GetData(infra, m_colInfraDuneIndx, infraIndex);

         int duCount = 0;
         /*m_pInfraLayer->GetData(point, m_colInfraCount, duCount);*/

         //bool isDeveloped = (duCount > 0) ? true : false;

         m_pInfraLayer->GetPointCoords(infra, xCoord, yCoord);
         m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

         MovingWindow *floodMovingWindow = m_floodInfraFreqArray.GetAt(infra);

         // within grid ?
         if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
            {
            m_pFloodedGrid->GetData(startRow, startCol, flooded);
            bool isFlooded = (flooded > 0.0f) ? true : false;

            if (isFlooded)
               {
               /*if (isDeveloped)
               m_floodedInfraCount += duCount;
               else*/
               m_floodedInfraCount++;
               floodMovingWindow->AddValue(1);

               m_pInfraLayer->SetData(infra, m_colInfraFlooded, flooded);
               }

            // Determine if infrastructure has been eroded by Bruun erosion
            for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
               {
               int duneIndex = -1;
               m_pNewDuneLineLayer->GetData(point, m_colDuneIndx, duneIndex);

               if (infraIndex == duneIndex)
                  {
                  REAL xDunePt = 0.0;
                  REAL yDunePt = 0.0;
                  m_pNewDuneLineLayer->GetPointCoords(point, xDunePt, yDunePt);
                  if (xDunePt >= xCoord)
                     {
                     m_pInfraLayer->SetData(infra, m_colInfraEroded, 1);
                     }
                  }
               } // end DuneLine iterate
            }
         }
      }

   /************************   Calculate Eroded Infrastructure Statistics ************************/

   if (m_pErodedGrid != NULL)
      {
      for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
         {
         REAL xCoord = 0.0;
         REAL yCoord = 0.0;

         int eroded = 0;
         int erodedInfra = 0;

         int startRow = -1;
         int startCol = -1;

         int infraIndex = -1;
         m_pBldgLayer->GetData(infra, m_colInfraDuneIndx, infraIndex);

         MovingWindow *eroMovingWindow = m_eroInfraFreqArray.GetAt(infra);

         // need to get all points from column before
         m_pInfraLayer->GetPointCoords(infra, xCoord, yCoord);
         m_pErodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

         // has building infrastructure been previously eroded
         m_pInfraLayer->GetData(infra, m_colInfraEroded, erodedInfra);

         bool isInfraEroded = (erodedInfra == 1) ? true : false;

         // within grid ?
         if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
            {
            m_pErodedGrid->GetData(startRow, startCol, eroded);

            bool isEEroded = (eroded == 1) ? true : false;
            bool isdoublelyEroded = (eroded == 2) ? true : false;

            if (isEEroded) // && 
               {
               eroMovingWindow->AddValue(1.0f);
               m_pInfraLayer->SetData(infra, m_colInfraEroded, isInfraEroded);
               if (!isInfraEroded)
                  int tempbreak = 10;
               //m_erodedInfraCount++;
               }
            }
         }
      }
   }

void ChronicHazards::ExportMapLayers(EnvContext *pEnvContext, int outputYear)
   {
   /*int outputYear;

   outputYear = pEnvContext->currentYear;*/

   if (m_exportMapInterval != -1 && ((pEnvContext->yearOfRun % m_exportMapInterval == 0) || (outputYear == pEnvContext->endYear)))
      {
      CPath outputPath(PathManager::GetPath(PM_OUTPUT_DIR));

      CPath duneShapeFilePath = outputPath;
      CPath duneCSVFilePath = outputPath;

      CPath duneOCShapeFilePath = outputPath;

      CPath bldgShapeFilePath = outputPath;
      //CPath bldgCSVFilePath = outputPath;
      //CString csvFolder = _T("\\MapCsvs\\");

      CString duneFilename;
      CString bldgFilename;
      CString floodedGridFile;
      CString cumFloodedGridFile;
      CString erodedGridFile;
      CString eelgrassGridFile;

      CString scenario = pEnvContext->pEnvModel->m_pScenario->m_name;
      int run = pEnvContext->run;

      //int outputYear;
      //
      //outputYear = pEnvContext->currentYear;

      /*if(pEnvContext->yearOfRun == 0)
      outputYear = pEnvContext->currentYear;
      else
      outputYear = pEnvContext->currentYear + 1;*/

      if (pEnvContext->yearOfRun == 0)
         {
         // remove all  .shp .dbf .prj .shx .flt
         }

      /*CString duneOceanShoresFilename;
      duneOceanShoresFilename.Format("BackshoreOS_Year%i_%s_Run%i", outputYear, scenario, run);

      CString ocQuery = "LOC = 1";

      QueryEngine qe(m_pNewDuneLineLayer);

      Query *pQuery = qe.ParseQuery(ocQuery, 0, "OC Query");

      qe.SelectQuery(pQuery, false); */

      //   /*if (qe != NULL && ocQuery.GetLength() > 0)
      //   {
      //      CString source(_T("Ocean Shores Query"));

      //      pQuery = qe->ParseQuery(ocQuery, 0, source);

      //      if (pQuery == NULL)
      //      {
      //         CString qmsg(_T("ChronicHazards: Error parsing dune layer query: '"));
      //         qmsg += ocQuery;
      //         qmsg += _T("' - the query will be ignored");
      //         Report::ErrorMsg(qmsg);
      //      }
      //   }*/

      ////   CString msg("ChronicHazards:: Exporting map layer: ");
      /*CString duneOCShapeFile = duneOceanShoresFilename + ".shp";
      duneOCShapeFilePath.Append(duneOCShapeFile);
      m_pNewDuneLineLayer->SaveShapeFile(duneOCShapeFilePath, true);


      CString duneWestportFilename;
      duneWestportFilename.Format("BackshoreWP_Year%i_%s_Run%i", outputYear, scenario, run);*/

      // Dune line layer naming
      duneFilename.Format("Backshore_Year%i_%s_Run%i", outputYear, scenario, run);

      CString msg("ChronicHazards:: Exporting map layer: ");
      CString duneShapeFile = duneFilename + ".shp";
      duneShapeFilePath.Append(duneShapeFile);
      int ok = m_pNewDuneLineLayer->SaveShapeFile(duneShapeFilePath, false, 0, 1);

      if (ok > 0)
         {
         msg += duneShapeFilePath;
         Report::Log(msg);
         }

      //      CString duneCSVFile = duneFilename + ".csv";
      //      duneCSVFilePath.Append(csvFolder);
      //      // make sure directory exists
      //#ifndef NO_MFC
      //      SHCreateDirectoryEx(NULL, duneCSVFilePath, NULL);
      //#else
      //      mkdir(duneCSVFilePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      //#endif
      //      duneCSVFilePath.Append(duneCSVFile);
      //      ok = m_pNewDuneLineLayer->SaveDataAscii(duneCSVFilePath, false, true);
      //      ok = 1;
      //      if (ok > 0)
      //         {
      //         msg.Replace(duneShapeFilePath, duneCSVFilePath);
      //         Report::Log(msg);
      //         }
      //
      // Buildings layer naming
      bldgFilename.Format("Buildings_Year%i_%s_Run%i", outputYear, scenario, run);

      CString bldgShapeFile = bldgFilename + ".shp";
      bldgShapeFilePath.Append(bldgShapeFile);
      ok = m_pBldgLayer->SaveShapeFile(bldgShapeFilePath, false, 0, 1);

      if (ok > 0)
         {
         msg.Replace(duneCSVFilePath, bldgShapeFilePath);
         Report::Log(msg);
         }

      //CString bldgCSVFile = bldgFilename + ".csv";
      //bldgCSVFilePath.Append(csvFolder);
      // make sure directory exists
#ifndef NO_MFC
      //SHCreateDirectoryEx(NULL, bldgCSVFilePath, NULL);
#else
      //mkdir(bldgCSVFilePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
      //bldgCSVFilePath.Append(bldgCSVFile);
      //ok = m_pBldgLayer->SaveDataAscii(bldgCSVFilePath, false, true);
      //ok = 1;
      //if (ok > 0)
      //   {
      //   msg.Replace(bldgShapeFilePath, bldgCSVFilePath);
      //   Report::Log(msg);
      //   }

      if (pEnvContext->yearOfRun > 0 || (outputYear == pEnvContext->endYear))
         {
         // write out the flooded grids at the chosen export interval
         CPath floodedGridPath = outputPath;
         //     if (m_writeGridFiles)
         floodedGridFile.Format("Flooded_Year%i_%s_Run%i", outputYear, scenario, run);
         CString gridFilename = floodedGridFile + ".flt";

         msg.Replace(bldgFilename, gridFilename);
         Report::Log(msg);

         floodedGridPath.Append(gridFilename);
         m_pFloodedGrid->SaveGridFile(floodedGridPath);

         // cumulative flooded grid as well
         CPath cumFloodedGridPath = outputPath;
         //     if (m_writeGridFiles)
         cumFloodedGridFile.Format("Cumulative_Flooded_Year%i_%s_Run%i", outputYear, scenario, run);
         CString cumGridFilename = cumFloodedGridFile + ".flt";

         msg.Replace(gridFilename, cumGridFilename);
         Report::Log(msg);

         cumFloodedGridPath.Append(cumGridFilename);
         m_pCumFloodedGrid->SaveGridFile(cumFloodedGridPath);


         // write out the eelgrass grids at the chosen export interval
         bool runEelgrassModel = (m_runEelgrassModel == 1) ? true : false;
         if (runEelgrassModel)
            {
            CPath eelgrassGridPath = outputPath;
            //     if (m_writeGridFiles)
            eelgrassGridFile.Format("Eelgrass_Year%i_%s_Run%i", outputYear, scenario, run);
            CString eelgridFilename = eelgrassGridFile + ".flt";

            msg.Replace(gridFilename, eelgridFilename);
            Report::Log(msg);

            eelgrassGridPath.Append(eelgridFilename);
            m_pEelgrassGrid->SaveGridFile(eelgrassGridPath);
            }
         }

      //if (m_runBayFloodModel == 1 && m_debugOn)
      //{
      //   if (pEnvContext->yearOfRun > 0 || (outputYear == pEnvContext->endYear))
      //   {
      //      CString floodedBayPtsFile;
      //      CPath floodedBayPtsPath = outputPath;
      //      floodedBayPtsFile.Format("FloodedBayPts_Year%i_%s_Run%i", outputYear, scenario, run);
      //      CString floodedBayPtsFilename = floodedBayPtsFile + ".csv";
      //      floodedBayPtsPath.Append(floodedBayPtsFilename);
      //      m_pInletLUT->WriteAscii(floodedBayPtsPath);

      //      //m_pElevationGrid->SaveGridFile("C:\\Envision\\StudyAreas\\GraysHarbor\\Outputs\\ghc_dem.flt");
      //      CString debugMsg("ChronicHazards:: Exporting map layer: ");
      //      debugMsg += floodedBayPtsFilename;
      //      Report::Log(debugMsg);
      //   }
      //}

      }

   } // end ExportMapLayers(EnvContext *pEnvContext)

void ChronicHazards::RunPolicies(EnvContext *pEnvContext)
   {
   FindProtectedBldgs();

   // Construct BPS if desired policy
   bool runConstructBPSPolicy = (m_runConstructBPSPolicy == 1) ? true : false;
   // Construct hard structures for protection if requested
   if (runConstructBPSPolicy && (pEnvContext->yearOfRun >= m_windowLengthTWL - 1))
      ConstructBPS(pEnvContext->currentYear);

   // Maintain BPS if desired policy
   bool runMaintainBPSPolicy = (m_runMaintainBPSPolicy == 1) ? true : false;
   // Construct hard structures for protection if requested
   if (runMaintainBPSPolicy) // && (pEnvContext->yearOfRun >= m_windowLengthTWL - 1))
      MaintainBPS(pEnvContext->currentYear);

   // Nourish beaches with BPS if desired policy
   bool runNourishBPSPolicy = (m_runNourishByTypePolicy == 1) ? true : false;
   if (runNourishBPSPolicy && (pEnvContext->yearOfRun >= m_windowLengthTWL))
      NourishBPS(pEnvContext->currentYear, true);

   // Rebuild dunes on shoreline if desired policy
   bool runConstructSPSPolicy = (m_runConstructSPSPolicy == 1) ? true : false;
   if (runConstructSPSPolicy && (pEnvContext->yearOfRun >= m_windowLengthTWL - 1))
      ConstructSPS(pEnvContext->currentYear);

   // Maintain SPS if desired policy
   bool runMaintainSPSPolicy = (m_runMaintainSPSPolicy == 1) ? true : false;
   // Construct hard structures for protection if requested
   if (runMaintainSPSPolicy && (pEnvContext->yearOfRun >= m_windowLengthTWL))
      MaintainSPS(pEnvContext->currentYear);

   // Nourish beaches with constructed dunes if desired policy
   bool runNourishSPSPolicy = (m_runNourishSPSPolicy == 1) ? true : false;
   if (runNourishSPSPolicy && (pEnvContext->yearOfRun >= m_windowLengthTWL))
      NourishSPS(pEnvContext->currentYear);

   // Raise existing bldgs and critical infrastructure to BFE
   // Relocate existing bldgs to safest site (highest elevation) 
   bool runRelocateSafestPolicy = (m_runRelocateSafestPolicy == 1) ? true : false;
   if (runRelocateSafestPolicy) //  && (pEnvContext->yearOfRun >= m_windowLengthFloodHzrd - 1))
      RaiseOrRelocateBldgToSafestSite(pEnvContext);

   //bool runRaiseInfrastructurePolicy = (m_runRaiseInfrastructurePolicy == 1) ? true : false;
   //if (runRaiseInfrastructurePolicy) //  && (pEnvContext->yearOfRun >= m_windowLengthFloodHzrd - 1))
   RaiseInfrastructure(pEnvContext);   // alway run this to account for infrastrucutre flooding

                                       // Remove homes from hazard zone if desired policy
   bool runRemoveBldgFrHazardZonePolicy = (m_runRemoveBldgFromHazardZonePolicy == 1) ? true : false;
   if (runRemoveBldgFrHazardZonePolicy && (pEnvContext->yearOfRun >= m_windowLengthFloodHzrd - 1))
      RemoveBldgFromHazardZone(pEnvContext);

   bool runRemoveInfraFrHazardZonePolicy = (m_runRemoveInfraFromHazardZonePolicy == 1) ? true : false;
   if (runRemoveInfraFrHazardZonePolicy && (pEnvContext->yearOfRun >= m_windowLengthFloodHzrd - 1))
      RemoveInfraFromHazardZone(pEnvContext);
   } // end RunPolicies(EnvContext *pEnvContext)


     // Determine if
bool ChronicHazards::FindProtectedBldgs()
   {
   // For each building get it's IDU
   // Then find the set of dunePts within the Northing extents of that IDU
   // Finally assign the closest building to those dunePts

   for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
      {
      REAL xBldg = 0.0;
      REAL yBldg = 0.0;
      m_pBldgLayer->GetPointCoords(bldg, xBldg, yBldg);

      CArray<int, int> iduIndices;
      int iduIndex = -1;
      int tsunami_hz = -1;

      // Get IDU of building if in Tsunamic Hazard Zone
      int sz = m_pIduBuildingLkup->GetPolysFromPointIndex(bldg, iduIndices);
      for (int i = 0; i < sz; i++)
         {
         m_pIDULayer->GetData(iduIndices[0], m_colTsunamiHazardZone, tsunami_hz);
         bool isTsunamiHzrd = (tsunami_hz == 1) ? true : false;
         if (isTsunamiHzrd)
            iduIndex = iduIndices[0];
         }

      m_pBldgLayer->SetData(bldg, m_colBldgIDUIndx, iduIndex);

      double northingTop = 0.0;
      double northingBtm = 0.0;

      if (iduIndex != -1)
         {
         m_pIDULayer->GetData(iduIndex, m_colNorthingTop, northingTop);
         m_pIDULayer->GetData(iduIndex, m_colNorthingBottom, northingBtm);
         }

      // Determine what building Dune Pts are protecting 
      for (MapLayer::Iterator dunePt = m_pNewDuneLineLayer->Begin(); dunePt < m_pNewDuneLineLayer->End(); dunePt++)
         {
         int beachType = -1;
         m_pNewDuneLineLayer->GetData(dunePt, m_colBeachType, beachType);

         int tst = dunePt;

         // only assign 
         if (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED)
            {
            double northing = 0.0;
            m_pNewDuneLineLayer->GetData(dunePt, m_colNorthingToe, northing);

            int duneIndx = -1;
            m_pNewDuneLineLayer->GetData(dunePt, m_colDuneIndx, duneIndx);

            REAL xDunePt = 0.0;
            REAL yDunePt = 0.0;
            m_pNewDuneLineLayer->GetPointCoords(dunePt, xDunePt, yDunePt);

            if (northing < northingTop && northing > northingBtm)
               {
               float distance = (float)sqrt((xBldg - xDunePt) * (xBldg - xDunePt) + (yBldg - yDunePt) * (yBldg - yDunePt));

               int bldgIndex = -1;
               m_pNewDuneLineLayer->GetData(dunePt, m_colDuneBldgIndx, bldgIndex);


               if (distance <= m_minDistToProtecteBldg)
                  {
                  if (bldgIndex == -1)
                     {
                     m_pNewDuneLineLayer->SetData(dunePt, m_colDuneBldgIndx, bldg);
                     m_pNewDuneLineLayer->SetData(dunePt, m_colDuneBldgDist, distance);
                     }
                  else
                     {
                     float distanceBtwn = 0.0f;
                     m_pNewDuneLineLayer->GetData(dunePt, m_colDuneBldgDist, distanceBtwn);
                     if (distance < distanceBtwn)
                        {
                        m_pNewDuneLineLayer->SetData(dunePt, m_colDuneBldgIndx, bldg);
                        m_pNewDuneLineLayer->SetData(dunePt, m_colDuneBldgDist, distance);
                        }
                     }
                  }
               }
            }
         }
      }


   //for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
   //{
   //   double xinfra = 0.0;
   //   double yinfra = 0.0;
   //   m_pInfraLayer->GetPointCoords(infra, xinfra, yinfra);

   //   CArray<int, int> iduIndices;
   //   int iduIndex = -1;
   //   int tsunami_hz = -1;

   //   // Get IDU of building if in Tsunamic Hazard Zone
   //   int sz = m_pIduBuildingLkup->GetPolysFromPointIndex(infra, iduIndices);
   //   for (int i = 0; i < sz; i++)
   //   {
   //      m_pIDULayer->GetData(iduIndices[ 0 ], m_colTsunamiHazardZone, tsunami_hz);
   //      bool isTsunamiHzrd = (tsunami_hz == 1) ? true : false;
   //      if (isTsunamiHzrd)
   //         iduIndex = iduIndices[ 0 ];
   //   }

   //   m_pInfraLayer->SetData(infra, m_colInfraIDUIndx, iduIndex);

   //   double northingTop = 0.0;
   //   double northingBtm = 0.0;

   //   if (iduIndex != -1)
   //   {
   //      m_pIDULayer->GetData(iduIndex, m_colNorthingTop, northingTop);
   //      m_pIDULayer->GetData(iduIndex, m_colNorthingBottom, northingBtm);
   //   }

   //   // Determine what building Dune Pts are protecting 
   //   for (MapLayer::Iterator dunePt = m_pNewDuneLineLayer->Begin(); dunePt < m_pNewDuneLineLayer->End(); dunePt++)
   //   {
   //      int beachType = -1;
   //      m_pNewDuneLineLayer->GetData(dunePt, m_colBeachType, beachType);

   //      int tst = dunePt;

   //      // only assign 
   //      if (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED)
   //      {
   //         double northing = 0.0;
   //         m_pNewDuneLineLayer->GetData(dunePt, m_colNorthingToe, northing);

   //         int duneIndx = -1;
   //         m_pNewDuneLineLayer->GetData(dunePt, m_colDuneIndx, duneIndx);

   //         double xDunePt = 0.0;
   //         double yDunePt = 0.0;
   //         m_pNewDuneLineLayer->GetPointCoords(dunePt, xDunePt, yDunePt);

   //         if (northing < northingTop && northing > northingBtm)
   //         {
   //            float distance = sqrt((xinfra - xDunePt) * (xinfra - xDunePt) + (yinfra - yDunePt) * (yinfra - yDunePt));

   //            int infraIndex = -1;
   //            m_pNewDuneLineLayer->GetData(dunePt, m_colDuneInfraIndx, infraIndex);

   //            if (infraIndex == -1)
   //            {
   //               m_pNewDuneLineLayer->SetData(dunePt, m_colDuneInfraIndx, infra);
   //               m_pNewDuneLineLayer->SetData(dunePt, m_colDuneInfraDist, distance);
   //            }
   //            else
   //            {
   //               float distanceBtwn = 0.0f;
   //               m_pNewDuneLineLayer->GetData(dunePt, m_colDuneInfraDist, distanceBtwn);
   //               if (distance < distanceBtwn)
   //               {
   //                  m_pNewDuneLineLayer->SetData(dunePt, m_colDuneInfraIndx, infra);
   //                  m_pNewDuneLineLayer->SetData(dunePt, m_colDuneInfraDist, distance);
   //               }
   //            }
   //         }

   //      }
   //   }
   //}


   return true;

   }

bool ChronicHazards::FindClosestDunePtToBldg(EnvContext *pEnvContext)
   {
   // Find closest Dune Pt for each Building
   //   for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
   //   {
   //      int bldgIndex = -1;
   //      m_pBldgLayer->GetData(bldg, m_colBldgDuneIndx, bldgIndex);
   //
   //      CArray<int, int> iduIndices;
   //      int iduIndex = -1;
   //      int tsunami_hz = -1;
   //
   //      int sz = m_pIduBuildingLkup->GetPolysFromPointIndex(bldg, iduIndices);
   //      for (int i = 0; i < sz; i++)
   //      {
   //         m_pIDULayer->GetData(iduIndices[ 0 ], m_colTsunamiHazardZone, tsunami_hz);
   //         bool isTsunamiHzrd = (tsunami_hz == 1) ? true : false;
   //         if (isTsunamiHzrd)
   //            iduIndex = iduIndices[ 0 ];
   //      }
   //
   //      m_pBldgLayer->SetData(bldg, m_colBldgIDUIndx, iduIndex);
   //
   //      float distance = 100000.0f;
   //      int duneIndex = -1;
   //
   //      double northingTop, northingBtm = 0.0;
   //
   //      if (iduIndex != -1)
   //      {
   //         m_pIDULayer->GetData(iduIndex, m_colNorthingTop, northingTop);
   //         m_pIDULayer->GetData(iduIndex, m_colNorthingBottom, northingBtm);
   //      }
   //
   //      // Currently : assign closest dune pt to building only once
   ////   if (bldgIndex == -1)// && iduIndex != -1)
   //      {
   //         double xBldg = 0.0;
   //         double yBldg = 0.0;
   //         m_pBldgLayer->GetPointCoords(bldg, xBldg, yBldg);
   //
   //         for (MapLayer::Iterator dunePt = m_pNewDuneLineLayer->Begin(); dunePt < m_pNewDuneLineLayer->End(); dunePt++)
   //         {
   //            int beachType = -1;
   //            m_pNewDuneLineLayer->GetData(dunePt, m_colBeachType, beachType);
   //
   //            // only assign 
   //            if (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED)
   //            {
   //               double xDunePt = 0.0;
   //               double yDunePt = 0.0;
   //               m_pNewDuneLineLayer->GetPointCoords(dunePt, xDunePt, yDunePt);
   //
   //               int index = -1;
   //               m_pNewDuneLineLayer->GetData(dunePt, m_colDuneIndx, index);
   //
   //               // Find closest dune point
   //               float distanceBtwn = sqrt((xBldg - xDunePt) * (xBldg - xDunePt)); // + (yBldg - yDunePt) * (yBldg - yDunePt));
   //               if (index != -1 && distanceBtwn < distance)
   //               {
   //                  distance = distanceBtwn;
   //                  duneIndex = index;
   //               }
   //            }            
   //         }
   //         m_pBldgLayer->SetData(bldg, m_colBldgDuneIndx, duneIndex);
   //         m_pBldgLayer->SetData(bldg, m_colBldgDuneDist, distance);
   //      }
   //   }

   // Determine if Dune Pt is protecting building
   //for (MapLayer::Iterator dunePt = m_pNewDuneLineLayer->Begin(); dunePt < m_pNewDuneLineLayer->End(); dunePt++)
   //{
   //   int duneIndx = -1;

   //   float minDistance = 1000000.0f;
   //   int bldgIndex = -1;

   //   m_pNewDuneLineLayer->GetData(dunePt, m_colDuneIndx, duneIndx);

   //   for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
   //   {
   //      int index = -1;
   //      m_pBldgLayer->GetData(bldg, m_colBldgDuneIndx, index);

   //      float distance = 0.0f;
   //      m_pBldgLayer->GetData(bldg, m_colBldgDuneDist, distance);

   //      // find bldg with minimum distance to dunePt in Tsunami Hazard Zone
   //      if (index != -1 && duneIndx == index && distance < minDistance)
   //      {
   //         minDistance = distance;
   //         bldgIndex = bldg;
   //      }
   //   }

   //   // ?? want to put both bldgIndex and iduIndex in the dune coverage
   //   m_pNewDuneLineLayer->SetData(dunePt, m_colDuneBldgIndx, bldgIndex);
   //}s


   // Find closest building to dune pt

   for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
      {
      int infraIndex = -1;
      m_pInfraLayer->GetData(infra, m_colInfraDuneIndx, infraIndex);

      // Currently :  assign closest dune pt to infrastructure only once
      if (infraIndex == -1)
         {
         REAL xInfra = 0.0;
         REAL yInfra = 0.0;
         m_pInfraLayer->GetPointCoords(infra, xInfra, yInfra);

         float distance = 100000.0f;
         int duneIndex = -1;

         for (MapLayer::Iterator dunePt = m_pNewDuneLineLayer->Begin(); dunePt < m_pNewDuneLineLayer->End(); dunePt++)
            {
            int beachType = -1;
            m_pNewDuneLineLayer->GetData(dunePt, m_colBeachType, beachType);

            // only assign 
            if (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED)
               {
               REAL xDunePt = 0.0;
               REAL yDunePt = 0.0;
               m_pNewDuneLineLayer->GetPointCoords(dunePt, xDunePt, yDunePt);

               int index = -1;
               m_pNewDuneLineLayer->GetData(dunePt, m_colDuneIndx, index);
               // Find closest dune point
               float distanceBtwn = (float)sqrt((xInfra - xDunePt) * (xInfra - xDunePt) + (yInfra - yDunePt) * (yInfra - yDunePt));
               if (distanceBtwn < distance)
                  {
                  distance = distanceBtwn;
                  duneIndex = index;
                  }
               }
            m_pInfraLayer->SetData(infra, m_colInfraDuneIndx, duneIndex);
            }
         }
      }

   // Determine if Dune Pt is protecting building
   /*for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
   {
   double xCoord = 0.0;
   double yCoord = 0.0;
   m_pNewDuneLineLayer->GetPointCoords(point, xCoord, yCoord);

   int bldgCount = 0;
   m_pNewDuneLineLayer->GetData(point, m_colNumBldgs, bldgCount);

   for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
   {
   double xBldg = 0.0;
   double yBldg = 0.0;

   m_pBldgLayer->GetPointCoords(point, xBldg, yBldg);

   float dx = xBldg - xCoord;
   float dy = yBldg - yCoord;

   if (dx + dy <= m_radiusOfErosion || (dx*dx + dy*dy < m_radiusOfErosion*m_radiusOfErosion) )
   m_pNewDuneLineLayer->SetData(point, m_colNumBldgs, ++bldgCount);
   }

   for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
   {
   double xInfrastructure = 0.0;
   double yInfrastructure = 0.0;

   m_pInfraLayer->GetPointCoords(point, xInfrastructure, yInfrastructure);

   float dx = xInfrastructure - xCoord;
   float dy = yInfrastructure - yCoord;

   if (dx + dy <= m_radiusOfErosion || (dx*dx + dy*dy < m_radiusOfErosion*m_radiusOfErosion) )
   m_pNewDuneLineLayer->SetData(point, m_colNumBldgs, ++bldgCount);
   }
   }*/
   return true;
   }

bool ChronicHazards::IsBldgImpactedByEErosion(int bldg, float avgEro, float distToBldg) //, float eroThreshold, int& iduIndex)
   {
   bool flag1 = false;
   bool flag2 = false;

   CArray< int, int > iduIndices;

   REAL xBldg = 0.0;
   REAL yCoord = 0.0;

   /*double xBldg = 0.0;
   double yBldg = 0.0;*/

   MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(bldg);
   float eroFreq = eroMovingWindow->GetFreqValue();

   float eroThreshold = (float(m_eroCount) / m_windowLengthEroHzrd);

   //m_pBldgLayer->GetData(bldg, m_colBldgEroFreq, eroFreq);

   if (eroFreq >= eroThreshold)
      flag1 = true;

   //m_pBldgLayer->GetPointCoords(bldg, xBldg, yBldg);
   //float distance = sqrt((xBldg - xDunePt) * (xBldg - xDunePt) + (yBldg - yDunePt) * (yBldg - yDunePt));
   //if ((xAvgEro + m_radiusOfErosion) >= distance)
   //flag2 = true;

   /*float distance = 5000.0;
   REAL xPt = 0.0;
   REAL yPt = 0.0;

   m_pNewDuneLineLayer->GetPointCoords(pt, xPt, yPt);
   m_pNewDuneLineLayer->GetData(pt, m_colDuneBldgDist, distance);*/

   if (avgEro >= (distToBldg - m_radiusOfErosion))
      flag2 = true;

   //m_pBldgLayer->GetPointCoords(bldg, xBldg, yCoord);
   /*
   if ((xAvgEro + m_radiusOfErosion) >= xBldg)
   {
   m_pNewDuneLineLayer->SetData(pt, m_colDuneBldgEast, xBldg);
   m_pNewDuneLineLayer->SetData(pt, m_colDuneBldgIndx2, bldg);
   flag2 = true;
   }*/

   return (flag1 || flag2);

   } // end ChronicHazards::IsBldgImpactedByErosion


bool ChronicHazards::CalculateExcessErosion(MapLayer::Iterator pt, float r, float s, bool north)
   {
   REAL xCoord = 0.0;
   REAL yCoord = 0.0;
   m_pNewDuneLineLayer->GetPointCoords(pt, xCoord, yCoord);

   int beachType = -1;
   m_pNewDuneLineLayer->GetData(pt, m_colBeachType, beachType);

   // Determine BPS end effects north
   if (north)
      {
      REAL yCoord_s = yCoord + s;
      while (yCoord < yCoord_s && beachType == BchT_SANDY_DUNE_BACKED)
         {
         REAL excessErosion = -(r / s) * (yCoord - yCoord_s);

         double eastingToe = 0.0;
         m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);
         eastingToe += excessErosion;

         Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
         if (pPoly->GetVertexCount() > 0)
            {
            pPoly->m_vertexArray[0].x = eastingToe;
            pPoly->InitLogicalPoints(m_pMap);
            }

         pt++;
         m_pNewDuneLineLayer->GetPointCoords(pt, xCoord, yCoord);
         m_pNewDuneLineLayer->GetData(pt, m_colBeachType, beachType);
         }
      } // end north
   else
      {
      REAL yCoord_s = yCoord - s;
      while (yCoord > yCoord_s && beachType == BchT_SANDY_DUNE_BACKED)
         {
         REAL excessErosion = (r / s) * (yCoord - yCoord_s);

         double eastingToe = 0.0;
         m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);
         eastingToe += excessErosion;

         Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
         if (pPoly->GetVertexCount() > 0)
            {
            pPoly->m_vertexArray[0].x = eastingToe;
            pPoly->InitLogicalPoints(m_pMap);
            }

         pt--;
         m_pNewDuneLineLayer->GetPointCoords(pt, xCoord, yCoord);
         m_pNewDuneLineLayer->GetData(pt, m_colBeachType, beachType);
         }
      } // end south

   return true;
   } // bool CalculateExcessErosion

bool ChronicHazards::CalculateImpactExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint, MapLayer::Iterator &minToePoint, MapLayer::Iterator &minDistPoint, MapLayer::Iterator &maxTWLPoint)
   {
   int duneBldgIndx = -1;
   m_pNewDuneLineLayer->GetData(startPoint, m_colDuneBldgIndx, duneBldgIndx);

   bool construct = false;

   // No longer needed 
   /*MapLayer::Iterator endPoint = point;
   MapLayer::Iterator maxPoint = point;
   MapLayer::Iterator minToePoint = point;
   MapLayer::Iterator minDistPoint = point;*/

   if (duneBldgIndx != -1)
      {
      MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndx);
      float eroFreq = eroMovingWindow->GetFreqValue();
      m_pNewDuneLineLayer->SetData(startPoint, m_colDuneEroFreq, eroFreq);

      int nextBldgIndx = duneBldgIndx;
      float buildBPSThreshold = m_idpyFactor * 365.0f;

      float maxAmtTop = 0.0f;
      float minDuneToe = 10000.0f;
      float minDistToBldg = 10000.0f;
      float maxTWL = 0.0f;

      while (duneBldgIndx == nextBldgIndx && endPoint < m_pNewDuneLineLayer->End())
         {
         int beachType = -1;
         m_pNewDuneLineLayer->GetData(endPoint, m_colBeachType, beachType);

         if (beachType == BchT_SANDY_DUNE_BACKED)
            {
            MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);

            float movingAvgIDPY = 0.0f;
            m_pNewDuneLineLayer->GetData(endPoint, m_colMvAvgIDPY, movingAvgIDPY);

            float xAvgKD = 0.0f;
            m_pNewDuneLineLayer->GetData(endPoint, m_colXAvgKD, xAvgKD);

            float avgKD = 0.0f;
            m_pNewDuneLineLayer->GetData(endPoint, m_colAvgKD, avgKD);
            float distToBldg = 0.0f;
            m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgDist, distToBldg);

            bool isImpactedByEErosion = false;
            isImpactedByEErosion = IsBldgImpactedByEErosion(nextBldgIndx, avgKD, distToBldg);

            // do any dune points cause trigger
            if (movingAvgIDPY >= buildBPSThreshold || isImpactedByEErosion)
               {
               construct = true;

               // Retrieve the maximum TWL within the moving window
               MovingWindow *twlMovingWindow = m_TWLArray.GetAt(endPoint);
               float movingMaxTWL = twlMovingWindow->GetMaxValue();

               float duneCrest = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneCrest, duneCrest);

               // Find dune point with highest overtopping
               if (maxAmtTop >= 0.0f && (movingMaxTWL - duneCrest) > maxAmtTop)
                  {
                  maxAmtTop = movingMaxTWL - duneCrest;
                  maxTWL = movingMaxTWL;
                  maxTWLPoint = endPoint;
                  }

               // Find dune point with minimum elevation
               float duneToe = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneToe, duneToe);
               if (minDuneToe <= duneToe)
                  {
                  minDuneToe = duneToe;
                  minToePoint = endPoint;
                  }

               // Find dune point closest to the protected building
               float distToBldg = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgDist, distToBldg);
               if (minDistToBldg <= distToBldg)
                  {
                  minDistToBldg = distToBldg;
                  minDistPoint = endPoint;
                  }
               }
            }

         endPoint++;
         if (endPoint < m_pNewDuneLineLayer->End())
            m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, nextBldgIndx);
         }

      } // end protecting building

   if (construct)
      {
      // determine total length of BPS structure, runs along IDU
      REAL yTop = 0.0;
      REAL yBtm = 0.0;
      REAL xCoord = 0.0;
      m_pOrigDuneLineLayer->GetPointCoords(endPoint, xCoord, yTop);
      m_pOrigDuneLineLayer->GetPointCoords(startPoint, xCoord, yBtm);

      // TO DO::: write this total BPSLength into IDU using duneBldgIndx
      double BPSLength = yTop - yBtm;
      }

   return construct;

   } // end CalculateImpactExtent

bool ChronicHazards::CalculateRebuildSPSExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint)
   {
   bool rebuild = false;

   int builtYear = 0;
   m_pNewDuneLineLayer->GetData(startPoint, m_colAddYearSPS, builtYear);

   int yr = builtYear;

   REAL eastingToe = 0.0;
   REAL eastingToeSPS = 0.0;
   float height = 0.0f;
   float heelWidth = 0.0f;
   float topWidth = 0.0f;

   while (yr > 0 && yr == builtYear)
      {
      m_pNewDuneLineLayer->GetData(endPoint, m_colEastingToe, eastingToe);
      m_pNewDuneLineLayer->GetData(endPoint, m_colEastingToeSPS, eastingToeSPS);

      m_pNewDuneLineLayer->GetData(endPoint, m_colTopWidthSPS, topWidth);
      m_pNewDuneLineLayer->GetData(endPoint, m_colHeelWidthSPS, heelWidth);
      float extentErosion = (float)(eastingToe - eastingToeSPS);

      if (extentErosion >= topWidth + m_rebuildFactorSPS * heelWidth) // + height/m_frontSlopeSPS) 
         rebuild = true;

      float extentKD = 0.0f;
      m_pNewDuneLineLayer->GetData(endPoint, m_colKD, extentKD);

      if (extentKD >= topWidth + heelWidth)
         rebuild = true;

      //int bldgIndx = -1;
      //m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, bldgIndx);

      //// Retrieve the Flooding Frequency of the Protected Building
      //if (bldgIndx != -1)
      //{
      //   MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndx);
      //   float floodFreq = floodMovingWindow->GetFreqValue();

      //   float floodThreshold = (float)m_floodCountSPS / m_windowLengthFloodHzrd;
      //   
      //   if (floodFreq >= floodThreshold)
      //      rebuild = true;            
      //}

      endPoint++;
      m_pNewDuneLineLayer->GetData(endPoint, m_colAddYearSPS, yr);
      }

   return rebuild;
   }
//
// return rebuild;
//)

// Helper Methods
bool ChronicHazards::CalculateSegmentLength(MapLayer::Iterator startPoint, MapLayer::Iterator endPoint, float &length)
   {
   // determine length of segment, runs along IDU
   REAL yTop = 0.0;
   REAL yBtm = 0.0;
   REAL xCoord = 0.0;
   m_pOrigDuneLineLayer->GetPointCoords(startPoint, xCoord, yBtm);
   m_pOrigDuneLineLayer->GetPointCoords(endPoint, xCoord, yTop);

   length = float(yTop - yBtm);

   if (length < 0)
      return false;
   return true;
   }

int ChronicHazards::GetBeachType(MapLayer::Iterator pt)
   {
   int beachType = -1;
   if (pt < m_pNewDuneLineLayer->End())
      m_pNewDuneLineLayer->GetData(pt, m_colBeachType, beachType);

   return beachType;
   }

int ChronicHazards::GetNextBldgIndx(MapLayer::Iterator pt)
   {
   int nextBldgIndx = -1;
   if (pt < m_pNewDuneLineLayer->End())
      m_pNewDuneLineLayer->GetData(pt, m_colDuneBldgIndx, nextBldgIndx);

   return nextBldgIndx;
   }

// Currently structure is built landward
// uncomment beachwidth narrowing and eastingToe translation to build seaward
// question of how much beach narrows and toe translates based upon geometry:  if every toe moves different distances
// dependent upon the beachslope use ConstructBPS
void ChronicHazards::ConstructBPS1(int currentYear)
   {
   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      int duneBldgIndx = -1;
      m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, duneBldgIndx);

      if (duneBldgIndx != -1)
         {
         MapLayer::Iterator endPoint = point;
         MapLayer::Iterator maxPoint = point;

         MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndx);
         float eroFreq = eroMovingWindow->GetFreqValue();
         m_pNewDuneLineLayer->SetData(point, m_colDuneEroFreq, eroFreq);

         // find the extent of the BPS as well as location of maximum TWL within that extent
         // doublely needs maximum overtopping and minimum duneToe

         bool cond = CalculateBPSExtent(point, endPoint, maxPoint);

         if (cond)
            {
            m_noConstructedBPS += 1;

            MovingWindow *twlMovingWindow = m_TWLArray.GetAt(maxPoint);
            // Retrieve the maxmum TWL within the designated window
            float newCrest = twlMovingWindow->GetMaxValue();

            float duneToe = 0.0f;
            float duneCrest = 0.0f;
            float beachWidth = 0.0f;
            REAL eastingToe = 0.0;
            float tanb = 0.0f;

            m_pNewDuneLineLayer->GetData(maxPoint, m_colDuneToe, duneToe);
            m_pNewDuneLineLayer->GetData(maxPoint, m_colDuneCrest, duneCrest);
            m_pNewDuneLineLayer->GetData(maxPoint, m_colEastingToe, eastingToe);
            m_pNewDuneLineLayer->GetData(maxPoint, m_colBeachWidth, beachWidth);
            m_pNewDuneLineLayer->GetData(maxPoint, m_colTranSlope, tanb);

            float height = newCrest - duneToe;

            // This is an engineering limit and also prevents the BPS from blocking the viewscape
            if (height > m_maxBPSHeight)
               {
               height = m_maxBPSHeight;
               newCrest = duneToe + height;
               }

            // There is a minimum height for construction
            if (height < m_minBPSHeight)
               {
               height = m_minBPSHeight;
               newCrest = duneToe + height;
               }

            // dune height not reduced
            if (newCrest < duneCrest)
               newCrest = duneCrest;

            float BPSHeight = newCrest - duneToe;

            // for debugging 
            if (m_debugOn)
               {
               //  TODO:::Still using old version, uncomment beachwidth reduction and easting toe translation after decision
               double newEastingToe = (BPSHeight + (tanb - m_slopeBPS) * eastingToe) / (tanb - m_slopeBPS);
               double deltaX = eastingToe - newEastingToe;
               double deltaX2 = BPSHeight / m_slopeBPS;
               m_pNewDuneLineLayer->SetData(point, m_colNewEasting, newEastingToe);
               m_pNewDuneLineLayer->SetData(point, m_colBWidth, deltaX2);
               m_pNewDuneLineLayer->SetData(point, m_colDeltaX, deltaX);
               /* eastingToe = newEastingToe;
               beachWidth -= deltaX;*/
               }

            REAL eastingCrest = eastingToe + BPSHeight / m_slopeBPS;

            // determine length of BPS structure, runs along IDU
            REAL yTop = 0.0;
            REAL yBtm = 0.0;
            REAL xCoord = 0.0;
            m_pOrigDuneLineLayer->GetPointCoords(endPoint, xCoord, yTop);
            m_pOrigDuneLineLayer->GetPointCoords(point, xCoord, yBtm);

            float BPSLength = float(yTop - yBtm);

            float BPSCost = BPSHeight * m_costs.BPS * BPSLength;
            m_constructCostBPS += BPSCost;

            // construct BPS by changing beachtype to RIP RAP BACKED
            for (MapLayer::Iterator pt = point; pt < endPoint; pt++, point++)
               {
               // all dune points of a BPS have the same characteristics
               m_pNewDuneLineLayer->SetData(pt, m_colBeachType, BchT_RIPRAP_BACKED);
               //m_pNewDuneLineLayer->SetData(pt, m_colTypeChange, 1);
               m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, newCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colAddYearBPS, currentYear);
               m_pNewDuneLineLayer->SetData(pt, m_colGammaRough, m_minBPSRoughness);
               m_pNewDuneLineLayer->SetData(pt, m_colCostBPS, BPSCost);
               m_pNewDuneLineLayer->SetData(pt, m_colLengthBPS, BPSLength);
               float width = (float)(eastingCrest - eastingToe);
               m_pNewDuneLineLayer->SetData(pt, m_colWidthBPS, width);

               m_pNewDuneLineLayer->SetData(pt, m_colEastingCrest, eastingCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colDuneToe, duneToe);
               m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, beachWidth);
               //      m_pNewDuneLineLayer->SetData(pt, m_colEastingToe, eastingToe);

               Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
               if (pPoly->GetVertexCount() > 0)
                  {
                  pPoly->m_vertexArray[0].x = eastingToe;
                  pPoly->InitLogicalPoints(m_pMap);
                  }
               }
            }
         }
      }
   }

void ChronicHazards::ConstructBPS(int currentYear)
   {
   // basic idea: start at the bottom of the dune point coverage.
   // Iterate thought the Dune points. looking for points that are protecting a building
   // When one of these is found, check tests to see if we should build a bps.

   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      /*MapLayer::Iterator endPoint = point;
      MapLayer::Iterator maxPoint = point;
      MapLayer::Iterator minPoint = point;
      bool construct = CalculateImpactExtent(point, endPoint, minPoint, maxPoint);*/

      int duneBldgIndx = -1;
      m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, duneBldgIndx);

      bool trigger = false;
      MapLayer::Iterator endPoint = point;
      MapLayer::Iterator maxPoint = point;
      MapLayer::Iterator minToePoint = point;
      MapLayer::Iterator minDistPoint = point;

      CArray<int> dlTypeChangeIndexArray;   // dune points for which the trigger is true (m_pNewDuneLineLayer->SetData(endPoint, m_colTypeChange, 1)
      CArray<int> iduAddBPSYrIndexArray;       // m_pIDULayer->SetData(northIndex, m_colIDUAddBPSYr, currentYear);

                                               // dune protecting building?
      if (duneBldgIndx != -1)
         {
         MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndx);
         float eroFreq = eroMovingWindow->GetFreqValue();
         m_pNewDuneLineLayer->SetData(point, m_colDuneEroFreq, eroFreq);

         int nextBldgIndx = duneBldgIndx;
         float buildBPSThreshold = m_idpyFactor * 365.0f;

         float maxAmtTop = 0.0f;
         float minToeElevation = 10000.0f;
         float minDistToBldg = 100000.0f;
         float maxTWL = 0.0f;

         // look along length of IDU determine if criteria for BPS construction is met
         // We know we are in the same IDU if the next dunepoint (endPoint) is protecting the
         // same buiding
         while (endPoint < m_pNewDuneLineLayer->End() && GetNextBldgIndx(endPoint) == duneBldgIndx)
            {
            int beachType = -1;
            m_pNewDuneLineLayer->GetData(endPoint, m_colBeachType, beachType);

            if (beachType == BchT_SANDY_DUNE_BACKED)
               {
               MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);

               float movingAvgIDPY = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colMvAvgIDPY, movingAvgIDPY);

               float xAvgKD = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colXAvgKD, xAvgKD);

               int duneIndx = -1;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneIndx, duneIndx);

               float avgKD = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colAvgKD, avgKD);
               float distToBldg = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgDist, distToBldg);

               bool isImpactedByEErosion = false;
               isImpactedByEErosion = IsBldgImpactedByEErosion(nextBldgIndx, avgKD, distToBldg);

               // do any dune points cause trigger ?
               if (movingAvgIDPY >= buildBPSThreshold || isImpactedByEErosion)
                  {
                  trigger = true;
                  //??? should this be moved to after cost constraint applied?
                  //m_pNewDuneLineLayer->SetData(endPoint, m_colTypeChange, 1);
                  dlTypeChangeIndexArray.Add(endPoint);
                  }

               // Retrieve the maximum TWL within the moving window
               MovingWindow *twlMovingWindow = m_TWLArray.GetAt(endPoint);
               float movingMaxTWL = twlMovingWindow->GetMaxValue();

               float duneCrest = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneCrest, duneCrest);

               // Find dune point with highest overtopping
               if (maxAmtTop >= 0.0f && (movingMaxTWL - duneCrest) > maxAmtTop)
                  {
                  maxAmtTop = movingMaxTWL - duneCrest;
                  maxTWL = movingMaxTWL;
                  maxPoint = endPoint;
                  }

               // Find dune point with minimum elevation
               float duneToe = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneToe, duneToe);
               if (minToeElevation <= duneToe)
                  {
                  minToeElevation = duneToe;
                  minToePoint = endPoint;
                  }

               // Find dune point closest to the protected building
               /*   float distToBldg = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgDist, distToBldg);*/
               if (minDistToBldg <= distToBldg)
                  {
                  minDistToBldg = distToBldg;
                  minDistPoint = endPoint;
                  }
               }
            endPoint++;
            }   // end of: while (endPoint < m_pNewDuneLineLayer->End() && GetNextBldgIndx(endPoint) == duneBldgIndx)

         endPoint--;  // we should be just over the northern edge of the IDU, back up so we are just inside of it.


         } // end protecting building

           // make sure everything is okay, diddn't overrun bounds
      if (endPoint >= m_pNewDuneLineLayer->End())
         Report::ErrorMsg("Coastal Hazards: Invalid Endpoint encountered when Constructing BPS (0)");
      if (endPoint < m_pNewDuneLineLayer->Begin())
         Report::ErrorMsg("Coastal Hazards: Invalid Endpoint encountered when Constructing BPS (1)");

      // determine length of construction 
      float bpsLength = 0.0f;
      float bpsAvgHeight = 0.0f;
      int iduIndex = -1;
      int northIndex = -1;
      int southIndex = -1;

      // conditions met for construcint BPS, 
      if (trigger)
         {
         m_pIDULayer->m_readOnly = false;
         CArray<int, int> iduIndices;
         CArray<int, int> neighbors;
         int sz = m_pIduBuildingLkup->GetPolysFromPointIndex(duneBldgIndx, iduIndices);
         iduIndex = iduIndices[0];

         REAL northingTop = 0.0;
         REAL northingBottom = 0.0;

         float iduLength = 0.0f;
         m_pIDULayer->GetData(iduIndex, m_colLength, iduLength);
         m_pIDULayer->GetData(iduIndex, m_colNorthingTop, northingTop);
         m_pIDULayer->GetData(iduIndex, m_colNorthingBottom, northingBottom);
         //      m_pIDULayer->SetData(iduIndex, m_colIDUAddBPSYr, currentYear);

         // if the idu length (top to bottom) is less than 500m, consider north, then south neighbor,
         // for expanding the BPS
         if (iduLength <= 500.0f)
            {
            bpsLength = iduLength;

            // find IDU to the north
            Poly *pPoly = m_pIDULayer->GetPolygon(iduIndex);

            Vertex v = pPoly->GetCentroid();
            //Vertex v(pPoly->xMin, northingTop);

            //      int count = m_pIDULayer->GetNearbyPolys(pPoly, neighbors, NULL, 10, 1);
            int count = m_pIDULayer->GetNextToPolys(iduIndex, neighbors);

            // Get north poly
            northIndex = m_pIDULayer->GetNearestPoly(v.x, northingTop + 1.0);
            if (northIndex >= 0)
               {
               if (northIndex == iduIndex) // make sure we got a different IDU
                  Report::LogWarning("Coastal Hazards: No northerly IDU found when extending BPS construction northward");
               else
                  {
                  float northLength = 0.0f;
                  bool ok = m_pIDULayer->GetData(northIndex, m_colLength, northLength);
                  if (ok && bpsLength + northLength <= 500.0f && northIndex >= 0)
                     {
                     ////??? should this be moved to after cost constraint applied?
                     //m_pIDULayer->SetData(northIndex, m_colIDUAddBPSYr, currentYear);
                     iduAddBPSYrIndexArray.Add(northIndex);

                     // check to make sure beachtype is Sandy Dune Backed
                     bpsLength += northLength;
                     int northPt = (int)(northLength / m_cellHeight) - 1;
                     int i = 0;
                     int beachType = -1;
                     m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
                     while (beachType == BchT_SANDY_DUNE_BACKED && endPoint < m_pNewDuneLineLayer->End() && i < northPt)
                        {
                        i++;
                        m_pNewDuneLineLayer->GetData(endPoint, m_colBeachType, beachType);
                        endPoint++;
                        }
                     }
                  }
               }

            float southLength = 0.0;
            southIndex = m_pIDULayer->GetNearestPoly(v.x, northingBottom - 1.0);
            ASSERT(southIndex >= 0);
            bool ok = m_pIDULayer->GetData(southIndex, m_colLength, southLength);
            if (ok && bpsLength + southLength <= 500.0f && southIndex >= 0)
               {
               ////??? should this be moved to after cost constraint applied?
               //m_pIDULayer->SetData(southIndex, m_colIDUAddBPSYr, currentYear);
               iduAddBPSYrIndexArray.Add(southIndex);

               bpsLength += southLength;
               float addedLength = float(v.y - northingBottom);
               int southPt = (int)((southLength + addedLength) / m_cellHeight) - 1;

               int i = 0;
               int beachType = -1;
               m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
               while (beachType == BchT_SANDY_DUNE_BACKED && point > m_pNewDuneLineLayer->Begin() && i < southPt)
                  {
                  i++;
                  m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
                  point--;
                  }
               }

            // determine cost of structure and whether there is money to construct it
            // before construction 

            }
         }  // end trigger (calculate bpsLength)

      ASSERT(endPoint < m_pNewDuneLineLayer->End());
      ASSERT(point >= m_pNewDuneLineLayer->Begin());
      if (endPoint >= m_pNewDuneLineLayer->End())
         endPoint = m_pNewDuneLineLayer->End();
      if (point < m_pNewDuneLineLayer->Begin())
         point = m_pNewDuneLineLayer->Begin();

      // determine cost of bps construction
      if (trigger)
         {
         // Retrieve the maximum TWL within the designated window
         // for the dune point with the maximum overtopping within the extent
         MovingWindow *twlMovingWindow = m_TWLArray.GetAt(maxPoint);
         float newCrest = twlMovingWindow->GetMaxValue() + m_safetyFactor;

         // Retrieve dune crest elevation at maximum overtopping
         float duneCrest = 0.0f;
         m_pNewDuneLineLayer->GetData(maxPoint, m_colDuneCrest, duneCrest);

         // Retreive the dune point with the minimum dune toe elevation within the extent
         float minDuneToe = 0.0f;
         m_pNewDuneLineLayer->GetData(minToePoint, m_colDuneToe, minDuneToe);

         // determine maximum height of structure
         float maxHeight = newCrest - minDuneToe;

         // This is an engineering limit and also prevents the BPS from blocking the viewscape
         if (maxHeight > m_maxBPSHeight)
            {
            maxHeight = m_maxBPSHeight;
            newCrest = minDuneToe + maxHeight;
            }

         // There is a minimum height for construction
         if (maxHeight < m_minBPSHeight)
            {
            maxHeight = m_minBPSHeight;
            newCrest = minDuneToe + maxHeight;
            }

         // structure elevation is the same as dune crest elevation of most impacted dune
         if (newCrest < duneCrest)
            newCrest = duneCrest;

         // determine where to build the BPS :  relative to the toe or bldg
         bool constructFromToe = true;
         float minDistToBldg = 0.0f;
         m_pNewDuneLineLayer->GetData(minDistPoint, m_colDuneBldgDist, minDistToBldg);
         float yToe = 0.0f;
         m_pNewDuneLineLayer->GetData(minDistPoint, m_colDuneToe, yToe);

         /*float maxWidth = (newCrest - yToe) / m_slopeBPS;
         if (minDistToBldg < maxWidth)
         constructFromToe = false;*/

         /*   }

         if (construct)
         {*/

         // Build BPS such that each section is a different height but is level at the top
         // of the structure
         int cnt = 0;
         for (MapLayer::Iterator dpt = point; dpt < endPoint; dpt++)
            {
            float duneToe = 0.0f;
            bool ok = m_pNewDuneLineLayer->GetData(dpt, m_colDuneToe, duneToe);
            if (ok)
               {
               float bpsHeight = newCrest - duneToe;
               bpsAvgHeight += bpsHeight;
               cnt++;
               }
            }

         bpsAvgHeight /= cnt;
         if (bpsAvgHeight < 0)
            bpsAvgHeight = 0;

         if (bpsLength < 0)
            bpsLength = 0;

         // check cost against budget
         float cost = bpsAvgHeight * m_costs.BPS * bpsLength;

         bool passCostConstraint = true;

         // get budget infor for Constructing BPS policy
         PolicyInfo &pi = m_policyInfoArray[PI_CONSTRUCT_BPS];

         // enough funds in original allocation ?
         if (pi.HasBudget() && pi.m_availableBudget <= cost && cost > 0) //armorAllocationResidual >= cost)
            passCostConstraint = false;

         AddToPolicySummary(currentYear, PI_CONSTRUCT_BPS, int(point), m_costs.BPS, cost, pi.m_availableBudget, bpsAvgHeight, bpsLength, passCostConstraint);

         if (passCostConstraint)
            {
            // Annual County wide cost for building BPS
            m_constructCostBPS += cost;
            pi.IncurCost(cost);

            //m_pResidualArray->Add(armorCol, row, -cost);   // subtract cost from this year
            //m_prevExpenditures[armorCol] = m_armorCost;

            if (iduIndex != -1)
               m_pIDULayer->SetData(iduIndex, m_colIDUAddBPSYr, currentYear);
            if (northIndex != -1)
               m_pIDULayer->SetData(northIndex, m_colIDUAddBPSYr, currentYear);
            if (southIndex != -1)
               m_pIDULayer->SetData(southIndex, m_colIDUAddBPSYr, currentYear);

            m_pIDULayer->m_readOnly = true;

            m_noConstructedBPS += 1;

            // Calculate Excess Erosion
            //float r = 0.10f * bpsLength;
            //float s = 0.69f * bpsLength;

            //CalculateExcessErosion(endPoint, r, s, true);   // Walk north
            //CalculateExcessErosion(point, r, s, false);      // Walk south      

            // construct BPS along length following contour of beach
            for (MapLayer::Iterator pt = point; pt < endPoint; pt++, point++)
               {
               // Retrieve existing dune charactersitics
               float duneToe = 0.0f;
               m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneToe);

               double eastingToe = 0.0;
               m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);

               double beachWidth = 0.0;
               m_pNewDuneLineLayer->GetData(pt, m_colBeachWidth, beachWidth);

               float tanb = 0.0f;
               m_pNewDuneLineLayer->GetData(pt, m_colTranSlope, tanb);

               // unsure if this should be done
               //if (!constructFromToe)
               // duneToe -= xToeShift * tanb;

               float bpsHeight = newCrest - duneToe;

               float bpsWidth = bpsHeight / m_slopeBPS;

               double eastingCrest = 0.0;

               // if BPS is constructed from the toe landward the beachwidth doesn't narrow
               if (constructFromToe)
                  {
                  eastingCrest = eastingToe + bpsWidth;
                  //  ??? does beach narrow
                  //   beachWidth -= width;
                  }
               else // construct seaward as much as possible, narrowing beach appropriately
                  {
                  float xShift = (bpsWidth - minDistToBldg);
                  beachWidth -= xShift;
                  eastingToe -= xShift;

                  eastingCrest -= xShift;
                  }

               // pinch point   y1 = m1 * x1 + b1 = y2 = m2 * x2 + b2
               //  x = (b2 - b1) / (m1 - m2)
               //  height = newCrest - duneToe
               //double newEastingToe = (height + (tanb - m_slopeBPS) * eastingToe) / (tanb - m_slopeBPS);
               //double deltaX = eastingToe - newEastingToe;

               //double eastingCrest = eastingToe;

               //float newDuneToe = tanb * (float)newEastingToe + (duneToe - tanb * (float)eastingToe);

               //eastingToe = newEastingToe;
               //beachWidth -= deltaX;
               //duneToe = newDuneToe;

               // determine cost of a section along the BPS
               // Cost is per height per meter
               //   float cost = bpsHeight * m_costs.BPS * (float)m_cellHeight;

               // construct BPS by changing beachtype to RIP RAP BACKED
               /*int beachType = -1;
               m_pNewDuneLineLayer->GetData(pt, m_colBeachType, beachType);
               if (beachTYpe == BchT_SANDY_DUNE_BACKED)
               { */
               m_pNewDuneLineLayer->SetData(pt, m_colBeachType, BchT_RIPRAP_BACKED);
               m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, newCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colAddYearBPS, currentYear);
               m_pNewDuneLineLayer->SetData(pt, m_colGammaRough, m_minBPSRoughness);

               //      m_pNewDuneLineLayer->SetData(pt, m_colCostBPS, cost);
               m_pNewDuneLineLayer->SetData(pt, m_colHeightBPS, bpsHeight);
               m_pNewDuneLineLayer->SetData(pt, m_colLengthBPS, m_cellHeight);
               m_pNewDuneLineLayer->SetData(pt, m_colWidthBPS, bpsWidth);
               m_pNewDuneLineLayer->SetData(pt, m_colDuneToe, duneToe);
               m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, beachWidth);

               m_pNewDuneLineLayer->SetData(pt, m_colEastingToeBPS, eastingToe);

               m_pNewDuneLineLayer->SetData(pt, m_colEastingCrest, eastingCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colEastingToe, eastingToe);


               // take care of those coverage updates we deferred from above
               for (int i = 0; i < (int)dlTypeChangeIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlTypeChangeIndexArray[i], m_colTypeChange, 1);

               for (int i = 0; i < (int)iduAddBPSYrIndexArray.GetSize(); i++)
                  m_pIDULayer->SetData(iduAddBPSYrIndexArray[i], m_colIDUAddBPSYr, currentYear);


               // update new dune toe position
               Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
               if (pPoly->GetVertexCount() > 0)
                  {
                  pPoly->m_vertexArray[0].x = eastingToe;
                  pPoly->InitLogicalPoints(m_pMap);
                  }
               }
            } // end cost constrained construction
         else
            {   // don't construct
            pi.m_unmetDemand += cost;

            point = endPoint;
            }
         }
      } // end Dune Points
   }


void ChronicHazards::ConstructSPS(int currentYear)
   {
   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      //// Determine the extent of the protection structure as well as the dune pt with the
      //// maximum overtopping, the dune pt with the minumum dune toe elevation and 
      //// the dune pt closest to the protected building
      //bool construct = CalculateImpactExtent(point, endPoint, minToePoint, minDistPoint, maxTWLPoint);

      int duneBldgIndx = -1;
      m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, duneBldgIndx);

      bool trigger = false;
      MapLayer::Iterator endPoint = point;
      MapLayer::Iterator maxTWLPoint = point;
      MapLayer::Iterator minToePoint = point;
      MapLayer::Iterator minDistPoint = point;
      MapLayer::Iterator maxDuneWidthPoint = point;

      CArray<int> dlTypeChangeIndexArray;               // m_pNewDuneLineLayer->SetData(endPoint, m_colTypeChange, 1);
      CArray<pair<int, float>> dlDuneToeSPSIndexArray;      // m_pNewDuneLineLayer->SetData(pt, m_colDuneToeSPS, duneToe);
      CArray<pair<int, float>> dlEastingToeSPSIndexArray;  // m_pNewDuneLineLayer->SetData(pt, m_colEastingToeSPS, eastingToe);
      CArray<pair<int, float>> dlDuneHeelIndexArray;      // m_pNewDuneLineLayer->SetData(pt, m_colDuneHeel, duneHeel);
      CArray<pair<int, float>> dlFrontSlopeIndexArray;    // m_pNewDuneLineLayer->SetData(pt, m_colFrontSlope, frontSlope);
      CArray<pair<int, float>> dlDuneToeIndexArray;       // m_pNewDuneLineLayer->SetData(pt, m_colDuneToe, duneToe);
      CArray<pair<int, float>> dlDuneCrestIndexArray;    // m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, duneCrest);
      CArray<pair<int, float>> dlDuneWidthIndexArray;    // m_pNewDuneLineLayer->SetData(pt, m_colDuneWidth, duneWidth);

      if (duneBldgIndx != -1)
         {
         MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndx);
         float eroFreq = eroMovingWindow->GetFreqValue();
         m_pNewDuneLineLayer->SetData(point, m_colDuneEroFreq, eroFreq);

         //   int nextBldgIndx = duneBldgIndx;
         float buildBPSThreshold = m_idpyFactor * 365.0f;

         float maxAmtTop = 0.0f;
         float minToeElevation = 10000.0f;
         float minDistToBldg = 100000.0f;
         float maxTWL = 0.0f;
         float maxDuneWidth = 0.0f;

         while (endPoint < m_pNewDuneLineLayer->End() && GetNextBldgIndx(endPoint) == duneBldgIndx)
            {
            int beachType = -1;
            m_pNewDuneLineLayer->GetData(endPoint, m_colBeachType, beachType);

            if (beachType == BchT_SANDY_DUNE_BACKED)
               {
               MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);

               float movingAvgIDPY = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colMvAvgIDPY, movingAvgIDPY);

               float xAvgKD = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colXAvgKD, xAvgKD);

               float avgKD = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colAvgKD, avgKD);
               float distToBldg = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgDist, distToBldg);

               bool isImpactedByEErosion = false;
               isImpactedByEErosion = IsBldgImpactedByEErosion(GetNextBldgIndx(endPoint), avgKD, distToBldg);

               // do any dune points cause trigger ?
               if (movingAvgIDPY >= buildBPSThreshold || isImpactedByEErosion)
                  {
                  int yrBuilt = 0;
                  m_pNewDuneLineLayer->GetData(endPoint, m_colAddYearSPS, yrBuilt);
                  if (yrBuilt == 0)
                     {
                     //??? defer to later
                     //m_pNewDuneLineLayer->SetData(endPoint, m_colTypeChange, 1);
                     dlTypeChangeIndexArray.Add(endPoint);
                     trigger = true;
                     }
                  }

               // Retrieve the maximum TWL within the moving window
               MovingWindow *twlMovingWindow = m_TWLArray.GetAt(endPoint);
               float movingMaxTWL = twlMovingWindow->GetMaxValue();

               float duneCrest = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneCrest, duneCrest);

               // Find dune point with highest overtopping
               if (maxAmtTop >= 0.0f && (movingMaxTWL - duneCrest) > maxAmtTop)
                  {
                  maxAmtTop = movingMaxTWL - duneCrest;
                  maxTWL = movingMaxTWL;
                  maxTWLPoint = endPoint;
                  }

               // Find dune point with minimum elevation
               float duneToe = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneToe, duneToe);
               if (minToeElevation <= duneToe)
                  {
                  minToeElevation = duneToe;
                  minToePoint = endPoint;
                  }

               // Find dune point closest to the protected building
               //float distToBldg = 0.0f;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgDist, distToBldg);
               if (minDistToBldg <= distToBldg)
                  {
                  minDistToBldg = distToBldg;
                  minDistPoint = endPoint;
                  }
               // Find widest dune within the extent
               float duneWidth = 0.0;
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneWidth, duneWidth);
               if (maxDuneWidth > duneWidth)
                  {
                  maxDuneWidth = duneWidth;
                  maxDuneWidthPoint = endPoint;
                  }
               }

            endPoint++;
            }
         endPoint--;

         } // end protecting building

      ASSERT(endPoint < m_pNewDuneLineLayer->End());
      ASSERT(point >= m_pNewDuneLineLayer->Begin());
      if (endPoint >= m_pNewDuneLineLayer->End())
         endPoint = m_pNewDuneLineLayer->End();
      if (point < m_pNewDuneLineLayer->Begin())
         point = m_pNewDuneLineLayer->Begin();

      /////*** cut here make this a method ******//
      // catch if dune was already constructed

      if (trigger)
         {
         // Build SPS such that each section is a different height but is level at the top
         // of the structure
         // m_noConstructedDune += 1;   moved to beow cost constraint

         // Retrieve the maximum TWL within the designated window
         // for the dune point with the maximum overtopping within the extent
         MovingWindow *twlMovingWindow = m_TWLArray.GetAt(maxTWLPoint);
         float newCrest = twlMovingWindow->GetMaxValue() + m_safetyFactor;

         // Retrieve dune crest elevation at maximum overtopping
         float duneCrest = 0.0f;
         m_pNewDuneLineLayer->GetData(maxTWLPoint, m_colDuneCrest, duneCrest);

         // Retrieve the dune point with the minimum dune toe elevation within the extent
         float minDuneToe = 0.0f;
         m_pNewDuneLineLayer->GetData(minToePoint, m_colDuneToe, minDuneToe);

         // Determine maximum height of structure
         float maxHeight = newCrest - minDuneToe;

         //These checks should be different should I base them on GHC averages??? Katy's data
         /*if (maxHeight > m_maxBPSHeight)
         {
         maxHeight = m_maxBPSHeight;
         newCrest = minDuneToe + maxHeight;
         }*/

         //// There is a minimum height for construction
         //if (maxHeight < m_minBPSHeight)
         //{
         //   maxHeight = m_minBPSHeight;
         //   newCrest = minDuneToe + maxHeight;
         //}

         // dune height not reduced
         if (newCrest < duneCrest)
            newCrest = duneCrest;

         //// determine total length of BPS structure, runs along IDU
         //REAL yTop = 0.0;
         //REAL yBtm = 0.0;
         //REAL xCoord = 0.0;
         //m_pOrigDuneLineLayer->GetPointCoords(endPoint, xCoord, yTop);
         //m_pOrigDuneLineLayer->GetPointCoords(point, xCoord, yBtm);

         //// TO DO::: write total length into IDU 
         //double SPSLength = yTop - yBtm;

         // determine where to build the SPS :  relative to the toe or bldg
         // Retrieve the minimum distance to building
         bool constructFromToe = true;
         float minDistToBldg = 0.0f;
         m_pNewDuneLineLayer->GetData(minDistPoint, m_colDuneBldgDist, minDistToBldg);
         float yToe = 0.0f;
         m_pNewDuneLineLayer->GetData(minDistPoint, m_colDuneToe, yToe);
         float yHeel = 0.0f;
         m_pNewDuneLineLayer->GetData(minDistPoint, m_colDuneHeel, yHeel);

         float widestDuneWidth = 0.0f;
         m_pNewDuneLineLayer->GetData(maxDuneWidthPoint, m_colDuneWidth, widestDuneWidth);

         /*   if (minDistToBldg  < widestDuneWidth)
         constructFromToe = false;*/

         float btmWidth = m_topWidthSPS + maxHeight / m_avgFrontSlope + maxHeight / m_avgBackSlope - (yHeel - yToe) / m_avgBackSlope;
         /*fooat oHeelWidth = (maxHeight - yHeel) / frontSlope;
         float topWidth = duneWidth - height / frontSlope - heelWidth;*/

         /*if (minDistToBldg  < btmWidth)
         constructFromToe = false;*/

         // Construct SPS by indicating year of construction  
         // Construct SPS following the contour of the shoreline
         for (MapLayer::Iterator pt = point; pt < endPoint; pt++, point++)
            {
            // Retrieve existing dune characteristics
            float duneToe = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneToe);

            // defer to later            
            //m_pNewDuneLineLayer->SetData(pt, m_colDuneToeSPS, duneToe);
            dlDuneToeSPSIndexArray.Add(pair<int, float>((int)pt, duneToe));

            float duneHeel = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneHeel);

            float duneWidth = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneWidth, duneWidth);

            float frontSlope = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colFrontSlope, frontSlope);

            float backSlope = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colBackSlope, backSlope);

            float eastingToe = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);


            // defer to later            
            // m_pNewDuneLineLayer->SetData(pt, m_colEastingToeSPS, eastingToe);
            dlEastingToeSPSIndexArray.Add(pair<int, float>((int)pt, eastingToe));

            double eastingCrest = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colEastingCrest, eastingCrest);

            double beachWidth = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colBeachWidth, beachWidth);

            float tanb = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colTranSlope, tanb);

            // Determine volume (area * length) of existing sand in dune
            // Consists of 2 triangles + rectangle - one triangle

            if (duneHeel <= 0.0)
               {
               duneHeel = m_avgDuneHeel;

               // defer to later
               //m_pNewDuneLineLayer->SetData(pt, m_colDuneHeel, duneHeel);
               dlDuneHeelIndexArray.Add(pair<int, float>((int)pt, duneHeel));
               }

            if (duneWidth <= 0.0)
               {
               duneWidth = m_avgDuneWidth;

               // defer to later
               //m_pNewDuneLineLayer->SetData(pt, m_colDuneWidth, duneWidth);
               dlDuneWidthIndexArray.Add(pair<int, float>((int)pt, duneWidth));
               }

            if (frontSlope < 0.0)
               {
               frontSlope = m_avgFrontSlope;

               // defer to later
               //m_pNewDuneLineLayer->SetData(pt, m_colFrontSlope, frontSlope);
               dlFrontSlopeIndexArray.Add(pair<int, float>((int)pt, frontSlope));
               }

            if (duneToe < 0.0)
               {
               duneToe = m_avgDuneToe;
               // defer to later
               //m_pNewDuneLineLayer->SetData(pt, m_colDuneToe, duneToe);
               dlDuneToeIndexArray.Add(pair<int, float>((int)pt, duneToe));
               }

            if (duneCrest < 0.0)
               {
               duneCrest = m_avgDuneCrest;
               // defer to later
               //m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, duneCrest);
               dlDuneCrestIndexArray.Add(pair<int, float>((int)pt, duneCrest));
               }

            double totalExistingArea = CalculateArea(duneToe, duneCrest, duneHeel, frontSlope, duneWidth);

            if (totalExistingArea < 0.0)
               totalExistingArea = 0.0;

            /*float x1 = (duneCrest - duneToe) / frontSlope;
            float h1 = duneCrest - duneToe;
            float x2 = duneWidth - x1;
            float h2 = duneCrest - duneHeel;
            float h3 = duneHeel - duneToe;

            float A1 = 0.5 * x1 * h1;
            float A2 = 0.5 * x2 * h2;
            float A3 = (duneWidth-x1) * h3;
            float A4 = 0.5 * duneWidth * h3;

            double totalExistingArea = A1 + A2 + A3 - A4;*/

            // unsure if this should be done
            //if (!constructFromToe)
            // duneToe -= xToeShift * tanb;


            // Strategy : Given the existing dune's width, front and back slopes, determine the dune's topWidth
            // Then fix that topWidth given a minimum, grow the dune in height, keeping slopes the same, widen the bottom
            // of the dune

            // Determine a topWidth for existing slopes and duneWidth
            // backSlope is negative value in the coverage
            float topWidth = duneWidth - ((duneCrest - duneToe) / frontSlope) - ((duneHeel - duneToe) / ABS(backSlope));

            if (topWidth < 2.0)
               topWidth = 2.0;

            // ???? do these change ????
            float newDuneToe = duneToe;
            float newDuneHeel = duneHeel;

            // height of this section
            float height = newCrest - newDuneToe;

            // Determine widened dune after heightened dune
            // backSlope is negative value in coverage
            float heelWidth = (height / ABS(backSlope)) + ((newDuneHeel - newDuneToe) / backSlope);
            float newDuneWidth = topWidth + (height / frontSlope) + heelWidth;
            /*
            float btmWidth = m_topWidthSPS + height / m_frontSlopeSPS + height / m_backSlopeSPS - (duneHeel - duneToe) / m_backSlopeSPS;
            float heelWidth = (newCrest - duneHeel) / m_backSlopeSPS;*/

            if (constructFromToe)
               {
               eastingCrest = eastingToe + height / frontSlope;
               }
            //else // construct seaward as much as possible from bldg
            //{
            //   float xShift = (btmWidth - minDistToBldg);
            //   eastingToe -= xShift;
            //   eastingCrest -= xShift;
            //}         

            //******       Determine volume of sand needed to construct dune ******//   

            //double trapHeight = newCrest - duneHeel;
            //float baseFront = (newCrest - duneHeel) / m_frontSlopeSPS;
            //float baseBack = (newCrest - duneHeel) / m_backSlopeSPS;

            //// trapezoid consists of 2 triangles and one rectangle
            //// since sides are not parallel
            //double rectangleArea = m_topWidthSPS * trapHeight;
            //double triangleFrontArea = 0.5 * baseFront * trapHeight;
            //double triangleBackArea = 0.5 * baseBack * trapHeight;

            //double trapArea = rectangleArea + triangleFrontArea + triangleBackArea;

            //// bottom part consists of 2 triangles (different from above)
            //float x = (duneHeel - duneToe) / m_frontSlopeSPS;
            //float y = (tanb * x) + duneToe;
            //double trianglesArea = 0.5 * (duneHeel - y) * (btmWidth- x) + 0.5 * x * (duneHeel - y);

            // double duneArea = CalculateArea(duneToe, newCrest, duneHeel, m_frontSlopeSPS, btmWidth);

            double duneArea = CalculateArea(newDuneToe, newCrest, newDuneHeel, frontSlope, newDuneWidth);
            //float new_x1 = (newCrest - duneToe) / m_frontSlopeSPS;
            //float new_h1 = newCrest - duneToe;
            //float new_x2 = btmWidth - new_x1;
            float h2 = newCrest - newDuneHeel;
            //float new_h3 = duneHeel - duneToe;

            //float new_A1 = 0.5 * new_x1 * new_h1;
            //float new_A2 = 0.5 * new_x2 * new_h2;
            //float new_A3 = (duneWidth - new_x1) * new_h3;
            //float new_A4 = 0.5 * duneWidth * newh3;

            float A5 = h2 * topWidth;

            // double totalDuneArea = trianglesArea + trapArea;
            double totalConstructedDuneArea = duneArea + A5;

            double constructVolume = 0.0;
            if (totalConstructedDuneArea > 0)
               constructVolume = totalConstructedDuneArea * m_cellHeight;

            // Subtract off area of existing sand, if enough sand
            if (totalExistingArea > 0)
               constructVolume = (totalConstructedDuneArea - totalExistingArea) * m_cellHeight;

            if (constructVolume < 0.0)
               constructVolume = 0.0;

            // Determine cost of a section along the SPS
            // Cost units : $ per cubic meter
            float cost = float(constructVolume * m_costs.SPS);

            // get budget infor for Constructing BPS policy
            PolicyInfo &pi = m_policyInfoArray[PI_CONSTRUCT_SPS];
            bool passCostConstraint = true;

            // enough funds in original allocation ?
            if (pi.HasBudget() && pi.m_availableBudget <= cost) //armorAllocationResidual >= cost)
               passCostConstraint = false;

            AddToPolicySummary(currentYear, PI_CONSTRUCT_SPS, int(pt), m_costs.SPS, cost, pi.m_availableBudget, (float)constructVolume, (float)duneArea, passCostConstraint);

            if (passCostConstraint)
               {
               pi.IncurCost(cost);

               m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, newCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colAddYearSPS, currentYear);

               m_pNewDuneLineLayer->SetData(pt, m_colCostSPS, cost);
               m_pNewDuneLineLayer->SetData(pt, m_colHeightSPS, height);
               m_pNewDuneLineLayer->SetData(pt, m_colWidthSPS, newDuneWidth);
               m_pNewDuneLineLayer->SetData(pt, m_colTopWidthSPS, topWidth);
               m_pNewDuneLineLayer->SetData(pt, m_colLengthSPS, m_cellHeight);

               m_pNewDuneLineLayer->SetData(pt, m_colDuneToe, newDuneToe);
               m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, beachWidth);
               m_pNewDuneLineLayer->SetData(pt, m_colHeelWidthSPS, heelWidth);
               m_pNewDuneLineLayer->SetData(pt, m_colConstructVolumeSPS, constructVolume);

               // Holds the easting location of the constructed dune -- does move through time
               m_pNewDuneLineLayer->SetData(pt, m_colEastingToeSPS, eastingToe);
               //         m_pNewDuneLineLayer->SetData(pt, m_colEastingCrestSPS, eastingCrest);

               m_pNewDuneLineLayer->SetData(pt, m_colEastingCrest, eastingCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colEastingToe, eastingToe);

               // updated deferred fields (Note: Some of these are redundant with the above
               for (int i = 0; i < (int)dlTypeChangeIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlTypeChangeIndexArray[i], m_colTypeChange, 1);

               for (int i = 0; i < (int)dlDuneToeSPSIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlDuneToeSPSIndexArray[i].first, m_colDuneToeSPS, dlDuneToeSPSIndexArray[i].second);

               for (int i = 0; i < (int)dlEastingToeSPSIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlEastingToeSPSIndexArray[i].first, m_colEastingToeSPS, dlEastingToeSPSIndexArray[i].second);

               for (int i = 0; i < (int)dlDuneHeelIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlDuneHeelIndexArray[i].first, m_colDuneHeel, dlDuneHeelIndexArray[i].second);

               for (int i = 0; i < (int)dlFrontSlopeIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlFrontSlopeIndexArray[i].first, m_colFrontSlope, dlFrontSlopeIndexArray[i].second);

               for (int i = 0; i < (int)dlDuneToeIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlDuneToeIndexArray[i].first, m_colDuneToe, dlDuneToeIndexArray[i].second);

               for (int i = 0; i < (int)dlDuneCrestIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlDuneCrestIndexArray[i].first, m_colDuneCrest, dlDuneCrestIndexArray[i].second);

               for (int i = 0; i < (int)dlDuneWidthIndexArray.GetSize(); i++)
                  m_pNewDuneLineLayer->SetData(dlDuneWidthIndexArray[i].first, m_colDuneWidth, dlDuneWidthIndexArray[i].second);

               Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
               if (pPoly->GetVertexCount() > 0)
                  {
                  pPoly->m_vertexArray[0].x = eastingToe;
                  pPoly->InitLogicalPoints(m_pMap);
                  }

               // Annual County wide cost building SPS
               m_restoreCostSPS += cost;
               // Annual County wide volume of sand of building SPS
               m_constructVolumeSPS += constructVolume;
               }
            else // don't construct due to cost constraint
               {
               pi.m_unmetDemand += cost;
               }
            }
         }
      }  // end of: for each dune point
   }

void ChronicHazards::ConstructDune(int currentYear)
   {
   //   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
   //   {
   //      int duneBldgIndx = -1;
   //      m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, duneBldgIndx);
   //
   //      if (duneBldgIndx != -1)
   //      {
   //         MapLayer::Iterator endPoint = point;
   //         MapLayer::Iterator maxPoint = point;
   //
   //         MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndx);
   //         float eroFreq = eroMovingWindow->GetFreqValue();
   //         m_pNewDuneLineLayer->SetData(point, m_colDuneEroFreq, eroFreq);
   //
   //         // find the extent of protection structure as well as location of maximum TWL within the extent
   //         bool cond = CalculateExtent(point, endPoint, maxPoint);
   //
   //         if (cond)
   //         {
   //            m_noConstructedDune += 1;
   //
   //            //MovingWindow *twlMovingWindow = m_TWLArray.GetAt(maxPoint);
   //            //// Retrieve the maxmum TWL within the designated window
   //            //float newCrest = twlMovingWindow->GetMaxValue();
   //
   //            /*float crestSPS = m_duneCrestSPS;
   //            float heelSPS = m_duneHeelSPS;
   //            float toeSPS = m_duneToeSPS;*/
   //            
   //            float duneToe = 0.0f;
   //            float duneCrest = 0.0f;
   //            double eastingToe = 0.0f;
   //            float beachWidth = 0.0f;
   //            float tanb = 0.0f;
   //
   //            m_pNewDuneLineLayer->GetData(maxPoint, m_colDuneToe, duneToe);
   //            m_pNewDuneLineLayer->GetData(maxPoint, m_colDuneCrest, duneCrest);
   //            m_pNewDuneLineLayer->GetData(maxPoint, m_colEastingToe, eastingToe);
   //            m_pNewDuneLineLayer->GetData(maxPoint, m_colBeachWidth, beachWidth);
   //            m_pNewDuneLineLayer->GetData(maxPoint, m_colTranSlope, tanb);
   //
   //            // 
   //            float diff = duneToe - m_duneToeSPS;
   //
   //
   //
   //            // Dunes are built seaward:
   //            // eastingHeel becomes easting toe, move eastingToe seaward and reduce beachwidth by duneWidth         
   //            float eastingHeel = eastingToe;
   //            eastingToe -= m_duneWidthSPS;
   //            beachWidth -= m_duneWidthSPS;
   //
   //            // determine easting of constructed dune
   //            // eastingCrest is at front edge of built dune
   //            // at new Toe location + (height of dune - height of toe ) / slope of front face of dune
   //            float eastingCrest = eastingToe + (m_duneCrestSPS - m_duneToeSPS) / m_frontSlopeSPS; 
   //            float wedge = eastingHeel - (m_duneCrestSPS - m_duneHeelSPS) / m_backSlopeSPS;
   //
   //            if (m_debugOn)
   //            {
   //               /*   double newEastingToe = ((duneToe + BPSHeight - (m_slopeBPS*eastingToe) -(duneToe - tanb*eastingToe))) / (tanb - m_slopeBPS);
   //               double newEastingToe = (BPSHeight + (tanb - m_slopeBPS) * eastingToe) / (tanb - m_slopeBPS);
   //               double deltaX = eastingToe - newEastingToe;
   //               eastingToe = newEastingToe;
   //               beachWidth -= deltaX;*/
   //            }
   //
   //            //// determine the newCrest elevation
   //            //float height = newCrest - duneToe;
   //
   //            //// This is an engineering limit and also prevents the BPS from blocking the viewscape
   //            //if (height > m_maxBPSHeight)
   //            //{
   //         //   height = m_maxBPSHeight;
   //         //   newCrest = duneToe + height;
   //         //}
   //
   //         //// There is a minimum height for construction
   //         //if (height < m_minBPSHeight)
   //         //{
   //         //   height = m_minBPSHeight;
   //         //   newCrest = duneToe + height;
   //         //}
   //
   //         //// dune height not reduced
   //         //if (newCrest < duneCrest)
   //         //   newCrest = duneCrest;
   //
   //         //// determine the new structure height
   //         //float SPSHeight = newCrest - duneToe;
   //
   //         REAL yTop = 0.0f;
   //         REAL yBtm = 0.0f;
   //         REAL xCoord = 0.0f;
   //         m_pOrigDuneLineLayer->GetPointCoords(endPoint, xCoord, yTop);
   //         m_pOrigDuneLineLayer->GetPointCoords(point, xCoord, yBtm);
   //
   //         // have to determine costs for building dune 
   //         REAL SPSLength = yTop - yBtm;
   //
   //         float SPSCost = SPSHeight * m_costs.SPS * SPSLength;
   //
   //         // County wide cost of restoring dunes
   //         m_restoreCost += SPSCost;
   //
   //
   //         // Calculate added sand volume for a triangular prism + 
   //         //volume = (0.5f * cellHeight * duneToe * (newBeachWidth - beachWidth));
   //
   //
   //         //******       Determine volume of sand needed to build dune ******//
   //
   //         // Consists of volume of trapezoidal prism and volume of  2 triangular prisms
   //
   //         // Volume of a trapezoidal prism 
   //         // V = L * H * (duneWidth + duneTopwidth) / 2.0
   //
   //         double height = m_duneCrestSPS - m_duneHeelSPS;
   //         double topWidthTrap = m_duneWidthSPS - (m_duneCrestSPS - m_duneToeSPS) / m_frontSlopeSPS - (m_duneCrestSPS - m_duneHeelSPS) / m_backSlopeSPS;
   //         double bottomWidthTrap = m_duneWidthSPS - (m_duneHeelSPS - m_duneToeSPS) / m_frontSlopeSPS;
   //
   //         double trapezoidalPrismArea = height * ((m_duneWidthSPS + topWidthTrap) / 2.0);            
   //         double trianglesArea = 0.5 * height * bottomWidthTrap + 0.5 * height * (m_duneWidthSPS - bottomWidthTrap);
   //
   //         // County wide volume of sand of restoring dunes
   //         m_constructVolumeSPS += SPSLength * (trapezoidalPrismArea + trianglesArea);      
   //
   //         // construct Dune by indicating year of construction  
   //         // set points to identical dune characteristics
   //         for (MapLayer::Iterator pt = point; pt < endPoint; pt++, point++)
   //         {
   //            // all dune points of a built dune have the same characteristics
   //      //      m_pNewDuneLineLayer->SetData(pt, m_colBeachType, BchT_RIPRAP_BACKED);
   //            m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, m_duneCrestSPS);
   //            m_pNewDuneLineLayer->SetData(pt, m_colAddYearSPS, currentYear);
   //            m_pNewDuneLineLayer->SetData(pt, m_colCostSPS, SPSCost);
   //            m_pNewDuneLineLayer->SetData(pt, m_colLengthSPS, SPSLength);
   //         
   //            m_pNewDuneLineLayer->SetData(pt, m_colEastingCrest, eastingCrest);
   //            m_pNewDuneLineLayer->SetData(pt, m_colDuneToe, m_duneToeSPS);
   //            m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, beachWidth);
   //      //      m_pNewDuneLineLayer->SetData(pt, m_colEastingToe, eastingToe);
   //            
   //            // Easting location of rebuilt dune
   //            m_pNewDuneLineLayer->SetData(pt, m_colEastingToeSPS, eastingToe);
   //
   //            Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
   //            if (pPoly->GetVertexCount() > 0)
   //            {
   //               pPoly->m_vertexArray[ 0 ].x = eastingToe;
   //               pPoly->InitLogicalPoints(m_pMap);
   //            }
   //         }
   //      }
   //   }
   //}

   }

// Maintains or rebuilds the constructed dune when the dune erodes past the angle of repose
// Rebuilds the dune back to its original constructed characteristics
void ChronicHazards::MaintainSPS(int currentYear)
   {
   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      MapLayer::Iterator endPoint = point;
      MapLayer::Iterator maxPoint = point;
      MapLayer::Iterator minPoint = point;

      // Determine the extent of the protection structure as well as the dune pt with the
      // maximum overtopping and the dune pt with the minumum dune toe elevation
      bool rebuild = CalculateRebuildSPSExtent(point, endPoint); // , minPoint, maxPoint);

      if (rebuild)
         {
         for (MapLayer::Iterator pt = point; pt < endPoint; pt++, point++)
            {
            // Retrieve elevation and location of constructed SPS
            float duneToeSPS = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneToeSPS, duneToeSPS);

            REAL eastingToeSPS = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colEastingToeSPS, eastingToeSPS);

            float height = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colHeightSPS, height);

            float btmWidth = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colWidthSPS, btmWidth);

            double constructVolume = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colConstructVolumeSPS, constructVolume);

            float duneCrest = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneCrest, duneCrest);

            float duneHeel = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneHeel, duneHeel);

            // Retrieve current values 
            float duneToe = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneToe);

            double eastingToe = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);

            double beachWidth = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colBeachWidth, beachWidth);

            float tanb = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colTranSlope, tanb);

            float frontSlope = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colFrontSlope, frontSlope);

            float backSlope = 0.0;
            m_pNewDuneLineLayer->GetData(pt, m_colBackSlope, backSlope);

            float xToeShift = (float)(eastingToe - eastingToeSPS);
            float erodedCrest = duneCrest;
            float width = btmWidth - xToeShift;
            if (xToeShift > m_topWidthSPS)
               erodedCrest -= (xToeShift - m_topWidthSPS) * backSlope;

            //double beachArea = xToeShift * (eastingToeSPS - MHW);
            double beachArea = xToeShift * (duneToeSPS - MHW);
            double totalVolume = (CalculateArea(duneToe, erodedCrest, duneHeel, frontSlope, width) + beachArea) * m_cellHeight;

            if (totalVolume < 0.0)
               totalVolume = 0.0;

            double rebuildVolume = constructVolume - totalVolume;

            if (rebuildVolume < 0.0)
               rebuildVolume = 0.0;

            // Determine cost of a section along the SPS
            // Cost is units $ per cubic meter
            float cost = 0.02f * float(totalVolume) * m_costs.SPS;
            cost += float(rebuildVolume * m_costs.SPS);

            // get budget info
            PolicyInfo &pi = m_policyInfoArray[PI_MAINTAIN_SPS];

            // enough funds in original allocation ?
            bool passCostConstraint = true;

            if (pi.HasBudget() && pi.m_availableBudget <= cost) //armorAllocationResidual >= cost)
               passCostConstraint = false;

            AddToPolicySummary(currentYear, PI_MAINTAIN_SPS, int(pt), m_costs.SPS, cost, pi.m_availableBudget, (float)rebuildVolume, (float)totalVolume, passCostConstraint);

            if (passCostConstraint)
               {
               // Annual County wide cost for building BPS
               pi.IncurCost(cost);

               //   //         // Calculate added sand volume for a triangular prism + 
               //   //         //volume = (0.5f * cellHeight * duneToe * (newBeachWidth - beachWidth));


               //   //******       Determine volume of sand needed to build dune ******//

               //   // Consists of volume of trapezoidal prism and volume of 2 triangular prisms

               //   // Volume of a trapezoidal prism 
               //   // V = L * H * (duneWidth + duneTopwidth) / 2.0

               //   double trapHeight = newCrest - duneHeel;
               //   float base1 = (newCrest - duneHeel) / m_frontSlopeSPS;
               //   float base2 = (newCrest - duneHeel) / m_backSlopeSPS;
               //   double trapTopWidth = m_duneWidthSPS - base1 - base2;

               //   double trapArea = trapHeight * trapTopWidth + 0.5 * height * height / m_frontSlopeSPS; +0.5 * height * height / m_backSlopeSPS;

               //   //   double trapezoidalPrismArea = trapHeight * ((m_duneWidthSPS + trapTopWidth) / 2.0);   
               //   float x = (duneHeel - duneToe) / m_frontSlopeSPS;
               //   float y = (tanb * x) + duneToe;
               //   double trianglesArea = 0.5 * (duneHeel - y) * (m_duneWidthSPS - x) + 0.5 * x * (duneHeel - y);

               //   double area = trianglesArea + trapArea;

               //   double volume = area * m_cellHeight;

               m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, duneCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colMaintYearSPS, currentYear);

               m_pNewDuneLineLayer->SetData(pt, m_colCostSPS, cost);

               m_pNewDuneLineLayer->SetData(pt, m_colHeightSPS, height);
               m_pNewDuneLineLayer->SetData(pt, m_colLengthSPS, m_cellHeight);

               m_pNewDuneLineLayer->SetData(pt, m_colDuneToe, duneToeSPS);
               m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, beachWidth);
               m_pNewDuneLineLayer->SetData(pt, m_colMaintVolumeSPS, rebuildVolume);

               //   m_pNewDuneLineLayer->SetData(pt, m_colEastingCrest, eastingCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colEastingToe, eastingToeSPS);
               m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);


               Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
               if (pPoly->GetVertexCount() > 0)
                  {
                  pPoly->m_vertexArray[0].x = eastingToe;
                  pPoly->InitLogicalPoints(m_pMap);
                  }

               // Annual County wide cost maintaining SPS
               m_maintCostSPS += cost;
               // Annual County wide volume of sand of maintaining SPS
               m_maintVolumeSPS += rebuildVolume;
               }   // end of: if (passCostConstraint)
            else
               {   // don't construct
               pi.m_unmetDemand += cost;
               }
            }
         }
      } // end dune points
   }

void ChronicHazards::ConstructBPS2(int currentYear)
   {
   //float buildBPSThreshold = m_idpyFactor * 365.0f;

   //for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
   //{
   //   int beachType = 0;
   //   float newCrest = 0.0f;
   //   float duneToe = 0.0f;
   //   float duneCrest = 0.0f;
   //   float eastingCrest = 0.0f;
   //   float eastingToe = 0.0f;
   //   float eastingMHW = 0.0f;
   //   double northing = 0.0;
   //   double northingTop = 0.0;
   //   double northingBtm = 0.0;

   //   int duneBldgIndx = -1;
   //   int iduIndx = -1;
   //   int duneIndx = -1;

   //   m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, duneBldgIndx);
   //   m_pNewDuneLineLayer->GetData(point, m_colDuneIndx, duneIndx);

   //   float xAvgKD = 0.0f;
   //   m_pNewDuneLineLayer->GetData(point, m_colXAvgKD, xAvgKD);

   //   // Moving Window of the yearly maximum TWL
   //   MovingWindow *twlMovingWindow = m_TWLArray.GetAt(point);

   //   // Retrieve the maxmum TWL within the designated window
   //   float movingMaxTWL = twlMovingWindow->GetMaxValue();
   //   //   m_pNewDuneLineLayer->SetData(point, m_colMvMaxTWL, movingMaxTWL);

   //   // Retrieve the average TWL within the designated window
   //   float movingAvgTWL = twlMovingWindow->GetAvgValue();
   //   //   m_pNewDuneLineLayer->SetData(point, m_colMvAvgTWL, movingAvgTWL);

   //   MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(point);
   //   // Retrieve the average impact days per yr within the designated window
   //   float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();
   ////   m_pNewDuneLineLayer->SetData(point, m_colMvAvgIDPY, movingAvgIDPY);

   //   m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
   //   m_pNewDuneLineLayer->GetData(point, m_colNorthingToe, northing);
   //   //northingTop = northingBtm = northing;

   //   m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, eastingCrest);
   //   
   //   bool isDunePtWithinIDU = true;
   //   // Can constrain based upon permits or global cost
   //   // ex: if ( numCurrentPermits < numAnnualPermits)

   //   // only construct BPS at dunes that are protecting buildings
   //   bool isImpactedByEErosion = false;
   //   if (duneBldgIndx != -1)
   //   {
   //      MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndx);
   //      float eroFreq = eroMovingWindow->GetFreqValue();
   //      m_pNewDuneLineLayer->SetData(point, m_colDuneEroFreq, eroFreq);
   //      isImpactedByEErosion = IsBldgImpactedByEErosion(duneBldgIndx, xAvgKD);// , iduIndx);
   //   }

   //   // construct BPS by changing beachtype to RIP RAP BACKED
   //   if (beachType == BchT_SANDY_DUNE_BACKED || (movingAvgIDPY >= buildBPSThreshold ) )// || isImpactedByEErosion) )
   //   {
   //      // determine new height of duneCrest

   //      // construct BPS to height of the maximum TWL within the last x years
   //      if (m_useMaxTWL == 1)
   //      {
   //         newCrest = movingMaxTWL;
   //         //add safety factor
   //         newCrest += m_safetyFactor;
   //      }
   //      else
   //      {
   //         // construct BPS to height of the average TWL within the last x years
   //         newCrest = movingAvgTWL;
   //         //add safety factor
   //         newCrest += m_safetyFactor;;
   //      }
   //      //   m_pNewDuneLineLayer->GetData(point, m_colAvgTWL, newCrest);

   //      m_pNewDuneLineLayer->GetData(point, m_colDuneToe, duneToe);
   //      m_pNewDuneLineLayer->GetData(point, m_colDuneCrest, duneCrest);

   //      float BPSHeight = newCrest - duneToe;

   //      // This is an engineering limit and also prevents the BPS from blocking the viewscape
   //      if (BPSHeight > m_maxBPSHeight)
   //      {
   //         BPSHeight = m_maxBPSHeight;
   //         newCrest = duneToe + BPSHeight;
   //      }

   //      // There is a minimum BPS height for construction
   //      if (BPSHeight < m_minBPSHeight)
   //      {
   //         BPSHeight = m_minBPSHeight;
   //         newCrest = duneToe + BPSHeight;
   //      }

   //      // dune height not reduced
   //      if (newCrest < duneCrest)
   //         newCrest = duneCrest;

   //   // If the building is associated with an IDU, run the length of the IDU
   //   // otherwise, run 50m 
   //   if (iduIndx != -1)
   //   {
   //      m_pIDULayer->GetData(iduIndx, m_colNorthingTop, northingTop);
   //      m_pIDULayer->GetData(iduIndx, m_colNorthingBottom, northingBtm);
   //   }
   //   else
   //   {
   //      northingTop = northing - 125.0;
   //      northingBtm = northing + 125.0;
   //   }

   //   m_noConstructedBPS += 1;

   //   // Walk South (decreasing iterator)   
   //   WalkSouth(point, newCrest, currentYear, northingTop, northingBtm);

   //   // This dune point and walk North (increasing interator)
   //   do
   //   {
   //      // change beachtype
   //      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
   //      m_pNewDuneLineLayer->GetData(point, m_colNorthingToe, northing);

   //      m_pNewDuneLineLayer->SetData(point, m_colBeachType, BchT_RIPRAP_BACKED);
   //      m_pNewDuneLineLayer->SetData(point, m_colDuneCrest, newCrest);
   //      m_pNewDuneLineLayer->SetData(point, m_colBPSAddYear, currentYear);
   //      m_pNewDuneLineLayer->SetData(point, m_colGammaRough, m_minBPSRoughness);

   //      m_hardenedShoreline++;

   //      m_pNewDuneLineLayer->GetData(point, m_colEastingToe, eastingToe);
   //      eastingCrest = eastingToe + newCrest / m_slopeBPS;
   //      m_pNewDuneLineLayer->SetData(point, m_colEastingCrest, eastingCrest);

   //      if (northing < northingBtm || northing > northingTop)
   //         isDunePtWithinIDU = false;

   //      point++;

   //      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
   //      m_pNewDuneLineLayer->GetData(point, m_colNorthingToe, northing);

   //   } while (beachType == BchT_SANDY_DUNE_BACKED && isDunePtWithinIDU && point < m_pNewDuneLineLayer->End());

   //      
   //      

   //      //MapLayer::Iterator dunePt = point;

   //      //while (beachType == BchT_SANDY_DUNE_BACKED && isDunePtWithinIDU)// && dunePt < m_pNewDuneLineLayer->End() )
   //      //   {
   //      //      // construct BPS to height of the maximum TWL within the last x years
   //      //      // change beachtype
   //      //      m_pNewDuneLineLayer->SetData(dunePt, m_colBeachType, BchT_RIPRAP_BACKED);
   //      //      m_pNewDuneLineLayer->SetData(dunePt, m_colDuneCrest, newCrest);
   //      //      m_pNewDuneLineLayer->SetData(dunePt, m_colBPSAddYear, currentYear);
   //      //      m_pNewDuneLineLayer->SetData(dunePt, m_colGammaRough, m_minBPSRoughness);

   //      //      m_hardenedShoreline++;

   //      //      m_pNewDuneLineLayer->GetData(dunePt, m_colEastingToe, eastingToe);
   //      //      eastingCrest = eastingToe + newCrest / m_slopeBPS;
   //      //      m_pNewDuneLineLayer->SetData(dunePt, m_colEastingCrest, eastingCrest);

   //      //      m_pNewDuneLineLayer->GetData(dunePt, m_colBeachType, beachType);
   //      //      m_pNewDuneLineLayer->GetData(dunePt, m_colNorthingToe, northing);

   //      //      if (northing > northingTop || northing < northingBtm)
   //      //         isDunePtWithinIDU = false;

   //      //      //   dunePt++;
   //      //   }

   //         /*point = dunePt--;*/

   //            /* m_pNewDuneLineLayer->SetData(point, m_colCPolicyL, PolicyArray[SNumber]);
   //             m_pNewDuneLineLayer->SetData(point, m_colBPSCost, BPSCost);*/

   //             /* if ((currentYear) > 2010)
   //               m_pNewDuneLineLayer->SetData(point, m_colBPSAddYear, 2040);
   //              if ((currentYear) > 2040)
   //               m_pNewDuneLineLayer->SetData(point, m_colBPSAddYear, 2060);
   //              if ((currentYear) > 2060)
   //               m_pNewDuneLineLayer->SetData(point, m_colBPSAddYear, 2100);*/


   //               // how does beachwidth change dependent upon BPS construction

   //                //// Beachwidth changes based on a 2:1 slope
   //                //float beachwidth;
   //                //m_pNewDuneLineLayer->GetData(point, m_colBeachWidth, beachwidth);
   //                //beachwidth -= (BPSHeight * 1.0f);
   //                //if ((duneToe / beachwidth) > 0.1f)
   //                //   beachwidth = duneToe / 0.1f;

   //                //m_pNewDuneLineLayer->SetData(point, m_colBeachWidth, beachwidth);
   //            // get information from next dune Point
   //         /*   point++;
   //         // Move North, building BPS
   //            dunePtIndx--;



   //         */
   //   //   }
   //         

   //   }

   //   // end dune line points

   //   //  if (idpy >= buildBPSThreshold && ( beachType != BchT_BAY ) )
   //   //  {     
   //   //     m_pNewDuneLineLayer->SetData(point, m_colPrevBeachType, beachType);
   //   //     m_pNewDuneLineLayer->SetData(point, m_colBeachType, BchT_RIPRAP_BACKED);
   //   ////     m_pNewDuneLineLayer->SetData(point, m_colBPSMaintYear, currentYear);
   //   //     m_pNewDuneLineLayer->SetData(point, m_colDuneCrest, m_BPSHeight);
   //   //     m_hardenedShoreline += 10;
   //   //  } // end impact days exceeds threshold

   ////   delete ipdyMovingWindow;
   //}

   // m_pNewDuneLineLayer->m_readOnly = true;

   } // end ConstructBPS(int currentYear)

bool ChronicHazards::IsMissingRow(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint)
   {


   return false;
   }

bool ChronicHazards::CalculateBPSExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint, MapLayer::Iterator &maxPoint)
   {
   float buildBPSThreshold = m_idpyFactor * 365.0f;

   bool constructBPS = false;
   int bldgIndx = -1;
   int nextBldgIndx = -1;

   float maxTWL = -10000.0f;

   m_pNewDuneLineLayer->GetData(startPoint, m_colDuneBldgIndx, bldgIndx);
   nextBldgIndx = bldgIndx;

   //before loop startPoint == endPoint
   while (bldgIndx == nextBldgIndx && endPoint < m_pNewDuneLineLayer->End())
      {
      int beachType = -1;
      m_pNewDuneLineLayer->GetData(endPoint, m_colBeachType, beachType);

      if (beachType == BchT_SANDY_DUNE_BACKED)
         //&& movingAvgIDPY >= buildBPSThreshold && isImpactedByEErosion)
         {
         MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
         ///   float movingAvgIDPY1 = ipdyMovingWindow->GetAvgValue();

         float movingAvgIDPY = 0.0f;
         m_pNewDuneLineLayer->GetData(endPoint, m_colMvAvgIDPY, movingAvgIDPY);

         float xAvgKD = 0.0f;
         m_pNewDuneLineLayer->GetData(endPoint, m_colXAvgKD, xAvgKD);

         float avgKD = 0.0f;
         m_pNewDuneLineLayer->GetData(endPoint, m_colAvgKD, avgKD);
         float distToBldg = 0.0f;
         m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgDist, distToBldg);

         bool isImpactedByEErosion = false;
         isImpactedByEErosion = IsBldgImpactedByEErosion(nextBldgIndx, avgKD, distToBldg);

         // do any dune points cause trigger
         if (movingAvgIDPY >= buildBPSThreshold || isImpactedByEErosion)
            {
            MovingWindow *twlMovingWindow = m_TWLArray.GetAt(endPoint);

            // Retrieve the maximum TWL within the designated window
            float movingMaxTWL = twlMovingWindow->GetMaxValue();

            if (movingMaxTWL > maxTWL)
               {
               maxTWL = movingMaxTWL;
               maxPoint = endPoint;
               }

            constructBPS = true;
            }
         }

      endPoint++;
      if (endPoint < m_pNewDuneLineLayer->End())
         m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, nextBldgIndx);
      }

   return constructBPS;

   } // end CalculateBPSExtent

bool ChronicHazards::CalculateExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint, MapLayer::Iterator &maxPoint)
   {
   float buildThreshold = m_idpyFactor * 365.0f;

   bool isConstruct = false;
   int bldgIndx = -1;
   int nextBldgIndx = -1;

   float maxTWL = -10000.0f;

   m_pNewDuneLineLayer->GetData(startPoint, m_colDuneBldgIndx, bldgIndx);
   nextBldgIndx = bldgIndx;

   while (bldgIndx == nextBldgIndx && endPoint < m_pNewDuneLineLayer->End())
      {
      int beachType = -1;
      m_pNewDuneLineLayer->GetData(endPoint, m_colBeachType, beachType);

      if (beachType == BchT_SANDY_DUNE_BACKED)
         //&& movingAvgIDPY >= buildBPSThreshold && isImpactedByEErosion)
         {
         MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
         float movingAvgIDPY1 = ipdyMovingWindow->GetAvgValue();

         float movingAvgIDPY = 0.0f;
         m_pNewDuneLineLayer->GetData(endPoint, m_colMvAvgIDPY, movingAvgIDPY);

         float xAvgKD = 0.0f;
         m_pNewDuneLineLayer->GetData(endPoint, m_colXAvgKD, xAvgKD);

         float avgKD = 0.0f;
         m_pNewDuneLineLayer->GetData(endPoint, m_colAvgKD, avgKD);
         float distToBldg = 0.0f;
         m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgDist, distToBldg);

         bool isImpactedByEErosion = false;
         isImpactedByEErosion = IsBldgImpactedByEErosion(nextBldgIndx, avgKD, distToBldg);

         if (movingAvgIDPY >= buildThreshold || isImpactedByEErosion)
            {
            MovingWindow *twlMovingWindow = m_TWLArray.GetAt(endPoint);

            // Retrieve the maximum TWL within the designated window
            float movingMaxTWL = twlMovingWindow->GetMaxValue();

            if (movingMaxTWL > maxTWL)
               {
               maxTWL = movingMaxTWL;
               maxPoint = endPoint;
               }

            isConstruct = true;
            }
         }

      endPoint++;
      if (endPoint < m_pNewDuneLineLayer->End())
         m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, nextBldgIndx);
      }

   return isConstruct;

   } // end CalculateExtent

int ChronicHazards::WalkSouth(MapLayer::Iterator dunePt, float newCrest, int currentYear, double northingTop, double northingBtm)
   {
   if (dunePt > m_pNewDuneLineLayer->Begin())
      dunePt--;
   else
      return 0;

   bool isDunePtWithinIDU = true;;

   int beachType = -1;
   double northing = 0.0;

   m_pNewDuneLineLayer->GetData(dunePt, m_colBeachType, beachType);
   m_pNewDuneLineLayer->GetData(dunePt, m_colNorthingToe, northing);

   if (northing < northingBtm || northing > northingTop)
      isDunePtWithinIDU = false;

   int count = 0;

   while (beachType == BchT_SANDY_DUNE_BACKED && isDunePtWithinIDU && dunePt > m_pNewDuneLineLayer->Begin());
   {
   // change beachtype
   m_pNewDuneLineLayer->SetData(dunePt, m_colBeachType, BchT_RIPRAP_BACKED);
   m_pNewDuneLineLayer->SetData(dunePt, m_colDuneCrest, newCrest);
   m_pNewDuneLineLayer->SetData(dunePt, m_colAddYearBPS, currentYear);
   m_pNewDuneLineLayer->SetData(dunePt, m_colGammaRough, m_minBPSRoughness);
   count = 1;

   //   m_hardenedShoreline++;

   float eastingToe = 0.0f;
   m_pNewDuneLineLayer->GetData(dunePt, m_colEastingToe, eastingToe);
   float eastingCrest = eastingToe + newCrest / m_slopeBPS;
   m_pNewDuneLineLayer->SetData(dunePt, m_colEastingCrest, eastingCrest);

   dunePt--;
   m_pNewDuneLineLayer->GetData(dunePt, m_colBeachType, beachType);
   m_pNewDuneLineLayer->GetData(dunePt, m_colNorthingToe, northing);

   if (northing < northingBtm || northing > northingTop)
      isDunePtWithinIDU = false;
   }

   return count;

   } // end WalkSouth

     //void ChronicHazards::RemoveBPS(int beachType, int pt, float newCrest, double top, double bottom, EnvContext *pEnvContext)
     //{
     //   bool isDunePtWithinIDU = true;
     //   int currentYear = pEnvContext->currentYear;
     //
     //   while (beachType == BchT_SANDY_DUNE_BACKED && isDunePtWithinIDU) // && dunePt < m_pNewDuneLineLayer->End() )
     //   {
     //      // construct BPS to height of the maximum TWL within the last x years
     //
     //      // change beachtype
     //      m_pNewDuneLineLayer->SetData(pt, m_colBeachType, BchT_RIPRAP_BACKED);
     //      m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, newCrest);
     //      m_pNewDuneLineLayer->SetData(pt, m_colBPSAddYear, currentYear);
     //      m_pNewDuneLineLayer->SetData(pt, m_colGammaRough, m_minBPSRoughness);
     //
     //      m_hardenedShoreline++;
     //
     //      m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);
     //      eastingCrest = eastingToe + newCrest / m_slopeBPS;
     //      m_pNewDuneLineLayer->SetData(pt, m_colEastingCrest, eastingCrest);
     //
     //      m_pNewDuneLineLayer->GetData(pt, m_colBeachType, beachType);
     //      m_pNewDuneLineLayer->GetData(pt, m_colNorthingToe, northing);
     //
     //      //   if (northing > northingTop || northing < northingBtm)
     //      isDunePtWithinIDU = false;
     //
     //      pt++;
     //
     //      /* m_pNewDuneLineLayer->SetData(point, m_colCPolicyL, PolicyArray[SNumber]);
     //      m_pNewDuneLineLayer->SetData(point, m_colBPSCost, BPSCost);*/
     //
     //      /* if ((currentYear) > 2010)
     //      m_pNewDuneLineLayer->SetData(point, m_colBPSAddYear, 2040);
     //      if ((currentYear) > 2040)
     //      m_pNewDuneLineLayer->SetData(point, m_colBPSAddYear, 2060);
     //      if ((currentYear) > 2060)
     //      m_pNewDuneLineLayer->SetData(point, m_colBPSAddYear, 2100);*/
     //
     //
     //      // how does beachwidth change dependent upon BPS construction
     //
     //      //// Beachwidth changes based on a 2:1 slope
     //      //float beachwidth;
     //      //m_pNewDuneLineLayer->GetData(point, m_colBeachWidth, beachwidth);
     //      //beachwidth -= (BPSHeight * 1.0f);
     //      //if ((duneToe / beachwidth) > 0.1f)
     //      //   beachwidth = duneToe / 0.1f;
     //
     //      //m_pNewDuneLineLayer->SetData(point, m_colBeachWidth, beachwidth);
     //      // get information from next dune Point
     //      /*   point++;
     //      // Move North, building BPS
     //      dunePtIndx--;
     //
     //
     //
     //      */
     //
     //   }
     //}

double ChronicHazards::CalculateArea(float duneToe, float duneCrest, float duneHeel, float frontSlope, float btmDuneWidth)
   {
   float x1 = (duneCrest - duneToe) / frontSlope;
   float h1 = duneCrest - duneToe;
   float x2 = btmDuneWidth - x1;
   float h2 = duneCrest - duneHeel;
   float h3 = duneHeel - duneToe;

   float A1 = 0.5f * x1 * h1;
   float A2 = 0.5f * x2 * h2;
   float A3 = (btmDuneWidth - x1) * h3;
   float A4 = 0.5f * btmDuneWidth * h3;

   double totalArea = A1 + A2 + A3 - A4;

   if (totalArea > 0)
      return totalArea;
   else
      return 0.0;
   }


void ChronicHazards::RemoveBPS(EnvContext *pEnvContext)
   {
   //   Only remove BPS if they are not protecting buildings and if they are  ???? at the end of a BPS segment

   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      int beachType = 0;
      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);

      //   Only remove BPS if they are not protecting buildings and if they are at the end of a BPS segment
      //   if (BuildingsArray[ SNumber ] == 0 && beachType == BchT_RIPRAP_BACKED && (duneTypeArray[ SNumber - 1 ] == 1 || duneTypeArray[ SNumber + 1 ] == 1))
      {
      float Cost;
      m_pNewDuneLineLayer->SetData(point, m_colBeachType, 1);
      m_pNewDuneLineLayer->SetData(point, m_colRemoveYearBPS, pEnvContext->currentYear);
      m_pNewDuneLineLayer->GetData(point, m_colCostBPS, Cost);
      //         m_BPSRemovalCost += Cost;
      }
      } // end dune pts

   } // end RemoveBPS


     // BPS is maintained at front of structure, reducing beachwidth and moving structure toe seaward
void ChronicHazards::MaintainBPS(int currentYear)
   {
   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      int beachType = -1;
      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);

      MapLayer::Iterator endPoint = point;
      MapLayer::Iterator maxPoint = point;

      // When did we last increase the height of the structure? 
      int lastMaintYear;
      m_pNewDuneLineLayer->GetData(endPoint, m_colMaintYearBPS, lastMaintYear);

      int addedBPSYear;
      m_pNewDuneLineLayer->GetData(endPoint, m_colAddYearBPS, addedBPSYear);

      if (addedBPSYear == 2010)
         lastMaintYear = 2010;

      int yrsSinceMaintained = currentYear - lastMaintYear;

      if (beachType == BchT_RIPRAP_BACKED && (yrsSinceMaintained > m_maintFreqBPS))
         {
         //Do we need to increase the height of the BPS to keep up with increasing TWL?
         int bldgIndx = -1;
         int nextBldgIndx = -1;

         float amtTopped = 0.0f;
         float newCrest = 0.0f;
         float oldCrest = 0.0f;
         double eastingToe = 0.0;

         bool isBldgFlooded = false;

         m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, bldgIndx);
         nextBldgIndx = bldgIndx;

         if (bldgIndx != -1)
            {
            while (bldgIndx == nextBldgIndx && endPoint < m_pNewDuneLineLayer->End())
               {
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneCrest, oldCrest);
               newCrest = oldCrest;

               // Retrieve the maximum TWL within the designated window
               MovingWindow *twlMovingWindow = m_TWLArray.GetAt(endPoint);
               float movingMaxTWL = twlMovingWindow->GetMaxValue();

               // Retrieve the Flooding Frequency of the Protected Building
               MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndx);
               float floodFreq = floodMovingWindow->GetFreqValue();

               float floodThreshold = (float(m_floodCountBPS) / m_windowLengthFloodHzrd);

               if (floodFreq >= floodThreshold)
                  isBldgFlooded = true;

               // Find dune point with highest overtopping
               if (amtTopped >= 0.0f && (movingMaxTWL - oldCrest) > amtTopped)
                  {
                  amtTopped = movingMaxTWL - oldCrest;
                  newCrest = movingMaxTWL;
                  maxPoint = endPoint;
                  }

               endPoint++;
               if (endPoint < m_pNewDuneLineLayer->End())
                  m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, nextBldgIndx);
               }
            }

         if (isBldgFlooded)
            {
            MapLayer::Iterator endPoint = point;
            MapLayer::Iterator minPoint = point;

            m_pNewDuneLineLayer->GetData(maxPoint, m_colDuneBldgIndx, bldgIndx);
            nextBldgIndx = bldgIndx;

            float maxBPSHeight = -10000.0f;

            float duneToe = 0.0f;

            // find dune point of Maximum BPS Height 
            while (bldgIndx == nextBldgIndx && endPoint < m_pNewDuneLineLayer->End())
               {
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneToe, duneToe);

               if ((newCrest - duneToe) > maxBPSHeight)
                  {
                  maxBPSHeight = newCrest - duneToe;
                  minPoint = endPoint;
                  }
               endPoint++;
               if (endPoint < m_pNewDuneLineLayer->End())
                  m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, nextBldgIndx);
               }

            // engineering limit
            if (maxBPSHeight > m_maxBPSHeight)
               {
               maxBPSHeight = m_maxBPSHeight;
               m_pNewDuneLineLayer->GetData(minPoint, m_colDuneToe, duneToe);
               newCrest = duneToe + m_maxBPSHeight;
               }

            float BPSLength = 0.0f;
            m_pNewDuneLineLayer->GetData(point, m_colLengthBPS, BPSLength);

            float cost = 0.02f * m_costs.BPS * BPSLength;
            if ((newCrest - oldCrest) > 0)
               cost += (newCrest - oldCrest) * m_costs.BPS * BPSLength;

            // get budget infor for Constructing BPS policy
            PolicyInfo &pi = m_policyInfoArray[PI_MAINTAIN_BPS];

            // enough funds in original allocation ?
            bool passCostConstraint = true;

            if (pi.HasBudget() && pi.m_availableBudget <= cost) //armorAllocationResidual >= cost)
               passCostConstraint = false;

            AddToPolicySummary(currentYear, PI_MAINTAIN_BPS, int(point), m_costs.BPS, cost, pi.m_availableBudget, BPSLength, (newCrest - oldCrest), passCostConstraint);

            if (passCostConstraint)
               {
               pi.IncurCost(cost);      // charge to budget

               float priorBPSCost;
               m_pNewDuneLineLayer->GetData(point, m_colCostBPS, priorBPSCost);
               float addedBPSCost = cost + priorBPSCost;
               m_pNewDuneLineLayer->SetData(point, m_colCostBPS, addedBPSCost);
               m_maintCostBPS += cost;

               for (MapLayer::Iterator pt = point; pt < endPoint; pt++)
                  {
                  // Beachwidth changes based on a 1:2 slope 
                  float beachWidth = 0.0f;
                  m_pNewDuneLineLayer->GetData(pt, m_colBeachWidth, beachWidth);

                  float duneCrest = 0.0f;
                  m_pNewDuneLineLayer->GetData(pt, m_colDuneCrest, duneCrest);

                  float duneToe = 0.0f;
                  m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneToe);

                  float tanb = 0.0f;
                  m_pNewDuneLineLayer->GetData(pt, m_colTranSlope, tanb);

                  m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);

                  double eastingCrest = 0.0f;
                  m_pNewDuneLineLayer->GetData(pt, m_colEastingCrest, eastingCrest);

                  if ((newCrest - duneCrest) > 0.0f)
                     {
                     //beachWidth -= ((newCrest - duneCrest) / m_slopeBPS);
                     //   double newEastingToe = (newCrest + (tanb - m_slopeBPS) * eastingToe) / (tanb - m_slopeBPS);
                     double newEastingToe = (newCrest - m_slopeBPS * eastingCrest - duneToe + (tanb * eastingToe)) / (tanb - m_slopeBPS);
                     float deltaX = (float)(eastingToe - newEastingToe);

                     beachWidth -= deltaX;
                     if ((duneToe / beachWidth) > 0.1f)
                        beachWidth = duneToe / 0.1f;

                     if (m_debugOn)
                        {
                        m_pNewDuneLineLayer->SetData(pt, m_colNewEasting, newEastingToe);
                        //      m_pNewDuneLineLayer->SetData(point, m_colBWidth, deltaX2);
                        m_pNewDuneLineLayer->SetData(pt, m_colDeltaX, deltaX);
                        }

                     // DuneCrest height is set to structure height
                     m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, newCrest);
                     m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, beachWidth);
                     m_pNewDuneLineLayer->SetData(pt, m_colMaintYearBPS, currentYear);
                     m_pNewDuneLineLayer->SetData(pt, m_colCostMaintBPS, cost);
                     //
                     // change easting of DuneToe

                     //eastingToe -= ((newCrest - duneCrest) / m_slopeBPS);
                     /*eastingToe = newEastingToe;
                     m_pNewDuneLineLayer->SetData(pt, m_colEastingToe, eastingToe);*/

                     m_pNewDuneLineLayer->SetData(pt, m_colEastingToe, newEastingToe);


                     Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
                     if (pPoly->GetVertexCount() > 0)
                        {
                        pPoly->m_vertexArray[0].x = newEastingToe;
                        pPoly->InitLogicalPoints(m_pMap);
                        }
                     }
                  }
               }   // end of: if (maintain)
            else
               {
               pi.m_unmetDemand += cost;
               }
            }  // end of: if (isBldgFlooded)
         }  // end of: if (beachType == BchT_RIPRAP_BACKED && (yrsSinceMaintained > m_maintFreqBPS))
      }  // end of: (for each dune point)
   } // end MaintainBPS



     // flowing not currently used
void ChronicHazards::MaintainStructure(MapLayer::Iterator point, int currentYear) //, bool isBPS)
   {
   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      /*int bType = -1;
      int beachType = -1;
      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);*/

      MapLayer::Iterator endPoint = point;
      MapLayer::Iterator maxPoint = point;

      // When did we last increase the height of the structure? 
      int maintFreq = 0;
      int lastMaintYear = 2010;
      int builtYear = 0;

      /*   if (isBPS)
      {*/
      m_pNewDuneLineLayer->GetData(endPoint, m_colMaintYearBPS, lastMaintYear);
      m_pNewDuneLineLayer->GetData(endPoint, m_colAddYearBPS, builtYear);
      maintFreq = m_maintFreqBPS;
      //}
      //else // if SPS
      //{
      //   m_pNewDuneLineLayer->GetData(endPoint, m_colMaintYearSPS, lastMaintYear);
      //   m_pNewDuneLineLayer->GetData(endPoint, m_colAddYearSPS, builtYear);
      //}

      /*if (beachType == BchT_RIPRAP_BACKED && (lastMaintYear + m_maintFreqBPS < currentYear))
      {*/


      if (builtYear > 0 && lastMaintYear + maintFreq < currentYear)
         {
         //Do we need to increase the height of the BPS to keep up with increasing TWL?
         int bldgIndx = -1;
         int nextBldgIndx = -1;

         float amtTopped = 0.0f;
         float newCrest = 0.0f;
         float oldCrest = 0.0f;
         double eastingToe = 0.0;

         bool isBldgFlooded = false;

         m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, bldgIndx);
         nextBldgIndx = bldgIndx;

         while (bldgIndx == nextBldgIndx && endPoint < m_pNewDuneLineLayer->End())
            {
            m_pNewDuneLineLayer->GetData(endPoint, m_colDuneCrest, oldCrest);
            newCrest = oldCrest;

            // Retrieve the maxmum TWL within the designated window
            MovingWindow *twlMovingWindow = m_TWLArray.GetAt(endPoint);
            float movingMaxTWL = twlMovingWindow->GetMaxValue();

            // Retrieve the Flooding Frequency of the Protected Building
            MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndx);
            float floodFreq = floodMovingWindow->GetFreqValue();

            float floodThreshold = (float(m_floodCountBPS) / m_windowLengthFloodHzrd);

            if (floodFreq >= floodThreshold)
               isBldgFlooded = true;

            // Find dune point with highest overtopping
            if (amtTopped >= 0.0f && (movingMaxTWL - oldCrest) > amtTopped)
               {
               amtTopped = movingMaxTWL - oldCrest;
               newCrest = movingMaxTWL;
               maxPoint = endPoint;
               }

            endPoint++;
            if (endPoint < m_pNewDuneLineLayer->End())
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, nextBldgIndx);
            }

         endPoint = point;
         MapLayer::Iterator minPoint = point;

         m_pNewDuneLineLayer->GetData(point, m_colDuneBldgIndx, bldgIndx);
         nextBldgIndx = bldgIndx;

         float maxStructureHeight = -10000.0f;

         float duneToe = 0.0f;

         // find dune point of Maximum Height 
         while (bldgIndx == nextBldgIndx && endPoint < m_pNewDuneLineLayer->End())
            {
            m_pNewDuneLineLayer->GetData(endPoint, m_colDuneToe, duneToe);

            if ((newCrest - duneToe) > maxStructureHeight)
               {
               maxStructureHeight = newCrest - duneToe;
               minPoint = endPoint;
               }
            endPoint++;
            if (endPoint < m_pNewDuneLineLayer->End())
               m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, nextBldgIndx);
            }

         // engineering limit
         float heightLimit = 0.0f;
         //if (isBPS)
         heightLimit = m_maxBPSHeight;
         //else // SPS
         //   //heightLimit = m_maxSPSHeight;

         //   if (maxStructureHeight > heightLimit)
         //   {
         //      maxStructureHeight = heightLimit;
         //      m_pNewDuneLineLayer->GetData(minPoint, m_colDuneToe, duneToe);
         //      newCrest = duneToe + heightLimit;
         //   }

         float structureSlope = 0.0f;

         /*if (isBPS)
         {*/
         // set structure slope based upon a 1:2 slope
         structureSlope = m_slopeBPS;

         // calculate costs based upon which hard protection structure
         float structureLength = 0.0f;
         m_pNewDuneLineLayer->GetData(point, m_colLengthBPS, structureLength);
         float cost = 0.0;
         if ((newCrest - oldCrest) > 0)
            cost = (newCrest - oldCrest) * m_costs.BPS * structureLength;

         m_maintCostBPS += cost;
         //   }
         //   else
         //   {
         //      // set structure slope based upon average front slpoe of dunes in GHC
         //      structureSlope = m_frontSlopeSPS;

         //      // calculate costs based upon which soft protection structure
         //      float structureLength = 0.0f;
         //      m_pNewDuneLineLayer->GetData(point, m_colLengthSPS, structureLength);
         //      float SPSMaintCost = (newCrest - oldCrest) * m_costs.SPS * structureLength;
         //   //   m_maintCostSPS += SPSMaintCost;
         //      m_maintCostSPS += SPSMaintCost;

         ///*      double maintVolume = structureLength * (newCrest - oldCrest) * duneWidth;
         //      m_maintVolumeSPS += maintVolume;*/
         //   }

         for (MapLayer::Iterator pt = point; pt < endPoint; pt++)
            {
            // Beachwidth changes based on a structure front slope 
            float beachWidth = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colBeachWidth, beachWidth);

            float duneCrest = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneCrest, duneCrest);

            /*float duneToe = 0.0f;
            m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneToe);*/


            /*if (isBPS)
            {*/
            if ((newCrest - duneCrest) > 0.0f)
               {
               beachWidth -= ((newCrest - duneCrest) / structureSlope);
               if ((duneToe / beachWidth) > 0.1f)
                  beachWidth = duneToe / 0.1f;

               // DuneCrest height is set to structure height
               m_pNewDuneLineLayer->SetData(pt, m_colDuneCrest, newCrest);
               m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, beachWidth);
               //   if (isBPS)
               m_pNewDuneLineLayer->SetData(pt, m_colMaintYearBPS, currentYear);
               m_pNewDuneLineLayer->SetData(pt, m_colCostMaintBPS, cost);

               /*   else
               m_pNewDuneLineLayer->SetData(pt, m_colMaintYearSPS, currentYear);*/

               // change easting of DuneToe since structure is maintained on seaward side
               m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);
               eastingToe -= ((newCrest - duneCrest) / structureSlope);
               m_pNewDuneLineLayer->SetData(pt, m_colEastingToe, eastingToe);

               Poly *pPoly = m_pNewDuneLineLayer->GetPolygon(pt);
               if (pPoly->GetVertexCount() > 0)
                  {
                  pPoly->m_vertexArray[0].x = eastingToe;
                  pPoly->InitLogicalPoints(m_pMap);
                  }
               //   }
               }
            }
         }
      }
   } // end MaintainStructure


void ChronicHazards::MaintainBPS2(int currentYear)
   {
   //for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
   //{
   //   //Do we need to increase the height of the BPS to keep up with increasing TWL?

   //   int lastMaintYear; //  year of the last height increase
   // //    float maxTWL;
   // //    float backshoreElev;//minimum elevation of the "first story" in the backshore (i.e. minimum elevation + height of the first story of a building"

   //   float duneCrest = 0.0f;
   //   float duneToe = 0.0f;
   //   int beachType = 0;

   //   // Moving Window of the yearly maximum TWL
   //   MovingWindow *twlMovingWindow = m_TWLArray.GetAt(point);

   //   // Retrieve the maxmum TWL within the designated window
   //   float movingMaxTWL = twlMovingWindow->GetMaxValue();

   //   // Retrieve the average TWL within the designated window
   //   float movingAvgTWL = twlMovingWindow->GetAvgValue();

   //   m_pNewDuneLineLayer->GetData(point, m_colDuneToe, duneToe);
   //   m_pNewDuneLineLayer->GetData(point, m_colDuneCrest, duneCrest);
   //   //    m_pNewDuneLineLayer->GetData(point, m_colBackshoreElev, backshoreElev);
   //   m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);

   //   //if ( ( duneCrest < movingAvgTWL || duneCrest < movingMaxTWL ) && beachType == BchT_RIPRAP_BACKED ) // && duneCrest < backshoreElev )
   //   if (duneCrest < movingMaxTWL && beachType == BchT_RIPRAP_BACKED) // && duneCrest < backshoreElev )
   //   {
   //      if ((lastMaintYear + m_maintFreqBPS) < currentYear)
   //      {
   //         float newCrest = 0.0f;
   //         float beachwidth = 0.0f;

   //         if (m_useMaxTWL == 1)
   //         {
   //            newCrest = movingMaxTWL;
   //            //add safety factor
   //            newCrest += m_safetyFactor;
   //         }
   //         else
   //         {
   //            newCrest = movingAvgTWL;
   //            //add safety factor
   //            newCrest += m_safetyFactor;
   //         }

   //         float BPSHeight = newCrest - duneToe;

   //         // Limited by Engineering Constraint
   //         if (BPSHeight > m_maxBPSHeight)
   //         {
   //            BPSHeight = m_maxBPSHeight;
   //            newCrest = duneToe + m_maxBPSHeight;
   //         }

   //         if (newCrest < duneCrest)
   //         {
   //            newCrest = duneCrest;
   //            BPSHeight = newCrest - duneToe;
   //         }

   //         // TODO put back ??
   //         //if (newCrest > backshoreElev)//limited to "first floor" of building
   //         //{
   //         //   newCrest = backshoreElev;
   //         //   BPSHeight = newCrest - duneToe;
   //         //}

   //         // newCrest = backshoreElev;
   //         BPSHeight = newCrest - duneToe;

   //         // DuneCrest height is set to structure height
   //         m_pNewDuneLineLayer->SetData(point, m_colDuneCrest, newCrest);

   //         // Beachwidth changes based on a 2:1 slope 
   //         m_pNewDuneLineLayer->GetData(point, m_colBeachWidth, beachwidth);
   //         if ((newCrest - duneCrest) > 0.0f)
   //         {

   //            beachwidth -= ((newCrest - duneCrest) * 1.0f);
   //            if ((duneToe / beachwidth) > 0.1f)
   //               beachwidth = duneToe / 0.1f;

   //            //  float BPSCost = (newCrest - duneCrest) * m_costs.BPS * pPoly->GetEdgeLength();;

   //            m_pNewDuneLineLayer->SetData(point, m_colBeachWidth, beachwidth);
   //            m_pNewDuneLineLayer->SetData(point, m_colBPSMaintYear, currentYear);

   //            /*if (Loc == 7)
   //            {

   //            m_RockBPSMaintenance += BPSCost;

   //            }
   //            if (Loc == 1)
   //            {

   //            m_NeskBPSMaintenance += BPSCost;

   //            }
   //            float priorBPSCost;
   //            m_pNewDuneLineLayer->GetData(i, m_colBPSCost, priorBPSCost);
   //            float addntlBPSCost = BPSCost + priorBPSCost;
   //            m_pNewDuneLineLayer->SetData(i, m_colBPSCost, addntlBPSCost);
   //            m_BPSMaintenance += BPSCost;
   //            }*/
   //         }
   //      }
   //   }
   //}

   } // end MaintainBPS

void ChronicHazards::NourishSPS(int currentYear)
   {
   // get height of cell which corresponds to length of shoreline
   //REAL cellHeight = m_pElevationGrid->GetGridCellHeight();

   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      double volume = 0.0;
      int nourishYear = 0;
      //   int builtYear = 0;

      double eastingToe = 0.0;
      double eastingToeSPS = 0.0;

      float duneToe = 0.0f;
      float duneToeSPS = 0.0f;
      float duneCrest = 0.0f;
      float beachWidth = 0.0f;
      float shorelineChangeRate = 0.0f;

      MapLayer::Iterator endPoint = point;
      //   bool cond = FindNourishExtent(builtYear, point, endPoint);
      bool isImpactThresholdExceeded = FindImpactExtent(point, endPoint, false);

      if (isImpactThresholdExceeded)
         {
         for (MapLayer::Iterator pt = point; pt < endPoint; pt++)
            {
            m_pNewDuneLineLayer->GetData(pt, m_colEastingToe, eastingToe);
            m_pNewDuneLineLayer->GetData(pt, m_colEastingToeSPS, eastingToeSPS);

            if ((eastingToe - eastingToeSPS) >= 3.0)
               {
               m_pNewDuneLineLayer->SetData(pt, m_colNourishYearSPS, nourishYear);

               if ((nourishYear + m_nourishFreq) < currentYear || nourishYear == 0)
                  {
                  m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneToe);
                  m_pNewDuneLineLayer->GetData(pt, m_colDuneCrest, duneCrest);

                  //  volume of sand needed along trapezoid face + volume of sand needed to restore beachwidth 
                  //  V = W * H * L - volume of triangular prism + volume of triangular prism
                  volume = (eastingToe - eastingToeSPS) * (duneCrest - duneToe) * m_cellHeight; // -0.5 * cellHeight * (eastingToe - eastingToeSPS) * (duneToe - duneToeSPS);
                                                                                                //}

                  m_pNewDuneLineLayer->GetData(pt, m_colNourishYearSPS, nourishYear);

                  m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneToe);
                  m_pNewDuneLineLayer->GetData(pt, m_colDuneCrest, duneCrest);

                  float bruunRuleSlope;
                  m_pNewDuneLineLayer->GetData(pt, m_colBeachWidth, beachWidth);
                  m_pNewDuneLineLayer->GetData(pt, m_colShorelineChange, shorelineChangeRate);
                  m_pNewDuneLineLayer->GetData(pt, m_colShoreface, bruunRuleSlope);

                  // determine how much duen has eroded

                  // determine change in sea level rise from previous year
                  float prevslr = 0.0f;
                  float slr = 0.0f;
                  int year = currentYear - 2010;
                  m_slrData.Get(0, year, slr);
                  if (currentYear > 2010)
                     m_slrData.Get(0, year - 1, prevslr);

                  // determine how much to widen beach 
                  float nourishSLR = 0.0f;
                  // nourish only when increasing slr (beach is narrowing)
                  if ((slr - prevslr) > 0)
                     nourishSLR = ((slr - prevslr) / bruunRuleSlope) * m_nourishFactor;

                  // nourish only when negative shoreline change rate (beach is narrowing)
                  float nourishSCRate = 0.0f;
                  if (shorelineChangeRate < 0.0f)
                     nourishSCRate = abs(shorelineChangeRate * m_nourishFactor);

                  // beachwidth increases with nourishment
                  float newBeachWidth = beachWidth + nourishSCRate + nourishSLR;

                  // Expand the beach by at least 3 meters
                  // nourishing the beach moves the easting of the MHW 
                  if ((newBeachWidth - beachWidth) < 3)
                     newBeachWidth += 3;
                  if ((duneToe / newBeachWidth) > 0.1f)
                     newBeachWidth = duneToe / 0.1f;

                  // easting location of MHW 
                  m_pNewDuneLineLayer->SetData(pt, m_colEastingShoreline, m_colEastingToe - newBeachWidth);

                  // Calculate added volume for a the front facing trapezoid
                  // 
                  volume += (0.5f * m_cellHeight * duneToe * (newBeachWidth - beachWidth));

                  if (volume < 0)
                     volume = 0;

                  float cost = float(volume * m_costs.nourishment);

                  // get budget info
                  PolicyInfo &pi = m_policyInfoArray[PI_NOURISH_SPS];

                  // enough funds in original allocation ?
                  bool passCostConstraint = true;
                  if (pi.HasBudget() && pi.m_availableBudget <= cost) //armorAllocationResidual >= cost)
                     passCostConstraint = false;

                  AddToPolicySummary(currentYear, PI_NOURISH_SPS, int(pt), m_costs.nourishment, cost, pi.m_availableBudget, float(volume), float(eastingToe - eastingToeSPS), passCostConstraint);

                  if (passCostConstraint)
                     {
                     // Annual County wide cost for building BPS
                     pi.IncurCost(cost);

                     int nourishFreq = 0;
                     m_pNewDuneLineLayer->GetData(pt, m_colNourishFreqSPS, nourishFreq);
                     nourishFreq += 1;

                     // Set new values in Duneline layer
                     //         m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, newBeachWidth);
                     m_pNewDuneLineLayer->SetData(pt, m_colNourishFreqSPS, nourishFreq);

                     //   float newBeachSlope = duneToe / newBeachWidth;

                     //   m_pNewDuneLineLayer->SetData(pt, m_colTranSlope, newBeachSlope);
                     m_pNewDuneLineLayer->SetData(pt, m_colNourishYearSPS, currentYear);
                     m_pNewDuneLineLayer->SetData(pt, m_colNourishVolSPS, volume);

                     // Calculate County wide metrics (volume, cost)
                     m_nourishVolumeSPS += volume;
                     m_nourishCostSPS += cost;
                     }
                  else
                     {   // don't construct
                     pi.m_unmetDemand += cost;
                     }
                  }
               }   // end of: if ( easting
            }   // end of: for (pt < endPoint)
         }
      } // end dune line points

   } // end Nourish

void ChronicHazards::NourishBPS(int currentYear, bool nourishByType)
   {
   // get height of cell which corresponds to length of shoreline
   //float cellHeight = m_pElevationGrid->GetGridCellHeight();

   int parcelIndex = 0;
   // ??? is there a nourish frequency for the whole county or location specific ???
   //      

   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      double volume = 0.0;
      int lastNourishYear = 0;

      float duneToe = 0.0f;
      float duneCrest = 0.0f;
      float beachWidth = 0.0f;
      float shorelineChangeRate = 0.0f;
      int beachType = -1;

      int randNum = 100;

      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
      //   m_pNewDuneLineLayer->GetData(point, m_colBeachType, parcelIndx);
      m_pNewDuneLineLayer->GetData(point, m_colNourishYearBPS, lastNourishYear);

      int addBPSYear = 0;
      m_pNewDuneLineLayer->GetData(point, m_colAddYearBPS, addBPSYear);

      if (addBPSYear == 2010)
         lastNourishYear = 2010;

      int yrsSinceNourished = currentYear - lastNourishYear;

      if (beachType == BchT_RIPRAP_BACKED)
         {
         if (yrsSinceNourished > m_nourishFreq)
            {
            MapLayer::Iterator endPoint = point;
            bool cond = FindNourishExtent(endPoint);

            if (cond)
               randNum = rand() % 100 + 1;

            if (true)  //randNum <= m_nourishPercentage)
               {
               for (MapLayer::Iterator pt = point; pt < endPoint; pt++)
                  {
                  m_pNewDuneLineLayer->GetData(pt, m_colDuneToe, duneToe);
                  m_pNewDuneLineLayer->GetData(pt, m_colDuneCrest, duneCrest);

                  float bruunRuleSlope;
                  m_pNewDuneLineLayer->GetData(pt, m_colBeachWidth, beachWidth);
                  m_pNewDuneLineLayer->GetData(pt, m_colShorelineChange, shorelineChangeRate);
                  m_pNewDuneLineLayer->GetData(pt, m_colShoreface, bruunRuleSlope);


                  // determine change in sea level rise from previous year
                  float prevslr = 0.0f;
                  float slr = 0.0f;
                  int year = currentYear - 2010;

                  m_slrData.Get(0, year, slr);
                  if (currentYear > 2010)
                     m_slrData.Get(0, year - 1, prevslr);

                  // determine how much to widen beach 
                  float nourishSLR = 0.0f;
                  // nourish only when increasing slr (beach is narrowing)
                  if ((slr - prevslr) > 0)
                     nourishSLR = ((slr - prevslr) / bruunRuleSlope) * m_nourishFactor;

                  // nourish only when negative shoreline change rate (beach is narrowing)
                  float nourishSCRate = 0.0f;
                  if (shorelineChangeRate < 0.0f)
                     nourishSCRate = abs(shorelineChangeRate * m_nourishFactor);

                  // beachwidth increases with nourishment
                  float newBeachWidth = beachWidth + nourishSCRate + nourishSLR;

                  // Expand the beach by at least 3 meters
                  // nourishing the beach moves the easting of the MHW 
                  if ((newBeachWidth - beachWidth) < 3)
                     newBeachWidth += 3;
                  if ((duneToe / newBeachWidth) > 0.1f)
                     newBeachWidth = duneToe / 0.1f;

                  // Calculate added sand volume for a triangular prism
                  volume = (0.5 * m_cellHeight * duneToe * (newBeachWidth - beachWidth));

                  if (volume > 0.0f)
                     {
                     float cost = float(volume * m_costs.nourishment);

                     // get budget infor for Constructing BPS policy
                     PolicyInfo &pi = m_policyInfoArray[PI_NOURISH_BY_TYPE];

                     // enough funds in original allocation ?
                     bool passCostConstraint = true;
                     if (pi.HasBudget() && pi.m_availableBudget <= cost) //armorAllocationResidual >= cost)
                        passCostConstraint = false;

                     AddToPolicySummary(currentYear, PI_NOURISH_BY_TYPE, int(pt), m_costs.nourishment, cost, pi.m_availableBudget, float(volume), float(newBeachWidth), passCostConstraint);

                     if (passCostConstraint)
                        {
                        // Annual County wide cost for building BPS
                        pi.IncurCost(cost);

                        int nourishFreq = 0;
                        m_pNewDuneLineLayer->GetData(pt, m_colNourishFreqBPS, nourishFreq);
                        nourishFreq += 1;

                        // easting location of MHW 
                        m_pNewDuneLineLayer->SetData(pt, m_colEastingShoreline, m_colEastingToe - newBeachWidth);
                        // Set new values in Duneline layer

                        m_pNewDuneLineLayer->SetData(pt, m_colBeachWidth, newBeachWidth);
                        m_pNewDuneLineLayer->SetData(pt, m_colNourishFreqBPS, nourishFreq);

                        float newBeachSlope = duneToe / newBeachWidth;

                        m_pNewDuneLineLayer->SetData(pt, m_colTranSlope, newBeachSlope);
                        m_pNewDuneLineLayer->SetData(pt, m_colNourishYearBPS, currentYear);
                        m_pNewDuneLineLayer->SetData(pt, m_colNourishVolBPS, volume);

                        // Calculate County wide metrics (volume, cost)
                        m_nourishVolumeBPS += volume;
                        m_nourishCostBPS += cost;
                        }   // end of: if (nourish)
                     else
                        { // don't nourish
                        pi.m_unmetDemand += cost;
                        }
                     }
                  }
               }
            }
         }

      } // end dune line points

   } // end Nourish

bool ChronicHazards::FindNourishExtent(MapLayer::Iterator &endPoint)
   {
   float buildBPSThreshold = m_idpyFactor * 365.0f;
   bool nourish = false;
   int beachType = -1;

   while (endPoint < m_pNewDuneLineLayer->End() && GetBeachType(endPoint) == BchT_RIPRAP_BACKED)
      {
      MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
      float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();

      if (movingAvgIDPY >= buildBPSThreshold)
         nourish = true;

      endPoint++;
      }
   endPoint--;

   return nourish;
   } // end FindNourishExtent


bool ChronicHazards::FindImpactExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint, bool isBPS)
   {
   bool nourish = false;

   float buildThreshold = 0.0f;
   int constructYrCol;


   if (isBPS)
      {
      buildThreshold = m_idpyFactor * 365.0f;
      constructYrCol = m_colAddYearBPS;
      }
   else // SPS
      {
      buildThreshold = m_idpyFactor * 365.0f;
      constructYrCol = m_colAddYearSPS;
      }

   int builtYear = 0;
   m_pNewDuneLineLayer->GetData(startPoint, constructYrCol, builtYear);

   int yr = builtYear;

   while (yr > 0 && yr == builtYear)
      {
      MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
      float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();

      if (movingAvgIDPY >= buildThreshold)
         {
         nourish = true;
         break;
         }

      //int bldgIndx = -1;
      //m_pNewDuneLineLayer->GetData(endPoint, m_colDuneBldgIndx, bldgIndx);

      //// Retrieve the Flooding Frequency of the Protected Building
      //if (bldgIndx != -1)
      //{
      //   MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndx);
      //   float floodFreq = floodMovingWindow->GetFreqValue();

      //   float floodThreshold = (float)m_floodCountSPS / m_windowLengthFloodHzrd;
      //   
      //   if (floodFreq >= floodThreshold)
      //      nourish = true;            
      //}

      endPoint++;
      m_pNewDuneLineLayer->GetData(endPoint, constructYrCol, yr);
      }

   return nourish;

   } // end FindImpactExtent

     //bool ChronicHazards::FindNourishExtent(int builtYear, MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint)
     //   {
     //   float buildThreshold = m_idpyFactor * 365.0f;
     //   bool nourish = false;
     //   //   int beachType = -1;
     //
     //   //   m_pNewDuneLineLayer->GetData(startPoint, m_colBeachType, beachType);
     //   int yr = builtYear;
     //
     //   /*while (beachType == BchT_RIPRAP_BACKED)*/
     //   while (yr > 0 && yr == builtYear)
     //      {
     //      MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
     //      float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();
     //
     //      if (movingAvgIDPY >= buildThreshold)
     //         nourish = true;
     //
     //      endPoint++;
     //      //   m_pNewDuneLineLayer->GetData(endPoint, m_colBeachType, beachType);
     //      m_pNewDuneLineLayer->GetData(endPoint, m_colAddYearSPS, yr);
     //      }
     //
     //   return nourish;
     //
     //   } // FindNourishExtent

void ChronicHazards::CompareBPSorNourishCosts(int currentYear)
   {
   //   float cellHeight = m_pNewDuneLineLayer->GetGridCellHeight();

   //float cellHeight = m_pElevationGrid->GetGridCellHeight();

   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
      {
      //For this policy in the less managed scenario, we need to compare the two methods, BPS and nourishment, to see which is cheaper
      float duneToe = 0.0f;
      float beachWidth = 0.0f;
      float shorelineChangeRate = 0.0f;

      m_pNewDuneLineLayer->GetData(point, m_colDuneToe, duneToe);
      m_pNewDuneLineLayer->GetData(point, m_colBeachWidth, beachWidth);
      m_pNewDuneLineLayer->GetData(point, m_colShorelineChange, shorelineChangeRate);

      // cost is determined over a 30 yr timeframe
      float newBeachWidth = beachWidth + (abs(shorelineChangeRate) * 30.0f);

      double volume = (0.5f * m_cellHeight * duneToe * (newBeachWidth - beachWidth));
      double nourishmentCost = volume * m_costs.nourishment;

      float newCrest;
      if (m_useMaxTWL == 1)
         {
         m_pNewDuneLineLayer->GetData(point, m_colYrMaxTWL, newCrest);
         //add safety factor
         newCrest += 1.0;
         }
      /*else
      {
      m_pNewDuneLineLayer->GetData(point, m_colYrAvgTWL, newCrest);
      newCrest += 1.0f;
      }*/

      float BPSHeight = newCrest - duneToe;

      if (BPSHeight > m_maxBPSHeight)
         {
         BPSHeight = m_maxBPSHeight;
         newCrest = duneToe + m_maxBPSHeight;
         }
      if (BPSHeight < m_minBPSHeight)
         {
         BPSHeight = m_minBPSHeight;
         newCrest = duneToe + m_minBPSHeight;
         }

      //float BPScost = BPSHeight * m_costs.BPS* pPoly->GetEdgeLength() + (BPSHeight * m_costs.BPS * pPoly->GetEdgeLength())*.02 * 30;
      double BPScost = BPSHeight * m_costs.BPS* m_cellHeight + (BPSHeight * m_costs.BPS * m_cellHeight) * 0.02 * 30.0;

      // ??? in this case perhaps should nourish even though not RIPRAP_BACKED
      if (BPScost > nourishmentCost)
         NourishBPS(currentYear, false);
      else
         ConstructBPS(currentYear);
      }
   } // end CompareBPSorNourishCost

     // If a new dwelling exists, it must be located on the safest site
void ChronicHazards::ConstructOnSafestSite(EnvContext *pEnvContext, bool inFloodPlain)
   {
   for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
      {
      float maxElevation;
      /*    float newElevation;*/

      int safestSiteRow = -1;
      int safestSiteCol = -1;

      //   float baseFloodElevation;
      //   int floodZone = -9999;

      CArray< int, int > ptArray;

      /*float popDensity;
      float population;
      int safeLoc;
      int numStructures;*/

      //   m_pIDULayer->GetData(idu, m_colMinElevation, minElevation);
      m_pIDULayer->GetData(idu, m_colMaxElevation, maxElevation);
      m_pIDULayer->GetData(idu, m_colSafestSiteRow, safestSiteRow);
      m_pIDULayer->GetData(idu, m_colSafestSiteCol, safestSiteCol);

      // assign safest site to centroid location
      Poly *pPoly = m_pIDULayer->GetPolygon(idu);
      Vertex v = pPoly->GetCentroid();
      REAL xSafeSite = v.x;
      REAL ySafeSite = v.y;

      // reassign safest site to highest in IDU
      if (safestSiteRow >= 0 && safestSiteCol >= 0)
         m_pElevationGrid->GetGridCellCenter(safestSiteRow, safestSiteCol, xSafeSite, ySafeSite);

      //   m_pIDULayer->GetData(idu, m_colBaseFloodElevation, baseFloodElevation);

      /*   if (inFloodPlain)
      {
      m_pIDULayer->GetData(idu, m_colFloodZone, floodZone);
      }
      */
      // Construct new buildings on safest site
      DeltaArray *pDeltaArray = pEnvContext->pDeltaArray;
      for (INT_PTR i = pEnvContext->firstUnseenDelta; i < pDeltaArray->GetCount(); i++)
         {
         DELTA &delta = pDeltaArray->GetAt(i);

         int numNewBldgs = 0;

         if (delta.col == m_colNDU)
            numNewBldgs = delta.newValue.GetInt() - delta.oldValue.GetInt();

         // new buildings must be located on the safest site
         if (numNewBldgs > 0)
            {
            // get Buildings in this IDU
            int numBldgs = m_pIduBuildingLkup->GetPointsFromPolyIndex(idu, ptArray);

            for (int i = 0; i < numBldgs; i++)
               {
               Poly *pPoly = m_pBldgLayer->GetPolygon(ptArray[i]);

               //   int thisSize = pPoly->m_vertexArray.GetSize();

               pPoly->m_vertexArray[0].x = xSafeSite;
               pPoly->m_vertexArray[0].y = ySafeSite;

               // should I put these in the Building Layer ???
               /*m_pBldgLayer->SetData(ptArray[ i ], m_colBldgMaxElev, maxElevation); */
               //   m_pBldgLayer->SetData(ptArray[ i ], m_colBldgBFE, baseFloodElevation);
               m_pBldgLayer->SetData(ptArray[i], m_colBldgSafeSiteYr, pEnvContext->currentYear);

               //   UpdateIDU(pEnvContext, idu, m_colSafeSiteYear, pEnvContext->currentYear, ADD_DELTA);

               /*double xCoord = 0;
               double yCoord = 0;*/

               /*xCoord = pPoly->m_vertexArray[ i ].x;
               yCoord = pPoly->m_vertexArray[ i ].y;*/

               /*float dx = xSafe - xCoord;
               float dy = ySafe - yCoord;
               pPoly->Translate(dx, dy); */

               // is this needed?
               pPoly->InitLogicalPoints(m_pMap);

               } // end each new building
            } // end new buildings added
         } // end delta array loop

      } // end looping through IDUs

   } // end ConstructOnSafestSite


void ChronicHazards::RaiseOrRelocateBldgToSafestSite(EnvContext *pEnvContext)
   {
   if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
      {
      // Raise Infrastructure to BFE or Raise existing homes/buildings to BFE
      // OR Relocate existing homes/buildings to safest site   
      for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
         {
         float maxElevation;
         //    float newElevation;

         int safestSiteRow = -1;
         int safestSiteCol = -1;

         float baseFloodElevation;
         int floodZone = -9999;

         CArray< int, int > bldgIndices;
         CArray< int, int > infraIndices;

         /*float popDensity;
         float population;
         int safeLoc;
         int numStructures;*/

         //   m_pIDULayer->GetData(idu, m_colMinElevation, minElevation);
         m_pIDULayer->GetData(idu, m_colMaxElevation, maxElevation);
         m_pIDULayer->GetData(idu, m_colSafestSiteRow, safestSiteRow);
         m_pIDULayer->GetData(idu, m_colSafestSiteCol, safestSiteCol);

         //// should I put these in the Building Layer ???
         //m_pBldgLayer->GetData(bldg, m_colBldgMaxElev, maxElevation);
         //m_pBldgLayer->GetData(bldg, m_colBldgBFE, baseFloodElevation);
         //m_pBldgLayer->GetData(bldg, m_colBldgSafeSiteYr, pEnvContext->currentYear);

         REAL xSafeSite = 0.0;
         REAL ySafeSite = 0.0;

         if (safestSiteRow >= 0 && safestSiteCol >= 0)
            m_pElevationGrid->GetGridCellCenter(safestSiteRow, safestSiteCol, xSafeSite, ySafeSite);

         m_pIDULayer->GetData(idu, m_colBaseFloodElevation, baseFloodElevation);

         // get Buildings in this IDU
         int numBldgs = m_pIduBuildingLkup->GetPointsFromPolyIndex(idu, bldgIndices);

         for (int i = 0; i < numBldgs; i++)
            {
            int bldgIndex = bldgIndices[i];
            ASSERT(bldgIndex < m_pBldgLayer->GetPolygonCount());

            Poly *pPoly = m_pBldgLayer->GetPolygon(bldgIndex);

            // Set BFE of building

            m_pBldgLayer->SetData(bldgIndex, m_colBldgBFE, baseFloodElevation);

            int safeSiteYr = 0;
            m_pBldgLayer->GetData(bldgIndex, m_colBldgSafeSiteYr, safeSiteYr);

            int raiseToBFEYr = 0;
            m_pBldgLayer->GetData(bldgIndex, m_colBldgBFEYr, raiseToBFEYr);

            // get moving windows and check for flooding
            MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);
            float floodFreq = floodMovingWindow->GetFreqValue();

            REAL xCoord = 0.0;
            REAL yCoord = 0.0;
            m_pBldgLayer->GetPointCoords(bldgIndex, xCoord, yCoord);

            // Do not relocate closer to coast
            if (xSafeSite < xCoord)
               {
               m_pIDULayer->m_readOnly = false;
               safestSiteCol = -1;
               safestSiteRow = -1;
               m_pIDULayer->SetData(idu, m_colSafestSiteRow, safestSiteRow);
               m_pIDULayer->SetData(idu, m_colSafestSiteCol, safestSiteCol);
               // uncomment to not allow new construction closer to coast
               // m_pIDULayer->SetData(idu, m_colSafeSite, 0);
               m_pIDULayer->m_readOnly = true;
               }

            // Raise the building to the BFE if the structure has NOT been 
            // raised and is not on the safest site
            if (raiseToBFEYr == 0 && safeSiteYr == 0)
               {
               /*if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
               {*/
               if (floodFreq >= (float(m_bfeCount) / m_windowLengthFloodHzrd))
                  {
                  /*REAL xCoord = 0.0;
                  REAL yCoord = 0.0;
                  m_pBldgLayer->GetPointCoords(ptrArrayIndex, xCoord, yCoord);*/
                  int row = -1;
                  int col = -1;
                  m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, row, col);
                  float elev = 0.0f;

                  if ((row >= 0 && row < m_numRows) && (col >= 0 && col < m_numCols))
                     {
                     m_pElevationGrid->GetData(row, col, elev);

                     // is the adj base flood elevation higher than the DEM elevation at this site?
                     // meaning, is this site too low to withstand flooding?
                     if ((baseFloodElevation + 0.0f) >= elev)   /// +1????
                        {
                        float value = 0;
                        m_pBldgLayer->GetData(bldgIndex, m_colBldgValue, value);

                        float cost = value * m_raiseRelocateSafestSiteRatio;

                        // get budget infor for raise/relocate policy
                        PolicyInfo &pi = m_policyInfoArray[PI_RAISE_RELOCATE_TO_SAFEST_SITE];
                        bool passCostConstraint = true;

                        // enough funds in original allocation ?
                        if (pi.HasBudget() && pi.m_availableBudget <= cost)
                           passCostConstraint = false;

                        AddToPolicySummary(pEnvContext->currentYear, PI_RAISE_RELOCATE_TO_SAFEST_SITE, bldgIndex, m_raiseRelocateSafestSiteRatio, cost, pi.m_availableBudget, value, 0, passCostConstraint);

                        if (passCostConstraint)
                           {
                           pi.IncurCost(cost);

                           // Building is below the BFE elevation so raise it to the BFE + 1.0, noting year
                           m_pBldgLayer->SetData(bldgIndex, m_colBldgBFEYr, pEnvContext->currentYear);
                           m_pBldgLayer->SetData(bldgIndex, m_colBldgBFE, baseFloodElevation + 1.0f);
                           floodMovingWindow->Clear();
                           }
                        else
                           { // don't nourish
                           pi.m_unmetDemand += cost;
                           }
                        }
                     else   // site is above base flood elevation, move to safest site
                        {
                        // BFE is below current elevation so move to safest site, noting year
                        if (safestSiteRow >= 0 && safestSiteCol >= 0)
                           {
                           float value = 0;
                           m_pBldgLayer->GetData(bldgIndex, m_colBldgValue, value);

                           float cost = value * m_raiseRelocateSafestSiteRatio;

                           // get budget infor for Constructing BPS policy
                           PolicyInfo &pi = m_policyInfoArray[PI_RAISE_RELOCATE_TO_SAFEST_SITE];
                           bool passCostConstraint = true;

                           // enough funds in original allocation ?
                           if (pi.HasBudget() && pi.m_availableBudget <= cost)
                              passCostConstraint = false;

                           AddToPolicySummary(pEnvContext->currentYear, PI_RAISE_RELOCATE_TO_SAFEST_SITE, bldgIndex, m_raiseRelocateSafestSiteRatio, cost, pi.m_availableBudget, value, 1, passCostConstraint);

                           if (passCostConstraint)
                              {
                              pi.IncurCost(cost);
                              pPoly->m_vertexArray[0].x = xSafeSite;
                              pPoly->m_vertexArray[0].y = ySafeSite;
                              m_pBldgLayer->SetData(bldgIndex, m_colBldgSafeSiteYr, pEnvContext->currentYear);
                              floodMovingWindow->Clear();
                              }
                           else
                              { // don't nourish
                              pi.m_unmetDemand += cost;
                              }
                           }
                        }
                     }
                  }
               //   }

               }
            // Building has been raised but not yet at safest site
            else if (raiseToBFEYr > 0 && safeSiteYr == 0)
               {
               if (pEnvContext->currentYear >= (raiseToBFEYr + m_windowLengthFloodHzrd - 1))
                  {
                  if (floodFreq >= (float(m_ssiteCount) / m_windowLengthFloodHzrd))
                     {
                     // move building to safest site
                     if (safestSiteRow >= 0 && safestSiteCol >= 0)
                        {
                        float value = 0;
                        m_pBldgLayer->GetData(bldgIndex, m_colBldgValue, value);

                        float cost = value * m_raiseRelocateSafestSiteRatio;

                        // get budget infor for Constructing BPS policy
                        PolicyInfo &pi = m_policyInfoArray[PI_RAISE_RELOCATE_TO_SAFEST_SITE];
                        bool passCostConstraint = true;

                        // enough funds in original allocation ?
                        if (pi.HasBudget() && pi.m_availableBudget <= cost)
                           passCostConstraint = false;

                        AddToPolicySummary(pEnvContext->currentYear, PI_RAISE_RELOCATE_TO_SAFEST_SITE, bldgIndex, m_raiseRelocateSafestSiteRatio, cost, pi.m_availableBudget, value, 2, passCostConstraint);

                        if (passCostConstraint)
                           {
                           pi.IncurCost(cost);

                           pPoly->m_vertexArray[0].x = xSafeSite;
                           pPoly->m_vertexArray[0].y = ySafeSite;
                           m_pBldgLayer->SetData(bldgIndex, m_colBldgSafeSiteYr, pEnvContext->currentYear);
                           floodMovingWindow->Clear();
                           }
                        else
                           { // didn't pass cost constraint, so add to unmet demand
                           pi.m_unmetDemand += cost;
                           }
                        }
                     }
                  }
               }
            else if (safeSiteYr > 0)
               {
               if (pEnvContext->currentYear >= (safeSiteYr + m_windowLengthFloodHzrd - 1))
                  {
                  if (floodFreq >= (float(m_ssiteCount) / m_windowLengthFloodHzrd))
                     {
                     // flag that no buildings allowed in this IDU
                     m_pIDULayer->m_readOnly = false;
                     m_pIDULayer->SetData(idu, m_colPopDensity, 0);
                     m_pIDULayer->SetData(idu, m_colSafeSite, 0);

                     /*m_pIDULayer->SetData(idu, m_colPopCap, 0);
                     m_pIDULayer->SetData(idu, m_colPopulation, 0);
                     m_pIDULayer->SetData(idu, m_colLandValue, 0);
                     m_pIDULayer->SetData(idu, m_colImpValue, 0);*/
                     }
                  }
               }

            //   delete floodMovingWindow;
            pPoly->InitLogicalPoints(m_pMap);

            } // end each buildings
         } // end each IDU
      }
   } // end RaiseOrRelocateToSafestSite


void ChronicHazards::RaiseInfrastructure(EnvContext *pEnvContext)
   {
   if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
      {
      bool runRaiseInfrastructurePolicy = (m_runRaiseInfrastructurePolicy == 1) ? true : false;

      // Raise Infrastructure to BFE
      for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
         {
         float baseFloodElevation;
         CArray< int, int > infraIndices;
         m_pIDULayer->GetData(idu, m_colBaseFloodElevation, baseFloodElevation);

         // get Infrastructure in this IDU
         int numInfra = m_pPolygonInfraPointLkup->GetPointsFromPolyIndex(idu, infraIndices);

         for (int i = 0; i < numInfra; i++)
            {
            int critical = 0;
            Poly *pPoly = m_pInfraLayer->GetPolygon(infraIndices[i]);
            int infraIndex = infraIndices[i];

            m_pInfraLayer->GetData(infraIndex, m_colInfraCritical, critical);
            bool isCritical = (critical == 1) ? true : false;

            int raiseToBFEYr = 0;
            m_pInfraLayer->GetData(infraIndex, m_colInfraBFEYr, raiseToBFEYr);

            if (isCritical)
               {
               if (runRaiseInfrastructurePolicy)
                  {
                  // and it hasn't been raised
                  if (raiseToBFEYr == 0 && baseFloodElevation > 0.0f)
                     {
                     MovingWindow *floodMovingWindow = m_floodInfraFreqArray.GetAt(infraIndex);
                     float floodFreq = floodMovingWindow->GetFreqValue();

                     if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
                        {
                        if (floodFreq >= (float(m_bfeCount) / (float)m_windowLengthFloodHzrd))
                           {
                           float value = 0;
                           m_pInfraLayer->GetData(infraIndex, m_colInfraValue, value);

                           float cost = value * m_raiseRelocateSafestSiteRatio;

                           // get budget infor for Constructing BPS policy
                           PolicyInfo &pi = m_policyInfoArray[PI_RAISE_INFRASTRUCTURE];
                           bool passCostConstraint = true;

                           // enough funds in original allocation ?
                           if (pi.HasBudget() && pi.m_availableBudget <= cost)
                              passCostConstraint = false;

                           AddToPolicySummary(pEnvContext->currentYear, PI_RAISE_INFRASTRUCTURE, infraIndex, m_raiseRelocateSafestSiteRatio, cost, pi.m_availableBudget, value, 0, passCostConstraint);

                           if (passCostConstraint)
                              {
                              pi.IncurCost(cost);

                              m_pInfraLayer->SetData(infraIndex, m_colInfraBFE, baseFloodElevation + 1.0f);
                              m_pInfraLayer->SetData(infraIndex, m_colInfraBFEYr, pEnvContext->currentYear);
                              }
                           else
                              { // didn't pass cost constraint, so add to unmet demand
                              pi.m_unmetDemand += cost;
                              }
                           }
                        }
                     //      delete floodMovingWindow;
                     }
                  }
               else
                  {
                  MovingWindow *floodMovingWindow = m_floodInfraFreqArray.GetAt(infraIndex);
                  float floodFreq = floodMovingWindow->GetFreqValue();

                  if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
                     {
                     if (floodFreq >= float(m_ssiteCount) / (float)m_windowLengthFloodHzrd)
                        {
                        int floodYr = 0;
                        m_pInfraLayer->GetData(infraIndex, m_colInfraFloodYr, floodYr);
                        if (floodYr == 0)
                           m_pInfraLayer->SetData(infraIndex, m_colInfraFloodYr, pEnvContext->currentYear);
                        m_pInfraLayer->SetData(infraIndex, m_colInfraFlooded, 1);
                        }
                     }
                  }
               }
            }

         } // end each IDU
      }
   } // end RaiseInfrastructure


void ChronicHazards::RemoveBldgFromHazardZone(EnvContext *pEnvContext)
   {
   bool buildIndex = false;

   // Relocate existing homes/buildings from hazard zone (100-yr flood plain)
   for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
      {
      int one100YrFloodHZ = 0;

      CArray< int, int > bldgIndices;
      CArray< int, int > infraIndices;

      m_pIDULayer->GetData(idu, m_colIDUFloodZoneCode, one100YrFloodHZ);
      bool is100YrFloodHazardZone = (one100YrFloodHZ == FHZ_ONEHUNDREDYR) ? true : false;

      // get building count in this idu
      int ndu = 0;
      m_pIDULayer->GetData(idu, m_colNDU, ndu);
      int newDu = 0;
      m_pIDULayer->GetData(idu, m_colNEWDU, newDu);

      // get Buildings in this IDU
      int numBldgs = m_pIduBuildingLkup->GetPointsFromPolyIndex(idu, bldgIndices);

      for (int i = 0; i < numBldgs; i++)
         {
         int yr = 0;

         Poly *pPoly = m_pBldgLayer->GetPolygon(bldgIndices[i]);
         int bldgIndex = bldgIndices[i];

         // get moving windows and check for flooding
         MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);
         float floodFreq = floodMovingWindow->GetFreqValue();

         // try this
         m_pBldgLayer->GetData(bldgIndex, m_colBldgFlooded, floodFreq);

         // Remove (relocate to unknown location) building from the IDU hazard zone if it is impacted
         /*   if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
         {*/
         if ((floodFreq >= (1.0f / m_windowLengthFloodHzrd)) && is100YrFloodHazardZone)
            {
            m_pBldgLayer->GetData(bldgIndex, m_colBldgRemoveYr, yr);

            if (yr == 0)
               {
               // Determine cost of a section along the SPS
               // Cost units : $ per cubic meter
               float value = 0;
               m_pBldgLayer->GetData(bldgIndex, m_colBldgValue, value);

               float cost = value * m_removeFromHazardZoneRatio;

               // get budget info for Remove from Hazard Zone policy
               PolicyInfo &pi = m_policyInfoArray[PI_REMOVE_BLDG_FROM_HAZARD_ZONE];
               bool passCostConstraint = true;

               // enough funds in original allocation ?
               if (pi.HasBudget() && pi.m_availableBudget <= cost)
                  passCostConstraint = false;

               AddToPolicySummary(pEnvContext->currentYear, PI_REMOVE_BLDG_FROM_HAZARD_ZONE, bldgIndex, m_removeFromHazardZoneRatio, cost, pi.m_availableBudget, value, 0, passCostConstraint);

               if (passCostConstraint)
                  {
                  pi.IncurCost(cost);

                  // track remove building count
                  m_noBldgsRemovedFromHazardZone += 1;

                  // write year and remove building in building layer
                  m_pBldgLayer->SetData(bldgIndex, m_colBldgRemoveYr, pEnvContext->currentYear);
                  //m_pBldgLayer->SetData(ptrArrayIndex, m_colBldgFloodHZ, 0);
                  floodMovingWindow->Clear();

                  // remove from layer
                  //      m_pBldgLayer->RemovePolygon(pPoly, false);
                  //      buildIndex = true;

                  // descrease the number new buildings in hazard zone
                  int bldgNdu = 0;
                  m_pBldgLayer->GetData(bldgIndex, m_colBldgNEWDU, bldgNdu);
                  if (bldgNdu > 0)
                     {
                     m_pBldgLayer->GetData(bldgIndex, m_colBldgNDU, --bldgNdu);
                     }

                  int bldgNewDu = 0;
                  m_pBldgLayer->GetData(bldgIndex, m_colBldgNEWDU, bldgNewDu);
                  if (bldgNewDu > 0)
                     {
                     m_pBldgLayer->GetData(bldgIndex, m_colBldgNEWDU, --bldgNewDu);
                     }

                  // IDU layer  - write year removed, remove population, and insure safe site flag not set
                  m_pIDULayer->m_readOnly = false;
                  m_pIDULayer->SetData(idu, m_colRemoveBldgYr, pEnvContext->currentYear);
                  m_pIDULayer->SetData(idu, m_colPopDensity, 0);
                  m_pIDULayer->SetData(idu, m_colSafeSite, 0);
                  if (ndu > 0)
                     {
                     m_pIDULayer->SetData(idu, m_colNDU, --ndu);
                     }

                  if (newDu > 0)
                     {
                     m_pIDULayer->SetData(idu, m_colNEWDU, --newDu);

                     }
                  }
               else
                  { // didn't pass cost constraint, so add to unmet demand
                  pi.m_unmetDemand += cost;
                  }
               }
            }
         //   }

         pPoly->InitLogicalPoints(m_pMap);
         } // end each buildings
      } // end each IDU
   } // end RemoveFromHazardZone


void ChronicHazards::RemoveInfraFromHazardZone(EnvContext *pEnvContext)
   {
   // Relocate existing homes/buildings from hazard zone (100-yr flood plain)
   for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
      {
      CArray< int, int > infraIndices;
      // see if infrastructure is impacted by flooding
      float baseFloodElevation = 0.0f;
      m_pIDULayer->GetData(idu, m_colBaseFloodElevation, baseFloodElevation);
      int numInfra = m_pPolygonInfraPointLkup->GetPointsFromPolyIndex(idu, infraIndices);

      for (int i = 0; i < numInfra; i++)
         {
         int critical = 0;
         Poly *pPoly = m_pInfraLayer->GetPolygon(infraIndices[i]);
         int infraIndex = infraIndices[i];

         m_pInfraLayer->GetData(infraIndex, m_colInfraCritical, critical);
         bool isCritical = (critical == 1) ? true : false;

         int raiseToBFEYr = 0;
         m_pInfraLayer->GetData(infraIndex, m_colInfraBFEYr, raiseToBFEYr);

         if (isCritical)
            {
            // and it hasn't been raised
            if (raiseToBFEYr == 0 && baseFloodElevation > 0.0f)
               {
               MovingWindow *floodMovingWindow = m_floodInfraFreqArray.GetAt(infraIndex);
               float floodFreq = floodMovingWindow->GetFreqValue();

               if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
                  {
                  if (floodFreq >= float(1.0f / m_windowLengthFloodHzrd))
                     {
                     float value = 0;
                     m_pInfraLayer->GetData(infraIndex, m_colInfraValue, value);

                     float cost = value * m_removeFromHazardZoneRatio;

                     // get budget infor for Constructing BPS policy
                     PolicyInfo &pi = m_policyInfoArray[PI_REMOVE_INFRA_FROM_HAZARD_ZONE];
                     bool passCostConstraint = true;

                     // enough funds in original allocation ?
                     if (pi.HasBudget() && pi.m_availableBudget <= cost) //armorAllocationResidual >= cost)
                        passCostConstraint = false;

                     AddToPolicySummary(pEnvContext->currentYear, PI_REMOVE_INFRA_FROM_HAZARD_ZONE, infraIndex, m_removeFromHazardZoneRatio, cost, pi.m_availableBudget, value, 0, passCostConstraint);

                     if (passCostConstraint)
                        {
                        pi.IncurCost(cost);

                        m_pInfraLayer->SetData(infraIndex, m_colInfraBFE, baseFloodElevation + 1.0f);
                        m_pInfraLayer->SetData(infraIndex, m_colInfraBFEYr, pEnvContext->currentYear);
                        }
                     else
                        { // didn't pass cost constraint, so add to unmet demand
                        pi.m_unmetDemand += cost;
                        }
                     }
                  }
               }
            }
         }

      } // end each IDU
   } // end RemoveFromHazardZone

void ChronicHazards::RemoveFromSafestSite()
   {
   /*for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
   {

   if ( building is flooded x times ?)
   {
   delete bldgs in IDU from bldgLayer

   m_pIDULayer->SetData(idu, m_colPopDensity, 0);
   m_pIDULayer->SetData(idu, m_colPopCap, 0);
   m_pIDULayer->SetData(idu, m_colPopulation, 0);
   m_pIDULayer->SetData(idu, m_colLandValue, 0);
   m_pIDULayer->SetData(idu, m_colImpValue, 0);
   }
   } */
   } // end RemoveFromSafestSite


bool ChronicHazards::LoadXml(LPCTSTR filename)
   {
   CString fullPath;
   PathManager::FindPath(filename, fullPath);

   TiXmlDocument doc;
   bool ok = doc.LoadFile(fullPath);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   // start iterating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // <chronic_hazards>
   if (pXmlRoot == NULL)
      return false;

   int flags = -1;
   if (pXmlRoot->Attribute("run_flags", &flags) != NULL)
      m_runFlags = flags;

   // envx file directory, slash-terminated
   CString projDir = PathManager::GetPath(PM_PROJECT_DIR);
   CString iduDir = PathManager::GetPath(PM_IDU_DIR);

   TiXmlElement *pXmlTWL = pXmlRoot->FirstChildElement("twl");
   if (pXmlTWL == NULL)
      return false;

   //  LPTSTR period = NULL;
   LPTSTR erosionInputDir = NULL;
   LPTSTR twlInputDir = NULL;
   LPTSTR city = NULL;

   XML_ATTR twlAttrs[] = { 
      // attr              type          address                           isReq  checkCol
      { "debug",             TYPE_INT,     &m_debug,                        true,    0 },
      { "writeRBF",          TYPE_INT,     &m_writeRBF,                     true,    0 },
      { "writebuoy",         TYPE_INT,     &m_writeDailyBouyData,           true,    0 },
      { "findSafeSiteCell",  TYPE_INT,     &m_findSafeSiteCell,             true,    0 },
      { "erosionInputDir",   TYPE_STRING,  &erosionInputDir,                true,    0 },
      { "twlInputDir",       TYPE_STRING,  &twlInputDir,                    true,    0 },
      { "transect_lookup",   TYPE_CSTRING, &m_transectLookupFile,           true,    0 },
      { "bathymetry",        TYPE_CSTRING, &m_bathyFiles,                   true,    0 },
      { "exportMapInterval", TYPE_INT,     &m_exportMapInterval,            true,    0 },
      { "simulationCount",   TYPE_INT,     &m_simulationCount,              true,    0 },
      { "inletFactor",       TYPE_FLOAT,   &m_inletFactor,                  true,    0 },
      { "TWLPeriod",         TYPE_INT,     &m_windowLengthTWL,              true,    0 },
      { "EroHzrdPeriod",     TYPE_INT,     &m_windowLengthEroHzrd,          true,    0 },
      { "ErosionCount",      TYPE_INT,     &m_eroCount,                     true,    0 },
      { "FloodHzrdPeriod",   TYPE_INT,     &m_windowLengthFloodHzrd,        true,    0 },
      { "BFECount",          TYPE_INT,     &m_bfeCount,                     true,    0 },
      { "SSiteCount",        TYPE_INT,     &m_ssiteCount,                   true,    0 },
      { "minSlopeThresh",    TYPE_FLOAT,   &m_slopeThresh,                  true,    0 },
      { "maxFloodArea",      TYPE_FLOAT,   &m_maxArea,                      true,    0 },
      { "accessThresh",      TYPE_FLOAT,   &m_accessThresh,                 true,    0 },
      { "constTD",           TYPE_FLOAT,   &m_constTD,                      true,    0 },
      { "shorelineLength",   TYPE_FLOAT,   &m_shorelineLength,              true,    0 },
      { "runSpatialBayTWL",  TYPE_INT,     &m_runSpatialBayTWL,             true,    0 },
      { NULL,                TYPE_NULL,    NULL,                            false,   0 } };

   if (TiXmlGetAttributes(pXmlTWL, twlAttrs, filename) == false)
      {
      CString msg;
      msg.Format(_T("ChronicHazards: Misformed <twl> element while reading in %s"), filename);
      Report::ErrorMsg(msg);
      return false;
      }

   //m_cityDir = city;
   // add "PolyGridLookups" directory to the Envision search paths
   CString pglPath = projDir + m_cityDir + "\\" + _T("PolyGridMaps");
   PathManager::AddPath(pglPath);
   m_twlInputDir = iduDir + twlInputDir + '/';
   CString erosionInputPath = projDir + erosionInputDir;

   PathManager::AddPath(m_twlInputDir);
   PathManager::AddPath(erosionInputPath);

   TiXmlElement *pXmlBatesPolicy = pXmlRoot->FirstChildElement("Bates");
   if (pXmlBatesPolicy == NULL)
      return false;

   XML_ATTR batesAttrs[] = {
      // attr               type           address                  isReq    checkCol
      { "city",            TYPE_STRING,  &city,                          false,   0 },
      { "runBayFloodModel",   TYPE_INT,     &m_runBayFloodModel,         true,    0 },
      { "runBatesFlood",      TYPE_INT,     &m_runBatesFlood,            true,    0 },
      { "BatesTimeStep",      TYPE_FLOAT,   &m_timeStep,                 true,    0 },
      { "BatesTimeDuration",  TYPE_FLOAT,   &m_batesFloodDuration,       true,    0 },
      { "ManningCoefficient", TYPE_FLOAT,   &m_ManningCoeff,             true,    0 },
      { NULL,                 TYPE_NULL,    NULL,                        false,   0 } };

   if (TiXmlGetAttributes(pXmlBatesPolicy, batesAttrs, filename) == false)
      return false;

   m_cityDir = city;

   TiXmlElement *pXmlBPSPolicy = pXmlRoot->FirstChildElement("bps");
   if (pXmlBPSPolicy == NULL)
      return false;

   XML_ATTR bpsAttrs[] = {
      // attr            type           address                          isReq    checkCol
      { "MinBPSHeight",      TYPE_FLOAT,   &m_minBPSHeight,                 true,    0 },
      { "MaxBPSHeight",      TYPE_FLOAT,   &m_maxBPSHeight,                 true,    0 },
      { "MinBPSRoughness",   TYPE_FLOAT,   &m_minBPSRoughness,              true,    0 },
      { "SlopeBPS",          TYPE_FLOAT,   &m_slopeBPS,                     true,    0 },
      { "MaintFreqBPS",      TYPE_INT,     &m_maintFreqBPS,                 true,    0 },
      //{ "BPSFloodCount",      TYPE_INT,    &m_floodCountBPS,              true,   0 },
      { "SafetyFactor",      TYPE_FLOAT,   &m_safetyFactor,                 true,    0 },
      { "IdpyFactor",        TYPE_FLOAT,   &m_idpyFactor,                   true,    0 },
      { "RadiusOfErosion",   TYPE_FLOAT,   &m_radiusOfErosion,              true,    0 },
      { "FloodCount",         TYPE_INT,    &m_floodCountBPS,              true,   0 },
      { "DistToProtectBldg", TYPE_FLOAT,   &m_minDistToProtecteBldg,           true,    0 },
      //   { "ConstructQuery",    TYPE_STRING,  &m_constructQueryStr,           true,    0 ),
      //   { "MaintainQuery",     TYPE_STRING,  &m_maintainQueryStr,           true,    0 ),
      { NULL,               TYPE_NULL,    NULL,                            false,   0 } };

   if (TiXmlGetAttributes(pXmlBPSPolicy, bpsAttrs, filename) == false)
      return false;

   TiXmlElement *pXmlNourishPolicy = pXmlRoot->FirstChildElement("nourish");
   if (pXmlNourishPolicy == NULL)
      return false;

   XML_ATTR nourishAttrs[] = { // attr            type           address                          isReq    checkCol
      { "NourishmentFreq",      TYPE_INT,     &m_nourishFreq,                  true,    0 },
      { "NourishmentFactor",      TYPE_INT,     &m_nourishFactor,                true,    0 },
      { "NourishmentPercentage",   TYPE_INT,     &m_nourishPercentage,            true,    0 },
      { NULL,               TYPE_NULL,    NULL,                            false,   0 } };

   if (TiXmlGetAttributes(pXmlNourishPolicy, nourishAttrs, filename) == false)
      return false;

   TiXmlElement *pXmlSPSPolicy = pXmlRoot->FirstChildElement("sps");
   if (pXmlSPSPolicy == NULL)
      return false;

   XML_ATTR SPSAttrs[] = { 
      // attr                  type           address                     isReq    checkCol
      { "AvgBackSlopeDune",  TYPE_FLOAT,     &m_avgBackSlope,       false,   0 },
      { "AvgFrontSlopeDune", TYPE_FLOAT,     &m_avgFrontSlope,      false,   0 },
      { "AvgDuneCrest",      TYPE_FLOAT,     &m_avgDuneCrest,       false,   0 },
      { "AvgDuneHeel",       TYPE_FLOAT,     &m_avgDuneHeel,        false,   0 },
      { "AvgDuneToe",        TYPE_FLOAT,     &m_avgDuneToe,         false,   0 },
      { "AvgDuneWidth",      TYPE_FLOAT,     &m_avgDuneWidth,       false,   0 },
      { "RebuildFactor",     TYPE_INT,       &m_rebuildFactorSPS,   true,    0 },
      { "FloodCount",        TYPE_INT,       &m_floodCountSPS,      true,    0 },
      { NULL,               TYPE_NULL,       NULL,                  false,   0 } };

   if (TiXmlGetAttributes(pXmlSPSPolicy, SPSAttrs, filename) == false)
      return false;

   TiXmlElement *pXmlTWLcosts = pXmlRoot->FirstChildElement("costs");
   if (pXmlTWLcosts == NULL)
      return false;

   XML_ATTR costAttrs[] = { 
      // attr              type           address                          isReq    checkCol
      { "TotalBudget",     TYPE_FLOAT,    &m_totalBudget,                 true,    0 },
      { "nourishment",     TYPE_FLOAT,    &m_costs.nourishment,            true,    0 },
      { "BPS",             TYPE_FLOAT,    &m_costs.BPS,                    true,    0 },
      { "BPSMaint",        TYPE_FLOAT,    &m_costs.BPSMaint,               true,    0 },
      { "SPS",             TYPE_FLOAT,    &m_costs.SPS,                    true,    0 },
      { "annual",          TYPE_INT,      &m_costs.permits,                true,    0 },
      { "SafeSite",        TYPE_FLOAT,    &m_costs.safeSite,               true,    0 },
      { "RemoveFromHazardZoneRatio",    TYPE_FLOAT, &m_removeFromHazardZoneRatio, true, 0 },
      { "RaiseRelocateSafestSiteRatio", TYPE_FLOAT, &m_raiseRelocateSafestSiteRatio, true, 0 },
      { NULL,              TYPE_NULL,     NULL,                            false,   0 } };

   if (TiXmlGetAttributes(pXmlTWLcosts, costAttrs, filename) == false)
      return false;

   /*
   if (m_runFlags & CH_MODEL_FLOODING)
      {
      // flooded areas
      TiXmlElement* pXmlFloodAreas = pXmlRoot->FirstChildElement("flood_areas");
      if (pXmlFloodAreas != NULL)
         {
         // take care of any constant expressions
         TiXmlElement* pXmlFloodArea = pXmlFloodAreas->FirstChildElement(_T("flood_area"));
         while (pXmlFloodArea != NULL)
            {
            FloodArea* pFloodArea = FloodArea::LoadXml(pXmlFloodArea, filename, m_pMap);

            if (pFloodArea != NULL)
               {
               m_floodAreaArray.Add(pFloodArea);

               CString msg("  FloodArea: Loaded Flood Grid for layer ");
               msg += pFloodArea->m_cellTypeGridName;
               Report::Log(msg);
               }

            pXmlFloodArea = pXmlFloodArea->NextSiblingElement(_T("flood_area"));
            }
         }
      }
      */

   // m_period = PERIOD_ANNUAL;

   /*if (period[0] == 'h')
   m_period = PERIOD_HOURLY;
   else if (period[0] == 'm')
   m_period = PERIOD_MONTHLY;*/

   m_useMaxTWL = 1;

   return true;

   } // end LoadXml


double ChronicHazards::Maximum(double *dat, int length)
   {

   double max = -9999;
   for (int i = 0; i < length; i++)
      {
      if (dat[i] > max) // && dat[i] > 0.0)
         max = dat[i];
      }
   if (max == -9999)
      max = 0;
   return max;
   }

double ChronicHazards::Minimum(double *dat, int length)

   {

   double min = 9999;
   for (int i = 0; i < length; i++)
      {
      if (dat[i] < min) // && dat[i]>0.0)
         min = dat[i];
      }
   if (min == 9999)
      min = 0;
   return min;
   }

void ChronicHazards::Mminvinterp(double *x, double *y, double *&xo, int length)
   {
   // MMINVINTERP 1-D Inverse Interpolation. From the text "Mastering MATLAB 7"
   // [Xo, Yo]=MMINVINTERP(X,Y,Yo) linearly interpolates the vector Y to find
   // the scalar value Yo and returns all corresponding values Xo interpolated
   // from the X vector. Xo is empty if no crossings are found. For
   // convenience, the output Yo is simply the scalar input Yo replicated so
   // that size(Xo)=size(Yo).
   // If Y maps uniquely into X, use INTERP1(Y,X,Yo) instead.
   //
   // See also INTERP1.
   /* float *xo = new float[length];
   memset(xo, 0, length*sizeof(float));*/
   /*float *y = new float[length];
   memset(y, 0, length*sizeof(float));*/


   //for (int ii = 0; ii < length; ii++)
   //{
   //   y[ii] = y1[ii] - y2;
   //}
   float yo = 0;

   for (int ii = 0; ii < length - 1; ii++)
      {

      if (((y[ii] < yo) && (y[ii + 1] > yo)) || ((y[ii] > yo) && (y[ii + 1] < yo)))
         {
         xo[ii] = ((yo - y[ii]) / (y[ii + 1] - y[ii]))* (x[ii + 1] - x[ii]) + x[ii];


         }
      if (y[ii] == 0)
         xo[ii] = x[ii];




      }
   }

// For programming purposes, approximation of kh^2  
// Ref : Hunt, J.N., 1979. Direct solution of wave dispersion equation. J.
// Waterw., Port, CoastalOcean Dlv., Am.Soc.Civ.Eng. 105(4),
// 457 - 459.
double ChronicHazards::CalculateCelerity2(float waterLevel, float wavePeriod, double &kh)
   {
   double celerity;

   double dCoeff[6];

   dCoeff[0] = 0.6;
   dCoeff[1] = 0.35;
   dCoeff[2] = 0.1608465608;
   dCoeff[3] = 0.0632098765;
   dCoeff[4] = 0.0217540484;
   dCoeff[5] = 0.0065407983;

   double sigma = (2 * PI) / wavePeriod;

   double x = (sigma * waterLevel) / sqrt(g * waterLevel);
   double sigmaSum = 0.0;


   for (int i = 0; i < 6; i++)
      {
      sigmaSum += dCoeff[i] * pow(x, 2.0 * (i + 1));
      }

   double khSquared = (pow(x, 4.0) + (pow(x, 2.0) / (1.0 + sigmaSum)));

   kh = sqrt(khSquared);

   double waveNumber = kh / waterLevel;

   celerity = (2.0 * PI) / (wavePeriod * waveNumber);

   return celerity;

   }


float ChronicHazards::CalculateCelerity(float waterLevel, float wavePeriod, float &n)
   {
   // The calculation below solves the dispersion relation for water waves. 
   // The Newton-Rhapson method is used to solve the equation.

   // Radian peak frequency (sig)
   float sig = (2.0f * PI) / wavePeriod;

   // Deep water wave number calculations
   // Determine x
   float sigsq = sig * sig;
   float kdeep = sigsq / g;
   float kh_0 = kdeep * waterLevel;
   float x = kdeep * waterLevel*pow(1.0f / tanh(pow(kdeep*waterLevel, 0.75f)), (2.0f / 3.0f));

   float deltax = 10.0f;
   int iterations = 0;
   while ((abs(deltax / x)) > pow(10.0f, -5.0f)) //
      {
      if (iterations > 100)
         break;

      float Fx = sigsq * waterLevel / g - x * tanh(x);
      float dFdx = x * tanh(x)*tanh(x) - tanh(x) - x;
      deltax = -Fx / dFdx;
      x += deltax;
      iterations++;
      }

   /* if (iterations > maxIterations)
   maxIterations = iterations;*/

   // Compute the wave number
   float k = x / waterLevel;
   n = (1.0f / 2.0f) * (1.0f + 2.0f * k*waterLevel / sinh(2.0f * k*waterLevel));       /* Cg/c */

                                                                                       // Compute the celerity
   float celerity = 1.0f / k * 2.0f * PI / wavePeriod;

   return celerity;

   } // end CalculateCelerity

     // Runs the Bates flooding model returning the number of grid cells flooded and the area flooded
     //////////double ChronicHazards::GenerateBatesFloodMap(int &floodedCount)
     //////////   {
     //////////   m_pFloodedGrid->SetAllData(0, false);
     //////////
     //////////   // Determine the area of the grid that was flooded
     //////////
     //////////   // area = grid cells that are flooded multiplied by the number of grid cells 
     //////////   double floodedArea = 0.0;
     //////////
     //////////   //clock_t start = clock();
     //////////   InitializeWaterElevationGrid();  // sets NewElevationGrid to elev Grid
     //////////                                 // sets WaterHeight grid to elev Grid
     //////////                                 // sets Discharge grid = 0
     //////////   // update timings
     //////////   //clock_t finish = clock();
     //////////   //float iduration = (float)(finish - start) / CLOCKS_PER_SEC;
     //////////
     //////////   //CString msg;
     //////////   //msg.Format("InitializeWaterHeightGrid generated in %i seconds", (int)iduration);
     //////////   //Report::Log(msg);
     //////////
     //////////   // Run Bates flooding model 
     //////////
     //////////   int duration = (int)(m_batesFloodDuration * 1000.0f);      // hrs
     //////////   int timeStep = (int)(m_timeStep * 1000.0f);
     //////////
     //////////
     //////////   // basic ide - iterate though time, then the grid
     //////////   for (int t = timeStep; t <= duration; t += timeStep)
     //////////      {
     //////////#pragma omp parallel for
     //////////      for (int row = 0; row < m_numRows; row++)
     //////////         {
     //////////         for (int col = 0; col < m_numCols; col++)
     //////////            {
     //////////            // get the discharge at this location
     //////////            double discharge = 0.0;
     //////////            m_pDischargeGrid->GetData(row, col, discharge);
     //////////            double boundaryDischarge = discharge;
     //////////
     //////////            // if the discharge value is not NoData, calulate discharges in/out of each side of cell
     //////////            if (boundaryDischarge != m_pDischargeGrid->GetNoDataValue())
     //////////               {
     //////////               discharge = 0.0f;
     //////////               CalculateFourQs(row, col, discharge);
     //////////               //    ASSERT(discharge >= 0.0f);          
     //////////               m_pDischargeGrid->SetData(row, col, (float)discharge);
     //////////               }
     //////////            }
     //////////         }
     //////////
     //////////#pragma omp parallel for
     //////////      for (int row = 0; row < m_numRows; row++)
     //////////         {
     //////////         for (int col = 0; col < m_numCols; col++)
     //////////            {
     //////////            float waterElevation = 0.0f;
     //////////            //   float elevation = 0.0f;
     //////////            m_pWaterElevationGrid->GetData(row, col, waterElevation);
     //////////
     //////////            //   m_pElevationGrid->GetData(row, col, elevation);
     //////////            if (waterElevation >= 0.0f)
     //////////               {
     //////////               float discharge = 0.0f;
     //////////               m_pDischargeGrid->GetData(row, col, discharge);
     //////////               if (discharge != m_pDischargeGrid->GetNoDataValue())
     //////////                  {
     //////////                  m_pWaterElevationGrid->SetData(row, col, waterElevation + (discharge * m_timeStep));
     //////////                  }
     //////////               }
     //////////            }
     //////////         } // end setting new water heights in the grid
     //////////
     //////////      } // end time duration 
     //////////
     //////////      //  // update timings
     //////////      //clock_t finish = clock();
     //////////      //float iduration = (float)(finish - start) / CLOCKS_PER_SEC;
     //////////
     //////////      //CString msg;
     //////////      //msg.Format("Calc4Q generated in %i seconds", (int)iduration);
     //////////      //Report::Log(msg);
     //////////
     //////////   float totalDepth = 0.0f;
     //////////
     //////////   // set flooding grid with integer 1/0 values
     //////////   for (int row = 0; row < m_numRows; row++)
     //////////      {
     //////////      for (int col = 0; col < m_numCols; col++)
     //////////         {
     //////////         float bedElevation = 0.0f;
     //////////         m_pNewElevationGrid->GetData(row, col, bedElevation);
     //////////
     //////////         if (bedElevation >= 0.0f)
     //////////            {
     //////////            float waterElevation = 0.0f;
     //////////            m_pWaterElevationGrid->GetData(row, col, waterElevation);
     //////////
     //////////            if (waterElevation >= 0.0f)
     //////////               {
     //////////               if ((waterElevation - bedElevation) > 0.0f)
     //////////                  {
     //////////                  float depth = waterElevation - bedElevation;
     //////////                  m_pFloodedGrid->SetData(row, col, depth);
     //////////                  
     //////////                  float cumDepth = 0;
     //////////                  m_pCumFloodedGrid->GetData(row, col, cumDepth);
     //////////                  if ( depth > cumDepth )
     //////////                     m_pCumFloodedGrid->SetData(row, col, depth);
     //////////
     //////////                  floodedCount++;
     //////////                  totalDepth += depth;
     //////////                  }
     //////////               }
     //////////            }
     //////////         }
     //////////      } // end setting flooding grid
     //////////     //
     //////////
     //////////
     //////////      /* m_pWaterElevationGrid->ResetBins();
     //////////      m_pWaterElevationGrid->AddBin(0, RGB(255, 255, 255), "Not Boundary", TYPE_FLOAT, -0.1f, 0.1f);
     //////////      m_pWaterElevationGrid->AddBin(0, RGB(0, 0, 0), "Boundary", TYPE_FLOAT, 0.2f, 10.0f);
     //////////      m_pWaterElevationGrid->ClassifyData();
     //////////      m_pWaterElevationGrid->Show();
     //////////      */
     //////////
     //////////      // m_pDischargeGrid->ResetBins();
     //////////      ///* m_pDischargeGrid->AddBin(0, RGB(255, 255, 255), "Discharge", TYPE_FLOAT, -0.1f, 0.1f);
     //////////      // m_pDischargeGrid->AddBin(0, RGB(0, 0, 0), "Discharge", TYPE_FLOAT, 0.2f, 10.0f);*/
     //////////      // m_pDischargeGrid->ClassifyData();
     //////////      // m_pDischargeGrid->Show();
     //////////
     //////////
     //////////   m_pFloodedGrid->ResetBins();
     //////////   m_pFloodedGrid->AddBin(0, RGB(255, 255, 255), "Not Flooded", TYPE_FLOAT, -0.1f, 0.09f);
     //////////   m_pFloodedGrid->AddBin(0, RGB(0, 0, 255), "Flooded", TYPE_FLOAT, 0.1f, 100.0f);
     //////////   m_pFloodedGrid->ClassifyData();
     //////////   m_pFloodedGrid->Hide();
     //////////
     //////////
     //////////   /*int polylineCount = m_pRoadLayer->GetPolygonCount();
     //////////
     //////////   clock_t start = clock();
     //////////
     //////////   m_pIDULayer->m_readOnly = false;*/
     //////////
     //////////   floodedArea = floodedCount * (m_cellWidth * m_cellHeight);  // in sq meters
     //////////
     //////////   return floodedArea;
     //////////
     //////////   } // end GenerateBatesFloodMap
     //////////
     //////////bool ChronicHazards::CalculateFourQs(int row, int col, double &flux)
     //////////   {
     //////////   //   CString msg;
     //////////
     //////////   float thisElevation = 0.0f;
     //////////   float thisWaterElevation = 0.0f;
     //////////   float manningCoeff = 0.0f;
     //////////
     //////////   float neighborElevation = 0.0f;
     //////////   float neighborWaterElevation = 0.0f;
     //////////
     //////////   flux = 0;
     //////////
     //////////   m_pManningMaskGrid->GetData(row, col, manningCoeff);  // ignore masked areas
     //////////   if (manningCoeff > 0.0f)
     //////////      {
     //////////      m_pNewElevationGrid->GetData(row, col, thisElevation);  // ignore areas with negative elevation
     //////////      if (thisElevation >= 0.0f)
     //////////         {
     //////////         m_pWaterElevationGrid->GetData(row, col, thisWaterElevation);   // ignore areas with no water (WaterElevation = 0)
     //////////         if (thisWaterElevation >= 0.0f)                             
     //////////            {
     //////////            for (int direction = 0; direction < 4; direction++)
     //////////               {
     //////////               int thisRow = row;
     //////////               int thisCol = col;
     //////////
     //////////               switch (direction)
     //////////                  {
     //////////                     case 0:  //NORTH
     //////////                        thisRow--;      //Step to the north one grid cell 
     //////////                        break;
     //////////
     //////////                     case 1:  //EAST
     //////////                        thisCol++;      //Step to the east one grid cell
     //////////                        break;
     //////////
     //////////                     case 2:  //SOUTH
     //////////                        thisRow++;      //Step to the south one grid cell
     //////////                        break;
     //////////
     //////////                     case 3:  //WEST
     //////////                        thisCol--;      //Step to the west one grid cell
     //////////                        break;
     //////////
     //////////                     default:
     //////////                        break;
     //////////
     //////////                  } // end switch
     //////////
     //////////               // get elevation, if value is OK and within grid, determine flow discharge
     //////////               if ((thisRow >= 0 && thisRow < m_numRows) && (thisCol >= 0 && thisCol < m_numCols))
     //////////                  {
     //////////                  m_pNewElevationGrid->GetData(thisRow, thisCol, neighborElevation);
     //////////                  // if the neighbor is valid, calculate flow between this cell and the neighbor
     //////////                  if (neighborElevation >= 0.0f)
     //////////                     {
     //////////                     m_pWaterElevationGrid->GetData(thisRow, thisCol, neighborWaterElevation);
     //////////                     if (neighborWaterElevation >= 0.0f)
     //////////                        {
     //////////                        // form 1: from Bates 1996
     //////////                        // determine flow depth 
     //////////                        /////double flowDepth = fabs(thisWaterElevation - thisElevation) - (neighborWaterElevation - neighborElevation);
     //////////                        /////// cross-sectional flow area
     //////////                        /////float csa = flowDepth * m_cellWidth;
     //////////                        /////
     //////////                        /////// hydraulic radius (flow area/wetted perimeter)
     //////////                        /////float r = csa / (m_cellWidth + (2 * flowDepth));
     //////////                        /////
     //////////                        /////// water surface slope (positive if slope toward this, negative if sloping away from this)
     //////////                        /////float slope = (neighborWaterElevation - thisWaterElevation ) / m_cellWidth;
     //////////                        /////
     //////////                        /////// solve mannings equation
     //////////                        /////float Q = (csa * pow(r, (2.0 / 3.0))*fabs(slope)) / manningCoeff;
     //////////                        /////if (slope < 0)
     //////////                        /////   Q = -Q;
     //////////
     //////////                        // form 2: from Bates 2005
     //////////                        double flowDepth = fmax(thisWaterElevation, neighborWaterElevation) - fmax(thisElevation, neighborElevation);
     //////////                        
     //////////                        double deltaH = neighborWaterElevation - thisWaterElevation;
     //////////                        
     //////////                        double Q = (pow(flowDepth, (5.0 / 3.0)) / manningCoeff)*sqrt(fabs(deltaH / m_cellWidth))*m_cellWidth;
     //////////                        
     //////////                        if (deltaH < 0)  // flow leaving this cell?
     //////////                           Q = -Q;
     //////////
     //////////                        flux += Q;
     //////////
     //////////                        // original below
     //////////                        //double flowDepth = fmax(thisWaterElevation, neighborWaterElevation) - fmax(thisElevation, neighborElevation);
     //////////                        //flowDepth = (flowDepth > 0.0) ? flowDepth : 0.0;
     //////////
     //////////                        //double waterElevationDiff = neighborWaterElevation - thisWaterElevation;
     //////////                        //
     //////////                        //double power = 5.0 / 3.0;
     //////////                        //
     //////////                        //// volumetric flow in x direction (East and West)                    
     //////////                        //if (direction % 2 == 1)
     //////////                        //   {
     //////////                        //   // determine if water is moving in or out of the cell by assigning appropriate sign
     //////////                        //   double slopeSign = sqrt(abs(waterElevationDiff) / m_cellWidth) * m_cellHeight;
     //////////                        //   if (waterElevationDiff < 0)
     //////////                        //      slopeSign = -slopeSign;
     //////////                        //   //   flux += (float)((pow(flowDepth, power) / m_ManningCoeff) * slopeSign);
     //////////                        //   flux += (float)((pow(flowDepth, power) / manningCoeff) * slopeSign);
     //////////                        //   }
     //////////                        //// volumetric flow in y direction (North & South)
     //////////                        //if (direction % 2 == 0)
     //////////                        //   {
     //////////                        //   // determine if water is moving in or out of the cell by assigning appropriate sign
     //////////                        //   double slopeSign = sqrt(abs(waterElevationDiff) / m_cellHeight) * m_cellWidth;
     //////////                        //   if (WaterElevationDiff < 0)
     //////////                        //      slopeSign = -slopeSign;
     //////////                        //   //   flux += (float)((pow(flowDepth, power) / m_ManningCoeff) * slopeSign);
     //////////                        //   flux += (float)((pow(flowDepth, power) / manningCoeff) * slopeSign);
     //////////                        //   }
     //////////                        } // valid location with WaterElevation 
     //////////                     } // valid location with elevation in grid
     //////////                  }  // within grid       
     //////////               } // end direction
     //////////               // this is the change in water height in the given cell per given time step
     //////////            flux /= (m_cellWidth * m_cellHeight);
     //////////            } // end valid Manning Coefficient
     //////////         } // end valid cell WaterElevation
     //////////      } // end valid cell Elevation
     //////////
     //////////   return true;
     //////////
     //////////   } // end CalculateFourQs
     //////////
     //////////bool ChronicHazards::InitializeWaterElevationGrid()
     //////////   {
     //////////   /********* BEGIN uncomment for testing/debugging creating a flat elevation surface *********/
     //////////
     //////////
     //////////   /*for (int row = 0; row < m_numRows; row++)
     //////////   {
     //////////      for (int col = 0; col < m_numCols; col++)
     //////////      {
     //////////       float elevation = 0.0f;
     //////////       m_pElevationGrid->GetData(row, col, elevation);
     //////////       if (elevation >= 0.0f)
     //////////         m_pElevationGrid->SetData(row, col, 3.0f);
     //////////      }
     //////////   }*/
     //////////   /********* END uncomment for testing/debugging creating a flat elevation surface *********/
     //////////
     //////////   if (m_pNewElevationGrid == NULL)
     //////////      {
     //////////      m_pNewElevationGrid = m_pMap->CloneLayer(*m_pElevationGrid);
     //////////      m_pNewElevationGrid->m_name = "New Elevation Grid";
     //////////      }
     //////////
     //////////   if (m_pWaterElevationGrid == NULL)
     //////////      {
     //////////      m_pWaterElevationGrid = m_pMap->CloneLayer(*m_pElevationGrid);
     //////////      m_pWaterElevationGrid->m_name = "Water Elevation Grid";
     //////////      }
     //////////
     //////////   float noDataValue = m_pWaterElevationGrid->GetNoDataValue();
     //////////
     //////////   for (int row = 0; row < m_numRows; row++)
     //////////      {
     //////////      for (int col = 0; col < m_numCols; col++)
     //////////         {
     //////////         float elevation = 0.0f;
     //////////         m_pElevationGrid->GetData(row, col, elevation);
     //////////         m_pNewElevationGrid->SetData(row, col, elevation);
     //////////         m_pWaterElevationGrid->SetData(row, col, elevation);
     //////////         }
     //////////      }
     //////////
     //////////   // set all negative DEM values (water) to NoData
     //////////   // Model won't work if flooding water on top of water
     //////////   // also read Mask grid and set where flooding model doesn't run to NoData
     //////////   for (int row = 0; row < m_numRows; row++)
     //////////      {
     //////////      for (int col = 0; col < m_numCols; col++)
     //////////         {
     //////////         float elevation = 0.0f;
     //////////         m_pNewElevationGrid->GetData(row, col, elevation);
     //////////         float manningsCoefficient = 0.0f;
     //////////         m_pManningMaskGrid->GetData(row, col, manningsCoefficient);
     //////////         if (elevation < 0.0f || manningsCoefficient <= 0.0f)
     //////////            m_pWaterElevationGrid->SetData(row, col, -9999.0f);
     //////////         }
     //////////      }
     //////////   /*for (int row = 0; row < m_numRows; row++)
     //////////   {
     //////////      for (int col = 0; col < m_numCols; col++)
     //////////      {
     //////////         float elevation = 0.0f;
     //////////         m_pNewElevationGrid->GetData(row, col, elevation);
     //////////         if (elevation < 0.0f)
     //////////            m_pWaterElevationGrid->SetData(row, col, -9999.0f);
     //////////      }
     //////////   }*/
     //////////
     //////////   if (m_pDischargeGrid == NULL)
     //////////      {
     //////////      m_pDischargeGrid = m_pMap->CloneLayer(*m_pWaterElevationGrid);
     //////////      m_pDischargeGrid->m_name = "Discharge Grid";
     //////////      }
     //////////   m_pDischargeGrid->SetAllData(0.0, false);
     //////////
     //////////   // initialize prev dune pt rows and columns 
     //////////   int prevDunePtRow = -1;
     //////////   int prevDunePtCol = -1;
     //////////   int count = 0;
     //////////
     //////////   MapLayer::Iterator endPoint = m_pNewDuneLineLayer->Begin();
     //////////
     //////////   // initialize boundary conditions for water free surface (water height) grid
     //////////   // at time t = 0
     //////////   for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
     //////////      {
     //////////      endPoint = point;
     //////////
     //////////      int beachType = -1;
     //////////      m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
     //////////
     //////////      REAL xDuneCrest = 0.0;
     //////////      REAL xDuneToe = 0.0;
     //////////      REAL xCoord = 0.0;
     //////////      REAL yCoord = 0.0;
     //////////
     //////////      int duneToeCol = -1;
     //////////
     //////////      int startRow = -1;
     //////////      int startCol = -1;
     //////////      int nextRow = -1;
     //////////
     //////////      float manningsCoefficient = 0.0f;
     //////////
     //////////      if (beachType == BchT_BAY)
     //////////         {
     //////////         manningsCoefficient = 0.0f;
     //////////
     //////////         float waterHeight = -9999.0f;
     //////////         m_pNewDuneLineLayer->GetData(point, m_colYrMaxTWL, waterHeight);
     //////////
     //////////         int colNorthing;
     //////////         int colEasting;
     //////////
     //////////         CheckCol(m_pBayTraceLayer, colNorthing, "NORTHING", TYPE_DOUBLE, CC_MUST_EXIST);
     //////////         CheckCol(m_pBayTraceLayer, colEasting, "EASTING", TYPE_DOUBLE, CC_MUST_EXIST);
     //////////
     //////////         // traversing along bay shoreline, determine is more than the elevation 
     //////////         for (MapLayer::Iterator bayPt = m_pBayTraceLayer->Begin(); bayPt < m_pBayTraceLayer->End(); bayPt++)
     //////////            {
     //////////            m_pBayTraceLayer->GetData(bayPt, colEasting, xCoord);
     //////////            m_pBayTraceLayer->GetData(bayPt, colNorthing, yCoord);
     //////////
     //////////            // get the yearly max TWL for this bay station
     //////////            bool runSpatialBayTWL = (m_runSpatialBayTWL == 1) ? true : false;
     //////////            if (runSpatialBayTWL)
     //////////               {
     //////////               m_pBayTraceLayer->GetData(bayPt, m_colBayYrMaxTWL, waterHeight);
     //////////               }
     //////////
     //////////            m_pNewElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
     //////////            //   
     //////////            //   is the bay trace point within grid being simulated?
     //////////            if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
     //////////               {
     //////////               //waterHeight = 5.0f;
     //////////               // if the mannings coeff is defined, then set the water height grid at the
     //////////               // same location as the bay point to the maxTWL for the bay point
     //////////               m_pManningMaskGrid->GetData(startRow, startCol, manningsCoefficient);
     //////////               if (manningsCoefficient > 0.0)
     //////////                  {
     //////////                  /*float elevation = 0.0f;
     //////////                  m_pNewElevationGrid->GetData(startRow, startCol, elevation);
     //////////                  if (waterHeight > elevation)*/
     //////////                  m_pWaterElevationGrid->SetData(startRow, startCol, waterHeight);
     //////////
     //////////                  m_pDischargeGrid->SetData(startRow, startCol, -9999.0f);   ////????
     //////////                  }
     //////////               } // end inner bay type 
     //////////            } // end bay inlet pt
     //////////         } // end bay inlet flooding 
     //////////      else if (beachType != BchT_UNDEFINED || beachType != BchT_RIVER) // || beachType != BchT_RIPRAP_BACKED)
     //////////         {
     //////////         manningsCoefficient = 0.0f;
     //////////         /* m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, xCoord);
     //////////          m_pNewDuneLineLayer->GetData(point, m_colNorthingCrest, yCoord);*/
     //////////         m_pNewDuneLineLayer->GetPointCoords(point, xDuneToe, yCoord);
     //////////         m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, xDuneCrest);
     //////////
     //////////         m_pNewElevationGrid->GetGridCellFromCoord(xDuneToe, yCoord, startRow, duneToeCol);
     //////////         m_pWaterElevationGrid->GetGridCellFromCoord(xDuneCrest, yCoord, startRow, startCol);
     //////////
     //////////         // within grid ?
     //////////         // write boundary conditions into water height grid cell
     //////////         if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
     //////////            {
     //////////            m_pManningMaskGrid->GetData(startRow, startCol, manningsCoefficient);
     //////////            if (manningsCoefficient > 0.0)
     //////////               {
     //////////               // get the yearly max TWL for this duneline point
     //////////               float waterHeight = -9999.0f;
     //////////               m_pNewDuneLineLayer->GetData(point, m_colYrMaxTWL, waterHeight);
     //////////
     //////////               //waterHeight = 9.0f;
     //////////               // get the duneCrest height for this dunePoint
     //////////               float duneCrest = 0.0f;
     //////////               m_pNewDuneLineLayer->GetData(point, m_colDuneCrest, duneCrest);
     //////////
     //////////               float duneToe = 0.0f;
     //////////               m_pNewDuneLineLayer->GetData(point, m_colDuneToe, duneToe);
     //////////
     //////////               float prevDuneCrest = 0.0f;
     //////////               m_pNewElevationGrid->SetData(startRow, startCol, duneCrest);
     //////////               m_pNewElevationGrid->SetData(startRow, duneToeCol, duneToe);
     //////////               m_pWaterElevationGrid->SetData(startRow, startCol, waterHeight);
     //////////
     //////////               // shift DEM west of DuneCrest
     //////////               /***************/
     //////////               m_pNewDuneLineLayer->GetData(point, m_colPrevCol, prevDunePtCol);
     //////////
     //////////               float elevation = 0.0f;
     //////////               m_pNewElevationGrid->GetData(startRow, startCol + 1, elevation);
     //////////               if (waterHeight > elevation)
     //////////                  count++;
     //////////
     //////////               m_pDischargeGrid->SetData(startRow, startCol, -9999.0f);
     //////////
     //////////               /*float shorefaceSlope = 0.0f;
     //////////               m_pNewDuneLineLayer->GetData(point, m_colShoreface, shorefaceSlope);*/
     //////////
     //////////               endPoint++;
     //////////               if (endPoint < m_pNewDuneLineLayer->End())
     //////////                  {
     //////////                  int nextRow = -1;
     //////////                  int nextCol = -1;
     //////////                  m_pNewDuneLineLayer->GetPointCoords(endPoint, xCoord, yCoord);
     //////////                  m_pWaterElevationGrid->GetGridCellFromCoord(xCoord, yCoord, nextRow, nextCol);
     //////////
     //////////                  if ((ABS(nextRow - startRow)) > 1)
     //////////                     {
     //////////                     for (int row = nextRow; row <= startRow; row++)
     //////////                        {
     //////////                        int col = startCol - 1;
     //////////                        while (col >= 0 && startCol != 0)
     //////////                           {
     //////////                           m_pNewElevationGrid->SetData(row, col, -9999.0f);
     //////////                           col--;
     //////////                           }
     //////////                        }
     //////////                     }
     //////////                  }
     //////////
     //////////               //elevation = duneToe;
     //////////               int col = duneToeCol - 1;
     //////////               //   int col = startCol - 1;
     //////////                  //   int prevCol = prevDunePtCol - 1;
     //////////
     //////////                     // move West
     //////////                  /*   while (col >= prevDunePtCol && duneToeCol != 0)
     //////////                     {
     //////////                        m_pElevationGrid->GetData(startRow, prevCol, elevation);
     //////////                        m_pNewElevationGrid->SetData(startRow, col, elevation);
     //////////                        prevCol--;
     //////////                        col--;
     //////////                     }*/
     //////////
     //////////               while (col >= 0 && duneToeCol != 0)
     //////////                  {
     //////////                  m_pNewElevationGrid->SetData(startRow, col, -9999.0f);
     //////////                  col--;
     //////////                  }
     //////////
     //////////               /* while (col >= 0 && startCol != 0)
     //////////               {
     //////////                  waterHeight = -9999.0f;
     //////////                  m_pWaterElevationGrid->SetData(startRow, col, waterHeight);
     //////////                  m_pDischargeGrid->SetData(startRow, col, waterHeight);
     //////////                  col--;
     //////////               }*/
     //////////               } // within grid
     //////////
     //////////               // is this needed anymore ????
     //////////               // back fill the missing rows to nodata
     //////////               //waterHeight = -9999.0f;
     //////////               //if (prevDunePtRow >= 0 && (startRow - prevDunePtRow) > 1)
     //////////               //{
     //////////               //   // determine col 
     //////////               //   float colStep = (float) ((startCol - prevDunePtCol) / (startRow - prevDunePtRow + 1));
     //////////               //   for (int row = prevDunePtRow +1; row < startRow; row++)
     //////////               //   {
     //////////               //      int thisCol = prevDunePtCol + (int)colStep;
     //////////               //      if ((row >= 0 && row < m_numRows) && (thisCol >= 0 && thisCol < m_numCols))
     //////////               //      {
     //////////               //         m_pWaterElevationGrid->SetData(row, thisCol, waterHeight);
     //////////               //         m_pDischargeGrid->SetData(row, thisCol, waterHeight);
     //////////               //         thisCol += (int)colStep;
     //////////
     //////////               //         int col = thisCol - 1;
     //////////               //         while (col >= 0 && thisCol != 0)
     //////////               //         {
     //////////               //            waterHeight = -9999.0f;
     //////////               //            m_pWaterElevationGrid->SetData(row, col, waterHeight);
     //////////               //            m_pDischargeGrid->SetData(row, col, waterHeight);
     //////////               //            col--;
     //////////               //         }
     //////////               //      }
     //////////               //   }
     //////////               //}
     //////////
     //////////            /*   prevDunePtRow = startRow;
     //////////               prevDunePtCol = startCol;*/
     //////////               //prevRow = startRow;
     //////////            }
     //////////         }
     //////////      } // end duneline pts
     //////////
     //////////   CString msg;
     //////////   msg.Format("The count  that flooded are: %i", count);
     //////////   Report::Log(msg);
     //////////   return true;
     //////////
     //////////
     //////////   } // end InitializeWaterElevationGrid
     //////////


     ///////////////////////////////////////////////////////////////////////////

/*

bool ChronicHazards::RunFloodAreas(EnvContext *pContext)
   {
   for (int i = 0; i < m_floodAreaArray.GetSize(); i++)
      {
      FloodArea *pFloodArea = m_floodAreaArray[i];

      InitializeFloodGrids(pContext, pFloodArea);

      pFloodArea->Run(pContext);
      }

   return true;
   }
 */

/*

bool ChronicHazards::InitializeFloodGrids(EnvContext *pContext, FloodArea *pFloodArea)
   {
   // first, make sure required grids are available
   if (pFloodArea->m_pElevationGrid == NULL)
      return false;

   if (pFloodArea->m_pCellTypeGrid == NULL)
      return false;

   if (pFloodArea->m_pWaterSurfaceElevationGrid == NULL)
      return false;

   if (pFloodArea->m_pDischargeGrid == NULL)
      return false;

   if (pFloodArea->m_pWaterDepthGrid == NULL)
      return false;

   ///// Step 1: set the FloodArea's water elevation grid to zero initially
   pFloodArea->m_pWaterSurfaceElevationGrid->SetAllData(0, false);

   ///// Step 2: Set boundary TWLs if necessary

   // initialize prev dune pt rows and columns 
   int prevDunePtRow = -1;
   int prevDunePtCol = -1;
   int count = 0;
   float elevDelta = 0;
   MapLayer::Iterator endPoint = m_pNewDuneLineLayer->Begin();

   pFloodArea->m_pCellTypeGrid->SetAllData(CT_MODEL, false);

   // iterate through dune points, setting associated TWLs in the FloodArea
   for (MapLayer::Iterator dunePtIndex = m_pNewDuneLineLayer->Begin(); dunePtIndex < m_pNewDuneLineLayer->End(); dunePtIndex++)
      {
      endPoint = dunePtIndex;

      int beachType = -1;
      m_pNewDuneLineLayer->GetData(dunePtIndex, m_colBeachType, beachType);

      REAL xDuneCrest = 0.0;
      REAL xDuneToe = 0.0;
      REAL xCoord = 0.0;
      REAL yCoord = 0.0;

      int duneToeCol = -1;

      int startRow = -1;
      int startCol = -1;
      int nextRow = -1;

      if (beachType == BchT_BAY)
         {
         //manningsCoefficient = 0.0f;
         //float waterHeight = -9999.0f;
         //m_pNewDuneLineLayer->GetData(dunePtIndex, m_colYrMaxTWL, waterHeight);
         //
         //int colNorthing;
         //int colEasting;
         //
         //// traversing along bay shoreline, determine if more than the elevation 
         //for (MapLayer::Iterator bayPt = m_pBayTraceLayer->Begin(); bayPt < m_pBayTraceLayer->End(); bayPt++)
         //   {
         //   m_pBayTraceLayer->GetData(bayPt, colEasting, xCoord);
         //   m_pBayTraceLayer->GetData(bayPt, colNorthing, yCoord);
         //
         //   // get the yearly max TWL for this bay station
         //   bool runSpatialBayTWL = (m_runSpatialBayTWL == 1) ? true : false;
         //   if (runSpatialBayTWL)
         //      {
         //      m_pBayTraceLayer->GetData(bayPt, m_colBayYrMaxTWL, waterHeight);
         //      }
         //
         //   m_pNewElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
         //   //   
         //   //   is the bay trace point within grid being simulated?
         //   if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
         //      {
         //      //waterHeight = 5.0f;
         //      // if the mannings coeff is defined, then set the water height grid at the
         //      // same location as the bay point to the maxTWL for the bay point
         //      m_pManningMaskGrid->GetData(startRow, startCol, manningsCoefficient);
         //      if (manningsCoefficient > 0.0)
         //         {
         //         //float elevation = 0.0f;
         //         //m_pNewElevationGrid->GetData(startRow, startCol, elevation);
         //         //if (waterHeight > elevation)
         //         //m_pWaterElevationGrid->SetData(startRow, startCol, waterHeight);
         //
         //         m_pDischargeGrid->SetData(startRow, startCol, -9999.0f);   ////????
         //         }
         //      } // end inner bay type 
         //   } // end bay inlet pt
         } // end bay inlet flooding 
      else if (beachType != BchT_UNDEFINED && beachType != BchT_RIVER) // || beachType != BchT_RIPRAP_BACKED)
         {
         // get the X/Y location of the dune point point.
         REAL yDune = 0;
         m_pNewDuneLineLayer->GetPointCoords(dunePtIndex, xDuneToe, yDune);

         // get the dune crest X location
         m_pNewDuneLineLayer->GetData(dunePtIndex, m_colEastingCrest, xDuneCrest);

         // is this a location we need to examine?
         int row, col;
         pFloodArea->m_pCellTypeGrid->GetGridCellFromCoord(xDuneCrest, yDune, row, col);

         // is the dune point on the cell grid?
         if (row < 0 || row >= pFloodArea->m_rows
            || col < 0 && col >= pFloodArea->m_cols)
            continue;

         // are we in the model domain?
         if (pFloodArea->GetCellType(row, col) == CT_OFFGRID)
            continue;

         // set the dune crest as fixed height source if TWL exceed dune height
         float waterHeight = -9999.0f;
         m_pNewDuneLineLayer->GetData(dunePtIndex, m_colYrMaxTWL, waterHeight);

         float duneHeight = 0.0f;
         m_pNewDuneLineLayer->GetData(dunePtIndex, m_colDuneCrest, duneHeight);

         if (waterHeight > duneHeight)
            {
            pFloodArea->m_pCellTypeGrid->SetData(row, col, CT_FIXEDHEIGHTSOURCE);
            pFloodArea->m_pWaterSurfaceElevationGrid->SetData(row, col, waterHeight);
            count++;
            elevDelta += (waterHeight - duneHeight);
            }
         }
      } // end duneline pts

   if (count > 0)
      elevDelta /= count;

   CString msg;
   msg.Format("  FloodArea: %i fixed-height sources were identified for %s, with an average delta of %.2fm.", count, (LPCTSTR)pFloodArea->m_siteName, elevDelta);
   Report::Log(msg);

   //pFloodArea->m_pCellTypeGrid->SetAllData(CT_MODEL,false);
   //pFloodArea->m_pCellTypeGrid->SetData(215,160, CT_FIXEDHEIGHTSOURCE);
   //pFloodArea->m_pElevationGrid->SetAllData(1, false);
   //pFloodArea->m_pWaterSurfaceElevationGrid->SetAllData(1, false);
   //pFloodArea->m_pWaterSurfaceElevationGrid->SetData(215, 160, 8.0f);

   return true;
   } // end InitializeWaterElevationGrid



     ///////////////////////////////////////////////////////////////////////////



double ChronicHazards::GenerateEelgrassMap(int &eelgrassCount)
   {
   if (m_pEelgrassGrid == NULL)
      {
      m_pEelgrassGrid = m_pMap->CloneLayer(*m_pBayBathyGrid);
      m_pEelgrassGrid->m_name = "Eelgrass Grid";
      }
   m_pEelgrassGrid->SetAllData(0.0f, false);

   // area = grid cell count multiplied by area of grid cell
   //double eelgrassCount = 0.0;

   float yearlyAvgLowSWL = 0.0f;
   float yearlyAvgSWL = 0.0f;

   MapLayer::Iterator point = m_pNewDuneLineLayer->Begin();

   m_pNewDuneLineLayer->GetData(point, m_colYrAvgSWL, yearlyAvgSWL);
   m_pNewDuneLineLayer->GetData(point, m_colYrAvgLowSWL, yearlyAvgLowSWL);

   //for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
   //{
   //   int beachType = 0;
   //   m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);

   //   int duneIndx = -1;
   //   m_pNewDuneLineLayer->GetData(point, m_colDuneIndx, duneIndx);

   //   //REAL xCoord = 0.0;
   //   //REAL yCoord = 0.0;

   //   //int startRow = -1;
   //   //int startCol = -1;

   float bayElevation = 0.0f;

   /*double duneCrest = 0.0;

   if (beachType == BchT_BAY)
   {
   float noDataValue = m_pBayBathyGrid->GetNoDataValue();

   //// get twl of inlet seed point
   //m_pNewDuneLineLayer->GetData(point, m_colYrAvgLowSWL, yearlyAvgLowSWL);
   //m_pNewDuneLineLayer->GetData(point, m_colYrAvgSWL, yearlyAvgSWL);

   //float avgWaterHeight = (yearlyAvgLowSWL + yearlyAvgSWL) / 2.0f;

   //float delta = avgWaterHeight; //  - m_avgWaterHeightBase;

   for (int row = 0; row < m_numBayRows; row++)
      {
      for (int col = 0; col < m_numBayCols; col++)
         {
         m_pBayBathyGrid->GetData(row, col, bayElevation);
         bayElevation -= m_delta;

         if (bayElevation != noDataValue)
            {
            //if (bayElevation >= 0)
            ////   waterElevation = yearlyMaxSWL - bayElevation;
            //   waterElevation = yearlyAvgSWL - bayElevation;
            //else
            //   //waterElevation = bayElevation + yearlyMaxSWL;
            //   waterElevation = bayElevation + yearlyAvgSWL;
            m_pEelgrassGrid->SetData(row, col, bayElevation);
            if (bayElevation >= -1.56f && bayElevation <= -0.46f)
               eelgrassCount++;
            }
         } // end grid cols
      } // grid rows
        //   } // end bay pt
        //   } // end bay pInlet



   m_pEelgrassGrid->ResetBins();
   m_pEelgrassGrid->AddBin(0, RGB(190, 210, 255), "Below Extreme Low Water", TYPE_FLOAT, -100.0f, -1.56f);
   m_pEelgrassGrid->AddBin(0, RGB(76, 230, 0), "Potential EelGrass Habitat", TYPE_FLOAT, -1.56f, -0.46f);
   m_pEelgrassGrid->AddBin(0, RGB(255, 234, 190), "Tidal Flats", TYPE_FLOAT, -0.46f, 2.1f);
   //   m_pEelgrassGrid->AddFieldInfo("WL", 0, "Below Extreme Low Water", _T(""), _T(""), TYPE_INT, MFIT_CATEGORYBINS, BCF_MIXED, 0, -100.f, -1.56f, true);


   m_pEelgrassGrid->ClassifyData();

   return (eelgrassCount * (m_bayCellHeight * m_bayCellWidth));
   }

   */



// Returns the interpolated value in ycol given an x in xcol
// Also returns the row, as an argument, that is closest to the computed interpolated value 
// If the magnitudes from the interpolated value are equal,
// method returns the first row encountered during the search (the lesser row in the case of the forward search and the greater row in the case of the reverse search)

double ChronicHazards::IGet(DDataObj *table, double x, int xcol, int ycol/*= 1*/, IMETHOD method/*=IM_LINEAR*/, int startRow/*=0*/, int *returnRow/*=NULL*/, bool forwardSearch/*=true*/)
   {
   double x1;

   int row = startRow;

   if (forwardSearch)
      {
      //-- find appropriate row
      for (row = startRow; row < table->GetRowCount(); row++)
         {
         x1 = table->Get(xcol, row);

         //-- have we found or passed the correct row?
         if (x1 >= x)
            break;
         }

      //-- x occured before first row?
      //-- return first element (doesn't wrap)
      if (row == 0)
         {
         *returnRow = 0;
         return table->Get(ycol, 0);
         }

      //-- x occurred after last row (never found row)?
      //-- return last element (doesn't wrap)
      if (row == table->GetRowCount())
         {
         *returnRow = table->GetRowCount() - 1;
         return table->Get(ycol, table->GetRowCount() - 1);
         }

      //-- was the found row an exact match?
      if (x1 == x)
         {
         *returnRow = row;
         return table->Get(ycol, row);
         }
      }
   else // reverseSearch
      {
      //-- find appropriate row
      for (row = startRow; row >= 0; row--)
         {
         x1 = table->Get(xcol, row);

         //-- have we found or passed the correct row?
         if (x1 <= x)
            break;
         }
      //-- x occured before first row?
      //-- return startRow element (doesn't wrap)
      if (row == startRow)
         {
         *returnRow = startRow;
         return table->Get(ycol, startRow);
         }

      //-- x occurred after first row (never found row)?
      //-- return last element (doesn't wrap)
      if (row == 0)
         {
         *returnRow = 0;
         return table->Get(ycol, 0);
         }

      //-- was the found row an exact match?
      if (x1 == x)
         {
         *returnRow = row;
         return table->Get(ycol, row);
         }
      }

   double y;

   //-- not an exact match, interpolate between found and previous rows
   if (forwardSearch)
      {
      double x0 = table->Get(xcol, row - 1);
      double y0 = table->Get(ycol, row - 1);
      double y1 = table->Get(ycol, row);

      switch (method)
         {
         case IM_LINEAR:
         default:
            y = y1 - (x1 - x) * (y1 - y0) / (x1 - x0);
         }

      // find row closest to interpolated value for non exact match
      // if the magnitude from the interpolated value are equal,
      // return the first row encountered during the search
      // the lesser row index in the case of the forward search
      double val1 = abs(x0 - x);
      double val2 = abs(x - x1);
      if (val1 < val2)
         *returnRow = row - 1;
      else
         *returnRow = row;
      }
   else // reverse search
      {
      double x0 = table->Get(xcol, row + 1);
      double y0 = table->Get(ycol, row + 1);
      double y1 = table->Get(ycol, row);

      switch (method)
         {
         case IM_LINEAR:
         default:
            y = y0 - (x - x1) * (y0 - y1) / (x0 - x1);
         }

      // find row closest to interpolated value for non exact match
      // if the magnitude from the interpolated value are equal,
      // return the first row encountered during the search
      // the grreater row index in the case of the reverse search
      double val1 = abs(x1 - x);
      double val2 = abs(x - x0);

      if (val1 < val2)
         *returnRow = row;
      else
         *returnRow = row + 1;
      }

   return y;

   } // end IGet

void ChronicHazards::CalculateYrMaxTWL(EnvContext *pEnvContext)
   {
   m_pNewDuneLineLayer->m_readOnly = false;
   // Determine Yearly Maximum TWL based upon daily TWL calculations at each shoreline location

   // this is looping through each day in the year

   //m_maxTWLArray.SetSize(m_maxDuneIndx+1, 365);

   // iterate through each day of the year
   for (int doy = 0; doy < 365; doy++)
      {
      int row = (pEnvContext->yearOfRun * 365) + doy;

      // Get the Still Water Level for that day ( in NVAVD88 vertical datum ) from the deep water time series
      float swl = 0.0f;
      m_buoyObsData.Get(3, row, swl);

      // Get the Still Water Level for that day ( in NVAVD88 vertical datum ) from the deep water time series
      float lowSWL = 0.0f;
      m_buoyObsData.Get(4, row, lowSWL);

      // tide range for that day based upon still water level
      float wlratio = (swl - (-1.5f)) / (4.5f + 1.5f);

      int   idusProcessed = 0;
      float maxTWLforPeriod = 0;
      float maxErosionforPeriod = 0;

      float meanPeakHeight = 0;
      float meanPeakPeriod = 0;
      float meanPeakDirection = 0;
      float meanPeakWaterLevel = 0;

      // for this day, iterate though Dune toe points, 
      //   calculating TWL and associated variables as we go.
      //#pragma omp parallel for
      for (MapLayer::Iterator point = m_pNewDuneLineLayer->Begin(); point < m_pNewDuneLineLayer->End(); point++)
         {
         bool isOvertopped = false;

         float S_comp = 0.0f;

         // Counters for Impact days (per Year) on dune toe 
         int IDDToeCount = 0;
         int IDDToeCountWinter = 0;
         int IDDToeCountSpring = 0;
         int IDDToeCountSummer = 0;
         int IDDToeCountFall = 0;

         // Counters for Impact days (per Year) on dune crest
         int IDDCrestCount = 0;
         int IDDCrestCountWinter = 0;
         int IDDCrestCountSpring = 0;
         int IDDCrestCountSummer = 0;
         int IDDCrestCountFall = 0;

         // dune line characteristics
         int duneIndx = -1;
         int profileIndx = -1;
         int tranIndx = -1;

         int beachType = 0;
         int origBeachType = 0;
         float tanb1 = 0.0f;
         //  float prevTanb1 = 0.0f;
         float duneCrest = 0.0f;
         float prevDuneCrest = 0.0f;
         float duneToe = 0.0f;
         float beachwidth = 0.0f;

         // influence factor for roughness element of slope
         float gammaRough = 0.0f;

         // influence factor for a berm (if persent)
         int gammaBerm = 0;

         float EPR = 0.0f;

         float dailyTWL = 0.0f;
         float dailyTWL_2 = 0.0f;
         float L = 0.0f;
         float shorelineAngle = 0.0f;

         double eastingCrest = 0.0;
         double eastingToe = 0.0;

         m_pNewDuneLineLayer->GetData(point, m_colDuneIndx, duneIndx);
         m_pNewDuneLineLayer->GetData(point, m_colTranIndx, tranIndx);
         m_pNewDuneLineLayer->GetData(point, m_colProfileIndx, profileIndx);

         m_pNewDuneLineLayer->GetData(point, m_colDuneToe, duneToe);
         m_pNewDuneLineLayer->GetData(point, m_colGammaBerm, gammaBerm);
         m_pNewDuneLineLayer->GetData(point, m_colScomp, S_comp);
         m_pNewDuneLineLayer->GetData(point, m_colGammaRough, gammaRough);
         m_pNewDuneLineLayer->GetData(point, m_colTranSlope, tanb1);
         //    m_pNewDuneLineLayer->GetData(point, m_colPrevSlope, prevTanb1);
         m_pNewDuneLineLayer->GetData(point, m_colBeachType, beachType);
         m_pNewDuneLineLayer->GetData(point, m_colOrigBeachType, origBeachType);
         m_pNewDuneLineLayer->GetData(point, m_colDuneCrest, duneCrest);
         m_pNewDuneLineLayer->GetData(point, m_colEastingCrestProfile, eastingCrest);
         m_pNewDuneLineLayer->GetData(point, m_colEastingToe, eastingToe);
         m_pNewDuneLineLayer->GetData(point, m_colPrevDuneCr, prevDuneCrest);
         //     m_pNewDuneLineLayer->GetData(point, m_colEPR, EPR);
         m_pNewDuneLineLayer->GetData(point, m_colShorelineAngle, shorelineAngle);
         m_pNewDuneLineLayer->GetData(point, m_colBeachWidth, beachwidth);

         float radPeakHeightL = 0.0f;
         float radPeakHeightH = 0.0f;
         float radPeakPeriodL = 0.0f;
         float radPeakPeriodH = 0.0f;
         float radPeakDirectionL = 0.0f;
         float radPeakDirectionH = 0.0f;
         float radPeakWaterLevelL = 0.0f;
         float radPeakWaterLevelH = 0.0f;

         FDataObj *pRBF = m_RBFDailyDataArray.GetAt(tranIndx - m_minTransect);   // zero-based
         pRBF->Get(0, row, radPeakHeightL);
         pRBF->Get(1, row, radPeakHeightH);
         pRBF->Get(2, row, radPeakPeriodL);
         pRBF->Get(3, row, radPeakPeriodH);
         pRBF->Get(4, row, radPeakDirectionL);
         pRBF->Get(5, row, radPeakDirectionH);
         pRBF->Get(6, row, radPeakWaterLevelL);
         pRBF->Get(7, row, radPeakWaterLevelH);

         // unused
         meanPeakHeight += (radPeakHeightH + radPeakHeightL) / 2.0f;
         meanPeakPeriod += (radPeakPeriodH + radPeakPeriodL) / 2.0f;
         meanPeakDirection += (radPeakDirectionH + radPeakDirectionL) / 2.0f;
         meanPeakWaterLevel += (radPeakWaterLevelH + radPeakWaterLevelL) / 2.0f;

         // calculated water level ratio

         // linearly interpolate four parameters on the tide level, these are all arrays that have been interpolated using the function above
         /*   float heightL = min(radPeakHeightL, radPeakHeightH);
         float heightH = max(radPeakHeightL, radPeakHeightH);

         float periodL = min(radPeakPeriodL, radPeakPeriodH);
         float periodH = max(radPeakPeriodL, radPeakPeriodH);

         float directionL = min(radPeakDirectionL, radPeakDirectionH);
         float directionH = max(radPeakDirectionL, radPeakDirectionH);

         float waterLevelL = min(radPeakWaterLevelL, radPeakWaterLevelH);
         float waterLevelH = max(radPeakWaterLevelL, radPeakWaterLevelH);*/

         // ???wave conditions at intermediate depth, 20m contour
         float waveHeight = radPeakHeightL + wlratio * (radPeakHeightH - radPeakHeightL);
         float wavePeriod = radPeakPeriodL + wlratio * (radPeakPeriodH - radPeakPeriodL);
         float waveDirection = radPeakDirectionL + wlratio * (radPeakDirectionH - radPeakDirectionL);
         float waterLevel = radPeakWaterLevelL + wlratio * (radPeakWaterLevelH - radPeakWaterLevelL);

         /*   float waveHeight = heightL + wlratio * (heightH - heightL);
         float wavePeriod = periodL + wlratio * (periodH - periodL);
         float waveDirection = directionL + wlratio * (directionH - directionL);
         float waterLevel = waterLevelL + wlratio * (waterLevelH - waterLevelL);*/

         // Linearly back shoal transformed waves from 20m/30m contour to deep water

         // Deepwater wave length (m)
         float L0 = (g * wavePeriod * wavePeriod) / (2.0f * PI);

         // Deepwater celerity 
         float Co = L0 / wavePeriod;

         //float kh = 0.0f;
         //float C2 = CalculateCelerity2(waterLevel, wavePeriod, kh);

         float n = 0.0f;
         float C = CalculateCelerity(waterLevel, wavePeriod, n);

         //Linear shoaling coefficient
         //double n2 = 0.5 * (1 + (2.0 *kh) / sinh(2.0 * kh));

         //double ks2 = sqrt(0.5 * 1.0 / (n2 * tanh(kh)));

         double ks = (sqrt(1.0f / (2.0f*n) * Co / C));

         float Hdeep = waveHeight / (sqrt(1.0f / (2.0f*n) * Co / C));

         // Breaking height  = 0.78 * breaking depth
         //double Hb = (0.563 * Hdeep) / pow((Hdeep / L), (1.0 / 5.0));

         // Breaking wave water depth relative to MHW ??
         // hb = Hb / gamma where gamma is a breaker index of 0.78  Kriebel and Dean
         //double hbl = Hb / 0.78;

         // Total Water Levels are comprised of a still water level measured at tide gauges and the wave runup
         // TWL = swl + Wave runup

         // Wave runup is a combination of the maximum setup at the shoreline and swash, the time-varying oscillations about the setup
         // Swash includes both incident and infragravity motions

         //***** Calculate the individual contributions of Wave Runup  *****//

         //Static Setup Stockdon et al (2006) formulation
         float eta = (0.35f * tanb1 * sqrt(Hdeep*L0));

         // Infragravity Swash, Stockdon (2006) Eqn: 12
         float STK_IG = 0.06f * sqrt(Hdeep*L0);

         // Incident Swash, Stockdon (2006) Eqn: 11
         float STK_INC = 0.75f*tanb1*sqrt(Hdeep*L0);

         // Stockdon Swash total, Stockdon (2006) Eqn: 7, 11, and 12
         float STK_Swash = sqrt(STK_IG*STK_IG + STK_INC * STK_INC) / 2.0f;

         // Calculate R2% using Stockdon et al (2006) formulation        
         float StockdonR2 = 1.1f*(eta + (sqrt((Hdeep*L0)*(0.563f*tanb1*tanb1 + 0.004f))) / 2.0f);
         float STK_Runup = 1.1f * (eta + STK_Swash);

         /// For very dissipative conditions (low-angle beaches) Stockdon et al., 2006, eqn 18
         /*double Iribarren = tanb1 * sqrt(L0 / Hdeep);

         if (Iribarren < 0.3)
         STK_Runup = 0.043 * sqrt(L0 * Hdeep);
         else if (Iribarren > 1.25)
         STK_Runup = 0.73 * sqrt(L0 );*/

         double structureR2 = 0.0;

         // Calculate the Stockdon TWL
         float STK_TWL = swl + STK_Runup;

         // Determine the Dynamic Water Level 2%
         // note : dailyTWL - dunetoe1
         float DWL2 = swl + 1.1f*(eta + STK_IG / 2.0f) - duneToe;

         // significant wave height at structure toe calculated using a breaker index of 0.78
         float Hmo = DWL2 * 0.78f;

         // If the depth limited breaker height is larger than the offshore conditions, 
         // then the latter will be used.
         if (Hmo > Hdeep)
            Hmo = Hdeep;

         // Irribarren number
         /*   float IbarrenNumber = localBarrierSlope / (sqrt(Hmo / L));

         double R2 = StockdonR2;

         if (IbarrenNumber < 0.3)
         R2 = 0.043 * sqrt(Hdeep*L0);
         else if (IbarrenNumber > 1.25)
         R2 = 0.73 * sqrt(Hdeep*L0);*/

         // Check for overtopping/flooding  at physical dunecrest location
         //  if ((1.1f*eta + swl) >= duneCrest)
         /*  double eastingDC = 0.0f;
         double northingDC = 0.0f;
         int rowDC = -1;
         int colDC = -1;
         m_pNewDuneLineLayer->GetData(point, m_colEastingCrest, eastingDC);
         m_pNewDuneLineLayer->GetData(point, m_colNorthingCrest, northingDC);
         m_pElevationGrid->GetGridCellFromCoord(eastingDC, northingDC, rowDC, colDC);
         m_pElevationGrid->GetData(rowDC, colDC, duneCrest);*/

         if ((swl + 1.1f*eta) >= duneCrest)
            isOvertopped = true;
         else
            isOvertopped = false;

         // if beach type is inlet,  twl = still water level + proportion of the deep water wave height (SWH)

         if (beachType == BchT_BAY)
            {
            dailyTWL = swl + (m_inletFactor * Hdeep);
            //dailyTWL = swl + m_inletFactor;
            duneCrest = (float)MHW;
            int dummybreak = 10;
            }
         else if (beachType == BchT_SANDY_DUNE_BACKED || beachType == BchT_SANDY_BLUFF_BACKED) // || Hmo < 0)
            {
            /*   if (Hmo < 0)
            dailyTWL = STK_TWL;
            else*/
            // Limit flooding to swl + setup for dune backed beaches (no swash)
            dailyTWL_2 = swl + 1.1f*(eta + STK_IG / 2.0f);

            dailyTWL = swl + STK_Runup;
            //   dailyTWL_2 = dailyTWL;
            }

         else if (beachType == BchT_RIPRAP_BACKED ||
            beachType == BchT_DUNE_BLUFF_BACKED ||
            beachType == BchT_CLIFF_BACKED ||
            beachType == BchT_WOODY_DEBRIS_BACKED ||
            beachType == BchT_WOODY_BLUFF_BACKED ||
            beachType == BchT_ROCKY_HEADLAND)
            {
            // Use Local/TAW Combination to calculate TWL for all other defined beachtypes 
            //    else if (beachType != BchT_UNDEFINED || beachType != BchT_RIVER )
            double xTop = 0.0;
            double xBottom = 0.0;

            // local barrier slope / structure slope
            double slope_TAW_local = 0.0;

            double slope_TAW_local_2 = 0.0;


            // local runup on barriers
            double runup_TAW_local = 0.0;

            //breaker parameter
            //float Ibarren_local = 0.0f;

            // structure height
            float height = duneCrest - duneToe;

            // Wave direction(Eqn D.4.5-38)
            // determine influence factor for oblique wave attack
            float beta = abs(waveDirection - shorelineAngle);
            float gammaBeta = 1.0f - 0.0033f * beta;
            if (beta >= 80.0f)
               gammaBeta = 1.0f - 0.0033f * 80.0f;

            /* if (yBerm < 0.6f)
            yBerm = 0.6f;
            if (yBerm > 1.0f)
            yBerm = 1.0f;*/

            //***** TAW Approach With Local Structure Slope (Swash slope) *****//

            //***** Compute the slope of where the swash acts *****//

            // The slope will be computed between where the twl level is computed using the Stockdon Runup (twl = swl + R2%) and the
            // level of the where twl = static setup + swl.

            // If we have raised the height of the BPS, we can no longer use the cross-shore profiles. Check if we have maintained
            int firstMaintYear = 0;
            //    m_pNewDuneLineLayer->GetData(point, m_colBPSMaintYear, maintYear);
            //    int maintFreqInterval = pEnvContext->currentYear - maintYear;
            firstMaintYear = pEnvContext->startYear + m_maintFreqBPS;

            // structure (barrier/bluff) is not flooded
            if (!isOvertopped)
               {
               //    float y1temp = 0.0f;

               // STK_TWL = swl + StockdonR2
               float y1temp = 0.0f;
               double yTop = STK_TWL;

               // If STK_TWL overtops the structue then use the top of the structure 
               // (minus 1cm for interpolation purposes, see curveintersect)
               if (yTop > duneCrest)
                  {
                  yTop = duneCrest - 0.01;
                  }

               // The bottom part of the slope is given by Static Setup (STK) + swl
               double yBottom = 1.1f*eta + swl;

               // Also need new slope when dune erodes and backshore(foredusne) beach slope changes
               // Slope not changing for bluff backed beaches
               //         if ( (profileIndx != prevProfileIndx || duneCrest != prevDuneCrest || tanb1 != prevTanb1 ) && (beachType != BT_RIPRAP_BACKED || maintYear < 2011))// Slope not changing for bluff backed
               //        if ( ( tanb1 != prevTanb1 ) && (beachType != BchT_RIPRAP_BACKED )) // || maintYear < 2011))// Slope not changing for bluff backed
               //    {

               // calculate slope of swash where bluff or barrier exists
               // this is needed to determine the wave runup on a barrier
               // do not use cross shore profile when the dune is dynamically changing (e.g. RIP RAP built or maintained)

               // profileIndex != prevProfileIndex eq to statement TnumRef != prevTNummRef

               if (beachType != BchT_RIPRAP_BACKED || pEnvContext->currentYear < firstMaintYear) //(maintFreq >= m_maintFreqBPS))
                  {
                  DDataObj *pBATH = m_BathyDataArray.GetAt(profileIndx);   // zero-based

                  int length = (int)pBATH->GetRowCount();

                  double *yt;
                  double *yb;
                  double *xtemp;
                  double *xotemp;

                  yt = new double[length];
                  memset(yt, 0, length * sizeof(double));
                  yb = new double[length];
                  memset(yb, 0, length * sizeof(double));
                  xtemp = new double[length];
                  memset(xtemp, 0, length * sizeof(double));
                  xotemp = new double[length];
                  memset(xotemp, 0, length * sizeof(double));

                  for (int ii = 0; ii < length; ii++)
                     {
                     pBATH->Get(0, ii, y1temp);
                     pBATH->Get(1, ii, xtemp[ii]);
                     yt[ii] = y1temp - yTop;
                     yb[ii] = y1temp - yBottom;
                     }

                  // interpolate results
                  Mminvinterp(xtemp, yt, xotemp, length);

                  // xTop (Location of STK_TWL)
                  xTop = Maximum(xotemp, length);

                  Mminvinterp(xtemp, yb, xotemp, length);

                  xBottom = Maximum(xotemp, length);
                  delete[] xotemp;
                  delete[]yb;
                  delete[] yt;
                  delete[] xtemp;

                  // Compute slope where the swash acts
                  slope_TAW_local = (yTop - yBottom) / abs(xTop - xBottom);
                  int tempbreak = 10;
                  if (m_debugOn)
                     {
                     m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 0);
                     m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);
                     m_pNewDuneLineLayer->SetData(point, m_colSTKTWL, STK_TWL);
                     }
                  }
               // RIP RAP has been constructed with a standard 2:1 slope
               // cannot use cross shore profile for site
               else
                  {
                  // the water is swashing on the structure
                  // assume TAW slope equal to BPS slope
                  if (yBottom > duneToe)
                     {
                     slope_TAW_local = m_slopeBPS;
                     if (m_debugOn)
                        {
                        m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 1);
                        m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);
                        }
                     }

                  // the water is swashing on the beach
                  // assume TAW slope equal to beach slope
                  if (yTop < duneToe)
                     {
                     slope_TAW_local = tanb1;
                     if (m_debugOn)
                        {
                        m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 2);
                        m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);
                        }
                     }

                  // the water is swashing on both the structure and the beach
                  // compute the composite slope using the known width of where the swash acts and the slopes 

                  // width weighted average of beach and structure slopes where swash acts
                  //////  // (beachSlope * beachWidth + structureSlope * structureWidth ) / (beachWidth + structureWidth)
                  //////  //  where structureWidth = height / structureSlope 
                  else
                     {
                     //// Compute slope where swash acts
                     double xTopWidth = yTop / m_slopeBPS;
                     double xBtmWidth = yBottom / tanb1;
                     //xTop = eastingToe + yTop / BPSslope;
                     //xBottom = eastingToe - yBottom / tanb1;
                     //  double slope_TAW_local_new = (tanb1 * xBtmWidth + BPSslope * xTopWidth) / (xTop - xBottom);
                     slope_TAW_local = (tanb1 * xBtmWidth + m_slopeBPS * xTopWidth) / (xTopWidth + xBtmWidth);
                     if (m_debugOn)
                        {
                        m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 3);
                        m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);
                        }

                     /*xBottom = yBottom / tanb1;
                     xTop = yTop / m_slopeBPS;
                     slope_TAW_local_2 = ((beachwidth - xBottom)*tanb1 + xTop*m_slopeBPS) / (beachwidth + (height / m_slopeBPS));*/
                     int tempbreak = 10;
                     }
                  }
               }
            // If setup + swl is greater than the structure crest then the
            // composite slope will be used. This corresponds to an overtopped case.

            // BPS structure is flooded not maintained 
            else if (isOvertopped && (beachType != BchT_RIPRAP_BACKED || pEnvContext->currentYear < firstMaintYear))
               //       else if (isOvertopped && (beachType == BchT_RIPRAP_BACKED || (maintFreq <= m_maintFreqBPS ) ) )
               {
               if (S_comp > 0)
                  {
                  slope_TAW_local = S_comp;
                  if (m_debugOn)
                     {
                     m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 4);
                     m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);
                     }
                  }
               else
                  {
                  slope_TAW_local = tanb1;
                  if (m_debugOn)
                     {
                     m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 5);
                     m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);
                     }
                  }
               }
            // compute the composite slope using the known width of the structure+beach and the slopes 
            //    else if (isOvertopped && (beachType == BchT_RIPRAP_BACKED || (maintFreq >= m_maintFreqBPS) ) )
            else if (isOvertopped && (beachType == BchT_RIPRAP_BACKED || pEnvContext->currentYear > firstMaintYear))
               {
               // width weighted average of beach and structure slopes
               // (beachSlope * beachWidth + structureSlope * structureWidth ) / (beachWidth + structureWidth)
               //  where structureWidth = height / structureSlope 
               slope_TAW_local = (tanb1*beachwidth + height) / ((height / m_slopeBPS) + beachwidth);
               if (m_debugOn)
                  {
                  m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 6);
                  m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);
                  }
               }

            //check to make sure the slope does not exceed the structure slope
            if (isNan<double>(slope_TAW_local))
               {
               slope_TAW_local = 0.3;
               if (m_debugOn)
                  {
                  /*   m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 7);
                  m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);*/
                  }
               }

            if (slope_TAW_local > 0.5)
               {
               slope_TAW_local = 0.5;
               if (m_debugOn)
                  {
                  /*   m_pNewDuneLineLayer->SetData(point, m_colRunupFlag, 7);
                  m_pNewDuneLineLayer->SetData(point, m_colTAWSlope, slope_TAW_local);*/
                  }
               }

            //////////// RUNUP EQUATION - DETERMINISTIC APPROACH (includes +1STDEV factor of safety) ////////////
            // The following are calculations for dynamic coastline

            // Mean Wave Period 
            double Tm = wavePeriod / 1.1;

            // Deep water wave length for the DIM method to be applied in the presence of structures/barriers.
            double L = (g * (Tm*Tm)) / (2.0 * PI);

            //breaker parameter
            // Irribarren number based on 2ND slope estimate - Eqn D.4.5-8
            double Ibarren_local = slope_TAW_local / (sqrt(Hmo / L));

            ////Berm factor always equal to 1
            //float gammaBerm = 1.0f;

            double limit = gammaBerm * Ibarren_local;

            // if the roughness reduction factor is unknown,
            // use an the average roughness reduction factor for shoreline backed by rip-rap 
            if (gammaRough < 0.1)
               gammaRough = m_minBPSRoughness;

            if (limit > 0.0f && limit < 1.8f)
               runup_TAW_local = Hmo * 1.75f * gammaRough * gammaBeta * gammaBerm * Ibarren_local;
            else if (limit >= 1.8f)
               runup_TAW_local = Hmo * gammaRough * gammaBeta * (4.3f - (1.6f / sqrt(Ibarren_local)));

            // COMBINE RUNUP VALUES TO CALCULATE COMBINED TWL //

            // Wave Runup consists of setup and swash (barrier runup)
            structureR2 = (runup_TAW_local + 1.1f * eta);

            // this attribute is for debugging
            if (m_debugOn)
               {
               m_pNewDuneLineLayer->SetData(point, m_colComputedSlope, slope_TAW_local);
               m_pNewDuneLineLayer->SetData(point, m_colComputedSlope2, slope_TAW_local_2);
               }

            dailyTWL = swl + (float)structureR2;
            }  // end of: else if (beachType == BchT_RIPRAP_BACKED ||
               //                  beachType == BchT_DUNE_BLUFF_BACKED ||
               //                  beachType == BchT_CLIFF_BACKED ||
               //                  beachType == BchT_WOODY_DEBRIS_BACKED ||
               //                  beachType == BchT_WOODY_BLUFF_BACKED ||
               //                  beachType == BchT_ROCKY_HEADLAND)

         // check for dune hours
         // collision regime (dune erosion expected

         //dailyTWL -= .23f;

         if (dailyTWL > duneToe && dailyTWL < duneCrest)
            {
            // hold running total of number of days dune toe is impacted
            m_pNewDuneLineLayer->GetData(point, m_colIDDToe, IDDToeCount);
            m_pNewDuneLineLayer->SetData(point, m_colIDDToe, ++IDDToeCount);

            // winter impact days per year on dune toe
            if (doy > 355 || doy < 79)
               {
               m_pNewDuneLineLayer->GetData(point, m_colIDDToeWinter, IDDToeCountWinter);
               m_pNewDuneLineLayer->SetData(point, m_colIDDToeWinter, ++IDDToeCountWinter);
               }
            // Spring impact days per year on dune toe
            else if (doy > 79 && doy < 172)
               {
               m_pNewDuneLineLayer->GetData(point, m_colIDDToeWinter, IDDToeCountSpring);
               m_pNewDuneLineLayer->SetData(point, m_colIDDToeWinter, ++IDDToeCountSpring);
               }
            // Summer impact days per year on dune toe
            else if (doy > 172 && doy < 266)
               {
               m_pNewDuneLineLayer->GetData(point, m_colIDDToeWinter, IDDToeCountSummer);
               m_pNewDuneLineLayer->SetData(point, m_colIDDToeWinter, ++IDDToeCountSummer);
               }
            // Fall impact days per year on dune toe
            else
               {
               m_pNewDuneLineLayer->GetData(point, m_colIDDToeWinter, IDDToeCountFall);
               m_pNewDuneLineLayer->SetData(point, m_colIDDToeWinter, ++IDDToeCountFall);
               }
            }
         // overtopped 
         // dune is flooded
         if (dailyTWL > duneCrest)
            {
            // hold running total of number of days dune crest is impacted
            m_pNewDuneLineLayer->GetData(point, m_colIDDCrest, IDDCrestCount);
            m_pNewDuneLineLayer->SetData(point, m_colIDDCrest, ++IDDCrestCount);

            // Winter impact days per year on dune crest
            if (doy > 355 || doy < 79)
               {
               m_pNewDuneLineLayer->GetData(point, m_colIDDCrestWinter, IDDCrestCountWinter);
               m_pNewDuneLineLayer->SetData(point, m_colIDDCrestWinter, ++IDDCrestCountWinter);
               }
            // Spring impact days per year on dune crest
            else if (doy > 79 && doy < 172)
               {
               m_pNewDuneLineLayer->GetData(point, m_colIDDCrestSpring, IDDCrestCountSpring);
               m_pNewDuneLineLayer->SetData(point, m_colIDDCrestSpring, ++IDDCrestCountSpring);
               }
            // Summer impact days per year on dune crest
            else if (doy > 172 && doy < 266)
               {
               m_pNewDuneLineLayer->GetData(point, m_colIDDCrestSummer, IDDCrestCountSummer);
               m_pNewDuneLineLayer->SetData(point, m_colIDDCrestSummer, ++IDDCrestCountSummer);
               }
            // Fall impact days per year on dune crest
            else
               {
               m_pNewDuneLineLayer->GetData(point, m_colIDDCrestFall, IDDCrestCountFall);
               m_pNewDuneLineLayer->SetData(point, m_colIDDCrestFall, ++IDDCrestCountFall);
               }
            }

         // get current Maximum TWL from coverage
         float yearlyMaxTWL = 0.0f;
         m_pNewDuneLineLayer->GetData(point, m_colYrMaxTWL, yearlyMaxTWL);

         // store the yearly average TWL
         float avgTWL = 0.0f;
         m_pNewDuneLineLayer->GetData(point, m_colYrAvgTWL, avgTWL);

         avgTWL += dailyTWL / 364.0f;
         m_pNewDuneLineLayer->SetData(point, m_colYrAvgTWL, avgTWL);

         //m_maxTWLArray.Set(duneIndx, row, dailyTWL);

         // store the yearly average SWL
         float avgSWL = 0.0f;
         m_pNewDuneLineLayer->GetData(point, m_colYrAvgSWL, avgSWL);

         avgSWL += swl / 364.0f;
         m_pNewDuneLineLayer->SetData(point, m_colYrAvgSWL, avgSWL);

         // store the yearly average low SWL
         float avgLowSWL = 0.0f;
         m_pNewDuneLineLayer->GetData(point, m_colYrAvgLowSWL, avgLowSWL);

         avgLowSWL += lowSWL / 364.0f;
         m_pNewDuneLineLayer->SetData(point, m_colYrAvgLowSWL, avgLowSWL);

         float yearlyMaxSWL = 0.0f;
         m_pNewDuneLineLayer->GetData(point, m_colYrMaxSWL, yearlyMaxSWL);

         // set variables when Maximum yearly TWL
         if (swl > yearlyMaxSWL)
            {
            if (beachType != BchT_UNDEFINED)
               {
               m_pNewDuneLineLayer->SetData(point, m_colYrMaxSWL, swl);
               }
            }

         if (dailyTWL > yearlyMaxTWL)
            {
            ////Are we near an inlet? If so, swash is not inlcuded in the twl term -- (this statement is meaningless  legacy form Tillamook -cls)
            //if (beachType == BchT_BAY)
            //{
            //   m_pNewDuneLineLayer->SetData(point, m_colYrMaxTWL, dailyTWL);
            //}
            //else 
            if (beachType != BchT_UNDEFINED)
               {
               m_pNewDuneLineLayer->SetData(point, m_colYrFMaxTWL, dailyTWL_2);
               m_pNewDuneLineLayer->SetData(point, m_colYrMaxTWL, dailyTWL);
               }

            ASSERT(Hdeep > 0.0f);
            m_pNewDuneLineLayer->SetData(point, m_colHdeep, Hdeep);
            m_pNewDuneLineLayer->SetData(point, m_colWPeriod, wavePeriod);
            m_pNewDuneLineLayer->SetData(point, m_colWHeight, waveHeight);
            m_pNewDuneLineLayer->SetData(point, m_colWDirection, waveDirection);
            m_pNewDuneLineLayer->SetData(point, m_colYrMaxDoy, doy);
            //      m_pNewDuneLineLayer->SetData(point, m_colYrMaxSWL, swl);
            m_pNewDuneLineLayer->SetData(point, m_colYrMaxSetup, eta);
            m_pNewDuneLineLayer->SetData(point, m_colYrMaxIGSwash, STK_IG);
            m_pNewDuneLineLayer->SetData(point, m_colYrMaxIncSwash, STK_INC);
            m_pNewDuneLineLayer->SetData(point, m_colYrMaxSwash, STK_Swash);

            if (m_debugOn)
               {
               m_pNewDuneLineLayer->SetData(point, m_colSTKRunup, STK_Runup);
               m_pNewDuneLineLayer->SetData(point, m_colSTKR2, StockdonR2);
               m_pNewDuneLineLayer->SetData(point, m_colStructR2, structureR2);
               }

            /*if (dailyTWL_2 > duneCrest && (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED))
            {
            m_pNewDuneLineLayer->SetData(point, m_colFlooded, 1);
            m_pNewDuneLineLayer->SetData(point, m_colAmtFlooded, (dailyTWL - duneCrest));
            }*/

            if (dailyTWL > duneCrest && (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED))
               {
               m_pNewDuneLineLayer->SetData(point, m_colFlooded, 1);
               m_pNewDuneLineLayer->SetData(point, m_colAmtFlooded, (dailyTWL - duneCrest));
               }

            //    m_pNewDuneLineLayer->SetData(point, m_colTranSlope, slope_TAW_local)

            /*SetUpArray[ duneIndx ] = eta;
            IGArray[ duneIndx ] = STK_IG;
            wlArray[ duneIndx ] = swl;
            stkTWLArray[ duneIndx ] = STK_TWL;*/
            }

         /*  if (ftwl > maxFTWLforPeriod)
         {
         periodMaxTWL = row;
         tranIndxMaxTWL = tranIndx;
         maxTWLforPeriod = twl;
         }

         if (ftwl > m_maxFTWL)
         {
         periodMaxTWL = row;
         tranIndxMaxTWL = tranIndx;
         m_maxTWL = twl;
         }*/

         /* prevProfileIndx = profileIndx;
         prevTranIndx = tranIndx;
         prevTanb1 = tanb1;
         prevDuneCrest = duneCrest;*/

         }  // each dune point in dune line


      bool runSpatialBayTWL = (m_runSpatialBayTWL == 1) ? true : false;
      if (runSpatialBayTWL)
         {
         m_pBayTraceLayer->m_readOnly = false;

         int colEasting = m_pBayLUT->GetCol("Easting");
         int colNorthing = m_pBayLUT->GetCol("Northing");

         // iterate though Bay Trace points, calculate TWL and associated variables as we go.
         for (MapLayer::Iterator point = m_pBayTraceLayer->Begin(); point < m_pBayTraceLayer->End(); point++)
            {
            float swl = 0.0f;
            float nextSwl = 0.0f;
            float dailyTWL = 0.0f;
            int traceIndx = -1;
            int nextTraceIndx = -1;

            MapLayer::Iterator nextPoint = point;

            m_pBayTraceLayer->GetData(point, m_colBayTraceIndx, traceIndx);

            if (traceIndx >= 0)
               {
               FDataObj *pBayStation = m_DailyBayDataArray.GetAt(traceIndx);
               pBayStation->Get(0, row, swl);

               int thisRow = m_pBayLUT->Find(0, VData(traceIndx), 0);

               double northing = m_pBayLUT->Get(colNorthing, thisRow);
               double easting = m_pBayLUT->Get(colEasting, thisRow);

               nextTraceIndx = traceIndx + 1;

               FDataObj *pNextBayStation = m_DailyBayDataArray.GetAt(nextTraceIndx);
               pNextBayStation->Get(0, row, nextSwl);

               int nextRow = m_pBayLUT->Find(0, VData(nextTraceIndx), 0);
               double nextNorthing = m_pBayLUT->Get(colNorthing, nextRow);
               double nextEasting = m_pBayLUT->Get(colEasting, nextRow);

               double distance = sqrt(pow((nextNorthing - northing), 2.0) + pow((nextEasting - easting), 2.0));

               dailyTWL = swl;

               // get current Maximum TWL from coverage
               float yearlyMaxTWL = 0.0f;
               m_pBayTraceLayer->GetData(point, m_colBayYrMaxTWL, yearlyMaxTWL);

               // store the yearly average TWL
               float avgTWL = 0.0f;
               m_pBayTraceLayer->GetData(point, m_colBayYrAvgTWL, avgTWL);

               avgTWL += dailyTWL / 364.0f;
               m_pBayTraceLayer->SetData(point, m_colBayYrAvgTWL, avgTWL);

               float yearlyMaxSWL = 0.0f;
               m_pBayTraceLayer->GetData(point, m_colBayYrMaxSWL, yearlyMaxSWL);

               // store the yearly average SWL
               float avgSWL = 0.0f;
               m_pBayTraceLayer->GetData(point, m_colBayYrAvgSWL, avgSWL);

               avgSWL += swl / 364.0f;
               m_pBayTraceLayer->SetData(point, m_colBayYrAvgSWL, avgSWL);

               if (swl > yearlyMaxSWL)
                  {
                  m_pBayTraceLayer->SetData(point, m_colBayYrMaxSWL, swl);
                  m_pBayTraceLayer->SetData(point, m_colBayYrMaxTWL, dailyTWL);
                  }

               if (distance <= 500.0)
                  {
                  nextPoint++;
                  m_pBayTraceLayer->GetData(nextPoint, m_colBayTraceIndx, nextTraceIndx);

                  // interpolate swl between two values (if distance is less than 500m)
                  float value = (nextSwl - swl) / ((float)distance / 10.0f);
                  while (nextTraceIndx == -1 && nextPoint < m_pBayTraceLayer->End())
                     {
                     swl += value;
                     dailyTWL = swl;

                     m_pBayTraceLayer->GetData(nextPoint, m_colBayTraceIndx, nextTraceIndx);

                     // get current Maximum TWL from coverage
                     float yearlyMaxTWL = 0.0f;
                     m_pBayTraceLayer->GetData(nextPoint, m_colBayYrMaxTWL, yearlyMaxTWL);

                     // store the yearly average TWL
                     float avgTWL = 0.0f;
                     m_pBayTraceLayer->GetData(nextPoint, m_colBayYrAvgTWL, avgTWL);

                     avgTWL += dailyTWL / 364.0f;
                     m_pBayTraceLayer->SetData(nextPoint, m_colBayYrAvgTWL, avgTWL);

                     float yearlyMaxSWL = 0.0f;
                     m_pBayTraceLayer->GetData(nextPoint, m_colBayYrMaxSWL, yearlyMaxSWL);

                     // store the yearly average SWL
                     float avgSWL = 0.0f;
                     m_pBayTraceLayer->GetData(nextPoint, m_colBayYrAvgSWL, avgSWL);

                     avgSWL += swl / 364.0f;
                     m_pBayTraceLayer->SetData(nextPoint, m_colBayYrAvgSWL, avgSWL);

                     if (swl > yearlyMaxSWL)
                        {
                        m_pBayTraceLayer->SetData(nextPoint, m_colBayYrMaxSWL, swl);
                        m_pBayTraceLayer->SetData(nextPoint, m_colBayYrMaxTWL, dailyTWL);
                        }

                     nextPoint++;
                     } // end while
                  } // end if spatially connected
               }

            //if (dailyTWL > yearlyMaxTWL)
            //{
            //}

            } // end Bay Trace Layer Pts

         } // end run spatially varying TWLs

      } // end doy

   m_pNewDuneLineLayer->ClassifyData();
   m_pBayTraceLayer->ClassifyData();

   //m_pProfileLUT->WriteAscii(_T("C:\\temp\\ProfileLookup.csv"));
   //m_maxTWLArray.WriteAscii(_T("C:\\temp\\twl.csv"));

   } // end CalculateYrMaxTWL

bool ChronicHazards::CalculateUTM2LL(MapLayer *pLayer, int point)
   {
   double easting = 0.0;
   pLayer->GetData(point, m_colEastingToe, easting);
   double northing = 0.0;
   pLayer->GetData(point, m_colNorthingToe, northing);

   GDALWrapper gdal;
   GeoSpatialDataObj geoSpatialObj(U_UNDEFINED);
   CString projectionWKT = m_pOrigDuneLineLayer->m_projection;
   int utmZone = geoSpatialObj.GetUTMZoneFromPrjWKT(projectionWKT);

   // 22 = WGS 84 EquatorialRadius, and 1/flattening.  UTM zone 10.
   double longitude = 0.0;
   double latitude = 0.0;
   gdal.UTMtoLL(22, northing, easting, utmZone, latitude, longitude);

   pLayer->SetData(point, m_colLatitudeToe, latitude);
   pLayer->SetData(point, m_colLongitudeToe, longitude);

   return true;
   } // end CalculateUTM2LL


bool ChronicHazards::AddToPolicySummary(int year, int policyID, int locator, float unitCost, float cost, float availBudget, float param1, float param2, bool passCostConstraint)
   {
   CString policyName;
   switch (policyID)
      {
      case PI_CONSTRUCT_BPS:                    policyName = "Construct BPS"; break;
      case PI_MAINTAIN_BPS:                     policyName = "Maintain BPS"; break;
      case PI_NOURISH_BY_TYPE:                  policyName = "Nourish BPS"; break;
      case PI_CONSTRUCT_SPS:                    policyName = "Construct SPS"; break;
      case PI_MAINTAIN_SPS:                     policyName = "Maintain SPS"; break;
      case PI_NOURISH_SPS:                      policyName = "Nourish SPS"; break;
         //case //PI_CONSTRUCT_SAFEST_SITE:        policyName=""                                 ; break;
      case PI_REMOVE_BLDG_FROM_HAZARD_ZONE:     policyName = "Remove Bldg from Hazard Zone"; break;
      case PI_REMOVE_INFRA_FROM_HAZARD_ZONE:    policyName = "Remove Infra From Hazard Zone"; break;
      case PI_RAISE_RELOCATE_TO_SAFEST_SITE:    policyName = "Raise/Relocate to Safest Site"; break;
      case PI_RAISE_INFRASTRUCTURE:             policyName = "Raise Infrastructure"; break;
      }


   fprintf(::fpPolicySummary, "\n%i,%s,%i,%.2f,%i,%i,%f,%f,%i",
      year, (LPCTSTR)policyName, locator, unitCost, int(cost), int(availBudget), param1, param2, passCostConstraint ? 1 : 0);

   return true;
   }


