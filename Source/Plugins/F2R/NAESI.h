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

enum  { FORESTCOVER=0,  NAESI_COUNT=1 , SLC_COUNT};

class NAESIModel
{
public:
   NAESIModel( void );
   ~NAESIModel(void);
   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );
      
   float GetSLCScore( SLC *pSLC, MapLayer *pLayer, int slcNumber );
   int m_id;
   int m_colArea;

protected:
   CString m_lulcField;
   float m_slcScoreArray[ SLC_COUNT ];
   int m_colLulcA;

   //FOREST
   // int m_forestComposition;   //NOT IMPLEMENTED: Not tracking cover types.  slc level reporting.  An areal summary of forest cover types.
   // float *m_forestComp;
   //LULC_B 8 and 9 (natural and managed forest).  We are not tracking age class
   float m_forestCover;   // Reporting unit – slc?  
   int m_colForestCover;
   float *m_forestCov;
   // Precent forest cover (LULC_B 8 and 9 or LULC_A 1)
   int m_colForestPatchSize;//Based on IDUs .  need to use adjacency rules to determine where patches are 
   float m_forestPatchCount;
   
   //and their area.
   //int m_colForestEdge;//NOT IMPLEMENTED Don't have operational definition of edge
  // int m_colForestLandscapeComplementation;//NOT IMPLEMENTED Don't have operational definition of corridors

   //WETLAND
   int m_colWetlandCover;//Assumes existing delineations/LULC types in IDU's are sufficient
   float *m_wetlandCov;
   float m_wetlandCover;
   // percent of slc that is wetland. 
   // LULC_A =4
   int m_colWetlandLandscapeComplementation;// NOT IMPLEMENTED:  Habitat width?//Limited by IDU resolution 
   // 100m habitat width surrounding wetlands 10% with 200-300m width
   // not clear what is meant by 'habitat width'
   int m_colWetlandInLandscape;//Limited by IDU resolution 
   int m_colNearByWetland;
   float m_wetlandInLandscape;
   // Groups of wetlands within500-1000m of the centre of each
  // int m_colWetlandPatchSize;// NOT IMPLEMENTED:  Unclear what is really needed here/  Variety of sizes, types, hydroperiods required marsh >200ha
  // int m_colImpervious;//NOT IMPLEMENTED:  Need to develop imperviousness by LULC class for developed



   //RIPARIAN
 //  int m_colRiparian;//NOT IMPLEMENTED: Our representation of riparian areas in the IDU coverage is poor at best

   //FARMLAND
   int m_colFarmlandGrasslandHabitatArea;//Reporting units?  SLC?
   float *m_grassCov;
   float m_grassCover;
   // Maintain existing grassland and cultural habitats for grassland-dependent species –10% in watershed for area sensitive species
   int m_colFarmlandGrasslandPatchSize;//Limited by IDU resolution
   // 50 to 200 ha patches in grassland and cultural habitats for area-sensitive species by watershed
   int m_colFarmlandEcosystemServices;
   float m_farmEcoServ;
   // 10-40% in natural or seminatural habitats within 1-3km of farm fields for ecosystem services (pollination and pest
   // control)– 40% for full pollinator services
   
   
   int m_colFarmlandConnectivity;//Not Implemented: We can't really see hedgerows in the spatial information
   int m_colRoads;//Won't change in the scenarios, so little value to this one.
  

   float GetCover(SLC *pSLC, MapLayer *pLayer , int value);
   int GetWetlandInLandscape(SLC *pSLC, MapLayer *pLayer, EnvContext *pEnvContext, bool useAddDelta );
   float GetFarmlandEcosytemServices(SLC *pSLC, MapLayer *pLayer, EnvContext *pEnvContext, bool useAddDelta );

   int GetForestPatchCount( SLC *pSLC, MapLayer *pLayer );
   };