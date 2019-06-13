///////////////////////////////////////////////////////////////////////////////////////
/// \file spinupdata.h
/// \brief Management of climate data for spinup
///
/// $Date: 2015-11-13 16:25:45 +0100 (Fri, 13 Nov 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_SPINUP_DATA_H
#define LPJ_GUESS_SPINUP_DATA_H

#include "guessmath.h"

class Spinup_data {

	// Class for management of climate data for spinup
	// (derived from first few years of historical climate data)

private:
	int nyear;
	int thisyear;
	double* data;
	bool havedata;

	// guess2008 - this array holds the climatology for the spinup period
	double dataclim[12];


public:
	Spinup_data(int nyear_loc) {
		nyear=nyear_loc;
		havedata=false;
		data=new double[nyear*12];
		thisyear=0;
		havedata=true;
		reset_clim(); // guess2008
	}

	~Spinup_data() {
		if (havedata) delete[] data;
	}

	double& operator[](int month) {

		return data[thisyear*12+month];
	}

	void nextyear() {
		if (thisyear==nyear-1) thisyear=0;
		else thisyear++;
	}

	void firstyear() {
		thisyear=0;
	}

	void get_data_from(double source[][12]) {
		
		int y,m;
		thisyear=0;
		for (y=0;y<nyear;y++) {
			for (m=0;m<12;m++) {
				data[y*12+m]=source[y][m];
			}
		}
	}

	// guess2008 - NEW METHODS 

	void reset_clim() {
		for (int ii = 0; ii < 12; ii++) dataclim[ii] = 0.0;
	}


	void make_clim() {
		
		reset_clim(); // Always reset before calculating

		int y,m;
		for (y=0;y<nyear;y++) {
			for (m=0;m<12;m++) {
				dataclim[m] += data[y*12+m] / (double)nyear;
			}
		}
	}


	bool extract_data(double source[][12], const int& startyear, const int& endyear) {
		
		// Populate data with data from the middle of source. 
		// Condition: endyear - startyear + 1 == nyear
		// if startyear == 1 and endyear == 30 then this function is identical to get_data_from above.

		if (endyear < startyear) return false;
		if (endyear - startyear + 1 == nyear) {

			int y,m;
			for (y=startyear-1;y<endyear;y++) {
				for (m=0;m<12;m++) {
					data[(y-(startyear-1))*12+m]=source[y][m];
				}
			}

		} else return false;

		return true;
	}


	void adjust_data(double anom[12], bool additive) {
		
		// Adjust the spinup data to the conditions prevailing at a particular time, as given by 
		// the (additive or multiplicative) anomalies in anom 
		int y,m;
		for (y=0;y<nyear;y++) {
			for (m=0;m<12;m++) {
				if (additive)	
					data[y*12+m] += anom[m];
				else
					data[y*12+m] *= anom[m];
			}
		}

	}
	
	
	// Replace interannual data with the period's climatology.
	void use_clim_data() {
	
		int y,m;
		for (y=0;y<nyear;y++) {
			for (m=0;m<12;m++) {
				data[y*12+m] = dataclim[m];
			}
		}
	}


	// Alter variability about the mean climatology
	void adjust_data_variability(const double& factor) {
	
		// factor == 0 gives us the climatology (i.e. generalises use_clim_data above)
		// factor == 1 leaves everything unchanged
		// Remember to check the for negative precip or cloudiness values etc. 
		// after calling this method.

		if (factor == 1.0) return;

		int y,m;
		for (y=0;y<nyear;y++) {
			for (m=0;m<12;m++) {
				data[y*12+m] = dataclim[m] + (data[y*12+m] - dataclim[m]) * factor;
			}
		}
	}


	void limit_data(double minval, double maxval) {

		// Limit data to a range
		int y,m;
		for (y=0;y<nyear;y++) {
			for (m=0;m<12;m++) {
				if (data[y*12+m] < minval) data[y*12+m] = minval;
				if (data[y*12+m] > maxval) data[y*12+m] = maxval;
			}
		}

	}
	
	
	void set_min_val(const double& oldval, const double& newval) {

		// Change values < oldval to newval
		int y,m;
		for (y=0;y<nyear;y++) {
			for (m=0;m<12;m++) {
				if (data[y*12+m] < oldval) data[y*12+m] = newval;
			}
		}

	}

	// guess2008 - END OF NEW METHODS


	void detrend_data(bool future = false) {

		int y,m;
		double a,b,anomaly;
		double* annual_mean=new double[nyear];
		double* year_number=new double[nyear];

		for (y=0;y<nyear;y++) {
			annual_mean[y]=0.0;
			for (m=0;m<12;m++) annual_mean[y]+=data[y*12+m];
			annual_mean[y]/=12.0;
			year_number[y]=y;
		}

		regress(year_number,annual_mean,nyear,a,b);

		for (y=0;y<nyear;y++) {
			if(future)
				anomaly = b * (double)(y - (nyear - 1));
			else
				anomaly=b*(double)y;
			for (m=0;m<12;m++)
				data[y*12+m]-=anomaly;
		}
		
		// guess2008 - added [] - Clean up
		delete[] annual_mean;
		delete[] year_number;
	}
};


/// Spinup data container supporting monthly, daily and sub-daily forcing
/** Similar to Spinup_data, but forcing data can have 12, 365 or a multiple
 *  of 365 values per year.
 *
 *  There's no support for leap years, get rid of leap days before sending
 *  the raw forcing data to this class.
 */
class GenericSpinupData {
public:
	static const int DAYS_PER_YEAR = 365;

	/// Datatype for the data, a 2D matrix of doubles
	typedef std::vector<std::vector<double> > RawData;

	GenericSpinupData();

	/// Loads the underlying forcing data (and sets the "current" year to 0)
	void get_data_from(RawData& source);

	/// Gets the value for a given timestep in the "current" year
	double operator[](int ts) const;

	/// Goes to the next year
	void nextyear();

	/// Goes to the first year
	void firstyear();

	/// Removes trend from the original data
	void detrend_data();

	/// Returns the number of years used to construct the spinup dataset
	size_t nbr_years() const;

private:
	/// The "current" year
	int thisyear;

	/// The forcing data which is used over and over during the spinup
	RawData data;
};

#endif // LPJ_GUESS_SPINUP_DATA_H
