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
#include <FDataObj.h>
#include <Flow.h>

class FlowContext;
class Reach;
class Flow;

class Flow_Solute
   {
   public:
      Flow_Solute(void);
      ~Flow_Solute(void);

      float *m_uplandWaterSourceSinkArray;
      float *m_soilDrainage;
      float *m_instreamWaterSourceSinkArray;
      int m_col_wp;
      int m_col_phi;
      int m_col_lamda;
      int m_col_kSat;
      int m_col_ple;
      int m_col_lp;
      int m_col_fc;
      int m_col_kSatGw;
      int m_col_pleGw;
      int m_col_phiGw;
      int m_col_lamdaGW;
      int m_col_wpGW;
      int m_col_fcPct;
      int m_col_k;
      int m_col_ic;
	   int m_col_AppRatePartialIPM;
	   int m_col_AppRateFullIPM;
      int m_col_AppRateConventional;
	   int m_col_AppRateZero;
	   int m_col_AppRateRecommended;
	   int m_col_AppRateMinimum;
      int m_col_AppStart;
      int m_col_AppFinish;


	   int m_col_percent_clay;
	   int m_col_thickness;
	   int m_col_AWC;
	   int m_col_OM;
	   int m_col_SSURGO_WP;
      int m_col_SSURGO_FC;
	   int m_col_albedo;
	   int m_col_SAT;
	   int m_col_BD;	  
      int m_col_om;
      int m_col_koc;
      int m_col_ks;
      int m_col_bd;

	   int m_col_PHU;
	   int m_colTbase;
	   int m_col_fr_n1;
	   int m_col_fr_n2;
	   int m_col_fr_n3;
	   int m_colRUE;
	   int m_colLAI;
	  
	   int m_colN_app;
      int m_colIPM;
      int m_colArea;
      int m_colDistStr;
      int m_colFracCGDD;
      int m_colStandAge;
	   int m_biomass;
	   int m_colLULC_B;
	   int m_colLULC_A;
      int m_colField;

      CArray< float, float > m_hruBiomassNArray;

      FDataObj *m_pRateData;

      FDataObj *m_pTimeSinceTracerData;

      FDataObj *m_pTracerData1;
      FDataObj *m_pTracerData2;
      FDataObj *m_pTracerData3;
      FDataObj *m_pTracerData4;
      FDataObj *m_pTracerData5;
      FDataObj *m_pTracerData6;
	  FDataObj *m_pDataDeposition;

      float Solute_Fluxes(FlowContext *pFlowContext);
      float HBV_N_Fluxes(FlowContext *pFlowContext);
      float Init_Solute_Fluxes(FlowContext *pFlowContext);
      float EndRun_Solute_Fluxes(FlowContext *pFlowContext);
      float Start_Year_Solute_Fluxes(FlowContext *pFlowContext);
      float Solute_Fluxes_SRS(FlowContext *pFlowContext);
	  float Solute_Fluxes_HJA(FlowContext *pFlowContext);
      float Solute_Fluxes_SRS_Old(FlowContext *pFlowContext);
      float CalcSatTracerFluxes(HRU *pHRU, HRUPool *pHRULayer, int k);
      float Init_Run_Solute_Fluxes(FlowContext *pFlowContext);
      float ApplicationRate(float time, HRUPool *pHRULayer, FlowContext *pFlowContext, int k);
	   float NitrogenApplicationRate(float time, HRUPool *pHRULayer, FlowContext *pFlowContext, int k);
      float Solute_Fluxes_SRS_Managed(FlowContext *pFlowContext);

   };