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
/* Fire.h
**
** By Tim Sheehan 2011.03.14
**
** The fire class holds the unique initializaiton data associated
** with one run of the FlamMap DLL. It also contains the date and 
** probability data associated with the fire. The probability data
** is used to determine whether the fire will be run or not.
**
** In the first (and maybe last) implementation of using a monte carlo
** method to determine whether a fire in the input file will be run,
** during the startup of a run, all fires are considered for running
** and only those that will be run are stored. The constructor for
** this class does the monte carlo draw.
**
** 2011.03.14 - tjs - dropping tested code into FlamMapAP
*/

#pragma once


class Fire
{
friend class FiresList;

private:
   int    m_yr;
   int    m_julDate;
   int    m_burnPeriod;
   int    m_windSpd;
   int    m_windAzmth;
   float  m_burnProb;
   bool   m_doRun;   // set in Init() based on prob
   TCHAR  m_FMSname[MAX_PATH];    // full path+filename
	TCHAR  m_shortFMSname[MAX_PATH];  // just filename
	double m_ignitX;  
	double m_ignitY;
	int    m_erc;
	float  m_origSizeHA;
	int    m_fuel;   // set to zero in Init()
	TCHAR m_fireID[MAX_PATH];
	TCHAR m_record[1024];
	int m_envisionFireID;

public:
	double FIL[20];
	void Init(
      int yr,
      float burnProb, 
      int julDate,
      int burnPeriod,
      int windSpd,
      int azmth,
      float monteCarloProb,
      LPCTSTR FMSname,
		LPCTSTR shortFMS,
		int erc,
		float size,
		float x,
		float y, 
		LPCTSTR fireID,
		LPCTSTR record,
		int envisionFireID
      );

   //Fire(const char *InitStr, float monteCarloProb, const char *baseFmsName);
   //Fire( int yr, float prob, int julDate, int burnPeriod, int windSpd, int azmth, float monteCarloProb );
	Fire() { m_FMSname[0] = 0; for (int f = 0; f < 20; f++) FIL[f] = 0.0; }
   Fire( const Fire &f ) { *this = f; }
   
   ~Fire(){}

   Fire &operator = ( const Fire &f ) { m_yr = f.m_yr; m_julDate = f.m_julDate; m_burnPeriod = f.m_burnPeriod; 
               m_windSpd = f.m_windSpd; m_windAzmth = f.m_windAzmth; m_burnProb = f.m_burnProb;
					m_doRun = f.m_doRun; strcpy( m_FMSname, f.m_FMSname ); strcpy(m_shortFMSname, f.m_shortFMSname); 
					m_ignitX = f.m_ignitX; m_ignitY = f.m_ignitY; m_erc = f.m_erc; m_origSizeHA = f.m_origSizeHA; 
					strcpy(m_fireID, f.m_fireID); strcpy(m_record, f.m_record); m_envisionFireID = f.m_envisionFireID;  return *this;
   }

   int    GetYr() { return m_yr; }
   int    GetJulDate() { return m_julDate; }
   int    GetWindSpd() { return m_windSpd; }
   int    GetWindAzmth() { return m_windAzmth; }
   double GetBurnProb() { return m_burnProb; }
   int    GetBurnPeriod() { return m_burnPeriod; }
   bool   GetDoRun() { return m_doRun; }
   void   SetDoRun(bool set)  {  m_doRun = set; }
	double GetIgnitX() { return m_ignitX; }
	double GetIgnitY() { return m_ignitY; }
	void   SetIgnitionPoint(double fireX, double fireY)   {  m_ignitX = fireX; m_ignitY = fireY; }
   char  *GetFMSname() { return m_FMSname; }
	char  *GetShortFMSname() { return m_shortFMSname; }
	int    GetERC() { return m_erc; }
	float  GetOrigSizeHA() { return m_origSizeHA; }
	int    GetFuel() { return m_fuel; }
	void   SetFuel(int _fuel) { m_fuel = _fuel; }
	char  *GetFireID() { return m_fireID; }
	TCHAR *GetRecord() { return m_record; }
	int GetEnvisionFireID() { return m_envisionFireID; }
	//double *GetFIL(){ return FIL; }
};


inline
void Fire::Init( int yr, float prob, int julDate, int burnPeriod, int windSpd, int azmth,
float monteCarloProb, LPCTSTR FMSname, LPCTSTR shortFMS, int erc, float size,
float x, float y, LPCTSTR fireID, LPCTSTR record, int envisionFireID)
   {
   ASSERT( yr >= -1 && julDate !=0 );

   m_yr         = yr;
   m_julDate    = julDate;
   m_windSpd    = windSpd;
   m_windAzmth  = azmth;
   m_burnProb   = prob;
   m_burnPeriod = burnPeriod;
   strcpy(m_FMSname, FMSname);
   strcpy(m_shortFMSname, shortFMS);
   strcpy(m_fireID, fireID);
   strcpy(m_record, record);
   m_erc = erc;
	m_origSizeHA = size;
	m_ignitX = x;
	m_ignitY = y;
   m_doRun      = (m_burnProb >= monteCarloProb);
	m_fuel       = 0;
	m_envisionFireID = envisionFireID;
	for (int f = 0; f < 20; f++)
		FIL[f] = 0.0;
   }
