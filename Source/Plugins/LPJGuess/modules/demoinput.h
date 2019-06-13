///////////////////////////////////////////////////////////////////////////////////////
/// \file demoinput.h
/// \brief Input module for demo data set
///
/// \author Joe Siltberg
/// $Date: 2015-11-13 16:25:45 +0100 (Fri, 13 Nov 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_DEMOINPUT_H
#define LPJ_GUESS_DEMOINPUT_H

#include "guess.h"
#include "inputmodule.h"
#include <vector>
#include "gutil.h"
#include "externalinput.h"

/// An input module for a toy data set (for demonstration purposes)
/** This input module is provided as an example of an input module.
 *  Included together with the LPJ-GUESS source code is a small
 *  toy data set (text based) which can be used to test the model,
 *  or to learn about how to write input modules.
 *
 *  \see InputModule for more documentation about writing input modules.
 */
class DemoInput : public InputModule {
public:

	/// Constructor
	/** Declares the instruction file parameters used by the input module.
	 */
	DemoInput();

	/// Destructor, cleans up used resources
	~DemoInput();

	/// Reads in gridlist and initialises the input module
	/** Gets called after the instruction file has been read */
	void init();

	/// See base class for documentation about this function's responsibilities
	bool getgridcell(Gridcell& gridcell);

	/// See base class for documentation about this function's responsibilities
	bool getclimate(Gridcell& gridcell);

	/// See base class for documentation about this function's responsibilities
	void getlandcover(Gridcell& gridcell);

	/// Obtains land management data for one day
	void getmanagement(Gridcell& gridcell) {management_input.getmanagement(gridcell);}

private:

	/// Type for storing grid cell longitude, latitude and description text
	struct Coord {

		int id;
		double lon;
		double lat;
		xtring descrip;
	};

	/// Land cover input module
	LandcoverInput landcover_input;
	/// Management input module
	ManagementInput management_input;

	/// Help function to readenv, reads in 12 monthly values from a text file
	bool read_from_file(Coord coord, xtring fname, const char* format,
	                    double monthly[12], bool soil = false);

	/// Reads in environmental data for a location
	bool readenv(Coord coord, long& seed);

	/// number of simulation years to run after spinup
	int nyear;

	/// A list of Coord objects containing coordinates of the grid cells to simulate
	ListArray_id<Coord> gridlist;

	/// Flag for getgridcell(). True indicates that the first gridcell has not been read yet by getgridcell()
	bool first_call;

	// Timers for keeping track of progress through the simulation
	Timer tprogress,tmute;
	static const int MUTESEC=20; // minimum number of sec to wait between progress messages

	// Daily temperature, precipitation and sunshine for one year
	double dtemp[Date::MAX_YEAR_LENGTH];
	double dprec[Date::MAX_YEAR_LENGTH];
	double dsun[Date::MAX_YEAR_LENGTH];
	// bvoc
	// Daily diurnal temperature range for one year
	double ddtr[Date::MAX_YEAR_LENGTH];

	/// atmospheric CO2 concentration (ppmv) (read from ins file)
	double co2;

	/// atmospheric nitrogen deposition (kgN/yr/ha) (read from ins file)
	double ndep;

};

#endif // LPJ_GUESS_DEMOINPUT_H
