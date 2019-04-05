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
// this file include "standard" constants used across multiple plug_in modules.


#pragma once


// Path Manager "guaranteed path manager entries" indices, used in PathManager::GetPath()
enum     
   {
   PM_ENVISION_DIR = 0,    // envision.exe directory
   PM_PROJECT_DIR  = 1,    // envx file directory
   PM_IDU_DIR      = 2,    // IDU shapefile directory
   PM_OUTPUT_DIR   = 3     // Output path (= IDU_DIR\Outputs\<ScenarioName> during the course of a run, otherwise IDU_DIR\Outputs)
   };


// disurbances _ typically used in a "DISTURB" column in an IDU coverage
enum DISTURB
   {
   NO_DISTURBANCE=0,
   HARVEST = 1,
   THINNING = 2,
   PARTIAL_HARVEST = 3,
   RESTORATION = 4,
   PLANTATION = 5,
   SELECTION_HARVEST = 6,
   DEVELOPED_TO_MIXED = 11,
   DEVELOPED_TO_WOODLAND = 12,
   DEVELOPED_TO_SAVANNA = 13,
   SURFACE_FIRE = 20,
   LOW_SEVERITY_FIRE = 21,
   HIGH_SEVERITY_FIRE = 22,
   STAND_REPLACING_FIRE = 23,
   PRESCRIBED_SURFACE_FIRE = 29,
   PRESCRIBED_LOW_SEVERITY_FIRE = 30,
   PRESCRIBED_HIGH_SEVERITY_FIRE = 31,
   PRESCRIBED_STAND_REPLACING_FIRE = 32,
   
   // forest restoration management
   MECHANICAL_THINNING = 50,
   MOWING_GRINDING = 51,
   SALVAGE_HARVEST = 52,
   SUPRESSION = 53,
   EVEN_AGED_TREATMENT = 54,
   THIN_FROM_BELOW = 55,
   PARTIAL_HARVEST_LIGHT = 56,
   PARTIAL_HARVEST_HIGH = 57,

   // fire treatments
   MECHANICAL_TREATMENT = 60,

   // climate_change_induced PVT changes 1XX
   // PVT index, PVT abbreviation
   // 1, fdw
   // 2, fmh
   // 3, fsi
   // 4, fto
   // 5, fvg
   // 6, fwi
   // 7, fuc
   // 8, ftm
   // 9, fdd
   // DISTURB value = 100 + 10*index of source PVT + index of destination PVT
   CC_DISTURB_LB = 110,  // works for up to 9 PVTs

   FDD2FDW = 191,
   FDD2FMH = 192,
   FDD2FSI = 193,
   FDD2FTO = 194,
   FDD2FUC = 197,
   FDD2FVG = 195,
   FDD2FWI = 196,
   FDD2FTM = 198,

   FDW2FTM = 118,
   FDW2FVG = 115,
   FDW2FWI = 116,

   FMH2FDD = 129,
   FMH2FSI = 123,
   FMH2FTO = 124,
   FMH2FVG = 125,
   FMH2FWI = 126,
   FMH2FUC = 127,

   FSI2FDW = 131,
   FSI2FDD = 139,
   FSI2FMH = 132,
   FSI2FTO = 134,
   FSI2FVG = 135,
   FSI2FWI = 136,
   FSI2FUC = 137,
   FSI2FTM = 138,

   FTM2FDW = 181,

   FTO2FVG = 145,
   FTO2FWI = 146,
   FTO2FUC = 147,
   FTO2FDD = 149,

   FUC2FDD = 179,
   FUC2FVG = 175,
   FUC2FWI = 176,
   FUC2FSI = 173,

   FVG2FDD = 159,
   FVG2FDW = 151,
   FVG2FSI = 153,
   FVG2FTO = 154,
   FVG2FWI = 156,
   FVG2FUC = 157,
   FVG2FTM = 158,

   FWI2FDD = 169,
   FWI2FDW = 161,
   FWI2FMH = 162,
   FWI2FSI = 163,
   FWI2FTO = 164,
   FWI2FVG = 165,
   FWI2FUC = 167,
   FWI2FTM = 168,

  
   CC_DISTURB_UB = 200,  // works for up to 9 PVTs

   // miscellaneous
   ABANDONMENT = 1000,

   AREC_TRANSITION = 1010 // Land use change generated by an AREC model (AREC=Agricultural & Resource Economics Dept in OSU College of Ag Science)
   // As of 6/18/13 there are 4 AREC transitions: to CROP, to PASTURE, to FOREST, and to URBAN.
   // These are encoded as AREC_TRANSITION+0, AREC_TRANSITION+1, AREC_TRANSITION+2, AREC_TRANSITION+3
   };

/*
    <attributes>
        <attr label="No Disturbance" color="(255,255,255)"  value="0" />
        <attr label="Harvest" color="(128,64,0)"  value="1" />
        <attr label="Thinning" color="(180,138,131)"  value="2" />
        <attr label="Partial Harvest" color="(180,138,131)"  value="3" />

        <attr label="Developed to Mixed" color="(250,55,55)"  value="11" />
        <attr label="Developed to Woodland" color="(190,255,10)"  value="12" />
        <attr label="Developed to Savanna" color="(255,255,255)"  value="13" />

        <attr label="Surface Fire" color="(195,80,80)"  value="20" />
        <attr label="Mixed Severity Low Burn " color="(215,60,60)"  value="21" />
        <attr label="Mixed Severity High Burn" color="(235,40,40)"  value="22" />
        <attr label="Stand Replacing Fire" color="(255,20,20)"  value="23" />
        <attr label="Prescribed Burn" color="(185,215,215)"  value="29" />
    </attributes>
*/
// management strategies _ typically used in a "MANAGE" column in an IDU coverage
enum MANAGE
   {
   Not_Managed =                        0,
   Structural_Savanna_Restoration =    10,  
   High_quality_Savanna_Restoration =  11,
   Structural_Woodland_Restoration =   12,
   High_quality_Woodland_Restoration = 13,
   Fire_Hazard_Reduction =             14
   };

// conservation strategies _ typically used in a "CONSERVE" column in an IDU coverage
enum CONSERVE
   {
   NOT_CONSERVED = 0,
   CONSERVATION_EASEMENT = 1,
   LOW_INTENSITY_FOREST_MANAGEMENT = 2,
   DEVELOPMENT_RIGHTS_TRANSFER = 3,
   DEVELOPMENT_RIGHTS_RESTRICTION = 4
   };