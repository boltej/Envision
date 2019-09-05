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

#include "Wildlife.h"
#include "F2R.h"
#include <Maplayer.h>
#include <tixml.h>


extern F2RProcess *theProcess;

const int HAB_USE_COL    = 5;
const int START_YEAR_COL = 2;
const int END_YEAR_COL   = 3;


/////////////////////////////////////////////////////////////////////
// W I L D L I F E   M O D E L 
/////////////////////////////////////////////////////////////////////

WildlifeModel::WildlifeModel( void )
   : m_id( WILDLIFE )
   , m_habScoreTable(U_UNDEFINED)
   , m_colLulc( -1 )
   , m_score( 0 )
   { }


bool WildlifeModel::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   // loads table
   bool ok = LoadXml( pEnvContext, initStr );

   if ( ! ok ) 
      return false;

   // add output variables and IDU columns for each group
   for ( int i=0; i < (int) m_groupArray.GetSize(); i++ )
      {
      SppGroup *pGroup = m_groupArray[ i ];

      CString label = "Wildlife.";
      label += pGroup->m_name;
      if ( i == 0 )
         theProcess->m_outVarIndexWildlife = theProcess->AddOutputVar( label, pGroup->m_habCapacity, label );
      else
         theProcess->AddOutputVar( label, pGroup->m_habCapacity, label );
      
      // generate IDU column if it doesn't already exist
      TCHAR field[ 16 ];
      _tcsncpy_s( field, pGroup->m_name, 12 );

      TCHAR *p = field;    // replace blanks with underscores
      while ( *p != NULL ) { if ( *p == ' ' ) *p = '_'; p++; }
      
      ok = theProcess->CheckCol( pEnvContext->pMapLayer, pGroup->m_col, field, TYPE_FLOAT, CC_AUTOADD );
      if ( ! ok ) 
         pGroup->m_col = -1;
      }
      
   theProcess->m_outVarCountWildlife = (int) m_groupArray.GetSize();

   ok = Run( pEnvContext, false );
   
   return ok;
   }


bool WildlifeModel::InitRun( EnvContext *pContext )
   {
   return true;
   }


bool WildlifeModel::Run( EnvContext *pContext, bool useAddDelta )
   {
   // basic idea - iterate through the SLCs, calculating a score for each inUse spp. 
   // Scoring is based on a combination of the various use types
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   int slcCount     = (int) theProcess->GetSLCCount();
   int sppCount     = GetSpeciesCount();
   int habUseCount  = (int) m_habUseArray.GetSize(); 
   int iduCount     = 0;

   int inUseSppCount = 0;
   for ( int i=0; i < sppCount; i++ )
      {
      // check starting year
      Specie *pSpecie = m_speciesArray[ i ];

      if ( pSpecie->IsInUse( pContext->currentYear ) )
         inUseSppCount++;
      }

   for ( int j=0; j < (int) m_groupArray.GetSize(); j++ )
      m_groupArray[ j ]->m_habCapacity = 0;
      
   float totalArea = 0; 
   for ( int i=0; i < slcCount; i++ )        // iterate through the SLCs
      {
      SLC *pSLC = theProcess->GetSLC( i );

      // get the scores for each group for this SLC
      GetSLCScores( pSLC, pLayer, pContext->currentYear );

      for ( int j=0; j < (int) m_groupArray.GetSize(); j++ )
         {
         SppGroup *pGroup = m_groupArray[ j ];         
         pGroup->m_habCapacity += pSLC->m_area * pGroup->m_slcHabCapacity;  // m_slcHabCapacity is 0-1 value.  Accumulate this information a
         ASSERT( pGroup->m_habCapacity >= 0 && pGroup->m_habCapacity < 10000000000 );
         }

      totalArea += pSLC->m_area;
      }  // end of: for each SLC

   // normalize score for each group (these are area weighted by SLCs)
   float totalScore = 0;
   for ( int j=0; j < (int) m_groupArray.GetSize(); j++ )
      {
      SppGroup *pGroup = m_groupArray[ j ];
      pGroup->m_habCapacity /= totalArea;   // m_habCapacity is set in GetSLCScore().  This makes this an area weighted average across SLCs

      totalScore += pGroup->m_habCapacity;  // this is the sum across groups
      }
 
   totalScore /= m_groupArray.GetSize();   // make this the average total score across groups
   
   pContext->rawScore = totalScore;
   pContext->score = -3.0f + ( totalScore/3 )*6;

   if ( pContext->score > 3 )
      pContext->score = 3;

   if ( pContext->score < -3 )
      pContext->score = -3;

   m_score = totalScore;
   return true;
   }
   

void WildlifeModel::GetSLCScores( SLC *pSLC, MapLayer *pLayer, int year )
   {
   // For the given SLC, species-specific habitat availability (SSHA) ( is calculated for breeding 
   // and feeding requirements, by generating a weighted average of habitat use based on the
   // relative proportion of cover types used and the value of that habitat to the species as
   // follows:
   // for each slc:
   //    {
   //    for each habitat type: calculate the percent of the slc area for each habitat 
   //    for each species:
   //        for each habitat type:
   //        {
   //        get the species usage for the habitat type 
   //        compute the SSHA score for that habitat type and store it
   //        }
   //    once we have the species-specific scores, compute a "habitat cacity"  for the SLC as the
   //    average of the species-specific scores
  
   // basic initialization
   int habTypeCount = (int) m_habTypeArray.GetSize();
   int habUseCount  = (int) m_habUseArray.GetSize();
   int sppCount     = (int) m_speciesArray.GetSize();
   int iduCount     = (int) pSLC->m_iduArray.GetSize();

   // initialize temporary storage vlaues for all HabitatTypes
   for ( int i=0; i < habTypeCount; i++ )
      {
      HabitatType *pType = m_habTypeArray[ i ];
      pType->m_slcPctArea = 0;
      pType->m_slcHabUseValue = 0;
      }

   // initial temporary values for species
   for ( int i=0; i < sppCount; i++ )
      m_speciesArray[ i ]->m_slcAlreadySeen = false;

   for ( int i=0; i < (int) m_groupArray.GetSize(); i++ ) 
      {
      m_groupArray[ i ]->m_slcHabCapacity  = 0;
      m_groupArray[ i ]->m_slcUsedSppCount = 0;
      }

   // compute pct areas for each habitat type.  These are stored in global HabitatType array
   for ( int i=0; i < iduCount; i++ )
      {
      int idu = pSLC->m_iduArray[ i ];
         
      // first, get habitat type
      int lulc = -1;
      pLayer->GetData( idu, m_colLulc, lulc );

      if ( lulc < 0 )
         continue;

      // get the column in the habitat score table corresponding to this lulc class
      HabitatSubType *pHabSubType = NULL;
      BOOL ok = m_lulcToHabSubTypeMap.Lookup( lulc, pHabSubType );
      
      if ( ! ok || pHabSubType == NULL || pHabSubType->m_col < 0 )
         continue;

      //  remember slc area for this habitat type
      HabitatType *pType = pHabSubType->m_pParent;

      float area = 0;
      pLayer->GetData( idu, theProcess->m_colArea, area );

      pType->m_slcPctArea += area;     // accumulate areas for this habitat type
      }  // end of: for ( i < m_iduArray.GetSize() )

   // have areas, normalize to percent
   for ( int i=0; i < habTypeCount; i++ )
      m_habTypeArray[ i ]->m_slcPctArea /= pSLC->m_area;      

   // now, for each species, compute a species-specific habitat availability score
   // to do this, we need to determine for the area-weighted Habitat Use Value (HUV)
   // for the species by iterating through the IDU's.  For each IDU, get the habScore 
   // from the habitat table.  depending on the score (primary, secondary, tertiary), 
   // accumlate the appropriate weighting
   int usedSppCount = 0;
  
   for ( int j=0; j < sppCount; j++ )
      {
      Specie *pSpecie = m_speciesArray[ j ];

      if ( pSpecie->IsInUse( year ) ) 
         {
         usedSppCount++;
         float usedSppArea = 0;      // area within IDU 
      
         for ( int idu=0; idu < iduCount; idu++ )
            {
            // get the idu's lulc
            int lulc = 0;
            pLayer->GetData( idu, m_colLulc, lulc );

            float area = 0;
            pLayer->GetData( idu, theProcess->m_colArea, area );

            // get the corresponding habitat subtype
            HabitatSubType *pHabSubType = NULL;
            BOOL ok = m_lulcToHabSubTypeMap.Lookup( lulc, pHabSubType );
      
            if ( ! ok || pHabSubType == NULL || pHabSubType->m_col < 0 )
               continue;

            // get corresponding type
            HabitatType *pType = pHabSubType->m_pParent;
            usedSppArea += area;  // This assumes that we only count the area of an IDU if the species uses it
      
            // for each habitat use, get corresponding score for this species by looking up in table
            // Note:  typically, there will be two habitat types - 1) feeding, and 2) breeding
            float sppHabUseValue = 0;   // area weighted average for the slc
            for ( int k=0; k < habUseCount; k++ )
               {
               int habUseRow = pSpecie->m_habUseRowArray[ k ];
               if ( habUseRow > 0 )   // is there a row entry for this use for this specie?
                  {
                  int habUse = -1;
                  bool ok = m_habScoreTable.Get( pHabSubType->m_col, habUseRow, habUse );
                  if ( ok )
                     {
                     switch( habUse )
                        {
                        case 1:     // primary use
                           sppHabUseValue += 1.0f * area;
                           break;

                        case 2:     // secondary use
                           sppHabUseValue += 0.75f * area;
                           break;

                        case 3:     // tertiary use
                           sppHabUseValue += 0.25f * area;
                        }
                     }
                  }
               }  // end of: for each habUse

            // add on to accumulating habitat use value.  Units of HUV values are effective m3 at this point for all spp
            if ( habUseCount > 0 )
               pType->m_slcHabUseValue += sppHabUseValue / habUseCount;   // this is the area-multiplied avg hab score 
            }  // end of: for each IDU

         // done iterating through IDUs (but still in species loop).
         // at this point, for each habitat type, we've determined pct SLC area 
         // and the area- and use type-normalized HUV value 
         // accumulate habitat capacity scores for each sppGroup
         for ( int i=0; i < habTypeCount; i++ )
            {
            HabitatType *pType = m_habTypeArray[ i ];
            if ( usedSppArea > 0 )
               pType->m_slcHabUseValue /= usedSppArea;    // normalize HUV's to slcArea (these are area-weighted)
            }                                          // units are now portion of SLC (0-1) with effective habitat

         // update sppGroups -
         for ( int k=0; k < (int) m_groupArray.GetSize(); k++ ) 
            {
            SppGroup *pGroup = m_groupArray[ k ];

            // is the current species in this group?
            if ( pGroup->IsSppInGroup( pSpecie ) )
               {
               pGroup->m_slcUsedSppCount++;
               
               // accumulate habitat capacity values for each each habitat type, normalized by area in that hab type
               for ( int i=0; i < habTypeCount; i++ )
                  {
                  HabitatType *pType = m_habTypeArray[ i ];
                  pGroup->m_slcHabCapacity += pType->m_slcHabUseValue * pType->m_slcPctArea;
                  }
               }
            }
         }  // end of: if ( spp is in use )
      }  // end of: for( j < sppCount )

   // normalize each group habitat capacity based on number of species in group
   for ( int i=0; i < (int) m_groupArray.GetSize(); i++ ) 
      {
      SppGroup *pGroup = m_groupArray[ i ];

      if ( pGroup->m_slcUsedSppCount > 0 )
         pGroup->m_slcHabCapacity /= pGroup->m_slcUsedSppCount;   // this is the average score across spp (0-1)
      }
   }


Specie *WildlifeModel::FindSpeciesFromName( LPCTSTR name ) 
   {
   for ( int i=0; i < (int) m_speciesArray.GetSize(); i++ ) 
      {
      Specie *pSpecie = m_speciesArray[ i ];
      
      if ( pSpecie->m_name.CompareNoCase( name ) == 0 ) 
         return m_speciesArray[ i ];
      }

   return NULL;
   }


HabitatType *WildlifeModel::FindHabitatType( LPCTSTR name )
   {
   for ( int i=0; i < (int) m_habTypeArray.GetSize(); i++ ) 
      if ( m_habTypeArray[ i ]->m_name.CompareNoCase( name ) == 0 ) 
         return m_habTypeArray[ i ];
   return NULL;
   }
   

HabitatSubType *WildlifeModel::FindHabitatSubType( LPCTSTR name )
   {
   for ( int i=0; i < (int) m_habTypeArray.GetSize(); i++ )
      {
      HabitatType *pHabType = m_habTypeArray[ i ];

      for ( int j=0; j < (int) pHabType->m_subTypeArray.GetSize(); j++ )
         {
         if ( pHabType->m_subTypeArray[ j ]->m_name.CompareNoCase( name ) == 0 ) 
            return pHabType->m_subTypeArray[ j ];
         }
      }

   return NULL;
   }


bool WildlifeModel::LoadXml( EnvContext *pContext, LPCTSTR filename )
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
   LPTSTR lookupTable = NULL;
   LPTSTR groupsTable = NULL;
   LPTSTR slcField    = NULL; 
   XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                      { "lulc_col",      TYPE_STRING,   &lulcField,   true,  CC_MUST_EXIST },
                      { "habitat_table", TYPE_STRING,   &lookupTable, true,  0 },
                      { "groups_table",  TYPE_STRING,   &groupsTable, true,  0 },
                      { NULL,            TYPE_NULL,     NULL,         false, 0 } };

   ok = TiXmlGetAttributes( pXmlModel, attrs, filename, pLayer );

   if ( ! ok )
      return false;

   // load the habitat scores table.  This initializes the Specie and HabitatUse arrays
   if ( LoadHabitatTable( lookupTable, lulcField, pLayer ) == false )
      return false; 

   // load the groups table.  This populates the SppGroups array
   if ( LoadGroupsTable( groupsTable ) == false )
      return false; 
   
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
            Report::Log( msg );
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

      XML_ATTR attrs[] = { // attr             type          address      isReq checkCol
                         { "lulc",             TYPE_INT,      &lulc,      true,  0 },
                         { "habitat_subtype",  TYPE_STRING,   &habitat,   true,  0 },
                         { NULL,               TYPE_NULL,     NULL,       false, 0 } };

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
   msg.Format( "NAHARP Wildlife: Loaded %i rows, %i species (%i in use), %i habitat types (%i subtypes), %i habitat uses, %i lulc mappings from %s, %i groups", 
      m_habScoreTable.GetRowCount(), (int) m_speciesArray.GetSize(), inUseSppCount, (int) m_habTypeArray.GetSize(), habSubtypeCount, 
      (int) m_habUseArray.GetSize(), (int) m_lulcToHabSubTypeMap.GetCount(), m_habScoreTable.m_path, (int) m_groupArray.GetSize() );
   Report::Log( msg );


   return true;
   }

   
bool WildlifeModel::LoadHabitatTable( LPCTSTR habScoreTable, LPCTSTR lulcField, MapLayer *pIduLayer )
   {
   this->m_habScoreTablePath = habScoreTable;
   this->m_lulcField = lulcField;

   bool ok = theProcess->CheckCol( (const MapLayer*) pIduLayer, this->m_colLulc, lulcField, TYPE_LONG, CC_MUST_EXIST );

   if ( ! ok )
      {
      CString msg( "NAHARP Wildlife: Unable to locate field '" );
      msg += lulcField;
      msg += "' in the IDU coverage - Unable to continue...";
      Report::ErrorMsg( msg );
      return false;
      }

   int rows = m_habScoreTable.ReadAscii( habScoreTable, ',' );
   CString msg;
   msg.Format( "NAHARP Wildlife: Loaded %i rows from %s", rows, habScoreTable );
   Report::Log( msg );

   // collect species
   int cols = m_habScoreTable.GetColCount();
   rows = m_habScoreTable.GetRowCount();

   if ( rows == 0 || cols <= 6 )
      {
      Report::ErrorMsg( "NAHARP Wildlife: No species scores found - check your file!" );
      return false;
      }

   CString sppName;
   //CString habUse;
   int sppCount = 0;
   //int habUseCount = 0;

   for ( int row=0; row < rows; row++ )
      {
      sppName = m_habScoreTable.GetAsString( 0, row );

      // have we seen this one before?
      bool found = false;
      Specie *pSpecie = this->FindSpeciesFromName( sppName );

      if ( pSpecie == NULL )   // if not, add it to our list.
         {
         pSpecie = new Specie;
         pSpecie->m_name = sppName;

         pSpecie->m_commonName = m_habScoreTable.GetAsString( 1, row );

         m_speciesArray.Add( pSpecie );
         sppCount++;
         }

      // any year info provided
      int startYear = -1;
      int endYear   = -1;

      bool ok = m_habScoreTable.Get( START_YEAR_COL, row, startYear );
      
      if ( ok && startYear > 0 && pSpecie->m_startYear < 0 )
         pSpecie->m_startYear = startYear;

      ok = m_habScoreTable.Get( END_YEAR_COL, row, endYear );
      
      if ( ok && endYear > 0 && pSpecie->m_endYear < 0 )
         pSpecie->m_endYear = endYear;
      }

   return true;
   }


bool WildlifeModel::GenerateHabUseRowMappings( void )
   {   
   // generate row mappings
   // start by allocating arrays
   int habitatUseCount = (int) m_habUseArray.GetSize();

   for ( int i=0; i < (int) m_speciesArray.GetSize(); i++ )
      {
      Specie *pSpecie = m_speciesArray[ i ];

      //if ( pSpecie->m_inUse )
      //   {
         pSpecie->m_habUseRowArray.SetSize( habitatUseCount );
         for ( int j=0; j < habitatUseCount; j++ )
            pSpecie->m_habUseRowArray[ j ] = -1;   // -1 = unset
      //   }
      }

   // arrays allocated, get columns
   int usedRows = 0;
   int rows = m_habScoreTable.GetRowCount();

   // iterate through the rows in the spp/habitat table
   for ( int row=0; row < rows; row++ )
      {
      CString name = m_habScoreTable.GetAsString( 0, row );    // col 0 = name of spp

      Specie *pSpecie = this->FindSpeciesFromName( name );     // find the spp for this row

      if ( pSpecie != NULL )
         {
         TCHAR id;
         m_habScoreTable.Get( HAB_USE_COL, row, id );  // ID (code) for habitat use e.g. 'C' (cover), 'R' (reproduction)

         // find the corresponding habitat use
         for ( int i=0; i < (int) m_habUseArray.GetSize(); i++ )
            {
            HabitatUse *pUse = m_habUseArray[ i ];

            if ( pUse->m_code == id )
               {
               pSpecie->m_habUseRowArray[ pUse->m_index ] = row;   // found habitat use, so store this row index in the Species habUseRowArray
               usedRows++;
               break;
               }
            }
         }
      }

   return true;
   }


bool WildlifeModel::LoadGroupsTable( LPCTSTR groupsTable )
   {
    VDataObj _groupsTable(U_UNDEFINED);
   int rows = _groupsTable.ReadAscii( groupsTable );

   if ( rows <= 0 )
      {
      CString msg( "NAHARP Wildlife: Unable to read groups table '" );
      msg += groupsTable;
      msg += "' - this model can't continue";
      Report::ErrorMsg( msg );
      }

   int cols = _groupsTable.GetColCount();
   rows = _groupsTable.GetRowCount();

   // have group table, build groups.  Note that groups start in column 2
   for ( int col=2; col < cols; col++ )
      {
      CString name = _groupsTable.GetLabel( col );
      if ( name.IsEmpty() )
         continue;
      
      SppGroup *pGroup = new SppGroup;
      pGroup->m_name = name;
      m_groupArray.Add( pGroup );

      for ( int row=0; row < rows; row++ )
         {
         VData &v = _groupsTable.Get( col, row );
         int use = _groupsTable.GetAsInt( col, row );
         if ( use )
            {
            CString sppName = _groupsTable.GetAsString( 0, row );

            if ( sppName.IsEmpty() )
               continue;

            Specie *pSpecie = FindSpeciesFromName( sppName );

            if ( pSpecie == NULL )
               {
               CString msg( "NAHARP Wildlife: Species '" );
               msg += sppName;
               msg += "' not found in habitat table when building group '";
               msg += pGroup->m_name;
               msg += "'";
               Report::WarningMsg( msg );
               }
            else
               pGroup->AddSpecie( pSpecie );
            }
         }
      }

   return m_groupArray.GetSize() > 0;
   }

