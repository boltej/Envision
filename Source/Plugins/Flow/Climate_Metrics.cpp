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
	{
	this->m_timing = GMT_START_STEP;
	}


Climate_Metrics::~Climate_Metrics()
{

}

bool Climate_Metrics::Init(FlowContext* pFlowContext)
{
	MapLayer* pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

	int iduCount = pIDULayer->GetRecordCount();

	pIDULayer->CheckCol(m_colTemp90, _T("Temp90"), TYPE_INT, CC_AUTOADD);
	pIDULayer->CheckCol(m_colTemp70, _T("Temp70"), TYPE_INT, CC_AUTOADD);

	//bool readOnlyFlag = pIDULayer->m_readOnly;
	//pIDULayer->m_readOnly = false;
	//pIDULayer->SetColData(m_colDailyUrbanDemand, VData(0), true);
	//pIDULayer->m_readOnly = readOnlyFlag;

	pFlowContext->pFlowModel->AddOutputVar(_T("#Days High > 90 Degrees F"), m_maxYearlyTemp90, "");
	pFlowContext->pFlowModel->AddOutputVar(_T("#Days Low > 70 Degrees F"), m_minYearlyTemp70, "");
	return TRUE;
}

bool Climate_Metrics::StartYear(FlowContext* pFlowContext)
{
	MapLayer* pLayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
	CalcClimateMetrics(pFlowContext);
	
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
	for (int hruIndex = 0; hruIndex < hruCount; hruIndex++)
		{
		HRU* pHRU = pFlowContext->pFlowModel->GetHRU(hruIndex);
		for (int doy = 0; doy < 365; doy++)
			{
			pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, doy , tMax);
			pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
			if (tMax >= 32.220f)
				numDays++;
			if (tMin > 21.11)
				numDaysAbove70++;
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
			}
		numDays = 0;
		numDaysAbove70 = 0;
		}
	m_maxYearlyTemp90 = maxYearlyTemp90;
	m_minYearlyTemp70 = minYearlyTemp70;
	return TRUE;
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