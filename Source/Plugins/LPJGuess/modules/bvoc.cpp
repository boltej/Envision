///////////////////////////////////////////////////////////////////////////////////////
/// \file bvoc.cpp
/// \brief The BVOC module
///
/// Calculation of VOC production and emission by vegetation.
///
/// \author Guy Schurgers (using Almut's previous attempts)
/// $Date: 2016-12-08 18:24:04 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

// WHAT SHOULD THIS FILE CONTAIN?
// Module source code files should contain, in this order:
//	 (1) a "#include" directive naming the framework header file. The framework
//	 header file should define all classes used as arguments to functions in
//	 the present module. It may also include declarations of global functions,
//	 constants and types, accessible throughout the model code;
//	 (2) other #includes, including header files for other modules accessed by
//	 the present one;
//	 (3) type definitions, constants and file scope global variables for use
//	 within the present module only;
//	 (4) declarations of functions defined in this file, if needed;
//	 (5) definitions of all functions. Functions that are to be accessible to
//	 other modules or to the calling framework should be declared in the module
//	 header file.
//
// PORTING MODULES BETWEEN FRAMEWORKS:
// Modules should be structured so as to be fully portable between models
// (frameworks). When porting between frameworks, the only change required
// should normally be in the "#include" directive referring to the framework
// header file.

#include "config.h"
#include "bvoc.h"
#include "canexch.h"

const double CO2 = 370;            // standard CO2 concentration
const double Tstand = 30;          // standard temperature, oC

void initbvoc(){

	// initialising the VOC calculations: calculating the fraction of electrones
	// available for isoprene production from the isoprene and monoterpene
	// emission capacities (at T = 30oC and Q = 1000 umol m-2 s-1) as given by e.g.
	// Guenther et al. (1997) for all PFTs

	const double Qstand = 1e-3;             // radiation, mol m-2 s-1
	const double Cfrac = 0.5;               // mass fraction of C in leaves
	const double frabs_Q = 0.35;    // fraction of light absorbed in the first
	                                // canopy layer (for standard measurements),
	                                // 25% to 35% (Almut)
	const double daylength = 12;

	PhotosynthesisResult phot;
 	pftlist.firstobj();
 	while (pftlist.isobj) {
 		Pft& pft = pftlist.getobj();

		double par = frabs_Q * Qstand * 3600 * daylength / alphaa(pft) / CQ;
				// par for the standard condition, J m-2 d-1
		photosynthesis(CO2, Tstand, par, daylength, 1.0, pft.lambda_max, pft, 1.0, false, phot, -1);

		double coeff = 1e-3 / (phot.je + phot.rd_g/24) / pft.sla / Cfrac;

		// electron fraction assigned to isoprene and monoterpenes for the
 		// standard case
		pft.eps_iso *= coeff;
		pft.eps_mon *= coeff;

		pftlist.nextobj();
	}
}

double daytime_temp(double temp, double daylength, double dtr) {

	// Convert daily temperature to daytime temperature (deg C) using the actual
	// amplitude the conversion assumes a perfect sine function for
	// temperature, with the maximum temperature reached at noon (derivation by
	// Colin Prentice, checked)

	double hdl = daylength * PI / 24;
	return temp + dtr / 2 * sin(hdl) / hdl;
}

void iso_mono(double co2, double temp, double daylength, const Pft& pft, double temprel,
				const PhotosynthesisResult& phot, Individual& indiv) {

	// Calculation of isoprene and monoterpene emissions coupled to
	// photosynthesis as described in Arneth et al. (2007) for isoprene and
	// Schurgers et al. (2009) for monoterpenes.

	// (selected) INPUT PARAMETERS
	// temp      = water-stressed leaf temperature for the day-light part of the
	//              period, same as temprel in diurnal mode (deg C)
	// daylength = duration of the period (h)
	// temprel   = water-stressed leaf temperature for the whole period (deg C)
	// phot      = non-water-stressed photosynthesis

	const double tcstor_s = 80;     // time constant for monoterpene storage under standard temp
	const double tcstor_max = 365;  // maximum time constant for monoterpene storage (d)
	const double tcstor_min = 2;    // minimum time constant for monoterpene storage (d)
	const double q10_mstor = 1.9;   // Q10 value for monoterpene storage
	const double f_tempmax = 2.3;   // maximum temperature scaling factor
	const double epsT = 0.1;        // temperature sensitivity

	double f_co2 = CO2/co2;                                     // CO2 scaling factor
	double f_temp = min(f_tempmax, exp(epsT*(temp-Tstand)));    // temp scaling factor

	double coeff = phot.je * daylength + phot.rd_g;
	// isoprene production, g C m-2 d-1
	indiv.iso = pft.eps_iso * f_co2 * f_temp * indiv.fvocseas * coeff;
	// monoterpene production, g C m-2 d-1
	// (only the production part is given here)
	indiv.mon = pft.eps_mon * f_co2 * f_temp * coeff;

	// release from monoterpene storage, g C m-2 d-1
	double dmonstor = tcstor_s / pow(q10_mstor, (temprel-Tstand)/10.);
	dmonstor = 1. / max(min(dmonstor, tcstor_max), tcstor_min) / date.subdaily;

	// convert from g C m-2 d-1 to mg C m-2 d-1
	indiv.iso *= 1e3 / date.subdaily;
	indiv.mon *= 1e3 / date.subdaily;
	double rmonstor = -indiv.monstor * dmonstor + pft.storfrac_mon * indiv.mon;
	indiv.monstor += rmonstor;
	indiv.mon -= rmonstor;
}

double leafT(double temp, double daylength, double ga, double rs_day, double aet,
             double lai_today, double fpar, double fpc, double fpc_today) {

	// Canopy temperature is calculated from the air temperature and the energy balance (longwave
	// radiation, shortwave radiation and sensible and latent heat loss).
	// Revised version compared to Arneth et al. (2007) and Schurgers et al. (2011).

	if(lai_today <= 1.e-2) {
		return temp;
	}

	const double lam = 2.45e6;      // latent heat loss of vapourisation (J g-1 at 20 deg C)
	const double sigma = 5.67e-8;   // Stefan-Boltzmann constant, W m-2 K-4
	const double emiss_leaf = .97;  // average emissivity for leaves, Campbell and Norman, (1998)
	const double rhoair = 1.204;    // air density, kg m-3
	const double cp = 1010;         // specific heat capacity of air, J kg-1 K-1

	// leaf temperature is calculated by balancing four fluxes:
	// 1. net SW radiation, computed from the incoming radiation
	//    S_net = -rs_day*fpar/(daylength*3600.)
	// 2. net LW radiation, computed as a first-order Taylor expansion of Stefan-Boltzman law,
	//    which makes it a linear function of the temperature difference deltaT
	//    L_net = 4*emiss_leaf*sigma*(T**3.)*deltaT*phen*lai
	// 3. latent heat, computed from actual evapotranspiration AET
	//    LH = aet*lam/(daylength*3600.)
	// 4. sensible heat, computed as a linear function of the temperat
	//    H = deltaT*rhoair*cp*ga*phen*lai
	//

	return temp+(rs_day*fpar-aet*lam)/(3600.*daylength)/
		(4.*emiss_leaf*sigma*pow(temp+K2degC,3.)*fpc_today+rhoair*cp*ga*lai_today);
}


void seasonality(Climate& climate, const Pft& pft, double& f_season) {

	// Calculating the seasonality for VOCs (isoprene and monoterpene) for PFTs
	// Revised version compared to Arneth et al. (2007).
	// Seasonality switch pft.seas_iso read in from .ins file

	const double rdr = .05;     // relative decay rate (d-1)
	const double tmin = 5;      // minimum temperature for end of growing season (oC)
	const double dmin = 11;     // minimum daylength for end of growing season (h)
	const double mulgdd = 2;    // required GDD sum for VOCs is assumed to be twice
	                            // as large as for phenology

	if (pft.seas_iso == 0) {
		f_season = 1;
	}
	else {
		double vocgdd5ramp = mulgdd * pft.phengdd5ramp;
				// GDD sum required for full expression of isoprene
				// synthase/isoprene production
		if (climate.agdd5 <= vocgdd5ramp) {
			f_season = vocgdd5ramp != 0 ? climate.agdd5/vocgdd5ramp : 1;
		}
		else if (climate.temp < tmin || climate.daylength < dmin) {
			f_season *= 1-rdr;
		}
		else {
			f_season = 1;
		}
	}
}

void bvoc(double temp, double hours, double rad, Climate& climate, Patch& patch,
		Individual& indiv, const Pft& pft, const PhotosynthesisResult& phot,
		double adtmm, const Day& day) {

	// Calculation of isoprene and monoterpene production in leaves as a function
	// of photosynthesis. Isoprene and monoterpenes are calculated from a
	// standardized fraction of the total photosynthesis, which is adjusted as
	// a function of temperature, CO2 concentration and (for isoprene)
	// seasonality.

	// Isoprene calculations following Arneth et al. (2007), monoterpene
	// calculations following Schurgers et al. (2009). Compared to the original
	// publications, adjustments were made in the calculation of leaf
	// temperatures (which account now for longwave radiation as well, and
	// perform a weighted averaging over the whole canopy), and in the calculation
	// of isoprene seasonality (which requires a GDD sum twice as large as
	// required for phenology, and decreases with a relative rate at the end of
	// the growing season).

	// Changes made to accommodate diurnal mode, include re-calculating seasonality
	// irrespective of the possibility of BVOC emissions, and switching to
	// photosynthesis pre-calculated with air temperature (instead of leaf
	// temperature previously).

	// (selected) INPUT PARAMETERS
	// temp      = temperature for this calculation period (deg C)
	// hours     = in diurnal mode should equal 24 (to convert to daily units),
	//             in daily/monthly mode should equal to "climate.daylength" parameter (h)
	// climate:
	// daylength = actual daylength of the day the calculation period belongs to (h)
	// dtr       = diurnal temperature range (not used in diurnal mode) (deg C)
	// phot      = non-water stressed photosynthesis
	// adtmm     = actual (water-stressed) photosynthesis production for the period (mm/m2/day)

	if (day.isstart) {
		// calculate seasonality for VOC emissions
		seasonality(climate, pft, indiv.fvocseas);
	}
	if (adtmm <= 0) {
		return;
	}


	double temp_leaf_daytime;
	double temp_leaf = leafT(temp, hours, pft.ga, rad, indiv.aet,
                                 indiv.lai_today(),indiv.fpar,indiv.fpc,indiv.fpc_today());

	if (date.diurnal()) {
			temp_leaf_daytime = temp_leaf;
	}
	else {
		// perform daily to daytime correction
		double temp_corrected = daytime_temp(climate.temp, climate.daylength, climate.dtr);

		// perform air temperature to leaf temperature correction
		temp_leaf_daytime = leafT(temp_corrected, climate.daylength, pft.ga, rad, indiv.aet,
		                          indiv.lai_today(),indiv.fpar,indiv.fpc, indiv.fpc_today());

	}

	// calculate isoprene and monoterpene emissions, g C m-2 d-1
	iso_mono(climate.co2, temp_leaf_daytime, hours, pft, temp_leaf, phot, indiv);

	indiv.report_flux(Fluxes::ISO, indiv.iso);
	indiv.report_flux(Fluxes::MON, indiv.mon);
}

// REFERENCES
//
// Arneth, A., Niinemets, Ü, Pressley, S., Bäck, J., Hari, P., Karl, T., Noe,
//	 S., Prentice, I.C., Serca, D., Hickler, T., Wolf, A., Smith, B., 2007.
//	 Process-based estimates of terrestrial ecosystem isoprene emissions:
//	 incorporating the effects of a direct CO2-isoprene interaction.
//	 Atmospheric Chemistry and Physics, 7, 31-53.
// Campbell, G.S., Norman, J.M., 1998. An introduction to environmental
//	 biophysics, 2nd edition. Springer, New York.
// Guenther, A., 1997. Seasonal and spatial variations in natural volatile
//	 organic compound emissions. Ecological Applications, 7, 34-45.
// Niinemets, Ü, Tenhunen, J.D., Harley, P.C., Steinbrecher, R., 1999. A model
//	 of isoprene emission based on energetic requirements for isoprene
//	 synthesisand leaf photosynthetic properties for Liquidambar and Quercus.
//	 Plant, Cell and Environment, 22, 1319-1335.
// Schurgers, G., Arneth, A., Holzinger, R., Goldstein, A., 2009. Process-
//	 based modelling of biogenic monoterpene emissions combining production
//	 and release from storage. Atmospheric Chemistry and Physics, 9, 3409-3423.
// Schurgers, G., Arneth, A., Hickler, T., 2011. Effect of climate-driven changes
//       in species composition on regional emission capacities of biogenic
//       compounds. Journal of Geophysical Research, 116, D22304.
