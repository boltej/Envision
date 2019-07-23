///////////////////////////////////////////////////////////////////////////////////////
/// \file lamarquendep.cpp
/// \brief Functionality for reading the Lamarque Nitrogen deposition data set
///
/// $Date: 2014-06-16 15:46:53 +0200 (Mon, 16 Jun 2014) $
///
///////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "config.h"
#include "lamarquendep.h"
#include "shell.h"
#include "guessmath.h"
#include "guessstring.h"
#include <stdio.h>
#include <string>

// header files for reading binary data archive of global nitrogen deposition
#include "GlobalNitrogenDeposition.h"
#include "GlobalNitrogenDepositionRCP26.h"
#include "GlobalNitrogenDepositionRCP45.h"
#include "GlobalNitrogenDepositionRCP60.h"
#include "GlobalNitrogenDepositionRCP85.h"

namespace Lamarque {

timeseriestype parse_timeseries(const std::string& str) {
	std::string strupper = to_upper(str);

	if (strupper == "HISTORIC") {
		return HISTORIC;
	}
	else if (strupper == "RCP26") {
		return RCP26;
	}
	else if (strupper == "RCP45") {
		return RCP45;
	}
	else if (strupper == "RCP60") {
		return RCP60;
	}
	else if (strupper == "RCP85") {
		return RCP85;
	}
	else if (strupper == "FIXED") {
		return FIXED;
	}
	else {
		fail("Unrecognized timeseries type: %s", str.c_str());
		return FIXED;
	}
}

const double convert = 1e-7;				// converting from gN ha-1 to kgN m-2

NDepData::NDepData() : timeseries(FIXED) {
	set_to_pre_industrial();
}

// Template function for getting the scenario data, 
// using different FastArchive classes depending on RCP
template<typename ArchiveType, typename RecordType>
void get_scenario(const char* file_ndep, const char* scen_suffix,
                  double lon, double lat,
                  double NHxDryDep[NYEAR_SCENNDEP][12],
                  double NHxWetDep[NYEAR_SCENNDEP][12],
                  double NOyDryDep[NYEAR_SCENNDEP][12],
                  double NOyWetDep[NYEAR_SCENNDEP][12]) {

	xtring scenario_filename = file_ndep;

	// insert the scenario suffix (e.g. "RCP26") into the filename 
	long position = scenario_filename.find(".bin");

	if (position != -1) {
		scenario_filename = scenario_filename.left(position) + xtring(scen_suffix) + ".bin";
	}
	else {
		fail("Invalid filename for ndep archive: %s", file_ndep);
	}

	ArchiveType ark;
	if (!ark.open((char*)scenario_filename)) {
		fail("Could not open %s for input", (char*)scenario_filename);
	}

	RecordType rec;
	rec.longitude = lon;
	rec.latitude = lat;

	if (!ark.getindex(rec)) {
		// The coordinate wasn't found in the archive
		ark.close();
		fail("Grid cell not found in %s", (char*)scenario_filename);
	}
	else {
		// Found the record, get the values
		for (int y = 0; y < NYEAR_SCENNDEP; y++) {
			for (int m = 0; m < 12; m++) {
				NHxDryDep[y][m] = rec.NHxDry[y * 12 + m] * convert;
				NHxWetDep[y][m] = rec.NHxWet[y * 12 + m] * convert;
				NOyDryDep[y][m] = rec.NOyDry[y * 12 + m] * convert;
				NOyWetDep[y][m] = rec.NOyWet[y * 12 + m] * convert;
			}
		}
		ark.close();
	}
}

void NDepData::getndep(const char* file_ndep,
                       double lon, double lat,
                       timeseriestype timeseries) {

	this->timeseries = timeseries;

	if (std::string(file_ndep) == "" || timeseries == FIXED) {
		set_to_pre_industrial();
		this->timeseries = FIXED;
	}
	else {
		// read in historic (and possibly scenario)

		GlobalNitrogenDepositionArchive ark;
		if (!ark.open(file_ndep)) {
			fail("Could not open %s for input", (char*)file_ndep);
		}

		GlobalNitrogenDeposition rec;
		rec.longitude = lon;
		rec.latitude = lat;

		if (!ark.getindex(rec)) {
			ark.close();
			fail("Grid cell not found in %s", (char*)file_ndep);
		}

		// Found the record, get the values
		for (int y=0; y<NYEAR_HISTNDEP; y++) {
			for (int m=0; m<12; m++) {
				NHxDryDep[y][m] = rec.NHxDry[y*12+m] * convert;
				NHxWetDep[y][m] = rec.NHxWet[y*12+m] * convert;
				NOyDryDep[y][m] = rec.NOyDry[y*12+m] * convert;
				NOyWetDep[y][m] = rec.NOyWet[y*12+m] * convert;
			}
		}
		ark.close();
		
		// scenario?
		if (timeseries != HISTORIC && timeseries != FIXED) {

			double scen_NHxDryDep[NYEAR_SCENNDEP][12];
			double scen_NHxWetDep[NYEAR_SCENNDEP][12];
			double scen_NOyDryDep[NYEAR_SCENNDEP][12];
			double scen_NOyWetDep[NYEAR_SCENNDEP][12];
			
			switch (timeseries) {
			case RCP26:
				get_scenario<GlobalNitrogenDepositionRCP26Archive,
					GlobalNitrogenDepositionRCP26>(file_ndep, "RCP26",
					                               lon, lat,
					                               scen_NHxDryDep,
					                               scen_NHxWetDep,
					                               scen_NOyDryDep,
					                               scen_NOyWetDep);
				break;
			case RCP45:
				get_scenario<GlobalNitrogenDepositionRCP45Archive,
					GlobalNitrogenDepositionRCP45>(file_ndep, "RCP45",
					                               lon, lat,
					                               scen_NHxDryDep,
					                               scen_NHxWetDep,
					                               scen_NOyDryDep,
					                               scen_NOyWetDep);
				break;
			case RCP60:
				get_scenario<GlobalNitrogenDepositionRCP60Archive,
					GlobalNitrogenDepositionRCP60>(file_ndep, "RCP60",
					                               lon, lat,
					                               scen_NHxDryDep,
					                               scen_NHxWetDep,
					                               scen_NOyDryDep,
					                               scen_NOyWetDep);
				break;
			case RCP85:
				get_scenario<GlobalNitrogenDepositionRCP85Archive,
					GlobalNitrogenDepositionRCP85>(file_ndep, "RCP85",
					                               lon, lat,
					                               scen_NHxDryDep,
					                               scen_NHxWetDep,
					                               scen_NOyDryDep,
					                               scen_NOyWetDep);
				break;
			default:
				// shouldn't happen
				fail("Unexpected timeseriestype!");
			}

			// for the overlapping decade, use mean value of historic and scenario
			for (int m = 0; m < 12; m++) {
				NHxDryDep[NYEAR_HISTNDEP-1][m] = mean(NHxDryDep[NYEAR_HISTNDEP-1][m],
				                                      scen_NHxDryDep[0][m]);
				NHxWetDep[NYEAR_HISTNDEP-1][m] = mean(NHxWetDep[NYEAR_HISTNDEP-1][m],
				                                      scen_NHxWetDep[0][m]);
				NOyDryDep[NYEAR_HISTNDEP-1][m] = mean(NOyDryDep[NYEAR_HISTNDEP-1][m],
				                                      scen_NOyDryDep[0][m]);
				NOyWetDep[NYEAR_HISTNDEP-1][m] = mean(NOyWetDep[NYEAR_HISTNDEP-1][m],
				                                      scen_NOyWetDep[0][m]);
			}

			// for the rest, simply copy from scenario
			for (int y = 1; y < NYEAR_SCENNDEP; y++) {
				for (int m = 0; m < 12; m++) {
					NHxDryDep[NYEAR_HISTNDEP-1+y][m] = scen_NHxDryDep[y][m];
					NHxWetDep[NYEAR_HISTNDEP-1+y][m] = scen_NHxWetDep[y][m];
					NOyDryDep[NYEAR_HISTNDEP-1+y][m] = scen_NOyDryDep[y][m];
					NOyWetDep[NYEAR_HISTNDEP-1+y][m] = scen_NOyWetDep[y][m];
				}
			}
		}
	}
}

void NDepData::get_one_calendar_year(int calendar_year,
									 double mndrydep[12],
									 double mnwetdep[12]) {
	int ndep_year = 0;

	if (timeseries != FIXED && calendar_year >= Lamarque::FIRSTHISTYEARNDEP) {
		ndep_year = (int)((calendar_year - Lamarque::FIRSTHISTYEARNDEP)/10);
	}

	if (timeseries == HISTORIC && ndep_year >= NYEAR_HISTNDEP) {
		fail("Tried to get ndep for year %d (not included in Lamarque historic ndep data set)",
		     calendar_year);
	}
	else if (timeseries != FIXED && ndep_year >= NYEAR_TOTNDEP) {
		fail("Tried to get ndep for year %d (not included in Lamarque historic or scenario ndep data set)",
		     calendar_year);
	}

	for (int m = 0; m < 12; m++) {
		mndrydep[m] = NHxDryDep[ndep_year][m] + NOyDryDep[ndep_year][m];
		
		mnwetdep[m] = NHxWetDep[ndep_year][m] + NOyWetDep[ndep_year][m];
	}	
}

void NDepData::set_to_pre_industrial() {

	// pre-industrial total nitrogen depostion set to 2 kgN/ha/year [kgN m-2]
	double dailyndep = 2000.0 / (4 * 365) * convert;

	for (int y=0; y<NYEAR_TOTNDEP; y++) {
		for (int m=0; m<12; m++) {
			NHxDryDep[y][m] = dailyndep;
			NHxWetDep[y][m] = dailyndep;
			NOyDryDep[y][m] = dailyndep;
			NOyWetDep[y][m] = dailyndep;
		}
	}
}

}
