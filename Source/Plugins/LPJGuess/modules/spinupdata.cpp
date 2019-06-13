///////////////////////////////////////////////////////////////////////////////////////
/// \file spinupdata.cpp
/// \brief Management of climate data for spinup
///
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "spinupdata.h"
#include "shell.h"

using std::vector;

GenericSpinupData::GenericSpinupData()
	: thisyear(0) {
}

void GenericSpinupData::get_data_from(RawData& source) {
	data = source;

	if (source.empty()) {
		fail("No source data given to GenericSpinupData::get_data_from()");
	}

	for (size_t i = 0; i < source.size(); ++i) {
		if ((source[i].size() != 12 &&
		     source[i].size() % DAYS_PER_YEAR != 0) ||
		    source[i].empty()) {
			fail("Incorrect number of timesteps per year in data given to\n"\
			     "GenericSpinupData (got %d)", source[i].size());
		}

		if (i > 0 && source[i].size() != source[i-1].size()) {
			fail("Different number of timesteps in different years in\n"\
			     "data given to GenericSpinupData::get_data_from()");
		}
	}
	
	firstyear();
}

double GenericSpinupData::operator[](int ts) const {
	if (ts < 0 || ts >= int(data[thisyear].size())) {
		fail("Trying to access data for timestep %d during spinup\n"\
		     "(should be 0-%d)", ts, data[thisyear].size());
	}
	return data[thisyear][ts];
}

void GenericSpinupData::nextyear() {
	thisyear = (thisyear + 1) % nbr_years();
}

void GenericSpinupData::firstyear() {
	thisyear = 0;
}

void GenericSpinupData::detrend_data() {

	vector<double> annual_mean(nbr_years(), 0);
	vector<double> year_number(nbr_years());

	for (size_t y = 0; y < nbr_years(); ++y) {
		for (size_t d = 0; d < data[y].size(); ++d) {
			annual_mean[y] += data[y][d];
		}
		annual_mean[y] /= data[y].size();
		year_number[y] = y;
	}

	double a, b;
	regress(&year_number.front(), &annual_mean.front(), (int) nbr_years(), a, b);

	for (size_t y = 0; y < nbr_years(); ++y) {
		double anomaly = b*y;
		for (size_t d = 0; d < data[y].size(); ++d) {
			data[y][d] -= anomaly;
		}
	}
}

size_t GenericSpinupData::nbr_years() const {
	return data.size();
}
