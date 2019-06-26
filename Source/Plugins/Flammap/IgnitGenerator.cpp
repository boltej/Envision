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
/* IgnitGenerator.cpp - see .h file for description
**
** 2011.03.23 - Added error handling
**
** tjs - 2011.06.09 - Switching from polygons to points
**   in the igntion file.
*/

#include "stdafx.h"
#pragma hdrstop

#include "IgnitGenerator.h"
#include <Maplayer.h>
#include <MAP.h>
#include <EnvContext.h>
#include <EnvModel.h>

//#include <randgen\randunif.hpp>

#include "FlamMapAP.h"

extern  FlamMapAP *gpFlamMapAP;
extern bool FuelIsNonburnable(int fuel);

IgnitGenerator::IgnitGenerator(EnvContext *pEnvContext) 
   {   
   ASSERT(pEnvContext != NULL);

   int polyCnt = ((MapLayer *)(pEnvContext->pMapLayer))->GetPolygonCount();

   m_MonteCarloIDUProbs = new double[polyCnt];
   for( int i=0; i < polyCnt; i++ )
      m_MonteCarloIDUProbs[i] = 0.;

   m_TtlProb = m_initialProb = m_IDU_TtlProb = m_IDU_InitialProb = 0.0;
   m_MaxPgonNdx = polyCnt - 1;
   m_gridCellProbs = NULL;
   m_numCells = m_rows = m_cols = 0;
   m_IN_UGB_Time0 = new int[polyCnt];
   memset(m_IN_UGB_Time0, 0, polyCnt *sizeof(int));
   } // IgnitGenerator::IgnitGenerator(EnvContext *pEnvContext)


IgnitGenerator::~IgnitGenerator() 
   {
   delete[] m_MonteCarloIDUProbs;
   delete[] m_IN_UGB_Time0;
   DestroyGridCellProbabilities();
   } // IgnitGenerator::~IgnitGenerator()

void IgnitGenerator::DestroyGridCellProbabilities()
{
   if(m_gridCellProbs)
   {
      delete[] m_gridCellProbs;
      m_gridCellProbs = NULL;
   }
}

double IgnitGenerator::GetIgnitRatio()
{
  // if(m_initialProb > 0.0)
     // return m_TtlProb/m_initialProb;
	if(m_IDU_InitialProb > 0.0)
		return this->m_IDU_TtlProb / m_IDU_InitialProb;
	return 1.0;
}

void IgnitGenerator::UpdateProbArray(EnvContext *pEnvContext, PolyGridLookups *pPolyGridLookups, CFlamMap *pFlamMap) 
   {
   ASSERT(pEnvContext != NULL);
   DestroyGridCellProbabilities();
   m_rows = pPolyGridLookups->GetNumGridRows();
   m_cols = pPolyGridLookups->GetNumGridCols();
   m_numCells = m_rows * m_cols;

   m_gridCellProbs = new double[m_numCells];
   if(!m_gridCellProbs)
      {
      //display an error
      return;
      }

  // memset(m_gridCellProbs, 0, m_numCells * sizeof(double));
	for(__int64 c = 0; c < m_numCells; c++)
		m_gridCellProbs[c] = -1.0;
   
   double
      distToMajRd,
      distToMinRd,
      distToUnidentifiedRoad,
      popDens,
      logit,
      ttlProbability = 0.0;
   int
      distToUnidentifiedRoadCol,
      distToMajRdCol,
      distToMinRdCol,
      popDensCol,
      ugbCol,
      ugb;

   __int64
      row,
      col;

   distToUnidentifiedRoadCol = pEnvContext->pMapLayer->GetFieldCol("D_ROADS");
   distToMajRdCol = pEnvContext->pMapLayer->GetFieldCol("RDSMAJ");
   distToMinRdCol = pEnvContext->pMapLayer->GetFieldCol("RDSMIN");
   popDensCol = pEnvContext->pMapLayer->GetFieldCol("POPDENS");
   ugbCol = pEnvContext->pMapLayer->GetFieldCol("IN_UGB");
   
   if( pEnvContext->yearOfRun == pEnvContext->startYear )
      { //populate m_IN_UGB_Time0 for exclusion of polygons
      for( int pgonNdx=0; pgonNdx<((MapLayer *)(pEnvContext->pMapLayer))->GetPolygonCount(); pgonNdx++) 
         {
         pEnvContext->pMapLayer->GetData(pgonNdx, ugbCol, ugb);
         m_IN_UGB_Time0[pgonNdx] = ugb;
         }
      }
   m_IDU_TtlProb = 0.0;
   POINT *pGridPtNdxs = NULL;
   int    GridPtCnt = 0;
   int    MaxGridPtCnt = 0;
	int fuel;

   // Calc probability for each polygon
   double terms[10];
   //CString outName;
   //outName.Format("C:\\Envision\\StudyAreas\\Eugene\\FlamMap\\%d.txt", pEnvContext->year);
   //FILE *out = fopen(outName, "wt");
   //fprintf(out, "ID, D_Major, D_Minor, D_Unkwn, POPDENS, Term1, Term2, Term3, Term4, Term5, Term6, Term7, Term8, Term9, Term10, Logit, Probability\n");
   for( int pgonNdx=0; pgonNdx<((MapLayer *)(pEnvContext->pMapLayer))->GetPolygonCount(); pgonNdx++) 
      {
      if(m_IN_UGB_Time0[pgonNdx] == 0)//not in Urban Growth Area, so use it
         {
         pEnvContext->pMapLayer->GetData(pgonNdx, distToMajRdCol, distToMajRd);
         pEnvContext->pMapLayer->GetData(pgonNdx, distToMinRdCol, distToMinRd);
         pEnvContext->pMapLayer->GetData(pgonNdx, distToUnidentifiedRoadCol, distToUnidentifiedRoad);
         pEnvContext->pMapLayer->GetData(pgonNdx, popDensCol, popDens);

         //convert popdens units (people/m^2 to people / 2.59km^2, using 2,589,988.110336 square meters per square mile
         popDens = popDens * 2590000.0f;
         //if(out)
            //fprintf(out, "%d, %f, %f, %f, %.16f, ", pgonNdx, distToMajRd, distToMinRd, distToUnidentifiedRoad, popDens);
         // calculate the logit for the logistic equation

         terms[0] = -5.533e-04 * distToMajRd;
         terms[1] = 6.869e-08 * pow(distToMajRd, 2);
         terms[2] = -2.539e-12 * pow(distToMajRd, 3);
         terms[3] = -6.681e-04 * distToMinRd;
         terms[4] = 1.017e-07 * pow(distToMinRd, 2);
         terms[5] = -4.098e-12 * pow(distToMinRd, 3);
         terms[6] = -1.905e-04 * distToUnidentifiedRoad;
         terms[7] = 1.243e-02 * popDens;
         terms[8] = -3.843e-05 * pow(popDens , 2);
         terms[9] = 2.759e-08 * pow(popDens , 3);// calculate the monte carlo upper bound for the polygon

         logit = 1.25;
         for(int t = 0; t < 10; t++)
            {
            logit += terms[t];
            //if(out)
               //fprintf(out, "%.16f, ", terms[t]);
            }
         /*   logit =
         1.25 +
         -5.533e-04 * distToMajRd +
         6.869e-08 * pow(distToMajRd, 2) +
         -2.539e-12 * pow(distToMajRd, 3) +
         -6.681e-04 * distToMinRd +
         1.017e-07 * pow(distToMinRd, 2) +
         -4.098e-12 * pow(distToMinRd, 3) +
         -1.905e-04 * distToUnidentifiedRoad +
         1.243e-02 * popDens +
         -3.843e-05 * pow(popDens , 2) +
         2.759e-08 * pow(popDens , 3);// calculate the monte carlo upper bound for the polygon
         */
         //m_TtlProb += exp(logit) / (exp(logit) + 1);
         if(logit >= 7.097827e+002)
            m_MonteCarloIDUProbs[pgonNdx] = 1.0;
         else if (logit <= -7.083964e+002)
            m_MonteCarloIDUProbs[pgonNdx] = 0.0;
         else
            m_MonteCarloIDUProbs[pgonNdx] = exp(logit) / (exp(logit) + 1);
         //fprintf(out, "%f, %.16f\n",logit, m_MonteCarloIDUProbs[pgonNdx]);
         }
      else//in UrbanGrowthArea, set probability to zero
         m_MonteCarloIDUProbs[pgonNdx] = 0.0;

      if((GridPtCnt = pPolyGridLookups->GetGridPtCntForPoly(pgonNdx)) > MaxGridPtCnt) 
         {
         MaxGridPtCnt = GridPtCnt;
         if(pGridPtNdxs)
            delete [] pGridPtNdxs;

         pGridPtNdxs = new POINT[MaxGridPtCnt];
         } // if((nGridPtCnt = m_pPolyGridLkUp->GetGridPtCntForPoly(i)) > nMaxGridPtCnt)

      // get the grid points for the poly
      pPolyGridLookups->GetGridPtNdxsForPoly(pgonNdx, pGridPtNdxs);
      for(int c = 0; c < GridPtCnt; c++)
        {
         row = pGridPtNdxs[c].x, col = pGridPtNdxs[c].y;
			fuel = (int) pFlamMap->GetLayerValueByCell(FUEL, (int) col, (int) row);
			if(!FuelIsNonburnable( fuel ))
				m_gridCellProbs[row * pPolyGridLookups->GetNumGridCols() + col] = m_MonteCarloIDUProbs[pgonNdx];
        }
	  m_IDU_TtlProb += m_MonteCarloIDUProbs[pgonNdx];
      } // for(int pgonNdx=0; pgonNdx<
   if( pGridPtNdxs )
      delete [] pGridPtNdxs;
	if(gpFlamMapAP->m_outputIgnitProbGrids)
	{
		char gridFileName[MAX_PATH];
		sprintf(gridFileName, "%s%d_ignitProb_IDUonly%03d_%04d.asc", (LPCTSTR) gpFlamMapAP->m_outputPath, gpFlamMapAP->processID, pEnvContext->run, pEnvContext->currentYear);
		FILE *gridFile = fopen(gridFileName, "wt");
		fprintf(gridFile, "NCOLS %ld\n", (long) m_cols);
		fprintf(gridFile, "NROWS %ld\n", (long) m_rows);
		fprintf(gridFile, "XLLCORNER %lf\n", pFlamMap->GetWest());
		fprintf(gridFile, "YLLCORNER %lf\n", pFlamMap->GetSouth());
		fprintf(gridFile, "CELLSIZE %lf\n", pFlamMap->GetCellSize());
		fprintf(gridFile, "NODATA_VALUE %f\n", -9999.0);
		__int64 cellLoc;
		for(int r = 0; r < m_rows; r++)
		{
			for(int c = 0; c < m_cols; c++)
			{
				cellLoc = r * m_cols + c;
				if(m_gridCellProbs[cellLoc] > 0.0)
					fprintf(gridFile, "%lf ",  m_gridCellProbs[cellLoc]);
				else
					fprintf(gridFile, "%lf ",  -9999.0);
			}
			fprintf(gridFile, "\n");
		}
		fclose(gridFile);
	}
   __int64 nProbs = 0;
	double minIDUprob = 0.0;
	for(int r = 0; r < m_rows; r++)
	{
		for(int c = 0; c < m_cols; c++)
		{
			if(m_gridCellProbs[r * m_cols + c] > 0.0)
			{
				//fuel = (int) pFlamMap->GetLayerValueByCell(FUEL, c, r);
				//if(!FuelIsNonburnable(fuel))
				//{
					ttlProbability += m_gridCellProbs[r * m_cols + c];
					minIDUprob = min(minIDUprob > 0.0 ? minIDUprob : m_gridCellProbs[r * m_cols + c], m_gridCellProbs[r * m_cols + c]);
					nProbs++;
				//}
			}
		}
	}
	/*for(__int64 c = 0; c < m_numCells; c++)
	{
		fuel = (int) pFlamMap->GetLayerValueByCell(FUEL, fireX, fireY);
		if(m_gridCellProbs[c] > 0.0)
		{
			ttlProbability += m_gridCellProbs[c];
			minIDUprob = min(minIDUprob > 0.0 ? minIDUprob : m_gridCellProbs[c], m_gridCellProbs[c]);
			nProbs++;
		}
	}*/
	for(int r = 0; r < m_rows; r++)
	{
		for(int c = 0; c < m_cols; c++)
		{
			if(m_gridCellProbs[r * m_cols + c] < 0.0)
			{
				fuel = (int) pFlamMap->GetLayerValueByCell(FUEL, c, r);
				if(!FuelIsNonburnable(fuel))
				{
					m_gridCellProbs[r * m_cols + c] = minIDUprob;
					ttlProbability += m_gridCellProbs[r * m_cols + c];
				}
			}
		}
	}
	/*for(__int64 c = 0; c < m_numCells; c++)
	{
		if(m_gridCellProbs[c] < 0.0)
		{
			m_gridCellProbs[c] = minIDUprob;
			ttlProbability += m_gridCellProbs[c];
		}
	}*/

	if(nProbs > 0)
	{
		for(__int64 c = 0; c < m_numCells; c++)
		{
			if(m_gridCellProbs[c] > 0.0)
				m_gridCellProbs[c] /= ttlProbability;
			else
				m_gridCellProbs[c] = 0.0;
			if(c > 0)
			{
				m_gridCellProbs[c] += m_gridCellProbs[c - 1];
			}
		}
	}
   
	if(gpFlamMapAP->m_outputIgnitProbGrids)
	{
		char gridFileName[MAX_PATH];
		sprintf(gridFileName, "%s%d_ignitProb_final%03d_%04d.asc", (LPCTSTR) gpFlamMapAP->m_outputPath, gpFlamMapAP->processID, pEnvContext->run, pEnvContext->currentYear);
		FILE *gridFile = fopen(gridFileName, "wt");
		fprintf(gridFile, "NCOLS %ld\n", (long) m_cols);
		fprintf(gridFile, "NROWS %ld\n", (long) m_rows);
		fprintf(gridFile, "XLLCORNER %lf\n", pFlamMap->GetWest());
		fprintf(gridFile, "YLLCORNER %lf\n", pFlamMap->GetSouth());
		fprintf(gridFile, "CELLSIZE %lf\n", pFlamMap->GetCellSize());
		fprintf(gridFile, "NODATA_VALUE %f\n", -9999.0);
		__int64 cellLoc;
		for(int r = 0; r < m_rows; r++)
		{
			for(int c = 0; c < m_cols; c++)
			{
				cellLoc = r * m_cols + c;
				if(cellLoc == 0 && m_gridCellProbs[cellLoc] > 0.0)
				{
					fprintf(gridFile, "%lf ",  m_gridCellProbs[cellLoc]);
				}
				else if(cellLoc > 0 && m_gridCellProbs[cellLoc] > m_gridCellProbs[cellLoc - 1])
				{
					double diff = m_gridCellProbs[cellLoc] - m_gridCellProbs[cellLoc - 1];
					fprintf(gridFile, "%lf ",  diff);

				}
				else
					fprintf(gridFile, "%lf ",  -9999.0);
			}
			fprintf(gridFile, "\n");
		}
		fclose(gridFile);
	}
   if( pEnvContext->yearOfRun == pEnvContext->startYear )
   {
      m_initialProb = ttlProbability;
	  m_IDU_InitialProb = m_IDU_TtlProb;
   }
   
   m_TtlProb = ttlProbability;
   //TRACE3("Year %d, InitialProb = %f, totalProb = %f", pEnvContext->year, m_initialProb, m_TtlProb);
   //CString msg;
   //msg.Format(_T("Year %d, InitialProb = %f, totalProb = %f"), pEnvContext->currentYear, m_initialProb, m_TtlProb);
   //Report::Log(msg);
   } // void IgnitGenerator::UpdateProbArray(EnvContext *pEnvContext)


int IgnitGenerator::GetIgnitionPointFromGrid(EnvContext *pEnvContext, REAL &x, REAL &y)
{
   int ret = 0;
   
   double monteCarloDraw = gpFlamMapAP->m_pIgnitRand->GetUnif01() * m_gridCellProbs[m_numCells - 1];  /////m_randUnif.RandValue(0.0, m_gridCellProbs[m_numCells - 1]);
   __int64 c = 0;

   while(monteCarloDraw > m_gridCellProbs[c] && c < m_numCells)
      c++;
   
   __int64 row, col;
   
   col = c % m_cols;
   row = (c - col) / m_cols;
   
   REAL xMin, xMax, yMin, yMax, res;
   pEnvContext->pMapLayer->GetExtents(xMin, xMax, yMin, yMax);
   
   res = (float) gpFlamMapAP->m_cellDim;
   x = xMin + col * res + res / 2.0;
   y = yMax - row * res - res / 2.0;
   
   return 1;
}
