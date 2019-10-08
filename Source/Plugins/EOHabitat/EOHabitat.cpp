// LPJGuess.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#pragma hdrstop

#include "EOHabitat.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new EOHabitat; }


bool EOHabitat::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   const MapLayer* pLayer = pEnvContext->pMapLayer;
   ASSERT(pLayer != NULL);
   
   //this->AddInputVar("Transition Probability Scalar", m_ccPCScalar, "Parameter describing climate change effect on transition probabilities (1=no scaling)");

   EnvExtension::Init(pEnvContext, initStr);

   this->CheckCol( pLayer, m_colArea, _T("Area"), TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(pLayer, m_colLulcA, _T("LULC_A"), TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pLayer, m_colLulcB, _T("LULC_B"), TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pLayer, m_colWShedID, _T("WShedID"), TYPE_INT, CC_MUST_EXIST);

   m_watershedLookup[EOH_OTTAWA_MISS] = 0;
   m_watershedLookup[EOH_OTTAWA_SOUTH] = 1;
   m_watershedLookup[EOH_RIDEAU] = 2;
   m_watershedLookup[EOH_ST_LAWRENCE] = 3;
   
   m_totalArea = 0;
   memset(m_watershedAreas,   0, 4 * sizeof(float));
   memset(m_pctWetlandCovers, 0, 4 * sizeof(float));
   memset(m_pctForestCovers, 0, 4 * sizeof(float));
   memset(m_pctImpervious,    0, 4 * sizeof(float));

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

      if ( ok )
         m_watershedAreas[wsIndex] += area;
      }

   return true;
   }


bool EOHabitat::InitRun(EnvContext* pEnvContext, bool)
   {
   m_scenario = (EOH_SCENARIO) pEnvContext->scenarioIndex;

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
      }

   // implement any needed environmental regulations

   switch (m_scenario)
      {
      case EOH_COWBOY:  // nothing required
      case EOH_STATUS_QUO:   // scenarios happen elsewhere
      case EOH_GROWTH_REGULATION:   // scenarios happen elsewhere
         return true;

      case EOH_LANDSCAPE_REGULATION:
         for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
            {
            // percent Wetland Coverage by watershed
            // percent impervious by watershed
            // percent forested cover
            int lulcA = -1;
            pLayer->GetData(idu, m_colLulcA, lulcA);

            int lulcB = -1;
            pLayer->GetData(idu, m_colLulcB, lulcA);

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

                  case 44: // Built pervious
                     m_pctImpervious[wsIndex] += 0.2 * area;
                     break;

                  case 45: // Built impervious
                     m_pctImpervious[wsIndex] += 0.8 * area;
                     break;

                  case 34: // Other urban
                  case 42: // Other urban
                     m_pctImpervious[wsIndex] += 0.5 * area;
                     break;
                  }

               switch (lulcB)
                  {
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
                        m_pctStreamCovers[wsIndex] += area;  // WRONG!!
                        break;
                     }
                  }
               }  // end of: if (wsIndex >= 0) 
            }  // end of: for ( each idu )

         for (int i = 0; i < 4; i++)
            {
            m_pctWetlandCovers[i] /= m_watershedAreas[i];
            m_pctForestCovers[i] /= m_watershedAreas[i];
            m_pctStreamCovers[i] /= m_watershedAreas[i];  // wrong!
            m_pctImpervious[i] /= m_watershedAreas[i];
            }

         // patch metrics next

         // at least one 200-ha core forest habitat
         // iterate though map of watersheds
         for (int i = 0; i < 4; i++)
            {
            // get key from value ????
            ? ? ? ? ?
               m_watershedLookup.
               m_maxForestPatch = GetMaxPatchSize(pLayer, wshedID);
            }


         break;

      case EOH_LANDSCAPE_ORIENTATION:
         break;
      }

   return true;
   }


float EOHabitat::GetMaxPatchSize(MapLayer* pLayer, int wshedID)
   {
   int iduCount = pLayer->GetRecordCount();
   bool* visited = new bool[iduCount];
   memset(visited, 0, iduCount * sizeof(bool));
   float maxPatchSize = 0;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      // have we already visited this idu?  Then continue
      if (visited[idu])
         continue;

      visited[idu] = true;

      int _wshedID = -1;
      pLayer->GetData(idu, m_colWShedID, wshedID);

      if (_wshedID == wshedID)
         {
         int lulcB = -1;
         pLayer->GetData(idu, m_colLulcB, lulcB);

         switch (lulcB)
            {
            case 210:   // Coniferous
            case 220:   // Broadleaf
            case 230:   // Mixedwood
               float size = 0;
               ExpandPatch(pLayer, wshedID, visited, size);
               
               if ( size > maxPatchSize )
                  maxPatchSize = size;
            }
         }
      }

   return maxPatchSize;
   }

float EOHabitat::ExpandPatch(MapLayer* pLayer, int wshedID, bool visited[], float &sizeSoFar)
   {


   float area = -1;
   pLayer->GetData(idu, m_colArea, area);

   if (wsIndex >= 0)
      {


               delete visited;
