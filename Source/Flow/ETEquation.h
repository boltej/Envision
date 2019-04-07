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

#include <vector>

using namespace std;

class HRU;
class EvapTrans;


/*!
This class is structured as a state machine that handles a number of Evapotranspiration calculations.   
This class allows for the choice of a reference ET calculation

Reference ET calculations currently implemented :

ASCE
FAO56
Penman-Monteith (PENN_MONT)
Kimberly-Penman (KIMB_PENN)
Hargreaves
Baier-robertson
*/

class ETEquation
   {
   public:

      /*!
         ET mode flags 
      */
      enum
         {
         NO_MODE=0,              // No refernce ET Equation specified
                                 //
		   ASCE,                   // ("THE ASCE STANDARDIZED REFERENCE EVAPOTRANSPIRATION EQUATION", Task Committee on Standardization of Reference Evapotranspiration
                                 //  Environmental and Water Resources Institute of the American Society of Civil Engineers January, 2005 Final Report
                                 //
         FAO56,                  // FAO56 
                                 // ("Step by Step Calculation of the Penman-Monteith Evapotranspiration(FAO - 56 Method)" Zotarelli, Dukes, Romero, Migliaccio, and Morgan)
                                 //
         PENN_MONT,              // ASCE-Pennman-Monteith ET Equation
                                 // 
         KIMB_PENN,              //  1982 Kimberly Pennman ET Equation 
                                 // ("Computation of the 1982 Kimberly-Penman and the Jensen-Haise Evapotranspiration Equations as Applied in the U.S.Bureau of Reclamation's Pacific Northwest AgriMet Program" February 1994 
                                 //  Prepared by Doug Dockter U.S.Bureau of Reclamation Pacific Northwest Region Water Conservation Center Revised January, 2008 Peter L.Palmer AgriMet Program)
                                 //
         HARGREAVES,             //  Hargreaves ET Equation

         HARGREAVES_1985,        //  Hargreaves ET Equation, no location adjustment

         BAIER_ROBERTSON,

         MODE_COUNT              //  Total Number of Mode Variables
         };

      ETEquation( EvapTrans* );  //  Default Constructor
      ~ETEquation();             // Default Destructor


      // Run Selected Equation

      // returns reference ET rate in mm/day
      float Run();      

      // Run Selected Equation
      /* 
         param m flag indicating which ET equation to run
         returns reference ET rate in mm/day
      */
      float Run( unsigned short m, HRU *pHRU );

      // Getters
      unsigned short GetMode();                          // return flag of currently selected mode \sa SetMode()
      bool GetUseTallRefCrop();                          // return TRUE if Tall Reference crop is being used, FALSE otherwise \sa SetUseTallRefCrop()
      double GetDailyMeanTemperature();                  // return provided mean air temperature in degrees C \sa SetDailyMeanTemperature()
      double GetVpd();
      double GetRelativeHumidity();                      // return provided relative humidity \sa SetRelativeHumidity()
      double GetWindSpeed();                             // return provided wind speed in mm/hr \sa SetWindSpeed()
      double GetSolarRadiation();                        // return provided incoming Solar Radiation in  MJ/(m^2 h) \sa SetSolarRadiation()
      double GetTwilightSolarRadiation();                // return provided Twilight Solar Radiation in MJ/(m^2 h) \sa SetTwilightRadiation()
      double GetStationElevation();                      // return provided Station elevation in meters \sa SetStationElevation()
      double GetStationLatitude();                       // return provided station latitude in degrees \sa SetStationLatitude()
      double GetStationLongitude();                      // return provided station longitude in degrees \sa SetStationLongitude()
      double GetTimeZoneLongitude();                     // return provided time zone longitude in degrees \sa SetTimeZoneLongitude()
      double GetDailyMaxTemperature();                   // return provided maximum daily temperature in degrees C \sa SetMaxDailyMeanTemperature()
      double GetDailyMinTemperature();                   // return provided minimum daily temperature in degrees C \sa SetMinDailyMeanTemperature()
      double GetRecentMeanTemperature();                 // return provided mean temperature for several previous days (Kimberly-Pennmen uses the mean temperature for the 3 previous days) \sa SetRecentMeanTemperature()

      // Only used for method Kimberly_Penman 
      // Gets the 5 Agrimet station coefficients (idu location specific)
      double GetAgrimentStationCoeff(const unsigned int coeff);

      // use for Penman_Monteith
      double GetLAI();                                   // return provided LAI
      double GetTreeHeight();                            // return provided TreeHeight
                                                            
      unsigned int GetYear();                            // return provided year \sa SetYear()
      unsigned int GetMonth();                           // return provided month \sa SetMonth()
      unsigned int GetDay();                             // return provided day \sa SetDay()
      unsigned int GetDoy();                             // return provided day of year \sa SetDoy()
      unsigned int GetHoursInDay();                      // return provided hours in a day \sa SetHoursInDay()

      // Setters
      void SetMode( unsigned short m );                  // param m an ET equation mode flag \sa GetMode()
      void SetUseTallRefCrop( bool utc );                // param utc a BOOL value indicating if tall crop assumptions should be used \sa GetUseTallRefCrop()
      void SetVpd(double vpd);
      void SetDailyMeanTemperature( double tMean );      // param tMean mean air temperature in degrees C \sa GetDailyMeanTemperature()
      void SetRelativeHumidity( double rh );             // param rh relative humidity \sa GetRelativeHumidity()
      void SetSpecificHumidity( double sph );            // param rh relative humidity \sa GetRelativeHumidity()
      void SetWindSpeed( double ws );                    // param ws wind speed in mm/hr (km/d for Kimberly Pennman) \sa GetWindSpeed()
      void SetSolarRadiation( double sr );               // param sr incoming solar radiation in MJ/(m^2 h) \sa GetSolarRadiation()
      void SetTwilightSolarRadiation( double tsr );      // param tsr incoming radiation during twilight in MJ/(m^2 h) \sa GetTwilightSolarRadiation()
      void SetStationElevation( double se );             // param se elevation of the station site in meters \sa GetStationElevation()
      void SetStationLatitude( double sl );              // param sl station latitude in degrees \sa GetStationLatitude()
      void SetStationLongitude( double sl );             // param sl station longitude in degrees \sa GetStationLongitude()
      void SetTimeZoneLongitude( double tzl );           // param tzl longitude of the time zone in degrees \sa GetTimeZoneLongitude()
      void SetDailyMaxTemperature(double tmp);           // param tmp The maximum daily temperature in degrees C \sa GetDailyMaxTemperature()
      void SetDailyMinTemperature(double tmp);           // param tmp The minimum daily temperature in degrees C \sa GetMinDailyMeanTemperature()
      void SetRecentMeanTemperature(double tmp);         // param tmp The mean temperature for several previous days (Kimberly-Pennmen uses the mean temperature for the 3 previous days) \sa GetShortwaveRadiation()

      // Only used for method Kimberly_Penman 
      // Sets the 5 Agrimet station coefficients (idu location specific) based on a Taylor explansion fit
      void SetAgrimentStationCoeff( vector<double> agrimetStationCoeffs );

      // use for method Penman_Monteith
      void SetLAI(double LAI);                           // param LAI The Leaf Area Index.
      void SetTreeHeight(double height);                 // param The provided TreeHeight.
                                                            
      void SetYear( unsigned int y );                    // param y the desired year \sa GetYear()
      void SetMonth( unsigned int m );                   // param m the desired month \sa GetMonth()
      void SetDay( unsigned int d );                     // param d the desired day \sa GetDay()
      void SetDoy( unsigned int d );                     // param d the desired day of year \sa GetDay()
      /*!
         Set the desired date
         \param d the desired day
         \param m the desired month
         \param y the desired year
         \sa SetYear(), SetMonth(), SetDay(), GetYear(), GetMonth(), and GetDay()
      */


      void SetDate( unsigned int d, unsigned int m, unsigned int y );
      void SetHoursInDay( unsigned int hid );         //!< \param hid the number of hours in a day \sa SetHoursInDay()

      double CalcRelHumidity(float specHumid, float tMin, float tMax, float elevation, double &ea);
      double CalcRelHumidityKP(float specHumid, float tMin, float tMax, float elevation, double &ea);
	   double CalculateRelHumidity(float specHumid, float meanTemp, float tMax, float elevation, double &ea, double &vpd);


   private:
      EvapTrans *m_pEvapTrans;
     
      static double NOVAL;                               // value that represents a variable that hasn't been provided a legitimate value
      
      unsigned short m_mode;                             // value that determines which ET Equation to run
      
      // FAO56 : Agriculture ET calcuation as tall reference crop
      bool m_useTallRefCrop;

      // climate variables needed for calculation of reference ET
 //     double m_airTemperature;
      double m_dailyMeanTemperature;
      double m_relativeHumidity;
      double m_specificHumidity;
      double m_vpd;
      double m_windSpeed;
      double m_solarRadiation;

      // Penman-Monteith : Forest ET calculation
      double m_treeHeight;
      double m_lai;

      double m_twilightSolarRadiation;
     
      double m_stationElevation;                         // TODO: atmospheric pressure? and Relative Solar Radiation? or just Elevation?
      double m_stationLatitude;
      double m_stationLongitude;
      double m_timeZoneLongitude;
      double m_dailyMaxTemperature;
      double m_dailyMinTemperature;
      
      double m_recentMeanTemperature;

      // Kimberly-Penman : Agrimet Station Coefficients 
      vector<double> m_agrimetStationCoeffs;

      unsigned int m_month;
  //    unsigned int m_day;
  //    unsigned int m_year;
      unsigned int m_doy;
   //   unsigned int m_hoursInDay;

      // Support methods
      template<class T>
      inline BOOL isNan(T arg)
         {
            //nans always return false when asked if equal to themselves
            return !(arg==arg);
         };

		static double CalcDewTemp(const double & relHum, double temp);

      // reference ET equation methods
     float PennMont(HRU *pHRU);                       // Pennman-Monteith equation with explicit resistances and previously supplied data, if possible; return ET demand in mm/day, or -1 if prequisite values were not provided
	  float Asce();                                    // Standardized Penman-Monteith equation with previously supplied data, if possible; return ET demand in mm/day
	  float Fao56();                                   // FAO56 Penman-Monteith equation with previously supplied data, if possible; return ET demand in mm/day
     float Hargreaves();                              // Hargreaves equation with previously supplied data, if possible; return ET demand in mm/day
     float Hargreaves1985();                          // Hargreaves equation with previously supplied data, if possible; return ET demand in mm/day
     float KimbPenn();                                // Kimberly-Pennman equation with previously supplied data, if possible; return ET demand in mm/day or -1 if prequisite values were not provided
     float BaierRobertson();
}
   ;
