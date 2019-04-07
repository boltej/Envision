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
#include <EnvEngine\EnvModel.h>
#include <UNITCONV.H>
#include <DATE.HPP>
#include <VideoRecorder.h>
#include <PathManager.h>
#include <GDALWrapper.h>
#include <GeoSpatialDataObj.h>

//#include <EnvEngine\DeltaArray.h>

#include <EnvInterface.h>
#include <EnvExtension.h>
#include <EnvEngine\EnvConstants.h>

//#include <EnvView.h>
#include <omp.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern FlowProcess *gpFlow;

FlowModel *gpModel = NULL;


int LoadTable( LPCTSTR filename, DataObj** pDataObj, int type );  // type=0->FDataObj, type=1->VDataObj;

MTDOUBLE HRU::m_mvDepthMelt = 0;  // volume of water in snow
MTDOUBLE HRU::m_mvDepthSWE = 0;   // volume of ice in snow
MTDOUBLE HRU::m_mvCurrentPrecip = 0;
MTDOUBLE HRU::m_mvCurrentRainfall = 0;
MTDOUBLE HRU::m_mvCurrentSnowFall = 0;
MTDOUBLE HRU::m_mvCurrentAirTemp = 0;
MTDOUBLE HRU::m_mvCurrentMinTemp = 0;
MTDOUBLE HRU::m_mvCurrentET = 0;
MTDOUBLE HRU::m_mvCurrentMaxET = 0;
MTDOUBLE HRU::m_mvCurrentCGDD = 0;
MTDOUBLE HRU::m_mvCurrentSediment = 0;


MTDOUBLE Reach::m_mvCurrentStreamFlow = 0;
MTDOUBLE Reach::m_mvCurrentStreamTemp = 0;
MTDOUBLE Reach::m_mvCurrentTracer = 0;

MTDOUBLE HRU::m_mvCumET = 0;
MTDOUBLE HRU::m_mvCumRunoff = 0;
MTDOUBLE HRU::m_mvCumMaxET = 0;
MTDOUBLE HRU::m_mvCumP = 0;

MTDOUBLE HRULayer::m_mvWC = 0;
MTDOUBLE HRULayer::m_mvESV = 0;

int ParamTable::CreateMap( void )
   {
   if ( m_pTable == NULL )
      return -1;

   VData data;
   int rows = (int) m_pTable->GetRowCount();

   for ( int i=0; i < m_pTable->GetRowCount(); i++ )
      {
      m_pTable->Get( 0, i, data );
      m_lookupMap.SetAt( data, i );
      }

   return rows;
   }


bool ParamTable::Lookup( VData key, int col, int &value )
   {
   if ( m_pTable == NULL )
      return false;

   int row = -1;

   if ( m_lookupMap.Lookup( key, row ) )
      return m_pTable->Get( col, row, value );

   return false;
   }


bool ParamTable::Lookup( VData key, int col, float &value )
   {
   if ( m_pTable == NULL )
      return false;

   int row = -1;

   if ( m_lookupMap.Lookup( key, row ) )
      return m_pTable->Get( col, row, value );

   return false;
   }


bool ModelOutput::Init( MapLayer *pIDULayer )
   {
   if ( m_queryStr.GetLength() > 0 )
      {
      if ( gpModel->m_pModelOutputQE == NULL )
         {
         gpModel->m_pModelOutputQE = new QueryEngine( pIDULayer );
         ASSERT( gpModel != NULL );
         }

      if ( gpModel->m_pMapExprEngine == NULL )
         {
         gpModel->m_pMapExprEngine = new MapExprEngine( pIDULayer ); //pEnvContext->pExprEngine;
         ASSERT( gpModel->m_pMapExprEngine != NULL );

         // add model vars for HRU stuff - these are legal names in ModelOutput expressions
         //                     description          variable name    internal variable address, these are statics
         gpModel->AddModelVar( "Melt Depth (mm)",     "hruVolMelt",    &HRU::m_mvDepthMelt );  //
         gpModel->AddModelVar( "SWE Depth (mm)",      "hruVolSwe",     &HRU::m_mvDepthSWE );   //
         gpModel->AddModelVar( "Total Precip (mm)",   "hruPrecip",     &HRU::m_mvCurrentPrecip );
         gpModel->AddModelVar( "Total Rainfall (mm)", "hruRainfall",   &HRU::m_mvCurrentRainfall );
         gpModel->AddModelVar( "Total Snowfall (mm)", "hruSnowfall",   &HRU::m_mvCurrentSnowFall );
         gpModel->AddModelVar( "Air Temp (C)",        "hruAirTemp",    &HRU::m_mvCurrentAirTemp );
			gpModel->AddModelVar( "Min Temp (C)",			"hruMinTemp",    &HRU::m_mvCurrentMinTemp);
         gpModel->AddModelVar( "ET (mm)",             "hruET",         &HRU::m_mvCurrentET );
         gpModel->AddModelVar( "Maximum ET (mm)",     "hruMaxET",      &HRU::m_mvCurrentMaxET );
			gpModel->AddModelVar( "CGDD0 (heat units)",  "hruCGDD",		  &HRU::m_mvCurrentCGDD);
         gpModel->AddModelVar( "Sediment",				"hruSedimentOut",&HRU::m_mvCurrentSediment);
         gpModel->AddModelVar( "Streamflow (mm)",     "reachOutflow",  &Reach::m_mvCurrentStreamFlow );
         gpModel->AddModelVar("Stream Temp (C)",		"reachTemp",	  &Reach::m_mvCurrentStreamTemp);

         gpModel->AddModelVar( "Cumu. Rainfall (mm)",  "hruCumP",       &HRU::m_mvCumP );
         gpModel->AddModelVar( "Cumu. Runoff (mm)",    "hruCumRunoff",  &HRU::m_mvCumRunoff);
         gpModel->AddModelVar( "Cumu. ET (mm)",        "hruCumET",      &HRU::m_mvCumET);
         gpModel->AddModelVar( "Cumu. Max ET (mm)",    "hruCumMaxET",   &HRU::m_mvCumMaxET );

         gpModel->AddModelVar( "WC",              "hruLayerWC",    &HRULayer::m_mvWC );
         gpModel->AddModelVar( "Extra State Variable Conc",   "esv_hruLayer",  &HRULayer::m_mvESV );

         gpModel->AddModelVar( "Streamflow",      "esv_reachOutflow",    &Reach::m_mvCurrentTracer );
         }

      m_pQuery = gpModel->m_pModelOutputQE->ParseQuery( m_queryStr, 0, "Model Output Query" );
      
      if ( m_pQuery == NULL )
         {
         CString msg( "Flow: Error parsing query expression '" );
         msg += m_queryStr;
         msg += "' - the output will be ignored...";
         Report::ErrorMsg( msg );
         m_inUse = false;
         }

      m_pMapExpr = gpModel->m_pMapExprEngine->AddExpr( m_name, m_exprStr, m_queryStr );
      ASSERT( m_pMapExpr != NULL );

      bool ok = gpModel->m_pMapExprEngine->Compile( m_pMapExpr );

      if ( ! ok )
         {
         CString msg( "Flow: Unable to compile map expression " );
         msg += m_exprStr;
         msg += " for <output> '";
         msg += m_name;
         msg += "'.  The expression will be ignored";
         Report::ErrorMsg( msg );
         m_inUse = false;
         }
      }

   // expression

   return true;
   }


bool ModelOutput::InitStream( MapLayer *pStreamLayer )
   {
   if ( m_queryStr.GetLength() > 0 )
      {
      if ( gpModel->m_pModelOutputStreamQE == NULL )
         {
         gpModel->m_pModelOutputStreamQE = new QueryEngine( pStreamLayer );
         ASSERT(gpModel->m_pModelOutputStreamQE != NULL );
         }

      m_pQuery = gpModel->m_pModelOutputStreamQE->ParseQuery( m_queryStr, 0, "Model Output Query" );
      
      if ( m_pQuery == NULL )
         {
         CString msg( "Flow: Error parsing query expression '" );
         msg += m_queryStr;
         msg += "' - the output will be ignored...";
         Report::ErrorMsg( msg );
         m_inUse = false;
         }

      m_pMapExpr = gpModel->m_pMapExprEngine->AddExpr( m_name, m_exprStr, m_queryStr );
      ASSERT( m_pMapExpr != NULL );
      
      bool ok = gpModel->m_pMapExprEngine->Compile( m_pMapExpr );

      if ( ! ok )
         {
         CString msg( "Flow: Unable to compile map expression " );
         msg += m_exprStr;
         msg += " for <output> '";
         msg += m_name;
         msg += "'.  The expression will be ignored";
         Report::ErrorMsg( msg );
         m_inUse = false;
         }
      }

   // expression

   return true;
   }
            

ModelOutputGroup &ModelOutputGroup::operator = ( ModelOutputGroup &mog )
   {
   m_name = mog.m_name; 
   m_pDataObj = NULL; 
   m_moCountIdu = mog.m_moCountIdu;
   m_moCountHru = mog.m_moCountHru; 
   m_moCountHruLayer = mog.m_moCountHruLayer; 
   m_moCountReach = mog.m_moCountReach; 
 
   // copy each model output
   for ( int i=0; i < (int) mog.GetSize(); i++ )
      {
      ModelOutput *pOutput = new ModelOutput( *(mog.GetAt( i )) );
      ASSERT( pOutput != NULL );
      this->Add( pOutput );
      }  
   
   return *this;
   }


///////////////////////////////////////////////////////////////////////////////
// F L U X  I N F O 
///////////////////////////////////////////////////////////////////////////////

FluxInfo::FluxInfo( void )
: m_type( FT_UNDEFINED )
, m_dataSource( FS_UNDEFINED )
, m_sourceLocation( FL_UNDEFINED )
, m_sinkLocation( FL_UNDEFINED )
, m_sourceLayer( -1 )
, m_sinkLayer( -1 )
, m_pSourceQuery( NULL )
, m_pSinkQuery( NULL )
, m_pFluxFn( NULL )
, m_pInitFn( NULL )
, m_pInitRunFn( NULL )
, m_pEndRunFn( NULL )
, m_hDLL( NULL )
, m_pFluxData( NULL )
, m_col( -1 )
, m_use( true )
, m_totalFluxRate( 0 )      // instantaneous, reflects total for the given time step
, m_annualFlux( 0 )         // over the current year
, m_cumFlux( 0 )            // over the run
   { }


FluxInfo::~FluxInfo()
   {
   if ( m_pFluxData != NULL )
      delete m_pFluxData;

   if ( m_hDLL )
      AfxFreeLibrary( m_hDLL );
   }


///////////////////////////////////////////////////////////////////////////////
// F L U X 
///////////////////////////////////////////////////////////////////////////////

float Flux::Evaluate( FlowContext *pContext )
   {
   ASSERT( m_pFluxInfo != NULL );
   ASSERT( m_pFluxInfo->m_use != false );
   
   clock_t start = clock();

   switch( m_pFluxInfo->m_dataSource )
      {
      case FS_FUNCTION:
         ASSERT( m_pFluxInfo->m_pFluxFn != NULL );
         m_value = m_pFluxInfo->m_pFluxFn( pContext );
         break;

      case FS_DATABASE:
         ASSERT( m_pFluxInfo->m_pFluxData != NULL );
         m_value = m_pFluxInfo->m_pFluxData->IGet( pContext->time, m_pFluxInfo->m_col, IM_LINEAR );
         break;

      default:
         ASSERT( 0 );
      }

   m_pFluxInfo->m_totalFluxRate += m_value;

   clock_t finish = clock();
   float duration = (float)(finish - start) / CLOCKS_PER_SEC;   
   gpModel->m_totalFluxFnRunTime += duration;   

   return m_value;
   }


///////////////////////////////////////////////////////////////////////////////
// F L U X C O N T A I N E R 
///////////////////////////////////////////////////////////////////////////////

int FluxContainer::AddFlux( FluxInfo *pFluxInfo, StateVar *pStateVar )
   {
   ASSERT( pFluxInfo != NULL );
   ASSERT( pStateVar != NULL );
   
   Flux *pFlux = new Flux( pFluxInfo, pStateVar );
   ASSERT( pFlux != NULL );
   return (int) m_fluxArray.Add( pFlux );
   }


int FluxContainer::RemoveFlux( Flux *pFlux, bool doDelete ) 
   { 
   for ( int i=0; i < (int) m_fluxArray.GetSize(); i++ ) 
      {
      if ( m_fluxArray[ i ] == pFlux ) 
         { 
         if ( doDelete )
            m_fluxArray.RemoveAt( i );
         else
            m_fluxArray.RemoveAtNoDelete( i );

         return i; 
         }
      }
   return -1; 
   }


///////////////////////////////////////////////////////////////////////////////
// H R U L A Y E R  
///////////////////////////////////////////////////////////////////////////////

HRULayer::HRULayer( HRU *pHRU ) 
   : FluxContainer()
   , StateVarContainer()
   , m_volumeWater( 0.0f )
   , m_contributionToReach( 0.0f )
   , m_verticalDrainage( 0.0f )
   , m_horizontalExchange(0.0f)
   , m_wc(0.0f)
   , m_volumeLateralInflow( 0.0f )
   , m_volumeLateralOutflow( 0.0f )
   , m_volumeVerticalInflow( 0.0f )
   , m_volumeVerticalOuflow( 0.0f )
   , m_pHRU( pHRU )
   , m_layer( -1 )
   , m_type( HT_SOIL )
   , m_waterFluxArray(NULL)
   , m_depth(1.0f)
   {
   ASSERT( m_pHRU != NULL );
   }


HRULayer &HRULayer::operator = ( HRULayer &l )
   {
   m_volumeWater          = l.m_volumeWater;
   m_contributionToReach  = l.m_contributionToReach;
   m_verticalDrainage     = l.m_verticalDrainage;
   m_horizontalExchange   = l.m_verticalDrainage;
   m_wc                   = l.m_wc;
   m_volumeLateralInflow  = l.m_volumeLateralInflow ;        // m3/day
   m_volumeLateralOutflow = l.m_volumeLateralOutflow;
   m_volumeVerticalInflow = l.m_volumeVerticalInflow;
   m_volumeVerticalOuflow = l.m_volumeVerticalOuflow;
   m_depth = l.m_depth;

   // general info
   m_pHRU  = l.m_pHRU;
   m_layer = l.m_layer;
   m_type  = l.m_type;

   return *this;
   }


Reach *HRULayer::GetReach( void )
   {
   return m_pHRU->m_pCatchment->m_pReach;
   }

   

///////////////////////////////////////////////////////////////////////////////
// H R U 
///////////////////////////////////////////////////////////////////////////////

HRU::HRU( void ) 
   : m_id( -1 )
   , m_area(0.0f)
   , m_precip_yr(0.0f)
   , m_precip_wateryr(0.0f)
   , m_precip_10yr( 20 )
   , m_precipCumApr1( 0.0f )
   , m_temp_yr(0.0f)
   , m_rainfall_yr( 0.0f )
   , m_snowfall_yr( 0.0f )
   , m_areaIrrigated(0.0f)
   , m_temp_10yr( 20 )
   , m_depthMelt( 0.0f )
   , m_depthSWE( 0.0f )
   , m_depthApr1SWE_yr( 0.0f )
   , m_apr1SWE_10yr( 20 )
   , m_aridityIndex( 20 )
   , m_centroid(0.0f,0.0f)
   , m_elevation(50.0f)
   , m_currentPrecip(0.0f)
   , m_currentRainfall(0.0f)
   , m_currentSnowFall(0.0f)
   , m_currentAirTemp(0.0f)
	, m_currentMinTemp(0.0f)
   , m_currentET(0.0f)
   , m_maxET_yr(0.0f)
   , m_et_yr(0.0f)
   , m_currentRunoff(0.0f)
   , m_runoff_yr(0.0f)
   , m_initStorage(0.0f)
   , m_endingStorage(0.0f)
   , m_storage_yr(0.0f)
   , m_percentIrrigated(0.0f)
   , m_meanLAI(0.0f)
   , m_currentMaxET(0.0f)
	, m_currentCGDD(0.0f)
   , m_currentSediment(0.0f)
   , m_climateIndex( -1 )
   , m_climateRow( -1 )
   , m_climateCol( -1 )
   , m_demRow(-1)
   , m_demCol(-1)
   , m_waterFluxArray(NULL)
   , m_pCatchment( NULL ) 
   { }


int HRU::AddLayers( int soilLayerCount, int snowLayerCount, int vegLayerCount, float initWaterContent, float initTemperature, bool grid )
   {
   HRULayer hruLayer( this );

   int layer = 0;

   for ( int i=0; i < vegLayerCount; i++ )
      {
      HRULayer *pHRULayer = new HRULayer( hruLayer );
      ASSERT( pHRULayer != NULL );

      m_layerArray.Add( pHRULayer );
      
      m_layerArray[layer]->m_layer = layer;
      m_layerArray[layer]->m_type = HT_VEG;
      layer++;
      }

   for ( int i=0; i < snowLayerCount; i++ )
      {
      HRULayer *pHRULayer = new HRULayer( hruLayer );
      ASSERT( pHRULayer != NULL );

      m_layerArray.Add( pHRULayer );
      m_layerArray[layer]->m_layer = layer;
      m_layerArray[layer]->m_type = HT_SNOW;
      layer++;
      }

   for ( int i=0; i < soilLayerCount; i++ )
      {
      HRULayer *pHRULayer = new HRULayer( hruLayer );
      ASSERT( pHRULayer != NULL );
 
      m_layerArray.Add( pHRULayer );
      m_layerArray[layer]->m_layer = layer;
      m_layerArray[layer]->m_type = HT_SOIL;

      if (gpModel->m_hruLayerDepths.GetCount()>0)
         m_layerArray[layer]->m_depth=(float) atof(gpModel->m_hruLayerDepths[i]);

      if (gpModel->m_initWaterContent.GetCount()==1)
         m_layerArray[layer]->m_volumeWater=atof(gpModel->m_initWaterContent[0]);
      else if (gpModel->m_initWaterContent.GetCount()>1)
         {
         ASSERT(gpModel->m_hruLayerDepths.GetCount()>0);//if this asserts, it probably means you did not include the same number of initial watercontents as you did layer depths.  Check your xml file (catchments tag)
         m_layerArray[layer]->m_volumeWater=atof(gpModel->m_initWaterContent[i])*pHRULayer->m_depth*0.4f;
         }

      layer++;
      }

   for (int i=0;i<m_layerArray.GetSize();i++)
      {
      HRULayer *pLayer = m_layerArray[i];
      if (!grid) //if it is a grid, the water flux array is allocated by BuildCatchmentFromGrids.  
         {
         pLayer->m_waterFluxArray = new float[7];
         for (int j=0; j < 7; j++)
            pLayer->m_waterFluxArray[j]=0.0f;
         }
      }

   return (int) m_layerArray.GetSize();
   }


///////////////////////////////////////////////////////////////////////////////
// C A T C H M E N T 
///////////////////////////////////////////////////////////////////////////////

Catchment::Catchment( void ) 
: m_id( -1 )
, m_area( 0 )
, m_currentAirTemp( 0 )
, m_currentPrecip( 0 )
, m_currentThroughFall( 0 )
, m_currentSnow( 0 )
, m_currentET( 0 )
, m_currentGroundLoss( 0 )
, m_meltRate( 0 )
, m_contributionToReach( 0.0f )    // m3/day
, m_pDownslopeCatchment( NULL )
, m_pReach( NULL )
, m_pGW( NULL )
   { }


///////////////////////////////////////////////////////////////////////////////
// R E A C H 
///////////////////////////////////////////////////////////////////////////////
  
Reach::Reach(  )
: ReachNode()
, m_width( 5.0f )
, m_depth( 0.5f )
, m_wdRatio( 10.0f )
, m_cumUpstreamArea( 0 )
, m_cumUpstreamLength( 0 )
, m_climateIndex( -1 )
, m_alpha( 1.0f)
, m_beta( 3/5.0f)
, m_n( 0.1f )
, m_meanYearlyDischarge(0.0f)
, m_maxYearlyDischarge(0.0f)
, m_sumYearlyDischarge(0.0f)
, m_isMeasured(false)
, m_pReservoir( NULL )
, m_pStageDischargeTable( NULL )
, m_instreamWaterRightUse ( 0.0f )
, m_currentStreamTemp( 0.0f )
   { }


Reach::~Reach( void )
   {
   if ( m_pStageDischargeTable != NULL )
      delete m_pStageDischargeTable;
   }


void Reach::SetGeometry( float wdRatio )
   {
   //ASSERT( pReach->m_qArray != NULL );
   ReachSubnode *pNode = (ReachSubnode*) m_subnodeArray[ 0 ];
   ASSERT( pNode != NULL );

   m_depth = GetDepthFromQ( pNode->m_discharge, wdRatio );
   m_width = wdRatio * m_depth;
   m_wdRatio = wdRatio;
   }


//???? CHECK/DOCUMENT UNITS!!!!
float Reach::GetDepthFromQ( float Q, float wdRatio )  // ASSUMES A SPECIFIC CHANNEL GEOMETRY
   {
   // Q=m3/sec
   // from kellie:  d = ( 5/9 Q n s^2 ) ^ 3/8   -- assumes width = 3*depth, rectangular channel
   float wdterm = (float) pow( (wdRatio/( 2 + wdRatio )), 2.0f/3.0f)*wdRatio;
   float depth  = (float) pow(((Q*m_n)/((float)sqrt(m_slope)*wdterm)), 3.0f/8.0f);
   return (float) depth;
   }


float Reach::GetDischarge( int subnode /*=-1*/ )
   {
   if ( subnode < 0 )
      subnode = this->GetSubnodeCount()-1;

   ReachSubnode *pNode = (ReachSubnode*) this->m_subnodeArray[ subnode ];
   ASSERT( pNode != NULL );

   float q=pNode->m_discharge;

   if (q < 0.0f)
      q = 0.0f;

   return q;
   }
float Reach::GetUpstreamInflow(int subnode /*=-1*/)
   {
   if (subnode < 0)
      subnode = this->GetSubnodeCount() - 1;
   float Q = 0.0f;
   ReachSubnode *pNodeLeft = NULL;
   if (this->m_pLeft && this->m_pLeft->m_polyIndex >= 0)
      pNodeLeft = (ReachSubnode*) this->m_pLeft->m_subnodeArray[0];
   ReachSubnode *pNodeRight = NULL;
   if (this->m_pRight && this->m_pRight->m_polyIndex >= 0)
      pNodeRight = (ReachSubnode*) this->m_pRight->m_subnodeArray[0];
   if (pNodeLeft != NULL)
      Q = pNodeLeft->m_discharge;
   if (pNodeRight != NULL)
      Q += pNodeRight->m_discharge;

   return Q;
   }

///////////////////////////////////////////////////////////////////////////////
// R E S E R V O I R  
///////////////////////////////////////////////////////////////////////////////

Reservoir::Reservoir(void)
   : FluxContainer()
   , StateVarContainer()
   , m_id( -1 )
   , m_col( -1 )
   , m_dam_top_elev( 0 )
   , m_fc1_elev( 0 )
   , m_fc2_elev( 0 )
   , m_fc3_elev( 0 )
   , m_tailwater_elevation( 0 )
   , m_buffer_elevation( 0 )
   , m_turbine_efficiency( 0 )
   , m_minOutflow( 0 )
   , m_maxVolume( 1300 )
   , m_gateMaxPowerFlow( 0 )
   , m_maxPowerFlow( 0 )
   , m_minPowerFlow( 0 )
   , m_powerFlow( 0 )
   , m_gateMaxRO_Flow( 0 )
   , m_maxRO_Flow( 0 )
   , m_minRO_Flow( 0 )
   , m_RO_flow( 0 )
   , m_gateMaxSpillwayFlow( 0 )   
   , m_maxSpillwayFlow( 0 )   
   , m_minSpillwayFlow( 0 )
   , m_spillwayFlow( 0 )
	, m_reservoirType( ResType_FloodControl )
	, m_releaseFreq(1)
   , m_inflow( 0 )
   , m_outflow( 0 )
   , m_elevation( 0 )
   , m_power( 0 )
   , m_filled( 0 )
   , m_volume( 50 )   
   , m_pReach( NULL )
   , m_pFlowfromUpstreamReservoir( NULL )
   , m_pAreaVolCurveTable( NULL )
   , m_pRuleCurveTable( NULL )
   , m_pBufferZoneTable( NULL )
   , m_pCompositeReleaseCapacityTable( NULL )
   , m_pRO_releaseCapacityTable( NULL )
   , m_pSpillwayReleaseCapacityTable( NULL )
   , m_pRulePriorityTable ( NULL )
   , m_pResData( NULL )
   , m_pResMetData( NULL )
   , m_pResSimFlowCompare( NULL )
   , m_pResSimFlowOutput( NULL )
   , m_pResSimRuleCompare( NULL )
   , m_pResSimRuleOutput( NULL )
   , m_activeRule("Uninitialized")
   , m_zone(-1)
   , m_daysInZoneBuffer(0)
   { }


Reservoir::~Reservoir(void)
   {
   if ( m_pAreaVolCurveTable != NULL )
      delete m_pAreaVolCurveTable;

   if ( m_pRuleCurveTable != NULL )
      delete m_pRuleCurveTable;

   if ( m_pCompositeReleaseCapacityTable != NULL )
      delete m_pCompositeReleaseCapacityTable;

   if ( m_pRO_releaseCapacityTable != NULL )
      delete m_pRO_releaseCapacityTable;
   
   if ( m_pSpillwayReleaseCapacityTable != NULL )
      delete m_pSpillwayReleaseCapacityTable;
   
   if ( m_pRulePriorityTable != NULL )
      delete m_pRulePriorityTable;

   if ( m_pResData != NULL )
      delete m_pResData;

   if (m_pResMetData != NULL)
      delete m_pResMetData;

   if ( m_pResSimFlowCompare != NULL )
      delete m_pResSimFlowCompare;
   
   if ( m_pResSimFlowOutput != NULL )
      delete m_pResSimFlowCompare;

   if ( m_pResSimRuleCompare != NULL )
      delete m_pResSimRuleCompare;

   if ( m_pResSimRuleOutput != NULL )
      delete m_pResSimRuleOutput;

   }


ControlPoint::~ControlPoint(void)
{
	if (  m_pControlPointConstraintsTable != NULL )
		delete m_pControlPointConstraintsTable;

	if (  m_pResAllocation != NULL )
		delete  m_pResAllocation;
}


void Reservoir::InitDataCollection( void )
   {
   if ( m_pResData == NULL )
      {
		if (m_reservoirType == ResType_CtrlPointControl)
			m_pResData = new FDataObj(4, 0);
		else
			m_pResData = new FDataObj(8, 0);

      ASSERT( m_pResData != NULL );
      }

   CString name( this->m_name );
   name += " Reservoir";
   m_pResData->SetName( name );

	if (m_reservoirType == ResType_CtrlPointControl)
	   { 
		m_pResData->SetLabel(0, "Time");
		m_pResData->SetLabel(1, "Pool Elevation (m)");
		m_pResData->SetLabel(2, "Inflow (m3/s)");
		m_pResData->SetLabel(3, "Outflow (m3/s)");
	   }
	else
	   {
		m_pResData->SetLabel(0, "Time");
		m_pResData->SetLabel(1, "Pool Elevation (m)");
		m_pResData->SetLabel(2, "Rule Curve Elevation (m)");
		m_pResData->SetLabel(3, "Inflow (m3/s)");
		m_pResData->SetLabel(4, "Outflow (m3/s)");
		m_pResData->SetLabel(5, "Powerhouse Flow (m3/s)");
		m_pResData->SetLabel(6, "Regulating Outlet Flow (m3/s)");
		m_pResData->SetLabel(7, "Spillway Flow (m3/s)");
	   }

   gpFlow->AddOutputVar( name, m_pResData, "" );

   if (m_pResMetData == NULL)
      {
      m_pResMetData = new FDataObj(7, 0);
      ASSERT(m_pResMetData != NULL);
      }

   CString name2(name);
   name2 += " Met";
   m_pResMetData->SetName(name2);

   m_pResMetData->SetLabel(0, "Time");
   m_pResMetData->SetLabel(1, "Avg Daily Temp (C)");
   m_pResMetData->SetLabel(2, "Precip (mm/d)");
   m_pResMetData->SetLabel(3, "Shortwave Incoming (watts/m2)");
   m_pResMetData->SetLabel(4, "Specific Humidity ");
   m_pResMetData->SetLabel(5, "Wind Speed (m/s)");
   m_pResMetData->SetLabel(6, "Wind Direction");


   gpFlow->AddOutputVar(name2, m_pResMetData, "");

   if ( m_pResSimFlowCompare == NULL )
      {
      m_pResSimFlowCompare = new FDataObj( 7, 0 );
      ASSERT( m_pResSimFlowCompare != NULL );
      }
   
   m_pResSimFlowCompare->SetLabel( 0, "Time" );
   m_pResSimFlowCompare->SetLabel( 1, "Pool Elevation (m) - ResSIM" );
   m_pResSimFlowCompare->SetLabel( 2, "Pool Elevation (m) - Envision" );
   m_pResSimFlowCompare->SetLabel( 3, "Inflow (m3/s) - ResSIM" );
   m_pResSimFlowCompare->SetLabel( 4, "Inflow (m3/s) - Envision" );
   m_pResSimFlowCompare->SetLabel( 5, "Outflow (m3/s) - ResSIM" );
   m_pResSimFlowCompare->SetLabel( 6, "Outflow (m3/s) - Envision" );
   
   CString name3( this->m_name );
   name3 += " Reservoir - ReSIM Flow Comparison";
   m_pResSimFlowCompare->SetName( name3 );
   
   if ( m_pResSimRuleCompare == NULL )
      {
      m_pResSimRuleCompare = new VDataObj( 7, 0 );
      ASSERT( m_pResSimRuleCompare != NULL );
      }
   
   m_pResSimRuleCompare->SetLabel( 0, "Time" );
   m_pResSimRuleCompare->SetLabel( 1, "Blank");
   m_pResSimRuleCompare->SetLabel( 2, "Inflow (cms)");
   m_pResSimRuleCompare->SetLabel( 3, "Outflow (cms)");
   m_pResSimRuleCompare->SetLabel( 4, "ews (m)");
   m_pResSimRuleCompare->SetLabel( 5, "Zone");
   m_pResSimRuleCompare->SetLabel( 6, "Active Rule - Envision" );

   CString name4( this->m_name );
   name4 += " Reservoir - ReSIM Rule Comparison";
   m_pResSimRuleCompare->SetName( name4 );
   }


// this function iterates through the columns of the rule priority table,
// loading up constraints
bool Reservoir::ProcessRulePriorityTable( void )
   {
   if ( m_pRulePriorityTable == NULL )
      return false;

   // in this table, each column is a zone.  Each row is a csv file
   int cols = m_pRulePriorityTable->GetColCount();
   int rows = m_pRulePriorityTable->GetRowCount();

   // iterate through the zones defined in the file
   for ( int i=0; i < cols; i++ )
      {
      ZoneInfo *pZone = new ZoneInfo;
      ASSERT( pZone != NULL );

      pZone->m_zone = (ZONE) i;  // ???????

      this->m_zoneInfoArray.Add( pZone );

      // iterate through the csv files for this zone
      for ( int j=0; j < rows; j++ )
         {
         CString filename = m_pRulePriorityTable->GetAsString( i, j );
              
         ResConstraint *pConstraint = FindResConstraint(gpModel->m_path+m_dir+m_rpdir+filename );
       
         if ( pConstraint == NULL && filename.CompareNoCase( "Missing" ) != 0  ) // not yet loaded?  and not 'Missing'?    Still not loaded?
            {
            pConstraint = new ResConstraint;
            ASSERT( pConstraint );

            pConstraint->m_constraintFileName = filename;
      
            // Parse heading to determine whether to look in control point folder or rules folder
            TCHAR heading[ 256 ];
            lstrcpy( heading, filename );
            
            TCHAR *control = NULL;
            TCHAR *next    = NULL;
            TCHAR delim[] = " ,_";  //allowable delimeters, space and underscore
            
            control = _tcstok_s( heading, delim, &next );  // Returns string at head of file. 
            int records = -1;

            if ( lstrcmpi( control, "cp" ) == 0 )
               records = LoadTable(gpModel->m_path+ m_dir+m_cpdir+filename, (DataObj**) &(pConstraint->m_pRCData), 0 );
            else
               records = LoadTable( gpModel->m_path+m_dir+m_rpdir+filename, (DataObj**) &(pConstraint->m_pRCData), 0 ); 
            
            if ( records <= 0 )  // No records? Display error msg
               {
               CString msg;
               msg.Format( "Flow: Unable to load reservoir constraints for Zone %i table %s in Reservoir rule priority file %s.  This constraint will be ignored...",
                    i, (LPCTSTR) filename, (LPCTSTR) m_pRulePriorityTable->GetName() );

               Report::ErrorMsg( msg );
               delete pConstraint;
               pConstraint = NULL;
               } // end of if (records <=0)
            else
               this->m_resConstraintArray.Add( pConstraint );
            }  // end of ( if ( pConstraint == NULL )
          
         // Get rule type (m_type).  Assume 1st characters of filename until first underscore or space contain rule type.  
         // Parse rule name and determine m_type  (maximum, minimum, Maximum rate of decrease, Maximum Rate of Increase)
         if ( pConstraint != NULL )
            {
            TCHAR  name[ 256 ];
            lstrcpy( name, filename );

            TCHAR *ruletype = NULL;
            TCHAR *next     = NULL;
            TCHAR  delim[] = " ,_";  //allowable delimeters, space and underscore
            
            // Ruletypes
            ruletype = strtok_s(name, delim, &next);  //Returns string at head of file. 
            
            //Compare string at the head of the file to allowable ruletypes
            if ( lstrcmpi( ruletype, "max" ) == 0 )
               pConstraint->m_type = RCT_MAX;
            else if ( lstrcmpi( ruletype, "min") == 0 )
               pConstraint->m_type = RCT_MIN;
            else if ( lstrcmpi( ruletype,"maxi") == 0 )
               pConstraint->m_type = RCT_INCREASINGRATE;
            else if ( lstrcmpi( ruletype, "maxd" ) == 0 )
               pConstraint->m_type = RCT_DECREASINGRATE;
            else if ( lstrcmpi( ruletype,"cp" ) == 0 )      //downstream control point
               pConstraint->m_type = RCT_CONTROLPOINT;
            else if ( lstrcmpi( ruletype, "pow") == 0 )     //Power plant flow
               pConstraint->m_type = RCT_POWERPLANT;
            else if ( lstrcmpi( ruletype, "ro" ) == 0 )     //RO flow
               pConstraint->m_type = RCT_REGULATINGOUTLET;
            else if ( lstrcmpi( ruletype, "spill" ) == 0)   //Spillway flow
               pConstraint->m_type = RCT_SPILLWAY;
            else 
               pConstraint->m_type = RCT_UNDEFINED;    //Unrecognized rule type     
            }  //end of if pConstraint != NULL )
   
         if ( pConstraint != NULL )    // if valid, add to the current zone
            {
            ASSERT( pZone != NULL );
            pZone->m_resConstraintArray.Add( pConstraint );
            }
         }  // end of: if ( j < rows )
      }  // end of: if ( i < cols );

   return true;
   }
   

// get pool elevation from volume - units returned are meters
float Reservoir::GetPoolElevationFromVolume()
   {
   if ( m_pAreaVolCurveTable == NULL )
      return 0;
   
   float volume = (float) m_volume;

   float poolElevation = m_pAreaVolCurveTable->IGet( volume, 1, 0, IM_LINEAR );  // this is in m
   
   return poolElevation;
   }


// get pool volume from elevation - units returned are meters cubed
float Reservoir::GetPoolVolumeFromElevation(float elevation)
   {
   if ( m_pAreaVolCurveTable == NULL )
      return 0;

   float poolVolume = m_pAreaVolCurveTable->IGet( elevation, 0, 1, IM_LINEAR );  // this is in m3
  
   return poolVolume;    //this is M3
   }

float Reservoir::GetBufferZoneElevation( int doy )
   {
   if ( m_pBufferZoneTable == NULL )
      return 0;

   // convert time to day of year
   //int doy = ::GetJulianDay( gpModel->m_month, gpModel->m_day, gpModel->m_year );   // 1-based
   //doy -= 1;  // make zero-based
   //int dayOfYear = int( fmod( m_timeInRun, 365 ) );  // zero based day of year

   //time = fmod( time, 365 );
   float bufferZoneElevation = m_pBufferZoneTable->IGet(float(doy), 1, IM_LINEAR);   // m
   return bufferZoneElevation;    //this is M
   }


// get target pool elevation from rule curve - units returned are meters
float Reservoir::GetTargetElevationFromRuleCurve( int doy  )
   {
   if ( m_pRuleCurveTable == NULL )
      return m_elevation;

   // convert time to day of year
   //int doy = ::GetJulianDay( gpModel->m_month, gpModel->m_day, gpModel->m_year );   // 1-based
   //doy -= 1;  // make zero-based

   //int dayOfYear = int( fmod( m_timeInRun, 365 ) );  // zero based day of year
   //time = fmod( time, 365 );
   float target = m_pRuleCurveTable->IGet( float( doy ), 1, IM_LINEAR );   // m
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


float Reservoir::GetResOutflow(Reservoir *pRes, int doy)
   {
   ASSERT( pRes != NULL );

   float outflow = 0.0f;

   float currentPoolElevation = pRes->GetPoolElevationFromVolume();
   pRes->m_elevation = currentPoolElevation;
   UpdateMaxGateOutflows( pRes, currentPoolElevation );  

   // check for river run reservoirs 
   if (pRes->m_reservoirType == ResType_RiverRun)
      {
      outflow = pRes->m_inflow / SEC_PER_DAY;    // convert outflow to m3/day
      pRes->m_activeRule ="RunOfRiver";
      }
   else
      {
      float targetPoolElevation = pRes->GetTargetElevationFromRuleCurve( doy );
      float targetPoolVolume    = pRes->GetPoolVolumeFromElevation(targetPoolElevation);  
      float currentVolume       = pRes->GetPoolVolumeFromElevation(currentPoolElevation);
      float bufferZoneElevation = pRes->GetBufferZoneElevation( doy );
      
      if ( currentVolume > pRes->m_maxVolume )
         {
         currentVolume = pRes->m_maxVolume;   //Don't allow res volumes greater than max volume.  This code may be removed once hydro model is calibrated.
         pRes->m_volume = currentVolume;      //Store adjusted volume as current volume.
         currentPoolElevation = pRes->GetPoolElevationFromVolume();  //Adjust pool elevation
         
         CString msgvol;
         msgvol.Format( "Flow: Reservoir volume at '%s' on day of year %i exceeds maximum.  Volume set to maxVolume. Mass Balance not closed.", (LPCTSTR) pRes->m_name, doy);
         Report::StatusMsg( msgvol );
         pRes->m_activeRule = "OverMaxVolume!";
         }

      pRes->m_volume = currentVolume;
      
      float desiredRelease = (currentVolume - targetPoolVolume)/SEC_PER_DAY;     //This would bring pool elevation back to the rule curve in one timestep.  Converted from m3/day to m3/s  (cms).  
                                                                   //This would be ideal if possible within the given constraints.
      if (currentVolume < targetPoolVolume) //if we want to fill the reservoir
         desiredRelease=pRes->m_minOutflow;
         //desiredRelease = 0.0f;//kbv 3/16/2014
      
      float actualRelease = desiredRelease;   //Before any constraints are applied
      
		
      pRes->m_activeRule = "GC";  //.Format(gc);               //Notation from Ressim...implies that guide curve is reached with actual release (e.g. no other constraints applied).
       
      ZONE zone = ZONE_UNDEFINED;   //zone 0 = top of dam, zone 1 = flood control high, zone 2 = conservation operations, zone 3 = buffer, zone 4 = alternate flood control 1, zone 5 = alternate flood control 2.  Right now, only use 1 and 2 (mmc 3/2/2012). 

      if ( currentPoolElevation > pRes->m_dam_top_elev )
         {
         CString msgtop;
         msgtop.Format( "Flow: Reservoir elevation at '%s' on day of year %i exceeds dam top elevation.", (LPCTSTR) pRes->m_name, doy);
         Report::StatusMsg( msgtop );
         currentPoolElevation = pRes->m_dam_top_elev - 0.1f;    //Set just below top elev to keep from exceeding values in lookup tables
         zone = ZONE_TOP;
         pRes->m_activeRule = "OverTopElevation!";
         }

		else if (pRes->m_reservoirType == ResType_CtrlPointControl)
   		{
			zone = ZONE_CONSERVATION;
	   	}
      else if (currentPoolElevation > pRes->m_fc1_elev)
         {
         zone = ZONE_TOP;
         pRes->m_activeRule = "OverFC1";
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
            else if (pRes->m_zone = ZONE_BUFFER)//if the zone did not change (you were in the buffer on the previous day)
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
      if ( zone >= 0 && zone <pRes->m_zoneInfoArray.GetSize() )
			pZone = pRes->m_zoneInfoArray.GetAt(zone);

      if ( pZone != NULL )
         {
         // Loop through constraints and modify actual release.  Apply the flood control rules in order here.
         for ( int i=0; i < pZone->m_resConstraintArray.GetSize(); i++ ) 
            {
            ResConstraint *pConstraint = pZone->m_resConstraintArray.GetAt(i);
            ASSERT( pConstraint != NULL );

            CString filename = pConstraint->m_constraintFileName;
            
            // Get first column label and appropriate data to use for lookup
            DataObj *pConstraintTable = pConstraint->m_pRCData;
            ASSERT( pConstraintTable != NULL );
            
            CString xlabel = pConstraintTable->GetLabel(0);
            
            int year=-1, month=1, day=-1;
            float xvalue = 0;
            float yvalue = 0;
   
            if (_stricmp(xlabel,"date") == 0)                           // Date based rule?  xvalue = current date.
               xvalue = (float) doy; 
            else if  (_stricmp(xlabel,"release_cms") == 0)              // Release based rule?  xvalue = release last timestep
               xvalue = pRes->m_outflow/SEC_PER_DAY;                    // SEC_PER_DAY = cubic meters per second to cubic meters per day
            else if  (_stricmp(xlabel,"pool_elev_m") == 0)              // Pool elevation based rule?  xvalue = pool elevation (meters)
               xvalue = pRes->m_elevation;
            else if  (_stricmp(xlabel,"inflow_cms") == 0)               // Inflow based rule?   xvalue = inflow to reservoir
               xvalue = pRes->m_inflow/SEC_PER_DAY;               
            else if  (_stricmp(xlabel, "Outflow_lagged_24h" ) == 0)     // 24h lagged outflow based rule?   xvalue = outflow from reservoir at last timestep
               xvalue = pRes->m_outflow/SEC_PER_DAY;                    // placeholder for now
            else if  (_stricmp(xlabel,"date_pool_elev_m") == 0)         // Lookup based on two values...date and pool elevation.  x value is date.  y value is pool elevation
               {
               xvalue = (float) doy;
               yvalue = pRes->m_elevation;
               }
            else if  (_stricmp(xlabel,"date_water_year_type") == 0)         //Lookup based on two values...date and wateryeartype (storage in 13 USACE reservoirs on May 20th).
               {
               xvalue = (float) doy;
               yvalue = gpModel->m_waterYearType;
               }
            else if  (_stricmp(xlabel, "date_release_cms") == 0) 
               {
               xvalue = (float) doy;
               yvalue = pRes->m_outflow/SEC_PER_DAY;
               }
            else                                                    //Unrecognized xvalue for constraint lookup table
               { 
               CString msg;
               msg.Format( "Flow:  Unrecognized x value for reservoir constraint lookup '%s', %s (id:%i) in stream network", (LPCTSTR) pConstraint->m_constraintFileName, (LPCTSTR) pRes->m_name, pRes->m_id );
               Report::WarningMsg( msg );
               }
   
            RCTYPE type = pConstraint->m_type;
            float constraintValue=0;
   
            ASSERT( pConstraint->m_pRCData != NULL );

            switch( type )
               {
               case RCT_MAX:  //maximum
                  {
                  if ( yvalue > 0 )  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                     constraintValue = pConstraint->m_pRCData->IGet(xvalue, yvalue, IM_LINEAR);
                  else             //If not, just use xvalue
                     constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                  
                  if ( actualRelease >= constraintValue )
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
                     constraintValue = constraintValue*24;   //Covert hourly to daily
                     }
                  else             //If not, just use xvalue
                     {
                     constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                     constraintValue = constraintValue*24;   //Covert hourly to daily
                     }
   
                  if (actualRelease >= (constraintValue + pRes->m_outflow/SEC_PER_DAY))   //Is planned release more than current release + contstraint? 
                     {
                     actualRelease = (pRes->m_outflow/SEC_PER_DAY) + constraintValue;  //If so, planned release can be no more than current release + constraint.
                     pRes->m_activeRule = pConstraint->m_constraintFileName;
                     }
                  }
                  break;

               case RCT_DECREASINGRATE:  //Decreasing Rate */
                  {
                  if (yvalue > 0)  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                     {
                     constraintValue = pConstraint->m_pRCData->IGet(xvalue, yvalue, IM_LINEAR);
                     constraintValue = constraintValue*24;   //Covert hourly to daily
                     }
                  else             //If not, just use xvalue
                     {
                     constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                     constraintValue = constraintValue*24;   //Covert hourly to daily
                     }
   
                  if (actualRelease <= (pRes->m_outflow/SEC_PER_DAY - constraintValue))    //Is planned release less than current release - contstraint?
                     {
                     actualRelease = (pRes->m_outflow/SEC_PER_DAY) - constraintValue;     //If so, planned release can be no less than current release - constraint.
                     pRes->m_activeRule = pConstraint->m_constraintFileName;
                     }
                  }
                  break;

               case RCT_CONTROLPOINT:  //Downstream control point 
                  {
	                CString filename = pConstraint->m_constraintFileName;
                  // get control point location.  Assumes last characters of filename contain a COMID
                  LPTSTR p = (LPTSTR) _tcsrchr( filename, '.' );
   
                  if ( p != NULL )
                     {
                     p--;
                     while ( isdigit( *p ) )
                        p--;
   
                     int comID = atoi( p+1 );
                     pConstraint->m_comID = comID;
                     }
                        
                  //Determine which control point this is.....use COMID to identify
                  for (int k=0;  k < gpModel->m_controlPointArray.GetSize(); k++) 
                     {
                     ControlPoint *pControl = gpModel->m_controlPointArray.GetAt(k);
                     ASSERT( pControl != NULL );

                     if ( pControl->InUse() )
                        {                     
                        int location = pControl->m_location;     // Get COMID of this control point
                        //if (pControl->m_location == pConstraint->m_comID)  //Do they match?
                        if (_stricmp(pControl->m_controlPointFileName,pConstraint->m_constraintFileName) == 0)  //compare names
                           {
                           ASSERT( pControl->m_pResAllocation != NULL );
									constraintValue = 0.0f;
									int releaseFreq = 1;

                           for ( int l=0; l < pControl->m_influencedReservoirsArray.GetSize(); l++ )
                              {
                              if ( pControl->m_influencedReservoirsArray[ l ] == pRes )
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
						//						constraintValue = pControl->m_pResAllocation->IGet(0, l);   //Flow allocated from this control point
											}
											else
												constraintValue = pControl->m_pResAllocation->IGet(0, l );   //Flow allocated from this control point
                                 
											actualRelease += constraintValue;    //constraint value will be negative if flow needs to be withheld, positive if flow needs to be augmented
                                 
                                 if ( constraintValue > 0.0 )         //Did this constraint affect release?  If so, save as active rule.
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
                  TCHAR *next     = NULL;
                  TCHAR delim[] = " ,_";  //allowable delimeters: , ; space ; underscore

                  ruletype = _tcstok_s(powname, delim, &next);  //Strip header.  should be pow_ for power plant rules
                  ruletype = _tcstok_s(NULL, delim, &next);  //Returns next string at head of file (max or min).
               
                  if (_stricmp(ruletype, "Max") == 0)  //Is this a maximum power flow?  Assign m_maxPowerFlow attribute.
                     {
                     ASSERT(  pConstraint->m_pRCData != NULL );
                     constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                     pRes->m_maxPowerFlow = constraintValue;  //Just for this timestep.  m_gateMaxPowerFlow is the physical limitation for the reservoir.
                     }
                  else if (_stricmp(ruletype, "Min" ) == 0 )   /// bug!!! maximum) == 0)  //Is this a minimum power flow?  Assign m_minPowerFlow attribute.
                     {
                     ASSERT(  pConstraint->m_pRCData != NULL );
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
                  TCHAR *next     = NULL;
                  TCHAR delim[] = " ,_";  //allowable delimeters: , ; space ; underscore
                  ruletype = _tcstok_s(roname, delim, &next);  //Strip header.  should be pow_ for power plant rules
                  ruletype = _tcstok_s(NULL, delim, &next);  //Returns next string at head of file (max or min).
   
                  if (_stricmp(ruletype, "Max") == 0)  //Is this a maximum RO flow?   Assign m_maxRO_Flow attribute.
                     {
                     constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                     pRes->m_maxRO_Flow = constraintValue;
                     }
                  else if (_stricmp(ruletype, "Min" ) == 0)  //Is this a minimum RO flow?   Assign m_minRO_Flow attribute.
                     {
                     ASSERT(  pConstraint->m_pRCData != NULL );
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
                  TCHAR *next     = NULL;
                  TCHAR delim[] = " ,_";  //allowable delimeters: , ; space ; underscore

                  ruletype = _tcstok_s(spillname, delim, &next);  //Strip header.  should be pow_ for power plant rules
                  ruletype =_tcstok_s(NULL, delim, &next);  //Returns next string at head of file (max or min).
               
                  if (_stricmp(ruletype, "Max" ) == 0)  //Is this a maximum spillway flow?  Assign m_maxSpillwayFlow attribute.
                     {
                     ASSERT(  pConstraint->m_pRCData != NULL );
                     constraintValue = pConstraint->m_pRCData->IGet(xvalue, 1, IM_LINEAR);
                     pRes->m_maxSpillwayFlow = constraintValue;
                     }
                  else if (_stricmp(ruletype, "Min") == 0)  //Is this a minimum spillway flow?  Assign m_minSpillwayFlow attribute.
                     {
                     ASSERT(  pConstraint->m_pRCData != NULL );
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

               resMsg.Format("Pool is only %8.1f m above inactive zone. Outflow set to drain %8.1fm above inactive zone.  RC outflow of %8.1f m3/s (from %s) would have resulted in %8.0f m3 of discharged water (over a day) but there is only %8.0f m3 above the inactive zone", pRes->m_elevation - pRes->m_inactive_elev, (pRes->m_elevation - pRes->m_inactive_elev) / 2, rc_outflowVolum / SEC_PER_DAY, pRes->m_activeRule, rc_outflowVolum, currentVolume - minVolum);
                  pRes->m_activeRule = resMsg;
              
               }
               */
            if (actualRelease < 0.0f)
               actualRelease = 0.0f;
            pRes->m_zone = (int)zone;

          // if (actualRelease < pRes->m_minOutflow)              // No release values less than the minimum
          //     actualRelease = pRes->m_minOutflow;

            if (pRes->m_elevation < pRes->m_inactive_elev)      //In the inactive zone, water is not accessible for release from any of the gates.
               actualRelease = pRes->m_outflow/SEC_PER_DAY*0.5f;

            outflow = actualRelease;
            }//end of for ( int i=0; i < pZone->m_resConstraintArray.GetSize(); i++ )
         }
      }  // end of: else (not run of river gate flow
 
   //Code here to assign total outflow to powerplant, RO and spillway (including run of river projects) 
	if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_RiverRun )
		AssignReservoirOutletFlows(pRes, outflow);


   // output activ rule to log window
   CString msg;
   msg.Format("Flow: Day %i: Active Rule for Reservoir %s is %s", doy, (LPCTSTR)pRes->m_name, (LPCTSTR)pRes->m_activeRule);


   return outflow;
   }


void Reservoir::AssignReservoirOutletFlows( Reservoir *pRes, float outflow )
   {
   ASSERT( pRes != NULL );

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

         pRes->m_spillwayFlow = excessFlow;
         
         if ( pRes->m_spillwayFlow < pRes->m_minSpillwayFlow )   //If spillway has a minimum flow, reduce RO flow to accomodate
            {
            pRes->m_spillwayFlow = pRes->m_minSpillwayFlow;
            pRes->m_RO_flow -= (pRes->m_minSpillwayFlow - excessFlow);
            }
         else if (pRes->m_spillwayFlow > pRes->m_maxSpillwayFlow)    //Does spillway flow exceed the max?  Give warning.
            {
            CString spillmsg;
            spillmsg.Format( "Flow: Maximum spillway volume exceeded at '%s', (id:%i)", (LPCTSTR) pRes->m_name, pRes->m_id  );
            Report::LogMsg( spillmsg );
            }
         }

     
      }
   //Reset max outflows to gate maximums for next timestep
     pRes->m_maxPowerFlow = pRes->m_gateMaxPowerFlow;
     pRes->m_maxRO_Flow = pRes->m_gateMaxRO_Flow;
     pRes->m_maxSpillwayFlow = pRes->m_gateMaxSpillwayFlow;

   float massbalancecheck = outflow - (pRes->m_powerFlow + pRes->m_RO_flow + pRes->m_spillwayFlow);    //massbalancecheck should = 0
   }

void  Reservoir::CalculateHydropowerOutput( Reservoir *pRes )
   {
      float head = pRes->m_elevation - pRes->m_tailwater_elevation;
      float powerOut = (1000*pRes->m_powerFlow*9.81f*head*pRes->m_turbine_efficiency)/1000000;   //power output in MW
      pRes->m_power = powerOut;

   }

void Reservoir::CollectData( void )
   {

   }


////////////////////////////////////////////////////////////////////////////////////////////////
// F L O W     M O D E L
////////////////////////////////////////////////////////////////////////////////////////////////

FlowModel::FlowModel()
 : m_id( -1 )
 , m_saveStateAtEndOfRun(false)
 , m_detailedOutputFlags( 0 )
 , m_reachTree()
 , m_minStreamOrder( 0 )
 , m_subnodeLength( 0 )
 , m_buildCatchmentsMethod( 1 )
 , m_soilLayerCount( 1 )
 , m_vegLayerCount( 0 )
 , m_snowLayerCount( 0 )
 , m_initTemperature( 20 )
 , m_hruSvCount( 0 )
 , m_reachSvCount( 0 )
 , m_reservoirSvCount( 0 )
 , m_colCatchmentArea( -1 )
 , m_colCatchmentJoin( -1 )
 , m_colCatchmentReachIndex( -1 )
 , m_colCatchmentCatchID( -1 )
 , m_colCatchmentHruID( -1 )
 , m_colCatchmentReachId( -1 )
 , m_colStreamFrom( -1 )
 , m_colStreamTo( -1 )
 , m_colStreamOrder( -1 )
 , m_colHruSWC( -1 )
 , m_colHruTemp( -1 )
 , m_colHruTempYr( -1 )
 , m_colHruTemp10Yr( -1 )
 , m_colHruPrecip( -1 )
 , m_colHruPrecipYr( -1 )
 , m_colHruPrecip10Yr( -1 )
 , m_colHruSWE( -1 )
 , m_colHruMaxSWE(-1 )
 , m_colHruApr1SWE10Yr( -1 )
 , m_colHruApr1SWE( -1 )
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
 , m_colStreamReachOutput( -1 )
 , m_colResID( -1 )
 , m_colStreamCumArea( -1 )
 , m_colCatchmentCumArea( -1 )
 , m_colTreeID( -1 )
 , m_pMap( NULL )
 , m_pCatchmentLayer( NULL )
 , m_pStreamLayer( NULL )
 , m_pResLayer( NULL )
 , m_stopTime( 0.0f )
 , m_currentTime( 0.0f )
 , m_timeStep( 1.0f )
 , m_collectionInterval( 1 )
 , m_useVariableStep( false )
 , m_rkvMaxTimeStep(10.0f)
 , m_rkvMinTimeStep(0.001f)
 , m_rkvTolerance(0.0001f)
 , m_rkvInitTimeStep(0.001f)

 , m_pReachRouting( NULL )
 , m_pLateralExchange( NULL )
 , m_pHruVertExchange( NULL )

 //, m_extraSvRxnMethod;
 , m_extraSvRxnExtFnDLL( NULL )
 , m_extraSvRxnExtFn( NULL )
 //,  RESMETHOD  m_reservoirFluxMethod;
 , m_reservoirFluxExtFnDLL( NULL )
 , m_reservoirFluxExtFn( NULL )
 , m_pResInflows( NULL )
 , m_pCpInflows( NULL )

 //, m_reachSolutionMethod( RSM_RK4 )
 //, m_latExchMethod( HREX_LINEARRESERVOIR )
 //, m_hruVertFluxMethod( VD_BROOKSCOREY )
 //, m_gwMethod( GWT_NONE )
 , m_extraSvRxnMethod( EXSV_EXTERNAL )
 , m_reservoirFluxMethod( RES_RESSIMLITE)
 //, m_reachExtFnDLL( NULL )
 //, m_reachExtFn( NULL )
 //, m_reachTimeStep( 1.0f )
 //, m_latExchExtFnDLL( NULL )
 //, m_latExchExtFn( NULL )
 //, m_hruVertFluxExtFnDLL( NULL )
 //, m_hruVertFluxExtFn( NULL )
 //, m_gwExtFnDLL( NULL )
 //, m_gwExtFn( NULL )

 , m_wdRatio( 10.0f ) //?????
 , m_minCatchmentArea( -1 )
 , m_maxCatchmentArea( -1 )
 , m_numReachesNoCatchments(0)
 , m_numHRUs(0)
 , m_pStreamQE( NULL )
 , m_pCatchmentQE( NULL )
 , m_pModelOutputQE( NULL )
 , m_pModelOutputStreamQE( NULL )
 , m_pMapExprEngine( NULL )
 , m_pStreamQuery( NULL )
 , m_pCatchmentQuery( NULL )
 //, m_pReachDischargeData( NULL )
 //, m_pHRUPrecipData( NULL )
 //, m_pHRUETData(NULL)
 , m_pTotalFluxData( NULL )
 , m_pGlobalFlowData( NULL )
 , m_mapUpdate( 2 )
 , m_gridClassification(2)
 , m_totalWaterInputRate( 0 )
 , m_totalWaterOutputRate( 0 )
 , m_totalWaterInput( 0 )
 , m_totalWaterOutput( 0 )
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
 , m_pClimateStationData(NULL)
 , m_timeOffset(0)
 , m_path()
 , m_grid()
 , m_climateFilePath()
 , m_climateStationElev(0.0f)
 , m_loadClimateRunTime( 0.0f )   
 , m_globalFlowsRunTime( 0.0f )   
 , m_externalMethodsRunTime( 0.0f )   
 , m_gwRunTime( 0.0f )   
 , m_hruIntegrationRunTime( 0.0f )   
 , m_reservoirIntegrationRunTime( 0.0f )   
 , m_reachIntegrationRunTime( 0.0f )   
 , m_massBalanceIntegrationRunTime( 0.0f )
 , m_totalFluxFnRunTime( 0.0f )
 , m_reachFluxFnRunTime( 0.0f )
 , m_hruFluxFnRunTime( 0.0f )
 , m_collectDataRunTime( 0.0f )
 , m_outputDataRunTime( 0.0f )
 , m_checkMassBalance( 0 )
 , m_integrator(IM_RK4)
 , m_minTimeStep(0.001f)
 , m_maxTimeStep(1.0f)
 , m_initTimeStep(1.0f)
 , m_errorTolerance(0.0001f)
 , m_useRecorder( false )
 , m_useStationData( false )
 , m_currentScenarioIndex( 0 )
 , m_pGrid(NULL)
 , m_minElevation(float(1E-6))
 , m_volumeMaxSWE(0.0f)
 , m_dateMaxSWE(0)
 , m_annualTotalET( 0 )     // acre-ft
 , m_annualTotalPrecip( 0 ) // acre-ft
 , m_annualTotalDischarge( 0 )  //acre-ft
 , m_annualTotalRainfall( 0 )  //acre-ft
 , m_annualTotalSnowfall( 0 )  //acre-ft
   {
   gpModel = this;
   gpFlow->AddInputVar( "Climate Scenario", m_currentScenarioIndex, "0=MIROC, 1=GFDL, 2=HadGEM, 3=Historic" );    
   gpFlow->AddInputVar( "Use Parameter Estimation", m_estimateParameters, "true if the model will be used to estimate parameters" );    
   gpFlow->AddInputVar( "Grid Classification?", m_gridClassification, "For Gridded Simulation:  Classify attribute 0,1,2?" );    

   gpFlow->AddOutputVar("Date of Max Snow", m_dateMaxSWE, "DOY of Max Snow");
   gpFlow->AddOutputVar("Volume Max Snow (cubic meters)", m_volumeMaxSWE, "Volume Max Snow (m3)");

   // the following were moved to m_pTotalFluxData dataobj
   //gpFlow->AddOutputVar("Annual Total ET (acre-ft)", m_annualTotalET, "");
   //gpFlow->AddOutputVar("Annual Total Precip (acre-ft)", m_annualTotalPrecip, "");
   //gpFlow->AddOutputVar("Annual Total Discharge (acre-ft)", m_annualTotalDischarge, "");
   }


FlowModel::~FlowModel()
   {
   if ( m_pStreamQE )
      delete m_pStreamQE; 
   
   if ( m_pCatchmentQE ) 
      delete m_pCatchmentQE; 

   if ( m_pModelOutputQE )
      delete m_pModelOutputQE;

   if ( m_pModelOutputStreamQE )
      delete m_pModelOutputStreamQE;

   if ( m_pMapExprEngine  )
      delete m_pMapExprEngine;

   //if ( m_pReachDischargeData != NULL )
   //   delete m_pReachDischargeData;
  
   //if ( m_pHRUPrecipData != NULL )
   //   delete m_pHRUPrecipData;

   //if ( m_pHRUETData != NULL )
   //   delete m_pHRUETData;

   if ( m_pGlobalFlowData != NULL)
      delete m_pGlobalFlowData;

   if ( m_pTotalFluxData != NULL )
      delete m_pTotalFluxData;
   
   if ( m_pErrorStatData != NULL)
      delete m_pErrorStatData;
  
   if ( m_pParameterData != NULL)
      delete m_pParameterData;

   if ( m_pDischargeData != NULL)
      delete m_pDischargeData;

   if ( m_pClimateStationData != NULL)
      delete m_pClimateStationData;

   //if ( m_pReachRouting )  // NOTE: These are managed by GLobalMethodManager
   //   delete m_pReachRouting;
   
   //if ( m_pEvapTrans )
   //   delete m_pEvapTrans;

   //if ( m_pLateralExchange )
   //   delete m_pLateralExchange;

   //if ( m_pHruVertExchange )
   //   delete m_pHruVertExchange;

   // these are PtrArrays
   m_catchmentArray.RemoveAll();
   m_reservoirArray.RemoveAll();
   m_fluxInfoArray.RemoveAll();
   m_stateVarArray.RemoveAll();
   m_vertexArray.RemoveAll();
   m_reachHruLayerDataArray.RemoveAll();
   m_hruLayerExtraSVDataArray.RemoveAll();
   m_reservoirDataArray.RemoveAll();
   m_reachMeasuredDataArray.RemoveAll();
   m_tableArray.RemoveAll();
   m_parameterArray.RemoveAll();
   }


bool FlowModel::Init( EnvContext *pContext )
   {
   m_id = pContext->id;

   ASSERT( pContext->pMapLayer != NULL );
   m_pMap = pContext->pMapLayer->GetMapPtr();

   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;
   m_flowContext.pEnvContext = pContext;
   m_flowContext.timing = GMT_INIT;

   if ( m_catchmentLayer.IsEmpty() )
      {
      m_pCatchmentLayer = m_pMap->GetLayer( 0 ); 
      m_catchmentLayer = m_pCatchmentLayer->m_name;
      }
   else
      m_pCatchmentLayer = m_pMap->GetLayer( m_catchmentLayer );
   
   if ( m_pCatchmentLayer == NULL )
      {
      CString msg( "Flow: Unable to locate catchment layer '" );
      msg += m_catchmentLayer;
      msg += "' - this is a required layer";
      Report::ErrorMsg( msg, "Fatal Error" );
      return false;
      }

   m_pStreamLayer = m_pMap->GetLayer( m_streamLayer );
   if ( m_pStreamLayer == NULL )
      {
      CString msg( "Flow: Unable to locate stream network layer '" );
      msg += m_streamLayer;
      msg += "' - this is a required layer";
      Report::ErrorMsg( msg, "Fatal Error" );
      return false;
      }

   // <catchments>
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colCatchmentArea,        m_areaCol,          TYPE_FLOAT, CC_MUST_EXIST );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colElev,                 m_elevCol,          TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colLai,                  _T("LAI"),          TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colAgeClass,             _T("AGECLASS"),     TYPE_INT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colLulcB,                _T("LULC_B"),       TYPE_INT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colLulcA,                _T("LULC_A"),       TYPE_INT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colCatchmentJoin,        m_catchmentJoinCol, TYPE_INT, CC_MUST_EXIST );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colCatchmentCatchID,     m_catchIDCol,       TYPE_INT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colCatchmentHruID,       m_hruIDCol,         TYPE_INT, CC_AUTOADD );
   //EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruSWC,             _T("SWC"),          TYPE_FLOAT, CC_AUTOADD ); Not used
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruTemp,              _T("TEMP"),         TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruTempYr,            _T("TEMP_YR"),      TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruTemp10Yr,          _T("TEMP_10YR"),    TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruPrecip,            _T("PRECIP"),       TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruPrecipYr,          _T("PRECIP_YR"),    TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruPrecip10Yr,        _T("PRECIP_10YR"),  TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruSWE,               _T("SNOW"),         TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruMaxSWE,            _T("MAXSNOW"),      TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruApr1SWE,           _T("SNOW_APR"),     TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruApr1SWE10Yr,       _T("SNOW_APR10"),   TYPE_FLOAT, CC_AUTOADD );
//   EnvExtension::CheckCol( m_pCatchmentLayer, m_colHruDecadalSnow,     _T("SNOW_dec"),     TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colSnowIndex,            _T("SnowIndex"),    TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colAridityIndex,         _T("AridityInd"),   TYPE_FLOAT, CC_AUTOADD );  // 20 year moving avg
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colRefSnow,              _T("SnowRefere"),   TYPE_FLOAT, CC_AUTOADD );

   EnvExtension::CheckCol( m_pCatchmentLayer, m_colET_yr  ,              _T("ET_yr"),        TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colMaxET_yr,              _T("MAX_ET_yr"),    TYPE_FLOAT, CC_AUTOADD);
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colPrecip_yr  ,          _T("Precip_yr"),    TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colRunoff_yr  ,          _T("Runoff_yr"),    TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colStorage_yr  ,         _T("Storage_yr"),   TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colIrrigation_yr  ,      _T("Irrig_yr"),     TYPE_FLOAT, CC_AUTOADD );

   EnvExtension::CheckCol(m_pCatchmentLayer, m_colIrrigation, "IRRIGATION", TYPE_INT, CC_AUTOADD); 

   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHRUPercentIrrigated, "HRU_IRR_P", TYPE_FLOAT, CC_AUTOADD); 
   EnvExtension::CheckCol(m_pCatchmentLayer, m_colHRUMeanLAI, "HRU_LAI", TYPE_FLOAT, CC_AUTOADD); 

   // <streams>
   EnvExtension::CheckCol( m_pStreamLayer,    m_colStreamFrom,         _T("FNODE_"),   TYPE_INT,   CC_MUST_EXIST );  // required for building topology
   EnvExtension::CheckCol( m_pStreamLayer,    m_colStreamTo,           _T("TNODE_"),   TYPE_INT,   CC_MUST_EXIST );
   EnvExtension::CheckCol(m_pStreamLayer, m_colStreamOrder, m_orderCol, TYPE_INT, CC_AUTOADD);
   //EnvExtension::CheckCol( m_pStreamLayer,    m_colStreamReachID,       _T("COMID"),   TYPE_INT,   CC_MUST_EXIST );
   EnvExtension::CheckCol( m_pStreamLayer,    m_colStreamJoin,           m_streamJoinCol,  TYPE_INT,   CC_MUST_EXIST );
   //EnvExtension::CheckCol(m_pStreamLayer,     m_colTempMax,             _T("TEMP_MAX"), TYPE_DOUBLE, CC_AUTOADD);
   EnvExtension::CheckCol( m_pStreamLayer,    m_colStreamReachOutput,   _T("Q"),       TYPE_FLOAT, CC_AUTOADD );
   
   EnvExtension::CheckCol( m_pStreamLayer,    m_colStreamCumArea,       _T("CUM_AREA"), TYPE_FLOAT, CC_AUTOADD );
   EnvExtension::CheckCol( m_pCatchmentLayer, m_colCatchmentCumArea,    _T("CUM_AREA"), TYPE_FLOAT, CC_AUTOADD );

   m_pStreamQE    = new QueryEngine( m_pStreamLayer );
   m_pCatchmentQE = new QueryEngine( m_pCatchmentLayer );

   m_pCatchmentLayer->SetColData( m_colHruTemp,          0.0f, true );
   m_pCatchmentLayer->SetColData( m_colHruTempYr,        0.0f, true );
   m_pCatchmentLayer->SetColData( m_colHruTemp10Yr,      0.0f, true );

   m_pCatchmentLayer->SetColData( m_colHruPrecip,        0.0f, true );
   m_pCatchmentLayer->SetColData( m_colHruPrecipYr,      0.0f, true );
   m_pCatchmentLayer->SetColData( m_colHruPrecip10Yr,    0.0f, true );

   m_pCatchmentLayer->SetColData( m_colHruSWE,           0.0f, true );
   m_pCatchmentLayer->SetColData( m_colHruMaxSWE,        0.0f, true );
   m_pCatchmentLayer->SetColData( m_colHruApr1SWE,       0.0f, true );
   m_pCatchmentLayer->SetColData( m_colHruApr1SWE10Yr,   0.0f, true );

   Report::LogMsg( "Flow: Initializing reaches/catchments/reservoirs/control points" );

   // build basic structures for streams, catchments, HRU's
   InitReaches();      // create, initialize reaches based on stream layer
   InitCatchments();   // create, initialize catchments based on catchment layer
   InitReservoirs();   // initialize reservoirs
   InitReservoirControlPoints();   //initialize reservoir control points

   // connect catchments to reaches
   Report::LogMsg( "Flow: Building topology" );
   ConnectCatchmentsToReaches();

   PopulateCatchmentCumulativeAreas();

   IdentifyMeasuredReaches();

   bool ok = true; //InitializeReachSampleArray();   // allocates m_pReachDischargeData object

   if ( this->m_pStreamQuery != NULL || ( this->m_minCatchmentArea > 0 && this->m_maxCatchmentArea > 0 ) )
      { 
      EnvExtension::CheckCol( m_pCatchmentLayer, m_colCatchmentCatchID, m_catchIDCol, TYPE_INT, CC_AUTOADD );   // is this really necessary?
      SimplifyNetwork();      
      }

   SetAllCatchmentAttributes();
   //PopulateHRUArray();
   
   InitReachSubnodes();

   //Default constructor not necesarily invoked for HRULayers???
   //set ContributiontToReach and VerticalDrainage to 0
   InitHRULayers();

   // iterate through catchments/hrus/hrulayers, setting up the fluxes
   InitFluxes();

   // allocate integrators, state variable ptr arrays
   InitIntegrationBlocks();
   
   // Init() to any plugins that have an init method
   InitPlugins();

   GlobalMethodManager::Init( &m_flowContext );
   
   // add output variables:
   m_pTotalFluxData = new FDataObj(7, 0);
   m_pTotalFluxData->SetName( "Global Flux Summary");
   m_pTotalFluxData->SetLabel( 0, "Year" );
   m_pTotalFluxData->SetLabel( 1, "Annual Total ET (acre-ft)" );
   m_pTotalFluxData->SetLabel( 2, "Annual Total Precip( acre-ft)" );
   m_pTotalFluxData->SetLabel( 3, "Annual Total Discharge (acre-ft)" );
   m_pTotalFluxData->SetLabel( 4, "Delta (Precip-(ET+Discharge)) (acre-ft)" );
   m_pTotalFluxData->SetLabel( 5, "Annual Total Rainfall (acre-ft)" );
   m_pTotalFluxData->SetLabel( 6, "Annual Total Snowfall (acre-ft)" );
   
   gpFlow->AddOutputVar( "Global Flux Summary", m_pTotalFluxData, "" );

   for ( int i=0; i < GetFluxInfoCount(); i++ )
      {
      FluxInfo *pFluxInfo = GetFluxInfo( i );
      CString name( "Flux: " );
      name += pFluxInfo->m_name;
      // need to add units
      gpFlow->AddOutputVar( name, pFluxInfo->m_annualFlux, "" );
      }
   
   for ( int i=0; i < (int) m_reservoirArray.GetSize(); i++ )
      {
      Reservoir *pRes = m_reservoirArray[ i ];
      CString name( pRes->m_name );
      name += "-Filled";
      gpFlow->AddOutputVar( name, pRes->m_filled, "" );      
      }

   // set up dataobj for any model outputs
   for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

      int moCount = 0;
      for ( int i=0; i < (int) pGroup->GetSize(); i++ )
         {
         if ( pGroup->GetAt( i )->m_inUse )
            moCount++;
         if ( pGroup->GetAt( i )->m_pDataObjObs!=NULL)
            moCount++;
         }

      ASSERT( pGroup->m_pDataObj == NULL );
      if ( moCount > 0 )
         {
         pGroup->m_pDataObj = new FDataObj( moCount+1, 0 );
         pGroup->m_pDataObj->SetName( pGroup->m_name );
         pGroup->m_pDataObj->SetLabel( 0, "Time" );

         int col = 1;
         for ( int i=0; i < (int) pGroup->GetSize(); i++ )
            {
            if ( pGroup->GetAt( i )->m_inUse )
               pGroup->m_pDataObj->SetLabel( col, pGroup->GetAt( i )->m_name );
            
            if (pGroup->GetAt( i )->m_pDataObjObs!=NULL)
               {
               CString name;
               name.Format("Obs:%s", (LPCTSTR) pGroup->GetAt( i )->m_name );
               int offset = ((pGroup->m_pDataObj->GetColCount()-1)/2);//remove 1 column (time) and divide by 2 to get the offset to the measured data
               pGroup->m_pDataObj->SetLabel(col+offset, name );       //Assumes that there is measured data for each observation point in this group!
               }
            col++;
            }
         gpFlow->AddOutputVar( pGroup->m_name, pGroup->m_pDataObj, pGroup->m_name );
         }
      }

   // call any initialization methods for fluxes
   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;
   m_flowContext.pEnvContext = pContext;

   int fluxInfoCount = (int) m_fluxInfoArray.GetSize();
   for ( int i=0; i < fluxInfoCount; i++ )
      {
      FluxInfo *pFluxInfo = m_fluxInfoArray[ i ];
      if ( pFluxInfo->m_pInitFn != NULL )
         {
         m_flowContext.pFluxInfo = pFluxInfo;
         pFluxInfo->m_pInitFn( &m_flowContext, pFluxInfo->m_initInfo );
         }
      }

   float areaTotal=0.0f;
   int numHRU=0; 
   int numPoly=0;
   for ( int i=0; i < m_catchmentArray.GetSize(); i++ )
      {
      Catchment *pCatchment = m_catchmentArray[i];
      areaTotal += pCatchment->m_area;
      for (int j=0; j < pCatchment->GetHRUCount(); j++)
         {
         HRU *pHRU = pCatchment->GetHRU( j );
         for (int k=0;k < pHRU->m_polyIndexArray.GetSize(); k++)
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
   msg.Format( "Flow: Total Area %8.1f km2.  Average catchment area %8.1f km2, Average HRU area %8.1f km2, number of polygons %i", 
       areaTotal/10000.0f/100.0f, areaTotal/m_catchmentArray.GetSize()/10000.0f/100.0f, areaTotal/numHRU/10000.0f/100.0f, numPoly );
   Report::LogMsg( msg );

   int numParam = (int) m_parameterArray.GetSize();
   if (m_estimateParameters)
      msg.Format( "Flow: Parameter Estimation run. %i parameters, %i runs, %i years. Scenario settings may override. Verify by checking Scenario variables ",numParam , m_numberOfRuns, m_numberOfYears );
   else
      msg.Format( "Flow: Single Run.  %i years. Scenario settings may override. Verify by checking Scenario variables ",  m_numberOfYears );
   Report::LogMsg( msg, RT_INFO );

   msg.Format( "Flow: Processors available=%i, Max Processors specified=%i, Processors used=%i", ::omp_get_num_procs(), gpFlow->m_maxProcessors, gpFlow->m_processorsUsed );
   Report::LogMsg( msg, RT_INFO );

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
            
            if (irrigation == 1 )    // it is irrigated
               areaIrrigated += area;
            }

         pHRU->m_percentIrrigated = areaIrrigated / totalArea;
         pHRU->m_meanLAI = meanLAI / totalArea;
         if (!m_estimateParameters)
            {
            for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
               {
               gpFlow->UpdateIDU(m_flowContext.pEnvContext, pHRU->m_polyIndexArray[k], m_colHRUPercentIrrigated, pHRU->m_percentIrrigated, true);
               gpFlow->UpdateIDU(m_flowContext.pEnvContext, pHRU->m_polyIndexArray[k], m_colHRUMeanLAI, pHRU->m_meanLAI, false);
               }
            }
         }
      }
   }

bool FlowModel::InitRun( EnvContext *pEnvContext )
   {
   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;

   m_flowContext.pEnvContext = pEnvContext;
   m_flowContext.timing   = GMT_INITRUN;

   m_totalWaterInputRate  = 0;
   m_totalWaterOutputRate = 0; 
   m_totalWaterInput      = 0;
   m_totalWaterOutput     = 0; 

   SummarizeIDULULC();

  // ReadState();//read file including initial values for the state variables. This avoids model spin up issues

   InitRunFluxes();
   InitRunPlugins();
   GlobalMethodManager::InitRun( &m_flowContext );

   m_pTotalFluxData->ClearRows();

   // initial reservoir data objs
   for ( int i=0; i < (int) m_reservoirArray.GetSize(); i++ )
      {
      Reservoir *pRes = m_reservoirArray.GetAt( i );
      ASSERT( pRes != NULL );
      ASSERT( pRes->m_pResData != NULL );
      ASSERT( pRes->m_pResSimFlowCompare != NULL ); 
      ASSERT( pRes->m_pResSimRuleCompare != NULL );

      pRes->m_filled = 0;

      pRes->m_pResData->ClearRows();
      pRes->m_pResMetData->ClearRows();
      pRes->m_pResSimFlowCompare->ClearRows();
      pRes->m_pResSimRuleCompare->ClearRows();   
      }

   // if model outputs are in use, set up the data object
   for ( int i=0; i < (int) m_modelOutputGroupArray.GetSize(); i++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ i ];

      if ( pGroup->m_pDataObj != NULL )
         {
         pGroup->m_pDataObj->ClearRows();
         pGroup->m_pDataObj->SetSize( pGroup->m_pDataObj->GetColCount(), 0 );
         }
      }

   if ( m_useRecorder && !m_estimateParameters )
      {
      for ( int i=0; i < (int) m_vrIDArray.GetSize(); i++ )
         ::EnvStartVideoCapture( m_vrIDArray[ i ] );
      }

   OpenDetailedOutputFiles();

   if ( m_estimateParameters )
      {
      for (int i = 0; i < m_numberOfYears; i++)
         {
         FlowScenario *pScenario = m_scenarioArray.GetAt(m_currentScenarioIndex);
         for (int j = 0; j < (int)pScenario->m_climateInfoArray.GetSize(); j++)//different climate parameters
            {
            ClimateDataInfo *pInfo = pScenario->m_climateInfoArray[j];
            GeoSpatialDataObj *pObj = new GeoSpatialDataObj;
            pObj->InitLibraries();
            pInfo->m_pDataObjArray.Add(pObj);
            }
         }

      InitializeParameterEstimationSampleArray();
      int initYear = pEnvContext->currentYear+1;//the value of pEnvContext lags the current year by 1????  see envmodel.cpp ln 3008 and 3050
      for ( int i=0; i < m_numberOfRuns; i++ )
         {  
         UpdateMonteCarloInput(pEnvContext,i);
         pEnvContext->run=i; 
         pEnvContext->currentYear=initYear;
         //ResetStateVariables();
         for (int j=0;j<m_numberOfYears;j++)
            { 
            if ( j != 0 )
               pEnvContext->currentYear++;

            pEnvContext->yearOfRun=j;
         
            Run( pEnvContext );
            }

         UpdateMonteCarloOutput(pEnvContext,i);

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


bool FlowModel::EndRun( EnvContext *pEnvContext )
   {
   if (m_saveStateAtEndOfRun)
      SaveState();
   m_flowContext.Reset();
   m_flowContext.pFlowModel = this;
   m_flowContext.pEnvContext = pEnvContext;
   m_flowContext.timing = GMT_ENDRUN;

   EndRunPlugins();
   GlobalMethodManager::EndRun( &m_flowContext );

   CloseDetailedOutputFiles();

   if ( m_useRecorder && !m_estimateParameters )
      {
      for ( int i=0; i < (int) m_vrIDArray.GetSize(); i++  )
         ::EnvEndVideoCapture( m_vrIDArray[ i ] );   // capture main map
      }

   // output timers
   ReportTimings( "Flow: Climate Data Run Time = %.0f seconds", m_loadClimateRunTime );   
   ReportTimings( "Flow: Global Flows Run Time = %.0f seconds", m_globalFlowsRunTime );   
   ReportTimings( "Flow: External Methods Run Time = %.0f seconds", m_externalMethodsRunTime );   
   ReportTimings( "Flow: Groundwater Run Time = %.0f seconds", m_gwRunTime );   
   ReportTimings( "Flow: HRU Integration Run Time = %.0f seconds (fluxes = %.0f seconds)", m_hruIntegrationRunTime, m_hruFluxFnRunTime );   
   ReportTimings( "Flow: Reservoir Integration Run Time = %.0f seconds", m_reservoirIntegrationRunTime );   
   ReportTimings( "Flow: Reach Integration Run Time = %.0f seconds (fluxes = %0.f seconds)", m_reachIntegrationRunTime, m_reachFluxFnRunTime );   
   ReportTimings( "Flow: Total Flux Function Run Time = %.0f seconds", m_totalFluxFnRunTime );   
   ReportTimings( "Flow: Model Output Collection Run Time = %.0f seconds", m_outputDataRunTime );
   ReportTimings( "Flow: Overall Mass Balance Run Time = %.0f seconds", m_massBalanceIntegrationRunTime ); 
   return true;
   }

int FlowModel::OpenDetailedOutputFiles()
   {
   int version = 1;
   int numElements = 12;
   int numElementsStream = 1;

   if ( m_detailedOutputFlags & 1 ) // daily IDU
      OpenDetailedOutputFilesIDU( m_iduDailyFilePtrArray );

   if ( m_detailedOutputFlags & 2 ) // annual IDU
      OpenDetailedOutputFilesIDU( m_iduAnnualFilePtrArray );

   if ( m_detailedOutputFlags & 4 ) // daily Reach
      OpenDetailedOutputFilesReach( m_reachDailyFilePtrArray );

   if ( m_detailedOutputFlags & 8 ) // annual Reach
      OpenDetailedOutputFilesReach( m_reachAnnualFilePtrArray );

   return 1;
   }


int FlowModel::OpenDetailedOutputFilesIDU( CArray< FILE*, FILE* > &filePtrArray )
   {
   int hruCount = GetHRUCount();
   int hruLayerCount = GetHRULayerCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int) m_reservoirArray.GetSize();
   int polyCount = m_pCatchmentLayer->GetRowCount();
   int modelStateVarCount = hruLayerCount;
   int numYears=m_flowContext.pEnvContext->endYear-m_flowContext.pEnvContext->startYear;
   int version = 1;

   CStringArray names;     // units??
   names.Add( "lai" );                   // 0
   names.Add( "age" );                   // 1
   names.Add( "et_yr" );                 // 2
   names.Add( "MAX_ET_yr" );                // 3
   names.Add( "precip_yr" );             // 4
   names.Add( "irrig_yr" );              // 5
   names.Add( "runoff_yr" );             // 6
   names.Add( "storage_yr" );            // 7
   names.Add( "LULC_B" );                // 8
   names.Add( "LULC_A" );                // 9
   names.Add( "SWE_April1" );            // 10
   names.Add("SWE_Max");                 // 11

   int numElements = 1;
   // close any open files
   for ( int i=0; i < filePtrArray.GetSize(); i++ )
      {
      if ( filePtrArray[ i ] != NULL )
         fclose( filePtrArray[ i ] );
      }

   // clear out any existing file ptrs
   filePtrArray.RemoveAll();

   int count = 0;
   for (int i = 0; i < names.GetSize(); i++)
      {
      CString outputPath;
      outputPath.Format("%s%s%s", PathManager::GetPath(PM_OUTPUT_DIR), (PCTSTR) names[i], ".flw");
      FILE *fp = NULL;
      PCTSTR file = (PCTSTR)outputPath;
      fopen_s(&fp, file, "wb");

      filePtrArray.Add(fp);

      if ( fp != NULL )
         {
         // write headers
         TCHAR buffer[64];
         memset( buffer, 0, 64);
         strncpy_s(buffer, names[i], 63);

         fwrite( &version,       sizeof(int), 1, fp);
         fwrite( buffer,         sizeof(char), 64, fp);
         fwrite( &numYears,      sizeof(int), 1, fp);
         fwrite( &hruCount,      sizeof(int), 1, fp);
         fwrite( &hruLayerCount, sizeof(int), 1, fp);
         fwrite( &polyCount,     sizeof(int), 1, fp);
         fwrite( &reachCount,    sizeof(int), 1, fp);
         fwrite( &numElements,   sizeof(int), 1, fp);

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


int FlowModel::OpenDetailedOutputFilesReach( CArray< FILE*, FILE* > &filePtrArray )
   {
   int hruCount = GetHRUCount();
   int hruLayerCount = GetHRULayerCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int) m_reservoirArray.GetSize();
   int polyCount = m_pCatchmentLayer->GetRowCount();
   int modelStateVarCount = hruLayerCount;
   int numYears=m_flowContext.pEnvContext->endYear-m_flowContext.pEnvContext->startYear;
   int version = 1;

   CStringArray names;
   names.Add( "Q" );         // 0

   int numElements = 1;
   // close any open files
   for ( int i=0; i < filePtrArray.GetSize(); i++ )
      {
      if ( filePtrArray[ i ] != NULL )
         fclose( filePtrArray[ i ] );
      }

   // clear out any existing file ptrs
   filePtrArray.RemoveAll();

   int count = 0;
   for (int i = 0; i < names.GetSize(); i++)
      {
      CString outputPath;
      outputPath.Format("%s%s%s", PathManager::GetPath(PM_OUTPUT_DIR), (PCTSTR) names[i], ".flw");
      FILE *fp = NULL;
      PCTSTR file = (PCTSTR)outputPath;
      fopen_s(&fp, file, "wb");

      filePtrArray.Add(fp);

      if ( fp != NULL )
         {
         TCHAR buffer[64];
         memset(buffer, 0, 64);
         strncpy_s(buffer, names[i], 63);

         fwrite( &version,       sizeof(int), 1, fp);
         fwrite( buffer,         sizeof(char), 64, fp);
         fwrite( &numYears,      sizeof(int), 1, fp);
         fwrite( &hruCount,      sizeof(int), 1, fp);
         fwrite( &hruLayerCount, sizeof(int), 1, fp);
         fwrite( &polyCount,     sizeof(int), 1, fp);
         fwrite( &reachCount,    sizeof(int), 1, fp);
         fwrite( &numElements,   sizeof(int), 1, fp);

         for (int j = 0; j < reachCount; j++)
            {
            Reach *pReach = m_reachArray[j];
            if (pReach->m_reachIndex >= 0)
               fwrite(&pReach->m_reachIndex, sizeof(int), 1, fp);
            }

         count++;
         }
      }

   return count;
   }


int FlowModel::SaveDetailedOutputIDU(CArray< FILE*, FILE* > &filePtrArray )
   {
   ASSERT( filePtrArray.GetSize() == 12 );

   int hruCount = GetHRUCount();
   int hruLayerCount = GetHRULayerCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int) m_reservoirArray.GetSize();
   int modelStateVarCount = ((hruLayerCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;
        
   // iterate through IDUs
   for (int i = 0; i < m_pCatchmentLayer->GetRowCount(); i++)
      {
      float lai=0.0f; float age=0.0f; float lulcB=0; float lulcA=0;
      float snow=0.0f; float snowIndex=0.0f; float aridity=0.0f; float irrig_yr=0.0f;
      float et_yr = 0.0f; float MAX_ET_yr=0.0f; float precip_yr = 0.0f; float runoff_yr = 0.0f; float storage_yr = 0.0f;
      float max_snow = 0.0f;
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colLai,lai);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colAgeClass,age);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colET_yr,et_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colMaxET_yr, MAX_ET_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colPrecip_yr,precip_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colIrrigation_yr,irrig_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colRunoff_yr,runoff_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colStorage_yr,storage_yr);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colLulcB,lulcB);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colLulcA,lulcA);
      m_flowContext.pEnvContext->pMapLayer->GetData(i,m_colHruSWE,snow);
      m_flowContext.pEnvContext->pMapLayer->GetData(i, m_colHruMaxSWE, max_snow);

      float height = (float) ( 46.6f*(1.0-exp(-0.0144*age)) );
      
      fwrite( &lai,        sizeof(float), 1, filePtrArray[0] );
      fwrite( &age,        sizeof(float), 1, filePtrArray[1] );
      fwrite( &et_yr,      sizeof(float), 1, filePtrArray[2] );
      fwrite( &MAX_ET_yr,     sizeof(float), 1, filePtrArray[3] );
      fwrite( &precip_yr,  sizeof(float), 1, filePtrArray[4] );
      fwrite( &irrig_yr,   sizeof(float), 1, filePtrArray[5] );
      fwrite( &runoff_yr,  sizeof(float), 1, filePtrArray[6] );
      fwrite( &storage_yr, sizeof(float), 1, filePtrArray[7] );
      fwrite( &lulcB,      sizeof(float), 1, filePtrArray[8] );
      fwrite( &lulcA,      sizeof(float), 1, filePtrArray[9] );
      fwrite( &snow,       sizeof(float), 1, filePtrArray[10]);
      fwrite(&max_snow,    sizeof(float), 1, filePtrArray[11]);
      //fwrite(&snowIndex, sizeof(float), 1, m_fp_DetailedOutput); 
      //fwrite(&aridity, sizeof(float), 1, m_fp_DetailedOutput); 
      }

   return 1;
   }


//   {
//   int hruCount = GetHRUCount();
//   int hruLayerCount = GetHRULayerCount();
//   int reachCount = GetReachCount();
//   int reservoirCount = (int) m_reservoirArray.GetSize();
//   int modelStateVarCount = ((hruLayerCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;
//
//   for (int i = 0; i < hruCount; i++)
//      {
//      HRU *pHRU = m_hruArray[i];
//      for (int j = 0; j < hruLayerCount; j++)
//         {
//         HRULayer *pLayer = pHRU->GetLayer(j);
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

int FlowModel::SaveDetailedOutputReach(CArray< FILE*, FILE* > &filePtrArray )
   {
   ASSERT( filePtrArray.GetSize() == 1 );

   int reachCount = GetReachCount();

   for (int i = 0; i < reachCount; i++)
      {
      Reach *pReach = m_reachArray[i];
      ReachSubnode *pNode = pReach->GetReachSubnode(0);

      if ( filePtrArray[ 0 ] != NULL )       
         fwrite(&pNode->m_discharge, sizeof(float), 1, filePtrArray[0]);
      }

   return 1;
   }


void FlowModel::CloseDetailedOutputFiles()
   {
   for (int i=0; i < (int) m_iduAnnualFilePtrArray.GetSize(); i++)
      {
      if (m_iduAnnualFilePtrArray[i] != NULL )
         {
         fclose(m_iduAnnualFilePtrArray[i]);
         m_iduAnnualFilePtrArray[ i ] = NULL;
         }
      }

   for (int i=0; i < (int) m_iduDailyFilePtrArray.GetSize(); i++)
      {
      if (m_iduDailyFilePtrArray[i] != NULL )
         {
         fclose(m_iduDailyFilePtrArray[i]);
         m_iduDailyFilePtrArray[ i ] = NULL;
         }
      }
         

   for (int i=0; i < (int) m_reachAnnualFilePtrArray.GetSize(); i++)
      {
      if (m_reachAnnualFilePtrArray[i] != NULL )
         {
         fclose(m_reachAnnualFilePtrArray[i]);
         m_reachAnnualFilePtrArray[ i ] = NULL;
         }
      }

   for (int i=0; i < (int) m_reachDailyFilePtrArray.GetSize(); i++)
      {
      if (m_reachDailyFilePtrArray[i] != NULL )
         {
         fclose(m_reachDailyFilePtrArray[i]);
         m_reachDailyFilePtrArray[ i ] = NULL;
         }
      }
   }


int FlowModel::SaveState()
   {
   int hruCount = GetHRUCount();
   int hruLayerCount = GetHRULayerCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int) m_reservoirArray.GetSize();
   int modelStateVarCount = ((hruLayerCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;

   // does file exist?
   CString tmpPath;

   tmpPath = PathManager::GetPath(PM_IDU_DIR);//directory with the idu

   FILE *fp;
   //CString filename;
  // filename.Format( tmpPath, m_initConditionsFileName);
   const char *file = tmpPath+ m_initConditionsFileName;
   errno_t err;
   if ((err = fopen_s(&fp, file, "wb")) != 0) {
  // fp = _fsopen(filename, "wb", _SH_DENYNO);
  /* fp = fopen(file, "wb");
   if (fp == NULL)
      {*/
      CString msg;
      msg.Format("Flow: Unable to open file %s.  Initial conditions won't be saved.  ", (LPCTSTR)tmpPath + m_initConditionsFileName);
      Report::LogMsg(msg, RT_WARNING);
    //  fclose(fp);
      return -1;
      }

   fwrite(&modelStateVarCount, sizeof(int), 1, fp);
   //fprintf(fp, "%i ", modelStateVarCount);//m3
   for (int i = 0; i < hruCount; i++)
      {
      HRU *pHRU = m_hruArray[i];
      for (int j = 0; j < hruLayerCount; j++)
         {
         HRULayer *pLayer = pHRU->GetLayer(j);
         if (pLayer->m_volumeWater >= -10.0f)
            fwrite(&pLayer->m_volumeWater, sizeof(double), 1, fp);
         else
            {
            CString msg;
            msg.Format("Flow: Bad value (%f) in HRU %i layer %i.  Initial conditions won't be saved.  ",pLayer->m_volumeWater, i,j);
            Report::LogMsg(msg, RT_WARNING);
            return -1;
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
            Report::LogMsg(msg, RT_WARNING);
            return -1;
            }
         for (int l = 0; l < m_hruSvCount - 1; l++)
            fwrite(&pNode->m_svArray[l], sizeof(double), 1, fp);
            //fprintf(fp, "%f ", pNode->m_svArray[l]);//kg
         }
      }
   CString msg1;
   msg1.Format("Flow: Initial Conditions file %s successfully written.  ", (LPCTSTR)m_initConditionsFileName);
   Report::LogMsg(msg1, RT_WARNING);
   //for (int i = 0; i < reservoirCount; i++)
   //   {
   //   Reservoir *pRes = m_reservoirArray[i];
   //   fwrite(&pRes->m_volume, sizeof(double), 1, fp);
   //   //fprintf(fp, "%f ", pRes->m_volume);
   //   }
   fclose(fp);
   m_saveStateAtEndOfRun=false;
   return 1;
   }

int FlowModel::ReadState()
   {
   int hruCount = GetHRUCount();
   int hruLayerCount = GetHRULayerCount();
   int reachCount = GetReachCount();
   int reservoirCount = (int) m_reservoirArray.GetSize();

   int modelStateVarCount = ((hruLayerCount*hruCount) + reachCount)*m_hruSvCount + reservoirCount;
   // does file exist?
   CString tmpPath = PathManager::GetPath(PM_IDU_DIR);//directory with the idu
   int fileStateVarCount = 0;

   if (PathManager::FindPath(m_initConditionsFileName, tmpPath) < 0) //the file cannot be found, so skip the read and set flag to write file at the end of the run
      {
      CString msg;
      msg.Format("Flow: Initial Conditions file '%s' can not be found.  It will be created at the end of this run", (LPCTSTR) m_initConditionsFileName);
      Report::LogMsg(msg, RT_WARNING);
      m_saveStateAtEndOfRun = true;
      return 1;
      }
   else //the file can be found. First check that it has the same # of state variables as the current model.  If not, the model changed, so don't read the file now
      {
      FILE *fp;
      CString filename;
      filename.Format(tmpPath,m_initConditionsFileName);
      const char *file = filename;
      errno_t err;
      if ((err = fopen_s(&fp, file, "rb")) != 0) {
     /* fp=fopen(file, "rb");
      if (fp == NULL)
         {*/
         CString msg;
         msg.Format("Unable to open file %s.  Initial conditions won't be saved.  ", filename);
         Report::LogMsg(msg, RT_WARNING);
         fclose(fp);
         return -1;
         }

     // fscanf(fp, "%i ", &fileStateVarCount);
      fread(&fileStateVarCount, sizeof(int), 1, fp);

      if (fileStateVarCount != modelStateVarCount)//the file is out of date, so skip the read and set flage to write file at the end of the run
         {
         CString msg;
         msg.Format("The current model is different than that saved in %s. %i versus %i state variables.  %s will be overwritten at the end of this run", (LPCTSTR) m_initConditionsFileName, modelStateVarCount, fileStateVarCount, (LPCTSTR) m_initConditionsFileName);
         Report::LogMsg(msg, RT_WARNING);
         m_saveStateAtEndOfRun = true;

         }
      else //read the file to populate initial conditions
         {
         CString msg1;
         msg1.Format("Flow: Initial Conditions file %s successfully read.  ", (LPCTSTR)m_initConditionsFileName);
         Report::LogMsg(msg1, RT_WARNING);
         float _val = 0;
         for (int i = 0; i < hruCount; i++)
            {
            HRU *pHRU = m_hruArray[i];
            for (int j = 0; j < hruLayerCount; j++)
               {
               HRULayer *pLayer = pHRU->GetLayer(j);
               //fscanf(fp, "%f ", &pLayer->m_volumeWater);
               fread(&pLayer->m_volumeWater, sizeof(double), 1, fp);
               for (int k = 0; k < m_hruSvCount - 1; k++)
                  fread(&pLayer->m_svArray[k], sizeof(double), 1, fp);
                  //fscanf(fp, "%f ", &pLayer->m_svArray[k]);
               }
            }
         for (int i = 0; i < reachCount; i++)
            {
            Reach *pReach = m_reachArray[i];
            for (int j = 0; j < pReach->m_subnodeArray.GetSize(); j++)
               {
               ReachSubnode *pNode = pReach->GetReachSubnode(j);
               fread(&pNode->m_discharge, sizeof(float), 1, fp);
               //fscanf(fp, "%f ", &pNode->m_discharge);
               for (int k = 0; k < m_hruSvCount - 1; k++)
                  fread(&pNode->m_svArray[k], sizeof(double), 1, fp);
                  //fscanf(fp, "%f ", &pNode->m_svArray[k]);
               }
            }
         //for (int i = 0; i < reservoirCount; i++)
         //   {
         //   Reservoir *pRes = m_reservoirArray[i];
         //   fread(&pRes->m_volume, sizeof(double), 1, fp);
         //   //fscanf(fp, "%f ", &pRes->m_volume);
         //   }
         fclose(fp);
         }//end of else read the file to populate initial conditions
      }//else file can be found
   return 1;
   }


void FlowModel::ResetDataStorageArrays(EnvContext *pEnvContext)
   {
   // if model outputs are in use, set up the data object
   for ( int i=0; i < (int) m_modelOutputGroupArray.GetSize(); i++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ i ];

      if ( pGroup->m_pDataObj != NULL )
         {
         pGroup->m_pDataObj->ClearRows();
       //  pGroup->m_pDataObj->Clear();
         pGroup->m_pDataObj->SetSize( pGroup->m_pDataObj->GetColCount(), 0 );         
         }
      }
   //Insert code here for m_reservoirDataArray;
   }


// FlowModel::Run() runs the model for one year
bool FlowModel::Run( EnvContext *pEnvContext )
   {
   CString msgStart;
   msgStart.Format("Flow: Starting year %i", pEnvContext->yearOfRun);
   Report::LogMsg(msgStart);
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

	if (pEnvContext->yearOfRun==0 /*&& !m_estimateParameters*/)
      ResetStateVariables();

   m_projectionWKT = pEnvContext->pMapLayer->m_projection; //the Well Known Text for the maplayer projection
   CString msg;

   clock_t start = clock();
   
   if ( !m_useStationData )
      {
      LoadClimateData(pEnvContext, pEnvContext->currentYear );
      }
 
   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
   m_loadClimateRunTime += (float) duration;   

   StartYear( &m_flowContext );     // calls GLobalMethods::StartYear(), initialize annual variable, accumulators

   // convert envision time to days
   m_currentTime = m_flowContext.time = pEnvContext->currentYear*365.0f;
   m_flowContext.svCount = m_hruSvCount-1;  //???? 
   m_timeInRun = pEnvContext->yearOfRun * 365.0f;   // days

   m_stopTime = m_currentTime + 365;      // always run for 1 year total to sync with Envision
   float stepSize = m_flowContext.timeStep = (float) m_timeStep;
   
   m_flowContext.Reset();
      
   m_year = pEnvContext->currentYear;

   ResetCumulativeValues( pEnvContext );

   omp_set_num_threads(gpFlow->m_processorsUsed); 

   m_volumeMaxSWE = 0.0f;
   m_dateMaxSWE= 0;

   //-------------------------------------------------------
   // Main within-year FLOW simulation loop starts here
   //-------------------------------------------------------
   while ( ( m_currentTime + TIME_TOLERANCE ) < m_stopTime )
      {
      int dayOfYear = int( fmod( m_timeInRun, 365 ) );  // zero based day of year
      int _year = 0;
      BOOL ok = ::GetCalDate( dayOfYear+1, &_year, &m_month, &m_day, TRUE );
      
      m_flowContext.dayOfYear = dayOfYear;
      m_flowContext.timing = GMT_START_STEP;

      double timeOfDay = m_currentTime-(int)m_currentTime;

      if (!m_estimateParameters)
         msg.Format( "Flow: Time: %6.1f - Simulating HRUs...", m_timeInRun );
      else
         msg.Format( "Flow: Run %i of %i: Time: %6.1f of %i days ", pEnvContext->run+1, m_numberOfRuns, m_timeInRun, m_numberOfYears*365 );
      
      Report::StatusMsg( msg );
      
      msg.Format( "%s %i, %i", ::GetMonthStr( m_month ), m_day, this->GetCurrentYear()  );
      ::EnvSetLLMapText( msg );

      // reset accumlator rates
      m_totalWaterInputRate  = 0;
      m_totalWaterOutputRate = 0; 
      for ( int i=0; i < GetFluxInfoCount(); i++ )
         GetFluxInfo( i )->m_totalFluxRate = 0;

      if ( m_pGlobalFlowData )
         {
         start = clock();
         
         SetGlobalFlows();
         
         finish = clock();
         duration = (float)(finish - start) / CLOCKS_PER_SEC;   
         m_globalFlowsRunTime += (float) duration;   
         }

      // any pre-run external methods?
      start = clock();
         
      GlobalMethodManager::StartStep( &m_flowContext );  // Calls any global methods with ( m_timing & GMT_START_STEP ) == true
     
      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;   
      m_externalMethodsRunTime += (float) duration;   
           
      start = clock();
      
      //if ( m_gwMethod != GW_EXTERNAL )
      //   ;  //m_reachBlock.Integrate( m_currentTime, m_currentTime+stepSize, GetReachDerivatives, &m_flowContext );  // NOTE: GetReachDerivatives does not work
      //else
      //   SolveGWDirect();
      
      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;   
      m_gwRunTime += (float) duration;  

      SetGlobalReservoirFluxes();         // establish inflows and outflows for all Reservoirs(NOTE: MAY NEED TO MOVE THIS TO GetCatchmentDerivatives()!!!!

      start = clock();
      
      // Note: GetCatchmentDerivates() calls GlobalMethod::Run()'s for any method with 
      // (m_timing&GMT_CATCHMENT) != 0 
      m_flowContext.timing = GMT_CATCHMENT;
      m_hruBlock.Integrate( (double)m_currentTime, (double)m_currentTime+stepSize, GetCatchmentDerivatives, &m_flowContext );  // update HRU swc and associated state variables
      
      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;   
      
      m_hruIntegrationRunTime += (float) duration;   
      
      m_flowContext.Reset();

      // reservoirs
      start = clock();
      
      // Note: GetReservoirDerivatives() calls GlobalMethod::Run()'s for any method with 
      // (m_timing&GMT_RESERVOIR) != 0 
      m_flowContext.timing = GMT_RESERVOIR;
      m_reservoirBlock.Integrate( m_currentTime, m_currentTime+stepSize, GetReservoirDerivatives, &m_flowContext );     // update stream reaches
      
      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;   
      m_reservoirIntegrationRunTime += (float) duration;   
      
      // check reservoirs for filling
      for ( int i=0; i < (int) m_reservoirArray.GetSize(); i++ )
         {
         Reservoir *pRes = m_reservoirArray[ i ];

         //CString msg;
         //msg.Format( "Reservoir check: Filled=%i, elevation=%f, dam top=%f", pRes->m_filled, pRes->m_elevation, pRes->m_dam_top_elev );
         //Report::LogMsg( msg );
         if ( pRes->m_filled == 0 && pRes->m_elevation >= ( pRes->m_dam_top_elev-2.0f ) )
            {
            //Report::LogMsg( "Reservoir filled!!!");
            pRes->m_filled = 1;   
            }
         }

      m_flowContext.Reset();
      
      start = clock();

      m_flowContext.timing = GMT_REACH;
      GlobalMethodManager::SetTimeStep(stepSize );
      GlobalMethodManager::Step( &m_flowContext );
      //if ( m_pReachRouting )
      //   {
      //   m_flowContext.timing = GMT_REACH;
      //   m_pReachRouting->m_reachTimeStep = stepSize;
      //   m_pReachRouting->Run( &m_flowContext );
      //   }


      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;   
      m_reachIntegrationRunTime += (float) duration;   
      
      // total refers to mass balance error ceheck
     // msg.Format( "Flow: Time: %6.1f - Computing Overall Mass Balances...", m_timeInRun );
     // Report::StatusMsg( msg );
      if ( m_checkMassBalance )
         {
         start = clock();
    
         m_totalBlock.Integrate( m_currentTime, m_currentTime+stepSize, GetTotalDerivatives, &m_flowContext );

         finish = clock();
         duration = (float)(finish - start) / CLOCKS_PER_SEC;   
         m_massBalanceIntegrationRunTime += (float) duration;   
         }

      start = clock();

      if ( m_detailedOutputFlags & 1 )  // daily IDU
         SaveDetailedOutputIDU( m_iduDailyFilePtrArray );

      if ( m_detailedOutputFlags & 4)  // daily Reach
         SaveDetailedOutputReach( m_reachDailyFilePtrArray );

      ResetCumulativeValues( pEnvContext );
      CollectData( dayOfYear );

      finish = clock();
      duration = (float)(finish - start) / CLOCKS_PER_SEC;   
      m_collectDataRunTime += (float) duration;   

      if (!m_estimateParameters)
         {
         if (dayOfYear<180)//get maximum winter snowpack for this year.  
            GetMaxSnowPack(pEnvContext);
         }

      EndStep( &m_flowContext );
      UpdateHRULevelVariables(pEnvContext);
      
      CollectModelOutput();
     
      // any post-run external methods?
      m_flowContext.timing = GMT_END_STEP;
      GlobalMethodManager::EndStep( &m_flowContext );   // this invokes EndStep() for any internal global methods 

      // update time...
      m_currentTime += stepSize;
      m_timeInRun += stepSize;

      // are we close to the end?
      double timeRemaining =  m_stopTime - m_currentTime;

      if ( timeRemaining  < ( m_timeStep + TIME_TOLERANCE ) )
         stepSize = float( m_stopTime - m_currentTime );
      
      if ( m_mapUpdate == 2 && !m_estimateParameters)
         WriteDataToMap(pEnvContext);

      if ( m_mapUpdate == 2 && !m_estimateParameters)
        RedrawMap( pEnvContext );

      if ( m_useRecorder && !m_estimateParameters )
         {
         for ( int i=0; i < (int) m_vrIDArray.GetSize(); i++  )
            ::EnvCaptureVideo( m_vrIDArray[ i ] );
         }
      
      if (dayOfYear == 90 && !m_estimateParameters)
         UpdateAprilDeltas(pEnvContext);

      if (dayOfYear == 274)
         ResetCumulativeWaterYearValues();         
      }  // end of: while ( m_time < m_stopTime )   // note - doesn't guarantee alignment with stop time

   // Done with the internal flow timestep loop, finish up...
   EndYear( &m_flowContext );
   
   if ( m_mapUpdate == 1 && !m_estimateParameters)
      {
      WriteDataToMap( pEnvContext );
      RedrawMap(pEnvContext);   
      }

   CloseClimateData();

   if (!m_estimateParameters)
      {
      UpdateYearlyDeltas(pEnvContext);
      ResetCumulativeYearlyValues();
      }

   if ( m_detailedOutputFlags & 2 )
      SaveDetailedOutputIDU( m_iduAnnualFilePtrArray );

   if ( m_detailedOutputFlags & 8 )
      SaveDetailedOutputReach( m_reachAnnualFilePtrArray );

    if (!m_estimateParameters)
      {
      for ( int i=0; i < m_reservoirArray.GetSize(); i++ )
         {
         Reservoir *pRes = m_reservoirArray[ i ];
         CString outputPath;
         outputPath.Format("%s%s%s", PathManager::GetPath(PM_OUTPUT_DIR), pRes->m_name, "AppliedConstraints.csv");
         pRes->m_pResSimRuleCompare->WriteAscii(outputPath);         
         }

      float channel=0.0f;
      float terrestrial=0.0f;
      GetTotalStorage(channel, terrestrial);
      msg.Format( "Flow: TotalInput: %6.1f 10^6m3. TotalOutput: %6.1f 10^6m3. Terrestrial Storage: %6.1f 10^6m3. Channel Storage: %6.1f 10^6m3  Difference: %6.1f m3", float( m_totalWaterInput/1E6 ), float( m_totalWaterOutput/1E6 ), float( terrestrial/1E6 ), float( channel/1E6), float(m_totalWaterInput-m_totalWaterOutput-(channel+terrestrial)) );
      Report::LogMsg( msg );
      }

   return TRUE;
   }


bool FlowModel::StartYear( FlowContext *pFlowContext )
   {
   m_annualTotalET        = 0; // acre-ft
   m_annualTotalPrecip    = 0; // acre-ft
   m_annualTotalDischarge = 0; //acre-ft
   m_annualTotalRainfall  = 0; // acre-ft
   m_annualTotalSnowfall  = 0; // acre-ft

   GlobalMethodManager::StartYear( &m_flowContext );   // this invokes StartYear() for any internal global methods 
   
   // reset flux accumulators for this year
   for ( int i=0; i < GetFluxInfoCount(); i++ )
      {
      GetFluxInfo( i )->m_totalFluxRate = 0;
      GetFluxInfo( i )->m_annualFlux = 0;
      }

   // reset reservoirs for this year
   for ( int i=0; i < (int) m_reservoirArray.GetSize(); i++ )
      {
      Reservoir *pRes = m_reservoirArray[ i ];
      pRes->m_filled = 0;   
      }


   const char* name = "IrrigatedSoil";
   if (m_hruLayerNames.GetSize()>3)
   {
      const char* label = m_hruLayerNames.GetAt(3).GetString();
      if (label[0]== name[0])
         {
         // fix up water transfers between irrigated and unirrigated polygons
         int hruCount = pFlowContext->pFlowModel->GetHRUCount();
         for (int h = 0; h < hruCount; h++)
            {
            HRU *pHRU = pFlowContext->pFlowModel->GetHRU(h);
            float hruIrrigatedArea=0.0f;
            for (int m = 0; m < pHRU->m_polyIndexArray.GetSize(); m++)
               {
               float area = 0.0f;
               float irrigated = 0.0f;
               pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[m], pFlowContext->pFlowModel->m_colIrrigation, irrigated);
               pFlowContext->pEnvContext->pMapLayer->GetData(pHRU->m_polyIndexArray[m], pFlowContext->pFlowModel->m_colCatchmentArea, area);
               if (irrigated != 0)
                  hruIrrigatedArea += area;
               }
         
            float irrAreaDelta = hruIrrigatedArea - pHRU->m_areaIrrigated ;//if this number is +, then we gained that much irrigated area
            float irrDeltaProportion = irrAreaDelta / (pHRU->m_areaIrrigated);//proportion of the irrigated landscape that was added to nonirrigated area
            float nonIrrDeltaProportion = 0.0f;
            if (pHRU->m_area - pHRU->m_areaIrrigated >0.0)
               nonIrrDeltaProportion = irrAreaDelta / (pHRU->m_area - pHRU->m_areaIrrigated);//proportion of the non-irrigated landscape that was added to irrigated area

            float volNonIrrigated = pHRU->GetLayer(2)->m_volumeWater;
            float volIrrigated = pHRU->GetLayer(3)->m_volumeWater;

            if (pFlowContext->pEnvContext->yearOfRun > 0)//don't move water around before the first year finishes- at the start of the first year, just set pHRU->m_areaIrrigated so that after this year, the comparison is valid
               {
               if (irrAreaDelta > 0.0f)//gained irrigated area, so move water from non-irrigated box into irrigated box.  Will result in dryer irrigated wc.  non-irrigated should be the same
                  {
                  pHRU->GetLayer(3)->m_volumeWater += volNonIrrigated*nonIrrDeltaProportion;
                  pHRU->GetLayer(2)->m_volumeWater -= volNonIrrigated*nonIrrDeltaProportion;
                  }
               if (irrAreaDelta < 0.0f) //move water from irrigated to non-irrigated
                  {
                  pHRU->GetLayer(3)->m_volumeWater += volIrrigated*irrDeltaProportion;//irrDeltaProportion is negative, so this subtracts from the irrigated volume.  wetter non-irrigated, but irrigated wc should stay the same
                  pHRU->GetLayer(2)->m_volumeWater -= volIrrigated*irrDeltaProportion;
                  }
               if ((pHRU->m_area - hruIrrigatedArea)>0.0f)
                  pHRU->GetLayer(2)->m_wc = float(pHRU->GetLayer(2)->m_volumeWater / (pHRU->m_area - hruIrrigatedArea)*1000.0f);
               if (hruIrrigatedArea > 0.0f)
                  pHRU->GetLayer(3)->m_wc = float(pHRU->GetLayer(3)->m_volumeWater / (hruIrrigatedArea)*1000.0f);
               }
            pHRU->m_areaIrrigated = hruIrrigatedArea;
            }
         }
      }

   return true;
   }


bool FlowModel::EndStep( FlowContext *pFlowContext )
   {
   // get discharge at pour points
   for ( int i=0; i < this->m_reachTree.GetRootCount(); i++ )
      {
      ReachNode *pRootNode = this->m_reachTree.GetRootNode( i );
      Reach *pReach = GetReachFromNode( pRootNode );
      float discharge = pReach->GetDischarge();   // m3/sec

      // convert to acreft/day
      discharge *= ACREFT_PER_M3 * SEC_PER_DAY;  // convert to acreft-day
      m_annualTotalDischarge += discharge;
      }

   return true;
   }


bool FlowModel::EndYear( FlowContext *pFlowContext )
   {
   for ( int i=0; i < (int) m_hruArray.GetSize(); i++ )
      {
      HRU *pHRU = m_hruArray[ i ];
      m_annualTotalET       += pHRU->m_et_yr       * pHRU->m_area * M_PER_MM * ACREFT_PER_M3;        // mm/year * area(m2) * m/mm * acreft/m3   = acre-ft
      m_annualTotalPrecip   += pHRU->m_precip_yr   * pHRU->m_area * M_PER_MM * ACREFT_PER_M3;        // mm/year * area(m2) * m/mm * acreft/m3   = acre-ft
      m_annualTotalRainfall += pHRU->m_rainfall_yr * pHRU->m_area * M_PER_MM * ACREFT_PER_M3;        // mm/year * area(m2) * m/mm * acreft/m3   = acre-ft
      m_annualTotalSnowfall += pHRU->m_rainfall_yr * pHRU->m_area * M_PER_MM * ACREFT_PER_M3;        // mm/year * area(m2) * m/mm * acreft/m3   = acre-ft
      }

   m_flowContext.timing = GMT_END_YEAR;
   GlobalMethodManager::EndYear( &m_flowContext );

   float row[ 7 ];
   row[ 0 ] = (float) pFlowContext->pEnvContext->currentYear;
   row[ 1 ] = m_annualTotalET;
   row[ 2 ] = m_annualTotalPrecip;
   row[ 3 ] = m_annualTotalDischarge;
   row[ 4 ] = m_annualTotalPrecip - ( m_annualTotalET + m_annualTotalDischarge );
   row[ 5 ] = m_annualTotalRainfall;
   row[ 6 ] = m_annualTotalSnowfall;

   m_pTotalFluxData->AppendRow( row, 7 );
   
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
   int numMeas=0;
   for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

      for ( int i=0; i < (int) pGroup->GetSize(); i++ )
         {
         if (pGroup->GetAt( i )->m_inUse)
            {
            ModelOutput *pOutput = pGroup->GetAt( i );
            if (pOutput->m_pDataObjObs!=NULL && pOutput->m_queryStr)
               numMeas++;
            }
         }
      }
   //Get the objective function for each of the locations with measured values
   float *ns_=new float[numMeas];
   int c=0;
   float ns = -1.0f; float nsLN = -1.0f; float ve = 0.0f;
   for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

      for ( int i=0; i < (int) pGroup->GetSize(); i++ )
         {
         if (pGroup->GetAt( i )->m_inUse)
            {
            ModelOutput *pOutput = pGroup->GetAt( i );
            if (pOutput->m_pDataObjObs!=NULL && pGroup->m_pDataObj!=NULL)
               {
               ns_[c] = GetObjectiveFunction(pGroup->m_pDataObj, ns, nsLN, ve);//this will include time meas model
               c++;
               }
            }
         }
      }
   //Get the highest value for the objective function
   float maxNS=-999999.0f;
   for (int i=0;i<numMeas;i++)
      if (ns_[i]>maxNS)
         maxNS=ns_[i];
   delete [] ns_;
   //Add objective functions and time series data to the dataobjects
   int count=0;
   for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

      for ( int i=0; i < (int) pGroup->GetSize(); i++ )
         {
         if (pGroup->GetAt( i )->m_inUse)
            {
            ModelOutput *pOutput = pGroup->GetAt( i );
            if (pOutput->m_pDataObjObs!=NULL && pGroup->m_pDataObj!=NULL)
               {             
               FDataObj *pObj = m_mcOutputTables.GetAt(count);//objective
               count++;
               FDataObj *pData = m_mcOutputTables.GetAt(count);//timeseries
               count++;

               if (pEnvContext->run==0)//add the measured data to the mc measured file
                  {
                  pData->SetSize(2,pGroup->m_pDataObj->GetRowCount());
                  for (int i=0;i<pData->GetRowCount();i++)
                     {
                     pData->Set(0,i,i);//time
                     pData->Set(1,i,pGroup->m_pDataObj->Get(2,i));//measured value
                     }
                  }
                if (maxNS > m_nsThreshold)
                     {
                     pData->AppendCol("q");
                     for (int i=0;i<pData->GetRowCount();i++)
                        pData->Set(pData->GetColCount()-1,i, pGroup->m_pDataObj->Get(1,i));
                     float ns = -1.0f; float nsLN = -1.0f; float ve = -1.0f;
                     GetObjectiveFunction(pGroup->m_pDataObj, ns, nsLN, ve);//this will include time meas model
                     float *nsArray = new float[4];
                     nsArray[0] = (float) pEnvContext->run;
                     nsArray[1] = ns;
                     nsArray[2] = nsLN;
                     nsArray[3] = ve;
                     pObj->AppendRow( nsArray, 4 );
                     delete [] nsArray;
                    }
                }
            }
         }
      }
   //if the simulation wasn't behavioral for any of the measurement points, then remove that parameter set from the parameter array
   if (maxNS < m_nsThreshold)
      m_pParameterData->DeleteRow(m_pParameterData->GetRowCount()-1);
   CString path = PathManager::GetPath(3);
   // write the results to the disk
   if (fmod((double)pEnvContext->run,m_saveResultsEvery) < 0.01f)
      {  
      count=0;
      for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
         {
         ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

         for ( int i=0; i < (int) pGroup->GetSize(); i++ )
            {
            if (pGroup->GetAt( i )->m_inUse)
               {
               ModelOutput *pOutput = pGroup->GetAt( i );
               if (pOutput->m_pDataObjObs!=NULL && pGroup->m_pDataObj!=NULL)
                  {   
                  CString fileOut;
              
                  if ( m_rnSeed >= 0 )
                     fileOut.Format("%s\\output\\%sObj_%i.csv", (LPCTSTR) path,pGroup->m_name, (int) m_rnSeed );
                  else
                     fileOut.Format("%s\\output\\%sObj.csv", (LPCTSTR)path, (LPCTSTR)pGroup->m_name);
                  m_mcOutputTables.GetAt(count)->WriteAscii(fileOut);

                  count++;

                  if ( m_rnSeed >= 0 )
                    fileOut.Format("%s\\output\\%sObs_%i.csv", (LPCTSTR)path, (LPCTSTR)pGroup->m_name, (int)m_rnSeed);
                  else
                    fileOut.Format("%s\\output\\%sObs.csv", (LPCTSTR)path, (LPCTSTR)pGroup->m_name);
                  
                  m_mcOutputTables.GetAt(count)->WriteAscii(fileOut); 
                  count++;

                 if (m_rnSeed >= 0)
                    fileOut.Format("%s\\output\\%sparam_%i.csv", (LPCTSTR)path, pGroup->m_name, (int)m_rnSeed);
                 else
                    fileOut.Format("%s\\output\\%sparam.csv", (LPCTSTR)path, pGroup->m_name);

                 m_pParameterData->WriteAscii(fileOut);

                 }
               if (pEnvContext->run == 0)//for the first run, look for a climate data object and write it to a file.  
                  {
                  CString fileOut; CString name;
                  if (pGroup->m_pDataObj != NULL && pGroup->m_name == "Climate")
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
                     fileOut.Format("%s\\output\\%sClimate.csv", (LPCTSTR)path, name);
                     pGroup->m_pDataObj->WriteAscii(fileOut);
                     }
                  }
              }
           }
        }
     }

   CString msg;
   msg.Format( "Flow: Results for Runs 1 to %i written to disk.  Check directory %soutput", pEnvContext->run+1, (LPCTSTR) path );
   Report::LogMsg( msg );
   }


void FlowModel::UpdateMonteCarloInput(EnvContext *pEnvContext,int runNumber)
   { 
   //update storage arrays (parameters, error  and also (perhaps...)modeled discharge)
   int paramCount =  (int) m_parameterArray.GetSize();
   float *paramValueData = new float[ paramCount+1 ];

   paramValueData[0] = (float) runNumber;

   for (int i=0; i < paramCount; i++)
      {
      ParameterValue *pParam = m_parameterArray.GetAt(i);

      float value= (float) m_randUnif1.RandValue(pParam->m_minValue,pParam->m_maxValue);
      
      //we have a value, and need to update the table with it.
      ParamTable *pTable = GetTable( pParam->m_table );  
      int col_name = pTable->GetFieldCol( pParam->m_name ); 
      float oldValue=0.0f;

      FDataObj *pTableData = (FDataObj*)pTable->GetDataObj();
      FDataObj *pInitialTableData = (FDataObj*)pTable->GetInitialDataObj();
      for (int j=0; j < pTableData->GetRowCount(); j++)
         {
         float oldValue = pInitialTableData->Get(col_name,j);
         float newValue = oldValue*value;
         if (pParam->m_distribute)
            pTableData->Set(col_name,j,newValue);   
         else
            pTableData->Set(col_name,j,value);  
         }

      paramValueData[i+1]=value;
      }

   m_pParameterData->AppendRow(paramValueData, int( m_parameterArray.GetSize())+1);
   delete [] paramValueData;
    
   }

bool FlowModel::InitHRULayers()
   {
   int catchmentCount = (int) m_catchmentArray.GetSize();
   int hruLayerCount = GetHRULayerCount();
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = m_catchmentArray[ i ];
      ASSERT( pCatchment != NULL );

      int hruCount = pCatchment->GetHRUCount();
      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         ASSERT( pHRU != NULL );

         for ( int l=0; l < hruLayerCount; l++ )
            {
            HRULayer *pHRULayer = pHRU->GetLayer( l );
            pHRULayer->m_contributionToReach = 0.0f;
            pHRULayer->m_verticalDrainage = 0.0f;
            pHRULayer->m_horizontalExchange = 0.0f;
            pHRULayer->m_volumeWater = 0.0f;
            if (gpModel->m_initWaterContent.GetSize()==hruLayerCount)//water content for each layer was specified
               pHRULayer->m_volumeWater = atof(gpModel->m_initWaterContent[l])*pHRULayer->m_depth*0.4f;
            else if (gpModel->m_initWaterContent.GetSize()==1) // a single value for water content was specified 
               pHRULayer->m_volumeWater = atof(gpModel->m_initWaterContent[0])*pHRULayer->m_depth*0.4f;
            for (int k=0; k < m_hruSvCount-1; k++)   
               {
               pHRULayer->m_svArray[ k ] = 0.0f;//concentration = 1 kg/m3 ???????
               pHRULayer->m_svArrayTrans[ k ] = 0.0f;//concentration = 1 kg/m3
               }
            }
         }
      }

   return true;
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


bool FlowModel::SolveGWDirect( void )
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
   ASSERT( 0 );
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



bool FlowModel::SetGlobalExtraSVRxn( void )
   {
   switch( m_extraSvRxnMethod )
      {
      case EXSV_EXTERNAL:
         {
         if ( m_extraSvRxnExtFn != NULL )
            m_extraSvRxnExtFn( &m_flowContext );
         return true;
         }

      case EXSV_INFLUXHANDLER:
         return 0.0f;
      }

   // anything else indicates a problem
   ASSERT( 0 );
   return false;
   }


bool FlowModel::SetGlobalReservoirFluxes( void  )
   {
   switch( m_reservoirFluxMethod )
      {
      case RES_EXTERNAL:
         {
         if ( m_reservoirFluxExtFn != NULL )
            m_reservoirFluxExtFn( &m_flowContext );
         return true;
         }

      case RES_RESSIMLITE:
         return SetGlobalReservoirFluxesResSimLite();
      }

   // anything else indicates a problem
   ASSERT( 0 );
   return false;
   }


bool FlowModel::InitReservoirControlPoints(void )
   {
   int controlPointCount = (int) m_controlPointArray.GetSize();
    // Code here to step through each control point

   int colComID = m_pStreamLayer->GetFieldCol("comID");

   for ( int i=0; i < controlPointCount; i++ )
      {
      ControlPoint *pControl = m_controlPointArray.GetAt(i);
      ASSERT( pControl != NULL );

      TCHAR *r = NULL;
      TCHAR delim[] = ",";
      TCHAR res[256];
      TCHAR *next = NULL;
      CString resString = pControl->m_reservoirsInfluenced;  //list of reservoirs influenced...from XML file
      lstrcpy( res, resString);
      r = _tcstok_s( res, delim, &next );   //Get first value

      ASSERT( r != NULL );
      int res_influenced = atoi (r);       //convert to int

      // Loop through reservoirs and point to the ones that this control point has influence over
      while ( r != NULL )
         {
         for (int j=0; j < m_reservoirArray.GetSize(); j++)
            {
            Reservoir *pRes = m_reservoirArray.GetAt(j);
            ASSERT( pRes != NULL );

            int resID = pRes->m_id;

            if (resID == res_influenced)       //Is this reservoir influenced by this control point?
               {
               pControl->m_influencedReservoirsArray.Add( pRes );    //Add reservoir to array of influenced reservoirs
               r = strtok_s( NULL, delim, &next );   //Look for delimiter, Get next value in string
               if ( r == NULL )
                  break;

               res_influenced = atoi (r);
               }
            }

         /// Need code here (or somewhere, to associate a pReach with a control point)
         // iterate through reach network, looking control point ID's...  need to associate a pReach with each control point
         // Probably need a more efficient way to do this than cycling through the network for every control point
         int reachCount = m_pStreamLayer->GetRecordCount();

         for ( int k=0; k < m_reachArray.GetSize(); k++ )
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
      
      if ( pControl->m_pReach != NULL )
         {
         ASSERT( pControl->m_pResAllocation == NULL );
         pControl->m_pResAllocation = new FDataObj (int( pControl->m_influencedReservoirsArray.GetSize()), 0);

			//load table for control point constraints 
         LoadTable(gpModel->m_path+ pControl->m_dir+pControl->m_controlPointFileName, (DataObj**) &(pControl->m_pControlPointConstraintsTable),    0 );   
         }
      }
   
   return true;
   }


bool FlowModel::UpdateReservoirControlPoints( int doy )
   {
   int controlPointCount = (int) m_controlPointArray.GetSize();
    // Code here to step through each control point
     
   for ( int i=0; i < controlPointCount; i++ )
      {
      ControlPoint *pControl = m_controlPointArray.GetAt(i);
      ASSERT( pControl != NULL );

      if (pControl->InUse() == false )
         continue;

      CString filename = pControl->m_controlPointFileName;

      int location = pControl->m_location;     //Reach where control point is located

      //Get constraint type (m_type).  Assume 1st characters of filename until first underscore or space contain rule type.  
      //Parse rule name and determine m_type  ( IN the case of downstream control points, only maximum and minimum)
      TCHAR name[ 256 ];
      strcpy_s(name, filename);
      TCHAR* ruletype=NULL, *next=NULL;
      TCHAR delim[] = _T( " ,_" );  //allowable delimeters: , ; space ; underscore

      ruletype = strtok_s(name, delim, &next);  //Strip header.  should be cp_ for control points
      ruletype = strtok_s(NULL, delim, &next);  //Returns next string at head of file (max or min). 
         
      // Is this a min or max value
      if ( _stricmp(ruletype, "Max" ) == 0)
         pControl->m_type = RCT_MAX;
      else if (_stricmp(ruletype, "Min" ) == 0)
         pControl->m_type = RCT_MIN;
      
      // Get constraint value
      DataObj *constraintTable = pControl->m_pControlPointConstraintsTable;
      ASSERT( constraintTable != NULL );

     Reach *pReach = pControl->m_pReach;       //get reach at location of control point  
    
      CString xlabel = constraintTable->GetLabel(0);

		float xvalue = 0.0f;
      float yvalue = 0.0f;
		bool isFlowDiff = 0;

		if (_stricmp(xlabel, "Date") == 0)                         //Date based rule?  xvalue = current date.
			xvalue = (float)doy;
		else if (_stricmp(xlabel, "Pool_Elev_m") == 0)              //Pool elevation based rule?  xvalue = pool elevation (meters)
			xvalue = 110;   //workaround 11_15.  Need to point to Fern Ridge pool elev.  only one control point used this xvalue
      else if (_stricmp(xlabel, "Outflow_lagged_24h" ) == 0)
         {
         xvalue = pReach->GetDischarge();
        }
      else if (_stricmp(xlabel,"Inflow_cms" ) == 0)     
         {}//Code here for lagged and inflow based constraints
      else if  (_stricmp(xlabel,"date_water_year_type") == 0)         //Lookup based on two values...date and wateryeartype (storage in 13 USACE reservoirs on May 20th).
         {
         xvalue = (float) doy;
         yvalue = gpModel->m_waterYearType;
         }
      else                                                    //Unrecognized xvalue for constraint lookup table
         { 
         CString msg;
         msg.Format( "Flow:  Unrecognized x value for reservoir constraint lookup '%s', %s (id:%i) in stream network", (LPCTSTR) pControl->m_controlPointFileName, (LPCTSTR) pControl->m_name, pControl->m_id );
         Report::WarningMsg( msg );
         }
        
      
      // if the reach doesn't exist, ignore this
      if ( pReach != NULL )
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
       
         int influenced = (int) pControl->m_influencedReservoirsArray.GetSize();     // varies by control point
         if ( influenced > 0 )
            {
            ASSERT( pControl->m_pResAllocation != NULL );
            ASSERT( influenced == pControl->m_pResAllocation->GetColCount() );

            float *resallocation = new float[influenced]();   //array the size of all reservoirs influenced by this control point
   
            switch( pControl->m_type )
               {
               case RCT_MAX:    //maximum
                  {
                  ASSERT( pControl->m_pControlPointConstraintsTable != NULL );

                  if (yvalue > 0)  //Does the constraint depend on two values?  If so, use both xvalue and yvalue
                     constraintValue = pControl->m_pControlPointConstraintsTable->IGet(xvalue, yvalue, IM_LINEAR);
                  else             //If not, just use xvalue
                     constraintValue = pControl->m_pControlPointConstraintsTable->IGet(xvalue, 1, IM_LINEAR);
            
                  //Compare to current discharge and allocate flow increases or decreases
                  //Currently allocated evenly......need to update based on storage balance curves in ResSIM  mmc 11_15_2012
                  if (currentdischarge > constraintValue)   //Are we above the maximum flow?   
                     {
                     for (int j=0; j < influenced; j++)
                        resallocation[j] = (((constraintValue - currentdischarge)/ influenced));// Allocate decrease in releases (should be negative) over "controlled" reservoirs if maximum, evenly for now
   
                     pControl->m_pResAllocation->ClearRows();     //Clear old data.  Only keep current allocation.
                     pControl->m_pResAllocation->AppendRow(resallocation, influenced);//Populate array with flows to be allocated
                     }
                  else                                      //If not above maximum, set maximum value high, so no constraint will be applied
                     {
                     for (int j=0; j < influenced; j++)
                        resallocation[ j ] = 0.0f;
                     
                     pControl->m_pResAllocation->ClearRows();     //Clear old data.  Only keep current allocation.
                     pControl->m_pResAllocation->AppendRow(resallocation, influenced );   //Populate array with flows to be allocated
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
							float resDischarge = pControl->m_influencedReservoirsArray[0]->m_pReach->GetDischarge();
							addedConstraint = resDischarge + constraintValue;
							int releaseFreq = 1;

							for (int j = 0; j < influenced; j++)
							{
								resallocation[j] = ((addedConstraint - currentdischarge) / influenced); // Allocate increase in releases (should be positive) over "controlled" reservoirs if maximum, evenly for now							}

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

            delete [] resallocation;
            }  // end of: if influenced > 0 )
         }  // end of: if ( pReach != NULL )
      }  // end of:  for each control point
   
   return true;
}


void FlowModel::UpdateReservoirWaterYear( int dayOfYear )
{
	int reservoirCount = (int) m_reservoirArray.GetSize();
	float resVolumeBasin = 0.0;
	for ( int i=0; i < reservoirCount; i++ )
	{
		Reservoir *pRes = m_reservoirArray.GetAt(i);
		ASSERT( pRes != NULL );
			resVolumeBasin += (float) pRes->m_volume;   // This value in m3
	}

		resVolumeBasin = resVolumeBasin*M3_PER_ACREFT*1000000;   //convert volume to millions of acre-ft  (MAF)

		if (resVolumeBasin > 1.48f)                            //USACE defined "Abundant" water year in Willamette
			gpModel->m_waterYearType = 1.48f;
		else if (resVolumeBasin < 1.48f && resVolumeBasin > 1.2f)      //USACE defined "Adequate" water year in Willamette
			gpModel->m_waterYearType = 1.2f;
		else if (resVolumeBasin < 1.2f && resVolumeBasin > 0.9f)       //USACE defined "Insufficient"  water year in Willamette
			gpModel->m_waterYearType = 0.9f;
		else if (resVolumeBasin < 0.9f)                         //USACE defined "Deficit" water year in Willamette
			gpModel->m_waterYearType = 0;

}


bool FlowModel::SetGlobalReservoirFluxesResSimLite( void )
   {
   int reservoirCount = (int) m_reservoirArray.GetSize();

   int dayOfYear = int( fmod( m_timeInRun, 365.0 ) );  // zero based day of year
   UpdateReservoirControlPoints(dayOfYear);   //Update current status of system control points

   //A Willamette specific ruletype that checks res volume in the basin on May 20th and adjusts rules based on water year classification.
   if (dayOfYear == 140)   //Is it May 20th?  Update basinwide reservoir volume and designate water year type
      {
      UpdateReservoirWaterYear( dayOfYear );
      }

   // iterate through reservoirs, calling fluxes as needed
   for ( int i=0; i < reservoirCount; i++ )
      {
     Reservoir *pRes = m_reservoirArray.GetAt(i);
      ASSERT( pRes != NULL );

      // first, inflow.  We get the inflow from any stream reaches flowing into the downstream reach this res is connected to.
      Reach *pReach = pRes->m_pReach;
      
      // no associated reach?
      if ( pReach == NULL )
         {
         pRes->m_inflow = pRes->m_outflow = 0;
         continue;
         }

      Reach *pLeft  = (Reach*) pReach->m_pLeft;
      Reach *pRight = (Reach*) pReach->m_pRight;

      //set inflow, outflows
      float inflow = 0;
     
      //   To be re-implemented with remainder of FLOW hydrology - ignored currently to test ResSIMlite
      if ( pLeft != NULL )
         inflow = pLeft->GetDischarge();
      if ( pRight != NULL )
         inflow += pRight->GetDischarge();
     

      /*////////////////////////////////////////////////////////////////////////////////////////////
     //Get inflows exported from ResSIM model.  Used for testing in conjunction with 0 precip.
     int resID = pRes->m_id;
     
    inflow = m_pResInflows->IGet(m_currentTime, resID);   //inflow value in cms
     
     //Hard code here for additional inflows into Lookout, Foster, Dexter and BigCliff from upstream dam releases

     //Lookout = Lookout + Hills Creek
      if (resID == 2)
       {
       int HCid= 0;
        Reservoir *pHC = m_reservoirArray.GetAt(HCid);
         inflow += pHC->m_outflow/SEC_PER_DAY; 
        }
     //Foster = Foster + Green Peter
      if (resID == 11)
       {
        int GPid = 9;
       Reservoir *pGP = m_reservoirArray.GetAt(GPid);
         inflow += pGP->m_outflow/SEC_PER_DAY;
        }

     //Dexter
     if (resID == 3)
        {
        int LOid = 1;
        Reservoir *pLO = m_reservoirArray.GetAt(LOid);
       inflow = pLO->m_outflow/SEC_PER_DAY;
        }

     //Big Cliff
     if (resID == 13)
       {
        int DETid = 11;
        Reservoir *pDET = m_reservoirArray.GetAt(DETid);
       inflow = pDET->m_outflow/SEC_PER_DAY;
        }
     ///////////////////////////////////////////////////////////////////////////////////////////*/ 
     

      float outflow = 0;

      // rule curve stuff goes here
      pRes->m_inflow = inflow*SEC_PER_DAY;  //m3 per day
     
      
     
      outflow = pRes->GetResOutflow(pRes,dayOfYear);
		ASSERT(outflow >= 0);
      pRes->m_outflow = outflow*SEC_PER_DAY;    //m3 per day 
      }

   return true;
   }


bool FlowModel::SetGlobalFlows( void )
   {
   if ( m_pGlobalFlowData == NULL )
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
         Report::WarningMsg( msg );
         
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
//   int hruLayerCount = GetHRULayerCount();
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
//         int l = hruLayerCount-1;      // bottom layer only
//         HRULayer *pHRULayer = pHRU->GetLayer( l );
//         
//         float depth = 1.0f; 
//         float porosity = 0.4f;
//         float voidVolume = depth*porosity*pHRU->m_area;
//         float wc = float( pHRULayer->m_volumeWater/voidVolume );
// 
//         float waterDepth = wc*depth;
//         float baseflow = GetBaseflow(waterDepth)*pHRU->m_area;//m3/d
//
//         pHRULayer->m_contributionToReach = baseflow;
//         pCatchment->m_contributionToReach += baseflow;
//         }
//      }
//
//   return true;
//   }



//bool FlowModel::SetGlobalHruVertFluxesBrooksCorey( void )
//   {
//   int catchmentCount = (int) m_catchmentArray.GetSize();
//   int hruLayerCount = GetHRULayerCount();
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
//         for ( int l=0; l < hruLayerCount-1; l++ )     //All but the bottom layer
//            {
//            HRULayer *pHRULayer = pHRU->GetLayer( l );
//
//            float depth = 1.0f; float porosity = 0.4f;
//            float voidVolume = depth*porosity*pHRU->m_area;
//            float wc = float( pHRULayer->m_volumeWater/voidVolume );
//
//            pHRULayer->m_verticalDrainage = GetVerticalDrainage(wc)*pHRU->m_area;//m3/d
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


bool FlowModel::RedrawMap(EnvContext *pEnvContext)
   {
   //WriteDataToMap(pEnvContext);

   int activeField = m_pCatchmentLayer->GetActiveField();
//   m_pStreamLayer->UseVarWidth(0, 200);
   m_pStreamLayer->ClassifyData();
   m_pCatchmentLayer->ClassifyData(activeField);

   if ( m_grid ) // m_buildCatchmentsMethod==2 )//a grid based simulation
      {
      if ( m_pGrid != NULL  )
         {
         if (m_gridClassification!=1)
            {
            m_pGrid->SetBins( -1, 0, 5, 20, TYPE_FLOAT );
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification==0)
            {
            m_pGrid->SetBins( -1, 0.1f, 0.4f, 20, TYPE_FLOAT );
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification==1)
            {
            m_pGrid->SetBins( -1, 0.0f, 2.0f, 20, TYPE_FLOAT );
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         if (m_gridClassification==6)
            {
            m_pGrid->SetBins( -1, 0.0f, 0.4f, 20, TYPE_FLOAT );
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);
            }
         
         if ( m_gridClassification==4)//SWE in mm
            {
            m_pGrid->SetBins( -1, 0.0f, 1200.0f, 20, TYPE_FLOAT );
            m_pGrid->SetBinColorFlag(BCF_BLUEGREEN);
            m_pGrid->ClassifyData(0);         
            }  
         if  (m_gridClassification==5)//SWE in mm
            {
            m_pGrid->SetBins( -1, -15.0f, 30.0f ,20, TYPE_FLOAT );
            m_pGrid->SetBinColorFlag(BCF_BLUERED);
            m_pGrid->ClassifyData(0);         
            }  
         }
      }
   
   ::EnvRedrawMap();

   //CDC *dc = m_pMap->GetDC();
   //m_pMap->DrawMap(*dc);
   //m_pMap->ReleaseDC(dc);
   //// yield control to other processes
   //MSG  msg;
   //while( PeekMessage( &msg, NULL, NULL, NULL , PM_REMOVE ) )
   //   {
   //   TranslateMessage(&msg); 
   //   DispatchMessage(& msg); 
   //   }

   return true;
   }


void FlowModel::UpdateYearlyDeltas(EnvContext *pEnvContext )
   {
   int activeField = m_pCatchmentLayer->GetActiveField();
   int catchmentCount = (int) m_catchmentArray.GetSize();
   float airTemp = -999.0f;
   
   if ( ! m_pGrid )   //m_buildCatchmentsMethod != 2 )    // not a grid based model?
      {
      for (int i=0; i< catchmentCount; i++)
         {
         Catchment *pCatchment = m_catchmentArray[ i ];
         HRU *pHRU = NULL;

         for ( int j=0; j < pCatchment->GetHRUCount(); j++ )
            {
            HRU *pHRU = pCatchment->GetHRU( j );

            float aridityIndex = pHRU->m_maxET_yr/pHRU->m_precip_yr;  
            pHRU->m_aridityIndex.AddValue( aridityIndex );
            float aridityIndexMovingAvg = pHRU->m_aridityIndex.GetValue();

            for (int k=0; k< pHRU->m_polyIndexArray.GetSize();k++)
               {
               int idu = pHRU->m_polyIndexArray[ k ];

               // annual climate variables
               gpFlow->UpdateIDU(pEnvContext, idu, m_colAridityIndex, aridityIndexMovingAvg, true);
               
               pHRU->m_temp_yr /= 365;   // note: assumed daily timestep
               pHRU->m_temp_10yr.AddValue( pHRU->m_temp_yr );
               pHRU->m_precip_10yr.AddValue( pHRU->m_precip_yr );

               gpFlow->UpdateIDU( pEnvContext, idu, m_colHruTempYr,   pHRU->m_temp_yr, true );   // C
               gpFlow->UpdateIDU( pEnvContext, idu, m_colHruTemp10Yr, pHRU->m_temp_10yr.GetValue(), true ); // C

               gpFlow->UpdateIDU( pEnvContext, idu, m_colHruPrecipYr,   pHRU->m_precip_yr, true );   // mm/year
               gpFlow->UpdateIDU( pEnvContext, idu, m_colHruPrecip10Yr, pHRU->m_precip_10yr.GetValue(), true ); // mm/year
               // m_precip_wateryr????

               // annual hydrologic variables

               // SWE-related variables (Note: Apr1 value previously Updated()
               //gpFlow->UpdateIDU( pEnvContext, idu, m_colHruApr1SWE, pHRU->m_depthApr1SWE_yr, true );
               //gpFlow->UpdateIDU( pEnvContext, idu, m_colHruApr1SWE10yr, pHRU->m_apr1SWE_10yr.GetValue(), true );

               gpFlow->UpdateIDU( pEnvContext, idu, m_colRunoff_yr, pHRU->m_runoff_yr, true );
               gpFlow->UpdateIDU( pEnvContext, idu, m_colStorage_yr, pHRU->m_storage_yr, true );
               gpFlow->UpdateIDU( pEnvContext, idu, m_colMaxET_yr, pHRU->m_maxET_yr, true);
               gpFlow->UpdateIDU( pEnvContext, idu, m_colET_yr, pHRU->m_et_yr, true);
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
   for (int i = 0; i< catchmentCount; i++)
      {
      Catchment *pCatchment = m_catchmentArray[i];
      for (int j = 0; j < pCatchment->GetHRUCount(); j++)
         {
         HRU *pHRU = pCatchment->GetHRU(j);    
         totalSnow+=((float)pHRU->GetLayer(0)->m_volumeWater + (float)pHRU->GetLayer(1)->m_volumeWater) ; //get the current year snowpack
         }
      }
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
         for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
            gpFlow->UpdateIDU(pEnvContext, pHRU->m_polyIndexArray[k], m_colHruMaxSWE, ((float)pHRU->GetLayer(0)->m_volumeWater + (float)pHRU->GetLayer(1)->m_volumeWater), false);
         }
      }
   m_pCatchmentLayer->m_readOnly = true;
   }


void FlowModel::UpdateAprilDeltas(EnvContext *pEnvContext )
   {
  // int activeField = m_pCatchmentLayer->GetActiveField();
   int catchmentCount = (int) m_catchmentArray.GetSize();
   m_pCatchmentLayer->m_readOnly=false;
  // float airTemp=-999.0f;
   
   if ( !m_pGrid )   //m_buildCatchmentsMethod != 2 )    // not a grid based model?
      {
      for (int i=0; i< catchmentCount; i++)
         {
         Catchment *pCatchment = m_catchmentArray[ i ];
         for ( int j=0; j < pCatchment->GetHRUCount(); j++ )
            {
            HRU *pHRU = pCatchment->GetHRU( j );
            for (int k=0; k< pHRU->m_polyIndexArray.GetSize();k++)
               {
               int idu = pHRU->m_polyIndexArray[k];

               float refSnow=0.0f;
               if (pEnvContext->yearOfRun==0) //reset the reference
                  m_pCatchmentLayer->SetData( idu, m_colRefSnow,  0.0f); 
               else
                  m_pCatchmentLayer->GetData( idu, m_colRefSnow,  refSnow ); 

               float _ref=((float)pHRU->GetLayer(0)->m_volumeWater+(float)pHRU->GetLayer(1)->m_volumeWater)/pHRU->m_area*1000.0f ; //get the current year snowpack (mm)
               
               if (pEnvContext->yearOfRun<10)//estimate the denominator of the snow index
                  m_pCatchmentLayer->SetData( idu, m_colRefSnow,  _ref+refSnow); //total amount of snow over the 5 year period
           
               float snowIndex = 0.0f;
               if (refSnow > 0.0f)
                  snowIndex=_ref/(refSnow/10.0f);    // compare to avg over first decade

               if (pEnvContext->yearOfRun >= 10)
                  gpFlow->UpdateIDU(pEnvContext, idu, m_colSnowIndex, snowIndex, false);

               // BAD!!!! assumes top two layers are snow!!!
               float apr1SWE = (((float)pHRU->GetLayer(0)->m_volumeWater + (float)pHRU->GetLayer(1)->m_volumeWater) / pHRU->m_area )*1000.0f; // (mm)
               if ( apr1SWE < 0 )
                  apr1SWE = 0;
               
               gpFlow->UpdateIDU(pEnvContext, idu, m_colHruApr1SWE, apr1SWE, true );

               pHRU->m_apr1SWE_10yr.AddValue( apr1SWE );
               float apr1SWEMovAvg = pHRU->m_apr1SWE_10yr.GetValue();
               gpFlow->UpdateIDU( pEnvContext, idu, m_colHruApr1SWE10Yr, apr1SWEMovAvg, true );

               pHRU->m_precipCumApr1 = pHRU->m_precip_yr;      // mm of cum precip Jan 1 - Apr 1)
               }
            }
         }
      }

   m_pCatchmentLayer->m_readOnly=true;
   }


// called each FLOW timestep when m_mapUpdate == 2
bool FlowModel::WriteDataToMap(EnvContext *pEnvContext )
   {
   int activeField = m_pCatchmentLayer->GetActiveField();
   m_pCatchmentLayer->m_readOnly=false;
   int catchmentCount = (int) m_catchmentArray.GetSize();

   float time= float( m_currentTime-(int)m_currentTime );
   float currTime=m_flowContext.dayOfYear+time;
   if ( !m_pGrid )   //m_buildCatchmentsMethod != 2 )    // not a grid based model?
      {
      int hruCount = (int) m_hruArray.GetSize();
      //#pragma omp parallel for  // firstprivate( pEnvContext )
      for ( int h=0; h < hruCount; h++ )
         {
         float airTemp=-999.0f;
         float prec=0.0f;
         HRU *pHRU = m_hruArray[h];
         GetHRUClimate(CDT_TMEAN,  pHRU, (int) currTime, airTemp);
         GetHRUClimate(CDT_PRECIP, pHRU, (int) currTime, prec);
         for (int k=0; k< pHRU->m_polyIndexArray.GetSize();k++)
            {  
            int idu = pHRU->m_polyIndexArray[k];

            m_pCatchmentLayer->SetData( idu, m_colHruTemp,  airTemp ); 
            m_pCatchmentLayer->SetData( idu, m_colHruPrecip, prec );

            float snow = pHRU->GetLayer(0)->m_volumeWater/pHRU->m_area*1000.0f;
            m_pCatchmentLayer->SetData( idu, m_colHruSWE, snow ); //mm of snow
         //   m_pCatchmentLayer->SetData( idu, m_colHruSWC,   pHRU->m_currentMAX_ET );
            }
         }
      }

   int reachCount = (int)m_reachArray.GetSize();
   m_pStreamLayer->m_readOnly = false;

   for ( int i=0; i < reachCount; i++ )
      {
      Reach *pReach = m_reachArray.GetAt(i);
      ASSERT( pReach != NULL );

      if ( pReach->m_subnodeArray.GetSize() > 0 && pReach->m_polyIndex >= 0 )
         {
         ReachSubnode *pNode = (ReachSubnode*) pReach->m_subnodeArray[ 0 ];
         ASSERT( pNode != NULL );
         m_pStreamLayer->SetData(pReach->m_polyIndex, m_colStreamReachOutput, pNode->m_discharge);  // m3/sec
         }
      }

   if ( m_pGrid ) // m_buildCatchmentsMethod==2)//a grid based simulation
      {
      int hruCount = (int) m_hruArray.GetSize();
      #pragma omp parallel for // firstprivate( pEnvContext )
      for ( int h=0; h < hruCount; h++ )
        {
        HRU *pHRU = m_hruArray[h];
         float wc = 0.0f;
         float myElev=50;
         if (m_gridClassification==0)
            wc=pHRU->GetLayer(0)->m_wc;         
        if (m_gridClassification==6)//water content in the 1st layer
            wc=pHRU->GetLayer(2)->m_wc;     

        if (m_gridClassification==1)
           {
           if (pHRU->GetLayer(1)->m_volumeWater>0.0f)
              {
              float porosity=0.4f;
              float area = pHRU->m_area;
              HRULayer *pHRULayer0 = pHRU->GetLayer(0);
              HRULayer *pHRULayer1 = pHRU->GetLayer(1);
              float d0 = pHRULayer0->m_depth;
              float d1 = pHRULayer1->m_depth;
              float w0 = (float) pHRULayer0->m_volumeWater;
              float w1 = (float) pHRULayer1->m_volumeWater;

              float depthWater = (w0+w1)/area ;//length of water
              float totalDepth = (d1+d0)*porosity; //total length (of airspace) available
              float depthToWT= totalDepth-depthWater;
              if (depthToWT < 0.0f)
                 depthToWT=0.0f;
              wc = depthToWT;
              }           
           }
        if (pHRU->GetLayer(0)->m_svArray != NULL)
           {
           if (m_gridClassification==2)
              {
              if (pHRU->GetLayer(0)->m_volumeWater>0.0f)
                 {
                 float water = (float)pHRU->GetLayer(0)->m_volumeWater;
                 float mass=pHRU->GetLayer(0)->GetExtraStateVarValue(0);
                 wc=mass/water;//kg/m3
                 int stop=1;
                 }           
              }
           if (m_gridClassification==3)
              {
              if (pHRU->GetLayer(1)->m_volumeWater>0.0f)
                 {
                 float water = (float)pHRU->GetLayer(1)->m_volumeWater;
                 float mass=pHRU->GetLayer(1)->GetExtraStateVarValue(0);
                 wc=mass/water;//kg/m3
                 int stop=1;
                 }           
              }
           
            }
         m_pGrid->SetData( pHRU->m_demRow, pHRU->m_demCol, wc);  
         }
      }

   return true;
   }


bool FlowModel::ResetCumulativeValues(  EnvContext *pEnvContext  )
   {
   for (int i=0; i < m_catchmentArray.GetSize(); i++)
      {
      ASSERT( m_catchmentArray[ i ] != NULL );
      m_catchmentArray[ i ]->m_contributionToReach=0.0f;
      }

   for ( int i=0; i < m_reachArray.GetSize(); i++ )
      {
      Reach *pReach = m_reachArray.GetAt(i);
      pReach->ResetFluxValue();

      ReachSubnode *subnode = pReach->GetReachSubnode(0);
      for (int k=0;k<m_hruSvCount-1;k++)
         subnode->ResetExtraSvFluxValue(k);
      }

   for ( int i=0; i < m_hruArray.GetSize(); i++ )
      {
      HRU *pHRU = m_hruArray.GetAt(i);
      int neighborCount = (int) pHRU->m_neighborArray.GetSize();
      //pHRU->m_totalMAX_ET = 0.0f;
      //pHRU->m_totalPrecip = 0.0f;
      for (int j=0;j<pHRU->m_layerArray.GetSize();j++)
         {
         HRULayer *pLayer = pHRU->m_layerArray.GetAt(j);
         pLayer->ResetFluxValue();

         for (int k=0;k<7+neighborCount;k++)
            pLayer->m_waterFluxArray[k]=0.0f;

         for (int l=0;l<m_hruSvCount-1;l++)
            pLayer->ResetExtraSvFluxValue(l);     
         }
      }

   return true;
   }


bool FlowModel::ResetCumulativeYearlyValues( )
   {
   EnvContext *pContext = m_flowContext.pEnvContext;
   
   for ( int i=0; i < m_hruArray.GetSize(); i++ )
      {
      HRU *pHRU = m_hruArray.GetAt(i);
      
      int hruLayerCount = pHRU->GetLayerCount();
      float waterDepth=0.0f;
      for ( int l=0; l < hruLayerCount; l++ )     
         {
         HRULayer *pHRULayer = pHRU->GetLayer( l );
         waterDepth += float( pHRULayer->m_wc );//mm of total storage
         }
      
      pHRU->m_endingStorage = waterDepth;
      pHRU->m_storage_yr    = pHRU->m_endingStorage-pHRU->m_initStorage;
      pHRU->m_initStorage   = pHRU->m_endingStorage;

      //for ( int i=0; i < pHRU->m_polyIndexArray.GetSize(); i++ )
      //   {
      //   int idu = pHRU->m_polyIndexArray[ i ];
      //
      //   gpFlow->UpdateIDU( pContext, idu, m_colPrecip_yr,  pHRU->m_precip_yr, true );
      //   gpFlow->UpdateIDU( pContext, idu, m_colRunoff_yr,  pHRU->m_runoff_yr, true );
      //   gpFlow->UpdateIDU( pContext, idu, m_colStorage_yr, pHRU->m_storage_yr, true );
      //   gpFlow->UpdateIDU( pContext, idu, m_colMaxET_yr,   pHRU->m_maxET_yr, true);
      //   gpFlow->UpdateIDU( pContext, idu, m_colET_yr,      pHRU->m_et_yr, true);
      //   }

      pHRU->m_temp_yr   = 0;
      pHRU->m_precip_yr = 0;
      pHRU->m_rainfall_yr = 0;
      pHRU->m_snowfall_yr = 0;
      pHRU->m_precipCumApr1 = 0;
      pHRU->m_maxET_yr = 0.0f;
      pHRU->m_et_yr = 0.0f;
      pHRU->m_runoff_yr =0.0f;
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
bool FlowModel::InitReaches( void )
   {  
   ASSERT( m_pStreamLayer != NULL );

   m_colStreamFrom = m_pStreamLayer->GetFieldCol( _T("FNODE_") );
   m_colStreamTo   = m_pStreamLayer->GetFieldCol( _T("TNODE_") );

   if ( m_colStreamFrom < 0 || m_colStreamTo < 0 )
      {
      Report::ErrorMsg( "Flow: Stream layer is missing a required column (FNODE_ and/or TNODE_).  These are necessary to establish topology" );
      return false;
      }
   
/////////////////////
//   FDataObj dataObj;
//   m_reachTree.BuildDistanceMatrix( &dataObj );
//   dataObj.WriteAscii( "C:\\envision\\studyareas\\wesp\\distance.csv" );
/////////////////////

   // figure out which stream layer reaches to actually use in the model
   if ( m_pStreamQE != NULL && m_streamQuery.GetLength() > 0 )
      {
      CString source( "Flow Stream Query" );
      
      m_pStreamQuery = m_pStreamQE->ParseQuery( m_streamQuery, 0, source );

      if ( m_pStreamQuery == NULL )
         {
         CString msg( "Flow: Error parsing stream query: '" );
         msg += m_streamQuery;
         msg += "' - the query will be ignored";
         Report::ErrorMsg( msg );
         }
      }

   // build the network topology for the stream reaches
   int nodeCount = m_reachTree.BuildTree( m_pStreamLayer, &Reach::CreateInstance, m_colStreamFrom, m_colStreamTo, m_colStreamJoin /*in*/, NULL /*m_pStreamQuery*/  );   // jpb 10/7
   
   if ( nodeCount < 0 )
      {
      CString msg;
      msg.Format( "Flow: Error building reach topology:  Error code %i", nodeCount );
      Report::ErrorMsg( msg );
      return false;
      }

   // populate stream order if column provided
   if ( m_colStreamOrder >= 0 )
      m_reachTree.PopulateOrder( m_colStreamOrder, true );

   // populate TreeID if specified
   if ( m_colTreeID >= 0 )
      {
      m_pStreamLayer->SetColNoData( m_colTreeID );
      int roots = m_reachTree.GetRootCount();

      for ( int i=0; i < roots; i++ )
         {
         ReachNode *pRoot = m_reachTree.GetRootNode( i );
         PopulateTreeID( pRoot, i );
         }
      }
      

   // for each tree, add Reaches to our internal array of Reach ptrs
   int roots = m_reachTree.GetRootCount();

   for ( int i=0; i < roots; i++ )
      {
      ReachNode *pRoot = m_reachTree.GetRootNode( i );
      ReachNode *pReachNode = (Reach*) m_reachTree.FindLeftLeaf( pRoot );  // find leftmost leaf of entire tree
      m_reachArray.Add( (Reach*) pReachNode );   // add leftmost node
   
      while ( pReachNode )  
         {
         ReachNode *pDownNode = pReachNode->m_pDown;

         if ( pDownNode == NULL ) // root of tree reached?
            break;

         if ( ( pDownNode->m_pLeft == pReachNode ) && ( pDownNode->m_pRight != NULL ) )
            pReachNode = m_reachTree.FindLeftLeaf( pDownNode->m_pRight );
         else 
            pReachNode = pDownNode;

         if ( pReachNode->IsPhantomNode() == false )          
            m_reachArray.Add( (Reach*) pReachNode );
         }
      }

   //ASSERT(nodeCount == (int) m_reachArray.GetSize() );

   // allocate subnodes to each reach
   m_reachTree.CreateSubnodes( &ReachSubnode::CreateInstance, m_subnodeLength );

   // allocate any necessary "extra" state variables
   if ( m_reachSvCount > 1 )
      {
      int reachCount = (int) m_reachArray.GetSize();
      for ( int i=0; i < reachCount; i++ )
         {
         Reach *pReach = m_reachArray[ i ];

         for ( int j=0; j < pReach->GetSubnodeCount(); j++ )
            {
            ReachSubnode *pSubnode = pReach->GetReachSubnode( j );
            pSubnode->AllocateStateVars( m_reachSvCount-1 );
            }
         }
      }
   
   return true;
   }


bool FlowModel::InitReachSubnodes( void )
   {  
   ASSERT( m_pStreamLayer != NULL );

   // allocate subnodes to each reach.  Note that this ignores phantom nodes
   m_reachTree.CreateSubnodes( &ReachSubnode::CreateInstance, m_subnodeLength );

   // allocate any necessary "extra" state variables
   if ( m_reachSvCount > 1 )
      {
      int reachCount = (int) m_reachArray.GetSize();
      for ( int i=0; i < reachCount; i++ )
         {
         Reach *pReach = m_reachArray[ i ];
         ASSERT( pReach != NULL );

         for ( int j=0; j < pReach->GetSubnodeCount(); j++ )
            {
            ReachSubnode *pSubnode = pReach->GetReachSubnode( j );
            ASSERT( pSubnode != NULL );

            pSubnode->AllocateStateVars( m_reachSvCount-1 );
            for (int k=0; k < m_reachSvCount-1; k++)   
               {
               pSubnode->m_svArray[ k ] = 0.0f;//concentration = 1 kg/m3 ???????
               pSubnode->m_svArrayTrans[ k ] = 0.0f;//concentration = 1 kg/m3
               }
            }
         }
      }
   
   return true;
   }


void FlowModel::PopulateTreeID( ReachNode *pNode, int treeID )
   {
   if ( pNode == NULL )
      return;

   if ( pNode->m_polyIndex < 0 )
      return;

   m_pStreamLayer->SetData( pNode->m_polyIndex, m_colTreeID, treeID );

   // recurse up tree
   PopulateTreeID( pNode->m_pLeft, treeID );
   PopulateTreeID( pNode->m_pRight, treeID );
   }


// create, initialize reaches based on the stream layer
bool FlowModel::InitCatchments( void )
   {  
   ASSERT( m_pCatchmentLayer != NULL );

   // first, figure out where the catchments and HRU's are.
   if ( m_pCatchmentQE != NULL && m_catchmentQuery.GetLength() > 0 )
      {
      m_pCatchmentQuery = m_pCatchmentQE->ParseQuery( m_catchmentQuery, 0, "Flow Catchment Query" );

      if ( m_pCatchmentQuery == NULL )
         {
         CString msg( "Flow: Error parsing Catchment query: '" );
         msg += m_catchmentQuery;
         msg += "' - the query will be ignored";
         Report::ErrorMsg( msg );
         }
      }

   if ( m_pGrid )//if m_pGrid is not NULL, then we are using grids to define the system of model elements
      BuildCatchmentsFromGrids();
   else
      {
      switch( m_buildCatchmentsMethod )
         {
         case 1:  
            BuildCatchmentsFromQueries();   // requires m_catchmentAggCols to be defined
            break;

         default:
            BuildCatchmentsFromLayer();      // requires 
         }
      }

  // SetAllCatchmentAttributes();     // populates internal HRU members (area, elevation, centroid) for all catchments

   // allocate any necessary "extra" state variables
   if ( m_hruSvCount > 1 )//then there are extra state variables..
      {
      int catchmentCount = (int) m_catchmentArray.GetSize();
      for ( int i=0; i < catchmentCount; i++)
         {
         Catchment *pCatchment = m_catchmentArray[ i ];
         HRU *pHRU = NULL;

         for ( int j=0; j < pCatchment->GetHRUCount(); j++ )
            {
            HRU *pHRU = pCatchment->GetHRU( j );

            for (int k=0; k < pHRU->GetLayerCount(); k++ )
               {
               HRULayer *pLayer = pHRU->GetLayer( k );
               pLayer->AllocateStateVars( m_hruSvCount-1 );
               }
            }
         }
      }

   return true;
   }


bool FlowModel::SetAllCatchmentAttributes(void)
   {
   if ( !m_pGrid)   // if ( m_buildCatchmentsMethod < 2 )
      {
      int catchmentCount = (int) m_catchmentArray.GetSize();
      for (int i=0; i< catchmentCount; i++)
         {
         Catchment *pCatchment = m_catchmentArray[ i ];
         SetHRUAttributes( pCatchment );
         }
      }

   return true;
   }


bool FlowModel::SetHRUAttributes( Catchment *pCatchment )
   {
   HRU *pHRU = NULL;
   pCatchment->m_area=0.0f;
   for ( int j=0; j < pCatchment->GetHRUCount(); j++ )
      {
      float area=0.0f; float elevation=0.0f;float _elev=0.0f;
      HRU *pHRU = pCatchment->GetHRU( j );

      pHRU->m_area = 0;

      for (int k=0; k < pHRU->m_polyIndexArray.GetSize();k++)
         {
         m_pCatchmentLayer->GetData( pHRU->m_polyIndexArray[k], m_colCatchmentArea, area );
         m_pCatchmentLayer->GetData( pHRU->m_polyIndexArray[k], m_colElev, _elev );
         pHRU->m_area += area;
         elevation    += _elev*area; 
         pCatchment->m_area += area;
         }

      pHRU->m_elevation = elevation / pHRU->m_area;
    
     // Vertex centroid = m_pCatchmentLayer->GetCentroidFromPolyOffsets(&pHRU->m_polyIndexArray);

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

bool FlowModel::BuildCatchmentsFromGrids( void )
   {
   ParseCols( m_pCatchmentLayer, m_catchmentAggCols, m_colsCatchmentAgg );
   ParseCols( m_pCatchmentLayer, m_hruAggCols, m_colsHruAgg );
   int colCatchID=m_pCatchmentLayer->m_pData->GetCol(m_catchIDCol);

   // first, do catchments
   CArray< Query*, Query* > catchmentQueryArray;
   int queryCount = GenerateAggregateQueries( m_pCatchmentLayer, m_colsCatchmentAgg, catchmentQueryArray );

   CArray< Query*, Query* > hruQueryArray;
   int hruQueryCount = GenerateAggregateQueries( m_pCatchmentLayer, m_colsHruAgg, hruQueryArray );

   // temporary allocations
   Catchment **catchments = new Catchment*[ queryCount ];  // placeholder for catchments
   memset( catchments, 0, queryCount*sizeof( Catchment* ) );  // initially, all NULL

   m_pCatchmentLayer->SetColNoData( m_colCatchmentCatchID );
   m_pCatchmentLayer->SetColNoData( m_colCatchmentHruID );

   int hruCols = (int) m_colsHruAgg.GetSize();
   VData *hruValueArray = new VData[ hruCols ];

   int polyCount = m_pCatchmentLayer->GetPolygonCount();
   int hruCount = 0;
   
   if ( m_pCatchmentQuery != NULL )
      polyCount = m_pCatchmentQuery->Select( true );

   // iterate though polygons that satisfy the catchment query.
   for ( int i=0; i < polyCount; i++ )
      {
      int polyIndex = m_pCatchmentQuery ? m_pCatchmentLayer->GetQueryResult( i ) : i;

      // get hru aggregate values from this poly
      for ( int h=0; h < hruCols; h++ )
         {
         bool ok = m_pCatchmentLayer->GetData( polyIndex, m_colsHruAgg[ h ], hruValueArray[ h ] );
         if ( ! ok )
            hruValueArray[ h ].SetNull();
         }

      // iterate through the catchment aggregate queries, to see if this polygon satisfies any of them
      for ( int j=0; j < queryCount; j++ )
         {
         Query *pQuery = catchmentQueryArray.GetAt( j );
         bool result = false;
         bool ok = pQuery->Run( polyIndex, result );

         if ( ok && result )  // is this polygon in a the specified Catchment (query?) - yes!
            {
            if ( catchments[ j ] == NULL )    // if first one, creaet a Catchment object for it
               {
               catchments[ j ] = new Catchment;
               catchments[ j ]->m_id = j;
               }
            m_pCatchmentLayer->SetData(i,colCatchID,j);
            }  // end of: if ( ok && result )  meaning adding polyIndex to catchment
         }  // end of: for ( j < queryCount )    
      }  // end of: for ( i < polyCount )

   // store catchments
   for ( int j=0; j < queryCount; j++ )
      {
      Catchment *pCatchment = catchments[ j ];
      if ( pCatchment != NULL )
         m_catchmentArray.Add( pCatchment );
      }

   // clean up
   delete [] catchments;
   delete [] hruValueArray;

   if ( m_pCatchmentQuery != NULL )
      m_pCatchmentLayer->ClearQueryResults();
  
   //Catchments are allocated, now we need to iterate the grid allocating an HRU for every grid cell.
   HRU *pHRU = NULL;
   hruCount=0;
   int foundHRUs=0;
   float minElev=1E10;
   for ( int i=0; i < m_pGrid->GetRowCount(); i++ )
      {
      for ( int j=0; j < m_pGrid->GetColCount(); j++ )
         {
         float x=0, y=0;
         int index=-1; 
         m_pGrid->GetGridCellCenter(i,j,x,y);
         Poly *pPoly = m_pCatchmentLayer->GetPolygonFromCoord(x,y,&index);
         float elev = 0.0f;
         
         m_pGrid->GetData( i, j, elev);
         if (elev != m_pGrid->GetNoDataValue())
            {
            if ( pPoly )      // found an IDU at the location provided, try to find a catchment 
               {
               for ( int k=0; k < (int) m_catchmentArray.GetSize(); k++ ) //figure out which catchment this HRU is in!
                  {
                  Catchment *pCatchment = m_catchmentArray[ k ];
                  ASSERT( pCatchment != NULL );

                  int catchID=-1;
                  m_pCatchmentLayer->GetData(index, colCatchID, catchID );
               
                  if ( catchID == pCatchment->m_id )//if the catchments ReachID equals this polygons reachID
                     {
                     pHRU = new HRU;
                     pHRU->m_id = hruCount;
                     pHRU->m_demRow = i;
                     pHRU->m_demCol = j;
                     pHRU->m_area = m_pGrid->GetGridCellWidth()*m_pGrid->GetGridCellWidth();
                     float x=0.0f;float y=0.0f;
                     m_pGrid->GetGridCellCenter(i,j,x,y);
                     pHRU->m_centroid.x=x;
                     pHRU->m_centroid.y=y;                  
                  
                     float elev = 0.0f;
                     m_pGrid->GetData( i, j, elev);
                     pHRU->m_elevation = elev;
                     if (elev<minElev)
                        minElev=elev;

                     pHRU->AddLayers( m_soilLayerCount, m_snowLayerCount, m_vegLayerCount, 0, m_initTemperature, true );

                     hruCount++;
      
                     AddHRU( pHRU, pCatchment );
                     pHRU->m_polyIndexArray.Add( index );
                     foundHRUs++;
                     break;
                     }
                  }
               }  // end of: if ( poly found )
            else//we have a grid cell that includes data, but its centoid does not overlay the IDUs.  This can happen, particularly with coarsened grids
               m_pGrid->SetData(i,j,m_pGrid->GetNoDataValue());
            }
         }
      }  // end of: iterating through elevation grid
   m_minElevation=minElev;
   hruCount = GetHRUCount();  
   for ( int h=0; h < hruCount; h++ )
      {
      HRU *pHRU = GetHRU(h);
      int neighborCount=0;
      for ( int k=0; k < hruCount; k++ )
         {
         HRU *pNeighborHRU = GetHRU( k );
         ASSERT( pNeighborHRU != NULL );
            
         if (pHRU == pNeighborHRU )
            int stop=1;
         else if (( pNeighborHRU->m_demRow == pHRU->m_demRow-1 || pNeighborHRU->m_demRow == pHRU->m_demRow || pNeighborHRU->m_demRow == pHRU->m_demRow+1 )
                  && pNeighborHRU->m_demCol == pHRU->m_demCol-1 ) 
            {
            pHRU->m_neighborArray.Add( pNeighborHRU );
            neighborCount++;
            }
         else if ((pNeighborHRU->m_demRow == pHRU->m_demRow-1 || pNeighborHRU->m_demRow == pHRU->m_demRow+1) 
                  && pNeighborHRU->m_demCol == pHRU->m_demCol ) 
            {
            pHRU->m_neighborArray.Add( pNeighborHRU );
            neighborCount++;
            }
         else if ((pNeighborHRU->m_demRow == pHRU->m_demRow-1 || pNeighborHRU->m_demRow == pHRU->m_demRow || pNeighborHRU->m_demRow == pHRU->m_demRow+1)
                  && pNeighborHRU->m_demCol == pHRU->m_demCol+1 ) 
            {
            pHRU->m_neighborArray.Add( pNeighborHRU );
            neighborCount++;
            }
        
         }

      // TODO - NEEDS REVIEW!!!!
      ASSERT(neighborCount<9);
      pHRU->m_waterFluxArray = new float[neighborCount];//add 2 to accomodate upward and downward fluxes
      int layerCount = pHRU->GetLayerCount();
      
      HRULayer *pHRULayer = pHRU->GetLayer(layerCount-1); //this is the bottom layer.  Assume it includes exchange between horizontal neighbors
      pHRULayer->m_waterFluxArray = new float[neighborCount+7];

      for (int i=0; i <neighborCount+7; i++)
         pHRULayer->m_waterFluxArray[i]=0.0f;
      
      for (int i=0; i< layerCount-1;i++)                  //Assume all other layers include exchange only in upward and downward directions
         {
         pHRULayer = pHRU->GetLayer(i);
         pHRULayer->m_waterFluxArray = new float[7];
         for (int j=0; j < 7; j++)
            pHRULayer->m_waterFluxArray[j]=0.0f;
         }
      
      }
   // Build topology between different HRUs
/*   for ( int i=0; i < m_catchmentArray.GetSize(); i++ ) //catchments
      {
      Catchment *pCatchment = m_catchmentArray[ i ];

      for ( int j=0; j < pCatchment->GetHRUCount(); j++ )
         {
         HRU *pHRU = pCatchment->GetHRU( j );
         ASSERT( pHRU != NULL );
         int neighborCount=0;
         for ( int k=0; k < pCatchment->GetHRUCount(); k++ )
            {
            HRU *pNeighborHRU = pCatchment->GetHRU( k );
            ASSERT( pNeighborHRU != NULL );
            
            if (pHRU == pNeighborHRU )
               ;
            else if (( pNeighborHRU->m_demRow == pHRU->m_demRow-1 || pNeighborHRU->m_demRow == pHRU->m_demRow || pNeighborHRU->m_demRow == pHRU->m_demRow+1 )
                     && pNeighborHRU->m_demCol == pHRU->m_demCol-1 ) 
               {
               pHRU->m_neighborArray.Add( pNeighborHRU );
               neighborCount++;
               }
            else if ((pNeighborHRU->m_demRow == pHRU->m_demRow-1 || pNeighborHRU->m_demRow == pHRU->m_demRow+1) 
                     && pNeighborHRU->m_demCol == pHRU->m_demCol ) 
               {
               pHRU->m_neighborArray.Add( pNeighborHRU );
               neighborCount++;
               }
            else if ((pNeighborHRU->m_demRow == pHRU->m_demRow-1 || pNeighborHRU->m_demRow == pHRU->m_demRow || pNeighborHRU->m_demRow == pHRU->m_demRow+1)
                     && pNeighborHRU->m_demCol == pHRU->m_demCol+1 ) 
               {
               pHRU->m_neighborArray.Add( pNeighborHRU );
               neighborCount++;
               }
            }
         ASSERT(neighborCount<9);
         pHRU->m_waterFluxArray = new float[neighborCount];//add 2 to accomodate upward and downward fluxes
         int layerCount = pHRU->GetLayerCount();
         HRULayer *pHRULayer = pHRU->GetLayer(layerCount-1); //Assume the bottom layer includes exchange between horizontal neighbors
         pHRULayer->m_waterFluxArray = new float[neighborCount+5];
         for (int i=0;i<neighborCount+5;i++)
            pHRULayer->m_waterFluxArray[i]=0.0f;
         for (int i=0; i< layerCount-1;i++)                  //Assume all other layers include exchange only in upward and downward directions
            {
            pHRULayer = pHRU->GetLayer(i);
            pHRULayer->m_waterFluxArray = new float[5];
            for (int j=0;j<5;j++)
               pHRULayer->m_waterFluxArray[j]=0.0f;
            }
         
         }
      }*/

   CString msg;
   msg.Format( _T("Flow is Going Grid!!: %i Catchments, %i HRUs built from %i polygons.  %i HRUs were associated with Catchments" ), (int) m_catchmentArray.GetSize(), hruCount, polyCount, foundHRUs ); 
   Report::LogMsg( msg, RT_INFO );
   return true;

   }

bool FlowModel::BuildCatchmentsFromQueries( void )
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

   ParseCols( m_pCatchmentLayer, m_catchmentAggCols, m_colsCatchmentAgg );
   ParseCols( m_pCatchmentLayer, m_hruAggCols, m_colsHruAgg );

   // first, do catchments
   CArray< Query*, Query* > catchmentQueryArray;
   int queryCount = GenerateAggregateQueries( m_pCatchmentLayer, m_colsCatchmentAgg, catchmentQueryArray );

   CArray< Query*, Query* > hruQueryArray;
   int hruQueryCount = GenerateAggregateQueries( m_pCatchmentLayer, m_colsHruAgg, hruQueryArray );

   // temporary allocations
   Catchment **catchments = new Catchment*[ queryCount ];  // placeholder for catchments
   memset( catchments, 0, queryCount*sizeof( Catchment* ) );  // initially, all NULL

   m_pCatchmentLayer->SetColNoData( m_colCatchmentCatchID );
   m_pCatchmentLayer->SetColNoData( m_colCatchmentHruID );

   int hruCols = (int) m_colsHruAgg.GetSize();
   VData *hruValueArray = new VData[ hruCols ];

   int polyCount = m_pCatchmentLayer->GetPolygonCount();
   int hruCount = 0;
   
   if ( m_pCatchmentQuery != NULL )
      polyCount = m_pCatchmentQuery->Select( true );

   // iterate though polygons that satisfy the catchment query.
   for ( int i=0; i < polyCount; i++ )
      {
      int polyIndex = m_pCatchmentQuery ? m_pCatchmentLayer->GetQueryResult( i ) : i;

      // get hru aggregate values from this poly
      for ( int h=0; h < hruCols; h++ )
         {
         bool ok = m_pCatchmentLayer->GetData( polyIndex, m_colsHruAgg[ h ], hruValueArray[ h ] );
         if ( ! ok )
            hruValueArray[ h ].SetNull();
         }

      // iterate through the catchment aggregate queries, to see if this polygon satisfies any of them
      for ( int j=0; j < queryCount; j++ )
         {
         Query *pQuery = catchmentQueryArray.GetAt( j );
         bool result = false;
         bool ok = pQuery->Run( polyIndex, result );

         if ( ok && result )  // is this polygon in a the specified Catchment (query?) - yes!
            {
            if ( catchments[ j ] == NULL )    // if first one, creaet a Catchment object for it
               {
               catchments[ j ] = new Catchment;
               catchments[ j ]->m_id = j;
               }

            // we know which catchment it's in, next figure out which HRU it is in
            Catchment *pCatchment = catchments[ j ];
            ASSERT( pCatchment != NULL );

            // store this in the catchment layer
            ASSERT( m_colCatchmentCatchID >= 0 );
            m_pCatchmentLayer->SetData( polyIndex, m_colCatchmentCatchID, pCatchment->m_id );

            // does it satisfy an existing HRU for this catchment?
            HRU *pHRU = NULL;
            for ( int k=0; k < pCatchment->GetHRUCount(); k++ )
               {
               HRU *_pHRU = pCatchment->GetHRU( k );
              // _pHRU->m_hruValueArray.SetSize(hruCols);
               ASSERT( _pHRU != NULL );
               ASSERT( _pHRU->m_polyIndexArray.GetSize() > 0 );

               UINT hruPolyIndex = _pHRU->m_polyIndexArray[ 0 ];

               // does the poly being examined match this HRU?
               bool match = true;
               for ( int m=0; m < hruCols; m++ )
                  {
                  VData hruValue;
                  m_pCatchmentLayer->GetData( hruPolyIndex, m_colsHruAgg[ m ], hruValue );
                  int hruVal;
                  m_pCatchmentLayer->GetData( hruPolyIndex, m_colsHruAgg[ m ], hruVal );
                  
                  if ( hruValue != hruValueArray[ m ] )
                     {
                     match = false;
                     break;
                     }

                  }  // end of: for ( m < hruCols )

               if ( match )
                  {
                  pHRU = _pHRU;
                  break;
                  }
               }  // end of: for ( k < pCatchment->GetHRUCount() )

            if ( pHRU == NULL )  // not found in existing HRU's for this catchment?  Then add...
               {
               pHRU = new HRU;
               AddHRU( pHRU, pCatchment );
               
               pHRU->m_id = hruCount;
               pHRU->AddLayers( m_soilLayerCount, m_snowLayerCount, m_vegLayerCount, 0, m_initTemperature, false );
               hruCount++;
               }

            ASSERT( m_colCatchmentHruID >= 0 );
            m_pCatchmentLayer->SetData( polyIndex, m_colCatchmentHruID, pHRU->m_id );

            pHRU->m_polyIndexArray.Add( polyIndex );
            break;  // done processing queries for this polygon, since we found one that satisifies it, no need to look further
            }  // end of: if ( ok && result )  meaning adding polyIndex to catchment
         }  // end of: for ( j < queryCount )    
      }  // end of: for ( i < polyCount )

   // store catchments
   for ( int j=0; j < queryCount; j++ )
      {
      Catchment *pCatchment = catchments[ j ];

      if ( pCatchment != NULL )
         m_catchmentArray.Add( pCatchment );
      }

   // clean up
   
   delete [] hruValueArray;
   delete [] catchments;

   if ( m_pCatchmentQuery != NULL )
      m_pCatchmentLayer->ClearQueryResults();
   
   CString msg;
   msg.Format( _T("Flow: %i Catchments, %i HRUs built from %i polygons" ), (int) m_catchmentArray.GetSize(), hruCount, polyCount ); 
   Report::LogMsg( msg, RT_INFO );

   return true;
   }


// bool FlowModel::BuildCatchmentsFromLayer( void ) -----------------------------
//
// builds Catchments and HRU's based on HRU ID's contained in the catchment layer
// Note that this method does NOT connect catchments to reaches
//-------------------------------------------------------------------------------

bool FlowModel::BuildCatchmentsFromLayer( void )
   {
   if ( this->m_colCatchmentHruID < 0 || m_colCatchmentCatchID < 0 )
      return false;
   
   int polyCount = m_pCatchmentLayer->GetPolygonCount();

   if ( m_pCatchmentQuery != NULL )
      polyCount = m_pCatchmentQuery->Select( true );

   int catchID, hruID;
   int hruCount = 0;

   CMap< int, int, Catchment*, Catchment* > catchmentMap;

   // iterate though polygons that satisfy the catchment query.
   for ( int i=0; i < polyCount; i++ )
      {
      int polyIndex = m_pCatchmentQuery ? m_pCatchmentLayer->GetQueryResult( i ) : i;

      bool ok = m_pCatchmentLayer->GetData( polyIndex, m_colCatchmentCatchID, catchID );
      if ( ! ok )
         continue;

      ok = m_pCatchmentLayer->GetData( polyIndex, m_colCatchmentHruID, hruID );
      if ( ! ok )
         continue;

      Catchment *pCatchment = NULL;
      ok = catchmentMap.Lookup( catchID, pCatchment ) ? true : false;
      if ( ! ok )
         {
         pCatchment = AddCatchment( catchID, NULL );
         catchmentMap.SetAt( catchID, pCatchment );
         }

      // catchment covered, now do HRU's
      HRU *pHRU = NULL;
      for ( int j=0; j < pCatchment->GetHRUCount(); j++ )
         {
         HRU *_pHRU = pCatchment->GetHRU( j );

         if ( _pHRU->m_id == hruID )
            {
            pHRU = _pHRU;
            break;
            }
         }

      if ( pHRU == NULL )
         {
         pHRU = new HRU;
         AddHRU( pHRU, pCatchment );
        // m_hruArray.Add(pHRU);
         pHRU->AddLayers( m_soilLayerCount, m_snowLayerCount, m_vegLayerCount, 0, m_initTemperature, false );
         pHRU->m_id = hruCount;
         hruCount++;
         }

      pHRU->m_polyIndexArray.Add( polyIndex );
      }  // end of:  for ( i < polyCount )
      
   CString msg;
   msg.Format( _T("Flow: %i Catchments, %i HRUs loaded from %i polygons" ), (int) m_catchmentArray.GetSize(), hruCount, polyCount ); 
   Report::LogMsg( msg, RT_INFO );

   return true;
   }


int FlowModel::BuildCatchmentFromPolygons( Catchment *pCatchment, int polyArray[], int polyCount )
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
   int hruQueryCount = GenerateAggregateQueries( m_pCatchmentLayer, m_colsHruAgg, hruQueryArray );

   int hruCols = (int) m_colsHruAgg.GetSize();     // number of cols referenced in the queries
   VData *hruValueArray = new VData[ hruCols ];    // will store field values during queries

   int hruCount = 0;

   // iterate though polygons
   for ( int i=0; i < polyCount; i++ )
      {
      int polyIndex = polyArray[ i ];

      // get hru aggregate values from this poly
      for ( int h=0; h < hruCols; h++ )
         {
         bool ok = m_pCatchmentLayer->GetData( polyIndex, m_colsHruAgg[ h ], hruValueArray[ h ] );
         if ( ! ok )
            hruValueArray[ h ].SetNull();
         }

      // store the catchment ID in the poly 
      ASSERT( m_colCatchmentCatchID >= 0 );
      m_pCatchmentLayer->SetData( polyIndex, m_colCatchmentCatchID, pCatchment->m_id );

      // does it satisfy an existing HRU for this catchment?
      HRU *pHRU = NULL;
      for ( int k=0; k < pCatchment->GetHRUCount(); k++ )
         {
         HRU *_pHRU = pCatchment->GetHRU( k );
         // _pHRU->m_hruValueArray.SetSize(hruCols);
         ASSERT( _pHRU != NULL );
         ASSERT( _pHRU->m_polyIndexArray.GetSize() > 0 );         
         UINT hruPolyIndex = _pHRU->m_polyIndexArray[ 0 ];

         // does the poly being examined match this HRU?
         bool match = true;
         for ( int m=0; m < hruCols; m++ )
            {
            VData hruValue;
            m_pCatchmentLayer->GetData( hruPolyIndex, m_colsHruAgg[ m ], hruValue );
            int hruVal;
            m_pCatchmentLayer->GetData( hruPolyIndex, m_colsHruAgg[ m ], hruVal );
            
            if ( hruValue != hruValueArray[ m ] )
               {
               match = false;
               break;
               }
            }  // end of: for ( m < hruCols )

         if ( match )
            {
            pHRU = _pHRU;
            break;
            }
         }  // end of: for ( k < pCatchment->GetHRUCount() )

      if ( pHRU == NULL )  // not found in existing HRU's for this catchment?  Then add...
         {
         pHRU = new HRU;
         AddHRU( pHRU, pCatchment );

         pHRU->m_id = 10000+m_numHRUs;
         m_numHRUs++;
         pHRU->AddLayers( m_soilLayerCount, m_snowLayerCount, m_vegLayerCount, 0, m_initTemperature, false );
         hruCount++;
         }

     ASSERT( m_colCatchmentHruID >= 0 );
     m_pCatchmentLayer->SetData( polyIndex, m_colCatchmentHruID, pHRU->m_id );

     pHRU->m_polyIndexArray.Add( polyIndex );
     }  // end of: for ( i < polyCount )
   
   // update hru info
   SetHRUAttributes( pCatchment );

   // clean up
   delete [] hruValueArray;
   
   //CString msg;
   //msg.Format( _T("Flow: Rebuild: %i HRUs built from %i polygons" ), hruCount, polyCount ); 
   //Report::LogMsg( msg, RT_INFO );
   return hruCount;
   }



int FlowModel::ParseCols( MapLayer *pLayer /*in*/, CString &aggCols /*in*/, CUIntArray &colsAgg /*out*/ )
   {
   // parse columns
   TCHAR *buffer = new TCHAR[ aggCols.GetLength()+1 ];
   TCHAR *nextToken = NULL;
   lstrcpy( buffer, (LPCTSTR) aggCols );

   TCHAR *col = _tcstok_s( buffer, ",", &nextToken );

   int colCount = 0;

   while( col != NULL )
      {
      int _col = pLayer->GetFieldCol( col );

      if ( _col < 0 )
         {
         CString msg( "Flow: Aggregation column " );
         msg += col;
         msg += " not found - it will be ignored";
         Report::ErrorMsg( msg );
         }
      else
         {
         colsAgg.Add( _col );
         colCount++;
         }

      col = _tcstok_s( NULL, ",", &nextToken );

      }

   delete [] buffer;

   return colCount;
   }



int FlowModel::GenerateAggregateQueries( MapLayer *pLayer /*in*/, CUIntArray &colsAgg /*in*/, CArray< Query*, Query* > &queryArray /*out*/ )
   {
   queryArray.RemoveAll();

   int colCount = (int) colsAgg.GetSize();

   if ( colCount == 0 )    // no column specified, no queries generatated
      return 0;

   // have columns, figure possible queries
   // for each column, get all possible values of the attributes in that column
   PtrArray< CStringArray > colStrArrays;  // one entry for each column, each entry has unique field values

   for ( int i=0; i < colCount; i++ )
      {
      CStringArray *attrValArray = new CStringArray;
      if ( pLayer->GetUniqueValues( colsAgg[ i ], *attrValArray ) > 0 )
         colStrArrays.Add( attrValArray );
     else
        delete attrValArray;

      }

   // now generate queries.  ASSUME AT MOST TWO COLUMNS FOR NOW!!!!  this needs to be improved!!!
   ASSERT( colCount <= 2 );

   if ( colCount == 1 )
      {
      CStringArray *attrValArray = colStrArrays[ 0 ];

      LPCTSTR colName = pLayer->GetFieldLabel( colsAgg[ 0 ] );

      for ( int j=0; j < attrValArray->GetSize(); j++ )
         {
         CString queryStr;
         queryStr.Format( "%s = %s", colName, (LPCTSTR) attrValArray->GetAt( j ) );
         /*
         if ( m_catchmentQuery.GetLength() > 0 )
            {
            queryStr += _T(" and ");
            queryStr += m_catchmentQuery;
            } */        

         ASSERT( m_pCatchmentQE != NULL );
         Query *pQuery = m_pCatchmentQE->ParseQuery( queryStr, 0, "Flow Aggregate Query" );

         if ( pQuery == NULL )
            {
            CString msg( "Flow: Error parsing Aggregation query: '" );
            msg += queryStr;
            msg += "' - the query will be ignored";
            Report::ErrorMsg( msg );
            }
         else
            queryArray.Add( pQuery );
         }
      }

   else if ( colCount == 2 )
      {
      CStringArray *attrValArray0 = colStrArrays[ 0 ];
      CStringArray *attrValArray1 = colStrArrays[ 1 ];

      LPCTSTR colName0 = pLayer->GetFieldLabel( colsAgg[ 0 ] );
      LPCTSTR colName1 = pLayer->GetFieldLabel( colsAgg[ 1 ] );

      for ( int j=0; j < attrValArray0->GetSize(); j++ )
         {
         for ( int i=0; i < attrValArray1->GetSize(); i++ )
            {
            CString queryStr;
            queryStr.Format( "%s = %s and %s = %s", colName0, (LPCTSTR) attrValArray0->GetAt( j ), colName1, (LPCTSTR) attrValArray1->GetAt( i )  );
            /*
            if ( m_catchmentQuery.GetLength() > 0 )
               {
               queryStr += _T(" and ");
               queryStr += m_catchmentQuery;
               } */         

            ASSERT( m_pCatchmentQE != NULL );
            Query *pQuery = m_pCatchmentQE->ParseQuery( queryStr, 0, "Flow Aggregate Query" );

            if ( pQuery == NULL )
               {
               CString msg( "Flow: Error parsing Aggregation query: '" );
               msg += queryStr;
               msg += "' - the query will be ignored";
               Report::ErrorMsg( msg );
               }
            else
               queryArray.Add( pQuery );
            }
         }
      }

   return (int) queryArray.GetSize();
   }



bool FlowModel::ConnectCatchmentsToReaches( void )
   {
   // This method determines which reach, if any, each catchment is connected to.  It does this by
   // iterate through the catchments, relating each catchment with it's associated reach.  We assume that
   // each polygon has a field indicated by the catchment join col that contains the index of the 
   // associated stream reach.

   INT_PTR catchmentCount = m_catchmentArray.GetSize();
   INT_PTR reachCount = m_reachArray.GetSize();

   int catchmentJoinID = 0;

   int unconnectedCatchmentCount = 0;
   int unconnectedReachCount     = 0;
   //int reRoutedCatchmentCount    = 0;
   //int reRoutedAndLost           = 0;

   // iterate through each catchment, getting the associated field value that
   // maps into the stream coverage
   for ( INT_PTR i=0; i < catchmentCount; i++ ) 
      {
      Catchment *pCatchment = m_catchmentArray[ i ];
      pCatchment->m_pReach = NULL;

      // have catchment, use the IDU associated with the first HRU in catchment to get
      // the value stored in the catchment join col in the catchment layer.  This is a lookup
      // key - if the corresponding value is found in the reach join col in the stream coverage,
      // we connect this catchment to the reach with the matching join ID
    //  ASSERT( pCatchment->GetHRUCount() > 0 );

     if (pCatchment->GetHRUCount() > 0)
        {
        HRU *pHRU = pCatchment->GetHRU( 0 );
        m_pCatchmentLayer->GetData( pHRU->m_polyIndexArray[ 0 ], m_colCatchmentJoin, catchmentJoinID );

        // look for this catchID in the reaches
        for ( INT_PTR j=0; j < reachCount; j++ )
          {
          Reach *pReach = m_reachArray[ j ];

          int streamJoinID = -1;
          m_pStreamLayer->GetData( pReach->m_polyIndex, this->m_colStreamJoin, streamJoinID );

          // try to join the stream and catchment join cols
          if ( streamJoinID == catchmentJoinID )       // join col values match?
            {
            pCatchment->m_pReach = pReach;               // indicate that the catchment is connected to this reach
            pReach->m_catchmentArray.Add( pCatchment );  // add this catchment to the reach's catchment array
            break;
            }
          }

        if ( pCatchment->m_pReach == NULL )
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

   for ( INT_PTR j=0; j < reachCount; j++ )
      {
      Reach *pReach = m_reachArray[ j ];

      if ( pReach->m_catchmentArray.GetSize() > 1 )
         Report::LogMsg( "Flow: More than one catchment attached to a reach!" );

      if ( pReach->m_catchmentArray.GetSize() == 0 )
         unconnectedReachCount++;
      }

   // populate the stream layers "CATCHID" field with the reach's catchment ID (if any)
   ////////////////////////////////////////  Is this really needed??? JPB 2/13
   int colCatchmentID = -1;
   gpFlow->CheckCol( this->m_pStreamLayer, colCatchmentID, "CATCHID", TYPE_INT, CC_AUTOADD );
   
   for ( INT_PTR j=0; j < reachCount; j++ )
      {
      Reach *pReach = m_reachArray[ j ];
      if ( pReach->m_catchmentArray.GetSize() > 0 )
         {
         Catchment *pCatchment = pReach->m_catchmentArray[ 0 ];
         m_pStreamLayer->SetData( pReach->m_polyIndex, colCatchmentID, pCatchment->m_id );
         }
      }
   ///////////////////////////////////////

   CString msg;
   msg.Format( _T("Flow: Topology established for %i catchments and %i reaches.  %i catchments are not connected to any reach, %i reaches are not connected to any catchments. "), (int) m_catchmentArray.GetSize(), (int) m_reachArray.GetSize(), unconnectedCatchmentCount, unconnectedReachCount );
   Report::LogMsg( msg );
   return true;
   }


bool FlowModel::InitReservoirs( void )
   {
   if ( m_colResID < 0 )
      return false;

   // iterate through reach network, looking for reservoir ID's...
   int reachCount = m_pStreamLayer->GetRecordCount();

   for ( int i=0; i < reachCount; i++ )
      {
      int resID = -1;
      m_pStreamLayer->GetData( i, m_colResID, resID );

      if ( resID > 0 )
         {
         Reservoir *pRes = FindReservoirFromID( resID );
      
         if ( pRes == NULL )
            {
            CString msg;
            msg.Format( "Flow: Unable to find reservoir ID '%i' in InitRes().", resID );
            Report::StatusMsg( msg );
            }
         else
            {
            if ( pRes->m_pReach == NULL )
               {
               Reach *pReach = (Reach*) m_reachTree.GetReachNodeFromPolyIndex( i );

               if ( pReach != NULL )
                  {
                  pRes->m_pReach = pReach;
                  pReach->m_pReservoir = pRes;
                  }
               else
                  {
                  CString msg;
                  msg.Format( "Flow:  Unable to locate reservoir (id:%i) in stream network", pRes->m_id );
                  Report::StatusMsg( msg );
                  }
               }  // end of: pRes->m_pReach
            }  // end of : else
         }  // end of: if ( resID > 0 )
      }  // end of: for each Reach


   /*///////////////////////////////////////////////////////////////////////////////////////////////////////////
   //Code here to load ResSIM inflows for testing...to be removed when discharge comes from FLOW  mmc 11_7_2012
   m_pResInflows = new FDataObj(0, 0 );
   LPCSTR ResSimInflows = "C:\\envision\\StudyAreas\\WW2100\\Reservoirs\\Inflows_from_ResSim\\ResSim_inflows_for_testing_cms.csv";
   
   m_pResInflows->ReadAscii( ResSimInflows );
   /*m_pCpInflows = new FDataObj(0, 0);
   LPCSTR CpLocalFlows = "C:\\envision\\StudyAreas\\WW2100\\Reservoirs\\Inflows_from_ResSim\\Control_point_local_flows_cms.csv";
   
   m_pCpInflows->ReadAscii( CpLocalFlows );
   ///////////////////////////////////////////////////////////////////////////////////////////////////////////*/
   
   // for each reservoir, follow downstream until the first reach downstream from
   // the reaches associated with the reservoir is found
   int resCount = (int) m_reservoirArray.GetSize();
   int usedResCount = 0;

   for ( int i=0; i < resCount; i++ )
      {
      // for each reservoir, move the Reach pointer downstream
      // until a different resID found
      Reservoir *pRes = m_reservoirArray[ i ];
      ASSERT( pRes != NULL );

      Reach *pReach = pRes->m_pReach;   // initially , this points to a reach "in" the reservoir, if any
      
		if ( pReach != NULL )   // any associated reachs?
		{
			ReachNode *pNode = pReach;

			// traverse down until this node's resID != this one
			while ( pNode != NULL )
			{
				ReachNode *pDown = pNode->GetDownstreamNode( true );

				if ( pDown == NULL )  // root of tree?
					break;

				int idu = pDown->m_polyIndex;
				int resID = -1;
				m_pStreamLayer->GetData( idu, m_colResID, resID );

				// do we see no reservoir or a different reservoir? then quit the loop
				pNode = pDown;
				if ( resID != pRes->m_id )
					break;
			}

			// at this point, pNode points to the last matching downstream reach.
			// fix up the pointers
			if ( pNode != NULL )
			{
				Reach *pDownstreamReach = GetReachFromNode( pNode );
				pRes->m_pReach->m_pReservoir = NULL;
				pRes->m_pReach = pDownstreamReach;
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



				LoadTable(gpModel->m_path + pRes->m_dir + pRes->m_avdir + pRes->m_areaVolCurveFilename, (DataObj**)&(pRes->m_pAreaVolCurveTable), 0);

				// For all Reservoir Types but ControlPointControl
				if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_RiverRun)
				{
					LoadTable(gpModel->m_path + pRes->m_dir + pRes->m_redir + pRes->m_releaseCapacityFilename, (DataObj**)&(pRes->m_pCompositeReleaseCapacityTable), 0);
					LoadTable(gpModel->m_path + pRes->m_dir + pRes->m_redir + pRes->m_RO_CapacityFilename, (DataObj**)&(pRes->m_pRO_releaseCapacityTable), 0);
					LoadTable(gpModel->m_path + pRes->m_dir + pRes->m_redir + pRes->m_spillwayCapacityFilename, (DataObj**)&(pRes->m_pSpillwayReleaseCapacityTable), 0);
				}

				// Not Needed for RiverRun Reservoirs
				if (pRes->m_reservoirType == ResType_FloodControl || pRes->m_reservoirType == ResType_CtrlPointControl)
				{
					LoadTable(gpModel->m_path + pRes->m_dir + pRes->m_rpdir + pRes->m_rulePriorityFilename, (DataObj**)&(pRes->m_pRulePriorityTable), 1);
				
					if (pRes->m_reservoirType == ResType_FloodControl)
					{
						LoadTable(gpModel->m_path + pRes->m_dir + pRes->m_rcdir + pRes->m_ruleCurveFilename, (DataObj**)&(pRes->m_pRuleCurveTable), 0);
						LoadTable(gpModel->m_path + pRes->m_dir + pRes->m_rcdir + pRes->m_bufferZoneFilename, (DataObj**)&(pRes->m_pBufferZoneTable), 0);

						//Load table with ResSIM outputs
						//LoadTable( pRes->m_dir+"Output_from_ResSIM\\"+pRes->m_ressimFlowOutputFilename,    (DataObj**) &(pRes->m_pResSimFlowOutput),    0 );
						LoadTable(pRes->m_dir + "Output_from_ResSIM\\" + pRes->m_ressimRuleOutputFilename, (DataObj**)&(pRes->m_pResSimRuleOutput), 1);
					}
					pRes->ProcessRulePriorityTable();
				}

				
			}
		} // end of: if ( pReach != NULL )

      pRes->InitDataCollection();    // set up data object and add as output variable      
      }  // end of: for each Reservoir

   //if ( usedResCount > 0 )
   //   this->AddReservoirLayer();
   
   CString msg;
   msg.Format( "Flow: Loaded %i reservoirs (%i were disconnected from the reach network and ignored)", usedResCount, resCount-usedResCount );
   Report::LogMsg( msg );
   
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

int FlowModel::SimplifyNetwork( void )
   {
   if ( m_pStreamQuery == NULL )
      return -1;

   int startReachCount = (int) m_reachArray.GetSize(); 
   int startCatchmentCount = (int) m_catchmentArray.GetSize();
   int startHRUCount=0;
   
   for ( int i=0; i < (int) m_catchmentArray.GetSize(); i++ )
      startHRUCount += m_catchmentArray[i]->GetHRUCount();

   CString msg( "Flow: Simplifying Catchments/Reaches: " );

   if ( this->m_streamQuery.GetLength() > 0 )
      {
      msg += "Stream Query: '";
      msg += this->m_streamQuery;
      msg += "' ";
      }

   if ( this->m_minCatchmentArea > 0 )
      {
      CString _msg;
      _msg.Format( "Min Catchment Area: %.1g ha, Max Catchment Area: %.1g ha", (float) this->m_minCatchmentArea/M2_PER_HA, (float) this->m_maxCatchmentArea/M2_PER_HA );
      msg += _msg;
      }

   Report::LogMsg( msg );

   // iterate through the roots of the tree, applying catchment constraints
   try 
      {
      // now, starting with the leaves, move down stream, looking for excluded stream reaches
      for ( int i=0; i < m_reachTree.GetRootCount(); i++ )
         {
         ReachNode *pRoot = m_reachTree.GetRootNode( i );

         // algorithm:  start with the root.  Simplify the left side, then the right side,
         // recursively with the following rule:
         //   if the current reachnode satifies the query, recurse
         //   otherwise, relocate the HRUs from everything above and including it to the downstream
         //   reach node's catchment

         if ( this->m_streamQuery.GetLength() > 0 )
            {
            int catchmentCount = (int) m_catchmentArray.GetSize();

            SimplifyTree( pRoot );   // prune leaves based on reach query

            int newCatchmentCount = (int) m_catchmentArray.GetSize();
            
            CString msg;
            msg.Format( "Flow: Pruned leaves for root %i, %i Catchments reduced to %i (%i combined)", i, catchmentCount, newCatchmentCount, (catchmentCount-newCatchmentCount) );
            Report::LogMsg( msg );
            }

         if ( this->m_minCatchmentArea > 0 && this->m_maxCatchmentArea > 0 )
            {
            int catchmentCount = (int) m_catchmentArray.GetSize();

            ApplyCatchmentConstraints( pRoot );              // apply min area constraints
            
            int newCatchmentCount = (int) m_catchmentArray.GetSize();
   
            CString msg;
            msg.Format( "Flow: applying area constraints for root %i, %i Catchments reduced to %i (%i combined)", i, catchmentCount, newCatchmentCount, (catchmentCount-newCatchmentCount) );
            Report::LogMsg( msg );
            }         
         }
      }

   catch( CException *ex) // Catch all other CException derived exceptions 
      {
      TCHAR info[300]; 
      if ( ex->GetErrorMessage( info, 300 ) == TRUE )
         {
         CString msg( "Exception thrown in Flow during SimplifyTree() " );
         msg += info;
         Report::ErrorMsg( msg );
         }
      ex->Delete(); // Will delete exception object correctly 
      } 
   catch(...) 
      { 
      CString msg( "Unknown Exception thrown in Flow during SimplifyTree() " );
      Report::ErrorMsg( msg );
      }

   // update catchment/reach cumulative drainage areas
   // this->PopulateCatchmentCumulativeAreas();

   int finalReachCount = (int) this->m_reachArray.GetSize();
   int finalHRUCount=0;

   // update catchID and join cols to reflect the simplified network
   m_pCatchmentLayer->SetColData( this->m_colCatchmentJoin, VData( -1 ), true );
   m_pStreamLayer->SetColData( this->m_colStreamJoin, VData( -1 ), true );
   
   float areaTotal = 0.0f;
   
   for ( int i=0; i < (int) m_catchmentArray.GetSize(); i++ )
      {
      Catchment *pCatchment = m_catchmentArray[ i ];
      finalHRUCount += pCatchment->GetHRUCount();

      for ( int j=0; j < pCatchment->GetHRUCount(); j++ )
         {
         HRU *pHRU = pCatchment->GetHRU( j );
         ASSERT( pHRU != NULL );
         areaTotal += pHRU->m_area;

         for ( int k=0; k < (int) pHRU->m_polyIndexArray.GetSize(); k++ )
            {
            // write new catchment ID to catchmentID col and catchment join col 
            m_pCatchmentLayer->SetData( pHRU->m_polyIndexArray[ k ], m_colCatchmentCatchID, pCatchment->m_id );
            m_pCatchmentLayer->SetData( pHRU->m_polyIndexArray[ k ], m_colCatchmentJoin, pCatchment->m_id );
            }
         }
      }

   // update stream join values
   for ( int i=0; i < this->m_reachArray.GetSize(); i++ )
      {
      Reach *pReach = m_reachArray[ i ];

      if ( pReach->m_catchmentArray.GetSize() > 0 )
         m_pStreamLayer->SetData( pReach->m_polyIndex, this->m_colStreamJoin, pReach->m_catchmentArray[0]->m_id );
      }
   
   msg.Format( "Flow: Reach Network Simplified from %i to %i Reaches, %i to %i Catchments and %i to %i HRUs", 
   startReachCount, finalReachCount, startCatchmentCount, (int) m_catchmentArray.GetSize(), startHRUCount, finalHRUCount);
   Report::LogMsg( msg );

   return startCatchmentCount - ((int) m_catchmentArray.GetSize() );
   }


// returns number of reaches in the simplified tree
int FlowModel::SimplifyTree( ReachNode *pNode )
   {
   if ( pNode == NULL )
      return 0;

   // skip phantom nodes - they have no associated catchments (they always succeed)
   if ( pNode->IsPhantomNode() || pNode->IsRootNode() )
      {
      int leftCount  = SimplifyTree( pNode->m_pLeft );
      int rightCount = SimplifyTree( pNode->m_pRight );
      return leftCount + rightCount;
      }

   // does this node satisfy the query?
   ASSERT( pNode->m_polyIndex >= 0 );
   bool result = false;
   bool ok = m_pStreamQE->Run( m_pStreamQuery, pNode->m_polyIndex, result );
   Reach *pReach = GetReachFromNode( pNode );
   if ( (ok && result) || pReach->m_isMeasured)
      {
      // it does, so we keep this one and move on to the upstream nodes
      int leftCount  = SimplifyTree( pNode->m_pLeft );
      int rightCount = SimplifyTree( pNode->m_pRight );    
      return 1 + leftCount + rightCount;
      }  

   else  // it fails, so this reach and all reaches of above it need to be rerouted to the reach below us
      {  // relocate this and upstream reaches to this nodes down stream pointer
      if ( pNode->IsRootNode() )    // if this is a root node, nothing required, the whole
         return 0;                  // tree will be deleted  NOTE: NOT IMPLEMENTED YET
      else
         {
         ReachNode *pDownNode = pNode->GetDownstreamNode( true );  // skips phantoms
         Reach *pDownReach = GetReachFromNode( pDownNode );
         if ( pDownReach == NULL  )
            {
            CString msg;
            msg.Format( "Flow: Warning - phantom root node encountered below Reach ID %i. Truncation can not proceed for this tree...", pNode->m_reachID );
            Report::WarningMsg( msg );
            }
         else
            {
            int beforeCatchCount = (int) this->m_catchmentArray.GetSize();
            MoveCatchments( pDownReach, pNode, true );   // moves all catchments for this and above (recursively)
            int afterCatchCount = (int) this->m_catchmentArray.GetSize();

            //CString msg;
            //msg.Format( "Flow: Pruning %i catchments that don't satisfy the Simplification query", (beforeCatchCount-afterCatchCount) );
            //Report::LogMsg( msg );

            RemoveReaches( pNode );   // removes all Reach and ReachNodes above and including this ReachNode
            }

         return 0;
         }
      }
   }


int FlowModel::ApplyCatchmentConstraints( ReachNode *pRoot )
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

   while ( removeCount > 0 && pass < 5 )
      {
      removeCount = 0;
      for ( int i=0; i < (int) m_reachArray.GetSize(); i++ )
         {
         Reach *pReach = m_reachArray[ i ];
         int removed = ApplyAreaConstraints( pReach ); 

         // if this reach was removed, then move i back one to compensate
        // i -= removed;

         removeCount += removed;
         }

      pass++;

      CString msg;
      msg.Format( "Flow: Applying catchment constraints (pass %i) - %i catchments consolidated", pass, removeCount );

      Report::LogMsg( msg );
      }

   return pass;
   }


// return 0 if no change, 1 if Reach is altered
int FlowModel::ApplyAreaConstraints( ReachNode *pNode )
   {
   // this method examines the current reach.  if the contributing area of the reach is too small, try to combine it with another catchments
   // Basic rule is:
   // if the catchment for this reach is too small, combine it with another
   // reach using the following rules:
   //    1) if one of the upstream catchment is below the minimum size, add this reaches
   //       catchment to the upstream reach.  (Note that this works best with a root to leaf search)
   //    2) if there is no upstream capacity, combine this reach with the downstream reach (note
   //       that this will work best with a leaf to root, breadth first search.)
   if ( pNode == NULL )   
      return 0;

   Reach *pReach = GetReachFromNode( pNode );

   if ( pReach == NULL )      // no reach exist no action required
      return 0;

   // does this Catchment  satisfy the min area constraint?  If so, we are done.
   float catchArea = pReach->GetCatchmentArea();

   if ( catchArea > m_minCatchmentArea )
      return 0;

   // catchment area for this reach is too small, combine it.
   // Step one, look above, left 
   ReachNode *pLeft = pNode->m_pLeft;
   while ( pLeft != NULL && GetReachFromNode( pLeft ) == NULL )
      pLeft = pLeft->m_pLeft;

   // does a left reach exist?  if so, check area and combine if minimum not satisfied
   if ( pLeft != NULL )
      {
      Reach *pLeftReach = GetReachFromNode( pLeft );
      ASSERT(  pLeftReach != NULL );
      if (!pLeftReach->m_isMeasured)
         {
         // is there available capacity upstream?
         if ( ( pLeftReach->GetCatchmentArea() + catchArea ) <= m_maxCatchmentArea)
            {
            MoveCatchments( pReach, pLeft, false );   // move the upstream reach to this one, no recursion
           // RemoveReach( pLeft ); //leave reaches in the network  KBV
            RemoveCatchment(pLeft);
            return 1;
            }
         }
      }
   
   // look above, right
   ReachNode *pRight = pNode->m_pRight;
   while ( pRight != NULL && GetReachFromNode( pRight ) == NULL )
      pRight = pRight->m_pRight;

   // does a right reach exist?  if so, check area and combine if minimum not satisfied
   if ( pRight != NULL )
      {
      Reach *pRightReach = GetReachFromNode( pRight );
      ASSERT(  pRightReach != NULL );
      if (!pRightReach->m_isMeasured)
         {
         // is there available capacity upstream?
         if ( ( pRightReach->GetCatchmentArea() + catchArea ) <= m_maxCatchmentArea )
            {
            MoveCatchments( pReach, pRight, false );   // move the upstream reach to this one, no recursion
           // RemoveReach( pRight );  //leave reaches in the network KBV
            RemoveCatchment(pRight);
            return 1;
            }
         }
      }

   // no capacity upstream, how about downstream?
   ReachNode *pDown = pNode->m_pDown;
   while ( pDown != NULL && GetReachFromNode( pDown ) == NULL )
      pDown = pDown->m_pDown;

   // does a down reach exist?  if so, check area and combine if minimum not satisfied
   if ( pDown != NULL && pReach->m_isMeasured==false )
      {
      Reach *pDownReach = GetReachFromNode( pDown );
      Reach *pReach = GetReachFromNode( pNode );
      ASSERT(  pDownReach != NULL );

      // is there available capacity upstream?
    //  if ( ( pDownReach->m_cumUpstreamArea + catchArea ) <= m_maxCatchmentArea )
     
      if (( pDownReach->GetCatchmentArea() + catchArea ) <= m_maxCatchmentArea )
         {
         MoveCatchments( pDownReach, pNode, false );   // move from this reach to the downstream reach
         //RemoveReach( pNode ); //leave reaches in the network  KBV
         RemoveCatchment(pNode);
         return 1;
         }
      }

   // else, no capacity anywhere adjacent to this reach, so give up
   return 0;
   }





float FlowModel::PopulateCatchmentCumulativeAreas( void )
   {
   float area = 0;
   for ( int i=0; i < m_reachTree.GetRootCount(); i++ )
      {
      ReachNode *pRoot = m_reachTree.GetRootNode( i );
      area += PopulateCatchmentCumulativeAreas( pRoot );
      }

   return area;
   }


// updates internal variables and (if columns are defined) the stream and catchment
// coverages with cumulative values.  Assumes internal catchment areas have already been 
// determined.

float FlowModel::PopulateCatchmentCumulativeAreas( ReachNode *pNode )
   {
   if ( pNode == NULL )
      return 0;

   float leftArea  = PopulateCatchmentCumulativeAreas( pNode->m_pLeft );
   float rightArea = PopulateCatchmentCumulativeAreas( pNode->m_pRight );

   Reach *pReach = GetReachFromNode( pNode );

   if ( pReach != NULL )
      {
      float area = 0;

      for ( int i=0; i < (int) pReach->m_catchmentArray.GetSize(); i++ )
         area += pReach->m_catchmentArray[ i ]->m_area;

      area += leftArea;
      area += rightArea;

      // write to coverages
      if ( m_colStreamCumArea >= 0 )
         m_pStreamLayer->SetData( pReach->m_polyIndex, m_colStreamCumArea, area );

      if ( m_colCatchmentCumArea >= 0 )
         {
         for ( int i=0; i < (int) pReach->m_catchmentArray.GetSize(); i++ )
            {
            Catchment *pCatch = pReach->m_catchmentArray[ i ];
            SetCatchmentData( pCatch, m_colCatchmentCumArea, area );
            }
         }

      pReach->m_cumUpstreamArea = area;
      return area;
      }

   return leftArea + rightArea;
   }



// FlowModel::MoveCatchments() takes the catchment HRUs associated wih the "from" node and moves them to the
//    "to" reach, and optionally repeats recursively up the network.
int FlowModel::MoveCatchments( Reach *pToReach, ReachNode *pFromNode, bool recurse )
   {
   if ( pFromNode == NULL || pToReach == NULL )
      return 0;

   if ( pFromNode->IsPhantomNode() )   // for phantom nodes, always recurse up
      {
      int leftCount  = MoveCatchments( pToReach, pFromNode->m_pLeft, recurse  );
      int rightCount = MoveCatchments( pToReach, pFromNode->m_pRight, recurse );
      return leftCount + rightCount;
      }
   
   Reach *pFromReach = GetReachFromNode( pFromNode );

   int fromCatchmentCount = (int) pFromReach->m_catchmentArray.GetSize();

   // SPECIAL CASE:  fromCatchmentCount = 0, then there are no catchments in the
   // upstream reach, so just rec

   // if the from reach is measured, we don't combine it's catchments.
   // or, if the from reach has no catchments, there isn't anything to do either
   if ( pFromReach->m_isMeasured == false && fromCatchmentCount > 0 )
      {
      // get the Catchments associated with this reach and combine them into the
      // first catchment in the "to" reach catchment list
      int toCatchmentCount = (int) pToReach->m_catchmentArray.GetSize();
   
      Catchment *pToCatchment = NULL;
      
      if ( toCatchmentCount == 0 )//KELLIE
         {
         m_numReachesNoCatchments++;
         pToCatchment = AddCatchment( 10000+m_numReachesNoCatchments, pToReach );
         }
      else
         pToCatchment = pToReach->m_catchmentArray[ 0 ];

      ASSERT( pToCatchment != NULL );

      for ( int i=0; i < fromCatchmentCount; i++ ) 
         {
         Catchment *pFromCatchment = pFromReach->m_catchmentArray.GetAt( i );
      
         pToReach->m_cumUpstreamArea   = pFromReach->m_cumUpstreamArea + pToReach->GetCatchmentArea();
         pToReach->m_cumUpstreamLength = pFromReach->m_cumUpstreamLength + pToReach->m_length;

         CombineCatchments( pToCatchment, pFromCatchment, true );   // deletes the original catchment
         }
      
      // add the from reach length to the to reach.  Note: this assumes the from reach will removed from Flow
      // pToReach->m_length += pFromReach->m_length;     // JPB KBV

      // remove dangling Catchment pointers from the From reach
      pFromReach->m_catchmentArray.RemoveAll();       // only ptrs deleted, not the catchments
      }
      
   // recurse if specified
   if ( recurse && !pFromReach->m_isMeasured) // we don't want to recurse farther if we have find a reach for which we are saving model output for comparison with measurements
      {
      ReachNode *pLeft  = pFromNode->m_pLeft;
      ReachNode *pRight = pFromNode->m_pRight;

      int leftCount  = MoveCatchments( pToReach, pLeft,  recurse );
      int rightCount = MoveCatchments( pToReach, pRight, recurse );

      return fromCatchmentCount + leftCount + rightCount;
      }
   else if ( recurse && pFromReach->m_isMeasured) // so skip the measured reach, and continue recursion above it
      {
      ReachNode *pLeft  = pFromNode->m_pLeft;
      ReachNode *pRight = pFromNode->m_pRight;

      int leftCount  = MoveCatchments( pFromReach, pLeft,  recurse );
      int rightCount = MoveCatchments( pFromReach, pRight, recurse );

      return fromCatchmentCount + leftCount + rightCount;
      }
   else
      return fromCatchmentCount;
   }


// Combines the two catchments into the target catchment, copying/combining HRUs in the process.
// Depending on the deleteSource flag, the source catchment is optionally deleted

int FlowModel::CombineCatchments( Catchment *pTargetCatchment, Catchment *pSourceCatchment, bool deleteSource )
   {
   // get list of polygons to move from soure to target
   int hruCount = pSourceCatchment->GetHRUCount();

   CArray< int > polyArray;

   for ( int i=0; i < hruCount; i++ )
      {
      HRU *pHRU = pSourceCatchment->GetHRU( i );
      
      for ( int j=0; j < pHRU->m_polyIndexArray.GetSize(); j++ )
         polyArray.Add( pHRU->m_polyIndexArray[ j ] ); 
      }

   // have polygon list built, generate/add to target catchments HRUs
   BuildCatchmentFromPolygons( pTargetCatchment, polyArray.GetData(), (int) polyArray.GetSize() );
   
 //  pTargetCatchment->m_area += pSourceCatchment->m_area;

   // remove from source
   //pSourceCatchment->RemoveAllHRUs();     // this deletes the HRUs
   RemoveCatchmentHRUs( pSourceCatchment );
   
   if ( deleteSource )
      {
      // remove from global list and delete catchment
      for( int i=0; i < (int) m_catchmentArray.GetSize(); i++ )
         {
         if ( m_catchmentArray[ i ] == pSourceCatchment )
            {
            m_catchmentArray.RemoveAt( i );
            break;
            }     
         }
      }

   return (int) polyArray.GetSize();
   }


int FlowModel::RemoveCatchmentHRUs( Catchment *pCatchment )
   {
   int count = pCatchment->GetHRUCount();
   // first, remove from FlowModel
   for ( int i=0; i < count; i++ )
      {
      HRU *pHRU = pCatchment->GetHRU( i );
      this->m_hruArray.Remove( pHRU );    // deletes the HHRU
      }

   // next, remove pointers from catchment
   pCatchment->m_hruArray.RemoveAll();
   pCatchment->m_area = 0;

   return count;
   }




// removes all reaches and reachnodes, starting with the passed in node
int FlowModel::RemoveReaches( ReachNode *pNode )
   {
   if ( pNode == NULL )
      return 0;

   int removed = 0;

   if ( pNode->IsPhantomNode() == false )
      {
      if ( m_colStreamOrder >= 0 )
         {
         int order = 0;
         m_pStreamLayer->GetData( pNode->m_polyIndex, m_colStreamOrder, order );
         m_pStreamLayer->SetData( pNode->m_polyIndex, m_colStreamOrder, -order );
         }

      Reach *pReach = GetReachFromNode( pNode );
      removed++;

      // remove from internal list
      for ( int i=0; i < (int) m_reachArray.GetSize(); i++ )
         {
         if ( m_reachArray[ i ] == pReach )
            {
            m_reachArray.RemoveAt( i );    // deletes the Reach
            break;
            }
         }
      }

   // remove from reach tree (note this does not delete the ReachNode, it turns it into a phantom node
   m_reachTree.RemoveNode( pNode, false );    // don't delete upstream nodes, this will occur below

   // recurse
   int leftCount  = RemoveReaches( pNode->m_pLeft );
   int rightCount = RemoveReaches( pNode->m_pRight );

   return removed + leftCount + rightCount;
   }


bool FlowModel::RemoveCatchment( ReachNode *pNode )
   {
   Reach *pReach = GetReachFromNode( pNode ); 

   if ( pReach == NULL )    // NULL, phantom, or no associated Reach
      return 0;

   if ( pReach->IsPhantomNode() )
      return 0;

   if ( pReach->IsRootNode() )
      return 0;

   // delete any associated catchments
   for ( int i=0; i < (int) pReach->m_catchmentArray.GetSize(); i++ )
      {
      Catchment *pCatchment = pReach->m_catchmentArray[ i ];

      for ( int j=0; j < (int) m_catchmentArray.GetSize(); j++ )
         {
         if ( m_catchmentArray[ i ] == pCatchment )
            {  
            m_catchmentArray.RemoveAt( i );   // deletes the catchment
            break;
            }
         }
      }
   return true;
   }

// removes this reach and reachnode, converting the node into a phantom node
bool FlowModel::RemoveReach( ReachNode *pNode )
   {
   Reach *pReach = GetReachFromNode( pNode ); 

   if ( pReach == NULL )    // NULL, phantom, or no associated Reach
      return 0;

   if ( pReach->IsPhantomNode() )
      return 0;

   if ( pReach->IsRootNode() )
      return 0;

   // delete any associated catchments
   for ( int i=0; i < (int) pReach->m_catchmentArray.GetSize(); i++ )
      {
      Catchment *pCatchment = pReach->m_catchmentArray[ i ];

      for ( int j=0; j < (int) m_catchmentArray.GetSize(); j++ )
         {
         if ( m_catchmentArray[ i ] == pCatchment )
            {
            m_catchmentArray.RemoveAt( i );
            break;
            }
         }
      }

   // remove from internal list
   for ( int i=0; i < (int) m_reachArray.GetSize(); i++ )
      {
      if ( m_reachArray[ i ] == pReach )
         {
         m_reachArray.RemoveAt( i );    // does NOT delete Reach, just removes it from the list.  The
         break;                         // Reach object is managed by m_reachTree
         }
      }
 
   // remove from reach tree (actually, this just converts the node to a phantom node)
   m_reachTree.RemoveNode( pNode, false );    // don't delete upstream nodes (no recursion)

   return true;
   }



// load table take a filename, creates a new DataObject of the specified type, and fills it
// with the file-based table
int LoadTable( LPCTSTR filename, DataObj** pDataObj, int type )
   {
   if ( *pDataObj != NULL )
      {
      delete *pDataObj;
      *pDataObj = NULL;
      }

   if ( filename == NULL || lstrlen( filename ) == 0 )
      {
      CString msg( "Flow: bad table name specified" );
      if ( filename != NULL )
         {
         msg += ": ";
         msg += filename;
         }
      
      Report::LogMsg( msg, RT_WARNING );
      return 0;
      }

   switch( type )
      {
      case 0:
         *pDataObj = new FDataObj;
         ASSERT( *pDataObj != NULL );
         break;

      case 1:
         *pDataObj = new VDataObj;
         ASSERT( *pDataObj != NULL );
         break;

      default:
         *pDataObj = NULL;
         return -1;
      }

   
   if ( (*pDataObj)->ReadAscii( filename ) <= 0 )
      {
      delete *pDataObj;
      *pDataObj = NULL;
   
      CString msg( "Flow: Error reading table: " );
      msg += ": ";
      msg += filename;
      Report::LogMsg( msg, RT_WARNING );
      return 0;
      }

   (*pDataObj)->SetName( filename );

   return (*pDataObj)->GetRowCount();
   }
    

bool FlowModel::InitFluxes( void )
   {
   int fluxCount = 0;
   int fluxInfoCount = (int) m_fluxInfoArray.GetSize();

   for ( int i=0; i < fluxInfoCount; i++ )
      {
      FluxInfo *pFluxInfo = m_fluxInfoArray[ i ];

      if ( pFluxInfo->m_sourceQuery.IsEmpty() && pFluxInfo->m_sinkQuery.IsEmpty() )
         {
         pFluxInfo->m_use = false;
         continue;
         }

      // source syntax is:
      // <flux name="Precipitation" path="WHydro.dll" description="" 
      //   source_type="hrulayer:*" source_query="AREA > 0" sink_type="" 
      //   sink_query=""  flux_handler="PrecipFluxHandler" />

      // first, figure out what kind of flux it is
      // if path is a dll and there is a flux handler defined, then it a function
      // if the path has a database extension, it's a database of some type.

      LPTSTR buffer = pFluxInfo->m_path.GetBuffer();
      LPTSTR ext = _tcsrchr( buffer, '.' );
      pFluxInfo->m_path.ReleaseBuffer();

      if ( ext == NULL )
         {
         CString msg( _T("Flow: Error parsing flux path '" ) );
         msg += pFluxInfo->m_path;
         msg += _T( "'.  Invalid file extension.  The flux will be ignored." );
         Report::WarningMsg( msg );
         pFluxInfo->m_use = false;
         continue;
         }

      ext++;
      
      FLUXSOURCE fsType = FS_UNDEFINED;
      if ( _tcsicmp( ext, "dll" ) == 0 )
         {
         if ( pFluxInfo->m_fnName.IsEmpty() )
            {
            CString msg( "Flux: Missing 'flux_handler' attribute for DLL-based flux source ");
            msg += pFluxInfo->m_path;
            msg += ". This is a required attribute";
            Report::WarningMsg( msg );
               pFluxInfo->m_use = false;
            continue;
            }
         else     // load dll
            {
            if ( ( pFluxInfo->m_hDLL = AfxLoadLibrary( pFluxInfo->m_path ) ) == NULL )
               {
               CString msg( "Flux: Unable to load flux specified in ");
               msg += pFluxInfo->m_path;
               msg += " or a dependent DLL. This flux will be disabled";
               Report::WarningMsg( msg );
               pFluxInfo->m_use = false;
               continue;
               }

            pFluxInfo->m_pFluxFn = (FLUXFN) ::GetProcAddress( pFluxInfo->m_hDLL, pFluxInfo->m_fnName );

            if ( pFluxInfo->m_pFluxFn == NULL )
               {
               CString msg( "Flux: Unable to load function '" );
               msg += pFluxInfo->m_fnName;
               msg += "'specifed in the 'flux_handler' attribute in ";
               msg += pFluxInfo->m_path;
               msg += ".  This flux will be disabled";
               Report::WarningMsg( msg );
               continue;
               }

            if ( pFluxInfo->m_initFn.GetLength() > 0 )
               {
               pFluxInfo->m_pInitFn = (FLUXINITFN) ::GetProcAddress( pFluxInfo->m_hDLL, pFluxInfo->m_initFn );
               if ( pFluxInfo->m_pInitFn == NULL )
                  {
                  CString msg( "Flux: Unable to load function '" );
                  msg += pFluxInfo->m_initFn;
                  msg += "'specifed in the 'init_handler' attribute in ";
                  msg += pFluxInfo->m_path;
                  msg += ".";
                  Report::WarningMsg( msg );
                  }
               }

            if ( pFluxInfo->m_initRunFn.GetLength() > 0 )
               {
               pFluxInfo->m_pInitRunFn = (FLUXINITRUNFN) ::GetProcAddress( pFluxInfo->m_hDLL, pFluxInfo->m_initRunFn );
               if ( pFluxInfo->m_pInitRunFn == NULL )
                  {
                  CString msg( "Flux: Unable to load function '" );
                  msg += pFluxInfo->m_initRunFn;
                  msg += "'specifed in the 'initRun_handler' attribute in ";
                  msg += pFluxInfo->m_path;
                  msg += ".";
                  Report::WarningMsg( msg );
                  }
               }

            if ( pFluxInfo->m_endRunFn.GetLength() > 0 )
               {
               pFluxInfo->m_pEndRunFn = (FLUXENDRUNFN) ::GetProcAddress( pFluxInfo->m_hDLL, pFluxInfo->m_endRunFn );
               if ( pFluxInfo->m_pEndRunFn == NULL )
                  {
                  CString msg( "Flux: Unable to load function '" );
                  msg += pFluxInfo->m_endRunFn;
                  msg += "'specifed in the 'endRun_handler' attribute in ";
                  msg += pFluxInfo->m_path;
                  msg += ".";
                  Report::WarningMsg( msg );
                  }
               }
            }  // end of: load DLL

         pFluxInfo->m_dataSource = FS_FUNCTION;
         }  // end of: if ( ext is DLL )

      else if ( ( _tcsicmp( ext, "csv" ) == 0 )
             || ( _tcsicmp( ext, "xls" ) == 0 )
             || ( _tcsicmp( ext, "dbf" ) == 0 )
             || ( _tcsicmp( ext, "mdb" ) == 0 )
             || ( _tcsicmp( ext, "ncf" ) == 0 ) )
         {
         fsType = FS_DATABASE;

         TCHAR *buffer = new TCHAR[ pFluxInfo->m_fieldName.GetLength()+1 ];
         lstrcpy( buffer, pFluxInfo->m_fieldName );

         LPTSTR tableName = buffer;
         LPTSTR fieldName = _tcschr( buffer, ':' );
         if ( fieldName == NULL )
            {
            *fieldName = '\0';
            fieldName++;
            tableName = NULL;
            }
         else
            fieldName = buffer;

         // have database info, load it
         ASSERT( pFluxInfo->m_pFluxData == NULL );
         pFluxInfo->m_pFluxData = new FDataObj;
         ASSERT( pFluxInfo->m_pFluxData != NULL );

         int records = pFluxInfo->m_pFluxData->ReadAscii( pFluxInfo->m_path );
         if ( records <= 0 )
            {
            CString msg( "Flux: Unable to load database '" );
            msg += pFluxInfo->m_path;
            msg += "'specified in the 'path' attribute of a flux. This flux will be disabled";
            Report::WarningMsg( msg );
            pFluxInfo->m_use = false;
            delete [] buffer;
            continue;
            }

         delete [] buffer;
         }
      
      // basics handled for the source of the flux.  
      // Next, figure out where this applies. This is determined by the associated query.
      if ( pFluxInfo->m_use == false )
         continue;

      // first, source
      if ( pFluxInfo->m_sourceLocation != FL_UNDEFINED )
         {
         ASSERT( pFluxInfo->m_sourceLocation != NULL );

         pFluxInfo->m_pSourceQuery = BuildFluxQuery( pFluxInfo->m_sourceQuery, pFluxInfo->m_sourceLocation );
   
         if ( pFluxInfo->m_pSourceQuery == NULL )
            {
            CString msg( "Flow: Error parsing flux source query: '" );
            msg += pFluxInfo->m_sourceQuery;
            msg += " for flux '";
            msg += pFluxInfo->m_name;
            msg += "' - this flux will be ignored";
            Report::ErrorMsg( msg );
            pFluxInfo->m_use = false;
            continue;
            }
         }

      // then sinks
      if ( pFluxInfo->m_sinkLocation != FL_UNDEFINED )
         {
         pFluxInfo->m_pSinkQuery = BuildFluxQuery( pFluxInfo->m_sinkQuery, pFluxInfo->m_sinkLocation );
   
         if ( pFluxInfo->m_pSinkQuery == NULL )
            {
            CString msg( "Flow: Error parsing flux sink query: '" );
            msg += pFluxInfo->m_sinkQuery;
            msg += " for flux '";
            msg += pFluxInfo->m_name;
            msg += "' - this flux will be ignored";
            Report::ErrorMsg( msg );
            pFluxInfo->m_use = false;
            continue;
            }
         }

      // now, build the straws (connections) for the FluxInfo.  
      // Each of these are a flux, as opposed to a FluxInfo, are are the
      // "straws" that convey water around the system

      /// NOTE THIS IS PROBABLY INCORRECT FOR TWO_WAY STRAWS (both sources and sinks)
      if ( pFluxInfo->m_sourceLocation != FL_UNDEFINED )
         fluxCount += AllocateFluxes( pFluxInfo, pFluxInfo->m_pSourceQuery, pFluxInfo->m_sourceLocation, pFluxInfo->m_sourceLayer );
      
      if ( pFluxInfo->m_sinkLocation != FL_UNDEFINED )
         fluxCount += AllocateFluxes( pFluxInfo, pFluxInfo->m_pSinkQuery, pFluxInfo->m_sinkLocation, pFluxInfo->m_sinkLayer  );
      }

   CString msg;
   msg.Format( "Flow: %i fluxes generated from %i Flux definitions", fluxCount, (int) m_fluxInfoArray.GetSize() );
   Report::LogMsg( msg );
   return true;
   }


bool FlowModel::InitRunFluxes( void )
   {
   int fluxInfoCount = (int) m_fluxInfoArray.GetSize();

   for ( int i=0; i < fluxInfoCount; i++ )
      {
      FluxInfo *pFluxInfo = m_fluxInfoArray[ i ];

      pFluxInfo->m_totalFluxRate = 0;
      pFluxInfo->m_annualFlux = 0;
      pFluxInfo->m_cumFlux = 0;
      }

   return true;
   }
   



int FlowModel::AllocateFluxes( FluxInfo *pFluxInfo, Query *pQuery, FLUXLOCATION type, int layer )
   {
   int fluxCount = 0;

   // query built, need to determine where it applies
   switch( type )
      {
      case FL_REACH:
         {
         for ( int i=0; i < m_reachArray.GetSize(); i++ )
            {
            Reach *pReach = m_reachArray[ i ];

            int polyIndex = pReach->m_polyIndex;
            bool result = false;
            bool ok = pQuery->Run( polyIndex, result );

            if ( ok && result )
               {
               for ( int j=0; j < (int) pFluxInfo->m_stateVars.GetSize(); j++ )
                  {
                  pReach->AddFlux( pFluxInfo, pFluxInfo->m_stateVars[ j ] );
                  fluxCount++;
                  }
               }
            }
         }
         break;

      case FL_HRULAYER:
         {
         int hruLayerCount = GetHRULayerCount();

         for ( int i=0; i < m_catchmentArray.GetSize(); i++ )
            {
            Catchment *pCatchment = m_catchmentArray[ i ];

            for ( int h=0; h < pCatchment->GetHRUCount(); h++ )
               {
               HRU *pHRU = pCatchment->GetHRU( h );

               // assume that if a majority of idu's satisfy the query, the flux applies to the HRU
               int countSoFar = 0;
               int polyCount = (int) pHRU->m_polyIndexArray.GetSize();

               for ( int k=0; k < polyCount; k++ )
                  {
                  bool result = false;
                  int polyIndex = pHRU->m_polyIndexArray[ k ];
                  bool ok = pQuery->Run( polyIndex, result );

                  if ( ok && result )
                     countSoFar++;
                  
                  if ( countSoFar >= polyCount )
                     {
                     for ( int j=0; j < (int) pFluxInfo->m_stateVars.GetSize(); j++ )
                        {
                        if ( layer < 0 )
                           {
                           for ( int l=0; l < hruLayerCount; l++ )
                              {
                              pHRU->GetLayer( l )->AddFlux( pFluxInfo, pFluxInfo->m_stateVars[ j ] );
                              fluxCount++;
                              }
                           }
                        else
                           {
                           pHRU->GetLayer( layer )->AddFlux( pFluxInfo, pFluxInfo->m_stateVars[ j ] );
                           fluxCount++;
                           }
                        }
                          
                     break;
                     }
                  }  // end of: for ( k < polyCount )
               }  // end of: for ( h < hruCount )
            }  // end of: for ( i < catchmentCount )
         }
         break;

      case FL_RESERVOIR:
      case FL_GROUNDWATER:
         break;

      case FL_IDU:
         {/*
         for ( int i=0; i < m_catchmentArray.GetSize(); i++ )
            {
            Catchment *pCatchment = m_catchmentArray[ i ];

            for ( int h=0; h < pCatchment->GetHRUCount(); h++ )
               {
               HRU *pHRU = pCatchment->GetHRU( h );
               int polyCount = (int) pHRU->m_polyIndexArray.GetSize();

               for ( int k=0; k < polyCount; k++ )
                  {
                  bool result = false;
                  int polyIndex = pHRU->m_polyIndexArray[ k ];
                  bool ok = pQuery->Run( polyIndex, result );



                  if ( ok && result )
                     countSoFar++;
                  
                  if ( countSoFar >= polyCount )
                     {
                     for ( int j=0; j < (int) pFluxInfo->m_stateVars.GetSize(); j++ )
                        {
                        if ( layer < 0 )
                           {
                           for ( int l=0; l < hruLayerCount; l++ )
                              {
                              pHRU->GetLayer( l )->AddFlux( pFluxInfo, pFluxInfo->m_stateVars[ j ] );
                              fluxCount++;
                              }
                           }
                        else
                           {
                           pHRU->GetLayer( layer )->AddFlux( pFluxInfo, pFluxInfo->m_stateVars[ j ] );
                           fluxCount++;
                           }
                        }
                          
                     break;
                     }
                  }  // end of: for ( k < polyCount )
               }  // end of: for ( h < hruCount )
            }  // end of: for ( i < catchmentCount )
            */
         }
         break;
         
      default:
         ASSERT( 0 );
      }
   
   return fluxCount;
   }


Query *FlowModel::BuildFluxQuery( LPCTSTR queryStr, FLUXLOCATION type )
   {
   Query *pQuery = NULL;

   switch( type )
      {
      case FL_REACH:
         ASSERT( m_pStreamQE != NULL );
         pQuery = m_pStreamQE->ParseQuery( queryStr, 0, "Flow Flux Query (Reach)" );
         break;

      case FL_HRULAYER:
         ASSERT( m_pCatchmentQE != NULL );
         pQuery = m_pCatchmentQE->ParseQuery( queryStr, 0, "Flow Flux Query (HRU)" );
         break;

      case FL_RESERVOIR:
      case FL_GROUNDWATER:
         break;
         
      case FL_IDU:
         ASSERT( m_pCatchmentQE != NULL );
         pQuery = m_pCatchmentQE->ParseQuery( queryStr, 0, "Flow Flux Query (IDU)" );
         break;

      default:
         ASSERT( 0 );
      }

   return pQuery;
   }





bool FlowModel::InitIntegrationBlocks( void )
   {
   // allocates arrays necessary for integration method.
   // For reaches, this allocates a state variable for each subnode "discharge" member for each reach,
   //   plus any "extra" state variables
   // For catchments, this allocates a state variable for each hru, for water volume,
   //   plus any "extra" state variables

   // catchments first
   int hruLayerCount = GetHRULayerCount();
   int catchmentCount = (int) m_catchmentArray.GetSize();
   int hruSvCount = 0;
   for ( int i=0; i < catchmentCount; i++ )
      {
      int count = m_catchmentArray[ i ]->GetHRUCount();
      hruSvCount += ( count * hruLayerCount );
      }

   // note: total size of m_hruBlock is ( total number of HRUs ) * ( number of layers ) * (number of sv's/HRU)
   
   m_hruBlock.Init( m_integrator, hruSvCount * m_hruSvCount, m_initTimeStep);   // ALWAYS RK4???. INT_METHOD::IM_RKF  Note: Derivatives are set in GetCatchmentDerivatives.
   if (m_integrator==IM_RKF)
      {
      //m_hruBlock.m_rkvInitTimeStep = m_initTimeStep;
      m_hruBlock.m_rkvMinTimeStep=m_minTimeStep;
      m_hruBlock.m_rkvMaxTimeStep=m_maxTimeStep;
      m_hruBlock.m_rkvTolerance=m_errorTolerance;
      }
   hruSvCount = 0;
   for ( int i=0; i < catchmentCount; i++ )
      {
      int count = m_catchmentArray[ i ]->GetHRUCount();

      for ( int j=0; j < count; j++ )
         {
         HRU *pHRU = m_catchmentArray[i]->GetHRU( j );

         for ( int k=0; k < hruLayerCount; k++ )
            {
            HRULayer *pHRULayer = m_catchmentArray[ i ]->GetHRU( j )->GetLayer( k );

            pHRULayer->m_svIndex = hruSvCount;
            m_hruBlock.SetStateVar(  &(pHRULayer->m_volumeWater), hruSvCount++ );

            for ( int l=0; l < m_hruSvCount-1; l++ )
               m_hruBlock.SetStateVar( &(pHRULayer->m_svArray[ l ]), hruSvCount++ );
            }          
         }
      }
      
   // next, reaches.  Each subnode gets a set of state variables
   if (  m_pReachRouting->GetMethod() == GM_RK4 
      || m_pReachRouting->GetMethod() == GM_EULER
      || m_pReachRouting->GetMethod() == GM_RKF )
      {
      int reachCount = (int) m_reachArray.GetSize();
      int reachSvCount = 0;
      for ( int i=0; i < reachCount; i++ )
        {
        Reach *pReach = m_reachArray[ i ];
        reachSvCount += pReach->GetSubnodeCount();
        }

      INT_METHOD intMethod = IM_RK4;
      if ( m_pReachRouting->GetMethod() == GM_EULER )
         intMethod = IM_EULER;
      else if ( m_pReachRouting->GetMethod() == GM_RKF )
         intMethod = IM_RKF;

      m_reachBlock.Init( intMethod, reachSvCount * m_reachSvCount, m_initTimeStep );

      reachSvCount = 0;
      for ( int i=0; i < reachCount; i++ )
        {
        Reach *pReach = m_reachArray[ i ];

        for ( int j=0; j < pReach->GetSubnodeCount(); j++ )
          {
          // assign state variables
          ReachSubnode *pNode = (ReachSubnode*) pReach->m_subnodeArray[j];
          pNode->m_svIndex = reachSvCount;
          m_reachBlock.SetStateVar( &pNode->m_volume, reachSvCount++ );

          for ( int k=0; k < m_reachSvCount-1; k++ )
            m_reachBlock.SetStateVar( &(pNode->m_svArray[ k ]), reachSvCount++ );
          }
        }
      }
   
   // next, reservoirs
   int reservoirCount = (int) m_reservoirArray.GetSize();
   m_reservoirBlock.Init( IM_RK4, reservoirCount, m_initTimeStep );

   int resSvCount = 0;
   for ( int i=0; i < reservoirCount; i++ )
      {
      Reservoir *pRes = m_reservoirArray.GetAt(i);
      pRes->m_svIndex = resSvCount;
      m_reservoirBlock.SetStateVar(  &(pRes->m_volume), resSvCount++ );

      for ( int k=0; k < m_reservoirSvCount-1; k++ )
         m_reachBlock.SetStateVar( &(pRes->m_svArray[ k ]), resSvCount++ );
      }

   // next, totals (for error-check mass balance).  total includes:
   // 1) total input
   // 2) total output
   // 3) one entry for each flux info to accumulate annual fluxes
   int fluxInfoCount = GetFluxInfoCount();

   m_totalBlock.Init( IM_RK4, 2 + fluxInfoCount, m_timeStep );
   m_totalBlock.SetStateVar( &m_totalWaterInput, 0 );
   m_totalBlock.SetStateVar( &m_totalWaterOutput, 1 );

   for ( int i=0; i < fluxInfoCount; i++ )
      {
      FluxInfo *pFluxInfo = GetFluxInfo( i );
      m_totalBlock.SetStateVar( &pFluxInfo->m_annualFlux, 2+i );
      }
   
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

/*
void FlowModel::UpdateCatchmentFluxes( float time, float timeStep, FlowContext *pFlowContext )
   {
   FlowModel *pModel = pFlowContext->pFlowModel;
   
   // compute derivative values for catchment
   pFlowContext->timeStep = timeStep;
   pFlowContext->time = time;

   int catchmentCount = (int) pModel->m_catchmentArray.GetSize();
   int svIndex = 0;
   int hruLayerCount = pModel->GetHRULayerCount();

   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = pModel->m_catchmentArray[ i ];
      int hruCount = pCatchment->GetHRUCount();
      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         pFlowContext->pHRU = pHRU;

         for ( int l=0; l < hruLayerCount; l++ )
            {
            HRULayer *pHRULayer = pHRU->GetLayer( l );

            pFlowContext->pHRULayer = pHRULayer;

            // update all fluxes for this hruLayer
            for ( int k=0; k < pHRULayer->GetFluxCount(); k++ )
               {
               Flux *pFlux = pHRULayer->GetFlux( k );
               pFlowContext->pFlux = pFlux;

               float flux = pFlux->Evaluate( pFlowContext );
               }

            svIndex += m_hruSvCount;
            }
         }
      }
   }

 */

void FlowModel::GetCatchmentDerivatives( double time, double timeStep, int svCount, double *derivatives /*out*/, void *extra )
   {
   FlowContext *pFlowContext = (FlowContext*) extra;
   
   pFlowContext->timeStep = (float) timeStep;
   pFlowContext->time = (float) time;
   
   FlowModel *pModel = pFlowContext->pFlowModel;
   pModel->ResetCumulativeValues( pFlowContext->pEnvContext );

   //pModel->m_pLateralExchange->Run( pFlowContext ); // SetGlobalHruToReachExchanges(); 
   //pModel->m_pHruVertExchange->Run( pFlowContext ); // SetGlobalHruVertFluxes();

   // evap trans (if any defined)
   //for( int i=0; i < (int) pModel->m_evapTransArray.GetSize(); i++ )
   //   pModel->m_evapTransArray[ i ]->Run( pFlowContext );
   //
   //// allocations (if any defined)
   //for( int i=0; i < (int) pModel->m_allocArray.GetSize(); i++ )
   //   pModel->m_allocArray[ i ]->Run( pFlowContext );

   pModel->SetGlobalExtraSVRxn();

   GlobalMethodManager::Step( pFlowContext );

   pFlowContext->svCount=gpModel->m_hruSvCount-1;
   
   int catchmentCount = (int) pModel->m_catchmentArray.GetSize();
   int hruLayerCount = pModel->GetHRULayerCount();

   FlowContext flowContext( *pFlowContext );    // make a copy for openMP
      
   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   clock_t start = clock();

   int hruCount = (int) pModel->m_hruArray.GetSize();
   #pragma omp parallel for firstprivate( flowContext )
   for ( int h=0; h < hruCount; h++ )
      {
      HRU *pHRU = pModel->m_hruArray[h];
      flowContext.pHRU = pHRU; 

      for ( int l=0; l < hruLayerCount; l++ )
         {
         HRULayer *pHRULayer = pHRU->GetLayer( l );
         flowContext.pHRULayer = pHRULayer;
         int svIndex = pHRULayer->m_svIndex;
         derivatives[svIndex] = pHRULayer->GetFluxValue();

         // initialize additional state variables=0
         for ( int k=1; k < gpModel->m_hruSvCount; k++ )
            derivatives[svIndex+k] = 0;

         // transport fluxes for the extra state variables
         for ( int k=1; k < gpModel->m_hruSvCount; k++ )
            derivatives[svIndex+k] = pHRULayer->GetExtraSvFluxValue(k-1);

         // add all fluxes for this hruLayer - this includes extra state variables!
         for ( int k=0; k < pHRULayer->GetFluxCount(); k++ )
            {
            Flux *pFlux = pHRULayer->GetFlux( k );
            flowContext.pFlux = pFlux;
            // get the flux.  Note that this is a rate applied over a timestep
            float flux = pFlux->Evaluate( &flowContext ); //pFlowContext );
            int sv = pFlux->m_pStateVar->m_index;
            if ( pFlux->IsSource() )
               derivatives[ svIndex + sv ] += flux;
            else
               derivatives[ svIndex + sv ] -= flux;
            }
         }
      }

   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
   gpModel->m_hruFluxFnRunTime += (float) duration;   
   }



void FlowModel::GetReservoirDerivatives( double time, double timeStep, int svCount, double *derivatives /*out*/, void *extra )
   {
   FlowContext *pFlowContext = (FlowContext*) extra;
   FlowModel *pModel = pFlowContext->pFlowModel;
   
   // compute derivative values for catchment
   pFlowContext->timeStep = (float) timeStep;
   pFlowContext->time = (float) time;

   int reservoirCount = (int) pModel->m_reservoirArray.GetSize();

   // iterate through reservoirs, calling fluxes as needed
   //#pragma omp parallel for
   for ( int i=0; i < reservoirCount; i++ )
      {
      FlowContext flowContext( *pFlowContext );    // make a copy for openMP

      Reservoir *pRes = pModel->m_reservoirArray[ i ];

      int svIndex = pRes->m_svIndex;

      // basic inflow/outflow (assumes these were calculated prior to this point)
      derivatives[svIndex] = pRes->m_inflow - pRes->m_outflow;//m3/d;

      // include additional state variables
      for ( int k=1; k < pModel->m_reservoirSvCount; k++ )
         derivatives[svIndex+k] = 0;

      // add all fluxes for this reservoir...not sure we will define external fluxes for reservoirs, but perhaps...
      for ( int k=0; k < pRes->GetFluxCount(); k++ )
         {
         Flux *pFlux = pRes->GetFlux( k );
         pFlowContext->pFlux = pFlux;

         float flux = pFlux->Evaluate( pFlowContext );

         // figure out which state var this flux is associated with
         int sv = pFlux->m_pStateVar->m_index;

         // source or sink?  If sink, flip sign  
         // CHECK THIS???? - probably not correct for two-way straws 
         if ( pFlux->IsSource() )
            {
            derivatives[ svIndex + sv ] += flux;
            pModel->m_totalWaterInputRate += flux;
            }
         else
            {
            derivatives[ svIndex + sv ] -= flux;
            pModel->m_totalWaterOutputRate += flux;
            }
         }
      }
   }


//void FlowModel::GetReachDerivatives( float time, float timeStep, int svCount, double *derivatives /*out*/, void *extra )
//   {
//   FlowContext *pFlowContext = (FlowContext*) extra;
//
//   FlowContext flowContext( *pFlowContext );   // make a copy for openMP
//
//   // NOTE: NEED TO INITIALIZE TIME IN FlowContext before calling here...
//   // NOTE:  Should probably do a breadth-first search of the tree(s) rather than what is here.  This could be accomplished by
//   //  sorting the m_reachArray in InitReaches()
//
//   FlowModel *pModel = pFlowContext->pFlowModel;
//   flowContext.timeStep = timeStep;
//   
//   // compute derivative values for reachs based on any associated subnode fluxes
//   int reachCount = (int) pModel->m_reachArray.GetSize();
//
//   omp_set_num_threads( gpFlow->m_processorsUsed );
//   #pragma omp parallel for firstprivate( flowContext )
//   for ( int i=0; i < reachCount; i++ )
//      {
//      Reach *pReach = pModel->m_reachArray[ i ];
//      flowContext.pReach = pReach;
//
//      for ( int l=0; l < pReach->GetSubnodeCount(); l++ )
//         {
//         ReachSubnode *pNode = pReach->GetReachSubnode( l );
//         int svIndex = pNode->m_svIndex;
//
//         flowContext.reachSubnodeIndex = l;
//         derivatives[svIndex]=0;
//
//         // add all fluxes for this reach
//         for ( int k=0; k < pReach->GetFluxCount(); k++ )
//            {
//            Flux *pFlux = pReach->GetFlux( k );
//            flowContext.pFlux = pFlux;
//
//            float flux = pFlux->Evaluate( pFlowContext );
//
//            // figure out which state var this flux is associated with
//            int sv = pFlux->m_pStateVar->m_index;
//
//            if ( pFlux->IsSource() )
//               {
//               derivatives[ svIndex + sv ] += flux;
//               pModel->m_totalWaterInputRate += flux;
//               }
//            else
//               {
//               derivatives[ svIndex + sv ] -= flux;
//               pModel->m_totalWaterOutputRate += flux;
//               }
//            }
//         }
//      }
//   }


void FlowModel::GetTotalDerivatives( double time, double timeStep, int svCount, double *derivatives /*out*/, void *extra )
   {
   FlowContext *pFlowContext = (FlowContext*) extra;

   // NOTE: NEED TO INITIALIZE TIME IN FlowContext before calling here...
   // NOTE:  Should probably do a breadth-first search of the tree(s) rather than what is here.  This could be accomplished by
   //  sorting the m_reachArray in InitReaches()
   FlowModel *pModel = pFlowContext->pFlowModel;

   int fluxInfoCount = pModel->GetFluxInfoCount();

   ASSERT( svCount == 2 + fluxInfoCount );
   derivatives[ 0 ] = pModel->m_totalWaterInputRate;
   derivatives[ 1 ] = pModel->m_totalWaterOutputRate;

   #pragma omp parallel for
   for ( int i=0; i < fluxInfoCount; i++ )
      {
      FluxInfo *pFluxInfo = pModel->GetFluxInfo( i );
      derivatives[ 2+i ]  = pFluxInfo->m_totalFluxRate;
      }
   }


ParamTable *FlowModel::GetTable( LPCTSTR name )
   {
   for ( int i=0; i < (int) m_tableArray.GetSize(); i++ )
      if ( lstrcmpi( m_tableArray[ i ]->m_pTable->GetName(), name ) == 0 )
         return m_tableArray[ i ];

   return NULL;
   }


bool FlowModel::GetTableValue( LPCTSTR name, int col, int row, float &value )
   {
   ParamTable *pTable = GetTable( name );

   if ( pTable == NULL )
      return false;

   return pTable->m_pTable->Get( col, row, value );
   }



void FlowModel::AddReservoirLayer( void )
   {
   Map *pMap = m_pCatchmentLayer->GetMapPtr();

   MapLayer *pResLayer = pMap->GetLayer( "Reservoirs" );

   if ( pResLayer != NULL )      // already exists?
      {
      return;
      }

   // doesn't already exist, so map existing reservoirs
   int resCount = (int) m_reservoirArray.GetSize();
   int usedResCount = 0;
   for ( int i=0; i < resCount; i++ )
      {
      if ( m_reservoirArray[ i ]->m_pReach != NULL )
         usedResCount++;
      }
   
   m_pResLayer = pMap->AddLayer( LT_POINT );

   DataObj *pData = m_pResLayer->CreateDataTable( usedResCount, 1, DOT_FLOAT );
   
   usedResCount = 0;
   for ( int i=0; i < resCount; i++ )
      {
      Reservoir *pRes = m_reservoirArray[ i ];

      if ( pRes->m_pReach != NULL )
         {
         int polyIndex = pRes->m_pReach->m_polyIndex;

         Poly *pPt = m_pStreamLayer->GetPolygon( polyIndex );
         int vertexCount = (int) pPt->m_vertexArray.GetSize();
         Vertex &v = pPt->m_vertexArray[ vertexCount-1 ];
      
         m_pResLayer->AddPoint( v.x, v.y );
         pData->Set( 0, usedResCount++, pRes->m_volume );
         }
      }

   // additional map setup
   m_pResLayer->m_name = "Reservoirs";
   m_pResLayer->SetFieldLabel( 0, "ResVol" );
   //m_pResLayer->AddBin( 0, RGB(0,0,127), "Reservoir Volume", TYPE_FLOAT, 0, 1000000 );
   m_pResLayer->SetActiveField( 0 );
   m_pResLayer->SetOutlineColor( RGB( 0, 0, 127 ) );
   m_pResLayer->SetSolidFill( RGB( 0,0, 255 ) );
//   m_pResLayer->UseVarWidth( 0, 100 );
   //pMap->RedrawWindow();
   }




////////////////////////////////////////////////////////////////////////////////////////////////
// F L O W    P R O C E S S 
////////////////////////////////////////////////////////////////////////////////////////////////

FlowProcess::FlowProcess()
: EnvAutoProcess()
, m_maxProcessors( -1 )
, m_processorsUsed( 0 )
   { }
  

FlowProcess::~FlowProcess(void)
   {
   m_flowModelArray.RemoveAll();
   }


BOOL FlowProcess::Setup( EnvContext *pEnvContext, HWND )
   {
   FlowModel *pModel = GetFlowModelFromID( pEnvContext->id );
   ASSERT( pModel != NULL );

   FlowDlg dlg( pModel, "Flow Setup" );

   dlg.DoModal();

   return TRUE;
   }



BOOL FlowProcess::Init( EnvContext *pEnvContext, LPCTSTR initStr  )
   {
   EnvExtension::Init( pEnvContext, initStr );

   MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;

   bool ok = LoadXml( initStr, pLayer );
   
   if ( ! ok )
      return FALSE;

   m_processorsUsed = omp_get_num_procs();

   m_processorsUsed = ( 3 * m_processorsUsed / 4) + 1;   // default uses 2/3 of the cores
   if ( m_maxProcessors > 0 && m_processorsUsed >= m_maxProcessors )
      m_processorsUsed = m_maxProcessors;

   omp_set_num_threads(m_processorsUsed); 

   FlowModel *pModel = m_flowModelArray.GetAt( m_flowModelArray.GetSize()-1 );
   ASSERT( pModel != NULL );

   ok = pModel->Init( pEnvContext );

   return ok ? TRUE : FALSE;
   }


BOOL FlowProcess::InitRun( EnvContext *pEnvContext  )
   {
   m_flowModelArray.GetAt( m_flowModelArray.GetSize()-1 );

   FlowModel *pModel = GetFlowModelFromID( pEnvContext->id );
   ASSERT( pModel != NULL );

   bool ok = pModel->InitRun( pEnvContext );
   
   return ok ? TRUE : FALSE;
   }


BOOL FlowProcess::Run( EnvContext *pEnvContext  )
   {
   m_flowModelArray.GetAt( m_flowModelArray.GetSize()-1 );

   FlowModel *pModel = GetFlowModelFromID( pEnvContext->id );
   ASSERT( pModel != NULL );

   bool ok = pModel->Run( pEnvContext );
   
   return ok ? TRUE : FALSE;
   }

BOOL FlowProcess::EndRun( EnvContext *pEnvContext  )
   {
   m_flowModelArray.GetAt( m_flowModelArray.GetSize()-1 );

   FlowModel *pModel = GetFlowModelFromID( pEnvContext->id );
   ASSERT( pModel != NULL );

   bool ok = pModel->EndRun( pEnvContext );
   
   return ok ? TRUE : FALSE;
   }


FlowModel *FlowProcess::GetFlowModelFromID( int id )
   {
   for ( int i=0; i < (int) m_flowModelArray.GetSize(); i++ )
      {
      FlowModel *pModel = m_flowModelArray.GetAt( i );
      
      if ( pModel->m_id == id )
         return pModel;
      }
   
   return NULL;
   }


// this method reads an xml input file
bool FlowProcess::LoadXml( LPCTSTR filename, MapLayer *pIDULayer )
   {
   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   bool loadSuccess = true;

   if ( ! ok )
      {
      CString msg;
      msg.Format("Flow: Error reading input file %s:  %s", filename, doc.ErrorDesc() );
      Report::ErrorMsg( msg );
      return false;
      }
   
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // <flow_model>

   FlowModel *pModel = new FlowModel;
   pModel->m_filename = filename;
   CString grid;
   CString integrator;
   grid.Format("none");

   int iduDailyOutput    = 0;
   int iduAnnualOutput   = 0;
   int reachDailyOutput  = 0;
   int reachAnnualOutput = 0;

   XML_ATTR rootAttrs[] = {
      // attr                     type          address                                isReq  checkCol
      { "name",                    TYPE_CSTRING,  &(pModel->m_name),                   false,   0 },
      { "time_step",               TYPE_FLOAT,    &(pModel->m_timeStep),               true,    0 },
      { "build_catchments_method", TYPE_INT,      &(pModel->m_buildCatchmentsMethod),  false,   0 },
      { "catchment_join_col",      TYPE_CSTRING,  &(pModel->m_catchmentJoinCol),       true,    0 },
      { "stream_join_col",         TYPE_CSTRING,  &(pModel->m_streamJoinCol),          true,    0 },
      { "simplify_query",          TYPE_CSTRING,  &(pModel->m_streamQuery),            false,   0 },
      { "simplify_min_area",       TYPE_FLOAT,    &(pModel->m_minCatchmentArea),       false,   0 },
      { "simplify_max_area",       TYPE_FLOAT,    &(pModel->m_maxCatchmentArea),       false,   0 },
      { "path",                    TYPE_CSTRING,  &(pModel->m_path),                   false,   0 },  
      { "check_mass_balance",      TYPE_INT,      &(pModel->m_checkMassBalance),       false,   0 },  
      { "max_processors",          TYPE_INT,      &m_maxProcessors,                    false,   0 },  
      { "grid",                    TYPE_CSTRING,  &grid,                               false,   0 },
      { "integrator",              TYPE_CSTRING,  &integrator,                         false,   0 },
      { "min_timestep",            TYPE_FLOAT,    &(pModel->m_minTimeStep),            false,   0 },
      { "max_timestep",            TYPE_FLOAT,    &(pModel->m_maxTimeStep),            false,   0 },
      { "error_tolerance",         TYPE_FLOAT,    &(pModel->m_errorTolerance),         false,   0 },
      { "init_timestep",           TYPE_FLOAT,    &(pModel->m_initTimeStep),           false,   0 },
      { "update_display",          TYPE_INT,      &(pModel->m_mapUpdate),              false,   0 },
      { "initial_conditions",      TYPE_CSTRING,  &(pModel->m_initConditionsFileName), false,   0 },
      { "daily_idu_output",        TYPE_INT,      &iduDailyOutput,                     false,   0 },
      { "annual_idu_output",       TYPE_INT,      &iduAnnualOutput,                    false,   0 },
      { "daily_reach_output",      TYPE_INT,      &reachDailyOutput,                   false,   0 },
      { "annual_reach_output",     TYPE_INT,      &reachAnnualOutput,                  false,   0 },
      { NULL,                      TYPE_NULL,     NULL,                                false,   0 } };

   ok = TiXmlGetAttributes( pXmlRoot, rootAttrs, filename, NULL );
   pModel->m_path = PathManager::GetPath(1);
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Flow: Misformed root element reading <flow> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      delete pModel;
      return false;
      }

   if (grid.CompareNoCase("none"))
      {
      pModel->m_pGrid= pIDULayer->GetMapPtr()->GetLayer(grid);
     pModel->m_grid=grid;
      if (!pModel->m_pGrid)
         {
         CString msg; 
         msg.Format( _T("Flow: Can't find %s in the Map. Check the envx file"), (LPCTSTR) grid );
         Report::ErrorMsg( msg );
         return false;
         }
      }
   if ( lstrcmpi( integrator, "rk4" ) == 0 )
      pModel->m_integrator = IM_RK4;
   else if ( lstrcmpi( integrator, "rkf" ) == 0 )
      pModel->m_integrator = IM_RKF;
   else
      pModel->m_integrator = IM_EULER;

   pModel->m_detailedOutputFlags = 0;
   if ( iduDailyOutput    ) pModel->m_detailedOutputFlags += 1;
   if ( iduAnnualOutput   ) pModel->m_detailedOutputFlags += 2;
   if ( reachDailyOutput  ) pModel->m_detailedOutputFlags += 4;
   if ( reachAnnualOutput ) pModel->m_detailedOutputFlags += 8;

   //------------------- <catchment> ------------------
   CString colNames;
   CString layerDepths;
   CString layerWC;
   TiXmlElement *pXmlCatchment = pXmlRoot->FirstChildElement( "catchments" ); 
   XML_ATTR catchmentAttrs[] = {
      // attr                 type          address                           isReq  checkCol
      { "layer",              TYPE_CSTRING,  &(pModel->m_catchmentLayer),     false,   0 },
      { "query",              TYPE_CSTRING,  &(pModel->m_catchmentQuery),     false,   0 },
      { "area_col",           TYPE_CSTRING,  &(pModel->m_areaCol),            false,   0 },
      { "elev_col",           TYPE_CSTRING,  &(pModel->m_elevCol),            false,   0 },
      //{ "join_col",           TYPE_CSTRING,  &(pModel->m_catchmentJoinCol),   true,    0 },  // typically "COMID"
      { "catchment_agg_cols", TYPE_CSTRING,  &(pModel->m_catchmentAggCols),   false,   0 },
      { "hru_agg_cols",       TYPE_CSTRING,  &(pModel->m_hruAggCols),         true,    0 },
      { "hruID_col",          TYPE_CSTRING,  &(pModel->m_hruIDCol),           false,   0 },
      { "catchmentID_col",    TYPE_CSTRING,  &(pModel->m_catchIDCol),         false,   0 },
      { "soil_layers",        TYPE_INT,      &(pModel->m_soilLayerCount),     false,   0 },
      { "snow_layers",        TYPE_INT,      &(pModel->m_snowLayerCount),     false,   0 },
      { "veg_layers",         TYPE_INT,      &(pModel->m_vegLayerCount),      false,   0 },
      { "init_water_content", TYPE_CSTRING,  &(layerWC),                      false,   0 },
      { "layer_names",        TYPE_CSTRING,  &(colNames),                     false,   0 },
      { "layer_depths",       TYPE_CSTRING,  &(layerDepths),                  false,   0 },
      { NULL,                 TYPE_NULL,     NULL,                            false,   0 } };
            
   ok = TiXmlGetAttributes( pXmlCatchment, catchmentAttrs, filename, NULL );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Flow: Misformed root element reading <catchments> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      delete pModel;
      return false;
      }

   pModel->ParseHRULayerDetails(colNames,0);
   pModel->ParseHRULayerDetails(layerDepths,1);
   pModel->ParseHRULayerDetails(layerWC,2);
   pModel->m_pCatchmentLayer = pIDULayer->GetMapPtr()->GetLayer( pModel->m_catchmentLayer );
   // ERROR CHECK

   if ( pModel->m_hruIDCol.IsEmpty() )
      pModel->m_hruIDCol = _T( "HRU_ID" );

   if ( pModel->m_catchIDCol.IsEmpty() )
      pModel->m_catchIDCol = _T( "CATCH_ID" );
      
   if ( pModel->m_areaCol.IsEmpty() )
      pModel->m_areaCol = _T( "AREA" );

   if ( pModel->m_orderCol.IsEmpty() )
      pModel->m_orderCol = _T( "ORDER" );

   if ( pModel->m_elevCol.IsEmpty() )
      pModel->m_elevCol = _T( "ELEV_MEAN" );

   pModel->m_colCatchmentArea = pModel->m_pCatchmentLayer->GetFieldCol(pModel->m_areaCol);
   if (pModel->m_colCatchmentArea < 0)
      {
      CString msg; 
      msg.Format( _T("Flow: Bad Area col %s"), filename );
      Report::ErrorMsg( msg );
      }

   pModel->m_colElev = pModel->m_pCatchmentLayer->GetFieldCol(pModel->m_elevCol);
   if (pModel->m_colElev < 0)
      {
      CString msg; 
      msg.Format( _T("Flow:  Elevation col %s doesn't exist.  It will be added and, if using station met data, it's values will be set to the station elevation (it is used for lapse rate calculations)"), filename );
      Report::ErrorMsg( msg );
      }

   gpFlow->AddModel( pModel );

   //-------------- <stream> ------------------
   TiXmlElement *pXmlStream = pXmlRoot->FirstChildElement( "streams" ); 
   float reachTimeStep = 1.0f;
   XML_ATTR streamAttrs[] = {
      // attr              type           address                          isReq  checkCol
      { "layer",           TYPE_CSTRING,  &(pModel->m_streamLayer),        true,    0 },
      //{ "query",           TYPE_CSTRING,  &(pModel->m_streamQuery),        false,   0 },
      { "order_col",       TYPE_CSTRING,  &(pModel->m_orderCol),           false,   0 },
      //{ "join_col",        TYPE_CSTRING,  &(pModel->m_streamJoinCol),      true,    0 },
      { "subnode_length",  TYPE_FLOAT,    &(pModel->m_subnodeLength),      false,   0 },
      { "wd_ratio",        TYPE_FLOAT,    &(pModel->m_wdRatio),            false,   0 },
      //{ "min_catch_area",  TYPE_FLOAT,    &(pModel->m_minCatchmentArea),   false,   0 },
      //{ "max_catch_area",  TYPE_FLOAT,    &(pModel->m_maxCatchmentArea),   false,   0 },
      //{ "min_order",       TYPE_INT,      &(pModel->m_minStreamOrder),     false,   0 },
      { "stepsize",        TYPE_FLOAT,    &reachTimeStep,                  false,   0 },    // not currently used!!!!
      { "treeID_col",      TYPE_CSTRING,  &(pModel->m_treeIDCol ),          false,   0 },
      { NULL,              TYPE_NULL,     NULL,                            false,   0 } };

   ok = TiXmlGetAttributes( pXmlStream, streamAttrs, filename, NULL );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Flow: Misformed root element reading <streams> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      delete pModel;
      return false;
      }

   // success - add model and get flux definitions
   pModel->m_pStreamLayer = pIDULayer->GetMapPtr()->GetLayer( pModel->m_streamLayer );

   if ( ! pModel->m_treeIDCol.IsEmpty() )
      CheckCol(pModel->m_pStreamLayer, pModel->m_colTreeID, pModel->m_treeIDCol, TYPE_LONG, CC_AUTOADD );
   
   //-------- <global_methods> ---------------///
   //pModel->m_reachSolutionMethod = RSM_KINEMATIC;     // if not defined, default to kinematic
   //pModel->m_latExchMethod = HREX_LINEARRESERVOIR;    // default to linear_reservoir
   //pModel->m_hruVertFluxMethod = VD_BROOKSCOREY;      // default to brooks-corey
  // pModel->m_gwMethod = GW_NONE;                    // defaults to no groundwater

   TiXmlElement *pXmlGlobal = pXmlRoot->FirstChildElement( "global_methods" ); 
   if ( pXmlGlobal != NULL )
      {
//      LPCTSTR reachRoutingMethod = NULL;
      LPCTSTR exchangeMethod = NULL;
      LPCTSTR vertFluxMethod = NULL;
      LPCTSTR gwMethod = NULL;
      LPCTSTR svRxnMethod = NULL;
      XML_ATTR globalAttrs[] = {
         // attr                    type            address            isReq  checkCol
         //{ "reach_routing",         TYPE_STRING,  &reachRoutingMethod, false,   0 },
         //{ "horizontal_exchange",   TYPE_STRING,  &exchangeMethod,     false,   0 },
         { "hru_vertical_exchange", TYPE_STRING,  &vertFluxMethod,     false,   0 },
         { "groundwater",           TYPE_STRING,  &gwMethod,           false,   0 },
         { "extraStateVarRxn",      TYPE_STRING,  &svRxnMethod,        false,   0 },
         { NULL,                     TYPE_NULL,    NULL,               false,   0 } };

      ok = TiXmlGetAttributes( pXmlGlobal, globalAttrs, filename, NULL );
      if ( ! ok )
         {
         CString msg; 
         msg.Format( _T("Flow: Misformed root element reading <global_methods> attributes in input file %s"), filename );
         Report::ErrorMsg( msg );
         return false;
         }

      //if ( reachRoutingMethod )
      //   {
      //   if ( lstrcmpi( reachRoutingMethod, "kinematic" ) == 0 )
      //      pModel->m_reachSolutionMethod = RSM_KINEMATIC;
      //   else if ( lstrcmpi( reachRoutingMethod, "euler" ) == 0 )
      //      pModel->m_reachSolutionMethod = RSM_EULER;
      //   else if ( lstrcmpi( reachRoutingMethod, "rkf" ) == 0 )
      //      pModel->m_reachSolutionMethod = RSM_RKF;
      //   else if ( lstrcmpi( reachRoutingMethod, "rk4" ) == 0 )
      //      pModel->m_reachSolutionMethod = RSM_RK4;
      //   else
      //      {
      //      pModel->m_reachSolutionMethod = RSM_EXTERNAL;
      //      pModel->m_reachExtSource = reachRoutingMethod;
      //      }
      //   }
      
      //if ( exchangeMethod )
      //   {
      //   if ( lstrcmpi( exchangeMethod, "linear_reservoir" ) == 0 )
      //      pModel->m_latExchMethod = HREX_LINEARRESERVOIR;
      //   else if ( lstrcmpi( exchangeMethod, "none" ) == 0 )
      //      pModel->m_latExchMethod = HREX_INFLUXHANDLER;
      //   else 
      //      {
      //      pModel->m_latExchMethod = HREX_EXTERNAL;
      //      pModel->m_latExchExtSource = exchangeMethod;
      //      }
      //   }

      //if ( vertFluxMethod )
      //   {
      //   if ( lstrcmpi( vertFluxMethod, "Brooks_Corey" ) == 0 )
      //      pModel->m_hruVertFluxMethod = VD_BROOKSCOREY;
      //   else if ( lstrcmpi( vertFluxMethod, "none" ) == 0 )
      //      pModel->m_hruVertFluxMethod = VD_INFLUXHANDLER;
      //   else
      //      {
      //      pModel->m_hruVertFluxMethod = VD_EXTERNAL;
      //      pModel->m_hruVertFluxExtSource = vertFluxMethod;
      //      }
      //   }

      //if ( gwMethod )
      //   {
      //   if ( lstrcmpi( gwMethod, "none" ) == 0 )
      //      pModel->m_gwMethod = GW_NONE;
      //   else
      //      {
      //      pModel->m_gwMethod = GW_EXTERNAL;
      //      pModel->m_gwExtSource = gwMethod;
      //      }
      //   }

      if ( svRxnMethod )
         {
         if ( lstrcmpi( svRxnMethod, "none" ) == 0 )
            pModel->m_extraSvRxnMethod = EXSV_INFLUXHANDLER;
         else
            {
            pModel->m_extraSvRxnMethod = EXSV_EXTERNAL;
            pModel->m_extraSvRxnExtSource = svRxnMethod;
            }
         }

      // Reach Routing
      TiXmlElement *pXmlReachRouting = pXmlGlobal->FirstChildElement( "reach_routing" );
      pModel->m_pReachRouting = ReachRouting::LoadXml( pXmlReachRouting, filename );
      if (pModel->m_pReachRouting != NULL) 
         GlobalMethodManager::AddGlobalMethod(pModel->m_pReachRouting);

      // lateral (horizontal) exchange
      TiXmlElement *pXmlLatExch = pXmlGlobal->FirstChildElement( "lateral_exchange" );
      pModel->m_pLateralExchange = LateralExchange::LoadXml( pXmlLatExch, filename );
      if (pModel->m_pLateralExchange != NULL) 
         GlobalMethodManager::AddGlobalMethod(pModel->m_pLateralExchange);

      // hru vertical exchange
      TiXmlElement *pXmlHruVertExch = pXmlGlobal->FirstChildElement( "hru_vertical_exchange" );
      pModel->m_pHruVertExchange = HruVertExchange::LoadXml( pXmlHruVertExch, filename );
      if (pModel->m_pHruVertExchange != NULL) 
         GlobalMethodManager::AddGlobalMethod(pModel->m_pHruVertExchange);
      
      // any external methods?
      TiXmlElement *pXmlMethod = pXmlGlobal->FirstChildElement("external");
      while (pXmlMethod != NULL)
      {
         GlobalMethod *pMethod = ExternalMethod::LoadXml(pXmlMethod, filename);
         if (pMethod != NULL)
            GlobalMethodManager::AddGlobalMethod(pMethod);

         pXmlMethod = pXmlMethod->NextSiblingElement("external");
      }  // end of : while ( Method xml is not NULL )

      // evapotranspiration (multiple entries allowed)
      TiXmlElement *pXmlEvapTrans = pXmlGlobal->FirstChildElement( "evap_trans" ); 
      while ( pXmlEvapTrans != NULL )
         {
         GlobalMethod *pMethod = EvapTrans::LoadXml( pXmlEvapTrans, pIDULayer, filename );
         if ( pMethod != NULL) GlobalMethodManager::AddGlobalMethod( pMethod );

         pXmlEvapTrans = pXmlEvapTrans->NextSiblingElement( "evap_trans" );
         }

      // daily urban water demand 
      TiXmlElement *pXmlDailyUrbWaterDmd = pXmlGlobal->FirstChildElement("Urban_Water_Demand");
      while (pXmlDailyUrbWaterDmd != NULL)
         {
         GlobalMethod *pMethod = DailyUrbanWaterDemand::LoadXml(pXmlDailyUrbWaterDmd, pIDULayer, filename);
         if (pMethod != NULL) 
            GlobalMethodManager::AddGlobalMethod(pMethod);

         pXmlDailyUrbWaterDmd = pXmlDailyUrbWaterDmd->NextSiblingElement("Urban_Water_Demand");
         }

      // flux (multiple entries allowed)
      TiXmlElement *pXmlFlux = pXmlGlobal->FirstChildElement( "flux" ); 
      while ( pXmlFlux != NULL )
         {
         GlobalMethod *pMethod = FluxExpr::LoadXml( pXmlFlux, pModel, pIDULayer, filename );
         if (pMethod != NULL) 
            GlobalMethodManager::AddGlobalMethod(pMethod);

         pXmlFlux = pXmlFlux->NextSiblingElement( "flux" );
         }

      // allocations (multiple entries allowed)
      TiXmlElement *pXmlAlloc = pXmlGlobal->FirstChildElement( "allocation" ); 
      while ( pXmlAlloc != NULL )
         {
         GlobalMethod *pMethod = WaterAllocation::LoadXml( pXmlAlloc, filename );
         if (pMethod != NULL) 
            GlobalMethodManager::AddGlobalMethod(pMethod);

         pXmlAlloc = pXmlAlloc->NextSiblingElement( "allocation" );
         }
      }

   // scenarios
   TiXmlElement *pXmlScenarios = pXmlRoot->FirstChildElement( "scenarios" );
   if ( pXmlScenarios == NULL )
      {
      CString msg( "Flow: Missing <scenarios> tag when reading " );
      msg += filename;
      msg += "This is a required tag";
      Report::ErrorMsg( msg );
      }
   else
      {
      int defaultIndex = 0;
      TiXmlGetAttr( pXmlScenarios, "default", defaultIndex, 0, false );

      TiXmlElement *pXmlScenario = pXmlScenarios->FirstChildElement( "scenario" );

      while ( pXmlScenario != NULL )
         {
         LPCTSTR name=NULL;
         int id = -1;

         XML_ATTR scenarioAttrs[] = {
            // attr      type          address         isReq  checkCol
            { "name",    TYPE_STRING,   &name,         false,   0 },
            { "id",      TYPE_INT,      &id,           true,    0 },
            { NULL,      TYPE_NULL,     NULL,          false,   0 } };
                             
         bool ok = TiXmlGetAttributes( pXmlScenario, scenarioAttrs, filename );
         if ( ! ok )
            {
            CString msg; 
            msg.Format( _T("Flow: Misformed element reading <scenario> attributes in input file %s - it is missing the 'id' attribute"), filename );
            Report::ErrorMsg( msg );
            break;
            }

         FlowScenario *pScenario = new FlowScenario;
         pScenario->m_id = id;
         pScenario->m_name = name;
         pModel->m_scenarioArray.Add( pScenario );
            
         //---------- <climate> ----------------------------------
         TiXmlElement *pXmlClimate = pXmlScenario->FirstChildElement( "climate" );
         while( pXmlClimate != NULL )
            {
            LPTSTR path=NULL, varName=NULL, cdtype=NULL;
            float delta = -999999999.0f;
            float elev = 0.0f;
            //CString path1;

            XML_ATTR climateAttrs[] = {
               // attr        type          address     isReq  checkCol
               { "type",      TYPE_STRING,   &cdtype,   true,   0 },
               { "path",      TYPE_STRING,   &path,     true,   0 },
               { "var_name",  TYPE_STRING,   &varName,  false,   0 },
               { "delta",     TYPE_FLOAT,    &delta,    false,  0 },
               { "elev",      TYPE_FLOAT,    &elev,     false,  0 },
               { NULL,        TYPE_NULL,     NULL,      false,  0 } };
            
            bool ok = TiXmlGetAttributes( pXmlClimate, climateAttrs, filename );
            if ( ! ok )
               {
               CString msg; 
               msg.Format( _T("Flow: Misformed element reading <climate> attributes in input file %s"), filename );
               Report::ErrorMsg( msg );
               }
            else 
               {
               ClimateDataInfo *pInfo = new ClimateDataInfo;
               pInfo->m_varName = varName;
               int driveLetter = pModel->ParseHRULayerDetails( CString( path ), 4);//is there a : in the path?  If so it is complete.  If not, assume it is a subdirectory of the project path
               if ( driveLetter > 1)//there was a colon (ParseHRULayerDetails returns 1+the number of colons in the string)
                  pInfo->m_path = path;
               else
                  pInfo->m_path = pModel->m_path+path;            


               pInfo->m_pDataObj = new GeoSpatialDataObj;
               

               pInfo->m_useDelta = pXmlClimate->Attribute( "delta" ) ? true : false;
               if ( pInfo->m_useDelta )
                  pInfo->m_delta = delta;

               //pModel->m_climateDataType = CDT_PRECIP; //Just makes clear that it is NOT measured time series

               switch( cdtype[ 0 ] )
                  {
                  case 'p':     // precip
                     pInfo->m_type = CDT_PRECIP;
                     break;

                  case 't':
                     if ( cdtype[2] == 'v' )          // tavg
                        pInfo->m_type = CDT_TMEAN;
                     else if ( cdtype[2] == 'i' )     // tmin
                        pInfo->m_type = CDT_TMIN;
                     else if ( cdtype[2] == 'a' )     // tmax
                        pInfo->m_type = CDT_TMAX;
                     break;

                  case 's':
                     if ( cdtype[1] == 'o' )          // tavg
                        pInfo->m_type = CDT_SOLARRAD;
                     break;

                  case 'h':
                     pInfo->m_type = CDT_SPHUMIDITY;
                     break;

                  case 'r':
                     pInfo->m_type = CDT_RELHUMIDITY;
                     break;

                  case 'w':
                     pInfo->m_type = CDT_WINDSPEED;
                     break;

                  case 'c':
                     pInfo->m_type = CDT_UNKNOWN;
                     pModel->m_useStationData = true;
                     pModel->m_climateFilePath = pInfo->m_path;
                     pModel->ReadClimateStationData(pModel->m_climateFilePath); 
                     pModel->m_climateStationElev = elev;
                     //delete pInfo;
                    // pInfo=NULL;
                     break;

                  default:
                     {
                     CString msg;
                     msg.Format( "Flow Error: Unrecognized climate data '%s'type reading file %s", cdtype, filename );
                     delete pInfo;
                     pInfo = NULL;
                     }
                  }  // end of: switch( type[ 0 ] )
         
               if ( pInfo != NULL )
                  {
                  //pModel->m_climateDataInfoArray.Add( pInfo );
                  pScenario->m_climateDataMap.SetAt( pInfo->m_type, pInfo );
                  pScenario->m_climateInfoArray.Add( pInfo );

                  if ( cdtype[ 0 ] != 'c' ) // No need to init GDAL libs for station data!
                     pInfo->m_pDataObj->InitLibraries();
                  }
               }  // end of: else ( valid info )
         
            pXmlClimate = pXmlClimate->NextSiblingElement( "climate" );
            }  // end of: while ( pXmlClimate != NULL )

         pModel->m_currentScenarioIndex = defaultIndex;
         if ( pModel->m_currentScenarioIndex >= defaultIndex )
            pModel->m_currentScenarioIndex = 0;

         pXmlScenario = pXmlScenario->NextSiblingElement( "scenario" );
         }  // end of: while ( pXmlScenario != NULL )
      }  // end of: if ( pXmlScenarios != NULL )
               
      //   if (type==0)
      //      {
      //      //pModel->m_pPrecipGrid = new GeoSpatialDataObj;
      //      //pModel->m_pTemperatureGrid = new GeoSpatialDataObj;
      //     // pModel->m_climateFilePath = path;
      //      pModel->m_climateFilePath = pModel->m_path+path;
      //
      //      // need to open,read
      //      }
      //   else
      //      {
      //      pModel->m_climateFilePath = pModel->m_path+path;
      //      pModel->ReadClimateStationData(pModel->m_climateFilePath = pModel->m_path+path);
      //      //pModel->m_climateFilePath = path;
      //      pModel->m_climateStationElev = elev;
      //      }
      //   }
    


   /*


   //---------- <climate> ----------------------------------
   TiXmlElement *pXmlClimate = pXmlRoot->FirstChildElement( "climate" ); 
   if ( pXmlClimate != NULL )
      {
      LPTSTR path;
      LPTSTR type;

      XML_ATTR climateAttrs[] = {
         // attr                 type          address                     isReq  checkCol
         { "type",               TYPE_STRING,   &type,                     true,   0 },
         { "path",               TYPE_STRING,   &path,                     true,   0 },
         { NULL,                 TYPE_NULL,     NULL,                      false,  0 } };
      
      bool ok = TiXmlGetAttributes( pXmlClimate, climateAttrs, filename );
      if ( ! ok )
         {
         CString msg; 
         msg.Format( _T("Flow: Misformed element reading <climate> attributes in input file %s"), filename );
         Report::ErrorMsg( msg );
         }
      else if (type=="NetCDF")
         {
         pModel->m_pPrecipGrid = new GeoSpatialDataObj;
         pModel->m_pTemperatureGrid = new GeoSpatialDataObj;
         pModel->m_climateFilePath = path;
         // need to open,read
         }
      elsea
         {
         pModel->ReadClimateStationData(path);
         pModel->m_climateFilePath = path;
         }
      }
      */
   //----------- <reservoirs> ------------------------------
   TiXmlElement *pXmlReservoirs = pXmlRoot->FirstChildElement( "reservoirs" ); 
   if ( pXmlReservoirs != NULL )
      {
      bool ok = true;

      LPCTSTR field = pXmlReservoirs->Attribute( "col" );
      if ( field == NULL )
         {
         CString msg; 
         msg.Format( _T("Flow: Missing 'col' attribute reading <reservoirs> tag in input file %s"), filename );
         Report::ErrorMsg( msg );
         }
      else
         ok = CheckCol( pModel->m_pStreamLayer, pModel->m_colResID, field, TYPE_INT, CC_MUST_EXIST );

      if ( ok )
         {
         TiXmlElement *pXmlRes = pXmlReservoirs->FirstChildElement( "reservoir" ); 

         while ( pXmlRes != NULL )
            {
            Reservoir *pRes = new Reservoir;
				LPCSTR reservoirType = NULL;
           
            float volume=0.0f;
            XML_ATTR resAttrs[] = {
               // attr                  type          address                           isReq  checkCol
               { "id",               TYPE_INT,      &(pRes->m_id),                      true,   0 },
               { "name",             TYPE_CSTRING,  &(pRes->m_name),                    true,   0 },
               { "path",             TYPE_CSTRING,  &(pRes->m_dir),                     false,   0 },
               { "initVolume",       TYPE_FLOAT,    &(volume),                          false,   0 },
               { "area_vol_curve",   TYPE_CSTRING,  &(pRes->m_areaVolCurveFilename),    false,   0 },
               { "av_dir",           TYPE_CSTRING,  &(pRes->m_avdir),                   false,   0 },
               { "minOutflow",       TYPE_FLOAT,    &(pRes->m_minOutflow),              false,   0 },
               { "rule_curve",       TYPE_CSTRING,  &(pRes->m_ruleCurveFilename),       false,   0 },
               { "buffer_zone",      TYPE_CSTRING,  &(pRes->m_bufferZoneFilename),      false,   0 },
               { "rc_dir",           TYPE_CSTRING,  &(pRes->m_rcdir),                   false,   0 },
               { "maxVolume",        TYPE_FLOAT,    &(pRes->m_maxVolume),               false,   0 },
               { "composite_rc",     TYPE_CSTRING,  &(pRes->m_releaseCapacityFilename), false,   0 },
               { "re_dir",           TYPE_CSTRING,  &(pRes->m_redir),                   false,   0 },
               { "maxPowerFlow",     TYPE_FLOAT,    &(pRes->m_gateMaxPowerFlow),        false,   0 },
               { "RO_rc",            TYPE_CSTRING,  &(pRes->m_RO_CapacityFilename),     false,   0 },
               { "spillway_rc",      TYPE_CSTRING,  &(pRes->m_spillwayCapacityFilename),false,   0 },
               { "cp_dir",           TYPE_CSTRING,  &(pRes->m_cpdir),                   false,   0 },
               { "rule_priorities",  TYPE_CSTRING,  &(pRes->m_rulePriorityFilename),    false,   0 },
               { "ressim_output_f",  TYPE_CSTRING,  &(pRes->m_ressimFlowOutputFilename),false,   0 },
               { "ressim_output_r",  TYPE_CSTRING,  &(pRes->m_ressimRuleOutputFilename),false,   0 },
               { "rp_dir",           TYPE_CSTRING,  &(pRes->m_rpdir),                   false,   0 },
               { "td_elev",          TYPE_FLOAT,    &(pRes->m_dam_top_elev),            false,   0 },  // Top of dam elevation
               { "fc1_elev",         TYPE_FLOAT,    &(pRes->m_fc1_elev),                false,   0 },  // Top of primary flood control zone
               { "fc2_elev",         TYPE_FLOAT,    &(pRes->m_fc2_elev),                false,   0 },  // Seconary flood control zone (if applicable)
               { "fc3_elev",         TYPE_FLOAT,    &(pRes->m_fc3_elev),                false,   0 },  // Tertiary flood control zone (if applicable)
               { "tailwater_elev",   TYPE_FLOAT,    &(pRes->m_tailwater_elevation),     false,   0 },
               { "turbine_efficiency",TYPE_FLOAT,   &(pRes->m_turbine_efficiency),      false,   0 },
               { "inactive_elev",    TYPE_FLOAT,    &(pRes->m_inactive_elev),           true,    0 },
					{ "reservoir_type",   TYPE_STRING,   &(reservoirType),						 true,	 0 },
					{ "release_freq",	    TYPE_INT,      &(pRes->m_releaseFreq),				 false,	 0 },
					{ NULL,               TYPE_NULL,     NULL,                               false,   0 } };

            
            ok = TiXmlGetAttributes( pXmlRes, resAttrs, filename, NULL );
            if ( ! ok )
               {
               CString msg; 
               msg.Format( _T("Flow: Misformed element reading <reservoir> attributes in input file %s"), filename );
               Report::ErrorMsg( msg );
               delete pRes;
               pRes = NULL;
               }
           
				switch (reservoirType[0])
				{
					case 'F':
					{
						pRes->m_reservoirType = ResType_FloodControl;
						CString msg;
						msg.Format("Flow: Reservoir %s is Flood Control", (LPCTSTR)pRes->m_name);
						break;
					}
					case 'R':
					{
						pRes->m_reservoirType = ResType_RiverRun;
						CString msg;
						msg.Format("Flow: Reservoir %s is River Run", (LPCTSTR)pRes->m_name);
						break;
					}
					case 'C':
					{
						pRes->m_reservoirType = ResType_CtrlPointControl;
						CString msg;
						msg.Format("Flow: Reservoir %s is Control Point Control", (LPCTSTR)pRes->m_name);
						break;
					}
					default:
					{
						CString msg("Flow: Unrecognized 'reservoir_type' attribute '");
						msg += reservoirType;
						msg += "' reading <reservoir> tag for Reservoir '";
						msg += (LPCSTR)pRes->m_name;
						//		msg += "'. This output will be ignored...";
						Report::ErrorMsg(msg);
						break;
					}
				}

				if (pRes != NULL)
				{
					pRes->InitializeReservoirVolume(volume);
					pModel->m_reservoirArray.Add(pRes);
				}

            pXmlRes = pXmlRes->NextSiblingElement( "reservoir" );
            }
         }
      }
   
   //----------- <control points> ------------------------------
   TiXmlElement *pXmlControlPoints = pXmlRoot->FirstChildElement( "controlPoints" ); 
   if ( pXmlControlPoints != NULL )
      {
      bool ok = true;

      if ( ok )
         {
         TiXmlElement *pXmlCP = pXmlControlPoints->FirstChildElement( "controlPoint" ); 

         while ( pXmlCP != NULL )
            {
            ControlPoint *pControl = new ControlPoint;

            XML_ATTR cpAttrs[] = {
               // attr               type         address                                isReq   checkCol
            { "name",               TYPE_CSTRING,   &(pControl->m_name),                 true,   0 },
            { "id",                 TYPE_INT,       &(pControl->m_id),                   false,  0 },
            { "path",               TYPE_CSTRING,   &(pControl->m_dir),                  true,   0 },
            { "control_point",      TYPE_CSTRING,   &(pControl->m_controlPointFileName), true,   0 },
            { "location",           TYPE_INT,       &(pControl->m_location),             true,   0 },
            { "reservoirs",         TYPE_CSTRING,   &(pControl->m_reservoirsInfluenced), true,   0 },
            { "local_col",          TYPE_INT,       &(pControl->localFlowCol),           false,  0 },
            { NULL,                  TYPE_NULL,     NULL,                                false,  0 } };
               
            ok = TiXmlGetAttributes( pXmlCP, cpAttrs, filename, NULL );
            if ( ! ok )
               {
               CString msg; 
               msg.Format( _T("Flow: Misformed element reading <controlPoint> attributes in input file %s"), filename );
               Report::ErrorMsg( msg );
               delete pControl;
               }
            else
               pModel->m_controlPointArray.Add( pControl );

            pXmlCP = pXmlCP->NextSiblingElement( "controlPoint" );
            }
         }
      }

   //-------- extra state variables next ------------
   StateVar *pSV = new StateVar;  // first is always "water"
   pSV->m_name  = "water";
   pSV->m_index = 0;
   pSV->m_uniqueID = -1;
   pModel->AddStateVar( pSV );
   LPTSTR appTablePath = NULL;
   LPTSTR paramTablePath = NULL;
   LPCTSTR fieldname = NULL;

   TiXmlElement *pXmlStateVars = pXmlRoot->FirstChildElement( "additional_state_vars" ); 
   if (pXmlStateVars != NULL)
      {
      TiXmlElement *pXmlStateVar = pXmlStateVars->FirstChildElement("state_var");

      while (pXmlStateVar != NULL)
         {
         pSV = new StateVar;

         XML_ATTR svAttrs[] = {
            // attr              type          address                   isReq  checkCol
            { "name", TYPE_CSTRING, &(pSV->m_name), true, 0 },
            { "unique_id", TYPE_INT, &(pSV->m_uniqueID), true, 0 },
            { "integrate", TYPE_INT, &(pSV->m_isIntegrated), false, 0 },

            { "fieldname", TYPE_STRING, &fieldname, false, 0 },

            { "applicationTable", TYPE_STRING, &appTablePath, false, 0 },
            { "parameterTable", TYPE_STRING, &paramTablePath, false, 0 },

            { "scalar", TYPE_FLOAT, &(pSV->m_scalar), false, 0 },



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


         DataObj *pDataObj;
         pDataObj = new FDataObj;

         int rows = pDataObj->ReadAscii(pModel->m_path + appTablePath);

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
                  msg += "; not found in IDU coverage";
                  Report::ErrorMsg(msg);
                  }
               }

            pTable->CreateMap();
            pSV->m_tableArray.Add(pTable);
            }

         // read in esv parameter  Table
         //pSV->m_paramTablePath = paramTablePath;
         pSV->m_paramTable = new VDataObj;
         pSV->m_paramTable->ReadAscii(paramTablePath, ',');

         pXmlStateVar = pXmlStateVar->NextSiblingElement("state_var");
         }
      }

   // set state variable count.  Assumes all pools have the same number of extra state vars

   pModel->m_hruSvCount = pModel->m_reachSvCount = pModel->m_reservoirSvCount = pModel->GetStateVarCount();
  
   //// fluxes next
   //TiXmlElement *pXmlFluxes = pXmlRoot->FirstChildElement( "fluxes" ); 
   //if ( pXmlFluxes != NULL )
   //   {
   //   // get fluxes next
   //   TiXmlElement *pXmlFlux = pXmlFluxes->FirstChildElement( "flux" );
   //   
   //   while ( pXmlFlux != NULL )
   //      {
   //      FluxInfo *pFluxInfo = new FluxInfo;
   //
   //      //  Specifies all fluxes.  
   //      //    type:   1 - reach, 2=catchment, 3=hru, 4=hruLayer
   //      //    query:  where does this flux apply
   //      //    source: fn:<dllpath:function> for functional, db:<datasourcepath:columnname> for datasets
   //      LPTSTR sourceLocation = NULL;
   //      LPTSTR sinkLocation   = NULL;
   //      LPTSTR stateVars  = NULL;
   //      LPTSTR fluxType   = NULL;
   //
   //      XML_ATTR attrs[] = {
   //         // attr             type            address                         isReq checkCol
   //         { "name",           TYPE_CSTRING,   &(pFluxInfo->m_name),          true,  0 },
   //         { "path",           TYPE_CSTRING,   &(pFluxInfo->m_path),          true,  0 },
   //         { "description",    TYPE_CSTRING,   &(pFluxInfo->m_description),   false, 0 },
   //         { "type",           TYPE_STRING,    &fluxType,                     false, 0 },
   //         { "source_type",    TYPE_STRING,    &sourceLocation,               false, 0 },
   //         { "source_query",   TYPE_CSTRING,   &(pFluxInfo->m_sourceQuery),   false, 0 },
   //         { "sink_type",      TYPE_STRING,    &sinkLocation,                 false, 0 },
   //         { "sink_query",     TYPE_CSTRING,   &(pFluxInfo->m_sinkQuery),     false, 0 },
   //         { "flux_handler",   TYPE_CSTRING,   &(pFluxInfo->m_fnName),        false, 0 },
   //         { "field",          TYPE_CSTRING,   &(pFluxInfo->m_fieldName),     false, 0 },
   //         { "state_vars",     TYPE_STRING,    &stateVars,                    false, 0 },
   //         { "init_handler",   TYPE_CSTRING,   &(pFluxInfo->m_initFn),        false, 0 },
   //         { "initRun_handler",TYPE_CSTRING,   &(pFluxInfo->m_initRunFn),     false, 0 },
   //         { "endRun_handler", TYPE_CSTRING,   &(pFluxInfo->m_endRunFn),      false, 0 },
   //         { "initInfo",       TYPE_CSTRING,   &(pFluxInfo->m_initInfo),      false, 0 },
   //         { NULL,             TYPE_NULL,      NULL,                          true,  0 }
   //         };
   //
   //      bool ok = TiXmlGetAttributes( pXmlFlux, attrs, filename );
   //
   //      if ( !ok )
   //         {
   //         CString msg; 
   //         msg.Format( _T("Flow: Misformed flux element reading <flux> attributes in input file %s"), filename );
   //         Report::ErrorMsg( msg );
   //         delete pFluxInfo;
   //         }
   //      else
   //         {
   //         pFluxInfo->m_type = FT_UNDEFINED;
   //
   //         if ( fluxType != NULL) 
   //            {
   //            if ( *fluxType =='p' || *fluxType == 'P' )
   //               pFluxInfo->m_type = FT_PRECIP;
   //            }
   //
   //         // good xml, figure everything out
   //         // start with source info
   //         bool okSource = true;
   //         bool okSink   = true;
   //
   //         if ( sourceLocation != NULL )
   //            {
   //            okSource = pModel->ParseFluxLocation( sourceLocation, pFluxInfo->m_sourceLocation, pFluxInfo->m_sourceLayer );
   //
   //            // trap errors
   //            if ( ! okSource )
   //               {
   //               CString msg; 
   //               msg.Format( _T("Flow: Unrecognized flux <source_type> attribute '%s' in input file %s - this should be 'hruLayer', 'reach', 'reservoir', or 'gw' "), sourceLocation, filename );
   //               Report::ErrorMsg( msg );
   //               }
   //
   //            if ( pFluxInfo->m_sourceLocation == FL_HRULAYER && pFluxInfo->m_sourceLayer >= pModel->GetHRULayerCount() )
   //               {
   //               CString msg;
   //               msg.Format( "Flow: layer specified in Flux '%s' exceeds layer count specfied in <catchment> definition.  This flux will be disabled.", (LPCTSTR) pFluxInfo->m_name );
   //               Report::WarningMsg( msg );
   //               pFluxInfo->m_use = false;
   //               }
   //            }
   //
   //         // next, sinks
   //         if ( sinkLocation != NULL )
   //            {
   //            okSink = pModel->ParseFluxLocation( sinkLocation, pFluxInfo->m_sinkLocation, pFluxInfo->m_sinkLayer );
   //            if ( ! okSink )
   //               {
   //               CString msg; 
   //               msg.Format( _T("Flow: Unrecognized flux <sink_type> attribute '%s' in input file %s - this should be 'hruLayer', 'reach', 'reservoir', or 'gw' "), sinkLocation, filename );
   //               Report::ErrorMsg( msg );
   //               }
   //
   //            if ( pFluxInfo->m_sinkLocation == FL_HRULAYER && pFluxInfo->m_sinkLayer >= pModel->GetHRULayerCount() )
   //               {
   //               CString msg;
   //               msg.Format( "Flow: layer specified in Flux '%s' exceeds layer count specfied in <catchment> definition.  This flux will be disabled.", (LPCTSTR) pFluxInfo->m_name );
   //               Report::WarningMsg( msg );
   //               pFluxInfo->m_use = false;
   //               }
   //            }
   //
   //         if ( okSource == false || okSink == false )
   //            delete pFluxInfo;
   //         else
   //            {
   //            // state vars next...
   //            if ( stateVars == NULL )
   //               {
   //               // assume default is "water" if not specified
   //               pFluxInfo->m_stateVars.Add( pModel->GetStateVar( 0 ) );
   //               }
   //            else
   //               {
   //               // parse state vars
   //               TCHAR *buff = new TCHAR[ lstrlen( stateVars )+1 ];
   //               lstrcpy( buff, stateVars );
   //               LPTSTR next;
   //               TCHAR *varName = _tcstok_s( buff, _T("|,"), &next );
   //
   //               while ( varName != NULL )
   //                  {
   //                  StateVar *pSV = pModel->FindStateVar( varName );
   //
   //                  if ( pSV != NULL )
   //                     pFluxInfo->m_stateVars.Add( pSV );
   //                  else
   //                     {
   //                     CString msg( "Flow: Unrecognized state variable '" );
   //                     msg += varName;
   //                     msg += "' found while reading <flux> '";
   //                     msg += pFluxInfo->m_name;
   //                     msg += "' attribute 'state_vars'";
   //                     Report::WarningMsg( msg );
   //                     }
   //
   //                  varName = _tcstok_s( NULL, _T( "|," ), &next );
   //                  }
   //
   //               delete [] buff;
   //               }
   //
   //            pModel->AddFluxInfo( pFluxInfo );
   //            }
   //         }
   //
   //      pXmlFlux = pXmlFlux->NextSiblingElement( "flux" );
   //      }
   //   }
   //
   // tables next
   TiXmlElement *pXmlTables = pXmlRoot->FirstChildElement( "tables" ); 
   if ( pXmlTables != NULL )
      {
      // get tables next
      TiXmlElement *pXmlTable = pXmlTables->FirstChildElement( "table" );
      while ( pXmlTable != NULL )
         {
         LPCTSTR name = NULL;
         LPCTSTR description = NULL;
         LPCTSTR fieldname = NULL;
         LPCTSTR type = NULL;
         LPCTSTR source = NULL;
         XML_ATTR attrs[] = {
            // attr             type            address                         isReq checkCol
            { "name",           TYPE_STRING,   &name,               true,  0 },
            { "description",    TYPE_STRING,   &description,        false, 0 },
            { "type",           TYPE_STRING,   &type,               true,  0 },
            { "col",            TYPE_STRING,   &fieldname,          false, 0 },
            { "source",         TYPE_STRING,   &source,             true,  0 },
            { NULL,             TYPE_NULL,     NULL,                true,  0 }
            };

         bool ok = TiXmlGetAttributes( pXmlTable, attrs, filename );

         if ( !ok )
            {
            CString msg; 
            msg.Format( _T("Flow: Misformed table element reading <table> attributes in input file %s"), filename );
            Report::ErrorMsg( msg );
            }
         else
            {
            DataObj *pDataObj;

            if ( *type == 'v' )
               pDataObj = new VDataObj;
            else if ( *type == 'i' )
               pDataObj = new IDataObj;
            else
               pDataObj = new FDataObj;

            int rows = pDataObj->ReadAscii( pModel->m_path+source );

            if ( rows <= 0 )
               {
               CString msg( _T( "Flow: Flux table source error: Unable to open/read ") );
               msg += source;
               Report::ErrorMsg( msg );
               delete pDataObj;
               }
            else
               {
               ParamTable *pTable = new ParamTable;

               pTable->m_pTable = pDataObj;
               pTable->m_pInitialValuesTable = pDataObj;  ///???? 
               pTable->m_pTable->SetName(name);
               pTable->m_type = pDataObj->GetDOType();
               pTable->m_description = description;
               
               if ( fieldname != NULL )
                  {
                  pTable->m_fieldName = fieldname;
                  pTable->m_iduCol = pIDULayer->GetFieldCol( fieldname );

                  if ( pTable->m_iduCol < 0 )
                     {
                     CString msg( _T( "Flow: Flux table source error: column '" ) );
                     msg += fieldname;
                     msg += "; not found in IDU coverage";
                     Report::ErrorMsg( msg );
                     }
                  }

               pTable->CreateMap();

               pModel->m_tableArray.Add( pTable );
               }
            }
    
         pXmlTable = pXmlTable->NextSiblingElement( "table" );
         }
      }

// Parameter Estimation 
   TiXmlElement *pXmlParameters = pXmlRoot->FirstChildElement( "parameterEstimation" ); 
   if ( pXmlParameters != NULL )
      {
      float numberOfRuns;
      float numberOfYears;
      bool estimateParameters;
      float saveResultsEvery;
      float nsThreshold;
      long rnSeed = -1;

      XML_ATTR paramAttrs[] = {
         { "estimateParameters",TYPE_BOOL,    &estimateParameters, true,   0 },
         { "numberOfRuns",      TYPE_FLOAT,   &numberOfRuns,       true,   0 },
         { "numberOfYears",     TYPE_FLOAT,   &numberOfYears,      true,   0 },
         { "saveResultsEvery",  TYPE_FLOAT,   &saveResultsEvery,   true,   0 },
         { "nsThreshold",       TYPE_FLOAT,   &nsThreshold,        false,   0 },
         { "randomSeed",        TYPE_LONG,    &rnSeed,             false,  0 },
         { NULL,                TYPE_NULL,    NULL,                false,  0 } };
      
      bool ok = TiXmlGetAttributes( pXmlParameters, paramAttrs, filename );
      if ( ! ok )
         {
         CString msg; 
         msg.Format( _T("Flow: Misformed element reading <parameterEstimation> attributes in input file %s"), filename );
         Report::ErrorMsg( msg );
         }
      else
         {
         pModel->m_estimateParameters=estimateParameters;
         pModel->m_numberOfRuns=(int)numberOfRuns;
         pModel->m_numberOfYears = (int)numberOfYears;
         pModel->m_saveResultsEvery = (int)saveResultsEvery;
         pModel->m_nsThreshold = nsThreshold;
         pModel->m_rnSeed = rnSeed;

         if ( rnSeed >= 0 )
            pModel->m_randUnif1.SetSeed( rnSeed );

         //if ( outputPath.IsEmpty() )
         //   {
         //   pModel->m_paramEstOutputPath = pModel->m_path;
         //   pModel->m_paramEstOutputPath += "\\output\\";
         //   }
         //else
         //   pModel->m_paramEstOutputPath += outputPath;
         }
      // get tables next
      TiXmlElement *pXmlParameter = pXmlParameters->FirstChildElement( "parameter" );
      while ( pXmlParameter != NULL )
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
            { "table",    TYPE_STRING,  &table,        true,  0 },
            { "name",     TYPE_STRING,   &name,       false, 0 },
            { "value",    TYPE_FLOAT,   &value,       false, 0 },
            { "minValue", TYPE_FLOAT,   &minValue,    false, 0 },
            { "maxValue", TYPE_FLOAT,   &maxValue,    false, 0 },
            { "distributeSpatially",  TYPE_BOOL,   &distributeSpatially,    false, 0 },
            { NULL,       TYPE_NULL,     NULL,        true,  0 }
            };

         bool ok = TiXmlGetAttributes( pXmlParameter, attrs, filename );

         if ( !ok )
            {
            CString msg; 
            msg.Format( _T("Flow: Misformed element reading <reach_location> attributes in input file %s"), filename );
            Report::ErrorMsg( msg );
            }
         else
            {
            ParameterValue *pParam = new ParameterValue;
            if ( name != NULL )
               {
               pParam->m_name = name;
               pParam->m_table = table;
               pParam->m_value = value;
               pParam->m_minValue = minValue;
               pParam->m_maxValue = maxValue;
               pParam->m_distribute = distributeSpatially;

               }
            pModel->m_parameterArray.Add( pParam );
            }     
         pXmlParameter = pXmlParameter->NextSiblingElement( "parameter" );
         }
      }


   // output expressions
   TiXmlElement *pXmlModelOutputs = pXmlRoot->FirstChildElement( "outputs" ); 
   if ( pXmlModelOutputs != NULL )
      {
      TiXmlElement *pXmlGroup = pXmlModelOutputs->FirstChildElement( "output_group" );

      while ( pXmlGroup != NULL )
         {
         ModelOutputGroup *pGroup = new ModelOutputGroup;         
         pGroup->m_name = pXmlGroup->Attribute( "name" ); 
         if ( pGroup->m_name.IsEmpty() )
            pGroup->m_name = _T( "Flow Model Output" );

         gpModel->m_modelOutputGroupArray.Add( pGroup );

         TiXmlElement *pXmlModelOutput = pXmlGroup->FirstChildElement( "output" );

         while ( pXmlModelOutput != NULL )
            {
            LPTSTR name   = NULL;
            LPTSTR query  = NULL;
            LPTSTR expr   = NULL;
            bool   inUse  = true;
            LPTSTR type   = NULL;
            LPTSTR domain = NULL;
            LPCSTR obs = NULL;
            LPCSTR format = NULL;
            int site=-1;
      
            XML_ATTR attrs[] = {
                  // attr           type         address                 isReq checkCol
                  { "name",         TYPE_STRING, &name,                  true,   0 },
                  { "query",        TYPE_STRING, &query,                 false,  0 },
                  { "value",        TYPE_STRING, &expr,                  true,   0 },
                  { "in_use",       TYPE_BOOL,   &inUse,                 false,  0 },
                  { "type",         TYPE_STRING, &type,                  true,   0 },
                  { "domain",       TYPE_STRING, &domain,                true,   0 },
                  { "obs",          TYPE_STRING, &obs,                   false,  0 },
                  { "format",       TYPE_STRING, &format,                false,  0 },
                  { "site",         TYPE_INT,    &site,                  false,  0 },
                  { NULL,           TYPE_NULL,   NULL,                   false,  0 } };
      
            bool ok = TiXmlGetAttributes( pXmlModelOutput, attrs, filename );
      
            if ( !ok )
               {
               CString msg; 
               msg.Format( _T("Flow: Misformed element reading <output> attributes in input file %s"), filename );
               Report::ErrorMsg( msg );
               }
            else
               {
               ModelOutput *pOutput = new ModelOutput;
   
               pOutput->m_name     = name;
               pOutput->m_queryStr = query;
               
               pOutput->m_inUse    = inUse;
               if (obs != NULL)
                  {
                  CString fullPath;
                  if ( PathManager::FindPath( obs, fullPath ) < 0 )
                     {
                     CString msg;
                     msg.Format( "Flow: Unable to find observation file '%s' specified for Model Output '%s'", obs, name );
                     Report::WarningMsg( msg );
                     pOutput->m_inUse = false;
                     }
                  else
                     {
                     pOutput->m_pDataObjObs = new FDataObj;
                     //CString inFile;
                     //inFile.Format("%s%s",pModel->m_path,obs);    // m_path is not slash-terminated
                     int rows=-1;
                     switch( format[0] )
                        {
                        case 'E':      rows = pOutput->m_pDataObjObs->ReadAscii(fullPath);        break;
                        case 'U':      rows = gpModel->ReadUSGSFlow(fullPath, *pOutput->m_pDataObjObs );  break;
                        case 'R':      rows = gpModel->ReadUSGSFlow(fullPath, *pOutput->m_pDataObjObs, 1 );  break;//set dataType to 1, representing reservoir levels
                        case 'T':      rows = gpModel->ReadUSGSFlow(fullPath, *pOutput->m_pDataObjObs, 2);  break;//set dataType to 2, representing stream temperatures
                        default:
                           {
                           CString msg( "Flow: Unrecognized observation 'format' attribute '" );
                           msg += type;
                           msg += "' reading <output> tag for '";
                           msg += name;
                           msg += "'. This output will be ignored...";
                           Report::ErrorMsg( msg );
                           pOutput->m_inUse = false;
                           }
                        }
   
                     if ( rows <= 0 ) 
                        {
                        CString msg;
                        msg.Format( "Flow: Unable to load observation file '%s' specified for Model Output '%s'", (LPCTSTR) fullPath, name );
                        Report::WarningMsg( msg );

                        delete pOutput->m_pDataObjObs;
                        pOutput->m_pDataObjObs = NULL;
                        pOutput->m_inUse = false;
                        }

                     pModel->m_numQMeasurements++;
                     }               
                  }

               pOutput->m_modelType = MOT_SUM;
               if ( type != NULL )
                  {   
                  switch( type[0] )
                     {
                     case 'S':
                     case 's':      pOutput->m_modelType = MOT_SUM;         break;
                     case 'A':
                     case 'a':      pOutput->m_modelType = MOT_AREAWTMEAN;  break;
                     case 'P':
                     case 'p':      pOutput->m_modelType = MOT_PCTAREA;     break; 
                     default:
                        {
                        CString msg( "Flow: Unrecognized 'type' attribute '" );
                        msg += type;
                        msg += "' reading <output> tag for '";
                        msg += name;
                        msg += "'. This output will be ignored...";
                        Report::ErrorMsg( msg );
                        pOutput->m_inUse = false;
                        }
                     }
                  }


               pOutput->m_modelDomain = MOD_IDU;

               if ( domain != NULL )
                  {
                  switch( domain[0] )
                     {
                     case 'i':      pOutput->m_modelDomain = MOD_IDU;         break;
                     case 'h':      pOutput->m_modelDomain = MOD_HRU;         break;
                     case '_':      pOutput->m_modelDomain = MOD_HRU_IDU;         break;
                     case 'l':      pOutput->m_modelDomain = MOD_HRULAYER;    
                        {
                        TCHAR *buffer = new TCHAR[ 255 ];
                        TCHAR *nextToken;
                        lstrcpy( buffer, (LPCTSTR) domain );

                        TCHAR *col = _tcstok_s( buffer, ":", &nextToken );
                        CString p;
                        while( col != NULL )
                           {
                           p = col;                   
                           col = _tcstok_s( NULL, ":", &nextToken );
                           }
                        pOutput->m_number=atoi(p);
                        delete [] buffer;
                        }
                        break;    // need mechanism to ID which layer
                     case 'r':      pOutput->m_modelDomain = MOD_REACH;       

                        break;  
                     case 's':      pOutput->m_modelDomain = MOD_RESERVOIR; break;
                     default:
                        {
                        CString msg( "Flow: Unrecognized 'domain' attribute '" );
                        msg += type;
                        msg += "' reading <output> tag for '";
                        msg += name;
                        msg += "'. This output will be ignored...";
                        Report::ErrorMsg( msg );
                        pOutput->m_inUse = false;
                        }
                     }
                  }
            
               if ( expr != NULL )
                  {
                  switch( expr[0] )
                     {
                     case 'e': 
                        {
                        TCHAR *buffer = new TCHAR[ 255 ];
                        TCHAR *nextToken=NULL;
                        lstrcpy( buffer, (LPCTSTR) expr );

                        TCHAR *col = _tcstok_s( buffer, ":", &nextToken );
                        
                        CString p;
                        p = col;
                        pOutput->m_exprStr  = p;
                        while( col != NULL )
                           {
                           p = col;                   
                           col = _tcstok_s( NULL, ":", &nextToken );
                           }
                        pOutput->m_esvNumber=atoi(p);
                        delete [] buffer;
                        }
                     break;
                                          
                     default: 
                        pOutput->m_exprStr  = expr;
                     }
                  }

               if (site > 0)
                  pOutput->m_siteNumber=site;

             //  pGroup->Add( pOutput );   
               pOutput->Init( pIDULayer );
               
               if ( pOutput->m_modelDomain == MOD_REACH )
                  pOutput->InitStream(pModel->m_pStreamLayer);
            
               // check to make sure the query for any stream sites returns a reach
               bool pass = true;
               if (pOutput->m_modelDomain == MOD_REACH  && pOutput->m_pDataObjObs)
                  {
                  pass = false;
                  for (MapLayer::Iterator i = pModel->m_pStreamLayer->Begin(); i < pModel->m_pStreamLayer->End(); i++)
                     { 
                     if ( pOutput->m_pQuery != NULL)
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

               // check to make sure the query for any HRU sites returns atleast 1 polygon
               // bool pass = true;
               if (pOutput->m_modelDomain == MOD_HRU )
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

               if ( pass )
                  pGroup->Add( pOutput );
               else
                  delete pOutput;
               }

            pXmlModelOutput = pXmlModelOutput->NextSiblingElement( "output" );
            }
         
         pXmlGroup = pXmlGroup->NextSiblingElement( "output_group" );
         }
      }

/*
   // Stream Sample Locations next
   TiXmlElement *pXmlReachess = pXmlRoot->FirstChildElement( "sample_locations" ); 
   if ( pXmlReachess != NULL )
      {
      // get tables next
      TiXmlElement *pXmlReach = pXmlReachess->FirstChildElement( "reach_location" );
      while ( pXmlReach != NULL )
         {
         LPCTSTR name = NULL;
         LPCTSTR description = NULL;
         LPCTSTR fieldname = NULL;
         LPCTSTR col = NULL;
         LPCTSTR meas = NULL;
         int id = NULL;
         XML_ATTR attrs[] = {
            // attr             type            address                         isReq checkCol
            { "name",           TYPE_STRING,   &name,               true,  0 },
            { "description",    TYPE_STRING,   &description,        false, 0 },
            { "col",            TYPE_STRING,   &fieldname,          false, 0 },
            { "id",             TYPE_INT,      &id,                 false, 0 },
            { "measured"      , TYPE_STRING,   &meas,               false, 0 },
            { NULL,             TYPE_NULL,     NULL,                true,  0 }
            };

         bool ok = TiXmlGetAttributes( pXmlReach, attrs, filename );

         if ( !ok )
            {
            CString msg; 
            msg.Format( _T("Flow: Misformed element reading <reach_location> attributes in input file %s"), filename );
            Report::ErrorMsg( msg );
            }
         else
            {
            ReachSampleLocation *pSample = new ReachSampleLocation;
            if ( fieldname != NULL )
               {
               pSample->m_fieldName = fieldname;
               pSample->m_iduCol = pIDULayer->GetFieldCol( fieldname );
               pSample->m_id = id;

               if ( pSample->m_iduCol < 0 )
                  {
                  CString msg( _T( "Flow: Reach sample source error: column '" ) );
                  msg += fieldname;
                  msg += "; not found in IDU coverage";
                  Report::ErrorMsg( msg );
                  }
               }

            pSample->m_name = name;
            if (meas != NULL)
               {
               pSample->m_pMeasuredData = new FDataObj;
               CString inFile;
               inFile.Format("%s%s",pModel->m_path,meas);
               int rows = gpModel->ReadUSGSFlow(inFile, *pSample->m_pMeasuredData );

               if ( rows <= 0 ) 
                  {
                  delete pSample->m_pMeasuredData;
                  pSample->m_pMeasuredData = NULL;
                  }
               //CString msg;
               //msg.Format("\\envision\\StudyAreas\\WW2100\\McKenzie\\DischargeData\\%s.csv",pSample->m_name);
               //pSample->m_pMeasuredData->WriteAscii(msg);
               pModel->m_numQMeasurements++;
               }
            pModel->m_reachSampleLocationArray.Add( pSample );
            }
         
         pXmlReach = pXmlReach->NextSiblingElement( "reach_location" );
         }
         */
      // get grids
      /*
      TiXmlElement *pXmlGrid = pXmlReachess->FirstChildElement( "grid_location" );
      while ( pXmlGrid != NULL )
         {
         LPCTSTR name = NULL;
         float x = NULL;
         float y = NULL;
         int id = NULL;
         XML_ATTR attrs[] = {
            // attr             type            address                         isReq checkCol
            { "name",           TYPE_STRING,   &name,       true,  0 },
            { "x",              TYPE_FLOAT,    &x,          false, 0 },
            { "y",              TYPE_FLOAT,    &y,          false, 0 },
            { NULL,             TYPE_NULL,     NULL,        true,  0 }
            };

         bool ok = TiXmlGetAttributes( pXmlGrid, attrs, filename );

         if ( !ok )
            {
            CString msg; 
            msg.Format( _T("Flow: Misformed element reading <reach_location> attributes in input file %s"), filename );
            Report::ErrorMsg( msg );
            }
         else
            {
            ReachSampleLocation *pSample = new ReachSampleLocation;
            if ( name != NULL )
               {
               pSample->m_fieldName = name;
               }

            pSample->m_name = name;
            Map *pMap = pIDULayer->GetMapPtr();

            MapLayer *pElev = pMap->GetLayer("Elevation");//Assumes this 
            pElev->GetGridCellFromCoord(x,y,pSample->row,pSample->col);//kbv
            pModel->m_reachSampleLocationArray.Add( pSample );
            }
         
         pXmlGrid = pXmlGrid->NextSiblingElement( "grid_location" );
         }
      } */

   // video capture
   TiXmlElement *pXmlVideoCapture = pXmlRoot->FirstChildElement( "video_capture" ); 
   if ( pXmlVideoCapture != NULL )
      {
      //pModel->m_useRecorder = true;
      pModel->m_useRecorder = true;
      int frameRate = 30;

      XML_ATTR attrs[] = {
            // attr           type         address                 isReq checkCol
            { "use",          TYPE_BOOL,   &pModel->m_useRecorder, false,  0 },
            { "frameRate",    TYPE_INT,    &frameRate,             false,  0 },
            { NULL,           TYPE_NULL,   NULL,                   false,  0 } };

      bool ok = TiXmlGetAttributes( pXmlVideoCapture, attrs, filename );

      if ( !ok )
         {
         CString msg; 
         msg.Format( _T("Flow: Misformed element reading <video_capture> attributes in input file %s"), filename );
         Report::ErrorMsg( msg );
         }
      else
         {
         //pModel->m_pVideoRecorder = new VideoRecorder;
   
         TiXmlElement *pXmlMovie = pXmlVideoCapture->FirstChildElement( "movie" ); 

         while( pXmlMovie != NULL )
            {
            LPTSTR field = NULL;
            LPTSTR file = NULL;

            XML_ATTR attrs[] = {
               // attr      type            address   isReq checkCol
               { "col",     TYPE_STRING,   &field,    true,   1 },
               { "file",    TYPE_STRING,   &file,     true,   0 },
               { NULL,      TYPE_NULL,      NULL,     false,  0 } };

            bool ok = TiXmlGetAttributes( pXmlMovie, attrs, filename, pIDULayer );

            if ( !ok )
               {
               CString msg; 
               msg.Format( _T("Flow: Misformed element reading <movie> attributes in input file %s"), filename );
               Report::ErrorMsg( msg );
               }
            else
               {
               int col = pIDULayer->GetFieldCol( field );

               if ( col >= 0 )
                  {
                  int vrID = EnvAddVideoRecorder( /*VRT_MAPPANEL*/ 2, "Flow Results", file, 30, VRM_CALLDIRECT, col );
                  pModel->m_vrIDArray.Add( vrID );
                  }
               }

            pXmlMovie = pXmlMovie->NextSiblingElement( "movie" );
            }
         }
      }
   
   return true;
   }


bool FlowModel::ParseFluxLocation( LPCTSTR typeStr, FLUXLOCATION &type, int &layer )
   {
   if ( *typeStr == 'h' ) // hruLayer
      {
      type = FL_HRULAYER;
      LPCTSTR colon = _tcschr( typeStr, ':' );

      if ( colon )
         {
         if ( *(colon+1) == '*' )
            layer = -1;
         else
            layer = atoi( colon+1 );
         }
      else
         layer = -1;  // applies to all layers
      }
         
   else if ( lstrcmpi( typeStr, "reach" ) == 0 )
      type = FL_REACH;

   else if ( lstrcmpi( typeStr, "reservoir" ) == 0 )
      type = FL_RESERVOIR;

   else if ( lstrcmpi( typeStr, "gw" ) == 0 )
      type = FL_GROUNDWATER;

   else
      return false;

   return true;
   }
         


int FlowModel::IdentifyMeasuredReaches(void)
   {
   int count = 0;

   //for ( MapLayer::Iterator i=this->m_pStreamLayer->Begin(); i < m_pStreamLayer->End(); i++ )
   for ( int i=0; i < (int) m_reachArray.GetSize(); i++ )
       {
       Reach *pReach = m_reachArray[ i ];

       int polyIndex = pReach->m_polyIndex;

       for ( int k=0; k < (int) this->m_modelOutputGroupArray.GetSize(); k++ )
         {
         ModelOutputGroup *pGroup = m_modelOutputGroupArray[ k ];

         for ( int j=0; j < (int) pGroup->GetSize(); j++ )
            {
            ModelOutput *pOutput = pGroup->GetAt( j );

            if ( pOutput->m_modelDomain == MOD_REACH && pOutput->m_pQuery != NULL )
               {
               // does it pass the query?
               bool result = false;
               bool ok = pOutput->m_pQuery->Run( polyIndex, result );

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



int FlowModel::ParseHRULayerDetails(CString &aggCols, int detail)
   {
   TCHAR *buffer = new TCHAR[ aggCols.GetLength()+1 ];
   TCHAR *nextToken = NULL;
   lstrcpy( buffer, (LPCTSTR) aggCols );

   TCHAR *col = _tcstok_s( buffer, ":", &nextToken );

   int colCount = 0;
   int numCols=0;
   while( col != NULL )
      {
      CString p= col;
      switch( detail)
         {
         case 0:
            m_hruLayerNames.Add(p);
            break;
         case 1:
            m_hruLayerDepths.Add(p);
            break;
         case 2:
            m_initWaterContent.Add(p);
            break;
         case 4:
            break;
         }
      numCols++;
      col = _tcstok_s( NULL, ":", &nextToken );
      }

   delete [] buffer;
   return numCols;
   }


bool FlowModel::InitializeParameterEstimationSampleArray(void)
   {
   int moCount = 0;//number of outputs with measured time series
   for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];   
      for ( int i=0; i < (int) pGroup->GetSize(); i++ )
         {
         if ( pGroup->GetAt( i )->m_pDataObjObs!=NULL)
            {
            moCount++;
            FDataObj *pErrorStatData = new FDataObj(4,0);
           // FDataObj *pParameterData = new FDataObj((int)m_parameterArray.GetSize(),0);
            FDataObj *pDischargeData = new FDataObj(1,1);
           // m_mcOutputTables.Add(pParameterData);
            m_mcOutputTables.Add(pErrorStatData);
            m_mcOutputTables.Add(pDischargeData);
            }
         }
      }
   m_pParameterData = new FDataObj((int)m_parameterArray.GetSize()+1,0);
   m_pParameterData->SetLabel( 0, "RunNumber"  );
   for ( int j=0; j < (int) m_parameterArray.GetSize(); j++)
      {
      CString name;
      name.Format("%s:%s", (LPCTSTR) m_parameterArray.GetAt(j)->m_table, (LPCTSTR) m_parameterArray.GetAt(j)->m_name);
      m_pParameterData->SetLabel( j+1, name  );
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
         m_pErrorStatData->SetLabel( meas, "NS_"+m_reachSampleLocationArray.GetAt(j)->m_name  );
         meas++;
         m_pErrorStatData->SetLabel( meas, "NS_LN_"+m_reachSampleLocationArray.GetAt(j)->m_name  );
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
   m_pHRUPrecipData->SetLabel( 0, "Time (Days)" );

   m_pHRUETData = new FDataObj( int( m_reachSampleLocationArray.GetSize() )+1, 0 );  
   m_pHRUETData->SetLabel( 0, "Time (Days)" );
   CString label;

   // for each sample location, allocate and label an FDataObj for storing HRULayer water contents
   for (int i=0; i < (int) m_reachSampleLocationArray.GetSize();i++)
      {
      FDataObj *pHruLayerData = new FDataObj(m_soilLayerCount+m_vegLayerCount+1,0);
      pHruLayerData->SetLabel( 0, "Time (Days)" );

      for (int j=0; j < m_soilLayerCount+m_vegLayerCount; j++)
        {
        label.Format( "%s", (LPCTSTR) m_hruLayerNames.GetAt(j));
        pHruLayerData->SetLabel( j+1, (char*)(LPCSTR) label );
        }
      m_reachHruLayerDataArray.Add( pHruLayerData );
      }


   //next look for any extra state variables
   for (int l=0;l<m_hruSvCount-1;l++)
      {
      // for each sample location, allocate and label an FDataObj for storing HRULayer water contents
      for (int i=0; i < (int) m_reachSampleLocationArray.GetSize();i++)
         {
         FDataObj *pHruLayerData = new FDataObj(m_soilLayerCount+m_vegLayerCount+1,0);
         pHruLayerData->SetLabel( 0, "Time (Days)" );

         for (int j=0; j < m_soilLayerCount+m_vegLayerCount; j++)
            {
            label.Format( "%s", (LPCTSTR) m_hruLayerNames.GetAt(j));
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
         label.Format( "%s", (LPCTSTR) pSampleLocation->m_name, i );
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
         pData->SetLabel( 0, "Time (Days)" );
         label.Format( "%s Mod", pSampleLocation->m_name );
         pData->SetLabel( 1, label );
         label.Format( "%s Meas", pSampleLocation->m_name );
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
  
void FlowModel::CollectData( int dayOfYear )
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
      if (!m_estimateParameters)
         {
         float *data1 = new float[7];
         data1[0] = (float) m_timeInRun;
         float tMean = 0.0f; float precip = 0.0f; float ws = 0.0f; float wd = 0.0f; float shrt = 0.0f; float hu = 0.0f;
      
         if (pRes->m_pReach)
            {
            GetReachClimate(CDT_TMEAN, pRes->m_pReach, dayOfYear, tMean);
            GetReachClimate(CDT_PRECIP, pRes->m_pReach, dayOfYear, precip);
            GetReachClimate(CDT_SOLARRAD, pRes->m_pReach, dayOfYear, shrt);
            GetReachClimate(CDT_SPHUMIDITY, pRes->m_pReach, dayOfYear, hu);
            GetReachClimate(CDT_WINDSPEED, pRes->m_pReach, dayOfYear, ws);//m/s?
            }

         data1[1] = tMean; 
         data1[2] = precip;
         data1[3] = shrt;
         data1[4] = hu;
         data1[5] = ws;
         data1[6] = 0.0;

         pRes->m_pResMetData->AppendRow(data1, 7);
         delete[] data1;
         }

     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     // This code is for model comparison with ResSIM only
     // The output files do not extend past 12/31/2009.  If this code is included in the build, the model will fail if it is run beyond that point.

      /*
     if (pRes->m_pResSimFlowOutput != NULL)
     {     
     
       float FlowData[ 7 ];

      FlowData[ 0 ] = time;
      FlowData[ 0 ] = this->m_timeInRun;
      FlowData[ 1 ] = pRes->m_pResSimFlowOutput->IGet(m_currentTime,0,1,IM_LINEAR);   //ResSim pool elevation value (m)
      //FlowData[ 1 ] = pRes->m_pResSimFlowOutput->Get();
      FlowData[ 2 ] = pRes->GetPoolElevationFromVolume();
       FlowData[ 3 ] = pRes->m_pResSimFlowOutput->IGet(m_currentTime,0,2,IM_LINEAR);   //ResSim inflow value (cms)
       FlowData[ 4 ] = pRes->m_inflow/SEC_PER_DAY;
       FlowData[ 5 ] = pRes->m_pResSimFlowOutput->IGet(m_currentTime,0,3,IM_LINEAR);   //ResSim outflow value (cms)
       FlowData[ 6 ] = pRes->m_outflow/SEC_PER_DAY;
      
      pRes->m_pResSimFlowCompare->AppendRow(FlowData, 7);
     }
     */
     if (pRes->m_pResSimRuleOutput != NULL)
     {  
     
      VData vtime = m_timeInRun;
      vtime.ChangeType(TYPE_INT);
      VData *Rules = new VData[ 7 ]; 
      //int rulerow = pRes->m_pResSimRuleOutput->Find(0,vtime,0);
      //VData ressimrule;
      //ressimrule.ChangeType(TYPE_CSTRING);
     // ressimrule = pRes->m_pResSimRuleOutput->Get(1,rulerow);

      VData activerule; 
      activerule.ChangeType(TYPE_CSTRING);
      activerule = pRes->m_activeRule;

      VData blank;
      blank.ChangeType(TYPE_CSTRING);
      blank = "none";

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
      Rules[5]  = zone;
      Rules[6] = activerule;
     
      pRes->m_pResSimRuleCompare->AppendRow(Rules, 7); 
      delete[] Rules;

     }  //end of if (pRes->m_pResSimRuleOutput != NULL)
      }   //end of  for ( int i=0; i < (int) this->m_reservoirArray.GetSize(); i++ )

/*
   int numSamples = (int) m_reachSampleLocationArray.GetSize();
   float *discharge = new float[ numSamples+1 ];
   discharge[0] = m_timeInRun ;
   for (int i=0; i < numSamples; i++)
      {
      ReachSampleLocation *pSam = m_reachSampleLocationArray.GetAt(i);
      ASSERT (pSam != NULL);
      if ( pSam->m_pReach != NULL )
         {
         ReachSubnode *pNode = (ReachSubnode*) pSam->m_pReach->m_subnodeArray[ 0 ];
         discharge[ i+1 ] = pNode->m_discharge;
         }
      else
         discharge[ i+1 ] = 0;
      }

   m_pReachDischargeData->AppendRow( discharge );  
   delete [] discharge;

   CollectDischargeMeasModelData();

   if (!m_estimateParameters)
      {
      CollectReservoirData();
      CollectHRULayerData();
      CollectHRULayerExtraSVData();
      CollectHRULayerDataSingleObject();
      }
   */
   }
   

// called daily (or whatever timestep FLOW is running at)
void FlowModel::UpdateHRULevelVariables(EnvContext *pEnvContext)
   {
   float time     = float( m_currentTime-(int)m_currentTime );
   float currTime = m_flowContext.dayOfYear+time;

   m_pCatchmentLayer->m_readOnly=false;

//   #pragma omp parallel for 
   for ( int i=0; i < (int) this->m_hruArray.GetSize(); i++ )
      {
      HRU *pHRU = m_hruArray[ i ];

      // climate
      GetHRUClimate( CDT_PRECIP,pHRU, (int) currTime,pHRU->m_currentPrecip);
      GetHRUClimate( CDT_TMEAN, pHRU, (int) currTime,pHRU->m_currentAirTemp);
		GetHRUClimate(CDT_TMIN, pHRU, (int)currTime, pHRU->m_currentMinTemp);
      
      pHRU->m_precip_yr      += pHRU->m_currentPrecip;         // mm
      pHRU->m_precip_wateryr += pHRU->m_currentPrecip;         // mm
      pHRU->m_rainfall_yr    += pHRU->m_currentRainfall;       // mm
      pHRU->m_snowfall_yr    += pHRU->m_currentSnowFall;       // mm
      pHRU->m_temp_yr        += pHRU->m_currentAirTemp;


      // hydrology
      pHRU->m_depthMelt = float( pHRU->GetLayer(1)->m_volumeWater/pHRU->m_area );  // volume of water in snow
      pHRU->m_depthSWE  = float( pHRU->GetLayer(0)->m_volumeWater/pHRU->m_area + pHRU->m_depthMelt );   // volume of ice in snow 

      pHRU->m_maxET_yr  += pHRU->m_currentMaxET;
      pHRU->m_et_yr     += pHRU->m_currentET;
      pHRU->m_runoff_yr += pHRU->m_currentRunoff;

      // THIS SHOULDN"T BE HERE!!!!!!
      for (int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++)
         {
         int idu = pHRU->m_polyIndexArray[k];
         // assume snow is in top two layers.  Note that this is the same as pHRU->m_depthSWE
         float snow = (((float)pHRU->GetLayer(0)->m_volumeWater + (float)pHRU->GetLayer(1)->m_volumeWater) / pHRU->m_area * 1000.0f);
         gpFlow->UpdateIDU(pEnvContext, idu, m_colHruSWE, snow , false);  // SetData, not AddDelta   // m_colHruSWE_yr
         }
      }

   m_pCatchmentLayer->m_readOnly = true;
   }


bool FlowModel::GetHRUData( HRU *pHRU, int col, VData &value, DAMETHOD method )
   {
   if ( method == DAM_FIRST )
      {
      if ( pHRU->m_polyIndexArray.IsEmpty() )
         return false;

      return m_pCatchmentLayer->GetData( pHRU->m_polyIndexArray[0], col, value );
      }
   else
      {
      DataAggregator da( m_pCatchmentLayer );
      da.SetPolys( (int*)(pHRU->m_polyIndexArray.GetData()),
                   (int ) pHRU->m_polyIndexArray.GetSize() );
      da.SetAreaCol( m_colCatchmentArea );
      return da.GetData(col, value, method );
      }
   }


bool FlowModel::GetHRUData( HRU *pHRU, int col, bool  &value, DAMETHOD method )
   {
   VData _value;
   if ( GetHRUData( pHRU, col, _value, method ) == false )
      return false;

   return _value.GetAsBool( value );
   }


bool FlowModel::GetHRUData( HRU *pHRU, int col, int  &value, DAMETHOD method )
   {
   VData _value;
   if ( GetHRUData( pHRU, col, _value, method ) == false )
      return false;

   return _value.GetAsInt( value );
   }


bool FlowModel::GetHRUData( HRU *pHRU, int col, float &value, DAMETHOD method )
   {
   VData _value;
   if ( GetHRUData( pHRU, col, _value, method ) == false )
      return false;

   return _value.GetAsFloat( value );
   }



bool FlowModel::GetCatchmentData( Catchment *pCatchment, int col, VData &value, DAMETHOD method )
   {
   if ( method == DAM_FIRST )
      {
      HRU *pHRU = pCatchment->GetHRU( 0 );
      if ( pHRU == NULL || pHRU->m_polyIndexArray.IsEmpty() )
         return false;

      return m_pCatchmentLayer->GetData( pHRU->m_polyIndexArray[0], col, value );
      }
   else
      {
      DataAggregator da( m_pCatchmentLayer );

      int hruCount = pCatchment->GetHRUCount();
      for ( int i=0; i < hruCount; i++ )
         {
         HRU *pHRU = pCatchment->GetHRU( i );

         da.AddPolys( (int*)(pHRU->m_polyIndexArray.GetData()),
                      (int ) pHRU->m_polyIndexArray.GetSize() );
         }

      da.SetAreaCol( m_colCatchmentArea );
      return da.GetData(col, value, method );
      }
   }


bool FlowModel::GetCatchmentData( Catchment *pCatchment, int col, int &value, DAMETHOD method )
   {
   VData _value;
   if ( GetCatchmentData( pCatchment, col, _value, method ) == false )
      return false;

   return _value.GetAsInt( value );
   }


bool FlowModel::GetCatchmentData( Catchment *pCatchment, int col, bool &value, DAMETHOD method )
   {
   VData _value;
   if ( GetCatchmentData( pCatchment, col, _value, method ) == false )
      return false;

   return _value.GetAsBool( value );
   }


bool FlowModel::GetCatchmentData( Catchment *pCatchment, int col, float &value, DAMETHOD method )
   {
   VData _value;
   if ( GetCatchmentData( pCatchment, col, _value, method ) == false )
      return false;

   return _value.GetAsFloat( value );
   }



bool FlowModel::SetCatchmentData( Catchment *pCatchment, int col, VData value )
   {
   if ( col < 0 )
      return false;

   for ( int i=0; i < pCatchment->GetHRUCount(); i++ )
      {
      HRU *pHRU = pCatchment->GetHRU( i );
      
      for ( int j=0; j < (int) pHRU->m_polyIndexArray.GetSize(); j++ )
         m_pCatchmentLayer->SetData( pHRU->m_polyIndexArray[ j ], col, value );
      }
   return true;
   }


bool FlowModel::GetReachData( Reach *pReach, int col, VData &value )
   {
   int record = pReach->m_polyIndex;
   return m_pStreamLayer->GetData( record, col, value );
   }

bool FlowModel::GetReachData( Reach *pReach, int col, bool &value )
   {
   int record = pReach->m_polyIndex;
   return m_pStreamLayer->GetData( record, col, value );
   }

 
bool FlowModel::GetReachData( Reach *pReach, int col, int &value )
   {
   int record = pReach->m_polyIndex;
   return m_pStreamLayer->GetData( record, col, value );
   }

bool FlowModel::GetReachData( Reach *pReach, int col, float &value )
   {
   int record = pReach->m_polyIndex;
   return m_pStreamLayer->GetData( record, col, value );
   }


bool FlowModel::MoveFlux( Flux *pFlux, FluxContainer *pStartContainer, FluxContainer *pEndContainer )
   {
   // first, remove the flux from the start layer
   int index = pStartContainer->RemoveFlux( pFlux, false );
   
   if ( index < 0 )
      return false;

   return pEndContainer->AddFlux( pFlux ) >= 0 ? true : false;
   }




//-- ::ReadAscii() ------------------------------------------
//
//--  Opens and loads the file into the data object
//-------------------------------------------------------------------

int FlowModel::ReadUSGSFlow( LPCSTR _fileName, FDataObj &dataObj, int dataType )
   {
   CString fileName;
   PathManager::FindPath( _fileName, fileName );

   HANDLE hFile = CreateFile(fileName, 
      GENERIC_READ, // open for reading
      FILE_SHARE_READ, // do not share (deprecated - jb)
      NULL, // no security
      OPEN_EXISTING, // existing file only
      FILE_ATTRIBUTE_NORMAL, // normal file
      NULL); // no attr. template

   //FILE *fp;
   //fopen_s( &fp, fileName, "rt" );     // open for "read"

   if ( hFile == INVALID_HANDLE_VALUE )
      {
      CString msg( "Flow: ReadUSGS: Couldn't find file '" );
      msg += fileName;
      msg += "'. ";

      DWORD dw = GetLastError(); 
      LPVOID lpMsgBuf = NULL;

      FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
      
      msg += (LPTSTR) lpMsgBuf;
      LocalFree(lpMsgBuf);
      Report::WarningMsg( msg );
      return -1;
      }

   dataObj.Clear();   // reset data object, strings

   LARGE_INTEGER _fileSize;
   BOOL ok = GetFileSizeEx( hFile, &_fileSize );
   ASSERT( ok );

   DWORD fileSize = (DWORD) _fileSize.LowPart;

   //long fileSize = _filelength ( _fileno( fp ) );
   
   TCHAR *buffer  = new TCHAR[ fileSize+2 ];
   memset( buffer, 0, (fileSize+2) * sizeof( TCHAR ) );

   //fread ( buffer, fileSize, 1, fp );
   //fclose( fp );
   DWORD bytesRead = 0;
   ReadFile( hFile, buffer, fileSize, &bytesRead, NULL );


   //-- skip any leading comments --//
   char *p = buffer;
   while ( *p == '#' || *p == 'a' || *p == '5'  )
      {
      p = strchr( p, '\n' );
      p++;
      }

   //-- start parsing buffer --//
   char *end = strchr( p, '\n' );

   //-- NULL-terminate then get column count --//
   *end    = NULL;
   int  _cols = 7;

   while ( *p != NULL )
      {
      if ( *p++ == '   ' )    // count the number of delimiters
         _cols++;
      }

   dataObj.SetSize( 2, 0 );  // resize matrix, statArray

   //-- set labels from first line (labels are delimited) --//
   char d[ 4 ];
   d[ 0 ] = '\t';
   d[ 1 ] = '-';
   d[ 2 ] = '\n';
   d[ 3 ] = NULL;
   char *next;

   p = strtok_s( buffer, d, &next );
   _cols = 0;

   while ( p != NULL )
      {
      while ( *p == ' ' ) p++;   // strip leading blanks

     // SetLabel( _cols++, p );
      p = strtok_s( NULL, d, &next );   // look for delimiter or newline
      }

   //-- ready to start parsing data --//
   int cols = 7;
   int c = 2;
   float *data = new float[ c ];
   memset( data, 0, c *sizeof( float ) );

   p = strtok_s( end+1, d, &next );  // look for delimiter

   while ( p != NULL )
      {
      //-- get a row of data --//
      int year, month, day=-1;

      for ( int i=0; i < cols; i++ )
         {
         if (*p=='U')//at the start of a line
            i=0;
         else if (i==2)//year
            year =  atoi( p ) ;
         else if (i==3)//month
            month= atoi( p ) ;
         else if (i==4)//day  
            day =  atoi( p ) ;
         else if (i==5)//q converted to m3/s
            {
            data[1]= ( (float) atof( p ))/35.35f ;

            if (dataType == 1) //reservoir level, ft
               data[1] = ((float)atof(p)) / 3.28f;//convert to meters
            else if (dataType == 2)
               data[1] = (float)(atof(p));

            //COleDateTime time;
            //time.SetDate(year,month,day);
            int dayOfYear = ::GetJulianDay (month, day, year );   // 1-based
            float time = (year*365.0f)+dayOfYear-1;   // days since 0000

            data[0] = time ;
            } 

         //data[ i ] =  (float) atof( p ) ;
         p = strtok_s( NULL, d, &next );  // look for delimeter

         //-- invalid line? --//
         if ( p == NULL && i < cols-1 )
            break;
         }

      dataObj.AppendRow( data, c );
      }

   //-- clean up --//
   delete [] data;
   delete [] buffer;

   return dataObj.GetRowCount();
   }
         
float FlowModel::GetObjectiveFunction(FDataObj *pData, float &ns, float &nsLN, float &ve)
   {
   int meas=0;      
   int offset=0;
   int n = pData->GetRowCount();
   float obsMean=0.0f, predMean=0.0f,obsMeanL=0.0f,predMeanL=0.0f;
   int firstSample = 200;
   for (int j=firstSample;j<n;j++) //50this skips the first 50 days. Assuming those values
      // might be unacceptable due to initial conditions...
      {
      float time = 0.0f; float obs=0.0f; float pred=0.0f;
      pData->Get(0, j,time);
      pData->Get(2, j,obs); //add up all the discharge values and then...
      obsMean+=obs;
      obsMeanL+=log10(obs);
      pData->Get(1,j,pred);
      predMean+=pred;
      predMeanL+=log10(pred);
      }
   obsMean = obsMean/((float)n-(float)firstSample); // divide them by the sample size to generate the average.
   //predMean = predMean/((float)n-(float)firstSample); // divide them by the sample size to generate the average.
   obsMeanL=obsMeanL/((float)n-(float)firstSample);
   //predMeanL=predMeanL/((float)n-(float)firstSample);

   double errorSum = 0.0f, errorSumDenom = 0.0f, errorSumLN = 0.0f, errorSumDenomLN = 0.0f, r2Denom1 = 0.0f, r2Denom2 = 0.0f; float ve_numerator = 0.0f; float ve_denominator = 0.0f;
   for (int row=firstSample; row < n; row++ ) 
      {
      float pred =0.0f, obs=0.0f;float time=0.0f;
      pData->Get(0, row,time);
      pData->Get(2, row,obs);
      pData->Get(1,row,pred); // Assuming col i holds data for the reach corresponding to measurement

      errorSum+=((obs-pred)*(obs-pred));
      errorSumDenom+=((obs-obsMean)*(obs-obsMean));

      ve_numerator += (obs - pred);
      ve_denominator += obs;
      
      double a=log10(obs)-log10(pred);
      double b=log10(obs)-obsMeanL;
     // double b = log10(obs) - log10(obsMean);
      errorSumLN+=pow(a,2);
      errorSumDenomLN+=pow(b,2);
      }

   if (errorSumDenom > 0.0f)
      {
      ns  = (float)(1.0f - (errorSum/errorSumDenom));
      nsLN  = (float)(1.0f - (errorSumLN/errorSumDenomLN));
      ve = (float)ve_numerator / ve_denominator;
      }
   return ns;
   }
  
void FlowModel::GetNashSutcliffe(float *ns)
   {
   int meas=0;      
   float _ns=1.0f; float _nsLN;
   float *data = new float[ m_reachMeasuredDataArray.GetSize() ];
   int offset=0;
   for ( int l=0; l < (int) m_reachMeasuredDataArray.GetSize(); l++)
      {
      FDataObj *pData = m_reachMeasuredDataArray.GetAt(l);
      int n = pData->GetRowCount();
      float obsMean=0.0f, predMean=0.0f;
      int firstSample = 200;
      for (int j=firstSample;j<n;j++) //50this skips the first 50 days. Assuming those values
         // might be unacceptable due to initial conditions...
         {
         float time = pData->Get(0, j);
         obsMean += pData->Get(2, j); //add up all the discharge values and then...
         predMean += pData->Get(1,j);
         }
      obsMean = obsMean/((float)n-(float)firstSample); // divide them by the sample size to generate the average.
      predMean = predMean/((float)n-(float)firstSample); // divide them by the sample size to generate the average.

      double errorSum=0.0f, errorSumDenom=0.0f, errorSumLN=0.0f, errorSumDenomLN=0.0f, r2Denom1=0.0f, r2Denom2=0.0f;
      for (int row=firstSample; row < n; row++ ) 
         {
         float pred =0.0f, obs=0.0f;
         float time = pData->Get(0, row);
         obs = pData->Get(2, row);
         pData->Get(1,row,pred); // Assuming col i holds data for the reach corresponding to measurement

         errorSum+=((obs-pred)*(obs-pred));
         errorSumDenom+=((obs-obsMean)*(obs-obsMean));
         double a=log10(obs)-log10(pred);
         double b=log10(obs)-log10(obsMean);
         errorSumLN+=pow(a,2);
         errorSumDenomLN+=(b,2);
         }

      if (errorSumDenom > 0.0f)
         {
         _ns  = (float)(1.0f - (errorSum/errorSumDenom));
         _nsLN  = (float)(1.0f - (errorSumLN/errorSumDenomLN));
         }
      ns[offset] = _ns;
      offset++;
      ns[offset] = _nsLN; 
      offset++;
      }
   delete [] data;
   }

int FlowModel::ReadClimateStationData( LPCSTR filename )
   {
   if ( m_pClimateStationData != NULL )  // is there already a climate data obj?
      delete m_pClimateStationData;

   m_pClimateStationData = new FDataObj;   // create a new one
   ASSERT( m_pClimateStationData != NULL );

   int records = (int) m_pClimateStationData->ReadAscii( filename, ',', TRUE );
   for (int i=0;i<m_pClimateStationData->GetRowCount();i++)
      {
      int dayOfYear = ::GetJulianDay ( (int) m_pClimateStationData->Get(1,i), (int) m_pClimateStationData->Get(2,i), (int) m_pClimateStationData->Get(3,i) );   // 1-based
      float time = (m_pClimateStationData->Get(3,i)*365.0f)+dayOfYear-1;   // days since 0000
      LPCTSTR label = m_pClimateStationData->GetLabel(4);
      if (label=="hour")
         {
         float hour=m_pClimateStationData->Get(4,i)/24.0f;
         float min=m_pClimateStationData->Get(5,i)/60.0f/100.0f;
         time+=(hour+min);
         }
      m_pClimateStationData->Set(0,i,time);

      }
   return records;
   }


void FlowModel::ResetStateVariables()
   {
   m_timeOffset=0;


   ReadState();//read file including initial values for the state variables. This avoids model spin up issues
   int catchmentCount = GetCatchmentCount();
   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
	for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();
      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         int hruLayerCount=pHRU->GetLayerCount();
         float waterDepth=0.0f;
         for ( int l=0; l < hruLayerCount; l++ )     
            {
            HRULayer *pHRULayer = pHRU->GetLayer( l );
            waterDepth+=float( pHRULayer->m_volumeWater/pHRU->m_area*1000.0f );//mm of total storage
            }
         pHRU->m_initStorage=waterDepth;
         }
      }
   }

void FlowModel::GetTotalStorage(float &channel, float &terrestrial)
   {

   int catchmentCount = GetCatchmentCount();
   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = GetCatchment(i);
      int hruCount = pCatchment->GetHRUCount();
      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         int hruLayerCount=pHRU->GetLayerCount();
         for ( int l=0; l < hruLayerCount; l++ )     
            {
            HRULayer *pHRULayer = pHRU->GetLayer( l );
            terrestrial+= float( pHRULayer->m_volumeWater );
            }
         }
      }

   int reachCount = (int) m_reachArray.GetSize();
   for ( int i=0; i < reachCount; i++ )
      {
      Reach *pReach = m_reachArray[ i ];
      SetGeometry(pReach,pReach->GetReachSubnode( 0 )->m_discharge);
      channel+=(pReach->m_width*pReach->m_depth*pReach->m_length);
      }
   }


bool FlowModel::InitPlugins( void )
   {
   //if (  m_pReachRouting->m_type = m_reachExtFnDLL )
   //   {
   //   FLOWINITFN initFn = (FLOWINITFN) ::GetProcAddress( m_reachExtFnDLL, "Init" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext, NULL );
   //   }

   //if (  m_latExchExtFnDLL )
   //   {
   //   FLOWINITFN initFn = (FLOWINITFN) ::GetProcAddress( m_latExchExtFnDLL, "Init" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext, NULL );
   //   }
   //
   //if (  m_hruVertFluxExtFnDLL )
   //   {
   //   FLOWINITFN initFn = (FLOWINITFN) ::GetProcAddress( m_hruVertFluxExtFnDLL, "Init" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext, NULL );
   //   }

   if (  m_extraSvRxnExtFnDLL )
      {
      FLOWINITFN initFn = (FLOWINITFN) ::GetProcAddress( m_extraSvRxnExtFnDLL, "Init" );

      if ( initFn != NULL )
         initFn( &m_flowContext, NULL );
      }

   if (  m_reservoirFluxExtFnDLL )
      {
      FLOWINITFN initFn = (FLOWINITFN) ::GetProcAddress( m_reservoirFluxExtFnDLL, "Init" );

      if ( initFn != NULL )
         initFn( &m_flowContext, NULL );
      }

   //if (  m_gwExtFnDLL )
   //   {
   //   FLOWINITFN initFn = (FLOWINITFN) ::GetProcAddress( m_gwExtFnDLL, "Init" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext, NULL );
   //   }

   //for ( INT_PTR i=0; i < this->m_extMethodArray.GetSize(); i++ )
   //   {
   //   ExternalMethod *pMethod = m_extMethodArray[ i ];
   //   pMethod->Init( &m_flowContext );
   //   }

   return true;
   }


bool FlowModel::InitRunPlugins( void )
   {
   //if (  m_reachExtFnDLL )  // TODO - this isn't currently supported!!!
   //   {
   //   FLOWINITRUNFN initFn = (FLOWINITRUNFN) ::GetProcAddress( m_reachExtFnDLL, "InitRun" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext );
   //   }

   //if (  m_latExchExtFnDLL )
   //   {
   //   FLOWINITRUNFN initFn = (FLOWINITRUNFN) ::GetProcAddress( m_latExchExtFnDLL, "InitRun" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext );
   //   }
   //
   //if (  m_hruVertFluxExtFnDLL )
   //   {
   //   FLOWINITRUNFN initFn = (FLOWINITRUNFN) ::GetProcAddress( m_hruVertFluxExtFnDLL, "InitRun" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext );
   //   }

   if (  m_extraSvRxnExtFnDLL )
      {
      FLOWINITRUNFN initFn = (FLOWINITRUNFN) ::GetProcAddress( m_extraSvRxnExtFnDLL, "InitRun" );

      if ( initFn != NULL )
         initFn( &m_flowContext );
      }

   if (  m_reservoirFluxExtFnDLL )
      {
      FLOWINITRUNFN initFn = (FLOWINITRUNFN) ::GetProcAddress( m_reservoirFluxExtFnDLL, "InitRun" );

      if ( initFn != NULL )
         initFn( &m_flowContext );
      }

   //if (  m_gwExtFnDLL )
   //   {
   //   FLOWINITRUNFN initFn = (FLOWINITRUNFN) ::GetProcAddress( m_gwExtFnDLL, "InitRun" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext );
   //   }   


   return true;
   }


bool FlowModel::EndRunPlugins( void )
   {
   //if (  m_reachExtFnDLL )
   //   {
   //   FLOWENDRUNFN initFn = (FLOWENDRUNFN) ::GetProcAddress( m_reachExtFnDLL, "EndRun" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext );
   //   }

   //if (  m_latExchExtFnDLL )
   //   {
   //   FLOWENDRUNFN initFn = (FLOWENDRUNFN) ::GetProcAddress( m_latExchExtFnDLL, "EndRun" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext );
   //   }
   //
   //if (  m_hruVertFluxExtFnDLL )
   //   {
   //   FLOWENDRUNFN initFn = (FLOWENDRUNFN) ::GetProcAddress( m_hruVertFluxExtFnDLL, "EndRun" );
   //
   //   if ( initFn != NULL )
   //      initFn( &m_flowContext );
   //   }

   if (  m_extraSvRxnExtFnDLL )
      {
      FLOWENDRUNFN initFn = (FLOWENDRUNFN) ::GetProcAddress( m_extraSvRxnExtFnDLL, "EndRun" );

      if ( initFn != NULL )
         initFn( &m_flowContext );
      }

   if (  m_reservoirFluxExtFnDLL )
      {
      FLOWENDRUNFN initFn = (FLOWENDRUNFN) ::GetProcAddress( m_reservoirFluxExtFnDLL, "EndRun" );

      if ( initFn != NULL )
         initFn( &m_flowContext );
      }

//   if (  m_gwExtFnDLL )
//      {
//      FLOWENDRUNFN initFn = (FLOWENDRUNFN) ::GetProcAddress( m_gwExtFnDLL, "EndRun" );
//
//      if ( initFn != NULL )
//         initFn( &m_flowContext );
//      }   

//   for ( INT_PTR i=0; i < this->m_extMethodArray.GetSize(); i++ )
//      {
//      ExternalMethod *pMethod = m_extMethodArray[ i ];
//      pMethod->EndRun( &m_flowContext );
//      }
//
   return true;
   }


int FlowModel::GetUpstreamCatchmentCount( ReachNode *pStartNode )
   {
   if ( pStartNode == NULL )
      return 0;

   int count = 0;
   if ( pStartNode->m_pLeft )
      {
      count += GetUpstreamCatchmentCount( pStartNode->m_pLeft );

      Reach *pReach = GetReachFromNode( pStartNode->m_pLeft );

      if ( pReach != NULL )
         count += (int) pReach->m_catchmentArray.GetSize();
      }

   if ( pStartNode->m_pRight)
      {
      count += GetUpstreamCatchmentCount( pStartNode->m_pRight );

      Reach *pReach = GetReachFromNode( pStartNode->m_pRight );

      if ( pReach != NULL )
         count += (int) pReach->m_catchmentArray.GetSize();
      }
      
   return count;
   }


/////////////////////////////////////////////////////////////////////////////////
// Climate methods
/////////////////////////////////////////////////////////////////////////////////


//void FlowModel::InitRunClimate( void )
//   {
//   for ( int i=0; i < (int) m_climateDataInfoArray.GetSize(); i++ )
//      {
//      ClimateDataInfo *pInfo = m_climateDataInfoArray[ i ];
//
//    //  if ( pInfo->m_pDataObj == NULL )
//         pInfo->m_pDataObj->InitLibraries();
//      }
//   }


// called at the beginning of each invocation of FlowModel::Run() (beginning of each year)
bool FlowModel::LoadClimateData(EnvContext *pEnvContext, int year )
   {
   FlowScenario *pScenario = m_scenarioArray.GetAt(m_currentScenarioIndex);
   for ( int i=0; i < (int) pScenario->m_climateInfoArray.GetSize(); i++ )
      {
      ClimateDataInfo *pInfo = pScenario->m_climateInfoArray[ i ];

      if ( pInfo->m_pDataObj == NULL )
         continue;

      if ( pInfo->m_useDelta )
         {
         //if ( GetCurrentYearOfRun() == 0 )  // first year?
          //  {
            CString path;
            path.Format( "%s_%i.nc", (LPCTSTR) pInfo->m_path, pEnvContext->startYear);
            CString msg;
            msg.Format( "Loading Climate %s", (LPCTSTR) pInfo->m_path );
            Report::LogMsg( msg, RT_INFO );

            pInfo->m_pDataObj->Open( path, pInfo->m_varName );
            pInfo->m_pDataObj->ReadSpatialData();
         //   }
         }

      
      else if (m_estimateParameters && pEnvContext->run == 0 )//estimating parameters and on the first run.  Load all climate data for this year
        {
        CString path;
        path.Format("%s_%i.nc", (LPCTSTR)pInfo->m_path, year);
        CString msg;
        msg.Format("Loading Climate %s", (LPCTSTR)pInfo->m_path);
        Report::LogMsg(msg, RT_INFO);
        GeoSpatialDataObj *pObj = pInfo->m_pDataObjArray.GetAt(GetCurrentYearOfRun());
        pObj->Open(path, pInfo->m_varName);
        pObj->ReadSpatialData();
        }

      else if (!m_estimateParameters)  // not using delta method and not estimating parameters, so open/read this years data
         {
         CString path;
         path.Format( "%s_%i.nc", (LPCTSTR) pInfo->m_path, year );

         CString msg;
         msg.Format( "Loading Climate %s", (LPCTSTR) pInfo->m_path );
         Report::LogMsg( msg, RT_INFO );

         pInfo->m_pDataObj->Open( path, pInfo->m_varName );
         pInfo->m_pDataObj->ReadSpatialData();
         }
      }

   return true;
   }


// called at the end of FlowModel::Run()
void FlowModel::CloseClimateData( void )
   {
   FlowScenario *pScenario = m_scenarioArray[ m_currentScenarioIndex ];

   for ( int i=0; i < (int) pScenario->m_climateInfoArray.GetSize(); i++ )
      {
      ClimateDataInfo *pInfo = pScenario->m_climateInfoArray[ i ];

      if ( pInfo->m_pDataObj != NULL )
         pInfo->m_pDataObj->Close();
      }
   }


bool FlowModel::GetHRUClimate( CDTYPE type, HRU *pHRU, int dayOfYear, float &value )
   {
   if (!m_useStationData) //it is from a NETCDF format
      {
      ClimateDataInfo *pInfo = GetClimateDataInfo(type);
      if (!m_estimateParameters)
         {
         if (pInfo != NULL && pInfo->m_pDataObj != NULL)   // find a data object?
            {
            if (pHRU->m_climateIndex < 0)
               {
               double x = pHRU->m_centroid.x;
               double y = pHRU->m_centroid.y;
               value = pInfo->m_pDataObj->Get(x, y, pHRU->m_climateIndex, dayOfYear, m_projectionWKT);   // units?????!!!!
               }
            else
               {
               value = pInfo->m_pDataObj->Get(pHRU->m_climateIndex, dayOfYear);
               }

            if (pInfo->m_useDelta)
               value += (pInfo->m_delta * this->GetCurrentYearOfRun());

            return true;
            }
         }

      else //m_estimateParameters is true
         {
         GeoSpatialDataObj *pObj = pInfo->m_pDataObjArray.GetAt(GetCurrentYearOfRun());//current year
         if (pInfo != NULL && pObj != NULL)   // find a data object?
            {
            if (pHRU->m_climateIndex < 0)
               {
               double x = pHRU->m_centroid.x;
               double y = pHRU->m_centroid.y;
               value = pObj->Get(x, y, pHRU->m_climateIndex, dayOfYear, m_projectionWKT);   // units?????!!!!
               }
            else
               {
               value = pObj->Get(pHRU->m_climateIndex, dayOfYear);
               }
            }
            return true;
         }
      }
   else if (m_pClimateStationData != NULL)
      {
      if (type==CDT_PRECIP)
         {
         value=m_pClimateStationData->IGetInternal((float) m_currentTime,6,IM_LINEAR,m_timeOffset, true);
         int stope = 1;
         }
      else if (type==CDT_TMEAN)
         {
         float temp = m_pClimateStationData->IGetInternal((float) m_currentTime,7,IM_LINEAR,m_timeOffset, false);
         if (pHRU->m_elevation==0)
            pHRU->m_elevation=m_climateStationElev;
         float elevKM=pHRU->m_elevation/1000.f;
         value = temp+(-4.0f*(elevKM-m_climateStationElev/1000.0f));       
         }
      if (type == CDT_SOLARRAD)
         {
         value = m_pClimateStationData->IGetInternal((float)m_currentTime, 10, IM_LINEAR, m_timeOffset, false);
         }
      if (type == CDT_RELHUMIDITY)
         {
         value = m_pClimateStationData->IGetInternal((float)m_currentTime, 12, IM_LINEAR, m_timeOffset, false);
         }


      if (type == CDT_TMIN)
         {
         value = m_pClimateStationData->IGetInternal((float)m_currentTime, 9, IM_LINEAR, m_timeOffset, false);
         }
      if (type == CDT_TMAX)
         {
         value = m_pClimateStationData->IGetInternal((float)m_currentTime, 8, IM_LINEAR, m_timeOffset, false);
         }
      if (type == CDT_WINDSPEED)
         {
         value = m_pClimateStationData->IGetInternal((float)m_currentTime, 11, IM_LINEAR, m_timeOffset, false);
         }
      if (type == CDT_SPHUMIDITY)
         {
         value = m_pClimateStationData->IGetInternal((float)m_currentTime, 13, IM_LINEAR, m_timeOffset, false);
         }
      else
         return false;
      return true;
      }

   return false;
   }


bool FlowModel::GetReachClimate( CDTYPE type, Reach *pReach, int dayOfYear, float &value )
   {
   ClimateDataInfo *pInfo = GetClimateDataInfo( type );

   if ( pInfo != NULL && pInfo->m_pDataObj != NULL )   // find a data object?
      {
      if ( pReach->m_climateIndex < 0 )
         {
         Poly *pPoly = m_pStreamLayer->GetPolygon( pReach->m_polyIndex );

         if ( pPoly->GetVertexCount() > 0 )
            {
            double x = pPoly->GetVertex( 0 ).x;  //pPoly->GetCentroid().x;
            double y = pPoly->GetVertex( 0 ).y;   //pPoly->GetCentroid().y;
            value = pInfo->m_pDataObj->Get( x, y, pReach->m_climateIndex, dayOfYear, m_projectionWKT  );   // units?????!!!!
            }
         else
            {
            CString msg;
            msg.Format( "Flow: Bad stream poly (id=%i) - no vertices!!!", pPoly->m_id );
            Report::ErrorMsg( msg );
            }
         }
      else
         {
         value = pInfo->m_pDataObj->Get( pReach->m_climateIndex, dayOfYear );
         }

      if ( pInfo->m_useDelta )
         value += ( pInfo->m_delta * this->GetCurrentYearOfRun() );

      return true;
      }

   return false;
   }

float FlowModel::GetTotalReachLength( void )
   {
   float length = 0;

   for ( int i=0; i < this->GetReachCount(); i++ )
      {
      Reach *pReach = m_reachArray[ i ];

      length += pReach->m_length;
      }

   return length;
   }



// called daily (or whatever timestep flow is running at
bool FlowModel::CollectModelOutput(void )
   {
   clock_t start = clock();

   // any outputs to collect?
   int moCount = 0;
   int moCountIdu = 0;
   int moCountHru = 0;
   int moCountHruLayer = 0;
   int moCountReach = 0;
   int moCountHruIdu =0;
   int moCountReservoir = 0;

   for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

      for ( int i=0; i < (int) pGroup->GetSize(); i++ )
         {
         ModelOutput *pOutput = pGroup->GetAt( i );

         // initialize ModelOutput
         pOutput->m_value = 0;
         pOutput->m_totalQueryArea = 0;
   
         if ( pOutput->m_inUse )
            {
            moCount++;

            switch( pOutput->m_modelDomain )
               {
               case MOD_IDU:        moCountIdu++;          break;
               case MOD_HRU:        moCountHru++;          break;
               case MOD_HRULAYER:   moCountHruLayer++;     break;
               case MOD_REACH:      moCountReach++;        break;
               case MOD_HRU_IDU:    moCountHruIdu++;       break;
               case MOD_RESERVOIR:   moCountReservoir++;    break;
               }
            }
         }
      }

   if ( moCount == 0 )
      return true;

   float totalIDUArea = 0;
   
   // iterate through IDU's to collect output if any of the expressions are IDU-domain
   if ( moCountIdu > 0 )
      {
      int colArea = m_pCatchmentLayer->GetFieldCol("AREA");

      for ( MapLayer::Iterator idu=this->m_pCatchmentLayer->Begin(); idu < m_pCatchmentLayer->End(); idu++ )
         {
         if ( m_pModelOutputQE )
            m_pModelOutputQE->SetCurrentRecord( idu );

         m_pMapExprEngine->SetCurrentRecord( idu );

         float area = 0;
         m_pCatchmentLayer->GetData( idu, colArea, area );
         totalIDUArea += area;

         for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
            {
            ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

            for( int i=0; i < (int) pGroup->GetSize(); i++ )
               {
               ModelOutput *pOutput = pGroup->GetAt( i );
         
               if ( pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_IDU )
                  continue;
         
               bool passConstraints = true;
         
               if ( pOutput->m_pQuery )
                  {
                  bool result = false;
                  bool ok = pOutput->m_pQuery->Run( idu, result );
               
                  if ( result == false )
                     passConstraints = false;
                  }

               // did we pass the constraints?  If so, evaluate preferences
               if ( passConstraints )
                  {
                  pOutput->m_totalQueryArea += area;
                  float value = 0;

                  if ( pOutput->m_pMapExpr != NULL )
                     {
                     bool ok = m_pMapExprEngine->EvaluateExpr( pOutput->m_pMapExpr, pOutput->m_pQuery ? true : false );
                     if ( ok )
                        value = (float) pOutput->m_pMapExpr->GetValue();
                     }

                  // we have a value for this IDU, accumulate it based on the model type
                  switch( pOutput->m_modelType )
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


   // Next, do HRU-level variables
   totalIDUArea = 0;
   if ( moCountHru > 0 )
      {
      for ( int h=0; h < (int) this->m_hruArray.GetSize(); h++ )
         {
         HRU *pHRU = m_hruArray[ h ];

         int idu = pHRU->m_polyIndexArray[ 0 ];       // use the first IDU to evaluate

         if ( m_pModelOutputQE )
            m_pModelOutputQE->SetCurrentRecord( idu );

         m_pMapExprEngine->SetCurrentRecord( idu );

         float area = pHRU->m_area;
         totalIDUArea += area;

         // update static HRU variables
         HRU::m_mvDepthMelt        = pHRU->m_depthMelt*1000.0f;  // volume of water in snow (converted from m to mm)
         HRU::m_mvDepthSWE         = pHRU->m_depthSWE*1000.0f;   // volume of ice in snow (converted from m to mm)
         HRU::m_mvCurrentPrecip    = pHRU->m_currentPrecip;
         HRU::m_mvCurrentRainfall  = pHRU->m_currentRainfall;
         HRU::m_mvCurrentSnowFall  = pHRU->m_currentSnowFall;
         HRU::m_mvCurrentAirTemp   = pHRU->m_currentAirTemp;
			HRU::m_mvCurrentMinTemp	  = pHRU->m_currentMinTemp;
         HRU::m_mvCurrentET        = pHRU->m_currentET;
         HRU::m_mvCurrentMaxET     = pHRU->m_currentMaxET;
			HRU::m_mvCurrentCGDD		  = pHRU->m_currentCGDD;
         HRU::m_mvCurrentSediment  = pHRU->m_currentSediment;
         HRU::m_mvCumP             = pHRU->m_precip_yr; 
         HRU::m_mvCumRunoff        = pHRU->m_runoff_yr;
         HRU::m_mvCumET            = pHRU->m_et_yr;
         HRU::m_mvCumMaxET         = pHRU->m_maxET_yr;

         for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
            {
            ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

            for( int i=0; i < (int) pGroup->GetSize(); i++ )
               {
               ModelOutput *pOutput = pGroup->GetAt( i );
         
               if ( pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_HRU )
                  continue;
         
               bool passConstraints = true;
         
               if ( pOutput->m_pQuery )
                  {
                  bool result = false;
                  bool ok = pOutput->m_pQuery->Run( idu, result );
               
                  if ( result == false )
                     passConstraints = false;
                  }

               // did we pass the constraints?  If so, evaluate preferences
               if ( passConstraints )
                  {
                  pOutput->m_totalQueryArea += area;
                  float value = 0;

                  if ( pOutput->m_pMapExpr != NULL )
                     {
                     bool ok = m_pMapExprEngine->EvaluateExpr( pOutput->m_pMapExpr, pOutput->m_pQuery ? true : false );
                     if ( ok )
                        value = (float) pOutput->m_pMapExpr->GetValue();
                     }

                  // we have a value for this IDU, accumulate it based on the model type
                  switch( pOutput->m_modelType )
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


   // Next, do HRULayer-level variables
   totalIDUArea = 1;
   if ( moCountHruLayer > 0 )
   {
      for ( int h = 0; h < (int) this->m_hruArray.GetSize(); h++ )
      {
         HRU *pHRU = m_hruArray[h];

         for ( int k = 0; k < pHRU->m_polyIndexArray.GetSize(); k++ )
         {
            float area = 0.0f;
            int idu = pHRU->m_polyIndexArray[ k ];

            if ( m_pModelOutputQE )
               m_pModelOutputQE->SetCurrentRecord( idu );

            m_pMapExprEngine->SetCurrentRecord( idu );

            for ( int j = 0; j < (int)m_modelOutputGroupArray.GetSize(); j++ )
            {
               ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ] ;

               for ( int i = 0; i < (int)pGroup->GetSize(); i++ )
               {
                  ModelOutput *pOutput = pGroup->GetAt( i );

                  if ( pOutput->m_inUse == true && pOutput->m_modelDomain == MOD_HRULAYER )
                  {
                     HRULayer *pHRULayer = pHRU->GetLayer( pOutput->m_number );
                     // update static HRULayer variables
                     HRULayer::m_mvWC = pHRULayer->m_wc;  // volume of water 
                     if ( pHRULayer->m_svArray != NULL )
                     {
                        if ( pHRULayer->m_volumeWater > 1.0f )
                           HRULayer::m_mvESV = pHRULayer->m_svArray[ pOutput->m_esvNumber ] / pHRULayer->m_volumeWater;  // volume of water 
                     }

                     bool passConstraints = true;

                     if ( pOutput->m_pQuery )
                     {
                        bool result = false;
                        bool ok = pOutput->m_pQuery->Run( idu, result );

                        if ( result == false )
                           passConstraints = false;
                     }

                     // did we pass the constraints?  If so, evaluate preferences
                     area = 0.0f;

                     if ( passConstraints )
                     {
                        m_pCatchmentLayer->GetData( idu, m_colCatchmentArea, area );
                        pOutput->m_totalQueryArea += area;
                        totalIDUArea += area;
                        float value = 0;

                        if ( pOutput->m_pMapExpr != NULL )
                        {
                           bool ok = m_pMapExprEngine->EvaluateExpr( pOutput->m_pMapExpr, pOutput->m_pQuery ? true : false );
                           if ( ok )
                              value = (float)pOutput->m_pMapExpr->GetValue();
                        }

                        // we have a value for this IDU, accumulate it based on the model type
                        switch ( pOutput->m_modelType )
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
            } // end if ( use model and HRULayer )
         } // end of: for ( each model )
      } // end of: for ( each output group )
   } // end of: if ( moCountHru > 0 )


   // Next, do Reach-level variables
   totalIDUArea = 1;
   if ( moCountReach > 0 )
      {
      for ( int h=0; h < (int) this->m_reachArray.GetSize(); h++ )
         {
         Reach *pReach = m_reachArray[ h ];
         ReachSubnode *pNode = pReach->GetReachSubnode(0);

         if ( m_pModelOutputStreamQE )
            m_pModelOutputStreamQE->SetCurrentRecord( pReach->m_reachIndex );

         m_pMapExprEngine->SetCurrentRecord( pReach->m_reachIndex );

         // update static HRU variables

         Reach::m_mvCurrentStreamFlow = pReach->GetDischarge();
         Reach::m_mvCurrentStreamTemp = pReach->m_currentStreamTemp;

         for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
            {
            ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

            for( int i=0; i < (int) pGroup->GetSize(); i++ )
               {
               ModelOutput *pOutput = pGroup->GetAt( i );
         
               if ( pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_REACH )
                  continue;
               if (pNode->m_svArray != NULL)
                  Reach::m_mvCurrentTracer  = pNode->m_svArray[pOutput->m_esvNumber]/pNode->m_volume;  // volume of water        
               bool passConstraints = true;
         
               if ( pOutput->m_pQuery )
                  {
                  bool result = false;
                  bool ok = pOutput->m_pQuery->Run( pReach->m_reachIndex, result );
               
                  if ( result == false )
                     passConstraints = false;
                  }

               // did we pass the constraints (we found the correct reach, or reaches)?  If so, evaluate preferences 
               if ( passConstraints )
                  {
                  float value = 0;

                  if ( pOutput->m_pMapExpr != NULL )
                     {
                     bool ok = m_pMapExprEngine->EvaluateExpr( pOutput->m_pMapExpr, false ? true : false );
                     if ( ok )
                        value = (float) pOutput->m_pMapExpr->GetValue();
                     }

                  // we have a value for this Reach, accumulate it based on the model type
                  switch( pOutput->m_modelType )
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
   if ( moCountReservoir > 0 )
      {
      for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
         {
         ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

         for( int i=0; i < (int) pGroup->GetSize(); i++ )
            {
            ModelOutput *pOutput = pGroup->GetAt( i );
         
            if ( pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_RESERVOIR )
               continue;
            else
               {
               for ( int h=0; h < (int) this->m_reservoirArray.GetSize(); h++ )
                  {
                  Reservoir *pRes = m_reservoirArray[ h ];
                  if (pRes->m_id==pOutput->m_siteNumber)         
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
   if ( moCountHruIdu > 0 )
      {
      for ( int h=0; h < (int) this->m_hruArray.GetSize(); h++ )
         {
         HRU *pHRU = m_hruArray[ h ];
         
         for (int z=0;z<pHRU->m_polyIndexArray.GetSize();z++)
            {
            int idu = pHRU->m_polyIndexArray[ z ];       // Here evaluate all the polygons - 

            if ( m_pModelOutputQE )
               m_pModelOutputQE->SetCurrentRecord( idu );

            m_pMapExprEngine->SetCurrentRecord( idu );

            float area = pHRU->m_area;
            totalIDUArea += area;

            // update static HRU variables
            HRU::m_mvDepthMelt       = pHRU->m_depthMelt*1000.0f;  // volume of water in snow (converted from m to mm)
            HRU::m_mvDepthSWE        = pHRU->m_depthSWE*1000.0f;   // volume of ice in snow (converted from m to mm)
            HRU::m_mvCurrentPrecip    = pHRU->m_currentPrecip;
            HRU::m_mvCurrentRainfall  = pHRU->m_currentRainfall;
            HRU::m_mvCurrentSnowFall  = pHRU->m_currentSnowFall;
            HRU::m_mvCurrentAirTemp   = pHRU->m_currentAirTemp;
				HRU::m_mvCurrentMinTemp   = pHRU->m_currentMinTemp;
            HRU::m_mvCurrentET        = pHRU->m_currentET;
				HRU::m_mvCurrentMaxET	  = pHRU->m_currentMaxET;
            HRU::m_mvCurrentCGDD      = pHRU->m_currentCGDD;
            HRU::m_mvCumRunoff = pHRU->m_runoff_yr;
            HRU::m_mvCumP             = pHRU->m_precip_yr;
            HRU::m_mvCumET            = pHRU->m_et_yr;
            HRU::m_mvCumMaxET        = pHRU->m_maxET_yr;

            for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
               {
               ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

               for( int i=0; i < (int) pGroup->GetSize(); i++ )
                  {
                  ModelOutput *pOutput = pGroup->GetAt( i );
         
                  if ( pOutput->m_inUse == false || pOutput->m_modelDomain != MOD_HRU_IDU )
                     continue;
         
                  bool passConstraints = true;
         
                  if ( pOutput->m_pQuery )
                     {
                     bool result = false;
                     bool ok = pOutput->m_pQuery->Run( idu, result );
               
                     if ( result == false )
                        passConstraints = false;
                     }

                  // did we pass the constraints?  If so, evaluate preferences
                  if ( passConstraints )
                     {
                     pOutput->m_totalQueryArea += area;
                     float value = 0;

                     if ( pOutput->m_pMapExpr != NULL )
                        {
                        bool ok = m_pMapExprEngine->EvaluateExpr( pOutput->m_pMapExpr, pOutput->m_pQuery ? true : false );
                        if ( ok )
                           value = (float) pOutput->m_pMapExpr->GetValue();
                        }

                     // we have a value for this IDU, accumulate it based on the model type
                     switch( pOutput->m_modelType )
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
   for ( int j=0; j < (int) m_modelOutputGroupArray.GetSize(); j++ )
      {
      ModelOutputGroup *pGroup = m_modelOutputGroupArray[ j ];

      // any outputs?
      int moCount = 0;
      for( int i=0; i < (int) pGroup->GetSize(); i++ )
         {
         ModelOutput *pOutput = pGroup->GetAt( i );
         if ( pOutput->m_inUse )
            moCount++;
         if (pOutput->m_pDataObjObs!=NULL)//add 1 column if we have observations for this point.
            moCount++;
         }

      if ( moCount == 0 )
         continue;
            
      float *outputArray = new float[ moCount+1 ];
      outputArray[ 0 ] = (float) m_timeInRun;

      int index = 1;
  
      for( int i=0; i < (int) pGroup->GetSize(); i++ )
         {
         ModelOutput *pOutput = pGroup->GetAt( i );

         if ( pOutput->m_inUse )
            {
            switch( pOutput->m_modelType )
               {
               case MOT_SUM:
                  break;      // no fixup required

               case MOT_AREAWTMEAN:
                  pOutput->m_value /= pOutput->m_totalQueryArea;      //totalArea;
                  break;

               case MOT_PCTAREA:
                  pOutput->m_value /= totalIDUArea;
               }

            outputArray[ index++ ] = pOutput->m_value;
            }
         if (pOutput->m_pDataObjObs != NULL)
            outputArray[index] = pOutput->m_pDataObjObs->IGet((float) m_currentTime);
         }

      pGroup->m_pDataObj->AppendRow( outputArray, moCount+1 );
      delete [] outputArray;
      }

   clock_t finish = clock();

   float duration = (float)(finish - start) / CLOCKS_PER_SEC;   
   gpModel->m_outputDataRunTime += duration;   

   return true;
   }

int FlowModel::AddModelVar( LPCTSTR label, LPCTSTR varName, MTDOUBLE *value )
   {
   ASSERT( m_pMapExprEngine != NULL );

   ModelVar *pModelVar = new ModelVar( label );
   
   m_pMapExprEngine->AddVariable( varName, value );
   
   return (int) m_modelVarArray.Add( pModelVar );
   }
    

/////////////////////////////////////////////////////////////////// 
//  additional functions

void SetGeometry( Reach *pReach, float discharge )
   {
   ASSERT( pReach != NULL );
   pReach->m_depth = GetDepthFromQ(pReach, discharge, pReach->m_wdRatio );
   pReach->m_width = pReach->m_wdRatio * pReach->m_depth;
   }

float GetDepthFromQ( Reach *pReach, float Q, float wdRatio )  // ASSUMES A SPECIFIC CHANNEL GEOMETRY
   {
  // from kellie:  d = ( 5/9 Q n s^2 ) ^ 3/8   -- assumes width = 3*depth, rectangular channel
   float wdterm = (float) pow( (wdRatio/( 2 + wdRatio )), 2.0f/3.0f)*wdRatio;
   float depth  = (float) pow(((Q*pReach->m_n)/((float)sqrt(pReach->m_slope)*wdterm)), 3.0f/8.0f);
   return depth;
   }


// figure out what kind of flux it is
// source string syntax= fn:<dllpath:function> for functional, db:<datasourcepath:columnname> for datasets
FLUXSOURCE ParseSource( LPCTSTR _sourceStr, CString &path, CString &source, HINSTANCE &hDLL, FLUXFN &fn )
   {
   TCHAR sourceStr[ 256 ];
   lstrcpy( sourceStr, _sourceStr );
      
   LPTSTR equal = _tcschr( sourceStr, '=' );
   if ( equal == NULL )
      {
      CString msg;
      msg.Format( "Flow: Error parsing source string '%s'.  The source string must start with either 'fn=' or 'db=' for functional or database sources, respectively", 
            sourceStr );
      Report::ErrorMsg( msg );
      return  FS_UNDEFINED;
      }

   if ( *sourceStr == 'f' )
      {
      //pFluxInfo->m_sourceLocation = FS_FUNCTION;
      LPTSTR _path = equal+1;
      LPTSTR colon = _tcschr( _path, ':' );

      if ( colon == NULL )
         {
         CString msg;
         msg.Format( "Flow: Error parsing source string '%s'.  The source string must be of the form 'fn=dllpath:entrypoint' ", 
                sourceStr );
         Report::ErrorMsg( msg );
         return  FS_UNDEFINED;
         }

      *colon = NULL;
      path = _path;
      path.Trim();
      source = colon+1;
      source.Trim();

      hDLL = AfxLoadLibrary( path ); 
      if ( hDLL == NULL )
         {
         CString msg( _T( "Flow error: Unable to load ") );
         msg += path;
         msg += _T(" or a dependent DLL" );
         Report::ErrorMsg( msg );
         return FS_UNDEFINED;
         }

      fn = (FLUXFN) GetProcAddress( hDLL, source );
      if ( fn == NULL )
         {
         CString msg( "Flow: Unable to find Flow function '");
         msg += source;
         msg += "' in ";
         msg += path;
         Report::ErrorMsg( msg );
         AfxFreeLibrary( hDLL );
         hDLL = NULL;
         return FS_UNDEFINED;         
         }
      return FS_FUNCTION;
      }
   else if ( *sourceStr =='d' )
      {
      //m_sourceLocation = FS_DATABASE;
      LPTSTR _path = equal+1;
      LPTSTR colon = _tcschr( _path, ':' );

      if ( colon == NULL )
         {
         CString msg;
         msg.Format( "Flow: Error parsing Flow source string '%s'.  The source string must be of the form 'db=dllpath:entrypoint' ", 
               sourceStr );
         Report::ErrorMsg( msg );
         return FS_UNDEFINED;
         }

      *colon = NULL;
      path = path;
      path.Trim();
      source = colon+1;
      source.Trim();

      /*
      //int rows = pFluxInfo->m_pData->ReadAscii( pFluxInfo->m_path );  // ASSUMES CSV FOR NOW????
      //if ( rows <= 0  )
      //   {
      //   CString msg( _T( "Flux datasource error: Unable to open/read ") );
      //   msg += path;
      //   Report::ErrorMsg( msg );
      //   pFluxInfo->m_use = false;
      //   continue;
      //   }

      col = pFluxInfo->m_pData->GetCol( pFluxInfo->m_fieldName );
      if ( pFluxInfo->m_col < 0 )
         {
         CString msg( "Unable to find Flux database field '");
         msg += pFluxInfo->m_fieldName;
         msg += "' in ";
         msg += path;
         Report::ErrorMsg( msg );
         pFluxInfo->m_use = false;
         continue;
         } */
      }

   return FS_UNDEFINED;   
   }



float GetVerticalDrainage( float wc )
   {
   //  For Loamy Sand (taken from Handbook of Hydrology).  .
   float wp = 0.035f; 
   float porosity = 0.437f;
   float lamda = 0.553f ; 
   float kSat = 1.0f;//m/d
   float ratio = (wc - wp)/(porosity-wp);
   float power = 3.0f+(2.0f/lamda);
   float kTheta = (kSat) * pow(ratio,power);
   
   if (wc < wp) //no flux if too dry
      kTheta=0.0f;

   return kTheta ;//m/d 
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
   float P = 101.3f * (float) pow(((293 - 0.0065 * elevation) / 293), 5.26);
   float e = specHumid/0.622f * P;
   // temp in deg C
   float E = 6.112f * (float) exp(17.62*tMean/(243.12+tMean));
   // rel. humidity [-]
   float ret = e/E;

   return ret;
   }

