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

#include "EasternOntario.h"


class LivestockProcess
{
public:
   LivestockProcess( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );

   void AllocateLivestock( EnvContext *pContext, bool useAddDelta );
   
   int m_id;

public:
   int m_colLulc;    // idu coverage
   int m_colLulcA;   
   int m_colArea;    // "AREA"

   int m_colDairyAvg;
   int m_colBeefAvg;
   int m_colPigsAvg;
   int m_colPoultryAvg;

protected:
   bool LoadXml( EnvContext*, LPCTSTR filename );
   
};


//inline
//bool Rotation::FindLulcIndex( int lulc, int &index )
//   {
//   for ( int i=index; i < (int) m_sequenceArray.GetSize(); i++ )
//      if ( 
//
//   }

