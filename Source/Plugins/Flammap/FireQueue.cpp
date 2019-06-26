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
/* FireQueue.cpp
**
** Loading and doling out the data for fires to be run
** during one run of Envision.
**
** See FireQueue.h for additional documentation.
**
** 2011.03.23 - tjs - added error handling
*/

#include "stdafx.h"
#pragma hdrstop

#include "FireQueue.h"
#include "Fire.h"
#include <MapLayer.h>
#include <Vdataobj.h>
#include <string>
#include <cstring>
#include <queue>
#include <cstdlib>
#include <cstdio>
#include <randgen\randunif.hpp>

#include "ParamLookup.h"
	extern ParamLookup
		g_RunParams;

#if 0

// VDataObj bug!!!

// This is the more elegant version of the constructor,
// but unfortunately VDataObj thinks the input file
// is longer than it is and brings in lines of
// mangled values where no lines should be.

// Code left here in case VDataObj gets fixed

FireQueue::FireQueue(const char * InitFName) {
	VDataObj
		inData;
	int
		recordsTtl;
	int
		yr,
		julDate,
		windSpd,
		azmth;
	float
		burnPeriod,
		prob;
	Fire 
		*tmpFire;

	if((recordsTtl = (int) inData.ReadAscii(InitFName,',',true)) <= 0) {
		g_RunParams.SetBoolParam(_T("RunStatus"),false);
		return;
	}
	
	// examine each line, determine if it is a fire to burn
	// queue it if it is, then put it on the queue
	for(int i=0;i<recordsTtl;i++) {
		if(!inData.Get(0,i,yr)) {
			g_RunParams.SetBoolParam(_T("RunStatus"),false);
			return;
		}
		if(!inData.Get(1,i,prob)) {
			g_RunParams.SetBoolParam(_T("RunStatus"),false);
			return;
		}
		if(!inData.Get(2,i,julDate)) {
			g_RunParams.SetBoolParam(_T("RunStatus"),false);
			return;
		}
		if(!inData.Get(3,i,burnPeriod)) {
			g_RunParams.SetBoolParam(_T("RunStatus"),false);
			return;
		}
		if(!inData.Get(4,i,windSpd)) {
			g_RunParams.SetBoolParam(_T("RunStatus"),false);
			return;
		}
		if(!inData.Get(4,i,azmth)) {
			g_RunParams.SetBoolParam(_T("RunStatus"),false);
			return;
		}
	
		tmpFire = new Fire(
			yr,
			prob,
			julDate,
			(int) (burnPeriod + 0.499),
			windSpd,
			azmth	
		);

		if(tmpFire->GetDoRun()) 
			// queue the fire
			Fires.push(tmpFire);
		else
			// discard the fire
			delete tmpFire;

	} // for(int i=0;i<recordsTtl;i++)

} // FireQueue(const char * InitFName)
#endif


// Less elegant method of reading .csv, but it has
// the distinct advantage that it works.

void FireQueue::Init(const char *InitFName) {
	CString msg;
	VDataObj inData (U_UNDEFINED);
	int recordsCnt = 0;

	FILE *inFile;
	char inBuf[2048];
	
	Fire *tmpFire;

	if((inFile = fopen(InitFName,"r")) == NULL) {
		msg = (_T("FireQueue::FireQueue: Failed to open file "));
		msg += InitFName;
		Report::ErrorMsg(msg);
		g_RunParams.SetBoolParam(_T("RunStatus"),false);
		return;
	}

	// burn the first line of headers
	fgets(inBuf, sizeof(inBuf)-1,inFile);

	RandUniform
		uniRandDist;
	float
		monteCarloProb;
	// read and process data lines
	while(fgets(inBuf, sizeof(inBuf)-1,inFile) != NULL) {

		monteCarloProb = 
			(float) uniRandDist.RandValue(0, 1);


		tmpFire = new Fire(inBuf,monteCarloProb);

		if(tmpFire->GetDoRun() && tmpFire->GetBurnPeriod() > 0) 
			// queue the fire
			Fires.push(tmpFire);
		else
			// discard the fire
			delete tmpFire;

		recordsCnt++;
	} // for(int i=0;i<recordsTtl;i++)

	fclose(inFile);

	int tmp = (int) Fires.size();

} // FireQueue(const char * InitFName)

FireQueue::~FireQueue() {

	while(!Fires.empty()) {
		delete Fires.front();
		Fires.pop();
	}
} // ~FireQueue()


int FireQueue::GetHdYr() {
	int
		rtrn = -99;
	if(!Fires.empty())
		rtrn = Fires.front()->GetYr();
	return rtrn;
}

int FireQueue::GetHdJulDate() {
	int
		rtrn = -99;
	if(!Fires.empty())
		rtrn = Fires.front()->GetJulDate();

	return rtrn;
}

int FireQueue::GetHdWindSpd() {
	int
		rtrn = -99;
	if(!Fires.empty())
		rtrn = Fires.front()->GetWindSpd();

	return rtrn;
}

int FireQueue::GetHdWindAzmth() {
	int
		rtrn = -99;
	if(!Fires.empty())
		rtrn = Fires.front()->GetWindAzmth();

	return rtrn;
}

double FireQueue::GetHdBurnProb() {
	double
		rtrn = -99.;
	if(!Fires.empty())
		rtrn = Fires.front()->GetBurnProb();

	return rtrn;
}

int FireQueue::GetHdBurnPeriod() {
	int
		rtrn = -99;
	if(!Fires.empty())
		rtrn = Fires.front()->GetBurnPeriod();

	return rtrn;
}

bool FireQueue::GetHdDoRun() {
	bool
		rtrn = false;
	if(!Fires.empty())
		rtrn = Fires.front()->GetDoRun();

	return rtrn;
}

int FireQueue::FireCnt() {
	return (int)Fires.size();
}


