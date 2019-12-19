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
#include <MapLayer.h>
#include <FDataObj.h>

#include <Flow.h>
//#include <FlowInterface.h>
//#include <EnvInterface.h>
class FlowContext;
class Reach;
class Flow;

class Flow_Grid
{
public:
   Flow_Grid( void )  ;
   ~Flow_Grid( void ) ;

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
   int m_col_HOF_Threshold;
   int m_col_runoffCoefficient;
   int m_col_k;
   int m_col_ic;

   int m_col_kSatArg;
   int m_col_lamdaArg;
   int m_col_phiArg;
   int m_col_wpArg;

   int m_colRotationAge;
   int m_colLulc_a;
   int m_colHeight;
   int m_colHarvest;
   CString m_col_rotation;
   int m_harvestFilenameOffSet;

   float Grid_Fluxes(FlowContext *pFlowContext); 
   float SRS_5_Layer(FlowContext *pFlowContext);
   float SRS_6_Layer(FlowContext *pFlowContext);
   float HJA_6_Layer(FlowContext *pFlowContext);
   float UpdateTreeGrowth(FlowContext *pFlowContext);
   float UpdateSmallWatershedTreeGrowth(FlowContext *pFlowContext);
   float UpdateHBVTreeGrowth(FlowContext *pFlowContext);
   float AdjustForSaturation(FlowContext *pFlowContext);
   float AdjustHJAForSaturation(FlowContext *pFlowContext);
   float AdjustForSaturation_Rates(FlowContext *pFlowContext);
   float Solute_Fluxes( FlowContext *pFlowContext );
   float CalcBrooksCoreyKTheta(float wc, float wp, float phi, float lamda, float kSat);
   float CalcSatWaterFluxes(HRU *pHRU, HRUPool *pHRUPool, float phi, float ple, float kSat, float &runoff, float minElev);
   float CalcSatWaterFluxes(HRU *pHRU, HRUPool *pHRUPool, float phi, float ple, float kSat, float &q1, float minElev, float totalDepth, float wc, ParamTable *pTable, FlowContext *pFlowContext);
   float PrecipFluxHandler( FlowContext *pFlowContext );
   float Tracer_Application( FlowContext *pFlowContext );

   float Init_Grid_Fluxes( FlowContext *pFlowContext );
   float InitRun_Grid_Fluxes(FlowContext *pFlowContext);

   float Init_FIRE_Grid_Fluxes(FlowContext *pFlowContext);

   float Init_Year(FlowContext *pFlowContext);
   float Init_Small_Watershed_Year(FlowContext *pFlowContext);
   bool LoadXml(LPCTSTR filename);
   CString m_laiFilename;
   CString m_rotationAgeFilename;
   CString m_ageHeightFilename;
   FDataObj *m_pLAI_Table;
   FDataObj *m_pRotationAge_Table;
   FDataObj *m_pAgeHeight_Table;
};