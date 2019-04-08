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
// ExternalMethods.cpp : Implements global external methods interface
//

#include "stdafx.h"
#pragma hdrstop

#include "Flow.h"
#include "GlobalMethods.h"

//#include <EnvConstants.h>
#include <PathManager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



bool ExternalMethod::Init( FlowContext *pFlowContext )
   {
   if ( ( m_timing & GMT_INIT ) == 0 )
      return false;

   if ( GlobalMethod::Init( pFlowContext ) == false ) 
      return false; 

   pFlowContext->initInfo = this->m_initInfo;
   
   bool ok = Step( pFlowContext ); // execute external method

   pFlowContext->initInfo = NULL;

   return ok;
   }


bool ExternalMethod::InitRun( FlowContext *pFlowContext )
   {
   if ( ( m_timing & GMT_INITRUN ) == 0 )
      return false;

   if ( GlobalMethod::InitRun( pFlowContext ) == false ) 
      return false; 
   
   return Step( pFlowContext ); // execute external method
   }
   
   
bool ExternalMethod::StartYear( FlowContext *pFlowContext )      // start of Flow::Run() invocation
   {
   if ( ( m_timing & GMT_START_YEAR ) == 0 )
      return false;

   if ( GlobalMethod::StartYear( pFlowContext ) == false ) 
      return false; 
   
   return Step( pFlowContext ); // execute external method
   }


bool ExternalMethod::StartStep( FlowContext *pFlowContext )      // start of Flow::Run() invocation
   {
   if ( ( m_timing & GMT_START_STEP ) == 0 )
      return false;

   if ( GlobalMethod::StartStep( pFlowContext ) == false ) 
      return false; 
   
   return Step( pFlowContext ); // execute external method
   }


bool ExternalMethod::EndStep( FlowContext *pFlowContext )      // start of Flow::Run() invocation
   {
   if ( ( m_timing & GMT_END_STEP ) == 0 )
      return false;

   if ( GlobalMethod::EndStep( pFlowContext ) == false ) 
      return false; 
   
   return Step( pFlowContext ); // execute external method
   }

bool ExternalMethod::EndYear( FlowContext *pFlowContext )      // end of a Flow::Run() invocation
   {
   if ( ( m_timing & GMT_END_YEAR ) == 0 )
      return false;

   if ( GlobalMethod::EndYear( pFlowContext ) == false ) 
      return false; 
   
   return Step( pFlowContext ); // execute external method
   }


bool ExternalMethod::EndRun ( FlowContext *pFlowContext )      // end of an Envision run
   {
    if ( ( m_timing & GMT_ENDRUN ) == 0 )
      return false;

   if ( GlobalMethod::EndRun( pFlowContext ) == false ) 
      return false; 
   
   return Step( pFlowContext ); // execute external method
   }



ExternalMethod *ExternalMethod::LoadXml( TiXmlElement *pXmlMethod, LPCTSTR filename )
   {
   if ( pXmlMethod == NULL )
      return NULL;

   LPTSTR name=NULL, path=NULL, method=NULL, timing=NULL;
   bool use = true;
   
   ExternalMethod *pMethod = new ExternalMethod( _T("External Method") );
   
   XML_ATTR methodAttrs[] = {
      // attr                 type          address                     isReq  checkCol
      { _T("name"),               TYPE_STRING,   &name,                     true,   0 },
      { _T("method"),             TYPE_STRING,   &method,                   true,   0 },
      { _T("timing"),             TYPE_STRING,   &timing,                   false,  0 },
      { _T("use"),                TYPE_BOOL,     &use,                      false,  0 },
      { _T("initInfo"),           TYPE_CSTRING,  &(pMethod->m_initInfo),    false,  0 },
      { NULL,                 TYPE_NULL,     NULL,                      false,  0 } };
   
   bool ok = TiXmlGetAttributes( pXmlMethod, methodAttrs, filename );
   if ( ! ok )
      {
      CString msg; 
      msg.Format( _T("Flow: Misformed element reading <external_method> attributes in input file %s"), filename );
      Report::ErrorMsg( msg );
      delete pMethod;
      return NULL;
      }

   pMethod->m_name = name;
   pMethod->SetMethod( GM_EXTERNAL );
   pMethod->m_use = use;

   // parse timing
   int _timing = 0xFFFF;   // assume everything by default
   if ( timing != NULL )
      {
      _timing = 0;   // if specified, assume nothing by default

      TCHAR *buffer = new TCHAR[ lstrlen( timing ) + 1 ];
      lstrcpy( buffer, timing );
      TCHAR *next = NULL;
      TCHAR *token = _tcstok_s( buffer, _T(",+|"), &next );
   
      while ( token != NULL )
         {
         int t = atoi( token );
         _timing += t;

         token = _tcstok_s( NULL, _T( ",+|" ), &next );
         }

      delete [] buffer;
      }

   pMethod->m_timing = _timing;

   // source string syntax= fn:<dllpath:function> for functional, db:<datasourcepath:columnname> for datasets
   pMethod->m_extSource = method;
   FLUXSOURCE sourceLocation = ParseSource( pMethod->m_extSource, pMethod->m_extPath, pMethod->m_extFnName,
         pMethod->m_extFnDLL, pMethod->m_extFn );
   
   if ( sourceLocation != FS_FUNCTION )
      {
      CString msg;
      msg.Format( _T("Error parsing <external> '%s' specification for '%s'.  This method will not be invoked."), method, name );
      Report::ErrorMsg( msg );
      pMethod->SetMethod( GM_NONE );
      pMethod->m_use = false;
      }

   return pMethod;
   }
