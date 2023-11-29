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
/* FlameLengthUpdater.cpp
**
** See header file for documentation
**
** 2011.03.22 - tjs - starting coding, adapting from
**   code previously residing in FlamMapAP.cpp
*/
#include "stdafx.h"
#pragma hdrstop

#include <MapLayer.h>
#include <EnvExtension.h>
#include "FlameLengthUpdater.h"
#include "FlammapAP.h"
#include "UNITCONV.H"


extern FlamMapAP* gpFlamMapAP;

using namespace std;

int Round(double in)
   {
   double ret = floor(in), ret2 = ceil(in);
   if (in - ret >= 0.5)
      return (int)ret2;
   return (int)ret;
   }


void FlameLengthUpdater::UpdateDeltaArray(
   FlamMapAP* pModel,
   EnvContext* pEnvContext,
   FireYearRunner* pFireYearRunner,
   PolyGridLookups* pPolyGridLkUp
)
   {
   ASSERT(pEnvContext != NULL);
   ASSERT(pFireYearRunner != NULL);
   ASSERT(pPolyGridLkUp != NULL);

   POINT* pGridPtNdxs = NULL;

   float* gridPtProportionsArray = NULL;
   //float //*FlameLengthValues = NULL,
   float totalPolyFlameLength = 0;
   float maxFlameLength = 0;     // ft
   float meanFlameLength = 0;    // ft
   float burnedAreaHa = 0;

   int gridPtCount;
   int maxGridPtCount = 0;
   int polyIndex;
   int gridPt;
   int burnedPolyCount = 0;
   float burnPropSum = 0.0;
   int fireID = 0;

   int colFlamLen = pModel->m_flameLenCol;
   int colArea = pModel->m_areaCol;
   int colFireID = pModel->m_fireIDCol;
   int colDisturb = pEnvContext->pMapLayer->GetFieldCol("DISTURB");
   int colVegClass = pEnvContext->pMapLayer->GetFieldCol("VegClass");
   int colVariant = pEnvContext->pMapLayer->GetFieldCol("Variant");
   int colCTSS = pEnvContext->pMapLayer->GetFieldCol("CTSS");
   int colPFlameLen = pEnvContext->pMapLayer->GetFieldCol("PFlameLen");
   int colFuelModel = pEnvContext->pMapLayer->GetFieldCol("FUELMODEL");
   FILE* deltaArrayCSV = NULL;
   if (pModel->m_logDeltaArrayUpdates)
      {
      deltaArrayCSV = fopen(pModel->m_outputDeltaArrayUpdatesName, "a+t");
      //fprintf(deltaArrayCSV, "Yr, Run, IDU, FireID, Flamelength\n");
      }
   // For each polygon, compute the mean flame length
   for (polyIndex = 0; polyIndex < pPolyGridLkUp->GetNumPolys(); polyIndex++)
      {
      // insure arrays have sufficient space
      if ((gridPtCount = pPolyGridLkUp->GetGridPtCntForPoly(polyIndex)) > maxGridPtCount)
         {
         maxGridPtCount = gridPtCount;

         if (gridPtProportionsArray)
            delete[] gridPtProportionsArray;

         gridPtProportionsArray = new float[maxGridPtCount];

         //if(FlameLengthValues)
         //   delete [] FlameLengthValues;
         //FlameLengthValues = new float[maxGridPtCount];

         if (pGridPtNdxs)
            delete[] pGridPtNdxs;

         pGridPtNdxs = new POINT[maxGridPtCount];
         } // end of: if((ngridPtCount = m_pPolyGridLkUp->GetgridPtCountForPoly(i)) > nmaxGridPtCount)

      for (int i = 0; i < maxGridPtCount; i++)
         {
         gridPtProportionsArray[i] = 0;
         //FlameLengthValues[i] = 0;
         pGridPtNdxs[i].x = 0;
         pGridPtNdxs[i].y = 0;
         }

      // get the grid points for the poly
      pPolyGridLkUp->GetGridPtNdxsForPoly(polyIndex, pGridPtNdxs);

      // get the proportions for the poly
      pPolyGridLkUp->GetGridPtProportionsForPoly(polyIndex, gridPtProportionsArray);

      // weighted mean flame length

      // get the flame length for each grid point
      // multiply it by its proportion and add it to
      //   total flame length

      totalPolyFlameLength = 0.0;
      burnPropSum = 0.0;
      fireID = 0;
      float polyFlameLength;
      float maxProportion = 0;
      int* fireNums = new int[gridPtCount];
      int tNum;
      for (gridPt = 0; gridPt < gridPtCount; gridPt++)
         {
         fireNums[gridPt] = 0;
         polyFlameLength = pFireYearRunner->GetFlameLen(pGridPtNdxs[gridPt].x, pGridPtNdxs[gridPt].y);// *
         if (polyFlameLength > 0.0f)
            {
            burnPropSum += gridPtProportionsArray[gridPt];
            fireNums[gridPt] = pFireYearRunner->GetFireNum(pGridPtNdxs[gridPt].x, pGridPtNdxs[gridPt].y);
            }
         }  // end of: for(gridPt=0;gridPt<ngridPtCount;gridPt++)

      if (burnPropSum >= pModel->m_iduBurnPercentThreshold) //IDU is considered burned?
         {
         //need to weight the flamelength based on proportions
         for (gridPt = 0; gridPt < gridPtCount; gridPt++)
            {
            polyFlameLength = pFireYearRunner->GetFlameLen(pGridPtNdxs[gridPt].x, pGridPtNdxs[gridPt].y);// *

            if (polyFlameLength > 0.0f)
               {
               totalPolyFlameLength += polyFlameLength * gridPtProportionsArray[gridPt] / burnPropSum;
               }
            } // for(gridPt=0;gridPt<ngridPtCount;gridPt++)

         list<int> firesList;
         for (int f = 0; f < gridPtCount; f++)
            {
            if (fireNums[f] > 0)
               {
               if (firesList.empty())
                  firesList.push_back(fireNums[f]);
               else
                  {
                  int found = false;
                  for (std::list<int>::iterator i = firesList.begin(); i != firesList.end(); ++i)
                     {
                     if (*i == fireNums[f])
                        {
                        found = true;
                        break;
                        }
                     }

                  if (!found)
                     firesList.push_back(fireNums[f]);
                  }
               }
            }

         if (firesList.size() == 1)
            {
            tNum = *firesList.begin();
            }

         else
            {
            float fireProp = 0.0, tProp;
            float* props = new float[firesList.size()];
            int loc = 0;
            for (std::list<int>::iterator i = firesList.begin(); i != firesList.end(); ++i)
               {
               props[loc] = 0.0;
               int tFire = *i;
               for (int f = 0; f < gridPtCount; f++)
                  {
                  if (fireNums[f] == tFire)
                     props[loc] += gridPtProportionsArray[gridPt] * pFireYearRunner->GetFlameLen(pGridPtNdxs[gridPt].x, pGridPtNdxs[gridPt].y);
                  }
               loc++;
               }

            tProp = props[0];
            tNum = fireNums[0];
            loc = 0;
            for (std::list<int>::iterator i = firesList.begin(); i != firesList.end(); ++i)
               {
               if (tProp < props[loc])
                  {
                  tProp = props[loc];
                  tNum = *i;
                  }
               loc++;
               }

            delete[] props;
            firesList.clear();
            }

         fireID = tNum;
         }

      delete[] fireNums;
      float oldFlameLength = 0;

      // convert flamelen from m to ft for IDU coverage
      //totalPolyFlameLength *= FT_PER_M;

      pEnvContext->pMapLayer->GetData(polyIndex, colFlamLen, oldFlameLength);

      if (totalPolyFlameLength > 0)
         {
         meanFlameLength += totalPolyFlameLength;
         burnedPolyCount++;

         if (totalPolyFlameLength > maxFlameLength)
            maxFlameLength = totalPolyFlameLength;

         float area = 0;
         pEnvContext->pMapLayer->GetData(polyIndex, colArea, area);
         burnedAreaHa += area;
         }

      if (totalPolyFlameLength != oldFlameLength)
         {
         gpFlamMapAP->UpdateIDU(pEnvContext, polyIndex, colFlamLen, totalPolyFlameLength, ADD_DELTA);
         gpFlamMapAP->UpdateIDU(pEnvContext, polyIndex, colFireID, fireID, ADD_DELTA);
         if (deltaArrayCSV)
            {
            //fprintf(deltaArrayCSV, "Yr, Run, IDU, FireID, Flamelength, PFlameLen, CTSS, VegClass, Variant, DISTURB, FUELMODEL\n");
            fprintf(deltaArrayCSV, "%d, %d, %d, %d, %f", pEnvContext->currentYear, pEnvContext->runID, polyIndex, fireID, totalPolyFlameLength);
            if (colPFlameLen >= 0)
               {
               float fl;
               pEnvContext->pMapLayer->GetData(polyIndex, colPFlameLen, fl);
               fprintf(deltaArrayCSV, ", %f", fl);
               }
            else
               fprintf(deltaArrayCSV, ", ");
            if (colCTSS >= 0)
               {
               CString str;
               pEnvContext->pMapLayer->GetData(polyIndex, colCTSS, str);
               fprintf(deltaArrayCSV, ", %s", (LPCTSTR)str);
               }
            else
               fprintf(deltaArrayCSV, ", ");
            if (colVegClass >= 0)
               {
               int vc;
               pEnvContext->pMapLayer->GetData(polyIndex, colVegClass, vc);
               fprintf(deltaArrayCSV, ", %d", vc);
               }
            else
               fprintf(deltaArrayCSV, ", ");
            if (colVariant >= 0)
               {
               int vc;
               pEnvContext->pMapLayer->GetData(polyIndex, colVariant, vc);
               fprintf(deltaArrayCSV, ", %d", vc);
               }
            else
               fprintf(deltaArrayCSV, ", ");
            if (colDisturb >= 0)
               {
               int vc;
               pEnvContext->pMapLayer->GetData(polyIndex, colDisturb, vc);
               fprintf(deltaArrayCSV, ", %d", vc);
               }
            else
               fprintf(deltaArrayCSV, ", ");
            if (colFuelModel >= 0)
               {
               int vc;
               pEnvContext->pMapLayer->GetData(polyIndex, colFuelModel, vc);
               fprintf(deltaArrayCSV, ", %d\n", vc);
               }
            else
               fprintf(deltaArrayCSV, ", \n");

            }
         }
      } // end of: for(polyIndex=0;polyIndex<pPolyGridLkUp->GetNumPolys();polyIndex++) 

   if (pGridPtNdxs != NULL)
      {
      delete[] pGridPtNdxs;
      pGridPtNdxs = NULL;
      }

   if (gridPtProportionsArray != NULL)
      {
      delete[] gridPtProportionsArray;
      gridPtProportionsArray = NULL;
      }

   pModel->m_meanFlameLen = burnedPolyCount > 0 ? meanFlameLength / burnedPolyCount : 0.0f;
   pModel->m_maxFlameLen = maxFlameLength;
   pModel->m_burnedAreaHa = burnedAreaHa / 10000;
   pModel->m_cumBurnedAreaHa += pModel->m_burnedAreaHa;

   if (deltaArrayCSV)
      {
      fclose(deltaArrayCSV);
      }
   //msg.Format(_T(" Leaving FlameLengthUpdater..."));
   // Report::Log(msg);
   // if(FlameLengthValues != NULL) {
   //    delete[] FlameLengthValues;
   // FlameLengthValues = NULL;

   }

