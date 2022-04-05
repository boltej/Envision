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

	double dlon, dlat, dnorth, deast;
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
		eof = !readfor(in_grid, "f,f,f,f,a#", &dlon, &dlat,&dnorth,&deast, &descrip);

		if (!eof && !(dlon == 0.0 && dlat == 0.0)) { // ignore blank lines at end (if any)
			Coord& c = gridlist.createobj(); // add new coordinate to grid list

			c.lon = dlon;
			c.lat = dlat;
			c.north=dnorth;
			c.east=deast;
			c.descrip = descrip;
		}
	}


	fclose(in_grid);

	// Read CO2 data from file
    xtring file_co2 = param["file_co2"].str;
	if (PathManager::FindPath(file_co2, path) < 0)
	{
		CString msg;
		msg.Format(_T("LPJ: Specified source table '%s'' can not be found.  This table will be ignored"), file_co2);
		Report::LogError(msg);
	}
	co2.load_file(path);
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

	if (save_state)
	{
		CString msg;
		msg.Format("Simulation is set with spinup time of %i years.  It will generate the %i years of climate from the CRU data and will save the system state for later.",nyear_spinup,nyear_spinup);
		Report::Log(msg);

		msg.Format("After completion, rerun Envision without restart=1 to start from saved state and run forward using Envision climate data.");
		Report::Log(msg);

	}
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
	for (int m = 0; m < 12; m++) {
		mdtr[m] = 0.;
		if (ifbvoc) {
			dprintf("WARNING: No data available for dtr in sample data set!\nNo daytime temperature correction for BVOC calculations applied.");
		}
	}
	for (j = 0; j < NYEAR_HIST; j++) {
		for (i = 0; i < 12; i++) {
			hist_mwet[j][i] = mwet[i];
			hist_mdtr[j][i] = mdtr[i];
		}
	}
	// end of test

	// temp
	CString path;
	if (PathManager::FindPath(file_temp, path) < 0)
	{
		CString msg;
		msg.Format(_T("LPJ: Specified source table '%s'' can not be found.  This table will be ignored"), file_temp);
		Report::LogError(msg);
	}
	in = fopen(path, "r");

	if (!in) fail("readenv: could not open %s for input", (char*)file_temp);
	found = false;
	for (j = 0; j < NYEAR_HIST;) {
		readfor(in, "f6.2,f5.2,f5.0,12f4.1", &dlon, &dlat, &elev, mtemp);
		//dprintf("Temperature %g\n", mtemp[0]);

		if (equal(longitude, dlon) && equal(latitude, dlat)) {
			for (i = 0; i < 12; i++) {
				hist_mtemp[j][i] = mtemp[i];
				//dprintf("Find Tmp for (%g,%g,%g,%g)\n",
				//	dlon,dlat,elev,hist_mtemp[j][i]);
			}
			j++;
			found = true;
		}

		if (feof(in) && !found) {
			dprintf("readenv: could not find record for (%g,%g) in %s",
				longitude, latitude, (char*)file_temp);
			fclose(in);
			return false;
		}
	}
	elevation = elev;
	fclose(in);

	// dtr
	
	if (PathManager::FindPath(file_dtr, path) < 0)
	{
		CString msg;
		msg.Format(_T("LPJ: Specified source table '%s'' can not be found.  This table will be ignored"), file_dtr);
		Report::LogError(msg);
	}
	in = fopen(path, "r");

	if (!in) fail("readenv: could not open %s for input", (char*)file_dtr);
	found = false;
	for (j = 0; j < NYEAR_HIST;) {
		readfor(in, "f6.2,f5.2,f5.0,12f4.1", &dlon, &dlat, &elev, mdtr);
		//	dprintf("(%g,%g,%g,%g)\n", dlon, dlat, elev, mdtr[0]);

		if (equal(longitude, dlon) && equal(latitude, dlat)) {
			for (i = 0; i < 12; i++) {
				hist_mdtr[j][i] = mdtr[i];
				//	dprintf(" find dtr for (%g,%g,%g,%g)\n",
					//	dlon,dlat,elev,hist_mdtr[j][i]);
			}
			j++;
			found = true;
		}

		if (feof(in) && !found) {
			//dprintf("readenv: could not find record for (%g,%g) in %s",
			//	longitude, latitude, (char*)file_dtr);
			fclose(in);
			return false;
		}
	}
	fclose(in);

	// prec
	
	if (PathManager::FindPath(file_prec, path) < 0)
	{
		CString msg;
		msg.Format(_T("LPJ: Specified source table '%s'' can not be found.  This table will be ignored"), file_prec);
		Report::LogError(msg);
	}
	in = fopen(path, "r");

	if (!in) fail("readenv: could not open %s for input", (char*)file_prec);
	found = false;
	for (j = 0; j < NYEAR_HIST;) {
		readfor(in, "f6.2,f5.2,f5.0,12f4", &dlon, &dlat, &elev, mprec);

		if (equal(longitude, dlon) && equal(latitude, dlat)) {
			for (i = 0; i < 12; i++) {
				hist_mprec[j][i] = mprec[i];
				//dprintf("find prec for (%g,%g,%g)\n",
				//	dlon, dlat, hist_mprec[j][i]);
			}
			j++;
			found = true;
		}

		if (feof(in) && !found) {
			dprintf("readenv: could not find record for (%g,%g) in %s",
				longitude, latitude, (char*)file_prec);
			fclose(in);
			return false;
		}
	}
	fclose(in);

	// sun
	
	if (PathManager::FindPath(file_sun, path) < 0)
	{
		CString msg;
		msg.Format(_T("LPJ: Specified source table '%s'' can not be found.  This table will be ignored"), file_sun);
		Report::LogError(msg);
	}
	in = fopen(path, "r");

	if (!in) fail("readenv: could not open %s for input", (char*)file_sun);
	found = false;
	for (j = 0; j < NYEAR_HIST;) {
		readfor(in, "f6.2,f5.2,f5.0,12f4", &dlon, &dlat, &elev, msun); //I think it can turn to another line
		//dprintf("sun %g\n", msun[0]);
		if (equal(longitude, dlon) && equal(latitude, dlat)) {
			for (i = 0; i < 12; i++) {
				hist_msun[j][i] = msun[i];
				//dprintf("Find sun for (%g,%g,%g)\n",
				//	dlon, dlat, hist_msun[j][i]);
			}
			j++;
			found = true;
		}

		if (feof(in) && !found) {
			dprintf("readenv: could not find record for (%g,%g) in %s",
				longitude, latitude, (char*)file_sun);
			fclose(in);
			return false;
		}
	}
	fclose(in);

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
		if (save_state)
		{
		while (!gridfound) {

			if (gridlist.isobj) {

				lon = gridlist.getobj().lon;
				lat = gridlist.getobj().lat;
				
				   gridfound = searchmydata(lon, lat);

				if (run_landcover && gridfound) {
					LUerror = landcover_input.loadlandcover(lon, lat);
					if (!LUerror)
						LUerror = management_input.loadmanagement(lon, lat);
				}

				//if (!gridfound || LUerror) {
				//	if (!gridfound)
				//		dprintf("\nError: could not find stand at (%g,%g) in climate data files\n", gridlist.getobj().lon, gridlist.getobj().lat);
				//	else if (LUerror)
				//		dprintf("\nError: could not find stand at (%g,%g) in landcover/management data file(s)\n", gridlist.getobj().lon, gridlist.getobj().lat);
				//	gridfound = false;
				//	gridlist.nextobj();
				//}
			}
			else return false;
		}

		// Give sub-classes a chance to modify the data
		adjust_raw_forcing_data(gridlist.getobj().lon,
			gridlist.getobj().lat,
			hist_mtemp, hist_mprec, hist_msun);

		// Build spinup data sets
		spinup_mtemp.get_data_from(hist_mtemp);
		spinup_mprec.get_data_from(hist_mprec);
		spinup_msun.get_data_from(hist_msun);

		// Detrend spinup temperature data
		spinup_mtemp.detrend_data();

		// guess2008 - new spinup data sets
		spinup_mfrs.get_data_from(hist_mfrs);
		spinup_mwet.get_data_from(hist_mwet);
		spinup_mdtr.get_data_from(hist_mdtr);
		}
		// We wont detrend dtr for now. Partly because dtr is at the moment only
		// used for BVOC, so what happens during the spinup is not affecting
		// results in the period thereafter, and partly because the detrending
		// can give negative dtr values.
		//spinup_mdtr.detrend_data();


	//	if (gridlist.getobj().descrip != "") dprintf(" (%s)\n\n",
	//		(char*)gridlist.getobj().descrip);
	//	else dprintf("\n\n");

		// Tell framework the coordinates of this grid cell
		gridcell.set_coordinates(gridlist.getobj().lon, gridlist.getobj().lat);

		gridcell.set_utm_coordinates(gridlist.getobj().north, gridlist.getobj().east);

		// Get nitrogen deposition data. 
		/* Since the historic data set does not reach decade 2010-2019,
		 * we need to use the RCP data for the last decade. */
		ndep.getndep(param["file_ndep"].str, gridlist.getobj().lon, gridlist.getobj().lat, Lamarque::RCP60);

		// The insolation data will be sent (in function getclimate, below)
		// as incoming shortwave radiation, averages are over 24 hours

		gridcell.climate.instype = SWRAD_TS;

		// Tell framework the soil type of this grid cell
//		soilparameters(gridcell.soiltype, soilcode);

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
	if (save_state)
		{
		if (date.day == 0) {

			// First day of year ...

			// Extract N deposition to use for this year,
			// monthly means to be distributed into daily values further down
			double mndrydep[12], mnwetdep[12];
			ndep.get_one_calendar_year(date.year - nyear_spinup + FIRSTHISTYEAR,
				mndrydep, mnwetdep);

			if (date.year < nyear_spinup) {

				// During spinup period

				if (date.year == state_year && restart) {

					int year_offset = state_year % NYEAR_SPINUP_DATA;

					for (int y = 0; y < year_offset; y++) {
						spinup_mtemp.nextyear();
						spinup_mprec.nextyear();
						spinup_msun.nextyear();
						spinup_mfrs.nextyear();
						spinup_mwet.nextyear();
						spinup_mdtr.nextyear();
					}
				}

				int m;
				double mtemp[12], mprec[12], msun[12];
				double mfrs[12], mwet[12], mdtr[12];

				for (m = 0; m < 12; m++) {
					mtemp[m] = spinup_mtemp[m];
					mprec[m] = spinup_mprec[m];
					msun[m] = spinup_msun[m];

					mfrs[m] = spinup_mfrs[m];
					mwet[m] = spinup_mwet[m];
					mdtr[m] = spinup_mdtr[m];
				}

				// Interpolate monthly spinup data to quasi-daily values
				interp_climate(mtemp, mprec, msun, mdtr, dtemp, dprec, dsun, ddtr);

				// Only recalculate precipitation values using weather generator
				// if rainonwetdaysonly is true. Otherwise we assume that it rains a little every day.
				if (ifrainonwetdaysonly) {
					// (from Dieter Gerten 021121)
					prdaily(mprec, dprec, mwet, gridcell.seed);
				}

				spinup_mtemp.nextyear();
				spinup_mprec.nextyear();
				spinup_msun.nextyear();

				spinup_mfrs.nextyear();
				spinup_mwet.nextyear();
				spinup_mdtr.nextyear();

			}
			else if (date.year < nyear_spinup + NYEAR_HIST) {

				// Historical period

				// Interpolate this year's monthly data to quasi-daily values
				interp_climate(hist_mtemp[date.year - nyear_spinup],
					hist_mprec[date.year - nyear_spinup], hist_msun[date.year - nyear_spinup],
					hist_mdtr[date.year - nyear_spinup],
					dtemp, dprec, dsun, ddtr);

				// Only recalculate precipitation values using weather generator
				// if ifrainonwetdaysonly is true. Otherwise we assume that it rains a little every day.
				if (ifrainonwetdaysonly) {
					// (from Dieter Gerten 021121)
					prdaily(hist_mprec[date.year - nyear_spinup], dprec, hist_mwet[date.year - nyear_spinup], gridcell.seed);
				}
			}
			else {
				// Return false if last year was the last for the simulation
				return false;
			}

			// Distribute N deposition
			distribute_ndep(mndrydep, mnwetdep, dprec, dndep);
		}



		if (FIRSTHISTYEAR + date.year <= pFlowContext->pEnvContext->endYear)
			climate.co2 = co2[FIRSTHISTYEAR + date.year - nyear_spinup];
		else
			climate.co2 = 414.01;

			climate.temp  = dtemp[date.day];
			climate.prec  = dprec[date.day];
			climate.insol = dsun[date.day];

			// First day of year only ...

			if (date.day == 0) {

				// Progress report to user and update timer

				if (tmute.getprogress() >= 1.0) 
					{

						float progress = (double)(gridlist.getobj().id * (nyear_spinup + NYEAR_HIST)
							+ date.year) / (double)(gridlist.nobj * (nyear_spinup + NYEAR_HIST));

						tprogress.setprogress(progress);
						CString msg;
						msg.Format(_T(" %3d%% complete, %s elapsed, %s remaining\n", (int)(progress * 100.0),tprogress.elapsed.str, tprogress.remaining.str));
						Report::Log(msg);
						//dprintf("%3d%% complete, %s elapsed, %s remaining\n", (int)(progress * 100.0),
						  // tprogress.elapsed.str, tprogress.remaining.str);
						tmute.settimer(MUTESEC);
					}

			}

		}
	else
		{

		float prec = 0.0f; float temp = 0.0f; float insol = 0.0f;float tmax=0;float tmin=0;

		HRU *pHRU = gridcell.pHRU;

		pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, pFlowContext->dayOfYear, prec);//mm
		pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, pFlowContext->dayOfYear, tmax);//C
		pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, pFlowContext->dayOfYear, tmin);//C
		pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, pFlowContext->dayOfYear, insol);//Incoming Short W/m2
		bool t = isnan(tmax);
		if (t)
			int m = 1;
		climate.temp = (tmax+tmin)/2;
		climate.prec = prec;
		climate.insol = insol;
		if (FIRSTHISTYEAR + date.year <= pFlowContext->pEnvContext->endYear)
			climate.co2 = co2[FIRSTHISTYEAR + date.year - nyear_spinup];
		else
			climate.co2 = 414.01;

		}
		//climate.temp = dtemp[date.day];
		//climate.prec = dprec[date.day];
		//climate.insol = dsun[date.day];
	// Nitrogen deposition
	climate.dndep = dndep[date.day];

	// bvoc
	if (ifbvoc) {
		climate.dtr = ddtr[date.day];
	}



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