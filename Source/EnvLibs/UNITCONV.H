/*
This file is part of Envision.

Envision is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Envision is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Envision.  If not, see <http://www.gnu.org/licenses/>

Copywrite 2012 - Oregon State University

*/
//-------------------------------------------------------------------
//   File: UNITCONV.H
//
//   Unit conversion factors.  Factors specific to water chemistry
//   are in wc.h.
//-------------------------------------------------------------------

#if !defined _UNITCONV_PER_H
#define _UNITCONV_PER_H

#include "EnvLibs.h"


enum UNIT_TYPE { UT_UNDEFINED, UT_LENGTH, UT_AREA, UT_VOLUME, UT_FORCE, UT_MASS };

enum UNIT_MEASURE { U_UNDEFINED,
   U_SECONDS, U_MINUTES, U_HOURS, U_DAYS, U_YEARS, 
   U_IN, U_FT, U_YD, U_MILE, 
   U_CM, U_M, U_KM };


//-- length -------------------------------------------------------
const float M_PER_KM    =      1000.0f      ;
const float MM_PER_M    =      1000.0f      ;    // millimeters per meter
const float MM_PER_CM   =        10.0f      ;    // millimeters per cm
const float CM_PER_M    =       100.0f      ;    // cm per m
const float M_PER_CM    =         0.01f     ;    // m per cm
const float CM_PER_MM   =         0.1f      ;    // cm per mm
const float KM_PER_M    =         0.001f    ;    // km per m
const float M_PER_MM    =         0.001f    ;    // m per mm
const float IN_PER_MM   =         0.03937f  ;
const float IN_PER_CM   =         0.3937f   ;
const float FT_PER_M    =         3.281f    ;
const float M_PER_FT    =         0.3048f   ;
const float FT_PER_MILE =      5280.0;
const float MI_PER_M    = 0.000621371f;   // Miles per Meter
const float MI_PER_KM   = 0.621371f;      // Miles per Kilometer
const float KM_PER_MI   = 1.609f;         // Kilometer per mile


//-- area --------------------------------------------------------
const float M2_PER_HA   =      10000.0f   ;    // m2 per hectare
const float HA_PER_M2   =      0.0001f    ;    // hectare per m2
const float FT2_PER_M2  =     10.76f      ;    // ft2 per m2
const float ACRE_PER_HA =      2.471f     ;    // acres/hectare     
const float FT2_PER_ACRE=  43560.0f       ;    // ft2/acre 
const float M2_PER_ACRE =   4046.8564224f ;    // m2/acre
const float ACRE_PER_M2 =   0.000247105f  ;    // acre/m2
const float M2_PER_MI2  = 2589988.11f     ;    // m2/sq mi
const float MI2_PER_M2  = 3.8610216e-07f  ;    // sq mi/m2
const float MI2_PER_KM2 = 3.8610216e-01f  ;    // sq mi/km2
const float FT2_PER_MI2 = 27878400        ;    // ft2 / sq mi
const float KM2_PER_MI2 = 2.5899881f      ;   // sq mi/km2


//-- volume ------------------------------------------------------
const float ML_PER_L    =      1000.0f    ;    // ml per L
const float L_PER_M3    =      1000.0f    ;    // Liters per cubic meter
const float CMD_PER_CFS =      2446.85f   ;    // m3/d per ft3/s
const float FT3_PER_M3  =        35.29f   ;    // ft3/m3    
const float M3_PER_ACREFT =    1233.48f   ;
const float ACREFT_PER_M3 = 0.0008107132f ;
const float M3_PER_GAL    = 0.00378541178f;

//-- mass -----------------------------------------------------------
const float KG_PER_G    =         0.001f  ;    // Kilograms per gram
const float G_PER_KG    =      1000.0f    ;    // Grams per kilogram
const float G_PER_MG    =         0.001f  ;    // g/mg
const float MG_PER_G    =      1000.0f    ;    // mg/g
const float MMOL_PER_MOL=      1000.0f    ;    // mmol per mol
const float LB_PER_KG   =         2.21f   ;    // lbs per kg
const float LB_PER_TON  =      2000.0f    ;    // lbs per ton


//-- energy ---------------------------------------------------------
const float N_PER_KN    =      1000.0f    ;    // N per kN
const float W_PER_KW    =      1000.0f    ;    // Watts per kilowatt
const float KW_PER_W    =         0.001f  ;    // kW/W
const float J_PER_KJ    =      1000.0f    ;
const float KJ_PER_J    =          .001f  ;
const float J_PER_CAL   =         4.1868f ;    // Joules per cal
const float KJ_PER_KWHR =      3600.0f    ;    // kJ/kWhr where J = one W/s

const float LANGLEY_PER_KJ  = 41.868f;                 // Langleys to kJ/m2
const float CALCM2_PER_KJM2 = 10000 * 4.1868f / 1000;  // cals/cm2 to kJ/m2
const float SOLARCONST  = 2880.0f;                 // Solar radn. intensity
                                                  //(cal/cm2/d) outside
                                                  //atmosphere
//-- Pressure. ------------------------------------------------------
//-- Ref: ASHRAE 1981 Fundamentals Handbook, ASAE ???
//-- Newton = N, Pascal = Pa, kg-m/s2 = N, kN/m2 = kP, kN/m3 = kP/m,
//-- psi = lbf/in2

const float  MMHG_PER_ATM  =      760.0f      ;  // mm Hg per atm (Hg temperature ???)
const float  MMHG_PER_KPA  =        7.500617f ;  // mm Hg per kP (Hg temperature ???)
const float  PA_PER_IN     =      248.84f     ;  // Pa per inch water at 60 F
const float  PA_PER_MBAR   =      100.0f      ;  // Pa per millibar
const float  PA_PER_PSI    =     6894.757f    ;  // Pa per psi
const float  PA_PER_ATM    =   101325.0f      ;  // Pa per standard atmosphere
const float  MBAR_PER_MMHG  =       1.333224f ;  // millibar per mm Hg at 0°C (???)


//-- Time. ----------------------------------------------------------
const float MIN_PER_DAY =      1440.0f    ;    // minutes per day
const float SEC_PER_MIN =        60.0f    ;    // seconds per minute
const float SEC_PER_HR  =      3600.0f    ;    // seconds per hour
const float SEC_PER_DAY =     86400.0f    ;    // Seconds per day
const float HR_PER_DAY  =        24.0f    ;    // Hours per day
const float DAY_PER_MONTH  =    365.25f/12;    // days / month



//-- Math constants.------------------------------------------------
#ifdef PI
#undef PI
#endif

const float BASE_PER_NAT_PER_LOG = 2.71828f;
const float RAD_PER_DEGREE = 3.1415927f / 180;
const float DEGREE_PER_RAD = 180 / 3.1415927f;
const float PI         = 3.1415927f;
#define RISERUN_PER_DEGREE( r ) ((r/200.0f)*90)
#define PERCENT_PER_DEGREE( r ) ((r/100.0f)*90)       // NON-decimal percent



#endif
