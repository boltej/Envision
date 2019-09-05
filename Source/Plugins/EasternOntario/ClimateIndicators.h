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


class ClimateIndicators
{
public:
   ClimateIndicators( void );
   ~ClimateIndicators(void);

   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext );
   bool Run    ( EnvContext *pContext, bool useAddDelta=true );
   bool LoadXml(EnvContext *pContext, LPCTSTR filename);
   int m_id;
   

protected:
   CString m_lulcField;
      
   int m_colLulc;    // idu coverage
   int m_colArea;    // "AREA"
   int m_colYDTR;    // Diural Temp Range
   int m_colGSL;     // Growing season length
   int m_colHWE;     // Hot weather extremes
   int m_colCWE;     // Cold weather extremes
   int m_colPI;      // Average Precip intensity
   int m_colR10mm;   // Number of Days with more than 10mm precip
   int m_colMDD;     // Max Number of consecutive dry days
   int m_colFPT;     // Precip Total

   int m_colETR;
   int m_colFT;
   int m_colFSL;
   int m_colPF;
   int m_colM3Day;

   int m_colHU;
   int m_colCIHU;
   int m_colP61_81;
   int m_colYield;

   float m_cornYieldAv;
   float m_cornYieldTotal;
   float m_cornArea;
   float m_soyYieldAv;
   float m_soyYieldTotal;
   float m_soyArea;
   float m_hayYieldAv;
   float m_hayYieldTotal;
   float m_hayArea;
   float m_growingSeasonHeatUnits;
   float m_growingSeasonP;

   float m_dtr;
   float m_gsl;
   float m_hwe;
   float m_cwe;
   float m_api;
   float m_d10mm;
   float m_cdd;
   float m_pt;
   float m_hu;


   void DisplayErrorBox(LPTSTR lpszFunction);
   int GetClimateSummary(LPCTSTR dir,LPCTSTR outdir);
   static void GetYearlyDiurnalTempRange( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetHotWeatherExtremes( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetColdWeatherExtremes( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetETR( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetGrowingSeasonLength( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetFrostSeasonLength( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetYearlyPercentFreezeThaw( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetPrecipFreq( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetPrecipIntensity( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetMaxDryDays( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetMax3DayTotal( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetR10mm( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetPrecipTotal( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetHeatUnits( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetCropInsuranceHeatUnits( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetPrecip61to81( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   static void GetYield(int idu,float precipJun1toAugust1,float heatUnitsAbove10, float &yield);


   bool UpdateOutputVariables(MapLayer *pLayer);

   FDataObj *m_pYDTR;
   FDataObj *m_pFT;
   FDataObj *m_pHWE;
   FDataObj *m_pCWE;
   FDataObj *m_pETR;
   FDataObj *m_pGSL;
   FDataObj *m_pFSL;

   FDataObj *m_pPF;
   FDataObj *m_pPI;
   FDataObj *m_pMDD;
   FDataObj *m_pM3Day;
   FDataObj *m_pR10mm;
   FDataObj *m_pFPT;

   FDataObj *m_pHU;
   FDataObj *m_pCIHU;

   FDataObj *m_pP61_81;

   CString m_directory;

   typedef void (*PASSFN)( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn );
   void SummarizeData(int startCol,int count, int slcNum, FDataObj *pData, FDataObj *pResults, FDataObj *pCol, PASSFN passFn);

  
};