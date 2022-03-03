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
// HBV.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "HBV.h"
#include <Flow.h>
#include <DATE.HPP>
#include <Maplayer.h>
#include <MAP.h>
#include <UNITCONV.H>
#include <omp.h>
#include <math.h>
#include <UNITCONV.H>
#include <alglib\AlgLib.h>
//#include "AlgLib\ap.h"
#include <AlgLib\interpolation.h>
//using namespace alglib_impl;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


float HBV::GetMean(FDataObj *pData, int col, int startRow, int numRows)
   {
   
   int rows = pData->GetRowCount();

   if (rows <= 0 || (int)startRow+numRows >= rows)
      return 0;

   float mean = 0.0;

   for (int i = startRow; i < startRow+numRows; i++)
      mean += pData->Get(col, i);

   return (mean / (numRows));
   }


float HBV::ExchangeFlowToCalvin(FlowContext *pFlowContext)
   {
   FDataObj *pMnthlyData = new FDataObj(25,0);
   int yearOffset = pFlowContext->pEnvContext->currentYear - pFlowContext->pEnvContext->startYear;
   int numRes = 0;
   int mnth[12];
   mnth[0]=9; mnth[1]=10; mnth[2]=11;
   for (int i=3;i<12;i++)
      mnth[i]=i-3;
      
  // FDataObj *pData = NULL;
   //Step 1.  Get daily data from Flow
   int outputCount = pFlowContext->pFlowModel->GetModelOutputGroupCount();
   for (int i = 0; i < outputCount ; i++)
      {
      ModelOutputGroup *pGroup = pFlowContext->pFlowModel->GetModelOutputGroup(i);
      CString name = pGroup->m_name;
      if (name=="Calvin") //find the offset of the model output with named Calvin.  It will have a column for each reservoir, plus time         
         {
         int offset = yearOffset*365;
         // Step 2.  Get average monthly data from Daily Data
         numRes = pGroup->m_pDataObj->GetColCount() - 1;//first column is date, so numres is colCount-1
         float *row = new float[numRes + 1]; //date, and 1 column for each reservoir
         for (int row_ = 0; row_ < 12; row_++)
            {
            row[0] = row_;//add month
            for (int col_ = 0; col_ < numRes; col_++)
               {
               float mn = GetMean(pGroup->m_pDataObj, col_ + 1, offset, 30);
               row[col_ + 1] = mn;
               }
            offset += 30;
            pMnthlyData->AppendRow(row, numRes + 1);
            }
         delete [] row;
         break;
         }   
         
      }
     
   CString nam;
   nam.Format(_T("%s%i%s"), "d:\\Envision\\StudyAreas\\CalFEWS\\calvin\\annual\\linksWY", pFlowContext->pEnvContext->currentYear,".csv");

	// Step 3.  Write monthly data into the Calvin input spreadsheet, for the current year (Calvin will read it in the next step, when it runs this year)
	VDataObj *pExchange = new VDataObj(0, 0, U_UNDEFINED);
	pExchange->ReadAscii(nam,',');
   for (int res = 0; res<1;res++)
      { 
      for (int link=0;link<pExchange->GetRowCount(); link++)
         { 
         CString a = pExchange->GetAsString(0,link);
         CString b = pExchange->GetAsString(1,link);
         CString aa=a.Left(6);
         CString bb=b.Left(6);
         int a1 = strcmp(aa,"INFLOW");
         int b1 = strcmp(bb,"SR_PNF");

         if (a1==0 && b1==0)
            {           
	         for (int i = 0; i < 12; i++)
	            {
		         pExchange->Set(5, link+i, pMnthlyData->Get(res+1,mnth[i]));
		         pExchange->Set(6, link+i, pMnthlyData->Get(res+1,mnth[i]));
	            }
            break;
            } 
         }
      }
   //Step 5. Write updates out to files
   pExchange->WriteAscii(nam);
   pMnthlyData->WriteAscii("d:\\envision\\studyareas\\calfews\\calvin\\exchange.csv");

   delete pMnthlyData;
   delete pExchange;

	return -1.0f;
   }

float HBV::ExchangeCalvinToFlow(FlowContext *pFlowContext)
{
   FDataObj *pDlyData = new FDataObj(360, 1);
   //Step 1.  Get monthly data from Calvin
   VDataObj *pExchange = new VDataObj(6, 0, U_UNDEFINED);
   pExchange->ReadAscii("d:\\envision\\studyareas\\calfews\\calvin\\example-results\\storage.csv");
   int calvinOffset=0;
   for (int i = 0; i < (int)pFlowContext->pFlowModel->m_reservoirArray.GetSize(); i++)
   {
      Reservoir *pRes = pFlowContext->pFlowModel->m_reservoirArray.GetAt(i);
      FDataObj *pData = pRes->m_pResData;
      
      CString name=pRes->m_name;
      for (int j=0;j<pExchange->GetColCount();j++)//search columns to find correct name
         { 
         CString lab = pDlyData->GetLabel(j);
         calvinOffset=j;
         if (lab==name)
            break;
         }
      int offset = 0;
      // Step 2.  Read data from Calvin Output, add that data to an Envision DataObj
          
      for (int k = 0; k < 24; k=k+2)
        {
         float dat = pExchange->GetAsFloat(calvinOffset, k);
         for (int y=0;y<30;y++)
            { 
            offset += 30;
            pDlyData->Add(2, y, dat);
            }
         }
      }
   return -1.0f;
   }

float HBV::InitHBV(FlowContext *pFlowContext, LPCTSTR inti)
   {
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" ); 
   m_col_cfmax = pHBVTable->GetFieldCol( "CFMAX" );         // get the location of the parameter in the table    
   m_col_tt    = pHBVTable->GetFieldCol( "TT" );  
   m_col_sfcf  = pHBVTable->GetFieldCol( "SFCF" ); 
   m_col_cwh   = pHBVTable->GetFieldCol( "CWH" ); 
   m_col_cfr   = pHBVTable->GetFieldCol( "CFR" );  
   m_col_lp    = pHBVTable->GetFieldCol( "LP" );  
   m_col_fc    = pHBVTable->GetFieldCol( "FC" );  
   m_col_beta  = pHBVTable->GetFieldCol( "BETA" );  
   m_col_kperc = pHBVTable->GetFieldCol( "PERC" ); 
   m_col_wp    = pHBVTable->GetFieldCol( "WP" ); 
   m_col_k0    = pHBVTable->GetFieldCol( "K0" );  
   m_col_k1    = pHBVTable->GetFieldCol( "K1" );  
   m_col_uzl   = pHBVTable->GetFieldCol( "UZL" );  
   m_col_k2    = pHBVTable->GetFieldCol("K2"); 
   m_col_rain  = pHBVTable->GetFieldCol("RAIN");
   m_col_snow  = pHBVTable->GetFieldCol("SNOW");
   m_col_sfcfCorrection = pHBVTable->GetFieldCol("SFCFCORRECTION");

   const char* p = pFlowContext->pFlowModel->GetPath();
   
   

   return -1.0f;
   }

float HBV::InitHBV_WithQuickflow(FlowContext *pFlowContext, LPCTSTR init)
   {
   /*ParamTable *pSoilPropertiesTable = pFlowContext->pFlowModel->GetTable("SoilData");
   m_col_thickness = pSoilPropertiesTable->GetFieldCol("THICK");
   m_col_SSURGO_WP = pSoilPropertiesTable->GetFieldCol("WP");
   m_col_SSURGO_FC = pSoilPropertiesTable->GetFieldCol("WTHIRDBAR");*/
   
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable("HBV");
   m_col_cfmax = pHBVTable->GetFieldCol("CFMAX");         // get the location of the parameter in the table    
   m_col_tt    = pHBVTable->GetFieldCol("TT");
   m_col_sfcf  = pHBVTable->GetFieldCol("SFCF");
   m_col_cwh   = pHBVTable->GetFieldCol("CWH");
   m_col_cfr   = pHBVTable->GetFieldCol("CFR");
   m_col_lp    = pHBVTable->GetFieldCol("LP");
   m_col_fc    = pHBVTable->GetFieldCol("FC");
   m_col_beta  = pHBVTable->GetFieldCol("BETA");
   m_col_kperc = pHBVTable->GetFieldCol("PERC");
   m_col_wp    = pHBVTable->GetFieldCol("WP");
   m_col_k0    = pHBVTable->GetFieldCol("K0");
   m_col_k1    = pHBVTable->GetFieldCol("K1");
   m_col_uzl   = pHBVTable->GetFieldCol("UZL");
   m_col_k2    = pHBVTable->GetFieldCol("K2");
   m_col_ksoilDrainage = pHBVTable->GetFieldCol("KSoil");    // specific to this method
   m_col_rain  = pHBVTable->GetFieldCol("RAIN");
   m_col_snow  = pHBVTable->GetFieldCol("SNOW");
   m_col_sfcfCorrection = pHBVTable->GetFieldCol("SFCFCORRECTION");
   /*
   double v=0.0f;
   if (alglib::fp_eq(v, 0.0)){}

   // from file
   ALRadialBasisFunction3D rbf;

   // optionally set type of model 
   // (radius should be about avg distance between neighboring points)
   rbf.SetQNN(1.0, 5.0);    // OR
   //rbf.SetML(1.0, 5, 0.005);   // radius, layers, lambdaV 

   // load the file (number of outputs inferred from file columns)
   rbf.CreateFromCSV(csvFile);

   // query for values
   double value = rbf.GetAt(1, 1, 0);


   // from data
   ALRadialBasisFunction3D rbf;
   rbf.Create(1);  // number of outputs

   // optionally set type of model 
   // (radius should be about avg distance between neighboring points)
   rbf.SetML(1.0, 5, 0.005);   // radius, layers, lambdaV 

   FDataObj inputData;
   inputData.ReadAscii("somefile.csv");

   rbf.SetPoints(inputData);  // rows are input points, col0=x, col1=y, col2=z, col3=f0, col4=f1 etc...

   rbf.Build();

   double value = rbf.GetAt(1, 1, 0);

   //alglib_impl:::bfmodel model;
   //alglib_impl::rbfcreate(2,1, model);
   //v = alglib_impl::rbfcalc2(model, 0.0, 0.0);
   //alglib::real_2d_array xy = "[[-1,0,2],[+1,0,3]]";
   //alglib_impl::rbfsetpoints(model, xy);

   //v = alglib::rbfcalc2(model, 0.0, 0.0);
   */
   return -1.0f;
   }


float HBV::HBV_Basic(FlowContext *pFlowContext)
   {
   if ( pFlowContext->timing & GMT_INIT ) // Init()
      return HBV::InitHBV( pFlowContext, NULL );

   if ( ! ( pFlowContext->timing & GMT_CATCHMENT) )   // only process these two timings, ignore everyting else
      return 0;

   // int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();  

   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)

   int doy = pFlowContext->dayOfYear;  // int( fmod( timeInRun, 364 ) );  // zero based day of year
   int _month = 0;int _day;int _year;
   BOOL ok = ::GetCalDate( doy, &_year, &_month, &_day, TRUE );

   //omp_set_num_threads( 8 );

   // iterate through hrus/hrulayers 
   #pragma omp parallel for firstprivate( pFlowContext )
   for ( int h=0; h < hruCount; h++ )
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      
      int hruLayerCount = pHRU->GetPoolCount();
      //Initialize local variables
      float CFMAX,TT,CFR,SFCF,CWH,Beta,kPerc,WP=0.0f;
      float k0,k1,UZL,k2,wp=0.0f;
      float fc, lp=0.0f;
      VData key;            // lookup key connecting rows in the csv files with IDU records
      float precip = 0.0f;
      float airTemp = -999.f;
      float snow =0.0f;
      float snowing=0.0f;
      float rain=0.0f;
      float refreezing=0.0f;
      float melting=0.0f;
      float meltWater=0.0f;
      float meltToSoil=0.0f;
      float etRc = 0.0f;

      // Get Model Parameters
      pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU key info for lookups
      if (pHBVTable->m_type==DOT_FLOAT)
            key.ChangeType(TYPE_FLOAT);

      bool ok = pHBVTable->Lookup( key, m_col_cfmax, CFMAX );   // degree-day factor (mm oC-1 day-1)
      ok = pHBVTable->Lookup( key, m_col_tt, TT );        // threshold temperature (oC)
      ok = pHBVTable->Lookup( key, m_col_cfr, CFR );      // refreezing coefficient
      ok = pHBVTable->Lookup( key, m_col_sfcf, SFCF );    // snowfall correction factor
      ok = pHBVTable->Lookup( key, m_col_cwh, CWH );      // snowpack retains water till this portion is exceeded
      ok = pHBVTable->Lookup( key, m_col_fc, fc );        // maximum soil moisture storage (mm)
      ok = pHBVTable->Lookup( key, m_col_lp, lp );        // soil moisture value above which ETact reaches ETpot (mm)
      ok = pHBVTable->Lookup( key, m_col_beta, Beta );    // parameter that determines the relative contribution to runoff from rain or snowmelt (-)
      ok = pHBVTable->Lookup( key, m_col_kperc, kPerc );  // Percolation constant (mm)
      ok = pHBVTable->Lookup( key, m_col_wp, WP );        // No ET below this value of soil moisture (-)
      ok = pHBVTable->Lookup( key, m_col_k0, k0 );        // Recession coefficient (day-1)
      ok = pHBVTable->Lookup( key, m_col_k1, k1 );        // Recession coefficient (day-1)
      ok = pHBVTable->Lookup( key, m_col_uzl, UZL );      // threshold parameter (mm)
      ok = pHBVTable->Lookup( key, m_col_wp, wp );        // permanent wilting point (mm)
      ok = pHBVTable->Lookup( key, m_col_k2, k2 );        // Recession coefficient (day-1)

      // Get the current Day/HRU climate data
      float tmax = 0.0f; float tmin = 0.0f;
      pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP,pHRU,pFlowContext->dayOfYear,precip);//mm
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMAX,pHRU,pFlowContext->dayOfYear,tmax);//C
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMIN, pHRU, pFlowContext->dayOfYear, tmin);//C
  //    ASSERT(tmax>-100.0f);

      airTemp = ( tmax + tmin) / 2.0f;
         
      // Get state variables from layers, these will be used to calculate some of the rates
      HRUPool *pHRULayer1 = pHRU->GetPool(1);
      HRUPool *pHRULayer0 = pHRU->GetPool(0);
      meltWater = ((float) pHRULayer1->m_volumeWater/pHRU->m_area*1000.f > 0.0f) ? float( pHRULayer1->m_volumeWater/pHRU->m_area*1000.f ) : 0.0f;//mm
      snow      = ((float) pHRULayer0->m_volumeWater/pHRU->m_area*1000.f > 0.0f) ? float( pHRULayer0->m_volumeWater/pHRU->m_area*1000.f ) : 0.0f;//mm
      float soilWater=float( pHRU->GetPool(2)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
      float upperGroundWater=float( pHRU->GetPool(3)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm

      //Calculate rates
      float gw = GroundWaterRechargeFraction(precip, soilWater, fc, Beta);
      float percolation = PercolationHBV(upperGroundWater, kPerc);//filling the deepest reservoir
      //float et = ET(soilWater,fc,wp,lp,airTemp,_month,pFlowContext->dayOfYear, etRc);

      //Write actual and reference crop ET back to FLOW
    //  pHRU->m_currentET = et;
     // pHRU->m_currentPET = etRc;

      //Partition precipitation into snow/rain and melt/refreeze
      if (airTemp < TT)//snowing and refreezing
         {
         snowing=precip*SFCF ;
         rain = 0.0f;
         refreezing = CFR*CFMAX*(TT-airTemp);  
         if (meltWater < refreezing)

            refreezing = meltWater; // don't refreeze any more than the water that is available in the 'Melt' layer
         }
      else//raining and melting
         {
         snowing=0.0f ;
         rain = precip;
         melting = CFMAX*(airTemp-TT);   //           
         if (meltWater > snow*0.1f)//meltwater transfers to soil, when beyond a threshold (CWH, usually 0.1) of the snowpack
            meltToSoil = meltWater-(snow*0.1f);

         if (snow < melting)
            melting = snow*0.1f;// don't melt (transfer to the soil) any more than the water that is available in the 'Snow' and 'Melt' layers
         }

      //Calculate the source/sink term for each HRUPool
      float q0=0.0f;float q2=0.0f;
    // #pragma omp parallel for 
      for ( int l=0; l < hruLayerCount; l++ )     
         {
         HRUPool *pHRUPool = pHRU->GetPool( l );
         float waterDepth=float( pHRUPool->m_volumeWater/pHRU->m_area*1000.0f );//mm
         float ss=0.0f;
         q0=0.0f;
         pHRUPool->m_wc=(float)pHRUPool->m_volumeWater/(pHRU->m_area*pHRUPool->m_depth);//of layer 0, saturated
        // pHRUPool->m_wc = (float)pHRUPool->m_volumeWater / pHRU->m_area *1000.0f;//mm water
         pHRUPool->m_wDepth = (float)pHRUPool->m_volumeWater / pHRU->m_area *1000.0f;//mm water
         switch( pHRUPool->m_pool )
            {             
            case 0://Snow
               pHRUPool->AddFluxFromGlobalHandler( snowing*pHRU->m_area/1000.0f,  FL_TOP_SOURCE );     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( melting*pHRU->m_area/1000.0f,  FL_BOTTOM_SINK );     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( refreezing*pHRU->m_area/1000.0f,  FL_BOTTOM_SOURCE );     //m3/d
               pHRU->m_depthSWE = pHRUPool->m_wDepth;
               break;             

            case 1://Melt
               pHRUPool->AddFluxFromGlobalHandler( melting*pHRU->m_area/1000.0f,     FL_TOP_SOURCE );     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( -rain*pHRU->m_area/1000.0f,       FL_SINK );     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( meltToSoil*pHRU->m_area/1000.0f,  FL_BOTTOM_SINK );     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( refreezing*pHRU->m_area/1000.0f,  FL_TOP_SINK );     //m3/d
               break; 

            case 2:  //Soil
               pHRUPool->AddFluxFromGlobalHandler( meltToSoil*(1-gw)*pHRU->m_area/1000.0f,  FL_TOP_SOURCE );     //m3/d
               break;

            case 3://Upper Groundwater
               q0 = Q0(waterDepth,k0, k1, UZL);
               if (q0+percolation > waterDepth)
                  percolation=0.0f;
               pHRUPool->AddFluxFromGlobalHandler( -meltToSoil*gw*pHRU->m_area/1000.0f,  FL_SINK );     //m3/d   TODO: Breaks with solute, FIX!!!
               pHRUPool->AddFluxFromGlobalHandler( percolation*pHRU->m_area/1000.0f,  FL_BOTTOM_SINK );     //m3/d 
               pHRUPool->AddFluxFromGlobalHandler( q0*pHRU->m_area/1000.0f, FL_STREAM_SINK );     //m3/d
               break;

            case 4://Lower Groundwater
               q2 = Q2(waterDepth, k2);
               pHRUPool->AddFluxFromGlobalHandler( percolation*pHRU->m_area/1000.0f,  FL_TOP_SOURCE );     //m3/d 
               pHRUPool->AddFluxFromGlobalHandler( q2*pHRU->m_area/1000.0f, FL_STREAM_SINK );     //m3/d
               break;

            default:
               //ss=0.0f;
               ;
            } // end of switch
         pHRU->m_currentRunoff=q0+q2;//mm_d
         Reach * pReach = pHRUPool->GetReach();
         if ( pReach )
            pReach->AddFluxFromGlobalHandler( ((q0+q2)/1000.0f*pHRU->m_area) ); //m3/d
         }
      }

//   Musle(pFlowContext);
   //  }
   return 0.0f;
   }
  

   float HBV::HBV_OnlyGroundwater(FlowContext *pFlowContext)
   {
	   if (pFlowContext->timing & GMT_INIT) // Init()
		   return HBV::InitHBV(pFlowContext, NULL);

	   if (!(pFlowContext->timing & GMT_CATCHMENT))   // only process these two timings, ignore everyting else
		   return 0;

	   // int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
	   int hruCount = pFlowContext->pFlowModel->GetHRUCount();

	   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable("HBV");   // store this pointer (and check to make sure it's not NULL)

	   int doy = pFlowContext->dayOfYear;  // int( fmod( timeInRun, 364 ) );  // zero based day of year
	   int _month = 0; int _day; int _year;
	   BOOL ok = ::GetCalDate(doy, &_year, &_month, &_day, TRUE);

	   //omp_set_num_threads( 8 );

	   // iterate through hrus/hrulayers 
#pragma omp parallel for firstprivate( pFlowContext )
	   for (int h = 0; h < hruCount; h++)
	   {
		   HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
         int col_hruid = pFlowContext->pEnvContext->pMapLayer->GetFieldCol("HRU_ID");//SWAT lulc field
         int hruid = -1;
         pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[0], col_hruid, hruid);
         if (hruid >= 1000000)
            { 
		      int hruLayerCount = pHRU->GetPoolCount();
		      //Initialize local variables
		      float CFMAX, TT, CFR, SFCF, CWH, Beta, kPerc, WP = 0.0f;
		      float k0, k1, UZL, k2, wp = 0.0f;
		      float fc, lp = 0.0f;
		      VData key;            // lookup key connecting rows in the csv files with IDU records

		      // Get Model Parameters
		      pFlowContext->pFlowModel->GetHRUData(pHRU, pHBVTable->m_iduCol, key, DAM_FIRST);  // get HRU key info for lookups
		      if (pHBVTable->m_type == DOT_FLOAT)
			      key.ChangeType(TYPE_FLOAT);

		      bool ok = pHBVTable->Lookup(key, m_col_k0, k0);        // Recession coefficient (day-1)
		      ok = pHBVTable->Lookup(key, m_col_k1, k1);        // Recession coefficient (day-1)
		      ok = pHBVTable->Lookup(key, m_col_uzl, UZL);      // threshold parameter (mm)
		      ok = pHBVTable->Lookup(key, m_col_k2, k2);        // Recession coefficient (day-1)
		      ok = pHBVTable->Lookup(key, m_col_kperc, kPerc);  // Percolation constant (mm)

		      // Get state variables from layers, these will be used to calculate some of the rates

		      float soilWater = float(pHRU->GetPool(0)->m_volumeWater / pHRU->m_area*1000.0f);//convert from m to mm
		      float upperGroundWater = float(pHRU->GetPool(1)->m_volumeWater / pHRU->m_area*1000.0f);//convert from m to mm

		      //Calculate rates
		      //float gw = GroundWaterRechargeFraction(precip, soilWater, fc, Beta);
		      float percolation = PercolationHBV_GW(pHRU->GetPool(0)->m_volumeWater, kPerc);//filling the deepest reservoir

		      //Calculate the source/sink term for each HRUPool
		      float q0 = 0.0f; float q2 = 0.0f;
		      // #pragma omp parallel for 
		      for (int l = 0; l < hruLayerCount; l++)
		      {
			      HRUPool *pHRUPool = pHRU->GetPool(l);
			      float waterDepth = float(pHRUPool->m_volumeWater / pHRU->m_area*1000.0f);//mm
			      float ss = 0.0f;
			      q0 = 0.0f;
			      pHRUPool->m_wc = (float)pHRUPool->m_volumeWater / (pHRU->m_area*pHRUPool->m_depth);//of layer 0, saturated
			     // pHRUPool->m_wc = (float)pHRUPool->m_volumeWater / pHRU->m_area *1000.0f;//mm water
			      pHRUPool->m_wDepth = (float)pHRUPool->m_volumeWater / pHRU->m_area *1000.0f;//mm water
			      switch (pHRUPool->m_pool)
			         {

			         case 0://Upper Groundwater
				         q0 = Q0(waterDepth, k0, k1, UZL);
				        // if (q0 + percolation > waterDepth)
					     //    percolation = 0.0f;
				       //  pHRUPool->AddFluxFromGlobalHandler(-meltToSoil * gw*pHRU->m_area / 1000.0f, FL_SINK);     //m3/d   TODO: Breaks with solute, FIX!!!
				       //  pHRUPool->AddFluxFromGlobalHandler(percolation*pHRU->m_area / 1000.0f, FL_BOTTOM_SINK);     //m3/d 
                     pHRUPool->AddFluxFromGlobalHandler(percolation , FL_BOTTOM_SINK);
				         pHRUPool->AddFluxFromGlobalHandler(q0*pHRU->m_area / 1000.0f, FL_STREAM_SINK);     //m3/d
				         break;

			         case 1://Lower Groundwater
				         q2 = Q2(waterDepth, k2);
				        // pHRUPool->AddFluxFromGlobalHandler(percolation*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d 
                     pHRUPool->AddFluxFromGlobalHandler(percolation  , FL_TOP_SOURCE);
				         pHRUPool->AddFluxFromGlobalHandler(q2*pHRU->m_area / 1000.0f, FL_STREAM_SINK);     //m3/d
                     pHRUPool->AddFluxFromGlobalHandler(0.025f * pHRUPool->m_volumeWater , FL_BOTTOM_SINK);
				         break;

			         default:
				         //ss=0.0f;
				         ;
			         } // end of switch
			      pHRU->m_currentRunoff = q0 + q2;//mm_d
			      Reach * pReach = pHRUPool->GetReach();
			      if (pReach)
				      pReach->AddFluxFromGlobalHandler(((q0 + q2) / 1000.0f*pHRU->m_area)); //m3/d
		      }
	      }
      }
	   return 0.0f;
   }



float HBV::HBV_WithRadiationDrivenSnow(FlowContext *pFlowContext)
   {
   if ( pFlowContext->timing & 1 ) // Init()
      return HBV::InitHBV( pFlowContext, NULL );

   if ( ! ( pFlowContext->timing & GMT_CATCHMENT) )   // only process these two timings, ignore everyting else
      return 0;

   // int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();  

   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)

   int doy = pFlowContext->dayOfYear;  // int( fmod( timeInRun, 364 ) );  // zero based day of year

   int _month = 0;int _day;int _year;
   BOOL ok = ::GetCalDate( doy, &_year, &_month, &_day, TRUE );

   omp_set_num_threads( 8 );

   float sn_abs = 0.6f; //adsorptivity of snow
   float lh_fus = 335;  //latent heat of fusion (kJ/kg)
   float rn = 0.0f;     //net radiation (kJ/m2/d)
   float rmelt = 0.0f;  //water fram radiation induced snow melt (kg/m2/d)

   // iterate through hrus/hrulayers 
   #pragma omp parallel for firstprivate( pFlowContext )
   for ( int h=0; h < hruCount; h++ )
      {

      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      int hruLayerCount = pHRU->GetPoolCount();
      //Initialize local variables
      float CFMAX,TT,CFR,SFCF,CWH,Beta,kPerc,WP=0.0f;
      float k0,k1,UZL,k2,wp=0.0f;
      float fc, lp=0.0f;
      VData key;
      float precip = 0.0f;
      float airTemp = -999.f;
      float snow =0.0f;
      float snowing=0.0f;
      float rain=0.0f;
      float refreezing=0.0f;
      float melting=0.0f;
      float meltWater=0.0f;
      float meltToSoil=0.0f;
      float etRc = 0.0f;
      float snowThreshold=-2.0f;
      float rainThreshold=4.0f;
      float sfcfCorrection=0.0f;

      //Get Model Parameters
      pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
      
      if (pHBVTable->m_type==DOT_FLOAT)
            key.ChangeType(TYPE_FLOAT);
      bool ok = pHBVTable->Lookup( key, m_col_cfmax, CFMAX );   // degree-day factor (mm oC-1 day-1)
      ok = pHBVTable->Lookup( key, m_col_tt, TT );        // threshold temperature (oC)
      ok = pHBVTable->Lookup( key, m_col_cfr, CFR );      // refreezing coefficient
      ok = pHBVTable->Lookup( key, m_col_sfcf, SFCF );    // snowfall correction factor
      ok = pHBVTable->Lookup( key, m_col_cwh, CWH );      // snowpack retains water till this portion is exceeded
      ok = pHBVTable->Lookup( key, m_col_fc, fc );        // maximum soil moisture storage (mm)
      ok = pHBVTable->Lookup( key, m_col_lp, lp );        // soil moisture value above which ETact reaches ETpot (mm)
      ok = pHBVTable->Lookup( key, m_col_beta, Beta );    // parameter that determines the relative contribution to runoff from rain or snowmelt (-)
      ok = pHBVTable->Lookup( key, m_col_kperc, kPerc );  // Percolation constant (mm)
      ok = pHBVTable->Lookup( key, m_col_k0, k0 );        // Recession coefficient (day-1)
      ok = pHBVTable->Lookup( key, m_col_k1, k1 );        // Recession coefficient (day-1)
      ok = pHBVTable->Lookup( key, m_col_uzl, UZL );      // threshold parameter (mm)
      ok = pHBVTable->Lookup( key, m_col_wp, wp );        // permanent wilting point (mm)
      ok = pHBVTable->Lookup( key, m_col_k2, k2 );        // Recession coefficient (day-1)
      ok = pHBVTable->Lookup(key, m_col_snow, snowThreshold);        // permanent wilting point (mm)
      ok = pHBVTable->Lookup(key, m_col_rain, rainThreshold);        // Recession coefficient (day-1)
      ok = pHBVTable->Lookup(key, m_col_sfcfCorrection, sfcfCorrection);        // Recession coefficient (day-1)

      // Get the current Day/HRU climate data
      pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP,  pHRU, pFlowContext->dayOfYear, precip);//mm
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN,   pHRU, pFlowContext->dayOfYear, airTemp);//C
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD,pHRU, pFlowContext->dayOfYear, rn);//W/m2
      rn*=3.6f;//kj/m2/d
      
    
      // Get state variables from layers, these will be used to calculate some of the rates
      HRUPool *pHRULayer1 = pHRU->GetPool(1);
      HRUPool *pHRULayer0 = pHRU->GetPool(0);
      meltWater = ((float) pHRULayer1->m_volumeWater/pHRU->m_area*1000.f > 0.0f) ? float( pHRULayer1->m_volumeWater/pHRU->m_area*1000.f ) : 0.0f;//mm
      snow      = ((float) pHRULayer0->m_volumeWater/pHRU->m_area*1000.f > 0.0f) ? float( pHRULayer0->m_volumeWater/pHRU->m_area*1000.f ) : 0.0f;//mm
      float soilWater=float( pHRU->GetPool(2)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
      float upperGroundWater=float( pHRU->GetPool(3)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm

      //Calculate rates
      float gw = GroundWaterRechargeFraction(precip, soilWater, fc, Beta);
      float percolation = Percolation(upperGroundWater, kPerc);//filling the deepest reservoir


      //Partition precipitation into snow/rain and melt/refreeze
   //   
      float iduRain = 0.0f;
      float iduSnow = 0.0f;
      float hruArea = 0.0f;
      for (int h = 0; h<pHRU->m_polyIndexArray.GetSize(); h++)
         {
         float iduMelt = 0.0f;
         float _iduRain = 0.0f;
         float _iduSnow = 0.0f;

         float area = 0.0f;
         float k = 0.5f;     //extinction coefficient 
         float swalb = 0.1f; //albedo (0-1)
         float lai = 0.0f;
         pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[h], pFlowContext->pFlowModel->m_colLai, lai);
         pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[h], pFlowContext->pFlowModel->m_colCatchmentArea, area);
         hruArea += area;
         float swinc = rn;  //shortwave incident
         float swfrac = 1 - exp(lai*-k);
         float swtrans = (1 - swalb)*swinc*(1 - swfrac); //shorwave transmitted through canopy (kg/m2/d)

         if (airTemp < rainThreshold)//snowing and refreezing
            {
            float slope = 1.0f / (rainThreshold-snowThreshold);
            float sfe = (slope*(rainThreshold - airTemp));
            if (pHRU->m_precip_wateryr>0.0f)
               {
               SFCF = 1.1924f - 0.0938f*lai - 1.1235f*(snow + meltWater) / pHRU->m_precip_wateryr;
               SFCF+=sfcfCorrection;
               }
            else
               SFCF=1.0f;
            if (SFCF>1.0f)
               SFCF=1.0f;
            _iduSnow = sfe*precip*SFCF;
         
            _iduRain = (1-sfe)*precip;
            if (airTemp < snowThreshold)
               {
               _iduSnow = precip*SFCF;
               _iduRain = 0.0f;
               }
            iduSnow+=_iduSnow*area;
            iduRain+=_iduRain*area;
            }

         else//raining and melting
            {     
            iduSnow += 0.0f;
            iduRain += precip*area;
            iduMelt = CFMAX*(airTemp-TT);   //         
            iduMelt += swtrans/lh_fus;      //include a radiation driven melt term
            melting +=iduMelt*area;
            }
         }

      melting = melting / hruArea;//area weight the HRU level melting term
      snowing = iduSnow / hruArea;
      rain = iduRain / hruArea;
      if (meltWater > snow*0.1f)//meltwater transfers to soil, when beyond a threshold (CWH, usually 0.1) of the snowpack
         meltToSoil = meltWater - (snow*0.1f);

      if (snow < melting)
         melting = snow;// don't melt (transfer to the soil) any more than the water that is available in the 'Snow' and 'Melt' layers
      if (airTemp<TT)
         refreezing = CFR*CFMAX*(TT - airTemp);
      if (meltWater < refreezing)
         refreezing = meltWater; // don't refreeze any more than the water that is available in the 'Melt' layer   

      //Calculate the source/sink term for each HRUPool
      float q0=0.0f;float q2=0.0f;
    // #pragma omp parallel for 
      for ( int l=0; l < hruLayerCount; l++ )     
         {
         HRUPool *pHRUPool = pHRU->GetPool( l );
         float waterDepth=float( pHRUPool->m_volumeWater/pHRU->m_area*1000.0f );//mm
         float ss=0.0f;
         q0=0.0f;
       //  pHRUPool->m_wc=(float)pHRUPool->m_volumeWater/(pHRU->m_area*pHRUPool->m_depth);//of layer 0, saturated
         pHRUPool->m_wc = waterDepth;//mm water

         switch( pHRUPool->m_pool )
            {             
            case 0://Snow
               //ss = snowing-melting+refreezing;//mm
               pHRUPool->AddFluxFromGlobalHandler( snowing*pHRU->m_area/1000.0f,  FL_TOP_SOURCE );     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( melting*pHRU->m_area / 1000.0f, FL_BOTTOM_SINK);     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( refreezing*pHRU->m_area/1000.0f,  FL_BOTTOM_SOURCE );     //m3/d
               break;             

            case 1://Melt
               pHRUPool->AddFluxFromGlobalHandler(melting*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( -rain*pHRU->m_area/1000.0f,       FL_SINK );     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( meltToSoil*pHRU->m_area/1000.0f,  FL_BOTTOM_SINK );     //m3/d
               pHRUPool->AddFluxFromGlobalHandler( refreezing*pHRU->m_area/1000.0f,  FL_TOP_SINK );     //m3/d
               //ss = melting+rain-meltToSoil-refreezing;//mm
               break; 

            case 2:  //Soil
               //ss = meltToSoil*(1-gw) - et; 
               pHRUPool->AddFluxFromGlobalHandler( meltToSoil*(1-gw)*pHRU->m_area/1000.0f,  FL_TOP_SOURCE );     //m3/d
     //          pHRUPool->AddFluxFromGlobalHandler( et*pHRU->m_area/1000.0f,  FL_SINK );     //m3/d
               break;

            case 3://Upper Groundwater
               q0 = Q0(waterDepth,k0, k1, UZL);
               //ss = meltToSoil*(gw) - percolation - q0;
               pHRUPool->AddFluxFromGlobalHandler( -meltToSoil*gw*pHRU->m_area/1000.0f,  FL_SINK );     //m3/d   TODO: Breaks with solute, FIX!!!
               pHRUPool->AddFluxFromGlobalHandler( percolation*pHRU->m_area/1000.0f,  FL_BOTTOM_SINK );     //m3/d 
               pHRUPool->AddFluxFromGlobalHandler( q0*pHRU->m_area/1000.0f, FL_STREAM_SINK );     //m3/d
               break;

            case 4://Lower Groundwater
               q2 = Q2(waterDepth, k2);
               //ss = percolation - q2; //filling the deepest reservoir
               pHRUPool->AddFluxFromGlobalHandler( percolation*pHRU->m_area/1000.0f,  FL_TOP_SOURCE );     //m3/d 
               pHRUPool->AddFluxFromGlobalHandler( q2*pHRU->m_area/1000.0f, FL_STREAM_SINK );     //m3/d
               break;

            default:
               //ss=0.0f;
               ;
            } // end of switch

         ///pHRUPool->m_horizontalExchange = ss*pHRU->m_area/1000.0f;     //m3/d
         ///pCatchment->m_contributionToReach += ((q0+q2)/1000.0f*pHRU->m_area); //m3/d
         //pHRUPool->AddFluxFromGlobalHandler( ss*pHRU->m_area/1000.0f,  );     //m3/d
         pHRU->m_currentRunoff = q0 + q2;//mm_d
         Reach * pReach = pHRUPool->GetReach();
         if ( pReach )
            pReach->AddFluxFromGlobalHandler( ((q0+q2)/1000.0f*pHRU->m_area) ); //m3/d
         }
      }
   
   return 0.0f;
   }


float HBV::HBV_WithIrrigation(FlowContext *pFlowContext)
   {
   if (pFlowContext->timing & 1) // Init()
      return HBV::InitHBV(pFlowContext, NULL);

   if ( ! ( pFlowContext->timing & GMT_CATCHMENT) )   // only process these two timings, ignore everyting else
      return 0;

   int hruCount = pFlowContext->pFlowModel->GetHRUCount();

   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable("HBV");   // store this pointer (and check to make sure it's not NULL)

   int doy = pFlowContext->dayOfYear;  // int( fmod( timeInRun, 364 ) );  // zero based day of year

   int _month = 0; int _day; int _year;
   BOOL ok = ::GetCalDate(doy, &_year, &_month, &_day, TRUE);

   omp_set_num_threads(8);

   float sn_abs = 0.6f; //adsorptivity of snow
   float lh_fus = 335.0f;  //latent heat of fusion (kJ/kg)
   float rn = 0.0f;     //net radiation (kJ/m2/d)
   float rmelt = 0.0f;  //water fram radiation induced snow melt (kg/m2/d)

   // iterate through hrus/hrulayers 
   #pragma omp parallel for firstprivate( pFlowContext )
   for (int h = 0; h < hruCount; h++)
      {

      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      int hruLayerCount = pHRU->GetPoolCount();
      //Initialize local variables
      float CFMAX, TT, CFR, SFCF, CWH, Beta, kPerc, WP = 0.0f;
      float k0, k1, UZL, k2, wp = 0.0f;
      float fc, lp = 0.0f;
      VData key;
      float precip = 0.0f;
      float airTemp = -999.f;
      float snow = 0.0f;
      float snowing = 0.0f;
      float rain = 0.0f;
      float refreezing = 0.0f;
      float melting = 0.0f;
      float meltWater = 0.0f;
      float meltToSoil = 0.0f;
      float etRc = 0.0f;
      float snowThreshold = -2.0f;
      float rainThreshold = 4.0f;
      float sfcfCorrection = 0.0f;

      //Get Model Parameters
      pFlowContext->pFlowModel->GetHRUData(pHRU, pHBVTable->m_iduCol, key, DAM_FIRST);  // get HRU info

      if (pHBVTable->m_type == DOT_FLOAT)
         key.ChangeType(TYPE_FLOAT);
      bool ok = pHBVTable->Lookup(key, m_col_cfmax, CFMAX);                // degree-day factor (mm oC-1 day-1)
      ok = pHBVTable->Lookup(key, m_col_tt, TT);                           // threshold temperature (oC)
      ok = pHBVTable->Lookup(key, m_col_cfr, CFR);                         // refreezing coefficient
      ok = pHBVTable->Lookup(key, m_col_sfcf, SFCF);                       // snowfall correction factor
      ok = pHBVTable->Lookup(key, m_col_cwh, CWH);                         // snowpack retains water till this portion is exceeded
      ok = pHBVTable->Lookup(key, m_col_fc, fc);                           // maximum soil moisture storage (mm)
      ok = pHBVTable->Lookup(key, m_col_lp, lp);                           // soil moisture value above which ETact reaches ETpot (mm)
      ok = pHBVTable->Lookup(key, m_col_beta, Beta);                       // parameter that determines the relative contribution to runoff from rain or snowmelt (-)
      ok = pHBVTable->Lookup(key, m_col_kperc, kPerc);                     // Percolation constant (mm)
      ok = pHBVTable->Lookup(key, m_col_wp, WP);                           // No ET below this value of soil moisture (-)
      ok = pHBVTable->Lookup(key, m_col_k0, k0);                           // Recession coefficient (day-1)
      ok = pHBVTable->Lookup(key, m_col_k1, k1);                           // Recession coefficient (day-1)
      ok = pHBVTable->Lookup(key, m_col_uzl, UZL);                         // threshold parameter (mm)
      ok = pHBVTable->Lookup(key, m_col_wp, wp);                           // permanent wilting point (mm)
      ok = pHBVTable->Lookup(key, m_col_k2, k2);                           // Recession coefficient (day-1)
      ok = pHBVTable->Lookup(key, m_col_snow, snowThreshold);              // permanent wilting point (mm)
      ok = pHBVTable->Lookup(key, m_col_rain, rainThreshold);              // Recession coefficient (day-1)
      ok = pHBVTable->Lookup(key, m_col_sfcfCorrection, sfcfCorrection);   // Recession coefficient (day-1)

      // Get the current Day/HRU climate data
      pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, pFlowContext->dayOfYear, precip);  // mm
      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, pFlowContext->dayOfYear, airTemp);  // C
      pFlowContext->pFlowModel->GetHRUClimate(CDT_SOLARRAD, pHRU, pFlowContext->dayOfYear, rn);    // W/m2
      rn *= 3.6f; // kj/m2/d


      // Get state variables from layers, these will be used to calculate some of the rates
      HRUPool *pHRULayer1 = pHRU->GetPool(1);
      HRUPool *pHRULayer0 = pHRU->GetPool(0);
      meltWater = ((float)pHRULayer1->m_volumeWater / pHRU->m_area*1000.f > 0.0f) ? float(pHRULayer1->m_volumeWater / pHRU->m_area*1000.f) : 0.0f;//mm
      snow = ((float)pHRULayer0->m_volumeWater / pHRU->m_area*1000.f > 0.0f) ? float(pHRULayer0->m_volumeWater / pHRU->m_area*1000.f) : 0.0f;//mm

      float hruIrrigatedArea = 0.0f;
      for (int idu = 0; idu < pHRU->m_polyIndexArray.GetSize(); idu++)
         {
         float area = 0.0f;
         int irrigated = 0;
         pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[idu], pFlowContext->pFlowModel->m_colIrrigation, irrigated);
         pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[idu], pFlowContext->pFlowModel->m_colCatchmentArea, area);
         if (irrigated != 0)
            hruIrrigatedArea += area;
         }

      float ratioIrrigated = 0.0f;
      if ( pHRU->m_area > 0.0f )
         ratioIrrigated = hruIrrigatedArea / pHRU->m_area;

      float soilWater =  0.0f;
      if ( pHRU->m_area - hruIrrigatedArea > 0.0f )
         soilWater = float(pHRU->GetPool(2)->m_volumeWater / (pHRU->m_area - hruIrrigatedArea)*1000.0f);//convert from m to mm
     
      float irrigatedSoilWater = 0.0f;
      if (hruIrrigatedArea > 0.0f)
         irrigatedSoilWater = float(pHRU->GetPool(3)->m_volumeWater / hruIrrigatedArea*1000.0f);//convert from m to mm
     
      float upperGroundWater = 0.0f;
      if (pHRU->m_area > 0.0f)
         upperGroundWater = float(pHRU->GetPool(4)->m_volumeWater / pHRU->m_area*1000.0f);//convert from m to mm

      //Calculate rates
      // gwIrrigated is the proportion of rain/snowmelt that bypasses the irrigated soil bucket, and is added directly to GW
      float gwIrrigated = GroundWaterRechargeFraction(precip, irrigatedSoilWater, fc, Beta); 
      // gwNonIrrigated is the proportion of rain/snowmelt that bypasses the non-irrigated soil bucket, and is added directly to GW
      float gwNonIrrigated = GroundWaterRechargeFraction(precip, soilWater, fc, Beta);
      float percolation = Percolation(upperGroundWater, kPerc);//filling the deepest reservoir

      //Partition precipitation into snow/rain and melt/refreeze
      //   
      float iduRain = 0.0f;
      float iduSnow = 0.0f;
      float hruArea = 0.0f;
      for (int idu = 0; idu <pHRU->m_polyIndexArray.GetSize(); idu++)
         {
         float iduMelt = 0.0f;
         float _iduRain = 0.0f;
         float _iduSnow = 0.0f;

         float area = 0.0f;
         float k = 0.5f;     //extinction coefficient 
         float swalb = 0.1f; //albedo (0-1)
         float lai = 0.0f;
         pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[idu], pFlowContext->pFlowModel->m_colLai, lai);
         pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[idu], pFlowContext->pFlowModel->m_colCatchmentArea, area);
         hruArea += area;
         float swinc = rn;  //shortwave incident
         float swfrac = 1 - exp(lai*-k);
         float swtrans = (1 - swalb)*swinc*(1 - swfrac); //shorwave transmitted through canopy (kg/m2/d)

         if (airTemp < rainThreshold)//snowing and raining
            {
            float slope = 1.0f / (rainThreshold - snowThreshold);
            float sfe = (slope*(rainThreshold - airTemp));
            if (pHRU->m_precip_wateryr>0.0f)
               {
               SFCF = 1.1924f - 0.0938f*lai - 1.1235f*(snow + meltWater) / pHRU->m_precip_wateryr;
               SFCF += sfcfCorrection;
               }
            else
               SFCF = 1.0f;
            if (SFCF>1.0f)
               SFCF = 1.0f;
            if (SFCF<0.0f)
               SFCF=0.0f;
            _iduSnow = sfe*precip*SFCF;

            _iduRain = (1 - sfe)*precip;
            if (airTemp < snowThreshold)
               {
               _iduSnow = precip*SFCF;
               _iduRain = 0.0f;
               }
            iduSnow += _iduSnow*area;
            iduRain += _iduRain*area;
            }

         else//raining and melting
            {
            iduSnow += 0.0f;
            iduRain += precip*area;
            iduMelt = CFMAX*(airTemp - rainThreshold);   //         
            iduMelt += swtrans / lh_fus;      //include a radiation driven melt term
            melting += iduMelt*area;
            }
         }//end polyIndexArray

      melting = melting / hruArea;//area weight the HRU level melting term
      if (melting<0.0f)
         melting=0.0f;
      snowing = iduSnow / hruArea;
      rain = iduRain / hruArea;

      if (meltWater > snow*0.1f)//meltwater transfers to soil, when beyond a threshold (CWH, usually 0.1) of the snowpack
         meltToSoil = meltWater - (snow*0.1f);

      if (snow < melting)
         melting = snow*0.1f;// don't melt (transfer to the soil) any more than the water that is available in the 'Snow' and 'Melt' layers
      if (melting<0.0f)
         melting=0.0f;
      if (airTemp < snowThreshold)
         {
         refreezing = CFR*CFMAX*(snowThreshold - airTemp);
         if (meltWater < refreezing)
            refreezing = meltWater; // don't refreeze any more than the water that is available in the 'Melt' layer  
         } 
      //recharge into the irrigated soil bucket, from rain/melt (m/d)
      float soilRechargeFromIrrigated = meltToSoil*(1 - gwIrrigated);
      //recharge into the nonirrigated soil bucket, from rain/melt (m/d)
      float soilRechargeFromNonIrrigated = meltToSoil*(1 - gwNonIrrigated);
     // float percentIrrigated = 0.5f;

      pHRU->m_currentRainfall = rain; //m/d
      pHRU->m_currentSnowFall = snowing;//m/d

      //Calculate the source/sink term for each HRUPool
      float q0 = 0.0f; float q2 = 0.0f;
      // #pragma omp parallel for 
      for (int l = 0; l < hruLayerCount; l++)
         {
         HRUPool *pHRUPool = pHRU->GetPool(l);
         float waterDepth = float(pHRUPool->m_volumeWater / pHRU->m_area*1000.0f);//mm
         float ss = 0.0f;
         q0 = 0.0f;
         //  pHRUPool->m_wc=(float)pHRUPool->m_volumeWater/(pHRU->m_area*pHRUPool->m_depth);//of layer 0, saturated
         pHRUPool->m_wc = waterDepth;//mm water - 
         pHRUPool->m_wDepth = waterDepth;

         switch (pHRUPool->m_pool)
            {
            case 0://Snow
               //ss = snowing-melting+refreezing;//mm
               pHRUPool->AddFluxFromGlobalHandler(snowing*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d
               pHRUPool->AddFluxFromGlobalHandler(melting*pHRU->m_area / 1000.0f, FL_BOTTOM_SINK);     //m3/d
               pHRUPool->AddFluxFromGlobalHandler(refreezing*pHRU->m_area / 1000.0f, FL_BOTTOM_SOURCE);     //m3/d
               break;

            case 1://Melt
               pHRUPool->AddFluxFromGlobalHandler(melting*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d
               pHRUPool->AddFluxFromGlobalHandler(-rain*pHRU->m_area / 1000.0f, FL_SINK);     //m3/d
               pHRUPool->AddFluxFromGlobalHandler(meltToSoil*pHRU->m_area / 1000.0f, FL_BOTTOM_SINK);     //m3/d
               pHRUPool->AddFluxFromGlobalHandler(refreezing*pHRU->m_area / 1000.0f, FL_TOP_SINK);     //m3/d
               //ss = melting+rain-meltToSoil-refreezing;//mm
               break;


            case 2:  //UnirrigatedSoil
               //ss = meltToSoil*(1-gw) - et; 
               pHRUPool->m_wc = soilWater;//mm water
               pHRUPool->AddFluxFromGlobalHandler(soilRechargeFromNonIrrigated*(pHRU->m_area - hruIrrigatedArea) / 1000.0f, FL_TOP_SOURCE);     //m3/d                            //          pHRUPool->AddFluxFromGlobalHandler( et*pHRU->m_area/1000.0f,  FL_SINK );     //m3/d
               break;
            case 3:  //IrrigatedSoil
               //ss = meltToSoil*(1-gw) - et; 
               pHRUPool->m_wc = irrigatedSoilWater;
               pHRUPool->AddFluxFromGlobalHandler(soilRechargeFromIrrigated*hruIrrigatedArea / 1000.0f, FL_TOP_SOURCE);     //m3/d
               break;

            case 4://Upper Groundwater
               q0 = Q0(waterDepth, k0, k1, UZL);
               //ss = meltToSoil*(gw) - percolation - q0;
               pHRUPool->AddFluxFromGlobalHandler(-meltToSoil*((gwIrrigated*ratioIrrigated) + (gwNonIrrigated*(1-ratioIrrigated)))*pHRU->m_area / 1000.0f, FL_SINK);     //m3/d   TODO: Breaks with solute, FIX!!!
               pHRUPool->AddFluxFromGlobalHandler(percolation*pHRU->m_area / 1000.0f, FL_BOTTOM_SINK);     //m3/d 
               pHRUPool->AddFluxFromGlobalHandler(q0*pHRU->m_area / 1000.0f, FL_STREAM_SINK);     //m3/d
               break;

            case 5://Lower Groundwater
               q2 = Q2(waterDepth, k2);
               //ss = percolation - q2; //filling the deepest reservoir
               pHRUPool->AddFluxFromGlobalHandler(percolation*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d 
               pHRUPool->AddFluxFromGlobalHandler(q2*pHRU->m_area / 1000.0f, FL_STREAM_SINK);     //m3/d
               break;

            default:
               //ss=0.0f;
               ;
            } // end of switch

         ///pHRUPool->m_horizontalExchange = ss*pHRU->m_area/1000.0f;     //m3/d
         ///pCatchment->m_contributionToReach += ((q0+q2)/1000.0f*pHRU->m_area); //m3/d
         //pHRUPool->AddFluxFromGlobalHandler( ss*pHRU->m_area/1000.0f,  );     //m3/d
         pHRU->m_currentRunoff = q0 + q2;//mm_d
         Reach * pReach = pHRUPool->GetReach();
         if (pReach)
            pReach->AddFluxFromGlobalHandler(((q0 + q2) / 1000.0f*pHRU->m_area)); //m3/d
         }
      }

   return 0.0f;
   }

float HBV::HBV_Vertical( FlowContext *pFlowContext )
   {
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_cfmax = pHBVTable->GetFieldCol( "CFMAX" );         // get the location of the parameter in the table    
   int m_col_tt = pHBVTable->GetFieldCol( "TT" );  
   int m_col_sfcf = pHBVTable->GetFieldCol( "SFCF" ); 
   int m_col_cwh = pHBVTable->GetFieldCol( "CWH" ); 
   int m_col_cfr = pHBVTable->GetFieldCol( "CFR" );  
   int m_col_lp = pHBVTable->GetFieldCol( "LP" );  
   int m_col_fc = pHBVTable->GetFieldCol( "FC" );  
   int m_col_beta = pHBVTable->GetFieldCol( "BETA" );  
   int m_col_kperc = pHBVTable->GetFieldCol( "PERC" ); 
   int m_col_wp = pHBVTable->GetFieldCol( "WP" ); 

   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = pFlowContext->pFlowModel->GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();
      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         float precip = pHRU->m_currentPrecip*1000.0f;//convert from m/d to mm/d
         float temp = pHRU->m_currentAirTemp;
         int hruLayerCount=pHRU->GetPoolCount();
         float CFMAX,TT,CFR,SFCF,CWH,FC, LP,Beta,kPerc,WP=0.0f;
         VData key;

         pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
         if (pHBVTable->m_type==DOT_FLOAT)
              key.ChangeType(TYPE_FLOAT);
         bool ok = pHBVTable->Lookup( key, m_col_cfmax, CFMAX );   // degree-day factor (mm oC-1 day-1)
         ok = pHBVTable->Lookup( key, m_col_tt, TT );        // threshold temperature (oC)
         ok = pHBVTable->Lookup( key, m_col_cfr, CFR );      // refreezing coefficient
         ok = pHBVTable->Lookup( key, m_col_sfcf, SFCF );    // snowfall correction factor
         ok = pHBVTable->Lookup( key, m_col_cwh, CWH );      // snowpack retains water till this portion is exceeded
         ok = pHBVTable->Lookup( key, m_col_fc, FC );        // maximum soil moisture storage (mm)
         ok = pHBVTable->Lookup( key, m_col_lp, LP );        // soil moisture value above which ETact reaches ETpot (mm)
         ok = pHBVTable->Lookup( key, m_col_beta, Beta );    // parameter that determines the relative contribution to runoff from rain or snowmelt (-)
         ok = pHBVTable->Lookup( key, m_col_kperc, kPerc );  // Recession coefficient (day-1)
         ok = pHBVTable->Lookup( key, m_col_wp, WP );  // Recession coefficient (day-1)
         float out=0.0f;
         for ( int l=0; l < hruLayerCount; l++ )     //All but the bottom layer
            {
            HRUPool *pHRUPool = pHRU->GetPool( l );
            float value = 0; float throughfall=0.0f;
            float waterDepth=float( pHRUPool->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm

            switch( pHRUPool->m_pool )
               {
               case 0:
                  value = GroundWaterRecharge(precip, waterDepth, FC, Beta);
                  break;  
               case 1:
                  value = Percolation(waterDepth, kPerc);//filling the deepest reservoir
                  out=Percolation(waterDepth, kPerc);
                  break;
               case 2:
                  value = out;
               default:
                  value=0.0f;
               } // end of switch
            pHRUPool->m_verticalDrainage = value*pHRU->m_area/1000.0f;     //m3/d
        //    pHRUPool->m_verticalDrainage = 0.0f;     //m3/d
          //  pHRUPool->m_wc = float( pHRUPool->m_volumeWater/pHRU->m_area );//m3/d
            pHRUPool->m_wc = (float)pHRUPool->m_volumeWater / pHRU->m_area *1000.0f;//mm water
           }
         }
      }
   return 0.0f;
   }


float HBV::N_Transformations( FlowContext *pFlowContext )
   {
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_cfmax = pHBVTable->GetFieldCol( "CFMAX" );         // get the location of the parameter in the table    

   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = pFlowContext->pFlowModel->GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();
      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         float precip = pHRU->m_currentPrecip*1000.0f;//convert from m/d to mm/d
         float temp = pHRU->m_currentAirTemp;
         int hruLayerCount=pHRU->GetPoolCount();
         float CFMAX=0.0f;
         VData key;

         pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
         if (pHBVTable->m_type==DOT_FLOAT)
              key.ChangeType(TYPE_FLOAT);
         bool ok = pHBVTable->Lookup( key, m_col_cfmax, CFMAX );   // degree-day factor (mm oC-1 day-1)

         for (int k=0;k<pFlowContext->svCount;k++)
            {
            for ( int l=0; l < hruLayerCount; l++ )     
               {
               HRUPool *pHRUPool = pHRU->GetPool( l );
               float value = 0; float throughfall=0.0f;
               float waterDepth = float( pHRUPool->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
               switch( pHRUPool->m_pool )
                  {
                  case 0://layer 0
                     {
                        value = 0.0f;

                        }
                     break;  
                  case 1://layer 1
                        {
                        value = 0.0f;

                        }
                     break;
                  case 2://layer 2
                        {
                        value = 0.0f;

                        }
                     break;

                  default:
                     value=0.0f;
                   
                  } // end of switch
               pHRUPool->m_svArrayTrans[k] = value;  
               }
           }
         }
      }
   return 0.0f;
   }


float HBV::HBV_Horizontal( FlowContext *pFlowContext )
   {
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_k0 = pHBVTable->GetFieldCol( "K0" );  
   int m_col_k1 = pHBVTable->GetFieldCol( "K1" );  
   int m_col_uzl = pHBVTable->GetFieldCol( "UZL" );  
   int m_col_k2 = pHBVTable->GetFieldCol( "K2" ); 
   int m_col_wp = pHBVTable->GetFieldCol( "WP" );  
   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = pFlowContext->pFlowModel->GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();

      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         int hruLayerCount=pHRU->GetPoolCount();

         float k0,k1,UZL,k2,wp=0.0f;
         VData key;

         pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info

         if (pHBVTable->m_type==DOT_FLOAT)
              key.ChangeType(TYPE_FLOAT);


         bool ok = pHBVTable->Lookup( key, m_col_k0, k0 );        // Recession coefficient (day-1)
         ok = pHBVTable->Lookup( key, m_col_k1, k1 );        // Recession coefficient (day-1)
         ok = pHBVTable->Lookup( key, m_col_uzl, UZL );      // threshold parameter (mm)
         ok = pHBVTable->Lookup( key, m_col_wp, wp );      // permanent wilting point (mm)
         ok = pHBVTable->Lookup( key, m_col_k2, k2 );        // Recession coefficient (day-1)


         for ( int l=0; l < hruLayerCount; l++ )     //All but the bottom layer
            { 
            HRUPool *pHRUPool = pHRU->GetPool( l );

            float value = 0;
            float d= float( pHRUPool->m_volumeWater/pHRU->m_area*1000.0f );//mm - 
            switch( pHRUPool->m_pool )
               {
               case 1:
                  value = Q0(d,k0, k1, UZL);
                  break;      
               case 2:
                  value = Q2(d, k2);
                  break;

               default:
                  value=0.0f;
                } 
            if (pHRUPool->m_wc*1000.0f < wp*0.001f)
               pHRUPool->m_contributionToReach=0.0f;
            else
               {
               pHRUPool->m_contributionToReach=value*pHRU->m_area/1000.0f;   // m3/d
              // pHRUPool->m_contributionToReach=0.0f;   // m3/d
               pCatchment->m_contributionToReach+=value*pHRU->m_area/1000.0f; // m3/d
               //pCatchment->m_contributionToReach=0.0f;
               }

             }
         }
      }   
   return 0.0f;
   }   

float HBV::Snow( float waterDepth, float precip, float temp,float CFMAX, float CFR, float TT  )
   {
   float melt=0.0f;
   if (temp >= TT) //it is melting
      {
      melt=CFMAX*(temp-TT);//mm/d
      if (waterDepth < melt)
         melt=waterDepth;
      }
   else//snowing
      melt=0.0f;
   
   if (melt<0.0f)
      melt=0.0f;

   return melt;
   }

float HBV::Melt( float waterDepth, float precip, float temp,float CFMAX, float CFR, float TT  )
   {
   float melt=0.0f;
   if (temp >= TT) //it is melting
      {
      melt=CFMAX*(temp-TT);//mm/d
      if (waterDepth < melt)
         melt=waterDepth;
      }
   else//melt is refreezing - we want to add this quantity to the layer ABOVE
      melt=CFR*CFMAX*(temp-TT);
   
   if (melt<0.0f)
      melt=0.0f;

   return melt;
   }

float HBV::GroundWaterRecharge(float precip, float waterDepth, float FC,  float Beta )
   {
   float value=0.0f;
   float lossFraction = (pow((waterDepth/FC),Beta));
   if (lossFraction > 1.0f)
      lossFraction=1.0f;

   if (waterDepth>0)
      value=precip*lossFraction;

   return value;

   }

float HBV::GroundWaterRechargeFraction(float precip, float waterDepth, float FC,  float Beta )
   {
   float value=0.0f;
   float lossFraction = (pow((waterDepth/FC),Beta));
   if (lossFraction > 1.0f)
      lossFraction=1.0f;
   return lossFraction;
   }

float HBV::Percolation( float waterDepth, float kPerc )
   {   
   float value=0.0f;
   if (waterDepth>5.0f)
      value = waterDepth*kPerc;
   //if (waterDepth>kPerc*2.0f)
    //  value=waterDepth*kPerc;
   return value;

   }

float HBV::PercolationHBV(float waterDepth, float kPerc)
   {
   float value = 0.0f;
   if (waterDepth>kPerc)
      value = kPerc;
   return value;
   }

float HBV::PercolationHBV_GW(float waterVolume, float kPerc)
{
   float value = kPerc*waterVolume;
   return value;
}



float HBV::Q0( float waterDepth, float k0, float k1, float UZL )
   {
   float value=0.0f;
   if (waterDepth>0 && waterDepth > UZL)
      value = (k0*(waterDepth-UZL))+(k1*waterDepth);
   else if (waterDepth>0 && waterDepth <= UZL)
      value=k1*waterDepth;
   return value;
   }
float HBV::Q2( float waterDepth, float k2 )
   {
   float value=0.0f;
   if (waterDepth>0)
      value=k2*waterDepth;
   return value;
   }
float HBV::ET( float swc, float fc, float wp, float lp, float temp, int month, int dayOfYear, float &etRc )
   {
   //Express all parameters in mm
   etRc=CalcET(temp, month, dayOfYear);
   float et = 0.0f;
   if (swc > lp)
      et = etRc;
   else if (swc <= wp)
      et=0.0f;
   else
      {
      float slope = 1.0F/(lp-wp);
      float fTheta = slope*(swc-wp);        
      et = etRc * fTheta;
      }
   return et;//mm/d
   }

float HBV::CalcET(  float temp, int month , int dayOfYear ) //hargreaves equation
   {
   float pi = 3.141592f;
   //float latitude = 17.0f/57.2957795f;//convertToRadians
   float latitude = 45.0f/57.2957795f;//convertToRadians
   float solarDeclination=0.4903f*sin(2.0f*pi/365.0f*dayOfYear-1.405f);

   float sd_degrees = solarDeclination*57.2957795f ;
   float sunsetHourAngle = acos(-tan(latitude)*tan(solarDeclination));
   float sHA_degress = sunsetHourAngle*57.2957795f ;
   float N = 24.0f/pi*sunsetHourAngle;
   float dr = 1+0.033f*cos(2*pi/365.f*dayOfYear);
   float So_mm_d=15.392f*dr*(sunsetHourAngle*sin(latitude)*sin(solarDeclination) + cos(latitude)*cos(solarDeclination)*sin(sunsetHourAngle)); 
   int *monthlyTempDiff = new int[ 12 ];

   //mean monthly diuranal temp differences from SALEM OREGON
   monthlyTempDiff[0]=7;
   monthlyTempDiff[1]=9;
   monthlyTempDiff[2]=9;
   monthlyTempDiff[3]=12;
   monthlyTempDiff[4]=14;
   monthlyTempDiff[5]=14;
   monthlyTempDiff[6]=17;
   monthlyTempDiff[7]=17;
   monthlyTempDiff[8]=14;
   monthlyTempDiff[9]=13;
   monthlyTempDiff[10]=8;
   monthlyTempDiff[11]=7;

   //float etrc = 0.0023f * So_mm_d * sqrt((float)monthlyTempDiff[month-1]) * ( temp + 17.8f ) * 7.5f ; //gives reference crop et in mm / d*/
   

   float etrc = 0.0023f * So_mm_d * sqrt((float)monthlyTempDiff[month-1]) * ( temp + 17.8f ) * 2.5f; //gives reference crop et in mm / d*/
 
   delete [] monthlyTempDiff;
  // etrc = 0.0075f * So_mm_d * 6.0f * ( temp + 17.8f ) ; //gives reference crop et in mm / d*/
   return etrc; 
   }


float HBV::ETFluxHandler( FlowContext *pFlowContext )
   {
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;
   float timeInRun = pFlowContext->pEnvContext->yearOfRun * 365.0f;   // days
   int doy = pFlowContext->dayOfYear;  // int( fmod( timeInRun, 364 ) );  // zero based day of year
   int _month = 0;int _day;int _year;
   BOOL ok = ::GetCalDate( doy, &_year, &_month, &_day, TRUE );

   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_lp = pHBVTable->GetFieldCol( "LP" );  
   int m_col_fc = pHBVTable->GetFieldCol( "FC" );  
   int m_col_wp = pHBVTable->GetFieldCol( "WP" ); 

    float fc,wp,lp =0.0f;
    VData key;
    pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
    float airTemp=10.0f;
    pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN,pHRU,pFlowContext->dayOfYear,airTemp);
    if (pHBVTable->m_type==DOT_FLOAT)
         key.ChangeType(TYPE_FLOAT);
    ok = pHBVTable->Lookup( key, m_col_fc, fc );        // maximum soil moisture storage (mm)
    ok = pHBVTable->Lookup( key, m_col_lp, lp );        // soil moisture value above which ETact reaches ETpot (mm)
    ok = pHBVTable->Lookup( key, m_col_wp, wp );  //
    float etRc=0.0f;
   float et = ET(float( pHRUPool->m_volumeWater/pHRU->m_area*1000.0f ),fc,wp,lp,airTemp,_month,pFlowContext->dayOfYear, etRc);
   
   pHRU->m_currentET = et;
   pHRU->m_currentMaxET = etRc;
   return et/1000.0f*pHRU->m_area;// m3/d
   }


float HBV::PrecipFluxHandler( FlowContext *pFlowContext )
   {
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;

   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_tt = pHBVTable->GetFieldCol( "TT" ); 
   int m_col_sfcf = pHBVTable->GetFieldCol( "SFCF" );

   float tt =0.0f;float SFCF=0.7f; 
   VData key;
   pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
   if (pHBVTable->m_type==DOT_FLOAT)
        key.ChangeType(TYPE_FLOAT);
   bool ok = pHBVTable->Lookup( key, m_col_tt, tt );      //threshold temperature
   ok = pHBVTable->Lookup( key, m_col_sfcf, SFCF );      //snowfall correction 
   float precip = 0.0f;
   float airTemp = -999.f;
   float snow =0.0f;
   pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP,pHRU,pFlowContext->dayOfYear,precip);
   pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN,pHRU,pFlowContext->dayOfYear,airTemp);
   precip/=1000.0f;
   if (airTemp < tt)
      {
      snow=precip*SFCF ;
      precip = 0.0f;
      }
   else
      {
      snow=0.0f ;
      precip = precip;
      }

   float value=0.0f;
   if (pHRUPool->m_pool==0 && airTemp < tt )//it is snowing and the snow layer
      value = snow;
   if (pHRUPool->m_pool==0 && airTemp >= tt )//it is raining and the snow layer...the precip will be added to the melt
      value = 0.0f;
   if (pHRUPool->m_pool==1 && airTemp < tt )//it is snowing and the melt layer..the precip will be added to the snow
      value = 0.0f;
   if (pHRUPool->m_pool==1 && airTemp >= tt )//it is raining and the melt layer
      value = precip;

   return value*pHRU->m_area;// m3/d
   }

float HBV::SnowFluxHandler( FlowContext *pFlowContext )
   {
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;
  
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_tt = pHBVTable->GetFieldCol( "TT" ); 
   int m_col_cfr = pHBVTable->GetFieldCol( "CFR" );
   int m_col_cfmax = pHBVTable->GetFieldCol( "CFMAX" );
   int m_col_sfcf = pHBVTable->GetFieldCol( "SFCF" );

   float TT =0.0f; float CFR=0.05f; float SFCF = 0.7f; float CFMAX=5.0f;
   VData key;
   pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
   if (pHBVTable->m_type==DOT_FLOAT)
        key.ChangeType(TYPE_FLOAT);
   bool ok = pHBVTable->Lookup( key, m_col_tt, TT );      //threshold temperature
   ok = pHBVTable->Lookup( key, m_col_cfr, CFR );      //threshold temperature
   ok = pHBVTable->Lookup( key, m_col_cfmax, CFMAX );      //threshold temperature
   ok = pHBVTable->Lookup( key, m_col_cfmax, SFCF );      //snowfall correction
   float value=0.0f;

   HRUPool *pHRULayer1 = pHRU->GetPool(1);
   HRUPool *pHRULayer0 = pHRU->GetPool(0);
   float meltWater = ((float) pHRULayer1->m_volumeWater/pHRU->m_area*1000.f > 0.0f) ? float( pHRULayer1->m_volumeWater/pHRU->m_area*1000.f ) : 0.0f;
   float snow      = ((float) pHRULayer0->m_volumeWater/pHRU->m_area*1000.f > 0.0f) ? float( pHRULayer0->m_volumeWater/pHRU->m_area*1000.f ) : 0.0f;
   float melting=0.0f;float refreezing=0.0f;
   float smRatio = snow/(snow+meltWater);
   float meltToSoil=0.0f;
   float precip = 0.0f;
   float airTemp = -999.f;

   pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP,pHRU,pFlowContext->dayOfYear,precip);
   pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN,pHRU,pFlowContext->dayOfYear,airTemp);

   if (airTemp < TT)//cold
      {
      refreezing = CFR*CFMAX*(TT-airTemp);  
      if (meltWater < refreezing)
         refreezing = meltWater; // don't refreeze any more than the water that is available in the 'Melt' layer
      }
   if (airTemp >= TT)//warm
      {
      //the snowpack melts
      melting = CFMAX*(airTemp-TT);   //
      //meltwater transfers to soil, when beyond a threshold (CWH, usually 0.1) of the snowpack
      if (meltWater > snow*0.1f)
         meltToSoil = meltWater-(snow*0.1f);

      if (snow < melting)
         melting = snow;// don't melt (transfer to the soil) any more than the water that is available in the 'Snow' and 'Melt' layers
      }

   if (pHRUPool->m_pool==0 && airTemp < TT )//it is snowing and the snow layer
      value = refreezing;//Add refreezing melt
   
   if (pHRUPool->m_pool==0 && airTemp >= TT )//it is raining and the snow layer...the precip will be added to the melt
      value=-melting;//mm/d of melting snow
   
   if (pHRUPool->m_pool==1 && airTemp < TT )//it is snowing and the melt layer..we are refreezing melt
      value=-refreezing;//mm/d or refreezing melt
  
   if (pHRUPool->m_pool==1 && airTemp >= TT )//it is raining and the melt layer
      value = melting - meltToSoil;
  
   if (pHRUPool->m_pool==2 && airTemp >= TT )//it is raining and the soil layer
      {
       int m_col_fc = pHBVTable->GetFieldCol( "FC" );  
       int m_col_beta = pHBVTable->GetFieldCol( "BETA" );  
       float  FC, Beta=0.0f;
       VData key;
       pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
       if (pHBVTable->m_type==DOT_FLOAT)
          key.ChangeType(TYPE_FLOAT);
  
      bool ok = pHBVTable->Lookup( key, m_col_fc, FC );        // maximum soil moisture storage (mm)
      ok = pHBVTable->Lookup( key, m_col_beta, Beta );    // parameter that determines the relative contribution to runoff from rain or snowmelt (-)

      float aWD= float( pHRU->GetPool(2)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
      float gw = GroundWaterRechargeFraction(precip, aWD, FC, Beta);
      value =  meltToSoil*(1-gw);//add the melting snow
      }
   if (pHRUPool->m_pool==3 && airTemp >= TT )//it is raining and the below soil layer
      {
       int m_col_fc = pHBVTable->GetFieldCol( "FC" );  
       int m_col_beta = pHBVTable->GetFieldCol( "BETA" );  
       float  FC, Beta=0.0f;
       VData key;
       pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
       if (pHBVTable->m_type==DOT_FLOAT)
          key.ChangeType(TYPE_FLOAT);
  
      bool ok = pHBVTable->Lookup( key, m_col_fc, FC );        // maximum soil moisture storage (mm)
      ok = pHBVTable->Lookup( key, m_col_beta, Beta );    // parameter that determines the relative contribution to runoff from rain or snowmelt (-)

      float aWD=float( pHRU->GetPool(2)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
      float gw = GroundWaterRechargeFraction(precip, aWD, FC, Beta);
      value =  meltToSoil*(gw);//add the melting snow
      }
   if (pHRUPool->m_pool==3)
      {
      int col_p = pHBVTable->GetFieldCol( "PERC" );  
      float  kPerc=0.0f;
      VData key;
      pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
      if (pHBVTable->m_type==DOT_FLOAT)
          key.ChangeType(TYPE_FLOAT);
  
      bool ok = pHBVTable->Lookup( key, col_p, kPerc );        // maximum soil moisture storage (mm)
      float aWD=float( pHRU->GetPool(3)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
      value -= Percolation(aWD, kPerc);//filling the deepest reservoir
      }

   if (pHRUPool->m_pool==4)
      {
      int col_p = pHBVTable->GetFieldCol( "PERC" );  
       float  kPerc=0.0f;
       VData key;
       pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
       if (pHBVTable->m_type==DOT_FLOAT)
          key.ChangeType(TYPE_FLOAT);
  
      bool ok = pHBVTable->Lookup( key, col_p, kPerc );        // maximum soil moisture storage (mm)
      float aWD=float( pHRU->GetPool(3)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
      value = Percolation(aWD, kPerc);//filling the deepest reservoir
      }

   return value/1000.0f*pHRU->m_area;// m3/d
   }

float HBV::UplandFluxHandler( FlowContext *pFlowContext )
   {
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;
   
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_cfr = pHBVTable->GetFieldCol( "CFR" );   
   int m_col_fc = pHBVTable->GetFieldCol( "FC" );  
   int m_col_beta = pHBVTable->GetFieldCol( "BETA" );  
   int m_col_kperc = pHBVTable->GetFieldCol( "PERC" ); 
   int m_col_wp = pHBVTable->GetFieldCol( "WP" ); 

   int m_col_k0 = pHBVTable->GetFieldCol( "K0" );  
   int m_col_k1 = pHBVTable->GetFieldCol( "K1" );  
   int m_col_uzl = pHBVTable->GetFieldCol( "UZL" );  
   int m_col_k2 = pHBVTable->GetFieldCol( "K2" ); 

   float  FC, Beta,kPerc,WP,k0,k1,UZL,k2=0.0f;
   VData key;

   pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
   if (pHBVTable->m_type==DOT_FLOAT)
         key.ChangeType(TYPE_FLOAT);
  
   bool ok = pHBVTable->Lookup( key, m_col_fc, FC );        // maximum soil moisture storage (mm)
   ok = pHBVTable->Lookup( key, m_col_beta, Beta );    // parameter that determines the relative contribution to runoff from rain or snowmelt (-)
   ok = pHBVTable->Lookup( key, m_col_kperc, kPerc );  // Recession coefficient (day-1)
   ok = pHBVTable->Lookup( key, m_col_wp, WP );        // Recession coefficient (day-1)
   ok = pHBVTable->Lookup( key, m_col_k0, k0 );        // Recession coefficient (day-1)
   ok = pHBVTable->Lookup( key, m_col_k1, k1 );        // Recession coefficient (day-1)
   ok = pHBVTable->Lookup( key, m_col_uzl, UZL );      // threshold parameter (mm)
   ok = pHBVTable->Lookup( key, m_col_k2, k2 );        // Recession coefficient (day-1) 
 
   float value = 0; 
   
   float aWD = float( pHRU->GetPool(2)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
   float bWD = float( pHRU->GetPool(3)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
   float cWD = float( pHRU->GetPool(4)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm

   float gw = GroundWaterRecharge(pHRU->m_currentRainfall, aWD, FC, Beta);
   float p = Percolation(bWD, kPerc);//filling the deepest reservoir
   float q0 = Q0(bWD,k0, k1, UZL);
   float q2 = Q2(cWD, k2);

   switch( pHRUPool->m_pool )
      {
      //case 2:
     //    value = -gw;
         //pFlowContext->pHRU->m_pCatchment->m_contributionToReach+=((q0+q2)*pHRU->m_area/1000.0f);
      //   break;  
      case 3:
         value = -p-q0+gw;
         break;
      case 4:
         value = -q2+p;
         break;
      default:
         value=0.0f;
      } // end of switch
   return value*pHRU->m_area/1000.0f;
   }

float HBV::RunoffFluxHandler( FlowContext *pFlowContext )
   {
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   Catchment *pCatchment = pHRUPool->m_pHRU->m_pCatchment;
   HRU *pHRU = pHRUPool->m_pHRU;
   float value=0.0f;
   float k0,k1,UZL,k2,wp=0.0f;
   VData key;
  
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
   int m_col_k0 = pHBVTable->GetFieldCol( "K0" );  
   int m_col_k1 = pHBVTable->GetFieldCol( "K1" );  
   int m_col_uzl = pHBVTable->GetFieldCol( "UZL" );  
   int m_col_k2 = pHBVTable->GetFieldCol( "K2" ); 
   int m_col_wp = pHBVTable->GetFieldCol( "WP" );  
   if (pHBVTable->m_type==DOT_FLOAT)
         key.ChangeType(TYPE_FLOAT);


   bool ok = pHBVTable->Lookup( key, m_col_k0, k0 );        // Recession coefficient (day-1)
   ok = pHBVTable->Lookup( key, m_col_k1, k1 );        // Recession coefficient (day-1)
   ok = pHBVTable->Lookup( key, m_col_uzl, UZL );      // threshold parameter (mm)
   ok = pHBVTable->Lookup( key, m_col_wp, wp );      // permanent wilting point (mm)
   ok = pHBVTable->Lookup( key, m_col_k2, k2 );        // Recession coefficient (day-1)

   float d=0.0f;
   switch( pHRUPool->m_pool )
      {
      case 3:
         d=float( pHRU->GetPool(3)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm

         value = Q0(d,k0, k1, UZL);
         break;      
      case 4:
         d=float( pHRU->GetPool(4)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
         value = Q2(d, k2);
         break;

      default:
         value=0.0f;
      }
   pCatchment->m_contributionToReach += value/1000.0f*pHRU->m_area;
   return  -value/1000.0f*pHRU->m_area;
   }
float HBV::RuninFluxHandler( FlowContext *pFlowContext )
   {
   for ( INT_PTR i=0; i < pFlowContext->pReach->m_catchmentArray.GetSize(); i++ )
      {
      Catchment *pCatchment = pFlowContext->pReach->m_catchmentArray[ i ];

      float value=0.0f;
      float k0,k1,UZL,k2,wp=0.0f;
      VData key;

      float d=0.0f;
      for (int j=0;j<pCatchment->GetHRUCount();j++)
         {
         HRU *pHRU = pCatchment->GetHRU(j);

  
         ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
         pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
         int m_col_k0 = pHBVTable->GetFieldCol( "K0" );  
         int m_col_k1 = pHBVTable->GetFieldCol( "K1" );  
         int m_col_uzl = pHBVTable->GetFieldCol( "UZL" );  
         int m_col_k2 = pHBVTable->GetFieldCol( "K2" ); 
         int m_col_wp = pHBVTable->GetFieldCol( "WP" );  
         if (pHBVTable->m_type==DOT_FLOAT)
               key.ChangeType(TYPE_FLOAT);

         bool ok = pHBVTable->Lookup( key, m_col_k0, k0 );        // Recession coefficient (day-1)
         ok = pHBVTable->Lookup( key, m_col_k1, k1 );        // Recession coefficient (day-1)
         ok = pHBVTable->Lookup( key, m_col_uzl, UZL );      // threshold parameter (mm)
         ok = pHBVTable->Lookup( key, m_col_wp, wp );      // permanent wilting point (mm)
         ok = pHBVTable->Lookup( key, m_col_k2, k2 );        // Recession coefficient (day-1)

         d= float( pHRU->GetPool(3)->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
         pCatchment->m_contributionToReach += Q0(d,k0, k1, UZL)/1000.0f*pHRU->m_area;
         d=float( pHRU->GetPool(3)->m_volumeWater/pHRU->m_area*1000.0f);//convert from m to mm
         pCatchment->m_contributionToReach += Q2(d, k2)/1000.0f*pHRU->m_area;
         }
      }
   return  0.0f;
   }

float HBV::HBV_VerticalwSnow( FlowContext *pFlowContext )
   {
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_cfmax = pHBVTable->GetFieldCol( "CFMAX" );         // get the location of the parameter in the table    
   int m_col_tt = pHBVTable->GetFieldCol( "TT" );  
   int m_col_sfcf = pHBVTable->GetFieldCol( "SFCF" ); 
   int m_col_cwh = pHBVTable->GetFieldCol( "CWH" ); 
   int m_col_cfr = pHBVTable->GetFieldCol( "CFR" );  
   int m_col_lp = pHBVTable->GetFieldCol( "LP" );  
   int m_col_fc = pHBVTable->GetFieldCol( "FC" );  
   int m_col_beta = pHBVTable->GetFieldCol( "BETA" );  
   int m_col_kperc = pHBVTable->GetFieldCol( "PERC" ); 
   int m_col_wp = pHBVTable->GetFieldCol( "WP" ); 

   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = pFlowContext->pFlowModel->GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();
      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
   
         int hruLayerCount=pHRU->GetPoolCount();
         float CFMAX,TT,CFR,SFCF,CWH,FC, LP,Beta,kPerc,WP=0.0f;
         VData key;

         pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info
         if (pHBVTable->m_type==DOT_FLOAT)
              key.ChangeType(TYPE_FLOAT);
         bool ok = pHBVTable->Lookup( key, m_col_cfmax, CFMAX );   // degree-day factor (mm oC-1 day-1)
         ok = pHBVTable->Lookup( key, m_col_tt, TT );        // threshold temperature (oC)
         ok = pHBVTable->Lookup( key, m_col_cfr, CFR );      // refreezing coefficient
         ok = pHBVTable->Lookup( key, m_col_sfcf, SFCF );    // snowfall correction factor
         ok = pHBVTable->Lookup( key, m_col_cwh, CWH );      // snowpack retains water till this portion is exceeded
         ok = pHBVTable->Lookup( key, m_col_fc, FC );        // maximum soil moisture storage (mm)
         ok = pHBVTable->Lookup( key, m_col_lp, LP );        // soil moisture value above which ETact reaches ETpot (mm)
         ok = pHBVTable->Lookup( key, m_col_beta, Beta );    // parameter that determines the relative contribution to runoff from rain or snowmelt (-)
         ok = pHBVTable->Lookup( key, m_col_kperc, kPerc );  // Recession coefficient (day-1)
         ok = pHBVTable->Lookup( key, m_col_wp, WP );  // Recession coefficient (day-1)


         for ( int l=0; l < hruLayerCount; l++ )     //All but the bottom layer
            {
            HRUPool *pHRUPool = pHRU->GetPool( l );
            float value = 0; float throughfall=0.0f;
            float waterDepth=float( pHRUPool->m_volumeWater/pHRU->m_area*1000.0f );//convert from m to mm
            switch( pHRUPool->m_pool )
               {
            //   case 2:
             //     value = GroundWaterRecharge(precip, waterDepth, FC, Beta);
            //      break;  
               case 3:
                  value = Percolation(waterDepth, kPerc);//filling the deepest reservoir
                  break;

               default:
                  value=0.0f;
               } // end of switch
            pHRUPool->m_verticalDrainage = value*pHRU->m_area/1000.0f;     //m3/d
            //pHRUPool->m_wc = float( pHRUPool->m_volumeWater/pHRU->m_area );//m3/d
            pHRUPool->m_wc = (float)pHRUPool->m_volumeWater / pHRU->m_area *1000.0f;//mm water
           }
         }
      }
   return 0.0f;
   }

/*  This is a global method that accumulates runoff from HRUs into reaches.
    It assumes that the loss of that water from the HRUs is calculated using the flux handler
    RunoffFluxHandler.  These are redundant calculations...
*/
float HBV::HBV_HorizontalwSnow( FlowContext *pFlowContext )
   {
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable( "HBV" );   // store this pointer (and check to make sure it's not NULL)
   int m_col_k0 = pHBVTable->GetFieldCol( "K0" );  
   int m_col_k1 = pHBVTable->GetFieldCol( "K1" );  
   int m_col_uzl = pHBVTable->GetFieldCol( "UZL" );  
   int m_col_k2 = pHBVTable->GetFieldCol( "K2" ); 
   int m_col_wp = pHBVTable->GetFieldCol( "WP" );  
   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = pFlowContext->pFlowModel->GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();

      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         int hruLayerCount=pHRU->GetPoolCount();

         float k0,k1,UZL,k2,wp=0.0f;
         VData key;

         pFlowContext->pFlowModel->GetHRUData( pHRU, pHBVTable->m_iduCol, key, DAM_FIRST );  // get HRU info

         if (pHBVTable->m_type==DOT_FLOAT)
              key.ChangeType(TYPE_FLOAT);

         bool ok = pHBVTable->Lookup( key, m_col_k0, k0 );        // Recession coefficient (day-1)
         ok = pHBVTable->Lookup( key, m_col_k1, k1 );        // Recession coefficient (day-1)
         ok = pHBVTable->Lookup( key, m_col_uzl, UZL );      // threshold parameter (mm)
         ok = pHBVTable->Lookup( key, m_col_wp, wp );      // permanent wilting point (mm)
         ok = pHBVTable->Lookup( key, m_col_k2, k2 );        // Recession coefficient (day-1)


         for ( int l=0; l < hruLayerCount; l++ )     //All but the bottom layer
            { 
            HRUPool *pHRUPool = pHRU->GetPool( l );

            float value = 0;
            float d = float( pHRUPool->m_volumeWater/pHRU->m_area*1000.0f );//mm - 
            switch( pHRUPool->m_pool )
               {
               case 3:
                  value = Q0(d,k0, k1, UZL);
                  break;      
               case 4:
                  value = Q2(d, k2);
                  break;

               default:
                  value=0.0f;
                }      
             pCatchment->m_contributionToReach+=value*pHRU->m_area/1000.0f; // m3/d
             }
         }
      }   
   return 0.0f;
   }   

float HBV::N_Deposition_FluxHandler( FlowContext *pFlowContext )
   {
   float time = pFlowContext->time;
   time  = fmod( time, 365.0f );  // jpb - repeat yearly
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;
   float dep=2.0f;
   if (time > 50.0f && time < 55.0f)
      dep = 10.0f;
   return pHRU->m_currentPrecip*pHRU->m_area*dep;// m3/d*kg/m3=kg/d
   }


float HBV::N_ET_FluxHandler( FlowContext *pFlowContext )
   {
  float time = pFlowContext->time;
   time  = fmod( time, 365.0f );  // jpb - repeat yearly
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;
   float dep=2.0f;
   if (time > 50.0f && time < 55.0f)
      dep = 10.0f;
   return pHRU->m_currentET/1000.0f*pHRU->m_area*dep;
   }


float HBV::PrecipFluxHandlerNoSnow( FlowContext *pFlowContext  )
   {
   HRUPool *pHRUPool = pFlowContext->pHRUPool;
   HRU *pHRU = pHRUPool->m_pHRU;
   return pHRU->m_currentPrecip*pHRU->m_area;// m3/d
   }


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

BOOL HBV::StreamTempReg(FlowContext *pFlowContext )
   {
   if (pFlowContext->timing & GMT_INIT)
      return HBV::StreamTempInit( pFlowContext, NULL );

   if ( pFlowContext->timing & GMT_REACH)
   HBV::StreamTempRun( pFlowContext );

   return TRUE;
   }

BOOL HBV::StreamTempInit( FlowContext *pFlowContext, LPCTSTR initStr )
   {
   if ( m_colTMax < 0 )
      {
      MapLayer *pIDULayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;
      MapLayer *pStreamLayer = pIDULayer->GetMapPtr()->GetLayer( 1 );

      EnvExtension::CheckCol( pStreamLayer, m_colTMax, "TEMP_MAX", TYPE_FLOAT, CC_AUTOADD );
      }

   if ( m_tMaxArray.GetSize() == 0 )
      {
      int reachCount = (int) pFlowContext->pFlowModel->m_reachArray.GetSize();
      m_tMaxArray.SetSize( reachCount );

      for ( int i=0; i < reachCount; i++ )
         m_tMaxArray[ i ] = -273.0f;
      }

   return TRUE;
   }

BOOL HBV::StreamTempRun( FlowContext *pFlowContext )
   {
   // basic idea - this gets called at the end of each time step.
   // it iterates through the stream network, calculating stream temperatures
   // for the period july-sept.  For each day in this period, we calculate
   // the maximum stream temperature.  During the period, we track the highest
   // temperature experienced over that period.  That value gets written to the 
   // reach coverage at the end of the run
   MapLayer *pIDULayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;
   MapLayer *pStreamLayer = pIDULayer->GetMapPtr()->GetLayer( 1 );

   int reachCount = (int) pFlowContext->pFlowModel->m_reachArray.GetSize();
   ASSERT( reachCount == m_tMaxArray.GetSize() );

   int dayOfYear = pFlowContext->dayOfYear;
   
   // first day of the year?  then reset tMax array for reach values
   if ( dayOfYear == 0 )
      {
      for ( int i=0; i < reachCount; i++ )
         m_tMaxArray[ i ] = -273.0f;
      }

   for ( int i=0; i < reachCount; i++ )
      {
      Reach *pReach = pFlowContext->pFlowModel->m_reachArray[ i ];

      // calculate the stream temperature for this reach
      float flow = pReach->GetDischarge();    // m3/day
      flow /= CMD_PER_CFS;
      float tMaxAir = 0;
      bool ok = pFlowContext->pFlowModel->GetReachClimate( CDT_TMAX, pReach, dayOfYear, tMaxAir );

      float tMaxStream = float( 4.7985f + (-4.3901e-005 * flow) + (0.52919f * tMaxAir) );
     
      pReach->m_currentStreamTemp = tMaxStream;


       if ( tMaxStream > m_tMaxArray[ i ] )
          m_tMaxArray[ i ] = tMaxStream;
       }

   // last dayOfYear of the year?
   if ( dayOfYear == 364 )
      {
      pStreamLayer->m_readOnly = false;

      for ( int i=0; i < reachCount; i++ )
         {
         Reach *pReach = pFlowContext->pFlowModel->m_reachArray[ i ];
         
         int reachIndex = pReach->m_polyIndex;

         if ( reachIndex >= 0 )
            pStreamLayer->SetData( reachIndex, m_colTMax, m_tMaxArray[ i ] );
         }
      }

   pStreamLayer->m_readOnly = true;
      
   return TRUE;
   }

/*

BOOL HBV::CalcDailyUrbanWater(FlowContext *pFlowContext)
   {
   if (pFlowContext->timing & GMT_INIT)
      return HBV::CalcDailyUrbanWaterInit(pFlowContext);

   if (pFlowContext->timing & GMT_REACH)
      HBV::CalcDailyUrbanWaterRun(pFlowContext);
   return TRUE;
   }

BOOL HBV::CalcDailyUrbanWaterInit(FlowContext *pFlowContext)
   {
   MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

   pIDULayer->CheckCol( m_colH2OResidnt, "H2ORESIDNT", TYPE_FLOAT, CC_MUST_EXIST );
   pIDULayer->CheckCol( m_colH2OIndComm, "H2OINDCOMM", TYPE_FLOAT, CC_MUST_EXIST );
   pIDULayer->CheckCol( m_colDailyUrbanDemand, "URBDMAND_DY", TYPE_FLOAT, CC_AUTOADD );
   pIDULayer->CheckCol(m_colIDUArea, "AREA", TYPE_FLOAT, CC_MUST_EXIST);
   pIDULayer->CheckCol(m_colUGB, "UGB", TYPE_INT, CC_MUST_EXIST);   // urban water 
   
   bool readOnlyFlag = pIDULayer->m_readOnly;
   pIDULayer->m_readOnly = false;
   pIDULayer->SetColData( m_colDailyUrbanDemand, VData(0), true );
   pIDULayer->m_readOnly = readOnlyFlag;

   return TRUE;
   }

BOOL HBV::CalcDailyUrbanWaterRun(FlowContext *pFlowContext)
   {
   MapLayer *pIDULayer = (MapLayer*)pFlowContext->pEnvContext->pMapLayer;

   float ETtG = 1.f; // average summer ET (april-October) for urban lawn grass in current time step
   float ET0G = 1.f; // average summer ET (april-October) for urban lawn grass in time step 0
   int   doy = pFlowContext->dayOfYear;

   for (MapLayer::Iterator idu = pIDULayer->Begin(); idu != pIDULayer->End(); idu++)
      {
      float annualResidentialDemand = 0.f; // ccf/day/acre
      float annualIndCommDemand = 0.f;     // ccf/day/acre
      float iduArea = 0.f; // m2
      int UGBid = 0;

      pIDULayer->GetData(idu, m_colUGB, UGBid);

      if ( UGBid < 1 ) continue; // skip IDU id not UGB

      pIDULayer->GetData(idu, m_colH2OResidnt, annualResidentialDemand);
      pIDULayer->GetData(idu, m_colH2OIndComm, annualIndCommDemand);
      pIDULayer->GetData(idu, m_colIDUArea, iduArea);
      
      // ccf/day/acre
      float dailyResidentialWaterDemand = (float) ( 1 - 0.2 * ( ETtG / ET0G ) * ( cos( 2 * PI * doy / 365 ) ) ) * annualResidentialDemand;
      float dailyIndCommWaterDemand = annualIndCommDemand / 365;
      
      // ccf/day/acre
      float dailyUrbanWaterDemand = dailyResidentialWaterDemand + dailyIndCommWaterDemand;
      
      // 1 cf/day  = 3.2774128e-07 m3/sec
      // 1 ccf/day = 3.2774128e-05 m3/sec
      // 1 acre = 4046.86 m2
      // m3/sec = ccf/day/acre * 3.2774128e-05  / 4046.86 * iduArea
      dailyUrbanWaterDemand = dailyUrbanWaterDemand * 3.2774128e-05f / M2_PER_ACRE * iduArea;
      
      bool readOnlyFlag = pIDULayer->m_readOnly;
      pIDULayer->m_readOnly = false;
      pIDULayer->SetData( idu, m_colDailyUrbanDemand, dailyUrbanWaterDemand );
      pIDULayer->m_readOnly = readOnlyFlag;
      }

   return TRUE;
   }
*/

float HBV::Musle( FlowContext *pFlowContext )
   {
   int catchmentCount = pFlowContext->pFlowModel->GetCatchmentCount();
   int hruCount = pFlowContext->pFlowModel->GetHRUCount();  

  // #pragma omp parallel for firstprivate( pFlowContext )
   for ( int h=0; h < hruCount; h++ )
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      HRUPool *pSoilLayer = pHRU->GetPool( 0 );
     Reach * pReach = pSoilLayer->GetReach();
     if ( pReach )
        {
        float qPeak=pReach->GetDischarge();
        float qSurf=pSoilLayer->m_waterFluxArray[FL_STREAM_SINK]*-1.0f;
        float areaHa=pHRU->m_area/10000;
        qSurf/=areaHa;//m/ha
        qSurf*=1000.0f;
     
         // sed:   daily sediment yield (metric tons)
         // qSurf: surface runoff volume (mm/h20/ha)
         // qPeak: peak runoff rate (m3/s)
         // k:     the usle soil erodibility factory (0.013 metric ton me hr/(m3-metric ton cm))
         // c:     cover and management factor
         // p      usle support practice factor
         // LS:    topographic factor
         // cfrg:  coarse fragment factor

         double c=0.1;
         double p=0.1;
         double slope=0.3;
         double riserun=0.3;
         double m=0.6*(1-exp(-35.853*slope));
         double lHill =100.0; //slope length (m)

         double ls= pow((lHill/22.1),m)*(65.41*(sin(riserun)*sin(riserun))+4.56*sin(riserun)+0.065);
         double kay=0.1;
         double r=pow(11.8*qPeak*qSurf*pHRU->m_area,0.56);
         double soilLoss=r*kay*ls*c*p;
         pHRU->m_currentSediment= (float) soilLoss/pHRU->m_area*4047.0f; //tonnes/acre/d
         }
      }
   return -1.0f;
   }


float HBV::HBV_WithQuickflow(FlowContext *pFlowContext)
   {
   if (pFlowContext->timing & GMT_INIT) // Init()
      return HBV::InitHBV_WithQuickflow(pFlowContext, NULL);

   if ( ! ( pFlowContext->timing & GMT_CATCHMENT) )   // only process these two timings, ignore everyting else
      return 0;

   int hruCount = pFlowContext->pFlowModel->GetHRUCount();

   ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable("HBV");   // store this pointer (and check to make sure it's not NULL)
   /*ParamTable *pSoilPropertiesTable = pFlowContext->pFlowModel->GetTable("SoilData");*/
   
   //Andrew's additions to align with SSURGO wilting point in Flow-Solute instead of WP from HBV
   //VData soilType = 0;
   //pFlowContext->pFlowModel->GetHRUData(pHRU, pSoilPropertiesTable->m_iduCol, soilType, DAM_FIRST);  // get soil for this particular HRU

   int doy = pFlowContext->dayOfYear;    // zero based day of year
   int _month = 0; int _day; int _year;
   BOOL ok = ::GetCalDate(doy, &_year, &_month, &_day, TRUE);

   // iterate through hrus/hrulayers 
   #pragma omp parallel for firstprivate( pFlowContext )
   for (int h = 0; h < hruCount; h++)
      {
      HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
      int hruLayerCount = pHRU->GetPoolCount();
      //Initialize local variables
      float  Beta, kPerc, WP = 0.0f;
      float k0, k1, UZL, k2, kSoilDrainage, wp = 0.0f;
      float fc, lp = 0.0f;
      VData key;
      float precip = 0.0f;
      float airTemp = -999.f;
      float etRc = 0.0f;

      VData soilType = 0;

      //pFlowContext->pFlowModel->GetHRUData(pHRU, pSoilPropertiesTable->m_iduCol, soilType, DAM_FIRST);  // get soil for this particular HRU
      //if (pSoilPropertiesTable->m_type == DOT_FLOAT)
      //   soilType.ChangeType(TYPE_FLOAT);
      //float SSURGO_WP = 0.0f;
      //float thickness = 0.0f;
      //float SSURGO_FC = 0.0f;
      //bool ok = pSoilPropertiesTable->Lookup(soilType, m_col_SSURGO_WP, SSURGO_WP);    // get the wilting point
      //ok = pSoilPropertiesTable->Lookup(soilType, m_col_thickness, thickness);         // get the thickness
      //ok = pSoilPropertiesTable->Lookup(soilType, m_col_SSURGO_FC, SSURGO_FC);         // get the thickness
      //thickness = thickness * 10;                                                      //depth_ly(mm) = (hzdepb_r - hzdept_r)(cm)* 10(mm / cm)
      //SSURGO_WP = (SSURGO_WP / 100)*thickness;                                          //WP(mmH20)=wfifteenbar(%*100)*thickness(mm)
      //SSURGO_FC = (SSURGO_FC / 100)*thickness;                                          //WP(mmH20)=wfifteenbar(%*100)*thickness(mm)

      //Get Model Parameters
      pFlowContext->pFlowModel->GetHRUData(pHRU, pHBVTable->m_iduCol, key, DAM_FIRST);  // get HRU info
      if (pHBVTable->m_type == DOT_FLOAT)
         key.ChangeType(TYPE_FLOAT);
      ok = pHBVTable->Lookup(key, m_col_fc, fc);        // maximum soil moisture storage (mm)
      ok = pHBVTable->Lookup(key, m_col_lp, lp);        // soil moisture value above which ETact reaches ETpot (mm)
      ok = pHBVTable->Lookup(key, m_col_beta, Beta);   
      ok = pHBVTable->Lookup(key, m_col_kperc, kPerc);  // Percolation constant (mm)
      ok = pHBVTable->Lookup(key, m_col_wp, WP);        // No ET below this value of soil moisture (-)
      ok = pHBVTable->Lookup(key, m_col_k0, k0);        // Recession coefficient (day-1)
      ok = pHBVTable->Lookup(key, m_col_k1, k1);        // Recession coefficient (day-1)
      ok = pHBVTable->Lookup(key, m_col_uzl, UZL);      // threshold parameter (mm)
      ok = pHBVTable->Lookup(key, m_col_wp, wp);        // permanent wilting point (mm)
      ok = pHBVTable->Lookup(key, m_col_k2, k2);        // Recession coefficient (day-1)

      ok = pHBVTable->Lookup(key, m_col_ksoilDrainage, kSoilDrainage);        // Recession coefficient (day-1)
      if (kPerc + k1 > 1.0f)
         {
         kPerc = 0.4f;
         k1 = 0.4f; 
         }
      // Get the current Day/HRU climate data
      pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, pFlowContext->dayOfYear, precip);//mm

      pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, pFlowContext->dayOfYear, airTemp);//C

      // Get state variables from layers, these will be used to calculate some of the rates
      HRUPool *pHRUPool = pHRU->GetPool(0);
      ASSERT( pHRUPool->m_volumeWater >= 0 );

      float soilWater = float(pHRUPool->m_volumeWater / pHRU->m_area*1000.0f);//convert from m to mm
      float upperGroundWater = float(pHRUPool->m_volumeWater / pHRU->m_area*1000.0f);//convert from m to mm

      //Calculate rates
     // float gw = GroundWaterRechargeFraction(precip, soilWater, fc, Beta);
      float percolation = Percolation(upperGroundWater, kPerc);//filling the deepest reservoir

      float responseBoxRecharge = 0.0;
      if (soilWater>WP)
         responseBoxRecharge = soilWater*kSoilDrainage;//soil box drains vertically with a time constant of 0.25

      //Calculate the source/sink term for each HRUPool
      float q0 = 0.0f; float q2 = 0.0f;

      for (int l = 0; l < hruLayerCount; l++)
         {
         pHRUPool = pHRU->GetPool(l);
         ASSERT( pHRUPool->m_volumeWater >= 0 );
         float waterDepth = float(pHRUPool->m_volumeWater / pHRU->m_area*1000.0f);//mm
         ASSERT(waterDepth > 0.0f);
         float ss = 0.0f;
         q0 = 0.0f;

         pHRUPool->m_wc = (float)pHRUPool->m_volumeWater / pHRU->m_area *1000.0f;//mm water
         switch (pHRUPool->m_pool)
            {
            case 0:  //Soil
               pHRUPool->AddFluxFromGlobalHandler(precip*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d
               pHRUPool->AddFluxFromGlobalHandler(responseBoxRecharge*pHRU->m_area / 1000.0f, FL_BOTTOM_SINK);     //m3/d
               break;

            case 1://Upper Groundwater
               q0 = Q0(waterDepth, k0, k1, fc*UZL);
               if (waterDepth < WP )
                  {
                  q0 = 0.0f;
                  percolation = 0.0f;
                  }

               pHRUPool->AddFluxFromGlobalHandler(responseBoxRecharge*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);
               pHRUPool->AddFluxFromGlobalHandler(percolation*pHRU->m_area / 1000.0f, FL_BOTTOM_SINK);     //m3/d 
               pHRUPool->AddFluxFromGlobalHandler(q0*pHRU->m_area / 1000.0f, FL_STREAM_SINK);     //m3/d
               break;

            case 2://Lower Groundwater
               q2 = Q2(waterDepth, k2);
               pHRUPool->AddFluxFromGlobalHandler(percolation*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d 
               pHRUPool->AddFluxFromGlobalHandler(q2*pHRU->m_area / 1000.0f, FL_STREAM_SINK);     //m3/d
               break;

            default:
               ;
            } // end of switch

         pHRU->m_currentRunoff = q0 + q2;//mm_d
         Reach * pReach = pHRUPool->GetReach();
         if (pReach)
            pReach->AddFluxFromGlobalHandler(((q0 + q2) / 1000.0f*pHRU->m_area)); //m3/d
         }
      }

   Musle(pFlowContext);
   //  }
   return 0.0f;
   }

   /*
   float HBV::HBV_Agriculture(FlowContext *pFlowContext)
      {
      if (pFlowContext->timing & 1) // Init()
         return HBV::InitHBV_Global(pFlowContext, NULL);

      int hruCount = pFlowContext->pFlowModel->GetHRUCount();

      ParamTable *pHBVTable = pFlowContext->pFlowModel->GetTable("HBV");   // store this pointer (and check to make sure it's not NULL)

      int doy = pFlowContext->dayOfYear;    // zero based day of year
      int _month = 0; int _day; int _year;
      BOOL ok = ::GetCalDate(doy, &_year, &_month, &_day, TRUE);

      // iterate through hrus/hrulayers 
#pragma omp parallel for firstprivate( pFlowContext )
      for (int h = 0; h < hruCount; h++)
         {
         HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
         int hruLayerCount = pHRU->GetPoolCount();
         //Initialize local variables
         float  Beta, kPerc, WP = 0.0f;
         float k0, k1, UZL, k2, wp = 0.0f;
         float fc, lp = 0.0f;
         VData key;
         float precip = 0.0f;
         float airTemp = -999.f;
         float etRc = 0.0f;

         //Get Model Parameters
         pFlowContext->pFlowModel->GetHRUData(pHRU, pHBVTable->m_iduCol, key, DAM_FIRST);  // get HRU info
         if (pHBVTable->m_type == DOT_FLOAT)
            key.ChangeType(TYPE_FLOAT);
         bool ok = pHBVTable->Lookup(key, m_col_fc, fc);        // maximum soil moisture storage (mm)
         ok = pHBVTable->Lookup(key, m_col_lp, lp);        // soil moisture value above which ETact reaches ETpot (mm)
         ok = pHBVTable->Lookup(key, m_col_beta, Beta);    // parameter that determines the relative contribution to runoff from rain or snowmelt (-)
         ok = pHBVTable->Lookup(key, m_col_kperc, kPerc);  // Percolation constant (mm)
         ok = pHBVTable->Lookup(key, m_col_wp, WP);        // No ET below this value of soil moisture (-)
         ok = pHBVTable->Lookup(key, m_col_k0, k0);        // Recession coefficient (day-1)
         ok = pHBVTable->Lookup(key, m_col_k1, k1);        // Recession coefficient (day-1)
         ok = pHBVTable->Lookup(key, m_col_uzl, UZL);      // threshold parameter (mm)
         ok = pHBVTable->Lookup(key, m_col_wp, wp);        // permanent wilting point (mm)
         ok = pHBVTable->Lookup(key, m_col_k2, k2);        // Recession coefficient (day-1)

         // Get the current Day/HRU climate data
         pFlowContext->pFlowModel->GetHRUClimate(CDT_PRECIP, pHRU, pFlowContext->dayOfYear, precip);//mm
         pFlowContext->pFlowModel->GetHRUClimate(CDT_TMEAN, pHRU, pFlowContext->dayOfYear, airTemp);//C

         // Get state variables from layers, these will be used to calculate some of the rates
         float soilWater = float(pHRU->GetPool(0)->m_volumeWater / pHRU->m_area*1000.0f);//convert from m to mm
         float upperGroundWater = float(pHRU->GetPool(1)->m_volumeWater / pHRU->m_area*1000.0f);//convert from m to mm

         //Calculate rates
         float gw = GroundWaterRechargeFraction(precip, soilWater, fc, Beta);
         float percolation = Percolation(upperGroundWater, kPerc);//filling the deepest reservoir

         //Calculate the source/sink term for each HRUPool
         float q0 = 0.0f; float q2 = 0.0f;

         for (int l = 0; l < hruLayerCount; l++)
            {
            HRUPool *pHRUPool = pHRU->GetPool(l);
            float waterDepth = float(pHRUPool->m_volumeWater / pHRU->m_area*1000.0f);//mm
            float ss = 0.0f;
            q0 = 0.0f;

            pHRUPool->m_wc = (float)pHRUPool->m_volumeWater / pHRU->m_area *1000.0f;//mm water
            switch (pHRUPool->m_pool)
               {
               case 0:  //Soil
                  pHRUPool->AddFluxFromGlobalHandler(precip*(1 - gw)*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d
                  break;

               case 1://Upper Groundwater
                  q0 = Q0(waterDepth, k0, k1, UZL);
                  //ss = meltToSoil*(gw) - percolation - q0;
                  pHRUPool->AddFluxFromGlobalHandler(-precip*gw*pHRU->m_area / 1000.0f, FL_SINK);     //m3/d   TODO: Breaks with solute, FIX!!!
                  pHRUPool->AddFluxFromGlobalHandler(percolation*pHRU->m_area / 1000.0f, FL_BOTTOM_SINK);     //m3/d 
                  pHRUPool->AddFluxFromGlobalHandler(q0*pHRU->m_area / 1000.0f, FL_STREAM_SINK);     //m3/d
                  break;

               case 2://Lower Groundwater
                  q2 = Q2(waterDepth, k2);
                  //ss = percolation - q2; //filling the deepest reservoir
                  pHRUPool->AddFluxFromGlobalHandler(percolation*pHRU->m_area / 1000.0f, FL_TOP_SOURCE);     //m3/d 
                  pHRUPool->AddFluxFromGlobalHandler(q2*pHRU->m_area / 1000.0f, FL_STREAM_SINK);     //m3/d
                  break;

               default:

                  ;
               } // end of switch

            pHRU->m_currentRunoff = q0 + q2;//mm_d
            Reach * pReach = pHRUPool->GetReach();
            if (pReach)
               pReach->AddFluxFromGlobalHandler(((q0 + q2) / 1000.0f*pHRU->m_area)); //m3/d
            }
         }

      Musle(pFlowContext);
      //  }
      return 0.0f;
      }
      */