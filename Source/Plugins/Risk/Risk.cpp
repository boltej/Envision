// Risk.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Risk.h"

#include <Maplayer.h>
#include <QueryEngine.h>
#include <tixml.h>


bool Risk::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);
   
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   this->CheckCol(pIDULayer, this->m_colArea, "AREA", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(pIDULayer, this->m_colHazard, "PFLAMELEN", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(pIDULayer, this->m_colImpact, "PopDens", TYPE_FLOAT, CC_MUST_EXIST);

   this->CheckCol(pIDULayer, this->m_colRisk, "FireRisk", TYPE_FLOAT, CC_AUTOADD);

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
      area += area;
      }

   totalRisk /= totalArea;

   float lowerBound = 0.000015f;
   float upperBound = 0.000035f;

   pEnvContext->rawScore = totalRisk;
   pEnvContext->score = (totalRisk - lowerBound) / (upperBound / lowerBound);  // 0-1
   pEnvContext->score *= 6;   // [0,6]
   pEnvContext->score -= 3;   // [-3,3]

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

/*   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   MapLayer* pMapLayer = (MapLayer*)pEnvContext->pMapLayer;

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // <acute_hazards>
   CString codePath;

   TiXmlElement* pXmlHazard = pXmlRoot->FirstChildElement("hazard");
   while (pXmlHazard != NULL)
      {
      CString name, hazField, acmPctileField, hazPctileField, viField, query;
      XML_ATTR attrs[] = {
         // attr                type            address  isReq checkCol
         { "name",              TYPE_CSTRING,   &name,           true,  0 },
         { "hazard_col",        TYPE_CSTRING,   &hazField,       true,  CC_MUST_EXIST },
         { "acm_pctile_col",    TYPE_CSTRING,   &acmPctileField, false, CC_AUTOADD },
         { "hazard_pctile_col", TYPE_CSTRING,   &hazPctileField, false, CC_AUTOADD },
         { "vi_col",            TYPE_CSTRING,   &viField,        true,  CC_MUST_EXIST },
         { "query",             TYPE_CSTRING,   &query,          true,  0 },
         { NULL,                TYPE_NULL,      NULL,            false, 0 }
         };
      
      ok = TiXmlGetAttributes(pXmlHazard, attrs, filename, pMapLayer);
      if (ok)
         {
         Hazard* pHazard = new Hazard;
         this->m_hazardArray.Add(pHazard);

         pHazard->m_name = name;
         pHazard->m_hazardField = hazField;  
         pHazard->m_acmPctileField = acmPctileField;
         pHazard->m_hazPctileField = hazPctileField;
         pHazard->m_vulnIndexField = viField;
         pHazard->m_query = query;

         pHazard->m_pQuery = pEnvContext->pQueryEngine->ParseQuery(query, 0, pHazard->m_name);

         CheckCol(pEnvContext->pMapLayer, pHazard->m_colHazard, hazField, TYPE_FLOAT, CC_MUST_EXIST);

         if ( ! acmPctileField.IsEmpty() )
            CheckCol(pEnvContext->pMapLayer, pHazard->m_colAcmPctile, acmPctileField, TYPE_FLOAT, CC_MUST_EXIST);

         if (! hazPctileField.IsEmpty())
            CheckCol(pMapLayer, pHazard->m_colHazPctile, hazPctileField, TYPE_FLOAT, CC_MUST_EXIST);

         CheckCol(pMapLayer, pHazard->m_colVulnIndex, viField, TYPE_FLOAT, CC_MUST_EXIST);

         TiXmlElement* pXmlAC = pXmlHazard->FirstChildElement("adapt_cap_metric");
         while (pXmlAC != NULL)
            {
            CString name, field;
            XML_ATTR attrs[] = {
               // attr     type            address     isReq checkCol
               { "name",   TYPE_CSTRING,   &name,      false,  0 },
               { "col",    TYPE_CSTRING,   &field,     true,  CC_MUST_EXIST },
               { NULL,     TYPE_NULL,      NULL,       false, 0 }
               };

            ok = TiXmlGetAttributes(pXmlAC, attrs, filename, pMapLayer);
            if (ok)
               {
               pHazard->m_adaptCapFields.Add(field);
               int col = -1;
               CheckCol(pMapLayer, col, field, TYPE_FLOAT, CC_MUST_EXIST);
               }

            pXmlAC = pXmlAC->NextSiblingElement("adapt_cap_metric");
            }
         }

      pXmlHazard = pXmlHazard->NextSiblingElement("hazard");
      }
      */
   
   return true;
   }





