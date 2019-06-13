////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file cropallocation.cpp
/// \brief Crop allocation and growth
/// \author Mats Lindeskog, Stefan Olin
/// $Date:  $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "landcover.h"
#include "cropallocation.h"

// Seed carbon allocation to leaves and roots are done over a 10-day period
const bool DELAYED_SEEDCARBON = false;

/// Updates patch members fpc_total and fpc_rescale for crops (to be called after crop_phenology())
void update_patch_fpc(Patch& patch) {

	if(patch.stand.landcover != CROPLAND) {
		return;
	}

	Vegetation& vegetation = patch.vegetation;
	patch.fpc_total = 0.0;

	vegetation.firstobj();
	while (vegetation.isobj) {
		Individual& indiv = vegetation.getobj();

		if(indiv.growingseason()) {
			patch.fpc_total += indiv.fpc;
		}
		vegetation.nextobj();
	}
	// Calculate rescaling factor to account for overlap between populations/
	// cohorts/individuals (i.e. total FPC > 1)
	// necessary to undate here after growingseason updated
	patch.fpc_rescale = 1.0 / max(patch.fpc_total, 1.0);
}


/// Updates lai_daily and fpc_daily from daily grs_cmass_leaf-value
/** lai during senescence declines according the function senescence_curve()
 */
void lai_crop(Patch& patch) {

	Vegetation& vegetation = patch.vegetation;
	vegetation.firstobj();

	while (vegetation.isobj) {

		Individual& indiv = vegetation.getobj();
		cropindiv_struct& cropindiv = *(indiv.get_cropindiv());
		Patchpft& patchpft = patch.pft[indiv.pft.id];
		cropphen_struct& ppftcrop = *(patchpft.get_cropphen());

		if(indiv.pft.phenology == CROPGREEN) {

			if(ppftcrop.growingseason) {

				if(!ppftcrop.senescence || ifnlim)
					indiv.lai_daily = cropindiv.grs_cmass_leaf * indiv.pft.sla;
				else
					// Follow the senescence curve from leaf cmass at senescence (cmass_leaf_sen):
					indiv.lai_daily = cropindiv.cmass_leaf_sen * indiv.pft.sla * senescence_curve(indiv.pft, ppftcrop.fphu);

				if(indiv.lai_daily < 0.0)
					indiv.lai_daily = 0.0;

				indiv.fpc_daily = 1.0 - lambertbeer(indiv.lai_daily);

				indiv.lai_indiv_daily = indiv.lai_daily;
			}
			else if(date.day == ppftcrop.hdate) {
				indiv.lai_daily = 0.0;
				indiv.lai_indiv_daily = 0.0;
				indiv.fpc_daily = 0.0;
			}
		}
		vegetation.nextobj();
	}
}

/// Turnover function for continuous grass, to be called from any day of the year from allocation_crop_daily().
void turnover_grass(Individual& indiv) {

	cropindiv_struct& cropindiv = *(indiv.get_cropindiv());
	Patchpft& patchpft = indiv.patchpft();

	double cmass_leaf_inc = cropindiv.grs_cmass_leaf - indiv.cmass_leaf_post_turnover;
	double cmass_root_inc = cropindiv.grs_cmass_root - indiv.cmass_root_post_turnover;

	double grs_npp = cmass_leaf_inc + cmass_root_inc - CMASS_SEED;
	double cmass_leaf_pre_turnover = cropindiv.grs_cmass_leaf;
	double cmass_root_pre_turnover = cropindiv.grs_cmass_root;
	double cton_leaf_bg = indiv.cton_leaf(false);
	double cton_root_bg = indiv.cton_root(false);

	indiv.nstore_longterm += indiv.nstore_labile;
	indiv.nstore_labile = 0.0;

	turnover(indiv.pft.turnover_leaf, indiv.pft.turnover_root,
		indiv.pft.turnover_sap, indiv.pft.lifeform, indiv.pft.landcover,
		indiv.cropindiv->grs_cmass_leaf, indiv.cropindiv->grs_cmass_root, indiv.cmass_sap, indiv.cmass_heart,
		indiv.nmass_leaf, indiv.nmass_root, indiv.nmass_sap, indiv.nmass_heart,
		patchpft.litter_leaf,
		patchpft.litter_root,
		patchpft.nmass_litter_leaf,
		patchpft.nmass_litter_root,
		indiv.nstore_longterm, indiv.max_n_storage,
		true);

	indiv.cmass_leaf_post_turnover = cropindiv.grs_cmass_leaf;
	indiv.cmass_root_post_turnover = cropindiv.grs_cmass_root;

	// Nitrogen longtime storage
	// Nitrogen approx retranslocated next season
	double retransn_nextyear = cmass_leaf_pre_turnover * indiv.pft.turnover_leaf / cton_leaf_bg * nrelocfrac +
		cmass_root_pre_turnover * indiv.pft.turnover_root / cton_root_bg * nrelocfrac;

	// Max longterm nitrogen storage
	indiv.max_n_storage = max(0.0, min(cmass_root_pre_turnover * indiv.pft.fnstorage / cton_leaf_bg, retransn_nextyear));

	// Scale this year productivity to max storage
	if (grs_npp > 0.0) {
		indiv.scale_n_storage = max(indiv.max_n_storage * 0.1, indiv.max_n_storage - retransn_nextyear) * cton_leaf_bg / grs_npp;
	}

	indiv.nstore_labile = indiv.nstore_longterm;
	indiv.nstore_longterm = 0.0;
}

/// Help function used by allocation_crop_nlim(), described in Olin et al. 2015.
void crop_allocation_devries(cropphen_struct& ppftcrop, Individual& indiv) {
	// Currently, the development stage (ds) calculations are done using a linear relationship
	// between ds and fphu to allow for dynamic variety selection (Lindeskog 2013).

	double t = 0.0;
	if(ppftcrop.fphu < 0.4367) {
		t = -0.07 + 2.45 * ppftcrop.fphu;
	}
	else {
		t = 0.2247 + 1.7753 * ppftcrop.fphu;
	}
	// Comment out this line if ds should be calculated according to Olin et al. 2015.
	ppftcrop.dev_stage = max(0.0,min(2.0,t));

	// Eq. 3-5, Olin 2015
	double f1 = min(1.0, max(0.0, richards_curve(indiv.pft.a1, indiv.pft.b1, indiv.pft.c1, indiv.pft.d1, ppftcrop.dev_stage)));
	double f2 = min(1.0, max(0.0, richards_curve(indiv.pft.a2, indiv.pft.b2, indiv.pft.c2, indiv.pft.d2, ppftcrop.dev_stage)));
	double f3 = min(1.0, max(0.0, richards_curve(indiv.pft.a3, indiv.pft.b3, indiv.pft.c3, indiv.pft.d3, ppftcrop.dev_stage)));

	// Eq. 15, Olin 2015
	if(indiv.daily_cmass_leafloss > 0.0)
		f2 *= f2 * f2;

	// Eq. 6, Olin 2015
	ppftcrop.f_alloc_root = f1 * (1-f3);
	ppftcrop.f_alloc_leaf = f2 * (1-f1)*(1-f3);
	ppftcrop.f_alloc_stem = (1.0 - f2)*(1.0 - f1)*(1.0 - f3);
	ppftcrop.f_alloc_horg = f3;
}

/// Daily allocation routine for crops with nitrogen limitation
/** Allocates daily npp to leaf, roots and harvestable organs
 *  according to Olin et al. 2015.
 */
void allocation_crop_nlim(Individual& indiv, double cmass_seed, double nmass_seed) {

	cropindiv_struct& cropindiv = *(indiv.get_cropindiv());
	Patch& patch = indiv.vegetation.patch;
	Patchpft& patchpft = patch.pft[indiv.pft.id];
	cropphen_struct& ppftcrop = *(patchpft.get_cropphen());

	if (!ppftcrop.growingseason) {
		return;
	}

	// report seed fluxes
	indiv.report_flux(Fluxes::SEEDC, -cmass_seed);
	indiv.report_flux(Fluxes::SEEDN, -nmass_seed);

	// add seed carbon
	double cmass_extra = cmass_seed;

	// add seed nitrogen, with the assumption that the seedling is initiated with 50% in roots and 50% in the leaves. 
	indiv.nmass_leaf += nmass_seed / 2.0;
	indiv.nmass_root += nmass_seed / 2.0;

	// Get the daily allocation strategy.
	crop_allocation_devries(ppftcrop, indiv);

	// Use the fast C pool when NPP is negative.
	if(indiv.dnpp < 0.0){

		if(-indiv.dnpp < cropindiv.grs_cmass_agpool) {
			cropindiv.grs_cmass_agpool -= -indiv.dnpp;
			cropindiv.ycmass_agpool -= -indiv.dnpp;
			indiv.dnpp = 0.0;
		}
		else {
			indiv.report_flux(Fluxes::NPP, (-indiv.dnpp - cropindiv.grs_cmass_agpool));
			cropindiv.ycmass_agpool -= cropindiv.grs_cmass_agpool;
			cropindiv.grs_cmass_agpool = 0.0;
			indiv.dnpp = 0.0;
		}

	}

	// Retranslocation from the fast C pool to the grains towards the end of the grainfilling period. 
	// NB, only works for cereals (grasses), needs to be adjusted to work for herb and tuber crops.
	if (cropindiv.grs_cmass_agpool > 0.0 && patchpft.cropphen->f_alloc_horg > 0.95) {
		cmass_extra += 0.1 * cropindiv.grs_cmass_agpool;
		cropindiv.ycmass_agpool -= 0.1 * cropindiv.grs_cmass_agpool;
		cropindiv.grs_cmass_agpool *= 0.9;
	}

	indiv.daily_cmass_rootloss = 0.0;
	indiv.daily_nmass_rootloss = 0.0;

	// If senescense have occured this day.
	if (indiv.daily_cmass_leafloss > 0.0) {

		// Daily C mass leaf increment.
		cropindiv.dcmass_leaf = (indiv.dnpp + cmass_extra) * patchpft.cropphen->f_alloc_leaf - indiv.daily_cmass_leafloss;
		cropindiv.grs_cmass_dead_leaf += indiv.daily_cmass_leafloss;
		cropindiv.ycmass_dead_leaf += indiv.daily_cmass_leafloss;
		if (indiv.daily_cmass_leafloss / 100.0<indiv.nmass_leaf) {
			cropindiv.nmass_dead_leaf += indiv.daily_cmass_leafloss / 100.0; // Fixed low C:N in the dead leaf
			cropindiv.ynmass_dead_leaf += indiv.daily_cmass_leafloss / 100.0;
			indiv.nmass_leaf -= indiv.daily_cmass_leafloss / 100.0;
		}
		double new_CN = (cropindiv.grs_cmass_leaf + cropindiv.dcmass_leaf) / indiv.nmass_leaf;

		// If the result is smaller (higher [N]) than the min C:N then that N is
		// put in to the ag N pool
		if( new_CN < indiv.pft.cton_leaf_min ) {

			indiv.daily_nmass_leafloss = max(0.0, indiv.nmass_leaf - (cropindiv.grs_cmass_leaf + cropindiv.dcmass_leaf) / (1.33 * indiv.pft.cton_leaf_min));
			if(indiv.daily_nmass_leafloss > indiv.nmass_leaf) {
				indiv.daily_nmass_leafloss = 0.0;
			}
		} else {
			indiv.daily_nmass_leafloss = 0.0;
		}

		// Very experimental, root senescence
		// N and C loss when root senescence is allowed (f_HO > 0.5)
		// d3, the DS after which more than half of the daily assimilates are going to the grains.
		if(patchpft.cropphen->dev_stage > indiv.pft.d3) {

			// only have root senescence when leaf scenescence has occured
			if (indiv.daily_nmass_leafloss > 0.0) {
				double kC = 0.0;
				double kN = 0.0;

				// The root senescence is proportional to that of the leaves, Eq. 10 Olin 2015
				if(indiv.nmass_leaf > 0.0) {
					kN = indiv.daily_nmass_leafloss / indiv.nmass_leaf;
				}
				if (indiv.cmass_leaf_today() > 0.0) {
					kC = indiv.daily_cmass_leafloss / indiv.cmass_leaf_today();
				}
				indiv.daily_cmass_rootloss = indiv.cmass_root_today() * kC;
				indiv.daily_nmass_rootloss = indiv.nmass_root * kN;
			}
		}

		indiv.nmass_leaf -= indiv.daily_nmass_leafloss;
		cropindiv.nmass_agpool += indiv.daily_nmass_leafloss;
	}
	else {
		cropindiv.dcmass_leaf = (indiv.dnpp + cmass_extra) * patchpft.cropphen->f_alloc_leaf;
	}
	cropindiv.dcmass_stem = (indiv.dnpp + cmass_extra) * patchpft.cropphen->f_alloc_stem;

	if (indiv.daily_cmass_rootloss > indiv.cmass_root_today())
		indiv.daily_cmass_rootloss = 0.0;

	cropindiv.dcmass_root = (indiv.dnpp + cmass_extra) * patchpft.cropphen->f_alloc_root - indiv.daily_cmass_rootloss;

	// The lost root C is directly put into the litter.
	patch.soil.sompool[SOILMETA].cmass += indiv.daily_cmass_rootloss;

	if (indiv.daily_nmass_rootloss < indiv.nmass_root) {
		indiv.nmass_root -= indiv.daily_nmass_rootloss;

		// 50% of the N in the lost root is retranslocated, the rest is going to litter.
		cropindiv.nmass_agpool += indiv.daily_nmass_rootloss * 0.5;
		patch.soil.sompool[SOILMETA].nmass += indiv.daily_nmass_rootloss * 0.5;
	}
	if (indiv.daily_cmass_rootloss > 0.0){
		patch.is_litter_day = true;
	}

	cropindiv.dcmass_ho = (indiv.dnpp + cmass_extra) * patchpft.cropphen->f_alloc_horg;
	cropindiv.dcmass_plant = cropindiv.dcmass_ho + cropindiv.dcmass_root + cropindiv.dcmass_stem + cropindiv.dcmass_leaf;

	// Add the daily increments to the annual and growing season variables.
	cropindiv.ycmass_leaf += cropindiv.dcmass_leaf;
	cropindiv.ycmass_root += cropindiv.dcmass_root;
	cropindiv.ycmass_ho += cropindiv.dcmass_ho;
	cropindiv.ycmass_plant += cropindiv.dcmass_plant;

	cropindiv.grs_cmass_leaf += cropindiv.dcmass_leaf;

	// 40% of the assimilates that goes to stem is put into the fast C pool (Sec. 2.1.1 Olin 2015)
	cropindiv.grs_cmass_stem += (1.0 - 0.4) * cropindiv.dcmass_stem;
	cropindiv.ycmass_stem += (1.0 - 0.4) * cropindiv.dcmass_stem;
	cropindiv.grs_cmass_agpool += 0.4 * cropindiv.dcmass_stem;
	cropindiv.ycmass_agpool += 0.4 * cropindiv.dcmass_stem;

	cropindiv.grs_cmass_root += cropindiv.dcmass_root;
	cropindiv.grs_cmass_ho += cropindiv.dcmass_ho;
	cropindiv.grs_cmass_plant += cropindiv.dcmass_plant;

	double ndemand_ho = 0.0;
	// The non-structural N that is potentially  available for retranslocation in leaves, roots and stem.
	double avail_leaf_N = max(0.0, (1.0 / indiv.cton_leaf(false) - 1.0 / indiv.pft.cton_leaf_max) * indiv.cmass_leaf_today());
	double avail_root_N = max(0.0, (1.0 / indiv.cton_root(false) - 1.0 / indiv.pft.cton_root_max) * indiv.cmass_root_today());
	double avail_stem_N = max(0.0,cropindiv.nmass_agpool - 1.0 / indiv.pft.cton_stem_max * cropindiv.grs_cmass_stem);
	double avail_N = avail_leaf_N + avail_root_N + avail_stem_N;

	if (avail_N > 0.0 && cropindiv.dcmass_ho > 0.0) {
		ndemand_ho = cropindiv.dcmass_ho / indiv.pft.cton_leaf_avr;
	}
	// N mass to be translocated from leaves and roots
	double trans_leaf_N = 0.0;
	double trans_root_N = 0.0;
	if (ndemand_ho > 0.0) {
		if(avail_stem_N > 0.0) {
			if (ndemand_ho > avail_stem_N) {
				ndemand_ho -= avail_stem_N;
				cropindiv.dnmass_ho += avail_stem_N;
				cropindiv.nmass_agpool -= avail_stem_N;
			}
			else {
				cropindiv.nmass_agpool -= ndemand_ho;
				cropindiv.dnmass_ho += ndemand_ho;
				ndemand_ho = 0.0;
			}
		}
		// "willingness" to let go of the N in the organ to meet the demand from the storage organ, Eq. 17 Olin 2015
		double y0 = (1.0 / indiv.pft.cton_leaf_min + 1.0 / indiv.pft.cton_leaf_avr) / 2.0;
		double y = 1.0 / indiv.cton_leaf(false);
		double y2 = 1.0 / (1.0 * indiv.pft.cton_leaf_max);
		double z = (y0 - y)/(y0 - y2);

		// The leaves willingness to contribute to meet the storage organs N demand.
		double w_l = 1.0 - max(0.0, min(1.0, pow(1.0 - z, 2.0)));
		y0 = 1.0 / indiv.pft.cton_root_avr;
		y = 1.0 / indiv.cton_root(false);
		y2 = 1.0 / (1.0 * indiv.pft.cton_root_max);
		z = (y0 - y) / (y0 - y2);

		// The roots willingness to contribute to meet the storage organs N demand.
		double w_r = 1.0 - max(0.0, min(1.0, pow(1.0 - z, 2.0)));
		double w_s = w_r + w_l;

		// The total willingness to contribute to meet the storage organs N demand.
		double w = min(1.0, w_s);

		// If there is N availble for retranslocation in the roots or leaves
		if(w_s > 0.0) {
			trans_leaf_N = max(0.0, w_l * w * ndemand_ho / w_s);
			trans_root_N = max(0.0, w_r * w * ndemand_ho / w_s);

			// Compare the N the organ is willing to let go of to N available in the organ, based on the C:N.
			if(trans_leaf_N > avail_leaf_N) {
				trans_leaf_N = avail_leaf_N;
			}
			if(trans_root_N > avail_root_N) {
				trans_root_N = avail_root_N;
			}
			// Add the N to the havestable organ
			cropindiv.dnmass_ho += trans_leaf_N;
			cropindiv.dnmass_ho += trans_root_N;
		}
	}
	// Subtract the N that has been added to theharvestable organ from the donor organs
	indiv.nmass_leaf -= trans_leaf_N;
	indiv.nmass_root -= trans_root_N;
	cropindiv.nmass_ho += cropindiv.dnmass_ho;
}

/// Daily allocation routine for crops without nitrogen limitation
/** Allocates daily npp to leaf, roots and harvestable organs
 *  SWAT equations are from Neitsch et al. 2002.
 */
void allocation_crop(Individual& indiv, double cmass_seed, double nmass_seed) {

	cropindiv_struct& cropindiv = *(indiv.get_cropindiv());
	Patch& patch = indiv.vegetation.patch;
	Patchpft& patchpft = patch.pft[indiv.pft.id];
	cropphen_struct& ppftcrop = *(patchpft.get_cropphen());

	nmass_seed = 0.0;	// for compatibility with previous code, doesn't affect crop growth

	// report seed flux
	indiv.report_flux(Fluxes::SEEDC, -cmass_seed);
	indiv.report_flux(Fluxes::SEEDN, -nmass_seed);

	// add seed carbon
	cropindiv.grs_cmass_plant += cmass_seed;
	cropindiv.ycmass_plant += cmass_seed;
	cropindiv.dcmass_plant += cmass_seed;

	// add seed nitrogen
	indiv.nmass_leaf += nmass_seed / 2.0;
	indiv.nmass_root += nmass_seed / 2.0;

	// add today's npp
	cropindiv.dcmass_plant += indiv.dnpp;
	cropindiv.grs_cmass_plant += indiv.dnpp;
	cropindiv.ycmass_plant += indiv.dnpp;

	// allocation to roots
	double froot = indiv.pft.frootstart -(indiv.pft.frootstart - indiv.pft.frootend) * ppftcrop.fphu;	// SWAT 5:2,1,21
	double grs_cmass_root_old = cropindiv.grs_cmass_root;
	cropindiv.grs_cmass_root = froot * cropindiv.grs_cmass_plant;
	cropindiv.dcmass_root = cropindiv.grs_cmass_root - grs_cmass_root_old;
	cropindiv.ycmass_root += cropindiv.dcmass_root;

	// allocation to harvestable organs
	double grs_cmass_ag = (1.0-froot) * cropindiv.grs_cmass_plant;
	double grs_cmass_ho_old = cropindiv.grs_cmass_ho;

	if (indiv.pft.hiopt <= 1.0)
		cropindiv.grs_cmass_ho = ppftcrop.hi * grs_cmass_ag;									// SWAT 5:2.4.2, 5:2.4.4
	else	// below-ground harvestable organs
		cropindiv.grs_cmass_ho = (1.0 - 1.0 / (1.0 + ppftcrop.hi)) * cropindiv.grs_cmass_plant;	// SWAT 5:2.4.3 8

	cropindiv.dcmass_ho = cropindiv.grs_cmass_ho - grs_cmass_ho_old;
	cropindiv.ycmass_ho += cropindiv.dcmass_ho;

	// allocation to leaves
	double grs_cmass_leaf_old = cropindiv.grs_cmass_leaf;
	cropindiv.grs_cmass_leaf = cropindiv.grs_cmass_plant - cropindiv.grs_cmass_root - cropindiv.grs_cmass_ho;

	cropindiv.dcmass_leaf = cropindiv.grs_cmass_leaf - grs_cmass_leaf_old;
	cropindiv.ycmass_leaf += cropindiv.dcmass_leaf;

	// allocation to above-ground pool (currently not used)
	cropindiv.dcmass_agpool = cropindiv.dcmass_plant - cropindiv.dcmass_root - cropindiv.dcmass_leaf - cropindiv.dcmass_ho;
	cropindiv.grs_cmass_agpool = cropindiv.grs_cmass_plant - cropindiv.grs_cmass_root - cropindiv.grs_cmass_leaf - cropindiv.grs_cmass_ho;
	cropindiv.ycmass_agpool = cropindiv.ycmass_plant - cropindiv.ycmass_root - cropindiv.ycmass_leaf - cropindiv.ycmass_ho;

	if (!largerthanzero(cropindiv.grs_cmass_agpool, -9))
		cropindiv.grs_cmass_agpool = 0,0;
	if (!largerthanzero(cropindiv.ycmass_agpool, -9))
		cropindiv.ycmass_agpool = 0,0;
}

/// Daily growth routine for crops
/** Allocates daily npp to leaf, roots and harvestable organs by calling
 *  allocation_crop_nlim() or allocation_crop()
 *  Requires updated value of fphu and hi.
 */
void growth_crop_daily(Patch& patch) {

	if(date.day == 0)
		patch.nharv = 0;
	patch.isharvestday = false;
	double nharv_today = 0;

	Landcover& lc = patch.stand.get_gridcell().landcover;
	Vegetation& vegetation = patch.vegetation;
	vegetation.firstobj();
	while (vegetation.isobj) {

		Individual& indiv = vegetation.getobj();
		cropindiv_struct& cropindiv = *(indiv.get_cropindiv());
		Patchpft& patchpft = patch.pft[indiv.pft.id];
		cropphen_struct& ppftcrop = *(patchpft.get_cropphen());

		if(date.day == 0) {

			cropindiv.ycmass_plant = 0.0;
			cropindiv.ycmass_leaf = 0.0;
			cropindiv.ycmass_root = 0.0;
			cropindiv.ycmass_ho = 0.0;
			cropindiv.ycmass_agpool = 0.0;
			cropindiv.ycmass_dead_leaf = 0.0;
			cropindiv.ycmass_stem = 0.0;

			cropindiv.ynmass_leaf = 0.0;
			cropindiv.ynmass_root = 0.0;
			cropindiv.ynmass_ho = 0.0;
			cropindiv.ynmass_agpool = 0.0;
			cropindiv.ynmass_dead_leaf = 0.0;

			cropindiv.harv_cmass_plant = 0.0;
			cropindiv.harv_cmass_root = 0.0;
			cropindiv.harv_cmass_leaf = 0.0;
			cropindiv.harv_cmass_ho = 0.0;
			cropindiv.harv_cmass_agpool = 0.0;

			cropindiv.cmass_ho_harvest[0] = 0.0;
			cropindiv.cmass_ho_harvest[1] = 0.0;

			cropindiv.cmass_leaf_max = 0.0;

			if(indiv.pft.phenology == ANY && !cropindiv.isintercropgrass) {		// zero of normal cc3g/cc4g cmass arbitrarily at new year

				cropindiv.grs_cmass_plant = 0.0;
				cropindiv.grs_cmass_root = 0.0;
				cropindiv.grs_cmass_ho = 0.0;
				cropindiv.grs_cmass_leaf = 0.0;
				cropindiv.grs_cmass_agpool = 0.0;
			}
		}

		cropindiv.dcmass_plant = 0.0;
		cropindiv.dcmass_leaf = 0.0;
		cropindiv.dcmass_root = 0.0;
		cropindiv.dcmass_ho = 0.0;
		cropindiv.dcmass_agpool = 0.0;
		cropindiv.dcmass_stem = 0.0;
		cropindiv.dnmass_ho = 0.0;

		// true crop allocation
		if(indiv.pft.phenology == CROPGREEN) {

			if(ppftcrop.growingseason) {

				double cmass_seed = 0.0;
				double nmass_seed = 0.0;

				if (DELAYED_SEEDCARBON) {
					// Seed carbon; portion the seed carbon over a 10-day period.
					if(dayinperiod(date.day, ppftcrop.sdate, stepfromdate(ppftcrop.sdate, 9))) {			

						cmass_seed = 0.1 * CMASS_SEED;
						nmass_seed = 0.1 * CMASS_SEED / indiv.pft.cton_leaf_min;
					}
				} else {
					// add seed carbon on sowing date
					if(date.day == ppftcrop.sdate) {

						cmass_seed = CMASS_SEED;
						nmass_seed = CMASS_SEED / indiv.pft.cton_leaf_min;
					}
				}

				if(ifnlim)
					allocation_crop_nlim(indiv, cmass_seed, nmass_seed);
				else
					allocation_crop(indiv, cmass_seed, nmass_seed);

				// save this year's maximum leaf carbon mass
				if(cropindiv.grs_cmass_leaf > cropindiv.cmass_leaf_max)
					cropindiv.cmass_leaf_max = cropindiv.grs_cmass_leaf;

				// save leaf carbon mass at the beginning of senescence
				if(date.day == ppftcrop.sendate)
					cropindiv.cmass_leaf_sen = cropindiv.grs_cmass_leaf;

				// Check that no plant cmass or nmass is negative, if so, and correct fluxes
				double negative_cmass = indiv.check_C_mass();
				if(largerthanzero(negative_cmass, -14))
					dprintf("Year %d day %d Stand %d indiv %d: Negative main crop C mass in growth_crop_daily: %.15f\n", date.year, date.day, indiv.vegetation.patch.stand.id, indiv.id, -negative_cmass);
				double negative_nmass = indiv.check_N_mass();
				if(largerthanzero(negative_nmass, -14))
					dprintf("Year %d day %d Stand %d indiv %d: Negative main crop N mass in growth_crop_daily: %.15f\n", date.year, date.day, indiv.vegetation.patch.stand.id, indiv.id, -negative_nmass);
			}
			else if(date.day == ppftcrop.hdate) {

				patch.stand.isrotationday = true;

				cropindiv.harv_cmass_plant += cropindiv.grs_cmass_plant;
				cropindiv.harv_cmass_root += cropindiv.grs_cmass_root;
				cropindiv.harv_cmass_ho += cropindiv.grs_cmass_ho;
				cropindiv.harv_cmass_leaf += cropindiv.grs_cmass_leaf;
				cropindiv.harv_cmass_agpool += cropindiv.grs_cmass_agpool;
				cropindiv.harv_cmass_stem += cropindiv.grs_cmass_stem;

				cropindiv.harv_nmass_root += indiv.nmass_root;
				cropindiv.harv_nmass_ho += cropindiv.nmass_ho;
				cropindiv.harv_nmass_leaf += indiv.nmass_leaf;
				cropindiv.harv_nmass_agpool += cropindiv.nmass_agpool;
				// dead_leaf to be addad

				if(ppftcrop.nharv == 1)
					cropindiv.cmass_ho_harvest[0] = cropindiv.grs_cmass_ho;
				else if(ppftcrop.nharv == 2)
					cropindiv.cmass_ho_harvest[1] = cropindiv.grs_cmass_ho;

				patch.isharvestday = true;

				if(indiv.has_daily_turnover()) {
					if(lc.updated && patchpft.cropphen->nharv == 1)
						scale_indiv(indiv, true);
					harvest_crop(indiv, indiv.pft, indiv.alive, indiv.cropindiv->isintercropgrass, true);
					patch.is_litter_day = true;
				}
				patch.nharv++;

				cropindiv.grs_cmass_plant = 0.0;
				cropindiv.grs_cmass_root = 0.0;
				cropindiv.grs_cmass_ho = 0.0;
				cropindiv.grs_cmass_leaf = 0.0;
				cropindiv.grs_cmass_agpool = 0.0;
				cropindiv.cmass_leaf_sen = 0.0;

				cropindiv.grs_cmass_stem = 0.0;
				cropindiv.grs_cmass_dead_leaf = 0.0;
				cropindiv.nmass_dead_leaf = 0.0;
				cropindiv.nmass_agpool = 0.0;
				indiv.nmass_leaf = 0.0;
				indiv.nmass_root = 0.0;
				cropindiv.nmass_ho = 0.0;
			}
		}
		// crop grass allocation
		// NB: Only intercrop grass enters here ! normal cc3g/cc4g grass is treated just like natural grass
		else if(indiv.pft.phenology == ANY && indiv.pft.id != patch.stand.pftid) {

			if(ppftcrop.growingseason) {

				cropindiv.dcmass_plant = 0.0;

				// add seed carbon
				if(!indiv.continous_grass() && date.day == patch.pft[patch.stand.pftid].cropphen->bicdate
					|| indiv.continous_grass() && date.day == stepfromdate(indiv.last_turnover_day, 1)) {

					double cmass_seed = CMASS_SEED;
					cropindiv.grs_cmass_plant += cmass_seed;
					cropindiv.ycmass_plant += cmass_seed;
					cropindiv.dcmass_plant += cmass_seed;
					double nmass_seed = cmass_seed / indiv.pft.cton_leaf_min;
					indiv.nmass_leaf += nmass_seed / 2.0;
					indiv.nmass_root += nmass_seed / 2.0;

					indiv.report_flux(Fluxes::SEEDC, -cmass_seed);
					indiv.report_flux(Fluxes::SEEDN, -nmass_seed);

					indiv.last_turnover_day = -1;
				}

				cropindiv.dcmass_plant += indiv.dnpp;
				cropindiv.grs_cmass_plant += indiv.dnpp;
				cropindiv.ycmass_plant += indiv.dnpp;

				indiv.ltor = indiv.wscal_mean() * indiv.pft.ltor_max;

				// allocation to roots
				double froot = 1.0 / (1.0 + indiv.ltor);
				double grs_cmass_root_old = cropindiv.grs_cmass_root;

				// Cumulative wscal-dependent root increase
				cropindiv.grs_cmass_root = froot * cropindiv.grs_cmass_plant;
				cropindiv.dcmass_root = cropindiv.grs_cmass_root - grs_cmass_root_old;
				cropindiv.ycmass_root += cropindiv.dcmass_root;

				// allocation to leaves
				double grs_cmass_leaf_old = cropindiv.grs_cmass_leaf;
				cropindiv.grs_cmass_leaf = cropindiv.grs_cmass_plant - cropindiv.grs_cmass_root;
				cropindiv.dcmass_leaf = cropindiv.grs_cmass_leaf - grs_cmass_leaf_old;
				cropindiv.ycmass_leaf += cropindiv.dcmass_leaf;

				// Check that no plant cmass is negative, if so, zero cmass and correct C fluxes
				double negative_cmass = indiv.check_C_mass();

				// save this year's maximum leaf carbon mass
				if(cropindiv.grs_cmass_leaf > cropindiv.cmass_leaf_max)
					cropindiv.cmass_leaf_max = cropindiv.grs_cmass_leaf;
			}
			else if(date.day == patch.pft[patch.stand.pftid].get_cropphen()->eicdate) {

				cropindiv.harv_cmass_plant += cropindiv.grs_cmass_plant;
				cropindiv.harv_cmass_root += cropindiv.grs_cmass_root;
				cropindiv.harv_cmass_leaf += cropindiv.grs_cmass_leaf;
				cropindiv.harv_cmass_ho += cropindiv.grs_cmass_ho;
				cropindiv.harv_cmass_agpool += cropindiv.grs_cmass_agpool;

				cropindiv.harv_nmass_root += indiv.nmass_root;
				cropindiv.harv_nmass_ho += cropindiv.nmass_ho;
				cropindiv.harv_nmass_leaf += indiv.nmass_leaf;
				cropindiv.harv_nmass_agpool += cropindiv.nmass_agpool;

				ppftcrop.nharv++;
				patch.isharvestday = true;
				nharv_today++;
				if(nharv_today > 1)	// In case of both C3 and C4 growing
					patch.nharv--;

				if(indiv.has_daily_turnover()) {
					if(lc.updated && patchpft.cropphen->nharv == 1)
						scale_indiv(indiv, true);
					harvest_crop(indiv, indiv.pft, indiv.alive, indiv.cropindiv->isintercropgrass, true);
					patch.is_litter_day = true;
				}
				else {
					cropindiv.grs_cmass_root = 0.0;
					cropindiv.grs_cmass_ho = 0.0;
					cropindiv.grs_cmass_leaf = 0.0;
					cropindiv.grs_cmass_agpool = 0.0;
				}
				patch.nharv++;

				cropindiv.grs_cmass_plant = cropindiv.grs_cmass_root + cropindiv.grs_cmass_leaf;
			}

			if(indiv.continous_grass() && indiv.is_turnover_day()
				|| ppftcrop.growingdays > date.year_length() && (indiv.is_turnover_day() || date.islastmonth && date.islastday)) {

				indiv.last_turnover_day = date.day;
				ppftcrop.growingdays = 0;

				cropindiv.harv_cmass_plant += cropindiv.grs_cmass_plant;
				cropindiv.harv_cmass_root += cropindiv.grs_cmass_root;
				cropindiv.harv_cmass_leaf += cropindiv.grs_cmass_leaf;
				cropindiv.harv_cmass_ho += cropindiv.grs_cmass_ho;
				cropindiv.harv_cmass_agpool += cropindiv.grs_cmass_agpool;

				cropindiv.harv_nmass_root += indiv.nmass_root;
				cropindiv.harv_nmass_ho += cropindiv.nmass_ho;
				cropindiv.harv_nmass_leaf += indiv.nmass_leaf;
				cropindiv.harv_nmass_agpool += cropindiv.nmass_agpool;

				ppftcrop.nharv++;
				patch.isharvestday = true;
				nharv_today++;
				if(nharv_today > 1)	// In case of both C3 and C4 growing
					patch.nharv--;

				if(indiv.has_daily_turnover()) {
					if(lc.updated && patchpft.cropphen->nharv == 1)
						scale_indiv(indiv, true);

					turnover_grass(indiv);
					patch.is_litter_day = true;
				}
				else {
					cropindiv.grs_cmass_root = 0.0;
					cropindiv.grs_cmass_ho = 0.0;
					cropindiv.grs_cmass_leaf = 0.0;
				}
				patch.nharv++;

				cropindiv.grs_cmass_plant = cropindiv.grs_cmass_root + cropindiv.grs_cmass_leaf;
				cropindiv.grs_cmass_agpool = 0.0;
			}
		}
		vegetation.nextobj();
	}
}

/// Handles daily crop allocation and daily lai calculation
/** Simple allocation based on heat unit accumulation.
 *  LAI is set directly after allocation from leaf carbon mass.
 */
void growth_daily(Patch& patch) {

	if(patch.stand.landcover == CROPLAND) {

		// allocate daily npp to leaf, roots and harvestable organs
		growth_crop_daily(patch);

		// update patchpft.lai_daily and fpc_daily
		lai_crop(patch);
	}
}

/// Handles yearly cropland lai and fpc calculation, called from allometry()
/** Uses cmass_leaf_max for true crops and lai from whole-year grass stands for
 *  cover-crop grass if present (NB. grass pft names must be as described in code !).
 *  If grass pft not present or found in other stands, pft laimax is used.
 */
void allometry_crop(Individual& indiv) {
 
	// crop grass compatible with natural grass
	if(indiv.pft.phenology == ANY) {

		indiv.lai_indiv = indiv.cmass_leaf * indiv.pft.sla;

		// For intercrop grass, use LAI of parent grass in its own stand.
		if(indiv.cropindiv->isintercropgrass) {

			bool done = false;

			Gridcell& gridcell = indiv.vegetation.patch.stand.get_gridcell();

			// First look in PASTURE.
			if(gridcell.landcover.frac[PASTURE] > 0.0) {

				for(unsigned int i = 0; i < gridcell.size() && !done; i++) {

					Stand& stand=gridcell[i];
					if(stand.landcover == PASTURE) {
						for(unsigned int j = 0; j < stand.nobj && !done; j++) {
							Patch& patch = stand[j];
							Vegetation& vegetation = patch.vegetation;
							for(unsigned int k = 0; k < vegetation.nobj && !done; k++) {
								Individual& grass_indiv = vegetation[k];

								if(grass_indiv.pft.phenology == ANY && grass_indiv.pft.pathway == indiv.pft.pathway) {
									indiv.lai_indiv = grass_indiv.lai_indiv;
									done = true;
								}
							}
						}
					}
				}
			}
			// If PASTURE landcover not used, look for crop stand with pasture grass.
			else {

				double highest_grass_lai = 0.0;
				double grass_cmass_leaf_sum = 0.0;

				// Get sum of intercrop grass cmass_leaf in this patch
				Vegetation& vegetation_self = indiv.vegetation;

				for(unsigned int k = 0; k < vegetation_self.nobj; k++) {

					Individual& indiv_veg = vegetation_self[k];

					if(indiv_veg.cropindiv->isintercropgrass)
						grass_cmass_leaf_sum += indiv_veg.cropindiv->cmass_leaf_max;
				}

				// Find highest lai in crop grass stands
				for(unsigned int i = 0; i < gridcell.size(); i++) {

					Stand& stand = gridcell[i];

					if(stand.landcover == CROPLAND && !stand.is_true_crop_stand())	{

						for(unsigned int j = 0; j < stand.nobj && !done; j++) {

							Patch& patch = stand[j];
							Vegetation& vegetation = patch.vegetation;

							for(unsigned int k = 0; k < vegetation.nobj && !done; k++) {

								Individual& grass_indiv = vegetation[k];

								if(grass_indiv.pft.phenology == ANY) {

									if(grass_indiv.lai_indiv > highest_grass_lai)
										highest_grass_lai = grass_indiv.lai_indiv;
								}
							}
						}
					}
				}

				if(grass_cmass_leaf_sum > 0.0)
					indiv.lai_indiv = indiv.cropindiv->cmass_leaf_max / grass_cmass_leaf_sum * highest_grass_lai;
				else
					indiv.lai_indiv = highest_grass_lai;

				if(highest_grass_lai)
					done = true;
			}

			// If no grass stand found in either cropland or pasture, use laimax value.
			if(!done) {

				double highest_grass_lai = 0.0;
				double grass_cmass_leaf_sum = 0.0;

				// Get sum of intercrop grass cmass_leaf and highest default laimax in this patch
				Vegetation& vegetation_self = indiv.vegetation;

				for(unsigned int k = 0; k < vegetation_self.nobj; k++) {

					Individual& indiv_veg = vegetation_self[k];

					if(indiv_veg.cropindiv->isintercropgrass) {

						grass_cmass_leaf_sum += indiv_veg.cropindiv->cmass_leaf_max;

						if(indiv_veg.pft.laimax > highest_grass_lai)
							highest_grass_lai = indiv_veg.pft.laimax;
					}
				}

				if(grass_cmass_leaf_sum > 0.0)
					indiv.lai_indiv = indiv.cropindiv->cmass_leaf_max / grass_cmass_leaf_sum * highest_grass_lai;
				else
					indiv.lai_indiv = highest_grass_lai;
			}
		}
		if(indiv.lai_indiv < 0.0)
			fail("lai_indiv negative for %s in stand %d year %d in growth: %f\n", 
				(char*)indiv.pft.name, indiv.vegetation.patch.stand.id, date.year, indiv.lai_indiv);

		// FPC (Eqn 10)
		indiv.fpc = 1.0 - lambertbeer(indiv.lai_indiv);

		// Stand-level LAI
		indiv.lai = indiv.lai_indiv;

	}
	else {	// cropgreen
		if (!negligible(indiv.cropindiv->cmass_leaf_max)) {

			// Grass "individual" LAI (Eqn 11)
			indiv.lai_indiv = indiv.cropindiv->cmass_leaf_max * indiv.pft.sla;

			// FPC (Eqn 10)
//			indiv.fpc = 1.0 - lambertbeer(indiv.lai_indiv);
			indiv.fpc = 1.0;

			// Stand-level LAI
			indiv.lai = indiv.lai_indiv;

		}
	}
}

/// Transfer of this year's growth (ycmass_xxx) to cmass_xxx_inc
/**   and pasture grass grown in cropland.
 *  OUTPUT PARAMETERS
 *  \param cmass_leaf_inc				leaf C biomass (kgC/m2)
 *  \param cmass_root_inc				fine root C biomass (kgC/m2)
 *  \param cmass_ho_inc					harvestable organ C biomass (kgC/m2)
 *  \param cmass_agpool_inc 			above-ground pool C biomass (kgC/m2)
 *  \param cmass_stem_inc 				stem C biomass (kgC/m2)
 */
void growth_crop_year(Individual& indiv, double& cmass_leaf_inc, double& cmass_root_inc, double& cmass_ho_inc, double& cmass_agpool_inc, double& cmass_stem_inc) {

	// true crop growth and grass intercrop growth; NB: bminit (cmass_repr & cmass_excess subtracted) not used !

	if(indiv.has_daily_turnover()) {

		indiv.cmass_leaf = 0.0;
		indiv.cmass_root = 0.0;
		indiv.cropindiv->cmass_ho = 0.0;
		indiv.cropindiv->cmass_agpool = 0.0;
		indiv.cropindiv->cmass_stem = 0.0;

		// Not completely accurate here when comparing this year's cmass after turnover with cmass increase (ycmass),
		// which could be from the preceding season, but probably OK, since values are not used for C balance.
		if(indiv.continous_grass()) {
			indiv.cmass_leaf = indiv.cmass_leaf_post_turnover;
			indiv.cmass_root = indiv.cmass_root_post_turnover;
		}
	}

	cmass_leaf_inc = indiv.cropindiv->ycmass_leaf + indiv.cropindiv->ycmass_dead_leaf;
	cmass_root_inc = indiv.cropindiv->ycmass_root;
	cmass_ho_inc = indiv.cropindiv->ycmass_ho;
	cmass_agpool_inc = indiv.cropindiv->ycmass_agpool;
	cmass_stem_inc = indiv.cropindiv->ycmass_stem;

	return;
}


//////////////////////////////////////////////////////////////////////////////////////////
// REFERENCES
//
// Neitsch SL, Arnold JG, Kiniry JR et al.2002 Soil and Water Assessment Tool, Theorethical
//   Documentation + User's Manual. USDA_ARS-SR Grassland, Soil and Water Research Laboratory.
//   Agricultural Reasearch Service, Temple,Tx, US.
// S. Olin, G. Schurgers, M. Lindeskog, D. W�rlind, B. Smith, P. Bodin, J. Holm�r, and A. Arneth. 2015
//   Biogeosciences 12, 2489-2515. Modelling the response of yields and tissue C:N to changes in
//   atmospheric CO2 and N management in the main wheat regions of western Europe
   
