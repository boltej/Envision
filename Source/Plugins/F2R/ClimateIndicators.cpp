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

#include "ClimateIndicators.h"
#include "F2R.h"
#include <Maplayer.h>
#include <tixml.h>
#include <EnvEngine\EnvModel.h>
#include <PathManager.h>
#include <EnvEngine\EnvConstants.h>

#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")


extern F2RProcess *theProcess;


/////////////////////////////////////////////////////////////////////
// W I L D L I F E   M O D E L 
/////////////////////////////////////////////////////////////////////

ClimateIndicatorsModel::ClimateIndicatorsModel( void )
   : m_id( CLIMATE )   
   , m_colLulc( -1 )
   , m_colYDTR(-1)
   , m_colHWE(-1)
   , m_colCWE(-1)
   , m_colETR(-1)
   , m_colGSL(-1)
   , m_colFT(-1)
   , m_colFSL(-1)
   , m_colPF(-1)
   , m_colPI(-1)
   , m_colMDD(-1)
   , m_colM3Day(-1)
   , m_colR10mm(-1)
   , m_colR1mmMay(-1)
   , m_colR1mmJune(-1)
   , m_colFPT(-1) 
   , m_colCIHU(-1) 
   , m_colP61_81(-1) 
   , m_pYDTR(NULL)
   , m_pHWE(NULL)
   , m_pCWE(NULL)
   , m_pETR(NULL)
   , m_pGSL(NULL)
   , m_pFT(NULL)
   , m_pFSL(NULL)
   , m_pPF(NULL)
   , m_pPI(NULL)
   , m_pMDD(NULL)
   , m_pM3Day(NULL)
   , m_pR10mm(NULL)
   , m_pR1mmMay(NULL)
   , m_pR1mmJune(NULL)
   , m_pFPT(NULL) 
   , m_pHU(NULL)
   , m_pCIHU(NULL)
   , m_pP61_81(NULL)
   , m_cornYieldAv(0.0f)
   , m_cornYieldTotal(0.0f)
   , m_cornArea(0.0f)
   , m_soyYieldAv(0.0f)
   , m_soyYieldTotal(0.0f)
   , m_soyArea(0.0f)
   , m_hayYieldAv(0.0f)
   , m_hayYieldTotal(0.0f)
   , m_hayArea(0.0f)
   , m_growingSeasonHeatUnits(0.0f)
   , m_growingSeasonP(0.0f)
   , m_dtr(0.0f)
   , m_gsl(0.0f)
   , m_hwe(0.0f)
   , m_cwe(0.0f)
   , m_api(0.0f)
   , m_d10mm(0.0f)
   , m_d1mmMay(0.0f)
   , m_d1mmJune(0.0f)
   , m_cdd(0.0f)
   , m_pt(0.0f)
   , m_hu(0.0f)
   , m_directory()
   {  
   theProcess->m_outVarIndexClimate = theProcess->AddOutputVar( "Average Corn Yield (bu/ha)", m_cornYieldAv, "" );
   theProcess->AddOutputVar( "Total Corn Yield (bu)", m_cornYieldTotal, "" );
   theProcess->AddOutputVar( "Total Corn Area (ha)", m_cornArea, "" );
   theProcess->AddOutputVar( "Average Soy Yield (bu/ha)", m_soyYieldAv, "" );
   theProcess->AddOutputVar( "Total Soy Yield (bu)", m_soyYieldTotal, "" );
   theProcess->AddOutputVar( "Total Soy Area (ha)", m_soyArea, "" );
   theProcess->AddOutputVar( "Average Hay Yield (tonnes/ha)", m_hayYieldAv, "" );
   theProcess->AddOutputVar( "Total Hay Yield (tonnes)", m_hayYieldTotal, "" );
   theProcess->AddOutputVar( "Total Hay Area (ha)", m_hayArea, "" );

   theProcess->AddOutputVar( "Growing Season Heat Units", m_growingSeasonHeatUnits, "" );
   theProcess->AddOutputVar( "Growing Season P", m_growingSeasonP, "" );

   theProcess->AddOutputVar( "Diurnal Temp Range", m_dtr, "" );
   theProcess->AddOutputVar( "Growing Season Length", m_gsl, "" );
   theProcess->AddOutputVar( "Hot Weather Extremes", m_hwe, "" );
   theProcess->AddOutputVar( "Cold weather extrememes", m_cwe, "" );
   theProcess->AddOutputVar( "Av Precip Intensity", m_api, "" );
   theProcess->AddOutputVar( "#Days > 10mm Precip", m_d10mm, "" );
   theProcess->AddOutputVar( "#Days in May > 1mm Precip", m_d1mmMay, "" );
   theProcess->AddOutputVar( "#Days in June > 1mm Precip", m_d1mmJune, "" );
   theProcess->AddOutputVar( "Max # Consecutive Dry Days", m_cdd, "" );
   theProcess->AddOutputVar( "Precip Total", m_pt, "" );

   int last = theProcess->AddOutputVar( "Heat Units", m_hu, "" );

   theProcess->m_outVarCountClimate = ( last - theProcess->m_outVarIndexClimate ) + 1;
   }


ClimateIndicatorsModel::~ClimateIndicatorsModel(void)
   {
   if ( m_pYDTR != NULL )
      delete m_pYDTR;

   if ( m_pHWE != NULL )
      delete m_pHWE;

   if ( m_pCWE != NULL )
      delete m_pCWE;

   if ( m_pETR != NULL )
      delete m_pETR;

   if ( m_pGSL != NULL )
      delete m_pGSL;

   if ( m_pFT != NULL )
      delete m_pFT;

   if ( m_pFSL != NULL )
      delete m_pFSL;

   if ( m_pPF != NULL )
      delete m_pPF;

   if ( m_pPI != NULL )
      delete m_pPI;

   if ( m_pMDD != NULL )
      delete m_pMDD;

   if ( m_pM3Day != NULL )
      delete m_pM3Day;

   if ( m_pR10mm != NULL )
      delete m_pR10mm;

   if ( m_pR1mmMay != NULL )
      delete m_pR1mmMay;

   if ( m_pR1mmJune != NULL )
      delete m_pR1mmJune;

   if ( m_pFPT != NULL )
      delete m_pFPT; 

   if ( m_pHU != NULL )
      delete m_pHU; 

   if ( m_pCIHU != NULL )
      delete m_pCIHU; 

   if ( m_pP61_81 != NULL )
      delete m_pP61_81; 
   }


bool ClimateIndicatorsModel::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   { 
   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;

   // check for required columns
   theProcess->CheckCol( pLayer, m_colLulc,     _T("LULC_C"),     TYPE_LONG, CC_MUST_EXIST );//Diural Temp Range
   theProcess->CheckCol( pLayer, m_colArea,     _T("AREA"),       TYPE_FLOAT, CC_MUST_EXIST );//Diural Temp Range
   theProcess->CheckCol( pLayer, m_colYDTR,     _T("CI_YDTR"),    TYPE_FLOAT, CC_AUTOADD );//Diural Temp Range
   theProcess->CheckCol( pLayer, m_colGSL,      _T("CI_GSL"),     TYPE_FLOAT, CC_AUTOADD );//Growing season length
   theProcess->CheckCol( pLayer, m_colHWE,      _T("CI_HWE"),     TYPE_FLOAT, CC_AUTOADD );//Hot weather extremes
   theProcess->CheckCol( pLayer, m_colCWE,      _T("CI_CWE"),     TYPE_FLOAT, CC_AUTOADD );//Cold weather extremes
                                                
   theProcess->CheckCol( pLayer, m_colPI,       _T("CI_PI"),      TYPE_FLOAT, CC_AUTOADD );//Average Precip intensity
   theProcess->CheckCol( pLayer, m_colR10mm,    _T("CI_R10mm"),   TYPE_FLOAT, CC_AUTOADD );//Number of Days with more than 10mm precip
   theProcess->CheckCol( pLayer, m_colR1mmMay,  _T("CI_R1mmMay"), TYPE_FLOAT, CC_AUTOADD );//Number of Days in May with more than 1mm precip
   theProcess->CheckCol( pLayer, m_colR1mmJune, _T("CI_R1mmJun"), TYPE_FLOAT, CC_AUTOADD );//Number of Days in June with more than 1mm precip
   theProcess->CheckCol( pLayer, m_colMDD,      _T("CI_MDD"),     TYPE_FLOAT, CC_AUTOADD );//Max Number of consecutive dry days
   theProcess->CheckCol( pLayer, m_colFPT,      _T("CI_FPT"),     TYPE_FLOAT, CC_AUTOADD );//Precip Total
   theProcess->CheckCol( pLayer, m_colHU,       _T("CI_HU"),      TYPE_FLOAT, CC_AUTOADD );//Heat Units
   theProcess->CheckCol( pLayer, m_colCIHU,     _T("CI_CIHU"),    TYPE_FLOAT, CC_AUTOADD );//Heat units above 50
   theProcess->CheckCol( pLayer, m_colP61_81,   _T("CI_P61_81"),  TYPE_FLOAT, CC_AUTOADD );//precip from 61-81
   theProcess->CheckCol( pLayer, m_colYield,    _T("CI_Yield"),   TYPE_FLOAT, CC_AUTOADD );//Yield

   m_pYDTR     = new FDataObj;
   m_pFT       = new FDataObj;
   m_pHWE      = new FDataObj;
   m_pCWE      = new FDataObj;
   m_pETR      = new FDataObj;
   m_pGSL      = new FDataObj;
   m_pFSL      = new FDataObj;
   m_pPF       = new FDataObj;
   m_pPI       = new FDataObj;
   m_pMDD      = new FDataObj;
   m_pM3Day    = new FDataObj;
   m_pR10mm    = new FDataObj;
   m_pR1mmMay  = new FDataObj;
   m_pR1mmJune = new FDataObj;
   m_pFPT      = new FDataObj;
   m_pHU       = new FDataObj;
   m_pCIHU     = new FDataObj;
   m_pP61_81   = new FDataObj;

   CString inPath = PathManager::GetPath( PM_IDU_DIR );  // terminated with  a'/'
   inPath += "climate\\ModelPredictions\\HADCM3";

   CString outPath = PathManager::GetPath( PM_IDU_DIR );  // terminated with a '/'
   outPath += "climate\\ModelPredictions";

   GetClimateSummary( inPath, outPath );

   bool ok = LoadXml( pEnvContext, initStr );

   if ( ! ok ) 
      return false;

   Run( pEnvContext, false );
   return true;
   }


bool ClimateIndicatorsModel::InitRun( EnvContext *pContext )
   {

   return true;
   }


bool ClimateIndicatorsModel::Run( EnvContext *pContext, bool useAddDelta )
   {
   // basic idea - iterate through the SLCs, calculating a contribution from each
   // of a variety of P influences.
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   int slcCount     = (int) theProcess->GetSLCCount();
   float totalScore = 0;
   int iduCount     = 0;

   // for each SLC
   for ( int i=0; i < slcCount; i++ )
      {
      SLC *pSLC = theProcess->GetSLC( i );

      float indicatorValue0 = -1.0f; 
      float indicatorValue1 = -1.0f;
      float indicatorValue2 = -1.0f;
      float indicatorValue3 = -1.0f;
      float indicatorValue4 = -1.0f; 
      float indicatorValue5 = -1.0f;
      float indicatorValue6 = -1.0f;
      float indicatorValue7 = -1.0f;
      float indicatorValue8 = -1.0f;
      float indicatorValue9 = -1.0f;
      float indicatorValue10 = -1.0f;
      float indicatorValue11 = -1.0f;
      float indicatorValue12 = -1.0f;
      if ( pSLC->m_slcNumber > 0 )
         {
         //get the value of the indicator for this year..
         VData valueToFind=pSLC->m_slcNumber;
      
         int year = pContext->yearOfRun+1;
         if ( year < 1 )
            year = 1;

         int SLCIndex=m_pGSL->Find(0, valueToFind);//get the row in the climate summaries that corresponds to this SLC
         m_pYDTR->Get(year,SLCIndex,indicatorValue1);//Diural Temp Range
         m_pGSL->Get(year,SLCIndex,indicatorValue0);//Growing season length
         m_pHWE->Get(year,SLCIndex,indicatorValue2);//Hot weather extremes
         m_pCWE->Get(year,SLCIndex,indicatorValue3);//Cold weather extremes
         m_pPI->Get(year,SLCIndex,indicatorValue4);//Average Precip intensity
         m_pR10mm->Get(year,SLCIndex,indicatorValue5);//Number of Days with more than 10mm precip
         m_pMDD->Get(year,SLCIndex,indicatorValue6);//Max Number of consecutive dry days
         m_pFPT->Get(year,SLCIndex,indicatorValue7);//Precip Total

         m_pHU->Get(year,SLCIndex,indicatorValue8);//Heat Units
         m_pCIHU->Get(year,SLCIndex,indicatorValue9);//Heat Units for crop insurance Yield Model
         m_pP61_81->Get(year,SLCIndex,indicatorValue10);//Heat Units for crop insurance Yield Model
         m_pR1mmMay->Get(year,SLCIndex,indicatorValue11);//Number of Days in May with more than 1mm precip
         m_pR1mmJune->Get(year,SLCIndex,indicatorValue12);//Number of Days in June with more than 1mm precip
         
         float totalScore = 0;
         // write to coverage if indicated
    //     if ( pContext->col >= 0 )
       //     {
         for ( int j=0; j < (int) pSLC->m_iduArray.GetSize(); j++ )
            {
            int idu = pSLC->m_iduArray[ j ];
            if ( useAddDelta )
               {
               float oldScore = 0;
               pLayer->GetData( idu, m_colGSL, oldScore );
               if ( fabs( oldScore - indicatorValue0 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colGSL, indicatorValue0 );

               pLayer->GetData( idu, m_colYDTR, oldScore );
               if ( fabs( oldScore - indicatorValue1 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colYDTR, indicatorValue1 );
               
               pLayer->GetData( idu, m_colHWE, oldScore );
               if ( fabs( oldScore - indicatorValue2 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colHWE, indicatorValue2 );
               
               pLayer->GetData( idu, m_colCWE, oldScore );
               if ( fabs( oldScore - indicatorValue3 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colCWE, indicatorValue3 );

               pLayer->GetData( idu, m_colPI, oldScore );
               if ( fabs( oldScore - indicatorValue4 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colPI, indicatorValue4 );

               pLayer->GetData( idu, m_colR10mm, oldScore );
               if ( fabs( oldScore - indicatorValue5 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colR10mm, indicatorValue5 );
               
               pLayer->GetData( idu, m_colR1mmMay, oldScore );
               if ( fabs( oldScore - indicatorValue11 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colR1mmMay, indicatorValue11 );

               pLayer->GetData( idu, m_colR1mmJune, oldScore );
               if ( fabs( oldScore - indicatorValue12 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colR1mmJune, indicatorValue12 );

               pLayer->GetData( idu, m_colMDD, oldScore );
               if ( fabs( oldScore - indicatorValue6 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colMDD, indicatorValue6 );

               pLayer->GetData( idu, m_colFPT, oldScore );
               if ( fabs( oldScore - indicatorValue7 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colFPT, indicatorValue7 );

               pLayer->GetData( idu, m_colHU, oldScore );
               if ( fabs( oldScore - indicatorValue8 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colHU, indicatorValue8 );

               pLayer->GetData( idu, m_colCIHU, oldScore );
               if ( fabs( oldScore - indicatorValue9 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colCIHU, indicatorValue9 );

               pLayer->GetData( idu, m_colP61_81, oldScore );
               if ( fabs( oldScore - indicatorValue10 ) > 0.0001f )
                  theProcess->AddDelta( pContext, idu, m_colP61_81, indicatorValue10 );
               }
            else
               {
               pLayer->SetData( idu, m_colGSL, indicatorValue0 );
               pLayer->SetData( idu, m_colYDTR, indicatorValue1 );
               pLayer->SetData( idu, m_colHWE, indicatorValue2 );
               pLayer->SetData( idu, m_colCWE, indicatorValue3 );
               pLayer->SetData( idu, m_colPI, indicatorValue4 );
               pLayer->SetData( idu, m_colR10mm, indicatorValue5 );
               pLayer->SetData( idu, m_colR1mmMay, indicatorValue11 );
               pLayer->SetData( idu, m_colR1mmJune, indicatorValue12 );
               pLayer->SetData( idu, m_colMDD, indicatorValue6 );
               pLayer->SetData( idu, m_colFPT, indicatorValue7 );
               pLayer->SetData( idu, m_colHU, indicatorValue8 );
               pLayer->SetData( idu, m_colCIHU, indicatorValue9 );
               pLayer->SetData( idu, m_colP61_81, indicatorValue10 );
               }
            //yield estimated from relationship between heat units above 10 C and precip total between Jun 1 and August 1.
            int lulc = -1;
            pLayer->GetData(idu,m_colLulc,lulc);
            float yield;
            GetYield(lulc,indicatorValue10,indicatorValue9,yield);
            pLayer->SetData(idu, m_colYield, yield);//bushels (corn and soy) or tons (hay)/acre
            }
         }
      totalScore += indicatorValue0;
      }  // end of: for ( idu < iduCount )
   totalScore /= slcCount;  // average over each SLC

   pContext->rawScore = totalScore;
   pContext->score = -3.0f + ( totalScore/10 )*6;

   if ( pContext->score > 3 )
      pContext->score = 3;

   if ( pContext->score < -3 )
      pContext->score = -3;

   UpdateOutputVariables(pLayer);
    
   return true;
   }


bool ClimateIndicatorsModel::UpdateOutputVariables(MapLayer *pLayer)
   {
   int slcCount     = (int) theProcess->GetSLCCount();
   int iduCount     = 0;
   float cornYieldSum=0.0f;
   float soyYieldSum=0.0f;
   float hayYieldSum=0.0f;
   float areaCorn=0.0f; float areaSoy=0.0f; float areaHay=0.0f; float areaT=0.0f;
   float gsHU=0.0f;
   float gsP=0.0f;
   float dtr_=0.0f;float gsl_=0.0f;float hwe_=0.0f;float cwe_=0.0f;float api_=0.0f;float d10mm_=0.0f, d1mmMay_=0.0f, d1mmJune_=0.0f;
   float cdd_=0.0f;float pt_=0.0f;float hu1_=0.0f;

   for ( int i=0; i < slcCount; i++ )
      {
      SLC *pSLC = theProcess->GetSLC( i );
      if (pSLC->m_slcNumber>0)
         {
         for ( int j=0; j < (int) pSLC->m_iduArray.GetSize(); j++ )
            {
            int idu = pSLC->m_iduArray[ j ];
            //get the LULC
            int lulc = -1;
            pLayer->GetData(idu,m_colLulc,lulc);

            float hu = -1;
            pLayer->GetData(idu,m_colCIHU,hu);
            float p = -1;
            pLayer->GetData(idu,m_colP61_81,p);

            float dtr = -1;
            pLayer->GetData(idu,m_colYDTR,dtr);
            float gsl = -1;
            pLayer->GetData(idu,m_colGSL,gsl);
            float hwe = -1;
            pLayer->GetData(idu,m_colHWE,hwe);
            float cwe = -1;
            pLayer->GetData(idu,m_colCWE,cwe);
            float api = -1;
            pLayer->GetData(idu,m_colPI,api);

            float d10mm = -1;
            pLayer->GetData(idu,m_colR10mm,d10mm);
            
            float d1mmMay = -1;            
            pLayer->GetData(idu,m_colR1mmMay,d1mmMay);

            float d1mmJune = -1;            
            pLayer->GetData(idu,m_colR1mmJune,d1mmJune);

            float cdd = -1;
            pLayer->GetData(idu,m_colMDD,cdd);
            float pt = -1;
            pLayer->GetData(idu,m_colFPT,pt);
            float hu1 = -1;
            pLayer->GetData(idu,m_colHU,hu1);

            float area = 0.0f;
            pLayer->GetData(idu,m_colArea,area);
            areaT    += area;//total study region Area
            gsP      += p*area;
            gsHU     += hu*area;
            dtr_     += dtr*area;
            gsl_     += gsl*area;
            hwe_     += hwe*area;
            cwe_     += cwe*area;
            api_     += api*area;
            d10mm_   += d10mm*area;
            d1mmMay_ += d1mmMay*area;
            d1mmJune_+= d1mmJune*area;
            cdd_     += cdd*area;
            pt_      += pt*area;
            hu1_     += hu1*area;

            if (lulc==15) //it is corn
               {
               float yield = 0.0f;
               pLayer->GetData(idu,m_colYield,yield);
               float areaAc=area/10000.0f/0.40469f;
               cornYieldSum+=yield*areaAc;//bushels
               areaCorn+=area/10000.0f;//ha
               }
            if (lulc==12) //it is  soy
               {
               float yield = 0.0f;
               pLayer->GetData(idu,m_colYield,yield);
               float areaAc=area/10000.0f/0.40469f;
               soyYieldSum+=yield*areaAc;
               areaSoy+=area/10000.0f;
               }
            if ( lulc==17) //it is hay
               {
               float yield = 0.0f;
               pLayer->GetData(idu,m_colYield,yield);
               float areaAc=area/10000.0f/0.40469f;
               hayYieldSum+=yield*areaAc;
               areaHay+=area/10000.0f;
               }
            }
         }
      }

   m_cornYieldAv=cornYieldSum/areaCorn;  //bushels/ha
   m_cornYieldTotal=cornYieldSum;// bushels
   m_cornArea=areaCorn;//ha
   m_soyYieldAv=soyYieldSum/areaSoy;
   m_soyYieldTotal=soyYieldSum;
   m_soyArea=areaSoy;
   m_hayYieldAv=hayYieldSum/areaHay;//tonnes/ha
   m_hayYieldTotal=hayYieldSum;//tonnes
   m_hayArea=areaHay;//tons

   m_growingSeasonHeatUnits=gsHU/areaT;
   m_growingSeasonP=gsP/areaT;

   m_dtr      = dtr_/areaT;
   m_gsl      = gsl_/areaT;
   m_hwe      = hwe_/areaT;
   m_cwe      = cwe_/areaT;
   m_api      = api_/areaT;
   m_d10mm    = d10mm_/areaT;
   m_d1mmMay  = d1mmMay_/areaT;
   m_d1mmJune = d1mmJune_/areaT;
   m_cdd      = cdd_/areaT;
   m_pt       = pt_/areaT;
   m_hu       = hu1_/areaT;

   return true;
   }


int ClimateIndicatorsModel::GetClimateSummary(LPCTSTR dir,LPCTSTR outdir)
   {
   WIN32_FIND_DATA ffd;
   TCHAR szDir[MAX_PATH];
  // size_t length_of_arg;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError=0;
  
   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.
 
   StringCchCopy(szDir, MAX_PATH, dir);
   StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
      {
      DisplayErrorBox(TEXT("FindFirstFile"));
      return dwError;
      }
   
   // List all the files in the directory with some info about them.
   FDataObj *pCol = new FDataObj;
   int i=0;

   // iterate through each file in the directory, summarizing datasets
   do {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
         {
         //_tprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
         }
      else
         {
         //filesize.LowPart = ffd.nFileSizeLow;
         //filesize.HighPart = ffd.nFileSizeHigh;

         //_tprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
         //open the first file

         FDataObj *pData = new FDataObj;
         CString n;
         n.Format("%s\\%s",dir,ffd.cFileName);
         pData->ReadAscii(n,',',TRUE);

         int slcCount = pData->GetRowCount();
         pCol->SetSize( 1, slcCount );

         int startCol=9;//air temperature
               // startcol, fileindex, #slcs, inputDO, resultsDO, colDO, function
               // function signature=fn( startCol, 3, pCol (out), pData (in) )
         SummarizeData( startCol, i, slcCount, pData, m_pYDTR, pCol, GetYearlyDiurnalTempRange);
         SummarizeData( startCol, i, slcCount, pData, m_pFT,   pCol, GetYearlyPercentFreezeThaw);
         SummarizeData( startCol, i, slcCount, pData, m_pHWE,  pCol, GetHotWeatherExtremes);
         SummarizeData( startCol, i, slcCount, pData, m_pCWE,  pCol, GetColdWeatherExtremes);
         SummarizeData( startCol, i, slcCount, pData, m_pETR,  pCol, GetETR);//intra-annual extreme temperature range
         SummarizeData( startCol, i, slcCount, pData, m_pGSL,  pCol, GetGrowingSeasonLength);
         SummarizeData( startCol, i, slcCount, pData, m_pFSL,  pCol, GetFrostSeasonLength);
         SummarizeData( startCol, i, slcCount, pData, m_pHU,   pCol, GetHeatUnits);
         SummarizeData( startCol, i, slcCount, pData, m_pCIHU, pCol, GetCropInsuranceHeatUnits);

         startCol=11;//precipitation
         SummarizeData( startCol, i, slcCount, pData, m_pPF,       pCol, GetPrecipFreq);
         SummarizeData( startCol, i, slcCount, pData, m_pPI,       pCol, GetPrecipIntensity);
         SummarizeData( startCol, i, slcCount, pData, m_pMDD,      pCol, GetMaxDryDays);
         SummarizeData( startCol, i, slcCount, pData, m_pM3Day,    pCol, GetMax3DayTotal);
         SummarizeData( startCol, i, slcCount, pData, m_pR10mm,    pCol, GetR10mm);
         SummarizeData( startCol, i, slcCount, pData, m_pR1mmMay,  pCol, GetR1mmMay);
         SummarizeData( startCol, i, slcCount, pData, m_pR1mmJune, pCol, GetR1mmJune);
         SummarizeData( startCol, i, slcCount, pData, m_pFPT,      pCol, GetPrecipTotal);
         SummarizeData( startCol, i, slcCount, pData, m_pP61_81,   pCol, GetPrecip61to81);
         i++;
         delete pData;
         }
      } while (FindNextFile(hFind, &ffd) != 0);
 
   dwError = GetLastError();
   if (dwError != ERROR_NO_MORE_FILES) 
      {
      DisplayErrorBox(TEXT("FindFirstFile"));
      }

   CString file;
   file.Format("%s\\%s.csv",outdir,"YearlyDiurnalTempRange");
   m_pYDTR->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"PercentFreezeThawDays");
   m_pYDTR->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"NumDaysOver27");
   m_pHWE->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"NumDaysUnder-20");
   m_pCWE->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"LargestDiurnalTempRange");
   m_pETR->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"GrowingSeasonLength");
   m_pGSL->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"FrostSeasonLength");
   m_pFSL->WriteAscii(file);  
   file.Format("%s\\%s.csv",outdir,"HeatUnits");
   m_pHU->WriteAscii(file); 
   file.Format("%s\\%s.csv",outdir,"CI_HeatUnits");
   m_pCIHU->WriteAscii(file); 


   file.Format("%s\\%s.csv",outdir,"PercentWetDays");
   m_pPF->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"AverageWetDayIntensity");
   m_pPI->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"MaxNumConsecDryDays");
   m_pMDD->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"Max3DayPrecip");
   m_pM3Day->WriteAscii(file);
   file.Format("%s\\%s.csv",outdir,"NumDaysOver3mm");
   m_pR10mm->WriteAscii(file);

   file.Format("%s\\%s.csv",outdir,"NumDaysOver1mmMay");
   m_pR1mmMay->WriteAscii(file);

   file.Format("%s\\%s.csv",outdir,"NumDaysOver1mmJune");
   m_pR1mmJune->WriteAscii(file);

   file.Format("%s\\%s.csv",outdir,"TotalYearlyPrecip");
   m_pFPT->WriteAscii(file); 
   file.Format("%s\\%s.csv",outdir,"TotalPrecip61_81");
   m_pP61_81->WriteAscii(file); 
   delete pCol;


   FindClose(hFind);
   return dwError;
   }


void ClimateIndicatorsModel::SummarizeData(int startCol, int index, int slcCount, FDataObj *pData, FDataObj *pResults, FDataObj *pCol, PASSFN passFn)
   {    
   if ( index == 0 )    // first file found - append the slcIDs to the dataset
      {
      pResults->SetSize(1,slcCount);   // one col (the result), a row for each SLC
      pResults->SetLabel(0,"SLCID");
      int s = pResults->GetColCount();

      for (int i=0; i < slcCount; i++ )
         {
         float value = pData->Get( 5, i );     // column 5 is POLY_ID, row is SLC index
         pResults->Set( 0, i, value );         // becomes the first column in the results
         }
      }

   passFn( startCol, 3, pCol, pData );

   CString file;
   file.Format( "yr%i", index );
   pResults->AppendCol( file );

   ASSERT(slcCount==pResults->GetRowCount() ); 

   for ( int i=0; i < slcCount; i++ )  // write the summary results (pCol) into the complete summary table (pResults)
      {
      int col = pResults->GetColCount()-1;
      float value = pCol->Get( 0, i );
      pResults->Set( col, i, value );
      }
   }


void ClimateIndicatorsModel::DisplayErrorBox(LPTSTR lpszFunction) 
   { 
   // Retrieve the system error message for the last-error code
   LPVOID lpMsgBuf;
   LPVOID lpDisplayBuf;
   DWORD dw = GetLastError(); 

   FormatMessage(
       FORMAT_MESSAGE_ALLOCATE_BUFFER | 
       FORMAT_MESSAGE_FROM_SYSTEM |
       FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       dw,
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
       (LPTSTR) &lpMsgBuf,
       0, NULL );

   // Display the error message and clean up

   lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
       (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
   StringCchPrintf((LPTSTR)lpDisplayBuf, 
       LocalSize(lpDisplayBuf) / sizeof(TCHAR),
       TEXT("%s failed with error %d: %s"), 
       lpszFunction, dw, lpMsgBuf); 

   Report::ErrorMsg( (LPCTSTR)lpDisplayBuf );
   //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

   LocalFree(lpMsgBuf);
   LocalFree(lpDisplayBuf);
   }


//-------------------------------------------------------------------
void ClimateIndicatorsModel::GetYearlyDiurnalTempRange( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();   // one for each SLC

   // iterate through SLCs
   for ( int row=0; row < rows; row++ )
      {
      float value = 0;
      int count=0;
      
      for ( int j=startCol; j < pDataIn->GetColCount(); j += (colIncrement-1) )
         {
         float v1 = pDataIn->Get( j, row );    // (col, row) - Tmin
         float v2 = pDataIn->Get( j+1, row );  // (col, row) - Tmax
         float range = v2-v1;
         value += range;
         count++;
         //j=j+colIncrement-1;
         }

      pDataOut->Set(0,row,value/count); // the average value for this row
      }
	}


void ClimateIndicatorsModel::GetYearlyPercentFreezeThaw( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      float count=0.0f;
      
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         float tMin = pDataIn->Get( j, i );
         float tMax = pDataIn->Get( j+1, i );
         if (tMin<0.0f && tMax>0.0f)
            count++;
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,count/365.0);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetHotWeatherExtremes( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int count=0;
      
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         //float v1 = pDataIn->Get( j, i );
         float v2 = pDataIn->Get( j+1, i );//tmax

         if (v2>27.0f)
            count++;
  
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,count);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetColdWeatherExtremes( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int count=0;
      
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         float v1 = pDataIn->Get( j, i );
        // float v2 = pDataIn->Get( j+1, i );//tmax

         if (v1<-20.0f)
            count++;
  
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,count);//the average value for this row
      }
	}

//Intra-annual extreme temperature range
void ClimateIndicatorsModel::GetETR( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int count=0;
      float min=1000.0f; float max=-1000.0f;
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         float min_ = pDataIn->Get( j, i );
         float max_ = pDataIn->Get( j+1, i );//tmax

         if (min_<min)
            min=min_;
         if (max_>max)
            max=max_;
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,max-min);//the average value for this row
      }
	}

//Growing season length
void ClimateIndicatorsModel::GetGrowingSeasonLength( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int count=0;
      float min=1000.0f; float max=-1000.0f;
      int startDay=0; int stopDay=0; int started=-1; int stopped=-1;int currentDay=0;
      for (int j=startCol;j<pDataIn->GetColCount()-(5*colIncrement);j++)
         {
         currentDay++;
         float tday=0.0f; int passStart=0;int passStop=0;
         for (int k=0;k<5;k++)
            {
            float min_ = pDataIn->Get( j+(k*colIncrement), i );
            float max_ = pDataIn->Get( j+1+(k*colIncrement), i );//tmax
            tday=(min_+max_)/2.0f;
            if (tday>5.0f)//this 5 day period fails
               passStart++;
            if (tday<5.0f && started==1)//we already found 5 warm days. and here is a cool day
               passStop++;
            }
         if (passStart==5 && started==-1)//we have the first period of 5 days over 5C
            {
            started=1;//indicate that we have begun the calculation
            startDay=currentDay;//mark the day when we began
            }
         if (passStop==5 && stopped==-1)//we have the first period of 5 days under 5C, but starting after the first 5 over 5
            {
            stopped=1;
            stopDay=currentDay;
            }
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,stopDay-startDay);//the average value for this row
      }
	}

//Frost season length
void ClimateIndicatorsModel::GetFrostSeasonLength( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int count=0;
      float min=1000.0f; float max=-1000.0f;
      int startDay=0; int stopDay=0; int started=-1; int stopped=-1;int currentDay=0;
      for (int j=startCol;j<pDataIn->GetColCount()-(5*colIncrement);j++)
         {
         currentDay++;
         float tday=0.0f; int passStart=0;int passStop=0;
         for (int k=0;k<5;k++)
            {
            float min_ = pDataIn->Get( j+(k*colIncrement), i );
            float max_ = pDataIn->Get( j+1+(k*colIncrement), i );//tmax
            tday=(min_+max_)/2.0f;
            if (tday>0.0f)//5 days above 0. this is the end of the frost season, in march or so. We get here first because we start in January
               passStart++;
            if (tday<0.0f && started==1)//
               passStop++;
            }
         if (passStart==5 && started==-1)//we have the first period of 5 days over 5C
            {
            started=1;//indicate that we have begun the calculation
            startDay=currentDay;//mark the day when we began
            }
         if (passStop==5 && stopped==-1)//we have the first period of 5 days under 5C, but starting after the first 5 over 5
            {
            stopped=1;
            stopDay=currentDay;
            }
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,365-stopDay+startDay);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetPrecipFreq( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      int wet=0;
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         float precip = pDataIn->Get( j, i );
         if (precip>1.f)
            wet++;

         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,wet/365.0f);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetPrecipIntensity( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int wet=0;
      float precipSum=0.0f; 
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         float precip = pDataIn->Get( j, i );
         precipSum+=precip;
         if (precip>0.1f)
            wet++;

         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,precipSum/wet);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetMaxDryDays( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      for (int j=startCol;j<pDataIn->GetColCount()-(25*colIncrement);j++)
         {
         float passStart=0.0f;
         for (int k=0;k<90;k++)//won't be longer than 90 days...
            {
            float precip = pDataIn->Get( j+(k*colIncrement), i );
            if (precip<0.1f)//5 days above 0. this is the end of the frost season, in march or so. We get here first because we start in January
               passStart++;
            else
               {
               if (passStart>value)
                  value=passStart;
               break;
               }
            }     
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,value);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetMax3DayTotal( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;float total=0.0f;
      for (int j=startCol;j<pDataIn->GetColCount()-(25*colIncrement);j++)
         {
         int passStart=0;
         for (int k=0;k<3;k++)
            {
            float precip = pDataIn->Get( j+(k*colIncrement), i );
            total+=precip;
            if (total>value)
               value=total;
            }     
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,value);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetR10mm( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      for (int j=startCol;j<pDataIn->GetColCount()-(25*colIncrement);j++)
         {
         float passStart=0.0f;
         for (int k=0;k<90;k++)
            {
            float precip = pDataIn->Get( j+(k*colIncrement), i );
            if (precip>10.f)//5 days above 0. this is the end of the frost season, in march or so. We get here first because we start in January
               passStart++;
            else
               {
               if (passStart>value)
                  value=passStart;
               break;
               }
            }     
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,value);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetR1mmMay( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();   // one slc per row

   for ( int i=0; i < rows; i++ )   // iterate through slc's
      {
      int precipDays = 0;
      int j=startCol + 120*colIncrement;  // startcol=jan1, j=May1 starting column

      for (int k=0; k < 31; k++ )  // 31 days in may  (startCol indicates May 1)
         {
         float precip = pDataIn->Get( j+(k*colIncrement), i );
         if (precip>1.f)  //5 days above 0. this is the end of the frost season, in march or so. We get here first because we start in January
            precipDays++;
          }

      pDataOut->Set(0,i,precipDays);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetR1mmJune( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();   // one slc per row

   for ( int i=0; i < rows; i++ )   // iterate through slc's
      {
      int precipDays = 0;
      int j=startCol + 150*colIncrement;  // startcol=jan1, j=June1 starting column

      for (int k=0; k < 30; k++ )  // 30 days in June  (startCol indicates June 1)
         {
         float precip = pDataIn->Get( j+(k*colIncrement), i );
         if (precip>1.f)  //5 days above 0. this is the end of the frost season, in march or so. We get here first because we start in January
            precipDays++;
          }

      pDataOut->Set(0,i,precipDays);//the average value for this row
      }
	}



void ClimateIndicatorsModel::GetPrecipTotal( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int wet=0;
      float precipSum=0.0f; 
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         float precip = pDataIn->Get( j, i );
         precipSum+=precip;
         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,precipSum);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetPrecip61to81( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();
   int rowsOut = pDataOut->GetRowCount();
   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int wet=0;
      float precipSum=0.0f; 
      int dayCount=0;
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         if (dayCount > 150 && dayCount <210)//between june 1 and august 1
            {
            float precip = pDataIn->Get( j, i );
            precipSum+=precip;
            }
         dayCount++;
         j=j+colIncrement-1;
         }
      ASSERT(rows==rowsOut);
      pDataOut->Set(0,i,precipSum);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetHeatUnits( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();

   for ( int i=0; i < rows; i++ )
      {
      float value = 0;
      int count=0;
      float ymin=1000.0f; 
      float ymax=-1000.0f; 
      float dailyCHU=0.0f; 
      float CHU=0.0f;
      int started=-1; int stopped=-1;

      for (int j=startCol; j < pDataIn->GetColCount(); j++)
         {
         float min_ = pDataIn->Get( j, i );
         float max_ = pDataIn->Get( j+1, i );//tmax
         float mean = (min_+max_)/2.0f;

         if (min_>6.5f)
            {
            (max_<10.0f) ? 0.0f : ymax=(3.33f*(max_-10.0f))-(0.084f*pow((max_-10.0f),2));
            (min_<4.44)  ? 0.0f : ymin=1.8f*(min_-4.44f);
            }

         if (mean>12.8f)
            started=1;

         if (min_<6.5f && started>0)
            stopped=1;

         if (started>0 && stopped <0)
            CHU+=(ymax+ymin)/2.0f;

         j=j+colIncrement-1;
         }
      pDataOut->Set(0,i,CHU);//the average value for this row
      }
	}



void ClimateIndicatorsModel::GetCropInsuranceHeatUnits( int startCol, int colIncrement, FDataObj *pDataOut, FDataObj *pDataIn )
   {
   int rows = pDataIn->GetRowCount();
   int rowsOut = pDataOut->GetRowCount();
   for ( int i=0; i < rows; i++ )
      {
      float CHU=0.0f;
      int dayCount=0;
      for (int j=startCol;j<pDataIn->GetColCount();j++)
         {
         if (dayCount > 150 && dayCount <240)//between june 1 and august 31
            {
            float min_ = pDataIn->Get( j, i );
            float max_ = pDataIn->Get( j+1, i );//tmax
            float mean = (min_+max_)/2.0f;

            if (mean>10.0f)
               CHU+=mean;
            }
         dayCount++;
         j=j+colIncrement-1;
         }
      ASSERT(rows==rowsOut);
      pDataOut->Set(0,i,CHU);//the average value for this row
      }
	}


void ClimateIndicatorsModel::GetYield(int lulc,float precipJun1toAugust1, float heatUnitsAbove10, float &yield)
   {
   yield = 0.0f;
   if (lulc==15)//corn bu/acre
      yield = (0.029795f*heatUnitsAbove10)+(0.015804f*precipJun1toAugust1)+95.23247f;
   if (lulc==12)//soybeans bu/acre
      yield = (0.012931f*heatUnitsAbove10)+(0.004948f*precipJun1toAugust1)+26.59982f;
   if (lulc==17)//hay tonnes/acre
      yield = (-0.00019f*heatUnitsAbove10)+(0.001687f*precipJun1toAugust1)+3.813044f; 
   }


bool ClimateIndicatorsModel::LoadXml( EnvContext *pContext, LPCTSTR filename )
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
   
   TiXmlElement *pXmlModel = pXmlRoot->FirstChildElement( "climate" );
   if ( pXmlModel == NULL )
      {
      CString msg( "Climate Model: missing <climate> element in input file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   if ( pXmlModel == NULL )
      return false;

   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   // lookup fields
   LPTSTR directoryIn   = NULL;
   LPTSTR directoryOut   = NULL;
   XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                      { "directoryIn",      TYPE_STRING,   &directoryIn,   true,  0 },
                      { "directoryOut",     TYPE_STRING,   &directoryOut,  true,  0 },
                      { NULL,               TYPE_NULL,     NULL,           false, 0 } };

   ok = TiXmlGetAttributes( pXmlModel, attrs, filename, pLayer );
   if (!ok)
      return false;

   //GetClimateSummary( directoryIn,directoryOut);
   //m_directory=directory;

   
   return true;
   }