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
//
#include "StdAfx.h"
#pragma hdrstop

#include "GlobalMethods.h"
#include "Flow.h"
#include <PathManager.h>
#include <Maplayer.h>
#include <UNITCONV.H>


FluxExpr::FluxExpr( FlowModel *pFlowModel, LPCTSTR name)
   : GlobalMethod(pFlowModel, name, GM_FLUX_EXPR )
   , m_isDynamic( false )
   , m_fluxType( FT_NONE )
   , m_sourceDomain( FD_UNDEFINED )
   , m_sinkDomain( FD_UNDEFINED )
   , m_valueType( VT_EXPR )
   , m_valueDomain( FD_UNDEFINED )
   , m_colJoinSrc( -1 )                // desired demand (m3/sec)
   , m_colJoinSink( -1 )
   , m_colStartDate( -1 )
   , m_colEndDate( -1 )
   , m_colLimit( -1 )
   , m_colMinInstreamFlow( -1 )
   , m_colDailyOutput( -1 )
   , m_colAnnualOutput( -1 )
   , m_colSourceDailyDemand( -1 )
   , m_colSourceAnnDemand( -1 )
   , m_colSourceUnsatisfiedDemand( -1 )
   , m_startDate( -1 )
   , m_endDate( -1 )
   , m_withdrawalCutoffCoeff( 0.1f )
   , m_lossCoeff( 0.0f )
   , m_collectOutput( false )
   , m_pSourceLayer( NULL )
   , m_pSinkLayer( NULL )
   , m_pValueLayer( NULL )
   , m_pMapExprEngine( NULL )
   , m_pMapExpr( NULL )
   , m_pValueData( NULL )
   , m_pSourceQE( NULL )
   , m_pSinkQE( NULL )
   , m_pSourceQuery( NULL )
   , m_pSinkQuery( NULL )
   , m_pDailyFluxData( NULL )
   , m_pCumFluxData( NULL )
   , m_dailySatisfiedDemand( 0 )  // m3/day
   , m_dailySinkDemand( 0 )   // m3/day
   , m_dailyUnsatisfiedDemand( 0 ) // m3/day  
   , m_cumSatisfiedDemand( 0 )  // m3/day
   , m_cumSinkDemand( 0 )   // m3/day
   , m_cumUnsatisfiedDemand( 0 ) // m3/day  
   , m_totalSinkArea( 0 )   // m2
   , m_annDailyAvgFlux( 0 )  // m3/day
   , m_stepCount( 0 )
   {  
   this->m_timing = GMT_CATCHMENT;

//   GMT_INIT       = 1,       // called during Init();
//   GMT_INITRUN    = 2,       // called during InitRun();
//   GMT_START_YEAR = 4,       // called at the beginning of a Envision timestep (beginning of each year of run)  (PreRun)
//   GMT_START_STEP = 8,       // called at the beginning of a Flow timestep PreRun)
//   GMT_CATCHMENT  = 16,      // called during GetCatchmentDerivatives() 
//   GMT_END_STEP   = 128,     // called at the beginning of a Flow timestep PreRun)
//   GMT_END_YEAR   = 256,     // called at the end of a Envision timestep (beginning of each year of run) (PostRun)
//   GMT_ENDRUN     = 512      // called during EndRun()



   }


FluxExpr::~FluxExpr(void)
   {
   if ( m_pSourceQE != NULL )
      delete m_pSourceQE;

   if ( m_pSinkQE != NULL )
      delete m_pSinkQE;

   if ( m_pMapExprEngine != NULL )
      {
      m_pMapExprEngine->RemoveExpr( m_pMapExpr );
      delete m_pMapExprEngine;
      }   

   if ( m_pValueData != NULL )
      delete m_pValueData;

   if ( m_pDailyFluxData != NULL )
      delete m_pDailyFluxData;

   if ( m_pCumFluxData != NULL )
      delete m_pCumFluxData;
   }


bool FluxExpr::Init( FlowContext *pFlowContext )
   {
   FlowModel *pFlowModel = pFlowContext->pFlowModel;

   MapLayer *pLayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;

   // must have a source layer
   if ( m_pSourceLayer == NULL ) // && m_pSinkLayer == NULL )
      {
      Report::ErrorMsg("Flow: Missing source layer when initializing Flux.");
      return false;
      }
   
   if ( this->m_pValueLayer == NULL )
      return false;

   if ((m_joinField.GetLength() > 0) || (m_dailyOutputField.GetLength() > 0) || (m_annualOutputField.GetLength() > 0))
      {
      if (m_pSinkLayer == NULL)
         {
         Report::ErrorMsg("Flow: Missing sink layer when initializing Flux.");
         return false;
         }
      }

   if (m_joinField.GetLength() > 0)
      {
      m_pSourceLayer->CheckCol( m_colJoinSrc,  m_joinField, TYPE_INT, CC_MUST_EXIST );
      m_pSinkLayer->CheckCol  ( m_colJoinSink, m_joinField, TYPE_INT, CC_MUST_EXIST );
      }
   
   m_pSourceLayer->CheckCol( m_colSourceDailyDemand, "IRR_DEMAND", TYPE_FLOAT, CC_AUTOADD ); 
   m_pSourceLayer->CheckCol( m_colSourceAnnDemand, "ANN_DEMAND", TYPE_FLOAT, CC_AUTOADD ); 

   m_pSourceLayer->SetColData( m_colSourceDailyDemand, 0.0f, true );
   m_pSourceLayer->SetColData( m_colSourceAnnDemand, 0.0f, true );

   if ( m_dailyOutputField.GetLength() > 0 )
      {
      m_pSinkLayer->CheckCol( m_colDailyOutput, m_dailyOutputField, TYPE_FLOAT, CC_AUTOADD );
      m_pSinkLayer->SetColData( m_colDailyOutput, VData( (float) 0 ), true );
      }

   if ( m_annualOutputField.GetLength() > 0 )
      {
      m_pSinkLayer->CheckCol( m_colAnnualOutput, m_annualOutputField, TYPE_FLOAT, CC_AUTOADD );
      m_pSinkLayer->SetColData( m_colAnnualOutput, VData( (float) 0 ), true );
      }

   if ( m_limitField.GetLength() > 0 )
      m_pValueLayer->CheckCol( m_colLimit, m_limitField, TYPE_FLOAT, CC_MUST_EXIST );

   if ( m_minInstreamFlowField.GetLength() > 0 )
      m_pSourceLayer->CheckCol( m_colMinInstreamFlow, m_minInstreamFlowField, TYPE_FLOAT, CC_MUST_EXIST );

    // compile queries
   if ( m_sourceQuery.GetLength() > 0 )
      {
      m_pSourceQE = new QueryEngine( m_pSourceLayer );
      m_pSourceQuery = m_pSourceQE->ParseQuery( m_sourceQuery, 0, "Source Query" );
      }

   if ( m_sinkQuery.GetLength() > 0 )
      {
      m_pSinkQE    = new QueryEngine( m_pSinkLayer );
      m_pSinkQuery = m_pSinkQE->ParseQuery( m_sinkQuery, 0, "Sink Query"  );
      }

   // compile value expression
   
   switch( m_valueType )
      {
      case VT_EXPR:
         m_pMapExprEngine = new MapExprEngine( m_pValueLayer, pFlowContext->pEnvContext->pQueryEngine );
         m_pMapExpr = m_pMapExprEngine->AddExpr( m_name, m_expr, NULL );
         m_pMapExpr->Compile();
         break;

      case VT_TIMESERIES:
         {
         m_pValueData = new FDataObj( 2, 0 );
         // parse value as pairs of time series
         TCHAR *values = new TCHAR[ this->m_expr.GetLength()+2 ];
         memset( values, 0, (this->m_expr.GetLength()+2) * sizeof( TCHAR ) );

         lstrcpy( values, this->m_expr );
         TCHAR *nextToken = NULL;

         // note: target values look like sequences of x/y pairs
         LPTSTR token = _tcstok_s( values, _T(",() ;\r\n"), &nextToken );
         float pair[2];
         while ( token != NULL )
            {
            pair[0] = (float) atof( token );
            token   = _tcstok_s( NULL, _T(",() ;\r\n"), &nextToken );
            pair[1] = (float) atof( token );
            token   = _tcstok_s( NULL, _T(",() ;\r\n"), &nextToken );
   
            m_pValueData->AppendRow( pair, 2 );
            }

         delete [] values;
         }
         break;

      case VT_FILE:
         m_pValueData = new FDataObj;
         CString path;
         PathManager::FindPath( m_expr, path );
         m_pValueData->ReadAscii( path );

         if ( m_pValueData->GetRowCount() <= 0 )
            {
            CString msg;
            msg.Format( "Flow: Unable to read file %s when initializing Flux '%s'.  This flux will be disabled", (LPCTSTR) this->m_expr, this->m_name );
            Report::ErrorMsg( msg );
            this->m_use = false;
            }
         break;
      }

   // Note: queries and expressions are set up in the LoadXml section
   int straws = BuildSourceSinks();

   // set up data obj for annual summaries
   CString label;
   //label.Format( "%s - Daily Flux (Annual Avg, m3 per day)", (LPCSTR) m_name );
   //m_annDailyAvgFluxData.SetName( label );
   //m_annDailyAvgFluxData.SetSize(2, 0);
   //m_annDailyAvgFluxData.SetLabel(0, "Time (days)");
   //m_annDailyAvgFluxData.SetLabel(1, label);
   //gpFlow->AddOutputVar( label, &m_annDailyAvgFluxData, "" );

   // for cross-run comparisons
   label.Format( "%s - Annual Avg Flux, m3 per year)", (LPCSTR) m_name );
   pFlowModel->AddOutputVar(label, m_annDailyAvgFlux, "");
   
   // set up data obj for daily fluxes
   m_pDailyFluxData = new FDataObj( 4, 0 );
   label.Format( "%s - Daily Flux Components", (LPCSTR) m_name );
   m_pDailyFluxData->SetName( label );
   m_pDailyFluxData->SetLabel( 0, "Time (days)" );

   m_pDailyFluxData->SetLabel( 1, "Daily Sink Demand (m3 per day)" );         // m_dailySinkDemand
   m_pDailyFluxData->SetLabel( 2, "Daily Satisfied Demand (m3 per day)" );    // m_dailySatisfiedDemand
   m_pDailyFluxData->SetLabel( 3, "Daily Unsatisfied Demand (m3 per day)" );  // m_dailyUnsatifiedDemand

   pFlowModel->AddOutputVar( label, m_pDailyFluxData, "" );

   label.Format( "%s - Cumulative Flux Components", (LPCSTR) m_name );
   m_pCumFluxData = new FDataObj( 4, 0 );
   m_pCumFluxData->SetName( label );
   m_pCumFluxData->SetLabel( 0, "Time (days)" );

   m_pCumFluxData->SetLabel( 1, "Cumulative Sink Demand (m3)" );            // m_dailySinkDemand
   m_pCumFluxData->SetLabel( 2, "Cumulative Satisfied Demand (m3)" );       // m_dailySatisfiedDemand
   m_pCumFluxData->SetLabel( 3, "Cumulative Unsatisfied Demand (m3)" );     // m_dailyUnsatifiedDemand

   pFlowModel->AddOutputVar( label, m_pCumFluxData, "" );

   return true;
   }


bool FluxExpr::InitRun( FlowContext *pFlowContext )
   {
   m_pDailyFluxData->ClearRows();
   m_pCumFluxData->ClearRows();
   //m_annDailyAvgFluxData.ClearRows();

   return true;
   }


bool FluxExpr::StartYear( FlowContext *pFlowContext )
   {
   if ( m_isDynamic )
      BuildSourceSinks();

   m_cumSinkDemand = 0;
   m_cumSatisfiedDemand = 0;
   m_cumUnsatisfiedDemand = 0;
   
   m_annDailyAvgFlux = 0;

   // initialize available discharge levels for the reaches
   //int reachCount = pFlowContext->pFlowModel->GetReachCount();
   //
   //for ( int i=0; i < reachCount; i++ )
   //   {
   //   Reach *pReach = pFlowContext->pFlowModel->GetReach( i );     // Note: these are guaranteed to be non-phantom
   //   pReach->m_availableDischarge = pReach->GetDischarge();
   //   }
   if ( m_colAnnualOutput >= 0 )
      {
      bool readOnly = m_pSinkLayer->m_readOnly;
      m_pSinkLayer->m_readOnly = false;
      m_pSinkLayer->SetColData( m_colAnnualOutput, VData( (float) 0 ), true );
      m_pSinkLayer->m_readOnly = readOnly;
      }
      
   if ( m_colSourceAnnDemand >= 0 )
      {
      bool readOnly = m_pSourceLayer->m_readOnly;
      m_pSourceLayer->m_readOnly = false;
      m_pSourceLayer->SetColData( m_colSourceAnnDemand, 0, true );
      m_pSourceLayer->m_readOnly = readOnly;
      }
      
   return true;
   }


bool FluxExpr::StartStep( FlowContext *pFlowContext )
   {
   m_dailySinkDemand = 0.0f;
   m_dailySatisfiedDemand = 0.0f;
   m_dailyUnsatisfiedDemand = 0.0f;
   
   m_totalSinkArea = 0.0F;

   m_stepCount = 0;

   return true;
   }


// called every step (possibly multiple times)
bool FluxExpr::Step( FlowContext *pFlowContext )
   {
   FlowModel *pFlowModel = pFlowContext->pFlowModel;

   m_stepCount++;

   // reset mapped outputs
   if ( m_colDailyOutput >= 0 )
      {
      m_pSinkLayer->m_readOnly = false;
      m_pSinkLayer->SetColData( m_colDailyOutput, 0, true );
      m_pSinkLayer->m_readOnly = true;
      }

   // check start, ending date constraints if defined
   if ( m_startDate >= 0 && pFlowContext->dayOfYear < m_startDate )  // start date constraint?
      return true;

   if ( m_endDate >= 0 && pFlowContext->dayOfYear > m_endDate )  // end date constraint?
      return true;

   // basic idea - iterate through the straws, allocating water based on demand
   FlowModel *pModel = pFlowContext->pFlowModel;

   // first, initialize available discharge for each reach
   int reachCount = pModel->GetReachCount();
   for ( int i=0; i < reachCount; i++ )
      {
      Reach *pReach = pModel->GetReach( i );     // Note: these are guaranteed to be non-phantom
      pReach->m_availableDischarge = pReach->GetUpstreamInflow();   // m3/sec
      }

   // start iterating through the straws
   // for each straw, see if it is connected to a source that passes the
   // sourceQuery, if defined.
   // If is it a valid source, compute the associated flux,
   // and distribute this flux FROM the source and TO all the
   // sinks in a way that preserves mass

   int colSinkArea = -1;
   if ( m_pSinkLayer != NULL ) 
      colSinkArea = m_pSinkLayer->GetFieldCol( "Area" );
   
   int strawCount = (int) m_ssArray.GetSize();
   for ( int i=0; i < strawCount; i++ ) 
      {
      // get the source and sink(s) associated with a particular straw
      FluxSourceSink *pStraw = m_ssArray[ i ];

      int sourceIndex = pStraw->m_sourceIndex;

      bool passSourceQuery = true;

      // if source or sink queries exist, see if they are satisfied
      if ( m_pSourceQuery != NULL )
         m_pSourceQuery->Run( sourceIndex, passSourceQuery );

      if ( passSourceQuery )
         {
         int sinkCount = (int) pStraw->m_sinkIndexArray.GetSize();
         float totalStrawSinkArea = 0;

         float *areaArray = NULL;
         float *fluxArray = NULL;

         if ( sinkCount > 0 )  // sinks defined, then allocate info containers for them
            {
            areaArray = new float[ sinkCount ];  // IDU areas, one for each sink IDU
            fluxArray = new float[ sinkCount ];  // flux values for each sink IDU, mm/day 
            }  // note:  should allocate as part of the straw to avoid repeated allocations!!!!

         // pass 1 - see which sink poly's to use (store in areaArray)
         for ( int j=0; j < sinkCount; j++ )
            {
            int sinkIndex = pStraw->m_sinkIndexArray[ j ];
            bool passSinkQuery = true;

            if ( m_pSinkQuery != NULL )
               m_pSinkQuery->Run( sinkIndex, passSinkQuery );

            if ( passSinkQuery )
               {
               float area = 0;
               if ( m_pSinkLayer != NULL )
                  m_pSinkLayer->GetData( sinkIndex, colSinkArea, area );

               // are there a timing constraints?
               bool passTimeConstraint = true;
               if ( this->m_colStartDate > 0 )
                  {
                  int startDate = -1;
                  if ( m_pSinkLayer != NULL )
                     m_pSinkLayer->GetData( sinkIndex, m_colStartDate, startDate );

                  if ( startDate >= 0 && pFlowContext->dayOfYear < startDate )
                     passTimeConstraint = false;
                  }

               if ( this->m_colEndDate > 0 )
                  {               
                  int endDate = -1;
                  if ( m_pSinkLayer )  
                     m_pSinkLayer->GetData( sinkIndex, m_colEndDate, endDate );

                  if ( endDate >= 0 && pFlowContext->dayOfYear > endDate )
                     passTimeConstraint = false;
                  }

               if ( ! passTimeConstraint )
                  areaArray[ j ] = 0;
               else
                  {
                  areaArray[ j ] = area;
                  totalStrawSinkArea += area;
                  }
               }
            else
               areaArray[ j ] = 0;
            }  // end of pass one

         // any need to allocate flux?
         if ( m_fluxType == FT_SOURCE || totalStrawSinkArea > 0 )  // one ended straw?  or any polys that use sink?
            {
            // calculate flux from value expression for the IDU and move water from source to sink

            // if flux dewaters the source, then limit to what is available, considering minimum instream flow if defined
            float minFlow = 0;
            if ( m_colMinInstreamFlow >= 0 )
               m_pSourceLayer->GetData( sourceIndex, m_colMinInstreamFlow, minFlow );

            // pull from source 
            float availSourceFlow=1E12f;   // initially, essentially unlimited
            Reach *pReach = NULL;
            if (m_sourceDomain==FD_REACH) //if the domain is a reach, then limit based on discharge.  If not, leave availSourceFlow as a large number (no limitation)
               { 
               pReach = pModel->GetReachFromStreamIndex(sourceIndex);
               ASSERT( pReach != NULL );
               availSourceFlow = pReach->m_availableDischarge;  // m3/sec
                                                                // make sure min flows are maintained
               availSourceFlow -= minFlow;      // m3/sec
               availSourceFlow *= SEC_PER_DAY;  // convert to m3/day
               }
            else if (m_sourceDomain == FD_RESERVOIR) // reservoir
               {
               // source index will be the first IDU that satisfies the query
               pReach = pModel->GetReachFromStreamIndex(sourceIndex);
               // assume unlimited flow  - should fix this!!!
               }
  
            // available source determined, allocate to sinks
            //if ( availSourceFlow > 0 )
               {
               m_totalSinkArea += totalStrawSinkArea;

               // compare the calculated flux to the available source flow.
               // to do this, we need IDU areas, since the flux expr units are mm/day,
               // availSourceFlow is m3/sec

               // estimate total sink demand
               float totalSinkDemand = 0;   // m3/day
               
               switch( m_fluxType )
                  {
                  case FT_SOURCE:   // source-only straw? then expression based on source
                     // calculate total sink demand (m3/sec) in the case of one-ended straws (mm/day otherwise)
                     if ( m_valueType == VT_EXPR )
                        {
                        m_pMapExpr->Evaluate(sourceIndex);
                        totalSinkDemand = (float)m_pMapExpr->GetValue();   
                        }
                     else
                        {
                        if ( m_pValueData != NULL )
                           totalSinkDemand = m_pValueData->IGet( (float) pFlowContext->dayOfYear );
                        }

                     //totalSinkDemand /= (1.0f - this->m_lossCoeff);  // increase sink demand to account for conveyance losses
                     break;

                  case FT_SOURCESINK:
                  case FT_SINK:
                     {            
                     // for each sink, determine the flux associated with the sink polygon.  Units in this case are mm/day.
                     // These are calculated for each sink IDU and stored in the fluxArray.  the total sink demand
                     // is the sum of all the individual sink IDU fluxes 
                     for ( int j=0; j < sinkCount; j++ )
                        {
                        // calculate the flux value from the expression or lookup from table.  We assume that the 
                        // value expression provides a flux rate (mm/day) that is taken from
                        // the source, and if a sink is defined, the flux is applied to the 
                        // target sink area.  We use this to generate a flux demand for the entire
                        // sink area of the straw.
                        // then, we try to satisy demand.
                        float iduFlux = 0;
                        if ( areaArray[ j ] > 0 )  // does this IDU get flux?
                           {
                           int sinkIndex = pStraw->m_sinkIndexArray[ j ];
                           if ( m_valueType == VT_EXPR )
                              {
                              m_pMapExpr->Evaluate( sinkIndex );
                              iduFlux = (float) m_pMapExpr->GetValue(); // mm/day
                              }
                           else if ( m_pValueData != NULL )
                              {
                              iduFlux = m_pValueData->IGet( (float) pFlowContext->pFlowModel->m_currentTime);  // mm/day
                              }

                           //iduFlux /= (1.0f - this->m_lossCoeff);     // increase IDU demand to account for conveynace losses

                           fluxArray[ j ] = iduFlux;     // mm/day, NOT including conveyance loss
                           totalSinkDemand += iduFlux * areaArray[ j ] / MM_PER_M;   // m3/day  
                           }
                        else
                           fluxArray[ j ] = 0;
                        }
                     }
                     break;
                  }  // end of: switch( m_fluxType )

               // adjust total demand for conveyance loss
               totalSinkDemand /= (1.0f - this->m_lossCoeff );

               // we now have total demand (m3/d), adjusted to include conveyance losses. compare to avail supply (m3/day)              
               float totalSatisfiedDemand = totalSinkDemand;
               float totalUnsatisfiedDemand = 0;  // m3/day

               if ( totalSinkDemand > availSourceFlow * m_withdrawalCutoffCoeff )   // 0.1 - to take care of low flow problem with stream routing
                  {
                  totalSatisfiedDemand   = availSourceFlow * m_withdrawalCutoffCoeff;
                  totalUnsatisfiedDemand = totalSinkDemand-totalSatisfiedDemand;
                  }
               
               float satRatio = totalSatisfiedDemand / totalSinkDemand;

               // Pass 2 - allocate the flux across the sink IDUs connected to this straw.
               // Note that if this is a source-only flux, sinkCount will be 0 and this loop will be skipped
               for ( int j=0; j < sinkCount; j++ )
                  {
                  int sinkIndex = pStraw->m_sinkIndexArray[ j ];   // this is an IDU index

                  // any flux to allocate?
                  if ( fluxArray != NULL && fluxArray[ j ] > 0 )
                     {
                     // get corresponding HRU
                     HRU *pHRU = GlobalMethod::m_iduHruPtrArray[ sinkIndex ];
                     ASSERT( pHRU != NULL );
   
                     // convert mm/day flux value to m3/day associated with the sink area
                     float iduFlux = fluxArray[ j ] * satRatio * areaArray[ j ] / MM_PER_M;   // mm/day * m2 / mm/m = m3/day, included conveyance loss
                     
							// choose the layer distribution that matches
							int poolIndex = m_pLayerDists->GetPoolIndex(sinkIndex);
							if (poolIndex != -1)
							   {
								// add to sink, according to layer distribution array
								int layerDistArraySize = (int)m_pLayerDists->m_poolArray[poolIndex]->poolDistArray.GetSize();
								for (int k = 0; k < layerDistArraySize; k++)
								   {
									PoolDist *pLD = m_pLayerDists->m_poolArray[poolIndex]->poolDistArray[k];
									HRUPool *pHRUPool = pHRU->GetPool(pLD->poolIndex);

									float layerFlux = iduFlux * pLD->fraction ;     // m3/day, with conveyance losses removed, so this is actual applied
                           
									pHRUPool->AddFluxFromGlobalHandler(layerFlux, FL_TOP_SOURCE);     // m3/d
								   }
							   } // end layer choice

                     // output results to sink layer (as mm/day)
                     if ( m_colDailyOutput >= 0 && m_fluxType != FT_SOURCE )
                        {
                        m_pSinkLayer->m_readOnly = false;
                        m_pSinkLayer->SetData( sinkIndex, m_colDailyOutput, fluxArray[ j ] );
                        m_pSinkLayer->m_readOnly = true;
                        }

                     // accumulate annual results in IDU coverage
                     if ( m_colAnnualOutput >= 0 )
                        {
                        float yearToDateFlux = 0;
                        m_pSinkLayer->GetData( sinkIndex, m_colAnnualOutput, yearToDateFlux );
                        yearToDateFlux += iduFlux;  // note: iduFlux does not includes conveyance losses, but rather actual water applied to IDU

                        m_pSinkLayer->m_readOnly = false;
                        m_pSinkLayer->SetData( sinkIndex, m_colAnnualOutput, yearToDateFlux );  // m3/day
                        m_pSinkLayer->m_readOnly = true;
                        }
                     }  // end of: if ( areaArray[ i ] > 0 ) - any flux to allocate?
                  }  // end of: for each sink

               // remove water 
               if (m_sourceDomain == FD_CATCHMENT) //HRU
                  {
                     // get corresponding HRU
                  HRU *pHRU = GlobalMethod::m_iduHruPtrArray[sourceIndex];
                  ASSERT(pHRU != NULL);

                  // convert mm/day flux value to m3/day associated with the sink area
                 // float iduFlux = fluxArray[j] * satRatio * areaArray[j] / MM_PER_M;   // mm/day * m2 / mm/m = m3/day, included conveyance loss

                                                                                       // choose the layer distribution that matches
                  int poolIndex = m_pLayerDists->GetPoolIndex(sourceIndex);
                  if (poolIndex != -1)
                     {
                     // add to source, according to layer distribution array
                     int layerDistArraySize = (int)m_pLayerDists->m_poolArray[poolIndex]->poolDistArray.GetSize();
                     for (int k = 0; k < layerDistArraySize; k++)
                        {
                        PoolDist *pLD = m_pLayerDists->m_poolArray[poolIndex]->poolDistArray[k];
                        HRUPool *pHRUPool = pHRU->GetPool(pLD->poolIndex);

                        float layerFlux = totalSinkDemand * pLD->fraction * pHRU->m_area / MM_PER_M;     // m3/day, with conveyance losses removed, so this is actual applied
                        pHRUPool->AddFluxFromGlobalHandler(layerFlux, FL_TOP_SINK);     // m3/d
                        }
                     } // end layer choice

                  }
               else if (m_sourceDomain == FD_REACH ) // || m_sourceDomain == FD_RESERVOIR) //reach/reservoir
                  {
                  pReach->AddFluxFromGlobalHandler( -totalSatisfiedDemand );              // m3/d, includes additional conveyance losses
                  pReach->m_availableDischarge -= totalSatisfiedDemand / SEC_PER_DAY;     // m3/sec
                  }
               else if (m_sourceDomain == FD_RESERVOIR) //reach/reservoir
                  {
                  Reservoir *pRes = pReach->m_pReservoir;
                  ASSERT(pRes != NULL);
                  pRes->AddFluxFromGlobalHandler(-totalSatisfiedDemand);              // m3/d, includes additional conveyance losses
                  }

               m_pSourceLayer->SetData( sourceIndex, m_colSourceDailyDemand, totalSatisfiedDemand / SEC_PER_DAY );  // m3/sec

               if ( m_colSourceUnsatisfiedDemand >= 0 )
                  m_pSourceLayer->SetData( sourceIndex, m_colSourceUnsatisfiedDemand, totalUnsatisfiedDemand / SEC_PER_DAY );  // m3/sec

               float demandSoFar = 0;
               m_pSourceLayer->GetData( sourceIndex, m_colSourceAnnDemand, demandSoFar );
               m_pSourceLayer->SetData( sourceIndex, m_colSourceAnnDemand, demandSoFar + totalSatisfiedDemand / SEC_PER_DAY );  // m3/sec

               m_dailySatisfiedDemand   += totalSatisfiedDemand;    // m3/day, adjusted for conveyance losses
               m_dailySinkDemand        += totalSinkDemand;         // m3/day, adjusted for conveyance losses
               m_dailyUnsatisfiedDemand += totalUnsatisfiedDemand;  // m3/day, adjusted for conveyance losses
               }  // end of: if ( availSource > 0 )
            }  // end of: if ( totalStrawSinkArea > 0 )

         if ( sinkCount > 0 )
            {
            delete [] fluxArray;
            delete [] areaArray;
            }
         }  // end of: if ( passSourceQuery )
      }  // end of: for ( each straw );

   return true;
   }


bool FluxExpr::EndStep(FlowContext *pFlowContext)
   {
   MapLayer *pLayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;
   ASSERT( m_stepCount > 0 );

   m_dailySatisfiedDemand   /= m_stepCount;   // m3/day
   m_dailyUnsatisfiedDemand /= m_stepCount;   // m3/day
   m_dailySinkDemand        /= m_stepCount;   // m3/day

   m_cumSatisfiedDemand     += m_dailySatisfiedDemand;    // m3/day
   m_cumSinkDemand          += m_dailySinkDemand;         // m3/day
   m_cumUnsatisfiedDemand   += m_dailyUnsatisfiedDemand;  // m3/day

   m_totalSinkArea        /= m_stepCount;   // m2

   m_annDailyAvgFlux += m_dailySatisfiedDemand;  // m3/day - this accumulates across area and time

   float time = pFlowContext->time - pFlowContext->pEnvContext->startYear*365;

   float data[ 4 ];
   data[ 0 ] = time;
   data[ 1 ] = m_dailySinkDemand;
   data[ 2 ] = m_dailySatisfiedDemand;
   data[ 3 ] = m_dailyUnsatisfiedDemand;
   m_pDailyFluxData->AppendRow( data, 4 );
   

   data[ 1 ] = m_cumSinkDemand;
   data[ 2 ] = m_cumSatisfiedDemand;
   data[ 3 ] = m_cumUnsatisfiedDemand;
   m_pCumFluxData->AppendRow( data, 4 );
   
   return true;
   }


bool FluxExpr::EndYear(FlowContext *pFlowContext)
   {
   //MapLayer *pLayer = (MapLayer*) pFlowContext->pEnvContext->pMapLayer;
   //ASSERT( pLayer == m_pSinkLayer );

   /// this should be current year
   float year = (float) pFlowContext->pEnvContext->currentYear;

   //float annFluxData[2];
   //annFluxData[0] = year;
   //annFluxData[1] = m_annDailyAvgFlux;
   //m_annDailyAvgFluxData.AppendRow(annFluxData, 2);

   //m_annDailyAvgFlux = 0.0f;

   // output results to sink layer
   if ( m_colAnnualOutput >= 0 && m_pSinkLayer != NULL )
      {
      int colArea = m_pSinkLayer->GetFieldCol( "Area" );
   
      for ( MapLayer::Iterator i=m_pSinkLayer->Begin(); i < m_pSinkLayer->End(); i++ )
         {
         float yearToDateFlux = 0;
         m_pSinkLayer->GetData( i, m_colAnnualOutput, yearToDateFlux );   // m3/day

         yearToDateFlux /= ( 365 * m_stepCount );  // m3/day, avg over year for this IDU

         // convert to mm
         float area = 0;
         m_pSinkLayer->GetData( i, colArea, area );

         yearToDateFlux *= ( MM_PER_M / area );  // mm/day
         if (!pFlowContext->pFlowModel->m_estimateParameters)
            pFlowContext->pFlowModel->UpdateIDU( pFlowContext->pEnvContext, i, m_colAnnualOutput, yearToDateFlux, ADD_DELTA );
         }
      }

   // output results to source layer
   for  ( int i=0; i < (int) m_ssArray.GetSize(); i++ )
      {
      FluxSourceSink *pSS = m_ssArray[ i ];

      float annAvgDemand = 0;
      m_pSourceLayer->GetData( pSS->m_sourceIndex, m_colSourceAnnDemand, annAvgDemand );  // m3/day, summed over years and steps;

      annAvgDemand /= 365 * m_stepCount;   // convert to avg m3/day
      annAvgDemand /= SEC_PER_DAY;         // m3/sec

      m_pSourceLayer->SetData( pSS->m_sourceIndex, m_colSourceAnnDemand, annAvgDemand );
      }

   return true;
   }


FluxExpr *FluxExpr::LoadXml( TiXmlElement *pXmlFluxExpr, MapLayer *pIDULayer, LPCTSTR filename, FlowModel *pModel)
   {
   if ( pXmlFluxExpr == NULL )
     return NULL;

   LPTSTR name = NULL, sourceQuery=NULL, sinkQuery=NULL, joinCol=NULL, sourceDomain=NULL, sinkDomain=NULL, value=NULL,
      layerDist=NULL, valueDomain=NULL, limitCol=NULL, dailyOutputCol=NULL, annualOutputCol=NULL, minCol=NULL,
      startDateCol=NULL, endDateCol=NULL, unsatisfiedOutputCol=NULL;
   
   LPTSTR valueType="expression";

   int startDate=-1, endDate=-1;
   bool use=true, dynamic=false, collectOutput=false;
   float withdrawalCutoffCoeff = 0.1f, lossCoeff = 0.0f;

   XML_ATTR attrs[] = {
      // attr                 type          address               isReq  checkCol
      { "name",               TYPE_STRING,    &name,              true,    0 },
      { "use",                TYPE_BOOL,      &use,               false,   0 },
      { "join_col",           TYPE_STRING,    &joinCol,           false,   0 },      
      { "source_domain",      TYPE_STRING,    &sourceDomain,      false,   0 },      
      { "sink_domain",        TYPE_STRING,    &sinkDomain,        false,   0 },
      { "source_query",       TYPE_STRING,    &sourceQuery,       true,    0 },      
      { "sink_query",         TYPE_STRING,    &sinkQuery,         false,   0 },
      { "value_domain",       TYPE_STRING,    &valueDomain,       true,    0 },
      { "value_type",         TYPE_STRING,    &valueType,         false,    0 },
      { "value",              TYPE_STRING,    &value,             true,    0 },
      { "min_col",            TYPE_STRING,    &minCol,            false,   0 },
      { "limit_col",          TYPE_STRING,    &limitCol,          false,   0 },
      { "daily_output_col",   TYPE_STRING,    &dailyOutputCol,    false,   0 },
      { "annual_output_col",  TYPE_STRING,    &annualOutputCol,   false,   0 },
      { "unsatisfied_output_col", TYPE_STRING,  &unsatisfiedOutputCol,   false, 0 },
      { "dynamic",            TYPE_BOOL,      &dynamic,           false,   0 },
      { "layer_distributions",TYPE_STRING,    &layerDist,         false,   0 },
      { "startdate_col",      TYPE_STRING,    &startDateCol,      false,   0 },
      { "enddate_col",        TYPE_STRING,    &endDateCol,        false,   0 },
      { "startdate",          TYPE_INT,       &startDate,         false,   0 },
      { "enddate",            TYPE_INT,       &endDate,           false,   0 },
      { "withdrawal_cutoff_coeff", TYPE_FLOAT,&withdrawalCutoffCoeff, false, 0 },
      { "loss_coeff",         TYPE_FLOAT,     &lossCoeff,         false,   0 },
      { NULL,                 TYPE_NULL,      NULL,               false,   0 } };

   bool ok = TiXmlGetAttributes( pXmlFluxExpr, attrs, filename );
   if ( ! ok )
     {
     CString msg; 
     msg.Format( _T("Flow: Misformed element reading global method <flux> attributes in input file %s"), filename );
     Report::ErrorMsg( msg );
     return NULL;
     }
   
   FluxExpr *pFluxExpr = new FluxExpr(pModel, name );
	// set layer distributions
	pFluxExpr->m_pLayerDists = new PoolDistributions;
   if ( layerDist != NULL )
      pFluxExpr->m_pLayerDists->ParsePoolDistributions( layerDist );
   else
   	pFluxExpr->m_pLayerDists->ParsePools(pXmlFluxExpr, pIDULayer, filename);

   pFluxExpr->m_joinField = joinCol;
   
   if ( dailyOutputCol != NULL )
      pFluxExpr->m_dailyOutputField = dailyOutputCol;

   if ( annualOutputCol != NULL )
      pFluxExpr->m_annualOutputField = annualOutputCol;

   if ( minCol != NULL )
      pFluxExpr->m_minInstreamFlowField = minCol;

   if ( unsatisfiedOutputCol != NULL )
      {
      pFluxExpr->m_unsatisfiedOutputField = unsatisfiedOutputCol;
      pIDULayer->CheckCol( pFluxExpr->m_colStartDate, startDateCol, TYPE_INT, CC_MUST_EXIST );   // ????
      }

   if ( startDateCol != NULL )
      pFluxExpr->m_startDateField = startDateCol;

   pFluxExpr->m_startDate = startDate;
   pFluxExpr->m_endDate = endDate;

   if ( endDateCol != NULL )
      {
      pFluxExpr->m_endDateField = endDateCol;
      pIDULayer->CheckCol( pFluxExpr->m_colEndDate, endDateCol, TYPE_INT, CC_MUST_EXIST );
      }

   if ( limitCol != NULL )
      pFluxExpr->m_limitField = limitCol;

   if ( sourceDomain == NULL )
      sourceDomain = "reach";
   //if ( sinkDomain == NULL )
   //   sinkDomain = "catchment";
   if ( valueDomain == NULL )
      valueDomain = "sink";

   switch( sourceDomain[0] )
      {
      case 'c':   // catchment
         pFluxExpr->m_sourceDomain = FD_CATCHMENT;   
         pFluxExpr->m_pSourceLayer = pModel->m_pCatchmentLayer;
         break;

      case 'r':   // reach or reservoir
         if ( sourceDomain[ 2 ] == 'a' )  // reach
            {
            pFluxExpr->m_sourceDomain = FD_REACH;
            pFluxExpr->m_pSourceLayer = pModel->m_pStreamLayer;
            }
         else
            {
            pFluxExpr->m_sourceDomain = FD_RESERVOIR;
            pFluxExpr->m_pSourceLayer = pModel->m_pStreamLayer;   // m_pResLayer;
            }
         break;
      }

   if( sinkDomain != NULL )
      {
      switch( sinkDomain[0] )
         {
         case 'c':   // catchment
            pFluxExpr->m_sinkDomain = FD_CATCHMENT;
            pFluxExpr->m_pSinkLayer = pModel->m_pCatchmentLayer;
            break;
      
         case 'r':   // reach or reservoir
            if ( sinkDomain[ 2 ] == 'a' )  // reach
               {
               pFluxExpr->m_sinkDomain = FD_REACH;
               pFluxExpr->m_pSinkLayer = pModel->m_pStreamLayer;
               }
            else
               {
               pFluxExpr->m_sinkDomain = FD_RESERVOIR;
               pFluxExpr->m_pSinkLayer = pModel->m_pStreamLayer;
               }
            break;
      
         default:
            ASSERT( 0 );
         }
      }

   switch( valueDomain[1] )
      {
      case 'o':   // source
         pFluxExpr->m_valueDomain = pFluxExpr->m_sourceDomain;
         pFluxExpr->m_pValueLayer = pFluxExpr->m_pSourceLayer;
         break;

      case 'i':   // sink
         pFluxExpr->m_valueDomain = FD_UNDEFINED;
         pFluxExpr->m_pValueLayer = pFluxExpr->m_pSinkLayer;
         break;

      default:
         {
         CString msg;
         msg.Format( "FluxExpr: Invalid 'value_domain' ('%s') specified when reading FluxExpr '%s'; must be 'source' or 'sink'", valueDomain, name );
         Report::ErrorMsg( msg );
         }
      }

   pFluxExpr->m_isDynamic = dynamic;

   pFluxExpr->m_sourceQuery = sourceQuery;
   pFluxExpr->m_sinkQuery   = sinkQuery;
   pFluxExpr->m_expr        = value;

   switch( valueType[ 0 ] )
      {
      case 'e':      // "expression" (default)
         pFluxExpr->m_valueType = VT_EXPR;
         break;

      case 't':      // "timeseries"
         pFluxExpr->m_valueType = VT_TIMESERIES;
         break;

      case 'f':      // "file"
         pFluxExpr->m_valueType = VT_FILE;
         break;
      }

   if ( pFluxExpr->m_pSourceLayer != NULL && pFluxExpr->m_unsatisfiedOutputField.GetLength() > 0 )
      pFluxExpr->m_pSourceLayer->CheckCol( pFluxExpr->m_colSourceUnsatisfiedDemand, pFluxExpr->m_unsatisfiedOutputField, TYPE_FLOAT, CC_AUTOADD );

   if ( withdrawalCutoffCoeff >  0 )
      pFluxExpr->m_withdrawalCutoffCoeff = withdrawalCutoffCoeff;  

   pFluxExpr->m_lossCoeff = lossCoeff;  

   int type = 0;
   if ( sourceQuery != NULL )
      type += (int) FT_SOURCE;
   if ( sinkQuery != NULL )
      type += (int) FT_SINK;

   pFluxExpr->m_fluxType = (FLUX_TYPE) type;   

   return pFluxExpr;
   }

int FluxExpr::BuildSourceSinks( void )
   {
   m_ssArray.RemoveAll();
   int sinkCount = 0;

   // are both a source and a sink defined?
   if ( m_pSourceLayer )
      {
      // this is a transfer - join source to sink
      for ( MapLayer::Iterator src=m_pSourceLayer->Begin(); src < m_pSourceLayer->End(); src++ )
         {
         int idu = src;

         if ( idu % 100 == 0 )
            {
            CString msg;
            msg.Format( "Flow Flux Builder - %i of %i sources examined", idu, m_pSourceLayer->GetRecordCount() );
            Report::StatusMsg( msg );
            }

         // find any source location that satisfies the source query
         bool pass = true;
         if ( m_pSourceQuery )
            {
            bool ok = m_pSourceQuery->Run( src, pass );
            }
      
         // found a source, look for corresponding sinks
         if ( pass )
            {
            // if no sink, just generate a straw with no sinks
            if ( m_pSinkLayer == NULL )
               {
               FluxSourceSink *pSS = new FluxSourceSink;
               pSS->m_sourceIndex = src;
               this->m_ssArray.Add( pSS );
               //m_pSourceLayer->AddSelection( src );
               }
            else  // sink layer defined - query it's join field
               {
               int srcJoinVal = -1;
               m_pSourceLayer->GetData( src, m_colJoinSrc, srcJoinVal );
      
               // have source join value, find corresonding sink join value(s)
               // note that his can be one-to-many
               CArray< int, int > sinkIndexes;
               VData val( srcJoinVal );
               int last = -1;
               FluxSourceSink *pSS = NULL;
               int sinkIndex = -1;
               while ( ( sinkIndex = m_pSinkLayer->m_pDbTable->FindData( m_colJoinSink, val, last ) ) >= 0 )
                  {
                  // does this sink pass the sink query?
                  pass = true;
                  if ( m_pSinkQuery )
                     {
                     bool ok = m_pSinkQuery->Run( sinkIndex, pass );
                     }

                  if ( pass )
                     {
                     if ( pSS == NULL )
                        {
                        pSS = new FluxSourceSink;
                        pSS->m_sourceIndex = src;
                        this->m_ssArray.Add( pSS );
                        //m_pSourceLayer->AddSelection( src );
                        }
   
                     pSS->m_sinkIndexArray.Add( sinkIndex );
                     //m_pSinkLayer->AddSelection( sinkIndex );

                     sinkCount++;
                     }

                  last = sinkIndex+1;
                  } 
               }  // end of:  if ( m_pSinkLayer != NULL ) 
            }  // end of: if ( source plygon passed source query )
         }  // end of: for each SourceLayer polygon
      }  // end of: if ( m_pSourceLayer && m_pSinkLayer )
   
   CString msg;
   msg.Format( "Flow Global Flux Builder created %i sources, %i sinks for %s", (int) m_ssArray.GetSize(), sinkCount, (LPCTSTR) this->m_name );
   Report::Log( msg );
      
   return (int) m_ssArray.GetSize();
   }