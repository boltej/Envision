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

You should have received a copy of the GNU General Public License
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

#include <algorithm>
#include <exception>
#include <typeinfo>
#include <stdexcept>

#include <MatlabDataArray.hpp>
//#include <MatlabEngine.hpp>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const float G = 9.81f;                  // gravity m/sec*sec
const float MHW = 2.1f;                  // NAVD88 mean high water meters 
const float MLLW = -0.46f;               // NAVD88 mean lower low water meters (t Westport)
const float EXLW_MLLW = 1.1f;            // NAVD88 Extreme low water below MLLW meters 

float PolicyInfo::m_budgetAllocFrac = 0.5f;


FILE* fpPolicySummary = nullptr;


TWLDATAFILE ChronicHazards::m_datafile;
COSTS ChronicHazards::m_costs;



//=====================================================================================================
// ChronicHazards
//=====================================================================================================


extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new ChronicHazards; }


ChronicHazards::ChronicHazards(void)
	: EnvModelProcess(0, 0)
{ }

ChronicHazards::~ChronicHazards(void)
{
	if (m_pIduErodedGridLkUp != nullptr) delete m_pIduErodedGridLkUp;
	if (m_pIduBuildingLkUp != nullptr) delete m_pIduBuildingLkUp;
	if (m_pIduInfraLkUp != nullptr) delete m_pIduInfraLkUp;

	if (m_pIduFloodedGridLkUp != nullptr) delete m_pIduFloodedGridLkUp;
	if (m_pRoadFloodedGridLkUp != nullptr) delete m_pRoadFloodedGridLkUp;
	//if (m_pBldgFloodedGridLkUp != nullptr) delete m_pBldgFloodedGridLkUp;
	//if (m_pInfraFloodedGridLkUp != nullptr) delete m_pInfraFloodedGridLkup;


	//if (m_pTransectLUT != nullptr) delete m_pTransectLUT;
	if (m_pProfileLUT != nullptr) delete m_pProfileLUT;
	//if (m_pSAngleLUT != nullptr) delete m_pSAngleLUT;
	//if (m_pInletLUT != nullptr) delete m_pInletLUT;
	//if (m_pBayLUT != nullptr) delete m_pBayLUT;
}


bool ChronicHazards::Init(EnvContext* pEnvContext, LPCTSTR initStr)
{
	// read config files and set internal variables based on that
	LoadXml(initStr);

	// get Map pointer
	m_pMap = pEnvContext->pMapLayer->GetMapPtr();

	// ****** Load Required MapLayers of the Map ****** //

	// IDU Layer
	m_pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

	// Roads Layer
	m_pRoadLayer = m_pMap->GetLayer("Roads");

	// Buildings Layer
	m_pBldgLayer = m_pMap->GetLayer("Buildings");
	if (m_pBldgLayer != nullptr)
		m_numBldgs = m_pBldgLayer->GetRowCount();

	// Infrastructure Layer
	m_pInfraLayer = m_pMap->GetLayer("Infrastructure Locations");

	// Original Dune line layer
	m_pDuneLayer = m_pMap->GetLayer("Dune Points");

	// Tidal Bathymetry
	//m_pBayTraceLayer = m_pMap->GetLayer("Bay Trace Pts");
	//m_pBayTraceLayer->Hide();

	// Check and store relevant columns (in IDU coverage)
	CheckCol(m_pIDULayer, m_colArea, "Area", TYPE_FLOAT, CC_MUST_EXIST);

	// initial submodels
	InitDuneModel(pEnvContext);  // checks DUNELINE coverage fields, populates DUNEINDEX, TRANS_ID;
								 // associates SWAN transects and cross-shore profiles to shoreline pts
	InitTWLModel(pEnvContext);

	if (m_runFlags & CH_MODEL_FLOODING)
		InitFloodingModel(pEnvContext);

	if (m_runFlags & CH_MODEL_EROSION)
		InitErosionModel(pEnvContext);

	if (m_runFlags & CH_MODEL_BUILDINGS)
		InitBldgModel(pEnvContext);

	if (m_runFlags & CH_MODEL_INFRASTRUCTURE)
		InitInfrastructureModel(pEnvContext);

	if (m_runFlags & CH_MODEL_POLICY)
		InitPolicyInfo(pEnvContext);

	// define input (scenario) variables
	AddInputVar("Climate Scenario ID", m_climateScenarioID, "0=BaseSLR, 1=LowSLR, 2=MedSLR, 3=HighSLR, 4=WorstCaseSLR");
	//AddInputVar("Track Habitat", m_runEelgrassModel, "0=Off, 1=On");

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

	// still need the following?
	//CString filename;
	//CString fullFilename = PathManager::GetPath(PM_PROJECT_DIR);
	//
	//if (m_cityDir.IsEmpty())
	//   fullFilename += "PolyGridMaps\\";
	//else
	//   fullFilename += m_cityDir + "\\PolyGridMaps\\";
	//
	//CString msg("Looking for polygrid lookups in ");
	//msg += fullFilename;
	//Report::LogInfo(msg);


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


	// Setup outputs for annual statistics
	this->AddOutputVar("Mean TWL (m)", m_meanTWL, "");
	this->AddOutputVar("% Beach Accessibility", m_avgAccess, "");
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

	if (m_runFlags && CH_MODEL_BUILDINGS)
	{
	}

	if (m_runFlags && CH_MODEL_INFRASTRUCTURE)
	{
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
	}

	if (m_runFlags && CH_MODEL_POLICY)
	{
		// Flooded and Eroded Buildings counted in Reporter
		/*  this->AddOutputVar("Flooded Buildings", m_floodedBldgCount, "");
		this->AddOutputVar("Eroded Buildings", m_erodedBldgCount, "");*/

		this->AddOutputVar("Policy Cost Allocation Frac", PolicyInfo::m_budgetAllocFrac, "");
		this->AddOutputVar("Policy Cost Summary", &m_policyCostData, "");

		this->AddOutputVar("Number of Buildings Removed From Hazard Zone (100-yr)", m_noBldgsRemovedFromHazardZone, "");
	}


	//  //Set all values to zero
	//  //m_BPSMaintenance = 0;
	//  //m_armorCost = 0;
	//  //m_nourishCostBPS = 0;
	//  //m_nourishVolumeBPS = 0;
	//  //m_randIndex += 1; // add to xml, only want when we are running probabalistically

	return TRUE;
}


bool ChronicHazards::InitTWLModel(EnvContext* pEnvContext)
{
	if ((m_runFlags & CH_MODEL_TWL) == 0)
		return false;

	Map* pMap = pEnvContext->pMapLayer->GetMapPtr();
	m_pTransectLayer = pMap->GetLayer(m_transectLayer);

	// Load Northing SWAN Lookup'
	//CString transPath = m_twlInputPath + m_transectPointsLookupFile;
	//m_pTransectLUT = new DDataObj;
	//m_numTransects = m_pTransectLUT->ReadAscii(transPath);
	m_numTransects = m_pTransectLayer->GetRowCount();

	CheckCol(m_pDuneLayer, m_colDLMvAvgTWL, "MvAvgTWL", TYPE_FLOAT, CC_AUTOADD | TYPE_FLOAT);
	CheckCol(m_pDuneLayer, m_colDLMvMaxTWL, "MvMaxTWL", TYPE_FLOAT, CC_AUTOADD | TYPE_FLOAT);

	CheckCol(m_pDuneLayer, m_colDLTransID, "TRANS_ID", TYPE_INT, CC_AUTOADD | TYPE_LONG);
	PopulateClosestIndex(m_pDuneLayer, "TRANS_ID", m_pTransectLayer->m_pData);  //     m_pTransectLUT);

	for (int i = 0; i < m_numTransects; i++)
	{
		int tID = -1;
		m_pTransectLayer->GetData(i, 0, tID);
		m_transectMap[tID] = i;   // key= transectID, value = index in Transect layer
	}

	LoadRBFs(); // Load/create radial basis functions for TWL model
	return true;
}


bool ChronicHazards::InitFloodingModel(EnvContext* pEnvContext)
{
	if ((m_runFlags & CH_MODEL_FLOODING) == 0)
		return false;

	using namespace matlab::engine;
	// Start MATLAB engine synchronously
	this->m_matlabPtr = startMATLAB();

	matlab::data::ArrayFactory factory;
	auto sfincs_home = factory.createCharArray((LPCTSTR)m_sfincsHome); // TODO: Remove hardcoding of SFICS home
	m_matlabPtr->feval(u"addpath", sfincs_home);  // moved to matlab side

	//// Elevation
	//m_pElevationGrid = m_pMap->GetLayer("Elevation");
	//if (m_pElevationGrid == nullptr)
	//   return false;

	//m_pElevationGrid->Hide();
	//m_pElevationGrid->GetCellSize(m_elevCellWidth, m_elevCellHeight);
	//
	//// Bay Bathymetry
	//m_pBayBathyGrid = m_pMap->GetLayer("Bay Bathy");
	//if (m_pBayBathyGrid != nullptr)
	//   m_pBayBathyGrid->Hide();
	//
	//// Bates Model Mask
	//m_pManningMaskGrid = m_pMap->GetLayer("Manning Mask");
	//m_pManningMaskGrid->Hide();
	//m_pFloodedGrid = m_pMap->CloneLayer(*m_pElevationGrid);
	//m_pFloodedGrid->m_name = "Flooded Grid";
	//m_pCumFloodedGrid = m_pMap->CloneLayer(*m_pElevationGrid);
	//m_pCumFloodedGrid->m_name = "Cumulative Flooded Grid";

	//// Tidal Bathymetry
	//m_pTidalBathyGrid = m_pMap->GetLayer("Tidal Bathy");
	//if (m_pTidalBathyGrid != nullptr)
	//   m_pTidalBathyGrid->Hide();

	// change to MUST_EXIST
	//CheckCol(m_pIDULayer, m_colIDUBaseFloodElevation, "STATIC_BFE", TYPE_FLOAT, CC_MUST_EXIST);
	/////CheckCol(m_pIDULayer, m_colIDUFloodZone, "FLD_ZONE", TYPE_STRING, CC_AUTOADD);
	//CheckCol(m_pIDULayer, m_colIDUFloodZoneCode, "FLD_HZ", TYPE_INT, CC_MUST_EXIST);
	/////CheckCol(m_pIDULayer, m_colLandValue, "LANDVALUE", TYPE_LONG, CC_MUST_EXIST);
	////////CheckCol(m_pIDULayer, m_colIDUFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
	CheckCol(m_pIDULayer, m_colIDUFracFlooded, "FRACFLOOD", TYPE_FLOAT, CC_AUTOADD);
	m_pIDULayer->SetColData(m_colIDUFracFlooded, VData(0.0f), true);

	// Bay Trace columns
	////////if (m_pBayTraceLayer)
	////////   {
	////////   CheckCol(m_pBayTraceLayer, m_colBayTraceIndex, "TRACEINDX", TYPE_INT, CC_MUST_EXIST);
	////////   CheckCol(m_pBayTraceLayer, m_colBayYrMaxTWL, "MAX_TWL_YR", TYPE_FLOAT, CC_AUTOADD);
	////////   m_pBayTraceLayer->SetColData(m_colBayYrMaxTWL, VData(0.0f), true);
	////////   CheckCol(m_pBayTraceLayer, m_colBayYrAvgTWL, "AVG_TWL_YR", TYPE_FLOAT, CC_AUTOADD);
	////////   m_pBayTraceLayer->SetColData(m_colBayYrAvgTWL, VData(0.0f), true);
	////////   CheckCol(m_pBayTraceLayer, m_colBayYrMaxSWL, "MAX_SWL_YR", TYPE_FLOAT, CC_AUTOADD);
	////////   m_pBayTraceLayer->SetColData(m_colBayYrMaxSWL, VData(0.0f), true);
	////////   CheckCol(m_pBayTraceLayer, m_colBayYrAvgSWL, "AVG_SWL_YR", TYPE_FLOAT, CC_AUTOADD);
	////////   m_pBayTraceLayer->SetColData(m_colBayYrAvgSWL, VData(0.0f), true);
	////////   }

	// Read in the Bay coverage if running Bay Flooding Model
	/*bool runBayFloodModel = (m_runBayFloodModel == 1) ? true : false;
	if (runBayFloodModel)
	   {
	   CString bayTraceFile = nullptr;
	   if (CString(m_cityDir).IsEmpty())
	   bayTraceFile = "BayPts.csv";
	   else
	   bayTraceFile = "BayPts_" + m_cityDir + ".csv";

	   PathManager::FindPath(bayTraceFile, fullPath);
	   m_pInletLUT = new DDataObj;
	   m_numBayPts = m_pInletLUT->ReadAscii(fullPath);
	   }*/


	   // Read in the Bay coverage if running Bay Flooding Model
	   /*bool runBayFloodModel = (m_runBayFloodModel == 1) ? true : false;
	   if (runBayFloodModel)
	   {
	   PathManager::FindPath("BayPts.csv", fullPath);
	   m_pInletLUT = new DDataObj;
	   m_numBayPts = m_pInletLUT->ReadAscii(fullPath);
	   }*/

	   // Associate Moving Windows for Erosion and Flooding Frequency to a building
	   // Set year of safest site for buildings equal the start year of the run
	for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
	{
		m_floodBldgFreqArray.Add(new MovingWindow(m_windowLengthFloodHzrd));
		m_floodInfraFreqArray.Add(new MovingWindow(m_windowLengthFloodHzrd));
		// m_pBldgLayer->SetData(bldg, m_colBldgSafeSiteYr, pEnvContext->startYear);
	}

	return true;
}

bool ChronicHazards::InitErosionModel(EnvContext* pEnvContext)
{
	if ((m_runFlags & CH_MODEL_EROSION) == 0)
		return false;

	////////// 1/0 field if IDU was annually flooded or eroded
	////////CheckCol(m_pIDULayer, m_colIDUEroded, "ERODED", TYPE_INT, CC_AUTOADD);

	//CheckCol(m_pDuneLayer, m_colDLDuneIndex, "DUNEINDEX", TYPE_INT, CC_AUTOADD | TYPE_INT);

	// Load Northing Profile Lookup (includes shoreface slope lookup for each profile)
	CString profilePath = m_erosionInputPath + m_bathyLookupFile;
	m_pProfileLUT = new DDataObj;

	m_numProfiles = m_pProfileLUT->ReadAscii(profilePath);
	for (int i = 0; i < m_pProfileLUT->GetRowCount(); i++)
		m_profileMap[m_pProfileLUT->GetAsInt(0, i)] = i;

	CheckCol(m_pDuneLayer, m_colDLProfileID, "PROFILE_ID", TYPE_INT, CC_AUTOADD);
	PopulateClosestIndex(m_pDuneLayer, "PROFILE_ID", m_pProfileLUT);

	// Read in cross shore profiles (bathymetry) for each transect
	for (int i = 0; i < m_numProfiles; i++)
	{
		DDataObj* pBathyData = new DDataObj;

		bool success = true;

		// first read in bathy (cross-shore profiles) files
		CString bathyPath = m_erosionInputPath + "bathy\\" + m_bathyMask;
		CString extension;
		extension.Format("_%i.csv", i);
		CString bathyFile = bathyPath + extension;

		CString msg("Processing bathy input file ");
		msg += bathyFile;
		Report::StatusMsg(msg);

		// series of Bathymetry lookup tables
		int numBathyRows = pBathyData->ReadAscii(bathyFile);
		if (numBathyRows < 0)
			success = false;

		if (success)
			this->m_BathyDataArray.Add(pBathyData);
		else
			delete pBathyData;
	}  // end, we have read in all files required for erosion models

	CString msg;
	msg.Format("Processed %i cross shore profiles", (int)m_BathyDataArray.GetSize());
	Report::Log(msg);

	//srand(time(nullptr));

	// Initialize Moving Window Dune line statistics data structures
	// Each dune pt has a its own set of moving windows
	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		MovingWindow* ipdyMovingWindow = new MovingWindow(m_windowLengthTWL);
		m_IDPYArray.Add(ipdyMovingWindow);

		MovingWindow* twlMovingWindow = new MovingWindow(m_windowLengthTWL);
		m_TWLArray.Add(twlMovingWindow);

		MovingWindow* eroKDMovingWindow = new MovingWindow(2);
		m_eroKDArray.Add(eroKDMovingWindow);
	}

	//// Associate Moving Windows for Erosion and Flooding frequency to critical infrastructure layer
	//for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
	//   {
	//   m_eroInfraFreqArray.Add(new MovingWindow(m_windowLengthEroHzrd));
	//   }

	// Associate Moving Windows for Erosion and Flooding (?) frequency to bldg layer
	for (MapLayer::Iterator bldgIndex = m_pBldgLayer->Begin(); bldgIndex < m_pBldgLayer->End(); bldgIndex++)
	{
		m_eroBldgFreqArray.Add(new MovingWindow(m_windowLengthEroHzrd));
	}

	// generate eroded grid
	const float erodedGridCellSize = 10;
	if (m_pErodedGrid == nullptr)
	{
		// create an elevation grid based on dune point extents
		REAL xMin, xMax, yMin, yMax;
		m_pDuneLayer->GetExtents(xMin, xMax, yMin, yMax);

		int rows = int(((yMax - yMin) + (2 * erodedGridCellSize)) / erodedGridCellSize);
		int cols = int(((xMax - xMin) + (2 * erodedGridCellSize)) / erodedGridCellSize);

		m_pErodedGrid = m_pIDULayer->GetMapPtr()->CreateGrid(rows, cols, xMin - erodedGridCellSize, yMin - erodedGridCellSize, erodedGridCellSize, 0, DOT_INT, false);
		m_pErodedGrid->m_name = "Eroded Grid";
	}

	return true;
}


bool ChronicHazards::InitBldgModel(EnvContext* pEnvContext)
{
	if ((m_runFlags & CH_MODEL_BUILDINGS) == 0)
		return false;

	CString ppmDir = PathManager::GetPath(PM_IDU_DIR);
	ppmDir += "PolyGridMaps\\";

	//// Building layers columns
	if (m_pBldgLayer)
	{
		CheckCol(m_pBldgLayer, m_colBldgIDUIndex, "IDUINDEX", TYPE_INT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgIDUIndex, VData(-1), true);
		CheckCol(m_pBldgLayer, m_colBldgNDU, "NDU", TYPE_INT, CC_AUTOADD);
		CheckCol(m_pBldgLayer, m_colBldgNEWDU, "NEWDU", TYPE_INT, CC_AUTOADD);
		CheckCol(m_pBldgLayer, m_colBldgBFE, "BFE", TYPE_FLOAT, CC_AUTOADD);
		//CheckCol(m_pBldgLayer, m_colBldgFloodHZ, "FLDHZ", TYPE_INT, CC_AUTOADD);
		//m_pBldgLayer->SetColData(m_colBldgFloodHZ, VData(0), true);

		CheckCol(m_pBldgLayer, m_colBldgFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgFlooded, VData(0), true);
		CheckCol(m_pBldgLayer, m_colBldgEroded, "ERODED", TYPE_INT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgEroded, VData(0), true);

		CheckCol(m_pBldgLayer, m_colBldgRemoveYr, "REMOVE_YR", TYPE_INT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgRemoveYr, VData(0), true);

		CheckCol(m_pBldgLayer, m_colBldgSafeSiteYr, "SFESITE_YR", TYPE_INT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgSafeSiteYr, VData(0), true);

		CheckCol(m_pBldgLayer, m_colBldgBFEYr, "BFE_YR", TYPE_INT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgBFEYr, VData(0), true);
		CheckCol(m_pBldgLayer, m_colBldgSafeSite, "SAFE_SITE", TYPE_INT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgSafeSite, VData(1), true);
		//CheckCol(m_pBldgLayer, m_colBldgFloodYr, "FLOOD_YR", TYPE_INT, CC_AUTOADD);
		//m_pBldgLayer->SetColData(m_colBldgFloodYr, VData(0), true);
		CheckCol(m_pBldgLayer, m_colBldgEroFreq, "ERO_FREQ", TYPE_FLOAT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgEroFreq, VData(0.0f), true);
		CheckCol(m_pBldgLayer, m_colBldgFloodFreq, "FLOOD_FREQ", TYPE_FLOAT, CC_AUTOADD);
		m_pBldgLayer->SetColData(m_colBldgFloodFreq, VData(0.0f), true);

		//CheckCol(m_pBldgLayer, m_colBldgCityCode, "CITY_CODE", TYPE_INT, CC_AUTOADD);
		CheckCol(m_pBldgLayer, m_colBldgValue, "BLDGVALUE", TYPE_INT, CC_MUST_EXIST);

		// build polygrid lookup
		Report::StatusMsg("Building polypoint mapper IDUBuildings.ppm");
		CString iduBldgFile = ppmDir + "IDUBuildings.ppm";
		m_pIduBuildingLkUp = new PolyPointMapper(m_pIDULayer, m_pBldgLayer, iduBldgFile);  // creates map if file note found, otherwse, just loads it
		//   m_pIduBuildingLkup->BuildIndex();

	}


	if (m_pErodedGrid != nullptr)
	{
		//  Load from file if found, otherwise create PolyGridMap for IDUs based upon cell sizes of the Erosion Grid Layer
		Report::StatusMsg("Building polygrid lookup IDUGridLkup.pgm");
		CString IDUPglFile = ppmDir + "IDUErodedGridLkup.pgm";
		m_pIduErodedGridLkUp = new PolyGridMapper(m_pIDULayer, m_pErodedGrid, 1, IDUPglFile);

		// Check built lookup matches grid 
		int numRows = m_pErodedGrid->GetRowCount();
		int numCols = m_pErodedGrid->GetColCount();

		if ((m_pIduErodedGridLkUp->GetNumGridCols() != numCols) || (m_pIduErodedGridLkUp->GetNumGridRows() != numRows))
			Report::ErrorMsg("ChronicHazards: IDU/Eroded Grid Lookup row or column mismatch, rebuild (delete) polygrid lookup");
		else if (m_pIduErodedGridLkUp->GetNumPolys() != m_pIDULayer->GetPolygonCount())
			Report::ErrorMsg("ChronicHazards: IDU/Eroded Grid Lookup IDU count mismatch, rebuild (delete) polygrid lookup");

		if (m_pRoadLayer != nullptr)
		{
			// Load from file if found, otherwise create PolyGridMap for Roads based upon cell sizes of the Elevation Layer
			CString roadsPglFile = ppmDir + "RoadErodedGridLkup.pgm";
			m_pRoadErodedGridLkUp = new PolyGridMapper(m_pRoadLayer, m_pErodedGrid, 1, roadsPglFile);

			// Check built lookup matches grid
			if ((m_pRoadErodedGridLkUp->GetNumGridCols() != numCols) || (m_pRoadErodedGridLkUp->GetNumGridRows() != numRows))
				Report::ErrorMsg("ChronicHazards: Road Grid Lookup row or column mismatch, rebuild (delete) polygrid lookup");
			/*else if (pRoadErodedGridLkUp->GetNumPolys() != m_pRoadLayer->GetPolygonCount())
			Report::ErrorMsg("ChronicHazards: Road Grid Lookup road count mismatch, rebuild lookup");*/
		}
	}

	return true;
}


bool ChronicHazards::InitInfrastructureModel(EnvContext* pEnvContext)
{
	if ((m_runFlags & CH_MODEL_INFRASTRUCTURE) == 0)
		return false;

	CString msg;

	// Load from file if found, otherwise create PolyPointMap for IDUs 
	CString ppmDir = PathManager::GetPath(PM_IDU_DIR);
	ppmDir += "PolyGridMaps\\";

	CheckCol(m_pIDULayer, m_colIDUNorthingBottom, "Bottom", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pIDULayer, m_colIDUNorthingTop, "Top", TYPE_DOUBLE, CC_MUST_EXIST);

	/////// temporary - set top, bottom fields /////
	for (int idu = 0; idu < m_pIDULayer->GetPolygonCount(); idu++)
	{
		Poly* pPoly = m_pIDULayer->GetPolygon(idu);
		float top = -999999999.0f, bottom = 999999999.0f;

		for (int j = 0; j < pPoly->GetVertexCount(); j++)
		{
			Vertex& v = pPoly->GetVertex(j);

			if (v.y > top)
				top = (float)v.y;
			if (v.y < bottom)
				bottom = (float)v.y;
		}
		m_pIDULayer->SetData(idu, m_colIDUNorthingTop, (int)top);
		m_pIDULayer->SetData(idu, m_colIDUNorthingBottom, (int)bottom);
	}


	//CheckCol(m_pIDULayer, m_colIDUZone, "ZONE_CODE", TYPE_INT, CC_MUST_EXIST);
	CheckCol(m_pIDULayer, m_colIDUPopDensity, "PopDens", TYPE_DOUBLE, CC_MUST_EXIST);
	//CheckCol(m_pIDULayer, m_colIDUPopulation, "Population", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pIDULayer, m_colIDUTsunamiHazardZone, "TSUNAMI379", TYPE_INT, CC_MUST_EXIST);

	//CheckCol(m_pIDULayer, m_colIDUPopCap, "POP_CAP", TYPE_FLOAT, CC_AUTOADD);

	CheckCol(m_pIDULayer, m_colIDUNDU, "NDU", TYPE_INT, CC_AUTOADD);
	CheckCol(m_pIDULayer, m_colIDUNEWDU, "NewDU", TYPE_INT, CC_AUTOADD);

	// IDU Attributes for Safest Site Policy
	//CheckCol(m_pIDULayer, m_colIDUMinElevation, "MIN_ELEV", TYPE_FLOAT, CC_AUTOADD);
	CheckCol(m_pIDULayer, m_colIDUMaxElevation, "MAX_ELEV", TYPE_FLOAT, CC_AUTOADD);

	// These can be calculated once in Init by turning on the findSafeSiteCell flag in GHC_Hazards.xml
	// They are the DEM cell location of the highest elevation in an IDU
	CheckCol(m_pIDULayer, m_colIDUSafestSiteRow, "SAFE_ROW", TYPE_FLOAT, CC_AUTOADD);
	CheckCol(m_pIDULayer, m_colIDUSafestSiteCol, "SAFE_COL", TYPE_FLOAT, CC_AUTOADD);

	// 1/0 Field to indicate Safe Site is available, 0 if taken  // not implemented = 0 if closer to coast, to avoid construction of new buildings
	CheckCol(m_pIDULayer, m_colIDUSafeSite, "SAFE_SITE", TYPE_INT, CC_AUTOADD);
	m_pIDULayer->SetColData(m_colIDUSafeSite, VData(1), true);

	CheckCol(m_pIDULayer, m_colIDURemoveBldgYr, "REMBLDGYR", TYPE_INT, CC_AUTOADD);
	m_pIDULayer->SetColData(m_colIDURemoveBldgYr, VData(0), true);

	////////CheckCol(m_pIDULayer, m_colEasementYear, "EASEMNT_YR", TYPE_INT, CC_AUTOADD);
	////////m_pIDULayer->SetColData(m_colEasementYear, VData(0.0f), true);

	// Road Layer columns
	if (m_pRoadLayer)
	{
		CheckCol(m_pRoadLayer, m_colRoadFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
		m_pRoadLayer->SetColData(m_colRoadFlooded, VData(0), true);
		//CheckCol(m_pRoadLayer, m_colRoadType, "TYPE", TYPE_INT, CC_MUST_EXIST);
		//CheckCol(m_pRoadLayer, m_colRoadLength, "LENGTH", TYPE_DOUBLE, CC_MUST_EXIST);
	}
	//


	//// Infrastructure Layer columns
	if (m_pInfraLayer)
	{
		CheckCol(m_pInfraLayer, m_colInfraCritical, "CRITICAL", TYPE_INT, CC_MUST_EXIST);
		CheckCol(m_pInfraLayer, m_colInfraDuneIndex, "DUNEINDEX", TYPE_INT, CC_AUTOADD);
		m_pInfraLayer->SetColData(m_colInfraDuneIndex, VData(-1), true);
		CheckCol(m_pInfraLayer, m_colInfraFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
		m_pInfraLayer->SetColData(m_colInfraFlooded, VData(0), true);
		CheckCol(m_pInfraLayer, m_colInfraEroded, "ERODED", TYPE_INT, CC_AUTOADD);
		m_pInfraLayer->SetColData(m_colInfraEroded, VData(0), true);
		CheckCol(m_pInfraLayer, m_colInfraBFE, "BFE", TYPE_FLOAT, CC_AUTOADD);
		m_pInfraLayer->SetColData(m_colInfraBFE, VData(0.0), true);
		CheckCol(m_pInfraLayer, m_colInfraBFEYr, "BFE_YR", TYPE_INT, CC_AUTOADD);
		m_pInfraLayer->SetColData(m_colInfraBFEYr, VData(0), true);
		CheckCol(m_pInfraLayer, m_colInfraFloodYr, "FLOOD_YR", TYPE_INT, CC_AUTOADD);
		m_pInfraLayer->SetColData(m_colInfraFloodYr, VData(0), true);
		CheckCol(m_pInfraLayer, m_colInfraValue, "InfraValue", TYPE_FLOAT, CC_MUST_EXIST);

		// Load from file if found, otherwise create PolyPointMap for IDUs 
		Report::StatusMsg("Building polypoint mapper IDUInfrastructure.ppm");
		CString iduInfraFile = ppmDir + "IDUInfrastructure.ppm";
		m_pIduInfraLkUp = new PolyPointMapper(m_pIDULayer, m_pInfraLayer, iduInfraFile);
		//m_pPolygonInfraPointLkup->BuildIndex();
	}

	//   SetInfrastructureValues();
	// 
	// 
	// 

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

			int numCells = m_pIduErodedGridLkUp->GetGridPtCntForPoly(idu);

			if (numCells != 0)
			{
				ROW_COL* indices = new ROW_COL[numCells];
				float* values = new float[numCells];

				m_pIduErodedGridLkUp->GetGridPtNdxsForPoly(idu, indices);

				for (int i = 0; i < numCells; i++)
				{
					float value = 0.0f;
					m_pErodedGrid->GetData(indices[i].row, indices[i].col, value);
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

			m_pIDULayer->SetData(idu, m_colIDUMaxElevation, maxElevation);
			m_pIDULayer->SetData(idu, m_colIDUSafestSiteRow, safestSiteRow);
			m_pIDULayer->SetData(idu, m_colIDUSafestSiteCol, safestSiteCol);
		}
	}

	//FindClosestDunePtToBldg(pEnvContext);
	if (m_runFlags & CH_MODEL_FLOODING || m_runFlags & CH_MODEL_EROSION)
	{
		//FindProtectedBldgs();  only need to run this once to populate file (must save)
	}

	return true;
}


bool ChronicHazards::InitDuneModel(EnvContext* pEnvContext)
{
	//These attributes come from LIDAR and other data sets in the original shape file
	// MUST_EXIST 
	CheckCol(m_pDuneLayer, m_colDLTranSlope, "TANB", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLDuneToe, "DUNETOE", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLDuneCrest, "DUNECREST", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLDuneHeel, "DUNEHEEL", TYPE_FLOAT, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLFrontSlope, "FRONTSLOPE", TYPE_FLOAT, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLBackSlope, "BACKSLOPE", TYPE_FLOAT, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLDuneWidth, "DUNEWIDTH", TYPE_FLOAT, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLBeachWidth, "BEACHWIDTH", TYPE_FLOAT, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLNorthing, "NORTHING", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLEastingToe, "EASTNGTOE", TYPE_DOUBLE, CC_MUST_EXIST);
	//CheckCol(m_pDuneLayer, m_colDLNorthingCrest, "NORTHNGCR", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLEastingCrest, "EASTNGCR", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLEastingShoreline, "EASTNGSH", TYPE_DOUBLE, CC_AUTOADD);
	CheckCol(m_pDuneLayer, m_colDLEastingMHW, "EASTNGMHW", TYPE_DOUBLE, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLShorelineChange, "SCR", TYPE_FLOAT, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLBeachType, "BEACHTYPE", TYPE_INT, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLGammaRough, "GAMMAROUGH", TYPE_FLOAT, CC_MUST_EXIST);
	CheckCol(m_pDuneLayer, m_colDLBruunSlope, "BruunSlope", TYPE_FLOAT, CC_MUST_EXIST);
	//CheckCol(m_pDuneLayer, m_colDLScomp, "COMPSLOPE", TYPE_FLOAT, CC_AUTOADD);

	CheckCol(m_pDuneLayer, m_colDLDuneIndex, "DUNEINDEX", TYPE_LONG, CC_AUTOADD | TYPE_LONG);
	for (int i = 0; i < m_pDuneLayer->GetRowCount(); i++)
		m_pDuneLayer->SetData(i, m_colDLDuneIndex, i);


	////////// short term shoreline change rate
	//////////  CheckCol(m_pDuneLayer, m_colEPR, "EPR", TYPE_FLOAT, CC_AUTOADD);

	///////// Shoreline locations to tally specific metrics or set differnt actions
	///////CheckCol(m_pDuneLayer, m_colDLLittCell, "LOC", TYPE_INT, CC_AUTOADD);

	// These attributes need values to be determined
	CheckCol(m_pDuneLayer, m_colDLShorelineAngle, "SHOREANGLE", TYPE_FLOAT, CC_MUST_EXIST);

	////////CheckCol(m_pDuneLayer, m_colGammaBerm, "GAMMABERM", TYPE_INT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colGammaBerm, VData(1.0f), true);

	//   CheckCol(m_pDuneLayer, m_colEastingMHW, "EASTINGMHW", TYPE_DOUBLE, CC_AUTOADD);
	CheckCol(m_pDuneLayer, m_colDLLongitudeToe, "LONGITUDE", TYPE_DOUBLE, CC_AUTOADD);
	CheckCol(m_pDuneLayer, m_colDLLatitudeToe, "LATITUDE", TYPE_DOUBLE, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colOrigBeachType, "ORBCHTYPE", TYPE_INT, CC_AUTOADD);
	////////
	////////CheckCol(m_pDuneLayer, m_colPrevSlope, "PREVTANB", TYPE_DOUBLE, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colPrevDuneCr, "PREVDUNECR", TYPE_DOUBLE, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colPrevRow, "PREVROW", TYPE_INT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colPrevCol, "PREVCOL", TYPE_INT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colDLEastingCrestProfile, "PEASTINGCR", TYPE_DOUBLE, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLEastingCrestProfile, VData(0.0), true);
	////////
	////////// These attributes are filled in on the fly from the Lookup tables to connect up the various layers
	////////
	////////// Index to lookup 
	////////CheckCol(m_pDuneLayer, m_colTranIndex, "TRANINDX", TYPE_INT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colProfileIndex, "PROFINDX", TYPE_INT, CC_AUTOADD);

	// Bruun Rule Slope (measured MHW - depth at 25m contour)
	CheckCol(m_pDuneLayer, m_colDLShoreface, "SHOREFACE", TYPE_FLOAT, CC_AUTOADD);

	// Foreshore Slope (measured in cross-shore profile within +- 0.5m vertically of MHW)
	CheckCol(m_pDuneLayer, m_colDLForeshore, "FORESHORE", TYPE_FLOAT, CC_AUTOADD);

	// Index of protected building
	CheckCol(m_pDuneLayer, m_colDLDuneBldgIndex, "BLDGINDX", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLDuneBldgIndex, VData(-1), true);
	// Distance (m) to protected building
	CheckCol(m_pDuneLayer, m_colDLDuneBldgDist, "BLDGDIST", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLDuneBldgDist, VData(5000.0f), true);


	// ******   These attributes are calculated during an annual run and represent the annual maximum or average ******
	CheckCol(m_pDuneLayer, m_colDLYrMaxSetup, "MAXSETUP", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrMaxSetup, VData(0.0), true);
	CheckCol(m_pDuneLayer, m_colDLYrMaxIGSwash, "MAXIGSWASH", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrMaxIGSwash, VData(0.0), true);
	CheckCol(m_pDuneLayer, m_colDLYrMaxIncSwash, "MXINCSWASH", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrMaxIncSwash, VData(0.0), true);
	CheckCol(m_pDuneLayer, m_colDLYrMaxSwash, "MAXSWASH", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrMaxSwash, VData(0.0), true);
	CheckCol(m_pDuneLayer, m_colDLYrMaxRunup, "MAXRUNUP", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrMaxRunup, VData(0.0f), true);

	// Yearly Maxmimum Total Water Level (TWL)
	CheckCol(m_pDuneLayer, m_colDLYrMaxTWL, "MAX_TWL", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrMaxTWL, VData(0.0f), true);
	CheckCol(m_pDuneLayer, m_colDLYrFMaxTWL, "MAX_FTWL", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrFMaxTWL, VData(0.0f), true);
	CheckCol(m_pDuneLayer, m_colDLYrAvgTWL, "AVG_TWL", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrAvgTWL, VData(0.0f), true);

	////////// Moving Average of TWL over a designated window
	////////CheckCol(m_pDuneLayer, m_colMvAvgTWL, "MVAVG_TWL", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colMvAvgTWL, VData(0.0f), true);
	////////
	////////// Maximum TWL within a designated moving window
	////////CheckCol(m_pDuneLayer, m_colMvMaxTWL, "MVMAX_TWL", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colMvMaxTWL, VData(0.0f), true);
	////////
	////////// Annual Data corresponding to the wave characteristics which have the yearly maximum TWL
	CheckCol(m_pDuneLayer, m_colDLWPeriod, "WPERIOD", TYPE_FLOAT, CC_AUTOADD);
	CheckCol(m_pDuneLayer, m_colDLWHeight, "WHEIGHT", TYPE_FLOAT, CC_AUTOADD);
	CheckCol(m_pDuneLayer, m_colDLWDirection, "WDIRECTION", TYPE_FLOAT, CC_AUTOADD);
	CheckCol(m_pDuneLayer, m_colDLHdeep, "HDEEP", TYPE_FLOAT, CC_AUTOADD);

	// Day of Year (doy) which has the yearly Maximum TWL
	CheckCol(m_pDuneLayer, m_colDLYrMaxDoy, "MAXDOY", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrMaxDoy, VData(0), true);


	CheckCol(m_pDuneLayer, m_colDLYrMaxSWL, "MAXSWL", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrMaxSWL, VData(0.0), true);

	CheckCol(m_pDuneLayer, m_colDLYrAvgSWL, "AVG_SWL_YR", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrAvgSWL, VData(0.0f), true);

	CheckCol(m_pDuneLayer, m_colDLYrAvgLowSWL, "AVGLWSWLYR", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLYrAvgLowSWL, VData(0.0f), true);



	////////
	////////CheckCol(m_pDuneLayer, m_colWb, "WB", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colHb, "HB", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colTS, "TS", TYPE_DOUBLE, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colAlpha, "ALPHA", TYPE_DOUBLE, CC_AUTOADD);
	////////
	////////
	// Erosion attributes

	// Annual K&D erosion extent
	CheckCol(m_pDuneLayer, m_colDLKD, "EROSION", TYPE_DOUBLE, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLKD, VData(0.0f), true);

	// K&D 2-yr average erosion extent
	CheckCol(m_pDuneLayer, m_colDLAvgKD, "AVGKD", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLAvgKD, VData(0.0f), true);

	CheckCol(m_pDuneLayer, m_colDLXAvgKD, "X_AVGKD", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLXAvgKD, VData(0.0f), true);

	// 0=Not Flooded, value=Yearly Maximum TWL
	CheckCol(m_pDuneLayer, m_colDLFlooded, "FLOODED", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLFlooded, VData(0), true);

	CheckCol(m_pDuneLayer, m_colDLAmtFlooded, "AMT_FLOOD", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLAmtFlooded, VData(0.0), true);

	////////// Frequency of K&D erosion event or flooded event
	////////CheckCol(m_pDuneLayer, m_colDLDuneEroFreq, "ERO_FREQ", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLDuneEroFreq, VData(0.0f), true);
	////////CheckCol(m_pDuneLayer, m_colDuneFloodFreq, "FLOOD_FREQ", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDuneFloodFreq, VData(0.0f), true);
	////////
	////////
	// ***** Impact Days ***** //

	// These attributes are calculated annually and represent the number of days per year the dune is impacted
	CheckCol(m_pDuneLayer, m_colDLIDDToe, "IDDUNETOE", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDToe, VData(0), true);
	CheckCol(m_pDuneLayer, m_colDLIDDCrest, "IDDUNECR", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDCrest, VData(0), true);

	// These attributes are calculated annually and represent the number of days per season the dune toe or dune crest is impacted
	CheckCol(m_pDuneLayer, m_colDLIDDToeWinter, "IDDTWINTER", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDToeWinter, VData(0), true);

	CheckCol(m_pDuneLayer, m_colDLIDDToeSpring, "IDDTSPRING", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDToeSpring, VData(0), true);

	CheckCol(m_pDuneLayer, m_colDLIDDToeSummer, "IDDTSUMMER", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDToeSummer, VData(0), true);

	CheckCol(m_pDuneLayer, m_colDLIDDToeFall, "IDDTFALL", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDToeFall, VData(0), true);

	CheckCol(m_pDuneLayer, m_colDLIDDCrestWinter, "IDDCWINTER", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDCrestWinter, VData(0), true);

	CheckCol(m_pDuneLayer, m_colDLIDDCrestSpring, "IDDCSPRING", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDCrestSpring, VData(0), true);

	CheckCol(m_pDuneLayer, m_colDLIDDCrestSummer, "IDDCSUMMER", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDCrestSummer, VData(0), true);

	CheckCol(m_pDuneLayer, m_colDLIDDCrestFall, "IDDCFALL", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLIDDCrestFall, VData(0), true);

	// Moving Average of Impact Days Per Year
	CheckCol(m_pDuneLayer, m_colDLMvAvgIDPY, "MVAVG_IDPY", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLMvAvgIDPY, VData(0.0f), true);

	CheckCol(m_pDuneLayer, m_colDLSCR, "SCR_NEW", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLSCR, VData(0.0f), true);


	CheckCol(m_pDuneLayer, m_colDLDeltaBruun, "DeltaBruun", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLDeltaBruun, VData(0.0f), true);

	CheckCol(m_pDuneLayer, m_colDLBchAccess, "BCH_ACCESS", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLBchAccess, VData(0.0f), true);

	CheckCol(m_pDuneLayer, m_colDLBchAccessWinter, "BCH_ACC_W", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLBchAccessWinter, VData(0.0f), true);

	CheckCol(m_pDuneLayer, m_colDLBchAccessSpring, "BCH_ACC_SP", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLBchAccessSpring, VData(0.0f), true);

	CheckCol(m_pDuneLayer, m_colDLBchAccessSummer, "BCH_ACC_SU", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLBchAccessSummer, VData(0.0f), true);

	CheckCol(m_pDuneLayer, m_colDLBchAccessFall, "BCH_ACC_F", TYPE_FLOAT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLBchAccessFall, VData(0.0f), true);




	////////
	////////
	////////// ****** Protection Structures ****** //
	////////
	////////// ****** Hard Protection Structures ****** //
	////////CheckCol(m_pDuneLayer, m_colDLEastingToeBPS, "EASTINGBPS", TYPE_DOUBLE, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLEastingToeBPS, VData(0.0), true);
	CheckCol(m_pDuneLayer, m_colDLAddYearBPS, "ADD_YR_BPS", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLAddYearBPS, VData(0), true);
	//CheckCol(m_pDuneLayer, m_colDLMaintYearBPS, "MAIN_YRBPS", TYPE_INT, CC_AUTOADD);
	//m_pDuneLayer->SetColData(m_colDLMaintYearBPS, VData(0), true);
	//CheckCol(m_pDuneLayer, m_colDLRemoveYearBPS, "REM_YR_BPS", TYPE_INT, CC_AUTOADD);
	//m_pDuneLayer->SetColData(m_colDLRemoveYearBPS, VData(0), true);
	////////
	////////
	////////// Hard Protection Structure Nourishment attributes
	//////CheckCol(m_pDuneLayer, m_colDLNourishVolBPS, "NOURVOLBPS", TYPE_FLOAT, CC_AUTOADD);
	//////CheckCol(m_pDuneLayer, m_colDLNourishYearBPS, "NOURYRBPS", TYPE_INT, CC_AUTOADD);
	//////m_pDuneLayer->SetColData(m_colDLNourishYearBPS, VData(0), true);
	//////CheckCol(m_pDuneLayer, m_colDLNourishFreqBPS, "NOURFRQBPS", TYPE_INT, CC_AUTOADD);
	////////
	////////
	////////// ****** Soft Protection Structures ****** //
	////////CheckCol(m_pDuneLayer, m_colDLEastingToeSPS, "EASTINGSPS", TYPE_DOUBLE, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLEastingToeSPS, VData(0.0), true);
	CheckCol(m_pDuneLayer, m_colDLAddYearSPS, "ADD_YR_SPS", TYPE_INT, CC_AUTOADD);
	m_pDuneLayer->SetColData(m_colDLAddYearSPS, VData(0), true);

	////////CheckCol(m_pDuneLayer, m_colDLMaintYearSPS, "MAIN_YRSPS", TYPE_INT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLMaintYearSPS, VData(0), true);
	////////
	////////CheckCol(m_pDuneLayer, m_colDLEastingCrestSPS, "ECRESTSPS", TYPE_DOUBLE, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLEastingCrestSPS, VData(0.0), true);
	////////CheckCol(m_pDuneLayer, m_colDLDuneToeSPS, "DUNETOESPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLDuneToeSPS, VData(0.0f), true);
	////////CheckCol(m_pDuneLayer, m_colDLHeelWidthSPS, "HWIDTHSPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLHeelWidthSPS, VData(0.0f), true);
	////////
	//////////   CheckCol(m_pDuneLayer, m_colDLEastingCrestSPS, "EASTCRESTSPS", TYPE_DOUBLE, CC_AUTOADD);
	////////
	////////// Soft Protection Structure Sand Volume attributes
	////////CheckCol(m_pDuneLayer, m_colDLConstructVolumeSPS, "BLDVOLSPS", TYPE_DOUBLE, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLConstructVolumeSPS, VData(0.0), true);
	////////CheckCol(m_pDuneLayer, m_colDLMaintVolumeSPS, "RBLDVOLSPS", TYPE_DOUBLE, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLMaintVolumeSPS, VData(0.0), true);
	////////
	////////
	////////// Soft Protection Structure Nourishment attributes
	////////CheckCol(m_pDuneLayer, m_colDLNourishVolSPS, "NOURVOLSPS", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colDLNourishYearSPS, "NOURYRSPS", TYPE_INT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLNourishYearSPS, VData(0), true);
	////////CheckCol(m_pDuneLayer, m_colDLNourishFreqSPS, "NOURFRQSPS", TYPE_INT, CC_AUTOADD);
	////////
	////////// Costs of Protection Structures
	////////CheckCol(m_pDuneLayer, m_colDLCostBPS, "COST_BPS", TYPE_DOUBLE, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLCostBPS, VData(0.0), true);
	////////
	////////CheckCol(m_pDuneLayer, m_colDLCostMaintBPS, "MNTCOSTBPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLCostMaintBPS, VData(0.0), true);
	////////
	////////CheckCol(m_pDuneLayer, m_colDLCostSPS, "COST_SPS", TYPE_DOUBLE, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLCostSPS, VData(0.0), true);
	//////////CheckCol(m_pDuneLayer, m_colNoursihCostSPS, "NOURCOSTBPS", TYPE_DOUBLE, CC_AUTOADD);
	//////////m_pDuneLayer->SetColData(m_colNourishCostSPS, VData(0.0), true);
	//////////CheckCol(m_pDuneLayer, m_colRebuildCostSPS, "RBLDCOSTBPS", TYPE_DOUBLE, CC_AUTOADD);
	//////////m_pDuneLayer->SetColData(m_colRebuildCostSPS, VData(0.0), true);
	////////
	////////// Height of Protection Structures
	////////CheckCol(m_pDuneLayer, m_colIDUHeightBPS, "HEIGHT_BPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colIDUHeightBPS, VData(0.0), true);
	////////CheckCol(m_pDuneLayer, m_colDLHeightSPS, "HEIGHT_SPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLHeightSPS, VData(0.0), true);
	////////
	////////// Length of Protection Structures
	////////CheckCol(m_pDuneLayer, m_colDLLengthBPS, "LENGTH_BPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLLengthBPS, VData(0.0), true);
	////////CheckCol(m_pDuneLayer, m_colDLLengthSPS, "LENGTH_SPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLLengthSPS, VData(0.0), true);
	////////
	////////// Width of Protection Structures
	////////CheckCol(m_pDuneLayer, m_colDLWidthBPS, "WIDTH_BPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLWidthBPS, VData(0.0), true);
	////////CheckCol(m_pDuneLayer, m_colDLWidthSPS, "WIDTH_SPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLWidthSPS, VData(0.0), true);
	////////CheckCol(m_pDuneLayer, m_colDLTopWidthSPS, "TOPWDTHSPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLTopWidthSPS, VData(0.0), true);
	////////
	////////
	////////// 1/0 Indication that Beachtype changed 
	////////// ??? track annually
	////////CheckCol(m_pDuneLayer, m_colDLTypeChange, "TYPECHANGE", TYPE_INT, CC_AUTOADD);
	////////
	/////////*CheckCol(m_pDuneLayer, m_colDLDuneWidthSPS, "DWIDTH_SPS", TYPE_FLOAT, CC_AUTOADD);
	////////m_pDuneLayer->SetColData(m_colDLDuneWidthSPS, VData(0.0), true);*/
	////////
	////////// Beach Accessibility attributes, annually and seasonally
	////////CheckCol(m_pDuneLayer, m_colBchAccess, "ACCESS", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colBchAccessWinter, "ACCESS_W", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colBchAccessSpring, "ACCESS_SP", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colBchAccessSummer, "ACCESS_SU", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colBchAccessFall, "ACCESS_F", TYPE_FLOAT, CC_AUTOADD);
	////////
	////////// currently unused
	////////
	/////////*CheckCol(m_pDuneLayer, m_colAvgDuneC, "AVG_DUNEC", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colAvgDuneT, "AVG_DUNET", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colAvgEro, "AVG_ERO", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colCPolicyL, "Policy", TYPE_INT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colThreshDate, "THR_DATE", TYPE_INT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colCalDCrest, "CAL_DCREST", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colCalDToe, "CAL_DTOE", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colCalEroExt, "CAL_EROEXT", TYPE_FLOAT, CC_AUTOADD);
	////////CheckCol(m_pDuneLayer, m_colCalDate, "CAL_DATE", TYPE_INT, CC_AUTOADD);*/
	//CheckCol(m_pDuneLayer, m_colBackshoreElev, "MIN_BACK_E", TYPE_FLOAT, CC_AUTOADD);
	//

	// Associate SWAN transects and cross shore profiles to duneline by NORTHING value
	// Initialize previous beachtype

	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		REAL xCoord = 0.0;
		REAL yCoord = 0.0;

		m_pDuneLayer->GetPointCoords(point, xCoord, yCoord);

		int beachType = -1;
		m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);

		// Determine SWAN Lookup transect index
		int rowIndex = -1;

		//double tIndex = IGet(m_pTransectLUT, yCoord, 0, 1, IM_LINEAR, 0, &rowIndex);
		//int transectIndex = m_pTransectLUT->GetAsInt(1, rowIndex);
		///*float index = m_pTransectLUT->IGet(yCoord, 1, IM_LINEAR);
		//int transectIndex = (int)round(index);*/
		//
		//if (transectIndex < m_minTransect)
		//   m_minTransect = transectIndex;
		//
		//if (transectIndex > m_maxTransect)
		//   m_maxTransect = transectIndex;

		//m_pDuneLayer->SetData(point, m_colTranIndex, transectIndex);

		// Determine ShorelineAngle 
		//   double shorelineAngle = m_pSAngleLUT->IGet((double)yCoord, 2, IM_LINEAR);
		//   m_pDuneLayer->SetData(point, m_colShorelineAngle, shorelineAngle);

		int duneIndex = -1;
		m_pDuneLayer->GetData(point, m_colDLDuneIndex, duneIndex);
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
		m_pDuneLayer->SetData(point, m_colDLDuneToe, duneToe);*/

		float duneCrest = 0.0;

		/*REAL xCoordCr = 0.0;
		m_pDuneLayer->GetData(point, m_colDLEastingCrest, xCoordCr);
		m_pElevationGrid->GetGridCellFromCoord(xCoordCr, yCoord, row, col);
		m_pElevationGrid->GetData(row, col, duneCrest);
		m_pDuneLayer->SetData(point, m_colDLDuneCrest, duneCrest);*/

		//   m_pDuneLayer->SetData(point, m_colProfileIndex, profileID);
		//   m_pDuneLayer->SetData(point, m_colDLShoreface, bruunRuleSlope);

		//   float foreshoreSlope = m_pProfileLUT->GetAsFloat(colForeshoreSlope, profileIndex);

		// just use foreshore slope (TANB) from Peter (in dune line coverage) instead of above
		float tanb1 = 0;
		m_pDuneLayer->GetData(point, m_colDLTranSlope, tanb1);
		m_pDuneLayer->SetData(point, m_colDLForeshore, tanb1);
		//rowtmp = -1;
		//double easting25m = IGet(pBathy, -25, 0, 1, IM_LINEAR, 0, &rowtmp, true);

		// The Bruun Rule slope is estimated to be the slope of the profile
		// between the 25m contour and MHW
		// The Bruun Rule slope (shoreface slope) is used to estimate
		// chronic erosion extents
		//distance = (easting25m - eastingMHW);
		//float BruunRuleSlope = (float)((-27.1) / distance);
		// float BruunSlope = 0.008f;

		//m_pDuneLayer->GetData(point, m_colDLBruunSlope, BruunSlope);

		//m_pDuneLayer->SetData(point, m_colDLShoreface, BruunRuleSlope);


		// find Maximum dune line index for display purposes
		if (duneIndex > m_maxDuneIndex)
			m_maxDuneIndex = duneIndex;

		//      float duneCrest = 0.0f;
		//////m_pDuneLayer->GetData(point, m_colDLDuneCrest, duneCrest);
		//////m_pDuneLayer->SetData(point, m_colPrevDuneCr, duneCrest);
		//////
		//////int startRow = -1;
		//////int startCol = -1;
		//////m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
		//////
		//////m_pDuneLayer->SetData(point, m_colPrevRow, startRow);
		//////m_pDuneLayer->SetData(point, m_colPrevCol, startCol);
		//////
		//////// Initialize original beachtype with existing beachtype
		//////beachType = 0;
		//////m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
		//////m_pDuneLayer->SetData(point, m_colOrigBeachType, beachType);
		//////
		//////int loc = 0;
		//////m_pDuneLayer->GetData(point, m_colLittCell, loc);
		//////if (loc == 1)
		//////   {
		//////   //m_numJettyPts++;
		//////   //m_numOceanShoresPts++;
		//////
		//////   }
		//////if (loc == 2)
		//////   {
		//////   //m_numJettyPts++;
		//////   //m_numWestportPts++;
		//////   }
		//////
		//////
		//////// Convert NORTHING and EASTING to LONGITUDE and LATITUDE for map csv display
		//////CalculateUTM2LL(m_pDuneLayer, point);

	} // end duneline points

	// Associate IDU <-> Duneline indexing
	////////int polyCount = m_pIDULayer->GetRowCount();
	////////for (int idu = 0; idu < polyCount; idu++)
	////////   {
	////////   Poly* pPoly = m_pIDULayer->GetPolygon(idu);
	////////   /* float northing = pPoly->m_yMax;*/
	////////   Vertex centroid = pPoly->GetCentroid();
	////////   REAL northing = centroid.y;
	////////   float minDiff = 9999.0F;
	////////   int duneIndex = -1;
	////////
	////////   for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	////////      {
	////////      REAL xCoord = 0.0;
	////////      REAL yCoord = 0.0;
	////////
	////////      m_pDuneLayer->GetPointCoords(point, xCoord, yCoord);
	////////      float tmpDiff = float(abs(yCoord - northing));
	////////
	////////      if (tmpDiff < minDiff)
	////////         {
	////////         minDiff = tmpDiff;
	////////         m_pDuneLayer->GetData(point, m_colDLDuneIndex, duneIndex);
	////////         }
	////////      }
	////////   m_pIDULayer->SetData(idu, m_colBeachfront, duneIndex);
	////////   }  // end of: for ( idu < polyCount )

			 // New Dune line layer starts off as copy of Original with changes of Label and Color
 //m_pDuneLayer = m_pMap->CloneLayer(*m_pDuneLayer);
 //m_pDuneLayer->m_name = "Active DuneLine";

//////// ////////// Change the color of the active dune line
 ////////int colIndex = m_pDuneLayer->GetFieldCol("DUNEINDEX");

//////// ////////// Rebin and classify for display
 ////////m_pDuneLayer->AddBin(colIndex, RGB(179, 89, 0), "All Points", TYPE_INT, 0, (float)m_maxDuneIndex + 1);
 ////////m_pDuneLayer->AddNoDataBin(colIndex);
 ////////m_pDuneLayer->ClassifyData();


	return true;
}

bool ChronicHazards::InitPolicyInfo(EnvContext* pEnvContext)
{

	CheckCol(m_pIDULayer, m_colIDULength, "Length", TYPE_FLOAT, CC_MUST_EXIST);
	CheckCol(m_pIDULayer, m_colIDUAddBPSYr, "ADD_YR_BPS", TYPE_INT, CC_AUTOADD);
	//CheckCol(m_pIDULayer, m_colIDUBeachfront, "BEACHFRONT", TYPE_INT, CC_AUTOADD);
	//CheckCol(m_pIDULayer, m_colIDUCityCode, "CITY_CODE", TYPE_INT, CC_AUTOADD);

	// set up policy information for budgeting
	m_policyInfoArray.SetSize(PI_COUNT);

	m_policyInfoArray[PI_CONSTRUCT_BPS].Init(_T("Construct BPS"), (int)PI_CONSTRUCT_BPS, &m_runConstructBPSPolicy, true);
	m_policyInfoArray[PI_MAINTAIN_BPS].Init(_T("Maintain BPS"), (int)PI_MAINTAIN_BPS, &m_runMaintainBPSPolicy, true);
	m_policyInfoArray[PI_NOURISH_BY_TYPE].Init(_T("Nourish By Type"), (int)PI_NOURISH_BY_TYPE, &m_runNourishByTypePolicy, true);

	m_policyInfoArray[PI_CONSTRUCT_SPS].Init(_T("Construct SPS"), (int)PI_CONSTRUCT_SPS, &m_runConstructSPSPolicy, true);
	m_policyInfoArray[PI_MAINTAIN_SPS].Init(_T("Maintain SPS"), (int)PI_MAINTAIN_SPS, &m_runMaintainSPSPolicy, true);
	m_policyInfoArray[PI_NOURISH_SPS].Init(_T("Nourish SPS"), (int)PI_NOURISH_SPS, &m_runNourishSPSPolicy, true);

	//m_policyInfoArray[PI_CONSTRUCT_SAFEST_SITE].Init(_T("Construct on Safest Site"), (int)PI_CONSTRUCT_SAFEST_SITE, &m_runConstructSafestPolicy, false);
	m_policyInfoArray[PI_REMOVE_BLDG_FROM_HAZARD_ZONE].Init(_T("Remove Bldg From Hazard Zone"), (int)PI_REMOVE_BLDG_FROM_HAZARD_ZONE, &m_runRemoveBldgFromHazardZonePolicy, true);
	m_policyInfoArray[PI_REMOVE_INFRA_FROM_HAZARD_ZONE].Init(_T("Remove Infra From Hazard Zone"), (int)PI_REMOVE_INFRA_FROM_HAZARD_ZONE, &m_runRemoveInfraFromHazardZonePolicy, true);
	m_policyInfoArray[PI_RAISE_RELOCATE_TO_SAFEST_SITE].Init(_T("Raise or Relocate to Safest Site"), (int)PI_RAISE_RELOCATE_TO_SAFEST_SITE, &m_runRelocateSafestPolicy, true);
	m_policyInfoArray[PI_RAISE_INFRASTRUCTURE].Init(_T("Raise Infrastructure"), (int)PI_RAISE_INFRASTRUCTURE, &m_runRaiseInfrastructurePolicy, true);

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



	return true;
}


bool ChronicHazards::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
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

	CString climateScenarioPath = "Climate_Scenarios\\" + m_climateScenarioStr + "\\";
	CString slrDataFile;
	slrDataFile.Format("%s%s_slr.csv", climateScenarioPath, m_climateScenarioStr);

	CString fullPath;
	PathManager::FindPath(slrDataFile, fullPath);
	int sEPRows = m_slrData.ReadAscii(fullPath);
	CString msg("Read in Sea Level Rise data: ");
	msg += slrDataFile;
	Report::LogInfo(msg);

	// Change the simulation number from 1 to generated number to randomize 

	// /*this->m_twlTime = 0;
	// this->m_floodingTime = 0;
	// this->m_erosionTime = 0;*/

	// //srand(time(nullptr));


	srand(static_cast<unsigned int>(time(nullptr)));

	/* if(m_randomize)
	{
	extension.Format("_%i.csv", m_randIndex);
	buoyFile += extension;
	}*/

	// Change the simulation number from 1 to generated number to randomize ****HERE****
	CString simulationPath;
	simulationPath.Format("%sSimulation_%i", climateScenarioPath, 1);
	//   simulationPath.Format("%sSimulation_%i", climateScenarioPath, m_randIndex);

	// get the offhore buoy files for this this climate scenario
	CString buoyFile;
	buoyFile.Format("%s\\BuoyData_%s.csv", simulationPath, m_climateScenarioStr);
	PathManager::FindPath(buoyFile, fullPath);

	//TODO: set m_inputinHourlydata through Envision
	m_inputinHourlydata = DATA_HOURLY; // You should set it to DATA_DAILY if input is daily data

	if (m_inputinHourlydata == DATA_HOURLY)
	{
		m_numDays = ConvertHourlyBuoyDataToDaily(fullPath);

		nsPath::CPath dailyBuoyFile(fullPath);
		dailyBuoyFile.RemoveExtension();
		CString _path = dailyBuoyFile.GetStr() + "_daily.csv";
		m_buoyObsData.WriteAscii(_path);
	}
	else
	{
		// Read daily buoy observations/simulations for the entire Envision time period
		m_numDays = m_buoyObsData.ReadAscii(fullPath);
	}
	int cols = -1;

	if (m_numDays <= 0)
	{
		CString msg;
		msg.Format("InitRun could not load buoy csv file \n");
		Report::LogInfo(msg);
		return false;
	}
	else
	{
		m_numYears = m_numDays / 365;
		CString msg;
		msg.Format("The buoy data %s contains %i years of daily data", (LPCTSTR)buoyFile, m_numYears);
		Report::LogInfo(msg);
		/*  Report::Log(buoyFile);
		cols = m_buoyObsData.GetColCount();*/
	}


	LoadDailyRBFOutputs(simulationPath);
	//WriteDailyData(fullPath);

	//////CString bayTimeSeriesFolder;
	//////bayTimeSeriesFolder.Format("%s\\DailyBayData_%s", (LPCTSTR) simulationPath, (LPCTSTR) m_climateScenarioStr);
	//////PathManager::FindPath(bayTimeSeriesFolder, fullPath);

	//////bool runSpatialBayTWL = (m_runSpatialBayTWL == 1) ? true : false;
	//////if (runSpatialBayTWL)
	//////   ReadDailyBayData(fullPath);

	// initialize policyInfo for this run (Note - m_isActive is already set by the scenario variable)
	if (m_runFlags & CH_MODEL_POLICY)
	{
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
			PolicyInfo& pi = m_policyInfoArray[i];

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

		// set up policy output file
		CString policySummaryPath(PathManager::GetPath(PM_OUTPUT_DIR));
		CString scname;
		Scenario* pScenario = ::EnvGetScenario(pEnvContext->pEnvModel, pEnvContext->scenarioIndex);

		CString path;
		path.Format("%sPolicySummary_%s_Run%i.csv", (LPCTSTR)policySummaryPath, (LPCTSTR)pScenario->m_name, pEnvContext->run);
		fopen_s(&fpPolicySummary, path, "wt");
		fputs("Year,Policy,Locator,Unit Cost,Total Cost, Avail Budget,Param1,Param2,PassCostConstraint", fpPolicySummary);
	}

	// reset the cumulative flood map
   // if (m_runFlags & CH_MODEL_FLOODING)
	//   m_pCumFloodedGrid->SetAllData(0, false);

	//// Read in the Bay coverage if running Bay Flooding Model
	/*bool runBayFloodModel = (m_runBayFloodModel == 1) ? true : false;
	if (runBayFloodModel)
	{
	PathManager::FindPath("BayPts.csv", fullPath);
	m_pInletLUT = new DDataObj;
	m_numBayPts = m_pInletLUT->ReadAscii(fullPath);
	}*/

	return TRUE;
} // end InitRun(EnvContext *pEnvContext, bool useInitialSeed)



bool ChronicHazards::Run(EnvContext* pEnvContext)
{
	//const float tolerance = 1.0f;    // tolerance value
	//MaxYearlyTWL(pEnvContext);

	int numDunePts = 0;
	if (m_pDuneLayer != nullptr)
	{
		m_pDuneLayer->m_readOnly = false;
		numDunePts = m_pDuneLayer->GetRowCount();
	}

	// Reset annual duneline attributes
	ResetAnnualVariables();      // reset annual variables for hte start of a new year
	//ResetAnnualBudget();      // reset budget allocations for the year

	RunTWLModel(pEnvContext);   // this populates yr max twl and associated cols, and computes impact days


	// Determine Yearly Maximum TWL and impact days per year 
	// based upon daily TWL calculations at each shoreline location
	// NOTE: THIS IS ONLY NEEDED FOR THE EROSION MODEL
	//CalculateYrMaxTWL(pEnvContext);

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

	if (m_runFlags & CH_MODEL_EROSION)
		RunErosionModel(pEnvContext);
	/******************** Run Scenario Policy to construct on safest site before tallying statistics  ********************/

	m_meanTWL /= numDunePts;

	// Scenarios indicate policy for new buildings to be constructed on the safest site
	/*bool runConstructSafestPolicy = (m_runConstructSafestPolicy == 1) ? true : false;
	if (runConstructSafestPolicy)
	ConstructOnSafestSite(pEnvContext, false);
	*/

	//if (m_runEelgrassModel > 0)
	//   RunEelgrassModel(pEnvContext);

	if (m_runFlags & CH_MODEL_FLOODING)
		RunFloodingModel(pEnvContext);

	if (m_runFlags & CH_MODEL_BUILDINGS)
	{
		Report::Log_i("Running building submodel for year% i", pEnvContext->currentYear);
		ComputeBuildingStatistics();
	}

	// Use generated Grid Maps to Calculate Statistics 
	// statistics to output or statistics which in turn trigger policies 
	if (m_runFlags & CH_MODEL_INFRASTRUCTURE)
	{
		Report::Log_i("Running infrastructure submodel for year% i", pEnvContext->currentYear);
		ComputeInfraStatistics();
	}

	// Calculate Shoreline Averages
	//m_avgCountyAccess /= (m_numDunePts / 100.0f);
	//m_avgOceanShoresAccess /= (m_numOceanShoresPts / 100.0f);
	//m_avgWestportAccess /= (m_numWestportPts / 100.0f);
	//m_avgJettyAccess /= (m_numJettyPts / 100.0f);


	// Run Scenario dependent policies
	if (m_runFlags & CH_MODEL_POLICY)
		RunPolicyManagement(pEnvContext);

	TallyCostStatistics(pEnvContext->currentYear);
	TallyDuneStatistics(pEnvContext->currentYear);
	TallyRoadStatistics();

	//  m_pMap->ChangeDrawOrder(m_pDuneLayer, DO_BOTTOM);
	m_pMap->ChangeDrawOrder(m_pDuneLayer, DO_BOTTOM);
	m_pDuneLayer->m_readOnly = true;

	if (m_exportMapInterval != -1 && ((pEnvContext->yearOfRun % m_exportMapInterval == 0) || (pEnvContext->endYear == pEnvContext->currentYear + 1)))
		ExportMapLayers(pEnvContext, pEnvContext->currentYear);

	return true;
} // end Run(EnvContext *pEnvContext)



bool ChronicHazards::RunTWLModel(EnvContext* pEnvContext)
{
	if (m_pDuneLayer == nullptr)
		return false;

	// initialize annual variables to zero prior to calulation below
	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		m_pDuneLayer->SetData(point, m_colDLHdeep, 0);
		m_pDuneLayer->SetData(point, m_colDLWPeriod, 0);
		m_pDuneLayer->SetData(point, m_colDLWHeight, 0);
		m_pDuneLayer->SetData(point, m_colDLWDirection, 0);

		m_pDuneLayer->SetData(point, m_colDLYrFMaxTWL, 0);
		m_pDuneLayer->SetData(point, m_colDLYrMaxTWL, 0);
		m_pDuneLayer->SetData(point, m_colDLYrMaxDoy, 0);
		//m_pDuneLayer->SetData(point, m_colYrMaxSWL, 0);
		m_pDuneLayer->SetData(point, m_colDLYrMaxSetup, 0);
		m_pDuneLayer->SetData(point, m_colDLYrMaxIGSwash, 0);
		m_pDuneLayer->SetData(point, m_colDLYrMaxIncSwash, 0);
		m_pDuneLayer->SetData(point, m_colDLYrMaxSwash, 0);
	}

	CalculateTWLandImpactDaysAtShorePoints(pEnvContext);
	return true;
}


bool ChronicHazards::RunErosionModel(EnvContext* pEnvContext)
{
	ASSERT(m_pDuneLayer != nullptr);

	Report::Log_i("Running erosion submodel for year %i", pEnvContext->currentYear);

	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		int beachType = 0;
		m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);


		float R_inf_KD = 0;
		float R_bruun = 0;
		float R_SCR_hist = 0;




		if ((beachType >= (int)BchT_SANDY_DUNE_BACKED && beachType <= (int)BchT_SANDY_WOODY_DEBRIS_BACKED) || beachType == BchT_SANDY_BURIED_RIPRAP_BACKED)
		{
			if (beachType == BchT_SANDY_DUNE_BACKED || beachType == BchT_MIXED_SEDIMENT_DUNE_BACKED)
			{
				R_inf_KD = KDmodel(point);


			}

			if ((beachType >= (int)BchT_SANDY_DUNE_BACKED && beachType <= (int)BchT_SANDY_COBBLEGRAVEL_BLUFF_BACKED) || beachType == BchT_SANDY_BURIED_RIPRAP_BACKED || beachType == BchT_SANDY_WOODY_DEBRIS_BACKED)
			{
				R_bruun = Bruunmodel(pEnvContext, point);

			}

			R_SCR_hist = SCRmodel(pEnvContext, point);

		}







		// Do NOT calculate erosion for beachtypes of Undefined (0), Bay (10) , River (11) , Rocky Headland (12) ,& JETTY (14)

		if ((beachType >= (int)BchT_SANDY_DUNE_BACKED && beachType <= (int)BchT_SANDY_WOODY_DEBRIS_BACKED) || beachType == BchT_SANDY_BURIED_RIPRAP_BACKED)
		{
			// Basic idea is for other (potentially erodable) beach types:
			// 1) determine shift in x positions based on erosions rates (done above)
			// 2) for erodable beach types, 
			//     a) shift the MHW easting by dx.
			//	   b) if resulting backshore slope exceed slope threshold, then
			//        adjust the dune to easting until tanb = max slope
			// 3) for non-erodible beach types:
			//    a) shift the MWH easting by the minimum of dx or that needed
			//       to achieve a backshore slope = max slope, leaving dune to easting
			//       unchanged

			// 1) determine shift in x positions based on erosions rates (done above)
			float dx = 0.0f;			

			// If shoreline is eroding, dune toe line moves landward
			if (R_SCR_hist < 0.0f)
				dx = (R_SCR_hist + R_bruun) + R_inf_KD;
			else
				dx = R_bruun;			

			// 2) for erodable beach types (no riprap)
			//     a) shift the MHW easting by dx.
			//	   b) if resulting backshore slope exceed slope threshold, then
			//        adjust the dune to easting until tanb = max slope
			if ((beachType >= (int)BchT_SANDY_DUNE_BACKED && beachType <= (int)BchT_SANDY_WOODY_DEBRIS_BACKED))
			{
				float eastingMHW = 0;
				m_pDuneLayer->GetData(point, m_colDLEastingMHW, eastingMHW);

				float newEastingMHW = eastingMHW - dx;

				float duneToeElevation = 0.0f;
				m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToeElevation);

				float duneToeEasting = 0;
				m_pDuneLayer->GetData(point, m_colDLEastingToe, duneToeEasting);

				float newBeachWidth = duneToeEasting - newEastingMHW;  // check signs

				float newBeachSlope = duneToeElevation / newBeachWidth;
				//	if resulting backshore slope exceed slope threshold, then
				//      adjust the dune toe easting until tanb = max slope

				if (newBeachSlope < 0.1f)
				{

				}





			m_pDuneLayer->GetData(point, m_colDLMHWEasting, duneToeElevation);


			float duneToeElevation = 0.0f;
			float duneToeEasting = 0.0f;
			float beachwidth = 0;
			//float tanb1 = 0;

			// Set new position of the Dune toe point
			//Move the point to the extent of erosion. For now, all three types of erosion are included.   
			// Currently only move dunetoe line for eroding shoreline (not prograding)
			Poly* pPoly = m_pDuneLayer->GetPolygon(point);
			//if ((beachType == BchT_SANDY_DUNE_BACKED) || (beachType==BchT_MIXED_SEDIMENT_DUNE_BACKED))
			float xCoord = pPoly->GetVertex(0).x;




			m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToeElevation);
			m_pDuneLayer->GetData(point, m_colDLEastingToe, duneToeEasting);
			m_pDuneLayer->GetData(point, m_colDLBeachWidth, beachwidth);

			// calculate prior beach slope

			float newBeachWidth = beachwidth - dx;   // narrow beach by dx  ???
			float newTanb = duneToeElevation / newBeachWidth;
			float newDuneToeElevation = duneToeElevation;   /// need to to move dune toe elevation?
			float newDuneToeEasting = duneToeEasting - dx;

			// in the case of hardened beaches, don't allow bachshore slope to exceed 0.1
			//// For Hard Protection Structures
			//// change the beachwidth and slope to reflect the new shoreline position
			//// beach is assumed to narrow at the rate of the total chronic erosion, reflected through 
			//// dynamic beach slopes
			//// beaches further narrow when maintaining and constructing BPS at a 1:2 slope
			//// 
			//// Need to account for negative shoreline change rate 
			//// shorelineChangeRate > 0.0f ? SCRate = shorelineChangeRate : SCRate;
			//
			//// Negative shoreline change rate indicates eroding shoreline
			//// Currently ignoring progradating shoreline

			if (beachType == BchT_SANDY_BURIED_RIPRAP_BACKED)
			{
				if (newTanb > 0.1f)
				{
					newBeachWidth = newDuneToeElevation / 0.1f;   // limit beach slope to a max of 0.1
					newDuneToeEasting = duneToeEasting;   // unchanged becuase we hit riprap
					newDuneToeElevation = duneToeElevation;   // unchanged becuase we hit riprap
					newTanb = 0.1f;
				}
			}

			m_pDuneLayer->SetData(point, m_colDLBeachWidth, newBeachWidth);
			m_pDuneLayer->SetData(point, m_colDLDuneToe, newDuneToeElevation);
			m_pDuneLayer->SetData(point, m_colDLTranSlope, newTanb);
			m_pDuneLayer->SetData(point, m_colDLEastingMHW, newDuneToeEasting);


	}
			}
			///// MOVE TO PolicyManagement!!!  ???
			/////else if (beachType == BchT_RIPRAP_BACKED)
			/////   {
			/////   float BPSCost;
			/////   m_pDuneLayer->GetData(point, m_colDLCostBPS, BPSCost);
			/////   m_maintCostBPS += BPSCost * m_costs.BPSMaint;
			/////   }

			//float Ero = (float)R_inf_KD;

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

			// Impact days per year on dune toe (note that this is calcualted by the TWL Model)
			m_pDuneLayer->GetData(point, m_colDLIDDToe, IDDToeCount);
			m_pDuneLayer->GetData(point, m_colDLIDDToeWinter, IDDToeCountWinter);
			m_pDuneLayer->GetData(point, m_colDLIDDToeSpring, IDDToeCountSpring);
			m_pDuneLayer->GetData(point, m_colDLIDDToeSummer, IDDToeCountSummer);
			m_pDuneLayer->GetData(point, m_colDLIDDToeFall, IDDToeCountFall);

			// Impact days per year on dune crest
			m_pDuneLayer->GetData(point, m_colDLIDDCrest, IDDCrestCount);
			m_pDuneLayer->GetData(point, m_colDLIDDCrestWinter, IDDCrestCountWinter);
			m_pDuneLayer->GetData(point, m_colDLIDDCrestSpring, IDDCrestCountSpring);
			m_pDuneLayer->GetData(point, m_colDLIDDCrestSummer, IDDCrestCountSummer);
			m_pDuneLayer->GetData(point, m_colDLIDDCrestFall, IDDCrestCountFall);

			float duneToe = 0;
			float yrMaxTWL = 0;
			float beachwidth = 0;


			MovingWindow* ipdyMovingWindow = m_IDPYArray.GetAt(point);
			ipdyMovingWindow->AddValue((float)(IDDToeCount + IDDCrestCount));

			// for debugging
			///// if (m_debug)
			/////    {
			/////    int totalIDPY = 0;
			/////    m_pDuneLayer->GetData(point, m_colNumIDPY, totalIDPY);
			/////    totalIDPY += (IDDToeCount + IDDCrestCount);
			/////    m_pDuneLayer->SetData(point, m_colNumIDPY, totalIDPY);
			/////    }


			// Retrieve the average imapact days per year within the designated window
			float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();
			m_pDuneLayer->SetData(point, m_colDLMvAvgIDPY, movingAvgIDPY);
		

			MovingWindow* twlMovingWindow = m_TWLArray.GetAt(point);
			twlMovingWindow->AddValue(yrMaxTWL);
			// Retrieve the maxmum TWL within the designated window
			float movingMaxTWL = twlMovingWindow->GetMaxValue();
			m_pDuneLayer->SetData(point, m_colDLMvMaxTWL, movingMaxTWL);

			// Retrieve the average TWL within the designated window
			float movingAvgTWL = twlMovingWindow->GetAvgValue();
			m_pDuneLayer->SetData(point, m_colDLMvAvgTWL, movingAvgTWL);

			MovingWindow* eroKDMovingWindow = m_eroKDArray.GetAt(point);
			eroKDMovingWindow->AddValue((float)R_inf_KD);

			//ptrArrayIndex++;


			// MOVE TO POLICY MANAGEMENT???
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

			if (beachAccess > m_accessThresh)
				m_avgAccess += 1;

			//m_hardenedShoreline = (percentArmored / float(shorelineLength)) * 100;
			m_pDuneLayer->m_readOnly = false;

			//         m_pDuneLayer->SetData(point, m_colAvgDuneT, avgduneTArray[ duneIndex ]);
			//         m_pDuneLayer->SetData(point, m_colAvgDuneC, avgduneCArray[ duneIndex ]);
			//         m_pDuneLayer->SetData(point, m_colAvgEro, avgEroArray[ duneIndex ]);
			//         m_pDuneLayer->SetData(point, m_colCPolicyL, 0);
			m_pDuneLayer->SetData(point, m_colDLBchAccess, beachAccess);
			m_pDuneLayer->SetData(point, m_colDLBchAccessWinter, beachAccessWinter);
			m_pDuneLayer->SetData(point, m_colDLBchAccessSpring, beachAccessSpring);
			m_pDuneLayer->SetData(point, m_colDLBchAccessSummer, beachAccessSummer);
			m_pDuneLayer->SetData(point, m_colDLBchAccessFall, beachAccessFall);
			/////// m_pDuneLayer->SetData(point, m_colDLWb, Wb);
			/////// m_pDuneLayer->SetData(point, m_colDLTS, TS);
			/////// m_pDuneLayer->SetData(point, m_colDLHb, Hb);
			m_pDuneLayer->SetData(point, m_colDLKD, R_inf_KD);
			/////// m_pDuneLayer->SetData(point, m_colDLAlpha, alpha);
			m_pDuneLayer->SetData(point, m_colDLShorelineChange, R_SCR_hist);
			m_pDuneLayer->SetData(point, m_colDLBeachWidth, beachwidth);
			//m_pDuneLayer->m_readOnly = true;

		} // end erosion calculation
		return true; 
} // end dune line points


 // Add this when needed
	//int erodedCount = 0;
	//GenerateErosionMap(erodedCount);

//	return true;
//}


bool ChronicHazards::RunFloodingModel(EnvContext* pEnvContext)
{
	/************************   Generate Flooding Grid ************************/

	Report::Log_i("Running flooding submodel for year% i", pEnvContext->currentYear);

	int count = 0;
	clock_t start = clock();

	CString outDir = PathManager::GetPath(PM_OUTPUT_DIR);

	// generate the input file for the SFINCS function.  This involves looking through the
	// year's TWLs and associated data to identify the "highest" TWL event of the year, which is passed to the 
	// SFINCS function in the form of a CSV file

	// first, create a data obj for the CSV table
	FDataObj twlData(8, 0);  // Cols:
	twlData.SetLabel(0, "Transect");
	twlData.SetLabel(1, "Northing");
	twlData.SetLabel(2, "Setup");
	twlData.SetLabel(3, "infragravity");
	twlData.SetLabel(4, "Incident band swash");
	twlData.SetLabel(5, "Runup");
	twlData.SetLabel(6, "WL");
	twlData.SetLabel(7, "TWL");

	// next, populate it with necessary info
	// note - these values were previously calculated by the TWL model
	float data[8] = { 0,0,0,0,0,0,0,0 };
	for (MapLayer::Iterator shorePt = m_pDuneLayer->Begin(); shorePt < m_pDuneLayer->End(); shorePt++)
	{
		m_pDuneLayer->GetData(shorePt, m_colDLTransID, data[0]);
		m_pDuneLayer->GetData(shorePt, m_colDLNorthing, data[1]);
		m_pDuneLayer->GetData(shorePt, m_colDLYrMaxSetup, data[2]);
		m_pDuneLayer->GetData(shorePt, m_colDLYrMaxIGSwash, data[3]);
		m_pDuneLayer->GetData(shorePt, m_colDLYrMaxIncSwash, data[4]);
		m_pDuneLayer->GetData(shorePt, m_colDLYrMaxRunup, data[5]);
		m_pDuneLayer->GetData(shorePt, m_colDLYrMaxSWL, data[6]);
		m_pDuneLayer->GetData(shorePt, m_colDLYrFMaxTWL, data[7]);

		if (data[7] <= 0.0f)   // if FTWL not calculated, use TWL
		{
			m_pDuneLayer->GetData(shorePt, m_colDLYrMaxTWL, data[7]);
			data[7] += 1.17f;
		}

		twlData.AppendRow(data, 8);
	}

	CString twlFile(m_sfincsHome); // = PathManager::GetPath(PM_IDU_DIR);
	twlFile += "/TWL.csv";
	twlData.WriteAscii(twlFile, ',');

	// persistent version
	twlFile.Format("%sTWL_%i.csv", outDir, pEnvContext->yearOfRun);
	twlData.WriteAscii(twlFile, ',');

	// Get ready to call SFINCS Model
	// Create MATLAB data array factory
	matlab::data::ArrayFactory factory;
	// Pass vector containing 3 scalar args in vector    
	std::vector<matlab::data::Array> args({
		factory.createCharArray((LPCTSTR)twlFile),// TWL_dir
		//factory.createScalar<double>(5), // TWL
		factory.createScalar<int>(0), // Advection
		factory.createScalar<int>(100), // Cellsize
		factory.createCharArray((LPCTSTR)m_sfincsHome),// sfincs home dir
		});

	// Create string buffer for standard output
	typedef std::basic_stringbuf<char16_t> StringBuf;
	std::shared_ptr<StringBuf> output = std::make_shared<StringBuf>();

	// Call MATLAB function and return result
	try {
		matlab::data::TypedArray<int> result = m_matlabPtr->feval(u"SFINCS_ENVISION", args, output);
	}
	catch (const std::exception& ex) {
		Report::LogError(ex.what());

		//ex.stackTrace[0].fileName
		//ex.stackTrace[0].lineNumber
		// std::cout<<ex.what();
	}
	catch (const std::string& ex) {
		Report::LogError(ex.c_str()); //std::cout << ex;
	}
	catch (...) {
		//std::exception_ptr p = std::current_exception();
		Report::LogError("Unspecified exception when running flood model");
	}

	// Display MATLAB output in C++
	matlab::engine::String output_ = output.get()->str();
	Report::Log(matlab::engine::convertUTF16StringToUTF8String(output_).c_str());

	// load grid
	Map* pMap = m_pDuneLayer->GetMapPtr();
	CString grdFile("Tillamook_Max_Flooding_Depths1.asc");

	Report::StatusMsg("Loading flooded depth grid");
	if (m_pFloodedGrid != nullptr)
		m_pFloodedGrid->m_pMap->RemoveLayer(m_pFloodedGrid, true);

	m_pFloodedGrid = pMap->AddGridLayer(grdFile, DO_TYPE::DOT_FLOAT);


	CalculateFloodImpacts(pEnvContext);


	// update timings
	clock_t finish = clock();
	float duration = (float)(finish - start) / CLOCKS_PER_SEC;

	CString msg;
	msg.Format("Flood Model ran in %i seconds", (int)duration);
	Report::Log(msg);
	return true;
}



bool ChronicHazards::CalculateFloodImpacts(EnvContext* pEnvContext)
{
	// Basic idea - calculate flooding impacts on:
	//  1) Roads   (m_colRoadFlooded)
	//  2) Infrastructure  (m_colInfraFlooded)
	//  3) IDUs  (m_colIDUFracFlooded, m_colIDUFlooded
	//  4) Buildings (m_colBldgFlooded, m_colBldgFloodFreq)
	// 
	if (m_pFloodedGrid == nullptr)
		return false;

	CString ppmDir = PathManager::GetPath(PM_IDU_DIR);
	ppmDir += "PolyGridMaps\\";


	// make sure mappings exist between:
	// 1) roads/flooded grid
	// 2) infrastructure/flooded grid
	// 3) idus/flooded grid
	// 4) buildings/flooded grid

	// make sure Roads/Flooded Grid lookup is available
	if (m_pRoadFloodedGridLkUp == nullptr)
	{
		// Load from file if found, otherwise create PolyGridMap for Roads based upon cell sizes of the Elevation Layer
		CString roadsPglFile = ppmDir + "Road_FloodedGridLkup.pgm";
		m_pRoadFloodedGridLkUp = new PolyGridMapper(m_pRoadLayer, m_pFloodedGrid, 1, roadsPglFile);

		// Check built lookup matches grid 
		int numRows = m_pFloodedGrid->GetRowCount();
		int numCols = m_pFloodedGrid->GetColCount();

		// Check built lookup matches grid
		if ((m_pRoadFloodedGridLkUp->GetNumGridCols() != numCols) || (m_pRoadFloodedGridLkUp->GetNumGridRows() != numRows))
			Report::ErrorMsg("ChronicHazards: Road Grid Lookup row or column mismatch, rebuild (delete) polygrid lookup");
	}

	// make sure IDU/Flooded Grid lookup is available
	if (m_pIduFloodedGridLkUp == nullptr)
	{
		Report::StatusMsg("Building polygrid lookup IDUGridLkup.pgm");
		CString IDUPglFile = ppmDir + "IDU_FloodGridLkup.pgm";
		m_pIduFloodedGridLkUp = new PolyGridMapper(m_pIDULayer, m_pFloodedGrid, 1, IDUPglFile);

		// Check built lookup matches grid 
		int numRows = m_pFloodedGrid->GetRowCount();
		int numCols = m_pFloodedGrid->GetColCount();

		if ((m_pIduFloodedGridLkUp->GetNumGridCols() != numCols) || (m_pIduFloodedGridLkUp->GetNumGridRows() != numRows))
			Report::ErrorMsg("ChronicHazards: IDU Grid Lookup row or column mismatch, rebuild (delete) polygrid lookup");
		else if (m_pIduFloodedGridLkUp->GetNumPolys() != m_pIDULayer->GetPolygonCount())
			Report::ErrorMsg("ChronicHazards: IDU Grid Lookup IDU count mismatch, rebuild (delete) polygrid lookup");
	}

	// make sure Bldg/Flooded Grid lookup is available
	//if (m_pBldgFloodedGridLkUp == nullptr)
	//   {
	//   Report::StatusMsg("Building polypoint mapper IDUBuildings.ppm");
	//   CString bldgPglFile = ppmDir + "IDUBuildings.ppm";
	//   m_pIduBuildingLkUp = new PolyPointMapper(m_pFloodedGrid, m_pBldgLayer, bldgPglFile);  // creates map if file note found, otherwse, just loads it
	//
	//   }


	// road impacts

	// Value: 0 if not flooded, TWL if flooded
	float flooded = 0.0f;

	// Count of area of grid cells in flooded grid that are flooded to determine area flooded
	double floodedArea = 0.0f; // grid cells that are flooded multiplied by the number of grid cells

	// road stats first
	if (m_runFlags & CH_MODEL_INFRASTRUCTURE && m_pRoadLayer != nullptr)
	{
		/************************   Calculate Flooded Road Statistics ************************/
		int numRows = m_pFloodedGrid->GetRowCount();
		int numCols = m_pFloodedGrid->GetColCount();

		float noDataValue = m_pFloodedGrid->GetNoDataValue();

		// iterate through the flooded grid cells
		for (int row = 0; row < numRows; row++)
		{
			for (int col = 0; col < numCols; col++)
			{
				// get the number of polygons that intersect this cell
				int polyCount = m_pIduFloodedGridLkUp->GetPolyCntForGridPt(row, col);

				/*if (polyCount > 0)
				{*/
				// next, we want to iterate through the intersecting polygons to determine which are flooded
				int* indices = new int[polyCount];
				int count = m_pIduFloodedGridLkUp->GetPolyNdxsForGridPt(row, col, indices);

				/*if (count > 0)
				{*/
				REAL xCenter = 0.0, yCenter = 0.0;
				COORD2d ll, ur;
				m_pFloodedGrid->GetGridCellRect(row, col, ll, ur);
				m_pFloodedGrid->GetGridCellCenter(row, col, xCenter, yCenter);
				REAL xMin0 = xCenter - (m_elevCellWidth / 2.0);
				REAL yMin0 = yCenter - (m_elevCellHeight / 2.0);
				REAL xMax0 = xCenter + (m_elevCellWidth / 2.0);
				REAL yMax0 = yCenter + (m_elevCellHeight / 2.0);

				// for each road in the grid cell...
				for (int i = 0; i < polyCount; i++)
				{
					int roadType = -1;
					Poly* pPoly = m_pRoadLayer->GetPolygon(indices[i]);
					m_pRoadLayer->GetData(indices[i], m_colRoadType, roadType);

					float flooded = 0.0f;
					m_pFloodedGrid->GetData(row, col, flooded);
					bool isFlooded = (flooded > 0.0f) ? true : false;

					if (isFlooded) // && flooded != noDataValue)
					{
						//if (roadType != 6)
						//   {
						//   if (roadType == 7)
						//      {
						//      double lengthOfRailroad = 0.0;
						//      lengthOfRailroad = DistancePolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), xMin0, yMin0, xMax0, yMax0);
						//      m_floodedRailroadMiles += float(lengthOfRailroad);
						//      m_pRoadLayer->SetData(indices[i], m_colRoadFlooded, 1);
						//
						//      }
						//   else if (roadType != 5)
						//      {
						double lengthOfRoad = 0.0;
						lengthOfRoad = DistancePolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), ll.x, ll.y, ur.x, ur.y);
						m_floodedRoad += float(lengthOfRoad);
						m_pRoadLayer->SetData(indices[i], m_colRoadFlooded, 1);

					}
				}
				delete[] indices;
			}
		}  // end of: if (m_runFlags & CH_MODEL_INFRASTRUCTURE && m_pRoadLayer != nullptr)

	 // meters to miles
		m_floodedRoadMiles = m_floodedRoad * MI_PER_M;
		m_floodedRailroadMiles *= MI_PER_M;
	}

	// road metrics complete.  next, infrastructure
	if (m_runFlags & CH_MODEL_INFRASTRUCTURE && m_pInfraLayer != nullptr)
	{
		/************************   Calculate Flooded Infrastructure Statistics ************************/
		int numRows = m_pFloodedGrid->GetRowCount();
		int numCols = m_pFloodedGrid->GetColCount();

		for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
		{
			REAL xCoord = 0.0;
			REAL yCoord = 0.0;

			float flooded = 0.0f;

			int startRow = -1;
			int startCol = -1;

			int infraIndex = -1;
			m_pInfraLayer->GetData(infra, m_colInfraDuneIndex, infraIndex);

			int duCount = 0;
			/*m_pInfraLayer->GetData(point, m_colInfraCount, duCount);*/

			//bool isDeveloped = (duCount > 0) ? true : false;
			m_pInfraLayer->GetPointCoords(infra, xCoord, yCoord);
			m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

			MovingWindow* floodMovingWindow = m_floodInfraFreqArray.GetAt(infra);

			// within grid ?
			if ((startRow >= 0 && startRow < numRows) && (startCol >= 0 && startCol < numCols))
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
			}
		}
	} // end of:  if (m_runFlags & CH_MODEL_INFRASTRUCTURE && m_pInfraLayer != nullptr)

 // road metrics complete.  next, infrastructure
	if (m_pDuneLayer != nullptr)
	{
		int numRows = m_pFloodedGrid->GetRowCount();
		int numCols = m_pFloodedGrid->GetColCount();

		int colNorthing = -1;
		int colEasting = -1;

		//if (m_pBayTraceLayer != nullptr)
		//   {
		//   CheckCol(m_pBayTraceLayer, colNorthing, "NORTHING", TYPE_DOUBLE, CC_MUST_EXIST);
		//   CheckCol(m_pBayTraceLayer, colEasting, "EASTING", TYPE_DOUBLE, CC_MUST_EXIST);
		//   }

		//clock_t start = clock();
		for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
		{
			int beachType = 0;
			m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);

			int duneIndex = -1;
			m_pDuneLayer->GetData(point, m_colDLDuneIndex, duneIndex);

			REAL xCoord = 0.0;
			REAL yCoord = 0.0;

			int startRow = -1;
			int startCol = -1;
			float minElevation = 0.0f;
			float yearlyMaxTWL = 0.0f;
			float yearlyAvgTWL = 0.0f;
			double duneCrest = 0.0;

			if (beachType == BchT_BAY) // && m_pBayTraceLayer != nullptr)
			{
				// get twl of inlet seed point
				m_pDuneLayer->GetData(point, m_colDLYrFMaxTWL, yearlyMaxTWL);

				//if (pEnvContext->yearOfRun < m_windowLengthTWL - 1)
				//   m_pDuneLayer->GetData(point, m_colYrAvgTWL, yearlyMaxTWL);
				//else
				//   m_pDuneLayer->GetData(point, m_colMvMaxTWL, yearlyMaxTWL);

				   // traversing along bay shoreline, determine if twl is more than the elevation 

				   //for (int inletRow = 0; inletRow < m_numBayPts; inletRow++)
				   //{
				   //   // bay trace can be beachtype of bay or river
				   //   // do not calculate flooding for river type
				   //   /*beachType = m_pInletLUT->GetAsInt(3, inletRow);            
				   //   if (beachType == BchT_BAY)
				   //   {
				   //      xCoord = m_pInletLUT->GetAsDouble(1, inletRow);
				   //      yCoord = m_pInletLUT->GetAsDouble(2, inletRow);
				   //      int bayFloodedCount = 0;
				   //      bayFloodedCount = m_pInletLUT->GetAsInt(4, inletRow);

				int numRows = m_pFloodedGrid->GetRowCount();
				int numCols = m_pFloodedGrid->GetColCount();

				//for (MapLayer::Iterator bayPt = m_pBayTraceLayer->Begin(); bayPt < m_pBayTraceLayer->End(); bayPt++)
				//   {
				//   m_pBayTraceLayer->GetData(bayPt, colEasting, xCoord);
				//   m_pBayTraceLayer->GetData(bayPt, colNorthing, yCoord);
				//
				//   m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
				//
				//   if ((startRow >= 0 && startRow < numRows) && (startCol >= 0 && startCol < numCols))
				//      {
				//      m_pElevationGrid->GetData(startRow, startCol, minElevation);
				//      m_pFloodedGrid->GetData(startRow, startCol, flooded);
				//
				//      bool isFlooded = (flooded > 0.0f) ? true : false;
				//      // for the cell corresponding to bay point coordinate
				//      if (!isFlooded && (yearlyMaxTWL > duneCrest) && (yearlyMaxTWL > minElevation) && (minElevation >= 0.0))
				//         {
				//         //m_pFloodedGrid->SetData(startRow, startCol, yearlyMaxTWL);
				//         //floodedCount++;
				//         //floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL); //, minElevation);
				//         }
				//      } // end inner bay type 
				//   } // end bay inlet pt
			} // end bay inlet flooding 
			else if (beachType != BchT_UNDEFINED && beachType != BchT_RIVER)
			{
				m_pDuneLayer->GetData(point, m_colDLDuneCrest, duneCrest);
				//m_pDuneLayer->GetData(point, m_colDLEastingCrest, xCoord);
				//m_pDuneLayer->GetData(point, m_colNorthingCrest, yCoord);

				  // use Northing location of Dune Toe and Easting location of the Dune Crest
				m_pDuneLayer->GetPointCoords(point, xCoord, yCoord);
				m_pDuneLayer->GetData(point, m_colDLEastingCrest, xCoord);

				m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
				m_pDuneLayer->GetData(point, m_colDLYrFMaxTWL, yearlyMaxTWL);
				//yearlyMaxTWL -= 0.23f;

				// Limit flooding to swl + setup for dune backed beaches (no swash)
				float swl = 0.0;
				m_pDuneLayer->GetData(point, m_colDLYrMaxSWL, swl);
				float eta = 0.0;
				m_pDuneLayer->GetData(point, m_colDLYrMaxSetup, eta);
				float STK_IG = 0.0;

				yearlyMaxTWL = swl + 1.1f * (eta + STK_IG / 2.0f);

				//if (duneIndex == 3191)
				//{
				// within grid ?

				if ((startRow >= 0 && startRow < numRows) && (startCol >= 0 && startCol < numCols))
				{
					//m_pFloodedGrid->GetData(startRow, startCol, minElevation);
					m_pFloodedGrid->GetData(startRow, startCol, flooded);

					bool isFlooded = (flooded > 0.0f) ? true : false;
					if (!isFlooded && (yearlyMaxTWL > duneCrest) && (yearlyMaxTWL > minElevation) && (minElevation >= 0))
					{
						//m_pFloodedGrid->SetData(startRow, startCol, yearlyMaxTWL);
						//floodedCount++;
						//   floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL, minElevation);
						//floodedCount += VisitNeighbors(startRow, startCol, yearlyMaxTWL);
					}
				}
				//}
			} // end shoreline pt
		} // end all duneline pts
		///////
		///////   m_pFloodedGrid->ResetBins();
		///////   m_pFloodedGrid->AddBin(0, RGB(255, 255, 255), "Not Flooded", TYPE_FLOAT, -0.1f, 0.09f);
		///////   m_pFloodedGrid->AddBin(0, RGB(0, 0, 255), "Flooded", TYPE_FLOAT, 0.1f, 100.0f);
		///////   m_pFloodedGrid->ClassifyData();
		///////   //  m_pFloodedGrid->Show();
		///////   m_pFloodedGrid->Hide();
	}

	return true;
}




bool ChronicHazards::RunEelgrassModel(EnvContext* pEnvContext)
{
	//////   float avgSWL = 0.0f;
	//////   float avgLowSWL = 0.0f;
	//////
	//////   MapLayer::Iterator pt = m_pDuneLayer->Begin();
	//////   m_pDuneLayer->GetData(pt, m_colYrAvgSWL, avgSWL);
	//////   m_pDuneLayer->GetData(pt, m_colYrAvgLowSWL, avgLowSWL);
	//////
	//////   float avgWL = (avgSWL + avgLowSWL) / 2.0f;
	//////
	//////   if (pEnvContext->yearOfRun == 0)
	//////      {
	//////      m_avgWL0 = avgWL;
	//////      }
	//////
	//////   m_delta = avgWL - m_avgWL0;
	//////
	//////   //m_eelgrassArea = GenerateEelgrassMap(count);
	//////   //m_eelgrassAreaSqMiles = m_eelgrassArea * 3.8610216e-07;
	//////   //
	//////   //int intertidalCount = 0;
	//////   //for (int row = 0; row < m_numTidalRows; row++)
	//////   //   {
	//////   //   for (int col = 0; col < m_numTidalCols; col++)
	//////   //      {
	//////   //
	//////   //      float elevation = 0.0f;
	//////   //      m_pTidalBathyGrid->GetData(row, col, elevation);
	//////   //      if (elevation != -9999.0f)
	//////   //         {
	//////   //         if (elevation >= avgLowSWL && elevation <= avgSWL)
	//////   //            intertidalCount++;
	//////   //         }
	//////   //      }
	//////   //   }
	//////
	//////   //m_intertidalArea = intertidalCount * m_elevCellHeight * m_elevCellWidth;
	//////   //m_intertidalAreaSqMiles = m_intertidalArea * 3.8610216e-07;
	return true;
}



bool ChronicHazards::RunPolicyManagement(EnvContext* pEnvContext)
{
	// Get new IDUBuildingsLkUp every year !
	m_pIduBuildingLkUp->BuildIndex();

	for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
	{
		//Associate new buildings with communities
		// get Buildings in this IDU
		CArray< int, int > ptArray;

		//int iduCityCode = 0;
		//m_pIDULayer->GetData(idu, m_colIDUCityCode, iduCityCode);

		int numBldgs = m_pIduBuildingLkUp->GetPointsFromPolyIndex(idu, ptArray);

		for (int i = 0; i < numBldgs; i++)
		{
			int bldgIndex = ptArray[i];

			/*int bldgCityCode = 0;
			m_pBldgLayer->GetData(ptArray[i], m_colBldgCityCode, bldgCityCode);*/

			//m_pBldgLayer->SetData(bldgIndex, m_colBldgCityCode, iduCityCode);
		} // end each new building
	}


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

	return true;
} // end RunPolicies(EnvContext *pEnvContext)


bool ChronicHazards::EndRun(EnvContext* pEnvContext)
{
	if (fpPolicySummary != nullptr)
		fclose(fpPolicySummary);

	return TRUE;
}




bool ChronicHazards::LoadRBFs()
{
	FILE* fpRBF = nullptr;

	CString rbfPath = m_twlInputPath + "SWAN_RBF_Params.rbf";

	m_randIndex = m_simulationCount;

	CPath _rbfPath(rbfPath);
	bool writeRBFfile = true;
	if (_rbfPath.Exists())
		writeRBFfile = false;

	// Parameterizing the RBF's from data and save to disk
	if (writeRBFfile)
	{
		if (fopen_s(&fpRBF, rbfPath, "wb") != 0)
		{
			Report::ErrorMsg("ChronicHazards: Unable to open RBF serialization file for writing");
			return false;
		}
	}
	else // are we loading from disk?
	{
		if (fopen_s(&fpRBF, rbfPath, "rb") != 0)
		{
			Report::ErrorMsg("ChronicHazards: Unable to open RBF serialization file for reading");
			return false;
		}
	}

	bool writeDailyBouyData = (m_writeDailyBouyData == 0);// ? true : false;
	m_minTransect = 0;
	m_maxTransect = m_numTransects - 1;

	if (writeRBFfile) // || writeDailyBouyData)
		Report::LogInfo("Generating RBFs");
	else
	{
		CString msg;
		msg.Format("Loading RBF file %s", (LPCTSTR)rbfPath);
		Report::LogInfo(msg);
	}

	int colTransID = m_pTransectLayer->GetFieldCol("TransID");
	ASSERT(colTransID >= 0);

	// Either read in the rbf file or generate the rbf file
	for (int i = m_minTransect; i <= m_maxTransect; i++)
	{
		//int ypNum = m_pTransectLUT->GetAsInt(0, i);
		int transID = -1;
		m_pTransectLayer->GetData(i, colTransID, transID);

		// create a lookup table
		LookupTable* pLookupTable = new LookupTable;

		bool success = true;

		// generate RBFs from data files
		if (writeRBFfile)
		{
			CString path(m_twlInputPath);
			path += "Lookup_Tables\\";

			CString file;

			file.Format("SWAN_lookup_Yp_%04i.csv", transID); // i);  //series of SWAN lookup tables (note ONE-based)
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

			fwrite((void*)&len, sizeof(int), 1, fpRBF);
			fwrite((void*)str.c_str(), sizeof(TCHAR), len, fpRBF);
		}
		else   // RBF already generated, just load from disk
		{
			// read the first int, it has the size of the serialization string
			int length;
			size_t count = fread(&length, sizeof(int), 1, fpRBF);

			if (count != 1)
				Report::ErrorMsg("ChronicHazards: Error reading RBF parameter header");

			TCHAR* buffer = new TCHAR[length];
			count = fread(buffer, sizeof(TCHAR), length, fpRBF);

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

	fclose(fpRBF);

	CString msg;
	msg.Format("Processed %i SWAN input files", (int)m_swanLookupTableArray.GetSize());
	Report::LogInfo(msg);

	return true;
}

bool ChronicHazards::ReadDailyBayData(LPCTSTR timeSeriesFolder)
{
	/*

	// if randomize, reread in
	//  m_DailyBayDataArray.Clear();

	for (int i = 0; i < m_numBayStations; i++)
	   {
	   bool success = true;
	   FDataObj* pBayDaily = new FDataObj;
	   pBayDaily->SetSize(1, m_numDays);

	   CString timeSeriesFile;
	   timeSeriesFile.Format("%s\\DailyBayData_%i.csv", timeSeriesFolder, i);

	   int numRows = pBayDaily->ReadAscii(timeSeriesFile);

	   if (numRows < 0)
		  success = false;
	   else
	   {
	   CString msg("ChronicHazards: processing daily bay data input file ");
	   msg += folder;
	   Report::Log(msg);
	   }

	   if (success)
		  this->m_DailyBayDataArray.Add(pBayDaily);
	   else
		  delete pBayDaily;
	   }

	CString msg;
	msg.Format("ChronicHazards: Processed DailyBayData input files for %i locations", (int)m_DailyBayDataArray.GetSize());
	Report::Log(msg);
	*/
	return true;
}


int ChronicHazards::SetInfrastructureValues()
{
	if (m_pBldgLayer == nullptr || m_pInfraLayer == nullptr)
		return -1;

	for (int i = 0; i < m_pInfraLayer->GetRecordCount(); i++)
	{
		Poly* pPolyI = m_pInfraLayer->GetPolygon(i);

		float closestDist = 100000000;
		int closestBldg = -1;

		for (int j = 0; j < m_pBldgLayer->GetRecordCount(); j++)
		{
			Poly* pPolyB = m_pBldgLayer->GetPolygon(j);

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

// This function converts hourly data to daily data and stores daily data in m_buoyObsdata
// Returns total number of days
// fullPath stores full file path of the hourly data.
int ChronicHazards::ConvertHourlyBuoyDataToDaily(LPCTSTR fullPath)
{
	m_buoyObsData.SetSize(TOTAL_BUOY_COL, 0);     // 9 cols, 0 rows
	m_buoyObsData.SetLabel(TIME_STEP, "Time");
	m_buoyObsData.SetLabel(WAVE_HEIGHT_HS, "Offshore_Height");
	m_buoyObsData.SetLabel(WAVE_PERIOD_Tp, "Offshore_Period");
	m_buoyObsData.SetLabel(WAVE_DIRECTION_Dir, "Wave_Direction");
	m_buoyObsData.SetLabel(WATER_LEVEL_WL, "Offshore_WL");
	m_buoyObsData.SetLabel(WAVE_SETUP, "Wavesetup");
	m_buoyObsData.SetLabel(INFRAGRAVITY, "Infragravity");
	m_buoyObsData.SetLabel(WAVEINDUCEDWL, "Waveinducedwl");
	m_buoyObsData.SetLabel(TWL, "TWL");

	VDataObj hourlyData(5, 0, UNIT_MEASURE::U_HOURS);
	int numrow = hourlyData.ReadAscii(fullPath);  // NOTE:  Run scripts/PrepBuoyData.py if necessary to strip leap days

	// We are assuming the data is complete
	assert(numrow % HOURLY_DATA_IN_A_DAY == 0);
	int numdays = numrow / HOURLY_DATA_IN_A_DAY;
	int cols = hourlyData.GetColCount();
	int row = 0;
	VData* data = new VData[TOTAL_BUOY_COL];

	Report::Log_i("Converting hourly buoy data to daily for %i days", numdays);

	for (int i = 0; i < numdays; i++)
	{
		// iterate through days (blocks of 24 hours), looking for the 
		// row (hour) that has the highest TWL, and remember that row
		VData date;
		hourlyData.Get(0, row, date);
		float maxWaterlevel = -100;
		int maxRow = -1;
		for (int j = 0; j < HOURLY_DATA_IN_A_DAY; j++)
		{
			//int rowindex = i * HOURLY_DATA_IN_A_DAY + j;
			float waveheight = hourlyData.GetAsFloat(WAVE_HEIGHT_HS, row);
			float waveperiod = hourlyData.GetAsFloat(WAVE_PERIOD_Tp, row);
			float waterlevel = hourlyData.GetAsFloat(WATER_LEVEL_WL, row);
			float wavedir = hourlyData.GetAsFloat(WAVE_DIRECTION_Dir, row);

			//Static Setup Stockdon et al (2006) formulation tanb1 is assumed as 0.05; eta = (0.35f * tanb1 * sqrt(Hdeep * L0));
			float setup = 0.35f * TANB1_005 * sqrt(waveheight * (G * waveperiod * waveperiod) / (2 * PI));
			// Infragravity Swash, Stockdon (2006) Eqn: 12 [IG = 0.06f * sqrt(Hdeep * L0)]
			float infragravity = 0.06f * sqrt((waveheight * (G * waveperiod * waveperiod) / (2 * PI)));
			float waveinducedWL = 1.1f * (setup + (infragravity / 2));  //wave induced water level
			float totalWL = waterlevel + waveinducedWL;

			if (totalWL > maxWaterlevel) {
				maxRow = row;
				maxWaterlevel = totalWL;

				////data[TIME_STEP] = i;
				////data[WAVE_HEIGHT_HS] = waveheight;
				////data[WAVE_PERIOD_Tp] = waveperiod;
				////data[WAVE_DIRECTION_Dir] = wavedir;
				////data[WATER_LEVEL_WL] = waterlevel;
				////
				data[INFRAGRAVITY] = infragravity;
				data[WAVE_SETUP] = setup;
				data[WAVEINDUCEDWL] = waveinducedWL;
				data[TWL] = totalWL;
			}

			row++;
		}

		// found the row (hour) to use for the day, add the daily values to the buoy data object
		for (int col = 0; col < cols; col++)
			hourlyData.Get(col, maxRow, data[col]);

		m_buoyObsData.AppendRow(data, TOTAL_BUOY_COL);
	}

	delete[] data;
	//assert(m_buoyObsData.GetRowCount() == numdays);
	return numdays;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Find average yealy maximum TWL data correposnding to 1390 transects and write the TWL, setup,
//infragravity, incident swash values
////////////////////////////////////////////////////////////////////////////////////////////////
int ChronicHazards::MaxYearlyTWL(EnvContext* pEnvContext)
{

	CString simulationPath;
	simulationPath.Format("%s\\Climate_Scenarios\\%s\\Simulation_1\\DailyData_%s", (LPCTSTR)m_twlInputPath,
		(LPCTSTR)m_climateScenarioStr, (LPCTSTR)m_climateScenarioStr);

	m_maxTransect = 1389;// TODO: remove this hard coding
	// m_maxTransect = m_numTransects - 1;
	FDataObj* transectdata = new FDataObj[m_maxTransect + 1];// declaring daily data object
	int rows;
	int TWLCol;
	int setupCol;
	int infragravityCol;
	int incidentbandswashCol;
	// reading 1390 transect data at ~25m depth
	for (int i = 0; i <= m_maxTransect; i++)
	{
		CString transectDataFile;
		transectDataFile.Format("%s\\DailyData_%i.csv", (CString)simulationPath, i);
		rows = transectdata[i].ReadAscii(transectDataFile);
		assert(rows != 0);
		setupCol = transectdata[i].AppendCol("Setup");
		infragravityCol = transectdata[i].AppendCol("infragravity");
		incidentbandswashCol = transectdata[i].AppendCol("Incident band swash");
		TWLCol = transectdata[i].AppendCol("TWL");
	}

	const int num_days_in_year = 365;

	int yearIndex = 0;

	//declaring vector of pairs for storing average TWL for each doy
	pair <double, int>  avg_twl[num_days_in_year];
	// calculate average TWL for all 1390 transects each doy
	for (int doy = 0; doy < num_days_in_year; doy++)
	{
		double avgTWL = 0;
		int rowindex = (yearIndex * num_days_in_year) + doy; // TODO: We will use this to access later year's data

		// collecting still water level from buoy data
		float swl = 0.0f;
		m_buoyObsData.Get(BUOY_OBSERVATION_DATA::WATER_LEVEL_WL, rowindex, swl);
		float wlratio = (swl - (-1.5f)) / (4.5f + 1.5f);

		// For each transect find TWL and accumulate
		for (int i = 0; i <= m_maxTransect; i++)
		{
			float radPeakHeightL = 0, radPeakHeightH = 0, radPeakPeriodL = 0, radPeakPeriodH = 0, radPeakDirectionL = 0,
				radPeakDirectionH = 0, radPeakWaterLevelL = 0, radPeakWaterLevelH = 0;
			// LOAD twl or data to cal twl
		   // add to avg tWL
			transectdata[i].Get(TRANSECTDAILYDATA::HEIGHT_L, rowindex, radPeakHeightL);
			transectdata[i].Get(TRANSECTDAILYDATA::HEIGHT_H, rowindex, radPeakHeightH);
			transectdata[i].Get(TRANSECTDAILYDATA::PERIOD_L, rowindex, radPeakPeriodL);
			transectdata[i].Get(TRANSECTDAILYDATA::PERIOD_H, rowindex, radPeakPeriodH);
			transectdata[i].Get(TRANSECTDAILYDATA::DIRECTION_L, rowindex, radPeakDirectionL);
			transectdata[i].Get(TRANSECTDAILYDATA::DIRECTION_H, rowindex, radPeakDirectionH);

			float waveHeight = radPeakHeightL + wlratio * (radPeakHeightH - radPeakHeightL);
			float wavePeriod = radPeakPeriodL + wlratio * (radPeakPeriodH - radPeakPeriodL);
			float waveDirection = radPeakDirectionL + wlratio * (radPeakDirectionH - radPeakDirectionL);

			float L0, setup, infragravitySwash, incidentSwash, twl;
			ComputeStockdon(swl, waveHeight, wavePeriod, waveDirection, &L0, &setup, &incidentSwash, &infragravitySwash, nullptr, &twl);

			// Storing setup, infragravity, incident band swash, and TWL
			transectdata[i].Set(setupCol, rowindex, setup);
			transectdata[i].Set(infragravityCol, rowindex, infragravitySwash);
			transectdata[i].Set(incidentbandswashCol, rowindex, incidentSwash);
			transectdata[i].Set(TWLCol, rowindex, twl);
			avgTWL += twl;      // accumulate TWL values over all transects.
		}

		avgTWL = avgTWL / (m_maxTransect + 1);

		// Store Avg TWL 
		avg_twl[doy] = pair<double, int>(avgTWL, doy);

	}
	// sorting maximum average TWL 
	for (int i = 0; i < num_days_in_year - 1; i++)
	{
		int max_idx = i;
		int j = -1;
		for (j = i + 1; j < num_days_in_year; j++)
			if (avg_twl[j].first > avg_twl[max_idx].first)
				max_idx = j;
		if (max_idx != i)
			avg_twl[max_idx].swap(avg_twl[i]);
	}
	// write sorted average TWL in file
	CString twlpath;
	twlpath.Format("%s\\avgTwl.txt", simulationPath);

	FILE* fp;
	fopen_s(&fp, twlpath, "w");
	for (auto avg_twl : avg_twl) {
		fprintf(fp, "%lf: %d\n", avg_twl.first, avg_twl.second);
	}
	fclose(fp);
	//write corresponding data for the doys with maximum average TWL for N (MAX_DAYS_FOR_FLOODING) events
	for (int i = 0; i < MAX_DAYS_FOR_FLOODING; i++)
	{
		CString maxEventDataPath;
		maxEventDataPath.Format("%s\\MaxTWL_Year_%d_Doy_%d.csv", simulationPath, yearIndex, (avg_twl[i].second + 1));
		FDataObj twlAtTransects(4, m_maxTransect + 1);
		twlAtTransects.SetLabel(0, "Setup");
		twlAtTransects.SetLabel(1, "infragravity");
		twlAtTransects.SetLabel(2, "Incident band swash");
		twlAtTransects.SetLabel(3, "TWL");
		for (int j = 0; j < m_maxTransect + 1; j++)
		{
			twlAtTransects.Set(0, j, transectdata[j].Get(setupCol, avg_twl[i].second)); // Write Setup
			twlAtTransects.Set(1, j, transectdata[j].Get(infragravityCol, avg_twl[i].second)); // Write infragravity
			twlAtTransects.Set(2, j, transectdata[j].Get(incidentbandswashCol, avg_twl[i].second)); //write incident band swash
			twlAtTransects.Set(3, j, transectdata[j].Get(TWLCol, avg_twl[i].second)); // write maximum TWL
		}
		twlAtTransects.WriteAscii(maxEventDataPath);
	}


	delete[] transectdata;
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




bool ChronicHazards::LoadDailyRBFOutputs(LPCTSTR simulationPath)
{
	// This function generates a set of daily interpolated (from offshore buoy data) the nearshore wave environment.
	// It does so by either (depending on the m_writeDailyBuoyObs:
	//    1) generating a series of data objects by running the RBF interpolator, storing results in a set of time-series
	//       data objects. one per transect, containing daily data
	//    2) reading in a set of pre-defined daily RBF outputs CSV 
	// if randomize, reread in
	//  m_dailyRBFOutputArray.Clear();
	this->m_dailyRBFOutputArray.Clear();

	const int outputCount = 8;
	double outputs[outputCount];

	// load daily RBF outputs for this year from the time series folder.
	CString timeSeriesFolder, fullPath;
	timeSeriesFolder.Format("%s\\DailyData_%s", (LPCTSTR)simulationPath, (LPCTSTR)m_climateScenarioStr);
	PathManager::FindPath(timeSeriesFolder, fullPath);

	//m_minTransect = 0;
	//m_maxTransect = m_numTransects - 1;

	// ??? Need to update to get correct transects?
	for (int i = m_minTransect; i <= m_maxTransect; i++)
	{
		bool success = true;
		FDataObj* pDailyRBFOutput = new FDataObj;
		pDailyRBFOutput->SetSize(outputCount, m_numDays);

		//   extension.Format("_%i", m_randIndex);
		//  CString folder = m_datafile.timeseries_file + extension + "\\";
		//PathManager::FindPath(folder, path);
		// folder = path + folder;

		CString timeSeriesFile;
		timeSeriesFile.Format(_T("%s%s\\DailyData_%i.csv"), m_twlInputPath, timeSeriesFolder, i);     //series of SWAN lookup tables

		// if we are writing this data, that means we have to generate it from the RBFs
		// otherwise, we can just read it from files stored in the climate scenario/daily data folder
		int(m_writeDailyBouyData) = 0;
		if (m_writeDailyBouyData)
		{
			CString msg;
			msg.Format("Generating RBF Output %i of %i, file: %s", i - m_minTransect, m_maxTransect - m_minTransect, timeSeriesFile);
			Report::StatusMsg(msg);

			// create a data obj for buoy observations, rows=day of year
			pDailyRBFOutput->SetLabel(0, "Height_L");
			pDailyRBFOutput->SetLabel(1, "Height_H");
			pDailyRBFOutput->SetLabel(2, "Period_L");
			pDailyRBFOutput->SetLabel(3, "Period_H");
			pDailyRBFOutput->SetLabel(4, "Direction_L");
			pDailyRBFOutput->SetLabel(5, "Direction_H");
			pDailyRBFOutput->SetLabel(6, "Depth_L");
			pDailyRBFOutput->SetLabel(7, "Depth_H");

			// the lookup table contains the RBF for a given transect
			LookupTable* pLUT = m_swanLookupTableArray.GetAt(i - m_minTransect);   // zero-based

			// for each day over the SIMULATION PERIOD (not year), get the buoy observations 
			// and run the RBF with them
			for (int row = 0; row < m_numDays; row++)
			{
				float height, period, direction, swl;

				m_buoyObsData.Get(BUOY_OBSERVATION_DATA::WAVE_HEIGHT_HS, row, height);
				m_buoyObsData.Get(BUOY_OBSERVATION_DATA::WAVE_PERIOD_Tp, row, period);
				m_buoyObsData.Get(BUOY_OBSERVATION_DATA::WAVE_DIRECTION_Dir, row, direction);
				m_buoyObsData.Get(BUOY_OBSERVATION_DATA::WATER_LEVEL_WL, row, swl);   // NOT USED???

				int count = pLUT->m_rbf.GetAt(height, period, direction, outputs);
				ASSERT(count == outputCount);

				// added this because rbf isn't doing well with small ( < 1) values
				// for wave heights; later this becomes a problem with a negative wave height
				if (outputs[HEIGHT_L] < 0.0f)
					outputs[HEIGHT_L] = 0.0f;

				if (outputs[HEIGHT_H] < 0.0f)
					outputs[HEIGHT_H] = 0.0f;

				// store the buoy data in the data object for this day
				pDailyRBFOutput->Set(0, row, (float)outputs[HEIGHT_L]);
				pDailyRBFOutput->Set(1, row, (float)outputs[HEIGHT_H]);
				pDailyRBFOutput->Set(2, row, (float)outputs[PERIOD_L]);
				pDailyRBFOutput->Set(3, row, (float)outputs[PERIOD_H]);
				pDailyRBFOutput->Set(4, row, (float)outputs[DIRECTION_L]);
				pDailyRBFOutput->Set(5, row, (float)outputs[DIRECTION_H]);
				pDailyRBFOutput->Set(6, row, (float)outputs[DEPTH_L]);
				pDailyRBFOutput->Set(7, row, (float)outputs[DEPTH_H]);
			}

			// dataobj populated, write to file
			pDailyRBFOutput->WriteAscii(timeSeriesFile);
		}
		else // writeDailyBuoyData != 0, so read the data from file
		{
			if (i % 10 == 0)
			{
				CString msg;
				msg.Format("Reading RBF Output %i of %i, file: %s", i - m_minTransect, m_maxTransect - m_minTransect, timeSeriesFile);
				Report::StatusMsg(msg);
			}

			int numRows = pDailyRBFOutput->ReadAscii(timeSeriesFile);

			if (numRows < 0)
				success = false;
		}

		if (success)
			this->m_dailyRBFOutputArray.Add(pDailyRBFOutput);
		else
			delete pDailyRBFOutput;
	}

	CString msg;
	msg.Format("Processed Daily RBF input files for generating TWLs for %i transects", (int)m_dailyRBFOutputArray.GetSize());
	Report::Log(msg);
	//m_randIndex += 1; // add to xml, only want when we are running probabalistically

	return true;

} // end LoadDailRBFOutputs()


bool ChronicHazards::ResetAnnualVariables()
{
	m_pDuneLayer->m_readOnly = false;
	m_pDuneLayer->SetColData(m_colDLYrFMaxTWL, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLYrMaxTWL, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLYrAvgTWL, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLYrAvgSWL, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLYrAvgLowSWL, VData(0.0f), true);

	m_pDuneLayer->SetColData(m_colDLFlooded, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLAmtFlooded, VData(0.0), true);

	m_pDuneLayer->SetColData(m_colDLYrMaxDoy, VData(-1), true);
	m_pDuneLayer->SetColData(m_colDLHdeep, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLWPeriod, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLWDirection, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLWHeight, VData(0.0f), true);

	m_pDuneLayer->SetColData(m_colDLYrMaxSetup, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLYrMaxSWL, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLYrMaxIGSwash, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLYrMaxIncSwash, VData(0.0f), true);
	m_pDuneLayer->SetColData(m_colDLYrMaxSwash, VData(0.0f), true);

	m_pDuneLayer->SetColData(m_colDLIDDToe, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLIDDToeWinter, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLIDDToeSpring, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLIDDToeSummer, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLIDDToeFall, VData(0), true);

	m_pDuneLayer->SetColData(m_colDLIDDCrest, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLIDDCrestWinter, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLIDDCrestSpring, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLIDDCrestSummer, VData(0), true);
	m_pDuneLayer->SetColData(m_colDLIDDCrestFall, VData(0), true);

	// Reset annual attributes for Building, Infrastructure and Roads
	////////////////m_pBldgLayer->SetColData(m_colBldgFlooded, VData(0), true);
	////////////////
	////////////////m_pInfraLayer->SetColData(m_colInfraFlooded, VData(0), true);
	//////////////////   m_pInfraLayer->SetColData(m_colInfraEroded, VData(0), true);
	////////////////m_pRoadLayer->SetColData(m_colRoadFlooded, VData(0), true);
	////////////////
	////////////////// Initialize or Reset annual statistics for the whole coastline/study area
	////////////////
	////////////////// Reset annual County Wide variables to zero
	////////////////
	////////////////m_meanTWL = 0.0f;
	////////////////m_floodedArea = 0.0f;         // square meters, m_floodedAreaSqMiles (sq Miles) is calculated from floodedArea
	////////////////                              //m_maxTWL = 0.0f;
	////////////////
	////////////////                              // Hard Protection Metrics
	////////////////m_noConstructedBPS = 0;
	////////////////m_constructCostBPS = 0.0;
	////////////////m_maintCostBPS = 0.0;
	////////////////m_nourishCostBPS = 0.0;
	////////////////m_nourishVolumeBPS = 0.0;
	////////////////m_totalHardenedShoreline = 0.0;         // meters
	////////////////m_totalHardenedShorelineMiles = 0.0;   // miles
	////////////////m_addedHardenedShoreline = 0.0;         // meters
	////////////////m_addedHardenedShorelineMiles = 0.0;   // miles
	////////////////
	////////////////
	////////////////                                       // Soft Protection Metrics
	////////////////m_noConstructedDune = 0;
	////////////////m_restoreCostSPS = 0.0;
	////////////////m_maintCostSPS = 0.0;
	////////////////m_nourishCostSPS = 0.0;
	////////////////m_constructVolumeSPS = 0.0;
	////////////////m_maintVolumeSPS = 0.0;
	////////////////m_nourishVolumeSPS = 0.0;
	////////////////m_addedRestoredShoreline = 0.0;         // meters
	////////////////m_addedRestoredShorelineMiles = 0.0;   // miles
	////////////////m_totalRestoredShoreline = 0.0;         // meters
	////////////////m_totalRestoredShorelineMiles = 0.0;   // miles
	////////////////
	////////////////
	////////////////m_noBldgsRemovedFromHazardZone = 0;
	//////////////////m_noBldgsEroded = 0;
	////////////////// Beach accesibility
	//////////////////m_avgCountyAccess = 0.0f;
	//////////////////m_avgOceanShoresAccess = 0.0f;
	//////////////////m_avgWestportAccess = 0.0f;
	//////////////////m_avgJettyAccess = 0.0f;
	//////////////////
	//////////////////m_eelgrassArea = 0.0;
	//////////////////m_eelgrassAreaSqMiles = 0.0;
	//////////////////
	//////////////////m_intertidalArea = 0.0;
	//////////////////m_intertidalAreaSqMiles = 0.0;
	////////////////
	////////////////m_erodedRoad = 0.0;
	////////////////m_erodedRoadMiles = 0.0;
	////////////////m_floodedArea = 0.0;
	////////////////m_floodedAreaSqMiles = 0.0;
	////////////////m_floodedRailroadMiles = 0.0;
	////////////////m_floodedRoad = 0.0;
	////////////////m_floodedRoadMiles = 0.0;
	////////////////
	////////////////// m_erodedBldgCount = 0;
	////////////////// m_floodedBldgCount = 0;

	return true;
}


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
   //   if (m_pIduElevationGridLkUp f!= nullptr)
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
   //   MapLayer *pGridMapLayer = nullptr;
   //
   //   if (m_pMap != nullptr)
   //   {
   //      pGridMapLayer = m_pMap->AddLayer(LT_GRID);
   //      ASSERT(pGridMapLayer != nullptr);
   //   }
   //
   //   else
   //   {
   //      // get Map pointer
   //      m_pMap = m_pIDULayer->GetMapPtr();
   //      ASSERT(m_pMap != nullptr);
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
   //   filename.Format("PolyFloodedLkUp.pgl");
   //   pglFile.Append(filename);
   //
   //   // if .pgl file exists: read poly - grid - lookup table from file
   //   // else (if .pgl file does NOT exists): create poly - grid - lookup table and write to .pgl file
   //   if (pglFile.Exists())
   //   {
   //      msg = "Loading PolyGrid from File ";
   //      msg += pglFile;
   //      Report::Log(msg);
   //
   //      m_pIduErodedGridLkUp = new PolyGridMapper(pglFile);
   //
   //      // if existing file has wrong dimension, create NEW poly - grid - lookup table and write to .pgl file
   //      if ((m_pIduElevationGridLkUp->GetNumGridCols() != m_numCols) || (m_pIduErodedGridLkUp->GetNumGridRows() != m_numRows))
   //      {
   //         msg = "Invalid dimensions: Recreating File ";
   //         msg += pglFile;
   //         Report::Log(msg);
   //         pRoadErodedGridLkUp = new PolyGridMapper(m_pRoadLayer, m_pElevationGrid, 1);
   //         //      m_pIduElevationGridLkUp = new PolyGridLookups(pGridMapLayer, m_pIDULayer, 1, maxEls, 0, -9999);
   //         m_pIduErodedGridLkUp->WriteToFile(pglFile);
   //      }
   //   }
   //   else  // doesn't exist, create it
   //   {
   //      msg = "Generating PolyGrid ";
   //      msg += pglFile;
   //      Report::Log(msg);
   //      m_pPolylineGridLkUp = new PolyGridMapper(m_pRoadLayer, m_pElevationGrid, 1);
   //      //    m_pIduElevationGridLkUp = new PolyGridLookups(pGridMapLayer, m_pIDULayer, 1, maxEls, 0, -9999);
   //      m_pIduErodedGridLkUp->WriteToFile(pglFile);
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
   //   if (pPolyGridLookup != nullptr)
   //      return true;
   //
   //   if (pGridLayer == nullptr)
   //      return false;
   //
   //   if (pPolyLayer == nullptr)
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
   //   MapLayer *pGridMapLayer = nullptr;
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
   //      ASSERT(m_pMap != nullptr);
   //      pGridMapLayer = m_pMap->AddLayer(LT_GRID);
   //      ASSERT(pGridMapLayer != nullptr);
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
   //   CString pglPath = nullptr;
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
   ///////   if (m_pFloodedGrid == nullptr)
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
   ///////   for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
   ///////      {
   ///////      int beachType = 0;
   ///////      m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
   ///////
   ///////      int duneIndex = -1;
   ///////      m_pDuneLayer->GetData(point, m_colDLDuneIndex, duneIndex);
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
   ///////         m_pDuneLayer->GetData(point, m_colYrFMaxTWL, yearlyMaxTWL);
   ///////
   ///////         //if (pEnvContext->yearOfRun < m_windowLengthTWL - 1)
   ///////         //   m_pDuneLayer->GetData(point, m_colYrAvgTWL, yearlyMaxTWL);
   ///////         //else
   ///////         //   m_pDuneLayer->GetData(point, m_colMvMaxTWL, yearlyMaxTWL);
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
   ///////         m_pDuneLayer->GetData(point, m_colDLDuneCrest, duneCrest);
   ///////         //m_pDuneLayer->GetData(point, m_colDLEastingCrest, xCoord);
   ///////         //m_pDuneLayer->GetData(point, m_colNorthingCrest, yCoord);
   ///////
   ///////           // use Northing location of Dune Toe and Easting location of the Dune Crest
   ///////         m_pDuneLayer->GetPointCoords(point, xCoord, yCoord);
   ///////         m_pDuneLayer->GetData(point, m_colDLEastingCrest, xCoord);
   ///////
   ///////         m_pElevationGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);
   ///////         m_pDuneLayer->GetData(point, m_colYrFMaxTWL, yearlyMaxTWL);
   ///////         //yearlyMaxTWL -= 0.23f;
   ///////
   ///////         // Limit flooding to swl + setup for dune backed beaches (no swash)
   ///////         float swl = 0.0;
   ///////         m_pDuneLayer->GetData(point, m_colYrMaxSWL, swl);
   ///////         float eta = 0.0;
   ///////         m_pDuneLayer->GetData(point, m_colYrMaxSetup, eta);
   ///////         float STK_IG = 0.0;
   ///////
   ///////         yearlyMaxTWL = swl + 1.1f*(eta + STK_IG / 2.0f);
   ///////
   ///////         //if (duneIndex == 3191)
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
   ///////            //   if (duneIndex == 1950 || duneIndex == 1951 || duneIndex == 1952 || duneIndex == 1953)
   ///////            //   {
   ///////            //      yearlyMaxTWL -= 0.49f;
   ///////            //   }
   ///////
   ///////               //if (duneIndex > 3020 && duneIndex < 3100)
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
   ///////   //      int size = m_pIduElevationGridLkUp->GetPolyCntForGridPt(row, col);
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
   ///////   //      int *polyIndexs = new int[size];
   ///////   //      int polyCount = m_pIduErodedGridLkUp->GetPolyNdxsForGridPt(row, col, polyIndexs);
   ///////   //      float *polyFractionalArea = new float[polyCount];
   ///////
   ///////   //      m_pIduElevationGridLkUp->GetPolyProportionsForGridPt(row, col, polyFractionalArea);
   ///////
   ///////   //      for (int idu = 0; idu < polyCount; idu++)
   ///////   //      {
   ///////   //         m_floodFreq.AddValue((float)isFlooded);
   ///////   //         m_pIDULayer->SetData(polyIndexs[idu], m_colFlooded, (int)isFlooded);
   ///////   //         //   if (pEnvContext->yearOfRun >= m_avgTime)
   ///////   //         //{
   ///////   //         //m_pDuneLayer->SetData(polyIndexs[idu], m_colFloodFreq, m_floodFreq.GetValue() * m_avgTime);
   ///////   //         //}
   ///////
   ///////   //         //Number of buildings that are flooded
   ///////   //         //         m_pIDULayer->SetData(polyIndexs[idu], m_colnumFbuildings, ++noBuildings);
   ///////   //      }
   ///////
   ///////   //      delete[] polyIndexs;
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
   ///////   floodedArea = m_cellWidth * m_elevCellHeight * floodedCount;
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

float ChronicHazards::GenerateErosionMap(int& erodedCount)
{
	ASSERT(m_pErodedGrid != nullptr);

	m_pErodedGrid->SetAllData(0, false);

	// Integer that indicates 1 if Eroded and 0 if not Eroded
	int eroded = 0;

	// area = grid cells that are Eroded multiplied by the number of grid cells 
	float ErodedArea = 0.0f;

	// clock_t start = clock();

	INT_PTR ptrArrayIndex = 0;

	// basic idea:  For each dune point, find it's location on the erosion grid
	// and population erosion grid where ever dunepoint experiences chronic of event erosion
	for (MapLayer::Iterator dunePt = m_pDuneLayer->Begin(); dunePt < m_pDuneLayer->End(); dunePt++)
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

		// find the grid cell corresponding to this dune point
		m_pDuneLayer->GetPointCoords(dunePt, xCoord, yCoord);
		m_pErodedGrid->GetGridCellFromCoord(xCoord, yCoord, row, col);

		// get the event erosion
		m_pDuneLayer->GetData(dunePt, m_colDLKD, eventErosion);
		float totalErosion = eventErosion + shorelineChange;

		//
		MovingWindow* eroKDMovingWindow = m_eroKDArray.GetAt(dunePt);

		float eroKD2yrExtent = eroKDMovingWindow->GetAvgValue();
		REAL xAvgKD = xCoord + eroKD2yrExtent;

		m_pDuneLayer->SetData(dunePt, m_colDLAvgKD, eroKD2yrExtent);

		REAL endXCoord = xCoord + totalErosion;
		int numRows = m_pErodedGrid->GetRowCount();
		int numCols = m_pErodedGrid->GetColCount();

		while (xCoord < endXCoord)
		{
			if ((row >= 0 && row < numRows) && (col >= 0 && col < numCols))
			{
				m_pDuneLayer->SetData(dunePt, m_colDLXAvgKD, xAvgKD);

				m_pErodedGrid->SetData(row, col, 1);
				erodedCount++;
				col++;
				m_pErodedGrid->GetGridCellCenter(row, col, xCoord, yCoord);
			}
			else
				xCoord = endXCoord;
		}

		m_pDuneLayer->GetPointCoords(dunePt, xCoord, yCoord);
		m_pErodedGrid->GetGridCellFromCoord(xCoord, yCoord, row, col);

		m_pDuneLayer->GetPointCoords(dunePt, xStartCoord, yCoord);
		while (xCoord > xStartCoord)
		{
			int eroded = 0;
			if ((row >= 0 && row < numRows) && (col >= 0 && col < numCols))
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
	// this function does the following:
	// 1) creates a new MovingWindow for erosion and flooding for each NEW building 
	//    and adds them to their respective collections
	// 2) for flooding, operates on the flooding grid to determine if the building has
	//    experienced flooding this year.  if so, updates the bldg point to indicate
	//    it has experienced flooding and set the building's flood frequency value
	// 3) for erosions, 



	// determine how many new buildings were added
	int numNewBldgs = m_pBldgLayer->GetRowCount() - m_numBldgs;
	m_numBldgs += numNewBldgs;

	// create moving windows for new buildings
	for (int i = 0; i < numNewBldgs; i++)
	{
		MovingWindow* eroFreqMovingWindow = new MovingWindow(m_windowLengthEroHzrd);
		m_eroBldgFreqArray.Add(eroFreqMovingWindow);

		MovingWindow* floodFreqMovingWindow = new MovingWindow(m_windowLengthFloodHzrd);
		m_floodBldgFreqArray.Add(floodFreqMovingWindow);

	} // end add MovingWindows for new buildings

 // update building stats related to flooding
	if (m_pFloodedGrid != nullptr)
	{
		int numRows = m_pFloodedGrid->GetRowCount();
		int numCols = m_pFloodedGrid->GetColCount();

		for (MapLayer::Iterator bldgIndex = m_pBldgLayer->Begin(); bldgIndex < m_pBldgLayer->End(); bldgIndex++)
		{
			MovingWindow* floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);

			REAL xCoord = 0.0;
			REAL yCoord = 0.0;

			float flooded = 0.0f;

			int startRow = -1;
			int startCol = -1;

			// is there actually a dwelling unit on this site?
			int duCount = 0;
			m_pBldgLayer->GetData(bldgIndex, m_colBldgNDU, duCount);
			bool isDeveloped = (duCount > 0) ? true : false;

			// get the location of the building point and corresponding cell on the flooded grid
			m_pBldgLayer->GetPointCoords(bldgIndex, xCoord, yCoord);
			m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

			// is the building located within the flooding grid ?
			if ((startRow >= 0 && startRow < numRows) && (startCol >= 0 && startCol < numCols))
			{
				//// Determine if building has been eroded by Bruun erosion
				//for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
				//{
				//   int duneBldgIndex = -1;
				//   m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, duneBldgIndex);

				//   CArray<int, int> iduIndices;

				//   if (bldg == duneBldgIndex)
				//   {
				//      double xDunePt = 0.0f;
				//      double yDunePt = 0.0f;
				//      m_pDuneLayer->GetPointCoords(point, xDunePt, yDunePt);
				//      if (xDunePt >= xCoord)
				//      {
				//         m_pBldgLayer->SetData(bldg, m_colBldgEroded, 1);

				//         int sz = m_pIduBuildingLkUp->GetPolysFromPointIndex(bldg, iduIndices);
				//         for (int i = 0; i < sz; i++)
				//         {
				//            int iduIndex = iduIndices[ 0 ];
				//            /*m_pIDULayer->SetData(iduIndex, m_colIDUPopDensity, 0);
				//            m_pIDULayer->SetData(iduIndex, m_colPopCap, 0);
				//            m_pIDULayer->SetData(iduIndex, m_colPopulation, 0);
				//            m_pIDULayer->SetData(iduIndex, m_colLandValue, 0);
				//            m_pIDULayer->SetData(iduIndex, m_colImpValue, 0);*/
				//         }
				//      }
				//   }
				//} // end DuneLine iterate

				m_pFloodedGrid->GetData(startRow, startCol, flooded);
				bool isFlooded = (flooded > 0.00001f) ? true : false;
				int removeYr = 0;
				m_pBldgLayer->GetData(bldgIndex, m_colBldgRemoveYr, removeYr);

				if (isFlooded && removeYr == 0)
				{
					if (isDeveloped)
						m_floodedBldgCount += duCount;
					else
						m_floodedBldgCount++;

					m_pBldgLayer->SetData(bldgIndex, m_colBldgFlooded, 1);
					floodMovingWindow->AddValue(1.0f);
					float floodFreq = floodMovingWindow->GetFreqValue();
					m_pBldgLayer->SetData(bldgIndex, m_colBldgFloodFreq, floodFreq);
				}
			}
		}
	} // end FloodedGrid

/*
   if (m_pBldgLayer != nullptr)
	  {
	  for (MapLayer::Iterator bldgIndex = m_pBldgLayer->Begin(); bldgIndex < m_pBldgLayer->End(); bldgIndex++)
		 {
		 MovingWindow* eroMovingWindow = m_eroBldgFreqArray.GetAt(bldgIndex);
		 //   ptrArrayIndex++;
		 REAL xCoord = 0.0;
		 REAL yCoord = 0.0;
		 int eroded = 0;
		 int erodedBldg = 0;
		 int startRow = -1;
		 int startCol = -1;
		 int duCount = 0;
		 m_pBldgLayer->GetData(bldgIndex, m_colBldgNDU, duCount);
		 bool isDeveloped = (duCount > 0) ? true : false;
		 // need to get all bldg from column before
		 //m_pBldgLayer->GetPointCoords(bldgIndex, xCoord, yCoord);
		 CArray<int,int> idus;
		 m_pIduBuildingLkUp->GetPolysFromPointIndex(bldgIndex, idus);
		 // has building previously eroded by chronic erosion
		 m_pBldgLayer->GetData(bldgIndex, m_colBldgEroded, erodedBldg);
		 // reset event eroded buildings
		 if (erodedBldg > 0)
			m_pBldgLayer->SetData(bldgIndex, m_colBldgEroded, 0);

//         // within grid ?
//         if ((startRow >= 0 && startRow < numRows) && (startCol >= 0 && startCol < numCols))
//            {
//            m_pErodedGrid->GetData(startRow, startCol, (int)eroded);
//
//            bool isEventEroded = (eroded == 1) ? true : false;
//            bool isChronicEroded = (eroded == 2) ? true : false;
	  for ( int i=0; i < idus.GetSize(); i++)
		 {
		 // Determine if building has been eroded by event erosion
		 // Tally buildings eroded by total erosion
		 int removeYr = 0;
		 m_pBldgLayer->GetData(bldgIndex, m_colBldgRemoveYr, removeYr);
		 if (isEventEroded && removeYr == 0)
			{
			float eroFreq = 0.0f;
			eroMovingWindow->AddValue(1.0f);
			m_pBldgLayer->SetData(bldgIndex, m_colBldgEroded, 1);
			m_pBldgLayer->GetData(bldgIndex, m_colBldgEroFreq, eroFreq);
			m_pBldgLayer->SetData(bldgIndex, m_colBldgEroFreq, ++eroFreq);
			if (isDeveloped)
				m_erodedBldgCount += duCount;
			else
			   m_erodedBldgCount++;
			}
		 //if (isChronicEroded && removeYr == 0)
		 //      {
		 //      m_pBldgLayer->SetData(bldg, m_colBldgEroded, 2);
		 //      //   m_noBldgsEroded++;
		 //      }
			}
		 }
	  }
*/
// deprecated? - no longer using eroded grids
   // if Building is eroded it is flagged and removed from future counts
	if (m_pErodedGrid != nullptr)
	{
		int numRows = m_pErodedGrid->GetRowCount();
		int numCols = m_pErodedGrid->GetColCount();

		for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
		{
			MovingWindow* eroMovingWindow = m_eroBldgFreqArray.GetAt(bldg);
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
			if ((startRow >= 0 && startRow < numRows) && (startCol >= 0 && startCol < numCols))
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
	if (m_runFlags & CH_MODEL_POLICY)
	{
		int cols = 1 + ((PI_COUNT + 1) * 7);
		ASSERT(cols == m_policyCostData.GetColCount());

		CArray<float, float> data;
		data.SetSize(cols);

		data[0] = (float)currentYear;

		int col = 1;

		float totals[7] = { 0,0,0,0,0,0,0 };

		for (int i = 0; i < PI_COUNT; i++)
		{
			PolicyInfo& pi = m_policyInfoArray[i];
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
}


void ChronicHazards::TallyDuneStatistics(int currentYear)
{
	//if (m_pFloodedGrid != nullptr)
	//{
	//   for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	//   {
	//      double xCoord = 0.0f;
	//      double yCoord = 0.0f;

	//      int flooded = 0;

	//      int startRow = -1;
	//      int startCol = -1;

	//      m_pDuneLayer->GetPointCoords(point, xCoord, yCoord);
	//      m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

	//      // within grid ?
	//      if ((startRow >= 0 && startRow < m_numRows) && (startCol >= 0 && startCol < m_numCols))
	//      {
	//         m_pFloodedGrid->GetData(startRow, startCol, flooded);
	//         bool isFlooded = (flooded == 1) ? true : false;

	//         if (isFlooded)
	//         {
	//            m_pDuneLayer->SetData(point, m_colFlooded, flooded);
	//         }
	//      }
	//   }
	//}

	int n = 0;
	int n_old = 0;

	int m = 0;
	int m_old = 0;

	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		int beachType = -1;
		m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);

		if (beachType == BchT_SANDY_RIPRAP_BACKED)
		{
			int yrAdded = 0;
			m_pDuneLayer->GetData(point, m_colDLAddYearBPS, yrAdded);
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
			m_pDuneLayer->GetData(point, m_colDLAddYearSPS, yrAdded);
			// count added this year
			if (yrAdded == currentYear)
				m++;
			else if (yrAdded != 0)
				// all the rest of constructed dunes, does not include existing SDB beaches
				m_old++;
		}
	}  // end of: for each DuneLine

 // determine length of BPS structures
	m_addedHardenedShoreline = float(m_elevCellHeight) * n;
	m_totalHardenedShoreline = m_addedHardenedShoreline + (float(m_elevCellHeight) * n_old);
	m_percentHardenedShoreline = (m_totalHardenedShoreline / float(m_shorelineLength)) * 100;

	m_addedHardenedShorelineMiles = m_addedHardenedShoreline * MI_PER_M;
	m_totalHardenedShorelineMiles = m_totalHardenedShoreline * MI_PER_M;
	m_percentHardenedShorelineMiles = m_percentHardenedShoreline * MI_PER_M;

	// determine length of constructed dunes
	m_addedRestoredShoreline = (float)m_elevCellHeight * m;
	m_totalRestoredShoreline = m_addedRestoredShoreline + ((float)m_elevCellHeight * m_old);
	m_percentRestoredShoreline = (m_totalRestoredShoreline / float(m_shorelineLength)) * 100;

	m_addedRestoredShorelineMiles = m_addedRestoredShoreline * MI_PER_M;
	m_totalRestoredShorelineMiles = m_totalRestoredShoreline * MI_PER_M;
	m_percentRestoredShorelineMiles = m_percentRestoredShoreline * MI_PER_M;

}

void ChronicHazards::ComputeFloodedIDUStatistics()
{
	m_pIDULayer->m_readOnly = false;

	if (m_pFloodedGrid != nullptr)
	{
		int numRows = m_pFloodedGrid->GetRowCount();
		int numCols = m_pFloodedGrid->GetColCount();

		for (int idu = 0; idu < m_pIduErodedGridLkUp->GetNumPolys(); idu++)
		{
			float fracFlooded = 0.0;
			int count = m_pIduErodedGridLkUp->GetGridPtCntForPoly(idu);

			if (count > 0)
			{
				ROW_COL* indexes = new ROW_COL[count];
				float* values = new float[count];

				m_pIduErodedGridLkUp->GetGridPtNdxsForPoly(idu, indexes);
				m_pIduErodedGridLkUp->GetGridPtProportionsForPoly(idu, values);

				for (int i = 0; i < count; i++)
				{

					int startRow = indexes[i].row;
					int startCol = indexes[i].col;

					if ((startRow >= 0 && startRow < numRows) && (startCol >= 0 && startCol < numCols))
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

				m_pIDULayer->SetData(idu, m_colIDUFracFlooded, fracFlooded);

				delete[] indexes;
				delete[] values;
			}
		}
	}
	m_pIDULayer->m_readOnly = true;

}  // end ComputeFloodedIDUStatistics

void ChronicHazards::ComputeIDUStatistics()
{
	ASSERT(m_pErodedGrid != nullptr);
	m_pIDULayer->m_readOnly = false;
	int numRows = m_pErodedGrid->GetRowCount();
	int numCols = m_pErodedGrid->GetColCount();


	// Lookup IDUs associated with ErodedGrid and set IDU attributes accordingly
	for (int row = 0; row < numRows; row++)
	{
		for (int col = 0; col < numCols; col++)
		{
			int eroded = 0;
			int noBuildings = 0;

			int size = m_pIduErodedGridLkUp->GetPolyCntForGridPt(row, col);

			m_pErodedGrid->GetData(row, col, (int)eroded);
			bool isIDUEroded = (eroded == 2) ? true : false;

			if (isIDUEroded)
			{
				// m_pBuildingGrid->GetData(row, col, buildings);

				int* polyIndexs = new int[size];
				int polyCount = m_pIduErodedGridLkUp->GetPolyNdxsForGridPt(row, col, polyIndexs);
				float* polyFractionalArea = new float[polyCount];

				m_pIduErodedGridLkUp->GetPolyProportionsForGridPt(row, col, polyFractionalArea);
				for (int idu = 0; idu < polyCount; idu++)
				{
					m_pIDULayer->SetData(polyIndexs[idu], m_colIDUEroded, eroded);
					m_pIDULayer->SetData(polyIndexs[idu], m_colIDUPopDensity, 0);  // zero out the population for eroded IDUs
				}
				delete[] polyIndexs;
				delete[] polyFractionalArea;
			}

		} //end columns of grid
	} // end rows of 

	m_pIDULayer->m_readOnly = true;
}

void ChronicHazards::TallyRoadStatistics()
{
	if (m_runFlags & CH_MODEL_INFRASTRUCTURE)
	{
		if (m_pRoadLayer != nullptr)
		{
			/************************   Calculate Flooded Road Statistics ************************/

			if (m_pFloodedGrid != nullptr)
			{
				int numRows = m_pFloodedGrid->GetRowCount();
				int numCols = m_pFloodedGrid->GetColCount();

				float noDataValue = m_pFloodedGrid->GetNoDataValue();

				for (int row = 0; row < numRows; row++)
				{
					for (int col = 0; col < numCols; col++)
					{

						int polyCount = m_pRoadFloodedGridLkUp->GetPolyCntForGridPt(row, col);

						/*if (polyCount > 0)
						{*/
						int* indices = new int[polyCount];
						int count = m_pRoadFloodedGridLkUp->GetPolyNdxsForGridPt(row, col, indices);

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
							REAL xMin0 = xCenter - (m_elevCellWidth / 2.0);
							REAL yMin0 = yCenter - (m_elevCellHeight / 2.0);
							REAL xMax0 = xCenter + (m_elevCellWidth / 2.0);
							REAL yMax0 = yCenter + (m_elevCellHeight / 2.0);

							/////int roadType = -1;
							/////Poly* pPoly = m_pRoadLayer->GetPolygon(indices[i]);
							/////m_pRoadLayer->GetData(indices[i], m_colRoadType, roadType);
							/////
							/////float flooded = 0.0f;
							/////m_pFloodedGrid->GetData(row, col, flooded);
							/////bool isFlooded = (flooded > 0.0f) ? true : false;
							/////
							/////if (isFlooded && flooded != noDataValue)
							/////   {
							/////
							/////   if (roadType != 6)
							/////      {
							/////      if (roadType == 7)
							/////         {
							/////         double lengthOfRailroad = 0.0;
							/////         lengthOfRailroad = DistancePolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), xMin0, yMin0, xMax0, yMax0);
							/////         m_floodedRailroadMiles += float(lengthOfRailroad);
							/////         m_pRoadLayer->SetData(indices[i], m_colRoadFlooded, 1);
							/////
							/////         }
							/////      else if (roadType != 5)
							/////         {
							/////         double lengthOfRoad = 0.0;
							/////         lengthOfRoad = DistancePolyLineInRect(pPoly->m_vertexArray.GetData(), pPoly->GetVertexCount(), ll.x, ll.y, ur.x, ur.y);
							/////         /*if (lengthOfRoad > 0 && m_debugOn)
							/////         {
							/////         CString thisMsg;
							/////         thisMsg.Format("Grid Cell: row=%i,  col==%i, xMin0=%7.3f,yMin0=%7.3f,xMax0=%7.3f,yMax0=%7.3f,",row,col,xMin0,yMin0,xMax0,yMax0);
							/////         Report::Log(thisMsg);
							/////         }*/
							/////         m_floodedRoad += float(lengthOfRoad);
							/////         m_pRoadLayer->SetData(indices[i], m_colRoadFlooded, 1);
							/////
							/////         }
							/////      }
							/////   }
						}
						delete[] indices;
					}
				}
			}
			// meters to miles: m *0.00062137 = miles
			m_floodedRoadMiles = m_floodedRoad * MI_PER_M;
			m_floodedRailroadMiles *= MI_PER_M;


			/************************   Calculate Eroded Road Statistics ************************/

			if (m_pErodedGrid != nullptr)
			{
				int numRows = m_pErodedGrid->GetRowCount();
				int numCols = m_pErodedGrid->GetColCount();

				for (int row = 0; row < numRows; row++)
				{
					for (int col = 0; col < numCols; col++)
					{
						int eroded = 0;
						m_pErodedGrid->GetData(row, col, (int)eroded);
						bool isEroded = (eroded == 2) ? true : false;

						if (isEroded)
						{
							int polyCount = m_pRoadFloodedGridLkUp->GetPolyCntForGridPt(row, col);

							if (polyCount > 0)
							{
								int* indices = new int[polyCount];
								int count = m_pRoadFloodedGridLkUp->GetPolyNdxsForGridPt(row, col, indices);

								double lengthOfRoad = 0.0;
								//   double lengthOfRailroad = 0.0;

								REAL xCenter = 0.0;
								REAL yCenter = 0.0;

								m_pErodedGrid->GetGridCellCenter(row, col, xCenter, yCenter);
								REAL xMin0 = xCenter - (m_elevCellWidth / 2.0);
								REAL yMin0 = yCenter - (m_elevCellHeight / 2.0);
								REAL xMax0 = xCenter + (m_elevCellWidth / 2.0);
								REAL yMax0 = yCenter + (m_elevCellHeight / 2.0);

								for (int i = 0; i < polyCount; i++)
								{
									int roadType = -1;
									Poly* pPoly = m_pRoadLayer->GetPolygon(indices[i]);
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
			m_erodedRoadMiles = m_erodedRoad * MI_PER_M;
			//   m_erodedRailroadMiles = m_erodedRailroad * MI_PER_M;
		}  // end RoadLayer exists
	}
}

void ChronicHazards::ComputeInfraStatistics()
{

	/************************   Calculate Eroded Infrastructure Statistics ************************/

	if (m_pErodedGrid != nullptr)
	{
		int numRows = m_pErodedGrid->GetRowCount();
		int numCols = m_pErodedGrid->GetColCount();

		for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
		{
			REAL xCoord = 0.0;
			REAL yCoord = 0.0;

			int eroded = 0;
			int erodedInfra = 0;

			int startRow = -1;
			int startCol = -1;

			int infraIndex = -1;
			m_pBldgLayer->GetData(infra, m_colInfraDuneIndex, infraIndex);

			MovingWindow* eroMovingWindow = m_eroInfraFreqArray.GetAt(infra);

			// need to get all points from column before
			m_pInfraLayer->GetPointCoords(infra, xCoord, yCoord);
			m_pErodedGrid->GetGridCellFromCoord(xCoord, yCoord, startRow, startCol);

			// has building infrastructure been previously eroded
			m_pInfraLayer->GetData(infra, m_colInfraEroded, erodedInfra);

			bool isInfraEroded = (erodedInfra == 1) ? true : false;

			// within grid ?
			if ((startRow >= 0 && startRow < numRows) && (startCol >= 0 && startCol < numCols))
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

			// Determine if infrastructure has been eroded by Bruun erosion
			for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
			{
				int duneIndex = -1;
				m_pDuneLayer->GetData(point, m_colDLDuneIndex, duneIndex);

				if (infraIndex == duneIndex)
				{
					REAL xDunePt = 0.0;
					REAL yDunePt = 0.0;
					m_pDuneLayer->GetPointCoords(point, xDunePt, yDunePt);
					if (xDunePt >= xCoord)
					{
						m_pInfraLayer->SetData(infra, m_colInfraEroded, 1);
					}
				}
			} // end DuneLine iterate

		}
	}
}

void ChronicHazards::ExportMapLayers(EnvContext* pEnvContext, int outputYear)
{
	/*int outputYear;

	outputYear = pEnvContext->currentYear;*/

	CPath outputPath(PathManager::GetPath(PM_OUTPUT_DIR));

	//CPath bldgShapeFilePath = outputPath;
	//CPath bldgCSVFilePath = outputPath;
	//CString csvFolder = _T("\\MapCsvs\\");

	//CString bldgFilename;
	//CString floodedGridFile;
	//CString cumFloodedGridFile;
	//CString erodedGridFile;
	//CString eelgrassGridFile;

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

	CString outDir = PathManager::GetPath(PM_OUTPUT_DIR);

	if (m_pDuneLayer != nullptr)
	{
		// Dune line layer naming
		CString duneFilename;
		duneFilename.Format("%sDunes_Year%i_%s_Run%i.shp", outDir, outputYear, scenario, run);

		CString msg("Exporting map layer: ");
		msg += duneFilename;
		Report::Log(msg);
		m_pDuneLayer->SaveShapeFile(duneFilename, false, 0, 1);
	}

	if (m_pErodedGrid != nullptr)
	{
		CString erodedFilename;
		erodedFilename.Format("%sEroded_Year%i_%s_Run%i.asc", outDir, outputYear, scenario, run);

		CString msg("Exporting map layer: ");
		msg += erodedFilename;
		Report::Log(msg);
		m_pErodedGrid->SaveGridFile(erodedFilename);
	}

	//      CString duneCSVFile = duneFilename + ".csv";
	//      duneCSVFilePath.Append(csvFolder);
	//      // make sure directory exists
	//#ifndef NO_MFC
	//      SHCreateDirectoryEx(nullptr, duneCSVFilePath, nullptr);
	//#else
	//      mkdir(duneCSVFilePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	//#endif
	//      duneCSVFilePath.Append(duneCSVFile);
	//      ok = m_pDuneLayer->SaveDataAscii(duneCSVFilePath, false, true);
	//      ok = 1;
	//      if (ok > 0)
	//         {
	//         msg.Replace(duneShapeFilePath, duneCSVFilePath);
	//         Report::Log(msg);
	//         }
	//
	// Buildings layer naming
  //bldgFilename.Format("Buildings_Year%i_%s_Run%i", outputYear, scenario, run);
  //
  //CString bldgShapeFile = bldgFilename + ".shp";
  //bldgShapeFilePath.Append(bldgShapeFile);
  //ok = m_pBldgLayer->SaveShapeFile(bldgShapeFilePath, false, 0, 1);
  //
  //if (ok > 0)
  //   {
  //   msg.Replace(duneCSVFilePath, bldgShapeFilePath);
  //   Report::Log(msg);
  //   }

  //CString bldgCSVFile = bldgFilename + ".csv";
  //bldgCSVFilePath.Append(csvFolder);
  // make sure directory exists
#ifndef NO_MFC
	  //SHCreateDirectoryEx(nullptr, bldgCSVFilePath, nullptr);
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

	  ////////if (m_pFloodedGrid != nullptr)
	  ////////   {
	  ////////   // write out the flooded grids at the chosen export interval
	  ////////   CPath floodedGridPath = outputPath;
	  ////////   //     if (m_writeGridFiles)
	  ////////   floodedGridFile.Format("Flooded_Year%i_%s_Run%i", outputYear, scenario, run);
	  ////////   CString gridFilename = floodedGridFile + ".flt";
	  ////////
	  ////////   msg.Replace(bldgFilename, gridFilename);
	  ////////   Report::Log(msg);
	  ////////
	  ////////   floodedGridPath.Append(gridFilename);
	  ////////   m_pFloodedGrid->SaveGridFile(floodedGridPath);
	  ////////
	  ////////   // cumulative flooded grid as well
	  ////////   CPath cumFloodedGridPath = outputPath;
	  ////////   //     if (m_writeGridFiles)
	  ////////   cumFloodedGridFile.Format("Cumulative_Flooded_Year%i_%s_Run%i", outputYear, scenario, run);
	  ////////   CString cumGridFilename = cumFloodedGridFile + ".flt";
	  ////////
	  ////////   msg.Replace(gridFilename, cumGridFilename);
	  ////////   Report::Log(msg);
	  ////////
	  ////////   cumFloodedGridPath.Append(cumGridFilename);
	  ////////   m_pCumFloodedGrid->SaveGridFile(cumFloodedGridPath);
	  ////////
	  ////////
	  ////////   // write out the eelgrass grids at the chosen export interval
	  ////////   //////bool runEelgrassModel = (m_runEelgrassModel == 1) ? true : false;
	  ////////   //////if (runEelgrassModel)
	  ////////   //////   {
	  ////////   //////   CPath eelgrassGridPath = outputPath;
	  ////////   //////   //     if (m_writeGridFiles)
	  ////////   //////   eelgrassGridFile.Format("Eelgrass_Year%i_%s_Run%i", outputYear, scenario, run);
	  ////////   //////   CString eelgridFilename = eelgrassGridFile + ".flt";
	  ////////   //////
	  ////////   //////   msg.Replace(gridFilename, eelgridFilename);
	  ////////   //////   Report::Log(msg);
	  ////////   //////
	  ////////   //////   eelgrassGridPath.Append(eelgridFilename);
	  ////////   //////   m_pEelgrassGrid->SaveGridFile(eelgrassGridPath);
	  ////////   //////   }
	  ////////   //////}
	  ////////
	  //////////if (m_runBayFloodModel == 1 && m_debugOn)
	  //////////{
	  //////////   if (pEnvContext->yearOfRun > 0 || (outputYear == pEnvContext->endYear))
	  //////////   {
	  //////////      CString floodedBayPtsFile;
	  //////////      CPath floodedBayPtsPath = outputPath;
	  //////////      floodedBayPtsFile.Format("FloodedBayPts_Year%i_%s_Run%i", outputYear, scenario, run);
	  //////////      CString floodedBayPtsFilename = floodedBayPtsFile + ".csv";
	  //////////      floodedBayPtsPath.Append(floodedBayPtsFilename);
	  //////////      m_pInletLUT->WriteAscii(floodedBayPtsPath);
	  ////////
	  //////////      //m_pElevationGrid->SaveGridFile("C:\\Envision\\StudyAreas\\GraysHarbor\\Outputs\\ghc_dem.flt");
	  //////////      CString debugMsg("ChronicHazards:: Exporting map layer: ");
	  //////////      debugMsg += floodedBayPtsFilename;
	  //////////      Report::Log(debugMsg);
	  //////////   }
	  //////////}


} // end ExportMapLayers(EnvContext *pEnvContext)


  // Determine if
bool ChronicHazards::FindProtectedBldgs()
{
	// For each building get it's IDU
	// Then find the set of dunePts within the Northing extents of that IDU
	// Finally assign the closest building to those dunePts
	Report::LogInfo("Locating protected buildings");

	for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
	{
		if (bldg % 100 == 0)
		{
			CString msg;
			msg.Format("Finding protected building %i of %i", (int)bldg, m_pBldgLayer->GetRowCount());
			Report::StatusMsg(msg);
		}

		REAL xBldg = 0.0;
		REAL yBldg = 0.0;
		m_pBldgLayer->GetPointCoords(bldg, xBldg, yBldg);

		CArray<int, int> iduIndices;
		int iduIndex = -1;
		int tsunami_hz = -1;

		// Get IDU of building if in Tsunamic Hazard Zone
		int sz = m_pIduBuildingLkUp->GetPolysFromPointIndex(bldg, iduIndices);
		for (int i = 0; i < sz; i++)
		{
			m_pIDULayer->GetData(iduIndices[0], m_colIDUTsunamiHazardZone, tsunami_hz);
			bool isTsunamiHzrd = (tsunami_hz == 1) ? true : false;
			if (isTsunamiHzrd)
				iduIndex = iduIndices[0];
		}

		m_pBldgLayer->SetData(bldg, m_colBldgIDUIndex, iduIndex);

		double northingTop = 0.0;
		double northingBtm = 0.0;

		if (iduIndex != -1)
		{
			m_pIDULayer->GetData(iduIndex, m_colIDUNorthingTop, northingTop);
			m_pIDULayer->GetData(iduIndex, m_colIDUNorthingBottom, northingBtm);
		}

		// Determine what building Dune Pts are protecting 
		for (MapLayer::Iterator dunePt = m_pDuneLayer->Begin(); dunePt < m_pDuneLayer->End(); dunePt++)
		{
			int beachType = -1;
			m_pDuneLayer->GetData(dunePt, m_colDLBeachType, beachType);

			int tst = dunePt;

			// only assign 
			if (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED)
			{
				double northing = 0.0;
				m_pDuneLayer->GetData(dunePt, m_colDLNorthing, northing);

				int duneIndex = -1;
				m_pDuneLayer->GetData(dunePt, m_colDLDuneIndex, duneIndex);

				REAL xDunePt = 0.0;
				REAL yDunePt = 0.0;
				m_pDuneLayer->GetPointCoords(dunePt, xDunePt, yDunePt);

				if (northing < northingTop && northing > northingBtm)
				{
					float distance = (float)sqrt((xBldg - xDunePt) * (xBldg - xDunePt) + (yBldg - yDunePt) * (yBldg - yDunePt));

					int bldgIndex = -1;
					m_pDuneLayer->GetData(dunePt, m_colDLDuneBldgIndex, bldgIndex);


					if (distance <= m_minDistToProtecteBldg)
					{
						if (bldgIndex == -1)
						{
							m_pDuneLayer->SetData(dunePt, m_colDLDuneBldgIndex, bldg);
							m_pDuneLayer->SetData(dunePt, m_colDLDuneBldgDist, distance);
						}
						else
						{
							float distanceBtwn = 0.0f;
							m_pDuneLayer->GetData(dunePt, m_colDLDuneBldgDist, distanceBtwn);
							if (distance < distanceBtwn)
							{
								m_pDuneLayer->SetData(dunePt, m_colDLDuneBldgIndex, bldg);
								m_pDuneLayer->SetData(dunePt, m_colDLDuneBldgDist, distance);
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
	//   int sz = m_pIduBuildingLkUp->GetPolysFromPointIndex(infra, iduIndices);
	//   for (int i = 0; i < sz; i++)
	//   {
	//      m_pIDULayer->GetData(iduIndices[ 0 ], m_colTsunamiHazardZone, tsunami_hz);
	//      bool isTsunamiHzrd = (tsunami_hz == 1) ? true : false;
	//      if (isTsunamiHzrd)
	//         iduIndex = iduIndices[ 0 ];
	//   }

	//   m_pInfraLayer->SetData(infra, m_colInfraIDUIndex, iduIndex);

	//   double northingTop = 0.0;
	//   double northingBtm = 0.0;

	//   if (iduIndex != -1)
	//   {
	//      m_pIDULayer->GetData(iduIndex, m_colIDUNorthingTop, northingTop);
	//      m_pIDULayer->GetData(iduIndex, m_colIDUNorthingBottom, northingBtm);
	//   }

	//   // Determine what building Dune Pts are protecting 
	//   for (MapLayer::Iterator dunePt = m_pDuneLayer->Begin(); dunePt < m_pDuneLayer->End(); dunePt++)
	//   {
	//      int beachType = -1;
	//      m_pDuneLayer->GetData(dunePt, m_colDLBeachType, beachType);

	//      int tst = dunePt;

	//      // only assign 
	//      if (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED)
	//      {
	//         double northing = 0.0;
	//         m_pDuneLayer->GetData(dunePt, m_colDLNorthing, northing);

	//         int duneIndex = -1;
	//         m_pDuneLayer->GetData(dunePt, m_colDLDuneIndex, duneIndex);

	//         double xDunePt = 0.0;
	//         double yDunePt = 0.0;
	//         m_pDuneLayer->GetPointCoords(dunePt, xDunePt, yDunePt);

	//         if (northing < northingTop && northing > northingBtm)
	//         {
	//            float distance = sqrt((xinfra - xDunePt) * (xinfra - xDunePt) + (yinfra - yDunePt) * (yinfra - yDunePt));

	//            int infraIndex = -1;
	//            m_pDuneLayer->GetData(dunePt, m_colDLDuneInfraIndex, infraIndex);

	//            if (infraIndex == -1)
	//            {
	//               m_pDuneLayer->SetData(dunePt, m_colDLDuneInfraIndex, infra);
	//               m_pDuneLayer->SetData(dunePt, m_colDLDuneInfraDist, distance);
	//            }
	//            else
	//            {
	//               float distanceBtwn = 0.0DLf;
	//               m_pDuneLayer->GetData(dunePt, m_colDLDuneInfraDist, distanceBtwn);
	//               if (distance < distanceBtwn)
	//               {
	//                  m_pDuneLayer->SetData(dunePt, m_colDLDuneInfraIndex, infra);
	//                  m_pDuneLayer->SetData(dunePt, m_colDLDuneInfraDist, distance);
	//               }
	//            }
	//         }

	//      }
	//   }
	//}


	return true;

}

bool ChronicHazards::FindClosestDunePtToBldg(EnvContext* pEnvContext)
{
	// Find closest Dune Pt for each Building
	//   for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
	//   {
	//      int bldgIndex = -1;
	//      m_pBldgLayer->GetData(bldg, m_colBldgDuneIndex, bldgIndex);
	//
	//      CArray<int, int> iduIndices;
	//      int iduIndex = -1;
	//      int tsunami_hz = -1;
	//
	//      int sz = m_pIduBuildingLkUp->GetPolysFromPointIndex(bldg, iduIndices);
	//      for (int i = 0; i < sz; i++)
	//      {
	//         m_pIDULayer->GetData(iduIndices[ 0 ], m_colTsunamiHazardZone, tsunami_hz);
	//         bool isTsunamiHzrd = (tsunami_hz == 1) ? true : false;
	//         if (isTsunamiHzrd)
	//            iduIndex = iduIndices[ 0 ];
	//      }
	//
	//      m_pBldgLayer->SetData(bldg, m_colBldgIDUIndex, iduIndex);
	//
	//      float distance = 100000.0f;
	//      int duneIndex = -1;
	//
	//      double northingTop, northingBtm = 0.0;
	//
	//      if (iduIndex != -1)
	//      {
	//         m_pIDULayer->GetData(iduIndex, m_colIDUNorthingTop, northingTop);
	//         m_pIDULayer->GetData(iduIndex, m_colIDUNorthingBottom, northingBtm);
	//      }
	//
	//      // Currently : assign closest dune pt to building only once
	////   if (bldgIndex == -1)// && iduIndex != -1)
	//      {
	//         double xBldg = 0.0;
	//         double yBldg = 0.0;
	//         m_pBldgLayer->GetPointCoords(bldg, xBldg, yBldg);
	//
	//         for (MapLayer::Iterator dunePt = m_pDuneLayer->Begin(); dunePt < m_pDuneLayer->End(); dunePt++)
	//         {
	//            int beachType = -1;
	//            m_pDuneLayer->GetData(dunePt, m_colDLBeachType, beachType);
	//
	//            // only assign 
	//            if (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED)
	//            {
	//               double xDunePt = 0.0;
	//               double yDunePt = 0.0;
	//               m_pDuneLayer->GetPointCoords(dunePt, xDunePt, yDunePt);
	//
	//               int index = -1;
	//               m_pDuneLayer->GetData(dunePt, m_colDLDuneIndex, index);
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
	//         m_pBldgLayer->SetData(bldg, m_colBldgDuneIndex, duneIndex);
	//         m_pBldgLayer->SetData(bldg, m_colBldgDuneDist, distance);
	//      }
	//   }

	// Determine if Dune Pt is protecting building
	//for (MapLayer::Iterator dunePt = m_pDuneLayer->Begin(); dunePt < m_pDuneLayer->End(); dunePt++)
	//{
	//   int duneIndex = -1;

	//   float minDistance = 1000000.0f;
	//   int bldgIndex = -1;

	//   m_pDuneLayer->GetData(dunePt, m_colDLDuneIndex, duneIndex);

	//   for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
	//   {
	//      int index = -1;
	//      m_pBldgLayer->GetData(bldg, m_colBldgDuneIndex, index);

	//      float distance = 0.0f;
	//      m_pBldgLayer->GetData(bldg, m_colBldgDuneDist, distance);

	//      // find bldg with minimum distance to dunePt in Tsunami Hazard Zone
	//      if (index != -1 && duneIndex == index && distance < minDistance)
	//      {
	//         minDistance = distance;
	//         bldgIndex = bldg;
	//      }
	//   }

	//   // ?? want to put both bldgIndex and iduIndex in the dune coverage
	//   m_pDuneLayer->SetData(dunePt, m_colDLDuneBldgIndex, bldgIndex);
	//}s


	// Find closest building to dune pt

	for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
	{
		int infraIndex = -1;
		m_pInfraLayer->GetData(infra, m_colInfraDuneIndex, infraIndex);

		// Currently :  assign closest dune pt to infrastructure only once
		if (infraIndex == -1)
		{
			REAL xInfra = 0.0;
			REAL yInfra = 0.0;
			m_pInfraLayer->GetPointCoords(infra, xInfra, yInfra);

			float distance = 100000.0f;
			int duneIndex = -1;

			for (MapLayer::Iterator dunePt = m_pDuneLayer->Begin(); dunePt < m_pDuneLayer->End(); dunePt++)
			{
				int beachType = -1;
				m_pDuneLayer->GetData(dunePt, m_colDLBeachType, beachType);

				// only assign 
				if (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED)
				{
					REAL xDunePt = 0.0;
					REAL yDunePt = 0.0;
					m_pDuneLayer->GetPointCoords(dunePt, xDunePt, yDunePt);

					int index = -1;
					m_pDuneLayer->GetData(dunePt, m_colDLDuneIndex, index);
					// Find closest dune point
					float distanceBtwn = (float)sqrt((xInfra - xDunePt) * (xInfra - xDunePt) + (yInfra - yDunePt) * (yInfra - yDunePt));
					if (distanceBtwn < distance)
					{
						distance = distanceBtwn;
						duneIndex = index;
					}
				}
				m_pInfraLayer->SetData(infra, m_colInfraDuneIndex, duneIndex);
			}
		}
	}

	// Determine if Dune Pt is protecting building
	/*for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
	double xCoord = 0.0;
	double yCoord = 0.0;
	m_pDuneLayer->GetPointCoords(point, xCoord, yCoord);

	int bldgCount = 0;
	m_pDuneLayer->GetData(point, m_colNumBldgs, bldgCount);

	for (MapLayer::Iterator bldg = m_pBldgLayer->Begin(); bldg < m_pBldgLayer->End(); bldg++)
	{
	double xBldg = 0.0;
	double yBldg = 0.0;

	m_pBldgLayer->GetPointCoords(point, xBldg, yBldg);

	float dx = xBldg - xCoord;
	float dy = yBldg - yCoord;

	if (dx + dy <= m_radiusOfErosion || (dx*dx + dy*dy < m_radiusOfErosion*m_radiusOfErosion) )
	m_pDuneLayer->SetData(point, m_colNumBldgs, ++bldgCount);
	}

	for (MapLayer::Iterator infra = m_pInfraLayer->Begin(); infra < m_pInfraLayer->End(); infra++)
	{
	double xInfrastructure = 0.0;
	double yInfrastructure = 0.0;

	m_pInfraLayer->GetPointCoords(point, xInfrastructure, yInfrastructure);

	float dx = xInfrastructure - xCoord;
	float dy = yInfrastructure - yCoord;

	if (dx + dy <= m_radiusOfErosion || (dx*dx + dy*dy < m_radiusOfErosion*m_radiusOfErosion) )
	m_pDuneLayer->SetData(point, m_colNumBldgs, ++bldgCount);
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

	MovingWindow* eroMovingWindow = m_eroBldgFreqArray.GetAt(bldg);
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

	m_pDuneLayer->GetPointCoords(pt, xPt, yPt);
	m_pDuneLayer->GetData(pt, m_colDLDuneBldgDist, distance);*/

	if (avgEro >= (distToBldg - m_radiusOfErosion))
		flag2 = true;

	//m_pBldgLayer->GetPointCoords(bldg, xBldg, yCoord);
	/*
	if ((xAvgEro + m_radiusOfErosion) >= xBldg)
	{
	m_pDuneLayer->SetData(pt, m_colDuneBldgEast, xBldg);
	m_pDuneLayer->SetData(pt, m_colDLDuneBldgIndex2, bldg);
	flag2 = true;
	}*/

	return (flag1 || flag2);

} // end ChronicHazards::IsBldgImpactedByErosion


bool ChronicHazards::CalculateExcessErosion(MapLayer::Iterator pt, float r, float s, bool north)
{
	REAL xCoord = 0.0;
	REAL yCoord = 0.0;
	m_pDuneLayer->GetPointCoords(pt, xCoord, yCoord);

	int beachType = -1;
	m_pDuneLayer->GetData(pt, m_colDLBeachType, beachType);

	// Determine BPS end effects north
	if (north)
	{
		REAL yCoord_s = yCoord + s;
		while (yCoord < yCoord_s && beachType == BchT_SANDY_DUNE_BACKED)
		{
			REAL excessErosion = -(r / s) * (yCoord - yCoord_s);

			double eastingToe = 0.0;
			m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);
			eastingToe += excessErosion;

			Poly* pPoly = m_pDuneLayer->GetPolygon(pt);
			if (pPoly->GetVertexCount() > 0)
			{
				pPoly->m_vertexArray[0].x = eastingToe;
				pPoly->InitLogicalPoints(m_pMap);
			}

			pt++;
			m_pDuneLayer->GetPointCoords(pt, xCoord, yCoord);
			m_pDuneLayer->GetData(pt, m_colDLBeachType, beachType);
		}
	} // end north
	else
	{
		REAL yCoord_s = yCoord - s;
		while (yCoord > yCoord_s && beachType == BchT_SANDY_DUNE_BACKED)
		{
			REAL excessErosion = (r / s) * (yCoord - yCoord_s);

			double eastingToe = 0.0;
			m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);
			eastingToe += excessErosion;

			Poly* pPoly = m_pDuneLayer->GetPolygon(pt);
			if (pPoly->GetVertexCount() > 0)
			{
				pPoly->m_vertexArray[0].x = eastingToe;
				pPoly->InitLogicalPoints(m_pMap);
			}

			pt--;
			m_pDuneLayer->GetPointCoords(pt, xCoord, yCoord);
			m_pDuneLayer->GetData(pt, m_colDLBeachType, beachType);
		}
	} // end south

	return true;
} // bool CalculateExcessErosion

bool ChronicHazards::CalculateImpactExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint, MapLayer::Iterator& minToePoint, MapLayer::Iterator& minDistPoint, MapLayer::Iterator& maxTWLPoint)
{
	int duneBldgIndex = -1;
	m_pDuneLayer->GetData(startPoint, m_colDLDuneBldgIndex, duneBldgIndex);

	bool construct = false;

	// No longer needed 
	/*MapLayer::Iterator endPoint = point;
	MapLayer::Iterator maxPoint = point;
	MapLayer::Iterator minToePoint = point;
	MapLayer::Iterator minDistPoint = point;*/

	if (duneBldgIndex != -1)
	{
		MovingWindow* eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndex);
		float eroFreq = eroMovingWindow->GetFreqValue();
		m_pDuneLayer->SetData(startPoint, m_colDLDuneEroFreq, eroFreq);

		int nextBldgIndex = duneBldgIndex;
		float buildBPSThreshold = m_idpyFactor * 365.0f;

		float maxAmtTop = 0.0f;
		float minDuneToe = 10000.0f;
		float minDistToBldg = 10000.0f;
		float maxTWL = 0.0f;

		while (duneBldgIndex == nextBldgIndex && endPoint < m_pDuneLayer->End())
		{
			int beachType = -1;
			m_pDuneLayer->GetData(endPoint, m_colDLBeachType, beachType);

			if (beachType == BchT_SANDY_DUNE_BACKED)
			{
				MovingWindow* ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);

				float movingAvgIDPY = 0.0f;
				m_pDuneLayer->GetData(endPoint, m_colDLMvAvgIDPY, movingAvgIDPY);

				float xAvgKD = 0.0f;
				m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, xAvgKD);

				float avgKD = 0.0f;
				m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, avgKD);
				float distToBldg = 0.0f;
				m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgDist, distToBldg);

				bool isImpactedByEErosion = false;
				isImpactedByEErosion = IsBldgImpactedByEErosion(nextBldgIndex, avgKD, distToBldg);

				// do any dune points cause trigger
				if (movingAvgIDPY >= buildBPSThreshold || isImpactedByEErosion)
				{
					construct = true;

					// Retrieve the maximum TWL within the moving window
					MovingWindow* twlMovingWindow = m_TWLArray.GetAt(endPoint);
					float movingMaxTWL = twlMovingWindow->GetMaxValue();

					float duneCrest = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneCrest, duneCrest);

					// Find dune point with highest overtopping
					if (maxAmtTop >= 0.0f && (movingMaxTWL - duneCrest) > maxAmtTop)
					{
						maxAmtTop = movingMaxTWL - duneCrest;
						maxTWL = movingMaxTWL;
						maxTWLPoint = endPoint;
					}

					// Find dune point with minimum elevation
					float duneToe = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneToe, duneToe);
					if (minDuneToe <= duneToe)
					{
						minDuneToe = duneToe;
						minToePoint = endPoint;
					}

					// Find dune point closest to the protected building
					float distToBldg = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgDist, distToBldg);
					if (minDistToBldg <= distToBldg)
					{
						minDistToBldg = distToBldg;
						minDistPoint = endPoint;
					}
				}
			}

			endPoint++;
			if (endPoint < m_pDuneLayer->End())
				m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, nextBldgIndex);
		}

	} // end protecting building

	if (construct)
	{
		// determine total length of BPS structure, runs along IDU
		REAL yTop = 0.0;
		REAL yBtm = 0.0;
		REAL xCoord = 0.0;
		m_pDuneLayer->GetPointCoords(endPoint, xCoord, yTop);
		m_pDuneLayer->GetPointCoords(startPoint, xCoord, yBtm);

		// TO DO::: write this total BPSLength into IDU using duneBldgIndex
		double BPSLength = yTop - yBtm;
	}

	return construct;

} // end CalculateImpactExtent

bool ChronicHazards::CalculateRebuildSPSExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint)
{
	bool rebuild = false;

	int builtYear = 0;
	m_pDuneLayer->GetData(startPoint, m_colDLAddYearSPS, builtYear);

	int yr = builtYear;

	REAL eastingToe = 0.0;
	REAL eastingToeSPS = 0.0;
	float height = 0.0f;
	float heelWidth = 0.0f;
	float topWidth = 0.0f;

	while (yr > 0 && yr == builtYear)
	{
		m_pDuneLayer->GetData(endPoint, m_colDLEastingToe, eastingToe);
		m_pDuneLayer->GetData(endPoint, m_colDLEastingToeSPS, eastingToeSPS);

		m_pDuneLayer->GetData(endPoint, m_colDLTopWidthSPS, topWidth);
		m_pDuneLayer->GetData(endPoint, m_colDLHeelWidthSPS, heelWidth);
		float extentErosion = (float)(eastingToe - eastingToeSPS);

		if (extentErosion >= topWidth + m_rebuildFactorSPS * heelWidth) // + height/m_frontSlopeSPS) 
			rebuild = true;

		float extentKD = 0.0f;
		m_pDuneLayer->GetData(endPoint, m_colDLKD, extentKD);

		if (extentKD >= topWidth + heelWidth)
			rebuild = true;

		//int bldgIndex = -1;
		//m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, bldgIndex);

		//// Retrieve the Flooding Frequency of the Protected Building
		//if (bldgIndex != -1)
		//{
		//   MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);
		//   float floodFreq = floodMovingWindow->GetFreqValue();

		//   float floodThreshold = (float)m_floodCountSPS / m_windowLengthFloodHzrd;
		//   
		//   if (floodFreq >= floodThreshold)
		//      rebuild = true;            
		//}

		endPoint++;
		m_pDuneLayer->GetData(endPoint, m_colDLAddYearSPS, yr);
	}

	return rebuild;
}
//
// return rebuild;
//)

// Helper Methods
bool ChronicHazards::CalculateSegmentLength(MapLayer::Iterator startPoint, MapLayer::Iterator endPoint, float& length)
{
	// determine length of segment, runs along IDU
	REAL yTop = 0.0;
	REAL yBtm = 0.0;
	REAL xCoord = 0.0;
	m_pDuneLayer->GetPointCoords(startPoint, xCoord, yBtm);
	m_pDuneLayer->GetPointCoords(endPoint, xCoord, yTop);

	length = float(yTop - yBtm);

	if (length < 0)
		return false;
	return true;
}

int ChronicHazards::GetBeachType(MapLayer::Iterator pt)
{
	int beachType = -1;
	if (pt < m_pDuneLayer->End())
		m_pDuneLayer->GetData(pt, m_colDLBeachType, beachType);

	return beachType;
}

int ChronicHazards::GetNextBldgIndex(MapLayer::Iterator pt)
{
	int nextBldgIndex = -1;
	if (pt < m_pDuneLayer->End())
		m_pDuneLayer->GetData(pt, m_colDLDuneBldgIndex, nextBldgIndex);

	return nextBldgIndex;
}

// Currently structure is built landward
// uncomment beachwidth narrowing and eastingToe translation to build seaward
// question of how much beach narrows and toe translates based upon geometry:  if every toe moves different distances
// dependent upon the beachslope use ConstructBPS
void ChronicHazards::ConstructBPS1(int currentYear)
{
	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		int duneBldgIndex = -1;
		m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, duneBldgIndex);

		if (duneBldgIndex != -1)
		{
			MapLayer::Iterator endPoint = point;
			MapLayer::Iterator maxPoint = point;

			MovingWindow* eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndex);
			float eroFreq = eroMovingWindow->GetFreqValue();
			m_pDuneLayer->SetData(point, m_colDLDuneEroFreq, eroFreq);

			// find the extent of the BPS as well as location of maximum TWL within that extent
			// doublely needs maximum overtopping and minimum duneToe

			bool cond = CalculateBPSExtent(point, endPoint, maxPoint);

			if (cond)
			{
				m_noConstructedBPS += 1;

				MovingWindow* twlMovingWindow = m_TWLArray.GetAt(maxPoint);
				// Retrieve the maxmum TWL within the designated window
				float newCrest = twlMovingWindow->GetMaxValue();

				float duneToe = 0.0f;
				float duneCrest = 0.0f;
				float beachWidth = 0.0f;
				REAL eastingToe = 0.0;
				float tanb = 0.0f;

				m_pDuneLayer->GetData(maxPoint, m_colDLDuneToe, duneToe);
				m_pDuneLayer->GetData(maxPoint, m_colDLDuneCrest, duneCrest);
				m_pDuneLayer->GetData(maxPoint, m_colDLEastingToe, eastingToe);
				m_pDuneLayer->GetData(maxPoint, m_colDLBeachWidth, beachWidth);
				m_pDuneLayer->GetData(maxPoint, m_colDLTranSlope, tanb);

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
				//////if (m_debugOn)
				//////   {
				//////   //  TODO:::Still using old version, uncomment beachwidth reduction and easting toe translation after decision
				//////   double newEastingToe = (BPSHeight + (tanb - m_slopeBPS) * eastingToe) / (tanb - m_slopeBPS);
				//////   double deltaX = eastingToe - newEastingToe;
				//////   double deltaX2 = BPSHeight / m_slopeBPS;
				//////   m_pDuneLayer->SetData(point, m_colNewEasting, newEastingToe);
				//////   m_pDuneLayer->SetData(point, m_colBWidth, deltaX2);
				//////   m_pDuneLayer->SetData(point, m_colDeltaX, deltaX);
				//////   /* eastingToe = newEastingToe;
				//////   beachWidth -= deltaX;*/
				//////   }

				REAL eastingCrest = eastingToe + BPSHeight / m_slopeBPS;

				// determine length of BPS structure, runs along IDU
				REAL yTop = 0.0;
				REAL yBtm = 0.0;
				REAL xCoord = 0.0;
				m_pDuneLayer->GetPointCoords(endPoint, xCoord, yTop);
				m_pDuneLayer->GetPointCoords(point, xCoord, yBtm);

				float BPSLength = float(yTop - yBtm);

				float BPSCost = BPSHeight * m_costs.BPS * BPSLength;
				m_constructCostBPS += BPSCost;

				// construct BPS by changing beachtype to RIP RAP BACKED
				for (MapLayer::Iterator pt = point; pt < endPoint; pt++, point++)
				{
					// all dune points of a BPS have the same characteristics
					m_pDuneLayer->SetData(pt, m_colDLBeachType, BchT_SANDY_RIPRAP_BACKED);
					//m_pDuneLayer->SetData(pt, m_colDLTypeChange, 1);
					m_pDuneLayer->SetData(pt, m_colDLDuneCrest, newCrest);
					m_pDuneLayer->SetData(pt, m_colDLAddYearBPS, currentYear);
					m_pDuneLayer->SetData(pt, m_colDLGammaRough, m_minBPSRoughness);
					m_pDuneLayer->SetData(pt, m_colDLCostBPS, BPSCost);
					m_pDuneLayer->SetData(pt, m_colDLLengthBPS, BPSLength);
					float width = (float)(eastingCrest - eastingToe);
					m_pDuneLayer->SetData(pt, m_colDLWidthBPS, width);

					m_pDuneLayer->SetData(pt, m_colDLEastingCrest, eastingCrest);
					m_pDuneLayer->SetData(pt, m_colDLDuneToe, duneToe);
					m_pDuneLayer->SetData(pt, m_colDLBeachWidth, beachWidth);
					//      m_pDuneLayer->SetData(pt, m_colDLEastingToe, eastingToe);

					Poly* pPoly = m_pDuneLayer->GetPolygon(pt);
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

	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		/*MapLayer::Iterator endPoint = point;
		MapLayer::Iterator maxPoint = point;
		MapLayer::Iterator minPoint = point;
		bool construct = CalculateImpactExtent(point, endPoint, minPoint, maxPoint);*/

		int duneBldgIndex = -1;
		m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, duneBldgIndex);

		bool trigger = false;
		MapLayer::Iterator endPoint = point;
		MapLayer::Iterator maxPoint = point;
		MapLayer::Iterator minToePoint = point;
		MapLayer::Iterator minDistPoint = point;

		CArray<int> dlTypeChangeIndexArray;   // dune points for which the trigger is true (m_pDuneLayer->SetData(endPoint, m_colDLTypeChange, 1)
		CArray<int> iduAddBPSYrIndexArray;       // m_pIDULayer->SetData(northIndex, m_colIDUAddBPSYr, currentYear);

												 // dune protecting building?
		if (duneBldgIndex != -1)
		{
			MovingWindow* eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndex);
			float eroFreq = eroMovingWindow->GetFreqValue();
			m_pDuneLayer->SetData(point, m_colDLDuneEroFreq, eroFreq);

			int nextBldgIndex = duneBldgIndex;
			float buildBPSThreshold = m_idpyFactor * 365.0f;

			float maxAmtTop = 0.0f;
			float minToeElevation = 10000.0f;
			float minDistToBldg = 100000.0f;
			float maxTWL = 0.0f;

			// look along length of IDU determine if criteria for BPS construction is met
			// We know we are in the same IDU if the next dunepoint (endPoint) is protecting the
			// same buiding
			while (endPoint < m_pDuneLayer->End() && GetNextBldgIndex(endPoint) == duneBldgIndex)
			{
				int beachType = -1;
				m_pDuneLayer->GetData(endPoint, m_colDLBeachType, beachType);

				if (beachType == BchT_SANDY_DUNE_BACKED)
				{
					MovingWindow* ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);

					float movingAvgIDPY = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLMvAvgIDPY, movingAvgIDPY);

					float xAvgKD = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, xAvgKD);

					int duneIndex = -1;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneIndex, duneIndex);

					float avgKD = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, avgKD);
					float distToBldg = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgDist, distToBldg);

					bool isImpactedByEErosion = false;
					isImpactedByEErosion = IsBldgImpactedByEErosion(nextBldgIndex, avgKD, distToBldg);

					// do any dune points cause trigger ?
					if (movingAvgIDPY >= buildBPSThreshold || isImpactedByEErosion)
					{
						trigger = true;
						//??? should this be moved to after cost constraint applied?
						//m_pDuneLayer->SetData(endPoint, m_colDLTypeChange, 1);
						dlTypeChangeIndexArray.Add(endPoint);
					}

					// Retrieve the maximum TWL within the moving window
					MovingWindow* twlMovingWindow = m_TWLArray.GetAt(endPoint);
					float movingMaxTWL = twlMovingWindow->GetMaxValue();

					float duneCrest = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneCrest, duneCrest);

					// Find dune point with highest overtopping
					if (maxAmtTop >= 0.0f && (movingMaxTWL - duneCrest) > maxAmtTop)
					{
						maxAmtTop = movingMaxTWL - duneCrest;
						maxTWL = movingMaxTWL;
						maxPoint = endPoint;
					}

					// Find dune point with minimum elevation
					float duneToe = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneToe, duneToe);
					if (minToeElevation <= duneToe)
					{
						minToeElevation = duneToe;
						minToePoint = endPoint;
					}

					// Find dune point closest to the protected building
					/*   float distToBldg = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgDist, distToBldg);*/
					if (minDistToBldg <= distToBldg)
					{
						minDistToBldg = distToBldg;
						minDistPoint = endPoint;
					}
				}
				endPoint++;
			}   // end of: while (endPoint < m_pDuneLayer->End() && GetNextBldgIndex(endPoint) == duneBldgIndex)

			endPoint--;  // we should be just over the northern edge of the IDU, back up so we are just inside of it.


		} // end protecting building

		  // make sure everything is okay, diddn't overrun bounds
		if (endPoint >= m_pDuneLayer->End())
			Report::ErrorMsg("Coastal Hazards: Invalid Endpoint encountered when Constructing BPS (0)");
		if (endPoint < m_pDuneLayer->Begin())
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
			int sz = m_pIduBuildingLkUp->GetPolysFromPointIndex(duneBldgIndex, iduIndices);
			iduIndex = iduIndices[0];

			REAL northingTop = 0.0;
			REAL northingBottom = 0.0;

			float iduLength = 0.0f;
			m_pIDULayer->GetData(iduIndex, m_colIDULength, iduLength);
			m_pIDULayer->GetData(iduIndex, m_colIDUNorthingTop, northingTop);
			m_pIDULayer->GetData(iduIndex, m_colIDUNorthingBottom, northingBottom);
			//      m_pIDULayer->SetData(iduIndex, m_colIDUAddBPSYr, currentYear);

			// if the idu length (top to bottom) is less than 500m, consider north, then south neighbor,
			// for expanding the BPS
			if (iduLength <= 500.0f)
			{
				bpsLength = iduLength;

				// find IDU to the north
				Poly* pPoly = m_pIDULayer->GetPolygon(iduIndex);

				Vertex v = pPoly->GetCentroid();
				//Vertex v(pPoly->xMin, northingTop);

				//      int count = m_pIDULayer->GetNearbyPolys(pPoly, neighbors, nullptr, 10, 1);
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
						bool ok = m_pIDULayer->GetData(northIndex, m_colIDULength, northLength);
						if (ok && bpsLength + northLength <= 500.0f && northIndex >= 0)
						{
							////??? should this be moved to after cost constraint applied?
							//m_pIDULayer->SetData(northIndex, m_colIDUAddBPSYr, currentYear);
							iduAddBPSYrIndexArray.Add(northIndex);

							// check to make sure beachtype is Sandy Dune Backed
							bpsLength += northLength;
							int northPt = (int)(northLength / m_elevCellHeight) - 1;
							int i = 0;
							int beachType = -1;
							m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
							while (beachType == BchT_SANDY_DUNE_BACKED && endPoint < m_pDuneLayer->End() && i < northPt)
							{
								i++;
								m_pDuneLayer->GetData(endPoint, m_colDLBeachType, beachType);
								endPoint++;
							}
						}
					}
				}

				float southLength = 0.0;
				southIndex = m_pIDULayer->GetNearestPoly(v.x, northingBottom - 1.0);
				ASSERT(southIndex >= 0);
				bool ok = m_pIDULayer->GetData(southIndex, m_colIDULength, southLength);
				if (ok && bpsLength + southLength <= 500.0f && southIndex >= 0)
				{
					////??? should this be moved to after cost constraint applied?
					//m_pIDULayer->SetData(southIndex, m_colIDUAddBPSYr, currentYear);
					iduAddBPSYrIndexArray.Add(southIndex);

					bpsLength += southLength;
					float addedLength = float(v.y - northingBottom);
					int southPt = (int)((southLength + addedLength) / m_elevCellHeight) - 1;

					int i = 0;
					int beachType = -1;
					m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
					while (beachType == BchT_SANDY_DUNE_BACKED && point > m_pDuneLayer->Begin() && i < southPt)
					{
						i++;
						m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
						point--;
					}
				}

				// determine cost of structure and whether there is money to construct it
				// before construction 

			}
		}  // end trigger (calculate bpsLength)

		ASSERT(endPoint < m_pDuneLayer->End());
		ASSERT(point >= m_pDuneLayer->Begin());
		if (endPoint >= m_pDuneLayer->End())
			endPoint = m_pDuneLayer->End();
		if (point < m_pDuneLayer->Begin())
			point = m_pDuneLayer->Begin();

		// determine cost of bps construction
		if (trigger)
		{
			// Retrieve the maximum TWL within the designated window
			// for the dune point with the maximum overtopping within the extent
			MovingWindow* twlMovingWindow = m_TWLArray.GetAt(maxPoint);
			float newCrest = twlMovingWindow->GetMaxValue() + m_safetyFactor;

			// Retrieve dune crest elevation at maximum overtopping
			float duneCrest = 0.0f;
			m_pDuneLayer->GetData(maxPoint, m_colDLDuneCrest, duneCrest);

			// Retreive the dune point with the minimum dune toe elevation within the extent
			float minDuneToe = 0.0f;
			m_pDuneLayer->GetData(minToePoint, m_colDLDuneToe, minDuneToe);

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
			m_pDuneLayer->GetData(minDistPoint, m_colDLDuneBldgDist, minDistToBldg);
			float yToe = 0.0f;
			m_pDuneLayer->GetData(minDistPoint, m_colDLDuneToe, yToe);

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
				bool ok = m_pDuneLayer->GetData(dpt, m_colDLDuneToe, duneToe);
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
			PolicyInfo& pi = m_policyInfoArray[PI_CONSTRUCT_BPS];

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
					m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneToe);

					double eastingToe = 0.0;
					m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);

					double beachWidth = 0.0;
					m_pDuneLayer->GetData(pt, m_colDLBeachWidth, beachWidth);

					float tanb = 0.0f;
					m_pDuneLayer->GetData(pt, m_colDLTranSlope, tanb);

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
					m_pDuneLayer->GetData(pt, m_colDLBeachType, beachType);
					if (beachTYpe == BchT_SANDY_DUNE_BACKED)
					{ */
					m_pDuneLayer->SetData(pt, m_colDLBeachType, BchT_SANDY_RIPRAP_BACKED);
					m_pDuneLayer->SetData(pt, m_colDLDuneCrest, newCrest);
					m_pDuneLayer->SetData(pt, m_colDLAddYearBPS, currentYear);
					m_pDuneLayer->SetData(pt, m_colDLGammaRough, m_minBPSRoughness);

					//      m_pDuneLayer->SetData(pt, m_colDLCostBPS, cost);
					m_pDuneLayer->SetData(pt, m_colDLHeightBPS, bpsHeight);
					m_pDuneLayer->SetData(pt, m_colDLLengthBPS, m_elevCellHeight);
					m_pDuneLayer->SetData(pt, m_colDLWidthBPS, bpsWidth);
					m_pDuneLayer->SetData(pt, m_colDLDuneToe, duneToe);
					m_pDuneLayer->SetData(pt, m_colDLBeachWidth, beachWidth);

					m_pDuneLayer->SetData(pt, m_colDLEastingToeBPS, eastingToe);

					m_pDuneLayer->SetData(pt, m_colDLEastingCrest, eastingCrest);
					m_pDuneLayer->SetData(pt, m_colDLEastingToe, eastingToe);


					// take care of those coverage updates we deferred from above
					for (int i = 0; i < (int)dlTypeChangeIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlTypeChangeIndexArray[i], m_colDLTypeChange, 1);

					for (int i = 0; i < (int)iduAddBPSYrIndexArray.GetSize(); i++)
						m_pIDULayer->SetData(iduAddBPSYrIndexArray[i], m_colIDUAddBPSYr, currentYear);


					// update new dune toe position
					Poly* pPoly = m_pDuneLayer->GetPolygon(pt);
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
	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		//// Determine the extent of the protection structure as well as the dune pt with the
		//// maximum overtopping, the dune pt with the minumum dune toe elevation and 
		//// the dune pt closest to the protected building
		//bool construct = CalculateImpactExtent(point, endPoint, minToePoint, minDistPoint, maxTWLPoint);

		int duneBldgIndex = -1;
		m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, duneBldgIndex);

		bool trigger = false;
		MapLayer::Iterator endPoint = point;
		MapLayer::Iterator maxTWLPoint = point;
		MapLayer::Iterator minToePoint = point;
		MapLayer::Iterator minDistPoint = point;
		MapLayer::Iterator maxDuneWidthPoint = point;

		CArray<int> dlTypeChangeIndexArray;               // m_pDuneLayer->SetData(endPoint, m_colDLTypeChange, 1);
		CArray<pair<int, float>> dlDuneToeSPSIndexArray;      // m_pDuneLayer->SetData(pt, m_colDLDuneToeSPS, duneToe);
		CArray<pair<int, float>> dlEastingToeSPSIndexArray;  // m_pDuneLayer->SetData(pt, m_colDLEastingToeSPS, eastingToe);
		CArray<pair<int, float>> dlDuneHeelIndexArray;      // m_pDuneLayer->SetData(pt, m_colDLDuneHeel, duneHeel);
		CArray<pair<int, float>> dlFrontSlopeIndexArray;    // m_pDuneLayer->SetData(pt, m_colDLFrontSlope, frontSlope);
		CArray<pair<int, float>> dlDuneToeIndexArray;       // m_pDuneLayer->SetData(pt, m_colDLDuneToe, duneToe);
		CArray<pair<int, float>> dlDuneCrestIndexArray;    // m_pDuneLayer->SetData(pt, m_colDLDuneCrest, duneCrest);
		CArray<pair<int, float>> dlDuneWidthIndexArray;    // m_pDuneLayer->SetData(pt, m_colDLDuneWidth, duneWidth);

		if (duneBldgIndex != -1)
		{
			MovingWindow* eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndex);
			float eroFreq = eroMovingWindow->GetFreqValue();
			m_pDuneLayer->SetData(point, m_colDLDuneEroFreq, eroFreq);

			//   int nextBldgIndex = duneBldgIndex;
			float buildBPSThreshold = m_idpyFactor * 365.0f;

			float maxAmtTop = 0.0f;
			float minToeElevation = 10000.0f;
			float minDistToBldg = 100000.0f;
			float maxTWL = 0.0f;
			float maxDuneWidth = 0.0f;

			while (endPoint < m_pDuneLayer->End() && GetNextBldgIndex(endPoint) == duneBldgIndex)
			{
				int beachType = -1;
				m_pDuneLayer->GetData(endPoint, m_colDLBeachType, beachType);

				if (beachType == BchT_SANDY_DUNE_BACKED)
				{
					MovingWindow* ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);

					float movingAvgIDPY = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLMvAvgIDPY, movingAvgIDPY);

					float xAvgKD = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, xAvgKD);

					float avgKD = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, avgKD);
					float distToBldg = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgDist, distToBldg);

					bool isImpactedByEErosion = false;
					isImpactedByEErosion = IsBldgImpactedByEErosion(GetNextBldgIndex(endPoint), avgKD, distToBldg);

					// do any dune points cause trigger ?
					if (movingAvgIDPY >= buildBPSThreshold || isImpactedByEErosion)
					{
						int yrBuilt = 0;
						m_pDuneLayer->GetData(endPoint, m_colDLAddYearSPS, yrBuilt);
						if (yrBuilt == 0)
						{
							//??? defer to later
							//m_pDuneLayer->SetData(endPoint, m_colDLTypeChange, 1);
							dlTypeChangeIndexArray.Add(endPoint);
							trigger = true;
						}
					}

					// Retrieve the maximum TWL within the moving window
					MovingWindow* twlMovingWindow = m_TWLArray.GetAt(endPoint);
					float movingMaxTWL = twlMovingWindow->GetMaxValue();

					float duneCrest = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneCrest, duneCrest);

					// Find dune point with highest overtopping
					if (maxAmtTop >= 0.0f && (movingMaxTWL - duneCrest) > maxAmtTop)
					{
						maxAmtTop = movingMaxTWL - duneCrest;
						maxTWL = movingMaxTWL;
						maxTWLPoint = endPoint;
					}

					// Find dune point with minimum elevation
					float duneToe = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneToe, duneToe);
					if (minToeElevation <= duneToe)
					{
						minToeElevation = duneToe;
						minToePoint = endPoint;
					}

					// Find dune point closest to the protected building
					//float distToBldg = 0.0f;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgDist, distToBldg);
					if (minDistToBldg <= distToBldg)
					{
						minDistToBldg = distToBldg;
						minDistPoint = endPoint;
					}
					// Find widest dune within the extent
					float duneWidth = 0.0;
					m_pDuneLayer->GetData(endPoint, m_colDLDuneWidth, duneWidth);
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

		ASSERT(endPoint < m_pDuneLayer->End());
		ASSERT(point >= m_pDuneLayer->Begin());
		if (endPoint >= m_pDuneLayer->End())
			endPoint = m_pDuneLayer->End();
		if (point < m_pDuneLayer->Begin())
			point = m_pDuneLayer->Begin();

		/////*** cut here make this a method ******//
		// catch if dune was already constructed

		if (trigger)
		{
			// Build SPS such that each section is a different height but is level at the top
			// of the structure
			// m_noConstructedDune += 1;   moved to beow cost constraint

			// Retrieve the maximum TWL within the designated window
			// for the dune point with the maximum overtopping within the extent
			MovingWindow* twlMovingWindow = m_TWLArray.GetAt(maxTWLPoint);
			float newCrest = twlMovingWindow->GetMaxValue() + m_safetyFactor;

			// Retrieve dune crest elevation at maximum overtopping
			float duneCrest = 0.0f;
			m_pDuneLayer->GetData(maxTWLPoint, m_colDLDuneCrest, duneCrest);

			// Retrieve the dune point with the minimum dune toe elevation within the extent
			float minDuneToe = 0.0f;
			m_pDuneLayer->GetData(minToePoint, m_colDLDuneToe, minDuneToe);

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
			//m_pDuneLayer->GetPointCoords(endPoint, xCoord, yTop);
			//m_pDuneLayer->GetPointCoords(point, xCoord, yBtm);

			//// TO DO::: write total length into IDU 
			//double SPSLength = yTop - yBtm;

			// determine where to build the SPS :  relative to the toe or bldg
			// Retrieve the minimum distance to building
			bool constructFromToe = true;
			float minDistToBldg = 0.0f;
			m_pDuneLayer->GetData(minDistPoint, m_colDLDuneBldgDist, minDistToBldg);
			float yToe = 0.0f;
			m_pDuneLayer->GetData(minDistPoint, m_colDLDuneToe, yToe);
			float yHeel = 0.0f;
			m_pDuneLayer->GetData(minDistPoint, m_colDLDuneHeel, yHeel);

			float widestDuneWidth = 0.0f;
			m_pDuneLayer->GetData(maxDuneWidthPoint, m_colDLDuneWidth, widestDuneWidth);

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
				m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneToe);

				// defer to later            
				//m_pDuneLayer->SetData(pt, m_colDLDuneToeSPS, duneToe);
				dlDuneToeSPSIndexArray.Add(pair<int, float>((int)pt, duneToe));

				float duneHeel = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneHeel);

				float duneWidth = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLDuneWidth, duneWidth);

				float frontSlope = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLFrontSlope, frontSlope);

				float backSlope = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLBackSlope, backSlope);

				float eastingToe = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);


				// defer to later            
				// m_pDuneLayer->SetData(pt, m_colDLEastingToeSPS, eastingToe);
				dlEastingToeSPSIndexArray.Add(pair<int, float>((int)pt, eastingToe));

				double eastingCrest = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLEastingCrest, eastingCrest);

				double beachWidth = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLBeachWidth, beachWidth);

				float tanb = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLTranSlope, tanb);

				// Determine volume (area * length) of existing sand in dune
				// Consists of 2 triangles + rectangle - one triangle

				if (duneHeel <= 0.0)
				{
					duneHeel = m_avgDuneHeel;

					// defer to later
					//m_pDuneLayer->SetData(pt, m_colDLDuneHeel, duneHeel);
					dlDuneHeelIndexArray.Add(pair<int, float>((int)pt, duneHeel));
				}

				if (duneWidth <= 0.0)
				{
					duneWidth = m_avgDuneWidth;

					// defer to later
					//m_pDuneLayer->SetData(pt, m_colDLDuneWidth, duneWidth);
					dlDuneWidthIndexArray.Add(pair<int, float>((int)pt, duneWidth));
				}

				if (frontSlope < 0.0)
				{
					frontSlope = m_avgFrontSlope;

					// defer to later
					//m_pDuneLayer->SetData(pt, m_colDLFrontSlope, frontSlope);
					dlFrontSlopeIndexArray.Add(pair<int, float>((int)pt, frontSlope));
				}

				if (duneToe < 0.0)
				{
					duneToe = m_avgDuneToe;
					// defer to later
					//m_pDuneLayer->SetData(pt, m_colDLDuneToe, duneToe);
					dlDuneToeIndexArray.Add(pair<int, float>((int)pt, duneToe));
				}

				if (duneCrest < 0.0)
				{
					duneCrest = m_avgDuneCrest;
					// defer to later
					//m_pDuneLayer->SetData(pt, m_colDLDuneCrest, duneCrest);
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
					constructVolume = totalConstructedDuneArea * m_elevCellHeight;

				// Subtract off area of existing sand, if enough sand
				if (totalExistingArea > 0)
					constructVolume = (totalConstructedDuneArea - totalExistingArea) * m_elevCellHeight;

				if (constructVolume < 0.0)
					constructVolume = 0.0;

				// Determine cost of a section along the SPS
				// Cost units : $ per cubic meter
				float cost = float(constructVolume * m_costs.SPS);

				// get budget infor for Constructing BPS policy
				PolicyInfo& pi = m_policyInfoArray[PI_CONSTRUCT_SPS];
				bool passCostConstraint = true;

				// enough funds in original allocation ?
				if (pi.HasBudget() && pi.m_availableBudget <= cost) //armorAllocationResidual >= cost)
					passCostConstraint = false;

				AddToPolicySummary(currentYear, PI_CONSTRUCT_SPS, int(pt), m_costs.SPS, cost, pi.m_availableBudget, (float)constructVolume, (float)duneArea, passCostConstraint);

				if (passCostConstraint)
				{
					pi.IncurCost(cost);

					m_pDuneLayer->SetData(pt, m_colDLDuneCrest, newCrest);
					m_pDuneLayer->SetData(pt, m_colDLAddYearSPS, currentYear);

					m_pDuneLayer->SetData(pt, m_colDLCostSPS, cost);
					m_pDuneLayer->SetData(pt, m_colDLHeightSPS, height);
					m_pDuneLayer->SetData(pt, m_colDLWidthSPS, newDuneWidth);
					m_pDuneLayer->SetData(pt, m_colDLTopWidthSPS, topWidth);
					m_pDuneLayer->SetData(pt, m_colDLLengthSPS, m_elevCellHeight);

					m_pDuneLayer->SetData(pt, m_colDLDuneToe, newDuneToe);
					m_pDuneLayer->SetData(pt, m_colDLBeachWidth, beachWidth);
					m_pDuneLayer->SetData(pt, m_colDLHeelWidthSPS, heelWidth);
					m_pDuneLayer->SetData(pt, m_colDLConstructVolumeSPS, constructVolume);

					// Holds the easting location of the constructed dune -- does move through time
					m_pDuneLayer->SetData(pt, m_colDLEastingToeSPS, eastingToe);
					//         m_pDuneLayer->SetData(pt, m_colDLEastingCrestSPS, eastingCrest);

					m_pDuneLayer->SetData(pt, m_colDLEastingCrest, eastingCrest);
					m_pDuneLayer->SetData(pt, m_colDLEastingToe, eastingToe);

					// updated deferred fields (Note: Some of these are redundant with the above
					for (int i = 0; i < (int)dlTypeChangeIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlTypeChangeIndexArray[i], m_colDLTypeChange, 1);

					for (int i = 0; i < (int)dlDuneToeSPSIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlDuneToeSPSIndexArray[i].first, m_colDLDuneToeSPS, dlDuneToeSPSIndexArray[i].second);

					for (int i = 0; i < (int)dlEastingToeSPSIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlEastingToeSPSIndexArray[i].first, m_colDLEastingToeSPS, dlEastingToeSPSIndexArray[i].second);

					for (int i = 0; i < (int)dlDuneHeelIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlDuneHeelIndexArray[i].first, m_colDLDuneHeel, dlDuneHeelIndexArray[i].second);

					for (int i = 0; i < (int)dlFrontSlopeIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlFrontSlopeIndexArray[i].first, m_colDLFrontSlope, dlFrontSlopeIndexArray[i].second);

					for (int i = 0; i < (int)dlDuneToeIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlDuneToeIndexArray[i].first, m_colDLDuneToe, dlDuneToeIndexArray[i].second);

					for (int i = 0; i < (int)dlDuneCrestIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlDuneCrestIndexArray[i].first, m_colDLDuneCrest, dlDuneCrestIndexArray[i].second);

					for (int i = 0; i < (int)dlDuneWidthIndexArray.GetSize(); i++)
						m_pDuneLayer->SetData(dlDuneWidthIndexArray[i].first, m_colDLDuneWidth, dlDuneWidthIndexArray[i].second);

					Poly* pPoly = m_pDuneLayer->GetPolygon(pt);
					if (pPoly->GetVertexCount() > 0)
					{
						pPoly->m_vertexArray[0].x = eastingToe;
						pPoly->InitLogicalPoints(m_pMap);
					}

					// Annual County wide cost building SPS
					m_restoreCostSPS += cost;
					// Annual County wide volume of sand of building SPS
					m_constructVolumeSPS += (float)constructVolume;
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
	//   for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	//   {
	//      int duneBldgIndex = -1;
	//      m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, duneBldgIndex);
	//
	//      if (duneBldgIndex != -1)
	//      {
	//         MapLayer::Iterator endPoint = point;
	//         MapLayer::Iterator maxPoint = point;
	//
	//         MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndex);
	//         float eroFreq = eroMovingWindow->GetFreqValue();
	//         m_pDuneLayer->SetData(point, m_colDLDuneEroFreq, eroFreq);
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
	//            m_pDuneLayer->GetData(maxPoint, m_colDLDuneToe, duneToe);
	//            m_pDuneLayer->GetData(maxPoint, m_colDLDuneCrest, duneCrest);
	//            m_pDuneLayer->GetData(maxPoint, m_colDLEastingToe, eastingToe);
	//            m_pDuneLayer->GetData(maxPoint, m_colDLBeachWidth, beachWidth);
	//            m_pDuneLayer->GetData(maxPoint, m_colDLTranSlope, tanb);
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
	//         m_pDuneLayer->GetPointCoords(endPoint, xCoord, yTop);
	//         m_pDuneLayer->GetPointCoords(point, xCoord, yBtm);
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
	//      //      m_pDuneLayer->SetData(pt, m_colDLBeachType, BchT_RIPRAP_BACKED);
	//            m_pDuneLayer->SetData(pt, m_colDLDuneCrest, m_duneCrestSPS);
	//            m_pDuneLayer->SetData(pt, m_colDLAddYearSPS, currentYear);
	//            m_pDuneLayer->SetData(pt, m_colDLCostSPS, SPSCost);
	//            m_pDuneLayer->SetData(pt, m_colDLLengthSPS, SPSLength);
	//         
	//            m_pDuneLayer->SetData(pt, m_colDLEastingCrest, eastingCrest);
	//            m_pDuneLayer->SetData(pt, m_colDLDuneToe, m_duneToeSPS);
	//            m_pDuneLayer->SetData(pt, m_colDLBeachWidth, beachWidth);
	//      //      m_pDuneLayer->SetData(pt, m_colDLEastingToe, eastingToe);
	//            
	//            // Easting location of rebuilt dune
	//            m_pDuneLayer->SetData(pt, m_colDLEastingToeSPS, eastingToe);
	//
	//            Poly *pPoly = m_pDuneLayer->GetPolygon(pt);
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
	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
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
				m_pDuneLayer->GetData(pt, m_colDLDuneToeSPS, duneToeSPS);

				REAL eastingToeSPS = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLEastingToeSPS, eastingToeSPS);

				float height = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLHeightSPS, height);

				float btmWidth = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLWidthSPS, btmWidth);

				double constructVolume = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLConstructVolumeSPS, constructVolume);

				float duneCrest = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLDuneCrest, duneCrest);

				float duneHeel = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLDuneHeel, duneHeel);

				// Retrieve current values 
				float duneToe = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneToe);

				double eastingToe = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);

				double beachWidth = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLBeachWidth, beachWidth);

				float tanb = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLTranSlope, tanb);

				float frontSlope = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLFrontSlope, frontSlope);

				float backSlope = 0.0;
				m_pDuneLayer->GetData(pt, m_colDLBackSlope, backSlope);

				float xToeShift = (float)(eastingToe - eastingToeSPS);
				float erodedCrest = duneCrest;
				float width = btmWidth - xToeShift;
				if (xToeShift > m_topWidthSPS)
					erodedCrest -= (xToeShift - m_topWidthSPS) * backSlope;

				//double beachArea = xToeShift * (eastingToeSPS - MHW);
				double beachArea = xToeShift * (duneToeSPS - MHW);
				double totalVolume = (CalculateArea(duneToe, erodedCrest, duneHeel, frontSlope, width) + beachArea) * m_elevCellHeight;

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
				PolicyInfo& pi = m_policyInfoArray[PI_MAINTAIN_SPS];

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

					m_pDuneLayer->SetData(pt, m_colDLDuneCrest, duneCrest);
					m_pDuneLayer->SetData(pt, m_colDLMaintYearSPS, currentYear);

					m_pDuneLayer->SetData(pt, m_colDLCostSPS, cost);

					m_pDuneLayer->SetData(pt, m_colDLHeightSPS, height);
					m_pDuneLayer->SetData(pt, m_colDLLengthSPS, m_elevCellHeight);

					m_pDuneLayer->SetData(pt, m_colDLDuneToe, duneToeSPS);
					m_pDuneLayer->SetData(pt, m_colDLBeachWidth, beachWidth);
					m_pDuneLayer->SetData(pt, m_colDLMaintVolumeSPS, rebuildVolume);

					//   m_pDuneLayer->SetData(pt, m_colDLEastingCrest, eastingCrest);
					m_pDuneLayer->SetData(pt, m_colDLEastingToe, eastingToeSPS);
					m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);


					Poly* pPoly = m_pDuneLayer->GetPolygon(pt);
					if (pPoly->GetVertexCount() > 0)
					{
						pPoly->m_vertexArray[0].x = eastingToe;
						pPoly->InitLogicalPoints(m_pMap);
					}

					// Annual County wide cost maintaining SPS
					m_maintCostSPS += cost;
					// Annual County wide volume of sand of maintaining SPS
					m_maintVolumeSPS += (float)rebuildVolume;
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

	//for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
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

	//   int duneBldgIndex = -1;
	//   int iduIndex = -1;
	//   int duneIndex = -1;

	//   m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, duneBldgIndex);
	//   m_pDuneLayer->GetData(point, m_colDLDuneIndex, duneIndex);

	//   float xAvgKD = 0.0f;
	//   m_pDuneLayer->GetData(point, m_colDLAvgKD, xAvgKD);

	//   // Moving Window of the yearly maximum TWL
	//   MovingWindow *twlMovingWindow = m_TWLArray.GetAt(point);

	//   // Retrieve the maxmum TWL within the designated window
	//   float movingMaxTWL = twlMovingWindow->GetMaxValue();
	//   //   m_pDuneLayer->SetData(point, m_colMvMaxTWL, movingMaxTWL);

	//   // Retrieve the average TWL within the designated window
	//   float movingAvgTWL = twlMovingWindow->GetAvgValue();
	//   //   m_pDuneLayer->SetData(point, m_colMvAvgTWL, movingAvgTWL);

	//   MovingWindow *ipdyMovingWindow = m_IDPYArray.GetAt(point);
	//   // Retrieve the average impact days per yr within the designated window
	//   float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();
	////   m_pDuneLayer->SetData(point, m_colDLMvAvgIDPY, movingAvgIDPY);

	//   m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
	//   m_pDuneLayer->GetData(point, m_colDLNorthing, northing);
	//   //northingTop = northingBtm = northing;

	//   m_pDuneLayer->GetData(point, m_colDLEastingCrest, eastingCrest);
	//   
	//   bool isDunePtWithinIDU = true;
	//   // Can constrain based upon permits or global cost
	//   // ex: if ( numCurrentPermits < numAnnualPermits)

	//   // only construct BPS at dunes that are protecting buildings
	//   bool isImpactedByEErosion = false;
	//   if (duneBldgIndex != -1)
	//   {
	//      MovingWindow *eroMovingWindow = m_eroBldgFreqArray.GetAt(duneBldgIndex);
	//      float eroFreq = eroMovingWindow->GetFreqValue();
	//      m_pDuneLayer->SetData(point, m_colDLDuneEroFreq, eroFreq);
	//      isImpactedByEErosion = IsBldgImpactedByEErosion(duneBldgIndex, xAvgKD);// , iduIndex);
	//   }

	//   // construct BPS by changing beachtype to RIP RAP BACKED
	//   if (beachType == BchT_SANDY_DUNE_BACKED || (movingAvgIDPY >= buildBPSThreshold ) )// || isImpactedByEErosion) )
	//   {
	//      // determine new height of duneCrest

	//      // construct BPS to height of the maximum TWL within the last x years
	//      if (m_floodTimestep == 1)
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
	//      //   m_pDuneLayer->GetData(point, m_colAvgTWL, newCrest);

	//      m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToe);
	//      m_pDuneLayer->GetData(point, m_colDLDuneCrest, duneCrest);

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
	//   if (iduIndex != -1)
	//   {
	//      m_pIDULayer->GetData(iduIndex, m_colIDUNorthingTop, northingTop);
	//      m_pIDULayer->GetData(iduIndex, m_colIDUNorthingBottom, northingBtm);
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
	//      m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
	//      m_pDuneLayer->GetData(point, m_colDLNorthing, northing);

	//      m_pDuneLayer->SetData(point, m_colDLBeachType, BchT_RIPRAP_BACKED);
	//      m_pDuneLayer->SetData(point, m_colDLDuneCrest, newCrest);
	//      m_pDuneLayer->SetData(point, m_colBPSAddYear, currentYear);
	//      m_pDuneLayer->SetData(point, m_colDLGammaRough, m_minBPSRoughness);

	//      m_hardenedShoreline++;

	//      m_pDuneLayer->GetData(point, m_colDLEastingToe, eastingToe);
	//      eastingCrest = eastingToe + newCrest / m_slopeBPS;
	//      m_pDuneLayer->SetData(point, m_colDLEastingCrest, eastingCrest);

	//      if (northing < northingBtm || northing > northingTop)
	//         isDunePtWithinIDU = false;

	//      point++;

	//      m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
	//      m_pDuneLayer->GetData(point, m_colDLNorthing, northing);

	//   } while (beachType == BchT_SANDY_DUNE_BACKED && isDunePtWithinIDU && point < m_pDuneLayer->End());

	//      
	//      

	//      //MapLayer::Iterator dunePt = point;

	//      //while (beachType == BchT_SANDY_DUNE_BACKED && isDunePtWithinIDU)// && dunePt < m_pDuneLayer->End() )
	//      //   {
	//      //      // construct BPS to height of the maximum TWL within the last x years
	//      //      // change beachtype
	//      //      m_pDuneLayer->SetData(dunePt, m_colDLBeachType, BchT_RIPRAP_BACKED);
	//      //      m_pDuneLayer->SetData(dunePt, m_colDLDuneCrest, newCrest);
	//      //      m_pDuneLayer->SetData(dunePt, m_colBPSAddYear, currentYear);
	//      //      m_pDuneLayer->SetData(dunePt, m_colDLGammaRough, m_minBPSRoughness);

	//      //      m_hardenedShoreline++;

	//      //      m_pDuneLayer->GetData(dunePt, m_colDLEastingToe, eastingToe);
	//      //      eastingCrest = eastingToe + newCrest / m_slopeBPS;
	//      //      m_pDuneLayer->SetData(dunePt, m_colDLEastingCrest, eastingCrest);

	//      //      m_pDuneLayer->GetData(dunePt, m_colDLBeachType, beachType);
	//      //      m_pDuneLayer->GetData(dunePt, m_colDLNorthing, northing);

	//      //      if (northing > northingTop || northing < northingBtm)
	//      //         isDunePtWithinIDU = false;

	//      //      //   dunePt++;
	//      //   }

	//         /*point = dunePt--;*/

	//            /* m_pDuneLayer->SetData(point, m_colCPolicyL, PolicyArray[SNumber]);
	//             m_pDuneLayer->SetData(point, m_colBPSCost, BPSCost);*/

	//             /* if ((currentYear) > 2010)
	//               m_pDuneLayer->SetData(point, m_colBPSAddYear, 2040);
	//              if ((currentYear) > 2040)
	//               m_pDuneLayer->SetData(point, m_colBPSAddYear, 2060);
	//              if ((currentYear) > 2060)
	//               m_pDuneLayer->SetData(point, m_colBPSAddYear, 2100);*/


	//               // how does beachwidth change dependent upon BPS construction

	//                //// Beachwidth changes based on a 2:1 slope
	//                //float beachwidth;
	//                //m_pDuneLayer->GetData(point, m_colDLBeachWidth, beachwidth);
	//                //beachwidth -= (BPSHeight * 1.0f);
	//                //if ((duneToe / beachwidth) > 0.1f)
	//                //   beachwidth = duneToe / 0.1f;

	//                //m_pDuneLayer->SetData(point, m_colDLBeachWidth, beachwidth);
	//            // get information from next dune Point
	//         /*   point++;
	//         // Move North, building BPS
	//            dunePtIndex--;



	//         */
	//   //   }
	//         

	//   }

	//   // end dune line points

	//   //  if (idpy >= buildBPSThreshold && ( beachType != BchT_BAY ) )
	//   //  {     
	//   //     m_pDuneLayer->SetData(point, m_colPrevBeachType, beachType);
	//   //     m_pDuneLayer->SetData(point, m_colDLBeachType, BchT_RIPRAP_BACKED);
	//   ////     m_pDuneLayer->SetData(point, m_colBPSMaintYear, currentYear);
	//   //     m_pDuneLayer->SetData(point, m_colDLDuneCrest, m_BPSHeight);
	//   //     m_hardenedShoreline += 10;
	//   //  } // end impact days exceeds threshold

	////   delete ipdyMovingWindow;
	//}

	// m_pDuneLayer->m_readOnly = true;

} // end ConstructBPS(int currentYear)

bool ChronicHazards::IsMissingRow(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint)
{


	return false;
}

bool ChronicHazards::CalculateBPSExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint, MapLayer::Iterator& maxPoint)
{
	float buildBPSThreshold = m_idpyFactor * 365.0f;

	bool constructBPS = false;
	int bldgIndex = -1;
	int nextBldgIndex = -1;

	float maxTWL = -10000.0f;

	m_pDuneLayer->GetData(startPoint, m_colDLDuneBldgIndex, bldgIndex);
	nextBldgIndex = bldgIndex;

	//before loop startPoint == endPoint
	while (bldgIndex == nextBldgIndex && endPoint < m_pDuneLayer->End())
	{
		int beachType = -1;
		m_pDuneLayer->GetData(endPoint, m_colDLBeachType, beachType);

		if (beachType == BchT_SANDY_DUNE_BACKED)
			//&& movingAvgIDPY >= buildBPSThreshold && isImpactedByEErosion)
		{
			MovingWindow* ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
			///   float movingAvgIDPY1 = ipdyMovingWindow->GetAvgValue();

			float movingAvgIDPY = 0.0f;
			m_pDuneLayer->GetData(endPoint, m_colDLMvAvgIDPY, movingAvgIDPY);

			float xAvgKD = 0.0f;
			m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, xAvgKD);

			float avgKD = 0.0f;
			m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, avgKD);
			float distToBldg = 0.0f;
			m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgDist, distToBldg);

			bool isImpactedByEErosion = false;
			isImpactedByEErosion = IsBldgImpactedByEErosion(nextBldgIndex, avgKD, distToBldg);

			// do any dune points cause trigger
			if (movingAvgIDPY >= buildBPSThreshold || isImpactedByEErosion)
			{
				MovingWindow* twlMovingWindow = m_TWLArray.GetAt(endPoint);

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
		if (endPoint < m_pDuneLayer->End())
			m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, nextBldgIndex);
	}

	return constructBPS;

} // end CalculateBPSExtent

bool ChronicHazards::CalculateExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint, MapLayer::Iterator& maxPoint)
{
	float buildThreshold = m_idpyFactor * 365.0f;

	bool isConstruct = false;
	int bldgIndex = -1;
	int nextBldgIndex = -1;

	float maxTWL = -10000.0f;

	m_pDuneLayer->GetData(startPoint, m_colDLDuneBldgIndex, bldgIndex);
	nextBldgIndex = bldgIndex;

	while (bldgIndex == nextBldgIndex && endPoint < m_pDuneLayer->End())
	{
		int beachType = -1;
		m_pDuneLayer->GetData(endPoint, m_colDLBeachType, beachType);

		if (beachType == BchT_SANDY_DUNE_BACKED)
			//&& movingAvgIDPY >= buildBPSThreshold && isImpactedByEErosion)
		{
			MovingWindow* ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
			float movingAvgIDPY1 = ipdyMovingWindow->GetAvgValue();

			float movingAvgIDPY = 0.0f;
			m_pDuneLayer->GetData(endPoint, m_colDLMvAvgIDPY, movingAvgIDPY);

			float xAvgKD = 0.0f;
			m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, xAvgKD);

			float avgKD = 0.0f;
			m_pDuneLayer->GetData(endPoint, m_colDLAvgKD, avgKD);
			float distToBldg = 0.0f;
			m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgDist, distToBldg);

			bool isImpactedByEErosion = false;
			isImpactedByEErosion = IsBldgImpactedByEErosion(nextBldgIndex, avgKD, distToBldg);

			if (movingAvgIDPY >= buildThreshold || isImpactedByEErosion)
			{
				MovingWindow* twlMovingWindow = m_TWLArray.GetAt(endPoint);

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
		if (endPoint < m_pDuneLayer->End())
			m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, nextBldgIndex);
	}

	return isConstruct;

} // end CalculateExtent

int ChronicHazards::WalkSouth(MapLayer::Iterator dunePt, float newCrest, int currentYear, double northingTop, double northingBtm)
{
	if (dunePt > m_pDuneLayer->Begin())
		dunePt--;
	else
		return 0;

	bool isDunePtWithinIDU = true;;

	int beachType = -1;
	double northing = 0.0;

	m_pDuneLayer->GetData(dunePt, m_colDLBeachType, beachType);
	m_pDuneLayer->GetData(dunePt, m_colDLNorthing, northing);

	if (northing < northingBtm || northing > northingTop)
		isDunePtWithinIDU = false;

	int count = 0;

	while (beachType == BchT_SANDY_DUNE_BACKED && isDunePtWithinIDU && dunePt > m_pDuneLayer->Begin());
	{
		// change beachtype
		m_pDuneLayer->SetData(dunePt, m_colDLBeachType, BchT_SANDY_RIPRAP_BACKED);
		m_pDuneLayer->SetData(dunePt, m_colDLDuneCrest, newCrest);
		m_pDuneLayer->SetData(dunePt, m_colDLAddYearBPS, currentYear);
		m_pDuneLayer->SetData(dunePt, m_colDLGammaRough, m_minBPSRoughness);
		count = 1;

		//   m_hardenedShoreline++;

		float eastingToe = 0.0f;
		m_pDuneLayer->GetData(dunePt, m_colDLEastingToe, eastingToe);
		float eastingCrest = eastingToe + newCrest / m_slopeBPS;
		m_pDuneLayer->SetData(dunePt, m_colDLEastingCrest, eastingCrest);

		dunePt--;
		m_pDuneLayer->GetData(dunePt, m_colDLBeachType, beachType);
		m_pDuneLayer->GetData(dunePt, m_colDLNorthing, northing);

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
  //   while (beachType == BchT_SANDY_DUNE_BACKED && isDunePtWithinIDU) // && dunePt < m_pDuneLayer->End() )
  //   {
  //      // construct BPS to height of the maximum TWL within the last x years
  //
  //      // change beachtype
  //      m_pDuneLayer->SetData(pt, m_colDLBeachType, BchT_RIPRAP_BACKED);
  //      m_pDuneLayer->SetData(pt, m_colDLDuneCrest, newCrest);
  //      m_pDuneLayer->SetData(pt, m_colBPSAddYear, currentYear);
  //      m_pDuneLayer->SetData(pt, m_colDLGammaRough, m_minBPSRoughness);
  //
  //      m_hardenedShoreline++;
  //
  //      m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);
  //      eastingCrest = eastingToe + newCrest / m_slopeBPS;
  //      m_pDuneLayer->SetData(pt, m_colDLEastingCrest, eastingCrest);
  //
  //      m_pDuneLayer->GetData(pt, m_colDLBeachType, beachType);
  //      m_pDuneLayer->GetData(pt, m_colDLNorthing, northing);
  //
  //      //   if (northing > northingTop || northing < northingBtm)
  //      isDunePtWithinIDU = false;
  //
  //      pt++;
  //
  //      /* m_pDuneLayer->SetData(point, m_colCPolicyL, PolicyArray[SNumber]);
  //      m_pDuneLayer->SetData(point, m_colBPSCost, BPSCost);*/
  //
  //      /* if ((currentYear) > 2010)
  //      m_pDuneLayer->SetData(point, m_colBPSAddYear, 2040);
  //      if ((currentYear) > 2040)
  //      m_pDuneLayer->SetData(point, m_colBPSAddYear, 2060);
  //      if ((currentYear) > 2060)
  //      m_pDuneLayer->SetData(point, m_colBPSAddYear, 2100);*/
  //
  //
  //      // how does beachwidth change dependent upon BPS construction
  //
  //      //// Beachwidth changes based on a 2:1 slope
  //      //float beachwidth;
  //      //m_pDuneLayer->GetData(point, m_colDLBeachWidth, beachwidth);
  //      //beachwidth -= (BPSHeight * 1.0f);
  //      //if ((duneToe / beachwidth) > 0.1f)
  //      //   beachwidth = duneToe / 0.1f;
  //
  //      //m_pDuneLayer->SetData(point, m_colDLBeachWidth, beachwidth);
  //      // get information from next dune Point
  //      /*   point++;
  //      // Move North, building BPS
  //      dunePtIndex--;
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


void ChronicHazards::RemoveBPS(EnvContext* pEnvContext)
{
	//   Only remove BPS if they are not protecting buildings and if they are  ???? at the end of a BPS segment

	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		int beachType = 0;
		m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);

		//   Only remove BPS if they are not protecting buildings and if they are at the end of a BPS segment
		//   if (BuildingsArray[ SNumber ] == 0 && beachType == BchT_RIPRAP_BACKED && (duneTypeArray[ SNumber - 1 ] == 1 || duneTypeArray[ SNumber + 1 ] == 1))
		{
			float Cost;
			m_pDuneLayer->SetData(point, m_colDLBeachType, 1);
			m_pDuneLayer->SetData(point, m_colDLRemoveYearBPS, pEnvContext->currentYear);
			m_pDuneLayer->GetData(point, m_colDLCostBPS, Cost);
			//         m_BPSRemovalCost += Cost;
		}
	} // end dune pts

} // end RemoveBPS


  // BPS is maintained at front of structure, reducing beachwidth and moving structure toe seaward
void ChronicHazards::MaintainBPS(int currentYear)
{
	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		int beachType = -1;
		m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);

		MapLayer::Iterator endPoint = point;
		MapLayer::Iterator maxPoint = point;

		// When did we last increase the height of the structure? 
		int lastMaintYear;
		m_pDuneLayer->GetData(endPoint, m_colDLMaintYearBPS, lastMaintYear);

		int addedBPSYear;
		m_pDuneLayer->GetData(endPoint, m_colDLAddYearBPS, addedBPSYear);

		if (addedBPSYear == 2010)
			lastMaintYear = 2010;

		int yrsSinceMaintained = currentYear - lastMaintYear;

		if (beachType == BchT_SANDY_RIPRAP_BACKED && (yrsSinceMaintained > m_maintFreqBPS))
		{
			//Do we need to increase the height of the BPS to keep up with increasing TWL?
			int bldgIndex = -1;
			int nextBldgIndex = -1;

			float amtTopped = 0.0f;
			float newCrest = 0.0f;
			float oldCrest = 0.0f;
			double eastingToe = 0.0;

			bool isBldgFlooded = false;

			m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, bldgIndex);
			nextBldgIndex = bldgIndex;

			if (bldgIndex != -1)
			{
				while (bldgIndex == nextBldgIndex && endPoint < m_pDuneLayer->End())
				{
					m_pDuneLayer->GetData(endPoint, m_colDLDuneCrest, oldCrest);
					newCrest = oldCrest;

					// Retrieve the maximum TWL within the designated window
					MovingWindow* twlMovingWindow = m_TWLArray.GetAt(endPoint);
					float movingMaxTWL = twlMovingWindow->GetMaxValue();

					// Retrieve the Flooding Frequency of the Protected Building
					MovingWindow* floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);
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
					if (endPoint < m_pDuneLayer->End())
						m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, nextBldgIndex);
				}
			}

			if (isBldgFlooded)
			{
				MapLayer::Iterator endPoint = point;
				MapLayer::Iterator minPoint = point;

				m_pDuneLayer->GetData(maxPoint, m_colDLDuneBldgIndex, bldgIndex);
				nextBldgIndex = bldgIndex;

				float maxBPSHeight = -10000.0f;

				float duneToe = 0.0f;

				// find dune point of Maximum BPS Height 
				while (bldgIndex == nextBldgIndex && endPoint < m_pDuneLayer->End())
				{
					m_pDuneLayer->GetData(endPoint, m_colDLDuneToe, duneToe);

					if ((newCrest - duneToe) > maxBPSHeight)
					{
						maxBPSHeight = newCrest - duneToe;
						minPoint = endPoint;
					}
					endPoint++;
					if (endPoint < m_pDuneLayer->End())
						m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, nextBldgIndex);
				}

				// engineering limit
				if (maxBPSHeight > m_maxBPSHeight)
				{
					maxBPSHeight = m_maxBPSHeight;
					m_pDuneLayer->GetData(minPoint, m_colDLDuneToe, duneToe);
					newCrest = duneToe + m_maxBPSHeight;
				}

				float BPSLength = 0.0f;
				m_pDuneLayer->GetData(point, m_colDLLengthBPS, BPSLength);

				float cost = 0.02f * m_costs.BPS * BPSLength;
				if ((newCrest - oldCrest) > 0)
					cost += (newCrest - oldCrest) * m_costs.BPS * BPSLength;

				// get budget infor for Constructing BPS policy
				PolicyInfo& pi = m_policyInfoArray[PI_MAINTAIN_BPS];

				// enough funds in original allocation ?
				bool passCostConstraint = true;

				if (pi.HasBudget() && pi.m_availableBudget <= cost) //armorAllocationResidual >= cost)
					passCostConstraint = false;

				AddToPolicySummary(currentYear, PI_MAINTAIN_BPS, int(point), m_costs.BPS, cost, pi.m_availableBudget, BPSLength, (newCrest - oldCrest), passCostConstraint);

				if (passCostConstraint)
				{
					pi.IncurCost(cost);      // charge to budget

					float priorBPSCost;
					m_pDuneLayer->GetData(point, m_colDLCostBPS, priorBPSCost);
					float addedBPSCost = cost + priorBPSCost;
					m_pDuneLayer->SetData(point, m_colDLCostBPS, addedBPSCost);
					m_maintCostBPS += cost;

					for (MapLayer::Iterator pt = point; pt < endPoint; pt++)
					{
						// Beachwidth changes based on a 1:2 slope 
						float beachWidth = 0.0f;
						m_pDuneLayer->GetData(pt, m_colDLBeachWidth, beachWidth);

						float duneCrest = 0.0f;
						m_pDuneLayer->GetData(pt, m_colDLDuneCrest, duneCrest);

						float duneToe = 0.0f;
						m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneToe);

						float tanb = 0.0f;
						m_pDuneLayer->GetData(pt, m_colDLTranSlope, tanb);

						m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);

						double eastingCrest = 0.0f;
						m_pDuneLayer->GetData(pt, m_colDLEastingCrest, eastingCrest);

						if ((newCrest - duneCrest) > 0.0f)
						{
							//beachWidth -= ((newCrest - duneCrest) / m_slopeBPS);
							//   double newEastingToe = (newCrest + (tanb - m_slopeBPS) * eastingToe) / (tanb - m_slopeBPS);
							double newEastingToe = (newCrest - m_slopeBPS * eastingCrest - duneToe + (tanb * eastingToe)) / (tanb - m_slopeBPS);
							float deltaX = (float)(eastingToe - newEastingToe);

							beachWidth -= deltaX;
							if ((duneToe / beachWidth) > 0.1f)
								beachWidth = duneToe / 0.1f;

							//if (m_debugOn)
							//   {
							//   m_pDuneLayer->SetData(pt, m_colNewEasting, newEastingToe);
							//   //      m_pDuneLayer->SetData(point, m_colBWidth, deltaX2);
							//   m_pDuneLayer->SetData(pt, m_colDeltaX, deltaX);
							//   }

							// DuneCrest height is set to structure height
							m_pDuneLayer->SetData(pt, m_colDLDuneCrest, newCrest);
							m_pDuneLayer->SetData(pt, m_colDLBeachWidth, beachWidth);
							m_pDuneLayer->SetData(pt, m_colDLMaintYearBPS, currentYear);
							m_pDuneLayer->SetData(pt, m_colDLCostMaintBPS, cost);
							//
							// change easting of DuneToe

							//eastingToe -= ((newCrest - duneCrest) / m_slopeBPS);
							/*eastingToe = newEastingToe;
							m_pDuneLayer->SetData(pt, m_colDLEastingToe, eastingToe);*/

							m_pDuneLayer->SetData(pt, m_colDLEastingToe, newEastingToe);


							Poly* pPoly = m_pDuneLayer->GetPolygon(pt);
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
	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		/*int bType = -1;
		int beachType = -1;
		m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);*/

		MapLayer::Iterator endPoint = point;
		MapLayer::Iterator maxPoint = point;

		// When did we last increase the height of the structure? 
		int maintFreq = 0;
		int lastMaintYear = 2010;
		int builtYear = 0;

		/*   if (isBPS)
		{*/
		m_pDuneLayer->GetData(endPoint, m_colDLMaintYearBPS, lastMaintYear);
		m_pDuneLayer->GetData(endPoint, m_colDLAddYearBPS, builtYear);
		maintFreq = m_maintFreqBPS;
		//}
		//else // if SPS
		//{
		//   m_pDuneLayer->GetData(endPoint, m_colDLMaintYearSPS, lastMaintYear);
		//   m_pDuneLayer->GetData(endPoint, m_colDLAddYearSPS, builtYear);
		//}

		/*if (beachType == BchT_RIPRAP_BACKED && (lastMaintYear + m_maintFreqBPS < currentYear))
		{*/


		if (builtYear > 0 && lastMaintYear + maintFreq < currentYear)
		{
			//Do we need to increase the height of the BPS to keep up with increasing TWL?
			int bldgIndex = -1;
			int nextBldgIndex = -1;

			float amtTopped = 0.0f;
			float newCrest = 0.0f;
			float oldCrest = 0.0f;
			double eastingToe = 0.0;

			bool isBldgFlooded = false;

			m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, bldgIndex);
			nextBldgIndex = bldgIndex;

			while (bldgIndex == nextBldgIndex && endPoint < m_pDuneLayer->End())
			{
				m_pDuneLayer->GetData(endPoint, m_colDLDuneCrest, oldCrest);
				newCrest = oldCrest;

				// Retrieve the maxmum TWL within the designated window
				MovingWindow* twlMovingWindow = m_TWLArray.GetAt(endPoint);
				float movingMaxTWL = twlMovingWindow->GetMaxValue();

				// Retrieve the Flooding Frequency of the Protected Building
				MovingWindow* floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);
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
				if (endPoint < m_pDuneLayer->End())
					m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, nextBldgIndex);
			}

			endPoint = point;
			MapLayer::Iterator minPoint = point;

			m_pDuneLayer->GetData(point, m_colDLDuneBldgIndex, bldgIndex);
			nextBldgIndex = bldgIndex;

			float maxStructureHeight = -10000.0f;

			float duneToe = 0.0f;

			// find dune point of Maximum Height 
			while (bldgIndex == nextBldgIndex && endPoint < m_pDuneLayer->End())
			{
				m_pDuneLayer->GetData(endPoint, m_colDLDuneToe, duneToe);

				if ((newCrest - duneToe) > maxStructureHeight)
				{
					maxStructureHeight = newCrest - duneToe;
					minPoint = endPoint;
				}
				endPoint++;
				if (endPoint < m_pDuneLayer->End())
					m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, nextBldgIndex);
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
			//      m_pDuneLayer->GetData(minPoint, m_colDLDuneToe, duneToe);
			//      newCrest = duneToe + heightLimit;
			//   }

			float structureSlope = 0.0f;

			/*if (isBPS)
			{*/
			// set structure slope based upon a 1:2 slope
			structureSlope = m_slopeBPS;

			// calculate costs based upon which hard protection structure
			float structureLength = 0.0f;
			m_pDuneLayer->GetData(point, m_colDLLengthBPS, structureLength);
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
			//      m_pDuneLayer->GetData(point, m_colDLLengthSPS, structureLength);
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
				m_pDuneLayer->GetData(pt, m_colDLBeachWidth, beachWidth);

				float duneCrest = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLDuneCrest, duneCrest);

				/*float duneToe = 0.0f;
				m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneToe);*/


				/*if (isBPS)
				{*/
				if ((newCrest - duneCrest) > 0.0f)
				{
					beachWidth -= ((newCrest - duneCrest) / structureSlope);
					if ((duneToe / beachWidth) > 0.1f)
						beachWidth = duneToe / 0.1f;

					// DuneCrest height is set to structure height
					m_pDuneLayer->SetData(pt, m_colDLDuneCrest, newCrest);
					m_pDuneLayer->SetData(pt, m_colDLBeachWidth, beachWidth);
					//   if (isBPS)
					m_pDuneLayer->SetData(pt, m_colDLMaintYearBPS, currentYear);
					m_pDuneLayer->SetData(pt, m_colDLCostMaintBPS, cost);

					/*   else
					m_pDuneLayer->SetData(pt, m_colDLMaintYearSPS, currentYear);*/

					// change easting of DuneToe since structure is maintained on seaward side
					m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);
					eastingToe -= ((newCrest - duneCrest) / structureSlope);
					m_pDuneLayer->SetData(pt, m_colDLEastingToe, eastingToe);

					Poly* pPoly = m_pDuneLayer->GetPolygon(pt);
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
	//for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
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

	//   m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToe);
	//   m_pDuneLayer->GetData(point, m_colDLDuneCrest, duneCrest);
	//   //    m_pDuneLayer->GetData(point, m_colBackshoreElev, backshoreElev);
	//   m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);

	//   //if ( ( duneCrest < movingAvgTWL || duneCrest < movingMaxTWL ) && beachType == BchT_RIPRAP_BACKED ) // && duneCrest < backshoreElev )
	//   if (duneCrest < movingMaxTWL && beachType == BchT_RIPRAP_BACKED) // && duneCrest < backshoreElev )
	//   {
	//      if ((lastMaintYear + m_maintFreqBPS) < currentYear)
	//      {
	//         float newCrest = 0.0f;
	//         float beachwidth = 0.0f;

	//         if (m_floodTimestep == 1)
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
	//         m_pDuneLayer->SetData(point, m_colDLDuneCrest, newCrest);

	//         // Beachwidth changes based on a 2:1 slope 
	//         m_pDuneLayer->GetData(point, m_colDLBeachWidth, beachwidth);
	//         if ((newCrest - duneCrest) > 0.0f)
	//         {

	//            beachwidth -= ((newCrest - duneCrest) * 1.0f);
	//            if ((duneToe / beachwidth) > 0.1f)
	//               beachwidth = duneToe / 0.1f;

	//            //  float BPSCost = (newCrest - duneCrest) * m_costs.BPS * pPoly->GetEdgeLength();;

	//            m_pDuneLayer->SetData(point, m_colDLBeachWidth, beachwidth);
	//            m_pDuneLayer->SetData(point, m_colBPSMaintYear, currentYear);

	//            /*if (Loc == 7)
	//            {

	//            m_RockBPSMaintenance += BPSCost;

	//            }
	//            if (Loc == 1)
	//            {

	//            m_NeskBPSMaintenance += BPSCost;

	//            }
	//            float priorBPSCost;
	//            m_pDuneLayer->GetData(i, m_colBPSCost, priorBPSCost);
	//            float addntlBPSCost = BPSCost + priorBPSCost;
	//            m_pDuneLayer->SetData(i, m_colBPSCost, addntlBPSCost);
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

	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
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
				m_pDuneLayer->GetData(pt, m_colDLEastingToe, eastingToe);
				m_pDuneLayer->GetData(pt, m_colDLEastingToeSPS, eastingToeSPS);

				if ((eastingToe - eastingToeSPS) >= 3.0)
				{
					m_pDuneLayer->SetData(pt, m_colDLNourishYearSPS, nourishYear);

					if ((nourishYear + m_nourishFreq) < currentYear || nourishYear == 0)
					{
						m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneToe);
						m_pDuneLayer->GetData(pt, m_colDLDuneCrest, duneCrest);

						//  volume of sand needed along trapezoid face + volume of sand needed to restore beachwidth 
						//  V = W * H * L - volume of triangular prism + volume of triangular prism
						volume = (eastingToe - eastingToeSPS) * (duneCrest - duneToe) * m_elevCellHeight; // -0.5 * cellHeight * (eastingToe - eastingToeSPS) * (duneToe - duneToeSPS);
																									  //}

						m_pDuneLayer->GetData(pt, m_colDLNourishYearSPS, nourishYear);

						m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneToe);
						m_pDuneLayer->GetData(pt, m_colDLDuneCrest, duneCrest);

						float bruunSlope;
						m_pDuneLayer->GetData(pt, m_colDLBeachWidth, beachWidth);
						m_pDuneLayer->GetData(pt, m_colDLShorelineChange, shorelineChangeRate);
						m_pDuneLayer->GetData(pt, m_colDLBruunSlope, bruunSlope);

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
							nourishSLR = ((slr - prevslr) / bruunSlope) * m_nourishFactor;

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
						m_pDuneLayer->SetData(pt, m_colDLEastingShoreline, m_colDLEastingToe - newBeachWidth);

						// Calculate added volume for a the front facing trapezoid
						// 
						volume += (0.5f * m_elevCellHeight * duneToe * (newBeachWidth - beachWidth));

						if (volume < 0)
							volume = 0;

						float cost = float(volume * m_costs.nourishment);

						// get budget info
						PolicyInfo& pi = m_policyInfoArray[PI_NOURISH_SPS];

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
							m_pDuneLayer->GetData(pt, m_colDLNourishFreqSPS, nourishFreq);
							nourishFreq += 1;

							// Set new values in Duneline layer
							//         m_pDuneLayer->SetData(pt, m_colDLBeachWidth, newBeachWidth);
							m_pDuneLayer->SetData(pt, m_colDLNourishFreqSPS, nourishFreq);

							//   float newBeachSlope = duneToe / newBeachWidth;

							//   m_pDuneLayer->SetData(pt, m_colDLTranSlope, newBeachSlope);
							m_pDuneLayer->SetData(pt, m_colDLNourishYearSPS, currentYear);
							m_pDuneLayer->SetData(pt, m_colDLNourishVolSPS, volume);

							// Calculate County wide metrics (volume, cost)
							m_nourishVolumeSPS += (float)volume;
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

	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		double volume = 0.0;
		int lastNourishYear = 0;

		float duneToe = 0.0f;
		float duneCrest = 0.0f;
		float beachWidth = 0.0f;
		float shorelineChangeRate = 0.0f;
		int beachType = -1;

		int randNum = 100;

		m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
		//   m_pDuneLayer->GetData(point, m_colDLBeachType, parcelIndex);
		m_pDuneLayer->GetData(point, m_colDLNourishYearBPS, lastNourishYear);

		int addBPSYear = 0;
		m_pDuneLayer->GetData(point, m_colDLAddYearBPS, addBPSYear);

		if (addBPSYear == 2010)
			lastNourishYear = 2010;

		int yrsSinceNourished = currentYear - lastNourishYear;

		if (beachType == BchT_SANDY_RIPRAP_BACKED)
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
						m_pDuneLayer->GetData(pt, m_colDLDuneToe, duneToe);
						m_pDuneLayer->GetData(pt, m_colDLDuneCrest, duneCrest);

						float bruunSlope;
						m_pDuneLayer->GetData(pt, m_colDLBeachWidth, beachWidth);
						m_pDuneLayer->GetData(pt, m_colDLShorelineChange, shorelineChangeRate);
						m_pDuneLayer->GetData(pt, m_colDLShoreface, bruunSlope);


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
							nourishSLR = ((slr - prevslr) / bruunSlope) * m_nourishFactor;

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
						volume = (0.5 * m_elevCellHeight * duneToe * (newBeachWidth - beachWidth));

						if (volume > 0.0f)
						{
							float cost = float(volume * m_costs.nourishment);

							// get budget infor for Constructing BPS policy
							PolicyInfo& pi = m_policyInfoArray[PI_NOURISH_BY_TYPE];

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
								m_pDuneLayer->GetData(pt, m_colDLNourishFreqBPS, nourishFreq);
								nourishFreq += 1;

								// easting location of MHW 
								m_pDuneLayer->SetData(pt, m_colDLEastingShoreline, m_colDLEastingToe - newBeachWidth);
								// Set new values in Duneline layer

								m_pDuneLayer->SetData(pt, m_colDLBeachWidth, newBeachWidth);
								m_pDuneLayer->SetData(pt, m_colDLNourishFreqBPS, nourishFreq);

								float newBeachSlope = duneToe / newBeachWidth;

								m_pDuneLayer->SetData(pt, m_colDLTranSlope, newBeachSlope);
								m_pDuneLayer->SetData(pt, m_colDLNourishYearBPS, currentYear);
								m_pDuneLayer->SetData(pt, m_colDLNourishVolBPS, volume);

								// Calculate County wide metrics (volume, cost)
								m_nourishVolumeBPS += (float)volume;
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

bool ChronicHazards::FindNourishExtent(MapLayer::Iterator& endPoint)
{
	float buildBPSThreshold = m_idpyFactor * 365.0f;
	bool nourish = false;
	int beachType = -1;

	while (endPoint < m_pDuneLayer->End() && GetBeachType(endPoint) == BchT_SANDY_RIPRAP_BACKED)
	{
		MovingWindow* ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
		float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();

		if (movingAvgIDPY >= buildBPSThreshold)
			nourish = true;

		endPoint++;
	}
	endPoint--;

	return nourish;
} // end FindNourishExtent


bool ChronicHazards::FindImpactExtent(MapLayer::Iterator startPoint, MapLayer::Iterator& endPoint, bool isBPS)
{
	bool nourish = false;

	float buildThreshold = 0.0f;
	int constructYrCol;


	if (isBPS)
	{
		buildThreshold = m_idpyFactor * 365.0f;
		constructYrCol = m_colDLAddYearBPS;
	}
	else // SPS
	{
		buildThreshold = m_idpyFactor * 365.0f;
		constructYrCol = m_colDLAddYearSPS;
	}

	int builtYear = 0;
	m_pDuneLayer->GetData(startPoint, constructYrCol, builtYear);

	int yr = builtYear;

	while (yr > 0 && yr == builtYear)
	{
		MovingWindow* ipdyMovingWindow = m_IDPYArray.GetAt(endPoint);
		float movingAvgIDPY = ipdyMovingWindow->GetAvgValue();

		if (movingAvgIDPY >= buildThreshold)
		{
			nourish = true;
			break;
		}

		//int bldgIndex = -1;
		//m_pDuneLayer->GetData(endPoint, m_colDLDuneBldgIndex, bldgIndex);

		//// Retrieve the Flooding Frequency of the Protected Building
		//if (bldgIndex != -1)
		//{
		//   MovingWindow *floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);
		//   float floodFreq = floodMovingWindow->GetFreqValue();

		//   float floodThreshold = (float)m_floodCountSPS / m_windowLengthFloodHzrd;
		//   
		//   if (floodFreq >= floodThreshold)
		//      nourish = true;            
		//}

		endPoint++;
		m_pDuneLayer->GetData(endPoint, constructYrCol, yr);
	}

	return nourish;

} // end FindImpactExtent

  //bool ChronicHazards::FindNourishExtent(int builtYear, MapLayer::Iterator startPoint, MapLayer::Iterator &endPoint)
  //   {
  //   float buildThreshold = m_idpyFactor * 365.0f;
  //   bool nourish = false;
  //   //   int beachType = -1;
  //
  //   //   m_pDuneLayer->GetData(startPoint, m_colDLBeachType, beachType);
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
  //      //   m_pDuneLayer->GetData(endPoint, m_colDLBeachType, beachType);
  //      m_pDuneLayer->GetData(endPoint, m_colDLAddYearSPS, yr);
  //      }
  //
  //   return nourish;
  //
  //   } // FindNourishExtent

void ChronicHazards::CompareBPSorNourishCosts(int currentYear)
{
	//   float cellHeight = m_pDuneLayer->GetGridCellHeight();

	//float cellHeight = m_pElevationGrid->GetGridCellHeight();

	for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	{
		//For this policy in the less managed scenario, we need to compare the two methods, BPS and nourishment, to see which is cheaper
		float duneToe = 0.0f;
		float beachWidth = 0.0f;
		float shorelineChangeRate = 0.0f;

		m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToe);
		m_pDuneLayer->GetData(point, m_colDLBeachWidth, beachWidth);
		m_pDuneLayer->GetData(point, m_colDLShorelineChange, shorelineChangeRate);

		// cost is determined over a 30 yr timeframe
		float newBeachWidth = beachWidth + (abs(shorelineChangeRate) * 30.0f);

		double volume = (0.5f * m_elevCellHeight * duneToe * (newBeachWidth - beachWidth));
		double nourishmentCost = volume * m_costs.nourishment;

		float newCrest;
		if (m_useMaxTWL == 1)
		{
			m_pDuneLayer->GetData(point, m_colDLYrMaxTWL, newCrest);
			//add safety factor
			newCrest += 1.0;
		}
		/*else
		{
		m_pDuneLayer->GetData(point, m_colYrAvgTWL, newCrest);
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
		double BPScost = BPSHeight * m_costs.BPS * m_elevCellHeight + (BPSHeight * m_costs.BPS * m_elevCellHeight) * 0.02 * 30.0;

		// ??? in this case perhaps should nourish even though not RIPRAP_BACKED
		if (BPScost > nourishmentCost)
			NourishBPS(currentYear, false);
		else
			ConstructBPS(currentYear);
	}
} // end CompareBPSorNourishCost

  // If a new dwelling exists, it must be located on the safest site
void ChronicHazards::ConstructOnSafestSite(EnvContext* pEnvContext, bool inFloodPlain)
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
		m_pIDULayer->GetData(idu, m_colIDUMaxElevation, maxElevation);
		m_pIDULayer->GetData(idu, m_colIDUSafestSiteRow, safestSiteRow);
		m_pIDULayer->GetData(idu, m_colIDUSafestSiteCol, safestSiteCol);

		// assign safest site to centroid location
		Poly* pPoly = m_pIDULayer->GetPolygon(idu);
		Vertex v = pPoly->GetCentroid();
		REAL xSafeSite = v.x;
		REAL ySafeSite = v.y;

		// reassign safest site to highest in IDU
		if (safestSiteRow >= 0 && safestSiteCol >= 0)
			m_pErodedGrid->GetGridCellCenter(safestSiteRow, safestSiteCol, xSafeSite, ySafeSite);

		//   m_pIDULayer->GetData(idu, m_colIDUBaseFloodElevation, baseFloodElevation);

		/*   if (inFloodPlain)
		{
		m_pIDULayer->GetData(idu, m_colFloodZone, floodZone);
		}
		*/
		// Construct new buildings on safest site
		DeltaArray* pDeltaArray = pEnvContext->pDeltaArray;
		for (INT_PTR i = pEnvContext->firstUnseenDelta; i < pDeltaArray->GetCount(); i++)
		{
			DELTA& delta = pDeltaArray->GetAt(i);

			int numNewBldgs = 0;

			if (delta.col == m_colIDUNDU)
				numNewBldgs = delta.newValue.GetInt() - delta.oldValue.GetInt();

			// new buildings must be located on the safest site
			if (numNewBldgs > 0)
			{
				// get Buildings in this IDU
				int numBldgs = m_pIduBuildingLkUp->GetPointsFromPolyIndex(idu, ptArray);

				for (int i = 0; i < numBldgs; i++)
				{
					Poly* pPoly = m_pBldgLayer->GetPolygon(ptArray[i]);

					//   int thisSize = pPoly->m_vertexArray.GetSize();

					pPoly->m_vertexArray[0].x = xSafeSite;
					pPoly->m_vertexArray[0].y = ySafeSite;

					// should I put these in the Building Layer ???
					/*m_pBldgLayer->SetData(ptArray[ i ], m_colBldgMaxElev, maxElevation); */
					//   m_pBldgLayer->SetData(ptArray[ i ], m_colBldgBFE, baseFloodElevation);
					m_pBldgLayer->SetData(ptArray[i], m_colBldgSafeSiteYr, pEnvContext->currentYear);

					//   UpdateIDU(pEnvContext, idu, m_colIDUSafeSiteYear, pEnvContext->currentYear, ADD_DELTA);

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


void ChronicHazards::RaiseOrRelocateBldgToSafestSite(EnvContext* pEnvContext)
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
			m_pIDULayer->GetData(idu, m_colIDUMaxElevation, maxElevation);
			m_pIDULayer->GetData(idu, m_colIDUSafestSiteRow, safestSiteRow);
			m_pIDULayer->GetData(idu, m_colIDUSafestSiteCol, safestSiteCol);

			//// should I put these in the Building Layer ???
			//m_pBldgLayer->GetData(bldg, m_colBldgMaxElev, maxElevation);
			//m_pBldgLayer->GetData(bldg, m_colBldgBFE, baseFloodElevation);
			//m_pBldgLayer->GetData(bldg, m_colBldgSafeSiteYr, pEnvContext->currentYear);

			REAL xSafeSite = 0.0;
			REAL ySafeSite = 0.0;

			if (safestSiteRow >= 0 && safestSiteCol >= 0)
				m_pErodedGrid->GetGridCellCenter(safestSiteRow, safestSiteCol, xSafeSite, ySafeSite);

			m_pIDULayer->GetData(idu, m_colIDUBaseFloodElevation, baseFloodElevation);

			// get Buildings in this IDU
			int numBldgs = m_pIduBuildingLkUp->GetPointsFromPolyIndex(idu, bldgIndices);

			int numRows = m_pErodedGrid->GetRowCount();
			int numCols = m_pErodedGrid->GetColCount();

			for (int i = 0; i < numBldgs; i++)
			{
				int bldgIndex = bldgIndices[i];
				ASSERT(bldgIndex < m_pBldgLayer->GetPolygonCount());

				Poly* pPoly = m_pBldgLayer->GetPolygon(bldgIndex);

				// Set BFE of building

				m_pBldgLayer->SetData(bldgIndex, m_colBldgBFE, baseFloodElevation);

				int safeSiteYr = 0;
				m_pBldgLayer->GetData(bldgIndex, m_colBldgSafeSiteYr, safeSiteYr);

				int raiseToBFEYr = 0;
				m_pBldgLayer->GetData(bldgIndex, m_colBldgBFEYr, raiseToBFEYr);

				// get moving windows and check for flooding
				MovingWindow* floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);
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
					m_pIDULayer->SetData(idu, m_colIDUSafestSiteRow, safestSiteRow);
					m_pIDULayer->SetData(idu, m_colIDUSafestSiteCol, safestSiteCol);
					// uncomment to not allow new construction closer to coast
					// m_pIDULayer->SetData(idu, m_colIDUSafeSite, 0);
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
						m_pFloodedGrid->GetGridCellFromCoord(xCoord, yCoord, row, col);
						float elev = 0.0f;

						if ((row >= 0 && row < numRows) && (col >= 0 && col < numCols))
						{
							m_pFloodedGrid->GetData(row, col, elev);

							// is the adj base flood elevation higher than the DEM elevation at this site?
							// meaning, is this site too low to withstand flooding?
							if ((baseFloodElevation + 0.0f) >= elev)   /// +1????
							{
								float value = 0;
								m_pBldgLayer->GetData(bldgIndex, m_colBldgValue, value);

								float cost = value * m_raiseRelocateSafestSiteRatio;

								// get budget infor for raise/relocate policy
								PolicyInfo& pi = m_policyInfoArray[PI_RAISE_RELOCATE_TO_SAFEST_SITE];
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
									PolicyInfo& pi = m_policyInfoArray[PI_RAISE_RELOCATE_TO_SAFEST_SITE];
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
								PolicyInfo& pi = m_policyInfoArray[PI_RAISE_RELOCATE_TO_SAFEST_SITE];
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
							m_pIDULayer->SetData(idu, m_colIDUPopDensity, 0);
							m_pIDULayer->SetData(idu, m_colIDUSafeSite, 0);

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


void ChronicHazards::RaiseInfrastructure(EnvContext* pEnvContext)
{
	if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
	{
		bool runRaiseInfrastructurePolicy = (m_runRaiseInfrastructurePolicy == 1) ? true : false;

		// Raise Infrastructure to BFE
		for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
		{
			float baseFloodElevation;
			CArray< int, int > infraIndices;
			m_pIDULayer->GetData(idu, m_colIDUBaseFloodElevation, baseFloodElevation);

			// get Infrastructure in this IDU
			int numInfra = m_pIduInfraLkUp->GetPointsFromPolyIndex(idu, infraIndices);

			for (int i = 0; i < numInfra; i++)
			{
				int critical = 0;
				Poly* pPoly = m_pInfraLayer->GetPolygon(infraIndices[i]);
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
							MovingWindow* floodMovingWindow = m_floodInfraFreqArray.GetAt(infraIndex);
							float floodFreq = floodMovingWindow->GetFreqValue();

							if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
							{
								if (floodFreq >= (float(m_bfeCount) / (float)m_windowLengthFloodHzrd))
								{
									float value = 0;
									m_pInfraLayer->GetData(infraIndex, m_colInfraValue, value);

									float cost = value * m_raiseRelocateSafestSiteRatio;

									// get budget infor for Constructing BPS policy
									PolicyInfo& pi = m_policyInfoArray[PI_RAISE_INFRASTRUCTURE];
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
						MovingWindow* floodMovingWindow = m_floodInfraFreqArray.GetAt(infraIndex);
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


void ChronicHazards::RemoveBldgFromHazardZone(EnvContext* pEnvContext)
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
		m_pIDULayer->GetData(idu, m_colIDUNDU, ndu);
		int newDu = 0;
		m_pIDULayer->GetData(idu, m_colIDUNEWDU, newDu);

		// get Buildings in this IDU
		int numBldgs = m_pIduBuildingLkUp->GetPointsFromPolyIndex(idu, bldgIndices);

		for (int i = 0; i < numBldgs; i++)
		{
			int yr = 0;

			Poly* pPoly = m_pBldgLayer->GetPolygon(bldgIndices[i]);
			int bldgIndex = bldgIndices[i];

			// get moving windows and check for flooding
			MovingWindow* floodMovingWindow = m_floodBldgFreqArray.GetAt(bldgIndex);
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
					PolicyInfo& pi = m_policyInfoArray[PI_REMOVE_BLDG_FROM_HAZARD_ZONE];
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
						m_pIDULayer->SetData(idu, m_colIDURemoveBldgYr, pEnvContext->currentYear);
						m_pIDULayer->SetData(idu, m_colIDUPopDensity, 0);
						m_pIDULayer->SetData(idu, m_colIDUSafeSite, 0);
						if (ndu > 0)
						{
							m_pIDULayer->SetData(idu, m_colIDUNDU, --ndu);
						}

						if (newDu > 0)
						{
							m_pIDULayer->SetData(idu, m_colIDUNEWDU, --newDu);

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


void ChronicHazards::RemoveInfraFromHazardZone(EnvContext* pEnvContext)
{
	// Relocate existing homes/buildings from hazard zone (100-yr flood plain)
	for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
	{
		CArray< int, int > infraIndices;
		// see if infrastructure is impacted by flooding
		float baseFloodElevation = 0.0f;
		m_pIDULayer->GetData(idu, m_colIDUBaseFloodElevation, baseFloodElevation);
		int numInfra = m_pIduInfraLkUp->GetPointsFromPolyIndex(idu, infraIndices);

		for (int i = 0; i < numInfra; i++)
		{
			int critical = 0;
			Poly* pPoly = m_pInfraLayer->GetPolygon(infraIndices[i]);
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
					MovingWindow* floodMovingWindow = m_floodInfraFreqArray.GetAt(infraIndex);
					float floodFreq = floodMovingWindow->GetFreqValue();

					if (pEnvContext->currentYear >= (pEnvContext->startYear + m_windowLengthFloodHzrd - 1))
					{
						if (floodFreq >= float(1.0f / m_windowLengthFloodHzrd))
						{
							float value = 0;
							m_pInfraLayer->GetData(infraIndex, m_colInfraValue, value);

							float cost = value * m_removeFromHazardZoneRatio;

							// get budget infor for Constructing BPS policy
							PolicyInfo& pi = m_policyInfoArray[PI_REMOVE_INFRA_FROM_HAZARD_ZONE];
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

	m_pIDULayer->SetData(idu, m_colIDUPopDensity, 0);
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
	TiXmlElement* pXmlRoot = doc.RootElement();  // <chronic_hazards>
	if (pXmlRoot == nullptr)
		return false;


	int twl = 0, flooding = 0, erosion = 0, buildings = 0, infra = 0, policy = 0;
	XML_ATTR attrs[] = {
		// attr                  type          address              isReq  
		{ "run_twl",               TYPE_INT,   &twl,                 true, 0 },
		{ "run_flooding",          TYPE_INT,   &flooding,            true, 0 },
		{ "run_erosion",           TYPE_INT,   &erosion,             true, 0 },
		{ "run_buildings",         TYPE_INT,   &buildings,           true, 0 },
		{ "run_infrastructure",    TYPE_INT,   &infra,               true, 0 },
		{ "run_policy",            TYPE_INT,   &policy,              true, 0 },
		{ "export_map_interval",   TYPE_INT,   &m_exportMapInterval, true, 0 },
		{ nullptr,                 TYPE_NULL,  nullptr,              false,   0 } };

	if (TiXmlGetAttributes(pXmlRoot, attrs, filename) == false)
	{
		CString msg;
		msg.Format(_T("ChronicHazards: Misformed <chronic_hazards> element while reading in %s"), filename);
		Report::ErrorMsg(msg);
		return false;
	}

	m_runFlags = 0;
	if (twl > 0)
		m_runFlags += CH_MODEL_TWL;
	if (flooding > 0)
		m_runFlags += CH_MODEL_FLOODING;
	if (erosion > 0)
		m_runFlags += CH_MODEL_EROSION;
	if (buildings > 0)
		m_runFlags += CH_MODEL_BUILDINGS;
	if (infra > 0)
		m_runFlags += CH_MODEL_INFRASTRUCTURE;
	if (policy > 0)
		m_runFlags += CH_MODEL_POLICY;


	// envx file directory, slash-terminated
	CString projDir = PathManager::GetPath(PM_PROJECT_DIR);
	CString iduDir = PathManager::GetPath(PM_IDU_DIR);

	TiXmlElement* pXmlTWL = pXmlRoot->FirstChildElement("twl");
	if (pXmlTWL == nullptr)
		return false;

	//  LPTSTR period = nullptr;
	LPTSTR twlInputDir = nullptr;
	LPTSTR city = nullptr;

	XML_ATTR twlAttrs[] = {
		// attr                  type          address                           isReq  
		// 
		//{ "debug",                 TYPE_INT,     &m_debug,                        true,    0 },
		{ "transect_points",       TYPE_CSTRING, &m_transectLayer,                true,    0 },
		{ "write_daily_buoy_data", TYPE_INT,     &m_writeDailyBouyData,           true,    0 },
		{ "find_safe_site",        TYPE_INT,     &m_findSafeSiteCell,             true,    0 },
		{ "twl_input_dir",         TYPE_STRING,  &twlInputDir,                    true,    0 },
		{ "simulation_count",      TYPE_INT,     &m_simulationCount,              true,    0 },
		{ "inlet_factor",          TYPE_FLOAT,   &m_inletFactor,                  true,    0 },
		{ "twl_period",            TYPE_INT,     &m_windowLengthTWL,              true,    0 },
		{ "flood_hazard_period",   TYPE_INT,     &m_windowLengthFloodHzrd,        true,    0 },
		{ "bfe_count",             TYPE_INT,     &m_bfeCount,                     true,    0 },
		{ "safe_site_count",       TYPE_INT,     &m_ssiteCount,                   true,    0 },
		//{ "maxFloodArea",          TYPE_FLOAT,   &m_maxArea,                      true,    0 },
		{ "shoreline_length",      TYPE_FLOAT,   &m_shorelineLength,              true,    0 },
		//{ "runSpatialBayTWL",      TYPE_INT,     &m_runSpatialBayTWL,             true,    0 },
		{ nullptr,                    TYPE_NULL,    nullptr,                            false,   0 } };

	if (TiXmlGetAttributes(pXmlTWL, twlAttrs, filename) == false)
	{
		CString msg;
		msg.Format(_T("ChronicHazards: Misformed <twl> element while reading in %s"), filename);
		Report::ErrorMsg(msg);
		return false;
	}

	// add twl directories to path
	CString twlInputPath = iduDir + twlInputDir;
	PathManager::AddPath(twlInputPath);
	m_twlInputPath = twlInputPath + "\\";

	//--------- erosion -------//
	TiXmlElement* pXmlErosion = pXmlRoot->FirstChildElement("erosion");
	if (pXmlErosion != nullptr)
	{
		LPTSTR erosionInputDir = nullptr;

		bool use = false;
		XML_ATTR eroAttrs[] = {
			// attr                  type          address                           isReq  
			//{ "use",                   TYPE_BOOL,    &use,                            true,    0 },
			{ "erosion_input_dir",     TYPE_STRING,  &erosionInputDir,                true,    0 },
			{ "bathymetry_lookup",     TYPE_CSTRING, &m_bathyLookupFile,              true,    0 },
			{ "bathymetry_mask",       TYPE_CSTRING, &m_bathyMask,                    true,    0 },
			{ "min_slope_thresh",      TYPE_FLOAT,   &m_slopeThresh,                  true,    0 },
			{ "erosion_count",         TYPE_INT,     &m_eroCount,                     true,    0 },
			{ "erosion_hazard_window", TYPE_INT,     &m_windowLengthEroHzrd,          true,    0 },
			{ "access_thresh",         TYPE_FLOAT,   &m_accessThresh,                 true,    0 },
			{ "constTD",               TYPE_FLOAT,   &m_constTD,                      true,    0 },
			{ nullptr,                    TYPE_NULL,    nullptr,                            false,   0 } };

		if (TiXmlGetAttributes(pXmlErosion, eroAttrs, filename) == false)
		{
			CString msg;
			msg.Format(_T("ChronicHazards: Misformed <erosion> element while reading in %s"), filename);
			Report::ErrorMsg(msg);
			return false;
		}

		CString erosionInputPath = iduDir + erosionInputDir;
		PathManager::AddPath(erosionInputPath);
		m_erosionInputPath = erosionInputPath + "\\";

		//if (use)
		//   m_runFlags += CH_MODEL_EROSION;
	}



	//--------- flooding -------//
	TiXmlElement* pXmlFlooding = pXmlRoot->FirstChildElement("flooding");
	if (pXmlFlooding != nullptr)
	{
		LPTSTR sfincsHome = nullptr, floodDir = nullptr;

		XML_ATTR floodAttrs[] = {
			// attr             type          address         isReq  
			//{ "use",            TYPE_BOOL,    &use,          true,    0 },
			{ "sfincs_home",    TYPE_STRING,  &sfincsHome,   true,    0 },
			//{ "flood_dir",      TYPE_STRING,  &floodDir,     true,    0 },
			{ nullptr,          TYPE_NULL,    nullptr,       false,   0 } };

		if (TiXmlGetAttributes(pXmlFlooding, floodAttrs, filename) == false)
		{
			CString msg;
			msg.Format(_T("ChronicHazards: Misformed <flooding> element while reading in %s"), filename);
			Report::ErrorMsg(msg);
			return false;
		}

		this->m_sfincsHome = sfincsHome;
		this->m_floodDir = floodDir;
		//if (use)
		//   m_runFlags += CH_MODEL_FLOODING;
	}

	TiXmlElement* pXmlBatesPolicy = pXmlRoot->FirstChildElement("Bates");
	if (pXmlBatesPolicy == nullptr)
		return false;

	XML_ATTR batesAttrs[] = {
		// attr               type           address                  isReq    checkCol
		{ "city",            TYPE_STRING,  &city,                          false,   0 },
		//{ "runBayFloodModel",   TYPE_INT,     &m_runBayFloodModel,         true,    0 },
		{ "runBatesFlood",      TYPE_INT,     &m_runBatesFlood,            true,    0 },
		{ "BatesTimeStep",      TYPE_FLOAT,   &m_floodTimestep,                 true,    0 },
		{ "BatesTimeDuration",  TYPE_FLOAT,   &m_batesFloodDuration,       true,    0 },
		{ "ManningCoefficient", TYPE_FLOAT,   &m_ManningCoeff,             true,    0 },
		{ nullptr,                 TYPE_NULL,    nullptr,                        false,   0 } };

	if (TiXmlGetAttributes(pXmlBatesPolicy, batesAttrs, filename) == false)
		return false;

	m_cityDir = city;
	// add "PolyGridLookups" directory to the Envision search paths
	CString pglPath;
	if (m_cityDir.IsEmpty())
		pglPath = iduDir + _T("PolyGridMaps");
	else
		pglPath = iduDir + m_cityDir + "\\" + _T("PolyGridMaps");
	PathManager::AddPath(pglPath);

	TiXmlElement* pXmlBPSPolicy = pXmlRoot->FirstChildElement("bps");
	if (pXmlBPSPolicy == nullptr)
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
		{ nullptr,               TYPE_NULL,    nullptr,                            false,   0 } };

	if (TiXmlGetAttributes(pXmlBPSPolicy, bpsAttrs, filename) == false)
		return false;

	TiXmlElement* pXmlNourishPolicy = pXmlRoot->FirstChildElement("nourish");
	if (pXmlNourishPolicy == nullptr)
		return false;

	XML_ATTR nourishAttrs[] = { // attr            type           address                          isReq    checkCol
	   { "NourishmentFreq",      TYPE_INT,     &m_nourishFreq,                  true,    0 },
	   { "NourishmentFactor",      TYPE_INT,     &m_nourishFactor,                true,    0 },
	   { "NourishmentPercentage",   TYPE_INT,     &m_nourishPercentage,            true,    0 },
	   { nullptr,               TYPE_NULL,    nullptr,                            false,   0 } };

	if (TiXmlGetAttributes(pXmlNourishPolicy, nourishAttrs, filename) == false)
		return false;

	TiXmlElement* pXmlSPSPolicy = pXmlRoot->FirstChildElement("sps");
	if (pXmlSPSPolicy == nullptr)
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
		{ nullptr,               TYPE_NULL,       nullptr,                  false,   0 } };

	if (TiXmlGetAttributes(pXmlSPSPolicy, SPSAttrs, filename) == false)
		return false;

	TiXmlElement* pXmlTWLcosts = pXmlRoot->FirstChildElement("costs");
	if (pXmlTWLcosts == nullptr)
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
		{ nullptr,              TYPE_NULL,     nullptr,                            false,   0 } };

	if (TiXmlGetAttributes(pXmlTWLcosts, costAttrs, filename) == false)
		return false;

	/*
	if (m_runFlags & CH_MODEL_FLOODING)
	   {
	   // flooded areas
	   TiXmlElement* pXmlFloodAreas = pXmlRoot->FirstChildElement("flood_areas");
	   if (pXmlFloodAreas != nullptr)
		  {
		  // take care of any constant expressions
		  TiXmlElement* pXmlFloodArea = pXmlFloodAreas->FirstChildElement(_T("flood_area"));
		  while (pXmlFloodArea != nullptr)
			 {
			 FloodArea* pFloodArea = FloodArea::LoadXml(pXmlFloodArea, filename, m_pMap);

			 if (pFloodArea != nullptr)
				{
				m_floodAreaArray.Add(pFloodArea);

				CString msg("FloodArea: Loaded Flood Grid for layer ");
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

	m_floodTimestep = 1;

	return true;

} // end LoadXml


double ChronicHazards::Maximum(double* dat, int length)
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

double ChronicHazards::Minimum(double* dat, int length)

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

void ChronicHazards::Mminvinterp(double* x, double* y, double*& xo, int length)
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
			xo[ii] = ((yo - y[ii]) / (y[ii + 1] - y[ii])) * (x[ii + 1] - x[ii]) + x[ii];
		}

		if (y[ii] == 0)
			xo[ii] = x[ii];
	}
}

// For programming purposes, approximation of kh^2  
// Ref : Hunt, J.N., 1979. Direct solution of wave dispersion equation. J.
// Waterw., Port, CoastalOcean Dlv., Am.Soc.Civ.Eng. 105(4),
// 457 - 459.
double ChronicHazards::CalculateCelerity2(float waterLevel, float wavePeriod, double& kh)
{
	double celerity;

	double dCoeff[6] = { 0.6, 0.35, 0.1608465608, 0.0632098765, 0.0217540484, 0.0065407983 };

	double sigma = (2 * PI) / wavePeriod;

	double x = (sigma * waterLevel) / sqrt(G * waterLevel);
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


float ChronicHazards::CalculateCelerity(float waterLevel, float wavePeriod, float& n)
{
	// The calculation below solves the dispersion relation for water waves. 
	// The Newton-Rhapson method is used to solve the equation.

	// Radian peak frequency (sig)
	float sig = (2.0f * PI) / wavePeriod;

	// Deep water wave number calculations
	// Determine x
	float sigsq = sig * sig;
	float kdeep = sigsq / G;
	float kh_0 = kdeep * waterLevel;
	float x = kdeep * waterLevel * pow(1.0f / tanh(pow(kdeep * waterLevel, 0.75f)), (2.0f / 3.0f));

	float deltax = 10.0f;
	int iterations = 0;
	while ((abs(deltax / x)) > pow(10.0f, -5.0f)) //
	{
		if (iterations > 100)
			break;

		float Fx = sigsq * waterLevel / G - x * tanh(x);
		float dFdx = x * tanh(x) * tanh(x) - tanh(x) - x;
		deltax = -Fx / dFdx;
		x += deltax;
		iterations++;
	}

	/* if (iterations > maxIterations)
	maxIterations = iterations;*/

	// Compute the wave number
	float k = x / waterLevel;
	n = (1.0f / 2.0f) * (1.0f + 2.0f * k * waterLevel / sinh(2.0f * k * waterLevel));       /* Cg/c */

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
	 //////////   int timeStep = (int)(m_floodTimestep * 1000.0f);
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
	 //////////                  m_pWaterElevationGrid->SetData(row, col, waterElevation + (discharge * m_floodTimestep));
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
	 //////////   floodedArea = floodedCount * (m_elevCellWidth * m_elevCellHeight);  // in sq meters
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
	 //////////                        /////float csa = flowDepth * m_elevCellWidth;
	 //////////                        /////
	 //////////                        /////// hydraulic radius (flow area/wetted perimeter)
	 //////////                        /////float r = csa / (m_cellWidth + (2 * flowDepth));
	 //////////                        /////
	 //////////                        /////// water surface slope (positive if slope toward this, negative if sloping away from this)
	 //////////                        /////float slope = (neighborWaterElevation - thisWaterElevation ) / m_elevCellWidth;
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
	 //////////                        double Q = (pow(flowDepth, (5.0 / 3.0)) / manningCoeff)*sqrt(fabs(deltaH / m_elevCellWidth))*m_elevCellWidth;
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
	 //////////                        //   double slopeSign = sqrt(abs(waterElevationDiff) / m_cellWidth) * m_elevCellHeight;
	 //////////                        //   if (waterElevationDiff < 0)
	 //////////                        //      slopeSign = -slopeSign;
	 //////////                        //   //   flux += (float)((pow(flowDepth, power) / m_ManningCoeff) * slopeSign);
	 //////////                        //   flux += (float)((pow(flowDepth, power) / manningCoeff) * slopeSign);
	 //////////                        //   }
	 //////////                        //// volumetric flow in y direction (North & South)
	 //////////                        //if (direction % 2 == 0)
	 //////////                        //   {
	 //////////                        //   // determine if water is moving in or out of the cell by assigning appropriate sign
	 //////////                        //   double slopeSign = sqrt(abs(waterElevationDiff) / m_elevCellHeight) * m_elevCellWidth;
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
	 //////////            flux /= (m_cellWidth * m_elevCellHeight);
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
	 //////////   if (m_pNewElevationGrid == nullptr)
	 //////////      {
	 //////////      m_pNewElevationGrid = m_pMap->CloneLayer(*m_pElevationGrid);
	 //////////      m_pNewElevationGrid->m_name = "New Elevation Grid";
	 //////////      }
	 //////////
	 //////////   if (m_pWaterElevationGrid == nullptr)
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
	 //////////   if (m_pDischargeGrid == nullptr)
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
	 //////////   MapLayer::Iterator endPoint = m_pDuneLayer->Begin();
	 //////////
	 //////////   // initialize boundary conditions for water free surface (water height) grid
	 //////////   // at time t = 0
	 //////////   for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	 //////////      {
	 //////////      endPoint = point;
	 //////////
	 //////////      int beachType = -1;
	 //////////      m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);
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
	 //////////         m_pDuneLayer->GetData(point, m_colDLYrMaxTWL, waterHeight);
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
	 //////////         /* m_pDuneLayer->GetData(point, m_colDLEastingCrest, xCoord);
	 //////////          m_pDuneLayer->GetData(point, m_colNorthingCrest, yCoord);*/
	 //////////         m_pDuneLayer->GetPointCoords(point, xDuneToe, yCoord);
	 //////////         m_pDuneLayer->GetData(point, m_colDLEastingCrest, xDuneCrest);
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
	 //////////               m_pDuneLayer->GetData(point, m_colDLYrMaxTWL, waterHeight);
	 //////////
	 //////////               //waterHeight = 9.0f;
	 //////////               // get the duneCrest height for this dunePoint
	 //////////               float duneCrest = 0.0f;
	 //////////               m_pDuneLayer->GetData(point, m_colDLDuneCrest, duneCrest);
	 //////////
	 //////////               float duneToe = 0.0f;
	 //////////               m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToe);
	 //////////
	 //////////               float prevDuneCrest = 0.0f;
	 //////////               m_pNewElevationGrid->SetData(startRow, startCol, duneCrest);
	 //////////               m_pNewElevationGrid->SetData(startRow, duneToeCol, duneToe);
	 //////////               m_pWaterElevationGrid->SetData(startRow, startCol, waterHeight);
	 //////////
	 //////////               // shift DEM west of DuneCrest
	 //////////               /***************/
	 //////////               m_pDuneLayer->GetData(point, m_colPrevCol, prevDunePtCol);
	 //////////
	 //////////               float elevation = 0.0f;
	 //////////               m_pNewElevationGrid->GetData(startRow, startCol + 1, elevation);
	 //////////               if (waterHeight > elevation)
	 //////////                  count++;
	 //////////
	 //////////               m_pDischargeGrid->SetData(startRow, startCol, -9999.0f);
	 //////////
	 //////////               /*float shorefaceSlope = 0.0f;
	 //////////               m_pDuneLayer->GetData(point, m_colDLShoreface, shorefaceSlope);*/
	 //////////
	 //////////               endPoint++;
	 //////////               if (endPoint < m_pDuneLayer->End())
	 //////////                  {
	 //////////                  int nextRow = -1;
	 //////////                  int nextCol = -1;
	 //////////                  m_pDuneLayer->GetPointCoords(endPoint, xCoord, yCoord);
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
	if (pFloodArea->m_pElevationGrid == nullptr)
	   return false;

	if (pFloodArea->m_pCellTypeGrid == nullptr)
	   return false;

	if (pFloodArea->m_pWaterSurfaceElevationGrid == nullptr)
	   return false;

	if (pFloodArea->m_pDischargeGrid == nullptr)
	   return false;

	if (pFloodArea->m_pWaterDepthGrid == nullptr)
	   return false;

	///// Step 1: set the FloodArea's water elevation grid to zero initially
	pFloodArea->m_pWaterSurfaceElevationGrid->SetAllData(0, false);

	///// Step 2: Set boundary TWLs if necessary

	// initialize prev dune pt rows and columns
	int prevDunePtRow = -1;
	int prevDunePtCol = -1;
	int count = 0;
	float elevDelta = 0;
	MapLayer::Iterator endPoint = m_pDuneLayer->Begin();

	pFloodArea->m_pCellTypeGrid->SetAllData(CT_MODEL, false);

	// iterate through dune points, setting associated TWLs in the FloodArea
	for (MapLayer::Iterator dunePtIndex = m_pDuneLayer->Begin(); dunePtIndex < m_pDuneLayer->End(); dunePtIndex++)
	   {
	   endPoint = dunePtIndex;

	   int beachType = -1;
	   m_pDuneLayer->GetData(dunePtIndex, m_colDLBeachType, beachType);

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
		  //m_pDuneLayer->GetData(dunePtIndex, m_colDLYrMaxTWL, waterHeight);
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
		  m_pDuneLayer->GetPointCoords(dunePtIndex, xDuneToe, yDune);

		  // get the dune crest X location
		  m_pDuneLayer->GetData(dunePtIndex, m_colDLEastingCrest, xDuneCrest);

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
		  m_pDuneLayer->GetData(dunePtIndex, m_colDLYrMaxTWL, waterHeight);

		  float duneHeight = 0.0f;
		  m_pDuneLayer->GetData(dunePtIndex, m_colDLDuneCrest, duneHeight);

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
	msg.Format("FloodArea: %i fixed-height sources were identified for %s, with an average delta of %.2fm.", count, (LPCTSTR)pFloodArea->m_siteName, elevDelta);
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
	if (m_pEelgrassGrid == nullptr)
	   {
	   m_pEelgrassGrid = m_pMap->CloneLayer(*m_pBayBathyGrid);
	   m_pEelgrassGrid->m_name = "Eelgrass Grid";
	   }
	m_pEelgrassGrid->SetAllData(0.0f, false);

	// area = grid cell count multiplied by area of grid cell
	//double eelgrassCount = 0.0;

	float yearlyAvgLowSWL = 0.0f;
	float yearlyAvgSWL = 0.0f;

	MapLayer::Iterator point = m_pDuneLayer->Begin();

	m_pDuneLayer->GetData(point, m_colYrAvgSWL, yearlyAvgSWL);
	m_pDuneLayer->GetData(point, m_colYrAvgLowSWL, yearlyAvgLowSWL);

	//for (MapLayer::Iterator point = m_pDuneLayer->Begin(); point < m_pDuneLayer->End(); point++)
	//{
	//   int beachType = 0;
	//   m_pDuneLayer->GetData(point, m_colDLBeachType, beachType);

	//   int duneIndex = -1;
	//   m_pDuneLayer->GetData(point, m_colDLDuneIndex, duneIndex);

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
	//m_pDuneLayer->GetData(point, m_colYrAvgLowSWL, yearlyAvgLowSWL);
	//m_pDuneLayer->GetData(point, m_colYrAvgSWL, yearlyAvgSWL);

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



	// Determines Yearly Maximum TWL based upon daily TWL calculations at each Dune point
void ChronicHazards::CalculateTWLandImpactDaysAtShorePoints(EnvContext* pEnvContext)
{
	if (m_pDuneLayer != nullptr)
		m_pDuneLayer->m_readOnly = false;

	// iterate through each day of the year, getting 
	Report::Log_i("Calculating Impact Days for year %i", pEnvContext->currentYear);

	// iterate through each day of the year
	CString msg;
	for (int doy = 0; doy < 365; doy++)
	{
		if (doy % 5 == 0)
		{
			msg.Format("Calculating impacts days for Day %i for %i shoreline points", doy, m_pDuneLayer->GetRecordCount());
			Report::StatusMsg(msg);
		}
		int row = (pEnvContext->yearOfRun * 365) + doy;

		// Get the Still Water Level for that day (in NAVD88 vertical datum) from the deep water time series
		float swl = 0.0f;
		m_buoyObsData.Get(BUOY_OBSERVATION_DATA::WATER_LEVEL_WL, row, swl);

		// Get the low Still Water Level for that day ( in NVAVD88 vertical datum ) from the deep water time series
		float lowSWL = 0.0f;
		// TODO: FIX!!! (this column doesn't exist) m_buoyObsData.Get(BUOY_OBSERVATION_DATA::WATER_LEVEL_LOW, row, lowSWL);

		// tide range for that day based upon still water level
		float wlratio = (swl - (-1.5f)) / (4.5f + 1.5f);

		int   idusProcessed = 0;
		float maxTWLforPeriod = 0;
		float maxErosionforPeriod = 0;

		float meanPeakHeight = 0;
		float meanPeakPeriod = 0;
		float meanPeakDirection = 0;
		float meanPeakWaterLevel = 0;

		// for this day, iterate though Dune toe points, calculating TWL and associated variables as we go.
		//#pragma omp parallel for
		for (MapLayer::Iterator dunePt = m_pDuneLayer->Begin(); dunePt < m_pDuneLayer->End(); dunePt++)
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
			int duneIndex = -1;
			int profileID = -1;
			int transID = -1;

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
			float EPR = 0.0f;

			float dailyTWL = 0.0f;
			float dailyTWL_Flood = 0.0f;
			float L = 0.0f;
			float shorelineAngle = 0.0f;
			double bruunSlope = 0.0f;

			double eastingCrest = 0.0;
			double eastingToe = 0.0;

			//int profileIndex = m_profileMap[profileID];
			m_pDuneLayer->GetData(dunePt, m_colDLDuneIndex, duneIndex);
			m_pDuneLayer->GetData(dunePt, m_colDLTransID, transID);
			m_pDuneLayer->GetData(dunePt, m_colDLProfileID, profileID);

			int transIndex = m_transectMap[transID];
			int profileIndex = m_profileMap[profileID];

			m_pDuneLayer->GetData(dunePt, m_colDLDuneToe, duneToe);
			//m_pDuneLayer->GetData(dunePt, m_colGammaBerm, gammaBerm);
			//m_pDuneLayer->GetData(dunePt, m_colScomp, S_comp);
			m_pDuneLayer->GetData(dunePt, m_colDLGammaRough, gammaRough);
			m_pDuneLayer->GetData(dunePt, m_colDLTranSlope, tanb1);
			//    m_pDuneLayer->GetData(dunePt, m_colPrevSlope, prevTanb1);
			m_pDuneLayer->GetData(dunePt, m_colDLBeachType, beachType);
			//m_pDuneLayer->GetData(dunePt, m_colOrigBeachType, origBeachType);
			m_pDuneLayer->GetData(dunePt, m_colDLDuneCrest, duneCrest);
			//m_pDuneLayer->GetData(dunePt, m_colDLEastingCrestProfile, eastingCrest);
			m_pDuneLayer->GetData(dunePt, m_colDLEastingToe, eastingToe);
			//m_pDuneLayer->GetData(dunePt, m_colPrevDuneCr, prevDuneCrest);
			//     m_pDuneLayer->GetData(dunePt, m_colEPR, EPR);
			m_pDuneLayer->GetData(dunePt, m_colDLShorelineAngle, shorelineAngle);
			m_pDuneLayer->GetData(dunePt, m_colDLBeachWidth, beachwidth);
			m_pDuneLayer->GetData(dunePt, m_colDLBruunSlope, bruunSlope);

			// get wave conditions at intermediate depth, 20m contour from the RBF interpolations
			float radPeakHeightL = 0.0, radPeakHeightH = 0.0f, radPeakPeriodL = 0.0f, radPeakPeriodH = 0.0f;
			float radPeakDirectionL = 0.0f, radPeakDirectionH = 0.0f, radPeakWaterDepthL = 0.0f, radPeakWaterDepthH = 0.0f;

			FDataObj* pRBFOutputData = m_dailyRBFOutputArray.GetAt(transIndex);   // zero-based
			pRBFOutputData->Get(0, row, radPeakHeightL);
			pRBFOutputData->Get(1, row, radPeakHeightH);
			pRBFOutputData->Get(2, row, radPeakPeriodL);
			pRBFOutputData->Get(3, row, radPeakPeriodH);
			pRBFOutputData->Get(4, row, radPeakDirectionL);
			pRBFOutputData->Get(5, row, radPeakDirectionH);
			pRBFOutputData->Get(6, row, radPeakWaterDepthL);
			pRBFOutputData->Get(7, row, radPeakWaterDepthH);

			float waveHeight = radPeakHeightL + wlratio * (radPeakHeightH - radPeakHeightL);
			float wavePeriod = radPeakPeriodL + wlratio * (radPeakPeriodH - radPeakPeriodL);
			float waveDirection = radPeakDirectionL + wlratio * (radPeakDirectionH - radPeakDirectionL);
			float waterDepth = radPeakWaterDepthL + wlratio * (radPeakWaterDepthH - radPeakWaterDepthL);

			// Linearly back shoal transformed waves from 20m/30m contour to deep water
			float L0 = 0;     // Deepwater wave length (m)
			float setup = 0;  // wave setup height
			float infragravitySwash = 0;
			float incidentSwash = 0;
			float r2Runup = 0;    // wave induced runup
			float stockdonTWL = 0;
			ComputeStockdon(swl, waveHeight, wavePeriod, waveDirection, &L0, &setup, &incidentSwash, &infragravitySwash, &r2Runup, &stockdonTWL);

			// account for linear shoaling process to get equivalent deepwater conditions
			// Deepwater celerity 
			float Co = L0 / wavePeriod;
			float n = 0.0f;
			float C = CalculateCelerity(waterDepth, wavePeriod, n);

			double ks = (sqrt(1.0f / (2.0f * n) * Co / C));
			float Hdeep = waveHeight / (sqrt(1.0f / (2.0f * n) * Co / C));

			//double structureR2 = 0.0;

			// Determine the Dynamic Water Level 2%
			// note : dailyTWL - dunetoe1
			//float DWL2 = swl + 1.1f * (eta + STK_IG / 2.0f) - duneToe;
			float DWL2 = stockdonTWL - duneToe;   /// ???? Check with Peter

			// significant wave height at structure toe calculated using a breaker index of 0.78
			float Hmo = DWL2 * 0.78f;

			// If the depth limited breaker height is larger than the offshore conditions, then the latter will be used.
			if (Hmo > Hdeep)
				Hmo = Hdeep;

			// Irribarren number
			/*   float IbarrenNumber = localBarrierSlope / (sqrt(Hmo / L));
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
			m_pDuneLayer->GetData(dunePt, m_colDLEastingCrest, eastingDC);
			m_pDuneLayer->GetData(dunePt, m_colNorthingCrest, northingDC);
			m_pElevationGrid->GetGridCellFromCoord(eastingDC, northingDC, rowDC, colDC);
			m_pElevationGrid->GetData(rowDC, colDC, duneCrest);*/

			if ((swl + 1.1f * setup) >= duneCrest)  // ignores swash, stillwater + setup only
				isOvertopped = true;
			else
				isOvertopped = false;

			// if beach type is inlet,  twl = still water level + proportion of the deep water wave height (SWH)
			if (beachType == BchT_BAY)
			{
				dailyTWL = swl + (m_inletFactor * Hdeep);
				duneCrest = (float)MHW;
			}
			else if (beachType == BchT_SANDY_DUNE_BACKED ||
				beachType == BchT_SANDY_BLUFF_CLIFF_BACKED ||
				beachType == BchT_SANDY_BURIED_RIPRAP_BACKED) // || Hmo < 0)
			{
				// Limit flooding to swl + setup for dune backed beaches (no swash)
				dailyTWL_Flood = swl + 1.1f * (setup + infragravitySwash / 2.0f);   // TWL-incident band swash
				dailyTWL = stockdonTWL;
			}

			// Use Local/TAW Combination to calculate TWL for all other defined beachtypes
			else if (beachType == BchT_SANDY_RIPRAP_BACKED ||
				beachType == BchT_MIXED_SEDIMENT_DUNE_BACKED ||
				beachType == BchT_MIXED_SEDIMENT_BLUFF_BACKED ||
				beachType == BchT_SANDY_COBBLEGRAVEL_BLUFF_BACKED ||
				beachType == BchT_SANDY_COBBLEGRAVEL_BERM_BACKED ||
				beachType == BchT_SANDY_WOODY_DEBRIS_BACKED ||
				beachType == BchT_JETTY ||
				beachType == BchT_ROCKY_CLIFF_HEADLAND)
			{
				// Use Local/TAW Combination to calculate TWL (insted of stockdon) for all other defined beachtypes 
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
				//    m_pDuneLayer->GetData(dunePt, m_colBPSMaintYear, maintYear);
				//    int maintFreqInterval = pEnvContext->currentYear - maintYear;
				firstMaintYear = pEnvContext->startYear + m_maintFreqBPS;

				// structure (barrier/bluff) is not flooded
				if (!isOvertopped)
				{
					//    float y1temp = 0.0f;

					// STK_TWL = swl + StockdonR2
					float y1temp = 0.0f;
					double yTop = stockdonTWL;

					// If STK_TWL overtops the structue then use the top of the structure 
					// (minus 1cm for interpolation purposes, see curveintersect)
					if (yTop > duneCrest)
						yTop = duneCrest - 0.01;

					// The bottom part of the slope is given by Static Setup (STK) + swl
					double yBottom = swl + 1.1f * setup;

					// Also need new slope when dune erodes and backshore(foredusne) beach slope changes
					// Slope not changing for bluff backed beaches
					//         if ( (profileID != prevProfileIndex || duneCrest != prevDuneCrest || tanb1 != prevTanb1 ) && (beachType != BT_RIPRAP_BACKED || maintYear < 2011))// Slope not changing for bluff backed
					//        if ( ( tanb1 != prevTanb1 ) && (beachType != BchT_RIPRAP_BACKED )) // || maintYear < 2011))// Slope not changing for bluff backed
					//    {

					// calculate slope of swash where bluff or barrier exists
					// this is needed to determine the wave runup on a barrier
					// do not use cross shore profile when the dune is dynamically changing (e.g. RIP RAP built or maintained)

					// profileID != prevProfileIndex eq to statement TnumRef != prevTNummRef

					if (beachType != BchT_SANDY_RIPRAP_BACKED || pEnvContext->currentYear < firstMaintYear) //(maintFreq >= m_maintFreqBPS))
					{
						DDataObj* pBATH = m_BathyDataArray.GetAt(profileIndex);   // zero-based

						int length = (int)pBATH->GetRowCount();

						double* yt;
						double* yb;
						double* xtemp;
						double* xotemp;

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

						// xTop (Location of stockdonTWL)
						xTop = Maximum(xotemp, length);

						Mminvinterp(xtemp, yb, xotemp, length);

						xBottom = Maximum(xotemp, length);
						delete[] xotemp;
						delete[] yb;
						delete[] yt;
						delete[] xtemp;

						// Compute slope where the swash acts
						slope_TAW_local = (yTop - yBottom) / abs(xTop - xBottom);
						int tempbreak = 10;
						/////if (m_debugOn)
						/////   {
						/////   m_pDuneLayer->SetData(dunePt, m_colRunupFlag, 0);
						/////   m_pDuneLayer->SetData(dunePt, m_colTAWSlope, slope_TAW_local);
						/////   m_pDuneLayer->SetData(dunePt, m_colSTKTWL, STK_TWL);
						/////   }
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
							/////if (m_debugOn)
							/////   {
							/////   m_pDuneLayer->SetData(dunePt, m_colRunupFlag, 1);
							/////   m_pDuneLayer->SetData(dunePt, m_colTAWSlope, slope_TAW_local);
							/////   }
						}

						// the water is swashing on the beach
						// assume TAW slope equal to beach slope
						if (yTop < duneToe)
						{
							slope_TAW_local = tanb1;
							/////if (m_debugOn)
							/////   {
							/////   m_pDuneLayer->SetData(dunePt, m_colRunupFlag, 2);
							/////   m_pDuneLayer->SetData(dunePt, m_colTAWSlope, slope_TAW_local);
							/////   }
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
							////if (m_debugOn)
							////   {
							////   m_pDuneLayer->SetData(dunePt, m_colRunupFlag, 3);
							////   m_pDuneLayer->SetData(dunePt, m_colTAWSlope, slope_TAW_local);
							////   }

							/*xBottom = yBottom / tanb1;
							xTop = yTop / m_slopeBPS;
							slope_TAW_local_2 = ((beachwidth - xBottom)*tanb1 + xTop*m_slopeBPS) / (beachwidth + (height / m_slopeBPS));*/
							int tempbreak = 10;
						}
					}
				}  //end of:  if ( ! overtopped )
			 // If setup + swl is greater than the structure crest then the
			 // composite slope will be used. This corresponds to an overtopped case.

			 // BPS structure is flooded not maintained 
				else if (isOvertopped && (beachType != BchT_SANDY_RIPRAP_BACKED || pEnvContext->currentYear < firstMaintYear))
					//       else if (isOvertopped && (beachType == BchT_RIPRAP_BACKED || (maintFreq <= m_maintFreqBPS ) ) )
				{
					if (S_comp > 0)
					{
						slope_TAW_local = S_comp;
						//if (m_debugOn)
						//   {
						//   m_pDuneLayer->SetData(dunePt, m_colRunupFlag, 4);
						//   m_pDuneLayer->SetData(dunePt, m_colTAWSlope, slope_TAW_local);
						//   }
					}
					else
					{
						slope_TAW_local = tanb1;
						//if (m_debugOn)
						//   {
						//   m_pDuneLayer->SetData(dunePt, m_colRunupFlag, 5);
						//   m_pDuneLayer->SetData(dunePt, m_colTAWSlope, slope_TAW_local);
						//   }
					}
				}
				// compute the composite slope using the known width of the structure+beach and the slopes 
				//    else if (isOvertopped && (beachType == BchT_RIPRAP_BACKED || (maintFreq >= m_maintFreqBPS) ) )
				else if (isOvertopped && (beachType == BchT_SANDY_RIPRAP_BACKED || pEnvContext->currentYear > firstMaintYear))
				{
					// width weighted average of beach and structure slopes
					// (beachSlope * beachWidth + structureSlope * structureWidth ) / (beachWidth + structureWidth)
					//  where structureWidth = height / structureSlope 
					slope_TAW_local = (tanb1 * beachwidth + height) / ((height / m_slopeBPS) + beachwidth);
					//if (m_debugOn)
					//   {
					//   m_pDuneLayer->SetData(dunePt, m_colRunupFlag, 6);
					//   m_pDuneLayer->SetData(dunePt, m_colTAWSlope, slope_TAW_local);
					//   }
				}

				//check to make sure the slope does not exceed the structure slope
				if (isNan<double>(slope_TAW_local))
					slope_TAW_local = 0.3;

				if (slope_TAW_local > 0.5)
					slope_TAW_local = 0.5;

				//////////// RUNUP EQUATION - DETERMINISTIC APPROACH (includes +1STDEV factor of safety) ////////////
				// The following are calculations for dynamic coastline

				// Mean Wave Period 
				double Tm = wavePeriod / 1.1;

				// Deep water wave length for the DIM method to be applied in the presence of structures/barriers.
				double L = (G * (Tm * Tm)) / (2.0 * PI);

				//breaker parameter
				// Irribarren number based on 2ND slope estimate - Eqn D.4.5-8
				double Ibarren_local = slope_TAW_local / (sqrt(Hmo / L));

				////Berm factor always equal to 1
				float gammaBerm = 1.0f;

				double limit = gammaBerm * Ibarren_local;

				// if the roughness reduction factor is unknown,
				// use an the average roughness reduction factor for shoreline backed by rip-rap 
				if (gammaRough > 99 || gammaRough < 0)
				{
					gammaRough = 0.55f;
					switch (beachType)
					{
					case BchT_MIXED_SEDIMENT_DUNE_BACKED:
					case BchT_MIXED_SEDIMENT_BLUFF_BACKED:
						gammaRough = 0.8f; // (similar to Till110 and 111 on page 129 of report)
						break;

					case BchT_SANDY_RIPRAP_BACKED:
					case BchT_SANDY_COBBLEGRAVEL_BLUFF_BACKED:
					case BchT_SANDY_COBBLEGRAVEL_BERM_BACKED:
					case BchT_JETTY:
					case BchT_ROCKY_CLIFF_HEADLAND:    // we probably do not need to compute runup at all on these beach types
						gammaRough = 0.55f;
						break;

					case BchT_SANDY_WOODY_DEBRIS_BACKED:
						gammaRough = 0.75f;
						break;
					}
				}

				if (gammaRough < 0.1)
					gammaRough = m_minBPSRoughness;

				if (limit > 0.0f && limit < 1.8f)
					runup_TAW_local = Hmo * 1.75f * gammaRough * gammaBeta * gammaBerm * Ibarren_local;
				else if (limit >= 1.8f)
					runup_TAW_local = Hmo * gammaRough * gammaBeta * (4.3f - (1.6f / sqrt(Ibarren_local)));

				// COMBINE RUNUP VALUES TO CALCULATE COMBINED TWL //

				// Wave Runup consists of setup and swash (barrier runup)
				double structureR2 = (runup_TAW_local + 1.1f * setup);
				r2Runup = (float)structureR2;

				// this attribute is for debugging
				////if (m_debugOn)
				////   {
				////   m_pDuneLayer->SetData(dunePt, m_colComputedSlope, slope_TAW_local);
				////   m_pDuneLayer->SetData(dunePt, m_colComputedSlope2, slope_TAW_local_2);
				////   }
				dailyTWL = swl + (float)structureR2;
			}  // end of: else if (beachType == BchT_RIPRAP_BACKED ||
			   //                  beachType == BchT_DUNE_BLUFF_BACKED ||
			   //                  beachType == BchT_CLIFF_BACKED ||
			   //                  beachType == BchT_WOODY_DEBRIS_BACKED ||
			   //                  beachType == BchT_WOODY_BLUFF_BACKED ||
			   //                  beachType == BchT_ROCKY_HEADLAND)

		  // check for dune hours
		  // collision regime (dune erosion expected

			if (dailyTWL > duneToe && dailyTWL < duneCrest)
			{
				// hold running total of number of days dune toe is impacted
				m_pDuneLayer->GetData(dunePt, m_colDLIDDToe, IDDToeCount);
				m_pDuneLayer->SetData(dunePt, m_colDLIDDToe, ++IDDToeCount);

				// winter impact days per year on dune toe
				if (doy > 355 || doy < 79)
				{
					m_pDuneLayer->GetData(dunePt, m_colDLIDDToeWinter, IDDToeCountWinter);
					m_pDuneLayer->SetData(dunePt, m_colDLIDDToeWinter, ++IDDToeCountWinter);
				}
				// Spring impact days per year on dune toe
				else if (doy > 79 && doy < 172)
				{
					m_pDuneLayer->GetData(dunePt, m_colDLIDDToeWinter, IDDToeCountSpring);
					m_pDuneLayer->SetData(dunePt, m_colDLIDDToeWinter, ++IDDToeCountSpring);
				}
				// Summer impact days per year on dune toe
				else if (doy > 172 && doy < 266)
				{
					m_pDuneLayer->GetData(dunePt, m_colDLIDDToeWinter, IDDToeCountSummer);
					m_pDuneLayer->SetData(dunePt, m_colDLIDDToeWinter, ++IDDToeCountSummer);
				}
				// Fall impact days per year on dune toe
				else
				{
					m_pDuneLayer->GetData(dunePt, m_colDLIDDToeWinter, IDDToeCountFall);
					m_pDuneLayer->SetData(dunePt, m_colDLIDDToeWinter, ++IDDToeCountFall);
				}
			}
			// overtopped 
			// dune is flooded
			if (dailyTWL > duneCrest)
			{
				// hold running total of number of days dune crest is impacted
				m_pDuneLayer->GetData(dunePt, m_colDLIDDCrest, IDDCrestCount);
				m_pDuneLayer->SetData(dunePt, m_colDLIDDCrest, ++IDDCrestCount);

				// Winter impact days per year on dune crest
				if (doy > 355 || doy < 79)
				{
					m_pDuneLayer->GetData(dunePt, m_colDLIDDCrestWinter, IDDCrestCountWinter);
					m_pDuneLayer->SetData(dunePt, m_colDLIDDCrestWinter, ++IDDCrestCountWinter);
				}
				// Spring impact days per year on dune crest
				else if (doy > 79 && doy < 172)
				{
					m_pDuneLayer->GetData(dunePt, m_colDLIDDCrestSpring, IDDCrestCountSpring);
					m_pDuneLayer->SetData(dunePt, m_colDLIDDCrestSpring, ++IDDCrestCountSpring);
				}
				// Summer impact days per year on dune crest
				else if (doy > 172 && doy < 266)
				{
					m_pDuneLayer->GetData(dunePt, m_colDLIDDCrestSummer, IDDCrestCountSummer);
					m_pDuneLayer->SetData(dunePt, m_colDLIDDCrestSummer, ++IDDCrestCountSummer);
				}
				// Fall impact days per year on dune crest
				else
				{
					m_pDuneLayer->GetData(dunePt, m_colDLIDDCrestFall, IDDCrestCountFall);
					m_pDuneLayer->SetData(dunePt, m_colDLIDDCrestFall, ++IDDCrestCountFall);
				}
			}

			// get current Maximum TWL from coverage
			float yearlyMaxTWL = 0.0f;
			m_pDuneLayer->GetData(dunePt, m_colDLYrMaxTWL, yearlyMaxTWL);

			// store the yearly average TWL
			float yearlyAvgTWL = 0.0f;
			m_pDuneLayer->GetData(dunePt, m_colDLYrAvgTWL, yearlyAvgTWL);

			yearlyAvgTWL += dailyTWL / 364.0f;
			m_pDuneLayer->SetData(dunePt, m_colDLYrAvgTWL, yearlyAvgTWL);

			//m_maxTWLArray.Set(duneIndex, row, dailyTWL);

			// store the yearly average SWL
			m_pDuneLayer->AddData(dunePt, m_colDLYrAvgSWL, swl / 364.0f);

			// store the yearly average low SWL
			m_pDuneLayer->AddData(dunePt, m_colDLYrAvgLowSWL, lowSWL / 364.0f);

			float yearlyMaxSWL = 0.0f;
			m_pDuneLayer->GetData(dunePt, m_colDLYrMaxSWL, yearlyMaxSWL);

			// set variables when Maximum yearly TWL
			if (swl > yearlyMaxSWL)
				m_pDuneLayer->SetData(dunePt, m_colDLYrMaxSWL, swl);

			// if this TWL is the highest yet so far this year for this location, remember it
			if (dailyTWL > yearlyMaxTWL)
			{
				m_pDuneLayer->SetData(dunePt, m_colDLYrFMaxTWL, dailyTWL_Flood);  // flooding (use for flood model handoff)
				m_pDuneLayer->SetData(dunePt, m_colDLYrMaxTWL, dailyTWL);

				ASSERT(Hdeep > 0.0f);
				m_pDuneLayer->SetData(dunePt, m_colDLHdeep, Hdeep);
				m_pDuneLayer->SetData(dunePt, m_colDLWPeriod, wavePeriod);
				m_pDuneLayer->SetData(dunePt, m_colDLWHeight, waveHeight);
				m_pDuneLayer->SetData(dunePt, m_colDLWDirection, waveDirection);
				m_pDuneLayer->SetData(dunePt, m_colDLYrMaxDoy, doy);
				////      m_pDuneLayer->SetData(dunePt, m_colYrMaxSWL, swl);
				m_pDuneLayer->SetData(dunePt, m_colDLYrMaxSetup, setup);
				m_pDuneLayer->SetData(dunePt, m_colDLYrMaxIGSwash, infragravitySwash);
				m_pDuneLayer->SetData(dunePt, m_colDLYrMaxIncSwash, incidentSwash);
				m_pDuneLayer->SetData(dunePt, m_colDLYrMaxRunup, r2Runup);

				float stockdonSwash = sqrt(infragravitySwash * infragravitySwash + incidentSwash * incidentSwash) / 2.0f;
				m_pDuneLayer->SetData(dunePt, m_colDLYrMaxSwash, stockdonSwash);


				/*if (dailyTWL_2 > duneCrest && (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED))
				{
				m_pDuneLayer->SetData(dunePt, m_colFlooded, 1);
				m_pDuneLayer->SetData(dunePt, m_colAmtFlooded, (dailyTWL - duneCrest));
				}*/

				if (dailyTWL > duneCrest && (beachType != BchT_BAY || beachType != BchT_RIVER || beachType != BchT_UNDEFINED))
				{
					m_pDuneLayer->SetData(dunePt, m_colDLFlooded, 1);
					m_pDuneLayer->SetData(dunePt, m_colDLAmtFlooded, (dailyTWL - duneCrest));
				}

				//    m_pDuneLayer->SetData(dunePt, m_colDLTranSlope, slope_TAW_local)

				/*SetUpArray[ duneIndex ] = eta;
				IGArray[ duneIndex ] = STK_IG;
				wlArray[ duneIndex ] = swl;
				stkTWLArray[ duneIndex ] = STK_TWL;*/
			}

			/*  if (ftwl > maxFTWLforPeriod)
			{
			periodMaxTWL = row;
			tranIndexMaxTWL = tranIndex;
			maxTWLforPeriod = twl;
			}

			if (ftwl > m_maxFTWL)
			{
			periodMaxTWL = row;
			tranIndexMaxTWL = tranIndex;
			m_maxTWL = twl;
			}*/

			/* prevProfileIndex = profileID;
			prevTranIndex = tranIndex;
			prevTanb1 = tanb1;
			prevDuneCrest = duneCrest;*/

		}  // end of: for each dune point in dune line


	} // end of: for each doy

 // fix up map classifications
 //m_pDuneLayer->ClassifyData();
 //m_pBayTraceLayer->ClassifyData();

  //m_pProfileLUT->WriteAscii(_T("C:\\temp\\ProfileLookup.csv"));
  //m_maxTWLArray.WriteAscii(_T("C:\\temp\\twl.csv"));

} // end CalculateYrMaxTWL





bool ChronicHazards::ComputeStockdon(float stillwaterLevel, float waveHeight, float wavePeriod, float waveDirection,
	float* _L0, float* _setup, float* _incidentSwash, float* _infragravitySwash, float* _r2Runup, float* _twl)
{
	// Wave runup is a combination of the maximum setup at the shoreline and swash, the time-varying oscillations about the setup
	// Swash includes both incident and infragravity motions

	float L0 = (G * wavePeriod * wavePeriod) / (2 * PI);
	float HL0 = waveHeight * L0;
	float setup = 0.35f * TANB1_005 * sqrt(HL0); // water level elevation above stillwater level due to wave momentum
	float incidentSwash = 0.75f * TANB1_005 * sqrt(HL0); // Incident Swash, Stockdon (2006) Eqn: 11
	float infragravitySwash = sqrt(HL0) * 0.06f;         // Infragravity Swash, Stockdon (2006) Eqn: 12 [IG = 0.06f * sqrt(Hdeep * L0)]

	// wave-induced water level
	// Total Water Levels are comprised of a still water level measured at tide gauges and the wave runup
	float r2Runup = 1.1f * (setup + (sqrt(HL0 * (0.563f * TANB1_005 * TANB1_005 + 0.004f))) / 2.0f); //Stockdon R2 - Runup Eq 19
	float twl = stillwaterLevel + r2Runup;

	if (_L0 != nullptr)
		*_L0 = L0;

	if (_setup != nullptr)
		*_setup = setup;

	if (_incidentSwash != nullptr)
		*_incidentSwash = incidentSwash;

	if (_infragravitySwash != nullptr)
		*_infragravitySwash = infragravitySwash;

	if (_r2Runup != nullptr)
		*_r2Runup = r2Runup;

	if (_twl != nullptr)
		*_twl = twl;

	return true;
}


bool ChronicHazards::PopulateClosestIndex(MapLayer* pFromLayer, LPCTSTR field, DataObj* pLookup)
{
	// NOTE: this assumes:
	// 1) the To layer is a point coverage
	// 1) points are ordered from north to south 
	int col = pFromLayer->GetFieldCol(field);
	if (col < 0)
		return false;

	CString msg;
	msg.Format("Populating index field [%s:%s] for lookup table %s", pFromLayer->m_name, field, pLookup->GetName());
	Report::StatusMsg(msg);
	int belowCount = 0;

	for (MapLayer::Iterator fromRow = pFromLayer->Begin(); fromRow < pFromLayer->End(); fromRow++)
	{
		ASSERT(pFromLayer->GetType() == LT_POINT);
		REAL x, y;
		pFromLayer->GetPointCoords(fromRow, x, y);

		// scan lookup points to find the row of the first lookup point "below" the From layer
		int luRow = -1;
		for (int i = 0; i < pLookup->GetRowCount(); i++)
		{
			double luY = pLookup->GetAsDouble(1, i);
			if (luY < y)   // just below from point?
			{
				luRow = i;
				break;
			}
		}

		if (luRow == -1)  // if we scanned down the lookup points, but didn't find any below us
		{
			belowCount++;
			luRow = pLookup->GetRowCount() - 1;
			//msg.Format("Couldn't find index below from point (%.0f) - lookups range is (%.0f-%.0f) when populating [%s].  The last lookup index will be used",
			//   y, (float) pLookup->GetAsFloat(1,0), (float) pLookup->GetAsFloat(1, luRow), field);
			//Report::LogWarning(msg);
		}

		// see if we are closer to point above or below
		else if (luRow > 0)
		{
			double luYAbove = pLookup->GetAsDouble(1, luRow - 1);
			double luYBelow = pLookup->GetAsDouble(1, luRow);
			if ((luYAbove - y) > (y - luYBelow))
				luRow -= 1;
		}

		int luIndex = (int)pLookup->GetAsInt(0, luRow);
		pFromLayer->SetData(fromRow, col, luIndex);
	}

	Report::StatusMsg("Populating index complete...");
	return true;
}

bool ChronicHazards::SetDuneToeUTM2LL(MapLayer* pLayer, int point)
{
	double easting = 0.0;
	pLayer->GetData(point, m_colDLEastingToe, easting);
	double northing = 0.0;
	pLayer->GetData(point, m_colDLNorthing, northing);

	GDALWrapper gdal;
	GeoSpatialDataObj geoSpatialObj(U_UNDEFINED);
	CString projectionWKT = m_pDuneLayer->m_projection;
	int utmZone = geoSpatialObj.GetUTMZoneFromPrjWKT(projectionWKT);

	// 22 = WGS 84 EquatorialRadius, and 1/flattening.  UTM zone 10.
	double longitude = 0.0;
	double latitude = 0.0;
	gdal.UTMtoLL(22, northing, easting, utmZone, latitude, longitude);

	bool readOnly = pLayer->m_readOnly;
	pLayer->m_readOnly = false;
	pLayer->SetData(point, m_colDLLatitudeToe, latitude);
	pLayer->SetData(point, m_colDLLongitudeToe, longitude);
	pLayer->m_readOnly = readOnly;

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



// Returns the interpolated value in ycol given an x in xcol
// Also returns the row, as an argument, that is closest to the computed interpolated value 
// If the magnitudes from the interpolated value are equal,
// method returns the first row encountered during the search (the lesser row in the case of the forward search and the greater row in the case of the reverse search)

double ChronicHazards::IGet(DDataObj* table, double x, int xcol, int ycol/*= 1*/, IMETHOD method/*=IM_LINEAR*/, int startRow/*=0*/, int* returnRow/*=nullptr*/, bool forwardSearch/*=true*/)
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
			{
				break;
			}
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


//Erosion models:
	//Event based erosion (Kriebel and Dean [KDmodel], 1993)
	//Coastal change rate associated with climate change (Bruun, 1962)
	//long-term coastal erosion rate [hist_SCR] associated with sediment budget (Ruggiero et al., 2013).

float ChronicHazards::KDmodel(int point)
{
	// Based upon yearly Maximum TWL
	float yrMaxTWL = 0.0f;
	float Hdeep = 0.0f;
	float Period = 0.0f;
	int doy = -1;
	float tanb1 = 0;
	float duneToe = 0;
	float duneCrest = 0;
	float foreshoreSlope = 0;
	// Currently set to a constant
	const float A = 0.08f;

	
	// SWH at yearly Maximum TWL   
	m_pDuneLayer->GetData(point, m_colDLHdeep, Hdeep);

	// Wave Period at yearly Maximum TWL                  
	m_pDuneLayer->GetData(point, m_colDLWPeriod, Period);

	// Day of Year of yearly Maximum TWL
	m_pDuneLayer->GetData(point, m_colDLYrMaxDoy, doy);
	// Erosion extents are calculated based upon the yearly Maximum TWL 


		// Get dynamic Duneline characteristics 
	m_pDuneLayer->GetData(point, m_colDLYrMaxTWL, yrMaxTWL);
	m_pDuneLayer->GetData(point, m_colDLTranSlope, tanb1);
	m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToe);
	m_pDuneLayer->GetData(point, m_colDLDuneCrest, duneCrest);
//	m_pDuneLayer->GetData(point, m_colDLBeachWidth, beachwidth);

	
	m_pDuneLayer->GetData(point, m_colDLForeshore, foreshoreSlope);


	//Deepwater wave length (m)
	double L = (G * (Period * Period)) / (2.0 * PI);

	double Cgo = (0.5 * G * Period) / (2.0 * PI);

	// Breaking height  = 0.78 * breaking depth
	//double Hb = ((0.563 * Hdeep) / pow((Hdeep / L), (1.0 / 5.0)));
	double Hb = (0.78 / pow(G, 1. / 5.)) * pow(Cgo * Hdeep * Hdeep, 2.0 / 5.0);

	// Breaking wave water depth
	// hb = Hb / gamma where gamma is a breaker index of 0.78  Kriebel and Dean
	double hbl = Hb / 0.78;

	// Need the negative value since profile is water height relative to land
	float nhbl = (float)-hbl;

	//int rowHB = -1;
	//double eastingHB = IGet(pBathy, nhbl, 0, 1, IM_LINEAR, rowMHW, &rowHB, false);
	double eastingHB = 0;  // ???? from peter

	// Calculate the new surf zone width referenced to the MHW
	//double Wb = eastingMHW - eastingHB;
	double Wb = 500;   // distance wave's break from shoreline (go back to origninal when we have eastingHB

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
	double TS = ((C1 * pow(Hb, (3.0 / 2.0)) / (pow(G, (1.0 / 2.0)) * pow(A, 3.0))) * pow((1.0 + hbl / (duneCrest - MHW) + foreshoreSlope * Wb / hbl), -1.0)) / 3600.0;

	//// Equation 25
	float R_inf_KD = ((yrMaxTWL - MHW) * (Wb - hbl / foreshoreSlope)) / ((duneCrest - MHW) + hbl - (yrMaxTWL - MHW) / 2.0);
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

	if (R_inf_KD < 0)
		R_inf_KD = 0;

	// Restrict maximum yearly event-based erosion
	if (R_inf_KD > 25)
		R_inf_KD = 25;


	return R_inf_KD;

}


float ChronicHazards::Bruunmodel(EnvContext* pEnvContext, int point)
{
	float R_bruun = 0;
	float duneToe = 0.0f;
	float bruunSlope = 0;
	float slr = 0.0f;
	float xBruun = 0.0f;
	float R_bruun_total = 0.0f;

	m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToe);
	m_pDuneLayer->GetData(point, m_colDLBruunSlope, bruunSlope);


	// Bruun Rule Calculation to incorporate SLR into the shoreline change rate
	if (pEnvContext->yearOfRun > 0)
	{
		float prevslr = 0.0f;
		int year = pEnvContext->yearOfRun;
		m_slrData.Get(0, year, slr);
		if (pEnvContext->yearOfRun > 0)
			m_slrData.Get(0, year - 1, prevslr);

		float toeRise = slr - prevslr;  // change since past year
		
			// Beach slope and beachwidth remain static as dune erodes landward and equilibrium is reached 
			// while the dune toe elevation rises at the rate of SLR
			if (toeRise > 0.0f)
				m_pDuneLayer->SetData(point, m_colDLDuneToe, (duneToe + toeRise));

		// Determine influence of SLR on chronic erosion, characterized using a Bruun rule calculation
		if (toeRise > 0.0f)
			R_bruun = toeRise / bruunSlope;    // should be stored in dune line
			R_bruun_total = pEnvContext->yearOfRun * R_bruun;
	}
	return R_bruun;
}


float ChronicHazards::SCRmodel(EnvContext* pEnvContext, int point)
{
	float R_SCR_hist = 0.0f;
	float shorelineChangeRate = 0.0f;
	float R_SCR_hist_Total= 0.0f;

	m_pDuneLayer->GetData(point, m_colDLShorelineChange, shorelineChangeRate); //get historic shoreline chnage rate data from duneline file 

	//neglect the progradation
	if (shorelineChangeRate < 0.0f)
		R_SCR_hist = (float)-shorelineChangeRate;
	else
		R_SCR_hist = 0.0f;
		R_SCR_hist_Total = pEnvContext->yearOfRun * R_SCR_hist;

	return R_SCR_hist;
}







/*
//	// These characteristics do not change through the time period of this model
//	int duneIndex = -1;
//	//int transID = -1;
//	//int profileID = -1;
//	float shorelineChangeRate = 0.0f;
//	float shorelineAngle = 0.0f;

//	// Get static Duneline characteristics
//	m_pDuneLayer->GetData(point, m_colDLDuneIndex, duneIndex);
//	//m_pDuneLayer->GetData(point, m_colDLProfileID, profileID);
//	m_pDuneLayer->GetData(point, m_colDLShorelineChange, shorelineChangeRate);
//	m_pDuneLayer->GetData(point, m_colDLShorelineAngle, shorelineAngle);
//	//   m_pDuneLayer->GetData(point, m_colA, A);

//	// A is a parameter that governs the overall steepness of the profile
//	// varies primarily with grain size or fall velocity
//	// currently set to a constant for the whole shoreline
//	// eventually want to estimate by doing a linear regression
//	// between the foreshore slope and the best-fit shape parameter
//	// at each profile
//	// Currently set to a constant
//	double A = 0.08f;

//	// Dynamically changine Duneline characteristics
//	float tanb1 = 0.0f;
//	float duneToe = 0.0f;
//	float duneCrest = 0.0f;
//	float beachwidth = 0;

//	// Get dynamic Duneline characteristics
//	m_pDuneLayer->GetData(point, m_colDLTranSlope, tanb1);
//	m_pDuneLayer->GetData(point, m_colDLDuneToe, duneToe);
//	m_pDuneLayer->GetData(point, m_colDLDuneCrest, duneCrest);
//	m_pDuneLayer->GetData(point, m_colDLBeachWidth, beachwidth);

//	// Get Bathymetry Data (cross-shore profile)
//	//int profileIndex = m_profileMap[profileID];
//	//DDataObj* pBathy = m_BathyDataArray.GetAt(profileIndex);   // zero-based
//	//ASSERT(pBathy != nullptr);
//	//
//	//// from the bathymetry data, get the mean high water mark from the bathymetry
//	//int rowMHW = -1;
//	//double eastingMHW = IGet(pBathy, MHW, 0, 1, IM_LINEAR, 0, &rowMHW, true);
//	double eastingMHW = 0;
//	m_pDuneLayer->GetData(point, m_colDLEastingMHW, eastingMHW);

//	// The foreshore and Bruun Rule slopes are calculated once using the
//	// cross-shore profile at the beginning of the simulation
//	float bruunSlope = 0;
//	float foreshoreSlope = 0;


//	// float BruunSlope = 0;
//	m_pDuneLayer->GetData(point, m_colDLBruunSlope, bruunSlope);
//	m_pDuneLayer->GetData(point, m_colDLForeshore, foreshoreSlope);

//	double xCoord = 0.0;
//	double yCoord = 0.0;

//	// Based upon yearly Maximum TWL
//	float yrMaxTWL = 0.0f;
//	float Hdeep = 0.0f;
//	float Period = 0.0f;
//	int doy = -1;

//	// Yearly Maximum TWL - This represents the largest storm of the year
//	//   m_pDuneLayer->GetData(point, m_colYrEMaxTWL, yrEMaxTWL);
//	m_pDuneLayer->GetData(point, m_colDLYrMaxTWL, yrMaxTWL);
//	/*if (pEnvContext->yearOfRun < m_windowLengthTWL -1)
//	m_pDuneLayer->GetData(point, m_colYrAvgTWL, yrMaxTWL);
//	else
//	m_pDuneLayer->GetData(point, m_colMvMaxTWL, yrMaxTWL);*/

//	m_meanTWL += yrMaxTWL;

//	// SWH at yearly Maximum TWL   
//	m_pDuneLayer->GetData(point, m_colDLHdeep, Hdeep);

//	// Wave Period at yearly Maximum TWL                  
//	m_pDuneLayer->GetData(point, m_colDLWPeriod, Period);

//	// Day of Year of yearly Maximum TWL
//	m_pDuneLayer->GetData(point, m_colDLYrMaxDoy, doy);


//	float waveHeight = 0.0;
//	m_pDuneLayer->GetData(point, m_colDLWHeight, waveHeight);

//	//Now we track ShorelineChange year by year
//	  float ShorelineChange = 0;

//	if ((pEnvContext->currentYear) > 2010 && EPR < 0)
//	ShorelineChange = -EPR;
//	

//	// Erosion extents are calculated based upon the yearly Maximum TWL 

//	//Deepwater wave length (m)
//	double L = (G * (Period * Period)) / (2.0 * PI);

//	double Cgo = (0.5 * G * Period) / (2.0 * PI);

//	// Breaking height  = 0.78 * breaking depth
//	//double Hb = ((0.563 * Hdeep) / pow((Hdeep / L), (1.0 / 5.0)));
//	double Hb = (0.78 / pow(G, 1. / 5.)) * pow(Cgo * Hdeep * Hdeep, 2.0 / 5.0);

//	// Breaking wave water depth
//	// hb = Hb / gamma where gamma is a breaker index of 0.78  Kriebel and Dean
//	double hbl = Hb / 0.78;

//	// Need the negative value since profile is water height relative to land
//	float nhbl = (float)-hbl;

//	//int rowHB = -1;
//	//double eastingHB = IGet(pBathy, nhbl, 0, 1, IM_LINEAR, rowMHW, &rowHB, false);
//	double eastingHB = 0;  // ???? from peter

//	// Calculate the new surf zone width referenced to the MHW
//	//double Wb = eastingMHW - eastingHB;
//	double Wb = 500;   // distance wave's break from shoreline (go back to origninal when we have eastingHB

//	// Equation 31 
//	// Constant storm duration, this could be altered       units: hrs
//	double TD = m_constTD;
//	double C1 = 320.0;

//	// using Jeremy Mull's Thesis equations:

//	// Equation 12
//	//double TS = ((C1*pow(Hb, (3.0 / 2.0)) / (pow(g, (1.0 / 2.0)) * pow(A, 3.0))) * pow((1.0 + hbl / (duneToe - MHW) + foreshoreSlope*Wb / hbl), -1.0)) / 3600.0;

//	//// Equation 7
//	//double R_inf_KD = ((yrMaxTWL - MHW)* (Wb - hbl / foreshoreSlope)) / (duneCrest - duneToe + hbl - (yrMaxTWL - MHW) / 2.0);

//	// exponential time scale Kriebel and Dean, eq 31  units: hrs
//	double TS = ((C1 * pow(Hb, (3.0 / 2.0)) / (pow(G, (1.0 / 2.0)) * pow(A, 3.0))) * pow((1.0 + hbl / (duneCrest - MHW) + foreshoreSlope * Wb / hbl), -1.0)) / 3600.0;

//	//// Equation 25
//	double R_inf_KD = ((yrMaxTWL - MHW) * (Wb - hbl / foreshoreSlope)) / ((duneCrest - MHW) + hbl - (yrMaxTWL - MHW) / 2.0);
//	//   double R_inf_KD = (yrMaxTWL* (Wb - hbl / foreshoreSlope)) / (duneCrest + hbl - yrMaxTWL / 2.0);


//	// Ratio of the erosion time scale to the storm duration K&D eq 11
//	double betaKD = 2.0 * PI * TS / TD;

//	// Phase lag of maximum erosion response
//	double phase1 = Maths::RootFinding::secant(betaKD, 1, 0, 0, 0, PI / 2.0f, PI);

//	// Time of maximum erosion 
//	double tm1 = TD * phase1 / PI;

//	// Rmax / equailibrium response, K&D eq 13
//	// Alpha is the characteristics rate parameter of the system = 1 / Ts ,  K&D eq 5
//	double alpha = 0.5 * (1.0f - cos(2.0 * PI * (tm1 / TD)));

//	if (isNan<double>(alpha))
//		alpha = 0.02;

//	if (alpha > .4)
//		alpha = 0.4;

//	//multiply by alpha to get 
//	R_inf_KD *= alpha;

//	if (isNan<double>(R_inf_KD))
//		R_inf_KD = 0;

//	// consider adding additional beach types?
//	if (beachType != BchT_SANDY_DUNE_BACKED) // && beachType != BchT_UNDEFINED) || R_inf_KD < 0)
//		R_inf_KD = 0;

//	if (R_inf_KD < 0)
//		R_inf_KD = 0;

//	// Restrict maximum yearly event-based erosion
//	if (R_inf_KD > 25)
//		R_inf_KD = 25;

//	float slr = 0.0f;
//	float xBruun = 0.0f;

//	// Bruun Rule Calculation to incorporate SLR into the shoreline change rate
//	if (pEnvContext->yearOfRun > 0)
//	{
//		float prevslr = 0.0f;
//		int year = pEnvContext->yearOfRun;
//		m_slrData.Get(0, year, slr);
//		if (pEnvContext->yearOfRun > 0)
//			m_slrData.Get(0, year - 1, prevslr);

//		float toeRise = slr - prevslr;  // change since past year
//		if (beachType == BchT_SANDY_DUNE_BACKED)
//		{
//			// Beach slope and beachwidth remain static as dune erodes landward and equilibrium is reached 
//			// while the dune toe elevation rises at the rate of SLR
//			if (toeRise > 0.0f)
//				m_pDuneLayer->SetData(point, m_colDLDuneToe, (duneToe + toeRise));
//		}

//		// Determine influence of SLR on chronic erosion, characterized using a Bruun rule calculation
//		if (toeRise > 0.0f)
//			xBruun = toeRise / bruunSlope;    // shoude be stored in dune line
//	}

//	// 
//	//////int Loc = 0;
//	//////m_pDuneLayer->GetData(point, m_colDLLittCell, Loc);

//	// For Hard Protection Structures
//	// change the beachwidth and slope to reflect the new shoreline position
//	// beach is assumed to narrow at the rate of the total chronic erosion, reflected through 
//	// dynamic beach slopes
//	// beaches further narrow when maintaining and constructing BPS at a 1:2 slope

//	// !!! include additional beach types
//	if (beachType == BchT_RIPRAP_BACKED)
//	{
//		// Need to account for negative shoreline change rate 
//		// shorelineChangeRate > 0.0f ? SCRate = shorelineChangeRate : SCRate;

//		// Negative shoreline change rate indicates eroding shoreline
//		// Currently ignoring progradating shoreline
//		float SCRate = 0.0f;

//		if (shorelineChangeRate < 0.0f)
//			SCRate = (float)-shorelineChangeRate;
//		else
//			SCRate = 0.0f;

//		// Negative shoreline change rate narrows beach
//		if (xBruun > 0.0f)
//			beachwidth = beachwidth - SCRate - xBruun;
//		else
//			beachwidth = beachwidth - SCRate;

//		if ((duneToe / beachwidth) > 0.1f)
//			beachwidth = duneToe / 0.1f;

//		tanb1 = duneToe / beachwidth;

//		// Determine a minimum slope for which the
//		// beach can erode

//		if (tanb1 > m_slopeThresh)
//			tanb1 = m_slopeThresh;

//		// slope is steepened due to beachwidth narrowing
//		m_pDuneLayer->SetData(point, m_colDLTranSlope, tanb1);

//		// keeping beachwidth static, translate MHW by same amount
//		/*m_pDuneLayer->GetData(point, m_colEastingMHW, x);
//		m_pDuneLayer->SetData(point, m_colEastingMHW, x + dx);*/
//	}