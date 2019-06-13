#ifdef HAVE_NETCDF

#include "guessnc.h"
#include <netcdf.h>

namespace GuessNC {

void handle_error(int status, const std::string& message) {
	// todo: simplify formatting message for more helpful messages?
	if (status != NC_NOERR) {
		std::string error_message = 
			std::string("NetCDF error: ") + nc_strerror(status) + ": " + message;

		throw GuessNCError(error_message);
	}
}

int open_ncdf(const char* fname) {
	int netcdf_id;
	int status = nc_open(fname, NC_NOWRITE, &netcdf_id);
	handle_error(status, (std::string("Cannot open NetCDF file: ") + fname).c_str());
	return netcdf_id;
}

void close_ncdf(int ncid) {
	int status = nc_close(ncid);
	handle_error(status, "Failed to close NetCDF file");
}

bool get_attribute(int ncid_file, int ncid_var,
                   const std::string& attr_name, std::string& value) {
	nc_type type;
	size_t len;
	int status = nc_inq_att(ncid_file, ncid_var, attr_name.c_str(), &type, &len);

	if (status == NC_ENOTATT) {
		return false;
	}

	handle_error(status, std::string("Failed to get attribute: ") + attr_name);

	if (type != NC_CHAR) {
		return false;
	}

	std::vector<char> buff(len+1, 0);
	status = nc_get_att_text(ncid_file, ncid_var, attr_name.c_str(), &buff.front());
	handle_error(status, "Failed to get attribute: " + attr_name);

	value = &buff.front();

	return true;
}

bool get_attribute(int ncid_file, int ncid_var,
                   const std::string& attr_name, double& value) {
	nc_type type;
	size_t len;
	int status = nc_inq_att(ncid_file, ncid_var, attr_name.c_str(), &type, &len);

	if (status == NC_ENOTATT) {
		return false;
	}

	handle_error(status, std::string("Failed to get attribute: ") + attr_name);

	status = nc_get_att_double(ncid_file, ncid_var, attr_name.c_str(), &value);
	handle_error(status, "Failed to get attribute: " + attr_name);

	return true;
}

void get_dimensions(int ncid_file, int ncid_var,
                    std::vector<int>& dimensions) {
	int ndims;
	int status = nc_inq_varndims(ncid_file, ncid_var, &ndims);
	handle_error(status, "Failed to get number of dimensions of variable" +
	             get_variable_name(ncid_file, ncid_var));

	dimensions.resize(ndims);

	status = nc_inq_vardimid(ncid_file, ncid_var, &dimensions.front());
	handle_error(status, "Failed to get dimensions for variable" +
	             get_variable_name(ncid_file, ncid_var));
}

bool get_coordinate_variable(int ncid_file, int ncid_dim, int& ncid_coord_var) {
	// Get the name of the dimension
	char name[NC_MAX_NAME+1];
	int status = nc_inq_dimname(ncid_file, ncid_dim, name);
	handle_error(status, "Failed to get name of a dimension");

	status = nc_inq_varid(ncid_file, name, &ncid_coord_var);

	if (status == NC_ENOTVAR) {
		return false;
	}

	handle_error(status,
	             std::string("Failed to get coordinate variable " \
	                         "corresponding to dimension: ") + name);
	return true;
}

std::string get_variable_name(int ncid_file, int ncid_var) {
	char name[NC_MAX_NAME+1];
	int status = nc_inq_varname(ncid_file, ncid_var, name);
	handle_error(status, "Failed to get name of a variable");

	return name;
}

} // namespace GuessNC 

#endif // HAVE_NETCDF
