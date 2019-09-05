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
#include "StdAfx.h"

#pragma hdrstop

#include "NAESI.h"
#include "F2R.h"
#include <Maplayer.h>
#include <tixml.h>
#include <EnvModel.h>
#include <UNITCONV.H>

#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")

extern F2RProcess *theProcess;


/////////////////////////////////////////////////////////////////////
// W I L D L I F E   M O D E L 
/////////////////////////////////////////////////////////////////////

NAESIModel::NAESIModel( void )
   : m_id( NAESI_HABITAT )  
   , m_colArea(-1)
   , m_colForestCover(-1)
   , m_colForestPatchSize( -1 )
   , m_forestPatchCount( 0 )
   , m_grassCov(NULL)
   , m_forestCover( NULL)
   , m_wetlandCover( NULL )
   , m_wetlandCov( NULL ) 
   {    
   theProcess->m_outVarIndexNaesi = theProcess->AddOutputVar( "Forest Cover", m_forestCover, "" );
   theProcess->AddOutputVar( "Wetland Cover", m_wetlandCover, "" );  
   theProcess->AddOutputVar( "Wetland In Landscape", m_wetlandInLandscape, "" );//number of wetlands with wetlands within 100 m (polygon based)
   theProcess->AddOutputVar( "Grassland Cover", m_grassCover, "" );
   theProcess->AddOutputVar( "Farmland Ecosys Serv", m_farmEcoServ, "" );
   theProcess->AddOutputVar( "Forest Patches < 200ha", m_forestPatchCount, "" );
   theProcess->m_outVarCountNaesi = 5;
   }

NAESIModel::~NAESIModel( void )
   {
   if (m_grassCov != NULL)
      delete [] m_grassCov;
   if (m_forestCov != NULL)
      delete [] m_forestCov;
   if (m_wetlandCov != NULL)
      delete [] m_wetlandCov;
   }


bool NAESIModel::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {   
   int slcCount = (int) theProcess->GetSLCCount();
   m_grassCov   = new float[slcCount];
   m_forestCov  = new float[slcCount];
   m_wetlandCov = new float[slcCount];
   
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   theProcess->CheckCol( pLayer, m_colArea, _T("AREA"), TYPE_FLOAT, CC_MUST_EXIST );//
   theProcess->CheckCol( pLayer, m_colLulcA, _T("LULC_A"), TYPE_LONG, CC_MUST_EXIST );//
   theProcess->CheckCol( pLayer, m_colForestCover, _T("ForestCov"), TYPE_FLOAT, CC_AUTOADD );//
   theProcess->CheckCol( pLayer, m_colWetlandCover, _T("WetCov"), TYPE_FLOAT, CC_AUTOADD );//
   theProcess->CheckCol( pLayer, m_colNearByWetland, _T("WetNear"), TYPE_LONG, CC_AUTOADD );//
   theProcess->CheckCol( pLayer, m_colFarmlandGrasslandHabitatArea, _T("GrassCov"), TYPE_FLOAT, CC_AUTOADD );//
   theProcess->CheckCol( pLayer, m_colFarmlandEcosystemServices, _T("FarmEcoS"), TYPE_FLOAT, CC_AUTOADD );//

   Run( pEnvContext, false );
   return true;
   }


bool NAESIModel::InitRun( EnvContext *pContext )
   {
   return true;
   }


bool NAESIModel::Run( EnvContext *pContext, bool useAddDelta )
   {
  // basic idea - iterate through the SLCs, calculating the NAESI indicators for each
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   int slcCount     = (int) theProcess->GetSLCCount();
   float totalScore = 0;
   int iduCount     = 0;
   float wetlandInLandscapeSum = 0;
   float farmLandEcoServiceSum=0;
   float forestArea=0; float wetlandArea=0.0f; float grassArea=0.0f;
   float totalArea=0;

   m_forestPatchCount = 0;

   for ( int i=0; i < slcCount; i++ )
      {
      SLC *pSLC = theProcess->GetSLC( i );
      // the following functions returns the pct area of the slc in the specified type
      m_forestCov[i]  = GetCover( pSLC, pLayer, 1);  // LULC_A = 1 is Forest.  Calculated for each SLC
      m_wetlandCov[i] = GetCover( pSLC, pLayer, 4);  // LULC_A = 4 is Wetland.  Calculated for each SLC
      m_grassCov[i]   = GetCover( pSLC, pLayer, 3);  // LULC_A = 3 is Grassland.  Calculated for each SLC
      wetlandInLandscapeSum += GetWetlandInLandscape(pSLC,pLayer,pContext, useAddDelta); // Calculated for each IDU, summed for entire study area (AddDelta in GetWetlandInLandscape)
      farmLandEcoServiceSum += GetFarmlandEcosytemServices(pSLC,pLayer,pContext, useAddDelta);// Calculated for each IDU, summed for entire study area (AddDelta in GetFarmlandEcosytemServices)

      totalArea   += pSLC->m_area;
      forestArea  += pSLC->m_area * m_forestCov[i];
      wetlandArea += pSLC->m_area * m_wetlandCov[i];
      grassArea   += pSLC->m_area * m_grassCov[i];

      float forestCover=0.0f;
      int slcPatchCount = 0;
     
      if ( pSLC->m_slcNumber >= 0 )
         {     
         float totalScore = 0;

         for ( int j=0; j < (int) pSLC->m_iduArray.GetSize(); j++ )
            {
            int idu = pSLC->m_iduArray[ j ];

            theProcess->UpdateIDU( pContext, idu, m_colForestCover, m_forestCov[i], useAddDelta ? ADD_DELTA : SET_DATA );  
            theProcess->UpdateIDU( pContext, idu, m_colWetlandCover, m_wetlandCov[i], useAddDelta ? ADD_DELTA : SET_DATA );  
            theProcess->UpdateIDU( pContext, idu, m_colFarmlandGrasslandHabitatArea, m_grassCov[i], useAddDelta ? ADD_DELTA : SET_DATA );  
            }

         //slcPatchCount = GetForestPatchCount( pSLC, pLayer );
         //m_forestPatchCount += slcPatchCount;
         }
      }  // end of: for ( idu < iduCount )

   m_forestCover  = forestArea/totalArea;
   m_wetlandCover = wetlandArea/totalArea;
   m_grassCover   = grassArea/totalArea;
   m_wetlandInLandscape = wetlandInLandscapeSum; //total number of wetlands with nearby wetlands
   m_farmEcoServ = farmLandEcoServiceSum/slcCount; //average % of natural habitat within 3 km of farmland..not area weighted
   return true;
   }



float NAESIModel::GetCover(SLC *pSLC, MapLayer *pLayer,int value )
   {
   float slcArea = 0;
   float totalScore = 0;
   float numerator=0;
   // compute pct areas for each habitat type
   for ( int i=0; i < (int) pSLC->m_iduArray.GetSize(); i++ )
      {
      int idu = pSLC->m_iduArray[ i ];
         
      // first, get habitat type
      int lulcA = -1;
      pLayer->GetData( idu, m_colLulcA, lulcA );

      if ( lulcA < 0 )
         continue;

      float area = 0;
      pLayer->GetData( idu, m_colArea, area );
      slcArea += area;

      if (lulcA == value)
         numerator += area;
      }  
   return numerator/slcArea;
   }

int NAESIModel::GetWetlandInLandscape(SLC *pSLC, MapLayer *pLayer, EnvContext *pEnvContext, bool useAddDelta )
   {
   float slcArea = 0.0f;
   
   float totalScore = 0;
   float numerator=0;
   int numWetlands=0;
   int   neighbors[256 ];
   int total=0;
   for ( int i=0; i < (int) pSLC->m_iduArray.GetSize(); i++ )
      {
      int idu = pSLC->m_iduArray[ i ];
         
      // first, get habitat type
      int lulc = -1;
      pLayer->GetData( idu, m_colLulcA, lulc );
      float areaT=0.0f;

      if ( lulc == 4 ) //wetland
           {
           numWetlands++;
           int nearbyWetlands=0;
           int lulcANeighbor=-1; int lulcBNeighbor=-1;
           float ag=0.0f;float forest=0.0f;float open=0.0f; float shrub=0.0f; float other=0.0f;
           Poly *pPoly = pLayer->GetPolygon( i );
           int count = pLayer->GetNearbyPolys( pPoly, neighbors, NULL, 256, 1000 );
           for ( int j=0; j < count; j++ )
              {
              pLayer->GetData(neighbors[j],m_colLulcA,lulcANeighbor);

              if (lulcANeighbor==4)//another wetland nearby
                 nearbyWetlands++;
              }
         theProcess->UpdateIDU( pEnvContext, idu,m_colNearByWetland,nearbyWetlands, useAddDelta ? ADD_DELTA : SET_DATA ); //reported for each IDU
         if (nearbyWetlands > 0)
            total++;
         }
      
      }   
   return total;//the total number of wetlands that have wetlands within 1000 m (polygon based)
   }

float NAESIModel::GetFarmlandEcosytemServices(SLC *pSLC, MapLayer *pLayer, EnvContext *pEnvContext, bool useAddDelta )
   {
   float slcArea = 0.0f;  
   float totalScore = 0;
   float numerator=0;
   int numWetlands=0;
   int   neighbors[ 256 ];
   int total=0;
   float slcNatWithin3km=0;
   float totalAreaForSLCAveraging=0;
   for ( int i=0; i < (int) pSLC->m_iduArray.GetSize(); i++ )
      {
      int idu = pSLC->m_iduArray[ i ];
         
      // first, get habitat type
      int lulc = -1;
      pLayer->GetData( idu, m_colLulcA, lulc );
      float areaT=0.0f;
      
      if ( lulc == 2 ) //Agriculture...look at neighbors within 3 km, sum the natural area within that distance
         {
         numWetlands++;
         float nearbyNaturalArea = 0;
         int lulcANeighbor       = -1;
         float neighborArea      = 0;
         float nearbyTotalArea   = 0;
         float ag = 0.0f;
         float forest=0.0f;float open=0.0f; float shrub=0.0f; float other=0.0f;
         Poly *pPoly = pLayer->GetPolygon( i );
         int count = pLayer->GetNearbyPolys( pPoly, neighbors, NULL, 256, 3000 );
         for ( int j=0; j < count; j++ )
            {
            pLayer->GetData(neighbors[j],m_colLulcA,lulcANeighbor);
            pLayer->GetData(neighbors[j],m_colArea,neighborArea);

            nearbyTotalArea += neighborArea;//area near this particular IDU
            totalAreaForSLCAveraging += neighborArea;//this will be larger than the total study area
            
            if ( lulcANeighbor==1 || lulcANeighbor==3 || lulcANeighbor==4 || lulcANeighbor==6 || lulcANeighbor==10 )//Not Developed or Agriculture
               {
               nearbyNaturalArea += neighborArea;//this is the area near this particular agricultural polygon
               slcNatWithin3km   += neighborArea;//this is the area within ALL agricultural polygons for any SLC.  The double counts some areas...
               }
           }
         theProcess->UpdateIDU( pEnvContext, idu,m_colFarmlandEcosystemServices,nearbyNaturalArea/nearbyTotalArea, useAddDelta ? ADD_DELTA : SET_DATA ); //reported for each IDU
         }
      }   
   return slcNatWithin3km/totalAreaForSLCAveraging;//percent natural within 3 km of ag for this SLC.  
   }


int NAESIModel::GetForestPatchCount( SLC *pSLC, MapLayer *pLayer )
   {
   int totalIDUCount = pLayer->GetRecordCount();
   int slcIDUCount = (int) pSLC->m_iduArray.GetSize();

   bool *visited = new bool[ totalIDUCount ];

   memset( visited, 0, totalIDUCount * sizeof( bool ) );      // initial all IDU's are unvisited

   int patchCountGT200ha = 0;

   for ( int i=0; i < slcIDUCount; i++ )
      {
      int idu = pSLC->m_iduArray[ i ];

      if ( ! visited[ idu ] )
         {
         // Get the patch.  Returns:
         //   1) area of the expansion area (NOT including the nucleus polygon) and 
         //   2) an array of polygon indexes considered for the patch (DOES include the nucelus polygon).  Zero or Positive indexes indicate they
         //      were included in the patch, negative values indicate they were considered but where not included in the patch
         CArray< int, int > expandArray;
         float expandArea = pLayer->GetExpandPolysFromAttr( idu, m_colLulcA, VData( 1 ), EQ, m_colArea, -1, expandArray );
   
         float iduArea = 0;
         pLayer->GetData( idu, m_colArea, iduArea );
         float patchArea = iduArea + expandArea;
   
         if ( patchArea > 200 * M2_PER_HA )
            patchCountGT200ha++;

         for ( int j=0; j < (int) expandArray.GetSize(); j++ )
            {
            int expandIDU = expandArray[ j ];
            if ( expandIDU < 0 )
               expandIDU = (-expandIDU) - 1;

            ASSERT( expandIDU >= 0 && expandIDU < totalIDUCount );

            visited[ expandIDU ] = true;
            }
         }
      }

   delete [] visited;

   return patchCountGT200ha;
   }
