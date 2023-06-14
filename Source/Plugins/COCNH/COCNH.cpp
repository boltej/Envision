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
// COCNH.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "COCNH.h"
#include <maplayer.h>
#include <UNITCONV.H>
#include <deltaarray.h>
#include <EnvAPI.h>
#include <EnvModel.h>
#include <misc.h>
#include <AlgLib\AlgLib.h>
#include <AlgLib\ap.h>
#include <omp.h>
#include <tinyxml.h>
#include <random>

using namespace alglib_impl;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension* Factory(EnvContext *pContext) 
   {
   switch (pContext->id)
      {
      case 1:  return (EnvExtension*) new  COCNHProcessPre1;
      case 4:  return (EnvExtension*) new COCNHProcessPre2;
      case 8:  return (EnvExtension*) new COCNHProcessPost1;
      case 16: return (EnvExtension*) new COCNHSetTSD;
      case 32: return (EnvExtension*) new COCNHProcessPost2;
      }

   return nullptr; 
   }




float GetPeoplePerDU(int);

int ComparePlanArea(const void *arg0, const void *arg1);


bool IsBetween(float x, float lower, float upper);
bool IsBetween(int x, int lower, int upper);

bool IsBetween(float x, float lower, float upper)
   {
   ASSERT(lower <= upper);
   return (lower <= x && x <= upper);
   }

bool IsBetween(int x, int lower, int upper)
   {
   ASSERT(lower <= upper);
   return (lower <= x && x <= upper);
   }


#define FailAndReturn(FailMsg) \
   {msg = failureID + (_T(FailMsg)); \
   Report::ErrorMsg(msg); \
   return false; }



///////////////////////////////////////////////////////
// VEGMAP
///////////////////////////////////////////////////////


// veg structure map - global

bool VegLookup::Init(LPCTSTR filename)
   {
   if (m_isInitialized == false)
      {
      int records = m_inputTable.ReadAscii(filename, ',', TRUE);
      if (records <= 0)
         return false;

      // get column and row counts
      int rows = m_inputTable.GetRowCount();
      int cols = m_inputTable.GetColCount();

      // column names, get column numbers based on column names
      TCHAR *colNames[VEGMAPCOLS] = { "BAA_GE_3", "TreesPha", "PUTR2_AvgCov", "JunipPha",
         "LiveBio_Mgha", "TotDeadBio_Mgha", "LiveC_Mgha", "TotDeadC_Mgha", "VPH_GE_3",
         "TotDeadVol_m3ha", "VPH_3_25", "VPH_25_50", "VPH_GE_50", "BPH_3_25","TPH_GE_50" };

      for (int i = 0; i < VEGMAPCOLS; i++)
         m_colArray[i] = m_inputTable.GetCol(colNames[i]);

      int colPVT = m_inputTable.GetCol("PVT");
      int colVegClass = m_inputTable.GetCol("VegClass");
      int colRegion = m_inputTable.GetCol("Region");

      // optimize map
      int hashSize = rows * 5 / 4;     // 20% greater than number of pairs
      while (!IsPrime(hashSize))
         hashSize++;

      InitHashTable(hashSize);

      // iterate through .csv file entries (rows)
      for (int i = 0; i < rows; i++)
         {
         int pvt = m_inputTable.GetAsInt(colPVT, i);
         int vegClass = m_inputTable.GetAsInt(colVegClass, i);
         int region = m_inputTable.GetAsInt(colRegion, i);

         VEGKEY _key((__int32)vegClass, (__int16)region, (__int16)pvt);
         __int64 key = _key.GetKey();

         SetAt(key, i);
         }

      m_isInitialized = true;
      }

   return true;
   }



///////////////////////////////////////////////////////
// FUELMODELMAP
///////////////////////////////////////////////////////


// veg structure map - global

bool FuelModelLookup::Init(LPCTSTR filename)
   {
   if (m_isInitialized == false)
      {
      int records = m_inputTable.ReadAscii(filename, ',', TRUE);
      if (records <= 0)
         return false;

      // get column and row counts
      int rows = m_inputTable.GetRowCount();
      int cols = m_inputTable.GetColCount();

      // column names, get column numbers based on column names
      //TCHAR *colNames[ FUELMODELMAPCOLS ] = { "Region", "PVT", "VegClass", "Variant", "LCP_FUEL_MODEL", "Fire_regime", "TIV" };
      //
      //for ( int i=0; i < FUELMODELMAPCOLS; i++ )
      //   m_colArray[ i ] = m_inputTable.GetCol( colNames[ i ] );

      m_lutColPVT = m_inputTable.GetCol("PVT");
      m_lutColVegClass = m_inputTable.GetCol("VegClass");
      m_lutColVariant = m_inputTable.GetCol("Variant");
      m_lutColRegion = m_inputTable.GetCol("Region");
      m_lutColTIV = m_inputTable.GetCol("TIV");
      m_lutColFuelModel = m_inputTable.GetCol("LCP_FUEL_MODEL");
      m_lutColFireRegime = m_inputTable.GetCol("Fire_regime");
      m_lutColSfSmoke = m_inputTable.GetCol("LS_avgsmoke");
      m_lutColMxSmoke = m_inputTable.GetCol("MS_avgsmoke");
      m_lutColSrSmoke = m_inputTable.GetCol("SR_avgsmoke");

      if (m_lutColPVT < 0 || m_lutColVegClass < 0 || m_lutColVariant < 0 || m_lutColRegion < 0
         || m_lutColTIV < 0 || m_lutColFuelModel < 0 || m_lutColFireRegime < 0 || m_lutColSfSmoke < 0
         || m_lutColMxSmoke < 0 || m_lutColSrSmoke < 0)
         {
         Report::ErrorMsg("FuelModelLookup::Init:  missing column in fuels lookup table");
         return FALSE;
         }

      // optimize map
      int hashSize = rows * 5 / 4;     // 20% greater than number of pairs
      while (!IsPrime(hashSize))
         hashSize++;

      InitHashTable(hashSize);

      // iterate through .csv file entries (rows)
      for (int i = 0; i < rows; i++)
         {
         int pvt = -1;
         bool ok = m_inputTable.Get(m_lutColPVT, i, pvt);
         //int pvt       = m_inputTable.GetAsInt( m_lutColPVT, i );
         int vegClass = m_inputTable.GetAsInt(m_lutColVegClass, i);
         int variant = m_inputTable.GetAsInt(m_lutColVariant, i);
         int region = m_inputTable.GetAsInt(m_lutColRegion, i);

         FUELMODELKEY _key((__int32)vegClass, (__int16)variant, (__int8)pvt, (__int8)region);
         __int64 key = _key.GetKey();

         SetAt(key, i);
         }

      m_isInitialized = true;
      }

   return true;
   }


bool COCNHProcess::m_isInitialized = false;
int COCNHProcess::m_wuiUpdateFreq = 5;
int COCNHProcess::m_initWUI = 1;
int COCNHProcess::m_colVegClass = -1;
int COCNHProcess::m_colCoverType = -1;
int COCNHProcess::m_colStructStg = -1;
int COCNHProcess::m_colCanopyCover = -1;
int COCNHProcess::m_colSize = -1;
int COCNHProcess::m_colLayers = -1;
int COCNHProcess::m_colBasalArea = -1;
int COCNHProcess::m_colPolicyID = -1;
int COCNHProcess::m_colTSD = -1;
int COCNHProcess::m_colTST = -1;
int COCNHProcess::m_colTSTH = -1;
int COCNHProcess::m_colPopDens = -1;
int COCNHProcess::m_colArea = -1;
int COCNHProcess::m_colWUI = -1;
int COCNHProcess::m_colDisturb = -1;
int COCNHProcess::m_colPdisturb = -1;
int COCNHProcess::m_colPotentialFlameLen = -1;
int COCNHProcess::m_colPVT = -1;
//int COCNHProcess::m_colPVTType = -1;
int COCNHProcess::m_colTSH = -1;
int COCNHProcess::m_colTSPH = -1;
int COCNHProcess::m_colTSF = -1;
int COCNHProcess::m_colTSPF = -1;
int COCNHProcess::m_colFuelModel = -1;
int COCNHProcess::m_colAspect = -1;
int COCNHProcess::m_colElevation = -1;
int COCNHProcess::m_colCostsPrescribedFire = -1;
int COCNHProcess::m_colCostsPrescribedFireHand = -1;
int COCNHProcess::m_colCostsMechTreatment = -1;
int COCNHProcess::m_colCostsMechTreatmentBio = -1;
int COCNHProcess::m_colTreatment = -1;
int COCNHProcess::m_colTimeInVariant = -1;
int COCNHProcess::m_colRegion = -1;
int COCNHProcess::m_colVariant = -1;
int COCNHProcess::m_colTreesPerHectare = -1;
int COCNHProcess::m_colJunipersPerHectare = -1;
int COCNHProcess::m_colBitterbrushCover = -1;
int COCNHProcess::m_colLCMgha = -1;
int COCNHProcess::m_colDCMgha = -1;
int COCNHProcess::m_colTCMgha = -1;
int COCNHProcess::m_colLVolm3ha = -1;
int COCNHProcess::m_colDVolm3ha = -1;
int COCNHProcess::m_colTVolm3ha = -1;
int COCNHProcess::m_colLBioMgha = -1;
int COCNHProcess::m_colDBioMgha = -1;
int COCNHProcess::m_colTBioMgha = -1;
int COCNHProcess::m_colVPH_3_25 = -1;
int COCNHProcess::m_colBPH_3_25 = -1;
int COCNHProcess::m_colLVol25_50 = -1;
int COCNHProcess::m_colLVolGe50 = -1;
int COCNHProcess::m_colSawTimberVolume = -1;
int COCNHProcess::m_colSawTimberHarvVolume = -1;
int COCNHProcess::m_colFireReg = -1;
int COCNHProcess::m_colThreEndSpecies = -1;
int COCNHProcess::m_colFire1000 = -1;
int COCNHProcess::m_colFire500 = -1;
int COCNHProcess::m_colSRFire1000 = -1;
int COCNHProcess::m_colMXFire1000 = -1;
int COCNHProcess::m_colPotentialFire500 =-1;
int COCNHProcess::m_colPotentialFire1000 = -1;
int COCNHProcess::m_colAvePotentialFlameLength1000 = -1;
int COCNHProcess::m_colPotentialSRFire1000 = -1;
int COCNHProcess::m_colPotentialMXFire1000 = -1;
int COCNHProcess::m_colFire2000 = -1;
int COCNHProcess::m_colFire10000 = -1;
int COCNHProcess::m_colPrescribedFire10000 = -1;
int COCNHProcess::m_colPrescribedFire2000 = -1;
int COCNHProcess::m_colCondFlameLength270 = -1;
int COCNHProcess::m_colTreesPerHect500 = -1;
int COCNHProcess::m_colStructure = -1;
//int COCNHProcess::m_colFireKill = -1;
int COCNHProcess::m_colHarvestVol = -1;
int COCNHProcess::m_colPriorVeg = -1;
int COCNHProcess::m_colPFDeadBio = -1;
int COCNHProcess::m_colPFDeadCarb = -1;
int COCNHProcess::m_colPlanAreaScore = -1;
int COCNHProcess::m_colPlanAreaFr = -1;
int COCNHProcess::m_colPlanAreaFireFr = -1;
int COCNHProcess::m_colPlanAreaScoreFire = -1;
int COCNHProcess::m_colNDU = -1;
int COCNHProcess::m_colNewDU = -1;
int COCNHProcess::m_colSmoke = -1;
int COCNHProcess::m_colPlanArea = -1;
int COCNHProcess::m_colPFSawVol = -1;
int COCNHProcess::m_colPFSawHarvestVol = -1;
int COCNHProcess::m_colOwner = -1;
int COCNHProcess::m_colAvailVolTFB = -1;
int COCNHProcess::m_colAvailVolPH = -1;
int COCNHProcess::m_colAvailVolSH = -1;
int COCNHProcess::m_colAvailVolPHH = -1;
int COCNHProcess::m_colAvailVolRH = -1;
int COCNHProcess::m_colFireWise = -1;
int COCNHProcess::m_colVegTranType = -1;

CString COCNHProcess::m_planAreaQueryStr;
float   COCNHProcess::m_minPlanAreaFrac = 0.20f;
int     COCNHProcess::m_minPlanAreaReuseTime = 10;
int     COCNHProcess::m_planAreaTreatmentWindow = 10;
Query  *COCNHProcess::m_pPAQuery = NULL;

CString COCNHProcess::m_planAreaFireQueryStr;
float   COCNHProcess::m_minPlanAreaFireFrac = 0.20f;
int     COCNHProcess::m_minPlanAreaFireReuseTime = 10;
int     COCNHProcess::m_planAreaFireTreatmentWindow = 10;
Query  *COCNHProcess::m_pPAFQuery = NULL;

float COCNHProcess::m_percentOfLivePassedOn20 = 0.9f;
float COCNHProcess::m_percentOfLivePassedOn21 = 0.5f;
float COCNHProcess::m_percentOfLivePassedOn23 = 0.1f;
float COCNHProcess::m_percentOfLivePassedOn29 = 0.9f;

float COCNHProcess::m_percentOfLivePassedOn3 =  0.5f;
float COCNHProcess::m_percentOfLivePassedOn31 = 0.5f;
float COCNHProcess::m_percentOfLivePassedOn32 = 0.1f;
float COCNHProcess::m_percentOfLivePassedOn51 = 1.0f;
float COCNHProcess::m_percentOfLivePassedOn52 = 0.5f;
float COCNHProcess::m_percentOfLivePassedOn55 = 0.8f;
float COCNHProcess::m_percentOfLivePassedOn56 = 0.5f;
float COCNHProcess::m_percentOfLivePassedOn57 = 0.25f;

// email from Eric White 02/06/2015 
float COCNHProcess::m_percentPreTransStructAvailable55 = 0.20f;
float COCNHProcess::m_percentPreTransStructAvailable3 = 0.50f;
float COCNHProcess::m_percentPreTransStructAvailable52 = 1.0f; // For Salvage, changed, now, feds 50%, all else 90%, handled else where, kept format, changed to a 1.0 or 100%
float COCNHProcess::m_percentPreTransStructAvailable57 = 0.75f;
float COCNHProcess::m_percentPreTransStructAvailable1 = 1.0f;

float COCNHProcess::m_priorBasalArea = 0.f;
float COCNHProcess::m_priorTreesPerHectare = 0.f;
float COCNHProcess::m_priorBitterbrushCover = 0.f;
float COCNHProcess::m_priorJunipersPerHa = 0.f;
float COCNHProcess::m_priorLiveBiomass = 0.f;
float COCNHProcess::m_priorDeadBiomass = 0.f;
float COCNHProcess::m_priorLiveCarbon = 0.f;
float COCNHProcess::m_priorDeadCarbon = 0.f;
float COCNHProcess::m_priorTotalCarbon = 0.f;
float COCNHProcess::m_priorLiveVolumeGe3 = 0.f;
float COCNHProcess::m_priorLiveVolume_3_25 = 0.f;
float COCNHProcess::m_priorLiveBiomass_3_25 = 0.f;
float COCNHProcess::m_priorLiveVolume_25_50 = 0.f;
float COCNHProcess::m_priorLiveVolumeGe50 = 0.f;
float COCNHProcess::m_priorDeadVolume = 0.f;
float COCNHProcess::m_priorTotalVolume = 0.f;
float COCNHProcess::m_priorSmallDiaVolume = 0.f;
float COCNHProcess::m_priorSawTimberVol = 0.f;

IDataObj COCNHProcess::m_initialLandscpAttribs(U_UNDEFINED);
bool COCNHProcess::m_restoreValuesOnInitRun = true;

CArray< int, int > COCNHProcess::m_accountedForFireThisStep;
CArray< int, int > COCNHProcess::m_accountedForNoTransThisStep;
CArray< int, int > COCNHProcess::m_accountedForSuccessionThisStep;
CArray< int, int > COCNHProcess::m_timeSinceFirewise;

RandUniform COCNHProcess::m_rn;
CArray< float, float > COCNHProcess::m_priorLiveVolumeGe3IDUArray;
CArray< float, float > COCNHProcess::m_priorLiveVolumeSawTimberIDUArray;
CArray< float, float > COCNHProcess::m_priorLiveBiomassIDUArray;
CArray< float, float > COCNHProcess::m_priorLiveCarbonIDUArray;
CArray< float, float > COCNHProcess::m_priorLiveVolume_3_25IDUArray;

CString COCNHProcess::m_vegStructurePath;
CString COCNHProcess::m_fuelModelPath;

QueryEngine *COCNHProcess::m_pQueryEngine = NULL;   // delete???

VegLookup       COCNHProcess::m_vegLookupTable;
FuelModelLookup COCNHProcess::m_fuelModelLookupTable;

PtrArray< PLAN_AREA_INFO > COCNHProcess::m_planAreaArray;
CMap< int, int, PLAN_AREA_INFO*, PLAN_AREA_INFO* > COCNHProcess::m_planAreaMap;

PtrArray< PLAN_AREA_INFO > COCNHProcess::m_planAreaFireArray;
CMap< int, int, PLAN_AREA_INFO*, PLAN_AREA_INFO* > COCNHProcess::m_planAreaFireMap;


bool COCNHProcess::Init(EnvContext *pContext, LPCTSTR initStr)
   {
   if (m_isInitialized)  // only initialize the first instance
      return TRUE;

   m_isInitialized = true;

   // load input file
   bool ok = LoadXml( initStr );
   if ( ! ok )
      return FALSE;

   // initialize queries
   ASSERT( m_pQueryEngine == NULL );
   m_pQueryEngine = new QueryEngine( (MapLayer*) pContext->pMapLayer );

   if ( m_pPAQuery == NULL && m_planAreaQueryStr.GetLength() > 0 )
      m_pPAQuery = m_pQueryEngine->ParseQuery( m_planAreaQueryStr, 0, "Plan Area Query" );

   if ( m_pPAFQuery == NULL && m_planAreaFireQueryStr.GetLength() > 0 )
      m_pPAFQuery = m_pQueryEngine->ParseQuery( m_planAreaFireQueryStr, 0, "Plan Area Fire Query" );

   // initialize PLAN_AREA_INFOs
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   int iduCount = pLayer->GetRecordCount();

   m_priorLiveVolumeGe3IDUArray.SetSize(iduCount);
   m_priorLiveVolumeSawTimberIDUArray.SetSize(iduCount);
   m_priorLiveBiomassIDUArray.SetSize(iduCount);
   m_priorLiveCarbonIDUArray.SetSize(iduCount);
   m_priorLiveVolume_3_25IDUArray.SetSize(iduCount);
   m_accountedForFireThisStep.SetSize(iduCount);
   m_accountedForNoTransThisStep.SetSize(iduCount);
   m_accountedForSuccessionThisStep.SetSize(iduCount);
   m_timeSinceFirewise.SetSize(iduCount);

   //get IDU column numbers for in/ouput variables
   bool dcn = DefineColumnNumbers(pContext);
   if (!dcn)
      FailAndReturn("DefineColumnNumbers() returned false in COCNH.cpp Init");

   CheckCol( pLayer, m_colPlanArea, "ALLOC_PLAN", TYPE_INT, CC_MUST_EXIST );

   int disturb = 0;
   int tsd = 0;
   int tsf = 0;
   int tspf = 0;
   int tst = 0;
   int tiv = 0;
   int tsth = 0;
   int tsh = 0;
   int tsph = 0;

   m_initialLandscpAttribs.SetSize(9,iduCount);

   int objSize = m_initialLandscpAttribs.SetLabel(0,"DISTURB");
   objSize      = m_initialLandscpAttribs.SetLabel(1,"TSD");
   objSize      = m_initialLandscpAttribs.SetLabel(2,"TSF");
   objSize      = m_initialLandscpAttribs.SetLabel(3,"TSPF");
   objSize      = m_initialLandscpAttribs.SetLabel(4,"TST");
   objSize      = m_initialLandscpAttribs.SetLabel(5,"TIV");
   objSize      = m_initialLandscpAttribs.SetLabel(6,"TSTH");
   objSize      = m_initialLandscpAttribs.SetLabel(7,"TSH");
   objSize      = m_initialLandscpAttribs.SetLabel(8,"TSPH");

   int colDisturb  = m_initialLandscpAttribs.GetCol("DISTURB");
   int colTSD       = m_initialLandscpAttribs.GetCol("TSD");
   int colTSF       = m_initialLandscpAttribs.GetCol("TSF");
   int colTSPF       = m_initialLandscpAttribs.GetCol("TSPF");
   int colTST       = m_initialLandscpAttribs.GetCol("TST");
   int colTIV       = m_initialLandscpAttribs.GetCol("TIV");
   int colTSTH       = m_initialLandscpAttribs.GetCol("TSTH");
   int colTSH       = m_initialLandscpAttribs.GetCol("TSH");
   int colTSPH       = m_initialLandscpAttribs.GetCol("TSPH");
                                                
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      pLayer->GetData(idu, m_colDisturb, disturb);
      pLayer->GetData(idu, m_colTSD, tsd );
      pLayer->GetData(idu, m_colTSF, tsf );
      pLayer->GetData(idu, m_colTSPF, tspf);
      pLayer->GetData(idu, m_colTST, tst );
      pLayer->GetData(idu, m_colTimeInVariant, tiv );
      pLayer->GetData(idu, m_colTSTH, tsth);
      pLayer->GetData(idu, m_colTSH, tsh );
      pLayer->GetData(idu, m_colTSPH, tsph);

      //store disturb and "time since" attributes to use in INITRUN to reset for replicate runs
      m_initialLandscpAttribs.Set(colDisturb, idu, disturb);
      m_initialLandscpAttribs.Set(colTSD, idu, tsd );
      m_initialLandscpAttribs.Set(colTSF, idu, tsf );
      m_initialLandscpAttribs.Set(colTSPF, idu, tspf);
      m_initialLandscpAttribs.Set(colTST, idu, tst );
      m_initialLandscpAttribs.Set(colTIV, idu, tiv );
      m_initialLandscpAttribs.Set(colTSTH, idu, tsth);
      m_initialLandscpAttribs.Set(colTSH, idu, tsh );
      m_initialLandscpAttribs.Set(colTSPH, idu, tsph);

      //init various arrays
      m_priorLiveVolumeGe3IDUArray[idu] = 0.0f;
      m_priorLiveVolumeSawTimberIDUArray[idu] = 0.0f;
      m_priorLiveBiomassIDUArray[idu] = 0.0f;
      m_priorLiveCarbonIDUArray[idu] = 0.0f;
      m_priorLiveVolume_3_25IDUArray[idu] = 0.0f;
      m_accountedForFireThisStep[idu] = 0;
      m_accountedForNoTransThisStep[idu] = 0;
      m_accountedForSuccessionThisStep[idu] = 0;
      m_timeSinceFirewise[idu] = 5;
      int planArea = -1;
      pLayer->GetData(idu, m_colPlanArea, planArea);

      // only count areas inside a plan area
      if ( planArea > 0 )
         {
         // is there already plan area info defined for this plan area?
         PLAN_AREA_INFO *pInfo = NULL;
         BOOL found = m_planAreaMap.Lookup(planArea, pInfo);
         if ( ! found )    // none found, so create one
            {
            pInfo = new PLAN_AREA_INFO;
            pInfo->id = planArea;
            pInfo->lastUsed     = -(m_minPlanAreaReuseTime + 1);      // make sure it is eligible
            int index = (int) m_planAreaArray.Add(pInfo);
            pInfo->index = index;
            pInfo->areaArray.SetSize( m_planAreaTreatmentWindow );
            for ( int i=0; i < m_planAreaTreatmentWindow; i++ )
               pInfo->areaArray[ i ] = 0;

            m_planAreaMap.SetAt(planArea, pInfo);

            // fire
            pInfo = new PLAN_AREA_INFO;
            pInfo->id = planArea;
            pInfo->lastUsed = -(m_minPlanAreaFireReuseTime + 1);      // make sure it is eligible
            index = (int) m_planAreaFireArray.Add(pInfo);
            pInfo->index = index;
            pInfo->areaArray.SetSize( m_planAreaFireTreatmentWindow );
            for ( int i=0; i < m_planAreaFireTreatmentWindow; i++ )
               pInfo->areaArray[ i ] = 0;

            m_planAreaFireMap.SetAt(planArea, pInfo); 
            }
         }
      }
 
   // set developed classes, structures based on pop density
   Report::Log("Initializing population structure information");
   PopulateStructure(pContext, false);

   // update vegclass related variables: CoverType, StructStg, CanopyCover, Size, Layers
   Report::Log("Initializing Veg structure information");
   bool uvcv = UpdateVegClassVars(pContext, false);
   if (!uvcv)
      FailAndReturn("UpdateVegClassVars() returned false in COCNH.cpp Init");

   // load lookup tables from .csv file
   //Report::Log("initializing veg and fuel model lookup tables");
   //m_vegLookupTable.Init(m_vegStructurePath);
   //m_fuelModelLookupTable.Init(m_fuelModelPath);

   // write data from lookup table to idu table for update
   bool wlds = InitVegParamsFromTable(pContext, false);

   Report::Log("Updating Trees per hectare");
   bool tph = UpdateAvgTreesPerHectare(pContext);
   if (!tph)
      FailAndReturn("WriteLookupDataStruct() returned false in COCNH.cpp Init");

   //write data from lookup table to idu table for fuel models
   Report::Log("Updating fuel models");
   bool wldfm = UpdateFuelModel(pContext, false);
   if (!wldfm)
      FailAndReturn("WriteLookupDataStruct() returned false in COCNH.cpp Init");

   //update column potential habitat for threatened or endangered species (0/1)
   Report::Log("Updating threatend species");
   bool utes = UpdateThreatenedSpecies(pContext, false);
   if (!utes)
      FailAndReturn("UpdateThreatEndSpecies() returned false in COCNH.cpp Init");

   //update costs for different fuel treatments on idus
   Report::Log("Calculating treatment costs");
   bool ctc = CalcTreatmentCost(pContext, false);
   if (!ctc)
      FailAndReturn("CalcTreatmentCost() returned false in COCNH.cpp Init");

   //update WUI parameters
   if (this->m_initWUI > 0)
      {
      Report::Log("Populating WUI");
      bool pw = PopulateWUI(pContext, false);
      if (!pw)
         FailAndReturn("PopulateWUI() returned false in COCNH.cpp Init");
      }

   // randomize TSD (temporary) 
   Report::Log("Initializing Harvest Fields");
   pLayer->SetColData(m_colHarvestVol, VData(0), true);
   pLayer->SetColData(m_colSawTimberHarvVolume, VData(0), true);
   pLayer->SetColData(m_colPFSawHarvestVol, VData(0), true);

   AddInputVar( "Percent Of Live Passed On 20", m_percentOfLivePassedOn20, "" );
   AddInputVar( "Percent Of Live Passed On 21", m_percentOfLivePassedOn21, "" );
   AddInputVar( "Percent Of Live Passed On 23", m_percentOfLivePassedOn23, "" );
   AddInputVar( "Percent Of Live Passed On 29", m_percentOfLivePassedOn29, "" );
   AddInputVar( "Percent Of Live Passed On 3",  m_percentOfLivePassedOn3 , "" );
   AddInputVar( "Percent Of Live Passed On 31", m_percentOfLivePassedOn31, "" );
   AddInputVar( "Percent Of Live Passed On 32", m_percentOfLivePassedOn32, "" );
   AddInputVar( "Percent Of Live Passed On 51", m_percentOfLivePassedOn51, "" );
   AddInputVar( "Percent Of Live Passed On 52", m_percentOfLivePassedOn52, "" );
   AddInputVar( "Percent Of Live Passed On 55", m_percentOfLivePassedOn55, "" );
   AddInputVar( "Percent Of Live Passed On 56", m_percentOfLivePassedOn56, "" );
   AddInputVar( "Percent Of Live Passed On 57", m_percentOfLivePassedOn57, "" );
   AddInputVar( "Percent PreTransStruct Available 55", m_percentPreTransStructAvailable55, "" );
   AddInputVar( "Percent PreTransStruct Available 3",  m_percentPreTransStructAvailable3, "" );
   AddInputVar( "Percent PreTransStruct Available 52", m_percentPreTransStructAvailable52, "" );
   AddInputVar( "Percent PreTransStruct Available 57", m_percentPreTransStructAvailable57, "" );
   AddInputVar( "Percent PreTransStruct Available 1",  m_percentPreTransStructAvailable1, "" );

   return true;
   }


bool COCNHProcess::LoadXml( LPCTSTR filename )
   {
   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   bool loadSuccess = true;

   if ( ! ok )
      {
      CString msg;
      msg.Format("Error reading input file %s:  %s", filename, doc.ErrorDesc() );
      Report::ErrorMsg( msg );
      return false;
      }
   
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // <cocnh>
   if ( pXmlRoot == NULL )
      {
      CString msg;
      msg.Format("Missing <cnh> tag in input file %s", filename );
      Report::ErrorMsg( msg );
      return false;
      }

   XML_ATTR attrs[] = {
      // attr             type           address                     isReq  checkCol
      { "struct_lookup",  TYPE_CSTRING,  &m_vegStructurePath,  true,   0 },
      { "fuels_lookup",   TYPE_CSTRING,  &m_fuelModelPath,     true,   0 },
      { "init_wui",       TYPE_INT,      &m_initWUI,           false, 0 },
      { "wui_update_freq",TYPE_INT,      &m_wuiUpdateFreq,     false, 0 },
      { "restore_values_on_initrun", TYPE_BOOL, &m_restoreValuesOnInitRun, false, 0 },
      { NULL,             TYPE_NULL,     NULL,                        false,   0 } };

   ok = TiXmlGetAttributes( pXmlRoot, attrs, filename, NULL );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Misformed element reading <plan_area> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      }

   Report::Log("Initializing veg and fuel model lookup tables");
   m_vegLookupTable.Init( m_vegStructurePath );
   m_fuelModelLookupTable.Init( m_fuelModelPath );

   // <plan_area> tag
   TiXmlElement *pXmlPlanArea = pXmlRoot->FirstChildElement( "plan_area" );
   if ( pXmlPlanArea == NULL )
      {
      CString msg;
      msg.Format("Missing <plan_area> tag in input file %s", filename );
      Report::ErrorMsg( msg );
      return false;
      }

   XML_ATTR paAttrs[] = {
      // attr                 type           address                     isReq  checkCol
      { "query",              TYPE_CSTRING,  &m_planAreaQueryStr,         true,   0 },
      { "min_reuse_fraction", TYPE_FLOAT,    &m_minPlanAreaFrac,          true,   0 },
      { "treatment_window",   TYPE_INT,      &m_planAreaTreatmentWindow,  true,   0 },
      { "min_reuse_time",     TYPE_INT,      &m_minPlanAreaReuseTime,     true,   0 },
      { NULL,                 TYPE_NULL,     NULL,                        false,   0 } };

   ok = TiXmlGetAttributes( pXmlPlanArea, paAttrs, filename, NULL );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Misformed element reading <plan_area> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      }

   // <plan_area_fire> tag
   TiXmlElement *pXmlPlanAreaFire = pXmlRoot->FirstChildElement( "plan_area_fire" );
   if ( pXmlPlanAreaFire == NULL )
      {
      CString msg;
      msg.Format("Missing <plan_area_fire> tag in input file %s", filename );
      Report::ErrorMsg( msg );
      return false;
      }

   XML_ATTR pafAttrs[] = {
      // attr                 type           address                        isReq  checkCol
      { "query",              TYPE_CSTRING,  &m_planAreaFireQueryStr,         true,   0 },
      { "min_reuse_fraction", TYPE_FLOAT,    &m_minPlanAreaFireFrac,          true,   0 },
      { "treatment_window",   TYPE_INT,      &m_planAreaFireTreatmentWindow,  true,   0 },
      { "min_reuse_time",     TYPE_INT,      &m_minPlanAreaFireReuseTime,     true,   0 },
      { NULL,                 TYPE_NULL,     NULL,                            false,   0 } };

   ok = TiXmlGetAttributes( pXmlPlanAreaFire, pafAttrs, filename, NULL );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Misformed element reading <plan_area_fire> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      }

   return true;
   }


/////////////////////////////////////////////////////
/// COCNHProcessPre1
/////////////////////////////////////////////////////

bool COCNHProcessPre1::Run(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;
   // set PRIOR_VEG equal to the value of VEGCLASS at beginning of Envision time step.
   //SetPriorVegClass( pContext );
   
   pLayer->m_readOnly = false;

   int cnt = (int) m_priorLiveVolumeGe3IDUArray.GetSize();

   for (int i = 0; i < cnt; i++) 
      {
      m_priorLiveVolumeGe3IDUArray[i] = 0.0f;
      m_priorLiveVolumeSawTimberIDUArray[i] = 0.0f;
      m_priorLiveBiomassIDUArray[i] = 0.0f;
      m_priorLiveCarbonIDUArray[i] = 0.0f;
      m_priorLiveVolume_3_25IDUArray[i] = 0.0f;
      m_accountedForFireThisStep[i] = 0;
      m_accountedForNoTransThisStep[i] = 0;
      m_accountedForSuccessionThisStep[i] = 0;
      }

   // set developed vegclasses, structures based on pop density   
   Report::StatusMsg("Populating Structures");
   PopulateStructure(pContext, true);

   // update WUI categorization
   if ((pContext->yearOfRun % m_wuiUpdateFreq == 0 ) && (pContext->yearOfRun != 0))
      {
      Report::StatusMsg("Populating WUI");
      PopulateWUI(pContext, true);
      }

   // update time related values (e.g. time since harvest)
   Report::StatusMsg("Updating Times Since...");
   UpdateTimeSinceTreatment(pContext);   // TST


   UpdateTimeSinceThinning(pContext);   // TSTH

   UpdateTimeSinceHarvest(pContext);

   UpdateTimeSincePartialHarvest(pContext);

   UpdateTimeInVariant(pContext);

   UpdateTimeSinceFire(pContext);

   UpdateTimeSincePrescribedFire(pContext);

   // set DISTURB to -DISTURB
   UpdateDisturbanceValue(pContext, true);

   //reset for each time step
   
   Report::StatusMsg("Initialing IDU data");
   pLayer->SetColData(m_colPdisturb, VData(0), true);
   pLayer->SetColData(m_colPotentialFlameLen, VData(0), true);
   pLayer->SetColData(m_colFire1000, VData(0), true);
   pLayer->SetColData(m_colSRFire1000, VData(0), true);
   pLayer->SetColData(m_colMXFire1000, VData(0), true);
   pLayer->SetColData(m_colPotentialFire1000, VData(0), true);
   pLayer->SetColData(m_colAvePotentialFlameLength1000, VData(0), true);
   pLayer->SetColData(m_colPotentialSRFire1000, VData(0), true);
   pLayer->SetColData(m_colPotentialMXFire1000, VData(0), true);
   pLayer->SetColData(m_colFire2000, VData(0), true);
   pLayer->SetColData(m_colFire10000, VData(0), true);
   pLayer->SetColData(m_colPrescribedFire10000, VData(0), true);
   pLayer->SetColData(m_colPrescribedFire2000, VData(0), true);
   //pLayer->SetColData(m_colFireWise, VData(0), true);
   pLayer->SetColData(m_colVegTranType, VData(0), true);
   pLayer->SetColData(m_colFire500, VData(0), true);
   pLayer->SetColData(m_colPotentialFire500, VData(0), true);
   pLayer->SetColData(m_colHarvestVol, VData(0), true);
   pLayer->SetColData(m_colSawTimberHarvVolume, VData(0), true);
   pLayer->SetColData(m_colPFSawHarvestVol, VData(0), true);
   pLayer->SetColData(m_colSmoke, VData(0), true);
   pLayer->m_readOnly = true;

   Report::StatusMsg("");
   return TRUE;
   }


/////////////////////////////////////////////////////
/// COCNHProcessPre2
/////////////////////////////////////////////////////
bool COCNHProcessPre2::Init(EnvContext *pContext, LPCTSTR initStr)
   {
   COCNHProcess::Init(pContext, initStr);
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;
   pLayer->SetColData(m_colHarvestVol, VData(0), true);
   pLayer->SetColData(m_colSawTimberHarvVolume, VData(0), true);
   pLayer->SetColData(m_colPFSawHarvestVol, VData(0), true);
   pLayer->SetColData(m_colAvailVolTFB, VData(0), true);
   pLayer->SetColData(m_colAvailVolPH, VData(0), true);
   pLayer->SetColData(m_colAvailVolPHH, VData(0), true);
   pLayer->SetColData(m_colAvailVolRH, VData(0), true);
   //AddOutputVar( "Minimum Plan Area Reuse Time", m_minReuseTime, "" );
   return TRUE;
   }


bool COCNHProcessPre2::InitRun(EnvContext *pContext, bool useInitSeed)
   {   
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   if ( COCNHProcess::m_restoreValuesOnInitRun )
      {   
      int disturb = 0;
      int tsd = 0;
      int tsf = 0;
      int tspf = 0;
      int tst = 0;
      int tiv = 0;
      int tsth = 0;
      int tsh = 0;
      int tsph = 0;
   
      int colDisturb  = m_initialLandscpAttribs.GetCol("DISTURB");
      int colTSD      = m_initialLandscpAttribs.GetCol("TSD");
      int colTSF      = m_initialLandscpAttribs.GetCol("TSF");
      int colTSPF     = m_initialLandscpAttribs.GetCol("TSPF");
      int colTST      = m_initialLandscpAttribs.GetCol("TST");
      int colTIV      = m_initialLandscpAttribs.GetCol("TIV");
      int colTSTH     = m_initialLandscpAttribs.GetCol("TSTH");
      int colTSH      = m_initialLandscpAttribs.GetCol("TSH");
      int colTSPH     = m_initialLandscpAttribs.GetCol("TSPH");
   
      for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
         {
         //restore disturb and "time since" attributes to reset for replicate runs
         m_initialLandscpAttribs.Get(colDisturb, idu, disturb);
         m_initialLandscpAttribs.Get(colTSD, idu, tsd );
         m_initialLandscpAttribs.Get(colTSF, idu, tsf );
         m_initialLandscpAttribs.Get(colTSPF, idu, tspf);
         m_initialLandscpAttribs.Get(colTST, idu, tst );
         m_initialLandscpAttribs.Get(colTIV, idu, tiv );
         m_initialLandscpAttribs.Get(colTSTH, idu, tsth);
         m_initialLandscpAttribs.Get(colTSH, idu, tsh );
         m_initialLandscpAttribs.Get(colTSPH, idu, tsph);
   
         pLayer->SetData(idu, m_colDisturb, disturb);
         pLayer->SetData(idu, m_colTSD, tsd );
         pLayer->SetData(idu, m_colTSF, tsf );
         pLayer->SetData(idu, m_colTSPF, tspf);
         pLayer->SetData(idu, m_colTST, tst );
         pLayer->SetData(idu, m_colTimeInVariant, tiv );
         pLayer->SetData(idu, m_colTSTH, tsth);
         pLayer->SetData(idu, m_colTSH, tsh );
         pLayer->SetData(idu, m_colTSPH, tsph);
         }
      }

   // write data from lookup table to idu table for update
   bool wlds = InitVegParamsFromTable(pContext, false);

   for (int i = 0; i < (int) m_planAreaArray.GetSize(); i++)
      {
      PLAN_AREA_INFO *pInfo = m_planAreaArray[i];
      pInfo->area = 0;
      pInfo->score = 0;
      pInfo->lastUsed = -(m_minPlanAreaReuseTime+1);
      }

   for (int i = 0; i < (int) m_planAreaFireArray.GetSize(); i++)
      {
      PLAN_AREA_INFO *pInfo = m_planAreaFireArray[i];
      pInfo->area = 0;
      pInfo->score = 0;
      pInfo->lastUsed = -(m_minPlanAreaFireReuseTime+1);
      }

   clock_t start = clock();
   bool saa = this->ScoreAllocationAreas(pContext);
   if (!saa)
      FailAndReturn("ScoreAreaAllocations() returned false in COCNH.cpp InitRun");

   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("ScoreAllocationAreas =%.2f secs", (float)duration);
   Report::Log(msg);

   int cnt = (int) m_timeSinceFirewise.GetSize();

   for (int i = 0; i < cnt; i++) 
      {
      m_timeSinceFirewise[i] = 5;
      }

   pLayer->SetColData(m_colPdisturb, VData(0), true);
   pLayer->SetColData(m_colPotentialFlameLen, VData(0), true);
   pLayer->SetColData(m_colFire1000, VData(0), true);
   pLayer->SetColData(m_colSRFire1000, VData(0), true);
   pLayer->SetColData(m_colMXFire1000, VData(0), true);
   pLayer->SetColData(m_colPotentialFire1000, VData(0), true);
   pLayer->SetColData(m_colAvePotentialFlameLength1000, VData(0), true);
   pLayer->SetColData(m_colPotentialSRFire1000, VData(0), true);
   pLayer->SetColData(m_colPotentialMXFire1000, VData(0), true);
   pLayer->SetColData(m_colFire2000, VData(0), true);
   pLayer->SetColData(m_colFire10000, VData(0), true);
   pLayer->SetColData(m_colPrescribedFire10000, VData(0), true);
   pLayer->SetColData(m_colPrescribedFire2000, VData(0), true);
   pLayer->SetColData(m_colFireWise, VData(0), true);
   pLayer->SetColData(m_colVegTranType, VData(0), true);
   pLayer->SetColData(m_colFire500, VData(0), true);
   pLayer->SetColData(m_colHarvestVol, VData(0), true);
   pLayer->SetColData(m_colSawTimberHarvVolume, VData(0), true);
   pLayer->SetColData(m_colPFSawHarvestVol, VData(0), true);
   pLayer->SetColData(m_colAvailVolTFB, VData(0), true);
   pLayer->SetColData(m_colAvailVolPH, VData(0), true);
   pLayer->SetColData(m_colAvailVolPHH, VData(0), true);
   pLayer->SetColData(m_colAvailVolRH, VData(0), true);
   pLayer->SetColData(m_colSmoke, VData(0), true);
      
   return TRUE;
   }


bool COCNHProcessPre2::Run(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   pLayer->m_readOnly = false;

   // Set PRIOR_VEG if VEGCLASS has changed
   //bool udpr = UpdatePriorVeg(pContext);
   //if (!udpr)
   //   FailAndReturn("UpdatePriorVeg() returned false in COCNH.cpp");

   // apply deltas generated above, since the IDU's need to be updated before the remaining code is run
   //ApplyDeltaArray();

   // write data from lookup table to idu table for update
   // note:  PRIOR_VEG will not be updated in the IDUs at this point
   clock_t start = clock();
   bool tph = UpdateAvgTreesPerHectare(pContext);
   if (!tph)
      FailAndReturn("UpdateAvgTreesPerHectare() returned false in COCNH.cpp Init");
   clock_t finish = clock();

   double duration = (float)(finish - start) / CLOCKS_PER_SEC;
   CString msg;
   msg.Format("Updating Avg Trees Per Hectare (%.1f secs)", (float)duration);
   Report::Log(msg);

   start = clock();
   bool wlds = UpdateVegParamsFromTable(pContext, false);
   if (!wlds)
      FailAndReturn("UpdateVegParamsFromTable() returned false in COCNH.cpp Init");
   finish = clock();

   UpdateAvailVolumesFromTable( pContext, false);

   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg;
   msg.Format("Updated Veg Params (%.1f secs)", (float)duration);
   Report::Log(msg);

   // updates dead bio and dead carbon, post fire saw timber
   DecayDeadResiduals(pContext, false);

   //CalcFireEffect(pContext, true);

   start = clock();
   bool uvcv = UpdateVegClassVars(pContext, true);
   if (!uvcv)
      FailAndReturn("UpdateVegClassVars() returned false in COCNH.cpp Init");

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Updated Veg Class Vars  (%.1f secs)", (float)duration);
   Report::Log(msg);

   // write data from lookup table to idu table for fuel models
   //start = clock();
   bool wldfm = UpdateFuelModel(pContext, true);
   if (!wldfm)
      FailAndReturn("UpdateFuelModel() returned false in COCNH.cpp Init");

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Updated Fuel Model  (%.1f secs)", (float)duration);
   Report::Log(msg);

   // update costs for different fuel treatments on idus
   start = clock();
   bool ctc = CalcTreatmentCost(pContext, true);
   if (!ctc)
      FailAndReturn("CalcTreatmentCost() returned false in COCNH.cpp Init");

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Calculated Treatment Costs (%.1f secs)", (float)duration);
   Report::Log(msg);

   // update fire occurence variables for decision making
   start = clock();
   bool ufo = UpdateFireOccurrences(pContext);
   if (!ufo)
      FailAndReturn("UpdateFireOccurrences() returned false in COCNH.cpp Init");

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Updated Fire Occurance (%.1f secs)", (float)duration);
   Report::Log(msg);

   start = clock();
   bool saa = this->ScoreAllocationAreas(pContext);
   if (!saa)
      FailAndReturn("ScoreAreaAllocations() returned false in COCNH.cpp Run");

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Scored Allocation Areas  (%.1f secs)", (float)duration);
   Report::Log(msg);

   start = clock();
   bool saaf = this->ScoreAllocationAreasFire(pContext);
   if (!saaf)
      FailAndReturn("ScoreAreaAllocationsFire() returned false in COCNH.cpp Init");

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Scored Allocation Areas Fire  (%.1f secs)", (float)duration);
   Report::Log(msg);

   start = clock();
   bool cfp = this->CalculateFirewise(pContext);
   if (!cfp)
      FailAndReturn("CalculateFirewise() returned false in COCNH.cpp Init");

   finish = clock();
   duration = (float)(finish - start) / CLOCKS_PER_SEC;
   msg.Format("Calculated Firewise  (%.1f secs)", (float)duration);
   Report::Log(msg);

   pLayer->m_readOnly = true;

   return TRUE;
   }


/////////////////////////////////////////////////////
/// COCNHProcessPost1
/////////////////////////////////////////////////////

COCNHProcessPost1::COCNHProcessPost1(void)
: COCNHProcess()
, m_disturb3Ha(0)
, m_disturb29Ha(0)
, m_disturb55Ha(0)
, m_disturb51Ha(0)
, m_disturb54Ha(0)
, m_disturb52Ha(0)
, m_disturb6Ha(0)
, m_disturb1Ha(0)
   {  }


COCNHProcessPost1::~COCNHProcessPost1(void)
   { }


bool COCNHProcessPost1::Run(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   pLayer->m_readOnly = false;

   // Set PRIOR_VEG if VEGCLASS has changed
   //bool udpr = UpdatePriorVeg( pContext );
   //if (!udpr)
   //   FailAndReturn("UpdatePriorVeg() returned false in COCNH.cpp");

   // apply deltas generated above, since the IDU's need to be updated before the remaining code is run
   EnvApplyDeltaArray(pContext->pEnvModel);

   // update vegclass related variables: CoverType, StructStg, CanopyCover, Size, Layers
   bool uvcv = UpdateVegClassVars(pContext, true);
   if (!uvcv)
      FailAndReturn("UpdateVegClassVars() returned false in COCNH.cpp Init");

   // write data from lookup table to idu table for update
   bool wlds = UpdateVegParamsFromTable(pContext, false);
   if (!wlds)
      FailAndReturn("WriteLookupDataStruct() returned false in COCNH.cpp Init");

   Report::Log("Updating Trees per hectare");
   bool tph = UpdateAvgTreesPerHectare(pContext);
   if (!tph)
      FailAndReturn("WriteLookupDataStruct() returned false in COCNH.cpp COCNHProcessPost1::Run");

   // write data from lookup table to idu table for fuel models
   bool wldfm = UpdateFuelModel(pContext, true);
   if (!wldfm)
      FailAndReturn("WriteLookupDataStruct() returned false in COCNH.cpp Init");

   //update column potential habitat for threatened or endangered species (0/1)
   bool utes = UpdateThreatenedSpecies(pContext, true);
   if (!utes)
      FailAndReturn("UpdateThreatEndSpecies() returned false in COCNH.cpp Init");

   // update outputs
   m_disturb3Ha = 0;
   m_disturb29Ha = 0;
   m_disturb55Ha = 0;
   m_disturb51Ha = 0;
   m_disturb54Ha = 0;
   m_disturb52Ha = 0;
   m_disturb6Ha = 0;
   m_disturb1Ha = 0;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = 0;

      if (pLayer->GetData(idu, m_colDisturb, disturb) && disturb > 0)
         {
         float area = 0;
         pLayer->GetData(idu, m_colArea, area);

         switch (disturb)
            {
            case 3:    m_disturb3Ha += area;   break;
            case 29:   m_disturb29Ha += area;  break;
            case 55:   m_disturb55Ha += area;  break;
            case 51:   m_disturb51Ha += area;  break;
            case 54:   m_disturb54Ha += area;  break;
            case 52:   m_disturb52Ha += area;  break;
            case 6:    m_disturb6Ha += area;   break;
            case 1:    m_disturb1Ha += area;   break;
            }
         }
      }

   m_disturb3Ha /= M2_PER_HA;
   m_disturb29Ha /= M2_PER_HA;
   m_disturb55Ha /= M2_PER_HA;
   m_disturb51Ha /= M2_PER_HA;
   m_disturb54Ha /= M2_PER_HA;
   m_disturb52Ha /= M2_PER_HA;
   m_disturb6Ha /= M2_PER_HA;
   m_disturb1Ha /= M2_PER_HA;

   pLayer->m_readOnly = true;

   return TRUE;
   }


bool COCNHProcessPost1::Init(EnvContext *pContext, LPCTSTR initStr)
   {
   COCNHProcess::Init(pContext, initStr);
   return TRUE;
   }




/////////////////////////////////////////////////////
/// COCNHProcessPost2
/////////////////////////////////////////////////////


bool COCNHProcessPost2::Run(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   pLayer->m_readOnly = false;

   UpdatePlanAreas( pContext );

   // updates harvest harvestVolume = m_priorLiveVolumeGe3 - liveVolumeGe3;
   CalcHarvestBiomass(pContext, false);

   pLayer->m_readOnly = true;

   return TRUE;
   }


/////////////////////////////////////////////////////
/// COCNHProcess
/////////////////////////////////////////////////////

bool COCNHProcess::DefineColumnNumbers(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   // get number of column for the different in/outputs required
   CheckCol(pLayer, m_colVegClass, "VEGCLASS", TYPE_LONG, CC_MUST_EXIST);
   CheckCol(pLayer, m_colCoverType, "CoverType", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colStructStg, "STRUCTSTG", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colCanopyCover, "CANOPY", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colSize, "SIZE", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colLayers, "LAYERS", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colBasalArea, "BASAL_AREA", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colPolicyID, "Policy", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colTSD, "TSD", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colTST, "TST", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colTSTH, "TSTH", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colArea, "AREA", TYPE_FLOAT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colPopDens, "POPDENS", TYPE_FLOAT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colWUI, "WUI", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colDisturb, "DISTURB", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colPVT, "PVT", TYPE_INT, CC_MUST_EXIST);
   //CheckCol( pLayer, m_colPVTType,                 "PVT_TYPE",    TYPE_INT,    CC_MUST_EXIST );
   CheckCol(pLayer, m_colTSH, "TSH", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colTSPH, "TSPH", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colTSF, "TSF", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colTSPF, "TSPF", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colFuelModel, "FUELMODEL", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colAspect, "ASPECT", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colElevation, "ELEVATION", TYPE_FLOAT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colCostsPrescribedFireHand, "COSTPrFiYN", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colCostsPrescribedFire, "COSTPrFiNN", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colCostsMechTreatment, "COSTMcTreN", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colCostsMechTreatmentBio, "COSTMcTreY", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colTreatment, "TREATMENT", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colTimeInVariant, "TIV", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colRegion, "REGION", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colVariant, "VARIANT", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colTreesPerHectare, "TreesPha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colJunipersPerHectare, "JunipPha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colBitterbrushCover, "PurshiaCov", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colLCMgha, "LCMgha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colDCMgha, "DCMgha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colTCMgha, "TCMgha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colLVolm3ha, "LVolm3ha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colDVolm3ha, "DVolm3ha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colTVolm3ha, "TVolm3ha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colLBioMgha, "LBioMgha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colDBioMgha, "DBioMgha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colTBioMgha, "TBioMgha", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colVPH_3_25, "VPH_3_25", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colBPH_3_25, "BPH_3_25", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colLVol25_50, "LVol25_50", TYPE_FLOAT, CC_AUTOADD);  // formerly "VPH_25_50"
   CheckCol(pLayer, m_colLVolGe50, "LVolGe50", TYPE_FLOAT, CC_AUTOADD);  // formerly "VPH_GE_50
   CheckCol(pLayer, m_colSawTimberVolume, "SAWTIMBVOL", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colSawTimberHarvVolume, "HARVMRCHVO", TYPE_FLOAT, CC_AUTOADD); //Merchanable timber = saw timber > 25cm
   CheckCol(pLayer, m_colFireReg, "FIRE_REGIM", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colThreEndSpecies, "TESPECIES", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colFire500, "FIRE_500M", TYPE_LONG, CC_AUTOADD); 
   CheckCol(pLayer, m_colFire1000, "FIRE_1KM", TYPE_LONG, CC_AUTOADD);  // formerly FIRE5_1000",
   CheckCol(pLayer, m_colSRFire1000, "FIRESR_1KM", TYPE_LONG, CC_AUTOADD);  // formerly FIRESR_1000"
   CheckCol(pLayer, m_colMXFire1000, "FIREMX_1KM", TYPE_LONG, CC_AUTOADD);  // formerly FIREMX_1000"
   CheckCol(pLayer, m_colFire2000, "FIRE_2KM", TYPE_LONG, CC_AUTOADD);  // formerly FIRE5_2000",
   CheckCol(pLayer, m_colFire10000, "FIRE_10KM", TYPE_LONG, CC_AUTOADD);  
   CheckCol(pLayer, m_colPrescribedFire10000, "PSFIRE_10K", TYPE_LONG, CC_AUTOADD); // prescribed fire within 10km, within 5 years"
   CheckCol(pLayer, m_colPrescribedFire2000, "PRFIRE_2KM", TYPE_LONG, CC_AUTOADD);  // formerly PREF5_2000",
   CheckCol(pLayer, m_colCondFlameLength270, "CFL_270", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colTreesPerHect500, "TPH_500", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colStructure, "STRUCTURE", TYPE_INT, CC_AUTOADD);
   //CheckCol(pLayer, m_colFireKill, "FIREKILL", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colHarvestVol, "HARVESTVOL", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colPriorVeg, "PRIOR_VEG", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colPFDeadBio, "PFDEADBIO", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colPFDeadCarb, "PFDEADCARB", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colPlanAreaScore, "PLAN_SCORE", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colPlanAreaFr, "PA_FRAC", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colPlanAreaFireFr, "PAF_FRAC", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colPlanAreaScoreFire, "PS_FIRE", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colNDU, "N_DU", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colNewDU, "NEW_DU", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colPdisturb, "PDISTURB", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colFireWise, "FIREWISE", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colPotentialFlameLen, "PFlameLen", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colPotentialFire500, "PFIRE_500M", TYPE_LONG, CC_AUTOADD); 
   CheckCol(pLayer, m_colPotentialFire1000, "PFIRE_1KM", TYPE_LONG, CC_AUTOADD);   // formerly PFIRE51000",
   CheckCol(pLayer, m_colAvePotentialFlameLength1000, "PFLEN_1KM", TYPE_LONG, CC_AUTOADD);
   CheckCol(pLayer, m_colPotentialSRFire1000, "PFIRESR_1K", TYPE_LONG, CC_AUTOADD);   // formerly PFIRESR1000"
   CheckCol(pLayer, m_colPotentialMXFire1000, "PFIREMX_1K", TYPE_LONG, CC_AUTOADD);   // formerly PFIREMX1000"
   CheckCol(pLayer, m_colPFSawVol, "PFSAWVOL", TYPE_FLOAT, CC_AUTOADD); 
   CheckCol(pLayer, m_colPFSawHarvestVol, "SALHARVEST", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colSmoke, "SMOKE", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colPlanArea, "ALLOC_PLAN", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colOwner, "OWNER", TYPE_INT, CC_MUST_EXIST);
   CheckCol(pLayer, m_colVegTranType, "VEGTRNTYPE", TYPE_INT, CC_AUTOADD);
   CheckCol(pLayer, m_colAvailVolTFB, "AVLVOL_TFB", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colAvailVolPH , "AVLVOL_PHL", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colAvailVolSH , "AVLVOL_SH", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colAvailVolPHH, "AVLVOL_PHH", TYPE_FLOAT, CC_AUTOADD);
   CheckCol(pLayer, m_colAvailVolRH , "AVLVOL_RH", TYPE_FLOAT, CC_AUTOADD);
   
   pLayer->SetColData(m_colPdisturb, VData(0), true);
   pLayer->SetColData(m_colPotentialFlameLen, VData(0), true);
   pLayer->SetColData(m_colFire1000, VData(0), true);
   pLayer->SetColData(m_colSRFire1000, VData(0), true);
   pLayer->SetColData(m_colMXFire1000, VData(0), true);
   pLayer->SetColData(m_colPotentialFire1000, VData(0), true);
   pLayer->SetColData(m_colAvePotentialFlameLength1000, VData(0), true);
   pLayer->SetColData(m_colPotentialSRFire1000, VData(0), true);
   pLayer->SetColData(m_colPotentialMXFire1000, VData(0), true);
   pLayer->SetColData(m_colFire2000, VData(0), true);
   pLayer->SetColData(m_colFire10000, VData(0), true);
   pLayer->SetColData(m_colPrescribedFire10000, VData(0), true);
   pLayer->SetColData(m_colPrescribedFire2000, VData(0), true);
   pLayer->SetColData(m_colFireWise, VData(0), true);
   pLayer->SetColData(m_colFire500, VData(0), true);
   pLayer->SetColData(m_colPlanAreaFr, VData(0), true);
   pLayer->SetColData(m_colPlanAreaFireFr, VData(0), true);
   pLayer->SetColData(m_colPotentialFire500, VData(0), true);
   pLayer->SetColData(m_colPFSawVol, VData(0), true);
   pLayer->SetColData(m_colPFSawHarvestVol, VData(0), true);
   
   return true;
   }


static int updateFuelModelErrCount = 0;

bool COCNHProcess::UpdateFuelModel(EnvContext *pContext, bool useAddDelta)
   {
   bool reportErrors = true;
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   int missingCount = 0;
   int foundCount = 0;

   // take care of any variant changtes that have happened before now   
   DeltaArray *deltaArray = pContext->pDeltaArray;

   if (deltaArray != NULL)
      {
      for (INT_PTR i = pContext->firstUnseenDelta; i < deltaArray->GetSize(); i++)
         {
         DELTA &delta = ::EnvGetDelta(deltaArray, i);
         if (delta.col == m_colVariant)
            UpdateIDU(pContext, delta.cell, m_colTimeInVariant, 0, useAddDelta ? ADD_DELTA : SET_DATA );
         }

      ::EnvApplyDeltaArray(pContext->pEnvModel);
      }

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int vegClass = -999;
      int pvt = -999;
      int variant = -999;
      int region = -999;
      int disturb = -999;

      pLayer->GetData(idu, m_colVegClass, vegClass);
      pLayer->GetData(idu, m_colPVT, pvt);
      pLayer->GetData(idu, m_colVariant, variant);
      pLayer->GetData(idu, m_colRegion, region);
      pLayer->GetData(idu, m_colDisturb, disturb);
      
      FUELMODELKEY key(vegClass, (__int16)variant, (__int8)pvt, (__int8)region);
      int luRow = -1;
      BOOL found = m_fuelModelLookupTable.Lookup(key.GetKey(), luRow);
      // does this fuel model match the pvt, vegclass and variant?

      int lcpFuelModel = 0;
      int fireRegime = 0;
      float surfaceSmoke = 0.f;
      float mixSevSmoke = 0.f;
      float standRepSmoke = 0.f;

      if (found && luRow >= 0)
         {
         foundCount++;

         int lutColFuelModel = m_fuelModelLookupTable.m_lutColFuelModel;
         int lutColFireRegime = m_fuelModelLookupTable.m_lutColFireRegime;
         int lutColTIV = m_fuelModelLookupTable.m_lutColTIV;
         int lutColSfSmoke = m_fuelModelLookupTable.m_lutColSfSmoke;
         int lutColMxSmoke = m_fuelModelLookupTable.m_lutColMxSmoke;
         int lutColSrSmoke = m_fuelModelLookupTable.m_lutColSrSmoke;

         int tiv = -1;
         pLayer->GetData(idu, m_colTimeInVariant, tiv);

         if ((variant > 1) && (tiv >= m_fuelModelLookupTable.GetColData(luRow, lutColTIV)))
            variant = 1;

         lcpFuelModel = (int)m_fuelModelLookupTable.GetColData(luRow, lutColFuelModel);
         fireRegime = (int)m_fuelModelLookupTable.GetColData(luRow, lutColFireRegime);
         surfaceSmoke = (float)m_fuelModelLookupTable.GetColData(luRow, lutColSfSmoke);
         mixSevSmoke = (float)m_fuelModelLookupTable.GetColData(luRow, lutColMxSmoke);
         standRepSmoke = (float)m_fuelModelLookupTable.GetColData(luRow, lutColSrSmoke);
      
         // column name: SMOKE_SF
         if ( disturb == 20 )
            {
            if ( surfaceSmoke >= (int) 0 )
               {
               bool goodSmokeValue = true;
               }
            else if ( (int) surfaceSmoke == -999 && (int) mixSevSmoke > 0 ) 
               {
               surfaceSmoke = 0.9f *  mixSevSmoke;
               }
            else if ( (int) surfaceSmoke == -999 && (int) mixSevSmoke == -999 && (int) standRepSmoke > 0 )
               {
               surfaceSmoke = 0.83f * standRepSmoke;
               }
            else
               {
               surfaceSmoke = 0.0f;
               }
                
            UpdateIDU(pContext, idu, m_colSmoke, surfaceSmoke, useAddDelta ? ADD_DELTA : SET_DATA);
            }

         // column name: MXSMOKE
         if ( disturb == 21 )
            {
            if ( mixSevSmoke >= (int) 0 )
               {
               bool goodSmokeValue = true;
               }
            else if ( (int) mixSevSmoke == -999 && (int) standRepSmoke > 0 ) 
               {
               mixSevSmoke = 0.9f *  standRepSmoke;
               }
            else if ( (int) mixSevSmoke == -999 && (int) standRepSmoke == -999 && (int) surfaceSmoke > 0 )
               {
               mixSevSmoke = 1.11f * surfaceSmoke;
               }
            else
               {
               mixSevSmoke = 0.0f;
               }
            UpdateIDU(pContext, idu, m_colSmoke, mixSevSmoke, useAddDelta ? ADD_DELTA : SET_DATA);
            }

         // column name: SRSMOKE
         if ( disturb == 23 )
            {
            if ( standRepSmoke >= (int) 0 )
               {
               bool goodSmokeValue = true;
               }
            else if ( (int) standRepSmoke == -999 && (int) mixSevSmoke > 0 ) 
               {
               standRepSmoke = 1.11f *  mixSevSmoke;
               }
            else if ( (int) standRepSmoke == -999 && (int) mixSevSmoke == -999 && (int) surfaceSmoke > 0 )
               {
               standRepSmoke = 1.20f * surfaceSmoke;
               }
            else
               {
               standRepSmoke = 0.0f;
               }
            UpdateIDU(pContext, idu, m_colSmoke, standRepSmoke, useAddDelta ? ADD_DELTA : SET_DATA);
            }

         // column name: VARIANT (Note: if this is updated, also set time in variant to 0
         
         if (UpdateIDU(pContext, idu, m_colVariant, variant, useAddDelta ? ADD_DELTA : SET_DATA ))
            UpdateIDU(pContext, idu, m_colTimeInVariant, 0, useAddDelta ? ADD_DELTA : SET_DATA );
         
         // column name: FUELMODEL
         if (lcpFuelModel == 0) // pathological case
            {
            CString msg;
            msg.Format("Bad Fuel lookup encountered: Vegclass=%i, pvt=%i, variant=%i, region=%i, tiv=%i",
               vegClass, pvt, variant, region, tiv);
            Report::LogWarning(msg);
            }
         else
            {
            UpdateIDU(pContext, idu, m_colFuelModel, lcpFuelModel, useAddDelta ? ADD_DELTA : SET_DATA );
            }

         // column name: FIRE_REGIM
         
         UpdateIDU(pContext, idu, m_colFireReg, fireRegime, useAddDelta ? ADD_DELTA : SET_DATA );
         
         // Note: FUELMODEL and FIRE_REGIM also need to be written to the IDUs, since they are read later in this process
         //bool readOnly = pLayer->m_readOnly;
         //
         //pLayer->SetData( idu, m_colFuelModel, lcpFuelModel );
         //pLayer->SetData( idu, m_colFireReg, fireRegime );
         //pLayer->m_readOnly = readOnly;
         }
      else  // lookup row not found - report missing row
         {
         if (reportErrors && vegClass > 2000000 && updateFuelModelErrCount < 10 )//minimum value for vegclass that have STMs
            {
            CString msg;
            msg.Format("Missing Fuel lookup encountered: Vegclass=%i, pvt=%i, variant=%i, region=%i",
               vegClass, pvt, variant, region);
            Report::LogWarning(msg);
            //reportErrors = false;
            updateFuelModelErrCount++;
            }

         missingCount++;
         }
      }

   if (useAddDelta)
      ::EnvApplyDeltaArray(pContext->pEnvModel);

   CString msg;
   msg.Format("UpdateFuelModel:  Found count=%i, missing count=%i", foundCount, missingCount);
   Report::Log(msg);

   return true;
   }


bool COCNHProcess::UpdateTimeInVariant(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   int timeInVariant = 0;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      pLayer->GetData(idu, m_colTimeInVariant, timeInVariant);
      if (timeInVariant >= 0)
         timeInVariant++;

      UpdateIDU(pContext, idu, m_colTimeInVariant, timeInVariant, ADD_DELTA);
      }

   DeltaArray *deltaArray = pContext->pDeltaArray;
   for (INT_PTR i = pContext->firstUnseenDelta; i < deltaArray->GetSize(); i++)
      {
      DELTA &delta = ::EnvGetDelta(deltaArray, i);
      if (delta.col == m_colVariant)
         UpdateIDU(pContext, delta.cell, m_colTimeInVariant, 0, ADD_DELTA);
      }

   ::EnvApplyDeltaArray(pContext->pEnvModel);

   return true;
   }



/* Not required, koch 02/2013
bool COCNHProcess::LoadLookupTableTreatCosts( EnvContext *pContext, LPCSTR initStr )
{
TRACE(_T("Starting COCNH.cpp LoadLookupTableTreatCosts"));

// get entries from csv file
int records = m_costInputtable.ReadAscii( initStr, ',', TRUE );
if ( records <= 0 )
FailAndReturn("Cannot load .csv file specified by initInfo in .envx file.");

int rows = m_costInputtable.GetRowCount();
int cols = m_costInputtable.GetColCount();
//iterate through .csv file entries (rows)
for( int i = 0; i < rows; i++)
{
// one of the entries holds a float --> cols -1
vector< int > lookup( (cols-1), -1 );
// get values as int from csv input table
int iduID      = m_costInputtable.GetAsInt( 0, i );
int teSPP      = m_costInputtable.GetAsInt( 1, i );
int fireReg    = m_costInputtable.GetAsInt( 2, i );

for ( int j = 0; j < (cols-1); j++ )
lookup.at(j) = m_costInputtable.GetAsInt( (j), i );

m_mapLookupTreatCostsInt.insert(make_pair( iduID, lookup ) );

float fuelLoad = m_costInputtable.GetAsFloat( 3, i );
float fuelLoad = m_costInputtable.GetAsFloat( 3, i );
m_mapLookupTreatCostsFloat.insert(make_pair( iduID, fuelLoad ) );
}

TRACE(_T("Completed COCNH.cpp LoadLookupTableTreatCosts successfully"));
return true;
}
*/



// basalArea, treesPerHectare, bitterbrushCover, junipersPerHa, liveBiomass, deadBiomass,
// liveCarbonStock, deadCarbonStock, totalCarbonStock, liveVolume, deadVolume, totalVolume,
// smallDiaVolume, sawTimberVol

static int updateVegParamsErrCount = 0;

bool COCNHProcess::UpdateVegParamsFromTable(EnvContext *pContext, bool useAddDelta)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   //   int iCPU = omp_get_num_procs();
   //   omp_set_num_threads(iCPU);

   //#pragma omp parallel for   
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int transType = 0;

      pLayer->GetData( idu, m_colVegTranType, transType );
      
      // only continue if there is ither a successional transition or a disturbance transition
      if ( transType == 0 )
         continue;
            
      int vegClass = -999;
      int pvt = -999;
      int region = -999;
      int disturb = 0;
      float iduAreaM2 = 0.0;
      
      
      pLayer->GetData(idu, m_colVegClass, vegClass);
      pLayer->GetData(idu, m_colPVT, pvt);
      pLayer->GetData(idu, m_colRegion, region);
      pLayer->GetData(idu, m_colDisturb, disturb);
      pLayer->GetData(idu, m_colArea, iduAreaM2);
      float iduAreaHa = iduAreaM2 * HA_PER_M2;

      VEGKEY key(vegClass, region, pvt);
      int luRow = -1;
      BOOL found = m_vegLookupTable.Lookup(key.GetKey(), luRow);

      //if ( ! found )
      //   {
      //   __int64 badKey = key.GetKey();
      //
      //   ASSERT( 0 );
      //   }

      float basalArea = 0.f;
      float treesPerHectare = 0.f;
      float bitterbrushCover = 0.f;
      float junipersPerHa = 0.f;
      float liveBiomass = 0.f;
      float deadBiomass = 0.f;
      float liveCarbon = 0.f;
      float deadCarbon = 0.f;
      float totalCarbon = 0.f;
      float liveVolumeGe3 = 0.f;
      float liveVolume_3_25 = 0.f;
      float liveBiomass_3_25 = 0.f;
      float liveVolume_25_50 = 0.f;
      float liveVolumeGe50 = 0.f;
      float deadVolume = 0.f;
      float totalVolume = 0.f;
      float smallDiaVolume = 0.f;
      float sawTimberVol = 0.f;

      pLayer->GetData(idu, m_colBasalArea,         m_priorBasalArea);
      pLayer->GetData(idu, m_colTreesPerHectare,   m_priorTreesPerHectare);
      pLayer->GetData(idu, m_colBitterbrushCover,   m_priorBitterbrushCover);
      pLayer->GetData(idu, m_colJunipersPerHectare,m_priorJunipersPerHa);
      pLayer->GetData(idu, m_colLBioMgha,            m_priorLiveBiomass);
      pLayer->GetData(idu, m_colDBioMgha,            m_priorDeadBiomass);
      pLayer->GetData(idu, m_colLCMgha,            m_priorLiveCarbon);
      pLayer->GetData(idu, m_colDCMgha,            m_priorDeadCarbon);
      pLayer->GetData(idu, m_colLVolm3ha,            m_priorLiveVolumeGe3);
      pLayer->GetData(idu, m_colVPH_3_25,            m_priorLiveVolume_3_25);
      pLayer->GetData(idu, m_colBPH_3_25,            m_priorLiveBiomass_3_25);
      pLayer->GetData(idu, m_colLVol25_50,         m_priorLiveVolume_25_50);
      pLayer->GetData(idu, m_colLVolGe50,            m_priorLiveVolumeGe50);
      pLayer->GetData(idu, m_colDVolm3ha,            m_priorDeadVolume);
      m_priorSawTimberVol = m_priorLiveVolumeGe3 - m_priorLiveVolume_3_25;
   
      pLayer->GetData(idu, m_colBasalArea, basalArea);
      pLayer->GetData(idu, m_colTreesPerHectare, treesPerHectare);
      pLayer->GetData(idu, m_colBitterbrushCover, bitterbrushCover);
      pLayer->GetData(idu, m_colJunipersPerHectare, junipersPerHa);
      pLayer->GetData(idu, m_colLBioMgha, liveBiomass);
      //pLayer->GetData(idu, m_colDBioMgha,            deadBiomass); handled in COCNHProcessPre2::DecayDeadResiduals
      //pLayer->GetData(idu, m_colDCMgha,            deadCarbon); handled in COCNHProcessPre2::DecayDeadResiduals
      //pLayer->GetData(idu, m_colDVolm3ha,            deadVolume); handled in COCNHProcessPre2::DecayDeadResiduals 
      pLayer->GetData(idu, m_colLCMgha, liveCarbon);
      pLayer->GetData(idu, m_colLVolm3ha, liveVolumeGe3);
      pLayer->GetData(idu, m_colVPH_3_25, liveVolume_3_25);
      pLayer->GetData(idu, m_colBPH_3_25, liveBiomass_3_25);
      pLayer->GetData(idu, m_colLVol25_50, liveVolume_25_50);
      pLayer->GetData(idu, m_colLVolGe50, liveVolumeGe50);

      m_priorLiveVolumeSawTimberIDUArray[idu] = m_priorSawTimberVol;
      m_priorLiveVolumeGe3IDUArray[idu] = m_priorLiveVolumeGe3;
      m_priorLiveBiomassIDUArray[idu] = m_priorLiveBiomass;
      m_priorLiveCarbonIDUArray[idu] = m_priorLiveCarbon;
      m_priorLiveVolume_3_25IDUArray[idu] = m_priorLiveVolume_3_25;

      int myidu = (int)idu;

      int seenSuccessionBefore = m_accountedForSuccessionThisStep[idu];

      // a successional transition occured in this IDU
      if ( (transType == 1 || transType == 2) && seenSuccessionBefore == 0 )
         {
         m_accountedForSuccessionThisStep[idu] = 1;         
         if (found && luRow >= 0)
            {
            basalArea = m_vegLookupTable.GetColData(luRow, BAA_GE_3);
            treesPerHectare = m_vegLookupTable.GetColData(luRow, TreesPha);
            bitterbrushCover = m_vegLookupTable.GetColData(luRow, PUTR2_AVGCOV);
            junipersPerHa = m_vegLookupTable.GetColData(luRow, JunipPha);
            liveBiomass = m_vegLookupTable.GetColData(luRow, LIVEBIO_MGHA);
            deadBiomass = m_vegLookupTable.GetColData(luRow, TOTDEADBIO_MGHA);
            liveCarbon = m_vegLookupTable.GetColData(luRow, LIVEC_MGHA);
            deadCarbon = m_vegLookupTable.GetColData(luRow, TOTDEADC_MGHA);
            liveVolumeGe3 = m_vegLookupTable.GetColData(luRow, VPH_GE_3);
            liveVolume_3_25 = m_vegLookupTable.GetColData(luRow, VPH_3_25);
            liveBiomass_3_25 = m_vegLookupTable.GetColData(luRow, BPH_3_25);
            liveVolume_25_50 = m_vegLookupTable.GetColData(luRow, VPH_25_50);
            liveVolumeGe50 = m_vegLookupTable.GetColData(luRow, VPH_GE_50);
            deadVolume = m_vegLookupTable.GetColData(luRow, TOTDEADVOL_M3HA);

            if ( basalArea        - m_priorBasalArea            <= 0.0 ) basalArea         = m_priorBasalArea;
            if ( treesPerHectare  - m_priorTreesPerHectare      <= 0.0 ) treesPerHectare   = m_priorTreesPerHectare;
            if ( bitterbrushCover - m_priorBitterbrushCover   <= 0.0 ) bitterbrushCover = m_priorBitterbrushCover;
            if ( junipersPerHa    - m_priorJunipersPerHa      <= 0.0 ) junipersPerHa      = m_priorJunipersPerHa;
            if ( liveBiomass      - m_priorLiveBiomass         <= 0.0 ) liveBiomass      = m_priorLiveBiomass;
            if ( deadBiomass      - m_priorDeadBiomass        < 0.0 ) deadBiomass      = m_priorDeadBiomass;
            if ( liveCarbon       - m_priorLiveCarbon         <= 0.0 ) liveCarbon         = m_priorLiveCarbon;
            if ( deadCarbon       - m_priorDeadCarbon         < 0.0 ) deadCarbon         = m_priorDeadCarbon;
            if ( liveVolumeGe3    - m_priorLiveVolumeGe3      <= 0.0 ) liveVolumeGe3      = m_priorLiveVolumeGe3;
            if ( liveVolume_3_25  - m_priorLiveVolume_3_25    <= 0.0 ) liveVolume_3_25   = m_priorLiveVolume_3_25;
            if ( liveBiomass_3_25 - m_priorLiveBiomass_3_25   <= 0.0 ) liveBiomass_3_25 = m_priorLiveBiomass_3_25;
            if ( liveVolume_25_50 - m_priorLiveVolume_25_50   <= 0.0 ) liveVolume_25_50 = m_priorLiveVolume_25_50;
            if ( liveVolumeGe50   - m_priorLiveVolumeGe50     <= 0.0 ) liveVolumeGe50   = m_priorLiveVolumeGe50;
            if ( deadVolume       - m_priorDeadVolume         < 0.0 ) deadVolume         = m_priorDeadVolume;

            sawTimberVol = liveVolumeGe3 - liveVolume_3_25;

            float totalBiomass = liveBiomass + deadBiomass;
            float totalCarbon = liveCarbon + liveCarbon;
            float totalVolume = liveVolumeGe3 + deadVolume;
         
            UpdateIDU(pContext, idu, m_colDVolm3ha, deadVolume, useAddDelta ? ADD_DELTA : SET_DATA );
            UpdateIDU(pContext, idu, m_colDCMgha, deadCarbon, useAddDelta ? ADD_DELTA : SET_DATA );
            UpdateIDU(pContext, idu, m_colDBioMgha, deadBiomass, useAddDelta ? ADD_DELTA : SET_DATA );
            UpdateIDU(pContext, idu, m_colTBioMgha, totalBiomass, useAddDelta ? ADD_DELTA : SET_DATA ); // live + dead
            UpdateIDU(pContext, idu, m_colTCMgha, totalCarbon, useAddDelta ? ADD_DELTA : SET_DATA );//live +dead
            UpdateIDU(pContext, idu, m_colTVolm3ha, totalVolume, useAddDelta ? ADD_DELTA : SET_DATA );
            UpdateIDU(pContext, idu, m_colSawTimberVolume, sawTimberVol, useAddDelta ? ADD_DELTA : SET_DATA );
            }
         else if ( vegClass >= 2000000 && updateVegParamsErrCount < 10 )
            {
            CString msg;
            msg.Format("UpdateVegParamsFromTable() - missing lookup for vegclass=%i, pvt=%i, region=%i", vegClass, pvt, region);
            Report::LogWarning(msg);
            updateVegParamsErrCount++;
            }
         } 
      
      int seenFireBefore = m_accountedForFireThisStep[idu];

      // if disturbance get from IDU layer and apply percentage.  This method is called in multiple instances of COCNH.
      // Only go into this condition if no wild fire prior.
      if ( transType == 3 && seenFireBefore == 0 )
         {
         //  set the fire flag 
         if ( disturb > 20 && disturb <=23 )
            m_accountedForFireThisStep[idu] = 1;

         seenFireBefore = m_accountedForFireThisStep[idu];
         
         float percentOfLivePassedOn = 1.0;

         //what is passed on 
         if (disturb == 3)   percentOfLivePassedOn = m_percentOfLivePassedOn3;  
         if (disturb == 20)  percentOfLivePassedOn = m_percentOfLivePassedOn20; 
         if (disturb == 21)  percentOfLivePassedOn = m_percentOfLivePassedOn21; 
         if (disturb == 23)  percentOfLivePassedOn = m_percentOfLivePassedOn23; // 0.1
         if (disturb == 29)  percentOfLivePassedOn = m_percentOfLivePassedOn29; 
         if (disturb == 31)  percentOfLivePassedOn = m_percentOfLivePassedOn31; 
         if (disturb == 32)  percentOfLivePassedOn = m_percentOfLivePassedOn32; 
         if (disturb == 51)  percentOfLivePassedOn = m_percentOfLivePassedOn51; 
         if (disturb == 52)  percentOfLivePassedOn = m_percentOfLivePassedOn52; 
         if (disturb == 55)  percentOfLivePassedOn = m_percentOfLivePassedOn55; 
         if (disturb == 56)  percentOfLivePassedOn = m_percentOfLivePassedOn56; 
         if (disturb == 57)  percentOfLivePassedOn = m_percentOfLivePassedOn57; 
         
         sawTimberVol = ( liveVolumeGe3 - liveVolume_3_25 ) * percentOfLivePassedOn; 
         basalArea *= percentOfLivePassedOn;
         treesPerHectare *= percentOfLivePassedOn;
         bitterbrushCover *= percentOfLivePassedOn;
         junipersPerHa *= percentOfLivePassedOn;
         liveBiomass *= percentOfLivePassedOn;
         //deadBiomass *= percentOfLivePassedOn;
         liveCarbon *= percentOfLivePassedOn;
         //deadCarbon *= percentOfLivePassedOn;         
         liveVolume_3_25 *= percentOfLivePassedOn;
         liveBiomass_3_25 *= percentOfLivePassedOn;
         liveVolume_25_50 *= percentOfLivePassedOn;
         liveVolumeGe50 *= percentOfLivePassedOn;
         //deadVolume *= percentOfLivePassedOn;

         liveVolumeGe3 =  liveVolume_3_25 + sawTimberVol ;

         //if clear cut, 100% taken, resulting volumes can't be zero, so go back to the table and volumes for this VEGCLASS
         if ( disturb == 1 )
            {
            if (found && luRow >= 0)
               {
               basalArea = m_vegLookupTable.GetColData(luRow, BAA_GE_3);
               treesPerHectare = m_vegLookupTable.GetColData(luRow, TreesPha);
               bitterbrushCover = m_vegLookupTable.GetColData(luRow, PUTR2_AVGCOV);
               junipersPerHa = m_vegLookupTable.GetColData(luRow, JunipPha);
               liveBiomass = m_vegLookupTable.GetColData(luRow, LIVEBIO_MGHA);
               deadBiomass = m_vegLookupTable.GetColData(luRow, TOTDEADBIO_MGHA);
               liveCarbon = m_vegLookupTable.GetColData(luRow, LIVEC_MGHA);
               deadCarbon = m_vegLookupTable.GetColData(luRow, TOTDEADC_MGHA);
               liveVolumeGe3 = m_vegLookupTable.GetColData(luRow, VPH_GE_3);
               liveVolume_3_25 = m_vegLookupTable.GetColData(luRow, VPH_3_25);
               liveBiomass_3_25 = m_vegLookupTable.GetColData(luRow, BPH_3_25);
               liveVolume_25_50 = m_vegLookupTable.GetColData(luRow, VPH_25_50);
               liveVolumeGe50 = m_vegLookupTable.GetColData(luRow, VPH_GE_50);
               deadVolume = m_vegLookupTable.GetColData(luRow, TOTDEADVOL_M3HA);

               if ( basalArea        - m_priorBasalArea            < 0.0 ) basalArea         = m_priorBasalArea;
               if ( treesPerHectare  - m_priorTreesPerHectare      < 0.0 ) treesPerHectare   = m_priorTreesPerHectare;
               if ( bitterbrushCover - m_priorBitterbrushCover   < 0.0 ) bitterbrushCover = m_priorBitterbrushCover;
               if ( junipersPerHa    - m_priorJunipersPerHa      < 0.0 ) junipersPerHa      = m_priorJunipersPerHa;
               if ( liveBiomass      - m_priorLiveBiomass         < 0.0 ) liveBiomass      = m_priorLiveBiomass;
               //if ( deadBiomass      - m_priorDeadBiomass        < 0.0 ) deadBiomass      = m_priorDeadBiomass;
               if ( liveCarbon       - m_priorLiveCarbon         < 0.0 ) liveCarbon         = m_priorLiveCarbon;
               //if ( deadCarbon       - m_priorDeadCarbon         < 0.0 ) deadCarbon         = m_priorDeadCarbon;
               if ( liveVolumeGe3    - m_priorLiveVolumeGe3      < 0.0 ) liveVolumeGe3      = m_priorLiveVolumeGe3;
               if ( liveVolume_3_25  - m_priorLiveVolume_3_25    < 0.0 ) liveVolume_3_25   = m_priorLiveVolume_3_25;
               if ( liveBiomass_3_25 - m_priorLiveBiomass_3_25   < 0.0 ) liveBiomass_3_25 = m_priorLiveBiomass_3_25;
               if ( liveVolume_25_50 - m_priorLiveVolume_25_50   < 0.0 ) liveVolume_25_50 = m_priorLiveVolume_25_50;
               if ( liveVolumeGe50   - m_priorLiveVolumeGe50     < 0.0 ) liveVolumeGe50   = m_priorLiveVolumeGe50;
               //if ( deadVolume       - m_priorDeadVolume         < 0.0 ) deadVolume         = m_priorDeadVolume;

                sawTimberVol = liveVolumeGe3 - liveVolume_3_25;
               }
            }
            
            UpdateIDU(pContext, idu, m_colSawTimberVolume, sawTimberVol, useAddDelta ? ADD_DELTA : SET_DATA );
         }

      if ( transType > 0 )
         {
      
         // column name: BASAL_AREA
         UpdateIDU(pContext, idu, m_colBasalArea, basalArea, useAddDelta ? ADD_DELTA : SET_DATA );

         //column name: TreesPHa
         UpdateIDU(pContext, idu, m_colTreesPerHectare, treesPerHectare, useAddDelta ? ADD_DELTA : SET_DATA );

         //column name: PurshiaCov
         UpdateIDU(pContext, idu, m_colBitterbrushCover, bitterbrushCover, useAddDelta ? ADD_DELTA : SET_DATA );

         //column name: JunipPHa
         UpdateIDU(pContext, idu, m_colJunipersPerHectare, junipersPerHa, useAddDelta ? ADD_DELTA : SET_DATA );

         //update columns for live, dead, and total biomass
         //column name: LBioMgha
         UpdateIDU(pContext, idu, m_colLBioMgha, liveBiomass, useAddDelta ? ADD_DELTA : SET_DATA );

         //column name: DBioMgha
         //UpdateIDU(pContext, idu, m_colDBioMgha, deadBiomass, useAddDelta ? ADD_DELTA : SET_DATA );

         //no column for this available, total biomass = live + dead
         //float totalBiomass = liveBiomass + deadBiomass;
         //UpdateIDU(pContext, idu, m_colTBioMgha, totalBiomass, useAddDelta ? ADD_DELTA : SET_DATA );

         // update columns for live, dead, and total carbon pools
         // column name: LCMgha
         UpdateIDU(pContext, idu, m_colLCMgha, liveCarbon, useAddDelta ? ADD_DELTA : SET_DATA );

         // column name: DCMgha
         //UpdateIDU(pContext, idu, m_colDCMgha, deadCarbon, useAddDelta ? ADD_DELTA : SET_DATA );

         // column name: TCMgha, total carbon stock = live + dead
         //totalCarbon = liveCarbon + deadCarbon;
         //UpdateIDU(pContext, idu, m_colTCMgha, totalCarbon, useAddDelta ? ADD_DELTA : SET_DATA );

         //update columns for live, dead, and total volume
         //column name: LVolm3ha
         UpdateIDU(pContext, idu, m_colLVolm3ha, liveVolumeGe3, useAddDelta ? ADD_DELTA : SET_DATA );

         // column name: DVolm3ha
         //UpdateIDU(pContext, idu, m_colDVolm3ha, deadVolume, useAddDelta ? ADD_DELTA : SET_DATA );

         // column name: TVolm3ha, total volume = live + dead
         //float totVolume = liveVolumeGe3 + deadVolume;
         //UpdateIDU(pContext, idu, m_colTVolm3ha, totVolume, useAddDelta ? ADD_DELTA : SET_DATA );

         // update columns for small diameter volume and saw timber volume
         // column name: VPH_3_25
         UpdateIDU(pContext, idu, m_colVPH_3_25, liveVolume_3_25, useAddDelta ? ADD_DELTA : SET_DATA );

         // column name: BPH_3_25
         UpdateIDU(pContext, idu, m_colBPH_3_25, liveBiomass_3_25, useAddDelta ? ADD_DELTA : SET_DATA );

         // column name: LVol25_50
         UpdateIDU(pContext, idu, m_colLVol25_50, liveVolume_25_50, useAddDelta ? ADD_DELTA : SET_DATA );

         // column name: LVolGe50
         UpdateIDU(pContext, idu, m_colLVolGe50, liveVolumeGe50, useAddDelta ? ADD_DELTA : SET_DATA );

         // column name: SAWTIMBVOL
         //UpdateIDU(pContext, idu, m_colSawTimberVolume, sawTimberVol, useAddDelta ? ADD_DELTA : SET_DATA );
      
         // this updates the available SawTimberVolume (GE3cm - 25cm) available for different treatment types (m3)
         //float availableSawTimberVol55 = sawTimberVol * m_percentOfLivePassedOnAvailable55 * iduAreaHa;
         //float availableSawTimberVol3 = sawTimberVol * m_percentOfLivePassedOnAvailable3 * iduAreaHa;
         //
         //float availableSawTimberVol57 = sawTimberVol * m_percentOfLivePassedOnAvailable57 * iduAreaHa;
         //float availableSawTimberVol1 = sawTimberVol * m_percentOfLivePassedOnAvailable1 * iduAreaHa;
         //
         //UpdateIDU(pContext, idu, m_colAvailVolTFB, availableSawTimberVol55, useAddDelta ? ADD_DELTA : SET_DATA );
         //UpdateIDU(pContext, idu, m_colAvailVolPH, availableSawTimberVol3, useAddDelta ? ADD_DELTA : SET_DATA );      
         //UpdateIDU(pContext, idu, m_colAvailVolPHH, availableSawTimberVol57, useAddDelta ? ADD_DELTA : SET_DATA );
         //UpdateIDU(pContext, idu, m_colAvailVolRH, availableSawTimberVol1, useAddDelta ? ADD_DELTA : SET_DATA );
         }      
      }

   ::EnvApplyDeltaArray(pContext->pEnvModel);

   return true;
   }


bool COCNHProcess::UpdateAvailVolumesFromTable(EnvContext *pContext, bool useAddDelta)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   //   int iCPU = omp_get_num_procs();
   //   omp_set_num_threads(iCPU);

   //#pragma omp parallel for   
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int transType = 0;
      pLayer->GetData( idu, m_colVegTranType, transType );

      float iduAreaM2;
      float sawTimberVol = 0.0f;
            
      pLayer->GetData(idu, m_colArea, iduAreaM2);
      pLayer->GetData( idu, m_colSawTimberVolume, sawTimberVol);
      float iduAreaHa = iduAreaM2 * HA_PER_M2;
         
      // this updates the available SawTimberVolume (GE3cm - 25cm) available for different treatment types (m3)
      float availableSawTimberVol55 = sawTimberVol * m_percentPreTransStructAvailable55 * iduAreaHa;
      float availableSawTimberVol3 = sawTimberVol * m_percentPreTransStructAvailable3 * iduAreaHa;
      
      float availableSawTimberVol57 = sawTimberVol * m_percentPreTransStructAvailable57 * iduAreaHa;
      float availableSawTimberVol1 = sawTimberVol * m_percentPreTransStructAvailable1 * iduAreaHa;

      UpdateIDU(pContext, idu, m_colAvailVolTFB, availableSawTimberVol55, useAddDelta ? ADD_DELTA : SET_DATA );
      UpdateIDU(pContext, idu, m_colAvailVolPH, availableSawTimberVol3, useAddDelta ? ADD_DELTA : SET_DATA );      
      UpdateIDU(pContext, idu, m_colAvailVolPHH, availableSawTimberVol57, useAddDelta ? ADD_DELTA : SET_DATA );
      UpdateIDU(pContext, idu, m_colAvailVolRH, availableSawTimberVol1, useAddDelta ? ADD_DELTA : SET_DATA );      
      }

   ::EnvApplyDeltaArray(pContext->pEnvModel);

   return true;
   }


bool COCNHProcess::InitVegParamsFromTable(EnvContext *pContext, bool useAddDelta)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   //   int iCPU = omp_get_num_procs();
   //   omp_set_num_threads(iCPU);

   //#pragma omp parallel for   
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int transType = 0;

      pLayer->GetData(idu, m_colVegTranType, transType);

      int vegClass = -999;
      int pvt = -999;
      int region = -999;
      int disturb = 0;
      float iduAreaM2 = 0.0;

      pLayer->GetData(idu, m_colArea, iduAreaM2);
      pLayer->GetData(idu, m_colVegClass, vegClass);
      pLayer->GetData(idu, m_colPVT, pvt);
      pLayer->GetData(idu, m_colRegion, region);
      pLayer->GetData(idu, m_colDisturb, disturb);

      float iduAreaHa = iduAreaM2 * HA_PER_M2;

      VEGKEY key(vegClass, region, pvt);
      int luRow = -1;
      BOOL found = m_vegLookupTable.Lookup(key.GetKey(), luRow);

      //if ( ! found )
      //   {
      //   __int64 badKey = key.GetKey();
      //
      //   ASSERT( 0 );
      //   }

      float basalArea = 0.f;
      float treesPerHectare = 0.f;
      float bitterbrushCover = 0.f;
      float junipersPerHa = 0.f;
      float liveBiomass = 0.f;
      float deadBiomass = 0.f;
      float liveCarbon = 0.f;
      float deadCarbon = 0.f;
      float totalCarbon = 0.f;
      float liveVolumeGe3 = 0.f;
      float liveVolume_3_25 = 0.f;
      float liveBiomass_3_25 = 0.f;
      float liveVolume_25_50 = 0.f;
      float liveVolumeGe50 = 0.f;
      float deadVolume = 0.f;
      float totalVolume = 0.f;
      float smallDiaVolume = 0.f;
      float sawTimberVol = 0.f;

      // a successional transition occured in this IDU
      
      if (found && luRow >= 0)
         {
         basalArea = m_vegLookupTable.GetColData(luRow, BAA_GE_3);
         treesPerHectare = m_vegLookupTable.GetColData(luRow, TreesPha);
         bitterbrushCover = m_vegLookupTable.GetColData(luRow, PUTR2_AVGCOV);
         junipersPerHa = m_vegLookupTable.GetColData(luRow, JunipPha);
         liveBiomass = m_vegLookupTable.GetColData(luRow, LIVEBIO_MGHA);
         deadBiomass = m_vegLookupTable.GetColData(luRow, TOTDEADBIO_MGHA);
         liveCarbon = m_vegLookupTable.GetColData(luRow, LIVEC_MGHA);
         deadCarbon = m_vegLookupTable.GetColData(luRow, TOTDEADC_MGHA);
         liveVolumeGe3 = m_vegLookupTable.GetColData(luRow, VPH_GE_3);
         liveVolume_3_25 = m_vegLookupTable.GetColData(luRow, VPH_3_25);
         liveBiomass_3_25 = m_vegLookupTable.GetColData(luRow, BPH_3_25);
         liveVolume_25_50 = m_vegLookupTable.GetColData(luRow, VPH_25_50);
         liveVolumeGe50 = m_vegLookupTable.GetColData(luRow, VPH_GE_50);
         deadVolume = m_vegLookupTable.GetColData(luRow, TOTDEADVOL_M3HA);

         sawTimberVol = liveVolumeGe3 - liveVolume_3_25;
         }
      else if (vegClass >= 2000000 && updateVegParamsErrCount < 10)
         {
         CString msg;
         msg.Format("UpdateVegParamsFromTable() - missing lookup for vegclass=%i, pvt=%i, region=%i", vegClass, pvt, region);
         Report::LogWarning(msg);
         updateVegParamsErrCount++;
         }

      // column name: BASAL_AREA
      pLayer->SetData(idu, m_colBasalArea, basalArea);

      //column name: TreesPHa
      pLayer->SetData(idu, m_colTreesPerHectare, treesPerHectare);

      //column name: PurshiaCov
      pLayer->SetData(idu, m_colBitterbrushCover, bitterbrushCover);

      //column name: JunipPHa
      pLayer->SetData(idu, m_colJunipersPerHectare, junipersPerHa);

      //update columns for live, dead, and total biomass
      //column name: LBioMgha
      pLayer->SetData(idu, m_colLBioMgha, liveBiomass);

      //column name: DBioMgha
      pLayer->SetData(idu, m_colDBioMgha, deadBiomass);

      //no column for this available, total biomass = live + dead
      float totalBiomass = liveBiomass + deadBiomass;
      pLayer->SetData(idu, m_colTBioMgha, totalBiomass);

      // update columns for live, dead, and total carbon pools
      // column name: LCMgha
      pLayer->SetData(idu, m_colLCMgha, liveCarbon);

      // column name: DCMgha
      pLayer->SetData(idu, m_colDCMgha, deadCarbon);

      // column name: TCMgha, total carbon stock = live + dead
      totalCarbon = liveCarbon + deadCarbon;
      pLayer->SetData(idu, m_colTCMgha, totalCarbon);

      //update columns for live, dead, and total volume
      //column name: LVolm3ha
      pLayer->SetData(idu, m_colLVolm3ha, liveVolumeGe3);

      // column name: DVolm3ha
      pLayer->SetData(idu, m_colDVolm3ha, deadVolume);

      // column name: TVolm3ha, total volume = live + dead
      float totVolume = liveVolumeGe3 + deadVolume;
      pLayer->SetData(idu, m_colTVolm3ha, totVolume);

      // update columns for small diameter volume and saw timber volume
      // column name: VPH_3_25
      pLayer->SetData(idu, m_colVPH_3_25, liveVolume_3_25);

      // column name: BPH_3_25
      pLayer->SetData(idu, m_colBPH_3_25, liveBiomass_3_25);

      // column name: LVol25_50
      pLayer->SetData(idu, m_colLVol25_50, liveVolume_25_50);

      // column name: LVolGe50
      pLayer->SetData(idu, m_colLVolGe50, liveVolumeGe50);

      // column name: SAWTIMBVOL
      pLayer->SetData(idu, m_colSawTimberVolume, sawTimberVol);

      // this updates the available SawTimberVolume (GE3cm - 25cm) available for different treatment types (m3)
      float availableSawTimberVol55 = sawTimberVol * m_percentPreTransStructAvailable55 * iduAreaHa;
      float availableSawTimberVol3 = sawTimberVol * m_percentPreTransStructAvailable3 * iduAreaHa;

      float availableSawTimberVol57 = sawTimberVol * m_percentPreTransStructAvailable57 * iduAreaHa;
      float availableSawTimberVol1 = sawTimberVol * m_percentPreTransStructAvailable1 * iduAreaHa;

      pLayer->SetData(idu, m_colAvailVolTFB, availableSawTimberVol55);
      pLayer->SetData(idu, m_colAvailVolPH, availableSawTimberVol3);
      pLayer->SetData(idu, m_colAvailVolPHH, availableSawTimberVol57);
      pLayer->SetData(idu, m_colAvailVolRH, availableSawTimberVol1);
      }

   return true;
   }


bool COCNHProcess::UpdateTimeSinceTreatment(EnvContext *pContext)
   {
   // Note:  this has to run BEFORE the disturb codes are reset
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      //int decisionRule = -1;
      int disturb = -1;
      int oldTST = -1;

      pLayer->GetData(idu, m_colTST, oldTST);
      //pLayer->GetData( idu, m_colPolicyID, decisionRule );
      pLayer->GetData(idu, m_colDisturb, disturb);

      int TST = oldTST;

      if ((disturb >= 50 && disturb < 70) || disturb == 2)
         TST = 1;
      else if (oldTST >= 0)    // -1 means never treated
         TST++;

      //if( decisionRule == 55 )
      //   // if disturbance is 5 = prescibed fire, set time since treatment to zero
      //   m_timeSinceTreat = 0;
      //else
      //   m_timeSinceTreat++;

      if (oldTST != TST)
         UpdateIDU(pContext, idu, m_colTST, TST, ADD_DELTA);
      }

   return true;
   }


bool COCNHProcess::SetPriorVegClass(EnvContext *pContext)
   {

   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {

      int currentVegClass = -1;
      int priorVegClass = -1;

      pLayer->GetData(idu, m_colVegClass, currentVegClass);

      pLayer->GetData(idu, m_colPriorVeg, priorVegClass);

      if (currentVegClass != priorVegClass)
         UpdateIDU(pContext, idu, m_colPriorVeg, priorVegClass, SET_DATA);
      }

   return true;
   }


bool COCNHProcess::UpdateTimeSinceHarvest(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = -1;
      int oldTSH = -1;

      pLayer->GetData(idu, m_colTSH, oldTSH);
      pLayer->GetData(idu, m_colDisturb, disturb);

      int TSH = oldTSH;

      switch (disturb)
         {
         case HARVEST:
            TSH = 1;
            break;

         default:
            if (oldTSH >= 0)
               TSH++;
         }

      if (oldTSH != TSH)
         UpdateIDU(pContext, idu, m_colTSH, TSH, ADD_DELTA);
      }

   return true;
   }


bool COCNHProcess::UpdateTimeSinceThinning(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = -1;
      int oldTSTH = -1;

      pLayer->GetData(idu, m_colTSTH, oldTSTH);
      pLayer->GetData(idu, m_colDisturb, disturb);

      int TSTH = oldTSTH;

      switch (disturb)
         {
         case THINNING:                   // 2
         case MECHANICAL_THINNING:        // 50
         case THIN_FROM_BELOW:            // 55
         case PARTIAL_HARVEST_LIGHT:      // 56
         case PARTIAL_HARVEST_HIGH:       // 57
            TSTH = 1;
            break;

         default:
            if (oldTSTH >= 0)
               TSTH++;
         }

      if (oldTSTH != TSTH)
         UpdateIDU(pContext, idu, m_colTSTH, TSTH, ADD_DELTA);
      }

   return true;
   }



bool COCNHProcess::UpdateTimeSincePartialHarvest(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = -1;
      int oldTSPH = -1;

      pLayer->GetData(idu, m_colTSPH, oldTSPH);
      pLayer->GetData(idu, m_colDisturb, disturb);

      int TSPH = oldTSPH;

      switch (disturb)
         {
         case PARTIAL_HARVEST:
         case PARTIAL_HARVEST_LIGHT:
         case PARTIAL_HARVEST_HIGH:
            TSPH = 1;
            break;

         default:
            if (oldTSPH >= 0)
               TSPH++;
         }

      if (oldTSPH != TSPH)
         UpdateIDU(pContext, idu, m_colTSPH, TSPH, ADD_DELTA);
      }

   return true;
   }



bool COCNHProcess::UpdateTimeSinceFire(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = -1;
      int oldTSF = -1;

      pLayer->GetData(idu, m_colTSF, oldTSF);
      pLayer->GetData(idu, m_colDisturb, disturb);

      int TSF = oldTSF;

      // if a positive fire disturbance occured in the prior timestep, then set TSF to 1
      // Note: this check happens at the beginning of a time step, but before DISTURBances
      // are flipped to negative values
      if ((disturb >= SURFACE_FIRE && disturb <= STAND_REPLACING_FIRE))
         //         || ( disturb >= PRESCRIBED_FIRE && disturb <= PRESCRIBED_STAND_REPLACING_FIRE ) )
         TSF = 1;

      else if (oldTSF >= 0)
         TSF++;

      if (oldTSF != TSF)
         UpdateIDU(pContext, idu, m_colTSF, TSF, ADD_DELTA);
      }

   return true;
   }


bool COCNHProcess::UpdateTimeSincePrescribedFire(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = -1;
      int oldTSPF = -1;

      pLayer->GetData(idu, m_colTSPF, oldTSPF);
      pLayer->GetData(idu, m_colDisturb, disturb);

      int TSPF = oldTSPF;

      if (disturb >= PRESCRIBED_FIRE && disturb <= PRESCRIBED_STAND_REPLACING_FIRE)
         TSPF = 1;

      else if (oldTSPF >= 0)
         TSPF++;

      if (oldTSPF != TSPF)
         UpdateIDU(pContext, idu, m_colTSPF, TSPF, ADD_DELTA);
      }

   return true;
   }


bool COCNHProcess::UpdateVegClassVars(EnvContext *pContext, bool useAddDelta)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   int noDataValue = (int)pLayer->GetNoDataValue();

   for (MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++)
      {
      int vegClass = -1;
      pLayer->GetData(idu, m_colVegClass, vegClass);

      int coverType = noDataValue;
      int structStg = noDataValue;
      int cover = noDataValue;
      int size = noDataValue;
      int layers = noDataValue;

      if (vegClass >= 0)
         {
         coverType = vegClass / 10000;

         structStg = vegClass % 10000;
         structStg /= 10;

         size = structStg / 100;

         cover = structStg % 100;
         cover /= 10;

         layers = structStg % 10;
         }
      
      UpdateIDU(pContext, idu, m_colCoverType, coverType, useAddDelta ? ADD_DELTA : SET_DATA );
      UpdateIDU(pContext, idu, m_colStructStg, structStg, useAddDelta ? ADD_DELTA : SET_DATA );
      UpdateIDU(pContext, idu, m_colCanopyCover, cover, useAddDelta ? ADD_DELTA : SET_DATA );
      UpdateIDU(pContext, idu, m_colSize, size, useAddDelta ? ADD_DELTA : SET_DATA );
      UpdateIDU(pContext, idu, m_colLayers, layers, useAddDelta ? ADD_DELTA : SET_DATA );
            
      }

   return true;
   }

bool COCNHProcess::UpdatePriorVeg(EnvContext *pContext)
   {
   // get a ptr to the delta array
   DeltaArray *deltaArray = pContext->pDeltaArray;

   // use static size so deltas added
   INT_PTR daSize = deltaArray->GetSize();

   INT_PTR index = daSize - 1;

   INT_PTR myLastUnseenDelta = pContext->lastUnseenDelta;
   // iterate through deltas added since last “seen”   
   while (index >= pContext->firstUnseenDelta)
      {
      DELTA &delta = EnvGetDelta(deltaArray, index);

      if (delta.col == m_colVegClass)
         {
         int priorVeg = 0;

         delta.oldValue.GetAsInt(priorVeg);

         UpdateIDU(pContext, delta.cell, m_colPriorVeg, priorVeg, SET_DATA);

         break;
         }
      index--;
      }

   return true;
   }


bool COCNHProcess::UpdateThreatenedSpecies(EnvContext *pContext, bool useAddDelta)
   {
   /*
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   int colTESpecies = pLayer->GetFieldCol("TESPECIES");

   int colS0 = pLayer->GetFieldCol("AmMarten");
   int colS1 = pLayer->GetFieldCol("BBWoodpeck");
   int colS2 = pLayer->GetFieldCol("NorGoshawk");
   int colS3 = pLayer->GetFieldCol("PiWoodpeck");
   int colS4 = pLayer->GetFieldCol("SpottedOwl");
   int colS5 = pLayer->GetFieldCol("RNSapsuck");
   int colS6 = pLayer->GetFieldCol("WeBluebird");
   int colS7 = pLayer->GetFieldCol("WHWoodpeck");

   //iterate through idu layer
   for (MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++)
      {
      vector< int > species(8, 0);
      int speciesCount = 0;
      int oldTESpeciesValue = -1;

      // get values from idu shapefile
      pLayer->GetData(idu, colS0, species.at(0));
      pLayer->GetData(idu, colS1, species.at(1));
      pLayer->GetData(idu, colS2, species.at(2));
      pLayer->GetData(idu, colS3, species.at(3));
      pLayer->GetData(idu, colS4, species.at(4));
      pLayer->GetData(idu, colS5, species.at(5));
      pLayer->GetData(idu, colS6, species.at(6));
      pLayer->GetData(idu, colS7, species.at(7));
      pLayer->GetData(idu, colTESpecies, oldTESpeciesValue);

      // idu potentially suitable as habitat for one of the endangered/threatened species?
      for (int i = 0; i < species.size(); i++)
         {
         if (species.at(i) == 1)
            speciesCount += 1;
         //if( speciesCount == 1 )
         //   break;
         }
      
      UpdateIDU(pContext, idu, colTESpecies, speciesCount, useAddDelta ? ADD_DELTA : SET_DATA );
      
      }
      */
   return true;
   }


bool COCNHProcess::PopulateStructure(EnvContext *pContext, bool useAddDelta)
   {
   /*
   // iterate through idu shapefile
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      float area = 0;
      float popDens = 0;
      float prevNDU = 0.f;
      float newDU = 0.f;

      int structr = -1;
      //int oldStructr  = -1;

      // get values from idus
      pLayer->GetData(idu, m_colArea, area);
      pLayer->GetData(idu, m_colPopDens, popDens);
      pLayer->GetData(idu, m_colNDU, prevNDU);

      int uga = 0;

      // number of dwelling units per idu
      float dwllUnits = popDens * area / GetPeoplePerDU(uga); // 1 is dummy value, koch 05/2013

      newDU = dwllUnits - prevNDU;
      
      UpdateIDU(pContext, idu, m_colNDU, dwllUnits, useAddDelta ? ADD_DELTA : SET_DATA );

      if (fp_neq(newDU, 0.0))
         {
         UpdateIDU(pContext, idu, m_colNewDU, newDU, useAddDelta ? ADD_DELTA : SET_DATA );
         }
      else
         {
         UpdateIDU(pContext, idu, m_colNewDU, 0.0f, useAddDelta ? ADD_DELTA : SET_DATA );
         }
      
      // is there at least one residential structure on the idu?
      if (dwllUnits > 1.0f)
         structr = 1;

      // have we crossed as population density to justify a veglass change to developed?
      //if ( IsResidential( zone ) )
      //   {
      //   if ( IsUrban( zone ) )
      //      {

      float duPerAc = dwllUnits * M2_PER_ACRE / area;

      int lulcC = -1;
      if (duPerAc > 0.5f && duPerAc <= 4)       lulcC = 1001100;  // low density
      else if (duPerAc > 4 && duPerAc <= 12)    lulcC = 1101200;   // medium density
      else if (duPerAc > 12)                    lulcC = 1201300;   // high density

      
      if (lulcC > 0)
         UpdateIDU(pContext, idu, m_colVegClass, lulcC, useAddDelta ? ADD_DELTA : SET_DATA );

      // add new value to delta array
      UpdateIDU(pContext, idu, m_colStructure, structr, useAddDelta ? ADD_DELTA : SET_DATA );
      
      }
      */
   return true;
   }


bool COCNHProcess::PopulateWUI(EnvContext *pContext, bool useAddDelta)
   {
   // From http://silvis.forest.wisc.edu/old/Library/WUIDefinitions.php
   //
   // WUI is composed of both interface and intermix communities. In both interface 
   // and intermix communities, housing must meet or exceed a minimum density of 
   // one structure per 40 acres (16 ha).
   //
   // Intermix communities are places where housing and vegetation intermingle.
   // In intermix, wildland vegetation is continuous, more than 50 percent vegetation,
   // in areas with more than 1 house per 16 ha. 
   //
   // Interface communities are areas with housing in the vicinity of 
   // contiguous vegetation. Interface areas have more than 1 house per 40 acres, 
   // have less than 50 percent vegetation, and are within 1.5 mi of an area 
   // (made up of one or more contiguous Census blocks) over 1,325 acres (500 ha)
   // that is more than 75 percent vegetated. The minimum size limit ensures 
   // that areas surrounding small urban parks are not classified as interface WUI.

   const float radius = 908.0f;     // radius (m) corresponding to 1 sq mile circle
   const int   maxNeighbors = 2048;
   const float lowDensThreshold = 1 / 40.0f; // du/acre; //6.178f * PEOPLE_PER_DU / 10000;  // people/m2
   const float medDensThreshold = 1 / 16.0f; // du/acre  // 49.421f * PEOPLE_PER_DU / 10000; // people/m2
   const float highDensThreshold = 1.0f;    // 741.31f * PEOPLE_PER_DU / 10000; // people/m2

   int maxCount = 0;

   // update residential LULC classes if needed
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   int neighbors[maxNeighbors];

   int   *iduPctBurnable = new int[pLayer->GetPolygonCount()];
   float *iduWuiPopDens = new float[pLayer->GetPolygonCount()];    // du/acre

   // pass 1: iterate through the UGA layer, computing WUI stats
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      Poly *pPoly = pLayer->GetPolygon(idu);
      int count = pLayer->GetNearbyPolys(pPoly, neighbors, NULL, maxNeighbors, radius);

      if (count > maxCount)
         maxCount = count;

      float totalArea = 0;
      float burnableArea = 0;
      float population = 0;
      float dus = 0;

      for (int i = 0; i < count; i++)
         {
         float area = 0;
         pLayer->GetData(neighbors[i], m_colArea, area);

         int uga = 0;
         //pLayer->GetData( neighbors[ i ], m_colUga, uga );

         totalArea += area;

         int vegClass = 0;
         pLayer->GetData(neighbors[i], m_colVegClass, vegClass);

         float popDens = 0;
         pLayer->GetData(neighbors[i], m_colPopDens, popDens);
         population += popDens * area;

         float ppDU = GetPeoplePerDU(uga);    // people per dwelling unit
         dus += popDens * area / ppDU;          // accumulate dwelling units

         // new burnable definition - from Emily email 9/5/13
         int fuelModel = 0;
         pLayer->GetData(neighbors[i], m_colFuelModel, fuelModel);

         float dusPerHa = popDens * M2_PER_HA / ppDU;        //#/m2 * m2/ha / #/du
         if (fuelModel > 100 && dusPerHa < 3)
            burnableArea += area;
         }

      // have neighbor info, determine WUI class 
      float popDens = population / totalArea;    // people/m2
      float dusPerAcre = dus / (totalArea / M2_PER_ACRE);   // popDens * M2_PER_ACRE / PEOPLE_PER_DU;
      int pctBurnable = int(burnableArea * 100 / totalArea);

      iduPctBurnable[idu] = pctBurnable;   // why was this commented out?
      iduWuiPopDens[idu] = dusPerAcre;  //popDens; 
      }

   // pass 2, classify and look for interface zones
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int wui = 0;

      if (iduPctBurnable[idu] <= 50)     // Interface candidate?
         {
         Poly *pPoly = pLayer->GetPolygon(idu);
         int count = pLayer->GetNearbyPolys(pPoly, neighbors, NULL, maxNeighbors, 2400);   // why 2400?

         bool isInterface = false;

         for (int i = 0; i < count; i++)
            {
            if (iduPctBurnable[neighbors[i]] > 75)
               {
               isInterface = true;
               break;
               }
            }

         if (isInterface)
            {
            if (IsBetween(iduWuiPopDens[idu], lowDensThreshold, medDensThreshold))
               wui = 6;
            else if (IsBetween(iduWuiPopDens[idu], medDensThreshold, highDensThreshold))
               wui = 5;
            else if (iduWuiPopDens[idu] > highDensThreshold)
               wui = 4;
            }
         }
      else  // intermix
         {
         if (IsBetween(iduWuiPopDens[idu], lowDensThreshold, medDensThreshold))
            wui = 3;
         else if (IsBetween(iduWuiPopDens[idu], medDensThreshold, highDensThreshold))
            wui = 2;
         else if (iduWuiPopDens[idu] > highDensThreshold)
            wui = 1;
         }
      
      UpdateIDU(pContext, idu, m_colWUI, wui, useAddDelta ? ADD_DELTA : SET_DATA );
      
      }

   // clean up
   delete[] iduPctBurnable;
   delete[] iduWuiPopDens;

   return true;
   }


float GetPeoplePerDU(int uga)
   {
   return 2.3f;
   }


bool COCNHProcess::CalcTreatmentCost(EnvContext *pContext, bool useAddDelta)
   {
   /* not required, koch 02/2013
   this->LoadLookupTableTreatCosts(pContext, m_initStrTreatCosts);
   */

   // conversion factor from square meters to acres
   float conversArea = 0.0002471f;

   // conversion factor from meters to 1000 feet, /1000 *3.28084
   float conversElev = (float) 0.00328084f;

   //conversion factor from tonnes per hectare to tonnes per acre
   // !!! nochmal kontrollieren, ob in der datenbank wirklich tonnes per hectare sind !!!
   float conversBiomass = (float) 0.404685642;

   //calculate treatment cost for all idus
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      float
         area = -1.0f,
         elevation = -1.0f,
         //activity type hand piling
         pileHand = -1.0f,
         //activity type biomass reduction
         biomassReduct = -1.0f,
         //fuel model
         fm = -1.0f,
         //fire regime
         fr = -1.0f,
         fuelLoad = -1.0f;

      int
         wui = -1,
         aspect = -1,
         //treatment = -1,
         fmDB = -1,
         fireReg = -1,
         teSpp = -1;

      //get values from IDU table
      //pLayer->GetData( idu, m_colTreatment, treatment );
      pLayer->GetData(idu, m_colArea, area);
      pLayer->GetData(idu, m_colElevation, elevation);
      pLayer->GetData(idu, m_colWUI, wui);
      pLayer->GetData(idu, m_colFuelModel, fm);
      pLayer->GetData(idu, m_colAspect, aspect);
      pLayer->GetData(idu, m_colFireReg, fireReg);
      pLayer->GetData(idu, m_colThreEndSpecies, teSpp);

      /* not required, koch 02/2013
      // iterate through map to get values from lookup table
      map< int, vector< int > >::iterator itInt;
      for( itInt = m_mapLookupTreatCostsInt.begin(); itInt != m_mapLookupTreatCostsInt.end(); ++itInt )
      {
      if( itInt->first == idu )
      {
      //pileDB = itInt->second.at( 1 );// hier pile raus, koch 08/2012
      teSpp = itInt->second.at( 1 );
      //airIgnit = itInt->second.at( 3 );// hier aerial ignition raus, koch 08/2012
      //fireReg = itInt->second.at( 2 ); // from IDU shapefile, koch 11/2012
      }
      }
      */

      // iterate through map to get values from lookup table
      map< int, float >::iterator itFloat;
      for (itFloat = m_mapLookupTreatCostsFloat.begin(); itFloat != m_mapLookupTreatCostsFloat.end(); ++itFloat)
         {
         if (itFloat->first == idu)
            fuelLoad = itFloat->second;
         }

      float
         //treatment costs for prescribed burn
         treatmentCostsPBYN = 0.0f,
         oldtreatmentCostsPBYN = 0.0f,
         treatmentCostsPBNN = 0.0f,
         oldtreatmentCostsPBNN = 0.0f,

         //treatment costs for mechanical treatment
         treatmentCostsMTY = 0.0f,
         oldtreatmentCostsMTY = 0.0f,
         treatmentCostsMTN = 0.0f,
         oldtreatmentCostsMTN = 0.0f;

      //if idu is located in wui (no differentiation between the different 
      //wui types), then factor will be considered in cost calculation
      if (wui > 0)
         wui = 1;

      //hand pile
      pileHand = -0.431f;
      //biomass reduction
      biomassReduct = 1.142f;

      // fuel model factor values are specified for prescribed fire
      if (fmDB == 2)
         fm = 1.06f;
      else if (fmDB == 6)
         fm = -0.859f;
      else if (fmDB == 11)
         fm = -0.731f;
      else if (fmDB == 12)
         fm = -1.048f;
      else
         fm = 0.0f;

      // fire regime factor values are specified for mechanical treatment
      if (fireReg == 2)
         fr = -0.446f;
      else if (fireReg == 3)
         fr = -0.697f;
      else if (fireReg == 4)
         fr = -1.002f;
      else
         fr = 0.0f;

      // aspect factor values are specified for mechanical treatment
      if ((aspect == 0) || (aspect == 1) || (aspect == 2))// aspect in [southeast, south, southwest]
         aspect = 1;
      else
         aspect = 0;
      
      // total $ for the IDU.  Note that these are the costs if treatment is actually done, not that it has been done
      treatmentCostsPBYN = area*conversArea*exp(6.29f - (0.349f*log(area * conversArea)) + 0.296f*wui + pileHand + fm + 0.506f*teSpp);
      UpdateIDU(pContext, idu, m_colCostsPrescribedFireHand, treatmentCostsPBYN, useAddDelta ? ADD_DELTA : SET_DATA );

      treatmentCostsPBNN = area*conversArea*exp(6.29f - (0.349f*log(area * conversArea)) + 0.296f*wui + fm + 0.506f*teSpp);
      UpdateIDU(pContext, idu, m_colCostsPrescribedFire, treatmentCostsPBNN, useAddDelta ? ADD_DELTA : SET_DATA );

      treatmentCostsMTY = area*conversArea*exp(6.954f - (0.299f*log(area * conversArea)) + 0.484f*wui + 1.142f*biomassReduct + 0.014f*(fuelLoad*conversBiomass) + fr + 0.255f*aspect - 0.184f*(elevation*conversElev));
      UpdateIDU(pContext, idu, m_colCostsMechTreatmentBio, treatmentCostsMTY, useAddDelta ? ADD_DELTA : SET_DATA );

      treatmentCostsMTN = area*conversArea*exp(6.954f - (0.299f*log(area * conversArea)) + 0.484f*wui + 0.014f*(fuelLoad*conversBiomass) + fr + 0.255f*aspect - 0.184f*(elevation*conversElev));
      UpdateIDU(pContext, idu, m_colCostsMechTreatment, treatmentCostsMTN, useAddDelta ? ADD_DELTA : SET_DATA );
      
      }

   return true;
   }


bool COCNHProcess::UpdateDisturbanceValue(EnvContext *pContext, bool useAddDelta)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++)
      {
      int disturb = 0;

      pLayer->GetData(idu, m_colDisturb, disturb);

      if (disturb > 0)
         UpdateIDU(pContext, idu, m_colDisturb, -disturb, useAddDelta ? ADD_DELTA : SET_DATA );
      }

   return true;
   }


bool COCNHProcess::UpdateAvgTreesPerHectare(EnvContext *pContext)
   {
   
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;
   const int
      expectedPolys = 500;

   int
      neighbors[expectedPolys],
      maxPolys = expectedPolys,
      colTreesPha = pLayer->GetFieldCol( "TreesPha" );

   float
      distances[expectedPolys],
      //TODO move variable definition and initialization
      thresDistTPH = 500.0f,
      tphValue     = 0.0f;

   
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int nDU = -1;
      int wui = -1;
      
      pLayer->GetData( idu, m_colNDU, nDU);
      pLayer->GetData( idu, m_colWUI, wui );
      
      // home owners only
      if ( nDU > 0 && wui > 0 )
         {
         int numFound = pLayer->GetNearbyPolys(pLayer->GetPolygon(idu), neighbors, distances, maxPolys, thresDistTPH);

         // iterate through IDUs within specified distance
         float totalArea = 0;
         float totalTrees = 0;

         for (int j = 0; j < numFound; j++)
            {
            float area = 0;
            pLayer->GetData( neighbors[j], m_colArea, area );

            totalArea += area;

            float value = 0.0f;
            pLayer->GetData(neighbors[j], colTreesPha, value);

            if ( value >= 0.0f )
               totalTrees += value * area;
            }

         if ( totalArea > 0 )
            tphValue = totalTrees / totalArea;
         else
            tphValue = 0.0f;

         //if different from former value, update the IDU (skip the delta array)
         UpdateIDU(pContext, idu, m_colTreesPerHect500, tphValue, SET_DATA);
         }
      }

   //ApplyDeltaArray();
   return true;
   }

bool COCNHProcess::UpdateAvgCondFlameLength(EnvContext *pContext)
   {
   const int expectedPolys = 500;

   int neighbors[expectedPolys];
   float distances[expectedPolys];
   int count = 0;

   //TODO move variable definition and initialization
   float m_thresDistCondFlameLength = 270.0f;
   float cflValue = 0.0f;

   // iterate through IDU layer
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int i = pLayer->GetNearbyPolys(pLayer->GetPolygon(idu), neighbors, distances, expectedPolys, m_thresDistCondFlameLength);
      for (int j = 0; j < i; j++)
         {
         // TODO: change FlameLen to Conditional Flame Length, change average to AREA WEIGHTED AVERAGE
         float value = 0.0f;
         pLayer->GetData(neighbors[j], pLayer->GetFieldCol("FlameLen"), value);
         if (value >= 0.0f)
            {
            count++;
            cflValue += value;
            }
         }

      //calculate average value
      if (count > 0)
         cflValue = cflValue / count;
      else
         cflValue = 0.0f;

      //if different from former value, update delta array
      float oldCflValue;
      pLayer->GetData(idu, m_colCondFlameLength270, oldCflValue);
      if (oldCflValue != cflValue)
         AddDelta(pContext, idu, m_colCondFlameLength270, cflValue);
      }

   return true;
   }


bool COCNHProcess::UpdateFireOccurrences(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   // array size variables
   const int expPolyDistShort = 750;
   const int expPolyDistMedium = 750;
   const int expPolyDistPreFireLong = 750;
   const int expPolyDistLong = 1000;
   const int expPolyDist500m = 750;

   // neighbor idu vectors
   int neighborsFireShort[expPolyDistShort];
   int neighborsFireMedium[expPolyDistMedium];
   int neighborsFireLong[expPolyDistLong];
   int neighborsPreFireLong[expPolyDistPreFireLong];
   int neighborsFire500m[expPolyDist500m];

   // counters for calculating averages
   int countFireShort = 0;
   int countFireMedium = 0;
   int countFireLong = 0;
   int countPreFireLong = 0;

   // distance vectors
   //distancesFireShort[expPolyDistShort],
   //distancesFireMedium[expPolyDistMedium],
   //distancesFireLong[expPolyDistLong],
   //distandesPreFireLong[expPolyDistMedium],
   // TODO move definition and initialization
   float m_thresDistFireShort = 1000.0f;
   float m_thresDistFireMedium = 2000.0f;
   float m_thresDistFireLong = 10000.0f;
   float m_thresDistPreFireLong = 2000.0f;
   float m_thresDistFire500m = 500.0f;

   // iterate through idu shapefile
   int idus = pLayer->GetPolygonCount();

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
   //for ( int idu=0; idu < idus; idu++)
      {
      // init counters
      int fShort = 0;
      int fShortSR = 0;
      int fShortMX = 0;
      int fMedium = 0;
      int fLong = 0;
      int fPSFLong = 0;
      int f500m = 0;
      int pFLong = 0;
      int fPotentialShort = 0;
      int fPotentialShortSR = 0;
      int fPotentialShortMX = 0;
      int fPotential500m = 0;

      //if (idu % 10000 == 0 )
      //   Report::Status_ii("Updating Fire Occurrences for IDU %i of %i", idu, idus);

      Poly *pPoly = pLayer->GetPolygon(idu);
      int countFShort = pLayer->GetNearbyPolys(pPoly, neighborsFireShort, NULL, expPolyDistShort, m_thresDistFireShort);
      ////int countFMedium = pLayer->GetNearbyPolys(pPoly, neighborsFireMedium, NULL, expPolyDistMedium, m_thresDistFireMedium);
      ////int countFLong = pLayer->GetNearbyPolys(pPoly, neighborsFireLong, NULL, expPolyDistLong, m_thresDistFireLong);
      ////int countPreFLong = pLayer->GetNearbyPolys(pPoly, neighborsPreFireLong, NULL, expPolyDistPreFireLong, m_thresDistPreFireLong);
      ////int countF500m = pLayer->GetNearbyPolys(pPoly, neighborsFire500m, NULL, expPolyDist500m, m_thresDistFire500m);
      
      float sumPotentialFlameLength = 0.0f;
      float avePotentialFlameLength1000 = 0.0f;
      int nDU = -1;
            
      pLayer->GetData( idu, m_colNDU, nDU);
      
      // IDUs with dwellings owners only
      if (nDU > 0)
         {         
         // calculate average potential flame lenght within 1km
         for ( int i = 0; i < countFShort; i++ )
            {            
            float potentialFlameLenth = 0.0f;
            pLayer->GetData(neighborsFireShort[i], m_colPotentialFlameLen, potentialFlameLenth);
            
            sumPotentialFlameLength =  sumPotentialFlameLength + potentialFlameLenth;
            }

         if ( countFShort > 0 && sumPotentialFlameLength > 0 )
            avePotentialFlameLength1000 = sumPotentialFlameLength/countFShort;

         // calculate value for FIRE5_500
         /////for (int i = 0; i < countF500m; i++)
         /////   {
         /////   int tsf = -1;
         /////   int dstrb = -1;
         /////   int potentialDisturb = -1;
         /////
         /////   //get values from idu file
         /////   pLayer->GetData(neighborsFire500m[i], m_colTSF, tsf);
         /////   pLayer->GetData(neighborsFire500m[i], m_colDisturb, dstrb);
         /////   pLayer->GetData(neighborsFire500m[i], m_colPdisturb, potentialDisturb);
         /////   // TODO: check timing, pre or post defines upper limit (5 or 6)
         /////   // only wildfire, no prescribed fire considered in utility model development
         /////
         /////   if ( ( dstrb >= SURFACE_FIRE  &&  dstrb <= STAND_REPLACING_FIRE ) || ( tsf >= 0 && tsf <= 5 ) )
         /////      f500m++;
         /////
         /////   if (  potentialDisturb >= SURFACE_FIRE  &&  potentialDisturb <= STAND_REPLACING_FIRE )
         /////      fPotential500m++;
         /////   }

         if (f500m > 0)
            f500m = 1;

         if (fPotential500m > 0)
            fPotential500m = 1;

         // calculate value for FIRE5_1000
         for (int i = 0; i < countFShort; i++)
            {
            int tsf = -1;
            int dstrb = -1;
            int potentialDisturb = -1;

            //get values from idu file
            pLayer->GetData(neighborsFireShort[i], m_colTSF, tsf);
            pLayer->GetData(neighborsFireShort[i], m_colDisturb, dstrb);
            pLayer->GetData(neighborsFireShort[i], m_colPdisturb, potentialDisturb);
            // TODO: check timing, pre or post defines upper limit (5 or 6)
            // only wildfire, no prescribed fire considered in utility model development
            if ( dstrb >= SURFACE_FIRE  &&  dstrb <= STAND_REPLACING_FIRE )
               fShort++;

            if ( potentialDisturb >= SURFACE_FIRE  &&  potentialDisturb <= STAND_REPLACING_FIRE )
               fPotentialShort++;

            if (  dstrb == STAND_REPLACING_FIRE ) // || ( dstrb == -STAND_REPLACING_FIRE  && ( tsf >= 0 && tsf <= 5 ) ) )
               fShortSR++;

            if ( potentialDisturb == STAND_REPLACING_FIRE )
               fPotentialShortSR++;

            //mixed serverity fire in this project.
            if ( dstrb == LOW_SEVERITY_FIRE ) // || ( dstrb == -LOW_SEVERITY_FIRE  && ( tsf >= 0 && tsf <= 5 ) ) )
               fShortMX++;

            //mixed serverity fire in this project.
            if ( potentialDisturb == LOW_SEVERITY_FIRE )
               fPotentialShortMX++;

            }
         if (fShort > 0)
            fShort = 1;

         if (fPotentialShort > 0)
            fPotentialShort = 1;

         if (fShortSR > 0)
            fShortSR = 1;

         if (fPotentialShortSR > 0)
            fPotentialShortSR = 1;

         if (fShortMX > 0)
            fShortMX = 1;

         if (fPotentialShortMX > 0)
            fPotentialShortMX = 1;

         // calculate value for FIRE5_2000
         ////for (int j = 0; j < countFMedium; j++)
         ////   {
         ////   int tsf = -1, dstrb = -1;
         ////
         ////   //get values from idu file
         ////   pLayer->GetData(neighborsFireMedium[j], m_colTSF, tsf);
         ////   pLayer->GetData(neighborsFireMedium[j], m_colDisturb, dstrb);
         ////   // TODO: check timing, pre or post defines upper limit (5 or 6)
         ////   // only wildfire, no prescribed fire considered in utility model development
         ////   if ( ( dstrb >= SURFACE_FIRE  && dstrb <= STAND_REPLACING_FIRE ) || ( tsf >= 0 && tsf <= 5 ) )
         ////      fMedium++;
         ////   }
         if (fMedium > 0)
            fMedium = 1;

         // calculate value for FIR5_10000
         ////for (int k = 0; k < countFLong; k++)
         ////   {
         ////              
         ////   int tsf = -1;
         ////   int dstrb = -1;
         ////   int tspf = -1;
         ////
         ////   //get values from idu file
         ////   pLayer->GetData(neighborsFireLong[k], m_colTSPF, tspf);
         ////   pLayer->GetData(neighborsFireLong[k], m_colTSF, tsf);
         ////   pLayer->GetData(neighborsFireLong[k], m_colDisturb, dstrb);
         ////   
         ////   // TODO: check timing, pre or post defines upper limit (5 or 6)
         ////   // only wildfire, no prescribed fire considered in utility model development
         ////   if ( ( dstrb >= SURFACE_FIRE  &&  dstrb <= STAND_REPLACING_FIRE )  || ( tsf >= 0 && tsf <= 5 ) )
         ////      fLong++;
         ////
         ////   if ( (dstrb >= PRESCRIBED_FIRE  &&  dstrb <= PRESCRIBED_STAND_REPLACING_FIRE )  || ( tspf >= 0 && tspf <= 5 ) )
         ////      fPSFLong++;
         ////   }

         if (fLong > 0)
            fLong = 1;

         if (fPSFLong > 0)
            fPSFLong = 1;

         // calculate value for PREF5_2000
         ////for (int l = 0; l < countFMedium; l++)
         ////   {
         ////  
         ////   int tsf = -1;
         ////   int dstrb = -1;
         ////   int tspf = -1;
         ////
         ////   //get values from idu file
         ////   pLayer->GetData(neighborsPreFireLong[l], m_colTSPF, tspf);
         ////   pLayer->GetData(neighborsPreFireLong[l], m_colTSF, tsf);
         ////   pLayer->GetData(neighborsPreFireLong[l], m_colDisturb, dstrb);
         ////
         ////   // TODO: check timing, pre or post defines upper limit (5 or 6)
         ////   // only wildfire, no prescribed fire considered in utility model development
         ////   if ( ( dstrb >= PRESCRIBED_SURFACE_FIRE  &&  dstrb <= PRESCRIBED_STAND_REPLACING_FIRE ) || ( tspf >= 0 && tspf <= 5 ) )
         ////      pFLong++;
         ////   }

         if (pFLong > 0)
            pFLong = 1;

         // vars for old IDU entries
         
         UpdateIDU(pContext, idu, m_colFire500, f500m, SET_DATA);
         UpdateIDU(pContext, idu, m_colFire1000, fShort, SET_DATA);
         UpdateIDU(pContext, idu, m_colSRFire1000, fShortSR, SET_DATA);
         UpdateIDU(pContext, idu, m_colMXFire1000, fShortMX, SET_DATA);
         UpdateIDU(pContext, idu, m_colPotentialFire500, fPotential500m, SET_DATA);
         UpdateIDU(pContext, idu, m_colPotentialFire1000, fPotentialShort, SET_DATA);
         UpdateIDU(pContext, idu, m_colAvePotentialFlameLength1000, avePotentialFlameLength1000, SET_DATA);
         UpdateIDU(pContext, idu, m_colPotentialSRFire1000, fPotentialShortSR, SET_DATA);
         UpdateIDU(pContext, idu, m_colPotentialMXFire1000, fPotentialShortMX, SET_DATA);
         UpdateIDU(pContext, idu, m_colFire2000, fMedium, SET_DATA);
         UpdateIDU(pContext, idu, m_colFire10000, fLong, SET_DATA);
         UpdateIDU(pContext, idu, m_colPrescribedFire10000, fPSFLong, SET_DATA);  
         UpdateIDU(pContext, idu, m_colPrescribedFire2000, pFLong, SET_DATA);
         }
      }

   return true;
   }



// called by COCNHProcessPre2
// Note: Assumes PLAN_AREA_INFO scores have been set in UpdatePlanAreas()
// basic idea of plan area scoring:
//   1) At the beginning of a timestep COCNHPre2), ScoreAllocationAreas[Fire]() is called
//      to score each plan area and populate PLAN_SCORE (and PS_FIRE) with this rank of the 
//      plan area score (based on query basal area (plan score) or query area (Fire))
//  2) after allocations are made (COCNHPost2), update the plan area fraction treated

bool COCNHProcess::ScoreAllocationAreas(EnvContext *pContext)
   {
   int paCount = (int)m_planAreaArray.GetSize();
   // reset plan area info
   for (int i = 0; i < paCount; i++)
      {
      PLAN_AREA_INFO *pInfo = m_planAreaArray[i];
      pInfo->area = 0;
      pInfo->score = 0;
      }

   // basic idea - iterate through 
   // iterate through idu shapefile
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   //int colPlanArea = pLayer->GetFieldCol("ALLOC_PLAN");
   if ( m_colPlanArea < 0 )
      {
      ASSERT(0);
      return true;
      }

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      // rank planning areas based on basal area 
      // planning area IDs are in "ALLOC_PLAN"

      // for each planning area, calculate average basal area for all forest LULC_A classes
      // rank the scores and populate the top three planning area "PLAN_SCORE" with 
      // 1=top, 2=second, 3=third,...N=Nth all others -1.

      int planArea = -1;
      pLayer->GetData(idu, m_colPlanArea, planArea);

      // only count areas inside a plan area
      if ( planArea > 0 )
         {
         // is there already plan area info defined for this plan area?
         PLAN_AREA_INFO *pInfo = NULL;
         BOOL found = m_planAreaMap.Lookup(planArea, pInfo);
         if ( ! found )
            {
            Report::ErrorMsg( "Error finding plan area ID");
            continue;
            }

         ASSERT( pInfo != NULL );
         if ( m_pPAQuery != NULL )
            {
            bool pass = false;
            m_pPAQuery->Run( idu, pass );

            if ( pass )
               {
               float area;
               pLayer->GetData(idu, m_colArea, area);

               float basalArea = 0;
               pLayer->GetData(idu, m_colBasalArea, basalArea);

               if ( area > 0 && basalArea > 0)
                  {
                  pInfo->score += basalArea * area;
                  pInfo->area += area;
                  }
               }  // end of: if ( pass query )
            }
         }  // end of: if ( in plan area )
      }  // end of: for each IDU

   // plan info gathered, scale and sort.  We don't want a plan that:
   // has been used in the recent past (defined by 'm_minPlanAreaReuseTime' (years))
   // Note that a plan area is considered "used" if more than specified fraction ('m_minPlanAreaFrac')
   // of the plan area has been used (treated) in the recent past (defined by m_minPlanAreaReuseTime)

   for (int i = 0; i < paCount; i++)
      {
      PLAN_AREA_INFO *pInfo = m_planAreaArray[i];
      pInfo->score /= pInfo->area;   // area-weighted avg basal area

      // have we used this one recently (meaning we've exceeded the area threshold recently?)
      // if so, set the score to a negative value
      if ((pContext->yearOfRun - pInfo->lastUsed) <= m_minPlanAreaReuseTime)
         pInfo->score = -pInfo->score;                 // consideration
      
      // if the cumulative area fraction exceed the threshold, then 
      // exclude this one from further consideration for a period of time
      else if ( pInfo->areaFracUsed >= m_minPlanAreaFrac )  // this one should be removed from
         {
         pInfo->score = -pInfo->score;                 // consideration
         pInfo->lastUsed = pContext->yearOfRun;
         }
      }

   // sort scores
   qsort(m_planAreaArray.GetData(), paCount, sizeof(INT_PTR), ComparePlanArea);
   
   // reset indexes
   for ( int i=0; i < paCount; i++ )
      {
      PLAN_AREA_INFO *pInfo = m_planAreaArray[ i ];
      pInfo->index = i;
      
      //if ( i < 10 )
      //   {
      //   CString msg;
      //   msg.Format( "PlanInfo: ID=%i, Rank=%i, AreaFr=%5.3f, lastUsed=%i, score=%f",
      //     pInfo->id, i, pInfo->areaFracUsed, pInfo->lastUsed, pInfo->score );
      //   Report::Log( msg );
      //   }
      }

   // put rank into IDU if in top plan areas
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int planArea = -1;
      pLayer->GetData(idu, m_colPlanArea, planArea);

      if ( planArea > 0 )
         {
         // populate planAreaScore col with ranking of the plan area
         // note that the array is sorted at this point, best=0, 2nd=1, etc
         PLAN_AREA_INFO *pInfo = NULL;
         BOOL found = m_planAreaMap.Lookup(planArea, pInfo);
         if ( ! found )
            {
            Report::ErrorMsg( "Error finding plan area ID");
            continue;
            }

         int rank = ( pInfo->score > 0 ) ? pInfo->index+1 : -1;
         UpdateIDU(pContext, idu, m_colPlanAreaScore, rank , ADD_DELTA);
         }  
      else
         {
         UpdateIDU(pContext, idu, m_colPlanAreaScore, -99, ADD_DELTA);  // no plan
         }
      }  // end of: for each IDU

   return true;
   }


int ComparePlanArea(const void *arg0, const void *arg1)
   {
   PLAN_AREA_INFO **pInfo0 = (PLAN_AREA_INFO**)arg0;
   PLAN_AREA_INFO **pInfo1 = (PLAN_AREA_INFO**)arg1;

   return (((*pInfo1)->score - (*pInfo0)->score) > 0) ? 1 : -1;
   }


bool COCNHProcess::ScoreAllocationAreasFire(EnvContext *pContext)
   {
   // reset plan area info
   int pafCount = (int)m_planAreaFireArray.GetSize();

   for (int i = 0; i < pafCount; i++)
      {
      PLAN_AREA_INFO *pInfo = m_planAreaFireArray[i];
      pInfo->area = 0;
      pInfo->score = 0;
      }

   // basic idea - iterate through 
   // iterate through idu shapefile
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      // planning areas are in "ALLOC_PLAN"
      // for each planning area, calculate area that satisfy the specified query
      // rank the scores and populate the top three planning area "PS_FIRE" with 
      // 1=top, 2=second, 3=third,...N=Nth all others -1.

      int planArea = -1;
      pLayer->GetData(idu, m_colPlanArea, planArea);

      // only count areas inside a plan area
      if ( planArea > 0 )
         {
         // is there already plan area info defined for this plan area?
         PLAN_AREA_INFO *pInfo = NULL;
         BOOL found = m_planAreaFireMap.Lookup(planArea, pInfo);
         if ( ! found )    // none found, so create one
            {
            Report::ErrorMsg( "Error finding plan area ID");
            continue;
            }

         ASSERT( pInfo != NULL );
         if ( m_pPAFQuery != NULL )
            {
            bool pass = false;
            m_pPAFQuery->Run( idu, pass );

            if ( pass )
               {
               float area;
               pLayer->GetData(idu, m_colArea, area);

               pInfo->score += area;
               pInfo->area += area;
               }
            }
         }
      }  // end of: for each IDU

   // plan info gathered, scale and sort.  We don't want a plan that:
   // has been used in the recent past (defined by 'm_minPlanAreaReuseTime' (years))
   // Note that a plan area is considered "used" if more than specified fraction ('m_minPlanAreaFrac')
   // of the plan area has been used (treated) in the recent past (defined by m_minPlanAreaReuseTime)

   for (int i = 0; i < pafCount; i++)
      {
      PLAN_AREA_INFO *pInfo = m_planAreaFireArray[i];
      //pInfo->score /= pInfo->area;   // area-weighted avg basal area - No need to normailize

      // have we used this one recently (meaning we've exceeded the area threshold recently?)
      // if so, set the score to a negative value
      if ((pContext->yearOfRun - pInfo->lastUsed) <= m_minPlanAreaFireReuseTime)
         pInfo->score = -pInfo->score;                 // consideration
      
      // if the cumulative area fraction exceed the threshold, then 
      // exclude this one from further consideration for a period of time
      else if ( pInfo->areaFracUsed >= m_minPlanAreaFireFrac )  // this one should be removed from
         {
         pInfo->score = -pInfo->score;                 // consideration
         pInfo->lastUsed = pContext->yearOfRun;
         }
      }

   qsort(m_planAreaFireArray.GetData(), pafCount, sizeof(INT_PTR), ComparePlanArea);

   // reset indexes
   for ( int i=0; i < pafCount; i++ )
      {
      PLAN_AREA_INFO *pInfo = m_planAreaFireArray[ i ];
      pInfo->index = i;
      
      if ( i < 10 )
         {
         CString msg;
         msg.Format( "PlanInfo: ID=%i, Rank=%i, AreaFr=%5.3f, lastUsed=%i, score=%f",
           pInfo->id, i, pInfo->areaFracUsed, pInfo->lastUsed, pInfo->score );
         Report::Log( msg );
         }
      }

   // put rank into IDU if in top plan areas
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int planArea = -1;
      pLayer->GetData(idu, m_colPlanArea, planArea);

      if ( planArea > 0 )
         {
         // populate planAreaScore col with ranking of the plan area
         // note that the array is sorted at this point, best=0, 2nd=1, etc
         PLAN_AREA_INFO *pInfo = NULL;
         BOOL found = m_planAreaFireMap.Lookup(planArea, pInfo);
         if ( ! found )
            {
            Report::ErrorMsg( "Error finding plan area ID");
            continue;
            }

         int rank = ( pInfo->score > 0 ) ? pInfo->index+1 : -1;
         UpdateIDU(pContext, idu, m_colPlanAreaScoreFire, rank , ADD_DELTA);
         }  
      else
         UpdateIDU(pContext, idu, m_colPlanAreaScoreFire, -99, ADD_DELTA);  // no plan
      }  // end of: for each IDU

   return true;
   }

bool COCNHProcess::CalculateFirewise(EnvContext *pContext)
   {
  
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;
   
   //temporarily constant for all IDUs (pers Comm Alan Ager via Erik White
   float burnProb = 0.002f;
   
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int nDU = -1;
      int wui = -1;
      int timeSinceFireWise;
      bool tsfwFlag = false;

      timeSinceFireWise = m_timeSinceFirewise[idu];

      pLayer->GetData( idu, m_colNDU, nDU);
      pLayer->GetData( idu, m_colWUI, wui );
      
      // home owners only
      if ( nDU > 0 && wui > 0 && timeSinceFireWise > 4 )
         {
         
         int fire5_500m = 0; // 1 if wildfire occured with 500m within past 5 years, else 0
         int fire5_10km = 0; // 1 if wildfire occured with 10km within past 5 years, else 0  
         int prescribedFire5_10km = 0;// 1 if prescribed fire occured with 10km within past 5 years, else 0
         int prescribedFire5_2km = 0;// 1 if prescribed fire occured with 2km within past 5 years, else 0
         float treePerHectare500m = 0.0f; // trees per hectare within 500m
         float iduAreaM2 = 0.0f; 
         
         pLayer->GetData( idu, m_colArea,  iduAreaM2);
         
         float iduAreaAcres = iduAreaM2 * ACRE_PER_M2;
         
         // Meant to be conditional flame lenght, for now potential flame length averaged within 1km buffer
         //float cfl = 0.0f; 
         float avePotentialFlameLen1000ft = 0.0f;

         pLayer->GetData( idu, m_colFire500, fire5_500m );
         pLayer->GetData( idu, m_colFire10000, fire5_10km );
         pLayer->GetData( idu, m_colPrescribedFire10000, prescribedFire5_10km );
         pLayer->GetData( idu, m_colPrescribedFire2000, prescribedFire5_2km );
         pLayer->GetData( idu, m_colAvePotentialFlameLength1000, avePotentialFlameLen1000ft );
         pLayer->GetData( idu, m_colTreesPerHect500, treePerHectare500m );
         
         float avePotentialFlameLen1000m = avePotentialFlameLen1000ft * M_PER_FT;

         float y = 0.735f + ( 209.235f * burnProb ) + ( 1.747f * fire5_10km ) + ( 1.322f * prescribedFire5_10km );
         float z = -5.556f + ( 0.390f * avePotentialFlameLen1000m ) + ( 0.006f * treePerHectare500m ) + ( 1.775f * fire5_10km ) + ( 1.775f * prescribedFire5_2km );

         float chanceWildfire = exp(y) / ( 1 + exp(y) );

         float chanceDamage = exp(z) / ( 1 + exp(z) );

         float x = 1.106f + ( 1.687f * fire5_500m ) + ( 2.154f * chanceWildfire * chanceDamage ); 

         float probabilityFirewise = exp(x) / ( 1 + exp(x) );

         float randNum = (float) m_rn.RandValue( 0., 1. );  //randum number between 0 and 1, uniform distribution

         if ( randNum < probabilityFirewise )
            {
            UpdateIDU( pContext, idu, m_colFireWise, 1, ADD_DELTA );
            m_timeSinceFirewise[idu] = 1;
            tsfwFlag = true;
            }
         else
            {
            UpdateIDU(pContext, idu, m_colFireWise, 0, ADD_DELTA);
            }
         }

         if ( tsfwFlag == false )
            m_timeSinceFirewise[idu] = timeSinceFireWise++;

      }  // for each IDU

   ::EnvApplyDeltaArray(pContext->pEnvModel);
   return true;
   }


// update PLAN_AREA_INFO's areaArray and areaFracUsed
int COCNHProcessPost2::UpdatePlanAreas( EnvContext *pContext )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   int paCount = (int) m_planAreaArray.GetSize();

   float *areaArray = new float[ paCount ];
   memset( areaArray, 0, paCount * sizeof( float ) );
   
   // for each IDU, look for harvests that occurred in this time step
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int planArea = -1;
      pLayer->GetData( idu, m_colPlanArea, planArea );

      // are we in a plan area?
      if ( planArea > 0 )
         {
         // yes, so see if a treatment happened
         int disturb = -1;
         pLayer->GetData( idu, m_colDisturb, disturb );

         if ( disturb == THIN_FROM_BELOW || disturb == PARTIAL_HARVEST || disturb == HARVEST )
            {
            PLAN_AREA_INFO *pInfo = NULL;
            BOOL found = m_planAreaMap.Lookup( planArea, pInfo );

            ASSERT( found );
            int index = pInfo->index;
            
            float area = 0;
            pLayer->GetData( idu, m_colArea, area );
            areaArray[ index ] += area;
            }
         }
      }

   // areas calculated for each plan area, add to queue
   for ( int i=0; i < paCount; i++ )
      {
      PLAN_AREA_INFO *pInfo = m_planAreaArray[ i ];

      int size = (int) pInfo->areaArray.GetSize();

      for ( int j = size-1; j > 0; j-- )
         pInfo->areaArray[ j ] = pInfo->areaArray[ j-1 ];

      pInfo->areaArray[ 0 ] = areaArray [ i ];

      // get total area in queue
      pInfo->areaFracUsed = 0;
      for ( int j=0; j < size; j++ )
         pInfo->areaFracUsed += pInfo->areaArray[ j ];

      pInfo->areaFracUsed /= pInfo->area;
      }

   delete [] areaArray;
   
   // update maps
   for ( MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int planArea = -1;
      pLayer->GetData( idu, m_colPlanArea, planArea );

      // are we in a plan area?
      if ( planArea > 0 )
         {
         PLAN_AREA_INFO *pInfo = NULL;
         BOOL found = m_planAreaMap.Lookup( planArea, pInfo );

         if ( pInfo->areaFracUsed > 0 )
            UpdateIDU(pContext, idu, m_colPlanAreaFr, pInfo->areaFracUsed, ADD_DELTA);
         else
            UpdateIDU(pContext, idu, m_colPlanAreaFr, -99, ADD_DELTA);
         }
      }

   //-------------------------------------------------------------------
   // repeat for fire
   //-------------------------------------------------------------------
   int pafCount = (int) m_planAreaFireArray.GetSize();

   areaArray = new float[ pafCount ];
   memset( areaArray, 0, pafCount * sizeof( float ) );
   
   // for each IDU, look for harvests that occurred in this time step
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int planArea = -1;
      pLayer->GetData( idu, m_colPlanArea, planArea );

      // are we in a plan area?
      if ( planArea > 0 )
         {
         // yes, so see if a treatment happened
         int disturb = -1;
         pLayer->GetData( idu, m_colDisturb, disturb );

        if ( disturb == PRESCRIBED_SURFACE_FIRE )  // a federal prescried fire
            {
            PLAN_AREA_INFO *pInfo = NULL;
            BOOL found = m_planAreaFireMap.Lookup( planArea, pInfo );

            ASSERT( found );
            int index = pInfo->index;
            
            float area = 0;
            pLayer->GetData( idu, m_colArea, area );
            areaArray[ index ] += area;
            }
         }
      }

   // areas calculated for each plan area, add to queue
   for ( int i=0; i < pafCount; i++ )
      {
      PLAN_AREA_INFO *pInfo = m_planAreaFireArray[ i ];

      int size = (int) pInfo->areaArray.GetSize();

      for ( int j = size-1; j > 0; j-- )
         pInfo->areaArray[ j ] = pInfo->areaArray[ j-1 ];

      pInfo->areaArray[ 0 ] = areaArray [ i ];

      // get total area in queue
      pInfo->areaFracUsed = 0;
      for ( int j=0; j < size; j++ )
         pInfo->areaFracUsed += pInfo->areaArray[ j ];

      pInfo->areaFracUsed /= pInfo->area;
      }

   delete [] areaArray;

   
   // update maps
   for ( MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int planArea = -1;
      pLayer->GetData( idu, m_colPlanArea, planArea );

      // are we in a plan area?
      if ( planArea > 0 )
         {
         PLAN_AREA_INFO *pInfo = NULL;
         BOOL found = m_planAreaFireMap.Lookup( planArea, pInfo );

         if ( pInfo->areaFracUsed > 0 )
            UpdateIDU(pContext, idu, m_colPlanAreaFireFr, pInfo->areaFracUsed, ADD_DELTA);
         else
            UpdateIDU(pContext, idu, m_colPlanAreaFireFr, -99, ADD_DELTA);
         }
      }

   return 1;
   }


static int calcFireEffectErrCount = 0;

int COCNHProcessPre2::CalcFireEffect(EnvContext *pContext, bool useAddDelta)
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   //
  // pLayer->SetColData( m_colFireKill, VData(0), true);   // m3/ha
   //

   // for each IDU, look for harvests that occurred in this time step
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = 0;
      pLayer->GetData(idu, m_colDisturb, disturb);

      if ( disturb == STAND_REPLACING_FIRE   // 23
        || disturb ==  LOW_SEVERITY_FIRE     //21
        || disturb == HIGH_SEVERITY_FIRE )   // 22
         {
         // we need to look at Previous VEGCLASS
         int priorVeg = 0;
         pLayer->GetData(idu, m_colPriorVeg, priorVeg);

         if (priorVeg > 0)
            {
            // we are in a post-harvest state, with defined prior veg
            //
            // 1. Get prior veg class values
            int region = 0;
            pLayer->GetData(idu, m_colRegion, region);

            int pvt = 0;
            pLayer->GetData(idu, m_colPVT, pvt);

            int vegClass = 0;
            pLayer->GetData(idu, m_colVegClass, vegClass);

            //float liveBiomass = 0;
            //float liveCarbon = 0;
            //float liveVolumeTotal = 0;
            float liveVolumeGe3 = 0.0f;
            float liveVolume_3_25 = 0;
            float liveVolume_25_50 = 0;
            float liveVolumeGe50 = 0;
            float liveVol = 0;

            pLayer->GetData(idu, m_colLVolm3ha,            liveVolumeGe3);
            pLayer->GetData(idu, m_colVPH_3_25,            liveVolume_3_25);
            pLayer->GetData(idu, m_colLVol25_50,         liveVolume_25_50);
            pLayer->GetData(idu, m_colLVolGe50,            liveVolumeGe50);
         
            //liveVol = liveVolume_25_50  + liveVolumeGe50;
            liveVol = liveVolumeGe3 - liveVolume_3_25;
            
            float priorLiveVol = m_priorLiveVolumeGe3 - m_priorLiveVolume_3_25;
              
            float fireKillVol = priorLiveVol - liveVol;

               if ( fireKillVol < 0.0f && calcFireEffectErrCount < 10 )
                  {
                  CString msg;
                  msg.Format("CalcFireKill: Negative volume killed: Region=%d  Pvt=%d  disturb=%i  PriorVegclass=%i  CurrentVegClass=%i  priorVol=%f  currentVol=%f  disturbVol=%f   \n", region, pvt, disturb, priorVeg, vegClass, priorLiveVol, liveVol, fireKillVol );
                  Report::LogWarning(msg);
   
                  calcFireEffectErrCount++;
                  }

               // column name: FIREKILL, replaced with PFSAWVOL that has a biomass decay function
               //UpdateIDU(pContext, idu, m_colFireKill, fireKillVol, useAddDelta ? ADD_DELTA : SET_DATA );
               
            }  // end of: if ( priorVeg > 0 )
         }  // end of: if (disturb)
      }  // end of: for each IDU

   return 1;
   }


static int calcHarvBiomassErrCount = 0;

int COCNHProcessPost2::CalcHarvestBiomass(EnvContext *pContext, bool useAddDelta)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;
   
   float totalHarvestCheck = 1.0f;

   // for each IDU, look for harvests that occurred in this time step
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = 0;
      pLayer->GetData(idu, m_colDisturb, disturb);
      int owner = 1;
      pLayer->GetData(idu, m_colOwner, owner);
      float salvageSawTimberHarvestVol = 0.0f;
      float sawTimberHarvVol = 0.0f;
      float totalSawTimberHarvVol = 0.0f;

      float totalTimberHarvVol = 0.0f;
      float timberHarvVol = 0.0f;
      float iduAreaM2 = 0.0f;      

      pLayer->GetData(idu, m_colArea, iduAreaM2);
      float iduAreaHa = iduAreaM2 * HA_PER_M2;

      if (disturb == HARVEST                // 1
         || disturb == THINNING               // 2
         || disturb == PARTIAL_HARVEST        // 3
         || disturb == SELECTION_HARVEST      // 6
         || disturb == SALVAGE_HARVEST        // 52
         || disturb == THIN_FROM_BELOW        // 55
         || disturb == PARTIAL_HARVEST_LIGHT  // 56
         || disturb == PARTIAL_HARVEST_HIGH) // 57
         {
         // harvest - we need to look at Previous VEGCLASS
         int priorVeg = 0;
         pLayer->GetData(idu, m_colPriorVeg, priorVeg);
       
           // we are in a post-harvest state, with defined prior veg
           //
           // 1. Get prior veg class values
           int region = 0;
           pLayer->GetData(idu, m_colRegion, region);

           int pvt = 0;
           pLayer->GetData(idu, m_colPVT, pvt);

           int vegClass = 0;
           pLayer->GetData(idu, m_colVegClass, vegClass);

           VEGKEY vegKey(vegClass, region, pvt);
           int luRow = -1;
           BOOL found = m_vegLookupTable.Lookup(vegKey.GetKey(), luRow);

           float liveBiomass = 0;
           float liveCarbon = 0;
           float liveVolumeGe3 = 0;
           float liveVolume_3_25 = 0;
           float liveVolume_25_50 = 0;
           float liveVolumeGe50 = 0;
         int   owner = -1;
      
         pLayer->GetData(idu, m_colLBioMgha,   liveBiomass);
         pLayer->GetData(idu, m_colLCMgha,   liveCarbon);   
         pLayer->GetData(idu, m_colLVolm3ha,   liveVolumeGe3);
         pLayer->GetData(idu, m_colVPH_3_25,   liveVolume_3_25);      
         pLayer->GetData(idu, m_colLVol25_50,liveVolume_25_50);
         pLayer->GetData(idu, m_colLVolGe50,   liveVolumeGe50);
         pLayer->GetData(idu, m_colLVolGe50,   liveVolumeGe50);
         pLayer->GetData(idu, m_colOwner,    owner);            

         if ( disturb == SALVAGE_HARVEST )
            {
            float pfSawVol = 0.0f;
            float availableSawTimberVol52 = 0.0f;
            float deadTotalVolume = 0;
            float deadTotalCarbon = 0;

            pLayer->GetData(idu, m_colPFSawVol, pfSawVol); // m3/ha
            pLayer->GetData(idu, m_colAvailVolSH, availableSawTimberVol52);
            pLayer->GetData(idu, m_colDVolm3ha,deadTotalVolume) ;// m3/ha            
            //pLayer->GetData(idu, m_colPFDeadBio, deadBiomass); // m3/ha
            //pLayer->GetData(idu, m_colPFDeadCarb, deadCarbon); // m3/ha

            salvageSawTimberHarvestVol = availableSawTimberVol52; // m3
            
            pfSawVol -= ( salvageSawTimberHarvestVol / iduAreaHa ); // m3/ha
            
            deadTotalVolume -= ( salvageSawTimberHarvestVol / iduAreaHa ); // m3/ha
            //deadCarbon -= ( salvageSawTimberHarvestVol * 0.5f / iduAreaHa );
            
            float liveTotalVolume = m_priorLiveVolumeGe3IDUArray[idu];
            
            float totalVolume = liveTotalVolume + deadTotalVolume;
                           
            UpdateIDU(pContext, idu, m_colDVolm3ha, deadTotalVolume, useAddDelta ? ADD_DELTA : SET_DATA);
            UpdateIDU(pContext, idu, m_colPFSawHarvestVol, salvageSawTimberHarvestVol, useAddDelta ? ADD_DELTA : SET_DATA);
            UpdateIDU(pContext, idu, m_colTVolm3ha, totalVolume, useAddDelta ? ADD_DELTA : SET_DATA);
            //UpdateIDU(pContext, idu, m_colPFDeadBio, deadBiomass, useAddDelta ? ADD_DELTA : SET_DATA);
            //UpdateIDU(pContext, idu, m_colPFDeadCarb, deadCarbon, useAddDelta ? ADD_DELTA : SET_DATA);

            // feds are going to remove 50%, all others 90%
            float potentiallyRemoved = (owner == 1) ? 0.5f : 0.9f;
            UpdateIDU(pContext, idu, m_colAvailVolSH, pfSawVol * potentiallyRemoved, useAddDelta ? ADD_DELTA : SET_DATA);
            }
         else // not salvage, all other harvest types
            {

            float availableTFB = 0.0f;
            float availablePH = 0.0f;
            float availablePHH = 0.0f;
            float availableRH = 0.0f;

            pLayer->GetData( idu, m_colAvailVolTFB, availableTFB ); // m3   DISTURB=55
            pLayer->GetData( idu, m_colAvailVolPH, availablePH ); // m3   DISTURB=3
            pLayer->GetData( idu, m_colAvailVolPHH, availablePHH ); // m3   DISTURB=57
            pLayer->GetData( idu, m_colAvailVolRH, availableRH ); // m3   DISTURB=1

            switch ( disturb )
               {
               // saw timber volume
               case HARVEST:         
                  sawTimberHarvVol =  availableRH;// m3   
                  timberHarvVol = sawTimberHarvVol + ( m_priorLiveVolume_3_25IDUArray[idu] * m_percentPreTransStructAvailable1 );// m3               
                  break;
               case PARTIAL_HARVEST:
                  sawTimberHarvVol = availablePH;// m3
                  timberHarvVol = sawTimberHarvVol + ( m_priorLiveVolume_3_25IDUArray[idu] * m_percentPreTransStructAvailable3 );// m3
                  break;
               case THIN_FROM_BELOW:
                  sawTimberHarvVol = availableTFB;// m3
                  timberHarvVol = sawTimberHarvVol + ( m_priorLiveVolume_3_25IDUArray[idu] * m_percentPreTransStructAvailable55 );// m3
                   break;
               case PARTIAL_HARVEST_HIGH: 
                  sawTimberHarvVol = availablePHH;// m3
                  timberHarvVol = sawTimberHarvVol + ( m_priorLiveVolume_3_25IDUArray[idu] * m_percentPreTransStructAvailable57 );// m3
                  break;
               }
                                    
            if ( timberHarvVol < 0.0f && disturb != 51 && calcHarvBiomassErrCount < 10 )
               {
               CString msg;
               msg.Format("CalcHarvestBiomass: Negative volume harvested: Region=%d  Pvt=%d  disturb=%i  PriorVegclass=%i  CurrentVegClass=%i  priorVol=%f  currentVol=%f  disturbVol=%f   \n", region, pvt, disturb, priorVeg, vegClass, m_priorLiveVolumeGe3, liveVolumeGe3, timberHarvVol);
               Report::LogWarning(msg);

               calcHarvBiomassErrCount++;
               }
            }
         
         float deadTotalVolume = 0.0f;
         pLayer->GetData( idu, m_colDVolm3ha, deadTotalVolume);
         
         float liveTotalVolume = m_priorLiveVolumeGe3IDUArray[idu];
         
         liveTotalVolume -= timberHarvVol / iduAreaHa;

         float totalVolume = liveTotalVolume + deadTotalVolume;

         m_priorLiveBiomassIDUArray[idu] = m_priorLiveBiomass;
         m_priorLiveCarbonIDUArray[idu] = m_priorLiveCarbon;

         totalSawTimberHarvVol = sawTimberHarvVol ;
         totalTimberHarvVol    = timberHarvVol + salvageSawTimberHarvestVol;

         UpdateIDU(pContext, idu, m_colSawTimberHarvVolume, totalSawTimberHarvVol, useAddDelta ? ADD_DELTA : SET_DATA);
         UpdateIDU(pContext, idu, m_colHarvestVol, totalTimberHarvVol, useAddDelta ? ADD_DELTA : SET_DATA);         
         UpdateIDU(pContext, idu, m_colTVolm3ha, totalVolume, useAddDelta ? ADD_DELTA : SET_DATA);
         UpdateIDU(pContext, idu, m_colLVolm3ha, liveTotalVolume, useAddDelta ? ADD_DELTA : SET_DATA);
         
         if ( owner == 1  )
            totalHarvestCheck += totalTimberHarvVol;
         //UpdateIDU(pContext, idu, m_colTBioMgha, totalBiomass, useAddDelta ? ADD_DELTA : SET_DATA); // live + dead
         //UpdateIDU(pContext, idu, m_colTCMgha, totalCarbon, useAddDelta ? ADD_DELTA : SET_DATA);//live +dead

         }  // if harvest
      }  // end of: for each IDU
   
      ::EnvApplyDeltaArray(pContext->pEnvModel);
      return 1;
   }

static int decayPostDisturbBioErrCount = 0;

int COCNHProcessPre2::DecayDeadResiduals(EnvContext *pContext, bool useAddDelta)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   // for each IDU, look for post-disturbance state that need to be decayed.
   // Basic idea: Immediately after a fire, we want to create dead biomass
   // and carbon from the burned prior veg. We know a disturbnace occurred
   // by looking at the DISTURB col in the IDU.  A positive value implies that
   // a disturbance occured previously in this time step. The PRIORVEG column
   // should at that point contain the prior vegclass upon which the newly dead
   // biomass is determined.
   //
   // Post-fire dead biomass and dead carbon are tracked in their own columns in the 
   // IDU coverage: PF_DEADBIO, PF_DEADCARB

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = 0;
      float iduAreaM2 = 0.0f;
      int vegClass = -999;
      int pvt = -999;
      int region = -999;
      int transType = 0;
      
      pLayer->GetData(idu, m_colArea, iduAreaM2);
      pLayer->GetData(idu, m_colVegClass, vegClass);
      pLayer->GetData(idu, m_colPVT, pvt);
      pLayer->GetData(idu, m_colRegion, region);
      pLayer->GetData(idu, m_colDisturb, disturb);
      pLayer->GetData( idu, m_colVegTranType, transType );
      
      // Salvage logging: feds are going to remove 50%, all others 90%,  add this back in and decay
      int owner = -1;
      pLayer->GetData(idu, m_colOwner, owner);
      float potentiallyRemoved = ( owner == 1 ) ? 0.5f : 0.9f;
      
      float iduAreaHa = iduAreaM2 * HA_PER_M2;

      VEGKEY key(vegClass, region, pvt);
      int luRow = -1;
      BOOL found = m_vegLookupTable.Lookup(key.GetKey(), luRow);

      int seenNoTransBefore = m_accountedForNoTransThisStep[idu];
      int seenSuccessionalTransBefore = m_accountedForSuccessionThisStep[idu];
      // did a fire just occur?  Then convert live biomass to dead biomass
      if (disturb == STAND_REPLACING_FIRE   // 23
         || disturb == HIGH_SEVERITY_FIRE     // 22
         || disturb == LOW_SEVERITY_FIRE      // 21 
         || disturb == SURFACE_FIRE)         // 20
         {
 
         // look up prior veg class variables from veg lookup table
         //float priorLiveBiomass = m_vegLookupTable.GetColData(luRow, LIVEBIO_MGHA);
         //float priorLiveCarbon = m_vegLookupTable.GetColData(luRow, LIVEC_MGHA);
         
         // for post-fire disturbances, track dead carbon, biomass
         float lossFraction = 0.0f;
         float percentPreTransStruct = 0.0f;
         switch (disturb)
            {
            // fire disturbances that occured this timestep
            case SURFACE_FIRE:         
               {
               lossFraction = 0.03f;   
               percentPreTransStruct = m_percentOfLivePassedOn20;
               break;
               }
            case LOW_SEVERITY_FIRE:
               {
               lossFraction = 0.07f;
               percentPreTransStruct = m_percentOfLivePassedOn21;
               break;
               }
            case HIGH_SEVERITY_FIRE:   
               break;
            case STAND_REPLACING_FIRE: 
               {
               lossFraction = 0.08f;
               percentPreTransStruct = m_percentOfLivePassedOn23;
               break;
               }
            }

         //float sawTimberVol = 0.0f;
         //float deadBio = 0.0f;
         //float liveBio = 0.0f;
         //float deadVolume = 0.0f;
         //float deadCarbon = 0.0f;

         if ((m_priorLiveBiomassIDUArray[idu] < 0 || m_priorLiveCarbonIDUArray[idu] < 0) && decayPostDisturbBioErrCount < 10)
            {
            CString msg;
            msg.Format("DecayDeadResiduals() - negative prior biomass found: vegClass=%i, pvt=%i, region=%i, biomass=%f.", pvt, region, m_priorLiveBiomassIDUArray[idu]);
            Report::LogWarning(msg);
            m_priorLiveBiomassIDUArray[idu] = 0;
            m_priorLiveCarbonIDUArray[idu] = 0;
            decayPostDisturbBioErrCount++;
            }
         
         float priorSawTimberVolume = m_priorLiveVolumeSawTimberIDUArray[idu]; // m3/ha
         float priorLiveVolume = m_priorLiveVolumeGe3IDUArray[idu]; // m3/ha
         float priorLiveBiomass = m_priorLiveBiomassIDUArray[idu]; // Mg/ha
         float priorLiveCarbon = m_priorLiveCarbonIDUArray[idu];

         float postFireSawTimberVolumeDead = priorSawTimberVolume * (1.0f - percentPreTransStruct) * (1.0f - lossFraction);
         float deadTotalBiomass = priorLiveBiomass * (1.0f - percentPreTransStruct) * (1.0f - lossFraction);
         float deadTotalVolume = priorLiveVolume * (1.0f - percentPreTransStruct) * (1.0f - lossFraction);
         float deadTotalCarbon = deadTotalBiomass * 0.5f;

         float currentLiveBiomass = 0.0f;
         pLayer->GetData(idu, m_colLBioMgha, currentLiveBiomass);
         
         float currentLiveVolumeGe3 = 0.0f;
         pLayer->GetData(idu, m_colLVolm3ha, currentLiveVolumeGe3);

         float totalBiomass = currentLiveBiomass + deadTotalBiomass;
         float totalCarbon = totalBiomass * 0.5f;
         float totalVolume = currentLiveVolumeGe3 + deadTotalVolume;

         UpdateIDU(pContext, idu, m_colPFSawVol, postFireSawTimberVolumeDead, useAddDelta ? ADD_DELTA : SET_DATA);
         //UpdateIDU(pContext, idu, m_colPFDeadBio, deadTotalBiomass, useAddDelta ? ADD_DELTA : SET_DATA);
         //UpdateIDU(pContext, idu, m_colPFDeadCarb, deadTotalCarbon, useAddDelta ? ADD_DELTA : SET_DATA); // 50% of bio is carbon      
         UpdateIDU(pContext, idu, m_colDVolm3ha, deadTotalVolume, useAddDelta ? ADD_DELTA : SET_DATA);
         UpdateIDU(pContext, idu, m_colDCMgha, deadTotalCarbon, useAddDelta ? ADD_DELTA : SET_DATA);
         UpdateIDU(pContext, idu, m_colDBioMgha, deadTotalBiomass, useAddDelta ? ADD_DELTA : SET_DATA);
         UpdateIDU(pContext, idu, m_colTBioMgha, totalBiomass, useAddDelta ? ADD_DELTA : SET_DATA); // live + dead
         UpdateIDU(pContext, idu, m_colTCMgha, totalCarbon, useAddDelta ? ADD_DELTA : SET_DATA);//live +dead
         UpdateIDU(pContext, idu, m_colTVolm3ha, totalVolume, useAddDelta ? ADD_DELTA : SET_DATA);

         float availableSawTimberVol52 = priorSawTimberVolume * m_percentPreTransStructAvailable52 * potentiallyRemoved * iduAreaHa;
         
         int owner = -1;
         pLayer->GetData(idu, m_colOwner, owner);
         
         UpdateIDU(pContext, idu, m_colAvailVolSH, availableSawTimberVol52, useAddDelta ? ADD_DELTA : SET_DATA);
         }  // end of: if disturb generated dead carbon
      else if ( seenNoTransBefore == 0 && transType == 0 )  // no VEGCLASS transition
         {
         m_accountedForNoTransThisStep[idu] = 1;

         // a fire did not happen in this time step.  in this case, decay any 
         // dead post-fire carbon that exists based on time since fire
         int timeSinceFire;
         pLayer->GetData(idu, m_colTSF, timeSinceFire);

         if ( timeSinceFire < 40 )  // TSF get updated after this call. 
            {
            
            /*float deadPFBiomass = 0;
            pLayer->GetData(idu, m_colPFDeadBio, deadPFBiomass);*/

            float deadSawTimberVol = 0;
            pLayer->GetData(idu, m_colPFSawVol, deadSawTimberVol);

            float availableSawTimberVol52 = 0;
            pLayer->GetData(idu, m_colAvailVolSH, availableSawTimberVol52);
            
            // get the total available, potentially removed is dependent on current owner
            availableSawTimberVol52 /= potentiallyRemoved;

            float deadTotalVolume = 0.0f;
            pLayer->GetData(idu, m_colDVolm3ha, deadTotalVolume);
            
            float deadTotalBiomass = 0.0f;
            pLayer->GetData(idu, m_colDBioMgha, deadTotalBiomass);
            
            int colIDUIndex = pLayer->GetFieldCol("IDU_INDEX");
            int iduIndex = 0;
            pLayer->GetData(idu, colIDUIndex,iduIndex);
                                    
            float k = 0.05f;
            float decay = (float)exp(-k);
            deadTotalBiomass *= decay;
            deadTotalVolume *= decay; 
            availableSawTimberVol52 *= decay;
            deadSawTimberVol *= decay;            
            float deadTotalCarbon = deadTotalBiomass * 0.5f;
            
            // For IDUs where wild fire left biomass to decay, dead values should never be less than the lookup table values for this IDU
            if ( found && luRow >= 0 && disturb <= -20 && disturb >= -23 )
               {
               float lookupTableDeadVolume = m_vegLookupTable.GetColData( luRow, TOTDEADVOL_M3HA );
               
               // the decayed values should never be less than the lookup table values for this IDU
               if (  deadTotalVolume < lookupTableDeadVolume )  
                  {
                  deadTotalVolume = lookupTableDeadVolume;
                  deadTotalBiomass = m_vegLookupTable.GetColData(luRow, TOTDEADBIO_MGHA);
                  deadTotalCarbon = deadTotalBiomass * 0.5f;                  
                  }
               }

            float currentLiveBiomass = 0.0f;
            pLayer->GetData(idu, m_colLBioMgha, currentLiveBiomass);

            float currentLiveVolumeGe3 = 0.0f;
            pLayer->GetData(idu, m_colLVolm3ha, currentLiveVolumeGe3);

            float totalBiomass = currentLiveBiomass + deadTotalBiomass;
            float totalCarbon = (currentLiveBiomass * 0.5f) + deadTotalCarbon;
            float totalVolume = currentLiveVolumeGe3 + deadTotalVolume;

            //UpdateIDU(pContext, idu, m_colPFDeadBio, deadPFBiomass, useAddDelta ? ADD_DELTA : SET_DATA);
            UpdateIDU(pContext, idu, m_colDCMgha, deadTotalCarbon, useAddDelta ? ADD_DELTA : SET_DATA);
            UpdateIDU(pContext, idu, m_colPFSawVol, deadSawTimberVol, useAddDelta ? ADD_DELTA : SET_DATA);
            UpdateIDU(pContext, idu, m_colDVolm3ha, deadTotalVolume , useAddDelta ? ADD_DELTA : SET_DATA);
            //UpdateIDU(pContext, idu, m_colDCMgha, deadPFCarbon, useAddDelta ? ADD_DELTA : SET_DATA);
            UpdateIDU(pContext, idu, m_colDBioMgha, deadTotalBiomass, useAddDelta ? ADD_DELTA : SET_DATA);            
            UpdateIDU(pContext, idu, m_colTBioMgha, totalBiomass, useAddDelta ? ADD_DELTA : SET_DATA); // live + dead
            UpdateIDU(pContext, idu, m_colTCMgha, totalCarbon, useAddDelta ? ADD_DELTA : SET_DATA);//live + dead
            UpdateIDU(pContext, idu, m_colTVolm3ha, totalVolume, useAddDelta ? ADD_DELTA : SET_DATA);// live + dead

            UpdateIDU(pContext, idu, m_colAvailVolSH, availableSawTimberVol52 * potentiallyRemoved, useAddDelta ? ADD_DELTA : SET_DATA);
            }  // end of: if time since fire > 0 && < 40
         else 
            {         
            //UpdateIDU(pContext, idu, m_colPFDeadBio, VData(0.0f), useAddDelta ? ADD_DELTA : SET_DATA);
            //UpdateIDU(pContext, idu, m_colPFDeadCarb, VData(0.0f), useAddDelta ? ADD_DELTA : SET_DATA);
            UpdateIDU(pContext, idu, m_colAvailVolSH, VData(0.0f), useAddDelta ? ADD_DELTA : SET_DATA);
            }
         }  // end of: else (post-fire)
      }  // end of: for each IDU

      ::EnvApplyDeltaArray(pContext->pEnvModel);
      return 1;
   }


// NOTE - STILL NEED TO INCORPORATE THIS
bool COCNHProcess::PopulateLulc(EnvContext *pContext, bool useDelta)
   {
   /*
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   int iduCount = pLayer->GetRecordCount();
   int ugaCount = (int) m_ugaArray.GetSize();

   // for each IDU, set LULC based on DUs for res classes
   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
   {
   // set res lulc
   int zone = 0;
   pLayer->GetData( idu, m_colZone, zone );

   int nDU = m_nDUs[ idu ];

   int lulc = -1;
   if ( IsResidential( zone ) )
   {
   if ( IsUrban( zone ) )
   {
   float area;
   pLayer->GetData( idu, m_colArea, area );
   area /= M2_PER_ACRE;   // convert to acres

   float duPerAc = nDU / area;

   if ( duPerAc > 0.5f && duPerAc <= 4 )       lulc = 1;
   else if ( duPerAc > 4 && duPerAc <= 9 )     lulc = 2;
   else if ( duPerAc > 9 && duPerAc <= 16 )    lulc = 3;
   else if ( duPerAc > 16 )                    lulc = 4;
   }

   //else // rural
   //   {
   // currently, no rural classes
   //   }
   }

   if ( lulc > 0 )
   {
   if ( useDelta )
   {
   int _lulc = 0;
   pLayer->GetData( idu, m_colVegClass, _lulc );

   if ( lulc != _lulc )
   AddDelta( pContext, idu, m_colVegClass, lulc );
   }
   else
   pLayer->SetData(  idu, m_colVegClass, lulc );
   }
   }  // end of: MapLayer iterator
   */
   return true;
   }

/*
NOTE:  NEED TO UPDATE THE CODES BELOW - THESE ARE FROM SWCNH!!!!!

bool COCNHProcess::IsResidential( int zone )
{
switch( zone )
{
//case 1:  // C2: Neighborhood Commercial
//case 2:  // C3: Commercial
//case 3:  // E25: Exclusive Farm Use (25 Acre Minimum)
//case 4:  // E30: Exclusive Farm Use (30 Acre Minimum)
//case 5:  // E40: Exclusive Farm Use (40 Acre Minimum)
//case 6:  // E60: Exclusive Farm Use (60 Acre Minimum)
//case 7:  // F1: Non-Impacted Forest
//case 8:  // F2: Impacted Forest
//case 9:  // M2: Light Industrial
//case 10:  // ML: Marginal Lands
//case 12:  // NR: Natural Resource
//case 13:  // PF: Public Facility
//case 14:  // PR: Park and Recreation
//case 15:  // QM: Quarry and Mining Operations
//case 16:  // RC: Rural Commercial
//case 17:  // RI: Rural Industrial
//case 18:  // RPF: Rural Public Facility
case 19:  // RR1: Rural Residential (1 Acre Minimum)
case 20:  // RR10: Rural Residential (10 Acre Minimum)
case 21:  // RR10-NRES: Non Resource (10 Acre Minimum)
case 22:  // RR2: Rural Residential (2 Acre Minimum)
case 23:  // RR5: Rural Residential (5 Acre Minimum)
case 24:  // RR5-NRES: Non Resource (5 Acre Minimum)
//case 25:  // SG: Sand, Gravel and Rock Products
case 100:   // UR-Low: 0-4 DU/ac
case 101:   // UR-Med: 4-9 DU/ac
case 102:   // UR-High: 9-16 DU/ac
case 103:   // UR-VHigh: > 16 DU/ac
//case 110:   // UR_CI: commercial/industrial
case 120:   // UR_MU: mixed use
case 130:   // RR-Cl: Cluster Developemnt
return true;
}

return false;
}


bool COCNHProcess::IsCommInd( int zone )
{
switch( zone )
{
case 1:  // C2: Neighborhood Commercial
case 2:  // C3: Commercial
//case 3:  // E25: Exclusive Farm Use (25 Acre Minimum)
//case 4:  // E30: Exclusive Farm Use (30 Acre Minimum)
//case 5:  // E40: Exclusive Farm Use (40 Acre Minimum)
//case 6:  // E60: Exclusive Farm Use (60 Acre Minimum)
//case 7:  // F1: Non-Impacted Forest
//case 8:  // F2: Impacted Forest
case 9:  // M2: Light Industrial
//case 10:  // ML: Marginal Lands
//case 12:  // NR: Natural Resource
//case 13:  // PF: Public Facility
//case 14:  // PR: Park and Recreation
case 15:  // QM: Quarry and Mining Operations
case 16:  // RC: Rural Commercial
case 17:  // RI: Rural Industrial
//case 18:  // RPF: Rural Public Facility
//case 19:  // RR1: Rural Residential (1 Acre Minimum)
//case 20:  // RR10: Rural Residential (10 Acre Minimum)
//case 21:  // RR10-NRES: Non Resource (10 Acre Minimum)
//case 22:  // RR2: Rural Residential (2 Acre Minimum)
//case 23:  // RR5: Rural Residential (5 Acre Minimum)
//case 24:  // RR5-NRES: Non Resource (5 Acre Minimum)
case 25:    // SG: Sand, Gravel and Rock Products
//case 100:   // UR-Low: 0-4 DU/ac
//case 101:   // UR-Med: 4-9 DU/ac
//case 102:   // UR-High: 9-16 DU/ac
//case 103:   // UR-VHigh: > 16 DU/ac
case 110:   // UR_CI: commercial/industrial
//case 120:   // UR_MU: mixed use
//case 130:   // RR-Cl: Cluster Developemnt
return true;
}

return false;
}

bool COCNHProcess::IsUrban( int zone )
{
switch( zone )
{
case 1:  // C2: Neighborhood Commercial
case 2:  // C3: Commercial
//case 3:  // E25: Exclusive Farm Use (25 Acre Minimum)
//case 4:  // E30: Exclusive Farm Use (30 Acre Minimum)
//case 5:  // E40: Exclusive Farm Use (40 Acre Minimum)
//case 6:  // E60: Exclusive Farm Use (60 Acre Minimum)
//case 7:  // F1: Non-Impacted Forest
//case 8:  // F2: Impacted Forest
//case 9:  // M2: Light Industrial
//case 10:  // ML: Marginal Lands
//case 12:  // NR: Natural Resource
//case 13:  // PF: Public Facility
//case 14:  // PR: Park and Recreation
//case 15:  // QM: Quarry and Mining Operations
//case 16:  // RC: Rural Commercial
//case 17:  // RI: Rural Industrial
//case 18:  // RPF: Rural Public Facility
//case 19:  // RR1: Rural Residential (1 Acre Minimum)
//case 20:  // RR10: Rural Residential (10 Acre Minimum)
//case 21:  // RR10-NRES: Non Resource (10 Acre Minimum)
//case 22:  // RR2: Rural Residential (2 Acre Minimum)
//case 23:  // RR5: Rural Residential (5 Acre Minimum)
//case 24:  // RR5-NRES: Non Resource (5 Acre Minimum)
//case 25:  // SG: Sand, Gravel and Rock Products
case 100:   // UR-Low: 0-4 DU/ac
case 101:   // UR-Med: 4-9 DU/ac
case 102:   // UR-High: 9-16 DU/ac
case 103:   // UR-VHigh: > 16 DU/ac
case 110:   // UR_CI: commercial/industrial
case 120:   // UR_MU: mixed use
//case 130:   // RR-Cl: Cluster Developemnt
return true;
}

return false;
}


bool COCNHProcess::IsRuralRes( int zone )
{
switch( zone )
{
//case 1:  // C2: Neighborhood Commercial
//case 2:  // C3: Commercial
//case 3:  // E25: Exclusive Farm Use (25 Acre Minimum)
//case 4:  // E30: Exclusive Farm Use (30 Acre Minimum)
//case 5:  // E40: Exclusive Farm Use (40 Acre Minimum)
//case 6:  // E60: Exclusive Farm Use (60 Acre Minimum)
//case 7:  // F1: Non-Impacted Forest
//case 8:  // F2: Impacted Forest
//case 9:  // M2: Light Industrial
//case 10:  // ML: Marginal Lands
//case 12:  // NR: Natural Resource
//case 13:  // PF: Public Facility
//case 14:  // PR: Park and Recreation
//case 15:  // QM: Quarry and Mining Operations
//case 16:  // RC: Rural Commercial
//case 17:  // RI: Rural Industrial
//case 18:  // RPF: Rural Public Facility
case 19:  // RR1: Rural Residential (1 Acre Minimum)
case 20:  // RR10: Rural Residential (10 Acre Minimum)
case 21:  // RR10-NRES: Non Resource (10 Acre Minimum)
case 22:  // RR2: Rural Residential (2 Acre Minimum)
case 23:  // RR5: Rural Residential (5 Acre Minimum)
case 24:  // RR5-NRES: Non Resource (5 Acre Minimum)
//case 25:  // SG: Sand, Gravel and Rock Products
//case 100:   // UR-Low: 0-4 DU/ac
//case 101:   // UR-Med: 4-9 DU/ac
//case 102:   // UR-High: 9-16 DU/ac
//case 103:   // UR-VHigh: > 16 DU/ac
//case 110:   // UR_CI: commercial/industrial
//case 120:   // UR_MU: mixed use
case 130:   // RR-Cl: Cluster Developemnt
return true;
}

return false;
}

*/



bool COCNHSetTSD::Run(EnvContext *pContext)
   {
   MapLayer *pLayer = (MapLayer*)pContext->pMapLayer;

   pLayer->m_readOnly = false;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int disturb = 0;
      if (pLayer->GetData(idu, m_colDisturb, disturb))
         {
         if (disturb > 0)
            UpdateIDU(pContext, idu, m_colTSD, 0, ADD_DELTA);

         else if (disturb <= 0)
            {
            int tsd = 0;
            pLayer->GetData(idu, m_colTSD, tsd);

            UpdateIDU(pContext, idu, m_colTSD, tsd + 1, ADD_DELTA);
            }
         }
      }

   pLayer->m_readOnly = true;

   return TRUE;
   }


/*
BOOL COCNHSetDisturb::Run( EnvContext *pContext )
{
MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
{
int disturb = 0;
if ( pLayer->GetData( idu, m_colDisturb, disturb ) && disturb > 0 )
UpdateIDU( pContext, idu, m_colDisturb, -disturb, ADD_DELTA );
}

return TRUE;
}
*/