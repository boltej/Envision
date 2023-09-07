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


   //this->AddOutputVar()


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
      int nDU = 0;
      pIDULayer->GetData(idu, this->m_colNDU, nDU);

      if (nDU > 0)
         {
         float pFlameLen = 0;
         pIDULayer->GetData(idu, this->m_colPFlameLen, pFlameLen);

         int lulc = 0;
         pIDULayer->GetData(idu, this->m_colLulc, lulc);

         int firewise = 0;
         pIDULayer->GetData(idu, m_colFirewise, firewise);

         float loss = 0, lossFrac = 0;
         GetLoss(pIDULayer, pFlameLen, lulc, firewise, nDU, loss, lossFrac);
         this->UpdateIDU(pEnvContext, idu, this->m_colRisk, loss, ADD_DELTA);
         totalLoss += loss;


         float flameLen = 0;
         pIDULayer->GetData(idu, this->m_colFlameLen, flameLen);
         GetLoss(pIDULayer, flameLen, lulc, firewise, nDU, loss, lossFrac);
         this->UpdateIDU(pEnvContext, idu, this->m_colDamageFrac, lossFrac, ADD_DELTA);
         this->UpdateIDU(pEnvContext, idu, this->m_colDamage, loss, ADD_DELTA);

         //totalArea += area;
         }
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



void Risk::GetLoss(MapLayer* pIDULayer, float flameLen, int lulc, int firewise, int nDU, float& loss, float& lossFrac)
   {
   loss = 0; lossFrac = 0;

   if (flameLen > 0.01 && nDU > 0)
      {
      int col = -1;
      if (flameLen < 0.7)
         col = 2;
      else if (flameLen < 1.25)
         col = 3;
      else if (flameLen < 2)
         col = 4;
      else if (flameLen < 4)
         col = 5;
      else if (flameLen < 5)
         col = 6;
      else if (flameLen < 6)
         col = 7;
      else if (flameLen < 7)
         col = 8;
      else if (flameLen < 8)
         col = 9;
      else if (flameLen < 12)
         col = 10;
      else
         col = 11;

      int row = this->m_lulcMap[lulc];
      lossFrac = this->m_pLossTable->GetAsFloat(col, row);
      if (firewise > 0)
         lossFrac *= this->m_firewiseFactor;

      loss = lossFrac * nDU;
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

   CString popDensField, flamelenField, pflamelenField, damageField, damageFracField, riskField, lookupTable;
   XML_ATTR attrs[] = {
      // attr                type            address  isReq checkCol
      { "lower_bound",       TYPE_FLOAT,     &this->m_lowerBound,   true,  0 },
      { "upper_bound",       TYPE_FLOAT,     &this->m_upperBound,   true,  0 },
      //{ "popDens_col",       TYPE_CSTRING,   &popDensField,   true, CC_MUST_EXIST },
      { "flamelen_col",      TYPE_CSTRING,   &flamelenField,  true, CC_MUST_EXIST },
      { "pflamelen_col",     TYPE_CSTRING,   &pflamelenField, true, CC_MUST_EXIST },
      { "risk_col",          TYPE_CSTRING,   &riskField,      true, CC_AUTOADD | TYPE_FLOAT },
      { "damage_col",        TYPE_CSTRING,   &damageField,    true, CC_AUTOADD | TYPE_FLOAT },
      { "damage_frac_col",   TYPE_CSTRING,   &damageFracField,true, CC_AUTOADD | TYPE_FLOAT },
      { "query",             TYPE_CSTRING,   &this->m_queryStr, true, 0 },
      { "loss_table",        TYPE_CSTRING,   &this->m_lossTableFile, false, 0 },
      { "firewise_factor",   TYPE_FLOAT,     &this->m_firewiseFactor, false, 0 },
      { NULL,                TYPE_NULL,      NULL,              false, 0 }
      };

   ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, pMapLayer);
   if (ok)
      {
      this->CheckCol(pMapLayer, this->m_colFlameLen, flamelenField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colPFlameLen, pflamelenField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colRisk, riskField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colDamage, damageField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colDamageFrac, damageFracField, TYPE_FLOAT, 0);
      //this->CheckCol(pMapLayer, this->m_colPopDens, popDensField, TYPE_FLOAT, 0);

      this->CheckCol(pMapLayer, this->m_colArea, "AREA", TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colLulc, "LULC_B", TYPE_INT, 0);
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





