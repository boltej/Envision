// CVA.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "CVA.h"

#include <Maplayer.h>
#include <QueryEngine.h>
#include <tixml.h>


bool CVA::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);
   return ok;
   }


bool CVA::InitRun(EnvContext *pEnvContext, bool useInitialSeed) 
   { 
   return true; 
   }


bool CVA::Run(EnvContext* pEnvContext)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   for (int i = 0; i < (int)m_hazardArray.GetSize(); i++)
      {
      Hazard* pHazard = m_hazardArray[i];

      pEnvContext->pQueryEngine->SelectQuery(pHazard->m_pQuery, true);
      int iduCount = pIDULayer->GetSelectionCount();
      int acmCount = (int) pHazard->m_colsAdaptCap.GetSize();

      float* values = new float[iduCount];
      float* pctiles = new float[iduCount];
      float* summedValues = new float[iduCount];
      memset(summedValues, 0, iduCount * sizeof(float));
      
      // get percentiles for the IDU values for each adaptive capacity metric
      for ( int j=0; j < acmCount; j++)
         {
         // iterate through selection
         int k = 0;
         for (MapLayer::Iterator idu = pIDULayer->Begin(MapLayer::ML_RECORD_SET::SELECTION); idu < pIDULayer->End(MapLayer::ML_RECORD_SET::SELECTION); idu++)
            pIDULayer->GetData(idu, pHazard->m_colsAdaptCap[j], values[k++]);

         ToPercentiles(values, pctiles, iduCount);

         // accumulate pctiles across acm variables
         for (k = 0; k < iduCount; k++)
            summedValues[k] += pctiles[k];
         }
      
      // get the percentiles of the sums
      ToPercentiles(summedValues, pctiles, iduCount);

      // get the hazard variables
      int k = 0;
      for (MapLayer::Iterator idu = pIDULayer->Begin(MapLayer::ML_RECORD_SET::SELECTION); idu < pIDULayer->End(MapLayer::ML_RECORD_SET::SELECTION); idu++)
         pIDULayer->GetData(idu, pHazard->m_colHazard, values[k++]);

      float* hazPctiles = summedValues;  // reuse this array, since we are done with it
      ToPercentiles(values, hazPctiles, iduCount);

      // have acms, hazard pctiles, compute vulnerability
      for (k = 0; k < iduCount; k++)
         values[k] = (float) sqrt(pctiles[k] * pctiles[k] + hazPctiles[k] * hazPctiles[k]);

      // write results to map
      k = 0;
      for (MapLayer::Iterator idu = pIDULayer->Begin(MapLayer::ML_RECORD_SET::SELECTION); idu < pIDULayer->End(MapLayer::ML_RECORD_SET::SELECTION); idu++)
         {
         if (pHazard->m_colAcmPctile >= 0)
            this->UpdateIDU(pEnvContext, idu, pHazard->m_colAcmPctile, pctiles[k], ADD_DELTA);

         if (pHazard->m_colHazPctile >= 0)
            this->UpdateIDU(pEnvContext, idu, pHazard->m_colHazPctile, hazPctiles[k], ADD_DELTA);

         if (pHazard->m_colVulnIndex >= 0)
            this->UpdateIDU(pEnvContext, idu, pHazard->m_colVulnIndex, values[k], ADD_DELTA);

         ++k;
         }
 
      delete[] values;
      delete[] pctiles;
      delete[] summedValues;
      }  // end of: for each hazard

   return true;
   }


// Function to calculate the percentile
void CVA::ToPercentiles(float values[], float pctiles[], int n)
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


bool CVA::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
   {
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

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
      
      ok = TiXmlGetAttributes(pXmlHazard, attrs, filename, NULL);
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
            CheckCol(pEnvContext->pMapLayer, pHazard->m_colHazPctile, hazPctileField, TYPE_FLOAT, CC_MUST_EXIST);

         CheckCol(pEnvContext->pMapLayer, pHazard->m_colVulnIndex, viField, TYPE_FLOAT, CC_MUST_EXIST);

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

            ok = TiXmlGetAttributes(pXmlAC, attrs, filename, NULL);
            if (ok)
               {
               pHazard->m_adaptCapFields.Add(field);
               int col = -1;
               CheckCol(pEnvContext->pMapLayer, col, field, TYPE_FLOAT, CC_MUST_EXIST);
               }

            pXmlAC = pXmlAC->NextSiblingElement("adapt_cap_metric");
            }
         }

      pXmlHazard = pXmlHazard->NextSiblingElement("hazard");
      }
   
   return true;
   }





