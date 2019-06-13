////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file cropphenology.h
/// \brief Crop phenology including phu calculations
/// \author Mats Lindeskog
/// $Date: 2015-11-13 16:25:45 +0100 (Fri, 13 Nov 2015) $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_CROPPHENOLOGY_H
#define LPJ_GUESS_CROPPHENOLOGY_H

/// Calculation of down-scaling of lai during crop senescence
double senescence_curve(Pft& pft, double fphu);
/// Handles heat unit and harvest index calculation and identifies harvest, senescence and intercrop events.
void crop_phenology(Patch& patch);
/// Updates crop phen from yesterday's lai_daily
void leaf_phenology_crop(Pft& pft, Patch& patch);

#endif // LPJ_GUESS_CROPPHENOLOGY_H
