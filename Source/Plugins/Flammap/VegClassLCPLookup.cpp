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
/* VegClassLCPLookup.cpp
**
** History:
** 2010.03.17 - tjs - started coding
** 2011.03.21 - tjs - adapted from existing code
** 2011.03.23 - tjs - added error handling
*/

#include "stdafx.h"
#pragma hdrstop

#include "VegClassLCPLookup.h"
#include <Vdataobj.h>
#include <stdio.h>
#include <cstring>
#include <cctype>

#include "FlamMapAP.h"

extern FlamMapAP *gpFlamMapAP;


VegClassLCPLookup::VegClassLCPLookup(const char *pcFName, const int canopyCoverUnits, const int canopyHeightUnits, const int canopyBaseHeightUnits, const int canopyBulkDensityUnits)
{
	VDataObj inData(U_UNDEFINED);

	int recordsTtl;
	int
		VegClass = 0,
		Variant = 0,
		FuelModel = 0,
		CanopyCover = 0,
		StandHeight = 0,
		BaseHeight = 0,
		region = 0,
		pvt = 0,
		FModelPlus = -1;
	int vegClassCol, variantCol, fuelModelCol, canopyCoverCol,
		standHeightCol, baseHeightCol, bulkDensityCol, fmodelPlusCol, regionCol, pvtCol;
	float
		BulkDensity = 0;
	bool
		status = true;

	m_InvalidValue = -9999;
	m_UnburnableValue = 99;

	if ((recordsTtl = (int)inData.ReadAscii(pcFName, ',', true)) <= 0)
	{
		gpFlamMapAP->m_runStatus = 0;
		return;
	}
	//get necessary columns
	vegClassCol = inData.GetCol("VEGCLASS");
	variantCol = inData.GetCol("VARIANT");
	fuelModelCol = inData.GetCol("LCP_FUEL_MODEL");
	canopyCoverCol = inData.GetCol("LCP_CNPY_COV");
	standHeightCol = inData.GetCol("LCP_CNPY_HT");
	baseHeightCol = inData.GetCol("LCP_CNPY_BS_HT");
	bulkDensityCol = inData.GetCol("LCP_CNPY_BLK_DNS");
	fmodelPlusCol = inData.GetCol("FMODELPLUS");
	regionCol = inData.GetCol("REGION");
	pvtCol = inData.GetCol("PVT");
	if (vegClassCol < 0 || variantCol < 0 || fuelModelCol < 0
		|| canopyCoverCol < 0 || standHeightCol < 0 || baseHeightCol < 0
		|| bulkDensityCol < 0)
	{
		gpFlamMapAP->m_runStatus = 0;
		return;
	}
	for (int i = 0; i < recordsTtl; i++)
	{
		if (!inData.Get(vegClassCol, i, VegClass))
		{
			gpFlamMapAP->m_runStatus = 0;
			return;
		}

		if (!inData.Get(variantCol, i, Variant))
		{
			gpFlamMapAP->m_runStatus = 0;
			return;
		}

		if (!inData.Get(fuelModelCol, i, FuelModel))
		{
			gpFlamMapAP->m_runStatus = 0;
			return;
		}
		if (fmodelPlusCol < 0)
		{
			FModelPlus = -1;
		}
		else
		{
			if (!inData.Get(fmodelPlusCol, i, FModelPlus))
			{
				gpFlamMapAP->m_runStatus = 0;
				return;
			}
		}
		if (regionCol < 0)
		{
			region = 0;
		}
		else
		{
			if (!inData.Get(regionCol, i, region))
			{
				gpFlamMapAP->m_runStatus = 0;
				return;
			}
		}
		if (pvtCol < 0)
		{
			pvt = 0;
		}
		else
		{
			if (!inData.Get(pvtCol, i, pvt))
			{
				gpFlamMapAP->m_runStatus = 0;
				return;
			}
		}
		if (!inData.Get(canopyCoverCol, i, CanopyCover))
		{
			gpFlamMapAP->m_runStatus = 0;
			return;
		}

		if (!inData.Get(standHeightCol, i, StandHeight))
		{
			gpFlamMapAP->m_runStatus = 0;
			return;
		}

		if (!inData.Get(baseHeightCol, i, BaseHeight))
		{
			gpFlamMapAP->m_runStatus = 0;
			return;
		}

		if (!inData.Get(bulkDensityCol, i, BulkDensity))
		{
			gpFlamMapAP->m_runStatus = 0;
			return;
		}

		//convert canopycover, standheight, baseheight, and bulkdensity from input units to lcp units (std)
		switch (canopyCoverUnits)
		{//want in percent
		case 0: //convert categories to percent
		{
			short tCover = 0;
			switch (CanopyCover)
			{
			case 1:
				tCover = 10;
				break;
			case 2:
				tCover = 30;
				break;
			case 3:
				tCover = 60;
				break;
			case 4:
				tCover = 75;
				break;
			default:
				tCover = 0;
			}
			CanopyCover = tCover;
			break;
		}
		case 1://leave alone
		default:
			break;
		}

		switch (canopyHeightUnits)
		{//want meters * 10
		case 1://meters to meters * 10
			StandHeight *= 10;
			break;
		case 2://feet to meters * 10
			StandHeight = (int)(StandHeight * 3.048f);
			break;
		case 3://leave alone
			break;
		case 4://feet * 10 to meters * 10
			StandHeight = (int)(StandHeight * 0.3048f);
			break;
		}

		switch (canopyBaseHeightUnits)
		{//want meters * 10
		case 1://meters to meters * 10
			BaseHeight *= 10;
			break;
		case 2://feet to meters * 10
			BaseHeight = (int)(BaseHeight * 3.048f);
			break;
		case 3://leave alone
			break;
		case 4://feet * 10 to meters * 10
			BaseHeight = (int)(BaseHeight * 0.3048f);
			break;
		}

		switch (canopyBulkDensityUnits)
		{//want kg/m^3 * 100
		case 1://kg/m^3 to kg/m^3 * 100
			BulkDensity *= 100.0f;
			break;
		case 2://lb/ft^3 to kg/m^3 * 100
			BulkDensity = BulkDensity * 1601.8463f;
			break;
		case 3://leave alone
			break;
		case 4:// lb/ft^3 x 1000 to kg/m^3 * 100
			BulkDensity = BulkDensity  * 0.16018463f;
			break;
		}

		//Canopy integrity check - if any values invalid set all to invalid (nodata)
		if (CanopyCover <= 0 || StandHeight <= 0 || BaseHeight < 0 || BulkDensity <= 0.0)
		{
			CanopyCover = StandHeight = BaseHeight = m_InvalidValue;
			BulkDensity = m_InvalidValue;
		}

		AddValues(
			VegClass,
			region, pvt,
			Variant,
			(short)FuelModel,
			(short)CanopyCover,
			(short)StandHeight,
			(short)BaseHeight,
			(short)(BulkDensity),
			(short)FModelPlus);
	} // for(int i=0;i<recordsTtl;i++) {
	//Output("F:\\vegclasslookup.csv");
} // VegClassLCPLookup::VegClassLCPLookup(const char *fName);


void VegClassLCPLookup::AddValues(
	int VegClass,
	int Variant,
	short FuelModel,
	short CanopyCover,
	short StandHeight,
	short BaseHeight,
	short BulkDensity,
	short FModelPlus)
{
	unsigned long
		key;

	LookupValues *values = new LookupValues(
		FuelModel,
		CanopyCover,
		StandHeight,
		BaseHeight,
		BulkDensity,
		FModelPlus);

	// Key combines VegClass and Variant
	key = (unsigned long)VegClass * 10000 + (unsigned long)Variant;

	m_MapVegClassLCPs[key] = values;
} // void VegClassLCPLookup::AddValues(...

void VegClassLCPLookup::AddValues(
	int VegClass,
	int Region,
	int Pvt,
	int Variant,
	short FuelModel,
	short CanopyCover,
	short StandHeight,
	short BaseHeight,
	short BulkDensity,
	short FModelPlus)
{
	__int64
		key;

	LookupValues *values = new LookupValues(
		FuelModel,
		CanopyCover,
		StandHeight,
		BaseHeight,
		BulkDensity,
		FModelPlus);

	// Key combines VegClass and Variant
	//key = (unsigned long)VegClass * 100000 + (unsigned long)Region * 1000 + (unsigned long)Pvt * 10 + (unsigned long)Variant;
	key = GetKey(VegClass, Region, Pvt, Variant);
	m_MapVegClassLCPs[key] = values;
} // void VegClassLCPLookup::AddValues(...

__int64 VegClassLCPLookup::GetKey(int VegClass, int Region, int Pvt, int Variant)
{
	__int64
		key = (__int64)VegClass * 100000 + (__int64)Region * 1000 + (__int64)Pvt * 10 + (__int64)Variant;
	return key;
}

VegClassLCPLookup::~VegClassLCPLookup()
{
	map<__int64, LookupValues *>::iterator iter;

	for (iter = m_MapVegClassLCPs.begin(); iter != m_MapVegClassLCPs.end(); iter++)
	{
		delete iter->second;
		// m_MapVegClassLCPs.erase(iter);
	}

	m_MapVegClassLCPs.erase(m_MapVegClassLCPs.begin(), m_MapVegClassLCPs.end());
} // short VegClassLCPLookup::~LCPLookup();


short VegClassLCPLookup::GetFuelModel(const int VegClass, const int Variant, const int useFModelPlus)
{
	short rtrn = m_UnburnableValue;
	unsigned long key = (unsigned long)VegClass * 10000 + (unsigned long)Variant;

	if (m_MapVegClassLCPs[key] != NULL)
	{
		if (useFModelPlus && m_MapVegClassLCPs[key]->FuelModelPlus > 0)
			rtrn = m_MapVegClassLCPs[key]->FuelModelPlus;
		else
			rtrn = m_MapVegClassLCPs[key]->FuelModel;
	}
	return rtrn;
} // short LCPLookup::GetFuelModel(const int VegClass);


short VegClassLCPLookup::GetCanopyCover(const int VegClass, const int Variant)
{
	short rtrn = m_InvalidValue;
	unsigned long key = (unsigned long)VegClass * 10000 + (unsigned long)Variant;

	if (m_MapVegClassLCPs[key] != NULL)
		rtrn = m_MapVegClassLCPs[key]->CanopyCover;
	return rtrn;
} // short LCPLookup::GetCanopyCover(const int VegClass);


short VegClassLCPLookup::GetStandHeight(const int VegClass, const int Variant)
{
	short rtrn = m_InvalidValue;
	unsigned long key = (unsigned long)VegClass * 10000 + (unsigned long)Variant;

	if (m_MapVegClassLCPs[key] != NULL)
		rtrn = m_MapVegClassLCPs[key]->StandHeight;

	return rtrn;
} // short VegClassLCPLookup::GetStandHeight(const int VegClass);

short VegClassLCPLookup::GetBaseHeight(const int VegClass, const int Variant)
{
	short rtrn = m_InvalidValue;
	unsigned long key = (unsigned long)VegClass * 10000 + (unsigned long)Variant;

	if (m_MapVegClassLCPs[key] != NULL)
		rtrn = m_MapVegClassLCPs[key]->BaseHeight;

	return rtrn;
} // short VegClassLCPLookup::GetBulkDensity(const int VegClass);


short VegClassLCPLookup::GetBulkDensity(const int VegClass, const int Variant)
{
	short rtrn = m_InvalidValue;
	unsigned long key = (unsigned long)VegClass * 10000 + (unsigned long)Variant;

	if (m_MapVegClassLCPs[key] != NULL)
		rtrn = m_MapVegClassLCPs[key]->BulkDensity;
	return rtrn;
} // short VegClassLCPLookup::GetBulkDensity(const int VegClass);

short VegClassLCPLookup::GetFuelModel(const int VegClass, const int Region, const int Pvt, const int Variant, const int useFModelPlus)
{
	__int64 key = GetKey(VegClass, Region, Pvt, Variant);
	short rtrn = m_UnburnableValue;

	if (m_MapVegClassLCPs[key] != NULL)
	{
		if (useFModelPlus && m_MapVegClassLCPs[key]->FuelModelPlus > 0)
			rtrn = m_MapVegClassLCPs[key]->FuelModelPlus;
		else
			rtrn = m_MapVegClassLCPs[key]->FuelModel;
	}
	return rtrn;
}
short VegClassLCPLookup::GetCanopyCover(const int VegClass, const int Region, const int Pvt, const int Variant)
{
	short rtrn = m_InvalidValue;
	__int64 key = GetKey(VegClass, Region, Pvt, Variant);
	if (m_MapVegClassLCPs[key] != NULL)
		rtrn = m_MapVegClassLCPs[key]->CanopyCover;
	return rtrn;
}
short VegClassLCPLookup::GetStandHeight(const int VegClass, const int Region, const int Pvt, const int Variant)
{
	short rtrn = m_InvalidValue;
	__int64 key = GetKey(VegClass, Region, Pvt, Variant);
	if (m_MapVegClassLCPs[key] != NULL)
		rtrn = m_MapVegClassLCPs[key]->StandHeight;
	return rtrn;
}
short VegClassLCPLookup::GetBaseHeight(const int VegClass, const int Region, const int Pvt, const int Variant)
{
	short rtrn = m_InvalidValue;
	__int64 key = GetKey(VegClass, Region, Pvt, Variant);
	if (m_MapVegClassLCPs[key] != NULL)
		rtrn = m_MapVegClassLCPs[key]->BaseHeight;
	return rtrn;
}
short VegClassLCPLookup::GetBulkDensity(const int VegClass, const int Region, const int Pvt, const int Variant)
{
	short rtrn = m_InvalidValue;
	__int64 key = GetKey(VegClass, Region, Pvt, Variant);
	if (m_MapVegClassLCPs[key] != NULL)
		rtrn = m_MapVegClassLCPs[key]->BulkDensity;
	return rtrn;
}

void VegClassLCPLookup::Output(const char *fName)
{       // writes data in readable form
	CString
		msg;
	int
		entryCnt = 0;

	FILE *oFile = fopen(fName, "w");
	if (oFile == NULL)
	{
		CString msg("Flammap: Error opening file ");
		msg += fName;
		Report::ErrorMsg(msg);
		return;
	}

	// write the m_nInvalidValue
	//fprintf(oFile, "m_InvalidValue: %d\n", m_InvalidValue);
	// fprintf(oFile, "m_UnburnableValue: %d\n", m_UnburnableValue);

	// How many states are being written?
	int objectCount = (int)m_MapVegClassLCPs.size();
	fprintf(oFile, "Key, VegClass, Region, PVT, Variant, FuelModel, CanopyCover, StandHeight, BaseHeight, BulkDensity\n");

	__int64 vc, reg, pvt, var;
	map<__int64, LookupValues *>::iterator iter;
	for (iter = m_MapVegClassLCPs.begin(); iter != m_MapVegClassLCPs.end(); iter++)
	{
		//entryCnt++;
		fprintf(oFile, "%I64d, ", iter->first);
		// fprintf(oFile, "VegClass and Variant: %I64d\n", iter->first);
		vc = iter->first / (__int64)100000;
		reg = (iter->first - vc * (__int64)100000) / (__int64)1000;
		pvt = (iter->first - vc * (__int64)100000 - reg * (__int64) 1000) / (__int64)10;
		//reg = iter->first % ((__int64) 10000)
		var = iter->first % ((__int64)10);
		fprintf(oFile, "%I64d, %I64d, %I64d, %I64d, ", vc, reg, pvt, var);
		fprintf(oFile, "%d, %d, %d, %d, %d\n", iter->second->FuelModel, iter->second->CanopyCover, 
			iter->second->StandHeight, iter->second->BaseHeight, iter->second->BulkDensity);
		/*fprintf(oFile, "  fuelModel:   %d\n", iter->second->FuelModel);
		fprintf(oFile, "  canopyCover: %d\n", iter->second->CanopyCover);
		fprintf(oFile, "  standHeight: %d\n", iter->second->StandHeight);
		fprintf(oFile, "  baseHeight:  %d\n", iter->second->BaseHeight);
		fprintf(oFile, "  bulkDensity: %d\n", iter->second->BulkDensity);*/
	} // for(iter=m_MapVegClassLCPs.begin(); iter!=m_MapVegClassLCPs.end(); iter++)

	// fprintf(oFile, "NumberOfStates: %d\n", entryCnt);

	fclose(oFile);
} // void VegClassLCPLookup::Output(const char *fName)
