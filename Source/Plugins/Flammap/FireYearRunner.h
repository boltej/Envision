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
/* FireYearRunner.h
**
** Runs the fires for one model year.
**
** Assumes that FireQ is a queue of fires sorted in year order
** and that calls to this routine will be done on a year-ordered
** basis.
**
** 2011.03.16 - tjs - coding initial version pulling much code
**   from FlamMapRunner.h which this FireYearRunner.h replaces.
**
Needs

flame length grid
flame length grid reset
fire queue
access to timestep stuff for file names
file name roots
LCP file
other file names - set these at creation time.

Pull bunches of stuff from FlamMapRunner

*/

#pragma once

#include "FiresList.h"
#include "IgnitGenerator.h"
#include <EnvContext.h>
#include "PolyGridLookups.h"


class FireYearRunner {
private:
   FiresList m_firesList;
   float
      *m_pYrFlameLens;
   int *m_pFireNums;

   IgnitGenerator
      *m_pIgnitGenerator;

	EnvContext
      *m_pEnvContext;

   // Fuel moistures, same fuel moistures used each run
   //CString
   //   m_FuelMoistures;
   
   void RunFlamMap();

public:
   FireYearRunner(EnvContext *pEnvContext);
   ~FireYearRunner();

   int RunFireYear(EnvContext *pEnvContext, PolyGridLookups *pPolyGridLookups);
   float GetFlameLen(const int x, const int y);
	double CalcIduAreaBurned(EnvContext *pEnvContext, float *flameLenLayer, long *numStudyAreaCells);
	int GetFireNum(const int x, const int y);

   bool CalcIduPotentialFlameLen( EnvContext *pContext, MapLayer *pGrid );

}; // class FireYearRunner