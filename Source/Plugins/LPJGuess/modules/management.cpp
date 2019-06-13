////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file management.cpp
/// \brief Harvest functions for cropland, managed forest and pasture
/// \author Mats Lindeskog
/// $Date:  $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "landcover.h"
#include "management.h"

/// Harvest function used for managed forest and for clearing natural vegetation at land use change
/** A fraction of trees is cut down (frac_cut)
 *  A fraction of wood is harvested (pft.harv_eff) and returned as acflux_harvest
 *  A fraction of harvested wood (pft.harvest_slow_frac) is returned as harvested_products_slow
 *  The rest, including leaves and roots, is returned as litter, unless a fraction of twigs or roots removed.
 *  Called from landcover_dynamics() first day of the year if any natural vegetation is transferred to another land use.
 *  INPUT PARAMETER
 *  \param frac_cut					fraction of trees cut
 *  \param harv_eff					harvest efficiency
 *  \param res_outtake_twig			removed twig fraction
 *  \param res_outtake_coarse_root	removed course root fraction
 *  INPUT/OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following indiv-specific public members:
 *   - cmass_leaf 					leaf C biomass (kgC/m2)
 *   - cmass_root					fine root C biomass (kgC/m2)
 *   - cmass_sap					sapwood C biomass (kgC/m2)
 *   - cmass_heart   				heartwood C biomass (kgC/m2)
 *   - cmass_debt					C "debt" (retrospective storage) (kgC/m2)
 *   - nmass_leaf 					leaf nitrogen biomass (kgN/m2)
 *   - nmass_root 					fine root nitrogen biomass (kgN/m2)
 *   - nmass_sap   					sapwood nitrogen biomass (kgC/m2)
 *   - nmass_heart    				heartwood nitrogen biomass (kgC/m2)
 *   - nstore_labile    			labile nitrogen storage (kgC/m2)
 *   - nstore_longterm    			longterm nitrogen storage (kgC/m2)
 *  OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following patchpft-specific public members:
 *   - litter_leaf    				new leaf C litter (kgC/m2)
 *   - litter_root 					new root C litter (kgC/m2)
 *   - litter_sap   				new sapwood C litter (kgC/m2)
 *   - litter_heart   				new heartwood C litter (kgC/m2)
 *   - nmass_litter_leaf 			new leaf nitrogen litter (kgN/m2)
 *   - nmass_litter_root			new root nitrogen litter (kgN/m2)
 *   - nmass_litter_sap 			new sapwood nitrogen litter (kgN/m2)
 *   - nmass_litter_heart        	new heartwood nitrogen litter (kgN/m2)
 *									,and the following patch-level public members:
 *   - acflux_harvest				harvest flux to atmosphere (kgC/m2)
 *   - harvested_products_slow		harvest products to slow pool (kgC/m2)
 *   - anflux_harvest   			harvest nitrogen flux out of system (kgC/m2)
 *   - harvested_products_slow_nmass harvest nitrogen products to slow pool (kgC/m2)
 */
void harvest_wood(Harvest_CN& i, Pft& pft, bool alive, double frac_cut, double harv_eff, double res_outtake_twig, double res_outtake_coarse_root) {

	double harvest = 0.0;
	double residue_outtake = 0.0;
	/// Fraction of wood cmass that are stems
	double stem_frac = 0.65;	// Temporary values, should be pft-specific
	/// Fraction of wood cmass that are twigs
	double twig_frac = 0.13;
	/// Fraction of wood cmass that are coarse roots
	double coarse_root_frac = 1.0 - stem_frac - twig_frac;	// 0.22 with default stem_frac and twig_frac values
	/// Fraction of leaves adhering to twigs at the time of removal
	double adhering_leaf_frac = 0.75;

	// only harvest trees
	if (pft.lifeform == GRASS)
		return;

	// all root carbon and nitrogen goes to litter
	if (alive) {

		i.litter_root += i.cmass_root * frac_cut;
		i.cmass_root *= (1.0 - frac_cut);
	}

	i.nmass_litter_root += i.nmass_root * frac_cut;
	i.nmass_litter_root += (i.nstore_labile + i.nstore_longterm) * frac_cut;
	i.nmass_root *= (1.0 - frac_cut);
	i.nstore_labile *= (1.0 - frac_cut);
	i.nstore_longterm *= (1.0 - frac_cut);

	if (alive) {

		// Carbon:

		if (i.cmass_debt <= i.cmass_sap + i.cmass_heart) {

			// harvested stem wood
			harvest += harv_eff * stem_frac * (i.cmass_sap + i.cmass_heart - i.cmass_debt) * frac_cut;

			// harvested products not consumed (oxidised) this year put into harvested_products_slow
			if (ifslowharvestpool) {
				i.harvested_products_slow += harvest * pft.harvest_slow_frac;
				harvest = harvest * (1.0 - pft.harvest_slow_frac);
			}

			// harvested products consumed (oxidised) this year put into acflux_harvest
			i.acflux_harvest += harvest;

			// removed leaves adhering to twigs
			residue_outtake += res_outtake_twig * adhering_leaf_frac * i.cmass_leaf * frac_cut;

			// removed twigs
			residue_outtake += res_outtake_twig * twig_frac * (i.cmass_sap + i.cmass_heart - i.cmass_debt) * frac_cut;

			// removed coarse roots
			residue_outtake += res_outtake_coarse_root * coarse_root_frac * (i.cmass_sap + i.cmass_heart - i.cmass_debt) * frac_cut;

			// removed residues are oxidised
			i.acflux_harvest += residue_outtake;

			// not removed residues are put into litter
			i.litter_leaf += i.cmass_leaf * (1.0 - res_outtake_twig * adhering_leaf_frac) * frac_cut;

			double to_partition_sap   = 0.0;
			double to_partition_heart = 0.0;

			if (i.cmass_heart >= i.cmass_debt) {
				to_partition_sap   = i.cmass_sap;
				to_partition_heart = i.cmass_heart - i.cmass_debt;
			}
			else {
				to_partition_sap   = i.cmass_sap + i.cmass_heart - i.cmass_debt;
//				dprintf("ATTENTION: pft %s: cmass_debt > cmass_heart; difference=%f\n", (char*)pft.name, i.cmass_debt-i.cmass_heart);
			}
			i.litter_sap += to_partition_sap * (1.0 - res_outtake_twig * twig_frac - res_outtake_coarse_root * coarse_root_frac - harv_eff * stem_frac) * frac_cut;
			i.litter_heart += to_partition_heart * (1.0 - res_outtake_twig * twig_frac - res_outtake_coarse_root * coarse_root_frac - harv_eff * stem_frac) * frac_cut;
		}
		// debt larger than existing wood biomass
		else {
			double debt_excess = i.cmass_debt - (i.cmass_sap + i.cmass_heart);
			dprintf("ATTENTION: cmass_debt > i.cmass_sap + i.cmass_heart; debt_excess=%f\n", debt_excess);
//			i.debt_excess += debt_excess * frac_cut;	// debt_excess currently not dealt with during wood harvest
		}

		// unharvested trees:
		i.cmass_leaf *= (1.0 - frac_cut);
		i.cmass_sap *= (1.0 - frac_cut);
		i.cmass_heart *= (1.0 - frac_cut);
		i.cmass_debt *= (1.0 - frac_cut);

		//Nitrogen:

		harvest = 0.0;

		// harvested products
		harvest += harv_eff * stem_frac * (i.nmass_sap + i.nmass_heart) * frac_cut;

		// harvested products not consumed this year put into harvested_products_slow_nmass
		if (ifslowharvestpool) {
			i.harvested_products_slow_nmass += harvest * pft.harvest_slow_frac;
			harvest = harvest * (1.0 - pft.harvest_slow_frac);
		}

		// harvested products consumed this year put into anflux_harvest
		i.anflux_harvest += harvest;

		residue_outtake = 0.0;

		// removed leaves adhering to twigs
		residue_outtake += res_outtake_twig * adhering_leaf_frac * i.nmass_leaf * frac_cut;

		// removed twigs
		residue_outtake += res_outtake_twig * twig_frac * (i.nmass_sap + i.nmass_heart) * frac_cut;

		// removed coarse roots
		residue_outtake += res_outtake_coarse_root * coarse_root_frac * (i.nmass_sap + i.nmass_heart) * frac_cut;

		// removed residues are oxidised
		i.anflux_harvest += residue_outtake;

		// not removed residues are put into litter
		i.nmass_litter_leaf += i.nmass_leaf * (1.0 - res_outtake_twig * adhering_leaf_frac) * frac_cut;
		i.nmass_litter_sap += i.nmass_sap * (1.0 - res_outtake_twig * twig_frac - res_outtake_coarse_root * coarse_root_frac - harv_eff * stem_frac) * frac_cut;
		i.nmass_litter_heart += i.nmass_heart * (1.0 - res_outtake_twig * twig_frac - res_outtake_coarse_root * coarse_root_frac - harv_eff * stem_frac) * frac_cut;

		// unharvested trees:
		i.nmass_leaf *= (1.0 - frac_cut);
		i.nmass_sap *= (1.0 - frac_cut);
		i.nmass_heart *= (1.0 - frac_cut);
	}
}

/// Harvest function used for managed forest and for clearing natural vegetation at land use change
/** A fraction of trees is cut down (frac_cut)
 *  A fraction of wood is harvested (pft.harv_eff) and returned as acflux_harvest
 *  A fraction of harvested wood (pft.harvest_slow_frac) is returned as harvested_products_slow
 *  The rest, including leaves and roots, is returned as litter.
 *  Called from landcover_dynamics() first day of the year if any natural vegetation is transferred to another land use.
 *
 *  This function copies variables from an individual and it's associated patchpft and patch to
 *  a Harvest_CN struct, which is then passed on to the main harvest_crop function.
 *  After the execution of the main harvest_crop function, the output variables are copied
 *  back to the individual and patchpft and the patch-level fluxes are updated.
 *
 *  INPUT PARAMETER
 *  \param frac_cut					fraction of trees cut
 *  \param harv_eff					harvest efficiency
 *  \param res_outtake_twig			removed twig fraction
 *  \param res_outtake_coarse_root	removed course root fraction
 *  \param lc_change				whether to save harvest in gridcell-level luc variable
 *  INPUT/OUTPUT PARAMETERS
 *  \param indiv					reference to an Individual containing the following indiv-specific public members:
 *   - cmass_leaf 					leaf C biomass (kgC/m2)
 *   - cmass_root					fine root C biomass (kgC/m2)
 *   - cmass_sap					sapwood C biomass (kgC/m2)
 *   - cmass_heart   				heartwood C biomass (kgC/m2)
 *   - cmass_debt					C "debt" (retrospective storage) (kgC/m2)
 *   - nmass_leaf 					leaf nitrogen biomass (kgN/m2)
 *   - nmass_root 					fine root nitrogen biomass (kgN/m2)
 *   - nmass_sap   					sapwood nitrogen biomass (kgC/m2)
 *   - nmass_heart    				heartwood nitrogen biomass (kgC/m2)
 *   - nstore_labile    			labile nitrogen storage (kgC/m2)
 *   - nstore_longterm    			longterm nitrogen storage (kgC/m2)
 *  OUTPUT PARAMETERS
 *  \param indiv					reference to an Individual containing the following patchpft-specific public members:
 *   - litter_leaf    				new leaf C litter (kgC/m2)
 *   - litter_root 					new root C litter (kgC/m2)
 *   - litter_sap   				new sapwood C litter (kgC/m2)
 *   - litter_heart   				new heartwood C litter (kgC/m2)
 *   - nmass_litter_leaf 			new leaf nitrogen litter (kgN/m2)
 *   - nmass_litter_root			new root nitrogen litter (kgN/m2)
 *   - nmass_litter_sap 			new sapwood nitrogen litter (kgN/m2)
 *   - nmass_litter_heart        	new heartwood nitrogen litter (kgN/m2)
 *									,and the following patch-level public members:
 *   - acflux_harvest				harvest flux to atmosphere (kgC/m2)
 *   - harvested_products_slow		harvest products to slow pool (kgC/m2)
 *   - anflux_harvest   			harvest nitrogen flux out of system (kgC/m2)
 *   - harvested_products_slow_nmass harvest nitrogen products to slow pool (kgC/m2)
 */
void harvest_wood(Individual& indiv, double frac_cut, double harv_eff, double res_outtake_twig, 
		double res_outtake_coarse_root, bool lc_change) {

	Harvest_CN indiv_cp;

	indiv_cp.copy_from_indiv(indiv);

	harvest_wood(indiv_cp, indiv.pft, indiv.alive, frac_cut, harv_eff, res_outtake_twig, res_outtake_coarse_root);

	indiv_cp.copy_to_indiv(indiv, false, lc_change);

	if (!lc_change) {
		return;
	}

	Stand& stand = indiv.vegetation.patch.stand;
	Landcover& lc = stand.get_gridcell().landcover;
	lc.acflux_landuse_change += stand.get_gridcell_fraction() * indiv_cp.acflux_harvest / (double)stand.nobj;
	lc.acflux_landuse_change_lc[stand.origin] += stand.get_gridcell_fraction() * indiv_cp.acflux_harvest / (double)stand.nobj;
	lc.anflux_landuse_change += stand.get_gridcell_fraction() * indiv_cp.anflux_harvest / (double)stand.nobj;
	lc.anflux_landuse_change_lc[stand.origin] += stand.get_gridcell_fraction() * indiv_cp.anflux_harvest / (double)stand.nobj;
}

/// Use for normal forest management in calls from growth(). For clearcut during landcover change, use harvest_wood() and kill_remaining_vegetation()
void clearcut(Individual& indiv, double anpp, bool& killed) {

	Patch& patch = indiv.vegetation.patch;
	Patchpft& ppft = patch.pft[indiv.pft.id];

	if (indiv.pft.lifeform == TREE) {

		if(indiv.alive)
			ppft.litter_sap += anpp;
		harvest_wood(indiv, 1.0, indiv.pft.harv_eff, indiv.pft.res_outtake); // frac_cut=1, harv_eff=pft.harv_eff, res_outtake_twig=pft.res_outtake, res_outtake_coarse_root=0
		indiv.kill();
		indiv.vegetation.killobj();
		killed = true;
	}

	patch.age = 0;	//important for results
	patch.managed = true;
	patch.plant_this_year = true;
}

// The following two functions are simplified adaptations (continous cutting) from Swedish forest management code by Fredrik Lagergren and should be
// developed further. Specifically, the productivity values, which should ideally be observed values for each gridcell, are set to a static value.
// Also, the calculated diameter limits and rotation times (which are dependent on productivity) are for Swedish forests.

/// Determines whether this patch should be cut this year.
double cut_fraction(Patch& patch) {

	if(!run_landcover)
		return 0.0;

	Stand& stand = patch.stand;
	xtring harvest_system = stlist[stand.stid].get_management(stand.current_rot).harvest_system;
	if(harvest_system == "")
		return 0.0;

	int first_cutyear = nyear_spinup; // Simulation year when forestry harvesting starts; default is directly after spinup.
	if(stlist[stand.stid].firstmanageyear < 100000)	// Initialised to 100000; other values set in instruction file.
		first_cutyear = stlist[stand.stid].firstmanageyear - date.first_calendar_year;

	if(date.year < first_cutyear)
		return 0.0;

	const double minbon = 2.351;	// The minimum average "bonitet" for a county in Sweden
	const double maxbon = 11.311;	// The maximum average "bonitet" for a county in Sweden
	const double bonitet = 10.0;	// Temporary static value (gives cut_int=17)
	double cut_fraction = 0.0;

	if(harvest_system == "CLEARCUT") {
		// First attempt to calculate optimum rotation age for clearcut 
		if(patch.cmass_wood() / patch.age > patch.get_cmass_wood_inc_5() && patch.age > 20)
			cut_fraction = 1.0;
	}
	else if(harvest_system == "CONTINUOUS") {

		// Continuous forestry
		int cut_int; //Interval between cuttings
		int patch_order; //Which year in a cutting interval the patch belongs to

//		cut_int=30-(int)(15.0*(stand.bonitet-minbon)/(maxbon-minbon));
		cut_int=30-(int)(15.0*(bonitet-minbon)/(maxbon-minbon));
		patch_order = (int)(patch.id * cut_int * 1.0 / (1.0 * stand.npatch()));

		if (!((date.year - first_cutyear - patch_order) % cut_int)) // rule needs to be corrected
			cut_fraction = 0.40;
	}

	return cut_fraction;
}

/// Determines if and how much of this (average) individual is to be cut.
/*	Individual is cut by a fraction, determined by cut_fraction(), if diameter is above a calculated limit and
 *  by 90 % if above a calculated maximum diameter.
 *  If clearcut is selected (depending on result from cut_fraction()), individual is killed
 */
void harvest_forest(Individual& indiv, Pft& pft, bool alive, double anpp, bool& killed) {

	Patch& patch = indiv.vegetation.patch;
	Patchpft& ppft = patch.pft[indiv.pft.id];
	const double minbon=2.351;		// The minimum average "bonitet" for a county in Sweden
	const double maxbon=11.311;		// The maximum average "bonitet" for a county in Sweden
	const double bonitet = 10.0;	// Temporary static value

	int age_class = 0;
	double man_strength = patch.man_strength;

	if (pft.lifeform==TREE && man_strength > 0.00) {

		double diam = pow(indiv.height / indiv.pft.k_allom2, 1.0 / indiv.pft.k_allom3);

		if (man_strength == 1.00) {
			clearcut(indiv, anpp, killed);
		}
		else {

			double diam_limit=0.13+0.07*(bonitet-minbon)/(maxbon-minbon); // Harvest of trees > 19 cm
			double diam_max = diam_limit * 2.0;

			if (diam>diam_limit) {
				if (diam > diam_max)
					man_strength = 0.9;
				harvest_wood(indiv, man_strength, indiv.pft.harv_eff, indiv.pft.res_outtake); // frac_cut=man_strength, harv_eff=pft.harv_eff, res_outtake_twig=pft.res_outtake, res_outtake_coarse_root=0
				indiv.densindiv *= (1.0 - man_strength);
			}
		}
		// Will tell the program to skip establishment and mortality if management has been performed on this patch,
		patch.managed_this_year = true;		
		patch.managed = true;
	}
}


/// Harvest function for pasture, representing grazing (previous year).
/*  Function for balancing carbon and nitrogen fluxes from last year's growth
 *  A fraction of leaves is harvested (pft.harv_eff) and returned as acflux_harvest
 *  This represents grazing minus return as manure.
 *  The rest is handled like natural grass in turnover().
 *  Called from growth() last day of the year for normal harvest/grazing.
 *  Also called from landcover_dynamics() first day of the year if any natural vegetation
 *    is transferred to another land use.
 *  This calls for a scaling factor, when the pasture area has increased.
 *
 *  INPUT/OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following indiv-specific public members:
 *   - cmass_leaf 					leaf C biomass (kgC/m2)
 *   - cmass_root					fine root C biomass (kgC/m2)
 *   - nmass_leaf 					leaf nitrogen biomass (kgN/m2)
 *   - nmass_root 					fine root nitrogen biomass (kgN/m2)
 *  OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following patchpft-specific public members:
 *   - litter_leaf    				new leaf C litter (kgC/m2)
 *   - litter_root 					new root C litter (kgC/m2)
 *   - nmass_litter_leaf 			new leaf nitrogen litter (kgN/m2)
 *   - nmass_litter_root			new root nitrogen litter (kgN/m2)
 *									,and the following patch-level public members:
 *   - acflux_harvest				harvest flux to atmosphere (kgC/m2)
 *   - harvested_products_slow		harvest products to slow pool (kgC/m2)
 *   - anflux_harvest   			harvest nitrogen flux out of system (kgC/m2)
 *   - harvested_products_slow_nmass harvest nitrogen products to slow pool (kgC/m2)
 */
void harvest_pasture(Harvest_CN& i, Pft& pft, bool alive) {

	double harvest;

	// harvest of leaves (grazing)

	// Carbon:
	harvest = pft.harv_eff * i.cmass_leaf;

	if (ifslowharvestpool) {
		i.harvested_products_slow += harvest * pft.harvest_slow_frac;
		harvest = harvest * (1 - pft.harvest_slow_frac);
	}
	if (alive)
		i.acflux_harvest += harvest;
	i.cmass_leaf -= harvest;

	// Nitrogen
	// Reduced removal of N relative to C during grazing.
	double N_harvest_scale = 0.25; // Value that works. Needs to be verified in literature.
	harvest = pft.harv_eff * i.nmass_leaf * N_harvest_scale;

	if (ifslowharvestpool) {
		i.harvested_products_slow_nmass += harvest * pft.harvest_slow_frac;
		harvest = harvest * (1 - pft.harvest_slow_frac);
	}
	i.anflux_harvest += harvest;
	i.nmass_leaf -= harvest;

	if (grassforcrop && alive) {
		// Carbon:
		double residue_outtake = pft.res_outtake * i.cmass_leaf;	// res_outtake currently set to 0.0,
		i.acflux_harvest += residue_outtake;				// could be used for burning
		i.cmass_leaf -= residue_outtake;

		// Nitrogen:
		residue_outtake = pft.res_outtake * i.nmass_leaf;
		i.anflux_harvest += residue_outtake;
		i.nmass_leaf -= residue_outtake;
	}
}

/// Harvest function for pasture, representing grazing (previous year).
/*  Function for balancing carbon and nitrogen fluxes from last year's growth
 *  A fraction of leaves is harvested (pft.harv_eff) and returned as acflux_harvest
 *  This represents grazing minus return as manure.
 *  The rest is handled like natural grass in turnover().
 *  Called from growth() last day of the year for normal harvest/grazing.
 *  Also called from landcover_dynamics() first day of the year if any natural vegetation
 *    is transferred to another land use.
 *  This calls for a scaling factor, when the pasture area has increased.
 *
 *  This function copies variables from an individual and it's associated patchpft and patch to
 *  a Harvest_CN struct, which is then passed on to the main harvest_crop function.
 *  After the execution of the main harvest_crop function, the output variables are copied
 *  back to the individual and patchpft and the patch-level fluxes are updated.
 *
 *  INPUT/OUTPUT PARAMETERS
 *  \param indiv					reference to an Individual containing the following indiv-specific public members:
 *   - cmass_leaf 					leaf C biomass (kgC/m2)
 *   - cmass_root					fine root C biomass (kgC/m2)
 *   - nmass_leaf 					leaf nitrogen biomass (kgN/m2)
 *   - nmass_root 					fine root nitrogen biomass (kgN/m2)
 *  OUTPUT PARAMETERS
 *  \param indiv					reference to an Individual containing the following patchpft-specific public members:
 *   - litter_leaf    				new leaf C litter (kgC/m2)
 *   - litter_root 					new root C litter (kgC/m2)
 *   - nmass_litter_leaf 			new leaf nitrogen litter (kgN/m2)
 *   - nmass_litter_root			new root nitrogen litter (kgN/m2)
 *									,and the following patch-level public members:
 *   - acflux_harvest				harvest flux to atmosphere (kgC/m2)
 *   - harvested_products_slow		harvest products to slow pool (kgC/m2)
 *   - anflux_harvest   			harvest nitrogen flux out of system (kgC/m2)
 *   - harvested_products_slow_nmass harvest nitrogen products to slow pool (kgC/m2)
 */
void harvest_pasture(Individual& indiv, Pft& pft, bool alive) {

	Harvest_CN indiv_cp;

	indiv_cp.copy_from_indiv(indiv);

	harvest_pasture(indiv_cp, pft, alive);

	indiv_cp.copy_to_indiv(indiv);

}

/// Harvest function for cropland, including true crops, intercrop grass
/**   and pasture grass grown in cropland.
 *  Function for balancing carbon and nitrogen fluxes from this year's harvested carbon and nitrogen.
 *  A fraction of harvestable organs (grass:leaves) is harvested (pft.harv_eff) and returned as acflux_harvest.
 *  A fraction of leaves is removed (pft.res_outtake) and returned as acflux_harvest
 *  The rest, including roots, is returned as litter, leaving NO carbon or nitrogen in living tissue.
 *  Called from growth() last day of the year for old-style harvest/grazing or, alternatively, from crop_growth_daily() at harvest day
 *	(hdate) or last intercrop day (eicdate).
 *  Also called from landcover_dynamics() first day of the year if any natural vegetation
 *    is transferred to another land use.
 *  This calls for a scaling factor, when the pasture area has increased.
 *
 *  This function takes a Harvest_CN struct as an input parameter, copied from an individual and it's associated patchpft and patch.
 *
 *  INPUT PARAMETERS
 *  \param alive					whether individual has survived the first year
 *  \param isintercropgrass			whether individual is cover crop grass
 *
 *  INPUT/OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following indiv-specific public members:
 *   - cmass_leaf 					leaf C biomass (kgC/m2)
 *   - cmass_root					fine root C biomass (kgC/m2)
 *   - cmass_ho						harvestable organ C biomass (kgC/m2)
 *   - cmass_agpool					above-ground pool C biomass (kgC/m2)
 *   - nmass_leaf 					leaf nitrogen biomass (kgN/m2)
 *   - nmass_root 					fine root nitrogen biomass (kgN/m2)
 *   - param nmass_ho				harvestable organ nitrogen biomass (kgC/m2)
 *   - param nmass_agpool			above-ground pool nitrogen biomass (kgC/m2)
 *   - nstore_labile    			labile nitrogen storage (kgC/m2)
 *   - nstore_longterm    			longterm nitrogen storage (kgC/m2)
 *  OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following patchpft-specific public members:
 *   - litter_leaf    				new leaf C litter (kgC/m2)
 *   - litter_root 					new root C litter (kgC/m2)
 *   - nmass_litter_leaf 			new leaf nitrogen litter (kgN/m2)
 *   - nmass_litter_root			new root nitrogen litter (kgN/m2)
 *									,and the following patch-level public members:
 *   - acflux_harvest				harvest flux to atmosphere (kgC/m2)
 *   - harvested_products_slow		harvest products to slow pool (kgC/m2)
 *   - anflux_harvest   			harvest nitrogen flux out of system (kgC/m2)
 *   - harvested_products_slow_nmass harvest nitrogen products to slow pool (kgC/m2)
 */
void harvest_crop(Harvest_CN& i, Pft& pft, bool alive, bool isintercropgrass) {

	double residue_outtake, harvest;

	if (pft.phenology==CROPGREEN) {

	// all root carbon and nitrogen goes to litter
		if (i.cmass_root > 0.0)
			i.litter_root += i.cmass_root;
		i.cmass_root = 0.0;

		if (i.nmass_root > 0.0)
			i.nmass_litter_root += i.nmass_root;
		if (i.nstore_labile > 0.0)
			i.nmass_litter_root += i.nstore_labile;
		if (i.nstore_longterm > 0.0)
			i.nmass_litter_root += i.nstore_longterm;
		i.nmass_root = 0.0;
		i.nstore_labile = 0.0;
		i.nstore_longterm = 0.0;

		// harvest of harvestable organs
		// Carbon:
		if (i.cmass_ho > 0.0) {
			// harvested products
			harvest = pft.harv_eff * i.cmass_ho;

			// not removed harvestable organs are put into litter
			if (pft.aboveground_ho)
				i.litter_leaf += (i.cmass_ho - harvest);
			else
				i.litter_root += (i.cmass_ho - harvest);

			// harvested products not consumed (oxidised) this year put into harvested_products_slow
			if (ifslowharvestpool) {
				i.harvested_products_slow += harvest * pft.harvest_slow_frac;
				harvest = harvest * (1 - pft.harvest_slow_frac);
			}

			// harvested products consumed (oxidised) this year put into acflux_harvest
			i.acflux_harvest += harvest;
		}
		i.cmass_ho = 0.0;

		// Nitrogen:
		if (i.nmass_ho > 0.0) {

			// harvested products
			harvest = pft.harv_eff * i.nmass_ho;

			// not removed harvestable organs are put into litter
			if (pft.aboveground_ho)
				i.nmass_litter_leaf += (i.nmass_ho - harvest);
			else
				i.nmass_litter_root += (i.nmass_ho - harvest);

			// harvested products not consumed this year put into harvested_products_slow_nmass
			if (ifslowharvestpool) {
				i.harvested_products_slow_nmass += harvest * pft.harvest_slow_frac;
				harvest = harvest * (1 - pft.harvest_slow_frac);
			}

			// harvested products consumed this year put into anflux_harvest
			i.anflux_harvest += harvest;
		}
		i.nmass_ho = 0.0;

		// residues
		// Carbon
		if ((i.cmass_leaf + i.cmass_agpool + i.cmass_dead_leaf + i.cmass_stem) > 0.0) {

			// removed residues are oxidised
			residue_outtake = pft.res_outtake * (i.cmass_leaf + i.cmass_agpool + i.cmass_dead_leaf + i.cmass_stem);
			i.acflux_harvest += residue_outtake;

			// not removed residues are put into litter
			i.litter_leaf += i.cmass_leaf + i.cmass_agpool + i.cmass_dead_leaf + i.cmass_stem - residue_outtake;
		}
		i.cmass_leaf = 0.0;
		i.cmass_agpool = 0.0;
		i.cmass_dead_leaf = 0.0;
		i.cmass_stem = 0.0;

		// Nitrogen:
		if ((i.nmass_leaf + i.nmass_agpool + i.nmass_dead_leaf) > 0.0) {

			// removed residues are oxidised
			residue_outtake = pft.res_outtake * (i.nmass_leaf + i.nmass_agpool + i.nmass_dead_leaf);
			i.nmass_litter_leaf += i.nmass_leaf + i.nmass_agpool + i.nmass_dead_leaf - residue_outtake;

			// not removed residues are put into litter
			i.anflux_harvest += residue_outtake;
		}
		i.nmass_leaf = 0.0;
		i.nmass_agpool = 0.0;
		i.nmass_dead_leaf = 0.0;
	}
	else if (pft.phenology == ANY) {

		// Intercrop grass
		if (isintercropgrass) {

			// roots

			// all root carbon and nitrogen goes to litter
			if (i.cmass_root > 0.0)
				i.litter_root += i.cmass_root;
			if (i.nmass_root > 0.0)
				i.nmass_litter_root += i.nmass_root;
			if (i.nstore_labile > 0.0)
				i.nmass_litter_root += i.nstore_labile;
			if (i.nstore_longterm > 0.0)
				i.nmass_litter_root += i.nstore_longterm;

			i.cmass_root = 0.0;
			i.nmass_root = 0.0;
			i.nstore_labile = 0.0;
			i.nstore_longterm = 0.0;


			// leaves

			// Carbon:
			if (i.cmass_leaf > 0.0) {

				// Harvest/Grazing of leaves:
				harvest = pft.harv_eff_ic * i.cmass_leaf;	// currently no harvest of intercrtop grass

				// not removed grass is put into litter
				i.litter_leaf += i.cmass_leaf - harvest;

				if (ifslowharvestpool) {
					i.harvested_products_slow += harvest * pft.harvest_slow_frac; // no slow harvest for grass
					harvest = harvest * (1 - pft.harvest_slow_frac);
				}

				i.acflux_harvest += harvest;
			}
			i.cmass_leaf = 0.0;
			i.cmass_ho = 0.0;
			i.cmass_agpool = 0.0;

			// Nitrogen:
			if (i.nmass_leaf > 0.0) {

				// Harvest/Grazing of leaves:
				harvest = pft.harv_eff_ic * i.nmass_leaf;	// currently no harvest of intercrtop grass

				// not removed grass is put into litter
				i.nmass_litter_leaf += i.nmass_leaf - harvest;

				if (ifslowharvestpool) {
					i.harvested_products_slow_nmass += harvest * pft.harvest_slow_frac; // no slow harvest for grass
					harvest = harvest * (1 - pft.harvest_slow_frac);
				}

				i.anflux_harvest += harvest;
			}
			i.nmass_leaf = 0.0;
			i.nmass_ho = 0.0;
			i.nmass_agpool = 0.0;

		}
		else {	// pasture grass

			// harvest of leaves (grazing)

			// Carbon:
			harvest = pft.harv_eff * i.cmass_leaf;

			if (ifslowharvestpool) {
				i.harvested_products_slow += harvest * pft.harvest_slow_frac;
				harvest = harvest * (1 - pft.harvest_slow_frac);
			}
			if (alive)
				i.acflux_harvest += harvest;
			i.cmass_leaf -= harvest;

			i.cmass_ho = 0.0;
			i.cmass_agpool = 0.0;

			// Nitrogen:
			// Reduced removal of N relative to C during grazing.
			double N_harvest_scale = 0.25; // Value that works. Needs to be verified in literature.
			harvest = pft.harv_eff * i.nmass_leaf * N_harvest_scale;

			if (ifslowharvestpool) {
				i.harvested_products_slow_nmass += harvest * pft.harvest_slow_frac;
				harvest = harvest * (1 - pft.harvest_slow_frac);
			}
			i.anflux_harvest += harvest;
			i.nmass_leaf -= harvest;

			i.nmass_ho=0.0;
			i.nmass_agpool=0.0;
		}
	}
}

/// Harvest function for cropland, including true crops, intercrop grass
/**   and pasture grass grown in cropland.
 *  Function for balancing carbon and nitrogen fluxes from this year's harvested carbon and nitrogen.
 *  A fraction of harvestable organs (grass:leaves) is harvested (pft.harv_eff) and returned as acflux_harvest.
 *  A fraction of leaves is removed (pft.res_outtake) and returned as acflux_harvest
 *  The rest, including roots, is returned as litter, leaving NO carbon or nitrogen in living tissue.
 *  Called from growth() last day of the year for old-style harvest/grazing or, alternatively, from crop_growth_daily() at harvest day
 *	(hdate) or last intercrop day (eicdate).
 *  Also called from landcover_dynamics() first day of the year if any natural vegetation
 *    is transferred to another land use.
 *  This calls for a scaling factor, when the pasture area has increased.
 *
 *  This function copies variables from an individual and it's associated patchpft and patch to
 *  a Harvest_CN struct, which is then passed on to the main harvest_crop() function.
 *  After the execution of the main harvest_crop function, the output variables are copied
 *  back to the individual and patchpft and the patch-level fluxes are updated.
 *
 *  INPUT PARAMETERS
 *  \param alive					whether individual has survived the first year
 *  \param isintercropgrass			whether individual is cover crop grass
 *  \param harvest_grsC				whether harvest daily carbon values are harvested
 *
 *  INPUT/OUTPUT PARAMETERS
 *  \param indiv					reference to an Individual containing the following indiv-specific public members:
 *   - cmass_leaf 					leaf C biomass (kgC/m2)
 *   - cmass_root					fine root C biomass (kgC/m2)
 *   - cmass_ho						harvestable organ C biomass (kgC/m2)
 *   - cmass_agpool					above-ground pool C biomass (kgC/m2)
 *   - nmass_leaf 					leaf nitrogen biomass (kgN/m2)
 *   - nmass_root 					fine root nitrogen biomass (kgN/m2)
 *   - param nmass_ho				harvestable organ nitrogen biomass (kgC/m2)
 *   - param nmass_agpool			above-ground pool nitrogen biomass (kgC/m2)
 *   - nstore_labile    			labile nitrogen storage (kgC/m2)
 *   - nstore_longterm    			longterm nitrogen storage (kgC/m2)
 *  OUTPUT PARAMETERS
 *  \param indiv					reference to an Individual containing the following patchpft-specific public members:
 *   - litter_leaf    				new leaf C litter (kgC/m2)
 *   - litter_root 					new root C litter (kgC/m2)
 *   - nmass_litter_leaf 			new leaf nitrogen litter (kgN/m2)
 *   - nmass_litter_root			new root nitrogen litter (kgN/m2)
 *									,and the following patch-level public members:
 *   - acflux_harvest				harvest flux to atmosphere (kgC/m2)
 *   - harvested_products_slow		harvest products to slow pool (kgC/m2)
 *   - anflux_harvest   			harvest nitrogen flux out of system (kgC/m2)
 *   - harvested_products_slow_nmass harvest nitrogen products to slow pool (kgC/m2)
 */
void harvest_crop(Individual& indiv, Pft& pft, bool alive, bool isintercropgrass, bool harvest_grsC) {

	Harvest_CN indiv_cp;

	indiv_cp.copy_from_indiv(indiv, harvest_grsC);

	harvest_crop(indiv_cp, pft, alive, isintercropgrass);

	indiv_cp.copy_to_indiv(indiv, harvest_grsC);

}


/// Transfers all carbon and nitrogen from living tissue to litter
/** Mainly used at land cover change when remaining vegetation after harvest (grass) is
 *   killed by tillage, following an optional burning.
 *
 *  This function takes a Harvest_CN struct as an input parameter, copied from an individual and it's associated patchpft and patch.
 *
 *  INPUT PARAMETERS
 *  \param alive					whether individual has survived the first year
 *  \param isintercropgrass			whether individual is cover crop grass
 *  \param burn						whether above-ground vegetation C & N is sent to the atmosphere
 *								     rather than to litter
 *  INPUT/OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following indiv-specific public members:
 *   - cmass_leaf 					leaf C biomass (kgC/m2)
 *   - cmass_root					fine root C biomass (kgC/m2)
 *   - cmass_ho						harvestable organ C biomass (kgC/m2)
 *   - cmass_agpool					above-ground pool C biomass (kgC/m2)
 *   - cmass_sap					sapwood C biomass (kgC/m2)
 *   - cmass_heart   				heartwood C biomass (kgC/m2)
 *   - cmass_debt					C "debt" (retrospective storage) (kgC/m2)
 *   - nmass_leaf 					leaf nitrogen biomass (kgN/m2)
 *   - nmass_root 					fine root nitrogen biomass (kgN/m2)
 *   - nmass_sap   					sapwood nitrogen biomass (kgC/m2)
 *   - nmass_heart    				heartwood nitrogen biomass (kgC/m2)
 *   - param nmass_ho				harvestable organ nitrogen biomass (kgC/m2)
 *   - param nmass_agpool			above-ground pool nitrogen biomass (kgC/m2)
 *   - nstore_labile    			labile nitrogen storage (kgC/m2)
 *   - nstore_longterm    			longterm nitrogen storage (kgC/m2)
 *  OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following patchpft-specific public members:
 *   - litter_leaf    				new leaf C litter (kgC/m2)
 *   - litter_root 					new root C litter (kgC/m2)
 *   - nmass_litter_leaf 			new leaf nitrogen litter (kgN/m2)
 *   - nmass_litter_root			new root nitrogen litter (kgN/m2)
 *									,and the following patch-level public members:
 *   - acflux_harvest				harvest flux to atmosphere (kgC/m2)
 *   - anflux_harvest   			harvest nitrogen flux out of system (kgC/m2)
 */
void kill_remaining_vegetation(Harvest_CN& cp, Pft& pft, bool alive, bool istruecrop_or_intercropgrass, bool burn) {


	if (alive || istruecrop_or_intercropgrass)  {
		cp.litter_root += cp.cmass_root;

		if (burn) {
			cp.acflux_harvest += cp.cmass_leaf;
			cp.acflux_harvest += cp.cmass_sap;
			cp.acflux_harvest += cp.cmass_heart - cp.cmass_debt;
		}
		else {
			cp.litter_leaf += cp.cmass_leaf;
			cp.litter_sap += cp.cmass_sap;
			cp.litter_heart += cp.cmass_heart - cp.cmass_debt;
		}
	}

	cp.nmass_litter_root += cp.nmass_root;
	cp.nmass_litter_root += cp.nstore_longterm;
	cp.nmass_litter_root += cp.nstore_labile;

	if (burn) {
		cp.anflux_harvest += cp.nmass_leaf;
		cp.anflux_harvest += cp.nmass_sap;
		cp.anflux_harvest += cp.nmass_heart;
	}
	else {
		cp.nmass_litter_leaf += cp.nmass_leaf;
		cp.nmass_litter_sap += cp.nmass_sap;
		cp.nmass_litter_heart += cp.nmass_heart;
	}

	if (pft.landcover == CROPLAND) {
		if (pft.aboveground_ho) {
			if (burn) {
				cp.acflux_harvest += cp.cmass_ho;
				cp.anflux_harvest += cp.nmass_ho;
			}
			else {
				cp.litter_leaf += cp.cmass_ho;
				cp.nmass_litter_leaf += cp.nmass_ho;
			}
		}
		else {
			cp.litter_root += cp.cmass_ho;
			cp.nmass_litter_root += cp.nmass_ho;
		}

		if (burn) {
			cp.acflux_harvest += cp.cmass_agpool;
			cp.anflux_harvest += cp.nmass_agpool;
		}
		else {
			cp.litter_leaf += cp.cmass_agpool;
			cp.nmass_litter_leaf += cp.nmass_agpool;
		}
	}

	cp.cmass_leaf = 0.0;
	cp.cmass_root = 0.0;
	cp.cmass_sap = 0.0;
	cp.cmass_heart = 0.0;
	cp.cmass_debt = 0.0;
	cp.cmass_ho = 0.0;
	cp.cmass_agpool = 0.0;
	cp.nmass_leaf = 0.0;
	cp.nmass_root = 0.0;
	cp.nstore_longterm = 0.0;
	cp.nstore_labile = 0.0;
	cp.nmass_sap = 0.0;
	cp.nmass_heart = 0.0;
	cp.nmass_ho = 0.0;
	cp.nmass_agpool = 0.0;

}

/// Transfers all carbon and nitrogen from living tissue to litter
/** Mainly used at land cover change when remaining vegetation after harvest (grass) is
 *   killed by tillage, following an optional burning.
 *
 *  This function copies variables from an individual and it's associated patchpft and patch to
 *  a Harvest_CN struct, which is then passed on to the main harvest_crop() function.
 *  After the execution of the main harvest_crop function, the output variables are copied
 *  back to the individual and patchpft and the patch-level fluxes are updated.
 *
 *  INPUT PARAMETERS
 *  \param alive					whether individual has survived the first year
 *  \param isintercropgrass			whether individual is cover crop grass
 *  \param burn						whether above-ground vegetation C & N is sent to the atmosphere
 *								     rather than to litter
 *  INPUT/OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following indiv-specific public members:
 *   - cmass_leaf 					leaf C biomass (kgC/m2)
 *   - cmass_root					fine root C biomass (kgC/m2)
 *   - cmass_ho						harvestable organ C biomass (kgC/m2)
 *   - cmass_agpool					above-ground pool C biomass (kgC/m2)
 *   - cmass_sap					sapwood C biomass (kgC/m2)
 *   - cmass_heart   				heartwood C biomass (kgC/m2)
 *   - cmass_debt					C "debt" (retrospective storage) (kgC/m2)
 *   - nmass_leaf 					leaf nitrogen biomass (kgN/m2)
 *   - nmass_root 					fine root nitrogen biomass (kgN/m2)
 *   - nmass_sap   					sapwood nitrogen biomass (kgC/m2)
 *   - nmass_heart    				heartwood nitrogen biomass (kgC/m2)
 *   - param nmass_ho				harvestable organ nitrogen biomass (kgC/m2)
 *   - param nmass_agpool			above-ground pool nitrogen biomass (kgC/m2)
 *   - nstore_labile    			labile nitrogen storage (kgC/m2)
 *   - nstore_longterm    			longterm nitrogen storage (kgC/m2)
 *  OUTPUT PARAMETERS
 *  \param Harvest_CN& i			struct containing the following patchpft-specific public members:
 *   - litter_leaf    				new leaf C litter (kgC/m2)
 *   - litter_root 					new root C litter (kgC/m2)
 *   - nmass_litter_leaf 			new leaf nitrogen litter (kgN/m2)
 *   - nmass_litter_root			new root nitrogen litter (kgN/m2)
 *									,and the following patch-level public members:
 *   - acflux_harvest				harvest flux to atmosphere (kgC/m2)
 *   - anflux_harvest   			harvest nitrogen flux out of system (kgC/m2)
 */
void kill_remaining_vegetation(Individual& indiv, bool burn, bool lc_change) {

	Harvest_CN indiv_cp;

	indiv_cp.copy_from_indiv(indiv);

	kill_remaining_vegetation(indiv_cp, indiv.pft, indiv.alive, indiv.istruecrop_or_intercropgrass(), burn);

	indiv_cp.copy_to_indiv(indiv, false, lc_change);

	if (burn && lc_change) {
		Stand& stand = indiv.vegetation.patch.stand;
		Landcover& lc = stand.get_gridcell().landcover;
		lc.acflux_landuse_change += stand.get_gridcell_fraction() * indiv_cp.acflux_harvest / (double)stand.nobj;
		lc.acflux_landuse_change_lc[stand.origin] += stand.get_gridcell_fraction() * indiv_cp.acflux_harvest / (double)stand.nobj;
		lc.anflux_landuse_change += stand.get_gridcell_fraction() * indiv_cp.anflux_harvest / (double)stand.nobj;
		lc.anflux_landuse_change_lc[stand.origin] += stand.get_gridcell_fraction() * indiv_cp.anflux_harvest / (double)stand.nobj;
	}

}

/// Scaling of last year's or harvest day individual carbon and nitrogen member values in stands that have increased their area fraction this year.
/** Called immediately before harvest functions in growth() or allocation_crop_daily().
 */
void scale_indiv(Individual& indiv, bool scale_grsC) {

	Stand& stand = indiv.vegetation.patch.stand;
	Gridcell& gridcell = stand.get_gridcell();

	if (stand.scale_LC_change >= 1.0) {
		return;
	}

	// Scale individual's C and N mass in stands that have increased in area
	// this year by (old area/new area):
	double scale = stand.scale_LC_change;

	if (scale_grsC) {

		if (indiv.pft.landcover == CROPLAND) {

			if (indiv.has_daily_turnover()) {

				indiv.cropindiv->grs_cmass_leaf -= indiv.cropindiv->grs_cmass_leaf_luc * (1.0 - scale);
				indiv.cropindiv->grs_cmass_root -= indiv.cropindiv->grs_cmass_root_luc * (1.0 - scale);
				indiv.cropindiv->grs_cmass_ho -= indiv.cropindiv->grs_cmass_ho_luc * (1.0 - scale);
				indiv.cropindiv->grs_cmass_agpool -= indiv.cropindiv->grs_cmass_agpool_luc * (1.0 - scale);
				indiv.cropindiv->grs_cmass_dead_leaf -= indiv.cropindiv->grs_cmass_dead_leaf_luc * (1.0 - scale);
				indiv.cropindiv->grs_cmass_stem -= indiv.cropindiv->grs_cmass_stem_luc * (1.0 - scale);

				indiv.check_C_mass();
			}
			else {
				indiv.cropindiv->grs_cmass_leaf *= scale;
				indiv.cropindiv->grs_cmass_root *= scale;
				indiv.cropindiv->grs_cmass_ho *= scale;
				indiv.cropindiv->grs_cmass_agpool *= scale;
				indiv.cropindiv->grs_cmass_plant *= scale;	//grs_cmass_plant not used
				indiv.cropindiv->grs_cmass_dead_leaf *= scale;
				indiv.cropindiv->grs_cmass_stem *= scale;
			}
		}
	}
	else {

		indiv.cmass_root *= scale;
		indiv.cmass_leaf *= scale;
		indiv.cmass_heart *= scale;
		indiv.cmass_sap *= scale;
		indiv.cmass_debt *= scale;

		if (indiv.pft.landcover == CROPLAND) {
			indiv.cropindiv->cmass_agpool *= scale;
			indiv.cropindiv->cmass_ho *= scale;
		}
	}

	// Deduct individual N present day 0 this year in stands that have increased in area this year, scaled by (1 - old area/new area):
	indiv.nmass_root = indiv.nmass_root - indiv.nmass_root_luc * (1.0 - scale);
	indiv.nmass_leaf = indiv.nmass_leaf - indiv.nmass_leaf_luc * (1.0 - scale);
	indiv.nmass_heart = indiv.nmass_heart - indiv.nmass_heart_luc * (1.0 - scale);
	indiv.nmass_sap = indiv.nmass_sap - indiv.nmass_sap_luc * (1.0 - scale);

	if (indiv.pft.landcover == CROPLAND) {
		indiv.cropindiv->nmass_agpool = indiv.cropindiv->nmass_agpool - indiv.cropindiv->nmass_agpool_luc * (1.0 - scale);
		indiv.cropindiv->nmass_ho = indiv.cropindiv->nmass_ho - indiv.cropindiv->nmass_ho_luc * (1.0 - scale);
		indiv.cropindiv->nmass_dead_leaf =indiv.cropindiv->nmass_dead_leaf - indiv.cropindiv->nmass_dead_leaf_luc * (1.0 - scale);
	}

	if (indiv.nstore_labile > indiv.nstore_labile_luc * (1.0 - scale))
		indiv.nstore_labile -= indiv.nstore_labile_luc * (1.0 - scale);
	else
		indiv.nstore_longterm -= indiv.nstore_labile_luc * (1.0 - scale);
	indiv.nstore_longterm = indiv.nstore_longterm - indiv.nstore_longterm_luc * (1.0 - scale);

	indiv.check_N_mass();
}

/// Yearly function for harvest of all land covers that have yearly allocation, turnover and gridcell.expand_to_new_stand[lc] = false.
/** Should only be called from growth().
//  Harvest functions are preceded by rescaling of living C.
//  Only affects natural stands if gridcell.expand_to_new_stand[NATURAL] is false.
 */
bool harvest_year(Individual& indiv) {

	Stand& stand = indiv.vegetation.patch.stand;
	Landcover& landcover = stand.get_gridcell().landcover;
	bool killed = false;

	// Reduce individual's C and N mass in stands that have increased in area this year:
	if (landcover.updated && !indiv.has_daily_turnover()) {
		scale_indiv(indiv, false);
	}

	if (stand.landcover == CROPLAND) {
		if (!indiv.has_daily_turnover())
			harvest_crop(indiv, indiv.pft, indiv.alive, indiv.cropindiv->isintercropgrass, false);
	}
	else if (stand.landcover == PASTURE) {
		harvest_pasture(indiv, indiv.pft, indiv.alive);
	}
	else if(stand.landcover == FOREST || stand.landcover == NATURAL && run_landcover)
		harvest_forest(indiv, indiv.pft, indiv.alive, indiv.anpp, killed);

	return killed;
}

/// Yield function for true crops and intercrop grass.
void yield_crop(Individual& indiv) {

	cropindiv_struct& cropindiv = *(indiv.get_cropindiv());

	if (indiv.pft.phenology == ANY) {			// grass intercrop yield

		// Yield dry wieght of allocated harvestable organs this year; NB independent from harvest calculation in harvest_crop (different years)
		if (cropindiv.ycmass_leaf > 0.0)
			cropindiv.yield = cropindiv.ycmass_leaf * indiv.pft.harv_eff_ic * 2.0;
		else
			cropindiv.yield = 0.0;

		// Yield dry wieght of actually harvest products this year; NB as above
		if (cropindiv.harv_cmass_leaf > 0.0)
			cropindiv.harv_yield = cropindiv.harv_cmass_leaf * indiv.pft.harv_eff_ic * 2.0;
		else
			cropindiv.harv_yield = 0.0;
	}
	else if (indiv.pft.phenology == CROPGREEN) {		//true crop yield

		// Yield dry wieght of allocated harvestable organs this year; NB independent from harvest calculation in harvest_crop (different years)
		if (cropindiv.ycmass_ho > 0.0)
			cropindiv.yield = cropindiv.ycmass_ho * indiv.pft.harv_eff * 2.0;// Should be /0.446 instead
		else
			cropindiv.yield = 0.0;

		// Yield dry wieght of actually harvest products this year; NB as above
		if (cropindiv.harv_cmass_ho > 0.0)
			cropindiv.harv_yield=cropindiv.harv_cmass_ho * indiv.pft.harv_eff * 2.0;
		else
			cropindiv.harv_yield = 0.0;

		// Yield dry wieght of actually harvest products this year; NB as above
		for (int i=0;i<2;i++) {
			if (cropindiv.cmass_ho_harvest[i] > 0.0)
				cropindiv.yield_harvest[i] = cropindiv.cmass_ho_harvest[i] * indiv.pft.harv_eff * 2.0;
			else
				cropindiv.yield_harvest[i]=0.0;
		}
	}

	return;
}

/// Yield function for pasture grass grown in cropland landcover
void yield_pasture(Individual& indiv, double cmass_leaf_inc) {

	cropindiv_struct& cropindiv = *(indiv.get_cropindiv());

	// Normal CC3G/CC4G stand growth (Pasture)

	// OK if turnover_leaf==1.0, else (cmass_leaf+cmass_leaf_inc)*indiv.pft.harv_eff*2.0
	cropindiv.yield = max(0.0, cmass_leaf_inc) * indiv.pft.harv_eff * 2.0;
	// Although no specified harvest date, harv_yield is set for compatibility.
	cropindiv.harv_yield = cropindiv.yield;
}

/// Function that determines amount of nitrogen applied today. Crop-specific, pft-based.
void nfert_crop(Patch& patch) {

	Gridcell& gridcell = patch.stand.get_gridcell();

	patch.dnfert = 0.0;

	pftlist.firstobj();
	// Loop through PFTs
	while(pftlist.isobj) {

		Pft& pft = pftlist.getobj();
		Patchpft& patchpft = patch.pft[pft.id];
		Gridcellpft& gridcellpft = gridcell.pft[pft.id];

		if (patch.stand.pft[pft.id].active && pft.phenology == CROPGREEN) {

			cropphen_struct& ppftcrop = *(patchpft.get_cropphen());
			if(!ppftcrop.growingseason) {
				pftlist.nextobj();
				continue;
			}

			double nfert = pft.N_appfert;
			if (gridcellpft.Nfert_read >= 0.0) {
				nfert = gridcellpft.Nfert_read;
			}
			if (!ppftcrop.fertilised[0] && ppftcrop.dev_stage > 0.0){
				// Fertiliser application at dev_stage = 0, sowing.
				patch.dnfert = nfert * (1.0 - pft.fertrate[0] - pft.fertrate[1]);
				ppftcrop.fertilised[0] = true;
			}
			else if (!ppftcrop.fertilised[1] && ppftcrop.dev_stage > pft.fert_stages[0]){
				patch.dnfert = nfert * pft.fertrate[0];
				ppftcrop.fertilised[1] = true;
			}
			else if (!ppftcrop.fertilised[2] && ppftcrop.dev_stage > pft.fert_stages[1]){
				patch.dnfert = nfert * (pft.fertrate[1]);
				ppftcrop.fertilised[2] = true;
			}
		}
		pftlist.nextobj();
	}
	patch.anfert += patch.dnfert;
}

/// Function that determines amount of nitrogen applied today.
void nfert(Patch& patch) {

	Stand& stand = patch.stand;
	StandType& st = stlist[stand.stid];
	Gridcell& gridcell = stand.get_gridcell();

	if(stand.landcover == CROPLAND) {
		nfert_crop(patch);
		return;
	}

	// General code for applying nitrogen to other land cover types, an equal amount every day.
	double nfert;
	if(gridcell.st[st.id].nfert >= 0.0) {	// todo: management type variable (mt.nfert)
		nfert = gridcell.st[st.id].nfert;
	}
	else {
		nfert = 0.0;
	}
	patch.dnfert = nfert / date.year_length();
	patch.anfert += patch.dnfert;
}

/// Updates crop rotation status
/** Sets new crop management variables, typically on harvest day
 */
void crop_rotation(Stand& stand) {

	if (stand.landcover != CROPLAND) {
		return;
	}

	CropRotation& rotation = stlist[stand.stid].rotation;

	stand.ndays_inrotation++;

	if (rotation.ncrops < 2 || !stand.isrotationday) {
		return;
	}

	int firstrotyear = rotation.firstrotyear - date.first_calendar_year;
	bool postpone_rotation = false;

	// Alternative uses of firstrotyear:
	// 1. Before firstrotyear, grow only crop1:
//	if(date.year < firstrotyear)
//		postpone_rotation = true;

	// 2. Synchronise rotation with firstrotyear:

	// A. At the creation of the stand:
	if (date.year < stand.first_year + 3)
	// B. At firstrotyear
//		if(date.year == firstrotyear - 1)
	// C. Continuously:
	{
		if ((abs(firstrotyear - date.year) % rotation.ncrops) != stand.current_rot)
			postpone_rotation = true;
	}

	if (!postpone_rotation) {

		if(stand.infallow) {
			stand.infallow = false;
			stand.get_gridcell().pft[stand.pftid].sowing_restriction = false;
		}

		int old_pftid = stand.pftid;

		stand.rotate();

		for (unsigned int p=0; p<stand.nobj; p++) {

			cropphen_struct& previous = *(stand[p].pft[old_pftid].get_cropphen());
			cropphen_struct& current = *(stand[p].pft[stand.pftid].get_cropphen());

			previous.bicdate = -1;
			if(!previous.intercropseason)
				current.bicdate = stepfromdate(date.day, 15);
			previous.eicdate = -1;
			current.eicdate = -1;
			previous.hdate = -1;
			current.intercropseason = previous.intercropseason;
		}

		// Adds sowing and harvest dates for the second crop in a double cropping system
		if (stlist[stand.stid].get_management(stand.current_rot).multicrop && rotation.ncrops == 2 && stand.current_rot == 1) {
			if (stand.pft[stand.pftid].sdate_force < 0)
				stand.pft[stand.pftid].sdate_force = stepfromdate(date.day, 10);
			if (stand.pft[stand.pftid].hdate_force < 0) {
				stand.pft[stand.pftid].hdate_force = stepfromdate(stand.pft[old_pftid].sdate_force, -10);
			}
		}

		if(stlist[stand.stid].get_management(stand.current_rot).fallow) {
			stand.infallow = true;
			stand.get_gridcell().pft[stand.pftid].sowing_restriction = true;
		}
	}

	stand.isrotationday = false;
}
