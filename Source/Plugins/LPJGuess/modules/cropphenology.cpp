////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file cropphenology.cpp
/// \brief Crop phenology including phu calculations
/// \author Mats Lindeskog
/// $Date:  $
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "landcover.h"
#include "cropphenology.h"

const double MAXHUTEMP = 30;         // Degree limit for heat unit summation

/// Calculation of down-scaling of lai during crop senescence
/** Follows Bondeau et al. 2007.
 */
double senescence_curve(Pft& pft, double fphu) {

	if (pft.shapesenescencenorm)
		return pow(1-fphu,2) / pow(1 - pft.fphusen, 2) * (1 - pft.flaimaxharvest) + pft.flaimaxharvest;
	else
		return pow(1 - fphu, 0.5) / pow(1 - pft.fphusen, 0.5) * (1 - pft.flaimaxharvest) + pft.flaimaxharvest;
}

/// Initiation of potential heat unit calculation
/** Calculates pvd (required vernalising days), tb (base temperature)
 *  and phu (potential heat units) based on Bondeau et al. 2007.
 *  Dynamic phu calculation based on Lindeskog et al. 2013.
 *  Called on sowing date.
 */
void phu_init(cropphen_struct& ppftcrop, Gridcellpft& gridcellpft, Patch& patch) {

	Pft& pft = gridcellpft.pft;
	const Climate& climate = patch.get_climate();
	double phu_last_year = ppftcrop.phu;

	ppftcrop.husum = 0.0;
	ppftcrop.vrf = 1.0;
	ppftcrop.vdsum = 0;
	ppftcrop.prf = 1.0;

	ppftcrop.pvd = pft.pvd;		// default; kept for TrMi, TePu, TeSb, TrMa, TeSo, TrPe
	ppftcrop.phu = pft.phu;
	ppftcrop.tb = pft.tb;

	ppftcrop.vdsum_alloc=0.0;
	ppftcrop.vd=0.0;
	ppftcrop.dev_stage=0.0;

	// Calculation of phu and pvd according to Bondeau et al. 2007
	if (pft.ifsdautumn) {	// TeWW,TeRa

		// maximum pvd allowed
		const int max_pvd = 60;

		if (gridcellpft.wintertype) {	// Autumn sowing
			// if neither spring or winter conditions for the past 20 years
			if ((gridcellpft.first_autumndate20 == climate.testday_temp || gridcellpft.first_autumndate20 == climate.coldestday)
				&& gridcellpft.first_autumndate % date.year_length() == gridcellpft.first_autumndate20
				&& (gridcellpft.last_springdate20 == climate.testday_temp || gridcellpft.last_springdate20 == climate.coldestday)
				&& gridcellpft.last_springdate == gridcellpft.last_springdate20) {

				ppftcrop.pvd = pft.pvd;
				ppftcrop.phu = pft.phu;
			}
			// not all past 20 years without vernendoccurred: too cold
			else if (!(gridcellpft.last_verndate20 == gridcellpft.last_springdate20 + def_verndate_ndays_after_last_springdate 
						&& gridcellpft.last_verndate == gridcellpft.last_verndate20)) {

				// pvd (required vernalising days): undocumented equations from Bondeau's code

				// vernalization (below 12 degrees) is supposed to occur directly at sowing (trg=tempautumn) for TeWW, for TeRa a 20-day lag (tempautumn=17)
				// first_autumndate20 occurred before last_verndate20
				if ((ppftcrop.sdate < 180 || gridcellpft.last_verndate20 >= 180) && climate.lat >= 0.0 || climate.lat < 0.0) {
					ppftcrop.pvd = (int)max(0, min(max_pvd, gridcellpft.last_verndate20 - ppftcrop.sdate - pft.vern_lag));
				}
				// first_autumndate20 occurred after last_verndate20
				else {
					ppftcrop.pvd = (int)max(0, min(max_pvd, gridcellpft.last_verndate20 + date.year_length() - ppftcrop.sdate - pft.vern_lag));
				}

				// phu (potential heat units): quadratic calculation according to sowing month; see Fig.2 in Bondeau et al.2007

				const double k1 = -0.1081;
				const double k2 = 3.1633;

				if (ppftcrop.sdate < 184 + climate.adjustlat) {
					ppftcrop.phu = max(pft.phu_min, k1 * pow((double)(ppftcrop.sdate - climate.adjustlat),2) + k2 * ((double)(ppftcrop.sdate - climate.adjustlat)) + pft.phu_max);
				}
				else {
					ppftcrop.phu = max(pft.phu_min, k1 * pow((double)ppftcrop.sdate - date.year_length(),2) + k2 * ((double)ppftcrop.sdate - 365) + pft.phu_max);
					ppftcrop.phu *= pft.phu_red_spring_sow;	// undocumented reduction in spring variants from Bondeau's code
				}
			}
		}
		else {	// spring sowing

			// default pvd of spring varieties when vernalisation has not occurred during the past 20 years
			const int pvd_def_spring = 30;
			// phu of spring varieties when vernalisation has occurred during the past 20 years
			const double phu_spring_ifvern = 1300.0;
			// phu of spring varieties when vernalisation has not occurred during the past 20 years
			const double phu_spring_ifnvern = 1500.0;

			// If last_verndate has occurred during the past 20 years (or too warm):
			if (!(gridcellpft.last_verndate20 == ppftcrop.sdate + def_verndate_ndays_after_last_springdate 
					&& gridcellpft.last_verndate == gridcellpft.last_verndate20)) {
				ppftcrop.pvd = (int)min(max_pvd, gridcellpft.last_verndate20 - ppftcrop.sdate);
				ppftcrop.phu = phu_spring_ifvern;
			}
			// If no last_verndate occurred during the past 20 year (too cold):
			else {
				ppftcrop.pvd = pvd_def_spring;
				ppftcrop.phu = phu_spring_ifnvern;
			}
		}
	}
	else {

		// phu: Linear calculation according to sowing month; see Fig.2 in Bondeau et al.2007

		if (pft.phu_calc_lin) {
			ppftcrop.phu = min(pft.phu_max, max(pft.phu_min, -(pft.phu_max - pft.phu_min) / pft.ndays_ramp_phu * (double)(ppftcrop.sdate - climate.adjustlat) + pft.phu_interc));
		}
	}

	// Calculation of potential heat units according to local climate.
	if (ifcalcdynamic_phu) {

		const double min_phu = 900.0;	// minimum allowed phu based on phu range in the literature
		const int min_phu_sample_period = 20;	// minimum no. of years of the phu sample period

		// add last year's husum_max to running 10-year mean
		if (ppftcrop.husum_max) {
			ppftcrop.nyears_hu_sample++;
			int years = min(ppftcrop.nyears_hu_sample, 10);
			ppftcrop.husum_max_10 = (ppftcrop.husum_max_10 * (years - 1) + ppftcrop.husum_max) / years;
		}
		ppftcrop.husum_max = 0.0;

		ppftcrop.phu_old = ppftcrop.phu;		// phu_old for printout

		// set phu according to running mean
		if (ppftcrop.nyears_hu_sample)
			ppftcrop.phu = max(min_phu, 0.9 * ppftcrop.husum_max_10);

		// ... unless past specified time period
		if (ifdyn_phu_limit && date.year >= patch.stand.first_year + min_phu_sample_period && date.year >= nyear_spinup + nyear_dyn_phu)
			ppftcrop.phu = phu_last_year;
	}
}

/// Calculation of harvest index
/** Based on fphu. Restricted by water stress.
 *  SWAT equations are from Neitsch et al. 2002.
 */
void harvest_index(Patch& patch, Pft& pft) {

	double wdf, fwdf, hi_save;
	Patchpft& patchpft = patch.pft[pft.id];
	cropphen_struct& ppftcrop = *(patchpft.get_cropphen());

	ppftcrop.hi = pft.hiopt * 100 * ppftcrop.fphu / (100 * ppftcrop.fphu + exp(11.1 - 10.0 * ppftcrop.fphu));	// SWAT 5:2.4.1
	ppftcrop.fhi_phen = ppftcrop.hi / pft.hiopt;

	// Correction of HI according to water stress:
	ppftcrop.demandsum_crop += patch.wdemand;
	if (patchpft.wsupply > patch.wdemand)
		ppftcrop.supplysum_crop += patch.wdemand;
	else
		ppftcrop.supplysum_crop += patchpft.wsupply;

	if (ppftcrop.demandsum_crop > 0.0)
		wdf = 100.0 * ppftcrop.supplysum_crop / ppftcrop.demandsum_crop;	// SWAT 5:3.3.2	: aetsum/petsum
	else
		wdf = 100.0;

	fwdf = wdf / (wdf + exp(6.13 - 0.0883 * wdf));		// (SWAT 5:3.3.1)

	hi_save = ppftcrop.hi;

	ppftcrop.hi = ppftcrop.fhi_phen * ((pft.hiopt - pft.himin) * fwdf + pft.himin);

	if (ppftcrop.hi > 0.0 && hi_save)
		ppftcrop.fhi_water = ppftcrop.hi / hi_save;

	ppftcrop.fhi = ppftcrop.fhi_phen * ppftcrop.fhi_water;
}

/// Calculation of accumulated of heat units
/** Accumulation of heat units during sampling period used for calculation of
 *  dynamic phu if ifcalcdynamic_phu = true. SWAT equation is from Neitsch et al. 2002
 */
void heat_units(Patch& patch, Pft& pft) {

	Patchpft& patchpft = patch.pft[pft.id];
	cropphen_struct& ppftcrop = *(patchpft.get_cropphen());
	const Climate& climate = patch.get_climate();

	// calculation av fphu:
	double hu = max(0.0, min(climate.temp, MAXHUTEMP) - ppftcrop.tb);

	// account for vernalization if needs for vernalization not yet satisfied	//trg=tb for crops other than TeWW and TeRa and don't enter here
	if (climate.temp < pft.trg && ppftcrop.vdsum < ppftcrop.pvd) {				//trg=12 for TeWW & TeRa

		ppftcrop.vdsum++;
		ppftcrop.vrf = min(1.0, (double)ppftcrop.vdsum / (double)ppftcrop.pvd);

		// vernalisation reduction factor has no effect once temp>trg even if needs are not satisfied
		// no effect as well if temp>trg at the beginning of the growing season...
		hu *= ppftcrop.vrf;
	}

	// account for response to photoperiod
	ppftcrop.prf = (1 - pft.psens) * min(1.0, max(0.0, (climate.daylength_save[date.day] - pft.pb) / (pft.ps - pft.pb))) + pft.psens;
	hu *= ppftcrop.prf;

	if (date.day == ppftcrop.sdate) {
		ppftcrop.husum = 0.0;
	}

	// Accumulate heat units during growing period
	if (ppftcrop.growingseason) {
		// daily effective temperature sum (degree-days)
		ppftcrop.husum += hu;

		// phenological scale (fraction of growing season)
		ppftcrop.fphu = min(1.0, ppftcrop.husum / ppftcrop.phu);		// SWAT 5:2.1.11
	}

	// Sample heat units for dynamic phu calculation
	if (ifcalcdynamic_phu) {

		if (date.day == ppftcrop.sdate) {
			ppftcrop.hu_samplingperiod = true;
			ppftcrop.hu_samplingdays = 0;
			ppftcrop.husum_sampled = 0;
		}

		if (ppftcrop.hu_samplingperiod) {

			ppftcrop.husum_sampled += hu;
			ppftcrop.hu_samplingdays++;

			if (date.day == ppftcrop.hucountend) {

				ppftcrop.husum_sampled -= hu;					// Don't count the hu's on last day
				ppftcrop.husum_max = ppftcrop.husum_sampled;

				ppftcrop.hu_samplingperiod = false;
			}
		}
	}
}

/// Temperature factor used in development stage calculation
inline double temperature_factor(double temp, double min, double opt, double max) {
	if (temp <= min || temp >= max) {
		return 0;
	}
	double _opt = opt - min;
	double alpha = log(2.) / log((max - min) / _opt);
	return (2 * pow(temp - min, alpha) * pow(_opt, alpha) - pow(temp - min, 2*alpha))/
		pow(_opt, 2*alpha);
}

/// Calculation of development stage
/** Accumulation of development during sampling period, based on Wang & Engel 1998.
 */
void development_stage(Patch& patch, Pft& pft) {

	Patchpft& patchpft = patch.pft[pft.id];
	cropphen_struct& ppftcrop = *(patchpft.get_cropphen());
	const Climate& climate = patch.get_climate();

	// account for vernalization if needs for vernalization not yet satisfied	//trg=tb for crops other than TeWW and TeRa and don't enter here
	if (ppftcrop.vdsum_alloc < 1 && climate.temp > pft.T_vn_min && climate.temp < pft.T_vn_max)	{
		ppftcrop.vd += temperature_factor(climate.temp, pft.T_vn_min, pft.T_vn_opt, pft.T_vn_max);
		double vd5 = pow(ppftcrop.vd, 5.);
		ppftcrop.vdsum_alloc = min(1.0, vd5 / (pow(22.5, 5.0) + vd5));
	}

	double daylength = max(0.0, climate.daylength_save[date.day] - pft.photo[0]);
	double e = exp(-pft.photo[1] * daylength);
	double fP = min(1.0, pft.photo[2] > 0 ? e : 1.0 - e);

	double T_min = pft.T_veg_min;
	double T_opt = pft.T_veg_opt;
	double T_max = pft.T_veg_max;

	if (ppftcrop.dev_stage >= 1) {
		T_min = pft.T_rep_min;
		T_opt = pft.T_rep_opt;
		T_max = pft.T_rep_max;
	}

	double fT = min(1.0, temperature_factor(climate.temp, T_min, T_opt, T_max));
	double dev_rate = 0.0;
	if (ppftcrop.dev_stage < 1.0)
		dev_rate = pft.dev_rate_veg * ppftcrop.vdsum_alloc * fP * fT;
	else
		dev_rate = pft.dev_rate_rep * fT;

	ppftcrop.dev_stage = min(2.0, ppftcrop.dev_stage + dev_rate);
}

/// Handles heat unit and harvest index calculation and identifies harvest, senescence and intercrop events.
/** Accumulation of heat units during sampling period used for calculation of dynamic phu if DYNAMIC_PHU defined.
 *  Sets patchpft.cropphen variables growingseason, hdate, intercropseason and senescence
 */
void crop_phenology(Patch& patch) {

	patch.pft.firstobj();
	while (patch.pft.isobj) {
		Patchpft& patchpft = patch.pft.getobj();
		Pft& pft=patchpft.pft;
		Standpft& standpft = patch.stand.pft[pft.id];
		Gridcell& gridcell = patch.stand.get_gridcell();
		Climate& climate = gridcell.climate;
		Gridcellpft& gridcellpft = gridcell.pft[pft.id];

		if (patch.stand.pft[pft.id].active) {

			if (pft.phenology == CROPGREEN) {

				cropphen_struct& ppftcrop = *(patchpft.get_cropphen());
				ppftcrop.growingseason_ystd = ppftcrop.growingseason;

				// resets on first day of the year:
				if (date.day == 0) {
					ppftcrop.fphu_harv = -1.0;
					ppftcrop.fhi_harv = -1.0;
					ppftcrop.sdate_harv = -1;
					ppftcrop.nsow = 0;
					ppftcrop.sendate = -1;
					ppftcrop.nharv = 0;

					ppftcrop.sownlastyear=false;

					for(int i=0;i<2;i++) {
						ppftcrop.sdate_harvest[i] = -1;
						ppftcrop.hdate_harvest[i] = -1;
						ppftcrop.sdate_thisyear[i] = -1;
					}
				}

				// initiations on sowing day:
				if (date.day == ppftcrop.sdate) {
					ppftcrop.fphu = 0.0;
					ppftcrop.fhi = 0.0;
					ppftcrop.fhi_phen = 0.0;
					ppftcrop.fhi_water = 1.0;
					ppftcrop.hdate = -1;
					ppftcrop.bicdate = -1;

					ppftcrop.growingseason = true;
					ppftcrop.growingdays = 0;
					ppftcrop.nsow++;

					if (ppftcrop.nsow == 1)
						ppftcrop.sdate_thisyear[0] = ppftcrop.sdate;
					else if (ppftcrop.nsow == 2)
						ppftcrop.sdate_thisyear[1] = ppftcrop.sdate;

					// calculate pvd, phu & tb
					phu_init(ppftcrop, gridcellpft, patch);
				}

				// Calculation of accumulated heat units and harvest index from sowing to maturity
				if (ppftcrop.growingseason) {

					ppftcrop.senescence_ystd = ppftcrop.senescence;
					ppftcrop.intercropseason = false;
					ppftcrop.growingdays++;

					// check if harvest is prescribed
					bool force_harvest = date.day == standpft.hdate_force;

					// before maturity is reached
					bool pre_maturity = ifnlim ? ppftcrop.dev_stage < 2.0 : ppftcrop.husum < ppftcrop.phu;

					if (pre_maturity && dayinperiod(date.day, ppftcrop.sdate, stepfromdate(ppftcrop.hlimitdate, -1)) && !force_harvest) {

						// count accumulated heat units after sowing date
						heat_units(patch, pft);

						if (ifnlim)
							development_stage(patch, pft);

						//  test for senescence
						if (ppftcrop.fphu >= pft.fphusen) {

							if (ppftcrop.senescence_ystd == false)
								ppftcrop.sendate = date.day;
							ppftcrop.senescence = true;
						}

						// calculated harvest index
						harvest_index(patch, pft);

					}
					else {	// harvest

						// save today as harvest day
						ppftcrop.hdate = date.day;

						ppftcrop.growingseason = false;
						ppftcrop.intercropseason = false;
						ppftcrop.senescence = false;

						ppftcrop.fertilised[0] = false;
						ppftcrop.fertilised[1] = false;
						ppftcrop.fertilised[2] = false;

						// set start of intercrop grass growth
						ppftcrop.bicdate = stepfromdate(ppftcrop.hdate, 15);

						// count number of harvest events this year
						ppftcrop.nharv++;

						// Save phenological values and dates at harvest:
						ppftcrop.fphu_harv = ppftcrop.fphu;
						ppftcrop.fhi_harv = ppftcrop.fhi;
						ppftcrop.sdate_harv = ppftcrop.sdate;
						ppftcrop.lgp = ppftcrop.growingdays;

						// allowing saving at two harvests per year
						if (ppftcrop.nharv == 1) {
							ppftcrop.sdate_harvest[0] = ppftcrop.sdate;
							ppftcrop.hdate_harvest[0] = date.day;
							if (ppftcrop.sdate > date.day)							
								ppftcrop.sownlastyear = true;
						}
						else if (ppftcrop.nharv == 2) {
							ppftcrop.sdate_harvest[1] = ppftcrop.sdate;
							ppftcrop.hdate_harvest[1] = date.day;
						}

						ppftcrop.demandsum_crop = 0.0;
						ppftcrop.supplysum_crop = 0.0;
						ppftcrop.sdate = -1;
						ppftcrop.eicdate = -1;
					} //end harvest
				}  //from sowing has taken place until harvest day

				// continue sampling heat units from hdate until last sampling date
				if (ifcalcdynamic_phu && ppftcrop.growingseason == false && ppftcrop.hu_samplingperiod) {
					heat_units(patch, pft);
				}

				if (patch.stand.pftid == pft.id && stlist[patch.stand.stid].intercrop == NATURALGRASS) {

					if (!ppftcrop.intercropseason && date.day == ppftcrop.bicdate) {
						ppftcrop.intercropseason = true;
					}

					if (date.day == ppftcrop.eicdate) {
						ppftcrop.intercropseason = false;
					}
				}
			}
			else if (pft.phenology == ANY) { // crop grasses using standard guess phenology calculation

				if (patch.stand.pftid != pft.id) {
					cropphen_struct& ppftcrop = *(patchpft.get_cropphen());

					if (date.day == 0) {
						ppftcrop.nharv = 0;
					}

					if (date.day == patch.pft[patch.stand.pftid].cropphen->bicdate) {
						ppftcrop.growingseason = true;
						ppftcrop.growingdays = 0;
					}
					else if (date.day == patch.pft[patch.stand.pftid].cropphen->eicdate) {
						ppftcrop.growingseason = false;
						ppftcrop.lgp = ppftcrop.growingdays;
					}
					if (ppftcrop.growingseason == true) {
						ppftcrop.growingdays++;
					}
				}
			}
		}

		patch.pft.nextobj();
	}
}

/// Updates crop phen from yesterday's lai_daily
/** True crops derive phen from yesterday's fpc_daily, assuming only one crop individual.
 *  Intercrop grass and pasture grass grown in crop stands use gdd5 and is treated
 *  similar to pasture grass.
 */
void leaf_phenology_crop(Pft& pft, Patch& patch) {

	Gridcell& gridcell = patch.stand.get_gridcell();
	Climate& climate = gridcell.climate;
	Patchpft& patchpft = patch.pft[pft.id];
	cropphen_struct& ppftcrop = *(patchpft.get_cropphen());

	if (pft.phenology == CROPGREEN) {
		if (ppftcrop.growingseason) {

			Vegetation& vegetation = patch.vegetation;

			vegetation.firstobj();
			while (vegetation.isobj) {
				Individual& indiv = vegetation.getobj();

				if (indiv.pft.id == pft.id) {
					patchpft.phen = indiv.fpc ? indiv.fpc_daily / indiv.fpc : 0;
				}

				vegetation.nextobj();
			}
		}
		else if (date.day == ppftcrop.hdate) {
			patchpft.phen = 0.0;
		}
	}
	else if (pft.phenology == ANY) { // crop grasses using standard guess phenology calculation

		if (patch.stand.pftid == pft.id // pasture grass in crop stand
			|| patch.stand.hasgrassintercrop && (patch.pft[patch.stand.pftid].cropphen->intercropseason // intercrop grass
			|| date.day == patch.pft[patch.stand.pftid].cropphen->eicdate)) {							// intercrop grass

			if (patch.stand.pftid != pft.id) {

				if (date.day == patch.pft[patch.stand.pftid].cropphen->bicdate) {
					patch.stand.gdd0_intercrop = climate.gdd5;
				}
				if (date.day == patch.pft[patch.stand.pftid].cropphen->eicdate) {
					patch.stand.gdd0_intercrop = 0.0;
					patchpft.phen = 0.0;
				}
			}

			// reset stand.gdd0_intercrop same day as gdd5
			if (climate.lat >= 0.0 && date.day == COLDEST_DAY_NHEMISPHERE
				|| climate.lat < 0.0 && date.day == COLDEST_DAY_SHEMISPHERE
				|| climate.gdd5 == 0.0) {

				patch.stand.gdd0_intercrop = 0.0;
			}

			if (ppftcrop.growingseason) {	// includes bicdate, not eicdate

				if (patch.stand.pftid == pft.id || gridcell.pft[patch.stand.pftid].sowing_restriction) { // Normal grass growth: gives identical result to natural stands.
					patchpft.phen = min(1.0, climate.gdd5 / pft.phengdd5ramp);
				}
				else if (patch.stand.gdd0_intercrop > 0.0) {
					patchpft.phen = min(1.0, (climate.gdd5 - patch.stand.gdd0_intercrop) / (pft.phengdd5ramp * 0.9)); // intercrop grass
				}
				else {
					patchpft.phen = min(1.0, (climate.gdd5 - patch.stand.gdd0_intercrop) / pft.phengdd5ramp); // intercrop grass
				}

				if (patchpft.phen < 0.0) {
					patchpft.phen = 0.0;
				}

				// raingreen phenology
				if (patchpft.wscal < pft.wscal_min) {

					patchpft.phen = 0.0;
					patch.stand.gdd0_intercrop = climate.gdd5;
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////
// REFERENCES
//
// Bondeau A, Smith PC, Zaehle S, Schaphoff S, Lucht W, Cramer W, Gerten D, Lotze-Campen H,
//   Müller C, Reichstein M & Smith B 2007. Modelling the role of agriculture for the
//   20th century global terrestrial carbon balance. Global Change Biology, 13:679-706.
// Lindeskog M, Arneth A, Bondeau A, Waha K, Seaquist J, Olin S, & Smith B 2013.
//   Implications of accounting for land use in simulations of ecosystem carbon cycling
//   in Africa. Earth Syst Dynam 4:385-407.
// Neitsch SL, Arnold JG, Kiniry JR et al.2002 Soil and Water Assessment Tool, Theorethical
//   Documentation + User's Manual. USDA_ARS-SR Grassland, Soil and Water Research Laboratory.
//   Agricultural Reasearch Service, Temple,Tx, US.
// Wang, E. and Engel, T. 1998 Simulation of phenological development
//   of wheat crops, Agr. Syst., 58, 1-24
