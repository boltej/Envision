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

bool VSMBModel::UpdateSoilMoisture(int idu, ClimateStation *pStation, int year, int doy)
   {
   SoilInfo *pSoilInfo = GetSoilInfo(idu);

   if (pSoilInfo == NULL)
      return false;

   return pSoilInfo->UpdateSoilMoisture(year, doy);
   }

bool VSMBModel::LoadParamFile(LPCTSTR paramFile)
   {
   VDataObj paramData(U_UNDEFINED);
   int rows = paramData.ReadAscii(paramFile, ',', FALSE);

   // get column indexes from the CSV file
   int colIduID     = paramData.GetCol("IDU_ID");// soil ID
   int colZoneThick = paramData.GetCol("THICKNESS");    // zone thickness (mm)
   int colPorosity  = paramData.GetCol("POROSITY"); // soil porosity (%)
   int colFieldCap  = paramData.GetCol("FIELDCAP"); // field capacity (%)
   int colPermWilt  = paramData.GetCol("WILTINGPT");     // wilting point (%)
//   int colAWHC      = paramData.GetCol("AHWC");   // zone available water holding capacity (mm)
//   int colDUL       = paramData.GetCol("DUL");    // Drained upper limit (ratio)
//   int colPLL       = paramData.GetCol("PLL");    // lower limit of plant extractable soil water (ratio)
//   int colSat       = paramData.GetCol("SAT");    // zone saturation (ratio)
//   int colWF        = paramData.GetCol("WF");     // weighting factor ??? only used in determineInfiltrationAndRunoff() below

   // add layers
   for (int i = 0; i < rows; i++)
      {
      SoilLayerParams *pParams = new SoilLayerParams;

      pParams->m_iduID     = paramData.GetAsInt( colIduID,  i);
      pParams->m_zoneThick = paramData.GetAsFloat( colZoneThick, i );               //  zone depth
      pParams->m_sat       = (paramData.GetAsFloat( colPorosity , i )) / 100.0;     //  zone saturation (ratio)
      pParams->m_DUL       = (paramData.GetAsFloat( colFieldCap , i )) / 100.0;     //  drained upper limit (ratio)
      pParams->m_PLL       = (paramData.GetAsFloat( colPermWilt , i )) / 100.0;     //  plant lower limit (ratio)
      pParams->m_AWHC = (pParams->m_DUL - pParams->m_PLL) * pParams->m_zoneThick ;  //  zone available water holding capacity (mm)

//    pParams->m_WF is calculated in determineInfiltrationAndRunoff() found below

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
   
   return true;
   }

bool VSMBModel::AllocateSoilArray(int size)
   {
   ASSERT(m_soilInfoArray.GetSize() == 0);
   m_soilInfoArray.SetSize(size);
   return true;
   }


bool VSMBModel::SetSoilInfo(int idu, LPCTSTR soilCode, ClimateStation *pStation)
   {
   // make a new soil info
   SoilInfo *pSoilInfo = new SoilInfo(idu, pStation);

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
      }
   catch (std::exception &)
      {
      CString msg;
      msg.Format("VSMB: Unable to find soil information for IDU %i", idu);
      Report::ErrorMsg(msg);
      return false;
      }
   return true;
   }


// TODO:  Verify VSMB model working correctly
bool SoilInfo::UpdateSoilMoisture(int year, int doy)
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

   DeterminePrecipitationType(year, doy);
   //  determine snow equivalent
   DetermineSnowEquivalent(snowCoef);
   // // determine daily average temperature
   // DetermineDayAverageTemp(); - obsolete
   // //Calculate the proper Phenology
   //CalculatePhenology(currentDate) //This is already done with Heat Unit Calculations
   //  determine adjustedPET(adjustment based on snow cover)
   DetermineAdjustedPET(year, doy);
   //  determine soil surface temperature
   DetermineSoilSurfaceTemp(year, doy);
   //  determine 3 day running average for soil temperature
   DetermineThreeDaySoilSurfaceAverage();
   //  determine snow budget
   DetermineSnowBudget();
   //  determine potential daily melt from snow
   DeterminePotentialDailyMelt();
   //  determine retention and actual loss to snow pack
   DetermineRetentionAndLoss();
   // determine Infiltration and run off
   DetermineInfiltrationAndRunoff();
   // determine snow depletion
   DetermineSnowDepletion();
   // determine ET
   DetermineET();
   // apply precipitation and evaporation to layers
   ApplyPrecipitationAndEvaporationToLayers();
   // calculate drainage and soil water distribution
   CalculateDrainageAndSoilWaterDistribution();        //  calculate drainage and soil water distribution
   // account for moisture redistribution or unsaturated flow
   AccountForMoistureRedistributionOrUnsaturatedFlow();

   //  Wrting output CSV
   //int columns = OutputDayVSMBResults(doy); //  calculate stress index = 1 - AET / PET
   return true;
   }


bool SoilInfo::DeterminePrecipitationType(int year, int doy)
   {
   // determines if precipitation is snow or rain based on daily temperature
   m_pClimateStation->GetData(doy, year, PRECIP, m_precip);
   m_pClimateStation->GetData(doy, year, TMIN,  m_tMin);
   m_pClimateStation->GetData(doy, year, TMEAN, m_tAvg);
   m_pClimateStation->GetData(doy, year, TMAX,  m_tMax);

   if (m_tMax <= 0.0f)
      {
      m_rain = 0.0;
      m_snow = m_precip;
      }
   else
      {
      m_rain = m_precip;
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
bool SoilInfo::DetermineAdjustedPET(int year, int doy)
   {
   // calculate adjustedPET based on snow equivalent

   m_snowCover += m_snowWatEq;  

   switch (m_petMethod)
      {
      case VPM_BAIER_ROBERTSON:
         m_computedPET = m_pClimateStation->GetPET( m_petMethod ); // TODO self.station.get_pe_baier_robertson(currentDay, self.rstop)
         break;

      case VPM_PENMAN_MONTEITH:
         m_computedPET = 0; // TODO self.dComputedPET = self.station.get_pe_penman(currentDay)
         break;
      
      case VPM_PRIESTLEY_TAYLOR:
      default:
         m_computedPET = 0; // TODO self.dComputedPET = self.station.get_pe(currentDay, self.dSnowCover)
      }

   m_adjustedPET = m_computedPET;

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


bool SoilInfo::DeterminePotentialDailyMelt()
   {
   // calculate potential daily melt

   // correct McKay value loaded
   /////m_dailySnowMelt = self.snow_melt.get_mckey_value(self.currentDaysOfYear, m_tdTa); // TODO
   /////
   /////if (m_tMax < 0)
   /////   {
   /////   m_dailySnowMelt = 0.0f;
   /////   m_packRetainMelt = 0.0f;
   /////   }
   /////
   /////if (m_dailySnowMelt >= m_snowCover)
   /////   {
   /////   m_dailySnowMelt = m_snowCover;
   /////   m_packRetainMelt = 0.0f;
   /////   m_snowCover = 0.0f;
   /////   }
   /////else
   /////   m_snowCover = m_snowCover - m_dailySnowMelt;

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
   float runoff = 0.0f;
   float runoffFrozen = 0.0f;

   // (14) original runoff equation was replaced by wole using the scs eqution shown below
   float surfaceWater = m_rain + m_meltDrain;

   if (surfaceWater != 0.0f)
      {
      // used the hob - krogman's technique to calculate dRunoff if the soil is frozen    
      // second method for calculating dRunoff in a frozen soil
      if (m_soilTempLast < 0.0f && m_meltDrain > 0.0f)
         runoffFrozen = surfaceWater * (m_soilLayerArray[0]->m_soilMoistContent / m_soilLayerArray[0]->m_pLayerParams->m_AWHC);  // TODO
      else // # s value from scs equation, transfer to mm scale # curve numbers for dry and wet soilsoil
         {
         float CNPW = 0.0f;  // para.of curve number when soil is wet
         float CNPD = 0.0f;  // para.of curve number when soil is dry
         //////float AC2  = 100.0f - StationDetails.CURVE2;   // TODO
         float DXX  = 0.0f;
         float* WF   = new float[GetLayerCount()];

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
         /////float wetCurveNum = StationDetails.CURVE2 * exp(0.006729 * AC2);
         /////float dryCurveNum = max(0.44f * StationDetails.CURVE2, StationDetails.CURVE2 - 20.0f * AC2 / (AC2 + exp(2.533 - 0.0636 * AC2)));
         /////if (StationDetails.CURVE2 > 96.0f)
         /////   dryCurveNum = StationDetails.CURVE2 * (0.02 * StationDetails.CURVE2 - 1.0);
         /////
         /////if (dryCurveNum >= 100.0f)
         /////   dryCurveNum = 100.0f;
         /////
         /////if (wetCurveNum >= 100.0f)
         /////   wetCurveNum = 100.0;

         for (int i = 0; i < GetLayerCount(); i++)
            {
            ///SoilLayerInfo *pLayer = m_soilLayerArray[i];
            ///SoilLayerParams *pParams = pLayer->m_pLayerParams;
            ///
            ///pLayer->m_soilMoistContent = (pLayer->m_soilMoistContent + pParams->m_permWilt) / pParams->m_zoneThick;
            ///
            ///CNPW += (pLayer->m_SW / pParams->m_DUL) * WF[i];
            ///CNPD += ((pLayer->m_SW - pParams->m_PLL) / (pParams->m_DUL - pParams->m_PLL)) * WF[i];
            ///
            ///float curveNum = 0.0f;
            ///
            ///// TODO
            ///if (CNPD >= 1.0f)
            ///   curveNum = StationDetails.CURVE2 + (wetCurveNum - StationDetails.CURVE2) * CNPW;
            ///else
            ///   curveNum = dryCurveNum + (StationDetails.CURVE2 - dryCurveNum) * CNPD;
            ///
            ///if (curveNum == 0.0f)
            ///   curveNum = 0.99f;
            ///
            ///float dSMX = 254.0f * (100.0f / curveNum - 1.0f);
            ///
            ///// reduce the retention factor if soil is frozen formula adapted from epic model,
            ///// this method was # found inappropriate as such use the hob - krogman's technique shown below. 
            ///float dPB = surfaceWater - 0.2f * dSMX;
            ///
            ///if (dPB > 0)
            ///   {
            ///   runoff = (dPB*dPB) / (surfaceWater + 0.8f * dSMX);
            ///   if (runoff < 0.0)
            ///      runoff = 0.0f;
            ///   }
            }  // end of for
            
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


bool SoilInfo::DetermineET()
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

         //////pLayer->m_kCoef = self.get_root_coeff(self.iCurrentCropStage, i);  // TODO
         //////
         //////if ((self.iCurrentCropStage >= StationDetails.KADJUSTSTAGE - 1) && (i != 0))  // TODO
         //////   {
         //////   int j = 1;
         //////   while (j <= i)
         //////      {
         //////      int k = j - 1;  // adjusting soil moisture coefficient for stress
         //////      SoilLayerInfo *pLayerK = m_soilLayerArray[k];
         //////      SoilLayerParams *pParamsK = pLayerK->m_pLayerParams;
         //////
         //////
         //////      if (l == 0)
         //////         pLayer->m_kCoef += pLayer->m_kCoef * pLayerK->m_kCoef
         //////         * (1.0f - pLayerK->m_soilMoistContent / pParamsK->m_AWHC);
         //////      else
         //////         pLayer->m_kCoef += pLayer->m_kCoef * pLayerK->m_kCoef
         //////         * (1.0f - pLayerK->m_content2 / pParamsK->m_AWHC);
         //////
         //////      j += 1;
         //////      }
         //////
         //////   int IT = int(floor(relMoistCont * 100.0f) - 1);
         //////
         //////   if (l == 1)
         //////      IT = int(floor(deltaA * 100.0f) - 1);  // should we be using a floor function here or should we round ? #Dim IT As Integer = CInt(dRelMoistCont * 100.0) - 1 #If(l = 1) Then IT = CInt(m_DeltaA * 100.0) - 1
         //////
         //////   float work = 0.0f;
         //////   if (IT < 0)
         //////      work = 0.0f;
         //////
         //////   else if (StationDetails.KNTROL <= GetLayerCount())  // TODO
         //////      {
         //////      if (i > StationDetails.KNTROL - 1)  // TODO
         //////         work = self.get_z_table2(IT);
         //////      else
         //////         work = self.get_z_table1(IT);
         //////      }
         //////   else if (self.iCurrentCropStage == 0)  // TODO
         //////      work = self.get_z_table2(IT);
         //////   else
         //////      work = self.get_z_table1(IT);   // TODO 
         //////
         //////   // water loss
         //////   if (l == 0)
         //////      {
         //////      actEvap = relMoistCont * work * m_adjustedPET * pLayer->m_kCoef;
         //////
         //////      if (actEvap > pLayer->m_soilMoistContent)
         //////         actEvap = pLayer->m_soilMoistContent;
         //////
         //////      pLayer->m_waterLoss = actEvap;
         //////      m_dailyAET += pLayer->m_waterLoss;
         //////      }
         //////   else
         //////      {
         //////      deltaB = deltaA * work * m_computedPET * pLayer->m_kCoef;
         //////
         //////      if (deltaB > pLayer->m_content2)
         //////         deltaB = pLayer->m_content2;
         //////
         //////      pLayer->m_delta = deltaB;
         //////      //m_deltaSum += pLayer->m_delta; ??? Never used?
         //////      }
         //////
         //////   }  // end of if ((self.iCurrentCropStage >= StationDetails.KADJUSTSTAGE - 1) && (i != 0))
         }  // end of: for each layer
      }  // end of: forl < 2
   m_dailyAET += m_snowLoss;

   return true;
   }


bool SoilInfo::ApplyPrecipitationAndEvaporationToLayers()
   {
   // apply precipitation and evaporation to each soil layer
   for (int i = 0; i < GetLayerCount(); i++)
      {
      SoilLayerInfo *pLayer = m_soilLayerArray[i];

      pLayer->m_soilMoistContent -= pLayer->m_waterLoss;
      pLayer->m_content2 -= pLayer->m_delta;

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
   //////m_soilLayerArray[0]->m_flux = m_surfaceWater - (m_runoff + m_runoffFrozen);   // TODO  where should these come from?
   //////int layers = GetLayerCount() - 1;
   //////int l = 0;
   //////
   //////for (l = 0; l < GetLayerCount(); l++)
   //////   {
   //////   SoilLayerInfo *pLayer = m_soilLayerArray[l];
   //////   SoilLayerParams *pParams = pLayer->m_pLayerParams;
   //////
   //////   float HOLDw = pParams->m_sat * pParams->m_zoneThick - (pLayer->m_soilMoistContent + pParams->m_permWilt);
   //////   if ((pLayer->m_flux == 0.0f) || (pLayer->m_flux <= HOLDw))
   //////      {
   //////      pLayer->m_soilMoistContent += pLayer->m_flux;
   //////      pLayer->m_drain = ((pLayer->m_soilMoistContent + pParams->m_permWilt) - pParams->m_DUL * pParams->m_zoneThick);  // # pLayer->DUL = constant for (layer, station) # StationDetails.DRS2 = 0.8
   //////
   //////      if (l == layers)
   //////         pLayer->m_drain = ((pLayer->m_soilMoistContent + pParams->m_permWilt) - StationDetails.DRS2 * pParams->m_DUL * pParams->m_zoneThick);  // TODO
   //////
   //////      if (pLayer->m_drain < 0.0f)
   //////         pLayer->m_drain = 0.0f;
   //////
   //////      pLayer->m_soilMoistContent -= pLayer->m_drain;
   //////      pLayer->m_flux = pLayer->m_drain;
   //////      }
   //////   else if (pLayer->m_flux > HOLDw)
   //////      {
   //////      pLayer->m_drain = (pParams->m_sat - pParams->m_DUL) * pParams->m_zoneThick;
   //////      pLayer->m_soilMoistContent = pParams->m_sat * pParams->m_zoneThick - pLayer->m_drain - pParams->m_permWilt;
   //////      pLayer->m_flux -= HOLDw + pLayer->m_drain;
   //////      }
   //////
   //////   if (l < layers)
   //////      m_soilLayerArray[l + 1]->m_flux = pLayer->m_flux;
   //////   }  // end of: for( l < GetLayerCount() )
   //////
   //////if (l >= layers)
   //////   l = layers;
   //////
   //////this->m_wleach = m_soilLayerArray[l]->m_flux;
   //////
   //////for (l = 0; l < GetLayerCount(); l++)
   //////   m_soilLayerArray[l]->m_flux = 0.0f;
   
   return true;
   }



bool SoilInfo::AccountForMoistureRedistributionOrUnsaturatedFlow()
   {
   // This is modification adapted from the ceres model to account for moisture 
   // redistribution or unsaturated flow also determines grow days 
   //////for (int l = 0; l < GetLayerCount(); l++)
   //////   {
   //////   SoilLayerInfo *pLayer = m_soilLayerArray[l];
   //////   SoilLayerParams *pParams = pLayer->m_pLayerParams;
   //////   pLayer->m_SWX = (pLayer->m_soilMoistContent + pParams->m_permWilt) / pParams->m_zoneThick;
   //////   }

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

   while (l <= layers)
      {
      SoilLayerInfo   *pLayer1 = m_soilLayerArray[l];
      SoilLayerInfo   *pLayer2 = m_soilLayerArray[l + 1];
      SoilLayerParams *pParams1 = pLayer1->m_pLayerParams;
      SoilLayerParams *pParams2 = pLayer2->m_pLayerParams;

      float moisture1 = pLayer1->m_soilMoistContent / pParams1->m_zoneThick;
      float moisture2 = pLayer2->m_soilMoistContent / pParams2->m_zoneThick;

      double Dbar = 0.88 * exp(35.4 * (moisture1 + moisture2) * 0.5);
      if (Dbar > 100)
         Dbar = 100.0;

      float flow = 10.0f * float(Dbar) * (moisture2 - moisture1) / ((pParams1->m_zoneThick + pParams2->m_zoneThick) * 0.5f);
      float wFlow = flow * 10.0f / pParams1->m_zoneThick;

      if (flow > 0)
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

      ////pLayer1->m_SWX += flow / (pParams1->m_zoneThick / 10.0f);
      ////pLayer2->m_SWX -= flow / (pParams2->m_zoneThick / 10.0f);
      ////pLayer1->m_soilMoistContent = pLayer1->m_SWX * pParams1->m_zoneThick - pParams1->m_permWilt;
      ////pLayer2->m_soilMoistContent = pLayer2->m_SWX * pParams2->m_zoneThick - pParams2->m_permWilt;
      l = l + 1;
      }  // end of while (l <= layers)

   return true;
   }



float SoilInfo::GetTotalSoilMoistContent()
   {
   float smc = 0;
   for (int i = 0; i < GetLayerCount(); i++)
      smc += m_soilLayerArray[i]->m_soilMoistContent;

   return smc;
   }
