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
// DailyUrbanWaterDemand.cpp : Implements global daily urban water demands methods for Flow
//

#include "stdafx.h"
#pragma hdrstop

#include "GlobalMethods.h"
#include "Flow.h"
#include "FlowInterface.h"
#include <UNITCONV.H>

using namespace std;

extern FlowProcess *gpFlow;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DailyUrbanWaterDemand::DailyUrbanWaterDemand(LPCTSTR name)
: GlobalMethod(name, GM_URBAN_DEMAND_DY)
, m_colDailyUrbanDemand(-1)// Calculated Daily Urband Water Demand m3/day
, m_colH2OResidnt(-1)		// Annual Residential Demand ccf/day/acre
, m_colH2OIndComm(-1)		// Annual Industrial & commercial Demand ccf/day/acre
, m_colIDUArea(-1)			// IDU area from IDU layer
, m_colUGB(-1)					// IDU UGB from IDU layer	
, m_colWrexist(-1)         // IDU attribute from IDU layer with WR information	
, m_iduMetroWaterDmdDy(-1)  // UGB 40 Metro daily urban water demand ccf/day
, m_iduEugSpWaterDmdDy(-1)	 // UGB 22 EugSp daily urban water demand ccf/day
, m_iduSalKiWaterDmdDy(-1)	 // UGB 51 Salki daily urban water demand ccf/day
, m_iduCorvlWaterDmdDy(-1)	 // UGB 12 Corvl daily urban water demand ccf/da
, m_iduAlbnyWaterDmdDy(-1)	 // UGB 02 Albny daily urban water demand ccf/day
, m_iduMcminWaterDmdDy(-1)	 // UGB 39 Mcmin daily urban water demand ccf/day
, m_iduNewbrWaterDmdDy(-1)	 // UGB 47 Newbr daily urban water demand ccf/day
, m_iduWoodbWaterDmdDy(-1)	 // UGB 71 Woodb daily urban water demand ccf/day


{
	this->m_timing = GMT_START_STEP;
}


DailyUrbanWaterDemand::~DailyUrbanWaterDemand()
{

}

bool DailyUrbanWaterDemand::Init(FlowContext *pFlowContext)
{
	MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

	int iduCount = pIDULayer->GetRecordCount();

	pIDULayer->CheckCol(m_colH2OResidnt, _T("H2ORESIDNT"), TYPE_FLOAT, CC_MUST_EXIST);
	pIDULayer->CheckCol(m_colH2OIndComm, _T("H2OINDCOMM"), TYPE_FLOAT, CC_MUST_EXIST);
	pIDULayer->CheckCol(m_colDailyUrbanDemand, _T("UDMAND_DY"), TYPE_FLOAT, CC_MUST_EXIST);
	pIDULayer->CheckCol(m_colIDUArea, _T("AREA"), TYPE_FLOAT, CC_MUST_EXIST);
	pIDULayer->CheckCol(m_colUGB, _T("UGB"), TYPE_INT, CC_MUST_EXIST);   // urban water 
	pIDULayer->CheckCol(m_colWrexist, _T("WREXISTS"), TYPE_INT, CC_MUST_EXIST);

	bool readOnlyFlag = pIDULayer->m_readOnly;
	pIDULayer->m_readOnly = false;
	pIDULayer->SetColData(m_colDailyUrbanDemand, VData(0), true);
	pIDULayer->m_readOnly = readOnlyFlag;

	this->m_timeSeriesMunDemandSummaries.SetName(_T("Daily Urban Water Demand Summary"));
	this->m_timeSeriesMunDemandSummaries.SetSize(9, 0);
	this->m_timeSeriesMunDemandSummaries.SetLabel(0, _T("Time (days)"));
	this->m_timeSeriesMunDemandSummaries.SetLabel(1, _T("Daily Metro Water Demand Summary (ccf/day)"));
	this->m_timeSeriesMunDemandSummaries.SetLabel(2, _T("Daily Eug-Spr Water Demand Summary (ccf/day)"));
	this->m_timeSeriesMunDemandSummaries.SetLabel(3, _T("Daily Salem-Kei Water Demand Summary (ccf/day)"));
	this->m_timeSeriesMunDemandSummaries.SetLabel(4, _T("Daily Corval Water Demand Summary (ccf/day)"));
	this->m_timeSeriesMunDemandSummaries.SetLabel(5, _T("Daily Albany Water Demand Summary (ccf/day)"));
	this->m_timeSeriesMunDemandSummaries.SetLabel(6, _T("Daily Mcminn Water Demand Summary (ccf/day)"));
	this->m_timeSeriesMunDemandSummaries.SetLabel(7, _T("Daily Newberg Water Demand Summary (ccf/day)"));
	this->m_timeSeriesMunDemandSummaries.SetLabel(8, _T("Daily Woodburn Water Demand Summary (ccf/day)"));

	gpFlow->AddOutputVar(_T("Daily Urban Water Demand Summary"), &m_timeSeriesMunDemandSummaries, "");

	return TRUE;
}

bool DailyUrbanWaterDemand::StartYear(FlowContext *pFlowContext)
	{
	MapLayer *pLayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

	m_iduMetroWaterDmdDy = 0.f;
	m_iduEugSpWaterDmdDy = 0.f;
	m_iduSalKiWaterDmdDy = 0.f;
	m_iduCorvlWaterDmdDy = 0.f;
	m_iduAlbnyWaterDmdDy = 0.f;
	m_iduMcminWaterDmdDy = 0.f;
	m_iduNewbrWaterDmdDy = 0.f;
	m_iduWoodbWaterDmdDy = 0.f;
		
	return true;
	}


bool DailyUrbanWaterDemand::StartStep(FlowContext *pFlowContext)
{
	// handle NONE, EXTERNAL cases if defined
	//if (GlobalMethod::Run(pFlowContext) == true)
	//	return true;
	
	switch (m_method)
	{
	case GM_URBAN_DEMAND_DY:

		m_iduMetroWaterDmdDy = 0.f;
		m_iduEugSpWaterDmdDy = 0.f;
		m_iduSalKiWaterDmdDy = 0.f;
		m_iduCorvlWaterDmdDy = 0.f;
		m_iduAlbnyWaterDmdDy = 0.f;
		m_iduMcminWaterDmdDy = 0.f;
		m_iduNewbrWaterDmdDy = 0.f;
		m_iduWoodbWaterDmdDy = 0.f;
	
		return CalcDailyUrbanWaterDemand(pFlowContext);

	case GM_NONE:
		return true;

	default:
		ASSERT(0);
	}

	return true;
}


bool DailyUrbanWaterDemand::EndStep(FlowContext *pFlowContext)
	{

	CArray< float, float > rowMuni;
	
	float time = pFlowContext->time - pFlowContext->pEnvContext->startYear * 365;

	rowMuni.SetSize(9);
	
	rowMuni[0] = time;
	rowMuni[1] = m_iduMetroWaterDmdDy;
	rowMuni[2] = m_iduEugSpWaterDmdDy;
	rowMuni[3] = m_iduSalKiWaterDmdDy;
	rowMuni[4] = m_iduCorvlWaterDmdDy;
	rowMuni[5] = m_iduAlbnyWaterDmdDy;
	rowMuni[6] = m_iduMcminWaterDmdDy;
	rowMuni[7] = m_iduNewbrWaterDmdDy;
	rowMuni[8] = m_iduWoodbWaterDmdDy;

	this->m_timeSeriesMunDemandSummaries.AppendRow(rowMuni);
	
	return true;	
	}

bool DailyUrbanWaterDemand::CalcDailyUrbanWaterDemand(FlowContext *pFlowContext)
{
	MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

	float ETtG = 1.f; // average summer ET (april-October) for urban lawn grass in current time step
	float ET0G = 1.f; // average summer ET (april-October) for urban lawn grass in time step 0
	int   doy = pFlowContext->dayOfYear;

	for (MapLayer::Iterator idu = pIDULayer->Begin(); idu != pIDULayer->End(); idu++)
		{
		float annualResidentialDemand = 0.f; // ccf/day
		float annualIndCommDemand = 0.f;     // ccf/day
		float iduArea = 0.f; // m2
		int   wrExists = 0; 
		
		int UGBid = 0;

		pIDULayer->GetData( idu, m_colUGB, UGBid );
		pIDULayer->GetData( idu, m_colWrexist, wrExists );
		
		unsigned __int16 iduUse = GetUse(wrExists);
		
		if ( UGBid > 0 && IsMunicipal(iduUse) )
			{
			pIDULayer->GetData( idu, m_colH2OResidnt, annualResidentialDemand ); // ccf/day
			pIDULayer->GetData( idu, m_colH2OIndComm, annualIndCommDemand ); // ccf/day
			pIDULayer->GetData( idu, m_colIDUArea, iduArea );
			
			// ccf/day
			float dailyResidentialWaterDemand = (float)(1 - 0.2 * (ETtG / ET0G) * (cos(2 * PI * doy / 365))) * annualResidentialDemand;
			
			float dailyIndCommWaterDemand = annualIndCommDemand;

			// ccf/day
			float dailyUrbanWaterDemand = dailyResidentialWaterDemand + dailyIndCommWaterDemand;

			if ( UGBid == 40 ) m_iduMetroWaterDmdDy += dailyUrbanWaterDemand;
			if ( UGBid == 22 ) m_iduEugSpWaterDmdDy += dailyUrbanWaterDemand;
			if ( UGBid == 51 ) m_iduSalKiWaterDmdDy += dailyUrbanWaterDemand;
			if ( UGBid == 12 ) m_iduCorvlWaterDmdDy += dailyUrbanWaterDemand;
			if ( UGBid == 2  ) m_iduAlbnyWaterDmdDy += dailyUrbanWaterDemand;
			if ( UGBid == 39 ) m_iduMcminWaterDmdDy += dailyUrbanWaterDemand;
			if ( UGBid == 47 ) m_iduNewbrWaterDmdDy += dailyUrbanWaterDemand;
			if ( UGBid == 71 ) m_iduWoodbWaterDmdDy += dailyUrbanWaterDemand;

			// ccf = 100 cf = 100 ft3
			// m3 = 35.3147 ft3
			// day = 86400 seconds
			// m3/sec = cf/day * 100 / 35.3147 / 86400
			dailyUrbanWaterDemand = (float) ( dailyUrbanWaterDemand  * 100 / 35.3147 / 86400 );

			bool readOnlyFlag = pIDULayer->m_readOnly;
			pIDULayer->m_readOnly = false;
			pIDULayer->SetData( idu, m_colDailyUrbanDemand, dailyUrbanWaterDemand );
			pIDULayer->m_readOnly = readOnlyFlag;
			}
		}

	return TRUE;
}

DailyUrbanWaterDemand *DailyUrbanWaterDemand::LoadXml(TiXmlElement *pXmlDailyUrbWaterDmd, MapLayer *pIDULayer, LPCTSTR filename)
{
	if (pXmlDailyUrbWaterDmd == NULL)
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

	bool ok = TiXmlGetAttributes(pXmlDailyUrbWaterDmd, attrs, filename, pIDULayer);
	if (!ok)
	{
		CString msg;
		msg.Format(_T("Flow: Misformed element reading <DailyUrbanWaterDemand> attributes in input file %s"), filename );
		Report::ErrorMsg(msg);
		return NULL;
	}

	DailyUrbanWaterDemand *pDayUrbDemand = new DailyUrbanWaterDemand(name);

	if (method != NULL)
	{
		switch (method[0])
		{
		case _T('d'):
		case _T('D'):
			pDayUrbDemand->SetMethod(GM_URBAN_DEMAND_DY);
			break;
		default:
			pDayUrbDemand->SetMethod(GM_NONE);
		}
	}

	return pDayUrbDemand;
}