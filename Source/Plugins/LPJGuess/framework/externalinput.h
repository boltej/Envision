////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file externalinput.h
/// \brief Input code for land cover, management and other data from text files.
/// \author Mats Lindeskog
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_EXTERNALINPUT_H
#define LPJ_GUESS_EXTERNALINPUT_H

#include "indata.h"

using namespace InData;

/// Reads gridlist in lon-lat-description format from text input file
void read_gridlist(ListArray_id<Coord>& gridlist, const char* file_gridlist);

/// Class that deals with all land cover input from text files
class LandcoverInput {

public:

	/// Constructor
	LandcoverInput();

	/// Opens land cover input files
	void init();

	/// Loads land cover and stand type area fractions from input files
	bool loadlandcover(double lon, double lat);

	/// Gets land cover and stand type fractions for a year.
	/** Updates landcover and stand type variables frac, frac_old and frac_change
	 *  Area fractions are re-scaled if sum is not 1.0
	 */ 
	void getlandcover(Gridcell& gridcell);

	/// Gets crop stand type fractions for a year, called from getlandcover() 
	double get_crop_fractions(Gridcell& gridcell, int year, TimeDataD& CFTdata);

	/// Gets land cover or stand type transitions for a year
	bool get_land_transitions(Gridcell& gridcell);

	/// Gets land cover transitions for a year
	/** Updates landcover frac_transfer array
	 *  Transition values are checked against net lcc fractions and 
	 *  rescaled if necessary. 
	 */ 
	bool get_lc_transfer(Gridcell& gridcell);

	/// Gets first historic year of net land cover fraction input data
	int getfirsthistyear();

private:

	// Objects handling land cover fraction data input
	InData::TimeDataD LUdata;
	InData::TimeDataD Peatdata;
	InData::TimeDataD grossLUC;
	InData::TimeDataD st_data[NLANDCOVERTYPES];

	/// Files names for land cover fraction input files
	xtring file_lu, file_grossLUC, file_peat;
	xtring file_lu_st[NLANDCOVERTYPES];

	/// Whether pfts not in crop fraction input file are removed from pftlist (0,1)
	bool minimizecftlist;

	/// Number of years to increase cropland fraction linearly from 0 to first year's value
	int nyears_cropland_ramp;

	/// whether to use stand types with suitable rainfed crops (based on crop pft tb and gridcell latitude) when using fixed crop fractions
	bool frac_fixed_default_crops;
};

/// Class that deals with all crop management input from text files
class ManagementInput {

public:

	/// Constructor
	ManagementInput();
	/// Opens management data files
	void init();
	/// Loads fertilisation, sowing and harvest dates from input files
	bool loadmanagement(double lon, double lat);
	/// Gets management data for a year
	void getmanagement(Gridcell& gridcell);

private:

	/// Input objects for each management text input file
	InData::TimeDataD sdates;
	InData::TimeDataD hdates;
	InData::TimeDataD Nfert;
	InData::TimeDataD Nfert_st;

	/// Files names for management input file
	xtring file_sdates, file_hdates, file_Nfert, file_Nfert_st;

	/// Gets sowing date data for a year
	void getsowingdates(Gridcell& gridcell);
	/// Gets harvest date data for a year
	void getharvestdates(Gridcell& gridcell);
	/// Gets nitrogen fertilisation data for a year
	void getNfert(Gridcell& gridcell);
};

#endif // LPJ_GUESS_EXTERNALINPUT_H