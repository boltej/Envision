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
#include <AlgLib/AlgLib.h>
#include <randgen/Randunif.hpp>

#include <MatlabEngine.hpp>

const int HOURLY_DATA_IN_A_DAY = 24;
const float TANB1_005 = 0.05f;
const int MAX_DAYS_FOR_FLOODING = 5;



#define _EXPORT __declspec( dllexport )

enum DATA_CYCLE
   {
   DATA_DAILY = 0,
   DATA_HOURLY,
   };

//TODO: CHange these based on the input buoy file.
enum BUOY_OBSERVATION_DATA {
   TIME_STEP = 0,
   WAVE_HEIGHT_HS = 1,
   WAVE_PERIOD_Tp = 2,
   WAVE_DIRECTION_Dir = 3,
   WATER_LEVEL_WL = 4,
   WAVE_SETUP = 5,
   INFRAGRAVITY = 6,
   WAVEINDUCEDWL = 7,
   TWL = 8,
   TOTAL_BUOY_COL,
   };


//enum CH_FLAGS
//   {
//   CH_NONE = 0,
//   CH_MODEL_TWL = 1,
//   CH_MODEL_FLOODING = 2,
//   CH_MODEL_EROSION = 4,
//   CH_MODEL_BAYFLOODING = 8,
//   CH_MODEL_BUILDINGS = 16,
//   CH_MODEL_INFRASTRUCTURE = 32,
//   CH_MODEL_POLICY = 64,
//   CH_ALL = 0x1111111
//   };


enum BEACHTYPE
   {
   BchT_UNDEFINED = 0,
   BchT_SANDY_DUNE_BACKED = 1,
   BchT_SANDY_BLUFF_CLIFF_BACKED = 2,
   BchT_SANDY_RIPRAP_BACKED = 3,
   BchT_SANDY_SEAWALL_BACKED = 4,
   BchT_MIXED_SEDIMENT_DUNE_BACKED = 5,
   BchT_MIXED_SEDIMENT_BLUFF_BACKED = 6,
   BchT_SANDY_COBBLEGRAVEL_BLUFF_BACKED = 7,//
   BchT_SANDY_COBBLEGRAVEL_BERM_BACKED = 8,
   BchT_SANDY_WOODY_DEBRIS_BACKED = 9,
   BchT_BAY = 10,
   BchT_RIVER = 11,
   BchT_ROCKY_CLIFF_HEADLAND = 12,
   BchT_SANDY_BURIED_RIPRAP_BACKED = 13,
   BchT_JETTY = 14
   };

bool IsHardened(BEACHTYPE beachType)
   {
   switch (beachType)
      {
      case BchT_SANDY_BURIED_RIPRAP_BACKED:
      case BchT_SANDY_RIPRAP_BACKED:
      case BchT_SANDY_SEAWALL_BACKED:
         return true;
      }

   return false;
   }


enum FLOODHAZARDZONE
   {
   FHZ_OPENWATER = -1,
   FHZ_ONEHUNDREDYR = 1,         // Flood Zones : A, AE, AH, AH, AO, VE, FW
   FHZ_FIVEHUNDREDYR = 2,         // Flood Zones: X, X500, X PROTECTED BY LEVEE
   FHZ_UNSTUDIED = 3               //
   };

enum TRANSECTDAILYDATA
   {
   HEIGHT_L, HEIGHT_H,
   PERIOD_L, PERIOD_H,
   DIRECTION_L, DIRECTION_H,
   DEPTH_L, DEPTH_H,
   X, Y
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
      float m_floodTimestep;
   };

class PolicyInfo
   {
   public:
      CString m_label;
      int m_index;  // corresponds to POLICY_ID below
      bool m_hasBudget;
      float m_startingBudget;     // budget at the beginning of the year (set in Run()) ($)
      float m_availableBudget;    // currently available budget (updated during Run()) ($)
      float m_spentBudget;      // annual expenditure
      float m_unmetDemand;        // current unmet demand (for this year, updated during Run()) ($)
      float m_allocFrac;           // portion of budget from SpentBudget fraction
      bool m_usingMinBudget;

      MovingWindow m_movAvgBudgetSpent;
      MovingWindow m_movAvgUnmetDemand;

      static float m_budgetAllocFrac;

   protected:
      int* m_isActive;

   public:
      PolicyInfo::PolicyInfo() :
         m_index(-1),
         m_isActive(nullptr),
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

      void Init(LPCTSTR label, int index, int* isActive, bool hasBudget)
         {
         m_label = label;  m_index = index; m_isActive = isActive; m_hasBudget = hasBudget;
         }

      bool IsActive() { return (m_isActive != nullptr && *m_isActive != 0); }
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
   PI_NOURISH_BPS,
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
      bool Init(EnvContext* pEnvContext, LPCTSTR initStr);
      bool InitRun(EnvContext* pEnvContext, bool useInitSeed);
      bool Run(EnvContext* pContext);
      bool EndRun(EnvContext* pEnvContext);



      // int ChronicHazards::ResizeAndCalculateBeta(long &rows, long &cols);


      ////  CString m_polyGridFName;
      //PolyGridLookups   *m_pPolyGridLkUp;          // store Poly-Grid lookup info for mapping grids to IDUs
      //FAGrid            *m_pGridMapLayer;
      //MapLayer *m_pPolyMapLayer;                        // MapLayer object, to hold values of IDU.shp.
      //Map *m_pMap;                                    // Object to handle map pointer required to create grids.
      ////   bool LoadPolyGridLookup(MapLayer *pIDULayer);
      //bool LoadPolyGridLookup(EnvContext *pEnvContext);
   protected:
      bool LoadRBFs();
      bool ResetAnnualVariables();
      //bool ResetAnnualBudget();
      //bool CalculateErosionExtents(EnvContext *pEnvContext);
      //bool RunEelgrassModel(EnvContext *pEnvContext);
      //bool GenerateFloodMap();
      bool AddToPolicySummary(int year, int policy, int loc, float unitCost, float cost, float availBudget, float param1, float param2, bool passCostConstraint);

      int ConvertHourlyBuoyDataToDaily(LPCTSTR fullPath);

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
      int SetInfrastructureValues();
      double KDmodel(int point);
      double Bruunmodel(int point, int yearOfRun);
      double SCRmodel(int point);

      // bool LoadPolyGridLookup();
      //  bool LoadPolyGridLookup(MapLayer *pGridLayer, MapLayer *pPolyLayer, CString filename, PolyGridLookups *&pLookupLayer);

   protected:
      //int m_runFlags;
      int m_runTWL = 1;
      int m_runFlooding = 1;
      int m_runErosion = 1;
      int m_runBuildings = 1;
      int m_runInfrastructure = 0;
      int m_runPolicy = 1;
      Map* m_pMap;                                       // ptr to Map required to create grids

      CUIntArray m_shuffleArray;
      RandUniform* m_pRandUniform = nullptr;

      // input layers used by submodels
      MapLayer* m_pIDULayer = nullptr;                     // ptr to IDU Layer
      MapLayer* m_pDuneLayer = nullptr;
      MapLayer* m_pRoadLayer = nullptr;                  // ptr to Road Layer
      MapLayer* m_pBldgLayer = nullptr;                  // ptr to Building Layer
      MapLayer* m_pInfraLayer = nullptr;                 // ptr to Infrastructure Building Layer

      //MapLayer *m_pElevationGrid = nullptr;              // ptr to Minimum Elevation Grid
      //MapLayer *m_pBayBathyGrid = nullptr;               // ptr to Bay Bathymetry Grid
      //MapLayer *m_pBayTraceLayer = nullptr;
      //MapLayer *m_pManningMaskGrid = nullptr;            // ptr to Bates Model Mannings Coefficient Grid (value = -1 if not running Bates model in region)
      //MapLayer *m_pTidalBathyGrid = nullptr;             // ptr to Tidal Bathymetry Grid


      // Created grids for mapping flooding, erosion
      MapLayer* m_pFloodedGrid = nullptr;                   // ptr to Flooded Grid
      MapLayer* m_pCumFloodedGrid = nullptr;                // ptr to Flooded Grid
      MapLayer* m_pErodedGrid = nullptr;                    // ptr to Eroded Grid

      //MapLayer* m_pEelgrassGrid = nullptr;                  // ptr to Eel Grass Grid


      //PolyGridMapper *m_pPolygonGridLkUp = nullptr;            // provides mapping between IDU layer and DEM grid
      PolyGridMapper* m_pRoadErodedGridLkUp = nullptr;         // provides mapping between roads layer and eroded grid
      PolyGridMapper* m_pIduFloodedGridLkUp = nullptr;         // provides mapping between IDU layer and flooded grid
      PolyGridMapper* m_pRoadFloodedGridLkUp = nullptr;         // provides mapping between roads layer and flooded grid
      //PolyPointMapper* m_pBldgFloodedGridLkUp = nullptr;      // provides mapping between bldg layer and flooded grid
      //PolyPointMapper* m_pInfraFloodedGridLkUp = nullptr;      // provides mapping between bldg layer and flooded grid
      PolyGridMapper* m_pIduErodedGridLkUp = nullptr;
      PolyPointMapper* m_pIduBuildingLkUp = nullptr;         // provides mapping between IDU layer, building (point) layer
      PolyPointMapper* m_pIduInfraLkUp = nullptr;   // provides mapping between IDU layer, infrastructure (point) layer



      // Bates Flooding Model
      MapLayer* m_pNewElevationGrid = nullptr;
      MapLayer* m_pWaterElevationGrid = nullptr;            // ptr to Flooded Water Depth Grid
      MapLayer* m_pDischargeGrid = nullptr;                 // ptr to Volumetric Flow rate Grid

      //PtrArray<BatesFlood> m_batesFloodArray;

      PtrArray< LookupTable > m_swanLookupTableArray;
      //     PtrArray< DDataObj > m_SurfZoneDataArray;
      PtrArray< DDataObj > m_BathyDataArray;
      //PtrArray< FDataObj > m_RBFDailyDataArray;
      PtrArray< FDataObj > m_dailyRBFOutputArray;
      //PtrArray< FDataObj > m_DailyBayDataArray;

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
      //DDataObj *m_pInletLUT = nullptr;
      //DDataObj *m_pBayLUT = nullptr;
      DDataObj* m_pProfileLUT = nullptr;
      std::map<int, int> m_profileMap;      // key=ID, value=index


      //DDataObj *m_pSAngleLUT = nullptr;
      //DDataObj *m_pTransectLUT = nullptr;
      MapLayer* m_pTransectLayer = nullptr;
      std::map<int, int> m_transectMap;      // key=ID, value=index



      // Policy allocations/expenditures
      CArray<PolicyInfo, PolicyInfo&> m_policyInfoArray;
      //vector<double> m_prevExpenditures;
      //FDataObj *m_pResidualArray;
      FDataObj m_policyCostData;

      // Lookup table for yearly Sea Level Rise data
      DDataObj m_slrData;

      // Deep water simulation Daily Time Series
      VDataObj m_buoyObsData;
      //FDataObj m_maxTWLArray;

      /*DDataObj m_TideData;
      DDataObj m_TimeData;
      DDataObj m_SimulationData; */
      DDataObj m_reducedbuoyObsData;

      // Setup variables
      CString m_twlInputPath;
      CString m_erosionInputPath;
      CString m_bathyInputPath;

      CString m_cityDir;
      bool m_inputinHourlydata = true;


      ////// int m_debug = 0;
      ////// int m_numBayBathyPts = 0;
      ////// int m_numBayPts = 0;
      ////// int m_numBayStations = 0;


      // TWL variables
      int m_climateScenarioID = 0;                  // 0=BaseSLR = 0; 1=LowSLR = 0; 2=MedSLR = 0; 3=HighSLR
      float m_meanTWL = 0;
      int m_windowLengthTWL = 0;
      int m_numTransects = 0;
      int m_numDays = -1;
      int m_numYears = -1;
      int m_randIndex = 0;
      int m_simulationCount = 0;

      // flooding variables
      int m_usePriorGrids = 0;
      float m_floodedArea = 0;
      float m_floodedAreaSqMiles = 0;
      float m_floodedRailroadMiles = 0;
      float m_floodedRoad = 0;
      float m_floodedRoadMiles = 0;
      int m_windowLengthFloodHzrd = 5;
      REAL m_elevCellWidth = -1;                        // cell Width (m) in DEM, Flooded, Eroded grid
      REAL m_elevCellHeight = -1;                        // cell Height (m) in DEM, Flooded, Eroded grid
      float m_ManningCoeff = 0.25f;
      int m_runBatesFlood = 0;
      float m_batesFloodDuration = 6;
      ///// int m_runBayFloodModel = 0;            // Run Bay Flooding Model: 0=Off; 1=On


      // erosion variables
      float m_erodedRoad = 0;
      float m_erodedRoadMiles = 0;
      int m_windowLengthEroHzrd = 5;
      int m_numProfiles = 0;
      float m_constTD = 10.0f;

      // Dune variables
      int m_maxTransect = -1;
      int m_minTransect = 99999;
      int m_maxDuneIndex = -1;
      int m_nourishFactor = 5;
      int m_nourishFreq = 5;
      int m_nourishPercentage = 20;
      
      // Infrastructure variables
      int m_numBldgs = 0;
      int m_findSafeSiteCell = 0;
      int m_erodedBldgCount = 0;
      int m_floodedBldgCount = 0;
      int m_floodedInfraCount = 0;

      int m_floodCountBPS = 0;
      int m_floodCountSPS = 0;


      ////// int m_numDunePts = 0;

      // Policy scenario xml variables 
      int m_runConstructBPSPolicy = 0;               // 0=Off = 0; 1=On
      ////// int m_runConstructSafestPolicy = 0;               // 0=Off = 0; 1=On
      int m_runConstructSPSPolicy = 0;               // 0=Off = 0; 1=On
      int m_runMaintainBPSPolicy = 0;                  // 0=Off = 0; 1=On
      int m_runMaintainSPSPolicy = 0;                  // 0=Off = 0; 1=On
      int m_runNourishBPSPolicy = 0;                  // 0=Off = 0; 1=On
      int m_runNourishSPSPolicy = 0;                  // 0=Off = 0; 1=On
      int m_runRaiseInfrastructurePolicy = 0;            // 0=Off = 0; 1=On
      int m_runRelocateSafestPolicy = 0;               // 0=Off = 0; 1=On   
      int m_runRemoveBldgFromHazardZonePolicy = 0;         // 0=Off = 0; 1=On
      int m_runRemoveInfraFromHazardZonePolicy;         // 0=Off = 0; 1=On
      float m_totalBudget = 0;
      float m_accessThresh = 90.0f;

      int m_noConstructedBPS = 0;
      int m_noConstructedDune;
      float m_maintCostBPS = 0;
      float m_maintCostSPS = 0;
      float m_maintVolumeSPS = 0;

      float m_nourishVolumeBPS = 0;
      float m_nourishVolumeSPS = 0;
      float m_nourishCostBPS = 0;
      float m_nourishCostSPS = 0;
      float m_percentHardenedShoreline = 0;
      float m_percentRestoredShoreline = 0;
      float m_percentHardenedShorelineMiles = 0;
      float m_percentRestoredShorelineMiles = 0;
      float m_restoreCostSPS = 0;
      float m_totalHardenedShoreline = 0;
      float m_totalHardenedShorelineMiles = 0;
      float m_totalRestoredShoreline = 0;
      float m_totalRestoredShorelineMiles = 0;
      float m_addedHardenedShoreline = 0;
      float m_addedHardenedShorelineMiles = 0;
      float m_addedRestoredShoreline = 0;
      float m_addedRestoredShorelineMiles = 0;
      float m_constructCostBPS = 0;
      float m_constructVolumeSPS = 0;

      int m_maintFreqBPS = 3;
      int m_useMaxTWL = 1;
      int m_bfeCount = 0;
      int m_ssiteCount = 0;


      int m_noBldgsRemovedFromHazardZone = 0;
      int m_numCntyNourishProjects = 0;
      int m_numCtnyNourishPrjts = 0;
      float m_avgAccess = 0;

      //   xml input file variables 
      int m_exportFloodMapInterval = -1;
      int m_exportDuneMapInterval = -1;
      int m_exportBldgMapInterval = -1;
      ///// int m_runEelgrassModel = 0;            // Run Eelgrass Model: 0=Off; 1=On
      ///// int m_runSpatialBayTWL = 0;             // Run Spatially varying TWLs in Bay: 0=Off; 1=On
      int m_writeDailyBouyData = 0;





      int m_eroCount = 0;

      float m_avgBackSlope = 0.08f;
      float m_avgFrontSlope = 0.16f;
      float m_avgDuneCrest = 8.0f;
      float m_avgDuneHeel = 6.0f;
      float m_avgDuneToe = 5.0f;
      float m_avgDuneWidth = 50.0f;
      ///// float m_avgWL0 = 0;
      ///// float m_delta = 0;
      float m_minDistToProtecteBldg = 200;
      float m_idpyFactor = 0.4f;
      float m_inletFactor = 0;
      ///// float m_maxArea = 7000000;
      float m_maxBPSHeight = 8.5f;
      float m_minBPSHeight = 2.0f;
      float m_minBPSRoughness = 0.55f;
      float m_radiusOfErosion = 10.0f;
      float m_rebuildFactorSPS = 0.;
      float m_safetyFactor = 1.5f;
      float m_slopeBPS = 0.5;
      float m_slopeThresh = 0;
      float m_shorelineLength = 10;
      float m_topWidthSPS = 0;
      float m_removeFromHazardZoneRatio = 0.5f;
      float m_raiseRelocateSafestSiteRatio = 0.4f;

      // Grid properties
      //int m_numRows = -1;                           // Number of rows in DEM, Flooded, Eroded grid
      //int m_numCols = -1;                           // Number of columns in DEM, Flooded, Eroded grid
      //
      //int m_numBayRows = -1;                        // Number of rows in Bay Bathy, Eelgrass grid
      //int m_numBayCols = -1;                        // Number of columns in Bay Bathy, Eelgrass grid   
      //REAL m_bayCellWidth = 0;                     // cell Width (m) in Bay Bathy, Eelgrass grid
      //REAL m_bayCellHeight = 0;                     // cell Height (m) in Bay Bathy, Eelgrass grid
      //
      //int m_numTidalRows = -1;                        // Number of rows in Tidal Bathy grid
      //int m_numTidalCols = -1;                        // Number of columns in Tidal Bathy grid   
      //REAL m_tidalCellWidth = 0;                     // cell Width (m) in Tidal Bathy grid
      //REAL m_tidalCellHeight = 0;                     // cell Height (m) in Tidal Bathy grid
      //
      //                                          // Bates Flooding Model
      float m_floodTimestep = 0.1f;

      // Study area-wide Metrics
      ///// 

      ///// float m_eelgrassArea = 0;
      ///// float m_eelgrassAreaSqMiles = 0;
      ///// float m_intertidalArea = 0;
      ///// float m_intertidalAreaSqMiles = 0;


      // output data objs
      //FDataObj *m_pTWLData




      // IDU coverage columns
      int m_colArea = -1;


      //--- infrastructure model columns --//
      //int m_colIDUZone = -1;
      //int m_colIDUPopCap = -1;
      int m_colIDUPopDensity = -1;
      int m_colIDUNDU = -1;
      int m_colIDUNEWDU = -1;
      int m_colIDUMaxElevation = -1;
      //int m_colIDUMinElevation = -1;
      //int m_colIDUSafeLoc = -1;
      int m_colIDUSafeSite = -1;
      //int m_colIDUSafeSiteYear = -1;
      int m_colIDUSafestSiteCol = -1;
      int m_colIDUSafestSiteRow = -1;

      //int m_colIDUPopulation = -1;

      int m_colBldgBFE = -1;
      int m_colBldgBFEYr = -1;
      //int m_colBldgCityCode = -1;
      int m_colBldgEroded = -1;
      int m_colBldgEroFreq = -1;
      int m_colBldgFloodFreq = -1;
      int m_colBldgFlooded = -1;
      //int m_colBldgFloodYr = -1;
      int m_colBldgIDUIndex = -1;
      int m_colBldgNDU = -1;
      int m_colBldgNEWDU = -1;
      int m_colBldgRemoveYr = -1;
      int m_colBldgSafeSite = -1;
      int m_colBldgSafeSiteYr = -1;
      //int m_colBldgTsunamiHZ = -1;
      int m_colBldgValue = -1;
      int m_colBldgResAge = -1;
      //int m_colNumBldgs = -1;



      //-- dune model columns --//
      int  m_colDLBeachType = -1;





      int m_colIDUBaseFloodElevation = -1;
      //int m_colIDUBeachfront = -1;
      //int m_colIDUCPolicy = -1;
      //int m_colIDUEasementYear = -1;
      //int m_colIDUExistDU = -1;
      //int m_colIDUFID = -1;
      //int m_colIDUFloodFreq = -1;
      int m_colIDUFracFlooded = -1;
      int m_colIDUAddBPSYr = -1;
      //int m_colIDUCityCode = -1;
      int m_colIDUEroded = -1;
      //int m_colIDUFlooded = -1;
      //int m_colIDUFloodZone = -1;
      int m_colIDUFloodZoneCode = -1;
      //int m_colIDUImpValue = -1;
      //int m_colIDULandValue = -1;
      int m_colIDULength = -1;
      int m_colIDUNorthingBottom = -1;
      int m_colIDUNorthingTop = -1;
      //int m_colIDUNumStruct = -1;

      //int m_colIDUPropEroded = -1;
      int m_colIDURemoveBldgYr = -1;
      //int m_colIDURemoveSC = -1;
      //int m_colIDUTaxlotID = -1;
      int m_colIDUTsunamiHazardZone = -1;
      //int m_colIDUType = -1;
      //int m_colIDUValue = -1;
      int m_colIDUPropInd = -1;
      int m_colIDUImprValue = -1;

      // Dune Line coverage columns      
      //int  m_colDLA = -1;
      int  m_colDLAddYearBPS = -1;
      int  m_colDLAddYearSPS = -1;
      //int  m_colDLAlpha = -1;
      //int  m_colDLAlpha2 = -1;
      int  m_colDLAmtFlooded = -1;
      int  m_colDLAvgKD = -1;
      //int  m_colDLBackshoreElev = -1;
      int  m_colDLBackSlope = -1;
      int  m_colDLBchAccess = -1;
      int  m_colDLBchAccessFall = -1;
      int  m_colDLBchAccessSpring = -1;
      int  m_colDLBchAccessSummer = -1;
      int  m_colDLBchAccessWinter = -1;
      //
      int  m_colDLBeachWidth = -1;
      int  m_colDLConstructVolumeSPS = -1;
      int  m_colDLFrontSlope = -1;
      int  m_colDLLengthBPS = -1;
      int  m_colDLLengthSPS = -1;
      int  m_colDLRemoveYearBPS = -1;
      //int  m_colDLCalDate = -1;
      //int  m_colDLCalDCrest = -1;
      //int  m_colDLCalDToe = -1;
      //int  m_colDLCalEroExt = -1;
      int  m_colDLCostBPS = -1;
      int  m_colDLCostMaintBPS = -1;
      int  m_colDLCostSPS = -1;
      //int  m_colDLCPolicyL = -1;
      int  m_colDLDuneBldgDist = -1;
      int  m_colDLDuneBldgIndex = -1;
      int  m_colDLDuneCrest = -1;
      int  m_colDLDuneEroFreq = -1;
      int  m_colDLDuneFloodFreq = -1;
      int  m_colDLDuneHeel = -1;
      int  m_colDLDuneIndex = -1;
      int  m_colDLDuneToe = -1;
      int  m_colDLDuneToeSPS = -1;
      int  m_colDLDuneWidth = -1;
      int  m_colDLEastingCrest = -1;
      //int  m_colDLEastingCrestProfile = -1;
      //int  m_colDLEastingCrestSPS = -1;
      int  m_colDLEastingShoreline = -1;
      int  m_colDLEastingToe = -1;
      int  m_colDLEastingToeBPS = -1;
      int  m_colDLEastingToeSPS = -1;
      int  m_colDLEastingHeel = -1;
      int  m_colDLEastingMHW = -1;
      int m_colDLErosion = -1;      // erosion rates
      int m_colDLEkd = -1;
      int m_colDLEhist = -1;
      int m_colDLEbruun = -1;

      int  m_colDLFlooded = -1;
      int  m_colDLForeshore = -1;
      //int  m_colDLGammaBerm = -1;
      int  m_colDLGammaRough = -1;
      //int  m_colDLHb = -1;
      int  m_colDLHdeep = -1;
      int  m_colDLHeelWidthSPS = -1;
      int  m_colDLHeightBPS = -1;
      int  m_colDLHeightSPS = -1;
      int  m_colDLIDDCrest = -1;
      int  m_colDLIDDCrestFall = -1;
      int  m_colDLIDDCrestSpring = -1;
      int  m_colDLIDDCrestSummer = -1;
      int  m_colDLIDDCrestWinter = -1;
      int  m_colDLIDDToe = -1;
      int  m_colDLIDDToeFall = -1;
      int  m_colDLIDDToeSpring = -1;
      int  m_colDLIDDToeSummer = -1;
      int  m_colDLIDDToeWinter = -1;
      int  m_colDLKD = -1;
      int  m_colDLLatitudeToe = -1;
      int  m_colDLLittCell = -1;
      int  m_colDLLongitudeToe = -1;
      int  m_colDLMaintVolumeSPS = -1;
      int  m_colDLMaintYearBPS = -1;
      int  m_colDLMaintYearSPS = -1;
      int  m_colDLMvAvgIDPY = -1;
      int  m_colDLMvAvgTWL = -1;
      int  m_colDLMvMaxTWL = -1;
      //int  m_colDLNorthingCrest = -1;
      int  m_colDLNorthing = -1;
      int  m_colDLNourishFreqBPS = -1;
      int  m_colDLNourishFreqSPS = -1;
      int  m_colDLNourishYearBPS = -1;
      int  m_colDLNourishYearSPS = -1;
      int  m_colDLNourishVolBPS = -1;
      int  m_colDLNourishVolSPS = -1;
      //int  m_colDLOrigBeachType = -1;
      //int  m_colDLPrevCol = -1;
      //int  m_colDLPrevDuneCr = -1;
      //int  m_colDLPrevRow = -1;
      //int  m_colDLPrevSlope = -1;
      int  m_colDLProfileID = -1;
      //int  m_colDLScomp = -1;
      int  m_colDLShoreface = -1;
      int  m_colDLShorelineAngle = -1;
      int  m_colDLShorelineChange = -1;
      int  m_colDLSCR = -1;
      int  m_colDLDeltaBruun = -1;
      int m_colDLBruunSlope = -1;
      //int  m_colDLTD = -1;
      //int  m_colDLThreshDate = -1;
      int  m_colDLTopWidthSPS = -1;
      int  m_colDLTransID = -1;
      int  m_colDLTranSlope = -1;
      //int  m_colDLTS = -1;
      //int  m_colDLWb = -1;
      int  m_colDLWDirection = -1;
      int  m_colDLWidthBPS = -1;
      int  m_colDLWidthSPS = -1;
      int  m_colDLWHeight = -1;
      int  m_colDLWPeriod = -1;
      int  m_colDLXAvgKD = -1;
      int  m_colDLYrAvgLowSWL = -1;
      int  m_colDLYrAvgSWL = -1;
      int  m_colDLYrAvgTWL = -1;
      int  m_colDLYrMaxDoy = -1;
      int  m_colDLYrMaxIGSwash = -1;
      int  m_colDLYrMaxIncSwash = -1;
      int  m_colDLYrMaxSetup = -1;
      int  m_colDLYrMaxSwash = -1;
      int  m_colDLYrMaxSWL = -1;
      int  m_colDLYrMaxRunup = -1;
      int  m_colDLYrFMaxTWL = -1;     // flooding max?
      int  m_colDLYrMaxTWL = -1;
      //
      //// columns used for debugging in the dune line
      //int  m_colDLBWidth = -1;
      //int  m_colDLComputedSlope = -1;
      //int  m_colDLComputedSlope2 = -1;
      //int  m_colDLDeltaX = -1;
      //int  m_colDLDuneBldgEast = -1;
      //int  m_colDLDuneBldgIndex2 = -1;
      //int  m_colDLNewEasting = -1;
      //int  m_colDLNumIDPY = -1;
      //int  m_colDLRunupFlag = -1;
      //int  m_colDLSTKRunup = -1;
      //int  m_colDLSTKR2 = -1;
      //int  m_colDLSTKTWL = -1;
      //int  m_colDLStructR2 = -1;
      //int  m_colDLTAWSlope = -1;
      int  m_colDLTypeChange = -1;
      //
      //// Bay Trace coverage columns
      //int  m_colBayTraceIndex = -1;
      //int  m_colBayYrAvgSWL = -1;
      //int  m_colBayYrAvgTWL = -1;
      //int  m_colBayYrMaxSWL = -1;
      //int  m_colBayYrMaxTWL = -1;
      //c
      //// Road coverage columns
      int m_colRoadFlooded = -1;
      //int m_colRoadType = -1;
      //int m_colRoadLength = -1;
      //
      //// Buildings coverage columns      
      //
      //// Infrastructure coverage columns
      int m_colInfraBFE = -1;
      int m_colInfraBFEYr = -1;
      //int m_colInfraCount = -1;
      int m_colInfraCritical = -1;
      int m_colInfraDuneIndex = -1;
      int m_colInfraEroded = -1;
      int m_colInfraFlooded = -1;
      int m_colInfraFloodYr = -1;
      int m_colInfraValue = -1;

      CString  m_polyGridFilename;
      CString  m_roadGridFilename;

      CString m_transectLayer;
      CString m_bathyLookupFile;
      CString m_bathyMask;
      //   CString m_surfZoneFiles;
      CString m_climateScenarioStr;

      //   CString  msg;                           // String for error handling. 
      //   CString failureID;                        // String for error handling.

      static TWLDATAFILE m_datafile;
      static COSTS m_costs;

      // flooding model
      //PtrArray<FloodArea> m_floodAreaArray;
      //bool InitializeFloodGrids(EnvContext *pContext, FloodArea *pFloodArea);
      //bool RunFloodAreas(EnvContext *pContext);

      std::unique_ptr<matlab::engine::MATLABEngine> m_matlabPtr = nullptr; // startMATLAB();

      CString m_sfincsHome;  // e.g.  "d:/Envision/Source/Plugins/ChronicHazards/FloodModel"
      CString m_floodDir;


      //  ******  Methods ******

      // Initialization
      bool LoadXml(LPCTSTR filename);
      bool InitTWLModel(EnvContext*);
      bool InitDuneModel(EnvContext*);
      bool InitBldgModel(EnvContext*);
      bool InitInfrastructureModel(EnvContext*);
      bool InitFloodingModel(EnvContext*);
      bool InitErosionModel(EnvContext*);
      bool InitPolicyInfo(EnvContext*);

      // run
      bool RunTWLModel(EnvContext*);
      bool RunErosionModel(EnvContext*);
      bool RunFloodingModel(EnvContext*);
      bool RunEelgrassModel(EnvContext*);
      bool RunPolicyManagement(EnvContext*);


      // InitRun()
      bool LoadDailyRBFOutputs(LPCTSTR simulationPath);



   protected:
      bool CalculateFloodImpacts(EnvContext* pEnvContext);


      bool ReadDailyBayData(LPCTSTR timeSeriesFolder);

      float CalculateCelerity(float waterLevel, float wavePeriod, float& n);
      double CalculateCelerity2(float waterLevel, float wavePeriod, double& n);
      void CalculateTWLandImpactDaysAtShorePoints(EnvContext* pEnvContext);
      int MaxYearlyTWL(EnvContext* pEnvContext);


      //void CalculateYrMaxTWLAtShoreline(EnvContext* pEnvContext);



      double GenerateBathtubFloodMap(int& floodedCount);
      double GenerateBatesFloodMap(int& floodedCount);
      float GenerateErosionMap(int& erodedCount);
      double GenerateEelgrassMap(int& eelgrassCount);


      bool InitializeWaterElevationGrid();


      bool CalculateFourQs(int row, int col, double& discharge);
      int VisitNeighbors(int row, int col, float twl, float& minElevation);
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
      //   { 1.2, 0, 0, 1.5, 1.0, 0, 1.8, 1.9 },
      //   { 1.0, 2.0, 2.0, 2.0, 2.0, 2.0, 1.0, 0 },
      //   { 1.0, 1.0, 1.0, 2.0, 1.3, 0, 1.0, 0 },
      //   { 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0, 0 },
      //   { 1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 1.0, 1.0 },
      //   { 1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 2.0, 1.0 },
      //};*/

      //float m_flood[ 8 ][ 8 ] = { 0 };

      void ComputeBuildingStatistics();
      void ComputeFloodedIDUStatistics();
      void ComputeIDUStatistics();
      void TallyCostStatistics(int currentYear);
      void TallyDuneStatistics(int currentYear);
      void TallyRoadStatistics();  // obsolete
      void ComputeInfraStatistics();

      bool UpdateDuneErosionStats(int dunePt, float);





      // Helper Functions
      bool ComputeStockdon(float stillwaterLevel, float waveHeight, float wavePeriod, float waveDirection,
         float* _L0, float* _setup, float* _incidentSwash, float* _infragravitySwash, float* _r2Runup, float* _twl);

      bool PopulateClosestIndex(MapLayer* pFromPtLayer, LPCTSTR field, DataObj* pLookup);

      int GetBeachType(MapLayer::Iterator pt);
      int GetNextBldgIndex(MapLayer::Iterator pt);

      void ExportMapLayers(EnvContext* pEnvContext, int outputYear);

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
      bool CalculateImpactExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint, MapLayer::Iterator& minToePoint, MapLayer::Iterator& minDistPoint, MapLayer::Iterator& maxTWLPoint);

      // Returns true to trigger an SPS rebuild
      bool CalculateRebuildSPSExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint);

      bool CalculateExcessErosion(MapLayer::Iterator startPoint, float r, float s, bool north);

      // old version - unused
      void ConstructBPS2(int currentYear);
      int WalkSouth(MapLayer::Iterator pt, float newCrest, int currentYear, double top, double bottom);
      bool FindClosestDunePtToBldg(EnvContext* pEnvContext);
      //

      bool FindNourishExtent(MapLayer::Iterator& endPoint);
      bool FindNourishExtent(int rebuiltYear, MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint);

      bool CalculateSegmentLength(MapLayer::Iterator startPoint, MapLayer::Iterator endPoint, float& length);

      bool FindImpactExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint, bool isBPS);

      bool CalculateBPSExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint, MapLayer::Iterator& maxPoint);
      void MaintainBPS(int currentYear);
      void MaintainBPS2(int currentYear);
      void RemoveBPS(EnvContext* pEnvContext);

      void MaintainStructure(MapLayer::Iterator pt, int currentYear); // , bool isBPS);
      void MaintainSPS(int currentYear);

      bool FindProtectedBldgs();
      bool IsBldgImpactedByEErosion(int bldg, float avgEro, float distToBldg); // , float distance); //, int& iduIndex);
      //int GetIDUIndexOfBldg(int bldg);

      void NourishBPS(int currentYear, bool nourishBPS); //, int beachType);
      void NourishSPS(int currentYear); //, int beachType);
      void CompareBPSorNourishCosts(int currentYear);

      void ConstructOnSafestSite(EnvContext* pEnvContext, bool inFloodPlain);
      void RaiseOrRelocateBldgToSafestSite(EnvContext* pEnvContext);
      void RaiseInfrastructure(EnvContext* pEnvContext);
      void RemoveBldgFromHazardZone(EnvContext* pEnvContext);
      void RemoveInfraFromHazardZone(EnvContext* pEnvContext);
      void RemoveFromSafestSite();

      bool IsMissingRow(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint);

      bool SetDuneToeUTM2LL(MapLayer* pLayer, int point);
      double IGet(DDataObj* table, double x, int xcol, int ycol = 1, IMETHOD method = IM_LINEAR, int startRowIndex = 0, int* returnRowIndex = nullptr, bool forwardDirection = true);

      // Tillamook unused
      double Maximum(double* dat, int length);
      double Minimum(double* dat, int length);
      void Mminvinterp(double* x, double* y, double*& xo, int length);

      template<class T>
      inline BOOL isNan(T arg)
         {
         //nans always return false when asked if equal to themselves
         return !(arg == arg);
         };

   };


