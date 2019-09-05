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

#include "EasternOntario.h"
#include "Wildlife.h"
#include "PModel.h"
#include "NModel.h"
#include "ClimateIndicators.h"
#include "NAESI.h"
#include "CropRotation.h"
#include "Livestock.h"
#include <QueryEngine.h>

#include <Maplayer.h>
#include <Report.h>
#include <tixml.h>
#include <fdataobj.h>
#include <unitconv.h>


/*
1: FarmModel
0: NAHAPR
*/

extern "C" _EXPORT EnvExtension* Factory(EnvContext *pContext)
   {
   switch (pContext->id)
      {
      case 1:  return (EnvExtension*) new  COCNHProcessPre1;
      case 4:  return (EnvExtension*) new COCNHProcessPre2;
      case 8:  return (EnvExtension*) new COCNHProcessPost1;
      case 16: return (EnvExtension*) new COCNHSetTSD;
      case 32: return (EnvExtension*) new COCNHProcessPost2;
      }

   return nullptr;
   }

EasternOntarioModel::EasternOntarioModel() 
: EnvEvaluator()
, m_pWildlife( NULL )
, m_pPhosphorus( NULL )
, m_pClimateIndicators(NULL)
, m_pNAESI(NULL)
, m_pNitrogen( NULL )
//, m_pCropRotation( NULL )
, m_outVarIndexWildlife( 0 )
, m_outVarCountWildlife( 0 )
, m_outVarIndexPModel( 0 )
, m_outVarCountPModel( 0 )
, m_outVarIndexNModel( 0 )
, m_outVarCountNModel( 0 )
, m_outVarIndexClimate( 0 )
, m_outVarCountClimate( 0 )
, m_outVarIndexNaesi( 0 )
, m_outVarCountNaesi( 0 )
//, m_outVarIndexCropRotation( 0 )
//, m_outVarCountCropRotation( 0 )
, m_colSLC( -1 )
, m_colArea( -1 )
   { }

EasternOntarioModel::~EasternOntarioModel( void )
   {
   if ( m_pWildlife != NULL )
      delete m_pWildlife;

   if ( m_pPhosphorus != NULL )
      delete m_pPhosphorus;

   if ( m_pNitrogen != NULL )
      delete m_pNitrogen;

   if ( m_pClimateIndicators != NULL )
      delete m_pClimateIndicators;

   if ( m_pNAESI != NULL )
      delete m_pNAESI;

   //if ( m_pCropRotation != NULL )
   //   delete m_pCropRotation;
   }

static int calcPopDens = false;

bool EasternOntarioModel::Init( EnvContext *pEnvContext, LPCTSTR initStr )
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
         Report::ErrorMsg( "NAHARP Error: IDU Coverage is missing 'SLC_ID' field - this is a required field." );
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
      //case CROP_ROTATION:     // 
      //   {
      //   if ( m_pCropRotation == NULL )
      //      m_pCropRotation = new CropRotationModel;
      //
      //   return m_pCropRotation->Init( pEnvContext, initStr ) ? TRUE : FALSE;
      //   }
      //   break;

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

      case CLIMATE:  
         {
         if ( m_pClimateIndicators == NULL )
            m_pClimateIndicators = new ClimateIndicators;

         return m_pClimateIndicators->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;

      case NAESI_HABITAT:  
         {
         if ( m_pNAESI == NULL )
            m_pNAESI= new NAESI;

         return m_pNAESI->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;

      default:
         ;
      }

   return FALSE;
   }


bool EasternOntarioModel::InitRun( EnvContext *pEnvContext, bool useInitialSeed )
   {
   switch( pEnvContext->id )
      {
      //case CROP_ROTATION:
      //   if ( m_pCropRotation != NULL )
      //      return m_pCropRotation->InitRun( pEnvContext ) ? TRUE : FALSE;
      //   break;

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

      case CLIMATE:
         if ( m_pClimateIndicators != NULL )
            return m_pClimateIndicators->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;

      case NAESI_HABITAT:
         if ( m_pNAESI != NULL )
            return m_pNAESI->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;
      }

   return FALSE;
   }



bool EasternOntarioModel::Run( EnvContext *pEnvContext )
   {
   switch( pEnvContext->id )
      {
      //case CROP_ROTATION:
      //   if ( m_pCropRotation != NULL )
      //      return m_pCropRotation->Run( pEnvContext ) ? TRUE : FALSE;
      //   break;

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

      case CLIMATE:
         if ( m_pClimateIndicators != NULL )
            return m_pClimateIndicators->Run( pEnvContext ) ? TRUE : FALSE;
         break;

      case NAESI_HABITAT:
         if ( m_pNAESI != NULL )
            return m_pNAESI->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;
      }

   return FALSE;
   }


bool EasternOntarioModel::BuildSLCs( MapLayer *pLayer )
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

bool EasternOntarioModel::BuildPopDens( MapLayer *pLayer )
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
   Report::LogMsg( msg );

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

      int selCount = pQuery->Select( true, false );

      for ( int j=0; j < selCount; j++ )
         {
         int idu = pLayer->GetQueryResult( j );
          
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
         int idu = pLayer->GetQueryResult( j );
         pLayer->SetData( idu, colPopDens, popDens );
         }
      }

   pLayer->ClearQueryResults();

   return true;
   }



int EasternOntarioModel::InputVar ( int id, MODEL_VAR** modelVar )
   {
   *modelVar = NULL;

   switch( id )
      {
      case CROP_ROTATION:
      case WILDLIFE:     // wildlife
      case PHOSPHORUS:
      case NITROGEN:
      case CLIMATE:
      case NAESI_HABITAT:
         return 0;
      }

   return 0;
   }


int EasternOntarioModel::OutputVar( int id, MODEL_VAR** modelVar )
   {
   *modelVar = NULL;

   switch( id )
      {
      //case CROP_ROTATION:
      //   if ( m_outVarCountCropRotation > 0 )
      //      {
      //      *modelVar = m_outputVars.GetData() + m_outVarIndexCropRotation;
      //      return m_outVarCountCropRotation;
      //      }
      //  break;

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

      case CLIMATE:
         if ( m_outVarCountClimate > 0 )
            {
            *modelVar = m_outputVars.GetData() + m_outVarIndexClimate;
            return m_outVarCountClimate;
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

EasternOntarioProcess::EasternOntarioProcess() 
: EnvModelProcess()
, m_pCropRotation( NULL )
, m_pLivestock( NULL )
, m_outVarIndexCropRotation( 0 )
, m_outVarCountCropRotation( 0 )
, m_outVarIndexLivestock( 0 )
, m_outVarCountLivestock( 0 )
   { }

EasternOntarioProcess::~EasternOntarioProcess( void )
   {
   if ( m_pCropRotation != NULL )
      delete m_pCropRotation;

   if ( m_pLivestock != NULL )
      delete m_pLivestock;
   }


bool EasternOntarioProcess::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   switch( pEnvContext->id )
      {
      case CROP_ROTATION:     // 
         {
         if ( m_pCropRotation == NULL )
            m_pCropRotation = new CropRotationProcess;

         return m_pCropRotation->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;

      case LIVESTOCK:     // 
         {
         if ( m_pLivestock == NULL )
            m_pLivestock = new LivestockProcess;

         return m_pLivestock->Init( pEnvContext, initStr ) ? TRUE : FALSE;
         }
         break;
      }

   return FALSE;
   }


bool EasternOntarioProcess::InitRun( EnvContext *pEnvContext, bool useInitialSeed )
   {
   switch( pEnvContext->id )
      {
      case CROP_ROTATION:
         if ( m_pCropRotation != NULL )
            return m_pCropRotation->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;

      case LIVESTOCK:
         if ( m_pLivestock != NULL )
            return m_pLivestock->InitRun( pEnvContext ) ? TRUE : FALSE;
         break;
      }

   return FALSE;
   }



bool EasternOntarioProcess::Run( EnvContext *pEnvContext )
   {
   switch( pEnvContext->id )
      {
      case CROP_ROTATION:
         if ( m_pCropRotation != NULL )
            return m_pCropRotation->Run( pEnvContext ) ? TRUE : FALSE;
         break;

      case LIVESTOCK:
         if ( m_pLivestock != NULL )
            return m_pLivestock->Run( pEnvContext ) ? TRUE : FALSE;
         break;
      }

   return FALSE;
   }



int EasternOntarioProcess::InputVar ( int id, MODEL_VAR** modelVar )
   {
   *modelVar = NULL;

   switch( id )
      {
      case CROP_ROTATION:
         return 0;

      case LIVESTOCK:
         return 0;
      }

   return 0;
   }


int EasternOntarioProcess::OutputVar( int id, MODEL_VAR** modelVar )
   {
   *modelVar = NULL;

   switch( id )
      {
      case CROP_ROTATION:
         if ( m_outVarCountCropRotation > 0 )
            {
            *modelVar = m_outputVars.GetData() + m_outVarIndexCropRotation;
            return m_outVarCountCropRotation;
            }
         break;

      case LIVESTOCK:
         if ( m_outVarCountLivestock > 0 )
            {
            *modelVar = m_outputVars.GetData() + m_outVarIndexLivestock;
            return m_outVarCountLivestock;
            }
         break;
      }  
   
   // default
   *modelVar = NULL;
   return 0;
   }





////////////////////////////////////////////////////

EasternOntarioReport::EasternOntarioReport() 
: EnvAutoProcess()
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


bool EasternOntarioReport::Init( EnvContext *pEnvContext, LPCTSTR initStr )
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
   AddOutputVar( "Area in Hay/Pasture (ha)", m_hayPastureAreaHa, "" );
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


bool EasternOntarioReport::InitRun( EnvContext *pEnvContext, bool useInitialSeed )
   {
   Run( pEnvContext );
     
   return TRUE;
   }



bool EasternOntarioReport::Run( EnvContext *pEnvContext )
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

   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      float area = 0;
      pLayer->GetData( idu, m_colArea, area );

      int lulcA = 0;
      pLayer->GetData( idu, colLulcA, lulcA );
      
      if ( lulcA == 2 )   // ag?
         {
         m_totalIDUAreaAg += area;

         int lulcC = 0;
         pLayer->GetData( idu, m_colLulc, lulcC );

         if ( IsCashCrop( lulcC ) )
            m_pctCashCrop += area;
         else if ( lulcC == 17 ) // Hay
            m_pctHayPasture += area;

         // bioenergy? fruits?  veggies?
         switch( lulcC )
            {
            case 17: m_hayPastureAreaHa += area;   break; 
            case 26: m_bioenergyAreaHa  += area;   break;
            case 18: m_fruitAreaHa      += area;   break;
            case 21: m_vegAreaHa        += area;   break;
            }

         // crop rotation?
         int rotationID = 0;
         pLayer->GetData( idu, theProcess->m_pCropRotation->m_colRotation, rotationID );
      
         // are we in a rotation?  If so, move through the rotation
         if ( rotationID > 0 )
            m_cropRotationAreaHa += area;
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


bool EasternOntarioReport::IsCashCrop( int lulcC )
   {
   switch( lulcC )
      {
      case 10:      // Beans
      case 11:      // Peas
      case 12:      // Soybeans
      case 13:      // Buckwheat
      case 14:      // Grains
      case 15:      // Corn
      case 16:      // Grassland/Rangeland
      case 18:      // Fruits
      case 20:      // Herbs
      case 21:      // Vegetables
      case 24:      // Alfalfa
      case 25:      // Silage
      case 26:      // Bioenergy Crop
         return true;
      }

   return false;
   }
