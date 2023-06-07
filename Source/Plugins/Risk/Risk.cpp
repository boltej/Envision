// Risk.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Risk.h"

#include <Maplayer.h>
#include <QueryEngine.h>
#include <Report.h>
#include <tixml.h>


bool Risk::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);

   ASSERT(this->m_colHazard >= 0);
   ASSERT( this->m_colImpact >= 0);
   ASSERT( this->m_colArea >= 0);
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
   
   float totalRisk = 0;
   float totalArea = 0;
   
   for (MapLayer::Iterator idu = pIDULayer->Begin(); idu < pIDULayer->End(); idu++)
      {
      float hazard, impact, area;
      pIDULayer->GetData(idu, this->m_colHazard, hazard);
      pIDULayer->GetData(idu, this->m_colImpact, impact);
      pIDULayer->GetData(idu, this->m_colArea, area);

      float risk = hazard * impact;
      this->UpdateIDU(pEnvContext, idu, this->m_colRisk, risk, ADD_DELTA);

      totalRisk += risk * area;
      totalArea += area;
      }

   totalRisk /= totalArea;

   pEnvContext->rawScore = totalRisk;
   pEnvContext->score = (totalRisk - m_lowerBound) / (m_upperBound - m_lowerBound);  // 0-1
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


   CString hazardField, impactField, riskField;
   XML_ATTR attrs[] = {
      // attr                type            address  isReq checkCol
      { "lower_bound",       TYPE_FLOAT,     &this->m_lowerBound,   true,  0 },
      { "upper_bound",       TYPE_FLOAT,     &this->m_upperBound,   true,  0 },
      { "hazard_col",        TYPE_CSTRING,   &hazardField, false, CC_MUST_EXIST },
      { "impact_col",        TYPE_CSTRING,   &impactField, false, CC_MUST_EXIST },
      { "risk_col",          TYPE_CSTRING,   &riskField, false, CC_AUTOADD },
      { NULL,                TYPE_NULL,      NULL,            false, 0 }
      };

   ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, pMapLayer);
   if (ok)
      {
      this->CheckCol(pMapLayer, this->m_colHazard, hazardField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colImpact, impactField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colRisk, riskField, TYPE_FLOAT, 0);
      this->CheckCol(pMapLayer, this->m_colArea, "AREA", TYPE_FLOAT, 0);
      }

   return true;
   }





