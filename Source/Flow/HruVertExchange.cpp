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
// HruVertExchange.cpp : Implements global evapotransporation methods for Flow
//

#include "stdafx.h"
#pragma hdrstop

#include "GlobalMethods.h"
#include "Flow.h"
#include <omp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern FlowProcess *gpFlow;
extern FlowModel   *gpModel;


bool HruVertExchange::Step( FlowContext *pFlowContext )
   {   
   // handle NONE, EXTERNAL cases if defined
   if ( GlobalMethod::Step( pFlowContext ) == true )
      return true;

   switch( m_method )
      {
      case GM_BROOKS_COREY:
         return SetGlobalHruVertFluxesBrooksCorey();

      case GM_NONE:
         return true;

      default:
         ASSERT( 0 );
      }

   return false;
   }


bool HruVertExchange::SetGlobalHruVertFluxesBrooksCorey( void )
   {
   int catchmentCount = gpModel->GetCatchmentCount();
   int hruLayerCount = gpModel->GetHRUPoolCount();

   // iterate through catchments/hrus/hrulayers, calling fluxes as needed
   for ( int i=0; i < catchmentCount; i++ )
      {
      Catchment *pCatchment = gpModel->GetCatchment( i );
      int hruCount = pCatchment->GetHRUCount();

      for ( int h=0; h < hruCount; h++ )
         {
         HRU *pHRU = pCatchment->GetHRU( h );

         for ( int l=0; l < hruLayerCount-1; l++ )     //All but the bottom layer
            {
            HRUPool *pHRUPool = pHRU->GetPool( l );

            float depth = 1.0f; float porosity = 0.4f;
            float voidVolume = depth*porosity*pHRU->m_area;
            float wc = float( pHRUPool->m_volumeWater/voidVolume );

            pHRUPool->m_verticalDrainage = GetVerticalDrainage(wc)*pHRU->m_area;//m3/d
            }
         }
      }

   return true;
   }


HruVertExchange *HruVertExchange::LoadXml( TiXmlElement *pXmlHruVertExchange, LPCTSTR filename )
   {
   HruVertExchange *pHruVertExch = new HruVertExchange( _T("Vertical Exchange") );  // defaults to GM_NONE

   if ( pXmlHruVertExchange == NULL )
      return pHruVertExch;

   LPTSTR name  = NULL;
   LPTSTR method  = NULL;
   LPTSTR query = NULL;   // ignored for now
   LPTSTR fn  = NULL;
   LPTSTR db  = NULL;

   XML_ATTR attrs[] = {
      // attr                 type          address                     isReq  checkCol
      { _T("name"),               TYPE_STRING,    &name,                    false,   0 },
      { _T("method"),             TYPE_STRING,    &method,                  false,   0 },
      { _T("query"),              TYPE_STRING,    &query,                   false,   0 },
      { NULL,                 TYPE_NULL,      NULL,                     false,  0 } };

   bool ok = TiXmlGetAttributes( pXmlHruVertExchange, attrs, filename );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Flow: Misformed element reading <hru_vertical_exchange> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      return NULL;
      }

   if ( name != NULL && *name != NULL && *name != _T(' ') )
      pHruVertExch->m_name = name;

   if ( method )
      {
      switch( method[ 0 ] )
         {
         case _T('b'):
         case _T('B'):
            pHruVertExch->SetMethod( GM_BROOKS_COREY  );
            break;

         case _T('N'):
         case _T('n'):
            pHruVertExch->SetMethod( GM_NONE );
            break;

         case _T('f'):
         case _T('F'):   // external
            {
            if ( strncmp( method, _T("fn"), 2 ) == 0 )
               {
               pHruVertExch->SetMethod( GM_EXTERNAL );
               // source string syntax= fn:<dllpath:function> for functional, db:<datasourcepath:columnname> for datasets
               pHruVertExch->m_extSource = method;
               FLUXSOURCE sourceLocation = ParseSource( pHruVertExch->m_extSource, pHruVertExch->m_extPath, pHruVertExch->m_extFnName,
                     pHruVertExch->m_extFnDLL, pHruVertExch->m_extFn );
               
               if ( sourceLocation != FS_FUNCTION )
                  {
                  Report::ErrorMsg( _T("Fatal Error on direct lateral exchange solution method - no solution will be performed") );
                  pHruVertExch->SetMethod( GM_NONE );
                  }
               }
            }

         default:
            {
            CString msg;
            msg.Format( _T("Flow: Invalid type '%s' specified for <hru_vertical_exchange> tag reading %s"), method, filename );
            Report::LogWarning( msg );
            pHruVertExch->SetMethod( GM_NONE );
            }
         }
      }

   return pHruVertExch;
   }
