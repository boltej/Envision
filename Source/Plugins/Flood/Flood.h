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
#pragma once

#include <EnvExtension.h>

#include "floodArea.h"
#include <PtrArray.h>
#include <MAP.h>


#define _EXPORT __declspec( dllexport )


class _EXPORT Flood : public EnvModelProcess
   {
   public:
      Flood(void);
      ~Flood(void) { } // if ( m_pQueryEngine ) delete m_pQueryEngine; }

      bool Init(EnvContext *pContext, LPCTSTR initStr);
      bool InitRun(EnvContext *pContext, bool useInitialSeed);
      bool Run(EnvContext *pContext);

   protected:

      Map *m_pMap;

      PtrArray<FloodArea> m_floodAreaArray;

      bool InitializeFloodGrids(EnvContext *pContext, FloodArea *pFloodArea);
      bool RunFloodAreas(EnvContext *pContext);


      bool LoadXml(LPCTSTR filename);
      
   };


