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

#include "NModel.h"
#include "F2R.h"
#include <Maplayer.h>
#include <tixml.h>
#include <UNITCONV.H>


extern F2RProcess *theProcess;


/////////////////////////////////////////////////////////////////////
// W I L D L I F E   M O D E L 
/////////////////////////////////////////////////////////////////////

NModel::NModel( void )
   : m_id( NITROGEN )   
   , m_colLulc( -1 )
   {
   for( int i=0; i < N_COUNT; i++ )
      m_outVarArray[ i ] = 0;
   }



bool NModel::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   theProcess->m_outVarCountNModel = N_COUNT;
   theProcess->m_outVarIndexNModel = theProcess->AddOutputVar( "Crop N Requirement (Gg)", m_outVarArray[ N_CROP_REC ], "" );
   theProcess->AddOutputVar( "N Fixed (Gg)",               m_outVarArray[ N_FIXED], "" );
   theProcess->AddOutputVar( "N Deposition (Gg)",          m_outVarArray[ N_DEPOSIT ], "" );
   theProcess->AddOutputVar( "N Removed with Crops (Gg)",  m_outVarArray[ N_CROP_REM ], "" );
   theProcess->AddOutputVar( "Total Manure N (Gg)",        m_outVarArray[ N_MANURE_TOT ], "" );
   theProcess->AddOutputVar( "Fertilizer N Reqd (Gg)",     m_outVarArray[ N_FERT ], "" );
   theProcess->AddOutputVar( "Net N (Gg)",                 m_outVarArray[ N_NET ], "" );
   
   // loads table
   bool ok = LoadXml( pEnvContext, initStr );

   if ( ! ok ) 
      return false;

   ok = theProcess->CheckCol( pEnvContext->pMapLayer, m_colArea, "Area", TYPE_FLOAT, CC_MUST_EXIST );
   if ( ! ok ) 
      return false;

   if ( pEnvContext->col >= 0 )
      ok = Run( pEnvContext, false );

   return ok;
   }


bool NModel::InitRun( EnvContext *pContext )
   {
   return true;
   }


bool NModel::Run( EnvContext *pContext, bool useAddDelta )
   {
   // basic idea - iterate through the SLCs, calculating a contribution from each
   // of a variety of P influences.
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   int slcCount     = (int) theProcess->GetSLCCount();
   float totalScore = 0;
   int iduCount     = 0;

   // reset outputs  
   for( int i=0; i < N_COUNT; i++ )
      m_outVarArray[ i ] = 0;

   float totalArea = 0;
   
   for ( int i=0; i < slcCount; i++ )
      {
      SLC *pSLC = theProcess->GetSLC( i );
      float slcScore = GetSLCScore( pSLC, pLayer );

      // write to coverage if indicated
      if ( pContext->col >= 0 )
         {
         for ( int j=0; j < (int) pSLC->m_iduArray.GetSize(); j++ )
            {
            int idu = pSLC->m_iduArray[ j ];
            if ( useAddDelta )
               {
               float oldScore = 0;
               pLayer->GetData( idu, pContext->col, oldScore );

               if ( fabs( oldScore - slcScore ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, pContext->col, slcScore );
               }
            else
               pLayer->SetData( idu, pContext->col, slcScore );
            }
         }

      totalScore += slcScore * pSLC->m_area;
      totalArea  += pSLC->m_area;
      }  // end of: for ( idu < iduCount )

   // unit converstion for outputs - kg -> Tg
   for ( int i=0; i < N_COUNT; i++ )
      m_outVarArray[ i ] /= 1000000.0f;

   totalScore /= totalArea;  // area-weighted average over each SLC

   pContext->rawScore = totalScore;
   pContext->score = -3.0f + ( totalScore/50 )*6;

   if ( pContext->score > 3 )
      pContext->score = 3;

   if ( pContext->score < -3 )
      pContext->score = -3;
    
   return true;
   }
   

float NModel::GetSLCScore( SLC *pSLC, MapLayer *pLayer )
   {
   int lulcCount = (int) m_lulcArray.GetSize();

   // populate m_lulcArray with areas of each ag class in the SLC
   float slcArea = PopulateSLCAreas( pSLC, pLayer );

   // RSN - Residual Soil Nitrogen model - calculated as the difference between N inputs from
   // chemical fertilizers, manure, biological N fixation and N in crop residues, and outputs in the form
   // of N in the harvested portion of crops. The main inputs are crop areas, livestock types and numbers,
   // and fertilizer use data

   // humid region calculations:
   //    RSN = Nfert + Nmanagement + Nfix + NDeposition - Ncrop - nGas
   // arid regions calculations:
   //    RSN = F*NcropRes + Nsfres (summer fallow mineralization) + Nlgres (legume crop residues)

   // note: all of the following have units of kgN/ha
   float Nrcmd = 0;     // recommended N - summed across crops in this SLC
   float Nfix  = 0;     // n fixation rate
   float Ndep  = 0;     // deposition on ag lands
   float Ncrop = 0;     // N stored in removed crops
   float farmArea = 0;  // ha
   for ( int i=0; i < lulcCount; i++ )
      {
      Lulc *pLulc = m_lulcArray[ i ];
     
      float pctArea = pLulc->m_slcArea / slcArea;

      if ( pctArea > 0.001f )
         {
         Nrcmd += pctArea * pLulc->m_Nrecrtl;
         Nfix  += pctArea * pLulc->m_Nfixrt;
         Ndep  += pctArea * 0.75f;
         Ncrop += pctArea * pLulc->m_Ncrop;
      
         farmArea += pLulc->m_slcArea/M2_PER_HA;
         }
      }
   
   float NmanTot = 0;   // total manure N produced - sum from cattle, pigs, poultry, etc.
   for ( int i=0; i < (int) m_animalArray.GetSize(); i++ )
      {
      Animal *pAnimal = m_animalArray[ i ];

      if ( pAnimal->m_col < 0 )
         continue;

      for ( int j=0; j < (int) pSLC->m_iduArray.GetSize(); j++ )
         {
         int idu = pSLC->m_iduArray[ j ];
         float count =0;
         
         pLayer->GetData( idu, pAnimal->m_col, count );
         float nmanure = count * pAnimal->m_nmanure;
         NmanTot += nmanure;   // kg/year
         }
      }

   // normalise manure to SLC area
   NmanTot /= ( pSLC->m_area / M2_PER_HA );  // make the units Kg/Ha/Year
   
   float Nman = 0.50f * NmanTot;    // 50% of total manure is available to crops
   float NmanUPast = 0.11f * Nman;  // 11% to that goes to unimproved pasture
   float NmanCrops = 0.89f * Nman;  // manure applied to crops
   float NmanExcess = NmanCrops - Nrcmd;   // get excess above and beyond crop needs

   if ( NmanExcess > 0 )
      NmanExcess = 0;

   // add in non-crop uses (
   NmanExcess += (NmanTot-Nman) + NmanUPast;    

   if ( NmanCrops > Nrcmd )
      NmanCrops = Nrcmd;
   
   float Nfert = Nrcmd - NmanCrops;  // N applied to crops as fertilizer
 
    // RSN - Residual Soil Nitrogen model - calculated as the difference between N inputs from
   // chemical fertilizers, manure, biological N fixation and N in crop residues, and outputs in the form
   // of N in the harvested portion of crops. The main inputs are crop areas, livestock types and numbers,
   // and fertilizer use data
   // + applied to Crops (inorganic + manure)
   // + deposited
   // + fixed
   // + excess manure NOT applied to chrops
   // - removed with crops (exported)
   float netN = Nrcmd + Nfix + Ndep + NmanExcess +  - Ncrop;   // kg/ha

   // set outputs kg/ha * m2 * ha/m2 = kg
   m_outVarArray[ N_CROP_REC   ] += Nrcmd   * slcArea / M2_PER_HA;
   m_outVarArray[ N_FIXED      ] += Nfix    * slcArea / M2_PER_HA;
   m_outVarArray[ N_DEPOSIT    ] += Ndep    * slcArea / M2_PER_HA;
   m_outVarArray[ N_CROP_REM   ] += Ncrop   * slcArea / M2_PER_HA;
   m_outVarArray[ N_MANURE_TOT ] += NmanTot * slcArea / M2_PER_HA;
   m_outVarArray[ N_FERT       ] += Nfert   * slcArea / M2_PER_HA;
   m_outVarArray[ N_NET        ] += netN    * slcArea / M2_PER_HA;
     
   return netN;
   }




bool NModel::LoadXml( EnvContext *pContext, LPCTSTR filename )
   {
   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   if ( ! ok )
      {      
      Report::ErrorMsg( doc.ErrorDesc() );
      return false;
      }
 
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // f2r

   TiXmlElement *pXmlModel = pXmlRoot->FirstChildElement( "nitrogen" );
   if ( pXmlModel == NULL )
      {
      CString msg( "NAHARP Nitrogen Model: missing <nitrogen> element in input file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   if ( pXmlModel == NULL )
      return false;

   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   // lookup fields
   LPTSTR lulcField   = NULL;
   XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                      { "lulc_col",      TYPE_STRING,   &lulcField,   true,  CC_MUST_EXIST },
                      { NULL,            TYPE_NULL,     NULL,         false, 0 } };

   ok = TiXmlGetAttributes( pXmlModel, attrs, filename, pLayer );

   if ( ! ok )
      return false;

   m_colLulc = pLayer->GetFieldCol( lulcField );
   if ( m_colLulc < 0 )
      {
      CString msg;
      msg.Format( "NAHARP Nitrogen: 'lulc_col'=%s;  The specified field was not found in the IDU coverage", lulcField );
      Report::ErrorMsg( msg );
      return false;
      }

   m_colArea = pLayer->GetFieldCol( "Area" );
   if ( m_colArea < 0 )
      {
      Report::ErrorMsg( "NAHARP Nitrogen: 'AREA' field was not found in the IDU coverage" );
      return false;
      }

   // next, load lulc info
   TiXmlElement *pXmlLulcs = pXmlModel->FirstChildElement( "lulc_classes" );
   if ( pXmlLulcs == NULL )
      {
      CString msg( "NAHARP Nitrogen: Missing <lulc_classes> tag reading file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   TiXmlElement *pXmlLulc = pXmlLulcs->FirstChildElement( "lulc" );

   while ( pXmlLulc != NULL )
      {
      LPTSTR name  = NULL;
      int    id    = 0;
      float  nRec  = 0;
      float  nFix  = 0;
      float  yield = 0;
      float  nCrop = 0;

      XML_ATTR attrs[] = { // attr          type        address      isReq checkCol
                         { "name",         TYPE_STRING,   &name,       true,  0 },
                         { "id",           TYPE_INT,      &id,         true,  0 },
                         { "n_rec",        TYPE_FLOAT,    &nRec,       true,  0 },
                         { "n_fix",        TYPE_FLOAT,    &nFix,       true,  0 },
                         { "yield",        TYPE_FLOAT,    &yield,      true,  0 },
                         { "n_crop",       TYPE_FLOAT,    &nCrop,      true,  0 },
                         { NULL,           TYPE_NULL,     NULL,        false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlLulc, attrs, filename );

      if ( ok )
         {
         Lulc *pLulc = new Lulc;
         pLulc->m_name = name;
         pLulc->m_id = id;
         pLulc->m_Nrecrtl = nRec;
         pLulc->m_Nfixrt = nFix;
         //pLulc->m_yield  = yield;
         pLulc->m_Ncrop  = nCrop;

         int index = (int) m_lulcArray.Add( pLulc );

         m_lulcIdToIndexMap.SetAt( id, index );
         }

      pXmlLulc = pXmlLulc->NextSiblingElement( "lulc" );
      }
   
   // next, load animal info
   TiXmlElement *pXmlAnimals = pXmlModel->FirstChildElement( "animal_classes" );
   if ( pXmlAnimals == NULL )
      {
      CString msg( "NAHARP Nitrogen: Missing <animals_classes> tag reading file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   TiXmlElement *pXmlAnimal = pXmlAnimals->FirstChildElement( "animal" );

   while ( pXmlAnimal != NULL )
      {
      LPTSTR name    = NULL;
      int    id      = 0;
      LPTSTR  col    = NULL;
      float  wt      = 0;
      float  manure  = 0;
      float  nmanure = 0;
      float  pmanure = 0;

      XML_ATTR attrs[] = { // attr       type        address      isReq checkCol
                         { "name",      TYPE_STRING,   &name,       true,  0 },
                         { "id",        TYPE_INT,      &id,         true,  0 },
                         { "col",       TYPE_STRING,   &col,        false,  CC_MUST_EXIST },
                         { "wt",        TYPE_FLOAT,    &wt,         false,  0 },
                         { "manure",    TYPE_FLOAT,    &manure,     false,  0 },
                         { "nmanure",   TYPE_FLOAT,    &nmanure,    false,  0 },
                         { "pmanure",   TYPE_FLOAT,    &pmanure,    false,  0 },
                         { NULL,        TYPE_NULL,     NULL,        false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlAnimal, attrs, filename, pLayer );

      if ( ok )
         {
         Animal *pAnimal = new Animal( id, name );
         pAnimal->m_name = name;

         if ( col != NULL )
            pAnimal->m_col = pLayer->GetFieldCol( col );

         pAnimal->m_wt = wt;
         pAnimal->m_manure = manure;
         pAnimal->m_nmanure = nmanure;
         pAnimal->m_pmanure = pmanure;

         int index = (int) m_animalArray.Add( pAnimal );
         }

      pXmlAnimal = pXmlAnimal->NextSiblingElement( "animal" );
      }

   CString msg;
   msg.Format( "NAHARP Nitrogen: Loaded %i lulc classes", (int) m_lulcArray.GetSize() );
   Report::Log( msg );

   return true;
   }
   

float NModel::PopulateSLCAreas( SLC *pSLC, MapLayer *pLayer )
   { 
   for( int i=0; i < (int) m_lulcArray.GetSize(); i++ )
      m_lulcArray[ i ]->m_slcArea = 0;

   float slcArea = 0;
   for( int i=0; i < (int) pSLC->m_iduArray.GetSize(); i++ )
      {
      int idu = pSLC->m_iduArray[ i ];

      float area = 0;
      pLayer->GetData( idu, m_colArea, area );

      slcArea += area;

      int lulc = -1;
      bool ok = pLayer->GetData( idu, m_colLulc, lulc );

      if ( ok && lulc >= 0 )
         {
         int index = -1;
         BOOL ok = m_lulcIdToIndexMap.Lookup( lulc, index );

         if ( ok && index >= 0 )
            m_lulcArray[ index ]->m_slcArea += area;
         }
      }

   return slcArea;
   }
