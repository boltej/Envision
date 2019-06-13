///////////////////////////////////////////////////////////////////////////////////////
/// \file somdynam.cpp
/// \brief Soil organic matter dynamics
///
/// \author Ben Smith (LPJ SOM dynamics, CENTURY), David Wårlind (CENTURY)
/// $Date: 2017-04-05 15:04:09 +0200 (Wed, 05 Apr 2017) $
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
#include "somdynam.h"

#include "driver.h"
#include <assert.h>
#include <bitset>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////////
// FILE SCOPE GLOBAL CONSTANTS

// Turnover times (in years, approximate) for litter and SOM fractions at 10 deg C with
// ample moisture (Meentemeyer 1978; Foley 1995)

static const double TAU_LITTER=2.85; // Thonicke, Sitch, pers comm, 26/11/01
static const double TAU_SOILFAST=33.0;
static const double TAU_SOILSLOW=1000.0;

static const double TILLAGE_FACTOR = 33.0 / 17.0;
	// (Inverse of "tillage factor" in Chatskikh et al. 2009, Value selected by T.Pugh)

static const double FASTFRAC=0.985;
	// fraction of litter decomposition entering fast SOM pool
static const double ATMFRAC=0.7;
	// fraction of litter decomposition entering atmosphere

// Corresponds to the amount of soil available nitrogen where SOM C:N ratio reach
// their minimum (nitrogen saturation) (Parton et al 1993, Fig. 4)
// Comment: NMASS_SAT is too high when considering BNF - Zaehle
static const double NMASS_SAT = 0.002 * 0.05;
// Corresponds to the nitrogen concentration in litter where SOM C:N ratio reach
// their minimum (nitrogen saturation) (Parton et al 1993, Fig. 4)
static const double NCONC_SAT = 0.02;

///////////////////////////////////////////////////////////////////////////////////////
// FILE SCOPE GLOBAL VARIABLES

// Exponential decay constants for litter and SOM fractions
// Values set from turnover times (constants above) on first call to decayrates

static double k_litter10;
static double k_soilfast10;
static double k_soilslow10;

static bool firsttime=true;
	// indicates whether function decayrates has been called before


///////////////////////////////////////////////////////////////////////////////////////
// SETCONSTANTS
// Internal function (do not call directly from framework)

void setconstants() {

	// DESCRIPTION
	// Calculate exponential decay constants (annual basis) for litter and
	// SOM fractions first time function decayrates is called

	k_litter10=1.0/TAU_LITTER;
	k_soilfast10=1.0/TAU_SOILFAST;
	k_soilslow10=1.0/TAU_SOILSLOW;
	firsttime=false;
}


///////////////////////////////////////////////////////////////////////////////////////
// DECAYRATES
// Internal function (do not call directly from framework)
// used by som_dynamic_lpj()

void decayrates(double wcont,double gtemp_soil,double& k_soilfast,double& k_soilslow,
	double& fr_litter,double& fr_soilfast,double& fr_soilslow, bool tillage) {

	// DESCRIPTION
	// Calculation of fractional decay amounts for litter and fast and slow SOM
	// fractions given current soil moisture and temperature

	// INPUT PARAMETERS
	// wcont       = water content of upper soil layer (fraction of AWC)
	// gtemp_soil  = respiration temperature response incorporating damping of Q10
	//               response due to temperature acclimation (Eqn 11, Lloyd & Taylor
	//               1994)

	// OUTPUT PARAMETERS
	// k_soilfast  = adjusted daily decay constant for fast SOM fraction
	// k_soilslow  = adjusted daily decay constant for slow SOM fraction
	// fr_litter   = litter fraction remaining following today's decomposition
	// fr_soilfast = fast SOM fraction remaining following today's decomposition
	// fr_soilslow = slow SOM fraction remaining following today's decomposition

	double moist_response; // moisture modifier of decomposition rate

	// On first call only: set exponential decay constants

	if (firsttime) setconstants();

	// Calculate response of soil respiration rate to moisture content of upper soil layer
	// Foley 1995 Eqn 19

	moist_response=0.25+0.75*wcont;

	// Calculate litter and SOM fractions remaining following today's decomposition
	// (Sitch et al 2000 Eqn 71) adjusting exponential decay constants by moisture and
	// temperature responses and converting from annual to daily basis
	// NB: Temperature response (gtemp; Lloyd & Taylor 1994) set by framework

	k_soilfast=k_soilfast10*gtemp_soil*moist_response/date.year_length();
	if (tillage) {
		k_soilfast *= TILLAGE_FACTOR; // Increased HR for crops (tillage)
	}
	k_soilslow=k_soilslow10*gtemp_soil*moist_response/date.year_length();

	fr_litter=exp(-k_litter10*gtemp_soil*moist_response/date.year_length());
	fr_soilfast=exp(-k_soilfast);
	fr_soilslow=exp(-k_soilslow);
}


///////////////////////////////////////////////////////////////////////////////////////
// DECAYRATES
// Should be called by framework on last day of simulation year, following call to
// som_dynamics, once annual litter production and vegetation PFT composition are close
// to their long term equilibrium (typically 500-1000 simulation years).
// NB: should be called ONCE ONLY during simulation for a particular grid cell

void equilsom_lpj(Soil& soil) {

	// DESCRIPTION
	// Analytically solves differential flux equations for fast and slow SOM pools
	// assuming annual litter inputs close to long term equilibrium

	// INPUT PARAMETER (class defined in framework header file)
	// soil = current soil status

	double nyear;
		// number of years over which decay constants and litter inputs averaged

	nyear=soil.soiltype.solvesom_end-soil.soiltype.solvesom_begin+1;

	soil.decomp_litter_mean/=nyear;
	soil.k_soilfast_mean/=nyear;
	soil.k_soilslow_mean/=nyear;

	soil.cpool_fast=(1.0-ATMFRAC)*FASTFRAC*soil.decomp_litter_mean/
		soil.k_soilfast_mean;
	soil.cpool_slow=(1.0-ATMFRAC)*(1.0-FASTFRAC)*soil.decomp_litter_mean/
		soil.k_soilslow_mean;
}


///////////////////////////////////////////////////////////////////////////////////////
// SOM DYNAMICS
// To be called each simulation day for each modelled area or patch, following update
// of soil temperature and soil water.

void som_dynamics_lpj(Patch& patch, bool tillage) {

	// DESCRIPTION
	// Calculation of soil decomposition and transfer of C between litter and soil
	// organic matter pools.

	double k_soilfast; // adjusted daily decay constant for fast SOM fraction
	double k_soilslow; // adjusted daily decay constant for slow SOM fraction
	double fr_litter;
		// litter fraction remaining following one day's/one month's decomposition
	double fr_soilfast;
		// fast SOM fraction remaining following one day's/one month's decomposition
	double fr_soilslow;
		// slow SOM fraction remaining following one day's/one month's decomposition
	double decomp_litter; // litter decomposition today/this month (kgC/m2)
	double cflux; // accumulated C flux to atmosphere today/this month (kgC/m2)
	int p;

	// Obtain reference to Soil object
	Soil& soil=patch.soil;

	// Calculate decay constants and rates given today's soil moisture and
	// temperature

	decayrates(soil.wcont[0],soil.gtemp,k_soilfast,k_soilslow,fr_litter,
		fr_soilfast,fr_soilslow, tillage);

	// From year soil.solvesom_begin, update running means for later solution
	// (at year soil.solvesom_end) of equilibrium SOM pool sizes

	if (date.year>=soil.soiltype.solvesom_begin) {
		soil.k_soilfast_mean+=k_soilfast;
		soil.k_soilslow_mean+=k_soilslow;
	}

	// Reduce litter and SOM pools, sum C flux to atmosphere from decomposition
	// and transfer correct proportions of litter decomposition to fast and slow
	// SOM pools

	// Reduce individual litter pools and calculate total litter decomposition
	// for today/this month

	decomp_litter=0.0;

	// Loop through PFTs

	for (p=0;p<npft;p++) {	//NB. also inactive pft's

		// For this PFT ...

		decomp_litter+=(patch.pft[p].litter_leaf+
			patch.pft[p].litter_root+
			patch.pft[p].litter_sap+
			patch.pft[p].litter_heart+
			patch.pft[p].litter_repr)*(1.0-fr_litter);

		patch.pft[p].litter_leaf*=fr_litter;
		patch.pft[p].litter_root*=fr_litter;
		patch.pft[p].litter_sap*=fr_litter;
		patch.pft[p].litter_heart*=fr_litter;
		patch.pft[p].litter_repr*=fr_litter;
	}

	if (date.year>=soil.soiltype.solvesom_begin)
		soil.decomp_litter_mean+=decomp_litter;

	// Partition litter decomposition among fast and slow SOM pools
	// and flux to atmosphere

	// flux to atmosphere
	cflux=decomp_litter*ATMFRAC;

	// remaining decomposition - goes to ...
	decomp_litter-=cflux;

	// ... fast SOM pool ...
	soil.cpool_fast+=decomp_litter*FASTFRAC;

	// ... and slow SOM pool
	soil.cpool_slow+=decomp_litter*(1.0-FASTFRAC);

	// Increment C flux to atmosphere by SOM decomposition
	cflux+=soil.cpool_fast*(1.0-fr_soilfast)+soil.cpool_slow*(1.0-fr_soilslow);

	// Reduce SOM pools
	soil.cpool_fast*=fr_soilfast;
	soil.cpool_slow*=fr_soilslow;

	// Updated soil fluxes
	patch.fluxes.report_flux(Fluxes::SOILC, cflux);

	// Solve SOM pool sizes at end of year given by soil.solvesom_end

	if (date.year==soil.soiltype.solvesom_end && date.islastmonth && date.islastday)
		equilsom_lpj(soil);

}

/////////////////////////////////////////////////
// CENTURY SOM DYNAMICS

/// Data type representing a selection of SOM pools
/** A selection of SOM pools is represented by a bitset,
 *  the selected pools have their corresponding bits switched on.
 */
typedef std::bitset<NSOMPOOL> SomPoolSelection;


/// Reduce decay rates to keep the daily nitrogen balance in the soil
/** Only a selected subset of the SOM pools (as specified by the caller),
 *  are considered for reducion of decay rates.
 *  Usually the first time is enough (decay rate reduction of litter).
 */
void reduce_decay_rates(double decay_reduction[NSOMPOOL], double net_min_pool[NSOMPOOL], const SomPoolSelection& selected, double neg_nmass_avail) {

	int neg_min_pool[NSOMPOOL] = {0};	// Keeping track on which pools that are negative

	// Add up immobilization for considered pools
	double tot_neg_min = 0.0;
	for (int p = 0; p < NSOMPOOL; p++) {
		if (selected[p] && net_min_pool[p] < 0.0) {
			tot_neg_min += net_min_pool[p];
			neg_min_pool[p] = 1;
		}
	}

	// Calculate decay reduction
	double decay_red = 0.0;

	if (tot_neg_min < neg_nmass_avail) {
		// Enough to reduce decay rates for these pools to achieve a
		// net positive mineralization
		decay_red = 1.0 - (tot_neg_min - neg_nmass_avail) / tot_neg_min;
	}
	else {
		// Need to stop these pools from decaying to be able to get
		// a net positive mineralization
		decay_red = 1.0;
	}

	// Reduce decay rate for considered pools
	for (int p = 0; p < NSOMPOOL; p++) {
		if (selected[p]) {
			decay_reduction[p] = decay_red * neg_min_pool[p];
		}
	}
}

/// Set N:C ratios for SOM pools
/** Set N:C ratios for slow, passive, humus and soil microbial pools
 *  based on mineral nitrogen pool or litter nitrogen fraction (Parton et al 1993, Fig 4)
 */
void setntoc(Soil& soil, double fac, pooltype pool, double cton_max, double cton_min,
	double fmin, double fmax) {

	if (fac <= fmin)
		soil.sompool[pool].ntoc = 1.0 / cton_max;
	else if (fac >= fmax)
		soil.sompool[pool].ntoc = 1.0 / cton_min;
	else {
		soil.sompool[pool].ntoc = 1.0 / (cton_min + (cton_max - cton_min) *
			(fmax - fac) / (fmax - fmin));
	}
}

/// Calculates CENTURY instantaneous decay rates
/** Calculates CENTURY instantaneous decay rates given soil temperature,
 *  water content of upper soil layer
 */
void decayrates(Soil& soil, double temp_soil, double wcont_soil, bool tillage) {

	// Maximum exponential decay constants for each SOM pool (daily basis)
	// (Parton et al 2010, Figure 2)
	// plus Kirschbaum et al 2001 coarse woody debris decay
	const double K_MAX[] = {9.5e-3, 1.9e-2, 4.2e-2, 4.8e-4, 2.7e-2, 3.8e-2, 1.1e-2, 2.2e-3, 7.0e-2, 1.7e-3, 1.9e-6};
	// pools SURFSTRUCT,SOILSTRUCT,SOILMICRO,SURFHUMUS,SURFMICRO,SURFMETA,SURFFWD,SURFCWD,SOILMETA,SLOWSOM,PASSIVESOM

	// Modifier for effect of soil texture
	// Eqn 5, Parton et al 1993:

	const double texture_mod = 1.0 - 0.75 * (soil.soiltype.clay_frac + soil.soiltype.silt_frac);

	// Calculate decomposition temperature modifier (in range 0-1)
	// [A(T_soil), Eqn A9, Comins & McMurtrie 1993; ET, Friend et al 1997; abiotic
	// effect of soil temperature, Parton et al 1993, Fig 2)

	double temp_mod = 0.0;

	if (temp_soil > 0.0) {
		temp_mod = max(0.0,
			0.0326 + 0.00351 * pow(temp_soil, 1.652) - pow(temp_soil / 41.748, 7.19));
	}

	// Calculate decomposition moisture modifier (in range 0-1)
	// Friend et al 1997, Eqn 53
	// (Parton et al 1993, Fig 2)

	// Water Filled Pore Spaces (wfps)
	// water holding capacity at wilting point (wp) and saturation capacity (wsats)
	// is calculated with the help of Cosby et al 1984;
	const double wfps = (wcont_soil * soil.soiltype.awc[0] + soil.soiltype.wp[0]) * 100.0 / soil.soiltype.wsats[0];

	double moist_mod;

	if (wfps < 60.0)
		moist_mod = exp((wfps - 60.0) * (wfps - 60.0) / -800.0);
	else
		moist_mod = 0.000371 * wfps * wfps - 0.0748 * wfps + 4.13;

	for (int p = 0; p < NSOMPOOL-1; p++) {

		// Calculate decay constant
		// (dC_I/dt / C_I; Parton et al 1993, Eqns 2-4)

		double k = K_MAX[p] * temp_mod * moist_mod;

		// Include effect of recalcitrance effect of lignin
		// Parton et al 1993 Eqn 2
		// Kirschbaum et al 2001 changed the exponential term
		// from 3 to 5.

		if (p == SURFSTRUCT || p == SOILSTRUCT || p == SURFFWD || p == SURFCWD) {
			k *= exp(-5.0 * soil.sompool[p].ligcfrac);
		}
		else if (p == SOILMICRO) {
			k *= texture_mod;
		}

		// Increased HR for crops (tillage)
		if (tillage && (p == SURFMICRO || p == SURFHUMUS || p == SOILMICRO || p == SLOWSOM)) {
			k *= TILLAGE_FACTOR;
		}

		// Calculate fraction of carbon pool remaining after today's decomposition
		soil.sompool[p].fracremain = exp(-k);

		if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr) {
			soil.sompool[p].mfracremain_mean[date.month] += soil.sompool[p].fracremain / date.ndaymonth[date.month];
		}
	}
}

/// Transfers specified fraction (frac) of today's decomposition
/** Transfers specified fraction (frac) of today's decomposition in donor pool type
 *  to receiver pool, transferring fraction respfrac of this to the accumulated CO2
 *  flux respsum (representing total microbial respiration today)
 */
void transferdecomp(Soil& soil, pooltype donor, pooltype receiver,
	double frac, double respfrac, double& respsum, double& nmin_actual,
	double& nimmob, double& net_min) {

	// decrement in donor carbon pool and nitrogen pools
	double cdec = soil.sompool[donor].cdec * frac;
	double ndec = soil.sompool[donor].ndec * frac;

	// associated nitrogen increment in receiver pool (Friend et al 1997, Eqn 49)
	double ninc = cdec * (1.0 - respfrac) * soil.sompool[receiver].ntoc;

	// if increase in receiver nitrogen greater than decrease in donor nitrogen,
	// balance must be immobilisation from mineral nitrogen pool
	// otherwise balance is nitrogen mineralisation
	if (ninc > ndec) {
		nimmob += ninc - ndec;
		net_min += ndec - ninc;
	}
	else {
		nmin_actual += ndec - ninc;
		net_min += ndec - ninc;
	}

	// "Transfer" carbon and nitrogen to receiver
	soil.sompool[receiver].delta_cmass += cdec * (1.0 - respfrac);
	soil.sompool[receiver].delta_nmass += ninc;

	// Transfer microbial respiration
	respsum += cdec * respfrac;
}

/// Fluxes between the CENTURY pools, and CO2 release to the atmosphere
/** Daily or monthly fluxes between the ten CENTURY pools, and CO2 release to the atmosphere
 *  Parton et al 1993, Fig 1; Comins & McMurtrie 1993, Appendix A
 *
 *  \param ifequilsom Whether the function is called during calculation om SOM pool equilibrium,
 *                    \see equilsom(). During this stage, somfluxes shouldn't calculate decayrates
 *                    itself, and shouldn't produce output like fluxes etc.
 */
void somfluxes(Patch& patch, bool ifequilsom, bool tillage) {

	double respsum ;
	double leachsum_cmass, leachsum_nmass;
	double nmin_actual;	// actual (not net) nitrogen mineralisation
	double nimmob;		// nitrogen immobilisation

	const double EPS = 1.0e-16;

	Soil& soil = patch.soil;

	if (date.day == 0) {
		soil.anmin = 0.0;
		soil.animmob = 0.0;
	}

	// Warning if soil available nitrogen is negative (if happens once or so no problem, but if it propagates through time then it is)
	if (ifnlim) {
		assert(soil.nmass_avail > -EPS);
	}

	// Set N:C ratios for humus, soil microbial, passive and slow pool based on estimated mineral nitrogen pool
	// (Parton et al 1993, Fig 4)

	// ForCent (Parton 2010) values
	setntoc(soil, soil.nmass_avail, SLOWSOM, 30.0, 15.0, 0.0, NMASS_SAT);

	setntoc(soil, soil.nmass_avail, SOILMICRO, 15.0, 6.0, 0.0, NMASS_SAT);

	setntoc(soil, soil.nmass_avail, SURFHUMUS, 30.0, 15.0, 0.0, NMASS_SAT);

	if (!ifequilsom) {

		// Calculate potential fraction remaining following decay today for all pools
		// (assumes no nitrogen limitation)
		decayrates(soil, soil.temp, soil.wcont[0], tillage);

	}

	// Calculate decomposition in all pools assuming these decay rates

	// Save delta carbon and nitrogen mass

	bool net_mineralization = false;
	int times = 0;
	double decay_reduction[NSOMPOOL] = {0.0};
	double init_negative_nmass, init_ntoc_reduction;
	double ntoc_reduction = 0.8;

	// If necessary, the decay rates in the pools will be reduced in groups, one group
	// is reduced after each iteration in the loop below. The groups are defined by
	// how the pools feed into each other.
	SomPoolSelection reduction_groups[4];
	reduction_groups[0].set(SURFSTRUCT).set(SURFMETA).set(SURFFWD).set(SURFCWD).set(SOILSTRUCT).set(SOILMETA);
	reduction_groups[1].set(SURFMICRO);
	reduction_groups[2].set(SURFHUMUS);
	reduction_groups[3].set(SOILMICRO).set(SLOWSOM).set(PASSIVESOM);

	// If mineralization together with soil available nitrogen is negative then decay rates are decreased
	// The SOM system have five try to get a positive result, after that all pools decay rate has been
	// affected by nitrogen limitation
	while(!net_mineralization && times < 5) {

		respsum = 0.0;
		nmin_actual = 0.0;
		nimmob = 0.0;
		leachsum_cmass = 0.0;
		leachsum_nmass = 0.0;

		// Calculate decomposition in all pools assuming these decay rates
		for (int p = 0; p < NSOMPOOL; p++) {
			soil.sompool[p].cdec = soil.sompool[p].cmass * (1.0 - soil.sompool[p].fracremain) * (1.0 - decay_reduction[p]);
			soil.sompool[p].ndec = soil.sompool[p].nmass * (1.0 - soil.sompool[p].fracremain) * (1.0 - decay_reduction[p]);

			soil.sompool[p].delta_cmass = 0.0;
			soil.sompool[p].delta_nmass = 0.0;
			soil.sompool[p].delta_cmass -= soil.sompool[p].cdec;
			soil.sompool[p].delta_nmass -= soil.sompool[p].ndec;
		}

		double net_min[NSOMPOOL] = {0};

		// Partition potential decomposition among receiver pools

		// Donor pool SURFACE STRUCTURAL

		transferdecomp(soil, SURFSTRUCT, SURFMICRO, 1.0 - soil.sompool[SURFSTRUCT].ligcfrac,
			0.6, respsum, nmin_actual, nimmob, net_min[SURFSTRUCT]);

		transferdecomp(soil, SURFSTRUCT, SURFHUMUS, soil.sompool[SURFSTRUCT].ligcfrac, 0.3,
			respsum, nmin_actual, nimmob, net_min[SURFSTRUCT]);

		// Donor pool SURFACE METABOLIC

		transferdecomp(soil, SURFMETA, SURFMICRO, 1.0, 0.6, respsum, nmin_actual, nimmob, net_min[SURFMETA]);

		// Donor pool SOIL STRUCTURAL

		transferdecomp(soil, SOILSTRUCT, SOILMICRO, 1.0 - soil.sompool[SOILSTRUCT].ligcfrac,
			0.55, respsum, nmin_actual, nimmob, net_min[SOILSTRUCT]);

		transferdecomp(soil, SOILSTRUCT, SLOWSOM, soil.sompool[SOILSTRUCT].ligcfrac, 0.3,
			respsum, nmin_actual, nimmob, net_min[SOILSTRUCT]);

		// Donor pool SOIL METABOLIC

		transferdecomp(soil, SOILMETA, SOILMICRO, 1.0, 0.55, respsum, nmin_actual, nimmob, net_min[SOILMETA]);

		// Donor pool SURFACE FINE WOODY DEBRIS

		transferdecomp(soil, SURFFWD, SURFMICRO, 1.0 - soil.sompool[SURFFWD].ligcfrac,
			0.76, respsum, nmin_actual, nimmob, net_min[SURFFWD]);

		transferdecomp(soil, SURFFWD, SURFHUMUS, soil.sompool[SURFFWD].ligcfrac, 0.4,
			respsum, nmin_actual, nimmob, net_min[SURFFWD]);

		// Donor pool SURFACE COARSE WOODY DEBRIS

		transferdecomp(soil, SURFCWD, SURFMICRO, 1.0 - soil.sompool[SURFCWD].ligcfrac,
			0.9, respsum, nmin_actual, nimmob, net_min[SURFCWD]);

		transferdecomp(soil, SURFCWD, SURFHUMUS, soil.sompool[SURFCWD].ligcfrac, 0.5,
			respsum, nmin_actual, nimmob, net_min[SURFCWD]);

		// Donor pool SURFACE MICROBE

		transferdecomp(soil, SURFMICRO, SURFHUMUS, 1.0, 0.6, respsum, nmin_actual, nimmob, net_min[SURFMICRO]);

		// Donor pool SURFACE HUMUS

		transferdecomp(soil, SURFHUMUS, SLOWSOM, 1.0, 0.6, respsum, nmin_actual, nimmob, net_min[SURFHUMUS]);

		// Donor pool SLOW SOM

		// First work out partitioning coefficients (Fig 1, Parton et al 1993)

		double csp = max(0.0, 0.003 - 0.009 * soil.soiltype.clay_frac);
		double respfrac = 0.55;
		double csa = 1.0 - csp - respfrac;

		transferdecomp(soil, SLOWSOM, SOILMICRO, csa, 0.0, respsum, nmin_actual, nimmob, net_min[SLOWSOM]);

		transferdecomp(soil, SLOWSOM, PASSIVESOM, csp, 0.0, respsum, nmin_actual, nimmob, net_min[SLOWSOM]);

		// Account for respiration flux
		// Nitrogen associated with this respiration is mineralised (Parton et al 1993, p 791)
		respsum += respfrac * soil.sompool[SLOWSOM].cdec;

		if (!negligible(soil.sompool[SLOWSOM].cmass))
			nmin_actual += respfrac * soil.sompool[SLOWSOM].cdec * soil.sompool[SLOWSOM].nmass / soil.sompool[SLOWSOM].cmass;

		// Donor pool SOIL MICROBE

		// Fraction lost to microbial respiration (F_t, Parton et al 1993 Eqn 7)
		respfrac = max(0.0, 0.85 - 0.68 * (soil.soiltype.clay_frac + soil.soiltype.silt_frac));

		// Fraction entering passive SOM pool (Parton et al 1993, Eqn 9)
		double cap = 0.003 + 0.032 * soil.soiltype.clay_frac;

		transferdecomp(soil, SOILMICRO, PASSIVESOM, cap, 0.0, respsum, nmin_actual, nimmob, net_min[SOILMICRO]);

		// Fraction entering slow SOM pool
		csp = 1.0 - respfrac - soil.orgleachfrac - cap;

		transferdecomp(soil, SOILMICRO, SLOWSOM, csp, 0.0, respsum, nmin_actual, nimmob, net_min[SOILMICRO]);

		// Account for respiration flux
		// nitrogen associated with this respiration is mineralised (Parton et al 1993, p 791)
		respsum += respfrac * soil.sompool[SOILMICRO].cdec;

		// Account for organic carbon leaching loss
		leachsum_cmass = soil.orgleachfrac * soil.sompool[SOILMICRO].cdec;

		if (!negligible(soil.sompool[SOILMICRO].cmass)) {
			nmin_actual += respfrac * soil.sompool[SOILMICRO].cdec * soil.sompool[SOILMICRO].nmass / soil.sompool[SOILMICRO].cmass;

			// Account for organic nitrogen leaching loss
			leachsum_nmass = soil.orgleachfrac * soil.sompool[SOILMICRO].cdec * soil.sompool[SOILMICRO].nmass / soil.sompool[SOILMICRO].cmass;
		}

		// Donor pool PASSIVE SOM

		transferdecomp(soil, PASSIVESOM, SOILMICRO, 1.0, 0.55, respsum, nmin_actual, nimmob, net_min[PASSIVESOM]);

		// Total net mineralization
		double tot_net_min = nmin_actual - nimmob;

		// Estimate daily soil mineral nitrogen pool after decomposition
		// (negative value = immobilisation)
		if (tot_net_min + soil.nmass_avail + EPS >= 0.0) {

			net_mineralization = true;
		}
		else if (!ifnlim) {

			// Not minding immobilisation higher than nmass_avail during free nitrogen years
			if (date.year > freenyears) {

				// Immobilization larger than soil available nitrogen -> reduce targeted N concentration in SOM pool with flexible N:C ratios
				if (times == 0) {
					// initial reduction
					init_negative_nmass = tot_net_min + soil.nmass_avail;
					init_ntoc_reduction = ntoc_reduction;
				}
				else {
					// trying to match needed N:C reduction
					ntoc_reduction = min(init_ntoc_reduction, pow(init_ntoc_reduction, 1.0 / (1.0 - (tot_net_min + soil.nmass_avail) / init_negative_nmass) + 1.0));
				}

				soil.sompool[SLOWSOM].ntoc *= ntoc_reduction;
				soil.sompool[SOILMICRO].ntoc *= ntoc_reduction;
				soil.sompool[SURFHUMUS].ntoc *= ntoc_reduction;

				net_mineralization = false;
			}
			else {
				net_mineralization = true;
			}
		}
		else {

			// Immobilization larger than soil available nitrogen -> reduce decay rates
			if (times < 4) {
				reduce_decay_rates(decay_reduction, net_min, reduction_groups[times], tot_net_min + soil.nmass_avail);
			}
			net_mineralization = false;
		}
		times++;
	}

	// Update pool sizes

	for (int p = 0; p < NSOMPOOL-1; p++) {
		soil.sompool[p].cmass += soil.sompool[p].delta_cmass;
		soil.sompool[p].nmass += soil.sompool[p].delta_nmass;
	}

	if (!ifequilsom) {

		// Updated soil fluxes
		patch.fluxes.report_flux(Fluxes::SOILC, respsum);

		// Transfer organic leaching to pool

		soil.sompool[LEACHED].cmass += leachsum_cmass;
		soil.sompool[LEACHED].nmass += leachsum_nmass;

		// Sum annual organic nitrogen leaching

		soil.aorgNleach += leachsum_nmass;

		// Sum annual organic carbon leaching

		soil.aorgCleach += leachsum_cmass;

		// Sum annuals
		soil.anmin += nmin_actual;
		soil.animmob += nimmob;
	}

	// Adding mineral nitrogen to soil available pool
	soil.nmass_avail += nmin_actual - nimmob;

	// Estimate of N flux from soil (simple CLM-CN approach)
	double nflux = nmin_actual - nimmob > 0.0 ? (nmin_actual - nimmob) * 0.01 : 0.0;
	soil.nmass_avail -= nflux;

	if (!ifequilsom) {
		patch.fluxes.report_flux(Fluxes::N_SOIL, nflux);
	}
	// If no nitrogen limitation or during free nitrogen years set soil
	// available nitrogen to its saturation level.
	if (date.year <= freenyears)
		soil.nmass_avail = NMASS_SAT;
}

/// Litter lignin to N ratio (for leaf and root litter)
/** Specific lignin fractions for leaf and root
 *  are specified in transfer_litter()
 */
double lignin_to_n_ratio(double cmass_litter, double nmass_litter, double LIGCFRAC, double cton_avr) {

	if (!negligible(nmass_litter) && ifnlim) {
		return max(0.0, LIGCFRAC * cmass_litter / nmass_litter);
	}
	else {
		return max(0.0, LIGCFRAC * cton_avr / (1.0 - nrelocfrac));
	}
}

/// Metabolic litter fraction (for leaf and root litter)
/** Fm, Parton et al 1993, Eqn 1:
 *  NB: incorrect/out-of-date intercept and slope given in Eqn 1; values used in
 *  code of CENTURY 4.0 used instead (also correct in Parton et al. 1993, figure 1)
 *
 * \param lton  Litter lignin:N ratio
 */
double metabolic_litter_fraction(double lton) {
	return max(0.0, 0.85 - lton * 0.013);
}

/// Transfers litter from growth, mortality and fire
/** Called monthly to transfer last year's litter from vegetation
 *  (turnover, mortality and fire) to soil litter pools.
 *  Alternatively, with daily allocation and harvest/turnover,
 *  the litter produced a certain day.
 */
void transfer_litter(Patch& patch) {

	Soil& soil = patch.soil;

	double lat = patch.get_climate().lat;

	double EPS = -1.0e-16;

	// Leaf, root and wood litter lignin fractions
	// Leaf and root fractions: Comins & McMurtrie 1993; Friend et al 1997
	// Not sure of wood fraction
	const double LIGCFRAC_LEAF = 0.2;
	const double LIGCFRAC_ROOT = 0.16;
	const double LIGCFRAC_WOOD = 0.3;

	double ligcmass_new, ligcmass_old;

	// Fire
	double litterme[NSOMPOOL];
	litterme[SURFSTRUCT]   = soil.sompool[SURFSTRUCT].cmass * soil.sompool[SURFSTRUCT].litterme;
	litterme[SURFMETA]     = soil.sompool[SURFMETA].cmass   * soil.sompool[SURFMETA].litterme;
	litterme[SURFFWD]      = soil.sompool[SURFFWD].cmass    * soil.sompool[SURFFWD].litterme;
	litterme[SURFCWD]      = soil.sompool[SURFCWD].cmass    * soil.sompool[SURFCWD].litterme;

	double fireresist[NSOMPOOL];
	fireresist[SURFSTRUCT] = soil.sompool[SURFSTRUCT].cmass * soil.sompool[SURFSTRUCT].fireresist;
	fireresist[SURFMETA]   = soil.sompool[SURFMETA].cmass   * soil.sompool[SURFMETA].fireresist;
	fireresist[SURFFWD]    = soil.sompool[SURFFWD].cmass    * soil.sompool[SURFFWD].fireresist;
	fireresist[SURFCWD]    = soil.sompool[SURFCWD].cmass    * soil.sompool[SURFCWD].fireresist;

	double leaf_littter = 0.0;
	double root_littter = 0.0;
	double wood_littter = 0.0;

	bool drop_leaf_root_litter = (lat >= 0.0 && date.month == 0) || (lat < 0.0 && date.month == 6) || patch.is_litter_day;

	patch.pft.firstobj();
	while (patch.pft.isobj) {
		Patchpft& pft=patch.pft.getobj();

		// For stands with yearly growth, drop leaf and root litter on first day of the year for northern hemisphere
		// and first day of the second half of the year for southern hemisphere.
		// For stands with daily growth, harvest and/or turnover, do this when patch.is_litter_day is true.
		if (drop_leaf_root_litter) {

			// LEAF

			leaf_littter += pft.litter_leaf;

			// Calculate inputs to surface structural and metabolic litter

			// Leaf litter lignin:N ratio
			double leaf_lton = lignin_to_n_ratio(pft.litter_leaf, pft.nmass_litter_leaf, LIGCFRAC_LEAF, pft.pft.cton_leaf_avr);

			// Metabolic litter fraction for leaf litter
			double fm = metabolic_litter_fraction(leaf_lton);

			ligcmass_old = soil.sompool[SURFSTRUCT].cmass * soil.sompool[SURFSTRUCT].ligcfrac;

			// Add to pools
			soil.sompool[SURFSTRUCT].cmass += pft.litter_leaf       * (1.0 - fm);
			soil.sompool[SURFSTRUCT].nmass += pft.nmass_litter_leaf * (1.0 - fm);
			soil.sompool[SURFMETA].cmass   += pft.litter_leaf       * fm;
			soil.sompool[SURFMETA].nmass   += pft.nmass_litter_leaf * fm;

			// Save litter input for equilsom()
			if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr) {
				soil.litterSolveSOM.add_litter(pft.litter_leaf * (1.0 - fm), pft.nmass_litter_leaf * (1.0 - fm), SURFSTRUCT);
				soil.litterSolveSOM.add_litter(pft.litter_leaf * fm, pft.nmass_litter_leaf * fm, SURFMETA);
			}

			// Fire
			litterme[SURFSTRUCT]   += pft.litter_leaf * (1.0 - fm) * pft.pft.litterme;
			fireresist[SURFSTRUCT] += pft.litter_leaf * (1.0 - fm) * pft.pft.fireresist;

			litterme[SURFMETA]     += pft.litter_leaf * fm * pft.pft.litterme;
			fireresist[SURFMETA]   += pft.litter_leaf * fm * pft.pft.fireresist;

			// NB: reproduction litter cannot contain nitrogen!!

			ligcmass_new = pft.litter_leaf * (1.0 - fm) * LIGCFRAC_LEAF;

			if (negligible(soil.sompool[SURFSTRUCT].cmass)) {
				soil.sompool[SURFSTRUCT].ligcfrac = 0.0;
			}
			else {
				soil.sompool[SURFSTRUCT].ligcfrac = (ligcmass_new + ligcmass_old)/
					soil.sompool[SURFSTRUCT].cmass;
			}

			// ROOT

			root_littter += pft.litter_root;

			// Calculate inputs to soil structural and metabolic litter

			// Root litter lignin:N ratio
			double root_lton = lignin_to_n_ratio(pft.litter_root, pft.nmass_litter_root, LIGCFRAC_ROOT, pft.pft.cton_root_avr);

			// Metabolic litter fraction for root litter
			fm = metabolic_litter_fraction(root_lton);

			ligcmass_new = pft.litter_root * (1.0 - fm) * LIGCFRAC_ROOT;
			ligcmass_old = soil.sompool[SOILSTRUCT].cmass * soil.sompool[SOILSTRUCT].ligcfrac;

			// Add to pools and update lignin fraction in structural pool
			soil.sompool[SOILSTRUCT].cmass += pft.litter_root       * (1.0 - fm);
			soil.sompool[SOILSTRUCT].nmass += pft.nmass_litter_root * (1.0 - fm);
			soil.sompool[SOILMETA].cmass   += pft.litter_root       * fm;
			soil.sompool[SOILMETA].nmass   += pft.nmass_litter_root * fm;

			// Save litter input for equilsom()
			if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr) {
				soil.litterSolveSOM.add_litter(pft.litter_root * (1.0 - fm), pft.nmass_litter_root * (1.0 - fm), SOILSTRUCT);
				soil.litterSolveSOM.add_litter(pft.litter_root * fm, pft.nmass_litter_root * fm, SOILMETA);
			}

			if (negligible(soil.sompool[SOILSTRUCT].cmass)) {
				soil.sompool[SOILSTRUCT].ligcfrac = 0.0;
			}
			else {
				soil.sompool[SOILSTRUCT].ligcfrac = (ligcmass_new + ligcmass_old) /
					soil.sompool[SOILSTRUCT].cmass;
			}

			// Remove association with vegetation
			pft.litter_leaf = 0.0;
			pft.litter_root = 0.0;
			pft.nmass_litter_leaf = 0.0;
			pft.nmass_litter_root = 0.0;
		}

		// WOOD

		if (pft.pft.lifeform == TREE && date.dayofmonth == 0) {

			// Woody debris enters two woody litter pools as described in
			// Kirschbaum and Paul (2002).

			if (date.month == 0) {
				pft.litter_sap_year = pft.litter_sap;
				pft.nmass_litter_sap_year = pft.nmass_litter_sap;
			}

			// Monthly fraction of last years litter
			double litter_sap = pft.litter_sap_year / 12.0;
			double nmass_litter_sap = pft.nmass_litter_sap_year / 12.0;

			pft.litter_sap -= pft.litter_sap_year / 12.0;
			pft.nmass_litter_sap -= pft.nmass_litter_sap_year / 12.0;
			soil.sompool[SURFFWD].nmass += nmass_litter_sap;

			if (!negligible(litter_sap)) {

				// Fine woody debris

				wood_littter += litter_sap;

				assert(litter_sap >= EPS);
				ligcmass_new = litter_sap * LIGCFRAC_WOOD;
				ligcmass_old = soil.sompool[SURFFWD].cmass * soil.sompool[SURFFWD].ligcfrac;

				// Add to structural pool and update lignin fraction in pool
				soil.sompool[SURFFWD].cmass += litter_sap;

				// Save litter input for equilsom()
				if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr) {
					soil.litterSolveSOM.add_litter(litter_sap, nmass_litter_sap, SURFFWD);
				}

				if (negligible(soil.sompool[SURFFWD].cmass)) {
					soil.sompool[SURFFWD].ligcfrac = 0.0;
				}
				else {
					double ligcfrac = (ligcmass_new + ligcmass_old) /
						soil.sompool[SURFFWD].cmass;
					soil.sompool[SURFFWD].ligcfrac = ligcfrac;
				}

				// Fire
				litterme[SURFFWD]   += litter_sap * pft.pft.litterme;
				fireresist[SURFFWD] += litter_sap * pft.pft.fireresist;
			}

			if (date.month == 0) {
				pft.litter_heart_year = pft.litter_heart;
				pft.nmass_litter_heart_year = pft.nmass_litter_heart;
			}

			// Monthly fraction of last years litter
			double litter_heart = pft.litter_heart_year / 12.0;
			double nmass_litter_heart = pft.nmass_litter_heart_year / 12.0;

			pft.litter_heart -= pft.litter_heart_year / 12.0;
			pft.nmass_litter_heart -= pft.nmass_litter_heart_year / 12.0;
			soil.sompool[SURFCWD].nmass += nmass_litter_heart;

			if (!negligible(litter_heart)) {

				// Coarse woody debris

				wood_littter += litter_heart;

				assert(litter_heart >= EPS);
				ligcmass_new = litter_heart * LIGCFRAC_WOOD;
				ligcmass_old = soil.sompool[SURFCWD].cmass * soil.sompool[SURFCWD].ligcfrac;

				// Add to structural pool and update lignin fraction in pool
				soil.sompool[SURFCWD].cmass += litter_heart;

				// Save litter input for equilsom()
				if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr) {
					soil.litterSolveSOM.add_litter(litter_heart, nmass_litter_heart, SURFCWD);
				}

				if (negligible(soil.sompool[SURFCWD].cmass)) {
					soil.sompool[SURFCWD].ligcfrac = 0.0;
				}
				else {
					double ligcfrac = (ligcmass_new + ligcmass_old) /
						soil.sompool[SURFCWD].cmass;
					soil.sompool[SURFCWD].ligcfrac = ligcfrac;
				}

				// Fire
				litterme[SURFCWD]   += litter_heart * pft.pft.litterme;
				fireresist[SURFCWD] += litter_heart * pft.pft.fireresist;
			}
		}

		// Remove association with vegetation
		if (date.islastmonth) {
			pft.litter_sap = 0.0;
			pft.litter_heart = 0.0;
			pft.nmass_litter_sap = 0.0;
			pft.nmass_litter_heart = 0.0;
		}

		patch.pft.nextobj();
	}

	// FIRE
	if (soil.sompool[SURFSTRUCT].cmass > 0.0) {
		soil.sompool[SURFSTRUCT].litterme   = litterme[SURFSTRUCT]   / soil.sompool[SURFSTRUCT].cmass;
		soil.sompool[SURFSTRUCT].fireresist = fireresist[SURFSTRUCT] / soil.sompool[SURFSTRUCT].cmass;
	}
	if (soil.sompool[SURFMETA].cmass > 0.0) {
		soil.sompool[SURFMETA].litterme   = litterme[SURFMETA]   / soil.sompool[SURFMETA].cmass;
		soil.sompool[SURFMETA].fireresist = fireresist[SURFMETA] / soil.sompool[SURFMETA].cmass;
	}
	if (soil.sompool[SURFFWD].cmass > 0.0) {
		soil.sompool[SURFFWD].litterme    = litterme[SURFFWD]   / soil.sompool[SURFFWD].cmass;
		soil.sompool[SURFFWD].fireresist  = fireresist[SURFFWD] / soil.sompool[SURFFWD].cmass;
	}
	if (soil.sompool[SURFCWD].cmass > 0.0) {
		soil.sompool[SURFCWD].litterme    = litterme[SURFCWD]   / soil.sompool[SURFCWD].cmass;
		soil.sompool[SURFCWD].fireresist  = fireresist[SURFCWD] / soil.sompool[SURFCWD].cmass;
	}

	// Calculate total litter carbon and nitrogen mass for set N:C ratio of surface microbial pool
	double litter_cmass = soil.sompool[SURFSTRUCT].cmass + soil.sompool[SURFMETA].cmass +
	                      soil.sompool[SURFFWD].cmass + soil.sompool[SURFCWD].cmass;
	double litter_nmass = soil.sompool[SURFSTRUCT].nmass + soil.sompool[SURFMETA].nmass +
	                      soil.sompool[SURFFWD].nmass + soil.sompool[SURFCWD].nmass;

	// Set N:C ratio of surface microbial pool based on N:C ratio of litter from all PFTs
	// Parton et al 1993 Fig 4. Dry mass litter == cmass litter * 2
	if (!negligible(litter_cmass)) {
		setntoc(soil, litter_nmass / (litter_cmass * 2.0), SURFMICRO, 20.0, 10.0, 0.0, NCONC_SAT);
	}

	// Add litter to solvesom array every month
	if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr) {
		if (date.dayofmonth == 0) {
			soil.solvesom.push_back(soil.litterSolveSOM);
			soil.litterSolveSOM.clear();
		}
	}
}


/// LEACHING
/** Leaching fractions for both organic and mineral leaching
 *  Should be called every day in both daily and monthly mode
 */
void leaching(Soil& soil) {

	double minleachfrac;

	if (!negligible(soil.dperc)) {

		// Leaching from available nitrogen mineral pool
		// in proportion to amount of water drainage
		minleachfrac = soil.dperc / (soil.dperc + soil.soiltype.awc[0] * soil.wcont[0] + soil.soiltype.awc[1] * soil.wcont[1]);

		// Leaching from decayed organic carbon/nitrogen
		// using Parton et al. eqn. 8; CENTURY 5 parameter update; from equation: C Leached=microbial_C*[OMLECH(1)+OMLECH(2)*sand_fraction]*[1.0f-(OMLECH(3)-water_leaching)/OMLECH(3)],
		// reorganised as: leachfrac = [water_leaching/OMLECH(3)]*[OMLECH(1)+OMLECH(2)*sand_fraction]
		// water_leaching was originally in cm/month
		double OMLECH_1 = 0.03;
		double OMLECH_2 = 0.12;
		double OMLECH_3 = 1.9;	// saturation point in leaching equation (cm H20/month)
		double cmpermonth_to_mmperday = 10.0 * 12.0 / 365.0;

		soil.orgleachfrac = min(1.0, soil.dperc / (OMLECH_3 * cmpermonth_to_mmperday)) * (OMLECH_1 + OMLECH_2 * soil.soiltype.sand_frac);
	}
	else {
		minleachfrac = 0.0;
		soil.orgleachfrac = 0.0;
	}

	// Leaching of soil mineral nitrogen
	// Allowed on days with residual nitrogen following vegetation uptake
	// in proportion to amount of water drainage
	if (soil.nmass_avail > 0.0) {
		double leaching = soil.nmass_avail * minleachfrac;
		soil.nmass_avail -= leaching;
		soil.aminleach += leaching;
		soil.sompool[LEACHED].nmass += leaching;
	}

	if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr) {
		soil.morgleach_mean[date.month] += soil.orgleachfrac / date.ndaymonth[date.month];
		soil.mminleach_mean[date.month] += minleachfrac      / date.ndaymonth[date.month];
	}
}


/// Nitrogen addition to the soil
/** Daily nitrogen addition to the soil
 *  from deposition and fixation
 */
void soilnadd(Patch& patch) {

	Soil& soil = patch.soil;

	double nflux = 0.01 * soil.ninput;
	soil.ninput -= nflux;

	patch.fluxes.report_flux(Fluxes::N_SOIL, nflux);

	// Nitrogen deposition and fertilization input to the soil (calculated in snow_ninput())
	soil.nmass_avail += soil.ninput;

	// Nitrogen fixation
	// If soil available nitrogen is above the value for minimum SOM C:N ratio, then
	// nitrogen fixation is reduced (nitrogen rich soils)
	if (soil.nmass_avail < NMASS_SAT) {

		const double daily_nfix = soil.anfix_calc / date.year_length();

		if (soil.nmass_avail + daily_nfix < NMASS_SAT) {
			soil.nmass_avail += daily_nfix;
			soil.anfix += daily_nfix;
		}
		else {
			soil.anfix += NMASS_SAT - soil.nmass_avail;
			soil.nmass_avail = NMASS_SAT;
		}
	}

	// Calculate nitrogen fixation (Cleveland et al. 1999)
	// by using five year average aaet
	if (date.islastmonth && date.islastday) {

		// Add this year's AET to aaet_5 which keeps track of the last 5 years
		patch.aaet_5.add(patch.aaet+patch.aevap+patch.aintercep);

		// Calculate estimated nitrogen fixation (aaet should be in cm/yr, eqn is in nitrogen/ha/yr)
		double cmtomm = 0.1;
		double hatom2 = 0.0001;
		soil.anfix_calc = max((nfix_a * patch.aaet_5.mean() * cmtomm + nfix_b) * hatom2, 0.0);

		if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr) {
			soil.anfix_mean += soil.anfix;
		}
	}
}

/// Vegetation nitrogen uptake
/** Daily vegetation uptake of mineral nitrogen
 *  Partitioned among individuals according to todays nitrogen demand
 *  and individuals root area
 */
void vegetation_n_uptake(Patch& patch) {

	// Daily nitrogen demand given by:
	//	 For individual:
	//     (1)  ndemand = leafndemand + rootndemand + sapndemand;
    //          where
	//          leafndemand is leaf demand based on vmax
	//			rootndemand is based on optimal leaf C:N ratio
	//          sapndemand is based on optimal leaf C:N ratio
	//
	// Actual nitrogen uptake for each day and individual given by:
	//     (2)  nuptake = ndemand * fnuptake
	//     where fnuptake is individual uptake capacity calculated in fnuptake in canexch.cpp

	double nuptake_day, orignmass;

	Vegetation& vegetation=patch.vegetation;
	Soil& soil = patch.soil;

	orignmass = soil.nmass_avail;

	// Loop through individuals

	vegetation.firstobj();
	while (vegetation.isobj) {
		Individual& indiv = vegetation.getobj();

		if (date.day == 0)
			indiv.anuptake = 0.0;

		nuptake_day           = indiv.ndemand * indiv.fnuptake;
		indiv.anuptake        += nuptake_day;
		indiv.nmass_leaf      += indiv.leaffndemand  * nuptake_day;
		indiv.nmass_root      += indiv.rootfndemand  * nuptake_day;
		indiv.nmass_sap       += indiv.sapfndemand   * nuptake_day;
		if (indiv.pft.phenology == CROPGREEN && ifnlim)
			indiv.cropindiv->nmass_agpool += indiv.storefndemand * nuptake_day;
		else
			indiv.nstore_longterm += indiv.storefndemand * nuptake_day;
		soil.nmass_avail      -= nuptake_day;

		if (!negligible(indiv.phen))
			indiv.cton_leaf_aavr += min(indiv.cton_leaf(),indiv.pft.cton_leaf_max);

		vegetation.nextobj();
	}

	if (date.year >= soil.solvesomcent_beginyr && date.year <= soil.solvesomcent_endyr && !negligible(orignmass)) {
		soil.fnuptake_mean[date.month] += (1.0 - soil.nmass_avail / orignmass) / date.ndaymonth[date.month];
	}
}

/// Add litter to equilsom()
void add_litter(Soil& soil, int year, int pool) {

	// If LUC have occurred in between solvesomcent_beginyr and solvesomcent_endyr then array might be too smalll
	if (year >= (int)soil.solvesom.size())
		year = year%(int)soil.solvesom.size();

	// Add to litter pools
	soil.sompool[pool].cmass += soil.solvesom[year].get_clitter(pool);
	soil.sompool[pool].nmass += soil.solvesom[year].get_nlitter(pool);
}

/// Iteratively solving differential flux equations for century SOM pools
/** Iteratively solving differential flux equations for century SOM pools
 *  assuming annual litter inputs, nitrogen uptake and leaching is close
 *  to long term equilibrium
 */
void equilsom(Soil& soil) {

	// Number of years to run SOM pools, value chosen to get cold climates to equilibrium
	const int EQUILSOM_YEARS = 40000;

	Patch& patch = soil.patch;
	const Climate& climate = soil.patch.get_climate();

	// Save nmass_avail status
	double save_nmass_avail = soil.nmass_avail;

	// Number of years with mean input data
	int nyear = soil.solvesomcent_endyr - soil.solvesomcent_beginyr + 1;

	for (int m = 0; m < 12; m++) {

		// Monthly average decay rates
		for (int p = 0; p < NSOMPOOL-1; p++) {
			soil.sompool[p].mfracremain_mean[m] = pow(soil.sompool[p].mfracremain_mean[m] / nyear, (double)date.ndaymonth[m]);
		}

		// Monthly average mineral nitrogen uptake
		soil.fnuptake_mean[m] /= nyear;

		// Monthly average organic carbon and nitrogen leaching
		soil.morgleach_mean[m] /= nyear;

		// Monthly average mineral nitrogen leaching
		soil.mminleach_mean[m] /= nyear;
	}

	// Annual average nitrogen fixation
	soil.anfix_mean /= nyear;

	// Spin SOM pools with saved litter input, nitrogen addition and fractions of
	// nitrogen uptake and leaching for EQUILSOM_YEARS years with monthly timesteps
	for (int yr = 0; yr < EQUILSOM_YEARS; yr++) {

		// Which year in saved data set
		int savedyear = yr%nyear;

		// Monthly time steps
		for (int m = 0; m < 12; m++) {

			// Transfer yearly mean litter on first day of year
			add_litter(soil, savedyear*12+m, SURFSTRUCT);
			add_litter(soil, savedyear*12+m, SURFMETA);
			add_litter(soil, savedyear*12+m, SOILSTRUCT);
			add_litter(soil, savedyear*12+m, SOILMETA);
			add_litter(soil, savedyear*12+m, SURFFWD);
			add_litter(soil, savedyear*12+m, SURFCWD);

			// Calculate total litter carbon and nitrogen mass for set N:C ratio of surface microbial pool
			double litter_cmass = soil.sompool[SURFSTRUCT].cmass + soil.sompool[SURFMETA].cmass +
				soil.sompool[SURFFWD].cmass + soil.sompool[SURFCWD].cmass;
			double litter_nmass = soil.sompool[SURFSTRUCT].nmass + soil.sompool[SURFMETA].nmass +
				soil.sompool[SURFFWD].nmass + soil.sompool[SURFCWD].nmass;

			// Set N:C ratio of surface microbial pool based on N:C ratio of litter from all PFTs
			if (!negligible(litter_cmass)) {
				setntoc(soil, litter_nmass / (litter_cmass * 2.0), SURFMICRO, 20.0, 10.0, 0.0, NCONC_SAT);
			}

			// Monthly nitrogen uptake
			soil.nmass_avail *= (1.0 - soil.fnuptake_mean[m]);

			// Monthly mineral nitrogen leaching
			soil.nmass_avail *= (1.0 - soil.mminleach_mean[m]);

			// Monthly nitrogen addition to the system
			soil.nmass_avail += (climate.andep + soil.anfix_mean) / 12.0;

			// Monthly decomposition and fluxes between SOM pools

			// Set this months decay rates
			for (int p = 0; p < NSOMPOOL-1; p++) {
				soil.sompool[p].fracremain = soil.sompool[p].mfracremain_mean[m];
			}

			// Set this months organic nitrogen leaching fraction
			soil.orgleachfrac = soil.morgleach_mean[m];

			// Monthly decomposition and fluxes between SOM pools
			// and nitrogen flux from soil
			somfluxes(patch, true, false);
		}
	}

	// Reset nmass_avail status
	soil.nmass_avail = save_nmass_avail;

	// Reset variables for next equilsom()
	for (int m = 0; m < 12; m++) {

		for (int p = 0; p < NSOMPOOL-1; p++) {
			soil.sompool[p].mfracremain_mean[m] = 0.0;
		}

		soil.fnuptake_mean[m]  = 0.0;
		soil.morgleach_mean[m] = 0.0;
		soil.mminleach_mean[m] = 0.0;
	}

	soil.anfix_mean = 0.0;
	soil.solvesom.clear();
	std::vector<LitterSolveSOM>().swap(soil.solvesom); // clear array memory
}

/// SOM CENTURY DYNAMICS
/** To be called each simulation day for each modelled patch, following update
 *  of soil temperature and soil water.
 *  Transfers litter, performes nitrogen uptake and addition, leaching and decomposition.
 */
void som_dynamics_century(Patch& patch, bool tillage) {

	// First day of every month or other harvest/turnover day
	if (date.dayofmonth == 0 || patch.is_litter_day) {
		// Transfer last year's litter to SOM pools
		// Leaf and fine root litter on first day of year (first day of july in SH)
		// Woody litter transfers a portion first day of every month
		// OR this year's litter on harvest/turnover day for stands with daily allocation
		transfer_litter(patch);
	}

	// Daily nitrogen uptake
	vegetation_n_uptake(patch);

	// Daily mineral and organic nitrogen leaching
	leaching(patch.soil);

	// Daily nitrogen addition to the soil
	soilnadd(patch);

	// Daily or monthly decomposition and fluxes between SOM pools
	somfluxes(patch, false, tillage);

	// Solve SOM pool sizes at end of year given by soil.solvesomcent_endyr
	if (date.year==patch.soil.solvesomcent_endyr && date.islastmonth && date.islastday)
		equilsom(patch.soil);
}

/// Choose between CENTURY or standard LPJ SOM dynamics
/**
*/
void som_dynamics(Patch& patch) {

	bool tillage = iftillage && patch.stand.landcover == CROPLAND;
	if (ifcentury) som_dynamics_century(patch, tillage);
	else som_dynamics_lpj(patch, tillage);
}

///////////////////////////////////////////////////////////////////////////////////////
// REFERENCES
//
// Chatskikh, D., Hansen, S., Olesen, J.E. & Petersen, B.M. 2009. A simplified modelling approach
//	 for quantifying tillage effects on soil carbon stocks. Eur.J.Soil.Sci., 60:924-934.
// CENTURY Soil Organic Matter Model Version 5; Century User's Guide and Reference.
//  http://www.nrel.colostate.edu/projects/century5/reference/index.htm; accessed Oct.7, 2016.
// Comins, H. N. & McMurtrie, R. E. 1993. Long-Term Response of Nutrient-Limited
//   Forests to CO2 Enrichment - Equilibrium Behavior of Plant-Soil Models.
//   Ecological Applications, 3, 666-681.
// Cosby, B. J., Hornberger, C. M., Clapp, R. B., & Ginn, T. R. 1984 A statistical exploration
//   of the relationships of soil moisture characteristic to the physical properties of soil.
//   Water Resources Research, 20: 682-690.
// Cleveland C C (1999) Global patterns of terrestrial biological nitrogen (N2) fixation
//   in natural ecosystems. GBC 13: 623-645
// Foley J A 1995 An equilibrium model of the terrestrial carbon budget
//   Tellus (1995), 47B, 310-319
// Friend, A. D., Stevens, A. K., Knox, R. G. & Cannell, M. G. R. 1997. A
//   process-based, terrestrial biosphere model of ecosystem dynamics
//   (Hybrid v3.0). Ecological Modelling, 95, 249-287.
// Kirschbaum, M. U. F. and K. I. Paul (2002). "Modelling C and N dynamics in forest soils
//   with a modified version of the CENTURY model." Soil Biology & Biochemistry 34(3): 341-354.
// Meentemeyer, V. (1978) Macroclimate and lignin control of litter decomposition
//   rates. Ecology 59: 465-472.
// Parton, W. J., Scurlock, J. M. O., Ojima, D. S., Gilmanov, T. G., Scholes, R. J., Schimel, D. S.,
//   Kirchner, T., Menaut, J. C., Seastedt, T., Moya, E. G., Kamnalrut, A. & Kinyamario, J. I. 1993.
//   Observations and Modeling of Biomass and Soil Organic-Matter Dynamics for the Grassland Biome
//   Worldwide. Global Biogeochemical Cycles, 7, 785-809.
// Parton, W. J., Hanson, P. J., Swanston, C., Torn, M., Trumbore, S. E., Riley, W. & Kelly, R. 2010.
//   ForCent model development and testing using the Enriched Background Isotope Study experiment.
//   Journal of Geophysical Research-Biogeosciences, 115.

