////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file management.h
/// \brief Harvest functions for cropland, managed forest and pasture			
/// \author Mats Lindeskog
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_MANAGEMENT_H
#define LPJ_GUESS_MANAGEMENT_H

// Forward declaration
struct Harvest_CN;

/// Harvest function for cropland, including true crops, intercrop grass
void harvest_crop(Harvest_CN& indiv_cp, Pft& pft, bool alive, bool isintercropgrass);
/// Harvest function for cropland, including true crops, intercrop grass
void harvest_crop(Individual& indiv, Pft& pft, bool alive, bool isintercropgrass, bool harvest_grs);
/// Harvest function used for managed forest and for clearing natural vegetation at land use change.
void harvest_wood(Harvest_CN& indiv_cp,Pft& pft, bool alive, double frac_cut, double harv_eff, double res_outtake_twig = 0.0, double res_outtake_coarse_root = 0.0);
/// Harvest function used for managed forest and for clearing natural vegetation at land use change.
void harvest_wood(Individual& indiv, double frac_cut, double harv_eff, double res_outtake_twig = 0.0, double res_outtake_coarse_root = 0.0, bool lc_change = false);
/// Harvest function for pasture, representing grazing.
void harvest_pasture(Harvest_CN& indiv_cp, Pft& pft, bool alive);
/// Harvest function for pasture, representing grazing.
void harvest_pasture(Individual& indiv, Pft& pft, bool alive);
/// Harvest function for managed forest
void harvest_forest(Individual& indiv, Pft& pft, bool alive, double anpp, bool& killed);
/// Transfers all carbon and nitrogen from living tissue to litter.
void kill_remaining_vegetation(Harvest_CN& indiv_cp, Pft& pft, bool alive, bool istruecrop_or_intercropgrass, bool burn = false);
/// Transfers all carbon and nitrogen from living tissue to litter.
void kill_remaining_vegetation(Individual& indiv, bool burn = false, bool lc_change = false);
/// Scaling of last year's or harvest day individual carbon and nitrogen member values in stands that have increased their area fraction this year.
void scale_indiv(Individual& indiv, bool scale_grsC);
/// Yearly function for harvest of all land covers. Should only be called from growth()
bool harvest_year(Individual& indiv);
/// Yield function for true crops and intercrop grass
void yield_crop(Individual& indiv);
/// Yield function for pasture grass grown in cropland landcover
void yield_pasture(Individual& indiv, double cmass_leaf_inc);
/// Determines amount of nitrogen applied today
void nfert(Patch& patch);
/// Updates crop rotation status
void crop_rotation(Stand& stand);
/// Determines cutting intensity before wood harvest
double cut_fraction(Patch& patch);

/// Struct for copies of carbon and nitrogen of an individual and associated litter and fluxes resulting from harvest
/// This is needed if we want to harvest only part of a stand, as during land cover change.
struct Harvest_CN {

	double cmass_leaf;
	double cmass_root;
	double cmass_sap;
	double cmass_heart;
	double cmass_debt;
	double cmass_ho;
	double cmass_agpool;
	double cmass_stem;
	double cmass_dead_leaf;
	double debt_excess;
	double nmass_leaf;
	double nmass_root;
	double nmass_sap;
	double nmass_heart;
	double nmass_ho;
	double nmass_agpool;
	double nmass_dead_leaf;
	double nstore_longterm;
	double nstore_labile;
	double max_n_storage;

	double litter_leaf;
	double litter_root;
	double litter_sap;
	double litter_heart;
	double nmass_litter_leaf;
	double nmass_litter_root;
	double nmass_litter_sap;
	double nmass_litter_heart;
	double acflux_harvest;
	double anflux_harvest;
	double harvested_products_slow;
	double harvested_products_slow_nmass;

	Harvest_CN() {

		cmass_leaf = cmass_root = cmass_sap = cmass_heart = cmass_debt = cmass_ho = cmass_agpool = cmass_stem = cmass_dead_leaf = debt_excess = 0.0;
		nmass_leaf = nmass_root = nmass_sap = nmass_heart = nmass_ho = nmass_agpool = nmass_dead_leaf = nstore_longterm = nstore_labile = max_n_storage = 0.0;
		litter_leaf = litter_root = litter_sap = litter_heart = 0.0;
		nmass_litter_leaf = nmass_litter_root = nmass_litter_sap = nmass_litter_heart = 0.0;
		acflux_harvest = anflux_harvest = 0.0;
		harvested_products_slow = harvested_products_slow_nmass = 0.0;
	}

	/// Copies C and N values from individual and patchpft tp struct.
	void copy_from_indiv(Individual& indiv, bool copy_grsC = false, bool copy_dead_C = true) {

		Patch& patch = indiv.vegetation.patch;
		Patchpft& ppft = patch.pft[indiv.pft.id];

		if(copy_grsC) {

			if(indiv.cropindiv) {

				cmass_leaf = indiv.cropindiv->grs_cmass_leaf;
				cmass_root = indiv.cropindiv->grs_cmass_root;

				if(indiv.pft.landcover == CROPLAND) {
					cmass_ho = indiv.cropindiv->grs_cmass_ho;
					cmass_agpool = indiv.cropindiv->grs_cmass_agpool;
					cmass_stem = indiv.cropindiv->grs_cmass_stem;
					cmass_dead_leaf = indiv.cropindiv->grs_cmass_dead_leaf;
				}
			}
		}
		else {

			cmass_leaf = indiv.cmass_leaf;
			cmass_root = indiv.cmass_root;
			cmass_sap = indiv.cmass_sap;
			cmass_heart = indiv.cmass_heart;
			cmass_debt = indiv.cmass_debt;

			if(indiv.pft.landcover == CROPLAND) {
				cmass_ho = indiv.cropindiv->cmass_ho;
				cmass_agpool = indiv.cropindiv->cmass_agpool;
//				cmass_stem = indiv.cropindiv->grs_cmass_stem;			// We can't use grs_cmass here !
//				cmass_dead_leaf = indiv.cropindiv->grs_cmass_dead_leaf;
			}
		}


		nmass_leaf = indiv.nmass_leaf;
		nmass_root = indiv.nmass_root;
		nmass_sap = indiv.nmass_sap;
		nmass_heart = indiv.nmass_heart;
		nstore_longterm = indiv.nstore_longterm;
		nstore_labile = indiv.nstore_labile;
		max_n_storage = indiv.max_n_storage;

		if(indiv.pft.landcover == CROPLAND) {
			nmass_ho = indiv.cropindiv->nmass_ho;
			nmass_agpool = indiv.cropindiv->nmass_agpool;
			nmass_dead_leaf = indiv.cropindiv->nmass_dead_leaf;
		}

		if(copy_dead_C) {

			litter_leaf = ppft.litter_leaf;
			litter_root = ppft.litter_root;
			litter_sap = ppft.litter_sap;
			litter_heart = ppft.litter_heart;

			nmass_litter_leaf = ppft.nmass_litter_leaf;
			nmass_litter_root = ppft.nmass_litter_root;
			nmass_litter_sap = ppft.nmass_litter_sap;
			nmass_litter_heart = ppft.nmass_litter_heart;

			// acflux_harvest and anflux_harvest only for output
			harvested_products_slow = ppft.harvested_products_slow;
			harvested_products_slow_nmass = ppft.harvested_products_slow_nmass;
		}
	}

	/// Copies C and N values from struct to individual, patchpft and patch (fluxes).
	void copy_to_indiv(Individual& indiv, bool copy_grsC = false, bool lc_change = false) {

		Patch& patch = indiv.vegetation.patch;
		Patchpft& ppft = patch.pft[indiv.pft.id];

		if(copy_grsC) {

			indiv.cropindiv->grs_cmass_leaf = cmass_leaf;
			indiv.cropindiv->grs_cmass_root = cmass_root;

			if(indiv.pft.landcover == CROPLAND) {
				indiv.cropindiv->grs_cmass_ho = cmass_ho;
				indiv.cropindiv->grs_cmass_agpool = cmass_agpool;
				indiv.cropindiv->grs_cmass_dead_leaf = cmass_dead_leaf;
				indiv.cropindiv->grs_cmass_stem = cmass_stem;
			}
		}
		else {

			indiv.cmass_leaf = cmass_leaf;
			indiv.cmass_root = cmass_root;
			indiv.cmass_sap = cmass_sap;
			indiv.cmass_heart = cmass_heart;
			indiv.cmass_debt = cmass_debt;

			if(indiv.pft.landcover == CROPLAND) {
				indiv.cropindiv->cmass_ho = cmass_ho;
				indiv.cropindiv->cmass_agpool = cmass_agpool;
//				indiv.cropindiv->grs_cmass_dead_leaf = cmass_dead_leaf;	// We can't use grs_cmass here !
//				indiv.cropindiv->grs_cmass_stem = cmass_stem;
			}
		}

		indiv.nmass_leaf = nmass_leaf;
		indiv.nmass_root = nmass_root;
		indiv.nmass_sap = nmass_sap;
		indiv.nmass_heart = nmass_heart;
		indiv.nstore_longterm = nstore_longterm;
		indiv.nstore_labile = nstore_labile;

		if(indiv.pft.landcover == CROPLAND) {
			indiv.cropindiv->nmass_ho = nmass_ho;
			indiv.cropindiv->nmass_agpool = nmass_agpool;
			indiv.cropindiv->nmass_dead_leaf = nmass_dead_leaf;
		}

		ppft.litter_leaf = litter_leaf;
		ppft.litter_root = litter_root;
		ppft.litter_sap = litter_sap;
		ppft.litter_heart = litter_heart;
		ppft.nmass_litter_leaf = nmass_litter_leaf;
		ppft.nmass_litter_root = nmass_litter_root;
		ppft.nmass_litter_sap = nmass_litter_sap;
		ppft.nmass_litter_heart = nmass_litter_heart;

		if(!lc_change) {
			patch.fluxes.report_flux(Fluxes::HARVESTC, acflux_harvest);	// Put into gridcell.acflux_landuse_change instead at land use change
			patch.fluxes.report_flux(Fluxes::HARVESTN, anflux_harvest);	// Put into gridcell.anflux_landuse_change instead at land use change
		}

//		indiv.report_flux(Fluxes::NPP, debt_excess);
//		indiv.report_flux(Fluxes::RA, -debt_excess);

		ppft.harvested_products_slow = harvested_products_slow;
		ppft.harvested_products_slow_nmass = harvested_products_slow_nmass;
	}
};

#endif // LPJ_GUESS_MANAGEMENT_H