// Risk.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Risk.h"

#include <Maplayer.h>
#include <QueryEngine.h>
#include <Report.h>
#include <tixml.h>
#include <PathManager.h>


bool Risk::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);

   //ASSERT(this->m_colHazard >= 0);
   //ASSERT( this->m_colImpact >= 0);
   //ASSERT( this->m_colArea >= 0);
   ASSERT( this->m_colRisk >= 0);
   
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   return ok;
   }


bool Risk::InitRun(EnvContext *pEnvContext, bool useInitialSeed) 
   {
   return Run(pEnvContext);
   }


bool Risk::Run(EnvContext* pEnvContext)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   
   float totalLoss = 0;
//   float totalArea = 0;
   
   for (MapLayer::Iterator idu = pIDULayer->Begin(); idu < pIDULayer->End(); idu++)
      {
      //float hazard, impact, area;
      //pIDULayer->GetData(idu, this->m_colHazard, hazard);
      //pIDULayer->GetData(idu, this->m_colImpact, impact);
      //pIDULayer->GetData(idu, this->m_colArea, area);
      //
      //float risk = hazard * impact;
      //this->UpdateIDU(pEnvContext, idu, this->m_colRisk, risk, ADD_DELTA);
      float loss = 0;
      
      float pFlameLen = 0;
      pIDULayer->GetData(idu, this->m_colPFlameLen, pFlameLen);

      int nDU = 0;
      pIDULayer->GetData(idu, this->m_colNDU, nDU);

      if (pFlameLen > 0.01 && nDU > 0)
         {
         int lulc = 0;
         pIDULayer->GetData(idu, this->m_colLulc, lulc);

         int col = -1;
         if (pFlameLen < 2)
            col = 2;
         else if (pFlameLen < 4)
            col = 3;
         else if (pFlameLen < 6)
            col = 4;
         else if (pFlameLen < 8)
            col = 5;
         else if (pFlameLen < 12)
            col = 6;
         else
            col = 7;

         int row = this->m_lulcMap[lulc];
         float lossFrac = this->m_pLossTable->GetAsFloat(col, row);
         loss = lossFrac * nDU;

         int firewise = 0;
         pIDULayer->GetData(idu, m_colFirewise, firewise);

         if (firewise > 0)
            loss *= this->m_firewiseFactor;
         }

      this->UpdateIDU(pEnvContext, idu, this->m_colRisk, loss, ADD_DELTA);
      totalLoss += loss;
      //totalArea += area;
      }

   //totalRisk /= totalArea;

   pEnvContext->rawScore = totalLoss;
   pEnvContext->score = (totalLoss - m_lowerBound) / (m_upperBound - m_lowerBound);  // 0-1
   pEnvContext->score *= 6;   // [0,6]
   pEnvContext->score -= 3;   // [-3,3]

   this->m_rawScore = pEnvContext->rawScore;
   this->m_score = pEnvContext->score;

   CString msg;
   msg.Format("Evaluator %s: Raw Score=%f, Scaled Score=%f", (LPCTSTR)this->m_name, this->m_rawScore, this->m_score);
   Report::LogInfo(msg);
   return true;
   }


/*
// Function to calculate the percentile
void Risk::ToPercentiles(float values[], float pctiles[], int n)
   {
   // Start of the loop that calculates percentile
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
*/

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

   CString hazardField, impactField, riskField, lookupTable;
   XML_ATTR attrs[] = {
      // attr                type            address  isReq checkCol
      { "lower_bound",       TYPE_FLOAT,     &this->m_lowerBound,   true,  0 },
      { "upper_bound",       TYPE_FLOAT,     &this->m_upperBound,   true,  0 },
      { "hazard_col",        TYPE_CSTRING,   &hazardField, true, CC_MUST_EXIST },
      { "impact_col",        TYPE_CSTRING,   &impactField, true, CC_MUST_EXIST },
      { "risk_col",          TYPE_CSTRING,   &riskField, true, CC_AUTOADD },
      { "query",             TYPE_CSTRING,   &this->m_queryStr, true, 0 },
      { "loss_table",        TYPE_CSTRING,   &this->m_lossTableFile, false, 0 },
      { "firewise_factor",   TYPE_CSTRING,   &this->m_firewiseFactor, false, 0 },
      { NULL,                TYPE_NULL,      NULL,              false, 0 }
      };

   ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, pMapLayer);
   if (ok)
      {
      this->CheckCol(pMapLayer, this->m_colHazard, hazardField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colImpact, impactField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colRisk, riskField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colArea, "AREA", TYPE_FLOAT, 0);

      this->CheckCol(pMapLayer, this->m_colLulc, "LULC_B", TYPE_INT, 0);
      this->CheckCol(pMapLayer, this->m_colPFlameLen, "PFLAMELEN", TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colNDU, "N_DU", TYPE_INT, 0);
      this->CheckCol(pMapLayer, this->m_colFirewise, "FIREWISE", TYPE_INT, 0);

      if (this->m_lossTableFile.IsEmpty() == false)
         {
         m_pLossTable = new VDataObj;
         CString path;
         PathManager::FindPath(this->m_lossTableFile, path);
         int records = m_pLossTable->ReadAscii(path,',');

         Report::Log_i("Loaded %i loss records", records);

         for (int i = 0; i < records; i++)
            {
            int lulc = this->m_pLossTable->GetAsInt(0,1);
            this->m_lulcMap.SetAt(lulc, i);
            }
         }
      }

   return true;
   }





