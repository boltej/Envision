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
// Flow.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "flow.h"
#include "FlowDlg.h"
#include <map.h>
#include <maplayer.h>
#include <math.h>
#include <fdataobj.h>
#include <idataobj.H>
#include <tinyxml.h>
#include <EnvModel.h>
#include <UNITCONV.H>
#include <DATE.HPP>
#include <VideoRecorder.h>
#include <PathManager.h>
#include <GDALWrapper.h>
#include <GeoSpatialDataObj.h>
#include "AlgLib\ap.h"

//#include <EnvEngine\DeltaArray.h>

#include <EnvisionAPI.h>
#include <EnvExtension.h>
#include <EnvConstants.h>

//#include <EnvView.h>
#include <omp.h>


// Required columns
// Catchment Columns
//   m_colCatchmentArea,        AREA,          TYPE_FLOAT,
//   m_colCatchmentJoin,        m_catchmentJoinCol, TYPE_INT,

// Stream Columns
//   m_colStreamFrom,         _T("FNODE_"),   TYPE_INT,   CC_MUST_EXIST );  // required for building topology
//   m_colStreamTo,           _T("TNODE_"),   TYPE_INT,   CC_MUST_EXIST );
//   m_colStreamJoin,           m_streamJoinCol,  TYPE_INT,   CC_MUST_EXIST );


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define _EXPORT __declspec( dllexport )

extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new FlowModel; }


MTDOUBLE HRU::m_mvDepthMelt = 0;  // volume of water in snow
MTDOUBLE HRU::m_mvDepthSWE = 0;   // volume of ice in snow
MTDOUBLE HRU::m_mvCurrentPrecip = 0;
MTDOUBLE HRU::m_mvCurrentRainfall = 0;
MTDOUBLE HRU::m_mvCurrentSnowFall = 0;
MTDOUBLE HRU::m_mvCurrentAirTemp = 0;
MTDOUBLE HRU::m_mvCurrentMinTemp = 0;
MTDOUBLE HRU::m_mvCurrentMaxTemp = 0;
MTDOUBLE HRU::m_mvCurrentRadiation = 0;
MTDOUBLE HRU::m_mvCurrentET = 0;
MTDOUBLE HRU::m_mvCurrentMaxET = 0;
MTDOUBLE HRU::m_mvCurrentCGDD = 0;
MTDOUBLE HRU::m_mvCurrentSediment = 0;

MTDOUBLE HRU::m_mvCurrentSWC = 0;
MTDOUBLE HRU::m_mvCurrentIrr = 0;

MTDOUBLE HRU::m_mvCumET = 0;
MTDOUBLE HRU::m_mvCumRunoff = 0;
MTDOUBLE HRU::m_mvCumMaxET = 0;
MTDOUBLE HRU::m_mvCumP = 0;
MTDOUBLE HRU::m_mvElws = 0;
MTDOUBLE HRU::m_mvDepthToWT = 0;
MTDOUBLE HRU::m_mvCumRecharge = 0;
MTDOUBLE HRU::m_mvCumGwFlowOut = 0;

MTDOUBLE HRU::m_mvCurrentRecharge = 0;
MTDOUBLE HRU::m_mvCurrentGwFlowOut = 0;
///// PtrArray< PoolInfo > HRU::m_poolInfoArray;     // just templates, actual pools are in m_poolArray
///// 
///// 
MTDOUBLE Reach::m_mvCurrentStreamFlow = 0;
MTDOUBLE Reach::m_mvCurrentStreamTemp = 0;
MTDOUBLE Reach::m_mvCurrentTracer = 0;

MTDOUBLE HRUPool::m_mvWDepth = 0;
MTDOUBLE HRUPool::m_mvWC = 0;
MTDOUBLE HRUPool::m_mvESV = 0;


const int JUN_1 = 151;    // zero-based
const int AUG_31 = 242;    // zero-based


int ParamTable::CreateMap(void)
   {
   if (m_pTable == NULL)
      return -1;

   VData data;
   int rows = (int)m_pTable->GetRowCount();

   for (int i = 0; i < m_pTable->GetRowCount(); i++)
      {
      m_pTable->Get(0, i, data);
      m_lookupMap.SetAt(data, i);
      }

   return rows;
   }


bool ParamTable::Lookup(VData key, int col, int &value)
   {
   if (m_pTable == NULL)
      return false;

   int row = -1;

   if (m_lookupMap.Lookup(key, row))
      return m_pTable->Get(col, row, value);

   return false;
   }


bool ParamTable::Lookup(VData key, int col, float &value)
   {
   if (m_pTable == NULL)
      return false;

   int row = -1;

   if (m_lookupMap.Lookup(key, row))
      return m_pTable->Get(col, row, value);

   return false;
   }


bool ModelOutput::Init(EnvContext *pEnvContext )
   {
   MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   FlowModel *pFlowModel = (FlowModel*)pEnvContext->pEnvExtension;

   if (m_queryStr.GetLength() > 0)
      {
      ASSERT(pFlowModel != NULL);
      if (pFlowModel->m_pModelOutputQE == NULL)
         {
         pFlowModel->m_pModelOutputQE = new QueryEngine(pIDULayer);
         }

      if (pFlowModel->m_pMapExprEngine == NULL)
         {
         pFlowModel->m_pMapExprEngine = new MapExprEngine(pIDULayer, pEnvContext->pQueryEngine); //pEnvContext->pExprEngine;
         ASSERT(pFlowModel->m_pMapExprEngine != NULL);

         // add model vars for HRU stuff - these are legal names in ModelOutput expressions
         //                     description          variable name    internal variable address, these are statics
         pFlowModel->AddModelVar(_T("Melt Depth (mm)"), _T("hruVolMelt"), &HRU::m_mvDepthMelt);  //
         pFlowModel->AddModelVar(_T("SWE Depth (mm)"), _T("hruVolSwe"), &HRU::m_mvDepthSWE);   //
         pFlowModel->AddModelVar(_T("Total Precip (mm)"), _T("hruPrecip"), &HRU::m_mvCurrentPrecip);
         pFlowModel->AddModelVar(_T("Total Rainfall (mm)"), _T("hruRainfall"), &HRU::m_mvCurrentRainfall);
         pFlowModel->AddModelVar(_T("Total Snowfall (mm)"), _T("hruSnowfall"), &HRU::m_mvCurrentSnowFall);
         pFlowModel->AddModelVar(_T("Air Temp (C)"), _T("hruAirTemp"), &HRU::m_mvCurrentAirTemp);
         pFlowModel->AddModelVar(_T("Min Temp (C)"), _T("hruMinTemp"), &HRU::m_mvCurrentMinTemp);
         pFlowModel->AddModelVar(_T("Max Temp (C)"), _T("hruMaxTemp"), &HRU::m_mvCurrentMaxTemp);
         pFlowModel->AddModelVar(_T("Radiation (w/m2)"), _T("hruRadiation"), &HRU::m_mvCurrentRadiation);
         pFlowModel->AddModelVar(_T("ET (mm)"), _T("hruET"), &HRU::m_mvCurrentET);

         pFlowModel->AddModelVar(_T("WC (mm/mm)"), _T("hruWC"), &HRU::m_mvCurrentSWC);
         pFlowModel->AddModelVar(_T("Irrigation (mm/d)"), _T("hruIrr"), &HRU::m_mvCurrentIrr);

         pFlowModel->AddModelVar(_T("Maximum ET (mm)"), _T("hruMaxET"), &HRU::m_mvCurrentMaxET);
         pFlowModel->AddModelVar(_T("CGDD0 (heat units)"), _T("hruCGDD"), &HRU::m_mvCurrentCGDD);
         pFlowModel->AddModelVar(_T("Sediment"), _T("hruSedimentOut"), &HRU::m_mvCurrentSediment);
         pFlowModel->AddModelVar(_T("Streamflow (mm)"), _T("reachOutflow"), &Reach::m_mvCurrentStreamFlow);
         pFlowModel->AddModelVar(_T("Stream Temp (C)"), _T("reachTemp"), &Reach::m_mvCurrentStreamTemp);

         pFlowModel->AddModelVar(_T("Recharge (mm)"), _T("hruGWRecharge"), &HRU::m_mvCurrentRecharge);
         pFlowModel->AddModelVar(_T("GW Loss (mm)"), _T("hruGWOut"), &HRU::m_mvCurrentGwFlowOut);


         pFlowModel->AddModelVar(_T("Cumu. Rainfall (mm)"), _T("hruCumP"), &HRU::m_mvCumP);
         pFlowModel->AddModelVar(_T("Cumu. Runoff (mm)"), _T("hruCumRunoff"), &HRU::m_mvCumRunoff);
         pFlowModel->AddModelVar(_T("Cumu. ET (mm)"), _T("hruCumET"), &HRU::m_mvCumET);
         pFlowModel->AddModelVar(_T("Cumu. Max ET (mm)"), _T("hruCumMaxET"), &HRU::m_mvCumMaxET);
         pFlowModel->AddModelVar(_T("Cumu. GW Out (mm)"), _T("hruCumGWOut"), &HRU::m_mvCumGwFlowOut);
         pFlowModel->AddModelVar(_T("Cumu. GW Recharge (mm)"), _T("hruCumGWRecharge"), &HRU::m_mvCumRecharge);

         pFlowModel->AddModelVar(_T("WC"), _T("hruLayerWC"), &HRUPool::m_mvWC);
         pFlowModel->AddModelVar(_T("WC"), _T("hruLayerWDepth"), &HRUPool::m_mvWDepth);
         pFlowModel->AddModelVar(_T("Elws"), _T("hruElws"), &HRU::m_mvElws);
         pFlowModel->AddModelVar(_T("Elws"), _T("hruDepthToWT"), &HRU::m_mvDepthToWT);
    
         pFlowModel->AddModelVar(_T("Extra State Variable Conc"), _T("esv_hruLayer"), &HRUPool::m_mvESV);

         pFlowModel->AddModelVar(_T("Streamflow"), _T("esv_reachOutflow"), &Reach::m_mvCurrentTracer);
         }

      m_pQuery = pFlowModel->m_pModelOutputQE->ParseQuery(m_queryStr, 0, _T("Model Output Query"));

      if (m_pQuery == NULL)
         {
         CString msg(_T("Flow: Error parsing query expression '"));
         msg += m_queryStr;
         msg += _T("' - the output will be ignored...");
         Report::ErrorMsg(msg);
         m_inUse = false;
         }

      m_pMapExpr = pFlowModel->m_pMapExprEngine->AddExpr(m_name, m_exprStr, m_queryStr);
      ASSERT(m_pMapExpr != NULL);

      bool ok = pFlowModel->m_pMapExprEngine->Compile(m_pMapExpr);

      if (!ok)
         {
         CString msg(_T("Flow: Unable to compile map expression "));
         msg += m_exprStr;
         msg += _T(" for <output> '");
         msg += m_name;
         msg += _T("'.  The expression will be ignored");
         Report::ErrorMsg(msg);
         m_inUse = false;
         }
      }

   // expression

   return true;
   }


bool ModelOutput::InitStream(FlowModel *pFlowModel, MapLayer *pStreamLayer)
   {
   if (m_queryStr.GetLength() > 0)
      {
      if (pFlowModel->m_pModelOutputStreamQE == NULL)
         {
         pFlowModel->m_pModelOutputStreamQE = new QueryEngine(pStreamLayer);
         ASSERT(pFlowModel->m_pModelOutputStreamQE != NULL);
         }

      m_pQuery = pFlowModel->m_pModelOutputStreamQE->ParseQuery(m_queryStr, 0, _T("Model Output Query"));

      if (m_pQuery == NULL)
         {
         CString msg(_T("Flow: Error parsing query expression '"));
         msg += m_queryStr;
         msg += _T("' - the output will be ignored...");
         Report::ErrorMsg(msg);
         m_inUse = false;
         }

      m_pMapExpr = pFlowModel->m_pMapExprEngine->AddExpr(m_name, m_exprStr, m_queryStr);
      ASSERT(m_pMapExpr != NULL);

      bool ok = pFlowModel->m_pMapExprEngine->Compile(m_pMapExpr);

      if (!ok)
         {
         CString msg(_T("Flow: Unable to compile map expression "));
         msg += m_exprStr;
         msg += _T(" for <output> '");
         msg += m_name;
         msg += _T("'.  The expression will be ignored");
         Report::ErrorMsg(msg);
         m_inUse = false;
         }
      }

   // expression

   return true;
   }


ModelOutputGroup &ModelOutputGroup::operator = (ModelOutputGroup &mog)
   {
   m_name = mog.m_name;
   m_pDataObj = NULL;
   m_moCountIdu = mog.m_moCountIdu;
   m_moCountHru = mog.m_moCountHru;
   m_moCountHruLayer = mog.m_moCountHruLayer;
   m_moCountReach = mog.m_moCountReach;

   // copy each model output
   for (int i = 0; i < (int)mog.GetSize(); i++)
      {
      ModelOutput *pOutput = new ModelOutput(*(mog.GetAt(i)));
      ASSERT(pOutput != NULL);
      this->Add(pOutput);
      }

   return *this;
   }



///////////////////////////////////////////////////////////////////////////////
// H R U P O O L
///////////////////////////////////////////////////////////////////////////////

HRUPool::HRUPool(HRU *pHRU)
   : FluxContainer()
   , StateVarContainer()
   , m_volumeWater(0.0f)
   , m_contributionToReach(0.0f)
   , m_verticalDrainage(0.0f)
   , m_horizontalExchange(0.0f)
   , m_wc(0.0f)
   , m_volumeLateralInflow(0.0f)
   , m_volumeLateralOutflow(0.0f)
   , m_volumeVerticalInflow(0.0f)
   , m_volumeVerticalOuflow(0.0f)
   , m_pHRU(pHRU)
   , m_pool(-1)
   , m_type(PT_SOIL)
   , m_depth(1.0f)
   {
   ASSERT(m_pHRU != NULL);
   }



HRUPool &HRUPool::operator = (HRUPool &l)
   {
   m_volumeWater = l.m_volumeWater;
   m_contributionToReach = l.m_contributionToReach;
   m_verticalDrainage = l.m_verticalDrainage;
   m_horizontalExchange = l.m_verticalDrainage;
   m_wc = l.m_wc;
   m_volumeLateralInflow = l.m_volumeLateralInflow;        // m3/day
   m_volumeLateralOutflow = l.m_volumeLateralOutflow;
   m_volumeVerticalInflow = l.m_volumeVerticalInflow;
   m_volumeVerticalOuflow = l.m_volumeVerticalOuflow;
   m_depth = l.m_depth;

   // general info
   m_pHRU = l.m_pHRU;
   m_pool = l.m_pool;
   m_type = l.m_type;

   return *this;
   }


Reach *HRUPool::GetReach(void)
   {
   return m_pHRU->m_pCatchment->m_pReach;
   }


void HRUPool::AddFluxFromGlobalHandler(float value, WFAINDEX wfaIndex)
   {
   switch (wfaIndex)
      {
      case FL_TOP_SINK:
      case FL_BOTTOM_SINK:
      case FL_STREAM_SINK:
      case FL_SINK:  //negative values are gains
         value = -value;
         break;
      default:
         break;
      }

   m_waterFluxArray[wfaIndex] += value;
   m_globalHandlerFluxValue += value;
   }


///////////////////////////////////////////////////////////////////////////////
// H R U 
///////////////////////////////////////////////////////////////////////////////

HRU::HRU(void)
   : m_id(-1)
   , m_area(0.0f)
   , m_precip_yr(0.0f)
   , m_precip_wateryr(0.0f)
   , m_precip_10yr(20)
   , m_precipCumApr1(0.0f)
   , m_temp_yr(0.0f)
   , m_rainfall_yr(0.0f)
   , m_snowfall_yr(0.0f)
   , m_gwRecharge_yr(0.0f)       // mm
   , m_gwFlowOut_yr(0.0f)
   , m_areaIrrigated(0.0f)
   , m_temp_10yr(20)
   , m_depthMelt(0.0f)
   , m_depthSWE(0.0f)
   , m_depthApr1SWE_yr(0.0f)
   , m_apr1SWE_10yr(20)
   , m_aridityIndex(20)
   , m_centroid(0.0f, 0.0f)
   , m_elevation(50.0f)
   , m_currentPrecip(0.0f)
   , m_currentRainfall(0.0f)
   , m_currentSnowFall(0.0f)
   , m_currentAirTemp(0.0f)
   , m_currentMinTemp(0.0f)
   , m_currentMaxTemp(0.0f)
   , m_currentRadiation(0.0f)
   , m_currentET(0.0f)
   , m_maxET_yr(0.0f)
   , m_et_yr(0.0f)
   , m_currentRunoff(0.0f)
   , m_currentGWRecharge(0.0f)
   , m_currentGWFlowOut(0.0f)
   , m_runoff_yr(0.0f)
   , m_initStorage(0.0f)
   , m_endingStorage(0.0f)
   , m_storage_yr(0.0f)
   , m_percentIrrigated(0.0f)
   , m_meanLAI(0.0f)
   , m_meanAge(0.0f)
   , m_currentMaxET(0.0f)
   , m_currentCGDD(0.0f)
   , m_currentSediment(0.0f)
   , m_climateIndex(-1)
   , m_climateRow(-1)
   , m_climateCol(-1)
   , m_demRow(-1)
   , m_demCol(-1)
   , m_lowestElevationHRU(false)
   , m_elws(0.0f)
   , m_soilTemp(10.0f)
   , m_biomass(0.0f)
   , m_swc(0.0f)
   , m_irr(0.0f)
   , m_pCatchment(NULL)
   { }

HRU::~HRU(void)
   {
   //if (m_waterFluxArray != NULL) 
   //   delete[] m_waterFluxArray; 

   //if (m_pCatchment) 
   //   delete m_pCatchment; 
   }


int HRU::AddPools(  FlowModel *pFlowModel, /*int poolCount, float initWaterContent, float initTemperature, */ bool grid)
   {
   HRUPool hruLayer(this);

   int layer = 0;

   for (int i = 0; i < pFlowModel->m_poolInfoArray.GetSize(); i++)
      {
      PoolInfo *pInfo = pFlowModel->m_poolInfoArray[ i ];

      HRUPool *pHRUPool = new HRUPool(hruLayer);
      ASSERT(pHRUPool != NULL);

      m_poolArray.Add(pHRUPool);
      pHRUPool->m_pool = layer;

      // clone from the template POOL_INFO
      pHRUPool->m_type = pInfo->m_type;
      pHRUPool->m_depth = pInfo->m_depth;

      //if (m_pFlowModel->m_hruLayerDepths.GetCount() > 0)
      //   m_poolArray[layer]->m_depth = (float)atof(m_pFlowModel->m_hruLayerDepths[i]);
      //
      //if (m_pFlowModel->m_initWaterContent.GetCount() == 1)
      //   m_poolArray[layer]->m_volumeWater = atof(m_pFlowModel->m_initWaterContent[0]);
      //else if (m_pFlowModel->m_initWaterContent.GetCount() > 1)
      //   {
      //   ASSERT(m_pFlowModel->m_hruLayerDepths.GetCount() > 0);//if this asserts, it probably means you did not include the same number of initial watercontents as you did layer depths.  Check your xml file (catchments tag)
      //   m_poolArray[layer]->m_volumeWater = atof(m_pFlowModel->m_initWaterContent[i])*pHRUPool->m_depth*0.4f*m_area;
      //   float test = (float)atof(m_pFlowModel->m_initWaterContent[i]);
      //   int stop = 1;
      //   }

      layer++;
      }

   for (int i = 0; i < m_poolArray.GetSize(); i++)
      {
      HRUPool *pLayer = m_poolArray[i];
      if ( !grid ) // if it is a grid, the water flux array is allocated by BuildCatchmentFromGrids.  
         {
         pLayer->m_waterFluxArray.SetSize(7);
         for (int j = 0; j < 7; j++)
            pLayer->m_waterFluxArray[j] = 0.0f;
         }
      }

   return (int)m_poolArray.GetSize();
   }



///////////////////////////////////////////////////////////////////////////////
// C A T C H M E N T 
///////////////////////////////////////////////////////////////////////////////

Catchment::Catchment(void)
   : m_id(-1)
   , m_area(0)
   , m_currentAirTemp(0)
   , m_currentPrecip(0)
   , m_currentThroughFall(0)
   , m_currentSnow(0)
   , m_currentET(0)
   , m_currentGroundLoss(0)
   , m_meltRate(0)
   , m_contributionToReach(0.0f)    // m3/day
   , m_climateIndex(1)
   , m_pDownslopeCatchment(NULL)
   , m_pReach(NULL)
   , m_pGW(NULL)
   { }


///////////////////////////////////////////////////////////////////////////////
// R E A C H 
///////////////////////////////////////////////////////////////////////////////

Reach::Reach()
   : ReachNode()
   , m_width(5.0f)
   , m_depth(0.5f)
   , m_wdRatio(10.0f)
   , m_cumUpstreamArea(0)
   , m_cumUpstreamLength(0)
   , m_climateIndex(-1)
   , m_alpha(1.0f)
   , m_beta(3 / 5.0f)
   , m_n(0.014f)
   , m_meanYearlyDischarge(0.0f)
   , m_maxYearlyDischarge(0.0f)
   , m_sumYearlyDischarge(0.0f)
   , m_isMeasured(false)
   , m_pReservoir(NULL)
   , m_pStageDischargeTable(NULL)
   , m_instreamWaterRightUse(0.0f)
   , m_currentStreamTemp(0.0f)
   { }


Reach::~Reach(void)
   {
   if (m_pStageDischargeTable != NULL)
      delete m_pStageDischargeTable;
   }


void Reach::SetGeometry(float wdRatio)
   {
   //ASSERT( pReach->m_qArray != NULL );
   ReachSubnode *pNode = (ReachSubnode*)m_subnodeArray[0];
   ASSERT(pNode != NULL);

   m_depth = GetDepthFromQ(pNode->m_discharge, wdRatio);
   m_width = wdRatio * m_depth;
   m_wdRatio = wdRatio;
   }


//???? CHECK/DOCUMENT UNITS!!!!
float Reach::GetDepthFromQ(float Q, float wdRatio)  // ASSUMES A SPECIFIC CHANNEL GEOMETRY
   {
   // Q=m3/sec
   // from kellie:  d = ( 5/9 Q n s^2 ) ^ 3/8   -- assumes width = 3*depth, rectangular channel
   float wdterm = (float)pow((wdRatio / (2 + wdRatio)), 2.0f / 3.0f)*wdRatio;
   float depth = (float)pow(((Q*m_n) / ((float)sqrt(m_slope)*wdterm)), 3.0f / 8.0f);
   return (float)depth;
   }


float Reach::GetDischarge(int subnode /*=-1*/)
   {
   if (subnode < 0)
      subnode = this->GetSubnodeCount() - 1;

   ReachSubnode *pNode = (ReachSubnode*) this->m_subnodeArray[subnode];
   ASSERT(pNode != NULL);

   float q = pNode->m_discharge;

   if (q < 0.0f)
      q = 0.0f;

   return q;
   }


float Reach::GetUpstreamInflow()
   {
   float Q = 0.0f;
   ReachSubnode *pNodeLeft = NULL;
   if (this->m_pLeft && this->m_pLeft->m_polyIndex >= 0)
      {
      int lastSubnode = this->m_pLeft->GetSubnodeCount() - 1;
      pNodeLeft = (ReachSubnode*) this->m_pLeft->m_subnodeArray[lastSubnode];
      }

   ReachSubnode *pNodeRight = NULL;
   if (this->m_pRight && this->m_pRight->m_polyIndex >= 0)
      {
      int lastSubnode = this->m_pRight->GetSubnodeCount() - 1;
      pNodeRight = (ReachSubnode*) this->m_pRight->m_subnodeArray[lastSubnode];
      }

   if (pNodeLeft != NULL)
      Q = pNodeLeft->m_discharge;
   if (pNodeRight != NULL)
      Q += pNodeRight->m_discharge;

   return Q;
   }

bool Reach::GetUpstreamInflow(float &QLeft, float &QRight)
   {
   ReachSubnode *pNodeLeft = NULL;
   if (this->m_pLeft && this->m_pLeft->m_polyIndex >= 0)
      {
      int lastSubnode = this->m_pLeft->GetSubnodeCount() - 1;
      pNodeLeft = (ReachSubnode*) this->m_pLeft->m_subnodeArray[lastSubnode];
      }

   ReachSubnode *pNodeRight = NULL;
   if (this->m_pRight && this->m_pRight->m_polyIndex >= 0)
      {
      int lastSubnode = this->m_pRight->GetSubnodeCount() - 1;
      pNodeRight = (ReachSubnode*) this->m_pRight->m_subnodeArray[lastSubnode];
      }

   if (pNodeLeft != NULL)
      QLeft = pNodeLeft->m_discharge;
   if (pNodeRight != NULL)
      QRight = pNodeRight->m_discharge;

   return true;
   }

bool Reach::GetUpstreamTemperature(float &tempLeft, float &tempRight)
   {
   ReachSubnode *pNodeLeft = NULL;
   if (this->m_pLeft && this->m_pLeft->m_polyIndex >= 0)
      {
      int lastSubnode = this->m_pLeft->GetSubnodeCount() - 1;
      pNodeLeft = (ReachSubnode*) this->m_pLeft->m_subnodeArray[lastSubnode];
      }

   ReachSubnode *pNodeRight = NULL;
   if (this->m_pRight && this->m_pRight->m_polyIndex >= 0)
      {
      int lastSubnode = this->m_pRight->GetSubnodeCount() - 1;
      pNodeRight = (ReachSubnode*) this->m_pRight->m_subnodeArray[lastSubnode];
      }

   if (pNodeLeft != NULL)
      tempLeft = pNodeLeft->m_temperature;
   if (pNodeRight != NULL)
      tempRight = pNodeRight->m_temperature;

   return true;
   }


///////////////////////////////////////////////////////////////////////////////
// R E S E R V O I R  
///////////////////////////////////////////////////////////////////////////////

Reservoir::Reservoir(void)
   : FluxContainer()
   , StateVarContainer()
   , m_id(-1)
   , m_col(-1)
   , m_dam_top_elev(0)
   , m_fc1_elev(0)
   , m_fc2_elev(0)
   , m_fc3_elev(0)
   , m_tailwater_elevation(0)
   , m_buffer_elevation(0)
   , m_turbine_efficiency(0)
   , m_minOutflow(0)
   , m_maxVolume(1300)
   , m_gateMaxPowerFlow(0)
   , m_maxPowerFlow(0)
   , m_minPowerFlow(0)
   , m_powerFlow(0)
   , m_gateMaxRO_Flow(0)
   , m_maxRO_Flow(0)
   , m_minRO_Flow(0)
   , m_RO_flow(0)
   , m_gateMaxSpillwayFlow(0)
   , m_maxSpillwayFlow(0)
   , m_minSpillwayFlow(0)
   , m_spillwayFlow(0)
   , m_reservoirType(ResType_FloodControl)
   , m_releaseFreq(1)
   , m_inflow(0)
   , m_outflow(0)
   , m_elevation(0)
   , m_power(0)
   , m_filled(0)
   , m_volume(50)
   , m_pDownstreamReach(NULL)
   , m_pFlowfromUpstreamReservoir(NULL)
   , m_pAreaVolCurveTable(NULL)
   , m_pRuleCurveTable(NULL)
   , m_pBufferZoneTable(NULL)
   , m_pCompositeReleaseCapacityTable(NULL)
   , m_pRO_releaseCapacityTable(NULL)
   , m_pSpillwayReleaseCapacityTable(NULL)
   , m_pRulePriorityTable(NULL)
   , m_pResData(NULL)
   //, m_pResMetData( NULL )
   , m_pResSimFlowCompare(NULL)
   , m_pResSimFlowOutput(NULL)
   , m_pResSimRuleCompare(NULL)
   , m_pResSimRuleOutput(NULL)
   , m_activeRule(_T("Uninitialized"))
   , m_zone(-1)
   , m_daysInZoneBuffer(0)
   , m_probMaintenance(-1)
   , m_avgPoolVolJunAug(0)
   , m_avgPoolElevJunAug(0)
   { }


Reservoir::~Reservoir(void)
   {
   if (m_pAreaVolCurveTable != NULL)
      delete m_pAreaVolCurveTable;

   if (m_pRuleCurveTable != NULL)
      delete m_pRuleCurveTable;

   if (m_pCompositeReleaseCapacityTable != NULL)
      delete m_pCompositeReleaseCapacityTable;

   if (m_pRO_releaseCapacityTable != NULL)
      delete m_pRO_releaseCapacityTable;

   if (m_pSpillwayReleaseCapacityTable != NULL)
      delete m_pSpillwayReleaseCapacityTable;

   if (m_pRulePriorityTable != NULL)
      delete m_pRulePriorityTable;

   if (m_pResData != NULL)
      delete m_pResData;

   //if (m_pResMetData != NULL)
   //   delete m_pResMetData;

   if (m_pResSimFlowCompare != NULL)
      delete m_pResSimFlowCompare;

   if (m_pResSimFlowOutput != NULL)
      delete m_pResSimFlowOutput;

   if (m_pResSimRuleCompare != NULL)
      delete m_pResSimRuleCompare;

   if (m_pResSimRuleOutput != NULL)
      delete m_pResSimRuleOutput;

   }


ControlPoint::~ControlPoint(void)
   {
   if (m_pControlPointConstraintsTable != NULL)
      delete m_pControlPointConstraintsTable;

   if (m_pResAllocation != NULL)
      delete  m_pResAllocation;
   }


void Reservoir::InitDataCollection(FlowModel *pFlowModel)
   {
   if (m_pResData == NULL)
      {
      if (m_reservoirType == ResType_CtrlPointControl)
         m_pResData = new FDataObj(4, 0, U_DAYS);
      else if (m_reservoirType == ResType_Optimized)
         m_pResData = new FDataObj(3, 0, U_DAYS);
      else
         m_pResData = new FDataObj(8, 0, U_DAYS);

      ASSERT(m_pResData != NULL);
      }

   CString name(this->m_name);
   name += _T(" Reservoir");
   m_pResData->SetName(name);

   if (m_reservoirType == ResType_CtrlPointControl)
      {
      m_pResData->SetLabel(0, _T("Time"));
      m_pResData->SetLabel(1, _T("Pool Elevation (m)"));
      m_pResData->SetLabel(2, _T("Inflow (m3/s)"));
      m_pResData->SetLabel(3, _T("Outflow (m3/s)"));
      }
   else
      {
      m_pResData->SetLabel(0, _T("Time"));
      m_pResData->SetLabel(1, _T("Pool Elevation (m)"));
      m_pResData->SetLabel(2, _T("Rule Curve Elevation (m)"));
      m_pResData->SetLabel(3, _T("Inflow (m3/s)"));
      m_pResData->SetLabel(4, _T("Outflow (m3/s)"));
      m_pResData->SetLabel(5, _T("Powerhouse Flow (m3/s)"));
      m_pResData->SetLabel(6, _T("Regulating Outlet Flow (m3/s)"));
      m_pResData->SetLabel(7, _T("Spillway Flow (m3/s)"));
      }

   pFlowModel->AddOutputVar(name, m_pResData, "");

   //if (m_pResMetData == NULL)
   //   {
   //   m_pResMetData = new FDataObj(7, 0);
   //   ASSERT(m_pResMetData != NULL);
   //   }
   //
   //CString name2(name);
   //name2 += _T(" Met");
   //m_pResMetData->SetName(name2);
   //
   //m_pResMetData->SetLabel(0, _T("Time"));
   //m_pResMetData->SetLabel(1, _T("Avg Daily Temp (C)"));
   //m_pResMetData->SetLabel(2, _T("Precip (mm/d)"));
   //m_pResMetData->SetLabel(3, _T("Shortwave Incoming (watts/m2)"));
   //m_pResMetData->SetLabel(4, _T("Specific Humidity "));
   //m_pResMetData->SetLabel(5, _T("Wind Speed (m/s)"));
   //m_pResMetData->SetLabel(6, _T("Wind Direction"));
   //gpFlow->AddOutputVar(name2, m_pResMetData, "");

   if (m_pResSimFlowCompare == NULL)
      {
      m_pResSimFlowCompare = new FDataObj(7, 0, U_DAYS);
      ASSERT(m_pResSimFlowCompare != NULL);
      }

   m_pResSimFlowCompare->SetLabel(0, _T("Time"));
   m_pResSimFlowCompare->SetLabel(1, _T("Pool Elevation (m) - ResSIM"));
   m_pResSimFlowCompare->SetLabel(2, _T("Pool Elevation (m) - Envision"));
   m_pResSimFlowCompare->SetLabel(3, _T("Inflow (m3/s) - ResSIM"));
   m_pResSimFlowCompare->SetLabel(4, _T("Inflow (m3/s) - Envision"));
   m_pResSimFlowCompare->SetLabel(5, _T("Outflow (m3/s) - ResSIM"));
   m_pResSimFlowCompare->SetLabel(6, _T("Outflow (m3/s) - Envision"));

   CString name3(this->m_name);
   name3 += _T(" Reservoir - ReSIM Flow Comparison");
   m_pResSimFlowCompare->SetName(name3);

   if (m_pResSimRuleCompare == NULL)
      {
      m_pResSimRuleCompare = new VDataObj(7, 0, U_DAYS);
      ASSERT(m_pResSimRuleCompare != NULL);
      }

   m_pResSimRuleCompare->SetLabel(0, _T("Time"));
   m_pResSimRuleCompare->SetLabel(1, _T("Blank"));
   m_pResSimRuleCompare->SetLabel(2, _T("Inflow (cms)"));
   m_pResSimRuleCompare->SetLabel(3, _T("Outflow (cms)"));
   m_pResSimRuleCompare->SetLabel(4, _T("ews (m)"));
   m_pResSimRuleCompare->SetLabel(5, _T("Zone"));
   m_pResSimRuleCompare->SetLabel(6, _T("Active Rule - Envision"));

   CString name4(this->m_name);
   name4 += _T(" Reservoir - ReSIM Rule Comparison");
   m_pResSimRuleCompare->SetName(name4);

   // TEMPORARY
   name = m_name;
   name += _T("_AvgPoolVolJunAug");
   pFlowModel->AddOutputVar(name, this->m_avgPoolVolJunAug, "");

   // TEMPORARY
   name = m_name;
   name += _T("_AvgPoolElevJunAug");
   pFlowModel->AddOutputVar(name, this->m_avgPoolElevJunAug, "");
   }


// this function iterates through the columns of the rule priority table,
// loading up constraints
bool Reservoir::ProcessRulePriorityTable(EnvModel* pEnvModel, FlowModel *pFlowModel)
   {
   if (m_pRulePriorityTable == NULL)
      return false;

   // in this table, each column is a zone.  Each row is a csv file
   int cols = m_pRulePriorityTable->GetColCount();
   int rows = m_pRulePriorityTable->GetRowCount();

   this->m_zoneInfoArray.RemoveAll();
   this->m_resConstraintArray.RemoveAll();

   // iterate through the zones defined in the file
   for (int i = 0; i < cols; i++)
      {
      ZoneInfo *pZone = new ZoneInfo;
      ASSERT(pZone != NULL);

      pZone->m_zone = (ZONE)i;  // ???????

      this->m_zoneInfoArray.Add(pZone);

      // iterate through the csv files for this zone
      for (int j = 0; j < rows; j++)
         {
         CString filename = m_pRulePriorityTable->GetAsString(i, j);

         ResConstraint *pConstraint = FindResConstraint(pFlowModel->m_path + m_dir + m_rpdir + filename);

         if (pConstraint == NULL && filename.CompareNoCase(_T("Missing")) != 0) // not yet loaded?  and not _T('Missing'))?    Still not loaded?
            {
            pConstraint = new ResConstraint;
            ASSERT(pConstraint);

            pConstraint->m_constraintFileName = filename;

            // Parse heading to determine whether to look in control point folder or rules folder
            TCHAR heading[256];
            lstrcpy(heading, filename);

            TCHAR *control = NULL;
            TCHAR *next = NULL;
            TCHAR delim[] = _T(" ,_");  //allowable delimeters, space and underscore

            control = _tcstok_s(heading, delim, &next);  // Returns string at head of file. 
            int records = -1;

            if (lstrcmpi(control, _T("cp")) == 0)
               records = pFlowModel->LoadTable(pEnvModel, pFlowModel->m_path + m_dir + m_cpdir + filename, (DataObj**) &(pConstraint->m_pRCData), 0);
            else
               records = pFlowModel->LoadTable(pEnvModel, pFlowModel->m_path + m_dir + m_rpdir + filename, (DataObj**) &(pConstraint->m_pRCData), 0);

            if (records <= 0)  // No records? Display error msg
               {
               CString msg;
               msg.Format(_T("Flow: Unable to load reservoir constraints for Zone %i table %s in Reservoir rule priority file %s.  This constraint will be ignored..."),
                  i, (LPCTSTR)filename, (LPCTSTR)m_pRulePriorityTable->GetName());

               Report::ErrorMsg(msg);
               delete pConstraint;
               pConstraint = NULL;
               } // end of if (records <=0)
            else
               this->m_resConstraintArray.Add(pConstraint);
            }  // end of ( if ( pConstraint == NULL )

         // Get rule type (m_type).  Assume 1st characters of filename until first underscore or space contain rule type.  
         // Parse rule name and determine m_type  (maximum, minimum, Maximum rate of decrease, Maximum Rate of Increase)
         if (pConstraint != NULL)
            {
            TCHAR  name[256];
            lstrcpy(name, filename);

            TCHAR *ruletype = NULL;
            TCHAR *next = NULL;
            TCHAR  delim[] = _T(" ,_");  //allowable delimeters, space and underscore

            // Ruletypes
            ruletype = strtok_s(name, delim, &next);  //Returns string at head of file. 

            //Compare string at the head of the file to allowable ruletypes
            if (lstrcmpi(ruletype, _T("max")) == 0)
               pConstraint->m_type = RCT_MAX;
            else if (lstrcmpi(ruletype, _T("min")) == 0)
               pConstraint->m_type = RCT_MIN;
            else if (lstrcmpi(ruletype, _T("maxi")) == 0)
               pConstraint->m_type = RCT_INCREASINGRATE;
            else if (lstrcmpi(ruletype, _T("maxd")) == 0)
               pConstraint->m_type = RCT_DECREASINGRATE;
            else if (lstrcmpi(ruletype, _T("cp")) == 0)      //downstream control point
               pConstraint->m_type = RCT_CONTROLPOINT;
            else if (lstrcmpi(ruletype, _T("pow")) == 0)     //Power plant flow
               pConstraint->m_type = RCT_POWERPLANT;
            else if (lstrcmpi(ruletype, _T("ro")) == 0)     //RO flow
               pConstraint->m_type = RCT_REGULATINGOUTLET;
            else if (lstrcmpi(ruletype, _T("spill")) == 0)   //Spillway flow
               pConstraint->m_type = RCT_SPILLWAY;
            else
               pConstraint->m_type = RCT_UNDEFINED;    //Unrecognized rule type     
            }  //end of if pConstraint != NULL )

         if (pConstraint != NULL)    // if valid, add to the current zone
            {
            ASSERT(pZone != NULL);
            pZone->m_resConstraintArray.Add(pConstraint);
            }
         }  // end of: if ( j < rows )
      }  // end of: if ( i < cols );

   return true;
   }


// get pool elevation from volume - units returned are meters
float Reservoir::GetPoolElevationFromVolume()
   {
   if (m_pAreaVolCurveTable == NULL)
      return 0;

   float volume = (float)m_volume;

   float poolElevation = m_pAreaVolCurveTable->IGet(volume, 1, 0, IM_LINEAR);  // this is in m

   return poolElevation;
   }


// get pool volume from elevation - units returned are meters cubed
float Reservoir::GetPoolVolumeFromElevation(float elevation)
   {
   if (m_pAreaVolCurveTable == NULL)
      return 0;

   float poolVolume = m_pAreaVolCurveTable->IGet(elevation, 0, 1, IM_LINEAR);  // this is in m3

   return poolVolume;    //this is M3
   }

float Reservoir::GetBufferZoneElevation(int doy)
   {
   if (m_pBufferZoneTable == NULL)
      return 0;

   // convert time to day of year
   //int doy = ::GetDayOfYear( m_pFlowModel->m_month, m_pFlowModel->m_day, m_pFlowModel->m_year );   // 1-based
   //doy -= 1;  // make zero-based
   //int dayOfYear = int( fmod( m_timeInRun, 365 ) );  // zero based day of year

   //time = fmod( time, 365 );
   float bufferZoneElevation = m_pBufferZoneTable->IGet(float(doy), 1, IM_LINEAR);   // m
   return bufferZoneElevation;    //this is M
   }


// get target pool elevation from rule curve - units returned are meters
float Reservoir::GetTargetElevationFromRuleCurve(int doy)
   {
   if (m_pRuleCurveTable == NULL)
      return m_elevation;

   // convert time to day of year
   //int doy = ::GetDayOfYear( m_pFlowModel->m_month, m_pFlowModel->m_day, m_pFlowModel->m_year );   // 1-based
   //doy -= 1;  // make zero-based

   //int dayOfYear = int( fmod( m_timeInRun, 365 ) );  // zero based day of year
   //time = fmod( time, 365 );
   float target = m_pRuleCurveTable->IGet(float(doy), 1, IM_LINEAR);   // m
   return target;
   }


void Reservoir::UpdateMaxGateOutflows(Reservoir *pRes, float currentPoolElevation)
   {
   /*ASSERT(pRes->m_pRO_releaseCapacityTable != NULL);
   ASSERT(pRes->m_pSpillwayReleaseCapacityTable != NULL);*/

   if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_RiverRun)
      {
      if (pRes->m_pRO_releaseCapacityTable != NULL)
         pRes->m_maxRO_Flow = pRes->m_pRO_releaseCapacityTable->IGet(currentPoolElevation, 1, IM_LINEAR);

      pRes->m_maxSpillwayFlow = pRes->m_pSpillwayReleaseCapacityTable->IGet(currentPoolElevation, 1, IM_LINEAR);
      }
   }


float Reservoir::GetResOutflow(FlowModel *pFlowModel, Reservoir *pRes, int doy)
   {
   ASSERT(pRes != NULL);

   float outflow = 0.0f;

   float currentPoolElevation = pRes->GetPoolElevationFromVolume();
   pRes->m_elevation = currentPoolElevation;
   UpdateMaxGateOutflows(pRes, currentPoolElevation);

   // check for river run reservoirs 
   if (pRes->m_reservoirType == ResType_RiverRun || pRes->m_reservoirType == ResType_Optimized)
      {
      outflow = pRes->m_inflow / SEC_PER_DAY;    // convert outflow to m3/day
      pRes->m_activeRule = _T("RunOfRiver");
      }
   else
      {
      float targetPoolElevation = pRes->GetTargetElevationFromRuleCurve(doy);
      float targetPoolVolume = pRes->GetPoolVolumeFromElevation(targetPoolElevation);
      float currentVolume = pRes->GetPoolVolumeFromElevation(currentPoolElevation);
      float bufferZoneElevation = pRes->GetBufferZoneElevation(doy);

      if (currentVolume > pRes->m_maxVolume)
         {
         currentVolume = pRes->m_maxVolume;   //Don't allow res volumes greater than max volume.  This code may be removed once hydro model is calibrated.
         pRes->m_volume = currentVolume;      //Store adjusted volume as current volume.
         currentPoolElevation = pRes->GetPoolElevationFromVolume();  //Adjust pool elevation

         CString msgvol;
         msgvol.Format(_T("Flow: Reservoir volume at '%s' on day of year %i exceeds maximum.  Volume set to maxVolume. Mass Balance not closed."), (LPCTSTR)pRes->m_name, doy);
         Report::StatusMsg(msgvol);
         pRes->m_activeRule = _T("OverMaxVolume!");
         }

      pRes->m_volume = currentVolume;

      float desiredRelease = (currentVolume - targetPoolVolume) / SEC_PER_DAY;     //This would bring pool elevation back to the rule curve in one timestep.  Converted from m3/day to m3/s  (cms).  
                                                                   //This would be ideal if possible within the given constraints.
      if (currentVolume < targetPoolVolume) //if we want to fill the reservoir
         desiredRelease = pRes->m_minOutflow;
      //desiredRelease = 0.0f;//kbv 3/16/2014

      float actualRelease = desiredRelease;   //Before any constraints are applied


      pRes->m_activeRule = _T("GC");  //.Format(gc);               //Notation from Ressim...implies that guide curve is reached with actual release (e.g. no other constraints applied).

      ZONE zone = ZONE_UNDEFINED;   //zone 0 = top of dam, zone 1 = flood control high, zone 2 = conservation operations, zone 3 = buffer, zone 4 = alternate flood control 1, zone 5 = alternate flood control 2.  Right now, only use 1 and 2 (mmc 3/2/2012). 

      if (currentPoolElevation > pRes->m_dam_top_elev)
         {
         CString msgtop;
         msgtop.Format(_T("Flow: Reservoir elevation at '%s' on day of year %i exceeds dam top elevation."), (LPCTSTR)pRes->m_name, doy);
         Report::StatusMsg(msgtop);
         currentPoolElevation = pRes->m_dam_top_elev - 0.1f;    //Set just below top elev to keep from exceeding values in lookup tables
         zone = ZONE_TOP;
         pRes->m_activeRule = _T("OverTopElevation!");
         }

      else if (pRes->m_reservoirType == ResType_CtrlPointControl)
         {
         zone = ZONE_CONSERVATION;
         }
      else if (currentPoolElevation > pRes->m_fc1_elev)
         {
         zone = ZONE_TOP;
         pRes->m_activeRule = _T("OverFC1");
         }
      else if (currentPoolElevation > targetPoolElevation)
         {
         if (currentPoolElevation <= pRes->m_fc2_elev)
            zone = ZONE_ALTFLOODCONTROL1;
         else if (pRes->m_fc3_elev > 0 && currentPoolElevation > pRes->m_fc3_elev)
            zone = ZONE_ALTFLOODCONTROL2;
         else
            zone = ZONE_FLOODCONTROL;
         }
      else if (currentPoolElevation <= targetPoolElevation)
         {
   
         if (currentPoolElevation <= bufferZoneElevation) //in the buffer zone
            {
            zone = ZONE_BUFFER;
            if (pRes->m_zone == ZONE_CONSERVATION)//if zone changed from conservation
               pRes->m_daysInZoneBuffer = 1; //reset the number of days
            else if (pRes->m_zone == ZONE_BUFFER)//if the zone did not change (you were in the buffer on the previous day)
               pRes->m_daysInZoneBuffer++; //increment the days in the buffer

            }
         else                                           //in the conservation zone
            {
            if (pRes->m_daysInZoneBuffer > 1)
               zone = ZONE_CONSERVATION;
            else
               zone = ZONE_BUFFER;
            }
         }

      // Once we know what zone we are in, we can access the array of appropriate constraints for the particular reservoir and zone.       
      ZoneInfo *pZone = NULL;
      if (zone >= 0 && zone < pRes->m_zoneInfoArray.GetSize())
         pZone = pRes->m_zoneInfoArray.GetAt(zone);

      if (pZone != NULL)
         {
         // Loop through constraints and modify actual release.  Apply the flood control rules in order here.
         for (int i = 0; i < pZone->m_resConstraintArray.GetSize(); i++)
            {
            ResConstraint *pConstraint = pZone->m_resConstraintArray.GetAt(i);
            ASSERT(pConstraint != NULL);

            CString filename = pConstraint->m_constraintFileName;

            // Get first column label and appropriate data to use for lookup
            DataObj *pConstraintTable = pConstraint->m_pRCData;
            ASSERT(pConstraintTable != NULL);

            CString xlabel = pConstraintTable->GetLabel(0);

            int year = -1, month = 1, day = -1;
            float xvalue = 0;
            float yvalue = 0;

            if (_stricmp(xlabel, _T("date")) == 0)                           // Date based rule?  xvalue = current date.
               xvalue = (float)doy;
            else if (_stricmp(xlabel, _T("release_cms")) == 0)              // Release based rule?  xvalue = release last timestep
               xvalue = pRes->m_outflow / SEC_PER_DAY;                    // SEC_PER_DAY = cubic meters per second to cubic meters per day
            else if (_stricmp(xlabel, _T("pool_elev_m")) == 0)              // Pool elevation based rule?  xvalue = pool elevation (meters)
               xvalue = pRes->m_elevation;
            else if (_stricmp(xlabel, _T("inflow_cms")) == 0)               // Inflow based rule?   xvalue = inflow to reservoir
               xvalue = pRes->m_inflow / SEC_PER_DAY;
            else if (_stricmp(xlabel, _T("Outflow_lagged_24h")) == 0)     // 24h lagged outflow based rule?   xvalue = outflow from reservoir at last timestep
               xvalue = pRes->m_outflow / SEC_PER_DAY;                    // placeholder for now
            else if (_stricmp(xlabel, _T("date_pool_elev_m")) == 0)         // Lookup based on two values...date and pool elevation.  x value is date.  y value is pool elevation
               {
               xvalue = (float)doy;
               yvalue = pRes->m_elevation;
               }
            else if (_stricmp(xlabel, _T("date_water_year_type")) == 0)         //Lookup based on two values...date and wateryeartype (storage in 13 USACE reservoirs on May 20th).
               {
               xvalue = (float)doy;
               yvalue = pFlowModel->m_waterYearType;
               }
            else if (_stricmp(xlabel, _T("date_release_cms")) == 0)
               {
               xvalue = (float)doy;
               yvalue = pRes->m_outflow / SEC_PER_DAY;
               }
            else                                                    //Unrecognized xvalue for constraint lookup table
               {
               CString msg;
               msg.Format(_T("Flow:  Unrecognized x value for reservoir constraint lookup '%s', %s (id:%i) in stream network"), (LPCTSTR)pConstraint->m_constraintFileName, (LPCTSTR)pRes->m_name, pRes->m_id);
               Report::LogWarning(msg);
               }

            RCTYPE type = pConstraint->m_type;
            float constraintValue = 0;

            ASSERT(pConstraint->m_pRCData != NULL);

            switch (type)
               {
               case RCT_MAX:  //maximum
               {
               if (yvalue > 0)  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, yvalue, IM_LINEAR);
               else             //If not, just use xvalue
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);

               if (actualRelease >= constraintValue)
                  {
                  actualRelease = constraintValue;
                  pRes->m_activeRule = pConstraint->m_constraintFileName;    //update active rule
                  }
               }
               break;

               case RCT_MIN:  //minimum
               {
               if (yvalue > 0)  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, yvalue, IM_LINEAR);
               else             //If not, just use xvalue
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);

               if (actualRelease <= constraintValue)
                  {
                  actualRelease = constraintValue;
                  pRes->m_activeRule = pConstraint->m_constraintFileName;
                  }
               }
               break;

               case RCT_INCREASINGRATE:  //Increasing Rate
               {
               if (yvalue > 0)  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                  {
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, yvalue, IM_LINEAR);
                  constraintValue = constraintValue * 24;   //Covert hourly to daily
                  }
               else             //If not, just use xvalue
                  {
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  constraintValue = constraintValue * 24;   //Covert hourly to daily
                  }

               if (actualRelease >= (constraintValue + pRes->m_outflow / SEC_PER_DAY))   //Is planned release more than current release + contstraint? 
                  {
                  actualRelease = (pRes->m_outflow / SEC_PER_DAY) + constraintValue;  //If so, planned release can be no more than current release + constraint.
                  pRes->m_activeRule = pConstraint->m_constraintFileName;
                  }
               }
               break;

               case RCT_DECREASINGRATE:  //Decreasing Rate */
               {
               if (yvalue > 0)  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                  {
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, yvalue, IM_LINEAR);
                  constraintValue = constraintValue * 24;   //Covert hourly to daily
                  }
               else             //If not, just use xvalue
                  {
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  constraintValue = constraintValue * 24;   //Covert hourly to daily
                  }

               if (actualRelease <= (pRes->m_outflow / SEC_PER_DAY - constraintValue))    //Is planned release less than current release - contstraint?
                  {
                  actualRelease = (pRes->m_outflow / SEC_PER_DAY) - constraintValue;     //If so, planned release can be no less than current release - constraint.
                  pRes->m_activeRule = pConstraint->m_constraintFileName;
                  }
               }
               break;

               case RCT_CONTROLPOINT:  //Downstream control point 
               {
               CString filename = pConstraint->m_constraintFileName;
               // get control point location.  Assumes last characters of filename contain a COMID
               LPTSTR p = (LPTSTR)_tcsrchr(filename, _T('.'));

               if (p != NULL)
                  {
                  p--;
                  while (isdigit(*p))
                     p--;

                  int comID = atoi(p + 1);
                  pConstraint->m_comID = comID;
                  }

               // Determine which control point this is.....use COMID to identify
               for (int k = 0; k < pFlowModel->m_controlPointArray.GetSize(); k++)
                  {
                  ControlPoint *pControl = pFlowModel->m_controlPointArray.GetAt(k);
                  ASSERT(pControl != NULL);

                  if (pControl->InUse())
                     {
                     int location = pControl->m_location;     // Get COMID of this control point
                     //if (pControl->m_location == pConstraint->m_comID)  //Do they match?
                     if (_stricmp(pControl->m_controlPointFileName, pConstraint->m_constraintFileName) == 0)  //compare names
                        {
                        ASSERT(pControl->m_pResAllocation != NULL);
                        constraintValue = 0.0f;
                        int releaseFreq = 1;

                        for (int l = 0; l < pControl->m_influencedReservoirsArray.GetSize(); l++)
                           {
                           if (pControl->m_influencedReservoirsArray[l] == pRes)
                              {
                              if (pRes->m_reservoirType == ResType_CtrlPointControl)
                                 {
                                 int rowCount = pControl->m_pResAllocation->GetRowCount();
                                 releaseFreq = pControl->m_influencedReservoirsArray[l]->m_releaseFreq;

                                 if (releaseFreq > 1 && doy >= releaseFreq - 1)
                                    {
                                    for (int k = 1; k <= releaseFreq; k++)
                                       {
                                       float tmp = pControl->m_pResAllocation->Get(l, rowCount - k);
                                       constraintValue += pControl->m_pResAllocation->Get(l, rowCount - k);
                                       }
                                    constraintValue = constraintValue / releaseFreq;
                                    }
                                 //                  constraintValue = pControl->m_pResAllocation->IGet(0, l);   //Flow allocated from this control point
                                 }
                              else
                                 constraintValue = pControl->m_pResAllocation->IGet(0, l);   //Flow allocated from this control point

                              actualRelease += constraintValue;    //constraint value will be negative if flow needs to be withheld, positive if flow needs to be augmented

                              if (constraintValue > 0.0)         //Did this constraint affect release?  If so, save as active rule.
                                 {
                                 pRes->m_activeRule = pConstraint->m_constraintFileName;
                                 }
                              break;
                              }
                           }
                        }
                     }
                  }
               }
               break;

               case RCT_POWERPLANT:  //Power plant rule
               { //first, we have to strip the first header to get to the 2nd, which tells us whether the rule is a min or a max 
               TCHAR powname[256];
               lstrcpy(powname, filename);
               TCHAR *ruletype = NULL;
               TCHAR *next = NULL;
               TCHAR delim[] = _T(" ,_");  //allowable delimeters: , ; space ; underscore

               ruletype = _tcstok_s(powname, delim, &next);  //Strip header.  should be pow_ for power plant rules
               ruletype = _tcstok_s(NULL, delim, &next);  //Returns next string at head of file (max or min).

               if (_stricmp(ruletype, _T("Max")) == 0)  //Is this a maximum power flow?  Assign m_maxPowerFlow attribute.
                  {
                  ASSERT(pConstraint->m_pRCData != NULL);
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  pRes->m_maxPowerFlow = constraintValue;  //Just for this timestep.  m_gateMaxPowerFlow is the physical limitation for the reservoir.
                  }
               else if (_stricmp(ruletype, _T("Min")) == 0)   /// bug!!! maximum) == 0)  //Is this a minimum power flow?  Assign m_minPowerFlow attribute.
                  {
                  ASSERT(pConstraint->m_pRCData != NULL);
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  pRes->m_minPowerFlow = constraintValue;
                  }
               //  pRes->m_activeRule = pConstraint->m_constraintFileName;
               }
               break;

               case RCT_REGULATINGOUTLET:  //Regulating outlet rule
               {
               //first, we have to strip the first header to get to the 2nd, which tells us whether the rule is a min or a max 
               TCHAR roname[256];
               lstrcpy(roname, filename);
               TCHAR *ruletype = NULL;
               TCHAR *next = NULL;
               TCHAR delim[] = _T(" ,_");  //allowable delimeters: , ; space ; underscore
               ruletype = _tcstok_s(roname, delim, &next);  //Strip header.  should be pow_ for power plant rules
               ruletype = _tcstok_s(NULL, delim, &next);  //Returns next string at head of file (max or min).

               if (_stricmp(ruletype, _T("Max")) == 0)  //Is this a maximum RO flow?   Assign m_maxRO_Flow attribute.
                  {
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  pRes->m_maxRO_Flow = constraintValue;
                  }
               else if (_stricmp(ruletype, _T("Min")) == 0)  //Is this a minimum RO flow?   Assign m_minRO_Flow attribute.
                  {
                  ASSERT(pConstraint->m_pRCData != NULL);
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  pRes->m_minRO_Flow = constraintValue;
                  }
               //  pRes->m_activeRule = pConstraint->m_constraintFileName;
               }
               break;

               case RCT_SPILLWAY:   //Spillway rule
               {
               //first, we have to strip the first header to get to the 2nd, which tells us whether the rule is a min or a max 
               TCHAR spillname[256];
               lstrcpy(spillname, filename);
               TCHAR *ruletype = NULL;
               TCHAR *next = NULL;
               TCHAR delim[] = _T(" ,_");  //allowable delimeters: , ; space ; underscore

               ruletype = _tcstok_s(spillname, delim, &next);  //Strip header.  should be pow_ for power plant rules
               ruletype = _tcstok_s(NULL, delim, &next);  //Returns next string at head of file (max or min).

               if (_stricmp(ruletype, _T("Max")) == 0)  //Is this a maximum spillway flow?  Assign m_maxSpillwayFlow attribute.
                  {
                  ASSERT(pConstraint->m_pRCData != NULL);
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  pRes->m_maxSpillwayFlow = constraintValue;
                  }
               else if (_stricmp(ruletype, _T("Min")) == 0)  //Is this a minimum spillway flow?  Assign m_minSpillwayFlow attribute.
                  {
                  ASSERT(pConstraint->m_pRCData != NULL);
                  constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  pRes->m_minSpillwayFlow = constraintValue;
                  }
               //  pRes->m_activeRule = pConstraint->m_constraintFileName;
               }
               break;
               }  // end of: switch( pConstraint->m_type )
               /*
            float minVolum = pRes->GetPoolVolumeFromElevation(m_inactive_elev);
            //float targetPoolElevation = pRes->GetTargetElevationFromRuleCurve(doy);
            float currentVolume = pRes->GetPoolVolumeFromElevation(currentPoolElevation);
            float rc_outflowVolum = actualRelease*SEC_PER_DAY;//volume that would be drained (m3 over the day)


            if (rc_outflowVolum > currentVolume - minVolum)      //In the inactive zone, water is not accessible for release from any of the gates.
               {
               actualRelease = (currentVolume - minVolum)/SEC_PER_DAY*0.5f;
               CString resMsg;

               resMsg.Format(_T("Pool is only %8.1f m above inactive zone. Outflow set to drain %8.1fm above inactive zone.  RC outflow of %8.1f m3/s (from %s) would have resulted in %8.0f m3 of discharged water (over a day) but there is only %8.0f m3 above the inactive zone"), pRes->m_elevation - pRes->m_inactive_elev, (pRes->m_elevation - pRes->m_inactive_elev) / 2, rc_outflowVolum / SEC_PER_DAY, pRes->m_activeRule, rc_outflowVolum, currentVolume - minVolum);
                  pRes->m_activeRule = resMsg;

               }
               */
            if (actualRelease < 0.0f)
               actualRelease = 0.0f;
            pRes->m_zone = (int)zone;

            // if (actualRelease < pRes->m_minOutflow)              // No release values less than the minimum
            //     actualRelease = pRes->m_minOutflow;

            if (pRes->m_elevation < pRes->m_inactive_elev)      //In the inactive zone, water is not accessible for release from any of the gates.
               actualRelease = pRes->m_outflow / SEC_PER_DAY*0.5f;

            outflow = actualRelease;
            }//end of for ( int i=0; i < pZone->m_resConstraintArray.GetSize(); i++ )
         }
      }  // end of: else (not run of river gate flow

   //Code here to assign total outflow to powerplant, RO and spillway (including run of river projects) 
   if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_RiverRun)
      outflow = AssignReservoirOutletFlows(pRes, outflow);


   // output activ rule to log window
   CString msg;
   msg.Format(_T("Flow: Day %i: Active Rule for Reservoir %s is %s"), doy, (LPCTSTR)pRes->m_name, (LPCTSTR)pRes->m_activeRule);

   return outflow;
   }


float Reservoir::AssignReservoirOutletFlows(Reservoir *pRes, float outflow)
   {
   ASSERT(pRes != NULL);

   //Reset gate values to zero
   pRes->m_powerFlow = 0.0;
   pRes->m_RO_flow = 0.0;
   pRes->m_spillwayFlow = 0.0;

   if (outflow <= pRes->m_maxPowerFlow)
      pRes->m_powerFlow = outflow;        //If possible, all flow routed through the powerhouse
   else
      {
      pRes->m_powerFlow = pRes->m_maxPowerFlow;            //If not, first route all water possible through the powerhouse
      float excessFlow = outflow - pRes->m_maxPowerFlow;   //Calculate excess
      if (excessFlow <= pRes->m_maxRO_Flow)                //Next, route all remaining flow possible through the regulating outlet(s), if they exist
         {
         pRes->m_RO_flow = excessFlow;
         if (pRes->m_RO_flow < pRes->m_minRO_Flow)      //Check to make sure the RO flow is not below a stated minimum
            {
            pRes->m_RO_flow = pRes->m_minRO_Flow;      //If so increase the RO flow to the minimum and adjust the power flow
            pRes->m_powerFlow = (outflow - pRes->m_minRO_Flow);
            }
         }
      else
         {
         pRes->m_RO_flow = pRes->m_maxRO_Flow;    //If both power plant and RO are at max capacity, route remaining flow to spillway.

         excessFlow -= pRes->m_RO_flow;
         
         // excess flow cannot exceed the max spillway flow
         if ( excessFlow > pRes->m_maxSpillwayFlow )
            excessFlow = pRes->m_maxSpillwayFlow;

         if (pRes->m_spillwayFlow < pRes->m_minSpillwayFlow)   //If spillway has a minimum flow, reduce RO flow to accomodate
            {
            pRes->m_spillwayFlow = pRes->m_minSpillwayFlow;
            pRes->m_RO_flow -= (pRes->m_minSpillwayFlow - excessFlow);
            }
         else if (pRes->m_spillwayFlow > pRes->m_maxSpillwayFlow)    //Does spillway flow exceed the max?  Give warning.
            {
            CString spillmsg;
            spillmsg.Format(_T("Flow: Maximum spillway volume exceeded at '%s', (id:%i)"), (LPCTSTR)pRes->m_name, pRes->m_id);
            Report::Log(spillmsg);
            }
         }


      }
   //Reset max outflows to gate maximums for next timestep
   pRes->m_maxPowerFlow = pRes->m_gateMaxPowerFlow;
   pRes->m_maxRO_Flow = pRes->m_gateMaxRO_Flow;
   pRes->m_maxSpillwayFlow = pRes->m_gateMaxSpillwayFlow;

   float adjOutflow = (pRes->m_powerFlow + pRes->m_RO_flow + pRes->m_spillwayFlow);
   float massbalancecheck = outflow - adjOutflow;    //massbalancecheck should = 0
   return adjOutflow;
   }

void  Reservoir::CalculateHydropowerOutput(Reservoir *pRes)
   {
   float head = pRes->m_elevation - pRes->m_tailwater_elevation;
   float powerOut = (1000 * pRes->m_powerFlow*9.81f*head*pRes->m_turbine_efficiency) / 1000000;   //power output in MW
   pRes->m_power = powerOut;

   }

void Reservoir::CollectData(void)
   {

   }


////////////////////////////////////////////////////////////////////////////////////////////////
// F L O W     M O D E L
////////////////////////////////////////////////////////////////////////////////////////////////

FlowModel::FlowModel()
   : EnvModelProcess()
   , m_saveStateAtEndOfRun(false)
   , m_detailedOutputFlags(0)
   , m_detailedSaveInterval(1)
   , m_detailedSaveAfterYears(0)
   , m_reachTree()
   , m_minStreamOrder(0)
   , m_subnodeLength(0)
   , m_buildCatchmentsMethod(1)
   //, m_soilLayerCount( 1 )
   //, m_vegLayerCount( 0 )
   //, m_snowLayerCount( 0 )
   //, m_poolCount(1)
   , m_initTemperature(20)
   , m_hruSvCount(0)
   , m_reachSvCount(0)
   , m_reservoirSvCount(0)
   , m_colCatchmentArea(-1)
   , m_colCatchmentJoin(-1)
   , m_colCatchmentReachIndex(-1)
   , m_colCatchmentCatchID(-1)
   , m_colCatchmentHruID(-1)
   , m_colCatchmentReachId(-1)
   , m_colStreamFrom(-1)
   , m_colStreamTo(-1)
   , m_colStreamOrder(-1)
   , m_colHruSWC(-1)
   , m_colHruTemp(-1)
   , m_colHruTempYr(-1)
   , m_colHruTemp10Yr(-1)
   , m_colHruPrecip(-1)
   , m_colHruPrecipYr(-1)
   , m_colHruPrecip10Yr(-1)
   , m_colHruSWE(-1)
   , m_colHruMaxSWE(-1)
   , m_colHruApr1SWE10Yr(-1)
   , m_colHruApr1SWE(-1)
   , m_colSnowIndex(-1)
   , m_colRefSnow(-1)
   , m_colET_yr(-1)
   , m_colHRUPercentIrrigated(-1)
   , m_colHRUMeanLAI(-1)
   , m_colMaxET_yr(-1)
   , m_colIrrigation_yr(-1)
   , m_colIrrigation(-1)
   , m_colPrecip_yr(-1)
   , m_colRunoff_yr(-1)
   , m_colStorage_yr(-1)
   , m_colLulcB(-1)
   , m_colLulcA(-1)
   , m_colAgeClass(-1)
   , m_colAridityIndex(-1)
   , m_colStreamReachOutput(-1)
   , m_colResID(-1)
   , m_colCsvYear(-1)
   , m_colCsvMonth(-1)
   , m_colCsvDay(-1)
   , m_colCsvHour(-1)
   , m_colCsvMinute(-1)
   , m_colCsvPrecip(-1)
   , m_colCsvTMean(-1)
   , m_colCsvTMin(-1)
   , m_colCsvTMax(-1)
   , m_colCsvSolarRad(-1)
   , m_colCsvWindspeed(-1)
   , m_colCsvRelHumidity(-1)
   , m_colCsvSpHumidity(-1)
   , m_colCsvVPD(-1)
   , m_provenClimateIndex(0)      // assumes an "bad" HRUs point to the first climate index
   , m_colStreamCumArea(-1)
   , m_colCatchmentCumArea(-1)
   , m_colTreeID(-1)
   , m_colStationID(-1)
   , m_pMap(NULL)
   , m_pCatchmentLayer(NULL)
   , m_pStreamLayer(NULL)
   , m_pResLayer(NULL)
   , m_stopTime(0.0f)
   , m_currentTime(0.0f)
   , m_timeStep(1.0f)
   , m_collectionInterval(1)
   , m_useVariableStep(false)
   , m_rkvMaxTimeStep(10.0f)
   , m_rkvMinTimeStep(0.001f)
   , m_rkvTolerance(0.0001f)
   , m_rkvInitTimeStep(0.001f)
   , m_rkvTimeStep(1.0f)
   , m_pReachRouting(NULL)
   , m_pLateralExchange(NULL)
   , m_pHruVertExchange(NULL)

   //, m_extraSvRxnMethod;
   , m_extraSvRxnExtFnDLL(NULL)
   , m_extraSvRxnExtFn(NULL)
   //,  RESMETHOD  m_reservoirFluxMethod;
   , m_reservoirFluxExtFnDLL(NULL)
   , m_reservoirFluxExtFn(NULL)
   , m_pResInflows(NULL)
   , m_pCpInflows(NULL)

   //, m_reachSolutionMethod( RSM_RK4 )
   //, m_latExchMethod( HREX_LINEARRESERVOIR )
   //, m_hruVertFluxMethod( VD_BROOKSCOREY )
   //, m_gwMethod( GWT_NONE )
   , m_extraSvRxnMethod(EXSV_EXTERNAL)
   , m_reservoirFluxMethod(RES_RESSIMLITE)
   //, m_reachExtFnDLL( NULL )
   //, m_reachExtFn( NULL )
   //, m_reachTimeStep( 1.0f )
   //, m_latExchExtFnDLL( NULL )
   //, m_latExchExtFn( NULL )
   //, m_hruVertFluxExtFnDLL( NULL )
   //, m_hruVertFluxExtFn( NULL )
   //, m_gwExtFnDLL( NULL )
   //, m_gwExtFn( NULL )

   , m_wdRatio(10.0f)
   , m_n(0.014f)
   , m_minCatchmentArea(-1)
   , m_maxCatchmentArea(-1)
   , m_numReachesNoCatchments(0)
   , m_numHRUs(0)
   , m_pStreamQE(NULL)
   , m_pCatchmentQE(NULL)
   , m_pModelOutputQE(NULL)
   , m_pModelOutputStreamQE(NULL)
   , m_pMapExprEngine(NULL)
   , m_pStreamQuery(NULL)
   , m_pCatchmentQuery(NULL)
   //, m_pReachDischargeData( NULL )
   //, m_pHRUPrecipData( NULL )
   //, m_pHRUETData(NULL)
   , m_pTotalFluxData(NULL)
   , m_pGlobalFlowData(NULL)
   , m_mapUpdate(2)
   , m_gridClassification(2)
   , m_totalWaterInputRate(0)
   , m_totalWaterOutputRate(0)
   , m_totalWaterInput(0)
   , m_totalWaterOutput(0)
   , m_numQMeasurements(0)
   , m_waterYearType(2.0f)
   , m_year(0)
   , m_month(0)
   , m_day(0)
   , m_pErrorStatData(NULL)
   , m_pParameterData(NULL)
   , m_pDischargeData(NULL)
   , m_estimateParameters(false)
   , m_numberOfRuns(5)
   , m_numberOfYears(1)
   , m_nsThreshold(0.0f)
   , m_saveResultsEvery(10)
   , m_climateStationRuns(false)
   , m_runFromFile(false)
   //, m_pClimateStationData(NULL)
   , m_pHruGrid(NULL)
   , m_timeOffset(0)
   , m_path()
   , m_grid()
   //, m_climateFilePath()
   //, m_climateStationElev(0.0f)
   , m_loadClimateRunTime(0.0f)
   , m_globalFlowsRunTime(0.0f)
   , m_externalMethodsRunTime(0.0f)
   , m_gwRunTime(0.0f)
   , m_hruIntegrationRunTime(0.0f)
   , m_reservoirIntegrationRunTime(0.0f)
   , m_reachIntegrationRunTime(0.0f)
   , m_massBalanceIntegrationRunTime(0.0f)
   , m_totalFluxFnRunTime(0.0f)
   , m_reachFluxFnRunTime(0.0f)
   , m_hruFluxFnRunTime(0.0f)
   , m_collectDataRunTime(0.0f)
   , m_outputDataRunTime(0.0f)
   , m_checkMassBalance(0)
   , m_integrator(IM_RK4)
   , m_minTimeStep(0.001f)
   , m_maxTimeStep(1.0f)
   , m_initTimeStep(1.0f)
   , m_errorTolerance(0.0001f)
   , m_useRecorder(false)
   , m_currentFlowScenarioIndex(0)
   , m_currentEnvisionScenarioIndex(-1)
   , m_pGrid(NULL)
   , m_minElevation(float(1E-6))
   , m_volumeMaxSWE(0.0f)
   , m_dateMaxSWE(0)
   , m_annualTotalET(0)     // acre-ft
   , m_annualTotalPrecip(0) // acre-ft
   , m_annualTotalDischarge(0)  //acre-ft
   , m_annualTotalRainfall(0)  //acre-ft
   , m_annualTotalSnowfall(0)  //acre-ft
   , m_randNorm1(1, 1, 1)
   {
   AddInputVar(_T("Climate Scenario"), m_currentFlowScenarioIndex, _T("0=MIROC, 1=GFDL, 2=HadGEM, 3=Historic"));
   AddInputVar(_T("Use Parameter Estimation"), m_estimateParameters, _T("true if the model will be used to estimate parameters"));
   AddInputVar(_T("Grid Classification?"), m_gridClassification, _T("For Gridded Simulation:  Classify attribute 0,1,2?"));

   AddInputVar(_T("Climate Station Runs"), m_climateStationRuns, _T("true if the model will be run using multiple climate files"));
   AddInputVar(_T("Run From File"), m_runFromFile, _T("true if the model will be run multiple times based on external file"));

   AddOutputVar(_T("Date of Max Snow"), m_dateMaxSWE, _T("DOY of Max Snow"));
   AddOutputVar(_T("Volume Max Snow (cubic meters)"), m_volumeMaxSWE, _T("Volume Max Snow (m3)"));
   }


FlowModel::~FlowModel()
   {
   if (m_pStreamQE)
      delete m_pStreamQE;

   if (m_pCatchmentQE)
      delete m_pCatchmentQE;

   if (m_pModelOutputQE)
      delete m_pModelOutputQE;

   if (m_pModelOutputStreamQE)
      delete m_pModelOutputStreamQE;

   if (m_pMapExprEngine)
      delete m_pMapExprEngine;

   if (m_pGlobalFlowData != NULL)
      delete m_pGlobalFlowData;

   if (m_pTotalFluxData != NULL)
      delete m_pTotalFluxData;

   if (m_pErrorStatData != NULL)
      delete m_pErrorStatData;

   if (m_pParameterData != NULL)
      delete m_pParameterData;

   if (m_pDischargeData != NULL)
      delete m_pDischargeData;

   //if (m_pClimateStationData != NULL)
   //   delete m_pClimateStationData;

   if (m_pHruGrid != NULL)
      delete m_pHruGrid;

   // these are PtrArrays
   m_catchmentArray.RemoveAll();
   m_reservoirArray.RemoveAll();
   m_stateVarArray.RemoveAll();
   m_vertexArray.RemoveAll();
   m_reachHruLayerDataArray.RemoveAll();
   m_hruLayerExtraSVDataArray.RemoveAll();
   m_reservoirDataArray.RemoveAll();
   m_reachMeasuredDataArray.RemoveAll();
   m_tableArray.RemoveAll();
   m_parameterArray.RemoveAll();

   }


bool FlowModel::Init(EnvContext *pEnvContext, LPCTSTR initStr)
   {
   EnvExtension::Init(pEnvContext, initStr);

   MapLayer *pLayer = (MapLayer*)pEnvContext->pMapLayer;

   bool ok = LoadXml(initStr, pEnvContext);

   if (!ok)
      return false;

   m_processorsUsed = omp_get_num_procs();

   m_processorsUsed = (2 * m_processorsUsed / 4) + 1;   // default uses 2/3 of the cores
   if (m_maxProcessors > 0 && m_processorsUsed >= m_maxProcessors)
      m_processorsUsed = m_maxProcessors;

   omp_set_num_threads(m_processorsUsed);

   ASSERT(pEnvContext->pMapLayer != NULL);
   m_pMap = pEnvContext->pMapLayer->GetMapPtr();

   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;
   m_flowContext.pEnvContext = pEnvContext;
   m_flowContext.timing = GMT_INIT;

   if (m_catchmentLayer.IsEmpty())
      {
      m_pCatchmentLayer = m_pMap->GetLayer(0);
      m_catchmentLayer = m_pCatchmentLayer->m_name;
      }
   else
      m_pCatchmentLayer = m_pMap->GetLayer(m_catchmentLayer);

   if (m_pCatchmentLayer == NULL)
      {
      CString msg(_T("Flow: Unable to locate catchment layer '"));
      msg += m_catchmentLayer;
      msg += _T("' - this is a required layer");
      Report::ErrorMsg(msg, _T("Fatal Error"));
      return false;
      }

   m_pStreamLayer = m_pMap->GetLayer(m_streamLayer);
   if (m_pStreamLayer == NULL)
      {
      CString msg(_T("Flow: Unable to locate stream network layer '"));
      msg += m_streamLayer;
      msg += _T("' - this is a required layer");
      Report::ErrorMsg(msg, _T("Fatal Error"));
      return false;
      }


   int count = m_pCatchmentLayer->GetColCount();

   // <catchments>
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colCatchmentArea, m_areaCol, TYPE_FLOAT, CC_MUST_EXIST);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colElev, m_elevCol, TYPE_FLOAT, CC_AUTOADD);
   //EnvExtension::CheckCol(m_pCatchmentLayer, m_colLai, _T("LAI"), TYPE_FLOAT, CC_AUTOADD);
  // EnvExtension::CheckCol(m_pCatchmentLayer, m_colAgeClass, _T("AGECLASS"), TYPE_INT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colLulcB, _T("LULC_B"), TYPE_INT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colLulcA, _T("LULC_A"), TYPE_INT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colCatchmentJoin, m_catchmentJoinCol, TYPE_INT, CC_MUST_EXIST);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colCatchmentCatchID, m_catchIDCol, TYPE_INT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colCatchmentHruID, m_hruIDCol, TYPE_INT, CC_AUTOADD);

   
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruSWC,             _T("SWC"),          TYPE_FLOAT, CC_AUTOADD ); //Not used
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruTemp, _T("TEMP"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruTempYr, _T("TEMP_YR"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruTemp10Yr, _T("TEMP_10YR"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruPrecip, _T("PRECIP"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruPrecipYr, _T("PRECIP_YR"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruPrecip10Yr, _T("PRECIP_10YR"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruSWE, _T("SNOW"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruMaxSWE, _T("MAXSNOW"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruApr1SWE, _T("SNOW_APR"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruApr1SWE10Yr, _T("SNOW_APR10"), TYPE_FLOAT, CC_AUTOADD);
   //   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruDecadalSnow,     _T("SNOW_dec"),     TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colSnowIndex, _T("SnowIndex"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colAridityIndex, _T("AridityInd"), TYPE_FLOAT, CC_AUTOADD);  // 20 year moving avg
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colRefSnow, _T("SnowRefere"), TYPE_FLOAT, CC_AUTOADD);

   EnvExtension::CheckCol(m_pCatchmentLayer, m_colET_yr, _T("ET_yr"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colMaxET_yr, _T("MAX_ET_yr"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colPrecip_yr, _T("Precip_yr"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colRunoff_yr, _T("Runoff_yr"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colStorage_yr, _T("Storage_yr"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colIrrigation_yr, _T("Irrig_yr"), TYPE_FLOAT, CC_AUTOADD);

   EnvExtension::CheckCol(m_pCatchmentLayer, m_colIrrigation, "IRRIGATION", TYPE_INT, CC_AUTOADD);

  EnvExtension::CheckCol(m_pCatchmentLayer, m_colHRUPercentIrrigated, "HRU_IRR_P", TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHRUMeanLAI, "HRU_LAI", TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colAgeClass, "HRU_AGE", TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHruSWC, "HRU_SWC", TYPE_FLOAT, CC_AUTOADD);
   
   // <streams>
   EnvExtension::CheckCol(m_pStreamLayer, m_colStreamFrom, m_fromCol, TYPE_INT, CC_MUST_EXIST);  // required for building topology
   EnvExtension::CheckCol(m_pStreamLayer, m_colStreamTo, m_toCol, TYPE_INT, CC_MUST_EXIST);
   EnvExtension::CheckCol(m_pStreamLayer, m_colStreamJoin, m_streamJoinCol, TYPE_INT, CC_MUST_EXIST);
   EnvExtension::CheckCol(m_pStreamLayer, m_colStreamReachOutput, _T("Q"), TYPE_FLOAT, CC_AUTOADD);

   EnvExtension::CheckCol(m_pStreamLayer, m_colStreamCumArea, _T("CUM_AREA"), TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colCatchmentCumArea, _T("CUM_AREA"), TYPE_FLOAT, CC_AUTOADD);
   count = m_pCatchmentLayer->GetColCount();

   m_pStreamQE = new QueryEngine(m_pStreamLayer);
   m_pCatchmentQE = new QueryEngine(m_pCatchmentLayer);

   m_pCatchmentLayer->SetColData(m_colHruTemp, 0.0f, true);
   m_pCatchmentLayer->SetColData(m_colHruTempYr, 0.0f, true);
   m_pCatchmentLayer->SetColData(m_colHruTemp10Yr, 0.0f, true);

   m_pCatchmentLayer->SetColData(m_colHruPrecip, 0.0f, true);
   m_pCatchmentLayer->SetColData(m_colHruPrecipYr, 0.0f, true);
   m_pCatchmentLayer->SetColData(m_colHruPrecip10Yr, 0.0f, true);

   m_pCatchmentLayer->SetColData(m_colHruSWE, 0.0f, true);
   m_pCatchmentLayer->SetColData(m_colHruMaxSWE, 0.0f, true);
   m_pCatchmentLayer->SetColData(m_colHruApr1SWE, 0.0f, true);
   m_pCatchmentLayer->SetColData(m_colHruApr1SWE10Yr, 0.0f, true);

   Report::Log("Flow: Initializing reaches/catchments/reservoirs/control points");

   // build basic structures for streams, catchments, HRU's
   InitReaches();      // create, initialize reaches based on stream layer
   InitCatchments();   // create, initialize catchments based on catchment layer
   InitReservoirs();   // initialize reservoirs
   InitReservoirControlPoints( pEnvContext->pEnvModel);   //initialize reservoir control points

   // connect catchments to reaches
   Report::Log("Flow: Building topology");
   ConnectCatchmentsToReaches();

   PopulateCatchmentCumulativeAreas();

   IdentifyMeasuredReaches();

   ok = true; //InitializeReachSampleArray();   // allocates m_pReachDischargeData object

   if (this->m_pStreamQuery != NULL || (this->m_minCatchmentArea > 0 && this->m_maxCatchmentArea > 0))
      {
      EnvExtension::CheckCol(m_pCatchmentLayer, m_colCatchmentCatchID, m_catchIDCol, TYPE_INT, CC_AUTOADD);   // is this really necessary?
      SimplifyNetwork();
      }

   // allocate integrators, state variable ptr arrays
   InitIntegrationBlocks();

   // Init() to any plugins that have an init method
   InitPlugins();

   GlobalMethodManager::Init(&m_flowContext);

   // add output variables:
   m_pTotalFluxData = new FDataObj(7, 0, U_YEARS);
   m_pTotalFluxData->SetName("Global Flux Summary");
   m_pTotalFluxData->SetLabel(0, "Year");
   m_pTotalFluxData->SetLabel(1, "Annual Total ET (acre-ft)");
   m_pTotalFluxData->SetLabel(2, "Annual Total Precip( acre-ft)");
   m_pTotalFluxData->SetLabel(3, "Annual Total Discharge (acre-ft)");
   m_pTotalFluxData->SetLabel(4, "Delta (Precip-(ET+Discharge)) (acre-ft)");
   m_pTotalFluxData->SetLabel(5, "Annual Total Rainfall (acre-ft)");
   m_pTotalFluxData->SetLabel(6, "Annual Total Snowfall (acre-ft)");

   this->AddOutputVar("Global Flux Summary", m_pTotalFluxData, "");
   
   for (int i = 0; i < (int)m_reservoirArray.GetSize(); i++)
      {
      Reservoir *pRes = m_reservoirArray[i];
      CString name(pRes->m_name);
      name += "-Filled";
      this->AddOutputVar(name, pRes->m_filled, "");
      }

   // set up dataobj for any model outputs
   for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

      int moCount = 0;
      for (int i = 0; i < (int)pGroup->GetSize(); i++)
         {
         if (pGroup->GetAt(i)->m_inUse)
            moCount++;
         if (pGroup->GetAt(i)->m_pDataObjObs != NULL)
            moCount++;
         }

      ASSERT(pGroup->m_pDataObj == NULL);
      if (moCount > 0)
         {
         pGroup->m_pDataObj = new FDataObj(moCount + 1, 0, U_DAYS);
         pGroup->m_pDataObj->SetName(pGroup->m_name);
         pGroup->m_pDataObj->SetLabel(0, "Time");

         int col = 1;
         for (int i = 0; i < (int)pGroup->GetSize(); i++)
            {
            if (pGroup->GetAt(i)->m_inUse)
               pGroup->m_pDataObj->SetLabel(col, pGroup->GetAt(i)->m_name);

            if (pGroup->GetAt(i)->m_pDataObjObs != NULL)
               {
               CString name;
               name.Format("Obs:%s", (LPCTSTR)pGroup->GetAt(i)->m_name);
               int offset = ((pGroup->m_pDataObj->GetColCount() - 1) / 2);//remove 1 column (time) and divide by 2 to get the offset to the measured data
               pGroup->m_pDataObj->SetLabel(col + offset, name);       //Assumes that there is measured data for each observation point in this group!
               }
            col++;
            }
         AddOutputVar(pGroup->m_name, pGroup->m_pDataObj, pGroup->m_name);
         }
      }

   // call any initialization methods for fluxes
   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;
   m_flowContext.pEnvContext = pEnvContext;
   m_flowContext.svCount = m_hruSvCount - 1;

   float areaTotal = 0.0f;
   int numHRU = 0;
   int numPoly = 0;
   for (int i = 0; i < m_catchmentArray.GetSize(); i++)
      {
      Catchment *pCatchment = m_catchmentArray[i];
      areaTotal += pCatchment->m_area;
      for (int j = 0; j < pCatchment->GetHRUCount(); j++)
         {
         HRU *pHRU = pCatchment->GetHRU(j);
         for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
            numPoly++;
         numHRU++;
         }
      }
   //if (m_detailedOutputFileName)
   //   OpenDetailedOutputFile( 0);
   //if (m_detailedOutputFileNameQ)
   //   OpenDetailedOutputFile( 1);

   //if (m_detailedOutputFileName)
   //   OpenDetailedOutputFile();


   CString msg;
   msg.Format("Flow: Total Area %8.1f km2.  Average catchment area %8.1f km2, Average HRU area %8.1f km2, number of polygons %i",
      areaTotal / 10000.0f / 100.0f, areaTotal / m_catchmentArray.GetSize() / 10000.0f / 100.0f, areaTotal / numHRU / 10000.0f / 100.0f, numPoly);
   Report::Log(msg);

   int numParam = (int)m_parameterArray.GetSize();
   if (m_estimateParameters)
      msg.Format("Flow: Parameter Estimation run. %i parameters, %i runs, %i years. Scenario settings may override. Verify by checking Scenario variables ", numParam, m_numberOfRuns, m_numberOfYears);
   else
      msg.Format("Flow: Single Run.  %i years. Scenario settings may override. Verify by checking Scenario variables ", m_numberOfYears);
   Report::Log(msg);

   msg.Format("Flow: Processors available=%i, Max Processors specified=%i, Processors used=%i", ::omp_get_num_procs(), m_maxProcessors, m_processorsUsed);
   Report::Log(msg);

   return true;
   }


void FlowModel::SummarizeIDULULC()
   {

   float lai = 0.0f; float age = 0.0f; int lulcB = 0; int lulcA = 0; int irrigation = 0;
   for (int i = 0; i < m_catchmentArray.GetSize(); i++)
      {
      Catchment *pCatchment = m_catchmentArray[i];
      for (int j = 0; j < pCatchment->GetHRUCount(); j++)
         {
         HRU *pHRU = pCatchment->GetHRU(j);
         float meanLAI = 0.0f;
         float areaIrrigated = 0.0f;
         float totalArea = 0.0f;
         float area = 0.0f;
         for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
            {
            m_flowContext.pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[k], m_colCatchmentArea, area);
            m_flowContext.pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[k], m_colLai, lai);
            meanLAI += lai*area;
            totalArea += area;

            m_flowContext.pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[k], m_colIrrigation, irrigation);

            if (irrigation == 1)    // it is irrigated
               areaIrrigated += area;
            }

         pHRU->m_percentIrrigated = areaIrrigated / totalArea;
         pHRU->m_meanLAI = meanLAI / totalArea;
         if (!m_estimateParameters)
            {
            for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
               {
               this->UpdateIDU(m_flowContext.pEnvContext, pHRU->m_polyIndexArray[k], m_colHRUPercentIrrigated, pHRU->m_percentIrrigated, ADD_DELTA);
           //    this->UpdateIDU(m_flowContext.pEnvContext, pHRU->m_polyIndexArray[k], m_colHRUMeanLAI, pHRU->m_meanLAI, SET_DATA);
               }
            }
         }
      }
   }

bool FlowModel::InitRun(EnvContext *pEnvContext, bool useInitialSeed)
   {
   EnvExtension::InitRun(pEnvContext, true);
   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;

   m_flowContext.pEnvContext = pEnvContext;
   m_flowContext.timing = GMT_INITRUN;

   m_currentEnvisionScenarioIndex = pEnvContext->scenarioIndex;

   m_totalWaterInputRate = 0;
   m_totalWaterOutputRate = 0;
   m_totalWaterInput = 0;
   m_totalWaterOutput = 0;

  // SummarizeIDULULC();

   // ReadState();//read file including initial values for the state variables. This avoids model spin up issues

   InitRunPlugins();
   InitRunReservoirs(pEnvContext);
   GlobalMethodManager::InitRun(&m_flowContext);

   m_pTotalFluxData->ClearRows();

   // if model outputs are in use, set up the data object
   for (int i = 0; i < (int)m_modelOutputGroupArray.GetSize(); i++)
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[i];

      if (pGroup->m_pDataObj != NULL)
         {
         pGroup->m_pDataObj->ClearRows();
         pGroup->m_pDataObj->SetSize(pGroup->m_pDataObj->GetColCount(), 0);
         }
      }

   if (m_useRecorder && !m_estimateParameters)
      {
      for (int i = 0; i < (int)m_vrIDArray.GetSize(); i++)
         ::EnvStartVideoCapture(m_vrIDArray[i]);
      }

   OpenDetailedOutputFiles();
   
   if (m_climateStationRuns) //we need to find the closest station and write the offset to the IDU
      {
      //Get the climate stations for this run.  These are coordinates for ALL the climate stations 
      int siteCount=16;
      int runNumber = m_flowContext.pEnvContext->run;

//      if (runNumber > 0)
//         siteCount = 4;

      FDataObj *pGridLocationData = new FDataObj;
      pGridLocationData->ReadAscii("GridLocations.csv");

      //get the subset of stations to be used in this model run
      FDataObj* pRealizationData = new FDataObj;
      pRealizationData->ReadAscii("C:\\Envision\\StudyAreas\\UGA_HBV\\ClimateStationsForEachRealization.csv");
      //m_numberOfRuns = pRealizationData->GetRowCount();//override the value from the flow input xml (in ParameterEstimation)
      //Have StationId, now write them into the IDU using nearest distance

      Vertex v(1, 1);

      for (int i = 0; i < m_flowContext.pEnvContext->pMapLayer->m_pPolyArray->GetSize(); i++)
         {
         Poly* pPoly = m_flowContext.pEnvContext->pMapLayer->m_pPolyArray->GetAt(i);
         float minDist = 1e10; int location = 0;
         for (int j=0;j<siteCount;j++)
            { 
            int realizRow = pRealizationData->GetAsInt(j+1 , runNumber);
 
            float x = pGridLocationData->Get(8, realizRow-1);
            float y = pGridLocationData->Get(9, realizRow-1);

            //Have x and y, now get distance to this polygon
            Vertex v = pPoly->GetCentroid();

            float dist = sqrt((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y));

            if (dist<minDist)
               { 
               minDist=dist;
               location= realizRow;
               int s=1;
               }
            }

         m_flowContext.pFlowModel->UpdateIDU(m_flowContext.pEnvContext, i, m_colStationID, location , ADD_DELTA);
         }
      delete pGridLocationData;
      delete pRealizationData;
      }


   if (m_estimateParameters)
      {
      for (int i = 0; i < m_numberOfYears; i++)
         {
         FlowScenario *pScenario = m_scenarioArray.GetAt(m_currentFlowScenarioIndex);

         if ( pScenario->m_useStationData == false )
            {
            for (int j = 0; j < (int)pScenario->m_climateInfoArray.GetSize(); j++)//different climate parameters
               {
               ClimateDataInfo *pInfo = pScenario->m_climateInfoArray[j];
               GeoSpatialDataObj *pObj = new GeoSpatialDataObj(U_UNDEFINED);
               pObj->InitLibraries();
               pInfo->m_pNetCDFDataArray.Add(pObj);
               }
            }
         }



      InitializeParameterEstimationSampleArray();
      int initYear = pEnvContext->currentYear + 1;//the value of pEnvContext lags the current year by 1????  see envmodel.cpp ln 3008 and 3050
      for (int i = 0; i < m_numberOfRuns; i++)
         {

         UpdateMonteCarloInput(pEnvContext, i);
         pEnvContext->run = i;
         pEnvContext->currentYear = initYear;
         if (i > 0)
             GlobalMethodManager::InitRun(&m_flowContext);

         for (int j = 0; j < m_numberOfYears; j++)
            {
            if (j != 0)
               pEnvContext->currentYear++;

            pEnvContext->yearOfRun = j;

            Run(pEnvContext);
            }

         UpdateMonteCarloOutput(pEnvContext, i);

         ResetDataStorageArrays(pEnvContext);

         if (m_saveStateAtEndOfRun)
            SaveState();

         EndRun(pEnvContext);
         }
      }

   m_loadClimateRunTime = 0;
   m_globalFlowsRunTime = 0;
   m_externalMethodsRunTime = 0;
   m_gwRunTime = 0;
   m_hruIntegrationRunTime = 0;
   m_reservoirIntegrationRunTime = 0;
   m_reachIntegrationRunTime = 0;
   m_massBalanceIntegrationRunTime = 0;
   m_totalFluxFnRunTime = 0;
   m_reachFluxFnRunTime = 0;
   m_hruFluxFnRunTime = 0;
   m_collectDataRunTime = 0;
   m_outputDataRunTime = 0;

   return TRUE;
   }


bool FlowModel::EndRun(EnvContext *pEnvContext)
   {
   if (m_saveStateAtEndOfRun)
      SaveState();
   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;
   m_flowContext.pEnvContext = pEnvContext;
   m_flowContext.timing = GMT_ENDRUN;

   EndRunPlugins();
   GlobalMethodManager::EndRun(&m_flowContext);

   CloseDetailedOutputFiles();

   if (m_useRecorder && !m_estimateParameters)
      {
      for (int i = 0; i < (int)m_vrIDArray.GetSize(); i++)
         ::EnvEndVideoCapture(m_vrIDArray[i]);   // capture main map
      }

   //Get the objective function for each of the locations with measured values
   if (!m_estimateParameters)
   {
      int c = 0;
      float ns = -1.0f; float nsLN = -1.0f; float ve = 0.0f;
      for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
      {
         ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

         for (int i = 0; i < (int)pGroup->GetSize(); i++)
         {
            if (pGroup->GetAt(i)->m_inUse)
            {
               ModelOutput *pOutput = pGroup->GetAt(i);
               if (pOutput->m_pDataObjObs != NULL && pGroup->m_pDataObj != NULL)
               {
                  GetObjectiveFunction(pGroup->m_pDataObj, ns, nsLN, ve);//this will include time meas model
                  LPCTSTR format = "Flow: %s All Data ns = %.3f, nsLog = %.3f, volumeError = %.3f";
                  CString msg; msg.Format(format, pGroup->m_name, ns, nsLN, ve); Report::Log(msg);

                  GetObjectiveFunctionGrouped(pGroup->m_pDataObj, ns, nsLN, ve);//this will include time meas model
                  format = "Flow: %s Daily ns = %.3f, nsLog = %.3f, volumeError = %.3f";
                  msg; msg.Format(format, pGroup->m_name, ns, nsLN, ve); Report::Log(msg);
                  c++;
               }
            }
         }
      }
   }

   // output timers
   ReportTimings("Flow: Climate Data Run Time = %.0f seconds", m_loadClimateRunTime);
   ReportTimings("Flow: Global Flows Run Time = %.0f seconds", m_globalFlowsRunTime);
   ReportTimings("Flow: External Methods Run Time = %.0f seconds", m_externalMethodsRunTime);
   ReportTimings("Flow: Groundwater Run Time = %.0f seconds", m_gwRunTime);
   ReportTimings("Flow: HRU Integration Run Time = %.0f seconds (fluxes = %.0f seconds)", m_hruIntegrationRunTime, m_hruFluxFnRunTime);
   ReportTimings("Flow: Reservoir Integration Run Time = %.0f seconds", m_reservoirIntegrationRunTime);
   ReportTimings("Flow: Reach Integration Run Time = %.0f seconds (fluxes = %0.f seconds)", m_reachIntegrationRunTime, m_reachFluxFnRunTime);
   ReportTimings("Flow: Total Flux Function Run Time = %.0f seconds", m_totalFluxFnRunTime);
   ReportTimings("Flow: Model Output Collection Run Time = %.0f seconds", m_outputDataRunTime);
   ReportTimings("Flow: Overall Mass Balance Run Time = %.0f seconds", m_massBalanceIntegrationRunTime);
   return true;
   }

int FlowModel::OpenDetailedOutputFiles()
   {
   int version = 1;
   int numElements = 12;
   int numElementsStream = 1;

   if (m_detailedOutputFlags & 1) // daily IDU
      {
      if (!m_pGrid)
         OpenDetailedOutputFilesIDU(m_iduDailyFilePtrArray);
      else
         OpenDetailedOutputFilesGrid(m_iduDailyFilePtrArray);
      }

   if (m_detailedOutputFlags & 2) // annual IDU
      OpenDetailedOutputFilesIDU(m_iduAnnualFilePtrArray);

   if (m_detailedOutputFlags & 4) // daily Reach
      OpenDetailedOutputFilesReach(m_reachDailyFilePtrArray);

   if (m_detailedOutputFlags & 8) // annual Reach
      OpenDetailedOutputFilesReach(m_reachAnnualFilePtrArray);

   if (m_pGrid)
      {
      if (m_detailedOutputFlags & 16) // daily IDU Tracer
         OpenDetailedOutputFilesGridTracer(m_iduDailyTracerFilePtrArray);
      }

   return 1;
   }


int FlowModel::OpenDetailedOutputFilesIDU(CArray< FILE*, FILE* > &filePtrArray)
   {
   int hruCount = GetHRUCount();
   int hruPoolCount = GetHRUPoolCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int)m_reservoirArray.GetSize();
   int polyCount = m_pCatchmentLayer->GetRowCount();
   int modelStateVarCount = hruPoolCount;
   int numYears = m_flowContext.pEnvContext->endYear - m_flowContext.pEnvContext->startYear;
   int version = 1;

   CStringArray names;     // units??
   names.Add("lai");                   // 0
   names.Add("age");                   // 1
   names.Add("et_yr");                 // 2
   names.Add("MAX_ET_yr");                // 3
   names.Add("precip_yr");             // 4
   names.Add("irrig_yr");              // 5
   names.Add("runoff_yr");             // 6
   names.Add("storage_yr");            // 7
   names.Add("LULC_B");                // 8
   names.Add("LULC_A");                // 9
   names.Add("SWE_April1");            // 10
   names.Add("SWE_Max");                 // 11

   int numElements = 1;
   // close any open files
   for (int i = 0; i < filePtrArray.GetSize(); i++)
      {
      if (filePtrArray[i] != NULL)
         fclose(filePtrArray[i]);
      }

   // clear out any existing file ptrs
   filePtrArray.RemoveAll();

   int count = 0;
   for (int i = 0; i < names.GetSize(); i++)
      {
      CString outputPath;
      outputPath.Format("%s%s%s", PathManager::GetPath(PM_OUTPUT_DIR), (PCTSTR)names[i], ".flw");
      FILE *fp = NULL;
      PCTSTR file = (PCTSTR)outputPath;
      fopen_s(&fp, file, "wb");

      filePtrArray.Add(fp);

      if (fp != NULL)
         {
         // write headers
         TCHAR buffer[64];
         memset(buffer, 0, 64);
         strncpy_s(buffer, names[i], 63);

         fwrite(&version, sizeof(int), 1, fp);
         fwrite(buffer, sizeof(char), 64, fp);
         fwrite(&numYears, sizeof(int), 1, fp);
         fwrite(&hruCount, sizeof(int), 1, fp);
         fwrite(&hruPoolCount, sizeof(int), 1, fp);
         fwrite(&polyCount, sizeof(int), 1, fp);
         fwrite(&reachCount, sizeof(int), 1, fp);
         fwrite(&numElements, sizeof(int), 1, fp);

         for (int j = 0; j < m_pCatchmentLayer->GetRowCount(); j++)
            {
            float area = 0.0f;
            m_pCatchmentLayer->GetData(j, m_colCatchmentArea, area);
            fwrite(&area, sizeof(float), 1, fp);
            }

         // an indicator for each polygon that can be used to write data back to shapefile.
         // this assumes that the shapefile is NOT SORTED between when this value is written and when it is used.
         for (int j = 0; j < m_pCatchmentLayer->GetRowCount(); j++)
            {
            float polygonOffset = (float)j;
            fwrite(&polygonOffset, sizeof(float), 1, fp);
            }

         count++;
         }  // end of: if ( fp != NULL )
      }  // end of: for ( i < names.GetSize() )

   return count;
   }

int FlowModel::OpenDetailedOutputFilesGrid(CArray< FILE*, FILE* > &filePtrArray)
   {
   int hruCount = GetHRUCount();
   int hruPoolCount = GetHRUPoolCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int)m_reservoirArray.GetSize();
   int polyCount = m_pCatchmentLayer->GetRowCount();
   int modelStateVarCount = hruPoolCount;
   int numYears = m_flowContext.pEnvContext->endYear - m_flowContext.pEnvContext->startYear - m_detailedSaveAfterYears;
   int version = 1;
   int numRows = m_pGrid->GetRowCount();
   int numCols = m_pGrid->GetColCount();
   REAL width = 0.0f; REAL height = 0.0f;
   m_pGrid->GetCellSize(width, height);
   int collectionInterval = m_detailedSaveInterval;
   int res = (int)width;

   CStringArray names;     // units??

  /* names.Add("L1Depth");                   // 0
   names.Add("L2Depth");
   names.Add("L3Depth");
   names.Add("L4Depth");
   names.Add("L5Depth");
   names.Add("Precip");
   names.Add("AET");
   names.Add("Recharge");
   names.Add("PET");
   names.Add("ELWS");
   names.Add("Runoff");
   names.Add("GWOut");
   names.Add("LossToGW");
   names.Add("GainFromGW");
   names.Add("L0Depth");
   */
   names.Add("SWE");


   int numElements = 1;
   // close any open files
   for (int i = 0; i < filePtrArray.GetSize(); i++)
      {
      if (filePtrArray[i] != NULL)
         fclose(filePtrArray[i]);
      }

   // clear out any existing file ptrs
   filePtrArray.RemoveAll();

   int count = 0;
   for (int i = 0; i < names.GetSize(); i++)
      {
      CString outputPath;
      outputPath.Format("%s%s%s", PathManager::GetPath(PM_OUTPUT_DIR), (PCTSTR)names[i], ".flw");
      FILE *fp = NULL;
      PCTSTR file = (PCTSTR)outputPath;
      fopen_s(&fp, file, "wb");

      filePtrArray.Add(fp);

      if (fp != NULL)
         {
         // write headers
         TCHAR buffer[64];
         memset(buffer, 0, 64);
         strncpy_s(buffer, names[i], 63);

         fwrite(&version, sizeof(int), 1, fp);
         fwrite(buffer, sizeof(char), 64, fp);
         fwrite(&numYears, sizeof(int), 1, fp);
         fwrite(&hruCount, sizeof(int), 1, fp);
         fwrite(&hruPoolCount, sizeof(int), 1, fp);
         fwrite(&polyCount, sizeof(int), 1, fp);
         fwrite(&reachCount, sizeof(int), 1, fp);
         fwrite(&numElements, sizeof(int), 1, fp);
         fwrite(&numRows, sizeof(int), 1, fp);
         fwrite(&numCols, sizeof(int), 1, fp);
         fwrite(&res, sizeof(int), 1, fp);
         fwrite(&collectionInterval, sizeof(int), 1, fp);

         for (int i = 0; i < hruCount; i++)
            {
            HRU *pHRU = m_hruArray[i];
            int row = pHRU->m_demRow;
            int col = pHRU->m_demCol;
            fwrite(&row, sizeof(int), 1, fp);
            fwrite(&col, sizeof(int), 1, fp);
            }
         for (int i = 0; i < numCols; i++)
            {
            for (int j = 0; j < numRows; j++)
               {
               float value = 0.0f;
               m_pGrid->GetData(j, i, value);
               fwrite(&value, sizeof(float), 1, fp);
               }
            }
         count++;
         }  // end of: if ( fp != NULL )
      }  // end of: for ( i < names.GetSize() )

   return count;
   }

int FlowModel::OpenDetailedOutputFilesGridTracer(CArray< FILE*, FILE* > &filePtrArray)
   {
   int hruCount = GetHRUCount();
   int hruLayerCount = GetHRUPoolCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int)m_reservoirArray.GetSize();
   int polyCount = m_pCatchmentLayer->GetRowCount();
   int modelStateVarCount = hruLayerCount;
   int numYears = m_flowContext.pEnvContext->endYear - m_flowContext.pEnvContext->startYear - m_detailedSaveAfterYears;
   int version = 1;
   int numRows = m_pGrid->GetRowCount();
   int numCols = m_pGrid->GetColCount();
   REAL width = 0.0f; REAL height = 0.0f;
   m_pGrid->GetCellSize(width, height);

   int collectionInterval = m_detailedSaveInterval;

   int res = (int)width;

   CStringArray names;     // units??

   names.Add("L1Conc");                   // 0
   names.Add("L2Conc");
   names.Add("L3Conc");
   names.Add("L4Conc");
   names.Add("L5Conc");
   names.Add("L0Conc");


   int numElements = m_hruSvCount - 1;
   // close any open files
   for (int i = 0; i < filePtrArray.GetSize(); i++)
      {
      if (filePtrArray[i] != NULL)
         fclose(filePtrArray[i]);
      }

   // clear out any existing file ptrs
   filePtrArray.RemoveAll();

   int count = 0;
   for (int i = 0; i < names.GetSize(); i++)
      {
      CString outputPath;
      outputPath.Format("%s%s%s", PathManager::GetPath(PM_OUTPUT_DIR), (PCTSTR)names[i], ".flw");
      FILE *fp = NULL;
      PCTSTR file = (PCTSTR)outputPath;
      fopen_s(&fp, file, "wb");

      filePtrArray.Add(fp);

      if (fp != NULL)
         {
         // write headers
         TCHAR buffer[64];
         memset(buffer, 0, 64);
         strncpy_s(buffer, names[i], 63);

         fwrite(&version, sizeof(int), 1, fp);
         fwrite(buffer, sizeof(char), 64, fp);
         fwrite(&numYears, sizeof(int), 1, fp);
         fwrite(&hruCount, sizeof(int), 1, fp);
         fwrite(&hruLayerCount, sizeof(int), 1, fp);
         fwrite(&polyCount, sizeof(int), 1, fp);
         fwrite(&reachCount, sizeof(int), 1, fp);
         fwrite(&numElements, sizeof(int), 1, fp);
         fwrite(&numRows, sizeof(int), 1, fp);
         fwrite(&numCols, sizeof(int), 1, fp);
         fwrite(&res, sizeof(int), 1, fp);
         fwrite(&collectionInterval, sizeof(int), 1, fp);

         for (int i = 0; i < hruCount; i++)
            {
            HRU *pHRU = m_hruArray[i];
            int row = pHRU->m_demRow;
            int col = pHRU->m_demCol;
            fwrite(&row, sizeof(int), 1, fp);
            fwrite(&col, sizeof(int), 1, fp);
            }
         for (int i = 0; i < numCols; i++)
            {
            for (int j = 0; j < numRows; j++)
               {
               float value = 0.0f;
               m_pGrid->GetData(j, i, value);
               fwrite(&value, sizeof(float), 1, fp);
               }
            }
         count++;
         }  // end of: if ( fp != NULL )
      }  // end of: for ( i < names.GetSize() )

   return count;
   }


int FlowModel::OpenDetailedOutputFilesReach(CArray< FILE*, FILE* > &filePtrArray)
   {
   int hruCount = GetHRUCount();
   int hruPoolCount = GetHRUPoolCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int)m_reservoirArray.GetSize();
   int polyCount = m_pCatchmentLayer->GetRowCount();
   int modelStateVarCount = hruPoolCount;
   int numYears = m_flowContext.pEnvContext->endYear - m_flowContext.pEnvContext->startYear ;
   int version = 2;
   int collectionInterval = m_detailedSaveInterval;


   CStringArray names;
   names.Add("Q");         // 0

   int numElements = m_hruSvCount - 1;
   // close any open files
   for (int i = 0; i < filePtrArray.GetSize(); i++)
      {
      if (filePtrArray[i] != NULL)
         fclose(filePtrArray[i]);
      }

   // clear out any existing file ptrs
   filePtrArray.RemoveAll();

   int count = 0;
   for (int i = 0; i < names.GetSize(); i++)
      {
      CString outputPath;
      outputPath.Format("%s%s%s", PathManager::GetPath(PM_OUTPUT_DIR), (PCTSTR)names[i], ".flw");
      FILE *fp = NULL;
      PCTSTR file = (PCTSTR)outputPath;
      fopen_s(&fp, file, "wb");

      filePtrArray.Add(fp);

      if (fp != NULL)
         {
         TCHAR buffer[64];
         memset(buffer, 0, 64);
         strncpy_s(buffer, names[i], 63);

         fwrite(&version, sizeof(int), 1, fp);
         fwrite(buffer, sizeof(char), 64, fp);
         fwrite(&numYears, sizeof(int), 1, fp);
         fwrite(&hruCount, sizeof(int), 1, fp);
         fwrite(&hruPoolCount, sizeof(int), 1, fp);
         fwrite(&polyCount, sizeof(int), 1, fp);
         fwrite(&reachCount, sizeof(int), 1, fp);
         fwrite(&numElements, sizeof(int), 1, fp);
         fwrite(&collectionInterval, sizeof(int), 1, fp);

         for (int j = 0; j < reachCount; j++)
            {
            Reach *pReach = m_reachArray[j];
            if (pReach->m_reachIndex >= 0)
               fwrite(&pReach->m_reachIndex, sizeof(int), 1, fp);
            }

         for (int j = 0; j < reachCount; j++)
            {
            Reach *pReach = m_reachArray[j];
            if (pReach->m_reachIndex >= 0)
               fwrite(&pReach->m_reachID, sizeof(int), 1, fp);
            }

         count++;
         }
      }

   return count;
   }


int FlowModel::SaveDetailedOutputIDU(CArray< FILE*, FILE* > &filePtrArray)
   {
   ASSERT(filePtrArray.GetSize() == 13);

   int hruCount = GetHRUCount();
   int hruPoolCount = GetHRUPoolCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int)m_reservoirArray.GetSize();
   int modelStateVarCount = ((hruPoolCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;

   // iterate through IDUs
   for (int i = 0; i < m_pCatchmentLayer->GetRowCount(); i++)
      {
      float lai = 0.0f; float age = 0.0f; float lulcB = 0; float lulcA = 0;
      float snow = 0.0f; float snowIndex = 0.0f; float aridity = 0.0f; float irrig_yr = 0.0f;
      float et_yr = 0.0f; float MAX_ET_yr = 0.0f; float precip_yr = 0.0f; float runoff_yr = 0.0f; float storage_yr = 0.0f;
      float max_snow = 0.0f;
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colLai, lai);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colAgeClass, age);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colIrrigation, et_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colHruTemp, MAX_ET_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colHruPrecip, precip_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colIrrigation, irrig_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colRunoff_yr, runoff_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colStorage_yr, storage_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colLulcB, lulcB);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colLulcA, lulcA);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colHruSWE, snow);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colHruMaxSWE, max_snow);

      float height = (float)(46.6f*(1.0 - exp(-0.0144*age)));

      fwrite(&lai, sizeof(float), 1, filePtrArray[0]);
      fwrite(&age, sizeof(float), 1, filePtrArray[1]);
      fwrite(&et_yr, sizeof(float), 1, filePtrArray[2]);
      fwrite(&MAX_ET_yr, sizeof(float), 1, filePtrArray[3]);
      fwrite(&precip_yr, sizeof(float), 1, filePtrArray[4]);
      fwrite(&irrig_yr, sizeof(float), 1, filePtrArray[5]);
      fwrite(&runoff_yr, sizeof(float), 1, filePtrArray[6]);
      fwrite(&storage_yr, sizeof(float), 1, filePtrArray[7]);
      fwrite(&lulcB, sizeof(float), 1, filePtrArray[8]);
      fwrite(&lulcA, sizeof(float), 1, filePtrArray[9]);
      fwrite(&snow, sizeof(float), 1, filePtrArray[10]);
      fwrite(&max_snow, sizeof(float), 1, filePtrArray[11]);
      //fwrite(&snowIndex, sizeof(float), 1, m_fp_DetailedOutput); 
      //fwrite(&aridity, sizeof(float), 1, m_fp_DetailedOutput); 
      }

   return 1;
   }


int FlowModel::SaveDetailedOutputGrid(CArray< FILE*, FILE* > &filePtrArray)
   {
   ASSERT(filePtrArray.GetSize() == 1);

   int hruCount = GetHRUCount();
   int hruPoolCount = GetHRUPoolCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int)m_reservoirArray.GetSize();
   int modelStateVarCount = ((hruPoolCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;

   int numRows = m_pGrid->GetRowCount();
   int numCols = m_pGrid->GetColCount();


   for (int i = 0; i < numCols; i++)
      {
      for (int j = 0; j < numRows; j++)
         {
         float value = 0.0f;

         m_pGrid->GetData(j, i, value);//Go to the elevation grid and get the value
         if (value == m_pGrid->GetNoDataValue())
            {
          /*  fwrite(&value, sizeof(float), 1, filePtrArray[0]);
            fwrite(&value, sizeof(float), 1, filePtrArray[1]);
            fwrite(&value, sizeof(float), 1, filePtrArray[2]);
            fwrite(&value, sizeof(float), 1, filePtrArray[3]);
            fwrite(&value, sizeof(float), 1, filePtrArray[4]);
            fwrite(&value, sizeof(float), 1, filePtrArray[5]);
            fwrite(&value, sizeof(float), 1, filePtrArray[6]);
            fwrite(&value, sizeof(float), 1, filePtrArray[7]);
            fwrite(&value, sizeof(float), 1, filePtrArray[8]);
            fwrite(&value, sizeof(float), 1, filePtrArray[9]);
            fwrite(&value, sizeof(float), 1, filePtrArray[10]);
            fwrite(&value, sizeof(float), 1, filePtrArray[11]);
            fwrite(&value, sizeof(float), 1, filePtrArray[12]);
            fwrite(&value, sizeof(float), 1, filePtrArray[13]);
            fwrite(&value, sizeof(float), 1, filePtrArray[14]);*/
            fwrite(&value, sizeof(float), 1, filePtrArray[0]);
            }
         else
            {
            int offSet = m_pHruGrid->Get(i, j);//this gives us the offset in the m_hruArray for this particular grid location (i and j)

            if (offSet > 0)
               {

               HRU *pHRU = GetHRU(offSet);


               float l1 = 0.0f; float l2 = 0.0f; float l3 = 0.0f; float l4 = 0.0f; float l5 = 0.0f; float l0 = 0.0f;

               HRUPool *pLayer = pHRU->GetPool(0);
               float area = pHRU->m_area;
               float precip = pHRU->m_currentPrecip;//precipitation (mm/d )
               float pet = pHRU->m_currentMaxET;
               float swe = pHRU->m_depthSWE;


               float et = pLayer->m_waterFluxArray[FL_TOP_SINK] / area*1000.0f*-1.0f;//AET (m3/d converted to mm/d )
               float elws = pHRU->m_elws;
               float gwOut = pHRU->m_currentGWFlowOut;
               float runoff = pHRU->m_currentRunoff;
               float recharge = 0;
//               HRUPool *pLayer2 = pHRU->GetPool(2);//Layer above Groundwater...specific to 5 Layer SRS model
 //              et += pLayer2->m_waterFluxArray[FL_TOP_SINK] / area*1000.0f*-1.0f;//this adds et from argillic

               et = pHRU->m_currentET;
 //              float recharge = -1.0f*((float)pLayer2->m_waterFluxArray[FL_BOTTOM_SINK] + (float)pLayer2->m_waterFluxArray[FL_BOTTOM_SOURCE]) / area*1000.0f;
               float lossToGW = 0.0f; float gainFromGW = 0.0f;
               if (recharge > 0.0f)
                  lossToGW = recharge;
               else
                  gainFromGW = recharge*-1.0f;

               recharge = pHRU->m_currentGWRecharge;

               l1 = (float)pLayer->m_volumeWater / area*1000.0f;
               fwrite(&swe, sizeof(float), 1, filePtrArray[0]);
              /* fwrite(&l1, sizeof(float), 1, filePtrArray[0]);
               fwrite(&precip, sizeof(float), 1, filePtrArray[5]);
               fwrite(&pet, sizeof(float), 1, filePtrArray[8]);
               fwrite(&et, sizeof(float), 1, filePtrArray[6]);
               fwrite(&recharge, sizeof(float), 1, filePtrArray[7]);//mm/d where + means gw is being recharged and - means gw is discharging into argillic.
               fwrite(&elws, sizeof(float), 1, filePtrArray[9]);
               fwrite(&runoff, sizeof(float), 1, filePtrArray[10]);
               fwrite(&gwOut, sizeof(float), 1, filePtrArray[11]);
               fwrite(&lossToGW, sizeof(float), 1, filePtrArray[12]);
               fwrite(&gainFromGW, sizeof(float), 1, filePtrArray[13]);


               pLayer = pHRU->GetPool( 1);
               l2 = (float)pLayer->m_volumeWater / area*1000.0f;
               fwrite(&l2, sizeof(float), 1, filePtrArray[1]);


               pLayer = pHRU->GetPool( 2);
               l3 = (float)pLayer->m_volumeWater / area*1000.0f;
               fwrite(&l3, sizeof(float), 1, filePtrArray[2]);


               pLayer = pHRU->GetPool( 3);
               l4 = (float)pLayer->m_volumeWater / area*1000.0f;
               fwrite(&l4, sizeof(float), 1, filePtrArray[3]);


               pLayer = pHRU->GetPool( 4);
               l5 = (float)pLayer->m_volumeWater / area*1000.0f;
               fwrite(&l5, sizeof(float), 1, filePtrArray[4]);

               pLayer = pHRU->GetPool( 5);
               l0 = (float)pLayer->m_volumeWater / area*1000.0f;
               fwrite(&l0, sizeof(float), 1, filePtrArray[14]);

               */
               }
            else
               {
               float val = 0.0f;
               fwrite(&val, sizeof(float), 1, filePtrArray[0]);
              /* fwrite(&val, sizeof(float), 1, filePtrArray[1]);
               fwrite(&val, sizeof(float), 1, filePtrArray[2]);
               fwrite(&val, sizeof(float), 1, filePtrArray[3]);
               fwrite(&val, sizeof(float), 1, filePtrArray[4]);
               fwrite(&val, sizeof(float), 1, filePtrArray[5]);
               fwrite(&val, sizeof(float), 1, filePtrArray[6]);
               fwrite(&val, sizeof(float), 1, filePtrArray[7]);
               fwrite(&val, sizeof(float), 1, filePtrArray[8]);
               fwrite(&val, sizeof(float), 1, filePtrArray[9]);
               fwrite(&val, sizeof(float), 1, filePtrArray[10]);
               fwrite(&val, sizeof(float), 1, filePtrArray[11]);
               fwrite(&val, sizeof(float), 1, filePtrArray[12]);
               fwrite(&val, sizeof(float), 1, filePtrArray[13]);
               fwrite(&val, sizeof(float), 1, filePtrArray[14]);

               */
               }
            }
         }
      }

   return 1;
   }

int FlowModel::SaveDetailedOutputGridTracer(CArray< FILE*, FILE* > &filePtrArray)
   {
   ASSERT(filePtrArray.GetSize() == 6);

   int hruCount = GetHRUCount();
   int hruLayerCount = GetHRUPoolCount();
   int modelStateVarCount = (hruLayerCount*hruCount)*m_hruSvCount;

   int numRows = m_pGrid->GetRowCount();
   int numCols = m_pGrid->GetColCount();

   for (int k = 0; k < m_hruSvCount - 1; k++)
   {
      for (int i = 0; i < numCols; i++)
      {
         for (int j = 0; j < numRows; j++)
         {
            float value = 0.0f;

            m_pGrid->GetData(j, i, value);//Go to the elevation grid and get the value
            if (value == m_pGrid->GetNoDataValue())
            {
               fwrite(&value, sizeof(float), 1, filePtrArray[0]);
               fwrite(&value, sizeof(float), 1, filePtrArray[1]);
               fwrite(&value, sizeof(float), 1, filePtrArray[2]);
               fwrite(&value, sizeof(float), 1, filePtrArray[3]);
               fwrite(&value, sizeof(float), 1, filePtrArray[4]);
               fwrite(&value, sizeof(float), 1, filePtrArray[5]);
            }
            else
            {
               int offSet = m_pHruGrid->Get(i, j);//this gives us the offset in the m_hruArray for this particular grid location (i and j)

               if (offSet > 0)
               {
                  HRU *pHRU = GetHRU(offSet);

                  float l1 = 0.0f; float l2 = 0.0f; float l3 = 0.0f; float l4 = 0.0f; float l5 = 0.0f; float l0 = 0.0f;

                  HRUPool *pLayer = pHRU->GetPool( 0);
                  float area = pHRU->m_area;

                  // pLayer = pHRU->GetPool(0);
                  if (pLayer->m_volumeWater > 0.0f)
                     l2 = float(pLayer->m_svArray[k] / pLayer->m_volumeWater);
                  fwrite(&l2, sizeof(float), 1, filePtrArray[0]);


                  pLayer = pHRU->GetPool( 1);
                  if (pLayer->m_volumeWater > 0.0f)
                     l3 = float(pLayer->m_svArray[k] / pLayer->m_volumeWater);
                  fwrite(&l3, sizeof(float), 1, filePtrArray[1]);


                  pLayer = pHRU->GetPool( 2);
                  if (pLayer->m_volumeWater > 0.0f)
                     l4 = float(pLayer->m_svArray[k] / pLayer->m_volumeWater);
                  fwrite(&l4, sizeof(float), 1, filePtrArray[2]);


                  pLayer = pHRU->GetPool( 3);
                  if (pLayer->m_volumeWater > 0.0f)
                     l5 = float(pLayer->m_svArray[k] / pLayer->m_volumeWater);
                  fwrite(&l5, sizeof(float), 1, filePtrArray[3]);

                  pLayer = pHRU->GetPool( 4);
                  if (pLayer->m_volumeWater > 0.0f)
                     l0 = float(pLayer->m_svArray[k] / pLayer->m_volumeWater);
                  fwrite(&l0, sizeof(float), 1, filePtrArray[4]);

                  pLayer = pHRU->GetPool( 5);
                  if (pLayer->m_volumeWater > 0.0f)
                     l0 = float(pLayer->m_svArray[k] / pLayer->m_volumeWater);
                  fwrite(&l0, sizeof(float), 1, filePtrArray[5]);
               }
               else
               {
                  float val = 0.0f;
                  fwrite(&val, sizeof(float), 1, filePtrArray[0]);
                  fwrite(&val, sizeof(float), 1, filePtrArray[1]);
                  fwrite(&val, sizeof(float), 1, filePtrArray[2]);
                  fwrite(&val, sizeof(float), 1, filePtrArray[3]);
                  fwrite(&val, sizeof(float), 1, filePtrArray[4]);
                  fwrite(&val, sizeof(float), 1, filePtrArray[5]);
               }
            }
         }
      }
   }

   return 1;
   }

//   {
//   int hruCount = GetHRUCount();
//   int hruPoolCount = GetHRUPoolCount();
//   int reachCount = GetReachCount();
//   int reservoirCount = (int) m_reservoirArray.GetSize();
//   int modelStateVarCount = ((hruPoolCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;
//
//   for (int i = 0; i < hruCount; i++)
//      {
//      HRU *pHRU = m_hruArray[i];
//      for (int j = 0; j < hruPoolCount; j++)
//         {
//         HRUPool *pLayer = pHRU->GetPool(j);
//         float area=pHRU->m_area;
//         float value=pLayer->m_waterFluxArray[FL_TOP_SOURCE]/area*1000.0f;
//         fwrite(&value, sizeof(float), 1, m_fp_DetailedOutput);
//         value=pLayer->m_waterFluxArray[FL_TOP_SINK]/area*1000.0f;
//         fwrite(&value, sizeof(float), 1, m_fp_DetailedOutput);
//         value=pLayer->m_waterFluxArray[FL_BOTTOM_SOURCE]/area*1000.0f;
//         fwrite(&value, sizeof(float), 1, m_fp_DetailedOutput);
//         value=pLayer->m_waterFluxArray[FL_BOTTOM_SINK]/area*1000.0f;
//         fwrite(&value, sizeof(float), 1, m_fp_DetailedOutput);
//         value=pLayer->m_waterFluxArray[FL_STREAM_SINK]/area*1000.0f;
//         fwrite(&value, sizeof(float), 1, m_fp_DetailedOutput);
//         value=pLayer->m_waterFluxArray[FL_SINK]/area*1000.0f;
//         fwrite(&value, sizeof(float), 1, m_fp_DetailedOutput); 
//         }
//      }
//
   /*
   for (int i = 0; i < reachCount; i++)
      {
      Reach *pReach = m_reachArray[i];

      for (int k = 0; k < pReach->m_subnodeArray.GetSize(); k++)
         {
         ReachSubnode *pNode = pReach->GetReachSubnode(k);

         fwrite(&pNode->m_discharge, sizeof(float), 1, fp);
         for (int l = 0; l < m_hruSvCount - 1; l++)
            fwrite(&pNode->m_svArray[l], sizeof(float), 1, fp);

         }
      }

   //return 1;
   }
*/

int FlowModel::SaveDetailedOutputReach(CArray< FILE*, FILE* > &filePtrArray)
   {
   ASSERT(filePtrArray.GetSize() == 1);

   int reachCount = GetReachCount();

   for (int i = 0; i < reachCount; i++)
      {
      Reach *pReach = m_reachArray[i];
      ReachSubnode *pNode = pReach->GetReachSubnode(0);

      if (filePtrArray[0] != NULL)
         fwrite(&pNode->m_discharge, sizeof(float), 1, filePtrArray[0]);

      for (int k = 0; k < m_hruSvCount - 1; k++)
         {
         float sv = (float) pNode->GetStateVar(k);
         fwrite(&sv, sizeof(float), 1, filePtrArray[0]);
         }
      }

   return 1;
   }


void FlowModel::CloseDetailedOutputFiles()
   {
   for (int i = 0; i < (int)m_iduAnnualFilePtrArray.GetSize(); i++)
      {
      if (m_iduAnnualFilePtrArray[i] != NULL)
         {
         fclose(m_iduAnnualFilePtrArray[i]);
         m_iduAnnualFilePtrArray[i] = NULL;
         }
      }

   for (int i = 0; i < (int)m_iduDailyFilePtrArray.GetSize(); i++)
      {
      if (m_iduDailyFilePtrArray[i] != NULL)
         {
         fclose(m_iduDailyFilePtrArray[i]);
         m_iduDailyFilePtrArray[i] = NULL;
         }
      }

   for (int i = 0; i < (int)m_iduDailyTracerFilePtrArray.GetSize(); i++)
      {
      if (m_iduDailyTracerFilePtrArray[i] != NULL)
         {
         fclose(m_iduDailyTracerFilePtrArray[i]);
         m_iduDailyTracerFilePtrArray[i] = NULL;
         }
      }

   for (int i = 0; i < (int)m_reachAnnualFilePtrArray.GetSize(); i++)
      {
      if (m_reachAnnualFilePtrArray[i] != NULL)
         {
         fclose(m_reachAnnualFilePtrArray[i]);
         m_reachAnnualFilePtrArray[i] = NULL;
         }
      }

   for (int i = 0; i < (int)m_reachDailyFilePtrArray.GetSize(); i++)
      {
      if (m_reachDailyFilePtrArray[i] != NULL)
         {
         fclose(m_reachDailyFilePtrArray[i]);
         m_reachDailyFilePtrArray[i] = NULL;
         }
      }
   }


int FlowModel::SaveState()
   {
   int hruCount = GetHRUCount();
   int hruPoolCount = GetHRUPoolCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int)m_reservoirArray.GetSize();
   int modelStateVarCount = ((hruPoolCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;

   // does file exist?
   CString tmpPath;

   tmpPath = PathManager::GetPath(PM_IDU_DIR);//directory with the idu

   FILE *fp;
   CString filename;
   filename.Format(_T("%s%s"), (LPCTSTR)tmpPath, (LPCTSTR)m_initConditionsFileName);
   const char *file = filename;

   errno_t err;
   if ((err = fopen_s(&fp, file, _T("wb"))) != 0)
      {
      CString msg;
      msg.Format(_T("Flow: Unable to open file %s.  Initial conditions won't be saved. Error %i "), (LPCTSTR)tmpPath + m_initConditionsFileName, (int)err);
      Report::LogWarning(msg);
      return -1;
      }

   fwrite(&modelStateVarCount, sizeof(int), 1, fp);
   //fprintf(fp, "%i ", modelStateVarCount);//m3
   float zero = 0;
   for (int i = 0; i < hruCount; i++)
      {
      HRU *pHRU = m_hruArray[i];
      for (int j = 0; j < hruPoolCount; j++)
         {
         HRUPool *pLayer = pHRU->GetPool(j);
         if (pLayer->m_volumeWater >= 0.0f)
            fwrite(&pLayer->m_volumeWater, sizeof(double), 1, fp);
         else
            {
            CString msg;
            msg.Format("Flow: Bad value (%f) in HRU %i layer %i.  Initial conditions won't be saved.  ", pLayer->m_volumeWater, i, j);
            Report::LogWarning(msg);
            fwrite(&zero, sizeof(double), 1, fp);
            // return -1;
            }
         //fprintf(fp, "%f ", pLayer->m_volumeWater);//m3
         for (int k = 0; k < m_hruSvCount - 1; k++)
            fwrite(&pLayer->m_svArray[k], sizeof(double), 1, fp);
         // fprintf(fp, "%f ", pLayer->m_svArray[k]);//kg
         }
      }

   for (int i = 0; i < reachCount; i++)
      {
      Reach *pReach = m_reachArray[i];

      for (int k = 0; k < pReach->m_subnodeArray.GetSize(); k++)
         {
         ReachSubnode *pNode = pReach->GetReachSubnode(k);
         //fprintf(fp, "%f ", pNode->m_discharge);//m3
         if (pNode->m_discharge >= -10.0f)
            fwrite(&pNode->m_discharge, sizeof(float), 1, fp);
         else
            {
            CString msg;
            msg.Format("Flow: Bad value (%f) in Reach %i .  Initial conditions won't be saved.  ", pNode->m_discharge, i);
            Report::LogWarning(msg);
            return -1;
            }
         for (int l = 0; l < m_hruSvCount - 1; l++)
            fwrite(&pNode->m_svArray[l], sizeof(double), 1, fp);
         //fprintf(fp, "%f ", pNode->m_svArray[l]);//kg
         }
      }
   CString msg1;
   msg1.Format("Flow: Initial Conditions file %s successfully written.  ", (LPCTSTR)filename);
   Report::LogWarning(msg1);

   fclose(fp);
   m_saveStateAtEndOfRun = false;
   return 1;
   }

int FlowModel::ReadState()
   {
   int hruCount = GetHRUCount();
   int hruPoolCount = GetHRUPoolCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int)m_reservoirArray.GetSize();

   int modelStateVarCount = ((hruPoolCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;
   // does file exist?
   CString tmpPath = PathManager::GetPath(PM_IDU_DIR);//directory with the idu
   int fileStateVarCount = 0;

   if (PathManager::FindPath(m_initConditionsFileName, tmpPath) < 0) //the file cannot be found, so skip the read and set flag to write file at the end of the run
      {
      CString msg;
      msg.Format(_T("Flow: Initial Conditions file '%s' can not be found.  It will be created at the end of this run"), (LPCTSTR)m_initConditionsFileName);
      Report::LogWarning(msg);
      m_saveStateAtEndOfRun = true;
      return 1;
      }
   else //the file can be found. First check that it has the same # of state variables as the current model.  If not, the model changed, so don't read the file now
      {
      FILE *fp = NULL;
      const char *file = tmpPath;
      errno_t err;
      if ((err = fopen_s(&fp, file, _T("rb"))) != 0)
         {
         CString msg;
         msg.Format(_T("Unable to open file %s.  Initial conditions won't be saved.  "), tmpPath);
         Report::LogWarning(msg);
         return -1;
         }

      fread(&fileStateVarCount, sizeof(int), 1, fp);

      if (fileStateVarCount != modelStateVarCount)//the file is out of date, so skip the read and set flage to write file at the end of the run
         {
         CString msg;
         msg.Format(_T("The current model is different than that saved in %s. %i versus %i state variables.  %s will be overwritten at the end of this run"), (LPCTSTR)m_initConditionsFileName, modelStateVarCount, fileStateVarCount, (LPCTSTR)m_initConditionsFileName);
         Report::LogWarning(msg);
         m_saveStateAtEndOfRun = true;

         }
      else //read the file to populate initial conditions
         {
         CString msg1;
         msg1.Format(_T("Flow: Initial Conditions file %s successfully read.  "), (LPCTSTR)m_initConditionsFileName);
         Report::Log(msg1);
         float _val = 0;
         for (int i = 0; i < hruCount; i++)
            {
            HRU *pHRU = m_hruArray[i];
            for (int j = 0; j < hruPoolCount; j++)
               {
               HRUPool *pLayer = pHRU->GetPool(j);
               //fscanf(fp, _T("%f "), &pLayer->m_volumeWater);
               fread(&pLayer->m_volumeWater, sizeof(double), 1, fp);
               if (pLayer->m_volumeWater < 0.0f)
                  pLayer->m_volumeWater = 0.0f;
               for (int k = 0; k < m_hruSvCount - 1; k++)
                  {
                  fread(&pLayer->m_svArray[k], sizeof(double), 1, fp);
                  if (pLayer->m_volumeWater < 0.0f)
                     pLayer->m_volumeWater = 0.0f;
                  }
               //fscanf(fp, _T("%f "), &pLayer->m_svArray[k]);
               }
            }
         for (int i = 0; i < reachCount; i++)
            {
            Reach *pReach = m_reachArray[i];
            for (int j = 0; j < pReach->m_subnodeArray.GetSize(); j++)
               {
               ReachSubnode *pNode = pReach->GetReachSubnode(j);
               fread(&pNode->m_discharge, sizeof(float), 1, fp);
               //fscanf(fp, _T("%f "), &pNode->m_discharge);
               for (int k = 0; k < m_hruSvCount - 1; k++)
                  fread(&pNode->m_svArray[k], sizeof(double), 1, fp);
               //fscanf(fp, _T("%f "), &pNode->m_svArray[k]);
               }
            }
         }//end of else read the file to populate initial conditions
      fclose(fp);
      }//else file can be found
   return 1;
   }


void FlowModel::ResetDataStorageArrays(EnvContext *pEnvContext)
   {
   // if model outputs are in use, set up the data object
   for (int i = 0; i < (int)m_modelOutputGroupArray.GetSize(); i++)
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[i];

      if (pGroup->m_pDataObj != NULL)
         {
         pGroup->m_pDataObj->ClearRows();
         //  pGroup->m_pDataObj->Clear();
         pGroup->m_pDataObj->SetSize(pGroup->m_pDataObj->GetColCount(), 0);
         }
      }
   //Insert code here for m_reservoirDataArray;
   }


// FlowModel::Run() runs the model for one year
bool FlowModel::Run(EnvContext *pEnvContext)
   {
   CString msgStart;
   msgStart.Format(_T("Flow: Starting year %i"), pEnvContext->yearOfRun);
   Report::Log(msgStart);
   MSG  msga;
   while (PeekMessage(&msga, NULL, NULL, NULL, PM_REMOVE))
      {
      TranslateMessage(&msga);
      DispatchMessage(&msga);
      }

   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;
   m_flowContext.pEnvContext = pEnvContext;
   m_flowContext.timing = GMT_START_YEAR;

   if (pEnvContext->yearOfRun == 0 /*&& !m_estimateParameters*/)
      {
      ResetStateVariables();
      int reachCount = (int)m_reachArray.GetSize();
      for (int i = 0; i < reachCount; i++)
         {
         Reach *pReach = m_reachArray[i];
         ASSERT(pReach != NULL);

         for (int j = 0; j < pReach->GetSubnodeCount(); j++)
            {
            ReachSubnode *pSubnode = pReach->GetReachSubnode(j);
            ASSERT(pSubnode != NULL);

            //initialize the subnode volumes using mannings equation
            float Q = pSubnode->m_discharge;
            SetGeometry(pReach, Q);
            pSubnode->m_volume = pReach->m_width*pReach->m_depth*pReach->m_length / pReach->m_subnodeArray.GetSize();
            }
         }
      }

   m_projectionWKT = pEnvContext->pMapLayer->m_projection; //the Well Known Text for the maplayer projection
   CString msg;

   clock_t start = clock();

   FlowScenario *pScenario = m_scenarioArray.GetAt(m_currentFlowScenarioIndex);
   if (!pScenario->m_useStationData)
      {
      LoadClimateData(pEnvContext, pEnvContext->currentYear);
      }

   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;
   m_loadClimateRunTime += (float)duration;

   //ASSERT(CheckHRUPoolValues() == true);

   StartYear(&m_flowContext);     // calls GLobalMethods::StartYear(), initialize annual variable, accumulators

   // convert envision time to days
   m_currentTime = m_flowContext.time = pEnvContext->currentYear*365.0f;
   m_flowContext.svCount = m_hruSvCount - 1;  //???? 
   m_timeInRun = pEnvContext->yearOfRun * 365.0;   // days

   m_stopTime = m_currentTime + 365;      // always run for 1 year total to sync with Envision
   float stepSize = m_flowContext.timeStep = (float)m_timeStep;

   m_flowContext.Reset();

   m_year = pEnvContext->currentYear;

   ResetFluxValuesForStep(pEnvContext);

   omp_set_num_threads(m_processorsUsed);

   m_volumeMaxSWE = 0.0f;
   m_dateMaxSWE = 0;
   ResetCumulativeYearlyValues();

   //ASSERT(CheckHRUPoolValues() == true);


   //-------------------------------------------------------
   // Main within-year FLOW simulation loop starts here
   //-------------------------------------------------------
  // m_stopTime = (m_currentTime + TIME_TOLERANCE);
   while ((m_currentTime + TIME_TOLERANCE) < m_stopTime)
      {
      int dayOfYear = int(fmod(m_timeInRun, 365));  // zero based day of year
      int _year = 0;
      BOOL ok = ::GetCalDate(dayOfYear + 1, &_year, &m_month, &m_day, TRUE);

      m_flowContext.dayOfYear = dayOfYear;
      m_flowContext.timing = GMT_START_STEP;

      double timeOfDay = m_currentTime - (int)m_currentTime;

      if (!m_estimateParameters)
         msg.Format(_T("Flow: Time: %6.1f - Simulating HRUs..."), m_timeInRun);
      else
         msg.Format(_T("Flow: Run %i of %i: Time: %6.1f of %i days "), pEnvContext->run + 1, m_numberOfRuns, m_timeInRun, m_numberOfYears * 365);

      Report::StatusMsg(msg);

      msg.Format(_T("%s %i, %i"), ::GetMonthStr(m_month), m_day, this->GetCurrentYear());
      ::EnvSetLLMapText(msg);

      // reset accumlator rates
      m_totalWaterInputRate = 0;
      m_totalWaterOutputRate = 0;

      if (m_pGlobalFlowData)
         {
         start = clock();

         SetGlobalFlows();

         finish = clock();
         duration = (float)(finish - start) / CLOCKS_PER_SEC;
         m_globalFlowsRunTime += (float)duration;
         }

      // any pre-run external methods?
      start = clock();

      StartStep(&m_flowContext);

      ResetFluxValuesForStep(pEnvContext);  // sets all global flux values to zero

      //ASSERT(CheckHRUPoolValues() == true);

      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;
      m_externalMethodsRunTime += (float)duration;

      //start = clock();
      //finish = clock();
      //duration = (float)(finish - start) / CLOCKS_PER_SEC;
      //m_gwRunTime += (float)duration;

      //-----------------------------------------------------
      // Establish inflows and outflows for all Reservoirs.  
      //-----------------------------------------------------
      SetGlobalReservoirFluxes();     // (NOTE: MAY NEED TO MOVE THIS TO GetCatchmentDerivatives()!!!!

      //-----------------------------------------------------
      // Integrate Catchment/HRUs
      //-----------------------------------------------------

      start = clock();

      // Note: GetCatchmentDerivates() calls GlobalMethod::Run()'s for any method with  (m_timing&GMT_CATCHMENT) != 0 
      m_flowContext.timing = GMT_CATCHMENT;
      m_hruBlock.Integrate((double)m_currentTime, (double)m_currentTime + stepSize, GetCatchmentDerivatives, &m_flowContext);  // update HRU swc and associated state variables

      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;
      m_hruIntegrationRunTime += (float)duration;

      m_flowContext.Reset();

      //-----------------------------------------------------
      // Integrate Reservoirs
      //-----------------------------------------------------
      start = clock();

      // Note: GetReservoirDerivatives() calls GlobalMethod::Run()'s for any method with 
      // (m_timing&GMT_RESERVOIR) != 0 
      if (m_reservoirFluxMethod != RES_OPTIMIZATION)//if it is, Reservoirs are handled elsewhere
      {
         m_flowContext.timing = GMT_RESERVOIR;
         m_reservoirBlock.Integrate(m_currentTime, m_currentTime + stepSize, GetReservoirDerivatives, &m_flowContext);     // update stream reaches

         finish = clock();
         duration = (float)(finish - start) / CLOCKS_PER_SEC;
         m_reservoirIntegrationRunTime += (float)duration;

         // check reservoirs for filling
         for (int i = 0; i < (int)m_reservoirArray.GetSize(); i++)
            {
            Reservoir *pRes = m_reservoirArray[i];
            //CString msg;
            //msg.Format( _T("Reservoir check: Filled=%i, elevation=%f, dam top=%f"), pRes->m_filled, pRes->m_elevation, pRes->m_dam_top_elev );
            //Report::Log( msg );
            if (pRes->m_filled == 0 && pRes->m_elevation >= (pRes->m_dam_top_elev - 2.0f))
               {
               //Report::Log( _T("Reservoir filled!!!"));
               pRes->m_filled = 1;
               }

            }
         }
      m_flowContext.Reset();

      //-----------------------------------------------------
      // Integrate Reaches
      //-----------------------------------------------------
      start = clock();
      m_flowContext.timing = GMT_REACH;
      GlobalMethodManager::SetTimeStep(stepSize);
      GlobalMethodManager::Step(&m_flowContext);
      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;
      m_reachIntegrationRunTime += (float)duration;

      //-----------------------------------------------------
      // Check mass balance if needed
      //-----------------------------------------------------
      // total refers to mass balance error ceheck
      if (m_checkMassBalance)
         {
         start = clock();

         m_totalBlock.Integrate(m_currentTime, m_currentTime + stepSize, GetTotalDerivatives, &m_flowContext);

         finish = clock();
         duration = (float)(finish - start) / CLOCKS_PER_SEC;
         m_massBalanceIntegrationRunTime += (float)duration;
         }

      //-----------------------------------------------------
      // Generate output
      //-----------------------------------------------------
      start = clock();

      if (m_detailedOutputFlags & 1)  // daily IDU
         {
         if (!m_pGrid)
            SaveDetailedOutputIDU(m_iduDailyFilePtrArray);
         else
            SaveDetailedOutputGrid(m_iduDailyFilePtrArray);
         }
      if (m_detailedOutputFlags & 4)  // daily Reach
         if ((fmod((double)m_currentTime, m_detailedSaveInterval) < 0.001f))//only save tracers every 10 days
            SaveDetailedOutputReach(m_reachDailyFilePtrArray);

      if (m_detailedOutputFlags & 16)  // daily Grid Tracer IDU
         {
         if ((m_pGrid && pEnvContext->currentYear - pEnvContext->startYear >= m_detailedSaveAfterYears &&  (fmod((double)m_currentTime, m_detailedSaveInterval) < 0.01f)))//only save tracers every 10 days
            SaveDetailedOutputGridTracer(m_iduDailyTracerFilePtrArray);
         }


      ResetFluxValuesForStep(pEnvContext);
      CollectData(dayOfYear);

      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;
      m_collectDataRunTime += (float)duration;

      if (!m_estimateParameters)
         {
         if (dayOfYear < 180)//get maximum winter snowpack for this year.  
            GetMaxSnowPack(pEnvContext);
         }

      EndStep(&m_flowContext);
      UpdateHRULevelVariables(pEnvContext);

      // any post-run external methods?
      m_flowContext.timing = GMT_END_STEP;
      GlobalMethodManager::EndStep(&m_flowContext);   // this invokes EndStep() for any internal global methods 

      CollectModelOutput();

      // update time...
      m_currentTime += stepSize;
      m_timeInRun += stepSize;

      // are we close to the end?
      double timeRemaining = m_stopTime - m_currentTime;

      if (timeRemaining < (m_timeStep + TIME_TOLERANCE))
         stepSize = float(m_stopTime - m_currentTime);

      if (m_mapUpdate == 2 && !m_estimateParameters)
         WriteDataToMap(pEnvContext);

      if (m_mapUpdate == 2 && !m_estimateParameters)
         ::EnvRedrawMap(); //RedrawMap(pEnvContext);

      if (m_useRecorder && !m_estimateParameters)
         {
         for (int i = 0; i < (int)m_vrIDArray.GetSize(); i++)
            ::EnvCaptureVideo(m_vrIDArray[i]);
         }

      if (dayOfYear == 90 && !m_estimateParameters)
         UpdateAprilDeltas(pEnvContext);

      if (dayOfYear == 274)
         ResetCumulativeWaterYearValues();
      }  // end of: while ( m_time < m_stopTime )   // note - doesn't guarantee alignment with stop time

   // Done with the internal flow timestep loop, finish up...
   EndYear(&m_flowContext);

   if (m_mapUpdate == 1 && !m_estimateParameters)
      {
      WriteDataToMap(pEnvContext);
      ::EnvRedrawMap();   // RedrawMap(pEnvContext);
      }

   CloseClimateData();

   if (!m_estimateParameters)
      {
      UpdateYearlyDeltas(pEnvContext);
      ResetCumulativeYearlyValues();
      }

   if (m_detailedOutputFlags & 2)
      SaveDetailedOutputIDU(m_iduAnnualFilePtrArray);

   if (m_detailedOutputFlags & 8)
      SaveDetailedOutputReach(m_reachAnnualFilePtrArray);



   if (!m_estimateParameters)
      {
      for (int i = 0; i < m_reservoirArray.GetSize(); i++)
         {
         Scenario *pScenario = EnvGetScenario(pEnvContext->pEnvModel,pEnvContext->scenarioIndex);

         Reservoir *pRes = m_reservoirArray[ i ];
         CString outputPath;
         outputPath.Format(_T("%s%sAppliedConstraints_%s_Run%i.csv"), PathManager::GetPath(PM_OUTPUT_DIR), (LPCTSTR) pRes->m_name, (LPCTSTR) pScenario->m_name, pEnvContext->run);
         pRes->m_pResSimRuleCompare->WriteAscii(outputPath);         
         }

      float channel = 0.0f;
      float terrestrial = 0.0f;
      GetTotalStorage(channel, terrestrial);
      msg.Format(_T("Flow: TotalInput: %6.1f 10^6m3. TotalOutput: %6.1f 10^6m3. Terrestrial Storage: %6.1f 10^6m3. Channel Storage: %6.1f 10^6m3  Difference: %6.1f m3"), float(m_totalWaterInput / 1E6), float(m_totalWaterOutput / 1E6), float(terrestrial / 1E6), float(channel / 1E6), float(m_totalWaterInput - m_totalWaterOutput - (channel + terrestrial)));
      Report::Log(msg);
      }

   return TRUE;
   }


bool FlowModel::StartYear(FlowContext *pFlowContext)
   {
   m_annualTotalET = 0; // acre-ft
   m_annualTotalPrecip = 0; // acre-ft
   m_annualTotalDischarge = 0; //acre-ft
   m_annualTotalRainfall = 0; // acre-ft
   m_annualTotalSnowfall = 0; // acre-ft

   GlobalMethodManager::StartYear(&m_flowContext);   // this invokes StartYear() for any internal global methods 

   FlowScenario *pScenario = m_scenarioArray.GetAt(m_currentFlowScenarioIndex);

   // reset reservoirs for this year
   for (int i = 0; i < (int)m_reservoirArray.GetSize(); i++)
      {
      Reservoir *pRes = m_reservoirArray[i];
      pRes->m_filled = 0;

      if (pRes->m_probMaintenance > -1)//the reservoir may be, or have been, taken offline
         {
         if (pRes->m_reservoirType == ResType_RiverRun)//it was taken off line last year
            pRes->m_reservoirType = ResType_FloodControl;//reset it back to FloodControl
         float value = (float)m_randUnif1.RandValue(0, 1);
         if (value < pRes->m_probMaintenance)
            {
            pRes->m_reservoirType = ResType_RiverRun;
            CString msg;
            msg.Format(_T("Flow: Reservoir %s will be offline for the next year. Offline Probability %f, rolled %f"), (LPCTSTR)pRes->m_name, pRes->m_probMaintenance, value);
            Report::Log(msg);
            }
         else
            {
            CString msg;
            msg.Format(_T("Flow: Reservoir %s will be ONline for the next year. Offline Probability %f, rolled %f"), (LPCTSTR)pRes->m_name, pRes->m_probMaintenance, value);
            Report::Log(msg);
            }
         }
      pRes->m_avgPoolVolJunAug = 0;
      pRes->m_avgPoolElevJunAug = 0;
      }

    // fix up any water transfers between irrigated and non-irrigated soil layers
   CUIntArray irrigArray;
   CUIntArray nonIrrigArray;
   for ( int i=0; i < this->m_poolInfoArray.GetSize(); i++ )
      {
      if ( this->m_poolInfoArray[ i ]->m_type == PT_SOIL )
         nonIrrigArray.Add( i );
      else if (this->m_poolInfoArray[i]->m_type == PT_IRRIGATED )
         irrigArray.Add( i );
      }

   if ( irrigArray.GetSize() > 0 && nonIrrigArray.GetSize() > 0 )
      {
      // okay, we know what pools are irrigated an non-irrigated.  Allocate between them

      // fix up water transfers between irrigated and unirrigated polygons
      int hruCount = GetHRUCount();
      for (int h = 0; h < hruCount; h++)
         {
         HRU *pHRU = GetHRU(h);
         double targetIrrigatedArea = 0.0;
         // get HRU area that is in irrigation
         for (int m = 0; m < pHRU->m_polyIndexArray.GetSize(); m++)
            {
            double area = 0.0;
            int irrigated = 0;
            m_pCatchmentLayer->GetData(pHRU->m_polyIndexArray[m], m_colIrrigation, irrigated);
            m_pCatchmentLayer->GetData(pHRU->m_polyIndexArray[m], m_colCatchmentArea, area);
            if (irrigated != 0)
               targetIrrigatedArea += area;
            }

         float targetNonIrrArea = (float)(pHRU->m_area - targetIrrigatedArea);
         float irrigatedVolume = 0;
         for ( int i=0; i < irrigArray.GetSize(); i++ )
            irrigatedVolume += (float) pHRU->GetPool( irrigArray[i] )->m_volumeWater;
         float nonIrrigatedVolume = 0;
         for (int i = 0; i < nonIrrigArray.GetSize(); i++)
            nonIrrigatedVolume += (float) pHRU->GetPool( nonIrrigArray[i] )->m_volumeWater;
         // old...
         //float nonIrrigatedVolume = (float)pHRU->GetPool(2)->m_volumeWater;
         //float irrigatedVolume = (float)pHRU->GetPool(3)->m_volumeWater;

         // Reapportion water between irrigated and non-irrigated parts of the HRU at the beginning of each year, 
         // to reflect this year's decisions about which IDUs to irrigate (the output of the irrigation decision model).            if (pFlowContext->pEnvContext->yearOfRun <= 0)
         float totWater = irrigatedVolume + nonIrrigatedVolume;

         // if more than one, reapportion between pools of the same type in proportion to their depths?
         // NOT YET IMPLEMENTED, assume one and one
         ASSERT( irrigArray.GetSize() == 1 && nonIrrigArray.GetSize() == 1 );
         pHRU->GetPool( nonIrrigArray[0] )->m_volumeWater = (targetNonIrrArea / pHRU->m_area) * totWater;
         pHRU->GetPool( irrigArray[0] )->m_volumeWater = (targetIrrigatedArea / pHRU->m_area) * totWater;
         pHRU->m_areaIrrigated = (float)targetIrrigatedArea;

         ASSERT( pHRU->GetPool( nonIrrigArray[0])->m_volumeWater >= 0 );
         ASSERT( pHRU->GetPool( irrigArray[0])->m_volumeWater >= 0);
         } // end of loop thru HRUs
      }
   return true;
   }


bool FlowModel::StartStep(FlowContext *pFlowContext)
   {
   int hruCount = GetHRUCount();

   for (int h = 0; h < hruCount; h++)
      {
      HRU *pHRU = GetHRU(h);
      pHRU->m_currentMaxET = 0;
      pHRU->m_currentET = 0;
      pHRU->m_currentRunoff = 0;
      pHRU->m_currentGWFlowOut = 0;
      pHRU->m_currentGWRecharge = 0;
      }

   GlobalMethodManager::StartStep(&m_flowContext);  // Calls any global methods with ( m_timing & GMT_START_STEP ) == true

   return true;
   }


bool FlowModel::EndStep(FlowContext *pFlowContext)
   {
   // get discharge at pour points
   for (int i = 0; i < this->m_reachTree.GetRootCount(); i++)
      {
      ReachNode *pRootNode = this->m_reachTree.GetRootNode(i);
      Reach *pReach = GetReachFromNode(pRootNode);
      float discharge = pReach->GetDischarge();   // m3/sec

      // convert to acreft/day
      discharge *= ACREFT_PER_M3 * SEC_PER_DAY;  // convert to acreft-day
      m_annualTotalDischarge += discharge;
      }

   return true;
   }


bool FlowModel::EndYear(FlowContext *pFlowContext)
   {
   m_annualTotalET = 0.0f;
   m_annualTotalPrecip = 0.0f;
   m_annualTotalRainfall = 0.0f;
   m_annualTotalSnowfall = 0.0f;

   // HRU's
   for (int i = 0; i < (int)m_hruArray.GetSize(); i++)
      {
      HRU *pHRU = m_hruArray[i];

//      ASSERT(pHRU->m_et_yr >= 0.0f && pHRU->m_et_yr <= 1.0E10f);
      m_annualTotalET += pHRU->m_et_yr       * pHRU->m_area * M_PER_MM * ACREFT_PER_M3;        // mm/year * area(m2) * m/mm * acreft/m3   = acre-ft
      m_annualTotalPrecip += pHRU->m_precip_yr   * pHRU->m_area * M_PER_MM * ACREFT_PER_M3;        // mm/year * area(m2) * m/mm * acreft/m3   = acre-ft
      m_annualTotalRainfall += pHRU->m_rainfall_yr * pHRU->m_area * M_PER_MM * ACREFT_PER_M3;        // mm/year * area(m2) * m/mm * acreft/m3   = acre-ft
      m_annualTotalSnowfall += pHRU->m_snowfall_yr * pHRU->m_area * M_PER_MM * ACREFT_PER_M3;        // mm/year * area(m2) * m/mm * acreft/m3   = acre-ft
      }

   // Reservoirs
   for (int i = 0; i < (int) this->m_reservoirArray.GetSize(); i++)
      {
      Reservoir *pRes = m_reservoirArray.GetAt(i);
      pRes->m_avgPoolVolJunAug  /= (AUG_31 - JUN_1 + 1);    // days between Jun 1, Aug 31
      pRes->m_avgPoolElevJunAug /= (AUG_31 - JUN_1 + 1);    // days between Jun 1, Aug 31
      }

   m_flowContext.timing = GMT_END_YEAR;
   GlobalMethodManager::EndYear(&m_flowContext);

   float row[7];
   row[0] = (float)pFlowContext->pEnvContext->currentYear;
   row[1] = m_annualTotalET;
   row[2] = m_annualTotalPrecip;
   row[3] = m_annualTotalDischarge;
   row[4] = m_annualTotalPrecip - (m_annualTotalET + m_annualTotalDischarge);
   row[5] = m_annualTotalRainfall;
   row[6] = m_annualTotalSnowfall;

   m_pTotalFluxData->AppendRow(row, 7);


   return true;
   }


// If estimating parameters, then we need to calculate some error, save the current
// parameters and resample a new set of parameters for the next model run.
// 
// When the model is finished for the last year of the current simulation, evaluate that model run over its entire period.
// if we are not in the last year, wait till the next Envision timestep
void FlowModel::UpdateMonteCarloOutput(EnvContext *pEnvContext, int runNumber)
   {
   //Get the number of locations with measured values
   int numMeas = 0;
   for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
   {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

      for (int i = 0; i < (int)pGroup->GetSize(); i++)
      {
         if (pGroup->GetAt(i)->m_inUse)
         {
            ModelOutput *pOutput = pGroup->GetAt(i);
            if (pOutput->m_pDataObjObs != NULL && pOutput->m_queryStr)
               numMeas++;
         }
      }
   }
   //Get the objective function for each of the locations with measured values
   float *ns_ = new float[numMeas];
   int c = 0;
   float ns = -1.0f; float nsLN = -1.0f; float ve = 0.0f;
   for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
   {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

      for (int i = 0; i < (int)pGroup->GetSize(); i++)
      {
         if (pGroup->GetAt(i)->m_inUse)
         {
            ModelOutput *pOutput = pGroup->GetAt(i);
            if (pOutput->m_pDataObjObs != NULL && pGroup->m_pDataObj != NULL)
            {
               ns_[c] = GetObjectiveFunction(pGroup->m_pDataObj, ns, nsLN, ve);//this will include time meas model
               c++;
            }
         }
      }
   }
   //Get the highest value for the objective function
   float maxNS = -999999.0f;
   for (int i = 0; i < numMeas; i++)
      if (ns_[i] > maxNS)
         maxNS = ns_[i];
   delete[] ns_;
   //Add objective functions and time series data to the dataobjects
   int count = 0;
   for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
   {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

      for (int i = 0; i < (int)pGroup->GetSize(); i++)
      {
         if (pGroup->GetAt(i)->m_inUse)
         {
            ModelOutput *pOutput = pGroup->GetAt(i);
            if (pOutput->m_pDataObjObs != NULL && pGroup->m_pDataObj != NULL)
            {
               FDataObj *pObj = m_mcOutputTables.GetAt(count);//objective
               count++;
               FDataObj *pData = m_mcOutputTables.GetAt(count);//timeseries
               count++;

               if (pEnvContext->run == 0)//add the measured data to the mc measured file
               {
                  pData->SetSize(2, pGroup->m_pDataObj->GetRowCount());
                  for (int i = 0; i < pData->GetRowCount(); i++)
                  {
                     pData->Set(0, i, i);//time
                     pData->Set(1, i, pGroup->m_pDataObj->Get(2, i));//measured value
                  }
               }
               if (maxNS > m_nsThreshold)
               {
                  pData->AppendCol(_T("q"));
                  for (int i = 0; i < pData->GetRowCount(); i++)
                     pData->Set(pData->GetColCount() - 1, i, pGroup->m_pDataObj->Get(1, i));
                  float ns = -1.0f; float nsLN = -1.0f; float ve = -1.0f;
                  float nsg = -1.0f; float nsLNg = -1.0f; float veg = -1.0f;
                  GetObjectiveFunction(pGroup->m_pDataObj, ns, nsLN, ve);//this will include time meas model
                  GetObjectiveFunctionGrouped(pGroup->m_pDataObj, nsg, nsLNg, veg);//this will include time meas model
                  float *nsArray = new float[7];
                  nsArray[0] = (float)pEnvContext->run;
                  nsArray[1] = ns;
                  nsArray[2] = nsLN;
                  nsArray[3] = ve;
                  nsArray[4] = nsg;
                  nsArray[5] = nsLNg;
                  nsArray[6] = veg;
                  pObj->AppendRow(nsArray, 7);
                  delete[] nsArray;
               }
            }
         }
      }
   }
   //if the simulation wasn't behavioral for any of the measurement points, then remove that parameter set from the parameter array
   if (maxNS < m_nsThreshold)
      m_pParameterData->DeleteRow(m_pParameterData->GetRowCount() - 1);
   CString path = PathManager::GetPath(3);
   // write the results to the disk
   if (fmod((double)pEnvContext->run, m_saveResultsEvery) < 0.01f)
   {
      count = 0;
      for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
      {
         ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

         for (int i = 0; i < (int)pGroup->GetSize(); i++)
         {
            if (pGroup->GetAt(i)->m_inUse)
            {
               ModelOutput *pOutput = pGroup->GetAt(i);
               if (pOutput->m_pDataObjObs != NULL && pGroup->m_pDataObj != NULL)
               {
                  CString fileOut;

                  if (m_rnSeed >= 0)
                     fileOut.Format(_T("%soutput\\%sObj_%i.csv"), (LPCTSTR)path, pGroup->m_name, (int)m_rnSeed);
                  else
                     fileOut.Format(_T("%soutput\\%sObj.csv"), (LPCTSTR)path, (LPCTSTR)pGroup->m_name);
                  m_mcOutputTables.GetAt(count)->WriteAscii(fileOut);

                  count++;

                  if (m_rnSeed >= 0)
                     fileOut.Format(_T("%soutput\\%sObs_%i.csv"), (LPCTSTR)path, (LPCTSTR)pGroup->m_name, (int)m_rnSeed);
                  else
                     fileOut.Format(_T("%soutput\\%sObs.csv"), (LPCTSTR)path, (LPCTSTR)pGroup->m_name);

                  m_mcOutputTables.GetAt(count)->WriteAscii(fileOut);
                  count++;

                  if (m_rnSeed >= 0)
                     fileOut.Format(_T("%soutput\\%sparam_%i.csv"), (LPCTSTR)path, pGroup->m_name, (int)m_rnSeed);
                  else
                     fileOut.Format(_T("%soutput\\%sparam.csv"), (LPCTSTR)path, pGroup->m_name);

                  m_pParameterData->WriteAscii(fileOut);

               }
               if (pEnvContext->run == 0)//for the first run, look for a climate data object and write it to a file.  
               {
                  CString fileOut; CString name;
                  if (pGroup->m_pDataObj != NULL && pGroup->m_name == _T("Climate"))
                  {
                     for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)//search the output array for a group that has a descriptive (place) m_name
                     {
                        ModelOutputGroup *pGroupName = m_modelOutputGroupArray[j];
                        for (int i = 0; i < (int)pGroupName->GetSize(); i++)
                        {
                           if (pGroupName->GetAt(i)->m_inUse)
                           {
                              ModelOutput *pOutput = pGroupName->GetAt(i);
                              if (pOutput->m_pDataObjObs != NULL && pGroupName->m_pDataObj != NULL)
                                 name = pGroupName->m_name;
                              continue;
                           }
                        }
                     }
                     fileOut.Format(_T("%soutput\\%sClimate.csv"), (LPCTSTR)path, name);
                     pGroup->m_pDataObj->WriteAscii(fileOut);
                  }
               }
            }
         }
      }
   }

   CString msg;
   msg.Format(_T("Flow: Results for Runs 1 to %i written to disk.  Check directory %soutput"), pEnvContext->run + 1, (LPCTSTR)path);
   Report::Log(msg);
   }


void FlowModel::UpdateMonteCarloInput(EnvContext *pEnvContext, int runNumber)
   {
   //update storage arrays (parameters, error  and also (perhaps...)modeled discharge)
   int paramCount = (int)m_parameterArray.GetSize();
   float *paramValueData = new float[paramCount + 1];

   paramValueData[0] = (float)runNumber;

   for (int i = 0; i < paramCount; i++)
      {
      ParameterValue *pParam = m_parameterArray.GetAt(i);

      float value = (float)m_randUnif1.RandValue(pParam->m_minValue, pParam->m_maxValue);

      //we have a value, and need to update the table with it.
      ParamTable *pTable = GetTable(pParam->m_table);
      int col_name = pTable->GetFieldCol(pParam->m_name);
      float oldValue = 0.0f;

      FDataObj *pTableData = (FDataObj*)pTable->GetDataObj();
      FDataObj *pInitialTableData = (FDataObj*)pTable->GetInitialDataObj();
      for (int j = 0; j < pTableData->GetRowCount(); j++)
         {
         float oldValue = pInitialTableData->Get(col_name, j);
         float newValue = oldValue*value;
         if (pParam->m_distribute)
            pTableData->Set(col_name, j, newValue);
         else
            pTableData->Set(col_name, j, value);
         }

      paramValueData[i + 1] = value;
      }

   m_pParameterData->AppendRow(paramValueData, int(m_parameterArray.GetSize()) + 1);
   delete[] paramValueData;

   if (m_runFromFile)//if we are running from a file...this is specific to UGA project
   {
      //Get the climate stations for this run.  These are coordinates for ALL the climate stations 
      int siteCount = 4;
      int runNumber = m_flowContext.pEnvContext->run;

      FDataObj* pGridLocationData = new FDataObj;
      pGridLocationData->ReadAscii("GridLocations.csv");

      //get the subset of stations to be used in this model run
      FDataObj* pRealizationData = new FDataObj;
      pRealizationData->ReadAscii("C:\\Envision\\StudyAreas\\UGA_HBV\\ClimateStationsForEachRealization.csv");

      //Have StationId, now write them into the IDU using nearest distance

      Vertex v(1, 1);

      for (int i = 0; i < m_flowContext.pEnvContext->pMapLayer->m_pPolyArray->GetSize(); i++)
      {
         Poly* pPoly = m_flowContext.pEnvContext->pMapLayer->m_pPolyArray->GetAt(i);
         float minDist = 1e10; int location = 0;
         for (int j = 0; j < siteCount; j++)
         {
            int realizRow = pRealizationData->GetAsInt(j + 1, runNumber);
            float x = pGridLocationData->Get(8, realizRow - 1);
            float y = pGridLocationData->Get(9, realizRow - 1);

            //Have x and y, now get distance to this polygon
            Vertex v = pPoly->GetCentroid();

            float dist = sqrt((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y));

            if (dist < minDist)
            {
               minDist = dist;
               location = realizRow;
               int s = 1;
            }
         }
         m_flowContext.pFlowModel->UpdateIDU(m_flowContext.pEnvContext, i, m_colStationID, location, ADD_DELTA);
      }
      delete pGridLocationData;
      delete pRealizationData;
   }

   }


//bool FlowModel::SolveReachDirect( void )
//   {
//   switch( m_reachSolutionMethod )
//      {
//      case RSM_EXTERNAL:
//         {
//         if ( m_reachExtFn != NULL )
//            m_reachExtFn( &m_flowContext );
//         return true;
//         }
//
//      case RSM_KINEMATIC:
//         return SolveReachKinematicWave();
//      }
//
//   // anything else indicates a problem
//   ASSERT( 0 );
//   return false;
//   }


bool FlowModel::SolveGWDirect(void)
   {
   //   switch( m_gwMethod )
   //      {
   //      case GW_EXTERNAL:
   //         {
   //         if ( m_gwExtFn != NULL )
   //            m_gwExtFn( &m_flowContext );
   //         return true;
   //         }
   //      }

      // anything else indicates a problem
   ASSERT(0);
   return false;
   }


//bool FlowModel::SetGlobalHruToReachExchanges( void )
//   {
//   switch( m_latExchMethod )
//      {
//      case HREX_EXTERNAL:
//         {
//         if ( m_latExchExtFn != NULL )
//            m_latExchExtFn( &m_flowContext );
//         return true;
//         }
//
//      case HREX_INFLUXHANDLER:
//         return 0.0f;
//
//      case HREX_LINEARRESERVOIR:
//         return SetGlobalHruToReachExchangesLinearRes();
//      }
//
//   // anything else indicates a problem
//   ASSERT( 0 );
//   return false;
//   }


//bool FlowModel::SetGlobalHruVertFluxes( void )
//   {
//   switch( m_hruVertFluxMethod )
//      {
//      case VD_EXTERNAL:
//         {
//         if ( m_hruVertFluxExtFn != NULL )
//            m_hruVertFluxExtFn( &m_flowContext );
//         return true;
//         }
//
//      case VD_INFLUXHANDLER:
//         return 0.0f;
//
//      case VD_BROOKSCOREY:
//         return SetGlobalHruVertFluxesBrooksCorey();
//      }
//
//   // anything else indicates a problem
//   ASSERT( 0 );
//   return false;
//   }



bool FlowModel::SetGlobalExtraSVRxn(void)
   {
   switch (m_extraSvRxnMethod)
      {
      case EXSV_EXTERNAL:
      {
      if (m_extraSvRxnExtFn != NULL)
         m_extraSvRxnExtFn(&m_flowContext);
      return true;
      }

      case EXSV_INFLUXHANDLER:
         return 0.0f;
      }

   // anything else indicates a problem
   ASSERT(0);
   return false;
   }


bool FlowModel::SetGlobalReservoirFluxes(void)
   {
   switch (m_reservoirFluxMethod)
      {
      case RES_EXTERNAL:
      {
      if (m_reservoirFluxExtFn != NULL)
         m_reservoirFluxExtFn(&m_flowContext);
      return true;
      }

      case RES_RESSIMLITE:
         return SetGlobalReservoirFluxesResSimLite();

      case RES_OPTIMIZATION:
         return SetGlobalReservoirFluxesResSimLite();
      }

   // anything else indicates a problem
   ASSERT(0);
   return false;
   }


bool FlowModel::InitReservoirControlPoints(EnvModel *pEnvModel)
   {
   int controlPointCount = (int)m_controlPointArray.GetSize();
   // Code here to step through each control point

   int colComID = m_pStreamLayer->GetFieldCol(_T("comID"));

   for (int i = 0; i < controlPointCount; i++)
      {
      ControlPoint *pControl = m_controlPointArray.GetAt(i);
      ASSERT(pControl != NULL);

      TCHAR *r = NULL;
      TCHAR delim[] = _T(",");
      TCHAR res[256];
      TCHAR *next = NULL;
      CString resString = pControl->m_reservoirsInfluenced;  //list of reservoirs influenced...from XML file
      lstrcpy(res, resString);
      r = _tcstok_s(res, delim, &next);   //Get first value

      ASSERT(r != NULL);
      int res_influenced = atoi(r);       //convert to int

      // Loop through reservoirs and point to the ones that this control point has influence over
      while (r != NULL)
         {
         for (int j = 0; j < m_reservoirArray.GetSize(); j++)
            {
            Reservoir *pRes = m_reservoirArray.GetAt(j);
            ASSERT(pRes != NULL);

            int resID = pRes->m_id;

            if (resID == res_influenced)       //Is this reservoir influenced by this control point?
               {
               pControl->m_influencedReservoirsArray.Add(pRes);    //Add reservoir to array of influenced reservoirs
               r = strtok_s(NULL, delim, &next);   //Look for delimiter, Get next value in string
               if (r == NULL)
                  break;

               res_influenced = atoi(r);
               }
            }

         /// Need code here (or somewhere, to associate a pReach with a control point)
         // iterate through reach network, looking control point ID's...  need to associate a pReach with each control point
         // Probably need a more efficient way to do this than cycling through the network for every control point
         int reachCount = m_pStreamLayer->GetRecordCount();

         for (int k = 0; k < m_reachArray.GetSize(); k++)
            {
            Reach *pReach = m_reachArray.GetAt(k);
            if (pReach->m_reachID == pControl->m_location)     // Is this location a control point?
               {
               //If so, add a pointer to the reach to the control point class
               pControl->m_pReach = pReach;
               break;
               }
            }
         }

      if (pControl->m_pReach != NULL)
         {
         ASSERT(pControl->m_pResAllocation == NULL);
         pControl->m_pResAllocation = new FDataObj(int(pControl->m_influencedReservoirsArray.GetSize()), 0, U_UNDEFINED);

         //load table for control point constraints 
         LoadTable(pEnvModel, this->m_path + pControl->m_dir + pControl->m_controlPointFileName, (DataObj**) &(pControl->m_pControlPointConstraintsTable), 0);
         }
      }

   return true;
   }


bool FlowModel::UpdateReservoirControlPoints(int doy)
   {
   int controlPointCount = (int)m_controlPointArray.GetSize();
   // Code here to step through each control point

   for (int i = 0; i < controlPointCount; i++)
      {
      ControlPoint *pControl = m_controlPointArray.GetAt(i);
      ASSERT(pControl != NULL);

      if (pControl->InUse() == false)
         continue;

      CString filename = pControl->m_controlPointFileName;

      int location = pControl->m_location;     //Reach where control point is located

      //Get constraint type (m_type).  Assume 1st characters of filename until first underscore or space contain rule type.  
      //Parse rule name and determine m_type  ( IN the case of downstream control points, only maximum and minimum)
      TCHAR name[256];
      strcpy_s(name, filename);
      TCHAR* ruletype = NULL, *next = NULL;
      TCHAR delim[] = _T(" ,_");  //allowable delimeters: , ; space ; underscore

      ruletype = strtok_s(name, delim, &next);  //Strip header.  should be cp_ for control points
      ruletype = strtok_s(NULL, delim, &next);  //Returns next string at head of file (max or min). 

      // Is this a min or max value
      if (_stricmp(ruletype, _T("Max")) == 0)
         pControl->m_type = RCT_MAX;
      else if (_stricmp(ruletype, _T("Min")) == 0)
         pControl->m_type = RCT_MIN;

      // Get constraint value
      DataObj *constraintTable = pControl->m_pControlPointConstraintsTable;
      ASSERT(constraintTable != NULL);

      Reach *pReach = pControl->m_pReach;       //get reach at location of control point  

      CString xlabel = constraintTable->GetLabel(0);

      float xvalue = 0.0f;
      float yvalue = 0.0f;
      bool isFlowDiff = 0;

      if (_stricmp(xlabel, _T("Date")) == 0)                         //Date based rule?  xvalue = current date.
         xvalue = (float)doy;
      else if (_stricmp(xlabel, _T("Pool_Elev_m")) == 0)              //Pool elevation based rule?  xvalue = pool elevation (meters)
         xvalue = 110;   //workaround 11_15.  Need to point to Fern Ridge pool elev.  only one control point used this xvalue
      else if (_stricmp(xlabel, _T("Outflow_lagged_24h")) == 0)
         {
         xvalue = pReach->GetDischarge();
         }
      else if (_stricmp(xlabel, _T("Inflow_cms")) == 0)
         {
         }//Code here for lagged and inflow based constraints
      else if (_stricmp(xlabel, _T("date_water_year_type")) == 0)         //Lookup based on two values...date and wateryeartype (storage in 13 USACE reservoirs on May 20th).
         {
         xvalue = (float)doy;
         yvalue = this->m_waterYearType;
         }
      else                                                    //Unrecognized xvalue for constraint lookup table
         {
         CString msg;
         msg.Format(_T("Flow:  Unrecognized x value for reservoir constraint lookup '%s', %s (id:%i) in stream network"), (LPCTSTR)pControl->m_controlPointFileName, (LPCTSTR)pControl->m_name, pControl->m_id);
         Report::LogWarning(msg);
         }


      // if the reach doesn't exist, ignore this
      if (pReach != NULL)
         {
         float currentdischarge = pReach->GetDischarge();    // Get current flow at reach
         /*/////////////////////////////////////////////////////////
       ///Change flows with ResSim values.  Use only for testing in conjunction with low or zero precipitation
       int flowcol = 0;
       flowcol = pControl->localFlowCol;
       float localflow = 0.0f;

       if (flowcol >= 1)
          {
          localflow = m_pCpInflows->IGet(m_currentTime, 0, flowcol, IM_LINEAR);   //local flow value from ressim
          currentdischarge =+ localflow;
          }
       /////////////////////////////////////////////////////////////////////////*/

         float constraintValue = 0.0f;
         float addedConstraint = 0.0f;

         int influenced = (int)pControl->m_influencedReservoirsArray.GetSize();     // varies by control point
         if (influenced > 0)
            {
            ASSERT(pControl->m_pResAllocation != NULL);
            ASSERT(influenced == pControl->m_pResAllocation->GetColCount());

            float *resallocation = new float[influenced]();   //array the size of all reservoirs influenced by this control point

            switch (pControl->m_type)
               {
               case RCT_MAX:    //maximum
               {
               ASSERT(pControl->m_pControlPointConstraintsTable != NULL);

               if (yvalue > 0)  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                  constraintValue = pControl->m_pControlPointConstraintsTable->IGet(xvalue, yvalue, IM_LINEAR);
               else             //If not, just use xvalue
                  constraintValue = pControl->m_pControlPointConstraintsTable->IGet(xvalue, 1, IM_LINEAR);

               //Compare to current discharge and allocate flow increases or decreases
               //Currently allocated evenly......need to update based on storage balance curves in ResSIM  mmc 11_15_2012
               if (currentdischarge > constraintValue)   //Are we above the maximum flow?   
                  {
                  for (int j = 0; j < influenced; j++)
                     resallocation[j] = (((constraintValue - currentdischarge) / influenced));// Allocate decrease in releases (should be negative) over "controlled" reservoirs if maximum, evenly for now

                  pControl->m_pResAllocation->ClearRows();     //Clear old data.  Only keep current allocation.
                  pControl->m_pResAllocation->AppendRow(resallocation, influenced);//Populate array with flows to be allocated
                  }
               else                                      //If not above maximum, set maximum value high, so no constraint will be applied
                  {
                  for (int j = 0; j < influenced; j++)
                     resallocation[j] = 0.0f;

                  pControl->m_pResAllocation->ClearRows();     //Clear old data.  Only keep current allocation.
                  pControl->m_pResAllocation->AppendRow(resallocation, influenced);   //Populate array with flows to be allocated
                  }
               }
               break;

               case RCT_MIN:  //minimum
               {
               if (yvalue > 0)  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                  addedConstraint = constraintValue = pControl->m_pControlPointConstraintsTable->IGet(xvalue, yvalue, IM_LINEAR);
               else             //If not, just use xvalue
                  addedConstraint = constraintValue = pControl->m_pControlPointConstraintsTable->IGet(xvalue, 1, IM_LINEAR);

               if (pControl->m_influencedReservoirsArray[0]->m_reservoirType == ResType_CtrlPointControl)
                  {
                  float resDischarge = pControl->m_influencedReservoirsArray[0]->m_pDownstreamReach->GetDischarge();
                  addedConstraint = resDischarge + constraintValue;
                  int releaseFreq = 1;

                  for (int j = 0; j < influenced; j++)
                     {
                     resallocation[j] = ((addedConstraint - currentdischarge) / influenced); // Allocate increase in releases (should be positive) over "controlled" reservoirs if maximum, evenly for now                     }

                     releaseFreq = pControl->m_influencedReservoirsArray[j]->m_releaseFreq;

                     ASSERT(pControl->m_pResAllocation != NULL);

                     if (resallocation[j] < 0) resallocation[j] = 0.0f;

                     if (releaseFreq == 1) pControl->m_pResAllocation->ClearRows();     //Clear old data.  Only keep current allocation.
                     pControl->m_pResAllocation->AppendRow(resallocation, influenced);   //Populate array with flows to be allocated
                     }
                  }
               else
                  {
                  if (currentdischarge < constraintValue)   //Are we below the minimum flow?   
                     {
                     for (int j = 0; j < influenced; j++)
                        resallocation[j] = ((addedConstraint - currentdischarge) / influenced);// Allocate increase in releases (should be positive) over "controlled" reservoirs if maximum, evenly for now

                     ASSERT(pControl->m_pResAllocation != NULL);
                     pControl->m_pResAllocation->ClearRows();     //Clear old data.  Only keep current allocation.
                     pControl->m_pResAllocation->AppendRow(resallocation, influenced);   //Populate array with flows to be allocated
                     }
                  else                                      //If not below minimum, set min value at 0, so no constraint will be applied
                     {
                     for (int j = 0; j < influenced; j++)
                        {
                        resallocation[j] = 0.0f;
                        }

                     pControl->m_pResAllocation->ClearRows();     //Clear old data.  Only keep current allocation.
                     pControl->m_pResAllocation->AppendRow(resallocation, influenced);   //Populate array with flows to be allocated
                     }
                  }
               }
               break;
               }  // end of:  switch( pControl->m_type )

            delete[] resallocation;
            }  // end of: if influenced > 0 )
         }  // end of: if ( pReach != NULL )
      }  // end of:  for each control point

   return true;
   }


void FlowModel::UpdateReservoirWaterYear(int dayOfYear)
   {
   int reservoirCount = (int)m_reservoirArray.GetSize();
   float resVolumeBasin = 0.0;
   for (int i = 0; i < reservoirCount; i++)
      {
      Reservoir *pRes = m_reservoirArray.GetAt(i);
      ASSERT(pRes != NULL);
      resVolumeBasin += (float)pRes->m_volume;   // This value in m3
      }

   resVolumeBasin = resVolumeBasin/(M3_PER_ACREFT*1000000.0f);   //convert volume to millions of acre-ft  (MAF)

   if (resVolumeBasin > 1.48f)                            //USACE defined "Abundant" water year in Willamette
      this->m_waterYearType = 1.48f;
   else if (resVolumeBasin < 1.48f && resVolumeBasin > 1.2f)      //USACE defined "Adequate" water year in Willamette
      this->m_waterYearType = 1.2f;
   else if (resVolumeBasin < 1.2f && resVolumeBasin > 0.9f)       //USACE defined "Insufficient"  water year in Willamette
      this->m_waterYearType = 0.9f;
   else if (resVolumeBasin < 0.9f)                         //USACE defined "Deficit" water year in Willamette
      this->m_waterYearType = 0;

   }


bool FlowModel::SetGlobalReservoirFluxesResSimLite(void)
   {
   int reservoirCount = (int)m_reservoirArray.GetSize();

   int dayOfYear = int(fmod(m_timeInRun, 365.0));  // zero based day of year
   UpdateReservoirControlPoints(dayOfYear);   //Update current status of system control points

   //A Willamette specific ruletype that checks res volume in the basin on May 20th and adjusts rules based on water year classification.
   if (dayOfYear == 140)   //Is it May 20th?  Update basinwide reservoir volume and designate water year type
      {
      UpdateReservoirWaterYear(dayOfYear);
      }

   // iterate through reservoirs, calling fluxes as needed
   for (int i = 0; i < reservoirCount; i++)
      {
      Reservoir *pRes = m_reservoirArray.GetAt(i);
      ASSERT(pRes != NULL);

      // first, inflow.  We get the inflow from any stream reaches flowing into the downstream reach this res is connected to.
      Reach *pReach = pRes->m_pDownstreamReach;

      // no associated reach?
      if (pReach == NULL)
         {
         pRes->m_inflow = pRes->m_outflow = 0;
         continue;
         }

      Reach *pLeft = (Reach*)pReach->m_pLeft;
      Reach *pRight = (Reach*)pReach->m_pRight;

      //set inflow, outflows
      float inflow = 0;

      //   To be re-implemented with remainder of FLOW hydrology - ignored currently to test ResSIMlite
      if (pLeft != NULL)
         inflow = pLeft->GetDischarge();
      if (pRight != NULL)
         inflow += pRight->GetDischarge();
      
      float outflow = 0;

      // rule curve stuff goes here
      pRes->m_inflow = inflow*SEC_PER_DAY;  //m3 per day


      outflow = pRes->GetResOutflow(this, pRes, dayOfYear);
      ASSERT(outflow >= 0);
      pRes->m_outflow = outflow*SEC_PER_DAY;    //m3 per day 
      }

   return true;
   }


bool FlowModel::SetGlobalFlows(void)
   {
   if (m_pGlobalFlowData == NULL)
      return false;

   // Basic idea:  The m_pGlobalFlowData (GeospatialDataObj) contains info about a collection
   // of points in the stream network that have flow data. A "slice" of the data object
   // contains flows for the collection of source points.
   //
   // Each point is connected to a value (default="COMID") in the reach coverage that defines
   // where in the reach network that flow applies.  (???Where is this coming from)
   //
   // Once the flow are set at the measured points, they are interpolated across the entire 
   // ReachTree containing the steam segment nodes, distributed based on contributing areas.
 /*
   CArray< int, int > m_globalFlowInputReachIndexArray;

   for ( int i=0; i < m_pGlobalFlowData->GetRowCount(); i++ )  // assumes 2 cols (COMID, FLOW)
      {
      int id;
      m_pGlobalFlowData->Get( 0, i, id );

      int reachIndex = m_pStreamLayer->FindIndex( m_colGlobalFlowInput, id );
      if ( reachIndex >= 0 )
         m_globalFlowInputReachIndexArray.Add( reachIndex );
      else
         {
         CString msg;
         msg.Format( "Flow: Reach '%s:%i referenced in flow specification file %s could not be located in the reach network.",
            (LPCTSTR) m_globalFlowReachFieldName, id, (LPCTSTR) m_globalFlowInputFile );
         Report::LogWarning( msg );

         m_globalFlowInputReachIndexArray.Add( -1 );
         }
      }

   // write data to reachtree


   // zero out flows in the Reach network
   for ( int i=0; i < (int) this->m_reachArray.GetSize(); i++ )
      {
      Reach *pReach = m_reachArray[ i ];
      int subnodeCount = pReach->GetSubnodeCount();

      for ( int j=0; j < subnodeCount; j++ )
         {
         ReachSubnode *pNode = pReach->GetReachSubnode( i );
         pNode->m_discharge = pNode->m_volume = 0.0F;
         }
      }

      */

   return true;
   }


//bool FlowModel::SetGlobalHruToReachExchangesLinearRes( void )
//   {
//   int catchmentCount = (int) m_catchmentArray.GetSize();
//   int hruPoolCount = GetHRUPoolCount();
//
//   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
//   for ( int i=0; i < catchmentCount; i++ )
//      {
//      Catchment *pCatchment = m_catchmentArray[ i ];
//      ASSERT( pCatchment != NULL );
//
//      int hruCount = pCatchment->GetHRUCount();
//      for ( int h=0; h < hruCount; h++ )
//         {
//         HRU *pHRU = pCatchment->GetHRU( h );
//         
//         int l = hruPoolCount-1;      // bottom layer only
//         HRUPool *pHRUPool = pHRU->GetPool( l );
//         
//         float depth = 1.0f; 
//         float porosity = 0.4f;
//         float voidVolume = depth*porosity*pHRU->m_area;
//         float wc = float( pHRUPool->m_volumeWater/voidVolume );
// 
//         float waterDepth = wc*depth;
//         float baseflow = GetBaseflow(waterDepth)*pHRU->m_area;//m3/d
//
//         pHRUPool->m_contributionToReach = baseflow;
//         pCatchment->m_contributionToReach += baseflow;
//         }
//      }
//
//   return true;
//   }



//bool FlowModel::SetGlobalHruVertFluxesBrooksCorey( void )
//   {
//   int catchmentCount = (int) m_catchmentArray.GetSize();
//   int hruPoolCount = GetHRUPoolCount();
//
//   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
//   for ( int i=0; i < catchmentCount; i++ )
//      {
//      Catchment *pCatchment = m_catchmentArray[ i ];
//      int hruCount = pCatchment->GetHRUCount();
//
//      for ( int h=0; h < hruCount; h++ )
//         {
//         HRU *pHRU = pCatchment->GetHRU( h );
//
//         for ( int l=0; l < hruPoolCount-1; l++ )     //All but the bottom layer
//            {
//            HRUPool *pHRUPool = pHRU->GetPool( l );
//
//            float depth = 1.0f; float porosity = 0.4f;
//            float voidVolume = depth*porosity*pHRU->m_area;
//            float wc = float( pHRUPool->m_volumeWater/voidVolume );
//
//            pHRUPool->m_verticalDrainage = GetVerticalDrainage(wc)*pHRU->m_area;//m3/d
//            }
//         }
//      }
//
//   return true;
//   }



// FlowModel::EstimateOutflow() ------------------------------------------
//
// solves the KW equations for the Reach downstream from pNode
//------------------------------------------------------------------------
/*
float FlowModel::EstimateReachOutflow( Reach *pReach, int subnode, float timeStep, float lateralInflow)
   {
   float beta  = 3.0f/5.0f;
   // compute Qin for this reach
   float dx = pReach->m_deltaX;
   double dt = timeStep* SEC_PER_DAY;
   float lateral = lateralInflow/timeStep;//m3/d
   float qsurface = lateral/dx*timeStep;  //m2

   float Qnew = 0.0f;
   float Qin = 0.0f;

   ReachSubnode *pSubnode = pReach->GetReachSubnode( subnode );
   ASSERT( pSubnode != NULL );

   float Q = pSubnode->m_discharge;
   SetGeometry(  pReach, Q );
   float width = pReach->m_width;
   float depth = pReach->m_depth;
   pSubnode->m_volume=width*depth*pReach->m_length/pReach->m_subnodeArray.GetSize();
   //pHydro->waterVolumeArrayPrevious[i]=width*depth*length;
   float n = pReach->m_n;
   float slope = pReach->m_slope;
   if (slope < 0.001f)
      slope = 0.05f;
   float wp = width + depth + depth;

   float alph =  n * (float)pow( (long double) wp, (long double) (2/3.)) / (float)sqrt( slope );

   float alpha = (float) pow((long double) alph, (long double) 0.6);
      // Qin is the value upstream at the current time
      // Q is the value at the current location at the previous time

   Qin = GetReachInflow( pReach, subnode );

   float Qstar = ( Q + Qin ) / 2.0f;   // from Chow, eqn. 9.6.4

   float z = alpha * beta * (float) pow( Qstar, beta-1.0f );
   //// start computing new Q value ///
   // next, inflow term
   float Qin_dx = Qin / dx * (float)dt;
   // next, current flow rate
   float Qcurrent_z_dt = Q * z ;
   // last, divisor
   float divisor = z + (float)dt/dx;

   // compute new Q
   Qnew = (qsurface + Qin_dx + Qcurrent_z_dt )/divisor; //m3/s

   SetGeometry(  pReach, Qnew );
   width = pReach->m_width;
   depth = pReach->m_depth;
   pSubnode->m_volume=width*depth*pReach->m_length/pReach->m_subnodeArray.GetSize();

   return Qnew*SEC_PER_DAY;
   }
*/

//float FlowModel::GetReachOutflow( ReachNode *pReachNode )   // recursive!!! for pahntom nodes
//   {
//   if ( pReachNode == NULL )
//      return 0;
//
//   if ( pReachNode->IsPhantomNode() )
//      {
//      float q = GetReachOutflow( pReachNode->m_pLeft );
//      q += GetReachOutflow( pReachNode->m_pRight );
//
//      return q;
//      }
//   else
//      {
//      Reach *pReach = GetReachFromNode( pReachNode );
//
//      if ( pReach != NULL )
//         return pReach->GetDischarge();
//      else
//         {
//         ASSERT( 0 );
//         return 0;
//         }
//      }
//   }


//float FlowModel::GetReachInflow( Reach *pReach, int subNode )
//   {
//   float Q = 0;
//
//   ///////
//   if ( subNode == 0 )  // look upstream?
//      {
//      Reservoir *pRes = pReach->m_pReservoir;
//
//      if ( pRes == NULL )
//         {
//         Q = GetReachOutflow( pReach->m_pLeft );
//         Q += GetReachOutflow( pReach->m_pRight );
//         }
//      else
//         //Q = pRes->GetResOutflow( pRes, m_flowContext.dayOfYear );
//       Q = pRes->m_outflow/SEC_PER_DAY;  //m3/day to m3/s
//      }
//   else
//      {
//      ReachSubnode *pNode = (ReachSubnode*) pReach->m_subnodeArray[ subNode-1 ];  ///->GetReachSubnode( subNode-1 );
//      ASSERT( pNode != NULL );
//      Q = pNode->m_discharge;
//      }
//
//   return Q;
//   }


//float FlowModel::GetLateralSVInflow( Reach *pReach, int sv )
//   {
//   float inflow = 0;
//   ReachSubnode *pNode = pReach->GetReachSubnode( 0 );
//   inflow=pNode->GetExtraSvFluxValue(sv);
//   inflow /= pReach->GetSubnodeCount();  // kg/d
//
//   return inflow;
//   }
//
//float FlowModel::GetReachSVOutflow( ReachNode *pReachNode, int sv )   // recursive!!! for pahntom nodes
//   {
//   if ( pReachNode == NULL )
//      return 0;
//
//   if ( pReachNode->IsPhantomNode() )
//      {
//      float flux = GetReachSVOutflow( pReachNode->m_pLeft, sv );
//      flux += GetReachSVOutflow( pReachNode->m_pRight, sv );
//
//      return flux;
//      }
//   else
//      {
//      Reach *pReach = GetReachFromNode( pReachNode );
//      ReachSubnode *pNode = (ReachSubnode*) pReach->m_subnodeArray[ 0 ]; 
//
//      if ( pReach != NULL && pNode->m_volume>0.0f)
//         return pNode->m_svArray[sv]/pNode->m_volume*pNode->m_discharge*86400.0f;//kg/d 
//      else
//         {
//         //ASSERT( 0 );
//         return 0;
//         }
//      }
//   }
//
//
//float FlowModel::GetReachSVInflow( Reach *pReach, int subNode, int sv )
//   {
//    float flux=0.0f;
//    float Q=0.0f;
//
//   if ( subNode == 0 )  // look upstream?
//      {
//      Reservoir *pRes = pReach->m_pReservoir;
//
//      if ( pRes == NULL )
//         {
//         flux = GetReachSVOutflow( pReach->m_pLeft, sv );
//         flux += GetReachSVOutflow( pReach->m_pRight, sv );
//         }
//      else
//         flux = pRes->m_outflow/SEC_PER_DAY;  //m3/day to m3/s
//      }
//   else
//      {
//      ReachSubnode *pNode = (ReachSubnode*) pReach->m_subnodeArray[ subNode-1 ];  ///->GetReachSubnode( subNode-1 );
//      ASSERT( pNode != NULL );
//     
//      if (pNode->m_svArray != NULL)
//         flux  = pNode->m_svArray[sv]/pNode->m_volume*  pNode->m_discharge*86400.0f;//kg/d 
//      }
//
//   return flux; //kg/d;
//   }

//void FlowModel::SetGeometry( Reach *pReach, float discharge )
//   {
//   ASSERT( pReach != NULL );
//   pReach->m_depth = GetDepthFromQ(pReach, discharge, pReach->m_wdRatio );
//   pReach->m_width = pReach->m_wdRatio * pReach->m_depth;
//   }
//
//
//float FlowModel::GetDepthFromQ( Reach *pReach, float Q, float wdRatio )  // ASSUMES A SPECIFIC CHANNEL GEOMETRY
//   {
//  // from kellie:  d = ( 5/9 Q n s^2 ) ^ 3/8   -- assumes width = 3*depth, rectangular channel
//   float wdterm = (float) pow( (wdRatio/( 2 + wdRatio )), 2.0f/3.0f)*wdRatio;
//   float depth  = (float) pow(((Q*pReach->m_n)/((float)sqrt(pReach->m_slope)*wdterm)), 3.0f/8.0f);
//   return depth;
//   }


/* deprecated

bool FlowModel::RedrawMap(EnvContext *pEnvContext)
   {
   //WriteDataToMap(pEnvContext);

   int activeField = m_pCatchmentLayer->GetActiveField();
   //   m_pStreamLayer->UseVarWidth(0, 200);
   m_pStreamLayer->ClassifyData();
   m_pCatchmentLayer->ClassifyData(activeField);

   if (m_grid) // m_buildCatchmentsMethod==2 )//a grid based simulation
      {
      if (m_pGrid != NULL)
         {
         if (m_gridClassification != 1)
            {
            m_pGrid->SetBins(-1, 0, 5, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 3)
            {
            m_pGrid->SetBins(-1, 0, 1, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 80)
            {
            m_pGrid->SetBins(-1, 0, 0.01f, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 0)
            {
            m_pGrid->SetBins(-1, 0.1f, 0.4f, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 1) //elws
            {
            m_pGrid->SetBins(-1, -10, 100, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 2) //depthToWT
            {
            m_pGrid->SetBins(-1, -5, 15, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 6)
            {
            m_pGrid->SetBins(-1, 0.0f, 1500.0f, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 20)
            {
            m_pGrid->SetBins(-1, 0.0f, 0.4f, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }

         if (m_gridClassification == 30)
            {
            m_pGrid->SetBins(-1, 0.f, 21000.f, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }

         if (m_gridClassification == 50)//actual et
            {
            m_pGrid->SetBins(-1, -0.1f, 6.f, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUERED);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 60)//runoff
            {
            m_pGrid->SetBins(-1, -.10f, 20, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_REDGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 70)//saturated ground
            {
            m_pGrid->SetBins(-1, 0, 1, 2, TYPE_INT);
            m_pGrid->SetBinColorFlag(BCF_MIXED);
            m_pGrid->ClassifyData(0);
            }


         if (m_gridClassification == 4)//SWE in mm
            {
            m_pGrid->SetBins(-1, 0.0f, 1200.0f, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification == 5)//SWE in mm
            {
            m_pGrid->SetBins(-1, -15.0f, 30.0f, 20, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUERED);
            m_pGrid->ClassifyData(0);
            }

         if (m_gridClassification == 300)//SWE in mm
            {
            m_pGrid->SetBins(-1, 0.0f, 0.001f, 15, TYPE_FLOAT);
            m_pGrid->SetBinColorFlag(BCF_BLUERED);
            m_pGrid->ClassifyData(0);
            }
         }
      }

   ::EnvRedrawMap();

   return true;
   }
   */

void FlowModel::UpdateYearlyDeltas(EnvContext *pEnvContext)
   {
   int activeField = m_pCatchmentLayer->GetActiveField();
   int catchmentCount = (int)m_catchmentArray.GetSize();
   float airTemp = -999.0f;

   if (!m_pGrid)   //m_buildCatchmentsMethod != 2 )    // not a grid based model?
      {
      for (int i = 0; i < catchmentCount; i++)
         {
         Catchment *pCatchment = m_catchmentArray[i];
         HRU *pHRU = NULL;

         for (int j = 0; j < pCatchment->GetHRUCount(); j++)
            {
            HRU *pHRU = pCatchment->GetHRU(j);

            float aridityIndex = pHRU->m_maxET_yr / pHRU->m_precip_yr;
            pHRU->m_aridityIndex.AddValue(aridityIndex);
            float aridityIndexMovingAvg = pHRU->m_aridityIndex.GetAvgValue();
            pHRU->m_temp_yr /= 365;   // note: assumed daily timestep

            for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
               {
               int idu = pHRU->m_polyIndexArray[k];

               // annual climate variables
               this->UpdateIDU(pEnvContext, idu, m_colAridityIndex, aridityIndexMovingAvg, ADD_DELTA);

               pHRU->m_temp_10yr.AddValue(pHRU->m_temp_yr);
               pHRU->m_precip_10yr.AddValue(pHRU->m_precip_yr);

               this->UpdateIDU(pEnvContext, idu, m_colHruTempYr, pHRU->m_temp_yr, ADD_DELTA);   // C
               this->UpdateIDU(pEnvContext, idu, m_colHruTemp10Yr, pHRU->m_temp_10yr.GetAvgValue(), ADD_DELTA); // C

               this->UpdateIDU(pEnvContext, idu, m_colHruPrecipYr, pHRU->m_precip_yr, ADD_DELTA);   // mm/year
               this->UpdateIDU(pEnvContext, idu, m_colHruPrecip10Yr, pHRU->m_precip_10yr.GetAvgValue(), ADD_DELTA); // mm/year
               // m_precip_wateryr????

               // annual hydrologic variables

               // SWE-related variables (Note: Apr1 value previously Updated()
               //gpFlow->UpdateIDU( pEnvContext, idu, m_colHruApr1SWE, pHRU->m_depthApr1SWE_yr, true );
               //gpFlow->UpdateIDU( pEnvContext, idu, m_colHruApr1SWE10yr, pHRU->m_apr1SWE_10yr.GetValue(), true );

               this->UpdateIDU(pEnvContext, idu, m_colRunoff_yr, pHRU->m_runoff_yr, ADD_DELTA);
               this->UpdateIDU(pEnvContext, idu, m_colStorage_yr, pHRU->m_storage_yr, ADD_DELTA);
               this->UpdateIDU(pEnvContext, idu, m_colMaxET_yr, pHRU->m_maxET_yr, ADD_DELTA);
               this->UpdateIDU(pEnvContext, idu, m_colET_yr, pHRU->m_et_yr, ADD_DELTA);
               }
            }
         }
      }
   }


void FlowModel::GetMaxSnowPack(EnvContext *pEnvContext)
   {
   m_pCatchmentLayer->m_readOnly = false;
   int catchmentCount = (int)m_catchmentArray.GetSize();
   float totalSnow = 0.0f;
   //First sum the total volume of snow for this day
   for (int i = 0; i < catchmentCount; i++)
      {
      Catchment *pCatchment = m_catchmentArray[i];
      for (int j = 0; j < pCatchment->GetHRUCount(); j++)
         {
         HRU *pHRU = pCatchment->GetHRU(j);
         for ( int k=0; k < this->m_poolInfoArray.GetSize(); k++ )
            {
            if ( this->m_poolInfoArray[k]->m_type == PT_SNOW /*|| this->m_poolInfoArray[k]->m_type == PT_MELT*/ )
                if ((float)pHRU->GetPool(k)->m_volumeWater > 0.0f)
                   totalSnow += (float)pHRU->GetPool( k)->m_volumeWater/1e6; //get the current year snowpack
            }
         }
      }
   if (totalSnow < 0)
       totalSnow = 0;
   // then compare the total volume to the previously defined maximum volume
   if (totalSnow > m_volumeMaxSWE)
      {
      m_volumeMaxSWE = totalSnow;
      int dayOfYear = int(fmod(m_timeInRun, 365));  // zero based day of year
      m_dateMaxSWE = dayOfYear;

      //#pragma omp parallel for
      for (int j = 0; j < m_hruArray.GetSize(); j++)
         {
         HRU *pHRU = m_hruArray.GetAt(j);
         float volSnow = 0;
         for (int k = 0; k < this->m_poolInfoArray.GetSize(); k++)
            {
            if (this->m_poolInfoArray[k]->m_type == PT_SNOW /*|| this->m_poolInfoArray[k]->m_type == PT_MELT*/)
               volSnow += (float)pHRU->GetPool(k)->m_volumeWater; //get the current year snowpack
            }

//         for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
 //           this->UpdateIDU(pEnvContext, pHRU->m_polyIndexArray[k], m_colHruMaxSWE, volSnow, SET_DATA);
         }
      }
   m_pCatchmentLayer->m_readOnly = true;
   }


void FlowModel::UpdateAprilDeltas(EnvContext *pEnvContext)
   {
   // int activeField = m_pCatchmentLayer->GetActiveField();
   int catchmentCount = (int)m_catchmentArray.GetSize();
   m_pCatchmentLayer->m_readOnly = false;
   // float airTemp=-999.0f;

   if (!m_pGrid)   //m_buildCatchmentsMethod != 2 )    // not a grid based model?
      {
      for (int i = 0; i < catchmentCount; i++)
         {
         Catchment *pCatchment = m_catchmentArray[i];
         for (int j = 0; j < pCatchment->GetHRUCount(); j++)
            {
            HRU *pHRU = pCatchment->GetHRU(j);
            for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
               {
               int idu = pHRU->m_polyIndexArray[k];

               float refSnow = 0.0f;
               if (pEnvContext->yearOfRun == 0) //reset the reference
                  m_pCatchmentLayer->SetData(idu, m_colRefSnow, 0.0f);
               else
                  m_pCatchmentLayer->GetData(idu, m_colRefSnow, refSnow);

               float volSnow = 0;
               for (int k = 0; k < this->m_poolInfoArray.GetSize(); k++)
                  {
                  if (this->m_poolInfoArray[k]->m_type == PT_SNOW /*|| this->m_poolInfoArray[k]->m_type == PT_MELT*/)
                     volSnow += (float) pHRU->GetPool( k)->m_volumeWater; //get the current year snowpack
                  }

               float _ref = volSnow / pHRU->m_area*1000.0f; //get the current year snowpack (mm)

               if (pEnvContext->yearOfRun < 10)//estimate the denominator of the snow index
                  m_pCatchmentLayer->SetData(idu, m_colRefSnow, _ref + refSnow); //total amount of snow over the 5 year period

               float snowIndex = 0.0f;
               if (refSnow > 0.0f)
                  snowIndex = _ref / (refSnow / 10.0f);    // compare to avg over first decade

               if (pEnvContext->yearOfRun >= 10)
                  this->UpdateIDU(pEnvContext, idu, m_colSnowIndex, snowIndex, SET_DATA);

               float apr1SWE = (volSnow / pHRU->m_area) * MM_PER_M; // (mm)
               if (apr1SWE < 0)
                  apr1SWE = 0;

               this->UpdateIDU(pEnvContext, idu, m_colHruApr1SWE, apr1SWE, ADD_DELTA);

               pHRU->m_apr1SWE_10yr.AddValue(apr1SWE);
               float apr1SWEMovAvg = pHRU->m_apr1SWE_10yr.GetAvgValue();
               this->UpdateIDU(pEnvContext, idu, m_colHruApr1SWE10Yr, apr1SWEMovAvg, ADD_DELTA);

               pHRU->m_precipCumApr1 = pHRU->m_precip_yr;      // mm of cum precip Jan 1 - Apr 1)
               }
            }
         }
      }

   m_pCatchmentLayer->m_readOnly = true;
   }

// called each FLOW timestep when m_mapUpdate == 2
bool FlowModel::WriteDataToMap(EnvContext *pEnvContext)
   {
   int activeField = m_pCatchmentLayer->GetActiveField();
   m_pCatchmentLayer->m_readOnly = false;
   int catchmentCount = (int)m_catchmentArray.GetSize();

   float time = float(m_currentTime - (int)m_currentTime);
   float currTime = m_flowContext.dayOfYear + time;
   //if (!m_pGrid)   //m_buildCatchmentsMethod != 2 )    // not a grid based model?
    //  {
      int hruCount = (int)m_hruArray.GetSize();
      //#pragma omp parallel for  // firstprivate( pEnvContext )
      for (int h = 0; h < hruCount; h++)
         {
         float airTempMin = -999.0f; float airTempMax = 0.0f; float airTemp = 0.0f;
         float prec = 0.0f;
         HRU *pHRU = m_hruArray[h];
         GetHRUClimate(CDT_TMIN, pHRU, (int)currTime, airTempMin);
         GetHRUClimate(CDT_TMAX, pHRU, (int)currTime, airTempMax);
         airTemp = (airTempMin + airTempMax) / 2.0f;
         GetHRUClimate(CDT_PRECIP, pHRU, (int)currTime, prec);
         for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
            {
            int idu = pHRU->m_polyIndexArray[k];

            m_pCatchmentLayer->SetData(idu, m_colHruTemp, airTemp);
            m_pCatchmentLayer->SetData(idu, m_colHruPrecip, prec);

            float snow = 0;     (float)pHRU->GetPool(0)->m_volumeWater ;
            for (int k = 0; k < this->m_poolInfoArray.GetSize(); k++)
               {
               if (this->m_poolInfoArray[k]->m_type == PT_SNOW )
                  snow += ((float) pHRU->GetPool( k)->m_volumeWater/ pHRU->m_area)*MM_PER_M; //get the current year snowpack
               }

            m_pCatchmentLayer->SetData(idu, m_colHRUMeanLAI, pHRU->m_meanLAI); //lai
            m_pCatchmentLayer->SetData(idu, m_colAgeClass, pHRU->m_meanAge); //lai
            m_pCatchmentLayer->SetData( idu, m_colHruSWC,   pHRU->m_swc );
            m_pCatchmentLayer->SetData(idu, m_colIrrigation, pHRU->m_irr);
            }
         }
	  activeField = m_pCatchmentLayer->GetActiveField();
	  m_pCatchmentLayer->ClassifyData(activeField);
    //  }

   int reachCount = (int)m_reachArray.GetSize();
   m_pStreamLayer->m_readOnly = false;

   for (int i = 0; i < reachCount; i++)
      {
      Reach *pReach = m_reachArray.GetAt(i);
      ASSERT(pReach != NULL);

      if (pReach->m_subnodeArray.GetSize() > 0 && pReach->m_polyIndex >= 0)
         {
         ReachSubnode *pNode = (ReachSubnode*)pReach->m_subnodeArray[0];
         ASSERT(pNode != NULL);
         m_pStreamLayer->SetData(pReach->m_polyIndex, m_colStreamReachOutput, pNode->m_discharge);  // m3/sec
         }
      }

   if (m_pGrid) // m_buildCatchmentsMethod==2)//a grid based simulation
      {
      int hruCount = (int)m_hruArray.GetSize();

/*
      float modelDepth = 0;
      for (int i = 0; i < this->m_poolInfoArray.GetSize(); i++)
         modelDepth += this->m_poolInfoArray[ i ]->m_depth;

#pragma omp parallel for // firstprivate( pEnvContext )
      for (int h = 0; h < hruCount; h++)
         {
         HRU *pHRU = m_hruArray[h];
         float waterDepth = 0.0f;
         for (int i = 0; i < pHRU->GetPoolCount(); i++)
            waterDepth += pHRU->GetPool(i)->m_wDepth;
         float elws = pHRU->m_elevation - modelDepth + (waterDepth / 1000.0f);//m
         float depthToWT = pHRU->m_elevation - elws;
         float wc = 0.0f;
         float myElev = 50;
         if (m_gridClassification == 0)
            wc = pHRU->GetPool(0)->m_wDepth;
         if (m_gridClassification == 1)
            wc = elws;
         if (m_gridClassification == 2)
            wc = depthToWT;
         if (m_gridClassification == 6)//water content in the 1st layer
            wc = pHRU->GetPool(1)->m_wDepth;
         if (m_gridClassification == 20)//water content in the 1st layer
            wc = float((pHRU->GetPool( 0)->m_volumeWater + pHRU->GetPool( 1)->m_volumeWater) / (pHRU->m_area*(pHRU->GetPool( 0)->m_depth + pHRU->GetPool( 1)->m_depth)));

         if (m_gridClassification == 30)//water content in the GW layer
            wc = pHRU->GetPool( 4)->m_wDepth;

         if (m_gridClassification == 50)//water content in the GW layer
            {
            wc = pHRU->m_currentET;
            }
         if (m_gridClassification == 60)//water content in the GW layer
            {
            wc = pHRU->m_currentRunoff;

            }
         if (m_gridClassification == 70)//map saturated ground
            {
            wc = pHRU->m_biomass;
            }


         if (m_gridClassification == 10)
            {
            if (pHRU->GetPool( 1)->m_volumeWater > 0.0f)
               {
               float porosity = 0.4f;
               float area = pHRU->m_area;
               HRUPool *pHRULayer0 = pHRU->GetPool( 0);
               HRUPool *pHRULayer1 = pHRU->GetPool( 1);
               float d0 = pHRULayer0->m_depth;
               float d1 = pHRULayer1->m_depth;
               float w0 = (float)pHRULayer0->m_volumeWater;
               float w1 = (float)pHRULayer1->m_volumeWater;

               float depthWater = (w0 + w1) / area;//length of water
               float totalDepth = (d1 + d0)*porosity; //total length (of airspace) available
               float depthToWT = totalDepth - depthWater;
               if (depthToWT < 0.0f)
                  depthToWT = 0.0f;
               wc = depthToWT;
               }
            }
         if (pHRU->GetPool( 0)->m_svArray != NULL)
            {
            if (m_gridClassification == 2)
               {
               if (pHRU->GetPool( 0)->m_volumeWater > 0.0f)
                  {
                  float water = (float)pHRU->GetPool( 0)->m_volumeWater;
                  float mass = pHRU->GetPool( 0)->GetExtraStateVarValue(0);
                  wc = mass / water;//kg/m3
                  int stop = 1;
                  }
               }
            if (m_gridClassification == 3)
               {
               if (pHRU->GetPool( 1)->m_volumeWater > 0.0f)
                  {
                  float water = (float)pHRU->GetPool( 1)->m_volumeWater;
                  float mass = pHRU->GetPool( 1)->GetExtraStateVarValue(0);
                  wc = mass / water;//kg/m3
                  int stop = 1;
                  }
               }

            if (m_gridClassification == 80)
               {
               if (pHRU->GetPool( 4)->m_volumeWater > 0.0f)
                  {
                  float water = (float)pHRU->GetPool( 4)->m_volumeWater;
                  float mass = pHRU->GetPool( 4)->GetExtraStateVarValue(0);
                  wc = mass / water;//kg/m3
                  int stop = 1;
                  }
               }
            if (m_gridClassification == 300)
               {
               float mass = pHRU->GetPool( 4)->GetExtraStateVarValue(0);
               if (pHRU->GetPool( 4)->m_volumeWater > 0.0f && mass > 0.0f)
                  {
                  float water = (float)pHRU->GetPool( 4)->m_volumeWater;
                  wc = mass / water;//kg/m3
                  int stop = 1;
                  }
               }

            }
         m_pGrid->SetData(pHRU->m_demRow, pHRU->m_demCol, wc);
         }
         */
      }

   return true;
   }


bool FlowModel::ResetFluxValuesForStep(EnvContext *pEnvContext)
   {
   for (int i = 0; i < m_catchmentArray.GetSize(); i++)
      {
      ASSERT(m_catchmentArray[i] != NULL);
      m_catchmentArray[i]->m_contributionToReach = 0.0f;
      }

   for (int i = 0; i < m_reachArray.GetSize(); i++)
      {
      Reach *pReach = m_reachArray.GetAt(i);
      pReach->ResetFluxValue();

      ReachSubnode *subnode = pReach->GetReachSubnode(0);
      for (int k = 0; k < m_hruSvCount - 1; k++)
         subnode->ResetExtraSvFluxValue(k);
      }

   for (int i = 0; i < m_reservoirArray.GetSize(); i++)
      {
      Reservoir *pRes = m_reservoirArray.GetAt(i);
      pRes->ResetFluxValue();
      }


   for (int i = 0; i < m_hruArray.GetSize(); i++)
      {
      HRU *pHRU = m_hruArray.GetAt(i);
      int neighborCount = (int)pHRU->m_neighborArray.GetSize();

      for (int j = 0; j < pHRU->m_poolArray.GetSize(); j++)
         {
         HRUPool *pLayer = pHRU->m_poolArray.GetAt(j);
         pLayer->ResetFluxValue();

         for (int k = 0; k < (int)pLayer->m_waterFluxArray.GetSize(); k++) //kbv 4/27/15   why would these two lines be commented out?
            pLayer->m_waterFluxArray[k] = 0.0f;

         for (int l = 0; l < m_hruSvCount - 1; l++)
            pLayer->ResetExtraSvFluxValue(l);
         }
      }

   return true;
   }


bool FlowModel::ResetCumulativeYearlyValues()
   {
   EnvContext *pEnvContext = m_flowContext.pEnvContext;

   for (int i = 0; i < m_hruArray.GetSize(); i++)
      {
      HRU *pHRU = m_hruArray.GetAt(i);

      int hruPoolCount = pHRU->GetPoolCount();
      float waterDepth = 0.0f;
      for (int l = 0; l < hruPoolCount; l++)
         {
         HRUPool *pHRUPool = pHRU->GetPool(l);
         waterDepth += float(pHRUPool->m_wDepth);//mm of total storage
         }

      pHRU->m_endingStorage = waterDepth;
      pHRU->m_storage_yr = pHRU->m_endingStorage - pHRU->m_initStorage;
      pHRU->m_initStorage = pHRU->m_endingStorage;

      //for ( int i=0; i < pHRU->m_polyIndexArray.GetSize(); i++ )
      //   {
      //   int idu = pHRU->m_polyIndexArray[ i ];
      //
      //   gpFlow->UpdateIDU( pEnvContext, idu, m_colPrecip_yr,  pHRU->m_precip_yr, true );
      //   gpFlow->UpdateIDU( pEnvContext, idu, m_colRunoff_yr,  pHRU->m_runoff_yr, true );
      //   gpFlow->UpdateIDU( pEnvContext, idu, m_colStorage_yr, pHRU->m_storage_yr, true );
      //   gpFlow->UpdateIDU( pEnvContext, idu, m_colMaxET_yr,   pHRU->m_maxET_yr, true);
      //   gpFlow->UpdateIDU( pEnvContext, idu, m_colET_yr,      pHRU->m_et_yr, true);
      //   }

      pHRU->m_temp_yr = 0;
      pHRU->m_precip_yr = 0;
      pHRU->m_rainfall_yr = 0;
      pHRU->m_snowfall_yr = 0;
      pHRU->m_gwRecharge_yr = 0.0f;       // mm
      pHRU->m_gwFlowOut_yr = 0.0f;
      pHRU->m_precipCumApr1 = 0;
      pHRU->m_maxET_yr = 0.0f;
      pHRU->m_et_yr = 0.0f;
      pHRU->m_runoff_yr = 0.0f;
      }

   return true;
   }


bool FlowModel::ResetCumulativeWaterYearValues()
   {
   for (int i = 0; i < m_hruArray.GetSize(); i++)
      {
      HRU *pHRU = m_hruArray.GetAt(i);
      pHRU->m_precip_wateryr = 0.0f;
      }
   return true;
   }


// create, initialize reaches based on the stream layer
bool FlowModel::InitReaches(void)
   {
   ASSERT(m_pStreamLayer != NULL);

   m_colStreamFrom = m_pStreamLayer->GetFieldCol(m_fromCol);
   m_colStreamTo = m_pStreamLayer->GetFieldCol(m_toCol);

   if (m_colStreamFrom < 0 || m_colStreamTo < 0)
      {
      CString msg;
      msg.Format(_T("Flow: Stream layer is missing a required column (%s and/or %s).  These are necessary to establish topology"), (LPCTSTR)m_fromCol, (LPCTSTR)m_toCol);
      Report::LogError(msg);
      return false;
      }

   /////////////////////
   //   FDataObj dataObj;
   //   m_reachTree.BuildDistanceMatrix( &dataObj );
   //   dataObj.WriteAscii( _T("C:\\envision\\studyareas\\wesp\\distance.csv") );
   /////////////////////

      // figure out which stream layer reaches to actually use in the model
   if (m_pStreamQE != NULL && m_streamQuery.GetLength() > 0)
      {
      CString source(_T("Flow Stream Query"));

      m_pStreamQuery = m_pStreamQE->ParseQuery(m_streamQuery, 0, source);

      if (m_pStreamQuery == NULL)
         {
         CString msg(_T("Flow: Error parsing stream query: '"));
         msg += m_streamQuery;
         msg += _T("' - the query will be ignored");
         Report::ErrorMsg(msg);
         }
      }

   // build the network topology for the stream reaches
   int nodeCount = m_reachTree.BuildTree(m_pStreamLayer, &Reach::CreateInstance, m_colStreamFrom, m_colStreamTo, m_colStreamJoin /*in*/, NULL /*m_pStreamQuery*/);   // jpb 10/7

   if (nodeCount < 0)
      {
      CString msg;
      msg.Format(_T("Flow: Error building reach topology:  Error code %i"), nodeCount);
      Report::ErrorMsg(msg);
      return false;
      }

   // populate stream order if column provided
   if (m_colStreamOrder >= 0)
      m_reachTree.PopulateOrder(m_colStreamOrder, true);

   // populate TreeID if specified
   if (m_colTreeID >= 0)
      {
      m_pStreamLayer->SetColNoData(m_colTreeID);
      int roots = m_reachTree.GetRootCount();

      for (int i = 0; i < roots; i++)
         {
         ReachNode *pRoot = m_reachTree.GetRootNode(i);
         PopulateTreeID(pRoot, i);
         }
      }


   // for each tree, add Reaches to our internal array of Reach ptrs
   int roots = m_reachTree.GetRootCount();

   for (int i = 0; i < roots; i++)
      {
      ReachNode *pRoot = m_reachTree.GetRootNode(i);
      ReachNode *pReachNode = (Reach*)m_reachTree.FindLeftLeaf(pRoot);  // find leftmost leaf of entire tree
      m_reachArray.Add((Reach*)pReachNode);   // add leftmost node

      while (pReachNode)
         {
         ReachNode *pDownNode = pReachNode->m_pDown;

         if (pDownNode == NULL) // root of tree reached?
            break;

         if ((pDownNode->m_pLeft == pReachNode) && (pDownNode->m_pRight != NULL))
            pReachNode = m_reachTree.FindLeftLeaf(pDownNode->m_pRight);
         else
            pReachNode = pDownNode;

         if (pReachNode->IsPhantomNode() == false)
            m_reachArray.Add((Reach*)pReachNode);
         }
      }

   //ASSERT(nodeCount == (int) m_reachArray.GetSize() );

   // allocate subnodes to each reach
   m_reachTree.CreateSubnodes(&ReachSubnode::CreateInstance, m_subnodeLength);
   //initialize the subnode volumes using mannings equation
   int reachCount = (int)m_reachArray.GetSize();
   for (int i = 0; i < reachCount; i++)
      {
      Reach *pReach = m_reachArray[i];

      for (int j = 0; j < pReach->GetSubnodeCount(); j++)
         {
         ReachSubnode *pSubnode = pReach->GetReachSubnode(j);
         float Q = pSubnode->m_discharge;
         SetGeometry(pReach, Q);
         pReach->m_n = m_n;
         pReach->m_wdRatio = m_wdRatio;
         pSubnode->m_volume = pReach->m_width*pReach->m_depth*pReach->m_length / pReach->m_subnodeArray.GetSize();
         int i = 0;
         }
      }


   // allocate any necessary "extra" state variables
   if (m_reachSvCount > 1)
      {
      int reachCount = (int)m_reachArray.GetSize();
      for (int i = 0; i < reachCount; i++)
         {
         Reach *pReach = m_reachArray[i];

         for (int j = 0; j < pReach->GetSubnodeCount(); j++)
            {
            ReachSubnode *pSubnode = pReach->GetReachSubnode(j);
            pSubnode->AllocateStateVars(m_reachSvCount - 1);

            }
         }
      }

   return true;
   }


bool FlowModel::InitReachSubnodes(void)
   {
   ASSERT(m_pStreamLayer != NULL);

   // allocate subnodes to each reach.  Note that this ignores phantom nodes
   m_reachTree.CreateSubnodes(&ReachSubnode::CreateInstance, m_subnodeLength);

   // allocate any necessary "extra" state variables
   if (m_reachSvCount > 1)
      {
      int reachCount = (int)m_reachArray.GetSize();
      for (int i = 0; i < reachCount; i++)
         {
         Reach *pReach = m_reachArray[i];
         ASSERT(pReach != NULL);

         for (int j = 0; j < pReach->GetSubnodeCount(); j++)
            {
            ReachSubnode *pSubnode = pReach->GetReachSubnode(j);
            ASSERT(pSubnode != NULL);

            pSubnode->AllocateStateVars(m_reachSvCount - 1);
            for (int k = 0; k < m_reachSvCount - 1; k++)
               {
               pSubnode->m_svArray[k] = 0.0f;//concentration = 1 kg/m3 ???????
               pSubnode->m_svArrayTrans[k] = 0.0f;//concentration = 1 kg/m3
               }
            }
         }
      }

   return true;
   }


void FlowModel::PopulateTreeID(ReachNode *pNode, int treeID)
   {
   if (pNode == NULL)
      return;

   if (pNode->m_polyIndex < 0)
      return;

   m_pStreamLayer->SetData(pNode->m_polyIndex, m_colTreeID, treeID);

   // recurse up tree
   PopulateTreeID(pNode->m_pLeft, treeID);
   PopulateTreeID(pNode->m_pRight, treeID);
   }


// create, initialize reaches based on the stream layer
bool FlowModel::InitCatchments(void)
   {
   ASSERT(m_pCatchmentLayer != NULL);

   // first, figure out where the catchments and HRU's are.
   if (m_pCatchmentQE != NULL && m_catchmentQuery.GetLength() > 0)
      {
      m_pCatchmentQuery = m_pCatchmentQE->ParseQuery(m_catchmentQuery, 0, _T("Flow Catchment Query"));

      if (m_pCatchmentQuery == NULL)
         {
         CString msg(_T("Flow: Error parsing Catchment query: '"));
         msg += m_catchmentQuery;
         msg += _T("' - the query will be ignored");
         Report::ErrorMsg(msg);
         }
      }

   if (m_pGrid)//if m_pGrid is not NULL, then we are using grids to define the system of model elements
      BuildCatchmentsFromGrids();
   else
      {
      switch (m_buildCatchmentsMethod)
         {
         case 1:
            BuildCatchmentsFromQueries();   // requires m_catchmentAggCols to be defined
            break;

         default:
            BuildCatchmentsFromLayer();      // requires 
         }
      }

   SetCatchmentAttributes();     // populates internal HRU members (area, elevation, centroid) for all catchments

    // allocate any necessary "extra" state variables
   if (m_hruSvCount > 1)//then there are extra state variables..
      {
      int catchmentCount = (int)m_catchmentArray.GetSize();
      for (int i = 0; i < catchmentCount; i++)
         {
         Catchment *pCatchment = m_catchmentArray[i];
         HRU *pHRU = NULL;

         for (int j = 0; j < pCatchment->GetHRUCount(); j++)
            {
            HRU *pHRU = pCatchment->GetHRU(j);

            for (int k = 0; k < pHRU->GetPoolCount(); k++)
               {
               HRUPool *pLayer = pHRU->GetPool(k);
               pLayer->AllocateStateVars(m_hruSvCount - 1);
               }
            }
         }
      }

   return true;
   }


bool FlowModel::SetCatchmentAttributes(void)
   {
   if (!m_pGrid)   // if ( m_buildCatchmentsMethod < 2 )
      {
      SetHRUAttributes();
      int catchmentCount = (int)m_catchmentArray.GetSize();
      for (int i = 0; i < catchmentCount; i++)
         {
         Catchment *pCatchment = m_catchmentArray[ i ];

         pCatchment->m_area = 0;
         for ( int j=0; j < pCatchment->GetHRUCount(); j++ )
            {
            HRU *pHRU = pCatchment->GetHRU( j );
            pCatchment->m_area += pHRU->m_area;
            }
         }
      }

   return true;
   }


bool FlowModel::SetHRUAttributes()
   {
   for (int j = 0; j < m_hruArray.GetSize(); j++)
      {
      HRU *pHRU = m_hruArray[ j ];
      float area = 0.0f; float elevation = 0.0f; float _elev = 0.0f;
      pHRU->m_area = 0;

      for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
         {
         m_pCatchmentLayer->GetData(pHRU->m_polyIndexArray[k], m_colCatchmentArea, area);
         m_pCatchmentLayer->GetData(pHRU->m_polyIndexArray[k], m_colElev, _elev);
         pHRU->m_area += area;
         elevation += _elev*area;
         }

      pHRU->m_elevation = elevation / pHRU->m_area;
    
      // set up pool areas
      for (int k = 0; k < this->m_poolInfoArray.GetSize(); k++)
         {
         HRUPool *pHRUPool = pHRU->m_poolArray[k];
         PoolInfo *pInfo = this->m_poolInfoArray[k];
         pHRUPool->m_volumeWater = pInfo->m_initWaterContent*pHRUPool->m_depth*pHRU->m_area;  
         }

      Poly *pPoly = m_pCatchmentLayer->GetPolygon(pHRU->m_polyIndexArray.GetAt(0));
      Vertex centroid = pPoly->GetCentroid();
      pHRU->m_centroid = centroid;

      }

   return true;
   }



// If we are using grids as the basis for simulation, assume that each grid cell represents an HRU while each catchment is still a catchment
// 
// Assume that the DEM is consistent with the catchment delinations in the IDUs, so that we can take a grid cell centroid and associate
// the catchment at that point with that particular grid cell
// 
// Catchments are collections of grid cells (HRUs) that still have HRULayers
// 
// 

bool FlowModel::BuildCatchmentsFromGrids(void)
   {
   ParseCols(m_pCatchmentLayer, m_catchmentAggCols, m_colsCatchmentAgg);
   ParseCols(m_pCatchmentLayer, m_hruAggCols, m_colsHruAgg);
   int colCatchID = m_pCatchmentLayer->m_pData->GetCol(m_catchIDCol);

   // first, do catchments
   CArray< Query*, Query* > catchmentQueryArray;
   int queryCount = GenerateAggregateQueries(m_pCatchmentLayer, m_colsCatchmentAgg, catchmentQueryArray);

   CArray< Query*, Query* > hruQueryArray;
   int hruQueryCount = GenerateAggregateQueries(m_pCatchmentLayer, m_colsHruAgg, hruQueryArray);

   // temporary allocations
   Catchment **catchments = new Catchment*[queryCount];  // placeholder for catchments
   memset(catchments, 0, queryCount * sizeof(Catchment*));  // initially, all NULL

   m_pCatchmentLayer->SetColNoData(m_colCatchmentCatchID);
   m_pCatchmentLayer->SetColNoData(m_colCatchmentHruID);

   int hruCols = (int)m_colsHruAgg.GetSize();
   VData *hruValueArray = new VData[hruCols];

   int polyCount = m_pCatchmentLayer->GetPolygonCount();
   int hruCount = 0;

   if (m_pCatchmentQuery != NULL)
      polyCount = m_pCatchmentQuery->Select(true);

   // iterate though polygons that satisfy the catchment query.
   for (int i = 0; i < polyCount; i++)
      {
      int polyIndex = m_pCatchmentQuery ? m_pCatchmentLayer->GetSelection(i) : i;

      // get hru aggregate values from this poly
      for (int h = 0; h < hruCols; h++)
         {
         bool ok = m_pCatchmentLayer->GetData(polyIndex, m_colsHruAgg[h], hruValueArray[h]);
         if (!ok)
            hruValueArray[h].SetNull();
         }

      // iterate through the catchment aggregate queries, to see if this polygon satisfies any of them
      for (int j = 0; j < queryCount; j++)
         {
         Query *pQuery = catchmentQueryArray.GetAt(j);
         bool result = false;
         bool ok = pQuery->Run(polyIndex, result);

         if (ok && result)  // is this polygon in a the specified Catchment (query?) - yes!
            {
            if (catchments[j] == NULL)    // if first one, creaet a Catchment object for it
               {
               catchments[j] = new Catchment;
               catchments[j]->m_id = j;
               }
            m_pCatchmentLayer->SetData(i, colCatchID, j);
            }  // end of: if ( ok && result )  meaning adding polyIndex to catchment
         }  // end of: for ( j < queryCount )    
      }  // end of: for ( i < polyCount )

   // store catchments
   for (int j = 0; j < queryCount; j++)
      {
      Catchment *pCatchment = catchments[j];
      if (pCatchment != NULL)
         m_catchmentArray.Add(pCatchment);
      }

   // clean up
   delete[] catchments;
   delete[] hruValueArray;

   if (m_pCatchmentQuery != NULL)
      m_pCatchmentLayer->ClearSelection();

   //Catchments are allocated, now we need to iterate the grid allocating an HRU for every grid cell.
   HRU *pHRU = NULL;
   hruCount = 0;
   int foundHRUs = 0;
   float minElev = 1E10;

   m_pHruGrid = new IDataObj(m_pGrid->GetColCount(), m_pGrid->GetRowCount(), U_UNDEFINED);

   for (int i = 0; i < m_pGrid->GetRowCount(); i++)
      {
      for (int j = 0; j < m_pGrid->GetColCount(); j++)
         {
         REAL x = 0, y = 0;
         int index = -1;
         m_pGrid->GetGridCellCenter(i, j, x, y);
         Poly *pPoly = m_pCatchmentLayer->GetPolygonFromCoord(x, y, &index);
         float elev = 0.0f;

         m_pGrid->GetData(i, j, elev);
         if (elev == m_pGrid->GetNoDataValue())
            m_pHruGrid->Set(j, i, m_pGrid->GetNoDataValue());

         else
            {
            if (pPoly)      // found an IDU at the location provided, try to find a catchment 
               {
               for (int k = 0; k < (int)m_catchmentArray.GetSize(); k++) //figure out which catchment this HRU is in!
                  {
                  Catchment *pCatchment = m_catchmentArray[k];
                  ASSERT(pCatchment != NULL);

                  int catchID = -1;
                  m_pCatchmentLayer->GetData(index, colCatchID, catchID);

                  if (catchID == pCatchment->m_id)//if the catchments ReachID equals this polygons reachID
                     {
                     pHRU = new HRU;
                     pHRU->m_id = hruCount;
                     pHRU->m_demRow = i;
                     pHRU->m_demCol = j;
                     pHRU->m_area = (float)( m_pGrid->GetGridCellWidth()*m_pGrid->GetGridCellWidth() );
                     REAL x = 0.0f; REAL y = 0.0f;
                     m_pGrid->GetGridCellCenter(i, j, x, y);
                     pHRU->m_centroid.x = x;
                     pHRU->m_centroid.y = y;

                     float elev = 0.0f;
                     m_pGrid->GetData(i, j, elev);
                     pHRU->m_elevation = elev;
                     if (elev < minElev)
                        minElev = elev;

                     pHRU->AddPools(this, /*m_poolCount, 0, m_initTemperature, */ true);

                     hruCount++;

                     for (int kk = 0; kk < this->m_poolInfoArray.GetSize(); kk++)
                        {
                        HRUPool *pHRUPool = pHRU->m_poolArray[kk];
                        PoolInfo *pInfo = this->m_poolInfoArray[kk];
                        pHRUPool->m_volumeWater = pInfo->m_initWaterContent*pHRUPool->m_depth*pHRU->m_area;   // 0.4 is the porosity?
                        }

                     AddHRU(pHRU, pCatchment);
                     int offset = (int)m_hruArray.GetSize() - 1;
                     m_pHruGrid->Set(j, i, offset);
                     pHRU->m_polyIndexArray.Add(index);
                     foundHRUs++;
                     break;
                     }
                  }
               }  // end of: if ( poly found )
            else//we have a grid cell that includes data, but its centoid does not overlay the IDUs.  This can happen, particularly with coarsened grids
               m_pHruGrid->Set(j, i, m_pGrid->GetNoDataValue());
            }
         }
      }  // end of: iterating through elevation grid

   m_minElevation = minElev;
   hruCount = GetHRUCount();

   for (int h = 0; h < hruCount; h++)
      {
      HRU *pHRU = GetHRU(h);
      int neighborCount = 0;
      if (pHRU->m_elevation == m_minElevation)
         pHRU->m_lowestElevationHRU = true;
      for (int k = 0; k < hruCount; k++) // go back through and add neighbor pointers to the neighbor array.
         {
         HRU *pNeighborHRU = GetHRU(k);
         ASSERT(pNeighborHRU != NULL);
         if (h != k)//don't add the current hru to its own neighbor array
            {
            if ((pNeighborHRU->m_demRow == pHRU->m_demRow - 1 || pNeighborHRU->m_demRow == pHRU->m_demRow || pNeighborHRU->m_demRow == pHRU->m_demRow + 1)
               && pNeighborHRU->m_demCol == pHRU->m_demCol - 1)
               {
               neighborCount++;
               pHRU->m_neighborArray.Add(pNeighborHRU);
               }

            else if ((pNeighborHRU->m_demRow == pHRU->m_demRow - 1 || pNeighborHRU->m_demRow == pHRU->m_demRow + 1)
               && pNeighborHRU->m_demCol == pHRU->m_demCol)
               {
               neighborCount++;
               pHRU->m_neighborArray.Add(pNeighborHRU);
               }

            else if ((pNeighborHRU->m_demRow == pHRU->m_demRow - 1 || pNeighborHRU->m_demRow == pHRU->m_demRow || pNeighborHRU->m_demRow == pHRU->m_demRow + 1)
               && pNeighborHRU->m_demCol == pHRU->m_demCol + 1)
               {
               neighborCount++;
               pHRU->m_neighborArray.Add(pNeighborHRU);
               }
            }
         }//end of loop through neighbors, for this HRU.  Next, allocate the water flux

      ASSERT(neighborCount < 9);
      int layerCount = pHRU->GetPoolCount();
      for (int i = 0; i < layerCount; i++)                  //Assume all other layers include exchange only in upward and downward directions
         {
         HRUPool *pHRUPool = pHRU->GetPool(i);

         pHRUPool->m_waterFluxArray.SetSize(neighborCount + 7);

         for (int i = 0; i < neighborCount + 7; i++)
            pHRUPool->m_waterFluxArray[i] = 0.0f;

         }
      }

   CString msg;
   msg.Format(_T("Flow is Going Grid!!: %i Catchments, %i HRUs built from %i polygons.  %i HRUs were associated with Catchments"), (int)m_catchmentArray.GetSize(), hruCount, polyCount, foundHRUs);
   Report::Log(msg);
   return true;
   }


bool FlowModel::BuildCatchmentsFromQueries(void)
   {
   // generate aggregates - these are the spatial units for running catchment-based models.  Two types of 
   // aggregates exist - catchments, and within catchments, collections of HRU's
   //
   // In both cases, the process is:
   //
   // 1) generate queries that indicate unique HRU poly collections, and unique Catchment poly collections.
   //    For Catchments, these are based on the catchment aggregation column(s), for HRUs, these are
   //    based on the catchment AND HRU columns, since HRU's are subsets of catchments
   // 2) iterate through the catchment layer polygons, identifying aggregates that represent HRUs and catchmentn
   //
   // Basic idea is that an aggregate is a collection of polygons that have the same attribute values
   // for the given field(s).  For example, if the coverage contains a HYDRO_ID value and that was specified 
   // as the aggregate col, then unique values of HYDRO_ID would be used to generate the  HRU's

   ParseCols(m_pCatchmentLayer, m_catchmentAggCols, m_colsCatchmentAgg);
   ParseCols(m_pCatchmentLayer, m_hruAggCols, m_colsHruAgg);

   // first, do catchments
   CArray< Query*, Query* > catchmentQueryArray;
   int queryCount = GenerateAggregateQueries(m_pCatchmentLayer, m_colsCatchmentAgg, catchmentQueryArray);

   CArray< Query*, Query* > hruQueryArray;
   int hruQueryCount = GenerateAggregateQueries(m_pCatchmentLayer, m_colsHruAgg, hruQueryArray);

   // temporary allocations
   Catchment **catchments = new Catchment*[queryCount];  // placeholder for catchments
   memset(catchments, 0, queryCount * sizeof(Catchment*));  // initially, all NULL

   m_pCatchmentLayer->SetColNoData(m_colCatchmentCatchID);
   m_pCatchmentLayer->SetColNoData(m_colCatchmentHruID);

   int hruCols = (int)m_colsHruAgg.GetSize();
   VData *hruValueArray = new VData[hruCols];

   int polyCount = m_pCatchmentLayer->GetPolygonCount();
   int hruCount = 0;

   if (m_pCatchmentQuery != NULL)
      polyCount = m_pCatchmentQuery->Select(true);

   // iterate though polygons that satisfy the catchment query.
   for (int i = 0; i < polyCount; i++)
      {
      int polyIndex = m_pCatchmentQuery ? m_pCatchmentLayer->GetSelection(i) : i;

      // get hru aggregate values from this poly
      for (int h = 0; h < hruCols; h++)
         {
         bool ok = m_pCatchmentLayer->GetData(polyIndex, m_colsHruAgg[h], hruValueArray[h]);
         if (!ok)
            hruValueArray[h].SetNull();
         }

      // iterate through the catchment aggregate queries, to see if this polygon satisfies any of them
      for (int j = 0; j < queryCount; j++)
         {
         Query *pQuery = catchmentQueryArray.GetAt(j);
         bool result = false;
         bool ok = pQuery->Run(polyIndex, result);

         if (ok && result)  // is this polygon in a the specified Catchment (query?) - yes!
            {
            if (catchments[j] == NULL)    // if first one, creaet a Catchment object for it
               {
               catchments[j] = new Catchment;
               catchments[j]->m_id = j;
               }

            // we know which catchment it's in, next figure out which HRU it is in
            Catchment *pCatchment = catchments[j];
            ASSERT(pCatchment != NULL);

            // store this in the catchment layer
            ASSERT(m_colCatchmentCatchID >= 0);
            m_pCatchmentLayer->SetData(polyIndex, m_colCatchmentCatchID, pCatchment->m_id);

            // does it satisfy an existing HRU for this catchment?
            HRU *pHRU = NULL;
            for (int k = 0; k < pCatchment->GetHRUCount(); k++)
               {
               HRU *_pHRU = pCatchment->GetHRU(k);
               // _pHRU->m_hruValueArray.SetSize(hruCols);
               ASSERT(_pHRU != NULL);
               ASSERT(_pHRU->m_polyIndexArray.GetSize() > 0);

               UINT hruPolyIndex = _pHRU->m_polyIndexArray[0];

               // does the poly being examined match this HRU?
               bool match = true;
               for (int m = 0; m < hruCols; m++)
                  {
                  VData hruValue;
                  m_pCatchmentLayer->GetData(hruPolyIndex, m_colsHruAgg[m], hruValue);
                  int hruVal;
                  m_pCatchmentLayer->GetData(hruPolyIndex, m_colsHruAgg[m], hruVal);

                  if (hruValue != hruValueArray[m])
                     {
                     match = false;
                     break;
                     }

                  }  // end of: for ( m < hruCols )

               if (match)
                  {
                  pHRU = _pHRU;
                  break;
                  }
               }  // end of: for ( k < pCatchment->GetHRUCount() )

            if (pHRU == NULL)  // not found in existing HRU's for this catchment?  Then add...
               {
               pHRU = new HRU;
               AddHRU(pHRU, pCatchment);

               pHRU->m_id = hruCount;
               pHRU->AddPools( this, false );  //m_poolCount, 0, m_initTemperature, false);
               hruCount++;
               }

            ASSERT(m_colCatchmentHruID >= 0);
            m_pCatchmentLayer->SetData(polyIndex, m_colCatchmentHruID, pHRU->m_id);

            pHRU->m_polyIndexArray.Add(polyIndex);
            break;  // done processing queries for this polygon, since we found one that satisifies it, no need to look further
            }  // end of: if ( ok && result )  meaning adding polyIndex to catchment
         }  // end of: for ( j < queryCount )    
      }  // end of: for ( i < polyCount )

   // store catchments
   for (int j = 0; j < queryCount; j++)
      {
      Catchment *pCatchment = catchments[j];

      if (pCatchment != NULL)
         m_catchmentArray.Add(pCatchment);
      }

   // clean up

   delete[] hruValueArray;
   delete[] catchments;

   if (m_pCatchmentQuery != NULL)
      m_pCatchmentLayer->ClearSelection();

   CString msg;
   msg.Format(_T("Flow: %i Catchments, %i HRUs built from %i polygons"), (int)m_catchmentArray.GetSize(), hruCount, polyCount);
   Report::Log(msg);

   return true;
   }


// bool FlowModel::BuildCatchmentsFromLayer( void ) -----------------------------
//
// builds Catchments and HRU's based on HRU ID's contained in the catchment layer
// Note that this method does NOT connect catchments to reaches
//-------------------------------------------------------------------------------

bool FlowModel::BuildCatchmentsFromLayer(void)
   {
   if (this->m_colCatchmentHruID < 0 || m_colCatchmentCatchID < 0)
      return false;

   int polyCount = m_pCatchmentLayer->GetPolygonCount();

   if (m_pCatchmentQuery != NULL)
      polyCount = m_pCatchmentQuery->Select(true);

   int catchID, hruID;
   int hruCount = 0;

   CMap< int, int, Catchment*, Catchment* > catchmentMap;

   // iterate though polygons that satisfy the catchment query.
   for (int i = 0; i < polyCount; i++)
      {
      int polyIndex = m_pCatchmentQuery ? m_pCatchmentLayer->GetSelection(i) : i;

      bool ok = m_pCatchmentLayer->GetData(polyIndex, m_colCatchmentCatchID, catchID);
      if (!ok)
         continue;

      ok = m_pCatchmentLayer->GetData(polyIndex, m_colCatchmentHruID, hruID);
      if (!ok)
         continue;

      Catchment *pCatchment = NULL;
      ok = catchmentMap.Lookup(catchID, pCatchment) ? true : false;
      if (!ok)
         {
         pCatchment = AddCatchment(catchID, NULL);
         catchmentMap.SetAt(catchID, pCatchment);
         }

      // catchment covered, now do HRU's
      HRU *pHRU = NULL;
      for (int j = 0; j < pCatchment->GetHRUCount(); j++)
         {
         HRU *_pHRU = pCatchment->GetHRU(j);

         if (_pHRU->m_id == hruID)
            {
            pHRU = _pHRU;
            break;
            }
         }

      if (pHRU == NULL)
         {
         pHRU = new HRU;
         AddHRU(pHRU, pCatchment);
         // m_hruArray.Add(pHRU);
         pHRU->AddPools(this, false ); //m_poolCount, 0, m_initTemperature, false);
         pHRU->m_id = hruCount;
         hruCount++;
         }

      pHRU->m_polyIndexArray.Add(polyIndex);
      }  // end of:  for ( i < polyCount )

   CString msg;
   msg.Format(_T("Flow: %i Catchments, %i HRUs loaded from %i polygons"), (int)m_catchmentArray.GetSize(), hruCount, polyCount);
   Report::Log(msg);

   return true;
   }


int FlowModel::BuildCatchmentFromPolygons(Catchment *pCatchment, int polyArray[], int polyCount)
   {
   // generate aggregates - these are the spatial units for running catchment-based models.  Two types of 
   // aggregates exist - catchments, and within catchments, collections of HRU's
   //
   // In both cases, the process is:
   //
   // 1) generate queries that indicate unique HRU poly collections, and unique Catchment poly collections.
   //    For Catchments, these are based on the catchment aggregation column(s), for HRUs, these are
   //    based on the catchment AND HRU columns, since HRU's are subsets of catchments
   // 2) iterate through the catchment layer polygons, identifying aggregates that represent HRUs and catchmentn
   //
   // Basic idea is that an aggregate is a collection of polygons that have the same attribute values
   // for the given field(s).  For example, if the coverage contains a HYDRO_ID value and that was specified 
   // as the aggregate col, then unique values of HYDRO_ID would be used to generate the  HRU's

   //ParseCols( m_pCatchmentLayer, m_hruAggCols, m_colsHruAgg ); not needed, already done in prior Buildxxx()

   CArray< Query*, Query* > hruQueryArray;   // will contain queries for all possible unique combination of HRU attributes
   int hruQueryCount = GenerateAggregateQueries(m_pCatchmentLayer, m_colsHruAgg, hruQueryArray);

   int hruCols = (int)m_colsHruAgg.GetSize();     // number of cols referenced in the queries
   VData *hruValueArray = new VData[hruCols];    // will store field values during queries

   int hruCount = 0;

   // iterate though polygons
   for (int i = 0; i < polyCount; i++)
      {
      int polyIndex = polyArray[i];

      // get hru aggregate values from this poly
      for (int h = 0; h < hruCols; h++)
         {
         bool ok = m_pCatchmentLayer->GetData(polyIndex, m_colsHruAgg[h], hruValueArray[h]);
         if (!ok)
            hruValueArray[h].SetNull();
         }

      // store the catchment ID in the poly 
      ASSERT(m_colCatchmentCatchID >= 0);
      m_pCatchmentLayer->SetData(polyIndex, m_colCatchmentCatchID, pCatchment->m_id);

      // does it satisfy an existing HRU for this catchment?
      HRU *pHRU = NULL;
      for (int k = 0; k < pCatchment->GetHRUCount(); k++)
         {
         HRU *_pHRU = pCatchment->GetHRU(k);
         // _pHRU->m_hruValueArray.SetSize(hruCols);
         ASSERT(_pHRU != NULL);
         ASSERT(_pHRU->m_polyIndexArray.GetSize() > 0);
         UINT hruPolyIndex = _pHRU->m_polyIndexArray[0];

         // does the poly being examined match this HRU?
         bool match = true;
         for (int m = 0; m < hruCols; m++)
            {
            VData hruValue;
            m_pCatchmentLayer->GetData(hruPolyIndex, m_colsHruAgg[m], hruValue);
            int hruVal;
            m_pCatchmentLayer->GetData(hruPolyIndex, m_colsHruAgg[m], hruVal);

            if (hruValue != hruValueArray[m])
               {
               match = false;
               break;
               }
            }  // end of: for ( m < hruCols )

         if (match)
            {
            pHRU = _pHRU;
            break;
            }
         }  // end of: for ( k < pCatchment->GetHRUCount() )

      if (pHRU == NULL)  // not found in existing HRU's for this catchment?  Then add...
         {
         pHRU = new HRU;
         AddHRU(pHRU, pCatchment);

         pHRU->m_id = 10000 + m_numHRUs;
         m_numHRUs++;
         pHRU->AddPools(this, /*m_poolCount, 0, m_initTemperature,*/ false);
         hruCount++;
         }

      ASSERT(m_colCatchmentHruID >= 0);
      m_pCatchmentLayer->SetData(polyIndex, m_colCatchmentHruID, pHRU->m_id);

      pHRU->m_polyIndexArray.Add(polyIndex);
      }  // end of: for ( i < polyCount )

   // clean up
   delete[] hruValueArray;

   //CString msg;
   //msg.Format( _T("Flow: Rebuild: %i HRUs built from %i polygons" ), hruCount, polyCount ); 
   //Report::LogInfo(msg);
   return hruCount;
   }



int FlowModel::ParseCols(MapLayer *pLayer /*in*/, CString &aggCols /*in*/, CUIntArray &colsAgg /*out*/)
   {
   // parse columns
   TCHAR *buffer = new TCHAR[aggCols.GetLength() + 1];
   TCHAR *nextToken = NULL;
   lstrcpy(buffer, (LPCTSTR)aggCols);

   TCHAR *col = _tcstok_s(buffer, _T(","), &nextToken);

   int colCount = 0;

   while (col != NULL)
      {
      int _col = pLayer->GetFieldCol(col);

      if (_col < 0)
         {
         CString msg(_T("Flow: Aggregation column "));
         msg += col;
         msg += _T(" not found - it will be ignored");
         Report::ErrorMsg(msg);
         }
      else
         {
         colsAgg.Add(_col);
         colCount++;
         }

      col = _tcstok_s(NULL, _T(","), &nextToken);

      }

   delete[] buffer;

   return colCount;
   }



int FlowModel::GenerateAggregateQueries(MapLayer *pLayer /*in*/, CUIntArray &colsAgg /*in*/, CArray< Query*, Query* > &queryArray /*out*/)
   {
   queryArray.RemoveAll();

   int colCount = (int)colsAgg.GetSize();

   if (colCount == 0)    // no column specified, no queries generatated
      return 0;

   // have columns, figure possible queries
   // for each column, get all possible values of the attributes in that column
   PtrArray< CStringArray > colStrArrays;  // one entry for each column, each entry has unique field values

   for (int i = 0; i < colCount; i++)
      {
      CStringArray *attrValArray = new CStringArray;
      if (pLayer->GetUniqueValues(colsAgg[i], *attrValArray) > 0)
         colStrArrays.Add(attrValArray);
      else
         delete attrValArray;

      }

   // now generate queries.  ASSUME AT MOST TWO COLUMNS FOR NOW!!!!  this needs to be improved!!!
   ASSERT(colCount <= 2);

   if (colCount == 1)
      {
      CStringArray *attrValArray = colStrArrays[0];

      LPCTSTR colName = pLayer->GetFieldLabel(colsAgg[0]);

      for (int j = 0; j < attrValArray->GetSize(); j++)
         {
         CString queryStr;
         queryStr.Format(_T("%s = %s"), colName, (LPCTSTR)attrValArray->GetAt(j));
         /*
         if ( m_catchmentQuery.GetLength() > 0 )
            {
            queryStr += _T(" and ");
            queryStr += m_catchmentQuery;
            } */

         ASSERT(m_pCatchmentQE != NULL);
         Query *pQuery = m_pCatchmentQE->ParseQuery(queryStr, 0, _T("Flow Aggregate Query"));

         if (pQuery == NULL)
            {
            CString msg(_T("Flow: Error parsing Aggregation query: '"));
            msg += queryStr;
            msg += _T("' - the query will be ignored");
            Report::ErrorMsg(msg);
            }
         else
            queryArray.Add(pQuery);
         }
      }

   else if (colCount == 2)
      {
      CStringArray *attrValArray0 = colStrArrays[0];
      CStringArray *attrValArray1 = colStrArrays[1];

      LPCTSTR colName0 = pLayer->GetFieldLabel(colsAgg[0]);
      LPCTSTR colName1 = pLayer->GetFieldLabel(colsAgg[1]);

      for (int j = 0; j < attrValArray0->GetSize(); j++)
         {
         for (int i = 0; i < attrValArray1->GetSize(); i++)
            {
            CString queryStr;
            queryStr.Format(_T("%s = %s and %s = %s"), colName0, (LPCTSTR)attrValArray0->GetAt(j), colName1, (LPCTSTR)attrValArray1->GetAt(i));
            /*
            if ( m_catchmentQuery.GetLength() > 0 )
               {
               queryStr += _T(" and ");
               queryStr += m_catchmentQuery;
               } */

            ASSERT(m_pCatchmentQE != NULL);
            Query *pQuery = m_pCatchmentQE->ParseQuery(queryStr, 0, _T("Flow Aggregate Query"));

            if (pQuery == NULL)
               {
               CString msg(_T("Flow: Error parsing Aggregation query: '"));
               msg += queryStr;
               msg += _T("' - the query will be ignored");
               Report::ErrorMsg(msg);
               }
            else
               queryArray.Add(pQuery);
            }
         }
      }

   return (int)queryArray.GetSize();
   }



bool FlowModel::ConnectCatchmentsToReaches(void)
   {
   // This method determines which reach, if any, each catchment is connected to.  It does this by
   // iterate through the catchments, relating each catchment with it's associated reach.  We assume that
   // each polygon has a field indicated by the catchment join col that contains the index of the 
   // associated stream reach.

   INT_PTR catchmentCount = m_catchmentArray.GetSize();
   INT_PTR reachCount = m_reachArray.GetSize();

   int catchmentJoinID = 0;

   int unconnectedCatchmentCount = 0;
   int unconnectedReachCount = 0;
   //int reRoutedCatchmentCount    = 0;
   //int reRoutedAndLost           = 0;

   // iterate through each catchment, getting the associated field value that
   // maps into the stream coverage
   for (INT_PTR i = 0; i < catchmentCount; i++)
      {
      Catchment *pCatchment = m_catchmentArray[i];
      pCatchment->m_pReach = NULL;

      // have catchment, use the IDU associated with the first HRU in catchment to get
      // the value stored in the catchment join col in the catchment layer.  This is a lookup
      // key - if the corresponding value is found in the reach join col in the stream coverage,
      // we connect this catchment to the reach with the matching join ID
    //  ASSERT( pCatchment->GetHRUCount() > 0 );

      if (pCatchment->GetHRUCount() > 0)
         {
         HRU *pHRU = pCatchment->GetHRU(0);
         m_pCatchmentLayer->GetData(pHRU->m_polyIndexArray[0], m_colCatchmentJoin, catchmentJoinID);

         // look for this catchID in the reaches
         for (INT_PTR j = 0; j < reachCount; j++)
            {
            Reach *pReach = m_reachArray[j];

            int streamJoinID = -1;
            m_pStreamLayer->GetData(pReach->m_polyIndex, this->m_colStreamJoin, streamJoinID);

            // try to join the stream and catchment join cols
            if (streamJoinID == catchmentJoinID)       // join col values match?
               {
               pCatchment->m_pReach = pReach;               // indicate that the catchment is connected to this reach
               pReach->m_catchmentArray.Add(pCatchment);  // add this catchment to the reach's catchment array
               break;
               }
            }

         if (pCatchment->m_pReach == NULL)
            unconnectedCatchmentCount++;
         }
      }

   /*
// check for truncated stream orders.  Basic idea is to scan catchments look for connections
// to stream reaches of order < m_minStreamOrder.  When found, re-route the catchment to
// the first downstream reach that is >= m_minStreamOrder.
if ( m_minStreamOrder > 1 )
   {
   for ( INT_PTR i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = m_catchmentArray[ i ];
      Reach *pReach = pCatchment->m_pReach;

      // have reach, check stream order
      if ( pReach && pReach->m_streamOrder < m_minStreamOrder )
         {
         Reach *pNewReach = pReach;

         // this one should be rerouted, so do it
         while ( pNewReach && pNewReach->m_streamOrder < m_minStreamOrder )
            {
            ReachNode *pDownNode = pNewReach->GetDownstreamNode( true );
            pNewReach = (Reach*) pDownNode;
            }

         if ( pNewReach == NULL )   // end of tree found?
            {
            pCatchment->m_pReach = NULL;
            reRoutedAndLost++;
            }
         else
            {
            pCatchment->m_pReach = pNewReach;
            pNewReach->m_catchmentArray.Add( pCatchment );
            reRoutedCatchmentCount++;
            }
         }  // end of:  if ( pReach->m_streamOrder < m_minStreamOrder )
      }  // end of: if ( pReach
   }

   */

   for (INT_PTR j = 0; j < reachCount; j++)
      {
      Reach *pReach = m_reachArray[j];

      if (pReach->m_catchmentArray.GetSize() > 1)
         Report::Log(_T("Flow: More than one catchment attached to a reach!"));

      if (pReach->m_catchmentArray.GetSize() == 0)
         unconnectedReachCount++;
      }

   // populate the stream layers "CATCHID" field with the reach's catchment ID (if any)
   ////////////////////////////////////////  Is this really needed??? JPB 2/13
   int colCatchmentID = -1;
   this->CheckCol(this->m_pStreamLayer, colCatchmentID, _T("CATCHID"), TYPE_INT, CC_AUTOADD);

   for (INT_PTR j = 0; j < reachCount; j++)
      {
      Reach *pReach = m_reachArray[j];
      if (pReach->m_catchmentArray.GetSize() > 0)
         {
         Catchment *pCatchment = pReach->m_catchmentArray[0];
         m_pStreamLayer->SetData(pReach->m_polyIndex, colCatchmentID, pCatchment->m_id);
         }
      }
   ///////////////////////////////////////

   CString msg;
   msg.Format(_T("Flow: Topology established for %i catchments and %i reaches.  %i catchments are not connected to any reach, %i reaches are not connected to any catchments. "), (int)m_catchmentArray.GetSize(), (int)m_reachArray.GetSize(), unconnectedCatchmentCount, unconnectedReachCount);
   Report::Log(msg);
   return true;
   }


bool FlowModel::InitReservoirs(void)
   {
   if (m_colResID < 0)
      return false;

   // iterate through reach network, looking for reservoir ID's...
   int reachCount = m_pStreamLayer->GetRecordCount();

   for (int i = 0; i < reachCount; i++)
      {
      int resID = -1;
      m_pStreamLayer->GetData(i, m_colResID, resID);

      if (resID > 0)
         {
         // found a reach with an associated reservoir
         // we will need to fix up the Reservoirs reach pointers,
         // and associated Reach's pRes pointer.
         Reservoir *pRes = FindReservoirFromID(resID);

         if (pRes == NULL)
            {
            CString msg;
            msg.Format(_T("Flow: Unable to find reservoir ID '%i' in InitRes()."), resID);
            Report::LogError(msg);
            }
         else
            {
            Reach *pReach = (Reach*)m_reachTree.GetReachNodeFromPolyIndex(i);

            // get the reach assocated with this reservoir ID
            // If found, set reach and reservoir to point to each other.
            if (pReach != NULL)
               {
               pRes->m_pDownstreamReach = pReach;    // note: this will be moved downstream below.
               pReach->m_pReservoir = pRes;   ///????
               pRes->m_reachArray.Add(pReach); 
               }
            else
               {
               CString msg;
               msg.Format(_T("Flow:  Unable to locate reservoir (id:%i) in stream network"), pRes->m_id);
               Report::LogError(msg);
               }
            }  // end of : else
         }  // end of: if ( resID > 0 )
      }  // end of: for each Reach

   // At this point, we have associated each reservoir with the reach identified in the reach coverage 
   // For each reservoir, follow downstream until the first reach downstream from
   // the reaches associated with the reservoir is found
   int resCount = (int)m_reservoirArray.GetSize();
   int usedResCount = 0;

   for (int i = 0; i < resCount; i++)
      {
      // for each reservoir, move the Reach pointer downstream
      // until a different resID found
      Reservoir *pRes = m_reservoirArray[i];
      ASSERT(pRes != NULL);

      Reach *pReach = pRes->m_pDownstreamReach;   // initially , this points to a reach "in" the reservoir, if any

      if (pReach != NULL)   // any associated reachs?
         {
         //if ( pRes->m_reachArray.GetSize() > 0 )
         //Reach *pReach = pRes->m_reachArray[0];   // initially, this points to a reach "in" the reservoir
         ReachNode *pNode = pReach;

         // traverse down until this node's resID != this one
         while (pNode != NULL)
            {
            ReachNode *pDown = pNode->GetDownstreamNode(true);

            if (pDown == NULL)  // root of tree?
               break;

            int idu = pDown->m_polyIndex;
            int resID = -1;
            m_pStreamLayer->GetData(idu, m_colResID, resID);

            // do we see no reservoir or a different reservoir? then quit the loop
            if (resID != pRes->m_id)
               break;
            else
               pNode = pDown;
            }

         // at this point, pNode points to the last matching downstream reach.
         // fix up the pointers
         if (pNode != NULL)
            {
            Reach *pDownstreamReach = GetReachFromNode(pNode);
            pRes->m_pDownstreamReach->m_pReservoir = NULL;
            pRes->m_pDownstreamReach = pDownstreamReach;
            pDownstreamReach->m_pReservoir = pRes;
            usedResCount++;

            //Initialize reservoir outflow to minimum outflow
            float minOutflow = pRes->m_minOutflow;
            pRes->m_outflow = minOutflow;

            //Initialize gate specific flow variables to zero
            pRes->m_maxPowerFlow = 0;
            pRes->m_minPowerFlow = 0;
            pRes->m_powerFlow = 0;

            pRes->m_maxRO_Flow = 0;
            pRes->m_minRO_Flow = 0;
            pRes->m_RO_flow = 0;

            pRes->m_maxSpillwayFlow = 0;
            pRes->m_minSpillwayFlow = 0;
            pRes->m_spillwayFlow = 0;

            //LoadTable(m_pFlowModel->m_path + pRes->m_dir + pRes->m_avdir + pRes->m_areaVolCurveFilename, (DataObj**)&(pRes->m_pAreaVolCurveTable), 0);

            // For all Reservoir Types but ControlPointControl
            //if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_RiverRun)
            //   {
            //   LoadTable(m_pFlowModel->m_path + pRes->m_dir + pRes->m_redir + pRes->m_releaseCapacityFilename, (DataObj**)&(pRes->m_pCompositeReleaseCapacityTable), 0);
            //   LoadTable(m_pFlowModel->m_path + pRes->m_dir + pRes->m_redir + pRes->m_RO_CapacityFilename, (DataObj**)&(pRes->m_pRO_releaseCapacityTable), 0);
            //   LoadTable(m_pFlowModel->m_path + pRes->m_dir + pRes->m_redir + pRes->m_spillwayCapacityFilename, (DataObj**)&(pRes->m_pSpillwayReleaseCapacityTable), 0);
            //   }
            //
            //// Not Needed for RiverRun Reservoirs
            //if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_CtrlPointControl)
            //   {
            //   LoadTable(m_pFlowModel->m_path + pRes->m_dir + pRes->m_rpdir + pRes->m_rulePriorityFilename, (DataObj**)&(pRes->m_pRulePriorityTable), 1);
            //
            //   if (pRes->m_reservoirType == ResType_FloodControl)
            //      {
            //      LoadTable(m_pFlowModel->m_path + pRes->m_dir + pRes->m_rcdir + pRes->m_ruleCurveFilename, (DataObj**)&(pRes->m_pRuleCurveTable), 0);
            //      LoadTable(m_pFlowModel->m_path + pRes->m_dir + pRes->m_rcdir + pRes->m_bufferZoneFilename, (DataObj**)&(pRes->m_pBufferZoneTable), 0);
            //
            //      //Load table with ResSIM outputs
            //      //LoadTable( pRes->m_dir+_T("Output_from_ResSIM\\")+pRes->m_ressimFlowOutputFilename,    (DataObj**) &(pRes->m_pResSimFlowOutput),    0 );
            //      LoadTable(pRes->m_dir + _T("Output_from_ResSIM\\") + pRes->m_ressimRuleOutputFilename, (DataObj**)&(pRes->m_pResSimRuleOutput), 1);
            //      }
            //   pRes->ProcessRulePriorityTable();
            //   }            
            }
         } // end of: if ( pReach != NULL )

      pRes->InitDataCollection(this);    // set up data object and add as output variable      
      }  // end of: for each Reservoir

   //if ( usedResCount > 0 )
   //   this->AddReservoirLayer();

   for (int i = 0; i < m_reservoirArray.GetSize(); i++)
      {
      Reservoir *pRes = m_reservoirArray.GetAt(i);
      this->AddInputVar(pRes->m_name + _T(" ResMaintenance"), pRes->m_probMaintenance, _T("Probability that the reservoir will need to be taken offline in any year"));
      }

   // allocate any necessary "extra" state variables
   if (m_reservoirSvCount > 1)//then there are extra state variables..
      {
      int resCount = (int)m_reservoirArray.GetSize();
      for (int i = 0; i < resCount; i++)
         {
         Reservoir *pRes = m_reservoirArray[i];
         ASSERT(pRes != NULL);
         pRes->AllocateStateVars(m_reservoirSvCount - 1);
         }
      }

   CString msg;
   msg.Format(_T("Flow: Loaded %i reservoirs (%i were disconnected from the reach network and ignored)"), usedResCount, resCount - usedResCount);
   Report::Log(msg);

   return true;
   }


bool FlowModel::InitRunReservoirs(EnvContext *pEnvContext)
   {
   int resCount = (int)m_reservoirArray.GetSize();
   int usedResCount = 0;
   EnvModel *pEnvModel = pEnvContext->pEnvModel;

   for (int i = 0; i < resCount; i++)
      {
      // for each reservoir, move the Reach pointer downstream
      // until a different resID found
      Reservoir *pRes = m_reservoirArray[i];
      ASSERT(pRes != NULL);

      Reach *pReach = pRes->m_pDownstreamReach;   // this points to the first reach downstream of the reservoir
      
      if (pReach != NULL)   // any downstream reache?  if not, this Reservoir is ignored
         {
         //Initialize reservoir outflow to minimum outflow
         float minOutflow = pRes->m_minOutflow;
         pRes->m_outflow = minOutflow;

         //Initialize gate specific flow variables to zero
         pRes->m_maxPowerFlow = 0;
         pRes->m_minPowerFlow = 0;
         pRes->m_powerFlow = 0;

         pRes->m_maxRO_Flow = 0;
         pRes->m_minRO_Flow = 0;
         pRes->m_RO_flow = 0;

         pRes->m_maxSpillwayFlow = 0;
         pRes->m_minSpillwayFlow = 0;
         pRes->m_spillwayFlow = 0;

         CString path = this->m_path + pRes->m_dir + pRes->m_avdir + pRes->m_areaVolCurveFilename;

         LoadTable(pEnvModel, path, (DataObj**)&(pRes->m_pAreaVolCurveTable), 0);

         // For all Reservoir Types but ControlPointControl
         if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_RiverRun)
            {
            LoadTable(pEnvModel, this->m_path + pRes->m_dir + pRes->m_redir + pRes->m_releaseCapacityFilename, (DataObj**)&(pRes->m_pCompositeReleaseCapacityTable), 0);
            LoadTable(pEnvModel, this->m_path + pRes->m_dir + pRes->m_redir + pRes->m_RO_CapacityFilename, (DataObj**)&(pRes->m_pRO_releaseCapacityTable), 0);
            LoadTable(pEnvModel, this->m_path + pRes->m_dir + pRes->m_redir + pRes->m_spillwayCapacityFilename, (DataObj**)&(pRes->m_pSpillwayReleaseCapacityTable), 0);
            }

         // Not Needed for RiverRun Reservoirs
         if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_CtrlPointControl)
            {
            LoadTable(pEnvModel, this->m_path + pRes->m_dir + pRes->m_rpdir + pRes->m_rulePriorityFilename, (DataObj**)&(pRes->m_pRulePriorityTable), 1);

            if (pRes->m_reservoirType == ResType_FloodControl)
               {
               LoadTable(pEnvModel, this->m_path + pRes->m_dir + pRes->m_rcdir + pRes->m_ruleCurveFilename, (DataObj**)&(pRes->m_pRuleCurveTable), 0);
               LoadTable(pEnvModel, this->m_path + pRes->m_dir + pRes->m_rcdir + pRes->m_bufferZoneFilename, (DataObj**)&(pRes->m_pBufferZoneTable), 0);

               //Load table with ResSIM outputs
               //LoadTable( pRes->m_dir+_T("Output_from_ResSIM\\")+pRes->m_ressimFlowOutputFilename,    (DataObj**) &(pRes->m_pResSimFlowOutput),    0 );
               //LoadTable(pEnvModel, pRes->m_dir + _T("Output_from_ResSIM\\") + pRes->m_ressimRuleOutputFilename, (DataObj**)&(pRes->m_pResSimRuleOutput), 1);
               }

            pRes->ProcessRulePriorityTable(pEnvModel,this);
            }
         } // end of: if ( pReach != NULL )

      ASSERT(pRes != NULL);
      ASSERT(pRes->m_pResData != NULL);
      ASSERT(pRes->m_pResSimFlowCompare != NULL);
      ASSERT(pRes->m_pResSimRuleCompare != NULL);

      pRes->m_filled = 0;

      pRes->m_pResData->ClearRows();
      //pRes->m_pResMetData->ClearRows();
      pRes->m_pResSimFlowCompare->ClearRows();
      pRes->m_pResSimRuleCompare->ClearRows();

      //pRes->InitDataCollection();    // set up data object and add as output variable      
      }  // end of: for each Reservoir

   //if ( usedResCount > 0 )
   //   this->AddReservoirLayer();

   //CString msg;
   //msg.Format( _T("Flow: Loaded %i reservoirs (%i were disconnected from the reach network and ignored)"), usedResCount, resCount-usedResCount );
   //Report::Log( msg );

   return true;
   }



//--------------------------------------------------------------------------------------------------
// Network simplification process:
//
// basic idea: iterate through the the stream network starting with the leaves and
// working towards the roots.  For each reach, if it doesn't satisfy the stream query
// (meaning it is to be pruned), move its corresponding HRUs to the first downstream 
// that satisfies the query.
//
// The basic idea of simplification is to aggregate Catchments using two distinct methods: 
//   1) combining catchments that meet a set of constraints, typically focused on 
// 
//  returns number of combined catchments
//--------------------------------------------------------------------------------------------------

int FlowModel::SimplifyNetwork(void)
   {
   if (m_pStreamQuery == NULL)
      return -1;

   int startReachCount = (int)m_reachArray.GetSize();
   int startCatchmentCount = (int)m_catchmentArray.GetSize();
   int startHRUCount = 0;

   for (int i = 0; i < (int)m_catchmentArray.GetSize(); i++)
      startHRUCount += m_catchmentArray[i]->GetHRUCount();

   CString msg(_T("Flow: Simplifying Catchments/Reaches: "));

   if (this->m_streamQuery.GetLength() > 0)
      {
      msg += _T("Stream Query: '");
      msg += this->m_streamQuery;
      msg += _T("' ");
      }

   if (this->m_minCatchmentArea > 0)
      {
      CString _msg;
      _msg.Format(_T("Min Catchment Area: %.1g ha, Max Catchment Area: %.1g ha"), (float) this->m_minCatchmentArea / M2_PER_HA, (float) this->m_maxCatchmentArea / M2_PER_HA);
      msg += _msg;
      }

   Report::Log(msg);

   // iterate through the roots of the tree, applying catchment constraints
   try
      {
      // now, starting with the leaves, move down stream, looking for excluded stream reaches
      for (int i = 0; i < m_reachTree.GetRootCount(); i++)
         {
         ReachNode *pRoot = m_reachTree.GetRootNode(i);

         // algorithm:  start with the root.  Simplify the left side, then the right side,
         // recursively with the following rule:
         //   if the current reachnode satifies the query, recurse
         //   otherwise, relocate the HRUs from everything above and including it to the downstream
         //   reach node's catchment

         if (this->m_streamQuery.GetLength() > 0)
            {
            int catchmentCount = (int)m_catchmentArray.GetSize();

            SimplifyTree(pRoot);   // prune leaves based on reach query

            int newCatchmentCount = (int)m_catchmentArray.GetSize();

            CString msg;
            msg.Format(_T("Flow: Pruned leaves for root %i, %i Catchments reduced to %i (%i combined)"), i, catchmentCount, newCatchmentCount, (catchmentCount - newCatchmentCount));
            Report::Log(msg);
            }

         if (this->m_minCatchmentArea > 0 && this->m_maxCatchmentArea > 0)
            {
            int catchmentCount = (int)m_catchmentArray.GetSize();

            ApplyCatchmentConstraints(pRoot);              // apply min area constraints

            int newCatchmentCount = (int)m_catchmentArray.GetSize();

            CString msg;
            msg.Format(_T("Flow: applying area constraints for root %i, %i Catchments reduced to %i (%i combined)"), i, catchmentCount, newCatchmentCount, (catchmentCount - newCatchmentCount));
            Report::Log(msg);
            }
         }
      }

   catch (CException *ex) // Catch all other CException derived exceptions 
      {
      TCHAR info[300];
      if (ex->GetErrorMessage(info, 300) == TRUE)
         {
         CString msg(_T("Exception thrown in Flow during SimplifyTree() "));
         msg += info;
         Report::ErrorMsg(msg);
         }
      ex->Delete(); // Will delete exception object correctly 
      }
   catch (...)
      {
      CString msg(_T("Unknown Exception thrown in Flow during SimplifyTree() "));
      Report::ErrorMsg(msg);
      }

   // update catchment/reach cumulative drainage areas
   // this->PopulateCatchmentCumulativeAreas();

   int finalReachCount = (int) this->m_reachArray.GetSize();
   int finalHRUCount = 0;

   // update catchID and join cols to reflect the simplified network
   m_pCatchmentLayer->SetColData(this->m_colCatchmentJoin, VData(-1), true);
   m_pStreamLayer->SetColData(this->m_colStreamJoin, VData(-1), true);

   float areaTotal = 0.0f;

   for (int i = 0; i < (int)m_catchmentArray.GetSize(); i++)
      {
      Catchment *pCatchment = m_catchmentArray[i];
      finalHRUCount += pCatchment->GetHRUCount();

      for (int j = 0; j < pCatchment->GetHRUCount(); j++)
         {
         HRU *pHRU = pCatchment->GetHRU(j);
         ASSERT(pHRU != NULL);
         areaTotal += pHRU->m_area;

         for (int k = 0; k < (int)pHRU->m_polyIndexArray.GetSize(); k++)
            {
            // write new catchment ID to catchmentID col and catchment join col 
            m_pCatchmentLayer->SetData(pHRU->m_polyIndexArray[k], m_colCatchmentCatchID, pCatchment->m_id);
            m_pCatchmentLayer->SetData(pHRU->m_polyIndexArray[k], m_colCatchmentJoin, pCatchment->m_id);
            }
         }
      }

   // update stream join values
   for (int i = 0; i < this->m_reachArray.GetSize(); i++)
      {
      Reach *pReach = m_reachArray[i];

      if (pReach->m_catchmentArray.GetSize() > 0)
         m_pStreamLayer->SetData(pReach->m_polyIndex, this->m_colStreamJoin, pReach->m_catchmentArray[0]->m_id);
      }

   msg.Format(_T("Flow: Reach Network Simplified from %i to %i Reaches, %i to %i Catchments and %i to %i HRUs"),
      startReachCount, finalReachCount, startCatchmentCount, (int)m_catchmentArray.GetSize(), startHRUCount, finalHRUCount);
   Report::Log(msg);

   return startCatchmentCount - ((int)m_catchmentArray.GetSize());
   }


// returns number of reaches in the simplified tree
int FlowModel::SimplifyTree(ReachNode *pNode)
   {
   if (pNode == NULL)
      return 0;

   // skip phantom nodes - they have no associated catchments (they always succeed)
   if (pNode->IsPhantomNode() || pNode->IsRootNode())
      {
      int leftCount = SimplifyTree(pNode->m_pLeft);
      int rightCount = SimplifyTree(pNode->m_pRight);
      return leftCount + rightCount;
      }

   // does this node satisfy the query?
   ASSERT(pNode->m_polyIndex >= 0);
   bool result = false;
   bool ok = m_pStreamQE->Run(m_pStreamQuery, pNode->m_polyIndex, result);
   Reach *pReach = GetReachFromNode(pNode);
   if ((ok && result) || pReach->m_isMeasured)
      {
      // it does, so we keep this one and move on to the upstream nodes
      int leftCount = SimplifyTree(pNode->m_pLeft);
      int rightCount = SimplifyTree(pNode->m_pRight);
      return 1 + leftCount + rightCount;
      }

   else  // it fails, so this reach and all reaches of above it need to be rerouted to the reach below us
      {  // relocate this and upstream reaches to this nodes down stream pointer
      if (pNode->IsRootNode())    // if this is a root node, nothing required, the whole
         return 0;                  // tree will be deleted  NOTE: NOT IMPLEMENTED YET
      else
         {
         ReachNode *pDownNode = pNode->GetDownstreamNode(true);  // skips phantoms
         Reach *pDownReach = GetReachFromNode(pDownNode);
         if (pDownReach == NULL)
            {
            CString msg;
            msg.Format(_T("Flow: Warning - phantom root node encountered below Reach ID %i. Truncation can not proceed for this tree..."), pNode->m_reachID);
            Report::LogWarning(msg);
            }
         else
            {
            int beforeCatchCount = (int) this->m_catchmentArray.GetSize();
            MoveCatchments(pDownReach, pNode, true);   // moves all catchments for this and above (recursively)
            int afterCatchCount = (int) this->m_catchmentArray.GetSize();

            //CString msg;
            //msg.Format( _T("Flow: Pruning %i catchments that don't satisfy the Simplification query"), (beforeCatchCount-afterCatchCount) );
            //Report::Log( msg );

            RemoveReaches(pNode);   // removes all Reach and ReachNodes above and including this ReachNode
            }

         return 0;
         }
      }
   }


int FlowModel::ApplyCatchmentConstraints(ReachNode *pRoot)
   {
   // applies min/max size constraints on a catchment.  Basic rule is:
   // if the catchment for this reach is too small, combine it with another
   // reach using the following rules:
   //    1) if one of the upstream catchment is below the minimum size, add this reaches
   //       catchment to the upstream reach.  (Note that this works best with a root to leaf search)
   //    2) if there is no upstream capacity, combine this reach with the downstream reach (note
   //       that this will work best with a leaf to root, breadth first search.)

   // iterate through catchments. Assumes cumulative drainage areas are already calculated
   int removeCount = 1;
   int pass = 0;

   while (removeCount > 0 && pass < 5)
      {
      removeCount = 0;
      for (int i = 0; i < (int)m_reachArray.GetSize(); i++)
         {
         Reach *pReach = m_reachArray[i];
         int removed = ApplyAreaConstraints(pReach);

         // if this reach was removed, then move i back one to compensate
        // i -= removed;

         removeCount += removed;
         }

      pass++;

      CString msg;
      msg.Format(_T("Flow: Applying catchment constraints (pass %i) - %i catchments consolidated"), pass, removeCount);

      Report::Log(msg);
      }

   return pass;
   }


// return 0 if no change, 1 if Reach is altered
int FlowModel::ApplyAreaConstraints(ReachNode *pNode)
   {
   // this method examines the current reach.  if the contributing area of the reach is too small, try to combine it with another catchments
   // Basic rule is:
   // if the catchment for this reach is too small, combine it with another
   // reach using the following rules:
   //    1) if one of the upstream catchment is below the minimum size, add this reaches
   //       catchment to the upstream reach.  (Note that this works best with a root to leaf search)
   //    2) if there is no upstream capacity, combine this reach with the downstream reach (note
   //       that this will work best with a leaf to root, breadth first search.)
   if (pNode == NULL)
      return 0;

   Reach *pReach = GetReachFromNode(pNode);

   if (pReach == NULL)      // no reach exist no action required
      return 0;

   // does this Catchment  satisfy the min area constraint?  If so, we are done.
   float catchArea = pReach->GetCatchmentArea();

   if (catchArea > m_minCatchmentArea)
      return 0;

   // catchment area for this reach is too small, combine it.
   // Step one, look above, left 
   ReachNode *pLeft = pNode->m_pLeft;
   while (pLeft != NULL && GetReachFromNode(pLeft) == NULL)
      pLeft = pLeft->m_pLeft;

   // does a left reach exist?  if so, check area and combine if minimum not satisfied
   if (pLeft != NULL)
      {
      Reach *pLeftReach = GetReachFromNode(pLeft);
      ASSERT(pLeftReach != NULL);
      if (!pLeftReach->m_isMeasured)
         {
         // is there available capacity upstream?
         if ((pLeftReach->GetCatchmentArea() + catchArea) <= m_maxCatchmentArea)
            {
            MoveCatchments(pReach, pLeft, false);   // move the upstream reach to this one, no recursion
           // RemoveReach( pLeft ); //leave reaches in the network  KBV
            RemoveCatchment(pLeft);
            return 1;
            }
         }
      }

   // look above, right
   ReachNode *pRight = pNode->m_pRight;
   while (pRight != NULL && GetReachFromNode(pRight) == NULL)
      pRight = pRight->m_pRight;

   // does a right reach exist?  if so, check area and combine if minimum not satisfied
   if (pRight != NULL)
      {
      Reach *pRightReach = GetReachFromNode(pRight);
      ASSERT(pRightReach != NULL);
      if (!pRightReach->m_isMeasured)
         {
         // is there available capacity upstream?
         if ((pRightReach->GetCatchmentArea() + catchArea) <= m_maxCatchmentArea)
            {
            MoveCatchments(pReach, pRight, false);   // move the upstream reach to this one, no recursion
           // RemoveReach( pRight );  //leave reaches in the network KBV
            RemoveCatchment(pRight);
            return 1;
            }
         }
      }

   // no capacity upstream, how about downstream?
   ReachNode *pDown = pNode->m_pDown;
   while (pDown != NULL && GetReachFromNode(pDown) == NULL)
      pDown = pDown->m_pDown;

   // does a down reach exist?  if so, check area and combine if minimum not satisfied
   if (pDown != NULL && pReach->m_isMeasured == false)
      {
      Reach *pDownReach = GetReachFromNode(pDown);
      Reach *pReach = GetReachFromNode(pNode);
      ASSERT(pDownReach != NULL);

      // is there available capacity upstream?
    //  if ( ( pDownReach->m_cumUpstreamArea + catchArea ) <= m_maxCatchmentArea )

      if ((pDownReach->GetCatchmentArea() + catchArea) <= m_maxCatchmentArea)
         {
         MoveCatchments(pDownReach, pNode, false);   // move from this reach to the downstream reach
         //RemoveReach( pNode ); //leave reaches in the network  KBV
         RemoveCatchment(pNode);
         return 1;
         }
      }

   // else, no capacity anywhere adjacent to this reach, so give up
   return 0;
   }





float FlowModel::PopulateCatchmentCumulativeAreas(void)
   {
   float area = 0;
   for (int i = 0; i < m_reachTree.GetRootCount(); i++)
      {
      ReachNode *pRoot = m_reachTree.GetRootNode(i);
      area += PopulateCatchmentCumulativeAreas(pRoot);
      }

   return area;
   }


// updates internal variables and (if columns are defined) the stream and catchment
// coverages with cumulative values.  Assumes internal catchment areas have already been 
// determined.

float FlowModel::PopulateCatchmentCumulativeAreas(ReachNode *pNode)
   {
   if (pNode == NULL)
      return 0;

   float leftArea = PopulateCatchmentCumulativeAreas(pNode->m_pLeft);
   float rightArea = PopulateCatchmentCumulativeAreas(pNode->m_pRight);

   Reach *pReach = GetReachFromNode(pNode);

   if (pReach != NULL)
      {
      float area = 0;

      for (int i = 0; i < (int)pReach->m_catchmentArray.GetSize(); i++)
         area += pReach->m_catchmentArray[i]->m_area;

      area += leftArea;
      area += rightArea;

      // write to coverages
      if (m_colStreamCumArea >= 0)
         m_pStreamLayer->SetData(pReach->m_polyIndex, m_colStreamCumArea, area);

      if (m_colCatchmentCumArea >= 0)
         {
         for (int i = 0; i < (int)pReach->m_catchmentArray.GetSize(); i++)
            {
            Catchment *pCatch = pReach->m_catchmentArray[i];
            SetCatchmentData(pCatch, m_colCatchmentCumArea, area);
            }
         }

      pReach->m_cumUpstreamArea = area;
      return area;
      }

   return leftArea + rightArea;
   }



// FlowModel::MoveCatchments() takes the catchment HRUs associated wih the "from" node and moves them to the
//    "to" reach, and optionally repeats recursively up the network.
int FlowModel::MoveCatchments(Reach *pToReach, ReachNode *pFromNode, bool recurse)
   {
   if (pFromNode == NULL || pToReach == NULL)
      return 0;

   if (pFromNode->IsPhantomNode())   // for phantom nodes, always recurse up
      {
      int leftCount = MoveCatchments(pToReach, pFromNode->m_pLeft, recurse);
      int rightCount = MoveCatchments(pToReach, pFromNode->m_pRight, recurse);
      return leftCount + rightCount;
      }

   Reach *pFromReach = GetReachFromNode(pFromNode);

   int fromCatchmentCount = (int)pFromReach->m_catchmentArray.GetSize();

   // SPECIAL CASE:  fromCatchmentCount = 0, then there are no catchments in the
   // upstream reach, so just rec

   // if the from reach is measured, we don't combine it's catchments.
   // or, if the from reach has no catchments, there isn't anything to do either
   if (pFromReach->m_isMeasured == false && fromCatchmentCount > 0)
      {
      // get the Catchments associated with this reach and combine them into the
      // first catchment in the "to" reach catchment list
      int toCatchmentCount = (int)pToReach->m_catchmentArray.GetSize();

      Catchment *pToCatchment = NULL;

      if (toCatchmentCount == 0)//KELLIE
         {
         m_numReachesNoCatchments++;
         pToCatchment = AddCatchment(10000 + m_numReachesNoCatchments, pToReach);
         }
      else
         pToCatchment = pToReach->m_catchmentArray[0];

      ASSERT(pToCatchment != NULL);

      for (int i = 0; i < fromCatchmentCount; i++)
         {
         Catchment *pFromCatchment = pFromReach->m_catchmentArray.GetAt(i);

         pToReach->m_cumUpstreamArea = pFromReach->m_cumUpstreamArea + pToReach->GetCatchmentArea();
         pToReach->m_cumUpstreamLength = pFromReach->m_cumUpstreamLength + pToReach->m_length;

         CombineCatchments(pToCatchment, pFromCatchment, true);   // deletes the original catchment
         }

      // add the from reach length to the to reach.  Note: this assumes the from reach will removed from Flow
      // pToReach->m_length += pFromReach->m_length;     // JPB KBV

      // remove dangling Catchment pointers from the From reach
      pFromReach->m_catchmentArray.RemoveAll();       // only ptrs deleted, not the catchments
      }

   // recurse if specified
   if (recurse && !pFromReach->m_isMeasured) // we don't want to recurse farther if we have find a reach for which we are saving model output for comparison with measurements
      {
      ReachNode *pLeft = pFromNode->m_pLeft;
      ReachNode *pRight = pFromNode->m_pRight;

      int leftCount = MoveCatchments(pToReach, pLeft, recurse);
      int rightCount = MoveCatchments(pToReach, pRight, recurse);

      return fromCatchmentCount + leftCount + rightCount;
      }
   else if (recurse && pFromReach->m_isMeasured) // so skip the measured reach, and continue recursion above it
      {
      ReachNode *pLeft = pFromNode->m_pLeft;
      ReachNode *pRight = pFromNode->m_pRight;

      int leftCount = MoveCatchments(pFromReach, pLeft, recurse);
      int rightCount = MoveCatchments(pFromReach, pRight, recurse);

      return fromCatchmentCount + leftCount + rightCount;
      }
   else
      return fromCatchmentCount;
   }


// Combines the two catchments into the target catchment, copying/combining HRUs in the process.
// Depending on the deleteSource flag, the source catchment is optionally deleted

int FlowModel::CombineCatchments(Catchment *pTargetCatchment, Catchment *pSourceCatchment, bool deleteSource)
   {
   // get list of polygons to move from soure to target
   int hruCount = pSourceCatchment->GetHRUCount();

   CArray< int > polyArray;

   for (int i = 0; i < hruCount; i++)
      {
      HRU *pHRU = pSourceCatchment->GetHRU(i);

      for (int j = 0; j < pHRU->m_polyIndexArray.GetSize(); j++)
         polyArray.Add(pHRU->m_polyIndexArray[j]);
      }

   // have polygon list built, generate/add to target catchments HRUs
   BuildCatchmentFromPolygons(pTargetCatchment, polyArray.GetData(), (int)polyArray.GetSize());

   //  pTargetCatchment->m_area += pSourceCatchment->m_area;

     // remove from source
     //pSourceCatchment->RemoveAllHRUs();     // this deletes the HRUs
   RemoveCatchmentHRUs(pSourceCatchment);

   if (deleteSource)
      {
      // remove from global list and delete catchment
      for (int i = 0; i < (int)m_catchmentArray.GetSize(); i++)
         {
         if (m_catchmentArray[i] == pSourceCatchment)
            {
            m_catchmentArray.RemoveAt(i);
            break;
            }
         }
      }

   return (int)polyArray.GetSize();
   }


int FlowModel::RemoveCatchmentHRUs(Catchment *pCatchment)
   {
   int count = pCatchment->GetHRUCount();
   // first, remove from FlowModel
   for (int i = 0; i < count; i++)
      {
      HRU *pHRU = pCatchment->GetHRU(i);
      this->m_hruArray.Remove(pHRU);    // deletes the HHRU
      }

   // next, remove pointers from catchment
   pCatchment->m_hruArray.RemoveAll();
   pCatchment->m_area = 0;

   return count;
   }




// removes all reaches and reachnodes, starting with the passed in node
int FlowModel::RemoveReaches(ReachNode *pNode)
   {
   if (pNode == NULL)
      return 0;

   int removed = 0;

   if (pNode->IsPhantomNode() == false)
      {
      if (m_colStreamOrder >= 0)
         {
         int order = 0;
         m_pStreamLayer->GetData(pNode->m_polyIndex, m_colStreamOrder, order);
         m_pStreamLayer->SetData(pNode->m_polyIndex, m_colStreamOrder, -order);
         }

      Reach *pReach = GetReachFromNode(pNode);
      removed++;

      // remove from internal list
      for (int i = 0; i < (int)m_reachArray.GetSize(); i++)
         {
         if (m_reachArray[i] == pReach)
            {
            m_reachArray.RemoveAt(i);    // deletes the Reach
            break;
            }
         }
      }

   // remove from reach tree (note this does not delete the ReachNode, it turns it into a phantom node
   m_reachTree.RemoveNode(pNode, false);    // don't delete upstream nodes, this will occur below

   // recurse
   int leftCount = RemoveReaches(pNode->m_pLeft);
   int rightCount = RemoveReaches(pNode->m_pRight);

   return removed + leftCount + rightCount;
   }


bool FlowModel::RemoveCatchment(ReachNode *pNode)
   {
   Reach *pReach = GetReachFromNode(pNode);

   if (pReach == NULL)    // NULL, phantom, or no associated Reach
      return 0;

   if (pReach->IsPhantomNode())
      return 0;

   if (pReach->IsRootNode())
      return 0;

   // delete any associated catchments
   for (int i = 0; i < (int)pReach->m_catchmentArray.GetSize(); i++)
      {
      Catchment *pCatchment = pReach->m_catchmentArray[i];

      for (int j = 0; j < (int)m_catchmentArray.GetSize(); j++)
         {
         if (m_catchmentArray[i] == pCatchment)
            {
            m_catchmentArray.RemoveAt(i);   // deletes the catchment
            break;
            }
         }
      }
   return true;
   }

// removes this reach and reachnode, converting the node into a phantom node
bool FlowModel::RemoveReach(ReachNode *pNode)
   {
   Reach *pReach = GetReachFromNode(pNode);

   if (pReach == NULL)    // NULL, phantom, or no associated Reach
      return 0;

   if (pReach->IsPhantomNode())
      return 0;

   if (pReach->IsRootNode())
      return 0;

   // delete any associated catchments
   for (int i = 0; i < (int)pReach->m_catchmentArray.GetSize(); i++)
      {
      Catchment *pCatchment = pReach->m_catchmentArray[i];

      for (int j = 0; j < (int)m_catchmentArray.GetSize(); j++)
         {
         if (m_catchmentArray[i] == pCatchment)
            {
            m_catchmentArray.RemoveAt(i);
            break;
            }
         }
      }

   // remove from internal list
   for (int i = 0; i < (int)m_reachArray.GetSize(); i++)
      {
      if (m_reachArray[i] == pReach)
         {
         m_reachArray.RemoveAt(i);    // does NOT delete Reach, just removes it from the list.  The
         break;                         // Reach object is managed by m_reachTree
         }
      }

   // remove from reach tree (actually, this just converts the node to a phantom node)
   m_reachTree.RemoveNode(pNode, false);    // don't delete upstream nodes (no recursion)

   return true;
   }



// load table take a filename, creates a new DataObject of the specified type, and fills it
// with the file-based table
int FlowModel::LoadTable(EnvModel *pEnvModel, LPCTSTR filename, DataObj** pDataObj, int type)
   {
   if (*pDataObj != NULL)
      {
      delete *pDataObj;
      *pDataObj = NULL;
      }

   if (filename == NULL || lstrlen(filename) == 0)
      {
      CString msg(_T("Flow: bad table name specified"));
      if (filename != NULL)
         {
         msg += _T(": ");
         msg += filename;
         }

      Report::LogWarning(msg);
      return 0;
      }

   CString path(filename);
   this->ApplyMacros(pEnvModel,path);

   switch (type)
      {
      case 0:
         *pDataObj = new FDataObj(U_UNDEFINED);
         ASSERT(*pDataObj != NULL);
         break;

      case 1:
         *pDataObj = new VDataObj(U_UNDEFINED);
         ASSERT(*pDataObj != NULL);
         break;

      default:
         *pDataObj = NULL;
         return -1;
      }


   if ((*pDataObj)->ReadAscii(path) <= 0)
      {
      delete *pDataObj;
      *pDataObj = NULL;

      CString msg(_T("Flow: Error reading table: "));
      msg += _T(": ");
      msg += path;
      Report::LogWarning(msg);
      return 0;
      }

   (*pDataObj)->SetName(path);

   return (*pDataObj)->GetRowCount();
   }



bool FlowModel::InitIntegrationBlocks(void)
   {
   // allocates arrays necessary for integration method.
   // For reaches, this allocates a state variable for each subnode "discharge" member for each reach,
   //   plus any "extra" state variables
   // For catchments, this allocates a state variable for each hru, for water volume,
   //   plus any "extra" state variables

   // catchments first
   int hruPoolCount = GetHRUPoolCount();
   int catchmentCount = (int)m_catchmentArray.GetSize();
   int hruSvCount = 0;
   for (int i = 0; i < catchmentCount; i++)
      {
      int count = m_catchmentArray[i]->GetHRUCount();
      hruSvCount += (count * hruPoolCount);
      }

   // note: total size of m_hruBlock is ( total number of HRUs ) * ( number of layers ) * (number of sv's/HRU)

   m_hruBlock.Init(m_integrator, hruSvCount * m_hruSvCount, m_initTimeStep);   // ALWAYS RK4???. INT_METHOD::IM_RKF  Note: Derivatives are set in GetCatchmentDerivatives.
   if (m_integrator == IM_RKF)
      {
      //m_hruBlock.m_rkvInitTimeStep = m_initTimeStep;
      m_hruBlock.m_rkvMinTimeStep = m_minTimeStep;
      m_hruBlock.m_rkvMaxTimeStep = m_maxTimeStep;
      m_hruBlock.m_rkvTolerance = m_errorTolerance;
      }
   hruSvCount = 0;
   for (int i = 0; i < catchmentCount; i++)
      {
      int count = m_catchmentArray[i]->GetHRUCount();

      for (int j = 0; j < count; j++)
         {
         HRU *pHRU = m_catchmentArray[i]->GetHRU(j);

         for (int k = 0; k < hruPoolCount; k++)
            {
            HRUPool *pHRUPool = m_catchmentArray[i]->GetHRU(j)->GetPool(k);

            pHRUPool->m_svIndex = hruSvCount;
            m_hruBlock.SetStateVar(&(pHRUPool->m_volumeWater), hruSvCount++);

            for (int l = 0; l < m_hruSvCount - 1; l++)
               { 
               m_hruBlock.SetStateVar(&(pHRUPool->m_svArray[l]), hruSvCount++);
               pHRUPool->m_svArray[l] = 0.0f;
               }
            }
         }
      }

   // next, reaches.  Each subnode gets a set of state variables
   if (m_pReachRouting->GetMethod() == GM_RK4
      || m_pReachRouting->GetMethod() == GM_EULER
      || m_pReachRouting->GetMethod() == GM_RKF)
      {
      int reachCount = (int)m_reachArray.GetSize();
      int reachSvCount = 0;
      for (int i = 0; i < reachCount; i++)
         {
         Reach *pReach = m_reachArray[i];
         reachSvCount += pReach->GetSubnodeCount();
         }

      INT_METHOD intMethod = IM_RK4;
      if (m_pReachRouting->GetMethod() == GM_EULER)
         intMethod = IM_EULER;
      else if (m_pReachRouting->GetMethod() == GM_RKF)
         intMethod = IM_RKF;

      m_reachBlock.Init(intMethod, reachSvCount * m_reachSvCount, m_initTimeStep);

      reachSvCount = 0;
      for (int i = 0; i < reachCount; i++)
         {
         Reach *pReach = m_reachArray[i];

         for (int j = 0; j < pReach->GetSubnodeCount(); j++)
            {
            // assign state variables
            ReachSubnode *pNode = (ReachSubnode*)pReach->m_subnodeArray[j];
            pNode->m_svIndex = reachSvCount;
            m_reachBlock.SetStateVar(&pNode->m_volume, reachSvCount++);

            for (int k = 0; k < m_reachSvCount - 1; k++)
               m_reachBlock.SetStateVar(&(pNode->m_svArray[k]), reachSvCount++);
            }
         }
      }

   // next, reservoirs
   int reservoirCount = (int)m_reservoirArray.GetSize();
   m_reservoirBlock.Init(IM_RK4, reservoirCount * m_reservoirSvCount, m_initTimeStep);

   int resSvCount = 0;
   for (int i = 0; i < reservoirCount; i++)
      {
      Reservoir *pRes = m_reservoirArray.GetAt(i);
      pRes->m_svIndex = resSvCount;
      m_reservoirBlock.SetStateVar(&(pRes->m_volume), resSvCount++);

      for (int k = 0; k < m_reservoirSvCount - 1; k++)
         m_reservoirBlock.SetStateVar(&(pRes->m_svArray[k]), resSvCount++);
      }

   // next, totals (for error-check mass balance).  total includes:
   // 1) total input
   // 2) total output
   // 3) one entry for each flux info to accumulate annual fluxes
   m_totalBlock.Init(IM_RK4, 2, m_timeStep);
   m_totalBlock.SetStateVar(&m_totalWaterInput, 0);
   m_totalBlock.SetStateVar(&m_totalWaterOutput, 1);


   return true;
   }


void FlowModel::AllocateOutputVars()
   {
   /*
   m_pInstreamData = new FDataObj( 3 , 0 );
   m_pInstreamData->SetLabel(0,_T("Time"));
   m_pInstreamData->SetLabel(1,_T("Mod Q (ft3/s)"));
   if (m_pMeasuredDischargeData != NULL)
      m_pInstreamData->SetLabel(2, _T("Meas Q (ft3/s)"));
   m_instreamDataObjArray.Add( m_pInstreamData );

   m_pTempData = new FDataObj( 2 , 0 );
   m_pTempData->SetLabel(0,_T("Time"));
   m_pTempData->SetLabel(1,_T("Temp (C)"));
   m_tempDataObjArray.Add( m_pTempData );

   m_pPrecipData = new FDataObj( 2 , 0 );
   m_pPrecipData->SetLabel(0,_T("Time"));
   m_pPrecipData->SetLabel(1,_T("Precip (mm/d)"));
   m_precipDataObjArray.Add( m_pPrecipData );

   m_pSoilMoistureData = new FDataObj( 2 , 0 );
   m_pSoilMoistureData->SetLabel(0,_T("Time"));
   m_pSoilMoistureData->SetLabel(1,_T("SoilMoisture"));
   m_soilMoistureDataObjArray.Add( m_pSoilMoistureData );

   m_pGWDepthData = new FDataObj( 2 , 0 );
   m_pGWDepthData->SetLabel(0,_T("Time"));
   m_pGWDepthData->SetLabel(1,_T("SoilMoisture"));
   m_gWDepthDataObjArray.Add( m_pGWDepthData );  */
   }


void FlowModel::GetCatchmentDerivatives(double time, double timeStep, int svCount, double *derivatives /*out*/, void *extra)
   {
   FlowContext *pFlowContext = (FlowContext*)extra;

   pFlowContext->timeStep = (float)timeStep;
   pFlowContext->time = (float)time;

   FlowModel *pModel = pFlowContext->pFlowModel;
   //pModel->ResetFluxValuesForStep(pFlowContext->pEnvContext);

   pModel->SetGlobalExtraSVRxn();

   GlobalMethodManager::Step(pFlowContext);

//   pFlowContext->svCount = m_pFlowModel->m_hruSvCount - 1;

   int catchmentCount = (int)pModel->m_catchmentArray.GetSize();
   int hruPoolCount = pModel->GetHRUPoolCount();

   FlowContext flowContext(*pFlowContext);    // make a copy for openMP

   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   clock_t start = clock();

   int hruCount = (int)pModel->m_hruArray.GetSize();
#pragma omp parallel for firstprivate( flowContext )
   for (int h = 0; h < hruCount; h++)
      {
      HRU *pHRU = pModel->m_hruArray[h];
      flowContext.pHRU = pHRU;

      for (int l = 0; l < hruPoolCount; l++)
         {
         HRUPool *pHRUPool = pHRU->GetPool(l);
         flowContext.pHRUPool = pHRUPool;
         int svIndex = pHRUPool->m_svIndex;
         float flux = pHRUPool->GetFluxValue();
         derivatives[svIndex] = flux;

         // initialize additional state variables=0
         for (int k = 1; k < pModel->m_hruSvCount; k++)
            derivatives[svIndex + k] = 0;

         // transport fluxes for the extra state variables
         for (int k = 1; k < pModel->m_hruSvCount; k++)
            derivatives[svIndex + k] = pHRUPool->GetExtraSvFluxValue(k - 1);
         }
      }

   if (pModel->m_pReachRouting->GetMethod() == GM_RKF)
      {
      pFlowContext->timing = GMT_REACH;
      GlobalMethodManager::SetTimeStep(pFlowContext->timeStep);
      // GlobalMethodManager::Step(pFlowContext);
      }

   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;
   pModel->m_hruFluxFnRunTime += (float)duration;
   }



void FlowModel::GetReservoirDerivatives(double time, double timeStep, int svCount, double *derivatives /*out*/, void *extra)
   {
   FlowContext *pFlowContext = (FlowContext*)extra;
   FlowModel *pModel = pFlowContext->pFlowModel;

   // compute derivative values for reservoir
   pFlowContext->timeStep = (float)timeStep;
   pFlowContext->time = (float)time;

   pFlowContext->timing = GMT_RESERVOIR;
   GlobalMethodManager::Step(pFlowContext);

   int reservoirCount = (int)pModel->m_reservoirArray.GetSize();

   // iterate through reservoirs, calling fluxes as needed
   //#pragma omp parallel for
   for (int i = 0; i < reservoirCount; i++)
      {
      FlowContext flowContext(*pFlowContext);    // make a copy for openMP

      Reservoir *pRes = pModel->m_reservoirArray[i];

      int svIndex = pRes->m_svIndex;

      // basic inflow/outflow (assumes these were calculated prior to this point)
      //derivatives[svIndex] = pRes->m_inflow - pRes->m_outflow;//m3/d;
      float fluxValue = pRes->GetFluxValue();
      derivatives[svIndex] = pRes->m_inflow - pRes->m_outflow + fluxValue;//m3/d;

      // include additional state variables
      for (int k = 1; k < pModel->m_reservoirSvCount; k++)
         derivatives[svIndex + k] = 0;
      }
   }



void FlowModel::GetTotalDerivatives(double time, double timeStep, int svCount, double *derivatives /*out*/, void *extra)
   {
   FlowContext *pFlowContext = (FlowContext*)extra;

   // NOTE: NEED TO INITIALIZE TIME IN FlowContext before calling here...
   // NOTE:  Should probably do a breadth-first search of the tree(s) rather than what is here.  This could be accomplished by
   //  sorting the m_reachArray in InitReaches()
   FlowModel *pModel = pFlowContext->pFlowModel;

   derivatives[0] = pModel->m_totalWaterInputRate;
   derivatives[1] = pModel->m_totalWaterOutputRate;
   }


ParamTable *FlowModel::GetTable(LPCTSTR name)
   {
   for (int i = 0; i < (int)m_tableArray.GetSize(); i++)
      if (lstrcmpi(m_tableArray[i]->m_pTable->GetName(), name) == 0)
         return m_tableArray[i];

   return NULL;
   }


bool FlowModel::GetTableValue(LPCTSTR name, int col, int row, float &value)
   {
   ParamTable *pTable = GetTable(name);

   if (pTable == NULL)
      return false;

   return pTable->m_pTable->Get(col, row, value);
   }



void FlowModel::AddReservoirLayer(void)
   {
   Map *pMap = m_pCatchmentLayer->GetMapPtr();

   MapLayer *pResLayer = pMap->GetLayer(_T("Reservoirs"));

   if (pResLayer != NULL)      // already exists?
      {
      return;
      }

   // doesn't already exist, so map existing reservoirs
   int resCount = (int)m_reservoirArray.GetSize();
   int usedResCount = 0;
   for (int i = 0; i < resCount; i++)
      {
      if (m_reservoirArray[i]->m_reachArray.GetSize() > 0 ) //pDownstreamReach != NULL)
         usedResCount++;
      }

   m_pResLayer = pMap->AddLayer(LT_POINT);

   DataObj *pData = m_pResLayer->CreateDataTable(0, 1, DOT_FLOAT);

   usedResCount = 0;
   for (int i = 0; i < resCount; i++)
      {
      Reservoir *pRes = m_reservoirArray[i];

      if (pRes->m_pDownstreamReach != NULL)
         {
         int polyIndex = pRes->m_pDownstreamReach->m_polyIndex;

         Poly *pPt = m_pStreamLayer->GetPolygon(polyIndex);
         int vertexCount = (int)pPt->m_vertexArray.GetSize();
         Vertex &v = pPt->m_vertexArray[vertexCount - 1];

         m_pResLayer->AddPoint(v.x, v.y);
         pData->Set(0, i, pRes->m_volume);
         usedResCount++;
         }
      }

   // additional map setup
   m_pResLayer->m_name = _T("Reservoirs");
   m_pResLayer->SetFieldLabel(0, _T("ResVol"));
   //m_pResLayer->AddBin( 0, RGB(0,0,127), _T("Reservoir Volume"), TYPE_FLOAT, 0, 1000000 );
   m_pResLayer->SetActiveField(0);
   m_pResLayer->SetOutlineColor(RGB(0, 0, 127));
   m_pResLayer->SetSolidFill(RGB(0, 0, 255));
   //   m_pResLayer->UseVarWidth( 0, 100 );
      //pMap->RedrawWindow();
   }




////////////////////////////////////////////////////////////////////////////////////////////////
// F L O W    P R O C E S S 
////////////////////////////////////////////////////////////////////////////////////////////////

//FlowProcess::FlowProcess()
//   : EnvModelProcess()
//   , m_maxProcessors(-1)
//   , m_processorsUsed(0)
//   { }


//FlowProcess::~FlowProcess(void)
//   {
//   m_flowModelArray.RemoveAll();
//   }


//bool FlowProcess::Setup(EnvContext *pEnvContext, HWND)
//   {
//   FlowModel *pModel = GetFlowModelFromID(pEnvContext->id);
//   ASSERT(pModel != NULL);
//
//   FlowDlg dlg(pModel, _T("Flow Setup"));
//
//   dlg.DoModal();
//
//   return true;
//   }



//bool FlowProcess::Init(EnvContext *pEnvContext, LPCTSTR initStr)
//   {
//   EnvExtension::Init(pEnvContext, initStr);
//
//   MapLayer *pLayer = (MapLayer*)pEnvContext->pMapLayer;
//
//   bool ok = LoadXml(initStr, pEnvContext);
//
//   if (!ok)
//      return false;
//
//   m_processorsUsed = omp_get_num_procs();
//
//   m_processorsUsed = (3 * m_processorsUsed / 4) + 1;   // default uses 2/3 of the cores
//   if (m_maxProcessors > 0 && m_processorsUsed >= m_maxProcessors)
//      m_processorsUsed = m_maxProcessors;
//
//   omp_set_num_threads(m_processorsUsed);
//
//   FlowModel *pModel = m_flowModelArray.GetAt(m_flowModelArray.GetSize() - 1);
//   ASSERT(pModel != NULL);
//
//   ok = pModel->Init(pEnvContext);
//
//   return ok ? true : false;
//   }
//

///bool FlowProcess::InitRun(EnvContext *pEnvContext)
///   {
///   m_flowModelArray.GetAt(m_flowModelArray.GetSize() - 1);
///
///   FlowModel *pModel = GetFlowModelFromID(pEnvContext->id);
///   ASSERT(pModel != NULL);
///
///   bool ok = pModel->InitRun(pEnvContext);
///
///   return ok ? true : false;
///   }


//bool FlowProcess::Run(EnvContext *pEnvContext)
//   {
//   m_flowModelArray.GetAt(m_flowModelArray.GetSize() - 1);
//
//   FlowModel *pModel = GetFlowModelFromID(pEnvContext->id);
//   ASSERT(pModel != NULL);
//
//   bool ok = pModel->Run(pEnvContext);
//
//   return ok ? true : false;
//   }
//
//bool FlowProcess::EndRun(EnvContext *pEnvContext)
//   {
//   m_flowModelArray.GetAt(m_flowModelArray.GetSize() - 1);
//
//   FlowModel *pModel = GetFlowModelFromID(pEnvContext->id);
//   ASSERT(pModel != NULL);
//
//   bool ok = pModel->EndRun(pEnvContext);
//
//   return ok ? true : false;
//   }
//
//
//FlowModel *FlowProcess::GetFlowModelFromID(int id)
//   {
//   for (int i = 0; i < (int)m_flowModelArray.GetSize(); i++)
//      {
//      FlowModel *pModel = m_flowModelArray.GetAt(i);
//
//      if (pModel->m_id == id)
//         return pModel;
//      }
//
//   return NULL;
//   }


// this method reads an xml input file
bool FlowModel::LoadXml(LPCTSTR filename, EnvContext *pEnvContext )
   {
   MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   bool loadSuccess = true;

   if (!ok)
      {
      CString msg;
      msg.Format(_T("Flow: Error reading input file %s:  %s"), filename, doc.ErrorDesc());
      Report::ErrorMsg(msg);
      return false;
      }

   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // <flow_model>

   FlowModel *pModel = this;

   pModel->m_filename = filename;
   CString grid;
   CString integrator;
   float ts = 1.0f;
   grid.Format(_T("none"));

   int iduDailyOutput = 0;
   int iduAnnualOutput = 0;
   int reachDailyOutput = 0;
   int reachAnnualOutput = 0;
   int iduDailyTracerOutput = 0;

   XML_ATTR rootAttrs[] = {
      // attr                     type          address                                isReq  checkCol
      { _T("name"),                    TYPE_CSTRING,  &(pModel->m_name),                   false,   0 },
      { _T("time_step"),               TYPE_FLOAT,    &ts,                                 false,   0 },
      //{ _T("build_catchments_method"), TYPE_INT,      &(pModel->m_buildCatchmentsMethod),  false,   0 }, deprecated
      { _T("catchment_join_col"),      TYPE_CSTRING,  &(pModel->m_catchmentJoinCol),       true,    0 },
      { _T("stream_join_col"),         TYPE_CSTRING,  &(pModel->m_streamJoinCol),          true,    0 },
      { _T("simplify_query"),          TYPE_CSTRING,  &(pModel->m_streamQuery),            false,   0 },
      { _T("simplify_min_area"),       TYPE_FLOAT,    &(pModel->m_minCatchmentArea),       false,   0 },
      { _T("simplify_max_area"),       TYPE_FLOAT,    &(pModel->m_maxCatchmentArea),       false,   0 },
      { _T("path"),                    TYPE_CSTRING,  &(pModel->m_path),                   false,   0 },
      { _T("check_mass_balance"),      TYPE_INT,      &(pModel->m_checkMassBalance),       false,   0 },
      { _T("max_processors"),          TYPE_INT,      &m_maxProcessors,                    false,   0 },
      { _T("grid"),                    TYPE_CSTRING,  &grid,                               false,   0 },
      { _T("integrator"),              TYPE_CSTRING,  &integrator,                         false,   0 },
      { _T("min_timestep"),            TYPE_FLOAT,    &(pModel->m_minTimeStep),            false,   0 },
      { _T("max_timestep"),            TYPE_FLOAT,    &(pModel->m_maxTimeStep),            false,   0 },
      { _T("error_tolerance"),         TYPE_FLOAT,    &(pModel->m_errorTolerance),         false,   0 },
      { _T("init_timestep"),           TYPE_FLOAT,    &(pModel->m_initTimeStep),           false,   0 },
      { _T("update_display"),          TYPE_INT,      &(pModel->m_mapUpdate),              false,   0 },
      { _T("initial_conditions"),      TYPE_CSTRING,  &(pModel->m_initConditionsFileName), false,   0 },
      { _T("daily_idu_output"),        TYPE_INT,      &iduDailyOutput,                     false,   0 },
      { _T("daily_idu_tracer_output"), TYPE_INT,      &iduDailyTracerOutput,                     false,   0 },
      { _T("annual_idu_output"),       TYPE_INT,      &iduAnnualOutput,                    false,   0 },
      { _T("daily_reach_output"),      TYPE_INT,      &reachDailyOutput,                   false,   0 },
      { _T("annual_reach_output"),     TYPE_INT,      &reachAnnualOutput,                  false,   0 },
      { NULL,                      TYPE_NULL,     NULL,                                false,   0 } };

   ok = TiXmlGetAttributes(pXmlRoot, rootAttrs, filename, NULL);
   pModel->m_path = PathManager::GetPath(1);
   if (!ok)
      {
      CString msg;
      msg.Format(_T("Flow: Misformed root element reading <flow> attributes in input file %s"), filename);
      Report::ErrorMsg(msg);
      delete pModel;
      return false;
      }
   pModel->m_timeStep = ts;
   if (grid.CompareNoCase(_T("none")) != 0)
      {
      pModel->m_pGrid = pIDULayer->GetMapPtr()->GetLayer(grid);
      pModel->m_grid = grid;
      if (!pModel->m_pGrid)
         {
         CString msg;
         msg.Format(_T("Flow: Can't find grid '%s' in the Map. Check the envx file"), (LPCTSTR)grid);
         Report::ErrorMsg(msg);
         return false;
         }
      }
   if (lstrcmpi(integrator, _T("rk4")) == 0)
      pModel->m_integrator = IM_RK4;
   else if (lstrcmpi(integrator, _T("rkf")) == 0)
      pModel->m_integrator = IM_RKF;
   else
      pModel->m_integrator = IM_EULER;

   pModel->m_detailedOutputFlags = 0;
   if (iduDailyOutput) pModel->m_detailedOutputFlags += 1;
   if (iduAnnualOutput) pModel->m_detailedOutputFlags += 2;
   if (reachDailyOutput) pModel->m_detailedOutputFlags += 4;
   if (reachAnnualOutput) pModel->m_detailedOutputFlags += 8;
   if (iduDailyTracerOutput) pModel->m_detailedOutputFlags += 16;

   //------------------- <scenario macros> ------------------
   TiXmlElement *pXmlMacros = pXmlRoot->FirstChildElement(_T("macros"));
   if (pXmlMacros)
      {
      TiXmlElement *pXmlMacro = pXmlMacros->FirstChildElement(_T("macro"));

      while (pXmlMacro != NULL)
         {
         ScenarioMacro *pMacro = new ScenarioMacro;
         XML_ATTR macroAttrs[] = {
            // attr                 type          address                     isReq  checkCol
            { _T("name"),              TYPE_CSTRING,   &(pMacro->m_name),          true,   0 },
            { _T("default"),           TYPE_CSTRING,   &(pMacro->m_defaultValue),  true,   0 },
            { NULL,                TYPE_NULL,      NULL,                       false,  0 } };

         ok = TiXmlGetAttributes(pXmlMacro, macroAttrs, filename, NULL);
         if (!ok)
            {
            CString msg;
            msg.Format(_T("Flow: Misformed root element reading <macro> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            delete pMacro;
            }
         else
            {
            pModel->m_macroArray.Add(pMacro);

            // parse any details
            TiXmlElement *pXmlScn = pXmlMacro->FirstChildElement(_T("scenario"));
            while (pXmlScn != NULL)
               {
               SCENARIO_MACRO *pScn = new SCENARIO_MACRO;
               CString scnName;
               XML_ATTR scnAttrs[] = {
                  // attr           type          address                  isReq  checkCol
                  { _T("name"),       TYPE_CSTRING,   &(pScn->envScenarioName), true,   0 },
                  { _T("value"),      TYPE_CSTRING,   &(pScn->value),           true,   0 },
                  { NULL,         TYPE_NULL,      NULL,                     false,  0 } };

               ok = TiXmlGetAttributes(pXmlScn, scnAttrs, filename, NULL);
               if (!ok)
                  {
                  CString msg;
                  msg.Format(_T("Flow: Misformed element reading <macro><scenario> attributes in input file %s"), filename);
                  Report::ErrorMsg(msg);
                  delete pScn;
                  }
               else
                  {
                  Scenario *pScenario = NULL;  //::EnvGetScenarioFromName( scnName, &(pScn->envScenarioIndex) );   // this is the envision scenario index

                  //if ( pScenario != NULL )
                  pMacro->m_macroArray.Add(pScn);
                  //else
                  //   delete pScn;
                  }
               pXmlScn = pXmlScn->NextSiblingElement(_T("scenario"));
               }  // end of: while( pXmlScn != NULL )
            }  // end of: else (Reading <macro> xml succesful

         pXmlMacro = pXmlMacro->NextSiblingElement(_T("macro"));
         }  // end of: while ( pXmlMacro != NULL )   }
      }
   //------------------- <catchment> ------------------
   TiXmlElement *pXmlCatchment = pXmlRoot->FirstChildElement(_T("catchments"));
   XML_ATTR catchmentAttrs[] = {
      // attr                 type          address                           isReq  checkCol
      { _T("layer"),              TYPE_CSTRING,  &(pModel->m_catchmentLayer),     false,   0 },
      { _T("query"),              TYPE_CSTRING,  &(pModel->m_catchmentQuery),     false,   0 },
      { _T("area_col"),           TYPE_CSTRING,  &(pModel->m_areaCol),            false,   0 },
      { _T("elev_col"),           TYPE_CSTRING,  &(pModel->m_elevCol),            false,   0 },
      { _T("catchment_agg_cols"), TYPE_CSTRING,  &(pModel->m_catchmentAggCols),   true,    0 },
      { _T("hru_agg_cols"),       TYPE_CSTRING,  &(pModel->m_hruAggCols),         true,    0 },
      { _T("hruID_col"),          TYPE_CSTRING,  &(pModel->m_hruIDCol),           false,   0 },
      { _T("catchmentID_col"),    TYPE_CSTRING,  &(pModel->m_catchIDCol),         false,   0 },
      // the following have been deprecated
      //{ _T("pools"),              TYPE_INT,      &(pModel->m_poolCount),          false,   0 },
      //{ _T("pool_names"),         TYPE_CSTRING,  &(colNames),                     false,   0 },
      //{ _T("pool_depths"),        TYPE_CSTRING,  &(layerDepths),                  false,   0 },
      //{ _T("init_water_content"), TYPE_CSTRING,  &(layerWC),                      false,   0 },
      //{ _T("soil_layers"),        TYPE_INT,      &soilLayers,                     false,   0 },// deprecated, use pools instead
      //{ _T("snow_layers"),        TYPE_INT,      &snowLayers,                     false,   0 },// deprecated
      //{ _T("veg_layers"),         TYPE_INT,      &vegLayers,                      false,   0 },// deprecated
      //{ _T("layer_names"),        TYPE_CSTRING,  &(colNames),                     false,   0 }, // deprecated
      //{ _T("layer_depths"),       TYPE_CSTRING,  &(layerDepths),                  false,   0 }, // deprecated
      { NULL,                 TYPE_NULL,     NULL,                            false,   0 } };

   ok = TiXmlGetAttributes(pXmlCatchment, catchmentAttrs, filename, NULL);
   if (!ok)
      {
      CString msg;
      msg.Format(_T("Flow: Misformed root element reading <catchments> attributes in input file %s"), filename);
      Report::ErrorMsg(msg);
      delete pModel;
      return false;
      }

   // process <pools> tag
   TiXmlElement *pXmlPools = pXmlCatchment->FirstChildElement(_T("pools"));
   if (pXmlPools == NULL )
      {
      Report::ErrorMsg("Flow: missing <pools> tags defined within the parent <catchment> tag. At least one pool is required.  Flow can not continue until this is fixed...");
      delete pModel;
      return false;
      }
   if ( pXmlPools->NoChildren())
      {
      Report::ErrorMsg("Flow: No <pool> tags defined within the parent <pool> tag. At least one pool is required.  Flow can not continue until this is fixed...");
      delete pModel;
      return false;
      }

   TiXmlElement *pXmlPool = pXmlPools->FirstChildElement(_T("pool"));
   while (pXmlPool != NULL)
      {
      PoolInfo *pInfo = new PoolInfo;
      CString type;
      XML_ATTR poolAttrs[] = {
      // attr                        type          address                           isReq  checkCol
         { _T("name"),               TYPE_CSTRING,  &(pInfo->m_name),                true,    0 },
         { _T("type"),               TYPE_CSTRING,  &type,                           true,    0 },
         { _T("depth"),              TYPE_FLOAT,    &(pInfo->m_depth),               true,    0 },
         { _T("init_water_content"), TYPE_FLOAT,    &(pInfo->m_initWaterContent),    false,   0 },
         { _T("init_water_temp"),    TYPE_FLOAT,    &(pInfo->m_initWaterTemp ),      false,   0 },
         { NULL,                     TYPE_NULL,     NULL,                            false,   0 } };

      ok = TiXmlGetAttributes(pXmlPool, poolAttrs, filename, NULL);
      if (!ok)
         {
         CString msg;
         msg.Format(_T("Flow: Misformed <pool> element reading <catchments> attributes in input file %s"), filename);
         Report::ErrorMsg(msg);
         delete pModel;
         return false;
         }

      type.MakeLower();

      if ( type[0] == 'v' )
         pInfo->m_type = PT_VEG;
      else if ( type[0] == 'i' )
         pInfo->m_type = PT_IRRIGATED;
      else if ( type[0] == 's' && type[1] == 'n' )
         pInfo->m_type = PT_SNOW;
      else if (type[0] == 's' && type[1] == 'o')
         pInfo->m_type = PT_SOIL;
      else if (type[0] == 'm' )
         pInfo->m_type = PT_MELT;
      else
         {
         CString msg;
         msg.Format( "Flow: Unrecognized pool type ('%s') - must be one of 'veg', 'soil', 'irrigated', or 'snow'.", (LPCTSTR) type);
         Report::LogWarning( msg );
         pInfo->m_type = PT_SOIL;
         }

      this->m_poolInfoArray.Add( pInfo );
      pXmlPool = pXmlPool->NextSiblingElement(_T("pool"));
      }

   // pModel->ParseHRULayerDetails(colNames, 0);
   // pModel->ParseHRULayerDetails(layerDepths, 1);
   // pModel->ParseHRULayerDetails(layerWC, 2);
   pModel->m_pCatchmentLayer = pIDULayer->GetMapPtr()->GetLayer(pModel->m_catchmentLayer);
   if (pModel->m_pCatchmentLayer == NULL)
      {
      CString msg;
      msg.Format(_T("Flow: Unable to find catchment layer '%s'.  This layer is not present.  Check your envx file <layer> entries!"), (LPCTSTR)pModel->m_catchmentLayer);
      Report::ErrorMsg(msg);
      delete pModel;
      return false;
      }

   // ERROR CHECK

   if (pModel->m_hruIDCol.IsEmpty())
      pModel->m_hruIDCol = _T("HRU_ID");

   if (pModel->m_catchIDCol.IsEmpty())
      pModel->m_catchIDCol = _T("CATCH_ID");

   if (pModel->m_areaCol.IsEmpty())
      pModel->m_areaCol = _T("AREA");

   if (pModel->m_elevCol.IsEmpty())
      pModel->m_elevCol = _T("ELEV_MEAN");

   pModel->m_colCatchmentArea = pModel->m_pCatchmentLayer->GetFieldCol(pModel->m_areaCol);
   if (pModel->m_colCatchmentArea < 0)
      {
      CString msg;
      msg.Format(_T("Flow: Bad Area col %s"), filename);
      Report::ErrorMsg(msg);
      }

   pModel->m_colElev = pModel->m_pCatchmentLayer->GetFieldCol(pModel->m_elevCol);
   if (pModel->m_colElev < 0)
      {
      CString msg;
      msg.Format(_T("Flow:  Elevation col %s doesn't exist.  It will be added and, if using station met data, it's values will be set to the station elevation (it is used for lapse rate calculations)"), filename);
      Report::ErrorMsg(msg);
      }

   //-------------- <stream> ------------------
   TiXmlElement *pXmlStream = pXmlRoot->FirstChildElement(_T("streams"));
   float reachTimeStep = 1.0f;
   XML_ATTR streamAttrs[] = {
      // attr              type           address                          isReq  checkCol
      { _T("layer"),           TYPE_CSTRING,  &(pModel->m_streamLayer),        true,    0 },
      { _T("order_col"),       TYPE_CSTRING,  &(pModel->m_orderCol),           false,   0 },
      { _T("subnode_length"),  TYPE_FLOAT,    &(pModel->m_subnodeLength),      false,   0 },
      { _T("wd_ratio"),        TYPE_FLOAT,    &(pModel->m_wdRatio),            false,   0 },
      { _T("mannings_n"),      TYPE_FLOAT,    &(pModel->m_n),                  false,   0 },
      { _T("stepsize"),        TYPE_FLOAT,    &reachTimeStep,                  false,   0 },    // not currently used!!!!
      { _T("treeID_col"),      TYPE_CSTRING,  &(pModel->m_treeIDCol),         false,   0 },
      { _T("to_col"),          TYPE_CSTRING,  &(pModel->m_toCol),             false,   0 },
      { _T("from_col"),        TYPE_CSTRING,  &(pModel->m_fromCol),           false,   0 },
      { NULL,              TYPE_NULL,     NULL,                            false,   0 } };

   ok = TiXmlGetAttributes(pXmlStream, streamAttrs, filename, NULL);
   if (!ok)
      {
      CString msg;
      msg.Format(_T("Flow: Misformed root element reading <streams> attributes in input file %s"), filename);
      Report::ErrorMsg(msg);
      delete pModel;
      return false;
      }

   // success - add model and get flux definitions
   pModel->m_pStreamLayer = pIDULayer->GetMapPtr()->GetLayer(pModel->m_streamLayer);

   if (pModel->m_pStreamLayer == NULL)
      {
      CString msg;
      msg.Format(_T("Flow:  Unable to find stream coverage '%s' - this s a required coverage.  Flow can not proceed."), (LPCTSTR)pModel->m_streamLayer);
      Report::ErrorMsg(msg);
      delete pModel;
      return false;
      }

   if (!pModel->m_treeIDCol.IsEmpty())
      CheckCol(pModel->m_pStreamLayer, pModel->m_colTreeID, pModel->m_treeIDCol, TYPE_LONG, CC_AUTOADD);

   if (pModel->m_toCol.IsEmpty())
      pModel->m_toCol = _T("TNODE_");

   CheckCol(pModel->m_pStreamLayer, pModel->m_colStreamTo, pModel->m_toCol, TYPE_LONG, CC_MUST_EXIST);

   if (pModel->m_fromCol.IsEmpty())
      pModel->m_fromCol = _T("FNODE_");

   CheckCol(pModel->m_pStreamLayer, pModel->m_colStreamFrom, pModel->m_fromCol, TYPE_LONG, CC_MUST_EXIST);

   if (!pModel->m_orderCol.IsEmpty())
      CheckCol(pModel->m_pStreamLayer, pModel->m_colStreamOrder, pModel->m_orderCol, TYPE_LONG, CC_AUTOADD);

   TiXmlElement *pXmlGlobal = pXmlRoot->FirstChildElement(_T("global_methods"));
   if (pXmlGlobal != NULL)
      {
      // process and child nodes
      TiXmlElement *pXmlGlobalChild = pXmlGlobal->FirstChildElement();

      while (pXmlGlobalChild != NULL)
         {
         if (pXmlGlobalChild->Type() == TiXmlNode::ELEMENT)
            {
            PCTSTR tagName = pXmlGlobalChild->Value();

            // Reach Routing
            if (_tcsicmp(tagName, _T("reach_routing")) == 0)
               {
               pModel->m_pReachRouting = ReachRouting::LoadXml(pXmlGlobalChild, filename,this);
               if (pModel->m_pReachRouting != NULL)
                  GlobalMethodManager::AddGlobalMethod(pModel->m_pReachRouting);
               }

            // lateral exchnage
            else if (_tcsicmp(tagName, _T("lateral_exchange")) == 0)
               {
               pModel->m_pLateralExchange = LateralExchange::LoadXml(pXmlGlobalChild, filename,this);
               if (pModel->m_pLateralExchange != NULL)
                  GlobalMethodManager::AddGlobalMethod(pModel->m_pLateralExchange);
               }

            // hru vertical exchange
            else if (_tcsicmp(tagName, _T("hru_vertical_exchange")) == 0)
               {
               pModel->m_pHruVertExchange = HruVertExchange::LoadXml(pXmlGlobalChild, filename,this);
               if (pModel->m_pHruVertExchange != NULL)
                  GlobalMethodManager::AddGlobalMethod(pModel->m_pHruVertExchange);
               }

            // external methods
            else if (_tcsicmp(tagName, _T("external")) == 0)
               {
               GlobalMethod *pMethod = ExternalMethod::LoadXml(pXmlGlobalChild, filename,this);
               if (pMethod != NULL)
                  GlobalMethodManager::AddGlobalMethod(pMethod);
               }

            // evap_trans
            else if (_tcsicmp(tagName, _T("evap_trans")) == 0)
               {
               GlobalMethod *pMethod = EvapTrans::LoadXml(pXmlGlobalChild, pIDULayer, filename, this);
               if (pMethod != NULL)
                  GlobalMethodManager::AddGlobalMethod(pMethod);
               }

            // daily urban water demand 
            else if (_tcsicmp(tagName, _T("climate_metrics")) == 0)
            {
                GlobalMethod* pMethod = Climate_Metrics::LoadXml(pXmlGlobalChild, pIDULayer, filename, this);
                if (pMethod != NULL)
                    GlobalMethodManager::AddGlobalMethod(pMethod);
            }


            // daily urban water demand 
            else if (_tcsicmp(tagName, _T("Urban_Water_Demand")) == 0)
               {
               GlobalMethod *pMethod = DailyUrbanWaterDemand::LoadXml(pXmlGlobalChild, pIDULayer, filename,this);
               if (pMethod != NULL)
                  GlobalMethodManager::AddGlobalMethod(pMethod);
               }

            // flux (multiple entries allowed)
            else if (_tcsicmp(tagName, _T("flux")) == 0)
               {
               GlobalMethod *pMethod = FluxExpr::LoadXml(pXmlGlobalChild, pIDULayer, filename, this);
               if (pMethod != NULL)
                  GlobalMethodManager::AddGlobalMethod(pMethod);
               }

            // allocations (multiple entries allowed)
            else if (_tcsicmp(tagName, _T("allocation")) == 0)
               {
               GlobalMethod *pMethod = WaterAllocation::LoadXml(pXmlGlobalChild, filename, this);
               if (pMethod != NULL)
                  GlobalMethodManager::AddGlobalMethod(pMethod);
               }
            }   // end of: if ( pXmlGlobalChild->Type() == TiXmlNode::ELEMENT )

         pXmlGlobalChild = pXmlGlobalChild->NextSiblingElement();
         }
      }  // end of: if ( pXmlGlobal != NULL )

   // scenarios
   TiXmlElement *pXmlScenarios = pXmlRoot->FirstChildElement(_T("scenarios"));
   if (pXmlScenarios == NULL)
      {
      CString msg(_T("Flow: Missing <scenarios> tag when reading "));
      msg += filename;
      msg += _T("This is a required tag");
      Report::ErrorMsg(msg);
      }
   else
      {
      int defaultIndex = 0;
      TiXmlGetAttr(pXmlScenarios, _T("default"), defaultIndex, 0, false);

      TiXmlElement *pXmlScenario = pXmlScenarios->FirstChildElement(_T("scenario"));

      while (pXmlScenario != NULL)
         {
         LPCTSTR name = NULL;
         LPCTSTR points = NULL;
         int id = -1;

         XML_ATTR scenarioAttrs[] = {
            // attr      type          address         isReq  checkCol
            { _T("name"),    TYPE_STRING,   &name,         false,   0 },
            { _T("id"),      TYPE_INT,      &id,           true,    0 },
            { _T("points"),  TYPE_STRING,   &points,       false,   0 },
            { NULL,      TYPE_NULL,     NULL,          false,   0 } };

         bool ok = TiXmlGetAttributes(pXmlScenario, scenarioAttrs, filename);
         if (!ok)
            {
            CString msg;
            msg.Format(_T("Flow: Misformed element reading <scenario> attributes in input file %s - it is missing the 'id' attribute"), filename);
            Report::ErrorMsg(msg);
            break;
            }

         FlowScenario *pScenario = new FlowScenario;
         pScenario->m_id = id;
         pScenario->m_name = name;
         pModel->m_scenarioArray.Add(pScenario);
         pScenario->m_useStationData = false;

         //---------- <climate> ----------------------------------
         TiXmlElement *pXmlClimate = pXmlScenario->FirstChildElement(_T("climate"));
         while (pXmlClimate != NULL)
            {
            LPTSTR path = NULL, varName = NULL, cdtype = NULL;
            float delta = -999999999.0f;
            float elev = 0.0f;
            int id = -1;
            //CString path1;

            XML_ATTR climateAttrs[] = {
               // attr        type          address     isReq  checkCol
               { _T("type"),      TYPE_STRING,   &cdtype,   true,   0 },
               { _T("path"),      TYPE_STRING,   &path,     true,   0 },
               { _T("var_name"),  TYPE_STRING,   &varName,  false,  0 },
               { _T("delta"),     TYPE_FLOAT,    &delta,    false,  0 },
               { _T("elev"),      TYPE_FLOAT,    &elev,     false,  0 },
               { _T("id"),        TYPE_INT,      &id,       false,  0 },
               
               { NULL,        TYPE_NULL,     NULL,      false,  0 } };

            bool ok = TiXmlGetAttributes(pXmlClimate, climateAttrs, filename);
            if (!ok)
               {
               CString msg;
               msg.Format(_T("Flow: Misformed element reading <climate> attributes in input file %s"), filename);
               Report::ErrorMsg(msg);
               }
            else
               {
               ClimateDataInfo *pInfo = new ClimateDataInfo;
               pInfo->m_varName = varName;

               // is there a : in the path?  If so it is complete.  If not, assume it is a subdirectory of the project path
               if ( _tcschr( path, ':') != NULL )   // there was a colon
                  pInfo->m_path = path;
               else
                  pInfo->m_path = pModel->m_path + path;

               // if not a weather station, allocate a geospatial data object
               if ( cdtype[0] != 'c' )
                  pInfo->m_pNetCDFData = new GeoSpatialDataObj(U_UNDEFINED);

               pInfo->m_useDelta = pXmlClimate->Attribute(_T("delta")) ? true : false;
               if (pInfo->m_useDelta)
                  pInfo->m_delta = delta;

               //pModel->m_climateDataType = CDT_PRECIP; //Just makes clear that it is NOT measured time series

               switch (cdtype[0])
                  {
                  case _T('p'):     // precip
                     pInfo->m_type = CDT_PRECIP;
                     break;

                  case _T('t'):
                     if (cdtype[2] == _T('v'))          // tavg
                        pInfo->m_type = CDT_TMEAN;
                     else if (cdtype[2] == _T('i'))     // tmin
                        pInfo->m_type = CDT_TMIN;
                     else if (cdtype[2] == _T('a'))     // tmax
                        pInfo->m_type = CDT_TMAX;
                     break;

                  case _T('s'):
                     if (cdtype[1] == _T('o'))          // tavg
                        pInfo->m_type = CDT_SOLARRAD;
                     break;

                  case _T('h'):
                     pInfo->m_type = CDT_SPHUMIDITY;
                     break;

                  case _T('r'):
                     pInfo->m_type = CDT_RELHUMIDITY;
                     break;

                  case _T('w'):
                     pInfo->m_type = CDT_WINDSPEED;
                     break;

                  case _T('c'):
                     {
                     pInfo->m_type = CDT_STATIONDATA;
                     pScenario->m_useStationData = true;

                     bool success = true;
                     CString _path;
                     if (PathManager::FindPath(path, _path) < 0)
                        {
                        CString msg;
                        msg.Format(_T("Flow: Climate Station path '%s' not found.  Climate data will not be available."), path);
                        Report::LogWarning(msg);
                        success = false;
                        }

                     if ( pModel->m_colStationID < 0 && id >= 0)
                        pModel->m_colStationID = pIDULayer->GetFieldCol( "STATIONID" );
  
                     if ( success )
                        {
                        pInfo->m_path = _path;
                        pInfo->m_id = id;
                        pInfo->m_stationElevation = elev;
                        pModel->ReadClimateStationData(pInfo, _path );
                        //pModel->m_climateFilePath = _path;
                        //pModel->ReadClimateStationData(_path);
                        //pModel->m_climateStationElev = elev;
                        }
                     }
                  break;

                  default:
                     {
                     CString msg;
                     msg.Format(_T("Flow Error: Unrecognized climate data '%s'type reading file %s"), cdtype, filename);
                     Report::LogWarning(msg);
                     delete pInfo;
                     pInfo = NULL;
                     }
                  }  // end of: switch( type[ 0 ] )

               if (pInfo != NULL)
                  {
                  if (cdtype[0] != _T('c')) // NOT weather station data
                     {
                     pScenario->m_climateDataMap.SetAt(pInfo->m_type, pInfo);
                     pScenario->m_climateInfoArray.Add(pInfo);

                     // No need to init GDAL libs for station data!
                     pInfo->m_pNetCDFData->InitLibraries();
                     }
                  else    // getting weather station data
                     {
                     if ( pScenario->m_climateInfoArray.GetSize() > 1 ) 
                        {
                        if ( pModel->m_colStationID < 0 )
                           {
                           Report::LogError(_T("Flow Error: Multiple weather stations defined, but [STATIONID] field is not present in the IDU coverage.  This field should be populated before proceeding.") );
                           }
                        else if ( pInfo->m_id < 0 )
                           {
                           CString msg;
                           msg.Format(_T("Flow Error: Weather station '%s' is missing it's ID.  This should be specified before proceeding."), (LPCTSTR) pInfo->m_path );
                           Report::LogError(msg);
                           }
                        }
               
                     pScenario->m_climateDataMap.SetAt(pInfo->m_id, pInfo);
                     pScenario->m_climateInfoArray.Add(pInfo);
                     }
                  }
               }  // end of: else ( valid info )

            pXmlClimate = pXmlClimate->NextSiblingElement(_T("climate"));
            }  // end of: while ( pXmlClimate != NULL )


         pModel->m_currentFlowScenarioIndex = defaultIndex;
         if (pModel->m_currentFlowScenarioIndex >= defaultIndex)
            pModel->m_currentFlowScenarioIndex = 0;

         pXmlScenario = pXmlScenario->NextSiblingElement(_T("scenario"));
         }  // end of: while ( pXmlScenario != NULL )
      }  // end of: if ( pXmlScenarios != NULL )

   //----------- <reservoirs> ------------------------------
   TiXmlElement *pXmlReservoirs = pXmlRoot->FirstChildElement(_T("reservoirs"));

   if (pXmlReservoirs != NULL)
      {
      bool ok = true;

      LPCTSTR field = pXmlReservoirs->Attribute(_T("col"));
      if (field == NULL)
         {
         CString msg;
         msg.Format(_T("Flow: Missing 'col' attribute reading <reservoirs> tag in input file %s"), filename);
         Report::ErrorMsg(msg);
         }
      else
         ok = CheckCol(pModel->m_pStreamLayer, pModel->m_colResID, field, TYPE_INT, CC_MUST_EXIST);

      if (ok)
         {
         TiXmlElement *pXmlRes = pXmlReservoirs->FirstChildElement(_T("reservoir"));

         while (pXmlRes != NULL)
            {
            Reservoir *pRes = new Reservoir;
            LPCSTR reservoirType = NULL;

            float volume = 0.0f;
            XML_ATTR resAttrs[] = {
               // attr                  type          address                           isReq  checkCol
               { _T("id"),               TYPE_INT,      &(pRes->m_id),                      true,   0 },
               { _T("name"),             TYPE_CSTRING,  &(pRes->m_name),                    true,   0 },
               { _T("path"),             TYPE_CSTRING,  &(pRes->m_dir),                     false,   0 },
               { _T("initVolume"),       TYPE_FLOAT,    &(volume),                          false,   0 },
               { _T("area_vol_curve"),   TYPE_CSTRING,  &(pRes->m_areaVolCurveFilename),    false,   0 },
               { _T("av_dir"),           TYPE_CSTRING,  &(pRes->m_avdir),                   false,   0 },
               { _T("minOutflow"),       TYPE_FLOAT,    &(pRes->m_minOutflow),              false,   0 },
               { _T("rule_curve"),       TYPE_CSTRING,  &(pRes->m_ruleCurveFilename),       false,   0 },
               { _T("buffer_zone"),      TYPE_CSTRING,  &(pRes->m_bufferZoneFilename),      false,   0 },
               { _T("rc_dir"),           TYPE_CSTRING,  &(pRes->m_rcdir),                   false,   0 },
               { _T("maxVolume"),        TYPE_FLOAT,    &(pRes->m_maxVolume),               false,   0 },
               { _T("composite_rc"),     TYPE_CSTRING,  &(pRes->m_releaseCapacityFilename), false,   0 },
               { _T("re_dir"),           TYPE_CSTRING,  &(pRes->m_redir),                   false,   0 },
               { _T("maxPowerFlow"),     TYPE_FLOAT,    &(pRes->m_gateMaxPowerFlow),        false,   0 },
               { _T("ro_rc"),            TYPE_CSTRING,  &(pRes->m_RO_CapacityFilename),     false,   0 },
               { _T("spillway_rc"),      TYPE_CSTRING,  &(pRes->m_spillwayCapacityFilename),false,   0 },
               { _T("cp_dir"),           TYPE_CSTRING,  &(pRes->m_cpdir),                   false,   0 },
               { _T("rule_priorities"),  TYPE_CSTRING,  &(pRes->m_rulePriorityFilename),    false,   0 },
               { _T("rp_dir"),           TYPE_CSTRING,  &(pRes->m_rpdir),                   false,   0 },
               { _T("td_elev"),          TYPE_FLOAT,    &(pRes->m_dam_top_elev),            false,   0 },  // Top of dam elevation
               { _T("fc1_elev"),         TYPE_FLOAT,    &(pRes->m_fc1_elev),                false,   0 },  // Top of primary flood control zone
               { _T("fc2_elev"),         TYPE_FLOAT,    &(pRes->m_fc2_elev),                false,   0 },  // Seconary flood control zone (if applicable)
               { _T("fc3_elev"),         TYPE_FLOAT,    &(pRes->m_fc3_elev),                false,   0 },  // Tertiary flood control zone (if applicable)
               { _T("tailwater_elev"),   TYPE_FLOAT,    &(pRes->m_tailwater_elevation),     false,   0 },
               { _T("turbine_efficiency"),TYPE_FLOAT,   &(pRes->m_turbine_efficiency),      false,   0 },
               { _T("inactive_elev"),    TYPE_FLOAT,    &(pRes->m_inactive_elev),           true,    0 },
               { _T("reservoir_type"),   TYPE_STRING,   &(reservoirType),                   true,    0 },
               { _T("release_freq"),     TYPE_INT,      &(pRes->m_releaseFreq),             false,    0 },
               { _T("p_maintenance"),    TYPE_FLOAT,    &(pRes->m_probMaintenance),         false, 0 },
               { _T("ressim_output_f"),  TYPE_CSTRING,  &(pRes->m_ressimFlowOutputFilename),false,   0 },
               { _T("ressim_output_r"),  TYPE_CSTRING,  &(pRes->m_ressimRuleOutputFilename),false,   0 },
               { NULL,               TYPE_NULL,     NULL,                               false,   0 } };

            ok = TiXmlGetAttributes(pXmlRes, resAttrs, filename, NULL);
            if (!ok)
               {
               CString msg;
               msg.Format(_T("Flow: Misformed element reading <reservoir> attributes in input file %s"), filename);
               Report::ErrorMsg(msg);
               delete pRes;
               pRes = NULL;
               }

            switch (reservoirType[0])
               {
               case _T('F'):
                  {
                  pRes->m_reservoirType = ResType_FloodControl;
                  CString msg;
                  msg.Format(_T("Flow: Reservoir %s is Flood Control"), (LPCTSTR)pRes->m_name);
                  break;
                  }
               case _T('R'):
                  {
                  pRes->m_reservoirType = ResType_RiverRun;
                  CString msg;
                  msg.Format(_T("Flow: Reservoir %s is River Run"), (LPCTSTR)pRes->m_name);
                  break;
                  }
               case _T('C'):
                  {
                  pRes->m_reservoirType = ResType_CtrlPointControl;
                  CString msg;
                  msg.Format(_T("Flow: Reservoir %s is Control Point Control"), (LPCTSTR)pRes->m_name);
                  break;
                  }
               case _T('O'):
               {
                  pRes->m_reservoirType = ResType_Optimized;
                  CString msg;
                  msg.Format(_T("Flow: Reservoir %s is Controlled by Calvin Allocation Model"), (LPCTSTR)pRes->m_name);
                  break;
               }
               default:
                  {
                  CString msg(_T("Flow: Unrecognized 'reservoir_type' attribute '"));
                  msg += reservoirType;
                  msg += _T("' reading <reservoir> tag for Reservoir '");
                  msg += (LPCSTR)pRes->m_name;
                  //      msg += _T("'. This output will be ignored...");
                  Report::ErrorMsg(msg);
                  break;
                  }
               }

            if (pRes != NULL)
               {
               pRes->InitializeReservoirVolume(volume);
               //pModel->m_reservoirArray.Add(pRes);
               pModel->AddReservoir( pRes );
               }

            pXmlRes = pXmlRes->NextSiblingElement(_T("reservoir"));
            }
         }
      }

   //----------- <control points> ------------------------------
   if (pXmlReservoirs != NULL)
      {
      TiXmlElement *pXmlControlPointsParent = pXmlRoot->FirstChildElement(_T("controlPoints"));

      if (pXmlControlPointsParent == NULL)
         pXmlControlPointsParent = pXmlReservoirs;

      if (pXmlControlPointsParent != NULL)
         {
         bool ok = true;

         if (ok)
            {
            TiXmlElement *pXmlCP = pXmlControlPointsParent->FirstChildElement(_T("controlPoint"));

            while (pXmlCP != NULL)
               {
               ControlPoint *pControl = new ControlPoint;

               XML_ATTR cpAttrs[] = {
                  // attr               type         address                                isReq   checkCol
                  { _T("name"),               TYPE_CSTRING,   &(pControl->m_name),                 true,   0 },
                  { _T("id"),                 TYPE_INT,       &(pControl->m_id),                   false,  0 },
                  { _T("path"),               TYPE_CSTRING,   &(pControl->m_dir),                  true,   0 },
                  { _T("control_point"),      TYPE_CSTRING,   &(pControl->m_controlPointFileName), true,   0 },
                  { _T("location"),           TYPE_INT,       &(pControl->m_location),             true,   0 },
                  { _T("reservoirs"),         TYPE_CSTRING,   &(pControl->m_reservoirsInfluenced), true,   0 },
                  { _T("local_col"),          TYPE_INT,       &(pControl->localFlowCol),           false,  0 },
                  { NULL,                  TYPE_NULL,     NULL,                                false,  0 } };

               ok = TiXmlGetAttributes(pXmlCP, cpAttrs, filename, NULL);
               if (!ok)
                  {
                  CString msg;
                  msg.Format(_T("Flow: Misformed element reading <controlPoint> attributes in input file %s"), filename);
                  Report::ErrorMsg(msg);
                  delete pControl;
                  }
               else
                  pModel->m_controlPointArray.Add(pControl);

               pXmlCP = pXmlCP->NextSiblingElement(_T("controlPoint"));
               }
            }
         }
      }  // end of: if ( pXmlReservoirs != NULL)

   //-------- extra state variables next ------------
   StateVar *pSV = new StateVar;  // first is always "water"
   pSV->m_name = _T("water");
   pSV->m_index = 0;
   pSV->m_uniqueID = -1;
   pModel->AddStateVar(pSV);
   LPTSTR appTablePath = NULL;
   LPTSTR paramTablePath = NULL;
   LPCTSTR fieldname = NULL;
   int numTracers = 0;

   TiXmlElement *pXmlStateVars = pXmlRoot->FirstChildElement(_T("additional_state_vars"));
   if (pXmlStateVars != NULL)
      {
      TiXmlElement *pXmlStateVar = pXmlStateVars->FirstChildElement(_T("state_var"));

      while (pXmlStateVar != NULL)
         {
         pSV = new StateVar;

         XML_ATTR svAttrs[] = {
            // attr              type          address                   isReq  checkCol
            { _T("name"), TYPE_CSTRING, &(pSV->m_name), true, 0 },
            { _T("unique_id"), TYPE_INT, &(pSV->m_uniqueID), true, 0 },
            { _T("integrate"), TYPE_INT, &(pSV->m_isIntegrated), false, 0 },

            { _T("fieldname"), TYPE_STRING, &fieldname, false, 0 },

            { _T("applicationTable"), TYPE_STRING, &appTablePath, false, 0 },
            { _T("parameterTable"), TYPE_STRING, &paramTablePath, false, 0 },

            { _T("scalar"), TYPE_FLOAT, &(pSV->m_scalar), false, 0 },
            { _T("numTracers"), TYPE_INT, &numTracers, false, 0 },
            { _T("saveEvery"), TYPE_INT, &pModel->m_detailedSaveInterval, false, 0 },
            { _T("saveAfterYears"), TYPE_INT, &pModel->m_detailedSaveAfterYears, false, 0 },

            { NULL, TYPE_NULL, NULL, false, 0 } };

         ok = TiXmlGetAttributes(pXmlStateVar, svAttrs, filename, NULL);
         if (!ok)
            {
            CString msg;
            msg.Format(_T("Flow: Misformed root element reading <state_var> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            delete pSV;
            pSV = NULL;
            }

         if (pSV)
            pModel->AddStateVar(pSV);

         if (pSV->m_name[0] == _T('R'))//use tracers to do residence time calculation
            {
            for (int i = 1; i < numTracers; i++)//add new tracer for each year of the simulation
               {
               pSV = new StateVar;
               pModel->AddStateVar(pSV);
               }
            }
/*
         CString path;
         if (PathManager::FindPath(appTablePath, path) < 0)
            {
            CString msg(_T("Flow: ESV table source error: Unable to open/read "));
            msg += appTablePath;
            Report::ErrorMsg(msg);
            }

         if (path.IsEmpty() == false)
            {
            DataObj *pDataObj;
            pDataObj = new FDataObj;   // MEMORY LEAK!!!!  NOT SURE WHAT THE INTENTION HERE IS?
            ASSERT(0);

            int rows = pDataObj->ReadAscii(path);

            if (rows <= 0)
               {
               CString msg(_T("Flow: ESV table source error: Unable to open/read "));
               msg += appTablePath;
               Report::ErrorMsg(msg);
               delete pDataObj;
               }
            else
               {
               ParamTable *pTable = new ParamTable;

               pTable->m_pTable = pDataObj;
               pTable->m_type = pDataObj->GetDOType();

               if (fieldname != NULL)
                  {
                  pTable->m_fieldName = fieldname;
                  pTable->m_iduCol = pIDULayer->GetFieldCol(fieldname);

                  if (pTable->m_iduCol < 0)
                     {
                     CString msg(_T("Flow: ESV table source error: column '"));
                     msg += fieldname;
                     msg += _T("; not found in IDU coverage");
                     Report::ErrorMsg(msg);
                     }
                  }

               pTable->CreateMap();
               pSV->m_tableArray.Add(pTable);
               }
            }

         // read in esv parameter  Table
         //pSV->m_paramTablePath = paramTablePath;
         path.Empty();
         if (PathManager::FindPath(paramTablePath, path) < 0)
            {
            CString msg(_T("Flow: ESV table source error: Unable to open/read "));
            msg += appTablePath;
            Report::ErrorMsg(msg);
            }

         if (path.IsEmpty() == false)
            {
            pSV->m_paramTable = new VDataObj;
            pSV->m_paramTable->ReadAscii(path, _T(','));
            }
*/
         pXmlStateVar = pXmlStateVar->NextSiblingElement(_T("state_var"));
         }
      }

   // set state variable count.  Assumes all pools have the same number of extra state vars
   pModel->m_hruSvCount = pModel->m_reachSvCount = pModel->m_reservoirSvCount = pModel->GetStateVarCount();

   // tables next
   TiXmlElement *pXmlTables = pXmlRoot->FirstChildElement(_T("tables"));
   if (pXmlTables != NULL)
      {
      // get tables next
      TiXmlElement *pXmlTable = pXmlTables->FirstChildElement(_T("table"));
      while (pXmlTable != NULL)
         {
         LPCTSTR name = NULL;
         LPCTSTR description = NULL;
         LPCTSTR fieldname = NULL;
         LPCTSTR type = NULL;
         LPCTSTR source = NULL;
         XML_ATTR attrs[] = {
            // attr             type            address                         isReq checkCol
            { _T("name"),           TYPE_STRING,   &name,               true,  0 },
            { _T("description"),    TYPE_STRING,   &description,        false, 0 },
            { _T("type"),           TYPE_STRING,   &type,               true,  0 },
            { _T("col"),            TYPE_STRING,   &fieldname,          false, 0 },
            { _T("source"),         TYPE_STRING,   &source,             true,  0 },
            { NULL,             TYPE_NULL,     NULL,                true,  0 }
            };

         bool ok = TiXmlGetAttributes(pXmlTable, attrs, filename);

         if (!ok)
            {
            CString msg;
            msg.Format(_T("Flow: Misformed table element reading <table> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            }
         else
            {
            DataObj *pDataObj;
            DataObj *pInitDataObj;

            if (*type == _T('v'))
               {
               pDataObj = new VDataObj(U_UNDEFINED);
               pInitDataObj = new VDataObj(U_UNDEFINED);
               }
            else if (*type == _T('i'))
               {
               pDataObj = new IDataObj(U_UNDEFINED);
               pInitDataObj = new IDataObj(U_UNDEFINED);
               }
            else
               {
               pDataObj = new FDataObj(U_UNDEFINED);
               pInitDataObj = new FDataObj(U_UNDEFINED);
               }

            CString path;
            if (PathManager::FindPath(source, path) < 0)
               {
               CString msg;
               msg.Format(_T("Flow: Specified source table '%s' for <table> '%s' can not be found.  This table will be ignored"), source, name);
               Report::LogError(msg);
               }
            else
               {
               int rows = pDataObj->ReadAscii(path);
               pInitDataObj->ReadAscii(path);
               if (rows <= 0)
                  {
                  CString msg(_T("Flow: Flux table source error: Unable to open/read "));
                  msg += source;
                  Report::ErrorMsg(msg);
                  delete pDataObj;
                  delete pInitDataObj;
                  }
               else
                  {
                  ParamTable *pTable = new ParamTable;

                  pTable->m_pTable = pDataObj;
                  pTable->m_pInitialValuesTable = pDataObj;  ///???? 
                  pTable->m_pTable->SetName(name);
                  pTable->m_type = pDataObj->GetDOType();
                  pTable->m_description = description;

                  if (fieldname != NULL)
                     {
                     pTable->m_fieldName = fieldname;
                     pTable->m_iduCol = pIDULayer->GetFieldCol(fieldname);

                     if (pTable->m_iduCol < 0)
                        {
                        CString msg(_T("Flow: Flux table source error: column '"));
                        msg += fieldname;
                        msg += _T("' not found in IDU coverage");
                        Report::ErrorMsg(msg);
                        }
                     }

                  pTable->CreateMap();

                  pModel->m_tableArray.Add(pTable);
                  }
               }
            }

         pXmlTable = pXmlTable->NextSiblingElement(_T("table"));
         }
      }

   // Parameter Estimation 
   TiXmlElement *pXmlParameters = pXmlRoot->FirstChildElement(_T("parameterEstimation"));
   if (pXmlParameters != NULL)
      {
      float numberOfRuns;
      float numberOfYears;
      bool estimateParameters;
      float saveResultsEvery;
      float nsThreshold;
      long rnSeed = -1;

      XML_ATTR paramAttrs[] = {
         { _T("estimateParameters"),TYPE_BOOL,    &estimateParameters, true,   0 },
         { _T("numberOfRuns"),      TYPE_FLOAT,   &numberOfRuns,       true,   0 },
         { _T("numberOfYears"),     TYPE_FLOAT,   &numberOfYears,      true,   0 },
         { _T("saveResultsEvery"),  TYPE_FLOAT,   &saveResultsEvery,   true,   0 },
         { _T("nsThreshold"),       TYPE_FLOAT,   &nsThreshold,        false,   0 },
         { _T("randomSeed"),        TYPE_LONG,    &rnSeed,             false,  0 },
         { NULL,                TYPE_NULL,    NULL,                false,  0 } };

      bool ok = TiXmlGetAttributes(pXmlParameters, paramAttrs, filename);
      if (!ok)
         {
         CString msg;
         msg.Format(_T("Flow: Misformed element reading <parameterEstimation> attributes in input file %s"), filename);
         Report::ErrorMsg(msg);
         }
      else
         {
         pModel->m_estimateParameters = estimateParameters;
         pModel->m_numberOfRuns = (int)numberOfRuns;
         pModel->m_numberOfYears = (int)numberOfYears;
         pModel->m_saveResultsEvery = (int)saveResultsEvery;
         pModel->m_nsThreshold = nsThreshold;
         pModel->m_rnSeed = rnSeed;
         //CTime theTime = CTime::GetCurrentTime();
         std::time_t now = std::time(0);
         if (rnSeed >= 0)
            pModel->m_randUnif1.SetSeed(rnSeed);
         else
            pModel->m_randUnif1.SetSeed((long)now);

         //if ( outputPath.IsEmpty() )
         //   {
         //   pModel->m_paramEstOutputPath = pModel->m_path;
         //   pModel->m_paramEstOutputPath += _T("\\output\\");
         //   }
         //else
         //   pModel->m_paramEstOutputPath += outputPath;
         }
      // get tables next
      TiXmlElement *pXmlParameter = pXmlParameters->FirstChildElement(_T("parameter"));
      while (pXmlParameter != NULL)
         {
         LPCTSTR name = NULL;
         LPCTSTR table = NULL;
         float value = 0.0f;
         float minValue = 0.0f;
         float maxValue = 0.0f;
         bool distributeSpatially = true;
         int id = NULL;
         XML_ATTR attrs[] = {
            // attr             type            address                         isReq checkCol
            { _T("table"),    TYPE_STRING,  &table,        true,  0 },
            { _T("name"),     TYPE_STRING,   &name,       false, 0 },
            { _T("value"),    TYPE_FLOAT,   &value,       false, 0 },
            { _T("minValue"), TYPE_FLOAT,   &minValue,    false, 0 },
            { _T("maxValue"), TYPE_FLOAT,   &maxValue,    false, 0 },
            { _T("distributeSpatially"),  TYPE_BOOL,   &distributeSpatially,    false, 0 },
            { NULL,       TYPE_NULL,     NULL,        true,  0 }
            };

         bool ok = TiXmlGetAttributes(pXmlParameter, attrs, filename);

         if (!ok)
            {
            CString msg;
            msg.Format(_T("Flow: Misformed element reading <reach_location> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            }
         else
            {
            ParameterValue *pParam = new ParameterValue;
            if (name != NULL)
               {
               pParam->m_name = name;
               pParam->m_table = table;
               pParam->m_value = value;
               pParam->m_minValue = minValue;
               pParam->m_maxValue = maxValue;
               pParam->m_distribute = distributeSpatially;

               }
            pModel->m_parameterArray.Add(pParam);
            }
         pXmlParameter = pXmlParameter->NextSiblingElement(_T("parameter"));
         }
      }


   // output expressions
   TiXmlElement *pXmlModelOutputs = pXmlRoot->FirstChildElement(_T("outputs"));
   if (pXmlModelOutputs != NULL)
      {
      TiXmlElement *pXmlGroup = pXmlModelOutputs->FirstChildElement(_T("output_group"));

      while (pXmlGroup != NULL)
         {
         ModelOutputGroup *pGroup = new ModelOutputGroup;
         pGroup->m_name = pXmlGroup->Attribute(_T("name"));
         if (pGroup->m_name.IsEmpty())
            pGroup->m_name = _T("Flow Model Output");

         m_modelOutputGroupArray.Add(pGroup);

         TiXmlElement *pXmlModelOutput = pXmlGroup->FirstChildElement(_T("output"));

         while (pXmlModelOutput != NULL)
            {
            LPTSTR name = NULL;
            LPTSTR query = NULL;
            LPTSTR expr = NULL;
            bool   inUse = true;
            LPTSTR type = NULL;
            LPTSTR domain = NULL;
            LPCSTR obs = NULL;
            LPCSTR format = NULL;
            float x = 0.0f;
            float y = 0.0f;
            int site = -1;

            XML_ATTR attrs[] = {
               // attr           type         address                 isReq checkCol
               { _T("name"),         TYPE_STRING, &name,                  true,   0 },
               { _T("query"),        TYPE_STRING, &query,                 false,  0 },
               { _T("value"),        TYPE_STRING, &expr,                  true,   0 },
               { _T("in_use"),       TYPE_BOOL,   &inUse,                 false,  0 },
               { _T("type"),         TYPE_STRING, &type,                  false,  0 },
               { _T("domain"),       TYPE_STRING, &domain,                false,  0 },
               { _T("obs"),          TYPE_STRING, &obs,                   false,  0 },
               { _T("format"),       TYPE_STRING, &format,                false,  0 },
               { _T("site"),         TYPE_INT,    &site,                  false,  0 },
               { _T("x"),            TYPE_FLOAT,  &x,                     false,  0 },
               { _T("y"),            TYPE_FLOAT,  &y,                     false,  0 },
               { NULL,           TYPE_NULL,   NULL,                   false,  0 } };

            bool ok = TiXmlGetAttributes(pXmlModelOutput, attrs, filename);

            if (!ok)
               {
               CString msg;
               msg.Format(_T("Flow: Misformed element reading <output> attributes in input file %s"), filename);
               Report::ErrorMsg(msg);
               }
            else
               {
               ModelOutput *pOutput = new ModelOutput;

               pOutput->m_name = name;
               pOutput->m_queryStr = query;

               pOutput->m_inUse = inUse;
               if (obs != NULL)
                  {
                  CString fullPath;
                  if (PathManager::FindPath(obs, fullPath) < 0)
                     {
                     CString msg;
                     msg.Format(_T("Flow: Unable to find observation file '%s' specified for Model Output '%s'"), obs, name);
                     Report::LogWarning(msg);
                     pOutput->m_inUse = false;
                     }
                  else
                     {
                     pOutput->m_pDataObjObs = new FDataObj(U_UNDEFINED);
                     //CString inFile;
                     //inFile.Format(_T("%s%s"),pModel->m_path,obs);    // m_path is not slash-terminated
                     int rows = -1;
                     switch (format[0])
                        {
                        case _T('C'):  case _T('c'):
                        case _T('E'):  case _T('e'):    rows = pOutput->m_pDataObjObs->ReadAscii(fullPath);        break;
                        case _T('U'):  case _T('u'):    rows = this->ReadUSGSFlow(fullPath, *pOutput->m_pDataObjObs);  break;
                        case _T('R'):  case _T('r'):    rows = this->ReadUSGSFlow(fullPath, *pOutput->m_pDataObjObs, 1);  break;//set dataType to 1, representing reservoir levels
                        case _T('T'):  case _T('t'):    rows = this->ReadUSGSFlow(fullPath, *pOutput->m_pDataObjObs, 2);  break;//set dataType to 2, representing stream temperatures
                        default:
                        {
                        CString msg(_T("Flow: Unrecognized observation 'format' attribute '"));
                        msg += type;
                        msg += _T("' reading <output> tag for '");
                        msg += name;
                        msg += _T("'. This output will be ignored...");
                        Report::ErrorMsg(msg);
                        pOutput->m_inUse = false;
                        }
                        }

                     if (rows <= 0)
                        {
                        CString msg;
                        msg.Format(_T("Flow: Unable to load observation file '%s' specified for Model Output '%s'"), (LPCTSTR)fullPath, name);
                        Report::LogWarning(msg);

                        delete pOutput->m_pDataObjObs;
                        pOutput->m_pDataObjObs = NULL;
                        pOutput->m_inUse = false;
                        }

                     pModel->m_numQMeasurements++;
                     }
                  }

               pOutput->m_modelType = MOT_SUM;
               if (type != NULL)
                  {
                  switch (type[0])
                     {
                     case _T('S'):   case _T('s'):      pOutput->m_modelType = MOT_SUM;         break;
                     case _T('A'):   case _T('a'):      pOutput->m_modelType = MOT_AREAWTMEAN;  break;
                     case _T('P'):   case _T('p'):      pOutput->m_modelType = MOT_PCTAREA;     break;
                     default:
                     {
                     CString msg(_T("Flow: Unrecognized 'type' attribute '"));
                     msg += type;
                     msg += _T("' reading <output> tag for '");
                     msg += name;
                     msg += _T("'. This output will be ignored...");
                     Report::ErrorMsg(msg);
                     pOutput->m_inUse = false;
                     }
                     }
                  }

               pOutput->m_modelDomain = MOD_IDU;

               if (domain != NULL)
                  {
                  switch (domain[0])
                     {
                     case _T('i'):   case _T('I'):   pOutput->m_modelDomain = MOD_IDU;         break;
                     case _T('H'):   case _T('h'):   pOutput->m_modelDomain = MOD_HRU;         break;
                     case _T('_'):               pOutput->m_modelDomain = MOD_HRU_IDU;     break;
                     case _T('g'):               pOutput->m_modelDomain = MOD_GRID;        break;

                     case _T('l'):
                     case _T('p'):
                     case _T('L'):
                     case _T('P'):
                     {
                     pOutput->m_modelDomain = MOD_HRULAYER;

                     TCHAR *buffer = new TCHAR[255];
                     TCHAR *nextToken;
                     lstrcpy(buffer, (LPCTSTR)domain);

                     TCHAR *col = _tcstok_s(buffer, _T(":"), &nextToken);
                     CString p;
                     while (col != NULL)
                        {
                        p = col;
                        col = _tcstok_s(NULL, _T(":"), &nextToken);
                        }
                     pOutput->m_number = atoi(p);
                     delete[] buffer;
                     }
                     break;    // need mechanism to ID which layer

                     case _T('r'):
                     case _T('R'):
                     {
                     if (domain[2] == _T('a'))
                        pOutput->m_modelDomain = MOD_REACH;
                     else
                        pOutput->m_modelDomain = MOD_RESERVOIR;
                     }
                     break;

                     case _T('s'):  //????
                        pOutput->m_modelDomain = MOD_RESERVOIR;
                        break;

                     default:
                     {
                     CString msg(_T("Flow: Unrecognized 'domain' attribute '"));
                     msg += type;
                     msg += _T("' reading <output> tag for '");
                     msg += name;
                     msg += _T("'. This output will be ignored...");
                     Report::ErrorMsg(msg);
                     pOutput->m_inUse = false;
                     }
                     }
                  }

               if (expr != NULL)
                  {
                  switch (expr[0])
                     {
                     case _T('e'):
                     {
                     TCHAR *buffer = new TCHAR[255];
                     TCHAR *nextToken = NULL;
                     lstrcpy(buffer, (LPCTSTR)expr);

                     TCHAR *col = _tcstok_s(buffer, _T(":"), &nextToken);

                     CString p;
                     p = col;
                     pOutput->m_exprStr = p;
                     while (col != NULL)
                        {
                        p = col;
                        col = _tcstok_s(NULL, _T(":"), &nextToken);
                        }
                     pOutput->m_esvNumber = atoi(p);
                     delete[] buffer;
                     }
                     break;

                     default:
                        pOutput->m_exprStr = expr;
                     }
                  }

               if (site > 0)
                  pOutput->m_siteNumber = site;

               //  pGroup->Add( pOutput );   
               pOutput->Init(pEnvContext);

               if (pOutput->m_modelDomain == MOD_REACH)
                  pOutput->InitStream(this, this->m_pStreamLayer);

               // check to make sure the query for any stream sites returns a reach
               bool pass = true;
               if (pOutput->m_modelDomain == MOD_REACH  && pOutput->m_pDataObjObs)
                  {
                  pass = false;
                  for (MapLayer::Iterator i = pModel->m_pStreamLayer->Begin(); i < pModel->m_pStreamLayer->End(); i++)
                     {
                     if (pOutput->m_pQuery != NULL)
                        {
                        // does it pass the query?
                        bool result = false;
                        bool ok = pOutput->m_pQuery->Run(i, result);

                        if (ok && result)
                           {
                           pass = true;
                           break;
                           }
                        }
                     }
                  }  // end of: for ( each stream reach )

               // check to make sure the query for any idu sites returns an idu
              // bool pass = true;
               if (pOutput->m_modelDomain == MOD_IDU && pOutput->m_pDataObjObs)
                  {
                  pass = false;
                  for (MapLayer::Iterator i = pIDULayer->Begin(); i < pIDULayer->End(); i++)
                     {
                     if (pOutput->m_pQuery != NULL)
                        {
                        // does it pass the query?
                        bool result = false;
                        bool ok = pOutput->m_pQuery->Run(i, result);

                        if (ok && result)
                           {
                           pass = true;
                           break;
                           }
                        }
                     }
                  }  // end of: for ( each IDU )
               if (pOutput->m_modelDomain == MOD_GRID)
                  {
                  pOutput->m_coord.x = x;
                  pOutput->m_coord.y = y;
                  }  // end of: for ( each IDU )

               // check to make sure the query for any HRU sites returns atleast 1 polygon
               // bool pass = true;
               if (pOutput->m_modelDomain == MOD_HRU)
                  {
                  pass = false;
                  for (MapLayer::Iterator i = pIDULayer->Begin(); i < pIDULayer->End(); i++)
                     {
                     if (pOutput->m_pQuery != NULL)
                        {
                        // does it pass the query?
                        bool result = false;
                        bool ok = pOutput->m_pQuery->Run(i, result);

                        if (ok && result)
                           {
                           pass = true;
                           break;
                           }
                        }
                     }
                  }  // end of: for ( each HRU )

               if (pass)
                  pGroup->Add(pOutput);
               else
                  delete pOutput;
               }

            pXmlModelOutput = pXmlModelOutput->NextSiblingElement(_T("output"));
            }

         pXmlGroup = pXmlGroup->NextSiblingElement(_T("output_group"));
         }
      }

   // video capture
   TiXmlElement *pXmlVideoCapture = pXmlRoot->FirstChildElement(_T("video_capture"));
   if (pXmlVideoCapture != NULL)
      {
      //pModel->m_useRecorder = true;
      pModel->m_useRecorder = true;
      int frameRate = 30;

      XML_ATTR attrs[] = {
         // attr           type         address                 isReq checkCol
         { _T("use"),          TYPE_BOOL,   &pModel->m_useRecorder, false,  0 },
         { _T("frameRate"),    TYPE_INT,    &frameRate,             false,  0 },
         { NULL,           TYPE_NULL,   NULL,                   false,  0 } };

      bool ok = TiXmlGetAttributes(pXmlVideoCapture, attrs, filename);

      if (!ok)
         {
         CString msg;
         msg.Format(_T("Flow: Misformed element reading <video_capture> attributes in input file %s"), filename);
         Report::ErrorMsg(msg);
         }
      else
         {
         //pModel->m_pVideoRecorder = new VideoRecorder;

         TiXmlElement *pXmlMovie = pXmlVideoCapture->FirstChildElement(_T("movie"));

         while (pXmlMovie != NULL)
            {
            LPTSTR field = NULL;
            LPTSTR file = NULL;

            XML_ATTR attrs[] = {
               // attr          type            address   isReq checkCol
               { _T("col"),     TYPE_STRING,   &field,    true,   1 },
               { _T("file"),    TYPE_STRING,   &file,     true,   0 },
               { NULL,          TYPE_NULL,      NULL,     false,  0 } };

            bool ok = TiXmlGetAttributes(pXmlMovie, attrs, filename, pIDULayer);

            if (!ok)
               {
               CString msg;
               msg.Format(_T("Flow: Misformed element reading <movie> attributes in input file %s"), filename);
               Report::ErrorMsg(msg);
               }
            else
               {
               int col = pIDULayer->GetFieldCol(field);

               if (col >= 0)
                  {
                  int vrID = EnvAddVideoRecorder( /*VRT_MAPPANEL*/ 2, _T("Flow Results"), file, 30, VRM_CALLDIRECT, col);
                  pModel->m_vrIDArray.Add(vrID);
                  }
               }

            pXmlMovie = pXmlMovie->NextSiblingElement(_T("movie"));
            }
         }
      }  // end of <video capture>

   return true;
   }



int FlowModel::IdentifyMeasuredReaches(void)
   {
   int count = 0;

   //for ( MapLayer::Iterator i=this->m_pStreamLayer->Begin(); i < m_pStreamLayer->End(); i++ )
   for (int i = 0; i < (int)m_reachArray.GetSize(); i++)
      {
      Reach *pReach = m_reachArray[i];

      int polyIndex = pReach->m_polyIndex;

      for (int k = 0; k < (int) this->m_modelOutputGroupArray.GetSize(); k++)
         {
         ModelOutputGroup *pGroup = m_modelOutputGroupArray[k];

         for (int j = 0; j < (int)pGroup->GetSize(); j++)
            {
            ModelOutput *pOutput = pGroup->GetAt(j);

            if (pOutput->m_modelDomain == MOD_REACH && pOutput->m_pQuery != NULL)
               {
               // does it pass the query?
               bool result = false;
               bool ok = pOutput->m_pQuery->Run(polyIndex, result);

               if (ok && result)
                  {
                  pReach->m_isMeasured = true;
                  count++;
                  }
               }
            }
         }
      }

   return count;
   }


/*
int FlowModel::ParseHRULayerDetails(CString &aggCols, int detail)
   {
   CStringArray tokens;
   int count = ::Tokenize(aggCols, _T(":,"), tokens);

   for (int i = 0; i < count; i++)
      {
      switch (detail)
         {
         case 0:
            m_hruLayerNames.Add(tokens[i]);
            break;
         case 1:
            m_hruLayerDepths.Add(tokens[i]);
            break;
         case 2:
            m_initWaterContent.Add(tokens[i]);
            break;

         case 4:
            break;
         }
      }

   return count;
   }
*/

bool FlowModel::InitializeParameterEstimationSampleArray(void)
   {
   int moCount = 0;//number of outputs with measured time series
   for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];
      for (int i = 0; i < (int)pGroup->GetSize(); i++)
         {
         if (pGroup->GetAt(i)->m_pDataObjObs != NULL)
            {
            moCount++;
            FDataObj *pErrorStatData = new FDataObj(7, 0, U_UNDEFINED);
            // FDataObj *pParameterData = new FDataObj((int)m_parameterArray.GetSize(),0);
            FDataObj *pDischargeData = new FDataObj(1, 1, U_UNDEFINED);
            // m_mcOutputTables.Add(pParameterData);
            m_mcOutputTables.Add(pErrorStatData);
            m_mcOutputTables.Add(pDischargeData);

            pErrorStatData->SetLabel(0, _T("RunNumber"));
            pErrorStatData->SetLabel(1, _T("NSE"));
            pErrorStatData->SetLabel(2, _T("Log_NSE"));
            pErrorStatData->SetLabel(3, _T("VolumeError"));
            pErrorStatData->SetLabel(4, _T("NotUsed"));
            pErrorStatData->SetLabel(5, _T("NotUsed"));
            pErrorStatData->SetLabel(6, _T("NotUsed"));
            }
         }
      }
   m_pParameterData = new FDataObj((int)m_parameterArray.GetSize() + 1, 0, U_UNDEFINED);
   m_pParameterData->SetLabel(0, _T("RunNumber"));
   for (int j = 0; j < (int)m_parameterArray.GetSize(); j++)
      {
      CString name;
      name.Format(_T("%s:%s"), (LPCTSTR)m_parameterArray.GetAt(j)->m_table, (LPCTSTR)m_parameterArray.GetAt(j)->m_name);
      m_pParameterData->SetLabel(j + 1, name);
      }
   /*
   m_pErrorStatData = new FDataObj((int)m_reachMeasuredDataArray.GetSize()*2,0);
   m_pParameterData = new FDataObj((int)m_parameterArray.GetSize(),0);
   m_pDischargeData = new FDataObj((int)m_pReachDischargeData->GetRowCount(),1);

   CString msg;
   int meas=0;
   for ( int j=0; j < (int) m_reachSampleLocationArray.GetSize(); j++)
      {
      if ( m_reachSampleLocationArray.GetAt(j)->m_pMeasuredData != NULL )
         {
         m_pErrorStatData->SetLabel( meas, _T("NS_")+m_reachSampleLocationArray.GetAt(j)->m_name  );
         meas++;
         m_pErrorStatData->SetLabel( meas, _T("NS_LN_")+m_reachSampleLocationArray.GetAt(j)->m_name  );
         meas++;
         }
      }

   for ( int j=0; j < (int) m_parameterArray.GetSize(); j++)
         m_pParameterData->SetLabel( j, m_parameterArray.GetAt(j)->m_name  );
         */
   return true;
   }

/*
bool FlowModel::InitializeHRULayerSampleArray(void)
   {
   int numLocations = (int) m_reachSampleLocationArray.GetSize()*(m_soilLayerCount+m_vegLayerCount);
   m_pHRUPrecipData = new FDataObj( int( m_reachSampleLocationArray.GetSize()) +1  , 0 );
   m_pHRUPrecipData->SetLabel( 0, _T("Time (Days)")) );

   m_pHRUETData = new FDataObj( int( m_reachSampleLocationArray.GetSize() )+1, 0 );
   m_pHRUETData->SetLabel( 0, _T("Time (Days)")) );
   CString label;

   // for each sample location, allocate and label an FDataObj for storing HRUPool water contents
   for (int i=0; i < (int) m_reachSampleLocationArray.GetSize();i++)
      {
      FDataObj *pHruLayerData = new FDataObj(m_soilLayerCount+m_vegLayerCount+1,0);
      pHruLayerData->SetLabel( 0, _T("Time (Days)")) );

      for (int j=0; j < m_soilLayerCount+m_vegLayerCount; j++)
        {
        label.Format( _T("%s")), (LPCTSTR) m_hruLayerNames.GetAt(j));
        pHruLayerData->SetLabel( j+1, (char*)(LPCSTR) label );
        }
      m_reachHruLayerDataArray.Add( pHruLayerData );
      }


   //next look for any extra state variables
   for (int l=0;l<m_hruSvCount-1;l++)
      {
      // for each sample location, allocate and label an FDataObj for storing HRUPool water contents
      for (int i=0; i < (int) m_reachSampleLocationArray.GetSize();i++)
         {
         FDataObj *pHruLayerData = new FDataObj(m_soilLayerCount+m_vegLayerCount+1,0);
         pHruLayerData->SetLabel( 0, _T("Time (Days)")) );

         for (int j=0; j < m_soilLayerCount+m_vegLayerCount; j++)
            {
            label.Format( _T("%s")), (LPCTSTR) m_hruLayerNames.GetAt(j));
            pHruLayerData->SetLabel( j+1, (char*)(LPCSTR) label );
            }
         m_hruLayerExtraSVDataArray.Add( pHruLayerData );
         }
      }


   // next, initialize the m_pHRUPrecipData object.  It contains a column for precipitation at each sample location,
   int count=0;
   for (int i=0; i < (int)m_reachSampleLocationArray.GetSize(); i++)
      {
     // for (int j=0; j < m_soilLayerCount+m_vegLayerCount; j++)
       //  {
         ReachSampleLocation *pSampleLocation = m_reachSampleLocationArray.GetAt(i);
         label.Format( _T("%s")), (LPCTSTR) pSampleLocation->m_name, i );
         m_pHRUPrecipData->SetLabel( count+1, label );
         m_pHRUETData->SetLabel( count+1, label );
         count++;
     //    }
      }

   for (int i=0; i < (int)m_reachSampleLocationArray.GetSize(); i++)
      {
      ReachSampleLocation *pSampleLocation = m_reachSampleLocationArray.GetAt(i);
      if ( pSampleLocation->m_pMeasuredData != NULL)
         {
         FDataObj *pData = new FDataObj(3,0);
         pData->SetLabel( 0, _T("Time (Days)")) );
         label.Format( _T("%s Mod")), pSampleLocation->m_name );
         pData->SetLabel( 1, label );
         label.Format( _T("%s Meas")), pSampleLocation->m_name );
         pData->SetLabel( 2, label );
         m_reachMeasuredDataArray.Add( pData );
         }
      }

   int numFound=0;
   for ( INT_PTR j=0; j < m_reachArray.GetSize(); j++ )
      {
      Reach *pReach = m_reachArray[j];
      for (int i=0;i<(int)m_reachSampleLocationArray.GetSize();i++)
         {
        // for (int k=0;k<m_soilLayerCount+m_vegLayerCount;k++)
          //  {
            ReachSampleLocation *pSam = m_reachSampleLocationArray.GetAt(i);
            if ( pReach->m_reachID == pSam->m_id )
               {
               if ( pReach->m_catchmentArray.GetSize() > 0 )
                  {
                  numFound++;
                  pSam->m_pHRU = pReach->m_catchmentArray[ 0 ]->GetHRU( 0 );
                  }
               break;
               }
          //  }

         }
      //next look for gridded locations

      for (int i=0;i<(int)m_reachSampleLocationArray.GetSize();i++)
         {
         ReachSampleLocation *pSam = m_reachSampleLocationArray.GetAt(i);
         for ( INT_PTR j=0; j < m_catchmentArray.GetSize(); j++ )
            {
            Catchment *pCatch = m_catchmentArray[j];
            for (int k=0;k<pCatch->GetHRUCount();k++)
               {
               HRU *pHRU=pCatch->GetHRU(k);
               if ( pHRU->m_climateRow == pSam->row && pHRU->m_climateCol == pSam->col  )
                  {
                  pSam->m_pHRU = pHRU;
                  break;
                  }
               }
            }
         }
      }
   bool ret = false;
   if (numFound==numLocations)
      ret = true;
   return ret;
   }
   */

void FlowModel::CollectData(int dayOfYear)
   {
   // reservoirs
   for (int i = 0; i < (int) this->m_reservoirArray.GetSize(); i++)
      {
      Reservoir *pRes = m_reservoirArray.GetAt(i);

      ASSERT(pRes->m_pResData != NULL);

      if (pRes->m_reservoirType == ResType_CtrlPointControl)
         {
         float *data = new float[4];
         data[0] = (float)m_timeInRun;
         data[1] = pRes->GetPoolElevationFromVolume();
         data[2] = pRes->m_inflow / SEC_PER_DAY;
         data[3] = pRes->m_outflow / SEC_PER_DAY;

         pRes->m_pResData->AppendRow(data, 4);

         delete[] data;
         }
      else if (pRes->m_reservoirType == ResType_Optimized)
         {
         float *data = new float[3];
         data[0] = (float)m_timeInRun;
         data[1] = pRes->m_inflow / SEC_PER_DAY;
         data[2] = pRes->m_outflow / SEC_PER_DAY;

         pRes->m_pResData->AppendRow(data, 3);

         delete[] data;
         }
      else
         {
         // float data [ 8 ];
         float *data = new float[8];
         data[0] = (float)m_timeInRun;
         data[1] = pRes->GetPoolElevationFromVolume();
         data[2] = pRes->GetTargetElevationFromRuleCurve(dayOfYear);
         data[3] = pRes->m_inflow / SEC_PER_DAY;
         data[4] = pRes->m_outflow / SEC_PER_DAY;
         data[5] = pRes->m_powerFlow;
         data[6] = pRes->m_RO_flow;
         data[7] = pRes->m_spillwayFlow;

         pRes->m_pResData->AppendRow(data, 8);

         delete[] data;
         }

      if (pRes->m_pResSimRuleOutput != NULL)
         {
         VData vtime = m_timeInRun;
         vtime.ChangeType(TYPE_INT);
         VData *Rules = new VData[7];

         VData activerule;
         activerule.ChangeType(TYPE_CSTRING);
         activerule = pRes->m_activeRule;

         VData blank;
         blank.ChangeType(TYPE_CSTRING);
         blank = _T("none");

         VData zone;
         zone.ChangeType(TYPE_INT);
         zone = pRes->m_zone;

         VData outflow;
         outflow.ChangeType(TYPE_FLOAT);
         outflow = pRes->m_outflow / SEC_PER_DAY;

         VData inflow;
         inflow.ChangeType(TYPE_FLOAT);
         inflow = pRes->m_inflow / SEC_PER_DAY;

         VData elevation;
         elevation.ChangeType(TYPE_FLOAT);
         elevation = pRes->m_elevation;

         Rules[0] = vtime;
         Rules[1] = blank;// ressimrule;
         Rules[2] = inflow;
         Rules[3] = outflow;
         Rules[4] = elevation;
         Rules[5] = zone;
         Rules[6] = activerule;

         pRes->m_pResSimRuleCompare->AppendRow(Rules, 7);
         delete[] Rules;
         }  //end of if (pRes->m_pResSimRuleOutput != NULL)

      if (dayOfYear >= JUN_1 && dayOfYear <= AUG_31)     // assume doy is zero-based
         {
         pRes->m_avgPoolVolJunAug += (float)pRes->m_volume;   // TEMPORARY
         pRes->m_avgPoolElevJunAug += (float)pRes->m_elevation;   // TEMPORARY
         }
      }   //end of  for ( int i=0; i < (int) this->m_reservoirArray.GetSize(); i++ )
   }


// called daily (or whatever timestep FLOW is running at)
void FlowModel::UpdateHRULevelVariables(EnvContext *pEnvContext)
   {
   float time = float(m_currentTime - (int)m_currentTime);
   float dt = m_flowContext.timeStep;
   float currTime = m_flowContext.dayOfYear + time;
   // currTime = m_currentTime;
   m_pCatchmentLayer->m_readOnly = false;

   //   #pragma omp parallel for 
   for (int i = 0; i < (int) this->m_hruArray.GetSize(); i++)
      {
      HRU *pHRU = m_hruArray[i];

      // climate
      float precip = 0.0f;
      GetHRUClimate(CDT_PRECIP, pHRU, (int)currTime, pHRU->m_currentPrecip);
      GetHRUClimate(CDT_TMEAN, pHRU, (int)currTime, pHRU->m_currentAirTemp);
      GetHRUClimate(CDT_TMIN, pHRU, (int)currTime, pHRU->m_currentMinTemp);
      GetHRUClimate(CDT_TMAX, pHRU, (int)currTime, pHRU->m_currentMaxTemp);
      GetHRUClimate(CDT_SOLARRAD, pHRU, (int)currTime, pHRU->m_currentRadiation);
      //ASSERT(pHRU->m_currentMaxTemp > -100.0f);
      if (pHRU->m_currentAirTemp < -100)
      {
          pHRU->m_currentAirTemp = 5;
          pHRU->m_currentPrecip = 0;
          pHRU->m_currentMinTemp = 5;
          pHRU->m_currentMaxTemp = 5;
      }
      pHRU->m_precip_yr += (float) (pHRU->m_currentPrecip*m_timeStep);         // mm
      pHRU->m_precip_wateryr += (float) (pHRU->m_currentPrecip*m_timeStep);         // mm
      pHRU->m_rainfall_yr += (float) (pHRU->m_currentRainfall*m_timeStep);       // mm
      pHRU->m_snowfall_yr += (float) (pHRU->m_currentSnowFall*m_timeStep);       // mm
      pHRU->m_temp_yr += (float) (pHRU->m_currentAirTemp*m_timeStep);
      pHRU->m_gwRecharge_yr += (float) (pHRU->m_currentGWRecharge*m_timeStep);       // mm
      pHRU->m_gwFlowOut_yr += (float) (pHRU->m_currentGWFlowOut*m_timeStep);

      // hydrology
      pHRU->m_depthMelt = 0;
     // pHRU->m_depthSWE  = 0;
      for ( int j=0; j < this->m_poolInfoArray.GetSize(); j++ )
         {
         switch(this->m_poolInfoArray[j]->m_type )
            {
            case PT_VEG:
           //    pHRU->m_depthMelt += float(pHRU->GetPool(j)->m_volumeWater / pHRU->m_area); // volume of ice in snow 
               break;

            case PT_SNOW:
            //   pHRU->m_depthSWE += float(pHRU->GetPool(j)->m_volumeWater / pHRU->m_area); // volume of ice in snow 
               break;
            }
         }

    //  pHRU->m_depthSWE += pHRU->m_depthMelt; // SWE includes melt + ice

      pHRU->m_maxET_yr += float( pHRU->m_currentMaxET*m_timeStep );
      pHRU->m_et_yr += float( pHRU->m_currentET*m_timeStep );
      pHRU->m_runoff_yr += float( pHRU->m_currentRunoff*m_timeStep );

      // THIS SHOULDN"T BE HERE!!!!!!
      /*
      for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
         {
         int idu = pHRU->m_polyIndexArray[k];
         float snow = 0;   // mm
         for (int j = 0; j < this->m_poolInfoArray.GetSize(); j++)
            {
            switch (this->m_poolInfoArray[j]->m_type)
               {
               case PT_SNOW:
                  snow += ((float)pHRU->GetPool(j)->m_volumeWater / pHRU->m_area ) * MM_PER_M;  // m3 / m2 
                  break;
               }
            }

         this->UpdateIDU(pEnvContext, idu, m_colHruSWE, pHRU->m_depthSWE, SET_DATA);  // SetData, not AddDelta   // m_colHruSWE_yr
         }
         */
      }

   m_pCatchmentLayer->m_readOnly = true;
   }


bool FlowModel::GetHRUData(HRU *pHRU, int col, VData &value, DAMETHOD method)
   {
   if (method == DAM_FIRST)
      {
      if (pHRU->m_polyIndexArray.IsEmpty())
         return false;

      return m_pCatchmentLayer->GetData(pHRU->m_polyIndexArray[0], col, value);
      }
   else
      {
      DataAggregator da(m_pCatchmentLayer);
      da.SetPolys((int*)(pHRU->m_polyIndexArray.GetData()),
         (int)pHRU->m_polyIndexArray.GetSize());
      da.SetAreaCol(m_colCatchmentArea);
      return da.GetData(col, value, method);
      }
   }


bool FlowModel::GetHRUData(HRU *pHRU, int col, bool  &value, DAMETHOD method)
   {
   VData _value;
   if (GetHRUData(pHRU, col, _value, method) == false)
      return false;

   return _value.GetAsBool(value);
   }


bool FlowModel::GetHRUData(HRU *pHRU, int col, int  &value, DAMETHOD method)
   {
   VData _value;
   if (GetHRUData(pHRU, col, _value, method) == false)
      return false;

   return _value.GetAsInt(value);
   }


bool FlowModel::GetHRUData(HRU *pHRU, int col, float &value, DAMETHOD method)
   {
   VData _value;
   if (GetHRUData(pHRU, col, _value, method) == false)
      return false;

   return _value.GetAsFloat(value);
   }



bool FlowModel::GetCatchmentData(Catchment *pCatchment, int col, VData &value, DAMETHOD method)
   {
   if (method == DAM_FIRST)
      {
      HRU *pHRU = pCatchment->GetHRU(0);
      if (pHRU == NULL || pHRU->m_polyIndexArray.IsEmpty())
         return false;

      return m_pCatchmentLayer->GetData(pHRU->m_polyIndexArray[0], col, value);
      }
   else
      {
      DataAggregator da(m_pCatchmentLayer);

      int hruCount = pCatchment->GetHRUCount();
      for (int i = 0; i < hruCount; i++)
         {
         HRU *pHRU = pCatchment->GetHRU(i);

         da.AddPolys((int*)(pHRU->m_polyIndexArray.GetData()),
            (int)pHRU->m_polyIndexArray.GetSize());
         }

      da.SetAreaCol(m_colCatchmentArea);
      return da.GetData(col, value, method);
      }
   }


bool FlowModel::GetCatchmentData(Catchment *pCatchment, int col, int &value, DAMETHOD method)
   {
   VData _value;
   if (GetCatchmentData(pCatchment, col, _value, method) == false)
      return false;

   return _value.GetAsInt(value);
   }


bool FlowModel::GetCatchmentData(Catchment *pCatchment, int col, bool &value, DAMETHOD method)
   {
   VData _value;
   if (GetCatchmentData(pCatchment, col, _value, method) == false)
      return false;

   return _value.GetAsBool(value);
   }


bool FlowModel::GetCatchmentData(Catchment *pCatchment, int col, float &value, DAMETHOD method)
   {
   VData _value;
   if (GetCatchmentData(pCatchment, col, _value, method) == false)
      return false;

   return _value.GetAsFloat(value);
   }



bool FlowModel::SetCatchmentData(Catchment *pCatchment, int col, VData value)
   {
   if (col < 0)
      return false;

   for (int i = 0; i < pCatchment->GetHRUCount(); i++)
      {
      HRU *pHRU = pCatchment->GetHRU(i);

      for (int j = 0; j < (int)pHRU->m_polyIndexArray.GetSize(); j++)
         m_pCatchmentLayer->SetData(pHRU->m_polyIndexArray[j], col, value);
      }
   return true;
   }


bool FlowModel::GetReachData(Reach *pReach, int col, VData &value)
   {
   int record = pReach->m_polyIndex;
   return m_pStreamLayer->GetData(record, col, value);
   }

bool FlowModel::GetReachData(Reach *pReach, int col, bool &value)
   {
   int record = pReach->m_polyIndex;
   return m_pStreamLayer->GetData(record, col, value);
   }


bool FlowModel::GetReachData(Reach *pReach, int col, int &value)
   {
   int record = pReach->m_polyIndex;
   return m_pStreamLayer->GetData(record, col, value);
   }

bool FlowModel::GetReachData(Reach *pReach, int col, float &value)
   {
   int record = pReach->m_polyIndex;
   return m_pStreamLayer->GetData(record, col, value);
   }



//-- ::ReadAscii() ------------------------------------------
//
//--  Opens and loads the file into the data object
//-------------------------------------------------------------------

int FlowModel::ReadUSGSFlow(LPCSTR _fileName, FDataObj &dataObj, int dataType)
   {
   CString fileName;
   PathManager::FindPath(_fileName, fileName);

   HANDLE hFile = CreateFile(fileName,
      GENERIC_READ, // open for reading
      FILE_SHARE_READ, // do not share (deprecated - jb)
      NULL, // no security
      OPEN_EXISTING, // existing file only
      FILE_ATTRIBUTE_NORMAL, // normal file
      NULL); // no attr. template

   //FILE *fp;
   //fopen_s( &fp, fileName, _T("rt")) );     // open for "read"

   if (hFile == INVALID_HANDLE_VALUE)
      {
      CString msg(_T("Flow: ReadUSGS: Couldn't find file '"));
      msg += fileName;
      msg += _T("'. ");

      DWORD dw = GetLastError();
      LPVOID lpMsgBuf = NULL;

      FormatMessage(
         FORMAT_MESSAGE_ALLOCATE_BUFFER |
         FORMAT_MESSAGE_FROM_SYSTEM |
         FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL,
         dw,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
         (LPTSTR)&lpMsgBuf,
         0, NULL);

      msg += (LPTSTR)lpMsgBuf;
      LocalFree(lpMsgBuf);
      Report::LogWarning(msg);
      return -1;
      }

   dataObj.Clear();   // reset data object, strings

   LARGE_INTEGER _fileSize;
   BOOL ok = GetFileSizeEx(hFile, &_fileSize);
   ASSERT(ok);

   DWORD fileSize = (DWORD)_fileSize.LowPart;

   //long fileSize = _filelength ( _fileno( fp ) );

   TCHAR *buffer = new TCHAR[fileSize + 2];
   memset(buffer, 0, (fileSize + 2) * sizeof(TCHAR));

   //fread ( buffer, fileSize, 1, fp );
   //fclose( fp );
   DWORD bytesRead = 0;
   ReadFile(hFile, buffer, fileSize, &bytesRead, NULL);


   //-- skip any leading comments --//
   char *p = buffer;
   while (*p == _T('#') || *p == _T('a') || *p == _T('5'))
      {
      p = strchr(p, _T('\n'));
      p++;
      }

   //-- start parsing buffer --//
   char *end = strchr(p, _T('\n'));

   //-- NULL-terminate then get column count --//
   *end = NULL;
   int  _cols = 7;

   while (*p != NULL)
      {
      if (*p++ == _T('   '))    // count the number of delimiters
         _cols++;
      }

   dataObj.SetSize(2, 0);  // resize matrix, statArray

   //-- set labels from first line (labels are delimited) --//
   char d[4];
   d[0] = _T('\t');
   d[1] = _T('-');
   d[2] = _T('\n');
   d[3] = NULL;
   char *next;

   p = strtok_s(buffer, d, &next);
   _cols = 0;

   while (p != NULL)
      {
      while (*p == _T(' ')) p++;   // strip leading blanks

     // SetLabel( _cols++, p );
      p = strtok_s(NULL, d, &next);   // look for delimiter or newline
      }

   //-- ready to start parsing data --//
   int cols = 7;
   int c = 2;
   float *data = new float[c];
   memset(data, 0, c * sizeof(float));

   p = strtok_s(end + 1, d, &next);  // look for delimiter

   while (p != NULL)
      {
      //-- get a row of data --//
      int year, month, day = -1;

      for (int i = 0; i < cols; i++)
         {
         if (*p == _T('U'))//at the start of a line
            i = 0;
         else if (i == 2)//year
            year = atoi(p);
         else if (i == 3)//month
            month = atoi(p);
         else if (i == 4)//day  
            day = atoi(p);
         else if (i == 5)//q converted to m3/s
            {
            data[1] = ((float)atof(p)) / 35.31f;

            if (dataType == 1) //reservoir level, ft
               data[1] = ((float)atof(p)) / 3.28f;//convert to meters
            else if (dataType == 2)
               data[1] = (float)(atof(p));

            //COleDateTime time;
            //time.SetDate(year,month,day);
            int dayOfYear = ::GetDayOfYear(month, day, year);   // 1-based
            float time = (year*365.0f) + dayOfYear - 1;   // days since 0000

            data[0] = time;
            }

         //data[ i ] =  (float) atof( p ) ;
         p = strtok_s(NULL, d, &next);  // look for delimeter

         //-- invalid line? --//
         if (p == NULL && i < cols - 1)
            break;
         }

      dataObj.AppendRow(data, c);
      }

   //-- clean up --//
   delete[] data;
   delete[] buffer;

   return dataObj.GetRowCount();
   }

float FlowModel::GetObjectiveFunction(FDataObj *pData, float &ns, float &nsLN, float &ve)
   {
   int meas = 0;
   int offset = 0;
   int n = pData->GetRowCount();
   float obsMean = 0.0f, predMean = 0.0f, obsMeanL = 0.0f, predMeanL = 0.0f;
   int firstSample = int( 180 / m_timeStep );
   for (int j = firstSample; j < n; j++) //50this skips the first 50 days. Assuming those values
      // might be unacceptable due to initial conditions...
      {
      float time = 0.0f; float obs = 0.0f; float pred = 0.0f;
      pData->Get(0, j, time);
      pData->Get(2, j, obs); //add up all the discharge values and then...
      obsMean += obs;
      if (obs!=0.0f)
         obsMeanL += log10(obs);
      pData->Get(1, j, pred);
      predMean += pred;
      if (pred!=0.0f)
         predMeanL += log10(pred);
      }
   obsMean = obsMean / ((float)n - (float)firstSample); // divide them by the sample size to generate the average.
   //predMean = predMean/((float)n-(float)firstSample); // divide them by the sample size to generate the average.
   obsMeanL = obsMeanL / ((float)n - (float)firstSample);
   //predMeanL=predMeanL/((float)n-(float)firstSample);

   double errorSum = 0.0f, errorSumDenom = 0.0f, errorSumLN = 0.0f, errorSumDenomLN = 0.0f, r2Denom1 = 0.0f, r2Denom2 = 0.0f; float ve_numerator = 0.0f; float ve_denominator = 0.0f;
   for (int row = firstSample; row < n; row++)
      {
      float pred = 0.0f, obs = 0.0f; float time = 0.0f;
      pData->Get(0, row, time);
      pData->Get(2, row, obs);
      pData->Get(1, row, pred); // Assuming col i holds data for the reach corresponding to measurement

      errorSum += ((obs - pred)*(obs - pred));
      errorSumDenom += ((obs - obsMean)*(obs - obsMean));

      ve_numerator += (obs - pred);
      ve_denominator += obs;

      double a = 0.0f;
      double b = 0.0f;
      if (obs > 0.0f)
        {
         a = log10(obs) - log10(pred);
         b = log10(obs) - obsMeanL;
         }
      // double b = log10(obs) - log10(obsMean);
      errorSumLN += pow(a, 2);
      errorSumDenomLN += pow(b, 2);
      }

   if (errorSumDenom > 0.0f)
      {
      ns = (float)(1.0f - (errorSum / errorSumDenom));
      nsLN = (float)(1.0f - (errorSumLN / errorSumDenomLN));
      ve = (float)ve_numerator / ve_denominator;
      }
   return ns;
   }

float FlowModel::GetObjectiveFunctionGrouped(FDataObj *pData, float &ns, float &nsLN, float &ve)
{
   int meas = 0;
   int offset = 0;
   int n = pData->GetRowCount();
   FDataObj *pDataGrouped = new FDataObj(3, 0, 0.0f, U_UNDEFINED);     // MEMORY LEAK!!!!!!
   float obsMean = 0.0f, predMean = 0.0f, obsMeanL = 0.0f, predMeanL = 0.0f;
   int firstSample = int( 180 / m_timeStep );
   for (int j = firstSample; j < n; j++) //50this skips the first 50 days. Assuming those values                                       // might be unacceptable due to initial conditions...
   {
      float time = 0.0f; float obs = 0.0f; float pred = 0.0f;
      pData->Get(0, j, time);
      pData->Get(2, j, obs); //add up all the discharge values and then...
      obsMean += obs;
      if (obs != 0.0f)
          obsMeanL += log10(obs);
      pData->Get(1, j, pred);
      predMean += pred;
      if (pred != 0.0f)
          predMeanL += log10(pred);
   }
   obsMean = obsMean / ((float)n - (float)firstSample); // divide them by the sample size to generate the average.
                                                        //predMean = predMean/((float)n-(float)firstSample); // divide them by the sample size to generate the average.
   obsMeanL = obsMeanL / ((float)n - (float)firstSample);
   //predMeanL=predMeanL/((float)n-(float)firstSample);

   float obsMeanDaily = 0.0f, predMeanDaily = 0.0f;
   int previousTimeDaily = (int)pData->Get(0, firstSample - 1);
   float numHour = 0;
   for (int j = firstSample; j < n; j++) //50this skips the first 50 days. Assuming those values                                         // might be unacceptable due to initial conditions...
   {
      float time = pData->Get(0, j);

      int timeDaily = (int)time;
      if (timeDaily == previousTimeDaily)
      {
         numHour++;
         obsMeanDaily += pData->Get(2, j); //add up all the discharge values and then...
         predMeanDaily += pData->Get(1, j);
      }
      else //we moved on to the next day, so average
      {
         obsMeanDaily = obsMeanDaily / numHour;
         predMeanDaily = predMeanDaily / numHour;
         numHour = 1;
         float dailyData[3]; dailyData[0] = (float)timeDaily - 1; dailyData[1] = predMeanDaily; dailyData[2] = obsMeanDaily;
         pDataGrouped->AppendRow(dailyData, 3);
         previousTimeDaily = timeDaily;
         obsMeanDaily = pData->Get(2, j); //add up all the discharge values and then...
         predMeanDaily = pData->Get(1, j);
      }
   }

   double errorSum = 0.0f, errorSumDenom = 0.0f, errorSumLN = 0.0f, errorSumDenomLN = 0.0f, r2Denom1 = 0.0f, r2Denom2 = 0.0f; float ve_numerator = 0.0f; float ve_denominator = 0.0f;
   n = pDataGrouped->GetRowCount();
   for (int row = 0; row < n; row++)
   {
      float pred = 0.0f, obs = 0.0f; float time = 0.0f;
      pDataGrouped->Get(0, row, time);
      pDataGrouped->Get(2, row, obs);
      pDataGrouped->Get(1, row, pred);

      errorSum += ((obs - pred)*(obs - pred));
      errorSumDenom += ((obs - obsMean)*(obs - obsMean));

      ve_numerator += (obs - pred);
      ve_denominator += obs;

      double a = log10(obs) - log10(pred);
      double b = log10(obs) - obsMeanL;
      // double b = log10(obs) - log10(obsMean);
      errorSumLN += pow(a, 2);
      errorSumDenomLN += pow(b, 2);
   }

   if (errorSumDenom > 0.0f)
   {
      ns = (float)(1.0f - (errorSum / errorSumDenom));
      nsLN = (float)(1.0f - (errorSumLN / errorSumDenomLN));
      ve = (float)ve_numerator / ve_denominator;
   }

   delete pDataGrouped;

   return ns;
}


void FlowModel::GetNashSutcliffeGrouped(float ns)
{
   int meas = 0;
   float _ns = 1.0f; float _nsLN = 1.0f;
   float *data = new float[m_reachMeasuredDataArray.GetSize()];
   int offset = 0;
   FDataObj *pDataGrouped = new FDataObj(U_UNDEFINED);
   for (int l = 0; l < (int)m_reachMeasuredDataArray.GetSize(); l++)
   {
      FDataObj *pData = m_reachMeasuredDataArray.GetAt(l);

      int n = pData->GetRowCount();
      float obsMean = 0.0f, predMean = 0.0f;
      int firstSample = int( 50 / m_flowContext.timeStep );

      for (int j = firstSample; j < n; j++) //50this skips the first 50 days. Assuming those values                                         // might be unacceptable due to initial conditions...
      {
         float time = pData->Get(0, j);
         obsMean += pData->Get(2, j); //add up all the discharge values and then...
         predMean += pData->Get(1, j);
      }
      obsMean = obsMean / ((float)n - (float)firstSample); // divide them by the sample size to generate the average.
      predMean = predMean / ((float)n - (float)firstSample); // divide them by the sample size to generate the average.

      float obsMeanDaily = 0.0f, predMeanDaily = 0.0f;
      int previousTimeDaily = (int)pData->Get(0, firstSample - 1);
      for (int j = firstSample; j < n; j++) //50this skips the first 50 days. Assuming those values                                         // might be unacceptable due to initial conditions...
      {
         float time = pData->Get(0, j);
         float numHour = 0;
         int timeDaily = (int)time;
         if (timeDaily == previousTimeDaily)
         {
            numHour++;
            obsMeanDaily += pData->Get(2, j); //add up all the discharge values and then...
            predMeanDaily += pData->Get(1, j);
         }
         else //we moved on to the next day, so average
         {
            obsMeanDaily = obsMeanDaily / numHour;
            predMeanDaily = predMeanDaily / numHour;
            float *dailyData = new float[3]; dailyData[0] = (float)timeDaily; dailyData[1] = predMean; dailyData[2] = obsMean;
            pDataGrouped->AppendRow(dailyData, 3);
            delete[] dailyData;
            previousTimeDaily = timeDaily;
            obsMeanDaily = pData->Get(2, j); //add up all the discharge values and then...
            predMeanDaily = pData->Get(1, j);
         }
      }

      double errorSum = 0.0f, errorSumDenom = 0.0f, errorSumLN = 0.0f, errorSumDenomLN = 0.0f, r2Denom1 = 0.0f, r2Denom2 = 0.0f;
      n = pDataGrouped->GetRowCount();
      for (int row = 0; row < n; row++)
      {
         float pred = 0.0f, obs = 0.0f;
         float time = pDataGrouped->Get(0, row);
         obs = pDataGrouped->Get(2, row);
         pDataGrouped->Get(1, row, pred); // Assuming col i holds data for the reach corresponding to measurement

         errorSum += ((obs - pred)*(obs - pred));
         errorSumDenom += ((obs - obsMean)*(obs - obsMean));
         double a = log10(obs) - log10(pred);
         double b = log10(obs) - log10(obsMean);
         errorSumLN += pow(a, 2);
         errorSumDenomLN += (b, 2);
      }

      if (errorSumDenom > 0.0f)
      {
         _ns = (float)(1.0f - (errorSum / errorSumDenom));
         _nsLN = (float)(1.0f - (errorSumLN / errorSumDenomLN));
      }
      ns = _ns;
   }
   delete[] data;
   delete pDataGrouped;
}


void FlowModel::GetNashSutcliffe(float *ns)
   {
   int meas = 0;
   float _ns = 1.0f; float _nsLN = 1.0f;
   float *data = new float[m_reachMeasuredDataArray.GetSize()];
   int offset = 0;
   for (int l = 0; l < (int)m_reachMeasuredDataArray.GetSize(); l++)
      {
      FDataObj *pData = m_reachMeasuredDataArray.GetAt(l);
      int n = pData->GetRowCount();
      float obsMean = 0.0f, predMean = 0.0f;
      int firstSample = 200;
      for (int j = firstSample; j < n; j++) //50this skips the first 50 days. Assuming those values
         // might be unacceptable due to initial conditions...
         {
         float time = pData->Get(0, j);
         obsMean += pData->Get(2, j); //add up all the discharge values and then...
         predMean += pData->Get(1, j);
         }
      obsMean = obsMean / ((float)n - (float)firstSample); // divide them by the sample size to generate the average.
      predMean = predMean / ((float)n - (float)firstSample); // divide them by the sample size to generate the average.

      double errorSum = 0.0f, errorSumDenom = 0.0f, errorSumLN = 0.0f, errorSumDenomLN = 0.0f, r2Denom1 = 0.0f, r2Denom2 = 0.0f;
      for (int row = firstSample; row < n; row++)
         {
         float pred = 0.0f, obs = 0.0f;
         float time = pData->Get(0, row);
         obs = pData->Get(2, row);
         pData->Get(1, row, pred); // Assuming col i holds data for the reach corresponding to measurement

         errorSum += ((obs - pred)*(obs - pred));
         errorSumDenom += ((obs - obsMean)*(obs - obsMean));
         double a = log10(obs) - log10(pred);
         double b = log10(obs) - log10(obsMean);
         errorSumLN += pow(a, 2);
         errorSumDenomLN += (b, 2);
         }

      if (errorSumDenom > 0.0f)
         {
         _ns = (float)(1.0f - (errorSum / errorSumDenom));
         _nsLN = (float)(1.0f - (errorSumLN / errorSumDenomLN));
         }
      ns[offset] = _ns;
      offset++;
      ns[offset] = _nsLN;
      offset++;
      }
   delete[] data;
   }


int FlowModel::ReadClimateStationData(ClimateDataInfo *pInfo, LPCSTR filename)
   {
   ASSERT( pInfo != NULL );

   if (pInfo->m_pStationData != NULL)  // is there already a climate data obj?
      delete pInfo->m_pStationData;

   pInfo->m_pStationData = new FDataObj(U_DAYS);   // create a new one
   ASSERT(pInfo->m_pStationData != NULL);

   CString path;
   if (PathManager::FindPath(filename, path) < 0)
      {
      CString msg;
      msg.Format(_T("Flow: Unable to find climate station data path '%s'"), filename);
      Report::LogWarning(msg);
      return -1;
      }

   int records = (int)pInfo->m_pStationData->ReadAscii(path, ',', TRUE);
   m_colCsvYear         = pInfo->m_pStationData->GetCol("year");
   m_colCsvMonth        = pInfo->m_pStationData->GetCol("month");
   m_colCsvDay          = pInfo->m_pStationData->GetCol("day");
   m_colCsvHour         = pInfo->m_pStationData->GetCol("hour");
   m_colCsvMinute       = pInfo->m_pStationData->GetCol("minute");
   m_colCsvPrecip       = pInfo->m_pStationData->GetCol("precip");
   m_colCsvTMean        = pInfo->m_pStationData->GetCol("tavg");
   m_colCsvTMin         = pInfo->m_pStationData->GetCol("tmin");
   m_colCsvTMax         = pInfo->m_pStationData->GetCol("tmax");
   m_colCsvSolarRad     = pInfo->m_pStationData->GetCol("srad");
   m_colCsvWindspeed    = pInfo->m_pStationData->GetCol("windspd");
   m_colCsvRelHumidity  = pInfo->m_pStationData->GetCol("relHumidity");
   m_colCsvSpHumidity   = pInfo->m_pStationData->GetCol("sphumidity");
   m_colCsvVPD = pInfo->m_pStationData->GetCol("vpdKPA");

   for (int i = 0; i < pInfo->m_pStationData->GetRowCount(); i++)
      {
      int year  = (int)pInfo->m_pStationData->Get(m_colCsvYear, i);
      int month = (int)pInfo->m_pStationData->Get(m_colCsvMonth, i);
      int day   = (int)pInfo->m_pStationData->Get(m_colCsvDay, i);

      int dayOfYear = ::GetDayOfYear(month, day, year);  // 1-based

      float time = (year*365.0f) + dayOfYear - 1;   // days since 0000

      if (m_colCsvHour >= 0)
         {
         float hour = pInfo->m_pStationData->Get(m_colCsvHour, i) / 24.0f;

         float min = 0;
         if (m_colCsvMinute >= 0)
            min = pInfo->m_pStationData->Get(m_colCsvMinute, i) / 60.0f / 100.0f;

         time += (hour + min);
         }

      pInfo->m_pStationData->Set(0, i, time);  // col 0 reserved for time
      }

   return records;
   }


void FlowModel::ResetStateVariables()
   {
   m_timeOffset = 0;

   ReadState();//read file including initial values for the state variables. This avoids model spin up issues
   int catchmentCount = GetCatchmentCount();
   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for (int i = 0; i < catchmentCount; i++)
      {
      Catchment *pCatchment = GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();
      for (int h = 0; h < hruCount; h++)
         {
         HRU *pHRU = pCatchment->GetHRU(h);
         int hruPoolCount = pHRU->GetPoolCount();
         float waterDepth = 0.0f;
         for (int l = 0; l < hruPoolCount; l++)
            {
            HRUPool *pHRUPool = pHRU->GetPool(l);
            waterDepth += float(pHRUPool->m_volumeWater / pHRU->m_area*1000.0f);//mm of total storage
            }
         pHRU->m_initStorage = waterDepth;
         }
      }
   }

void FlowModel::GetTotalStorage(float &channel, float &terrestrial)
   {

   int catchmentCount = GetCatchmentCount();
   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for (int i = 0; i < catchmentCount; i++)
      {
      Catchment *pCatchment = GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();
      for (int h = 0; h < hruCount; h++)
         {
         HRU *pHRU = pCatchment->GetHRU(h);
         int hruPoolCount = pHRU->GetPoolCount();
         for (int l = 0; l < hruPoolCount; l++)
            {
            HRUPool *pHRUPool = pHRU->GetPool(l);
            terrestrial += float(pHRUPool->m_volumeWater);
            }
         }
      }

   int reachCount = (int)m_reachArray.GetSize();
   for (int i = 0; i < reachCount; i++)
      {
      Reach *pReach = m_reachArray[i];
      SetGeometry(pReach, pReach->GetReachSubnode(0)->m_discharge);
      channel += (pReach->m_width*pReach->m_depth*pReach->m_length);
      }
   }


bool FlowModel::InitPlugins(void)
   {
   if (m_extraSvRxnExtFnDLL)
      {
      FLOWINITFN initFn = (FLOWINITFN) ::GetProcAddress(m_extraSvRxnExtFnDLL, _T("Init"));

      if (initFn != NULL)
         initFn(&m_flowContext, NULL);
      }

   if (m_reservoirFluxExtFnDLL)
      {
      FLOWINITFN initFn = (FLOWINITFN) ::GetProcAddress(m_reservoirFluxExtFnDLL, _T("Init"));

      if (initFn != NULL)
         initFn(&m_flowContext, NULL);
      }

   return true;
   }


bool FlowModel::InitRunPlugins(void)
   {

   if (m_extraSvRxnExtFnDLL)
      {
      FLOWINITRUNFN initFn = (FLOWINITRUNFN) ::GetProcAddress(m_extraSvRxnExtFnDLL, _T("InitRun"));

      if (initFn != NULL)
         initFn(&m_flowContext);
      }

   if (m_reservoirFluxExtFnDLL)
      {
      FLOWINITRUNFN initFn = (FLOWINITRUNFN) ::GetProcAddress(m_reservoirFluxExtFnDLL, _T("InitRun"));

      if (initFn != NULL)
         initFn(&m_flowContext);
      }

   return true;
   }


bool FlowModel::EndRunPlugins(void)
   {
   if (m_extraSvRxnExtFnDLL)
      {
      FLOWENDRUNFN initFn = (FLOWENDRUNFN) ::GetProcAddress(m_extraSvRxnExtFnDLL, _T("EndRun"));

      if (initFn != NULL)
         initFn(&m_flowContext);
      }

   if (m_reservoirFluxExtFnDLL)
      {
      FLOWENDRUNFN initFn = (FLOWENDRUNFN) ::GetProcAddress(m_reservoirFluxExtFnDLL, _T("EndRun"));

      if (initFn != NULL)
         initFn(&m_flowContext);
      }

   return true;
   }


int FlowModel::GetUpstreamCatchmentCount(ReachNode *pStartNode)
   {
   if (pStartNode == NULL)
      return 0;

   int count = 0;
   if (pStartNode->m_pLeft)
      {
      count += GetUpstreamCatchmentCount(pStartNode->m_pLeft);

      Reach *pReach = GetReachFromNode(pStartNode->m_pLeft);

      if (pReach != NULL)
         count += (int)pReach->m_catchmentArray.GetSize();
      }

   if (pStartNode->m_pRight)
      {
      count += GetUpstreamCatchmentCount(pStartNode->m_pRight);

      Reach *pReach = GetReachFromNode(pStartNode->m_pRight);

      if (pReach != NULL)
         count += (int)pReach->m_catchmentArray.GetSize();
      }

   return count;
   }


/////////////////////////////////////////////////////////////////////////////////
// Climate methods
/////////////////////////////////////////////////////////////////////////////////

// called at the beginning of each invocation of FlowModel::Run() (beginning of each year)
bool FlowModel::LoadClimateData(EnvContext *pEnvContext, int year)
   {
   FlowScenario *pScenario = m_scenarioArray.GetAt(m_currentFlowScenarioIndex);
   for (int i = 0; i < (int)pScenario->m_climateInfoArray.GetSize(); i++)
      {
      ClimateDataInfo *pInfo = pScenario->m_climateInfoArray[i];

      if (pInfo->m_pNetCDFData == NULL)
         continue;

      if (pInfo->m_useDelta)
         {
         //if ( GetCurrentYearOfRun() == 0 )  // first year?
          //  {
         CString path;
         path.Format(_T("%s_%i.nc"), (LPCTSTR)pInfo->m_path, pEnvContext->startYear);
         CString msg;
         msg.Format(_T("Loading Climate %s"), (LPCTSTR)pInfo->m_path);
         Report::Log(msg);

         pInfo->m_pNetCDFData->Open(path, pInfo->m_varName);
         pInfo->m_pNetCDFData->ReadSpatialData();
         //   }
         }


      else if (m_estimateParameters && pEnvContext->run == 0)//estimating parameters and on the first run.  Load all climate data for this year
         {
         CString path;
         path.Format(_T("%s_%i.nc"), (LPCTSTR)pInfo->m_path, year);
         CString msg;
         msg.Format(_T("Loading Climate %s"), (LPCTSTR)pInfo->m_path);
         Report::Log(msg);
         GeoSpatialDataObj *pObj = pInfo->m_pNetCDFDataArray.GetAt(GetCurrentYearOfRun());
         pObj->Open(path, pInfo->m_varName);
         pObj->ReadSpatialData();
         }

      else if (!m_estimateParameters)  // not using delta method and not estimating parameters, so open/read this years data
         {
         CString path;
         path.Format(_T("%s_%i.nc"), (LPCTSTR)pInfo->m_path, year);

         CString msg;
         msg.Format(_T("Loading Climate %s"), (LPCTSTR)pInfo->m_path);
         Report::Log(msg);

         pInfo->m_pNetCDFData->Open(path, pInfo->m_varName);
         pInfo->m_pNetCDFData->ReadSpatialData();
         }
      }

   return true;
   }


// called at the end of FlowModel::Run()
void FlowModel::CloseClimateData(void)
   {
   FlowScenario *pScenario = m_scenarioArray[m_currentFlowScenarioIndex];

   for (int i = 0; i < (int)pScenario->m_climateInfoArray.GetSize(); i++)
      {
      ClimateDataInfo *pInfo = pScenario->m_climateInfoArray[i];

      if (pInfo->m_pNetCDFData != NULL)
         pInfo->m_pNetCDFData->Close();
      }
   }


bool FlowModel::GetHRUClimate(CDTYPE type, HRU *pHRU, int dayOfYear, float &value)
   {
   FlowScenario *pScenario = m_scenarioArray.GetAt(m_currentFlowScenarioIndex);
   if (!pScenario->m_useStationData) //it is from a NETCDF format
      {
      ClimateDataInfo *pInfo = GetClimateDataInfo((int) type);
      if (!m_estimateParameters)
         {
         if (pInfo != NULL && pInfo->m_pNetCDFData != NULL)   // find a data object?
            {
             if (pHRU->m_climateIndex < 0)   // first time through
                 {
                 double x = pHRU->m_centroid.x;
                 double y = pHRU->m_centroid.y;
                 value = pInfo->m_pNetCDFData->Get(x, y, pHRU->m_climateIndex, dayOfYear, m_projectionWKT, false);
                 
                 //if (value > -100 && m_provenClimateIndex == -1)
                 //    m_provenClimateIndex = pHRU->m_climateIndex;

                 if (value == -9999) //missing data in the file. Code identifies a nearby gridcell that does have data
                     {
                     pHRU->m_climateIndex = m_provenClimateIndex;
                     value = pInfo->m_pNetCDFData->Get(pHRU->m_climateIndex, dayOfYear);
                     pHRU->m_pCatchment->m_climateIndex = pHRU->m_climateIndex;
                     }
                    
                 }
            else
               value = pInfo->m_pNetCDFData->Get(pHRU->m_climateIndex, dayOfYear);


            if (pInfo->m_useDelta)
               value += (pInfo->m_delta * this->GetCurrentYearOfRun());

            return true;
            }
         }

      else //m_estimateParameters is true
         {
         GeoSpatialDataObj *pObj = pInfo->m_pNetCDFDataArray.GetAt(GetCurrentYearOfRun());//current year
         if (pInfo != NULL && pObj != NULL)   // find a data object?
            {
            if (pHRU->m_climateIndex < 0)
               {
               double x = pHRU->m_centroid.x;
               double y = pHRU->m_centroid.y;
               value = pObj->Get(x, y, pHRU->m_climateIndex, dayOfYear, m_projectionWKT, false);   // units?????!!!!
               }
            else
               {
               value = pObj->Get(pHRU->m_climateIndex, dayOfYear);
               }
            }
         return true;
         }
      }
   else // if (m_pStationData != NULL)
      {
      // variable       column
      // CDT_PRECIP:       6
      // CDT_TMEAN:        7
      // CDT_TMIN:         9
      // CDT_TMAX:         8
      // CDT_SOLARRAD:    10
      // CDT_WINDSPEED:   11
      // CDT_RELHUMIDITY: 12
      // CDT_SPHUMIDITY:  13
      // CDT_VPD:         14
      ASSERT( pHRU->m_polyIndexArray.GetSize() > 0 );
      if ( pScenario->m_climateInfoArray.GetSize() > 1 && m_colStationID < 0 )
         return false;

      int idu = pHRU->m_polyIndexArray[0];
      int stationID = 0;
      if ( m_colStationID >= 0 )
         this->m_pCatchmentLayer->GetData( idu, m_colStationID, stationID );

      ClimateDataInfo *pInfo = GetClimateDataInfo(stationID );

      if ( pInfo == NULL || pInfo->m_pStationData == NULL )
         return false;

      value = 0;
      switch (type)
         {
         case CDT_PRECIP:   // 1
            if (m_colCsvPrecip > 0)
               //value = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvPrecip, IM_LINEAR, m_timeOffset, true);
               value = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvPrecip, IM_LINEAR, &m_timeOffset);
            break;

         case CDT_TMEAN:    // 2
            {
            float temp = 0;
            int timeOffset = m_timeOffset;
            if (m_colCsvTMean >= 0) // tmean col defined?
               temp = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvTMean, IM_LINEAR, &timeOffset);
            //temp = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvTMean, IM_LINEAR, m_timeOffset, false);
            else if (m_colCsvTMin >= 0 && m_colCsvTMax >= 0)
               {
               timeOffset = m_timeOffset;
               float tMin = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvTMin, IM_LINEAR, &timeOffset);
               //float tMin = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvTMin, IM_LINEAR, m_timeOffset, false);
               timeOffset = m_timeOffset;
               float tMax = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvTMax, IM_LINEAR, &timeOffset);
               //float tMax = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvTMax, IM_LINEAR, m_timeOffset, false);
               temp = (tMin + tMax) / 2;
               }

            // adjust for elevation
            if (pHRU->m_elevation == 0)
               pHRU->m_elevation = pInfo->m_stationElevation;

            float elevKM = pHRU->m_elevation / 1000.f;
            value = temp + (-4.0f*(elevKM - pInfo->m_stationElevation / 1000.0f));
            }
            break;

         case CDT_TMIN: // 4
            if (m_colCsvTMin >= 0)
               {
               int timeOffset = m_timeOffset;
               value = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvTMin, IM_LINEAR, &timeOffset);
               // value = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvTMin, IM_LINEAR, m_timeOffset, false);

               }
            break;

         case CDT_TMAX: // 8
            if (m_colCsvTMax >= 0)
               {
               int timeOffset = m_timeOffset;
               value = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvTMax, IM_LINEAR, &timeOffset);
               //value = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvTMax, IM_LINEAR, m_timeOffset, false);
               }
            break;

         case CDT_SOLARRAD:   // 16
            if (m_colCsvSolarRad >= 0)
               {
               int timeOffset = m_timeOffset;
               value = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvSolarRad, IM_LINEAR, &timeOffset);
               //value = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvSolarRad, IM_LINEAR, m_timeOffset, false);
               }
            break;

         case CDT_RELHUMIDITY:   // 32
            if (m_colCsvRelHumidity >= 0)
               {
               int timeOffset = m_timeOffset;
               value = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvRelHumidity, IM_LINEAR, &timeOffset);
               //value = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvRelHumidity, IM_LINEAR, m_timeOffset, false);
               }
            break;

         case CDT_SPHUMIDITY:    // 64
            if (m_colCsvSpHumidity >= 0)
               {
               int timeOffset = m_timeOffset;
               value = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvSpHumidity, IM_LINEAR, &timeOffset);
               //value = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvSpHumidity, IM_LINEAR, m_timeOffset, false);
               }
            break;

         case CDT_WINDSPEED:  // 128
            if (m_colCsvWindspeed >= 0)
               {
               int timeOffset = m_timeOffset;
               value = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvWindspeed, IM_LINEAR, &timeOffset);
               //value = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvWindspeed, IM_LINEAR, m_timeOffset, false);
               }
            break;

         case CDT_VPD:     // 256
            if (m_colCsvVPD >= 0)
               {
               int timeOffset = m_timeOffset;
               value = pInfo->m_pStationData->IGet((float)m_currentTime, 0, m_colCsvVPD, IM_LINEAR, &timeOffset);
               //value = m_pClimateStationData->IGetInternal((float)m_currentTime, m_colCsvVPD, IM_LINEAR, m_timeOffset, false);
               }
            break;

         default:
            {
            CString msg;
            msg.Format(_T("Flow: Runtime Error - Invalid climate type selector (%i) passed while getting HRU climate data"), type);
            Report::LogWarning(msg);
            return false;
            }
         }

      return true;
      }

   return false;
   }


ClimateDataInfo *FlowModel::GetClimateDataInfo(int typeOrStationID ) 
   {
   FlowScenario *pScenario = m_scenarioArray.GetAt(m_currentFlowScenarioIndex);
   if ( pScenario == NULL )
      return NULL;
   
   ClimateDataInfo *pInfo = NULL;
 
   if ( pScenario->m_climateDataMap.Lookup(typeOrStationID, pInfo) ) 
      return pInfo; 
   
   return NULL;
   }


bool FlowModel::GetReachClimate(CDTYPE type, Reach *pReach, int dayOfYear, float &value)
   {
   ClimateDataInfo *pInfo = GetClimateDataInfo(type);

   if (pInfo != NULL && pInfo->m_pNetCDFData != NULL)   // find a data object?
      {
      if (pReach->m_climateIndex < 0)
         {
         Poly *pPoly = m_pStreamLayer->GetPolygon(pReach->m_polyIndex);

         if (pPoly->GetVertexCount() > 0)
            {
            double x = pPoly->GetVertex(0).x;  //pPoly->GetCentroid().x;
            double y = pPoly->GetVertex(0).y;   //pPoly->GetCentroid().y;
            value = pInfo->m_pNetCDFData->Get(x, y, pReach->m_climateIndex, dayOfYear, m_projectionWKT, false);   // units?????!!!!
            }
         else
            {
            CString msg;
            msg.Format(_T("Flow: Bad stream poly (id=%i) - no vertices!!!"), pPoly->m_id);
            Report::ErrorMsg(msg);
            }
         }
      else
         {
         value = pInfo->m_pNetCDFData->Get(pReach->m_climateIndex, dayOfYear);
         }

      if (pInfo->m_useDelta)
         value += (pInfo->m_delta * this->GetCurrentYearOfRun());

      return true;
      }

   return false;
   }

float FlowModel::GetTotalReachLength(void)
   {
   float length = 0;

   for (int i = 0; i < this->GetReachCount(); i++)
      {
      Reach *pReach = m_reachArray[i];

      length += pReach->m_length;
      }

   return length;
   }



// called daily (or whatever timestep flow is running at
bool FlowModel::CollectModelOutput(void)
   {
   clock_t start = clock();

   // any outputs to collect?
   int moCount = 0;
   int moCountIdu = 0;
   int moCountHru = 0;
   int moCountHruLayer = 0;
   int moCountReach = 0;
   int moCountHruIdu = 0;
   int moCountReservoir = 0;
   int moCountGrid = 0;

   for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

      for (int i = 0; i < (int)pGroup->GetSize(); i++)
         {
         ModelOutput *pOutput = pGroup->GetAt(i);

         // initialize ModelOutput
         pOutput->m_value = 0;
         pOutput->m_totalQueryArea = 0;

         if (pOutput->m_inUse)
            {
            moCount++;

            switch (pOutput->m_modelDomain)
               {
               case MOD_IDU:        moCountIdu++;          break;
               case MOD_HRU:        moCountHru++;          break;
               case MOD_HRULAYER:   moCountHruLayer++;     break;
               case MOD_REACH:      moCountReach++;        break;
               case MOD_HRU_IDU:    moCountHruIdu++;       break;
               case MOD_RESERVOIR:  moCountReservoir++;    break;
               case MOD_GRID:       moCountGrid++;         break;
               }
            }
         }
      }

   if (moCount == 0)
      return true;

   float totalIDUArea = 0;

   // iterate through IDU's to collect output if any of the expressions are IDU-domain
   if (moCountIdu > 0)
      {
      int colArea = m_pCatchmentLayer->GetFieldCol(_T("AREA"));
      for (MapLayer::Iterator idu = this->m_pCatchmentLayer->Begin(); idu < m_pCatchmentLayer->End(); idu++)
         {
         if (m_pModelOutputQE)
            m_pModelOutputQE->SetCurrentRecord(idu);

         m_pMapExprEngine->SetCurrentRecord(idu);

         float area = 0;
         m_pCatchmentLayer->GetData(idu, colArea, area);
         totalIDUArea += area;

         for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
            {
            ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

            for (int i = 0; i < (int)pGroup->GetSize(); i++)
               {
               ModelOutput *pOutput = pGroup->GetAt(i);

               if (pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_IDU)
                  continue;

               bool passConstraints = true;

               if (pOutput->m_pQuery)
                  {
                  bool result = false;
                  bool ok = pOutput->m_pQuery->Run(idu, result);

                  if (result == false)
                     passConstraints = false;
                  }

               // did we pass the constraints?  If so, evaluate preferences
               if (passConstraints)
                  {
                  pOutput->m_totalQueryArea += area;
                  float value = 0;

                  if (pOutput->m_pMapExpr != NULL)
                     {
                     bool ok = m_pMapExprEngine->EvaluateExpr(pOutput->m_pMapExpr, pOutput->m_pQuery ? true : false);
                     if (ok)
                        value = (float)pOutput->m_pMapExpr->GetValue();
                     }

                  // we have a value for this IDU, accumulate it based on the model type
                  switch (pOutput->m_modelType)
                     {
                     case MOT_SUM:
                        pOutput->m_value += value;
                        break;

                     case MOT_AREAWTMEAN:
                        pOutput->m_value += value * area;
                        break;

                     case MOT_PCTAREA:
                        pOutput->m_value += area;
                        break;
                        //case MO_PCT_CONTRIB_AREA:
                        //   return true;
                     }
                  }  // end of: if( passed constraints)
               }  // end of: for ( each model )
            }  // end of: for ( each group )
         }  // end of: for ( each IDU )
      }  // end of: if ( moCountIdu > 0 )

   // GRID-domain
   if (moCountGrid > 0)
      {
      for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
         {
         ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

         for (int i = 0; i < (int)pGroup->GetSize(); i++)
            {
            ModelOutput *pOutput = pGroup->GetAt(i);

            if (pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_GRID)
               continue;
            int row = 0; int col = 0;
            m_pGrid->GetGridCellFromCoord(pOutput->m_coord.x, pOutput->m_coord.y, row, col);

            int offSet = m_pHruGrid->Get(row, col);//this gives us the offset in the m_hruArray for this particular grid location (i and j)
            if (offSet > 0)
               {
               HRU *pHRU = GetHRU(offSet);
               HRU::m_mvElws = pHRU->m_elws;
               pOutput->m_value = pHRU->m_elws;
               pOutput->m_totalQueryArea = 1.0f;
               }
            }  // end of: for ( each group )
         }
      }  // end of: if ( moCountIdu > 0 )


   // Next, do HRU-level variables
   totalIDUArea = 0;
   if (moCountHru > 0)
      {
      for (int h = 0; h < (int) this->m_hruArray.GetSize(); h++)
         {
         HRU *pHRU = m_hruArray[h];

         int idu = pHRU->m_polyIndexArray[0];       // use the first IDU to evaluate

         if (m_pModelOutputQE)
            m_pModelOutputQE->SetCurrentRecord(idu);

         m_pMapExprEngine->SetCurrentRecord(idu);

         float area = pHRU->m_area;
         totalIDUArea += area;

         // update static HRU variables
         HRU::m_mvDepthMelt = pHRU->m_depthMelt;  // volume of water in snow (converted from m to mm)
         HRU::m_mvDepthSWE = pHRU->m_depthSWE;   // volume of ice in snow (converted from m to mm)
         HRU::m_mvCurrentPrecip = pHRU->m_currentPrecip;
         HRU::m_mvCurrentRainfall = pHRU->m_currentRainfall;
         HRU::m_mvCurrentSnowFall = pHRU->m_currentSnowFall;
         HRU::m_mvCurrentAirTemp = pHRU->m_currentAirTemp;
         HRU::m_mvCurrentMinTemp = pHRU->m_currentMinTemp;
         HRU::m_mvCurrentMaxTemp = pHRU->m_currentMaxTemp;
         HRU::m_mvCurrentRadiation = pHRU->m_currentRadiation;
         HRU::m_mvCurrentRecharge = pHRU->m_currentGWRecharge;
         HRU::m_mvCurrentGwFlowOut = pHRU->m_currentGWFlowOut;

         HRU::m_mvCurrentSWC = pHRU->m_swc;
         HRU::m_mvCurrentIrr = pHRU->m_irr;

         HRU::m_mvCurrentET = pHRU->m_currentET;
         HRU::m_mvCurrentMaxET = pHRU->m_currentMaxET;
         HRU::m_mvCurrentCGDD = pHRU->m_currentCGDD;
         HRU::m_mvCurrentSediment = pHRU->m_currentSediment;
         HRU::m_mvCumP = pHRU->m_precip_yr;
         HRU::m_mvCumRunoff = pHRU->m_runoff_yr;
         HRU::m_mvCumET = pHRU->m_et_yr;
         HRU::m_mvCumMaxET = pHRU->m_maxET_yr;
         HRU::m_mvCumRecharge = pHRU->m_gwRecharge_yr;
         HRU::m_mvCumGwFlowOut = pHRU->m_gwFlowOut_yr;

         HRU::m_mvElws = pHRU->m_elws;
         HRU::m_mvDepthToWT = pHRU->m_elevation - pHRU->m_elws;

         for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
            {
            ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

            for (int i = 0; i < (int)pGroup->GetSize(); i++)
               {
               ModelOutput *pOutput = pGroup->GetAt(i);

               if (pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_HRU)
                  continue;

               bool passConstraints = true;

               if (pOutput->m_pQuery)
                  {
                  bool result = false;
                  bool ok = pOutput->m_pQuery->Run(idu, result);

                  if (result == false)
                     passConstraints = false;
                  }

               // did we pass the constraints?  If so, evaluate preferences
               if (passConstraints)
                  {
                  pOutput->m_totalQueryArea += area;
                  float value = 0;

                  if (pOutput->m_pMapExpr != NULL)
                     {
                     bool ok = m_pMapExprEngine->EvaluateExpr(pOutput->m_pMapExpr, pOutput->m_pQuery ? true : false);
                     if (ok)
                        value = (float)pOutput->m_pMapExpr->GetValue();
                     }

                  // we have a value for this IDU, accumulate it based on the model type
                  switch (pOutput->m_modelType)
                     {
                     case MOT_SUM:
                        pOutput->m_value += value;
                        break;

                     case MOT_AREAWTMEAN:
                        pOutput->m_value += value * area;
                        break;

                     case MOT_PCTAREA:
                        pOutput->m_value += area;
                        break;
                        //case MO_PCT_CONTRIB_AREA:
                        //   return true;
                     }
                  }  // end of: if( passed constraints)
               }  // end of: for ( each model )
            }  // end of: for ( each output group )
         }  // end of: for ( each HRU )
      }  // end of: if ( moCountHru > 0 )


   // Next, do HRUPool-level variables
   totalIDUArea = 1;
   if (moCountHruLayer > 0)
      {
      for (int h = 0; h < (int) this->m_hruArray.GetSize(); h++)
         {
         HRU *pHRU = m_hruArray[h];

         for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
            {
            float area = 0.0f;
            int idu = pHRU->m_polyIndexArray[k];

            if (m_pModelOutputQE)
               m_pModelOutputQE->SetCurrentRecord(idu);

            m_pMapExprEngine->SetCurrentRecord(idu);

            for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
               {
               ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

               for (int i = 0; i < (int)pGroup->GetSize(); i++)
                  {
                  ModelOutput *pOutput = pGroup->GetAt(i);

                  if (pOutput->m_inUse == true && pOutput->m_modelDomain == MOD_HRULAYER)
                     {
                     HRUPool *pHRUPool = pHRU->GetPool(pOutput->m_number);
                     // update static HRUPool variables
                     HRUPool::m_mvWC = pHRUPool->m_wc;  // volume of water 
                     HRUPool::m_mvWDepth = pHRUPool->m_wDepth;
                     if (pHRUPool->m_svArray != NULL)
                        {
                        if (pHRUPool->m_volumeWater > 1.0f && pOutput->m_esvNumber < m_hruSvCount - 1)
                           HRUPool::m_mvESV = pHRUPool->m_svArray[pOutput->m_esvNumber] / pHRUPool->m_volumeWater;  // volume of water 
                        }

                     bool passConstraints = true;

                     if (pOutput->m_pQuery)
                        {
                        bool result = false;
                        bool ok = pOutput->m_pQuery->Run(idu, result);

                        if (result == false)
                           passConstraints = false;
                        }

                     // did we pass the constraints?  If so, evaluate preferences
                     area = 0.0f;

                     if (passConstraints)
                        {
                        m_pCatchmentLayer->GetData(idu, m_colCatchmentArea, area);
                        pOutput->m_totalQueryArea += area;
                        totalIDUArea += area;
                        float value = 0;

                        if (pOutput->m_pMapExpr != NULL)
                           {
                           bool ok = m_pMapExprEngine->EvaluateExpr(pOutput->m_pMapExpr, pOutput->m_pQuery ? true : false);
                           if (ok)
                              value = (float)pOutput->m_pMapExpr->GetValue();
                           }

                        // we have a value for this IDU, accumulate it based on the model type
                        switch (pOutput->m_modelType)
                           {
                           case MOT_SUM:
                              pOutput->m_value += value;
                              break;

                           case MOT_AREAWTMEAN:
                              pOutput->m_value += value * area;
                              break;

                           case MOT_PCTAREA:
                              pOutput->m_value += area;
                              break;

                           }
                        } // end of: if( passed constraints)
                     } // end of: for ( each idu  )
                  } // end of: for ( each HRU ) 
               } // end if ( use model and HRUPool )
            } // end of: for ( each model )
         } // end of: for ( each output group )
      } // end of: if ( moCountHru > 0 )


      // Next, do Reach-level variables
   totalIDUArea = 1;
   if (moCountReach > 0)
      {
      for (int h = 0; h < (int) this->m_reachArray.GetSize(); h++)
         {
         Reach *pReach = m_reachArray[h];
         ReachSubnode *pNode = pReach->GetReachSubnode(0);

         if (m_pModelOutputStreamQE)
            m_pModelOutputStreamQE->SetCurrentRecord(pReach->m_reachIndex);

         m_pMapExprEngine->SetCurrentRecord(pReach->m_reachIndex);

         // update static HRU variables

         Reach::m_mvCurrentStreamFlow = pReach->GetDischarge();
         Reach::m_mvCurrentStreamTemp = pReach->m_currentStreamTemp;

         for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
            {
            ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

            for (int i = 0; i < (int)pGroup->GetSize(); i++)
               {
               ModelOutput *pOutput = pGroup->GetAt(i);

               if (pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_REACH)
                  continue;
               if (pNode->m_svArray != NULL)
                  Reach::m_mvCurrentTracer = pNode->m_svArray[pOutput->m_esvNumber] ;  // volume of water        
               bool passConstraints = true;

               if (pOutput->m_pQuery)
                  {
                  bool result = false;
                  bool ok = pOutput->m_pQuery->Run(pReach->m_reachIndex, result);

                  if (result == false)
                     passConstraints = false;
                  }

               // did we pass the constraints (we found the correct reach, or reaches)?  If so, evaluate preferences 
               if (passConstraints)
                  {
                  float value = 0;

                  if (pOutput->m_pMapExpr != NULL)
                     {
                     bool ok = m_pMapExprEngine->EvaluateExpr(pOutput->m_pMapExpr, false ? true : false);
                     if (ok)
                        value = (float)pOutput->m_pMapExpr->GetValue();
                     }

                  // we have a value for this Reach, accumulate it based on the model type
                  switch (pOutput->m_modelType)
                     {
                     case MOT_SUM:
                        pOutput->m_value += value;
                        break;

                        //case MO_PCT_CONTRIB_AREA:
                        //   return true;
                     }
                  }  // end of: if( passed constraints)
               }  // end of: for ( each model )
            }  // end of: for ( each output group )
         }  // end of: for ( each reach )
      }  // end of: if ( moCountReach > 0 )

   totalIDUArea = 1;

   // next look for Reservoirs
   if (moCountReservoir > 0)
      {
      for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
         {
         ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

         for (int i = 0; i < (int)pGroup->GetSize(); i++)
            {
            ModelOutput *pOutput = pGroup->GetAt(i);

            if (pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_RESERVOIR)
               continue;
            else
               {
               for (int h = 0; h < (int) this->m_reservoirArray.GetSize(); h++)
                  {
                  Reservoir *pRes = m_reservoirArray[h];
                  if (pRes->m_id == pOutput->m_siteNumber)
                     pOutput->m_value = pRes->m_elevation;
                  }
               }  // end of: for ( each model )
            }  // end of: for ( each output group )
         }  // end of: for ( each reach )
      }  // end of: if ( moCountReach > 0 )


  // Last, look for IDU specific values that are only stored in the HRUs.
  // This should be used if we have IDU specific queries AND model results that are not being written from HRUs to IDUs.
  // This will occur anytime we do not write to the screen
   totalIDUArea = 0;
   if (moCountHruIdu > 0)
      {
      for (int h = 0; h < (int) this->m_hruArray.GetSize(); h++)
         {
         HRU *pHRU = m_hruArray[h];

         for (int z = 0; z < pHRU->m_polyIndexArray.GetSize(); z++)
            {
            int idu = pHRU->m_polyIndexArray[z];       // Here evaluate all the polygons - 

            if (m_pModelOutputQE)
               m_pModelOutputQE->SetCurrentRecord(idu);

            m_pMapExprEngine->SetCurrentRecord(idu);

            float area = pHRU->m_area;
            totalIDUArea += area;

            // update static HRU variables
            HRU::m_mvDepthMelt = pHRU->m_depthMelt*1000.0f;  // volume of water in snow (converted from m to mm)
            HRU::m_mvDepthSWE = pHRU->m_depthSWE*1000.0f;   // volume of ice in snow (converted from m to mm)
            HRU::m_mvCurrentPrecip = pHRU->m_currentPrecip;
            HRU::m_mvCurrentRainfall = pHRU->m_currentRainfall;
            HRU::m_mvCurrentSnowFall = pHRU->m_currentSnowFall;
            HRU::m_mvCurrentAirTemp = pHRU->m_currentAirTemp;
            HRU::m_mvCurrentMinTemp = pHRU->m_currentMinTemp;
            HRU::m_mvCurrentMaxTemp = pHRU->m_currentMaxTemp;
            HRU::m_mvCurrentRadiation = pHRU->m_currentRadiation;
            HRU::m_mvCurrentET = pHRU->m_currentET;
            HRU::m_mvCurrentMaxET = pHRU->m_currentMaxET;
            HRU::m_mvCurrentCGDD = pHRU->m_currentCGDD;
            HRU::m_mvCurrentRecharge = pHRU->m_currentGWRecharge;
            HRU::m_mvCurrentGwFlowOut = pHRU->m_currentGWFlowOut;
            HRU::m_mvCumRunoff = pHRU->m_runoff_yr;
            HRU::m_mvCumP = pHRU->m_precip_yr;
            HRU::m_mvCumET = pHRU->m_et_yr;
            HRU::m_mvCumMaxET = pHRU->m_maxET_yr;
            HRU::m_mvCumRecharge = pHRU->m_gwRecharge_yr;
            HRU::m_mvCumGwFlowOut = pHRU->m_gwFlowOut_yr;

            for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
               {
               ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

               for (int i = 0; i < (int)pGroup->GetSize(); i++)
                  {
                  ModelOutput *pOutput = pGroup->GetAt(i);

                  if (pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_HRU_IDU)
                     continue;

                  bool passConstraints = true;

                  if (pOutput->m_pQuery)
                     {
                     bool result = false;
                     bool ok = pOutput->m_pQuery->Run(idu, result);

                     if (result == false)
                        passConstraints = false;
                     }

                  // did we pass the constraints?  If so, evaluate preferences
                  if (passConstraints)
                     {
                     pOutput->m_totalQueryArea += area;
                     float value = 0;

                     if (pOutput->m_pMapExpr != NULL)
                        {
                        bool ok = m_pMapExprEngine->EvaluateExpr(pOutput->m_pMapExpr, pOutput->m_pQuery ? true : false);
                        if (ok)
                           value = (float)pOutput->m_pMapExpr->GetValue();
                        }

                     // we have a value for this IDU, accumulate it based on the model type
                     switch (pOutput->m_modelType)
                        {
                        case MOT_SUM:
                           pOutput->m_value += value;
                           break;

                        case MOT_AREAWTMEAN:
                           pOutput->m_value += value * area;
                           break;

                        case MOT_PCTAREA:
                           pOutput->m_value += area;
                           break;
                           //case MO_PCT_CONTRIB_AREA:
                           //   return true;
                        }
                     }
                  }  // end of: if( passed constraints)
               }  // end of: for ( each model )
            }  // end of: for ( each output group )
         }  // end of: for ( each HRU )
      }  // end of: if ( moCountHru > 0 )



   // do any fixup needed and add to data object for each group
   for (int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++)
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[j];

      // any outputs?
      int moCount = 0;
      for (int i = 0; i < (int)pGroup->GetSize(); i++)
         {
         ModelOutput *pOutput = pGroup->GetAt(i);
         if (pOutput->m_inUse)
            moCount++;
         if (pOutput->m_pDataObjObs != NULL)//add 1 column if we have observations for this point.
            moCount++;
         }

      if (moCount == 0)
         continue;

      float *outputArray = new float[moCount + 1];
      outputArray[0] = (float)m_timeInRun;

      int index = 1;

      for (int i = 0; i < (int)pGroup->GetSize(); i++)
         {
         ModelOutput *pOutput = pGroup->GetAt(i);

         if (pOutput->m_inUse)
            {
            switch (pOutput->m_modelType)
               {
               case MOT_SUM:
                  break;      // no fixup required

               case MOT_AREAWTMEAN:
                  pOutput->m_value /= pOutput->m_totalQueryArea;      //totalArea;
                  break;

               case MOT_PCTAREA:
                  pOutput->m_value /= totalIDUArea;


               }

            outputArray[index++] = pOutput->m_value;
            }
         if (pOutput->m_pDataObjObs != NULL)
            outputArray[index] = pOutput->m_pDataObjObs->IGet((float)m_currentTime-2.0f);
         }

      pGroup->m_pDataObj->AppendRow(outputArray, moCount + 1);
      delete[] outputArray;
      }

   clock_t finish = clock();

   float duration = (float)(finish - start) / CLOCKS_PER_SEC;
   this->m_outputDataRunTime += duration;

   return true;
   }

int FlowModel::AddModelVar(LPCTSTR label, LPCTSTR varName, MTDOUBLE *value)
   {
   ASSERT(m_pMapExprEngine != NULL);

   ModelVar *pModelVar = new ModelVar(label);

   m_pMapExprEngine->AddVariable(varName, value);

   return (int)m_modelVarArray.Add(pModelVar);
   }


void FlowModel::ApplyMacros(EnvModel *pEnvModel, CString &str)
   {
   if (str.Find(_T('{')) < 0)
      return;

   // get the current scenario
   int scnIndex = this->m_currentEnvisionScenarioIndex;

   if (scnIndex < 0)
      return;

   // iterate through the macros, applying substitutions as needed
   for (int i = 0; i < (int) this->m_macroArray.GetSize(); i++)
      {
      ScenarioMacro *pMacro = this->m_macroArray[i];

      // for this macro, see if a value is defined for the current scenario.
      // if so, use it; if not, use the default value
      CString value(pMacro->m_defaultValue);
      for (int j = 0; j < (int)pMacro->m_macroArray.GetSize(); j++)
         {
         SCENARIO_MACRO *pScn = pMacro->m_macroArray[j];

         int index = -1;
         if (::EnvGetScenarioFromName(pEnvModel, pScn->envScenarioName, &index) != NULL)
            {
            if (index == scnIndex)
               {
               value = pScn->value;
               break;
               }
            }
         }

      CString strToReplace = _T("{") + pMacro->m_name + _T("}");
      str.Replace(strToReplace, value);
      }
   }


/////////////////////////////////////////////////////////////////// 
//  additional functions

void SetGeometry(Reach *pReach, float discharge)
   {
   ASSERT(pReach != NULL);

   pReach->m_depth = GetDepthFromQ(pReach, discharge, pReach->m_wdRatio);
   pReach->m_width = pReach->m_wdRatio * pReach->m_depth;

   }

float GetDepthFromQ(Reach *pReach, float Q, float wdRatio)  // ASSUMES A SPECIFIC CHANNEL GEOMETRY
   {
   // from kellie:  d = ( 5/9 Q n s^2 ) ^ 3/8   -- assumes width = 3*depth, rectangular channel
   float wdterm = (float)pow((wdRatio / (2 + wdRatio)), 2.0f / 3.0f)*wdRatio;
   float depth = (float)pow(((Q*pReach->m_n) / ((float)sqrt(pReach->m_slope)*wdterm)), 3.0f / 8.0f);
   return depth;
   }


// figure out what kind of flux it is
// source string syntax= fn:<dllpath:function> for functional, db:<datasourcepath:columnname> for datasets
FLUXSOURCE ParseSource(LPCTSTR _sourceStr, CString &path, CString &source, HINSTANCE &hDLL, FLUXFN &fn)
   {
   TCHAR sourceStr[256];
   lstrcpy(sourceStr, _sourceStr);

   LPTSTR equal = _tcschr(sourceStr, _T('='));
   if (equal == NULL)
      {
      CString msg;
      msg.Format(_T("Flow: Error parsing source string '%s'.  The source string must start with either 'fn=' or 'db=' for functional or database sources, respectively"),
         sourceStr);
      Report::ErrorMsg(msg);
      return  FS_UNDEFINED;
      }

   if (*sourceStr == _T('f'))
      {
      //pFluxInfo->m_sourceLocation = FS_FUNCTION;
      LPTSTR _path = equal + 1;
      LPTSTR colon = _tcschr(_path, _T(':'));

      if (colon == NULL)
         {
         CString msg;
         msg.Format(_T("Flow: Error parsing source string '%s'.  The source string must be of the form 'fn=dllpath:entrypoint' "),
            sourceStr);
         Report::ErrorMsg(msg);
         return  FS_UNDEFINED;
         }

      *colon = NULL;
      path = _path;
      path.Trim();
      source = colon + 1;
      source.Trim();

      hDLL = AfxLoadLibrary(path);
      if (hDLL == NULL)
         {
         CString msg(_T("Flow error: Unable to load "));
         msg += path;
         msg += _T(" or a dependent DLL");
         Report::ErrorMsg(msg);
         return FS_UNDEFINED;
         }

      fn = (FLUXFN)GetProcAddress(hDLL, source);
      if (fn == NULL)
         {
         CString msg(_T("Flow: Unable to find Flow function '"));
         msg += source;
         msg += _T("' in ");
         msg += path;
         Report::ErrorMsg(msg);
         AfxFreeLibrary(hDLL);
         hDLL = NULL;
         return FS_UNDEFINED;
         }
      return FS_FUNCTION;
      }
   else if (*sourceStr == _T('d'))
      {
      //m_sourceLocation = FS_DATABASE;
      LPTSTR _path = equal + 1;
      LPTSTR colon = _tcschr(_path, _T(':'));

      if (colon == NULL)
         {
         CString msg;
         msg.Format(_T("Flow: Error parsing Flow source string '%s'.  The source string must be of the form 'db=dllpath:entrypoint' "),
            sourceStr);
         Report::ErrorMsg(msg);
         return FS_UNDEFINED;
         }

      *colon = NULL;
      path = path;
      path.Trim();
      source = colon + 1;
      source.Trim();

      /*
      //int rows = pFluxInfo->m_pData->ReadAscii( pFluxInfo->m_path );  // ASSUMES CSV FOR NOW????
      //if ( rows <= 0  )
      //   {
      //   CString msg( _T( _T("Flux datasource error: Unable to open/read "))) );
      //   msg += path;
      //   Report::ErrorMsg( msg );
      //   pFluxInfo->m_use = false;
      //   continue;
      //   }

      col = pFluxInfo->m_pData->GetCol( pFluxInfo->m_fieldName );
      if ( pFluxInfo->m_col < 0 )
         {
         CString msg( _T("Unable to find Flux database field '")));
         msg += pFluxInfo->m_fieldName;
         msg += _T("' in "));
         msg += path;
         Report::ErrorMsg( msg );
         pFluxInfo->m_use = false;
         continue;
         } */
      }

   return FS_UNDEFINED;
   }



float GetVerticalDrainage(float wc)
   {
   //  For Loamy Sand (taken from Handbook of Hydrology).  .
   float wp = 0.035f;
   float porosity = 0.437f;
   float lamda = 0.553f;
   float kSat = 1.0f;//m/d
   float ratio = (wc - wp) / (porosity - wp);
   float power = 3.0f + (2.0f / lamda);
   float kTheta = (kSat)* pow(ratio, power);

   if (wc < wp) //no flux if too dry
      kTheta = 0.0f;

   return kTheta;//m/d 
   }


float GetBaseflow(float wc)
   {
   float k1 = 0.01f;
   float baseflow = wc*k1;
   return baseflow;//m/d
   }


float CalcRelHumidity(float specHumid, float tMean, float elevation)
   {
   // Atmospheric Pressure; kPa
   // p28, eq34
   float P = 101.3f * (float)pow(((293 - 0.0065 * elevation) / 293), 5.26);
   float e = specHumid / 0.622f * P;
   // temp in deg C
   float E = 6.112f * (float)exp(17.62*tMean / (243.12 + tMean));
   // rel. humidity [-]
   float ret = e / E;

   return ret;
   }

bool FlowModel::CheckHRUPoolValues( int doy, int year)
   {
   int count = 0;
   for (int i = 0; i < m_hruArray.GetSize(); i++)
      {
      HRU *pHRU = m_hruArray[i];
   
      for (int j = 0; j < pHRU->m_poolArray.GetSize(); j++)
         {
         if ( pHRU->m_poolArray[j]->m_volumeWater < 0)
            {
            CString msg;
            msg.Format( "Flow: HRUPool %i:%i has negative water content", i, j);
            TRACE( msg );
            Report::LogWarning(msg);
            pHRU->m_poolArray[j]->m_volumeWater = 0;
            count++;
            }
         }
      }

   if ( count > 0 )
      {
      CString msg;
      msg.Format( "Flow: Negative water volumes found in %i of %i HRUpools at day %i, year %i", count, int(m_hruArray.GetSize()*this->GetHRUPoolCount()),  doy, year );
      Report::LogWarning( msg );
      return false;
      }

   return true;
   }
