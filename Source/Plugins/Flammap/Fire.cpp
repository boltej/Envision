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
// Fire.cpp 
//
// By Tim Sheehan 2011.03.14
//
// Stores parameters for a fire. See Fire.h for description

#include "stdafx.h"

#include "FlammapAP.h"

#include <string>
#include <queue>
#include <cstdlib>
#include "Fire.h"
#include <cstring>
//#include "ParamLookup.h"

extern FlamMapAP *gpFlamMapAP;


//Fire::Fire(const char *InitStr, float monteCarloProb, const char *baseFmsName)
//   {
//   char workStr[2048], *tmp;
//
//   strcpy(workStr, InitStr);
//
//   int  
//      yr,
//      julDate,
//      burnPeriod,
//      windSpd,
//      azmth,
//		erc = 0;
//		
//   float 
//      prob, sizeHA = 0.0;
//   char FMSname[MAX_PATH], shortFMS[MAX_PATH];
//   // parse string into tokens, convert tokens into values
//   char seps[15];
//   strcpy(seps, ", ");
//   tmp = strtok(workStr,seps);
//   yr = atoi(tmp);
//   tmp = strtok(NULL,seps);
//   prob = (float)atof(tmp);
//   tmp = strtok(NULL,seps);
//   julDate = atoi(tmp);
//   tmp = strtok(NULL,seps);
//   burnPeriod = (int) (atof(tmp) + 0.499);//converted to minutes
//   tmp = strtok(NULL,seps);
//   windSpd = atoi(tmp);
//   tmp = strtok(NULL,seps);
//   azmth = atoi(tmp);
//   tmp = strtok(NULL,seps);
//   if(tmp)
//   {
//      for(int c = (int) strlen(tmp) - 1; c>=0; c--)
//      {
//         if(!isalnum(tmp[c]))
//            tmp[c] = 0;
//         else
//            break;
//      }
//      CString nameCStr;
//      nameCStr.Format(_T("%s%s"), baseFmsName, tmp );
//      lstrcpy(FMSname, nameCStr);
//		strcpy(shortFMS, tmp);
//		//add ERC and original (calibrated) fire size
//		tmp = strtok(NULL,seps);
//		if(tmp)
//			erc = atoi(tmp);
//		tmp = strtok(NULL,seps);
//		if(tmp)
//			sizeHA = (float) atof(tmp);
//   }
//   //else
//  //    strcpy(FMSname, gpFlamMapAP->m_scenarioFuelMoisturesFName );
//
//   Init( yr, prob, julDate, burnPeriod, windSpd, azmth, monteCarloProb, FMSname, shortFMS, erc, sizeHA );
//   } // Fire(char  *InitStr)
//

/*Fire::Fire( int yr, float prob, int julDate, int burnPeriod, int windSpd, int azmth, float monteCarloProb ) 
   {
   Init( yr, prob, julDate, burnPeriod, windSpd, azmth, monteCarloProb, "", "", 0, 0 );
   } // Fire(int yr,...)
	*/
/*
void Fire::Init( int yr, float prob, int julDate, int burnPeriod, int windSpd, int azmth,float monteCarloProb, 
	char *FMSname, char *shortFMS, int erc, float size )
   {
   ASSERT( yr >= 0 && julDate !=0 );

   m_yr = yr;
   m_julDate = julDate;
   m_windSpd = windSpd;
   m_windAzmth = azmth;
   m_burnProb = prob;
   m_burnPeriod = burnPeriod;
   strcpy(m_FMSname, FMSname);
	strcpy(m_shortFMSname, shortFMS);
	m_erc = erc;
	m_origSizeHA = size;
	m_ignitX = m_ignitY = 0.0;
   m_doRun = (m_burnProb >= monteCarloProb);
	m_fuel = 0;
   } // Fire::Init(...

   */