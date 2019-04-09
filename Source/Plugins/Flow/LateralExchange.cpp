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
// LateralExchanges.cpp : Implements global evapotransporation methods for Flow
//

#include "stdafx.h"
#pragma hdrstop

#include "GlobalMethods.h"
#include "Flow.h"
#include <omp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool LateralExchange::Step( FlowContext *pFlowContext )
   {
   // handle NONE, EXTERNAL cases if defined
   if ( GlobalMethod::Step( pFlowContext ) == true )
      return true;

   switch( m_method )
      {
      case GM_LINEAR_RESERVOIR:
         return SetGlobalHruToReachExchangesLinearRes();

      case GM_NONE:
         return true;

      default:
         ASSERT( 0 );
      }

   return false;
   }


bool LateralExchange::SetGlobalHruToReachExchangesLinearRes( void )
   {
   int catchmentCount = m_pFlowModel->GetCatchmentCount(); // (int) m_catchmentArray.GetSize();
   int hruLayerCount = m_pFlowModel->GetHRUPoolCount();

   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = m_pFlowModel->GetCatchment( i ); //m_catchmentArray[ i ];
      ASSERT( pCatchment != NULL );

      int hruCount = pCatchment->GetHRUCount();
      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );
         
         int l = hruLayerCount-1;      // bottom layer only
         HRUPool *pHRUPool = pHRU->GetPool( l );
         
         float depth = 1.0f; 
         float porosity = 0.4f;
         float voidVolume = depth*porosity*pHRU->m_area;
         float wc = float( pHRUPool->m_volumeWater/voidVolume );
 
         float waterDepth = wc*depth;
         float baseflow = GetBaseflow(waterDepth)*pHRU->m_area;//m3/d

         pHRUPool->m_contributionToReach = baseflow;
         pCatchment->m_contributionToReach += baseflow;
         }
      }

   return true;
   }


LateralExchange *LateralExchange::LoadXml( TiXmlElement *pXmlLateralExchange, LPCTSTR filename, FlowModel *pFlowModel)
   {
   LateralExchange *pLatExch = new LateralExchange(pFlowModel, _T("Lateral Exchange") );  // defaults to GMT_NONE

   if ( pXmlLateralExchange == NULL )
      return pLatExch;

   LPTSTR name  = NULL;
   LPTSTR method  = NULL;
   LPTSTR query = NULL;   // ignored for now

   XML_ATTR attrs[] = {
      // attr                 type          address                   isReq  checkCol
      { _T("name"),               TYPE_STRING,  &name,                    false,   0 },
      { _T("method"),             TYPE_STRING,  &method,                  false,   0 },
      { _T("query"),              TYPE_STRING,  &query,                   false,   0 },  // deprecated
      { NULL,                 TYPE_NULL,    NULL,                     false,  0 } };

   bool ok = TiXmlGetAttributes( pXmlLateralExchange, attrs, filename );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Flow: Misformed element reading <lateral_exchange> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      return NULL;
      }

   if ( name != NULL && *name != NULL && *name != _T(' ') )
      pLatExch->m_name = name;

   if ( method )
      {
      switch( method[ 0 ] )
         {
         case _T('l'):
         case _T('L'):
            pLatExch->SetMethod( GM_LINEAR_RESERVOIR  );
            break;

         case _T('N'):
         case _T('n'):
            pLatExch->SetMethod( GM_NONE );
            break;

         case _T('f'):
         case _T('F'):
            if ( strncmp( method, _T("fn"), 2 ) == 0 )
              {
              pLatExch->m_method = GM_EXTERNAL;
              // source string syntax= fn:<dllpath:function> for functional, db:<datasourcepath:columnname> for datasets
              pLatExch->m_extSource = method;
              FLUXSOURCE sourceLocation = ParseSource( pLatExch->m_extSource, pLatExch->m_extPath, pLatExch->m_extFnName,
                    pLatExch->m_extFnDLL, pLatExch->m_extFn );
              
              if ( sourceLocation != FS_FUNCTION )
                 {
                 Report::ErrorMsg( _T("Fatal Error on direct lateral exchange solution method - no solution will be performed") );
                 pLatExch->SetMethod( GM_NONE );
                 }
              }
            break;         

         default:
            {
            CString msg;
            msg.Format( _T("Flow: Invalid method '%s' specified for <lateral_exchange> tag reading %s"), method, filename );
            Report::LogWarning( msg );
            }
         }
      }

   return pLatExch;
   }
