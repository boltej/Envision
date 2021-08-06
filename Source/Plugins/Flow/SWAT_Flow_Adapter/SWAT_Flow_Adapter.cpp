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
// HBV.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "SWAT_Flow_Adapter.h"
#include <Flow.h>
#include <DATE.HPP>
#include <Maplayer.h>
#include <MAP.h>
#include <UNITCONV.H>
#include <omp.h>
#include <math.h>
#include <UNITCONV.H>



#include <tixml.h>
#include <Path.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



float SWAT_Flow_Adapter::InitSWAT_Flow_Adapter(FlowContext *pFlowContext, LPCTSTR inti)
   {
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" ); 
   m_col_cfmax = pHBVTable->GetFieldCol( "CFMAX" );         // get the location of the parameter in the table    

   const char* p = pFlowContext->pFlowModel->GetPath();
   return -1.0f;
   }

float SWAT_Flow_Adapter::RunSWAT_Flow_Adapter(FlowContext* pFlowContext, LPCTSTR initStr)
{
	if (pFlowContext->timing & GMT_INIT) // Init()
		return SWAT_Flow_Adapter::Init(pFlowContext->pEnvContext, initStr);

	if (pFlowContext->timing & GMT_INITRUN) // Init()
		return SWAT_Flow_Adapter::InitRun(pFlowContext,0);

	if ((pFlowContext->timing & GMT_START_YEAR))   // At the start of each year
		return SWAT_Flow_Adapter::InitYear(pFlowContext);

	if ((pFlowContext->timing & GMT_START_STEP))   // at the start of each day
		return SWAT_Flow_Adapter::RunDay(pFlowContext);
   

   const char* p = pFlowContext->pFlowModel->GetPath();
   return -1.0f;
}



bool SWAT_Flow_Adapter::Init(EnvContext* pEnvContext, LPCTSTR initStr)
{
	bool ok = LoadXml(pEnvContext, initStr);
	if (!ok)
		return FALSE;

#if defined( _DEBUG )
	m_hInst = ::AfxLoadLibrary("SWAT_DLL.dll");
	// m_hInst = ::AfxLoadLibrary("Example.dll");
//	 m_hInst = LoadLibrary("C:\\Users\\kelli\\Documents\\GitHub\\Envision\\Source\\Example\\debug\\Example_DLL.dll");
//	 m_hInst = ::AfxLoadLibrary("HBV.dll");
#else
	m_hInst = ::AfxLoadLibrary("SWAT_DLL.dll");
#endif

	if (m_hInst == 0)
	{
		CString msg("SWAT: Unable to find the SWAT dll");

		Report::ErrorMsg(msg);
		return FALSE;
	}

	//m_GetNamFilFn = (MODFLOW_GETNAMFILFN)GetProcAddress(m_hInst, "GETNAMFIL");
	m_RunSWAT_AllFn = (SWAT_RUN_ALLFN)GetProcAddress(m_hInst, "SWAT_DLL");
	m_InitSWATFn = (SWAT_INITFN)GetProcAddress(m_hInst, "INIT");
	m_RunSWATFn = (SWAT_RUNFN)GetProcAddress(m_hInst, "RUNYEAR");
	m_InitSWAT_YearFn = (SWAT_RUNFN)GetProcAddress(m_hInst, "FLOWINITYEAR");
	m_RunSWAT_DayFn = (SWAT_RUN_DAYFN)GetProcAddress(m_hInst, "FLOWRUNDAY");
	m_EndRunSWATFn = (SWAT_ENDRUNFN)GetProcAddress(m_hInst, "ENDRUN");

	if (m_hInst != 0)
	{
		CString msg("SWAT_Adapter::Init successfull");
		Report::LogInfo(msg);
	}

	int m_iunit[100];
	m_iGrid = 2;


#define numNewAgents 5
	//In C, if a variable is an array or a pointer, do not apply the '&' operator 
	//to the variable name when passing it to Fortran.
	//In other words, pass the pointer, not the address of the pointer.
	double starttime = 0;
	double timeStep = 10;
	double agents_in_time[numNewAgents];
	int agents_in_door[numNewAgents];


	for (int i = 0; i < numNewAgents; ++i)
	{
		agents_in_time[i] = 2.5;
		agents_in_door[i] = 15 - i;
	}
	//char* fileName = "\\Envision\\StudyAreas\\WW2100\\ModFlow\\TRR.nam";
	//char* fileName = m_swatPath;
	LPCTSTR x = (LPCTSTR)m_swatPath;
	m_pathSz = strlen(m_swatPath);
	InitSWAT(agents_in_door, &m_iGrid, agents_in_time, (char*)x,&m_pathSz);


	return TRUE;
}
int int_cmp(const void* a, const void* b);
bool SWAT_Flow_Adapter::InitRun(FlowContext* pFlowContext, bool useInitialSeed)
{
	const int hruCount = pFlowContext->pFlowModel->GetHRUCount();
	//int swCol=pFlowContext->pEnvContext->pMapLayer->GetFieldCol("HRUID");
	int swCol = pFlowContext->pEnvContext->pMapLayer->GetFieldCol("HRU_GISNUM");//the HRU number from SWAT.  We sort by this value to align with SWAT HRU arrays

	m_precipArray = new double[hruCount];
	m_tmxArray = new double[hruCount];
	m_tmnArray = new double[hruCount];
	m_petArray = new double[hruCount];
	m_aetArray = new double[hruCount];
	m_swcArray = new double[hruCount];
	m_irrArray = new double[hruCount];
	m_irrEfficiency = new double[hruCount][2];
	m_gwRechargeArray = new double[hruCount];
	m_lulcArray = new int[hruCount];
	m_irrToSWATArray = new double[hruCount];

	int foundCount = 0;
	//A subset of HRUs correspond to SWAT.  This code ignores all HRUs outside of that subset.
	//Here we need to look at all HRUs and identify those with HRU_ID > 0.  The HRU_ID values (>0)
	//correspond to the SWAT HRU id number.  If the HRUs are sorted by that value, they
	//will be in the same order as the SWAT array that stores hrus.
	for (int h = 0; h < hruCount; h++)
	   {
		HRU* pHRU = pFlowContext->pFlowModel->GetHRU(h);
		float tmax = 0.0f; float tmin = 0.0f; float prec = 0; int hrunum = 0;
		for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
		   {
			pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[k], swCol, hrunum);
			if (hrunum > 0 && hrunum < 1000000)//this polygon is in a SWAT HRU
			   {
				//m_hruOffsetArray.Add(h);//want hruOffsetArray to end up same size as number hrus in SWAT
				SWAT_OFFSET_INFO* pInfo = new SWAT_OFFSET_INFO;
				pInfo->swat_HRU_Number = hrunum;
				pInfo->idu_offset = pHRU->m_polyIndexArray.GetAt(k);
				pInfo->hru_offset = h;
				pInfo->pHRU = pHRU;	
				int index = (int)m_hruOffsetArray.Add(pInfo);//these are added in the order in which the are stored in Flow.  Need to reorder to match SWAT HRU arrays
				foundCount++;
				break;//once a polygon is identified, just assume that is the only HRU that represents it
			   }
		   }
	   }

	for (int i = 0; i < m_hruOffsetArray.GetSize(); i++)
	{
		SWAT_OFFSET_INFO* P = m_hruOffsetArray.GetAt(i);
		int j = 0;
	}

	qsort(m_hruOffsetArray.GetData(), (int)m_hruOffsetArray.GetSize(), sizeof(INT_PTR), int_cmp);//sort the hruindex by hru value.  Matches SWAT HRU array ordering
	for (int i = 0; i < m_hruOffsetArray.GetSize(); i++)
		{
		SWAT_OFFSET_INFO* P = m_hruOffsetArray.GetAt(i);
		int j=0;
		}
	int t = 1;
	return TRUE;
}
bool SWAT_Flow_Adapter::Run(EnvContext* pContext)
{
	RunSWAT();
	CString msg;
	msg.Format("SWAT_Adapter::Year %i done", pContext->currentYear);
	Report::LogInfo(msg);
	return TRUE;
}
bool SWAT_Flow_Adapter::InitYear(FlowContext* pFlowContext)
{
   //landcover may change once per year.  At the beginning of each year, update the array passed to SWAT (length numberSWATHrus) with the Envision landcover data.  The values
	//have to be valid SWAT lulc codes, that SWAT will use to query it's crop database.
	int col_lulcb = pFlowContext->pEnvContext->pMapLayer->GetFieldCol("SWAT_ID");//SWAT lulc field
	for (int i = 0; i < m_hruOffsetArray.GetSize(); i++)
	   {
		HRU* pHRU = pFlowContext->pFlowModel->GetHRU(m_hruOffsetArray[i]->hru_offset);//has been sorted by SWAT hru number
		int lulc=-1;
		int offset= m_hruOffsetArray.GetAt(i)->idu_offset;
		pFlowContext->pEnvContext->pMapLayer->GetData(offset, col_lulcb, lulc);
		m_lulcArray[i]=lulc;//Get the lulc for this polygon, and put it in the correct location in the SWAT lulc array.
		m_aetArray[i]=0.0;
		m_petArray[i]=0.0;
		m_swcArray[i]=0.0;
		m_irrArray[i] = 0.0;
		m_precipArray[i] = 0.0;
		m_tmxArray[i]=0.0;
		m_tmnArray[i]=0.0;
		m_irrToSWATArray[i] = 0.0;
		m_gwRechargeArray[i] = 0.0;

	   }
	InitSWAT_Year();
	CString msg;
	msg.Format("SWAT_Adapter::Simulation complete (%i years)", pFlowContext->pEnvContext->yearOfRun);
	Report::LogInfo(msg);
	return TRUE;
}
bool SWAT_Flow_Adapter::RunDay(FlowContext* pFlowContext)
{
	m_nhru = m_hruOffsetArray.GetSize();
	//3840
	double* irrEfficiency = static_cast<double*>(m_irrEfficiency);
	float prec = 0; float tmn = 0; float tmx = 0.0f; float gwr=0.0f;

	int hruCount = pFlowContext->pFlowModel->GetHRUCount();
	for (int i=0;i< m_hruOffsetArray.GetSize();i++)
	   {
	   //for each hru that corresponds to a swat hru, get climate data
		HRU* pHRU = pFlowContext->pFlowModel->GetHRU(m_hruOffsetArray[i]->hru_offset);//has been sorted by SWAT hru number
		pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, pFlowContext->dayOfYear, tmx);//C
		pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, pFlowContext->dayOfYear, tmn);//C
		pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, pFlowContext->dayOfYear, prec);//mm
		m_precipArray[i] = prec;
		m_tmxArray[i] = tmx;
		m_tmnArray[i]=tmn;
		m_gwRechargeArray[i] = gwr;

		m_irrArray[i]=1.25f;
	   }

	m_iGrid = pFlowContext->dayOfYear;
	//idplt(ihru) = ncrp
	//idplt is array of size m_nhru. It is the offset in the crop database of that hrus landcover.
	//we need to modify that offset if landcover changes.
	RunSWAT_Day(&m_iGrid, m_precipArray, m_tmxArray, m_tmnArray, &m_nhru, m_lulcArray, m_gwRechargeArray, m_petArray, m_aetArray, m_swcArray, m_irrArray);

	//pass some SWAT results back to flow
	for (int i = 0; i < m_hruOffsetArray.GetSize(); i++)
	   {
		//for each hru that corresponds to a swat hru, get climate data
		HRU* pHRU = pFlowContext->pFlowModel->GetHRU(m_hruOffsetArray[i]->hru_offset);//has been sorted by SWAT hru number
		pHRU->m_swc= m_swcArray[i];
		pHRU->m_currentMaxET = m_petArray[i];
		pHRU->m_currentET = m_aetArray[i];
		pHRU->m_irr = m_irrArray[i];
		int stop=1;
	  }

	//CString msg;
	//msg.Format("SWAT_Adapter::Day %i of Year %i done", pFlowContext->pEnvContext->currentYear, pFlowContext->pEnvContext->currentYear);
	//Report::LogInfo(msg);
	return TRUE;
}
bool SWAT_Flow_Adapter::EndYear(EnvContext* pContext)
{
	EndRunSWAT();

	delete [] m_precipArray ;
	delete[] m_tmxArray ;
	delete[] m_tmnArray ;
	delete[] m_petArray ;
	delete[] m_aetArray ;
	delete[] m_swcArray ;
	delete[] m_irrArray ;
	delete[] m_gwRechargeArray ;
	delete[] m_lulcArray ;
	delete[] m_irrToSWATArray;
	delete[] m_irrEfficiency;
	CString msg;
	msg.Format("SWAT_Adapter::Simulation complete (%i years)", pContext->yearOfRun);
	Report::LogInfo(msg);
	return TRUE;
}


void SWAT_Flow_Adapter::RunSWAT_All()
{
	m_RunSWAT_AllFn();
}
void SWAT_Flow_Adapter::RunSWAT()
{
	m_RunSWATFn();
}
void SWAT_Flow_Adapter::RunSWAT_Day(int* jiunit, void* prec, void* tmx, void* tmn, int* nhru, void* lulc, void* gwRecharge, void* pet, void* aet, void* swc, void* irr)
{
	m_RunSWAT_DayFn(jiunit, prec, tmx, tmn, nhru, lulc, gwRecharge, pet, aet, swc, irr);
}
void SWAT_Flow_Adapter::InitSWAT_Year()
{
	m_InitSWAT_YearFn();
}
void SWAT_Flow_Adapter::InitSWAT(int* jiunit, int* enviGrid, void* envCC, char* name, int* sz)
{
	m_InitSWATFn(jiunit, enviGrid, envCC, name, sz);
}
void SWAT_Flow_Adapter::EndRunSWAT()
{
	m_EndRunSWATFn();
}


bool SWAT_Flow_Adapter::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
{

	// have xml string, start parsing
	TiXmlDocument doc;
	bool ok = doc.LoadFile(filename);

	if (!ok)
	{
		Report::ErrorMsg(doc.ErrorDesc());
		return false;
	}

	// start interating through the nodes
	TiXmlElement* pXmlRoot = doc.RootElement();  // <acute_hazards>
	CString codePath;


	XML_ATTR attrs[] = {
		// attr            type           address            isReq checkCol
		{ "swat_path",    TYPE_CSTRING,   &m_swatPath,     true,  0 },
		{ NULL,           TYPE_NULL,     NULL,          false, 0 } };
	ok = TiXmlGetAttributes(pXmlRoot, attrs, m_swatPath, NULL);
	return true;

}

/* qsort int comparison function */
int int_cmp(const void* a, const void* b)
{
	//const int* ia = (const int*)a; // casting pointer types 
	//const int* ib = (const int*)b;
	//return *ia - *ib;

	SWAT_OFFSET_INFO** pInfo0 = (SWAT_OFFSET_INFO**)a;
	SWAT_OFFSET_INFO** pInfo1 = (SWAT_OFFSET_INFO**)b;

	return (((*pInfo0)->swat_HRU_Number - (*pInfo1)->swat_HRU_Number) > 0) ? 1 : -1;
	/* integer comparison: returns negative if b > a
	and positive if a > b */
}
