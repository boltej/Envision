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
//#include "VSMB.h"


// column references for ClimateStation::GetData() calls
const int TMIN      = -2;
const int TMAX      = -3;
const int TMEAN     = -4;
const int PRECIP    = -5;

const int GDD0JAN1  = -10;  
const int GDD5APR1  = -12;
const int GDD0APR15 = -13;
const int GDD5MAY1  = -14;
const int CHUCMAY1  = -15;
const int PDAYS     = -16;

// day of year constants (one-based)
const int JAN1   = 1;
const int APR1   = 91;
const int APR15  = 106;
const int MAY1   = 121;
const int MAY10  = 130;
const int MAY15  = 135;
const int MAY24  = 144;
const int MAY31  = 151;
const int JUN1   = 152;
const int JUN7   = 158;
const int JUN10  = 161;
const int JUN20  = 171;
const int JUL6   = 187;
const int JUL7   = 188;
const int AUG1   = 213;
const int AUG3   = 215;
const int AUG31  = 243;
const int SEP1   = 244;
const int SEP20  = 263;

class ClimateStation
{
public:
   int     m_id;         // attribute code
   CString m_name;       // optional name
   CString m_path;       // full or partially qualified path
   CString m_pathHistoric;   // historic (mean) datasets

   // "known" columns in the climate datasets
   int m_colTmin;    // [TMIN]
   int m_colTmax;    // [TMAX]
   int m_colPrecip;  // [PRECIP]

public:
   ClimateStation() : m_id( -1 ), m_name(), m_path(), 
         m_doyCHU0( -1 ), m_doyCHU600( -1 ), m_doyCHU680( -1 ), m_doyCHU1300( -1 ), m_doyCHU1600( -1 ), 
         m_doyCHU1826( -1 ),m_doyCHU2000( -1 ),m_doyCHU2165( -1 ),
         m_doyCHU2475( -1 ),
         m_annualPrecip( 0 ), m_annualTMin( 0 ), m_annualTMax( 0 ), m_annualTMean( 0 ),
         m_r10mmDays( 0 ), m_r10yrDays( 0 ), m_r100yrDays( 0 ),
         m_rShortDurationDays( 0 ), m_extHeatDays( 0 ), m_extColdDays( 0 ), 
         m_pClimateData( NULL ),  m_pHistoricMeans( NULL ),
         m_colTmin( -1 ), m_colTmax( -1 ), m_colPrecip( -1 )
         { }

   ~ClimateStation() { if ( m_pClimateData != NULL ) delete m_pClimateData;  if ( m_pHistoricMeans != NULL ) delete m_pHistoricMeans; }

   // various methods
   int GetFieldCol( LPCTSTR field ) { return ( m_pClimateData == NULL ) ? -1 : m_pClimateData->GetCol( field ); }

   bool GetData( int doy, int year, int col, float &value );  // note: DOY's are ONE based
   bool GetHistoricMean( int doy, int col, float &value );
   bool GetPeriodGDD( int startDOY, int endDOY, int year, float baseTemp, float &cdd );
   bool GetPeriodPrecip( int startDOY, int endDOY, int year, float &cumPrecip );
   bool GetMaxConsDryDays( int startDOY, int endDOY, int year, float threshold );   
   bool GetGrowingSeasonLength( int year, int &length, int &startDOY, int &endDOY );

   float GetPET(int method);


   // calculated climate variables (for each day of the year)
   float m_cumDegDays0[ 365 ];        // cumulative GDD - base temp=0, one per station
   //float m_cumDegDays5[ 365 ];        // cumulative GDD - base temp=5, one per station
   float m_cumDegDays0Apr15[ 365 ];   // cumulative GDD - base temp=0, from Apr 15 only, one per station
   float m_cumDegDays5Apr1[ 365 ];    // cumulative GDD - base temp=5, from Apr 1 only, one per station
   float m_cumDegDays5May1[ 365 ];    // cumulative GDD - base temp=5, from May 1 only, one per station
   float m_chuCornMay1[ 365 ];        // cumulative heat units - corn
   float m_cummDegDayAlfafa[ 365 ];
   float m_pDays[ 365 ];              // potato days

   int m_doyCHU0;
   int m_doyCHU600;
   int m_doyCHU680;
   int m_doyCHU1300;
   int m_doyCHU1600;
   int m_doyCHU1826;
   int m_doyCHU2000;
   int m_doyCHU2165;
   int m_doyCHU2475;

   float m_annualPrecip;
   float m_annualTMin;
   float m_annualTMax;
   float m_annualTMean;

   float m_rx1[ 12 ];            // monthly Max 1 Day Precip;
   float m_rx3[ 12 ];            // monthly Max 3 Day Precip;
   int   m_r10mmDays;            // number of days with more than 10mm precip
   int   m_r10yrDays;            // number of days precip > 74mm (10 year storm)
   int   m_r100yrDays;           // number of days precip > 106mm (100 year storm)
   int   m_rShortDurationDays;   // number of events of days with P>=50mm in 24 hrs or P>=175mm in 48 hrs
   int   m_maxConsDryDays;
   int   m_extHeatDays;          // number of days Tmax >= 30
   int   m_extColdDays;          // number of days Tmin <= -20
   int   m_gslDays;              // (Number of days between 5 days with Tmean>5°C and 5 days with Tmean<5°C)

   // climate data for this station
   FDataObj *m_pClimateData;
   FDataObj *m_pHistoricMeans;

   // various maps
   CMap< CString, LPCTSTR, int, int > m_colMap;    // key=col name, value=col index
   CMap< int, int, int, int > m_yearMap;           // key=year, value=row index of start of that year

};


class ClimateScenario
{
public:
   int m_id;
   CString m_name;

   PtrArray< ClimateStation > m_stationArray;

   // various maps
   CMap< int, int, ClimateStation*, ClimateStation* > m_stationMap;        // key=stationID, value=station ptr
};


class ClimateManager
{
public:
   ClimateManager();
   ~ClimateManager();
   
   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( int scenarioID );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );

   bool LoadXml(EnvContext *pContext, LPCTSTR filename);
   int  SetCurrentScenarioID( int scenarioID );

public:
   int GetStationIDCol() { return m_colStationID; }
   ClimateStation *GetStationFromID( int stationID );
   ClimateStation *GetStation( int i );
   int GetStationCount();
   bool GetData( int doy, int year, int col, int station, float &value );  // note: DOY's are ONE based
   bool GetHistoricMean( int doy, int col, int station, float &value );  // note: DOY's are ONE based
   // specific climate calculation
   bool UpdateDailyValueArrays( int year );

protected:
   // climate scenario definitions
   PtrArray< ClimateScenario > m_scenarioArray;

   //int  m_currentScenarioID;    // note: use SetCurrentScenarioID() to set this value!
   ClimateScenario *m_pCurrentScenario;
   bool m_isDirty;

public:
   // "known" IDU coverage columns
   int m_colStationID;

protected:
   int LoadScenarioData();   // loads current scenarion data from disk if needed
   int GenerateHistoricClimateMeans( FDataObj &d );
   float GetPfromT( float t );

};



/////////////////////////////////////////////////////////////////////////////////////////
//   I N L I N E    M E T H O D S 
/////////////////////////////////////////////////////////////////////////////////////////


inline
int ClimateManager::GetStationCount()
   {
   if ( m_pCurrentScenario == NULL )
      return false;

   // find station
   return (int)  m_pCurrentScenario->m_stationArray.GetSize();
   }

inline
ClimateStation *ClimateManager::GetStation( int i )
   {
   if ( m_pCurrentScenario == NULL )
      return false;

   // find station
   return m_pCurrentScenario->m_stationArray[ i ];
   }




inline
bool ClimateManager::GetData( int doy, int year, int col, int stationID, float &value )  // doy=one-based
   {
   if ( m_pCurrentScenario == NULL )
      return false;

   // find station
   ClimateStation *pStation = NULL;
   if ( m_pCurrentScenario->m_stationMap.Lookup( stationID, pStation ) == 0 )
      return false;

   return pStation->GetData( doy, year, col, value );
   }


inline
bool ClimateManager::GetHistoricMean( int doy, int col, int stationID, float &value )  // doy=one-based
   {
   if ( m_pCurrentScenario == NULL )
      return false;

   // find station
   ClimateStation *pStation = NULL;
   if ( m_pCurrentScenario->m_stationMap.Lookup( stationID, pStation ) == 0 )
      return false;

   return pStation->GetHistoricMean( doy, col, value );
   }


inline
ClimateStation *ClimateManager::GetStationFromID( int stationID )
   {
   if ( m_pCurrentScenario == NULL )
      return NULL;

   ClimateStation *pStation= NULL;

   if ( m_pCurrentScenario->m_stationMap.Lookup( stationID, pStation ) == false )
      return NULL;

   return pStation;
   }
