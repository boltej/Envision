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

#include <EnvExtension.h>
#include <Maplayer.h>
#include <PtrArray.h>
#include "PolyGridLookups.h"
#include "LCPGenerator.h"
#include "FireYearRunner.h"
#include "FlameLengthUpdater.h"
#include "FlamMap_DLL.h"

#include <randgen\Randunif.hpp>  // for fire list handling
#include "shapefil.h"
#include "MinTravelTime.h"

class FAGrid : public MapLayer 
   {
   public:
     FAGrid(Map *pMap);
     ~FAGrid(){};
   };


struct FIRELIST
   {
   CString m_path;
   FIRELIST( LPCTSTR name ) : m_path( name ) { } 
   };


class FMScenario
{
public:
   CString m_name;
   int     m_id;

   PtrArray< FIRELIST > m_fireListArray;

   int AddFireList( FIRELIST *pFireList ) { return (int) m_fireListArray.Add( pFireList ); }
   FIRELIST *GetFireList( int index=-1 ); // note - default picks a list randomly
};


class FlamMapAP : public EnvModelProcess
{
	friend class CFlamMapAPDlg;
public:

   // For Tracking over the history of the simulation
   float m_meanFlameLen;    // ft
   float m_maxFlameLen;     // ft
   float m_burnedAreaHa;
   float m_cumBurnedAreaHa;

   FlamMapAP(void);
   ~FlamMapAP(void);

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext, bool useInitSeed);
   bool Run    ( EnvContext *pContext );
   bool EndRun ( EnvContext *pContext);
   //int  Setup(EnvContext* pContext, HWND);

protected:
   Map               *m_pMap;
   FAGrid            *m_ptLayer;
   PolyGridLookups   *m_pPolyGridLkUp;          // store Poly-Grid lookup info for mapping grids to IDUs
  // LCPGenerator       m_lcpGenerator;
   LCPGenerator       *m_pLcpGenerator;
   FlameLengthUpdater m_flameLengthUpdater;
   EnvContext        *m_pEnvContext;
   FireYearRunner    *m_pFireYearRunner;
 
public:
   int     m_areaCol;         // used by FlameLengthUpdater (NOT SET!!!
   int     m_coverTypeCol;
   int     m_structStageCol;
   int     m_disturbCol;
   int     m_flameLenCol;     // used by FlameLengthUpdater
   int     m_fireIDCol;     // used by FlameLengthUpdater
   int     m_variantCol;      // used by LCPGenerator
   int     m_vegClassCol;     // used by LCPGenerator
   int     m_saveReportFlag;
   int     m_fModel_plusCol;      // used by LCPGenerator
   int     m_regionCol;      // used by LCPGenerator
   int     m_pvtCol;      // used by LCPGenerator
   CString m_flamMapDir;
   CString m_flamMapSrcDir;
   CString m_scenarioLCPFNameRoot;  // LCPGenerator
   CString m_outputEnvisionFirelistName;
   CString m_outputEchoFirelistName;
   CString m_outputDeltaArrayUpdatesName;

   bool LoadPolyGridLookup( MapLayer *pIDULayer );
	PolyGridLookups   *GetPolyGridLookups(){return m_pPolyGridLkUp;}

   int ResolvePath( CString &filename, LPCTSTR errorMsg  );

   // runparams
   
public:
   // XML
   CString m_workingPath;
   int     m_doLogging;
   CString m_logFileName;
   int     m_polyGridReadFromFile;     // deprecated
   int     m_polyGridSaveToFile;       // deprecated
   CString m_polyGridReadFName;        // deprecated
   CString m_polyGridSaveFName;        // deprecated

   CString m_polyGridFName;

   int     m_logFlameLengths;
   int     m_logAnnualFlameLengths;
   int     m_logArrivalTimes;
   int     m_logPerimeters;
   int		m_logDeltaArrayUpdates;
   CString m_startingLCPFName;
   CString m_customFuelFileName;
  // CString m_startingFAParamsFName;
   //CString m_fuelMoisturesFName;

   int m_vegClassHeightUnits;
   int m_vegClassBulkDensUnits;
   int m_vegClassCanopyCoverUnits;

   CString m_lcpFNameRoot;
  // CString m_inputFNameRoot;
   //CString m_ignitionFNameRoot;

	double m_spotProbability;
	int m_crownFireMethod;
	int m_foliarMoistureContent;

//	int m_outputFirelists;
	int m_outputEnvisionFirelists;

   int m_maxProcessors;
	int m_randomSeed;
	int m_sequentialFirelists;
   // end of XML
	int m_replicateNum;
   // other stuff - can any of this go?
public:
	void CreateRandUniforms(bool useInitSeed);
	void DestroyRandUniforms();
	int ReadStaticFires();
	bool CreateLCPGenerator();
  // CString m_scenarioPath;
   CString m_scenarioFuelMoisturesFName;
   CString m_scenarioFiresFName;
  // CString m_scenarioStartingFAParamsFName;
   CString m_lcpFName;
   
   int m_runStatus;     // can be changed by other objects to signal that an error has
                         // occurred in this plugin.
   int     m_rows;
   int     m_cols;
   double  m_cellDim;

   CString m_scenarioName;
protected:
  // CString m_firesFName;
   CString m_vegClassLCPLookupFName;
   

  // CString m_scenarioInputFNameRoot;
  // CString m_scenarioIgnitionFNameRoot;

   PtrArray< FMScenario > m_scenarioArray;
   int m_scenarioID;  // id of current scenario.  This is exposed as an input variable 
   int m_lastScenarioID;  // id of last scenario.  This is for tracking scenario changes
   MapLayer *m_pPolyMapLayer;
   bool LoadXml( LPCTSTR, MapLayer* );

public:
	CString m_outputPath;
   CString m_fmsPath;   // path to fuel moisture files

	bool m_outputIgnitProbGrids;
	double m_iduBurnPercentThreshold; //a proportion which represents the proportion of IDU that needs to burn (cell wise) before an IDU is considered burned.
	bool m_deleteLCPs;
	//random number generators
	RandUniform *m_pFiresRand; // for selecting Fires
	RandUniform *m_pIgnitRand; // for selecting ignition points
	RandUniform *m_pFiresListRand; //for selecting fireslist
	void GetPolyGridStats(PolyGridLookups *pGrid);
	int maxPolyValTtl;

	CString m_strStaticFireList;
	Fire *m_staticFires;
	int m_numStaticFires;
	int m_useFirelistIgnitions;
	//for annual 'static' FlamMap Basic fire behavior
	CString m_strStaticFMS;
	int m_staticWindSpeed;
	int m_staticWindDir;
	//long m_FiresSeed;
	//long m_FireListSeed;
	//long m_ignitSeed;
	SHPHandle m_hShpPerims;
	DBFHandle m_hDbfPerims;
	int yearField;
	int runField;
	int hectareField;
	int ercField;
	int originalSizeField;
	int xCoordField;
	int yCoordField;
	int burnPerField;
	int azimuthField;
	int windSpeedField;
	int fmsField;
	int fireIDField;
	int envisionFireIDField;
	int FILfields[20];

	int processID;
	void WritePerimeterShapefile(CPerimeter *perim, int _year, int _run, int _erc, double _area_ha, double _resolution,
		double originalSize,
		double xCoord,
		double yCoord,
		int burnPer,
		int azimuth,
		int windSpeed,
		char *fms,
		char *fireID,
		int envisionFireID,
		double *FIL);
	double  m_FireSizeRatio;
	int m_maxRetries;
	int m_envisionFireID;
};


