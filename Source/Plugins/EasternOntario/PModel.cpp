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

#include "PModel.h"
#include "EasternOntario.h"
#include <Maplayer.h>
#include <tixml.h>


extern EasternOntarioModel *theModel;


/////////////////////////////////////////////////////////////////////
// W I L D L I F E   M O D E L 
/////////////////////////////////////////////////////////////////////

PModel::PModel( void )
   : m_id( PHOSPHORUS )   
   , m_colLulc( -1 )
   , m_colLulcB(-1)
   , m_colArea(-1)
   , m_colIROWC_P06(-1)
   , m_agOnAtRisk(0.0)
   , m_devOnAtRisk(0.0f)
   , m_livestockOnAtRisk(0.0f)
   { 
   theModel->m_outVarCountPModel = 3;
   theModel->m_outVarIndexPModel = theModel->AddOutputVar( "Ag (km2) on IROWC_P high P risk", m_agOnAtRisk, "" );
   theModel->AddOutputVar( "Urban (km2) on IROWC_P high P risk", m_devOnAtRisk, "" );
   theModel->AddOutputVar( "Livestock (km2) on IROWC_P high P risk", m_livestockOnAtRisk, "" );
   }


bool PModel::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   theModel->CheckCol( pLayer, m_colLulc, _T("LULC_A"), TYPE_LONG, CC_MUST_EXIST );//
   theModel->CheckCol( pLayer, m_colLulcB, _T("LULC_B"), TYPE_LONG, CC_MUST_EXIST );//
   theModel->CheckCol( pLayer, m_colArea, _T("AREA"), TYPE_FLOAT, CC_MUST_EXIST );//
   bool ok = theModel->CheckCol( pLayer, m_colIROWC_P06, _T("IROWC_P06"), TYPE_FLOAT, CC_MUST_EXIST );//
   if (!ok)
      return false;
   /*
   // loads table
   bool ok = LoadXml( pEnvContext, initStr );

   if ( ! ok ) 
      return false;

   ok = theModel->CheckCol( pEnvContext->pMapLayer, m_colArea, "Area", TYPE_FLOAT, CC_MUST_EXIST );
   if ( ! ok ) 
      return false;

   //BuildSLCs( (MapLayer*) pEnvContext->pMapLayer );

   if ( pEnvContext->col >= 0 )
      ok = Run( pEnvContext, false );

   return ok; */
   return true;
   }


bool PModel::InitRun( EnvContext *pContext )
   {
   return true;
   }


bool PModel::Run( EnvContext *pContext, bool useAddDelta )
   {
   // basic idea - iterate through the SLCs, calculating a contribution from each
   // of a variety of P influences.
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   int slcCount     = (int) theModel->GetSLCCount();
   float totalScore = 0;
   int iduCount     = 0;

   for ( int i=0; i < slcCount; i++ )
      {
      SLC *pSLC = theModel->GetSLC( i );
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
                  theModel->AddDelta( pContext, idu, pContext->col, slcScore );
               }
            else
               pLayer->SetData( idu, pContext->col, slcScore );
            }
         }

      totalScore += slcScore;
      }  // end of: for ( idu < iduCount )

   totalScore /= slcCount;  // average over each SLC

   pContext->rawScore = totalScore;
   pContext->score = -3.0f + ( totalScore/10 )*6;

   if ( pContext->score > 3 )
      pContext->score = 3;

   if ( pContext->score < -3 )
      pContext->score = -3;
   GetSummary( pContext );   
   return true;
   }
   

float PModel::GetSLCScore( SLC *pSLC, MapLayer *pLayer )
   {
   /*
   float slcArea = 0;
   float totalScore = 0;
   // compute pct areas for each habitat type
   for ( int i=0; i < (int) pSLC->m_iduArray.GetSize(); i++ )
      {
      int idu = pSLC->m_iduArray[ i ];
         
      // first, get habitat type
      int lulc = -1;
      pLayer->GetData( idu, m_colLulc, lulc );

      if ( lulc < 0 )
         continue;

      float area = 0;
      pLayer->GetData( idu, m_colArea, area );
      slcArea += area;

      // start getting indicies
      // P-Status components:
      //   P-Sat - crop-specific values (for forage, corn, cereals, potatos ) and for different soil types (Details???) 
      //   P-SoilTest - crop specific values (for forage, corn, cereals, potatos ) and for different soil types (Details???) 
      // P-Transport components:
      //   P-Erosion - based on RUSLE, modified for Canada (Shelton & Wall, 1998) - includes precip, topography, land use practises, and crop type
      //   P-Overland - based on a matrix, including dominant soil, slope and curve number.  Slope from SLC? Assumes "good" hydrologic condiiton.  hydro group based on Bolinder eta al 2000 
      // P-Balance components: all are % of P exported at harvest
      //   P-Excess - based on: area of pasture, field crops, veg crops, potatoes; % of SLC polygons in each regional county municipality; fertilizer recommendation by crop (CPVQ 1996); P-harvest coefficients (Can. Fert. Inst, 2001); revised yield estimates for each ag zone (Institut de la satistique du Quebec)
      //   P-Manure - kg P produce annually by livestock - #animals*k, where k=coefficient (where is this defined???)
      //   P-Fertilizer - $ spent on P in an SLC Poly (from ag census?) * (kg P sold in the province)/($ spent on fertilizers/lime) see AAFC's Strategy Policy Branch (Canadian Fertilizer Consumption, Shipments, and Trade 2001/2002 (April 2002))

      m_slcScoreArray[ P_SAT          ] = GetSat( pLayer, idu );
      m_slcScoreArray[ P_SOILTEST     ] = GetSoilTest( pLayer, idu );
      m_slcScoreArray[ P_EROSION      ] = GetErosion( pLayer, idu );
      m_slcScoreArray[ P_OVERLANDFLOW ] = GetOverlandFlow( pLayer, idu );
      m_slcScoreArray[ P_EXCESS       ] = GetExcess( pLayer, idu );
      m_slcScoreArray[ P_MANURE       ] = GetManure( pLayer, idu );
      m_slcScoreArray[ P_FERT         ] = GetFert( pLayer, idu );

      float slcScore = 0;
      for ( int i=0; i < P_COUNT; i++ )
         slcScore += m_weightArray[ i ] * m_slcScoreArray[ i ];
      }
   
   return totalScore; */
   return 0;
   }


float PModel::GetSat( MapLayer *pLayer, int idu )
	{
	return true;
   }

float PModel::GetSoilTest( MapLayer *pLayer, int idu )
	{
	return true;
   }

float PModel::GetErosion( MapLayer *pLayer, int idu )
	{
	return true;
   }

float PModel::GetOverlandFlow( MapLayer *pLayer, int idu )
	{
	return true;
   }

float PModel::GetExcess( MapLayer *pLayer, int idu )
	{
	return true;
   }

float PModel::GetManure( MapLayer *pLayer, int idu )
	{
	return true;
   }

float PModel::GetFert( MapLayer *pLayer, int idu )
	{
	return true;
   }

void PModel::GetSummary( EnvContext *pEnvContext )
   {
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
   float resultDev=0.0f; float totalArea=0.0f;float result=0.0f;float resultLive=0.0f;
   //Get initial values of area for Forest Cover, Wetlands and Impervious Area
    for ( MapLayer::Iterator i=pLayer->Begin(); i < pLayer->End(); i++ ) 
        {
        int lulcA = 0;float IROWC_P06=0.0f; float area=0.0f;int lulcB=0;
        
        pLayer->GetData( i, m_colLulc, lulcA ); 
        pLayer->GetData( i, m_colLulcB, lulcB ); 
        pLayer->GetData( i, m_colArea, area );
        totalArea+=area;
        pLayer->GetData( i, m_colIROWC_P06, IROWC_P06 );
        if (lulcA==2)//ag area
           {
           if (IROWC_P06 > 0.4f)//This represents about 20% of the total range of values for the indicator (ranges from 0 to 0.5)
              result+=area;
           }
        if (lulcA==5)// developed area
           {
           if (IROWC_P06 > 0.4f)//This represents about 20% of the total range of values for the indicator (ranges from 0 to 0.5)
              resultDev+=area;   
           }
        if (lulcB==6)//Grass/pasture (ie livestock)
           {
           if (IROWC_P06 > 0.4f)//This represents about 20% of the total range of values for the indicator (ranges from 0 to 0.5)
              resultLive+=area;   
           }

	    }
   m_agOnAtRisk=result/1000000.0f;//area of Ag land on high risk soils (km2)
   m_devOnAtRisk=resultDev/1000000.0f;//area of Ag land on high risk soils (km2)
   m_livestockOnAtRisk=resultLive/1000000.0f;//area of Ag land on high risk soils (km2)

   }

bool PModel::LoadXml( EnvContext *pContext, LPCTSTR filename )
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

   /*
   TiXmlElement *pXmlModel = pXmlRoot->FirstChildElement( "wildlife" );
   if ( pXmlModel == NULL )
      {
      CString msg( "NAHARP Wildlife Model: missing <wildlife> element in input file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   if ( pXmlModel == NULL )
      return false;

   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   // lookup fields
   LPTSTR lulcField   = NULL;
   LPTSTR lookup      = NULL;
   LPTSTR slcField    = NULL; 
   XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                      { "lulc_col",      TYPE_STRING,   &lulcField,   true,  CC_MUST_EXIST },
                      { "slc_col",       TYPE_STRING,   &slcField,    true,  CC_MUST_EXIST },
                      { "habitat_table", TYPE_STRING,   &lookup,      true,  0 },
                      { NULL,            TYPE_NULL,     NULL,         false, 0 } };

   ok = TiXmlGetAttributes( pXmlModel, attrs, filename, pLayer );

   if ( ! ok )
      return false;

   // load the table.  This initializes the Specie and HabitatUse arrays
   if ( LoadTable( lookup, lulcField, pLayer ) == false )
      return false; 

   m_colSLC = pLayer->GetFieldCol( slcField );
   if ( m_colSLC < 0 )
      {
      CString msg;
      msg.Format( "NAHARP Wildlife: 'slc_col'=%s;  The specified field was not found in the IDU coverage", slcField );
      Report::ErrorMsg( msg );
      }

   // next, load an species info
   TiXmlElement *pXmlSpps = pXmlModel->FirstChildElement( "species" );
   if ( pXmlSpps == NULL )
      {
      CString msg( "NAHARP Wildlife: Missing <species> tag reading file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   int defaultUse = 1;  // default is "all"
   const TCHAR *attrDefaultUse = pXmlSpps->Attribute( "default_use" );
   if ( attrDefaultUse != NULL && *attrDefaultUse == 'n' )
      defaultUse = 0;

   for ( INT_PTR i=0; i < m_speciesArray.GetSize(); i++ )
      m_speciesArray[ i ]->m_inUse = defaultUse ? true : false;
   
   TiXmlElement *pXmlSpp = pXmlSpps->FirstChildElement( "species" );

   while ( pXmlSpp != NULL )
      {
      LPTSTR name;
      LPTSTR commonName;
      int include = 0;
      int exclude = 0;
      LPTSTR field = NULL;

      XML_ATTR attrs[] = { // attr          type        address      isReq checkCol
                         { "name",         TYPE_STRING,   &name,       true,  0 },
                         { "common_name",  TYPE_STRING,   &commonName, false, 0 },
                         { "include",      TYPE_INT,      &include,    false, 0 },
                         { "exclude",      TYPE_INT,      &exclude,    false, 0 },
                         { NULL,           TYPE_NULL,     NULL,        false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlSpp, attrs, filename );

      if ( ok )
         {
         Specie *pSpecie = FindSpeciesFromName( name );
         
         if ( pSpecie != NULL )
            {
            if ( include )
               pSpecie->m_inUse = true;
            else if ( exclude )
               pSpecie->m_inUse = false;
            }
         else
            {
            CString msg( "NAHARP Wildlife: Specie '" );
            msg += name;
            msg += "' specified in <specie> tag not found - ignoring";
            Report::LogMsg( msg );
            }
         }

      pXmlSpp->NextSiblingElement( "specie" );
      }

   // next, load habitatUse
   TiXmlElement *pXmlHabitats = pXmlModel->FirstChildElement( "habitats" );
   
   if ( pXmlHabitats == NULL )
      {
      CString msg( "NAHARP Wildlife: Missing <habitats> tag reading file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   TiXmlElement *pXmlHabType = pXmlHabitats->FirstChildElement( "habitat_type" );

   while ( pXmlHabType != NULL )
      {
      LPTSTR name = NULL;
      XML_ATTR attrs[] = { // attr          type        address      isReq checkCol
                         { "name",         TYPE_STRING,   &name,      true,  0 },
                         { NULL,           TYPE_NULL,     NULL,       false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlHabType, attrs, filename );

      if ( ok )
         {
         HabitatType *pType = new HabitatType;
         pType->m_name  = name;
         pType->m_index = (int) m_habTypeArray.Add( pType );
         }

      pXmlHabType = pXmlHabType->NextSiblingElement( "habitat_type" );
      }

   int habSubtypeCount = 0;

   TiXmlElement *pXmlHabSubType = pXmlHabitats->FirstChildElement( "habitat_subtype" );

   while ( pXmlHabSubType != NULL )
      {
      LPTSTR name = NULL;
      LPTSTR type = NULL;
      XML_ATTR attrs[] = { // attr          type        address      isReq checkCol
                         { "name",         TYPE_STRING,   &name,      true,  0 },
                         { "type",         TYPE_STRING,   &type,      true,  0 },
                         { NULL,           TYPE_NULL,     NULL,       false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlHabSubType, attrs, filename );

      if ( ok )
         {
         HabitatType *pType = FindHabitatType( type );

         if ( pType == NULL )
            {
            CString msg;
            msg.Format( "NAHARP Wildlife: Invalid type '%s' encountered when processing subtype '%s'", type, name );
            Report::WarningMsg( msg );
            }
         else
            {
            HabitatSubType *pSubType = new HabitatSubType;
            pSubType->m_name = name;
            pSubType->m_pParent = pType;
            pSubType->m_col = m_habScoreTable.GetCol( name );

            if ( pSubType->m_col < 0 )
               {
               CString msg;
               msg.Format( "NAHARP Wildlife: Habitat subtype '%s' was not found in habitat table and will be ignored", name );
               Report::WarningMsg( msg );
               }

            pType->m_subTypeArray.Add( pSubType );
            habSubtypeCount++;
            }
         }

      pXmlHabSubType = pXmlHabSubType->NextSiblingElement( "habitat_subtype" );
      }


   TiXmlElement *pXmlHabUse = pXmlHabitats->FirstChildElement( "habitat_use" );

   while ( pXmlHabUse != NULL )
      {
      LPTSTR name = NULL;
      LPTSTR code = NULL;
      //float weight = 0;

      XML_ATTR attrs[] = { // attr          type        address      isReq checkCol
                         { "name",         TYPE_STRING,   &name,      true,  0 },
                         { "code",         TYPE_STRING,   &code,      true,  0 },
                         //{ "weight",       TYPE_FLOAT,    &weight,    false, 0 },
                         { NULL,           TYPE_NULL,     NULL,       false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlHabUse, attrs, filename );

      if ( ok )
         {
         HabitatUse *pHU = new HabitatUse;
         pHU->m_name = name;
         pHU->m_code = code[ 0 ];
         pHU->m_index = (int) AddHabitatUse( pHU );
         }

      pXmlHabUse = pXmlHabUse->NextSiblingElement( "habitat_use" );
      }

   GenerateHabUseRowMappings();

   // next, load lulc mappings
   TiXmlElement *pXmlMaps = pXmlModel->FirstChildElement( "lulc_maps" );
   
   if ( pXmlMaps == NULL )
      {
      CString msg( "NAHARP Wildlife: Missing <lulc_maps> tag reading file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   TiXmlElement *pXmlMap = pXmlMaps->FirstChildElement( "lulc_map" );

   while ( pXmlMap != NULL )
      {
      int lulc   = -1;
      LPTSTR habitat = NULL;

      XML_ATTR attrs[] = { // attr          type        address      isReq checkCol
                         { "lulc",         TYPE_INT,      &lulc,      true,  0 },
                         { "habitat",      TYPE_STRING,   &habitat,   true,  0 },
                         { NULL,           TYPE_NULL,     NULL,       false, 0 } };

      bool ok = TiXmlGetAttributes( pXmlMap, attrs, filename );

      if ( ok )
         {
         // find corresponding column
         HabitatSubType *pHabSubType = FindHabitatSubType( habitat );

         if ( pHabSubType == NULL )
            {
            CString msg( "NAHARP Wildlife: Invalid lulc_map - habitat '" );
            msg += habitat;
            msg += "' not found in habitat table - this mapping will be ignored";
            Report::WarningMsg( msg );
            }
         else
            m_lulcToHabSubTypeMap.SetAt( lulc, pHabSubType ); 
         }

      pXmlMap = pXmlMap->NextSiblingElement( "lulc_map" );
      }

   int inUseSppCount = 0;
   for ( INT_PTR i=0; i < m_speciesArray.GetSize(); i++ )
      if ( m_speciesArray[ i ]->m_inUse )
         inUseSppCount++;   

   CString msg;
   msg.Format( "NAHARP Wildlife: Loaded %i rows, %i species (%i in use), %i habitat types (%i subtypes), %i habitat uses, %i lulc mappings from %s", 
      m_habScoreTable.GetRowCount(), (int) m_speciesArray.GetSize(), inUseSppCount, (int) m_habTypeArray.GetSize(), habSubtypeCount, 
      (int) m_habUseArray.GetSize(), (int) m_lulcToHabSubTypeMap.GetCount(), m_habScoreTable.m_path );
   Report::LogMsg( msg );

   */
   return true;
   }

