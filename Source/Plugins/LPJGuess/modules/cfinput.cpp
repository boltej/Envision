///////////////////////////////////////////////////////////////////////////////////////
/// \file cfinput.cpp
/// \brief Input module for CF conforming NetCDF files
///
/// \author Joe Siltberg
/// $Date: 2015-12-14 16:08:55 +0100 (Mon, 14 Dec 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "cfinput.h"

#ifdef HAVE_NETCDF

#include "guess.h"
#include "driver.h"
#include "guessstring.h"
#include <fstream>
#include <sstream>
#include <algorithm>

REGISTER_INPUT_MODULE("cf", CFInput)

using namespace GuessNC::CF;

namespace {

const int SECONDS_PER_DAY = 24*60*60;

// Converts a CF standard name to one of our insolation types
// Calls fail() if the standard name is invalid
insoltype cf_standard_name_to_insoltype(const std::string& standard_name) {
	if (standard_name == "surface_downwelling_shortwave_flux_in_air" ||
	    standard_name == "surface_downwelling_shortwave_flux") {
		return SWRAD_TS;
	}
	else if (standard_name == "surface_net_downward_shortwave_flux") {
		return NETSWRAD_TS;
	}
	else if (standard_name == "cloud_area_fraction") {
		return SUNSHINE;
	}
	else {
		fail("Unknown insolation type: %s", standard_name.c_str());
		return SUNSHINE; // To avoid compiler warning
	}
}

// Gives the maximum allowed value for insolation, given an insolation type
// Used as an upper limit when interpolating from monthly to daily values
double max_insolation(insoltype instype) {
	if (instype == SUNSHINE) {
		return 100;
	}
	else {
		return std::numeric_limits<double>::max();
	}
}

// Checks if a DateTime is at the first day of the year
bool first_day_of_year(GuessNC::CF::DateTime dt) {
	return dt.get_month() == 1 && dt.get_day() == 1;
}

// Checks if a DateTime is in January
bool first_month_of_year(GuessNC::CF::DateTime dt) {
	return dt.get_month() == 1;
}

// Compares a Date with a GuessNC::CF::DateTime to see if the Date is on an earlier day
bool earlier_day(const Date& date, int calendar_year,
                 const GuessNC::CF::DateTime& date_time) {
	std::vector<int> d1(3),d2(3);

	d1[0] = calendar_year;
	d2[0] = date_time.get_year();

	d1[1] = date.month+1;
	d2[1] = date_time.get_month();

	d1[2] = date.dayofmonth+1;
	d2[2] = date_time.get_day();

	return d1 < d2;
}

// Compares a Date with a GuessNC::CF::DateTime to see if the Date is on a later day
// The date object must know about its calendar years (i.e. set_first_calendar_year must
// have been called)
bool later_day(const Date& date,
               const GuessNC::CF::DateTime& date_time) {
	std::vector<int> d1(3),d2(3);

	d1[0] = date.get_calendar_year();
	d2[0] = date_time.get_year();

	d1[1] = date.month+1;
	d2[1] = date_time.get_month();

	d1[2] = date.dayofmonth+1;
	d2[2] = date_time.get_day();

	return d1 > d2;
}

// Checks if the variable contains daily data
bool is_daily(const GuessNC::CF::GridcellOrderedVariable* cf_var) {

	// Check if first and second timestep is one day apart

	DateTime dt1 = cf_var->get_date_time(0);
	DateTime dt2 = cf_var->get_date_time(1);

	dt1.add_time(1, GuessNC::CF::DAYS, cf_var->get_calendar_type());

	return dt1 == dt2;
}

// Returns a DateTime in the last day for which the variable has data.
// For daily data, this is simply the day of the last timestep, for monthly data
// we need to find the last day of the last timestep's month.
GuessNC::CF::DateTime last_day_to_simulate(const GuessNC::CF::GridcellOrderedVariable* cf_var) {
	GuessNC::CF::DateTime last = cf_var->get_date_time(cf_var->get_timesteps()-1);
	if (is_daily(cf_var)) {
		return last;
	}
	else {
		// Not daily, assume monthly.
		GuessNC::CF::DateTime prev = last;
		GuessNC::CF::DateTime next = last;

		do {
			prev = next;
			next.add_time(1, GuessNC::CF::DAYS, cf_var->get_calendar_type());
		} while (next.get_month() == last.get_month());

		return prev;
	}
}

// Verifies that a CF variable with air temperature data contains what we expect
void check_temp_variable(const GuessNC::CF::GridcellOrderedVariable* cf_var) {
	if (cf_var->get_standard_name() != "air_temperature") {
		fail("Temperature variable doesn't seem to contain air temperature data");
	}
	if (cf_var->get_units() != "K") {
		fail("Temperature variable doesn't seem to be in Kelvin");
	}
}

// Verifies that a CF variable with precipitation data contains what we expect
void check_prec_variable(const GuessNC::CF::GridcellOrderedVariable* cf_var) {
	if (cf_var->get_standard_name() == "precipitation_flux") {
		if (cf_var->get_units() != "kg m-2 s-1") {
			fail("Precipitation is given as flux but does not have the correct unit (kg m-2 s-1)");
		}
	}
	else if (cf_var->get_standard_name() == "precipitation_amount") {
		if (cf_var->get_units() != "kg m-2") {
			fail("Precipitation is given as amount but does not have the correct unit (kg m-2)");
		}
	}
	else {
		fail("Unrecognized precipitation type");
	}
}

// Verifies that a CF variable with insolation data contains what we expect
void check_insol_variable(const GuessNC::CF::GridcellOrderedVariable* cf_var) {
	if (cf_var->get_standard_name() != "surface_downwelling_shortwave_flux_in_air" &&
	    cf_var->get_standard_name() != "surface_downwelling_shortwave_flux" &&
	    cf_var->get_standard_name() != "surface_net_downward_shortwave_flux" &&
	    cf_var->get_standard_name() != "cloud_area_fraction") {
		fail("Insolation variable doesn't seem to contain insolation data");
	}

	if (cf_var->get_standard_name() == "cloud_area_fraction") {
		if (cf_var->get_units() != "1") {
			fail("Unrecognized unit for cloud cover");
		}
	}
	else {
		if (cf_var->get_units() != "W m-2") {
			fail("Insolation variable given as radiation but unit doesn't seem to be in W m-2");
		}
	}
}

// Verifies that a CF variable with wetdays data contains what we expect
void check_wetdays_variable(const GuessNC::CF::GridcellOrderedVariable* cf_var) {
	const char* wetdays_standard_name =
		"number_of_days_with_lwe_thickness_of_precipitation_amount_above_threshold";

	if (cf_var && cf_var->get_standard_name() != wetdays_standard_name) {
		fail("Wetdays variable should have standard name %s", wetdays_standard_name);
	}
}

// Checks if two variables contain data for the same time period
//
// Compares start and end of time series, the day numbers are only compared if
// both variables are daily.
void check_compatible_timeseries(const GuessNC::CF::GridcellOrderedVariable* var1,
                                 const GuessNC::CF::GridcellOrderedVariable* var2) {
	GuessNC::CF::DateTime start1, start2, end1, end2;

	const std::string error_message = format_string("%s and %s have incompatible timeseries",
		var1->get_variable_name().c_str(), var2->get_variable_name().c_str());

	start1 = var1->get_date_time(0);
	start2 = var2->get_date_time(0);

	end1 = var1->get_date_time(var1->get_timesteps() - 1);
	end2 = var2->get_date_time(var2->get_timesteps() - 1);

	if (start1.get_year() != start2.get_year() ||
		start1.get_month() != start2.get_month()) {
		fail(error_message.c_str());
	}

	if (end1.get_year() != end2.get_year() ||
		end1.get_month() != end2.get_month()) {
		fail(error_message.c_str());
	}

	if (is_daily(var1) && is_daily(var2)) {
		if (start1.get_day() != start2.get_day() ||
			end1.get_day() != end2.get_day()) {
			fail(error_message.c_str());
		}
	}
}

// Makes sure all variables have compatible time series
void check_compatible_timeseries(const std::vector<GuessNC::CF::GridcellOrderedVariable*> variables) {

	for (size_t i = 0; i < variables.size(); ++i) {
		for (size_t j = i + 1; j < variables.size(); ++j) {
			check_compatible_timeseries(variables[i], variables[j]);
		}
	}
}

void check_same_spatial_domains(const std::vector<GuessNC::CF::GridcellOrderedVariable*> variables) {

	for (size_t i = 1; i < variables.size(); ++i) {
		if (!variables[0]->same_spatial_domain(*variables[i])) {
			fail("%s and %s don't have the same spatial domain",
				variables[0]->get_variable_name().c_str(),
				variables[i]->get_variable_name().c_str());
		}
	}
}

}

CFInput::CFInput()
	: cf_temp(0),
	  cf_prec(0),
	  cf_insol(0),
	  cf_wetdays(0),
	  cf_min_temp(0),
	  cf_max_temp(0),
	  ndep_timeseries("historic") {

	declare_parameter("ndep_timeseries", &ndep_timeseries, 10, "Nitrogen deposition time series to use (historic, rcp26, rcp45, rcp60 or rcp85");

}

CFInput::~CFInput() {
	delete cf_temp;		
	delete cf_prec;		
	delete cf_insol;	
	delete cf_wetdays;	
	delete cf_min_temp; 
	delete cf_max_temp; 

	cf_temp = 0;
	cf_prec = 0;
	cf_insol = 0;
	cf_wetdays = 0;
	cf_min_temp = 0;
	cf_max_temp = 0;
}

void CFInput::init() {

	// Read CO2 data from file
	co2.load_file(param["file_co2"].str);

	file_cru = param["file_cru"].str;

	// Try to open the NetCDF files
	try {
		cf_temp = new GridcellOrderedVariable(param["file_temp"].str, param["variable_temp"].str);
		cf_prec = new GridcellOrderedVariable(param["file_prec"].str, param["variable_prec"].str);
		cf_insol = new GridcellOrderedVariable(param["file_insol"].str, param["variable_insol"].str);

		if (param["file_wetdays"].str != "") {
			cf_wetdays = new GridcellOrderedVariable(param["file_wetdays"].str, param["variable_wetdays"].str);
		}

		if (param["file_min_temp"].str != "") {
			cf_min_temp = new GridcellOrderedVariable(param["file_min_temp"].str, param["variable_min_temp"].str);
		}

		if (param["file_max_temp"].str != "") {
			cf_max_temp = new GridcellOrderedVariable(param["file_max_temp"].str, param["variable_max_temp"].str);
		}
	}
	catch (const std::runtime_error& e) {
		fail(e.what());
	}

	// Make sure they contain what we expect

	check_temp_variable(cf_temp);

	check_prec_variable(cf_prec);

	check_insol_variable(cf_insol);

	check_wetdays_variable(cf_wetdays);

	if (cf_min_temp) {
		check_temp_variable(cf_min_temp);
	}

	if (cf_max_temp) {
		check_temp_variable(cf_max_temp);
	}

	check_compatible_timeseries(all_variables());

	check_same_spatial_domains(all_variables());

	extensive_precipitation = cf_prec->get_standard_name() == "precipitation_amount";

	// Read list of localities and store in gridlist member variable

	// Retrieve name of grid list file as read from ins file
	xtring file_gridlist=param["file_gridlist_cf"].str;

	std::ifstream ifs(file_gridlist, std::ifstream::in);

	if (!ifs.good()) fail("CFInput::init: could not open %s for input",(char*)file_gridlist);

	std::string line;
	while (getline(ifs, line)) {

		// Read next record in file
		int rlat, rlon;
		int landid;
		std::string descrip;
		Coord c;

		std::istringstream iss(line);

		if (cf_temp->is_reduced()) {
			if (iss >> landid) {
				getline(iss, descrip);

				c.landid = landid;
			}
		}
		else {
			if (iss >> rlon >> rlat) {
				getline(iss, descrip);

				c.rlat = rlat;
				c.rlon = rlon;
	
			}
		}
		c.descrip = trim(descrip);
		gridlist.push_back(c);
	}

	current_gridcell = gridlist.begin();

	// Open landcover files
	landcover_input.init();
	// Open management files
	management_input.init();

	date.set_first_calendar_year(cf_temp->get_date_time(0).get_year() - nyear_spinup);

	// Set timers
	tprogress.init();
	tmute.init();

	tprogress.settimer();
	tmute.settimer(MUTESEC);
}

bool CFInput::getgridcell(Gridcell& gridcell) {

	double lon, lat;
	double cru_lon, cru_lat;
	int soilcode;

	// Load data for next gridcell, or if that fails, skip ahead until
	// we find one that works.
	while (current_gridcell != gridlist.end() &&
	       !load_data_from_files(lon, lat, cru_lon, cru_lat, soilcode)) {
		++current_gridcell;
	}

	if (current_gridcell == gridlist.end()) {
		// simulation finished
		return false;
	}

	if (run_landcover) {
		bool LUerror = false;
		LUerror = landcover_input.loadlandcover(cru_lon, cru_lat);
		if (!LUerror)
			LUerror = management_input.loadmanagement(cru_lon, cru_lat);
		if (LUerror) {
			dprintf("\nError: could not find stand at (%g,%g) in landcover/management data file(s)\n", cru_lon, cru_lat);
			return false;
		}
	}

	gridcell.set_coordinates(lon, lat);

	// Load spinup data for all variables

	load_spinup_data(cf_temp, spinup_temp);
	load_spinup_data(cf_prec, spinup_prec);
	load_spinup_data(cf_insol, spinup_insol);

	if (cf_wetdays) {
		load_spinup_data(cf_wetdays, spinup_wetdays);
	}

	if (cf_min_temp) {
		load_spinup_data(cf_min_temp, spinup_min_temp);
	}

	if (cf_max_temp) {
		load_spinup_data(cf_max_temp, spinup_max_temp);
	}

	spinup_temp.detrend_data();

	gridcell.climate.instype = cf_standard_name_to_insoltype(cf_insol->get_standard_name());

	// Get nitrogen deposition, using the found CRU coordinates
	ndep.getndep(param["file_ndep"].str, cru_lon, cru_lat,
	             Lamarque::parse_timeseries(ndep_timeseries));

	// Setup the soil type
	soilparameters(gridcell.soiltype, soilcode);

	historic_timestep_temp = -1;
	historic_timestep_prec = -1;
	historic_timestep_insol = -1;
	historic_timestep_wetdays = -1;
	historic_timestep_min_temp = -1;
	historic_timestep_max_temp = -1;

	dprintf("\nCommencing simulation for gridcell at (%g,%g)\n", lon, lat);
	if (current_gridcell->descrip != "") {
		dprintf("Description: %s\n", current_gridcell->descrip.c_str());
	}
	dprintf("Using soil code and Nitrogen deposition for (%3.1f,%3.1f)\n", cru_lon, cru_lat);

	return true;
}

bool CFInput::load_data_from_files(double& lon, double& lat,
                                   double& cru_lon, double& cru_lat,
                                   int& soilcode) {

	int rlon = current_gridcell->rlon;
	int rlat = current_gridcell->rlat;
	int landid = current_gridcell->landid;

	// Try to load the data from the NetCDF files

	if (cf_temp->is_reduced()) {
		if (!cf_temp->load_data_for(landid) ||
		    !cf_prec->load_data_for(landid) ||
		    !cf_insol->load_data_for(landid) ||
		    (cf_wetdays && !cf_wetdays->load_data_for(landid)) ||
		    (cf_min_temp && !cf_min_temp->load_data_for(landid)) ||
		    (cf_max_temp && !cf_max_temp->load_data_for(landid))) {
			dprintf("Failed to load data for (%d) from NetCDF files, skipping.\n", landid);
			return false;
		}
	}
	else {
		if (!cf_temp->load_data_for(rlon, rlat) ||
		    !cf_prec->load_data_for(rlon, rlat) ||
		    !cf_insol->load_data_for(rlon, rlat) ||
		    (cf_wetdays && !cf_wetdays->load_data_for(rlon, rlat)) ||
		    (cf_min_temp && !cf_min_temp->load_data_for(rlon, rlat)) ||
		    (cf_max_temp && !cf_max_temp->load_data_for(rlon, rlat))) {
			dprintf("Failed to load data for (%d, %d) from NetCDF files, skipping.\n", rlon, rlat);
			return false;
		}
	}

	// Get lon/lat for the gridcell

	if (cf_temp->is_reduced()) {
		cf_temp->get_coords_for(landid, lon, lat);
	}
	else {
		cf_temp->get_coords_for(rlon, rlat, lon, lat);
	}

	// Find nearest CRU grid cell in order to get the soilcode

	cru_lon = lon;
	cru_lat = lat;
	double dummy[CRU_TS30::NYEAR_HIST][12];

	const double searchradius = 1;

	if (!CRU_TS30::findnearestCRUdata(searchradius, file_cru, cru_lon, cru_lat, soilcode,
	                                  dummy, dummy, dummy)) {
		dprintf("Failed to find soil code from CRU archive, close to coordinates (%g,%g), skipping.\n",
		        cru_lon, cru_lat);
		return false;
	}

	return true;
}

void CFInput::get_yearly_data(std::vector<double>& data,
                              const GenericSpinupData& spinup,
                              GridcellOrderedVariable* cf_historic,
                              int& historic_timestep) {
	// Extract all values for this year, for one variable,
	// either from spinup dataset or historical dataset

	int calendar_year = date.get_calendar_year();

	if (is_daily(cf_historic)) {

		data.resize(date.year_length());

		// This function is called at the first day of the year, so current_day
		// starts at Jan 1, then we step through the whole year, getting data
		// either from spinup or historical period.
		Date current_day = date;

		while (current_day.year == date.year) {

			// In the spinup?
			if (earlier_day(current_day, calendar_year, cf_historic->get_date_time(0))) {

				int spinup_day = current_day.day;
				// spinup object always has 365 days, deal with leap years
				if (current_day.ndaymonth[1] == 29 && current_day.month > 1) {
					--spinup_day;
				}
				data[current_day.day]  = spinup[spinup_day];
			}
			else {
				// Historical period

				if (historic_timestep + 1 < cf_historic->get_timesteps()) {

					++historic_timestep;
					GuessNC::CF::DateTime dt = cf_historic->get_date_time(historic_timestep);

					// Deal with calendar mismatch

					// Leap day in NetCDF variable but not in LPJ-GUESS?
					if (dt.get_month() == 2 && dt.get_day() == 29 &&
						current_day.ndaymonth[1] == 28) {
						++historic_timestep;
					}
					// Leap day in LPJ-GUESS but not in NetCDF variable?
					else if (current_day.month == 1 && current_day.dayofmonth == 28 &&
						cf_historic->get_calendar_type() == NO_LEAP) {
						--historic_timestep;
					}
				}

				if (historic_timestep < cf_historic->get_timesteps()) {
					data[current_day.day]  = cf_historic->get_value(max(0, historic_timestep));
				}
				else {
					// Past the end of the historical period, these days wont be simulated.
					data[current_day.day] = data[max(0, current_day.day-1)];
				}
			}

			current_day.next();
		}
	}
	else {

		// for now, assume that data set must be monthly since it isn't daily

		data.resize(12);

		for (int m = 0; m < 12; ++m) {

			GuessNC::CF::DateTime first_date = cf_historic->get_date_time(0);

			// In the spinup?
			if (calendar_year < first_date.get_year() ||
			    (calendar_year == first_date.get_year() &&
			     m+1 < first_date.get_month())) {
				data[m] = spinup[m];
			}
			else {
				// Historical period
				if (historic_timestep + 1 < cf_historic->get_timesteps()) {
					++historic_timestep;
				}

				if (historic_timestep < cf_historic->get_timesteps()) {
					data[m] = cf_historic->get_value(historic_timestep);
				}
				else {
					// Past the end of the historical period, these months wont be simulated.
					data[m] = data[max(0, m-1)];
				}
			}
		}
	}
}

void CFInput::populate_daily_array(double* daily,
                                   const GenericSpinupData& spinup,
                                   GridcellOrderedVariable* cf_historic,
                                   int& historic_timestep,
                                   double minimum,
                                   double maximum) {

	// Get the data from spinup and/or historic
	std::vector<double> data;
	get_yearly_data(data, spinup, cf_historic, historic_timestep);

	if (is_daily(cf_historic)) {
		// Simply copy from data to daily

		std::copy(data.begin(), data.end(), daily);
	}
	else {
		// for now, assume that data set must be monthly since it isn't daily

		// Interpolate from monthly to daily values

		interp_monthly_means_conserve(&data.front(), daily, minimum, maximum);
	}
}

void CFInput::populate_daily_prec_array(long& seed) {

	// Get the data from spinup and/or historic
	std::vector<double> prec_data;
	get_yearly_data(prec_data, spinup_prec, cf_prec, historic_timestep_prec);

	std::vector<double> wetdays_data;
	if (cf_wetdays) {
		get_yearly_data(wetdays_data, spinup_wetdays, cf_wetdays, historic_timestep_wetdays);
	}

	if (is_daily(cf_prec)) {
		// Simply copy from data to daily, and if needed convert from
		// precipitation rate to precipitation amount

		for (size_t i = 0; i < prec_data.size(); ++i) {
			dprec[i] = prec_data[i];

			if (!extensive_precipitation) {
				dprec[i] *= SECONDS_PER_DAY;
			}
		}
	}
	else {
		// for now, assume that data set must be monthly since it isn't daily

		// If needed convert from precipitation rate to precipitation amount
		if (!extensive_precipitation) {
			for (int m = 0; m < 12; ++m) {
				// TODO: use the dataset's calendar type to figure out number of days in month?
				prec_data[m] *= SECONDS_PER_DAY * date.ndaymonth[m];
			}
		}

		if (cf_wetdays) {
			prdaily(&prec_data.front(), dprec, &wetdays_data.front(), seed);
		}
		else {
			interp_monthly_totals_conserve(&prec_data.front(), dprec, 0);
		}
	}
}

void CFInput::populate_daily_arrays(long& seed) {
	// Extract daily values for all days in this year, either from
	// spinup dataset or historical dataset

	populate_daily_array(dtemp, spinup_temp, cf_temp, historic_timestep_temp, 0);
	populate_daily_prec_array(seed);
	populate_daily_array(dinsol, spinup_insol, cf_insol, historic_timestep_insol, 0,
	                     max_insolation(cf_standard_name_to_insoltype(cf_insol->get_standard_name())));

	if (cf_min_temp) {
		populate_daily_array(dmin_temp, spinup_min_temp, cf_min_temp, historic_timestep_min_temp, 0);
	}

	if (cf_max_temp) {
		populate_daily_array(dmax_temp, spinup_max_temp, cf_max_temp, historic_timestep_max_temp, 0);
	}

	// Convert to units the model expects
	bool cloud_fraction_to_sunshine = (cf_standard_name_to_insoltype(cf_insol->get_standard_name()) == SUNSHINE);
	for (int i = 0; i < date.year_length(); ++i) {
		dtemp[i] -= K2degC;

		if (cf_min_temp) {
			dmin_temp[i] -= K2degC;
		}

		if (cf_max_temp) {
			dmax_temp[i] -= K2degC;
		}

		if (cloud_fraction_to_sunshine) {
			// Invert from cloudiness to sunshine,
			// and convert fraction (0-1) to percent (0-100)
			dinsol[i] = (1-dinsol[i]) * 100.0;
		}
	}

	// Move to next year in spinup dataset

	spinup_temp.nextyear();
	spinup_prec.nextyear();
	spinup_insol.nextyear();

	if (cf_wetdays) {
		spinup_wetdays.nextyear();
	}

	if (cf_min_temp) {
		spinup_min_temp.nextyear();
	}

	if (cf_max_temp) {
		spinup_max_temp.nextyear();
	}

	// Get monthly ndep values and convert to daily

	double mndrydep[12];
	double mnwetdep[12];

	ndep.get_one_calendar_year(date.get_calendar_year(),
	                           mndrydep, mnwetdep);

	// Distribute N deposition
	distribute_ndep(mndrydep, mnwetdep, dprec, dndep);
}

void CFInput::getlandcover(Gridcell& gridcell) {

	landcover_input.getlandcover(gridcell);
	landcover_input.get_land_transitions(gridcell);
}

bool CFInput::getclimate(Gridcell& gridcell) {

	Climate& climate = gridcell.climate;

	GuessNC::CF::DateTime last_date = last_day_to_simulate(cf_temp);

	if (later_day(date, last_date)) {
		++current_gridcell;
		return false;
	}

	climate.co2 = co2[date.get_calendar_year()];

	if (date.day == 0) {
		populate_daily_arrays(gridcell.seed);
	}

	climate.temp = dtemp[date.day];
	climate.prec = dprec[date.day];
	climate.insol = dinsol[date.day];

	// Nitrogen deposition
	climate.dndep = dndep[date.day];

	// bvoc
	if(ifbvoc){
		if (cf_min_temp && cf_max_temp) {
			climate.dtr = dmax_temp[date.day] - dmin_temp[date.day];
		}
		else {
			fail("When BVOC is switched on, valid paths for minimum and maximum temperature must be given.");
		}
	}

	// First day of year only ...

	if (date.day == 0) {

		// Progress report to user and update timer

		if (tmute.getprogress()>=1.0) {

			int first_historic_year = cf_temp->get_date_time(0).get_year();
			int last_historic_year = cf_temp->get_date_time(cf_temp->get_timesteps()-1).get_year();
			int historic_years = last_historic_year - first_historic_year + 1;

			int years_to_simulate = nyear_spinup + historic_years;

			int cells_done = distance(gridlist.begin(), current_gridcell);

			double progress=(double)(cells_done*years_to_simulate+date.year)/
				(double)(gridlist.size()*years_to_simulate);
			tprogress.setprogress(progress);
			dprintf("%3d%% complete, %s elapsed, %s remaining\n",(int)(progress*100.0),
				tprogress.elapsed.str,tprogress.remaining.str);
			tmute.settimer(MUTESEC);
		}
	}

	return true;

}

void CFInput::load_spinup_data(const GuessNC::CF::GridcellOrderedVariable* cf_var,
                               GenericSpinupData& spinup_data) {

	const std::string error_message =
		format_string("Not enough data to build spinup, at least %d years needed",
		              NYEAR_SPINUP_DATA);

	GenericSpinupData::RawData source;

	int timestep = 0;

	bool daily = is_daily(cf_var);
	// for now, assume that each data set is either daily or monthly
	bool monthly = !daily;

	// Skip the first year if data doesn't start at the beginning of the year
	while ((daily && !first_day_of_year(cf_var->get_date_time(timestep))) ||
	       (monthly && !first_month_of_year(cf_var->get_date_time(timestep)))) {
		++timestep;

		if (timestep >= cf_var->get_timesteps()) {
			fail(error_message.c_str());
		}
	}

	// Get all the values for the first NYEAR_SPINUP_DATA years,
	// and put them into source
	for (int i = 0; i < NYEAR_SPINUP_DATA; ++i) {
		std::vector<double> year(daily ? GenericSpinupData::DAYS_PER_YEAR : 12);

		for (size_t i = 0; i < year.size(); ++i) {
			if (timestep < cf_var->get_timesteps()) {
				GuessNC::CF::DateTime dt = cf_var->get_date_time(timestep);

				if (daily && dt.get_month() == 2 && dt.get_day() == 29) {
					++timestep;
				}
			}

			if (timestep >= cf_var->get_timesteps()) {
				fail(error_message.c_str());
			}

			year[i] = cf_var->get_value(timestep);
			++timestep;
		}

		source.push_back(year);
	}

	spinup_data.get_data_from(source);
}

namespace {

// Help function for call to remove_if below - checks if a pointer is null
bool is_null(const GuessNC::CF::GridcellOrderedVariable* ptr) {
	return ptr == 0;
}

}

std::vector<GuessNC::CF::GridcellOrderedVariable*> CFInput::all_variables() const {
	std::vector<GuessNC::CF::GridcellOrderedVariable*> result;
	result.push_back(cf_temp);
	result.push_back(cf_prec);
	result.push_back(cf_insol);
	result.push_back(cf_wetdays);
	result.push_back(cf_min_temp);
	result.push_back(cf_max_temp);

	// Get rid of null pointers
	result.erase(std::remove_if(result.begin(), result.end(), is_null),
	             result.end());

	return result;
}

#endif // HAVE_NETCDF
