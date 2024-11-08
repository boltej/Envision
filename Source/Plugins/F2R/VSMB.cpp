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
#include "StdAfx.h"

#pragma hdrstop

#include "VSMB.h"
#include "FarmModel.h"
#include <Maplayer.h>

int VSMBModel::m_kntrol = 7;
int VSMBModel::m_dcurve2 = 80;
float VSMBModel::m_drs2 = 0.8f;
int VSMBModel::m_iTotalStages = 5;
int VSMBModel::m_iKAdjustStage = 3;
int VSMBModel::m_iYearlyStages = 5;
FDataObj* VSMBModel::m_pNAISSnowMeltTable=NULL;
PtrArray<FDataObj> VSMBModel::m_pOutputObjArray = NULL;

VSMBModel::VSMBModel(void)

{

}


VSMBModel::~VSMBModel(void)
   {
   if (m_pNAISSnowMeltTable != NULL)
      delete m_pNAISSnowMeltTable;
   }

bool VSMBModel::UpdateSoilMoisture(int idu, ClimateStation *pStation, int year, int doy, int dayOfSimulation, int currentStage,  FDataObj* pRoot)
   {
   SoilInfo *pSoilInfo = GetSoilInfo(idu);//this is already in the IDU layer

   if (pSoilInfo == NULL)
      return false;

   return pSoilInfo->UpdateSoilMoisture(year, doy, pStation, dayOfSimulation, currentStage, pRoot);
   }

float VSMBModel::GetSoilMoisture(int idu, int layer)
   {
   SoilInfo* pSoilInfo = GetSoilInfo(idu);

   if (pSoilInfo == NULL)
      return 0.0;
   SoilLayerInfo* pLayer = pSoilInfo->m_soilLayerArray[layer];
   return pLayer->m_soilMoistContent;




   }

float VSMBModel::GetSWE(int idu)
{
   SoilInfo* pSoilInfo = GetSoilInfo(idu);

   if (pSoilInfo == NULL)
      return 0.0;

   return pSoilInfo->m_snowCover;
}

float VSMBModel::GetPercentAWC(int idu)
{
   SoilInfo* pSoilInfo = GetSoilInfo(idu);

   if (pSoilInfo == NULL)
      return 0.0;

   return pSoilInfo->m_percentAWC;
}

float VSMBModel::GetAverageSWC(int idu)
{
   SoilInfo* pSoilInfo = GetSoilInfo(idu);

   if (pSoilInfo == NULL)
      return 0.0;

   return pSoilInfo->m_avSWC;
}

float VSMBModel::GetPercentSaturated(int idu)
{
   SoilInfo* pSoilInfo = GetSoilInfo(idu);

   if (pSoilInfo == NULL)
      return 0.0;

   return pSoilInfo->m_percentSat;
}

bool VSMBModel::LoadParamFile(LPCTSTR paramFile)
   {
   VDataObj paramData(U_UNDEFINED);
   int rows = paramData.ReadAscii(paramFile, ',', FALSE);

   // get column indexes from the CSV file
   int colIduID     = paramData.GetCol("FID_VSMB");// soil ID  kbv - should be IDU_ID
   int colZoneThick = paramData.GetCol("THICKNESS");    // zone thickness (mm)
   int colPorosity  = paramData.GetCol("POROSITY"); // soil porosity (%)
   int colFieldCap  = paramData.GetCol("FIELDCAP"); // field capacity (%)
   int colPermWilt  = paramData.GetCol("WILTINGPT");     // wilting point (%)

   // add layers
   for (int i = 0; i < rows; i++)
      {
      SoilLayerParams *pParams = new SoilLayerParams;
      pParams->m_iduID = paramData.GetAsInt(colIduID, i);
      pParams->m_zoneThick = paramData.GetAsFloat(colZoneThick, i);               //  zone depth
      //pParams->m_AWHC = (((paramData.GetAsFloat(colPorosity, i)) - (paramData.GetAsFloat(colPermWilt, i))) / 100.0f)* pParams->m_zoneThick;//length
      pParams->m_permWilt = ((paramData.GetAsFloat(colPermWilt, i)) / 100.0f)*pParams->m_zoneThick;//length
      pParams->m_sat = ((paramData.GetAsFloat(colPorosity, i)) / 100.0f) ;     //  zone saturation (ratio)
     // pParams->m_DUL = (pParams->m_AWHC + pParams->m_permWilt)/ pParams->m_zoneThick;     //  drained upper limit (ratio)
      pParams->m_DUL = ((paramData.GetAsFloat(colFieldCap, i)) / 100.0f) ;     //  drained upper limit (ratio)
     pParams->m_PLL = pParams->m_permWilt / pParams->m_zoneThick;     //  plant lower limit (ratio)
     pParams->m_AWHC = (pParams->m_DUL - pParams->m_PLL) * pParams->m_zoneThick;
     
      // add to params list
      m_soilLayerParams.Add(pParams);

      // and to lookup map
      try
         {
         std::vector<SoilLayerParams*> &v = m_soilLayerParamsMap.at(pParams->m_iduID);
         v.push_back(pParams);
         }
      catch (std::exception &)   // no entry for this soil code yet, so add one
         {
         std::vector<SoilLayerParams*> v;
         v.push_back(pParams);
         m_soilLayerParamsMap.insert(std::make_pair(pParams->m_iduID, v));
         }

      // add a map entry for this soil code to enable fact lookup later
      //m_soilLayerParamsMap.insert(make_pair(std::string(pParams->m_soilCode), pParams));
      }
   //kbv
   m_pNAISSnowMeltTable = new FDataObj;
   m_pNAISSnowMeltTable->ReadAscii("NAISSnowMeltTable.csv", ',');
      

   return true;
   }

bool VSMBModel::WriteResults(int idu, LPCTSTR name)
   {
   SoilInfo* pSoilInfo = GetSoilInfo(idu);//this is already in the IDU layer

   if (pSoilInfo == NULL)
      return false;
   if (pSoilInfo->m_pResults==NULL)
      return false;

   pSoilInfo->WriteSoilResults(name);
   return true;
   }
bool VSMBModel::AllocateSoilArray(int size)
   {
   ASSERT(m_soilInfoArray.GetSize() == 0);
   m_soilInfoArray.SetSize(size);


   return true;
   }


SoilInfo* VSMBModel::SetSoilInfo(int idu, LPCTSTR soilCode, ClimateStation *pStation, bool saveResults)
   {
   // make a new soil info
   SoilInfo *pSoilInfo = new SoilInfo(idu, pStation);

   pSoilInfo->m_pClimateStation = pStation;

   try
      {
      // add soil layer info
      std::vector<SoilLayerParams*> &v = m_soilLayerParamsMap.at(idu);

      for (int i = 0; i < v.size(); i++)
         {
         SoilLayerParams *pParams = v[i];
         pSoilInfo->m_soilLayerArray.Add(new SoilLayerInfo(pParams));
         }

      // and add this to the soilInfoArray
      m_soilInfoArray[idu] = pSoilInfo;

      if (saveResults)
         { 
         ASSERT(pSoilInfo->m_pResults == NULL);
         pSoilInfo->m_pResults = new FDataObj(11, 0);

         pSoilInfo->m_pResults->SetName("VSMB Results");
         pSoilInfo->m_pResults->SetLabel(0, "Time");
         pSoilInfo->m_pResults->SetLabel(1, "Temp Max");
         pSoilInfo->m_pResults->SetLabel(2, "Temp Min");
         pSoilInfo->m_pResults->SetLabel(3, "Precip (mm)"); //kbv
         pSoilInfo->m_pResults->SetLabel(4, "GDD"); //kbv
         pSoilInfo->m_pResults->SetLabel(5, "Adjusted PET");
         pSoilInfo->m_pResults->SetLabel(6, "AET");
         pSoilInfo->m_pResults->SetLabel(7, "Stress Index"); //kbv
         pSoilInfo->m_pResults->SetLabel(8, "Moisture Content (mm)"); //kbv
         pSoilInfo->m_pResults->SetLabel(9, "Moisture Content (mm/mm)"); //kbv
         pSoilInfo->m_pResults->SetLabel(10, "Snow Water (mm)"); //kbv
         m_pOutputObjArray.Add(pSoilInfo->m_pResults);

         ASSERT(pSoilInfo->m_pSoilMoistureResults == NULL);
         pSoilInfo->m_pSoilMoistureResults = new FDataObj(5, 0);

         pSoilInfo->m_pSoilMoistureResults->SetName("VSMB Soil Moisture Results");
         pSoilInfo->m_pSoilMoistureResults->SetLabel(0, "Time");
         pSoilInfo->m_pSoilMoistureResults->SetLabel(1, "Layer 1 SWC (mm)"); //kbv
         pSoilInfo->m_pSoilMoistureResults->SetLabel(2, "Layer 2 SWC (mm)"); //kbv
         pSoilInfo->m_pSoilMoistureResults->SetLabel(3, "Layer 3 SWC (mm)"); //kbv
         pSoilInfo->m_pSoilMoistureResults->SetLabel(4, "Layer 4 SWC (mm)"); //kbv
         m_pOutputObjArray.Add(pSoilInfo->m_pSoilMoistureResults);
         }

      }
   catch (std::exception &)
      {
      CString msg;
      msg.Format("VSMB: Unable to find soil information for IDU %i", idu);
      Report::ErrorMsg(msg);
      return false;
      }

   return pSoilInfo;
   }

SoilInfo* VSMBModel::GetSoilInfo(int idu)
{
   // make a new soil info
  // SoilInfo* pSoilInfo = new SoilInfo(idu, pStation);

   //try
   //{
   //   // add soil layer info
   //   std::vector<SoilLayerParams*>& v = m_soilLayerParamsMap.at(idu);

   //   for (int i = 0; i < v.size(); i++)
   //   {
   //      SoilLayerParams* pParams = v[i];
   //      pSoilInfo->m_soilLayerArray.Add(new SoilLayerInfo(pParams));
   //   }

   //   // and add this to the soilInfoArray
   //   m_soilInfoArray[idu] = pSoilInfo;
   //}
   //catch (std::exception&)
   //{
   //   CString msg;
   //   msg.Format("VSMB: Unable to find soil information for IDU %i", idu);
   //   Report::ErrorMsg(msg);
   //   return false;
   //}
   SoilInfo* pSoilInfo=m_soilInfoArray.GetAt(idu);
   return pSoilInfo;
}


// TODO:  Verify VSMB model working correctly
bool SoilInfo::UpdateSoilMoisture(int year, int doy, ClimateStation* pStation, int dayOfSimulation, int currentStage, FDataObj* pRoot)
   {
   //
   //  Initial strategy to implement VBMS within the Envision F2R plugin. 2/10/2019
   //
   // Called for each FarmField. Method follows from VBSM python code.
   //
   // Updates soil moisture for each soil layer using sequential integration strategy as
   // used in VSMB.  A simultaneous integration strategy would be an improvement.
   //
   // Called from FieldArray loop in GrowCrops()
   //
   // NOTE:  Methods called below need to be implemented!!
   //
   //  get weather for the day, calculate it once to save time
   float snowCoef = 1;

   DeterminePrecipitationType(year, doy, pStation);
   //  determine snow equivalent
   DetermineSnowEquivalent(snowCoef);
   // // determine daily average temperature
   // DetermineDayAverageTemp(); - obsolete
   // //Calculate the proper Phenology
   //CalculatePhenology(currentDate) //This is already done with Heat Unit Calculations
   //  determine adjustedPET(adjustment based on snow cover)
   DetermineAdjustedPET(year, doy, pStation);
   //  determine soil surface temperature
   DetermineSoilSurfaceTemp(year, doy);
   //  determine 3 day running average for soil temperature
   DetermineThreeDaySoilSurfaceAverage();
   //  determine snow budget
   DetermineSnowBudget();
   //  determine potential daily melt from snow
   DeterminePotentialDailyMelt(doy, m_tAvg);
   //  determine retention and actual loss to snow pack
   DetermineRetentionAndLoss(); 
   // determine Infiltration and run off
   DetermineInfiltrationAndRunoff();
   // determine snow depletion
   DetermineSnowDepletion();
   // determine ET
   DetermineET(currentStage, pRoot);
   // apply precipitation and evaporation to layers
   ApplyPrecipitationAndEvaporationToLayers();
   // calculate drainage and soil water distribution
   CalculateDrainageAndSoilWaterDistribution();        //  calculate drainage and soil water distribution
   // account for moisture redistribution or unsaturated flow
   AccountForMoistureRedistributionOrUnsaturatedFlow();


   PopulateF2R();
   //  Wrting output CSV
   if (m_pResults != NULL)
      int columns = OutputDayVSMBResults(dayOfSimulation); //  calculate stress index = 1 - AET / PET
   return true;
   }


int SoilInfo::OutputDayVSMBResults(int doy)
   {
   CArray< float, float > data;
   data.Add((float)doy);
   data.Add(m_tMax);
   data.Add(m_tMin);
   data.Add(m_precip);
   data.Add(0.0f);
   data.Add(m_adjustedPET);
   data.Add(m_dailyAET);
   float si=0.0f;
   if (m_adjustedPET > 0.0f)
      si=1-m_dailyAET/m_adjustedPET;//stress index
   data.Add(si);
   float m1=m_soilLayerArray.GetAt(0)->m_soilMoistContent+ m_soilLayerArray.GetAt(1)->m_soilMoistContent+ m_soilLayerArray.GetAt(2)->m_soilMoistContent+ m_soilLayerArray.GetAt(3)->m_soilMoistContent;
   if (m1<0.0f)
      m1=0.0f;
   float m2= m_soilLayerArray.GetAt(0)->m_pLayerParams->m_AWHC + m_soilLayerArray.GetAt(1)->m_pLayerParams->m_AWHC + m_soilLayerArray.GetAt(2)->m_pLayerParams->m_AWHC + m_soilLayerArray.GetAt(3)->m_pLayerParams->m_AWHC;
   float m3 = 0.0f;
   if (m2>0.0f)
      m3=m1/m2;
   data.Add(m1);
   data.Add(m3);
   data.Add(m_snow);
   m_pResults->AppendRow(data);


   CArray< float, float > data1;
   data1.Add((float)doy);

   data1.Add(m_soilLayerArray.GetAt(0)->m_soilMoistContent/ m_soilLayerArray.GetAt(0)->m_pLayerParams->m_AWHC);
   data1.Add(m_soilLayerArray.GetAt(1)->m_soilMoistContent/ m_soilLayerArray.GetAt(1)->m_pLayerParams->m_AWHC);
   data1.Add(m_soilLayerArray.GetAt(2)->m_soilMoistContent/ m_soilLayerArray.GetAt(2)->m_pLayerParams->m_AWHC);
   data1.Add(m_soilLayerArray.GetAt(3)->m_soilMoistContent/ m_soilLayerArray.GetAt(3)->m_pLayerParams->m_AWHC);
   m_pSoilMoistureResults->AppendRow(data1);

   return 1;
   }

int SoilInfo::WriteSoilResults(LPCTSTR name)
   {
   CString na;
   na.Format("%s%s",name,".csv");
   m_pResults->WriteAscii(na);
   na.Format("%s%s", name, "sm.csv");
   m_pSoilMoistureResults->WriteAscii(na);
   return 1;
   }

bool SoilInfo::DeterminePrecipitationType(int year, int doy,  ClimateStation* pStation)
   {
   // determines if precipitation is snow or rain based on daily temperature
   pStation->GetData(doy, year, PRECIP, m_precip);
   pStation->GetData(doy, year, TMIN,  m_tMin);
   pStation->GetData(doy, year, TMEAN, m_tAvg);
   pStation->GetData(doy, year, TMAX,  m_tMax);

   if (m_tMax <= 0.0f)
      {
      m_rain = 0.0;
      m_snow = m_precip ;
      }
   else
      {
      m_rain = m_precip ;
      m_snow = 0.0;
      }

   return true;
   }

bool SoilInfo::DetermineSnowEquivalent(float snowCoef )
   {
   // determine snow water equivalent
   m_snowWatEq = (m_precip - m_rain) * snowCoef;

   if (m_snowWatEq < 0.0f)
      m_snowWatEq = 0.0f;

   return true;
   }


// determine adjusted PET
bool SoilInfo::DetermineAdjustedPET(int year, int doy, ClimateStation* pStation)
   {
   // calculate adjustedPET based on snow equivalent

   m_snowCover += m_snowWatEq;  

   switch (m_petMethod)
      {
      case VPM_BAIER_ROBERTSON:
         m_computedPET = pStation->GetPET( m_petMethod , doy, year); // TODO self.station.get_pe_baier_robertson(currentDay, self.rstop)
         break;

      case VPM_PENMAN_MONTEITH:
         m_computedPET = 0; // TODO self.dComputedPET = self.station.get_pe_penman(currentDay)
         break;
      
      case VPM_PRIESTLEY_TAYLOR:
      default:
         m_computedPET = 0; // TODO self.dComputedPET = self.station.get_pe(currentDay, self.dSnowCover)
      }

   m_adjustedPET = m_computedPET ;

   // For every day, there is P(which is an input from raw data and this the total precipitation for the day).
   // P - PE thus PE is the potential loss every day and the difference between the two is what we call demand.
   // During very hot days(spring and summer) it is likely to have negative values but if it rains a bunch,
   // then the loss will be low and value will be positive.
   // Thus, dPt(P) = Daily precipitation   minus   dComputedPET(PE) = Potential loss every day
   m_ppe = m_precip - m_computedPET;

   return true; 
   }



bool SoilInfo::DetermineSoilSurfaceTemp(int year, int doy)   //  determine soil surface temperature
   {
   // find the soil surface temperature
   float soil_surface_temp = m_tAvg + 0.25f * (m_tMax - m_tMin);

   m_soilTempHistory.AddValue(soil_surface_temp);  // add to the moving average

   return true;
   }


// calculate three day running average for soil temperature
bool SoilInfo::DetermineThreeDaySoilSurfaceAverage()
   {
   //for j in range(self.SOIL_TEMP_DAYS) :
   //   if (self.dSoilTempHistory[j] != 999.9) :
   //      self.dSoilTempSum += self.dSoilTempHistory[j]
   // the above code gets the SUM of the last three temperatures
   // in C++, this is taken care of by the MovingWindow class

   //self.dSoilTempSnowAdjust = 0;

   // is this really needed?
   m_soilTempAvg = m_soilTempHistory.GetAvgValue();  // self.dSoilTempSum / self.FLOAT_SOIL_TEMP_DAYS
   
   float soilTempSnowAdjust = 0;
   if (m_snowCover > 0.0f)
      soilTempSnowAdjust = m_snowCover / (m_snowCover + (float) exp(2.303 - 0.2197 * m_snowCover));

   // TODO:  - figure out soiltemp1,2 rationale
   m_soilTempLast = soilTempSnowAdjust * m_soilTempLast + (1 - soilTempSnowAdjust) * m_soilTempAvg; // move the temp accumulator one day forward
   
   // TODO:  what is the python code below actually doing?
   m_soilTempHistory.AddValue(m_soilTempLast); // self.dSoilTempHistory[self.SOIL_TEMP_DAYS - 1] = self.dSoilTemp1
   return true;
   }


bool SoilInfo::DetermineSnowBudget()
   {
   ///m_deficitTotal = self.station.get_total_awc() - GetTotalSoilMoistContent();  // TODO
   ///
   ///// below was commented out in the oringal python code
   /////##for j in self.range_layers:
   /////##    self.dDeficitTotal -= self.dSoilMoistContent[j]
   ///
   m_packRetainMelt = m_snowCover * 0.15f;
   m_meltDrain = 0.0f;

   return true;
   }


bool SoilInfo::DeterminePotentialDailyMelt(int doy, float tAvg)
   {
   // calculate potential daily melt

   // correct McKay value loaded
   
   m_dailySnowMelt = GetMcKayValue(doy, tAvg); 
   
   if (m_tMax < 0)
      {
      m_dailySnowMelt = 0.0f;
      m_packRetainMelt = 0.0f;
      }
   
   if (m_dailySnowMelt >= m_snowCover)
      {
      m_dailySnowMelt = m_snowCover;
      m_packRetainMelt = 0.0f;
      m_snowCover = 0.0f;
      }
   else
      m_snowCover = m_snowCover - m_dailySnowMelt;

   return true;
   }


bool SoilInfo::DetermineRetentionAndLoss()
   {
   // calculate retention and loss to snow pack
   m_packSnowMelt = m_T2 + m_dailySnowMelt;

   if (m_packSnowMelt < m_packRetainMelt)
      {
      m_packRetainMelt = m_packSnowMelt;
      m_meltDrain = 0.0f;
      }
   else
      m_meltDrain = m_packSnowMelt - m_packRetainMelt;

   m_T2 = m_packRetainMelt;

   if (m_meltDrain <= 0.0)
      m_meltDrain = 0.0;

   return true;
   }


bool SoilInfo::DetermineInfiltrationAndRunoff()
   {
   //  rather large function to determine Infiltration and Run off 
   //m_meltInfil = meltDrain;  //??? not used?
   m_deficitTotal -= m_meltDrain;

   if (m_snowCover <= 0.0f)
      m_snowCover = 0.0;  // modified mcKay above to be examined

   // (13) infiltration rate adjustment due to freezing of soil 
   m_runoff = 0.0f;
   m_runoffFrozen = 0.0f;

   // (14) original runoff equation was replaced by wole using the scs eqution shown below
   m_surfaceWater = m_rain + m_meltDrain;

   if (m_surfaceWater != 0.0f)
      {
      // used the hob - krogman's technique to calculate dRunoff if the soil is frozen    
      // second method for calculating dRunoff in a frozen soil
      if (m_soilTempLast < 0.0f && m_meltDrain > 0.0f)
         m_runoffFrozen = m_surfaceWater * (m_soilLayerArray[0]->m_soilMoistContent / m_soilLayerArray[0]->m_pLayerParams->m_AWHC);  // TODO
      else // # s value from scs equation, transfer to mm scale # curve numbers for dry and wet soilsoil
         {
         float CNPW = 0.0f;  // para.of curve number when soil is wet
         float CNPD = 0.0f;  // para.of curve number when soil is dry
         //float AC2  = 100.0f - 80.0f;// StationDetails.CURVE2;   // TODO
         float DXX  = 0.0f;
         float* WF   = new float[4];

         // calculation of m_WF for each layer # October 5, 1987
         // change the depmax to 45 cm when calculate wx value
         // assume only the top 45 cm water content affects runoff
         // calculate runoff by williams - scs curve no.technique
         // calculation of runoff according to scs curve number

         float totalDepth = 0.0f;

         for (int i = 0; i < GetLayerCount(); i++)
            {
			   WF[i] = 0.0f;
            SoilLayerInfo *pLayer = m_soilLayerArray[i];
            totalDepth += (pLayer->m_pLayerParams->m_zoneThick / 10.0f);
            float WX = 1.016f * (1.0f -(float) exp(-4.16 * totalDepth / 45.0));
            WF[i] = WX - DXX;
            DXX = WX;
            }

         // TODO
         float CURVE2=80.0f; //these should be member variables of the VSMB class
         float AC2=100-CURVE2;
         float wetCurveNum = CURVE2 * (float) exp(0.006729 * AC2);
         float dryCurveNum = max(0.44f * CURVE2, CURVE2 - 20.0f * AC2 / (AC2 + (float) exp(2.533 - 0.0636 * AC2)));
         if (CURVE2 > 96.0f)
            dryCurveNum = CURVE2 * (0.02f * CURVE2 - 1.0f);
         
         if (dryCurveNum >= 100.0f)
            dryCurveNum = 100.0f;
         
         if (wetCurveNum >= 100.0f)
            wetCurveNum = 100.0;

         for (int i = 0; i < GetLayerCount(); i++)
            {
            SoilLayerInfo *pLayer = m_soilLayerArray[i];
            SoilLayerParams *pParams = pLayer->m_pLayerParams;
            
            pLayer->m_SW = (pLayer->m_soilMoistContent + pParams->m_permWilt) / pParams->m_zoneThick;
            
            CNPW += (pLayer->m_SW / pParams->m_DUL) * WF[i];
            CNPD += ((pLayer->m_SW - pParams->m_PLL) / (pParams->m_DUL - pParams->m_PLL)) * WF[i];
            }
         float curveNum = 0.0f;
            
         /// TODO
         if (CNPD >= 1.0f)
            curveNum = CURVE2 + (wetCurveNum - CURVE2) * CNPW;
         else
            curveNum = dryCurveNum + (CURVE2 - dryCurveNum) * CNPD;
            
         if (curveNum == 0.0f)
            curveNum = 0.99f;
            
         float dSMX = 254.0f * (100.0f / curveNum - 1.0f);
            
         /// reduce the retention factor if soil is frozen formula adapted from epic model,
         /// this method was # found inappropriate as such use the hob - krogman's technique shown below. 
         float dPB = m_surfaceWater - 0.2f * dSMX;
            
         if (dPB > 0)
            {
            m_runoff = (dPB*dPB) / (m_surfaceWater + 0.8f * dSMX);
            if (m_runoff < 0.0)
               m_runoff = 0.0f;
            }
      //   }  // end of for
            
         delete[] WF;
            
         } // end of else

      }  // end of: if surfaceWater != 0.0f

   m_dailyAET = 0.0f;

   return true;
   }

bool SoilInfo::DetermineSnowDepletion()
   {
   // calculate snow depletion

   m_snowLoss = 0.0f;

   if (m_snowCover > 10.0) // allow sublimation to go on at maximum PET only if more than 10 mm of snow
      {
      if (m_snowCover > m_adjustedPET)
         {
         m_snowCover -= m_adjustedPET;
         m_snowLoss = m_adjustedPET;
         m_adjustedPET = 0.0f;
         }
      else
         {
         m_adjustedPET -= m_snowCover;
         m_snowLoss = m_snowCover;
         m_snowCover = 0.0f;
         }
      }
   return true;
   }


bool SoilInfo::DetermineET(int currentStage, FDataObj* pRoot)
   {
   // find the ET for each layer
   float actEvap = 0.0f;
   float deltaB = 0.0f;
   float relMoistCont = 0.0f;
   float deltaA = 0.0f;

   //  (16) calculates ET twice, once with rain then with no rain # root coefficients are used here
   for (int l = 0; l < 2; l++)
      {
      for (int i = 0; i < GetLayerCount(); i++)
         {
         SoilLayerInfo *pLayer = m_soilLayerArray[i];
         SoilLayerParams *pParams = pLayer->m_pLayerParams;

         relMoistCont = pLayer->m_soilMoistContent / pParams->m_AWHC;

         if (l == 1)
            deltaA = pLayer->m_content2 / pParams->m_AWHC;

         if (relMoistCont > 1.0)
            relMoistCont = 1.0f;

         if (deltaA > 1.0)
            deltaA = 1.0f;

         //pLayer->m_kCoef = self.get_root_coeff(self.iCurrentCropStage, i);  // TODO
         //Should depend on crop stage.  See RootCoeffients.csv.

         pLayer->m_kCoef = 0.0f;
         if (pRoot && currentStage < pRoot->GetColCount() && i < m_soilLayerArray.GetSize())
            pLayer->m_kCoef = pRoot->Get(currentStage,i);

         int iCurrentCropStage=currentStage;
         int iKAdjustStage = 2 ;//crop stage for k-adjustment start.  Should be member of VSMB
         if ((iCurrentCropStage >= iKAdjustStage - 1) && (i != 0))  // TODO
            {
            int j = 1;
            while (j <= i)
               {
               int k = j - 1;  // adjusting soil moisture coefficient for stress
               SoilLayerInfo *pLayerK = m_soilLayerArray[k];
               SoilLayerParams *pParamsK = pLayerK->m_pLayerParams;
                 
               if (l == 0)
                  pLayer->m_kCoef += pLayer->m_kCoef * pLayerK->m_kCoef
                  * (1.0f - pLayerK->m_soilMoistContent / pParamsK->m_AWHC);
               else
                  pLayer->m_kCoef += pLayer->m_kCoef * pLayerK->m_kCoef
                  * (1.0f - pLayerK->m_content2 / pParamsK->m_AWHC);
         
               j += 1;                 
               }
            int IT = int(floor(relMoistCont * 100.0f) - 1);
         
            if (l == 1)
               IT = int(floor(deltaA * 100.0f) - 1);  // should we be using a floor function here or should we round ? #Dim IT As Integer = CInt(dRelMoistCont * 100.0) - 1 #If(l = 1) Then IT = CInt(m_DeltaA * 100.0) - 1
         
            float work = 0.0f;
            if (IT < 0)
               work = 0.0f;

            else if (iKAdjustStage <= GetLayerCount())  // TODO
               {
               if (i > iKAdjustStage - 1)  // TODO
                  work = get_z_table2(IT);
               else
                  work = get_z_table1(IT);
               }
            else if (iCurrentCropStage == 0)  // TODO
               work = get_z_table2(IT);
            else
               work = get_z_table1(IT);   // TODO 
         
            // water loss
            if (l == 0)
               {
               actEvap = relMoistCont * work * m_adjustedPET * pLayer->m_kCoef;
         
               if (actEvap > pLayer->m_soilMoistContent)
                  actEvap = pLayer->m_soilMoistContent ;
         
               pLayer->m_waterLoss = actEvap;
               m_dailyAET += pLayer->m_waterLoss;
               }
            else
               {
               deltaB = deltaA * work * m_computedPET * pLayer->m_kCoef;
         
               if (deltaB > pLayer->m_content2)
                  deltaB = pLayer->m_content2;
         
               pLayer->m_delta = deltaB;
               //m_deltaSum += pLayer->m_delta; ??? Never used?
               }
         
            }  // end of if ((self.iCurrentCropStage >= StationDetails.KADJUSTSTAGE - 1) && (i != 0))
         }  // end of: for each layer
      }  // end of: forl < 2
   m_dailyAET += m_snowLoss;

   return true;
   }

float SoilInfo::get_z_table1(int val)
   {
   //' Drying Curve Index Parameters (Table 1...Wheat Curve)
   //   ' Adapted from Ralph Wright (7/27/06)
   float dSeedM = 0.27f;
   float dSeedN = 0.9f;
   float dH9 = 0.3f;
   float dR9 = 1.58f;
   float retval = 0.0f;
   if (dSeedM > 0 || dSeedN > 0) 
      { 
      float dN = dSeedN;
      float dM = dSeedM;
      CArray<float,float>dZtable1;
      for (int i = 0; i<100;i++)
         { 
         float dX9 = ((float)i + 1) / 100.0f;
         if (dX9 >= dR9) 
            {
            dM = 0.0;
            dN = 1.0;
            }
         float dRZ = float(pow((dX9 / dR9), dM) * dN / dX9) + ((pow(((dR9 - dX9) / dR9), dN) * dM / dR9));
         float dRZ1 = float(pow((dX9 / dR9), (dM * dN * dH9)) * dRZ);
         dZtable1.Add(dRZ1);


         }
   retval = dZtable1.GetAt(val);
   }
   return retval;
   }

float SoilInfo::get_z_table2(int val)
   {
   // Drying Curve Index Parameters (Table 2...Fallow Curve)
   // Adapted from Ralph Wright (7/27/06)

   float dSeedM = 0.66f;
   float dSeedN = 0.95f;
   float dH9 = 0.3f;
   float dR9 = 1.47f;
   float retval=0.0f;
   if (dSeedM > 0 || dSeedN > 0)
      {
      float dN = dSeedN;
      float dM = dSeedM;
      CArray<float, float>dZtable2;
      for (int i = 0; i < 100; i++)
         {
         float dX9 = ((float)i + 1) / 100.0f;
         if (dX9 >= dR9)
            {
            dM = 0.0f;
            dN = 1.0f;
            }
         float dRZ = float(pow((dX9 / dR9), dM) * dN / dX9) + ((pow(((dR9 - dX9) / dR9), dN) * dM / dR9));
         float dRZ1 = float(pow((dX9 / dR9), (dM * dN * dH9)) * dRZ);
         dZtable2.Add(dRZ1);
         }
      retval = dZtable2.GetAt(val);
      }
   return retval;
   }


bool SoilInfo::ApplyPrecipitationAndEvaporationToLayers()
   {
   // apply precipitation and evaporation to each soil layer
   for (int i = 0; i < GetLayerCount(); i++)
      {
      SoilLayerInfo *pLayer = m_soilLayerArray[i];

      pLayer->m_soilMoistContent -= pLayer->m_waterLoss;//kbv test soil water movement by turning off et
      pLayer->m_content2 -= pLayer->m_delta;//kbv

      if (pLayer->m_content2 < 0.0f)
         pLayer->m_content2 = 0.0f;

      if (pLayer->m_soilMoistContent < 0.0f)
         pLayer->m_soilMoistContent = 0.0f;

      pLayer->m_waterLoss = 0;
      pLayer->m_delta = 0;
      }

   return true;
   }


bool SoilInfo::CalculateDrainageAndSoilWaterDistribution()
   {
   // find drainage and soil water distribution
   m_soilLayerArray[0]->m_flux = m_surfaceWater - (m_runoff + m_runoffFrozen);   // TODO  where should these come from?
   int layers = GetLayerCount() - 1;
   int l = 0;
   
   for (l = 0; l < GetLayerCount(); l++)
      {
      SoilLayerInfo *pLayer = m_soilLayerArray[l];
      SoilLayerParams *pParams = pLayer->m_pLayerParams;
   
      float HOLDw = pParams->m_sat * pParams->m_zoneThick - (pLayer->m_soilMoistContent + pParams->m_permWilt);
      if ((pLayer->m_flux == 0.0f) || (pLayer->m_flux <= HOLDw))
         {
         pLayer->m_soilMoistContent += pLayer->m_flux;
         pLayer->m_drain = ((pLayer->m_soilMoistContent + pParams->m_permWilt) - pParams->m_DUL * pParams->m_zoneThick);  // # pLayer->DUL = constant for (layer, station) # StationDetails.DRS2 = 0.8
   
         if (l == layers)
            pLayer->m_drain = ((pLayer->m_soilMoistContent + pParams->m_permWilt) - VSMBModel::m_drs2 * pParams->m_DUL * pParams->m_zoneThick);  // TODO...done
   
         if (pLayer->m_drain < 0.0f)
            pLayer->m_drain = 0.0f;
   
         pLayer->m_soilMoistContent -= pLayer->m_drain; 
         pLayer->m_flux = pLayer->m_drain;
         }
      else if (pLayer->m_flux > HOLDw)
         {
         pLayer->m_drain = (pParams->m_sat - pParams->m_DUL) * pParams->m_zoneThick;
         pLayer->m_soilMoistContent = pParams->m_sat * pParams->m_zoneThick - pLayer->m_drain - pParams->m_permWilt;
         pLayer->m_flux -= HOLDw + pLayer->m_drain;
         }
   
      if (l < layers)
         m_soilLayerArray[l + 1]->m_flux = pLayer->m_flux;
      }  // end of: for( l < GetLayerCount() )
   
   if (l >= layers)
      l = layers;
   
   this->m_wleach = m_soilLayerArray[l]->m_flux;
   
   for (l = 0; l < GetLayerCount(); l++)
      m_soilLayerArray[l]->m_flux = 0.0f;
   
   return true;
   }



bool SoilInfo::AccountForMoistureRedistributionOrUnsaturatedFlow()
   {
   // This is modification adapted from the ceres model to account for moisture 
   // redistribution or unsaturated flow also determines grow days 
   for (int l = 0; l < GetLayerCount(); l++)
      {
      SoilLayerInfo *pLayer = m_soilLayerArray[l];
      SoilLayerParams *pParams = pLayer->m_pLayerParams;
      pLayer->m_SWX = (pLayer->m_soilMoistContent + pParams->m_permWilt) / pParams->m_zoneThick;
      }

   // if the first layer is less than 50cm deep then begin processing the second layer 
   int startLayer = 0;
   if (m_soilLayerArray[0]->m_pLayerParams->m_zoneThick <= 50.0f)
      startLayer = 1;
   else
      startLayer = 0;

   // only go to the second last layer as ... # layer L is the layer being 
   // processed # layer m is the layer below the layer being processed
   int l = startLayer;
   int layers = GetLayerCount() - 2;

   //while (l <= layers)
   for (l=startLayer; l<=layers;l++)
      {
      SoilLayerInfo   *pLayer1 = m_soilLayerArray[l];
      SoilLayerInfo   *pLayer2 = m_soilLayerArray[l + 1];
      SoilLayerParams *pParams1 = pLayer1->m_pLayerParams;
      SoilLayerParams *pParams2 = pLayer2->m_pLayerParams;

      float moisture1 = pLayer1->m_soilMoistContent / pParams1->m_zoneThick;
      float moisture2 = pLayer2->m_soilMoistContent / pParams2->m_zoneThick;

      float Dbar = 0.88f * (float) exp(35.4 * (moisture1 + moisture2) * 0.5);
      if (Dbar > 100)
         Dbar = 100.0f;

      float flow = 10.0f * float(Dbar) * (moisture2 - moisture1) / ((pParams1->m_zoneThick + pParams2->m_zoneThick) * 0.5f);
      float wFlow = flow * 10.0f / pParams1->m_zoneThick;

      if (flow >= 0)
         {
         if (pLayer1->m_SWX + wFlow > pParams1->m_sat)
            flow = (pParams1->m_sat - pLayer1->m_SWX) * (pParams1->m_zoneThick / 10.0f);
         else
            {
            if (pLayer1->m_SWX + wFlow > pParams1->m_DUL)
               flow = (pParams1->m_DUL - pLayer1->m_SWX) * (pParams1->m_zoneThick / 10.0f);
            }
         }
      else
         {
         if (pLayer2->m_SWX - wFlow > pParams2->m_sat)
            flow = (pLayer2->m_SWX - pParams2->m_sat) * (pParams2->m_zoneThick / 10.0f);
         else
            {
            if (pLayer2->m_SWX - wFlow > pParams2->m_DUL)
               flow = (pLayer2->m_SWX - pParams2->m_DUL) * (pParams2->m_zoneThick / 10.0f);
            }
         }

      pLayer1->m_SWX += flow / (pParams1->m_zoneThick / 10.0f);
      pLayer2->m_SWX -= flow / (pParams2->m_zoneThick / 10.0f);

      pLayer1->m_soilMoistContent = pLayer1->m_SWX * pParams1->m_zoneThick - pParams1->m_permWilt;
      pLayer2->m_soilMoistContent = pLayer2->m_SWX * pParams2->m_zoneThick - pParams2->m_permWilt;

    //  l = l + 1;
      }  // end of while (l <= layers)

   return true;
   }

float SoilInfo::PopulateF2R()
{
   float waterDepth = 0;//total depth of water (mm)
   for (int i = 0; i < GetLayerCount(); i++)
      waterDepth += m_soilLayerArray[i]->m_soilMoistContent;
   float totalDepth=0;
   for (int i = 0; i < GetLayerCount(); i++)
      totalDepth += m_soilLayerArray[i]->m_pLayerParams->m_zoneThick;

   m_avSWC = waterDepth/totalDepth;

   float totalAWHC = 0;
   for (int i = 0; i < GetLayerCount(); i++)
      totalAWHC += m_soilLayerArray[i]->m_pLayerParams->m_AWHC;

   m_percentAWC = waterDepth/totalAWHC*100.0f;

   m_percentSat = m_avSWC*100;

return 0;
}

float SoilInfo::GetTotalSoilMoistContent()
   {
   float smc = 0;
   for (int i = 0; i < GetLayerCount(); i++)
      smc += m_soilLayerArray[i]->m_soilMoistContent;

   return smc;
   }

float SoilInfo::GetMcKayValue(int iDayOfYear , float dMeanTemp )
   {
   float meltRate = 0.0f;
   float d=(float)iDayOfYear;
   if (dMeanTemp > 0.0f && dMeanTemp < 15.5f)
      meltRate = VSMBModel::m_pNAISSnowMeltTable->IGet(d, dMeanTemp, IM_LINEAR);
   return meltRate;
  /* if(iDayOfYear < 0) 
      get value for January 1
      GetMcKayValue = dMcKay(1, GetTempIndex(dMeanTemp));
    else if(iDayOfYear > DAYS) Then
       get value for December 31 (could be leap year)
      GetMcKayValue = dMcKay(DAYS, GetTempIndex(dMeanTemp));
      Else
         GetMcKayValue = dMcKay(iDayOfYear - 1, GetTempIndex(dMeanTemp));
 

      If (GetMcKayValue < 0.0) Then
      GetMcKayValue = 0.0
      End If*/
   }

