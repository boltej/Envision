///////////////////////////////////////////////////////////////////////////////////////
/// \file cru_ts30.h
/// \brief Functions for reading the CRU-NCEP data set
///
/// The binary files contain CRU-NCEP half-degree global historical climate data
/// for 1901-2015.
///
/// $Date: 2017-04-05 15:04:09 +0200 (Wed, 05 Apr 2017) $
///
///////////////////////////////////////////////////////////////////////////////////////

#ifndef LPJ_GUESS_CRU_TS30_H
#define LPJ_GUESS_CRU_TS30_H

namespace CRU_TS30 {

/// number of years of historical climate
/** CRU-NCEP v7 has 150 years of data (1950-2005) */
const int NYEAR_HIST=56;

/// calendar year corresponding to first year in CRU climate data set
static const int FIRSTHISTYEAR=1979;

/// Determine temp, precip, sunshine & soilcode
bool searchcru(char* cruark,double dlon,double dlat,int& soilcode,
               double mtemp[NYEAR_HIST][12],
               double mprec[NYEAR_HIST][12],
               double msun[NYEAR_HIST][12]);

/// Determine elevation, frs frq, wet frq & DTR
bool searchcru_misc(char* cruark,double dlon,double dlat,int& elevation,
                    double mfrs[NYEAR_HIST][12],
                    double mwet[NYEAR_HIST][12],
                    double mdtr[NYEAR_HIST][12]);

/// Returns CRU data from the nearest cell to (lon,lat) within a given search radius
/* * lon and lat are set to the coordinates of the found CRU gridcell, if found
bool findnearestCRUdata(double searchradius, char* cruark, double& lon, double& lat, 
                        int& scode, 
                        double hist_mtemp1[NYEAR_HIST][12], 
                        double hist_mprec1[NYEAR_HIST][12], 
                        double hist_msun1[NYEAR_HIST][12]); */

}

#endif // LPJ_GUESS_CRU_TS30_H
