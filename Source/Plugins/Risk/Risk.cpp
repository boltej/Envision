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

   m_pOutputData = new FDataObj(24, 0);
   int col = 0;
   m_pOutputData->SetName(this->m_name);
   m_pOutputData->SetLabel(col++, "Time");
   m_pOutputData->SetLabel(col++, "Actual Loss-Housing ($)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Housing ($)");
   m_pOutputData->SetLabel(col++, "Actual Loss-Timber ($)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Timber ($)");

   m_pOutputData->SetLabel(col++, "Actual Loss-Total ($)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Total ($)");

   m_pOutputData->SetLabel(col++, "Actual Expected Loss-Housing (#)");
   m_pOutputData->SetLabel(col++, "Potential Expected Loss-Housing (#)");

   m_pOutputData->SetLabel(col++, "Actual Loss-Houses Impacted (#)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Houses Impacted (#)");
   m_pOutputData->SetLabel(col++, "Actual Loss-Timber Vol (m3)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Timber Vol (m3)");

   m_pOutputData->SetLabel(col++, "Actual Loss Frac-Housing");
   m_pOutputData->SetLabel(col++, "Potential Loss Frac-Housing");
   m_pOutputData->SetLabel(col++, "Actual Loss Frac-Timber");
   m_pOutputData->SetLabel(col++, "Potential Loss Frac-Timber");

   m_pOutputData->SetLabel(col++, "Actual Loss-Housing ($,MovingAvg)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Housing ($,MovingAvg)");
   m_pOutputData->SetLabel(col++, "Actual Loss-Timber ($,MovingAvg)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Timber ($,MovingAvg)");
   m_pOutputData->SetLabel(col++, "Actual Loss-Total ($,MovingAvg)");
   m_pOutputData->SetLabel(col++, "Potential Loss-Total ($,MovingAvg)");


   //m_pOutputData->SetLabel(col++, "Risk-Actual");
   //m_pOutputData->SetLabel(col++, "Risk-Potential");
   m_pOutputData->SetLabel(col++, "Risk-Total");

   this->AddOutputVar(this->m_name, m_pOutputData, "");

   m_movingWindowHousingPotentialLoss.Clear();
   m_movingWindowHousingActualLoss.Clear();
   m_movingWindowTimberPotentialLoss.Clear();
   m_movingWindowTimberActualLoss.Clear();

   int iduCount = pIDULayer->GetRowCount();
   iduMovingWindowsHousingPotentialLoss.SetSize(iduCount);
   iduMovingWindowsHousingActualLoss.SetSize(iduCount);
   iduMovingWindowsTimberPotentialLoss.SetSize(iduCount);
   iduMovingWindowsTimberActualLoss.SetSize(iduCount);

   for (int i = 0; i < iduCount; i++)
      {
      iduMovingWindowsHousingPotentialLoss.SetAt(i, new MovingWindow(5));
      iduMovingWindowsHousingActualLoss.SetAt(i, new MovingWindow(5));
      iduMovingWindowsTimberPotentialLoss.SetAt(i, new MovingWindow(5));
      iduMovingWindowsTimberActualLoss.SetAt(i, new MovingWindow(5));
      }

   return ok;
   }


bool Risk::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   m_movingWindowHousingPotentialLoss.Clear();
   m_movingWindowHousingActualLoss.Clear();
   m_movingWindowTimberPotentialLoss.Clear();
   m_movingWindowTimberActualLoss.Clear();

   int iduCount = pIDULayer->GetRowCount();
   for (int idu = 0; idu < iduCount; idu++)
      {
      iduMovingWindowsHousingPotentialLoss[idu]->Clear();
      iduMovingWindowsHousingActualLoss[idu]->Clear();
      iduMovingWindowsTimberPotentialLoss[idu]->Clear();
      iduMovingWindowsTimberActualLoss[idu]->Clear();
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
   float totalActualExpectedLossHousesCount = 0;
   float totalPotentialLossHousesCount = 0;
   float expectedPotentialLossHousesCount = 0;
   float totalPotentialExpectedLossHousesCount = 0;

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
   float maxPotentialLossPerHa = 0;
   float maxActualLossPerHa = 0;

   int colForAllocZone = pIDULayer->GetFieldCol("ForAllocZo");
   ASSERT(colForAllocZone >= 0);

   vector<float> potLossPerHas;
   vector<float> actLossPerHas;

   float totalArea = pIDULayer->GetTotalArea();

   for (MapLayer::Iterator idu = pIDULayer->Begin(); idu < pIDULayer->End(); idu++)
      {
      float area = 0;
      pIDULayer->GetData(idu, this->m_colArea, area);


      int nDU = 0;
      pIDULayer->GetData(idu, this->m_colNDU, nDU);

      float imprVal = nDU * this->m_medianHomeValue;
      //pIDULayer->GetData(idu, this->m_colImprVal, imprVal);

      float potentialLossHousing = 0, actualLossHousing = 0;
      float potentialLossTimber = 0, actualLossTimber = 0, lossFrac = 0;

      // Housing model
      if (nDU > 0)
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
            totalPotentialExpectedLossHousesCount += expectedPotentialLossHousesCount;
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
            totalActualExpectedLossHousesCount += expectedActualLossHousesCount;  // this is used for risk
            }
         }

      this->UpdateIDU(pEnvContext, idu, this->m_colLossFracHousing, lossFrac, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colLossHousing, actualLossHousing, ADD_DELTA); // actual expected, $
      this->UpdateIDU(pEnvContext, idu, this->m_colPotentialLossHousing, potentialLossHousing, ADD_DELTA);
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

      this->UpdateIDU(pEnvContext, idu, this->m_colLossFracTimber, lossFrac, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colLossTimber, actualLossTimber, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colPotentialLossTimber, potentialLossTimber, ADD_DELTA);


      //?????????? potential risk = potential / maxPotential
      // risk for a given IDU is the potential/actual risk/ha normalized by the maxLossPotentialPerHa

      float potentialLossPerHa = (potentialLossTimber + potentialLossHousing) / (area * HA_PER_M2);
      float actualLossPerHa = (actualLossTimber + actualLossHousing) / (area * HA_PER_M2);

      iduMovingWindowsHousingPotentialLoss[idu]->AddValue(potentialLossHousing);  // $
      iduMovingWindowsHousingActualLoss[idu]->AddValue(actualLossHousing);
      iduMovingWindowsTimberPotentialLoss[idu]->AddValue(potentialLossTimber);  // $
      iduMovingWindowsTimberActualLoss[idu]->AddValue(actualLossTimber);

      float potLossPerHaMovAvg = (iduMovingWindowsHousingPotentialLoss[idu]->GetAvgValue()
                                 + iduMovingWindowsHousingPotentialLoss[idu]->GetAvgValue()) / (area * HA_PER_M2);

      float actLossPerHaMovAvg = (iduMovingWindowsHousingActualLoss[idu]->GetAvgValue() 
                                 + iduMovingWindowsHousingActualLoss[idu]->GetAvgValue()) / (area * HA_PER_M2);

      if (potLossPerHaMovAvg > 0.1f) potLossPerHas.push_back(potLossPerHaMovAvg);
      if (actLossPerHaMovAvg > 0.1f) actLossPerHas.push_back(actLossPerHaMovAvg);

      this->UpdateIDU(pEnvContext, idu, this->m_colPotentialLossPerHa, potLossPerHaMovAvg, ADD_DELTA);
      this->UpdateIDU(pEnvContext, idu, this->m_colActualLossPerHa, actLossPerHaMovAvg, ADD_DELTA);

      float iduMaxTimberLossPotential = this->m_maxTimberLossPotential * area / totalArea;
      float iduMaxHousingLossPotential = this->m_maxHousingLossPotential * area / totalArea;
      float iduMinTimberLossPotential = this->m_minTimberLossPotential * area / totalArea;
      float iduMinHousingLossPotential = this->m_minHousingLossPotential * area / totalArea;

      //float movAvgPotentialLossTimber = iduMovingWindowsTimberPotentialLoss[idu]->GetAvgValue();
      //float movAvgPotentialLossHousing = iduMovingWindowsHousingPotentialLoss[idu]->GetAvgValue();

      //float maPLT = std::min<float>(movAvgPotentialLossTimber,  iduMaxTimberLossPotential);
      //float maPLH = std::min<float>(movAvgPotentialLossHousing, iduMaxHousingLossPotential);
      // [0,1]
      //float iduRisk =
      //      (((maPLT - iduMinTimberLossPotential) / (iduMaxTimberLossPotential - iduMinTimberLossPotential))
      //      + ((maPLH - iduMinHousingLossPotential) / (iduMaxHousingLossPotential - iduMinHousingLossPotential))
      //      ) / 2.0f;

      float PLT = std::min<float>(potentialLossTimber,  iduMaxTimberLossPotential);
      float PLH = std::min<float>(potentialLossHousing, iduMaxHousingLossPotential);
      // [0,1]
      float iduRisk =
            (((PLT - iduMinTimberLossPotential) / (iduMaxTimberLossPotential - iduMinTimberLossPotential))
            + ((PLH - iduMinHousingLossPotential) / (iduMaxHousingLossPotential - iduMinHousingLossPotential))
            ) / 2.0f;

      this->UpdateIDU(pEnvContext, idu, this->m_colRisk, iduRisk, ADD_DELTA);

      if (potentialLossPerHa > maxPotentialLossPerHa)
         maxPotentialLossPerHa = potentialLossPerHa;

      if (actualLossPerHa > maxActualLossPerHa)
         maxActualLossPerHa = actualLossPerHa;
      }  // end of : for each idu

   m_movingWindowHousingPotentialLoss.AddValue(totalPotentialLossHousing);
   m_movingWindowHousingActualLoss.AddValue(totalActualLossHousing);
   m_movingWindowTimberPotentialLoss.AddValue(totalPotentialLossTimber);
   m_movingWindowTimberActualLoss.AddValue(totalActualLossTimber);

   //float riskHousingPotential = m_movingWindowHousingPotentialLoss.GetAvgValue()/this->m_maxHousingLossPotential;
   //float riskHousingActual = m_movingWindowHousingActualLoss.GetAvgValue() /this->m_maxHousingLossActual;
   //float riskTimberPotential = m_movingWindowTimberPotentialLoss.GetAvgValue() / this->m_maxTimberLossPotential;
   //float riskTimberActual = m_movingWindowTimberActualLoss.GetAvgValue() / this->m_maxTimberLossActual;
      
   //float riskPotential = (m_movingWindowPotentialLoss.GetAvgValue() / (totalArea * HA_PER_M2)) / this->m_maxLossPotentialPerHa;
   //float riskActual = (m_movingWindowActualLoss.GetAvgValue() / (totalArea * HA_PER_M2)) / this->m_maxLossActualPerHa;
   //float risk = (this->m_potentialWt * riskPotential) + ((1.0f-this->m_potentialWt) * riskActual);

   float totalActualLosses = totalActualLossHousing + totalActualLossTimber;
   float totalPotentialLosses = totalPotentialLossHousing + totalPotentialLossTimber;

   float movAvgActualLossTimber = m_movingWindowTimberActualLoss.GetAvgValue();
   float movAvgPotentialLossTimber = m_movingWindowTimberPotentialLoss.GetAvgValue();
   float movAvgActualLossHousing = m_movingWindowHousingActualLoss.GetAvgValue();
   float movAvgPotentialLossHousing = m_movingWindowHousingPotentialLoss.GetAvgValue();

   float maPLT = std::min<float>(movAvgPotentialLossTimber, this->m_maxTimberLossPotential);
   float maALT = std::min<float>(movAvgActualLossTimber, this->m_maxTimberLossActual);
   float maPLH = std::min<float>(movAvgPotentialLossHousing, this->m_maxHousingLossPotential);
   float maALH = std::min<float>(movAvgActualLossHousing, this->m_maxHousingLossActual);

   // [0,1]
   float risk =
      ((this->m_potentialWt *
         ((( maPLT - this->m_minTimberLossPotential) / (this->m_maxTimberLossPotential-this->m_minTimberLossPotential))
            + (maPLH - this->m_minHousingLossPotential) / (this->m_maxHousingLossPotential-this->m_minHousingLossPotential)))
      + ((1 - this->m_potentialWt) *
            (((maALT - this->m_minTimberLossActual) / (this->m_maxTimberLossActual - this->m_minTimberLossActual))
            + (maALH - this->m_minHousingLossActual) / (this->m_maxHousingLossActual - this->m_minHousingLossActual)))
      ) / 2.0f;
   // Landscape signal(-3, 3) = (Fire risk * 6) - 3


   CArray<float, float> data;
   data.Add((float)pEnvContext->currentYear);

   data.Add((float)totalActualLossHousing);
   data.Add((float)totalPotentialLossHousing);
   data.Add((float)totalActualLossTimber);
   data.Add((float)totalPotentialLossTimber);
   data.Add(totalActualLosses);
   data.Add(totalPotentialLosses);

   data.Add((float)totalActualExpectedLossHousesCount);
   data.Add((float)totalPotentialExpectedLossHousesCount);

   data.Add((float)totalActualLossHousesCount);
   data.Add((float)totalPotentialLossHousesCount);
   data.Add((float)totalActualLossTimberVol);
   data.Add((float)totalPotentialLossTimberVol);

   data.Add(lossAreaActualHousing > 0 ? (float)totalActualLossFracHousing / lossAreaActualHousing : 0);
   data.Add(lossAreaPotentialHousing > 0 ? (float)totalPotentialLossFracHousing / lossAreaPotentialHousing : 0);
   data.Add(lossAreaActualTimber > 0 ? (float)totalActualLossFracTimber / lossAreaActualTimber : 0);
   data.Add(lossAreaPotentialTimber > 0 ? (float)totalPotentialLossFracTimber / lossAreaPotentialTimber : 0);

   data.Add(movAvgActualLossHousing);
   data.Add(movAvgPotentialLossHousing);
   data.Add(movAvgActualLossTimber);
   data.Add(movAvgPotentialLossTimber);
   data.Add(movAvgActualLossHousing + movAvgActualLossTimber);
   data.Add(movAvgPotentialLossHousing + movAvgPotentialLossTimber);
   
   data.Add(risk);

   this->m_pOutputData->AppendRow(data);

   // add barts warping of risk signal

   // Apply the logarithmic transformation

   float epsilon = 4.54e-5;  // pow(e,-10)
   float lnRisk = (float) log(risk + epsilon);

   // Normalize the log_value to the range [0, 1]
   float lnMin = -10;  // float(log(epsilon));
   float lnMax = 0;
   float normalized_lnRisk = (lnRisk - lnMin) / (lnMax - lnMin);

   // Apply logistic normalization 
   risk = 1 / (1 + exp(-this->m_c * (normalized_lnRisk - this->m_b)));

   pEnvContext->rawScore = risk;   // [0-1]
   pEnvContext->score = risk;  // [0-1]
   pEnvContext->score *= 6;   // [0,6]
   pEnvContext->score -= 3;   // [-3,3]

   this->m_rawScore = pEnvContext->rawScore;
   this->m_score = pEnvContext->score;

   CString msg;
   msg.Format("Evaluator %s: Raw Score=%f, Scaled Score=%f", (LPCTSTR)this->m_name, this->m_rawScore, this->m_score);
   Report::LogInfo(msg);

   msg.Format("Max Loss: Max PotentialLossPerHa=%f, Max ActualPotentialLossPerHa=%f", maxPotentialLossPerHa, maxActualLossPerHa);
   Report::LogInfo(msg);

   if (this->m_colRiskSignal >= 0)
      {
      for (MapLayer::Iterator idu = pIDULayer->Begin(); idu < pIDULayer->End(); idu++)
         this->UpdateIDU(pEnvContext, idu, this->m_colRiskSignal, risk, ADD_DELTA);
      }

   Report::StatusMsg("Calculating Loss Percentiles...");

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
         msg.Format("90th Percentile Potential Loss ($/ha) = %f", potLossPerHas[i]);
         Report::LogInfo(msg);
         break;
         }
      }

   for (int i = 0; i < pctilesAct.size(); i++)
      {
      if (pctilesAct[i] >= 0.9f)
         {
         msg.Format("90th Percentile ActualLoss ($/ha) = %f", actLossPerHas[i]);
         Report::LogInfo(msg);
         break;
         }
      }

   Report::StatusMsg("Calculating Loss Percentiles...Completed");

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

   CString flamelenField, pflamelenField, riskField, riskSignalField, potLossPerHaField, actLossPerHaField, lossFieldHousing, potLossFieldHousing,
      lossFracFieldHousing, lossFieldTimber, potLossFieldTimber, lossFracFieldTimber, lookupFieldHousing, 
      lookupFieldTimber, actExpHousingLoss, potExpHousingLoss;

   XML_ATTR attrs[] = {
      // attr                          type            address  isReq checkCol
      { "flamelen_col",                TYPE_CSTRING,   &flamelenField,  true, 0},
      { "pflamelen_col",               TYPE_CSTRING,   &pflamelenField, true, 0},
      { "risk_col",                    TYPE_CSTRING,   &riskField,      true, 0 },
      { "risk_signal_col",             TYPE_CSTRING,   &riskSignalField, false, 0 },
      { "potential_loss_col",          TYPE_CSTRING,   &potLossPerHaField,   true, 0 },
      { "actual_loss_col",             TYPE_CSTRING,   &actLossPerHaField,   true, 0 },
      { "query",                       TYPE_CSTRING,   &this->m_queryStr, true, 0 },
      { "median_home_value",           TYPE_FLOAT,     &this->m_medianHomeValue, true,  0 },  // totals for the study area

      { "timber_potential_loss_max",   TYPE_FLOAT,     &this->m_maxTimberLossPotential,  true,  0 },  // totals for the study area
      { "timber_actual_loss_max",      TYPE_FLOAT,     &this->m_maxTimberLossActual,     true,  0 },
      { "housing_potential_loss_max",  TYPE_FLOAT,     &this->m_maxHousingLossPotential, true,  0 },
      { "housing_actual_loss_max",     TYPE_FLOAT,     &this->m_maxHousingLossActual,    true,  0 },

      { "timber_potential_loss_min",   TYPE_FLOAT,     &this->m_minTimberLossPotential,  true,  0 },  // totals for the study area
      { "timber_actual_loss_min",      TYPE_FLOAT,     &this->m_minTimberLossActual,     true,  0 },
      { "housing_potential_loss_min",  TYPE_FLOAT,     &this->m_minHousingLossPotential, true,  0 },
      { "housing_actual_loss_min",     TYPE_FLOAT,     &this->m_minHousingLossActual,    true,  0 },

      { "b",                           TYPE_FLOAT,     &this->m_b,    false,  0 },
      { "c",                           TYPE_FLOAT,     &this->m_c,    false,  0 },

      { "firewise_reduction_factor",   TYPE_FLOAT,     &this->m_firewiseReductionFactor, false, 0 },
      { "flamelength_threshold",       TYPE_FLOAT,     &this->m_flamelenThreshold, false, 0 },
      { "potential_weight",            TYPE_FLOAT,     &this->m_potentialWt, false, 0 },

      { "housing_loss_table",          TYPE_CSTRING,   &this->m_lossTableFileHousing, false, 0 },
      { "housing_lookup",              TYPE_CSTRING,   &lookupFieldHousing, true,0 },
      { "housing_loss_col",            TYPE_CSTRING,   &lossFieldHousing,    true, 0 },
      { "housing_potential_loss_col",  TYPE_CSTRING,   &potLossFieldHousing,    true, 0 },
      { "housing_loss_frac_col",       TYPE_CSTRING,   &lossFracFieldHousing,true, 0 },
      { "actual_exp_housing_loss_col", TYPE_CSTRING,   &actExpHousingLoss,true, 0 },
      { "potential_exp_housing_loss_col", TYPE_CSTRING,  &potExpHousingLoss,true, 0 },

      { "timber_loss_table",           TYPE_CSTRING,   &this->m_lossTableFileTimber, false, 0 },
      { "timber_lookup",               TYPE_CSTRING,   &lookupFieldTimber, true, 0 },
      { "timber_loss_col",             TYPE_CSTRING,   &lossFieldTimber,    true, 0 },
      { "timber_potential_loss_col",   TYPE_CSTRING,   &potLossFieldTimber, true, 0 },
      { "timber_loss_frac_col",        TYPE_CSTRING,   &lossFracFieldTimber,true, 0 },
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
      if ( riskSignalField.IsEmpty() == false)
         this->CheckCol(pMapLayer, this->m_colRiskSignal, riskSignalField, TYPE_FLOAT, CC_AUTOADD | TYPE_FLOAT);

      this->CheckCol(pMapLayer, this->m_colActualExpHousingLoss, actExpHousingLoss,  TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colPotentialExpHousingLoss, potExpHousingLoss, TYPE_FLOAT, CC_AUTOADD);

      this->CheckCol(pMapLayer, this->m_colLookupHousing, lookupFieldHousing, TYPE_INT, CC_MUST_EXIST);
      this->CheckCol(pMapLayer, this->m_colLossHousing, lossFieldHousing, TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colPotentialLossHousing, potLossFieldHousing, TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colLossFracHousing, lossFracFieldHousing, TYPE_FLOAT, CC_AUTOADD);

      this->CheckCol(pMapLayer, this->m_colLookupTimber, lookupFieldTimber, TYPE_INT, CC_MUST_EXIST);
      this->CheckCol(pMapLayer, this->m_colLossTimber, lossFieldTimber, TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colPotentialLossTimber, potLossFieldTimber, TYPE_FLOAT, CC_AUTOADD);
      this->CheckCol(pMapLayer, this->m_colLossFracTimber, lossFracFieldTimber, TYPE_FLOAT, CC_AUTOADD);
      
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





