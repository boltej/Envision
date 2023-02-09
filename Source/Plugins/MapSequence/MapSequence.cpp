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
// MapSequence.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#pragma hdrstop

#include "MapSequence.h"

//#include <maplayer.h>
#include <Report.h>
#include <tixml.h>
#include <PathManager.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool MapSequence::Init( MapLayer *pIDULayer )
   {
   PathManager::FindPath( m_sourceTableName, m_sourceTablePath );

   m_pSourceData = new VDataObj;

   int count = 0;
   if ( IsLayerType() )
      count = m_pSourceData->ReadDBF( m_sourceTablePath );
   else
      count = m_pSourceData->ReadAscii( m_sourceTablePath );

   if ( count <= 0 )
      {
      CString msg( "MapSequence: Unable to read input file '" );
      msg += m_sourceTablePath;
      msg += "' - this sequence will be ignored";
      Report::ErrorMsg( msg );
      m_use = false;
      }

   m_colSource = m_pSourceData->GetCol( m_sourceField );
   m_colTarget = pIDULayer->GetFieldCol( m_targetField );

   if ( m_colSource < 0 )
      {
      CString msg( "MapSequence: Unable to find source field '" );
      msg += m_sourceField;
      msg += "' - this sequence will be ignored";
      Report::ErrorMsg( msg );
      m_use = false;
      }

   if ( m_colTarget < 0 )
      {
      CString msg( "MapSequence: Unable to find target field '" );
      msg += m_targetField;
      msg += "' - this sequence will be ignored";
      Report::ErrorMsg( msg );
      m_use = false;
      }

   if ( IsLayerType() )
      {
      if ( count != pIDULayer->GetRecordCount() )
         {
         CString msg;
         msg.Format( "MapSequence: Specified input layer database '%s' has %i records; the IDU database has %i records.  The record count must match.  This sequence will be ignored" ,
            (LPCTSTR) m_sourceTableName, count, pIDULayer->GetRecordCount() );
         Report::ErrorMsg( msg );
         m_use = false;
         }
      }
	
   //else
   //   {
   //   
   //   }

   return true;
   }


BOOL MapSequenceProcess::Init( EnvContext *pContext, LPCTSTR initStr )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

	//Create an IDU index map, used for sub areas and partial IDU layer loading 
	IDUIndexLookupMap(pContext);

	if ( LoadXml( initStr, pLayer ) == false )
      return FALSE;

//   AddOutputVar( "Dwelling Units", &m_nDUData, "" );
//   AddOutputVar( "New Dwelling Units", &m_newDUData, "" );

   return TRUE; 
   }


BOOL MapSequenceProcess::InitRun( EnvContext *pContext, bool useInitialSeed )
   {
   Run( pContext, false );
   return TRUE; 
   }


// Allocates new DUs, Updates N_DU and NEW_DU
BOOL MapSequenceProcess::Run( EnvContext *pContext, bool useAddDelta )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   
	vector<int> *iduNdxVec = 0;

	int iduNdx;

	int iduCount = pLayer->GetRecordCount();
   
   for ( int i=0; i < (int) m_seqArray.GetSize(); i++ )
      {
      MapSequence *pSeq = m_seqArray[ i ];

      if ( pSeq->m_use == false )
         continue;

      if ( pSeq->IsLayerType() )
         {
         if ( pSeq->m_applyDate == pContext->currentYear )
            {
            for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
               {
               VData value;
               pSeq->m_pSourceData->Get( pSeq->m_colSource, idu, value );
               UpdateIDU( pContext, idu, pSeq->m_colTarget, value, useAddDelta ? ADD_DELTA : SET_DATA );
               }
            }
         }
      else // table type
         {
         int rows = pSeq->m_pSourceData->GetRowCount();

         for ( int i=0; i < rows; i++ )
            {
            // assume first col is time
            int year = pSeq->m_pSourceData->GetAsInt( 0, i );

            if ( year == pContext->currentYear )
               {
               // assume second col is IDU
               int idu = pSeq->m_pSourceData->GetAsInt( 1, i );
               
               VData value;
               pSeq->m_pSourceData->Get( pSeq->m_colSource, i, value );

					m_iduIndexLookupKey.iduIndex = idu;
					
			      //returns vector with the idu index for idu layer
			      iduNdxVec = &m_IDUIndexMap[m_iduIndexLookupKey];

				   if (iduNdxVec->size() == 0)
						{
						continue;
						}
					else
						{
						iduNdx = iduNdxVec->at(0);
						}

               UpdateIDU( pContext, iduNdx, pSeq->m_colTarget, value, useAddDelta ? ADD_DELTA : SET_DATA );
               }
            }
         }  // end of: else // table type
      }  // end of for each sequence
  
   return true;
   }


bool MapSequenceProcess::LoadXml( LPCTSTR _filename, MapLayer *pLayer )
   {
   CString filename;

   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "MapSequence: Input file '%s' not found - this process will be disabled", _filename );
      Report::ErrorMsg( msg );
      return false;
      }

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile( filename );

   if ( ! ok )
      {      
      Report::ErrorMsg( doc.ErrorDesc() );
      return false;
      }
 
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // MapSequence
   TiXmlElement *pXmlSequence = pXmlRoot->FirstChildElement( "sequence" );

   while ( pXmlSequence != NULL )
      {
      // <sequence source="myMaplayer.shp" time="2010" source_col="myMapLayerCol" target_col="someIduCol" />
      // <sequence source="mytable.csv" />

      MapSequence *pSeq = new MapSequence;

      XML_ATTR attrs[] = {
         // attr              type          address                      isReq checkCol
         { "source",          TYPE_CSTRING, &(pSeq->m_sourceTableName),  true,     0 },
         { "source_col",      TYPE_CSTRING, &(pSeq->m_sourceField),      false,    0 },
         { "target_col",      TYPE_CSTRING, &(pSeq->m_targetField),      true,     0 },
         { "apply_date",      TYPE_INT,     &(pSeq->m_applyDate),        false,    0 },
         { NULL,              TYPE_NULL,    NULL,                        false,    0 } };

      ok = TiXmlGetAttributes( pXmlSequence, attrs, filename, pLayer );

		if (!ok)
			{		
         delete pSeq;
			}
		else
			{
         pSeq->Init( pLayer );
			m_seqArray.Add(pSeq);
		   }

      pXmlSequence = pXmlSequence->NextSiblingElement( "sequence" );
      }

   return true;
   }

bool MapSequence::IsLayerType( void ) 
{ 

CString layerType1 = "dbf";
CString layerType2 = "DBF";

int loc = m_sourceTableName.ReverseFind('.');

CString type = m_sourceTableName.Mid( loc+1 );

if ( layerType1 == type || layerType2 == type )
	{
	return true;
	}
else
	{
	return false;
	}

}
  
int MapSequenceProcess::IDUIndexLookupMap(EnvContext *pContext)
   {
	int nIDUs = 0;

	MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

	m_colIDUIndex = pLayer->GetFieldCol("IDU_INDEX");

	for (MapLayer::Iterator idu = pLayer->Begin(); idu != pLayer->End(); idu++)
	   {
		// Build the key for the map lookup
		if ( m_colIDUIndex == -1 )
			{
			m_iduIndexInsertKey.iduIndex = idu;

			nIDUs++;
			}
		else
			{
			int iduIndex = -1;

			pLayer->GetData(idu, m_colIDUIndex, iduIndex);

			m_iduIndexInsertKey.iduIndex = iduIndex;

			nIDUs++;
			}

		// Build the map
		m_IDUIndexMap[m_iduIndexInsertKey].push_back(idu);
	
	
		} 
	return nIDUs;   
	}	