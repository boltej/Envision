#ifndef LPJGUESS_GUESSNC_CFVARIABLE_H
#define LPJGUESS_GUESSNC_CFVARIABLE_H

#ifdef HAVE_NETCDF

#include <vector>
#include <string>
#include "cftime.h"

namespace GuessNC {


namespace CF {

/// Class for reading data from a NetCDF file following a limited CF convention
/** Expected to be used for dealing with one gridcell at a time, so the NetCDF
 *  file should also be stored in that order to get acceptable performance.
 *
 *  Limitations on top of CF:
 *
 *   * Main variable and time coordinate must be numeric NetCDF classic data types.
 *
 *   * Time and 2D space dimensions are understood by the class. The main variable
 *     may also have an additional extra dimension (for instance for height), which
 *     this class simply treats as 'the extra dimension' if available.
 */
class GridcellOrderedVariable {
public:

	/// Constructor
	/** \param filename The NetCDF file to open
	 *  \param variable The name of the variable to read from
	 */
	GridcellOrderedVariable(const char* filename, const char* variable);

	/// Destructor, releases resources
	~GridcellOrderedVariable();

	/// Loads data for all timesteps for a given location
	/**
	 *  This version of the function is to be used for datasets where
	 *  locations are identified with an x- and a y-coordinate.
	 *
	 *  x and y could be coordinates in lon and lat dimensions, but could
	 *  also represent coordinates in some other coordinate system, with
	 *  longitude and latitude represented by an auxiliary coordinate 
	 *  variable.
	 *
	 *  \param x The x coordinate in main variable's coordinate system
	 *  \param y The y coordinate in main variable's coordinate system
	 *  \returns whether the location exists and has only valid (non-missing) values.
	 */
	bool load_data_for(size_t x, size_t y);

	/// Loads data for all timesteps for a given land id
	/** 
	 *  This version of the function is to be used for datasets
	 *  using a reduced horizontal grid, as in the example in 
	 *  section 5.3 of the 1.6 version of the CF spec.
	 *
	 *  \param landid Index identifying the cell in the main variable's coordinate system
	 *  \returns whether the location exists and has only valid (non-missing) values.
	 */
	bool load_data_for(size_t landid);

	/// Are locations identified with one or two indices?
	/** In a variable with a reduced horizontal grid, the locations are
	 *  identified with a simple land id. Otherwise an (x,y)-pair is used.
	 *
	 *  \see load_data_for
	 */
	bool is_reduced() const;

	/// Retrieves actual (lon, lat)-coordinates for a given location
	void get_coords_for(size_t x, size_t y, double& lon, double& lat) const;

	/// Retrieves actual (lon,lat)-coordinates for a given location
	void get_coords_for(size_t landid, double& lon, double& lat) const;

	/// Gets the number of timesteps for the variable
	int get_timesteps() const;

	/// Gets variable's value for currently loaded location in a certain timestep
	/** Use this function to retrieve values in a 3 dimensional data set,
	 *  where each (lat,lon,time) triple corresponds to a single scalar value.
	 *
	 *  \see get_values for data sets with an extra dimension
	 *
	 *  If this function is used with a data set with an extra dimension,
	 *  it will return the first value for the given time step.
	 */
	double get_value(int timestep) const;

	/// Gets all values for currently loaded location in a certain timestep
	/** Use this function to retrieve values in a 4 dimensional data set,
	 *  for instance when height is a fourth dimension.
	 *
	 *  The values vector will be resized if necessary.
	 */
	void get_values(int timestep, std::vector<double>& values) const;

	/// Gets a DateTime corresponding to a timestep
	DateTime get_date_time(int timestep) const;

	/// Checks if data exists for a given location
	bool location_exists(size_t x, size_t y) const;

	/// Checks if data exists for a given location
	bool location_exists(size_t landid) const;

	/// Returns the 'units' attribute of the main variable
	std::string get_units() const;

	/// Returns the main variable's name
	std::string get_variable_name() const;

	/// Returns the main variable's standard name
	/** Can be used to identify the variable */
	std::string get_standard_name() const;

	/// Returns a descriptive name of the variable
	/** Should be used for presentation for the user, not for
	 *  identification of the variable.
	 */
	std::string get_long_name() const;

	/// Gets the values for each coordinate value in the 'extra' dimension
	/** Returns an empty vector if there is no extra dimension.
	 */
	void get_extra_dimension(std::vector<double>& coordinate_values) const;

	/// Gets the values for each coordinate value in the 'extra' dimension
	/** Returns an empty vector if there is no extra dimension.
	 *
	 *  Only defined if compiled with support for NetCDF4 files (needs the
	 *  string data type).
	 */
	void get_extra_dimension(std::vector<std::string>& coordinate_values) const;

	/// \returns the calendar type used by this variable
	CalendarType get_calendar_type() const;

	/// Checks if another GridcellOrderedVariable has the same spatial domain as this one
	bool same_spatial_domain(const GridcellOrderedVariable& other) const;

private:

	/** Called by constructor to figure out all we need to know about
	 *  the time dimension and how to interpret time.
	 *
	 *  Assumes ncid_file and ncid_var already set.
	 */
	void find_time_dimension();

	/** Called by constructor to figure out all we need to know about
	 *  the spatial dimensions.
	 *
	 *  Assumes ncid_file and ncid_var already set.
	 */
	void find_spatial_dimensions();

	/** Called by constructor to figure out if there is an extra
	 *  dimension, and if so, its id and size.
	 *
	 *  Assumes time and spatial dimensions already found.
	 */
	void find_extra_dimension();

	/** Unpacks the raw data according to scale_factor and add_offset
	 *  arguments, if present.
	 */
	void unpack_data();

	/// Help function for same_spatial_domain, compares either the lat coordinate variable or lon
	bool same_spatial_coordinates(int ncid_my_coordvar, 
		const GridcellOrderedVariable& other, 
		int ncid_other_coordvar) const;

	/// NC handle to NetCDF file
	int ncid_file;

	/// NC handle to main variable
	int ncid_var;

	/// Name of main variable (for error messages)
	std::string variable_name;

	/// Data for all timesteps for current location
	std::vector<double> data;

	/// Time offsets for all timesteps
	/** The times are relative to a starting time given in time_spec.
	 *  This follows how times are represented in CF, see CF spec
	 *  for overview.
	 */
	std::vector<double> time;

	/// Reference time specification
	/** Defines how the time offsets in the time vector are to be
	 *  interpreted, \see TimeUnitSpecification for more information.
	 */
	TimeUnitSpecification time_spec;

	/// The calendar used by the variable
	CalendarType calendar;

	// The order of the dimensions in the main variable

	size_t t_dimension_index;
	size_t x_dimension_index;
	size_t y_dimension_index;
	size_t landid_dimension_index;
	size_t extra_dimension_index;

	// NC handles to the dimensions of the main variable

	int ncid_x_dimension;
	int ncid_y_dimension;
	int ncid_landid_dimension;
	int ncid_t_dimension;
	int ncid_extra_dimension;

	// NC handles to variables with lon/lat values,
	// either regular coordinate variables as in COARDS, or auxiliary 
	// coordinate variables as defined by CF

	int ncid_lon;
	int ncid_lat;

	/// Whether the spatial coordinate system is a reduced horizontal grid
	bool reduced;

	/// The size of the 'extra' dimension (for instance height)
	/** Set to 1 if there is no extra dimension */
	size_t extra_dimension_size;
};

}

}

#endif // HAVE_NETCDF

#endif // LPJGUESS_GUESSNC_CFVARIABLE_H
