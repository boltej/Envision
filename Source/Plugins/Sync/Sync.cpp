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
// Sync.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "sync.h"
#include <deltaarray.h>
#include <tinyxml.h>
#include <randgen\randunif.hpp>
#include <envmodel.h>
#include <EnvAPI.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new SyncProcessCollection; }


RandUniform rn( 0.0, 1.0, 1 );


MapElement::MapElement(MapElement &me)
   {
   m_isRange= me.m_isRange;

   m_sourceValue=me.m_sourceValue;    // for single-valued elements
   m_srcMinValue = me.m_srcMinValue;    // next two are for elements defined with ranges
   m_srcMaxValue = me.m_srcMaxValue;

   m_syncOutcomeArray.Copy(me.m_syncOutcomeArray);
   }


void MapElement::ApplyOutcome( EnvContext *pContext, int idu, int colTarget )
   {
   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   float randVal = (float) rn.RandValue();
   float value = 0;
   int   j = 0;
   for ( ; j < (int) m_syncOutcomeArray.GetSize(); j++ )
      {
      value += m_syncOutcomeArray.GetAt( j )->probability;

      if ( value >= randVal )
         {
         SYNC_OUTCOME *pSyncOutcome = m_syncOutcomeArray.GetAt( j );

         // check for redundancy
         VData oldValue;
         pContext->pMapLayer->GetData( idu, colTarget, oldValue );

         // during init
         if ( oldValue != pSyncOutcome->targetValue )
            {            
            if (pContext->run < 0)
               pLayer->SetData(idu, colTarget, pSyncOutcome->targetValue);
            else
               pContext->ptrAddDelta( pContext->pEnvModel, idu, colTarget,
                                   pContext->currentYear, pSyncOutcome->targetValue, pContext->handle );
            }
         }
      }
   }


SyncMap::SyncMap(SyncMap &sm)
   {
   m_name = sm.m_name;

   m_sourceCol = sm.m_sourceCol;
   m_targetCol = sm.m_targetCol;
   m_colSource = sm.m_colSource;
   m_colTarget = sm.m_colTarget;
   m_inUse  = sm.m_inUse;
   m_method = sm.m_method;

   // copy ptrs and objects
   m_mapElementArray.DeepCopy(sm.m_mapElementArray);

   ASSERT(0);
   //m_valueMap. = sm.m_valueMap;
   // CMap< UINT, UINT, MapElement*, MapElement* > m_valueMap;    // maps MapElement source values to th assocated array of outcomes
   }


SyncMap::~SyncMap()
   {
   this->m_valueMap.RemoveAll();
   }


MapElement *SyncMap::FindMapElement( VData &vData )
   {
   if ( vData.IsNull() )
      return NULL;

   int count = (int) m_mapElementArray.GetSize();
   MapElement *pMapElement = NULL;

   for ( int i = 0; i < count; i++ )
      {
      pMapElement = m_mapElementArray[i];

      if ( pMapElement->m_isRange )
         {
         float minVal, maxVal, value;
         bool okMin = pMapElement->m_srcMinValue.GetAsFloat( minVal );
         bool okMax = pMapElement->m_srcMaxValue.GetAsFloat( maxVal );
         bool okVal = vData.GetAsFloat( value );

         if ( minVal <= value && value <= maxVal )
            return pMapElement;
         }
      else  // it is not a range
         {
         LONG key = vData.val.vUInt; //MAKELONG( vData.type, (short) vData.val.vUInt );
         BOOL found = m_valueMap.Lookup( key, pMapElement );

         if ( found )
            return pMapElement;

         // is it a wildcard?
         if (pMapElement->IsWildcard())
            return pMapElement;

         //float value, mapValue;
         //pMapElement->m_sourceValue.GetAsFloat( mapValue );
         //vData.GetAsFloat( value );

         //if ( value == mapValue )
         //   return pMapElement;
         }
      }  // end of: for ( i < count )

   // if we get this far, the element wasn't found.  is there a wildcard?
   //for ( int i = 0; i < count; i++ )
   //   {
   //   pMapElement = m_mapElementArray[i];
   //
   //   if ( pMapElement->IsWildcard() )
   //      return pMapElement;
   //   }

   // nothing found...
   return NULL;
   }


int SyncMap::AddMapElement( MapElement *pMap )
   {
   int index = (int) m_mapElementArray.Add( pMap );

   VData &v = pMap->m_sourceValue;
   LONG key = v.val.vUInt;   //MAKELONG( v.type, (short) v.val.vUInt );
   m_valueMap[key] = pMap;
   return index;
   }



// this method reads an xml input file
bool SyncProcess::LoadXml( LPCTSTR filename, MapLayer *pLayer )
   {
   // start parsing input file
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   bool loadSuccess = true;

   if ( !ok )
      {
      CString msg;
      msg.Format( "Error reading input file, %s", filename );
      Report::ErrorMsg( doc.ErrorDesc(), msg );
      return false;
      }

   int allocationSetCount = 0;
   int allocationCount = 0;
   int reportCount = 0;
   int constCount = 0;

   /* start iterating through the document
    * general structure is of the form:
    *
    * <?xml version="1.0" encoding="utf-8"?>
    *
    * <sync method="useDelta" >  <<<<<< corresponds to a SyncProcess
    *   <sync_map source_col="LULC_A" target_col="STANDCLASS" >   <<<<< corresinds to a SyncMap
    *      <map source_value="1">         <<<<< corresponds to a MapElement (can be value or range e.g "1-3")
    *         <outcome target_value="4" probability="20" />     <<<< Corresponds to a SYNC_OUTCOME
    *         <outcome target_value="5" probability="80" />
    *      </map>
    *   </sync_map>
    * </sync>
    */

   TiXmlElement *pXmlRoot = doc.RootElement();  // <sync  ....>

   // get any sync_maps
   TiXmlElement *pXmlSyncMap = pXmlRoot->FirstChildElement( _T( "sync_map" ) );
   int count = 0;
   while ( pXmlSyncMap != NULL )
      {
      SyncMap *pSyncMap = new SyncMap;
      CString method;
      int init = 0;

      XML_ATTR smAttrs[] =
         { // attr          type           address                   isReq   checkCol
               { "name", TYPE_CSTRING, &( pSyncMap->m_name ), true, 0 },
               { "source_col", TYPE_CSTRING, &( pSyncMap->m_sourceCol ), true, 0 },
               { "target_col", TYPE_CSTRING, &( pSyncMap->m_targetCol ), true, 0 },
               { "method", TYPE_CSTRING, &method, false, 0 },
               { "init",   TYPE_INT, &init, false, 0 },
               { NULL, TYPE_NULL, NULL, false, 0 } };

      if ( TiXmlGetAttributes( pXmlSyncMap, smAttrs, filename, pLayer ) == false )
         {
         delete pSyncMap;
         return false;
         }

      pSyncMap->m_colSource = pLayer->GetFieldCol( pSyncMap->m_sourceCol );
      pSyncMap->m_colTarget = pLayer->GetFieldCol( pSyncMap->m_targetCol );
      pSyncMap->m_init = init;

      if ( pSyncMap->m_colSource < 0 )
         {
         CString msg;
         msg.Format( _T( "Sync: Source column %s not found in IDU coverage when reading <sync_map>" ), pSyncMap->m_sourceCol );
         Report::ErrorMsg( msg );
         delete pSyncMap;

         pXmlSyncMap = pXmlSyncMap->NextSiblingElement( _T( "sync_map" ) );
         continue;
         }

      if ( pSyncMap->m_colTarget < 0 )
         {
         CString msg;
         msg.Format( _T( "Sync: Target column %s not found in IDU coverage when reading <sync_map>" ), pSyncMap->m_targetCol );
         Report::ErrorMsg( msg );
         delete pSyncMap;

         pXmlSyncMap = pXmlSyncMap->NextSiblingElement( _T( "sync_map" ) );
         continue;
         }

      if ( method.CompareNoCase( _T( "useMap" ) ) == 0 )
         pSyncMap->m_method = SyncMap::METHOD::USE_MAP;
      else
         pSyncMap->m_method = SyncMap::METHOD::USE_DELTA;

      this->m_syncMapArray.Add( pSyncMap );
 
      TYPE srcType = pLayer->GetFieldType( pSyncMap->m_colSource );
      TYPE targType = pLayer->GetFieldType( pSyncMap->m_colTarget );

      // look for child <map> elements (these are class MapElement)
      TiXmlElement *pXmlMap = pXmlSyncMap->FirstChildElement( _T( "map" ) );
      int mapCount = 0;

      while ( pXmlMap != NULL )
         {
         MapElement *pMapElement = new MapElement;
         LPCTSTR srcValue = pXmlMap->Attribute( _T( "source_value" ) );

         if ( srcValue == NULL )
            {
            CString msg( _T( "Sync: Unable to find 'source_value' attribute reading <map> element" ) );
            Report::ErrorMsg( msg );
            delete pMapElement;
            }
         else
            {
            TCHAR *buffer = new TCHAR[lstrlen( srcValue ) + 1];
            lstrcpy( buffer, srcValue );

            bool isRange = false;

            TCHAR *dash = _tcschr( buffer + 1, '-' );   //+1 to avoild leading negative signs
            if ( dash == NULL )
               {
               VData _srcValue( srcValue );
               _srcValue.ChangeType( srcType );

               if ( lstrcmp( srcValue, _T( "*" ) ) == 0 ) // * matches anything
                  _srcValue.SetNull();

               pMapElement->m_sourceValue = _srcValue;
               pMapElement->m_isRange = false;
               }
            else     // range specified                
               {
               *dash = NULL;

               pMapElement->m_srcMinValue.Parse( buffer );
               pMapElement->m_srcMinValue.ChangeType( srcType );
               pMapElement->m_sourceValue = pMapElement->m_srcMinValue;

               pMapElement->m_srcMaxValue.Parse( dash + 1 );
               pMapElement->m_srcMaxValue.ChangeType( srcType );

               pMapElement->m_isRange = true;
               }

            delete buffer;

            // successful define MapElement, add it to the SyncMap
            pSyncMap->AddMapElement( pMapElement );

            // start looking for outcome associations
            const TiXmlElement *pXmlOutcome = pXmlMap->FirstChildElement( _T( "outcome" ) );

            while ( pXmlOutcome != NULL )
               {
               LPCTSTR targetValue = pXmlOutcome->Attribute( _T( "target_value" ) );
               if ( targetValue == NULL )
                  {
                  CString msg( _T( "Unable to find 'target_value' attribute reading <map> element" ) );
                  Report::ErrorMsg( msg );
                  }
               else
                  {
                  VData _targetValue( targetValue );
                  _targetValue.ChangeType( targType );

                  LPCTSTR probability = pXmlOutcome->Attribute( _T( "probability" ) );

                  SYNC_OUTCOME *pSyncOutcome = new SYNC_OUTCOME;

                  pSyncOutcome->targetValue = _targetValue;

                  if ( probability == NULL )
                     pSyncOutcome->probability = 1.0f;
                  else
                     {
                     float p = (float) atof( probability );
                     if ( p > 1.0f )
                        pSyncOutcome->probability = p / 100.0f;
                     else
                        pSyncOutcome->probability = p;
                     }

                  pMapElement->m_syncOutcomeArray.Add( pSyncOutcome );
                  }

               pXmlOutcome = pXmlOutcome->NextSiblingElement( _T( "outcome" ) );
               }  // end of: while (pXmlOutcome != NULL )
            }  // end of: else (srcValue != NULL )

         mapCount++;
         pXmlMap = pXmlMap->NextSiblingElement( _T( "map" ) );
         }  // end of: while( pXmlMap != NULL );

      CString msg;
      msg.Format( _T( "Loaded sync_map for source column %s, target column %s (%i mappings)\n" ),
                  (LPCTSTR) pSyncMap->m_sourceCol, (LPCTSTR) pSyncMap->m_targetCol, mapCount );
      Report::LogInfo( msg );
      TRACE( msg );

      pXmlSyncMap = pXmlSyncMap->NextSiblingElement( _T( "sync_map" ) );
      }  // end of: while ( pXmlSyncMap != NULL )

   m_filename = filename;
   return true;
   }


   bool SyncProcess::Run( EnvContext *pContext )
      {
      // Basic idea:
      //
      const MapLayer *pLayer = pContext->pMapLayer;

      int syncMapCount = (int) m_syncMapArray.GetCount();

      for ( int i = 0; i < syncMapCount; i++ )
         {
         SyncMap *pSyncMap = m_syncMapArray[i];
         ASSERT( pSyncMap != NULL );

         if ( pSyncMap->m_inUse == false )
            continue;

         SyncMap::METHOD method = pSyncMap->m_method;
         // called from init() only run if asked?
         if (pContext->run < 0)
            {
            if (pSyncMap->m_init > 0)
               pSyncMap->m_method = SyncMap::METHOD::USE_MAP;
            else
               continue;      // skip if init <= 0
            }            
            
         if (pContext->currentYear < 0 && pSyncMap->m_init <= 0)
            continue;

         int colSource = pSyncMap->m_colSource;
         int colTarget = pSyncMap->m_colTarget;
         bool found = false;

         switch ( pSyncMap->m_method )
            {
            case SyncMap::METHOD::USE_DELTA:
               {
               DeltaArray *deltaArray = pContext->pDeltaArray;// get a ptr to the delta array

               // iterate through deltas added since last “seen”
               INT_PTR size = deltaArray->GetSize();
               for ( INT_PTR i = pContext->firstUnseenDelta; i < size; ++i )
                  {
                  if ( i < 0 )
                     break;

                  DELTA &delta = ::EnvGetDelta( deltaArray, i );
                  if ( delta.col == colSource )   // does the delta column match this SyncMap?
                     {
                     // matches, get corresponding MapElement
                     MapElement *pMapElement = pSyncMap->FindMapElement( delta.newValue );

                     if ( pMapElement != NULL )   // not found?  
                        pMapElement->ApplyOutcome( pContext, delta.cell, colTarget );
                     }  // end of  if ( delta.col = colSource )
                  }  // end of: deltaArray loop
               break;
               }

            case SyncMap::METHOD::USE_MAP:
               {
               for ( MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++ )
                  {
                  VData srcValue;
                  pLayer->GetData( idu, colSource, srcValue );

                  MapElement *pMapElement = pSyncMap->FindMapElement( srcValue );

                  if ( pMapElement != NULL )   // not found?  
                     pMapElement->ApplyOutcome( pContext, idu, colTarget );
                  }

               break;
               }
            }

         pSyncMap->m_method = method;
         }

      return true;
      }


   bool SyncProcessCollection::Init( EnvContext *pEnvContext, LPCTSTR  initStr /*xml input file*/ )
      {
      CString string( initStr );
      ASSERT( !string.IsEmpty() );

      SyncProcess *pProcess = new SyncProcess( pEnvContext->id );
      if ( pProcess == NULL )
         {
         CString msg( _T( "Unable to create SyncProcess instance " ) );
         msg += pEnvContext->pEnvExtension->m_name;
         Report::ErrorMsg( msg );
         return FALSE;
         }

      // load XML input file specified in initStr
      MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;
      bool ok = pProcess->LoadXml( initStr, pLayer );

      if ( !ok )
         return FALSE;

      // add to internal collection
      this->m_syncProcessArray.Add( pProcess );

      // add inUse variable for output
      for ( int j = 0; j < pProcess->GetSyncMapCount(); j++ )
         {
         SyncMap *pMap = pProcess->GetSyncMap( j );

         CString varName( pMap->m_name + ".InUse" );
         AddInputVar( varName, pMap->m_inUse, "" );
         }

      pProcess->Run(pEnvContext);

      return TRUE;
      }


   bool SyncProcessCollection::Run( EnvContext *pEnvContext )
      {
      const MapLayer *pLayer = pEnvContext->pMapLayer;

      int index;
      SyncProcess *pProcess = GetSyncProcessFromID( pEnvContext->id, &index );
      ASSERT( pProcess != NULL );

      bool ok = pProcess->Run( pEnvContext );

      return ok ? TRUE : FALSE;
      }


   SyncProcess *SyncProcessCollection::GetSyncProcessFromID( int id, int *index )
      {
      for ( int i = 0; i < m_syncProcessArray.GetSize(); i++ )
         {
         SyncProcess *pProcess = m_syncProcessArray.GetAt( i );

         if ( pProcess->m_processID == id )
            {
            if ( index != NULL )
               *index = i;

            return pProcess;
            }
         }

      if ( index != NULL )
         *index = -1;

      return NULL;
      }
