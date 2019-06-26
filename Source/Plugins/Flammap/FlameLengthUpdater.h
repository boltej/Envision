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
/* FlameLengthUpdater.h
**
** Function used for updating FlameLengths in the 
** delta matrix.
**
** The strategy here is to get the flame lengths from
** grid space as they are stored in the FireYearRunner
** and use the PolyGridLookUp to populate the flame
** length in the delta array for each polygon.
**
** All that's needed is a function to compute the flame
** length for each polygon and update the delta array
**
*/

#pragma once
#include <EnvExtension.h>
#include <Maplayer.h>
#include "FireYearRunner.h"
#include "PolyGridLookups.h"


class FlamMapAP;


class FlameLengthUpdater
{
public:
	void UpdateDeltaArray(
      FlamMapAP *pModel,
		EnvContext *pEnvContext,
		FireYearRunner *pFireYearRunner,
		PolyGridLookups *pPolyGridLkUp
		);

};
