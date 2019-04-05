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
#include "EnvLibs.h"
#pragma hdrstop

#include "Delta.h"
//#include "EnvModel.h"
/*

LPCTSTR DELTA::GetTypeStr( short type )
   {
   switch( type )
      {
      case DT_NULL:           return _T("Null");
      case DT_POLICY:         return _T("Policy");
      case DT_SUCCESSION:     return _T("Veg Trans");
      case DT_SUBDIVIDE:      return _T("Subdivide");
      case DT_MERGE:          return _T("Merge");
      case DT_INCREMENT_COL:  return _T("IncrementCol");
      case DT_POP:            return _T("POP");
      }

#ifndef DELTA_NO_ENVMODEL

   if ( type >= DT_MODEL && type < DT_PROCESS )
      {
      int index = type-DT_MODEL;
      ASSERT( index < EnvModel::GetModelCount() );
      ENV_EVAL_MODEL *pInfo = EnvModel::GetModelInfo( index );
      return pInfo->name;
      }

   else if ( type >= DT_PROCESS )
      {
      int index = type-DT_PROCESS;
      ASSERT( index < EnvModel::GetAutonomousProcessCount() );
      ENV_AUTO_PROCESS *pInfo = EnvModel::GetAutonomousProcessInfo( index );
      return pInfo->name;
      }
#endif

   return _T("Unknown");
   }

   */