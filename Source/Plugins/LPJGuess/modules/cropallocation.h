////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file cropallocation.h
/// \brief Crop allocation and growth
/// \author Mats Lindeskog
/// $Date: 2015-11-13 16:25:45 +0100 (Fri, 13 Nov 2015) $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_CROPALLOCATION_H
#define LPJ_GUESS_CROPALLOCATION_H

/// Updates patch members fpc_total and fpc_rescale for crops (to be called after crop_phenology())
void update_patch_fpc(Patch& patch);
/// Handles daily crop allocation and daily lai calculation
void growth_daily(Patch& patch);
/// Updates yearly lai and fpc for cropland
void allometry_crop(Individual& indiv);
/// Transfer of this year's growth (ycmass_xxx) to cmass_xxx_inc in growth()
void growth_crop_year(Individual& indiv, double& cmass_leaf_inc,double& cmass_root_inc,double& cmass_ho_inc,double& cmass_agpool_inc, double& cmass_stem_inc);

#endif // LPJ_GUESS_CROPALLOCATION_H
