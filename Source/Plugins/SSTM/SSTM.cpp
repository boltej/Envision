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
// SSTM.cpp : Defines the initialization routines for the DLL.
//
#include "stdafx.h"

#include "SSTM.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension* Factory() { return (EnvExtension*) new SimpleSTM; }



bool SimpleSTM::Run( EnvContext *pEnvContext  )
   {
   // Note: SimpleSTM is responsible for making all changes to the cellLayer
   //       i.e. if it changes lulcC, then it must change lulcB and lulcA.

   const MapLayer *pLayer = pEnvContext->pMapLayer;
   DeltaArray     *pDeltaArray = pEnvContext->pDeltaArray;
   //CUIntArray     *pTargetPolyArray = pEnvContext->pTargetPolyArray;

   ASSERT( pLayer != NULL );
   ASSERT( pDeltaArray != NULL );

   m_transMatrix.Update( pEnvContext ); //pLayer, pDeltaArray, pEnvContext->firstUnseenDelta );
   m_transMatrix.Process( pEnvContext ) ; //pDeltaArray, pLayer, pEnvContext->year, pEnvContext->pLulcTree, pTargetPolyArray );

   return TRUE;
   }


bool SimpleSTM::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   const MapLayer *pLayer = pEnvContext->pMapLayer;
   ASSERT( pLayer != NULL );

   bool ok = m_initialMatrix.LoadXml( pEnvContext, initStr, pLayer );
   m_initialMatrix.Initialize( this, pEnvContext, pLayer );

   this->AddInputVar( "Dormancy Period Scalar", m_ccPeriodScalar, "Parameter describing climate change effect on dormancy period of transition (1=no scaling)" );
   this->AddInputVar( "Transition Probability Scalar", m_ccPCScalar, "Parameter describing climate change effect on transition probabilities (1=no scaling)" );

   EnvExtension::Init( pEnvContext, initStr );

   return ok ? TRUE : FALSE;
   }


bool SimpleSTM::InitRun( EnvContext *pEnvContext, bool )
   {
   m_transMatrix = m_initialMatrix;   
   return TRUE;
   }


