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
// Population.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#pragma hdrstop

#include <maplayer.h>
#include <DeltaArray.h>

#include "Target.h"
#include "TargetDlg.h"

#include <tixml.h>
#include <PathManager.h>

#include <queryEngine.h>
//#include <randgen/randunif.hpp>
#include <unitconv.h>
#include <math.h>
#include <FDATAOBJ.H>
#include <envcontext.h>
#include <envmodel.h>
#include <limits.h>


using std::string;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

TargetProcess* gTargetProcess = nullptr;

extern "C" _EXPORT EnvExtension* Factory(EnvContext*)
   {
   if ( gTargetProcess == nullptr)
      gTargetProcess = new TargetProcess; 

   return (EnvExtension*) gTargetProcess;
   }


ALLOCATION::~ALLOCATION()
   {
   /*if ( pParser ) delete pParser;*/
   if ( pQuery != nullptr && pTarget->m_pQueryEngine != nullptr )
      pTarget->m_pQueryEngine->RemoveQuery( pQuery );

   if ( parserVars != nullptr )
      delete parserVars; 
   }

PREFERENCE::~PREFERENCE()
   {
   /*if ( pParser ) delete pParser;*/
   //if ( pQuery != nullptr && pTarget->m_pQueryEngine != nullptr )
   //   pTarget->m_pQueryEngine->RemoveQuery( pQuery );

   //if ( parserVars != nullptr )
   //   delete parserVars; 
   }


TargetReport::~TargetReport()
   {
    if ( m_pQuery != nullptr && m_pTarget->m_pQueryEngine != nullptr )
       m_pTarget->m_pQueryEngine->RemoveQuery(m_pQuery );

  } 


ALLOCATION *AllocationSet::AddAllocation( LPCTSTR name, Target *pTarget, LPCTSTR query, LPCTSTR expr, float multiplier )
   {
   ALLOCATION *pAllocation = new ALLOCATION( name, pTarget, query, expr, multiplier );
   ASSERT( pAllocation != nullptr );

   CArray::Add( pAllocation );

   return pAllocation;
   }


PREFERENCE *AllocationSet::AddPreference( LPCTSTR name, Target *pTarget, LPCTSTR query, float value )
   {
   PREFERENCE *pPref = m_preferences.Add( name, pTarget, query, value );

   // process query
   if ( query != nullptr )
      {
      CString source( "Allocation Preference '" );
      source += name;
      source += "'";

      Query *pQuery = pPref->pTarget->m_pQueryEngine->ParseQuery( query, 0, source );
      if ( pQuery )    // successful compile?
         {            
         pQuery->m_text = query;
         pPref->pQuery = pQuery;
         }
      else
         {
         CString msg( "Unable to compile allocation query: " );
         msg += query;
         Report::LogWarning( msg );
         pPref->pQuery = nullptr;
         }
      }

   return pPref;
   }



bool ALLOCATION::Compile( Target *pTarget, const MapLayer *pLayer )
   {
   // process query
   if ( ! queryStr.IsEmpty() )
      {
      CString source( "Target Allocation '" );
      source += pTarget->m_name;
      source += "' Allocation '";
      source += this->name;
      source += "'";
      
      pQuery = pTarget->m_pQueryEngine->ParseQuery( queryStr, 0, source );
      if ( pQuery )    // successful compile?
         pQuery->m_text = queryStr;
      else
         {
         CString msg( "Unable to compile allocation query: " );
         msg += queryStr;
         Report::LogWarning( msg );
         pQuery = nullptr;
         }
      }

   // take care of compiling value formula
   // global constants first
   for (int i = 0; i < pTarget->m_pProcess->m_constArray.GetSize(); i++)
      {
      Constant *pConst = pTarget->m_pProcess->m_constArray[i];
      parser.defineConst(pConst->m_name, pConst->m_value);
      }

   // then local (Target) constants
   for (int i = 0; i < pTarget->m_constArray.GetSize(); i++)
      {
      Constant *pConst = pTarget->m_constArray[i];
      parser.defineConst(pConst->m_name, pConst->m_value);
      }


   // compile this formula
   try 
      {
      ASSERT( parserVars == nullptr ); 

      parser.enableAutoVarDefinition( true );
      parser.compile( expression );

      parserVarCount = parser.getNbUsedVars();
      if ( parserVarCount > 0 )
         {
         parserVars = new PARSER_VAR[ parserVarCount ];

         for ( UINT i=0; i < parserVarCount; i++ )
            {
            std::string str = parser.getUsedVar( i );
            LPCTSTR cstr = str.c_str();
            CString var( cstr );

            int col = pLayer->GetFieldCol( var );
            if ( col < 0 )
               {
               CString msg( _T("Target: Unrecognized variable found: '") );
               msg += var;
               msg += "' - Target will not be able to continue...";

               Report::LogError( msg );
               break;
               }
            
            parserVars[ i ].col = col;
            parser.redefineVar( var, &(parserVars[i].value));
            }
        }
      }
   catch (...)
      {
      CString msg( _T("Error encountered parsing target allocation 'value' expression: ") );
      msg += expression;
      msg += _T("... setting to zero" );
      Report::LogWarning( msg );

      parser.compile( _T("0") );
      }
   
   return true;
   }




///////////////////////////////////////////////////
// Target class methods
///////////////////////////////////////////////////


Target::Target( TargetProcess *pProcess, int id, LPCTSTR name ) 
: m_name( name )
, m_modelID( id )
, m_startTotalActualValue( 0 )
, m_lastTotalActualValue( 0 )
, m_curTotalActualValue( 0 )
, m_curTotalTargetValue( 0 )
, m_totalCapacity( 0 )
, m_totalAvailCapacity( 0 )
, m_totalModifiedAvailCapacity( 0 )
, m_totalOverCapacity( 0 )
, m_totalPercentAvailable( 0 )
, m_totalAllocated( 0 )
, m_rate( 0.05 )
, m_method( TM_UNDEFINED )
, m_areaUnits( -1 )
, m_pTargetData( nullptr )
, m_targetData_year0( 0 )
, m_colArea( -1 )
, m_colTarget( -1 )
, m_colDensXarea( -1 )
, m_colTargetBin(-1)
, m_colStartTarget( -1 )
, m_colTargetCapacity( -1 )
, m_colCapDensity( -1 )
, m_colAvailCapacity( -1 )
, m_colAvailDens( -1 )
, m_colPctAvailCapacity( -1 )
, m_colPrefs( -1 )
, m_activeAllocSetID( -1 )
, m_pQueryEngine(nullptr)
, m_pQuery( nullptr )
, m_inVarStartIndex( -1 )
, m_outVarStartIndex( -1 )
, m_outVarCount( 0 )
, m_estimateParams( false )
, m_pProcess( pProcess )
   { }


Target::~Target()
   {
   for ( int i=0; i < m_constArray.GetSize(); i++ )
      delete m_constArray.GetAt( i );

   m_constArray.RemoveAll();

   if ( m_pTargetData != nullptr )
      delete m_pTargetData;
   }



// note: this gets called during APInit()
bool Target::Init( EnvContext *pEnvContext )
   {
   m_curTotalActualValue = GetCurrentActualValue( pEnvContext );
   m_startTotalActualValue = m_lastTotalActualValue = m_curTotalActualValue;   

   // calculates total value for capacities, populated IDUs 

   if (this->m_pQuery != nullptr) 
      {
      int count = m_pQueryEngine->SelectQuery(this->m_pQuery, true);
      CString msg;
      msg.Format("Target %s had %i qualifying IDUs", this->m_name, count);
      Report::LogInfo(msg);
      }

   PopulateCapacity( pEnvContext, false );

   // initialize TargetReport variables
   for ( int i=0; i < m_reportArray.GetSize(); i++ )
      {
      TargetReport *pReport = m_reportArray[ i ];
      ASSERT( pReport != nullptr );
      pReport->m_count = 0;
      pReport->m_fraction = 0;
      }

   if ( this->m_pQuery != nullptr )
      ((MapLayer*) pEnvContext->pMapLayer)->ClearSelection();

   // generate initial report
   CString msg;
   for (int i = 0; i < m_allocationSetArray.GetSize(); i++)
      {
      AllocationSet *pAllocationSet = m_allocationSetArray.GetAt(i);

      msg.Format(_T("   Allocation Set %s (%i): Initial Allocation Values\n"), (LPCTSTR)pAllocationSet->m_name, pAllocationSet->m_id);
      Report::LogInfo( msg );

     float totalValue = 0;
     float totalCapacity = 0;
      for (int j = 0; j < pAllocationSet->GetSize(); j++)
         {
         ALLOCATION *pAlloc = pAllocationSet->GetAt(j);

         //msg.Format( _T("      %s - Current: %g, Capacity: %g, Area (ha): %.0g\n"), (LPCTSTR) pAlloc->name, pAlloc->currentValue, pAlloc->currentCapacity, pAlloc->currentArea/ M2_PER_HA );
         //Report::LogInfo( msg );
         totalValue += pAlloc->currentValue;
         totalCapacity += pAlloc->currentCapacity;
         }
     msg.Format(_T("      Total: Current: %g, Capacity: %g\n"), totalValue, totalCapacity);
     Report::LogInfo(msg);
   }

   //msg.Format( _T("   Starting %s: %g, Capacity: %g, Available: %g, Over Capacity: %g\n"),
   // (LPCTSTR) m_name, m_startTotalActualValue, m_totalCapacity, m_totalAvailCapacity, m_totalOverCapacity );
   //Report::LogInfo( msg );

   return true;
   }


// note: this gets called during APInitRun()
bool Target::InitRun( EnvContext *pEnvContext )
   {
   m_pQueryEngine = pEnvContext->pQueryEngine;

   if ( this->m_method == TM_TABLE )
      this->LoadTableValues();

   // allocate internal arrays if needed and not already allocated
   if ( m_preferenceArray.size() == 0 )
      {
      for ( int i=0; i < (int) this->m_allocationSetArray.GetSize(); i++ )
         {
         if ( m_allocationSetArray[ i ]->m_preferences.GetCount() > 0 ) 
            {
            int recordCount = pEnvContext->pMapLayer->GetRecordCount();
            m_preferenceArray.resize( recordCount );
            std::fill( m_preferenceArray.begin(),m_preferenceArray.end(), 0.0f);
            break;
            }
         }
      }
   
   // get existing total, target totals
   m_curTotalActualValue = GetCurrentActualValue( pEnvContext );
   m_curTotalTargetValue = GetCurrentTargetValue( pEnvContext,-1);

   // initialize various summary variables
   m_startTotalActualValue = m_lastTotalActualValue = m_curTotalActualValue;   

   // updates targetCapacity, availableCapacity, pctAvailCapacity and IDU-level values
   if (this->m_pQuery != nullptr)
      m_pQueryEngine->SelectQuery(this->m_pQuery, true);

   PopulateCapacity( pEnvContext, false );

   // take care of init_mask if defined
   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;
   if (this->m_colInitMask >= 0 && this->m_pInitMaskQuery != nullptr)
      {
      int count = 0;
      for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
         {
         bool result = false;
         bool ok = this->m_pInitMaskQuery->Run(idu, result);

         if (ok && result)
            {
            pLayer->SetData(idu, this->m_colInitMask, 1);
            count++;
            }
         }
      PopulateCapacity(pEnvContext, false);

      Report::Log_i("Initial Mask excluded %i IDUs", count);
      }

   // initialize TargetReport variables
   for ( int i=0; i < m_reportArray.GetSize(); i++ )
      {
      TargetReport *pReport = m_reportArray[ i ];
      ASSERT( pReport != nullptr );
      pReport->m_count = 0;
      pReport->m_fraction = 0;
      }

   if ( m_pQuery == nullptr )
      ((MapLayer*) pEnvContext->pMapLayer)->ClearSelection();

   AllocationSet* pAllocationSet = GetActiveAllocationSet();
   ASSERT(pAllocationSet != nullptr);

   CString msg;
   msg.Format( _T("%s (%i) - Starting: %g, Capacity: %g, Available: %g, Over Capacity: %g, Available Capacity Fraction: %g\n"),
      (LPCTSTR) m_name, pAllocationSet->m_id, m_startTotalActualValue, m_totalCapacity, m_totalAvailCapacity, 
      m_totalOverCapacity, m_totalAvailCapacity/m_totalCapacity);
   Report::LogInfo( msg );

   return true;
   }


Constant *Target::AddConstant( LPCTSTR name, LPCTSTR expr )
   {
   Constant *pConst = new Constant;
   pConst->m_name = name;
   pConst->m_expression = expr;
   m_constArray.Add( pConst );

   MTParser parser;
   pConst->m_value = parser.evaluate( expr );

   return pConst;
   }


TargetReport *Target::AddReport( LPCTSTR name, LPCTSTR query )
   {
   TargetReport *pReport = new TargetReport( name, this, query );
   m_reportArray.Add( pReport );

   CString source( "Target '" );
   source += this->m_name;
   source += "' Report '";
   source += name;
   source += "'";
   
   Query *pQuery = Target::m_pQueryEngine->ParseQuery( query, 0, source );
   if ( pQuery )    // successful compile?
      {            
      pQuery->m_text = query;
      pReport->m_pQuery = pQuery;
      }
   else
      {
      CString msg( "Unable to compile report query: " );
      msg += query;
      Report::LogWarning( msg );
      pReport->m_pQuery = nullptr;
      }

   return pReport;
   }


//---------------------------------------------------------------------------------------
// Target::PopulateCapacity()
//
// populates Target::m_capacityArray and Target::m_availCapacityArray, updating
// the MapLayer if columns exist.
// It computes m_totalCapacity, m_totalAvailCapacity in the process
//---------------------------------------------------------------------------------------

void Target::PopulateCapacity( EnvContext *pEnvContext, bool useAddDelta )
   {
   const MapLayer *pLayer = pEnvContext->pMapLayer;
   int colCount = pLayer->GetColCount();

   // initialize as needed...
   m_totalCapacity = 0;
   m_totalAvailCapacity = 0;
   m_totalModifiedAvailCapacity = 0;
   m_totalOverCapacity = 0;   

   std::fill(m_capacityArray.begin(), m_capacityArray.end(), (float)LONG_MIN - 1);
   std::fill(m_availCapacityArray.begin(), m_availCapacityArray.end(), (float)LONG_MIN - 1);
   std::fill(m_preferenceArray.begin(), m_preferenceArray.end(), 1.0f);

   // basic idea:  for each allocation specified in the input file:
   //   1) Run the associated spatial query to get IDU's contained within that allocation
   //   2) Execute the allocation formula to compute a non-normalized population capacity surface (spatial distribution).
   //      This surface gives the total population capacity of the landscape.  Note that the capacity 
   //      surface can be modified by preferences
   //   3) Compute the spatially explict growth capacity, the difference between capacity and current population,
   //      taking into account those IDU's that are over capacity.
   //   4) Distribute new population over the landscape based on the growth capacity of each IDU 
   int redundantIDUs = 0;

   AllocationSet *pAllocationSet = GetActiveAllocationSet();
   ASSERT( pAllocationSet != nullptr );

   MapLayer::ML_RECORD_SET iteratorType = MapLayer::ACTIVE;
   if (this->m_pQuery != nullptr)
      iteratorType = MapLayer::SELECTION;
   
   for ( int i=0; i < (int) pAllocationSet->GetCount(); i++ )
      {
      ALLOCATION *pAlloc = pAllocationSet->GetAt( i );
      ASSERT( pAlloc != nullptr );
      pAlloc->ResetCurrent(); 

      int queryCount = 0;

      for ( MapLayer::Iterator _idu=pLayer->Begin(iteratorType); _idu < pLayer->End(iteratorType); _idu++ )
         {
         int idu = (int)_idu;

         if ( pAlloc->pQuery != nullptr )
            {
            bool result = false;
            bool ok = pAlloc->pQuery->Run( idu, result );

            if ( !ok || !result )
               continue;
            }

         // this idu passed the query (if it existed), so continue to run the associated math expression
         queryCount++;
         float area = 0;
         pLayer->GetData(idu, m_colArea, area);
         pAlloc->currentArea += area;

         // first allocation that passes the query wins
         if ( m_availCapacityArray[ idu ] > float( LONG_MIN )  )
            {
            redundantIDUs++;
            //CString msg;
            //msg.Format( _T("Target: redundant IDU (%i) encountered processing allocation query %s\n"), (int) idu, (LPCTSTR) pAlloc->queryStr );
            //TRACE( msg );
            continue;
            }

         // run formula
         try
            {
            // load up this idu's parsers variables
            for ( UINT m=0; m < pAlloc->parserVarCount; m++ )
               {
               int col = pAlloc->parserVars[ m ].col;
               float iduValue = 0;
               pLayer->GetData( idu, col, iduValue );
               pAlloc->parserVars[ m ].value = iduValue;
               }

            double value;
            if ( pAlloc->parser.evaluate( value ) == false )
               value = 0;     // set to zero if evaluation fails

            pAlloc->value = (float) value; // units are "target/area"
            }
         catch ( ... )
            {
            TRACE( _T("Exception thrown evaluating MTParser expression in Target.DLL") );
            }

         float capacity = pAlloc->value * pAlloc->multiplier * area;   // this does NOT have preferences applied
         
         pAlloc->currentCapacity += capacity;  // absolute numbers

         if ( m_colTargetCapacity >= 0 )
            {
            // get existing capacity
            float existingCapacity;
            pLayer->GetData( idu, m_colTargetCapacity, existingCapacity );

            // if update needed, then do it
            if ( existingCapacity != capacity )
               {
               m_pProcess->UpdateIDU( pEnvContext, idu, m_colTargetCapacity, capacity, useAddDelta ? ADD_DELTA : SET_DATA );  // non-density units

               if ( m_colCapDensity >= 0 )
                  {
                  // get existing capacity
                  float area = 0;
                  pLayer->GetData( idu, m_colArea, area );
                  m_pProcess->UpdateIDU( pEnvContext, idu, m_colCapDensity, capacity/area, useAddDelta ? ADD_DELTA : SET_DATA );  // non-density units
                  }
               }
            }

         // compute available capacity
         float targetDensity;   // this is the current density value
         pLayer->GetData( idu, m_colTarget, targetDensity );

         pAlloc->currentValue += targetDensity*area;

         float availCapacity = capacity - ( targetDensity * area );   // capacity (current) - current # = available #, numbers, not density

         if (availCapacity < 0) // over capacity?
            {
            m_totalOverCapacity += -availCapacity;
            availCapacity = 0;
            }

         if ( m_colAvailCapacity >= 0 )  // really, these are deltas...
            {
            // get existing capacity
            float existingAvailCapacity;
            pLayer->GetData( idu, m_colAvailCapacity, existingAvailCapacity);

            if ( existingAvailCapacity != availCapacity)
               m_pProcess->UpdateIDU( pEnvContext, idu, m_colAvailCapacity, availCapacity, useAddDelta ? ADD_DELTA : SET_DATA );  // non-density units (can be negative if overallocated)
            }

         m_capacityArray[ idu ] = capacity;
         m_availCapacityArray[ idu ] = availCapacity;  // only consider postive available capacities

         m_totalCapacity += capacity;
         m_totalAvailCapacity += availCapacity;

         // calculate any preferences to find modified total available capacity
         // calculate any allocation_set preferences
         float modifiedAvailCapacity = availCapacity;   // for this idu
         if ( availCapacity > 0 )
            {
            // apply preferences.  the sum of the preferences is stored in this Target's m_preference array
            for ( int p=0; p < pAllocationSet->m_preferences.GetSize(); p++ )
               {
               PREFERENCE *pPref = pAllocationSet->m_preferences.GetAt( p );
               ASSERT( pPref != nullptr );
               ASSERT(this->m_preferenceArray.size() > 0);

               if ( pPref->pQuery != nullptr )
                  {
                  bool result;
                  bool ok = pPref->pQuery->Run( idu, result );

                  if ( ok && result == true )
                     {
                     float oldPref = this->m_preferenceArray[idu];
                     this->m_preferenceArray[idu] = oldPref * pPref->value;
                     modifiedAvailCapacity *= pPref->value;

                     // evaluate expression
                     // TODO - add expressions
                     }
                  }
               }  // end of: for (preferenceCount)

             m_totalModifiedAvailCapacity += modifiedAvailCapacity;
            }  // end of : if( availCapacity > 0 )
         }  // end of: iterating through IDU's for a target specification and query allocation

         //TRACE("Allocation: %s, IDUs: %i\n", pAlloc->name, queryCount);
      }  // end of:  for (each allocation)


   // populate map with pctAvailCapacity, preference factor if needed
   if ( m_colPctAvailCapacity >= 0 || m_colPrefs >= 0 )
      {
      for ( MapLayer::Iterator _idu=pLayer->Begin(iteratorType); _idu < pLayer->End(iteratorType); _idu++ )
         {
         int idu = (int)_idu;

         // TEMPORAY
         //CString _msg;
         //_msg.Format("Processing IDU %i\n", idu);
         //TRACE(_msg);
         //ASSERT(idu != 70866);

         if ( m_colPctAvailCapacity >= 0 )
            {
            float pctAvailCapacity = 0;

            // only compute capacities if greater than 0 
            if ((m_availCapacityArray[idu] > LONG_MIN) && (m_capacityArray[idu] > 0.0f))
               {
               ASSERT(m_capacityArray[idu] > 0);
               pctAvailCapacity = m_availCapacityArray[idu] / m_capacityArray[idu];

               if (pctAvailCapacity < 0)
                  pctAvailCapacity = 0;
               ASSERT(std::isnan(pctAvailCapacity) == false);
               ASSERT(std::isinf(pctAvailCapacity) == false);
              }

            float curPctAvailCapacity;
            pLayer->GetData(idu, m_colPctAvailCapacity, curPctAvailCapacity );

            if ( curPctAvailCapacity != pctAvailCapacity )
               m_pProcess->UpdateIDU( pEnvContext, idu, m_colPctAvailCapacity, pctAvailCapacity, useAddDelta ? ADD_DELTA : SET_DATA );
            }

         if ( m_colAvailDens >= 0 && m_colCapDensity >= 0 )
            {
            float capDens = 0;
            float targetDens = 0;
            pLayer->GetData(idu, m_colCapDensity, capDens );
            pLayer->GetData(idu, m_colTarget, targetDens );

            float oldAvailDens = 0;
            pLayer->GetData(idu, m_colAvailDens, oldAvailDens );
            
            float availDens = capDens - targetDens;
            if (availDens < 0)
               availDens = 0;

            if ( availDens != oldAvailDens )
               m_pProcess->UpdateIDU( pEnvContext, idu, m_colAvailDens, availDens, useAddDelta ? ADD_DELTA : SET_DATA );
            }

         if ( m_colPrefs >= 0 )
            {
            float curPref;
            pLayer->GetData(idu, m_colPrefs, curPref );

            if ( curPref != m_preferenceArray[idu] )
               m_pProcess->UpdateIDU( pEnvContext, idu, m_colPrefs, m_preferenceArray[idu], useAddDelta ? ADD_DELTA : SET_DATA );
            }
         }  // end of MapLayer iterator
      }  // end of: if ( m_colPctAvailCapacity >= 0 && m_colPrefs >= 0 )

   // at this point, all allocations have been examined, and the following have been computed:
   //   m_capacityArray[ idu ]
   //   m_availCapacityArray[ idu ] 
   //   m_preferenceArray[ idu ]
   //   m_totalCapacity
   //   m_totalAvailCapacity
   //   m_totalModifiedAvailCapacity
   //   m_totalOverCapacity
   if (m_totalCapacity > 0)
      m_totalPercentAvailable = (m_totalAvailCapacity / m_totalCapacity) * 100;
   else
      m_totalPercentAvailable = 0;

   //if (this->m_pQuery != nullptr)
   //   ((MapLayer*)pLayer)->ClearSelection();

   return;
   }  // end of : PopulateCapacity()


void Target::LoadTableValues()
   {
   CStringArray ti;
   int count = ::Tokenize(this->m_targetValues, ".", ti);

   if (count != 2)
      {
      Report::LogWarning("Bad table value");
      return;
      }

   TTable* pTable = this->m_pProcess->GetTTable(ti[0]);
   if (pTable == nullptr)
      {
      CString msg("Target: Couldn't find table ");
      msg += ti[0];      
      Report::LogWarning(msg);
      return;
      }

   this->m_pTargetData->ClearRows();

   //Report::Log_s("Loading target from Table (%s): ", (LPCTSTR) this->m_name);
   float pair[2]={0,0};
   for (int i = 0; i < pTable->GetYearCount(); i++)
      {
      pair[0] = (float)pTable->GetYear(i);
      pair[1] = pTable->Lookup(ti[1], pTable->GetYear(i));
      this->m_pTargetData->AppendRow(pair, 2);

      //CString msg;
      //msg.Format("(%i,%i) ", (int)pair[0], (int)pair[1]);
      //
      //Report::LogInfo(msg, RA_APPEND);
      }
   }



bool Target::Run( EnvContext *pEnvContext )
   {
   // Basic idea:
   //
   // 1) compute a projection of the total needed value of the target variable
   // 2) compute new growth increment by comparing its value to last known target value
   // 3) update IDU-level capacities, available capacities via PopulateCapacities();
   // 4) iterate through the idu's allocating the new growth according by computing the
   //    portion of the growth allocation for an IDU by computing a ratio of 
   //    iduAvailableCapacity/totalAvailable capacity to get this IDU's proportional "share"
   //    of the new growth (ratio*new growth give the total new growth for the IDU), taking
   //    into accout any preference factors for the IDU

   const MapLayer *pLayer = pEnvContext->pMapLayer;

   MapLayer::ML_RECORD_SET iteratorType = MapLayer::ACTIVE;
   if (this->m_pQuery != nullptr)
      {
      iteratorType = MapLayer::SELECTION;
      m_pQueryEngine->SelectQuery(this->m_pQuery, true);
      }

   try
      {
      PopulateCapacity( pEnvContext, true );   // generate basic allocation information, sets selection set in IDUs if target    query defiend

      m_curTotalTargetValue = GetCurrentTargetValue(pEnvContext, -1);

      if ( m_totalAvailCapacity <= 0 )
         {
         CString msg;
         msg.Format( _T("%s: target of %i cannnot be accomodated by available capacity in year %i - no new population will be allocated!!!"), 
            (LPCTSTR) m_name, (int) m_curTotalTargetValue, pEnvContext->currentYear);
         Report::LogError( msg);

         return true;
         }

      // all allocations/deltas are processed.  Compute and distribute new target value based on relative available capacity.
      // Note: relative available capacity is the IDU delta/total growth capacity
      
      // get the current estimate of the target
      //m_curTotalTargetValue = GetCurrentTargetValue( pEnvContext,-1 );
      //CString _msg;
      //_msg.Format("Target: %s is ")
      //Report::Log_i("Taget for ")

      // new growth to be allocated is the difernece between what is there vs what is desired
      float newGrowth = m_curTotalTargetValue - m_lastTotalActualValue;   // this is new growth needed to achieve the projection

      int count = pLayer->GetRecordCount();
      //memset( m_allocatedArray, 0, count * sizeof( float ) );  // for each IDU, track allocated population
      std::fill(m_allocatedArray.begin(), m_allocatedArray.end(), 0.0f);

      // iterate through map, computing new densities to sum up the actual values
      this->m_totalAllocated = 0;        // this is the total population allocated across IDUs (#'s, not density)
      this->m_curTotalActualValue = 0;   // reset this, will be accumulating through loop below
      this->m_totalAvailCapacity = 0;
      
      float totalTarget = 0;
      float totalCap = 0;
      float newAllocated = 0;  // temp
      //int countIDUs = 0;

      for ( MapLayer::Iterator idu=pLayer->Begin( iteratorType ); idu < pLayer->End( iteratorType ); idu++ )
         {
         // get existing value of target
         float area = 0;
         float existingDensity = 0;
         pLayer->GetData( idu, m_colArea, area );
         pLayer->GetData( idu, m_colTarget, existingDensity );

         float existingValue =  existingDensity * area;

         // m_curTotalActualValue += existingValue;  // based on existing density
         //countIDUs++;  // why???

         float newValue = existingValue;   // new value for IDU, non-density, start with existing, update below if new growth
         float iduAvailCapacity = m_availCapacityArray[ idu ];  // amount of target that can be added to the idu

         float capacity = 0;
         pLayer->GetData(idu, this->m_colTargetCapacity, capacity);
         totalCap += capacity;

         if ( iduAvailCapacity > 0 )   // any available capacity?
            {
            // apply preference score if defined (must be 0 to inf)
            if ( m_preferenceArray.size() > 0 )
               iduAvailCapacity *= m_preferenceArray[ idu ];

            // get new allocation (non-density) based on available capacity as a portion of total, scaled to new growth
            float iduAlloc = newGrowth * ( iduAvailCapacity / m_totalModifiedAvailCapacity );  // units are "#people"
    
            // if new population allocated to this IDU, then record it on the map
            if ( iduAlloc > 0.0f )
               {
               newAllocated += iduAlloc;

               // update it to the new level
               newValue = existingValue + iduAlloc;   // updated non-density value for IDU
               float newDensity = newValue / area;          // updated density for IDU

               m_pProcess->UpdateIDU(pEnvContext, idu, m_colTarget, newDensity, ADD_DELTA);

               if ( m_colDensXarea >= 0 )
                  m_pProcess->UpdateIDU(pEnvContext, idu, m_colDensXarea, newValue, ADD_DELTA);

               // update available capacities
               if (m_colAvailCapacity >= 0)
                  {
                  float availCap = capacity - newValue;
                  m_pProcess->UpdateIDU(pEnvContext, idu, m_colAvailCapacity, availCap, ADD_DELTA);
                  }
               if (m_colPctAvailCapacity >= 0)
                  {
                  float pctAvailCap = (capacity - newValue) / capacity;
                  m_pProcess->UpdateIDU(pEnvContext, idu, m_colPctAvailCapacity, pctAvailCap, ADD_DELTA);
                  }

               totalTarget += newValue;

               if (m_colTargetBin >= 0)
                  {
                  int bin = 0;
                  if (newDensity < 0.0000625f)     // .25
                     bin = 1;
                  else if (newDensity < 0.000125f)  // .5/ac
                     bin = 2;
                  else if (newDensity < 0.00025f)  // 1/ac
                     bin = 3;
                  else if (newDensity < 0.00125f)  // 5/ac
                     bin = 4;
                  else if (newDensity < 0.0025f)   // 10/ac
                     bin = 5;
                  else if (newDensity < 0.00625f)  // 25/ac
                     bin = 6;  
                  else if (newDensity < 0.000001f)     // .004/ac
                     bin = 0;
                  else
                     bin = 7;

                  m_pProcess->UpdateIDU(pEnvContext, idu, m_colTargetBin, bin, ADD_DELTA);
                  }
               
               // remember the total newly allocated and idu-level newly allocated
               m_totalAllocated += iduAlloc;    // total new IDU allocation
               m_allocatedArray[ idu ] = iduAlloc; // absolute (non-density)
               m_availCapacityArray[idu] = (capacity - newValue);  // absolute (non-density)
               } // end of: if ( iduAlloc > 0.0f)

            float avail = capacity - newValue;
            if (avail > 0)
               m_totalAvailCapacity += avail;
            } // end of: if ( iduAvailCapacity > 0 )

         m_curTotalActualValue += newValue;
         }  // end of: for each IDU

      CString msg;
      msg.Format("%s - Year %i, Target: %i, Actual: %i, Desired Allocation: %i, Actual Allocation: %i, Pct Achieved: %4.1f, Capacity: %.0f, Available Capacity Fraction: %.2f (%.2f)",
         (LPCTSTR)m_name, pEnvContext->yearOfRun, (int)m_curTotalTargetValue, (int)m_curTotalActualValue,
         (int)newGrowth, (int)m_totalAllocated, newGrowth * 100 / m_totalAllocated, totalCap, m_totalAvailCapacity / m_totalCapacity, (totalCap - totalTarget) / totalCap );
      Report::LogInfo(msg);

//      msg.Format( "%s - NewGrowth: %i, NewAllocated: %i, CurActTot:%i, LastActTot:%i, Curr-Last Act: %i \n", 
//         (LPCTSTR)m_name, (int) newGrowth, (int) newAllocated,
//         int (m_curTotalActualValue), int(m_lastTotalActualValue), 
//         int (m_curTotalActualValue-m_lastTotalActualValue) );
//      Report::LogInfo( msg );
//
      m_lastTotalActualValue = m_curTotalActualValue;
      }

   catch( CMemoryException *e )
      {
      char msg[ 128 ];
      e->GetErrorMessage( msg, 128 );
      Report::LogError( msg );
      }

   catch( ... )
      {
      Report::LogError(_T("Unknown exception thrown in Target.dll") );
      }



   pEnvContext->pEnvModel->ApplyDeltaArray((MapLayer*)pLayer);


   // update report variables
   for ( int i=0; i < m_reportArray.GetSize(); i++ )
      {
      TargetReport *pReport = m_reportArray[ i ];
      ASSERT( pReport != nullptr );

      pReport->m_count = 0;
      pReport->m_fraction = 0;

      // was anything allocated?
      if ( m_totalAllocated > 0 )
         {
         // run query
         ASSERT( pReport->m_pQuery != nullptr );

         // iterate through map, computing report values
         for ( MapLayer::Iterator idu=pLayer->Begin( iteratorType ); idu < pLayer->End( iteratorType ); idu++ )
            {
            bool result = false;
            bool ok = pReport->m_pQuery->Run( idu, result );

            if ( ok && result )
               pReport->m_count += m_allocatedArray[ idu ];
            }

         // m_value has units of #allocated in the query/total #allocated
         // e.g. proportion of allocation satisfying the query
         pReport->m_fraction = pReport->m_count/m_totalAllocated;
         }
      }

   AllocationSet *pActiveSet = GetActiveAllocationSet();
   
   float totalAllocCap = 0;
   float totalAllocVal = 0;
   for (int i = 0; i < pActiveSet->GetSize(); i++)
      {
      ALLOCATION *pAlloc = pActiveSet->GetAt(i);
      
      //TRACE("ALLOCATION %s: Capacity=%.1f, Existing=%.1f\n",
      //   (LPCTSTR) pAlloc->name, pAlloc->currentCapacity, pAlloc->currentValue);
      
      totalAllocCap += pAlloc->currentCapacity;
      totalAllocVal += pAlloc->currentValue;
      }

   if (this->m_pQuery != nullptr)
      ((MapLayer*)pLayer)->ClearSelection();
    
   //TRACE("ALLOCATION Total: Capacity=%.1f, Existing=%.1f\n", totalAllocCap, totalAllocVal);
   return true;
   }

   
float Target::GetCurrentTargetValue( EnvContext *pEnvContext, int idu )
   {
   // calculate actual target growth rate.
   float actualProjTarget = 0;  // not density, rather, total value of target
   switch( m_method )
      {
      case TM_RATE_LINEAR:
         actualProjTarget = m_startTotalActualValue * (1 + (float(m_rate)*(pEnvContext->yearOfRun+1))); 
         break;

      case TM_RATE_EXP:
         actualProjTarget = m_startTotalActualValue * (float) exp( float(m_rate)*(pEnvContext->yearOfRun+1));
         break;

      case TM_TIMESERIES:
      case TM_TABLE:
         ASSERT( m_pTargetData != nullptr );
         actualProjTarget =  m_pTargetData->IGet((float) pEnvContext->currentYear, 0, 1, IM_LINEAR);
         break;

      case TM_TIMESERIES_CONSTANT_RATE:
         ASSERT( m_pTargetData != nullptr );
         actualProjTarget =  m_pTargetData->IGet((float) pEnvContext->currentYear, 0, 1, IM_CONSTANT_RATE);
         break;

      default:
         ASSERT( 0 );
      }

   return actualProjTarget;
   }


float Target::GetCurrentActualValue( EnvContext *pEnvContext )
   {
   const MapLayer *pLayer = pEnvContext->pMapLayer; 
   // finished reading file, fix up any remaining issues
   ASSERT( m_colArea >= 0 );

   MapLayer::ML_RECORD_SET iteratorType = MapLayer::ACTIVE;
   if ( this->m_pQuery != nullptr )
      {
      iteratorType = MapLayer::SELECTION;
      m_pQueryEngine->SelectQuery( this->m_pQuery, true );
      }

   float curActualValue = 0;     // actual target is the current popDens * area = # of target
   for ( MapLayer::Iterator idu=pLayer->Begin( iteratorType ); idu < pLayer->End( iteratorType ); idu++ )
      {
      float area;
      float existingDensity; // NOTE: Assumes target is expressed as DENSITY, in units consistent with map area

      pLayer->GetData( idu, m_colArea, area );
      pLayer->GetData( idu, m_colTarget, existingDensity );

      float targetValue = existingDensity * area;

      curActualValue += targetValue;
      }

   float _curActualValue = 0;     // actual target is the current popDens * area = # of target
   for (MapLayer::Iterator _idu = pLayer->Begin(); _idu < pLayer->End(); _idu++)
      {
      float area;
      float existingDensity; // NOTE: Assumes target is expressed as DENSITY, in units consistent with map area

      pLayer->GetData(_idu, m_colArea, area);
      pLayer->GetData(_idu, m_colTarget, existingDensity);

      float targetValue = existingDensity * area;

      _curActualValue += targetValue;
      }

   return curActualValue;
   }


bool Target::EndRun( EnvContext *pEnvContext )
   {
   if ( this->m_estimateParams )
      {
      // for the current allocation set, get queries associated with any preferences
      // to get populations numbers associated with the query area.  These are compared to allocation
      // targets, with the prefernece factors adjusted as needed
      
      AllocationSet *pAllocationSet = GetActiveAllocationSet();
      ASSERT( pAllocationSet != nullptr );
      if ( pAllocationSet->m_preferences.GetSize() == 0 )
         return true;

      // initialize prefs
      for ( int i=0; i < (int) pAllocationSet->m_preferences.GetSize(); i++ )
         {
         PREFERENCE *pPref = pAllocationSet->m_preferences[ i ];
         ASSERT( pPref != nullptr );
         pPref->currentValue = 0;
         }            

      // iterate through the map, getting populates count associated with each preference area
      MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;

      for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
         {
         // get existing value of target
         float area;
         float targetDensity;
         pLayer->GetData( idu, m_colArea, area );
         pLayer->GetData( idu, m_colTarget, targetDensity );
         float existingTarget =  targetDensity * area;

         // have idu info, see if it applies to any of the preferences
         for ( int i=0; i < (int) pAllocationSet->m_preferences.GetSize(); i++ )
            {
            PREFERENCE *pPref = pAllocationSet->m_preferences[ i ];
            
            if ( pPref->pQuery != nullptr )
               {
               bool result;
               bool ok = pPref->pQuery->Run( idu, result );
   
               if ( ok && result == true )
                  pPref->currentValue += existingTarget;
               }
            }
         }  // end of:  for each idu

      // get current target values, compare to targets
      float convergence = 0;
      for ( int i=0; i < (int) pAllocationSet->m_preferences.GetSize(); i++ )
         {
         PREFERENCE *pPref = pAllocationSet->m_preferences[ i ];
         
         float oldValue = pPref->value;
         pPref->value *= pPref->target / pPref->currentValue;

         float delta = ( pPref->currentValue - pPref->target ) / pPref->target; 
         convergence += abs( delta );

         CString msg;
         msg.Format( "Target: Preference %s New Value=%.3g, Old Value=%.3g, Delta=%3.2g", (LPCTSTR) pPref->name, pPref->value, oldValue, delta );
         Report::LogInfo( msg );
         }

      convergence /= pAllocationSet->m_preferences.GetSize();
      CString msg;
      msg.Format( "Target: Preference convergence=%g", convergence );
      Report::LogInfo( msg );
      }  // end of: if ( estimateParams )

   return true;
   }


AllocationSet *Target::GetAllocationSetFromID( int id, int *index /*=nullptr*/ )
   {
   for ( int i=0; i < m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocationSet = m_allocationSetArray.GetAt( i );

      if ( pAllocationSet->m_id == id )
         {
         if ( index != nullptr )
            *index = i;

         return pAllocationSet;
         }
      }

   if ( index != nullptr )
      *index = -1;

   return nullptr;
   }


bool Target::SetQuery( LPCTSTR query )
   {
   // take care of compiling spatial query
   ASSERT( m_pQueryEngine != nullptr );

   // the target has a query - compile it...
   if ( query != nullptr && query[0] != ' ' )
      {
      CString source( "Target '" );
      source += m_name;
      source += "' Query";

      m_pQuery = m_pQueryEngine->ParseQuery( query, 0, source );
      if ( m_pQuery )    // successful compile?
         {            
         m_pQuery->m_text = query;
         }
      else
         {
         CString msg( "Target: Unable to compile target query: " );
         msg += query;
         Report::LogWarning( msg );
         return false;
         }
      }

   return true;
   }



///////////////////////////////////////////////////
// TargetProcess class methods
///////////////////////////////////////////////////

TargetProcess::TargetProcess( ) 
: EnvModelProcess()
, m_id( -1 )
, m_colArea( -1 )
, m_outVarsSoFar( 0 )
, m_initialized( false )
, m_totalActualValue( 0 )
, m_totalTargetValue( 0 )
, m_totalCapacity( 0 )
, m_totalAvailCapacity( 0 )
, m_totalPercentAvailable( 0 )
   { }


TargetProcess::~TargetProcess( void )
   {
   // note: Targets are automatically deleted
   }

bool TargetProcess::SetConfig(EnvContext *pEnvContext, LPCTSTR configStr)
   {
   m_configStr = configStr;

   TiXmlDocument doc;
   doc.Parse(configStr);
   TiXmlElement *pRoot = doc.RootElement();

   bool ok = LoadXml(pRoot,pEnvContext);

   return ok ? TRUE : FALSE;
   }


// this method reads an xml input file
bool TargetProcess::LoadXml(LPCTSTR _filename, EnvContext *pEnvContext)
   {
   MapLayer *pLayer = (MapLayer*)pEnvContext->pMapLayer;

   CString filename;
   if (PathManager::FindPath(_filename, filename) < 0) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format("Target: Input file '%s' not found - this process will be disabled", _filename);
      Report::ErrorMsg(msg);
      return false;
      }

   m_configFile = (LPCTSTR) filename;

   //FileToString(filename, m_configStr);

   // start parsing input file
   TiXmlDocument doc;
   //bool ok = doc.Parse(m_configStr.c_str());
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      CString msg;
      msg.Format("Target: Error reading input file, %s", filename);
      Report::ErrorMsg(msg);
      return false;
      }

   TiXmlElement *pXmlRoot = doc.RootElement();  // <target_process>
   return LoadXml(pXmlRoot, pEnvContext);
   }


bool TargetProcess::LoadXml(TiXmlElement* pXmlRoot, EnvContext* pEnvContext)
   {
   /* start interating through the document
    * general structure is of the form:
    *
    * <?xml version="1.0" encoding="utf-8"?>
    *
    * <target_process>
    *  <target method="rate" value="0.02", col="POPDENS" multiplier="1.0" query="">
    *     <scenario id="1" name='base scenario'  description="description">
    *        <allocation name='' query="" value="" multiplier=''/>
    *         . . .
    *        <allocation name='' query="" value="" mulitplier='' />
    *     </scenario>
    *  </target>
    */

   m_targetArray.RemoveAll();
   m_initialized = false;

   int allocationSetCount = 0;
   int allocationCount = 0;
   int modifierCount = 0;
   int reportCount = 0;
   int constCount = 0;
   int tableCount = 0;

   int modelID = 0;

   // checkCols
   LPCTSTR checkCols = pXmlRoot->Attribute("check_cols");
   if ( checkCols != nullptr)
      {
      CStringArray tokens;
      int count = ::Tokenize(checkCols, _T(",;"), tokens);

      int col = 0;
      for (int i = 0; i < count; i++)
         {
         CStringArray token;
         int _count = ::Tokenize(tokens[i], _T(":"), token);

         TYPE type = TYPE_INT;
         if (_count == 2)
            {
            switch (_tolower(token[1][0]))
               {
               case 'f':   type = TYPE_FLOAT;   break;
               case 'd':   type = TYPE_DOUBLE;  break;
               case 's':   type = TYPE_STRING;  break;
               case 'l':   type = TYPE_LONG;    break;
               }
            }
         this->CheckCol(pEnvContext->pMapLayer, col, token[0], type, CC_AUTOADD);
         }
      }


   // take care of any globally-defined constant expressions
   const TiXmlElement *pXmlGlobalConst = pXmlRoot->FirstChildElement(_T("const"));
   while (pXmlGlobalConst != nullptr)
      {
      LPCTSTR name = pXmlGlobalConst->Attribute(_T("name"));
      LPCTSTR value = pXmlGlobalConst->Attribute(_T("value"));

      if (name == nullptr || value == nullptr)
         {
         CString msg(_T("Error in <const> specification (Global)"));
         msg += _T("); a required attribute (name/value) is missing");
         Report::ErrorMsg(msg);
         continue;
         }

      this->AddConstant(name, value);

      constCount++;
      pXmlGlobalConst = pXmlGlobalConst->NextSiblingElement(_T("const"));
      }

   TiXmlElement* pXmlTable = pXmlRoot->FirstChildElement(_T("table"));
   while (pXmlTable != nullptr)
      {
      LPCTSTR name = pXmlTable->Attribute(_T("name"));
      LPCTSTR years = pXmlTable->Attribute(_T("years"));

      if (name == nullptr || years== nullptr)
         {
         CString msg(_T("Error in <table> specification (Global)"));
         msg += _T("); a required attribute (name/years) is missing");
         Report::ErrorMsg(msg);
         continue;
         }

      TTable* pTable = this->AddTable(name);
      //pTable->m_field = field;
      //pTable->m_col = pEnvContext->pMapLayer->GetFieldCol(field);

      CStringArray tokens;
      int count = ::Tokenize(years, ",", tokens);

      int* _years= new int[count];
      for (int i = 0; i < count; i++)
         _years[i] = atoi(tokens[i]);
      pTable->SetYears(_years, count);
      delete[] _years;

      float* values = new float[count];

      // get individual records
      TiXmlElement* pXmlRecord = pXmlTable->FirstChildElement(_T("record"));
      while (pXmlRecord != nullptr)
         {
         LPCTSTR name = pXmlRecord->Attribute(_T("name"));
         LPCTSTR value= pXmlRecord->Attribute(_T("value"));
         LPCTSTR data = pXmlRecord->Attribute(_T("data"));

         // parse data
         int count = ::Tokenize(data, ",", tokens);
         //int index = atoi(tokens[0]);
         for (int i = 0; i < count; i++)
            values[i] = (float)atof(tokens[i]);

         //VData _name(name);  // TYPE_DSTRING
         pTable->AddRecord(name, atoi(value), values);
         pXmlRecord = pXmlRecord->NextSiblingElement(_T("record"));
         }

      delete[] values;
      tableCount++;
      pXmlTable = pXmlTable->NextSiblingElement(_T("table"));
      }


   TiXmlElement *pXmlTarget = pXmlRoot->FirstChildElement( _T("target") );
   while( pXmlTarget != nullptr )
      {
      LPTSTR name = nullptr;  //
      LPTSTR descr = nullptr; //
      LPTSTR queryStr = nullptr; //
      bool estParams = false;
      LPTSTR method = nullptr;
      LPTSTR value  = nullptr;
      LPTSTR colTarget = nullptr;
      LPTSTR colTargetCapacity = nullptr;
      LPTSTR colAvailCapacity = nullptr;
      LPTSTR colPctAvailCapacity = nullptr;
      LPTSTR colCapDensity = nullptr;
      LPTSTR colDensXarea = nullptr;
      LPTSTR colTargetBin = nullptr;
      LPTSTR colAvailDens = nullptr;
      LPTSTR colArea = nullptr;
      LPTSTR colPrefs = nullptr;
      LPTSTR targetBins = nullptr;
      LPTSTR initMask = nullptr;
      XML_ATTR attrs[] = {
         // attr            type           address                         isReq checkCol
            { "name",         TYPE_STRING,  &name,                      false,  0 },
            { "description",  TYPE_STRING,  &descr,                       false,  0 },
            { "method",       TYPE_STRING,   &method,                      true,  0 },
            { "value",        TYPE_STRING,   &value,                       true,  0 },
            { "col",          TYPE_STRING,   &colTarget,                   true,  0 },
            { "capCol",       TYPE_STRING,   &colTargetCapacity,           false, 0 },
            { "availCol",     TYPE_STRING,   &colAvailCapacity,            false, 0 },
            { "pctAvailCol",  TYPE_STRING,   &colPctAvailCapacity,         false, 0 },
            { "capDensCol",   TYPE_STRING,   &colCapDensity,               false, 0 },
            { "densXareaCol", TYPE_STRING,   &colDensXarea,                false, 0 },
            { "availDensCol", TYPE_STRING,   &colAvailDens,                false, 0 },
            { "binCol",       TYPE_STRING,   &colTargetBin,                false, 0 },
            { "bins",         TYPE_STRING,   &targetBins,                  false, 0 },
            { "areaCol",      TYPE_STRING,   &colArea,                     false, 0 },
            { "prefsCol",     TYPE_STRING,   &colPrefs,                    false, 0 },
            { "query",        TYPE_STRING,   &queryStr,                    false, 0 },
            { "init_mask",    TYPE_STRING,   &initMask,                    false, 0 },
            { "estimate_params", TYPE_BOOL,  &estParams,   false, 0 },
            { nullptr,        TYPE_NULL,     nullptr,                false, 0 } };

      MapLayer *pLayer = (MapLayer*)pEnvContext->pMapLayer;

      bool ok = TiXmlGetAttributes( pXmlTarget, attrs, m_configFile.c_str(), pLayer);

      if (!ok) 
         {
         CString msg(_T("Error in target specification for <target> "));
         msg += name;
         msg += _T("; a required attribute (name/method/value) is missing");
         Report::ErrorMsg(msg);
         return false;
         }

      Target* pTarget = new Target(this, modelID++, "Target");
      if (pTarget == nullptr)
         {
         Report::ErrorMsg("Target: Unable to create Target instance ");
         return false;
         }

      if ( name != nullptr )
         pTarget->m_name = name;
      if ( descr != nullptr)
         pTarget->m_description = descr;
      if ( queryStr != nullptr )
         pTarget->m_queryStr = queryStr;
      if ( estParams == true)
         pTarget->m_estimateParams = estParams;

      pTarget->m_pQueryEngine = pEnvContext->pQueryEngine;

      this->m_targetArray.Add( pTarget );

      EnvExtension::CheckCol( pLayer, pTarget->m_colTarget, colTarget, TYPE_FLOAT, CC_AUTOADD );

      // check the optional columns
      if ( colTargetCapacity )
         {
         EnvExtension::CheckCol( pLayer, pTarget->m_colTargetCapacity, colTargetCapacity, TYPE_FLOAT, CC_AUTOADD );
         pLayer->SetColData( pTarget->m_colTargetCapacity, 0, false );
         }
     
      if (colAvailCapacity)
         EnvExtension::CheckCol( pLayer, pTarget->m_colAvailCapacity, colAvailCapacity, TYPE_FLOAT, CC_AUTOADD );
     
      if (colPctAvailCapacity)
         EnvExtension::CheckCol( pLayer, pTarget->m_colPctAvailCapacity, colPctAvailCapacity, TYPE_FLOAT, CC_AUTOADD );

      if (colCapDensity)
         EnvExtension::CheckCol( pLayer, pTarget->m_colCapDensity, colCapDensity, TYPE_FLOAT, CC_AUTOADD );

      if (colPrefs)
         EnvExtension::CheckCol( pLayer, pTarget->m_colPrefs, colPrefs, TYPE_FLOAT, CC_AUTOADD );

      if ( colArea )
         EnvExtension::CheckCol( pLayer, pTarget->m_colArea, colArea, TYPE_FLOAT, CC_MUST_EXIST );
      else
         pTarget->m_colArea = pLayer->GetFieldCol( "Area" ); 

      if ( colDensXarea )
         {
         EnvExtension::CheckCol( pLayer, pTarget->m_colDensXarea, colDensXarea, TYPE_FLOAT, CC_AUTOADD );
         pLayer->SetColData( pTarget->m_colDensXarea, 0, false );
         }

      if ( colAvailDens )
         {
         EnvExtension::CheckCol( pLayer, pTarget->m_colAvailDens, colAvailDens, TYPE_FLOAT, CC_AUTOADD );
         pLayer->SetColData( pTarget->m_colAvailDens, 0, false );
         }

      if (colTargetBin)
         {
         EnvExtension::CheckCol(pLayer, pTarget->m_colTargetBin, colTargetBin, TYPE_INT, CC_AUTOADD);
         pLayer->SetColData(pTarget->m_colTargetBin, 0, false);
         }

      if ( lstrcmpi( method, _T("rateLinear") ) == 0 )
         pTarget->m_method = TM_RATE_LINEAR;
      else if ( lstrcmpi( method, _T("rateExp") ) == 0 )
         pTarget->m_method = TM_RATE_EXP;
      else if ( lstrcmpi( method, _T("timeseries_constant_rate") ) == 0 )
         pTarget->m_method = TM_TIMESERIES_CONSTANT_RATE;
      else if ( lstrcmpi( method, _T("timeseries") ) == 0 )
         pTarget->m_method = TM_TIMESERIES;
      else if (lstrcmpi(method, _T("table")) == 0)
         pTarget->m_method = TM_TABLE;
      else
         {
         CString msg;
         msg.Format( "Target: Unrecognized 'method' attribute: %s.  This target will be disabled", method );
         Report::LogError( msg);
         pTarget->m_method = TM_UNDEFINED;
         }

      if (initMask != nullptr)
         {
         CStringArray tokens;
         int count = ::Tokenize(initMask, ":", tokens);

         pLayer->CheckCol(pTarget->m_colInitMask, tokens[0], TYPE_INT, CC_AUTOADD);

         if (pTarget->m_colInitMask < 0)
            {
            CString msg;
            msg.Format("Missing Init Mask Column: %s", initMask);
            Report::WarningMsg(msg);
            }
         else
            {
            pTarget->m_initMask = tokens[1];
            pTarget->m_pInitMaskQuery = pTarget->m_pQueryEngine->ParseQuery(tokens[1], 0, "Target Init Mask");
            }
         }

      switch (pTarget->m_method)
         {
         case TM_RATE_LINEAR:
         case TM_RATE_EXP:
            pXmlTarget->Attribute(_T("value"), &(pTarget->m_rate));
            break;

         case TM_TIMESERIES:
         case TM_TIMESERIES_CONSTANT_RATE:
            {
            pTarget->m_targetValues = value;

            ASSERT(pTarget->m_pTargetData == nullptr);
            pTarget->m_pTargetData = new FDataObj(2, 0, U_YEARS);

            TCHAR* targetValues = new TCHAR[lstrlen(value) + 2];
            lstrcpy(targetValues, value);

            TCHAR* nextToken = nullptr;
            LPTSTR token = _tcstok_s(targetValues, _T(",() ;\r\n"), &nextToken);

            float pair[2] = {0,0};
            while (token != nullptr)
               {
               pair[0] = (float)atof(token);
               token = _tcstok_s(nullptr, _T(",() ;\r\n"), &nextToken);
               pair[1] = (float)atof(token);
               token = _tcstok_s(nullptr, _T(",() ;\r\n"), &nextToken);

               pTarget->m_pTargetData->AppendRow(pair, 2);
               }

            delete[] targetValues;

            pTarget->m_targetData_year0 = pTarget->m_pTargetData->GetAsFloat(1, 0);
            ASSERT(pTarget->m_targetData_year0 >= 0);
            }
            break;

         case TM_TABLE:
            {
            // store table info, e.g 'TableName.ReecordName'
            pTarget->m_targetValues = value;

            // allocate 2 col dataobj, year and populate from record
            ASSERT(pTarget->m_pTargetData == nullptr);
            pTarget->m_pTargetData = new FDataObj(2, 0, U_YEARS);

            pTarget->LoadTableValues();
         
            pTarget->m_targetData_year0 = pTarget->m_pTargetData->GetAsFloat(1, 0);
            ASSERT(pTarget->m_targetData_year0 >= 0);
            }
            break;

         default:
            ASSERT( 0 );
         }

      pTarget->SetQuery( pTarget->m_queryStr );

      // get any reports
      const TiXmlElement *pXmlReport = pXmlTarget->FirstChildElement( _T("report") );
      while( pXmlReport != nullptr )
         {
         LPCTSTR name  = pXmlReport->Attribute( _T("name") );
         LPCTSTR query = pXmlReport->Attribute( _T("query") );
         if ( name == nullptr || query == nullptr )
               {
               CString msg( _T("Error in <report> specification (" ) );
               msg += pTarget->m_name;
               msg += _T("); a required attribute (name/id/query) is missing" );
               Report::ErrorMsg( msg );
               continue;
               }

         pTarget->AddReport( name, query );

         reportCount++;
         pXmlReport = pXmlReport->NextSiblingElement( _T("report") );
         }

      // get any outputs (same as reports)
      pXmlReport = pXmlTarget->FirstChildElement( _T("output") );
      while( pXmlReport != nullptr )
         {
         LPCTSTR name  = pXmlReport->Attribute( _T("name") );
         LPCTSTR query = pXmlReport->Attribute( _T("query") );
         if ( name == nullptr || query == nullptr )
               {
               CString msg( _T("Error in <output> specification (" ) );
               msg += pTarget->m_name;
               msg += _T("); a required attribute (name/id/query) is missing" );
               Report::ErrorMsg( msg );
               continue;
               }

         pTarget->AddReport( name, query );

         reportCount++;
         pXmlReport = pXmlReport->NextSiblingElement( _T("output") );
         }

      // take care of any constant expressions
      const TiXmlElement *pXmlConst = pXmlTarget->FirstChildElement( _T("const") );
      while( pXmlConst != nullptr )
         {
         LPCTSTR name  = pXmlConst->Attribute( _T("name") );
         LPCTSTR value = pXmlConst->Attribute( _T("value") );

         if ( name == nullptr || value == nullptr )
            {
            CString msg( _T("Error in <const> specification (" ) );
            msg += pTarget->m_name;
            msg += _T("); a required attribute (name/value) is missing" );
            Report::ErrorMsg( msg );
            continue;
            }

         pTarget->AddConstant( name, value );

         constCount++;
         pXmlConst = pXmlConst->NextSiblingElement( _T("const") );
         }

      // start iterating through children <allocation_set> elements
      const TiXmlElement *pXmlAllocationSet = pXmlTarget->FirstChildElement( _T("allocation_set") );
      if ( pXmlAllocationSet == nullptr )
         Report::ErrorMsg( _T("Error finding <allocation_set> section reading target input file" ) );

      while (pXmlAllocationSet != nullptr)
         {
         LPCTSTR id = pXmlAllocationSet->Attribute(_T("id"));
         LPCTSTR name = pXmlAllocationSet->Attribute(_T("name"));
         LPCTSTR description = pXmlAllocationSet->Attribute(_T("description"));
         LPCTSTR ref = pXmlAllocationSet->Attribute(_T("ref"));

         //if (name == nullptr || id == nullptr)
         //   {
         //   CString msg(_T("Error in <allocation_set> specification ("));
         //   msg += pTarget->m_name;
         //   msg += _T("); a required attribute (name/id) is missing");
         //   Report::ErrorMsg(msg);
         //   continue;
         //   }

         AllocationSet* pAllocationSet = nullptr;
         // is a reference to another allocation defined?
         // then just set a reference to it
         if (ref != nullptr)
            {
            CStringArray tokens;
            int count = ::Tokenize(ref, ".", tokens);
            bool found = false;
            
            for (int i = 0; i < this->GetTargetCount(); i++)
               {
               Target* _pTarget = this->GetTarget(i);
               if (lstrcmpi(tokens[0], _pTarget->m_name) == 0)
                  {
                  for (int j = 0; j < _pTarget->m_allocationSetArray.GetSize(); j++)
                     {
                     AllocationSet* pSet = _pTarget->m_allocationSetArray[j];
                     if (lstrcmpi(pSet->m_name, tokens[1]) == 0)
                        {
                        pTarget->m_allocationSetArray.Add(pSet);
                        allocationSetCount++;
                        pSet->m_refCount++;

                        if (pTarget->m_activeAllocSetID < 0) // first allocation set is the default
                           pTarget->m_activeAllocSetID = pSet->m_id;

                        found = true;
                        break;
                        }
                     }
                  break;
                  }
               }
            if (!found)
               {
               CString msg;
               msg.Format("Allocation set reference %s not found", ref);
               Report::LogWarning(msg);
               }
            }
         else
            {
            pAllocationSet = new AllocationSet;
            pTarget->m_allocationSetArray.Add(pAllocationSet);
            allocationSetCount++;

            pAllocationSet->m_id = atoi(id);
            pAllocationSet->m_name = name;
            pAllocationSet->m_description = description;

            if (pTarget->m_activeAllocSetID < 0) // first allocation set is the default
               pTarget->m_activeAllocSetID = pAllocationSet->m_id;
            // iterate through individual allocations for this set
            const TiXmlElement* pXmlAllocation = pXmlAllocationSet->FirstChildElement(_T("capacity"));

            while (pXmlAllocation != nullptr)
               {
               LPCTSTR _name = pXmlAllocation->Attribute(_T("name"));
               LPCTSTR _query = pXmlAllocation->Attribute(_T("query"));
               LPCTSTR _value = pXmlAllocation->Attribute(_T("value"));
               LPCTSTR _multiplier = pXmlAllocation->Attribute(_T("multiplier"));

               if (_name == nullptr || _value == nullptr) //_query == nullptr || _value == nullptr  )
                  {
                  CString msg(_T("Error in <capacity> specification ("));
                  msg += _name;
                  msg += _T("); a required attribute (name/value) is missing");
                  Report::ErrorMsg(msg);
                  continue;
                  }

               // take care of multiplier if defined
               float multiplier = 1.0f;
               if (_multiplier != nullptr)
                  multiplier = (float)atof(_multiplier);

               ALLOCATION* pAllocation = pAllocationSet->AddAllocation(_name, pTarget, _query, _value, multiplier);
               ASSERT(pAllocation != nullptr);
               pAllocation->Compile(pTarget, pLayer);

               allocationCount++;
               pXmlAllocation = pXmlAllocation->NextSiblingElement(_T("capacity"));
               }

            //////////////////////////////////////////////////////////////////////
            /// the next section is for backward compatibility only!!!!!

            // iterate through individual allocations for this set
            pXmlAllocation = pXmlAllocationSet->FirstChildElement(_T("allocation"));

            while (pXmlAllocation != nullptr)
               {
               LPCTSTR _name = pXmlAllocation->Attribute(_T("name"));
               LPCTSTR _query = pXmlAllocation->Attribute(_T("query"));
               LPCTSTR _value = pXmlAllocation->Attribute(_T("value"));
               LPCTSTR _multiplier = pXmlAllocation->Attribute(_T("multiplier"));

               if (_name == nullptr || _value == nullptr) //_query == nullptr || _value == nullptr  )
                  {
                  CString msg(_T("Error in <allocation> specification ("));
                  msg += _name;
                  msg += _T("); a required attribute (name/value) is missing");
                  Report::ErrorMsg(msg);
                  continue;
                  }

               // take care of multiplier if defined
               float multiplier = 1.0f;
               if (_multiplier != nullptr)
                  multiplier = (float)atof(_multiplier);

               ALLOCATION* pAllocation = pAllocationSet->AddAllocation(_name, pTarget, _query, _value, multiplier);
               ASSERT(pAllocation != nullptr);
               pAllocation->Compile(pTarget, pLayer);

               allocationCount++;
               pXmlAllocation = pXmlAllocation->NextSiblingElement(_T("allocation"));
               }
            ////////////////////////////////////////

            // iterate through preferences for this set
            const TiXmlElement* pXmlPreference = pXmlAllocationSet->FirstChildElement(_T("bias"));

            while (pXmlPreference != nullptr)
               {
               LPCTSTR _name = pXmlPreference->Attribute(_T("name"));
               LPCTSTR _query = pXmlPreference->Attribute(_T("query"));
               LPCTSTR _value = pXmlPreference->Attribute(_T("value"));
               LPCTSTR _target = pXmlPreference->Attribute(_T("target"));

               if (_name == nullptr || _value == nullptr || _query == nullptr)
                  {
                  CString msg(_T("Error in <preference> specification ("));
                  msg += _name;
                  msg += _T("); a required attribute (name/query/value) is missing");
                  Report::ErrorMsg(msg);
                  continue;
                  }

               float value = float(atof(_value));
               PREFERENCE* pModifier = pAllocationSet->AddPreference(_name, pTarget, _query, value);
               ASSERT(pModifier != nullptr);
               modifierCount++;

               if (_target != nullptr)
                  pModifier->target = (float)atof(_target);

               pXmlPreference = pXmlPreference->NextSiblingElement(_T("bias"));
               }
            }
         pXmlAllocationSet = pXmlAllocationSet->NextSiblingElement( _T("allocation_set") );
         }  // end of <allocations>

      // note: because we can't determine the dynamic extents of the following arrays,
      // we have to allocate for the full number of IDUs
      int recordCount = pLayer->GetRecordCount();
 
      pTarget->m_capacityArray.resize(recordCount);
      std::fill(pTarget->m_capacityArray.begin(), pTarget->m_capacityArray.end(), 0.0f);
      
      pTarget->m_availCapacityArray.resize(recordCount);
      std::fill(pTarget->m_availCapacityArray.begin(), pTarget->m_availCapacityArray.end(), 0.0f);

      pTarget->m_allocatedArray.resize(recordCount);
      std::fill(pTarget->m_allocatedArray.begin(), pTarget->m_allocatedArray.end(), 0.0f);

      if ( modifierCount > 0 && pTarget->m_preferenceArray.size() == 0 )
         {
         pTarget->m_preferenceArray.resize( recordCount );
         std::fill(pTarget->m_preferenceArray.begin(), pTarget->m_preferenceArray.end(), 0.0f);
         }
            
      ///if ( Target::m_capacityArray == nullptr )
      ///   {
      ///   Target::m_capacityArray = new float[ recordCount ];
      ///   memset( Target::m_capacityArray, 0, recordCount  * sizeof( float ) );
      ///   }
      ///
      ///if ( Target::m_availCapacityArray == nullptr )
      ///   {
      ///   Target::m_availCapacityArray = new float[ recordCount  ];
      ///   memset( Target::m_availCapacityArray, 0, recordCount  * sizeof( float ) );
      ///   }
      ///
      ///if ( Target::m_allocatedArray == nullptr )
      ///   {
      ///   Target::m_allocatedArray = new float[ recordCount ];
      ///   memset( Target::m_allocatedArray, 0, recordCount  * sizeof( float ) );
      ///   }
      ///
      ///ASSERT( Target::m_capacityArray != nullptr && Target::m_availCapacityArray != nullptr && Target::m_allocatedArray != nullptr );
      ///
      ///if ( modifierCount > 0 && Target::m_preferenceArray == nullptr )
      ///   {
      ///   Target::m_preferenceArray = new float[ recordCount ];
      ///   memset( Target::m_preferenceArray, 0, recordCount  * sizeof( float ) );
      ///   }

      CString msg;
      msg.Format( _T("   Target %s loaded %i reports, %i constants, %i allocation sets, %i preferences with a total of %i allocations\n"), (LPCTSTR) pTarget->m_name, reportCount, constCount, allocationSetCount, modifierCount, allocationCount );
      Report::LogInfo( msg );
      //TRACE( msg );

      pXmlTarget = pXmlTarget->NextSiblingElement( _T("target") );
      }

   return true;
   }


bool TargetProcess::SaveXml( LPCTSTR filename, MapLayer *pLayer )
   {
   // open the file and write contents
   FILE *fp;
   fopen_s( &fp, filename, "wt" );
   if ( fp == nullptr )
      {
      LONG s_err = GetLastError();
      CString msg = "Failure to open " + CString(filename) + " for writing.  ";
      Report::SystemErrorMsg( s_err, msg );
      return false;
      }

   fputs( "<?xml version='1.0' encoding='utf-8' ?>\n\n", fp );

   // settings
   fputs( "<!--\n", fp );
   fputs( "======================================================================================\n", fp );
   fputs( "                               T A R G E T \n", fp );
   fputs( "======================================================================================\n", fp );
   fputs( "Target specifies a landscape variable whose total value across the landscape \n", fp );
   fputs( "is represented as a trajectory, described as a rate or as a time series, and\n", fp );
   fputs( "a set of 'allocations' that describe a target distribution surface in terms of \n", fp );
   fputs( "underlying spatial attributes.\n\n ", fp );
   fputs( "An 'allocation' consists of a spatial query defining the landscape region where\n", fp );
   fputs( "the allocation is targeted, and a target equation that describes, in dimensionless,\n", fp );
   fputs( "relative terms, the portion of the total target allocated to that region. Attributes\n", fp );
   fputs( "of an allocation include:\n", fp );
   fputs( "  name:       a string describing the allocation (e.g. 'population center')\n", fp );
   fputs( "  query:      a spatial query (using Envision spatial query syntax) define where this allocation \n", fp );
   fputs( "              is located in the underlying IDU coverage\n", fp );
   fputs( "  value:      a mathematical expression (supporting IDU field references) that returns a relative\n", fp );
   fputs( "              value of the future target surface.\n", fp );
   fputs( "  multiplier: (optional) a floating point value that scales up (>1) or down (0-1) the value expression\n\n", fp );
   fputs( "Note that if allocation queries (regions) overlap, the one specified FIRST has\n", fp );
   fputs( "precedence over successive allocations.\n\n", fp );
   fputs( "Multiple allocation scenarios can be described for a given target by specifying\n", fp );
   fputs( "multiple <allocations> blocks, each with a unique 'id' (integer) and 'name' attribute.\n", fp );
   fputs( "-->\n\n", fp );
   fputs( "<target_process>\n\n", fp );

   // global constants
   for (int i = 0; i < this->m_constArray.GetSize(); i++)
      {
      Constant *pConst = this->m_constArray[i];
      fprintf(fp, "  <const name='%s' value='%s' /> \n", (LPCTSTR)pConst->m_name, (LPCTSTR)pConst->m_expression);
      }


   for ( int i=0; i < this->m_targetArray.GetSize(); i++ )
      {
      Target *pTarget = m_targetArray[ i ];

      TCHAR *pMethod = nullptr;
      switch ( pTarget->m_method )
         {
         case TM_RATE_LINEAR:
            pMethod = _T("rateLinear" );
            break;
         
         case TM_RATE_EXP:
            pMethod = _T("rateExponential");
            break;
         
         case TM_TIMESERIES_CONSTANT_RATE:
            pMethod = _T( "timeSeries_constant_rate");
            break;
      
         case TM_TIMESERIES:
            pMethod = _T( "timeSeries");
            break;

         case TM_TABLE:
            pMethod = _T("table");
            break;
         }  // end of: switch( pTarget->m_method )
            
      TCHAR *col         = pTarget->m_colTarget >= 0           ? pLayer->GetFieldLabel( pTarget->m_colTarget ) : "" ;
      TCHAR *colCap      = pTarget->m_colTargetCapacity >= 0   ? pLayer->GetFieldLabel( pTarget->m_colTargetCapacity ) : "" ;
      TCHAR *colCapDens  = pTarget->m_colCapDensity >= 0       ? pLayer->GetFieldLabel( pTarget->m_colCapDensity ) : "" ;
      TCHAR *colAvail    = pTarget->m_colAvailCapacity >= 0    ? pLayer->GetFieldLabel( pTarget->m_colAvailCapacity ) : "" ;
      TCHAR *colPctAvail = pTarget->m_colPctAvailCapacity >= 0 ? pLayer->GetFieldLabel( pTarget->m_colPctAvailCapacity ) : "" ;
      
      fprintf( fp, "  <target id='%i' name='%s' method='%s' value='%f' col='%s'  capCol='%s' availCol='%s' pctAvailCol='%s' ",  
         i, (LPCTSTR) pTarget->m_name, pMethod, (float) pTarget->m_rate, col, colCap, colAvail, colPctAvail );
      if ( pTarget->m_colArea >= 0 )
         fprintf( fp, "areaCol='%s' ", pLayer->GetFieldLabel( pTarget->m_colArea ) );
      if ( ! pTarget->m_queryStr.IsEmpty() )
         fprintf( fp, "query='%s' ", (LPCTSTR) pTarget->m_queryStr );
      fputs( " > \n", fp );


      fputs( "\n<!-- any numer of output can be defined using the syntax below -->\n", fp );

      // reports
      for ( int j=0; j < pTarget->m_reportArray.GetSize(); j++ )
         {
         TargetReport *pOutput = pTarget->m_reportArray[ j ];
         fprintf( fp, "     <outputs id='%i' name='%s' query='%s' /> \n", j, (LPCTSTR) pOutput->m_name, (LPCTSTR) pOutput->m_query  );
         }

      // constants
      for ( int j=0; j < pTarget->m_constArray.GetSize(); j++ )
         {
         Constant *pConst = pTarget->m_constArray[ j ];
         fprintf( fp, "     <const name='%s' value='%s' /> \n", (LPCTSTR) pConst->m_name, (LPCTSTR) pConst->m_expression  );
         }

      // allocation sets
      for ( int j=0; j < pTarget->m_allocationSetArray.GetSize(); j++ )
         {
         AllocationSet *pSet = pTarget->m_allocationSetArray[ j ];
         fprintf( fp, "     <allocation_set id='%i' name='%s' > \n", j, (LPCTSTR) pSet->m_name );

         for ( int k=0; k < pSet->GetSize(); k++ )
            {
            ALLOCATION *pAlloc = pSet->GetAt( k );
            fprintf( fp, "        <capacity name='%s' query='%s'  value='%s', multiplier='%f' /> \n",
               (LPCTSTR) pAlloc->name, (LPCTSTR) pAlloc->queryStr,  (LPCTSTR) pAlloc->expression, pAlloc->multiplier );
            }
         
         fputs( "      </allocation_set> \n", fp );
         }

      fputs( "   </target> \n\n", fp );
      }

   fputs( "</target_process> \n", fp );

   fclose( fp );

   return true;
   }


bool TargetProcess::Init( EnvContext *pEnvContext, LPCTSTR  initStr /*xml input file*/ )
   {
   EnvExtension::Init( pEnvContext, initStr );

   if ( m_initialized == true )
      {
      Report::ErrorMsg( "More than on instance of Target specified - this is not allowed" );
      return false;
      }

   CString string( initStr );
   ASSERT( ! string.IsEmpty() );
   if ( string.IsEmpty() )
      return FALSE;

   //MapLayer *pLayer = (MapLayer*) pEnvContext->pMapLayer;

   // load XML input file specified in initStr
   bool ok = LoadXml( initStr, pEnvContext);

   if ( ! ok )
      return FALSE;

   // initialize loaded models
   m_totalActualValue = 0;
   m_totalTargetValue = 0;
   //m_lastTotalActualValue = 0;
   m_totalCapacity = 0;
   m_totalAvailCapacity = 0;
   m_totalPercentAvailable = 0;

   for ( int i=0; i < this->m_targetArray.GetSize(); i++ )
      {
      Target *pTarget = m_targetArray[ i ];
      pTarget->Init( pEnvContext );

      m_totalCapacity         += pTarget->m_totalCapacity;
      m_totalAvailCapacity    += pTarget->m_totalAvailCapacity;
      m_totalActualValue      += pTarget->m_curTotalActualValue;
      m_totalTargetValue      += pTarget->m_curTotalTargetValue;
      }

   m_totalPercentAvailable = m_totalAvailCapacity / m_totalCapacity;
      
   // set up exposed variables

   // only one input variable - allocation set to use (NOTE THIS IS GLOBAL TO ALL TARGETS)
   CString description( _T("Identifier for the allocation scenario to use.  Valid values are: " ) );
   Target *pTarget = m_targetArray[ 0 ];
   for ( int i=0; i < pTarget->m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pSet = pTarget->m_allocationSetArray[ i ];
      ASSERT( pSet != nullptr );

      CString a;
      a.Format( "%i - %s", pSet->m_id, (LPCTSTR) pSet->m_name );

      if ( pSet->m_description.IsEmpty() )
         a += "; ";
      else
         {
         a += " (";
         a += pSet->m_description;
         a += "); ";
         }

      description += a;
      }

   AddInputVar( _T("Allocation Set"), pTarget->m_activeAllocSetID, description );
   AddInputVar( _T("Growth Rate"), pTarget->m_rate, _T("Growth Rate (decimal percent/year)" ) );

   for (int i = 0; i < this->m_tableArray.GetSize(); i++)
      AddInputVar(this->m_tableArray[i]->m_name, this->m_tableArray[i]->m_value, _T("Table Lookup Value"));

   // global outputs
   AddOutputVar( "Target", m_totalTargetValue, _T("") );
   AddOutputVar( "Actual", m_totalActualValue, _T("") );
   AddOutputVar( "Total Capacity", m_totalCapacity, _T("") );
   AddOutputVar( "Total Available Capacity", m_totalAvailCapacity, _T("") );
   AddOutputVar( "Total Percent of Capacity Available", m_totalPercentAvailable, _T("") );

   // add outputs for each target 
   for ( int i=0; i < this->m_targetArray.GetSize(); i++ )
      {
      Target *pTarget = m_targetArray[ i ];

      pTarget->m_outVarStartIndex = m_outVarsSoFar;
      CString vname( pTarget->m_name );
      int varCount = 0;
   
      if ( m_targetArray.GetSize() > 1 )
         {
         vname.Format( "%s.Target", (LPCTSTR) pTarget->m_name );
         AddOutputVar( vname, pTarget->m_curTotalTargetValue, _T("") );

         vname.Format( "%s.Actual", (LPCTSTR) pTarget->m_name );
         AddOutputVar( vname, pTarget->m_curTotalActualValue, _T("") );

         vname.Format( "%s.Total Capacity", (LPCTSTR) pTarget->m_name );
         AddOutputVar(  vname, pTarget->m_totalCapacity, _T("") );

         vname.Format( "%s.Available Capacity", (LPCTSTR) pTarget->m_name );
         AddOutputVar( vname, pTarget->m_totalAvailCapacity, _T("") );

         vname.Format( "%s.Percent of Capacity Available", (LPCTSTR) pTarget->m_name );
         AddOutputVar( vname, pTarget->m_totalPercentAvailable, _T("") );

         varCount = 5;    // based on above
         }

      // add a output variable for each report
      for ( int i=0; i < pTarget->m_reportArray.GetCount(); i++ )
         {
         TargetReport *pReport = pTarget->m_reportArray[ i ];

         vname.Format( "%s.%s.Count", (LPCTSTR) pTarget->m_name, (LPCTSTR) pReport->m_name );
         AddOutputVar(vname, pReport->m_count, _T("") );
         vname.Format("%s.%s.Fraction", (LPCTSTR)pTarget->m_name, (LPCTSTR)pReport->m_name);
         AddOutputVar(vname, pReport->m_fraction, _T(""));
         varCount += 2;
         }

      pTarget->m_outVarCount = varCount;
      m_outVarsSoFar += varCount;
      }

   InitRun( pEnvContext, true );
   return TRUE;
   }
   

bool TargetProcess::InitRun( EnvContext *pEnvContext, bool )
   {
   bool success = true;

   m_totalActualValue = 0;
   m_totalTargetValue = 0;
   m_totalCapacity = 0;
   m_totalAvailCapacity = 0;
   m_totalPercentAvailable = 0;
   
   for ( int i=0; i < this->m_targetArray.GetSize(); i++ )
      {
      Target *pTarget = m_targetArray[ i ];

      if (i != 0) // sync up all allocation set IDs
         pTarget->m_activeAllocSetID = m_targetArray[0]->m_activeAllocSetID;

      bool ok = pTarget->InitRun( pEnvContext );
      if ( ! ok )
         success = false;
      else
         {
         m_totalActualValue      += pTarget->m_startTotalActualValue;
         m_totalTargetValue      += pTarget->m_curTotalTargetValue;
         m_totalCapacity         += pTarget->m_totalCapacity;
         m_totalAvailCapacity    += pTarget->m_totalAvailCapacity;
         }
      }

   m_totalPercentAvailable = m_totalAvailCapacity / m_totalCapacity;

   return success ? TRUE : FALSE;
   }


bool TargetProcess::Run( EnvContext *pEnvContext )
   {   
   // run all target models
   m_totalActualValue = 0;   // current total value of the target variable (updated during Run()) (non-density)
   m_totalTargetValue = 0;

   m_totalCapacity = 0;
   m_totalAvailCapacity = 0;
   m_totalPercentAvailable = 0;

   bool success = true;

   for ( int i=0; i < this->m_targetArray.GetSize(); i++ )
      {
      Target *pTarget = m_targetArray[ i ];
      bool ok = pTarget->Run( pEnvContext );

      if ( ok )
         {
         m_totalActualValue      += pTarget->m_curTotalActualValue;
         m_totalTargetValue      += pTarget->m_curTotalTargetValue;
         m_totalCapacity         += pTarget->m_totalCapacity;
         m_totalAvailCapacity    += pTarget->m_totalAvailCapacity;
         }
      else
         success = false;
      }

   m_totalPercentAvailable = m_totalAvailCapacity / m_totalCapacity;
   
   return success ? TRUE : FALSE;
   }


bool TargetProcess::EndRun( EnvContext *pEnvContext )
   {   
   for ( int i=0; i < this->m_targetArray.GetSize(); i++ )
      {
      Target *pTarget = m_targetArray[ i ];
      pTarget->EndRun( pEnvContext );
      }

   return TRUE;
   }


bool TargetProcess::Setup( EnvContext *pContext, HWND hWnd)
   {
   TargetDlg dlg( this, (MapLayer*) pContext->pMapLayer );
   dlg.DoModal();

   //PopulationProcessDlg dlg( pParent, this );
   //return (int) dlg.DoModal();
   return IDOK;
   }


//void TargetProcess::LoadFieldVars( const MapLayer *pLayer, Target int idu )
//   {
//   int colCount= pLayer->GetColCount();
//
//   // load current values into the m_fieldVarArray
//   for ( int col=0; col < colCount; col++ )
//      {
//      VData value;
//      bool ok = pLayer->GetData( idu, col, value );
//
//      float fValue;
//      if ( ok && value.GetAsFloat( fValue ) )
//         Target::m_fieldVarsArray[ col ] = fValue;
//      else
//         Target::m_fieldVarsArray[ col ] = 0;
//      }
//   }

Constant *TargetProcess::AddConstant(LPCTSTR name, LPCTSTR expr)
   {
   Constant *pConst = new Constant;
   pConst->m_name = name;
   pConst->m_expression = expr;
   m_constArray.Add(pConst);

   MTParser parser;
   pConst->m_value = parser.evaluate(expr);
   return pConst;
   }



///////////////////////////////////////////////////////////////////


TargetProcess *TargetProcessCollection::GetTargetProcessFromID( int id, int  *index )
   {
   if ( m_targetProcessArray.GetSize() == 1 )
      {
      if ( index != nullptr )
         *index = 0;
   
      return m_targetProcessArray.GetAt( 0 );
      }

   for ( int i=0; i < m_targetProcessArray.GetSize(); i++ )
      {
      TargetProcess *pProcess = m_targetProcessArray.GetAt( i );

      if ( pProcess->m_id == id )
         {
         if ( index != nullptr )
            *index = i;

         return pProcess;
         }
      }

   if ( index != nullptr )
      *index = -1;

   return nullptr;
   }



bool TargetProcessCollection::Init( EnvContext *pContext, LPCTSTR initStr /*xml input file*/ )
   {
   // first time through?
   if ( this->m_targetProcessArray.GetSize() == 0 )
      {
      //// store variables from IDU layer that mtparser needs
      //ASSERT( Target::m_fieldVarsArray == nullptr );
      //
      //// allocate variables to store individual IDU info
      //if ( Target::m_fieldVarsArray == nullptr )
      //   {
      //   int colCount = pContext->pMapLayer->GetColCount();
      //   Target::m_fieldVarsArray = new double[ colCount ];
      //
      //   memset( Target::m_fieldVarsArray, 0, pContext->pMapLayer->GetColCount() * sizeof( double ) );
      //   }
      //
      //// make a query engine
      //ASSERT( Target::m_pQueryEngine == nullptr );
      //Target::m_pQueryEngine = pContext->pQueryEngine;
      }

   // keep going...
   TargetProcess *pProcess = new TargetProcess;
   pProcess->m_id = pContext->id;

   this->m_targetProcessArray.Add( pProcess );

   return pProcess->Init( pContext, initStr );
   }


bool TargetProcessCollection::InitRun( EnvContext *pContext, bool b )
   {
   TargetProcess *pProcess = this->GetTargetProcessFromID( pContext->id );

   if ( pProcess )
      return pProcess->InitRun( pContext, b );
   else
      return FALSE;
   }


bool TargetProcessCollection::Run( EnvContext *pContext )
   {
   TargetProcess *pProcess = this->GetTargetProcessFromID( pContext->id );

   if ( pProcess )
      return pProcess->Run( pContext );
   else
      return FALSE;
   }


bool TargetProcessCollection::EndRun( EnvContext *pContext )
   {
   TargetProcess *pProcess = this->GetTargetProcessFromID( pContext->id );

   if ( pProcess )
      return pProcess->EndRun( pContext );
   else
      return FALSE;
   }

int TargetProcessCollection::Setup( EnvContext *pContext, HWND hWnd )
   {
   TargetProcess *pProcess = this->GetTargetProcessFromID( pContext->id );

   if ( pProcess )
      return pProcess->Setup( pContext, hWnd );
   else
      return 0;
   }



int TargetProcessCollection::InputVar ( int id, MODEL_VAR** modelVar )
   {
   TargetProcess *pProcess = this->GetTargetProcessFromID( id );

   if ( pProcess )
      return pProcess->InputVar( id, modelVar );
   else
      return FALSE;
   }


int TargetProcessCollection::OutputVar( int id, MODEL_VAR** modelVar )
   {
   TargetProcess *pProcess = this->GetTargetProcessFromID( id );

   if ( pProcess )
      return pProcess->OutputVar( id, modelVar );
   else
      return FALSE;
   }

bool TargetProcessCollection::GetConfig(EnvContext *pContext, std::string &configStr)
   {
   TargetProcess *pProcess = this->GetTargetProcessFromID(pContext->id);

   if (pProcess)
      return pProcess->GetConfig(pContext, configStr);
   else
      return FALSE;
   }


bool TargetProcessCollection::SetConfig(EnvContext *pContext, LPCTSTR configStr)
   {
   TargetProcess *pProcess = this->GetTargetProcessFromID(pContext->id);

   if (pProcess)
      return pProcess->SetConfig(pContext, configStr);
   else
      return FALSE;
   }