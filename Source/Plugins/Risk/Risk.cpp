// Risk.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Risk.h"

#include <Maplayer.h>
#include <QueryEngine.h>
#include <Report.h>
#include <tixml.h>
#include <PathManager.h>

#include <algorithm>

bool Risk::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);

   ASSERT(this->m_colRisk >= 0);

   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   m_pOutputData = new FDataObj(20, 0);
   int col = 0;
   m_pOutputData->SetName(this->m_name);
   m_pOutputData->SetLabel(col++, "Time");
   m_pOutputData->SetLabel(col++, "Actual Loss-Housing ($)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Housing ($)");
   m_pOutputData->SetLabel(col++, "Actual Loss-Timber ($)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Timber ($)");

   m_pOutputData->SetLabel(col++, "Actual Loss-Houses Impacted (#)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Houses Impacted (#)");
   m_pOutputData->SetLabel(col++, "Actual Loss-Timber Vol (m3)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Timber Vol (m3)");

   m_pOutputData->SetLabel(col++, "Actual Loss Frac-Housing");
   m_pOutputData->SetLabel(col++, "Potential Loss Frac-Housing");
   m_pOutputData->SetLabel(col++, "Actual Loss Frac-Timber");
   m_pOutputData->SetLabel(col++, "Potential Loss Frac-Timber");

   m_pOutputData->SetLabel(col++, "Loss-Actual ($,MovingAvg)");
   m_pOutputData->SetLabel(col++, "Loss-Actual ($)");
   m_pOutputData->SetLabel(col++, "Loss-Potential ($,MovingAvg)");
   m_pOutputData->SetLabel(col++, "Loss-Potential ($)");

   m_pOutputData->SetLabel(col++, "Risk-Actual");
   m_pOutputData->SetLabel(col++, "Risk-Potential");
   m_pOutputData->SetLabel(col++, "Risk-Total");

   this->AddOutputVar(this->m_name, m_pOutputData, "");

   m_movingWindowPotentialLoss.Clear();
   m_movingWindowActualLoss.Clear();

   int iduCount = pIDULayer->GetRowCount();
   iduMovingWindowsPotentialLoss.SetSize(iduCount);
   iduMovingWindowsActualLoss.SetSize(iduCount);

   for (int i = 0; i < iduCount; i++)
      {
      iduMovingWindowsPotentialLoss.SetAt(i, new MovingWindow(5));
      iduMovingWindowsActualLoss.SetAt(i, new MovingWindow(5));
      }

   return ok;
   }


bool Risk::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   m_movingWindowPotentialLoss.Clear();
   m_movingWindowActualLoss.Clear();

   int iduCount = pIDULayer->GetRowCount();
   for (int idu = 0; idu < iduCount; idu++)
      {
      iduMovingWindowsPotentialLoss[idu]->Clear();
      iduMovingWindowsActualLoss[idu]->Clear();
      }

   m_pOutputData->ClearRows();
   
   return true; // Run(pEnvContext);
   }


bool Risk::Run(EnvContext* pEnvContext)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   float totalPotentialLossTimber = 0;
   float totalPotentialLossHousing = 0;
   float totalActualLossTimber = 0;
   float totalActualLossHousing = 0;

   float totalActualLossHousesCount = 0;
   float expectedActualLossHousesCount = 0;
   float totalPotentialLossHousesCount = 0;
   float expectedPotentialLossHousesCount = 0;

   float totalPotentialLossTimberVol = 0;
   float totalActualLossTimberVol = 0;

   float totalPotentialLossFracHousing = 0;
   float totalPotentialLossFracTimber = 0;
   float totalActualLossFracHousing = 0;
   float totalActualLossFracTimber = 0;

   float lossAreaActualHousing = 0;
   float lossAreaActualTimber = 0;
   float lossAreaPotentialHousing = 0;
   float lossAreaPotentialTimber = 0;

   /// temp?
   /// temp?
   float totalArea = 0;
   float maxPotentialDamagePerHa = 0;
   float maxActualDamagePerHa = 0;

   int colForAllocZone = pIDULayer->GetFieldCol("ForAllocZo");
   ASSERT(colForAllocZone >= 0);

   vector<float> potLossPerHas;
   vector<float> actLossPerHas;

   for (MapLayer::Iterator idu = pIDULayer->Begin(); idu < pIDULayer->End(); idu++)
      {
      float area = 0;
      pIDULayer->GetData(idu, this->m_colArea, area);
      totalArea += area;

      float imprVal = 0;
      pIDULayer->GetData(idu, this->m_colImprVal, imprVal);

      int nDU = 0;
      pIDULayer->GetData(idu, this->m_colNDU, nDU);

      float potentialLossHousing = 0, actualLossHousing = 0;
      float potentialLossTimber = 0, actualLossTimber = 0, lossFrac = 0;

      // Housing model
      if (imprVal > 0)
         {
         int lu = 0;
         pIDULayer->GetData(idu, this->m_colLookupHousing, lu);

         int firewise = 0;
         pIDULayer->GetData(idu, m_colFirewise, firewise);

         // potential loss
         float pFlameLen = 0;
         pIDULayer->GetData(idu, this->m_colPFlameLen, pFlameLen);
         GetLoss(pIDULayer, pFlameLen, lu, false, firewise, lossFrac );
         if (lossFrac > 0)
            {
            potentialLossHousing = lossFrac * imprVal;
            totalPotentialLossHousing += potentialLossHousing;
            lossAreaPotentialHousing += area;
            totalPotentialLossFracHousing += (lossFrac * area);
            totalPotentialLossHousesCount += nDU;
            expectedPotentialLossHousesCount = nDU * lossFrac;
            }

         // actual loss
         float flameLen = 0;
         pIDULayer->GetData(idu, this->m_colFlameLen, flameLen);
         GetLoss(pIDULayer, flameLen, lu, false, firewise, lossFrac);
         if (lossFrac > 0)
            {
            actualLossHousing = lossFrac * imprVal;
            totalActualLossHousing += actualLossHousing;
            lossAreaActualHousing += area;
            totalActualLossFracHousing += (lossFrac * area);
            totalActualLossHousesCount += nDU;
            expectedActualLossHousesCount = nDU * lossFrac;
            }
         }

      this->UpdateIDU(pEnvContext, idu, this->m_colDamageFracHousing, lossFrac, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colDamageHousing, actualLossHousing, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colPotentialDamageHousing, potentialLossHousing, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colActualExpHousingLoss, expectedActualLossHousesCount, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colPotentialExpHousingLoss, expectedPotentialLossHousesCount, ADD_DELTA);

      // timber model
      float timberVol = 0; // m3/ha
      pIDULayer->GetData(idu, this->m_colTimberVol, timberVol);

      int forAllocZone = -1;
      pIDULayer->GetData(idu, colForAllocZone, forAllocZone);

      if (timberVol > 0 && forAllocZone != 5 && forAllocZone != 7)
         {
         int lu = 0;
         pIDULayer->GetData(idu, this->m_colLookupTimber, lu);

         // potential loss
         float pFlameLen = 0;
         pIDULayer->GetData(idu, this->m_colPFlameLen, pFlameLen);
         GetLoss(pIDULayer, pFlameLen, lu, true, 0, lossFrac);
         if (lossFrac > 0)
            {
            potentialLossTimber = lossFrac * timberVol * HA_PER_M2 * area * TBF_PER_M3 * this->m_timberValueMBF;  // $10/thousand BF
            totalPotentialLossTimber += potentialLossTimber;
            totalPotentialLossTimberVol += lossFrac * timberVol * HA_PER_M2 * area;
            lossAreaPotentialTimber += area;
            totalPotentialLossFracTimber += (lossFrac * area);
            }

         // actual loss
         float flameLen = 0;
         pIDULayer->GetData(idu, this->m_colFlameLen, flameLen);
         GetLoss(pIDULayer, flameLen, lu, true, 0, lossFrac);
         totalActualLossTimber += actualLossTimber;
         if (lossFrac > 0)
            {
            actualLossTimber = lossFrac * timberVol * HA_PER_M2 * area * TBF_PER_M3 * this->m_timberValueMBF;
            totalActualLossTimber += actualLossTimber;
            totalActualLossTimberVol += lossFrac * timberVol * HA_PER_M2 * area;
            lossAreaActualTimber += area;
            totalActualLossFracTimber += (lossFrac * area);
            }
         }

      this->UpdateIDU(pEnvContext, idu, this->m_colDamageFracTimber, lossFrac, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colDamageTimber, actualLossTimber, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colPotentialDamageTimber, potentialLossTimber, ADD_DELTA);


      //?????????? potential risk = potential / maxPotential
      // risk for a given IDU is the potential/actual risk/ha normalized by the maxDamagePotentialPerHa

      float potentialLossPerHa = (potentialLossTimber + potentialLossHousing) / (area * HA_PER_M2);
      float actualLossPerHa = (actualLossTimber + actualLossHousing) / (area * HA_PER_M2);

      iduMovingWindowsPotentialLoss[idu]->AddValue(potentialLossTimber + potentialLossHousing);  // $
      iduMovingWindowsActualLoss[idu]->AddValue(actualLossTimber + actualLossHousing);

      float potLossPerHaMovAvg = (iduMovingWindowsPotentialLoss[idu]->GetAvgValue() / (area * HA_PER_M2));
      float actLossPerHaMovAvg = (iduMovingWindowsActualLoss[idu]->GetAvgValue() / (area * HA_PER_M2));

      if (potLossPerHaMovAvg > 0.1f) potLossPerHas.push_back(potLossPerHaMovAvg);
      if (actLossPerHaMovAvg > 0.1f) actLossPerHas.push_back(actLossPerHaMovAvg);

      this->UpdateIDU(pEnvContext, idu, this->m_colPotentialLossPerHa, potLossPerHaMovAvg, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colActualLossPerHa, actLossPerHaMovAvg, ADD_DELTA);

      float iduRiskPotential = potLossPerHaMovAvg / this->m_maxDamagePotentialPerHa;
      float iduRiskActual = actLossPerHaMovAvg / this->m_maxDamageActualPerHa;
      float iduRisk = this->m_potentialWt * iduRiskPotential + (1.0f-this->m_potentialWt) * iduRiskActual;

      this->UpdateIDU(pEnvContext, idu, this->m_colRisk, iduRisk, ADD_DELTA);

      if (potentialLossPerHa > maxPotentialDamagePerHa)
         maxPotentialDamagePerHa = potentialLossPerHa;

      if (actualLossPerHa > maxActualDamagePerHa)
         maxActualDamagePerHa = actualLossPerHa;


      }  // end of : for each idu

   m_movingWindowPotentialLoss.AddValue(totalPotentialLossTimber + totalPotentialLossHousing);
   m_movingWindowActualLoss.AddValue(totalActualLossTimber + totalActualLossHousing);

   float riskPotential = (m_movingWindowPotentialLoss.GetAvgValue() / (totalArea * HA_PER_M2)) / this->m_maxDamagePotentialPerHa;
   float riskActual = (m_movingWindowActualLoss.GetAvgValue() / (totalArea * HA_PER_M2)) / this->m_maxDamageActualPerHa;
   float risk = (this->m_potentialWt * riskPotential) + ((1.0f-this->m_potentialWt) * riskActual);

   CArray<float, float> data;
   data.Add((float)pEnvContext->currentYear);

   data.Add((float)totalActualLossHousing);
   data.Add((float)totalPotentialLossHousing);

   data.Add((float)totalActualLossTimber);
   data.Add((float)totalPotentialLossTimber);

   data.Add((float)totalActualLossHousesCount);
   data.Add((float)totalPotentialLossHousesCount);
   data.Add((float)totalActualLossTimberVol);
   data.Add((float)totalPotentialLossTimberVol);

   data.Add(lossAreaActualHousing > 0 ? (float)totalActualLossFracHousing / lossAreaActualHousing : 0);
   data.Add(lossAreaPotentialHousing > 0 ? (float)totalPotentialLossFracHousing / lossAreaPotentialHousing : 0);
   data.Add(lossAreaActualTimber > 0 ? (float)totalActualLossFracTimber / lossAreaActualTimber : 0);
   data.Add(lossAreaPotentialTimber > 0 ? (float)totalPotentialLossFracTimber / lossAreaPotentialTimber : 0);

   data.Add(m_movingWindowActualLoss.GetAvgValue());
   data.Add(totalActualLossTimber + totalActualLossHousing);
   data.Add(m_movingWindowPotentialLoss.GetAvgValue());
   data.Add(totalPotentialLossTimber + totalPotentialLossHousing);

   data.Add(riskActual);
   data.Add(riskPotential);
   data.Add(risk);

   this->m_pOutputData->AppendRow(data);

   pEnvContext->rawScore = risk;
   pEnvContext->score = risk;  // [0-1]
   pEnvContext->score *= 6;   // [0,6]
   pEnvContext->score -= 3;   // [-3,3]

   this->m_rawScore = pEnvContext->rawScore;
   this->m_score = pEnvContext->score;

   CString msg;
   msg.Format("Evaluator %s: Raw Score=%f, Scaled Score=%f", (LPCTSTR)this->m_name, this->m_rawScore, this->m_score);
   Report::LogInfo(msg);

   msg.Format("Max Damage: Max PotentialDamagePerHa=%f, Max ActualPotentialDamagePerHa=%f", maxPotentialDamagePerHa, maxActualDamagePerHa);
   Report::LogInfo(msg);

   Report::StatusMsg("Calculating Damage Percentiles...");

   vector<float> pctilesPot;
   vector<float> pctilesAct;
   std::sort(potLossPerHas.begin(), potLossPerHas.end());
   std::sort(actLossPerHas.begin(), actLossPerHas.end());
   ToPercentiles(potLossPerHas, pctilesPot);
   ToPercentiles(actLossPerHas, pctilesAct);


   for (int i = 0; i < pctilesPot.size(); i++)
      {
      if (pctilesPot[i] >= 0.9f)
         {
         msg.Format("90th Percentile Potential Damage ($/ha) = %f", potLossPerHas[i]);
         Report::LogInfo(msg);
         break;
         }
      }

   for (int i = 0; i < pctilesAct.size(); i++)
      {
      if (pctilesAct[i] >= 0.9f)
         {
         msg.Format("90th Percentile ActualDamage ($/ha) = %f", actLossPerHas[i]);
         Report::LogInfo(msg);
         break;
         }
      }

   Report::StatusMsg("Calculating Damage Percentiles...Completed");

   return true;
   }



// Function to calculate the percentile
void Risk::ToPercentiles(std::vector<float>& values, std::vector<float>& pctiles)
   {
   // Start of the loop that calculates percentile
   int n = (int)values.size();
   pctiles.resize(n);

   for (int i = 0; i < n; i++)
      {
      int count = 0;
      for (int j = 0; j < n; j++) {
         // Comparing the value "i" to all others
         // with all other students
         if (values[i] > values[j]) {
            count++;
            }
         }
      pctiles[i] = (count * 100.0f) / (n - 1);
      }
   }




void Risk::GetLoss(MapLayer* pIDULayer, float flameLen, int lu, bool doTimber, int firewise, float& loss)
   {
   loss = 0;

   if (flameLen > this->m_flamelenThreshold && lu > 0)
      {
      int col = -1;
      if (flameLen < 0.7)
         col = 2;
      else if (flameLen < 1.25)
         col = 3;
      else if (flameLen < 2)
         col = 4;
      else if (flameLen < 3)
         col = 5;
      else if (flameLen < 4)
         col = 6;
      else if (flameLen < 5)
         col = 7;
      else if (flameLen < 6)
         col = 8;
      else if (flameLen < 7)
         col = 9;
      else if (flameLen < 8)
         col = 10;
      else if (flameLen < 12)
         col = 11;
      else
         col = 12;

      if (doTimber)
         {
         int row = this->m_luMapTimber[lu];
         loss = this->m_pLossTableTimber->GetAsFloat(col, row);
         }
      else
         {
         int row = this->m_luMapHousing[lu];
         loss = this->m_pLossTableHousing->GetAsFloat(col, row);

         if (firewise > 0)
            loss -= this->m_firewiseReductionFactor;

         if (loss < 0)
            loss = 0;
         }
      }

   return;
   }


bool Risk::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
   {
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   MapLayer* pMapLayer = (MapLayer*)pEnvContext->pMapLayer;

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // <risk>

   CString flamelenField, pflamelenField, riskField, potLossPerHaField, actLossPerHaField, damageFieldHousing, potDamageFieldHousing,
      damageFracFieldHousing, damageFieldTimber, potDamageFieldTimber, damageFracFieldTimber, lookupFieldHousing, 
      lookupFieldTimber, actExpHousingLoss, potExpHousingLoss;

   XML_ATTR attrs[] = {
      // attr                          type            address  isReq checkCol
      { "flamelen_col",                TYPE_CSTRING,   &flamelenField,  true, 0},
      { "pflamelen_col",               TYPE_CSTRING,   &pflamelenField, true, 0},
      { "risk_col",                    TYPE_CSTRING,   &riskField,      true, 0 },
      { "potential_loss_col",          TYPE_CSTRING,   &potLossPerHaField,   true, 0 },
      { "actual_loss_col",             TYPE_CSTRING,   &actLossPerHaField,   true, 0 },
      { "query",                       TYPE_CSTRING,   &this->m_queryStr, true, 0 },
      { "potential_damage_max_per_ha", TYPE_FLOAT,     &this->m_maxDamagePotentialPerHa,   true,  0 },
      { "actual_damage_max_per_ha",    TYPE_FLOAT,     &this->m_maxDamageActualPerHa,   true,  0 },
      { "firewise_reduction_factor",   TYPE_FLOAT,     &this->m_firewiseReductionFactor, false, 0 },
      { "flamelength_threshold",       TYPE_FLOAT,     &this->m_flamelenThreshold, false, 0 },
      { "potential_weight",            TYPE_FLOAT,     &this->m_potentialWt, false, 0 },

      { "housing_loss_table",          TYPE_CSTRING,   &this->m_lossTableFileHousing, false, 0 },
      { "housing_lookup",              TYPE_CSTRING,   &lookupFieldHousing, true,0 },
      { "housing_damage_col",          TYPE_CSTRING,   &damageFieldHousing,    true, 0 },
      { "housing_potential_damage_col",TYPE_CSTRING,   &potDamageFieldHousing,    true, 0 },
      { "housing_damage_frac_col",     TYPE_CSTRING,   &damageFracFieldHousing,true, 0 },
      { "actual_exp_housing_loss_col", TYPE_CSTRING,   &actExpHousingLoss,true, 0 },
      { "potential_exp_housing_loss_col", TYPE_CSTRING,  &potExpHousingLoss,true, 0 },

      { "timber_loss_table",           TYPE_CSTRING,   &this->m_lossTableFileTimber, false, 0 },
      { "timber_lookup",               TYPE_CSTRING,   &lookupFieldTimber, true, 0 },
      { "timber_damage_col",           TYPE_CSTRING,   &damageFieldTimber,    true, 0 },
      { "timber_potential_damage_col", TYPE_CSTRING,   &potDamageFieldTimber, true, 0 },
      { "timber_damage_frac_col",      TYPE_CSTRING,   &damageFracFieldTimber,true, 0 },
      { "timber_value_MBF",            TYPE_FLOAT,     &this->m_timberValueMBF, false, 0 },
      { NULL,                          TYPE_NULL,      NULL,              false, 0 }
      };

   ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, pMapLayer);
   if (ok)
      {
      this->CheckCol(pMapLayer, this->m_colFlameLen, flamelenField, TYPE_FLOAT, CC_MUST_EXIST);
      this->CheckCol(pMapLayer, this->m_colPFlameLen, pflamelenField, TYPE_FLOAT, CC_MUST_EXIST);
      this->CheckCol(pMapLayer, this->m_colPotentialLossPerHa, potLossPerHaField, TYPE_FLOAT, CC_AUTOADD | TYPE_FLOAT);
      this->CheckCol(pMapLayer, this->m_colActualLossPerHa, actLossPerHaField, TYPE_FLOAT, CC_AUTOADD | TYPE_FLOAT);
      this->CheckCol(pMapLayer, this->m_colRisk, riskField, TYPE_FLOAT, CC_AUTOADD | TYPE_FLOAT);

      this->CheckCol(pMapLayer, this->m_colActualExpHousingLoss, actExpHousingLoss,  TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colPotentialExpHousingLoss, potExpHousingLoss, TYPE_FLOAT, CC_AUTOADD);

      this->CheckCol(pMapLayer, this->m_colLookupHousing, lookupFieldHousing, TYPE_INT, CC_MUST_EXIST);
      this->CheckCol(pMapLayer, this->m_colDamageHousing, damageFieldHousing, TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colPotentialDamageHousing, potDamageFieldHousing, TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colDamageFracHousing, damageFracFieldHousing, TYPE_FLOAT, CC_AUTOADD);

      this->CheckCol(pMapLayer, this->m_colLookupTimber, lookupFieldTimber, TYPE_INT, CC_MUST_EXIST);
      this->CheckCol(pMapLayer, this->m_colDamageTimber, damageFieldTimber, TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colPotentialDamageTimber, potDamageFieldTimber, TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colDamageFracTimber, damageFracFieldTimber, TYPE_FLOAT, CC_AUTOADD);
      
      this->CheckCol(pMapLayer, this->m_colNDU, "N_DU", TYPE_INT, 0);
      this->CheckCol(pMapLayer, this->m_colArea, "AREA", TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colImprVal, "RMVIMP", TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colTimberVol, "SAWTIMBVOL", TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colFirewise, "FIREWISE", TYPE_INT, 0);

      if (this->m_lossTableFileHousing.IsEmpty() == false)
         {
         m_pLossTableHousing = new VDataObj;
         CString path;
         PathManager::FindPath(this->m_lossTableFileHousing, path);
         int records = m_pLossTableHousing->ReadAscii(path, ',');

         Report::Log_i("Loaded %i loss records", records);

         for (int i = 0; i < records; i++)
            {
            int lulc = this->m_pLossTableHousing->GetAsInt(0, i);
            this->m_luMapHousing.SetAt(lulc, i);
            }
         }

      if (this->m_lossTableFileTimber.IsEmpty() == false)
         {
         m_pLossTableTimber = new VDataObj;
         CString path;
         PathManager::FindPath(this->m_lossTableFileTimber, path);
         int records = m_pLossTableTimber->ReadAscii(path, ',');

         Report::Log_i("Loaded %i loss records", records);

         for (int i = 0; i < records; i++)
            {
            int lulc = this->m_pLossTableTimber->GetAsInt(0, i);
            this->m_luMapTimber.SetAt(lulc, i);
            }
         }
      }

   return true;
   }





