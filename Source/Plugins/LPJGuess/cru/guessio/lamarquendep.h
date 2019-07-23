///////////////////////////////////////////////////////////////////////////////////////
/// \file lamarquendep.h
/// \brief Functionality for reading the Lamarque Nitrogen deposition data set
///
/// $Date: 2017-04-05 15:04:09 +0200 (Wed, 05 Apr 2017) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_LAMARQUENDEP_H
#define LPJ_GUESS_LAMARQUENDEP_H

#include <string>

namespace Lamarque {

/// number of years of historical nitrogen deposition 
/** One year from each decade 1850-2009 */
const int NYEAR_HISTNDEP = 16;

/// number of years of scenario nitrogen deposition
/** One year from each decade 2000-2109 */
const int NYEAR_SCENNDEP = 11;

/// total number of years of nitrogen deposition 
/** historic and scenario overlap one decade */
const int NYEAR_TOTNDEP = NYEAR_HISTNDEP + NYEAR_SCENNDEP - 1;

/// calendar year corresponding to first year nitrogen deposition
const int FIRSTHISTYEARNDEP=1850;

/// Type of time series to use (historic/scenario/fixed)
enum timeseriestype { 
	/// Only the historic (1850-2009) data set
	HISTORIC,
	/// Historic + RCP 2.6
	RCP26,
	/// Historic + RCP 4.5
	RCP45,
	/// Historic + RCP 6.0
	RCP60,
	/// Historic + RCP 8.5
	RCP85,
	/// Fixed pre-industrial values
	FIXED,
};

/// Converts a string ("historic", "rcp26" etc.) to a timeseriestype
/** Case insensitive */
timeseriestype parse_timeseries(const std::string& str);

/// Nitrogen deposition forcing for a single grid cell
class NDepData {
public:

	/// Default constructor
	/** Before getndep is called the data will all be set to
	 *  pre-industrial level (same as calling getndep with an empty filename).
	 */
	NDepData();

	/// Retrieves nitrogen deposition for a particular gridcell
	/** The values are either taken from a binary archive file or when it's not
	 *  provided default to pre-industrial level of 2 kgN/ha/year.
	 *
	 *  The binary archive files have nitrogen deposition in gN/m2 on a monthly timestep
	 *  for 16 years with 10 year interval starting from 1850 (Lamarque et. al., 2011).
	 *
	 *  \param  file_ndep   Path to binary archive (empty gives pre-industrial values)
	 *  \param  lon         Longitude
	 *  \param  lat         Latitude
	 *  \param  timeseries  Which time series to use
	 */
	void getndep(const char* file_ndep,
	             double lon, double lat,
	             timeseriestype timeseries = HISTORIC);

	/// Returns nitrogen deposition for one year
	/** Given a calendar year, this function chooses values from the correct 
	 *  10 year interval, and sums the different types of wet and dry 
	 *  deposition.
	 *
	 *  If calendar_year is earlier than the first year in data set (1850),
	 *  the values for the first year will be used. If later than the last
	 *  year, fail() is called and the program terminated.
	 *
	 *  \param calendar_year The year for which to get ndep data
	 *  \param mndrydep      Monthly values for dry nitrogen deposition (kgN/m2/day)
	 *  \param mnwetdep      Monthly values for wet nitrogen deposition (kgN/m2/day)
	 */
	void get_one_calendar_year(int calendar_year,
		double mndrydep[12],
		double mnwetdep[12]);

private:

	/// Fills all arrays with pre-industrial level of 2 kgN/ha/year
	void set_to_pre_industrial();

	/// Currently chosen type of time series
	timeseriestype timeseries;

	/// Monthly data on daily dry NHx deposition (kgN/m2/day)
	double NHxDryDep[NYEAR_TOTNDEP][12];

	/// Monthly data on daily wet NHx deposition (kgN/m2/day)
	double NHxWetDep[NYEAR_TOTNDEP][12];

	/// Monthly data on daily dry NOy deposition (kgN/m2/day)
	double NOyDryDep[NYEAR_TOTNDEP][12];

	/// Monthly data on daily wet NOy deposition (kgN/m2/day)
	double NOyWetDep[NYEAR_TOTNDEP][12];
};


}

#endif // LPJ_GUESS_LAMARQUENDEP_H
