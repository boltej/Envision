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
// Flow_Solute.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Flow_Solute.h"
#include <Flow\Flow.h>
#include <PathManager.h>
#include <UNITCONV.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


Flow_Solute::Flow_Solute(void)
: m_uplandWaterSourceSinkArray()
, m_instreamWaterSourceSinkArray()
, m_soilDrainage()
, m_col_wp(-1)
, m_col_phi(-1)
, m_col_lamda(-1)
, m_col_kSat(-1)
, m_col_ple(-1)
, m_col_lp(-1)
, m_col_fc(-1)
, m_col_kSatGw(-1)
, m_col_pleGw(-1)
, m_col_phiGw(-1)
, m_col_fcPct(-1)
, m_col_k(-1)
, m_col_ic(-1)
, m_col_om(-1)
, m_col_bd(-1)
, m_col_koc(-1)
, m_col_ks(-1)
, m_colField(-1)
, m_colIPM(-1)
, m_colN_app(-1)
, m_colArea(-1)
, m_colDistStr(-1)
, m_colFracCGDD(-1)
, m_colStandAge(-1)
, m_colLULC_B(-1)
, m_colLULC_A(-1)
, m_col_AppRateConventional(-1)
, m_col_AppRateRecommended(-1)
, m_col_AppRateZero(-1)
, m_col_AppRateMinimum(-1)
, m_pRateData(NULL)
   {}

Flow_Solute::~Flow_Solute(void)
   {
   if (m_uplandWaterSourceSinkArray != NULL)
      delete[] m_uplandWaterSourceSinkArray;

   if (m_soilDrainage != NULL)
      delete[] m_soilDrainage;

   if (m_instreamWaterSourceSinkArray != NULL)
      delete[] m_instreamWaterSourceSinkArray;

   if (m_pRateData != NULL)
      delete m_pRateData;

   }

float Flow_Solute::EndRun_Solute_Fluxes(FlowContext *pFlowContext)
   {
   CString fileOut = NULL;
   CString path = PathManager::GetPath(3);
   fileOut.Format("%soutput\\rates.csv", (LPCTSTR)path);
   m_pRateData->WriteAscii(fileOut);
   CString msg;
   msg.Format(_T("Flow Solute: Rate file written to %s "), fileOut);
   Report::LogMsg(msg);
   return 1.0f;
   }
float Flow_Solute::Init_Solute_Fluxes(FlowContext *pFlowContext)
   {

   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colField, "TAXLOT", TYPE_STRING, CC_AUTOADD);
   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colIPM, "IPM", TYPE_INT, CC_AUTOADD);
   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colN_app, "N_app", TYPE_INT, CC_AUTOADD);
   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colDistStr, "NEAR_DIST", TYPE_DOUBLE, CC_AUTOADD);
   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colFracCGDD, "FracCGDD_D", TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colArea, "AREA", TYPE_FLOAT, CC_MUST_EXIST);
   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colLULC_A, "LULC_A", TYPE_INT, CC_MUST_EXIST);
   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colLULC_B, "LULC_B", TYPE_INT, CC_AUTOADD); //get crop number from LULC_B
   EnvExtension::CheckCol(pFlowContext->pFlowModel->m_pCatchmentLayer, m_colStandAge, "Stand_Age", TYPE_INT, CC_AUTOADD); //get crop number from LULC_B
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();


   //ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable("GRID");   // store this pointer (and check to make sure it's not NULL)
   //m_col_wp = pLULC_Table->GetFieldCol("wp");         // get the location of the parameter in the table    
   //m_col_phi = pLULC_Table->GetFieldCol("phi");
   //m_col_lamda = pLULC_Table->GetFieldCol("lamda");
   //m_col_kSat = pLULC_Table->GetFieldCol("kSat");
   //m_col_ple = pLULC_Table->GetFieldCol("ple");
   //m_col_lp = pLULC_Table->GetFieldCol("lp");
   //m_col_fc = pLULC_Table->GetFieldCol("fc");
   //m_col_fcPct = pLULC_Table->GetFieldCol("fcPct");
   //m_col_ic = pLULC_Table->GetFieldCol("ic");

   //ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable("GWParams");   // store this pointer (and check to make sure it's not NULL)
   //if (pGWParam_Table)
   //   {

   //   m_col_kSatGw = pGWParam_Table->GetFieldCol("ksat");         // get the location of the parameter in the table    
   //   m_col_pleGw = pGWParam_Table->GetFieldCol("ple");
   //   m_col_phiGw = pGWParam_Table->GetFieldCol("phi");
   //   m_col_k = pLULC_Table->GetFieldCol("k");
   //   }

   StateVar *pSV = pFlowContext->pFlowModel->GetStateVar(1);//this is the first extra state variable.
   /*
   if (pSV->m_tableArray[0] != NULL)
      { 
      ParamTable *pApplication_Table = pSV->m_tableArray[0];
      m_col_AppRateConventional = pApplication_Table->GetFieldCol("ConventionalRate");// get the application rate for this crop (what are the units for application rate?)
      m_col_AppRateRecommended = pApplication_Table->GetFieldCol("RecommendedRate");// get the application rate for this crop
      m_col_AppRateMinimum = pApplication_Table->GetFieldCol("MinRate");// get the application rate for this crop
      m_col_AppRateZero = pApplication_Table->GetFieldCol("NoFertilizer");// get the application rate for this crop
      m_col_AppStart = pApplication_Table->GetFieldCol("DOY");// get the start application date (it is a day of year in the table   
      }

      */
   /*ParamTable *pSoilPropertiesTable = pFlowContext->pFlowModel->GetTable("SoilProperties");
   m_col_om = pSoilPropertiesTable->GetFieldCol("OM");
   m_col_bd = pSoilPropertiesTable->GetFieldCol("BD");   */   

  /* m_col_koc = pSV->m_paramTable->GetCol("koc");
   m_col_ks = pSV->m_paramTable->GetCol("ks");*/

   int hruCount = pFlowContext->pFlowModel->GetHRUCount();
   HRU *pHRU = pFlowContext->pFlowModel->GetHRU(0);
   int hruLayerCount = pHRU->GetPoolCount();

   m_hruBiomassNArray.SetSize(hruCount);

   m_uplandWaterSourceSinkArray = new float[hruCount*hruLayerCount]; //store the ss terms for each state variable at each timestep.  
   m_soilDrainage = new float[hruCount];
   int count = 0;
   for (int i = 0; i < hruCount; i++)
      {
      m_soilDrainage[i] = 0.0f;
      m_hruBiomassNArray[i] = 0.0f;
      }
   for (int j = 0; j<pHRU->GetPoolCount(); j++)
      {
      count++;
      m_uplandWaterSourceSinkArray[count] = 0.0f;
      }

   m_pRateData = new FDataObj(30, 0);

   m_pRateData->SetName("Rates");
   m_pRateData->SetLabel(0, "Time");
   m_pRateData->SetLabel(1, "x");
   m_pRateData->SetLabel(2, "y");
   m_pRateData->SetLabel(3, "l6TSO");
   m_pRateData->SetLabel(4, "l6BSO");
   m_pRateData->SetLabel(5, "l6STREAMSINK");
   m_pRateData->SetLabel(6, "l6TSI");
   m_pRateData->SetLabel(7, "l1TSI");
   m_pRateData->SetLabel(8, "l1TSO");
   m_pRateData->SetLabel(9, "l1BSO");
   m_pRateData->SetLabel(10, "lBSI");
   m_pRateData->SetLabel(11, "l1FLSINK");
   m_pRateData->SetLabel(12, "l2TSI");
   m_pRateData->SetLabel(13, "l2TSO");
   m_pRateData->SetLabel(14, "l2BSO");
   m_pRateData->SetLabel(15, "l2BSI");
   m_pRateData->SetLabel(16, "l2FLSINK");
   m_pRateData->SetLabel(17, "l3TSI");
   m_pRateData->SetLabel(18, "l3TSO");
   m_pRateData->SetLabel(19, "l3BSO");
   m_pRateData->SetLabel(20, "l3BSI");
   m_pRateData->SetLabel(21, "l3FLSINK");
   m_pRateData->SetLabel(22, "l4TSI");
   m_pRateData->SetLabel(23, "l4TSO");
   m_pRateData->SetLabel(24, "l4BSO");
   m_pRateData->SetLabel(25, "l4BSI");
   m_pRateData->SetLabel(26, "l5TSI");
   m_pRateData->SetLabel(27, "l5TSO");
   m_pRateData->SetLabel(28, "l5BSO");
   m_pRateData->SetLabel(29, "l5BSI");
   return 0.0f;
   }

float Flow_Solute::Start_Year_Solute_Fluxes(FlowContext *pFlowContext)
   {
   return -1;

   }


float Flow_Solute::Solute_Fluxes_SRS_Old(FlowContext *pFlowContext)
{
   if (pFlowContext->timing & 1) // Init()
      return Flow_Solute::Init_Solute_Fluxes(pFlowContext);
   if (pFlowContext->timing & GMT_START_YEAR) // Init()
      return Flow_Solute::Start_Year_Solute_Fluxes(pFlowContext);
   float time = pFlowContext->time;
   time = (float)fmod(time, 365);

   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();

  // ParamTable *pSoilPropertiesTable = pFlowContext->pFlowModel->GetTable("SoilProperties");
#pragma omp parallel for
   for (int h = 0; h < hruCount; h++) //for each HRU
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      HRUPool *pLayer1 = pHRU->GetPool(0);//unsat
      HRUPool *pLayer2 = pHRU->GetPool(1);//sat
      HRUPool *pLayer3 = pHRU->GetPool(2);//unsat argillic
      HRUPool *pLayer4 = pHRU->GetPool(3);//unsat ground
      HRUPool *pLayer5 = pHRU->GetPool(4);//sat ground
      HRUPool *pLayer6 = pHRU->GetPool(5);//ponded
      int hruLayerCount = pHRU->GetPoolCount();
      int svC = pFlowContext->svCount;

      //VData soilType = 0;

      //pFlowContext->pFlowModel->GetHRUData(pHRU, pSoilPropertiesTable->m_iduCol, soilType, DAM_FIRST);  // get soil for this particular HRU
      //if (pSoilPropertiesTable->m_type == DOT_FLOAT)
      //   soilType.ChangeType(TYPE_FLOAT);
      //float om = 0.0f;
      //float bd = 0.0f;
      //bool ok = pSoilPropertiesTable->Lookup(soilType, m_col_om, om);// get the organic matter content
      //ok = pSoilPropertiesTable->Lookup(soilType, m_col_bd, bd);// get the organic matter content

      for (int k = 0; k<svC; k++)   //for each extra state variable
         {
         StateVar *pSV = pFlowContext->pFlowModel->GetStateVar(k + 1);

         VData val = pSV->m_name.GetString();
        // int row = pSV->m_paramTable->Find(0, val, 0);
        // float koc = pSV->m_paramTable->GetAsFloat(m_col_koc, row);
        // float ks = pSV->m_paramTable->GetAsFloat(m_col_ks, row);

         float cL1 = 0.0f;
         if (pLayer1->m_volumeWater>0.0f)
            cL1 = (float)pLayer1->GetExtraStateVarValue(k) / (float)pLayer1->m_volumeWater;

         float cL2 = 0.0f;
         if (pLayer2->m_volumeWater>0.0f)
            cL2 = (float)pLayer2->GetExtraStateVarValue(k) / (float)pLayer2->m_volumeWater;

         float cL3 = 0.0f;
         if (pLayer3->m_volumeWater>0.0f)
            cL3 = (float)pLayer3->GetExtraStateVarValue(k) / (float)pLayer3->m_volumeWater;

         float cL4 = 0.0f;
         if (pLayer4->m_volumeWater>0.0f)
            cL4 = (float)pLayer4->GetExtraStateVarValue(k) / (float)pLayer4->m_volumeWater;

         float cL5 = 0.0f;
         if (pLayer5->m_volumeWater>0.0f)
            cL5 = (float)pLayer5->GetExtraStateVarValue(k) / (float)pLayer5->m_volumeWater;

         float cL6 = 0.0f;
         if (pLayer6->m_volumeWater>0.0f)
            cL6 = (float)pLayer6->GetExtraStateVarValue(k) / (float)pLayer6->m_volumeWater;


         //Begin Layer 1 - Unsaturated
         float ss = 0.0f;
        // float uptakeEfficiency = 0.5f;
         float a1 = pLayer1->m_waterFluxArray[FL_TOP_SINK] * cL1 ;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         //float appRate = ApplicationRate(time, pLayer1, pFlowContext, k);
         float appRate=0.0f;
         int currYear=pFlowContext->pFlowModel->GetCurrentYearOfRun();


       /*  if (k == currYear)//add this tracer to precipitation
            {
            appRate = pLayer1->m_waterFluxArray[FL_TOP_SOURCE];//concentration in tracer is 1 kg/m3.  m3/d*1kg/m3=kg/d
            }
*/
            
        //if (time>100 && time<105 && pFlowContext->pEnvContext->yearOfRun==4)
        // if (time>0 && time<180 && pFlowContext->pEnvContext->yearOfRun > -1)

         MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
         int age=0;
         pIDULayer->GetData(pHRU->m_polyIndexArray[0], m_colStandAge, age);//get fraction of CGDD from this idu		 
        
        // if (age>60 && age <62 || age == 68 || age == 38 )
      //   if (age == 1021 || age == 68 || age == 38)
         if (age == 65)
            appRate = pLayer1->m_waterFluxArray[FL_TOP_SOURCE]*1;//(1.0f - cL1)*pLayer1->m_volumeWater / pFlowContext->timeStep;
            
            
          // appRate=10*pLayer1->m_volumeWater;
      //   float b1 = pLayer1->m_waterFluxArray[FL_TOP_SOURCE] * cL6;//down flux across upper surface of this layer (a gain) precip,
         float b1 = appRate;
         float c1 = pLayer1->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL2;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         float d1 = pLayer1->m_waterFluxArray[FL_BOTTOM_SINK] * cL1; //down flux across lower surface of this layer (a loss)
         float e111 = pLayer1->m_waterFluxArray[FL_STREAM_SINK] * cL1; //loss to streams

         float advectiveFluxL1 = a1 + b1 + c1 + d1 + e111;
         ss = advectiveFluxL1;
         pLayer1->AddExtraSvFluxFromGlobalHandler(k, ss);
         Reach * pReach = pLayer2->GetReach();
         if (pReach)
            {
            ReachSubnode *subnode = pReach->GetReachSubnode(0);
            subnode->AddExtraSvFluxFromGlobalHandler(k, -e111); //m3/d
            }
         // Finish Unsaturated

         //Begin Layer 2 - Shallow saturated

         // float uptakeEfficiency = 0.5f;
         a1 = pLayer2->m_waterFluxArray[FL_TOP_SINK] * cL2;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
        // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer2->m_waterFluxArray[FL_TOP_SOURCE] * cL1;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
        // b1 += appRate;
         c1 = pLayer2->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL3;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer2->m_waterFluxArray[FL_BOTTOM_SINK] * cL2; //down flux across lower surface of this layer (a loss)
         //float e111 = pLayer2->m_waterFluxArray[FL_STREAM_SINK] * cL2; //loss to streams
         e111 = CalcSatTracerFluxes(pHRU, pLayer2, k);

         advectiveFluxL1 = a1 + b1 + c1 + d1 + e111;
         ss = advectiveFluxL1;
         pLayer2->AddExtraSvFluxFromGlobalHandler(k, ss);


         // Finish Shallow Saturated

         //Begin Layer  3 - Argillic

         // float uptakeEfficiency = 0.5f;
         a1 = pLayer3->m_waterFluxArray[FL_TOP_SINK] * cL3;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer3->m_waterFluxArray[FL_TOP_SOURCE] * cL2;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         // b1 += appRate;
         c1 = pLayer3->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL4;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer3->m_waterFluxArray[FL_BOTTOM_SINK] * cL3; //down flux across lower surface of this layer (a loss)
         //float e111 = pLayer2->m_waterFluxArray[FL_STREAM_SINK] * cL2; //loss to streams
         //e111 = CalcSatTracerFluxes(pHRU, pLayer3, k, pFlowContext);

         advectiveFluxL1 = a1 + b1 + c1 + d1;
         ss = advectiveFluxL1;
         pLayer3->AddExtraSvFluxFromGlobalHandler(k, ss);
         // Finish Argillic


         //Begin Layer 4 - Deep unsaturated

         // float uptakeEfficiency = 0.5f;
         a1 = pLayer4->m_waterFluxArray[FL_TOP_SINK] * cL4;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer4->m_waterFluxArray[FL_TOP_SOURCE] * cL3;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         // b1 += appRate;
         c1 = pLayer4->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL5;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer4->m_waterFluxArray[FL_BOTTOM_SINK] * cL4; //down flux across lower surface of this layer (a loss)
         //float e111 = pLayer2->m_waterFluxArray[FL_STREAM_SINK] * cL2; //loss to streams
         //e111 = CalcSatTracerFluxes(pHRU, pLayer4, k, pFlowContext);

         advectiveFluxL1 = a1 + b1 + c1 + d1 ;
         ss = advectiveFluxL1;
         pLayer4->AddExtraSvFluxFromGlobalHandler(k, ss);
         // Finish Layer 4 - Deep unsaturated


         //Begin Layer 5 - Deep Saturated

         // float uptakeEfficiency = 0.5f;
         a1 = pLayer5->m_waterFluxArray[FL_TOP_SINK] * cL5;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer5->m_waterFluxArray[FL_TOP_SOURCE] * cL4;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         // b1 += appRate;
         c1 = pLayer5->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer5->m_waterFluxArray[FL_BOTTOM_SINK] * cL5; //down flux across lower surface of this layer (a loss)
         //float e111 = pLayer2->m_waterFluxArray[FL_STREAM_SINK] * cL2; //loss to streams
         e111 = CalcSatTracerFluxes(pHRU, pLayer5, k);

         advectiveFluxL1 = a1 + b1 + c1 + d1 + e111;
         ss = advectiveFluxL1;
         pLayer5->AddExtraSvFluxFromGlobalHandler(k, ss);
         // Finish Layer 4 - Deep Saturated



         //Begin Layer 6 - Ponded

         // float uptakeEfficiency = 0.5f;
         a1 = pLayer6->m_waterFluxArray[FL_TOP_SINK] * cL6;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer6->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         // b1 += appRate;
         c1 = pLayer6->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL1;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer6->m_waterFluxArray[FL_BOTTOM_SINK] * cL6; //down flux across lower surface of this layer (a loss)
         e111 = pLayer6->m_waterFluxArray[FL_STREAM_SINK] * cL6; //loss to streams
       /*  if (k == currYear)//add this tracer to precipitation, which is added to ponded
            {
            appRate = pLayer6->m_waterFluxArray[FL_TOP_SOURCE];//concentration in tracer is 1 kg/m3.  m3/d*1kg/m3=kg/d
            }*/
         advectiveFluxL1 = a1 + b1 + c1 + d1 + e111 ;
         ss = advectiveFluxL1;
         pLayer6->AddExtraSvFluxFromGlobalHandler(k, ss);

         if (pReach)
            {
            ReachSubnode *subnode = pReach->GetReachSubnode(0);
            subnode->AddExtraSvFluxFromGlobalHandler(k, -e111); //m3/d   e111 is negative, but we want to add a positive value here (it is a gain)
            }
            
         // Finish Layer 6 - Ponded

      }
   }
   return -1.0f;


}

float Flow_Solute::Solute_Fluxes_SRS(FlowContext *pFlowContext)
{
   if (pFlowContext->timing & 1) // Init()
      return Flow_Solute::Init_Solute_Fluxes(pFlowContext);
   if (pFlowContext->timing & GMT_START_YEAR) // Init()
      return Flow_Solute::Start_Year_Solute_Fluxes(pFlowContext);
   if (pFlowContext->timing & GMT_ENDRUN) // Init()
      return Flow_Solute::EndRun_Solute_Fluxes(pFlowContext);
   float time = pFlowContext->time;
   time = (float)fmod(time, 365);

   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();

   // ParamTable *pSoilPropertiesTable = pFlowContext->pFlowModel->GetTable("SoilProperties");
#pragma omp parallel for
   for (int h = 0; h < hruCount; h++) //for each HRU
   {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      HRUPool *pLayer1 = pHRU->GetPool(0);//unsat
      HRUPool *pLayer2 = pHRU->GetPool(1);//sat
      HRUPool *pLayer3 = pHRU->GetPool(2);//unsat argillic
      HRUPool *pLayer4 = pHRU->GetPool(3);//unsat ground
      HRUPool *pLayer5 = pHRU->GetPool(4);//sat ground
      HRUPool *pLayer6 = pHRU->GetPool(5);//ponded
      int hruLayerCount = pHRU->GetPoolCount();
      int svC = pFlowContext->svCount;

      //VData soilType = 0;

      //pFlowContext->pFlowModel->GetHRUData(pHRU, pSoilPropertiesTable->m_iduCol, soilType, DAM_FIRST);  // get soil for this particular HRU
      //if (pSoilPropertiesTable->m_type == DOT_FLOAT)
      //   soilType.ChangeType(TYPE_FLOAT);
      //float om = 0.0f;
      //float bd = 0.0f;
      //bool ok = pSoilPropertiesTable->Lookup(soilType, m_col_om, om);// get the organic matter content
      //ok = pSoilPropertiesTable->Lookup(soilType, m_col_bd, bd);// get the organic matter content

      for (int k = 0; k<svC; k++)   //for each extra state variable
      {
         StateVar *pSV = pFlowContext->pFlowModel->GetStateVar(k + 1);

         VData val = pSV->m_name.GetString();
         // int row = pSV->m_paramTable->Find(0, val, 0);
         // float koc = pSV->m_paramTable->GetAsFloat(m_col_koc, row);
         // float ks = pSV->m_paramTable->GetAsFloat(m_col_ks, row);
         float cL1 = 0.0f;
         if (pLayer1->m_volumeWater>0.000f)
            cL1 = (float)pLayer1->GetExtraStateVarValue(k ) / (float)pLayer1->m_volumeWater;

         float cL2 = 0.0f;
         if (pLayer2->m_volumeWater>0.000f)
            cL2 = (float)pLayer2->GetExtraStateVarValue(k ) / (float)pLayer2->m_volumeWater;

         float cL3 = 0.0f;
         if (pLayer3->m_volumeWater>0.000f)
            cL3 = (float)pLayer3->GetExtraStateVarValue(k ) / (float)pLayer3->m_volumeWater;

         float cL4 = 0.0f;
         if (pLayer4->m_volumeWater>0.000f)
            cL4 = (float)pLayer4->GetExtraStateVarValue(k ) / (float)pLayer4->m_volumeWater;

         float cL5 = 1.0f;
         if (pLayer5->m_volumeWater>0.000f)
            cL5 = (float)pLayer5->GetExtraStateVarValue(k ) / (float)pLayer5->m_volumeWater;

         float cL6 = 0.0f;
         if (pLayer6->m_volumeWater>0.000f)
            cL6 = (float)pLayer6->GetExtraStateVarValue(k ) / (float)pLayer6->m_volumeWater;

//ASSERT(cL2>=0.0f);
//ASSERT(cL1>=0.0f);

         //Begin Layer 1 - Unsaturated
         float ss = 0.0f;
         // float uptakeEfficiency = 0.5f;


         /*  if (k == currYear)//add this tracer to precipitation
         {
         appRate = pLayer1->m_waterFluxArray[FL_TOP_SOURCE];//concentration in tracer is 1 kg/m3.  m3/d*1kg/m3=kg/d
         }
         */
         //float appRate = ApplicationRate(time, pLayer1, pFlowContext, k);      
         // appRate=10*pLayer1->m_volumeWater;
         //   float b1 = pLayer1->m_waterFluxArray[FL_TOP_SOURCE] * cL6;//down flux across upper surface of this layer (a gain) precip,
         //if (time>100 && time<105 && pFlowContext->pEnvContext->yearOfRun==4)
         // if (time>0 && time<180 && pFlowContext->pEnvContext->yearOfRun > -1)

         MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
         int age = 0;
         pIDULayer->GetData(pHRU->m_polyIndexArray[0], m_colStandAge, age);//get fraction of CGDD from this idu		 
         int currYear = pFlowContext->pFlowModel->GetCurrentYearOfRun();
                                                                           // if (age>60 && age <62 || age == 68 || age == 38 )
                                                                           //   if (age == 1021 || age == 68 || age == 38)
         float appRate = 0.0f;
    //     if (age == 65)
            appRate = pLayer6->m_waterFluxArray[FL_TOP_SOURCE] * 1;//(1.0f - cL1)*pLayer1->m_volumeWater / pFlowContext->timeStep;   


       //Begin Layer 6 - Ponded

                                                                   // float uptakeEfficiency = 0.5f;
                                                                   // a1 = pLayer6->m_waterFluxArray[FL_TOP_SINK] * cL6;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
                                                                   // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
                                                                   // b1 = pLayer6->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
                                                                   // b1 += appRate;
         float c1 = pLayer6->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL1;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
     //    c1=0.0f;
         float d1 = pLayer6->m_waterFluxArray[FL_BOTTOM_SINK] * cL6; //down flux across lower surface of this layer (a loss)
         float e111 = pLayer6->m_waterFluxArray[FL_STREAM_SINK] * cL6; //loss to streams
                                                                 /*  if (k == currYear)//add this tracer to precipitation, which is added to ponded
                                                                 {
                                                                 appRate = pLayer6->m_waterFluxArray[FL_TOP_SOURCE];//concentration in tracer is 1 kg/m3.  m3/d*1kg/m3=kg/d
                                                                 }*/
         float advectiveFluxL1 = appRate + c1 + d1 + e111;
         ss = advectiveFluxL1;
         pLayer6->AddExtraSvFluxFromGlobalHandler(k, ss);


  
         float a1 = pLayer1->m_waterFluxArray[FL_TOP_SINK] * cL1;//upward flux across upper surface of this layer (a loss) -                                                        
         float b1 = pLayer1->m_waterFluxArray[FL_TOP_SOURCE] * cL6;
         c1 = pLayer1->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL2;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer1->m_waterFluxArray[FL_BOTTOM_SINK] * cL1; //down flux across lower surface of this layer (a loss)
         float et = pLayer1->m_waterFluxArray[FL_SINK] * cL1;
         e111= 0.0f;
         advectiveFluxL1 = a1 + b1 + c1 + d1 + et ;
         ss = advectiveFluxL1;
         pLayer1->AddExtraSvFluxFromGlobalHandler(k, ss);
         Reach * pReach = pLayer2->GetReach();

         // Finish Unsaturated

         //Begin Layer 2 - Shallow saturated

         // float uptakeEfficiency = 0.5f;
         a1 = pLayer2->m_waterFluxArray[FL_TOP_SINK] * cL2;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
                                                           // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer2->m_waterFluxArray[FL_TOP_SOURCE] * cL1;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
                                                             // b1 += appRate;
         c1 = pLayer2->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL3;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer2->m_waterFluxArray[FL_BOTTOM_SINK] * cL2; //down flux across lower surface of this layer (a loss)
                                                               //float e111 = pLayer2->m_waterFluxArray[FL_STREAM_SINK] * cL2; //loss to streams
         e111 = CalcSatTracerFluxes(pHRU, pLayer2, k);
         et = pLayer2->m_waterFluxArray[FL_SINK] * cL2;
         advectiveFluxL1 = a1 + b1 + c1 + d1 + e111 + et;
         ss = advectiveFluxL1;
         pLayer2->AddExtraSvFluxFromGlobalHandler(k, ss);


         // Finish Shallow Saturated

         //Begin Layer  3 - Argillic

         // float uptakeEfficiency = 0.5f;
         a1 = pLayer3->m_waterFluxArray[FL_TOP_SINK] * cL3;//upward flux across upper surface of this layer (a loss) - 
                                                           // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer3->m_waterFluxArray[FL_TOP_SOURCE] * cL2;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
                                                             // b1 += appRate;
         c1 = pLayer3->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL4;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
   //      c1=0.0f;         
         d1 = pLayer3->m_waterFluxArray[FL_BOTTOM_SINK] * cL3; //down flux across lower surface of this layer (a loss)
                                                               //float e111 = pLayer2->m_waterFluxArray[FL_STREAM_SINK] * cL2; //loss to streams
                                                               //e111 = CalcSatTracerFluxes(pHRU, pLayer3, k, pFlowContext);
         et = pLayer3->m_waterFluxArray[FL_SINK] * cL3;         
         advectiveFluxL1 = a1 + b1 + c1 + d1 + et;
         ss = advectiveFluxL1;
         pLayer3->AddExtraSvFluxFromGlobalHandler(k, ss);
         // Finish Argillic


         //Begin Layer 4 - Deep unsaturated

         a1 = pLayer4->m_waterFluxArray[FL_TOP_SINK] * cL4;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
                                                           // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer4->m_waterFluxArray[FL_TOP_SOURCE] * cL3;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
                                                             // b1 += appRate;
         c1 = pLayer4->m_waterFluxArray[FL_BOTTOM_SOURCE] * cL5;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer4->m_waterFluxArray[FL_BOTTOM_SINK] * cL4; //down flux across lower surface of this layer (a loss)
                                                               //float e111 = pLayer2->m_waterFluxArray[FL_STREAM_SINK] * cL2; //loss to streams
                                                               //e111 = CalcSatTracerFluxes(pHRU, pLayer4, k, pFlowContext);

         advectiveFluxL1 = a1 + b1 + c1 + d1;
         ss = advectiveFluxL1;
         pLayer4->AddExtraSvFluxFromGlobalHandler(k, ss);
         // Finish Layer 4 - Deep unsaturated


         //Begin Layer 5 - Deep Saturated
         a1 = pLayer5->m_waterFluxArray[FL_TOP_SINK] * cL5;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
                                                           // float appRate = ApplicationRate(time, pLayer2, pFlowContext, k);
         b1 = pLayer5->m_waterFluxArray[FL_TOP_SOURCE] * cL4;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
                                                             // b1 += appRate;
         c1 = pLayer5->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pLayer5->m_waterFluxArray[FL_BOTTOM_SINK] * cL5; //down flux across lower surface of this layer (a loss)
                                                               //float e111 = pLayer2->m_waterFluxArray[FL_STREAM_SINK] * cL2; //loss to streams
         e111 = CalcSatTracerFluxes(pHRU, pLayer5, k);


         advectiveFluxL1 = a1 + b1 + c1 + d1 + e111;
         ss = advectiveFluxL1;
         pLayer5->AddExtraSvFluxFromGlobalHandler(k, ss);
         // Finish Layer 4 - Deep Saturated




         if (pReach)
            {
            ReachSubnode *subnode = pReach->GetReachSubnode(0);
            subnode->AddExtraSvFluxFromGlobalHandler(k, -e111); //m3/d   e111 is negative, but we want to add a positive value here (it is a gain)
            }
/*
         if (pHRU->m_id==207)
            {
            float *data = new float[30];
            data[0] = time;
            data[1] = pHRU->m_centroid.x;
            data[2] = pHRU->m_centroid.y;
            data[3] = pLayer6->m_waterFluxArray[FL_TOP_SOURCE];
            data[4] = pLayer6->m_waterFluxArray[FL_BOTTOM_SOURCE];
            data[5] = pLayer6->m_waterFluxArray[FL_STREAM_SINK];
            data[6] = pLayer6->m_waterFluxArray[FL_TOP_SINK];
            data[7] = pLayer1->m_waterFluxArray[FL_TOP_SINK];
            data[8] = pLayer1->m_waterFluxArray[FL_TOP_SOURCE];
            data[9] = pLayer1->m_waterFluxArray[FL_BOTTOM_SOURCE] ;
            data[10] = pLayer1->m_waterFluxArray[FL_BOTTOM_SINK];
            data[11] = pLayer1->m_waterFluxArray[FL_SINK];
            data[12] = pLayer2->m_waterFluxArray[FL_TOP_SINK] ;
            data[13] = pLayer2->m_waterFluxArray[FL_TOP_SOURCE] ;
            data[14] = pLayer2->m_waterFluxArray[FL_BOTTOM_SOURCE];
            data[15] = pLayer2->m_waterFluxArray[FL_BOTTOM_SINK];
            data[16] = pLayer2->m_waterFluxArray[FL_SINK];
            data[17] = pLayer3->m_waterFluxArray[FL_TOP_SINK];
            data[18] = pLayer3->m_waterFluxArray[FL_TOP_SOURCE];//kg/ha
            data[19] = pLayer3->m_waterFluxArray[FL_BOTTOM_SOURCE];//kg/ha
            data[20] = pLayer3->m_waterFluxArray[FL_BOTTOM_SINK];//kg/ha
            data[21] = pLayer3->m_waterFluxArray[FL_SINK];
            data[22] = pLayer4->m_waterFluxArray[FL_TOP_SINK];
            data[23] = pLayer4->m_waterFluxArray[FL_TOP_SOURCE];//kg/ha
            data[24] = pLayer4->m_waterFluxArray[FL_BOTTOM_SOURCE];//kg/ha
            data[25] = pLayer4->m_waterFluxArray[FL_BOTTOM_SINK];//kg/ha
            data[26] = pLayer5->m_waterFluxArray[FL_TOP_SINK];
            data[27] = pLayer5->m_waterFluxArray[FL_TOP_SOURCE];//kg/ha
            data[28] = pLayer5->m_waterFluxArray[FL_BOTTOM_SOURCE];//kg/ha
            data[29] = pLayer5->m_waterFluxArray[FL_BOTTOM_SINK];//kg/ha

            float *data = new float[30];
            data[0] = time;
            data[1] = pFlowContext->time;
            data[2] = pHRU->m_centroid.y;
            data[3] = pHRU->m_elws;
            data[4] = pHRU->m_id;
            data[5] = (float)pLayer1->GetExtraStateVarValue(k);
            data[6] = (float)pLayer1->m_volumeWater;
            data[7] = (float)pLayer1->GetExtraStateVarValue(k) / (float)pLayer1->m_volumeWater;
            data[8] = (float)pLayer2->GetExtraStateVarValue(k);
            data[9] = (float)pLayer2->m_volumeWater; 
            data[10] = (float)pLayer2->GetExtraStateVarValue(k) / (float)pLayer2->m_volumeWater;
            data[11] = (float)pLayer3->GetExtraStateVarValue(k);
            data[12] = (float)pLayer3->m_volumeWater;
            data[13] = (float)pLayer3->GetExtraStateVarValue(k) / (float)pLayer3->m_volumeWater;
            data[14] = (float)pLayer4->GetExtraStateVarValue(k);
            data[15] = (float)pLayer4->m_volumeWater;
            data[16] = (float)pLayer4->GetExtraStateVarValue(k) / (float)pLayer4->m_volumeWater;
            data[17] = (float)pLayer5->GetExtraStateVarValue(k);
            data[18] = (float)pLayer5->m_volumeWater;
            data[19] = (float)pLayer5->GetExtraStateVarValue(k) / (float)pLayer5->m_volumeWater;
            data[20] = (float)pLayer6->GetExtraStateVarValue(k);
            data[21] = (float)pLayer6->m_volumeWater;
            data[22] = (float)pLayer6->GetExtraStateVarValue(k) / (float)pLayer6->m_volumeWater;
            data[23] = 0;
            data[24] = 0; 
            data[25] = 0;
            data[26] = pLayer5->m_waterFluxArray[FL_TOP_SINK];
            data[27] = pLayer5->m_waterFluxArray[FL_TOP_SOURCE];//kg/ha
            data[28] = pLayer5->m_waterFluxArray[FL_BOTTOM_SOURCE];//kg/ha
            data[29] = pLayer5->m_waterFluxArray[FL_BOTTOM_SINK];//kg/ha
            m_pRateData->AppendRow(data, 30);
            delete[] data;
            }
*/
      }
   }
   return -1.0f;


}


float Flow_Solute::CalcSatTracerFluxes(HRU *pHRU, HRUPool *pHRULayer, int k )
   {

   float outflow = 0.0f;
   float inflow = 0.0f;
   for (int upCheck = 0; upCheck<pHRU->m_neighborArray.GetSize(); upCheck++)
      {
      HRU *pUpCheck = pHRU->m_neighborArray[upCheck];
      ASSERT(pUpCheck != NULL);
     
      if (pUpCheck != NULL)
         {
         //StateVar *pSV = pFlowContext->pFlowModel->GetStateVar(sV + 1);
         //VData val = pSV->m_name.GetString();

         float conc = 0.0f; float concUp=0.0f;
         if (pHRULayer->m_volumeWater>0.0f)
            conc = (float)pHRULayer->GetExtraStateVarValue(k) / (float)pHRULayer->m_volumeWater;

         HRUPool *pUpCheckLayer = pUpCheck->GetPool(pHRULayer->m_pool);
         if (pUpCheckLayer->m_volumeWater>0.0f)
            concUp = (float)pUpCheckLayer->GetExtraStateVarValue(k) / (float)pUpCheckLayer->m_volumeWater;

         float flux=pHRULayer->m_waterFluxArray[upCheck + 7];
         if (flux<0.0f)//water leaves this grid cell in this direction
            outflow += (flux * conc);//kg/d
         else
            inflow += (flux * concUp);//kg/d
         }
      }

   return  inflow+outflow;//outflow has a negative sign, so we add a negative value - effectively subtracting outflow from inflow
   }
/*
float Flow_Solute::Solute_Fluxes( FlowContext *pFlowContext )
{
if ( pFlowContext->timing & 1 ) // Init()
return Flow_Solute::Init_Solute_Fluxes( pFlowContext );

int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
int hruCount = pFlowContext->pFlowModel->GetHRUCount();

// #pragma omp parallel for firstprivate( pFlowContext )
for ( int h=0; h < hruCount; h++ )
{
HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
HRULayer *pOverLayer = pHRU->GetLayer( 0 );
HRULayer *pUpperLayer = pHRU->GetLayer( 1 );
HRULayer *pLowerLayer = pHRU->GetLayer( 2 );

int svC = pFlowContext->svCount;
for (int k=0;k<svC;k++)
{
float concOver=0.0f;
if (pOverLayer->m_volumeWater>0.0f)
concOver=(float)pOverLayer->GetExtraStateVarValue(k)/(float)pOverLayer->m_volumeWater;

float concUpper=0.0f;
if (pUpperLayer->m_volumeWater>0.0f)
concUpper=(float)pUpperLayer->GetExtraStateVarValue(k)/(float)pUpperLayer->m_volumeWater;

float concLower=0.0f;
if (pLowerLayer->m_volumeWater>0.0f)
concLower=(float)pLowerLayer->GetExtraStateVarValue(k)/(float)pLowerLayer->m_volumeWater;

float ss=0.0f;

float a1 = pOverLayer->m_waterFluxArray[FL_TOP_SINK]*concOver;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
float b1 = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE]*0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
float c1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SOURCE]*0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
float d1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SINK]*0; //down flux across lower surface of this layer (a loss)
float e111 = pOverLayer->m_waterFluxArray[FL_STREAM_SINK]*concOver; //loss to streams

ss=a1+b1+c1+d1+e111;
// ss=0.0f;
pOverLayer->AddExtraSvFluxFromGlobalHandler(k, ss);

a1 = pUpperLayer->m_waterFluxArray[FL_TOP_SINK]*concUpper;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
b1 = pUpperLayer->m_waterFluxArray[FL_TOP_SOURCE]*0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
c1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SOURCE]*concLower;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
d1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SINK]*concUpper; //down flux across lower surface of this layer (a loss)
float e11 = pUpperLayer->m_waterFluxArray[FL_STREAM_SINK]*concUpper; //loss to streams

float jdw = ks*mass; //ks is a lumped first order rate constant
float e=pow(0.784f,((log(kow)-1.78)*(log(kow)-1.78))/2.44f);
float ju=f*e*mass;//uptake rate. fi s fraction of water used for et over the timestep, e is an efficiency factor
float jer=sm*rom*kd*cw;
float jfof=

ss=a1+b1+c1+d1+e11;

// ss=0.0f;
pUpperLayer->AddExtraSvFluxFromGlobalHandler(k, ss);

float e1 = pLowerLayer->m_waterFluxArray[FL_TOP_SINK]*concLower;//upward flux across upper surface of this layer (a loss) -saturated to unsaturated zone
float f1 = pLowerLayer->m_waterFluxArray[FL_TOP_SOURCE]*concUpper;//down flux across upper surface of this layer (a gain) - gwREcharge
float g1 = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SOURCE]*0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturate zone
float h1 = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SINK]*concLower; //down flux across lower surface of this layer (a loss)
float i1 = 0.0f; // outgoing flow to horizontal neighbors
float j1 = 0.0f; // incoming flow from horizontal neighbors

for (int upCheck=0;upCheck<pHRU->m_neighborArray.GetSize(); upCheck++)
{
HRU *pUpCheck = pHRU->m_neighborArray[ upCheck ];
HRULayer *pUpCheckLayer = pUpCheck->GetLayer(2);

float concNeighbor=0.0f;
if (pUpCheckLayer->m_volumeWater>0.0f)
concNeighbor=(float)pUpCheckLayer->GetExtraStateVarValue(k)/(float)pUpCheckLayer->m_volumeWater;
if (pLowerLayer->m_waterFluxArray[upCheck+7] <= 0.0f)//negative flow, so this flux is leaving me
i1+=pLowerLayer->m_waterFluxArray[upCheck+7]*concLower;
else                                           //positive flow, so this flux is leaving my neighbor
j1+=pLowerLayer->m_waterFluxArray[upCheck+7]*concNeighbor;
}

ss=e1+f1+g1+h1+i1+j1;
ss=0.0f;

pLowerLayer->AddExtraSvFluxFromGlobalHandler(k, ss );     //m3/d
Reach * pReach = pUpperLayer->GetReach();
ReachSubnode *subnode = pReach->GetReachSubnode(0);
if ( pReach )
subnode->AddExtraSvFluxFromGlobalHandler(k, ((-e11-e111)) ); //kg/d.  e1 is stored as a loss from the unsaturated zone - so it is negative and needs to be converted to a + value.
}

}
return -1.0f;
}
*/

float Flow_Solute::Solute_Fluxes(FlowContext *pFlowContext)
   {
   if (pFlowContext->timing & 1) // Init()
      return Flow_Solute::Init_Solute_Fluxes(pFlowContext);
   if (pFlowContext->timing & GMT_START_YEAR) // Init()
      return Flow_Solute::Start_Year_Solute_Fluxes(pFlowContext);
   float time = pFlowContext->time;
   time = (float)fmod(time, 365);  

   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();

   ParamTable *pSoilPropertiesTable = pFlowContext->pFlowModel->GetTable("SoilProperties");
   for (int h = 0; h < hruCount; h++) //for each HRU
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      HRUPool *pOverLayer = pHRU->GetPool(0);
      HRUPool *pUpperLayer = pHRU->GetPool(1);
      HRUPool *pLowerLayer = pHRU->GetPool(2);
      int hruLayerCount = pHRU->GetPoolCount();
      int svC = pFlowContext->svCount;

      VData soilType = 0;

      pFlowContext->pFlowModel->GetHRUData(pHRU, pSoilPropertiesTable->m_iduCol, soilType, DAM_FIRST);  // get soil for this particular HRU
      if (pSoilPropertiesTable->m_type == DOT_FLOAT)
         soilType.ChangeType(TYPE_FLOAT);
      float om = 0.0f;
      float bd = 0.0f;
      bool ok = pSoilPropertiesTable->Lookup(soilType, m_col_om, om);// get the organic matter content
      ok = pSoilPropertiesTable->Lookup(soilType, m_col_bd, bd);// get the organic matter content

      for (int k = 0; k<svC; k++)   //for each extra state variable
         {
         StateVar *pSV = pFlowContext->pFlowModel->GetStateVar(k + 1);

         VData val = pSV->m_name.GetString();
         int row = pSV->m_paramTable->Find(0, val, 0);
         float koc = pSV->m_paramTable->GetAsFloat(m_col_koc, row);
         float ks = pSV->m_paramTable->GetAsFloat(m_col_ks, row);

         float concOver = 0.0f;
         if (pOverLayer->m_volumeWater>0.0f)
            concOver = (float)pOverLayer->GetExtraStateVarValue(k) / (float)pOverLayer->m_volumeWater;

         float concUpper = 0.0f;
         if (pUpperLayer->m_volumeWater > 0.0f)
            concUpper = (float)pUpperLayer->GetExtraStateVarValue(k) / (float)pUpperLayer->m_volumeWater;

         float concLower = 0.0f;
         if (pLowerLayer->m_volumeWater > 0.0f)
            concLower = (float)pLowerLayer->GetExtraStateVarValue(k) / (float)pLowerLayer->m_volumeWater;


         //Begin Ponded Calculation
         float ss = 0.0f;
         float uptakeEfficiency = 0.5f;
         float a1 = pOverLayer->m_waterFluxArray[FL_TOP_SINK] * concOver * uptakeEfficiency;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         float appRate = ApplicationRate(time, pOverLayer, pFlowContext, k);
         float b1 = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         b1 += appRate;
         float c1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         float d1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SINK] * 0; //down flux across lower surface of this layer (a loss)
         float e111 = pOverLayer->m_waterFluxArray[FL_STREAM_SINK] * concOver; //loss to streams

         float advectiveFluxOver = a1 + b1 + c1 + d1 + e111;


         float kd = koc*om / 1000.0f; //ml/g
         float transformation = (ks*pOverLayer->GetExtraStateVarValue(k)) + (concOver*kd*ks*bd);//degraded of absorbed

         ss = advectiveFluxOver - transformation;
         pOverLayer->AddExtraSvFluxFromGlobalHandler(k, ss);
         // Finish Ponded Calculations

         // Begin Upper Soil Calculations

         a1 = pUpperLayer->m_waterFluxArray[FL_TOP_SINK] * concUpper;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         b1 = pUpperLayer->m_waterFluxArray[FL_TOP_SOURCE] * concOver;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         c1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * concLower;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SINK] * concUpper; //down flux across lower surface of this layer (a loss)
         float e11 = pUpperLayer->m_waterFluxArray[FL_STREAM_SINK] * concUpper; //loss to streams

         advectiveFluxOver = a1 + b1 + c1 + d1 + e11;

         transformation = (ks*pUpperLayer->GetExtraStateVarValue(k)) + (concUpper*kd*ks*bd);//degraded of absorbed

         ss = advectiveFluxOver - transformation;
         pUpperLayer->AddExtraSvFluxFromGlobalHandler(k, ss);

         // Finish Upper Soil Calculations

         // Begin Lower Soil Calculations

         float e1 = pLowerLayer->m_waterFluxArray[FL_TOP_SINK] * concLower;//upward flux across upper surface of this layer (a loss) -saturated to unsaturated zone
         float f1 = pLowerLayer->m_waterFluxArray[FL_TOP_SOURCE] * concUpper;//down flux across upper surface of this layer (a gain) - gwREcharge
         float g1 = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturate zone
         float h1 = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SINK] * concLower; //down flux across lower surface of this layer (a loss)
         float n1 = 0.0f;

         float concNeighbor = 0.0f;
         for (int upCheck = 0; upCheck < pHRU->m_neighborArray.GetSize(); upCheck++)
            {
            HRU *pUpCheck = pHRU->m_neighborArray[upCheck];
            if (pUpCheck != NULL)
               {
               HRUPool *pUpCheckLayer = pUpCheck->GetPool(2);
               if (pLowerLayer->m_waterFluxArray[upCheck + 7]<0.0f)//an outflow   
                  n1 += pLowerLayer->m_waterFluxArray[upCheck + 7] * concLower;
               else                                              //an inflow, so we need to know the concentration in the neighbor
                  {
                  if (pUpCheckLayer->m_volumeWater > 0.0f)
                     concNeighbor = (float)pUpCheckLayer->GetExtraStateVarValue(k) / (float)pUpCheckLayer->m_volumeWater;
                  n1 += pLowerLayer->m_waterFluxArray[upCheck + 7] * concNeighbor;
                  }
               }
            }

         advectiveFluxOver = e1 + f1 + g1 + h1 + n1;

         transformation = (ks*pLowerLayer->GetExtraStateVarValue(k)) + (concLower*kd*ks*bd);//degraded of absorbed
         ss = advectiveFluxOver - transformation;
         pLowerLayer->AddExtraSvFluxFromGlobalHandler(k, ss);     //m3/d

         // End Lower Soil Layer Calculations


         Reach * pReach = pUpperLayer->GetReach();
         ReachSubnode *subnode = pReach->GetReachSubnode(0);
         if (pReach)
            subnode->AddExtraSvFluxFromGlobalHandler(k, ((-e11 - e111))); //kg/d.  e1 is stored as a loss from the unsaturated zone - so it is negative and needs to be converted to a + value.
         }
      }
   return -1.0f;


   }
   float Flow_Solute::ApplicationRate(float time, HRUPool *pHRULayer, FlowContext *pFlowContext, int k)
   {
	   HRU *pHRU = pHRULayer->m_pHRU;
	   StateVar *pSV = pFlowContext->pFlowModel->GetStateVar(k + 1);
	   int ipm;
	   pFlowContext->pFlowModel->GetHRUData(pHRU, m_colIPM, ipm, DAM_FIRST);  // get crop for this particular HRU 
	   float distStr;
	   pFlowContext->pFlowModel->GetHRUData(pHRU, m_colDistStr, distStr, DAM_AREAWTMEAN);  // get crop for this particular HRU 
	   ParamTable *pApplication_Table = pSV->m_tableArray[0];
	   //Get Model Parameters
	   VData cropNum;
	   float appRate = 0.0f; float appStart = 1E6; float appFinish = 1E6;

	   pFlowContext->pFlowModel->GetHRUData(pHRU, pApplication_Table->m_iduCol, cropNum, DAM_FIRST);  // get crop for this particular HRU

	   if (pApplication_Table->m_type == DOT_FLOAT)
		   cropNum.ChangeType(TYPE_FLOAT);

	   int c = -1;
	   cropNum.GetAsInt(c);

	   bool ok = pApplication_Table->Lookup(cropNum, m_col_AppStart, appStart);// get the start application date (it is a day of year in the table

	   if (time > appStart && time <= appStart + 1.f)
	   {
		   if (ipm < 2)//no ipm
			   ok = pApplication_Table->Lookup(cropNum, m_col_AppRateConventional, appRate);// get the application rate for this crop

		   if (ipm == 2)//partial ipm
			   ok = pApplication_Table->Lookup(cropNum, m_col_AppRatePartialIPM, appRate);// get the application rate for this crop

		   if (ipm == 3)//partial ipm - 10 m stream buffer
		   {
			   if (distStr>10)
				   ok = pApplication_Table->Lookup(cropNum, m_col_AppRatePartialIPM, appRate);// get the application rate for this crop
		   }

		   if (ipm == 4)//partial ipm - 100 m stream buffer
		   {
			   if (distStr > 100)
				   ok = pApplication_Table->Lookup(cropNum, m_col_AppRatePartialIPM, appRate);// get the application rate for this crop
		   }
		   if (ipm == 5)//no fertilizer - 100 m stream buffer
		   {
			   if (distStr > 100)
				   ok = pApplication_Table->Lookup(cropNum, m_col_AppRateFullIPM, appRate);// get the application rate for this crop
		   }
	   }

	   return appRate;
   }
  float Flow_Solute::NitrogenApplicationRate(float time, HRUPool *pHRULayer, FlowContext *pFlowContext, int k)
   {
   HRU *pHRU = pHRULayer->m_pHRU;
   StateVar *pSV = pFlowContext->pFlowModel->GetStateVar(k + 1);
   VData cropNum;
   int N_app;
   pFlowContext->pFlowModel->GetHRUData(pHRU, m_colN_app, N_app, DAM_MAJORITYAREA);  // get nitrogen application strategy for this particular HRU 
   float distStr;
   pFlowContext->pFlowModel->GetHRUData(pHRU, m_colDistStr, distStr, DAM_MAJORITYAREA);  // get distance to stream for this particular HRU 
   ParamTable *pApplication_Table = pSV->m_tableArray[0];
   //Get Model Parameters
   
   float appRate = 0.0f; float appStart = 1E6; float appFinish = 1E6;

   pFlowContext->pFlowModel->GetHRUData(pHRU, pApplication_Table->m_iduCol, cropNum, DAM_MAJORITYAREA);  // get cropNum for this particular IDU

   if (pApplication_Table->m_type == DOT_FLOAT)
      cropNum.ChangeType(TYPE_FLOAT);

   int c = -1;
   cropNum.GetAsInt(c);

   bool ok = pApplication_Table->Lookup(cropNum, m_col_AppStart, appStart);// get the start application date (it is a day of year in the table

   if (time > appStart && time <= appStart + 1.f)
      {
      if (N_app < 1)//no fertilizer applied
		  ok = pApplication_Table->Lookup(cropNum, m_col_AppRateZero, appRate);// get the application rate for this crop

      if (N_app == 1)//Minimum Fertilizer Application Rate
         ok = pApplication_Table->Lookup(cropNum, m_col_AppRateMinimum, appRate);// get the application rate for this crop

      if (N_app == 2)//OSU Recommended Fertilizer Application Rate
		  ok = pApplication_Table->Lookup(cropNum, m_col_AppRateRecommended, appRate);// get the application rate for this crop

	   if (N_app == 3)//Maximum Fertilizer Application Rate
		  ok = pApplication_Table->Lookup(cropNum, m_col_AppRateConventional, appRate);// get the application rate for this crop

	   //if (N_app == 4)//OSU Recommended Fertilizer Application Rate - No application within 10 m stream buffer
    //     {
    //     if (distStr>10)
			 //ok = pApplication_Table->Lookup(cropNum, m_col_AppRateRecommended, appRate);// get the application rate for this crop
    //     }
	   //if (N_app == 5)//OSU Recommended Fertilizer Application Rate - No application within - 100 m stream buffer
    //     {
    //     if (distStr > 100)
			 //ok = pApplication_Table->Lookup(cropNum, m_col_AppRateRecommended, appRate);// get the application rate for this crop
    //     }
	   //if (N_app == 6)//no fertilizer - 100 m stream buffer
    //     {
    //     if (distStr > 100)
			 //ok = pApplication_Table->Lookup(cropNum, m_col_AppRateZero, appRate);// get the application rate for this crop
    //     }
      }

   return appRate;
   } 
float Flow_Solute::HBV_N_Fluxes(FlowContext *pFlowContext)
   {
   if (pFlowContext->timing & 1) // Init()
      return Flow_Solute::Init_Solute_Fluxes(pFlowContext);

   if (pFlowContext->timing & GMT_ENDRUN) // End()
      return Flow_Solute::EndRun_Solute_Fluxes(pFlowContext);


   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();


   float time = pFlowContext->time;
   float tim = pFlowContext->time;
   float year = (float)pFlowContext->pEnvContext->currentYear;
   time = (float)fmod(time, 365);
   

   ParamTable *pSoilPropertiesTable = pFlowContext->pFlowModel->GetTable("SoilData");
   m_col_om = pSoilPropertiesTable->GetFieldCol("OM");
   m_col_AWC = pSoilPropertiesTable->GetFieldCol("AWC");
   m_col_percent_clay = pSoilPropertiesTable->GetFieldCol("CLAY");
   m_col_thickness = pSoilPropertiesTable->GetFieldCol("THICK");
   m_col_albedo = pSoilPropertiesTable->GetFieldCol("ALBEDO");
   m_col_SAT = pSoilPropertiesTable->GetFieldCol("SAT");
   m_col_BD = pSoilPropertiesTable->GetFieldCol("BD");
   m_col_SSURGO_WP = pSoilPropertiesTable->GetFieldCol("WP");
   m_col_SSURGO_FC = pSoilPropertiesTable->GetFieldCol("WTHIRDBAR");

   ParamTable *pCropNUptakeTable = pFlowContext->pFlowModel->GetTable("Crop_N_Uptake");
   m_col_PHU = pCropNUptakeTable->GetFieldCol("PHU");
   m_colTbase = pCropNUptakeTable->GetFieldCol("Tbase");
   m_col_fr_n1 = pCropNUptakeTable->GetFieldCol("fr_n1");
   m_col_fr_n2 = pCropNUptakeTable->GetFieldCol("fr_n2");
   m_col_fr_n3 = pCropNUptakeTable->GetFieldCol("fr_n3");
   m_colRUE = pCropNUptakeTable->GetFieldCol("RUE");
   m_colLAI = pCropNUptakeTable->GetFieldCol("LAI");
    
   for (int h = 0; h < hruCount; h++) //for each HRU
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      HRUPool *pOverLayer = pHRU->GetPool(0);
      HRUPool *pUpperLayer = pHRU->GetPool(1);
      HRUPool *pLowerLayer = pHRU->GetPool(2);
      int hruLayerCount = pHRU->GetPoolCount();
      int svC = pFlowContext->svCount;

      //IDU Attributes
      float euler_e = 2.71828182845904523536f;
      float area_hru = pHRU->m_area / 10000.0f;										//(hectares) = 10,000 m2

	  //SOIL PROPERTIES
      VData soilType = 0;

      float SoilWater = pOverLayer->m_wc;															         //layer water content (mmH20)
	   pFlowContext->pFlowModel->GetHRUData(pHRU, pSoilPropertiesTable->m_iduCol, soilType, DAM_MAJORITYAREA);  // get soil for this particular HRU
	   if (pSoilPropertiesTable->m_type == DOT_FLOAT)
		   soilType.ChangeType(TYPE_FLOAT);
	   float OM = 0.0f;																		            //organic matter content (%)
      float WP = 100.0f;																	               //wilting point (mmH20), wfifteenbar in SSURGO expressed as volumetric water content (%)
      float field_capacity = 441.113f;                                                 //field capacity (mmH20), wthirdbar in SSURGO expresed as volumetric water content (%)
      float AWC = field_capacity - WP;												               //available water capacity (mmH20)
	   float albedo = 0.0f;																	            //albedodry in SSURGO, ratio of the incident short-wave (solar) radiation that is reflected by the soil surface
	   float SAT = 0.0f;																	               //saturated water content of the layer (mmH20) 
	   float bulk_density = 0.0f;															            //bulk density (Mg/m3), dbovendry in SSURGO, oven dry weight
	   float thickness = 0.0f;																            //depth_ly(mm) ----> interpreted this as layer thickness 
	   float percent_clay = 0.0f;															            //claytotal_r in SSURGO
      //float field_capacity = 0.0f;                                                     //wthirdbar in SSURGO, volumetric water content at thirdbar
	   bool ok = pSoilPropertiesTable->Lookup(soilType, m_col_om, OM);						// get the organic matter content
	   ok = pSoilPropertiesTable->Lookup(soilType, m_col_thickness, thickness);			// get the thickness 
	   //ok = pSoilPropertiesTable->Lookup(soilType, m_col_SSURGO_WP, SSURGO_WP);			// get the wilting point 													
	   //ok = pSoilPropertiesTable->Lookup(soilType, m_col_AWC, AWC);							// get the available water content
	   ok = pSoilPropertiesTable->Lookup(soilType, m_col_albedo, albedo);					// get the albedo
	   ok = pSoilPropertiesTable->Lookup(soilType, m_col_SAT, SAT);							// get the saturated water content
	   ok = pSoilPropertiesTable->Lookup(soilType, m_col_BD, bulk_density);					// get the bulk density	  
	   ok = pSoilPropertiesTable->Lookup(soilType, m_col_percent_clay, percent_clay);	// get the percent clay
      //ok = pSoilPropertiesTable->Lookup(soilType, m_col_SSURGO_FC, SSURGO_FC);         // get the field capacity
      thickness = thickness;															               //depth_ly(mm) = (hzdepb_r - hzdept_r)(cm)*10(mm/cm)--->this conversion is done in the table
      /*OM = OM / 100; */                                                              //OM(%)=OM(%*100)/100
      //SSURGO_WP = (SSURGO_WP/100)*thickness;															//WP(mmH20)=wfifteenbar(%*100)*thickness(mm) 
      //SSURGO_FC = (SSURGO_FC / 100)*thickness;														//WP(mmH20)=wthirdbar(%*100)*thickness(mm)
      //AWC = (AWC/100)*thickness;																	      //AWC(mmH20)=wthirdbar(field capacity) - wfifteenbar(wilting point)(%*100)*thickness(mm). In SSURGO, awc is estimated as difference between wthirdbar(field capacity) and wfifteenbar(wilting point)
	   SAT = (SAT/100)*thickness;																	      //SAT(mmH20)=wsatiated(%*100)*thickness(mm)
	   float	orgC = OM / 1.72f;															            //organic carbon content of the layer (%)
	   float theta_e = 0.50f;																            //fraction of porosity from which anions are excluded, SWAT default=0.50	  
      float depth_surf = 10.0f;                                                           //depth of the surface layer (mm)
      float z = 1.f;																		               //depth to layer top(mm)	  
	   float z_mid = 5.0f;													         //depth to layer mid (mm), set to 5 because averaging the entire soil layer thickness was underestimating volatilization
	   float phi = SoilWater / ((0.356f - 0.144f*bulk_density)*thickness);		//
      float dd_max = 1000.f + (2500.f * bulk_density) / (bulk_density + 686.f * exp(- 5.63f*bulk_density)); //
      float dd = dd_max*exp((log(500.f / dd_max))*((1.f - phi) / (1.f + phi)*(1.f + phi))); //
	   float zd = z / dd;																	            //ratio of depth at layer center to dampening depth, z/dd			  		
      float df = zd / (zd + exp(- 0.867f - 2.078f*zd));							//depth factor used in soil calculations   		
	  	  
	   //CLIMATE VARIABLES
      float rad = 0.0f;                                                                //
      float H_day = 0.0f;																	            //incident total solar (MJ/m^2)
	   float TAAir = 11.6f;																	            //annual avg air temp (degrees C)	  														 
	   float Tav = 0.0f;																	               //air temperature average for day (C)
	   float Tmax = 0.0f;																	            //air temperature maximum for day (C)
	   float Tmin = 0.0f;																	            //air temperature minimum for day (C)
	   pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, pFlowContext->dayOfYear, Tmax);//C;                         
	   pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, pFlowContext->dayOfYear, Tmin);//C;
	   pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, pFlowContext->dayOfYear, Tav);//C; 
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, pFlowContext->dayOfYear, rad);//W/m2
      //H_day*=(0.001f);                                                                  //(MJ/m^2)
      // float H_day_KJ_m2 = H_day*3.6f;//KJ/m2
      //  float H_day_MJ_m2 = H_day_KJ_m2 / 1000.0f;
      //   H_day = H_day_MJ_m2;
      float MJPERD_PER_W = 0.0864f;             // Watts/m^2 -> MegaJoules/m^2*day
      H_day = rad * MJPERD_PER_W;                         // incoming radiation MJ/(m^2 d) 

 
      float precip = 0.0f;																	            //initialize precip

      pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, pFlowContext->dayOfYear, precip);//mm;	//amount of rainfall in a given day (mm H20)
      precip *= pHRU->m_area / 1000.0f;//m3/d
      Reach * pReach = pUpperLayer->GetReach();

      ////////////////////////////////////////LAYER 1 Advective Movement////////////////////////////////////////////

      //Precipitation and fertilizer application are additions to this layer - they are not accounted for here

      // Andrew needs to verify that mobileN1 is less than concOver_NO3, but not that much less.  If it is a lot different
      // consider simply allowing ALL the NO3 to be mobile, or perhaps some constant percentage of it.
      //NITROGEN STATE VARIABLES
      float NO3 = (float)pOverLayer->GetExtraStateVarValue(0);				               //(kg)
      //if (tim == 0.0f)
      //   NO3 = 7.f * pow(euler_e, (-z / 1000.f));						                     //initial nitrate levels in soil are varied by layer depth
      float NH4 = (float)pOverLayer->GetExtraStateVarValue(1);				               //(kg)
      float OrgN = (float)pOverLayer->GetExtraStateVarValue(2);			               //(kg)
      if (tim == 0.0f)
        OrgN = pow(10, 4)*(orgC / 14);							                              //initial concentration (mg/kg) of humic organic nitrogen is a function of organic carbon content
                  
      float B_NO3 = 0.2f;			                                                      //nitrate percolation coefficient, SWAT recommends range from 0.01 - 1.0
      
      //float w_perc1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SINK];	   //amount of water percolating to the underlying soil layer on a given day (mmH20)
      //float Q_lat1 = pOverLayer->m_waterFluxArray[FL_STREAM_SINK];	      //amount of water going to stream as subsurface runoff on a given day (mmH20)
      //float w_mobile1 = Q_lat1 + w_perc1;												         //amount of mobile water in the layer (mm H2O)
      float concOver_NO3 = 0.0f;
      if (pOverLayer->m_volumeWater > 0.0f )
         concOver_NO3 = (float)pOverLayer->GetExtraStateVarValue(0) / ((float)pOverLayer->m_volumeWater);	//(kg/mmH2O)
      float mobileN1 = 0.0f;
      if ((float)pOverLayer->GetExtraStateVarValue(0) > 0.0f)
         mobileN1 = concOver_NO3;
      /*if (w_mobile1 > 0.0f)
         mobileN1 = (float)pOverLayer->GetExtraStateVarValue(0) *(((1 - pow(euler_e, ((-1.0f*w_mobile1) / ((1 - theta_e)*SAT)))) / w_mobile1));*/
      
      float NO3_Rainfall = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0.0001f;       //kg/m3 or 1000 mg/l
      float sink = 0.0f;
      if (mobileN1 > 0.0f)
         sink = pOverLayer->m_waterFluxArray[FL_BOTTOM_SINK] + pOverLayer->m_waterFluxArray[FL_TOP_SINK] + pOverLayer->m_waterFluxArray[FL_SINK] + pOverLayer->m_waterFluxArray[FL_STREAM_SINK];
   
      float AdvectiveFluxLayer1_NO3 = NO3_Rainfall;
      if (((float)pOverLayer->GetExtraStateVarValue(0) + (sink*mobileN1)) > 0.0f)
         AdvectiveFluxLayer1_NO3 = (sink * mobileN1) + NO3_Rainfall;

      /*pOverLayer->AddExtraSvFluxFromGlobalHandler(0, AdvectiveFluxLayer1_NO3);   */      //(kg/d)

      /////////////////////////////////////////Layer 1 NH4 Advective Movement///////////////////////////////////////////
      //float concOver_NH4 = 0.0f;
      //if (pOverLayer->m_volumeWater > 0.0f)
      //   concOver_NO3 = (float)pOverLayer->GetExtraStateVarValue(0) / (float)pOverLayer->m_volumeWater;	//(kg/mmH2O)
      /*concOver_NH4 = (float)pOverLayer->GetExtraStateVarValue(1) / (float)pOverLayer->m_volumeWater;
      float mobile_NH4 = concOver_NH4;*/
      float NH4_Rainfall = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0.0001f;       //kg/m3 or 1000 mg/l
      float AdvectiveFluxLayer1_NH4 = NH4_Rainfall;
               
      //if (pReach)
      //{
      //   ReachSubnode *subnode = pReach->GetReachSubnode(0);
      //   subnode->AddExtraSvFluxFromGlobalHandler(0, pOverLayer->m_waterFluxArray[FL_STREAM_SINK] * concOver_OrgN * -1.0f);			//(kg/d)  
      //}     
       
      ////////////////////////////////////////LAYER 2 Advective Movement////////////////////////////////////////////
      float NO3_Upper = (float)pUpperLayer->GetExtraStateVarValue(0);				               //(kg)
            											
      float concUpper_NO3 = 0.0f;
      float mobileN2 = 0.0f;
      
      if (pUpperLayer->m_volumeWater > 0.0f)
         {
         concUpper_NO3 = (float)pUpperLayer->GetExtraStateVarValue(0) / (float)pUpperLayer->m_volumeWater; //(kg/m3)         
         sink = 0.0f;
         if (NO3_Upper  > 0.0f)
            sink=pUpperLayer->m_waterFluxArray[FL_BOTTOM_SINK] + pUpperLayer->m_waterFluxArray[FL_TOP_SINK] + pUpperLayer->m_waterFluxArray[FL_SINK] + pUpperLayer->m_waterFluxArray[FL_STREAM_SINK];
         if (concUpper_NO3 > 0.0f)
            mobileN2 = concUpper_NO3;
         float AdvectiveFluxLayer2_NO3 = (sink * mobileN2);
         if ((pUpperLayer->m_waterFluxArray[FL_TOP_SOURCE] * mobileN1) > 0.0f)
            AdvectiveFluxLayer2_NO3 = (pUpperLayer->m_waterFluxArray[FL_TOP_SOURCE] * mobileN1) + (sink * mobileN2);
         if (((float)pUpperLayer->GetExtraStateVarValue(0) - (sink * mobileN2)) < 0.0f)
            AdvectiveFluxLayer2_NO3 = NO3_Upper*-1.0f;
         pUpperLayer->AddExtraSvFluxFromGlobalHandler(0, AdvectiveFluxLayer2_NO3);					//(kg/d)
         if (pReach)
            {
            ReachSubnode *subnode = pReach->GetReachSubnode(0);
            subnode->AddExtraSvFluxFromGlobalHandler(0, pUpperLayer->m_waterFluxArray[FL_STREAM_SINK] * mobileN2 * -1.0f);			//(kg/d)  
            }
         }

      
      ////////////////////////////////////////LAYER 3 Advective Movement////////////////////////////////////////////
      float NO3_Lower = (float)pLowerLayer->GetExtraStateVarValue(0);				               //(kg)
      								
      float concLower_NO3 = 0.0f;
      if (pLowerLayer->m_volumeWater > 0.0f)
         concLower_NO3 = (float)pLowerLayer->GetExtraStateVarValue(0) / (float)pLowerLayer->m_volumeWater;
      if (concUpper_NO3 > 0.0f)
         mobileN2 = concUpper_NO3;
      sink = 0.0f;
      if (NO3_Lower > 0.0f) 
         sink = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SINK] + pLowerLayer->m_waterFluxArray[FL_TOP_SINK] + pLowerLayer->m_waterFluxArray[FL_SINK] + pLowerLayer->m_waterFluxArray[FL_STREAM_SINK];
            	
      float AdvectiveFluxLayer3_NO3 = (sink * concLower_NO3);
      if ((pLowerLayer->m_waterFluxArray[FL_TOP_SOURCE] * mobileN2) > 0.0f)
         AdvectiveFluxLayer3_NO3 = (pLowerLayer->m_waterFluxArray[FL_TOP_SOURCE] * mobileN2) + (sink * concLower_NO3);

      pLowerLayer->AddExtraSvFluxFromGlobalHandler(0, AdvectiveFluxLayer3_NO3);	//(kg/d)
      if (pReach)
         {
         ReachSubnode *subnode = pReach->GetReachSubnode(0);
         subnode->AddExtraSvFluxFromGlobalHandler(0, pLowerLayer->m_waterFluxArray[FL_STREAM_SINK] * concLower_NO3 * -1.0f);			//(kg/d) 
         } 

      
	 /////////////////////////////////////////////////////////////////////////////
	 ///////////////////////////////PLANT GROWTH//////////////////////////////////
	 /////////////////////////////////////////////////////////////////////////////

      MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;
      float fr_phu_av = 0.0f; float fr_phu = 0.0f;
      for (int idu = 0; idu < pHRU->m_polyIndexArray.GetSize(); idu++)
         {
         float area = 0.0f;
         int index = pHRU->m_polyIndexArray[idu];
         pIDULayer->GetData(index, m_colArea, area);
         pIDULayer->GetData(pHRU->m_polyIndexArray[idu], m_colFracCGDD, fr_phu);//get fraction of CGDD from this idu		 
         fr_phu_av += fr_phu * area;
         }
      fr_phu_av /= pHRU->m_area;
      fr_phu = fr_phu_av;
	  	  
	   int LULC_A;
	   pFlowContext->pFlowModel->GetHRUData(pHRU, m_colLULC_A, LULC_A, DAM_MAJORITYAREA);
               
	  	
            
		//////////////////////////////////////////////////////////////////////////////////////////
      ///////////////////////NITROGEN FERTILIZER APPLICATION////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////////////////////

      //NITROGEN FERTILILZER APPLICATION 
      float appRate = 0.0f;
      appRate = NitrogenApplicationRate(time, pOverLayer, pFlowContext, 0);

      //CROP & NUTRIENT MANAGEMENT
      float N_fertilizer_application = 0.0f;									
      if (LULC_A == 2)
         N_fertilizer_application = appRate;                           //application rates defined in OSU extension guide
      float NH4_fertilizer = 0.85f*N_fertilizer_application;
      float NO3_fertilizer = 0.15f*N_fertilizer_application;
      float organic_N_fertilizer = 0.0f;
      if (time == 305)
         organic_N_fertilizer = 0.001f * pHRU->m_biomass;               //0.1% of biomass converted to OrgN on DOY 305. Default in SWAT is 100% harvest efficiency, meaning 0% biomass to OrgN; however the option to leave residue exists (Neitsch 2011) 
      
      /////////////////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////PLANT NITRATE UPTAKE////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////////

      //PLANT GROWTH & NITROGEN UPTAKE
      //From SWAT literature: Simplified version of the EPIC plant growth model. Plant growth based on daily accumulated heat units, potential biomass based on a method developed by Monteith, 
      //harvest index used to calculate yield, and plant growth can be inhibited by temperature, water, nitrogen, or phosphorus stress. 
      //No detailed root growth, micronutrient cycling, or toxicity responses included. Nirogen fixation?			  

      float biomass = 0.0f;                                             //kg/ha
      float delta_biomass = 0.0f;                                       //change in biomass
      float fr_n1 = 0.0f;																//normal fraction of nitrogen in the plant biomass at emergence
      float fr_n2 = 0.0f;																//normal fraction of nitrogen in the plant biomass at 50% growth
      float fr_n3 = 0.0f;																//normal fraction of nitrogen in the plant biomass at maturity

      float NO3_PlantUptake = 0.0f;
      float LAI = 0.0f;
      float HU = 0.0f;
      float biomass_N_optimal = 0.0f;
      if (LULC_A == 2)
      {
         float kl = 0.65f;			 												   //light extinction coefficient
         //leaf area index
         float RUE = 0.0f;															   //radiation use efficiency
         float Tbase = 0.0f;															//plant's minimum temp for growth (degrees C), set to 5.0, but this should reference table value	   
         VData CropType = 0;

         float PHU = 0.0f;															   //total heat units required for plant maturity (heat units)	   

         pFlowContext->pFlowModel->GetHRUData(pHRU, pCropNUptakeTable->m_iduCol, CropType, DAM_MAJORITYAREA);  // get soil for this particular HRU
         if (pCropNUptakeTable->m_type == DOT_FLOAT)
            CropType.ChangeType(TYPE_FLOAT);

         bool ok = pCropNUptakeTable->Lookup(CropType, m_col_PHU, PHU);
         ok = pCropNUptakeTable->Lookup(CropType, m_colTbase, Tbase);
         ok = pCropNUptakeTable->Lookup(CropType, m_col_fr_n1, fr_n1);
         ok = pCropNUptakeTable->Lookup(CropType, m_col_fr_n2, fr_n2);
         ok = pCropNUptakeTable->Lookup(CropType, m_col_fr_n3, fr_n3);
         ok = pCropNUptakeTable->Lookup(CropType, m_colRUE, RUE);			//get radiation use efficiency
         ok = pCropNUptakeTable->Lookup(CropType, m_colLAI, LAI);			//get leaf area index

         float H_photosynth = 0.5f*H_day*(1.f - exp(-kl*LAI));//amount of photsynthetically active radiation intercepted on a given day (MJ/m^2 d)
         float delta_biomass = RUE*H_photosynth;								//potential increase in total plant biomass (kg/ha), assumes that the photosynthetic rate of a canopy is a linear function of radiant energy
         if (time == 305)
            {
            delta_biomass = -0.8f * pHRU->m_biomass;
            m_hruBiomassNArray[h] = 0.2f * m_hruBiomassNArray[h];
            }
         biomass = delta_biomass + pHRU->m_biomass;							//total plant biomass on a given day (kg/ha)
         pHRU->m_biomass = biomass;													//biomass from the day before

         HU = max(((Tmax + Tmin) / 2 - Tbase), 0);								//number of heat units accumulated on a given day when Tav > Tbase

         /*SWAT recommended Heat Unit Fractions (fraction of PHU)
         0.15 planting, 1.0 harvest/kill for crops with no dry-down, 1.2 harvest/kill for crops with dry-down	*/

         /*fraction of potential heat units accumulated for the plant on a given day in the growing season*/

         float fr_phu50 = 0.5f;														   //fraction of potential heat units accumulated at 50% maturity (SWAT default = 0.5)
         float fr_phu100 = 1.0f;														   //fraction of potential heat units accumulated for the plant at maturity (SWAT default = 1.0)		   
         //float n2 = (1.0f / (fr_phu100 - fr_phu50))*(log(fr_phu50 / (1.0f - ((fr_n2 - fr_n3) / (fr_n1 - fr_n3))) - fr_phu50) - log(fr_phu100 / (1.0f - ((0.00001f) / (fr_n1 - fr_n3))) - fr_phu100)); //shape coefficient
         //float n1 = log(fr_phu50 / (1 - ((fr_n2 - fr_n3) / (fr_n1 - fr_n3))) - fr_phu50) + n2 + fr_phu50; //shape coefficient
         float pa = (0.5f / (1.0f - ((fr_n2 - fr_n3) / (fr_n1 - fr_n3)))) - 0.5f;
         float p1 = log(pa);
         float pb = 1.0f / (1.0f - 0.00001f / (fr_n1 - fr_n3)) - 1.0f;
         float p2 = log(pb);
         //float n2 = (1.0f / (1.0f - 0.5f))*(log(0.5f / (1.0f - ((fr_n2 - fr_n3) / (fr_n1 - fr_n3)) - 0.5f)) - log(1.0f / (1.0f - ((0.00001f) / (fr_n1 - fr_n3)) - 1.0f))); //shape coefficient
         float n2 = (p1 - p2) / 0.5f;
         float n1 = p1 + n2 * 0.5f; //shape coefficient	
         float fr_N = (fr_n1 - fr_n3)*(1.0f - (fr_phu / (fr_phu + exp(n1 - n2*fr_phu)))) + fr_n3; //fraction of nitrogen in the plant biomass on a given day		   
         /*float biomass_N = fr_N*biomass;	*/										//actual mass of nitrogen stored in plant material (kg/ha)
         						               //optimal mass of nitrogen stored in plant material (kg/ha)
         if (delta_biomass > 0.0f)
            biomass_N_optimal = fr_N*biomass;
         float m3 = biomass_N_optimal - m_hruBiomassNArray[h];
         float m4 = 0.0f;
         if (delta_biomass > 0.0f)
            m4 = 4.f*fr_n3*delta_biomass;
         float N_up = min(m3, m4);			
         if (N_up < 0.0f)//potential nitrogen uptake (SWAT uses min(m3,m4), but I changed to max because it's been underpredicting)
            N_up = 0.0f;
         float Bn = 10.0f;															      //nitrogen uptake distribution parameter (SWAT shows effect of Bn values from 1 to 15, with 15 being the highest uptake %)
         NO3_PlantUptake = min(N_up, NO3/area_hru);										   //called N_actual in Neitsche et al 2011
        // NO3_PlantUptake = N_up;
         m_hruBiomassNArray[h] += NO3_PlantUptake;

         }

      //////////////////////////////////////////////////////////////////////////////////////////
      /////////////////////////////SOIL TEMP////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////

      //SOIL TEMP
      float epsilon_sr = (H_day*(1.0f - albedo) - 14.0f) / 20.0f;							   //radiation term for bare soil surface temperature calculation
      float Tbare = Tav + epsilon_sr*((Tmax - Tmin) / 2.0f);								   //temperature of soil surface with no cover, 
      float CV = biomass;																		         //Total aboveground biomass and residue present on current day (kg/ha)
      /*float SNO = 1.0f;*/																	         //water content of snow cover on current day (mmH20)
      float l = 0.8f;																		            //lag coefficient (SWAT standard is 0.8)
      float m1 = CV / (CV + exp(7.563f - (1.0f / 297.0f * 0.0001f*CV)));
      /*float m2 = (SNO) / (SNO + pow(euler_e, (6.055f - 0.3002f*SNO)));*/
      float bcv = m1;																                  //soil temperature weighting factor for impact of groundcover on soil surface temperature, SWAT takes maximum of m1 and m2, but since snow is not included in model we're just using m1 
      float Tssurf = bcv*pHRU->m_soilTemp + (1.0f - bcv)*Tbare;								//soil surface temperature (C)
      float T_soil = l*pHRU->m_soilTemp + (1.0f - l)*(df*(TAAir - Tssurf) + Tssurf);	//temperature of layer (C)	  
      pHRU->m_soilTemp = T_soil;															            //soil temperature from the day before

      //////////////////////////////////////////////////////////////////////////////////////////
      ///////////////////////TRANSFORMATIONS////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////

      //MINERALIZATION					
      float y_sw = SoilWater / field_capacity;									//nutrient cycling water factor           
      float y_tmp = 0.9f*(T_soil / (T_soil + exp(9.93f - 0.312f*T_soil))) + 0.1f;	//nutrient cycling temperature factor
      float B_min = 0.0008f;														   // rate coefficient of mineralization SWAT default = 0.0003
		float mineralization = B_min*(pow((y_sw*y_tmp), 0.5f))*OrgN;		//net mineralization from OrgN pool to NO3 (kg/ha)*area(ha)=(kg)

      //NITRIFICATION & VOLATILIZATION
      float n_cec = 0.15f;															   //volatilization cation exchange factor, SWAT default=0.15
      float n_midz = 1.f - (z_mid / (z_mid + exp(4.706f - 0.0305f*z_mid)));//volatilization factor 
      float n_sw = 1.0f;
      if (SoilWater<(0.25f*field_capacity - 0.75f*WP))
         n_sw = (SoilWater - WP) / (0.25f*(field_capacity - WP));					//nitrification soil water factor 
      float n_tmp = 0.0f;
      if (T_soil>5.0f)
         n_tmp = 0.41f*((T_soil - 5.0f) / 10.0f);								//nitrification/volatilization temperature factor 
      float n_nit = n_tmp*n_sw;													   //nitrification regulator
      float n_vol = n_tmp*n_midz*n_cec;											//volatilization regulator
      float N_nit_vol = NH4/area_hru*(1.f - exp(-n_nit - n_vol));		   //amount of ammonium converted via nitrification and volatilization
      float fr_nit = 1.f - exp(- n_nit);							            //estimated fraction of nitrogen lost by nitrification
      float fr_vol = 1.f - exp(- n_vol);							            //estimated fraction of nitrogen lost by volatilization
      float nitrification = 0.0f;													//flow from NH4 to NO3 (kg)
      if (fr_nit + fr_vol > 0.0f)
			nitrification = (fr_nit / (fr_nit + fr_vol))*N_nit_vol;
      float volatilization = 0.0f;													//flow from NH4 out of the system (kg)
      if (fr_nit + fr_vol > 0.0f)
         volatilization = (fr_vol / fr_nit + fr_vol)*N_nit_vol;

      //DENITRIFCATION 
      float B_denit = 1.4f;														   //rate coefficient of denitrification
		float y_sw_thr = 0.28f;														   //threshold value of nutrient cycling water factor for denitrification to occur, SWAT Default = 1.1
      float N_denit = 0.0f;                                             //denitrification calculation, flow from NO3 out of the system (kg/ha)*area(ha)=(kg)
      if ((float)pOverLayer->GetExtraStateVarValue(0) > 0.0f)
         N_denit = ((float)pOverLayer->GetExtraStateVarValue(0))*(1.f - exp(-B_denit*y_tmp*orgC));
      float denitrification = 0.0f;												   
      if ((y_sw > y_sw_thr) && (((float)pOverLayer->GetExtraStateVarValue(0) - N_denit) > 0.0f)) //denitrification only occurs if soil water content exceeds threshold, y_sw_thr
         denitrification = N_denit;
      
      /////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////NO3 SOIL FLUX//////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////////  
			 
      float NO3_transformation = (mineralization - NO3_PlantUptake - denitrification + NO3_fertilizer + nitrification)*area_hru;	//Nitrate transformation only take place in the surface layer for now
      float Combined = (float)pOverLayer->GetExtraStateVarValue(0) * -1.0f;
      if ((float)pOverLayer->GetExtraStateVarValue(0) + (AdvectiveFluxLayer1_NO3 + NO3_transformation) > 0.0f)
         Combined = AdvectiveFluxLayer1_NO3 + NO3_transformation;

      pOverLayer->AddExtraSvFluxFromGlobalHandler(0, Combined);         //(kg/d)
      
      float NO3_to_Stream = (pOverLayer->m_waterFluxArray[FL_STREAM_SINK] * mobileN1) + (pUpperLayer->m_waterFluxArray[FL_STREAM_SINK] * concUpper_NO3) + (pLowerLayer->m_waterFluxArray[FL_STREAM_SINK] * concLower_NO3);//combined loss of NO3 to streams

      /////////////////////////////////////////////////////////////////////////////////////////////
      /////////////////////////OrgN ENRICHED SEDIMENT TRANSPORT////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////////
       
      float sed = pHRU->m_currentSediment;                                 //daily sediment yield (metric tons), 1697 lb * 0.00045359237 lb/metric tons = 0.77metric tons; 0.77tons/365 days=0.002 tons/day       
      float Conc_sed_surf = sed / (10 * area_hru*pOverLayer->m_waterFluxArray[FL_STREAM_SINK]); //(kg/ha)
      float epsilon_nsed = 0.0f;                                           //nitrogen enrichment ratio for nitrogen transported with sediment, Conc_sed_surf from MUSLE
      if (Conc_sed_surf>0.0f)
         epsilon_nsed = 0.78f*pow(Conc_sed_surf, -0.2468f);					
      float Conc_OrgN = 100.0f * (OrgN) / (depth_surf*bulk_density);			//concentration of organic nitrogen in the top 10mm of the the soil (mg/kg)
      
      float AdvectiveFluxLayer1_OrgN = organic_N_fertilizer - (0.001f*Conc_OrgN*(sed / area_hru)*epsilon_nsed);//amount of organic nitrogen transported to the main channel in surface runoff (kg/ha)
      
      pOverLayer->AddExtraSvFluxFromGlobalHandler(2, AdvectiveFluxLayer1_OrgN - (fabs(mineralization)));

      /////////////////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////NH4 SOIL FLUX////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////////
            	
      float NH4_transformation = NH4_fertilizer - nitrification - volatilization;//NH4 transformations limited to the soil box
           
      pOverLayer->AddExtraSvFluxFromGlobalHandler(1, AdvectiveFluxLayer1_NH4 + (NH4_transformation*area_hru));					//(kg/d)

      /////////////////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////////////MODEL OUTPUT/////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////////

      float *data = new float[21];
      data[0] = time;
      data[1] = pHRU->m_area;
      data[2] = (float)pHRU->m_id;
      /*data[3] = Tav;*/
      data[3] = T_soil;
      data[4] = (float)LULC_A;
      data[5] = HU;
      data[6] = SoilWater;
      /*data[8] = LAI;*/
      data[7] = biomass;
      float KGM3_PER_KGHA = pOverLayer->m_volumeWater / (pHRU->m_area*(float)M2_PER_HA);
      data[8] = (NH4_fertilizer + NO3_fertilizer);
      data[9] = nitrification /*/ area_hru*KGM3_PER_KGHA*/;
      data[10] = mineralization /*/ area_hru/**KGM3_PER_KGHA*/;
      data[11] = NO3_PlantUptake /*/ area_hru/**KGM3_PER_KGHA*/;
      data[12] = denitrification /*/ area_hru/**KGM3_PER_KGHA*/;
      data[13] = volatilization /*/ area_hru/**KGM3_PER_KGHA*/;
      data[14] = fabs(NO3_to_Stream)*KGM3_PER_KGHA;
      data[15] = pFlowContext->pEnvContext->currentYear;
      data[16] = tim;
      data[17] = NO3_Rainfall *KGM3_PER_KGHA;
      /*data[20] = m_hruBiomassNArray[h] ;*/ 
      /*data[21] = biomass_N_optimal ;*/
      data[18] = NO3 / area_hru;//kg/ha
      data[19] = NH4 / area_hru;//kg/ha
      data[20] = OrgN / area_hru;//kg/ha

      m_pRateData->AppendRow(data, 21);

      delete[] data;

      }//End of HRUs
   return -1.0f;
   }
     //PLANT GROWTH & NITROGEN UPTAKE
     //From SWAT literature: Simplified version of the EPIC plant growth model. Plant growth based on daily accumulated heat units, potential biomass based on a method developed by Monteith, 
     //harvest index used to calculate yield, and plant growth can be inhibited by temperature, water, nitrogen, or phosphorus stress. 
     //No detailed root growth, micronutrient cycling, or toxicity responses included. Nirogen fixation?			  
     //float LAI = 0.0f;															//leaf area index, should reference a table
     //float kt = 0.0f;															   //light extinction coefficient
     //float H_photosynth = 0.5*H_day*(1 - exp(-kt*LAI));				//amount of photsynthetically active radiation intercepted on a given day (MJ/m^2 d)
     //float RUE = 0.0f;															//radiation use efficiency
     //float Tbase = 0.0f;															//plant's minimum temp for growth (degrees C), set to 5.0, but this should reference table value
     //float PHU = pull up a table, lookup crop specific values;		//total heat units required for plant maturity (heat units)
     //float HU = 0.0f;															   //number of heat units accumulated on a given day when Tav > Tbase

     //  while (HU <= PHU)
     //  {

     //CalculateCGDD( pFlowContext, pHRU, Tbase, doy, false, HU );	 
     //float fr_phu = HU / PHU;	

     //  }
     //SWAT recommended Heat Unit Fractions (fraction of PHU)
     //0.15 planting, 1.0 harvest/kill for crops with no dry-down, 1.2 harvest/kill for crops with dry-down			   

     //fraction of potential heat units accumulated for the plant on a given day in the growing season
     //float fr_phu50 = 0.5f;														//fraction of potential heat units accumulated at 50% maturity (SWAT default = 0.5)
     //float fr_phu100 = 1.0f;														//fraction of potential heat units accumulated for the plant at maturity (SWAT default = 1.0)
     //float fr_n1 = ;															//normal fraction of nitrogen in the plant biomass at emergence
     //float fr_n2 = ;															//normal fraction of nitrogen in the plant biomass at 50% growth
     //float fr_n3 = ;															//normal fraction of nitrogen in the plant biomass at maturity
     //float n2 = (1 / (fr_phu100 - fr_phu50))*(log(fr_phu50 / (1 - ((fr_n2 - fr_n3) / (fr_n1 - fr_n3))) - fr_phu50) - log(fr_phu100 / (1 - ((0.00001) / (fr_n1 - fr_n3))) - fr_phu100)); //shape coefficient
     //float n1 = log(fr_phu50/(1-((fr_n2-fr_n3)/(fr_n1-fr_n3)))-fr_phu50)+n2+fr_phu50; //shape coefficient			  			  
     //float fr_N = (fr_n1 - fr_n3)*(1 - (fr_phu / (fr_phu + exp(n1 - n2*fr_phu)))) + fr_n3; //fraction of nitrogen in the plant biomass on a given day
     //float delta_biomass = RUE*H_photosynth;										//potential increase in total plant biomass (kg/ha), assumes that the photosynthetic rate of a canopy is a linear function of radiant energy
     ////float biomass = sum(delta_biomass);										//total plant biomass on a given day (kg/ha)
     //float biomass_N = fr_N*biomass;												//actual mass of nitrogen stored in plant material (kg/ha)
     //float biomass_N_optimal = fr_N*biomass;										//optimal mass of nitrogen stored in plant material (kg/ha)  
     //float m3 = biomass_N_optimal-biomass_N;
     //float m4 = 4.f*fr_n3*delta_biomass;
     //float N_up = min(m3, m4);													//potential nitrogen uptake ------->I think it'll be fine to just use this from the top layer as opposed to do a depth distribution
     //float Bn = 10.0f;															//nitrogen uptake distribution parameter (SWAT shows effect of Bn values from 1 to 15, with 15 being the highest uptake %)
     //float z_root = 0.0f;														//depth of root development in the soil, probably okay to use an average for each crop?
     //if (layer == 0)
     //z_root= z_bottom;
     //float N_up_z = (N_up / (1 - exp(-Bn)))*(1 - exp(-Bn*z / z_root));			//potential nitrogen uptake from the soil surface to depth z
     //float NO3_PlantUptake = min(N_up, NO3);										//called N_actual in Neitsche et al 2011

        //float koc = 0.01;
        //float ks = 0.1f;



   //      VData soilType = 0;

    //     pFlowContext->pFlowModel->GetHRUData(pHRU, pSoilPropertiesTable->m_iduCol, soilType, DAM_FIRST);  // get soil for this particular HRU
    //     if (pSoilPropertiesTable->m_type == DOT_FLOAT)
    //        soilType.ChangeType(TYPE_FLOAT);
         //float om = 10.0f;
         //float bd = 1.0f;
        // bool ok = pSoilPropertiesTable->Lookup(soilType, m_col_om, om);// get the organic matter content
        // ok = pSoilPropertiesTable->Lookup(soilType, m_col_bd, bd);// get the organic matter content

         //Begin Soil Rate Calculations
         
		 /*float ss = 0.0f;
         float uptakeEfficiency = 0.5f;
         float a1 = pOverLayer->m_waterFluxArray[FL_TOP_SINK] * concOver * uptakeEfficiency;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         float appRate = 0.0f;
         if (k<2)
            appRate = ApplicationRate(time, pOverLayer, pFlowContext, k);
         float b1 = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         b1 += appRate;
         float c1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         float d1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SINK] * 0; //down flux across lower surface of this layer (a loss)
         float e111 = pOverLayer->m_waterFluxArray[FL_STREAM_SINK] * concOver; //loss to streams

         float advectiveFluxOver = a1 + b1 + c1 + d1 + e111;


         float kd = koc*om / 1000.0f; //ml/g
         float transformation = (ks*pOverLayer->GetExtraStateVarValue(k)) + (concOver*kd*ks*bd);//degraded of absorbed

         ss = advectiveFluxOver - transformation; //need to reflect calculations for state variable
         pOverLayer->AddExtraSvFluxFromGlobalHandler(k, ss);
         // Finish Soil Rate Calculations*/

         // Begin Upper Box  Rate Calculations
	  
         /*a1 = pUpperLayer->m_waterFluxArray[FL_TOP_SINK] * concUpper;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         b1 = pUpperLayer->m_waterFluxArray[FL_TOP_SOURCE] * concOver;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         c1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * concLower;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SINK] * concUpper; //down flux across lower surface of this layer (a loss)
         float e11 = pUpperLayer->m_waterFluxArray[FL_STREAM_SINK] * concUpper; //loss to streams

         advectiveFluxOver = a1 + b1 + c1 + d1 + e11;

         transformation = (ks*pUpperLayer->GetExtraStateVarValue(k)) + (concUpper*kd*ks*bd);//degraded of absorbed

         ss = advectiveFluxOver - transformation;
         pUpperLayer->AddExtraSvFluxFromGlobalHandler(k, ss);
		 */
         // Finish Upper Box Rate Calculations

         // Begin Lower Box Rate Calculations

         //transformation = (ks*pLowerLayer->GetExtraStateVarValue(k)) + (concLower*kd*ks*bd);//degraded of absorbed
         //ss = advectiveFluxOver - transformation;
         //pLowerLayer->AddExtraSvFluxFromGlobalHandler(k, ss);     //m3/d

         // End Lower Soil Box Rate Calculations



		 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		 /*

         float a1 = pOverLayer->m_waterFluxArray[FL_TOP_SINK] * concOver;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         float b1 = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         float c1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         float d1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SINK] * 0; //down flux across lower surface of this layer (a loss)
         float e111 = pOverLayer->m_waterFluxArray[FL_STREAM_SINK] * concOver; //loss to streams

         ss = a1 + b1 + c1 + d1 + e111;

         pOverLayer->AddExtraSvFluxFromGlobalHandler(k, ss);

         a1 = pUpperLayer->m_waterFluxArray[FL_TOP_SINK] * concUpper;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         b1 = pUpperLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         c1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * concLower;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         d1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SINK] * concUpper; //down flux across lower surface of this layer (a loss)
         float e11 = pUpperLayer->m_waterFluxArray[FL_STREAM_SINK] * concUpper; //loss to streams

         ss = a1 + b1 + c1 + d1 + e11;

         pUpperLayer->AddExtraSvFluxFromGlobalHandler(k, ss);

         float e1 = pLowerLayer->m_waterFluxArray[FL_TOP_SINK] * concLower;//upward flux across upper surface of this layer (a loss) -saturated to unsaturated zone
         float f1 = pLowerLayer->m_waterFluxArray[FL_TOP_SOURCE] * concUpper;//down flux across upper surface of this layer (a gain) - gwREcharge
         float g1 = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturate zone
         float h1 = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SINK] * concLower; //down flux across lower surface of this layer (a loss)
         float i1 = 0.0f; // outgoing flow to horizontal neighbors
         float j1 = 0.0f; // incoming flow from horizontal neighbors

         ss = e1 + f1 + g1 + h1 + i1 + j1;
         ss = 0.0f;

         pLowerLayer->AddExtraSvFluxFromGlobalHandler(k, ss);     //m3/d
         Reach * pReach = pUpperLayer->GetReach();
         if (pReach)
            {
            ReachSubnode *subnode = pReach->GetReachSubnode(0);
            subnode->AddExtraSvFluxFromGlobalHandler(k, ((-e11 - e111))); //kg/d.  e1 is stored as a loss from the unsaturated zone - so it is negative and needs to be converted to a + value.
            }*/









   //////////////////APP RATE BEFORE MODIFICATIONS ON 2/10/15//////////////////////////

   /**\













   //////////////////////////////////////////////////////////////////////////////////////////////////////////////

   //////////////////COMMENTED SECTION BELOW SAVED FROM 1/23/15 FOR FUTURE REVERSION, JUST IN CASE///////////////

   //////////////////////////////////////////////////////////////////////////////////////////////////////////////

   //float Flow_Solute::HBV_N_Fluxes(FlowContext *pFlowContext)
   //{
	  // if (pFlowContext->timing & 1) // Init()
		 //  return Flow_Solute::Init_Solute_Fluxes(pFlowContext);

	  // int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
	  // int hruCount = pFlowContext->pFlowModel->GetHRUCount();


	  // float time = pFlowContext->time;
	  // time = fmod(time, 365);
	  // ParamTable *pSoilPropertiesTable = pFlowContext->pFlowModel->GetTable("SoilData");
	  // for (int h = 0; h < hruCount; h++) //for each HRU
	  // {
		 //  HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
		 //  HRULayer *pOverLayer = pHRU->GetLayer(0);
		 //  HRULayer *pUpperLayer = pHRU->GetLayer(1);
		 //  HRULayer *pLowerLayer = pHRU->GetLayer(2);
		 //  int hruLayerCount = pHRU->GetLayerCount();
		 //  int svC = pFlowContext->svCount;


		 //  for (int k = 0; k<svC; k++)   //for each N box
		 //  {

			//   StateVar *pSV = pFlowContext->pFlowModel->GetStateVar(k + 1);
			//   float concOver = 0.0f;
			//   if (pOverLayer->m_volumeWater > 0.0f)
			//	   concOver = (float)pOverLayer->GetExtraStateVarValue(k) / (float)pOverLayer->m_volumeWater;

			//   float concUpper = 0.0f;
			//   if (pUpperLayer->m_volumeWater > 0.0f)
			//	   concUpper = (float)pUpperLayer->GetExtraStateVarValue(k) / (float)pUpperLayer->m_volumeWater;

			//   float concLower = 0.0f;
			//   if (pLowerLayer->m_volumeWater > 0.0f)
			//	   concLower = (float)pLowerLayer->GetExtraStateVarValue(k) / (float)pLowerLayer->m_volumeWater;

			//   for (int layer = 0; layer < pHRU->GetLayerCount(); layer++)
			//   {
			//	   HRULayer *pLayer = pHRU->GetLayer(layer);

			//	   VData val = pSV->m_name.GetString();

			//	   //INORGANIC NITROGEN STATE VARIABLES
			//	   float NO3 = (float)pLayer->GetExtraStateVarValue(0) / (float)pLayer->m_volumeWater;		//(kg/ha)
			//	   float NH4 = (float)pLayer->GetExtraStateVarValue(1) / (float)pLayer->m_volumeWater; //(k g/ha)

			//	   //ORGANIC NITROGEN STATE VARIABLES
			//	   //float OrgN_fresh = (float)pLayer->GetExtraStateVarValue(4) / (float)pLayer->m_volumeWater;		//(kg/ha)
			//	   //float OrgN_active = (float)pLayer->GetExtraStateVarValue(2) / (float)pLayer->m_volumeWater;		//(mg/kg) orgN_humic *fr_actN
			//	   //float OrgN_stable = (float)pLayer->GetExtraStateVarValue(3) / (float)pLayer->m_volumeWater;		//(mg/kg) orgN_humic*(1-fr_actN)
			//	   //float Conc_orgN = 100.0f * (OrgN_stable + OrgN_fresh + OrgN_active) / (depth_surf*bulk_density_surface);		//(mg/kg)

			//	   //INORGANIC PHOSPHORUS STATE VARIABLES
			//	   /*float OrgP_fresh = (float)pLayer->GetExtraStateVarValue(8) / (float)pLayer->m_volumeWater;		//(kg/ha)
			//	   float MinP_active = (float)pLayer->GetExtraStateVarValue(6) / (float)pLayer->m_volumeWater;		//(kg/ha)
			//	   float MinP_stable = (float)pLayer->GetExtraStateVarValue(5) / (float)pLayer->m_volumeWater;		//(kg/ha)
			//	   float PSolution = (float)pLayer->GetExtraStateVarValue(7) / (float)pLayer->m_volumeWater;		//(kg/ha)

			//	   //ORGANIC PHOSPHORUS STATE VARIABLES
			//	   float OrgP_humic = (float)pLayer->GetExtraStateVarValue(9) / (float)pLayer->m_volumeWater;		//(kg/ha) Humic P pool
			//	   float OrgP_active = OrgP_humic*(OrgN_active / (OrgN_active + OrgN_stable)); // error: -1 IND0000
			//	   float OrgP_stable = OrgP_humic*(OrgN_stable / (OrgN_stable + OrgN_active)); //	error: -1 IND0000*/


			//	   //IDU Attributes
			//	   float area_hru = 0.404686f; //(hectares) set to 1 acre for testing

			//	   //SOIL PROPERTIES
			//	   float AWC = 0.22f;       //available water capacity
			//	   float WP = 0.05f;        //wilting point mmH20 (wfifteenbar in SSURGO)
			//	   float albedo = 0.2f;        //source Coakley 2003
			//	   float field_capacity = WP + AWC;
			//	   float SAT = 0.485f;       //saturated water content of the layer, effective porosity (value for silt loam, Dingman 2002)
			//	   float bulk_density = 1.34f; // for dayton silt loam(g/cm^3)
			//	   float bulk_density_surface = bulk_density;//for dayton silt loam(g/cm^3) NEED TO CHANGE THIS SO IT'S ONLY CALCULATED FOR LAYER=0
			//	   float orgC = 1.0f;         //organic carbon content of the layer (%)
			//	   float om = 1.72f*orgC; //organic matter content (%) --OM exists in some SSURGO data, and it may be necessary to work from OM to get OrgC--
			//	   float z = 1.0f;            //depth (hzdept in SSURGO) (mm)
			//	   float z_bottom = 2.0f;     //depth to layer bottom (hzdepb in SSURGO) (mm)
			//	   float z_mid = (z + (z_bottom - z) / 2.0f);	//depth to layer mid (mm)
			//	   float scaling_factor = SW / ((0.356f - 0.144f*bulk_density)*z_bottom); //
			//	   float dd_max = 1000.f + (2500.f * bulk_density) / (bulk_density + 686.f * exp(-5.63f*bulk_density)); //
			//	   float dd = dd_max*exp((log(500.f / dd_max))*((1.f - scaling_factor) / (1.f + scaling_factor)*(1.f + scaling_factor))); //
			//	   float zd = z / dd;           //ratio of depth at layer center to dampening depth, z/dd
			//	   float thickness = 190.0f;        //defined in SWAT as depth --depth of the layer-- (perhaps thickness?) (mm)		
			//	   float depth_surf = 10.0f;          //depth of surface soil layer (10mm)
			//	   float df = zd / (zd + exp(-0.867f - 2.078f*zd));   //depth facotr used in soil calculations
			//	   float Total_Organic_N = 10000.0f * (orgC / 14.0f); //
			//	   float percent_clay = 17.5f; // 
			//	   float percent_silt = 32.5f;
			//	   float percent_sand = 25.0f;
			//	   float percent_rock = 25.0f;
			//	   float theta_e = 0.50f;		//fraction of porosity from which anions are excluded, SWAT default=0.50
			//	   float rsd = 685.8f;		//residue in the layer, kg/ha (ex:685.8 for Dayton silt loam) reference soil properties table

			//	   //HYDROLOGY FROM FLOW 
			//	   float precip = 0.0f; //initialize precip
			//	   float R_day = pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, pFlowContext->dayOfYear, precip);//mm;	//amount of rainfall in a given day (mm H20)
			//	   float SW = pLayer->m_wc;			//layer water content, mmH20
			//	   float w_perc = pLayer->m_waterFluxArray[FL_BOTTOM_SINK];		//amount of water percolating to the underlying soil layer on a given day (SWAT needs these flows to be in mmH20, but I think they're in kg/m^3)
			//	   float Q_lat = pLayer->m_waterFluxArray[FL_STREAM_SINK];		//amount of water going to stream as subsurface runoff on a given day
			//	   float Q_surf = 0.0f;											//amount of water going to stream as surface runoff on a given day; can only come from surface layer
			//	   if (k = 0)
			//		   Q_surf = pLayer->m_waterFluxArray[FL_STREAM_SINK];

			//	   //CLIMATE VARIABLES
			//	   float H_day = 0.0f;	//incident total solar (MJ/m^2)
			//	   float TAAir = 11.6f;                //annual avg air temp (degrees C)
			//	   float T_soil_d_minus_1 = 10.0f;     //soil temperature from the day before		 
			//	   float Tav = 10.0f;                  //air temperature average for day (C)
			//	   float Tmax = 0.0f;		//air temperature maximum for day (C)
			//	   float Tmin = 0.0f;		//air temperature minimum for day (C)
			//	   pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX, pHRU, pFlowContext->dayOfYear, Tmax);//C;                         
			//	   pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, pFlowContext->dayOfYear, Tmin);//C;
			//	   pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, pFlowContext->dayOfYear, H_day)*(0.001); //(MJ/m^2)
			//	   float epsilon_sr = (H_day*(1.0f - albedo) - 14.0f) / 20.0f;		//radiation term for bare soil surface temperature calculation
			//	   float Tbare = Tav + epsilon_sr*((Tmax - Tmin) / 2.0f);			//temperature of soil surface with no cover, 
			//	   float CV = 1.0f;
			//	   float SNO = 1.0f;
			//	   float l = 0.8f;			//lag coefficient (SWAT standard is 0.8)
			//	   float m1 = CV / (CV + exp(7.563f - (1.0f / 297.0f * 0.0001f*CV)));
			//	   float m2 = (SNO) / (SNO + exp(6.055f - 0.3002f*SNO));
			//	   float bcv = max(m1, m2);		//soil temperature weighting factor,	
			//	   float Tssurf = bcv*T_soil_d_minus_1 + (1.0f - bcv)*Tbare;		//soil surface temperature (C)
			//	   float T_soil = l*T_soil_d_minus_1 + (1.0f - l)*(df*(TAAir - Tssurf) + Tssurf);		//temperature of layer (C)
			//	   //float OrgN_humic = pow(10, 4)*(orgC / 14);

			//	   //NUTRIENT CYCLING FACTORS
			//	   //float epsilon_cn = (0.58f*rsd) / (OrgN_fresh + NO3);		//residue C:N ratio in the soil layer
			//	   //float epsilon_cp = (0.58f*rsd) / (OrgP_fresh + PSolution);		//residue C:P ratio in the soil layer
			//	   //float a = exp(-0.693f*(epsilon_cn - 25.f) / 25.f);
			//	   //float b = (exp(-0.693f*(epsilon_cp - 200.f) / 200.f));
			//	   //float y_ntr = 1.0f;
			//	   //if (min(a, b > 1.0f))
			//	   //	y_ntr = min((exp(-0.693f*(epsilon_cn - 25.f) / 25.f)), (exp(-0.693f*(epsilon_cp - 200.f) / 200.f))); //
			//	   float y_sw = SW / field_capacity; //nutrient cycling water factor
			//	   float y_sw_thr = 1.1f;		//threshold value of nutrient cycling water factor for denitrification to occur
			//	   float y_tmp = 0.9f*(T_soil / (T_soil + exp(9.93f - 0.312f*T_soil))) + 0.1f;			//nutrient cycling temperature factor,
			//	   float B_min = 0.0003f;		// rate coefficient of mineralization 
			//	   float B_rsd = 0.05f;		// rate coefficient for mineralization of residue fresh organic nutrients
			//	   //float delta_ntr = B_rsd*y_ntr*pow((y_sw*y_tmp), 0.5);		//residue decay rate constant

			//	   //DORMANCY
			//	   float t_dorm = 1.0; //dormancy threshold used to compare actual daylength to minimum daylength if latitude > 40 degrees. If 20 to 40 degrees, t_dorm= (latitude-20)/20. If lat < 20, t_dorm=0.0

			//	   //PLANT GROWTH & NUTRIENT UPTAKE
			//	   //From SWAT literature: Simplified version of the EPIC plant growth model. Plant growth based on daily accumulated heat units, potential biomass based on a method developed by Monteith, 
			//	   //harvest index used to calculate yield, and plant growth can be inhibited by temperature, water, nitrogen, or phosphorus stress. 
			//	   //No detailed root growth, micronutrient cycling, or toxicity responses included. Nirogen fixation?			  
			//	   float LAI = 0.0f; //leaf area index, should reference a table
			//	   float kt = 0.0f; //light extinction coefficient
			//	   float H_photosynth = 0.5*H_day*(1 - exp(-kt*LAI)); //amount of photsynthetically active radiation intercepted on a given day (MJ/m^2 d)
			//	   float RUE = 0.0f; //radiation use efficiency
			//	   float Tbase = 0.0f; //plant's minimum temp for growth (degrees C), set to 5.0, but this should reference table value
			//	   float HU = 0.0f;
			//	   if (Tav > Tbase)
			//		   HU = (Tav - Tbase); //number of heat units accumulated on a given day when Tav > Tbase
			//	   //float PHU = sum(HU); //total heat units required for plant maturity (heat units)
			//	   //SWAT recommended Heat Unit Fractions (fraction of PHU)
			//	   //0.15 planting
			//	   //1.0 harvest/kill for crops with no dry-down
			//	   //1.2 harvest/kill for crops with dry-down

			//	   //NITROGEN UPTAKE 			  
			//	   //float fr_phu = 0.0f; //fraction of potential heat units accumulated for the plant on a given day in the growing season
			//	   //float fr_phu50 = 0.5f; //fraction of potential heat units accumulated at 50% maturity (SWAT default = 0.5)
			//	   //float fr_phu100 = 1.0f; //fraction of potential heat units accumulated for the plant at maturity (SWAT default = 1.0)
			//	   //float fr_n1 = 0.0f; //normal fraction of nitrogen in the plant biomass at emergence
			//	   //float fr_n2 = 0.0f; //normal fraction of nitrogen in the plant biomass at 50% growth
			//	   //float fr_n3 = 0.0f; //normal fraction of nitrogen in the plant biomass at maturity
			//	   //float n2 = (1 / (fr_phu100 - fr_phu50))*(log(fr_phu50 / (1 - ((fr_n2 - fr_n3) / (fr_n1 - fr_n3))) - fr_phu50) - log(fr_phu100 / (1 - ((0.00001) / (fr_n1 - fr_n3))) - fr_phu100)); //shape coefficient
			//	   //float n1 = log(fr_phu50/(1-((fr_n2-fr_n3)/(fr_n1-fr_n3)))-fr_phu50)+n2+fr_phu50; //shape coefficient			  			  
			//	   //float fr_N = (fr_n1 - fr_n3)*(1 - (fr_phu / (fr_phu + exp(n1 - n2*fr_phu)))) + fr_n3; //fraction of nitrogen in the plant biomass on a given day
			//	   //float delta_biomass = RUE*H_photosynth; //potential increase in total plant biomass (kg/ha), assumes that the photosynthetic rate of a canopy is a linear function of radiant energy
			//	   ////float biomass = sum(delta_biomass); //total plant biomass on a given day (kg/ha)
			//	   //float biomass_N = fr_N*biomass; //actual mass of nitrogen stored in plant material (kg/ha)
			//	   //float biomass_N_optimal = fr_N*biomass; //optimal mass of nitrogen stored in plant material (kg/ha)  
			//	   //float m3 = biomass_N_optimal-biomass_N;
			//	   //float m4 = 4.f*fr_n3*delta_biomass;
			//	   //float N_up = min(m3, m4);		//potential nitrogen uptake ------->I think it'll be fine to just use this from the top layer as opposed to do a depth distribution
			//	   //float Bn = 10.0f; //nitrogen uptake distribution parameter (SWAT shows effect of Bn values from 1 to 15, with 15 being the highest uptake %)
			//	   //float z_root = 0.0f; //depth of root development in the soil, probably okay to use an average for each crop?
			//	   //float N_up_z = (N_up / (1 - exp(-Bn)))*(1 - exp(-Bn*z / z_root)); //potential nitrogen uptake from the soil surface to depth z
			//	   //float NO3_PlantUptake = min(N_up, NO3); //called N_actual in Neitsche et al 2011

			//	   //PHOSPHORUS UPTAKE
			//	   //float fr_p1 = 0.0f; //normal fraction of phosphorus in the plant biomass at emergence
			//	   //float fr_p2 = 0.0f; //normal fraction of phosphorus in the plant biomass at 50% growth
			//	   //float fr_p3 = 0.0f; //normal fraction of phosphorus in the plant biomass at maturity
			//	   //float p2 = (1.f / (fr_phu100 - fr_phu50))*(log(fr_phu50 / (1 - ((fr_p2 - fr_p3) / (fr_p1 - fr_p3))) - fr_phu50) - log(fr_phu100 / (1.f - ((0.00001f) / (fr_p1 - fr_p3))) - fr_phu100)); //shape coefficient
			//	   //float p1 = log(fr_phu50 / (1 - ((fr_p2 - fr_p3) / (fr_p1 - fr_p3))) - fr_phu50) + p2 + fr_phu50; //shape coefficient
			//	   //float fr_p = (fr_p1 - fr_p3)*(1 - (fr_phu / (fr_phu + exp(p1 - p2*fr_phu)))) + fr_p3; //fraction of phosphorus in the plant biomass on a given day
			//	   //float biomass_P = fr_p*biomass; //actual mass of phosphorus stored in plant material (kg/ha)
			//	   //float biomass_P_optimal = fr_p*biomass; //optimal mass of phosphorus stored in plant material (kg/ha)
			//	   //float m5 = biomass_P_optimal - biomass_P;
			//	   //float m6 = 4.f * fr_p3*delta_biomass;
			//	   //float P_up = 1.5f* min(m3, m4);		//potential phosphorus uptake ------->I think it'll be fine to just use this from the top layer as opposed to do a depth distribution
			//	   //float P_PlantUptake = min(P_up, Psolution);

			//	   //CROP YIELD
			//	   //float fr_root = 0.40 - 0.20*fr_phu; //fraction of total biomass partitioned to roots on a given day in the growing season. SWAT varies the fraction of total biomass in roots from 0.40 at emergence to 0.20 at maturity
			//	   //float bio_ag = (1 - fr_root)*biomass; //aboveground biomass on the day of harvest (kg/ha)
			//	   //float HI_optimal = 0.0f;
			//	   //float HI = HI_optimal*((100 * fr_phu) / (100 * fr_phu + exp(11.1 - 10 * fr_phu)));
			//	   //float yield = 0.0f; //crop yield (kg/ha)
			//	   //if (HI > 1)
			//	   // yield = biomass*(1 - (1 / (1 + HI)));
			//	   // else
			//	   // yield=bio_ag*HI;
			//	   //float yield_N = fr_N*yield;
			//	   //float yield_P = fr_P*yield;


			//	   //MINERALIZATION

			//	   /*if (pSV->m_name = "OrgN_active")

			//	   {
			//	   float N_mina = B_min*(pow((y_sw*y_tmp), 0.5f)*OrgN_active);		//mineralization from orgN_active pool to NO3
			//	   float N_minf = 0.8f*delta_ntr*OrgN_fresh;		//mineralization from orgN_fresh to NO3

			//	   //ORGANIC NITROGEN
			//	   float fr_actN = 0.02f;
			//	   float B_trns = 0.00001f;			//rate constant for humus mineralization
			//	   float epsilon_cn = (0.58f*rsd) / (OrgN_fresh + NO3);		//residue C:N ratio in the soil layer
			//	   float epsilon_cp = (0.58f*rsd) / (OrgP_fresh + PSolution);			//residue C:P ratio in the soil layer

			//	   //OrgN FLOWS
			//	   float delta_ntr = B_rsd*y_ntr*pow((y_sw*y_tmp), 0.5f);		//residue decay rate constant
			//	   float N_dec = 0.2f*OrgN_fresh*delta_ntr;
			//	   float Ntrans = 0.0f;
			//	   if (B_trns*OrgN_active*((1.f / fr_actN) - 1.f) - OrgN_stable <0)
			//	   Ntrans = B_trns*OrgN_active*((1.f / fr_actN) - 1.f) - OrgN_stable;
			//	   else
			//	   Ntrans = -B_trns*OrgN_active*((1.f / fr_actN) - 1.f) - OrgN_stable;
			//	   }
			//	   */




			//	   float P_solution = 0.01f*rsd;		//phosphorus in solution (mg/kg) initial value for PSolution?


			//	   //NITRIFICATION & VOLATILIZATION

			//	   float n_cec = 0.15f;		//volatilization cation exchange factor, SWAT default=0.15
			//	   float n_midz = 1.f - (z_mid / (z_mid + exp(4.706f - 0.0305f*z_mid)));		//volatilization factor, 
			//	   float n_sw = 1.0f;
			//	   if (SW<(0.25f*field_capacity - 0.75f*WP))
			//		   n_sw = (SW - WP) / (0.25*(field_capacity - WP));		//nitrification soil water factor, 
			//	   float n_tmp = 0.0f;
			//	   if (T_soil>5.0f)
			//		   n_tmp = 0.41f*((T_soil - 5.0f) / 10.0f);		//nitrification/volatilization temperature factor, 
			//	   float n_nit = n_tmp*n_sw;		//nitrification regulator,
			//	   float n_vol = n_tmp*n_midz*n_cec;		//volatilization regulator,
			//	   float N_nit_vol = NH4*(1.f - exp(-n_nit - n_vol));		//amount of ammonium converted via nitrification and volatilization,
			//	   float fr_nit = 1.f - exp(-n_nit); //
			//	   float fr_vol = 1.f - exp(-n_vol);		//estimated fraction of nitrogen lost by volatilization, 

			//	   //NITRIFICATION & VOLATILIZATION FLOWS
			//	   float nitrification = 0.0f; //flow from NH4 to NO3
			//	   if (fr_nit + fr_vol != 0)
			//		   nitrification = (fr_nit / (fr_nit + fr_vol))*N_nit_vol;
			//	   float volatization = 0.0f;
			//	   if (fr_nit + fr_vol != 0)
			//		   volatization = (fr_vol / fr_nit + fr_vol)*N_nit_vol;

			//	   //SEDIMENT TRANSPORT
			//	   /*float w_mobile_surf = Q_surf + Q_lat + w_perc;		//amount of mobile water in the surface layer (mm H2O)
			//	   float sed = pHRU->m_currentSediment;		//daily sediment yield (metric tons), 1697 lb * 0.00045359237 lb/metric tons = 0.77metric tons; 0.77tons/365 days=0.002 tons/day
			//	   float Conc_sed_surf = sed / (10 * area_hru*Q_surf);
			//	   float epsilon_nsed = 0.78f*pow(Conc_sed_surf, -0.2468f);		//nitrogen enrichment ratio for nitrogen transported with sediment, Conc_sed_surf from MUSLE
			//	   float orgN_surf_runoff = 0.001f*Conc_orgN*(sed / area_hru)*epsilon_nsed; //(kg/ha)

			//	   //PHOSPHORUS STATE VARIABLES
			//	   float epsilon_psed = 0.78f*pow(Conc_sed_surf, -0.2468f);		//phosphorus enrichment ratio, Menzel (1980)
			//	   float orgP_frsh_surf = 0.0003f*rsd;			//this doesn't actually connect to anything, I need to check why it's in the model
			//	   float k_d_surf = 175.f;		 //phosphorus soil partitioning coefficient (m3/mg), ratio of soluble phosphorus in the surface 10mm of soil to the concentration of soluble phosphorus in surface runoff, SWAT default=175.0
			//	   float P_surf = (PSolution*Q_surf) / (bulk_density_surface*depth_surf*k_d_surf); //amount of soluble P lost in surface runoff (kg/ha)
			//	   float B_eqP = 0.0006f;		 //slow equilibrium constant default=0.0006
			//	   float pai=0.4f;		 //phosphorus availability index (default from SWAT=0.4, chose simpler version because more complicated approach requires knowledge of P in solution before and after fertilizer application)
			//	   float Pmina_humic = fabs(1.4f*B_min*pow((y_tmp*y_sw), 0.5f)*OrgP_active);		//(kg/ha)
			//	   float Conc_sedP = 100.f * (MinP_active + MinP_stable + OrgP_fresh + OrgP_humic) / (depth_surf*bulk_density_surface); //concentration of phosphorus attached to sediment in the soil surface layer (g/metric ton)
			//	   float sedP_surf = 0.001f*Conc_sedP*(sed / area_hru)*epsilon_psed;		//amount of phosphorus transported with sediment to the main channel in surface runoff (kg/ha)
			//	   float P_decay = 0.2f*delta_ntr*OrgP_fresh;			//Flow from OrgP_fresh stock to OrgP_humic
			//	   float P_active_to_stable = B_eqP*(4.f * MinP_active - MinP_stable);		 //Flow between MinP_stable and MinP_active (kg/ha)..............still not sure this is right
			//	   if (MinP_stable > 4.f * MinP_active)
			//	   {
			//	   if (0.1f*B_eqP*(4 * MinP_active - MinP_stable) > 0)
			//	   P_active_to_stable = (0.1f*B_eqP*(4.f * MinP_active - MinP_stable));
			//	   else
			//	   P_active_to_stable = (-0.1f*B_eqP*(4.f * MinP_active - MinP_stable));
			//	   }
			//	   float P_soluble_to_active = 0.6f*(PSolution - MinP_active*(pai / (1.f - pai)));		 // Flow between PSolution and MinP_active.........check this with Kellie to make sure the equations setup correctly
			//	   if (PSolution > MinP_active*(pai / (1.f - pai)))
			//	   P_soluble_to_active = PSolution - MinP_active*(pai / (1.f - pai));
			//	   else P_soluble_to_active = 0.1f*(PSolution - MinP_active*(pai / (1.f - pai)));
			//	   float Pmina_fresh = fabs(0.8f*delta_ntr*OrgP_fresh); //(kg/ha)
			//	   float P_plant_uptake = 0.0f;
			//	   if (PSolution > 0.f)
			//	   P_plant_uptake = 0.006f*PSolution;
			//	   */


			//	   //Begin Soil Rate Calculations for NO3 and NH4

			//	   float SoilFlux_NO3 = 0.0f; //NO3 OverLayer calculations
			//	   float uptakeEfficiency = 0.5f;

			//	   //DENITRIFCATION & PLANT UPTAKE
			//	   float B_denit = 1.4f;		// rate coefficient of denitrification
			//	   float denitrification = 0.0f; //flow from NO3 out of the system
			//	   if (y_sw > y_sw_thr)
			//		   denitrification = NO3*(1.f - exp(-B_denit*y_tmp*orgC)); //upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere

			//	   //NO3 FERTILILZER APPLICATION AND NO3 IN RAINFALL			
			//	   float R_no3 = 0.01f;		//concentration of nitrate in the rain (mg/L) (set to 0.01 for testing)			
			//	   //float NO3_Rainfall = 0.01f*R_no3*R_day;		//Nitrate added by rainfall (kg/ha), only added to the surface layer
			//	   float appRate = 0.0f;
			//	   if (k < 1)
			//		   appRate = ApplicationRate(time, pOverLayer, pFlowContext, k); //HOW DO I LINK THIS TO THE APPLICATION TABLE/RATE? -->OSU EXTENSION SAYS 100-140lbN/acre (112.27 - 157.18kg/ha)			
			//	   float NO3_Rainfall = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0.01f*R_no3;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
			//	   NO3_Rainfall += appRate;

			//	   // --> CROP & NUTRIENT MANAGEMENT <--
			//	   float N_fertilizer_application = 0.0f;			//defined in OSU extension guide
			//	   if (time == 182.0f)
			//		   N_fertilizer_application = area_hru*appRate;
			//	   // float P_fertilizer_application = 1.0f;			//defined in OSU extension guide
			//	   float NH4_fertilizer = 0.5f*N_fertilizer_application;
			//	   float NO3_fertilizer = 0.5f*N_fertilizer_application;
			//	   float organic_N_fertilizer = 1.0f;
			//	   float residue_application = 0.0f;
			//	   if (time == 182.0f)
			//		   residue_application = 89.75f;

			//	   //float c1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone

			//	   //NO3 LEACHING			 
			//	   float B_NO3 = 0.2f;			//nitrate percolation coefficient, SWAT recommends range from 0.01 - 1.0
			//	   float NO3_conc = 7.f * exp(-z / 1000.f);		//initial NO3 concentration is a function of depth (z), (mg/kg)
			//	   float Conc_NO3_mobile = 0.0f;
			//	   float w_mobile = 0.0f;							//amount of mobile water in the layer (mm H2O)
			//	   if (k = 0)
			//		   w_mobile = Q_lat + w_perc + Q_surf;
			//	   else w_mobile = Q_lat + w_perc;
			//	   if (w_mobile > 0.f)
			//		   Conc_NO3_mobile = (NO3*(1 - exp((-w_mobile) / ((1 - theta_e)*SAT)))) / w_mobile;
			//	   float NO3_lateral = Conc_NO3_mobile*Q_lat;		//Lateral movement of dissolved nitrate, 
			//	   float NO3_surf = 0.0f;
			//	   if (k == 0)
			//		   NO3_surf = Conc_NO3_mobile*B_NO3*Q_surf;	//Nitrate export in surface runoff,
			//	   float NO3_perc = w_perc * Conc_NO3_mobile; //down flux across lower surface of this layer (a loss) Percolation of dissolved NO3 			 
			//	   float advectiveFluxOver = NO3_Rainfall - NO3_perc - NO3_lateral;
			//	   float NO3_transformation = nitrification - denitrification + NO3_fertilizer; //- NO3_PlantUptake; //(ks*pOverLayer->GetExtraStateVarValue(k)) + (concOver*kd*ks*bd);//degraded of absorbed			
			//	   SoilFlux_NO3 = advectiveFluxOver - NO3_transformation; //need to reflect calculations for state variable
			//	   pOverLayer->AddExtraSvFluxFromGlobalHandler(k, SoilFlux_NO3);

			//	   //NH4 OVERLAYER CALCULATIONS			
			//	   float R_nh4 = 0.01;		//concentration of ammonium in the rain (mg/L) (set to 0.01 for testing)
			//	   float NH4_Rainfall = 0.01f*R_day*R_nh4;		//Ammonium added by rainfall (kg/ha), only added to the surface layer						
			//	   float NH4_transformation = NH4_fertilizer - nitrification; //(ks*pOverLayer->GetExtraStateVarValue(k)) + (concOver*kd*ks*bd);//degraded of absorbed			
			//	   float SoilFlux_NH4 = 0.0f; //NH4 OverLayer calculations			
			//	   SoilFlux_NH4 = advectiveFluxOver - NH4_transformation; //need to reflect calculations for state variable
			//	   pOverLayer->AddExtraSvFluxFromGlobalHandler(k, SoilFlux_NH4);
			//	   // Finish Soil Rate Calculations

			//   }

			//   //float koc = 0.01;
			//   //float ks = 0.1f;



			//   //      VData soilType = 0;

			//   //     pFlowContext->pFlowModel->GetHRUData(pHRU, pSoilPropertiesTable->m_iduCol, soilType, DAM_FIRST);  // get soil for this particular HRU
			//   //     if (pSoilPropertiesTable->m_type == DOT_FLOAT)
			//   //        soilType.ChangeType(TYPE_FLOAT);
			//   //float om = 10.0f;
			//   //float bd = 1.0f;
			//   // bool ok = pSoilPropertiesTable->Lookup(soilType, m_col_om, om);// get the organic matter content
			//   // ok = pSoilPropertiesTable->Lookup(soilType, m_col_bd, bd);// get the organic matter content

			//   //Begin Soil Rate Calculations

			//   /*float ss = 0.0f;
			//   float uptakeEfficiency = 0.5f;
			//   float a1 = pOverLayer->m_waterFluxArray[FL_TOP_SINK] * concOver * uptakeEfficiency;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
			//   float appRate = 0.0f;
			//   if (k<2)
			//   appRate = ApplicationRate(time, pOverLayer, pFlowContext, k);
			//   float b1 = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
			//   b1 += appRate;
			//   float c1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
			//   float d1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SINK] * 0; //down flux across lower surface of this layer (a loss)
			//   float e111 = pOverLayer->m_waterFluxArray[FL_STREAM_SINK] * concOver; //loss to streams

			//   float advectiveFluxOver = a1 + b1 + c1 + d1 + e111;


			//   float kd = koc*om / 1000.0f; //ml/g
			//   float transformation = (ks*pOverLayer->GetExtraStateVarValue(k)) + (concOver*kd*ks*bd);//degraded of absorbed

			//   ss = advectiveFluxOver - transformation; //need to reflect calculations for state variable
			//   pOverLayer->AddExtraSvFluxFromGlobalHandler(k, ss);
			//   // Finish Soil Rate Calculations*/

			//   // Begin Upper Box  Rate Calculations

			//   /*a1 = pUpperLayer->m_waterFluxArray[FL_TOP_SINK] * concUpper;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
			//   b1 = pUpperLayer->m_waterFluxArray[FL_TOP_SOURCE] * concOver;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
			//   c1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * concLower;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
			//   d1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SINK] * concUpper; //down flux across lower surface of this layer (a loss)
			//   float e11 = pUpperLayer->m_waterFluxArray[FL_STREAM_SINK] * concUpper; //loss to streams

			//   advectiveFluxOver = a1 + b1 + c1 + d1 + e11;

			//   transformation = (ks*pUpperLayer->GetExtraStateVarValue(k)) + (concUpper*kd*ks*bd);//degraded of absorbed

			//   ss = advectiveFluxOver - transformation;
			//   pUpperLayer->AddExtraSvFluxFromGlobalHandler(k, ss);
			//   */
			//   // Finish Upper Box Rate Calculations

			//   // Begin Lower Box Rate Calculations

			//   //transformation = (ks*pLowerLayer->GetExtraStateVarValue(k)) + (concLower*kd*ks*bd);//degraded of absorbed
			//   //ss = advectiveFluxOver - transformation;
			//   //pLowerLayer->AddExtraSvFluxFromGlobalHandler(k, ss);     //m3/d

			//   // End Lower Soil Box Rate Calculations

			//   Reach * pReach = pUpperLayer->GetReach();


			//   /////////////////////////////////////////THIS SEGMENT WILL LIKELY BE REALLY IMPORTANT TO RESTORE///////////////////

			//   //if (pReach) 
			//   //   {
			//   //   ReachSubnode *subnode = pReach->GetReachSubnode(0);
			//   //   subnode->AddExtraSvFluxFromGlobalHandler(k, ((-e11 - e111))); //kg/d.  e1 is stored as a loss from the unsaturated zone - so it is negative and needs to be converted to a + value.
			//   //   }

			//   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			//   /*

			//   float a1 = pOverLayer->m_waterFluxArray[FL_TOP_SINK] * concOver;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
			//   float b1 = pOverLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
			//   float c1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
			//   float d1 = pOverLayer->m_waterFluxArray[FL_BOTTOM_SINK] * 0; //down flux across lower surface of this layer (a loss)
			//   float e111 = pOverLayer->m_waterFluxArray[FL_STREAM_SINK] * concOver; //loss to streams

			//   ss = a1 + b1 + c1 + d1 + e111;

			//   pOverLayer->AddExtraSvFluxFromGlobalHandler(k, ss);

			//   a1 = pUpperLayer->m_waterFluxArray[FL_TOP_SINK] * concUpper;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
			//   b1 = pUpperLayer->m_waterFluxArray[FL_TOP_SOURCE] * 0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
			//   c1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * concLower;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
			//   d1 = pUpperLayer->m_waterFluxArray[FL_BOTTOM_SINK] * concUpper; //down flux across lower surface of this layer (a loss)
			//   float e11 = pUpperLayer->m_waterFluxArray[FL_STREAM_SINK] * concUpper; //loss to streams

			//   ss = a1 + b1 + c1 + d1 + e11;

			//   pUpperLayer->AddExtraSvFluxFromGlobalHandler(k, ss);

			//   float e1 = pLowerLayer->m_waterFluxArray[FL_TOP_SINK] * concLower;//upward flux across upper surface of this layer (a loss) -saturated to unsaturated zone
			//   float f1 = pLowerLayer->m_waterFluxArray[FL_TOP_SOURCE] * concUpper;//down flux across upper surface of this layer (a gain) - gwREcharge
			//   float g1 = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SOURCE] * 0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturate zone
			//   float h1 = pLowerLayer->m_waterFluxArray[FL_BOTTOM_SINK] * concLower; //down flux across lower surface of this layer (a loss)
			//   float i1 = 0.0f; // outgoing flow to horizontal neighbors
			//   float j1 = 0.0f; // incoming flow from horizontal neighbors

			//   ss = e1 + f1 + g1 + h1 + i1 + j1;
			//   ss = 0.0f;

			//   pLowerLayer->AddExtraSvFluxFromGlobalHandler(k, ss);     //m3/d
			//   Reach * pReach = pUpperLayer->GetReach();
			//   if (pReach)
			//   {
			//   ReachSubnode *subnode = pReach->GetReachSubnode(0);
			//   subnode->AddExtraSvFluxFromGlobalHandler(k, ((-e11 - e111))); //kg/d.  e1 is stored as a loss from the unsaturated zone - so it is negative and needs to be converted to a + value.
			//   }*/
		 //  }
	  // }

	  // return -1.0f;
   //}