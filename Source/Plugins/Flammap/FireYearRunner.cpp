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
/* FireYearRunner.cpp - see .h file for details
**
** 2011.03.23 - tjs - added error handling
*/


#include "stdafx.h"
#pragma hdrstop

#include "MinTravelTime.h"
#include "FireYearRunner.h"
//#include "ParamLookup.h"
#include "FlammapAP.h"
#include <MapLayer.h>
#include <Report.h>

#include <MAP.h>
#include <UNITCONV.H>


//#include <MAP.h>
#include <stdio.h>
#include <cstring>
#include <cctype>
#include <math.h>

extern FlamMapAP* gpFlamMapAP;
extern int Round(double in);
// For logging and debugging
//#include "TLogger.h"
//extern TLogger g_MyLog;

extern FlamMapAP* gpFlamMapAP;

// Flag for debugging
#define DBG_FIRE_YEAR_RUNNER

const int RESULT_MTT_OK = 1;
const int RESULT_IGNIT_OFF_LCP = -1;
const int RESULT_IGNIT_NON_BURNABLE = -2;
const int RESULT_NO_BURN = -3;

FireYearRunner::FireYearRunner(EnvContext* pEnvContext)
	{
	// FILE *inFile;
	 //  char  inBuf[2048];
	//m_pFireNums = NULL;
	//m_pYrFlameLens = NULL;

	m_pEnvContext = pEnvContext;

	//if (!gpFlamMapAP->m_staticFires)
	if (gpFlamMapAP->m_staticFires.size() == 0)
		{
		Report::Log_s("Initializing firelist for %s", gpFlamMapAP->m_scenarioFiresFName);
		m_firesList.Init(gpFlamMapAP->m_scenarioFiresFName);
		}

	// Make sure the run hasn't gone bad somewhere else
	if (gpFlamMapAP->m_runStatus < 0)
		return;

	CString igGenIntFName;

	Report::LogInfo("Initializing ignition generator");
	m_pIgnitGenerator = new IgnitGenerator(m_pEnvContext);

	//m_pYrFlameLens = new float[gpFlamMapAP->m_rows * gpFlamMapAP->m_cols];
	//m_pFireNums = new int[gpFlamMapAP->m_rows * gpFlamMapAP->m_cols];
	m_yrFlameLens.resize(gpFlamMapAP->m_rows * gpFlamMapAP->m_cols, 0);  // m_pYrFlameLens = new float[gpFlamMapAP->m_rows * gpFlamMapAP->m_cols];
	m_fireNums.resize(gpFlamMapAP->m_rows * gpFlamMapAP->m_cols, 0);    // = new int[gpFlamMapAP->m_rows * gpFlamMapAP->m_cols];

	} // FireYearRunner::FireYearRunner()


bool FuelIsNonburnable(int fuel)
	{
	if (fuel <= 0 || fuel > 256)
		return true;
	if (fuel >= 90 && fuel <= 99)
		return true;
	return false;
	}

void getNewIgnitionPoint(CFlamMap* pFlamMap, RandUniform* pIgnitRand, double originalX, double originalY, double* x, double* y)
	{//modified 10/25/2017 to restrict to within 10Km of original fire location
	double newX, newY, dist;
	double minX = max(originalX - 10000.0, pFlamMap->GetWest());
	double maxX = min(originalX + 10000.0, pFlamMap->GetEast());
	double minY = max(originalY - 10000.0, pFlamMap->GetSouth());
	double maxY = min(originalY + 10000.0, pFlamMap->GetNorth());

	newX = minX + ((maxX - minX) * pIgnitRand->GetUnif01());
	newY = minY + ((maxY - minY) * pIgnitRand->GetUnif01());
	dist = ((newX - originalX) * (newX - originalX)) + ((newY - originalY) * (newY - originalY));
	if (dist > 0.0)
		dist = sqrt(dist);
	int fuel = (int)pFlamMap->GetLayerValue(FUEL, newX, newY);
	int maxTries = 1000; // if can't find a burnable fuel within 10KM in 1000 tries bail
	int tries = 0;
	while ((FuelIsNonburnable(fuel) || dist > 10000.0) && tries < maxTries)
		{
		//get ignition point for fire
		newX = minX + ((maxX - minX) * pIgnitRand->GetUnif01());
		newY = minY + ((maxY - minY) * pIgnitRand->GetUnif01());
		dist = ((newX - originalX) * (newX - originalX)) + ((newY - originalY) * (newY - originalY));
		if (dist > 0.0)
			dist = sqrt(dist);
		fuel = (int)pFlamMap->GetLayerValue(FUEL, newX, newY);
		tries++;
		}
	*x = newX;
	*y = newY;
	}


// main entry point to running fires for the current year
int FireYearRunner::RunFireYear(EnvContext* pEnvContext, PolyGridLookups* pPolyGridLookups)
	{
	CString msg;
	CString outFName;
	CString outStr;
	CString tmpStr;

	int fireCnt = 0;
	//int year = pEnvContext->currentYear;
	int year = pEnvContext->yearOfRun;
	//int numOmpProcs = omp_get_num_procs();

	__int64 nRows = gpFlamMapAP->m_rows;
	__int64 nCols = gpFlamMapAP->m_cols;

	// reset this year's flame lengths
	for (int i = 0; i < (nRows * nCols); i++)
		{
		//m_pYrFlameLens[i] = 0.0;
		//m_pFireNums[i] = 0;
		m_yrFlameLens.at(i) = 0.0;
		m_fireNums.at(i) = 0;
		}

	// make an instance of the FlamMap wrapper and give it a LCP file
	CFlamMap tFlamMap;
	int ret = tFlamMap.SetLandscapeFile((TCHAR*)(LPCTSTR)gpFlamMapAP->m_lcpFName);
	if (ret != 1)
		{
		msg.Format(_T("Error: FlamMap MTT can't open landscape file, %s! Aborting..."),
			(LPCSTR)gpFlamMapAP->m_lcpFName);
		Report::ErrorMsg(msg);
		return 0;
		}

	// Flammap was successfully created, initialized, get ready to run fires
	// load custom fuel file if defined in input file
	if (gpFlamMapAP->m_customFuelFileName.GetLength() > 0)
		{
		ret = tFlamMap.SetCustomFuelsFile((char*)(LPCTSTR)gpFlamMapAP->m_customFuelFileName);
		if (ret != 1)
			{
			msg.Format(_T("Error: FlamMap MTT can't open custom fuels file, %s! Aborting..."),
				(LPCSTR)gpFlamMapAP->m_customFuelFileName);
			Report::ErrorMsg(msg);
			return 0;
			}
		}
	// There are two operating modes:
	//   1) Static fire behavior, which used the same fuel moisture, windirection and windspeeds
	//   2) 

	if (gpFlamMapAP->m_strStaticFMS.GetLength() > 0 && gpFlamMapAP->m_staticWindDir >= 0 && gpFlamMapAP->m_staticWindSpeed >= 0)
		{
		//static run of annual basic fire behavior
		TCHAR fms[MAX_PATH];
		lstrcpy(fms, gpFlamMapAP->m_strStaticFMS);


		ret = tFlamMap.SetFuelMoistureFile((char*)(LPCTSTR)gpFlamMapAP->m_strStaticFMS);
		if (ret != 1)
			{
			msg.Format("Error loading static FMS file %s\n\t%s", gpFlamMapAP->m_strStaticFMS, tFlamMap.CommandFileError(ret));
			Report::ErrorMsg(msg);
			}
		else
			{
			// set Flammap parameters
			tFlamMap.SetConstWind(gpFlamMapAP->m_staticWindSpeed, gpFlamMapAP->m_staticWindDir);
			tFlamMap.SetFoliarMoistureContent(gpFlamMapAP->m_foliarMoistureContent);
			tFlamMap.SetUseScottReinhardt(gpFlamMapAP->m_crownFireMethod);
			tFlamMap.SelectOutputLayer(CROWNSTATE, true);
			tFlamMap.SelectOutputLayer(FLAMELENGTH, true);

			// run Flammap once to generate potential fire behavior
			ret = tFlamMap.LaunchFlamMap(0);

			if (ret != 1)
				{
				msg.Format("Error running static FlamMap: %s", tFlamMap.CommandFileError(ret));
				Report::ErrorMsg(msg);
				}

			// output Potential Flame length and Crown Fire activity layers
			CString pflName;
			pflName.Format("%s%d_FlameLength_%03d_%04d.asc", gpFlamMapAP->m_outputPath, gpFlamMapAP->processID, pEnvContext->runID, year);
			tFlamMap.WriteOutputLayerToDisk(FLAMELENGTH, (char*)pflName.GetString());

			CString msg;
			msg.Format("Writing potential flame length grid to %s", (LPCTSTR)pflName);
			Report::StatusMsg(msg);

			CString cfName;
			cfName.Format("%s%d_CrownFire_%03d_%04d.asc", gpFlamMapAP->m_outputPath, gpFlamMapAP->processID, pEnvContext->runID, year);
			tFlamMap.WriteOutputLayerToDisk(CROWNSTATE, (char*)cfName.GetString());
			msg.Format("Writing crown fire grid to %s", (LPCTSTR)cfName);
			Report::StatusMsg(msg);

			// reload potential flame length as ASCII grids 

			MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;
			Map* pMap = pLayer->GetMapPtr();

			msg.Format("Adding potential flame length grid %s to Envision", (LPCTSTR)pflName);
			Report::StatusMsg(msg);
			MapLayer* pGrid = pMap->AddGridLayer(pflName, DOT_FLOAT, -1, 10, BCF_RED_INCR);

			if (pGrid)
				{
				Report::StatusMsg("Flammap:: Setting potential flame length bins");

				int colPotFlameLen = pLayer->GetFieldCol("PFLAMELEN");
				pGrid->SetBins(pLayer, colPotFlameLen);

				Report::StatusMsg("Flammap:: Success adding potential flame length grid");

				// loaded grid, write grid results to IDUs 
				CalcIduPotentialFlameLen(pEnvContext, pGrid);

				pMap->RemoveLayer(pGrid, true);
				Report::StatusMsg("Flammap:: Success removing potential flame length grid");
				}
			}
		}  // end of: run static behavior

	int firesToDo = 0;
	//if (!gpFlamMapAP->m_staticFires)
	if (gpFlamMapAP->m_staticFires.size() == 0)
		{
		if (!gpFlamMapAP->m_useFirelistIgnitions)
			{
			m_pIgnitGenerator->UpdateProbArray(pEnvContext, pPolyGridLookups, &tFlamMap);
			double pRatio = m_pIgnitGenerator->GetIgnitRatio();
			m_firesList.AdjustYear(year, pRatio);
			msg.Format(_T("Adjusting fire probabilities for year %d by %lf"), year, pRatio);
			Report::Log(msg);
			}
		FireList::iterator i;
		int count = 0;
		for (i = m_firesList.firesList.begin(); i != m_firesList.firesList.end(); ++i)
			{
			Fire fire = *i;
			if (fire.GetYr() == year && fire.GetDoRun())
				firesToDo++;
			}
		}
	else     //running with static fires
		{
		firesToDo = 0;
		//for (int i = 0; i < gpFlamMapAP->m_numStaticFires; i++)
		for (int i = 0; i < gpFlamMapAP->m_staticFires.size(); i++)
			{
			if (gpFlamMapAP->m_staticFires[i].GetYr() == year)
				firesToDo++;
			}
		}

	if (firesToDo <= 0)
		{
		msg.Format("No fires selected for year %d (%d)", year, pEnvContext->currentYear);
		Report::Log(msg);
		return 0;
		}

	// create the fires.  The "Fire" class represents info about the fire
	// these are the fires we will run
	Fire* fireArray = new Fire[firesToDo];
	//if (gpFlamMapAP->m_staticFires)
	if (gpFlamMapAP->m_staticFires.size() > 0)
		{
		int fLoc = 0;
		//for (int i = 0; i < gpFlamMapAP->m_numStaticFires; i++)
		for (int i = 0; i < gpFlamMapAP->m_staticFires.size(); i++)
			{
			if (gpFlamMapAP->m_staticFires[i].GetYr() == year)
				{
				fireArray[fLoc] = gpFlamMapAP->m_staticFires[i];
				fLoc++;
				}
			}
		}
	else
		{
		int fLoc = 0;
		FireList::iterator i;

		for (i = m_firesList.firesList.begin(); i != m_firesList.firesList.end(); ++i)
			{
			Fire fire = *i;
			if (fire.GetYr() == year && fire.GetDoRun())
				{
				fireArray[fLoc] = fire;
				fLoc++;
				}
			}
		}

	bool logFlameLengths = gpFlamMapAP->m_logFlameLengths ? true : false;
	CMinTravelTime* pMtt = NULL;
	int MaxTries = gpFlamMapAP->m_maxRetries + 1;
	//get the ignition points here (avoid issues with random in OpenMP loop...
	if (gpFlamMapAP->m_staticFires.size() == 0 && !gpFlamMapAP->m_useFirelistIgnitions)
		{
		for (int n = 0; n < firesToDo; n++)
			{
			//get ignition point for fire
			REAL fireX, fireY;
			int fuel;
			m_pIgnitGenerator->GetIgnitionPointFromGrid(pEnvContext, fireX, fireY);
			//ensure ignition point is burnable!
			fuel = (int)tFlamMap.GetLayerValue(FUEL, fireX, fireY);
			while (FuelIsNonburnable(fuel))
				{
				//get ignition point for fire
				m_pIgnitGenerator->GetIgnitionPointFromGrid(pEnvContext, fireX, fireY);
				fuel = (int)tFlamMap.GetLayerValue(FUEL, fireX, fireY);
				}
			fireArray[n].SetFuel(fuel);
			fireArray[n].SetIgnitionPoint(fireX, fireY);
			}
		}
	else
		{
		for (int n = 0; n < firesToDo; n++)
			{
			int fuel = (int)tFlamMap.GetLayerValue(FUEL, fireArray[n].GetIgnitX(), fireArray[n].GetIgnitY());
			fireArray[n].SetFuel(fuel);
			}
		}

	FILE* envFireList = NULL;
	//FILE* echoFiresList = NULL;
	if (gpFlamMapAP->m_outputEnvisionFirelists)
		{
		envFireList = fopen(gpFlamMapAP->m_outputEnvisionFirelistName, "a+t");
		//echoFiresList = fopen(gpFlamMapAP->m_outputEchoFirelistName, "a+t");
		}

	msg.Format(_T("Starting fires for year %d (%d), running %d fires"), year, pEnvContext->currentYear, firesToDo);
	Report::Log(msg);

	float* flameLenLayer, maxFlameLen = 0;
	float** flameLengthArray = new float* [firesToDo];
	//int c;
	CString FlameLengthFName, ArrivalTimeFName, AnnualFlameLengthName;
	int firesDone = 0;
	long nCellsBurned;
	long iduCells;
	double iduArea, newX, newY, originalX, originalY;
	int errorsFMS = 0, errorsFMP = 0, errorsMTT = 0, errorsIgnition = 0, offLandscapeIgnition = 0, retryCount = 0, flLoc = 0;
	bool needToRetry = false;
	CPerimeter* perim = nullptr;
	__int64 nCells = nRows * nCols;
	int* burnMask = new int[nCells];
	memset(burnMask, 0, nCells * sizeof(int));
	for (int n = 0; n < firesToDo; n++)
		{
		flameLengthArray[n] = NULL;
		// Go through this year's fires
		// run each in order
		tFlamMap.ResetBurn();
		tFlamMap.SelectOutputReturnType(0);
		if (n > 0)
			tFlamMap.SetNoBurnMask(burnMask);
		ret = tFlamMap.SetFuelMoistureFile(fireArray[n].GetFMSname());
		if (ret != 1)
			{
			errorsFMS++;
			continue;
			}
		tFlamMap.SetConstWind(fireArray[n].GetWindSpd(), fireArray[n].GetWindAzmth());
		tFlamMap.SetFoliarMoistureContent(gpFlamMapAP->m_foliarMoistureContent);
		tFlamMap.SetUseScottReinhardt(gpFlamMapAP->m_crownFireMethod);
		tFlamMap.SelectOutputLayer(MAXSPREADDIR, true);
		tFlamMap.SelectOutputLayer(ELLIPSEDIM_A, true);
		tFlamMap.SelectOutputLayer(ELLIPSEDIM_B, true);
		tFlamMap.SelectOutputLayer(ELLIPSEDIM_C, true);
		tFlamMap.SelectOutputLayer(MAXSPOT, true);
		tFlamMap.SelectOutputLayer(INTENSITY, true);
		tFlamMap.SelectOutputLayer(SPREADRATE, true);
		tFlamMap.SetNumberThreads(1);

		ret = tFlamMap.LaunchFlamMap(0);
		if (ret != 1)
			{
			errorsFMP++;
			continue;
			}
		originalX = fireArray[n].GetIgnitX();
		originalY = fireArray[n].GetIgnitY();
		pMtt = new CMinTravelTime;
		needToRetry = true;
		retryCount = 0;
		while (needToRetry && retryCount < MaxTries)
			{
			double cellSize = tFlamMap.GetCellSize();
			int burnPeriod = fireArray[n].GetBurnPeriod();
			pMtt->SetFlamMap(&tFlamMap);
			pMtt->SetResolution(cellSize);
			pMtt->SetMaxSimTime(burnPeriod);
			pMtt->SetNumProcessors(1);
			pMtt->SetSpotProbability(gpFlamMapAP->m_spotProbability);
			if (retryCount > 0)
				{
				//need new ignition point
				getNewIgnitionPoint(&tFlamMap, gpFlamMapAP->m_pIgnitRand, originalX, originalY, &newX, &newY);
				fireArray[n].SetIgnitionPoint(newX, newY);
				}
			// is this fire's ignition point inside the MTT grid?
			double xIgnition = fireArray[n].GetIgnitX();
			double yIgnition = fireArray[n].GetIgnitY();

			if (BETWEEN(pMtt->GetWest(), xIgnition, pMtt->GetEast()) && BETWEEN(pMtt->GetSouth(), yIgnition, pMtt->GetNorth()))     //if (xIgnition >= pMtt->GetWest() && xIgnition <= pMtt->GetEast() 	&& yIgnition >= pMtt->GetSouth() && yIgnition <= pMtt->GetNorth())
				{
				// return value of "1" indicates success
				ret = pMtt->SetIgnitionPoint(xIgnition, yIgnition);
				}
			else  // outside the grid, records as off-landscape ignition
				{
				ret = 2;  // signal it was unsuccessful
				offLandscapeIgnition++;
				delete pMtt;
				pMtt = NULL;
				break;
				}
			// if unsuccessful, try again
			if (ret != 1)
				{
				retryCount++;
				if (retryCount < MaxTries)
					continue;

				if (envFireList && MaxTries <= 1)  // write to firelist output file
					{
					fprintf(envFireList, "%d, %f, %d, %d, %d, %d, %s, %f, %f, %f, %d, %f, %d, 0.0, 0, -1, %s, %d, %d, %s, %d",
						year,
						fireArray[n].GetBurnProb(),
						fireArray[n].GetJulDate(),
						fireArray[n].GetBurnPeriod(),
						fireArray[n].GetWindSpd(),
						fireArray[n].GetWindAzmth(),
						(LPCTSTR)fireArray[n].GetShortFMSname(),
						fireArray[n].GetIgnitX(),
						fireArray[n].GetIgnitY(),
						0.0,
						fireArray[n].GetERC(),
						fireArray[n].GetOrigSizeHA(),
						fireArray[n].GetFuel(),
						(LPCTSTR)gpFlamMapAP->m_scenarioName,
						pEnvContext->runID,
						ret == 2 ? RESULT_IGNIT_OFF_LCP : RESULT_IGNIT_NON_BURNABLE,
						fireArray[n].GetFireID(),
						fireArray[n].GetEnvisionFireID());

					for (flLoc = 0; flLoc < 20; flLoc++)
						fprintf(envFireList, ", 0.000000");

					fprintf(envFireList, "\n");
					//if (echoFiresList)
					//	fprintf(echoFiresList, "%s, %d\n", fireArray[n].GetRecord(), fireArray[n].GetEnvisionFireID());
					}
				delete pMtt;
				pMtt = NULL;
				if (ret != 2)    // if return value not equal to 1 or 2, signals an ignition error
					errorsIgnition++;
				continue;
				}

			ret = pMtt->LaunchMTTQuick();
			if (ret != 1)
				{
				retryCount++;
				if (retryCount < MaxTries)
					continue;
				if (envFireList && MaxTries <= 1)
					{
					fprintf(envFireList, "%d, %f, %d, %d, %d, %d, %s, %f, %f, %f, %d, %f, %d, 0.0, 0, -1, %s, %d, %d, %s, %d",
						year,
						fireArray[n].GetBurnProb(),
						fireArray[n].GetJulDate(),
						fireArray[n].GetBurnPeriod(),
						fireArray[n].GetWindSpd(),
						fireArray[n].GetWindAzmth(),
						(LPCTSTR)fireArray[n].GetShortFMSname(),
						fireArray[n].GetIgnitX(),
						fireArray[n].GetIgnitY(),
						tFlamMap.GetCellSize() * tFlamMap.GetCellSize() / 10000.0,
						fireArray[n].GetERC(),
						fireArray[n].GetOrigSizeHA(),
						fireArray[n].GetFuel(),
						(LPCTSTR)gpFlamMapAP->m_scenarioName,
						pEnvContext->runID,
						RESULT_NO_BURN,
						fireArray[n].GetFireID(),
						fireArray[n].GetEnvisionFireID());
					for (flLoc = 0; flLoc < 20; flLoc++)
						fprintf(envFireList, ", 0.000000");
					fprintf(envFireList, "\n");
					}
				//if (echoFiresList)
				//	{
				//	fprintf(echoFiresList, "%s, %d\n", fireArray[n].GetRecord(), fireArray[n].GetEnvisionFireID());
				//	}
				delete pMtt;
				pMtt = NULL;

				errorsMTT++;
				continue;
				}
			//here, need to ensure fire reached the required proportion of "Orginal_Size" 
			flameLenLayer = pMtt->GetFlameLengthGrid();
			nCellsBurned = 0;

			for (__int64 c = 0; c < nRows * nCols; c++)
				{
				if (flameLenLayer[c] > 0.0)
					nCellsBurned++;
				}
			if (fireArray[n].GetOrigSizeHA() > 0.0
				&& nCellsBurned * tFlamMap.GetCellSize() * tFlamMap.GetCellSize() / 10000.0 < fireArray[n].GetOrigSizeHA() * gpFlamMapAP->m_FireSizeRatio)
				{
				//fire too small, rerun
				retryCount++;
				if (retryCount < MaxTries)
					continue;
				//if got here exceeded retry count
				delete pMtt;
				pMtt = NULL;
				break;
				}
			else
				needToRetry = false;

			}//end while retryCount < MaxTries

			// Get the flame length results
		if (pMtt == NULL)
			{
			continue;
			}
		flameLenLayer = pMtt->GetFlameLengthGrid();

		// write the ascii file with flame lengths
		if (logFlameLengths)
			{
			// For printing out FlameLength and related Files
			FlameLengthFName.Format(_T("%s%d_FlameLength_%03d_TS%04d_Day%04d.asc"), gpFlamMapAP->m_outputPath, gpFlamMapAP->processID, pEnvContext->runID, year, fireArray[n].GetJulDate());
			pMtt->WriteFlameLengthGrid((char*)FlameLengthFName.GetString());
			}
		if (gpFlamMapAP->m_logArrivalTimes)
			{
			ArrivalTimeFName.Format(_T("%s%d_ArrivalTime_%03d_TS%04d_Day%04d.asc"), gpFlamMapAP->m_outputPath, gpFlamMapAP->processID, pEnvContext->runID, year, fireArray[n].GetJulDate());
			pMtt->WriteArrivalTimeGrid((char*)ArrivalTimeFName.GetString());
			}
		nCellsBurned = 0;
		flameLengthArray[n] = new float[nRows * nCols];

		for (flLoc = 0; flLoc < 20; flLoc++)
			{
			fireArray[n].FIL[flLoc] = 0.0;
			}
		for (__int64 c = 0; c < nRows * nCols; c++)
			{
			flameLengthArray[n][c] = flameLenLayer[c];
			if (flameLenLayer[c] > 0.0)
				{
				//convert flame length feet to meters for fire FIL
				flLoc = (int)((flameLenLayer[c] * 0.3048) / 0.5);
				if (flLoc >= 20)
					flLoc = 19;
				fireArray[n].FIL[flLoc] += 1.0;
				//mark the mask so only burn once in a year
				burnMask[c] = 1;
				nCellsBurned++;
				}
			} // for(int i=0;...
			//conert fire's FIL array to proportions
		if (nCellsBurned > 0)
			{
			for (flLoc = 0; flLoc < 20; flLoc++)
				{
				fireArray[n].FIL[flLoc] /= (double)nCellsBurned;
				}
			}
		if (envFireList)
			{
			iduArea = CalcIduAreaBurned(pEnvContext, flameLenLayer, &iduCells);
			fprintf(envFireList, "%d, %f, %d, %d, %d, %d, %s, %f, %f, %f, %d, %f, %d, %f, %f, %d, %s, %d, %d, %s, %d",
				year,
				fireArray[n].GetBurnProb(),
				fireArray[n].GetJulDate(),
				fireArray[n].GetBurnPeriod(),
				fireArray[n].GetWindSpd(),
				fireArray[n].GetWindAzmth(),
				(LPCTSTR)fireArray[n].GetShortFMSname(),
				fireArray[n].GetIgnitX(),
				fireArray[n].GetIgnitY(),
				nCellsBurned * tFlamMap.GetCellSize() * tFlamMap.GetCellSize() / 10000.0,
				fireArray[n].GetERC(),
				fireArray[n].GetOrigSizeHA(),
				fireArray[n].GetFuel(),
				iduArea,
				((double)iduCells) / ((double)nCellsBurned),
				n + 1,
				(LPCTSTR)gpFlamMapAP->m_scenarioName,
				pEnvContext->runID,
				RESULT_MTT_OK,
				fireArray[n].GetFireID(),
				fireArray[n].GetEnvisionFireID());
			for (flLoc = 0; flLoc < 20; flLoc++)
				fprintf(envFireList, ", %f", fireArray[n].FIL[flLoc]);
			fprintf(envFireList, "\n");
			//if (echoFiresList)
			//	{
			//	fprintf(echoFiresList, "%s, %d\n", fireArray[n].GetRecord(), fireArray[n].GetEnvisionFireID());
			//	}
			}
		//end added for firesizelists
		if (gpFlamMapAP->m_logPerimeters)
			{
			//#pragma omp critical
			//			 {
			perim = pMtt->GetFirePerimeter();
			if (perim)
				gpFlamMapAP->WritePerimeterShapefile(perim, year, pEnvContext->runID, fireArray[n].GetERC(),
					nCellsBurned * tFlamMap.GetCellSize() * tFlamMap.GetCellSize() / 10000.0,
					tFlamMap.GetCellSize(), fireArray[n].GetOrigSizeHA(), fireArray[n].GetIgnitX(), fireArray[n].GetIgnitY(),
					fireArray[n].GetBurnPeriod(), fireArray[n].GetWindAzmth(), fireArray[n].GetWindSpd(), fireArray[n].GetFMSname(),
					fireArray[n].GetFireID(), fireArray[n].GetEnvisionFireID(), fireArray[n].FIL);
			//			 }
			// pMtt->GetFirePerimeter((char *)ArrivalTimeFName.GetString());
			}
		if (pMtt != NULL)
			{	
			delete(pMtt);
			pMtt = NULL;
			flameLenLayer = NULL;
			}
		firesDone++;
		} // fires loop


		//get maximum flamelengths, destroy individual runs
	for (int n = 0; n < firesToDo; n++)
		{
		if (flameLengthArray[n])
			{
			for (__int64 c = 0; c < nRows * nCols; c++)
				{
				//if (flameLengthArray[n][c] > m_pYrFlameLens[c])
				//	m_pYrFlameLens[c] = flameLengthArray[n][c];
				//
				//m_pFireNums[c] = n + 1;
				if (flameLengthArray[n][c] > m_yrFlameLens.at(c))
					m_yrFlameLens[c] = flameLengthArray[n][c];

				m_fireNums.at(c) = n + 1;
				}

			delete[] flameLengthArray[n];
			}
		}

	delete[] flameLengthArray;
	if (burnMask)
		delete[] burnMask;
	if (envFireList)
		fclose(envFireList);
	//if (echoFiresList)
	//	fclose(echoFiresList);
	if (gpFlamMapAP->m_logAnnualFlameLengths != 0)
		{//dump year's flame length to an ascii grid
		AnnualFlameLengthName.Format(_T("%s%d_Run_%03d_Flamelength_Year%04d_.asc"), gpFlamMapAP->m_outputPath, gpFlamMapAP->processID, pEnvContext->runID, year + 1);
		FILE* yrFl = fopen(AnnualFlameLengthName, "wt");
		if (yrFl)
			{
			fprintf(yrFl, "ncols %d\n", gpFlamMapAP->m_cols);
			fprintf(yrFl, "nrows %d\n", gpFlamMapAP->m_rows);
			fprintf(yrFl, "xllcorner %f\n", tFlamMap.GetWest());
			fprintf(yrFl, "yllcorner %f\n", tFlamMap.GetSouth());
			fprintf(yrFl, "cellsize %f\n", gpFlamMapAP->m_cellDim);
			fprintf(yrFl, "NODATA_VALUE 0\n");
			for (int r = 0; r < nRows; r++)
				{
				for (int c = 0; c < nCols; c++)
					{
					//fprintf(yrFl, "%f ", m_pYrFlameLens[r * nCols + c]);
					fprintf(yrFl, "%f ", m_yrFlameLens.at(r * nCols + c));
					}

				fprintf(yrFl, "\n");
				}
			fclose(yrFl);
			}
		else
			{
			msg.Format(_T("Error creating annual flame length grid file: %s"), AnnualFlameLengthName);
			Report::LogError(msg);
			}
		}

	//TRACE3(_T(" Finished fires for year %d, ran %d of %d fires\n"), year, firesDone, firesToDo);
	msg.Format(_T("Finished fires for year %d (%d), attempted to run %d of %d fires"), year, pEnvContext->currentYear, firesDone, firesToDo);
	Report::Log(msg);
	if (errorsFMS > 0)
		{
		msg.Format(_T("Encountered %d FMS file errors"), errorsFMS);
		Report::LogWarning(msg);
		}
	if (errorsFMP > 0)
		{
		msg.Format(_T("Encountered %d FlamMap run errors"), errorsFMP);
		Report::LogWarning(msg);
		}
	if (offLandscapeIgnition > 0)
		{
		msg.Format(_T("Encountered %d Ignitions off landscape"), offLandscapeIgnition);
		Report::LogWarning(msg);
		}
	if (errorsIgnition > 0)
		{
		msg.Format(_T("Encountered %d Ignition errors"), errorsIgnition);
		Report::LogWarning(msg);
		}
	if (errorsMTT > 0)
		{
		msg.Format(_T("Encountered %d MTT non-burning fires"), errorsMTT);
		Report::LogWarning(msg);
		}
	// cleanup to be sure
	if (fireArray)
		delete[] fireArray;

	if (pMtt != NULL)
		{
		delete(pMtt);
		pMtt = NULL;
		}

	if (gpFlamMapAP->m_deleteLCPs)
		{
		_unlink(gpFlamMapAP->m_lcpFName);
		}

	return firesToDo;
	} // int FireYearRunner::RunFireYear()


int FireYearRunner::GetFireNum(const int x, const int y)
	{
	//return m_pFireNums[x * gpFlamMapAP->m_cols + y];
	return m_fireNums.at(x * gpFlamMapAP->m_cols + y);
	}

float FireYearRunner::GetFlameLen(const int x, const int y)
	{
	//return m_pYrFlameLens[x * gpFlamMapAP->m_cols + y];
	return m_yrFlameLens.at(x * gpFlamMapAP->m_cols + y);
	}


FireYearRunner::~FireYearRunner()
	{
	//if (m_pFireNums != NULL)
	//	delete[] m_pFireNums;
	//
	//if (m_pYrFlameLens != NULL)
	//	delete[] m_pYrFlameLens;

	m_fireNums.clear();  // if (m_pFireNums != NULL) delete[] m_pFireNums;
	m_yrFlameLens.clear(); //   if (m_pYrFlameLens != NULL) delete[] m_pYrFlameLens;


	if (m_pIgnitGenerator != NULL)
		delete m_pIgnitGenerator;
	} // FireYearRunner::~FireYearRunner()


double FireYearRunner::CalcIduAreaBurned(EnvContext* pEnvContext, float* flameLenLayer, long* numStudyAreaCells)
	{
	int colArea = gpFlamMapAP->m_areaCol;
	int maxGridPtCnt = 0;
	int burnedPolyCount = 0;
	PolyGridLookups* pPolyGridLkUp = gpFlamMapAP->GetPolyGridLookups();
	float* gridPtProportions = NULL;
	POINT* pGridPtNdxs = NULL;

	int tSubCells = 0;
	double iduArea = 0.0;
	*numStudyAreaCells = 0;
	long studyCells = 0;

	for (int poly = 0; poly < pPolyGridLkUp->GetNumPolys(); poly++)
		{
		int gridPtCnt = pPolyGridLkUp->GetGridPtCntForPoly(poly);
		// insure arrays have sufficient space
		if (gridPtCnt > maxGridPtCnt)
			{
			maxGridPtCnt = gridPtCnt;

			if (gridPtProportions)
				delete[] gridPtProportions;

			gridPtProportions = new float[maxGridPtCnt];

			if (pGridPtNdxs)
				delete[] pGridPtNdxs;

			pGridPtNdxs = new POINT[maxGridPtCnt];
			} // if((nGridPtCnt = m_pPolyGridLkUp->GetGridPtCntForPoly(i)) > nMaxGridPtCnt)

		// intialize the temporary storage for gris location/proportion info
		for (int i = 0; i < maxGridPtCnt; i++)
			{
			gridPtProportions[i] = 0;
			pGridPtNdxs[i].x = 0;
			pGridPtNdxs[i].y = 0;
			}

		// get the grid points for the poly
		pPolyGridLkUp->GetGridPtNdxsForPoly(poly, pGridPtNdxs);

		// get the proportions for the poly
		pPolyGridLkUp->GetGridPtProportionsForPoly(poly, gridPtProportions);

		// did it burn?
		int gridPolysBurned = 0;
		float burnPropSum = 0.0f;

		float polyFlameLength;     // meters
		for (int gridPt = 0; gridPt < gridPtCnt; gridPt++)
			{
			polyFlameLength = flameLenLayer[pGridPtNdxs[gridPt].x * gpFlamMapAP->m_cols + pGridPtNdxs[gridPt].y];
			if (polyFlameLength > 0.0f)
				{
				gridPolysBurned++;
				studyCells++;
				burnPropSum += gridPtProportions[gridPt];
				}
			} // for(GridPt=0;GridPt<nGridPtCnt;GridPt++)

		if (burnPropSum >= gpFlamMapAP->m_iduBurnPercentThreshold) //IDU is considered burned
			{           //need to weight the area based on proportions
			float area;
			pEnvContext->pMapLayer->GetData(poly, colArea, area);

			// iterate through all the grid cells that intersect this IDU
			for (int gridPt = 0; gridPt < gridPtCnt; gridPt++)
				{
				polyFlameLength = flameLenLayer[pGridPtNdxs[gridPt].x * gpFlamMapAP->m_cols + pGridPtNdxs[gridPt].y];
				if (polyFlameLength > 0.0f)
					{
					iduArea += area * gridPtProportions[gridPt] / burnPropSum;
					tSubCells = pPolyGridLkUp->GetPolyCntForGridPt(pGridPtNdxs[gridPt].x, pGridPtNdxs[gridPt].y);
					//TRACE3("Cell x,y = (%d, %d), subCells = %d\n", pGridPtNdxs[GridPt].x, pGridPtNdxs[GridPt].y, tSubCells);
					}
				} // for(GridPt=0;GridPt<nGridPtCnt;GridPt++)
			}
		} // for(Poly=0;Poly<pPolyGridLkUp->GetNumPolys();Poly++) 

	if (pGridPtNdxs != NULL)
		{
		delete[] pGridPtNdxs;
		pGridPtNdxs = NULL;
		}

	if (gridPtProportions != NULL)
		{
		delete[] gridPtProportions;
		gridPtProportions = NULL;
		}

	//int subCellsPerCell = pPolyGridLkUp->GetPolyCntForGridPt(1,1);
	if (tSubCells > 0)
		studyCells /= tSubCells;

	*numStudyAreaCells = studyCells;
	return iduArea / M2_PER_HA;     // assume m3, convert to HA
	}


bool FireYearRunner::CalcIduPotentialFlameLen(EnvContext* pContext, MapLayer* pGrid)
	{
	int colPotFlameLen = pContext->pMapLayer->GetFieldCol("PFLAMELEN");

	PolyGridLookups* pPolyGridLkUp = gpFlamMapAP->GetPolyGridLookups();  // this provides the vector/raster mapping

	// we will allocate an array to store the proportion of an IDU that lays in various grid cells.
	// insure arrays have sufficient space by figuring out max number of intersecting grid cells for any given IDU
	int maxGridPtCnt = 0;

	for (int idu = 0; idu < pPolyGridLkUp->GetNumPolys(); idu++)
		{
		int numGridPts = pPolyGridLkUp->GetGridPtCntForPoly(idu);

		if (numGridPts > maxGridPtCnt)
			maxGridPtCnt = numGridPts;
		}

	// allocate and initialize arrays for storing proportions of the idu in various grid cells.
	float* gridPtProportionsArray = new float[maxGridPtCnt];
	memset(gridPtProportionsArray, 0, maxGridPtCnt * sizeof(float));

	POINT* gridPtNdxArray = new POINT[maxGridPtCnt];    // this stores the coords of the grids intersecting any
	memset(gridPtNdxArray, 0, maxGridPtCnt * sizeof(POINT));      // given IDU



	///////////////////////////////
	//int rows = pPolyGridLkUp->GetNumGridRows();
	//int cols = pPolyGridLkUp->GetNumGridCols();
	//
	//int iduCount = pContext->pMapLayer->GetRecordCount();
	//int polyCount = pPolyGridLkUp->GetNumPolys();
	//
	//ASSERT( iduCount == polyCount );
	//
	//for ( int idu=0; idu < polyCount; idu++ ) 
	//   {
	//   int numGridPts = pPolyGridLkUp->GetGridPtCntForPoly( idu );
	//
	//   // get the grid points for the poly
	//   pPolyGridLkUp->GetGridPtNdxsForPoly( idu, gridPtNdxArray );
	//
	//   // get the proportions of intersecting grid cells that intersect this poly
	//   pPolyGridLkUp->GetGridPtProportionsForPoly( idu, gridPtProportionsArray );
	//
	//   for( int gridPt=0; gridPt < numGridPts; gridPt++ ) 
	//      {
	//      // rows = 445, cols =278
	//      POINT &pt = gridPtNdxArray[ gridPt ];
	//
	//      int gridCol = pt.x;
	//      int gridRow = pt.y;
	//
	//      if ( gridCol > 260 && gridRow < 10  )
	//         {
	//         int iduIndex = 0;
	//         int iduIndexCol = pContext->pMapLayer->GetFieldCol( "IDU_INDEX" );
	//         pContext->pMapLayer->GetData( idu, iduIndexCol, iduIndex );
	//
	//         CString msg;
	//         msg.Format( "Poly: %i, Grid: %i,  x: %i y: %i, pctArea: %g\n", idu, gridPt, pt.x, pt.y, gridPtProportionsArray[ gridPt ] );
	//         TRACE( msg );
	//         }
	//      }
	//   }






	// for each IDU polygon, get the portion of each cell in the polygon
	//
	// Basic idea:  for each IDU, get the proportions of vvarious grid cells that intersect that IDU.
	// Then, for each of the grid cell, calculate an area weighted average of values of the
	// potential flame lenge, and populate this value into the IDU
	for (int idu = 0; idu < pPolyGridLkUp->GetNumPolys(); idu++)
		{
		// get the grid points for the poly
		pPolyGridLkUp->GetGridPtNdxsForPoly(idu, gridPtNdxArray);

		// get the proportions of intersecting grid cells that intersect this poly
		pPolyGridLkUp->GetGridPtProportionsForPoly(idu, gridPtProportionsArray);

		float iduFlameLen = 0;

		int numGridPts = pPolyGridLkUp->GetGridPtCntForPoly(idu);

		for (int gridPt = 0; gridPt < numGridPts; gridPt++)
			{
			// rows = 445, cols =278
			POINT& pt = gridPtNdxArray[gridPt];


			//int gridCol = pt.x;
			//int gridRow = pt.y;
			int gridCol = pt.y;
			int gridRow = pt.x;

			if (gridCol < 0 || gridCol >= pPolyGridLkUp->GetNumGridCols() || gridRow < 0 || gridRow >= pPolyGridLkUp->GetNumGridRows())
				{
				CString msg;
				msg.Format("Poly: %i, Grid: %i,  x: %i y: %i, pctArea: %g\n", idu, gridPt, pt.x, pt.y, gridPtProportionsArray[gridPt]);
				TRACE(msg);
				}
			else
				{
				float flameLen = 0;
				pGrid->GetData(gridRow, gridCol, flameLen);  // this is in meter

				iduFlameLen += (flameLen * gridPtProportionsArray[gridPt]);
				}
			}  // end of: for ( gridPt=0; gridPt < numGridPts; gridPt++ )

		iduFlameLen *= FT_PER_M;
		gpFlamMapAP->UpdateIDU(pContext, idu, colPotFlameLen, iduFlameLen, ADD_DELTA); // ft
		} // for( idu=0; idu < pPolyGridLkUp->GetNumPolys(); idu++) 

	// clean up
	delete[] gridPtNdxArray;
	delete[] gridPtProportionsArray;

	return true;
	}