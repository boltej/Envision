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
// Economy.cpp : Defines the initialization routines for the DLL.

#include "stdafx.h"
#include "Economy.h"
#include <randgen\Randunif.hpp>
#include <misc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern "C" _EXPORT EnvExtension* Factory(EnvContext *pContext)
   {
   return (EnvExtension*) new Economy;
   }


#define FailAndReturn(FailMsg) \
   {msg = failureID + (_T(FailMsg)); \
      Report::ErrorMsg(msg); \
      return false; }

Economy::Economy( void )
   : EnvModelProcess()
   , m_pIDULayer( NULL )
   , m_pExpr( NULL )
   , m_pMapExprEngine( NULL )
   //, m_pPolicy( NULL )
   , m_colUtilityID( -1 )
   , m_rndUnif( 0.0, 1.0, 0 )
   {
   failureID = (_T("Error: "));
   }

Economy::~Economy( void )
   {
   delete m_pMapExprEngine;
   }


bool Economy::Init( EnvContext *pContext, LPCTSTR initStr)
   {
   ASSERT( pContext != NULL );
   ASSERT( initStr != NULL );

   // get IDU map layer
   m_pIDULayer = (MapLayer *) pContext->pMapLayer;
   m_pMapExprEngine = new MapExprEngine( m_pIDULayer, pContext->pQueryEngine );

   bool ok = LoadXml( initStr );
   if( ! ok )
      FailAndReturn("LoadXml() returned false in Economy.cpp Init");

   this->CheckCol( pContext->pMapLayer, m_colUtilityID, m_utilityIDCol, TYPE_INT, CC_AUTOADD );   
   
   m_values.SetSize( m_pMapExprEngine->GetExprCount() );

   // convert all equation strings to evaluable equations
   int sizeExprArrayUtilities = m_pMapExprEngine->CompileAll();

   // evaluate equations, choose one, and write corresponding policy ID to IDU column
   ok = EvaluateUtilities( pContext, false );
   if( ! ok )
      FailAndReturn("EvaluateUtilities() returned false in Economy.cpp Init");
  
   return TRUE;
   }


bool Economy::Run( EnvContext *pContext )
   {
   ASSERT( pContext != NULL );

   // convert all equation strings to evaluable equations
   int sizeExprArrayUtilities = m_pMapExprEngine->CompileAll();
   // calculate equations, choose max result and write corresponding policy ID to IDU column
   bool evu = EvaluateUtilities( pContext, true );
   if( ! evu )
      FailAndReturn( "EvaluateUtilities() returned false in Economy.cpp Run" );

   return TRUE;
   }


bool Economy::LoadXml( LPCTSTR filename )
   {
   TRACE (_T( "Starting Economy.cpp LoadXmlUtilities" ) );
   ASSERT( filename != NULL );

    // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );
   if ( ! ok )
      {      
      Report::ErrorMsg( doc.ErrorDesc() );
      return false;
      }
 
   // start interating through the activity nodes
   TiXmlElement *pXmlRoot = doc.RootElement();
   m_mode = pXmlRoot->Attribute( "mode" );
   m_utilityIDCol = pXmlRoot->Attribute( "col" );

   TiXmlElement *pXmlActivity = pXmlRoot->FirstChildElement( "utility" );
   if ( pXmlActivity == NULL )
      {
      CString msg;
      msg.Format( _T("Error reading Economy input file %s: Missing <utility> entry.  This is required to continue..."), filename );
      Report::ErrorMsg( msg );
      return false;
      }

   while ( pXmlActivity != NULL )
      {
      LPTSTR name = NULL;
      LPTSTR expr  = NULL;
      LPTSTR query = NULL;
      int id = -1;
      XML_ATTR attrs[] = {
         // attr                       type           address     isReq       checkCol
         { "name",                     TYPE_STRING,   &name,      false,      0 },
         { "policy_id",                TYPE_INT,      &id,        false,      0 },   // deprecated
         { "id",                       TYPE_INT,      &id,        false,      0 },
         { "query",                    TYPE_STRING,   &query,     true,       0 },
         { "value",                    TYPE_STRING,   &expr,      true,       0 },
         { NULL,                       TYPE_NULL,     NULL,       false,      0 }
         };

      // get contents of xml file (filename) for nodes (actiity) into array (attrs)
      bool ok = TiXmlGetAttributes( pXmlActivity, attrs, filename );
      if ( ok )
         {
         if ( id < 0 )
            {
            CString msg;
            msg.Format( "Economy: utility function '%s' is missing its 'id' attribute - this is a required attribute.  This utility function will be ignored", name );
            Report::LogError( msg );
            }
         else
            {
            // add new expression, including name, equation, spatial query and policy ID
            m_pExpr = m_pMapExprEngine->AddExpr( name, expr, query );
            m_pExpr->m_extra = id;
            }
         }
         
      pXmlActivity = pXmlActivity->NextSiblingElement( "utility" );
      }

   return true;
   }


bool Economy::EvaluateUtilities( EnvContext *pContext, bool useAddDelta )
   {
   ASSERT(pContext != NULL);
   int noDataValue = (int) m_pIDULayer->GetNoDataValue();

   // iterate trought IDU table
   for( MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++ )
      {
      int scr = m_pMapExprEngine->SetCurrentRecord( idu );
      if ( scr < 0 )
         FailAndReturn("No record set in Economy.cpp EvaluateUtilities");

      // evaluate equations derived from xml file for current idu (if spatial query returns true)
      m_pMapExprEngine->EvaluateAll( true );
      
      // get expression that returns max result
      int index = -1;
            
      if( m_mode == "DET" )
         index = EvaluateUtilitiesDeterministic( m_pMapExprEngine );
      else if( m_mode == "STO" )
         index = EvaluateUtilitiesStochastic( m_pMapExprEngine );
      else
         FailAndReturn("Economy evaluation mode not specified");

      // get policy id that corresponds to the expression that returns max result and write policy id to column in IDU table
      int oldID = -1;
      m_pIDULayer->GetData( idu, m_colUtilityID, oldID );
      
      if ( index >= 0 ) // was a utility function selected?
         {
         m_pExpr = m_pMapExprEngine->GetExpr( index );

         int newID = (int) m_pExpr->m_extra;

         if ( oldID != newID )
            UpdateIDU( pContext, idu, m_colUtilityID, newID, useAddDelta ? ADD_DELTA : SET_DATA);
         }
      else
         {
         if ( oldID != noDataValue )
            UpdateIDU( pContext, idu, m_colUtilityID, noDataValue, useAddDelta ? ADD_DELTA : SET_DATA );
         }
      } 
   return true;
   }


int Economy::EvaluateUtilitiesDeterministic( MapExprEngine *m_pMapExprEngine )
   {
   ASSERT(m_pMapExprEngine != NULL);

   int count = m_pMapExprEngine->GetExprCount();
   float maxValue = 0;
   int index = -1;
    
   for( int i = 0; i < count; i++ )
      {
      MapExpr *pExpr = m_pMapExprEngine->GetExpr( i );
      if ( pExpr->IsValid() )
         {
         if ( pExpr->GetValue() > maxValue )
            {
            index = i;
            maxValue = (float) pExpr->GetValue();
            }
         }
      }

   return index;
   }



int Economy::EvaluateUtilitiesStochastic( MapExprEngine *m_pMapExprEngine )
   {
   // this allows for a do-nothing case if utility functions don't sum to 1.0 for a given owner class
   ASSERT(m_pMapExprEngine != NULL);
   int exprCount = m_pMapExprEngine->GetExprCount();
   ASSERT( exprCount == (int) m_values.GetCount() );

   for( int i = 0; i < exprCount; i++ )
      {
      MapExpr *pExpr = m_pMapExprEngine->GetExpr( i );
      
      if ( pExpr->IsValid() )    // does this pass the query for the current IDU?
         m_values[ i ] = (float) pExpr->GetValue();
      else
         m_values[ i ] = 0;
       }

   int drawIndex = MonteCarloDraw( &m_rndUnif, m_values, false );

   return drawIndex;
   }

