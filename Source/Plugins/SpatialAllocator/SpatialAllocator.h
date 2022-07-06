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
#pragma once

#include <EnvExtension.h>

#include <FDATAOBJ.H>
#include <QueryEngine.h>
#include <MapExprEngine.h>
#include <PtrArray.h>
#include <RANDOM.HPP>
#include <EnvConstants.h>


#define _EXPORT __declspec( dllexport )

/*---------------------------------------------------------------------------------------------------------------------------------------
 general idea:  the SpatialAllocator class represents a process that has targets that it allocates to the landscape, limited by various constraints
 
 Targets are defined in the xml input file, specified as series of preferneces and/or constraints. 

 The basic idea is the for each IDU, score the idu for a particular attribute. If any constraints apply, the score is zeroed out.
     Scoring is achieved by applying preferences scores.  A preference score is a computed quantity that computes a weighted average
     score with a preference count bonus.

------------------------------------------------------------------------------------------------------------------------------------------- */

enum TARGET_SOURCE { TS_UNDEFINED=0, TS_TIMESERIES=1, TS_RATE=2, TS_FILE=3, TS_USEALLOCATIONSET=4, TS_USEALLOCATION=5 };
enum TARGET_DOMAIN { TD_UNDEFINED=0, TD_AREA=2, TD_PCTAREA=3, TD_ATTR=4 /*unused*/, TD_USEALLOCATION=5 };

enum CONSTRAINT_TYPE { CT_QUERY=0 };
enum ALLOC_METHOD { AM_SCORE_PRIORITY=0, AM_BEST_WINS=1 };
enum EXPAND_TYPE  { ET_CONSTANT=0, ET_UNIFORM=1, ET_NORMAL=2, ET_WEIBULL=3, ET_LOGNORMAL=4, ET_EXPONENTIAL=5 };

class AllocationSet;
class Allocation;
class SpatialAllocator;

struct IDU_SCORE { int idu; float score; };

typedef int(*SCOREPROC)(SpatialAllocator*, int status, int idu, float score, Allocation *pAlloc, float remainingArea, float pctAllocated, LONG_PTR extra );  // return 0 to interrupt, anything else to continue

struct SCOREPROCINFO
   {
   SCOREPROC proc;
   LONG_PTR extra;

   SCOREPROCINFO(SCOREPROC p, LONG_PTR ex) : proc(p), extra(ex) { }
   };


class Preference
{
public:
   CString m_name;
   CString m_queryStr;
   CString m_weightExpr;

   Allocation *m_pAlloc;
   Query   *m_pQuery;
   MapExpr *m_pMapExpr;

//protected:
   float m_value;    // current value

public:
   Preference( Allocation *pAlloc, LPCTSTR name, LPCTSTR queryStr, LPCTSTR weightExpr );
   Preference( Preference &p ) { *this = p; }
   ~Preference( void ) { if ( m_pMapExpr != NULL ) delete m_pMapExpr; }

   Preference &operator=( Preference& );

   bool Init( void );
   };


class Constraint
{
public:
   CONSTRAINT_TYPE m_cType;
   CString m_queryStr;
   Query  *m_pQuery;

   Allocation *m_pAlloc;

   Constraint() : m_queryStr(), m_cType( CT_QUERY), m_pQuery( NULL ), m_pAlloc( NULL ) { } 
   Constraint( Allocation *pAlloc, LPCTSTR queryStr );
   Constraint(Constraint &c) { *this = c; }

   ~Constraint( void ) { }
   void SetConstraintQuery( LPCTSTR query );

   Query *CompileQuery( void );

   Constraint &operator=( Constraint &c );
};


// a "TargetContainer" is a either an allocation or an allocation set that 
// specifies a target to be achieved.  Because either can be a Target

enum TARGET_CLASS { TC_ALLOCATION, TC_ALLOCATIONSET };

class TargetContainer
{
public:
   CString m_name;
   TARGET_SOURCE  m_targetSource;   // where are target values defined? timeseries, rate, file, ...
   TARGET_DOMAIN  m_targetDomain;   // what units are being allocated area, pctarea, attr???
  
   CString m_targetValues;          // string containing target source information/values
   
   CString m_targetBasisField;      // NEW, contains name of field that is the basis for what is used as the target
   int     m_colTargetBasis;        // field being allocated, e.g. the units of the target values, default = "AREA"
   CString m_description;
   CString m_status;                // string capturing last run results

public:
   FDataObj *m_pTargetData;      // input data table for target inputs
  
   // Best Wins temporary variables used during run
   float m_allocationSoFar;     // temporary variable, units of basis
   float m_areaSoFar;
   float m_currentTarget;
   float m_targetRate;   // for TT_RATE only!!

   CString m_targetQuery;           // only used if target domain = TD_PCTAREA; defines 
   Query  *m_pTargetQuery;          // the area...???

   SpatialAllocator *m_pSAModel;

   TargetContainer() 
      : m_name( "" )
      , m_targetSource( TS_UNDEFINED )
      , m_targetDomain( TD_UNDEFINED )
      , m_targetValues()
      , m_targetBasisField()
      , m_colTargetBasis( -1 )
      , m_description()
      , m_status()
      , m_pTargetData( NULL )
      , m_allocationSoFar(0)
      , m_areaSoFar(0)
      , m_currentTarget( 0 )
      , m_targetRate( 0 )
      , m_pTargetQuery( NULL )
      , m_pSAModel(NULL)
         { }

   TargetContainer( SpatialAllocator *pSAModel, LPCTSTR name ) 
      : m_name( name )
      , m_targetSource( TS_UNDEFINED )
      , m_targetDomain( TD_UNDEFINED )
      , m_targetValues()
      , m_targetBasisField()
      , m_colTargetBasis( -1 )
      , m_description()
      , m_status()
      , m_pTargetData( NULL )
      , m_allocationSoFar(0)
      , m_areaSoFar(0)
      , m_currentTarget( 0 )
      , m_targetRate( 0 )
      , m_pTargetQuery( NULL )
      , m_pSAModel( pSAModel)
         { }

   TargetContainer(SpatialAllocator *pSAModel, LPTSTR name, TARGET_SOURCE source, TARGET_DOMAIN domain, LPCTSTR targetValues ) 
      : m_name( name )
      , m_targetSource( source )
      , m_targetDomain( domain )
      , m_targetValues( targetValues )
      , m_targetBasisField()
      , m_colTargetBasis( -1 )
      , m_description()
      , m_status()
      , m_pTargetData( NULL )
      , m_allocationSoFar(0)
      , m_areaSoFar(0)
      , m_currentTarget( 0 )
      , m_targetRate( 0 )
      , m_pTargetQuery( NULL )
      , m_pSAModel( pSAModel )
         { }
   
   TargetContainer( TargetContainer &tc ) { *this = tc; }

   TargetContainer &operator = ( TargetContainer &tc )
      {
      m_name = tc.m_name;
      m_pTargetQuery   = NULL;
      m_pTargetData = NULL; 
     
      m_targetSource = tc.m_targetSource;
      m_targetDomain = tc.m_targetDomain;
      m_targetValues = tc.m_targetValues;
      m_targetBasisField = tc.m_targetBasisField;
      m_colTargetBasis = tc.m_colTargetBasis;
      m_description = tc.m_description;
      m_status      = tc.m_status;
      m_allocationSoFar = tc.m_allocationSoFar;
      m_areaSoFar = tc.m_areaSoFar;
      m_currentTarget = tc.m_currentTarget;
      m_targetRate = tc.m_targetRate;
      m_targetQuery = tc.m_targetQuery;
      m_pSAModel = tc.m_pSAModel;

      return *this;
      }
   
   ~TargetContainer() { if ( m_pTargetData != NULL ) delete m_pTargetData; } 
   
   virtual TARGET_CLASS GetTargetClass( void ) = 0;

   void SetTargetParams( MapLayer *pLayer, LPCTSTR basis, LPCTSTR source, LPCTSTR values, LPCTSTR domain, LPCTSTR query );

   float SetTarget( int year );   
   bool GetTargetRate( float &rate ) { if ( m_targetSource != TS_RATE ) return false; rate = (float) atof( m_targetValues ); return true; }

   void Init( int id, int colAllocSet, int colSequence );

   bool IsTargetDefined() { return ( m_targetSource == TS_UNDEFINED || m_targetSource == TS_USEALLOCATIONSET || m_targetSource == TS_USEALLOCATION ) ? false : true; }

};


// an allocation represents an <allocation> tag in the xml, e.g.
//  <allocation name="Soybeans" id="12"
//              target_type="rate" 
//              target_values="0.005" 
//              col="SA_BEANS" >
// it contains targets for a given allocation as well as the code used to id the allocation.
// if col is specified, a field is added to the IDUs that has the score of the allocation 
// stored

class Allocation : public TargetContainer
{
public:
   int m_id;      // id
   
   // expansion params.
   bool    m_useExpand;          // allow expanded outcomes?
   CString m_expandAreaStr;      // string specified in 'expand_area" tag
   float   m_expandMaxArea;      // maximum area allowed for expansion, total including kernal
   int     m_expandType;         // 0=constant value (m_expandMaxArea), 1 = uniform, 2 = normal
   
   Rand   *m_pRand;           
   double  m_expandAreaParam0;   // distribution-dependant param 0 - holds max value for constants, shape param for dists
   double  m_expandAreaParam1;   // distribution-dependant param 0, scale param for dists

   CString m_expandQueryStr;  // expand into adjacent IDUs that satify this query 
   Query  *m_pExpandQuery;
   
   // area stats
   float   m_initPctArea;
   float   m_constraintAreaSatisfied;

   // basis stats
   float   m_cumBasisActual;
   float   m_cumBasisTarget;

   // additional stats
   float m_minScore;       // min and max score for this allocation
   float m_maxScore;
   int   m_scoreCount;     // number of scores greater than 0 at any given time
   int   m_usedCount;      
   float m_scoreArea;      // area of scores greater than 0 at any given time
   float m_scoreBasis;     // target amount associated with scored IDUs
   float m_expandArea;
   float m_expandBasis;
      
public:   
   PtrArray< Preference > m_prefsArray;
   Constraint m_constraint;

   CArray< int > m_sequenceArray;   // array of sequence codes if defined
   CMap< int, int, int, int > m_sequenceMap;   // key=Attr code, value=index in sequence array

   CArray< IDU_SCORE, IDU_SCORE& > m_iduScoreArray;  // sorted list of idu/score pairs

   CString m_scoreCol;
   int     m_colScores;

   // ScorePriority temporary variables
   int m_currentIduScoreIndex;   // this stores the current index of this allocation in the iduScore array
   
   AllocationSet *m_pAllocSet;   // associated allocation set

public:
   // constructor
   Allocation( AllocationSet *pSet, LPCTSTR name, int id ); //, TARGET_SOURCE targetSource, LPCTSTR targetValues );

   Allocation(Allocation &a ) { *this = a; }
   Allocation &operator = ( Allocation &a );

   ~Allocation( void );

   // methods
   void  SetExpandQuery( LPCTSTR expandQuery );
   void  SetMaxExpandAreaStr( LPCTSTR expandAreaStr );
   float SetMaxExpandArea( void );
   int   GetCurrentIdu  ( void ) { return m_currentIduScoreIndex >= m_iduScoreArray.GetSize() ? -1 : m_iduScoreArray[ m_currentIduScoreIndex ].idu; }
   float GetCurrentScore( void ) { return m_currentIduScoreIndex >= m_iduScoreArray.GetSize() ? -1 : m_iduScoreArray[ m_currentIduScoreIndex ].score; }

   bool  IsSequence( void ) { return m_sequenceArray.GetSize() > 0; }
   float GetCurrentTarget();  // returns allocation if defined, otherwise, allocation set target

   virtual TARGET_CLASS GetTargetClass( void ) { return TC_ALLOCATION; }
};


class AllocationSet : public TargetContainer
{
public:
   SpatialAllocator *m_pSAModel;
   CString m_field;
   CString m_seqField;   // used to id the current sequence
   CString m_targetLocation;  // this can be an expression, in which case it is used to define a probabiliy surface
                              // that is sampled to determine the order in which IDUs are selected

   int m_col;                 // the IDU field which is populated by the "winning" allocation ID

   int m_colSequence;

   // note: the following are also defined at the Allocation level 
   bool m_inUse;
   //bool m_shuffleIDUs;

   MapExpr  *m_pTargetMapExpr;  // memory managed by?
   
   PtrArray< Allocation > m_allocationArray;

   FDataObj *m_pOutputData;         // target and realized % areas for each allocation
   
   AllocationSet( SpatialAllocator *pSAModel, LPCTSTR name, LPCTSTR field ) 
      : TargetContainer( pSAModel, name ), 
        m_pSAModel( pSAModel), m_field( field ), m_col( -1 ), m_colSequence( -1 ),
        m_inUse( true ), /*m_shuffleIDUs( true ),*/ m_pOutputData( NULL ), /* m_pOutputDataCum( NULL ), */
        m_pTargetMapExpr( NULL )
        { }
        
   AllocationSet( AllocationSet &as ) { *this = as; }

   AllocationSet &operator = ( AllocationSet &as );

   ~AllocationSet( ) { if ( m_pOutputData ) delete m_pOutputData;/*  if ( m_pOutputDataCum ) delete m_pOutputDataCum; */}

   int OrderIDUs( CUIntArray &iduArray, bool shuffleIDUs, RandUniform *pRandUnif );
   int GetAllocationCount( void ) { return (int) m_allocationArray.GetSize(); }
   Allocation *GetAllocation( int i ) { return m_allocationArray.GetAt( i ); }

   virtual TARGET_CLASS GetTargetClass( void ) { return TC_ALLOCATIONSET; }
};



class _EXPORT SpatialAllocator : public EnvModelProcess
{
public:
   SpatialAllocator();
   SpatialAllocator( SpatialAllocator &sa );

   ~SpatialAllocator( void );
   
   bool Init   ( EnvContext *pEnvContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pEnvContext, bool useInitialSeed );
   bool Run    ( EnvContext *pContext );
	bool EndRun ( EnvContext *pContext );

#ifndef NO_MFC
   bool Setup  ( EnvContext *pContext, HWND hWnd );
#endif
   
   CString        m_xmlPath;              // xml input file path
   MapLayer      *m_pMapLayer;            // memory managed by EnvModel
   MapExprEngine *m_pMapExprEngine;       // memory managed by EnvModel
   QueryEngine   *m_pQueryEngine;         // memory managed by EnvModel

   PtrArray< AllocationSet > m_allocationSetArray;
   int m_colArea;                         // default="AREA"
   ALLOC_METHOD m_allocMethod;

   CArray< int, int > m_expansionIDUs;   // idus that

   float m_scoreThreshold;    // 0 be default - pref scores < this value are not considered

protected:
   CUIntArray m_iduArray;    // used for shuffling IDUs

   void SetAllocationTargets( int yearOfRun );  // always 0-based 
   void ScoreIduAllocations( EnvContext *pContext, bool useAddDelta );
   void AllocScorePriority( EnvContext *pContext, bool useAddDelta );
   void AllocBestWins( EnvContext *pContext, bool useAddDelta );
   bool PopulateSequences( void );
   float Expand( int idu, Allocation *pAlloc, bool useAddDelta, int currAttrCode, int newAttrCode, int currSequenceID, int newSequenceID, EnvContext*, CArray< bool > &isOpenArray, float &expandArea );
   float AddIDU( int idu, Allocation *pAlloc, bool &addToPool, float areaSoFar, float maxArea );
   int  CollectExpandStats( void );
   void CollectData( EnvContext *pContext );

public:
   bool LoadXml( LPCTSTR filename, PtrArray< AllocationSet > *pAllocSetArray=NULL );
   bool UpdateAllocationsFromFile();
   bool SaveXml( LPCTSTR filename );   // can include full path

   int ReplaceAllocSets( PtrArray< AllocationSet > &allocSet );
   void InitAllocSetData( void );   // pAllocSet->m_pOutputData + TargetContainer::Init() -> allocates m_pTargetData FDataObj
   void ScoreIduAllocation( Allocation *pAlloc, int flags );
   FDataObj* m_pBuildFromFile;
   CString m_update_from_file_name;
protected:
   int SaveXml( FILE *fp );   // helper function

   bool m_shuffleIDUs;
   RandUniform *m_pRandUnif;     // for shuffling IDUs   

   float m_totalArea;

	int m_runNumber;

   // data collection
   FDataObj m_outputData;

   bool m_collectExpandStats;
   FDataObj *m_pExpandStats;    // cols are size bins, rows are active allocations
   
protected:   
   PtrArray < SCOREPROCINFO > m_notifyProcArray;
   int Notify( int status, int idu, float score, Allocation *pAlloc, float remainingArea, float pctAllocated );

public:
   bool InstallNotifyHandler(SCOREPROC p, LONG_PTR extra) { m_notifyProcArray.Add(new SCOREPROCINFO(p, extra)); return true; }
   bool RemoveNotifyHandler (SCOREPROC p, LONG_PTR extra);
};


inline 
bool SpatialAllocator::RemoveNotifyHandler( SCOREPROC p, LONG_PTR extra )
   {
   for ( int i=0; i < m_notifyProcArray.GetSize(); i++ )
      {
      SCOREPROCINFO *pInfo = m_notifyProcArray[ i ];
      if ( pInfo->proc == p && pInfo->extra == extra ) 
         {
         m_notifyProcArray.RemoveAt( i );
         return true;
         }
      }
   return false;
   }


inline
int SpatialAllocator::Notify( int status, int idu, float score, Allocation *pAlloc, float remainingArea, float pctAllocated )
   {
   int procVal = 0;
   int retVal  = 1;
   for ( int i=0; i < m_notifyProcArray.GetSize(); i++ )
      {
      SCOREPROCINFO *pInfo = m_notifyProcArray[ i ];
      if ( pInfo->proc != NULL ) 
         {
         procVal =  pInfo->proc( this, status, idu, score, pAlloc, remainingArea, pctAllocated, pInfo->extra ); 
         if ( procVal == 0 )
            retVal = 0;
         }
      }

   return retVal;
   }

