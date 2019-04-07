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
#include "stdafx.h"

#pragma hdrstop

#include "GlobalMethods.h"
#include "Flow.h"

#include <UNITCONV.H>
#include <omp.h>

extern FlowProcess *gpFlow;
extern FlowModel *gpModel;



bool ReachRouting::Step( FlowContext *pFlowContext )
   {         
      //if ( m_reachSolutionMethod < RSM_EXTERNAL )
      //   m_reachBlock.Integrate( m_currentTime, m_currentTime+stepSize, GetReachDerivatives, &m_flowContext );  // NOTE: GetReachDerivatives does not work
      //else
      //   SolveReachDirect();

   // handle NONE, EXTERNAL cases if defined
   if ( GlobalMethod::Step( pFlowContext ) == true )    // true means GlobalMethod handled it.
      return true;

   switch( m_method )
      {
      case GM_EULER:
      case GM_RK4:
      case GM_RKF:
         gpModel->m_reachBlock.Integrate( gpModel->m_currentTime, gpModel->m_currentTime+this->m_reachTimeStep, GetReachDerivatives, pFlowContext );  // NOTE: GetReachDerivatives does not work
         return true;

      case GM_KINEMATIC:
         return SolveReachKinematicWave( pFlowContext );

      case GM_2KW:
         return SolveReach2KW(pFlowContext);

      case GM_NONE:
         return true;

      default:
         ASSERT( 0 );
      }

   return false;
   }

bool ReachRouting::SolveReach2KW(FlowContext *pFlowContext)
   {
   pFlowContext->Reset();

   int reachCount = gpModel->GetReachCount();

   clock_t start = clock();

   // basic idea - for each Reach, estimate it's outflow bases on lateral flows and any fluxes
   for (int i = 0; i < reachCount; i++)
      {
      Reach *pReach = gpModel->GetReach(i);     // Note: these are guaranteed to be non-phantom

      ReachNode *pN = (ReachNode*)pReach;

      //for (int l = 0; l < pReach->GetSubnodeCount(); l++)
     //    {
         pFlowContext->pReach = pReach;
         ReachSubnode *pNode = pReach->GetReachSubnode(0);
         float lateralInflow = GetLateralInflow(pReach)/SEC_PER_DAY*pReach->GetSubnodeCount();//m3/s
        // double dt = pFlowContext->timeStep* SEC_PER_DAY;
         //float lateral = float(lateralInflow / dt);//m3/d
         //float externalFluxes = GetReachFluxes(pFlowContext, pReach);
         float Qin = GetReachInflow(pReach, 0);//m3/s
         
        // float outflow = EstimateReachOutflow(pReach, l, pFlowContext->timeStep, lateralInflow + externalFluxes);
         pNode->m_discharge = lateralInflow + Qin;  //convert units to m3/s;  

         
         for (int k = 0; k < pFlowContext->svCount; k++)
            {
            float svIn = GetLateralSVInflow(pReach, k);//kg/ms from this reach and upstream neighbors

            pNode->SetStateVar(k, svIn );
            }
     //    }
      }

   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;
   gpModel->m_reachFluxFnRunTime += (float)duration;

   return true;
}

bool ReachRouting::SolveReachKinematicWave( FlowContext *pFlowContext )
   {
   pFlowContext->Reset();

   int reachCount = gpModel->GetReachCount();

   clock_t start = clock();

   // basic idea - for each Reach, estimate it's outflow bases on lateral flows and any fluxes
   for ( int i=0; i < reachCount; i++ )
      {
      Reach *pReach = gpModel->GetReach( i );     // Note: these are guaranteed to be non-phantom
    
      ReachNode *pN = (ReachNode*) pReach;

      for ( int l=0; l < pReach->GetSubnodeCount(); l++ )
         {
         pFlowContext->pReach = pReach;
         ReachSubnode *pNode = pReach->GetReachSubnode( l );
         float lateralInflow = GetLateralInflow( pReach );//m3/d

         //float externalFluxes = GetReachFluxes( pFlowContext, pReach ); 
            
         float outflow = EstimateReachOutflow(pReach, l, pFlowContext->timeStep, lateralInflow); // +externalFluxes );
         pNode->m_discharge = outflow/SEC_PER_DAY;  //convert units to m3/s;  

         for (int k=0; k < pFlowContext->svCount;k++)
            {
            float svIn = GetLateralSVInflow( pReach , k);//kg/d from this reach and upstream neighbors
            float previousConc = (float) pNode->GetStateVar(k);
            pNode->SetStateVar(k, (svIn+previousConc)/2.0f);
            }
         }
      }

   clock_t finish = clock();
   double duration = (float)(finish - start) / CLOCKS_PER_SEC;   
   gpModel->m_reachFluxFnRunTime += (float) duration;   

   return true;
   }


float ReachRouting::GetLateralInflow( Reach *pReach )
   {
   float inflow = 0;
   inflow = pReach->GetFluxValue();
   inflow /= pReach->GetSubnodeCount();  // m3/day

   for (int i = 0; i < pReach->GetSubnodeCount(); i++)
      {
      ReachSubnode *pSubnode = pReach->GetReachSubnode(i);
      pSubnode->m_lateralInflow = inflow;
      }

   return inflow;
   }

//float ReachRouting::GetReachFluxes( FlowContext *pFlowContext, Reach *pReach )
//   {
//   float totalFlux = 0;
//
//   pFlowContext->pReach = pReach;
//
//   for ( int i=0; i < pReach->GetFluxCount(); i++ )
//      {
//      Flux *pFlux = pReach->GetFlux( i );
//      ASSERT( pFlux != NULL );
//
//      pFlowContext->pFlux = pFlux;
//      pFlowContext->pFluxInfo = pFlux->m_pFluxInfo;
//      
//      float flux = pFlux->Evaluate( pFlowContext );
//      
//      // Note: this is a rate, applied over a time step
//
//      // figure out which state var this flux is associated with
//      //int sv = pFlux->m_pStateVar->m_index;
//
//      // source or sink?  If sink, flip sign  
//      if ( pFlux->IsSource() )
//         totalFlux += flux;
//      else
//         totalFlux -= flux;
//      }
//
//   return totalFlux;
//   }


// FlowModel::EstimateOutflow() ------------------------------------------
//
// solves the KW equations for the Reach downstream from pNode
//------------------------------------------------------------------------

float ReachRouting::EstimateReachOutflow( Reach *pReach, int subnode, double timeStep, float lateralInflow)
   {
   float beta  = 3.0f/5.0f;
   // compute Qin for this reach
   float dx = pReach->m_deltaX;
   double dt = timeStep* SEC_PER_DAY;//seconds
   ///float lateral = float( lateralInflow/timeStep );//m3/d
   float lateral = float(lateralInflow );//m3/d
   float qsurface = float( lateral/dx*timeStep );  //m2

   float Qnew = 0.0f;      
   float Qin = 0.0f;

   ReachSubnode *pSubnode = pReach->GetReachSubnode( subnode );
   ASSERT( pSubnode != NULL );

   float Q = pSubnode->m_discharge;
   SetGeometry(pReach, Q);
   float width = pReach->m_width;
   float depth = pReach->m_depth;
 //  pSubnode->m_volume = width*depth*pReach->m_length / pReach->m_subnodeArray.GetSize();

   //pHydro->waterVolumeArrayPrevious[i]=width*depth*length;
   float n = pReach->m_n;
   float slope = pReach->m_slope;
   if (slope < 0.02f)
      slope = 0.02f;
   float wp = width + depth + depth;

   float alph =  n * (float)pow( (long double) wp, (long double) (2/3.)) / (float)sqrt( slope );

   float alpha = (float) pow((long double) alph, (long double) 0.6);
      // Qin is the value upstream at the current time
      // Q is the value at the current location at the previous time

   Qin = GetReachInflow( pReach, subnode );

   float Qstar = ( Q + Qin  ) / 2.0f;   // from Chow, eqn. 9.6.4

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

   if (Qnew<0.00001f) //this condition may occur if we are withdrawing water from the reach.  This solution impacts the mass balance, but only slightly because the flows are near 0 anyway.
      Qnew=0.00001f;

   SetGeometry(  pReach, Qnew);
   width = pReach->m_width;
   depth = pReach->m_depth;

   pSubnode->m_volume=width*depth*pReach->m_length/pReach->m_subnodeArray.GetSize(); 

   pSubnode->m_previousVolume = pSubnode->m_volume;
 //  pSubnode->m_volume += ( Qin - Qnew ) * dt + lateral *  timeStep;


 //  pSubnode->m_volume = width*depth*pReach->m_length / pReach->m_subnodeArray.GetSize();

//   ASSERT(pSubnode->m_volume > -1);

   return Qnew*SEC_PER_DAY;
   }


float ReachRouting::GetReachInflow( Reach *pReach, int subNode )
   {
   float Q = 0;

   ///////
   if ( subNode == 0 )  // look upstream?
      {
      Reservoir *pRes = pReach->m_pReservoir;

      if ( pRes == NULL )
         {
         Q = GetReachOutflow( pReach->m_pLeft );
         Q += GetReachOutflow( pReach->m_pRight );
         }
      else
         //Q = pRes->GetResOutflow( pRes, m_flowContext.dayOfYear );
       Q = pRes->m_outflow/SEC_PER_DAY;  //m3/day to m3/s
      }
   else
      {
      ReachSubnode *pNode = (ReachSubnode*) pReach->m_subnodeArray[ subNode-1 ];  ///->GetReachSubnode( subNode-1 );
      ASSERT( pNode != NULL );
      Q = pNode->m_discharge;
      }

   return Q;
   }


float ReachRouting::GetReachOutflow( ReachNode *pReachNode )   // recursive!!! for pahntom nodes
   {
   if ( pReachNode == NULL )
      return 0;

   if ( pReachNode->IsPhantomNode() )
      {
      float q = GetReachOutflow( pReachNode->m_pLeft );
      q += GetReachOutflow( pReachNode->m_pRight );

      return q;
      }
   else
      {
      Reach *pReach = gpModel->GetReachFromNode( pReachNode );

      if ( pReach != NULL )
         return pReach->GetDischarge();
      else
         {
         ASSERT( 0 );
         return 0;
         }
      }
   }


float ReachRouting::GetLateralSVInflow(Reach *pReach, int sv)
{
   float inflow = 0;
   ReachSubnode *pNode = pReach->GetReachSubnode(0);
   inflow = pNode->GetExtraSvFluxValue(sv);
  // inflow /*/= pReach->GetSubnodeCount()*/;  // kg/d

   float lateralInflow = GetLateralInflow(pReach)*pReach->m_subnodeArray.GetSize();
  // float lateralConc = inflow/lateralInflow;

   if (pReach->m_pLeft)
      { 
      ReachSubnode *pNode1 = (ReachSubnode*)pReach->m_pLeft->m_subnodeArray[0];
      inflow+=pNode1->GetExtraSvFluxValue(sv)/*pReach->m_pLeft->GetSubnodeCount();//kg/d*/;
      lateralInflow += GetLateralInflow(gpModel->GetReachFromNode(pReach->m_pLeft))*pReach->m_subnodeArray.GetSize();//m3/d
      }
   if (pReach->m_pRight)
      { 
      ReachSubnode *pNode2 = (ReachSubnode*)pReach->m_pRight->m_subnodeArray[0];
      inflow += pNode2->GetExtraSvFluxValue(sv) /* *pReach->m_pRight->GetSubnodeCount();//kg/d*/;
      lateralInflow += GetLateralInflow(gpModel->GetReachFromNode(pReach->m_pRight))*pReach->m_subnodeArray.GetSize();//m3/d
      }

   float conc = 0;
   if (lateralInflow > 0.0f)
     conc = inflow / lateralInflow;

   return conc ;//kg/m3.  Not this is a concentration, not a mass!
}

/*

float ReachRouting::GetLateralSVInflow( Reach *pReach, int sv )
   {
   float inflow = 0;
   ReachSubnode *pNode = pReach->GetReachSubnode( 0 );
   inflow=pNode->GetExtraSvFluxValue(sv);
   inflow /= pReach->GetSubnodeCount();  // kg/d

   return inflow;
   }
   */

float ReachRouting::GetReachSVOutflow( ReachNode *pReachNode, int sv )   // recursive!!! for pahntom nodes
   {
   if ( pReachNode == NULL )
      return 0;

   if ( pReachNode->IsPhantomNode() )
      {
      float flux = GetReachSVOutflow( pReachNode->m_pLeft, sv );
      flux += GetReachSVOutflow( pReachNode->m_pRight, sv );

      return flux;
      }
   else
      {
      Reach *pReach = gpModel->GetReachFromNode( pReachNode );
      ReachSubnode *pNode = (ReachSubnode*) pReach->m_subnodeArray[ 0 ]; 

      if ( pReach != NULL && pNode->m_volume>0.0f)
         //return float( pNode->GetStateVar( sv ) / pNode->m_volume*pNode->m_discharge*86400.0f );//kg/d 
         return GetLateralSVInflow(pReach,  sv);
      else
         {
         //ASSERT( 0 );
         return 0;
         }
      }
   }

float ReachRouting::GetReachSVInflow( Reach *pReach, int subNode, int sv )
   {
    float flux=0.0f;
    float Q=0.0f;

   if ( subNode == 0 )  // look upstream?
      {
      Reservoir *pRes = pReach->m_pReservoir;

      if ( pRes == NULL )
         {
         flux = GetReachSVOutflow( pReach->m_pLeft, sv );
         flux += GetReachSVOutflow( pReach->m_pRight, sv );
         }
      else
         flux = pRes->m_outflow/SEC_PER_DAY;  //m3/day to m3/s
      }
   else
      {
      ReachSubnode *pNode = (ReachSubnode*) pReach->m_subnodeArray[ subNode-1 ];  ///->GetReachSubnode( subNode-1 );
      ASSERT( pNode != NULL );
     
      if ( pNode->m_svArray != NULL)
         {
         flux = GetReachSVOutflow(pReach->m_pLeft, sv);
         flux += GetReachSVOutflow(pReach->m_pRight, sv);
         }
        // flux  = float( pNode->m_svArray[sv]/pNode->m_volume*  pNode->m_discharge*86400.0f );//kg/d 
      }

   return flux; //kg/d;
   }
   

void ReachRouting::GetReachDerivatives( double time, double timeStep, int svCount, double *derivatives /*out*/, void *extra )
   {
   FlowContext *pFlowContext = (FlowContext*) extra;

   FlowContext flowContext( *pFlowContext );   // make a copy for openMP

   // NOTE: NEED TO INITIALIZE TIME IN FlowContext before calling here...
   // NOTE:  Should probably do a breadth-first search of the tree(s) rather than what is here.  This could be accomplished by
   //  sorting the m_reachArray in InitReaches()

   FlowModel *pModel = pFlowContext->pFlowModel;
   flowContext.timeStep = (float) timeStep;
   
   // compute derivative values for reachs based on any associated subnode fluxes
   int reachCount = (int) pModel->m_reachArray.GetSize();

   omp_set_num_threads( gpFlow->m_processorsUsed );
   #pragma omp parallel for firstprivate( flowContext )
   for ( int i=0; i < reachCount; i++ )
      {
      Reach *pReach = pModel->m_reachArray[ i ];
      flowContext.pReach = pReach;

      for ( int l=0; l < pReach->GetSubnodeCount(); l++ )
         {
         ReachSubnode *pNode = pReach->GetReachSubnode( l );
         int svIndex = pNode->m_svIndex;

         flowContext.reachSubnodeIndex = l;
         derivatives[svIndex]=0;

         // add all fluxes for this reach
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
         }
      }
   }


ReachRouting *ReachRouting::LoadXml( TiXmlElement *pXmlReachRouting, LPCTSTR filename )
   {
   ReachRouting *pRouting = new ReachRouting( _T("Reach Routing") );  // defaults to GM_KINEMATIC

   LPTSTR method = NULL;
   LPTSTR query = NULL;   // ignored for now
   LPTSTR fn = NULL;
   LPTSTR db = NULL;

   XML_ATTR attrs[] = {
      // attr                 type          address                     isReq  checkCol
      { _T("name"),               TYPE_CSTRING,   &(pRouting->m_name),      false,   0 },
      { _T("method"),             TYPE_STRING,    &method,                  false,   0 },
      { _T("query"),              TYPE_STRING,    &query,                   false,   0 },
      { NULL,                 TYPE_NULL,     NULL,                      false,  0 } };

   bool ok = TiXmlGetAttributes( pXmlReachRouting, attrs, filename );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Flow: Misformed element reading <reach_routing> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      }

   if ( method )
      {
      if ( lstrcmpi( method, _T("kinematic") ) == 0 )
         pRouting->SetMethod(  GM_KINEMATIC );
      else if ( lstrcmpi( method, _T("euler") ) == 0 )
         pRouting->SetMethod(  GM_EULER );
      else if ( lstrcmpi( method, _T("rkf") ) == 0 )
         pRouting->SetMethod(  GM_RKF );
      else if ( lstrcmpi( method, _T("rk4") ) == 0 )
         pRouting->SetMethod(  GM_RK4 );
      else if ( lstrcmpi( method, _T("none") ) == 0 )
         pRouting->SetMethod(  GM_NONE );
      else if (lstrcmpi(method, _T("2kw")) == 0)
         pRouting->SetMethod(GM_2KW);
      else if ( strncmp( method, _T("fn"), 2 ) == 0 )
         {
         pRouting->SetMethod(  GM_EXTERNAL );
         // source string syntax= fn:<dllpath:function> for functional, db:<datasourcepath:columnname> for datasets
         pRouting->m_extSource = method;
         FLUXSOURCE sourceLocation = ParseSource( pRouting->m_extSource, pRouting->m_extPath, pRouting->m_extFnName,
               pRouting->m_extFnDLL, pRouting->m_extFn );
         
         if ( sourceLocation != FS_FUNCTION )
            {
            Report::ErrorMsg( _T("Fatal Error on direct reach solution method - no solution will be performed") );
            pRouting->SetMethod( GM_NONE );
            }
         }
      }

   return pRouting;
   }
   