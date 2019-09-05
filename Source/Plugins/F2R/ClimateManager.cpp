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
#include "stdafx.h"
#pragma hdrstop

#include "ClimateManager.h"
#include "F2R.h"

#include <Maplayer.h>
#include <tixml.h>
#include <PathManager.h>
#include <EnvConstants.h>
#include <Report.h>
#include <date.hpp>


extern F2RProcess *theProcess;


bool ClimateStation::GetData( int doy, int year, int col, float &value )  // doy=one-based
   {
   if ( m_pClimateData == NULL )
      return false;

   // get offset
   int startRow = -1;
   BOOL ok = m_yearMap.Lookup( year, startRow );

   if ( ! ok )
      return false;

   int row = startRow + doy - 1;  // -1 because DOY is one-based, assume on row for each day of the year
      
   // fix up col if needed
   switch( col )
      {
      case TMAX:
         if ( m_colMap.Lookup( "tempmax", col ) == false )
            return false;
         break;

      case TMIN:
         if ( m_colMap.Lookup( "tempmin", col ) == false )
            return false;
         break;

      case TMEAN:
         {
         int colTmin = -1, colTmax = -1;
         if ( m_colMap.Lookup( "tempmin", colTmin ) == false || m_colMap.Lookup( "tempmax", colTmax ) == false )
            return false;
         
         float tMin, tMax;
         m_pClimateData->Get( colTmin, row, tMin );
         m_pClimateData->Get( colTmax, row, tMax );
         value = (tMax+tMin)/2;
         return true;
         }

      case PRECIP:
         if ( m_colMap.Lookup( "totprecip (mm)", col ) == false )
            return false;
         break;
         
      case GDD0JAN1:
         value = m_cumDegDays0[ doy-1 ];
         return true;

      case GDD5APR1:
         value = m_cumDegDays5Apr1[ doy-1 ];
         return true;

      case GDD0APR15:
         value = m_cumDegDays0Apr15[ doy-1 ];
         return true;

      case GDD5MAY1:
         value = m_cumDegDays5May1[ doy-1 ];
         return true;

      case CHUCMAY1:
         value = m_chuCornMay1[ doy-1 ];
         return true;

      case PDAYS:
         value = m_pDays[ doy-1 ];
         return true;

      default:
         break;
      }

   return m_pClimateData->Get( col, row, value );
   }


bool ClimateStation::GetHistoricMean( int doy, int col, float &value )  // doy=one-based
   {
   if ( m_pHistoricMeans == NULL )
      return false;
      
   // fix up col if needed
   switch( col )
      {
      case TMIN:     col = 1;    break;
      case TMAX:     col = 2;    break;
      case TMEAN:    col = 3;    break;
      case PRECIP:   col = 4;    break;
      }

   return m_pHistoricMeans->Get( col, doy-1, value );
   }


bool ClimateStation::GetPeriodGDD( int startDOY, int endDOY, int year, float baseTemp, float &cdd )  // doy=one-based
   {
   cdd = 0;
   for ( int doy=startDOY; doy <= endDOY; doy++ )
      {
      float tMean = 0;

      if ( this->GetData( doy, year, TMEAN, tMean ) == false )
         return false;

      cdd += tMean - baseTemp;
      }
      
   return true;
   }


bool ClimateStation::GetPeriodPrecip( int startDOY, int endDOY, int year, float &cumPrecip )  // doy=one-based
   {
   cumPrecip = 0;
   for ( int doy=startDOY; doy <= endDOY; doy++ )
      {
      float precip = 0;

      if ( this->GetData( doy, year, PRECIP, precip ) == false )
         return false;

      cumPrecip += precip;
      }
      
   return true;
   }


bool ClimateStation::GetGrowingSeasonLength( int year, int &length, int &startDOY, int &endDOY )  // doy=one-based
   {
   // first, look for five days with tMean > 5
   int consDays = 0;
   length = startDOY = endDOY = 0;

   for ( int doy=JUN1; doy > 0; doy-- )
      {
      float tMean = 0;

      if ( this->GetData( doy, year, TMEAN, tMean ) == false )
         return false;

      if ( tMean > 5 )
         consDays++;
      else
         consDays = 0;

      if ( consDays == 5 )
         {
         startDOY = doy;
         break;
         }
      }
      
   if ( startDOY == 0 )
      return false;

   // next look for 5 day window with tMean < 5 after Sept 20
   consDays = 0;
   for ( int doy=SEP20; doy <= 365; doy++ )
      {
      float tMean = 0;

      if ( this->GetData( doy, year, TMEAN, tMean ) == false )
         return false;

      if ( tMean < 5 )
         consDays++;
      else
         consDays = 0;

      if ( consDays == 5 )
         {
         endDOY = doy-5;
         break;
         }
      }

   if ( endDOY == 0 )
      return false;

   length = endDOY - startDOY;
   
   return true;
   }

//    def get_pe(self, dDay, dSnowIceCover):
//        """ calculate potential evapotranspiration
//        
//        Args:
//            dDay (float): Fraction of the day in a year
//            dSnowIceCover (float): Snow Ice Cover
//        
//        Returns:
//            float: returns potential evapotranspiration
//        """  
//
//        dSolRad = self.get_sol_rad(dDay)
//        
//        #  WARNING: This 'timetuple' function is very slow
//        day_of_year = utils.get_days_of_the_year(dDay) #   dDay.timetuple().tm_yday    
//        year = dDay.year
//        tx = self.get_tx(year, day_of_year)
//        tn = self.get_tn(year, day_of_year)
//        
//        dt_mean, dt_meank = utils.get_daylymean(tx, tn)
//
//        #  calculate the saturated vapour pressure
//        exp = math.exp  #  Speed up built-in function pointer
//        log = math.log        
//        dSatVP = 10.0 * (exp(52.58 - (6790.5 / dt_meank) - 5.03 * log(dt_meank)))
//
//        #  calculate the slope of the saturated vapour presuure in pa/oC and convert to mbar.oc
//        dSlopeSatVP = dSatVP * (6790.5 / dt_meank ** 2 - 5.03 / dt_meank)
//
//        #  calculate the psychrometric constant and convert
//        dPsycConst = (0.06428 * dt_mean + 64.5571) / 100.0
//
//        #  calculate net radiation and convert to mm/day of water
//        dRad2 = 0.63 * (dSolRad * 1000.0 * 1000.0 / 43200.0) - 40.0   #  WATT/M2
//        if (dRad2 < 0.0):
//            dRad2 = 0.0
//        dRad3 = dRad2 * 43200.0 / (2450.0 * 1000.0)     #  MM/Day
//
//        #  account for the albedo of snow in winter
//        if (dSnowIceCover > 10.0):
//            dRad2 = 0.25 * (dSolRad * 1000.0 * 1000.0 / 43200.0) - 40.0 #  WATT/M2
//            if (dRad2 < 0.0):
//                dRad2 = 0.0
//            dRad3 = dRad2 * 43200.0 / (2830.0 * 1000.0)   #  MM/Day
//
//        # return total precipitation for the day
//       pe = 1.28 * (dSlopeSatVP * dRad3) / (dSlopeSatVP + dPsycConst)
//       
//        return pe 

float ClimateStation::GetPET(int method ) //, int doy, float iceCover)
   {
//	    if(method != VPM_PRIESTLEY_TAYLOR)
//           Report::ErrorMsg( "F2R Error: Only the Priestley_Taylow method has been implemented.  Call with VPM_PRIESTLEY_TAYLOR." );
   return 0;
   }


/////////////////////////////////////////////////////////////////////////////////////////
//
//                   C L I M A T E   M A N A G E R 
//
/////////////////////////////////////////////////////////////////////////////////////////


ClimateManager::ClimateManager()
   : m_pCurrentScenario( NULL )
   , m_isDirty( true )
   , m_colStationID( -1 )
   //, m_pClimateIndices( NULL )
   { }


ClimateManager::~ClimateManager()
   {    
   //if ( m_pClimateIndices != NULL )
   //   delete m_pClimateIndices;
   }


bool ClimateManager::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   bool ok = LoadXml( pEnvContext, initStr );

   if ( this->m_scenarioArray.GetSize() > 0 )
      this->m_pCurrentScenario = this->m_scenarioArray[ 0 ];
   
   //////////////////////////////
   // temporary
   //FDataObj d;
   //d.ReadAscii( "/envision/studyareas/easternontario/climate/1961-2010DailyBaseline/Cornwall_ED_546.csv");
   //GenerateHistoricClimateMeans( d );
   //d.WriteAscii( "/envision/studyareas/easternontario/climate/historic/Cornwall_ED_546_historic_means.csv" );
   //
   //d.Clear();
   //d.ReadAscii( "/envision/studyareas/easternontario/climate/1961-2010DailyBaseline/Kemptville CS_ED_547.csv");
   //GenerateHistoricClimateMeans( d );
   //d.WriteAscii( "/envision/studyareas/easternontario/climate/historic/Kemptville CS_ED_547_historic_means.csv" );
   //
   //d.Clear();
   //d.ReadAscii( "/envision/studyareas/easternontario/climate/1961-2010DailyBaseline/Ottawa CDA_ED_545.csv");
   //GenerateHistoricClimateMeans( d );
   //d.WriteAscii( "/envision/studyareas/easternontario/climate/historic/Ottawa CDA_ED_545_historic_means.csv" );
   //
   //d.Clear();
   //d.ReadAscii( "/envision/studyareas/easternontario/climate/1961-2010DailyBaseline/Russell_ED_543.csv");
   //GenerateHistoricClimateMeans( d );
   //d.WriteAscii( "/envision/studyareas/easternontario/climate/historic/Russell_ED_543_historic_means.csv" );
   //
   //d.Clear();
   //d.ReadAscii( "/envision/studyareas/easternontario/climate/1961-2010DailyBaseline/St. Albert_ED_544.csv");
   //GenerateHistoricClimateMeans( d );
   //d.WriteAscii( "/envision/studyareas/easternontario/climate/historic/St. Albert_ED_544_historic_means.csv" );
   //
   //d.Clear();
   //d.ReadAscii( "/envision/studyareas/easternontario/climate/1961-2010DailyBaseline/St. Anicet_ED_541.csv");
   //GenerateHistoricClimateMeans( d );
   //d.WriteAscii( "/envision/studyareas/easternontario/climate/historic/St. Anicet_ED_541_historic_means.csv" );
   //////////////////////////////

   if ( ! ok ) 
      return false;

   //if ( m_scenarioArray.GetSize() > 0  )
   //   m_currentScenarioID = 0;

   return true;
   }


bool ClimateManager::InitRun( int scenarioID )
   {
   SetCurrentScenarioID( scenarioID ); // sets current scenarion ptr

   LoadScenarioData();  // clears out any existing data and loads new data.
   
   return true;
   }


// run one time step (one year).  
bool ClimateManager::Run( EnvContext *pContext, bool useAddDelta)
   {
   // reset annual accumulators
   UpdateDailyValueArrays( pContext->currentYear );     // resets annual accumulators
   
   return true;
   }


bool ClimateManager::LoadXml( EnvContext *pContext, LPCTSTR filename )
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
   
   TiXmlElement *pXmlScenarios = pXmlRoot->FirstChildElement( "climate_scenarios" );
   if ( pXmlScenarios == NULL )
      {
      CString msg( "Climate Manager: missing climate <scenarios> element in input file: " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   LPCTSTR stationCol = pXmlScenarios->Attribute( "station_col" );
   if ( stationCol == NULL )
      {
      CString msg;
      msg.Format( "Climate Manager: missing 'station_col' attribute reading <climate_scenarios> tag in %s - this is a required attribute...", (LPCTSTR) filename );
      Report::ErrorMsg( msg );
      return false;
      }

   this->m_colStationID = pContext->pMapLayer->GetFieldCol( stationCol );

   if ( this->m_colStationID < 0 )
      {
      CString msg;
      msg.Format( "Climate Manager: 'station_col' attribute field [%s] defined in <climate_scenarios> tag in %s does not exist.  This is a required field...", stationCol, (LPCTSTR) filename );
      Report::ErrorMsg( msg );
      return false;
      }

   TiXmlElement *pXmlScenario = pXmlScenarios->FirstChildElement( "scenario" );

   while ( pXmlScenario != NULL )
      {
      // lookup fields
      LPTSTR name = NULL, path = NULL;
      int id = -1;

      XML_ATTR attrs[] = { // attr    type          address    isReq checkCol
                         { "id",      TYPE_INT,      &id,      true,  0 },
                         { "name",    TYPE_STRING,   &name,    true,  0 },
                         //{ "path",    TYPE_STRING,   &path,    true,  0 },
                         { NULL,      TYPE_NULL,      NULL,    false, 0 } };

      ok = TiXmlGetAttributes( pXmlScenario, attrs, filename );
      if (!ok)
         {
         CString msg( "Climate Manager: misformed <scenario> element reading <climate><scenarios> in input file: " );
         msg += filename;
         msg += ". This scenario will be ignored...";
         Report::ErrorMsg( msg );
         }
      else
         {
         ClimateScenario *pScenario = new ClimateScenario;

         pScenario->m_id = id;
         pScenario->m_name = name;
         m_scenarioArray.Add( pScenario );

         // get station defs for this scenario
         TiXmlElement *pXmlStation = pXmlScenario->FirstChildElement( "station" );

         while ( pXmlStation != NULL )
            {
            // lookup fields
            LPTSTR name = NULL, path = NULL, historic=NULL;
            int id = -1;

            XML_ATTR attrs[] = { // attr     type          address     isReq checkCol
                               { "id",       TYPE_INT,      &id,       true,  0 },
                               { "name",     TYPE_STRING,   &name,     false, 0 },
                               { "path",     TYPE_STRING,   &path,     true,  0 },
                               { "historic", TYPE_STRING,   &historic, true,  0 },
                               { NULL,       TYPE_NULL,      NULL,     false, 0 } };

            ok = TiXmlGetAttributes( pXmlStation, attrs, filename );
            if (!ok)
               {
               CString msg( "Climate Manager: misformed <station> element reading <climate><scenarios> in input file: " );
               msg += filename;
               msg += ". This station will be ignored...";
               Report::ErrorMsg( msg );
               }
            else
               {
               ClimateStation *pStation = new ClimateStation;

               pStation->m_id = id;
               pStation->m_name = name;
               pStation->m_path = path;
               pStation->m_pathHistoric = historic;

               pScenario->m_stationArray.Add( pStation );
               pScenario->m_stationMap.SetAt( id, pStation );
               }

            pXmlStation = pXmlStation->NextSiblingElement( "station" );
            }
         }

      pXmlScenario = pXmlScenario->NextSiblingElement( "scenario" );
      }

   return true;
   }


int ClimateManager::SetCurrentScenarioID( int scenarioID )
   {
   // any change?
   if ( m_pCurrentScenario != NULL && m_pCurrentScenario->m_id == scenarioID ) 
      return scenarioID; 
   
   int last = -1;
   if ( m_pCurrentScenario != NULL )
      last = m_pCurrentScenario->m_id;

   m_pCurrentScenario = NULL;

   for ( int i=0; i < (int) m_scenarioArray.GetSize(); i++ )
      {
      if ( m_scenarioArray[ i ]->m_id == scenarioID )
         {
         m_pCurrentScenario = m_scenarioArray[ i ];
         break;         
         }      
      }

   if ( m_pCurrentScenario == NULL )
      {
      CString msg;
      msg.Format( "Climate Manager: Invalid scenario ID '%i' specified when setting current scenario.", scenarioID );
      Report::Log( msg );
      }
   
   m_isDirty = true; 
   return last; 
   }


int ClimateManager::LoadScenarioData()   // called by InitRun() only
   {
   if ( m_isDirty == false )     // correct data already loaded?
      return 0;
   
   if ( m_pCurrentScenario == NULL )
      {
      CString msg( "Climate Manager: No valid climate data is available." );
      Report::LogWarning(msg);
      return -1;
      }

   int count = 0;

   // for each station defined for this scenario, load data
   for ( int i=0; i < (int) m_pCurrentScenario->m_stationArray.GetSize(); i++ )
      {
      ClimateStation *pStation = m_pCurrentScenario->m_stationArray[ i ];

      // get path to dataset
      CString path;
      if ( PathManager::FindPath( pStation->m_path, path ) < 0 )
         {
         CString msg;
         msg.Format( "Climate Manager: Unable to find climate scenario input file %s. No climate data will be available for this run", (LPCTSTR) pStation->m_path );
         Report::LogWarning(msg);
         return -1;
         }

      CString pathHistoric;
      if ( PathManager::FindPath( pStation->m_pathHistoric, pathHistoric ) < 0 )
         {
         CString msg;
         msg.Format( "Climate Manager: Unable to find climate scenario input file %s. ", (LPCTSTR) pStation->m_pathHistoric );
         Report::Log( msg );
         }

      // read to read file
      if ( pStation->m_pClimateData == NULL )
         {
         pStation->m_pClimateData = new FDataObj;
         pStation->m_pClimateData->ReadAscii( path, ',' );

         pStation->m_pHistoricMeans = new FDataObj;
         pStation->m_pHistoricMeans->ReadAscii( pathHistoric, ',' );

         // update column mapping
         int cols = pStation->m_pClimateData->GetColCount();
         int rows = pStation->m_pClimateData->GetRowCount();
   
         pStation->m_colMap.RemoveAll();
         pStation->m_yearMap.RemoveAll();
   
         for ( int i=0; i < cols; i++ )
            {
            LPCTSTR field = pStation->m_pClimateData->GetLabel( i );
            CString _field( field );
            _field.Trim();
            _field.MakeLower();
   
            pStation->m_colMap[ _field ] = i;
            }
   
         // update year mapping
         int yearCol = -1;
         pStation->m_colMap.Lookup( "year", yearCol );
         if ( yearCol < 0 )
            {
            CString msg;
            msg.Format( "Climate Manager: Unable to find column [Year] in the climate data file %s. This is a required field.  No climate data will be available for this run", (LPCTSTR) path );
            Report::LogWarning(msg);
            return -1;
            }
   
         // look for known columns
         pStation->m_colMap.Lookup( "tempmin", pStation->m_colTmin );
         pStation->m_colMap.Lookup( "tempmax", pStation->m_colTmax );
         pStation->m_colMap.Lookup( "totprecip (mm)", pStation->m_colPrecip );
   
         // create year map
         int startYear = -LONG_MAX;
         for ( int i=0; i < rows; i++ )
            {
            int year = pStation->m_pClimateData->GetAsInt( yearCol, i );
   
            if ( startYear != year )   // did we hit a new year?
               {                       // yes, so add this row index into the year map
               pStation->m_yearMap.SetAt( year, i );
               startYear = year;
               }
            }
   
         count++;
         }  // end of: if ( pStation->m_pClimateData == NULL )

      CString msg;
      msg.Format( "Climate Manager: Successfully loaded %i years of data from climate data file %s", (int) pStation->m_yearMap.GetCount(), (LPCTSTR) path );
      Report::LogInfo(msg);
      }  // end of: for each station


   m_isDirty = false;

   return count;
   }


bool ClimateManager::UpdateDailyValueArrays( int year )
   {
   if ( m_pCurrentScenario == NULL )
      return false;

   for ( int i=0; i < (int) m_pCurrentScenario->m_stationArray.GetSize(); i++ )
      {
      ClimateStation *pStation = m_pCurrentScenario->m_stationArray[ i ];

      if ( pStation->m_colTmax < 0 || pStation->m_colTmin < 0 )
         return false;

      // reset day of year counters
      pStation->m_doyCHU0    = -1;
      pStation->m_doyCHU600  = -1;
      pStation->m_doyCHU680  = -1;
      pStation->m_doyCHU1300 = -1;
      pStation->m_doyCHU1600 = -1;
      pStation->m_doyCHU1826 = -1;
      pStation->m_doyCHU2000 = -1;
      pStation->m_doyCHU2165 = -1;
      pStation->m_doyCHU2475 = -1;

      // reset anything else
      pStation->m_annualPrecip = 0;
      pStation->m_annualTMin   = 0;
      pStation->m_annualTMean  = 0;
      pStation->m_annualTMax   = 0;

      for ( int i=0; i < 12; i++)
         { pStation->m_rx1[ i ] = pStation->m_rx3[ i ] = 0; }

      pStation->m_r10mmDays        = 0;
      pStation->m_r10yrDays        = 0;
      pStation->m_r100yrDays       = 0;
      pStation->m_rShortDurationDays = 0;
      pStation->m_maxConsDryDays   = 0;
      pStation->m_extHeatDays      = 0;
      pStation->m_extColdDays      = 0;

      int start, end;
      pStation->GetGrowingSeasonLength( year, pStation->m_gslDays, start, end );
   
      // iterate through the year, updating array values
      int row = -1;
      pStation->m_yearMap.Lookup( year, row );

      if ( row < 0 )
         {
         CString msg;
         msg.Format( "Farm Model: Unable to find year %i in %s climate data", year, (LPCTSTR) pStation->m_name );
         Report::ErrorMsg( msg );
         return false;
         }

      float lastPrecip = 0;
      int dryDayCount = 0;
      int doyTMeanGt5 = -1;  // last doy with 5 prior days with Tmean>5°C
      int doyTMeanLt5 = -1;  // first day after doyTMeanGt5 where there are 5 following days with Tmean<5°C
      int consDayCount = 0;
      int currentMon   = 1;   // one-based months
      float monMax1Day = 0;
      float monMax3Day = 0;
      float precipM1   = 0;   // one day back
      float precipM2   = 0;   // two days back

      for ( int day=0; day < 365; day++ )
         {
         // first, find the starting row in th source data for this year
         float degDays0 = 0;
         float degDays5 = 0;
         float degDays0Apr15 = 0;
         float degDays5Apr1 = 0;
         float degDays5May1 = 0;
         float huCornMay1 = 0;
  
         float precip = pStation->m_pClimateData->GetAsFloat( pStation->m_colPrecip, row+day );
         float tMin   = pStation->m_pClimateData->GetAsFloat( pStation->m_colTmin, row+day );
         float tMax   = pStation->m_pClimateData->GetAsFloat( pStation->m_colTmax, row+day );
         float tMean  = (tMax+tMin)/2;

         pStation->m_annualPrecip += precip;
         pStation->m_annualTMin   += tMin;
         pStation->m_annualTMean  += tMean;
         pStation->m_annualTMax   += tMax;

         // max monthly precipitation
         int _year=2000, _month=0, _day=0;  // _month, _day are one-based
         ::GetCalDate( day+1, &_year, &_month, &_day, FALSE );

         if ( currentMon != _month || day == 364 )  // are we changing months?
            {
            pStation->m_rx1[ currentMon-1 ] = monMax1Day;
            pStation->m_rx3[ currentMon-1 ] = monMax3Day;

            currentMon = _month;
            monMax1Day = 0;
            monMax3Day = 0;
            }

         if ( precip > monMax1Day )
            monMax1Day = precip;

         float threeDay = precip + precipM1 + precipM2;
         if ( threeDay > monMax3Day )
            monMax3Day = threeDay;

         precipM2 = precipM1;    // get ready for next day
         precipM1 = precip;
 
         // additional precip metrics
         if ( precip > 10 )
            pStation->m_r10mmDays++;

         if ( precip > 74 )
            pStation->m_r10yrDays++;

         if ( precip > 106 )
            pStation->m_r100yrDays++;

         if ( precip >= 50 || lastPrecip + precip >= 175 )
            pStation->m_rShortDurationDays++;

         // consecutive drought days
         if ( precip > 0.1f )
            dryDayCount = 0;
         else 
            dryDayCount++;

         if ( dryDayCount > pStation->m_maxConsDryDays )
            pStation->m_maxConsDryDays = dryDayCount;

         // extreme heat, cold
         if ( tMax >= 30 )
            pStation->m_extHeatDays++;

         if ( tMin <= -20 )
            pStation->m_extColdDays++;
                  
         if ( tMean > 0 )
            {   
            degDays0 = tMean;
            degDays5 = (tMean-5);

            if ( day+1 >= APR1 ) // Apr 1
               degDays5Apr1 = (tMean-5);

            if ( day+1 >= APR15 ) // Apr 15
               degDays0Apr15 = tMean;

            if ( day+1 >= MAY1 ) // May 1
               degDays5May1 = tMean-5;
            }

         // Daily CHU = (Ymax + Ymin) ÷ 2, where 
         //    Y max = (3.33 x (T max-10)) - (0.084 x (T max-10.0)2) and 
         //    Y min = (1.8 x (T min - 4.4)) (If values are negative, set to 0). 
         // Accumulation begins on May 1st and ends with the first occurrence of -2°C in the fall.

         if ( day+1 >= MAY1 )
            {
            float yMax = (3.33f * (tMax-10)) - (0.084f * (tMax-10)*(tMax-10));
            float yMin = 1.8f * (tMin - 4.4f);
            float huCorn = ( yMax + yMin ) / 2;      
            if ( huCorn > 0 )
               huCornMay1 = huCorn;
            }

         float t[4], p[4];
         t[0] = tMin;
         t[1] = ((2*tMin) + tMax )/3;
         t[2] = (tMin + (2*tMax))/3;
         t[3] = tMax;

         p[0] = GetPfromT( t[0] );
         p[1] = GetPfromT( t[1] );
         p[2] = GetPfromT( t[2] );
         p[3] = GetPfromT( t[3] );
         
         float pDays = ( 5*p[0] + 8*p[1] + p[2] + 3*p[3] )/24.0f;

         // first day of the year?
         if ( day == 0 )
            {
            pStation->m_cumDegDays0[ 0 ]      = degDays0;
            //pStation->m_cumDegDays5[ 0 ]      = degDays5;
            pStation->m_cumDegDays0Apr15[ 0 ] = degDays0Apr15;
            pStation->m_cumDegDays5Apr1[ 0 ]  = degDays5Apr1; 
            pStation->m_cumDegDays5May1[ 0 ]  = degDays5May1; 
            pStation->m_chuCornMay1[ 0 ]      = huCornMay1;     
            pStation->m_pDays[ 0 ]            = pDays;     
            }
         else
            {
            pStation->m_cumDegDays0[ day ]      = pStation->m_cumDegDays0[ day-1 ]      + degDays0;
            //pStation->m_cumDegDays5[ day ]      = pStation->m_cumDegDays5[ day-1 ]      + degDays5;
            pStation->m_cumDegDays0Apr15[ day ] = pStation->m_cumDegDays0Apr15[ day-1 ] + degDays0Apr15;
            pStation->m_cumDegDays5Apr1[ day ]  = pStation->m_cumDegDays5Apr1[ day-1 ]  + degDays5Apr1; 
            pStation->m_cumDegDays5May1[ day ]  = pStation->m_cumDegDays5May1[ day-1 ]  + degDays5May1; 
            pStation->m_chuCornMay1[ day ]      = pStation->m_chuCornMay1[ day-1 ]      + huCornMay1;     
            pStation->m_pDays[ day ]            = pStation->m_pDays[ day-1 ]            + pDays;
            }

         // check corn doy's
         if ( day == MAY1 )
            pStation->m_doyCHU0 = day;   // for now

         if ( pStation->m_chuCornMay1[ day ] > 600 && pStation->m_doyCHU600 < 0 )
            pStation->m_doyCHU600 = day+1;

         if ( pStation->m_chuCornMay1[ day ] > 680 && pStation->m_doyCHU680 < 0 )
            pStation->m_doyCHU680 = day+1;

         if ( pStation->m_chuCornMay1[ day ] > 1300 && pStation->m_doyCHU1300 < 0 )
            pStation->m_doyCHU1300 = day+1;

         if ( pStation->m_chuCornMay1[ day ] > 1600 && pStation->m_doyCHU1600 < 0 )
            pStation->m_doyCHU1600 = day+1;

         if ( pStation->m_chuCornMay1[ day ] > 1826 && pStation->m_doyCHU1826 < 0 )
            pStation->m_doyCHU1826 = day+1;

         if ( pStation->m_chuCornMay1[ day ] > 2000 && pStation->m_doyCHU2000 < 0 )
            pStation->m_doyCHU2000 = day+1;

         if ( pStation->m_chuCornMay1[ day ] > 2165 && pStation->m_doyCHU2165 < 0 )
            pStation->m_doyCHU2165 = day+1;

         if ( pStation->m_chuCornMay1[ day ] > 2475 && pStation->m_doyCHU2475 < 0 )
            pStation->m_doyCHU2475 = day+1;

         lastPrecip = precip;
         }  // for each day of the year

      // fix up accumulated annual values
      pStation->m_annualTMin   /= 365;
      pStation->m_annualTMean  /= 365;
      pStation->m_annualTMax   /= 365;
      }  // for each ( station )

   return true;
   }


int ClimateManager::GenerateHistoricClimateMeans( FDataObj &d )
   {
   FDataObj _d( 5, 365, 0.0f );    // doy, tmin, tmax, precip

   for ( int i=0; i < 365; i++ )    // doy
      _d.Set( 0, i, i+1 );

   //TRACE( "%i,%i,%i,%i \n", d.GetAsInt(0,0), d.GetAsInt(1,0), d.GetAsInt(3,0), d.GetAsInt(3,0) );

   int years = 0;
   int currentYear = -1;
   for ( int row=0; row < d.GetRowCount(); row++ )
      {
      int doy = d.GetAsInt( 3, row );

      if ( doy > 365 )      // skip leap years
         continue;

      float tMin = d.GetAsFloat( 4, row );
      float tMax = d.GetAsFloat( 5, row );
      float prec = d.GetAsFloat( 6, row );

      _d.Add( 1, doy-1, tMin );
      _d.Add( 2, doy-1, tMax );
      _d.Add( 3, doy-1, (tMax+tMin)/2 );
      _d.Add( 4, doy-1, prec );

      int year = d.GetAsInt( 2, row );
      if ( year != currentYear )
         {
         currentYear = year;
         years++;
         }
      }

   // done extracting data, normalize
   for ( int i=0; i < 365; i++ )    // doy
      {
      _d.Div( 1, i, (float) years );
      _d.Div( 2, i, (float) years );
      _d.Div( 3, i, (float) years );
      _d.Div( 4, i, (float) years );
      }

   // replace passed in data obj
   d.Clear();
   d.SetSize( 5, 365 );
   d.SetLabel( 0, "DOY" );
   d.SetLabel( 1, "TempMin" );
   d.SetLabel( 2, "TempMax" );
   d.SetLabel( 3, "TempMean" );
   d.SetLabel( 4, "TotPrecip (mm)" );

   for ( int j=0; j < 5; j++ )
      {
      for ( int i=0; i < 365; i++ )    // doy
         {
         d.Set( j, i, _d.Get( j, i ) );
         }
      }

   return years;
   }


float ClimateManager::GetPfromT( float t )
   {
   float k = 10; 

   if ( t < 7 || t >= 30 )
      return 0;
   
   if ( t < 21 )
      return  k * ( 1 - ((t-21)*(t-21))/196 );

   //if ( t < 30 )
   return k * ( 1 - ((t-21)*(t-21))/81 );
   }


