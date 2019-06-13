///////////////////////////////////////////////////////////////////////////////////////
/// \file q10.h
/// \brief Q10 calculations for photosynthesis
///
/// Calculations of Q10 values for photosynthesis, formerly
/// placed in canexch.cpp now moved to separate header file
/// because of their application for BVOC calculations as well.
///
/// \author Guy Schurgers (based on LPJ-GUESS 2.1 / Ben Smith)
/// $Date: 2012-07-11 18:10:03 +0200 (Wed, 11 Jul 2012) $
///
///////////////////////////////////////////////////////////////////////////////////////

// WHAT SHOULD THIS FILE CONTAIN?
// Module header files need normally contain only declarations of functions
// defined in the module that are to be accessible to the calling framework or
// to other modules.

#ifndef LPJ_GUESS_Q10_H
#define LPJ_GUESS_Q10_H

#include "guessmath.h"
#include <vector>

// Constants required for Q10 lookup tables used by photosynthesis
const double Q10_MINTEMP = -70;	// minimum temperature ever (deg C)
const double Q10_MAXTEMP = 70;	// maximum temperature ever (deg C)
const double Q10_PRECISION = 0.01;	// rounding precision for temperature
const int Q10_NDATA = static_cast<int>((Q10_MAXTEMP-Q10_MINTEMP)/Q10_PRECISION + 1.5);
	// maximum number of values to store in each lookup table

/// Q10 lookup table class
/** Stores pre-calculated temperature-adjusted values based on Q10 and
 *  a 25-degree base value.
 */
class LookupQ10 {

private:
	/// The temperature-adjusted values
	std::vector<double> data;

public:
	/// Creates a lookup table
	/** \param q10    The Q10 to be used for the table
	 *  \param base25 Base value for 25 degrees C
	 */
	LookupQ10(double q10, double base25) : data(Q10_NDATA) {

		for (int i=0; i<Q10_NDATA; i++) {
			data[i] = base25 * pow(q10, (Q10_MINTEMP + i*Q10_PRECISION - 25.0) / 10.0);
		}
	}

	/// "Array element" operator
	/** \param temp  Temperature (deg C)
	 *  \returns     Temperature-adjusted value based on Q10 and 25-degree base value 
	 */
	double& operator[](double& temp) {
		// Element number corresponding to a particular temperature
		if (temp < Q10_MINTEMP) {
			temp = Q10_MINTEMP;
		} else if (temp > Q10_MAXTEMP) {
			temp = Q10_MAXTEMP;
		}

		int i = static_cast<int>((temp-Q10_MINTEMP)/Q10_PRECISION+0.5);

		return data[i];
	}

};

#endif // LPJ_GUESS_Q10_H
