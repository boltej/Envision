///////////////////////////////////////////////////////////////////////////////////////
/// \file outputmodule.cpp
/// \brief Implementation of the common output module
///
/// \author Joe Siltberg
/// $Date: 2015-04-09 18:40:34 +0200 (Thu, 09 Apr 2015) $
///
///////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "miscoutput.h"
#include "parameters.h"
#include "guess.h"

namespace GuessOutput {

// Nitrogen output is in kgN/ha instead of kgC/m2 as for carbon
const double m2toha = 10000.0;

REGISTER_OUTPUT_MODULE("misc", MiscOutput)

MiscOutput::MiscOutput() {
	// Annual output variables
	declare_parameter("file_cmass_cropland", &file_cmass_cropland, 300, "Annual cropland cmass output file");
	declare_parameter("file_cmass_pasture", &file_cmass_pasture, 300, "Annual pasture cmass output file");
	declare_parameter("file_cmass_natural", &file_cmass_natural, 300, "Annual natural vegetation cmass output file");
	declare_parameter("file_cmass_forest", &file_cmass_forest, 300, "Annual managed forest cmass output file");
	declare_parameter("file_anpp_cropland", &file_anpp_cropland, 300, "Annual cropland NPP output file");
	declare_parameter("file_anpp_pasture", &file_anpp_pasture, 300, "Annual pasture NPP output file");
	declare_parameter("file_anpp_natural", &file_anpp_natural, 300, "Annual natural vegetation NPP output file");
	declare_parameter("file_anpp_forest", &file_anpp_forest, 300, "Annual managed forest NPP output file");
	declare_parameter("file_yield",&file_yield,300, "Crop yield output file");
	declare_parameter("file_yield1",&file_yield1,300,"Crop first yield output file");
	declare_parameter("file_yield2",&file_yield2,300,"Crop second yield output file");
	declare_parameter("file_sdate1",&file_sdate1,300,"Crop first sowing date output file");
	declare_parameter("file_sdate2",&file_sdate2,300,"Crop second sowing date output file");
	declare_parameter("file_hdate1",&file_hdate1,300,"Crop first harvest date output file");
	declare_parameter("file_hdate2",&file_hdate2,300,"Crop second harvest date output file");
	declare_parameter("file_lgp",&file_lgp,300,"Crop length of growing period output file");
	declare_parameter("file_phu",&file_phu,300,"Crop potential heat units output file");
	declare_parameter("file_fphu",&file_fphu,300,"Crop attained fraction of potential heat units output file");
	declare_parameter("file_fhi",&file_fhi,300,"Crop attained fraction of harvest index output file");
	declare_parameter("file_irrigation",&file_irrigation,300,"Crop irrigation output file");
	declare_parameter("file_seasonality",&file_seasonality,300,"Seasonality output file");
	declare_parameter("file_cflux_cropland", &file_cflux_cropland, 300, "C fluxes output file");
	declare_parameter("file_cflux_pasture", &file_cflux_pasture, 300, "C fluxes output file");
	declare_parameter("file_cflux_natural", &file_cflux_natural, 300, "C fluxes output file");
	declare_parameter("file_cflux_forest", &file_cflux_forest, 300, "C fluxes output file");
	declare_parameter("file_dens_natural", &file_dens_natural, 300, "Natural vegetation tree density output file");
	declare_parameter("file_dens_forest", &file_dens_forest, 300, "Managed forest tree density output file");
	declare_parameter("file_cpool_cropland", &file_cpool_cropland, 300, "Soil C output file");
	declare_parameter("file_cpool_pasture", &file_cpool_pasture, 300, "Soil C output file");
	declare_parameter("file_cpool_natural", &file_cpool_natural, 300, "Soil C output file");
	declare_parameter("file_cpool_forest", &file_cpool_forest, 300, "Soil C output file");
	declare_parameter("file_nflux_cropland", &file_nflux_cropland, 300, "N fluxes output file");
	declare_parameter("file_nflux_pasture", &file_nflux_pasture, 300, "N fluxes output file");
	declare_parameter("file_nflux_natural", &file_nflux_natural, 300, "N fluxes output file");
	declare_parameter("file_nflux_forest", &file_nflux_forest, 300, "N fluxes output file");
	declare_parameter("file_npool_cropland", &file_npool_cropland, 300, "Soil N output file");
	declare_parameter("file_npool_pasture", &file_npool_pasture, 300, "Soil N output file");
	declare_parameter("file_npool_natural", &file_npool_natural, 300, "Soil N output file");
	declare_parameter("file_npool_forest", &file_npool_forest, 300, "Soil N output file");

	//daily
	declare_parameter("file_daily_lai",&file_daily_lai,300,"Daily output.");
	declare_parameter("file_daily_npp",&file_daily_npp,300,"Daily output.");
	declare_parameter("file_daily_nmass",&file_daily_nmass,300,"Daily output.");
	declare_parameter("file_daily_ndemand",&file_daily_ndemand,300,"Daily output.");
	declare_parameter("file_daily_cmass",&file_daily_cmass,300,"Daily output.");
	declare_parameter("file_daily_cton",&file_daily_cton,300,"Daily output.");
	declare_parameter("file_daily_cmass_leaf",&file_daily_cmass_leaf,300,"Daily output.");
	declare_parameter("file_daily_nmass_leaf",&file_daily_nmass_leaf,300,"Daily output.");
	declare_parameter("file_daily_cmass_root",&file_daily_cmass_root,300,"Daily output.");
	declare_parameter("file_daily_nmass_root",&file_daily_nmass_root,300,"Daily output.");
	declare_parameter("file_daily_cmass_stem",&file_daily_cmass_stem,300,"Daily output.");
	declare_parameter("file_daily_nmass_stem",&file_daily_nmass_stem,300,"Daily output.");
	declare_parameter("file_daily_cmass_storage",&file_daily_cmass_storage,300,"Daily output.");
	declare_parameter("file_daily_nmass_storage",&file_daily_nmass_storage,300,"Daily output.");

	declare_parameter("file_daily_cmass_dead_leaf",&file_daily_cmass_dead_leaf,300,"Daily output.");
	declare_parameter("file_daily_nmass_dead_leaf",&file_daily_nmass_dead_leaf,300,"Daily output.");

	declare_parameter("file_daily_n_input_soil",&file_daily_n_input_soil,300,"Daily output.");
	declare_parameter("file_daily_avail_nmass_soil",&file_daily_avail_nmass_soil,300,"Daily output.");
	declare_parameter("file_daily_upper_wcont",&file_daily_upper_wcont,300,"Daily output.");
	declare_parameter("file_daily_lower_wcont",&file_daily_lower_wcont,300,"Daily output.");
	declare_parameter("file_daily_irrigation",&file_daily_irrigation,300,"Daily output.");

	declare_parameter("file_daily_nminleach",&file_daily_nminleach,300,"Daily output.");
	declare_parameter("file_daily_norgleach",&file_daily_norgleach,300,"Daily output.");
	declare_parameter("file_daily_nuptake",&file_daily_nuptake,300,"Daily output.");

	declare_parameter("file_daily_temp",&file_daily_temp,300,"Daily output.");
	declare_parameter("file_daily_prec",&file_daily_prec,300,"Daily output.");
	declare_parameter("file_daily_rad",&file_daily_rad,300,"Daily output.");

	declare_parameter("file_daily_fphu",&file_daily_fphu,300,"Daily DS output file"); //daglig ds

	if (ifnlim) {
		declare_parameter("file_daily_ds",&file_daily_ds,300,"Daily DS output file"); //daglig ds
		declare_parameter("file_daily_stem",&file_daily_stem,300,"Daily stem allocation output file");
		declare_parameter("file_daily_leaf",&file_daily_leaf,300,"Daily leaf allocation output file");
		declare_parameter("file_daily_root",&file_daily_root,300,"Daily root allocation output file");
		declare_parameter("file_daily_storage",&file_daily_storage,300,"Daily storage allocation output file");
	}
}

MiscOutput::~MiscOutput() {
}

/// Define all output tables and their formats
void MiscOutput::init() {
	
	define_output_tables();
}

/// Specify all columns in all output tables
/** This function specifies all columns in all output tables, their names,
 *  column widths and precision.
 *
 *  For each table a TableDescriptor object is created which is then sent to
 *  the output channel to create the table.
 */
void MiscOutput::define_output_tables() {
	// create a vector with the pft names
	std::vector<std::string> pfts;

	// create a vector with the crop pft names
	std::vector<std::string> crop_pfts;

	pftlist.firstobj();
	while (pftlist.isobj) {
		Pft& pft=pftlist.getobj();

		pfts.push_back((char*)pft.name);

		if (pft.landcover==CROPLAND)
			crop_pfts.push_back((char*)pft.name);

		pftlist.nextobj();
	}

	// create a vector with the landcover column titles
	std::vector<std::string> landcovers;

	const char* landcover_string[]={"Urban_sum", "Crop_sum", "Pasture_sum",
			"Forest_sum", "Natural_sum", "Peatland_sum", "Barren_sum"};
	for (int i=0; i<NLANDCOVERTYPES; i++) {
		if (run[i]) {
			landcovers.push_back(landcover_string[i]);
		}
	}

	// Create the month columns
	ColumnDescriptors month_columns;
	ColumnDescriptors month_columns_wide;
	xtring months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	for (int i = 0; i < 12; i++) {
		month_columns      += ColumnDescriptor(months[i], 8,  3);
		month_columns_wide += ColumnDescriptor(months[i], 10, 3);
	}

	// Create the columns for each output file

	// CMASS
	ColumnDescriptors cmass_columns;
	cmass_columns += ColumnDescriptors(pfts,               8, 3);
	cmass_columns += ColumnDescriptor("Total",             8, 3);
	ColumnDescriptors cmass_columns_lc = cmass_columns;
	cmass_columns += ColumnDescriptors(landcovers,        13, 3);

	// ANPP
	ColumnDescriptors anpp_columns = cmass_columns;
	ColumnDescriptors anpp_columns_lc = cmass_columns_lc;

	// DENS
	ColumnDescriptors dens_columns;
	dens_columns += ColumnDescriptors(pfts,                8, 4);
	dens_columns += ColumnDescriptor("Total",              8, 4);
	ColumnDescriptors dens_columns_lc = dens_columns;

	// CFLUX
	ColumnDescriptors cflux_columns;
	cflux_columns += ColumnDescriptor("Veg",               8, 3);
	cflux_columns += ColumnDescriptor("Repr",              8, 3);
	cflux_columns += ColumnDescriptor("Soil",              8, 3);
	cflux_columns += ColumnDescriptor("Fire",              8, 3);
	cflux_columns += ColumnDescriptor("Est",               8, 3);
	cflux_columns += ColumnDescriptor("Seed",         8, 3);
	cflux_columns += ColumnDescriptor("Harvest",      9, 3);
	cflux_columns += ColumnDescriptor("LU_ch",        9, 3);
	cflux_columns += ColumnDescriptor("Slow_h",       9, 3);
	cflux_columns += ColumnDescriptor("NEE",              10, 5);

	// CPOOL
	ColumnDescriptors cpool_columns;
	cpool_columns += ColumnDescriptor("VegC",              8, 3);

	if (!ifcentury) {
		cpool_columns += ColumnDescriptor("LittC",         8, 3);
		cpool_columns += ColumnDescriptor("SoilfC",        8, 3);
		cpool_columns += ColumnDescriptor("SoilsC",        8, 3);
	}
	else {
		cpool_columns += ColumnDescriptor("LitterC",       8, 3);
		cpool_columns += ColumnDescriptor("SoilC",         8, 3);
	}
	if (run_landcover && ifslowharvestpool) {
		 cpool_columns += ColumnDescriptor("HarvSlowC",   10, 3);
	}
	cpool_columns += ColumnDescriptor("Total",            10, 3);

	//CROP YIELD
	ColumnDescriptors crop_columns;
	crop_columns += ColumnDescriptors(crop_pfts,           8, 3);

	//CROP SDATE & HDATE
	ColumnDescriptors date_columns;
	date_columns += ColumnDescriptors(crop_pfts,           8, 0);

	//IRRIGATION
	ColumnDescriptors irrigation_columns;
	irrigation_columns += ColumnDescriptor("Total",       10, 3);

	//SEASONALITY
	ColumnDescriptors seasonality_columns;
	seasonality_columns += ColumnDescriptor("Seasonal",   10, 0);
	seasonality_columns += ColumnDescriptor("V_temp",     10, 3);
	seasonality_columns += ColumnDescriptor("V_prec",     10, 3);
	seasonality_columns += ColumnDescriptor("temp_min",   10, 1);
	seasonality_columns += ColumnDescriptor("temp_mean",  10, 1);
	seasonality_columns += ColumnDescriptor("temp_seas",  10, 0);
	seasonality_columns += ColumnDescriptor("prec_min",   10, 2);
	seasonality_columns += ColumnDescriptor("prec",       10, 1);
	seasonality_columns += ColumnDescriptor("prec_range", 12, 0);

	// NPOOL
	ColumnDescriptors npool_columns;
	npool_columns += ColumnDescriptor("VegN",              9, 4);
	npool_columns += ColumnDescriptor("LitterN",           9, 4);
	npool_columns += ColumnDescriptor("SoilN",             9, 4);

	if (run_landcover && ifslowharvestpool) {
		npool_columns += ColumnDescriptor("HarvSlowN",    10, 4);
	}

	npool_columns += ColumnDescriptor("Total",            10, 4);

	// NFLUX
	ColumnDescriptors nflux_columns;
	nflux_columns += ColumnDescriptor("dep",               8, 2);
	nflux_columns += ColumnDescriptor("fix",               8, 2);
	nflux_columns += ColumnDescriptor("fert",              8, 2);
	nflux_columns += ColumnDescriptor("flux",              8, 2);
	nflux_columns += ColumnDescriptor("leach",             8, 2);
	if (run_landcover) {
		nflux_columns += ColumnDescriptor("seed",         8, 2);
		nflux_columns += ColumnDescriptor("harvest",       8, 2);
		nflux_columns += ColumnDescriptor("LU_ch",         8, 3);
		nflux_columns += ColumnDescriptor("Slow_h",        8, 3);
	}
	nflux_columns += ColumnDescriptor("NEE",               8, 2);

	ColumnDescriptors daily_columns;
	daily_columns += ColumnDescriptors(crop_pfts, 13, 3);

	// *** ANNUAL OUTPUT VARIABLES ***

	create_output_table(out_cmass_cropland, file_cmass_cropland, cmass_columns_lc);
	create_output_table(out_cmass_pasture,  file_cmass_pasture,  cmass_columns_lc);
	create_output_table(out_cmass_natural,  file_cmass_natural,  cmass_columns_lc);
	create_output_table(out_cmass_forest,   file_cmass_forest,   cmass_columns_lc);
	create_output_table(out_anpp_cropland,  file_anpp_cropland,  anpp_columns_lc);
	create_output_table(out_anpp_pasture,   file_anpp_pasture,   anpp_columns_lc);
	create_output_table(out_anpp_natural,   file_anpp_natural,   anpp_columns_lc);
	create_output_table(out_anpp_forest,    file_anpp_forest,    anpp_columns_lc);
	create_output_table(out_dens_natural,   file_dens_natural,   dens_columns_lc);
	create_output_table(out_dens_forest,    file_dens_forest,    dens_columns_lc);
	create_output_table(out_cflux_cropland, file_cflux_cropland, cflux_columns);
	create_output_table(out_cflux_pasture,  file_cflux_pasture,  cflux_columns);
	create_output_table(out_cflux_natural,  file_cflux_natural,  cflux_columns);
	create_output_table(out_cflux_forest,	file_cflux_forest,   cflux_columns);
	create_output_table(out_cpool_cropland, file_cpool_cropland, cpool_columns);
	create_output_table(out_cpool_pasture,  file_cpool_pasture,  cpool_columns);
	create_output_table(out_cpool_natural,  file_cpool_natural,  cpool_columns);
	create_output_table(out_cpool_forest,	file_cpool_forest,   cpool_columns);

	if (run_landcover && run[CROPLAND]) {
		create_output_table(out_yield,      file_yield,          crop_columns);
		create_output_table(out_yield1,     file_yield1,         crop_columns);
		create_output_table(out_yield2,     file_yield2,         crop_columns);
		create_output_table(out_sdate1,     file_sdate1,         date_columns);
		create_output_table(out_sdate2,     file_sdate2,         date_columns);
		create_output_table(out_hdate1,     file_hdate1,         date_columns);
		create_output_table(out_hdate2,     file_hdate2,         date_columns);
		create_output_table(out_lgp,        file_lgp,            date_columns);
		create_output_table(out_phu,        file_phu,            date_columns);
		create_output_table(out_fphu,       file_fphu,           crop_columns);
		create_output_table(out_fhi,        file_fhi,            crop_columns);
        create_output_table(out_seasonality,file_seasonality,    seasonality_columns);
	}

	if(run_landcover)
		create_output_table(out_irrigation, file_irrigation,     irrigation_columns); 

	create_output_table(out_npool_cropland, file_npool_cropland, npool_columns);
	create_output_table(out_npool_pasture,  file_npool_pasture,  npool_columns);
	create_output_table(out_npool_natural,  file_npool_natural,  npool_columns);
	create_output_table(out_npool_forest,	file_npool_forest,   npool_columns);
	create_output_table(out_nflux_cropland, file_nflux_cropland, nflux_columns);
	create_output_table(out_nflux_pasture,  file_nflux_pasture,  nflux_columns);
	create_output_table(out_nflux_natural,  file_nflux_natural,  nflux_columns);
	create_output_table(out_nflux_forest,	file_nflux_forest,   nflux_columns);

	// *** DAILY OUTPUT VARIABLES ***

	create_output_table(out_daily_lai,					file_daily_lai,					daily_columns);
	create_output_table(out_daily_npp,					file_daily_npp,					daily_columns);
	create_output_table(out_daily_ndemand,				file_daily_ndemand,				daily_columns);
	create_output_table(out_daily_nmass,				file_daily_nmass,				daily_columns);
	create_output_table(out_daily_cmass,				file_daily_cmass,				daily_columns);
	create_output_table(out_daily_nmass_leaf,			file_daily_nmass_leaf,			daily_columns);
	create_output_table(out_daily_cmass_leaf,			file_daily_cmass_leaf,			daily_columns);
	create_output_table(out_daily_nmass_root,			file_daily_nmass_root,			daily_columns);
	create_output_table(out_daily_cmass_root,			file_daily_cmass_root,			daily_columns);
	create_output_table(out_daily_nmass_stem,			file_daily_nmass_stem,			daily_columns);
	create_output_table(out_daily_cmass_stem,			file_daily_cmass_stem,			daily_columns);
	create_output_table(out_daily_nmass_storage,        file_daily_nmass_storage,       daily_columns);
	create_output_table(out_daily_cmass_storage,        file_daily_cmass_storage,       daily_columns);
	create_output_table(out_daily_nmass_dead_leaf,      file_daily_nmass_dead_leaf,     daily_columns);
	create_output_table(out_daily_cmass_dead_leaf,      file_daily_cmass_dead_leaf,     daily_columns);
	create_output_table(out_daily_n_input_soil,         file_daily_n_input_soil,        daily_columns);
	create_output_table(out_daily_avail_nmass_soil,     file_daily_avail_nmass_soil,    daily_columns);

	create_output_table(out_daily_upper_wcont,			file_daily_upper_wcont,         daily_columns);
	create_output_table(out_daily_lower_wcont,			file_daily_lower_wcont,         daily_columns);
	create_output_table(out_daily_irrigation,			file_daily_irrigation,			daily_columns);

	create_output_table(out_daily_temp,					file_daily_temp,				daily_columns);
	create_output_table(out_daily_prec,					file_daily_prec,				daily_columns);
	create_output_table(out_daily_rad,					file_daily_rad,					daily_columns);

	create_output_table(out_daily_cton,					file_daily_cton,				daily_columns);

	create_output_table(out_daily_nminleach,			file_daily_nminleach,			daily_columns);
	create_output_table(out_daily_norgleach,			file_daily_norgleach,			daily_columns);
	create_output_table(out_daily_nuptake,				file_daily_nuptake,				daily_columns);

	if (ifnlim) {
		create_output_table(out_daily_ds,				file_daily_ds,					daily_columns);
		create_output_table(out_daily_fphu,				file_daily_fphu,				daily_columns);
		create_output_table(out_daily_stem,				file_daily_stem,				daily_columns);
		create_output_table(out_daily_leaf,				file_daily_leaf,				daily_columns);
		create_output_table(out_daily_root,				file_daily_root,				daily_columns);
		create_output_table(out_daily_storage,			file_daily_storage,				daily_columns);
	}

}

/// Local analogue of OutputRows::add_value for restricting output
/** Use to restrict output to specified range of years
  * (or other user-specified limitation)
  *
  * If only yearly output between, say 1961 and 1990 is requred, use:
  *  if (date.get_calendar_year() >= 1961 && date.get_calendar_year() <= 1990)
  *  (assuming the input module has set the first calendar year in the date object)	
  */
void outlimit_misc(OutputRows& out, const Table& table, double d) {

	if (date.year>=nyear_spinup)
		out.add_value(table, d);
}

/// Output of simulation results at the end of each year
/** Output of simulation results at the end of each year, or for specific years in
  * the simulation of each stand or grid cell. 
  * This function does not have to provide any information to the framework.
  *
  * Restrict output to specific years in the local helper function outlimit_misc().
  *
  * Changes in the structure of CommonOutput::outannual() should be mirrored here.
  */
void MiscOutput::outannual(Gridcell& gridcell) {

	if (date.year < nyear_spinup) {
		if(printseparatestands)
			closelocalfiles(gridcell);
		return;
	}

	if(printseparatestands)
		openlocalfiles(gridcell);

	double lon = gridcell.get_lon();
	double lat = gridcell.get_lat();

	Landcover& lc = gridcell.landcover;
	// The OutputRows object manages the next row of output for each
	// output table
	OutputRows out(output_channel, lon, lat, date.get_calendar_year());

	double landcover_cmass[NLANDCOVERTYPES]={0.0};
	double landcover_nmass[NLANDCOVERTYPES]={0.0};
	double landcover_clitter[NLANDCOVERTYPES]={0.0};
	double landcover_nlitter[NLANDCOVERTYPES]={0.0};
	double landcover_anpp[NLANDCOVERTYPES]={0.0};
	double landcover_densindiv_total[NLANDCOVERTYPES]={0.0};

	double mean_standpft_anpp_lc[NLANDCOVERTYPES]={0.0};
	double mean_standpft_cmass_lc[NLANDCOVERTYPES]={0.0};
	double mean_standpft_densindiv_total_lc[NLANDCOVERTYPES]={0.0};

	double irrigation_gridcell=0.0;

	double standpft_cmass=0.0;
	double standpft_nmass=0.0;
	double standpft_clitter=0.0;
	double standpft_nlitter=0.0;
	double standpft_anpp=0.0;
	double standpft_yield=0.0;
	double standpft_yield1=0.0;
	double standpft_yield2=0.0;
	double standpft_densindiv_total=0.0;

	pftlist.firstobj();
	while (pftlist.isobj) {

		Pft& pft=pftlist.getobj();

		// Sum values across stands, patches and PFTs
		double mean_standpft_yield=0.0;
		double mean_standpft_yield1=0.0;
		double mean_standpft_yield2=0.0;

		for (int i=0; i<NLANDCOVERTYPES; i++) {
			mean_standpft_anpp_lc[i]=0.0;
			mean_standpft_cmass_lc[i]=0.0;
			mean_standpft_densindiv_total_lc[i]=0.0;
		}

		// Determine area fraction of stands where this pft is active:
		double active_fraction = 0.0;
		double active_fraction_lc[NLANDCOVERTYPES]={0.0};

		Gridcell::iterator gc_itr = gridcell.begin();

		while (gc_itr != gridcell.end()) {
			Stand& stand = *gc_itr;

			if (stand.pft[pft.id].active) {
				active_fraction += stand.get_gridcell_fraction();
				active_fraction_lc[stand.landcover] += stand.get_gridcell_fraction();
			}

			++gc_itr;
		}

		// Loop through Stands
		gc_itr = gridcell.begin();

		while (gc_itr != gridcell.end()) {
			Stand& stand = *gc_itr;

			Standpft& standpft=stand.pft[pft.id];
			if (!standpft.active) {
				++gc_itr;
				continue;
			}
			// Sum values across patches and PFTs
			standpft_cmass=0.0;
			standpft_nmass=0.0;
			standpft_clitter=0.0;
			standpft_nlitter=0.0;
			standpft_anpp=0.0;
			standpft_yield=0.0;
			standpft_yield1=0.0;
			standpft_yield2=0.0;
			standpft_densindiv_total = 0.0;

			stand.firstobj();

			// Loop through Patches
			while (stand.isobj) {
				Patch& patch = stand.getobj();
				Patchpft& patchpft = patch.pft[pft.id];
				Vegetation& vegetation = patch.vegetation;

				standpft_anpp += patch.fluxes.get_annual_flux(Fluxes::NPP, pft.id);

				standpft_clitter += patchpft.litter_leaf + patchpft.litter_root + patchpft.litter_sap + patchpft.litter_heart + patchpft.litter_repr;
				standpft_nlitter += patchpft.nmass_litter_leaf + patchpft.nmass_litter_root + patchpft.nmass_litter_sap + patchpft.nmass_litter_heart;

				vegetation.firstobj();
				while (vegetation.isobj) {
					Individual& indiv=vegetation.getobj();

					if (indiv.id!=-1 && indiv.alive && indiv.pft.id==pft.id) {

						standpft_cmass += indiv.ccont();
						standpft_nmass += indiv.ncont();

						if (pft.landcover == CROPLAND) {
							standpft_yield += indiv.cropindiv->harv_yield;
							standpft_yield1 += indiv.cropindiv->yield_harvest[0];
							standpft_yield2 += indiv.cropindiv->yield_harvest[1];
						}
						else {

							if (vegmode==COHORT || vegmode==INDIVIDUAL) {

								// guess2008 - only count trees with a trunk above a certain diameter
								if (pft.lifeform==TREE && indiv.age>0) {
									double diam=pow(indiv.height/indiv.pft.k_allom2,1.0/indiv.pft.k_allom3);
									if (diam>0.03) {
										standpft_densindiv_total+=indiv.densindiv; // indiv/m2
									}
								}
							}
						}

					}
					vegetation.nextobj();
				}

				stand.nextobj();
			} // end of patch loop

			standpft_cmass/=(double)stand.npatch();
			standpft_nmass/=(double)stand.npatch();
			standpft_clitter/=(double)stand.npatch();
			standpft_nlitter/=(double)stand.npatch();
			standpft_anpp/=(double)stand.npatch();
			standpft_densindiv_total/=(double)stand.npatch();

			//Update landcover totals
			landcover_cmass[stand.landcover]+=standpft_cmass*stand.get_landcover_fraction();
			landcover_nmass[stand.landcover]+=standpft_nmass*stand.get_landcover_fraction();
			landcover_clitter[stand.landcover]+=standpft_clitter*stand.get_landcover_fraction();
			landcover_nlitter[stand.landcover]+=standpft_nlitter*stand.get_landcover_fraction();
			landcover_anpp[stand.landcover]+=standpft_anpp*stand.get_landcover_fraction();
			landcover_densindiv_total[stand.landcover]+=standpft_densindiv_total*stand.get_landcover_fraction();

			//Update pft means for active stands
			mean_standpft_yield += standpft_yield * stand.get_gridcell_fraction() / active_fraction;
			mean_standpft_yield1 += standpft_yield1 * stand.get_gridcell_fraction() / active_fraction;
			mean_standpft_yield2 += standpft_yield2 * stand.get_gridcell_fraction() / active_fraction;

			//Update pft mean for active stands in landcover
			mean_standpft_anpp_lc[stand.landcover] += standpft_anpp * stand.get_gridcell_fraction() / active_fraction_lc[stand.landcover];
			mean_standpft_cmass_lc[stand.landcover] += standpft_cmass * stand.get_gridcell_fraction() / active_fraction_lc[stand.landcover];
			mean_standpft_densindiv_total_lc[stand.landcover] += standpft_densindiv_total * stand.get_gridcell_fraction() / active_fraction_lc[stand.landcover];

			//Update stand totals
			stand.anpp += standpft_anpp;
			stand.cmass += standpft_cmass;

			// Print per-stand pft values
			if (printseparatestands) {

				int id = stand.id;
				if(stand.id >= MAXNUMBER_STANDS)
					fail("Number of stands to high, increase MAXNUMBER_STANDS for output of individual stands !\n");

				if (stand.landcover == NATURAL) {

					if (!out_anpp_stand_natural[id].invalid())
						outlimit_misc(out, out_anpp_stand_natural[id],      standpft_anpp);
					if (!out_cmass_stand_natural[id].invalid())
						outlimit_misc(out, out_cmass_stand_natural[id],      standpft_cmass);
				}
				else if (stand.landcover == FOREST) {

					if (!out_anpp_stand_forest[id].invalid())
						outlimit_misc(out, out_anpp_stand_forest[id],      standpft_anpp);
					if (!out_cmass_stand_forest[id].invalid())
						outlimit_misc(out, out_cmass_stand_forest[id],      standpft_cmass);
				}
			}

			++gc_itr;
		}//End of loop through stands

		// Print to landcover files in case pft:s are common to several landcovers (currently only used in NATURAL and FOREST)
		if (run_landcover) {
			for (int i=0;i<NLANDCOVERTYPES;i++) {
				if (run[i]) {

					switch (i) {
					case CROPLAND:
						break;
					case PASTURE:
						if (run[NATURAL]) {
							outlimit_misc(out, out_anpp_pasture,			mean_standpft_anpp_lc[i]);
							outlimit_misc(out, out_cmass_pasture,		mean_standpft_cmass_lc[i]);
						}
						break;
					case BARREN:
						break;
					case NATURAL:
//						if(run[FOREST] || run[PASTURE]) {
						if(run[FOREST]) {
							outlimit_misc(out, out_anpp_natural,			mean_standpft_anpp_lc[i]);
							outlimit_misc(out, out_cmass_natural,		mean_standpft_cmass_lc[i]);
							outlimit_misc(out, out_dens_natural,			mean_standpft_densindiv_total_lc[i]);
						}
						break;
					case FOREST:
						if (run[NATURAL]) {
							outlimit_misc(out, out_anpp_forest,			mean_standpft_anpp_lc[i]);
							outlimit_misc(out, out_cmass_forest,			mean_standpft_cmass_lc[i]);
							outlimit_misc(out, out_dens_forest,			mean_standpft_densindiv_total_lc[i]);
						}
						break;
					case URBAN:
						break;
					case PEATLAND:
						break;
					default:
						if (date.year == nyear_spinup)
							dprintf("Modify code to deal with landcover output!\n");
					}
				}
			}
		}

		if (pft.landcover == CROPLAND) {
			outlimit_misc(out, out_yield,   mean_standpft_yield);
			outlimit_misc(out, out_yield1,  mean_standpft_yield1);
			outlimit_misc(out, out_yield2,  mean_standpft_yield2);

			int pft_sdate1=-1;
			int pft_sdate2=-1;
			int pft_hdate1=-1;
			int pft_hdate2=-1;
			int pft_lgp=-1;
			double pft_phu=-1;
			double pft_fphu=-1;
			double pft_fhi=-1;

			Gridcell::iterator gc_itr = gridcell.begin();
			while (gc_itr != gridcell.end()) {
				Stand& stand = *gc_itr;

				if (stlist[stand.stid].pftinrotation(pft.name) >= 0) {
					pft_sdate1=stand[0].pft[pft.id].cropphen->sdate_thisyear[0];
					pft_sdate2=stand[0].pft[pft.id].cropphen->sdate_thisyear[1];
					pft_hdate1=stand[0].pft[pft.id].cropphen->hdate_harvest[0];
					pft_hdate2=stand[0].pft[pft.id].cropphen->hdate_harvest[1];
					pft_lgp=stand[0].pft[pft.id].cropphen->lgp;
					pft_phu=stand[0].pft[pft.id].cropphen->phu;
					pft_fphu=stand[0].pft[pft.id].cropphen->fphu_harv;
					pft_fhi=stand[0].pft[pft.id].cropphen->fhi_harv;
				}

				++gc_itr;
			}
			outlimit_misc(out, out_sdate1, pft_sdate1);
			outlimit_misc(out, out_sdate2, pft_sdate2);
			outlimit_misc(out, out_hdate1, pft_hdate1);
			outlimit_misc(out, out_hdate2, pft_hdate2);
			outlimit_misc(out, out_lgp,    pft_lgp);
			outlimit_misc(out, out_phu,    pft_phu);
			outlimit_misc(out, out_fphu,   pft_fphu);
			outlimit_misc(out, out_fhi,    pft_fhi);
		}

		pftlist.nextobj();

	} // *** End of PFT loop ***

	double flux_veg_lc[NLANDCOVERTYPES], flux_repr_lc[NLANDCOVERTYPES],
		 flux_soil_lc[NLANDCOVERTYPES], flux_fire_lc[NLANDCOVERTYPES],
		 flux_est_lc[NLANDCOVERTYPES], flux_seed_lc[NLANDCOVERTYPES];
	double flux_charvest_lc[NLANDCOVERTYPES], c_org_leach_lc[NLANDCOVERTYPES];
	double c_litter_lc[NLANDCOVERTYPES], c_fast_lc[NLANDCOVERTYPES],
		  c_slow_lc[NLANDCOVERTYPES], c_harv_slow_lc[NLANDCOVERTYPES];
	double surfsoillitterc_lc[NLANDCOVERTYPES], cwdc_lc[NLANDCOVERTYPES],
		 centuryc_lc[NLANDCOVERTYPES];

	double n_harv_slow_lc[NLANDCOVERTYPES], availn_lc[NLANDCOVERTYPES],
		   andep_lc[NLANDCOVERTYPES], anfert_lc[NLANDCOVERTYPES];
	double anmin_lc[NLANDCOVERTYPES], animm_lc[NLANDCOVERTYPES],
		   anfix_lc[NLANDCOVERTYPES], n_org_leach_lc[NLANDCOVERTYPES],
		   n_min_leach_lc[NLANDCOVERTYPES];
	double flux_ntot_lc[NLANDCOVERTYPES], flux_nharvest_lc[NLANDCOVERTYPES],
		   flux_nseed_lc[NLANDCOVERTYPES];
	double surfsoillittern_lc[NLANDCOVERTYPES], cwdn_lc[NLANDCOVERTYPES],
		   centuryn_lc[NLANDCOVERTYPES];

	for (int i=0; i<NLANDCOVERTYPES; i++) {
		flux_veg_lc[i]=0.0;
		flux_repr_lc[i]=0.0;
		flux_soil_lc[i]=0.0;
		flux_fire_lc[i]=0.0;
		flux_est_lc[i]=0.0;
		flux_seed_lc[i]=0.0;
		flux_charvest_lc[i]=0.0;
		c_org_leach_lc[i]=0.0;

		c_litter_lc[i]=0.0;
		c_fast_lc[i]=0.0;
		c_slow_lc[i]=0.0;
		c_harv_slow_lc[i]=0.0;
		surfsoillitterc_lc[i]=0.0;
		cwdc_lc[i]=0.0;
		centuryc_lc[i]=0.0;

		flux_ntot_lc[i]=0.0;
		flux_nharvest_lc[i]=0.0;
		flux_nseed_lc[i]=0.0;

		availn_lc[i]=0.0;
		andep_lc[i]=0.0;	// same value for all land covers
		anfert_lc[i]=0.0;
		anmin_lc[i]=0.0;
		animm_lc[i]=0.0;
		anfix_lc[i]=0.0;
		n_org_leach_lc[i]=0.0;
		n_min_leach_lc[i]=0.0;
		n_harv_slow_lc[i]=0.0;
		surfsoillittern_lc[i]=0.0;
		cwdn_lc[i]=0.0;
		centuryn_lc[i]=0.0;
	}

	// Sum C fluxes, dead C pools and runoff across patches

	Gridcell::iterator gc_itr = gridcell.begin();

	// Loop through Stands
	while (gc_itr != gridcell.end()) {
		Stand& stand = *gc_itr;
		stand.firstobj();

		//Loop through Patches
		while (stand.isobj) {
			Patch& patch = stand.getobj();

			double to_gridcell_average = stand.get_gridcell_fraction() / (double)stand.npatch();

			flux_nseed_lc[stand.landcover]+=patch.fluxes.get_annual_flux(Fluxes::SEEDN)*to_gridcell_average;
			flux_nharvest_lc[stand.landcover]+=patch.fluxes.get_annual_flux(Fluxes::HARVESTN)*to_gridcell_average;
			flux_ntot_lc[stand.landcover]+=(patch.fluxes.get_annual_flux(Fluxes::NH3_FIRE) +
					   patch.fluxes.get_annual_flux(Fluxes::NOx_FIRE) +
					   patch.fluxes.get_annual_flux(Fluxes::N2O_FIRE) +
					   patch.fluxes.get_annual_flux(Fluxes::N2_FIRE) +
					   patch.fluxes.get_annual_flux(Fluxes::N_SOIL)) * to_gridcell_average;

			flux_veg_lc[stand.landcover]+=-patch.fluxes.get_annual_flux(Fluxes::NPP)*to_gridcell_average;
			flux_repr_lc[stand.landcover]+=-patch.fluxes.get_annual_flux(Fluxes::REPRC)*to_gridcell_average;
			flux_soil_lc[stand.landcover]+=patch.fluxes.get_annual_flux(Fluxes::SOILC)*to_gridcell_average;
			flux_fire_lc[stand.landcover]+=patch.fluxes.get_annual_flux(Fluxes::FIREC)*to_gridcell_average;
			flux_est_lc[stand.landcover]+=patch.fluxes.get_annual_flux(Fluxes::ESTC)*to_gridcell_average;
			flux_seed_lc[stand.landcover]+=patch.fluxes.get_annual_flux(Fluxes::SEEDC)*to_gridcell_average;
			flux_charvest_lc[stand.landcover]+=patch.fluxes.get_annual_flux(Fluxes::HARVESTC)*to_gridcell_average;

			c_fast_lc[stand.landcover]+=patch.soil.cpool_fast*to_gridcell_average;
			c_slow_lc[stand.landcover]+=patch.soil.cpool_slow*to_gridcell_average;

			//Sum slow pools of harvested products
			if (run_landcover && ifslowharvestpool) {
				for (int q=0;q<npft;q++) {
					Patchpft& patchpft=patch.pft[q];
					c_harv_slow_lc[stand.landcover]+=patchpft.harvested_products_slow*to_gridcell_average;		  //slow pool in receiving landcover (1)
					n_harv_slow_lc[stand.landcover]+=patchpft.harvested_products_slow_nmass*to_gridcell_average;
				}
			}

			//Gridcell irrigation
			irrigation_gridcell += patch.irrigation_y*to_gridcell_average;

			andep_lc[stand.landcover] += stand.get_climate().andep * to_gridcell_average;
			anfert_lc[stand.landcover] += patch.anfert * to_gridcell_average;
			anmin_lc[stand.landcover] += patch.soil.anmin * to_gridcell_average;
			animm_lc[stand.landcover] += patch.soil.animmob * to_gridcell_average;
			anfix_lc[stand.landcover] += patch.soil.anfix * to_gridcell_average;
			n_min_leach_lc[stand.landcover] += patch.soil.aminleach * to_gridcell_average;
			n_org_leach_lc[stand.landcover] += patch.soil.aorgNleach * to_gridcell_average;
			c_org_leach_lc[stand.landcover] += patch.soil.aorgCleach * to_gridcell_average;
			availn_lc[stand.landcover] += (patch.soil.nmass_avail + patch.soil.snowpack_nmass) * to_gridcell_average;

			for (int r = 0; r < NSOMPOOL-1; r++) {

				if (r == SURFMETA || r == SURFSTRUCT || r == SOILMETA || r == SOILSTRUCT){
					surfsoillitterc_lc[stand.landcover] += patch.soil.sompool[r].cmass * to_gridcell_average;
					surfsoillittern_lc[stand.landcover] += patch.soil.sompool[r].nmass * to_gridcell_average;
				}
				else if (r == SURFFWD || r == SURFCWD) {
					cwdc_lc[stand.landcover] += patch.soil.sompool[r].cmass * to_gridcell_average;
					cwdn_lc[stand.landcover] += patch.soil.sompool[r].nmass * to_gridcell_average;
				}
				else {
					centuryc_lc[stand.landcover] += patch.soil.sompool[r].cmass * to_gridcell_average;
					centuryn_lc[stand.landcover]  += patch.soil.sompool[r].nmass * to_gridcell_average;
				}
			}
			stand.nextobj();
		} // patch loop
		++gc_itr;
	} // stand loop

	// Print per-stand totals
	if (printseparatestands) {

		Gridcell::iterator gc_itr = gridcell.begin();
		while (gc_itr != gridcell.end()) {

			Stand& stand = *gc_itr;
			int id = stand.id;;

			if (stand.landcover==NATURAL) {

				if (!out_anpp_stand_natural[id].invalid())
					outlimit_misc(out, out_anpp_stand_natural[id],   stand.anpp);
				if (!out_cmass_stand_natural[id].invalid())
					outlimit_misc(out, out_cmass_stand_natural[id],  stand.cmass);
			}
			else if (stand.landcover == FOREST) {

				if (!out_anpp_stand_forest[id].invalid())
					outlimit_misc(out, out_anpp_stand_forest[id],    stand.anpp);
				if (!out_cmass_stand_forest[id].invalid())
					outlimit_misc(out, out_cmass_stand_forest[id],   stand.cmass);
			}

			++gc_itr;
		}
	}

	if (run[CROPLAND]) {
		outlimit_misc(out, out_irrigation,   irrigation_gridcell);
	}

	// Print landcover totals to files
	if (run_landcover) {
		for (int i=0;i<NLANDCOVERTYPES;i++) {
			if (run[i]) {
				switch (i) {
				case CROPLAND:
					break;
				case PASTURE:
					if (run[NATURAL]) {
						outlimit_misc(out, out_anpp_pasture,     landcover_anpp[i]);
						outlimit_misc(out, out_cmass_pasture,	landcover_cmass[i]);
					}
					break;
				case BARREN:
					break;
				case NATURAL:
//					if(run[FOREST] || run[PASTURE]) {
					if(run[FOREST]) {
						outlimit_misc(out, out_anpp_natural,     landcover_anpp[i]);
						outlimit_misc(out, out_cmass_natural,	landcover_cmass[i]);
						outlimit_misc(out, out_dens_natural,		landcover_densindiv_total[i]);
					}
					break;
				case FOREST:
					if (run[NATURAL]) {
						outlimit_misc(out, out_anpp_forest,      landcover_anpp[i]);
						outlimit_misc(out, out_cmass_forest,		landcover_cmass[i]);
						outlimit_misc(out, out_dens_forest,		landcover_densindiv_total[i]);
					}
					break;
				case URBAN:
					break;
				case PEATLAND:
					break;
				default:
					if (date.year == nyear_spinup)
						dprintf("Modify code to deal with landcover output!\n");
				}
			}
		}
	}

	// Print C fluxes to per-landcover files
	if (run_landcover) {
		for (int i=0;i<NLANDCOVERTYPES;i++) {
			if (run[i]) {

				GuessOutput::Table* table_p=NULL;
				GuessOutput::Table* table_p_N=NULL;

				switch (i) {
				case CROPLAND:
					table_p=&out_cflux_cropland;
					table_p_N=&out_nflux_cropland;
					break;
				case PASTURE:
					table_p=&out_cflux_pasture;
					table_p_N=&out_nflux_pasture;
					break;
				case NATURAL:
					table_p=&out_cflux_natural;
					table_p_N=&out_nflux_natural;
					break;
				case FOREST:
					table_p=&out_cflux_forest;
					table_p_N=&out_nflux_forest;
					break;
				case URBAN:
					break;
				case PEATLAND:
					break;
				case BARREN:
					break;
				default:
					if (date.year == nyear_spinup)
						dprintf("Modify code to deal with landcover output!\n");
				}

				if (table_p) {
					outlimit_misc(out, *table_p, flux_veg_lc[i]);
					outlimit_misc(out, *table_p, -flux_repr_lc[i]);
					outlimit_misc(out, *table_p, flux_soil_lc[i] + c_org_leach_lc[i]);
					outlimit_misc(out, *table_p, flux_fire_lc[i]);
					outlimit_misc(out, *table_p, flux_est_lc[i]);

					outlimit_misc(out, *table_p_N, -andep_lc[i] * m2toha);
					outlimit_misc(out, *table_p_N, -anfix_lc[i] * m2toha);
					outlimit_misc(out, *table_p_N, -anfert_lc[i] * m2toha);
					outlimit_misc(out, *table_p_N, flux_ntot_lc[i] * m2toha);
					outlimit_misc(out, *table_p_N, (n_min_leach_lc[i] + n_org_leach_lc[i]) * m2toha);

					if (run_landcover) {
						 outlimit_misc(out, *table_p, flux_seed_lc[i]);
						 outlimit_misc(out, *table_p, flux_charvest_lc[i]);
						 outlimit_misc(out, *table_p, lc.acflux_landuse_change_lc[i]);
						 outlimit_misc(out, *table_p, lc.acflux_harvest_slow_lc[i]);
						 outlimit_misc(out, *table_p_N, flux_nseed_lc[i] * m2toha);
						 outlimit_misc(out, *table_p_N, flux_nharvest_lc[i] * m2toha);
						 outlimit_misc(out, *table_p_N, lc.anflux_landuse_change_lc[i] * m2toha);
						 outlimit_misc(out, *table_p_N, lc.anflux_harvest_slow_lc[i] * m2toha);
					}
				}

				double cflux_total = flux_veg_lc[i] - flux_repr_lc[i] + flux_soil_lc[i] + flux_fire_lc[i] + flux_est_lc[i] + c_org_leach_lc[i];
				double nflux_total = -andep_lc[i] - anfix_lc[i] - anfert_lc[i] + flux_ntot_lc[i] + n_min_leach_lc[i] + n_org_leach_lc[i];

				if (run_landcover) {
					cflux_total += flux_seed_lc[i];
					cflux_total += flux_charvest_lc[i];
					cflux_total += lc.acflux_landuse_change_lc[i];
					cflux_total += lc.acflux_harvest_slow_lc[i];
					nflux_total += flux_nseed_lc[i];
					nflux_total += flux_nharvest_lc[i];
					nflux_total += lc.anflux_landuse_change_lc[i];
					nflux_total += lc.anflux_harvest_slow_lc[i];
				}
				if (table_p) {
					outlimit_misc(out, *table_p,  cflux_total);
					outlimit_misc(out, *table_p_N,  nflux_total * m2toha);
				}
			}
		}

		// Print C pools to per-landcover files
		for (int i=0;i<NLANDCOVERTYPES;i++) {
			if (run[i]) {

				GuessOutput::Table* table_p=NULL;
				GuessOutput::Table* table_p_N=NULL;

				switch (i) {
				case CROPLAND:
					table_p=&out_cpool_cropland;
					table_p_N=&out_npool_cropland;
					break;
				case PASTURE:
					table_p=&out_cpool_pasture;
					table_p_N=&out_npool_pasture;
					break;
				case NATURAL:
					table_p=&out_cpool_natural;
					table_p_N=&out_npool_natural;
					break;
				case FOREST:
					table_p=&out_cpool_forest;
					table_p_N=&out_npool_forest;
					break;
				case URBAN:
					break;
				case PEATLAND:
					break;
				case BARREN:
					break;
				default:
					if (date.year == nyear_spinup)
						dprintf("Modify code to deal with landcover output!\n");
				}

				if (table_p) {
					outlimit_misc(out, *table_p, landcover_cmass[i] * lc.frac[i]);
					outlimit_misc(out, *table_p_N, (landcover_nmass[i] + landcover_nlitter[i]) * lc.frac[i]);

					if (!ifcentury) {
						outlimit_misc(out, *table_p, landcover_clitter[i] * lc.frac[i]);
						outlimit_misc(out, *table_p, c_fast_lc[i]);
						outlimit_misc(out, *table_p, c_slow_lc[i]);
					}
					else {
						outlimit_misc(out, *table_p, landcover_clitter[i] * lc.frac[i] + surfsoillitterc_lc[i] + cwdc_lc[i]);
						outlimit_misc(out, *table_p, centuryc_lc[i]);
						outlimit_misc(out, *table_p_N, surfsoillittern_lc[i] + cwdn_lc[i]);
						outlimit_misc(out, *table_p_N, centuryn_lc[i] + availn_lc[i]);
					}

					if (run_landcover && ifslowharvestpool) {
						outlimit_misc(out, *table_p, c_harv_slow_lc[i]);
						outlimit_misc(out, *table_p_N, n_harv_slow_lc[i]);
					}
				}

				// Calculate total cpool, starting with cmass and litter...
				double cpool_total = (landcover_cmass[i] + landcover_clitter[i]) * lc.frac[i];
				double npool_total = (landcover_nmass[i] + landcover_nlitter[i]) * lc.frac[i];

				// Add SOM pools
				if (!ifcentury) {
					cpool_total += c_fast_lc[i] + c_slow_lc[i];
				}
				else {
					cpool_total += centuryc_lc[i] + surfsoillitterc_lc[i] + cwdc_lc[i];
					npool_total += centuryn_lc[i] + surfsoillittern_lc[i] + cwdn_lc[i] + availn_lc[i];
				}

				// Add slow harvest pool if needed
				if (run_landcover && ifslowharvestpool) {
					cpool_total += c_harv_slow_lc[i];
					npool_total += n_harv_slow_lc[i];
				}
				if (table_p) {
					outlimit_misc(out, *table_p, cpool_total);
					outlimit_misc(out, *table_p_N, npool_total);
				}
			}
		}
	}

	//Output of seasonality variables
	if (run[CROPLAND]) {
		outlimit_misc(out, out_seasonality,   gridcell.climate.seasonality);
		outlimit_misc(out, out_seasonality,   gridcell.climate.var_temp);
		outlimit_misc(out, out_seasonality,   gridcell.climate.var_prec);
		outlimit_misc(out, out_seasonality,   gridcell.climate.mtemp_min20);
		outlimit_misc(out, out_seasonality,   gridcell.climate.atemp_mean);
		outlimit_misc(out, out_seasonality,   gridcell.climate.temp_seasonality);
		outlimit_misc(out, out_seasonality,   gridcell.climate.mprec_petmin20);
		outlimit_misc(out, out_seasonality,   gridcell.climate.aprec);
		outlimit_misc(out, out_seasonality,   gridcell.climate.prec_range);
	}
}

/// Output of simulation results at the end of each day
/** This function does not have to provide any information to the framework.
  */
void MiscOutput::outdaily(Gridcell& gridcell) {

	double lon = gridcell.get_lon();
	double lat = gridcell.get_lat();
	OutputRows out(output_channel, lon, lat, date.get_calendar_year(), date.day);

	if (date.year < nyear_spinup) {
		return;
	}

	pftlist.firstobj();
	while (pftlist.isobj) {
		Pft& pft=pftlist.getobj();

		if (pft.landcover != CROPLAND) {
			pftlist.nextobj();
			continue;
		}

		Gridcell::iterator gc_itr = gridcell.begin();
		while (gc_itr != gridcell.end()) {

			Stand& stand = *gc_itr;
			if (stand.landcover != CROPLAND || stand.npatch() > 1) {
				break;
			}

			stand.firstobj();
			while (stand.isobj) {
				Patch& patch = stand.getobj();
				Vegetation& vegetation=patch.vegetation;
				Patchpft& patchpft=patch.pft[pft.id];

				double cwdn = patch.soil.sompool[SURFCWD].nmass + patch.soil.sompool[SURFFWD].nmass;
				vegetation.firstobj();
				while (vegetation.isobj) {
					Individual& indiv=vegetation.getobj();
					if (indiv.id != -1 && indiv.pft.id == pft.id && !indiv.cropindiv->isintercropgrass) {
						//To be able to print values for the year after establishment of crops !
						// (if not dead and has existed for at least one year)
						// Ben 2007-11-28

						plot("daily leaf N [g/m2]",pft.name,date.day,indiv.nmass_leaf*m2toha);
						plot("daily leaf C [kg/m2]",pft.name,date.day,indiv.cmass_leaf_today()*m2toha);
						outlimit_misc(out, out_daily_lai,indiv.lai_today());
						outlimit_misc(out, out_daily_npp,indiv.dnpp*m2toha);
						outlimit_misc(out, out_daily_cmass_leaf,indiv.cmass_leaf_today()*m2toha);
						outlimit_misc(out, out_daily_nmass_leaf,indiv.nmass_leaf*m2toha);
						outlimit_misc(out, out_daily_cmass_root,indiv.cmass_root_today()*m2toha);
						outlimit_misc(out, out_daily_nmass_root,indiv.nmass_root*m2toha);
						outlimit_misc(out, out_daily_avail_nmass_soil,m2toha*(patch.soil.nmass_avail+cwdn));
						outlimit_misc(out, out_daily_n_input_soil,patch.soil.ninput*m2toha);

						double uw = patch.soil.dwcontupper[date.day];
						if (uw < 1e-22) {
							uw = 0.0;
						}
						double lw = patch.soil.dwcontlower[date.day];
						if (lw < 1e-22) {
							lw = 0.0;
						}
						outlimit_misc(out, out_daily_upper_wcont,uw);
						outlimit_misc(out, out_daily_lower_wcont,lw);
						outlimit_misc(out, out_daily_irrigation,patch.irrigation_d);

						outlimit_misc(out, out_daily_cmass_storage,indiv.cropindiv->grs_cmass_ho*m2toha);
						outlimit_misc(out, out_daily_nmass_storage,indiv.cropindiv->nmass_ho*m2toha);

						outlimit_misc(out, out_daily_ndemand,indiv.ndemand);
						outlimit_misc(out, out_daily_cton,limited_cton(indiv.cmass_leaf_today(),indiv.nmass_leaf*m2toha));

						if (ifnlim) {
							outlimit_misc(out, out_daily_ds,patch.pft[pft.id].cropphen->dev_stage); // daglig ds
							outlimit_misc(out, out_daily_cmass_stem,(indiv.cropindiv->grs_cmass_agpool+indiv.cropindiv->grs_cmass_stem)*m2toha);
							outlimit_misc(out, out_daily_nmass_stem,indiv.cropindiv->nmass_agpool*m2toha);

							outlimit_misc(out, out_daily_cmass_dead_leaf,indiv.cropindiv->grs_cmass_dead_leaf*m2toha);
							outlimit_misc(out, out_daily_nmass_dead_leaf,indiv.cropindiv->nmass_dead_leaf*m2toha);

							outlimit_misc(out, out_daily_fphu,patch.pft[pft.id].cropphen->fphu);
							outlimit_misc(out, out_daily_stem,patch.pft[pft.id].cropphen->f_alloc_stem);
							outlimit_misc(out, out_daily_leaf,patch.pft[pft.id].cropphen->f_alloc_leaf);
							outlimit_misc(out, out_daily_root,patch.pft[pft.id].cropphen->f_alloc_root);
							outlimit_misc(out, out_daily_storage,patch.pft[pft.id].cropphen->f_alloc_horg);
						}

					}
					vegetation.nextobj();
				}
				stand.nextobj();
			}
			++gc_itr;
		}
		pftlist.nextobj();
	}

	outlimit_misc(out, out_daily_temp, gridcell.climate.temp);
	outlimit_misc(out, out_daily_prec, gridcell.climate.prec);
	outlimit_misc(out, out_daily_rad, gridcell.climate.rad);
}

void MiscOutput::openlocalfiles(Gridcell& gridcell) {

	if(!printseparatestands)
		return;

	bool open_natural = false;
	bool open_forest = false;
	double lon = gridcell.get_lon();
	double lat = gridcell.get_lat();


	Gridcell::iterator gc_itr = gridcell.begin();

	// Loop through Stands
	while (gc_itr != gridcell.end()) {
		Stand& stand = *gc_itr;

		stand.anpp=0.0;
		stand.cmass=0.0;

		if(stand.first_year == date.year || stand.clone_year == date.year) {
			if(stand.landcover == NATURAL) {
				open_natural = true;
			}
			else if(stand.landcover == FOREST) {
				open_forest = true;
			}
		}

		++gc_itr;
	}

	if(PRINTFIRSTSTANDFROM1901 && date.year == nyear_spinup) {
		open_natural = true;
		open_forest = true;
	}

	if(open_natural || open_forest) {

		gc_itr = gridcell.begin();

		while (gc_itr != gridcell.end()) {

			Stand& stand = *gc_itr;

			int id = stand.id;
			char outfilename[100]={'\0'}, buffer[50]={'\0'};

			sprintf(buffer, "%.1f_%.1f_%d",lon, lat, id);
			strcat(buffer, ".out");

			// create a vector with the pft names
			std::vector<std::string> pfts;

			pftlist.firstobj();
			while (pftlist.isobj) {

				 Pft& pft=pftlist.getobj();	 
				 Standpft& standpft=stand.pft[pft.id];

				 if(standpft.active)
					 pfts.push_back((char*)pft.name);

				 pftlist.nextobj();
			}
			ColumnDescriptors anpp_columns;
			anpp_columns += ColumnDescriptors(pfts,               8, 3);
			anpp_columns += ColumnDescriptor("Total",             8, 3);

			if(open_natural && stand.landcover == NATURAL) {

				strcpy(outfilename, "anpp_natural_");
				strcat(outfilename, buffer);

				if(out_anpp_stand_natural[id].invalid())
					create_output_table(out_anpp_stand_natural[id], outfilename, anpp_columns);

				outfilename[0] = '\0';
				strcpy(outfilename, "cmass_natural_");
				strcat(outfilename, buffer);

				if(out_cmass_stand_natural[id].invalid())
					create_output_table(out_cmass_stand_natural[id], outfilename, anpp_columns);
			}
			else if(open_forest && stand.landcover == FOREST) {

				strcpy(outfilename, "anpp_forest_");
				strcat(outfilename, buffer);

				if(out_anpp_stand_forest[id].invalid())
					create_output_table(out_anpp_stand_forest[id], outfilename, anpp_columns);

				outfilename[0] = '\0';
				strcpy(outfilename, "cmass_forest_");
				strcat(outfilename, buffer);

				if(out_cmass_stand_forest[id].invalid())
					create_output_table(out_cmass_stand_forest[id], outfilename, anpp_columns);
			}

			++gc_itr;
		}
	}
}

void MiscOutput::closelocalfiles(Gridcell& gridcell) {

	if(!printseparatestands)
		return;

	for(int id=0;id<MAXNUMBER_STANDS;id++) {

		if(!out_anpp_stand_natural[id].invalid())
			close_output_table(out_anpp_stand_natural[id]);
		if(!out_cmass_stand_natural[id].invalid())
			close_output_table(out_cmass_stand_natural[id]);
		if(!out_anpp_stand_forest[id].invalid())
			close_output_table(out_anpp_stand_forest[id]);
		if(!out_cmass_stand_forest[id].invalid())
			close_output_table(out_cmass_stand_forest[id]);
	}
}

} // namespace
