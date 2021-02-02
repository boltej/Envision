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
#pragma once

//#include <EnvExtension.h>
//
//#include <Vdataobj.h>
//#include <FDATAOBJ.H>
//#include <PtrArray.h>
//#include <UNITCONV.H>
//#include <randgen\randunif.hpp>
//
#include "F2R.h"
#include "ClimateManager.h"
#include <MovingWindow.h>

#include <map>
#include <vector>


enum VSMB_PET_METHOD
   {
   VPM_BAIER_ROBERTSON,
   VPM_PENMAN_MONTEITH,
   VPM_PRIESTLEY_TAYLOR,
   };



//=========== VSMB classes ============//
class SoilLayerParams
   {
   public:
      int m_iduID;                  //  soilCode is no longer needed: the IDU identifier will be used directly to identify records in soil input file
      float m_zoneThick;            //  zone depth = read from soil input file (mm)
      float m_sat;                  //  zone saturation = "POROSITY" / 100 (ratio)  
      float m_DUL;                  //  Drained upper limit = "FIELDCAP" / 100 (ratio)
      float m_PLL;                  //  lower limit of plant extractable soil water = "WILTINGPT" / 100 (ratio)
      float m_AWHC;                 //  zone available water holding capacity = calculated from values above (mm)
      float m_permWilt;
      //float voidCapacity;         //  void capacity(total porosity) for each zone
      // float m_WF;                //  weighting factor??? - calculated and used only in determineInfiltrationAndRunoff() not needed here
   };

// SoilLayerInfo holds info particular to a specific soil layer
// TODO: 1) review variables, 2) document units
class SoilLayerInfo
   {
   public:
      // member data
      float m_soilMoistContent;     //  soil moist content for each zone
      float m_kCoef;                //  soil moisture coefficient determined for differenc crop stages, used in AET calcualtion
      float m_SW;                   //  ? used in ceres model adaptation, soil water in layers(zones) volume fraction ?
      float m_SWX;                  //  ? used in ceres model adaptation, soil water in layers(zones) volume fraction ?
      float m_flux;                 //  downward flux ?
      float m_waterLoss;            //  water loss, used to determine AET
      float m_drain;                //
      float m_content2;             //  ?
      float m_delta;                //  ?
      float m_waterContent;         //  Soil water content
 
      SoilLayerParams *m_pLayerParams;

      // constructor
      SoilLayerInfo(SoilLayerParams *pParams)
         : m_soilMoistContent(2.f)
         , m_kCoef(0)
         , m_SW(0)
         , m_SWX(0)
         , m_flux(0)
         , m_waterLoss(0)
         , m_drain(0)
         , m_content2(0)
         , m_delta(0)
         , m_pLayerParams(pParams)
         { }

         
      //   dSoilMoistContent(0.0f), dKCoef(0.0f), SWX(0.0f), dFlux(0.0f), dWaterLoss(0.0f), dContent2(0.0f), dDelta(0.0f), WF(0.0f),
      //   SW(0.0f), dPermWilt(0.0f), dAWHC(0.0f), dZoneDepth(0.0f), DUL(0.0f), dSat(0.0f), PLL(0.0f), dVoidCapacity(0.0f) {}
   };

// SoilInfo holds info particular to a specific soil, including an array of soil layers
// TODO: 1) review variables, 2) document variables, units
class SoilInfo
   {
   public:
      // constructor
      SoilInfo(int idu, ClimateStation *pStation)
         : m_idu(idu)
         , m_pClimateStation(pStation)
         , m_tMin(0)
         , m_tAvg(0)
         , m_tMax(0)
         , m_T2(0)
         , m_precip(0)
         , m_rain(0)
         , m_snow(0)
         , m_snowWatEq(0)
         , m_packRetainMelt(0)
         , m_packSnowMelt(0)
         , m_meltDrain(0)
         , m_wleach(0)
         , m_dailySnowMelt(0)
         , m_dailyAET(0)
         , m_computedPET(0)
         , m_adjustedPET(0)
         , m_ppe(0)
         , m_soilTempAvg(0)
         , m_soilTempLast(0)
         , m_deficitTotal(0)
         , m_soilTempHistory(3)   // three day window
         , m_snowLoss(0)
         , m_snowCover(0)
         , m_petMethod(VPM_BAIER_ROBERTSON)
         , m_runoff(0)
         , m_runoffFrozen(0)
         , m_surfaceWater(0)

         , m_pResults(NULL)
         , m_pRootCoefficientTable(NULL)
         { }

      SoilInfo::~SoilInfo(void)
      {


         if (m_pResults != NULL)
            delete m_pResults;

         if (m_pRootCoefficientTable != NULL)//Should be member of VSMBModel
            delete m_pRootCoefficientTable;
      }

      // dynamic variables
      float m_tMin;
      float m_tAvg;
      float m_tMax;
      float m_T2;
      float m_precip; 
      float m_rain;
      float m_snow;
      float m_snowWatEq;
      float m_packRetainMelt;
      float m_packSnowMelt;
      float m_meltDrain;
      float m_wleach;
      float m_dailySnowMelt;
      float m_dailyAET;
      float m_computedPET;
      float m_adjustedPET;
      float m_ppe;
      float m_soilTempAvg;
      float m_soilTempLast;
      float m_deficitTotal;
      float m_snowLoss;
      float m_runoff;
      float m_runoffFrozen;
      float m_surfaceWater;

      MovingWindow m_soilTempHistory;

      // state variables
      float m_snowCover;

      // settings
      VSMB_PET_METHOD m_petMethod; ///MOVE???

      // static variables
      int m_idu;
      ClimateStation *m_pClimateStation;
      PtrArray<SoilLayerInfo> m_soilLayerArray;
      FDataObj *m_pResults;
      FDataObj *m_pRootCoefficientTable;

      
      bool UpdateSoilMoisture(int year, int doy, ClimateStation* pStation);
      int WriteSoilResults(LPCTSTR name);
   protected:
      bool DeterminePrecipitationType(int year, int doy, ClimateStation* pStation);   // sets m_rain, m_snow based on m_precip

      bool DetermineSnowEquivalent(float snowCoef);

      //bool DetermineDayAverageTemp();               // obsolete - just call climate station
      //bool CalculatePhenology(int currentDate);     // Calculate the proper Phenology - not implemented in Python code
      
      bool DetermineAdjustedPET(int year, int doy, ClimateStation* pStation);    // determine adjustedPET(adjustment based on snow cover)
      bool DetermineSoilSurfaceTemp(int year, int doy);   //  determine soil surface temperature
      bool DetermineThreeDaySoilSurfaceAverage();         //  determine 3 day running average for soil temperature
      bool DetermineSnowBudget();                     //  determine snow budget
      bool DeterminePotentialDailyMelt(int doy, float tAvg);
      bool DetermineRetentionAndLoss();
      bool DetermineInfiltrationAndRunoff();
      bool DetermineSnowDepletion();
      bool DetermineET();
      bool ApplyPrecipitationAndEvaporationToLayers(); //  determine potential daily melt from snow
      bool CalculateDrainageAndSoilWaterDistribution();        //  calculate drainage and soil water distribution
      bool AccountForMoistureRedistributionOrUnsaturatedFlow();

      float GetMcKayValue(int iDayOfYear, float dMeanTemp);
      float  get_z_table1(int val);
      float  get_z_table2(int val);
     // float GetTempIndex(float dTemperature);

      // new
      int GetLayerCount() { return (int)m_soilLayerArray.GetSize(); }
      float GetTotalSoilMoistContent();
      int OutputDayVSMBResults(int doy);

   };


class VSMBModel
   {
   public:
      VSMBModel(void);
      ~VSMBModel(void);
      bool LoadParamFile(LPCTSTR paramFile);
      bool AllocateSoilArray(int size);
      SoilInfo* SetSoilInfo(int idu, LPCTSTR soilCode, ClimateStation *pStation, bool saveResults=false);

      bool UpdateSoilMoisture(int idu, ClimateStation *pStation, int year, int doy);

      SoilInfo *GetSoilInfo(int idu);///?????

      //  Wrting output CSV
      int OutputDayVSMBResults(int currentDate) { return -1; }//  calculate stress index = 1 - AET / PET
      float GetSoilMoisture(int idu, int layer);
      float GetSWE(int idu);
      bool WriteResults(int idu, LPCTSTR name);

      //kbv
      static int m_kntrol;//VSMBModel::m_kntrol.  Initialize in the cpp file.
      static int m_dcurve2;
      static float  m_drs2;
      static int m_iTotalStages;
      static int m_iKAdjustStage;
      static int m_iYearlyStages;

   public:
      PtrArray<SoilInfo> m_soilInfoArray;    // loaded from CSV file during LoadXml

      PtrArray<SoilLayerParams> m_soilLayerParams;
      static FDataObj* m_pNAISSnowMeltTable;//kbv
      // lookup map fpr soil param - key=soilID, value=array of ptrs to associated SoilLayerParams
      std::map < int, std::vector<SoilLayerParams*> > m_soilLayerParamsMap;
   };

