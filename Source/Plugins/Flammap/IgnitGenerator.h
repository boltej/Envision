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
/* IgnitGenerator.h
**
** Generates an ignition file for use with the FlamMap DLL based
** on a probability surface.
**
** The probability surface is generated in Envision's polygon space.
**
** A monte carlo method is used to pick the ignition location from
** the probability surface.
*/

#pragma once

#include "PolyGridLookups.h"
#include <EnvExtension.h>
#include <Maplayer.h>
#include <randgen\Randunif.hpp>
#include "FlamMap_DLL.h"

class IgnitGenerator
{
private:
   double *m_MonteCarloIDUProbs; // 1 element for each IDU
   double m_TtlProb;
   double m_initialProb;
   double m_IDU_TtlProb;
   double m_IDU_InitialProb;

   int m_MaxPgonNdx;
   __int64 m_numCells;
   __int64 m_rows;
   __int64 m_cols;

   void DestroyGridCellProbabilities();

   double *m_gridCellProbs;   
   int    *m_IN_UGB_Time0;

public:
   IgnitGenerator(EnvContext *pEnvContext);
   ~IgnitGenerator();

   void UpdateProbArray(EnvContext *pEnvContext, PolyGridLookups *pPolyGridLookups, CFlamMap *pFlamMap);
   int GetIgnitionPointFromGrid(EnvContext *pEnvContext, REAL &x, REAL &y);
   double GetIgnitRatio();
}; // class IgnitGenerator