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
#include <QueryEngine.h>


#define _EXPORT __declspec( dllexport )

class _EXPORT PugetSound: public EnvModelProcess
{
public:
   PugetSound( void );
   ~PugetSound( void ) { } // if ( m_pQueryEngine ) delete m_pQueryEngine; }

   bool Init   ( EnvContext *pContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pContext, bool useInitialSeed );
   bool Run    ( EnvContext *pContext );
   bool Setup  ( EnvContext *pContext, HWND hWnd )          { return FALSE; }

protected:
   int m_colPopDens;



   int UpdateLULC(EnvContext* pEnvContext);
   //bool LoadXml( LPCTSTR filename, EnvContext* );
   
};


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new PugetSound; }
