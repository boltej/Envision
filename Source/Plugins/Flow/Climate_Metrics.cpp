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
// Climate_Metrics.cpp : Implements climate summaries for Flow
//

#include "stdafx.h"
#pragma hdrstop

#include "GlobalMethods.h"
#include "Flow.h"
#include "FlowInterface.h"
#include <UNITCONV.H>

using namespace std;

//extern FlowProcess *gpFlow;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Climate_Metrics::Climate_Metrics(FlowModel* pFlowModel, LPCTSTR name)
	: GlobalMethod(pFlowModel, name, GM_CLIMATE_METRICS)
	, m_colTemp90(-1)// Calculated Daily Urband Water Demand m3/day
	, m_colTemp70(-1)
	, m_maxYearlyTemp90(0)
	, m_minYearlyTemp70(0)
	, m_meanYearlyTemp(0.0f)
	, m_meanWinterTemp(0.0f)
	, m_meanSummerTemp(0.0f)
	, m_pModeledTemperature(NULL)
	, m_pCoeff(NULL)
	, m_maxYearlyStreamTemp(0.0f)
{
	this->m_timing = GMT_START_STEP;
}


Climate_Metrics::~Climate_Metrics()
{
	if (m_pModeledTemperature)
		delete  m_pModeledTemperature;

	if (m_pCoeff)
		delete  m_pCoeff;
}

bool Climate_Metrics::Init(FlowContext* pFlowContext)
{
	MapLayer* pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

	int iduCount = pIDULayer->GetRecordCount();

	pIDULayer->CheckCol(m_colTemp90, _T("Temp90"), TYPE_INT, CC_AUTOADD);
	pIDULayer->CheckCol(m_colTemp70, _T("Temp70"), TYPE_INT, CC_AUTOADD);


	pFlowContext->pFlowModel->AddOutputVar(_T("#Days High > 90 Degrees F"), m_maxYearlyTemp90, "");
	pFlowContext->pFlowModel->AddOutputVar(_T("#Days Low > 70 Degrees F"), m_minYearlyTemp70, "");
	pFlowContext->pFlowModel->AddOutputVar(_T("Avg. Daily Air Temperature"), m_meanYearlyTemp, "");
	pFlowContext->pFlowModel->AddOutputVar(_T("JFM - Avg. Daily Air Temperature"), m_meanWinterTemp, "");
	pFlowContext->pFlowModel->AddOutputVar(_T("JAS - Avg. Daily Air Temperature"), m_meanSummerTemp, "");
	int numYears= pFlowContext->pEnvContext->endYear - pFlowContext->pEnvContext->startYear;

	LPCTSTR filename = "CIG_Temp_Coefficients_UTM.csv";
	m_pCoeff = new VDataObj(13,0, U_UNDEFINED);
	m_pCoeff->ReadAscii(filename, _T(',')); 
	m_pModeledTemperature = new FDataObj(m_pCoeff->GetRowCount()+1, 0, 0.0f, U_UNDEFINED);
	pFlowContext->pFlowModel->AddOutputVar("Simulated Steam Temperature", m_pModeledTemperature, "");
	pFlowContext->pFlowModel->AddOutputVar(_T("Max Summer Stream Temperature"), m_maxYearlyStreamTemp, "");
	LPCTSTR name = "Date";
	m_pModeledTemperature->SetLabel(0, name);
	for (int i=0;i<m_pCoeff->GetRowCount();i++)
	   { 
		CString name = m_pCoeff->GetAsString(3, i);
	   m_pModeledTemperature->SetLabel(i+1, (LPCTSTR)name);
	   }


	m_climateIndex.SetSize(1 + static_cast<INT_PTR>(m_pCoeff->GetRowCount()));   // includes 'year' and week
	for (int i=0;i<m_climateIndex.GetSize();i++)
	   m_climateIndex[i]=-1;

	return TRUE;
}

bool Climate_Metrics::InitRun(FlowContext* pFlowContext)
   {
	m_pModeledTemperature->ClearRows();
	m_pModeledTemperature->SetSize(m_pModeledTemperature->GetColCount(), 0);
	return true;
   }

bool Climate_Metrics::StartYear(FlowContext* pFlowContext)
{
	MapLayer* pLayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
	//CalcClimateMetrics(pFlowContext);
	//GetCIGWaterTemperature(pFlowContext);
	return true;
}
bool Climate_Metrics::EndYear(FlowContext* pFlowContext)
{
	MapLayer* pLayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
	CalcClimateMetrics(pFlowContext);
	GetCIGWaterTemperature(pFlowContext);
	return true;
}


bool Climate_Metrics::StartStep(FlowContext* pFlowContext)
{

	return true;
}


bool Climate_Metrics::EndStep(FlowContext* pFlowContext)
{


	return true;
}

bool Climate_Metrics::CalcClimateMetrics(FlowContext* pFlowContext)
{
	MapLayer* pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
	int hruCount = pFlowContext->pFlowModel->GetHRUCount();
	float tMax = 0.f; // local tMax
	float tMin = 0.0f;
	int numDays = 0;  //local numDays above 90
	int maxYearlyTemp90 = 0; int numDaysAbove70 = 0;
	int minYearlyTemp70 = 0;
	float meanTemp = 0;
	float JFMTemp = 0.0f;
	float JASTemp = 0.0f;
	int numHRU = 0;
	for (int hruIndex = 0; hruIndex < hruCount; hruIndex++)
	{
		HRU* pHRU = pFlowContext->pFlowModel->GetHRU(hruIndex);
		numHRU++;
		for (int doy = 0; doy < 365; doy++)
		{
			pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, doy, tMax);
			pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
			if (tMax >= 32.220f)
				//if (tMax >= 43.330f)
				numDays++;
			if (tMin > 21.11)
				numDaysAbove70++;
			meanTemp += (tMax + tMin) / 2.0f;
			if (doy >= 0 && doy < 90)
				JFMTemp += (tMax + tMin) / 2.0f;
			if (doy >= 182 && doy < 274)
				JASTemp += (tMax + tMin) / 2.0f;

		}
		if (numDays > maxYearlyTemp90)
			maxYearlyTemp90 = numDays;
		if (numDaysAbove70 > minYearlyTemp70)
			minYearlyTemp70 = numDaysAbove70;

		for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
		{
			int idu = pHRU->m_polyIndexArray[k];
			pFlowContext->pFlowModel->UpdateIDU(pFlowContext->pEnvContext, idu, m_colTemp90, numDays, ADD_DELTA);
			pFlowContext->pFlowModel->UpdateIDU(pFlowContext->pEnvContext, idu, m_colTemp70, numDaysAbove70, ADD_DELTA);
			//	pFlowContext->pFlowModel->UpdateIDU(pFlowContext->pEnvContext, idu, m_colTemp90, numDays, ADD_DELTA);  
			//	pFlowContext->pFlowModel->UpdateIDU(pFlowContext->pEnvContext, idu, m_colTemp70, numDaysAbove70, ADD_DELTA);
		}
		numDays = 0;
		numDaysAbove70 = 0;
	}
	m_maxYearlyTemp90 = maxYearlyTemp90;
	m_minYearlyTemp70 = minYearlyTemp70;
	m_meanYearlyTemp = meanTemp / 365 / numHRU;
	m_meanWinterTemp = JFMTemp / 90 / numHRU;
	m_meanSummerTemp = JASTemp / 90 / numHRU;



	return TRUE;
}

int Climate_Metrics::GetCIGWaterTemperature(FlowContext* pFlowContext)
   {
	ClimateDataInfo* pInfo = pFlowContext->pFlowModel->GetClimateDataInfo(CDT_TMAX);
	ClimateDataInfo* pInfo2 = pFlowContext->pFlowModel->GetClimateDataInfo(CDT_TMIN);
	//loop through cig parameter file
	CArray<float, float> modeledTemperatureData;
	int siteCount = m_pCoeff->GetRowCount();
	modeledTemperatureData.SetSize(1 + static_cast<INT_PTR>(siteCount));   // includes 'year' and week
	memset(modeledTemperatureData.GetData(), 0, (1 + static_cast<unsigned long long>(siteCount)) * sizeof(float));


	float maxTemp = 20.0f;

	int doy=0;
	float avWkTemp=0;
	for (int wk = 0; wk < 52; wk++)
	   {
		modeledTemperatureData[0] = (float)pFlowContext->pEnvContext->currentYear + (float)wk/52.0f;   // length = 2+numsites

		for (int site = 0; site < siteCount; site++)
			{		
			float x = m_pCoeff->GetAsFloat(11, site);
			float y = m_pCoeff->GetAsFloat(12, site);
			float mu = m_pCoeff->GetAsFloat(9, site); 
		    float gamma = m_pCoeff->GetAsFloat(8, site);
			float beta = m_pCoeff->GetAsFloat(7, site);
			float alpha = m_pCoeff->GetAsFloat(6, site);
			//tw = mu + (alpha - mu) / 1 + exp^(gamma(beta-Tair))
			float weekTempMin=0.0f; float weekTempMax = 0.0f;
			for (int dy = 0; dy < 7; dy++)
				{ 
				if (m_climateIndex[site] < 0)
				   weekTempMax += (pInfo->m_pNetCDFData->Get(x, y, m_climateIndex[site], doy+dy, pFlowContext->pFlowModel->m_projectionWKT, false))/7.0f;
			   else
					weekTempMax += (pInfo->m_pNetCDFData->Get(m_climateIndex[site], doy + dy)) / 7.0f;
					
				weekTempMin += (pInfo2->m_pNetCDFData->Get(m_climateIndex[site], doy + dy)) / 7.0f;
				}
         //for this site, we have the weekly temp.  Calculate water temp.
			float weekTemp = (weekTempMax+weekTempMin)/2.0f;
			float tw = mu + ((alpha - mu) / (1 + exp(gamma*(beta - weekTemp))));
			modeledTemperatureData[static_cast<INT_PTR>(site)+1] = tw;
			if (tw>maxTemp)
			   maxTemp=tw;
			}//end of sites
		doy += 7;
		m_pModeledTemperature->AppendRow(modeledTemperatureData);
		}
	m_maxYearlyStreamTemp = maxTemp;
	return 0;
   }

Climate_Metrics* Climate_Metrics::LoadXml(TiXmlElement* pXmlClimateMetrics, MapLayer* pIDULayer, LPCTSTR filename, FlowModel* pFlowModel)
{
	if (pXmlClimateMetrics == NULL)
		return NULL;

	// get attributes
	LPTSTR name = NULL;
	LPTSTR method = NULL;
	bool use = true;

	XML_ATTR attrs[] = {
		// attr                 type          address               isReq  checkCol
		{ _T("name"), TYPE_STRING, &name, false, 0 },
		{ _T("method"), TYPE_STRING, &method, true, 0 },
		{ _T("use"), TYPE_BOOL, &use, false, 0 },
		{ NULL, TYPE_NULL, NULL, false, 0 } };

	bool ok = TiXmlGetAttributes(pXmlClimateMetrics, attrs, filename, pIDULayer);
	if (!ok)
	{
		CString msg;
		msg.Format(_T("Flow: Misformed element reading <ClimateMetrics> attributes in input file %s"), filename);
		Report::ErrorMsg(msg);
		return NULL;
	}

	Climate_Metrics* pClimateMetrics = new Climate_Metrics(pFlowModel, name);

	if (method != NULL)
	{
		switch (method[0])
		{
		case _T('t'):
		case _T('T'):
			pClimateMetrics->SetMethod(GM_CLIMATE_METRICS);
			break;
		default:
			pClimateMetrics->SetMethod(GM_NONE);
		}
	}

	return pClimateMetrics;
}