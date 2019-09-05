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

#include "Livestock.h"
#include "EasternOntario.h"
#include <Maplayer.h>
#include <tixml.h>
#include <UNITCONV.H>
#include <misc.h>

#include <randgen\Randunif.hpp>
#include <LULCTREE.H>



extern EasternOntarioProcess *theProcess;

/////////////////////////////////////////////////////////////////////
// L I V E S T O C K    M O D E L 
/////////////////////////////////////////////////////////////////////

LivestockProcess::LivestockProcess( void )
: m_id( NITROGEN )   
, m_colLulc( -1 )
, m_colArea( -1 )
, m_colDairyAvg( -1 )
, m_colBeefAvg( -1 )
, m_colPigsAvg( -1 )
, m_colPoultryAvg( -1 )
   { }



bool LivestockProcess::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   // load table
   bool ok = LoadXml( pEnvContext, initStr );
   if ( ! ok ) 
      return false;
   
   ok = theProcess->CheckCol( pEnvContext->pMapLayer, m_colArea, "Area", TYPE_FLOAT, CC_MUST_EXIST );
   if ( ! ok ) 
      return false;

   ok = theProcess->CheckCol( pEnvContext->pMapLayer, m_colDairyAvg, "DairyAvg", TYPE_FLOAT, CC_MUST_EXIST );
   if ( ! ok ) 
      return false;

   ok = theProcess->CheckCol( pEnvContext->pMapLayer, m_colBeefAvg, "BeefAvg", TYPE_FLOAT, CC_MUST_EXIST );
   if ( ! ok ) 
      return false;

   ok = theProcess->CheckCol( pEnvContext->pMapLayer, m_colPoultryAvg, "Poultry_Av", TYPE_FLOAT, CC_MUST_EXIST );
   if ( ! ok ) 
      return false;
   
   ok = theProcess->CheckCol( pEnvContext->pMapLayer, m_colPigsAvg, "Pigs_Avg", TYPE_FLOAT, CC_MUST_EXIST );
   if ( ! ok ) 
      return false;

   //if ( pEnvContext->col >= 0 )
   //   ok = Run( pEnvContext, false );

   theProcess->m_outVarCountLivestock = 0;
   theProcess->m_outVarIndexLivestock = 0;

   //for ( int i=0; i < (int) this->m_rotationArray.GetSize(); i++ )
   //   {
   //   Rotation *pRotation = m_rotationArray.GetAt( i );
   //
   //   CString name = pRotation->m_name;
   //   name += " Area (ha)";
   //   theProcess->AddOutputVar( name, pRotation->m_totalArea, "" );
   //   theProcess->m_outVarCountCropRotation++;
   //   
   //   name = pRotation->m_name;
   //   name += " Area (%)";
   //   theProcess->AddOutputVar( name, pRotation->m_totalAreaPct, "" );
   //   theProcess->m_outVarCountCropRotation++;
   //   
   //   name = pRotation->m_name;
   //   name += " Area (% of Ag)";
   //   theProcess->AddOutputVar( name, pRotation->m_totalAreaPctAg, "" );
   //   theProcess->m_outVarCountCropRotation++;
   //   }

   return true;
   }


bool LivestockProcess::InitRun( EnvContext *pContext )
   {
   AllocateLivestock( pContext, false );

   return true;
   }


bool LivestockProcess::Run( EnvContext *pContext, bool useAddDelta )
   {
   AllocateLivestock( pContext, true );

   return true;
   }


void LivestockProcess::AllocateLivestock( EnvContext *pContext, bool useAddDelta )
   {
   // basic idea - iterate through the SLCs, calculating a contribution from each
   // of a variety of P influences.
   
   //MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   //
   //for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
   //   {
   //   int lulcA = -1;
   //   pLayer->GetData( idu, m_colLulcA, lulcA );
   //
   //   float area = 0;
   //   pLayer->GetData( idu, m_colArea, area );
   //
   //   if ( lulcA == 2 )
   //      m_totalIDUAreaAg += area;
   //
   //   int lulc = -1;
   //   pLayer->GetData( idu, m_colLulc, lulc );
   //
   //   // track areas of crops in the rotations for output
   //   // (Note that these are starting values, which are adjusted as needed below)
   //   for ( int i=0; i < (int) m_rotLulcArray.GetSize(); i++ )
   //      {
   //      if ( lulc == m_rotLulcArray[ i ] )
   //         {
   //         m_rotLulcAreaArray[ i ] += ( area / M2_PER_HA );
   //         break;
   //         }
   //      }
   //
   //   int rotationID = 0;
   //   pLayer->GetData( idu, m_colRotation, rotationID );
   //   
   //   int rotIndex = 0;
   //   pLayer->GetData( idu, m_colRotIndex, rotIndex );
   //
   //   // are we in a rotation?  If so, move through the rotation
   //   if ( rotationID > 0 )
   //      {
   //      Rotation *pRotation = NULL;
   //      BOOL found = m_rotationMap.Lookup( rotationID, pRotation ); 
   //
   //      if ( ! found || pRotation == NULL )
   //         continue;      // no valid rotation found
   //
   //      if ( lulc < 0 )   // no lulc value found
   //         continue;
   //
   //      // see where we are at in the rotation.  NOTE:  if we are in a rotation, but the 
   //      // lulc value is invalid OR the rotIndex < 0 , then randomly assign an lulc         
   //      if ( rotIndex >= (int) pRotation->m_sequenceArray.GetSize()-1 )// last item in sequence
   //         rotIndex = 0;     // then move to start of rotation
   //      else if ( rotIndex < 0 )   // not yet assigned a value in sequence
   //         {
   //         // randomly assign an lulc class in this rotation
   //         int rotLength = (int) pRotation->m_sequenceArray.GetSize();
   //         rotIndex = (int) rnRotIndex.RandValue( 0, (double) rotLength );
   //         }
   //      else
   //         rotIndex++;
   //
   //      //found = pRotation->m_sequenceMap.Lookup( lulc, index );   // false if same crops shows up twice in rotation
   //      //found = false;
   //      //for ( int i=0; i < (int) pRotation->m_sequenceArray.GetSize(); i++ )
   //      //   {
   //      //   if ( pRotation->m_sequenceArray[ i ] == lulc )
   //      //      {
   //      //      found = true;
   //      //      break;
   //      //      }
   //      //   }
   //      //
   //      //if ( ! found ) // || index < 0 )
   //      //   continue;   // rotation found, but lulc value is invalid for rotation
   //
   //      int nextLulc = pRotation->m_sequenceArray[ rotIndex ];
   //      if ( nextLulc != lulc )
   //         theProcess->UpdateIDU( pContext, idu, m_colLulc, nextLulc, useAddDelta );
   //
   //      theProcess->UpdateIDU( pContext, idu, m_colRotIndex, rotIndex, useAddDelta );
   //
   //      pRotation->m_totalArea += area / M2_PER_HA;
   //
   //      for ( int i=0; i < (int) m_rotLulcArray.GetSize(); i++ )
   //         {
   //         if ( lulc == m_rotLulcArray[ i ] )
   //            m_rotLulcAreaArray[ i ] -= ( area / M2_PER_HA );
   //
   //         if ( nextLulc == m_rotLulcArray[ i ] )
   //            m_rotLulcAreaArray[ i ] += ( area / M2_PER_HA );
   //         }
   //      }  // end of: if (rotationID > 0 )
   //
   //   else  // check for case where rotation < 0, make sure rotIndex = -1
   //      {
   //      if ( rotIndex >= 0 )
   //         theProcess->UpdateIDU( pContext, idu, m_colRotIndex, -1, useAddDelta );
   //      }
   //   }
   //
   //// update rotation summaries
   //for ( int j=0; j < (int) m_rotationArray.GetSize(); j++ )
   //   {
   //   Rotation *pRotation = m_rotationArray.GetAt( j );
   //   pRotation->m_totalAreaPct   = 100*pRotation->m_totalArea/( m_totalIDUArea / M2_PER_HA );
   //   pRotation->m_totalAreaPctAg = 100*pRotation->m_totalArea/( m_totalIDUAreaAg / M2_PER_HA );
   //
   //   //CString msg;
   //   //msg.Format( "Crop Rotation '%s': Target=%f, Realized=%4.1f", pRotation->m_name, pRotation->m_initPctArea*100, pRotation->m_totalAreaPct );
   //   //Report::LogMsg( msg );
   //   }

   return;
   }
   
   
bool LivestockProcess::LoadXml( EnvContext *pContext, LPCTSTR filename )
   {
//   // have xml string, start parsing
//   TiXmlDocument doc;
//   bool ok = doc.LoadFile( filename );
//
//   if ( ! ok )
//      {      
//      Report::ErrorMsg( doc.ErrorDesc() );
//      return false;
//      }
// 
//   // start interating through the nodes
//   TiXmlElement *pXmlRoot = doc.RootElement();  // f2r
//
//   TiXmlElement *pXmlRotations = pXmlRoot->FirstChildElement( "rotations" );
//   if ( pXmlRotations == NULL )
//      {
//      CString msg( "Crop Rotation Model: missing <rotations> element in input file " );
//      msg += filename;
//      Report::ErrorMsg( msg );
//      return false;
//      }
//
//   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
//
//   // lookup fields
//   XML_ATTR attrs[] = { // attr          type           address         isReq checkCol
//                      { "lulc_col",      TYPE_CSTRING,  &m_lulcField,       true,  CC_MUST_EXIST },
//                      { "rotation_col",  TYPE_CSTRING,  &m_rotationField,   true,  CC_MUST_EXIST },
//                      { NULL,            TYPE_NULL,     NULL,               false, 0 } };
//
//   ok = TiXmlGetAttributes( pXmlRotations, attrs, filename, pLayer );
//
//   if ( ! ok )
//      return false;
//
//   m_colLulc = pLayer->GetFieldCol( m_lulcField );
//   if ( m_colLulc < 0 )
//      {
//      CString msg;
//      msg.Format( "Crop Rotation: 'lulc_col'=%s;  The specified field was not found in the IDU coverage", m_lulcField );
//      Report::ErrorMsg( msg );
//      return false;
//      }
//
//   m_colRotation = pLayer->GetFieldCol( m_rotationField );
//   if ( m_colRotation < 0 )
//      {
//      CString msg;
//      msg.Format( "Crop Rotation: 'rotation_col'=%s;  The specified field was not found in the IDU coverage", m_rotationField );
//      Report::ErrorMsg( msg );
//      return false;
//      }
//
//   m_colLulcA = pLayer->GetFieldCol( "LULC_A" );
//   if ( m_colLulcA < 0 )
//      {
//      Report::ErrorMsg( "Crop Rotation: 'LULC_A' field was not found in the IDU coverage" );
//      return false;
//      }
//
//   // next, load lulc info
//   TiXmlElement *pXmlRotation = pXmlRotations->FirstChildElement( "rotation" );
//   if ( pXmlRotation == NULL )
//      {
//      CString msg( "Crop Rotation: Missing <rotation> tag reading file " );
//      msg += filename;
//      Report::ErrorMsg( msg );
//      return false;
//      }
//
//   while ( pXmlRotation != NULL )
//      {
//      // <rotation name="Corn/Soybean/Cereal/Alfalfa/Alfalfa" id="101" sequence="15,12,14,24,24"  init_pct_area="0.073" />
//      LPTSTR name  = NULL;
//      int    id   = 0;
//      LPTSTR rotations = 0;
//      float  initPctArea = 0;
//
//      XML_ATTR attrs[] = { // attr          type        address      isReq checkCol
//                         { "name",          TYPE_STRING,   &name,        true,  0 },
//                         { "id",            TYPE_INT,      &id,          true,  0 },
//                         { "sequence",      TYPE_STRING,   &rotations,   true,  0 },
//                         { "init_pct_area", TYPE_FLOAT,    &initPctArea, true,  0 },
//                         { NULL,            TYPE_NULL,     NULL,         false, 0 } };
//
//      bool ok = TiXmlGetAttributes( pXmlRotation, attrs, filename );
//
//      if ( ok )
//         {
//         Rotation *pRotation = new Rotation;
//
//         pRotation->m_name = name;
//         pRotation->m_rotationID = id;
//         
//         // parse sequence
//         TCHAR *buffer = new TCHAR[ lstrlen( rotations ) + 1 ];
//         lstrcpy( buffer, rotations );
//         TCHAR *next = NULL;
//         TCHAR *token = _tcstok_s( buffer, _T(","), &next );
//
//         while ( token != NULL )
//            {
//            int attrCode = atoi( token );
//
//            int index = (int) pRotation->m_sequenceArray.Add( attrCode );
//            pRotation->m_sequenceMap.SetAt( attrCode, index );    // fails if th same crops shows up twice in thw sequence
//            token = _tcstok_s( NULL, _T( "," ), &next );
//            }
//         delete [] buffer;
//
//         pRotation->m_initPctArea = initPctArea;
//
//         m_rotationMap.SetAt( id, pRotation );
//         m_rotationArray.Add( pRotation );
//         }
//    
//      pXmlRotation = pXmlRotation->NextSiblingElement( "rotation" );
//      }
//   
//   CString msg;
//   msg.Format( "Crop Rotation: Loaded %i rotations", (int) m_rotationArray.GetSize() );
//   Report::LogMsg( msg );
//
   return true;
   }


   
