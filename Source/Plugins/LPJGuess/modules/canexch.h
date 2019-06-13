///////////////////////////////////////////////////////////////////////////////////////
/// \file canexch.h
/// \brief The canopy exchange module header file
///
/// Vegetation-atmosphere exchange of H2O and CO2 via
/// production, respiration and evapotranspiration.
///
/// \author Ben Smith
/// $Date: 2016-12-08 18:50:55 +0100 (Thu, 08 Dec 2016) $
///
///////////////////////////////////////////////////////////////////////////////////////

// WHAT SHOULD THIS FILE CONTAIN?
// Module header files need normally contain only declarations of functions defined in
// the module that are to be accessible to the calling framework or to other modules.

#ifndef LPJ_GUESS_CANEXCH_H
#define LPJ_GUESS_CANEXCH_H

#include "guess.h"

void interception(Patch& patch, Climate& climate);
void canopy_exchange(Patch& patch, Climate& climate);
void photosynthesis(double co2, double temp, double par, double daylength,
					double fpar, double lambda, const Pft& pft,
					double nactive, bool ifnlimvmax,
					PhotosynthesisResult& result, double vm);

/// Nitrogen- and landuse specific alpha a
double alphaa(const Pft& pft);

// Constants for photosynthesis calculations

/// conversion factor for solar radiation at 550 nm from J/m2 to mol_quanta/m2 (E=mol quanta); mol J-1
const double CQ = 4.6e-6;

/// intrinsic quantum efficiency of CO2 uptake, C3 plants
const double ALPHA_C3 = 0.08;

/// intrinsic quantum efficiency of CO2 uptake, C4 plants
const double ALPHA_C4 = 0.053;

/// O2 partial pressure (Pa)
const double PO2 = 2.09e4;

/// colimitation (shape) parameter
const double THETA = 0.7;

/// 'saturation' ratio of intercellular to ambient CO2 partial pressure for C4 plants
const double LAMBDA_SC4 = 0.4;

/// leaf respiration as fraction of maximum rubisco, C3 plants
const double BC3 = 0.015;

/// leaf respiration as fraction of maximum rubisco, C4 plants
const double BC4 = 0.02;

const double CMASS = 12.0;		// atomic mass of carbon
const double ALPHAA = 0.45;		// value chosen to give global carbon pool and flux values that
								// agree with published estimates.
								// scaling factor for PAR absorption from leaf to plant projective area level
								// alias "twigloss". Should normally be in the range 0-1

const double ALPHAA_NLIM = 0.6; // Same as ALPHAA above but chosen to give pools and flux values
								// that agree with published estimates when Nitrogen limitation is
								// switched on.

const double ALPHAA_CROP = 0.65;		// Value for crops without N limitation.
const double ALPHAA_CROP_NLIM = 0.85;	// Value for crops with N limitation

/// Lambert-Beer extinction law (Prentice et al 1993; Monsi & Saeki 1953)
inline double lambertbeer(double lai) {
	return exp(-.5 * lai);
}

/// Alternative parameterisations of the convective boundary layer
/**
 *	AET_MONTEITH_HYPERBOLIC = hyperbolic parameterisation (Huntington & Monteith 1998)
 *	AET_MONTEITH_EXPONENTIAL = exponential parameterisation (Monteith 1995)
 *	aet_monteith		Returns AET given equilibrium evapotranspiration and canopy conductance
 *	gc_monteith			Returns canopy conductance given AET and equilibrium evapotranspiration
 */

// Comment out one of the following two lines:
#define AET_MONTEITH_HYPERBOLIC
//#define AET_MONTEITH_EXPONENTIAL

// Check:
#if defined(AET_MONTEITH_HYPERBOLIC) && defined(AET_MONTEITH_EXPONENTIAL)
#error Only one of AET_MONTEITH_HYPERBOLIC and AET_MONTEITH_EXPONENTIAL should be #defined
#elif !defined(AET_MONTEITH_HYPERBOLIC) && !defined(AET_MONTEITH_EXPONENTIAL)
#error One of AET_MONTEITH_HYPERBOLIC and AET_MONTEITH_EXPONENTIAL must be #defined
#endif

#if defined(AET_MONTEITH_EXPONENTIAL)

const double ALPHAM = 1.4;
const double GM = 5.0;

inline double aet_monteith(double& eet, double& gc) {
	return negligible(gc) ? 0.0 : eet*ALPHAM*(1.0-exp(-gc/GM));
}

inline double gc_monteith(double& aet, double& eet) {
	if (negligible(eet)) return 0.0;
	double t = aet/eet/ALPHAM;
	if (t >= 1.0) fail("gc_monteith: invalid value for aet/eet/ALPHAM");
	return -GM * log(1.0 - t);
}
#elif defined(AET_MONTEITH_HYPERBOLIC)

const double ALPHAM = 1.391;
const double GM = 3.26;

inline double aet_monteith(double& eet, double& gc) {
	return eet*ALPHAM*gc/(gc+GM);
}

inline double gc_monteith(double& aet, double& eet) {
	return (aet*GM) / (eet*ALPHAM-aet);
}
#endif

#endif // LPJ_GUESS_CANEXCH_H
