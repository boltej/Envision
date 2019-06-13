///////////////////////////////////////////////////////////////////////////////////////
/// \file demoinput.cpp
/// \brief LPJ-GUESS input module for a toy data set (for demonstration purposes)
///
///
/// \author Ben Smith
/// $Date: 2015-11-13 16:25:45 +0100 (Fri, 13 Nov 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "demoinput.h"

#include "driver.h"
#include "outputchannel.h"
#include <stdio.h>

REGISTER_INPUT_MODULE("demo", DemoInput)

// Anonymous namespace for variables and functions with file scope
namespace {

// File names for temperature, precipitation, sunshine and soil code driver files
xtring file_temp,file_prec,file_sun,file_soil;

// LPJ soil code
int soilcode;

/// Interpolates monthly data to quasi-daily values.
void interp_climate(double* mtemp, double* mprec, double* msun, double* mdtr,
					double* dtemp, double* dprec, double* dsun, double* ddtr) {
	interp_monthly_means_conserve(mtemp, dtemp);
	interp_monthly_totals_conserve(mprec, dprec, 0);
	interp_monthly_means_conserve(msun, dsun, 0, 100);
	interp_monthly_means_conserve(mdtr, ddtr, 0);
}

} // namespace

DemoInput::DemoInput()
	: nyear(1) {

	// Declare instruction file parameters
	declare_parameter("nyear", &nyear, 1, 10000, "Number of simulation years to run after spinup");
}

bool DemoInput::read_from_file(Coord coord, xtring fname, const char* format,
                               double monthly[12], bool soil /* = false */) {
	double dlon, dlat;
	int elev;
	FILE* in = fopen(fname, "r");
	if (!in) {
		fail("readenv: could not open %s for input", (char*)fname);
	}

	bool foundgrid = false;
	while (!feof(in) && !foundgrid) {
		if (!soil) {
			readfor(in, format, &dlon, &dlat, &elev, monthly);
		} else {
			readfor(in, format, &dlon, &dlat, &soilcode);
		}
		foundgrid = equal(coord.lon, dlon) && equal(coord.lat, dlat);
	}


	fclose(in);
	if (!foundgrid) {
		dprintf("readenv: could not find record for (%g,%g) in %s",
										coord.lon, coord.lat, (char*)fname);
	}
	return foundgrid;
}

bool DemoInput::readenv(Coord coord, long& seed) {

	// Searches for environmental data in driver temperature, precipitation,
	// sunshine and soil code files for the grid cell whose coordinates are given by
	// 'coord'. Data are written to arrays mtemp, mprec, msun and the variable
	// soilcode, which are defined as global variables in this file

	// The temperature, precipitation and sunshine files (Cramer & Leemans,
	// unpublished) should be in ASCII text format and contain one-line records for the
	// climate (mean monthly temperature, mean monthly percentage sunshine or total
	// monthly precipitation) of a particular 0.5 x 0.5 degree grid cell. Elevation is
	// also included in each grid cell record.

	// The following sample record from the temperature file:
	//   " -4400 8300 293-298-311-316-239-105  -7  26  -3 -91-184-239-277"
	// corresponds to the following data:
	//   longitude 44 deg (-=W)
	//   latitude 83 deg (+=N)
	//   elevation 293 m
	//   mean monthly temperatures (deg C) -29.8 (Jan), -31.1 (Feb), ..., -27.7 (Dec)

	// The following sample record from the precipitation file:
	//   " 12750 -200 223 190 165 168 239 415 465 486 339 218 162 149 180"
	// corresponds to the following data:
	//   longitude 127.5 deg (+=E)
	//   latitude 20 deg (-=S)
	//   elevation 223 m
	//   monthly precipitation sum (mm) 190 (Jan), 165 (Feb), ..., 180 (Dec)

	// The following sample record from the sunshine file:
	//   "  2600 7000 293  0 20 38 37 31 28 28 25 21 17  9  7"
	// corresponds to the following data:
	//   longitude 26 deg (+=E)
	//   latitude 70 deg (+=N)
	//   elevation 293 m
	//   monthly mean %age of full sunshine 0 (Jan), 20 (Feb), ..., 7 (Dec)

	// The LPJ soil code file is in ASCII text format and contains one-line records for
	// each grid cell in the form:
	//   <lon> <lat> <soilcode>
	// where <lon>      = longitude as a floating point number (-=W, +=E)
	//       <lat>      = latitude as a floating point number (-=S, +=N)
	//       <soilcode> = integer in the range 0 (no soil) to 9 (see function
	//                    soilparameters in driver module)
	// The fields in each record are separated by spaces

	double mtemp[12];		// monthly mean temperature (deg C)
	double mprec[12];		// monthly precipitation sum (mm)
	double msun[12];		// monthly mean percentage sunshine values

	double mwet[12]={31,28,31,30,31,30,31,31,30,31,30,31}; // number of rain days per month

	double mdtr[12];		// monthly mean diurnal temperature range (oC)
	for(int m=0; m<12; m++) {
		mdtr[m] = 0.;
		if (ifbvoc) {
			dprintf("WARNING: No data available for dtr in sample data set!\nNo daytime temperature correction for BVOC calculations applied.");
		}
	}

	bool gridfound = read_from_file(coord, file_temp, "f6.2,f5.2,i4,12f4.1", mtemp);
	if(gridfound)
		gridfound = read_from_file(coord, file_prec, "f6.2,f5.2,i4,12f4", mprec);
	if(gridfound)
		gridfound = read_from_file(coord, file_sun, "f6.2,f5.2,i4,12f3", msun);
	if(gridfound)
		gridfound = read_from_file(coord, file_soil, "f,f,i", msun, true);	// msun is not used here: just dummy

	if(gridfound) {
		// Interpolate monthly values for environmental drivers to daily values
		// (relevant daily values will be sent to the framework each simulation
		// day in function getclimate, below)
		interp_climate(mtemp, mprec, msun, mdtr, dtemp, dprec, dsun, ddtr);

		// Recalculate precipitation values using weather generator
		// (from Dieter Gerten 021121)
		prdaily(mprec, dprec, mwet, seed);
	}

	return gridfound;
}


void DemoInput::init() {

	// DESCRIPTION
	// Initialises input (e.g. opening files), and reads in the gridlist

	//
	// Reads list of grid cells and (optional) description text from grid list file
	// This file should consist of any number of one-line records in the format:
	//   <longitude> <latitude> [<description>]

	double dlon,dlat;
	bool eof=false;
	xtring descrip;

	// Read list of grid coordinates and store in global Coord object 'gridlist'

	// Retrieve name of grid list file as read from ins file
	xtring file_gridlist=param["file_gridlist"].str;

	FILE* in_grid=fopen(file_gridlist,"r");
	if (!in_grid) fail("initio: could not open %s for input",(char*)file_gridlist);

	gridlist.killall();
	first_call = true;

	while (!eof) {

		// Read next record in file
		eof=!readfor(in_grid,"f,f,a#",&dlon,&dlat,&descrip);

		if (!eof && !(dlon==0.0 && dlat==0.0)) { // ignore blank lines at end (if any)
			Coord& c=gridlist.createobj(); // add new coordinate to grid list

			c.lon=dlon;
			c.lat=dlat;
			c.descrip=descrip;
		}
	}


	fclose(in_grid);

	// Retrieve specified CO2 value as read from ins file
	co2=param["co2"].num;

	// Retrieve specified N value as read from ins file
	ndep=param["ndep"].num;

	// Open landcover files
	landcover_input.init();
	// Open management files
	management_input.init();

	// Retrieve input file names as read from ins file

	file_temp=param["file_temp"].str;
	file_prec=param["file_prec"].str;
	file_sun=param["file_sun"].str;
	file_soil=param["file_soil"].str;

	// Set timers
	tprogress.init();
	tmute.init();

	tprogress.settimer();
	tmute.settimer(MUTESEC);
}

bool DemoInput::getgridcell(Gridcell& gridcell) {

	// See base class for documentation about this function's responsibilities

	// Select coordinates for next grid cell in linked list
	bool gridfound = false;

	bool LUerror = false;

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

		while(!gridfound) {

			// Retrieve coordinate of next grid cell from linked list
			Coord& c = gridlist.getobj();

			// Load environmental data for this grid cell from files
			if(run_landcover) {
				LUerror = landcover_input.loadlandcover(gridlist.getobj().lon, gridlist.getobj().lat);
				if(!LUerror)
					LUerror = management_input.loadmanagement(gridlist.getobj().lon, gridlist.getobj().lat);
			}
			if (!LUerror) {
				gridfound = readenv(c, gridcell.seed);
			} else {
				gridlist.nextobj();
				if(!gridlist.isobj)
					return false;
			}
		}

		dprintf("\nCommencing simulation for stand at (%g,%g)",gridlist.getobj().lon,
			gridlist.getobj().lat);
		if (gridlist.getobj().descrip!="") dprintf(" (%s)\n\n",
			(char*)gridlist.getobj().descrip);
		else dprintf("\n\n");

		// Tell framework the coordinates of this grid cell
		gridcell.set_coordinates(gridlist.getobj().lon, gridlist.getobj().lat);

		// The insolation data will be sent (in function getclimate, below)
		// as percentage sunshine

		gridcell.climate.instype=SUNSHINE;

		// Tell framework the soil type of this grid cell
		soilparameters(gridcell.soiltype,soilcode);

		// For Windows shell - clear graphical output
		// (ignored on other platforms)

		clear_all_graphs();

		return true; // simulate this stand
	}

	return false; // no more stands
}


void DemoInput::getlandcover(Gridcell& gridcell) {

	landcover_input.getlandcover(gridcell);
	landcover_input.get_land_transitions(gridcell);
}

bool DemoInput::getclimate(Gridcell& gridcell) {

	// See base class for documentation about this function's responsibilities

	double progress;

	Climate& climate = gridcell.climate;


	// Send environmental values for today to framework

	climate.dndep  = ndep / (365.0 * 10000.0);

	climate.co2 = co2;

	climate.temp  = dtemp[date.day];
	climate.prec  = dprec[date.day];
	climate.insol = dsun[date.day];

	// bvoc

	climate.dtr=ddtr[date.day];


	// First day of year only ...

	if (date.day == 0) {

		// Return false if last year was the last for the simulation
		if (date.year==nyear_spinup+nyear) return false;

		// Progress report to user and update timer

		if (tmute.getprogress()>=1.0) {
			progress=(double)(gridlist.getobj().id*(nyear_spinup+nyear)
				+date.year)/(double)(gridlist.nobj*(nyear_spinup+nyear));


			tprogress.setprogress(progress);
			dprintf("%3d%% complete, %s elapsed, %s remaining\n",(int)(progress*100.0),
				tprogress.elapsed.str,tprogress.remaining.str);
			tmute.settimer(MUTESEC);
		}
	}

	return true;
}

DemoInput::~DemoInput() {

	// Performs memory deallocation, closing of files or other "cleanup" functions.

}
