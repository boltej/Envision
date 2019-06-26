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
// VDDTCvrTypeStrctStageLCPLookUp.h
// Class for converting a veg class 
// into the values used for the 5 needed layers
// of the LCP file.

#pragma once
#include <afxtempl.h>
#include <iostream>
#include <cstdio>
#include <map>
#include <MapLayer.h>
#include "ParseUtils.h"

// needed for using map
using namespace std;

class VegClassLCPLookup {
private:

	// The 5 variable layers of the 8 layer sammo
	struct LookupValues {
		short
		  FuelModel,
		  FuelModelPlus,
		  CanopyCover,
		  StandHeight,
		  BaseHeight,   // canopy base height
		  BulkDensity;  // canopy bulk density
		LookupValues(
			short v1,
			short v2,
			short v3,
			short v4,
			short v5,
			short v6)
		{
			FuelModel = v1;
			CanopyCover = v2;
			StandHeight = v3;
			BaseHeight = v4;
			BulkDensity = v5;
			FuelModelPlus = v6;
		}
	}; // struct LookupValues

	map<__int64, LookupValues *>
		m_MapVegClassLCPs;

	short 
		m_InvalidValue,
		m_UnburnableValue;

	void AddValues(
		int VegClass,
		int Variant,
		short FuelModel,
		short CanopyCover,
		short StandHeight,
		short BaseHeight,
		short BulkDensity,
		short FModelPlus);

	void AddValues(
		int VegClass,
		int Region,
		int Pvt,
		int Variant,
		short FuelModel,
		short CanopyCover,
		short StandHeight,
		short BaseHeight,
		short BulkDensity,
		short FModelPlus);
	//unsigned long GetKey(int VegClass, int Region, int Pvt, int Variant);
	public:
		~VegClassLCPLookup();
		VegClassLCPLookup(const char *fName, const int canopyCoverUnits, const int canopyHeightUnits, const int canopyBaseHeightUnits, const int canopyBulkDensityUnits);
		short GetFuelModel(const int VegClass,const int Variant, const int useFModelPlus);
		short GetCanopyCover(const int VegClass,const int Variant);
		short GetStandHeight(const int VegClass,const int Variant);
		short GetBaseHeight(const int VegClass,const int Variant);
		short GetBulkDensity(const int VegClass,const int Variant);
		short GetFuelModel(const int VegClass, const int Region, const int Pvt, const int Variant, const int useFModelPlus);
		short GetCanopyCover(const int VegClass, const int Region, const int Pvt, const int Variant);
		short GetStandHeight(const int VegClass, const int Region, const int Pvt, const int Variant);
		short GetBaseHeight(const int VegClass, const int Region, const int Pvt, const int Variant);
		short GetBulkDensity(const int VegClass, const int Region, const int Pvt, const int Variant);
		void Output(const char *fName);       // writes data in readable form
		__int64 GetKey(int VegClass, int Region, int Pvt, int Variant);

}; // class LCPLookup {



