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
#include "WaterMaster.h"
#include "AltWaterMaster.h"
#include "Flow.h"

#include <UNITCONV.H>
#include <omp.h>


WaterAllocation::~WaterAllocation( void )
   {
   if ( m_pWaterMaster ) delete m_pWaterMaster; 
   if ( m_pAltWM ) delete m_pAltWM; 
   }


bool WaterAllocation::Init( FlowContext *pFlowContext )
   {
   GlobalMethod::Init( pFlowContext );

   if ( m_pWaterMaster )
      m_pWaterMaster->Init( pFlowContext );

   if ( m_pAltWM )
      m_pAltWM->Init( pFlowContext );

   return true;
   }


bool WaterAllocation::InitRun( FlowContext *pFlowContext )
   {
   GlobalMethod::InitRun( pFlowContext );

   if ( m_pWaterMaster )
      m_pWaterMaster->InitRun( pFlowContext );

   if ( m_pAltWM )
      m_pAltWM->InitRun( pFlowContext );

   return true;
   }




bool WaterAllocation::StartYear( FlowContext *pFlowContext )
   {       
  // if ( GlobalMethod::EndYear( pFlowContext ) == true )
  //    return true;

   switch( m_method )
      {
      case GM_WATER_RIGHTS:
         {
         if ( m_pWaterMaster == NULL )
            return false;

         return m_pWaterMaster->StartYear( pFlowContext );
         }
         break;
         
      case GM_ALTWM:
         {
         if ( m_pAltWM == NULL )
            return false;

         return m_pAltWM->StartYear( pFlowContext );
         }
         break;

      //case GM_EXPR_ALLOCATOR:
      //   return RunExprAllocator( pFlowContext );

      default:
         ASSERT( 0 );
      }

   return false;
   }

bool WaterAllocation::StartStep( FlowContext *pFlowContext )
   {       
   // if ( GlobalMethod::EndYear( pFlowContext ) == true )
   //    return true;

   switch( m_method )
      {
      case GM_WATER_RIGHTS:
         {
         if ( m_pWaterMaster == NULL )
            return false;

         return m_pWaterMaster->StartStep( pFlowContext );
         }
         break;

      //case GM_EXPR_ALLOCATOR:
      //   return RunExprAllocator( pFlowContext );

      default:
         ASSERT( 0 );
      }

   return false;
   }

bool WaterAllocation::Step( FlowContext *pFlowContext )
   {       
   if ( ( pFlowContext->timing & m_timing ) == 0 )
      return true;

   if ( GlobalMethod::Step( pFlowContext ) == true )   // returns true if handled, faluse otherwise
      return true;

   switch( m_method )
      {
      case GM_WATER_RIGHTS:
         return RunWaterRights( pFlowContext );

      case GM_ALTWM:
         return RunAltWM( pFlowContext );

      case GM_EXPR_ALLOCATOR:
         return RunExprAllocator( pFlowContext );

      default:
         ASSERT( 0 );
      }

   return false;
   }


bool WaterAllocation::EndStep( FlowContext *pFlowContext )
   {       
   // if ( GlobalMethod::EndYear( pFlowContext ) == true )
   //    return true;

   switch( m_method )
      {
      case GM_WATER_RIGHTS:
         {
         if ( m_pWaterMaster == NULL )
            return false;

         return m_pWaterMaster->EndStep( pFlowContext );
         }
         break;

      case GM_ALTWM:
         {
         if ( m_pAltWM == NULL )
            return false;

         return m_pAltWM->EndStep( pFlowContext );
         }
         break;

      //case GM_EXPR_ALLOCATOR:
      //   return RunExprAllocator( pFlowContext );

      default:
         ASSERT( 0 );
      }

   return false;
   }


bool WaterAllocation::EndYear( FlowContext *pFlowContext )
   {       
  // if ( GlobalMethod::EndYear( pFlowContext ) == true )
  //    return true;

   switch( m_method )
      {
      case GM_WATER_RIGHTS:
         {
         if ( m_pWaterMaster == NULL )
            return false;

         return m_pWaterMaster->EndYear( pFlowContext );
         }
         break;
         
      case GM_ALTWM:
         {
         if ( m_pAltWM == NULL )
            return false;

         return m_pAltWM->EndYear( pFlowContext );
         }
         break;


      //case GM_EXPR_ALLOCATOR:
      //   return RunExprAllocator( pFlowContext );

      default:
         ASSERT( 0 );
      }

   return false;
   }

bool WaterAllocation::RunWaterRights( FlowContext *pFlowContext )
   {
   if ( m_pWaterMaster == NULL )
      return false;

   m_pWaterMaster->Step( pFlowContext );
   return true;
   }


bool WaterAllocation::RunAltWM( FlowContext *pFlowContext )
   {
   if ( m_pAltWM == NULL )
      return false;

   m_pAltWM->Step( pFlowContext );
   return true;
   }


bool WaterAllocation::RunExprAllocator( FlowContext* )
   {
   return true;
   }


// Note: this method is static
WaterAllocation *WaterAllocation::LoadXml( TiXmlElement *pXmlWaterAllocation, LPCTSTR filename, FlowModel *pFlowModel)
   {
   CString method;
   bool ok = ::TiXmlGetAttr( pXmlWaterAllocation, _T("method"), method, _T(""), true );

   if ( ! ok )
      return NULL;

   WaterAllocation *pMethod = new WaterAllocation(pFlowModel, _T("Allocation"), GM_NONE );

   ASSERT( method.IsEmpty() == false );

   switch( method[ 0 ] )
      {
      case _T('W'):
      case _T('w'):   // water_rights
         {
         pMethod->SetMethod( GM_WATER_RIGHTS );
         WaterMaster *pWM = new WaterMaster( pFlowModel, pMethod );
         pWM->LoadXml( pMethod, pXmlWaterAllocation, filename );
         pMethod->m_pWaterMaster = pWM;
         }
         break;

      case _T('A'):
      case _T('a'):   // alt water_rights
         {
         pMethod->SetMethod( GM_ALTWM );
         AltWaterMaster *pAWM = new AltWaterMaster(pFlowModel, pMethod );
         pAWM->LoadXml( pMethod, pXmlWaterAllocation, filename );
         pMethod->m_pAltWM = pAWM;
         }
         break;

      case _T('E'):
      case _T('e'):   // expression
         pMethod->SetMethod( GM_EXPR_ALLOCATOR );
         break;         
      
      case _T('F'):
      case _T('f'):   // external function
         {
         pMethod->SetMethod( GM_EXTERNAL );

         // source string syntax= fn:<dllpath:function> for functional, db:<datasourcepath:columnname> for datasets
         pMethod->m_extSource = method;
         FLUXSOURCE sourceLocation = ParseSource( pMethod->m_extSource, pMethod->m_extPath, pMethod->m_extFnName,
               pMethod->m_extFnDLL, pMethod->m_extFn );
         
         if ( sourceLocation != FS_FUNCTION )
            {
            Report::ErrorMsg( _T("Fatal Error parsing Water Allocation External method - no solution will be performed") );
            pMethod->SetMethod( GM_NONE );
            }
         }
         break;

      default:
         pMethod->SetMethod( GM_NONE );
      }
   
   return pMethod;
   }
