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

enum  { N_CROP_REC=0, N_FIXED, N_DEPOSIT, N_CROP_REM, N_MANURE_TOT, N_FERT, N_NET, N_COUNT };


class Lulc
{
public:
   CString m_name;
   int     m_id;

   // additional variables
   float m_slcArea;      // slc variable - area in this lulc class
   float m_Nrecrtl;      // recommended fertilizer rate (kg/ha/yr)
   float m_Nfixrt;       // nitrogen fixation rate (kg/ha/yr)
   float m_Ncrop;        // crop nitrogen (kg N/ha )  N-contained in crop
};


class NModel
{
public:
   NModel( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );

   int m_id;

protected:
   CString m_lulcField;
      
   int m_colLulc;   // idu coverage
   int m_colArea;   // "AREA"


   //int m_colPigs;
   //int m_colPoultry;
   //int m_colCattle;

   //CMap< int, int, HabitatSubType*, HabitatSubType* > m_lulcToHabSubTypeMap;  // key = attribute value, value=associated column in lookup table
   //PtrArray< Specie      > m_speciesArray;    // collection of species infor (rows in table)
   //PtrArray< HabitatUse  > m_habUseArray;     // collection of habitat use info
   //PtrArray< HabitatType > m_habTypeArray;    // collection of habitat type info (cols in table)
   //
   bool  LoadXml( EnvContext*, LPCTSTR filename );
   //bool  LoadTable( LPCTSTR habScoreTable, LPCTSTR habitatTypeField, MapLayer *pIduLayer );
 
   float GetSLCScore( SLC *pSLC, MapLayer *pLayer );
   float PopulateSLCAreas( SLC*, MapLayer *pLayer );

   PtrArray< Lulc > m_lulcArray;        // one for each ag class
   CMap< int, int, int, int > m_lulcIdToIndexMap; // key=id,  value=index in m_lulcArray

   PtrArray< Animal > m_animalArray;

   float m_outVarArray[ N_COUNT ];
   //float m_weightArray[ P_COUNT ];
  
};


