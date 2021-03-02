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
// NAHARP.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "F2R.h"
#include "Wildlife.h"
#include "PModel.h"
#include "NModel.h"
#include "ClimateIndicators.h"
#include "NAESI.h"
#include "CropRotation.h"
#include "FarmModel.h"
#include "Livestock.h"
#include <QueryEngine.h>

#include <Maplayer.h>
#include <Report.h>
#include <tixml.h>
#include <fdataobj.h>
#include <unitconv.h>

static int calcPopDens = false;

F2RProcess *theProcess = NULL;

extern "C" _EXPORT EnvExtension* Factory(EnvContext *pContext)
   {
   switch (pContext->id)
      {

      case WILDLIFE:
      case PHOSPHORUS:
      case NITROGEN:
      case NAESI_HABITAT:
      case FARM_MODEL:
         if (theProcess == NULL)
            theProcess = new F2RProcess;

         return (EnvExtension*) theProcess;
      
      case REPORT:
         return (EnvExtension*) new F2RReport;
      }

   return nullptr;
   }

////////////////////////////////////////////////////

F2RProcess::F2RProcess() 
: EnvModelProcess()
, m_inVarIndexFarmModel( 0 )
, m_inVarCountFarmModel( 0 )
, m_outVarIndexFarmModel( 0 )
, m_outVarCountFarmModel( 0 )
, m_pWildlife( NULL )
, m_pPhosphorus( NULL )
, m_pNAESI(NULL)
, m_pNitrogen( NULL )
, m_pFarmModel( NULL )
, m_outVarIndexWildlife( 0 )
, m_outVarCountWildlife( 0 )
, m_outVarIndexPModel( 0 )
, m_outVarCountPModel( 0 )
, m_outVarIndexNModel( 0 )
, m_outVarCountNModel( 0 )
, m_outVarIndexNaesi( 0 )
, m_outVarCountNaesi( 0 )
, m_colSLC( -1 )
, m_colArea( -1 )
   { }


F2RProcess::~F2RProcess( void )
   {
   if ( m_pFarmModel != NULL )
      delete m_pFarmModel;

   if ( m_pWildlife != NULL )
      delete m_pWildlife;

   if ( m_pPhosphorus != NULL )
      delete m_pPhosphorus;

   if ( m_pNitrogen != NULL )
      delete m_pNitrogen;

//   if ( m_pClimateIndicators != NULL )
//      delete m_pClimateIndicators;

   if ( m_pNAESI != NULL )
      delete m_pNAESI;
   }


bool F2RProcess::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   //////////////
   if ( calcPopDens )
      {
      MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
      BuildPopDens( pLayer );
      calcPopDens = false;
      }
   ////////////////////

   if ( GetSLCCount() == 0 )
      {
      m_colSLC = pEnvContext->pMapLayer->GetFieldCol( "SLC_ID" );
      if ( m_colSLC < 0 )
         {
         Report::ErrorMsg( "F2R Error: IDU Coverage is missing 'SLC_ID' field - this is a required field." );
         return FALSE;
         }

      TiXmlDocument doc;
      bool ok = doc.LoadFile( initStr );

      if ( ! ok )
         {      
         Report::ErrorMsg( doc.ErrorDesc() );
         return FALSE;
         }
 
      // start interating through the nodes
      TiXmlElement *pXmlRoot = doc.RootElement();  // naharp
      LPCTSTR areaCol = pXmlRoot->Attribute( "area_col" );
      if ( areaCol == NULL )
         areaCol = "AREA";
   
      ok = CheckCol( pEnvContext->pMapLayer, m_colArea, areaCol, TYPE_FLOAT, CC_MUST_EXIST );
      if ( ! ok ) 
         return false;
      
      BuildSLCs( (MapLayer*) pEnvContext->pMapLayer );
      }

   switch( pEnvContext->id )
      {
      case FARM_MODEL:     // 
         {
         if ( m_pFarmModel == NULL )
            m_pFarmModel = new FarmModel;

         return m_pFarmModel->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;

      case WILDLIFE:     // wildlife
         {
         if ( m_pWildlife == NULL )
            m_pWildlife = new WildlifeModel;

         return m_pWildlife->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;

      case PHOSPHORUS:  
         {
         if ( m_pPhosphorus == NULL )
            m_pPhosphorus = new PModel;

         return m_pPhosphorus->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;

      case NITROGEN:  
         {
         if ( m_pNitrogen == NULL )
            m_pNitrogen = new NModel;

         return m_pNitrogen->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;

      //case CLIMATE:  
      //   {
      //   if ( m_pClimateIndicators == NULL )
      //      m_pClimateIndicators = new ClimateIndicatorsModel;
      //
      //   return m_pClimateIndicators->Init( pEnvContext, initStr ) ? TRUE : FALSE;
      //   }
      //   break;

      case NAESI_HABITAT:  
         {
         if ( m_pNAESI == NULL )
            m_pNAESI= new NAESIModel;

         return m_pNAESI->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;

      default:
         ;
      }

   return FALSE;
   }


bool F2RProcess::InitRun( EnvContext *pEnvContext, bool useInitialSeed )
   {
   switch( pEnvContext->id )
      {
      case FARM_MODEL:
         if ( m_pFarmModel != NULL )
            return m_pFarmModel->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;

      case WILDLIFE:     // wildlife
         if ( m_pWildlife != NULL )
            return m_pWildlife->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;

      case PHOSPHORUS:
         if ( m_pPhosphorus != NULL )
            return m_pPhosphorus->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;

      case NITROGEN:
         if ( m_pNitrogen != NULL )
            return m_pNitrogen->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;

      case NAESI_HABITAT:
         if ( m_pNAESI != NULL )
            return m_pNAESI->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;
      }

   return FALSE;
   }



bool F2RProcess::Run( EnvContext *pEnvContext )
   {
   switch( pEnvContext->id )
      {
      case FARM_MODEL:
         if ( m_pFarmModel != NULL )
            return m_pFarmModel->Run( pEnvContext ) ? TRUE : FALSE;
         break;

      case WILDLIFE:     // wildlife
         if ( m_pWildlife != NULL )
            return m_pWildlife->Run( pEnvContext ) ? TRUE : FALSE;
         break;

      case PHOSPHORUS:
         if ( m_pPhosphorus != NULL )
            return m_pPhosphorus->Run( pEnvContext ) ? TRUE : FALSE;
         break;

      case NITROGEN:
         if ( m_pNitrogen != NULL )
            return m_pNitrogen->Run( pEnvContext ) ? TRUE : FALSE;
         break;

      case NAESI_HABITAT:
         if ( m_pNAESI != NULL )
            return m_pNAESI->Run( pEnvContext ) ? TRUE : FALSE;
         break;
      }

   return FALSE;
   }




bool F2RProcess::EndRun(EnvContext *pEnvContext)
   {
   switch (pEnvContext->id)
      {
      case FARM_MODEL:
         if (m_pFarmModel != NULL)
            return m_pFarmModel->EndRun(pEnvContext) ? TRUE : FALSE;
         break;

      //case WILDLIFE:     // wildlife
      //   if (m_pWildlife != NULL)
      //      return m_pWildlife->EndRun(pEnvContext) ? TRUE : FALSE;
      //   break;
      //
      //case PHOSPHORUS:
      //   if (m_pPhosphorus != NULL)
      //      return m_pPhosphorus->EndRun(pEnvContext) ? TRUE : FALSE;
      //   break;
      //
      //case NITROGEN:
      //   if (m_pNitrogen != NULL)
      //      return m_pNitrogen->EndRun(pEnvContext) ? TRUE : FALSE;
      //   break;
      //
      //case NAESI_HABITAT:
      //   if (m_pNAESI != NULL)
      //      return m_pNAESI->EndRun(pEnvContext) ? TRUE : FALSE;
      //   break;
      }

   return TRUE;
   }



int F2RProcess::InputVar ( int id, MODEL_VAR** modelVar )
   {
   *modelVar = NULL;

   switch( id )
      {
      case FARM_MODEL:
         if ( m_inVarCountFarmModel > 0 )
            {
            *modelVar = m_inputVars.GetData() + m_inVarIndexFarmModel;
            return m_inVarCountFarmModel;
            }
         
      case WILDLIFE:     // wildlife
      case PHOSPHORUS:
      case NITROGEN:
      case NAESI_HABITAT:
         return 0;
      }

   return 0;
   }


int F2RProcess::OutputVar( int id, MODEL_VAR** modelVar )
   {
   *modelVar = NULL;

   switch( id )
      {
      case FARM_MODEL:
         if ( m_outVarCountFarmModel > 0 )
            {
            *modelVar = m_outputVars.GetData() + m_outVarIndexFarmModel;
            return m_outVarCountFarmModel;
            }
         break;

      case WILDLIFE:     // wildlife
         if ( m_outVarCountWildlife > 0 )
            {
            *modelVar = m_outputVars.GetData() + m_outVarIndexWildlife;
            return m_outVarCountWildlife;
            }
        break;
           
      case PHOSPHORUS:
         if ( m_outVarCountPModel > 0 )
            {
            *modelVar = m_outputVars.GetData() + m_outVarIndexPModel;
            return m_outVarCountPModel;
            }
        break;

      case NITROGEN:
         if ( m_outVarCountNModel > 0 )
            {
            *modelVar = m_outputVars.GetData() + m_outVarIndexNModel;
            return m_outVarCountNModel;
            }
        break;

      case NAESI_HABITAT:
         if ( m_outVarCountNaesi > 0 )
            {
            *modelVar = m_outputVars.GetData() + m_outVarIndexNaesi;
            return m_outVarCountNaesi;
            }
         break;
      }  
   
   // default
   *modelVar = NULL;
   return 0;
   }





////////////////////////////////////////////////////

F2RReport::F2RReport() 
: EnvModelProcess()
, m_colArea( -1 )
, m_colLulc( -1 )
, m_colDairyAvg( -1 )
, m_colBeefAvg( -1 )
, m_colPigsAvg( -1 )
, m_colPoultryAvg( -1 )
, m_totalIDUArea ( 0 )
, m_totalIDUAreaAg( 0 )
, m_pctCashCrop( 0 )
, m_pctHayPasture( 0 )
, m_hayPastureAreaHa( 0 )
, m_bioenergyAreaHa( 0 )
, m_cropRotationAreaHa( 0 )
, m_fruitAreaHa( 0 )
, m_vegAreaHa( 0 )
, m_beefAreaHa( 0 )
, m_dairyAreaHa( 0 )
, m_pigAreaHa( 0 )
, m_dairyCount( 0 )
, m_beefCount( 0 )
, m_poultryCount( 0 )
, m_pigCount( 0 )
   { }


bool F2RReport::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;

   m_colArea = pEnvContext->pMapLayer->GetFieldCol( "AREA" );
   m_colLulc = pEnvContext->pMapLayer->GetFieldCol( "LULC_C" );
   m_colDairyAvg = pEnvContext->pMapLayer->GetFieldCol( "DairyAvg" );
   m_colBeefAvg  = pEnvContext->pMapLayer->GetFieldCol( "BeefAvg" );
   m_colPoultryAvg = pEnvContext->pMapLayer->GetFieldCol( "Poultry_Av" );
   m_colPigsAvg    = pEnvContext->pMapLayer->GetFieldCol( "Pigs_Avg" );

   m_totalIDUArea = 0;

   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      float area = 0;
      pLayer->GetData( idu, m_colArea, area );

      m_totalIDUArea += area;
      }      

   AddOutputVar( "Fraction of Ag Area in Cash Crops", m_pctCashCrop, "" );
   AddOutputVar( "Fraction of Ag Area in Hay-Pasture", m_pctHayPasture, "" );
   AddOutputVar( "Area in Crop Rotation (ha)", m_cropRotationAreaHa, "" );
   AddOutputVar( "Area in Pasture (ha)", m_hayPastureAreaHa, "" );
   AddOutputVar( "Area in Bioenergy Crop (ha)", m_bioenergyAreaHa, "" );
   AddOutputVar( "Area in Fruit Crop (ha)", m_fruitAreaHa, "" );
   AddOutputVar( "Area in Vegetable Crop (ha)", m_vegAreaHa, "" );
   AddOutputVar( "Area in Beef (ha)", m_beefAreaHa, "" );
   AddOutputVar( "Area in Dairy (ha)", m_dairyAreaHa, "" );
   AddOutputVar( "Area in Pig (ha)", m_pigAreaHa, "" );
   AddOutputVar( "Area in Poultry (ha)", m_poultryAreaHa, "" );

   AddOutputVar( "Number of Beef",    m_beefCount, "" );
   AddOutputVar( "Number of Dairy",   m_dairyCount, "" );
   AddOutputVar( "Number of Pig",     m_pigCount, "" );
   AddOutputVar( "Number of Poultry", m_poultryCount, "" );
   return TRUE;
   }


bool F2RReport::InitRun( EnvContext *pEnvContext, bool useInitialSeed )
   {
   Run( pEnvContext );
     
   return TRUE;
   }



bool F2RReport::Run( EnvContext *pEnvContext )
   {
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;

   int colLulcA = pLayer->GetFieldCol( "LULC_A");

   m_totalIDUAreaAg = 0;
   m_pctCashCrop    = 0;
   m_pctHayPasture  = 0;

   m_hayPastureAreaHa = 0;
   m_bioenergyAreaHa = 0;
   m_cropRotationAreaHa = 0;
   m_fruitAreaHa = 0;
   m_vegAreaHa   = 0;

   m_beefAreaHa  = 0;
   m_dairyAreaHa = 0;
   m_pigAreaHa   = 0;
   m_poultryAreaHa = 0;

   m_beefCount  = 0;
   m_dairyCount = 0;
   m_pigCount   = 0;
   m_poultryCount = 0;
   
   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      float area = 0;
      pLayer->GetData( idu, m_colArea, area );

      int lulcA = 0;
      pLayer->GetData( idu, colLulcA, lulcA );
      
      if ( lulcA == 2 || lulcA == 3 )   // ag?
         {
         m_totalIDUAreaAg += area;

         int lulc = 0;
         pLayer->GetData( idu, m_colLulc, lulc );

         if ( IsCashCrop( lulc ) )
            m_pctCashCrop += area;
         else if ( lulc == 50 ) // Pasture
            m_pctHayPasture += area;

         // bioenergy? fruits?  veggies?
         switch( lulc )
            {
            case 50: m_hayPastureAreaHa += area;   break; 
            case 300:
            case 301: m_bioenergyAreaHa  += area;   break;

            case 180: m_fruitAreaHa      += area;   break;
            case 179: m_vegAreaHa        += area;   break;
            }

         // crop rotation?
         if ( theProcess->m_pFarmModel )
            {
            int rotationID = 0;
            pLayer->GetData( idu, theProcess->m_pFarmModel->m_colRotation, rotationID );
      
            // are we in a rotation?  If so, move through the rotation
            if ( rotationID > 0 )
               m_cropRotationAreaHa += area;
            }
         }

      // animal operation?
      float beefAvg = 0;
      pLayer->GetData( idu, m_colBeefAvg, beefAvg );
      if ( beefAvg > 0 )
         {
         m_beefCount  += beefAvg;  // UNITS????
         m_beefAreaHa += area;
         }

      float dairyAvg = 0;
      pLayer->GetData( idu, m_colDairyAvg, dairyAvg );
      if ( dairyAvg > 0 )
         {
         m_dairyCount  += dairyAvg;
         m_dairyAreaHa += area;
         }

      float pigAvg = 0;
      pLayer->GetData( idu, m_colPigsAvg, pigAvg );
      if ( pigAvg > 0 )
         {
         m_pigCount += pigAvg;
         m_pigAreaHa += area;
         }

      float poultryAvg = 0;
      pLayer->GetData( idu, m_colPoultryAvg, poultryAvg );
      if ( poultryAvg > 0 )
         {
         m_poultryCount += poultryAvg;
         m_poultryAreaHa += area;
         }
      }

   m_pctCashCrop    *= ( 100 / m_totalIDUAreaAg );
   m_pctHayPasture  *= ( 100 / m_totalIDUAreaAg );

   m_cashCropRatio   = 100 * m_pctCashCrop   / ( m_pctCashCrop + m_pctHayPasture );
   m_hayPastureRatio = 100 * m_pctHayPasture / ( m_pctCashCrop + m_pctHayPasture );

   m_hayPastureAreaHa   /= M2_PER_HA;
   m_bioenergyAreaHa    /= M2_PER_HA;
   m_cropRotationAreaHa /= M2_PER_HA;
   m_fruitAreaHa        /= M2_PER_HA;
   m_vegAreaHa          /= M2_PER_HA;

   m_beefAreaHa    /= M2_PER_HA;
   m_dairyAreaHa   /= M2_PER_HA;
   m_pigAreaHa     /= M2_PER_HA;
   m_poultryAreaHa /= M2_PER_HA;

   return TRUE;
   }


bool F2RReport::IsCashCrop( int lulcC )
   {
   switch( lulcC )
      {
      case 167:      // Beans
      case 162:      // Peas
      case 158:      // Soybeans
      case 195:      // Buckwheat
      case 133:      // Barley
      case 136:      // oats
      case 140:      // wheat
      case 147:      // Corn
      case 110:      // Grassland/Rangeland
      case 180:      // Fruits
      case 193:      // Herbs
      case 179:      // Vegetables
      case 122:      // Alfalfa
      case 300:      // Annual Bioenergy Crop
      case 301:      // Perennial Bioenergy Crop
         return true;
      }

   return false;
   }


//---------------------------------------------------------------------------
// F2RProcess methods
//---------------------------------------------------------------------------


bool F2RProcess::BuildSLCs( MapLayer *pLayer )
   {
   m_slcArray.RemoveAll();
   m_slcIDtoIndexMap.RemoveAll();
   
   if ( m_colSLC < 0 )
      return false;

   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      // get SLC
      int slcID = -1;
      pLayer->GetData( idu, m_colSLC, slcID );
      
      if ( slcID < 0 )
         continue;

      SLC *pSLC = NULL;

      int index = -1;
      bool ok = m_slcIDtoIndexMap.Lookup( slcID, index );

      if  ( ok ) // already seen this one?  then no further action required
         pSLC = m_slcArray[ index ];
      else
         {
         pSLC = new SLC;
         INT_PTR index = m_slcArray.Add( pSLC );
         m_slcIDtoIndexMap.SetAt( slcID, (int) index );

         pSLC->m_slcNumber=slcID;
         }

      pSLC->m_iduArray.Add( idu );
      
      float area = 0;
      pLayer->GetData( idu, m_colArea, area );
      pSLC->m_area += area;
      }  // end of: for ( idu < iduCount )

   return true;
   }

bool F2RProcess::BuildPopDens( MapLayer *pLayer )
   {
   int colID   = pLayer->GetFieldCol( "DBUID" );
   int colArea = pLayer->GetFieldCol( "AREA" );
   int colPop  = pLayer->GetFieldCol( "POPULATION" );
   int colPopDens  = pLayer->GetFieldCol( "POPDENS" );
   
   CStringArray idArray;
   pLayer->GetUniqueValues( colID, idArray );

   QueryEngine qe( pLayer );

   CArray< float > cuAreaArray;
   cuAreaArray.SetSize( idArray.GetSize() );

   CArray< float > cuPopArray;
   cuPopArray.SetSize( idArray.GetSize() );

   for ( int i=0; i < (int) idArray.GetSize(); i++ )
      {
      cuAreaArray[ i ] = 0;
      cuPopArray[ i ] = 0;
      }

   CString msg;
   msg.Format( "Initializing PopDens - %i Census units found", (int) idArray.GetSize() );
   Report::Log( msg );

   for ( int i=0; i < (int) idArray.GetSize(); i++ )
      {
      if ( i % 10 == 0 )
         {
         CString msg;
         msg.Format( "PopDens: processing %i of %i census units", i, (int) idArray.GetSize() );
         Report::StatusMsg( msg );
         }
      
      CString query;
      query.Format( "CDUID=%s", idArray[ i ] );

      Query *pQuery = qe.ParseQuery( query, 0, "" );

      int selCount = pQuery->Select( true );

      for ( int j=0; j < selCount; j++ )
         {
         int idu = pLayer->GetSelection( j );
          
         float area = 0;
         pLayer->GetData( idu, colArea, area );

         float pop = 0;
         pLayer->GetData( idu, colPop, pop );

         cuAreaArray[ i ] += area;
         cuPopArray[ i ]  += pop;
         }

      // have sums, now distribute to IDUs
      float popDens = cuPopArray[ i ] / cuAreaArray[ i ];

      for ( int j=0; j < selCount; j++ )
         {
         int idu = pLayer->GetSelection( j );
         pLayer->SetData( idu, colPopDens, popDens );
         }
      }

   pLayer->ClearSelection();

   return true;
   }



