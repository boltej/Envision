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
// PugetSound.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "PugetSound.h"
#include <Maplayer.h>
#include <Map.h>
#include <Report.h>
#include <tixml.h>
#include <PathManager.h>
#include <UNITCONV.H>
#include <envconstants.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


PugetSound::PugetSound() 
: EnvModelProcess()
, m_colPopDens(-1)
{ }


bool PugetSound::Init( EnvContext *pContext, LPCTSTR initStr )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   //m_pQueryEngine = pContext->pQueryEngine;

   //if ( LoadXml( initStr, pContext ) == false )
   //   return FALSE;

   m_colPopDens = pLayer->GetFieldCol("PopDens");

   return true; 
   }


bool PugetSound::InitRun( EnvContext *pContext, bool useInitialSeed )
   {

   return true; 
   }


// Allocates new DUs, Updates N_DU and NEW_DU
bool PugetSound::Run( EnvContext *pEnvContext )
   {
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   int iduCount = pLayer->GetRecordCount();

   UpdateLULC(pEnvContext);

   return true;
   }



int PugetSound::UpdateLULC(EnvContext* pEnvContext)
   {
   DeltaArray* deltaArray = pEnvContext->pDeltaArray;


   if (deltaArray != NULL)
      {
      INT_PTR size = deltaArray->GetSize();
      if (size > 0)
         {
         for (INT_PTR i = pEnvContext->firstUnseenDelta; i < size; i++)
            {
            DELTA& delta = ::EnvGetDelta(deltaArray, i);
            if (delta.col == m_colPopDens)
               {
               float popDens = 0;
               delta.newValue.GetAsFloat(popDens);
               
               //if ( Between(popDens,0.)    // 1 du per acre
               //
               //
               //   DU / Acre		# / m2
               //
               //   1		0.00623
               //   2		0.01247
               //   4		0.02494
               //   6		0.03740
               //   8		0.04987
               //   12		0.07481
               //   18		0.11221
               //   24		0.14962
               //   48		0.29924



               }
            }
         }
      }

   // apply deltas generated above, since the IDU's need to be updated before the remaining code is run
   //::EnvApplyDeltaArray(pEnvContext->pEnvModel);

   return true;
   }


