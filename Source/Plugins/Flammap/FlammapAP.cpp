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
// Flammap.cpp : Defines the initialization routines for the DLL.
//
#include "stdafx.h"
#pragma hdrstop

#include "FlammapAP.h"
#include <MapLayer.h>
#include "PolyGridLookups.h"
#include "MinTravelTime.h"
#include <EnvContext.h>
#include <EnvConstants.h>
#include <EnvModel.h>

#include "FireYearRunner.h"
#include "FlameLengthUpdater.h"

#include <Path.h>
#include <PathManager.h>

#include <math.h>
#include <iostream>
#include <fstream>
#include <afxtempl.h>
#include <cstdlib>

#include <randgen\Randunif.hpp>
//#include "FlamMapAPDlg.h"


using namespace std;
using namespace nsPath;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern "C" _EXPORT EnvExtension * Factory(EnvContext * pContext)
   {
   return (EnvExtension*) new FlamMapAP;
   }


const long INIT_SEED = 4;

FlamMapAP* gpFlamMapAP = NULL;

bool FailAndReturn(LPCTSTR failureID, LPCTSTR failMsg);
bool FailAndReturnOnBadStatus(int runStatus, LPCTSTR failureID, LPCTSTR failMsg);

FAGrid::FAGrid(Map* pMap) : MapLayer(pMap)
   {}


FIRELIST* FMScenario::GetFireList(int index /*=-1*/) // note - default picks a list randomly
   {
   if (index >= 0)
      return this->m_fireListArray[index];

   index = (int)gpFlamMapAP->m_pFiresListRand->RandValue(0, m_fireListArray.GetSize() - 0.000001f);
   ASSERT(index >= 0 && index < (int)m_fireListArray.GetSize());

   return m_fireListArray[index];
   }


void FlamMapAP::GetPolyGridStats(PolyGridLookups* pGrid)
   {
   int nRows, nCols;
   nRows = pGrid->GetNumGridRows();
   nCols = pGrid->GetNumGridCols();
   maxPolyValTtl = 0;
   for (int x = 0; x < nRows; x++)
      {
      for (int y = 0; y < nCols; y++)
         {
         int c = pGrid->GetPolyValTtlForGridPt(x, y);
         maxPolyValTtl = max(maxPolyValTtl, c);
         }
      }
   TRACE1("MaxPolYValTtl = %d\n", maxPolyValTtl);
   }

FlamMapAP::FlamMapAP(void)
   : EnvModelProcess()
   , m_meanFlameLen(0)
   , m_maxFlameLen(0)
   , m_burnedAreaHa(0)
   , m_cumBurnedAreaHa(0)
   , m_pMap(NULL)
   , m_ptLayer(NULL)
   , m_pPolyGridLkUp(NULL)
   //, m_lcpGenerator()
   , m_flameLengthUpdater()
   , m_pEnvContext(NULL)
   , m_pFireYearRunner(NULL)
   , m_coverTypeCol(-1)
   , m_structStageCol(-1)
   , m_disturbCol(-1)
   , m_flameLenCol(-1)
   , m_fireIDCol(-1)
   , m_variantCol(-1)
   , m_vegClassCol(-1)
   , m_regionCol(-1)
   , m_pvtCol(-1)
   , m_saveReportFlag(-1)
   , m_runStatus(1)
   , m_polyGridReadFromFile(false)
   , m_polyGridSaveToFile(false)
   , m_vegClassHeightUnits(1)
   , m_vegClassBulkDensUnits(1)
   , m_vegClassCanopyCoverUnits(1)
   , m_rows(-1)
   , m_cols(-1)
   , m_cellDim(-1)
   , m_scenarioID(0)
   , m_lastScenarioID(0)
   , m_spotProbability(0.0)
   , m_crownFireMethod(1)
   , m_foliarMoistureContent(100)
   , m_outputEnvisionFirelists(0)
   , m_outputEnvisionFirelistName("")
   , m_outputEchoFirelistName("")
   , m_outputDeltaArrayUpdatesName("")
   , m_pFiresListRand(NULL)
   , m_pFiresRand(NULL)
   , m_pIgnitRand(NULL)
   , m_lcpFNameRoot("")
   , m_outputPath("")
   , m_outputIgnitProbGrids(false)
   , m_iduBurnPercentThreshold(0.5)
   , m_deleteLCPs(false)
   , m_randomSeed(INIT_SEED)
   , m_maxProcessors(4)
   , maxPolyValTtl(1)
   , m_customFuelFileName("")
   , m_strStaticFireList("")
   , m_fModel_plusCol(-1)
   //, m_staticFires(NULL)
   //, m_numStaticFires(0)
   , m_strStaticFMS("")
   , m_staticWindSpeed(-1)
   , m_staticWindDir(-1)
   , m_logFlameLengths(0)
   , m_logAnnualFlameLengths(0)
   , m_logArrivalTimes(0)
   , m_logPerimeters(0)
   , m_logDeltaArrayUpdates(0)
   , yearField(-1)
   , runField(-1)
   , hectareField(-1)
   , ercField(-1)
   , m_useFirelistIgnitions(1)
   , processID(0)
   , m_FireSizeRatio(0.0)
   , originalSizeField(-1)
   , xCoordField(-1)
   , yCoordField(-1)
   , burnPerField(-1)
   , azimuthField(-1)
   , windSpeedField(-1)
   , fmsField(-1)
   , fireIDField(-1)
   , m_maxRetries(0)
   , m_replicateNum(0)
   , m_sequentialFirelists(0)
   , m_envisionFireID(1)
   , envisionFireIDField(-1)
   {
   gpFlamMapAP = this;
   m_pPolyMapLayer = NULL;
   m_pLcpGenerator = NULL;
   processID = GetCurrentProcessId();
   }


FlamMapAP::~FlamMapAP(void)
   {
   /***************************************/
   //if (m_staticFires != NULL) delete[] m_staticFires;
   if (m_pPolyGridLkUp != NULL) delete m_pPolyGridLkUp;
   if (m_pFireYearRunner != NULL) delete m_pFireYearRunner;
   if (m_ptLayer != NULL)
      delete m_ptLayer;
   DestroyRandUniforms();
   if (m_pLcpGenerator)
      delete m_pLcpGenerator;
   gpFlamMapAP = NULL;
   }

void FlamMapAP::CreateRandUniforms(bool useInitSeed)
   {
   DestroyRandUniforms();
   if (useInitSeed)
      {
      m_pFiresRand = new RandUniform(0.0, 1.0, m_randomSeed); // for selecting Fires
      m_pIgnitRand = new RandUniform(0.0, 1.0, m_randomSeed); // for selecting ignition points
      m_pFiresListRand = new RandUniform(0.0, 1.0, m_randomSeed); //for selecting fireslist
      }
   else
      {
      m_pFiresRand = new RandUniform(); // for selecting Fires
      m_pIgnitRand = new RandUniform(); // for selecting ignition points
      m_pFiresListRand = new RandUniform(); //for selecting fireslist
      }
   }

void FlamMapAP::DestroyRandUniforms()
   {
   if (m_pFiresRand)
      delete m_pFiresRand;
   if (m_pIgnitRand)
      delete m_pIgnitRand;
   if (m_pFiresListRand)
      delete m_pFiresListRand;
   m_pFiresRand = m_pIgnitRand = m_pFiresListRand = NULL;
   }

bool FlamMapAP::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   CString msg;
   CString failureID = (_T("FlamMapAP::Init Failed: "));
   EnvExtension::Init(pEnvContext, initStr);
   m_pPolyMapLayer = (MapLayer*)(pEnvContext->pMapLayer);
   ASSERT(m_pPolyMapLayer != NULL);

   m_pPolyGridLkUp = NULL;
   //Report::InfoMsg(_T("Starting FlamMapAP.cpp Init"));


   // Process the init file for params used throughout this plugin
   int retVal = LoadXml(initStr, NULL);
   if (retVal <= 0)
      return FailAndReturn(failureID, " Could not process initialization file");

   Report::LogInfo("Setting scenario description");
   CString desc;
   for (int i = 0; i < (int)m_scenarioArray.GetSize(); i++)
      {
      CString temp;
      temp.Format("%i=%s", m_scenarioArray[i]->m_id, (LPCTSTR)m_scenarioArray[i]->m_name);
      desc += temp;
      if (i < (int)m_scenarioArray.GetSize() - 1)
         desc += ", ";
      }


   Report::LogInfo("Adding input var");
   AddInputVar("Scenario ID", m_scenarioID, desc);

   Report::LogInfo("Creating random generators");
   CreateRandUniforms(true);

   // can be changed by other objects to signal that an error has
   // occurred in this plugin.

   // Init the LCPGenerator LCP values, this provides access to the
   // number of rows and columns in the LCP file. These are used
   // to generate the PolyGrid lookup, which then must be handed
   // to the LCP Generator.

   Report::LogInfo("Creating LCP generator");
   CreateLCPGenerator();

   m_meanFlameLen = m_maxFlameLen = m_burnedAreaHa = m_cumBurnedAreaHa = 0.0f;

   Report::LogInfo("Adding output vars");
   AddOutputVar("MaxFlameLen", m_maxFlameLen, "Maximum flame length from FlamMap run");
   AddOutputVar("MeanFlameLen", m_meanFlameLen, "Mean flame length over burned and unburned areas from a FlamMap run");
   AddOutputVar("Burned Area (ha)", m_burnedAreaHa, "");
   AddOutputVar("Cumulative Burned Area (ha)", m_cumBurnedAreaHa, "");

   m_pFireYearRunner = nullptr;

   Report::LogInfo("Adding output vars");

   // Add col for FlameLen if it is not already there
   CheckCol(m_pPolyMapLayer, m_flameLenCol, "FlameLen", TYPE_FLOAT, CC_AUTOADD);

   // Add col for FireID if it is not already there
   CheckCol(m_pPolyMapLayer, m_fireIDCol, "FireID", TYPE_INT, CC_AUTOADD);

   // Add col for VegClass Variant if it is missing
   CheckCol(m_pPolyMapLayer, m_variantCol, "Variant", TYPE_INT, CC_AUTOADD);

   // Add col for Potential FlameLen if it is not already there
   int col = 0;
   CheckCol(m_pPolyMapLayer, col, "PFlameLen", TYPE_FLOAT, CC_AUTOADD);

   Report::LogInfo("Setting column data");
   m_pPolyMapLayer->SetColData(col, VData(0.0f), true);

   //read in static fires list if selected
   if (this->m_strStaticFireList.GetLength() > 0)
      {
      Report::LogInfo("Reading static fires");
      if (!ReadStaticFires())
         Report::Log(_T("Error reading static fires file, running normally"));
      }

   if (m_logDeltaArrayUpdates)
      {
      Report::LogInfo("Logging flammap deltas");
      m_outputDeltaArrayUpdatesName.Format("%s%d_FlamMapAPDeltaArrayUpdates.csv", (LPCTSTR) m_outputPath, gpFlamMapAP->processID);
      FILE* deltaArrayCSV = fopen(m_outputDeltaArrayUpdatesName, "wt");
      fprintf(deltaArrayCSV, "Yr, Run, IDU, FireID, Flamelength, PFlameLen, CTSS, VegClass, Variant, DISTURB, FUELMODEL\n");
      fclose(deltaArrayCSV);
      }
   Report::Log(_T("Completed FlamMapAP Init successfully"));

   return TRUE;
   }


int FlamMapAP::ReadStaticFires()
   {
   //m_numStaticFires = 0;
   int numStaticFires = 0;
   m_staticFires.clear();
   VDataObj inData(U_UNDEFINED);

   CString fName;
   if (PathManager::FindPath(m_strStaticFireList, fName) < 0)    // search envision paths
      {
      CString msg;
      msg.Format("Unable to locate firelist file '%s' on path", (LPCTSTR) m_strStaticFireList);
      Report::ErrorMsg(msg);
      return 0;
      }

   int recordsTtl = 0;
   if ((recordsTtl = (int)inData.ReadAscii(fName)) <= 0)
      {
      CString msg;
      msg.Format("Error reading static firelist '%s'", (LPCTSTR) fName);
      Report::ErrorMsg(msg);
      return 0;
      }

   int yrCol, probCol, julianCol, burnPerCol,
      windSpeedCol, azimuthCol, fmFileCol, ignitionXCol, ignitionYCol,
      hectaresCol, ercCol, original_SizeCol, fireIDcol;

   int yr, julian, burnPer, windSpeed, azimuth, erc;
   double prob, ignitionX, ignitionY, hectares, original_Size;
   CString fmFile, fireID;

   //use first fire list for location of fms files
   //FIRELIST *pFireList =  m_scenarioArray[ 0 ]->GetFireList( 0 );
   //strcpy(fmsBase, pFireList->m_path);

   TCHAR fmsBase[MAX_PATH];
   lstrcpy(fmsBase, gpFlamMapAP->m_fmsPath);   // this will be termainted with a '\'

   //TCHAR fmsBase[MAX_PATH];
   //char *p = strrchr(fmsBase, '\\');
   //p++;
   //*p = 0;

   yrCol = inData.GetCol("Yr");
   probCol = inData.GetCol("Prob");
   julianCol = inData.GetCol("Julian");
   burnPerCol = inData.GetCol("BurnPer");
   windSpeedCol = inData.GetCol("WindSpeed");
   azimuthCol = inData.GetCol("Azimuth");
   fmFileCol = inData.GetCol("FMFile");
   ignitionXCol = inData.GetCol("IgnitionX");
   ignitionYCol = inData.GetCol("IgnitionY");
   hectaresCol = inData.GetCol("Hectares");
   ercCol = inData.GetCol("ERC");
   original_SizeCol = inData.GetCol("Original_Size");
   fireIDcol = inData.GetCol("fireID");

   if (yrCol < 0 || probCol < 0 || julianCol < 0 || burnPerCol < 0 ||
      windSpeedCol < 0 || azimuthCol < 0 || fmFileCol < 0 || ignitionXCol < 0 || ignitionYCol < 0 ||
      //hectaresCol < 0 || ercCol < 0 || original_SizeCol < 0)
      ercCol < 0 || original_SizeCol < 0)
      {
      //gpFlamMapAP->m_runStatus = 0;
      return 0;
      }
   int fireIDnum = 1;
   for (int i = 0; i < recordsTtl; i++)
      {
      if (!inData.Get(yrCol, i, yr))
         continue;
      if (!inData.Get(probCol, i, prob))
         continue;
      if (!inData.Get(julianCol, i, julian))
         continue;
      if (!inData.Get(burnPerCol, i, burnPer))
         continue;
      if (!inData.Get(windSpeedCol, i, windSpeed))
         continue;
      if (!inData.Get(azimuthCol, i, azimuth))
         continue;
      if (!inData.Get(fmFileCol, i, fmFile))
         continue;
      if (fmFile.GetLength() <= 0)
         continue;
      if (fmFile.GetLength() <= 0)
         continue;
      if (!inData.Get(ignitionXCol, i, ignitionX))
         continue;
      if (!inData.Get(ignitionYCol, i, ignitionY))
         continue;
      if (hectaresCol >= 0)
         {
         if (!inData.Get(hectaresCol, i, hectares))
            continue;
         }
      if (!inData.Get(ercCol, i, erc))
         continue;
      if (!inData.Get(original_SizeCol, i, original_Size))
         continue;

      numStaticFires++;
      }

   if (numStaticFires <= 0)
      {
      return 0;
      }

   //m_staticFires = new Fire[m_numStaticFires];
   m_staticFires.clear();

   int fireNum = 0;

   for (int i = 0; i < recordsTtl; i++)
      {
      if (!inData.Get(yrCol, i, yr))
         continue;
      if (!inData.Get(probCol, i, prob))
         continue;
      if (!inData.Get(julianCol, i, julian))
         continue;
      if (!inData.Get(burnPerCol, i, burnPer))
         continue;
      if (!inData.Get(windSpeedCol, i, windSpeed))
         continue;
      if (!inData.Get(azimuthCol, i, azimuth))
         continue;
      if (!inData.Get(fmFileCol, i, fmFile))
         continue;
      if (fmFile.GetLength() <= 0)
         continue;
      if (!inData.Get(ignitionXCol, i, ignitionX))
         continue;
      if (!inData.Get(ignitionYCol, i, ignitionY))
         continue;
      if (hectaresCol >= 0)
         {
         if (!inData.Get(hectaresCol, i, hectares))
            continue;
         }
      else
         hectares = 0.0;
      if (!inData.Get(ercCol, i, erc))
         continue;
      if (!inData.Get(original_SizeCol, i, original_Size))
         continue;
      if (fireIDcol > 0)
         inData.Get(fireIDcol, i, fireID);
      else
         fireID.Format("%d", fireNum);

      fmFile.Trim();

      CString fmsFull(fmsBase);
      fmsFull += fmFile;

      CString tRecord = "";
      for (int tc = 0; tc < inData.GetColCount(); tc++)
         {
         if (tc > 0)
            tRecord += ",";
         tRecord += inData.GetAsString(tc, i);
         }
      // init Fire info.  Note: years are 1-based in input file, make them zero-based here.
      Fire fire;
      fire.Init(yr - 1, (float)prob, julian, burnPer, windSpeed, azimuth, 0.0f, fmsFull, fmFile, erc, (float)original_Size, (float)ignitionX, (float)ignitionY, fireID, tRecord, m_envisionFireID++);
      m_staticFires.push_back(fire);   //m_staticFires[fireNum].Init(yr - 1, (float)prob, julian, burnPer, windSpeed, azimuth, 0.0f, fmsFull, fmFile, erc, (float)original_Size, (float)ignitionX, (float)ignitionY, fireID, tRecord, m_envisionFireID++);

      //m_staticFires[fireNum].Init(yr - 1, (float)prob, julian, burnPer, windSpeed, azimuth, 0.0f, fmsFull, fmFile, erc, (float)original_Size, (float)ignitionX, (float)ignitionY, fireID, tRecord, m_envisionFireID++);
      // m_staticFires[fireNum].SetIgnitionPoint(ignitionX, ignitionY);

      fireNum++;
      }

   return numStaticFires;
   }

//int  FlamMapAP::Setup(EnvContext* pContext, HWND)
//{
//	CFlamMapAPDlg fDlg(this, (MapLayer*)pContext->pMapLayer);
//	fDlg.DoModal();
//	return IDOK;
//}

bool FlamMapAP::InitRun(EnvContext* pEnvContext, bool useInitSeed)
   {
   CString msg;
   CString nameCStr;
   CString failureID(_T("FlamMapAP::InitRun Failed: "));
   // Get the Scenario directory
   LPCTSTR scenarioName = NULL;

   if (pEnvContext->pEnvModel->m_pScenario != NULL)
      scenarioName = pEnvContext->pEnvModel->m_pScenario->m_name;
   else
      scenarioName = _T("Default");

   //Report::InfoMsg(_T("FlamMapAP::InitRun 010"));
   m_scenarioName = scenarioName;

   if (m_outputEnvisionFirelists)
      {
      //Report::LogInfo("Writing fire lists");
      m_outputEnvisionFirelistName.Format("%sEnvisionFireList_%s_%d.csv", (LPCTSTR)m_outputPath, scenarioName, pEnvContext->runID);
      m_outputEchoFirelistName.Format("%s%d_EchoFireList.csv", (LPCTSTR)m_outputPath, gpFlamMapAP->processID);	   //m_outputEnvisionFirelistName += _T(".csv");
      FILE* envFireList = fopen(m_outputEnvisionFirelistName, "wt");
      fprintf(envFireList, "Yr, Prob, Julian, BurnPer, WindSpeed, Azimuth, FMFile, IgnitionX, IgnitionY, Hectares, ERC, Original_Size, IgnitionFuel, IDU_HA_Burned, IDU_Proportion, FireID, Scenario, Run, ResultCode, FireID, EnvFire_ID, FIL1, FIL2, FIL3, FIL4, FIL5, FIL6, FIL7, FIL8, FIL9, FIL10, FIL11, FIL12, FIL13, FIL14, FIL15, FIL16, FIL17, FIL18, FIL19, FIL20\n");// Julian, WindSpeed, Azimuth, BurnPeriod, Hectares\n");
      fclose(envFireList);
      }

   // jpb - change this to be random selected within a specified scenario
   FMScenario* pScenario = NULL;
   for (int i = 0; i < (int)m_scenarioArray.GetSize(); i++)
      {
      if (m_scenarioArray[i]->m_id == m_scenarioID)
         {
         pScenario = m_scenarioArray[i];
         break;
         }
      }

   if (pScenario == NULL)
      {
      Report::ErrorMsg(" Unable to find specified scenario");
      return FALSE;
      }

   msg = "InitRun retrieving scenario firelist";
   Report::Log(msg);
   // randomly select a fire list
   int selNum = -1;
   if (m_lastScenarioID != m_scenarioID)  // different scenario?  Then reset replicate number
      {
      m_replicateNum = 0;
      m_lastScenarioID = m_scenarioID;
      }
   if (m_sequentialFirelists) // if we are using sequential fire lists, 
      {
      if (m_replicateNum >= pScenario->m_fireListArray.GetSize())
         m_replicateNum = 0;
      selNum = m_replicateNum;
      m_replicateNum++;
      }
   FIRELIST* pFireList = pScenario->GetFireList(selNum);
   m_scenarioFiresFName = pFireList->m_path;
   msg = "Using scenario '";
   msg += pScenario->m_name;
   msg += "', firelist file ";
   msg += m_scenarioFiresFName;
   Report::Log(msg);

   m_scenarioLCPFNameRoot.Format(_T("%s%d_%s"), (LPCTSTR)m_outputPath, gpFlamMapAP->processID, (LPCTSTR)m_lcpFNameRoot);

   m_flameLenCol = pEnvContext->pMapLayer->GetFieldCol("FlameLen");
   m_fireIDCol = pEnvContext->pMapLayer->GetFieldCol("FireID");
   m_vegClassCol = pEnvContext->pMapLayer->GetFieldCol("VegClass");
   m_areaCol = pEnvContext->pMapLayer->GetFieldCol("Area");
   m_fModel_plusCol = pEnvContext->pMapLayer->GetFieldCol("FModelPlus");
   m_regionCol = pEnvContext->pMapLayer->GetFieldCol("Region");
   m_pvtCol = pEnvContext->pMapLayer->GetFieldCol("PVT");
   if (m_logPerimeters)
      {

      }

   // FireYearRunner runs a years worth of fires every timestep
   if (m_pFireYearRunner != NULL)
      {
      delete m_pFireYearRunner;
      m_pFireYearRunner = NULL;
      }

   if (m_logPerimeters)
      {
      Report::LogInfo("Setting up perimeter logging");
      CString PerimeterFName, dbfName;
      //PerimeterFName.Format(_T("%s%d_Perimeter_Run%03d.shp"), m_outputPath, gpFlamMapAP->processID, pEnvContext->runID);
      PerimeterFName.Format(_T("%sFirePerimeter_%s_Run%d.shp"), (LPCTSTR) m_outputPath, (LPCTSTR) pEnvContext->pScenario->m_name, pEnvContext->runID);
      int extLoc = PerimeterFName.ReverseFind('.');
      dbfName = PerimeterFName.Left(extLoc);
      dbfName += _T(".dbf");
      m_hShpPerims = SHPCreate(PerimeterFName, SHPT_POLYGON);
      if (!m_hShpPerims)
         {
         msg.Format("Unable to create perimeter shape file: %s", (LPCTSTR) PerimeterFName);
         Report::ErrorMsg(msg);
         }
      m_hDbfPerims = DBFCreate(dbfName);
      if (!m_hDbfPerims)
         {
         msg.Format("Unable to create perimeter shape file database: %s", (LPCTSTR)dbfName);
         Report::ErrorMsg(msg);
         }
      else
         {
         char FILname[16];
         yearField = DBFAddField(m_hDbfPerims, "YEAR", FTInteger, 8, 0);
         runField = DBFAddField(m_hDbfPerims, "RUN", FTInteger, 8, 0);
         hectareField = DBFAddField(m_hDbfPerims, "AREA_HA", FTDouble, 16, 2);
         ercField = DBFAddField(m_hDbfPerims, "ERC", FTInteger, 8, 0);
         xCoordField = DBFAddField(m_hDbfPerims, "Xcoord", FTDouble, 18, 6);
         yCoordField = DBFAddField(m_hDbfPerims, "Ycoord", FTDouble, 18, 6);
         burnPerField = DBFAddField(m_hDbfPerims, "BurnPer", FTInteger, 16, 0);
         azimuthField = DBFAddField(m_hDbfPerims, "Azimuth", FTInteger, 16, 0);
         windSpeedField = DBFAddField(m_hDbfPerims, "WindSpeed", FTInteger, 16, 0);
         fmsField = DBFAddField(m_hDbfPerims, "FMFile", FTString, 255, 0);
         originalSizeField = DBFAddField(m_hDbfPerims, "OrigSize", FTDouble, 18, 6);
         fireIDField = DBFAddField(m_hDbfPerims, "fireID", FTString, 255, 0);
         envisionFireIDField = DBFAddField(m_hDbfPerims, "EnvFire_ID", FTInteger, 16, 0);
         for (int f = 0; f < 20; f++)
            {
            sprintf(FILname, "FIL%d", f + 1);
            FILfields[f] = DBFAddField(m_hDbfPerims, FILname, FTDouble, 8, 6);
            }
         DBFClose(m_hDbfPerims);
         m_hDbfPerims = DBFOpen(dbfName, "rb+");
         if (!m_hDbfPerims)
            {
            msg.Format("Unable to open perimeter shape file database: %s", (LPCTSTR)dbfName);
            Report::ErrorMsg(msg);
            }
         }
      }

   Report::LogInfo("Creating Fire Year Runner");

   m_pFireYearRunner = new FireYearRunner(pEnvContext);
   FailAndReturnOnBadStatus(m_runStatus, failureID, "Failed to initialize FireYearRunner");
   Report::LogInfo("Successfully created Fire Year Runner");

   // intialize output variables
   this->m_maxFlameLen = this->m_meanFlameLen = this->m_burnedAreaHa = this->m_cumBurnedAreaHa = 0;

   Report::LogInfo("Flammap run initialization complete");

   return TRUE;
   } // bool FlamMapAP::InitRun( EnvContext *pEnvContext )


bool FlamMapAP::Run(EnvContext* pEnvContext)
   {

   //  Report::Log("entering FlamMapAP::Run()");
   CString runID;
   CString msg;
   CString failureID = (_T("FlamMapAP::Run Failed:"));

   ASSERT(pEnvContext != NULL);
   ASSERT(m_pPolyGridLkUp != NULL);

   m_pEnvContext = pEnvContext;

   m_pLcpGenerator->PrepLandscape(pEnvContext);
   int nFiresBurned = m_pFireYearRunner->RunFireYear(pEnvContext, m_pPolyGridLkUp);
   FailAndReturnOnBadStatus(m_runStatus, failureID, "m_pFireYearRunner->RunFireYear failed");

   // write values to delta array
   m_flameLengthUpdater.UpdateDeltaArray(this, pEnvContext, m_pFireYearRunner, m_pPolyGridLkUp);

   return TRUE;
   } // bool FlamMapAP::Run( EnvContext *pEnvContext )


bool FlamMapAP::EndRun(EnvContext* pContext)
   {
   if (m_logPerimeters)
      {
      if (m_hShpPerims)
         {
         SHPClose(m_hShpPerims);
         m_hShpPerims = NULL;
         }
      if (m_hDbfPerims)
         {
         DBFClose(m_hDbfPerims);
         m_hDbfPerims = NULL;
         }
      }
   if (m_pFireYearRunner != NULL)
      delete m_pFireYearRunner;

   m_pFireYearRunner = NULL;
   return true;
   }


bool FlamMapAP::LoadXml(LPCTSTR filename, MapLayer* pLayer)
   {
   // search for file along path
   CString path;
   int result = PathManager::FindPath(filename, path);  // finds first full path that contains passed in path if relative or filename, return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 

   if (result < 0)
      {
      CString msg("Unable to find input file ");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   TiXmlDocument doc;
   bool ok = doc.LoadFile(path);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   int scenarioCount = 0;
   int firelistCount = 0;

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // flammap

   XML_ATTR rootAttrs[] = {
      // attr                         type          address                     isReq  checkCol

      // the following are deprecated
      { "polyGridReadFromFile",        TYPE_INT,     &m_polyGridReadFromFile,    false, 0 },
      { "polyGridSaveToFile",          TYPE_INT,     &m_polyGridSaveToFile,      false, 0 },
      { "polyGridReadFileName",        TYPE_CSTRING, &m_polyGridReadFName,       false, 0 },
      { "polyGridSaveFileName",        TYPE_CSTRING, &m_polyGridSaveFName,       false, 0 },

      // use this instead
      { "polyGridFileName",            TYPE_CSTRING, &m_polyGridFName,           true,  0 },  // uses paths

     { "logFlameLengths", TYPE_INT, &m_logFlameLengths, false, 0 },
     { "logArrivalTimes", TYPE_INT, &m_logArrivalTimes, false, 0 },
     { "logPerimeters", TYPE_INT, &m_logPerimeters, false, 0 },
     { "logDeltaArrayUpdates", TYPE_INT, &m_logDeltaArrayUpdates, false, 0 },
     { "startingLCPFileName", TYPE_CSTRING, &m_startingLCPFName, true, 0 },  // uses paths
      { "outputPath",                  TYPE_CSTRING, &m_outputPath,              true,  0 },  // appended to studyarea folder
      { "fmsPath",                     TYPE_CSTRING, &m_fmsPath,                 true,  0 },
      { "vegClassLCPLookupFileName",   TYPE_CSTRING, &m_vegClassLCPLookupFName,  true,  0 },   // uses paths
      { "vegClassHeightUnits",         TYPE_INT,     &m_vegClassHeightUnits,     true,  0 },
      { "vegClassBulkDensUnits",       TYPE_INT,     &m_vegClassBulkDensUnits,   true,  0 },
      { "vegClassCanopyCoverUnits",    TYPE_INT,     &m_vegClassCanopyCoverUnits,true,  0 },
      { "iduBurnPercentThreshold",     TYPE_DOUBLE,  &m_iduBurnPercentThreshold, false, 0 },
      { "spotProbability",             TYPE_DOUBLE,  &m_spotProbability,         false, 0 },
      { "crownFireMethod",             TYPE_INT,     &m_crownFireMethod,         false, 0 },
      { "foliarMoistureContent",       TYPE_INT,     &m_foliarMoistureContent,   false, 0 },
      { "outputEnvisionFirelists",     TYPE_INT,     &m_outputEnvisionFirelists, false, 0 },
      { "outputIgnitionProbabilities", TYPE_INT,     &m_outputIgnitProbGrids,    false, 0 },
      { "deleteLCPs",                  TYPE_INT,     &m_deleteLCPs,              false, 0 },
      { "customFuelFileName",          TYPE_CSTRING, &m_customFuelFileName,      false, 0 },     // uses paths
      { "randomSeed",                  TYPE_INT,     &m_randomSeed,              false, 0 },
      { "maxProcessors",               TYPE_INT,     &m_maxProcessors,           false, 0 },
      { "useExistingFireList",         TYPE_CSTRING, &m_strStaticFireList,       false, 0 },     // uses paths (in FireYearRunner)
      { "staticFMS",                   TYPE_CSTRING, &m_strStaticFMS,            false, 0 },     // uses paths
      { "staticWindSpeed",             TYPE_INT,     &m_staticWindSpeed,         false, 0 },
     { "staticWindDirection",		   TYPE_INT,     &m_staticWindDir,           false, 0 },
     { "useFirelistIgnitions",			TYPE_INT,     &m_useFirelistIgnitions,    false, 0 },
     { "minFireSizeRatio",				TYPE_DOUBLE,	&m_FireSizeRatio,			false, 0 },
     { "logAnnualFlameLengths",		TYPE_INT,		&m_logAnnualFlameLengths,	false, 0 },
     { "sequentialFirelists",			TYPE_INT,		&m_sequentialFirelists,		false, 0 },
     { "maxFireRetries", TYPE_INT, &m_maxRetries, false, 0 },
     { NULL, TYPE_NULL, NULL, false, 0 } };

   ok = TiXmlGetAttributes(pXmlRoot, rootAttrs, filename, pLayer);
   if (!ok)
      {
      CString msg;
      msg.Format(_T("Misformed root element reading <flammap> attributes in FlammapAP input file %s"), filename);
      Report::ErrorMsg(msg);
      return false;
      }

   // make sure paths are slash-terminated
   CPath oPath = m_outputPath;
   if (oPath.IsRelative())
      {
      CString outputPath = PathManager::GetPath(PM_IDU_DIR);   // slash terminated, assumes directory is off of study area
      outputPath += m_outputPath;
      m_outputPath = outputPath;
      }
   if (m_outputPath[m_outputPath.GetLength() - 1] != '\\' && m_outputPath[m_outputPath.GetLength() - 1] != '/')
      m_outputPath += "\\";

   if (m_fmsPath[m_fmsPath.GetLength() - 1] != '\\' && m_fmsPath[m_fmsPath.GetLength() - 1] != '/')
      m_fmsPath += "\\";

   // fix up paths
   ResolvePath(m_startingLCPFName, "Unable to find starting LCP file");
   ResolvePath(m_vegClassLCPLookupFName, "Unable to find veg class LCP lookup file");
   ResolvePath(m_customFuelFileName, "Unable to find custom fuel file");
   ResolvePath(m_strStaticFMS, "Unable to find static FMS file");

   // Done with root elements, get scenarios  
   TiXmlElement* pXmlScenarios = pXmlRoot->FirstChildElement("scenarios");
   if (pXmlScenarios == NULL)
      {
      CString msg;
      msg.Format(_T("Error reading FlammapAP input file %s: Missing <scenarios> entry.  This is required to continue..."), filename);
      Report::ErrorMsg(msg);
      return false;
      }

   TiXmlElement* pXmlScenario = pXmlScenarios->FirstChildElement("scenario");
   if (pXmlScenario == NULL)
      {
      CString msg;
      msg.Format(_T("Error reading FlammapAP input file %s: Missing <scenario> entry.  This is required to continue..."), filename);
      Report::ErrorMsg(msg);
      return false;
      }

   while (pXmlScenario != NULL)
      {
      FMScenario* pScenario = NULL;

      CString name;
      int id = 0;
      XML_ATTR attrs[] = {
         // attr             type            address    isReq checkCol
         { "id",             TYPE_INT,       &id,       true,   0 },
         { "name",           TYPE_CSTRING,   &name,     true,   0 },
         { NULL,             TYPE_NULL,      NULL,      false,  0 }
         };

      bool ok = TiXmlGetAttributes(pXmlScenario, attrs, filename);

      if (ok)
         {
         pScenario = new FMScenario;
         pScenario->m_name = name;
         pScenario->m_id = id;
         this->m_scenarioArray.Add(pScenario);
         scenarioCount++;
         }

      // have scenario, get fire list(s)
      TiXmlElement* pXmlFireList = pXmlScenario->FirstChildElement("firelist");
      if (pXmlFireList == NULL)
         {
         CString msg;
         msg.Format(_T("Error reading FlammapAP input file %s: Missing <firelist> entry.  This is required to continue..."), filename);
         Report::ErrorMsg(msg);
         return false;
         }

      while (pXmlFireList != NULL)
         {
         CString name;
         XML_ATTR attrs[] = {
            // attr             type            address    isReq checkCol
            { "name",           TYPE_CSTRING,   &name,     true,   0 },
            { NULL,             TYPE_NULL,      NULL,      false,  0 }
            };

         bool ok = TiXmlGetAttributes(pXmlFireList, attrs, filename);

         if (ok)
            {
            CString path;
            int retVal = PathManager::FindPath(name, path);

            if (retVal < 0)
               {
               CString msg;
               msg.Format("Unable to find firelist file '%s'", (LPCTSTR) name);
               Report::ErrorMsg(msg);
               }
            else
               {
               CString msg;
               msg.Format("Scenario '%s': Adding Firelist file '%s'", (LPCTSTR)pScenario->m_name, (LPCTSTR)path);
               Report::Log(msg);

               pScenario->AddFireList(new FIRELIST(path));
               firelistCount++;
               }
            }

         pXmlFireList = pXmlFireList->NextSiblingElement("firelist");
         }

      pXmlScenario = pXmlScenario->NextSiblingElement("scenario");
      }

   // done with scenarios

   //form the lcp root name for output lcp generation 
   //m_lcpFNameRoot = "";
   //char tmp[MAX_PATH], * slash, * dot;
   //strcpy(tmp, m_startingLCPFName);
   //slash = strrchr(tmp, '\\');
   //slash++;
   //dot = strrchr(slash, '.');
   //*dot = 0;
   //m_lcpFNameRoot = slash;
   nsPath::CPath lcpPath(m_startingLCPFName);
   m_lcpFNameRoot = lcpPath.GetTitle();

   //ensure outputPath exists!
   //int dirRet = SHCreateDirectoryEx(NULL, m_outputPath, NULL);
   //switch (dirRet)
   //   {
   //   case ERROR_SUCCESS:
   //   case ERROR_FILE_EXISTS:
   //   case ERROR_ALREADY_EXISTS:
   //      break;
   //   case ERROR_BAD_PATHNAME:
   //   case ERROR_PATH_NOT_FOUND:
   //   case ERROR_FILENAME_EXCED_RANGE:
   //   default:
   //   {
   //   CString msg;
   //   msg.Format(_T("FlamMapAP outputDirectory can not be created"));
   //   Report::ErrorMsg(msg);
   //   return false;
   //   }
   //   }


   CString msg;
   msg.Format("Loaded %i scenarios, %i firelists from input file '%s'", scenarioCount, firelistCount, (LPCTSTR) path);
   Report::Log(msg);

   return true;
   }



bool FailAndReturn(LPCTSTR failureID, LPCTSTR failMsg)
   {
   CString msg(failureID);
   msg += (_T(failMsg));
   Report::ErrorMsg(msg);
   return FALSE;
   }

bool FailAndReturnOnBadStatus(int runStatus, LPCTSTR failureID, LPCTSTR failMsg)
   {
   if (!runStatus)
      {
      CString msg(failureID);
      msg += (_T(failMsg));
      Report::ErrorMsg(msg);
      return FALSE;
      }

   return FALSE;
   }



bool FlamMapAP::LoadPolyGridLookup(MapLayer* pIDULayer)
   {
   TRACE(_T("Starting FlammapAP.cpp LoadPolyGridLookup.\n"));

   CString pglPath;
   CString msg;

   // set path for .pgl file (bin file with poly <-> grid relationship
   //pglFile.Format( _T( "/Envision/StudyAreas/CentralOregon/North/TerrBiodiv%im.pgl" ), GetGridCellSize()  );

   // status: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
   int status = PathManager::FindPath(m_polyGridFName, pglPath);    // look along Envision paths

   // if .pgl file exists: read poly - grid - lookup table from file
   if (status >= 0)
      {
      msg = _T("Loading PolyGrid from File: ");
      msg += pglPath;
      Report::Log(msg);

      //Report::Log(_T("Creating PolyGrid - Starting"));
      m_pPolyGridLkUp = new PolyGridLookups(pglPath);
      //Report::Log(_T("Creating PolyGrid - Finished"));
      }
   else   // else (if .pgl file does NOT exists): create poly - grid - lookup table and write to .pgl file
      {
      int maxEls = m_rows * m_cols * 2;

      Report::Log(_T("Creating PolyGrid from Data - Starting"));
      m_pPolyGridLkUp = new PolyGridLookups(
         m_ptLayer,
         pIDULayer,
         9,               // int cellSubDivisions,
         maxEls,          // int maxElements,
         0,               // int defaultVal,
         -9999);          // int nullVal

      Report::Log(_T("Generating PolyGrid from Data - Finished"));

      pglPath = m_polyGridFName;

      // write to file.  If the name is full qualified, use it.  Otherwise, use the IDU directory
      //if ( pglPath.Find( '\\' ) < 0 && pglPath.Find( '/' ) < 0 )     // no slashes specfied? 
      if (pglPath[1] != ':' && pglPath[0] != '\\' && pglPath[0] != '/')
         {
         pglPath = PathManager::GetPath(PM_IDU_DIR);
         pglPath += m_polyGridFName;
         }

      msg = _T("Writing PolyGrid to File: ");
      msg += pglPath;
      Report::Log(msg);
      m_pPolyGridLkUp->WriteToFile(pglPath);

      Report::Log(_T("Writing PolyGrid to File - Finished"));
      }

   msg.Format("Polygrid Rows=%i, Cols=%i", m_pPolyGridLkUp->GetNumGridRows(), m_pPolyGridLkUp->GetNumGridCols());
   Report::Log(msg);
   return TRUE;
   }


int FlamMapAP::ResolvePath(CString& filename, LPCTSTR errorMsg)
   {
   CString path;
   int retVal = PathManager::FindPath(filename, path);
   if (retVal < 0)
      {
      CString msg;
      msg.Format("%s '%s' when searching paths", errorMsg, (LPCTSTR)filename);
      Report::ErrorMsg(msg);
      }
   else
      {
      CString msg;
      msg.Format("Resolving file '%s' to path '%s'", (LPCTSTR)filename, (LPCTSTR)path);
      Report::Log(msg);

      filename = path;
      }

   return retVal;
   }

int AdjustVerticesToPreventSelfIntersection(double adj, int nVertices, double* fX, double* fY)
   {
   int nAdjust = 0;
   for (int p1 = 1; p1 < nVertices - 1; p1++)
      {
      for (int p2 = p1 + 1; p2 < nVertices - 1; p2++)
         {
         if (fX[p1] == fX[p2] && fY[p1] == fY[p2]) //overlap caused by crossing same corner
            {
            if (fX[p1 - 1] == fX[p1]) //moving vertically
               {
               if (fY[p1 - 1] > fY[p1])//moving down
                  {
                  fY[p1] += adj;
                  }
               else
                  {//moving up
                  fY[p1] -= adj;
                  }
               }
            else if (fY[p1 - 1] == fY[p1])//moving horizontally
               {
               if (fX[p1 - 1] > fX[p1])
                  fX[p1] += adj;
               else
                  fX[p1] -= adj;
               }
            else //crossing diagonally (second point of a cross-over)
               {
               if (fX[p1 + 1] == fX[p1])//headed vertically
                  {
                  if (fY[p1 + 1] > fY[p1])
                     fY[p1] += adj;
                  else
                     fY[p1] -= adj;
                  }
               else//headed horizontally
                  {
                  if (fX[p1 + 1] > fX[p1])
                     fX[p1] += adj;
                  else
                     fX[p1] -= adj;
                  }
               }
            nAdjust++;
            }
         }
      }
   return nAdjust;
   }
void FlamMapAP::WritePerimeterShapefile(CPerimeter* perim, int _year, int _run, int _erc, double _area_ha, double _resolution,
   double originalSize,
   double xCoord,
   double yCoord,
   int burnPer,
   int azimuth,
   int windSpeed,
   char* fms,
   char* fireID,
   int envisionFireID,
   double* FIL)

   //void FlamMapAP::WritePerimeterShapefile(CPerimeter *perim, int _year, int _run, int _erc, double _area_ha, double _resolution)
   {
   if (m_hDbfPerims && m_hShpPerims)
      {

      int nParts = perim->GetNumLines();
      int* partStarts = new int[nParts];
      int* partTypes = new int[nParts];
      int loc = 0, lineNum = 0;
      for (int l = 0; l < nParts; l++)
         {
         partStarts[l] = loc;
         loc += perim->GetLine(l)->GetNumPoints();// fire->perims[l].numPoints;
         partTypes[l] = SHPP_RING;
         }
      int totalVertices = loc, p = 0;
      double* padfX = new double[totalVertices];
      double* padfY = new double[totalVertices];
      double* padfZ = new double[totalVertices];
      double* padfM = new double[totalVertices];
      loc = 0;
      float tX, tY;
      for (int f = 0; f < perim->GetNumLines(); f++)
         {
         for (int p = 0; p < perim->GetLine(f)->GetNumPoints(); p++)
            {
            perim->GetLine(f)->GetPoint(p, &tX, &tY);
            padfX[loc] = tX;// fire->perims[f].points[p].x;
            padfY[loc] = tY;// fire->perims[f].points[p].y;
            padfZ[loc] = 1;//
            padfM[loc] = 2;
            loc++;
            }
         }
      double adjust = _resolution / 100.0;
      int nAdj = AdjustVerticesToPreventSelfIntersection(adjust, totalVertices, padfX, padfY);
      SHPObject* pShape = SHPCreateObject(SHPT_POLYGON, -1, nParts, partStarts, partTypes, totalVertices,
         padfX, padfY, padfZ, padfM);
      int entity = SHPWriteObject(m_hShpPerims, -1, pShape);
      // find erc percentile, starting with the 80th and working up
      DBFWriteIntegerAttribute(m_hDbfPerims, entity, yearField, _year);
      DBFWriteIntegerAttribute(m_hDbfPerims, entity, runField, _run);
      DBFWriteIntegerAttribute(m_hDbfPerims, entity, ercField, _erc);
      DBFWriteDoubleAttribute(m_hDbfPerims, entity, hectareField, _area_ha);
      DBFWriteDoubleAttribute(m_hDbfPerims, entity, originalSizeField, originalSize);
      DBFWriteDoubleAttribute(m_hDbfPerims, entity, xCoordField, xCoord);
      DBFWriteDoubleAttribute(m_hDbfPerims, entity, yCoordField, yCoord);
      DBFWriteIntegerAttribute(m_hDbfPerims, entity, burnPerField, burnPer);
      DBFWriteIntegerAttribute(m_hDbfPerims, entity, azimuthField, azimuth);
      DBFWriteIntegerAttribute(m_hDbfPerims, entity, windSpeedField, windSpeed);
      DBFWriteStringAttribute(m_hDbfPerims, entity, fmsField, fms);
      DBFWriteStringAttribute(m_hDbfPerims, entity, fireIDField, fireID);
      DBFWriteIntegerAttribute(m_hDbfPerims, entity, envisionFireIDField, envisionFireID);
      for (int f = 0; f < 20; f++)
         DBFWriteDoubleAttribute(m_hDbfPerims, entity, FILfields[f], FIL[f]);
      SHPDestroyObject(pShape);
      delete[] padfX;
      delete[] padfY;
      delete[] padfZ;
      delete[] padfM;
      delete[] partStarts;
      delete[] partTypes;
      }
   }


bool FlamMapAP::CreateLCPGenerator()
   {
   CString failureID = (_T("FlamMapAP::CreateLCPGenerator Failed: "));
   CString msg;
   int status = 0;
   m_pLcpGenerator = new LCPGenerator();
   if (!m_pLcpGenerator->InitLCPValues(m_startingLCPFName))
      return FailAndReturn(failureID, " LCP initialization failed.") ? true : false;
   else
      Report::Log(_T("LCP Initialized"));

   // Write LCP Header to ASCII file for checking
   if (!m_pLcpGenerator->InitVegClassLCPLookup(m_vegClassLCPLookupFName))
      return FailAndReturn(failureID, " VegClassLCPLookup initialization failed.") ? true : false;
   else
      Report::Log(_T("VegClassLCPLookup Initialized"));

   REAL xMin = 0;
   REAL xMax = 0;
   REAL yMin = 0;
   REAL yMax = 0;

   m_pPolyMapLayer->GetExtents(xMin, xMax, yMin, yMax);
   REAL xExtent = xMax - xMin;
   REAL yExtent = yMax - yMin;
   m_cellDim = m_pLcpGenerator->GetCellDim();
   m_rows = (int)ceil(yExtent / m_cellDim);
   m_cols = (int)ceil(xExtent / m_cellDim);

   msg.Format(_T("Params from LCP file: Rows: %d  Cols: %d  CellDim: %f"), m_rows, m_cols, m_cellDim);
   Report::Log(msg);

   m_pMap = m_pPolyMapLayer->GetMapPtr();
   ASSERT(m_pMap != NULL);

   m_ptLayer = new FAGrid(m_pMap);

   ASSERT(m_ptLayer != NULL);

   int gridRslt = m_ptLayer->CreateGrid(
      m_rows,
      m_cols,
      xMin,//lcpWest,//xMin - xOffset,
      yMin,//lcpSouth,//yMin - yOffset, 
      (float)m_cellDim,
      -9999,
      DOT_INT,
      FALSE,
      FALSE);

   ASSERT(gridRslt);

   // Initializing PolyGridLookup
   if (!LoadPolyGridLookup(m_pPolyMapLayer))
      return FailAndReturn(failureID, " LoadPolyGridLookup failed.") ? true : false;

   // Set the PolyGrid Lookup in the LCPGenerator
   ASSERT(m_pPolyGridLkUp != NULL);
   m_pLcpGenerator->InitPolyGridLookup(m_pPolyGridLkUp);
   if (status >= 0)
      return true;
   return false;
   }