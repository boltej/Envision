///////////////////////////////////////////////////////////////////////////////////////
/// \file cruinput.cpp
/// \brief LPJ-GUESS input module for CRU-NCEP data set
///
/// This input module reads in CRU-NCEP climate data in a customised binary format.
/// The binary files contain CRU-NCEP half-degree global historical climate data
/// for 1901-2015.
///
/// \author Ben Smith
/// $Date: 2017-04-05 15:04:09 +0200 (Wed, 05 Apr 2017) $
///
///////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "config.h"
#include "envision_input.h"

#include "driver.h"
#include "parameters.h"
#include <stdio.h>
#include <utility>
#include <vector>
#include <algorithm>
#include <PathManager.h>

#include <..\Plugins\Flow\Flow.h>
REGISTER_INPUT_MODULE("envision", ENVInput)

// Anonymous namespace for variables and functions with file scope
namespace {

	xtring file_cru;
	xtring file_cru_misc;

	// weichao revision
	// File names for temperature, precipitation, sunshine and soil code driver files
	xtring file_temp, file_prec, file_sun, file_dtr, file_soil;
	// LPJ soil code
	int soilcode;
	//end of weichao revision

	/// Interpolates monthly data to quasi-daily values.
	void interp_climate(double* mtemp, double* mprec, double* msun, double* mdtr,
		double* dtemp, double* dprec, double* dsun, double* ddtr) {
		interp_monthly_means_conserve(mtemp, dtemp);
		interp_monthly_totals_conserve(mprec, dprec, 0);
		interp_monthly_means_conserve(msun, dsun, 0);
		interp_monthly_means_conserve(mdtr, ddtr, 0);
	}

} // namespace


ENVInput::ENVInput()
	: searchradius(0),
	spinup_mtemp(NYEAR_SPINUP_DATA),
	spinup_mprec(NYEAR_SPINUP_DATA),
	spinup_msun(NYEAR_SPINUP_DATA),
	spinup_mfrs(NYEAR_SPINUP_DATA),
	spinup_mwet(NYEAR_SPINUP_DATA),
	spinup_mdtr(NYEAR_SPINUP_DATA) {

	// Declare instruction file parameters

	declare_parameter("searchradius", &searchradius, 0, 100,
		"If specified, CRU data will be searched for in a circle");
}


void ENVInput::init() {

	// DESCRIPTION
	// Initialises input (e.g. opening files), and reads in the gridlist

	//
	// Reads list of grid cells and (optional) description text from grid list file
	// This file should consist of any number of one-line records in the format:
	//   <longitude> <latitude> [<description>]

	double dlon, dlat;
	bool eof = false;
	xtring descrip;

	// Read list of grid coordinates and store in global Coord object 'gridlist'

	// Retrieve name of grid list file as read from ins file
	xtring file_gridlist = param["file_gridlist"].str;


	CString path;
	//tmpPath = PathManager::GetPath(PM_IDU_DIR);//directory with the idu
	if (PathManager::FindPath(file_gridlist, path) < 0)
	{
		CString msg;
		msg.Format(_T("LPJ: Specified source table '%s'' can not be found.  This table will be ignored"), file_gridlist);
		Report::LogError(msg);
	}

	FILE* in_grid = fopen(path, "r");
	if (!in_grid) fail("initio: could not open %s for input", (char*)file_gridlist);

	//file_cru=param["file_cru"].str;
	//file_cru_misc=param["file_cru_misc"].str;

	// weichao revision
	file_temp = param["file_temp"].str;
	file_dtr = param["file_dtr"].str;
	file_prec = param["file_prec"].str;
	file_sun = param["file_sun"].str;
	//file_soil = param["file_soil"].str;
	// end

	gridlist.killall();
	first_call = true;

	while (!eof) {

		// Read next record in file
		eof = !readfor(in_grid, "f,f,a#", &dlon, &dlat, &descrip);

		if (!eof && !(dlon == 0.0 && dlat == 0.0)) { // ignore blank lines at end (if any)
			Coord& c = gridlist.createobj(); // add new coordinate to grid list

			c.lon = dlon;
			c.lat = dlat;
			c.descrip = descrip;
		}
	}


	fclose(in_grid);

	// Read CO2 data from file
	co2.load_file(param["file_co2"].str);
	// Open landcover files
	landcover_input.init();
	// Open management files
	management_input.init();

	date.set_first_calendar_year(FIRSTHISTYEAR - nyear_spinup);
	// Set timers
	tprogress.init();
	tmute.init();

	tprogress.settimer();
	tmute.settimer(MUTESEC);
}

//


bool ENVInput::searchmydata(double longitude, double latitude) {

	// search data in file for the location input
	FILE* in;
	int i, j;
	double elev;
	double dlon, dlat;
	double mtemp[12];		// monthly mean temperature (deg C)
	double mprec[12];		// monthly precipitation sum (mm)
	double msun[12];		// monthly mean percentage sunshine values
	bool found = true;

	// weichao test 
	double mwet[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 }; // number of rain days per month

	double mdtr[12];		// monthly mean diurnal temperature range (oC)
	//for (int m = 0; m < 12; m++) {
		//mdtr[m] = 0.;
		//if (ifbvoc) {
			//dprintf("WARNING: No data available for dtr in sample data set!\nNo daytime temperature correction for BVOC calculations applied.");
		//}
	//}
	for (j = 0; j < NYEAR_HIST; j++) {
		for (i = 0; i < 12; i++) {
			hist_mwet[j][i] = mwet[i];
			//hist_mdtr[j][i] = mdtr[i];
		}
	}
	// end of test



	// soiltype


	return found;
}


//



void ENVInput::get_monthly_ndep(int calendar_year,
	double* mndrydep,
	double* mnwetdep) {

	ndep.get_one_calendar_year(calendar_year,
		mndrydep, mnwetdep);
}


void ENVInput::adjust_raw_forcing_data(double lon,
	double lat,
	double hist_mtemp[NYEAR_HIST][12],
	double hist_mprec[NYEAR_HIST][12],
	double hist_msun[NYEAR_HIST][12]) {

	// The default (base class) implementation does nothing here.
}




bool ENVInput::getgridcell(Gridcell& gridcell) {

	// See base class for documentation about this function's responsibilities

	int soilcode;

	//weichao
	soilcode = 2;

	//

	// Make sure we use the first gridcell in the first call to this function,
	// and then step through the gridlist in subsequent calls.
	if (first_call) {
		gridlist.firstobj();

		// Note that first_call is static, so this assignment is remembered
		// across function calls.
		first_call = false;
	}
	else gridlist.nextobj();

	if (gridlist.isobj) {

		bool gridfound = false;
		bool LUerror = false;
		double lon;
		double lat;

		while (!gridfound) {

			if (gridlist.isobj) {

				lon = gridlist.getobj().lon;
				lat = gridlist.getobj().lat;
				gridfound = searchmydata(lon, lat);
				//gridfound = CRU_TS30::findnearestCRUdata(searchradius, file_cru, lon, lat, soilcode,
					//hist_mtemp, hist_mprec, hist_msun);

				//if (gridfound) // Get more historical CRU data for this grid cell
					//gridfound = CRU_TS30::searchcru_misc(file_cru_misc, lon, lat, elevation,
						//hist_mfrs, hist_mwet, hist_mdtr);

				if (run_landcover && gridfound) {
					LUerror = landcover_input.loadlandcover(lon, lat);
					if (!LUerror)
						LUerror = management_input.loadmanagement(lon, lat);
				}

				if (!gridfound || LUerror) {
					if (!gridfound)
						dprintf("\nError: could not find stand at (%g,%g) in climate data files\n", gridlist.getobj().lon, gridlist.getobj().lat);
					else if (LUerror)
						dprintf("\nError: could not find stand at (%g,%g) in landcover/management data file(s)\n", gridlist.getobj().lon, gridlist.getobj().lat);
					gridfound = false;
					gridlist.nextobj();
				}
			}
			else return false;
		}

		// Give sub-classes a chance to modify the data
	//	adjust_raw_forcing_data(gridlist.getobj().lon,
	//		gridlist.getobj().lat,
//			hist_mtemp, hist_mprec, hist_msun);

		// Build spinup data sets
	//	spinup_mtemp.get_data_from(hist_mtemp);
	//	spinup_mprec.get_data_from(hist_mprec);
	//	spinup_msun.get_data_from(hist_msun);

		// Detrend spinup temperature data
	//	spinup_mtemp.detrend_data();

		// guess2008 - new spinup data sets
	//	spinup_mfrs.get_data_from(hist_mfrs);
	//	spinup_mwet.get_data_from(hist_mwet);
	//	spinup_mdtr.get_data_from(hist_mdtr);

		// We wont detrend dtr for now. Partly because dtr is at the moment only
		// used for BVOC, so what happens during the spinup is not affecting
		// results in the period thereafter, and partly because the detrending
		// can give negative dtr values.
		//spinup_mdtr.detrend_data();


	//	dprintf("\nCommencing simulation for stand at (%g,%g)", gridlist.getobj().lon,
	//		gridlist.getobj().lat);
	//	if (gridlist.getobj().descrip != "") dprintf(" (%s)\n\n",
	//		(char*)gridlist.getobj().descrip);
	//	else dprintf("\n\n");

		// Tell framework the coordinates of this grid cell
		gridcell.set_coordinates(gridlist.getobj().lon, gridlist.getobj().lat);

		// Get nitrogen deposition data. 
		/* Since the historic data set does not reach decade 2010-2019,
		 * we need to use the RCP data for the last decade. */
		ndep.getndep(param["file_ndep"].str, lon, lat, Lamarque::RCP60);

		// The insolation data will be sent (in function getclimate, below)
		// as incoming shortwave radiation, averages are over 24 hours

		gridcell.climate.instype = SWRAD_TS;

		// Tell framework the soil type of this grid cell
		soilparameters(gridcell.soiltype, soilcode);

		// For Windows shell - clear graphical output
		// (ignored on other platforms)

		clear_all_graphs();

		return true; // simulate this stand
	}

	return false; // no more stands
}


void ENVInput::getlandcover(Gridcell& gridcell) {

	landcover_input.getlandcover(gridcell);
	landcover_input.get_land_transitions(gridcell);
}


bool ENVInput::getclimate(Gridcell& gridcell, FlowContext *pFlowContext) {

	// See base class for documentation about this function's responsibilities

	double progress;

	Climate& climate = gridcell.climate;



	climate.co2 = co2[FIRSTHISTYEAR + date.year - nyear_spinup];

	//	climate.temp  = dtemp[date.day];
	//	climate.prec  = dprec[date.day];
	//	climate.insol = dsun[date.day];


	float prec = 0.0f; float temp = 0.0f; float insol = 0.0f;
	//HRU *pHRU = pFlowContext->pFlowModel->GetHRU(0);
	HRU *pHRU = gridcell.pHRU;
	//HRU* pHRU = gridcell.m_hruArray[0];
	pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, pFlowContext->dayOfYear, prec);//mm
	pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, pFlowContext->dayOfYear, temp);//C
	pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, pFlowContext->dayOfYear, insol);//C

	climate.temp = temp;
	climate.prec = prec;
	climate.insol = insol;


	// Nitrogen deposition
	climate.dndep = dndep[date.day];

	// bvoc
	if (ifbvoc) {
		climate.dtr = ddtr[date.day];
	}

	// First day of year only ...

/*	if (date.day == 0) {

		// Progress report to user and update timer

		if (tmute.getprogress() >= 1.0) {
			progress = (double)(gridlist.getobj().id*(nyear_spinup + NYEAR_HIST)
				+ date.year) / (double)(gridlist.nobj*(nyear_spinup + NYEAR_HIST));
			tprogress.setprogress(progress);
			dprintf("%3d%% complete, %s elapsed, %s remaining\n", (int)(progress*100.0),
				tprogress.elapsed.str, tprogress.remaining.str);
			tmute.settimer(MUTESEC);
		}
		
	}*/

	return true;
}


ENVInput::~ENVInput() {

	// Performs memory deallocation, closing of files or other "cleanup" functions.

}


///////////////////////////////////////////////////////////////////////////////////////
// REFERENCES
// Lamarque, J.-F., Kyle, G. P., Meinshausen, M., Riahi, K., Smith, S. J., Van Vuuren,
//   D. P., Conley, A. J. & Vitt, F. 2011. Global and regional evolution of short-lived
//   radiatively-active gases and aerosols in the Representative Concentration Pathways.
//   Climatic Change, 109, 191-212.
// Nakai, T., Sumida, A., Kodama, Y., Hara, T., Ohta, T. (2010). A comparison between
//   various definitions of forest stand height and aerodynamic canopy height.
//   Agricultural and Forest Meteorology, 150(9), 1225-1233