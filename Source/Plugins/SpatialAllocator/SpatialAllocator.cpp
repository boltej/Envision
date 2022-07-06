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
// SpatialAllocator.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "SpatialAllocator.h"

#include <tixml.h>
#include <BitArray.h>
#include <randgen/Randunif.hpp>
#include <misc.h>
#include <PathManager.h>
#include <fstream>
#include <iostream>
#include <EnvModel.h>
#include <stdio.h>


#ifndef NO_MFC
#include "SAllocView.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new SpatialAllocator; }

int CompareScores(const void *elem0, const void *elem1 );


//std::ofstream m_spatialAllocatorResultsFile;

Preference::Preference( Allocation *pAlloc, LPCTSTR name, LPCTSTR queryStr, LPCTSTR weightExpr ) 
   : m_name( name )
   , m_queryStr( queryStr )
   , m_weightExpr( weightExpr )
   , m_pQuery( NULL )
   , m_pMapExpr( NULL ) 
   , m_pAlloc( pAlloc )
   , m_value( 0 )
   {
   // compile query
   if (queryStr != NULL && queryStr[0] != NULL )
      {
      SpatialAllocator *pModel = this->m_pAlloc->m_pAllocSet->m_pSAModel;
      m_pQuery = pModel->m_pQueryEngine->ParseQuery( queryStr, 0, "SpatialAllocator.Preference" );

      if ( m_pQuery == NULL )
         {
         CString msg;
         msg.Format( "  Spatial Allocator: Bad Preference query encountered reading Allocation '%s' - Query is '%s'", (LPCTSTR) pAlloc->m_name, queryStr );
         msg += queryStr;
         Report::ErrorMsg( msg );
         }
      }

   // see the if weightExpr is a constant
   bool isConstant = true;
   for( int i=0; i < lstrlen( weightExpr ); i++ )
      if ( !( isdigit( weightExpr[ i ] ) || weightExpr[ i ] == '-' || weightExpr[ i ] == '.' ) )
         {
         isConstant = false;
         break;
         }

   if ( isConstant )
      m_value = (float) atof( weightExpr );
   else
      {
      SpatialAllocator *pModel = this->m_pAlloc->m_pSAModel;

      m_pMapExpr = pModel->m_pMapExprEngine->AddExpr( name, weightExpr, queryStr );
      bool ok = pModel->m_pMapExprEngine->Compile( m_pMapExpr );

      if ( ! ok )
         {
         CString msg( "  Spatial Allocator: Unable to compile map expression " );
         msg += weightExpr;
         msg += " for <preference> '";
         msg += name;
         msg += "'.  The expression will be ignored";
         Report::ErrorMsg( msg );
         }
      }
   }


Preference &Preference::operator=( Preference &p )
   {
   m_name = p.m_name;
   m_queryStr = p.m_queryStr;
   m_weightExpr = p.m_weightExpr;

   m_pQuery = NULL;
   m_pMapExpr = NULL;
   m_value = p.m_value;

   m_pAlloc = p.m_pAlloc;

   if (m_queryStr.IsEmpty() == false)
      {
      SpatialAllocator *pModel = this->m_pAlloc->m_pSAModel;
      m_pQuery = pModel->m_pQueryEngine->ParseQuery(m_queryStr, 0, m_name);
      }

   //if ( m_weightExpr.IsEmpty() == false )
   //   m_pMapExpr = new MapExpr( *p.m_pMapExpr );

   return *this;
   }



Constraint::Constraint( Allocation *pAlloc, LPCTSTR queryStr ) 
   : m_pAlloc( pAlloc )
   , m_queryStr( queryStr )
   , m_cType( CT_QUERY )
   , m_pQuery( NULL ) 
   { 
   SetConstraintQuery( queryStr );
   }


Constraint &Constraint::operator=( Constraint &c )
   {
   m_queryStr = c.m_queryStr;
   m_cType = c.m_cType;
   m_pQuery = NULL;
   m_pAlloc = c.m_pAlloc;

   CompileQuery();

   return *this;
   }

void Constraint::SetConstraintQuery( LPCTSTR queryStr )
   {
   m_queryStr = queryStr;
   CompileQuery();
   }

Query *Constraint::CompileQuery()
   {
   CString queryStr = m_queryStr;

   if ( m_queryStr.IsEmpty() )
      queryStr = "1";
 
   SpatialAllocator *pModel = this->m_pAlloc->m_pSAModel;
   m_pQuery = pModel->m_pQueryEngine->ParseQuery( queryStr, 0, "SpatialAllocator.Constraint" );

   if ( m_pQuery == NULL )
      {
      CString msg( "  Spatial Allocator: Unable to compile constraint query" );
      msg += m_queryStr;
      msg += "'.  The query will be ignored";
      Report::ErrorMsg( msg );
      }
   return m_pQuery;
   }


float TargetContainer::SetTarget( int year )
   {
   // is a target defined?

   if ( ! IsTargetDefined() )
      {
      m_currentTarget = -1;
      return 0;
      }

   // target is defined, get value
   ASSERT( m_pTargetData->GetRowCount() > 0 );

   m_currentTarget = m_pTargetData->IGet( (float) year, 0, 1 );   // xCol=0, yCol=1

   // apply domain, query modifiers if needed
   if ( this->m_targetDomain == TD_PCTAREA )
      {
      // targets values should be considered decimal fractions. These area applied to the 
      // area that satisfies the target query, or the entire study area if a query is not defined

      SpatialAllocator *pModel = this->m_pSAModel;

      float targetArea = 0;
      MapLayer *pLayer = pModel->m_pMapLayer;

      if ( this->m_pTargetQuery != NULL )
         {
         // run query
         this->m_pTargetQuery->Select( true );

         int selectedCount = pLayer->GetSelectionCount();
         
         for ( int i=0; i < selectedCount; i++ )
            {
            int idu = pLayer->GetSelection( i );

            float area = 0;
            pLayer->GetData( idu, pModel->m_colArea, area );

            targetArea += area;
            }

         pLayer->ClearSelection();
         }
      else
         targetArea = pLayer->GetTotalArea();

      // have area to use, convert target fromdecimal  percent to area
      m_currentTarget *= targetArea;
      }

   return m_currentTarget;
   }


void TargetContainer::Init( int id, int colAllocSet, int colSequence )
   {
   // not a valid target?  no action required then
   if (! IsTargetDefined() )
      {
      m_currentTarget = -1;
      return;
      }

   if ( m_pTargetData != NULL )
      delete m_pTargetData;

   m_pTargetData = new FDataObj( 2, 0, U_YEARS );   // two cols, zero rows

   //if ( m_targetLocation.GetLength() > 0 )
   //   {
   //   m_pTargetMapExpr = theProcess->m_pMapExprEngine->AddExpr( m_name, m_targetLocation, NULL );  // no query string needed, this is handled by constraints
   //   bool ok = theProcess->m_pMapExprEngine->Compile( m_pTargetMapExpr );
   //
   //   if ( ! ok )
   //      {
   //      CString msg( "  Spatial Allocator: Unable to compile target location map expression " );
   //      msg += m_targetLocation;
   //      msg += " for <allocation> '";
   //      msg += m_name;
   //      msg += "'.  The expression will be ignored";
   //      Report::ErrorMsg( msg );
   //
   //      theProcess->m_pMapExprEngine->RemoveExpr( pExpr );
   //      m_pTargetMapExpr = NULL;
   //      }
   //   }

   switch( this-> m_targetSource )
      {
      case TS_FILE:
         {
         int rows = m_pTargetData->ReadAscii( this->m_targetValues );
         if ( rows <= 0 )
            {
            CString msg( "  Spatial Allocator: Unable to load target file '" );
            msg += m_targetValues;
            msg += "' - this allocation will be ignored";
            Report::ErrorMsg( msg );
            }
         }
         break;

      case TS_TIMESERIES:
         {
         TCHAR *targetValues = new TCHAR[ this->m_targetValues.GetLength()+2 ];
         memset( targetValues, 0, (this->m_targetValues.GetLength()+2) * sizeof( TCHAR ) );

         lstrcpy( targetValues, this->m_targetValues );
         TCHAR *nextToken = NULL;

         // note: target values look like sequences of x/y pairs
         LPTSTR token = _tcstok_s( targetValues, _T(",() ;\r\n"), &nextToken );
         float pair[2];
         while ( token != NULL )
            {
            pair[0] = (float) atof( token );
            token   = _tcstok_s( NULL, _T(",() ;\r\n"), &nextToken );
            pair[1] = (float) atof( token );
            token   = _tcstok_s( NULL, _T(",() ;\r\n"), &nextToken );
   
            m_pTargetData->AppendRow( pair, 2 );
            }

         if ( m_pTargetData->GetRowCount() == 0 )
            {
            CString msg;
            msg.Format("SA: Time series values missing for Allocation '%s'!", (LPCTSTR) this->m_name );
            Report::LogWarning(msg);
            }

         delete [] targetValues;
         }
         break;

      case TS_RATE:
         {
         m_targetRate = (float) atof( m_targetValues );

         // note: target values are expressed in terms of the target basis field
         TCHAR *targetValues = new TCHAR[ this->m_targetValues.GetLength()+2 ];
         memset( targetValues, 0, (this->m_targetValues.GetLength()+2 )*sizeof( TCHAR ) );

         lstrcpy( targetValues, this->m_targetValues );
         TCHAR *nextToken = NULL;

         float rate = (float) atof( targetValues );
         float basis = 0;
         MapLayer *pLayer = m_pSAModel->m_pMapLayer;

         //bool isSequence = this->IsSequence() && m_pAllocSet->m_colSequence >= 0;
         
         // is a target basis field defined?  if not, use area
         if ( m_colTargetBasis < 0  )
            m_colTargetBasis = pLayer->GetFieldCol( this->m_targetBasisField );
         if ( m_colTargetBasis < 0 )
            m_colTargetBasis = pLayer->GetFieldCol( "AREA" );

         // get starting values from map
         for ( MapLayer::Iterator idu=pLayer->Begin(); idu < pLayer->End(); idu++ )
            {
            float iduBasis = 0;
            pLayer->GetData( idu, m_colTargetBasis, iduBasis );

            if ( colSequence >= 0  )
               {
               int sequenceID = -1;
               pLayer->GetData( idu, colSequence, sequenceID );

               if ( sequenceID == id )
                  basis += iduBasis;
               }
            else
               {
               int attr = -1;
               pLayer->GetData( idu, colAllocSet, attr );
               if ( attr == id )
                  basis += iduBasis;
               }
            }

         float pair[ 2 ];
         pair[0] = 0;
         pair[1] = basis;
         m_pTargetData->AppendRow( pair, 2 );

         float growth = basis * rate * 100; 
         
         pair[0] = 100;
         pair[1] = basis + growth;
         m_pTargetData->AppendRow( pair, 2 );

         delete [] targetValues;
         }

      break;
      }
   
   //CString msg( "SA: Initial Allocation Target Table for " );
   //msg += m_name;   
   //Report::Log( msg );
   //
   //for ( int i=0; i < m_pTargetData->GetRowCount(); i++ )
   //   {
   //   int year = m_pTargetData->GetAsInt( 0, i );
   //
   //   float area = m_pTargetData->GetAsFloat( 1, i );
   //
   //   CString data;
   //   if ( this->m_targetDomain == TD_PCTAREA )
   //      {
   //      area *= 100;
   //      data.Format( "    Year: %i  Target Area: %.2f percent", year, area );
   //      }
   //   else
   //      {
   //      area /= 10000;  // m2 to ha
   //      data.Format( "    Year: %i  Target Area: %.1f ha", year, area );
   //      }
   //
   //   Report::Log( data );
   //   }

   }


void TargetContainer::SetTargetParams( MapLayer *pLayer, LPCTSTR basis, LPCTSTR source, LPCTSTR values, LPCTSTR domain, LPCTSTR query )
   {
   // source
   if ( source != NULL )
      {
      if ( source[0] == 't' || source[0] == 'T' )
         this->m_targetSource = TS_TIMESERIES;
      else if ( source[0] == 'f' || source[0] == 'F' )
         this->m_targetSource = TS_FILE;
      else if ( source[0] == 'r' || source[0] == 'R' )
         this->m_targetSource = TS_RATE;
      else if ( source[0] == 'u'  || source[0] == 'U')
         {
         if ( this->GetTargetClass() == TC_ALLOCATION )
            this->m_targetSource = TS_USEALLOCATIONSET;
         else
            this->m_targetSource = TS_USEALLOCATION;
         }
      }
   else
      {
      // no source specified - assume specified elsewhere
      this->m_targetSource = TS_UNDEFINED;
      }

   // values
   if ( values != NULL )
      this->m_targetValues = values;      

   // domain
   if ( domain != NULL && domain[ 0 ] == 'p' )   // note - if not specified, TD_AREA is used
      {
      m_targetDomain = TD_PCTAREA;
      
      if ( query != NULL )
         {
         this->m_targetQuery = query;
         this->m_pTargetQuery = m_pSAModel->m_pQueryEngine->ParseQuery( query, 0, "" );

         if ( this->m_pTargetQuery == NULL )
            {
            CString msg( "  Spatial Allocator: Unable to parse target query '" );
            msg += query;
            msg += "' - it will be ignored";
            Report::WarningMsg( msg );
            }
         }
      }
         
   // basis field (default is "AREA" unless defined elsewhere, in which case it is blank.)
   if ( basis != NULL )
      this->m_targetBasisField = basis;
   else
      {
      // defined at the allocation set level?
      if ( this->GetTargetClass() == TC_ALLOCATIONSET && ( this->m_targetSource == TS_USEALLOCATION ) )
         this->m_targetBasisField = "AREA";
      else if ( this->GetTargetClass() == TC_ALLOCATION && this->m_targetSource != TS_USEALLOCATIONSET )
         this->m_targetBasisField = "AREA";
      else
         this->m_targetBasisField == "";
      }

   this->m_colTargetBasis = pLayer->GetFieldCol( this->m_targetBasisField );
   }





Allocation::Allocation( AllocationSet *pSet, LPCTSTR name, int id ) //, TARGET_SOURCE targetSource, LPCTSTR targetValues )
   : TargetContainer( pSet->m_pSAModel, name ) // targetSource, TD_AREA, targetValues )
   , m_id( id )
   , m_constraint()
   //, m_pTargetMapExpr( NULL )
   , m_useExpand( false )       // allo expanded outcomes?
   , m_expandAreaStr()
   , m_expandType( 0 )
   , m_pRand( NULL )
   , m_expandMaxArea( -1 )      // maximum area allowed for expansion, total including kernal
   , m_expandAreaParam0( -1 )
   , m_expandAreaParam1( 0 )
   , m_expandQueryStr()         // expand into adjacent IDUs that satify this query 
   , m_pExpandQuery( NULL )
   , m_initPctArea( 0 )
   , m_cumBasisActual( 0 )
   , m_cumBasisTarget( 0 )
   , m_minScore( 0 )       // min and max score for this allocation
   , m_maxScore( 0 )
   , m_scoreCount( 0 )     // number of scores greater than 0 at any given time
   , m_usedCount( 0 )
   , m_scoreArea( 0 )      // area of scores greater than 0 at any given time
   , m_scoreBasis( 0 )    // 
   , m_expandArea( 0 )    // 
   ,  m_expandBasis( 0 )    // 
   , m_constraintAreaSatisfied( 0 )
   , m_colScores( -1 )
   , m_pAllocSet( pSet )
   //, m_iduScoreArray( NULL )
   , m_currentIduScoreIndex( -1 )
   {
   m_constraint.m_pAlloc = this;
   }

Allocation::~Allocation( void )
   {
   if ( m_pRand != NULL )
      delete m_pRand;
   }

Allocation& Allocation::operator = ( Allocation &a )
   {
   TargetContainer::operator = ( a );

   m_id            = a.m_id;      // id
   m_useExpand     = a.m_useExpand;
   m_expandAreaStr = a.m_expandAreaStr;
   m_expandType    = a.m_expandType;

   if ( a.m_pRand != NULL )
      {
      switch( m_expandType )
         {
         case ET_UNIFORM:
            m_pRand = new RandUniform( a.m_expandAreaParam0, a.m_expandAreaParam1, 0 );
            break;

         case ET_NORMAL:
            m_pRand = new RandNormal( a.m_expandAreaParam0, a.m_expandAreaParam1, 0 );
            break;

         case ET_WEIBULL:
            m_pRand = new RandWeibull( a.m_expandAreaParam0, a.m_expandAreaParam1 );
            break;

         case ET_LOGNORMAL:
            m_pRand = new RandLogNormal( a.m_expandAreaParam0, a.m_expandAreaParam1, 0 );
            break;

         case ET_EXPONENTIAL:
            m_pRand = new RandExponential( a.m_expandAreaParam0 );
            break;
         }
      }
   else
      m_pRand = NULL;

   m_expandAreaParam0 = a.m_expandAreaParam0;
   m_expandAreaParam1 = a.m_expandAreaParam1;

   m_expandMaxArea = a.m_expandMaxArea;   
   m_expandQueryStr = a.m_expandQueryStr;
   if ( a.m_pExpandQuery != NULL )
      m_pExpandQuery = new Query( *a.m_pExpandQuery );
   else
      m_pExpandQuery = NULL;

   m_initPctArea = a.m_initPctArea;
   m_constraintAreaSatisfied = a.m_constraintAreaSatisfied;

   m_cumBasisActual = 0;
   m_cumBasisTarget = 0;

   // additional stats
   m_minScore    = a.m_minScore;
   m_maxScore    = a.m_maxScore;
   m_scoreCount  = a.m_scoreCount;
   m_usedCount   = a.m_usedCount;      
   m_scoreArea   = a.m_scoreArea; 
   m_scoreBasis  = a.m_scoreBasis;
   m_expandArea  = a.m_expandArea;
   m_expandBasis = a.m_expandBasis;
     
   m_prefsArray.DeepCopy( a.m_prefsArray );      // allocates new objects based on copy constructor
   m_constraint = a.m_constraint;  //m_constraintArray.DeepCopy( (const PtrArray<Constraint> ) a.m_constraintArray );
 
   m_sequenceArray.Copy( a.m_sequenceArray );

   m_sequenceMap.RemoveAll();
   POSITION p = a.m_sequenceMap.GetStartPosition(); 
   while (p) 
      { 
      int key, value; 
      a.m_sequenceMap.GetNextAssoc(p,key,value); 
      m_sequenceMap.SetAt(key,value);
      } 

   m_iduScoreArray.Copy( a.m_iduScoreArray );

   m_scoreCol = a.m_scoreCol;
   m_colScores = a.m_colScores;

   // ScorePriority temporary variables
   m_currentIduScoreIndex = 0;   // this stores the current index of this allocation in the 
   
   m_pAllocSet = NULL;     // fixed up in AlloctionSet:: operator=()

   return *this;
   }


void Allocation::SetExpandQuery( LPCTSTR expandQuery )
   {
   m_expandQueryStr = expandQuery;

   if ( m_expandQueryStr.IsEmpty() )
      {
      m_useExpand = false;
      m_pExpandQuery = NULL;
      return;
      }
   
   m_useExpand = true;
   CString _expandQuery( expandQuery );
   _expandQuery.Replace( "@", m_constraint.m_queryStr );

   m_pExpandQuery = this->m_pSAModel->m_pQueryEngine->ParseQuery( _expandQuery, 0, "Allocation.Expand" );

   if ( m_pExpandQuery == NULL )
      {
      CString msg( "  Spatial Allocator: Unable to parse expand query '" );
      msg += _expandQuery;
      msg += "' - it will be ignored";
      Report::WarningMsg( msg );
      }

   }

void Allocation::SetMaxExpandAreaStr( LPCTSTR expandAreaStr )
   {
   m_expandAreaStr = expandAreaStr;

   if ( m_expandAreaStr.IsEmpty() )
      return;

   // legal possibiliites include:  a constant number, uniform(lb,ub), normal( mean, variance)
   bool success = true;
   if ( m_pRand != NULL )
      {
      delete m_pRand;
      m_pRand = NULL;
      }

   switch( expandAreaStr[ 0 ] )
      {
      case 'u':      // uniform distribution
      case 'U':
      case 'n':      // normal distribution
      case 'N':
      case 'w':      // weibull distribution
      case 'W':
      case 'l':      // lognormal distribution
      case 'L':
      case 'e':      // lognormal distribution
      case 'E':
         {
         TCHAR buffer[ 64 ];
         lstrcpy( buffer, expandAreaStr );
         TCHAR *token = _tcschr( buffer, '(' );
         if ( token == NULL ) { success = false; break; }

         token++;    // point to first param (mean)
         m_expandAreaParam0 = atof( token );

         if ( expandAreaStr[ 0 ] != 'e' && expandAreaStr[ 0 ] != 'E' )
            {
            token = _tcschr( buffer, ',' );
            if ( token == NULL ) { success = false; break; }
            m_expandAreaParam1 = atof( token+1 );
            }

         switch( expandAreaStr[ 0 ] )
            {
            case 'u':      // uniform distribution
            case 'U':
               m_expandType = ET_UNIFORM;
               m_pRand = new RandUniform( m_expandAreaParam0, m_expandAreaParam1, 0 );
               break;

            case 'n':      // normal distribution
            case 'N':
               m_expandType = ET_NORMAL;
               m_pRand = new RandNormal( m_expandAreaParam0, m_expandAreaParam1, 0 );
               break;

            case 'w':      // weibull distribution
            case 'W':
               m_expandType = ET_WEIBULL;
               m_pRand = new RandWeibull( m_expandAreaParam0, m_expandAreaParam1 );
               break;

            case 'l':      // lognormal distribution
            case 'L':
               m_expandType = ET_LOGNORMAL;
               m_pRand = new RandLogNormal( m_expandAreaParam0, m_expandAreaParam1, 0 );
               break;

            case 'e':      // exponential distribution
            case 'E':
               m_expandType = ET_EXPONENTIAL;
               m_pRand = new RandExponential( m_expandAreaParam0 );
               break;
            }

         m_useExpand  = true;
         }
         break;

      case '0':      // it is a number
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         m_expandAreaParam0 = m_expandMaxArea = (float) atof( expandAreaStr );
         m_useExpand = true;
         break;

      default:
         success = false;
      }

   if ( success == false )
      {
      CString msg( "  Spatial Allocator: unrecognized expand area specification: " );
      msg += expandAreaStr;
      Report::ErrorMsg( msg );
      //m_useExpand = false;
      }
   }
  

float Allocation::SetMaxExpandArea( void )
   {
   switch( this->m_expandType )
      {
      case ET_CONSTANT:
         break;      // nothing required

      case ET_UNIFORM:
      case ET_NORMAL:
      case ET_WEIBULL:
      case ET_LOGNORMAL:
      case ET_EXPONENTIAL:
         ASSERT( m_pRand != NULL );
         m_expandMaxArea = (float) m_pRand->RandValue();
         break;
      }

   return m_expandMaxArea;
   }


float Allocation::GetCurrentTarget()  // returns allocation if defined, otherwise, allocation set target
   {
   if ( IsTargetDefined() )
      return m_currentTarget;
   else
      return this->m_pAllocSet->m_currentTarget;
   }


AllocationSet &AllocationSet::operator = ( AllocationSet &as )
   {
   TargetContainer::operator = ( as );

   m_field       = as.m_field;
   m_seqField    = as.m_seqField;
   m_col         = as.m_col;
   m_colSequence = as.m_colSequence;
   m_inUse       = as.m_inUse;
   //m_shuffleIDUs = as.m_shuffleIDUs;

   m_allocationArray.DeepCopy( as.m_allocationArray );
   
   for ( int i=0; i < m_allocationArray.GetCount(); i++ )
      m_allocationArray[ i ]->m_pAllocSet = this;

   if ( as.m_pOutputData != NULL )  
      m_pOutputData = new FDataObj( *as.m_pOutputData );
   else
      m_pOutputData = NULL;
   //m_pOutputDataCum = new FDataObj( *as.m_pOutputDataCum );

   if ( as.m_pTargetMapExpr != NULL  )
      m_pTargetMapExpr = new MapExpr( *as.m_pTargetMapExpr );
   else
      m_pTargetMapExpr = NULL;

   m_targetLocation = as.m_targetLocation;

   return *this;
   }



int AllocationSet::OrderIDUs( CUIntArray &iduArray, bool shuffleIDUs, RandUniform *pRandUnif )
   {
   int iduCount = (int) iduArray.GetSize();

   // order is dependant on two things:
   // 1) whether shuffling is turned on - this turns on/off random/probabilistic sampling
   // 2) whether a target location surface is defined.
   // --------------------------------------------------------------------------
   //            Shuffling ->   |          On            |         Off         |
   // --------------------------------------------------------------------------
   //  Location is defined      | probablistically       | pick from best      |
   //                           | sample location surface| downward            |
   // --------------------------------------------------------------------------
   //  Location is NOT defined  | shuffle IDUs           | go in IDU order     |
   // --------------------------------------------------------------------------

   if ( shuffleIDUs )
      {
      // case 1: Shuffling on, location defined...
      if ( m_pTargetMapExpr )
         {
         // generate probability surface
         //for ( int i=0; i < 


         }
      else  // case 2:  shuffling on, no location defined
         {
         ::ShuffleArray< UINT >( iduArray.GetData(), iduCount, pRandUnif );
         }
      }
   else
      {
      // case 3: Shuffling off, location defined...
      if ( m_pTargetMapExpr )
         {
         }
      else  // case 4:  shuffling off, no location defined
         {
         for ( int i=0; i < iduCount; i++ )
            iduArray[ i ] = i;
         }
      }

   return iduCount;
   }





//=====================================================================================================
// S P A T I A L    A L L O C A T O R
//=====================================================================================================

SpatialAllocator::SpatialAllocator() 
: EnvModelProcess()
, m_xmlPath()
, m_allocationSetArray()
, m_pMapLayer( NULL )
, m_colArea( -1 ) 
, m_allocMethod( AM_SCORE_PRIORITY )
, m_expansionIDUs()
, m_scoreThreshold( 0 )
, m_iduArray()
, m_shuffleIDUs( true )
, m_pRandUnif(NULL)
, m_totalArea(0)
, m_runNumber( -1 )
, m_outputData(U_YEARS)
, m_collectExpandStats(false)
, m_pExpandStats(NULL)
, m_pBuildFromFile(NULL)
   { }


SpatialAllocator::SpatialAllocator( SpatialAllocator &sa )
   : EnvModelProcess() 
   , m_xmlPath( sa.m_xmlPath )
   , m_pMapLayer( sa.m_pMapLayer )
   , m_allocationSetArray()  // sa.m_allocationSetArray )
   , m_colArea( sa.m_colArea )
   , m_allocMethod( sa.m_allocMethod )
   , m_expansionIDUs()
   , m_scoreThreshold( sa.m_scoreThreshold )
   , m_iduArray()
   , m_shuffleIDUs( sa.m_shuffleIDUs )
   , m_pRandUnif( NULL )
   , m_totalArea( sa.m_totalArea )
   , m_runNumber( -1 )
   , m_outputData( sa.m_outputData )   // ?????????????????
   , m_collectExpandStats( false )    // ????????????????
   , m_pExpandStats( NULL )
   {
   m_iduArray.Copy( sa.m_iduArray );   
   
   m_allocationSetArray.DeepCopy( sa.m_allocationSetArray );

   if ( m_shuffleIDUs )
      m_pRandUnif = new RandUniform( 0.0, (double) m_pMapLayer->GetRecordCount(), 0 );
   }


SpatialAllocator::~SpatialAllocator( void )
   { 
   //if ( m_pMapExprEngine ) 
   //   delete m_pMapExprEngine; 

   // and local data
   if ( m_pRandUnif )
      delete m_pRandUnif;
   if ( m_pBuildFromFile)
      delete m_pBuildFromFile;
   
   }


bool SpatialAllocator::Init( EnvContext *pEnvContext, LPCTSTR initStr )
   {
   m_pMapLayer = (MapLayer*) pEnvContext->pMapLayer;

   // allocate static expression and query engines if needed
   m_pMapExprEngine = pEnvContext->pExprEngine;
   m_pQueryEngine = pEnvContext->pQueryEngine;

   if (m_colArea < 0)
      m_colArea = m_pMapLayer->GetFieldCol(_T("AREA"));

   // initialize internal array for sorting IDUs
   int iduCount = m_pMapLayer->GetPolygonCount();

   m_iduArray.SetSize( iduCount );
   for ( int i=0; i < iduCount; i++ )
      m_iduArray[ i ] = i;

   // load input file (or just return if no file specified)

   if ( initStr == NULL || *initStr == '\0' )
      return TRUE;

   if ( LoadXml( initStr ) == false )
      return FALSE;
   
   // iterate through AllocationSets and Allocations to initialize internal data
   for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];
      
      for ( int j=0; j < (int) pAllocSet->m_allocationArray.GetSize(); j++ )
         {
         Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];

         pAlloc->m_iduScoreArray.SetSize( iduCount );
         for ( int k=0; k < iduCount; k++ )
            {
            pAlloc->m_iduScoreArray[ k ].idu = -1;
            pAlloc->m_iduScoreArray[ k ].score = 0;
            }
         }
      }

   // allocation sequences on map
   PopulateSequences();

   // Allocate output data (pAllocSet->m_pOutputData, TargetContainer::Init() for all target containers to allocate Target DataObj)
   InitAllocSetData();

   // set up input/output variables
   for ( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];

      // add output variables
      CString varName( pAllocSet->m_name );
      varName += ".InUse";
      AddInputVar( varName, pAllocSet->m_inUse, "Use this Allocation Set for the given scenario" );

      varName = "AllocationSet-";
      varName += pAllocSet->m_name;
      AddOutputVar( varName, pAllocSet->m_pOutputData, "" );

      for ( int j=0; j < pAllocSet->GetAllocationCount(); j++ )
         {
         Allocation *pAlloc = pAllocSet->GetAllocation( j );

         // next, for each Allocation, add input variable for the target rate and initPctArea
         if ( pAlloc->m_targetSource == TS_RATE )
            {
            CString varName( pAlloc->m_name );
            varName += ".Target Rate";
            AddInputVar( varName, pAlloc->m_targetRate, "Target Rate of change of the allocation, dec. percent" );
         
            //if ( pAlloc->IsSequence() )
            //   {
            //   varName = pAlloc->m_name;
            //   varName += ".Initial Percent Area";
            //   AddInputVar( varName, pAlloc->m_initPctArea, "Initial portion of landscape area, dec. percent" );
            //   }
            }
         }
      }


   // report results
   for ( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];

      CString msg;
      msg.Format( "  Spatial Allocator:  Loaded Allocation Set %s", pAllocSet->m_name );
      Report::Log( msg );

      for ( int j=0; j < pAllocSet->GetAllocationCount(); j++ )
         {
         Allocation *pAlloc = pAllocSet->GetAllocation( j );

         msg.Format( "                    Allocation %s", pAlloc->m_name );
         Report::Log( msg );
         }
      }

   return TRUE;
   }


void SpatialAllocator::InitAllocSetData( void )
   {
   for ( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];

      pAllocSet->Init( -1, pAllocSet->m_col, -1 );

      // set up data objects for the AllocationSet
      bool isTargetDefined = pAllocSet->IsTargetDefined();

      int cols = 0;
      if ( isTargetDefined )
         cols = 1 + 3 + 10*pAllocSet->GetAllocationCount();  // time + 2 per AllocationSet + 9 per Allocation
      else
         cols = 1 + 0 + 11*pAllocSet->GetAllocationCount();  // time + 0 per AllocationSet + 10 per Allocation

      // first one percent areas
      int col = 0;
      CString label( "AllocationSet-" );
      label += pAllocSet->m_name;
      pAllocSet->m_pOutputData = new FDataObj( cols, 0, U_YEARS );
      pAllocSet->m_pOutputData->SetName( label );
      pAllocSet->m_pOutputData->SetLabel( col++, "Time" );

      if ( isTargetDefined )
         {
         pAllocSet->m_pOutputData->SetLabel( col++, "Total Target");
         pAllocSet->m_pOutputData->SetLabel( col++, "Total Realized");         
         pAllocSet->m_pOutputData->SetLabel(col++, "Total Area Realized");
         }

      for ( int j=0; j < pAllocSet->GetAllocationCount(); j++ )
         {
         Allocation *pAlloc = pAllocSet->GetAllocation( j );
         
         pAlloc->Init( pAlloc->m_id, pAllocSet->m_col, pAlloc->IsSequence() ? pAllocSet->m_colSequence : -1 );

         if ( ! isTargetDefined )
            {
            label = pAlloc->m_name + "-Target";
            pAllocSet->m_pOutputData->SetLabel( col++, label );
            }
                  
         label = pAlloc->m_name + "-Realized";
         pAllocSet->m_pOutputData->SetLabel( col++, label );

         label = pAlloc->m_name + "-Realized Area";
         pAllocSet->m_pOutputData->SetLabel(col++, label);

         label = pAlloc->m_name + "-pctRealized";
         pAllocSet->m_pOutputData->SetLabel( col++, label );

         label = pAlloc->m_name + "-ConstraintArea";
         pAllocSet->m_pOutputData->SetLabel( col++, label );

         label = pAlloc->m_name + "-MinScore";
         pAllocSet->m_pOutputData->SetLabel( col++, label );

         label = pAlloc->m_name + "-MaxScore";
         pAllocSet->m_pOutputData->SetLabel( col++, label );

         label = pAlloc->m_name + "-ScoreCount";
         pAllocSet->m_pOutputData->SetLabel( col++, label );

         label = pAlloc->m_name + "-ScoreBasis";
         pAllocSet->m_pOutputData->SetLabel( col++, label );

         label = pAlloc->m_name + "-ExpandArea";
         pAllocSet->m_pOutputData->SetLabel( col++, label );

         label = pAlloc->m_name + "-ExpandBasis";
         pAllocSet->m_pOutputData->SetLabel( col++, label );
         }
      }  // end of: for each allocation set
   }



bool SpatialAllocator::InitRun( EnvContext *pEnvContext, bool useInitialSeed )
   {
   // populate initial scores
   SetAllocationTargets( pEnvContext->currentYear );
   ScoreIduAllocations( pEnvContext, false );

   // clear data objects
   for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];
      pAllocSet->m_allocationSoFar = 0;
      pAllocSet->m_areaSoFar = 0;
      
      // init each allocation
      for ( int j=0; j < pAllocSet->GetAllocationCount(); j++ )
         {
         Allocation *pAlloc = pAllocSet->GetAllocation( j );
         pAlloc->m_cumBasisActual = 0;
         pAlloc->m_cumBasisTarget = 0;
         pAlloc->m_allocationSoFar = 0;
         pAlloc->m_areaSoFar = 0;
         pAlloc->m_constraintAreaSatisfied = 0;
         }

      // clear out output data
      if ( pAllocSet->m_pOutputData )
         pAllocSet->m_pOutputData->ClearRows();

      //if ( pAllocSet->m_pOutputDataCum )
      //   pAllocSet->m_pOutputDataCum->ClearRows();
      }
   
   return TRUE; 
   }


bool SpatialAllocator::Run( EnvContext *pContext )
   {
   if (m_pBuildFromFile)
      UpdateAllocationsFromFile();
   // set the desired value for the targets
   SetAllocationTargets( pContext->currentYear );

   // first score idus for each allocation
   ScoreIduAllocations( pContext, true );

   // so, at this point we've computed scores for each allocation in each alloction set
   // now allocate them based on scores and allocation targets.  
   switch( m_allocMethod )
      {
      case AM_SCORE_PRIORITY:
         AllocScorePriority( pContext, true);
         break;

      case AM_BEST_WINS:
         AllocBestWins( pContext, true );
         break;
      }

   // report results to output window
   CollectData( pContext );

   return TRUE;
   }


bool SpatialAllocator::EndRun( EnvContext *pContext )
   {
	return true;
	}


bool SpatialAllocator::PopulateSequences( void )
   {
   // only called during Init - sets up initial sequences
   //Report::Log( "  Spatial Allocator: Populating Sequences" );
   bool hasSequences = false;

   // get total basis value
   // NOTE: FOR SEQUENCES, ONLY AREA IS CURRENTLY CONSIDERED - THIS SHOULD BE MODIFIED IN THE FUTURE
   
   m_totalArea = m_pMapLayer->GetTotalArea();
   int colArea = m_pMapLayer->GetFieldCol( "AREA" );

   // determine whether there are any sequences
   for ( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];
      
      if ( pAllocSet->m_inUse )
         {   
         if ( pAllocSet->m_colSequence >= 0 )
            {
            m_pMapLayer->SetColNoData( pAllocSet->m_colSequence );
            hasSequences = true;

            // if a sequence, initialize based on starting pct
            for ( int j=0; j < (int) pAllocSet->m_allocationArray.GetSize(); j++ )
               {
               Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];
   
               pAlloc->m_allocationSoFar = 0;
               pAlloc->m_areaSoFar = 0;
               pAlloc->m_currentTarget = pAlloc->m_initPctArea * m_totalArea;   // why is this here?
               }
            }
         }
      }

   if ( hasSequences == false )
      return false;

   // set up a shuffle array to randomly look through IDUs when allocating sequences
   int iduCount = m_pMapLayer->GetPolygonCount(); 
   CArray< int > iduArray;
   iduArray.SetSize( iduCount );
   for ( int i=0; i < iduCount; i++ )
      iduArray[ i ] = i;      

   RandUniform rnd( 0, iduCount );

   ::ShuffleArray< int >( iduArray.GetData(), iduCount, &rnd );

   // iterate through maps as needed
   //for ( int MapLayer::Iterator idu=m_pMapLayer->Begin(); idu < m_pMapLayer->End(); idu++ )
   for ( int m=0; m < iduCount; m++ )
      {
      int idu = iduArray[ m ];         // a random grab

      m_pQueryEngine->SetCurrentRecord( idu );
      m_pMapExprEngine->SetCurrentRecord( idu );

      for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
         {
         AllocationSet *pAllocSet = m_allocationSetArray[ i ];
 
         if ( pAllocSet->m_inUse && pAllocSet->m_colSequence >= 0 )
            {
            // is this IDU already tagged to a sequence in this allocation set?  if so, skip it.
            int sequenceID = -1;
            m_pMapLayer->GetData( idu, pAllocSet->m_colSequence, sequenceID );

            if ( sequenceID >= 0 )
               continue;   // already in a sequence, so skip it and go to next AllocationSet

            int currAttr = -1;   // this is the current IDU's value for the AllocSet's col
            m_pMapLayer->GetData( idu, pAllocSet->m_col, currAttr );
            
            // look through allocations in this Allocation Set, to try to allocate them
            for ( int j=0; j < (int) pAllocSet->m_allocationArray.GetSize(); j++ )
               {
               Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];

               if ( pAlloc->IsSequence() && pAlloc->m_allocationSoFar < pAlloc->m_currentTarget )
                  {
                  // is it already in a sequence (from a prior Allocation in this AllocationSet)?
                  int _sequenceID = -1;
                  m_pMapLayer->GetData( idu, pAllocSet->m_colSequence, _sequenceID );

                  if ( _sequenceID >= 0 )
                     break;   // already in a sequence, so skip on to the next AllocationSet

                  // next, see if the attribute in the coverage is in the sequence
                  bool inSequence = false;
                  for ( int k=0; k < (int) pAlloc->m_sequenceArray.GetSize(); k++ )
                     {
                     if ( pAlloc->m_sequenceArray[ k ] == currAttr )
                        {
                        inSequence = true;
                        break;
                        }
                     }

                  // skip this allocation if the IDU doesn't fit it.
                  if ( inSequence == false )   
                     continue;   // skip to next Allocation 

                  // second, see if it passes all constraints
                  bool passConstraints = true;
               
                  bool result = false;
                  bool ok = pAlloc->m_constraint.m_pQuery->Run( idu, result );
               
                  if ( result == false )
                     {
                     passConstraints = false;
                     break;
                     }               

                  // did we pass the constraints?  If so, add area and tag coverage
                  if ( passConstraints )
                     {
                     m_pMapLayer->SetData( idu, pAllocSet->m_colSequence, pAlloc->m_id );

                     float area = 0, basis = 0;
                     m_pMapLayer->GetData( idu, m_colArea, area );
                     m_pMapLayer->GetData(idu, pAllocSet->m_colTargetBasis, basis);

                     pAlloc->m_allocationSoFar += basis;
                     pAlloc->m_areaSoFar += area;
                     //break;  // go to next AllocationSet
                     }
                  }  // end of:  if pAlloc->IsSequence() && pAlloc->m_allocationSoFar < pAlloc->m_currentTarget )
               }  // end of: for each Allocation
            }  // end of: if ( pAllocSet->m_inUse )
         }  // end of: for each AllocationSet
      }  // end of: for each IDU

   // report results
   for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];

      if ( pAllocSet->m_inUse )
         { 
         // otherwise, look through allocations to try to allocate them
         for ( int j=0; j < (int) pAllocSet->m_allocationArray.GetSize(); j++ )
            {
            Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];
   
            if ( pAlloc->IsSequence() )
               {
               CString msg;
               msg.Format( "  Spatial Allocator: Sequence '%s' allocated to %4.1f percent of the area (target = %4.1f)",
                  (LPCTSTR) pAlloc->m_name, pAlloc->m_allocationSoFar * 100 / m_totalArea, pAlloc->m_initPctArea*100 );
   
               pAlloc->m_allocationSoFar = 0;
               pAlloc->m_areaSoFar = 0;
               pAlloc->m_currentTarget = 0;
   
               Report::Log( msg );
               }
            }
         }
      }

   return true;
   }


void SpatialAllocator::SetAllocationTargets( int currentYear )
   {
   for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];

      pAllocSet->SetTarget( currentYear );

      for ( int j=0; j < (int) pAllocSet->m_allocationArray.GetSize(); j++ )
         {
         Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];

         if ( pAllocSet->m_inUse )
            pAlloc->SetTarget( currentYear );
         else
            pAlloc->m_currentTarget = 0;
         }
      }
   }


void SpatialAllocator::ScoreIduAllocations( EnvContext *pContext, bool useAddDelta )
   {
   int iduCount = m_pMapLayer->GetRecordCount();

   if ( m_shuffleIDUs && m_pRandUnif == NULL )
      m_pRandUnif = new RandUniform( 0.0, (double) m_pMapLayer->GetRecordCount(), 0 );

   /////////////////
   int scoreCount = 0;
   /////////////////
   
   // first, zero out allocation scores.  They will get calculated in the next section 
   for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];
      pAllocSet->m_allocationSoFar = 0;
      pAllocSet->m_areaSoFar = 0;
      pAllocSet->OrderIDUs( m_iduArray, m_shuffleIDUs, m_pRandUnif );
      
      if ( pAllocSet->m_inUse )
         {
         for ( int j=0; j < (int) pAllocSet->m_allocationArray.GetSize(); j++ )
            {
            Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];

            pAlloc->m_allocationSoFar = 0;
            pAlloc->m_areaSoFar = 0;
            pAlloc->m_constraintAreaSatisfied = 0;
            pAlloc->m_minScore   = (float) LONG_MAX;     // min and max score for this allocation
            pAlloc->m_maxScore   = (float) -LONG_MAX;
            pAlloc->m_scoreCount = 0;     // number of scores greater than 0 at any given time
            pAlloc->m_usedCount  = 0;     // number of scores that are actually applied at any given time
            pAlloc->m_scoreArea  = 0;     // area of scores greater than 0 at any given time
            pAlloc->m_scoreBasis  = 0;    // target values of scores greater than 0 at any given time
            pAlloc->m_expandArea  = 0;    // target values of scores greater than 0 at any given time
            pAlloc->m_expandBasis = 0;    // target values of scores greater than 0 at any given time

            for ( int k=0; k < iduCount; k++ )
               {
               pAlloc->m_iduScoreArray[ k ].idu = m_iduArray[ k ];
               pAlloc->m_iduScoreArray[ k ].score = this->m_scoreThreshold-1; 
               }
            }
         }
      }

   // next, score each IDU for each allocation
   // we iterate as follows:
   //  for each IDU...
   //     for each AllocationSet
   //         for each Allocation
   //             apply constraints.  If idu passes...
   //                 evaluate score via preference expressions
   //                 write to the IDU if a column specified for the Allocation

   ////////
   int _countPassedCon = 0;
   ////////

   for ( int m=0; m < iduCount; m++ )
      {
      int idu = m_iduArray[ m ];

      m_pQueryEngine->SetCurrentRecord( idu );
      m_pMapExprEngine->SetCurrentRecord( idu );
      
      for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
         {
         AllocationSet *pAllocSet = m_allocationSetArray[ i ];

         if ( pAllocSet->m_inUse )
            {
            // iterate through the allocation set's allocations
            for ( int j=0; j < (int) pAllocSet->m_allocationArray.GetSize(); j++ )
               {
               Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];

               bool passConstraints = true;
               // first, apply any constraints

               if ( pAlloc->m_constraint.m_pQuery == NULL )   // skip this constraint
                  {
                  passConstraints = false;
                  continue;
                  }

               bool result = false;
               bool ok = pAlloc->m_constraint.m_pQuery->Run( idu, result );
               
               if ( result == false )
                  {
                  passConstraints = false;
                  continue;
                  }               

               // did we pass the constraints?  If so, evaluate preferences
               if ( passConstraints )
                  {
                  float area = 0;
                  m_pMapLayer->GetData( idu, m_colArea, area );
                  pAlloc->m_constraintAreaSatisfied += area;

                  float score = 0;
                  for ( int k=0; k < (int) pAlloc->m_prefsArray.GetSize(); k++ )
                     {
                     Preference *pPref = pAlloc->m_prefsArray[ k ];
                     float value = 0;

                     bool passedQuery = true;
                     if ( pPref->m_pQuery != NULL ) // is the preference associated with a query?
                        {
                        bool result = false;
                        bool ok = pPref->m_pQuery->Run( idu, result );
            
                        if ( !ok || ! result )
                           passedQuery = false;
                        }

                     if ( passedQuery )
                        {
                        if ( pPref->m_pMapExpr )   // is the preference an expression that reference fields in the IDU coverage?
                           {
                           bool ok = m_pMapExprEngine->EvaluateExpr( pPref->m_pMapExpr, false ); // pPref->m_pQuery ? true : false );
                           if ( ok )
                              value += (float) pPref->m_pMapExpr->GetValue();
                           }
                        else
                           value = pPref->m_value;
                        }
                     
                     score += value;      // accumulate preference scores
                     }  // end of: for each preference in this allocation
   
                  pAlloc->m_iduScoreArray[ m ].idu   = idu;
                  pAlloc->m_iduScoreArray[ m ].score = score;

                  // update min/max scores if needed
                  if ( score < pAlloc->m_minScore )
                     pAlloc->m_minScore = score;

                  if ( score > pAlloc->m_maxScore )
                     pAlloc->m_maxScore = score;

                  // if score is valid, update associated stats for this allocation
                  if ( score >= m_scoreThreshold )
                     {
                     /////////////
                     if ( i==0 && j == 0 )
                        scoreCount++;
                     //////////////

                     pAlloc->m_scoreCount++;                    
                     pAlloc->m_scoreArea += area;

                     // target value
                     float targetValue = 0;

                     int targetCol = -1;
                     if ( pAllocSet->m_colTargetBasis >= 0  )
                        targetCol = pAllocSet->m_colTargetBasis;
                     else
                        targetCol = pAlloc->m_colTargetBasis;

                     m_pMapLayer->GetData( idu, targetCol, targetValue );
                     pAlloc->m_scoreBasis += targetValue;
                     }   
                  }  // end of: if (passedConstraints)
               else
                  {  // failed constraint
                  pAlloc->m_iduScoreArray[ idu ].score = m_scoreThreshold-1;    // indicate we failed a constraint
                  }   

               // if field defined for this allocation, populate it with the score
               if ( pAlloc->m_colScores >= 0 )
                  {
                  float newScore = pAlloc->m_iduScoreArray[ idu ].score;                  
                  UpdateIDU( pContext, idu, pAlloc->m_colScores, newScore, ADD_DELTA );
                  }

               }  // end of: for ( each allocation )
            }  // end of: if ( pAllocSet->m_inUse )
         }  // end of: for ( each allocationSet )
      }  // end of:  for ( each IDU )

      ////////
      //AllocationSet *pAS = this->m_allocationSetArray[ 0 ];
      //Allocation *pAlloc = pAS->GetAllocation( 0 );
      //
      //FILE *fp = fopen( "\\Envision\\StudyAreas\\CentralOregon\\Test.csv", "wt" );
      //float maxScore = -2;
      //for ( int i=0; i < iduCount; i++ )
      //   {
      //   if ( pAlloc->m_iduScoreArray[i].score > maxScore )
      //      maxScore = pAlloc->m_iduScoreArray[i].score;
      //
      //   fprintf( fp, "%.1f\n", pAlloc->m_iduScoreArray[ i ].score );
      //   }
      //fclose( fp );
      //         
      /////////
      //CString msg;
      //msg.Format( "MAXSCORE A: %.1f, scoreCount=%i (%i)", maxScore, scoreCount, pAlloc->m_scoreCount );
      //Report::Log( msg, RT_ERROR );
      ///////
 
   return;
   }



void SpatialAllocator::ScoreIduAllocation( Allocation *pAlloc, int flags )
   {
   int iduCount = m_pMapLayer->GetRecordCount();
   pAlloc->m_minScore = float( LONG_MAX );
   pAlloc->m_maxScore = float( -LONG_MAX );

   /////////////////
   int scoreCount = 0;
   /////////////////

   if ( pAlloc->m_iduScoreArray.IsEmpty() )
      pAlloc->m_iduScoreArray.SetSize( m_pMapLayer->GetRecordCount() );
   
   for ( int i=0; i < iduCount; i++ )
      {
      pAlloc->m_iduScoreArray[ i ].idu = i;
      pAlloc->m_iduScoreArray[ i ].score = this->m_scoreThreshold-1; 
  
      int idu = i;

      m_pQueryEngine->SetCurrentRecord( idu );
      m_pMapExprEngine->SetCurrentRecord( idu );

      // first, apply any constraints
      bool passConstraints = true;

      if ( pAlloc->m_constraint.m_pQuery == NULL )   // skip this constraint
         {
         passConstraints = false;
         continue;
         }

      bool result = false;
      bool ok = pAlloc->m_constraint.m_pQuery->Run( idu, result );
      
      if ( result == false )
         {
         passConstraints = false;
         break;
         }               
   
      // did we pass the constraints?  If so, evaluate preferences
      if ( passConstraints )
         {
         //float area = 0;
         //m_pMapLayer->GetData( idu, m_colArea, area );
         //pAlloc->m_constraintAreaSatisfied += area;
         float score = 0;
         for ( int k=0; k < (int) pAlloc->m_prefsArray.GetSize(); k++ )
            {
            Preference *pPref = pAlloc->m_prefsArray[ k ];
            float value = 0;

            bool passedQuery = true;
            if ( pPref->m_pQuery != NULL ) // is the preference associated with a query?
               {
               bool result = false;
               bool ok = pPref->m_pQuery->Run( idu, result );
      
               if ( !ok || ! result )
                  passedQuery = false;
               }

            if ( passedQuery )
               {
               if ( pPref->m_pMapExpr )   // is the preference an expression that reference fields in the IDU coverage?
                  {
                  bool ok = m_pMapExprEngine->EvaluateExpr( pPref->m_pMapExpr, false ); // pPref->m_pQuery ? true : false );
                  if ( ok )
                     value += (float) pPref->m_pMapExpr->GetValue();
                  }
               else
                  value = pPref->m_value;
               }
            
            score += value;      // accumulate preference scores
            }  // end of: for each preference in this allocation
   
         pAlloc->m_iduScoreArray[ idu ].score = score;

         // update min/max scores if needed
         if ( score < pAlloc->m_minScore )
            pAlloc->m_minScore = score;

         if ( score > pAlloc->m_maxScore )
            pAlloc->m_maxScore = score;
         }  // end of: if (passedConstraints)
      else
         {  // failed constraint
         pAlloc->m_iduScoreArray[ idu ].score = -99;    // indicate we failed a constraint
         }
      }  // end of:  for ( each IDU )

   return;
   }


void SpatialAllocator::AllocBestWins( EnvContext *pContext, bool useAddDelta )
   {
   for ( MapLayer::Iterator idu=m_pMapLayer->Begin(); idu < m_pMapLayer->End(); idu++ )
      {
      for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
         {
         AllocationSet *pAllocSet = m_allocationSetArray[ i ];

         if ( pAllocSet->m_inUse )
            {
            int bestIndex = -1;
            float bestScore = 0;
         
            // find the "best" allocation
            for ( int j=0; j < (int) pAllocSet->m_allocationArray.GetSize(); j++ )
               {
               Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];

               float allocationSoFar = 0;
               float currentTarget = 0;
               if ( pAlloc->m_targetSource == TS_USEALLOCATIONSET )
                  {
                  allocationSoFar = pAllocSet->m_allocationSoFar;
                  currentTarget = pAllocSet->m_currentTarget;
                  }
               else
                  {
                  allocationSoFar = pAlloc->m_allocationSoFar;
                  currentTarget = pAlloc->m_currentTarget;
                  }
                  
               // is this one already fully allocated?  Then skip it
               if ( allocationSoFar >= currentTarget )
                  continue;
               
               if ( pAlloc->m_iduScoreArray[ idu ].score > bestScore )
                  {
                  bestIndex = j;
                  bestScore = pAlloc->m_iduScoreArray[ idu ].score;
                  }            
               }  // end of: for ( each allocation )
   
            // done assessing best choice, now allocate it
            if ( bestIndex >= 0 )
               {
               Allocation *pAlloc = pAllocSet->m_allocationArray[ bestIndex ];
               
               if ( useAddDelta )
                  {
                  int currAttrCode = 0;
       
                  m_pMapLayer->GetData( idu, pAllocSet->m_col, currAttrCode );
   
                  if  ( currAttrCode != pAlloc->m_id )
                     AddDelta( pContext, idu, pAllocSet->m_col, pAlloc->m_id );
                  }
               else
                  m_pMapLayer->SetData( idu, pAllocSet->m_col, pAlloc->m_id );
               
               float area = 0, basis = 0;
               m_pMapLayer->GetData( idu, this->m_colArea, area);
               m_pMapLayer->GetData(idu, pAlloc->m_colTargetBasis, basis);

               pAlloc->m_allocationSoFar += basis;
               pAlloc->m_areaSoFar += area;

               if (pAlloc->m_targetSource == TS_USEALLOCATIONSET)
                  {
                  pAllocSet->m_allocationSoFar += basis;
                  pAllocSet->m_areaSoFar += area;
                  }
               }
            }  // end of: if ( pAllocSet->m_inUse )
         }  // end of: for ( each allocationSet )
      }  // end of: for ( each idu );

   return;
   }


void SpatialAllocator::AllocScorePriority( EnvContext *pContext, bool useAddDelta )
   {
   // this gets called once per time step.  It's basic task is to allocate
   //  the various Allocations to each IDU .
   //
   // Before getting here, each Allocation's m_iduArray.score value has been calculated
   // for the current map.  Here, we use these per-Allocation score arrays
   // to create a sorted list of idu scores, best scoring in the front, worst
   // scoring in the rear, repeating form each Allocation.
   // Once this sorting is complete, the algorithm starts allocating 
   // Allocation to IDU, by iterating through the sorted pAlloc->m_iduArray[]'s,
   // tracking allocated areas and maintaining a pointer to the next Allocation
   // to allocate if it's score is sufficiently high relative to the other Allocations
   // it is competing with and there is any remaining allocatable area for the Allocation.
   // When an Allocation is determined for a given IDU, the IDU is updated in the coverage
   // for the pAllocSet->m_col field, with the ID of the "best" Allocation
   int iduCount = m_pMapLayer->GetRecordCount();

      
   ////////
   //AllocationSet *pAS = this->m_allocationSetArray[ 0 ];
   //Allocation *pAl = pAS->GetAllocation( 0 );
   //
   //QueryEngine qe( (MapLayer*) pContext->pMapLayer );
   //Query *pQuery = qe.ParseQuery( pAl->m_constraintArray[0]->m_queryStr, 0, "");
   //
   //float maxScore = -2;
   //float constraintArea = 0;
   //for ( int i=0; i < iduCount; i++ )
   //   {
   //   if ( pAl->m_iduScoreArray[i].score > maxScore )
   //      maxScore = pAl->m_iduScoreArray[i].score;
   //
   //   bool result = false;
   //   pQuery->Run( i, result );
   //
   //   if ( result )
   //      {
   //      float area = 0;
   //      pContext->pMapLayer->GetData( i, m_colArea, area );
   //      constraintArea += area;
   //      }
   //   }
   //CString msg;
   //msg.Format( "MAXSCORE start of asp: %.1f, count=%i, used=%i, constraintArea=%i", maxScore, pAl->m_scoreCount, pAl->m_usedCount, (int) constraintArea );
   //Report::Log( msg, RT_ERROR );
   ///////

   CArray< bool > isIduOpen;           // this contains a flag indicating whether a given IDU has been allocated yet
   isIduOpen.SetSize( iduCount );      // this array is never shuffled, always in IDU order

   // first, for each allocation, sort it's score list
   for ( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      // get an allocation set
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];

      if ( pAllocSet->m_inUse )
         {
         int terminateFlag = 0;     // 0=not terminated, 1=nothing left to allocate, 

         // start by initializing openIDU array for this allocation set
         for ( int k=0; k < iduCount; k++ )
            isIduOpen[ k ] = true; 
         
         int allocCount = (int) pAllocSet->m_allocationArray.GetSize();
   
         // for each allocation in the set, sort the score array
         for ( int j=0; j < allocCount; j++ )
            {
            Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];   // an Allocation defines scores for a particular alloc tpe (e.g. "Corn")
   
            // sort it, so the front of the list is the best scoring 
            qsort( pAlloc->m_iduScoreArray.GetData(), iduCount, sizeof( IDU_SCORE ), CompareScores );
          
            //////////////////////////////
            // dump the allocations
            //int colLulcA = m_pMapLayer->GetFieldCol( "LULC_A" );
            //for ( int l=0; l < 10; l++ )
            //   {
            //   int lulc = 0;
            //   m_pMapLayer->GetData( pAlloc->m_iduScoreArray[ l ].idu, colLulcA, lulc );
            //
            //   CString msg;
            //   msg.Format( "Score: %6.2f  IDU: %i   LULC_A: %i\n", pAlloc->m_iduScoreArray[ l ].score, pAlloc->m_iduScoreArray[ l ].idu, lulc ); //  );
            //   TRACE( msg );
            //   }

            //for ( int l=0; l < 20; l++ )
            //   {
            //   CString msg;
            //   msg.Format( "Score: %6.2f  IDU: %i\n", pAlloc->m_iduScoreArray[ l ].score, pAlloc->m_iduScoreArray[ l ].idu ); //  );
            //   TRACE( msg );
            //   }


            //////////////////////////////////////

            // and initialize runtime monitor variables
            pAlloc->m_currentIduScoreIndex = 0;  // this indicates we haven't pulled of this list yet
            }
  
         // now, cycle through lists, tracking best score, until everything is allocated
         bool stillAllocating = true;
         float lastBestScore = (float) LONG_MAX;
   
         while ( stillAllocating )    // start looping through the allocation lists until they are fully allocated
            {
            // get the Allocation with the best idu score by looking through the allocation score arrays
            // Basic idea is that each allocation has a sorted list that, at the front, lists
            // that allocation's best score and corresponding IDU.  All list start out pointing to the 
            // front of the list.  We then start pulling of list entries, keeping track of the current "best"
            // score.  We pop an item of an Allocation's array if the allocation score is 
            // better than any other currently active allocation's score.  An Allocation score is
            // "active" if it has not been fully allocated.
            // Once a best score is identified, the corresponding allocation is applied to the IDU,
            // and the IDU is marked a no longer available.  
            // at the end of each step, the allocation stack are checked to see if any of the top IDUs 
            // are no longer available - if so, those are popped of the list.
   
            Allocation *pBest = NULL;
            float currentBestScore = (float) -LONG_MAX;
   
            for ( int j=0; j < allocCount; j++ )
               {
               Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];
   
               // if this Allocation has already been fully consumed, don't consider it
               if ( pAlloc->m_currentIduScoreIndex >= iduCount )
                  continue;

               // if this Allocation is aleady fully allocated, don't consider it further
               if ( pAlloc->m_targetSource != TS_USEALLOCATIONSET 
                 && pAlloc->m_allocationSoFar >= pAlloc->m_currentTarget )
                  continue;
   
               // if the IDU that represents the current best score for this Allocation is 
               // not available (already allocated), don't consider it
               int currentBestIDU = pAlloc->GetCurrentIdu();
               ASSERT( currentBestIDU >= 0 );
   
               if ( isIduOpen[ currentBestIDU ] == false )
                  continue;
   
               // if this Allocation's current best score <= the current best, don't consider it,
               // some other allocation has a higher score
               float currentScore = pAlloc->GetCurrentScore();

               if ( currentScore <= currentBestScore )
                  {
                  //TRACE3( "SA: Allocation %s - this allocations current best score (%f) is less than the current best score (%f)\n", 
                  //   (LPCTSTR) pAlloc->m_name, pAlloc->m_iduScoreArray[ pAlloc->m_currentIduScoreIndex ].score, currentBestScore );
                  continue;
                  }
                  
               if ( currentScore < this->m_scoreThreshold )  // are we into the failed scores? (didn't pass the constraints?)
                  continue;

               // it passes, so this allocation becomes the leading candidate so far
               pBest = pAlloc;
               currentBestScore = currentScore;
               }  // end of: for each Allocation
   
            if ( pBest == NULL )
               {
               terminateFlag = 1; 
               break;      // nothing left to allocate
               }

            // at this point, we've ID'd the best next score and associated Allocation.  Next, allocate it
            int idu = pBest->GetCurrentIdu();
            isIduOpen[ idu ] = false;
   
            lastBestScore = pBest->GetCurrentScore();
            
            // apply the allocation to the IDUs 
            int currAttrCode = -1;
            m_pMapLayer->GetData( idu, pAllocSet->m_col, currAttrCode );
            
            int newAttrCode = pBest->m_id;    // this is the value to write to the idu col
            int newSequenceID = -1;
            int currSequenceID = -1;
   
            // if this is a sequence, figure out where we are in the sequence
            if ( pBest->IsSequence() )
               {
               m_pMapLayer->GetData( idu, pAllocSet->m_colSequence, currSequenceID );
   
               int seqIndex = 0;
               if ( currSequenceID == pBest->m_id )   // are we already in the sequence?
                  {
                  BOOL found = pBest->m_sequenceMap.Lookup( currAttrCode, seqIndex );
                  ASSERT( found );
   
                  seqIndex++;    // move to the next item in the sequence, wrapping if necessary
                  if ( seqIndex >= (int) pBest->m_sequenceArray.GetSize() )
                     seqIndex = 0;
   
                  newAttrCode = pBest->m_sequenceArray[ seqIndex ];
                  newSequenceID = pBest->m_id;
                  }
               else  // starting a new sequence
                  {
                  newAttrCode = pBest->m_sequenceArray[ 0 ];
                  newSequenceID = pBest->m_id;
                  }
               }
   
            // ok, new attrCode and sequenceID determined, write to the IDU coverage
            this->UpdateIDU( pContext, idu, pAllocSet->m_col, newAttrCode, useAddDelta ? ADD_DELTA : SET_DATA );
            
            if ( pAllocSet->m_colSequence >= 0 && currSequenceID != newSequenceID )
               this->UpdateIDU( pContext, idu, pAllocSet->m_colSequence, newSequenceID, useAddDelta ?ADD_DELTA : SET_DATA );

            // update internals with allocation basis value

            float area=0, basis=0;
            m_pMapLayer->GetData( idu, this->m_colArea, area );
            m_pMapLayer->GetData(idu, pBest->m_colTargetBasis, basis);

            pBest->m_allocationSoFar += basis;   // accumulate the basis target of this allocation
            pBest->m_areaSoFar += area;   // accumulate the basis target of this allocation
            pBest->m_usedCount++;

            if (pBest->m_targetSource == TS_USEALLOCATIONSET)
               {
               pAllocSet->m_allocationSoFar += basis;
               pAllocSet->m_areaSoFar += area;
               }

            // Expand if needed
            if (pBest->m_useExpand)
               {
               float expandArea = 0;
               float expandAreaBasis = Expand(idu, pBest, useAddDelta, currAttrCode, newAttrCode, currSequenceID, newSequenceID, pContext, isIduOpen, expandArea);

               pBest->m_allocationSoFar += expandAreaBasis;   // accumulate the basis target of this allocation
               pBest->m_areaSoFar += expandArea;        // accumulate the area of this allocation
               pBest->m_expandBasis += expandAreaBasis;
               pBest->m_expandArea += expandArea;

               if (pBest->m_targetSource == TS_USEALLOCATIONSET)
                  {
                  pAllocSet->m_allocationSoFar += expandAreaBasis;
                  pAllocSet->m_areaSoFar += expandArea;
                  }
               }

            pBest->m_currentIduScoreIndex++;    // pop of the front of the list for the best allocation   

            // generate notification for any scoring listeners
            float currentTarget = pBest->GetCurrentTarget();
            float remaining = currentTarget - pBest->m_allocationSoFar;
            float pctAllocated = -1;
            if (currentTarget > 0 )
               pctAllocated = 100*pBest->m_allocationSoFar/currentTarget;

            Notify( 1, idu, currentBestScore, pBest, remaining, pctAllocated );

            // are we done yet?
            stillAllocating = false;
   
            for ( int j=0; j < allocCount; j++ )
               {
               // we look at the allocationSet first, to see if the target is defined there.

               // is the allocation global?
               Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];

               float currentTarget = 0;
               float allocationSoFar = 0;
               if ( pAlloc->m_targetSource == TS_USEALLOCATIONSET )
                  {
                  currentTarget = pAllocSet->m_currentTarget;
                  allocationSoFar = pAllocSet->m_allocationSoFar;
                  }
               else
                  {
                  currentTarget = pAlloc->m_currentTarget;
                  allocationSoFar = pAlloc->m_allocationSoFar;
                  }
   
               if ( allocationSoFar < currentTarget )
                  {
                  // move past any orphaned IDU_SCORE entries
                  int idu = pAlloc->GetCurrentIdu();
                  if ( idu >= iduCount || idu < 0 )
                        break;

                  bool isOpen = isIduOpen[ idu ];
      
                  while ( isOpen == false )     // loop through "closed" idus to find the first "open" (available) idu)
                     {
                     pAlloc->m_currentIduScoreIndex++;
      
                     if ( pAlloc->m_currentIduScoreIndex >= iduCount )
                        break;
      
                     idu = pAlloc->GetCurrentIdu();

                     if ( idu >= iduCount || idu < 0 )
                        break;

                     isOpen = isIduOpen[ idu ];
                     }
                  }
               }  // end of: for ( j < allocationCount )
   
            // check to see if allocations still needed
            for ( int j=0; j < allocCount; j++ )
               {
               Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];

               float currentTarget = 0;
               float allocationSoFar = 0;
               if ( pAlloc->m_targetSource == TS_USEALLOCATIONSET )
                  {
                  currentTarget = pAllocSet->m_currentTarget;
                  allocationSoFar = pAllocSet->m_allocationSoFar;
                  }
               else
                  {
                  currentTarget = pAlloc->m_currentTarget;
                  allocationSoFar = pAlloc->m_allocationSoFar;
                  }
   
               if ( ( allocationSoFar < currentTarget )  // not fully allocated already?
                 && ( pAlloc->m_currentIduScoreIndex < iduCount ) )          // and still on the list?
                  {
                  stillAllocating = true;
                  break;
                  }
               }
            }  // end of:  while ( stillAllocating )

         // dump some results

         // temporary!
         //int outcount = 0;
         //if ( i == 0 )     // first allocationset only
         //   {
         //   Allocation *pAlloc = pAllocSet->GetAllocation( 0 );
         //   for ( int j=0; j < iduCount; j++ )
         //      {
         //      //if ( pAlloc->m_iduScoreArray[j].score >= this->m_scoreThreshold )
         //      //   {
         //         CString msg;
         //         msg.Format( "Score: %6.2f  Rank: %i\n", pAlloc->m_iduScoreArray[j].score, j );
         //         Report::Log( msg, RT_ERROR );
         //         outcount++;
         //      //   }
         //
         //      if ( outcount > 20 )
         //         break;
         //      }
         //
         //   CString msg;
         //   msg.Format( "Current Score Rank (%s) = %i, UsedCount=%i\n", (PCTSTR) pAlloc->m_name, pAlloc->m_currentIduScoreIndex, pAlloc->m_usedCount );
         //   Report::Log( msg, RT_ERROR );               
         //   }
         }     // end of:  if ( pAllocSet->m_inUse )
      }  // end of: for ( each AllocationSet )
   }


bool SpatialAllocator::LoadXml( LPCTSTR filename, PtrArray< AllocationSet > *pAllocSetArray /*=NULL */)
   {
   // search for file along path
   CString path;
   int result = PathManager::FindPath( filename, path );  // finds first full path that contains passed in path if relative or filename, return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 

   if ( result < 0 )
      {
      CString msg( "  Spatial Allocator: Unable to find input file " );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   TiXmlDocument doc;
   bool ok = doc.LoadFile( path );

   if ( ! ok )
      {      
      Report::ErrorMsg( doc.ErrorDesc() );
      return false;
      }

   if ( pAllocSetArray == NULL )
      pAllocSetArray = &( this->m_allocationSetArray );

   CString msg( "  Spatial Allocator: Loading input file " );
   msg += path;
   Report::Log( msg );

   m_xmlPath = path;
 
   // start interating through the nodes
   TiXmlElement *pXmlRoot = doc.RootElement();  // spatial_allocator

   LPTSTR areaCol = NULL; //, scoreCol=NULL;
   LPTSTR method  = NULL;
   LPTSTR updatefromfile = NULL;
   int    shuffleIDUs = 1;
   XML_ATTR rattrs[] = { // attr          type           address       isReq checkCol
                      { "area_col",     TYPE_STRING,   &areaCol,       false, CC_MUST_EXIST | TYPE_FLOAT },
                      //{ "score_col",    TYPE_STRING,   &scoreCol,      false, CC_AUTOADD    | TYPE_FLOAT },
                      { "method",       TYPE_STRING,   &method,        false, 0 },
                      { "shuffle_idus", TYPE_INT,      &shuffleIDUs,   false, 0 },
                      { "update_from_file", TYPE_STRING,   &updatefromfile,        false, 0 },
                      { NULL,           TYPE_NULL,     NULL,           false, 0 } };

   if ( TiXmlGetAttributes( pXmlRoot, rattrs, path, m_pMapLayer ) == false )
      return false;
   
   if ( areaCol == NULL )
      areaCol = "AREA";
   if (updatefromfile != NULL)
     {
      m_pBuildFromFile = new FDataObj();
      m_pBuildFromFile->ReadAscii(updatefromfile);
      m_update_from_file_name = updatefromfile;
      }

   this->m_colArea = m_pMapLayer->GetFieldCol( areaCol );
   if ( m_colArea < 0 )
      {
      CString msg( "  Spatial Allocator: unable to find AREA field in input file" );
      msg += filename;
      Report::ErrorMsg( msg );
      return false;
      }

   //if ( scoreCol )
   //   m_colScore = m_pMapLayer->GetFieldCol( scoreCol );


   ALLOC_METHOD am = AM_SCORE_PRIORITY;
   if ( method != NULL )
      {
      switch( method[ 0 ] )
         {
         case 's':       // score priority - already the default
            break;

         case 'b':       // best wins
            am = AM_BEST_WINS;
            break;
         }
      }

   this->m_allocMethod = am;

   if ( shuffleIDUs )
      {
      m_shuffleIDUs = true;
      m_pRandUnif = new RandUniform( 0.0, (double) m_pMapLayer->GetRecordCount(), 0 );
      }
   else
      m_shuffleIDUs = false;

   // allocation sets
   TiXmlElement *pXmlAllocSet = pXmlRoot->FirstChildElement( "allocation_set" );
   if ( pXmlAllocSet == NULL )
      {
      CString msg( "  Spatial Allocator: missing <allocation_set> element in input file " );
      msg += path;
      Report::ErrorMsg( msg );
      return false;
      }

   while ( pXmlAllocSet != NULL )
      {
      // lookup fields
      LPTSTR name     = NULL;
      LPTSTR field    = NULL;
      LPTSTR seqField = NULL;
      LPTSTR targetSource = NULL;
      LPTSTR targetType   = NULL;
      LPTSTR targetValues = NULL;
      LPTSTR targetBasis  = NULL;
      LPTSTR description  = NULL;
      int use = 1;

      XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                         { "name",          TYPE_STRING,   &name,         true,  0 },
                         { "use",           TYPE_INT,      &use,          false, 0 },
                         { "description",   TYPE_STRING,    &description,  false,  0 },
                         { "col",           TYPE_STRING,   &field,        true,  CC_MUST_EXIST | TYPE_LONG },
                         { "sequence_col",  TYPE_STRING,   &seqField,     false, CC_AUTOADD | TYPE_LONG },
                         // these are optional, only specify if using global (meaning allocation_set) targets
                         { "target_type",   TYPE_STRING,   &targetType,   false,  0 },  // deprecated
                         { "target_source", TYPE_STRING,   &targetSource, false,  0 },
                         { "target_basis",  TYPE_STRING,   &targetBasis,  false,  0 },
                         { "target_values", TYPE_STRING,   &targetValues, false,  0 },

                         { NULL,           TYPE_NULL,     NULL,          false, 0 } };

      if ( TiXmlGetAttributes( pXmlAllocSet, attrs, path, m_pMapLayer ) == false )
         return false;

      AllocationSet *pAllocSet = new AllocationSet( this, name, field );
      pAllocSet->m_col = m_pMapLayer->GetFieldCol( field );
      pAllocSet->m_inUse = use ? true : false;

      pAllocSet->m_targetSource = TS_USEALLOCATION;
      if ( targetSource == NULL )
         targetSource = targetType;

      pAllocSet->SetTargetParams( m_pMapLayer, targetBasis, targetSource, targetValues, NULL, NULL );      
      
      if ( description != NULL )
         pAllocSet->m_description = description;

      if ( seqField != NULL )
         {
         pAllocSet->m_seqField = seqField;
         pAllocSet->m_colSequence = m_pMapLayer->GetFieldCol( seqField );
         }
      
      pAllocSetArray->Add( pAllocSet );

      // have allocationset set up, get allocations
      TiXmlElement *pXmlAlloc = pXmlAllocSet->FirstChildElement( "allocation" );
      if ( pXmlAlloc == NULL )
         {
         CString msg( "  Spatial Allocator: missing <allocation> element in input file - at least one allocation is required..." );
         msg += path;
         Report::ErrorMsg( msg );
         return false;
         }

      while ( pXmlAlloc != NULL )
         {
         // lookup fields
         LPTSTR name   = NULL;
         LPTSTR description = NULL;
         int attrCode  = -1;
         LPTSTR targetSource = NULL;
         LPTSTR targetType   = NULL;
         LPTSTR targetValues = NULL;
         LPTSTR targetBasis  = NULL;
         LPTSTR targetDomain = NULL;
         LPTSTR targetQuery  = NULL;
         LPTSTR constraint   = NULL;
         LPCTSTR scoreCol    = NULL;
         LPCTSTR seq   = NULL;

         LPCTSTR expandQuery = NULL;
         //float   expandMaxArea  = -1;
         LPCTSTR expandArea = NULL;

         float initPctArea = 0;

         XML_ATTR attrs[] = { // attr          type           address       isReq checkCol
                            { "name",          TYPE_STRING,   &name,         true,  0 },
                            { "id",            TYPE_INT,      &attrCode,     true,  0 },
                            { "description",   TYPE_STRING,   &description,  false, 0 },
                            { "target_type",   TYPE_STRING,   &targetType, false, 0 },   // deprecated
                            { "target_source", TYPE_STRING,   &targetSource, false, 0 },
                            { "target_values", TYPE_STRING,   &targetValues, false, 0 },
                            { "target_domain", TYPE_STRING,   &targetDomain, false, 0 },
                            { "target_basis",  TYPE_STRING,   &targetBasis,  false, 0 },
                            { "target_query",  TYPE_STRING,   &targetQuery,  false, 0 },
                            { "constraint",    TYPE_STRING,   &constraint,   false, 0 },
                            { "score_col",     TYPE_STRING,   &scoreCol,     false, CC_AUTOADD | TYPE_FLOAT },
                            { "sequence",      TYPE_STRING,   &seq,          false, 0 },
                            { "init_pct_area", TYPE_FLOAT,    &initPctArea,  false, 0 },
                            { "expand_query",  TYPE_STRING,   &expandQuery,  false, 0 },
                            { "expand_area",   TYPE_STRING,   &expandArea,   false, 0 },
                            { NULL,            TYPE_NULL,     NULL,        false, 0 } };
      
         if ( TiXmlGetAttributes( pXmlAlloc, attrs, path, m_pMapLayer ) == false )
            return false;

         // is a target specified?  Okay if allocation set has targets defined.  An AllocationSet assumes the
         // Allocation defines the source if the AllocationSet source is UNDEFINED or USEALLOCATION 
         bool ok = true;
         //if ( targetSource == NULL && pAllocSet->m_targetSource != TS_UNDEFINED && pAllocSet->m_targetSource != TS_USEALLOCATION )
         if ( targetSource == NULL )
            targetSource = targetType;

         if ( targetSource == NULL && ( pAllocSet->m_targetSource == TS_UNDEFINED || pAllocSet->m_targetSource == TS_USEALLOCATION ) )
            {
            ok = false;
            CString msg;
            msg.Format( "  Spatial Allocator: missing 'target_source' attribute for allocation %s", name );
            Report::ErrorMsg( msg );
            }

         if ( ok )
            {
            Allocation *pAlloc = new Allocation( pAllocSet, name, attrCode ); //, ttype, targetValues );

            if ( description != NULL )
               pAllocSet->m_description = description;
   
            if ( targetSource == NULL )
               targetSource = targetType;
           
            pAlloc->SetTargetParams( m_pMapLayer, targetBasis, targetSource, targetValues, targetDomain, targetQuery );

            // score column (optional)
            if ( scoreCol != NULL )
               {
               pAlloc->m_scoreCol = scoreCol;
               m_pMapLayer->CheckCol( pAlloc->m_colScores, scoreCol, TYPE_FLOAT, CC_AUTOADD );
               }

            if ( seq != NULL ) // is this a sequence?  then parse out attr codes
               {
               TCHAR *buffer = new TCHAR[ lstrlen( seq ) + 1 ];
               lstrcpy( buffer, seq );
               TCHAR *next = NULL;
               TCHAR *token = _tcstok_s( buffer, _T(","), &next );
               while ( token != NULL )
                  {
                  int attrCode = atoi( token );
                  int index = (int) pAlloc->m_sequenceArray.Add( attrCode );
                  pAlloc->m_sequenceMap.SetAt( attrCode, index );
                  token = _tcstok_s( NULL, _T( "," ), &next );
                  }
               delete [] buffer;

               pAlloc->m_initPctArea = initPctArea;
               }

            // constraint
            if ( constraint != NULL )
               pAlloc->m_constraint.SetConstraintQuery( constraint );
            
            // expansion query
            if ( expandQuery != NULL )
               pAlloc->SetExpandQuery( expandQuery );
               
            // was a string provided describing the max area expansion?  If so, parse it.
            if ( expandArea != NULL )
               pAlloc->SetMaxExpandAreaStr( expandArea );
                  
            // all done, add allocation to set's array
            pAllocSet->m_allocationArray.Add( pAlloc );

            // next, get any preferences
            TiXmlElement *pXmlPref = pXmlAlloc->FirstChildElement( "preference" );
      
            while ( pXmlPref != NULL )
               {
               // lookup fields
               LPTSTR name   = NULL;
               LPTSTR query  = NULL;
               LPTSTR wtExpr = NULL;
      
               XML_ATTR attrs[] = { // attr          type           address   isReq checkCol
                                  { "name",          TYPE_STRING,   &name,    true,  0 },
                                  { "query",         TYPE_STRING,   &query,   true,  0 },
                                  { "weight",        TYPE_STRING,   &wtExpr,  true,  0 },
                                  { NULL,            TYPE_NULL,     NULL,          false, 0 } };
            
               if ( TiXmlGetAttributes( pXmlPref, attrs, path ) == false )
                  return false;
               
               Preference *pPref = new Preference( pAlloc, name, query, wtExpr );
               pAlloc->m_prefsArray.Add( pPref );

               pXmlPref = pXmlPref->NextSiblingElement( "preference" );
               }

            // next, get any constraints  NOTE: THIS HAS BEEN DEPRECATED
            TiXmlElement *pXmlConstraint = pXmlAlloc->FirstChildElement( "constraint" );
            bool hasConstraints = false;
            while ( pXmlConstraint != NULL )
               {
               // lookup fields
               LPTSTR name  = NULL;
               LPTSTR query = NULL;
      
               XML_ATTR attrs[] = { // attr          type           address   isReq checkCol
                                  { "name",          TYPE_STRING,   &name,    true,  0 },
                                  { "query",         TYPE_STRING,   &query,   true,  0 },
                                  { NULL,            TYPE_NULL,     NULL,     false, 0 } };
            
               if ( TiXmlGetAttributes( pXmlConstraint, attrs, path ) == false )
                  return false;
               
               //Constraint *pConstraint = new Constraint( name, query );
               pAlloc->m_constraint.m_queryStr += query;
               hasConstraints = true;

               pXmlConstraint = pXmlConstraint->NextSiblingElement( "constraint" );
               }
            if ( hasConstraints )
               pAlloc->m_constraint.CompileQuery();
            }  // end of: if ( ok )

         pXmlAlloc = pXmlAlloc->NextSiblingElement( "allocation" );
         }

      pXmlAllocSet = pXmlAllocSet->NextSiblingElement( "allocation_set" );
      }

   return true;
   }

   // The method can be called each timestep (it will be if m_pBuildFromFile is not NULL.  A tag has been added to the LoadXml code:

  // <spatial_allocator area_col = "AREA" method = "score priority" shuffle_idus = "1" update_from_file = "D:\\Envision\\StudyAreas\\CALFEWS\\calvin\\AgModel_Envision.csv">
  //    <allocation_set name = "AgCrops" col = "SWAT_ID" use = "1" >
  //    <allocation name = "Almonds and Pistachios" id = "106" target_source = "timeseries" target_values = "(1950,50),(2100,50)"><constraint name = "Ag" query = "LULC_A=7 {Agriculture} " / ><preference name = "Current" query = "SWAT_ID = 106 {Almonds}" weight = "0.75" / >< / allocation>
  //    <allocation name = "Almonds and Pistachios" id = "106" target_source = "timeseries" target_values = "(1950,50),(2100,50)">
  //    <constraint name = "Ag" query = "LULC_A=7 {Agriculture} " / >
 //     <preference name = "Current" query = "SWAT_ID = 106 {Almonds}" weight = "0.75" / >
 //     < / allocation>
// Follow with additional allocations
      

   // m_update_from_file_name is a csv file written by separate process each year
   // Use LoadXml to build Allocation - the size of the allocationSet/allocation should match the size of m_update_from_file_name
   // This code modifies the existing allocations with values from m_update_from_file_name
   // The values for m_pTargetData are set to the same values as defined by the external process.  This assumes the allocations were specified
   // as a time series with 2 points.  Because the 2 values in the updated time series are equivalent, the sampling will return the same value.
bool SpatialAllocator::UpdateAllocationsFromFile()
   {
   if (m_pBuildFromFile != NULL)
      {
      m_pBuildFromFile->Clear();
      m_pBuildFromFile->ReadAscii(m_update_from_file_name);
      int sz2 = m_pBuildFromFile->GetRowCount();
      for (int i = 0; i < (int)m_allocationSetArray.GetSize(); i++)
         {
         AllocationSet* pAllocSet = m_allocationSetArray[i];
         for (int j = 0; j < (int)pAllocSet->m_allocationArray.GetSize(); j++)
            {
            //Update the lulc id, target values, constraints, and preferences
            Allocation* pAlloc = pAllocSet->m_allocationArray[j];
            int sz1 = (int)pAllocSet->m_allocationArray.GetSize(); 
            ASSERT(sz1 == sz2);
            pAlloc->m_id = m_pBuildFromFile->GetAsInt(1, j);
            CString nam;
            nam.Format("SWAT Code %i CVPM %i", pAlloc->m_id, m_pBuildFromFile->GetAsInt(0, i));
            pAlloc->m_name = nam;
           // pAlloc->m_targetValues = m_pBuildFromFile->GetAsString(3, j);
            CString query;
            query.Format("LULC_A = 7 and CVPM_INT = %i", m_pBuildFromFile->GetAsInt(0, i));//find correct CVPM and constrain to Ag lands
            //pAlloc->m_pTargetQuery->m_value = query;
            pAlloc->m_constraint.m_queryStr = query;
            Preference* pPref = pAlloc->m_prefsArray.GetAt(0);
            CString prefQuery;
            prefQuery.Format("SWAT_ID = %i", m_pBuildFromFile->GetAsInt(1, i));//preference for land already growing this crop
            pPref->m_queryStr = prefQuery;
            if (pAllocSet->m_inUse && pAlloc->m_pTargetData != NULL)
               //pAlloc->SetTarget(m_pBuildFromFile->GetAsFloat(1, j));//area to disaggregate
               {
               pAlloc->m_pTargetData->Set(1, 0, m_pBuildFromFile->GetAsFloat(1, j));
               pAlloc->m_pTargetData->Set(1, 1, m_pBuildFromFile->GetAsFloat(1, j));
               }
            else
               pAlloc->m_currentTarget = 0;
            }
         }
      }
   return true;
}

bool SpatialAllocator::SaveXml( LPCTSTR path )
   {
   //check to see if is xml
   LPCTSTR extension = _tcsrchr( path, '.' );
   if ( extension && lstrcmpi( extension+1, "xml" ) != 0 )
      {
      CString msg( "Error saving allocationsets - Unsupported File Extension (" );
      msg += path;
      msg += ")";
      Report::ErrorMsg( msg );
      return 0;
      }

   // open the file and write contents
   FILE *fp;
   fopen_s( &fp, path, "wt" );
   if ( fp == NULL )
      {
      LONG s_err = GetLastError();
      CString msg = "Failure to open ";
      msg += path;
      msg += " for writing.";
      Report::SystemErrorMsg  ( s_err, msg );
      return false;
      }
   
   int count = SaveXml( fp );
   fclose( fp );
   
   return true;
   }

//-----------------------------------------------------------------------
// SpatialAllocator::SaveXml() = FILE* version
//-----------------------------------------------------------------------

int SpatialAllocator::SaveXml( FILE *fp )
   {
   bool useXmlStrings = false;
   
   ASSERT( fp != NULL );
   fputs( "<?xml version='1.0' encoding='utf-8' ?>\n\n", fp );
   fputs( "<!--\n", fp );
   fputs( "<spatial_allocator> \n", fp );
   fputs( "   area_col:   name of column with IDU areas\n", fp );
   fputs( "   method:      'score priority' (default) or 'best wins'\n\n", fp );

   fputs( "'AllocationSets' define groups of allocations \n", fp );
   fputs( "  <allocation_set> \n", fp );
   fputs( "     name:         name of column with IDU areas\n", fp );
   fputs( "     col:          IDU column associated with this AllocationSet, populated with Allocation ID\n\n", fp );

   fputs( "   An AllocationSet is defined by specifying the things to allocate (Allocations)\n", fp );
   fputs( "   An Allocation defines the things to allocate.  This can be a single item or a sequence (e.g. a crop rotation)\n", fp );
   fputs( "   An Allocation has the following attributes:\n", fp );
   fputs( "     <allocation> \n", fp );
   fputs( "        name:          name of the item to be allocated (required) \n", fp );
   fputs( "        id:            unique identifer for the item (required)\n", fp );
   fputs( "                       Notes:  1) if the item is NOT a sequence, then the ID is the attribute code for the item.\n", fp );
   fputs( "                              2) if the item IS a sequence, then the ID is not used to identify initial \n", fp );
   fputs( "        target_source: 'rate' - indicates the value specified in the 'target_values' attribute is a growth rate\n", fp );
   fputs( "                       'file' - load from a conformant CSV file, filename specified in 'target_values'.  Conformat\n", fp );
   fputs( "                                files are text files, first row containing column headers, remaining rows containing\n", fp );
   fputs( "                                time/target value pairs, where time=calendar year or 0-based, depending on whether a start year is specified in the\n", fp );
   fputs( "                                envx file), and target value= landscape target for the iteming being allocated\n", fp );
   fputs( "                       'timeseries' - use specified time series value pairs specified in the 'target_values' attribute  \n", fp );                      
   fputs( "                        (required)\n", fp );
   fputs( "        target_values: for rate types, indicates the rate of growth of the item being allocated (decimal percent) (req)\n", fp );
   fputs( "                       for file types, indicates full path to file.\n", fp );
   fputs( "                       for timeseries type, a list of (time,value) pairs eg. '(0,500), (20,600), (40,800)'\n", fp );
   fputs( "                       (required)\n", fp );
   fputs( "        target_col:    IDU field used to calculate targets.  This is what is accumulated during the allocation\n", fp );
   fputs( "                       process, measured in units of the target.  This can be thought of as an IDU field that\n", fp );
   fputs( "                       stores what is being specified by the allocation target.  For example, if the target is\n", fp );
   fputs( "                       measured in $, this would specified a field that has the cost of the item being allocated\n", fp );
   fputs( "                       for each IDU.  The the targets are areas, then the field containing the area of the IDU would \n", fp );
   fputs( "                       be specified (optional, defaults to 'AREA' ).\n", fp );
   fputs( "        score_col:    IDU column holding calculated score for the allocation (optional)\n", fp );
   fputs( "        sequence:      for sequences, a comma-separated list of ID's defining the sequence (e.g. a crop rotation)\n", fp );
   fputs( "        init_pct_area: for sequences, the percent of the initial land area that is in a sequence\n", fp );
   fputs( "        expand_query:  indicates an allocation should be expanded when applied, to neighbor cells that satisfy the query.  Can use kernal references!\n", fp );
   fputs( "        expand_area:   indicates the maximum area allowed for a expansion. can be a fixed value (eg 400) or a distribution.\n", fp );
   fputs( "                        Distributions include Uniform: 'U(min,max)', Normal: 'N(mean,stdev)', Weibull: 'W(a,b)', LogNormal: 'L(mean,stdev)', Exponential: 'E(mean)'\n\n", fp );
   
   fputs( "**NOTE** THIS IS A MACHINE_GENERATED FILE AND MAY BE OVERWRITEN BY SOME FUTURE PROCESS\n", fp );
   fputs( "-->\n\n", fp );

   //----- main <spatial_allocator> tag --------
   LPCTSTR areaCol = this->m_pMapLayer->GetFieldLabel( this->m_colArea );
   LPCTSTR method = "score priority";
   if ( this->m_allocMethod == ALLOC_METHOD::AM_BEST_WINS )
      method = "best wins";
   int shuffle = this->m_shuffleIDUs ? 1 : 0;

   fprintf( fp, "<spatial_allocator area_col='%s' method='%s' shuffle_idus='%i'>\n\n", areaCol, method, shuffle );

   // allocation sets next
   for ( int i=0; i < (int) this->m_allocationSetArray.GetSize(); i++ )
      {
      //--------------------------------------------------------
      //--- <allocation_set> -----------------------------------
      //--------------------------------------------------------
      AllocationSet *pSet = this->m_allocationSetArray[ i ];

      CString xmlStr;
      GetXmlStr((LPCTSTR)pSet->m_name, xmlStr);
      fprintf( fp, "<allocation_set name='%s' col='%s' use='%i' ", (LPCTSTR) xmlStr, (LPCTSTR) pSet->m_field, pSet->m_inUse ? 1 : 0 );

      if ( pSet->m_description.GetLength() > 0 )
         {
         GetXmlStr((LPCTSTR)pSet->m_description, xmlStr);
         fprintf( fp, "\n      description='%s'", (LPCTSTR) xmlStr );
         }

      if ( pSet->m_seqField.GetLength() > 0 )
         fprintf( fp, "\n      seqField='%s'", (LPCTSTR) pSet->m_seqField );

      if ( pSet->m_targetSource != TS_UNDEFINED )
         {
         LPCTSTR ts = NULL, tv = NULL, tb=NULL, td=NULL;
         switch( pSet->m_targetSource )
            {
            case TS_TIMESERIES:    ts = "timeseries";       tv = (LPCTSTR) pSet->m_targetValues;  break;
            case TS_RATE:          ts = "rate";             tv = (LPCTSTR) pSet->m_targetValues;  break;
            case TS_FILE:          ts = "file";             tv = (LPCTSTR) pSet->m_targetValues;  break;
            case TS_USEALLOCATIONSET:   ASSERT( 0 );       break;
            case TS_USEALLOCATION: ts = "useAllocation";   break;
            }

         switch( pSet->m_targetDomain )
               {
               case TD_UNDEFINED:      break;               
               case TD_AREA:           td = "area";       break;
               case TD_PCTAREA:        td = "pctArea";    break;
               case TD_ATTR:           td = "attribute";  break;   // unused at this point
               case TD_USEALLOCATION:  ASSERT( 0 );   break;
               }

         if ( pSet->m_targetBasisField.IsEmpty() == false )
            tb = pSet->m_targetBasisField;

         if ( ts != NULL )
            fprintf( fp, "\n      target_source='%s'", ts );
         
         if ( tv != NULL )
            fprintf( fp, "\n      target_values='%s'", tv );

         if ( td != NULL )
            fprintf( fp, "\n      target_domain='%s'", td );

         if ( tb != NULL )
            fprintf( fp, "\n      target_basis='%s'", tb );
         }

      fputs( " >\n\n", fp );  // end of <allocation_set>

      //--------------------------------------------------------
      // Next - <allocation> -----------------------------------
      //--------------------------------------------------------
      for ( int j=0; j < pSet->GetAllocationCount(); j++ )
         {
         Allocation *pAlloc = pSet->GetAllocation( j );

         LPCTSTR ts = NULL, td=NULL, tb=NULL, tv=NULL;
         if ( pAlloc->m_targetSource != TS_UNDEFINED )
            {
            switch( pAlloc->m_targetSource )
               {
               case TS_TIMESERIES:    ts = "timeseries";  tv=(LPCTSTR) pAlloc->m_targetValues;  break;
               case TS_RATE:          ts = "rate";        tv=(LPCTSTR) pAlloc->m_targetValues;  break;
               case TS_FILE:          ts = "file";        tv=(LPCTSTR) pAlloc->m_targetValues;  break;
               case TS_USEALLOCATIONSET: ts = "useAllocationSet";   break;
               case TS_USEALLOCATION: ASSERT( 0 );   break;
               }
            }

         if ( pAlloc->m_targetBasisField.IsEmpty() == false )
            tb = pAlloc->m_targetBasisField;

         switch( pAlloc->m_targetDomain )
            {
            case TD_UNDEFINED:      break;               
            case TD_AREA:           td = "area";       break;
            case TD_PCTAREA:        td = "pctArea";    break;
            case TD_ATTR:           td = "attribute";  break;   // unused at this point
            case TD_USEALLOCATION:  ASSERT( 0 );   break;
            }

         GetXmlStr((LPCTSTR)pAlloc->m_name, xmlStr);
         fprintf( fp, "   <allocation name='%s' id='%i' \n", (LPCTSTR) xmlStr, pAlloc->m_id );

         if ( ts != NULL )
            fprintf( fp, "         target_source='%s' \n", ts );
         if ( tv != NULL )
            fprintf( fp, "         target_values='%s' \n", tv );
         if ( td != NULL )
            fprintf( fp, "         target_domain='%s' \n", td );
         if ( tb != NULL )
            fprintf( fp, "         target_basis='%s' \n", tb );
   
         // expansion stuff
         if ( pAlloc->m_expandQueryStr.IsEmpty() == false )
            {
            CString query, area; // make sure the strings are xml-compliant 
            GetXmlStr( (LPCTSTR) pAlloc->m_expandQueryStr, query );
            GetXmlStr( (LPCTSTR) pAlloc->m_expandAreaStr, area );
         
            fprintf( fp, "         expand_query='%s' \n", (LPCTSTR) query );
            fprintf( fp, "         expand_area='%s' \n", (LPCTSTR) area );
            }

         // constraint
         CString constraint;
         GetXmlStr( (LPCTSTR) pAlloc->m_constraint.m_queryStr, constraint );
         fprintf( fp, "         constraint='%s' \n", (LPCTSTR) constraint  );

         fputs( "        >\n", fp ); // close <allocation> tag

         // next, preferences
         for ( int k=0; k < (int) pAlloc->m_prefsArray.GetSize(); k++ )
            {
            Preference *pPref = pAlloc->m_prefsArray[ k ];
            CString query;
            GetXmlStr( (LPCTSTR) pPref->m_queryStr, query );
            GetXmlStr((LPCTSTR)pPref->m_name, xmlStr);
            
            fprintf( fp, "      <preference name='%s' weight='%s' query='%s' />\n", (LPCTSTR) xmlStr, (LPCTSTR) pPref->m_weightExpr, (LPCTSTR) query );
//            fprintf( fp, "                  query='%s' />\n", (LPCTSTR) pPref->m_queryStr );           
            }

         fputs( "   </allocation>\n\n", fp );
         }  // end of: for each Allocation in the AllocationSet

      // closing tag
      fputs( "</allocation_set>\n\n", fp );
      }

   fputs( "</spatial_allocator>\n", fp );

   return 1;
   }


int SpatialAllocator::ReplaceAllocSets( PtrArray< AllocationSet > &allocSet )
   {
   this->m_allocationSetArray.RemoveAll();
   this->m_allocationSetArray.DeepCopy( allocSet );

   // generate any needed data
   InitAllocSetData();   // initialize instance-level allocator dataobjs

   return (int) this->m_allocationSetArray.GetSize();
   }


float SpatialAllocator::Expand( int idu, Allocation *pAlloc, bool useAddDelta, int currAttrCode, int newAttrCode, 
            int currSequenceID, int newSequenceID, EnvContext *pContext, CArray< bool > &isIduOpen, float &expandArea )
   {   
   // expand the allocation to the area around the idu provided
   // returns the basis value associated with the expand area.
   ASSERT( this->m_pMapLayer != NULL );
   MapLayer *pLayer = this->m_pMapLayer;

   // basic idea - expand in concentric circles aroud the idu (subject to constraints)
   // until necessary area is found.  We do this keeping track of candidate expansion IDUs
   // in m_expansionArray, starting with the "nucleus" IDU and iteratively expanding outward.
   // for the expansion array, we track the index in the expansion array of location of
   // the lastExpandedIndex and the number of unprocessed IDUs that are expandable (poolSize).
   int neighbors[ 64 ];
   bool addToPool = false;
   INT_PTR lastExpandedIndex = 0;
   int poolSize = 1;       // number of unprocessed IDUs that are expandable

   pAlloc->SetMaxExpandArea();  // really, basis

   // note: values in m_expansionArray are -(index+1)
   m_expansionIDUs.RemoveAll();
   m_expansionIDUs.Add( idu );  // the first one (nucleus) is added whether it passes the query or not
                                // note: this means the nucleus IDU gets treated the same as any
                                // added IDU's passing the test, meaning the expand outcome gets applied
                                // to the nucleus as well as the neighbors.  However, the nucleus
                                // idu is NOT included in the "areaSoFar" calculation, and so does not
                                // contribute to the area count to the max expansion area
   float areaSoFar = 0;
    
   // We collect all the idus that are next to the idus already processed that we haven't seen already
   while( poolSize > 0 && ( pAlloc->m_expandMaxArea <= 0 || areaSoFar < pAlloc->m_expandMaxArea ) )
      {
      INT_PTR countSoFar = m_expansionIDUs.GetSize();

      // iterate from the last expanded IDU on the list to the end of the current list,
      // adding any neighbor IDU's that satisfy the expansion criteria.
      for ( INT_PTR i=lastExpandedIndex; i < countSoFar; i++ )
         {
         int nextIDU = m_expansionIDUs[ i ];

         if ( nextIDU >= 0 )   // is this an expandable IDU?
            {
            Poly *pPoly = pLayer->GetPolygon( nextIDU );  // -1 );    // why -1?
            int count = pLayer->GetNearbyPolys( pPoly, neighbors, NULL, 64, 1 );

            for ( int i=0; i < count; i++ )
               {
               // see if this is an elgible IDU (if so, addToPool will be set to true and the IDU area will be returned. 
               float area = AddIDU( neighbors[ i ], pAlloc, addToPool, areaSoFar, pAlloc->m_expandMaxArea ); 

               if ( addToPool )  // AddIDU() added on to the expansion array AND it is an expandable IDU?
                  {
                  poolSize++;
                  areaSoFar += area;
                  ASSERT( pAlloc->m_expandMaxArea <= 0 || areaSoFar <= pAlloc->m_expandMaxArea );
                  // goto applyOutcome;
                  }
               }

            poolSize--;    // remove the "i"th idu just processed from the pool of unexpanded IDUs 
                           // (if it was in the pool to start with)
            }  // end of: if ( nextIDU > 0 ) 
         }  // end of: for ( i < countSoFar ) - iterated through last round of unexpanded IDUs

      lastExpandedIndex = countSoFar;
      }

   // at this point, the expansion area has been determined (it's stored in the m_expansionIDUs array) 
   // - apply outcomes.  NOTE: this DOES NOT includes the kernal IDU, since it is the first idu in the expansionIDUs array
   float totalBasis = 0;

   for ( INT_PTR i=1; i < m_expansionIDUs.GetSize(); i++ ) // skip kernal IDU
      {
      int expIDU = m_expansionIDUs[ i ];
      if ( expIDU >= 0 )
         {
         // get the basis associated with the expansion IDU being examined
         float basis = 0;
         pLayer->GetData( expIDU, pAlloc->m_colTargetBasis, basis );
         totalBasis += basis;

         isIduOpen[ expIDU ] = false;    // indicate we've allocated this IDU

         if ( useAddDelta )
            {
            // apply the new attribute.
            if ( currAttrCode != newAttrCode )
                AddDelta( pContext, expIDU, pAlloc->m_pAllocSet->m_col, newAttrCode );
           
            // apply the new sequence
            if ( currSequenceID != newSequenceID )
               AddDelta( pContext, expIDU, pAlloc->m_pAllocSet->m_colSequence, newSequenceID );
            }
         else
            {
            if ( currAttrCode != newAttrCode )
               m_pMapLayer->SetData( expIDU, pAlloc->m_pAllocSet->m_col, newAttrCode );

            if ( pAlloc->m_pAllocSet->m_colSequence >= 0 && currSequenceID != newSequenceID )
               m_pMapLayer->SetData( expIDU, pAlloc->m_pAllocSet->m_colSequence, newSequenceID );
            }
         }
      }

   expandArea = areaSoFar;

   return totalBasis;  // note: areaSoFar only includes expansion area, not kernal area
   }


float SpatialAllocator::AddIDU( int idu, Allocation *pAlloc, bool &addToPool, float areaSoFar, float maxArea )
   {
   // have we seen this on already?
   for ( int i=0; i < m_expansionIDUs.GetSize(); i++ )
      {
      int expansionIndex = m_expansionIDUs[ i ];

      if ( ( expansionIndex < 0 && -(expansionIndex+1) == idu )    // seen but not an expandable IDU
        || ( expansionIndex >= 0 && expansionIndex == idu ) )      // seen and is an expandable IDU
         {
         addToPool = false;
         return 0;
         }
      }

   // so, we haven't seen this one before. Is the constraint satisified?
   addToPool = true;

   if ( pAlloc->m_pExpandQuery )
      {
      bool result = false;
      bool ok = pAlloc->m_pExpandQuery->Run( idu, result );
      if ( ! ok || ! result )
         addToPool = false;
      }

   // would this IDU cause the area limit to be exceeded?
   float area = 0;
   m_pMapLayer->GetData( idu, m_colArea, area );

   if ( addToPool && ( maxArea > 0 ) && ( ( areaSoFar + area ) > maxArea ) )
      addToPool = false;

   float allocationSoFar = 0;
   float currentTarget = 0;
   if ( pAlloc->m_targetSource == TS_USEALLOCATIONSET )
      {
      allocationSoFar = pAlloc->m_pAllocSet->m_allocationSoFar;
      currentTarget = pAlloc->m_pAllocSet->m_currentTarget;
      }
   else
      {
      allocationSoFar = pAlloc->m_allocationSoFar;
      currentTarget = pAlloc->m_currentTarget;
      }

   if ( addToPool && ( ( allocationSoFar + areaSoFar + area ) > currentTarget ) )
      addToPool = false;

   if ( addToPool )
      m_expansionIDUs.Add( idu/*+1*/ );
   else
      m_expansionIDUs.Add( -(idu+1) ); // Note: what gets added to the expansion array is -(index+1)
   
   return area;
   }



int SpatialAllocator::CollectExpandStats( void )
   {
   //isIduOpen.SetSize( iduCount );      // this array is never shuffled, always in IDU order

   // first, for each allocation, sort it's score list
   for ( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      // get an allocation set
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];

      if ( pAllocSet->m_inUse )
         {
         int allocCount = (int) pAllocSet->m_allocationArray.GetSize();
   
         // for each allocation in the set, sort the score array
         for ( int j=0; j < allocCount; j++ )
            {
            Allocation *pAlloc = pAllocSet->m_allocationArray[ j ];   // an Allocation defines scores for a particular alloc tpe (e.g. "Corn")
   
            }
         }
      }

   return 0;
   }


void SpatialAllocator::CollectData(EnvContext *pContext)
   {
   //----------------------------------------------
   // output data format
   //
   // For each allocation set that is in use, data is collected
   // AS.
   // if target is defined at the AllocationSet level:
   //    Time
   //    Total Target
   //    Total Realized
   //    for each allocation
   //        Realized
   //        pct Realized
   //        ConstraintsArea
   //        MinScore
   //        MaxScore
   //        ScoreCount
   //        ScoreBasis
   //        ExpandArea
   //        ExpandBasis
   //
   // if targets are defined for each individual allocation:
   //    Time
   //    for each allocation
   //        Target
   //        Realized
   //        pct Realized
   //        ConstraintsArea
   //        MinScore
   //        MaxScore
   //        ScoreCount
   //        ScoreBasis
   //        ExpandArea
   //        ExpandBasis

   // collect output
   for( int i=0; i < (int) m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pAllocSet = m_allocationSetArray[ i ];

      bool isTargetDefined = pAllocSet->IsTargetDefined();

      //if ( pAllocSet->m_inUse )
      //   {
      // collect data (whether in use or not - should be smarter than this!!!
      CString msg( "  Spatial Allocator: " );

      CArray< float, float > outputs;
      outputs.Add( (float) pContext->currentYear );  // time

      if ( isTargetDefined )
         {
         outputs.Add( pAllocSet->m_currentTarget );
         outputs.Add( pAllocSet->m_allocationSoFar );
         outputs.Add(pAllocSet->m_areaSoFar);  // added 
         }

      for ( int j=0; j < pAllocSet->GetAllocationCount(); j++ )
         {
         Allocation *pAlloc = pAllocSet->GetAllocation( j );

         if ( ! isTargetDefined )
            outputs.Add( pAlloc->m_currentTarget );

         outputs.Add( pAlloc->m_allocationSoFar );    // realized target
         outputs.Add(pAlloc->m_areaSoFar);    // realized target

         if ( isTargetDefined )
            outputs.Add( pAlloc->m_allocationSoFar*100 / pAllocSet->m_currentTarget );
         else
            outputs.Add( pAlloc->m_allocationSoFar*100 / pAlloc->m_currentTarget );

         outputs.Add( pAlloc->m_constraintAreaSatisfied );

         if ( pAlloc->m_minScore >= (float) LONG_MAX )
            pAlloc->m_minScore = -99;

         if ( pAlloc->m_maxScore <= 0 )
            pAlloc->m_maxScore = -99;  //m_scoreThreshold-1;

         outputs.Add( pAlloc->m_minScore );
         outputs.Add( pAlloc->m_maxScore );
         outputs.Add( (float) pAlloc->m_scoreCount );
         outputs.Add( pAlloc-> m_scoreBasis );

         outputs.Add( pAlloc->m_expandArea );
         outputs.Add( pAlloc-> m_expandBasis );

         //pAlloc->m_cumBasisTarget += pAlloc->m_currentTarget;   ////????
         //pAlloc->m_cumBasisActual       += pAlloc->m_allocationSoFar;  ////????
         //
         //cumOutputs.Add( pAlloc->m_cumBasisTarget );
         //cumOutputs.Add( pAlloc->m_cumBasisActual );
         }

      if ( pContext->yearOfRun >= 0 )
         {
         if ( pAllocSet->m_pOutputData )
            pAllocSet->m_pOutputData->AppendRow( outputs );

         if ( pAllocSet->m_inUse )
            {
            // report resutls for this allocation set
            CString msg;
            if ( isTargetDefined )
               msg.Format( "Spatial Allocator [%s]: Time: %i,  Total Target: %.0f,  Realized: %.0f", (LPCTSTR) pAllocSet->m_name, pContext->currentYear, outputs[ 1 ], outputs[ 2 ] );
            else
               msg.Format( "Spatial Allocator [%s]: Time: %i",  (LPCTSTR) pAllocSet->m_name, pContext->currentYear );

            pAllocSet->m_status = msg;
            Report::Log( msg );

            for ( int j=0; j < pAllocSet->GetAllocationCount(); j++ )
               {
               Allocation *pAlloc = pAllocSet->GetAllocation( j );

               if ( isTargetDefined )
                  msg.Format( "  Allocation [%s]: Realized: %.0f,  %% Realized: %.1f,  Constraint Area: %.0f,  Min Score: %.0f,  Max Score: %.0f,  Count: %i (%i used), Score Basis: %.0f, Expand Area: %.0f, Expand Basis: %.0f, currentIndex=%i",
                                  (LPCTSTR) pAlloc->m_name, pAlloc->m_allocationSoFar, pAlloc->m_allocationSoFar*100/pAllocSet->m_currentTarget, pAlloc->m_constraintAreaSatisfied,
                                  pAlloc->m_minScore, pAlloc->m_maxScore, pAlloc->m_scoreCount, pAlloc->m_usedCount, pAlloc-> m_scoreBasis, pAlloc->m_expandArea, pAlloc->m_expandBasis, pAlloc->m_currentIduScoreIndex );
               else
                  msg.Format( "  Allocation [%s]: Target: %.0f,  Realized: %.1f,  %% Realized: %.1f,  Constraint Area: %.0f,  Min Score: %.0f,  Max Score: %.0f,  Count: %i (%i used), Score Basis: %.0f, Expand Area: %.0f, Expand Basis: %.0f, currentIndex=%i",
                                  (LPCTSTR) pAlloc->m_name, pAlloc->m_currentTarget, pAlloc->m_allocationSoFar, pAlloc->m_allocationSoFar*100/pAlloc->m_currentTarget, pAlloc->m_constraintAreaSatisfied,
                                  pAlloc->m_minScore, pAlloc->m_maxScore, pAlloc->m_scoreCount, pAlloc->m_usedCount, pAlloc-> m_scoreBasis, pAlloc->m_expandArea, pAlloc->m_expandBasis, pAlloc->m_currentIduScoreIndex );

               pAlloc->m_status = msg;
               Report::Log( msg );
               }
            }
         }

      // all done with this allocation set
      }
   }




int CompareScores(const void *elem0, const void *elem1 )
   {
   // The return value of this function should represent whether elem0 is considered
   // less than, equal to, or greater than elem1 by returning, respectively, 
   // a negative value, zero or a positive value.
   
   IDU_SCORE *pIduScore0 = (IDU_SCORE*) elem0;
   IDU_SCORE *pIduScore1 = (IDU_SCORE*) elem1;

   if ( pIduScore0->score < pIduScore1->score )
      return 1;
   else if ( pIduScore0->score == pIduScore1->score )
      return 0;
   else
      return -1;
   }


#ifndef NO_MFC
bool SpatialAllocator::Setup( EnvContext *pContext, HWND hWnd )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   if (this->m_allocationSetArray.GetSize() == 0)
      {
      AllocationSet *pAllocSet = new AllocationSet( this, "AllocSet_Default", "LULC_A");
      Allocation *pAlloc = new Allocation(pAllocSet, "Alloc_Default", -1);
      this->m_allocationSetArray.Add(pAllocSet);
      }

   CRect rect;
   GetClientRect( hWnd, &rect );

   CPoint ul( rect.left, rect.top );
   CPoint lr( rect.right, rect.bottom );
   ClientToScreen( hWnd, &ul );
   ClientToScreen( hWnd, &lr );

   rect.left = ul.x;
   rect.top = ul.y;
   rect.right = lr.x;
   rect.bottom = lr.y;

   SAllocView dlg(this, pLayer, rect, pContext );
   INT_PTR nResponse = dlg.DoModal();

  pLayer->ClearSelection();

   //if (nResponse == IDOKAS)
   //{
   //   // TODO: Place code here to handle when the dialog is
   //   //  dismissed with OK
   //}
   //else if (nResponse == IDCANCELAS)
   //{
   //   // TODO: Place code here to handle when the dialog is
   //   //  dismissed with Cancel
   //}

   // Since the dialog has been closed, return FALSE so that we exit the
   //  application, rather than start the application's message pump.
   return FALSE;
}

#endif