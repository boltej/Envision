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

#include <Vdataobj.h>
#include <FDATAOBJ.H>
#include <PtrArray.h>
#include "F2R.h"

enum  { P_SAT=0, P_SOILTEST, P_EROSION, P_OVERLANDFLOW, P_EXCESS, P_MANURE, P_FERT, P_COUNT };



class PModel
{
public:
   PModel( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );

   int m_id;

protected:
   CString m_lulcField;
      
   int m_colLulc;   // idu coverage
   int m_colArea;   // "AREA"
   int m_colLulcB;
   int m_colIROWC_P06;
   float m_agOnAtRisk;
   float m_devOnAtRisk;
   float m_livestockOnAtRisk;
   float m_agArea;
   float m_devArea;
   //CMap< int, int, HabitatSubType*, HabitatSubType* > m_lulcToHabSubTypeMap;  // key = attribute value, value=associated column in lookup table
   //PtrArray< Specie      > m_speciesArray;    // collection of species infor (rows in table)
   //PtrArray< HabitatUse  > m_habUseArray;     // collection of habitat use info
   //PtrArray< HabitatType > m_habTypeArray;    // collection of habitat type info (cols in table)
   //
   bool  LoadXml( EnvContext*, LPCTSTR filename );
   //bool  LoadTable( LPCTSTR habScoreTable, LPCTSTR habitatTypeField, MapLayer *pIduLayer );
   float GetSLCScore( SLC *pSLC, MapLayer *pLayer );   
   float GetSat( MapLayer *pLayer, int idu );
   float GetSoilTest( MapLayer *pLayer, int idu );
   float GetErosion( MapLayer *pLayer, int idu );
   float GetOverlandFlow( MapLayer *pLayer, int idu );
   float GetExcess( MapLayer *pLayer, int idu );
   float GetManure( MapLayer *pLayer, int idu );
   float GetFert( MapLayer *pLayer, int idu );

   float m_slcScoreArray[ P_COUNT ];
   float m_weightArray[ P_COUNT ];

   void GetSummary( EnvContext *pEnvContext );
  
};


