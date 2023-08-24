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
// FlameLenDisturbHandler.cpp : Defines the initialization routines for the DLL.
#include "stdafx.h"
#include <afxstr.h>
#include <afx.h>
#pragma hdrstop
#include <iostream>
#include "FlameLenDisturbHandler.h"
#include <EnvExtension.h>
#include <EnvContext.h>
#include <Maplayer.h>
#include <map.h>
#include <DbTable.h>
#include <Vdataobj.h>
#include <tixml.h>
#include <algorithm>
#include <vector>
#include <afxtempl.h>
#include <PathManager.h>
#include "AlgLib\ap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern "C" _EXPORT EnvExtension* Factory(EnvContext *pContext)
   {
   return (EnvExtension*) new FlameLenDisturbHandler;
   }


//#define MAKELONGER(a, b, c, d) \
// (unsigned int)((((unsigned int)(a)) << 24) | (((unsigned int)(b)) << 16) | (((unsigned int)(c)) << 8) | (((unsigned int)(d))))

__int64 GetKey(int VegClass, int Region, int Pvt, int Variant)
{
	__int64
		key = (__int64)VegClass * 100000 + (__int64)Region * 1000 + (__int64)Pvt * 10 + (__int64)Variant;
	return key;
}

//char filenamej[] = "C:\\Envision\\StudyAreas\\Lebanon\\vegclass_fire_vddt_transitions.csv";

FlameLenDisturbHandler::FlameLenDisturbHandler()
: EnvModelProcess()
, m_colDisturb ( -1 )
, m_colFlameLen ( -1 )
, m_colVegClass ( -1 )
, m_colVariant ( -1 )
, m_colTSD ( -1 )
, m_colPVT ( -1 )
, m_colRegion ( -1 )
, m_colArea( -1 )
, m_pvt ( 0 )
, m_vegClass ( -1 )
, m_variant ( 1 )
, m_region ( -1 )
, m_disturb ( -99 )
, m_surfaceFireAreaHa( 0 )
, m_surfaceFireAreaPct( 0 )
, m_surfaceFireAreaCumHa( 0 )
, m_lowSeverityFireAreaHa( 0 )
, m_lowSeverityFireAreaPct( 0 )
, m_lowSeverityFireAreaCumHa( 0 )
, m_highSeverityFireAreaHa( 0 )
, m_highSeverityFireAreaPct( 0 )   
, m_highSeverityFireAreaCumHa( 0 ) 
, m_standReplacingFireAreaHa( 0 ) 
, m_standReplacingFireAreaPct( 0 ) 
, m_standReplacingFireAreaCumHa( 0 )
, m_colPotentialDisturb(-1)
, m_colPotentialFlameLen(-1)
 {
 m_vegFireMap.InitHashTable( 10000 );
 }; // end of FlameLenthToDisturb constructor
 
FlameLenDisturbHandler::~FlameLenDisturbHandler()
   {
   
   } // end of FlameLenthToDisturb destructor

//======================================================
// Disturbance generator
//======================================================

bool FlameLenDisturbHandler::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   const MapLayer *pLayer = pEnvContext->pMapLayer;
   
   bool ok = LoadXml( initStr ); // get input file names and input variable

   if ( ! ok )
      {
      CString msg( "FlameLenDisturbHandler: Unable to find FlameLenDisturbHandler's .xml init file" );
     
      Report::ErrorMsg( msg );
      return FALSE;
      }   
    
   // check and store relevant columns
   CheckCol( pLayer, m_colVegClass, "VEGCLASS", TYPE_INT, CC_MUST_EXIST );
   CheckCol( pLayer, m_colDisturb,  "DISTURB" , TYPE_INT, CC_MUST_EXIST );
   CheckCol( pLayer, m_colVariant,  "VARIANT",   TYPE_INT,   CC_MUST_EXIST );
   CheckCol( pLayer, m_colFlameLen, "FLAMELEN",  TYPE_FLOAT, CC_MUST_EXIST );
   CheckCol( pLayer, m_colPVT,      "PVT" ,     TYPE_INT, CC_MUST_EXIST );
   CheckCol( pLayer, m_colRegion,   "REGION" ,  TYPE_INT, CC_MUST_EXIST );
   CheckCol( pLayer, m_colArea,     "AREA",     TYPE_FLOAT, CC_MUST_EXIST );
   CheckCol( pLayer, m_colTSD,      "TSD" ,     TYPE_INT, CC_MUST_EXIST );

	m_colPotentialDisturb = pLayer->GetFieldCol("PDISTURB");
	m_colPotentialFlameLen = pLayer->GetFieldCol("PFlameLen");
   

   // read fire lookup file
   CString path;
   if (PathManager::FindPath(m_fireLookupFile.firelookup_filename, path) < 0) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format("FlameLenDisturbHandler: Input file %s not found!", (LPCTSTR)m_fireLookupFile.firelookup_filename);
      Report::ErrorMsg(msg);
      return false;
      }

   VDataObj data(U_UNDEFINED);
   int rows = data.ReadAscii( path );
   
   if ( rows <= 0 )
      {
      CString msg( "FlameLenDisturbHandler initialization error:  Unable to read  file " );
      msg += path;
      Report::ErrorMsg( msg );
      return FALSE;
      }

   int vegclass_col        = data.GetCol( "VEGCLASS");
	int region_col				= data.GetCol( "REGION");
	int pvt_col					= data.GetCol( "PVT");
   int variant_col         = data.GetCol( "VARIANT");	
   int lcpfuelmod_col      = data.GetCol( "LCP_FUEL_MODEL");	
   int lcpcnpycov_col      = data.GetCol( "LCP_CNPY_COV");	
   int lcpcnpyht_col       = data.GetCol( "LCP_CNPY_HT");	
   int lcpcnpybsht_col     = data.GetCol( "LCP_CNPY_BS_HT");	
   int lcpcnpyblkdns_col   = data.GetCol( "LCP_CNPY_BLK_DNS");	
   int nofire_col          = data.GetCol( "NO_FIRE");	
   int surface_col         = data.GetCol( "SURFACE_FIRE");	
   int mixlow_col          = data.GetCol( "MIXED_SEVERITY_BURN_LOW");	
   int mixhigh_col         = data.GetCol( "MIXED_SEVERITY_BURN_HIGH");	
   int standrpl_col        = data.GetCol( "STAND_REPLACING_FIRE");	
   int notrans_col         = data.GetCol( "VEGCLASS_NO_FIRE_TRANSITION");	
   int surfacetrans_col    = data.GetCol( "VEGCLASS_SURFACE_FIRE_TRANSITION");	
   int mixlowtrans_col     = data.GetCol( "VEGCLASS_MIXED_SEVERITY_BURN_LOW_TRANSITION");	
   int mixhightrans_col    = data.GetCol( "VEGCLASS_MIXED_SEVERITY_BURN_HIGH_TRANSITION");	
   int standrpltrans_col   = data.GetCol( "VEGCLASS_STAND_REPLACING_FIRE_TRANSITION");

   if ( vegclass_col    < 0 || lcpcnpybsht_col   < 0 ||  mixhigh_col   < 0 || mixhightrans_col  < 0 || 
        variant_col     < 0 || lcpcnpyblkdns_col < 0 ||  standrpl_col  < 0 || standrpltrans_col < 0 || 
        lcpfuelmod_col  < 0 || nofire_col        < 0 ||  notrans_col   < 0 || lcpcnpycov_col    < 0 || 
        surface_col     < 0 || surfacetrans_col  < 0 ||  lcpcnpyht_col < 0 || mixlow_col        < 0 ||  
		  mixlowtrans_col < 0 || region_col < 0 || pvt_col < 0 )             
      {
      CString msg;
      msg.Format("FlameLenDisturbHandler::Init One or more column headings are incorrect in fire lookup file\n");
      Report::ErrorMsg( msg );
      return false;
      }
                              
   for ( int i=0; i < rows; i++ )
      {
      FIRE_STATE *pFS = new FIRE_STATE;
      data.Get( vegclass_col,      i, pFS->vegClass );
      data.Get( variant_col,       i, pFS->variant );
		data.Get( region_col,        i, pFS->region );
		data.Get( pvt_col,			  i, pFS->pvt );
      data.Get( nofire_col,        i, pFS->none );
      data.Get( surface_col,       i, pFS->groundFire );
      data.Get( mixlow_col,        i, pFS->mixedSeverityFire1 );
      data.Get( mixhigh_col,       i, pFS->mixedSeverityFire2 );
      data.Get( standrpl_col,      i, pFS->severeFire ); 
	   data.Get( surfacetrans_col,  i, pFS->surfaceToVeg );
      data.Get( mixlowtrans_col,   i, pFS->mixedSeverityFire1ToVeg );
      data.Get( mixhightrans_col,  i, pFS->mixedSeverityFire2ToVeg );
      data.Get( standrpltrans_col, i, pFS->severeToVeg );
      
	   m_fireSeverityArray.Add( pFS );
      }

   for ( int i=0; i < m_fireSeverityArray.GetSize(); i++ )
      {
      if ( m_fireSeverityArray[ i ]->vegClass > 0 )
         {
        // LONG vegPvtRegionVariant = MAKELONGER( m_fireSeverityArray[ i ]->vegClass, m_fireSeverityArray[ i ]->pvt,
		//		m_fireSeverityArray[ i ]->region, m_fireSeverityArray[ i ]->variant );
		  __int64 vegPvtRegionVariant = GetKey(m_fireSeverityArray[i]->vegClass, m_fireSeverityArray[i]->region,
			  m_fireSeverityArray[i]->pvt, m_fireSeverityArray[i]->variant);
			m_vegFireMap.SetAt( vegPvtRegionVariant, i );
			//ATLTRACE2("Added to FlameLenDisturbHandler::m_vegFireMap: %I64d, %d\n", vegPvtRegionVariant, i);
         }
      }

   AddOutputVar( _T( "Surface Fire Area (ha)" ),                        m_surfaceFireAreaHa,    _T("") );
   AddOutputVar( _T( "Surface Fire Area (%)" ),                         m_surfaceFireAreaPct,   _T("") );
   AddOutputVar( _T( "Surface Fire Area (Cumulative - ha)" ),           m_surfaceFireAreaCumHa, _T("") );

   AddOutputVar( _T( "Low Severity Fire Area (ha)" ),                   m_lowSeverityFireAreaHa,    _T("") );
   AddOutputVar( _T( "Low Severity Fire Area (%)" ),                    m_lowSeverityFireAreaPct,   _T("") );
   AddOutputVar( _T( "Low Severity Fire Area (Cumulative - ha)" ),      m_lowSeverityFireAreaCumHa, _T("") );

   AddOutputVar( _T( "High Severity Fire Area (ha)" ),                  m_highSeverityFireAreaHa,    _T("") );
   AddOutputVar( _T( "High Severity Fire Area (%)" ),                   m_highSeverityFireAreaPct,   _T("") );
   AddOutputVar( _T( "High Severity Fire Area (Cumulative - ha)" ),     m_highSeverityFireAreaCumHa, _T("") );

   AddOutputVar( _T( "Stand Replacing Fire Area (ha)" ),                m_standReplacingFireAreaHa,    _T("") );
   AddOutputVar( _T( "Stand Replacing Fire Area (%)" ),                 m_standReplacingFireAreaPct,   _T("") );
   AddOutputVar( _T( "Stand Replacing Fire Area (Cumulative - ha)" ),   m_standReplacingFireAreaCumHa, _T("") );
  
   return TRUE;
   }

bool FlameLenDisturbHandler::InitRun( EnvContext *pContext, bool useInitSeed )
   {
   m_surfaceFireAreaHa = 0;
   m_surfaceFireAreaPct = 0;
   m_surfaceFireAreaCumHa = 0;
   
   m_lowSeverityFireAreaHa = 0;
   m_lowSeverityFireAreaPct = 0;
   m_lowSeverityFireAreaCumHa = 0;
   
   m_highSeverityFireAreaHa = 0;
   m_highSeverityFireAreaPct = 0;   
   m_highSeverityFireAreaCumHa = 0; 
   
   m_standReplacingFireAreaHa = 0; 
   m_standReplacingFireAreaPct = 0; 
   m_standReplacingFireAreaCumHa = 0;

   return TRUE;
   }

bool FlameLenDisturbHandler::Run( EnvContext *pEnvContext )
   {
   m_surfaceFireAreaHa = 0;
   m_surfaceFireAreaPct = 0;
   
   m_lowSeverityFireAreaHa = 0;
   m_lowSeverityFireAreaPct = 0;
   
   m_highSeverityFireAreaHa = 0;
   m_highSeverityFireAreaPct = 0;   
   
   m_standReplacingFireAreaHa = 0; 
   m_standReplacingFireAreaPct = 0; 
     
   MapLayer *pMapLayer = ( MapLayer* ) pEnvContext->pMapLayer;

   int current_year = pEnvContext->currentYear; 

   int vegCount = (int) this->m_vegFireMap.GetSize();

   for ( MapLayer::Iterator idu = pMapLayer->Begin( ); idu != pMapLayer->End(); idu++ )
      {
      float flameLen = -1.f;
		float potentialFlameLen = -1.f;
      int newvegclass = -1;
	   int newvariant = -1;
      float area;
      int disturb = -99;
		int potentialDisturb = -99;
      
      if ( m_colFlameLen != -1 )
         pMapLayer->GetData( idu, m_colFlameLen, flameLen );

		if ( m_colPotentialFlameLen != -1 )
			pMapLayer->GetData(idu, m_colPotentialFlameLen, potentialFlameLen);

      pMapLayer->GetData( idu, m_colVegClass, m_vegClass );
		pMapLayer->GetData( idu, m_colPVT, m_pvt );
		pMapLayer->GetData( idu, m_colRegion, m_region );
      pMapLayer->GetData( idu, m_colVariant, m_variant);
      pMapLayer->GetData( idu, m_colArea, area);
      
		if (alglib::fp_greater(flameLen, 0.0001f))		
			{
			int index = -1;      // this is a row index in the veg/Fire lookup table
			
			//LONG vegPvtRegionVariant = MAKELONGER( (short) m_vegClass, (short) m_pvt,
			//	(short) m_region, (short) m_variant );
			__int64 vegPvtRegionVariant = GetKey(m_vegClass, m_region,
				m_pvt, m_variant);

			bool result = m_vegFireMap.Lookup( vegPvtRegionVariant, index );

			if ( result == FALSE )
				{     // mapping not found in table, so skip to next IDU
			   CString msg;
				msg.Format("FlameLenDisturbHandler: Unable to type fire severity based on flame length, m_vegClass = %d , m_region = %d, , m_pvt = %d, , m_variant = %d", m_vegClass, m_region, m_pvt, m_variant );
            Report::Log(msg);
				continue;
				}
			// mapping found, interpret...
			
			FIRE_STATE *pFS = m_fireSeverityArray[ index ];

			// check fire states - first is NONE
			if ( pFS->none > 0 && flameLen <= pFS->none )
            ;

			else if ( pFS->groundFire > 0 && flameLen <= pFS->groundFire )
			   {
				disturb = SURFACE_FIRE;

			   m_surfaceFireAreaHa    += area / 10000;  // assume coverage is in meters!!!
			   m_surfaceFireAreaPct   += area / (10000 * pMapLayer->GetTotalArea());
			   m_surfaceFireAreaCumHa += area / 10000;

			   }
			else if ( pFS->mixedSeverityFire1 > 0 && flameLen <= pFS->mixedSeverityFire1 )
			   {
				disturb = LOW_SEVERITY_FIRE;

			   m_lowSeverityFireAreaHa    += area / 10000;  // assume coverage is in meters!!!
			   m_lowSeverityFireAreaPct   += area / (10000 * pMapLayer->GetTotalArea());
			   m_lowSeverityFireAreaCumHa += area / 10000;
			   }
			else if ( pFS->mixedSeverityFire2 > 0 && flameLen <= pFS->mixedSeverityFire2 )
			   {				
				disturb = HIGH_SEVERITY_FIRE;

			   m_highSeverityFireAreaHa    += area / 10000;  // assume coverage is in meters!!!
			   m_highSeverityFireAreaPct   += area / (10000 * pMapLayer->GetTotalArea());
			   m_highSeverityFireAreaCumHa += area / 10000;
			   }
			else if ( pFS->severeFire > 0 && flameLen <= pFS->severeFire )
			   {
				disturb = STAND_REPLACING_FIRE;

			   m_standReplacingFireAreaHa    += area / 10000;  // assume coverage is in meters!!!
			   m_standReplacingFireAreaPct   += area / (10000 * pMapLayer->GetTotalArea());
			   m_standReplacingFireAreaCumHa += area / 10000;
			   }				
			}	
		
		m_disturb = disturb;

		if (alglib::fp_greater(potentialFlameLen, 0.0001f))
			{
			int index = -1;      // this is a row index in the veg/Fire lookup table

			//LONG vegPvtRegionVariant = MAKELONGER( (short) m_vegClass, (short) m_pvt,
			//	(short) m_region, (short) m_variant );
			__int64 vegPvtRegionVariant = GetKey(m_vegClass, m_region,
				m_pvt, m_variant);

			bool result = m_vegFireMap.Lookup(vegPvtRegionVariant, index);

			if (result == FALSE)     // mapping not found in table, so skip to next IDU
				continue;

			// mapping found, interpret...

			FIRE_STATE *pFS = m_fireSeverityArray[index];

			// check fire states - first is NONE
			if (pFS->none > 0 && potentialFlameLen <= pFS->none);

			else if (pFS->groundFire > 0 && potentialFlameLen <= pFS->groundFire)
				{		
				potentialDisturb = SURFACE_FIRE;
				}
			else if (pFS->mixedSeverityFire1 > 0 && potentialFlameLen <= pFS->mixedSeverityFire1)
				{
				potentialDisturb = LOW_SEVERITY_FIRE;
				}
			else if (pFS->mixedSeverityFire2 > 0 && potentialFlameLen <= pFS->mixedSeverityFire2)
				{
				potentialDisturb = HIGH_SEVERITY_FIRE;
				}
			else if (pFS->severeFire > 0 && potentialFlameLen <= pFS->severeFire)
				{
				potentialDisturb = STAND_REPLACING_FIRE;
				}
			}

		if (m_disturb > 0)
			{
			UpdateIDU(pEnvContext, idu, m_colDisturb, m_disturb, ADD_DELTA);
			UpdateIDU(pEnvContext, idu, m_colTSD, 0, ADD_DELTA);
			}

		if (potentialDisturb > 0 && m_colPotentialDisturb != -1 )
			{
			UpdateIDU(pEnvContext, idu, m_colPotentialDisturb, potentialDisturb, ADD_DELTA);
			}
		
      }// end IDU loop

   return TRUE;
   }

bool FlameLenDisturbHandler::LoadXml( LPCTSTR filename )
   {
   // start parsing input file
   TiXmlDocument doc;
   
   bool ok = doc.LoadFile( filename );

   bool loadSuccess = true;

   if ( ! ok )
      {
      CString msg;
      msg.Format("Error reading input file %s:  %s", filename, doc.ErrorDesc() );
      Report::ErrorMsg( msg );
      return false;
      }
   
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // <integrator
     
   TiXmlElement *pXmlfireLookupFiles = pXmlRoot->FirstChildElement( "fireLookupFiles" );
   
   if ( pXmlfireLookupFiles == NULL )
      {
      CString msg( "FlameLenDisturbHandler::LoadXml:Unable to find <fireLookupFiles> tag reading " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   TiXmlElement *pXmlfireLookupFile = pXmlfireLookupFiles->FirstChildElement( "fireLookupFile" );
  
      XML_ATTR setAttrs[] = {
         // attr                     type           address                                     isReq  checkCol        
         { "fireLookupFile",         TYPE_CSTRING,  &(m_fireLookupFile.firelookup_filename),    true,   0 },  
         { NULL,                     TYPE_NULL,     NULL,                                       false,  0 } };

      ok = TiXmlGetAttributes( pXmlfireLookupFile, setAttrs, filename, NULL );
     
   return true;
   }






