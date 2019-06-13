///////////////////////////////////////////////////////////////////////////////////////
/// \file vegdynam.cpp
/// \brief Vegetation dynamics and disturbance
///
/// \author Ben Smith
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

// WHAT SHOULD THIS FILE CONTAIN?
// Module source code files should contain, in this order:
//   (1) a "#include" directive naming the framework header file. The framework header
//       file should define all classes used as arguments to functions in the present
//       module. It may also include declarations of global functions, constants and
//       types, accessible throughout the model code;
//   (2) other #includes, including header files for other modules accessed by the
//       present one;
//   (3) type definitions, constants and file scope global variables for use within
//       the present module only;
//   (4) declarations of functions defined in this file, if needed;
//   (5) definitions of all functions. Functions that are to be accessible to other
//       modules or to the calling framework should be declared in the module header
//       file.
//
// PORTING MODULES BETWEEN FRAMEWORKS:
// Modules should be structured so as to be fully portable between models (frameworks).
// When porting between frameworks, the only change required should normally be in the
// "#include" directive referring to the framework header file.

#include "config.h"
#include "vegdynam.h"

#include "growth.h"
#include "driver.h"


/// Internal help function for splitting up nitrogen fire fluxes into components
void report_fire_nfluxes(Patch& patch, double nflux_fire) {
	patch.fluxes.report_flux(Fluxes::NH3_FIRE, Fluxes::NH3_FIRERATIO * nflux_fire);
	patch.fluxes.report_flux(Fluxes::NOx_FIRE, Fluxes::NOx_FIRERATIO * nflux_fire);
	patch.fluxes.report_flux(Fluxes::N2O_FIRE, Fluxes::N2O_FIRERATIO * nflux_fire);
	patch.fluxes.report_flux(Fluxes::N2_FIRE,  Fluxes::N2_FIRERATIO  * nflux_fire);
}


///////////////////////////////////////////////////////////////////////////////////////
// RANDPOISSON
// Internal functions for generating random numbers


int randpoisson(double expectation, long& seed) {

	// DESCRIPTION
	// Returns a random integer drawn from the Poisson distribution with specified
	// expectation (for computational reasons, the Gaussian normal distribution is
	// used as an approximation of the Poisson distribution for expected values >100)

	double p, q, r;
	int n;

	if (expectation <= 100) {

		// For expected values up to 100, calculate a true Poisson number

		p = exp(-expectation);
		q = p;
		r=randfrac(seed);

		n = 0;
		while (q < r) {
			n++;
			p *= expectation / (double)n;
			q += p;
		}
		return n;
	}

	// For higher expected values than 100, approximate the Poisson distribution
	// by the Gaussian normal distribution with mean equal to the expected value,
	// and standard deviation the square root of this value

	do {
		r=randfrac(seed)*8.0-4.0;
		p = exp(-r * r / 2.0);
	} while (randfrac(seed)>p);

	return max(0, (int)(r * sqrt(expectation) + expectation + 0.5));
}


///////////////////////////////////////////////////////////////////////////////////////
// BIOCLIMATIC LIMITS ON ESTABLISHMENT AND SURVIVAL
// Internal functions (do not call directly from framework)

bool establish(Patch& patch, const Climate& climate, Pft& pft) {

	// DESCRIPTION
	// Determines whether specified PFT is within its bioclimatic limits for
	// establishment in a given patch and climate. Returns true if PFT can establish
	// under specified conditions, false otherwise

	// The following limits are implemented:
	//   tcmin_est   = minimum coldest month mean temperature for the last 20 years
	//   tcmax_est   = maximum coldest month mean temperature for the last 20 years
	//   twmin_est   = minimum warmest month mean temperature
	//   gdd5min_est = minimum growing degree day sum on 5 deg C base

	if (!patch.managed && (climate.mtemp_min20 < pft.tcmin_est ||
		climate.mtemp_min20 > pft.tcmax_est ||
		climate.mtemp_max < pft.twmin_est ||
		climate.agdd5 < pft.gdd5min_est)) return false;

	if(patch.stand.landcover != CROPLAND) {
		if (vegmode != POPULATION && patch.par_grass_mean < pft.parff_min) return false;
	}


	// guess2008 - DLE - new drought limited establishment
    if (ifdroughtlimitedestab) {
		// Compare this PFT's/species' drought_tolerance with the average wcont over the
		// growing season, in this patch. Higher drought_tolerance values (set in the .ins file)
		// lead to greater restrictions on establishment.
        if (pft.drought_tolerance > patch.soil.awcont[0]) {
           return false;
        }
    }


	// else

	return true;
}


bool survive(const Climate& climate, Pft& pft) {

	// DESCRIPTION
	// Determines whether specified PFT is within its bioclimatic limits for survival
	// in a given climate. Returns true if PFT can survive in the specified climate,
	// false otherwise

	// The following limit is implemented:
	//   tcmin_surv = minimum coldest month temperature for the last 20 years (deg C)

	if (climate.mtemp_min20 < pft.tcmin_surv ||
		climate.mtemp_max20 - climate.mtemp_min20 < pft.twminusc) return false;

	// else

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////
// ESTABLISHMENT
// Internal functions (do not call directly from framework)

void establishment_lpj(Stand& stand,Patch& patch) {

	// DESCRIPTION
	// Establishment in population (standard LPJ) mode.
	// Simulates population increase through establishment each simulation year for
	// trees and grasses and introduces new PFTs if climate conditions become suitable.
	// This function assumes each Individual object represents the average individual
	// of a PFT population, and that there is (at most) one Individual object per PFT
	// per modelled area (stand and patch).
	// This is the only function in which new average individuals are added to the
	// vegetation in population mode.

	const double K_EST=0.12; // maximum overall sapling establishment rate (indiv/m2)

	double fpc_tree; // summed fractional cover for tree PFTs
	double fpc_grass; // summed fractional cover for grass PFTs
	double est_tree;
		// overall establishment rate for trees on modelled area basis (indiv/m2)
	double est_pft;
		// establishment rate for a particular PFT on modelled area basis (for trees,
		// indiv/m2; for grasses, fraction of modelled area colonised)
	int ntree_est; // number of establishing tree PFTs
	int ngrass_est; // number of establishing grass PFTs
	bool present;

	// Obtain reference to Vegetation object

	Vegetation& vegetation=patch.vegetation;

	// Loop through all possible PFTs to determine whether there are any not currently
	// present but within their bioclimatic limits for establishment

	pftlist.firstobj();
	while (pftlist.isobj) {
		Pft& pft=pftlist.getobj();

		if (stand.pft[pft.id].active) {	//standpft.active is set in landcover_init according to rules for each stand

		// Is this PFT already represented?

			present=false;
			vegetation.firstobj();
			while (vegetation.isobj && !present) {
				Individual& indiv=vegetation.getobj();
				if (indiv.pft.id==pft.id) present=true;
				vegetation.nextobj();
			}

			if (!present) {
				if (establish(patch,stand.get_climate(),pft)) {

					// Not present but can establish, so introduce as new average
					// individual

					Individual& indiv=vegetation.createobj(pft,vegetation);
					if (pft.lifeform==GRASS) {
						indiv.height=0.0;
						indiv.crownarea=1.0; // (value not used)
						indiv.densindiv=1.0;
						indiv.fpc=0.0;
					}
				}
			}
		}
		pftlist.nextobj(); // ... on to next PFT
	}

	// Calculate total FPC and number of PFT's (i.e. average individuals) establishing
	// this year for trees and grasses respectively

	fpc_tree=0.0;
	fpc_grass=0.0;
	ntree_est=0;
	ngrass_est=0;

	// Loop through individuals

	vegetation.firstobj();
	while (vegetation.isobj) {
		Individual& indiv=vegetation.getobj();

		// For this individual ...

		if (indiv.pft.lifeform==TREE) {
			if (establish(patch, stand.get_climate(), indiv.pft)) ntree_est++;
			fpc_tree+=indiv.fpc;
		}
		else if (indiv.pft.lifeform==GRASS) {
			fpc_grass+=indiv.fpc;
			if (establish(patch, stand.get_climate(), indiv.pft)) ngrass_est++;
		}

		vegetation.nextobj(); // ... on to next individual
	}

	// Calculate overall establishment rate for trees
	// Trees can establish in unoccupied regions of the stand, and in regions
	// occupied by grasses. Establishment reduced in response to canopy closure as
	// tree cover approaches 100%
	// Smith et al 2001, Eqn (17)

	est_tree=K_EST*(1.0-exp(-5.0*(1.0-fpc_tree)))*(1.0-fpc_tree);

	// Loop through individuals

	vegetation.firstobj();
	while (vegetation.isobj) {

		// For this individual ...

		Individual& indiv=vegetation.getobj();

		if (indiv.pft.lifeform==TREE && establish(patch, stand.get_climate(), indiv.pft)) {

			// ESTABLISHMENT OF NEW TREE SAPLINGS
			// Partition overall establishment equally among establishing PFTs

			est_pft=est_tree/(double)ntree_est;

			if (est_pft<0.0)
				fail("establishment_area: negative establishment rate");

			// Adjust density of individuals across modelled area to account for
			// addition of new saplings

			indiv.densindiv+=est_pft;

			// Account for flux from the atmosphere to new saplings
			// (flux is downward and therefore negative)
			if(!indiv.istruecrop_or_intercropgrass()) {
				indiv.report_flux(Fluxes::ESTC,
								  -(indiv.pft.regen.cmass_leaf+
									indiv.pft.regen.cmass_root+indiv.pft.regen.cmass_sap+
									indiv.pft.regen.cmass_heart)*est_pft);
			}

			// Adjust average individual C biomass based on average biomass and density
			// of the new saplings

			indiv.cmass_leaf+=indiv.pft.regen.cmass_leaf*est_pft;
			indiv.cmass_root+=indiv.pft.regen.cmass_root*est_pft;
			indiv.cmass_sap+=indiv.pft.regen.cmass_sap*est_pft;
			indiv.cmass_heart+=indiv.pft.regen.cmass_heart*est_pft;

			// Calculate storage pool size
			indiv.max_n_storage = (indiv.cmass_leaf + indiv.cmass_root) / indiv.pft.cton_leaf_avr;
			if (!indiv.alive)
				indiv.scale_n_storage = indiv.max_n_storage * indiv.pft.cton_leaf_avr /
					((indiv.pft.regen.cmass_leaf + indiv.pft.regen.cmass_root +
					indiv.pft.regen.cmass_sap +	indiv.pft.regen.cmass_heart) * est_pft);
		}
		else if ((indiv.pft.lifeform==GRASS && indiv.pft.phenology!=CROPGREEN) &&
			establish(patch, stand.get_climate(), indiv.pft)) {

			// ESTABLISHMENT OF GRASSES
			// Grasses establish throughout unoccupied regions of the grid cell
			// Overall establishment partitioned equally among establishing PFTs

			est_pft=(1.0-fpc_tree-fpc_grass)/(double)ngrass_est;

			// Account for flux from atmosphere to grass regeneration
			if (!indiv.istruecrop_or_intercropgrass()) {
				indiv.report_flux(Fluxes::ESTC,
			                  -(indiv.pft.regen.cmass_leaf+
			                    indiv.pft.regen.cmass_root)*est_pft);
			}

			// Add regeneration biomass to overall biomass

			indiv.cmass_leaf+=est_pft*indiv.pft.regen.cmass_leaf;
			indiv.cmass_root+=est_pft*indiv.pft.regen.cmass_root;

			// Calculate storage pool size
			indiv.max_n_storage = (indiv.cmass_leaf + indiv.cmass_root) / indiv.pft.cton_leaf_avr;
			if (!indiv.alive)
				indiv.scale_n_storage = indiv.max_n_storage * indiv.pft.cton_leaf_avr /
					((indiv.pft.regen.cmass_leaf + indiv.pft.regen.cmass_root) * est_pft);
		}

		// Update allometry to account for changes in average individual biomass

		allometry(indiv);

		vegetation.nextobj(); // ... on to next individual
	}
}


void establishment_guess(Stand& stand,Patch& patch) {

	// DESCRIPTION
	// Establishment in cohort or individual mode.
	// Establishes new tree saplings and simulates grass population increase each
	// establishment year (every year in individual mode). New grass PFTs are
	// introduced (any year) if climate conditions become suitable.
	// Saplings are given an amount of carbon proportional to the hypothetical forest
	// floor NPP for this year and are "moulded" by calls to the standard allocation
	// function (growth module). The coefficient SAPSIZE controls the relative amount
	// of carbon given each sapling and should be set to a value resulting in saplings
	// around breast height in the first year (before development of a shading canopy).

	// For tree PFTs within their bioclimatic limits for establishment, the expected
	// number of saplings (est) established in each patch is given by:
	//
	// for model initialisation:
	//   (1) est = est_max*patcharea
	// if spatial mass effect enabled (stand-level "propagule pool" influences
	// establishment):
	//   (2) est = c*(kest_repr*cmass_repr+kest_bg)
	// if no spatial mass effect and propagule pool exists (cmass_repr non-negligible):
	//   (3) est = c*(kest_pres+kest_bg)
	// if no spatial mass effect and no propagule pool:
	//   (4) est = c*kest_bg
	// where
	//   (5) c = mu(anetps_ff/anetps_ff_max)*est_max*patcharea
	//   (6) mu(F) = exp(alphar*(1-1/F)) (Fulton 1991, Eqn 4)
	// where
	//   kest_repr     = PFT-specific constant (kest_repr*cmass_repr should
	//                   approximately equal 1 at the highest plausible value of
	//                   cmass_repr for the PFT)
	//   kest_bg       = PFT-specific constant in range 0-1 (usually << 1)
	//                   (set to zero if background establishment disabled)
	//   kest_pres     = PFT-specific constant in range 0-1 (usually ~1)
	//   cmass_repr    = net C allocated to reproduction for this PFT in all patches of
	//                   this stand this year (kgC/m2)
	//   est_max       = (nominal) maximum establishment rate for this PFT
	//                  (saplings/m2/year)
	//   patcharea     = patch area (m2)
	//   anetps_ff     = potential annual net assimilation at forest floor for this
	//                   PFT (kgC/m2/year)
	//   anetps_ff_max = maximum value of anetps_ff for this PFT in this stand so far
	//                   in the simulation (kgC/m2/year)
	//   alphar        = PFT-specific constant (parameter capturing non-linearity in
	//                   recruitment rate relative to understorey growing conditions)
	//
	// In individual mode, and in cohort mode if stochastic establishment is enabled,
	// the actual number of new saplings established is a number drawn from the Poisson
	// distribution with expectation 'est'. In cohort mode, with stochastic
	// establishment disabled, a cohort representing exactly 'est' individuals (may be
	// not-integral) is established.

	const double SAPSIZE=0.1;
		// coefficient in calculation of initial sapling size and initial
		// grass biomass (see comment above)

	bool present; // whether PFT already present in this patch
	double c; // constant in equation for number of new saplings (Eqn 5)
	double est; // expected number of new saplings for PFT in this patch
	double nsapling;
		// actual number of new saplings for PFT in this patch (may include a
		// fractional part in cohort mode with stochastic establishment disabled)
	double bminit; // initial sapling biomass (kgC) or new grass biomass (kgC/m2)
	double ltor; // leaf to fine root mass ratio for new saplings or grass
	int newindiv; // number of new Individual objects to add to vegetation for this PFT
	double kest_bg;
	int i;

	// Obtain reference to Vegetation object

	Vegetation& vegetation=patch.vegetation;

	const bool establish_active_pfts_before_management = true;

	// guess2008 - determine the number of woody PFTs that can establish
	// Thomas Hickler
	int nwoodypfts_estab=0;
	pftlist.firstobj();
	while (pftlist.isobj) {
		Pft& pft=pftlist.getobj();
		Standpft& standpft=stand.pft[pft.id];

		bool force_planting = patch.plant_this_year && standpft.plant;
		bool est_this_year;
		if(establish_active_pfts_before_management)
			est_this_year = !patch.managed || !patch.plant_this_year && standpft.reestab;
		else
			est_this_year = !run_landcover || !patch.plant_this_year && standpft.reestab;

		if (establish(patch, stand.get_climate(), pft) && pft.lifeform == TREE && standpft.active && (est_this_year || force_planting))
			nwoodypfts_estab++;
		pftlist.nextobj();
	}


	// Loop through PFTs

	pftlist.firstobj();
	while (pftlist.isobj) {
		Pft& pft=pftlist.getobj();
		Standpft& standpft=stand.pft[pft.id];

		// For this PFT ...

		// Stands cloned this year to be treated here as first year
		bool init_clone = date.year == stand.clone_year && pft.landcover == stand.landcover;

		// No grass establishment during planting year
		bool force_planting = patch.plant_this_year && standpft.plant;
		bool est_this_year;
		if(establish_active_pfts_before_management)
			est_this_year = !patch.managed || !patch.plant_this_year && standpft.reestab;
		else
			est_this_year = !run_landcover || !patch.plant_this_year && standpft.reestab;

		if (stand.pft[pft.id].active) {
			if (patch.age==0 || init_clone) {
				patch.pft[pft.id].anetps_ff_est=patch.pft[pft.id].anetps_ff;
				patch.pft[pft.id].wscal_mean_est=patch.pft[pft.id].wscal_mean;

				// BLARP
				if (date.year==0 || date.year==stand.first_year || init_clone)
					patch.pft[pft.id].anetps_ff_est_initial=patch.pft[pft.id].anetps_ff;

			}
			else {
				patch.pft[pft.id].anetps_ff_est+=patch.pft[pft.id].anetps_ff;
				patch.pft[pft.id].wscal_mean_est+=patch.pft[pft.id].wscal_mean;
			}

			if (establish(patch, stand.get_climate(), pft) && (est_this_year || force_planting)) {

				if (pft.lifeform==GRASS) {

					// ESTABLISHMENT OF GRASSES

					// Each grass PFT represented by just one individual in each patch
					// Check whether this grass PFT already represented ...

					present=false;
					vegetation.firstobj();
					while (vegetation.isobj && !present) {
						if (vegetation.getobj().pft.id==pft.id) present=true;
						vegetation.nextobj();
					}

					if (!present) {

						// ... if not, add it

						Individual& indiv=vegetation.createobj(pft,vegetation);
						indiv.height=0.0;
						indiv.crownarea=1.0; // (value not used)
						indiv.densindiv=1.0;
						indiv.fpc=1.0;

						// Initial grass biomass proportional to potential forest floor
						// net assimilation this year on patch area basis

						if(pft.phenology == CROPGREEN)
							bminit = SAPSIZE * 0.01;
						else if(patch.has_disturbances() && patch.disturbed)
							bminit = SAPSIZE * patch.pft[pft.id].anetps_ff_est_initial;
						else
							bminit = SAPSIZE * patch.pft[pft.id].anetps_ff;

						// Initial leaf to fine root biomass ratio based on
						// hypothetical value of water stress parameter

						ltor=patch.pft[pft.id].wscal_mean*pft.ltor_max;

						// Allocate initial biomass
						if(!indiv.istruecrop_or_intercropgrass())
							allocation_init(bminit,ltor,indiv);

						// Calculate initial allometry

						allometry(indiv);

						// Calculate storage pool size
						if(indiv.pft.phenology == CROPGREEN) {
							indiv.max_n_storage = CMASS_SEED / indiv.pft.cton_leaf_avr;
							indiv.scale_n_storage = indiv.max_n_storage * indiv.pft.cton_leaf_avr / CMASS_SEED;
						}
						else {
							indiv.max_n_storage = indiv.cmass_root * indiv.pft.fnstorage / indiv.pft.cton_leaf_avr;
							indiv.scale_n_storage = indiv.max_n_storage * indiv.pft.cton_leaf_avr / bminit;
						}

						// Establishment flux is not debited for 'new' Individual
						// objects - their carbon is debited in function growth()
						// if they survive the first year
					}
				}
				else if (pft.lifeform==TREE) {

					// ESTABLISHMENT OF NEW TREE SAPLINGS

					if (patch.age == 0 || init_clone){

						// First simulation year - initialising patch
						// Eqn 1

						est=pft.est_max*patcharea;
					}
					else {

						// Every year except year 1
						// Eqns 5, 6

						if (patch.pft[pft.id].anetps_ff>0.0 &&
							!negligible(patch.pft[pft.id].anetps_ff)) {

							c=exp(pft.alphar-pft.alphar/patch.pft[pft.id].anetps_ff*
								stand.pft[pft.id].anetps_ff_max)*pft.est_max*patcharea;
						}
						else
							c=0.0;

						// Background establishment enabled?

						if (ifbgestab)
							kest_bg=pft.kest_bg;
						else
							kest_bg=0.0;

						// Spatial mass effect enabled?
						// Eqns 2, 3, 4

						if (ifsme)
							est=c*(pft.kest_repr*stand.pft[pft.id].cmass_repr+kest_bg);
						else if (!negligible(stand.pft[pft.id].cmass_repr))
							est=c*(pft.kest_pres+kest_bg);
						else
							est=c*kest_bg;
					}


					// guess2008 - scale est by the number of woody PFTs/species that can establish
					// Otherwise, simply adding more PFTs or species would increase est
					est*=3.0/double(nwoodypfts_estab);


					// Have a value for expected number of new saplings (est)
					// Actual number of new saplings drawn from the Poisson distribution
					// (except cohort mode with stochastic establishment disabled)

					if (ifstochestab && !force_planting || vegmode==INDIVIDUAL) nsapling=randpoisson(est, stand.seed);
					else nsapling=est;

					if (vegmode==COHORT) {

						// BLARP added for OECD experiment (is this sensible?)
						if (patch.has_disturbances() && patch.disturbed || force_planting) {

							patch.pft[pft.id].anetps_ff_est=
								patch.pft[pft.id].anetps_ff_est_initial;
							patch.pft[pft.id].wscal_mean_est=patch.pft[pft.id].wscal_mean;
							newindiv=!negligible(nsapling);
						}
						else if (patch.age%estinterval && !init_clone || patch.plant_this_year) {

							// Not an establishment year - save sapling count for
							// establishment the next establishment year
							// Establishment each year in managed patches

							patch.pft[pft.id].nsapling+=nsapling;
							newindiv=0;
						}
						else {
							if (patch.age && !init_clone) {// all except first year after disturbance
								nsapling+=patch.pft[pft.id].nsapling;
								patch.pft[pft.id].anetps_ff_est/=(double)estinterval;
								patch.pft[pft.id].wscal_mean_est/=(double)estinterval;
							}
							newindiv=!negligible(nsapling);
								// round down to 0 if nsapling very small
						}
					}
					else if (vegmode == INDIVIDUAL) {
						newindiv=(int)(nsapling+0.5); // round up to be on the safe side
					}

					// Now create 'newindiv' new Individual objects

					for (i=0;i<newindiv;i++) {

						// Create average individual for a new cohort (cohort mode)
						// or an actual individual (individual mode)

						Individual& indiv=vegetation.createobj(pft,vegetation);

						if (vegmode==COHORT)
							indiv.densindiv=nsapling/patcharea;
						else if (vegmode==INDIVIDUAL)
							indiv.densindiv=1.0/patcharea;

						indiv.age=0.0;

						// Initial biomass proportional to potential forest floor net
						// assimilation for this PFT in this patch

						bminit=SAPSIZE*patch.pft[pft.id].anetps_ff_est;

						// Initial leaf to fine root biomass ratio based on hypothetical
						// value of water stress parameter

						ltor=patch.pft[pft.id].wscal_mean_est*pft.ltor_max;

						// Allocate initial biomass

						allocation_init(bminit,ltor,indiv);

						// Calculate initial allometry

						allometry(indiv);

						// Calculate storage pool size
						indiv.max_n_storage = indiv.cmass_sap * indiv.pft.fnstorage / indiv.pft.cton_leaf_avr;
						indiv.scale_n_storage = indiv.max_n_storage * indiv.pft.cton_leaf_avr / bminit;

						// Establishment flux is not debited for 'new' Individual
						// objects - their carbon is debited in function growth()
						// if they survive the first year
					}
				}
			}

			// Reset running sums for next year (establishment years only in cohort mode)

			if (vegmode!=COHORT || !(patch.age%estinterval) && !patch.plant_this_year) {
				patch.pft[pft.id].nsapling=0.0;
				patch.pft[pft.id].wscal_mean_est=0.0;
				patch.pft[pft.id].anetps_ff_est=0.0;
			}

		}

		// ... on to next PFT
		pftlist.nextobj();
	}
}


///////////////////////////////////////////////////////////////////////////////////////
// MORTALITY
// Internal functions (do not call directly from framework)

void mortality_lpj(Stand& stand, Patch& patch, const Climate& climate, double fireprob) {

	// DESCRIPTION
	// Mortality in population (standard LPJ) mode.
	// Simulates population decrease through mortality each simulation year for trees
	// and grasses; "kills" PFT populations if climate conditions become unsuitable for
	// survival.
	// This function assumes each Individual object represents the average individual
	// of a PFT population, and that there is (at most) one Individual object per PFT
	// per modelled area (stand and patch).

	// For tree PFTs, the fraction of the population killed is given by:
	//
	//   (1) mort = min ( mort_greff + mort_shade + mort_fire, 1)
	// where mort_greff = mortality preempted by low growth efficiency, given by:
	//   (2) mort_greff = K_MORT1 / (1 + K_MORT2*greff)
	//   (3) greff = annual_npp / leaf_area
	//   (4) leaf_area = cmass_leaf * sla
	// mort_shade = mortality due to shading ("self thinning") as total tree cover
	//   approaches 1 (see code)
	// mort_fire = mortality due to fire; the fraction of the modelled area affected by
	//   fire (fireprob) is calculated in function fire; actual mortality is influenced
	//   by PFT-specific resistance to burning (see code).
	//
	// Grasses are subject to shading and fire mortality only

	// References: Sitch et al 2001, Smith et al 2001, Thonicke et al 2001

	// INPUT PARAMETER
	// fireprob = fraction of modelled area affected by fire

	const double K_MORT1=0.01; // constant in mortality equation [c.f. mort_max; LPJF]
	const double K_MORT2=35.0;
		// constant in mortality equation [c.f. k_mort; LPJF]; the value here differs
		// from LPJF's k_mort in that growth efficiency here based on annual NPP, c.f.
		// net growth increment; LPJF
	const double FPC_TREE_MAX=0.95; // maximum summed tree fractional projective cover

	double fpc_dec; // required reduction in FPC
	double fpc_tree; // summed FPC for all tree PFTs
	double fpc_grass; // summed FPC for all grasses
	double deltafpc_tree_total; // total tree increase in FPC this year
	double mort;
		// total mortality for PFT (fraction of current FPC)
	double mort_greffic;
		// background mortality plus mortality preempted by low growth efficiency
		// (fraction of current FPC)
	double mort_shade;
		// mortality associated with light competition (fraction of current FPC)
	double mort_fire;
	bool killed;

	// Obtain reference Vegetation object

	Vegetation& vegetation=patch.vegetation;

	// Calculate total tree and grass FPC and total increase in FPC this year for trees

	fpc_tree=0.0;
	fpc_grass=0.0;
	deltafpc_tree_total=0.0;

	// Loop through individuals

	vegetation.firstobj();
	while (vegetation.isobj) {
		Individual& indiv=vegetation.getobj();

		// For this individual ...

		if (indiv.pft.lifeform==TREE) {

			fpc_tree+=indiv.fpc;
			if (indiv.deltafpc<0.0) indiv.deltafpc=0.0;
			deltafpc_tree_total+=indiv.deltafpc;

		}
		else if (indiv.pft.lifeform==GRASS) fpc_grass+=indiv.fpc;

		vegetation.nextobj(); // ... on to next individual
	}

	// KILL PFTs BEYOND BIOCLIMATIC LIMITS FOR SURVIVAL

	// Loop through individuals

	vegetation.firstobj();
	while (vegetation.isobj) {
		Individual& indiv=vegetation.getobj();

		// For this individual ...

		killed=false;

		if (!survive(climate,indiv.pft)) {

			// Remove completely if PFT beyond its bioclimatic limits for survival

			indiv.kill();

			vegetation.killobj();
			killed=true;
		}

		if (!killed) vegetation.nextobj(); // ... on to next individual
	}

	// CALCULATE AND IMPLEMENT MORTALITY

	// Loop through individuals

	vegetation.firstobj();
	while (vegetation.isobj) {
		Individual& indiv=vegetation.getobj();

		// For this individual ...

		killed=false;

		if (indiv.pft.lifeform==TREE) {

			// TREE MORTALITY

			// Mortality associated with low growth efficiency (includes
			// parameterisation of population background mortality)
			// NOTE: growth efficiency quantified as (annual NPP)/(leaf area)
			//       c.f. LPJF, which uses net growth increment instead of NPP

			mort_greffic=K_MORT1/(1.0+K_MORT2*max(indiv.anpp,0.0)/indiv.cmass_leaf/
				indiv.pft.sla);

			// Mortality associated with light competition
			// Self thinning imposed when total tree cover above FPC_TREE_MAX,
			// partitioned among tree PFTs in proportion to this year's FPC increment

			if (fpc_tree>FPC_TREE_MAX) {
				if (!negligible(deltafpc_tree_total))
					fpc_dec=(fpc_tree-FPC_TREE_MAX)*indiv.deltafpc/deltafpc_tree_total;
				else
					fpc_dec=0.0;
				mort_shade=1.0-fracmass_lpj(indiv.fpc-fpc_dec,indiv.fpc,indiv);
			}
			else
				mort_shade=0.0;

			// Mortality due to fire

			if (patch.has_fires()) mort_fire=fireprob*(1.0-indiv.pft.fireresist);
			else mort_fire=0.0;

			// Sum mortality components to give total mortality (maximum 1)

			mort=min(mort_greffic+mort_shade+mort_fire,1.0);

			// Reduce population density and C biomass on modelled area basis
			// to account for loss of killed individuals
			indiv.reduce_biomass(mort, mort_fire);
		}
		else if (indiv.pft.lifeform==GRASS) {

			// GRASS MORTALITY

			if (indiv.pft.landcover==CROPLAND)
				fpc_grass=indiv.fpc;

			// Shading mortality: grasses can persist only on regions not occupied
			// by trees

			if (fpc_grass>1.0-min(fpc_tree,FPC_TREE_MAX)) {
				fpc_dec=(fpc_grass-1.0+min(fpc_tree,FPC_TREE_MAX))*indiv.fpc/fpc_grass;
				mort_shade=1.0-fracmass_lpj(indiv	.fpc-fpc_dec,indiv.fpc,indiv);
			}
			else
				mort_shade=0.0;

			if (mort_shade>0.0) {
				mort_shade=mort_shade;
			}

			// Mortality due to fire

			if (patch.has_fires())
				mort_fire=fireprob*(1.0-indiv.pft.fireresist);
			else mort_fire=0.0;

			// Sum mortality components to give total mortality (maximum 1)

			mort=min(mort_shade+mort_fire,1.0);

			// Reduce C biomass on modelled area basis to account for biomass lost
			// through mortality
			indiv.reduce_biomass(mort, mort_fire);
		}

		// Remove this PFT population completely if all individuals killed

		if (negligible(indiv.densindiv)) {
			vegetation.killobj();
			killed=true;
		}

		// Update allometry

		else allometry(indiv);

		if (!killed) vegetation.nextobj(); // ... on to next individual
	}
}


void mortality_guess(Stand& stand, Patch& patch, const Climate& climate, double fireprob) {

	// DESCRIPTION
	// Mortality in cohort and individual modes.
	// Simulates death of individuals or reduction in individual density through
	// mortality (including fire mortality) each simulation year for trees, and
	// reduction in biomass due to fire for grasses; kills individuals/cohorts if
	// climate conditions become unsuitable for survival.
	//
	// For trees, death can result from the following factors:
	//
	//   (1) a background mortality rate related to PFT mean longevity;
	//   (2) when mean growth efficiency over an integration period (five years) falls
	//       below a PFT-specific threshold;
	//   (3) stress associated with high temperatures (a "soft" bioclimatic limit,
	//       intended mainly for boreal tree PFTs; Sitch et al 2001, Eqn 55);
	//   (4) fire (probability calculated in function fire)
	//   (5) when climatic conditions are such that all individuals of the PFT are
	//       killed.
	//
	// For grasses, (4) and (5) above are modelled only.
	//
	// For trees and in cohort mode, mortality may be stochastic or deterministic
	// (according to the value of global variable ifstochmort). In stochastic mode,
	// expected mortality rates are imposed as stochastic probabilities of death; in
	// deterministic mode, cohort density is reduced by the fraction represented by
	// the mortality rate.

	if(patch.managed_this_year)
		return;

	// INPUT PARAMETER
	// fireprob = probability of fire in this patch

	double mort;
		// overall mortality (excluding fire mortality): fraction of cohort killed, or:
		// probability of individual being killed
	double mort_min;
		// background component of overall mortality (see 'mort')
	double mort_greff;
		// component of overall mortality associated with low growth efficiency
	double mort_fire;
		// expected fraction of cohort killed (or: probability of individual being
		// killed) due to fire
	double frac_survive;
		// fraction of cohort (or individual) surviving
	double greff;
		// growth efficiency for individual/cohort this year (kgC/m2 leaf/year)
	double greff_mean;
		// five-year-mean growth efficiency (kgC/m2 leaf/year)
	int nindiv; // number of individuals (remaining) in cohort
	int nindiv_prev; // number of individuals in cohort prior to mortality
	int i;
	bool killed;

	const double KMORTGREFF=0.3;
		// value of mort_greff when growth efficiency below PFT-specific threshold
	const double KMORTBG_LNF=-log(0.001);
		// coefficient in calculation of background mortality (negated natural log of
		// fraction of population expected to survive to age 'longevity'; see Eqn 14
		// below)
	const double KMORTBG_Q=2.0;
		// exponent in calculation of background mortality (shape parameter for
		// relationship between mortality and age (0=constant mortality; 1=linear
		// increase; >1->exponential; steepness increases for increasing positive
		// values)

	// Obtain reference to Vegetation object for this patch

	Vegetation& vegetation=patch.vegetation;

	// FIRE MORTALITY
	if (patch.has_fires()) {

		// Impose fire in this patch with probability 'fireprob'

		if (randfrac(stand.seed)<fireprob) {

			// Loop through individuals

			vegetation.firstobj();
			while (vegetation.isobj) {
				Individual& indiv=vegetation.getobj();

				// For this individual ...

				killed=false;

				// Fraction of biomass destroyed by fire

				mort_fire=1.0-indiv.pft.fireresist;

				if (indiv.pft.lifeform==GRASS) {

					// GRASS PFT

					// Reduce individual biomass
					indiv.reduce_biomass(mort_fire, mort_fire);

					// Update allometry

					allometry(indiv);
				}
				else {

					// TREE PFT

					if (ifstochmort) {

						// Impose stochastic mortality
						// Each individual in cohort dies with probability 'mort_fire'

						// Number of individuals represented by 'indiv'
						// (round up to be on the safe side)

						nindiv=(int)(indiv.densindiv*patcharea+0.5);
						nindiv_prev=nindiv;

						for (i=0;i<nindiv_prev;i++)
							if (randfrac(stand.seed)>indiv.pft.fireresist) nindiv--;

						if (nindiv_prev)
							frac_survive=(double)nindiv/(double)nindiv_prev;
						else
							frac_survive=0.0;
					}

					// Deterministic mortality (cohort mode only)

					else frac_survive=1.0-mort_fire;

					// Reduce individual biomass on patch area basis
					// to account for loss of killed individuals
					indiv.reduce_biomass(1.0 - frac_survive, 1.0 - frac_survive);

					// Remove this cohort completely if all individuals killed
					// (in individual mode: removes individual if killed)

					if (negligible(indiv.densindiv)) {
						vegetation.killobj();
						killed=true;
					}
				}

				if (!killed) vegetation.nextobj(); // ... on to next individual
			}
		}
	}

	// MORTALITY NOT ASSOCIATED WITH FIRE

	// Loop through individuals

	vegetation.firstobj();
	while (vegetation.isobj) {
		Individual& indiv=vegetation.getobj();

		// For this individual ...

		killed=false;

		// KILL INDIVIDUALS/COHORTS BEYOND BIOCLIMATIC LIMITS FOR SURVIVAL

		if (!survive(climate,indiv.pft)) {

			// Kill cohort/individual, transfer biomass to litter

			indiv.kill();

			vegetation.killobj();
			killed=true;
		}
		else {

			if (indiv.pft.lifeform==TREE) {

				// Calculate this year's growth efficiency
				// Eqn 31, Smith et al 2001

				if (!negligible(indiv.cmass_leaf))
					greff=max(indiv.anpp,0.0)/indiv.cmass_leaf/indiv.pft.sla;
				else
					greff=0.0;

				// Calculate 5 year mean growth efficiency

				indiv.greff_5.add(greff);
				greff_mean = indiv.greff_5.mean();

				// BACKGROUND MORTALITY
				//
				// This is now modelled as an increasing probability of death with
				// age. The expected likelihood of death by "background" factors at a
				// given age is given by the general relationship [** = raised to the
				// power of]:
				//  (1) mort(age) = min( Z * ( age / longevity ) ** Q , 1)
				// where Z         is a constant (value derived below);
				//       Q         is a positive constant
				//                 (or zero for constant mortality with age)
				//       longevity is the age at which the fraction of the initial
				//                 cohort expected to survive is a known value, F
                // It is possible to derive the value of Z given values for longevity,
				// Q and F, by integration:
				//
				// The rate of change of cohort size (P) at any age is given by:
				//  (2) dP/dage = -mort(age) * P
				// Combining (1) and (2),
				//  (3) dP/dage = -Z.(age/longevity)**Q.P
				// From (3),
				//  (4) (1/P) dP = -Z.(age/longevity)**Q.dage
				// Integrating the LHS of eqn 4 from P_0 (initial cohort size) to
				// P_longevity (cohort size at age=longevity); and the RHS of eqn 4
				// from 0 to longevity:
				//   (5) LHS = ln(P_longevity) - ln(P_0)
				//   (6)     = ln(P_longevity/P_0)
				//   (7)     = ln(P_0*F/P_0)
				//   (8)     = ln(F)
				//   (9) RHS = integral[0,longevity] -Z.(age/longevity)**Q.dage
				//  (10)     = integral[0,longevity] -Z.(1/longevity)**Q.age**Q.dage
				//  (11)     = -Z.(1/longevity)**Q.integral[0,longevity] age**Q.dage
				//  (12)     = -Z.(1/longevity)**Q.longevity**(Q+1)/(Q+1)
				//             (Zwillinger 1996, p 360)
				// Combining (8) and (12),
				//  (13) Z = - ln(F) * (Q+1) / longevity
				// Combining (1) and (13),
				//  (14) mort(age) =
				//         min ( -ln(F) * (Q+1) / longevity * (age/longevity)**P, 1)

				// Eqn 14
				mort_min=min(1.0,KMORTBG_LNF*(KMORTBG_Q+1)/indiv.pft.longevity*
					pow(indiv.age/indiv.pft.longevity,KMORTBG_Q));


				// Growth suppression mortality
				// Smith et al 2001; c.f. Pacala et al 1993, Eqn 5

				// guess2008 - introduce a smoothly-varying mort_greff - 5 is the exponent in the global validation
				if (ifsmoothgreffmort)
					mort_greff=KMORTGREFF/(1.0+pow((greff_mean/(indiv.pft.greff_min)),5.0));
				else {
					// Standard case, as in guess030124
					if (greff_mean<indiv.pft.greff_min)
						mort_greff=KMORTGREFF;
					else
						mort_greff=0.0;
				}


				// Increase growth efficiency mortality if summed crown area within
				// cohort exceeds 1 (to ensure self-thinning for shade-tolerant PFTs)

				if (vegmode==COHORT) {

					if (indiv.crownarea*indiv.densindiv>1.0)
						mort_greff=max((indiv.crownarea*indiv.densindiv-1.0)/
							(indiv.crownarea*indiv.densindiv),mort_greff);
				}

				// Overall mortality: c.f. Eqn 29, Smith et al 2001

				mort=mort_min+mort_greff-mort_min*mort_greff;


				// guess2008 - added safety check
				if (mort > 1.0 || mort < 0.0)
					fail("error in mortality_guess: bad mort value");

				if (ifstochmort) {

					// Impose stochastic mortality
					// Each individual in cohort dies with probability 'mort'

					nindiv=(int)(indiv.densindiv*patcharea+0.5);
					nindiv_prev=nindiv;

					for (i=0;i<nindiv_prev;i++)
						if (randfrac(stand.seed)<mort) nindiv--;

					if (nindiv_prev)
						frac_survive=(double)nindiv/(double)nindiv_prev;
					else
						frac_survive=0.0;
				}

				// Deterministic mortality (cohort mode only)

				else frac_survive=1.0-mort;

				// Reduce individual density and biomass on patch area basis
				// to account for loss of killed individuals
				indiv.reduce_biomass(1.0 - frac_survive, 0.0);

				// Remove this cohort completely if all individuals killed
				// (in individual mode: removes individual if killed)

				if (negligible(indiv.densindiv)) {
					vegetation.killobj();
					killed=true;
				}

				// Update allometry

				else allometry(indiv);
			}
		}

		// ... on to next individual

		if (!killed) vegetation.nextobj();
	}
}


///////////////////////////////////////////////////////////////////////////////////////
// FIRE DISTURBANCE
// Internal function (do not call directly from framework)

void fire(Patch& patch,double& fireprob) {

	// DESCRIPTION
	// Calculates probability/incidence of disturbance by fire; implements
	// volatilisation of above-ground litter due to fire.
	// Mortality due to fire implemented in mortality functions, not here.
	//
	// Reference: Thonicke et al 2001, with updated Eqn 9

	// OUTPUT PARAMETER
	// fireprob = probability of fire in this patch this year
	//            (in population mode: fraction of modelled area affected by fire)

	const double MINFUEL=0.2;
		// Minimum total aboveground litter required for fire (kgC/m2)
	double litter_ag;
		// total aboveground litter (kgC/m2) [litter_ag_total/1000, fuel/1000; LPJF]
	double me_mean;
		// mean litter flammability moisture threshold [=moisture of extinction, me;
		// Thonicke et al 2001 Eqn 2; moistfactor; LPJF]
	double pm;
		// probability of fire on a given day [p(m); Thonicke et al 2001, Eqn 2;
		// fire_prob; LPJF]
	double n;
		// summed length of fire season (days) [N; Thonicke et al 2001, Eqn 4;
		// fire_length; LPJF]
	double s;
		// annual mean daily probability of fire [s; Thonicke et al 2001, Eqn 8;
		// fire_index; LPJF]
	double sm; // s-1
	double mort_fire; // fire mortality as fraction of current FPC
	int p;

	// Calculate fuel load (total aboveground litter)

	litter_ag=0.0;

	// Loop through PFTs

	for (p=0;p<npft;p++) {
		litter_ag += patch.pft[p].litter_leaf + patch.pft[p].litter_sap + patch.pft[p].litter_heart +
			patch.pft[p].litter_repr;
	}

	// Soil Litter
	litter_ag += patch.soil.sompool[SURFSTRUCT].cmass + patch.soil.sompool[SURFMETA].cmass +
		patch.soil.sompool[SURFFWD].cmass + patch.soil.sompool[SURFCWD].cmass;

	// Prevent fire if fuel load below prescribed threshold

	if (litter_ag<MINFUEL) {
		fireprob=0.0;
		return;
	}

	// Calculate mean litter flammability moisture threshold

	me_mean=0.0;

	// Loop through PFTs

	for (p=0;p<npft;p++) {
		me_mean += (patch.pft[p].litter_leaf + patch.pft[p].litter_sap + patch.pft[p].litter_heart +
			patch.pft[p].litter_repr) * patch.pft[p].pft.litterme / litter_ag;
	}

	// Soil litter
	me_mean += (patch.soil.sompool[SURFSTRUCT].cmass * patch.soil.sompool[SURFSTRUCT].litterme +
		patch.soil.sompool[SURFMETA].cmass * patch.soil.sompool[SURFMETA].litterme +
		patch.soil.sompool[SURFFWD].cmass * patch.soil.sompool[SURFFWD].litterme +
		patch.soil.sompool[SURFCWD].cmass * patch.soil.sompool[SURFCWD].litterme) / litter_ag;

	// Calculate length of fire season in days
	// Eqn 2, 4, Thonicke et al 2001
	// NOTE: error in Eqn 2, Thonicke et al - multiplier should be PI/4, not PI

	n=0.0;
	for (int day = 0; day < date.year_length(); day++) {

		// Eqn 2
		pm=exp(-PI*patch.soil.dwcontupper[day]/me_mean*patch.soil.dwcontupper[day]/
			me_mean);

		// Eqn 4
		n+=pm;
	}

	// Calculate fraction of grid cell burnt
	// Thonicke et al 2001, Eqn 9

	s=n/date.year_length();
	sm=s-1;

	fireprob=s*exp(sm/(0.45*sm*sm*sm+2.83*sm*sm+2.96*sm+1.04));

	if (fireprob>1.0)
		fail("fire: probability of fire >1.0");
	else if (fireprob<0.001) fireprob=0.001; // c.f. LPJF

	// Calculate expected flux from litter due to fire
	// (fluxes from vegetation calculated in mortality functions)

	for (p=0;p<npft;p++) {

		// Assume litter is as fire resistant as the vegetation it originates from

		mort_fire=fireprob*(1.0-patch.pft[p].pft.fireresist);

		// Calculate flux from burnt litter

		patch.fluxes.report_flux(Fluxes::FIREC,
		                         mort_fire*(patch.pft[p].litter_leaf + patch.pft[p].litter_sap +
		                                    patch.pft[p].litter_heart + patch.pft[p].litter_repr));

		report_fire_nfluxes(patch, mort_fire * (patch.pft[p].nmass_litter_leaf +
			                patch.pft[p].nmass_litter_sap + patch.pft[p].nmass_litter_heart));

		// Account for burnt above ground litter

		patch.pft[p].litter_leaf        *= (1.0 - mort_fire);
		patch.pft[p].litter_sap         *= (1.0 - mort_fire);
		patch.pft[p].litter_heart       *= (1.0 - mort_fire);
		patch.pft[p].litter_repr        *= (1.0 - mort_fire);
		patch.pft[p].nmass_litter_leaf  *= (1.0 - mort_fire);
		patch.pft[p].nmass_litter_sap   *= (1.0 - mort_fire);
		patch.pft[p].nmass_litter_heart *= (1.0 - mort_fire);
	}

	// Soil litter
	double mort_fire_struct = fireprob * (1.0 - patch.soil.sompool[SURFSTRUCT].fireresist);
	double mort_fire_meta   = fireprob * (1.0 - patch.soil.sompool[SURFMETA].fireresist);
	double mort_fire_fwd    = fireprob * (1.0 - patch.soil.sompool[SURFFWD].fireresist);
	double mort_fire_cwd    = fireprob * (1.0 - patch.soil.sompool[SURFCWD].fireresist);

	// Calculate flux from burnt soil litter
	patch.fluxes.report_flux(Fluxes::FIREC,patch.soil.sompool[SURFSTRUCT].cmass * mort_fire_struct +
	                                       patch.soil.sompool[SURFMETA].cmass   * mort_fire_meta +
	                                       patch.soil.sompool[SURFFWD].cmass    * mort_fire_fwd +
	                                       patch.soil.sompool[SURFCWD].cmass    * mort_fire_cwd);

	// Account for burnt above ground litter
	patch.soil.sompool[SURFSTRUCT].cmass *= (1.0 - mort_fire_struct);
	patch.soil.sompool[SURFMETA].cmass   *= (1.0 - mort_fire_meta);
	patch.soil.sompool[SURFFWD].cmass    *= (1.0 - mort_fire_fwd);
	patch.soil.sompool[SURFCWD].cmass    *= (1.0 - mort_fire_cwd);

	// Calculate nitrogen flux from burnt soil litter
	double nflux_fire = patch.soil.sompool[SURFSTRUCT].nmass * mort_fire_struct +
	                    patch.soil.sompool[SURFMETA].nmass   * mort_fire_meta +
	                    patch.soil.sompool[SURFFWD].nmass    * mort_fire_fwd +
						patch.soil.sompool[SURFCWD].nmass    * mort_fire_cwd;

	report_fire_nfluxes(patch, nflux_fire);

	patch.soil.sompool[SURFSTRUCT].nmass *= (1.0 - mort_fire_struct);
	patch.soil.sompool[SURFMETA].nmass   *= (1.0 - mort_fire_meta);
	patch.soil.sompool[SURFFWD].nmass    *= (1.0 - mort_fire_fwd);
	patch.soil.sompool[SURFCWD].nmass    *= (1.0 - mort_fire_cwd);
}


///////////////////////////////////////////////////////////////////////////////////////
// DISTURBANCE
// Generic patch-destroying disturbance with a prescribed probability
// Internal function - do not call from framework

void disturbance(Patch& patch, double disturb_prob) {

	// DESCRIPTION
	// Destroys all biomass in a patch with a certain stochastic probability.
	// Biomass enters the litter, which is not affected by the disturbance.
	// NB: cohort and individual mode only

	// INPUT PARAMETER
	// disturb_prob = the probability of a disturbance this year

	if (randfrac(patch.stand.seed)<disturb_prob) {

		Vegetation& vegetation = patch.vegetation;

		vegetation.firstobj();
		while (vegetation.isobj) {
			Individual& indiv = vegetation.getobj();

			indiv.kill();

			vegetation.killobj();
		}

		patch.disturbed = true;
		patch.age = 0;
	}

	else patch.disturbed = false;
}


///////////////////////////////////////////////////////////////////////////////////////
// VEGETATION DYNAMICS
// Should be called by framework at the end of each simulation year, after vegetation,
// climate and soil attributes have been updated

void vegetation_dynamics(Stand& stand,Patch& patch) {

	// DESCRIPTION
	// Implementation of fire disturbance and population dynamics (establishment and
	// mortality) at end of simulation year. Bioclimatic constraints to survival and
	// establishment are imposed within mortality and establishment functions
	// respectively.

	double fireprob = 0.0;
		// probability of fire in this patch this year
		// (in population mode: fraction of modelled area affected by fire this year)

	// Calculate fire probability and volatilise litter
	if (patch.has_fires()) {
		fire(patch,fireprob);
	}
	patch.fireprob = fireprob;

	if (vegmode == POPULATION) {

		// POPULATION MODE

		// Mortality
		mortality_lpj(stand, patch, stand.get_climate(), fireprob);

		// Establishment
		establishment_lpj(stand,patch);

	}
	else {

		// INDIVIDUAL AND COHORT MODES

		// Patch-destroying disturbance

		// Disturbance when N limitation is switched on to get right pft composition under N limitation faster
		if (ifcentury && ifnlim && date.year == freenyears){
			disturbance(patch, 1.0);
			if (patch.disturbed) {
				return; // no mortality or establishment this year
			}
		}

		// Normal disturbance with probability interval of distinterval
		if (patch.has_disturbances()) {

			// We don't allow disturbance while documenting for calculation of Century equilibrium
			bool during_century_solvesom = ifcentury &&
										   date.year >= patch.soil.solvesomcent_beginyr &&
										   date.year <= patch.soil.solvesomcent_endyr;

			if (patch.age && !during_century_solvesom && date.year != stand.clone_year) {
				disturbance(patch,1.0 / distinterval);
				if (patch.disturbed) {
					return; // no mortality or establishment this year
				}
			}
		}

		// Mortality
		mortality_guess(stand, patch, stand.get_climate(), fireprob);

		// Establishment
		establishment_guess(stand,patch);
	}

	patch.age++;
}


///////////////////////////////////////////////////////////////////////////////////////
// REFERENCES
//
// LPJF refers to the original FORTRAN implementation of LPJ as described by Sitch
//   et al 2001
// Fulton, MR 1991 Adult recruitment rate as a function of juvenile growth in size-
//   structured plant populations. Oikos 61: 102-105.
// Pacala SW, Canham, CD & Silander JA Jr 1993 Forest models defined by field
//   measurements: I. The design of a northeastern forest simulator. Canadian Journal
//   of Forest Research 23: 1980-1988.
// Prentice, IC, Sykes, MT & Cramer W 1993 A simulation model for the transient
//   effects of climate change on forest landscapes. Ecological Modelling 65:
//   51-70.
// Sitch, S, Prentice IC, Smith, B & Other LPJ Consortium Members (2000) LPJ - a
//   coupled model of vegetation dynamics and the terrestrial carbon cycle. In:
//   Sitch, S. The Role of Vegetation Dynamics in the Control of Atmospheric CO2
//   Content, PhD Thesis, Lund University, Lund, Sweden.
// Smith, B, Prentice, IC & Sykes, M (2001) Representation of vegetation dynamics in
//   the modelling of terrestrial ecosystems: comparing two contrasting approaches
//   within European climate space. Global Ecology and Biogeography 10: 621-637
// Thonicke, K, Venevsky, S, Sitch, S & Cramer, W (2001) The role of fire disturbance
//   for global vegetation dynamics: coupling fire into a Dynamic Global Vegetation
//   Model. Global Ecology and Biogeography 10: 661-677.
// Zwillinger, D 1996 CRC Standard Mathematical Tables and Formulae, 30th ed. CRC
//   Press, Boca Raton, Florida.
