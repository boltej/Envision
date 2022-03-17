#pragma once


#include <EnvExtension.h>

#include <Vdataobj.h>
#include <Idataobj.h>
#include <DDataObj.h>
#include <FDataObj.h>

#include <maplayer.h>
#include <tixml.h>
#include "MovingWindow.h"

#include <map>
#include <vector>

#include <PolyGridMapper.h>
#include <PolyPtMapper.h>
#include <AlgLib\AlgLib.h>


//#define N 1854

#define N 8

// this privides a basic class definition for this
// plug-in module.
// Note that we want to override the parent methods
// for Init, InitRun, and Run.



#define _EXPORT __declspec( dllexport )




enum CH_FLAGS
   {
   CH_NONE = 0,
   CH_MODEL_TWL = 1,
   CH_MODEL_FLOODING = 2,
   CH_MODEL_EROSION = 4,
   CH_MODEL_BAYFLOODING = 8,

   CH_REPORT_BUILDINGS = 64,
   
   CH_ALL = 127
   };


enum BEACHTYPE
   {
   BchT_UNDEFINED = 0,
   BchT_SANDY_DUNE_BACKED = 1,
   BchT_SANDY_BLUFF_BACKED,
   BchT_RIPRAP_BACKED,
   BchT_SEAWALL_BACKED,
   BchT_DUNE_BLUFF_BACKED,
   BchT_CLIFF_BACKED,
   BchT_WOODY_DEBRIS_BACKED,
   BchT_WOODY_BLUFF_BACKED,
   BchT_BAY,
   BchT_RIVER,
   BchT_ROCKY_HEADLAND
   };

enum FLOODHAZARDZONE
   {
   FHZ_OPENWATER = -1,
   FHZ_ONEHUNDREDYR = 1,			// Flood Zones : A, AE, AH, AH, AO, VE, FW
   FHZ_FIVEHUNDREDYR = 2,			// Flood Zones: X, X500, X PROTECTED BY LEVEE
   FHZ_UNSTUDIED = 3				// Tahaloh in latest FEMA coverage
   };

enum
   {
   HEIGHT_L, HEIGHT_H,
   PERIOD_L, PERIOD_H,
   DIRECTION_L, DIRECTION_H,
   DEPTH_L, DEPTH_H,
   X, Y
   };


enum  // flags for ReducebuoyObsData
   {
   PERIOD_HOURLY,
   PERIOD_MONTHLY,
   PERIOD_ANNUAL
   };

struct TWLDATAFILE
   {
   CString timeseries_filename;
   CString timeseries_file;
   CString slr;
   };


struct TWLDataFile
   {
   CString timeseries_filename;
   CString timeseries_file;
   CString NRCslr;
   };


struct COSTS
   {
   float nourishment;
   float BPS;
   int permits;
   float BPSMaint;
   float SPS;
   float safeSite;
   };

class LookupTable
   {
   public:
      FDataObj m_data;
      ALRadialBasisFunction3D m_rbf;

   };

// Bates Flooding Model
class BatesFlood
   {
   public:
      int m_runBatesFlood;
      CString m_cityDir;
      float m_duration;
      float m_ManningCoeff;
      float m_timeStep;
   };

class PolicyInfo
   {
   public:
      CString m_label;
      int m_index;  // corresponds to POLICY_ID below
      bool m_hasBudget;
      float m_startingBudget;     // budget at the beginning of the year (set in Run()) ($)
      float m_availableBudget;    // currently available budget (updated during Run()) ($)
      float m_spentBudget;		// annual expenditure
      float m_unmetDemand;        // current unmet demand (for this year, updated during Run()) ($)
      float m_allocFrac;	        // portion of budget from SpentBudget fraction
      bool m_usingMinBudget;

      MovingWindow m_movAvgBudgetSpent;
      MovingWindow m_movAvgUnmetDemand;

      static float m_budgetAllocFrac;

   protected:
      int *m_isActive;

   public:
      PolicyInfo::PolicyInfo() :
         m_index(-1),
         m_isActive(NULL),
         m_hasBudget(false),
         m_startingBudget(0),
         m_availableBudget(0),
         m_spentBudget(0),
         m_unmetDemand(0),
         m_movAvgBudgetSpent(10),
         m_movAvgUnmetDemand(10),
         m_usingMinBudget(false),
         m_allocFrac(0.5f)
         {
         }

      void Init(LPCTSTR label, int index, int *isActive, bool hasBudget)
         {
         m_label = label;  m_index = index; m_isActive = isActive; m_hasBudget = hasBudget;
         }

      bool IsActive() { return (m_isActive != NULL && *m_isActive != 0); }
      bool HasBudget() { return (IsActive() && m_hasBudget); }
      void IncurCost(float cost)
         {
         m_availableBudget -= cost;
         m_spentBudget += cost;
         }

   };

enum POLICY_ID
   {
   PI_CONSTRUCT_BPS = 0,
   PI_MAINTAIN_BPS,
   PI_NOURISH_BY_TYPE,
   PI_CONSTRUCT_SPS,
   PI_MAINTAIN_SPS,
   PI_NOURISH_SPS,
   //PI_CONSTRUCT_SAFEST_SITE,
   PI_REMOVE_BLDG_FROM_HAZARD_ZONE,
   PI_REMOVE_INFRA_FROM_HAZARD_ZONE,
   PI_RAISE_RELOCATE_TO_SAFEST_SITE,
   PI_RAISE_INFRASTRUCTURE,
   PI_COUNT  // must be last
   };


class _EXPORT ChronicHazards : public EnvModelProcess
   {
   public:
      ChronicHazards(void);
      ~ChronicHazards(void);

      // main Envision entry points
      bool Init(EnvContext *pEnvContext, LPCTSTR initStr);
      bool InitRun(EnvContext *pEnvContext, bool useInitSeed);
      bool Run(EnvContext *pContext);
      bool EndRun(EnvContext *pEnvContext);
 


      // int ChronicHazards::ResizeAndCalculateBeta(long &rows, long &cols);


      ////  CString m_polyGridFName;
      //PolyGridLookups   *m_pPolyGridLkUp;          // store Poly-Grid lookup info for mapping grids to IDUs
      //FAGrid            *m_pGridMapLayer;
      //MapLayer *m_pPolyMapLayer;								// MapLayer object, to hold values of IDU.shp.
      //Map *m_pMap;												// Object to handle map pointer required to create grids.
      ////   bool LoadPolyGridLookup(MapLayer *pIDULayer);
      //bool LoadPolyGridLookup(EnvContext *pEnvContext);
   protected:
      bool LoadRBFs();
      //bool ResetAnnualVariables();
      //bool ResetAnnualBudget();
      //bool CalculateErosionExtents(EnvContext *pEnvContext);
      //bool RunEelgrassModel(EnvContext *pEnvContext);
      //bool GenerateFloodMap();
      bool AddToPolicySummary(int year, int policy, int loc, float unitCost, float cost, float availBudget, float param1, float param2, bool passCostConstraint);



      /*  void SetGridCellWidth(int val) { m_cellWidth = val; };
      void SetGridCellHeight(int val) { m_cellHeight = val; };
      int  GetGridCellSizeWidth(void) { return m_cellWidth; };
      int  GetGridCellSizeHeight(void) { return m_cellHeight; };

      void SetNumberSubGridCells(int val) { m_numSubGridCells = val; };
      int  GetNumberSubGridCells(void) { return m_numSubGridCells; };*/

      //bool RunTestCase(EnvContext *pEnvContext);

      // Initialize the Polygon <-> Grid relationship.
      // Uses the PolyGridLookups class to set up the spatial correlation between the IDU shapefile and a grid,
      // that is created based on the extent of the shapefile, a cellsize given by the variable m_cellDim and 
      // a number of subgridcells specified by the variable m_numSubGridCells. Furthermore, the calculated 
      // PolyGridLookup relation is written to a binary file (*.pgl), in case such a file does not yet exist.
      int ReducebuoyObsData(int period);
      int SetInfrastructureValues();

      // bool LoadPolyGridLookup();
      //  bool LoadPolyGridLookup(MapLayer *pGridLayer, MapLayer *pPolyLayer, CString filename, PolyGridLookups *&pLookupLayer);

   protected:
      int m_runFlags;
      Map * m_pMap;									// ptr to Map required to create grids
      MapLayer *m_pIDULayer;							// ptr to IDU Layer
      MapLayer *m_pRoadLayer;                         // ptr to Road Layer
      MapLayer *m_pBldgLayer;                         // ptr to Building Layer
      MapLayer *m_pInfraLayer;                        // ptr to Infrastructure Building Layer
      MapLayer *m_pElevationGrid;                     // ptr to Minimum Elevation Grid

      MapLayer *m_pBayBathyGrid;                     // ptr to Bay Bathymetry Grid
      MapLayer *m_pBayTraceLayer;
      MapLayer *m_pManningMaskGrid;                  // ptr to Bates Model Mannings Coefficient Grid (value = -1 if not running Bates model in region)
      MapLayer *m_pTidalBathyGrid;                   // ptr to Tidal Bathymetry Grid

      MapLayer *m_pOrigDuneLineLayer;                 // ptr to Original Dune Line Layer
      MapLayer *m_pNewDuneLineLayer;                  // ptr to Changed Dune Line Layer

                                                      //   MapLayer *m_pTsunamiInunLayer;                  // ptr to Tsunami Inundaiton Layer

      PolyGridMapper *m_pPolygonGridLkUp;				// provides mapping between IDU layer and DEM grid
      PolyGridMapper *m_pPolylineGridLkUp;			// provides mapping between roads layer and DEM grid
      PolyPointMapper *m_pIduBuildingLkup;			// provides mapping between IDU layer, building (point) layer
      PolyPointMapper *m_pPolygonInfraPointLkup;		// provides mapping between IDU layer, infrastructure (point) layer

                                                      // Created grids that overlay DEM grid
      MapLayer *m_pFloodedGrid;                       // ptr to Flooded Grid
      MapLayer *m_pCumFloodedGrid;                    // ptr to Flooded Grid
      MapLayer *m_pErodedGrid;                        // ptr to Eroded Grid

      MapLayer *m_pEelgrassGrid;                      // ptr to Eel Grass Grid

                                                      // Bates Flooding Model
      MapLayer *m_pNewElevationGrid;
      MapLayer *m_pWaterElevationGrid;                   // ptr to Flooded Water Depth Grid
      MapLayer *m_pDischargeGrid;                     // ptr to Volumetric Flow rate Grid

                                                      //PtrArray<BatesFlood> m_batesFloodArray;

      PtrArray< LookupTable > m_swanLookupTableArray;
      //	  PtrArray< DDataObj > m_SurfZoneDataArray;
      PtrArray< DDataObj > m_BathyDataArray;
      PtrArray< FDataObj > m_RBFDailyDataArray;
      PtrArray< FDataObj > m_DailyBayDataArray;

      // Moving Windows for Dune Line Statistics
      PtrArray< MovingWindow > m_IDPYArray;
      PtrArray< MovingWindow > m_TWLArray;
      PtrArray< MovingWindow > m_eroKDArray;

      // Moving Windows for Building Statistics
      PtrArray< MovingWindow > m_eroBldgFreqArray;
      PtrArray< MovingWindow > m_floodBldgFreqArray;

      // Moving Windows for Infrastructure Statistics
      PtrArray< MovingWindow > m_eroInfraFreqArray;
      PtrArray< MovingWindow > m_floodInfraFreqArray;

      // Lookup tables for transects and cross shore profiles
      DDataObj *m_pInletLUT;
      DDataObj *m_pBayLUT;
      DDataObj *m_pProfileLUT;
      DDataObj *m_pSAngleLUT;
      DDataObj *m_pTransectLUT;

      // Policy allocations/expenditures
      CArray<PolicyInfo, PolicyInfo&> m_policyInfoArray;
      //vector<double> m_prevExpenditures;
      //FDataObj *m_pResidualArray;
      FDataObj m_policyCostData;

      // Lookup table for yearly Sea Level Rise data
      DDataObj m_slrData;

      // Deep water simulation Daily Time Series
      FDataObj m_buoyObsData;
      //FDataObj m_maxTWLArray;

      /*DDataObj m_TideData;
      DDataObj m_TimeData;
      DDataObj m_SimulationData; */
      DDataObj m_reducedbuoyObsData;

      // Setup variables

      CString m_cityDir;
      bool m_debugOn;

      int
         m_debug,
         m_numBayBathyPts,
         m_numBayPts,
         m_numBayStations,
         m_numBldgs,
         m_numDays,
         m_numDunePts,
         m_numProfiles,
         m_numTransects,
         m_numYears,
         m_simulationCount,
         m_maxDuneIndx,
         /*	m_maxTraceIndx,
         m_minTraceIndx,*/
         m_maxTransect,
         m_minTransect,
         m_randIndex;

      // Policy scenario xml variables 
      int m_climateScenarioID,						// 0=BaseSLR, 1=LowSLR, 2=MedSLR, 3=HighSLR
         m_runConstructBPSPolicy,					// 0=Off, 1=On
         m_runConstructSafestPolicy,					// 0=Off, 1=On
         m_runConstructSPSPolicy,					// 0=Off, 1=On
         m_runMaintainBPSPolicy,						// 0=Off, 1=On
         m_runMaintainSPSPolicy,						// 0=Off, 1=On
         m_runNourishByTypePolicy,					// 0=Off, 1=On
         m_runNourishSPSPolicy,						// 0=Off, 1=On
         m_runRaiseInfrastructurePolicy,				// 0=Off, 1=On
         m_runRelocateSafestPolicy,					// 0=Off, 1=On	
         m_runRemoveBldgFromHazardZonePolicy,			// 0=Off, 1=On
         m_runRemoveInfraFromHazardZonePolicy;			// 0=Off, 1=On

                                                      //   xml variables 
      int
         //		m_period,		
         m_exportMapInterval,
         m_findSafeSiteCell,
         m_runBayFloodModel,				// Run Bay Flooding Model: 0=Off; 1=On
         m_runEelgrassModel,				// Run Eelgrass Model: 0=Off; 1=On
         m_runSpatialBayTWL,             // Run Spatially varying TWLs in Bay: 0=Off; 1=On
         m_writeDailyData,
         m_writeRBF;

      int
         m_bfeCount,
         m_eroCount,
         m_floodCountBPS,
         m_floodCountSPS,
         m_maintFreqBPS,
         //		m_maintFreqSPS,

         m_nourishFactor,
         m_nourishFreq,
         m_nourishPercentage,
         m_ssiteCount,
         m_useMaxTWL,
         m_windowLengthEroHzrd,
         m_windowLengthFloodHzrd,
         m_windowLengthTWL;

      float
         m_accessThresh,
         m_avgBackSlope,
         m_avgFrontSlope,
         m_avgDuneCrest,
         m_avgDuneHeel,
         m_avgDuneToe,
         m_avgDuneWidth,
         m_avgWL0,
         m_constTD,
         m_delta,
         m_minDistToProtecteBldg,
         /*	m_duneCrestSPS,
         m_duneHeelSPS,
         m_duneToeSPS,*/
         m_idpyFactor,
         m_inletFactor,
         m_maxArea,
         m_maxBPSHeight,
         m_minBPSHeight,
         m_minBPSRoughness,
         m_radiusOfErosion,
         m_rebuildFactorSPS,
         m_safetyFactor,
         m_slopeBPS,
         m_slopeThresh,
         m_shorelineLength,
         m_topWidthSPS,
         m_removeFromHazardZoneRatio,
         m_raiseRelocateSafestSiteRatio,
         m_totalBudget;

      // Grid properties
      int m_numRows;								   // Number of rows in DEM, Flooded, Eroded grid
      int m_numCols;								   // Number of columns in DEM, Flooded, Eroded grid
      REAL m_cellWidth;							   // cell Width (m) in DEM, Flooded, Eroded grid
      REAL m_cellHeight;							   // cell Height (m) in DEM, Flooded, Eroded grid

      int m_numBayRows;							   // Number of rows in Bay Bathy, Eelgrass grid
      int m_numBayCols;							   // Number of columns in Bay Bathy, Eelgrass grid	
      REAL m_bayCellWidth;						   // cell Width (m) in Bay Bathy, Eelgrass grid
      REAL m_bayCellHeight;						   // cell Height (m) in Bay Bathy, Eelgrass grid

      int m_numTidalRows;							   // Number of rows in Tidal Bathy grid
      int m_numTidalCols;							   // Number of columns in Tidal Bathy grid	
      REAL m_tidalCellWidth;						   // cell Width (m) in Tidal Bathy grid
      REAL m_tidalCellHeight;						   // cell Height (m) in Tidal Bathy grid

                                                // Bates Flooding Model
      int m_runBatesFlood;
      float m_batesFloodDuration;
      float m_ManningCoeff;
      float m_timeStep;

      // County Wide Metrics
      int
         m_erodedBldgCount,
         m_floodedBldgCount,
         m_floodedInfraCount,
         m_noBldgsRemovedFromHazardZone,
         m_noConstructedBPS,
         m_noConstructedDune;
      //		m_numCntyNourishProjects,
      //		m_numCtnyNourishPrjts,

      float
         m_avgCountyAccess;

      double
         //		m_eroFreq,
         //		m_floodFreq, 
         m_addedHardenedShoreline,
         m_addedHardenedShorelineMiles,
         m_addedRestoredShoreline,
         m_addedRestoredShorelineMiles,
         m_constructCostBPS,
         m_constructVolumeSPS,
         m_eelgrassArea,
         m_eelgrassAreaSqMiles,
         m_erodedRoad,
         m_erodedRoadMiles,
         m_floodedArea,
         m_floodedAreaSqMiles,
         m_floodedRailroadMiles,
         m_floodedRoad,
         m_floodedRoadMiles,
         m_intertidalArea,
         m_intertidalAreaSqMiles,
         m_maintCostBPS,
         m_maintCostSPS,
         m_maintVolumeSPS,
         //		m_maxTWL,
         //		m_meanErosion,
         m_meanTWL,
         m_nourishVolumeBPS,
         m_nourishVolumeSPS,
         m_nourishCostBPS,
         m_nourishCostSPS,
         m_percentHardenedShoreline,
         m_percentRestoredShoreline,
         m_percentHardenedShorelineMiles,
         m_percentRestoredShorelineMiles,
         m_restoreCostSPS,
         m_totalHardenedShoreline,
         m_totalHardenedShorelineMiles,
         m_totalRestoredShoreline,
         m_totalRestoredShorelineMiles;


      // IDU coverage columns
      int
         m_colArea,
         m_colBaseFloodElevation,
         m_colBeachfront,
         //m_colBldgValue,
         m_colCPolicy,
         m_colEasementYear,
         m_colExistDU,
         m_colFID,
         m_colFloodFreq,
         m_colFracFlooded,
         m_colIDUAddBPSYr,
         m_colIDUCityCode,
         m_colIDUEroded,
         m_colIDUFlooded,
         m_colIDUFloodZone,
         m_colIDUFloodZoneCode,
         m_colImpValue,
         m_colLandValue,
         m_colLength,
         m_colMaxElevation,
         m_colMinElevation,
         m_colNDU,
         m_colNEWDU,
         m_colNorthingBottom,
         m_colNorthingTop,
         m_colNumStruct,
         m_colPopCap,
         m_colPopDensity,
         m_colPopulation,
         m_colPropEroded,
         m_colRemoveBldgYr,
         m_colRemoveSC,
         m_colSafeLoc,
         m_colSafeSite,
         m_colSafeSiteYear,
         m_colSafestSiteCol,
         m_colSafestSiteRow,
         m_colTaxlotID,
         m_colTsunamiHazardZone,
         m_colType,
         m_colValue,
         m_colZone,

         // Dune Line coverage columns      
         //		m_colFreeboard,	
         //		m_colInlet, 	
         //		m_colKDstd, 	
         //		m_colSA, 	
         //		m_colTWL, 	
         m_colA,
         m_colAddYearBPS,
         m_colAddYearSPS,
         m_colAlpha,
         m_colAlpha2,
         m_colAmtFlooded,
         m_colAvgKD,
         //		m_colAvgDuneC,
         //		m_colAvgDuneT,
         //		m_colAvgEro,
         m_colBackshoreElev,
         m_colBackSlope,
         m_colBchAccess,
         m_colBchAccessFall,
         m_colBchAccessSpring,
         m_colBchAccessSummer,
         m_colBchAccessWinter,
         m_colBeachType,
         m_colBeachWidth,
         m_colConstructVolumeSPS,
         m_colFrontSlope,
         m_colLengthBPS,
         m_colLengthSPS,
         m_colRemoveYearBPS,
         m_colCalDate,
         m_colCalDCrest,
         m_colCalDToe,
         m_colCalEroExt,
         m_colCostBPS,
         m_colCostMaintBPS,
         m_colCostSPS,
         m_colCPolicyL,
         m_colDuneBldgDist,
         m_colDuneBldgIndx,
         m_colDuneCrest,
         m_colDuneEroFreq,
         m_colDuneFloodFreq,
         m_colDuneHeel,
         m_colDuneIndx,
         m_colDuneToe,
         m_colDuneToeSPS,
         m_colDuneWidth,
         m_colEastingCrest,
         m_colEastingCrestProfile,
         m_colEastingCrestSPS,
         m_colEastingShoreline,
         m_colEastingToe,
         m_colEastingToeBPS,
         m_colEastingToeSPS,
         //		m_colEPR,
         m_colFlooded,
         m_colForeshore,
         m_colGammaBerm,
         m_colGammaRough,
         m_colHb,
         m_colHdeep,
         m_colHeelWidthSPS,
         m_colHeightBPS,
         m_colHeightSPS,
         m_colIDDCrest,
         m_colIDDCrestFall,
         m_colIDDCrestSpring,
         m_colIDDCrestSummer,
         m_colIDDCrestWinter,
         m_colIDDToe,
         m_colIDDToeFall,
         m_colIDDToeSpring,
         m_colIDDToeSummer,
         m_colIDDToeWinter,
         m_colKD,
         m_colLatitudeToe,
         m_colLittCell,
         m_colLongitudeToe,
         m_colMaintVolumeSPS,
         m_colMaintYearBPS,
         m_colMaintYearSPS,
         m_colMvAvgIDPY,
         m_colMvAvgTWL,
         m_colMvMaxTWL,
         m_colNorthingCrest,
         m_colNorthingToe,
         m_colNourishFreqBPS,
         m_colNourishFreqSPS,
         m_colNourishYearBPS,
         m_colNourishYearSPS,
         m_colNourishVolBPS,
         m_colNourishVolSPS,
         m_colOrigBeachType,
         m_colPrevCol,
         m_colPrevDuneCr,
         m_colPrevRow,
         m_colPrevSlope,
         m_colProfileIndx,
         m_colScomp,
         m_colShoreface,
         m_colShorelineAngle,
         m_colShorelineChange,
         m_colTD,
         m_colThreshDate,
         m_colTopWidthSPS,
         m_colTranIndx,
         m_colTranSlope,
         m_colTS,
         m_colWb,
         m_colWDirection,
         m_colWidthBPS,
         m_colWidthSPS,
         m_colWHeight,
         m_colWPeriod,
         m_colXAvgKD,
         m_colYrAvgLowSWL,
         m_colYrAvgSWL,
         m_colYrAvgTWL,
         m_colYrMaxDoy,
         m_colYrMaxIGSwash,
         m_colYrMaxIncSwash,
         m_colYrMaxSetup,
         m_colYrMaxSwash,
         m_colYrMaxSWL,
         m_colYrFMaxTWL,
         m_colYrMaxTWL,

         // columns used for debugging in the dune line
         m_colBWidth,
         m_colComputedSlope,
         m_colComputedSlope2,
         m_colDeltaX,
         m_colDuneBldgEast,
         m_colDuneBldgIndx2,
         m_colNewEasting,
         m_colNumIDPY,
         m_colRunupFlag,
         m_colSTKRunup,
         m_colSTKR2,
         m_colSTKTWL,
         m_colStructR2,
         m_colTAWSlope,
         m_colTypeChange,

         // Bay Trace coverage columns
         m_colBayTraceIndx,
         m_colBayYrAvgSWL,
         m_colBayYrAvgTWL,
         m_colBayYrMaxSWL,
         m_colBayYrMaxTWL,

         // Road coverage columns
         m_colRoadFlooded,
         m_colRoadType,
         m_colRoadLength,
         // Buildings coverage columns		
         //		m_colBldgDuneDist,
         //		m_colBldgDuneIndx,
         //		m_colBldgMaxElev, 
         m_colBldgBFE,
         m_colBldgBFEYr,
         m_colBldgCityCode,
         m_colBldgEroded,
         m_colBldgEroFreq,
         m_colBldgFloodFreq,
         m_colBldgFlooded,
         //m_colBldgFloodHZ,
         m_colBldgFloodYr,
         m_colBldgIDUIndx,
         m_colBldgNDU,
         m_colBldgNEWDU,
         m_colBldgRemoveYr,
         m_colBldgSafeSite,
         m_colBldgSafeSiteYr,
         m_colBldgTsunamiHZ,
         m_colBldgValue,

         m_colNumBldgs,

         // Infrastructure coverage columns
         m_colInfraBFE,
         m_colInfraBFEYr,
         m_colInfraCount,
         m_colInfraCritical,
         m_colInfraDuneIndx,
         m_colInfraEroded,
         m_colInfraFlooded,
         m_colInfraFloodYr,
         m_colInfraValue;

      CString  m_polyGridFilename;
      CString  m_roadGridFilename;

      CString m_transectLookupFile;
      CString m_bathyFiles;
      //	CString m_surfZoneFiles;
      CString m_climateScenarioStr;

      //	CString  msg;									// String for error handling. 
      //	CString failureID;								// String for error handling.

      static TWLDATAFILE m_datafile;
      static COSTS m_costs;

      // flooding model
      //PtrArray<FloodArea> m_floodAreaArray;
      //bool InitializeFloodGrids(EnvContext *pContext, FloodArea *pFloodArea);
      //bool RunFloodAreas(EnvContext *pContext);


      //  ******  Methods ******

      bool LoadXml(LPCTSTR filename);

      /* bool WriteRBF(); */
      bool WriteDailyData(CString timeSeriesFolder);
      bool ReadDailyBayData(CString timeSeriesFolder);
      float CalculateCelerity(float waterLevel, float wavePeriod, float &n);
      double CalculateCelerity2(float waterLevel, float wavePeriod, double &n);


      void CalculateYrMaxTWL(EnvContext *pEnvContext);

      double GenerateBathtubFloodMap(int &floodedCount);
      double GenerateBatesFloodMap(int &floodedCount);

      float GenerateErosionMap(int &erodedCount);

      double GenerateEelgrassMap(int &eelgrassCount);

      bool InitializeWaterElevationGrid();
      bool CalculateFourQs(int row, int col, double &discharge);
      int VisitNeighbors(int row, int col, float twl, float &minElevation);
      int VisitNeighbors(int row, int col, float twl);
      int VNeighbors(int row, int col, float twl);

      //void generateFMap();
      //void floodFill(int x, int y, float newC);
      //void floodFillUtil(int x, int y, int startCol, double prevC, float newC);

      //float m_screen[ 8 ][ 8 ] = { { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0, 0.0 },
      //{ 1.2f, 0, 0, 1.5f, 1.0f, 0, 1.8f, 1.9f },
      //{ 1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 1.0f, 0 },
      //{ 1.0f, 1.0f, 1.0f, 2.0f, 1.3f, 0, 1.0f, 0 },
      //{ 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f, 0 },
      //{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 1.0f, 1.0f },
      //{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 1.0f },
      //};

      ///*double m_screen[ 8 ][ 8 ] = { { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 },{ 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0 },
      //	{ 1.2, 0, 0, 1.5, 1.0, 0, 1.8, 1.9 },
      //	{ 1.0, 2.0, 2.0, 2.0, 2.0, 2.0, 1.0, 0 },
      //	{ 1.0, 1.0, 1.0, 2.0, 1.3, 0, 1.0, 0 },
      //	{ 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0, 0 },
      //	{ 1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 1.0, 1.0 },
      //	{ 1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 2.0, 1.0 },
      //};*/

      //float m_flood[ 8 ][ 8 ] = { 0 };

      void ComputeBuildingStatistics();
      void ComputeFloodedIDUStatistics();
      void ComputeIDUStatistics();
      void TallyCostStatistics(int currentYear);
      void TallyDuneStatistics(int currentYear);
      void TallyRoadStatistics();
      void ComputeInfraStatistics();

      // Helper Functions
      int GetBeachType(MapLayer::Iterator pt);
      int GetNextBldgIndx(MapLayer::Iterator pt);

      void ExportMapLayers(EnvContext *pEnvContext, int outputYear);
      void RunPolicies(EnvContext *pEnvContext);

      // Build BPS with identical characteristics
      void ConstructBPS(int currentYear);

      // Build BPS with contour of shoreline
      void ConstructBPS1(int currentYear);

      // Build SPS with contour of shoreline
      void ConstructSPS(int currentYear);

      // Builds SPS to average GHC average characteristics -- not working
      void ConstructDune(int currentYear);

      //void RebuildSPS(int currentYear);

      //Calculates the area under a dune
      double CalculateArea(float duneToe, float duneCrest, float duneHeel, float frontSlope, float btmDuneWidth);

      // Returns true if construction of protection structure is triggered
      // Both Construction of Hard and Soft Protection Structures use this method to determine extent of construction
      bool CalculateImpactExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint, MapLayer::Iterator &minToePoint, MapLayer::Iterator &minDistPoint, MapLayer::Iterator &maxTWLPoint);

      // Returns true to trigger an SPS rebuild
      bool CalculateRebuildSPSExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint);

      bool CalculateExcessErosion(MapLayer::Iterator startPoint, float r, float s, bool north);

      // old version - unused
      void ConstructBPS2(int currentYear);
      int WalkSouth(MapLayer::Iterator pt, float newCrest, int currentYear, double top, double bottom);
      bool FindClosestDunePtToBldg(EnvContext *pEnvContext);
      //

      bool FindNourishExtent(MapLayer::Iterator &endPoint);
      bool FindNourishExtent(int rebuiltYear, MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint);

      bool CalculateSegmentLength(MapLayer::Iterator startPoint, MapLayer::Iterator endPoint, float &length);

      bool FindImpactExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint, bool isBPS);

      bool CalculateBPSExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint, MapLayer::Iterator &maxPoint);
      bool CalculateExtent(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint, MapLayer::Iterator &maxPoint);

      void MaintainBPS(int currentYear);
      void MaintainBPS2(int currentYear);
      void RemoveBPS(EnvContext *pEnvContext);

      void MaintainStructure(MapLayer::Iterator pt, int currentYear); // , bool isBPS);
      void MaintainSPS(int currentYear);

      bool FindProtectedBldgs();
      bool IsBldgImpactedByEErosion(int bldg, float avgEro, float distToBldg); // , float distance); //, int& iduIndex);
                                                                               //int GetIDUIndexOfBldg(int bldg);

      void NourishBPS(int currentYear, bool nourishBPS); //, int beachType);
      void NourishSPS(int currentYear); //, int beachType);
      void CompareBPSorNourishCosts(int currentYear);

      void ConstructOnSafestSite(EnvContext *pEnvContext, bool inFloodPlain);
      void RaiseOrRelocateBldgToSafestSite(EnvContext *pEnvContext);
      void RaiseInfrastructure(EnvContext *pEnvContext);
      void RemoveBldgFromHazardZone(EnvContext *pEnvContext);
      void RemoveInfraFromHazardZone(EnvContext *pEnvContext);
      void RemoveFromSafestSite();

      bool IsMissingRow(MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint);

      bool CalculateUTM2LL(MapLayer *pLayer, int point);
      double IGet(DDataObj *table, double x, int xcol, int ycol = 1, IMETHOD method = IM_LINEAR, int startRowIndex = 0, int *returnRowIndex = NULL, bool forwardDirection = true);

      // Tillamook unused
      double Maximum(double *dat, int length);
      double Minimum(double *dat, int length);
      void Mminvinterp(double *x, double*y, double *&xo, int length);

      template<class T>
      inline BOOL isNan(T arg)
         {
         //nans always return false when asked if equal to themselves
         return !(arg == arg);
         };

   };


