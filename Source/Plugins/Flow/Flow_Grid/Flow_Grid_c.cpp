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
// Flow_Grid.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Flow_Grid.h"
#include <Flow\Flow.h>
#include <DATE.HPP>
#include <omp.h>
#include <EnvEngine\EnvModel.h>
#include <EnvExtension.h>



//extern FlowProcess *gpFlow;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Flow_Grid::Flow_Grid(void) 
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
   , m_col_kSatArg(-1)
   , m_col_lamdaArg(-1)
   , m_col_phiArg(-1)
   , m_col_wpArg(-1)
   , m_col_fcPct(-1)
   , m_col_HOF_Threshold(-1)
   , m_col_runoffCoefficient(-1)
   , m_col_k(-1)
   , m_col_ic(-1)
   , m_laiFilename()
   , m_pLAI_Table(NULL)
   , m_pAgeHeight_Table(NULL)
   , m_pRotationAge_Table(NULL)
   , m_rotationAgeFilename()
   , m_harvestFilenameOffSet(0)
   , m_colRotationAge(-1)
   , m_colHeight(-1)
   , m_colLulc_a(-1)
   , m_col_rotation()
   {}

Flow_Grid::~Flow_Grid(void)
   {
   if ( m_uplandWaterSourceSinkArray != NULL )
      delete [] m_uplandWaterSourceSinkArray;

   if ( m_soilDrainage != NULL )
      delete [] m_soilDrainage;

   if ( m_instreamWaterSourceSinkArray != NULL )
      delete [] m_instreamWaterSourceSinkArray;
   }

float Flow_Grid::Init_Year(FlowContext *pFlowContext)
   {
  
   //10YrsAG 10YrsPINE 25YrsPINE 35Yrs 35YrAG
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();
   EnvContext *pEnvContext = pFlowContext->pEnvContext;
   MapLayer *pLayer = (MapLayer*)pEnvContext->pMapLayer;
   pLayer->m_readOnly = false;
   for (MapLayer::Iterator idu = pEnvContext->pMapLayer->Begin(); idu != pEnvContext->pMapLayer->End(); idu++)
      {
      //Get the harvest unit from map
      int harvestOffset=-1;
      int col = pLayer->GetFieldCol(m_col_rotation);
      ASSERT(col>0);
      pLayer->GetData(idu, col, harvestOffset);
                
      //Get the lai for that harvest unit from the lai table
      if (harvestOffset>0 && harvestOffset <50) //if the value is outside this range, that area is not part of the overall timber strategy (it is agricultural or developed)
         { 
         float lai= m_pLAI_Table->Get(harvestOffset-1,pFlowContext->pEnvContext->yearOfRun);
         float rotationAge = m_pRotationAge_Table->Get(harvestOffset - 1, pFlowContext->pEnvContext->yearOfRun);
         float height = m_pAgeHeight_Table->IGet(rotationAge);
         //Set LAI in the IDU layer
         pFlowContext->pFlowProcess->UpdateIDU(pFlowContext->pEnvContext, idu, pFlowContext->pFlowModel->m_colLai, lai, 1);
         pFlowContext->pFlowProcess->UpdateIDU(pFlowContext->pEnvContext, idu, m_colRotationAge, rotationAge, 1);
         pFlowContext->pFlowProcess->UpdateIDU(pFlowContext->pEnvContext, idu, m_colHeight, height, 1);
         pLayer->SetData(idu, pFlowContext->pFlowModel->m_colLai, lai);  
         pLayer->SetData(idu, m_colRotationAge, rotationAge);
         pLayer->SetData(idu, m_colHeight, height);
         }    
      }
   pLayer->m_readOnly = true;
   return 1.0f;
   }

float Flow_Grid::Init_Grid_Fluxes( FlowContext *pFlowContext )
   {
   EnvExtension::CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colRotationAge, _T("AGECLASS"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colHeight, _T("Height"), TYPE_FLOAT, CC_AUTOADD); 
   EnvExtension::CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colLulc_a, _T("LULC_A"), TYPE_INT, CC_AUTOADD);
   pFlowContext->pFlowProcess->AddInputVar(_T("Harvest Scenario"), m_harvestFilenameOffSet, _T("0=first, 1, 2, 3, 4"));

   m_pLAI_Table = new FDataObj;
   m_pRotationAge_Table = new FDataObj;
   m_pAgeHeight_Table = new FDataObj;
 
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable( "GRID" );   // store this pointer (and check to make sure it's not NULL)
   m_col_wp = pLULC_Table->GetFieldCol( "wp" );         // get the location of the parameter in the table    
   m_col_phi = pLULC_Table->GetFieldCol( "phi" );  
   m_col_lamda = pLULC_Table->GetFieldCol( "lamda" ); 
   m_col_kSat = pLULC_Table->GetFieldCol( "kSat" ); 
   m_col_ple = pLULC_Table->GetFieldCol( "ple" ); 
   m_col_lp = pLULC_Table->GetFieldCol( "lp" ); 
   m_col_fc = pLULC_Table->GetFieldCol( "fc" ); 
   m_col_HOF_Threshold = pLULC_Table->GetFieldCol("HOF_Threshold");
   m_col_fcPct = pLULC_Table->GetFieldCol( "fcPct" ); 
   m_col_ic = pLULC_Table->GetFieldCol( "ic" ); 
   m_col_runoffCoefficient = pLULC_Table->GetFieldCol("runoffCoefficient");

   ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable( "GWParams" );   // store this pointer (and check to make sure it's not NULL)
   m_col_kSatGw = pGWParam_Table->GetFieldCol( "ksat" );         // get the location of the parameter in the table    
   m_col_pleGw = pGWParam_Table->GetFieldCol( "ple" );  
   m_col_phiGw = pGWParam_Table->GetFieldCol( "phi" ); 
   m_col_k = pLULC_Table->GetFieldCol( "k" ); 

   ParamTable *pArgillicParam_Table = pFlowContext->pFlowModel->GetTable("ArgillicParams");   // store this pointer (and check to make sure it's not NULL)
   m_col_kSatArg = pGWParam_Table->GetFieldCol("ksat");         // get the location of the parameter in the table    
   m_col_wpArg = pGWParam_Table->GetFieldCol("wp");
   m_col_phiArg = pGWParam_Table->GetFieldCol("phi");
   m_col_lamdaArg = pLULC_Table->GetFieldCol("lamda");
  

   int hruCount = pFlowContext->pFlowModel->GetHRUCount();  
   HRU *pHRU = pFlowContext->pFlowModel->GetHRU(0);
   int hruLayerCount = pHRU->GetPoolCount();


   return 0.0f;
   }

float Flow_Grid::InitRun_Grid_Fluxes(FlowContext *pFlowContext)
{
 //  EnvExtension::CheckCol(pFlowContext->pEnvContext->pMapLayer, m_colRotationAge, _T("Rot_Age"), TYPE_FLOAT, CC_AUTOADD);
//   pFlowContext->pFlowProcess->AddInputVar(_T("Harvest Scenario"), m_harvestFilenameOffSet, _T("0=first, 1, 2, 3, 4"));
   CString file;
   file.Format("FlowGrid%i.xml", m_harvestFilenameOffSet);
   LoadXml(file);

   //CString fileName = NULL;
  // fileName.Format(_T("%s"), m_laiFilename);//the name of the csv file that includes the LAI
   m_pLAI_Table->ReadAscii(m_laiFilename);
 //  fileName.Format(_T("%s"), m_rotationAgeFilename);//the name of the csv file that includes the Age of each rotation
//   m_pRotationAge_Table->ReadAscii(m_rotationAgeFilename);
   m_pAgeHeight_Table->ReadAscii(m_ageHeightFilename);

   return 0.0f;
}


float Flow_Grid::Grid_Fluxes( FlowContext *pFlowContext )
   {
   if ( pFlowContext->timing & 1 ) // Init()
      return Flow_Grid::Init_Grid_Fluxes( pFlowContext );

   if (pFlowContext->timing & 4) // Init()
      return Flow_Grid::Init_Year(pFlowContext);

   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable( "GRID" );   // store this pointer (and check to make sure it's not NULL)
   ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable( "GWParams" ); 
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();  
   float minElev=pFlowContext->pFlowModel->m_minElevation;

   int hruIndex=0;
#pragma omp parallel for 
   for ( int h=0; h < hruCount; h++ )
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      int hruLayerCount = pHRU->GetPoolCount();

      float airTemp=0.0f; float precip=0.0f;
      float time=pFlowContext->time-(int)pFlowContext->time;
      float currTime=(float)pFlowContext->dayOfYear+time;
      pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP,pHRU,(int)currTime,precip);//mm/d


      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN,pHRU,(int)currTime,airTemp);//C 
      float wp,phi,lamda,kSat,ple,kSatGw,pleGw,fc, phiGw,fcPct,k, ic, lp=0.0f;

      VData lulcA=0;
      pFlowContext->pFlowModel->GetHRUData( pHRU, pLULC_Table->m_iduCol, lulcA, DAM_FIRST );  // get HRU info
      if (pLULC_Table->m_type==DOT_FLOAT)
            lulcA.ChangeType(TYPE_FLOAT);
      bool ok = pLULC_Table->Lookup( lulcA, m_col_wp, wp );     // wilting point
      ok = pLULC_Table->Lookup( lulcA, m_col_phi, phi );        // porosity
      ok = pLULC_Table->Lookup( lulcA, m_col_lamda, lamda );    // pore size distribution index
      ok = pLULC_Table->Lookup( lulcA, m_col_kSat, kSat );      // saturated hydraulic conductivity    
      ok = pLULC_Table->Lookup( lulcA, m_col_ple, ple );        // power law exponent
      ok = pLULC_Table->Lookup( lulcA, m_col_fc, fc );          // field capacity
      ok = pLULC_Table->Lookup( lulcA, m_col_lp, lp );          // power law exponent
      ok = pLULC_Table->Lookup( lulcA, m_col_k, k );            // overland flow recession constant
      ok = pLULC_Table->Lookup( lulcA, m_col_ic, ic );          // infiltration capacity
      fcPct=1.0f;

      VData geo=0;
      pFlowContext->pFlowModel->GetHRUData( pHRU, pGWParam_Table->m_iduCol, geo, DAM_FIRST );  // get HRU info
      if (pGWParam_Table->m_type==DOT_FLOAT)
            geo.ChangeType(TYPE_FLOAT);

      ok = pGWParam_Table->Lookup( geo, m_col_kSatGw, kSatGw );    // saturated hydraulic conductivity    
      ok = pGWParam_Table->Lookup( geo, m_col_pleGw, pleGw );        // power law exponent
      ok = pGWParam_Table->Lookup( geo, m_col_phiGw, phiGw );        // power law exponent

      float infiltrationExcess=0.0f;
      float soilToStream=0.0f;
      float groundwaterToSoil=0.0f;
      float gwRecharge=0.0f;
      float ssUpper=0.0f;
      float ssLower=0.0f;

      //Upper Soil Layer
      HRUPool *pUpperLayer = pHRU->GetPool( 1 );
    
      float wcU=(float)pUpperLayer->m_volumeWater/(pHRU->m_area*pUpperLayer->m_depth);//of layer 0, unsaturated
      pUpperLayer->m_wc=wcU;


      if (precip > ic && wcU > lp)
         {
         infiltrationExcess=((precip-ic))/1000.0f;
         precip=ic;
         }

      precip/=1000.0f;//m

      if (wcU > wp)
         {
         if (wcU > phi)
            {
            soilToStream=(float)pUpperLayer->m_volumeWater*(wcU-(phi))/pHRU->m_area ;
            wcU = phi;
            infiltrationExcess+=precip;
            precip=0.0f;
            }

         gwRecharge=CalcBrooksCoreyKTheta(wcU, wp, phi, lamda, kSat);
         ASSERT (gwRecharge < 100.0f);
         }     
      
      //Lower Groundwater Layer
      HRUPool *pLowerLayer = pHRU->GetPool( 2 );
      float wcL=(float)pLowerLayer->m_volumeWater/(pHRU->m_area*pLowerLayer->m_depth);//of layer 0, saturated
      
      pLowerLayer->m_wc=wcL;
    //  waterDepth = float(pLowerLayer->m_volumeWater / pHRU->m_area*1000.0f);//mm
    //  pLowerLayer->m_wc = waterDepth;//mm water

      float gwExchange = CalcSatWaterFluxes(pHRU, pLowerLayer, phiGw, pleGw, kSatGw, groundwaterToSoil, minElev);
      ASSERT (groundwaterToSoil >= 0.0f);

      float verticalDrainage = 0.0f;//kSatGw*0.01f;//10% of the vertical drainage rate is assumed to leave the bottom of this lowest simulated layer

      // Overland flow Layer
      HRUPool *pOverLayer = pHRU->GetPool( 0 );
      float quickflowToStream=((float)pOverLayer->m_volumeWater/pHRU->m_area)*k;
      float ssOverland=infiltrationExcess-quickflowToStream;
      pOverLayer->m_wc=(float)pOverLayer->m_volumeWater/pHRU->m_area;
      pLowerLayer->m_wc=wcL;
     // waterDepth = float(pOverLayer->m_volumeWater / pHRU->m_area*1000.0f);//mm
      //pOverLayer->m_wc = waterDepth;//mm water

      pOverLayer->AddFluxFromGlobalHandler(infiltrationExcess*pHRU->m_area,FL_TOP_SOURCE);
      pOverLayer->AddFluxFromGlobalHandler(quickflowToStream*pHRU->m_area,FL_STREAM_SINK);

      pUpperLayer->AddFluxFromGlobalHandler(precip*pHRU->m_area,FL_TOP_SOURCE);
      pUpperLayer->AddFluxFromGlobalHandler(groundwaterToSoil*pHRU->m_area,FL_BOTTOM_SOURCE);
      pUpperLayer->AddFluxFromGlobalHandler(gwRecharge*pHRU->m_area,FL_BOTTOM_SINK);
      pUpperLayer->AddFluxFromGlobalHandler(soilToStream*pHRU->m_area,FL_STREAM_SINK);

      pLowerLayer->AddFluxFromGlobalHandler(groundwaterToSoil*pHRU->m_area,FL_TOP_SINK);
      pLowerLayer->AddFluxFromGlobalHandler(gwRecharge*pHRU->m_area,FL_TOP_SOURCE);
      pLowerLayer->AddFluxFromGlobalHandler(verticalDrainage*pHRU->m_area,FL_BOTTOM_SINK);

      pLowerLayer->AddFluxFromGlobalHandler(gwExchange*pHRU->m_area,FL_USER_DEFINED);

      Reach * pReach = pLowerLayer->GetReach(); 
      if ( pReach )
         pReach->AddFluxFromGlobalHandler( (soilToStream+quickflowToStream)*pHRU->m_area ); //m3/d
	   }

   return 0.0f;
   }

float Flow_Grid::UpdateTreeGrowth(FlowContext *pFlowContext)
   {
	if (pFlowContext->timing & 1) // Init()
		return Flow_Grid::Init_Grid_Fluxes(pFlowContext);

	if (pFlowContext->timing & 2) // Init()
		return Flow_Grid::InitRun_Grid_Fluxes(pFlowContext);


	int hruCount = pFlowContext->pFlowModel->GetHRUCount();
	EnvContext *pEnvContext = pFlowContext->pEnvContext;
	MapLayer *pLayer = (MapLayer*)pEnvContext->pMapLayer;
	pLayer->m_readOnly = false;
	float rotationAge = 0.0f;
	for (MapLayer::Iterator idu = pEnvContext->pMapLayer->Begin(); idu != pEnvContext->pMapLayer->End(); idu++)
	   {
		//Get the harvest unit from map
		int lulcA = -1;

		pLayer->GetData(idu, m_colLulc_a, lulcA);

		//Get the lai for that harvest unit from the lai table
		if (lulcA==2) //if the polygon is forested
		   {		
			pLayer->GetData(idu, m_colRotationAge,rotationAge);
			float lai = m_pLAI_Table->IGet(rotationAge);
			float height = m_pAgeHeight_Table->IGet(rotationAge);
			//Set LAI in the IDU layer
			pFlowContext->pFlowProcess->UpdateIDU(pFlowContext->pEnvContext, idu, pFlowContext->pFlowModel->m_colLai, lai, ADD_DELTA);
			                      //pFlowContext->pFlowProcess->UpdateIDU(pFlowContext->pEnvContext, idu, m_colRotationAge, rotationAge, 1);
			pFlowContext->pFlowProcess->UpdateIDU(pFlowContext->pEnvContext, idu, m_colHeight, height, ADD_DELTA);
		//	pLayer->SetData(idu, pFlowContext->pFlowModel->m_colLai, lai);
			         //pLayer->SetData(idu, m_colRotationAge, rotationAge);
		//	pLayer->SetData(idu, m_colHeight, height);
		   }
	   }
	pLayer->m_readOnly = true;
	return 1.0f;
   }

float Flow_Grid::SRS_5_Layer(FlowContext *pFlowContext)
      {
      if (pFlowContext->timing & 1) // Init()
         return Flow_Grid::Init_Grid_Fluxes(pFlowContext);

      int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
      ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable("GRID");   // store this pointer (and check to make sure it's not NULL)
      ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable("GWParams");
      ParamTable *pArgillicParam_Table = pFlowContext->pFlowModel->GetTable("ArgillicParams");
      int hruCount = pFlowContext->pFlowModel->GetHRUCount();
      float minElev = pFlowContext->pFlowModel->m_minElevation;


      int hruIndex = 0;

  #pragma omp parallel for 
      for (int h = 0; h < hruCount; h++)
         {
         HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
         int hruLayerCount = pHRU->GetPoolCount();

         float airTemp = 0.0f; float precip = 0.0f;
         float time = pFlowContext->time - (int)pFlowContext->time;
         float currTime = (float)pFlowContext->dayOfYear + time;
         float dt = pFlowContext->timeStep;
         //dt = 1.0f;

         pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, (int)currTime, precip);//mm/d


         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, (int)currTime, airTemp);//C 
         float wp = 0.1f; float phi = 0.33f; float lamda = 0.1f; float kSat = 1.0f; float ple = 2.f;
         float kSatGw = 1.0f; float pleGw = 2.f; float phiGw = 0.33f; float fcPct = 0.5f; float kSatArg = 0.1f;
         float wpArg = 10.0f; float lamdaArg = 0.1f; float phiArg = 0.3f;

         VData lulcA = 0;
         pFlowContext->pFlowModel->GetHRUData(pHRU, pLULC_Table->m_iduCol, lulcA, DAM_FIRST);  // get HRU info
         if (pLULC_Table->m_type == DOT_FLOAT)
            lulcA.ChangeType(TYPE_FLOAT);
         bool ok = pLULC_Table->Lookup(lulcA, m_col_wp, wp);     // wilting point
         ok = pLULC_Table->Lookup(lulcA, m_col_phi, phi);        // porosity
         ok = pLULC_Table->Lookup(lulcA, m_col_lamda, lamda);    // pore size distribution index
         ok = pLULC_Table->Lookup(lulcA, m_col_kSat, kSat);      // saturated hydraulic conductivity    
         ok = pLULC_Table->Lookup(lulcA, m_col_ple, ple);        // power law exponent
        // ok = pLULC_Table->Lookup(lulcA, m_col_fc, fc);          // field capacity
         //ok = pLULC_Table->Lookup(lulcA, m_col_lp, lp);          // power law exponent
         fcPct = 1.0f;

         VData geo = 0;
         pFlowContext->pFlowModel->GetHRUData(pHRU, pGWParam_Table->m_iduCol, geo, DAM_FIRST);  // get HRU info
         if (pGWParam_Table->m_type == DOT_FLOAT)
            geo.ChangeType(TYPE_FLOAT);

         ok = pGWParam_Table->Lookup(geo, m_col_kSatGw, kSatGw);    // saturated hydraulic conductivity    
         ok = pGWParam_Table->Lookup(geo, m_col_pleGw, pleGw);        // power law exponent
         ok = pGWParam_Table->Lookup(geo, m_col_phiGw, phiGw);        // power law exponent

         if (pArgillicParam_Table->m_type == DOT_FLOAT)
            geo.ChangeType(TYPE_FLOAT);

         ok = pArgillicParam_Table->Lookup(geo, m_col_kSatArg, kSatArg);    // saturated hydraulic conductivity    
         ok = pArgillicParam_Table->Lookup(geo, m_col_lamdaArg, lamdaArg);        // power law exponent
         ok = pArgillicParam_Table->Lookup(geo, m_col_wpArg, wpArg);        // power law exponent
         ok = pArgillicParam_Table->Lookup(geo, m_col_phiArg, phiArg);        // power law exponent

         float infiltrationExcess = 0.0f;
         float soilToStream = 0.0f;
         float groundwaterToSoil = 0.0f;
         float unsatToSat = 0.0f;
         float satToArgillic = 0.0;
         float argillicToUnsat = 0.0f;
         float gwUnsatToSat = 0.0f;
         float ssf = 0.0f;
         float gwf = 0.0f;
         float ssUpper = 0.0f;
         float ssLower = 0.0f;
         float shallowGWOutOfCatchment = 0.0f;

         float satToUnsat = 0.0f;
         float argillicToSat = 0.0f;
         float unsatGWToArgillic = 0.0f;
         float satGWToUnsatGW = 0.0f;
         float gwOutOfCatchment = 0.0f;
         float depthOfWater = 0.0f;
         bool isSat1 = false; bool isSat2 = false; bool isSat3 = false; bool isSat4 = false; bool isSat5 = false;

   //ponded
         HRUPool *pPonded = pHRU->GetPool(5);
         float infiltration = pPonded->m_volumeWater*0.5f;
         float runoff = pPonded->m_volumeWater*0.2f;

   //Unsaturated
         HRUPool *pUnsat = pHRU->GetPool(0);
         float wcU = (float)pUnsat->m_volumeWater / (pHRU->m_area*pUnsat->m_depth);//of layer 0, unsaturated m3/m3
         
         float storage = 0.1f;

         precip /= 1000.0f;//m/d

         if (wcU > wp)
            {
            if (wcU > phi)
               {
               soilToStream = (float)pUnsat->m_volumeWater*(wcU - phi) / pHRU->m_area/dt ;
               soilToStream += precip;
               precip = 0.0f;
               wcU = phi;    
               isSat2 = true;
               }
            unsatToSat = CalcBrooksCoreyKTheta(wcU, wp, phi, lamda, kSat);
            }
         pUnsat->m_wc = wcU;//
         pUnsat->m_wDepth = wcU*pUnsat->m_depth / phi*1000.0f;//mm of water
   //End Unsaturated

   //Saturated Above Argillic
         HRUPool *pSatAboveArgillic = pHRU->GetPool(1);
         float totalDepth = pUnsat->m_depth + pSatAboveArgillic->m_depth;
         float wcL = (float)pSatAboveArgillic->m_volumeWater / (pHRU->m_area*pSatAboveArgillic->m_depth);
         if (wcL > phi)
            {
            satToUnsat = (float)pSatAboveArgillic->m_volumeWater*(wcL - phi) / pHRU->m_area / dt  ;
            wcL = phi;
            isSat3 = true;
            unsatToSat = 0.0f;
            }
         pSatAboveArgillic->m_wc = wcL;//water content
         pSatAboveArgillic->m_wDepth = wcL*pSatAboveArgillic->m_depth / phi*1000.0f;//mm of water

         satToArgillic = (float)pSatAboveArgillic->m_volumeWater / pHRU->m_area*0.025f;

        // if (wcL > 0.25f)
            ssf = CalcSatWaterFluxes(pHRU, pSatAboveArgillic, phi, ple, kSat, satToUnsat, minElev, totalDepth, wcL, pLULC_Table, pFlowContext);

         if (pHRU->m_lowestElevationHRU && ssf>0.0f)
            shallowGWOutOfCatchment = ssf*0.01f ;
         
   //End Saturated Above Argillic
         
   // Argillic
         HRUPool *pArgillic = pHRU->GetPool(2);
         float wcArgillic = (float)pArgillic->m_volumeWater / (pHRU->m_area*pArgillic->m_depth);
         if (wcArgillic > phiArg)
            {
            argillicToSat = (float)pArgillic->m_volumeWater*(wcArgillic - phiArg) / pHRU->m_area / dt  ;
            wcArgillic = phiArg;
            isSat4 = true;
    //        satToArgillic = 0.0f;
            }
         pArgillic->m_wc = wcArgillic;//mm of water
         pArgillic->m_wDepth = wcArgillic*pArgillic->m_depth / phiArg*1000.0f;//mm of water

         argillicToUnsat = CalcBrooksCoreyKTheta(wcArgillic, wpArg, phiArg, lamdaArg, kSatArg);
        // if (wcL>wp)
         //   satToArgillic = argillicToUnsat;
   // End Argillic

   // Unsaturated Below Argillic
         HRUPool *pUnBelowArgillic = pHRU->GetPool(3);
         float wcUnBelowArgillic = (float)pUnBelowArgillic->m_volumeWater / (pHRU->m_area*pUnBelowArgillic->m_depth);

         if (wcUnBelowArgillic > phiGw)
            {
            unsatGWToArgillic = (float)pUnBelowArgillic->m_volumeWater*(wcUnBelowArgillic - phiGw) / pHRU->m_area / dt  ;
            wcUnBelowArgillic = phiGw;
            isSat5 = true;
     //       argillicToUnsat = 0.0f;
            }
         pUnBelowArgillic->m_wc = wcUnBelowArgillic;//mm of water
         pUnBelowArgillic->m_wDepth = wcUnBelowArgillic*pUnBelowArgillic->m_depth / phiGw*1000.0f;//mm of water
         gwUnsatToSat = CalcBrooksCoreyKTheta(wcUnBelowArgillic, wp, phiGw, lamda, kSatGw);
   // End Unsaturated Below Argillic

   //Saturated Below Argillic
         HRUPool *pSatBelowArgillic = pHRU->GetPool(4);
         float wcSatBelowArgillic = (float)pSatBelowArgillic->m_volumeWater / (pHRU->m_area*pSatBelowArgillic->m_depth);
         if (wcSatBelowArgillic > phiGw)
            {
            satGWToUnsatGW = (float)pSatBelowArgillic->m_volumeWater*(wcSatBelowArgillic - phiGw) / pHRU->m_area / dt;
            wcSatBelowArgillic = phiGw;
  //          gwUnsatToSat = 0.0f; //saturated, so no flow in from above
            }
         totalDepth = pUnsat->m_depth + pSatAboveArgillic->m_depth + pArgillic->m_depth+pUnBelowArgillic->m_depth + pSatBelowArgillic->m_depth;
         float waterDepth = 0.0f;
       //  for (int i = 0; i < pHRU->GetPoolCount(); i++)
        //    waterDepth += pHRU->GetPool(i)->m_wc;
        
         pSatBelowArgillic->m_wc = wcSatBelowArgillic;//mm of water - should be saturated?
         pSatBelowArgillic->m_wDepth = wcSatBelowArgillic*pSatBelowArgillic->m_depth / phiGw*1000.0f;//mm of water
         gwf = CalcSatWaterFluxes(pHRU, pSatBelowArgillic, phiGw, pleGw, kSatGw, satGWToUnsatGW, minElev, totalDepth, wcSatBelowArgillic, pGWParam_Table, pFlowContext);
         if (pHRU->m_lowestElevationHRU && gwf>0.0f)
            gwOutOfCatchment = gwf*0.01f;
         float deepGWLoss = (float)pSatBelowArgillic->m_volumeWater*0.005f;

         waterDepth = pSatBelowArgillic->m_wDepth;

         if (isSat5)
            {
            waterDepth += pUnBelowArgillic->m_wDepth;
           // argillicToUnsat = 0.0f;//saturated, so no flow in from above
            if (isSat4)
               {
               waterDepth += pArgillic->m_wDepth;
               //satToArgillic = 0.0f;//saturated, so no flow in from above
               if (isSat3)
                  {
                  waterDepth += pSatAboveArgillic->m_wDepth;
                 // unsatToSat = 0.0f;//saturated, so no flow in from above
                  if (isSat2)
                     waterDepth += pUnsat->m_wDepth;
                  }
               }
            }
         waterDepth += (pUnBelowArgillic->m_wDepth + pArgillic->m_wDepth + pSatAboveArgillic->m_wDepth);
         pHRU->m_elws = pHRU->m_elevation - totalDepth + (waterDepth / 1000.0f);//m
   //End Saturated Below Argillic

        // if (soilToStream*1000.0f > 500.0f)
         //   soilToStream = 500.0f / 1000.0f;

         pHRU->m_currentRunoff = soilToStream*1000.0f ; // mm/d
         
         pHRU->m_currentGWRecharge = (argillicToUnsat-unsatGWToArgillic)*1000.0f;  // mm/d
         pHRU->m_currentGWFlowOut = (gwOutOfCatchment + shallowGWOutOfCatchment)*1000.0f/pHRU->m_area;  // mm/d

         pUnsat->AddFluxFromGlobalHandler(precip*pHRU->m_area, FL_TOP_SOURCE);
         pUnsat->AddFluxFromGlobalHandler(unsatToSat*pHRU->m_area, FL_BOTTOM_SINK);
         pUnsat->AddFluxFromGlobalHandler(soilToStream*pHRU->m_area, FL_STREAM_SINK);
         pUnsat->AddFluxFromGlobalHandler(satToUnsat*pHRU->m_area, FL_BOTTOM_SOURCE);

         pSatAboveArgillic->AddFluxFromGlobalHandler(unsatToSat*pHRU->m_area, FL_TOP_SOURCE);
         pSatAboveArgillic->AddFluxFromGlobalHandler(satToUnsat*pHRU->m_area, FL_TOP_SINK);
         pSatAboveArgillic->AddFluxFromGlobalHandler(satToArgillic*pHRU->m_area, FL_BOTTOM_SINK);
         pSatAboveArgillic->AddFluxFromGlobalHandler(ssf, FL_USER_DEFINED);
         pSatAboveArgillic->AddFluxFromGlobalHandler(argillicToSat*pHRU->m_area, FL_BOTTOM_SOURCE);     
         
         pArgillic->AddFluxFromGlobalHandler(satToArgillic*pHRU->m_area, FL_TOP_SOURCE);
         pArgillic->AddFluxFromGlobalHandler(argillicToUnsat*pHRU->m_area, FL_BOTTOM_SINK);
         pArgillic->AddFluxFromGlobalHandler(unsatGWToArgillic*pHRU->m_area, FL_BOTTOM_SOURCE);
         pArgillic->AddFluxFromGlobalHandler(argillicToSat*pHRU->m_area, FL_TOP_SINK);

         pUnBelowArgillic->AddFluxFromGlobalHandler(argillicToUnsat*pHRU->m_area, FL_TOP_SOURCE);
         pUnBelowArgillic->AddFluxFromGlobalHandler(gwUnsatToSat*pHRU->m_area, FL_BOTTOM_SINK);
         pUnBelowArgillic->AddFluxFromGlobalHandler(satGWToUnsatGW*pHRU->m_area, FL_BOTTOM_SOURCE);
         pUnBelowArgillic->AddFluxFromGlobalHandler(unsatGWToArgillic*pHRU->m_area, FL_TOP_SINK);

         pSatBelowArgillic->AddFluxFromGlobalHandler(gwUnsatToSat*pHRU->m_area, FL_TOP_SOURCE);
         pSatBelowArgillic->AddFluxFromGlobalHandler(gwf, FL_USER_DEFINED);
         pSatBelowArgillic->AddFluxFromGlobalHandler(satGWToUnsatGW*pHRU->m_area, FL_TOP_SINK);
         pSatBelowArgillic->AddFluxFromGlobalHandler(gwOutOfCatchment , FL_BOTTOM_SINK);
         
         Reach * pReach = pUnsat->GetReach();
         if (pReach)
            pReach->AddFluxFromGlobalHandler(soilToStream * pHRU->m_area); //m3/d
         }

      return 0.0f;
      }
      /*
      float Flow_Grid::SRS_6_Layer(FlowContext *pFlowContext)
      {
         if (pFlowContext->timing & 1) // Init()
            return Flow_Grid::Init_Grid_Fluxes(pFlowContext);

         int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
         ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable("GRID");   // store this pointer (and check to make sure it's not NULL)
         ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable("GWParams");
         ParamTable *pArgillicParam_Table = pFlowContext->pFlowModel->GetTable("ArgillicParams");
         int hruCount = pFlowContext->pFlowModel->GetHRUCount();
         float minElev = pFlowContext->pFlowModel->m_minElevation;


         int hruIndex = 0;

#pragma omp parallel for
         for (int h = 0; h < hruCount; h++)
         {
            HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
            int hruLayerCount = pHRU->GetPoolCount();

            float airTemp = 0.0f; float precip = 0.0f;
            float time = pFlowContext->time - (int)pFlowContext->time;
            float currTime = (float)pFlowContext->dayOfYear + time;
            float dt = pFlowContext->timeStep;
            //dt = 1.0f;

            pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, (int)currTime, precip);//mm/d


            pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, (int)currTime, airTemp);//C 
            float wp = 0.1f; float phi = 0.33f; float lamda = 0.1f; float kSat = 1.0f; float ple = 2.f;
            float kSatGw = 1.0f; float pleGw = 2.f; float phiGw = 0.33f; float fcPct = 0.5f; float kSatArg = 0.1f;
            float wpArg = 10.0f; float lamdaArg = 0.1f; float phiArg = 0.3f;

            VData lulcA = 0;
            pFlowContext->pFlowModel->GetHRUData(pHRU, pLULC_Table->m_iduCol, lulcA, DAM_FIRST);  // get HRU info
            if (pLULC_Table->m_type == DOT_FLOAT)
               lulcA.ChangeType(TYPE_FLOAT);
            bool ok = pLULC_Table->Lookup(lulcA, m_col_wp, wp);     // wilting point
            ok = pLULC_Table->Lookup(lulcA, m_col_phi, phi);        // porosity
            ok = pLULC_Table->Lookup(lulcA, m_col_lamda, lamda);    // pore size distribution index
            ok = pLULC_Table->Lookup(lulcA, m_col_kSat, kSat);      // saturated hydraulic conductivity    
            ok = pLULC_Table->Lookup(lulcA, m_col_ple, ple);        // power law exponent
            fcPct = 1.0f;

            VData geo = 0;
            pFlowContext->pFlowModel->GetHRUData(pHRU, pGWParam_Table->m_iduCol, geo, DAM_FIRST);  // get HRU info
            if (pGWParam_Table->m_type == DOT_FLOAT)
               geo.ChangeType(TYPE_FLOAT);

            ok = pGWParam_Table->Lookup(geo, m_col_kSatGw, kSatGw);    // saturated hydraulic conductivity    
            ok = pGWParam_Table->Lookup(geo, m_col_pleGw, pleGw);        // power law exponent
            ok = pGWParam_Table->Lookup(geo, m_col_phiGw, phiGw);        // power law exponent

            if (pArgillicParam_Table->m_type == DOT_FLOAT)
               geo.ChangeType(TYPE_FLOAT);

            ok = pArgillicParam_Table->Lookup(geo, m_col_kSatArg, kSatArg);    // saturated hydraulic conductivity    
            ok = pArgillicParam_Table->Lookup(geo, m_col_lamdaArg, lamdaArg);        // power law exponent
            ok = pArgillicParam_Table->Lookup(geo, m_col_wpArg, wpArg);        // power law exponent
            ok = pArgillicParam_Table->Lookup(geo, m_col_phiArg, phiArg);        // power law exponent

            float infiltrationExcess = 0.0f;
            float soilToStream = 0.0f;
            float groundwaterToSoil = 0.0f;
            float unsatToSat = 0.0f;
            float satToArgillic = 0.0;
            float argillicToUnsat = 0.0f;
            float gwUnsatToSat = 0.0f;
            float ssf = 0.0f;
            float gwf = 0.0f;
            float ssUpper = 0.0f;
            float ssLower = 0.0f;
            float shallowGWOutOfCatchment = 0.0f;

            float satToUnsat = 0.0f;
            float argillicToSat = 0.0f;
            float unsatGWToArgillic = 0.0f;
            float satGWToUnsatGW = 0.0f;
            float gwOutOfCatchment = 0.0f;
            float depthOfWater = 0.0f;
            bool isSat1 = false; bool isSat2 = false; bool isSat3 = false; bool isSat4 = false; bool isSat5 = false;
            if (precip > 100.0f)
               precip = 0.0f;
            precip /= 1000.0f;//m/d

            //ponded
            float infiltration = 0.0f;
            float runoff = 0.0f;
            HRUPool *pPonded = pHRU->GetPool(5);
            // runoff = pPonded->m_volumeWater / pHRU->m_area*0.0f;//m/d

            pPonded->m_wc = 1.0;//water content
            pPonded->m_wDepth = pPonded->m_volumeWater*pHRU->m_area / 1000.0f;//mm of water

            float runoffCoefficient = 0.75f;
            infiltration = pPonded->m_volumeWater / pHRU->m_area / dt *runoffCoefficient;//everything infiltrates in this timest
            //Unsaturated
            HRUPool *pUnsat = pHRU->GetPool(0);
            float wcU = (float)pUnsat->m_volumeWater / (pHRU->m_area*pUnsat->m_depth);//of layer 0, unsaturated m3/m3

            float storage = 0.1f;

            if (wcU > wp)
               {
               if (wcU > phi)
                  {
                  soilToStream = (float)pUnsat->m_volumeWater*(wcU - phi) / pHRU->m_area / dt;
                  runoff = infiltration;
                  infiltration = 0.0f;
                  isSat2 = true;
                  wcU = phi;
                  }
               unsatToSat = CalcBrooksCoreyKTheta(wcU, wp, phi, lamda, kSat);
            }
            pUnsat->m_wc = wcU;//
            pUnsat->m_wDepth = wcU*pUnsat->m_depth / phi*1000.0f;//mm of water
            //End Unsaturated

            //Saturated Above Argillic
            HRUPool *pSatAboveArgillic = pHRU->GetPool(1);
            float totalDepth = pUnsat->m_depth + pSatAboveArgillic->m_depth;
            float wcL = (float)pSatAboveArgillic->m_volumeWater / (pHRU->m_area*pSatAboveArgillic->m_depth);
            if (wcL > phi)
               {
               satToUnsat = (float)pSatAboveArgillic->m_volumeWater*(wcL - phi) / pHRU->m_area / dt;
               wcL = phi;
               isSat3 = true;
               unsatToSat = 0.0f;
               }
            pSatAboveArgillic->m_wc = wcL;//water content
            pSatAboveArgillic->m_wDepth = wcL*pSatAboveArgillic->m_depth / phi*1000.0f;//mm of water

            satToArgillic = (float)pSatAboveArgillic->m_volumeWater / pHRU->m_area*0.025f;

            // if (wcL > 0.25f)
            ssf = CalcSatWaterFluxes(pHRU, pSatAboveArgillic, phi, ple, kSat, satToUnsat, minElev, totalDepth, wcL, pLULC_Table, pFlowContext);

            if (pHRU->m_lowestElevationHRU && ssf>0.0f)
               shallowGWOutOfCatchment = ssf;

            //End Saturated Above Argillic

            // Argillic
            HRUPool *pArgillic = pHRU->GetPool(2);
            float wcArgillic = (float)pArgillic->m_volumeWater / (pHRU->m_area*pArgillic->m_depth);
            if (wcArgillic > phiArg)
               {
               argillicToSat = (float)pArgillic->m_volumeWater*(wcArgillic - phiArg) / pHRU->m_area / dt;
               wcArgillic = phiArg;
               satToArgillic = 0.0f;
               isSat4 = true;
               //        satToArgillic = 0.0f;
               }
            pArgillic->m_wc = wcArgillic;//mm of water
            pArgillic->m_wDepth = wcArgillic*pArgillic->m_depth / phiArg*1000.0f;//mm of water

            argillicToUnsat = CalcBrooksCoreyKTheta(wcArgillic, wpArg, phiArg, lamdaArg, kSatArg);
            if (wcL>wp)
               satToArgillic = argillicToUnsat;
            // End Argillic

            // Unsaturated Below Argillic
            HRUPool *pUnBelowArgillic = pHRU->GetPool(3);
            float wcUnBelowArgillic = (float)pUnBelowArgillic->m_volumeWater / (pHRU->m_area*pUnBelowArgillic->m_depth);

            if (wcUnBelowArgillic > phiGw)
               {
               unsatGWToArgillic = (float)pUnBelowArgillic->m_volumeWater*(wcUnBelowArgillic - phiGw) / pHRU->m_area / dt;
               wcUnBelowArgillic = phiGw;
               argillicToUnsat = 0.0f;
               isSat5 = true;
               //       argillicToUnsat = 0.0f;
               }
            pUnBelowArgillic->m_wc = wcUnBelowArgillic;//mm of water
            pUnBelowArgillic->m_wDepth = wcUnBelowArgillic*pUnBelowArgillic->m_depth / phiGw*1000.0f;//mm of water
            gwUnsatToSat = CalcBrooksCoreyKTheta(wcUnBelowArgillic, wp, phiGw, lamda, kSatGw);
            // End Unsaturated Below Argillic

            //Saturated Below Argillic
            HRUPool *pSatBelowArgillic = pHRU->GetPool(4);
            float wcSatBelowArgillic = (float)pSatBelowArgillic->m_volumeWater / (pHRU->m_area*pSatBelowArgillic->m_depth);
            if (wcSatBelowArgillic > phiGw)
               {
               satGWToUnsatGW = (float)pSatBelowArgillic->m_volumeWater*(wcSatBelowArgillic - phiGw) / pHRU->m_area / dt;
               wcSatBelowArgillic = phiGw;
               gwUnsatToSat = 0.0f;
               //          gwUnsatToSat = 0.0f; //saturated, so no flow in from above
               }
            totalDepth = pUnsat->m_depth + pSatAboveArgillic->m_depth + pArgillic->m_depth + pUnBelowArgillic->m_depth + pSatBelowArgillic->m_depth + pPonded->m_depth;
            float waterDepth = 0.0f;
            //  for (int i = 0; i < pHRU->GetPoolCount(); i++)
            //    waterDepth += pHRU->GetPool(i)->m_wc;

            pSatBelowArgillic->m_wc = wcSatBelowArgillic;//mm of water - should be saturated?
            pSatBelowArgillic->m_wDepth = wcSatBelowArgillic*pSatBelowArgillic->m_depth / phiGw*1000.0f;//mm of water
            gwf = CalcSatWaterFluxes(pHRU, pSatBelowArgillic, phiGw, pleGw, kSatGw, satGWToUnsatGW, minElev, totalDepth, wcSatBelowArgillic, pGWParam_Table, pFlowContext);
            if (pHRU->m_lowestElevationHRU && gwf>0.0f)
               gwOutOfCatchment = gwf;
            float deepGWLoss = (float)pSatBelowArgillic->m_volumeWater*0.0001f;

            waterDepth = pSatBelowArgillic->m_wDepth;

          
            waterDepth += (pUnBelowArgillic->m_wDepth + pArgillic->m_wDepth + pSatAboveArgillic->m_wDepth);
            pHRU->m_elws = pHRU->m_elevation - totalDepth + (waterDepth / 1000.0f);//m

            //End Saturated Below Argillic

            // if (soilToStream*1000.0f > 500.0f)
            //   soilToStream = 500.0f / 1000.0f;

            pHRU->m_currentRunoff += runoff*1000.0f*dt; // mm

            pHRU->m_currentGWRecharge = (argillicToUnsat - unsatGWToArgillic)*1000.0f;  // mm/d
            pHRU->m_currentGWFlowOut += (gwOutOfCatchment + shallowGWOutOfCatchment)*1000.0f / pHRU->m_area*dt;  // mm/d

            pPonded->AddFluxFromGlobalHandler((precip+soilToStream)*pHRU->m_area, FL_TOP_SOURCE);
            pPonded->AddFluxFromGlobalHandler(infiltration*pHRU->m_area, FL_BOTTOM_SINK);
            pPonded->AddFluxFromGlobalHandler(runoff*pHRU->m_area, FL_STREAM_SINK);

            pUnsat->AddFluxFromGlobalHandler(infiltration*pHRU->m_area, FL_TOP_SOURCE);
            pUnsat->AddFluxFromGlobalHandler(unsatToSat*pHRU->m_area, FL_BOTTOM_SINK);
            pUnsat->AddFluxFromGlobalHandler(soilToStream*pHRU->m_area, FL_STREAM_SINK);
            pUnsat->AddFluxFromGlobalHandler(satToUnsat*pHRU->m_area, FL_BOTTOM_SOURCE);

            pSatAboveArgillic->AddFluxFromGlobalHandler(unsatToSat*pHRU->m_area, FL_TOP_SOURCE);
            pSatAboveArgillic->AddFluxFromGlobalHandler(satToUnsat*pHRU->m_area, FL_TOP_SINK);
            pSatAboveArgillic->AddFluxFromGlobalHandler(satToArgillic*pHRU->m_area, FL_BOTTOM_SINK);
            pSatAboveArgillic->AddFluxFromGlobalHandler(ssf, FL_USER_DEFINED);
            pSatAboveArgillic->AddFluxFromGlobalHandler(argillicToSat*pHRU->m_area, FL_BOTTOM_SOURCE);
            pSatAboveArgillic->AddFluxFromGlobalHandler(shallowGWOutOfCatchment, FL_BOTTOM_SINK);

            pArgillic->AddFluxFromGlobalHandler(satToArgillic*pHRU->m_area, FL_TOP_SOURCE);
            pArgillic->AddFluxFromGlobalHandler(argillicToUnsat*pHRU->m_area, FL_BOTTOM_SINK);
            pArgillic->AddFluxFromGlobalHandler(unsatGWToArgillic*pHRU->m_area, FL_BOTTOM_SOURCE);
            pArgillic->AddFluxFromGlobalHandler(argillicToSat*pHRU->m_area, FL_TOP_SINK);

            pUnBelowArgillic->AddFluxFromGlobalHandler(argillicToUnsat*pHRU->m_area, FL_TOP_SOURCE);
            pUnBelowArgillic->AddFluxFromGlobalHandler(gwUnsatToSat*pHRU->m_area, FL_BOTTOM_SINK);
            pUnBelowArgillic->AddFluxFromGlobalHandler(satGWToUnsatGW*pHRU->m_area, FL_BOTTOM_SOURCE);
            pUnBelowArgillic->AddFluxFromGlobalHandler(unsatGWToArgillic*pHRU->m_area, FL_TOP_SINK);

            pSatBelowArgillic->AddFluxFromGlobalHandler(gwUnsatToSat*pHRU->m_area, FL_TOP_SOURCE);
            pSatBelowArgillic->AddFluxFromGlobalHandler(gwf, FL_USER_DEFINED);
            pSatBelowArgillic->AddFluxFromGlobalHandler(satGWToUnsatGW*pHRU->m_area, FL_TOP_SINK);
            pSatBelowArgillic->AddFluxFromGlobalHandler(gwOutOfCatchment, FL_BOTTOM_SINK);
            pSatBelowArgillic->AddFluxFromGlobalHandler(deepGWLoss, FL_BOTTOM_SINK);
            Reach * pReach = pUnsat->GetReach();
            if (pReach)
               pReach->AddFluxFromGlobalHandler(runoff * pHRU->m_area); //m3/d
         }

         return 0.0f;
      }
      
   float Flow_Grid::AdjustForSaturation(FlowContext *pFlowContext)
      {
      float infiltrationExcess = 0.0f;
      float soilToStream = 0.0f;
      float groundwaterToSoil = 0.0f;
      float unsatToSat = 0.0f;
      float satToArgillic = 0.0;
      float argillicToUnsat = 0.0f;
      float gwUnsatToSat = 0.0f;
      float ssf = 0.0f;
      float gwf = 0.0f;
      float ssUpper = 0.0f;
      float ssLower = 0.0f;
      float shallowGWOutOfCatchment = 0.0f;

      float satToUnsat = 0.0f;
      float argillicToSat = 0.0f;
      float unsatGWToArgillic = 0.0f;
      float satGWToUnsatGW = 0.0f;
      float gwOutOfCatchment = 0.0f;
      float depthOfWater = 0.0f;
      float rechargeAccounting = 0.0f; float rechargeAccountingArgillic = 0.0f;
      bool isSat1 = false; bool isSat2 = false; bool isSat3 = false; bool isSat4 = false; bool isSat5 = false;
      bool isSatDeepGround = false; bool isSatShallowGround = false; bool isUnsatShallowGround = false;
      int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
      ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable("GRID");   // store this pointer (and check to make sure it's not NULL)
      ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable("GWParams");
      ParamTable *pArgillicParam_Table = pFlowContext->pFlowModel->GetTable("ArgillicParams");
      int hruCount = pFlowContext->pFlowModel->GetHRUCount();

      for (int h = 0; h < hruCount; h++)
         {
         HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
         int hruLayerCount = pHRU->GetPoolCount();  
         float wp = 0.1f; float phi = 0.33f; float lamda = 0.1f; float kSat = 1.0f; float ple = 2.f;
         float kSatGw = 1.0f; float pleGw = 2.f; float phiGw = 0.33f; float fcPct = 0.5f; float kSatArg = 0.1f;
         float wpArg = 10.0f; float lamdaArg = 0.1f; float phiArg = 0.3f;

         VData lulcA = 0;
         pFlowContext->pFlowModel->GetHRUData(pHRU, pLULC_Table->m_iduCol, lulcA, DAM_FIRST);  // get HRU info
         if (pLULC_Table->m_type == DOT_FLOAT)
            lulcA.ChangeType(TYPE_FLOAT);
         bool ok = pLULC_Table->Lookup(lulcA, m_col_wp, wp);     // wilting point
         ok = pLULC_Table->Lookup(lulcA, m_col_phi, phi);        // porosity
         ok = pLULC_Table->Lookup(lulcA, m_col_lamda, lamda);    // pore size distribution index
         ok = pLULC_Table->Lookup(lulcA, m_col_kSat, kSat);      // saturated hydraulic conductivity    
         ok = pLULC_Table->Lookup(lulcA, m_col_ple, ple);        // power law exponent
         fcPct = 1.0f;

         VData geo = 0;
         pFlowContext->pFlowModel->GetHRUData(pHRU, pGWParam_Table->m_iduCol, geo, DAM_FIRST);  // get HRU info
         if (pGWParam_Table->m_type == DOT_FLOAT)
            geo.ChangeType(TYPE_FLOAT);
         ok = pGWParam_Table->Lookup(geo, m_col_kSatGw, kSatGw);    // saturated hydraulic conductivity    
         ok = pGWParam_Table->Lookup(geo, m_col_pleGw, pleGw);        // power law exponent
         ok = pGWParam_Table->Lookup(geo, m_col_phiGw, phiGw);        // power law exponent

         if (pArgillicParam_Table->m_type == DOT_FLOAT)
            geo.ChangeType(TYPE_FLOAT);
         ok = pArgillicParam_Table->Lookup(geo, m_col_kSatArg, kSatArg);    // saturated hydraulic conductivity    
         ok = pArgillicParam_Table->Lookup(geo, m_col_lamdaArg, lamdaArg);        // power law exponent
         ok = pArgillicParam_Table->Lookup(geo, m_col_wpArg, wpArg);        // power law exponent
         ok = pArgillicParam_Table->Lookup(geo, m_col_phiArg, phiArg);        // power law exponent
    
         HRUPool *pUnsat = pHRU->GetPool(0);
         HRUPool *pSatAboveArgillic = pHRU->GetPool(1);
         HRUPool *pArgillic = pHRU->GetPool(2);
         HRUPool *pUnBelowArgillic = pHRU->GetPool(3);
         HRUPool *pSatBelowArgillic = pHRU->GetPool(4);
         HRUPool *pPonded = pHRU->GetPool(5);

         //Saturated Below Argillic
         pSatBelowArgillic->m_wDepth = (float)pSatBelowArgillic->m_volumeWater / pHRU->m_area / phiGw;//m
         float totalDepthLower = (pSatBelowArgillic->m_depth + (pUnBelowArgillic->m_depth*0.9f));
         if (pSatBelowArgillic->m_wDepth > totalDepthLower) // if depth of water is greater than groundwater zone depth, move excess to argillic
            {
           // rechargeAccounting = (pSatBelowArgillic->m_wDepth - totalDepthLower)*pHRU->m_area*phiGw;
            rechargeAccounting = pSatBelowArgillic->m_volumeWater - (totalDepthLower*pHRU->m_area*phiGw);
          //  if (rechargeAccounting > pSatBelowArgillic->m_volumeWater)
           //    rechargeAccounting = pSatBelowArgillic->m_volumeWater;
            pUnBelowArgillic->m_volumeWater += rechargeAccounting;
            pSatBelowArgillic->m_volumeWater -= rechargeAccounting;
            pSatBelowArgillic->m_wDepth = totalDepthLower;
            isSatDeepGround = true;



            }

         pSatBelowArgillic->m_wc = phiGw;//mm of water - should be saturated?
         float waterDepthLowerSat = pSatBelowArgillic->m_wDepth;//m

         // Unsaturated Below Argillic
         float waterDepthLowerUnsat = pSatBelowArgillic->m_depth + pUnBelowArgillic->m_depth - waterDepthLowerSat;//total depth of the unsaturated zone below the argillic
         float wcUnBelowArgillic = phiGw;
         if (waterDepthLowerUnsat>0.0f)
            wcUnBelowArgillic = (float)pUnBelowArgillic->m_volumeWater / (pHRU->m_area*waterDepthLowerUnsat);

         if (wcUnBelowArgillic > phiGw) //if the lower unsaturated zone is over saturation, move water to argillic
            {
           // rechargeAccountingArgillic = (wcUnBelowArgillic - phiGw) * pUnBelowArgillic->m_volumeWater;
            rechargeAccountingArgillic = pUnBelowArgillic->m_volumeWater - (waterDepthLowerUnsat*pHRU->m_area*phiGw);
           // if (rechargeAccountingArgillic > pUnBelowArgillic->m_volumeWater)
          //     rechargeAccountingArgillic = pUnBelowArgillic->m_volumeWater;
            pArgillic->m_volumeWater += rechargeAccountingArgillic;
            pUnBelowArgillic->m_volumeWater -= rechargeAccountingArgillic;
            wcUnBelowArgillic = phiGw;
            isSat5 = true;
            }
         pUnBelowArgillic->m_wc = wcUnBelowArgillic;//
         pUnBelowArgillic->m_wDepth = wcUnBelowArgillic*waterDepthLowerUnsat / phiGw;//mm of saturated water (but remember this is an unsaturated zone)
         gwUnsatToSat = CalcBrooksCoreyKTheta(wcUnBelowArgillic, wp, phiGw, lamda, kSat);
         // End Unsaturated Below Argillic
         totalDepthLower += pUnsat->m_depth + pSatAboveArgillic->m_depth + pArgillic->m_depth;

         // Argillic
         float wcArgillic = (float)pArgillic->m_volumeWater / (pHRU->m_area*pArgillic->m_depth);

         if (wcArgillic > phiArg) //over saturated
            {
            float excess = pArgillic->m_volumeWater - (pArgillic->m_depth*pHRU->m_area*phiArg);
            //float excess = (wcArgillic - phiArg) * pArgillic->m_volumeWater;
            if (excess > pArgillic->m_volumeWater)
               excess = pArgillic->m_volumeWater;
            pSatAboveArgillic->m_volumeWater += excess;
            pArgillic->m_volumeWater -= excess;
            wcArgillic = phiArg;
            //  argillicToUnsat *= 0.03f;
            isSat4 = true;
            }
         argillicToUnsat = CalcBrooksCoreyKTheta(wcArgillic, wpArg, phiArg, lamdaArg, kSatArg);
         pArgillic->m_wc = wcArgillic;//mm of water
         pArgillic->m_wDepth = pArgillic->m_volumeWater / pHRU->m_area / phiArg;

         // End Argillic

         //Saturated Above Argillic
         float infiltration = 0.0f;
         float runoff = 0.0f;

         float totalDepthUpper = (pUnsat->m_depth*0.5f) + pSatAboveArgillic->m_depth;
         float wcL = phi;
         pSatAboveArgillic->m_wc = wcL;//water content         

         if (pSatAboveArgillic->m_volumeWater > (totalDepthUpper*pHRU->m_area*phi)) //sat+unsat depth is fully saturated with water
            {
            float exchange = pSatAboveArgillic->m_volumeWater - (totalDepthUpper*pHRU->m_area*phi);
        //    if (exchange > pSatAboveArgillic->m_volumeWater)
         //      exchange = pSatAboveArgillic->m_volumeWater;
            pUnsat->m_volumeWater += exchange;
            pSatAboveArgillic->m_volumeWater -= exchange;
            pSatAboveArgillic->m_wDepth = totalDepthUpper;
            isSatShallowGround = true;
            }
         else
            pSatAboveArgillic->m_wDepth = (float)pSatAboveArgillic->m_volumeWater / pHRU->m_area / phi; //m
         //End Saturated Above Argillic

         //Unsaturated
         float wcU = phi;

         float waterDepthUpperUnsat = pUnsat->m_depth + pSatAboveArgillic->m_depth - pSatAboveArgillic->m_wDepth;//total depth of the zone above the argillic
         wcU = (float)pUnsat->m_volumeWater / (pHRU->m_area*waterDepthUpperUnsat);

         if (wcU > wp)
            {
            if (wcU >= phi)//greater than field capacity
               {
               float excess = pUnsat->m_volumeWater - (waterDepthUpperUnsat * pHRU->m_area * phi);
              // float excess = (wcU - phi) * pUnsat->m_volumeWater;
            //   if (excess > pUnsat->m_volumeWater)
            //      excess = pUnsat->m_volumeWater;
               pPonded->m_volumeWater += excess*0.05f;
               pUnsat->m_volumeWater -= excess*0.05f;
               ASSERT(pUnsat->m_volumeWater>0.0f);
               wcU = phi;
               isUnsatShallowGround = true;
               }
            }
         pUnsat->m_wDepth = pUnsat->m_volumeWater / pHRU->m_area / phi;//m of water without soil.  total depth is waterDepthUpperUnsat!
         pUnsat->m_wc = wcU;//
         //End Unsaturated

         //ponded
         pPonded->m_wc = 0.3;//water content
         pPonded->m_wDepth = pPonded->m_volumeWater / pHRU->m_area;//m of water


         pHRU->m_elws = pHRU->m_elevation - totalDepthLower + waterDepthLowerSat ;//m
         pPonded->m_wDepth *= 1000.0f;
         pUnsat->m_wDepth *= 1000.0f;
         pSatAboveArgillic->m_wDepth *= 1000.0f;
         pArgillic->m_wDepth *= 1000.0f;
         pUnBelowArgillic->m_wDepth *= 1000.0f;
         pSatBelowArgillic->m_wDepth *= 1000.0f;

         pHRU->m_currentGWRecharge = (argillicToUnsat - (rechargeAccountingArgillic / pHRU->m_area))*1000.0f;  // mm/d
         }
      return -1;
      }
      */

   float Flow_Grid::AdjustForSaturation_Rates(FlowContext *pFlowContext)
      {
         float infiltrationExcess = 0.0f;
         float soilToStream = 0.0f;
         float groundwaterToSoil = 0.0f;
         float unsatToSat = 0.0f;
         float satToArgillic = 0.0;
         float argillicToUnsat = 0.0f;
         float gwUnsatToSat = 0.0f;
         float ssf = 0.0f;
         float gwf = 0.0f;
         float ssUpper = 0.0f;
         float ssLower = 0.0f;
         float shallowGWOutOfCatchment = 0.0f;

         float satToUnsat = 0.0f;
         float argillicToSat = 0.0f;
         float unsatGWToArgillic = 0.0f;
         float satGWToUnsatGW = 0.0f;
         float gwOutOfCatchment = 0.0f;
         float depthOfWater = 0.0f;
         float rechargeAccounting = 0.0f; float rechargeAccountingArgillic = 0.0f;
         bool isSat1 = false; bool isSat2 = false; bool isSat3 = false; bool isSat4 = false; bool isSat5 = false;
         bool isSatDeepGround = false; bool isSatShallowGround = false; bool isUnsatShallowGround = false;
         int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
         ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable("GRID");   // store this pointer (and check to make sure it's not NULL)
         ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable("GWParams");
         ParamTable *pArgillicParam_Table = pFlowContext->pFlowModel->GetTable("ArgillicParams");
         int hruCount = pFlowContext->pFlowModel->GetHRUCount();
         float dt = pFlowContext->timeStep;
     //   #pragma omp parallel for
         for (int h = 0; h < hruCount; h++)
         {
            HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
            int hruLayerCount = pHRU->GetPoolCount();
            float wp = 0.1f; float phi = 0.33f; float lamda = 0.1f; float kSat = 1.0f; float ple = 2.f;
            float kSatGw = 1.0f; float pleGw = 2.f; float phiGw = 0.33f; float fcPct = 0.5f; float kSatArg = 0.1f;
            float wpArg = 10.0f; float lamdaArg = 0.1f; float phiArg = 0.3f;float ic=1;

            VData lulcA = 0;
            pFlowContext->pFlowModel->GetHRUData(pHRU, pLULC_Table->m_iduCol, lulcA, DAM_FIRST);  // get HRU info
            if (pLULC_Table->m_type == DOT_FLOAT)
               lulcA.ChangeType(TYPE_FLOAT);
            bool ok = pLULC_Table->Lookup(lulcA, m_col_wp, wp);     // wilting point
            ok = pLULC_Table->Lookup(lulcA, m_col_phi, phi);        // porosity
            ok = pLULC_Table->Lookup(lulcA, m_col_lamda, lamda);    // pore size distribution index
            ok = pLULC_Table->Lookup(lulcA, m_col_kSat, kSat);      // saturated hydraulic conductivity    
            ok = pLULC_Table->Lookup(lulcA, m_col_ple, ple);        // power law exponent
            ok = pLULC_Table->Lookup(lulcA, m_col_ic, ic);
            fcPct = 1.0f;

            VData geo = 0;
            pFlowContext->pFlowModel->GetHRUData(pHRU, pGWParam_Table->m_iduCol, geo, DAM_FIRST);  // get HRU info
            if (pGWParam_Table->m_type == DOT_FLOAT)
               geo.ChangeType(TYPE_FLOAT);
            ok = pGWParam_Table->Lookup(geo, m_col_kSatGw, kSatGw);    // saturated hydraulic conductivity    
            ok = pGWParam_Table->Lookup(geo, m_col_pleGw, pleGw);        // power law exponent
            ok = pGWParam_Table->Lookup(geo, m_col_phiGw, phiGw);        // power law exponent

            if (pArgillicParam_Table->m_type == DOT_FLOAT)
               geo.ChangeType(TYPE_FLOAT);
            ok = pArgillicParam_Table->Lookup(geo, m_col_kSatArg, kSatArg);    // saturated hydraulic conductivity    
            ok = pArgillicParam_Table->Lookup(geo, m_col_lamdaArg, lamdaArg);        // power law exponent
            ok = pArgillicParam_Table->Lookup(geo, m_col_wpArg, wpArg);        // power law exponent
            ok = pArgillicParam_Table->Lookup(geo, m_col_phiArg, phiArg);        // power law exponent

            HRUPool *pUnsat = pHRU->GetPool(0);
            HRUPool *pSatAboveArgillic = pHRU->GetPool(1);
            HRUPool *pArgillic = pHRU->GetPool(2);
            HRUPool *pUnBelowArgillic = pHRU->GetPool(3);
            HRUPool *pSatBelowArgillic = pHRU->GetPool(4);
            HRUPool *pPonded = pHRU->GetPool(5);

            //Saturated Below Argillic
            float m_wDepthGW = (float)pSatBelowArgillic->m_volumeWater / pHRU->m_area / phiGw;//m
            float totalDepthLower = (pSatBelowArgillic->m_depth /*+ (pUnBelowArgillic->m_depth*0.5)*/);
            if (m_wDepthGW > totalDepthLower) // if depth of water is greater than groundwater zone depth, move excess to argillic
               {
               // rechargeAccounting = (pSatBelowArgillic->m_wDepth - totalDepthLower)*pHRU->m_area*phiGw;
               rechargeAccounting = pSatBelowArgillic->m_volumeWater - (totalDepthLower*pHRU->m_area*phiGw);
               //  if (rechargeAccounting > pSatBelowArgillic->m_volumeWater)
               //    rechargeAccounting = pSatBelowArgillic->m_volumeWater;
               pUnBelowArgillic->m_volumeWater += rechargeAccounting;
               pSatBelowArgillic->m_volumeWater -= rechargeAccounting;
//               pUnBelowArgillic->AddFluxFromGlobalHandler(rechargeAccounting / dt, FL_BOTTOM_SOURCE);
//               pSatBelowArgillic->AddFluxFromGlobalHandler(rechargeAccounting / dt, FL_TOP_SINK);
               m_wDepthGW = totalDepthLower;
               isSatDeepGround = true;
               }

            pSatBelowArgillic->m_wc = phiGw;//mm of water - should be saturated?
            pSatBelowArgillic->m_wDepth = m_wDepthGW;
            float waterDepthLowerSat = pSatBelowArgillic->m_wDepth;//m

            // Unsaturated Below Argillic
            float waterDepthLowerUnsat = pUnBelowArgillic->m_depth;// pSatBelowArgillic->m_depth + pUnBelowArgillic->m_depth - waterDepthLowerSat;//total depth of the unsaturated zone below the argillic
            float wcUnBelowArgillic = phiGw;
            if (waterDepthLowerUnsat>0.0f)
               wcUnBelowArgillic = (float)pUnBelowArgillic->m_volumeWater / (pHRU->m_area*waterDepthLowerUnsat);

            if (wcUnBelowArgillic > phiGw) //if the lower unsaturated zone is over saturation, move water to argillic
               {
               // rechargeAccountingArgillic = (wcUnBelowArgillic - phiGw) * pUnBelowArgillic->m_volumeWater;
               rechargeAccountingArgillic = pUnBelowArgillic->m_volumeWater - (waterDepthLowerUnsat*pHRU->m_area*phiGw);//m3/timestep
               // if (rechargeAccountingArgillic > pUnBelowArgillic->m_volumeWater)
               //     rechargeAccountingArgillic = pUnBelowArgillic->m_volumeWater;
               pArgillic->m_volumeWater += rechargeAccountingArgillic;
               pUnBelowArgillic->m_volumeWater -= rechargeAccountingArgillic;
              // pArgillic->AddFluxFromGlobalHandler(rechargeAccountingArgillic / dt, FL_BOTTOM_SOURCE);//m3/d
              // pUnBelowArgillic->AddFluxFromGlobalHandler(rechargeAccountingArgillic / dt, FL_TOP_SINK);
               wcUnBelowArgillic = phiGw;
               isSat5 = true;
               }
            pUnBelowArgillic->m_wc = wcUnBelowArgillic;//
            pUnBelowArgillic->m_wDepth = wcUnBelowArgillic*waterDepthLowerUnsat / phiGw;//mm of saturated water (but remember this is an unsaturated zone)
           // gwUnsatToSat = CalcBrooksCoreyKTheta(wcUnBelowArgillic, wp, phiGw, lamda, kSat);
            // End Unsaturated Below Argillic
            totalDepthLower += pUnsat->m_depth + pSatAboveArgillic->m_depth + pArgillic->m_depth;

            // Argillic
            float wcArgillic = (float)pArgillic->m_volumeWater / (pHRU->m_area*pArgillic->m_depth);
            float m_wDepth = pArgillic->m_volumeWater / pHRU->m_area / phiArg;
            if (wcArgillic > phiArg) //over saturated
               {
               float excess = pArgillic->m_volumeWater - (pArgillic->m_depth*pHRU->m_area*phiArg);
               //float excess = (wcArgillic - phiArg) * pArgillic->m_volumeWater;
               if (excess > pArgillic->m_volumeWater)
                  excess = pArgillic->m_volumeWater;
               pSatAboveArgillic->m_volumeWater += excess;
               pArgillic->m_volumeWater -= excess;
               //pSatAboveArgillic->AddFluxFromGlobalHandler(excess / dt, FL_BOTTOM_SOURCE);
               //pArgillic->AddFluxFromGlobalHandler(excess / dt, FL_TOP_SINK);
               wcArgillic = phiArg;
               m_wDepth = pArgillic->m_depth;
               //  argillicToUnsat *= 0.03f;
               isSat4 = true;
               }
            argillicToUnsat = CalcBrooksCoreyKTheta(wcArgillic, wpArg, phiArg, lamdaArg, kSatArg);
            pArgillic->m_wc = wcArgillic;//mm of water
            pArgillic->m_wDepth = m_wDepth;

            // End Argillic

            //Saturated Above Argillic
            float infiltration = 0.0f;
            float runoff = 0.0f;

            float totalDepthUpper =/* (pUnsat->m_depth*0.5f) + */pSatAboveArgillic->m_depth;
            float wcL = phi;
            pSatAboveArgillic->m_wc = wcL;//water content         

            if (pSatAboveArgillic->m_volumeWater > (totalDepthUpper*pHRU->m_area)) //sat+unsat depth is fully saturated with water
               {
               float exchange = pSatAboveArgillic->m_volumeWater - (totalDepthUpper*pHRU->m_area*phi);
               //    if (exchange > pSatAboveArgillic->m_volumeWater)
               //      exchange = pSatAboveArgillic->m_volumeWater;
               pUnsat->m_volumeWater += exchange;
               pSatAboveArgillic->m_volumeWater -= exchange;
               
               pSatAboveArgillic->m_wDepth = totalDepthUpper;

               //pUnsat->AddFluxFromGlobalHandler(exchange / dt, FL_BOTTOM_SOURCE);
               //pSatAboveArgillic->AddFluxFromGlobalHandler(exchange / dt, FL_TOP_SINK);
               isSatShallowGround = true;
               }
            else
               pSatAboveArgillic->m_wDepth = (float)pSatAboveArgillic->m_volumeWater / pHRU->m_area / phi; //m
            //End Saturated Above Argillic

            //Unsaturated
            float wcU = phi;

            float waterDepthUpperUnsat = pUnsat->m_depth ;/*+ pSatAboveArgillic->m_depth - pSatAboveArgillic->m_wDepth*/;//total depth of the zone above the argillic
            m_wDepth = pUnsat->m_volumeWater / pHRU->m_area / phi;
            wcU = (float)pUnsat->m_volumeWater / (pHRU->m_area*waterDepthUpperUnsat);

            if (wcU > wp)
               {
               if (wcU >= phi)//greater than field capacity
                  {
                  float excess = pUnsat->m_volumeWater - (waterDepthUpperUnsat * pHRU->m_area * phi);
                  //pUnsat->m_volumeWater -=excess;                
                  //pPonded->m_volumeWater += excess;
                 
                 if (excess > ic/1000.0f)
                    { 
                    pUnsat->m_volumeWater -= pHRU->m_area*ic/1000.0f;//lose rate at ic mm/day
                    pPonded->m_volumeWater += pHRU->m_area*ic / 1000.0f;
                    //pUnsat->AddFluxFromGlobalHandler(pHRU->m_area*ic / 1000.0f / dt, FL_BOTTOM_SOURCE);
                    //pPonded->AddFluxFromGlobalHandler(pHRU->m_area*ic / 1000.0f / dt, FL_TOP_SINK);
                    }
                 else
                    {                  
                    pUnsat->m_volumeWater -=excess;                
                    pPonded->m_volumeWater += excess;                     
                   // pUnsat->AddFluxFromGlobalHandler(excess / dt, FL_BOTTOM_SOURCE);
                    //pPonded->AddFluxFromGlobalHandler(excess / dt, FL_TOP_SINK);

                    }
                  ASSERT(pUnsat->m_volumeWater>0.0f);
                  wcU = phi;
                  m_wDepth= pUnsat->m_depth;
                  isUnsatShallowGround = true;
                  }
               }
            pUnsat->m_wDepth = m_wDepth;//m of water without soil.  total depth is waterDepthUpperUnsat!
            pUnsat->m_wc = wcU;//
            //End Unsaturated

            //ponded
            pPonded->m_wc = 0.3;//water content
            pPonded->m_wDepth = pPonded->m_volumeWater / pHRU->m_area;//m of water

            float totalWaterDepth = pSatAboveArgillic->m_wDepth+pArgillic->m_wDepth+pUnBelowArgillic->m_wDepth+pSatBelowArgillic->m_wDepth ;
            float totalDepth = pSatAboveArgillic->m_depth + pArgillic->m_depth + pUnBelowArgillic->m_depth + pSatBelowArgillic->m_depth;

            totalWaterDepth = pSatBelowArgillic->m_wDepth + pArgillic->m_wDepth + pUnBelowArgillic->m_wDepth;
            totalDepth = pSatBelowArgillic->m_depth + pArgillic->m_depth + pUnBelowArgillic->m_depth;

            pHRU->m_elws = pHRU->m_elevation - totalDepth + totalWaterDepth ;//m
           // pHRU->m_elws = pHRU->m_elevation - pSatBelowArgillic->m_depth + pSatBelowArgillic->m_wDepth;//m
            pPonded->m_wDepth *= 1000.0f;
            pUnsat->m_wDepth *= 1000.0f;
            pSatAboveArgillic->m_wDepth *= 1000.0f;
            pArgillic->m_wDepth *= 1000.0f;
            pUnBelowArgillic->m_wDepth *= 1000.0f;
            pSatBelowArgillic->m_wDepth *= 1000.0f;

            pHRU->m_currentGWRecharge = (argillicToUnsat - (rechargeAccountingArgillic/dt / pHRU->m_area))*1000.0f;  // mm/d
            }
         return -1;
         }

      float Flow_Grid::AdjustForSaturation(FlowContext *pFlowContext)
         {
            float infiltrationExcess = 0.0f;
            float soilToStream = 0.0f;
            float groundwaterToSoil = 0.0f;
            float unsatToSat = 0.0f;
            float satToArgillic = 0.0;
            float argillicToUnsat = 0.0f;
            float gwUnsatToSat = 0.0f;
            float ssf = 0.0f;
            float gwf = 0.0f;
            float ssUpper = 0.0f;
            float ssLower = 0.0f;
            float shallowGWOutOfCatchment = 0.0f;

            float satToUnsat = 0.0f;
            float argillicToSat = 0.0f;
            float unsatGWToArgillic = 0.0f;
            float satGWToUnsatGW = 0.0f;
            float gwOutOfCatchment = 0.0f;
            float depthOfWater = 0.0f;
            float rechargeAccounting = 0.0f; float rechargeAccountingArgillic = 0.0f;
            bool isSat1 = false; bool isSat2 = false; bool isSat3 = false; bool isSat4 = false; bool isSat5 = false;
            bool isSatDeepGround = false; bool isSatShallowGround = false; bool isUnsatShallowGround = false;
            int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
            ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable("GRID");   // store this pointer (and check to make sure it's not NULL)
            ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable("GWParams");
            ParamTable *pArgillicParam_Table = pFlowContext->pFlowModel->GetTable("ArgillicParams");
            int hruCount = pFlowContext->pFlowModel->GetHRUCount();
            float dt = pFlowContext->timeStep;
            //   #pragma omp parallel for
            for (int h = 0; h < hruCount; h++)
            {
               HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
               int hruLayerCount = pHRU->GetPoolCount();
               float wp = 0.1f; float phi = 0.33f; float lamda = 0.1f; float kSat = 1.0f; float ple = 2.f;
               float kSatGw = 1.0f; float pleGw = 2.f; float phiGw = 0.33f; float fcPct = 0.5f; float kSatArg = 0.1f;
               float wpArg = 10.0f; float lamdaArg = 0.1f; float phiArg = 0.3f; float ic = 1;

               VData lulcA = 0;
               pFlowContext->pFlowModel->GetHRUData(pHRU, pLULC_Table->m_iduCol, lulcA, DAM_FIRST);  // get HRU info
               if (pLULC_Table->m_type == DOT_FLOAT)
                  lulcA.ChangeType(TYPE_FLOAT);
               bool ok = pLULC_Table->Lookup(lulcA, m_col_wp, wp);     // wilting point
               ok = pLULC_Table->Lookup(lulcA, m_col_phi, phi);        // porosity
               ok = pLULC_Table->Lookup(lulcA, m_col_lamda, lamda);    // pore size distribution index
               ok = pLULC_Table->Lookup(lulcA, m_col_kSat, kSat);      // saturated hydraulic conductivity    
               ok = pLULC_Table->Lookup(lulcA, m_col_ple, ple);        // power law exponent
               ok = pLULC_Table->Lookup(lulcA, m_col_ic, ic);
               fcPct = 1.0f;

               VData geo = 0;
               pFlowContext->pFlowModel->GetHRUData(pHRU, pGWParam_Table->m_iduCol, geo, DAM_FIRST);  // get HRU info
               if (pGWParam_Table->m_type == DOT_FLOAT)
                  geo.ChangeType(TYPE_FLOAT);
               ok = pGWParam_Table->Lookup(geo, m_col_kSatGw, kSatGw);    // saturated hydraulic conductivity    
               ok = pGWParam_Table->Lookup(geo, m_col_pleGw, pleGw);        // power law exponent
               ok = pGWParam_Table->Lookup(geo, m_col_phiGw, phiGw);        // power law exponent

               if (pArgillicParam_Table->m_type == DOT_FLOAT)
                  geo.ChangeType(TYPE_FLOAT);
               ok = pArgillicParam_Table->Lookup(geo, m_col_kSatArg, kSatArg);    // saturated hydraulic conductivity    
               ok = pArgillicParam_Table->Lookup(geo, m_col_lamdaArg, lamdaArg);        // power law exponent
               ok = pArgillicParam_Table->Lookup(geo, m_col_wpArg, wpArg);        // power law exponent
               ok = pArgillicParam_Table->Lookup(geo, m_col_phiArg, phiArg);        // power law exponent

               HRUPool *pUnsat = pHRU->GetPool(0);
               HRUPool *pSatAboveArgillic = pHRU->GetPool(1);
               HRUPool *pArgillic = pHRU->GetPool(2);
               HRUPool *pUnBelowArgillic = pHRU->GetPool(3);
               HRUPool *pSatBelowArgillic = pHRU->GetPool(4);
               HRUPool *pPonded = pHRU->GetPool(5);

               //Saturated Below Argillic
               float m_wDepthGW = (float)pSatBelowArgillic->m_volumeWater / pHRU->m_area / phiGw;//m
               float totalDepthLower = (pSatBelowArgillic->m_depth /*+ (pUnBelowArgillic->m_depth*0.5)*/);
               if (m_wDepthGW > totalDepthLower) // if depth of water is greater than groundwater zone depth, move excess to argillic
                  {
                  rechargeAccounting = pSatBelowArgillic->m_volumeWater - (totalDepthLower*pHRU->m_area*phiGw);
                  pUnBelowArgillic->AddFluxFromGlobalHandler(rechargeAccounting / dt, FL_BOTTOM_SOURCE);
                  pSatBelowArgillic->AddFluxFromGlobalHandler(rechargeAccounting / dt, FL_TOP_SINK);
                  m_wDepthGW = totalDepthLower;
                  isSatDeepGround = true;
                  }

               pSatBelowArgillic->m_wc = phiGw;//mm of water - should be saturated?
               pSatBelowArgillic->m_wDepth = m_wDepthGW;
               float waterDepthLowerSat = pSatBelowArgillic->m_wDepth;//m

                                                                      // Unsaturated Below Argillic
               float waterDepthLowerUnsat = pUnBelowArgillic->m_depth;// pSatBelowArgillic->m_depth + pUnBelowArgillic->m_depth - waterDepthLowerSat;//total depth of the unsaturated zone below the argillic
               float wcUnBelowArgillic = phiGw;
               if (waterDepthLowerUnsat>0.0f)
                  wcUnBelowArgillic = (float)pUnBelowArgillic->m_volumeWater / (pHRU->m_area*waterDepthLowerUnsat);

               if (wcUnBelowArgillic > phiGw) //if the lower unsaturated zone is over saturation, move water to argillic
                  {
                  rechargeAccountingArgillic = pUnBelowArgillic->m_volumeWater - (waterDepthLowerUnsat*pHRU->m_area*phiGw);//m3/timestep
                  pArgillic->AddFluxFromGlobalHandler(rechargeAccountingArgillic / dt, FL_BOTTOM_SOURCE);//m3/d
                  pUnBelowArgillic->AddFluxFromGlobalHandler(rechargeAccountingArgillic / dt, FL_TOP_SINK);
                  wcUnBelowArgillic = phiGw;
                  isSat5 = true;
                  }
               pUnBelowArgillic->m_wc = wcUnBelowArgillic;//
               pUnBelowArgillic->m_wDepth = wcUnBelowArgillic*waterDepthLowerUnsat / phiGw;//mm of saturated water (but remember this is an unsaturated zone)
// End Unsaturated Below Argillic
               totalDepthLower += pUnsat->m_depth + pSatAboveArgillic->m_depth + pArgillic->m_depth;

               // Argillic
               float wcArgillic = (float)pArgillic->m_volumeWater / (pHRU->m_area*pArgillic->m_depth);
               float m_wDepth = pArgillic->m_volumeWater / pHRU->m_area / phiArg;
               if (wcArgillic > phiArg) //over saturated
               {
                  float excess = pArgillic->m_volumeWater - (pArgillic->m_depth*pHRU->m_area*phiArg);
                  //float excess = (wcArgillic - phiArg) * pArgillic->m_volumeWater;
                  if (excess > pArgillic->m_volumeWater)
                     excess = pArgillic->m_volumeWater;
 //                 pSatAboveArgillic->m_volumeWater += excess;
 //                 pArgillic->m_volumeWater -= excess;
                  pSatAboveArgillic->AddFluxFromGlobalHandler(excess / dt, FL_BOTTOM_SOURCE);
                  pArgillic->AddFluxFromGlobalHandler(excess / dt, FL_TOP_SINK);
                  wcArgillic = phiArg;
                  m_wDepth = pArgillic->m_depth;
                  //  argillicToUnsat *= 0.03f;
                  isSat4 = true;
               }
               argillicToUnsat = CalcBrooksCoreyKTheta(wcArgillic, wpArg, phiArg, lamdaArg, kSatArg);
               pArgillic->m_wc = wcArgillic;//mm of water
               pArgillic->m_wDepth = m_wDepth;

               // End Argillic

               //Saturated Above Argillic
               float infiltration = 0.0f;
               float runoff = 0.0f;

               float totalDepthUpper =/* (pUnsat->m_depth*0.5f) + */pSatAboveArgillic->m_depth;
               float wcL = phi;
               pSatAboveArgillic->m_wc = wcL;//water content         

               if (pSatAboveArgillic->m_volumeWater > (totalDepthUpper*pHRU->m_area*phi)) //sat+unsat depth is fully saturated with water
               {
                  float exchange = pSatAboveArgillic->m_volumeWater - (totalDepthUpper*pHRU->m_area*phi);
                  //    if (exchange > pSatAboveArgillic->m_volumeWater)
                  //      exchange = pSatAboveArgillic->m_volumeWater;
   //               pUnsat->m_volumeWater += exchange;
  //                pSatAboveArgillic->m_volumeWater -= exchange;

                  pSatAboveArgillic->m_wDepth = totalDepthUpper;

                  pUnsat->AddFluxFromGlobalHandler(exchange / dt, FL_BOTTOM_SOURCE);
                  pSatAboveArgillic->AddFluxFromGlobalHandler(exchange / dt, FL_TOP_SINK);
                  isSatShallowGround = true;
               }
               else
                  pSatAboveArgillic->m_wDepth = (float)pSatAboveArgillic->m_volumeWater / pHRU->m_area / phi; //m
                                                                                                              //End Saturated Above Argillic

                                                                                                              //Unsaturated
               float wcU = phi;

               float waterDepthUpperUnsat = pUnsat->m_depth;/*+ pSatAboveArgillic->m_depth - pSatAboveArgillic->m_wDepth*/;//total depth of the zone above the argillic
               m_wDepth = pUnsat->m_volumeWater / pHRU->m_area / phi;
               wcU = (float)pUnsat->m_volumeWater / (pHRU->m_area*waterDepthUpperUnsat);

               if (wcU > wp)
                  {
                  if (wcU >= phi)//greater than field capacity
                     {
                     float excess = pUnsat->m_volumeWater - (waterDepthUpperUnsat * pHRU->m_area * phi);
                     //pUnsat->m_volumeWater -=excess;                
                     //pPonded->m_volumeWater += excess;

                     if (excess/pHRU->m_area > ic / 1000.0f) //ic is maximum length (mm) that can be exfiltrated
                        {
                      //  pUnsat->m_volumeWater -= pHRU->m_area*ic / 1000.0f;//lose rate at ic mm/day
                      //  pPonded->m_volumeWater += pHRU->m_area*ic / 1000.0f;
                        pUnsat->AddFluxFromGlobalHandler(pHRU->m_area*ic / 1000.0f / dt, FL_TOP_SINK);
                        pPonded->AddFluxFromGlobalHandler(pHRU->m_area*ic / 1000.0f / dt, FL_BOTTOM_SOURCE);
                        }
                     else
                        {
                       // pUnsat->m_volumeWater -= excess;
                       // pPonded->m_volumeWater += excess;
                         pUnsat->AddFluxFromGlobalHandler(excess / dt, FL_TOP_SINK);
                         pPonded->AddFluxFromGlobalHandler(excess / dt, FL_BOTTOM_SOURCE);
                        }
                     ASSERT(pUnsat->m_volumeWater>0.0f);
                     wcU = phi;
                     m_wDepth = pUnsat->m_depth;
                     isUnsatShallowGround = true;
                     }
                  }
               pUnsat->m_wDepth = m_wDepth;//m of water without soil.  total depth is waterDepthUpperUnsat!
               pUnsat->m_wc = wcU;//
                                  //End Unsaturated

                                  //ponded
               pPonded->m_wc = 0.3;//water content
               pPonded->m_wDepth = pPonded->m_volumeWater / pHRU->m_area;//m of water

               float totalWaterDepth = pSatAboveArgillic->m_wDepth + pArgillic->m_wDepth + pUnBelowArgillic->m_wDepth + pSatBelowArgillic->m_wDepth;
               float totalDepth = pSatAboveArgillic->m_depth + pArgillic->m_depth + pUnBelowArgillic->m_depth + pSatBelowArgillic->m_depth;

               totalWaterDepth = pSatBelowArgillic->m_wDepth + pArgillic->m_wDepth + pUnBelowArgillic->m_wDepth;
               totalDepth = pSatBelowArgillic->m_depth + pArgillic->m_depth + pUnBelowArgillic->m_depth;

               pHRU->m_elws = pHRU->m_elevation - totalDepth + totalWaterDepth;//m
                                                                               // pHRU->m_elws = pHRU->m_elevation - pSatBelowArgillic->m_depth + pSatBelowArgillic->m_wDepth;//m
               pPonded->m_wDepth *= 1000.0f;
               pUnsat->m_wDepth *= 1000.0f;
               pSatAboveArgillic->m_wDepth *= 1000.0f;
               pArgillic->m_wDepth *= 1000.0f;
               pUnBelowArgillic->m_wDepth *= 1000.0f;
               pSatBelowArgillic->m_wDepth *= 1000.0f;

               pHRU->m_currentGWRecharge = (argillicToUnsat - (rechargeAccountingArgillic / dt / pHRU->m_area))*1000.0f;  // mm/d
            }
            return -1;
         }

   float Flow_Grid::SRS_6_Layer(FlowContext *pFlowContext)
      {
      if (pFlowContext->timing & 1) // Init()
         return Flow_Grid::Init_Grid_Fluxes(pFlowContext);

      if (pFlowContext->timing & 2) // InitRun()
         return Flow_Grid::InitRun_Grid_Fluxes(pFlowContext);

   //   if (pFlowContext->timing & 4) // Init()
    //     return Flow_Grid::Init_Year(pFlowContext);
    //  if (pFlowContext->timing & 8) // End of Flow timestep
    //     return Flow_Grid::AdjustForSaturation_Rates(pFlowContext);
        // return Flow_Grid::AdjustForSaturation(pFlowContext);
      AdjustForSaturation(pFlowContext); 

      int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
      ParamTable *pLULC_Table = pFlowContext->pFlowModel->GetTable("GRID");   // store this pointer (and check to make sure it's not NULL)
      ParamTable *pGWParam_Table = pFlowContext->pFlowModel->GetTable("GWParams");
      ParamTable *pArgillicParam_Table = pFlowContext->pFlowModel->GetTable("ArgillicParams");
      int hruCount = pFlowContext->pFlowModel->GetHRUCount();
      float minElev = pFlowContext->pFlowModel->m_minElevation;
      int hruIndex = 0;

      #pragma omp parallel for
      for (int h = 0; h < hruCount; h++)
         {
         HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
         int hruLayerCount = pHRU->GetPoolCount();

         float airTemp = 0.0f; float precip = 0.0f;
         float time = pFlowContext->time - (int)pFlowContext->time;
         float currTime = (float)pFlowContext->dayOfYear + time;
         float dt = pFlowContext->timeStep;

         pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, (int)currTime, precip);//mm/d        
         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, (int)currTime, airTemp);//C 
         float wp = 0.1f; float phi = 0.33f; float lamda = 0.1f; float kSat = 1.0f; float ple = 2.f;
         float kSatGw = 1.0f; float pleGw = 2.f; float phiGw = 0.33f; float fcPct = 0.5f; float kSatArg = 0.1f;
         float runoffCoefficient = 1.0f; float lp = 0.28f; float k = 0.1f;
         float wpArg = 10.0f; float lamdaArg = 0.1f; float phiArg = 0.3f;
         float HOF_Threshold=25.0f;

         VData lulcA = 0;
         pFlowContext->pFlowModel->GetHRUData(pHRU, pLULC_Table->m_iduCol, lulcA, DAM_FIRST);  // get HRU info
         if (pLULC_Table->m_type == DOT_FLOAT)
            lulcA.ChangeType(TYPE_FLOAT);
         bool ok = pLULC_Table->Lookup(lulcA, m_col_wp, wp);     // wilting point
         ok = pLULC_Table->Lookup(lulcA, m_col_phi, phi);        // porosity
         ok = pLULC_Table->Lookup(lulcA, m_col_lamda, lamda);    // pore size distribution index
         ok = pLULC_Table->Lookup(lulcA, m_col_kSat, kSat);      // saturated hydraulic conductivity    
         ok = pLULC_Table->Lookup(lulcA, m_col_ple, ple);        // power law exponent
         ok = pLULC_Table->Lookup(lulcA, m_col_lp, lp);        // power law exponent
         ok = pLULC_Table->Lookup(lulcA, m_col_k, k);        // power law exponent
         ok = pLULC_Table->Lookup(lulcA, m_col_fcPct, fcPct);
         ok = pLULC_Table->Lookup(lulcA, m_col_runoffCoefficient, runoffCoefficient);
         ok = pLULC_Table->Lookup(lulcA, m_col_HOF_Threshold, HOF_Threshold);
         HOF_Threshold *=24.0f;//the threshold is in mm/hr, but the units we need are mm/day.  The model requires hourly data (overland flow happens over short time scales)
         VData geo = 0;
         pFlowContext->pFlowModel->GetHRUData(pHRU, pGWParam_Table->m_iduCol, geo, DAM_FIRST);  // get HRU info
         if (pGWParam_Table->m_type == DOT_FLOAT)
            geo.ChangeType(TYPE_FLOAT);

         ok = pGWParam_Table->Lookup(geo, m_col_kSatGw, kSatGw);    // saturated hydraulic conductivity    
         ok = pGWParam_Table->Lookup(geo, m_col_pleGw, pleGw);        // power law exponent
         ok = pGWParam_Table->Lookup(geo, m_col_phiGw, phiGw);        // power law exponent

         if (pArgillicParam_Table->m_type == DOT_FLOAT)
            geo.ChangeType(TYPE_FLOAT);

         ok = pArgillicParam_Table->Lookup(geo, m_col_kSatArg, kSatArg);    // saturated hydraulic conductivity    
         ok = pArgillicParam_Table->Lookup(geo, m_col_lamdaArg, lamdaArg);        // power law exponent
         ok = pArgillicParam_Table->Lookup(geo, m_col_wpArg, wpArg);        // power law exponent
         ok = pArgillicParam_Table->Lookup(geo, m_col_phiArg, phiArg);        // power law exponent

         float infiltrationExcess = 0.0f;
         float soilToStream = 0.0f;
         float groundwaterToSoil = 0.0f;
         float unsatToSat = 0.0f;
         float satToArgillic = 0.0;
         float argillicToUnsat = 0.0f;
         float gwUnsatToSat = 0.0f;
         float ssf = 0.0f;
         float gwf = 0.0f;
         float ssUpper = 0.0f;
         float ssLower = 0.0f;
         float shallowGWOutOfCatchment = 0.0f;

         float satToUnsat = 0.0f;
         float argillicToSat = 0.0f;
         float unsatGWToArgillic = 0.0f;
         float satGWToUnsatGW = 0.0f;
         float gwOutOfCatchment = 0.0f;
         float depthOfWater = 0.0f;
         float rechargeAccounting = 0.0f; float rechargeAccountingArgillic=0.0f;
         bool isSat1 = false; bool isSat2 = false; bool isSat3 = false; bool isSat4 = false; bool isSat5 = false;
         bool isSatDeepGround = false; bool isSatShallowGround = false; bool isUnsatShallowGround = false;
         if (precip > 10000.0f)
            precip = 0.0f;
         //pHRU->m_currentPrecip = precip / 1000.0f;//convert from mm/d to m/d
         float HOF=0.0f;
         float nearstream = 100000.0f;
         int col = pFlowContext->pEnvContext->pMapLayer->GetFieldCol("NEAR_DIST");
         pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[0], col, nearstream);
         if (nearstream < 5.0f)
            {
            if (precip > HOF_Threshold)
               {
               HOF = precip - HOF_Threshold;
               precip = HOF_Threshold;
               }
            }
         precip /= 1000.0f;//m/d
         HOF /= 1000.0f;   //m/d
         
         HRUPool *pUnsat = pHRU->GetPool(0);
         HRUPool *pSatAboveArgillic = pHRU->GetPool(1);
         HRUPool *pArgillic = pHRU->GetPool(2);
         HRUPool *pUnBelowArgillic = pHRU->GetPool(3);
         HRUPool *pSatBelowArgillic = pHRU->GetPool(4);
         HRUPool *pPonded = pHRU->GetPool(5);

         //Saturated Below Argillic

         float wDepthGW = (float)pSatBelowArgillic->m_volumeWater / pHRU->m_area / phiGw ;//m
         float totalDepthLower = (pSatBelowArgillic->m_depth );
         if (wDepthGW > totalDepthLower) // if depth of water is greater than groundwater zone depth, move excess to argillic
            {
            wDepthGW = totalDepthLower;
            isSatDeepGround = true;
            }

       //  pSatBelowArgillic->m_wc = phiGw;//mm of water - should be saturated?
         gwf = CalcSatWaterFluxes(pHRU, pSatBelowArgillic, phiGw, pleGw, kSatGw, satGWToUnsatGW, minElev, pSatBelowArgillic->m_depth + pUnBelowArgillic->m_depth, phiGw, pGWParam_Table, pFlowContext);

         if (pHRU->m_lowestElevationHRU )
            { 
            if (gwf > 0.0f)
               gwOutOfCatchment = gwf;      
            }

         float waterDepthLowerSat = pSatBelowArgillic->m_wDepth;//m

         // Unsaturated Below Argillic
         float waterDepthLowerUnsat = pUnBelowArgillic->m_depth*phiGw;// pSatBelowArgillic->m_depth + pUnBelowArgillic->m_depth - waterDepthLowerSat;//total depth of the unsaturated zone below the argillic
         float wcUnBelowArgillic=phiGw;
         if (waterDepthLowerUnsat>0.0f)
            wcUnBelowArgillic = (float)pUnBelowArgillic->m_volumeWater / (pHRU->m_area*waterDepthLowerUnsat);

         if (wcUnBelowArgillic > phiGw) //if the lower unsaturated zone is over saturation, move water to argillic
            {             
            wcUnBelowArgillic = phiGw;
            isSat5 = true;
            }
        // if (!isSatDeepGround)
            gwUnsatToSat = CalcBrooksCoreyKTheta(wcUnBelowArgillic, wp, phiGw, lamda, kSat);
            // End Unsaturated Below Argillic
      
         float deepGWLoss = fcPct/1000.0f*pHRU->m_area;// units of the parameter are mm/d so convert to m3/d
         if (pSatBelowArgillic->m_volumeWater/pHRU->m_area*1000.0f > 100.0f)
            deepGWLoss=0.0f;
         totalDepthLower += pUnsat->m_depth + pSatAboveArgillic->m_depth + pArgillic->m_depth ;

         // Argillic
         float wcArgillic = 0.0f;
         if (pHRU->m_area*pArgillic->m_depth*phiArg>0.0f)
            wcArgillic = pArgillic->m_volumeWater / (pHRU->m_area*pArgillic->m_depth);
            
         if (wcArgillic > (phiArg)) //over saturated
            {         
            wcArgillic = phiArg;
            isSat4 = true;
            }
         if (!isSat5)//if the zone below the argillic is unsaturated, then move water into it
            argillicToUnsat = CalcBrooksCoreyKTheta(wcArgillic, wpArg, phiArg, lamdaArg, kSatArg);
                     
         // End Argillic

         //Saturated Above Argillic
         float infiltration = 0.0f;
         float runoff = 0.0f;
         float fastRunoff=0.0f;

         float totalDepthUpper = pSatAboveArgillic->m_depth;
         float wcL = phi;        

         if (pSatAboveArgillic->m_volumeWater > (totalDepthUpper*pHRU->m_area*phi)) //sat+unsat depth is fully saturated with water
            {
            pSatAboveArgillic->m_wDepth = totalDepthUpper;
            isSatShallowGround=true;
            }
        // else
            ssf = CalcSatWaterFluxes(pHRU, pSatAboveArgillic, phi, ple, kSat, satToUnsat, minElev, totalDepthUpper, wcL, pLULC_Table, pFlowContext);

         if (pHRU->m_lowestElevationHRU && ssf>0.0f)
            shallowGWOutOfCatchment = ssf;

         //End Saturated Above Argillic

         //Unsaturated
         float wcU=phi;
         float waterDepthUpperUnsat = pUnsat->m_depth;//total depth of the  zone above the argillic
         if (waterDepthUpperUnsat > 0.0f)
            wcU = (float)pUnsat->m_volumeWater / (pHRU->m_area*waterDepthUpperUnsat);
         float uns = 0.0f;
         if (wcU > wp)
            {
            if (wcU >= phi)//greater than field capacity
               { 
               wcU = phi;
               isUnsatShallowGround=true;
               }
            uns= CalcBrooksCoreyKTheta(wcU, wp, phi, lamda, kSat);
            if (!isSatShallowGround)
               unsatToSat = uns ;
            }        
         if (!isSat4 && pSatAboveArgillic->m_volumeWater/pHRU->m_area*1000 > 5.0f)
            satToArgillic = uns;
         if (!isUnsatShallowGround)//if the near surface soil is not saturated, then infiltration can occur
            infiltration = pPonded->m_volumeWater / pHRU->m_area ;//constant fraction infiltrates  m/d
         if (pPonded->m_volumeWater / pHRU->m_area > k /1000.0f) //above a threshold (k (mm))
              runoff = ((pPonded->m_volumeWater / pHRU->m_area) - (k /1000.0f))*runoffCoefficient;
         if (!isUnsatShallowGround)
            infiltration-=runoff;

ASSERT(infiltration>=0.0f);

         //End Ponded
         pHRU->m_currentPrecip = precip * 1000.0f;
         pHRU->m_currentRunoff = (runoff+HOF ) * 1000.0f; // mm/d

         pHRU->m_currentGWFlowOut = (gwOutOfCatchment + shallowGWOutOfCatchment + deepGWLoss)*1000.0f / pHRU->m_area;  // mm/d

         pPonded->AddFluxFromGlobalHandler(precip*pHRU->m_area, FL_TOP_SOURCE);
         pPonded->AddFluxFromGlobalHandler(infiltration*pHRU->m_area, FL_BOTTOM_SINK);
         pPonded->AddFluxFromGlobalHandler(runoff*pHRU->m_area, FL_STREAM_SINK);

         pUnsat->AddFluxFromGlobalHandler(infiltration*pHRU->m_area, FL_TOP_SOURCE);
         pUnsat->AddFluxFromGlobalHandler(unsatToSat*pHRU->m_area, FL_BOTTOM_SINK);
            
         pSatAboveArgillic->AddFluxFromGlobalHandler(unsatToSat*pHRU->m_area, FL_TOP_SOURCE);
         pSatAboveArgillic->AddFluxFromGlobalHandler(satToArgillic*pHRU->m_area, FL_BOTTOM_SINK);
       //  pSatAboveArgillic->AddFluxFromGlobalHandler(ssf, FL_USER_DEFINED);
         pSatAboveArgillic->AddFluxFromGlobalHandler(shallowGWOutOfCatchment, FL_BOTTOM_SINK);
                 
         pArgillic->AddFluxFromGlobalHandler(satToArgillic*pHRU->m_area, FL_TOP_SOURCE);
         pArgillic->AddFluxFromGlobalHandler(argillicToUnsat*pHRU->m_area, FL_BOTTOM_SINK);
            
         pUnBelowArgillic->AddFluxFromGlobalHandler(argillicToUnsat*pHRU->m_area, FL_TOP_SOURCE);
         pUnBelowArgillic->AddFluxFromGlobalHandler(gwUnsatToSat*pHRU->m_area, FL_BOTTOM_SINK);
            
         pSatBelowArgillic->AddFluxFromGlobalHandler(gwUnsatToSat*pHRU->m_area, FL_TOP_SOURCE);
         pSatBelowArgillic->AddFluxFromGlobalHandler(gwf, FL_USER_DEFINED);
         pSatBelowArgillic->AddFluxFromGlobalHandler(gwOutOfCatchment, FL_BOTTOM_SINK); 
         pSatBelowArgillic->AddFluxFromGlobalHandler(deepGWLoss, FL_BOTTOM_SINK);
           
         Reach * pReach = pUnsat->GetReach();
         if (pReach)
            if (runoff > 0.0f)
               pReach->AddFluxFromGlobalHandler((runoff+HOF) * pHRU->m_area); //m3/d
         }

      return 0.0f;
         
      }
         

float Flow_Grid::CalcBrooksCoreyKTheta(float wc, float wp, float phi, float lamda, float kSat)
   {
   float value=0.0f;
   if (wc - wp <= 0.0f)
      return value;
   float ratio = (wc - wp)/(phi-wp);
   float power = 3.0f+(2.0f/lamda);
   float kTheta = kSat * pow(ratio,power);
   if (kTheta > kSat)
      kTheta = kSat;
   value = kTheta ;
   return value;
   }


float Flow_Grid::CalcSatWaterFluxes(HRU *pHRU, HRUPool *pHRUPool, float phi, float ple, float kSat, float &q1, float minElev, float totalDepth, float wc, ParamTable *pTable, FlowContext *pFlowContext)
   {
   float wcL = wc;// water content 
   float depth = ((float)pHRUPool->m_volumeWater / pHRU->m_area) / phi;  //actual depth of water filled area

   wcL = (float)pHRUPool->m_volumeWater / (pHRU->m_area*pHRUPool->m_depth);
  // HRUPool *pUpperLayer = pHRU->GetPool(1);
   //float totalDepth = pHRUPool->m_depth + pUpperLayer->m_depth; // total depth of the 2 layers

   if (wcL > phi) //by definition, this will not occur - the zone is saturated so water content = phi!
      {
      q1 = (((float)pHRUPool->m_volumeWater * (wcL - phi)) / pHRU->m_area);
      wcL = phi;
      depth = pHRUPool->m_depth;
      }

   float myElev = pHRU->m_elevation - (totalDepth - depth);
  // myElev = pHRU->m_elevation;
   float width = (float)sqrt(pHRU->m_area);

   float outflow = 0.0f;
   float inflow = 0.0f;

   for (int upCheck = 0; upCheck<pHRU->m_neighborArray.GetSize(); upCheck++)
      {
      HRU *pUpCheck = pHRU->m_neighborArray[upCheck];
      ASSERT(pUpCheck != NULL);
      float kSatGwUp = 0.0f; float phiGwUp = 0.0f;
      VData geo = 0;
      pFlowContext->pFlowModel->GetHRUData(pUpCheck, pTable->m_iduCol, geo, DAM_FIRST);  // get HRU info
      if (pTable->m_type == DOT_FLOAT)
         geo.ChangeType(TYPE_FLOAT);
      pTable->Lookup(geo, m_col_kSatGw, kSatGwUp);    // saturated hydraulic conductivity  
      pTable->Lookup(geo, m_col_phiGw, phiGwUp);        // power law exponent
      if (pUpCheck != NULL)
         {
         HRUPool *pUpCheckLayer = pUpCheck->GetPool(pHRUPool->m_pool);
         
        // float wcUp = pUpCheckLayer->m_wc;
         float depthUp = ((float)pUpCheckLayer->m_volumeWater / pHRU->m_area) / phiGwUp;  //depth of water

         float wcUp = (float)pUpCheckLayer->m_volumeWater / (pHRU->m_area*pUpCheckLayer->m_depth);

         if (wcUp > phiGwUp)
            {
            wcUp = phiGwUp;
            depthUp = pUpCheckLayer->m_depth;
            }

         float w = 0.0f;
         if (pUpCheck->m_demRow == pHRU->m_demRow || pUpCheck->m_demCol == pHRU->m_demCol)
            w = width*0.5f;
         else
            w = width*0.354f; //from Quinn et al., 1991...
         float otherElev = pUpCheck->m_elevation - (totalDepth - depthUp);
     //    otherElev = pUpCheck->m_elevation;
         if (myElev > otherElev && depth > 0.1f) // calculate my outputs
            {
            float iOut = 1 - ((totalDepth - depth) / totalDepth);
            if (iOut > 1.0f)
               iOut = 1.0f;
            ASSERT(iOut >= 0);
            float slope = (myElev - otherElev) / w;
            float T = (kSat*depth/ple)*pow(iOut, ple);
            outflow += T*slope*w;
            if (pHRUPool->m_volumeWater <= 1.0f)
               outflow = 0.0f;
            pHRUPool->m_waterFluxArray[upCheck + 7] = -T*slope*w;
            }
         if (myElev < otherElev  && depthUp > 0.1f) //calculate my inputs
            {
            float iIn = 1 - ((totalDepth - depthUp) / totalDepth);
            if (iIn > 1.0f)
               iIn = 1.0f;
            float slope = (otherElev - myElev) / w;
            float T = (kSatGwUp*depthUp / ple)*pow(iIn, ple);
            inflow += T*slope*w;
            if (pUpCheckLayer->m_volumeWater <= 1.0f)
               inflow = 0.0f;
            pHRUPool->m_waterFluxArray[upCheck + 7] = T*slope*w;
            }
         }
      }
   // if (myElev==minElev)
   //    outflow=inflow;//assume a zero tension downstream boundary condition for the saturated groundwater
   return inflow - outflow;
   }


float Flow_Grid::CalcSatWaterFluxes(HRU *pHRU, HRUPool *pHRUPool, float phi, float ple, float kSat, float &q1, float minElev)
   {
   float wcL = pHRUPool->m_wc;// water content 
   float depth = ((float)pHRUPool->m_volumeWater / pHRU->m_area) / phi;  //actual depth of water filled area
   HRUPool *pUpperLayer = pHRU->GetPool(1);
   float totalDepth = pHRUPool->m_depth + pUpperLayer->m_depth; // total depth of the 2 layers

   if (wcL > phi)
      {
      q1 = (((float)pHRUPool->m_volumeWater * (wcL - phi)) / pHRU->m_area);
      wcL = phi;
      depth = pHRUPool->m_depth;
      }

   float myElev = pHRU->m_elevation - (totalDepth - depth);
   float width = (float)sqrt(pHRU->m_area);

   float outflow = 0.0f;
   float inflow = 0.0f;

   for (int upCheck = 0; upCheck<pHRU->m_neighborArray.GetSize(); upCheck++)
      {
      HRU *pUpCheck = pHRU->m_neighborArray[upCheck];
      ASSERT(pUpCheck != NULL);
      if (pUpCheck != NULL)
         {
         HRUPool *pUpCheckLayer = pUpCheck->GetPool(2);
         //ASSERT(pUpCheck->m_waterFluxArray. > 7);//we assume that the last layer is layer 2 and that it has horizontal fluxes!
         float wcUp = pUpCheckLayer->m_wc;
         float depthUp = ((float)pUpCheckLayer->m_volumeWater / pHRU->m_area) / phi;  //depth of water
         if (wcUp > phi)
            {
            wcUp = phi;
            depthUp = pUpCheckLayer->m_depth;
            }

         float w = 0.0f;
         if (pUpCheck->m_demRow == pHRU->m_demRow || pUpCheck->m_demCol == pHRU->m_demCol)
            w = width*0.5f;
         else
            w = width*0.354f; //from Quinn et al., 1991...
         float otherElev = pUpCheck->m_elevation - (totalDepth - depthUp);

         if (myElev > otherElev) // calculate my outputs
            {
            float iOut = 1 - ((totalDepth - depth) / totalDepth);
            if (iOut > 1.0f)
               iOut = 1.0f;
            ASSERT(iOut >= 0);
            float slope = (myElev - otherElev) / w;
            float T = (kSat*totalDepth)*pow(iOut, ple);
            outflow += T*slope*w;
            if (pHRUPool->m_volumeWater == 0.0f)
               outflow = 0.0f;
            pHRUPool->m_waterFluxArray[upCheck + 7] = -T*slope*w*pHRU->m_area;
            }
         if (myElev < otherElev) //calculate my inputs
            {
            float iIn = 1 - ((totalDepth - depthUp) / totalDepth);
            if (iIn > 1.0f)
               iIn = 1.0f;
            float slope = (otherElev - myElev) / w;
            float T = (kSat*totalDepth)*pow(iIn, ple);
            inflow += T*slope*w;
            if (pUpCheckLayer->m_volumeWater == 0.0f)
               inflow = 0.0f;
            pHRUPool->m_waterFluxArray[upCheck + 7] = T*slope*w*pHRU->m_area;
            }
         }
      }
   // if (myElev==minElev)
   //    outflow=inflow;//assume a zero tension downstream boundary condition for the saturated groundwater
   return inflow - outflow;
   }
float Flow_Grid::PrecipFluxHandler( FlowContext *pFlowContext )
   {
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;
   return pHRU->m_currentPrecip*pHRU->m_area;// m3/d
   }

float Flow_Grid::Tracer_Application( FlowContext *pFlowContext )
   {
   float time = pFlowContext->time;

   time  = fmod( time, 365.0f );  // jpb - repeat yearly
   float dep=0.0f;
   float precip=0.0f;
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;
   if (time > 0.0f && time < 200.0f)
      {
      //pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP,pHRU,pFlowContext->dayOfYear,precip);//mm
      //precip/=1000.0f;//m
      precip=pHRUPool->m_waterFluxArray[FL_TOP_SOURCE];//the downward flux into this layer
      dep = 1.0f;//kg/m3
      }
   return precip*dep;// m3/d*kg/m3=kg/d
   }

float Flow_Grid::Solute_Fluxes( FlowContext *pFlowContext )
   {
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();  

   #pragma omp parallel for firstprivate( pFlowContext )
   for ( int h=0; h < hruCount; h++ )
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      HRUPool *pLowerLayer = pHRU->GetPool( 1 );
      HRUPool *pUpperLayer = pHRU->GetPool( 0 );

      int svC = pFlowContext->svCount;   
      for (int k=0;k<svC;k++)	  
         {
         float concUpper=0.0f;
         if (pUpperLayer->m_volumeWater>0.0f)
            concUpper=(float)pUpperLayer->GetExtraStateVarValue(k)/(float)pUpperLayer->m_volumeWater;

         float concLower=0.0f;
         if (pLowerLayer->m_volumeWater>0.0f)
            concLower=(float)pLowerLayer->GetExtraStateVarValue(k)/(float)pLowerLayer->m_volumeWater;

         float ss=0.0f;

         
         float a1 = pUpperLayer->m_waterFluxArray[0]*concUpper;//upward flux across upper surface of this layer (a loss) - ET, calculated elsewhere
         float b1 = pUpperLayer->m_waterFluxArray[1]*0;//down flux across upper surface of this layer (a gain) precip, but accounted for elsewhere (a flux)
         float c1 = pUpperLayer->m_waterFluxArray[2]*concLower;//upward flux across lower surface of this layer (a gain) - saturated to unsaturated zone
         float d1 = pUpperLayer->m_waterFluxArray[3]*concUpper; //down flux across lower surface of this layer (a loss)
         float e11 = pUpperLayer->m_waterFluxArray[4]*concUpper; //loss to streams

        /* float jdw = ks*mass; //ks is a lumped first order rate constant
         float e=pow(0.784f,((log(kow)-1.78)*(log(kow)-1.78))/2.44f); 
         float ju=f*e*mass;//uptake rate. fi s fraction of water used for et over the timestep, e is an efficiency factor
         float jer=sm*rom*kd*cw;
         float jfof=
            */
         ss=a1+b1+c1+d1+e11;

         //ss=0.0f;
         pUpperLayer->AddExtraSvFluxFromGlobalHandler(k, ss*pHRU->m_area); 

         float e1 = pLowerLayer->m_waterFluxArray[0]*concLower;//upward flux across upper surface of this layer (a loss) -saturated to unsaturated zone
         float f1 = pLowerLayer->m_waterFluxArray[1]*concUpper;//down flux across upper surface of this layer (a gain) - gwREcharge
         float g1 = pLowerLayer->m_waterFluxArray[2]*0;//upward flux across lower surface of this layer (a gain) - saturated to unsaturate zone
         float h1 = pLowerLayer->m_waterFluxArray[3]*0; //down flux across lower surface of this layer (a loss)
         float i1 = 0.0f; // outgoing flow to horizontal neighbors
         float j1 = 0.0f; // incoming flow from horizontal neighbors
         
         for (int upCheck=0;upCheck<pHRU->m_neighborArray.GetSize(); upCheck++) 
		    {    
		    HRU *pUpCheck = pHRU->m_neighborArray[ upCheck ];
            HRUPool *pUpCheckLayer = pUpCheck->GetPool(1);

            float concNeighbor=0.0f;
            if (pUpCheckLayer->m_volumeWater>0.0f)
               concNeighbor=(float)pUpCheckLayer->GetExtraStateVarValue(k)/(float)pUpCheckLayer->m_volumeWater;
            if (pLowerLayer->m_waterFluxArray[upCheck+5] <= 0.0f)//negative flow, so this flux is leaving me
               i1+=pLowerLayer->m_waterFluxArray[upCheck+5]*concLower; 
            else                                           //positive flow, so this flux is leaving my neighbor
               j1+=pLowerLayer->m_waterFluxArray[upCheck+5]*concNeighbor; 
            } 

         ss=e1+f1+g1+h1+i1+j1;
        // ss=0.0f;

         pLowerLayer->AddExtraSvFluxFromGlobalHandler(k, ss*pHRU->m_area );     //m3/d
         Reach * pReach = pUpperLayer->GetReach();
         ReachSubnode *subnode = pReach->GetReachSubnode(0);
         if ( pReach )
            subnode->AddExtraSvFluxFromGlobalHandler(k, (-e11*pHRU->m_area) ); //m3/d.  e1 is stored as a loss from the unsaturated zone - so it is negative and needs to be converted to a + value.
         }
      }
   return 0.0f;
   }


bool Flow_Grid::LoadXml(LPCTSTR filename)
{
   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);
   bool loadSuccess = true;
   ///put back///FILE *fp;

   if (!ok)
      {
      CString msg;
      msg.Format("Error reading input file, %s", filename);
      AfxGetMainWnd()->MessageBox(doc.ErrorDesc(), msg);
      return false;
      }


   TiXmlElement *pXmlRoot = doc.RootElement();  // <flow_model>
   TiXmlElement *pXmlInitial = pXmlRoot->FirstChildElement(_T("Model"));
   if (pXmlInitial != NULL)
      {
      TiXmlElement *pXmlinit = pXmlInitial->FirstChildElement("model");

      while (pXmlinit != NULL)
         {
         LPCTSTR conname = NULL;
         LPCTSTR conrot = NULL;
         LPCTSTR rotname = NULL;
         LPCTSTR agename = NULL;
         XML_ATTR attrs[] = {
            // attr             type            address            isReq  checkCol
            { "lai",		       TYPE_STRING,		&conname,		true, 0 },
            { "col_rotation",	 TYPE_STRING,		&conrot,		true, 0 },
            { "rotationAge",	 TYPE_STRING,		&rotname,		true, 0 },
            { "ageheight",	    TYPE_STRING,		&agename,		true, 0 },
            { NULL,            TYPE_NULL,		NULL,          false,  0 }
         };

         bool ok = TiXmlGetAttributes(pXmlinit, attrs, filename);

         if (!ok)
            {
            CString msg(_T("Error in specification of Initial in ReefSim initial xml "));
            Report::ErrorMsg(msg);
            return false;
            }
         else 
            {
               m_laiFilename = conname;
               m_rotationAgeFilename = rotname;
               m_ageHeightFilename = agename;
               m_col_rotation = conrot;
            }
         pXmlinit = pXmlinit->NextSiblingElement(_T("initial"));
      }
   }
   return true;
}
