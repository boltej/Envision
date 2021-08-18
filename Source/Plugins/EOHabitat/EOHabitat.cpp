// LPJGuess.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#pragma hdrstop

#include "EOHabitat.h"
#include <QueryEngine.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new EOHabitat; }


bool EOHabitat::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   if (initStr[strlen(initStr) - 1] == '0')
      m_enableMaxPatchSize = false;
   else
      m_enableMaxPatchSize = true;


   const MapLayer* pLayer = pEnvContext->pMapLayer;
   ASSERT(pLayer != NULL);

   //this->AddInputVar("Transition Probability Scalar", m_ccPCScalar, "Parameter describing climate change effect on transition probabilities (1=no scaling)");

   EnvExtension::Init(pEnvContext, initStr);

   this->CheckCol(pLayer, m_colArea, _T("Area"), TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(pLayer, m_colLulcA, _T("LULC_A"), TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pLayer, m_colLulcB, _T("LULC_B"), TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pLayer, m_colWShedID, _T("WShedID"), TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pLayer, m_colDStream, _T("D_STREAM"), TYPE_FLOAT, CC_MUST_EXIST);

   m_watershedLookup[EOH_OTTAWA_MISS] = 0;
   m_watershedLookup[EOH_OTTAWA_SOUTH] = 1;
   m_watershedLookup[EOH_RIDEAU] = 2;
   m_watershedLookup[EOH_ST_LAWRENCE] = 3;

   m_totalArea = 0;
   memset(m_watershedAreas,   0, 4 * sizeof(float));
   memset(m_pctWetlandCovers, 0, 4 * sizeof(float));
   memset(m_pctForestCovers,  0, 4 * sizeof(float));
   memset(m_pctImpervious,    0, 4 * sizeof(float));
   memset(m_coreForestSize,   0, 4 * sizeof(float));

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      float area = -1;
      pLayer->GetData(idu, m_colArea, area);
      m_totalArea += area;

      int lulcA = -1;
      pLayer->GetData(idu, m_colLulcA, lulcA);

      // get watershed
      int wshed = -1;
      pLayer->GetData(idu, m_colWShedID, wshed);

      int wsIndex = -1;
      bool ok = m_watershedLookup.Lookup(wshed, wsIndex);

      if (ok)
         m_watershedAreas[wsIndex] += area;
      }

   // add output vars
   this->AddOutputVar("WetlandCover_Miss", m_pctWetlandCovers[0], "");
   this->AddOutputVar("WetlandCover_South", m_pctWetlandCovers[1], "");
   this->AddOutputVar("WetlandCover_Rideau", m_pctWetlandCovers[2], "");
   this->AddOutputVar("WetlandCover_StLawrence", m_pctWetlandCovers[3], "");

   this->AddOutputVar("ForestCover_Miss", m_pctForestCovers[0], "");
   this->AddOutputVar("ForestCover_South", m_pctForestCovers[1], "");
   this->AddOutputVar("ForestCover_Rideau", m_pctForestCovers[2], "");
   this->AddOutputVar("ForestCover_StLawrence", m_pctForestCovers[3], "");

   this->AddOutputVar("StreamCover_Miss", m_pctStreamCovers[0], "");
   this->AddOutputVar("StreamCover_South", m_pctStreamCovers[1], "");
   this->AddOutputVar("StreamCover_Rideau", m_pctStreamCovers[2], "");
   this->AddOutputVar("StreamCover_StLawrence", m_pctStreamCovers[3], "");

   this->AddOutputVar("Impervious_Miss", m_pctImpervious[0], "");
   this->AddOutputVar("Impervious_South", m_pctImpervious[1], "");
   this->AddOutputVar("Impervious_Rideau", m_pctImpervious[2], "");
   this->AddOutputVar("Impervious_StLawrence", m_pctImpervious[3], "");

   this->AddOutputVar("CoreForestSize_Miss", m_coreForestSize[0], "");
   this->AddOutputVar("CoreForestSize_South", m_coreForestSize[1], "");
   this->AddOutputVar("CoreForestSize_Rideau", m_coreForestSize[2], "");
   this->AddOutputVar("CoreForestSize_StLawrence", m_coreForestSize[3], "");

   return true;
   }


bool EOHabitat::InitRun(EnvContext* pEnvContext, bool)
   {
   m_scenario = (EOH_SCENARIO)pEnvContext->scenarioIndex;
   return true;
   }


bool EOHabitat::Run(EnvContext* pEnvContext)
   {
   const MapLayer* pLayer = pEnvContext->pMapLayer;

   for (int i = 0; i < 4; i++)
      {
      m_pctWetlandCovers[i] = 0;
      m_pctForestCovers[i] = 0;
      m_pctImpervious[i] = 0;
      m_coreForestSize[0] = 0;
      }

   // implement any needed environmental regulations

   switch (m_scenario)
      {
      case EOH_COWBOY:  // nothing required
      case EOH_STATUS_QUO:   // scenarios happen elsewhere
      case EOH_GROWTH_REGULATION:   // scenarios happen elsewhere
         return true;

      case EOH_LANDSCAPE_REGULATION:
      case EOH_LANDSCAPE_ORIENTATION:
         {
         // metrics calculated:
         // pct Wetland Cover by watershed
         // pct Impervious by watershed
         // pct Forest Cover by watershed
         // pct of Stream Corridor in natural cover
   
         float streamCorridorArea = 0;
         for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
            {
            // percent Wetland Coverage by watershed
            // percent impervious by watershed
            // percent forested cover
            int lulcA = -1;
            pLayer->GetData(idu, m_colLulcA, lulcA);
   
            int lulcB = -1;
            pLayer->GetData(idu, m_colLulcB, lulcB);
   
            int wshed = -1, wsIndex = -1;
            pLayer->GetData(idu, m_colWShedID, wshed);
            bool ok = m_watershedLookup.Lookup(wshed, wsIndex);
   
            float area = -1;
            pLayer->GetData(idu, m_colArea, area);
   
            if (wsIndex >= 0)
               {
               switch (lulcA)
                  {
                  case 6:  // wetland?
                     m_pctWetlandCovers[wsIndex] += area;
                     break;
                  }

               switch(lulcB)
                  {   
                  case 44: // Built pervious
                     m_pctImpervious[wsIndex] += 0.2f * area;
                     break;
   
                  case 45: // Built impervious
                     m_pctImpervious[wsIndex] += 0.8f * area;
                     break;
   
                  case 34: // Other urban
                  case 42: // Other urban
                     m_pctImpervious[wsIndex] += 0.5f * area;
                     break;

                  case 210:   // Coniferous
                  case 220:   // Broadleaf
                  case 230:   //  Mixedwood
                     m_pctForestCovers[wsIndex] += area;
                     break;
                  }
   
               float dStream = 0;
               pLayer->GetData(idu, m_colDStream, dStream);
               if (dStream < 1)
                  {
                  switch (lulcB)
                     {
                     case 210:   // Coniferous
                     case 220:   // Broadleaf
                     case 230:   //  Mixedwood
                     case 51:    // Swamp
                     case 55:    // Fen
                     case 59:    // Bog
                     case 63:    // Marsh
                     case 80:    // Wetland / Undifferentiated
                     case 110:   // Grassland / Rangeland
                        streamCorridorArea += area;
                        m_pctStreamCovers[wsIndex] += area;  // length instead?
                        break;
                     }
                  }
               }  // end of: if (wsIndex >= 0) 
            }  // end of: for ( each idu )
   
         for (int i = 0; i < 4; i++)
            {
            m_pctWetlandCovers[i] /= m_watershedAreas[i];
            m_pctForestCovers[i] /= m_watershedAreas[i];
            m_pctStreamCovers[i] /= streamCorridorArea;
            m_pctImpervious[i] /= m_watershedAreas[i];
            }
   
         // patch metrics next
   
         // at least one 200-ha core forest habitat
         // iterate though map of watersheds
         if (m_enableMaxPatchSize)
            {
            m_coreForestSize[0] = GetMaxPatchSize(pEnvContext, EOH_OTTAWA_MISS);
            m_coreForestSize[1] = GetMaxPatchSize(pEnvContext, EOH_OTTAWA_SOUTH);
            m_coreForestSize[2] = GetMaxPatchSize(pEnvContext, EOH_RIDEAU);
            m_coreForestSize[3] = GetMaxPatchSize(pEnvContext, EOH_ST_LAWRENCE);
            }
   
         // Don't worry about #7,#8 in 1/25 email.
         break;
         }  // end of: case Landscape Scenario
      }

   return true;
   }


float EOHabitat::GetMaxPatchSize(EnvContext* pContext, int wshedID)
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;
   QueryEngine* pQE = pContext->pQueryEngine;
   ASSERT(pQE != NULL);

   char queryStr[128];
   strcpy(queryStr, "WShedID=3 and (LULC_B=210 or LULC_B=220 or LULC_B = 230)");
   Query* pQuery = pQE->ParseQuery(queryStr, 0, "");

   CArray<float, float> patchSizeArray;
   int count = pLayer->GetPatchSizes(pQuery, m_colArea, patchSizeArray);

   float maxPatchSize = 0;
   for (int i = 0; i < count; i++)
      {
      if (patchSizeArray[i] > maxPatchSize)
         maxPatchSize = patchSizeArray[i];
      }

   return maxPatchSize * HA_PER_M2;
   }