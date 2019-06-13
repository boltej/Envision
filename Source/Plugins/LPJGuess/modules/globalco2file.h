///////////////////////////////////////////////////////////////////////////////////////
/// \file globalco2file.h
/// \brief A class for reading in CO2 values from a text file
///
///
/// \author Joe Siltberg
///
/// $Date: 2013-10-10 10:20:33 +0200 (Thu, 10 Oct 2013) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_GLOBALCO2FILE_H
#define LPJ_GUESS_GLOBALCO2FILE_H

#include <vector>

/// Reads in and stores CO2 values from a text file
class GlobalCO2File {
public:
	 GlobalCO2File();

	 /// Loads CO2 values from text file
	 /** The text file specifies CO2 values for historical period, each line in
	  *  the format <year> <co2-value>. The years can be for any period but all
	  *  years in the period must be specified.
	  */
	 void load_file(const char* path);

	 /// Returns the CO2 value for a given year
	 /** When accessing co2 values outside of the period specified in the file,
	  *  the class will return the co2 value for the first or the last year
	  *  specified.
	  *
	  *  \param year The historical year for which to get the CO2 value
	  */
	 double operator[](int year) const;

private:

	 /// The co2 values from file
	 std::vector<double> co2;

	 /// The first historical year
	 int first_year;
};

#endif // LPJ_GUESS_GLOBALCO2FILE_H
