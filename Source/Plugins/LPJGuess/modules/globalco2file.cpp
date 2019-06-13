///////////////////////////////////////////////////////////////////////////////////////
/// \file globalco2file.cpp
/// \brief A class for reading in CO2 values from a text file
///
/// \author Joe Siltberg
/// $Date: 2014-06-23 15:50:25 +0200 (Mon, 23 Jun 2014) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "globalco2file.h"

#include "guess.h"
#include <stdio.h>
#include <limits>
#include "gutil.h"

namespace {

// Constant to indicate that the file hasn't been properly loaded
const int BAD_YEAR = std::numeric_limits<int>::min();

}

GlobalCO2File::GlobalCO2File()
		  : first_year(BAD_YEAR) {
}

void GlobalCO2File::load_file(const char* path) {
	 // Reads in atmospheric CO2 concentrations for historical period
	 // from ascii text file with records in format: <year> <co2-value>
	 
	 
	 // Have we already loaded a file?
	 if (first_year != BAD_YEAR) {
		  // Go back to initial state
		  first_year = BAD_YEAR;
		  co2.clear();
	 }
	 
	 int calender_year;
	 double next_co2;
	 
	 FILE* in = fopen(path, "rt");
	 if (!in) {
		  fail("GlobalCO2File::load_file: could not open CO2 file %s for input", 
				 path);
	 }

	 // Read in the values from the file
	 while (readfor(in,"i,f",&calender_year,&next_co2)) {
		  if (first_year == BAD_YEAR) {
				first_year = calender_year;
		  }
		  else if (calender_year != first_year+int(co2.size())) {
				fail("GlobalCO2File::load_file: %s, line %d - bad year specified",
					  path,co2.size()+1);
		  }
		  co2.push_back(next_co2);
	 }

	 if (first_year == BAD_YEAR) {
		  fail("GlobalCO2File::load_file: failed to load from file %s", path);
	 }
	 
	 fclose(in);
}

double GlobalCO2File::operator[](int year) const {
	 if (first_year == BAD_YEAR) {
		  fail("GlobalCO2File::operator[]: "\
				 "Tried to get CO2 value before loading from file!");
	 }

	 if (year < first_year) {
		  return co2.front();
	 }
	 else if (year >= first_year+static_cast<int>(co2.size())) {
		  fail("GlobalCO2File::operator[]: "\
				 "Tried to get CO2 value after last year in file\n"\
				 "Last year: %d, tried to get CO2 for: %d",
				 first_year+co2.size()-1, year);

		  return 0.0; // to avoid compiler warning
	 }
	 else {
		  return co2[year-first_year];
	 }
}
