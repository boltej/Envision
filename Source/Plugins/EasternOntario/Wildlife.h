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
#include "EasternOntario.h"

class HabitatType;


// ---------------------------------------------------------------------------------
//  Basic idea:
//      Species        - holds collection of row indexes in the spp/habitat score table
//      HabitatType    - top level NAHARP Habitat categories, e.g. "Cropland"
//                       Contains HabitatSubTypes
//      HabitatSubType - Subtypes defined by cols in Spp/Habitat score table
//      HabitatType    - contains information about habitat use types (the colums of the habitat table)
//                       including weights for the habitat type. e.g. "Reproduction", "Feeding"
// ---------------------------------------------------------------------------------



class Specie
{
// contains information about each species, specifically
// row indexes into the habitat score table associated with this specie,
// as well as usage info
public:
   Specie( void ) : m_inUse( false ), m_slcAlreadySeen( false ), m_startYear( -1 ), m_endYear( -1 ) { }

   CString m_name;
   CString m_commonName;

   int  m_startYear;
   int  m_endYear;

   bool m_inUse;
   bool m_slcAlreadySeen;   // intermediate varaible used in slc score calculation
   
   CArray< int, int > m_habUseRowArray;    // row indexes for each habitat use type in the habitat scores table

   bool IsInUse( int year ) 
      {
      bool passStart = m_startYear < 0 || year >= m_startYear;
      bool passEnd   = m_endYear   < 0 || year <= m_endYear;
      return m_inUse && passStart && passEnd;
      }   
};


class HabitatSubType    // example - "IDLE_LND" i.e. second-level NAHARP categories
{
public:
   HabitatSubType( void ) : m_pParent( NULL ), m_col( -1 ) { }

   CString m_name;
   HabitatType *m_pParent;

   int     m_col;    // column for this entry in the habitat score table
};


class HabitatType    // example - "Cropland" i.e. top level NAHARP Habitat categories
{
public:
   HabitatType( void ) : m_index( -1 ) { }

   CString m_name;
   int m_index;    // array offset int m_habTypeArray
   PtrArray< HabitatSubType > m_subTypeArray;
   CArray< float, float > m_typeBaselineScore;     // remember time 0 scores for each type to compute deltas
   
   // intermediate storage
   //int   m_slcSppCount;
   float m_slcPctArea;   
   float m_slcHabUseValue;     // temporary,

};


class HabitatUse
{ 
// contains information about habitat use types (the colums of the habitat table)
// including weights for the habitat type. e.g. "Reproduction", "Feeding"
public:
   HabitatUse( void ) : m_index( -1 ), m_code( 0 ) { }

   CString m_name;         // use name, e.g. "reproduction", "feeding", "loafing" ...
   TCHAR   m_code;         // character code for this use, from spp/habitat table
   int     m_index;        // array offset for this use in global habitat use array
};


class SppGroup
{
public:
   SppGroup( void ) : m_col( -1 ) { }

   CString m_name;

   CMap< Specie*, Specie*, int, int > m_sppMap; 

   int   m_col;
   int   m_slcUsedSppCount;
   float m_slcHabCapacity;
   float m_habCapacity;

   // methods
   void AddSpecie( Specie *pSpecie ) { m_sppMap.SetAt( pSpecie, 0 ); }
   bool IsSppInGroup( Specie *pSpecie ) { int value; return m_sppMap.Lookup( pSpecie, value ) ? true : false; }
   int  GetSppCount( void ) { return (int) m_sppMap.GetSize(); }
};


class WildlifeModel
{
public:
   WildlifeModel( void );

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );

   int   m_id;
   float m_score;

   Specie *GetSpecies( int i ) { return m_speciesArray[ i ]; }
   int     GetSpeciesCount( void ) { return (int) m_speciesArray.GetSize(); }
   Specie *FindSpeciesFromName( LPCTSTR name );

   HabitatUse *GetHabitatUse( int i ) { return m_habUseArray[ i ]; }
   int         AddHabitatUse( HabitatUse *pHU ) { return (int) m_habUseArray.Add( pHU ); }

   HabitatType    *FindHabitatType   ( LPCTSTR name );
   HabitatSubType *FindHabitatSubType( LPCTSTR name );

protected:
   VDataObj m_habScoreTable;    // habitat score table

   // intermediate data storage
   CString m_habScoreTablePath;
   CString m_lulcField;
   CString m_habMatrixPath;
   CString m_outputField;
      
   int m_colLulc;   // idu coverage

   //float m_scoreThreshold;

   CMap< int, int, HabitatSubType*, HabitatSubType* > m_lulcToHabSubTypeMap;  // key = attribute value, value=associated column in lookup table
   PtrArray< Specie      > m_speciesArray;    // collection of species infor (rows in table)
   PtrArray< HabitatUse  > m_habUseArray;     // collection of habitat use info
   PtrArray< HabitatType > m_habTypeArray;    // collection of habitat type info (cols in table)
   PtrArray< SppGroup    > m_groupArray;

   bool  LoadXml( EnvContext*, LPCTSTR filename );
   bool  LoadHabitatTable( LPCTSTR habScoreTable, LPCTSTR habitatTypeField, MapLayer *pIduLayer );
   bool  LoadGroupsTable( LPCTSTR groupsTable );
   //bool  BuildSLCs( MapLayer *pLayer );
   void  GetSLCScores( SLC *pSLC, MapLayer *pLayer, int year );
   bool  GenerateHabUseRowMappings( void );


};


