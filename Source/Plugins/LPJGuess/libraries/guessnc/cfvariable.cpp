#ifdef HAVE_NETCDF

#include "cfvariable.h"
#include "cf.h"
#include "guessnc.h"
#include <netcdf.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <numeric>

// todo:
//  Find gridcell by lat/lon (with epsilon), range search?
//  Get data by DateTime (closest following?)
//  Getting sizes of spatial dimensions

namespace GuessNC {

/// Check whether a data type is one of the NetCDF classic numeric data types
bool classic_numeric_type(nc_type type) {
	return 
		type == NC_BYTE ||
		type == NC_SHORT ||
		type == NC_INT ||
		type == NC_FLOAT ||
		type == NC_DOUBLE;
}

namespace CF {

GridcellOrderedVariable::
GridcellOrderedVariable(const char* filename, 
                        const char* variable) {

	// Constructor - opens the file and figures out how time works

	// Open the file
	ncid_file = open_ncdf(filename);

	// Get a handle to the main variable
	int status = nc_inq_varid(ncid_file, variable, &ncid_var);
	handle_error(status, std::string("Failed to get variable ") + variable);

	variable_name = GuessNC::get_variable_name(ncid_file, ncid_var);

	// Verify datatype - we require one of the NetCDF classic numeric types,
	// even though that's not a CF requirement. The 64 bit integral types in
	// NetCDF4 would require special care since we can't represent that with
	// a double.
	nc_type type;
	status = nc_inq_vartype(ncid_file, ncid_var, &type);
	handle_error(status, 
	             std::string("Failed to get type of main variable: ") +
	             variable_name);
	
	if (!classic_numeric_type(type)) {
		throw CFError(variable, "Main variable must be a NetCDF classic numeric type");
	}

	// Figure out how time works in this file
	find_time_dimension();

	// Figure out how the horizontal coordinate system works in this file
	find_spatial_dimensions();

	// Verify that we haven't identified any dimension in two different roles
	if (reduced) {
		if (t_dimension_index == landid_dimension_index ||
		    ncid_t_dimension == ncid_landid_dimension) {
			throw CFError(variable, "Ambiguous land/time dimensions");
		}
	}
	else {
		if (x_dimension_index == y_dimension_index ||
		    x_dimension_index == t_dimension_index ||
		    y_dimension_index == t_dimension_index ||
		    ncid_x_dimension == ncid_y_dimension ||
		    ncid_x_dimension == ncid_t_dimension ||
		    ncid_y_dimension == ncid_t_dimension) {
			throw CFError(variable, "Ambiguous x/y/time dimensions");
		}
	}

	// Figure out if there is an extra dimension
	find_extra_dimension();

	// Verify that the variable has no dimensions apart from those
	// which we have already identified and know how to interpret
	std::vector<int> dims;
	get_dimensions(ncid_file, ncid_var, dims);

	for (size_t d = 0; d < dims.size(); ++d) {
		int dim = dims[d];
		
		if (dim != ncid_x_dimension &&
		    dim != ncid_y_dimension &&
		    dim != ncid_landid_dimension &&
		    dim != ncid_t_dimension &&
		    dim != ncid_extra_dimension) {
			throw CFError(variable, "Unsupported number of dimensions for variable");
		}
	}
}

void GridcellOrderedVariable::find_time_dimension() {
	// Get the dimensions of the main variable
	std::vector<int> ncid_dims;
	get_dimensions(ncid_file, ncid_var, ncid_dims);
	
	bool foundit = false;
	int ncid_timecoord;
	int status;

	// For each dimension, check the corresponding coordinate variable
	for (size_t d = 0; d < ncid_dims.size(); ++d) {
		
		// Get the corresponding coordinate variable
		int ncid_coord_var;
		if (!get_coordinate_variable(ncid_file, ncid_dims[d], ncid_coord_var)) {
			continue;
		}

		// According to CF spec, time coordinate variables must have a units attribute
		std::string units;
		if (!get_attribute(ncid_file, ncid_coord_var, "units", units)) {
			continue;
		}

		// Is the units string a valid time spec?
		try {
			TimeUnitSpecification tus(units.c_str());
		}
		catch (const CFError&) {
			continue;
		}
		
		// Verify datatype - we require one of the NetCDF classic numeric types,
		// even though that's not a CF requirement. The 64 bit integral types in
		// NetCDF4 would require special care since we can't represent that with
		// a double.
		nc_type type;
		status = nc_inq_vartype(ncid_file, ncid_coord_var, &type);
		handle_error(status, 
		             std::string("Failed to get type of time variable: ") +
		             GuessNC::get_variable_name(ncid_file, ncid_coord_var));

		if (!classic_numeric_type(type)) {
			throw CFError(variable_name, "Time coordinates must be a NetCDF classic numeric type");
		}
		
		// Make sure there's only one time dimension associated with the main variable
		if (foundit) {
			throw CFError(variable_name, "More than one time dimension associated with main variable");
		}
		else {
			foundit = true;
			ncid_t_dimension = ncid_dims[d];
			ncid_timecoord = ncid_coord_var;
			t_dimension_index = d;
		}
	}

	if (!foundit) {
		throw CFError(variable_name, "No proper time dimension found");
	}

	// We've now found the time dimension and coordinate variable
	// read in time data and store the time unit specification

	size_t nbr_timesteps;
	status = nc_inq_dimlen(ncid_file, ncid_t_dimension, &nbr_timesteps);

	// Read in the time variable
	time.resize(nbr_timesteps);

	status = nc_get_var_double(ncid_file, ncid_timecoord, &time.front());

	// Get information about calendar and time unit
	std::string calendar_attribute;
	if (!get_attribute(ncid_file, ncid_timecoord, "calendar", calendar_attribute)) {
		throw CFError(variable_name, "Time coordinate variable didn't have a proper calendar attribute");
	}
	
	calendar = parse_calendar(calendar_attribute.c_str());

	std::string units_attribute;
	if (!get_attribute(ncid_file, ncid_timecoord, "units", units_attribute)) {
		throw CFError(variable_name, "Time coordinate variable didn't have a proper units attribute");
	}

	time_spec = TimeUnitSpecification(units_attribute.c_str());

	// Try getting the reference time with the given calendar.
	// If the standard/gregorian calendar is used, we only allow dates
	// after the Julian/Gregorian switch, time_spec will throw an
	// exception if a year before 1583 is used with a standard/gregorian
	// calendar.
	time_spec.get_date_time(0, calendar);
}


bool is_longitude_coordinate_variable(const std::string& units, 
                                      const std::string& standard_name) {
	static const char* acceptable_units[] = {
		"degree_east",
		"degrees_east",
		"degree_E",
		"degrees_E",
		0 };

	if (standard_name == "longitude") {

		for (int i = 0; acceptable_units[i] != 0; ++i) {
			if (units == acceptable_units[i]) {
				return true;
			}
		}

	}
	
	return false;
}


bool is_latitude_coordinate_variable(const std::string& units, 
                                     const std::string& standard_name) {
	static const char* acceptable_units[] = {
		"degree_north",
		"degrees_north",
		"degree_N",
		"degrees_N",
		0 };

	if (standard_name == "latitude") {

		for (int i = 0; acceptable_units[i] != 0; ++i) {
			if (units == acceptable_units[i]) {
				return true;
			}
		}

	}
	
	return false;
}


void GridcellOrderedVariable::find_spatial_dimensions() {

	// -1 is an invalid variable and dimension id
	ncid_lon = -1;
	ncid_lat = -1;
	ncid_x_dimension = -1;
	ncid_y_dimension = -1;
	ncid_landid_dimension = -1;
	
	// Loop over main variable's dimensions, try to find out
	// which corresponds to x and y

	std::vector<int> ncid_dims;
	get_dimensions(ncid_file, ncid_var, ncid_dims);

	for (size_t d = 0; d < ncid_dims.size(); ++d) {

		if (ncid_dims[d] == ncid_t_dimension) {
			continue;
		}

		// Get corresponding coordinate variable
		int ncid_coord_var;
		if (!get_coordinate_variable(ncid_file, ncid_dims[d], ncid_coord_var)) {
			continue;
		}

		// Check units and standard_name first to see if it's a regular lon/lat
		std::string units, standard_name;
		if (get_attribute(ncid_file, ncid_coord_var, "units", units) &&
		    get_attribute(ncid_file, ncid_coord_var, "standard_name", standard_name)) {
			
			if (is_longitude_coordinate_variable(units, standard_name)) {
				// This dimension should be the x axis, and it has a corresponding
				// coordinate variable for getting longitudes
				x_dimension_index = d;
				ncid_x_dimension = ncid_dims[d];
				ncid_lon = ncid_coord_var;
				continue;
			}
			else if (is_latitude_coordinate_variable(units, standard_name)) {
				// This dimension should be the y axis, and it has a corresponding
				// coordinate variable for getting latitudes
				y_dimension_index = d;
				ncid_y_dimension = ncid_dims[d];
				ncid_lat = ncid_coord_var;
				continue;
			}
		}

		// Otherwise, check if it has an axis attribute indicating X or Y
		std::string axis;
		if (get_attribute(ncid_file, ncid_coord_var, "axis", axis)) {
			if (axis == "X") {
				// Found an x axis, but will need to get a longitude (auxiliary)
				// coordinate variable via the 'coordinates' attribute
				x_dimension_index = d;
				ncid_x_dimension = ncid_dims[d];
			}
			else if (axis == "Y") {
				// Found a y axis, but will need to get a latitude (auxiliary)
				// coordinate variable via the 'coordinates' attribute
				y_dimension_index = d;
				ncid_y_dimension = ncid_dims[d];
			}
		}
	}
	
	// If we didn't find lat or lon coordinate variables by now, require that main 
	// variable has a 'coordinates' attribute, use that to get longitude and 
	// latitude (auxiliary) coordinate variables
	if (ncid_lon == -1 && ncid_lat == -1) {

		std::string coordinates;
		if (!get_attribute(ncid_file, ncid_var, "coordinates", coordinates)) {
			throw CFError(variable_name + " isn't indexed by lat/lon and"\
			              " doesn't have a 'coordinates' attribute");
		}

		std::istringstream is(coordinates);
		std::string coordinate;

		while (is >> coordinate) {

			int ncid_coord_var;
			int status = nc_inq_varid(ncid_file, coordinate.c_str(), &ncid_coord_var);
			handle_error(status, "Failed getting coordinate variable specified in " + 
			             variable_name + ":coordinates");

			std::string units, standard_name;

			if (get_attribute(ncid_file, ncid_coord_var, "units", units) &&
			    get_attribute(ncid_file, ncid_coord_var, "standard_name", standard_name)) {

				if (is_longitude_coordinate_variable(units, standard_name)) {
					ncid_lon = ncid_coord_var;
				}
				else if (is_latitude_coordinate_variable(units, standard_name)) {
					ncid_lat = ncid_coord_var;
				}
			}
		}
	}

	if (ncid_lon == -1 || ncid_lat == -1) {
		throw CFError(variable_name, "Couldn't find coordinate variables for lat/lon");
	}

	// If we only found one of x or y, something is wrong
	if ( (ncid_x_dimension >= 0) != (ncid_y_dimension >= 0) ) {
		throw CFError(variable_name, "Found one spatial dimension, but not the other");
	}

	// If neither, see if main variable has a dimension which also is the one 
	// and only dimension for the lon/lat auxiliary coordinate variables
	std::vector<int> ncid_lon_dims, ncid_lat_dims;
	get_dimensions(ncid_file, ncid_lon, ncid_lon_dims);
	get_dimensions(ncid_file, ncid_lat, ncid_lat_dims);

	if (ncid_lon_dims.size() == 1 &&
	    ncid_lat_dims.size() == 1 &&
	    ncid_lon_dims.front() == ncid_lat_dims.front()) {

		std::vector<int>::iterator itr = 
			std::find(ncid_dims.begin(), ncid_dims.end(), ncid_lon_dims.front());
		
		if (itr != ncid_dims.end()) {
			landid_dimension_index = itr-ncid_dims.begin();
			ncid_landid_dimension = ncid_lon_dims.front();
		}
		else {
			throw CFError(variable_name, "Failed to find x or y axis, or reduced horizontal grid");
		}
	}

	reduced = ncid_landid_dimension != -1;
}


void GridcellOrderedVariable::find_extra_dimension() {
	ncid_extra_dimension = -1;
	extra_dimension_size = 1;

	// Go through all dimensions, if we find one that isn't already
	// identified as a time or spatial dimension, we'll assume
	// that it's an extra dimension.

	std::vector<int> ncid_dims;
	get_dimensions(ncid_file, ncid_var, ncid_dims);

	for (size_t d = 0; d < ncid_dims.size(); ++d) {
		int dim = ncid_dims[d];
		if (dim != ncid_t_dimension &&
		    dim != ncid_x_dimension &&
		    dim != ncid_y_dimension &&
		    dim != ncid_landid_dimension) {
			
			// Found an unidentified dimension
			extra_dimension_index = d;
			ncid_extra_dimension = dim;

			int status = nc_inq_dimlen(ncid_file, ncid_extra_dimension, &extra_dimension_size);
			handle_error(status, "Failed to get size of extra dimension for " + variable_name);

			break;
		}
	}
}


bool GridcellOrderedVariable::load_data_for(size_t x, size_t y) {
	data.resize(get_timesteps() * extra_dimension_size);

	if (!location_exists(x, y)) {
		return false;
	}

	size_t start[4];
	start[x_dimension_index] = x;
	start[y_dimension_index] = y;
	start[t_dimension_index] = 0;

	size_t count[4];
	count[x_dimension_index] = 1;
	count[y_dimension_index] = 1;
	count[t_dimension_index] = get_timesteps();

	ptrdiff_t imap[4];
	imap[x_dimension_index] = 1;
	imap[y_dimension_index] = 1;
	imap[t_dimension_index] = extra_dimension_size;

	if (ncid_extra_dimension != -1) {
		start[extra_dimension_index] = 0;
		count[extra_dimension_index] = extra_dimension_size;
		imap[extra_dimension_index] = 1;
	}

	int status = nc_get_varm_double(ncid_file, ncid_var, start, count, 0, imap, &data.front());
	handle_error(status, 
	             std::string("Failed to read data from variable ") + variable_name);

	// Check if the data for this location contains a missing value
	double missing_value;
	if (get_attribute(ncid_file, ncid_var, "missing_value", missing_value)) {
		if (std::find(data.begin(), data.end(), missing_value) != data.end()) {
			return false;
		}
	}

	unpack_data();
	
	return true;
}


bool GridcellOrderedVariable::load_data_for(size_t landid) {
	data.resize(get_timesteps());

	if (!location_exists(landid)) {
		return false;
	}

	size_t start[3];
	start[landid_dimension_index] = landid;
	start[t_dimension_index] = 0;

	size_t count[3];
	count[landid_dimension_index] = 1;
	count[t_dimension_index] = get_timesteps();

	ptrdiff_t imap[3];
	imap[landid_dimension_index] = 1;
	imap[t_dimension_index] = extra_dimension_size;

	if (ncid_extra_dimension != -1) {
		start[extra_dimension_index] = 0;
		count[extra_dimension_index] = extra_dimension_size;
		imap[extra_dimension_index] = 1;
	}

	int status = nc_get_varm_double(ncid_file, ncid_var, start, count, 0, imap, &data.front());
	handle_error(status,
	             std::string("Failed to read data from variable ") + variable_name);

	// Check if the data for this location contains a missing value
	double missing_value;
	if (get_attribute(ncid_file, ncid_var, "missing_value", missing_value)) {
		if (std::find(data.begin(), data.end(), missing_value) != data.end()) {
			return false;
		}
	}

	unpack_data();

	return true;
}

bool GridcellOrderedVariable::is_reduced() const {
	return reduced;
}

GridcellOrderedVariable::~GridcellOrderedVariable() {
	close_ncdf(ncid_file);
}


void GridcellOrderedVariable::get_coords_for(size_t x, size_t y, 
                                             double& lon, double& lat) const {

	if (!location_exists(x, y)) {
		std::ostringstream os;
		os << "Attempted to get coordinates for invalid location: " << x << ", " << y;
		throw CFError(variable_name, os.str());
	}

	size_t lon_index[2];

	// Get the dimensions of the lon variable
	std::vector<int> londims;
	get_dimensions(ncid_file, ncid_lon, londims);
	
	// Loop over the dimensions to set up lon_index correctly with values x and y
	for (size_t d = 0; d < londims.size(); ++d) {

		if (londims[d] == ncid_x_dimension) {
			lon_index[d] = x;
		}
		else if (londims[d] == ncid_y_dimension) {
			lon_index[d] = y;
		}
		else {
			throw CFError(variable_name, "Unexpected dimension found in longitude coordinate variable");
		}

	}

	int status = nc_get_var1_double(ncid_file, ncid_lon, lon_index, &lon);
	handle_error(status, "Failed lon indexing");


	size_t lat_index[2];

	// Get the dimensions of the lat variable
	std::vector<int> latdims;
	get_dimensions(ncid_file, ncid_lat, latdims);
	
	// Loop over the dimensions to set up lat_index correctly with values x and y
	for (size_t d = 0; d < latdims.size(); ++d) {

		if (latdims[d] == ncid_x_dimension) {
			lat_index[d] = x;
		}
		else if (latdims[d] == ncid_y_dimension) {
			lat_index[d] = y;
		}
		else {
			throw CFError(variable_name, "Unexpected dimension found in latitude coordinate variable");
		}

	}

	status = nc_get_var1_double(ncid_file, ncid_lat, lat_index, &lat);
	handle_error(status, "Failed lat indexing");
}


void GridcellOrderedVariable::get_coords_for(size_t landid,
                                             double& lon, double& lat) const {
	if (!location_exists(landid)) {
		std::ostringstream os;
		os << "Attempted to get coordinates for invalid location: " << landid;
		throw CFError(variable_name, os.str());
	}

	const size_t index[] = { landid };

	int status = nc_get_var1_double(ncid_file, ncid_lon, index, &lon);
	handle_error(status, "Failed lon indexing");

	status = nc_get_var1_double(ncid_file, ncid_lat, index, &lat);
	handle_error(status, "Failed lat indexing");
}

int GridcellOrderedVariable::get_timesteps() const {
	return time.size();
}

double GridcellOrderedVariable::get_value(int timestep) const {
	return data[timestep * extra_dimension_size];
}

void GridcellOrderedVariable::get_values(int timestep, std::vector<double>& values) const {
	if (values.size() < extra_dimension_size) {
		values.resize(extra_dimension_size);
	}

	for (size_t i = 0; i < extra_dimension_size; ++i) {
		values[i] = data[timestep * extra_dimension_size + i];
	}
}

DateTime GridcellOrderedVariable::get_date_time(int timestep) const {
	return time_spec.get_date_time(time[timestep], calendar);
}

bool GridcellOrderedVariable::location_exists(size_t x, size_t y) const {

	if (reduced) {
		throw CFError(variable_name, "Attempted to identify location with (x,y)-pair"\
		              " in file with reduced grid");
	}

	size_t x_len, y_len;

	int status = nc_inq_dimlen(ncid_file, ncid_x_dimension, &x_len);
	handle_error(status, "Failed to get x-dimension length");

	status = nc_inq_dimlen(ncid_file, ncid_y_dimension, &y_len);
	handle_error(status, "Failed to get y-dimension length");

	return x < x_len && y < y_len;
}

bool GridcellOrderedVariable::location_exists(size_t landid) const {
	if (!reduced) {
		throw CFError(variable_name, "Attempted to identify a location with a single index"\
		              " in a file with (x,y)-coordinate system");
	}

	size_t land_len;
	
	int status = nc_inq_dimlen(ncid_file, ncid_landid_dimension, &land_len);
	handle_error(status, "Failed to get land dimension length");

	return landid < land_len;
}

std::string GridcellOrderedVariable::get_units() const {
	std::string result;
	if (get_attribute(ncid_file, ncid_var, "units", result)) {
		return result;
	}
	else {
		// The units attribute isn't required by the CF standard
		return "";
	}
}

std::string GridcellOrderedVariable::get_variable_name() const {
	return variable_name;
}

std::string GridcellOrderedVariable::get_standard_name() const {
	std::string attribute;
	if (get_attribute(ncid_file, ncid_var, "standard_name", attribute)) {

		// Get the first token from the string, ignoring "Standard Name Modifier"
		// if there happens to be one

		std::string result;
		std::istringstream is(attribute);
		is >> result;
		
		return result;
	}
	else {
		// The standard_name attribute isn't required by the CF standard
		return "";
	}
}

std::string GridcellOrderedVariable::get_long_name() const {
	std::string result;
	if (get_attribute(ncid_file, ncid_var, "long_name", result)) {
		return result;
	}
	else {
		// The long_name attribute isn't required by the CF standard
		return "";
	}
}

void GridcellOrderedVariable::
get_extra_dimension(std::vector<double>& coordinate_values) const {
	if (ncid_extra_dimension == -1) {
		coordinate_values.resize(0);
	}
	else {
		// Find the corresponding coordinate variable
		int ncid_coord;
		if (!get_coordinate_variable(ncid_file, ncid_extra_dimension, ncid_coord)) {
			throw CFError(variable_name, "Extra dimension has no coordinate variable");
		}

		size_t start[] = { 0 };
		size_t count[] = { extra_dimension_size };

		if (coordinate_values.size() < extra_dimension_size) {
			coordinate_values.resize(extra_dimension_size);
		}

		int status = nc_get_vara_double(ncid_file, ncid_coord, start, count, &coordinate_values.front());
		handle_error(status, "Failed to read values from extra dimension coordinate variable");
	}
}

CalendarType GridcellOrderedVariable::get_calendar_type() const {
	return calendar;
}

bool GridcellOrderedVariable::same_spatial_coordinates(int ncid_my_coordvar, 
	const GridcellOrderedVariable& other, 
	int ncid_other_coordvar) const {

	// Compare dimensions
	std::vector<int> mydims;
	get_dimensions(ncid_file, ncid_my_coordvar, mydims);

	std::vector<int> otherdims;
	get_dimensions(other.ncid_file, ncid_other_coordvar, otherdims);

	if (mydims.size() != otherdims.size()) {
		return false;
	}

	std::vector<size_t> dim_sizes(mydims.size());

	for (size_t i = 0; i < mydims.size(); ++i) {
		size_t mydimlen, otherdimlen;

		const std::string error_message("Failed to get dimension length of coordinate variable associated with variable ");

		int status = nc_inq_dimlen(ncid_file, mydims[i], &mydimlen);
		handle_error(status, error_message + variable_name);

		status = nc_inq_dimlen(other.ncid_file, otherdims[i], &otherdimlen);
		handle_error(status, error_message + other.variable_name);

		if (mydimlen != otherdimlen) {
			return false;
		}

		dim_sizes[i] = mydimlen;
	}

	// Compare data for the coordinate variables
	size_t total_size = std::accumulate(dim_sizes.begin(), dim_sizes.end(), 1, std::multiplies<size_t>());

	std::vector<double> mydata(total_size), otherdata(total_size);

	std::vector<size_t> start(mydims.size(), 0);

	const std::string error_message("Failed to get data for coordinate variable associated with variable ");

	int status = nc_get_vara_double(ncid_file, ncid_my_coordvar, &start[0], &dim_sizes[0], &mydata[0]);
	handle_error(status, error_message + variable_name);

	status = nc_get_vara_double(other.ncid_file, ncid_other_coordvar, &start[0], &dim_sizes[0], &otherdata[0]);
	handle_error(status, error_message + variable_name);

	return mydata == otherdata;
}

bool GridcellOrderedVariable::same_spatial_domain(const GridcellOrderedVariable& other) const {
	if (reduced != other.reduced) {
		return false;
	}

	return same_spatial_coordinates(ncid_lat, other, other.ncid_lat) &&
		same_spatial_coordinates(ncid_lon, other, other.ncid_lon);
}

#ifdef NC_STRING
void GridcellOrderedVariable::
get_extra_dimension(std::vector<std::string>& coordinate_values) const {
}
#endif

void GridcellOrderedVariable::unpack_data() {

	// First, multiply all data by scale_factor (if present)

	double factor;
	if (get_attribute(ncid_file, ncid_var, "scale_factor", factor)) {

		for (size_t i = 0; i < data.size(); ++i) {
			data[i] *= factor;
		}
	}

	// Then add add_offset (if present)

	double offset;
	if (get_attribute(ncid_file, ncid_var, "add_offset", offset)) {

		for (size_t i = 0; i < data.size(); ++i) {
			data[i] += offset;
		}		
	}
}

} // namespace CF

} // namespace GuessNC

#endif // #ifdef HAVE_NETCDF
