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
// EvapTrans.cpp : Implements global evapotransporation methods for Flow
//

#include "stdafx.h"
#pragma hdrstop

#include "GlobalMethods.h"
#include <DATE.HPP>
#include "Flow.h"
#include <UNITCONV.H>
#include <omp.h>
#include <EnvConstants.h>
#include <vector>

#include <PathManager.h>


using namespace std;

//extern FlowProcess *gpFlow;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define EPSILON  0.00000001
#define FP_EQUAL(a, b)             (fabs(a-b) <= EPSILON)

EvapTrans::EvapTrans(FlowModel *pFlowModel, LPCTSTR name)
   : GlobalMethod(pFlowModel, name, GM_NONE)
   , m_ETEq(this)                                                  // the type of ET equation to use for calculation of the reference ET (ASCE, FAO56, Penn_Mont, Kimb_Penn, Hargreaves)
   , m_flowContext(NULL)                                           // pointer to flowContext to inform ETEquation
   , m_pCropTable(NULL)                                            // pointer to Crop table (.csv) holding crop coefficents
   , m_pSoilTable(NULL)                                            // pointer to Soil table (.csv) holding soil values
   , m_pStationTable(NULL)                                         // pointer to the 5 Agrimet Station coefficents for Kimberly-Penman reference ET calculation
   , m_phruCropStartGrowingSeasonArray(NULL)                       // FDataObj holding the planting doy for every crop for every HRU
   , m_phruCropEndGrowingSeasonArray(NULL)                         // FDataObj holding the harvest doy for every crop for every HRU
   , m_phruCurrCGDDArray(NULL)                                     // FDataObj holding the cummulative Growing Degree Days for the crop since planted
   , m_pLandCoverCoefficientLUT(NULL)                              // FDataObj holing the land cover coefficents representing percentile stages of growth 
   //  , m_pDailyReferenceET( NULL )                                 // DEBUGGING: Uncomment to output reference ET for a particular HRU
   , m_bulkStomatalResistance(1000.0f)                             // Bulk Stomatal Resistance for Penman Monteith reference ET calculation 
   , m_latitude(45.0f)                                             // latitude needed for reference ET calculation
   , m_longitude(123.0f)                                             // longitue needed for reference ET calculation
   , m_pLayerDists(NULL)                                           // pointer to the soil layer distribution
   , m_colLulc(-1)                                                 // LULC_B idu layer column ("LULC_B")
   , m_colIduSoilID(-1)                                            // Soil ID idu layer column ("SoilID")
   , m_colArea(-1)                                                 // Area idu layer column ("AREA")
   , m_colAnnAvgET(-1)                                             // Annual ET idu layer column ("ET_YR")
   , m_colAnnAvgMaxET(-1)                                          // Annual Maximum ET idu layer column ("MAX_ET_YR")
   , m_colDailyET(-1)
   , m_colDailyMaxET(-1)                                           // Daily Maximum ET idu layer column ("MAX_ET_DAY")
   , m_colDailySoilMoisture(-1)                                    // Daily Soil Moisture idu layer column ("SM_DAY")
   , m_colIrrigation(-1)
   , m_colIDUIndex(-1)
   , m_colYieldReduction(-1)                                       // Crop Yield reduction idu layer column ("YieldReduc")
   , m_calcFracCGDD(false)
   , m_colLAI(-1)                                                  // Leaf Area Index idu layer column ("LAI")
   , m_colAgeClass(-1)                                             // Age Class idu layer column ("AGECLASS")
   , m_colHeight(-1)
   , m_colAgrimetStationID(-1)                                     // Agrimet Station ID idu layer column, only needed for Kimberly-Penman reference ET calculation
   , m_colSoilTableSoilID(-1)                                        // Soil Table ID column to match idu layer SOILID            
   , m_colWP(-1)                                                   // Wilting Point of soil ("WP")
   , m_colLP(-1)                                                   // Penetration Depth of soil ("LP")
   , m_colFC(-1)                                                   // Field Capacity of soil ("FC")
   , m_cropCount(-1)                                               // Crop count of the number of crops modeled
   , m_colCropTableLulc(-1)                                        // Crop Table LULC_B column to match idu layer LULC_B
   , m_colPlantingMethod(-1)                                       // Planting Method (1 = T30, 2 = CGDD) crop table column  
   , m_colPlantingThreshold(-1)                                    // Threshold of either Temp or Heat Unit (dependent on Planting Method) crop table column 
   , m_colT30Baseline(-1)
   , m_colEarliestPlantingDate(-1)                                 // Earliest Planting Day of the Year crop table column 
   , m_colGddBaseTemp(-1)                                          // Growing Degree Day Base Temp crop table column
   , m_colTermMethod(-1)                                           // Harvest Method (1=CGDD, 2=Frost, 3=Happen First (1 or 2), 4=Desert Vegetation Class) crop table column
   , m_colGddEquationType(-1)                                      // GDD Equation either for Corn or all other crops (1-= other crops, 2=corn) crop table column
   , m_colTermCGDD(-1)                                             // Harvest Method by CGDD crop table column
   , m_colTermTemp(-1)                                             // Harvest Method by Frost crop table column
   , m_colMinGrowSeason(-1)
   , m_colBinToEFC(-1)
   , m_colCGDDtoEFC(-1)
   , m_colUseGDD(-1)
   , m_colBinEFCtoTerm(-1)
   , m_colBinMethod(-1)
   , m_colConstCropCoeffEFCtoT(-1)
   , m_colDepletionFraction(-1)
   , m_colYieldResponseFactor(-1)                                  // Growing Season Yield Reduction Factor crop table column
   , m_colPrecipThreshold(-1)                                      // last sseven days of precipitation threshold for planting
   , m_colStartGrowSeason(-1)                                      // doy (=day of year) for start of growing season in idu layer column ("PLANTDATE")
   , m_colEndGrowSeason(-1)                                        // doy (=day of year) for end of growing season in idu layer column ("HARVDATE")
   , m_colStationTableID(-1)
   , m_colStationCoeffOne(-1)
   , m_colStationCoeffTwo(-1)
   , m_colStationCoeffThree(-1)
   , m_colStationCoeffFour(-1)
   , m_colStationCoeffFive(-1)
   , m_stationLatitude(0)
   , m_stationLongitude(0)
   , m_stationCount(0)
   , m_currReferenceET(0)
   , m_currMonth(-1)                                               // needed only for Hargreaves reference ET calculation, could be moved to ETEquation based on doy
   , m_runCount(0)                                                 // run count for integration
   {
   this->m_timing = GMT_CATCHMENT | GMT_START_STEP;               // Called during GetCatchmentDerivatives()
   this->m_ETEq.SetMode(ETEquation::HARGREAVES);                // default reference ET is Hargreaves

   }


EvapTrans::~EvapTrans()
   {
   if (m_pCropTable)
      delete m_pCropTable;

   if (m_pSoilTable)
      delete m_pSoilTable;

   if (m_pStationTable)
      delete m_pStationTable;

   if (m_pLayerDists)
      delete m_pLayerDists;

   if (m_phruCropStartGrowingSeasonArray)
      delete  m_phruCropStartGrowingSeasonArray;

   if (m_phruCropEndGrowingSeasonArray)
      delete m_phruCropEndGrowingSeasonArray;

   if (m_phruCurrCGDDArray)
      delete  m_phruCurrCGDDArray;

   if (m_pLandCoverCoefficientLUT)
      delete m_pLandCoverCoefficientLUT;

   /* if ( m_pDailyReferenceET != NULL )
       delete m_pDailyReferenceET;*/
   }


bool EvapTrans::Init(FlowContext *pFlowContext)
   {
   // compile query
   GlobalMethod::Init(pFlowContext);

   FlowModel *pFlowModel = pFlowContext->pFlowModel;

   if (m_method != GM_PENMAN_MONTIETH && m_method != GM_BAIER_ROBERTSON && m_method != GM_HARGREAVES_1985)
      {
      // DEBUGGING: use to output reference ET for a particular HRU

      /*m_pDailyReferenceET = new FDataObj(2, 0);
      m_pDailyReferenceET->SetName(_T("Daily ReferenceET calculation"));
      m_pDailyReferenceET->SetLabel(0, _T("Time (days)"));
      m_pDailyReferenceET->SetLabel(1, _T("ReferenceET (mm/day)"));

      gpFlow->AddOutputVar(_T("Daily ReferenceET Summary"), m_pDailyReferenceET, "");*/

      pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, this->m_colStartGrowSeason, _T("PLANTDATE"), TYPE_INT, CC_AUTOADD);
      pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, this->m_colEndGrowSeason, _T("HARVDATE"), TYPE_INT, CC_AUTOADD);

      int hruCount = pFlowModel->GetHRUCount();

      /*m_hruT30OldestIndexArray.SetSize( hruCount );
      m_hruIsT30FilledArray.SetSize( hruCount );*/

      /*for ( int i = 0; i < hruCount; i++)
        {
        m_hruT30OldestIndexArray[ i ] = 0;
        m_hruIsT30FilledArray[i] = false;
        }*/

        //  FDataObj m_hruT30Array(hruCount, 30, -273.0F);


        // Create and initialize the planting and harvest doy and cumulative gorwing degree days since planting lookup tables
      m_phruCropStartGrowingSeasonArray = new IDataObj(hruCount, m_cropCount, 999, U_UNDEFINED);
      m_phruCropEndGrowingSeasonArray = new IDataObj(hruCount, m_cropCount, -999, U_UNDEFINED);
      m_phruCurrCGDDArray = new FDataObj(hruCount, m_cropCount, 0.0f, U_UNDEFINED);


      if (m_method == GM_KIMBERLY_PENNMAN)
         {
         pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colAgrimetStationID, _T("AgStaID"), TYPE_LONG, CC_MUST_EXIST);
         pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colYieldReduction, _T("YieldReduc"), TYPE_LONG, CC_AUTOADD);            // TODO: make EvapTrans tag option
         }
      }

   // Evapotranspiration and Soil Moisture IDU attributes calculated 
   pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colDailyET, _T("ET_DAY"), TYPE_FLOAT, CC_AUTOADD);
   pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colDailyMaxET, _T("MAX_ET_DAY"), TYPE_FLOAT, CC_AUTOADD);
   pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colAnnAvgET, _T("ET_YR"), TYPE_FLOAT, CC_AUTOADD);
   pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colAnnAvgMaxET, _T("MAX_ET_YR"), TYPE_FLOAT, CC_AUTOADD);
   pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colDailySoilMoisture, _T("SM_DAY"), TYPE_FLOAT, CC_AUTOADD);
   pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colIDUIndex, _T("IDU_INDEX"), TYPE_INT, CC_AUTOADD);
   //pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colIrrigation, _T("IRRIGATION"), TYPE_INT, CC_MUST_EXIST);
   //pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colIduSoilID, _T("SoilID"), TYPE_LONG, CC_AUTOADD);

   // IDU attributes needed for Penman-Monteith reference ET calculation
   if (m_method == GM_PENMAN_MONTIETH)
      {
      pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colLAI, _T("LAI"), TYPE_FLOAT, CC_MUST_EXIST);
      pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colAgeClass, _T("AGECLASS"), TYPE_INT, CC_MUST_EXIST);
      pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colHeight, _T("height"), TYPE_FLOAT, CC_MUST_EXIST);
      }

   return true;
   }


bool EvapTrans::InitRun(FlowContext *pFlowContext)
   {
   GlobalMethod::InitRun(pFlowContext);

   // m_pDailyReferenceET->ClearRows();

   //run dependent setup
   m_currMonth = -1;
   m_flowContext = pFlowContext;

   return true;
   }


bool EvapTrans::StartYear(FlowContext *pFlowContext)
   {
   GlobalMethod::StartYear(pFlowContext);  // this inits the m_hruQueryStatusArray to 0;

   FlowModel *pFlowModel = pFlowContext->pFlowModel;
   MapLayer  *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

   int iduCount = (int)pFlowContext->pEnvContext->pMapLayer->GetRecordCount();

   for (int i = 0; i < iduCount; i++)
      {
      m_iduIrrRequestArray[i] = 0.0f;
      m_iduAnnAvgETArray[i] = 0.0f;
      m_iduAnnAvgMaxETArray[i] = 0.0f;
      m_iduSeasonalAvgETArray[i] = 0.0f;
      m_iduSeasonalAvgMaxETArray[i] = 0.0f;
      if (!pFlowModel->m_estimateParameters)
         {
         pFlowModel->UpdateIDU(pFlowContext->pEnvContext, i, m_colAnnAvgET, m_iduAnnAvgETArray[i], ADD_DELTA);         // mm/year
         pFlowModel->UpdateIDU(pFlowContext->pEnvContext, i, m_colAnnAvgMaxET, m_iduAnnAvgMaxETArray[i], ADD_DELTA);   // mm/year
         }
      }

   //run dependent setup

   m_flowContext = pFlowContext;

   m_runCount = 0;

   FillLookupTables(pFlowContext);

   // write planting dates to map
   if (m_pCropTable)
      {
      for (int h = 0; h < pFlowModel->GetHRUCount(); h++)
         {
         HRU *pHRU = pFlowModel->GetHRU(h);

         for (int i = 0; i < (int)pHRU->m_polyIndexArray.GetSize(); i++)
            {
            int idu = pHRU->m_polyIndexArray[i];

            int lulc = -1;
            if (pIDULayer->GetData(idu, m_colLulc, lulc))
               {
               // have lulc for this HRU, find the corresponding row in the cc table
               int row = m_pCropTable->Find(m_colCropTableLulc, VData(lulc), 0);

               int startDate = 999;
               int harvestDate = -999;
               if (row >= 0)
                  {
                  startDate = this->m_phruCropStartGrowingSeasonArray->Get(h, row);
                  harvestDate = this->m_phruCropEndGrowingSeasonArray->Get(h, row);
                  }

               if (!pFlowModel->m_estimateParameters)
                  {
                  pFlowModel->UpdateIDU(pFlowContext->pEnvContext, idu, m_colStartGrowSeason, startDate, ADD_DELTA);
                  pFlowModel->UpdateIDU(pFlowContext->pEnvContext, idu, m_colEndGrowSeason, harvestDate, ADD_DELTA);
                  }
               }
            }
         }
      }
   ////////////////
   //Run( pFlowContext );
   ////////////////
   return true;
   }


bool EvapTrans::StartStep(FlowContext *pFlowContext)
   {
   // populate the Crop Table with begining of step values:
   if (m_pCropTable)
      {
      int hruCount = pFlowContext->pFlowModel->GetHRUCount();
      int doy = pFlowContext->dayOfYear;

      // iterate through HRUs, setting values for each hru X crop for the cummulative degree
      // days since planted (m_phruCurrCGDDArray)
#pragma omp parallel for 
      for (int hruIndex = 0; hruIndex < hruCount; hruIndex++)
         {
         HRU *pHRU = pFlowContext->pFlowModel->GetHRU(hruIndex);

         // iterate through crops, filling cGDD array with the cumulative growing degree days since planted
         for (int row = 0; row < m_cropCount; row++)
            {
            float baseTemp = m_pCropTable->GetAsFloat(this->m_colGddBaseTemp, row);
            int gddEquationType = m_pCropTable->GetAsInt(this->m_colGddEquationType, row);
            int useGDD = m_pCropTable->GetAsInt(this->m_colUseGDD, row);

            bool isCorn = (gddEquationType == 1) ? false : true;
            bool isGDD = (useGDD == 1) ? true : false;

            // if doy is within growing season
            int plantingDoy = m_phruCropStartGrowingSeasonArray->Get(hruIndex, row);
            int harvestDoy = m_phruCropEndGrowingSeasonArray->Get(hruIndex, row);

            if (doy >= plantingDoy && doy <= harvestDoy)
               {
               if (isGDD)
                  {
                  float cGDD = m_phruCurrCGDDArray->Get(hruIndex, row);
                  CalculateCGDD(pFlowContext, pHRU, baseTemp, doy, isCorn, cGDD);
                  m_phruCurrCGDDArray->Set(hruIndex, row, cGDD);
                  }
               else
                  m_phruCurrCGDDArray->Set(hruIndex, row, -1);
               }  // end doy in growing season
            }  // end row 
         } // end hru 
      } // end m_pCropTable

   return true;
   }


bool EvapTrans::Step(FlowContext *pFlowContext)
   {
   if (GlobalMethod::Step(pFlowContext) == true)
      return true;

   if (pFlowContext == NULL)
      return false;

   int hruCount = pFlowContext->pFlowModel->GetHRUCount();

   // iterate through hrus/hrulayers 
#pragma omp parallel for 
   for (int h = 0; h < hruCount; h++)
      {
      //		float aet = 0.0f;
      //		float maxET = 0.0f;
      float cgdd0 = 0.0f;

      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      int doy = pFlowContext->dayOfYear;

      if (DoesHRUPassQuery(h))
         {
         if (!pFlowContext->pFlowModel->m_pGrid)
            {
            // TODO: move this to Flow -- only works for one instance of EvapTrans
            CalculateCGDD(pFlowContext, pHRU, 0, doy, 0, cgdd0);
            pHRU->m_currentCGDD += cgdd0;                                           // Celsius heat units

            GetHruET(pFlowContext, pHRU, h);
            }
         else
            GetHruET(pFlowContext, pHRU, h, true);

         }
      }

   //FlowModel *pModel = pFlowContext->pFlowModel;
   //MapLayer  *pIDULayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;
   //
   //int iduCount = (int) pFlowContext->pEnvContext->pMapLayer->GetRecordCount();
   //
   //
   //pIDULayer->m_readOnly = false;
   //
   //// write annual ET to map
   //for ( int h=0; h < pModel->GetHRUCount(); h++ )
   //   {
   //   HRU *pHRU = pModel->GetHRU( h );
   //   if ( this->m_hruQueryStatusArray[ h ] & (1<< this->m_id ) )
   //      {
   //      for ( int i=0; i < (int) pHRU->m_polyIndexArray.GetSize(); i++ )
   //         {
   //         int idu = pHRU->m_polyIndexArray[ i ];
   //         pIDULayer->SetData( idu, colET, pHRU->m_currentET );
   //         }
   //      }
   //   }
   //

   m_runCount++;

   //CString msg;
   //msg.Format( _T("EvapTrans:Run() '%s' processed %i IDUs"), this->m_name, idusProcessed );
   //Report::Log( msg );
   return TRUE;
   }


bool EvapTrans::EndStep(FlowContext *pFlowContext)
   {

   // needed to watch referenceET for a specific hru/idu
   //if (m_pCropTable)
   //{
   //   float referenceETValues[2];
   //   //   referenceETValues.SetSize( 2 );
   //   float time = pFlowContext->time - pFlowContext->pEnvContext->startYear * 365;
   //   referenceETValues[0] = time;
   //   referenceETValues[1] = this->m_currReferenceET;
   //   m_pDailyReferenceET->AppendRow(referenceETValues, 2);
   //   this->m_currReferenceET = 0.0F;
   //}


   // temporary
   //FlowModel *pModel = pFlowContext->pFlowModel;
   //MapLayer  *pIDULayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;
   //
   //int iduCount = (int) pFlowContext->pEnvContext->pMapLayer->GetRecordCount();
   //
   //
   //pIDULayer->m_readOnly = false;
   //
   //// write annual ET to map
   //for ( int h=0; h < pModel->GetHRUCount(); h++ )
   //   {
   //   HRU *pHRU = pModel->GetHRU( h );
   //   if ( this->m_hruQueryStatusArray[ h ] & (1<< this->m_id ) )
   //      {
   //      for ( int i=0; i < (int) pHRU->m_polyIndexArray.GetSize(); i++ )
   //         {
   //         int idu = pHRU->m_polyIndexArray[ i ];
   //         pIDULayer->SetData( idu, colET, pHRU->m_currentET );
   //         }
   //      }
   //   }
   //
   //pIDULayer->m_readOnly = true;
   return true;
   }


bool EvapTrans::EndYear(FlowContext *pFlowContext)
   {
   FlowModel *pFlowModel = pFlowContext->pFlowModel;
   MapLayer  *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

   int iduCount = (int)pFlowContext->pEnvContext->pMapLayer->GetRecordCount();


   // write annual ET and Yield Reduction fraction to map
   for (int i = 0; i < iduCount; i++)
      {
      //  m_iduAnnAvgETArray[ i ] /*/= m_runCount*/;   // runcount =365*4  // mm/day
      if (!pFlowModel->m_estimateParameters)
         {
         pFlowModel->UpdateIDU(pFlowContext->pEnvContext, i, m_colAnnAvgET, m_iduAnnAvgETArray[i], ADD_DELTA);         // mm/year
         pFlowModel->UpdateIDU(pFlowContext->pEnvContext, i, m_colAnnAvgMaxET, m_iduAnnAvgMaxETArray[i], ADD_DELTA);   // mm/year

         float yieldFraction = 1.0f;

         // output yield reduction ratio values to map if it is a crop
         if (m_pCropTable)
            {
            // get lulc id from HRU
            int lulc = -1;
            int row = -1;
            if (pIDULayer->GetData(i, m_colLulc, lulc))
               {
               // have lulc for this HRU, find the corresponding row in the cc table
               row = m_pCropTable->Find(m_colCropTableLulc, VData(lulc), 0);
               if (row >= 0 && m_colYieldResponseFactor >= 0 && m_iduSeasonalAvgMaxETArray[i] > 0.0f)
                  {
                  float ratio = m_iduSeasonalAvgETArray[i] / m_iduSeasonalAvgMaxETArray[i];
                  float ky = m_pCropTable->GetAsFloat(this->m_colYieldResponseFactor, row);
                  yieldFraction = 1.0f - (ky * (1.0f - ratio));   // (1 - Ya/Ym)
                  }  // end 
               }   // end lulc match
            }  // end cropTable

         if (yieldFraction < 0.0f) yieldFraction = 1.0f;
         if (yieldFraction > 1.0f) yieldFraction = 0.0f;
         if (m_colYieldReduction > 0) pFlowModel->UpdateIDU(pFlowContext->pEnvContext, i, m_colYieldReduction, yieldFraction, ADD_DELTA);     // fraction
         }
      }

   return true;
   }


bool EvapTrans::SetMethod(GM_METHOD method)
   {
   m_method = method;

   switch (method)
      {
      case GM_ASCE:                m_ETEq.SetMode(ETEquation::ASCE);        break;
      case GM_FAO56:               m_ETEq.SetMode(ETEquation::FAO56);       break;
      case GM_HARGREAVES:          m_ETEq.SetMode(ETEquation::HARGREAVES);  break;
      case GM_HARGREAVES_1985:     m_ETEq.SetMode(ETEquation::HARGREAVES_1985);  break;
      case GM_PENMAN_MONTIETH:     m_ETEq.SetMode(ETEquation::PENN_MONT);   break;
      case GM_KIMBERLY_PENNMAN:    m_ETEq.SetMode(ETEquation::KIMB_PENN);   break;

      default:
         return false;
      }

   return true;
   }

//void EvapTrans::CalculateFrost(FlowContext *pFlowContext, HRU *pHRU, int &lastSpringFrostDoy, int &numFrostFreeDays, int &firstFallFrostDoy)
//{
//	float tMin = 0.0f;
//
//	int doy = pFlowContext->dayOfYear;
//
//	if (doy <= 171)
//	{
//		pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
//		if (tMin <= 0)
//		{
//			lastSpringFrostDoy = doy;
//			numFrostFreeDays = 0;
//		}
//	}
//	/*else
//	{
//		pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
//		if (tMin <= 0 )
//		{
//			firstFallFrostDoy = doy;
//	}*/
//
//	if (doy > lastSpringFrostDoy)
//		numFrostFreeDays++;
//}

// Returns a cumulative GDD which can take a date range and/or a CGDD seed
// date range can be a single day 
// seed can be 0.0 or a nonzero number
void EvapTrans::CalculateCGDD(FlowContext *pFlowContext, HRU *pHRU, float baseTemp, int startDoy, int endDoy, bool isCorn, float *cGDD)
   {

   float tMean = 0;
   float tMin = 0;
   float tMax = 0;
   float val1 = 0;
   float val2 = 0;

   for (int i = startDoy; i <= endDoy; i++)
      {
      if (isCorn)
         {
         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, i, tMax);
         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, i, tMin);
         val1 = min(tMax, 30);
         val2 = min(tMin, 30);

         *cGDD += (max(val1, 10) + max(val2, 10)) / 2 - 10;
         }
      else
         {
         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, i, tMean);
         *cGDD += max((tMean - baseTemp), 0);
         }
      }
   }

// Returns a cumulative GDD for a single day
void EvapTrans::CalculateCGDD(FlowContext *pFlowContext, HRU *pHRU, float baseTemp, int doy, bool isCorn, float &cGDD)
   {
   FlowModel *pFlowModel = pFlowContext->pFlowModel;

   float tMean = 0.0f;
   float tMin = 0.0f;
   float tMax = 0.0f;
   float val1 = 0.0f;
   float val2 = 0.0f;

   if (cGDD < 0.0f)
      {
      cGDD = 0.0f;
      }

   // determine cGDD for corn
   if (isCorn)
      {
      pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, doy, tMax);
      pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
      val1 = min(tMax, 30);
      val2 = min(tMin, 30);

      cGDD += (max(val1, 10.0f) + max(val2, 10.0f)) / 2.0f - 10.0f;
      }
   // determine cGDD for crops other than corn
   else
      {
      pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, doy, tMean);
      cGDD += max((tMean - baseTemp), 0.0f);
      }
   }

// Determines the Planting and Harvest Dates as zero-based day of the year for all crops for all HRUS
// The tables are overpopulated
bool EvapTrans::FillLookupTables(FlowContext* pFlowContext)
   {
   FlowModel *pFlowModel = pFlowContext->pFlowModel;

   if (m_pCropTable)
      {
      MapLayer  *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
      // iterate through hrus/hrulayers to build lookup tables startGrowingSeason and endGrowSeason by CropID
      int hruCount = pFlowContext->pFlowModel->GetHRUCount();

      // reset LUTs annually
      for (int i = 0; i < hruCount; i++)
         {
         for (int j = 0; j < m_cropCount; j++)
            {
            m_phruCropStartGrowingSeasonArray->Set(i, j, 999);
            m_phruCropEndGrowingSeasonArray->Set(i, j, -999);
            m_phruCurrCGDDArray->Set(i, j, 0.0f);
            }
         }

      // determine planting date for every possible crop in every HRU
#pragma omp parallel for 
      for (int hruIndex = 0; hruIndex < hruCount; hruIndex++)
         {
         HRU *pHRU = pFlowModel->GetHRU(hruIndex);
         float t30 = -273.0f;
         float p7 = 0.0f;
         float precipThreshold = 1000.0f;
         vector< float > cGDDArray(m_cropCount);

         //CString label;
         // label.Format(_T("%d"), hruIndex);
         // m_phruCropStartGrowingSeasonArray->SetLabel(hruIndex, label);

         for (int doy = 0; doy < 365; doy++)
            {
            CalculateT30(pFlowContext, pHRU, doy, t30);
            CalculateP7(pFlowContext, pHRU, doy, p7);

            for (int row = 0; row < m_cropCount; row++)
               {
               int plantingMethod = m_pCropTable->GetAsInt(this->m_colPlantingMethod, row);
               float plantingThreshold = m_pCropTable->GetAsFloat(this->m_colPlantingThreshold, row);
               int t30Baseline = m_pCropTable->GetAsInt(this->m_colT30Baseline, row);
               float baseTemp = m_pCropTable->GetAsFloat(this->m_colGddBaseTemp, row);
               int gddEquationType = m_pCropTable->GetAsInt(this->m_colGddEquationType, row);
               int earliestPlantingDate = m_pCropTable->GetAsInt(this->m_colEarliestPlantingDate, row);

               if (m_colPrecipThreshold >= 0)
                  precipThreshold = m_pCropTable->GetAsFloat(this->m_colPrecipThreshold, row);

               bool isCorn = (gddEquationType == 1) ? false : true;

               if (m_phruCropStartGrowingSeasonArray->Get(hruIndex, row) == 999)
                  {
                  if (plantingMethod == PM_CGDDTHRESHOLD)
                     {
                     CalculateCGDD(pFlowContext, pHRU, baseTemp, doy, isCorn, cGDDArray.at(row));

                     if (cGDDArray.at(row) >= plantingThreshold && doy >= earliestPlantingDate && p7 <= precipThreshold)
                        {
                        // put planting date into lookup table
                        m_phruCropStartGrowingSeasonArray->Set(hruIndex, row, doy);
                        }
                     }
                  else if (plantingMethod == PM_30DAYTHRESHOLD)
                     {
                     if (t30 >= plantingThreshold  && doy >= earliestPlantingDate && p7 <= precipThreshold)
                        {
                        // put planting date into lookup table
                        m_phruCropStartGrowingSeasonArray->Set(hruIndex, row, doy - t30Baseline);
                        }
                     }
                  else if ( plantingMethod == PM_CONTINUOUS )
                     m_phruCropStartGrowingSeasonArray->Set(hruIndex, row, 0 );

                  } // end planting date not set (== 999)
               } // end row
            } // end doy
         } // end hru


      //determine harvest dates for every possible crop in every HRU
#pragma omp parallel for 
      for (int hruIndex = 0; hruIndex < hruCount; hruIndex++)
         {
         HRU *pHRU = pFlowContext->pFlowModel->GetHRU(hruIndex);
         int count = (int)pHRU->m_polyIndexArray.GetSize();
         vector< float > cGDDArray(m_cropCount);

         /*CString label;
         label.Format(_T("%d"), hruIndex);
         m_phruCropEndGrowingSeasonArray->SetLabel(hruIndex, label);*/

         for (int row = 0; row < m_cropCount; row++)
            {
            float harvestCGDDThreshold = 10000.0F;
            float killingFrostThreshold = -273.0F;
            float baseTemp = m_pCropTable->GetAsFloat(this->m_colGddBaseTemp, row);
            int termMethod = m_pCropTable->GetAsInt(this->m_colTermMethod, row);
            int gddEquationType = m_pCropTable->GetAsInt(this->m_colGddEquationType, row);
            float tMin = 0.0F;

            bool isCorn = (gddEquationType == 1) ? false : true;

            if (m_phruCropStartGrowingSeasonArray->Get(hruIndex, row) != 999)
               {
               int plantingDoy = m_phruCropStartGrowingSeasonArray->Get(hruIndex, row);

               for (int doy = plantingDoy; doy < 365; doy++)
                  {
                  if (m_phruCropEndGrowingSeasonArray->Get(hruIndex, row) == -999)
                     {
                     // crop is planted start calculating its cGDD 
                     CalculateCGDD(pFlowContext, pHRU, baseTemp, doy, isCorn, cGDDArray.at(row));

                     // determine harvest date
                     switch (termMethod)
                        {
                        case HM_CGDDTHRESHOLD:
                        {
                        harvestCGDDThreshold = m_pCropTable->GetAsFloat(this->m_colTermCGDD, row);
                        if (cGDDArray.at(row) >= harvestCGDDThreshold)
                           {
                           m_phruCropEndGrowingSeasonArray->Set(hruIndex, row, doy);
                           }
                        }
                        break;

                        case HM_KILLINGFROST:
                        {
                        killingFrostThreshold = m_pCropTable->GetAsFloat(this->m_colTermTemp, row);
                        pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);

                        int minGrowingSeason = m_pCropTable->GetAsInt(this->m_colMinGrowSeason, row);
                        if (doy - plantingDoy > minGrowingSeason)
                           {
                           if (tMin <= killingFrostThreshold)
                              {
                              m_phruCropEndGrowingSeasonArray->Set(hruIndex, row, doy);
                              }
                           }
                        }
                        break;

                        case HM_BOTH:
                        {
                        harvestCGDDThreshold = m_pCropTable->GetAsFloat(this->m_colTermCGDD, row);
                        killingFrostThreshold = m_pCropTable->GetAsFloat(this->m_colTermTemp, row);
                        pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);

                        int minGrowingSeason = m_pCropTable->GetAsInt(this->m_colMinGrowSeason, row);

                        if (doy - plantingDoy > minGrowingSeason)
                           {
                           if (cGDDArray.at(row) >= harvestCGDDThreshold || tMin <= killingFrostThreshold)
                              {
                              m_phruCropEndGrowingSeasonArray->Set(hruIndex, row, doy);
                              }
                           }
                        }
                        break;

                        case HM_JULY15MIRROR:
                        {
                        // special case harvestDate = July 15 + (July 15 - plantingDate)
                        m_phruCropEndGrowingSeasonArray->Set(hruIndex, row, 196 + (196 - m_phruCropStartGrowingSeasonArray->Get(hruIndex, row)));
                        }
                        break;

                        default:
                           m_phruCropEndGrowingSeasonArray->Set(hruIndex, row, 364);
                           break;
                        }
                     }  //end harvest date not set ( == -999 )
                  }  // end doy
               if (m_phruCropEndGrowingSeasonArray->Get(hruIndex, row) == -999)  m_phruCropEndGrowingSeasonArray->Set(hruIndex, row, 364);
               }  // end crop is planted ( != 999 )
            } // end row 
         }  // end hru

       //    m_phruCropStartGrowingSeasonArray->WriteAscii(_T("C:\\temp\\startprecip.csv"));
      //     m_phruCropEndGrowingSeasonArray->WriteAscii(_T("C:\\temp\\end.csv"));	

      if (m_method == GM_KIMBERLY_PENNMAN)
         {
         for (int hruIndex = 0; hruIndex < hruCount; hruIndex++)
            {
            HRU *pHRU = pFlowContext->pFlowModel->GetHRU(hruIndex);
            int count = (int)pHRU->m_polyIndexArray.GetSize();
            vector< float > cGDDArray(m_cropCount);

            for (int row = 0; row < m_cropCount; row++)
               {
               float baseTemp = m_pCropTable->GetAsFloat(this->m_colGddBaseTemp, row);
               int gddEquationType = m_pCropTable->GetAsInt(this->m_colGddEquationType, row);
               int useGDD = m_pCropTable->GetAsInt(this->m_colUseGDD, row);

               int plantingDoy = m_phruCropStartGrowingSeasonArray->Get(hruIndex, row);
               int harvestDoy = m_phruCropEndGrowingSeasonArray->Get(hruIndex, row);

               bool isGDD = (useGDD == 1) ? true : false;
               bool isCorn = (gddEquationType == 1) ? false : true;

               if (isGDD)
                  {
                  for (int doy = plantingDoy; doy < harvestDoy; doy++)
                     {
                     CalculateCGDD(pFlowContext, pHRU, baseTemp, doy, isCorn, cGDDArray.at(row));
                     }

                  for (int i = 0; i < count; i++)
                     {
                     int idu = pHRU->m_polyIndexArray[i];
                     CString lulc = m_pCropTable->GetAsString(m_colCropTableLulc, row);
                     CString label = _T("CGGD_") + lulc;
                     PCTSTR CGGDColName = (PCTSTR)label;
                     int colCGDD = -1;
                     pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, colCGDD, CGGDColName, TYPE_INT, CC_AUTOADD);
                     pFlowModel->UpdateIDU(pFlowContext->pEnvContext, idu, colCGDD, cGDDArray.at(row), ADD_DELTA); // degree C days  
                     } // end idus in hru
                  } // end useGDD
               } // end row
            } // end hru
         } // end KIMBERLY_PENMAN



      }  // end m_pCropTable

   return true;

   }


//void EvapTrans::CalculateT30(int hruIndex, float &t30)
//{ 
//   int index;
//   index = m_hruT30OldestIndexArray[hruIndex];
//   m_hruT30Array.Set(hruIndex, index, t30);
//   m_hruT30OldestIndexArray[hruIndex]++;
//
//   if(m_hruT30OldestIndexArray[hruIndex] > 29)
//   {
//      m_hruT30OldestIndexArray[hruIndex] = 0;
//      m_hruIsT30FilledArray[hruIndex] = true;
//   }
//
//   if(m_hruIsT30FilledArray[hruIndex])
//    {
//     t30 = 0;
//     for(int i = 0; i < 30; i++)
//   {
//     t30 += m_hruT30Array.Get(hruIndex, i);
//   }
//     t30 = t30/30;
//   }
//   else
//   {
//     t30 = -273.0F;
//   }
//}

// Calculate the mean air temperature of the last 30 days
bool EvapTrans::CalculateT30(FlowContext *pFlowContext, HRU *pHRU, int doy, float &t30)
   {
   float tMean = 0.0f;
   if (doy >= 29)
      {
      t30 = 0.0f;
      for (int i = 0; i < 30; i++)
         {
         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, doy - i, tMean);
         t30 += tMean;
         }
      t30 /= 30.0F;
      return true;
      }
   else
      return false;
   }

// Calculate the mean precipitation of the last 7 days
bool EvapTrans::CalculateP7(FlowContext *pFlowContext, HRU *pHRU, int doy, float &p7)
   {
   float precip = 0.0f;
   if (doy >= 6)
      {
      p7 = 0.0f;
      for (int i = 0; i < 7; i++)
         {
         pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, doy - i, precip);
         p7 += precip;
         }
      p7 /= 7.0F;
      return true;
      }
   else
      return false;
   }

void EvapTrans::LookupLandCoverCoefficient(int row, float cGDD, float denominator, float &landCover_coefficient)
   {
   float percentile = 10.0F*(cGDD / denominator);
   landCover_coefficient = m_pLandCoverCoefficientLUT->IGet(percentile, ++row, IM_LINEAR);
   }

void EvapTrans::CalculateReferenceET(FlowContext *pFlowContext, HRU *pHRU, unsigned short etMethod, int doy, float &referenceET)
   {
   // placeholder
   double PH_twilightRadiation = 0.0f;    // http://solardat.uoregon.edu/SolarData.html

   switch (etMethod)
      {
      case ETEquation::PENN_MONT:
      {

      // Get climate data from Flow
      float precip = 0.0f;
      float tMean = 0.0f;
      float tMin = 0.0f;
      float tMax = 0.0f;

      float specHumid = 0.0f;
      double relHumid = 0.0;
      float windSpeed = 0.0f;
      float elevation = 0.0f;
      float vpd = 0.0f;
      float solarRad = 0.0f;

      pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, doy, precip);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, doy, tMean);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, doy, tMax);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, doy, solarRad);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SPHUMIDITY, pHRU, doy, specHumid);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_WINDSPEED, pHRU, doy, windSpeed);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_VPD, pHRU, doy, vpd);

      elevation = pHRU->m_elevation;

      m_ETEq.SetDailyMinTemperature(tMin);                             //from IDU/HRU
      m_ETEq.SetVpd(vpd);
      m_ETEq.SetDailyMaxTemperature(tMax);                           //from IDU/HRU
      m_ETEq.SetDailyMeanTemperature(tMean);                                         //from IDU/HRU //from IDU/HRU
      m_ETEq.SetSpecificHumidity(specHumid); //PH_humidity);             //from IDU/HRU or context
      m_ETEq.SetWindSpeed(windSpeed);                                    //from IDU/HRU or context
      m_ETEq.SetSolarRadiation(solarRad);                                //from lookup table; may replace with reference longwave/shortwave radiation ref values
      m_ETEq.SetTwilightSolarRadiation(PH_twilightRadiation);            //from lookup table; may replace with reference longwave/shortwave radiation ref values
      m_ETEq.SetStationElevation(elevation);                             // can be culled if radiation info can be pulled from elsewhere
      m_ETEq.SetStationLatitude(m_latitude);                             // ??? ; can be culled if long wave radiation info can be pulled from elsewhere 
      m_ETEq.SetStationLongitude(m_longitude);                           // ??? ; can be culled if long wave radiation info can be pulled from elsewhere
      m_ETEq.SetTimeZoneLongitude(m_longitude);                          // ??? ; can be culled if long wave radiation info can be pulled from elsewhere
      m_ETEq.SetDoy(pFlowContext->dayOfYear);
      }
      break;

      case ETEquation::BAIER_ROBERTSON:
      {

      // Get climate data from Flow
      float precip = 0.0f;
      float tMean = 0.0f;
      float tMin = 0.0f;
      float tMax = 0.0f;

      pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, doy, precip);

      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, doy, tMax);
      tMean = (tMin + tMax) / 2.0f;

      m_ETEq.SetDailyMinTemperature(tMin);                             //from IDU/HRU
      m_ETEq.SetDailyMaxTemperature(tMax);                           //from IDU/HRU
      m_ETEq.SetDailyMeanTemperature(tMean);                                         //from IDU/HRU //from IDU/HRU

      m_ETEq.SetStationLatitude(m_latitude);                             // ??? ; can be culled if long wave radiation info can be pulled from elsewhere 
      m_ETEq.SetStationLongitude(m_longitude);                           // ??? ; can be culled if long wave radiation info can be pulled from elsewhere
      m_ETEq.SetTimeZoneLongitude(m_longitude);                          // ??? ; can be culled if long wave radiation info can be pulled from elsewhere
      m_ETEq.SetDoy(doy);
      }
      break;

      case ETEquation::ASCE:
      case ETEquation::FAO56:
      {

      // Get climate data from Flow
     /* float precip = 0.0f;*/
      float tMean = 0.0f;
      float tMin = 0.0f;
      float tMax = 0.0f;

      float specHumid = 0.0f;
      float windSpeed = 0.0f;
      float elevation = 0.0f;
      float solarRad = 0.0f;

      /*      pFlowContext->pFlowModel->GetHRUClimate( CDT_PRECIP, pHRU, doy, precip );*/
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, doy, tMean);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, doy, tMax);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, doy, solarRad);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SPHUMIDITY, pHRU, doy, specHumid);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_WINDSPEED, pHRU, doy, windSpeed);

      elevation = pHRU->m_elevation;

      m_ETEq.SetDailyMinTemperature(tMin);                                  //
      m_ETEq.SetDailyMaxTemperature(tMax);                                  //from Climate data
      m_ETEq.SetDailyMeanTemperature(tMean);                                //from Climate data 
      m_ETEq.SetSpecificHumidity(specHumid); //PH_humidity);                //from Climate data
      m_ETEq.SetWindSpeed(windSpeed);                                       //from Climate data
      m_ETEq.SetSolarRadiation(solarRad);                                   //from Climate data

  //    m_ETEq.SetTwilightSolarRadiation( PH_twilightRadiation );               // from lookup table; may replace with reference longwave/shortwave radiation ref values

      m_ETEq.SetStationElevation(elevation);                                // from HRU
      m_ETEq.SetStationLatitude(m_latitude);                                // from input tag or default
      m_ETEq.SetStationLongitude(m_longitude);                              // from input tag or default

      m_ETEq.SetTimeZoneLongitude(m_longitude);                             // ??? ; can be culled if long wave radiation info can be pulled from elsewhere

      m_ETEq.SetDoy(pFlowContext->dayOfYear);
      }
      break;

      case ETEquation::HARGREAVES:
      case ETEquation::HARGREAVES_1985:
         {
         float tMean = 0.0f, tMin=0.0f, tMax = 0.0f, solarRad = 0.0f;
         //pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, doy, tMean);
         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, doy, tMax);
        // pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, doy, solarRad);
         tMean = (tMax + tMin) / 2;
         m_ETEq.SetDailyMinTemperature(tMin);                                  //
         m_ETEq.SetDailyMaxTemperature(tMax);                                  //from Climate data
         m_ETEq.SetDailyMeanTemperature(tMean);                                //from Climate data 
       //  m_ETEq.SetSolarRadiation(solarRad);                                   //from Climate data

         m_ETEq.SetStationLatitude(m_latitude);                             // ??? ; can be culled if long wave radiation info can be pulled from elsewhere 
         m_ETEq.SetStationLongitude(m_longitude);                           // ??? ; can be culled if long wave radiation info can be pulled from elsewhere
         m_ETEq.SetTimeZoneLongitude(m_longitude);                          // ??? ; can be culled if long wave radiation info can be pulled from elsewhere
         m_ETEq.SetDoy(doy);

         int year = 0, month = -1, day = -1;
         ::GetCalDate(doy, &year, &month, &day, FALSE);
         m_currMonth = month;
         m_ETEq.SetMonth(m_currMonth);
         //Express all parameters in mm, get table for lc, fc, and wp definition
         }
      break;


      case ETEquation::KIMB_PENN:
      {
      float tMin = 0.0f;
      float tMax = 0.0f;
      float tMean = 0.0f;
      float solarRad = 0.0f;
      float windSpeed = 0.0f;
      float elevation = 0.0f;
      float specHumid = 0.0f;
      double relHumid = 0.0;

      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, doy, tMin);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, doy, tMax);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, doy, tMean);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, doy, solarRad);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_WINDSPEED, pHRU, doy, windSpeed);
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SPHUMIDITY, pHRU, doy, specHumid);

      int iduStationID = -1;
      int row = -1;
      double coefficient = 0.0;
      vector< double > agrimetStationCoeffs(m_stationCount);

      if (pFlowContext->pFlowModel->GetHRUData(pHRU, m_colAgrimetStationID, iduStationID, DAM_FIRST))  // get HRU info
         {
         // have Station ID for this HRU, find the corresponding row in the station table
         row = m_pStationTable->Find(m_colStationTableID, VData(iduStationID), 0);
         }

      if (row >= 0 && iduStationID >= 0)
         {
         coefficient = m_pStationTable->GetAsFloat(this->m_colStationCoeffOne, row);
         agrimetStationCoeffs.push_back(coefficient);
         coefficient = m_pStationTable->GetAsFloat(this->m_colStationCoeffTwo, row);
         agrimetStationCoeffs.push_back(coefficient);
         coefficient = m_pStationTable->GetAsFloat(this->m_colStationCoeffThree, row);
         agrimetStationCoeffs.push_back(coefficient);
         coefficient = m_pStationTable->GetAsFloat(this->m_colStationCoeffFour, row);
         agrimetStationCoeffs.push_back(coefficient);
         coefficient = m_pStationTable->GetAsFloat(this->m_colStationCoeffFive, row);
         agrimetStationCoeffs.push_back(coefficient);

         m_ETEq.SetAgrimentStationCoeff(agrimetStationCoeffs);
         }

      double ea = 0.0;
      elevation = pHRU->m_elevation;
      if (specHumid >= 0)
         relHumid = m_ETEq.CalcRelHumidityKP(specHumid, tMin, tMax, elevation, ea);
      else
         relHumid = -9999.0;

      m_ETEq.SetDoy(doy);
      m_ETEq.SetSolarRadiation(solarRad);
      m_ETEq.SetStationElevation(elevation);
      m_ETEq.SetDailyMeanTemperature(tMean);                                    //from IDU/HRU  
      m_ETEq.SetDailyMaxTemperature(tMax);
      m_ETEq.SetDailyMinTemperature(tMin);
      m_ETEq.SetSpecificHumidity(specHumid);
      m_ETEq.SetRelativeHumidity(relHumid);
      m_ETEq.SetWindSpeed(windSpeed);

      //calculate mean temperature for previous 3 days.
      float t3 = 0.0f;
      float tTemp = 0.0f;
      if (doy >= 3)
         {
         for (int i = 1; i <= 3; i++)
            {
            pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, doy - i, tTemp);
            t3 += tTemp;
            }
         //average temps
         t3 /= 3.0;
         }
      m_ETEq.SetRecentMeanTemperature(t3);
      }
      break;

      default:
         //ASSERT( 0 );
         int x = 1;
      }
   }



//void EvapTrans::GetHruET( FlowContext *pFlowContext, HRU *pHRU, int hruIndex, float &maxET, float &aet)
void EvapTrans::GetHruET(FlowContext *pFlowContext, HRU *pHRU, int hruIndex)
   {
   // This method calculates Potential ET and Actual ET for a given HRU
   //
   // Basic idea: 
   //   1) Using a specified ET method (FAO56, HARGRAVES, PENMAN_MONTIETH ), calculate reference (potential)
   //      ET value for s.
   //   2) Once the maxET is determined, estimate actual ET based on crop coefficients.
   //
   // This method relies on two tables:
   //   1) A soils table _T("SOILS") which provided information on the wilting point and lower wate extraction limit for
   //      the soil type present in the HRU.  This table should be named "Soil.csv" and be present
   //      on the Envision path.  Further, it must contain columns "SoilType", "WP" and "fieldCapacity" with the information
   //      described above
   //   2) a Crop Coefficient table that contains crop coefficients for the HRU land use types for which
   //      actual ET calculations are made.
   //
   //-----------------------------------------------------------------------------------------------

   // NOTE: Alternative to elevMean would be the model's m_climateStationElev, if it is ever exposed to the model.

   FlowModel *pFlowModel = pFlowContext->pFlowModel;
   MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

   //  HRU *pHRU = m_pCurrHru;
   int doy = pFlowContext->dayOfYear;     // zero-based

   float referenceET = 0.0F;

   unsigned short etMethod = m_ETEq.GetMode();

   CalculateReferenceET(pFlowContext, pHRU, etMethod, doy, referenceET);

   // get reference ET from ET method
   referenceET = m_ETEq.Run(etMethod, pHRU);    // reference mm/hr is returned by method

   if (referenceET < 0.0f) { referenceET = 0.0f; }

   /*if (m_pCropTable)
   {
   if (pHRU->m_id == 6524 || pHRU->m_id == 314 || pHRU->m_id == 241 )
   {
   m_currReferenceET = referenceET;
   }
   }*/


   float hruAET = 0.0f;
   float hruMaxET = 0.0f;
   float hruArea = pHRU->m_area;
   float iduArea = 0.0f;
   float landCover_coefficient = 0.0f;
   float depletionFraction = 1.0f;  // depletionFraction is 1.0 for forest and a function of crop and referenceET for agriculture
   float maxET = 0.0f;
   float aet = 0.0f;

   float totalFlux = 0.0f;
   float totalRequest = 0.0f;

   // initialize growing season to be outside the year
   int plantingDoy = 999;
   int harvestDoy = 364;

   int count = (int)pHRU->m_polyIndexArray.GetSize();
   for (int i = 0; i < count; i++)
      {
      int idu = pHRU->m_polyIndexArray[i];

      // if a query is defined, does this IDU pass the query?
      if (DoesIDUPassQuery(idu))
         {
         float lp = 0.0f;  // soil moisture value above which ETact reaches ETpot (mm)
         float pFraction = 0.0f;  // crop specific depletion fraction in table
         float fc = 0.0f;  // field capacity - maximum soil moisture storage (mm)
         float wp = 0.0f;  // wilting point?

         maxET = 0.0f;
         aet = 0.0f;

         int soilRow = -1;
         if (m_pSoilTable)
            {
            // get soils id from HRU
            int soilID = -1;
            if (pIDULayer->GetData(idu, m_colIduSoilID, soilID))
               {
               // have soil ID for this HRU, find the corresponding row in the soils table
               int row = m_pSoilTable->Find(m_colSoilTableSoilID, VData(soilID), 0);
               soilRow = row;

               if (row >= 0)  // found?
                  {
                  m_pSoilTable->Get(m_colLP, row, lp);
                  m_pSoilTable->Get(m_colFC, row, fc);
                  m_pSoilTable->Get(m_colWP, row, wp);
                  }
               }
            }

         // if no soil data found, assume no demand
         if (soilRow < 0)
            {
            aet = maxET = 0;
            return;
            }

         // determine landCover coefficient
         pIDULayer->GetData(idu, m_colArea, iduArea);
         if (iduArea > 0.0f && hruArea > 0.0f)
            {
            if (m_pCropTable)
               {
               // get lulc id from HRU
               int lulc = -1;
               int row = -1;
               landCover_coefficient = 0.0f;

               if (pIDULayer->GetData(idu, m_colLulc, lulc))
                  {
                  // have lulc for this HRU, find the corresponding row in the cc table
                  row = m_pCropTable->Find(m_colCropTableLulc, VData(lulc), 0);

                  if (row >= 0)
                     {
                     // the landCover specific values needed for looking up the cGDD percentile
                     // and interpolating the landCover_coefficient               

                     float denominator = -1;
                     float binToEFC = m_pCropTable->GetAsFloat(this->m_colBinToEFC, row);
                     float cGDDtoEFC = m_pCropTable->GetAsFloat(this->m_colCGDDtoEFC, row);
                     float binEFCtoTerm = m_pCropTable->GetAsFloat(this->m_colBinEFCtoTerm, row);
                     int binMethod = m_pCropTable->GetAsInt(this->m_colBinMethod, row);

                     plantingDoy = 999;
                     harvestDoy = 364;
                     plantingDoy = m_phruCropStartGrowingSeasonArray->Get(hruIndex, row);
                     harvestDoy = m_phruCropEndGrowingSeasonArray->Get(hruIndex, row);
                     int useGDD = m_pCropTable->GetAsInt(this->m_colUseGDD, row);


                     bool isGDD = (useGDD == 1) ? true : false;
                     bool isFraction = false;

                     if (doy >= plantingDoy && doy <= harvestDoy)
                        {
                        //crop specific depletion fraction 
                        pFraction = m_pCropTable->GetAsFloat(this->m_colDepletionFraction, row);

                        // using Growing Degree Days (heat unit)
                        float cGDD = m_phruCurrCGDDArray->Get(hruIndex, row);
                        if (m_calcFracCGDD)
                           {
                           float potentialHeatUnits = m_pCropTable->GetAsFloat(this->m_colTermCGDD, row);
                           int colCGDD = -1;
                           float cGDDFraction = 0.0f;
                           if (potentialHeatUnits != 0.0f)
                              cGDDFraction = cGDD / potentialHeatUnits;

                           pFlowModel->CheckCol(pFlowContext->pEnvContext->pMapLayer, colCGDD, _T("FracCGDD_D"), TYPE_FLOAT, CC_AUTOADD);
                           pIDULayer->SetData(idu, colCGDD, cGDDFraction); // degree C days
                           }

                        if (cGDD < cGDDtoEFC  && isGDD)
                           {
                           denominator = binToEFC;
                           LookupLandCoverCoefficient(row, cGDD, denominator, landCover_coefficient);
                           }
                        // using Growing Days (number of days)
                        else if (!isGDD)
                           {
                           int growingDays = doy - m_phruCropStartGrowingSeasonArray->Get(hruIndex, row);
                           denominator = (float)m_phruCropEndGrowingSeasonArray->Get(hruIndex, row) - m_phruCropStartGrowingSeasonArray->Get(hruIndex, row);
                           LookupLandCoverCoefficient(row, (float)growingDays, denominator, landCover_coefficient);
                           }
                        // using GDD after EFC, first lookup method
                        else if (binMethod == 1)
                           {
                           denominator = binEFCtoTerm;
                           LookupLandCoverCoefficient(row, cGDD, denominator, landCover_coefficient);
                           }
                        // using GDD after EFC, second lookup method
                        else if (binMethod == 2)
                           {
                           landCover_coefficient = m_pCropTable->GetAsFloat(this->m_colConstCropCoeffEFCtoT, row);
                           }
                        }
                     else
                        {
                        // outside of Growing Season landCover_coefficient is 0% percentile value
                        denominator = 1;
                        //        LookupLandCoverCoefficient( row, 0.0f, denominator, landCover_coefficient );
                        landCover_coefficient = 0.10f; // bare soil ET
                        }
                     } // crop is found
                  }  // get lulc
               } // end m_pCropTable
            } // end if has area

         // get potential ET 
         switch (GetMethod())
            {
            case GM_FAO56:
            case GM_KIMBERLY_PENNMAN:
               maxET = referenceET * landCover_coefficient;
               ASSERT(maxET >= 0.0f && maxET <= 1.0E10f);
               /*if (pFraction < 0)
                  depletionFraction = 1.0f;
                  else
                  depletionFraction = pFraction + 0.04f * (5.0f - maxET);    */
               break;
            case GM_HARGREAVES:
            case GM_HARGREAVES_1985:
                maxET = referenceET;
                break;
            case GM_BAIER_ROBERTSON:
               maxET = referenceET * landCover_coefficient;;
               break;

            case GM_PENMAN_MONTIETH:
               if (m_iduIrrRequestArray[idu] >= 0.0f)
                  maxET = m_iduIrrRequestArray[idu];
               m_iduIrrRequestArray[idu] = 0.0f;
               //				depletionFraction = 1.0f;
               break;
            }

         // calculate actual ET based on soil condition. This is a fractional value based on the distributions
         float totalVolWater = 0.0f;    // m3
         float soilwater = 0.0f;       // mm 
         bool isRatio = false;

         // choose the layer distribution that matches
         int poolIndex = m_pLayerDists->GetPoolIndex(idu);
         if (poolIndex != -1)
            {
            isRatio = m_pLayerDists->m_poolArray[poolIndex]->isRatio;
            int layerDistArraySize = (int)m_pLayerDists->m_poolArray[poolIndex]->poolDistArray.GetSize();

            // determine total available water
            for (int j = 0; j < layerDistArraySize; j++)
               {
               PoolDist *pLD = m_pLayerDists->m_poolArray[poolIndex]->poolDistArray[j];
               HRUPool *pHRUPool = pHRU->GetPool(pLD->poolIndex);
               totalVolWater += float(pHRUPool->m_volumeWater);   // m3  Results in the volume of water
               soilwater += pHRUPool->m_wDepth * pLD->fraction;       // mm.  Results in the length of water   
               }

            if (isRatio)
               {
               // determine fraction in each layer to remove later
               // int layerDistArraySize = (int)m_pLayerDists->m_poolArray[poolIndex]->poolDistArray.GetSize();
               for (int j = 0; j < layerDistArraySize; j++)
                  {
                  PoolDist *pLD = m_pLayerDists->m_poolArray[poolIndex]->poolDistArray[j];
                  HRUPool *pHRUPool = pHRU->GetPool(pLD->poolIndex);
                  if (totalVolWater > 0.0f)
                     pLD->fraction = float(pHRUPool->m_volumeWater) / totalVolWater;
                  }

               float areaToUse = pHRU->m_area - pHRU->m_areaIrrigated;    // m2
               if (areaToUse > 0.0f) soilwater = totalVolWater / areaToUse * MM_PER_M;   // mm
               }
            } // end layer choice

            // calculate actual ET based on soil condition     
            // currently depletionFraction = 1.0 for all idus
         float threshold = lp; // *depletionFraction;
         threshold = 0.5f * (fc - wp);

         // wet conditions
         // ks = 1
         if (soilwater > threshold)
            aet = maxET;
         // dry conditions
         // ks = 0
         else if (soilwater <= wp)
            aet = 0.0f;
         // intermediate conditions
         // ks is a linear function of soilwater 
         else
            {
            float slope = 1.0F / (threshold - wp);
            float fTheta = slope * (soilwater - wp);
            aet = maxET * fTheta;
            }

         // reset iduCropDemandArray
         m_iduIrrRequestArray[idu] = 0.0f;
         float irrRequest = 0.0f;

         /*int irrigate = 0;
         pIDULayer->GetData(idu, m_colIrrigate, irrigate);
         bool isIrrigated = (irrigate == 1) ? true : false;*/

         // fill annual actual and maximum ET arrays
         m_iduAnnAvgETArray[idu] += aet;   // mm/day
         m_iduAnnAvgMaxETArray[idu] += maxET;   // mm/day

         // fill seasonal actual and maximum ET arrays
         if (doy >= plantingDoy && doy <= harvestDoy)
            {
            m_iduSeasonalAvgETArray[idu] += aet;   // mm/day
            m_iduSeasonalAvgMaxETArray[idu] += maxET;   // mm/day

            if (soilwater < (fc - wp))
               {
               //      m_iduIrrRequestArray[idu] = (threshold - soilwater) * (1 + m_irrigLossFactor); //mm/d, assuming xx% over watering given inefficiency
               m_iduIrrRequestArray[idu] = maxET;
               irrRequest = m_iduIrrRequestArray[idu] * iduArea * M_PER_MM * ACREFT_PER_M3;   // acre-ft per d
               }
            }

         // output daily values
         pIDULayer->m_readOnly = false;

         pIDULayer->SetData(idu, m_colDailyET, aet);  // mm/day
         pIDULayer->SetData(idu, m_colDailyMaxET, maxET);  // mm/day
         pIDULayer->SetData(idu, m_colDailySoilMoisture, soilwater);  // mm/day
         pIDULayer->m_readOnly = true;

         // store in shared data
         float val = 1.0;
         if (iduArea / hruArea < 1.0)
            val = iduArea / hruArea;

         hruAET += aet * val;
         hruMaxET += maxET * val;

         int time = pFlowContext->dayOfYear;
         // soil layer distribution
         if (aet > 0.0)
            {
            // found layer that matches idu
            if (poolIndex != -1)
               {
               // determine distributions
               int layerDistArraySize = (int)m_pLayerDists->m_poolArray[poolIndex]->poolDistArray.GetSize();
               for (int j = 0; j < layerDistArraySize; j++)
                  {
                  PoolDist *pLD = m_pLayerDists->m_poolArray[poolIndex]->poolDistArray[j];
                  HRUPool *pHRUPool = pHRU->GetPool(pLD->poolIndex);
                  float flux = aet * pLD->fraction * iduArea * M_PER_MM;
                  pHRUPool->AddFluxFromGlobalHandler(flux, FL_TOP_SINK);     // m3/d                  
                  } // end layer
               } // end pass
            } // end aet positive

         }  // end of: if (idu processed)
      }  // end of: for ( each IDU in the HRU )

   // all done

   pHRU->m_currentMaxET = hruMaxET;    // mm/day
   pHRU->m_currentET = hruAET;     // mm/day

   return;
   }

void EvapTrans::GetHruET(FlowContext *pFlowContext, HRU *pHRU, int hruIndex, bool isGrid)
   {
   FlowModel *pFlowModel = pFlowContext->pFlowModel;
   MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
   int doy = pFlowContext->dayOfYear;     // zero-based
   float referenceET = 0.0F;
   unsigned short etMethod = m_ETEq.GetMode();
   CalculateReferenceET(pFlowContext, pHRU, etMethod, doy, referenceET);

   // get reference ET from ET method
   referenceET = m_ETEq.Run(etMethod, pHRU);    // reference mm/hr is returned by method
   if (referenceET < 0.0f) { referenceET = 0.0f; }
   float hruAET = 0.0f;
   float hruMaxET = 0.0f;
   float hruArea = pHRU->m_area;
   float iduArea = 0.0f;
   float landCover_coefficient = 0.0f;
   float depletionFraction = 1.0f;  // depletionFraction is 1.0 for forest and a function of crop and referenceET for agriculture
   float maxET = 0.0f;
   float aet = 0.0f;
   float totalFlux = 0.0f;
   float totalRequest = 0.0f;

   float lp = 0.0f;  // soil moisture value above which ETact reaches ETpot (mm)
   float pFraction = 0.0f;  // crop specific depletion fraction in table
   float fc = 0.0f;  // field capacity - maximum soil moisture storage (mm)
   float wp = 0.0f;  // wilting point?

   maxET = 0.0f;
   aet = 0.0f;

   int soilRow = -1;
   if (m_pSoilTable)
      {
      // get soils id from HRU
      int soilID = -1;
      if (pIDULayer->GetData(pHRU->m_polyIndexArray[0], m_colIduSoilID, soilID))
         {
         // have soil ID for this HRU, find the corresponding row in the soils table
         int row = m_pSoilTable->Find(m_colSoilTableSoilID, VData(soilID), 0);
         soilRow = row;

         if (row >= 0)  // found?
            {
            m_pSoilTable->Get(m_colLP, row, lp);
            m_pSoilTable->Get(m_colFC, row, fc);
            m_pSoilTable->Get(m_colWP, row, wp);
            }
         }
      }
   // if no soil data found, assume no demand
   if (soilRow < 0)
      {
      aet = maxET = 0;
      return;
      }

   // get potential ET 
   switch (GetMethod())
      {
      case GM_PENMAN_MONTIETH:
         if (pHRU->m_polyIndexArray[0] >= 0.0f)
            maxET = m_iduIrrRequestArray[pHRU->m_polyIndexArray[0]];
         m_iduIrrRequestArray[pHRU->m_polyIndexArray[0]] = 0.0f;
         break;
      }
   int idu = pHRU->m_polyIndexArray[0];
   // calculate actual ET based on soil condition. This is a fractional value based on the distributions
   float totalVolWater = 0.0f;    // m3
   float soilwater = 0.0f;       // mm 
   bool isRatio = false;

   // choose the layer distribution that matches
   int poolIndex = m_pLayerDists->GetPoolIndex(idu);
   float totalDepth = 0.0f;
   if (poolIndex != -1)
      {
      isRatio = m_pLayerDists->m_poolArray[poolIndex]->isRatio;
      int layerDistArraySize = (int)m_pLayerDists->m_poolArray[poolIndex]->poolDistArray.GetSize();

      // determine total available water
      for (int j = 0; j < layerDistArraySize; j++)
         {
         PoolDist *pLD = m_pLayerDists->m_poolArray[poolIndex]->poolDistArray[j];
         HRUPool *pHRUPool = pHRU->GetPool(pLD->poolIndex);
         totalVolWater += float(pHRUPool->m_volumeWater);   // m3  Results in the volume of water
         soilwater += pHRUPool->m_wDepth * pLD->fraction;       // mm.  Results in the length of water   
         totalDepth += pHRUPool->m_depth;
         }

      if (isRatio)
         {
         // determine fraction in each layer to remove later
         for (int j = 0; j < layerDistArraySize; j++)
            {
            PoolDist *pLD = m_pLayerDists->m_poolArray[poolIndex]->poolDistArray[j];
            HRUPool *pHRUPool = pHRU->GetPool(pLD->poolIndex);
            if (totalVolWater > 0.0f)
               pLD->fraction = float(pHRUPool->m_volumeWater) / totalVolWater;
            }
         soilwater = totalVolWater / (pHRU->m_area * totalDepth) ;   // m/m
         }
      } // end layer choice

   // calculate actual ET based on soil condition     
   float threshold = lp;

   // wet conditions
   if (soilwater > threshold)
      aet = maxET;
   // dry conditions
   else if (soilwater <= wp)
      aet = 0.0f;
   // intermediate conditions 
   else
      {
      float slope = 1.0F / (threshold - wp);
      float fTheta = slope * (soilwater - wp);
      aet = maxET * fTheta;
      }

   m_iduIrrRequestArray[idu] = 0.0f;
   float irrRequest = 0.0f;

   m_iduAnnAvgETArray[idu] += aet;   // mm/day
   m_iduAnnAvgMaxETArray[idu] += maxET;   // mm/day

   int time = pFlowContext->dayOfYear;
   // soil layer distribution
   if (aet > 0.0)
      {
      // found layer that matches idu
      if (poolIndex != -1)
         {
         // determine distributions
         int layerDistArraySize = (int)m_pLayerDists->m_poolArray[poolIndex]->poolDistArray.GetSize();
         for (int j = 0; j < layerDistArraySize; j++)
            {
            PoolDist *pLD = m_pLayerDists->m_poolArray[poolIndex]->poolDistArray[j];
            HRUPool *pHRUPool = pHRU->GetPool(pLD->poolIndex);
            float flux = aet * pLD->fraction * pHRU->m_area * M_PER_MM;
            pHRUPool->AddFluxFromGlobalHandler(flux, FL_SINK);     // m3/d                  
            } // end layer
         } // end pass
      } // end aet positive
   pHRU->m_currentMaxET = maxET;    // mm/day
   pHRU->m_currentET = aet;     // mm/day
   return;
   }


EvapTrans *EvapTrans::LoadXml(TiXmlElement *pXmlEvapTrans, MapLayer *pIDULayer, LPCTSTR filename, FlowModel *pFlowModel )
   {
   if (pXmlEvapTrans == NULL)
      return NULL;

   // add "WaterDemand" directory to the Envision search paths
   CString wdDir = PathManager::GetPath(PM_PROJECT_DIR);    // envx file directory, slash-terminated
   wdDir += _T("WaterDemand");

   PathManager::AddPath(wdDir);

   // get attributes
   LPTSTR name = NULL;
   LPTSTR cropTablePath = NULL;
   LPTSTR stationTablePath = NULL;
   LPTSTR soilTablePath = NULL;
   LPTSTR method = NULL;
   LPTSTR query = NULL;
   bool use = true;
   LPTSTR lulcCol = NULL;
   LPTSTR soilCol = NULL;
   float latitude = 45;
   float longitude = 123;
   float rSt = 1000.0f;//bulk stomatal resistance across all forest cover classes
   float irrigLossFactor = 0.25f;
   bool calcFracCGDD = false;

   XML_ATTR attrs[] = {
      // attr                 type          address               isReq  checkCol
      { _T("name"),                  TYPE_STRING,   &name,            false, 0 },
      { _T("method"),                TYPE_STRING,   &method,          true,  0 },
      { _T("query"),                 TYPE_STRING,   &query,           false, 0 },
      { _T("use"),                   TYPE_BOOL,     &use,             false, 0 },
      { _T("lulc_col"),              TYPE_STRING,   &lulcCol,         true, CC_MUST_EXIST },
      { _T("soil_col"),              TYPE_STRING,   &soilCol,         false, 0 },
      { _T("crop_table"),            TYPE_STRING,   &cropTablePath,   false, 0 },
      { _T("station_table"),         TYPE_STRING,   &stationTablePath,false, 0 },
      { _T("soil_table"),            TYPE_STRING,   &soilTablePath,   false, 0 },
      { _T("irrig_loss_factor"),     TYPE_FLOAT,    &irrigLossFactor, false, 0 },
      { _T("latitude"),              TYPE_FLOAT,    &latitude,        true,  0 },
      { _T("longitude"),             TYPE_FLOAT,    &longitude,       false, 0 },
      { _T("bulkStomatalResistance"),TYPE_FLOAT,    &rSt,             false, 0 },
      { _T("outputFractionCGDD"),    TYPE_BOOL,     &calcFracCGDD,    false, 0 },
      { NULL,                    TYPE_NULL,     NULL,             false, 0 } };

   bool ok = TiXmlGetAttributes(pXmlEvapTrans, attrs, filename, pIDULayer);
   if (!ok)
      {
      CString msg;
      msg.Format(_T("Flow: Misformed element reading <evap_trans> attributes in input file %s"), cropTablePath);
      Report::ErrorMsg(msg);
      return NULL;
      }

   EvapTrans *pEvapTrans = new EvapTrans(pFlowModel, name);

   pEvapTrans->m_pLayerDists = new PoolDistributions();
   pEvapTrans->m_pLayerDists->ParsePools(pXmlEvapTrans, pIDULayer, filename);

   int poolCount = pFlowModel->GetHRUPoolCount();

   // verify that pools are valid
   for( int i=0; i < pEvapTrans->m_pLayerDists->GetPoolCount(); i++ )
      {
      Pool *pPool = pEvapTrans->m_pLayerDists->GetPool( i );

      for ( int j=0; j < (int) pPool->poolDistArray.GetSize(); j++ )
         {
         if ( pPool->poolDistArray[j]->poolIndex >= poolCount)
            {
            CString msg;
            msg.Format( "Flow: Invalid Pool index specified in <evap_trans> section: The specified pool index (%i) equals or exceeds the number of pools (%i).  Pool idexes are zero-based.  This should be corrected before running a model.",
               pPool->poolDistArray[j]->poolIndex, poolCount );
            Report::ErrorMsg( msg );
            Report::LogError( msg );
            }
         }
      }

   pEvapTrans->m_colLulc = pIDULayer->GetFieldCol(lulcCol);
   pEvapTrans->m_colIduSoilID = pIDULayer->GetFieldCol(soilCol);
   pEvapTrans->m_colArea = pIDULayer->GetFieldCol(_T("AREA"));

   pEvapTrans->m_calcFracCGDD = calcFracCGDD;

   if (method != NULL)
      {
      switch (method[0])
         {
         case _T('a'):
         case _T('A'):
            pEvapTrans->SetMethod(GM_ASCE);
            pEvapTrans->m_ETEq.SetMode(ETEquation::ASCE);
            break;
         case _T('f'):
         case _T('F'):
         {
         if (method[1] == _T('a') || method[1] == _T('A'))
            {
            pEvapTrans->SetMethod(GM_FAO56);
            }
         else if (method[1] == _T('n') || method[1] == _T('N'))
            {
            pEvapTrans->m_extSource = method;
            FLUXSOURCE sourceLocation = ParseSource(pEvapTrans->m_extSource, pEvapTrans->m_extPath, pEvapTrans->m_extFnName,
               pEvapTrans->m_extFnDLL, pEvapTrans->m_extFn);

            if (sourceLocation != FS_FUNCTION)
               {
               CString msg;
               msg.Format(_T("Error parsing <external_method> 'fn=%s' specification for '%s'.  This method will not be invoked."), method, name);
               Report::ErrorMsg(msg);
               pEvapTrans->SetMethod(GM_NONE);
               pEvapTrans->m_use = false;
               }
            else
               pEvapTrans->SetMethod(GM_EXTERNAL);
            }
         }
         break;

         case _T('p'):
         case _T('P'):
            pEvapTrans->SetMethod(GM_PENMAN_MONTIETH);
            pEvapTrans->m_ETEq.SetMode(ETEquation::PENN_MONT);
            break;

         case _T('h'):
         case _T('H'):
            pEvapTrans->SetMethod(GM_HARGREAVES);
            pEvapTrans->m_ETEq.SetMode(ETEquation::HARGREAVES);
            break;

         case _T('s'):
         case _T('S'):
            pEvapTrans->SetMethod(GM_HARGREAVES_1985);
            pEvapTrans->m_ETEq.SetMode(ETEquation::HARGREAVES_1985);
            break;


         case _T('k'):
         case _T('K'):
            pEvapTrans->SetMethod(GM_KIMBERLY_PENNMAN);
            pEvapTrans->m_ETEq.SetMode(ETEquation::KIMB_PENN);
            break;

         case _T('b'):
         case _T('B'):
            pEvapTrans->SetMethod(GM_BAIER_ROBERTSON);
            pEvapTrans->m_ETEq.SetMode(ETEquation::BAIER_ROBERTSON);
            break;

         default:
            pEvapTrans->SetMethod(GM_NONE);
         }
      }  // end of: if ( method != NULL )

   if (pEvapTrans->m_method != GM_PENMAN_MONTIETH && pEvapTrans->m_method != GM_BAIER_ROBERTSON && pEvapTrans->m_method != GM_HARGREAVES_1985) //Assuming penman montieth is not a reference crop method (we don't need crop coefficients)
      {
      CString tmpPath;

      if (pEvapTrans->m_method == GM_KIMBERLY_PENNMAN)
         {
         if (stationTablePath == NULL)
            {
            CString msg;
            msg.Format(_T("EvapTrans: Missing station_table attribute when reading '%s'.  Table be loaded"), stationTablePath);
            Report::LogWarning(msg);
            pEvapTrans->SetMethod(GM_NONE);
            return pEvapTrans;
            }

         if (PathManager::FindPath(stationTablePath, tmpPath) < 0)
            {
            CString msg;
            msg.Format(_T("EvapTrans: Specified station table '%s' for method '%s' can not be found, this method will be ignored"), stationTablePath, name);
            Report::LogWarning(msg);
            pEvapTrans->SetMethod(GM_NONE);
            return pEvapTrans;
            }

         // read in Station Coefficients Table
         pEvapTrans->m_stationTablePath = tmpPath;
         pEvapTrans->m_pStationTable = new FDataObj(U_UNDEFINED);
         pEvapTrans->m_stationCount = pEvapTrans->m_pStationTable->ReadAscii(tmpPath, _T(','));

         if (pEvapTrans->m_stationCount <= 0)
            {
            CString msg;
            msg.Format(_T("EvapTrans::  Error loading Station file '%s'"), tmpPath);
            Report::InfoMsg(msg);
            return NULL;
            }

         // get StationTable column numbers for column headers
         pEvapTrans->m_colStationTableID = pEvapTrans->m_pStationTable->GetCol(_T("Station ID"));
         pEvapTrans->m_colStationCoeffOne = pEvapTrans->m_pStationTable->GetCol(_T("Coefficient One"));
         pEvapTrans->m_colStationCoeffTwo = pEvapTrans->m_pStationTable->GetCol(_T("Coefficient Two"));
         pEvapTrans->m_colStationCoeffThree = pEvapTrans->m_pStationTable->GetCol(_T("Coefficient Three"));
         pEvapTrans->m_colStationCoeffFour = pEvapTrans->m_pStationTable->GetCol(_T("Coefficient Four"));
         pEvapTrans->m_colStationCoeffFive = pEvapTrans->m_pStationTable->GetCol(_T("Coefficient Five"));

         if (pEvapTrans->m_colStationTableID < 0 || pEvapTrans->m_colStationCoeffOne < 0 || pEvapTrans->m_colStationCoeffTwo < 0 ||
            pEvapTrans->m_colStationCoeffThree < 0 || pEvapTrans->m_colStationCoeffFour < 0 || pEvapTrans->m_colStationCoeffFive < 0)
            {
            CString msg;
            msg.Format(_T("Evaptrans: One or more column headings are incorrect in Station file '%s'"), (LPCTSTR)tmpPath);
            Report::ErrorMsg(msg);
            return NULL;
            }
         }  // end of: if ( GM_KIMBERLY_PENNMAN )

      if (cropTablePath == NULL)
         {
         CString msg;
         msg.Format(_T("EvapTrans: Missing crop_table attribute when reading '%s'.  Table be loaded"), cropTablePath);
         Report::LogWarning(msg);
         pEvapTrans->SetMethod(GM_NONE);
         return pEvapTrans;
         }


      if (PathManager::FindPath(cropTablePath, tmpPath) < 0)
         {
         CString msg;
         msg.Format(_T("EvapTrans: Specified crop table '%s' for method '%s' can not be found, this method will be ignored"), cropTablePath, name);
         Report::LogWarning(msg);
         pEvapTrans->SetMethod(GM_NONE);
         return pEvapTrans;
         }

      // read in CropTable
      pEvapTrans->m_cropTablePath = tmpPath;
      pEvapTrans->m_pCropTable = new VDataObj(U_UNDEFINED);
      pEvapTrans->m_cropCount = pEvapTrans->m_pCropTable->ReadAscii(tmpPath, _T(','));

      if (pEvapTrans->m_cropCount <= 0)
         {
         CString msg;
         msg.Format(_T("EvapTrans::LoadXML could not load Crop .csv file \n"));
         Report::InfoMsg(msg);
         return NULL;
         }

      // get CropTable column numbers for column headers
      pEvapTrans->m_pCropTable->CheckCol(lulcCol, pEvapTrans->m_colCropTableLulc, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Planting Date Method"), pEvapTrans->m_colPlantingMethod, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Planting Threshold"), pEvapTrans->m_colPlantingThreshold, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("T30 Baseline [days]"), pEvapTrans->m_colT30Baseline, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("GDD Equation"), pEvapTrans->m_colGddEquationType, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("GDD Base Temp [C]"), pEvapTrans->m_colGddBaseTemp, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Term Date Method"), pEvapTrans->m_colTermMethod, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Killing Frost Temp [C]"), pEvapTrans->m_colTermTemp, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Min Growing Season [days]"), pEvapTrans->m_colMinGrowSeason, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("CGDD P to T"), pEvapTrans->m_colTermCGDD, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Earliest Planting Date"), pEvapTrans->m_colEarliestPlantingDate, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("useGDD"), pEvapTrans->m_colUseGDD, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("GDD per 10% P to EFC"), pEvapTrans->m_colBinToEFC, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("CGDD P/GU to EFC"), pEvapTrans->m_colCGDDtoEFC, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("GDD per 10% EFC to T"), pEvapTrans->m_colBinEFCtoTerm, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Interp Method EFC to T (1-CGDD; 2-constant)"), pEvapTrans->m_colBinMethod, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Kc value EFC to T if constant"), pEvapTrans->m_colConstCropCoeffEFCtoT, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Depletion Fraction"), pEvapTrans->m_colDepletionFraction, CC_MUST_EXIST);
      pEvapTrans->m_pCropTable->CheckCol(_T("Precipitation Threshold"), pEvapTrans->m_colPrecipThreshold, 0);
      pEvapTrans->m_pCropTable->CheckCol(_T("Yield Response Factor"), pEvapTrans->m_colYieldResponseFactor, CC_AUTOADD);

      //pEvapTrans->m_colPlantingMethod = pEvapTrans->m_pCropTable->GetCol( _T("Planting Date Method") );
      //pEvapTrans->m_colPlantingThreshold = pEvapTrans->m_pCropTable->GetCol( _T("Planting Threshold") );
      //pEvapTrans->m_colT30Baseline = pEvapTrans->m_pCropTable->GetCol( _T("T30 Baseline [days]") );
      //pEvapTrans->m_colGddEquationType = pEvapTrans->m_pCropTable->GetCol( _T("GDD Equation") );
      //pEvapTrans->m_colGddBaseTemp = pEvapTrans->m_pCropTable->GetCol( _T("GDD Base Temp [C]") );
      //pEvapTrans->m_colTermMethod = pEvapTrans->m_pCropTable->GetCol( _T("Term Date Method") );
      //pEvapTrans->m_colTermTemp = pEvapTrans->m_pCropTable->GetCol( _T("Killing Frost Temp [C]") );
      //pEvapTrans->m_colMinGrowSeason = pEvapTrans->m_pCropTable->GetCol( _T("Min Growing Season [days]") );
      //pEvapTrans->m_colTermCGDD = pEvapTrans->m_pCropTable->GetCol( _T("CGDD P to T") );
      //pEvapTrans->m_colEarliestPlantingDate = pEvapTrans->m_pCropTable->GetCol( _T("Earliest Planting Date") );
      //pEvapTrans->m_colUseGDD = pEvapTrans->m_pCropTable->GetCol( _T("useGDD") );
      //pEvapTrans->m_colBinToEFC = pEvapTrans->m_pCropTable->GetCol( _T("GDD per 10% P to EFC") );
      //pEvapTrans->m_colCGDDtoEFC = pEvapTrans->m_pCropTable->GetCol( _T("CGDD P/GU to EFC") );
      //pEvapTrans->m_colBinEFCtoTerm = pEvapTrans->m_pCropTable->GetCol( _T("GDD per 10% EFC to T") );
      //pEvapTrans->m_colBinMethod = pEvapTrans->m_pCropTable->GetCol( _T("Interp Method EFC to T (1-CGDD; 2-constant)") );
      //pEvapTrans->m_colConstCropCoeffEFCtoT = pEvapTrans->m_pCropTable->GetCol( _T("Kc value EFC to T if constant") );
      //pEvapTrans->m_colDepletionFraction = pEvapTrans->m_pCropTable->GetCol( _T("Depletion Fraction") );
      //pEvapTrans->m_colPrecipThreshold = pEvapTrans->m_pCropTable->GetCol( _T("Precipitation Threshold") );  // optional
      //pEvapTrans->m_colYieldResponseFactor = pEvapTrans->m_pCropTable->GetCol( _T("Yield Response Factor") );  // optional
      //
      //if ( pEvapTrans->m_colCropTableLulc < 0 || pEvapTrans->m_colPlantingThreshold < 0 || pEvapTrans->m_colT30Baseline < 0 ||
      //   pEvapTrans->m_colGddEquationType < 0 || pEvapTrans->m_colGddBaseTemp < 0 || pEvapTrans->m_colTermMethod < 0 || pEvapTrans->m_colMinGrowSeason < 0 ||
      //   pEvapTrans->m_colTermTemp < 0 || pEvapTrans->m_colTermCGDD < 0 || pEvapTrans->m_colUseGDD < 0 || pEvapTrans->m_colBinToEFC < 0 || pEvapTrans->m_colCGDDtoEFC < 0 ||
      //   pEvapTrans->m_colBinEFCtoTerm < 0 || pEvapTrans->m_colBinMethod < 0 || pEvapTrans->m_colConstCropCoeffEFCtoT < 0 || pEvapTrans->m_colEarliestPlantingDate < 0 ||
      //   pEvapTrans->m_colDepletionFraction < 0 )
      //   {
      //   CString msg;
      //   msg.Format( _T("Evaptrans::LoadXML: One or more column headings are incorrect in Crop file '%s' .csv file"), (LPCTSTR) tmpPath );
      //   Report::ErrorMsg( msg );
      //   return NULL;
      //   }

      int per0Col = pEvapTrans->m_pCropTable->GetCol(_T("0%"));
      int per10Col = pEvapTrans->m_pCropTable->GetCol(_T("10%"));
      int per20Col = pEvapTrans->m_pCropTable->GetCol(_T("20%"));
      int per30Col = pEvapTrans->m_pCropTable->GetCol(_T("30%"));
      int per40Col = pEvapTrans->m_pCropTable->GetCol(_T("40%"));
      int per50Col = pEvapTrans->m_pCropTable->GetCol(_T("50%"));
      int per60Col = pEvapTrans->m_pCropTable->GetCol(_T("60%"));
      int per70Col = pEvapTrans->m_pCropTable->GetCol(_T("70%"));
      int per80Col = pEvapTrans->m_pCropTable->GetCol(_T("80%"));
      int per90Col = pEvapTrans->m_pCropTable->GetCol(_T("90%"));
      int per100Col = pEvapTrans->m_pCropTable->GetCol(_T("100%"));
      int per110Col = pEvapTrans->m_pCropTable->GetCol(_T("110%"));
      int per120Col = pEvapTrans->m_pCropTable->GetCol(_T("120%"));
      int per130Col = pEvapTrans->m_pCropTable->GetCol(_T("130%"));
      int per140Col = pEvapTrans->m_pCropTable->GetCol(_T("140%"));
      int per150Col = pEvapTrans->m_pCropTable->GetCol(_T("150%"));
      int per160Col = pEvapTrans->m_pCropTable->GetCol(_T("160%"));
      int per170Col = pEvapTrans->m_pCropTable->GetCol(_T("170%"));
      int per180Col = pEvapTrans->m_pCropTable->GetCol(_T("180%"));
      int per190Col = pEvapTrans->m_pCropTable->GetCol(_T("190%"));
      int per200Col = pEvapTrans->m_pCropTable->GetCol(_T("200%"));

      if (per0Col < 0 || per10Col < 0 || per20Col < 0 || per30Col < 0 || per40Col < 0 || per50Col < 0 || per60Col < 0 ||
         per70Col < 0 || per80Col < 0 || per90Col < 0 || per100Col < 0 || per110Col < 0 || per120Col < 0 || per130Col < 0 ||
         per140Col < 0 || per150Col < 0 || per160Col < 0 || per170Col < 0 || per180Col < 0 || per190Col < 0 || per200Col < 0)
         {
         CString msg;
         msg.Format(_T("Evaptrans::LoadXML: One or more percentile column headings are incorrect in Crop file '%s' file"), (LPCTSTR)tmpPath);
         Report::ErrorMsg(msg);
         return NULL;
         }

      int colCount = pEvapTrans->m_cropCount + 1;
      int rowCount = 21;
      pEvapTrans->m_pLandCoverCoefficientLUT = new FDataObj(colCount, rowCount, U_UNDEFINED);

      float percentile;
      for (int i = 0; i < rowCount; i++)
         {
         percentile = i * 10.0f;
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(0, i, percentile);
         }

      for (int i = 0; i < pEvapTrans->m_cropCount; i++)
         {
         int j = 0;
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per0Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per10Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per20Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per30Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per40Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per50Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per60Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per70Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per80Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per90Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per100Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per110Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per120Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per130Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per140Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per150Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per160Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per170Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per180Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per190Col, i));
         pEvapTrans->m_pLandCoverCoefficientLUT->Set(i + 1, j++, pEvapTrans->m_pCropTable->GetAsFloat(per200Col, i));
         }
      }  // end of: if ( pEvapTrans->m_method != GM_PENMAN_MONTIETH && pEvapTrans->m_method != GM_BAIER_ROBERTSON )

   pEvapTrans->m_irrigLossFactor = irrigLossFactor;

   CString tmpPath;
   if (soilTablePath == NULL)
      {
      CString msg;
      msg.Format(_T("EvapTrans: Missing soil_table attribute for method '%s'.  This method will be ignored"), name);
      Report::LogWarning(msg);
      pEvapTrans->SetMethod(GM_NONE);
      return pEvapTrans;
      }

   // does file exist?
   if (PathManager::FindPath(soilTablePath, tmpPath) < 0)
      {
      CString msg;
      msg.Format(_T("EvapTrans: Specified soil table '%s' for method '%s' can not be found, this method will be ignored"), soilTablePath, name);
      Report::LogWarning(msg);
      pEvapTrans->SetMethod(GM_NONE);
      return pEvapTrans;
      }

   // read in SoilTable
   pEvapTrans->m_soilTablePath = soilTablePath;
   pEvapTrans->m_pSoilTable = new FDataObj(U_UNDEFINED);
   int soilCount = pEvapTrans->m_pSoilTable->ReadAscii(tmpPath, _T(','));

   if (soilCount <= 0)
      {
      CString msg;
      msg.Format(_T("EvapTrans::LoadXML could not load Soil .csv file \n"));
      Report::InfoMsg(msg);
      return NULL;
      }

   pEvapTrans->m_colSoilTableSoilID = pEvapTrans->m_pSoilTable->GetCol(soilCol);

   pEvapTrans->m_colWP = pEvapTrans->m_pSoilTable->GetCol(_T("WP"));
   pEvapTrans->m_colFC = pEvapTrans->m_pSoilTable->GetCol(_T("FC"));
   pEvapTrans->m_colLP = pEvapTrans->m_pSoilTable->GetCol(_T("LP"));

   pEvapTrans->m_bulkStomatalResistance = rSt;
   pEvapTrans->m_latitude = latitude;
   pEvapTrans->m_longitude = longitude;

   if (query)
      pEvapTrans->m_query = query;

   return pEvapTrans;
   }