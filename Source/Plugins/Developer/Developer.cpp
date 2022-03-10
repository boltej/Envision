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
// Developer.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "Developer.h"
#include <Maplayer.h>
#include <Map.h>
#include <Report.h>
#include <tixml.h>
#include <PathManager.h>
#include <UNITCONV.H>
#include <envconstants.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int CompareUGAPriorities(const void *elem0, const void *elem1 );
int CompareNDU(const void *elem0, const void *elem1 );
//float GetPeoplePerDU( int );

struct NDU_SORT { float fracNDU; int idu; int duAreaIndex; /*int prevNDU; int newDU;*/ };


Developer::Developer() 
: EnvModelProcess()
, m_allocatePopDens( false )
, m_allocateDUs( false )
, m_expandUGAs( false )
, m_initDUs( INIT_NDUS_FROM_NONE )
, m_pDuPtLayer( NULL )
, m_duPtIndexArray( NULL )
, m_colPtNDU( -1 )
, m_colPtNewDU( -1 )
, m_colNDU( -1 )
, m_colNewDU( -1 )
, m_colUga( -1 )
, m_colPopDens( -1 )
, m_colPopDensInit( -1 )
, m_colArea( -1 )
, m_colZone( -1 )
, m_colPopCap(-1)
, m_colUxNearDist(-1)
, m_colUxNearUga(-1)
, m_colUxPriority(-1)
, m_colUxEvent(-1)
, m_polyPtMapper()
, m_pQueryEngine( NULL )
, m_currUxScenarioID( 0 )
, m_pCurrentUxScenario( NULL )
, m_startPop( -1 )
, m_nDUData( U_UNDEFINED )      // for each uga + totals
, m_newDUData( U_UNDEFINED )
, m_pUxData(NULL)
{ }



bool Developer::Init( EnvContext *pContext, LPCTSTR initStr )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   m_pQueryEngine = pContext->pQueryEngine;

   if ( LoadXml( initStr, pContext ) == false )
      return false;

   // global things first

   // if point (Buildings) layer defined, then build mapping index
   if ( m_pDuPtLayer != NULL )
      {
      m_polyPtMapper.SetLayers( pLayer, m_pDuPtLayer, true /*false*/ );     // 'true" means build index

      //CString path = PathManager::GetPath(PM_IDU_DIR);   // '\' terminated
	   //path += _T("PolyGridMaps\\IduBuildings.ppm");
      //m_polyPtMapper.LoadFromFile( path );
      }

   // need to alloc population? (<population> tag specified in input file)
   if ( m_allocatePopDens )
      InitPopDens( pContext );

   int iduCount = pLayer->GetRecordCount();
   m_polyIndexArray.SetSize( iduCount );  // for shuffled polygons
   for ( int i=0; i < iduCount; i++ )
      m_polyIndexArray[ i ] = i;

   // if using allocations (<dwellings> tag defined), initialize required data
   if ( m_allocateDUs )
      {
      ShufflePolyIDs();    // this is to randomize the initial assignment of fractional IDUs
   
      m_nDUs.SetSize( iduCount );
      m_prevNDUs.SetSize( iduCount );
      m_startNDUs.SetSize( iduCount );
      m_priorPops.SetSize( iduCount );
      for ( int i=0; i < iduCount; i++ )
         {
         m_nDUs[ i ] = 0;
         m_prevNDUs[ i ] = 0 ;
         m_startNDUs[ i ] = 0;
         m_priorPops[i] = 0;
         }
   
      // allocate starting NDU, NewDU values (in coverage if column defined, and in m_nDU array)
      // and set initial NDU values for each DUArea
      if ( m_pDuPtLayer )
         m_pDuPtLayer->SetColData(m_colPtNewDU, VData(0), true);

      InitNDUs( pLayer );
   
      // set up data collection
      int cols = int( m_duAreaArray.GetSize() ) + 2;
      m_nDUData.SetSize( cols, 0 );
      m_nDUData.SetName( "Dwelling Units (#)" );
      
      int col = 0;
      m_nDUData.SetLabel( col++, "Time" );
      for ( int i=0; i < (int) m_duAreaArray.GetSize(); i++ )
         m_nDUData.SetLabel( col++, m_duAreaArray[ i ]->m_name );
      m_nDUData.SetLabel( col, "Total" );
   
      // newDUs
      m_newDUData.SetSize( cols, 0 );
      m_newDUData.SetName( "New Dwelling Units (#)" );
      
      col = 0;
      m_newDUData.SetLabel( col++, "Time" );
      for ( int i=0; i < (int) m_duAreaArray.GetSize(); i++ )
         m_newDUData.SetLabel( col++, m_duAreaArray[ i ]->m_name );
      m_newDUData.SetLabel( col, "Total" );
   
      AddOutputVar( "Dwelling Units", &m_nDUData, "" );
      AddOutputVar( "New Dwelling Units", &m_newDUData, "" );
      }  // end of: if ( m_allocateDUs )

   if ( m_expandUGAs )
      {
      pLayer->CheckCol(m_colPopDensInit, "POPDENS0", TYPE_FLOAT, CC_AUTOADD );
      pLayer->CopyColData( m_colPopDensInit, m_colPopDens );  

      m_pCurrentUxScenario = this->FindUxScenarioFromID(m_currUxScenarioID);
      for (int i = 0; i < m_pCurrentUxScenario->m_uxArray.GetSize(); i++)
         {
         UGA* pUGA = m_pCurrentUxScenario->m_uxArray[i];
         pUGA->m_commExpArea = 0;
         pUGA->m_resExpArea = 0;
         pUGA->m_totalExpArea = 0;
         }

      // prioritize UX expansion areas  (this should be moved to init????
      PrioritizeUxAreas(pContext);
      }

   //////////////////////////////////////////
   // iterate through the DUArea layer, computing capacities for each active DUArea
   ///////float popAna = 0;
   ///////float popOH = 0;
   ///////for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
   ///////   {
   ///////   int uga = -1;
   ///////   pLayer->GetData(idu, m_colUga, uga);
   ///////
   ///////   float area = 0;
   ///////   pLayer->GetData(idu, m_colArea, area);
   ///////
   ///////   float popDens = 0;         // people/m2
   ///////   pLayer->GetData(idu, m_colPopDens, popDens);
   ///////   float pop = popDens * area;
   ///////   if (uga == 3)
   ///////      popAna += pop;
   ///////   if (uga == 83)
   ///////      popOH += pop;
   ///////   }
      /////////////////////////
   InitOutput();
   return TRUE; 
   }

bool Developer::InitRun( EnvContext *pContext, bool useInitialSeed )
   {
   m_nDUData.ClearRows();
   m_newDUData.ClearRows();

   if ( m_allocateDUs )  // <dwellings> tag defined
      {
      if (useInitialSeed == false )
         ShufflePolyIDs();    // this is to randomize the initial assignment of fractional IDUs

      // reset DUArea DU metrics
      for ( int i=0; i < m_duAreaArray.GetSize(); i++ )
         {
         DUArea *pDUArea = m_duAreaArray[ i ];
            pDUArea->m_startNDUs = 0;
         }

      // update map
      MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

      if ( m_colNewDU >= 0 )
         pLayer->SetColData( m_colNewDU, VData( 0 ), true );

      if (m_pDuPtLayer)
         m_pDuPtLayer->SetColData(m_colPtNewDU, VData(0), true );

      // update internal arrays
      int iduCount = pLayer->GetRecordCount();
      for ( int i=0; i < iduCount; i++ )
         m_nDUs[ i ] = 0;
   
      // get initial populations 
      for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
         {
         // population
         float popDens = 0;
         pLayer->GetData( idu, m_colPopDens, popDens );

         float area = 0;
         pLayer->GetData( idu, m_colArea, area );

         int nDU = 0;
         pLayer->GetData( idu, m_colNDU, nDU );

         DUArea *pDUArea = FindDUArea( idu );
         if ( pDUArea != NULL )
            {
            pDUArea->m_population += popDens * area;
            pDUArea->m_startNDUs  += nDU;
            }

         m_priorPops[idu] = popDens * area;   // at start of run
         }

      // reset DUArea DU metrics
      for ( int i=0; i < m_duAreaArray.GetSize(); i++ )
         {
         DUArea *pDUArea = m_duAreaArray[ i ];
         pDUArea->m_nDUs = pDUArea->m_startNDUs;
         //pDUArea->m_newDUs = 0;
         pDUArea->m_lastPopulation = pDUArea->m_population;

         if ( m_initDUs == INIT_NDUS_FROM_DULAYER )
            {
            pDUArea->m_peoplePerDU = pDUArea->m_population/pDUArea->m_nDUs;
            CString msg;
            msg.Format( "  Adjusted PPDU for %s=%.2f", (LPCTSTR) pDUArea->m_name, pDUArea->m_peoplePerDU );
            Report::Log( msg );
            }
         }
      }

   // reset UGA metrics
   if ( this->m_expandUGAs )
      {
      UpdateUGAStats(pContext, true);
      m_pCurrentUxScenario = this->FindUxScenarioFromID( m_currUxScenarioID );

      for ( int i=0; i < m_pCurrentUxScenario->m_uxArray.GetSize(); i++ )
         {
         UGA* pUGA = m_pCurrentUxScenario->m_uxArray[i];
         pUGA->m_commExpArea = 0;
         pUGA->m_resExpArea = 0;
         pUGA->m_totalExpArea = 0;
         }
      }

   return TRUE; 
   }


// Allocates new DUs, Updates N_DU and NEW_DU
bool Developer::Run( EnvContext *pContext )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   int iduCount = pLayer->GetRecordCount();

   if ( m_pDuPtLayer )
      m_polyPtMapper.BuildIndex();

   // first get existing NDU values from coverage if <dwellings> defined
   if (m_allocateDUs)
      {
      for (int i = 0; i < iduCount; i++)
         {
         int nDUs = 0;
         pLayer->GetData(i, this->m_colNDU, nDUs);
         m_prevNDUs[i] = m_nDUs[i] = nDUs;      // remember previous values

         if (pContext->yearOfRun == 0)
            m_startNDUs[i] = nDUs;
         }
     
      // are we supposed to allocate DUs in response to changing population densities?
      //  this will be active if the <dwellings> tag is specified in the input file
      AllocateNewDUs( pContext, pLayer );  // this populates m_nDU[], m_colNewDU, m_colNDU
      }  // end of: if ( m_allocateDUs )

   // do we need to expand the urban growth areas?
   // this will be active if the <uga_expansion> tag is specified in the input file
   if ( m_expandUGAs )
      {
      ExpandUGAs( pContext, pLayer );

      // Populate NewDU and update NDU for new DUs.  To allocate new DUs:
      //  1) for each DUArea, determine new population growth.  This gives new DUs for each DUArea.
      //  2) for each idu,
      //     a) get the DU "capacity", defined as: DU's based on popDens (float) - current DUs (int)
      //     b) use capacity to generate a sorted list (index=0 -> largest capacity
      //     c) pull from the list, until new DUs allocated

      // NDU's allocated, update DUArea stats if field area dfeind 
      //for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      //   {
      //   int uga = 0;
      //   pLayer->GetData( idu, m_colUGA, uga );
      //
      //   }  // end of: for each IDU
      }  // end of: if ( m_allocateDUs )
   
   CollectOutput( pContext->currentYear );

   return true;
   }



bool Developer::EndRun(EnvContext* pContext)
   {
   EnvGenLulcTransTable(pContext->pEnvModel);

   // first get existing NDU values from coverage if <dwellings> defined
   return true;
   }



PopDens *Developer::AddPopDens( EnvContext *pContext )
   {
   PopDens *pPopDens = new PopDens;
   ASSERT( pPopDens != NULL );
   m_popDensArray.Add( pPopDens );
   
   return pPopDens;
   }


bool Developer::InitPopDens( EnvContext *pEnvContext )
   {
   MapLayer      *pLayer         = (MapLayer*) pEnvContext->pMapLayer;
   MapExprEngine *pMapExprEngine = pEnvContext->pExprEngine;  // use Envision's MapExprEngine
   
   int recordCount = pLayer->GetRecordCount(); // # of IDUs

   // initialize popDens objects - one for each population density specificed in the xml input file
   for ( int i=0; i < (int) m_popDensArray.GetSize(); i++ )
      {
      PopDens *pPopDens = m_popDensArray[ i ];
      pPopDens->m_queryArea = 0;
      pPopDens->m_initPop = 0;
      pPopDens->m_reallocPop = 0;
      pPopDens->m_queryPop = 0;
      }      

   // get 1) total population, 2) areas associated with each PopDens area unit
   float totalPop = 0;
   float totalPopFromExpr = 0;

   float *iduPopArray = new float[ recordCount ];
   memset( iduPopArray, 0, recordCount * sizeof( float ) );

    for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      // population
      float popDens = 0;
      pLayer->GetData( idu, m_colPopDens, popDens );

      float area = 0;
      pLayer->GetData( idu, m_colArea, area );

      totalPop += popDens * area;

      for ( int i=0; i < (int) m_popDensArray.GetSize(); i++ )
         {
         PopDens *pPopDens = m_popDensArray[ i ];

         if ( pPopDens->m_use && pPopDens->m_pQuery != NULL )
            {
            bool result = false;
            bool ok = pPopDens->m_pQuery->Run( idu, result );

            if ( ok && result )
               {
               pPopDens->m_queryArea += area;

               // areas associated with each subarea
               pPopDens->m_pMapExpr->Evaluate( idu );
               float duPerAcre = (float) pPopDens->m_pMapExpr->GetValue();   // du/ac

               DUArea *pDUArea = FindDUArea( idu );
               float ppdu = 2.3f;
               if ( pDUArea )
                  ppdu = pDUArea->m_peoplePerDU;

               float pop = duPerAcre * ppdu * ACRE_PER_M2 * area;
               iduPopArray[ idu ]     = pop;
               pPopDens->m_queryPop  += pop;
               totalPopFromExpr      += pop;

               pPopDens->m_initPop += popDens * area;
               
               break;   // don't look any further if this PopDens satisfied the query
               }
            }
         }  // end of: for each PopDens
      }  // end of: for each idu

   if ( m_startPop < 0 )
      m_startPop = totalPop;

   // have total population, idu pop estimates, allocate according to area densities
   pLayer->SetColData( m_colPopDens, VData( 0.0f ), true );
   float totalNewPop = 0;
   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      float iduPopFromExpr = iduPopArray[ idu ];
      float iduPop = m_startPop * iduPopFromExpr / totalPopFromExpr;   // allocate as a portion of the total pop

      float area = 0;
      pLayer->GetData( idu, m_colArea, area );
      
      float popDens = iduPop / area;
      pLayer->SetData( idu, m_colPopDens, popDens );

      totalNewPop += iduPop;

      // add to PopDens 
      PopDens *pPopDens = NULL;
      for ( int i=0; i < (int) m_popDensArray.GetSize(); i++ )
         {
         PopDens *_pPopDens = m_popDensArray[ i ];

         if ( _pPopDens->m_use && _pPopDens->m_pQuery != NULL )
            {
            bool result = false;
            bool ok = _pPopDens->m_pQuery->Run( idu, result );

            if ( ok && result )
               {
               pPopDens = _pPopDens;
               break;
               }
            }
         }

      if ( pPopDens )
         pPopDens->m_reallocPop += iduPop;
      }  // end of: for each IDU

   // clean up
   delete [] iduPopArray;

   CString msg;   msg.Format( "  Starting population: %i, reallocated population: %i", (int) m_startPop, (int) totalNewPop );
   Report::Log( msg );   

   for( int i=0; i < (int) m_popDensArray.GetSize(); i++ )
      {
      PopDens *pPopDens = m_popDensArray[ i ];
      msg.Format( "  %s: Reallocations: %i", (PCTSTR) pPopDens->m_name, int( pPopDens->m_reallocPop-pPopDens->m_initPop) );
      Report::Log( msg );
      }

   return true;
   }


// allocate DUs to internal m_nDU array at the beginning of run
void Developer::InitNDUs( MapLayer *pLayer )
   {
   if ( m_colPopDens < 0 || m_colArea < 0 || m_colNDU < 0 )
      return;

   int duCount  = (int) m_duAreaArray.GetSize();
   int iduCount = pLayer->GetRecordCount();

   CArray< int, int > ptArray;

   int totalNDUs = 0;
   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      int ndus = 0;

      switch( m_initDUs )
         {
         case INIT_NDUS_FROM_NONE:  // from coverage
            pLayer->GetData( idu, m_colNDU, ndus );
            totalNDUs += ndus;
            break;

         case INIT_NDUS_FROM_POPDENS:  //from popDens (requires densities)
            // TODO - need to implement
            break;

         case INIT_NDUS_FROM_DULAYER:  // from buildings point coverage
            {
            ASSERT( m_pDuPtLayer != NULL );
            ndus = m_polyPtMapper.GetPointsFromPolyIndex( idu, ptArray );            
            totalNDUs += ndus;

            for ( int i=0; i < ndus; i++ )
               m_pDuPtLayer->SetData( ptArray[ i ], m_colPtNDU, 1 );
            }
            break;
         }

      m_nDUs[ idu ] = ndus;
      m_startNDUs[ idu ] = ndus;
      if ( m_initDUs != INIT_NDUS_FROM_NONE )
         pLayer->SetData( idu, m_colNDU, ndus );

      DUArea *pDUArea = FindDUArea( idu );
      if ( pDUArea )
         pDUArea->m_nDUs += ndus;
      }

   if ( m_initDUs == INIT_NDUS_FROM_DULAYER )
      {
      CString msg;
      msg.Format("  Initializing DU_COUNT from point coverage for %i of %i Dwelling unit points", totalNDUs, m_pDuPtLayer->GetRecordCount());
      Report::Log(msg);
      }

   // remember starting value
   for ( int i=0; i < (int) m_duAreaArray.GetSize(); i++ )
      m_duAreaArray[ i ]->m_startNDUs = m_duAreaArray[ i ]->m_nDUs;

   for ( int i=0; i < iduCount; i++ )
      m_prevNDUs[ i ] = m_nDUs[ i ];

   return;
   }



// ---------------------------------------------------------------------------------
// Called during Run(), this method allocates DUs to an internal array
//
// basic idea - for each DUArea, calcuate new pop, translated into DUs.  
// Then allocate the DUs based on greatest
// descrepency between idu nDU's based on popdens and existing NDUs within UGAs
// ---------------------------------------------------------------------------------

void Developer::AllocateNewDUs( EnvContext *pContext, MapLayer *pLayer )
   {
   int duCount = (int) m_duAreaArray.GetSize();
   int iduCount = pLayer->GetRecordCount();
   
   float *nduDeficits = new float[ duCount ];              // stores fractional unallocated DUs 
   memset( nduDeficits, 0, duCount * sizeof( float ) );
   
   NDU_SORT *duInfo = new NDU_SORT[ iduCount ];   // note: this is a RANDOMLY SORTED array
   for ( int i=0; i < iduCount; i++ )
      {
      duInfo[ i ].duAreaIndex = -1;
      duInfo[ i ].fracNDU = 0;
      duInfo[ i ].idu = -1;
      }
   
   // initialize any DUArea fields
   for ( int i=0; i < m_duAreaArray.GetSize(); i++ )
      {
      DUArea *pDUArea = m_duAreaArray[ i ]; 
      pDUArea->m_lastPopulation = pDUArea->m_population;
      pDUArea->m_population = 0;
      } 

   // determine what the total deficits of DUs is for each DUArea (stored in nduDeficits)
   // and the IDU-specific deficit information.  
   // In both cases, deficits are the diffence between population density 
   // and the number of dwelling units current established.
   // Note that if the IDU is not associated with a DUArea, it is ignored.
   for ( int i=0; i < iduCount; i++ )
      {
      int idu = m_polyIndexArray[ i ];   // Randomly select an IDU
                                         // find the corresponding DUArea
      DUArea *pDUArea = FindDUArea(idu);

      if (pDUArea == NULL)  // not found, no dwelling units 
         continue;   // no need to go any further, no DU's will be allocated

      float popDens = 0;
      pLayer->GetData( idu, m_colPopDens, popDens );

      float area = 0;
      pLayer->GetData( idu, m_colArea, area );

      float pop = popDens * area;

      duInfo[i].idu = idu;
      duInfo[i].duAreaIndex = pDUArea->m_index;

      // any addition in population?
      if ( pop > m_priorPops[i] )
         {
         // calculate desired nDU's
         float ppDU = pDUArea->m_peoplePerDU;
         float fNDUs = pop / ppDU;   // (#)/(#/du) = du - this is the float representation of DUs for the idu's popDens  
         float fracNDU = fNDUs - m_nDUs[ idu ]; // difference between desired and actual.  this is the amount to allocate to this IDU

         // note that the idu-level NDU deficit can be negative if there are more structures than the 
         // popdens calls for.  In this case, we set the deficit to zero for the IDU, 
         // and reduce the DUarea deficit by this amount
         nduDeficits[ pDUArea->m_index ] += fracNDU;  // unallocated fractional DUs for this DUArea 

         if ( fracNDU < 0 )
            fracNDU = 0;
         
         // remember this info.
         duInfo[i].fracNDU = fracNDU;
         }
      else  // don't allocate unles population growth has occured
         {
         duInfo[i].fracNDU = 0;
         }

      m_priorPops[i] = pop;

      pDUArea->m_population += pop;
      }  // end of: for each IDU

   // report results
   CString msg;
   msg.Format( "  DU Deficits (PopDens-derived - actual DUs) - Year %i", pContext->currentYear );
   Report::Log( msg );
   for ( int i=0; i < m_duAreaArray.GetSize(); i++ )
      {
      DUArea *pDUArea = m_duAreaArray[ i ]; 
      pDUArea->m_duDeficit = nduDeficits[ i ];
      msg.Format( "  %s - DU deficit=%i", (LPCTSTR) pDUArea->m_name, (int) nduDeficits[ i ] );
      Report::Log( msg );
      }

   // Next, need to sort the duInfo by fractional DUs, largest first.  This determines
   // the priority at which DU's are allocated to each DUArea
   qsort( duInfo, iduCount, sizeof( NDU_SORT ), CompareNDU );

   // report results
   //for ( int i=0; i < 10; i++ )
   //   {
   //   CString msg;
   //   int duIndex = duInfo[ i ].duAreaIndex;
   //   DUArea *pDUArea = this->FindDUArea( duInfo[ i ].idu );
   //
   //   msg.Format( "Top ten DUInfos: %s  %f", pDUArea->m_name, duInfo[ i ].fracNDU );
   //   Report::Log( msg );
   //   }

   // We are now ready Allocate fractions DUs to the IDUs- draw from sorted capacities until all DUs are allocated
   int i = 0;
   bool allocated = false;
   int allocatedIduCount = 0;

   while( ! allocated && i < iduCount )
      {
      int idu = duInfo[ i ].idu;
      int duAreaIndex = duInfo[ i ].duAreaIndex;

      // is this IDU assoicated with a DUArea?
      if ( duAreaIndex >= 0 )
         {
         // any remaining needs in this DUArea and IDU? if not, skip to the next duInfo
         if ( nduDeficits[ duAreaIndex ] > 0 && duInfo[ i ].fracNDU > 0 )
            {
            // this DUArea deficit still needed
            float nduDeficit = nduDeficits[ duAreaIndex ];

            // the DUs to be allocated for this IDU is the smaller of the 
            // remaining DUArea deficit and the deficit associated with this IDU
            int allocation = (int) min( nduDeficit , duInfo[ i ].fracNDU );   
           
            if ( allocation == 0 )
               allocation = 1;

            //if ( allocation > 0 && allocation < 1 ) 
            //   allocation = 1;
            //else if ( allocation < 0 )
            //   allocation = 0;

            m_nDUs[ idu ]              += allocation;
            duInfo[ i ].fracNDU        -= allocation;
            nduDeficits[ duAreaIndex ] -= allocation;

            allocatedIduCount++;
            }
         }
      i++;

      //if ( i == iduCount )
      //   i = 0;

      // have we fully satisfied the request?  If so, stop allocating
      allocated = true;
      for ( int j=0; j < duCount; j++ )
         {
         if ( nduDeficits[ j ] > 0 )    // check to see whether this DUArea is fully allocated
            {
            allocated = false;
            break;
            }
         }
      }  // end of: while ( still allocating)

   if ( true ) // i == iduCount ) // did we run out of IDUs?
      {
      float deficit = 0;
      for ( int j=0; j < (int) m_duAreaArray.GetSize(); j++ )
         deficit += nduDeficits[ j ];

      CString msg;
      msg.Format( "  DU deficits remaining after Year %i allocations = %i.  IDU Counts %i of %i examined", pContext->currentYear, (int) deficit, allocatedIduCount, i );
      Report::Log( msg );

      for ( int i=0; i < m_duAreaArray.GetSize(); i++ )
         {
         DUArea *pDUArea = m_duAreaArray[ i ]; 

         int newPop = int( pDUArea->m_population - pDUArea->m_lastPopulation );
         CString msg;
         msg.Format( "  %s - New Pop: %i starting DU deficit: %i, ending DU deficit=%i", 
            (LPCTSTR) pDUArea->m_name, newPop, (int) pDUArea->m_duDeficit, (int) nduDeficits[ i ] );
         Report::Log( msg );
         }
      }

   // reset DUArea counts
   for ( int i=0; i < (int) this->m_duAreaArray.GetSize(); i++ )
      {
      DUArea *pDUArea = this->m_duAreaArray[ i ];
      pDUArea->m_nDUs   = 0;
      //pDUArea->m_newDUs = 0; 
      }

   // write results to delta array
   int totalDUs = 0, totalStartDUs=0, totalNewDUs=0;
   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      int cumNewDU   = m_nDUs[ idu ] - m_startNDUs[ idu ];     // this is the CUMULATIVE newDUs for this IDU (over time)
      int newDU      = m_nDUs[ idu ] - m_prevNDUs[ idu ];

      totalDUs      += m_nDUs[ idu ];                // note - m_nDUs[] array was updated earlier in this method and reflects new growth
      totalStartDUs += m_startNDUs[ idu ];
      totalNewDUs   += newDU;

      UpdateIDU( pContext, idu, m_colNewDU, cumNewDU, ADD_DELTA  );
      UpdateIDU( pContext, idu, m_colNDU, (int) m_nDUs[ idu ], ADD_DELTA  );

      // if we have an associated building pt layer, add a point to it.
      if ( newDU > 0 && m_pDuPtLayer != NULL && m_colPtNDU >= 0 )
         {
         // first, see if there is a a point associated with this IDU
         // to do this, we look up the point index(s) using the idu index as the lookup key.
         CArray<int, int> polyIndexes;
         int count = m_polyPtMapper.GetPointsFromPolyIndex( idu, polyIndexes );

         if ( count > 0 )     // existing points?
            {                 // then just increment the building count of the first one
            int nDUs = 0;
            m_pDuPtLayer->GetData( polyIndexes[0], m_colPtNDU, nDUs );
            m_pDuPtLayer->SetData( polyIndexes[0], m_colPtNDU, nDUs + newDU );
            m_pDuPtLayer->SetData(polyIndexes[0], m_colPtNewDU, newDU);
            }
         else
            {
            Poly *pPoly = pLayer->GetPolygon( idu );
            Vertex v = pPoly->GetCentroid();

            int ptIndex = m_pDuPtLayer->AddPoint( v.x, v.y );
            m_polyPtMapper.Add( idu, ptIndex );
            m_pDuPtLayer->SetData( ptIndex, m_colPtNDU, newDU );
            m_pDuPtLayer->SetData(ptIndex, m_colPtNewDU, newDU);
            }
         }
      
      // update DUArea stats
      DUArea *pDUArea = FindDUArea( idu );
      if ( pDUArea != NULL )
         {
         pDUArea->m_nDUs   += m_nDUs[ idu ];
         //pDUArea->m_newDUs += newDU; 
         } 
      } // end of: for each IDU
   
   CString msg2;
   msg2.Format( "  Year: %i, DUs=%i, New DUs=%i, StartDUs=%i", pContext->currentYear, totalDUs, totalNewDUs, totalStartDUs );
   Report::Log( msg2 );      

   delete [] duInfo;
   delete [] nduDeficits;
   return;
   }
   


DUArea *Developer::AddDUArea( void )
   {
   DUArea *pDUArea = new DUArea;
   ASSERT( pDUArea != NULL );

   int index = (int) m_duAreaArray.Add( pDUArea );
   pDUArea->m_index = index;

   return pDUArea;
   }


DUArea *Developer::FindDUArea( int idu )
   {
   for ( int j=0; j < (int) m_duAreaArray.GetSize(); j++ )
      {
      DUArea *pDUArea = m_duAreaArray[ j ];

      if ( pDUArea->m_use && pDUArea->m_pQuery != NULL )
         {
         bool result = false;
         bool ok = pDUArea->m_pQuery->Run( idu, result );

         if ( ok && result )  // found the applicable DUArea?
            return pDUArea;
         }
      }

   return NULL;
   }

   
//DUArea *Developer::FindUGAFromName( LPCTSTR name )
//   {
//   for ( int i=0; i < (int) m_duAreaArray.GetSize(); i++ )
//      if ( m_duAreaArray[ i ]->m_name.CompareNoCase( name ) == 0 )
//         return m_duAreaArray[ i ];
//
//   return NULL;
//   }


int Developer::AddUGA( UxScenario *pScenario, UGA *pUGA )
   {
   int index = (int) pScenario->m_uxArray.Add( pUGA );
   pUGA->m_index = index;
   m_idToUGAMap.SetAt( pUGA->m_id, pUGA );

   return index;
   }
   

   
UGA *Developer::FindUGAFromName( LPCTSTR name )
   {
   if ( m_pCurrentUxScenario == NULL )
      return NULL;

   for ( int i=0; i < (int) m_pCurrentUxScenario->m_uxArray.GetSize(); i++ )
      if ( m_pCurrentUxScenario->m_uxArray[ i ]->m_name.CompareNoCase( name ) == 0 )
         return m_pCurrentUxScenario->m_uxArray[ i ];

   return NULL;
   }

UxScenario *Developer::FindUxScenarioFromID( int id )
   {
   for ( int i=0; i < (int) m_uxScenarioArray.GetSize(); i++ )
      if ( m_uxScenarioArray[ i ]->m_id == id )
         return m_uxScenarioArray[ i ];

   return NULL;
   }


//float Developer::GetPeoplePerDU( int uga )
//   {
//   DUArea *pDUArea = FindUGAFromID( uga );
//
//   if ( pDUArea == NULL )
//      return 2.3f;
//
//   return pDUArea->m_peoplePerDU;
//   }


void Developer::InitOutput()
   {
   if (this->m_expandUGAs)
      {
      if (m_pUxData == NULL)
         {
         // get current scenario
         m_pCurrentUxScenario = this->FindUxScenarioFromID(m_currUxScenarioID);

         // currentPopulation, currentArea, newPopulation, pctAvilCapacity
         int ugaCount = (int)m_pCurrentUxScenario->m_uxArray.GetSize();
         int cols = 1 + ((ugaCount+1) * 7);
         m_pUxData = new FDataObj(cols, 0, U_YEARS);
         ASSERT(m_pUxData != NULL);
         m_pUxData->SetName("Developer");

         m_pUxData->SetLabel(0, _T("Time"));

         int col = 1;
         for (int i = 0; i < m_pCurrentUxScenario->m_uxArray.GetSize(); i++)
            {
            UGA* pUGA = m_pCurrentUxScenario->m_uxArray[i];
            CString name = pUGA->m_name;
            m_pUxData->SetLabel(col++, _T(name + ".Population"));
            m_pUxData->SetLabel(col++, _T(name + ".Area (ac)"));
            m_pUxData->SetLabel(col++, _T(name + ".New Population"));
            m_pUxData->SetLabel(col++, _T(name + ".Pct Available Capacity"));
            m_pUxData->SetLabel(col++, _T(name + ".Res Expansion Area (ac)"));
            m_pUxData->SetLabel(col++, _T(name + ".Comm Expansion Area (ac)"));
            m_pUxData->SetLabel(col++, _T(name + ".Total Expansion Area (ac)"));
            }
         // add totals
         m_pUxData->SetLabel(col++, _T("Total.Population"));
         m_pUxData->SetLabel(col++, _T("Total.Area (ac)"));
         m_pUxData->SetLabel(col++, _T("Total.New Population"));
         m_pUxData->SetLabel(col++, _T("Total.Pct Available Capacity"));
         m_pUxData->SetLabel(col++, _T("Total.Res Expansion Area (ac)"));
         m_pUxData->SetLabel(col++, _T("Total.Comm Expansion Area (ac)"));
         m_pUxData->SetLabel(col++, _T("Total.Total Expansion Area (ac)"));
         ASSERT(col == cols);
         }
      }

   this->AddOutputVar("Developer", m_pUxData, "");
   }

void Developer::CollectOutput( int year )
   {
   if (m_allocateDUs)  // <dwellings> tag defined
      {
      int cols = (int)m_duAreaArray.GetSize() + 2;   // time + total
      float* data = new float[cols];
      data[0] = (float)year;

      // number of DUs first
      int ndus = 0;
      for (int i = 0; i < (int)m_duAreaArray.GetSize(); i++)
         {
         DUArea* pDUArea = m_duAreaArray[i];
         data[i + 1] = (float)pDUArea->m_nDUs;
         ndus += pDUArea->m_nDUs;
         }

      data[cols - 1] = (float)ndus;
      m_nDUData.AppendRow(data, cols);

      // number of new DUs nexts
      int newdus = 0;
      for (int i = 0; i < (int)m_duAreaArray.GetSize(); i++)
         {
         DUArea* pDUArea = m_duAreaArray[i];
         data[i + 1] = (float)(pDUArea->m_nDUs - pDUArea->m_startNDUs);
         newdus += pDUArea->m_nDUs - pDUArea->m_startNDUs;
         }

      data[cols - 1] = (float)newdus;
      m_newDUData.AppendRow(data, cols);

      delete[] data;
      }

   if (this->m_expandUGAs)
      {
      ASSERT(m_pUxData != NULL);

      // currentPopulation, currentArea, newPopulation, pctAvilCapacity
      int ugaCount = (int)m_pCurrentUxScenario->m_uxArray.GetSize();
      int cols = 1 + ((ugaCount+1) * 7);

      float* data = new float[cols];
      data[0] = year;

      float totalPop = 0.0f;
      float totalAc = 0.0f;
      float totalNewPop = 0.0f;
      float totalPctAvailCap = 0.0f;
      float totalResExpAc = 0.0f;
      float totalCommExpAc = 0.0f;
      float totalExpAc = 0.0f;
      int col = 1;
      for (int i = 0; i < m_pCurrentUxScenario->m_uxArray.GetSize(); i++)
         {
         UGA* pUGA = m_pCurrentUxScenario->m_uxArray[i];

         data[col++] = pUGA->m_currentPopulation;
         data[col++] = pUGA->m_currentArea * ACRE_PER_M2;
         data[col++] = pUGA->m_newPopulation;
         data[col++] = pUGA->m_pctAvailCap;
         data[col++] = pUGA->m_resExpArea * ACRE_PER_M2;
         data[col++] = pUGA->m_commExpArea * ACRE_PER_M2;
         data[col++] = pUGA->m_totalExpArea * ACRE_PER_M2;
         totalPop         += pUGA->m_currentPopulation;
         totalAc          += pUGA->m_currentArea * ACRE_PER_M2;
         totalNewPop      += pUGA->m_newPopulation;
         totalPctAvailCap += (pUGA->m_pctAvailCap * pUGA->m_currentArea * ACRE_PER_M2);  //area weighted by UGA area
         totalResExpAc    += pUGA->m_resExpArea * ACRE_PER_M2;
         totalCommExpAc   += pUGA->m_commExpArea * ACRE_PER_M2;
         totalExpAc       += pUGA->m_totalExpArea * ACRE_PER_M2;
         }

      data[col++] = totalPop;
      data[col++] = totalAc;
      data[col++] = totalNewPop;
      data[col++] = totalPctAvailCap / totalAc;
      data[col++] = totalResExpAc;
      data[col++] = totalCommExpAc;
      data[col++] = totalExpAc;
      ASSERT(col == cols);
      m_pUxData->AppendRow(data, cols);
      delete[] data;
      }
   }


bool Developer::LoadXml( LPCTSTR _filename, EnvContext *pContext )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   MapExprEngine *pMapExprEngine = pContext->pExprEngine;  // use Envision's MapExprEngine

   CString filename;
   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "  Input file '%s' not found - this process will be disabled", _filename );
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
   TiXmlElement *pXmlRoot = doc.RootElement();  // developer

   LPTSTR name=NULL, popDensCol=NULL, popCapCol=NULL, initFrom=NULL, duPtLayer=NULL;

   XML_ATTR attrs[] = {
         // attr              type        address       isReq checkCol
         { "name",            TYPE_STRING, &name,       true,    0 },
         { "popDens_col",     TYPE_STRING, &popDensCol, true,    CC_MUST_EXIST },
         //{ "init_ndu_from",   TYPE_STRING, &initFrom,   false,   0 },
         { "du_points",       TYPE_STRING, &duPtLayer,  false,   0 },
         { NULL,              TYPE_NULL,    NULL,       false,   0 } };

   ok = TiXmlGetAttributes( pXmlRoot, attrs, filename, pLayer );

   if ( ! ok )
      return false;

   this->m_colArea    = pLayer->GetFieldCol( "AREA" );
   this->m_colPopDens = pLayer->GetFieldCol( popDensCol );

   //if ( initFrom != NULL )
   //   {
   //   switch( initFrom[ 0 ] )
   //      {
   //      case 'c':   // from coverage
   //      case 'C':
   //         m_populateDUs = false;
   //         break;
   //
   //      case 'p':   // from popDens
   //      case 'P':
   //         m_populateDUs = true;
   //         break;
   //      }
   //   }

   if ( duPtLayer != NULL )
      {
      m_duPtLayerName = duPtLayer;
      m_pDuPtLayer = pLayer->m_pMap->GetLayer( m_duPtLayerName );

      if ( m_pDuPtLayer == NULL )
         {
         CString msg( "  Unable to find Point layer [" );
         msg += duPtLayer;
         msg += "] was not found in the Map.  This functionality will be disabled.";
         Report::ErrorMsg( msg );
         }
      else
         {
         m_pDuPtLayer->CheckCol(m_colPtNDU, "NDU", TYPE_INT, CC_AUTOADD );
         m_pDuPtLayer->CheckCol(m_colPtNewDU, "NewDU", TYPE_INT, CC_AUTOADD);

         //m_pDuPtLayer->SetOutlineColor( RGB( 0, 0, 127 ) );
         //m_pDuPtLayer->SetSolidFill( RGB( 0,0, 255 ) );
         m_pDuPtLayer->SetActiveField( m_colPtNDU );
         //m_pDuPtLayer->UseVarWidth( 0, 5 );
        }
      }

   //------------------------------------------------------------------------------------
   //-- PopAlloc ------------------------------------------------------------------------
   //------------------------------------------------------------------------------------

   TiXmlElement *pXmlPop = pXmlRoot->FirstChildElement( _T("population" ) );

   if ( pXmlPop != NULL )
      {
      float startPop = -1;
      int result = pXmlPop->QueryFloatAttribute( "start_pop", &startPop );
      if ( result )
         m_startPop = startPop;

      TiXmlElement *pXmlPopDens = pXmlPop->FirstChildElement( _T("pop_dens" ) );
      while ( pXmlPopDens != NULL )
         {
         m_allocatePopDens = true;

         LPCTSTR name=NULL, query=NULL, expr=NULL;
         XML_ATTR pdAttrs[] = {
            // attr      type           address  isReq  checkCol
            { "name",  TYPE_STRING,     &name,  true,    0 },
            { "query", TYPE_STRING,     &query, true,    0 },
            { "value", TYPE_STRING,     &expr,  true,    0 },   // dus/ac
            { NULL,    TYPE_NULL,       NULL,   false,   0 } };
         
         ok = TiXmlGetAttributes( pXmlPopDens, pdAttrs, filename );
         if ( ! ok )
            {
            CString msg; 
            msg.Format( _T("Misformed element reading <pop_dens> attributes in input file %s"), filename );
            Report::ErrorMsg( msg );

            return false;
            }

         PopDens *pPopDens = this->AddPopDens( pContext );
         pPopDens->m_name = name;
         pPopDens->m_query = query;
         pPopDens->m_expr  = expr;

         // compile query
         if ( pPopDens->m_query.GetLength() > 0 )
            {
            ASSERT( m_pQueryEngine != NULL );
            pPopDens->m_pQuery = m_pQueryEngine->ParseQuery( query, 0, name );
            }

         // compile expression
         if ( pPopDens->m_expr.GetLength() > 0 )
            {
            pPopDens->m_pMapExpr = pMapExprEngine->AddExpr( name, expr, NULL ); //pPopDens->m_query );
         
            if ( pPopDens->m_pMapExpr == NULL )
               {
               CString msg;
               msg.Format( "  Invalid 'value' expression '%s' encountered for output '%s' This output will be ignored...", (LPCTSTR) pPopDens->m_expr, (LPCTSTR) pPopDens->m_name );
         
               Report::ErrorMsg( msg );
               pPopDens->m_use = false;
               }
            else
               pPopDens->m_pMapExpr->Compile();
            }

         //CString msg( "  Added PopDens element " );
         //msg += name;
         //Report::Log( msg );

         pXmlPopDens = pXmlPopDens->NextSiblingElement( "pop_dens" );
         }

      CString msg;
      msg.Format( "  Added %i PopDens elements", (int) this->m_popDensArray.GetSize() );
      Report::Log( msg );
      }

   //-----------------------------------------------------------------------------------
   //-- Dwelling units -----------------------------------------------------------------
   //-----------------------------------------------------------------------------------
 
   TiXmlElement *pXmlDUs = pXmlRoot->FirstChildElement( _T("dwellings" ) );

   if ( pXmlDUs != NULL )
      {
      LPTSTR duCol="N_DU", newDUCol="New_DU", initFrom="none";

      XML_ATTR dwellingAttrs[] = {
            // attr         type         address      isReq  checkCol
            { "du_col",     TYPE_STRING, &duCol,      false,   CC_AUTOADD | TYPE_LONG },
            { "newDU_col",  TYPE_STRING, &newDUCol,   false,   CC_AUTOADD | TYPE_LONG },
            { "init_from",  TYPE_STRING, &initFrom,   false,   0 },
            { NULL,         TYPE_NULL,   NULL,        false,   0 } };
         
      ok = TiXmlGetAttributes( pXmlDUs, dwellingAttrs, filename, pLayer );
      if ( ! ok )
         {
         CString msg; 
         msg.Format( _T("Misformed root element reading <dwellings> attributes in input file %s"), filename );
         Report::ErrorMsg( msg );
         return false;
         }

      this->CheckCol( pLayer, m_colNDU, duCol, TYPE_INT, CC_AUTOADD );
      this->CheckCol( pLayer, m_colNewDU, newDUCol, TYPE_INT, CC_AUTOADD );
 
      switch( initFrom[ 0 ] )
         {
         case 'n':   // none
         case 'N':
         case 0:
            m_initDUs = INIT_NDUS_FROM_NONE;
            break;

         case 'p':   // popDens
         case 'P':
            m_initDUs = INIT_NDUS_FROM_POPDENS;
            break;

         case 'd':
         case 'D':
            m_initDUs = INIT_NDUS_FROM_DULAYER;
            break;
         }

      TiXmlElement *pXmlDUArea = pXmlDUs->FirstChildElement( _T("du_area" ) );
      while ( pXmlDUArea != NULL )
         {
         m_allocateDUs = true;

         //DUArea *pDUArea = new DUArea;
         LPTSTR name=NULL, query=NULL;
         float ppdu = 2.3f;

         XML_ATTR duAreaAttrs[] = {
            // attr      type           address        isReq  checkCol
            { "name",  TYPE_STRING,    &name,          true,    0 },
            { "query", TYPE_STRING,    &query,         true,    0 },
            { "ppdu",  TYPE_FLOAT,     &ppdu,          true,    0 },
            { NULL,    TYPE_NULL,      NULL,           false,   0 } };
         
         ok = TiXmlGetAttributes( pXmlDUArea, duAreaAttrs, filename );
         if ( ! ok )
            {
            CString msg; 
            msg.Format( _T("Misformed root element reading <du_area> attributes in input file %s"), filename );
            Report::ErrorMsg( msg );
            return false;
            }

         DUArea *pDUArea = this->AddDUArea(); // pLayer, id, true );
         pDUArea->m_name = name;
         pDUArea->m_peoplePerDU = ppdu;
     
         if ( query != NULL )
            {
            pDUArea->m_query = query;

            ASSERT( m_pQueryEngine != NULL );
            pDUArea->m_pQuery = m_pQueryEngine->ParseQuery( query, 0, name );
            }

         //CString msg( "  Added DUArea " );
         //msg += name;
         //Report::Log( msg );

         pXmlDUArea = pXmlDUArea->NextSiblingElement( "du_area" );
         }

      CString msg;
      msg.Format( "  Added %i DUAreas", (int) this->m_duAreaArray.GetSize() );
      Report::Log( msg );

      }  // end of: id ( pXmlDwellings != NULL )
      
   //---------------------------------------------------------------------------------------------
   //-- UGA Expansion ----------------------------------------------------------------------------
   //---------------------------------------------------------------------------------------------

   TiXmlElement *pXmlUGAs = pXmlRoot->FirstChildElement( _T("uga_expansions" ) );
   if ( pXmlUGAs != NULL )
      {
      LPTSTR method="nearest", ugaCol="UGA", popCapCol=NULL, zoneCol=NULL, distCol=NULL, nearUgaCol=NULL, priorityCol=NULL, eventCol=NULL;

      XML_ATTR uxAttrs[] = {
            // attr             type         address      isReq  checkCol
            { "method",         TYPE_STRING, &method,      false,   0 },
            { "uga_col",        TYPE_STRING, &ugaCol,      true,   CC_MUST_EXIST },
            { "zone_col",       TYPE_STRING, &zoneCol,     true,   CC_MUST_EXIST },
            { "pop_cap_col",    TYPE_STRING, &popCapCol,   true,   CC_MUST_EXIST },
            { "near_dist_col",  TYPE_STRING, &distCol,     true,   CC_MUST_EXIST },
            { "near_uga_col",   TYPE_STRING, &nearUgaCol,  true,   CC_MUST_EXIST },
            { "priority_col",   TYPE_STRING, &priorityCol, true,   CC_AUTOADD | TYPE_LONG },
            { "event_col",      TYPE_STRING, &eventCol,    true,   CC_AUTOADD | TYPE_LONG },
            { NULL,             TYPE_NULL,   NULL,         false,   0 } };
         
      ok = TiXmlGetAttributes( pXmlUGAs, uxAttrs, filename, pLayer );
      if ( ! ok )
         {
         CString msg; 
         msg.Format( _T("Misformed root element reading <uga_expansions> tag in input file %s"), filename );
         Report::ErrorMsg( msg );
         return false;
         }

      this->m_colUga            = pLayer->GetFieldCol( ugaCol );
      this->m_colZone           = pLayer->GetFieldCol( zoneCol );
      this->m_colPopCap         = pLayer->GetFieldCol( popCapCol );
      this->m_colUxPriority     = pLayer->GetFieldCol( priorityCol );
      this->m_colUxEvent        = pLayer->GetFieldCol( eventCol );
      this->m_colUxNearUga    = pLayer->GetFieldCol(nearUgaCol);
      this->m_colUxNearDist     = pLayer->GetFieldCol(distCol);

      m_expandUGAs = true;

      TiXmlElement *pXmlUxScn = pXmlUGAs->FirstChildElement( _T("scenario" ) );
      
      while ( pXmlUxScn != NULL )
         {
         UxScenario *pUxScn = new UxScenario;
   
         XML_ATTR uxScnAttrs[] = {
            // attr                 type           address                      isReq  checkCol
            { "name",             TYPE_CSTRING,   &(pUxScn->m_name),            true,    0 },
            { "id",               TYPE_INT,       &(pUxScn->m_id),              true,    0 },
            { "plan_horizon",     TYPE_INT,       &(pUxScn->m_planHorizon),     true,    0 },
            { "expand_trigger",   TYPE_FLOAT,     &(pUxScn->m_expandTrigger),   true,    0 },
            //{ "res_query",        TYPE_CSTRING,   &(pUxScn->m_resQuery),        true,    0 },
            //{ "comm_query",       TYPE_CSTRING,   &(pUxScn->m_commQuery),       true,    0 },
            { NULL,               TYPE_NULL,      NULL,                         false,   0 } };

         ok = TiXmlGetAttributes( pXmlUxScn, uxScnAttrs, filename );
         if ( ! ok )
            {
            CString msg; 
            msg.Format( _T("  Misformed element reading <scenario> attributes in input file %s"), filename );
            Report::ErrorMsg( msg );
            delete pUxScn;
            return false;
            }

         // compile res, commercial queries
         //pUxScn->m_pResQuery = pContext->pQueryEngine->ParseQuery(pUxScn->m_resQuery, 0, pUxScn->m_name + "_res");
         //pUxScn->m_pCommQuery = pContext->pQueryEngine->ParseQuery(pUxScn->m_commQuery, 0, pUxScn->m_name + "_comm");

         // have scenario, start adding UGA element
         TiXmlElement *pXmlUga = pXmlUxScn->FirstChildElement( _T("uga_expansion" ) );
         
         while ( pXmlUga != NULL )
            {
            UGA *pUGA = new UGA;
   
            XML_ATTR ugaAttrs[] = {
               // attr                 type           address                      isReq  checkCol
               { "name",             TYPE_CSTRING,   &(pUGA->m_name),            true,    0 },
               { "id",               TYPE_INT,       &(pUGA->m_id),              true,    0 },
               { "est_growth_rate",  TYPE_FLOAT,     &(pUGA->m_estGrowthRate),   true,    0 },
               { "new_res_density",  TYPE_FLOAT,     &(pUGA->m_newResDensity),   true,    0 },
               { "res_comm_ratio",   TYPE_FLOAT,     &(pUGA->m_resCommRatio),    true,    0 },
               { "res_constraint",   TYPE_CSTRING,   &(pUGA->m_resQuery),        true,    0 },
               { "comm_constraint",  TYPE_CSTRING,   &(pUGA->m_commQuery),       true,    0 },
               { "res_zone",         TYPE_INT,       &(pUGA->m_zoneRes),         false,   0 },
               { "comm_zone",        TYPE_INT,       &(pUGA->m_zoneComm),        false,   0 },
               { "startPop",         TYPE_FLOAT,     &(pUGA->m_startPopulation), false,   0 },
               { "ppdu",             TYPE_FLOAT,     &(pUGA->m_ppdu),            false,   0 },
               { NULL,               TYPE_NULL,      NULL,                          false,   0 } };
            
            ok = TiXmlGetAttributes( pXmlUga, ugaAttrs, filename );
            if ( ! ok )
               {
               CString msg; 
               msg.Format( _T("  Misformed element reading <uga_expansion> attributes in input file %s"), filename );
               Report::ErrorMsg( msg );
               delete pUGA;
               return false;
               }
   
            AddUGA( pUxScn, pUGA );
   
            // compile res query
            if ( pUGA->m_resQuery.GetLength() > 0 )
               {
               ASSERT( m_pQueryEngine != NULL );
               pUGA->m_pResQuery = m_pQueryEngine->ParseQuery( pUGA->m_resQuery, 0, pUGA->m_name );
               }
   
            // compile comm query
            if ( pUGA->m_commQuery.GetLength() > 0 )
               {
               ASSERT( m_pQueryEngine != NULL );
               pUGA->m_pCommQuery = m_pQueryEngine->ParseQuery( pUGA->m_commQuery, 0, pUGA->m_name );
               }

            CString msg( "  Added UGA Expansion for " );
            msg += pUGA->m_name;
            Report::Log( msg );
   
            pXmlUga = pXmlUga->NextSiblingElement( "uga_expansion" );
            }  // end of: while ( pXmlUga != NULL )

         this->m_uxScenarioArray.Add( pUxScn );

         pXmlUxScn = pXmlUxScn->NextSiblingElement( "scenario");
         }

      if ( m_uxScenarioArray.GetSize() > 0 )
         m_pCurrentUxScenario = m_uxScenarioArray.GetAt( 0 );
      
      // next, zone information      
      TiXmlElement *pXmlZones = pXmlRoot->FirstChildElement( _T("zones" ) );
   
      if ( pXmlZones )
         {
         TiXmlElement *pXmlRes = pXmlZones->FirstChildElement( "residential" );
         while ( pXmlRes != NULL )
            {
            ZoneInfo *pZone = new ZoneInfo;
   
            XML_ATTR zoneAttrs[] = {
               // attr                 type           address                isReq  checkCol
               { "id",               TYPE_INT,       &(pZone->m_id),        true,    0 },
               { "density",          TYPE_FLOAT,     &(pZone->m_density),   true,    0 },
               { "constraint",       TYPE_CSTRING,   &(pZone->m_query),     false,   0 },
               { NULL,               TYPE_NULL,      NULL,                  false,   0 } };
         
            ok = TiXmlGetAttributes( pXmlRes, zoneAttrs, filename );
            if ( ! ok )
               {
               CString msg; 
               msg.Format( _T("  Misformed element reading <residential> attributes in input file %s"), filename );
               Report::ErrorMsg( msg );
               delete pZone;
               }
            else
               {
               m_resZoneArray.Add( pZone );
   
               if ( pZone->m_query.GetLength() > 0 )
                  pZone->m_pQuery = m_pQueryEngine->ParseQuery( pZone->m_query, 0, "Developer" );
               }
            pXmlRes = pXmlRes->NextSiblingElement( "residential");
            }
   
         TiXmlElement *pXmlComm = pXmlZones->FirstChildElement( "commercial" );
         while ( pXmlComm != NULL )
            {
            ZoneInfo *pZone = new ZoneInfo;
   
            XML_ATTR zoneAttrs[] = {
               // attr                 type           address                isReq  checkCol
               { "id",               TYPE_INT,       &(pZone->m_id),        true,    0 },
               { "density",          TYPE_FLOAT,     &(pZone->m_density),   true,    0 },
               { "constraint",       TYPE_CSTRING,   &(pZone->m_query),     false,   0 },
               { NULL,               TYPE_NULL,      NULL,                  false,   0 } };
         
            ok = TiXmlGetAttributes( pXmlComm, zoneAttrs, filename );
            if ( ! ok )
               {
               CString msg; 
               msg.Format( _T("  Misformed element reading <commercial> attributes in input file %s"), filename );
               Report::ErrorMsg( msg );
               delete pZone;
               }
            else
               {
               m_commZoneArray.Add( pZone );
   
               if ( pZone->m_query.GetLength() > 0 )
                  pZone->m_pQuery = m_pQueryEngine->ParseQuery( pZone->m_query, 0, "eveloper" );
                  pZone->m_pQuery = m_pQueryEngine->ParseQuery( pZone->m_query, 0, "eveloper" );
               }
   
            pXmlComm = pXmlComm->NextSiblingElement( "commercial" );
            }
         }  // end of:  if ( pXmlZones )

      }  // end of: if ( pXmlUGAs )


   return true;
   }


   
int CompareNDU(const void *elem0, const void *elem1 )
   {
   // The return value of this function should represent whether elem0 is considered
   // less than, equal to, or greater than elem1 by returning, respectively, 
   // a negative value, zero or a positive value.
   
   float v0 = *((float*) elem0 );
   float v1 = *((float*) elem1 );

   //v0 = fmod( v0, 1.0f );
   //v1 = fmod( v1, 1.0f );

   if ( v0 < v1 )
      return 1;
   else if ( v0 == v1 )
      return 0;
   else
      return -1;
   }
   

bool Developer::ExpandUGAs(EnvContext* pContext, MapLayer* pLayer)
   {
   if (m_pCurrentUxScenario == NULL)
      {
      Report::LogError("  No current scenario defined during Run()");
      return false;
      }

   // for each UGA, determine available capacity
   UpdateUGAStats(pContext, false);   // false

   INT_PTR ugaCount = m_pCurrentUxScenario->m_uxArray.GetSize();

   for (INT_PTR i = 0; i < ugaCount; i++)
      {
      UGA* pUGA = m_pCurrentUxScenario->m_uxArray[i];

      // time to trigger a UGA expansion?
      if (pUGA->m_pctAvailCap < m_pCurrentUxScenario->m_expandTrigger)   // decimal percent
         {
         ExpandUGA(pUGA, pContext, pLayer);
         }
      }
   return true;
   }


bool Developer::ExpandUGA(UGA* pUGA, EnvContext* pContext, MapLayer* pLayer)
   {
   // time to expand this UGA.  Basic idea is to:
   //  1) determine the area needed to accommodate new growth
   //  2) pull items of the priority list to add to the UGA until area requirement met
   MapLayer* pIDULayer = (MapLayer*)pContext->pMapLayer;

   float startingArea = pUGA->m_currentArea;

   float expPop = 20 * pUGA->m_estGrowthRate*pUGA->m_currentPopulation;    // assume 20 year land supply
   float resExpArea = expPop / (pUGA->m_newResDensity * pUGA->m_ppdu * ACRE_PER_M2);   // m2
   float commExpArea = resExpArea / pUGA->m_resCommRatio;         // add in commercial area

   float resArea = 0;
   for (int i = pUGA->m_nextResPriority; i < pUGA->m_priorityListRes.GetSize(); i++)
      {
      pUGA->m_nextResPriority++;

      // have we annexed enough area? then stop
      if (resArea >= resExpArea)
         break;  // all done

      // get the next IDU on the priority list
      UGA_PRIORITY* pPriority = pUGA->m_priorityListRes[i];

      int uga;
      pLayer->GetData(pPriority->idu, m_colUga, uga);

      // has this idu already been annexed?  Then skip it
      if (uga > 0)
         continue;

      // expand to nearby IDUs
      // Inputs:
      //   idu = index of the kernal (nucleus) polygon
      //   pPatchQuery = the column storing the value to be compared to the value
      //   colArea = column containing poly area.  if -1, areas are ignored, including maxExpandArea; only the expand array is populated
      //   maxExpandArea = upper limit expansion size, if a positive value.  If negative, it is ignored and the max patch size is not limited
      //
      // returns: 
      //   1) area of the expansion area (NOT including the nucleus polygon) and 
      //   2) an array of polygon indexes considered for the patch (DOES include the nucelus polygon).  Zero or Positive indexes indicate they
      //      were included in the patch, negative values indicate they were considered but where not included in the patch
      //------------------------------------------------------------------------------------------------------------------------------------
      // the max expansion area is the lesser of the size of the needed land area and 100ha
      float maxExpandArea = std::fminf(1000000.0f, resExpArea); 
      // target area
      CArray< int, int > expandArray;
      float expandArea = pIDULayer->GetExpandPolysFromQuery(pPriority->idu, pUGA->m_pResQuery, m_colArea,
                  maxExpandArea, expandArray);

      // add in kernal IDU area to accumlating total
      float area = 0;
      pLayer->GetData(pPriority->idu, m_colArea, area);
      resArea += area;
      pUGA->m_resExpArea += area;
      pUGA->m_totalExpArea += area;

      // annex all expanded polygons
      for (INT_PTR j = 0; j < expandArray.GetSize(); j++)
         {
         if (expandArray[j] >= 0)
            {
            pLayer->GetData(expandArray[j], m_colArea, area);
            resArea += area;
            pUGA->m_resExpArea += area;
            pUGA->m_totalExpArea += area;
            UpdateIDU(pContext, expandArray[j], m_colUga, pUGA->m_id, ADD_DELTA);                  // add to UGA
            UpdateIDU(pContext, expandArray[j], m_colUxEvent, pUGA->m_currentEvent, ADD_DELTA);    // indicate expansion event
            UpdateIDU(pContext, expandArray[j], m_colZone, pUGA->m_zoneRes, ADD_DELTA);
            }
         }
      }

   float commArea = 0;
   for (int i = pUGA->m_nextCommPriority; i < pUGA->m_priorityListComm.GetSize(); i++)
      {
      pUGA->m_nextCommPriority++;
      
      // have we annexed enough area? then stop
      if (commArea >= commExpArea)
         break;  // all done

      // get the next IDU on the priority list
      UGA_PRIORITY* pPriority = pUGA->m_priorityListComm[i];

      int uga;
      pLayer->GetData(pPriority->idu, m_colUga, uga);

      // has this idu already been annexed?  Then skip it
      if (uga > 0)
         continue;

      // expand to nearby IDUs
      // Inputs:
      //   idu = index of the kernal (nucleus) polygon
      //   pPatchQuery = the column storing the value to be compared to the value
      //   colArea = column containing poly area.  if -1, areas are ignored, including maxExpandArea; only the expand array is populated
      //   maxExpandArea = upper limit expansion size, if a positive value.  If negative, it is ignored and the max patch size is not limited
      //
      // returns: 
      //   1) area of the expansion area (NOT including the nucleus polygon) and 
      //   2) an array of polygon indexes considered for the patch (DOES include the nucelus polygon).  Zero or Positive indexes indicate they
      //      were included in the patch, negative values indicate they were considered but where not included in the patch
      //------------------------------------------------------------------------------------------------------------------------------------
      // the max expansion area is the lesser of the size of the needed land area and 100ha
      float maxExpandArea = std::fminf(1000000.0f, commExpArea);
      // target area
      CArray< int, int > expandArray;
      float expandArea = pIDULayer->GetExpandPolysFromQuery(pPriority->idu, pUGA->m_pCommQuery, m_colArea,
         maxExpandArea, expandArray);

      // add in kernal IDU area to accumlating total
      float area = 0;
      pLayer->GetData(pPriority->idu, m_colArea, area);
      commArea += area;
      pUGA->m_commExpArea += area;
      pUGA->m_totalExpArea += area;

      // annex all expanded polygons
      for (INT_PTR j = 0; j < expandArray.GetSize(); j++)
         {
         if (expandArray[j] >= 0)
            {
            pLayer->GetData(expandArray[j], m_colArea, area);
            commArea += area;
            pUGA->m_commExpArea += area;
            pUGA->m_totalExpArea += area;
            UpdateIDU(pContext, expandArray[j], m_colUga, pUGA->m_id, ADD_DELTA);                  // add to UGA
            UpdateIDU(pContext, expandArray[j], m_colUxEvent, pUGA->m_currentEvent, ADD_DELTA);    // indicate expansion event
            UpdateIDU(pContext, expandArray[j], m_colZone, pUGA->m_zoneComm, ADD_DELTA);
            }
         }
      }

   float totalExpAreaAc = (resExpArea + commExpArea) * ACRE_PER_M2;
   float totalAreaAc = (resArea + commArea) * ACRE_PER_M2;
   startingArea *= ACRE_PER_M2;

   CString msg;
   msg.Format("Developer:  UGA Expansion Event for %s:  Demand: %.0f acres, Achieved %.0f acres (from %.0f to %.0f acres, Event=%i)", 
      pUGA->m_name, totalExpAreaAc, totalAreaAc, startingArea, startingArea+totalAreaAc, pUGA->m_currentEvent);
   Report::Log(msg);
   pUGA->m_currentEvent++;

   return true;
   }

float Developer::UpdateUGAStats( EnvContext *pContext, bool outputStartInfo )
   {
   if ( m_pCurrentUxScenario == NULL )
      return 0;

   INT_PTR ugaCount = m_pCurrentUxScenario->m_uxArray.GetSize();

   float *curPopArray = new float[ ugaCount ];

   // reset all 
   for ( INT_PTR i=0; i < ugaCount; i++ )
      {
      UGA *pUGA = m_pCurrentUxScenario->m_uxArray[ i ];
      
      curPopArray[ i ] = pUGA->m_currentPopulation;

      pUGA->m_currentArea = 0;             // m2
      pUGA->m_currentResArea = 0;          // m2
      pUGA->m_currentCommArea = 0;         // m2
      pUGA->m_currentPopulation = 0;       // people
      pUGA->m_newPopulation = 0;           // people
      pUGA->m_popIncr = 0;                 // people
      pUGA->m_capacity = 0;                // people
      pUGA->m_pctAvailCap = 0;             // decimal percent
      pUGA->m_avgAllowedDensity = 0;       // du/ac
      pUGA->m_avgActualDensity = 0;        // du/ac
      }

   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   
   float initPop = 0;

   // iterate through the DUArea layer, computing capacities for each active DUArea
   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      int uga = -1;
      pLayer->GetData( idu, m_colUga, uga );
         
      // outside a UGA?
      if (uga <= 0)
         continue;

      UGA *pUGA = FindUGAFromID( uga );
      if ( pUGA == NULL )
         {
         CString msg;
         msg.Format( "Unable to find UGA Expansion: %i - bad IDU value for IDU %i", uga, (int) idu );
         Report::Log( msg );
         continue;
         }

      if ( pUGA->m_use == false )
         continue;

      // have UGA, update stats
      float area = 0;
      pLayer->GetData( idu, m_colArea, area );
      pUGA->m_currentArea += area;   // m2

      int zone = 0;
      pLayer->GetData( idu, m_colZone, zone );
      if ( IsResidential( zone ) )
         pUGA->m_currentResArea += area;   // m2
      else if ( IsCommercial( zone ) )
         pUGA->m_currentCommArea += area; // m2

      float popDens = 0;         // people/m2
      pLayer->GetData( idu, m_colPopDens, popDens );
      float pop = popDens * area;
      pUGA->m_currentPopulation += pop;    // people

      float popCap = 0;          // people (not density)
      pLayer->GetData( idu, m_colPopCap, popCap );
      if ( popCap > 0 )
         pUGA->m_capacity += popCap;
         
      float popDens0 = 0;         // people/m2
      pLayer->GetData( idu, m_colPopDensInit, popDens0 );
      initPop += popDens0 * area;

      pUGA->m_newPopulation += (( popDens-popDens0 ) * area );    // people

      float allowedDens = GetAllowedDensity( pLayer, idu, zone );  // du/ac
      float ppdu = pUGA->m_ppdu;
      allowedDens *= ppdu/M2_PER_ACRE;

      pUGA->m_avgAllowedDensity += allowedDens * area;   //#
      }

   // normalize as needed
   for ( INT_PTR i=0; i < ugaCount; i++ )
      {
      UGA *pUGA = m_pCurrentUxScenario->m_uxArray[ i ];

      pUGA->m_pctAvailCap = 1-( pUGA->m_currentPopulation/pUGA->m_capacity );     // decimal percent
      
      float peoplePerDU = pUGA->m_ppdu;

      float allowedDens = ( pUGA->m_avgAllowedDensity / pUGA->m_currentArea ) * ( M2_PER_ACRE / peoplePerDU );
      pUGA->m_avgAllowedDensity = allowedDens;     // du/ac

      float actualDens = ( pUGA->m_currentPopulation / pUGA->m_currentArea ) * ( M2_PER_ACRE / peoplePerDU );
      pUGA->m_avgActualDensity = actualDens;        // du/ac

      if ( outputStartInfo && pContext->yearOfRun == 0 )
         {
         CString msg;
         msg.Format( "  Starting Population for %s: %i",
                  (LPCTSTR) pUGA->m_name, (int) pUGA->m_startPopulation );
         Report::Log( msg );
         }

      if ( pContext->yearOfRun > 0 )
         pUGA->m_popIncr = pUGA->m_currentPopulation - curPopArray[ i ];  // calculate population added this step
      
      if (pUGA->m_computeStartPop)
         {
         pUGA->m_startPopulation += pUGA->m_currentPopulation;
         pUGA->m_computeStartPop = false;
         }
      }

   delete [] curPopArray;

   return 0;
   }

bool Developer::PrioritizeUxAreas( EnvContext *pContext )
   {
   // basic idea:
   // populates two required columns:
   //   1) m_colUxEvent, which contains the event id indicate the order of IDU was expanded into
   //   2) m_colUxPriority, which contains the priority of expansion for the given IDU
   // It does this by:
   //   For the current scenario, for each UGA,
   //   1) Find all expansion areas IDUs for the UGA.  These are indicated by a negative value in the m_colUga (UGA_ID) column
   //   2) Rank by increasing distance from the existing UGA boundary  (well, really, this just looks up the priority in the IDU)
   //      This becomes the priority list for the scenario, one for commercial, one for residential.  This is sorted arrays.
   //   3) The highest priority IDU idu is tagged with 0, the next with 1, etc... in column U_PRIORITY
   // At this point, the prioritized res/commercial lists can be used to expand UGAs

   if ( m_pCurrentUxScenario == NULL )
      return false;

   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   ASSERT( m_colUga >= 0 );
   ASSERT( m_colUxNearUga >= 0);
   ASSERT( m_colUxNearDist >= 0);
   ASSERT( m_colUxEvent >= 0 );
   ASSERT( m_colUxPriority >= 0 );

   // set appropriate columns to NODATA
   pLayer->SetColNoData( m_colUxEvent );
   pLayer->SetColNoData( m_colUxPriority );

   int count = 0;
   int resOrCommCount = 0;
   int resOrCommMultiplier = 2;
   bool doingRes = true;

   // iterate through the IDU layer, looking for appropriate expansion polys
   for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
      {
      int uga = -1;
      pLayer->GetData( idu, m_colUga, uga );   // negative of UGA code if in an expansion area
      if ( uga > 0 )   // skip if inside existing DUArea
         continue;

      // get the nearest UGA id
      int nearUGA = -1;
      pLayer->GetData(idu, m_colUxNearUga, nearUGA);

      if (nearUGA < 0)  // skip if no nearUGA
         continue;

      float nearDist = 0;
      pLayer->GetData(idu, m_colUxNearDist, nearDist);
      
      if (nearDist > 3000)   // ignore IDUs greater than 3km from an existing UGA
         continue;

      // we now know the associated UGA id and distance,
      // use the distance to prioritize new UGA idus

      // get the associated UGA record
      UGA *pUGA = FindUGAFromID( nearUGA );
      if ( pUGA == NULL )
         continue;
      if ( pUGA->m_use == false )
         continue;

      ///////
      //int lulcB, lulcA;
      //int col = pLayer->GetFieldCol("LULC_B");
      //pLayer->GetData(idu, col, lulcB);
      //col = pLayer->GetFieldCol("LULC_A");
      //pLayer->GetData(idu, col, lulcA);
      //
      ///////

      // get area from the IDU coverage
      float area = 0;
      pLayer->GetData( idu, m_colArea, area );

      // does it pass constraints?
      // first residential
      bool passRes = false;
      if ( pUGA->m_pResQuery == NULL )
         passRes = true;
      else
         {
         bool result = false;
         bool ok = pUGA->m_pResQuery->Run( idu, result );
         if ( ok && result )
            passRes = true;
         }

      // second commercial/industrial
      bool passComm = false;
      if ( pUGA->m_pCommQuery == NULL )
         passComm = true;
      else
         {
         bool result = false;
         bool ok = pUGA->m_pCommQuery->Run( idu, result );
         if ( ok && result )
            passComm = true;
         }

      if ( passRes == false && passComm == false )
         continue;

      // get priority from DUArea Expansion coverage
      //float priority = 0;
      float priority = nearDist;
      //bool ok = pLayer->GetData(idu, m_colUxPriority, priority);
      //ASSERT( ok );
      
      //pLayer->SetData( idu, m_colUga, -nearestUgb );   // for expansion areas, set to the negative of the associated DUArea
      
      // NOTE:  need conflict resolution strategy!  Below assumes residential always wins
      // Note: there are three options - 
      //   1) idu passes comm and res - apply conflict resolution strategy
      //   2) idu passes comm only - add to comm list
      //   3) idu passes res only - add to res list
      resOrCommCount++;
      if ( passRes && passComm )   // case 1
         {
         if ( doingRes )
            {
            pUGA->m_priorityListRes.Add( new UGA_PRIORITY( idu, priority ) );
         
            if ( resOrCommCount > pUGA->m_resCommRatio * resOrCommMultiplier )
               {     // reset and switch to comm
               resOrCommCount = 0;
               doingRes = false;
               }
            }
         else // doingRes == false
            {
            pUGA->m_priorityListComm.Add( new UGA_PRIORITY( idu, priority ) );
            
            if ( resOrCommCount > resOrCommMultiplier )
               {  // reset and switch to res
               doingRes = true;
               resOrCommCount = 0;
               }
            }
         }
      else if ( passRes )   // case 2
         {
         resOrCommCount++;
         pUGA->m_priorityListRes.Add( new UGA_PRIORITY( idu, priority ) );
         }
      else if ( passComm ) // case 3
         {
         resOrCommCount++;
         pUGA->m_priorityListComm.Add( new UGA_PRIORITY( idu, priority ) );
         }
      
      //if ( passRes  )
      //   pUGA->m_priorityListRes.Add( new UGA_PRIORITY( idu, priority ) );
      //else
      //   pUGA->m_priorityListComm.Add( new UGA_PRIORITY( idu, priority ) );

      count++;
      }  // end of: MapLayer::Iterator

   // priority lists populate, now sort to prioritize

   // sort distances
   for ( int j=0; j < m_pCurrentUxScenario->m_uxArray.GetSize(); j++ )
      {
      UGA *pUGA = m_pCurrentUxScenario->m_uxArray[ j ];
      if ( pUGA->m_use == false )
         continue;

      // first res
      UGA_PRIORITY **ugaDist = pUGA->m_priorityListRes.GetData();
      qsort( (void*) ugaDist, pUGA->m_priorityListRes.GetSize(), sizeof( INT_PTR ), CompareUGAPriorities );
      
      for ( int k=0; k < pUGA->m_priorityListRes.GetSize(); k++ )
         {
         UGA_PRIORITY *pUD = pUGA->m_priorityListRes.GetAt( k );
         pLayer->SetData( pUD->idu, m_colUxPriority, k ); 
         }

      // second comm/ind
      ugaDist = pUGA->m_priorityListComm.GetData();
      qsort( (void*) ugaDist, pUGA->m_priorityListComm.GetSize(), sizeof( INT_PTR ), CompareUGAPriorities );
      
      for ( int k=0; k < pUGA->m_priorityListComm.GetSize(); k++ )
         {
         UGA_PRIORITY *pUD = pUGA->m_priorityListComm.GetAt( k );
         pLayer->SetData( pUD->idu, m_colUxPriority, k ); 
         }
      }

   CString msg;
   msg.Format( "Finished Prioritizing %i UGBs, %i IDUs", (int) m_duAreaArray.GetSize(), count );
   Report::Log( msg );

   return true;
   }


bool Developer::IsResidential( int zone )
   { 
   for ( int i=0; i < (int) m_resZoneArray.GetSize(); i++ ) 
      if ( zone == m_resZoneArray[ i ]->m_id )
         return true;

   return false;
   }

bool Developer::IsCommercial( int zone )
   { 
   for ( int i=0; i < (int) m_commZoneArray.GetSize(); i++ ) 
      if ( zone == m_commZoneArray[ i ]->m_id )
         return true;

   return false;
   }



// returns allowed density in people/m2.  Note:  If idu<0, then density is based on passed in zone.
//  otherwise, zone is retrieved from the IDU and the "zone" argument is ignored

float Developer::GetAllowedDensity( MapLayer *pLayer, int idu, int zoneID )
   {
   float allowedDens = 0;     // du/acre

   // get zone info
   ZoneInfo *pZone = NULL;

   for ( int i=0; i < (int) m_resZoneArray.GetSize(); i++ )
      {
      if ( m_resZoneArray[ i ]->m_id == zoneID )
         {
         pZone = m_resZoneArray[ i ];
         break;
         }
      }

   if ( pZone == NULL )
      {
      for ( int i=0; i < (int) m_commZoneArray.GetSize(); i++ )
         {
         if ( m_commZoneArray[ i ]->m_id == zoneID )
            {
            pZone = m_commZoneArray[ i ];
            break;
            }
         }
      }

   if ( pZone == NULL )
      return 0;

   // pass constraints?
   if ( pZone->m_pQuery != NULL )
      {
      bool passed = false;
      bool ok = pZone->m_pQuery->Run( idu, passed );

      if ( ok && passed )
         return pZone->m_density;
      else
         return 0;      
      }

   return pZone->m_density;
   }


int CompareUGAPriorities(const void *elem0, const void *elem1 )
   {
   UGA_PRIORITY *p0 = *((UGA_PRIORITY**) elem0 );
   UGA_PRIORITY *p1 = *((UGA_PRIORITY**) elem1 );

   return ( p0->priority < p1->priority ) ? -1 : 1;
   }
