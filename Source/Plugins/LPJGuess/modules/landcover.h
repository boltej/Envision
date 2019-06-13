///////////////////////////////////////////////////////////////////////////////////////
/// \file landcover.h
/// \brief Functions handling landcover aspects, such as creating or resizing Stands
///
/// \author Mats Lindeskog
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_LANDCOVER_H
#define LPJ_GUESS_LANDCOVER_H
#include "guess.h"
#include "growth.h"
#include "canexch.h"
#include "cropsowing.h"
#include "cropphenology.h"
#include "cropallocation.h"
#include "management.h"
#include "externalinput.h"
#include "inputmodule.h"

const int def_verndate_ndays_after_last_springdate = 60;

///	Creates stands for landcovers present in the gridcell first year of the simulation
void landcover_init(Gridcell& gridcell, InputModule* input_module);

/// Handles changes in the landcover fractions from year to year
/** This function will for instance kill or create new stands
 *  if needed.
 */
void landcover_dynamics(Gridcell& gridcell, InputModule* input_module);

/// Query whether a date is within a period spanned by two dates.
bool dayinperiod(int day, int start, int end);
/// Step n days from a date.
int stepfromdate(int day, int step);

/// struct storing carbon, nitrogen and water (plus aaet_5 and anfix_calc) during landcover change
struct landcover_change_transfer {

	double *transfer_litter_leaf;
	double *transfer_litter_sap;
	double *transfer_litter_heart;
	double *transfer_litter_root;
	double *transfer_litter_repr;
	double *transfer_harvested_products_slow;
	double *transfer_nmass_litter_leaf;
	double *transfer_nmass_litter_sap;
	double *transfer_nmass_litter_heart;
	double *transfer_nmass_litter_root;
	double *transfer_harvested_products_slow_nmass;

	double transfer_acflux_harvest;
	double transfer_anflux_harvest;

	double transfer_cpool_fast;
	double transfer_cpool_slow;
	double transfer_wcont[NSOILLAYER];
	double transfer_wcont_evap;
	double transfer_decomp_litter_mean;
	double transfer_k_soilfast_mean;
	double transfer_k_soilslow_mean;
	Sompool transfer_sompool[NSOMPOOL];
	double transfer_nmass_avail;
	double transfer_snowpack;
	double transfer_snowpack_nmass;

	Historic<double, NYEARAAET> transfer_aaet_5;
	double transfer_anfix_calc;

	landcover_change_transfer();
	~landcover_change_transfer();
	void allocate();

	double ccont() {

		double ccont = transfer_acflux_harvest;

		for(int i=0; i<npft; i++) {
			ccont += transfer_litter_leaf[i];
			ccont += transfer_litter_sap[i];
			ccont += transfer_litter_heart[i];
			ccont += transfer_litter_root[i];
			ccont += transfer_litter_repr[i];
			ccont += transfer_harvested_products_slow[i];
		}

		for(int i=0; i<NSOMPOOL-1; i++)
			ccont += transfer_sompool[i].cmass;

		return ccont;
	}

	double ncont() {

		double ncont = transfer_anflux_harvest;

		for(int i=0; i<npft; i++) {
			ncont += transfer_nmass_litter_leaf[i];
			ncont += transfer_nmass_litter_sap[i];
			ncont += transfer_nmass_litter_heart[i];
			ncont += transfer_nmass_litter_root[i];
			ncont += transfer_harvested_products_slow_nmass[i];
		}

		for(int i=0; i<NSOMPOOL-1; i++)
			ncont += transfer_sompool[i].nmass;

		ncont += transfer_nmass_avail;
		ncont += transfer_snowpack_nmass;

		return ncont;
	}

	// Adds non-living C, N and water from a stand
	void add_from_stand(Stand& stand, double scale) {

		stand.firstobj();
		while(stand.isobj) {
			Patch& patch = stand.getobj();

			// sum original litter C & N:
			for(int n=0; n<npft; n++)
			{
				transfer_litter_leaf[n] += patch.pft[n].litter_leaf * scale;
				transfer_litter_root[n] += patch.pft[n].litter_root * scale;
				transfer_litter_sap[n] += patch.pft[n].litter_sap * scale;
				transfer_litter_heart[n] += patch.pft[n].litter_heart * scale;
				transfer_litter_repr[n] += patch.pft[n].litter_repr * scale;

				transfer_nmass_litter_leaf[n] += patch.pft[n].nmass_litter_leaf * scale;
				transfer_nmass_litter_root[n] += patch.pft[n].nmass_litter_root*scale;
				transfer_nmass_litter_sap[n] += patch.pft[n].nmass_litter_sap * scale;
				transfer_nmass_litter_heart[n] += patch.pft[n].nmass_litter_heart * scale;

				if(ifslowharvestpool) {
					transfer_harvested_products_slow[n] += patch.pft[n].harvested_products_slow * scale;
					transfer_harvested_products_slow_nmass[n] += patch.pft[n].harvested_products_slow_nmass * scale;
				}
			}

			// sum litter C & N:
			transfer_cpool_fast += patch.soil.cpool_fast * scale;
			transfer_cpool_slow += patch.soil.cpool_slow * scale;

			// sum soil C & N:
			for(int i=0; i<NSOMPOOL; i++) {
				transfer_sompool[i].cmass += patch.soil.sompool[i].cmass * scale;
				transfer_sompool[i].fireresist += patch.soil.sompool[i].fireresist * scale;
				transfer_sompool[i].fracremain += patch.soil.sompool[i].fracremain * scale;
				transfer_sompool[i].ligcfrac += patch.soil.sompool[i].ligcfrac * scale;
				transfer_sompool[i].litterme += patch.soil.sompool[i].litterme * scale;
				transfer_sompool[i].nmass += patch.soil.sompool[i].nmass * scale;
				transfer_sompool[i].ntoc += patch.soil.sompool[i].ntoc * scale;
			}

			transfer_nmass_avail += patch.soil.nmass_avail * scale;

			// sum wcont:
			for(int i=0; i<NSOILLAYER; i++) {
				transfer_wcont[i] += patch.soil.wcont[i] * scale;
			}
			transfer_wcont_evap += patch.soil.wcont_evap * scale;

			transfer_snowpack += patch.soil.snowpack * scale;
			transfer_snowpack_nmass += patch.soil.snowpack_nmass * scale;

			transfer_decomp_litter_mean += patch.soil.decomp_litter_mean * scale;
			transfer_k_soilfast_mean += patch.soil.k_soilfast_mean * scale;
			transfer_k_soilslow_mean += patch.soil.k_soilslow_mean * scale;

			double aaet_5_cp[NYEARAAET] = {0.0};
			transfer_aaet_5.to_array(aaet_5_cp);
			for(unsigned int i=0;i<NYEARAAET;i++)
				transfer_aaet_5.add(aaet_5_cp[i] + patch.aaet_5[i] * scale);

			transfer_anfix_calc += patch.soil.anfix_calc * scale;

			stand.nextobj();
		}

		return;
	}

	void copy(landcover_change_transfer& from) {

		if(transfer_litter_leaf)
			memcpy(transfer_litter_leaf, from.transfer_litter_leaf, sizeof(double) * npft);
		if(transfer_litter_sap)
			memcpy(transfer_litter_sap, from.transfer_litter_sap, sizeof(double) * npft);
		if(transfer_litter_heart)
			memcpy(transfer_litter_heart, from.transfer_litter_heart, sizeof(double) * npft);
		if(transfer_litter_root)
			memcpy(transfer_litter_root, from.transfer_litter_root, sizeof(double) * npft);
		if(transfer_litter_repr)
			memcpy(transfer_litter_repr, from.transfer_litter_repr, sizeof(double) * npft);
		if(transfer_harvested_products_slow)
			memcpy(transfer_harvested_products_slow, from.transfer_harvested_products_slow, sizeof(double) * npft);
		if(transfer_nmass_litter_leaf)
			memcpy(transfer_nmass_litter_leaf, from.transfer_nmass_litter_leaf, sizeof(double) * npft);
		if(transfer_nmass_litter_sap)
			memcpy(transfer_nmass_litter_sap, from.transfer_nmass_litter_sap, sizeof(double) * npft);
		if(transfer_nmass_litter_heart)
			memcpy(transfer_nmass_litter_heart, from.transfer_nmass_litter_heart, sizeof(double) * npft);
		if(transfer_nmass_litter_root)
			memcpy(transfer_nmass_litter_root, from.transfer_nmass_litter_root, sizeof(double) * npft);
		if(transfer_harvested_products_slow_nmass)
			memcpy(transfer_harvested_products_slow_nmass, from.transfer_harvested_products_slow_nmass, sizeof(double) * npft);

		transfer_acflux_harvest = from.transfer_acflux_harvest;
		transfer_anflux_harvest = from.transfer_anflux_harvest;
		transfer_cpool_fast = from.transfer_cpool_fast;
		transfer_cpool_slow = from.transfer_cpool_slow;
		for(int i=0; i<NSOILLAYER; i++)
			transfer_wcont[i] = from.transfer_wcont[i];
		transfer_wcont_evap = from.transfer_wcont_evap;
		transfer_decomp_litter_mean = from.transfer_decomp_litter_mean;
		transfer_k_soilfast_mean = from.transfer_k_soilfast_mean;
		for(int i=0; i<NSOMPOOL; i++) {
			transfer_sompool[i].cmass = from.transfer_sompool[i].cmass;
			transfer_sompool[i].fireresist = from.transfer_sompool[i].fireresist;
			transfer_sompool[i].fracremain = from.transfer_sompool[i].fracremain;
			transfer_sompool[i].ligcfrac = from.transfer_sompool[i].ligcfrac;
			transfer_sompool[i].litterme = from.transfer_sompool[i].litterme;
			transfer_sompool[i].nmass = from.transfer_sompool[i].nmass;
			transfer_sompool[i].ntoc = from.transfer_sompool[i].ntoc;
		}
		transfer_nmass_avail = from.transfer_nmass_avail;
		transfer_snowpack = from.transfer_snowpack;
		transfer_snowpack_nmass = from.transfer_snowpack_nmass;

		for(unsigned int i=0;i<from.transfer_aaet_5.size();i++)
			transfer_aaet_5.add(from.transfer_aaet_5[i]);

		transfer_anfix_calc = from.transfer_anfix_calc;
	}

	void add(landcover_change_transfer& from, double multiplier = 1.0) {

		for(int i=0; i<npft; i++) {
			if(transfer_litter_leaf)
				transfer_litter_leaf[i] += from.transfer_litter_leaf[i] * multiplier;
			if(transfer_litter_sap)
				transfer_litter_sap[i] += from.transfer_litter_sap[i] * multiplier;
			if(transfer_litter_heart)
				transfer_litter_heart[i] += from.transfer_litter_heart[i] * multiplier;
			if(transfer_litter_root)
				transfer_litter_root[i] += from.transfer_litter_root[i] * multiplier;
			if(transfer_litter_repr)
				transfer_litter_repr[i] += from.transfer_litter_repr[i] * multiplier;
			if(transfer_harvested_products_slow)
				transfer_harvested_products_slow[i] += from.transfer_harvested_products_slow[i] * multiplier;
			if(transfer_nmass_litter_leaf)
				transfer_nmass_litter_leaf[i] += from.transfer_nmass_litter_leaf[i] * multiplier;
			if(transfer_nmass_litter_sap)
				transfer_nmass_litter_sap[i] += from.transfer_nmass_litter_sap[i] * multiplier;
			if(transfer_nmass_litter_heart)
				transfer_nmass_litter_heart[i] += from.transfer_nmass_litter_heart[i] * multiplier;
			if(transfer_nmass_litter_root)
				transfer_nmass_litter_root[i] += from.transfer_nmass_litter_root[i] * multiplier;
			if(transfer_harvested_products_slow_nmass)
				transfer_harvested_products_slow_nmass[i] += from.transfer_harvested_products_slow_nmass[i] * multiplier;
		}
		transfer_acflux_harvest += from.transfer_acflux_harvest * multiplier;
		transfer_anflux_harvest += from.transfer_anflux_harvest * multiplier;
		transfer_cpool_fast += from.transfer_cpool_fast * multiplier;
		transfer_cpool_slow += from.transfer_cpool_slow * multiplier;
		for(int i=0; i<NSOILLAYER; i++)
			transfer_wcont[i] += from.transfer_wcont[i] * multiplier;
		transfer_wcont_evap += from.transfer_wcont_evap * multiplier;
		transfer_decomp_litter_mean += from.transfer_decomp_litter_mean * multiplier;
		transfer_k_soilfast_mean += from.transfer_k_soilfast_mean * multiplier;
		for(int i=0; i<NSOMPOOL; i++) {
			transfer_sompool[i].cmass += from.transfer_sompool[i].cmass * multiplier;
			transfer_sompool[i].fireresist += from.transfer_sompool[i].fireresist * multiplier;
			transfer_sompool[i].fracremain += from.transfer_sompool[i].fracremain * multiplier;
			transfer_sompool[i].ligcfrac += from.transfer_sompool[i].ligcfrac * multiplier;
			transfer_sompool[i].litterme += from.transfer_sompool[i].litterme * multiplier;
			transfer_sompool[i].nmass += from.transfer_sompool[i].nmass * multiplier;
			transfer_sompool[i].ntoc += from.transfer_sompool[i].ntoc * multiplier;
		}
		transfer_nmass_avail += from.transfer_nmass_avail * multiplier;
		transfer_snowpack += from.transfer_snowpack * multiplier;
		transfer_snowpack_nmass += from.transfer_snowpack_nmass * multiplier;

		double aaet_5_cp[NYEARAAET] = {0.0};
		for(unsigned int i=0;i<transfer_aaet_5.size();i++)
			aaet_5_cp[i] = transfer_aaet_5[i];
		for(unsigned int i=0;i<NYEARAAET;i++)
			transfer_aaet_5.add(aaet_5_cp[i] + from.transfer_aaet_5[i] * multiplier);

		transfer_anfix_calc += from.transfer_anfix_calc * multiplier;
	}
};

struct lc_change_harvest_params {

	double harv_eff;
	double res_outtake_twig;
	double res_outtake_coarse_root;
	bool burn;	// to be developed

	lc_change_harvest_params() {

		res_outtake_twig = 0.0;
		res_outtake_coarse_root = 0.0;
		burn = false;
	}
};

#endif // LPJ_GUESS_LANDCOVER_H
