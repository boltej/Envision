#ifndef LPJGUESS_GUESSNC_H
#define LPJGUESS_GUESSNC_H

#ifdef HAVE_NETCDF

#include "cftime.h"
#include "cfvariable.h"

#include <stdexcept>
#include <string>

namespace GuessNC {

/// Exceptions thrown by this library are of this type (or sub-class)
/** Like any exception inheriting from std::runtime_error, error
 *  message can be retrieved by calling the what() member function.
 */
class GuessNCError : public std::runtime_error {
public:
	GuessNCError(const std::string& what) : std::runtime_error(what) {}
};

/// Help function to deal with status codes from NetCDF library
/** Throws GuessNCError if status indicates an error */
void handle_error(int status, const std::string& message);

/// Opens a NetCDF file for reading and returns its id
/** Throws GuessNCError if the file couldn't be opened */
int open_ncdf(const char* fname);

/// Closes a NetCDF file
/** Throws a GuessNCError if NetCDF library reports an error */
void close_ncdf(int ncid);

/// Gets the value of a string attribute for a variable
/** \returns false if the attribute didn't exist, or wasn't a string */
bool get_attribute(int ncid_file, int ncid_var, 
                   const std::string& attr_name, std::string& value);

/// Gets the value of a numeric attribute for a variable
/** \returns false if the attribute didn't exist, or couldn't be interpreted as a double */
bool get_attribute(int ncid_file, int ncid_var,
                   const std::string& attr_name, double& value);

/// Gets the dimension ids for a variable
void get_dimensions(int ncid_file, int ncid_var, std::vector<int>& dimensions);

/// Gets a coordinate variable corresponding to a dimension
/** \returns false if there was no corresponding coordinate variable */
bool get_coordinate_variable(int ncid_file, int ncid_dim, int& ncid_coord_var);

/// Gets the name of a variable with a given id
std::string get_variable_name(int ncid_file, int ncid_var);

}

#endif // HAVE_NETCDF

#endif // LPJGUESS_GUESSNC_H
