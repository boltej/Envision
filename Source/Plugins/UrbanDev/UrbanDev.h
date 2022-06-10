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

#include <QueryEngine.h>
#include <EnvExtension.h>

#include <Vdataobj.h>
#include <PtrArray.h>
#include <misc.h>
#include <FDATAOBJ.H>

#include <randgen\Randunif.hpp>

#include <MapExprEngine.h>

#include <PolyPtMapper.h>



// UrbanDev contains four functions, each configured via XML
//
// <population> - allocates initial population based on map expressions,
//   specified using <pop_dens> tags the compute an initial density expression
//   based on a map expresion and a query.
//
// <dwellings> - populates  NDU and NEWDU (or other user-specified fields) 
//   based on a floating point value "ppdu" that specifies the "#people/dwelling unit"
//   in an aea sepcieifed with a query.
//
// <uga_expansion> - defines how UGAs (urban growth areas) can expand as population is added
//    Columns:
//       UGA_ID:     (in) IDU field indicating existing UGAs
//       Zone:       (in) Contains zoning codes
//       UX_DIST:    (in) Distance to the closest UGA
//       UX_UGA_ID:  (in) if >= 0, identifies an expansion IDU for the given UGA
//       UX_PRIORTY: (out) ranking of priority for consideraion of UGA expansions
//       UX_EVENT:   (out) contains the expansion eventID if the IDU is annexed
//
// <transitions>  -- generates transition matrix for teh specific classes
//    cols=class to generate transitions for
//    

enum { INIT_NDUS_FROM_NONE=0, INIT_NDUS_FROM_POPDENS=1,INIT_NDUS_FROM_DULAYER=2 };

#define _EXPORT __declspec( dllexport )


struct UGA_PRIORITY
   {
   public:
      int idu;    // in idu coverage
      float priority;

      UGA_PRIORITY(int _idu, float _priority) : idu(_idu), priority(_priority) { }
   };

struct UGA  // new
   {
   int m_id;
   CString m_name;
   CString m_type;

   float m_startPopulation;         // people - set in init 
   float m_resExpArea;              // m2
   float m_commExpArea;             // m2
   float m_totalExpArea;              // m2
   float m_currentArea;             // m2
   float m_currentResArea;          // m2
   float m_currentCommArea;         // m2
   float m_currentPopulation;       // people
   float m_newPopulation;           // people
   float m_popIncr;                 // people
   float m_capacity;                // people
   float m_pctAvailCap;             // decimal percent
   float m_avgAllowedDensity;       // du/ac
   float m_avgActualDensity;        // du/ac
   float m_impervious;              // % impervious

   //??????????
   int m_nextResPriority;     // ptrs to current spot in list
   int m_nextCommPriority;
   int m_currentEvent;

   PtrArray< UGA_PRIORITY > m_priorityListRes;   // sorted lists of UGB Expansion areas
   PtrArray< UGA_PRIORITY > m_priorityListComm;
   CArray<int, int> m_iduArray;
   //CArray<int, int> m_ruralIDUArray;

   CMap<int, int, UINT, UINT> m_iduToUpzonedMap;   // actual IDU index to count of upzones

   bool m_use;
   bool m_computeStartPop;


   UGA(void)
      : m_id(-1)
      , m_startPopulation(0)   
      , m_resExpArea(0)        
      , m_commExpArea(0)       
      , m_totalExpArea(0)      
      , m_currentArea(0)       
      , m_currentResArea(0)    
      , m_currentCommArea(0)   
      , m_currentPopulation(0) 
      , m_newPopulation(0)     
      , m_popIncr(0)           
      , m_capacity(0)          
      , m_pctAvailCap(0)       
      , m_avgAllowedDensity(0) 
      , m_avgActualDensity(0)
      , m_impervious(0)
      , m_nextResPriority(0)   
      , m_nextCommPriority(0)
      , m_currentEvent(0)
      , m_use(true)
      , m_computeStartPop(true)
      {}

   };


class PopDens
{
public:
   PopDens() : m_pQuery( NULL ), m_pMapExpr( NULL ), m_use( true ), m_queryArea( 0 ), m_initPop( 0 ), m_reallocPop( 0 ), m_queryPop( 0 ) { }
   ~PopDens() { }

   CString m_name;
   CString m_query;
   CString m_expr;

   bool m_use;

   Query   *m_pQuery;      // memory managed by Envision
   MapExpr *m_pMapExpr;    // memory managed by ???

   // run time support
   float m_queryArea;
   float m_initPop;     // based on popDens in the initial coverage (before reallocation)
   float m_reallocPop;  // base on popDens after reallocation
   float m_queryPop;    // current value of the expression for pop density 
};


class DUArea
{
public:
   DUArea( void ) : m_pQuery( NULL ), /*m_id( -1 ), */ m_index( -1 ), m_use( true ), 
      m_peoplePerDU( 2.3f ), /* m_newDUs( 0 ),*/ m_nDUs( 0 ), m_startNDUs( 0 ), m_duDeficit( 0 ),
      m_lastPopulation( 0 ), m_population( 0 ) { }

public:
   CString m_name;
   CString m_query;

   Query   *m_pQuery;     // memory managed by Envision
   //MapExpr *m_pMapExpr;

   //int   m_id;
   int   m_index;
   bool  m_use;
   float m_peoplePerDU;

   // run time support
   //int m_newDUs;
   int m_nDUs;
   int m_startNDUs;
   float m_duDeficit;
   float m_lastPopulation;
   float m_population;
};



class UxTrigger
   {
   public:
      CString m_name;
      bool m_use;
      CString m_type;
      CString m_queryStr;
      float m_minPop;
      float m_maxPop;
      float m_trigger;
      int   m_startAfter;
      int   m_stopAfter;

      Query* m_pQuery;

      UxTrigger() 
         : m_use(true)
         , m_minPop(-1)
         , m_maxPop(-1)
         , m_trigger(0)
         , m_startAfter(-1)
         , m_stopAfter(-1)
         , m_pQuery(NULL)
         {}
   };

class UxExpandWhen : public UxTrigger
{
public:
   UxExpandWhen( void ) 
      : UxTrigger()
   { }

public:
   //int  m_zoneRes;
   //int  m_zoneComm;
   //bool m_computeStartPop;
 
};


class UxUpzoneWhen : public UxTrigger
   {
   public:
      UxUpzoneWhen(void)
         : UxTrigger()
         , m_step(-1)
         , m_rural(0)
      { }

   public:
      int m_step;
      int m_rural;           // >0 indicates rule to be applied outside UGA with m_adjacent distance
   };


class UxScenario
{
public:
   UxScenario(void)
      : m_id(-1)
      , m_planHorizon(20)
      , m_expandTrigger(0.50f)
      , m_estGrowthRate(0.01f)
      , m_newResDensity(0.01f)
      , m_resCommRatio(8)
      , m_ppdu(2.3f)
      , m_pResQuery(NULL)
      , m_pCommQuery(NULL)
   {}

   int      m_id;
   CString  m_name;

   int m_planHorizon;
   float m_expandTrigger;  // fraction of avail capacity that triggers an expansion event
   float m_estGrowthRate;  // annual estimated growth rate, decimal percent (required)
   float m_newResDensity;  // estimate density (du/ac) to new residential (required)
   float m_resCommRatio;   // ratio of residential area to comm/ind area in urban expansion areas (required)
   float m_ppdu;           // people per dwelling unit 
   float m_imperviousFactor;  // factor scaling new impervious development

   CString m_resQuery;
   CString m_commQuery;

   int  m_zoneRes;
   int  m_zoneComm;

   Query* m_pResQuery;
   Query* m_pCommQuery;

   PtrArray< UxExpandWhen > m_uxExpandArray;
   PtrArray< UxUpzoneWhen > m_uxUpzoneArray;
   };



class ZoneInfo
{
public:
   ZoneInfo( void ) : m_id( -1 ), m_index(-1), m_density( 0 ), m_pQuery( NULL ) { }

   int     m_id;
   int     m_index;
   float   m_density;    // du/ac
   float   m_imperviousFraction;  // decimal percent imperviosu associaetd with zoning
   CString m_query;
   
   Query  *m_pQuery;
};


class _EXPORT UrbanDev : public EnvModelProcess
{
public:
   UrbanDev( void );
   ~UrbanDev( void ) { if ( m_pUxData != NULL ) delete m_pUxData; }

   bool Init   ( EnvContext *pContext, LPCTSTR initStr );
   bool InitRun( EnvContext *pContext, bool useInitialSeed );
   bool Run    ( EnvContext *pContext );
   bool EndRun ( EnvContext* pContext );
   bool Setup  ( EnvContext *pContext, HWND hWnd ) { return FALSE; }

protected:
   //bool Run( EnvContext *pContext, bool AddDelta );




   //------------------------------------------------
   //-- Globals -------------------------------------
   //------------------------------------------------
   bool LoadXml( LPCTSTR filename, EnvContext* );
   bool InitUGAs(EnvContext* pContext);

   void InitOutputs();
   
   // operation flags
   bool m_allocatePopDens;   // true if <pop_dens> tags defined
   bool m_allocateDUs;
   bool m_expandUGAs;         // true if <uga_expansions> defined
   bool m_upzoneUGAs;         // true if <uga_upzone> defined

   // (optional) Buildings point layer
   CString   m_duPtLayerName;
   MapLayer *m_pDuPtLayer;
   int       m_colPtNDU;
   int       m_colPtNewDU;
   PolyPointMapper m_polyPtMapper;  // provides mapping between idu layer, building (point) layer


   //-------------------------------------------------
   //-- Impervious
   //-------------------------------------------------

   float GetImperviousFromZone(int zone);

   void UpdateUGAPops(EnvContext* pContext);



   //--------------------------------------------------
   //-- UGAs ------------------------------------------
   //--------------------------------------------------
   PtrArray<UGA> m_ugaArray;
   CMap< int, int, UGA*, UGA* > m_idToUGAMap;   // maps unique DUArea identifiers to the corresponding DUArea ptr
   
   int AddUGA(UGA* pUGA);
   UGA* FindUGAFromID(int id) {UGA* pUGA = NULL;  BOOL ok = m_idToUGAMap.Lookup(id, pUGA);  if (ok) return pUGA; else return NULL; }
   UGA* FindUGAFromName(LPCTSTR name);


   //--------------------------------------------------
   //-- PopAlloc --------------------------------------
   //--------------------------------------------------
   bool InitPopDens( EnvContext *pContext  );
   PopDens *AddPopDens( EnvContext *pContext );

   PtrArray< PopDens > m_popDensArray;
   float m_startPop;

   //--------------------------------------------------
   //-- Dwelling units --------------------------------
   //--------------------------------------------------
   DUArea *AddDUArea( void ); //MapLayer *pLayer, int id, bool use );
   DUArea *FindDUArea( int idu );
   void InitNDUs( MapLayer *pLayer );
   void AllocateNewDUs( EnvContext *pContext, MapLayer *pLayer );

   int   m_colNDU;
   int   m_colNewDU;
   int   m_initDUs;       // flag indicating source of nDU info during initialization.  
                          // 0=from coverage (default), 1=from popDens, 2=from building point layer
                          // if 1 or 2, IDU nDU column will be populated
  // output data
   FDataObj m_nDUData;      // for each uga + totals
   FDataObj m_newDUData;

   int *m_duPtIndexArray;

   //--------------------------------------------------
   //-- UGA Expansion ---------------------------------
   //--------------------------------------------------
   // urban expansion data
   int m_currUxScenarioID;      // exposed scenario variable
   UxScenario* m_pCurrentUxScenario;
   PtrArray< UxScenario > m_uxScenarioArray;

   PtrArray< ZoneInfo > m_ruralResZoneArray;
   PtrArray< ZoneInfo > m_resZoneArray;
   PtrArray< ZoneInfo > m_commZoneArray;

   int m_colPopCap;
   int m_colUxNearDist;
   int m_colUxNearUga;
   int m_colUxPriority;   // input (optional) - if defined, indicates priority of expansions (0=high)
   int m_colUxEvent;      // output, tags locations for each DUArea expansion event
   int m_colPopDensInit;      // initial population density
   int m_colImpervious;
   int m_colUgaPop;

   // methods
   int UxAddExpandWhen( UxScenario *pScenario, UxExpandWhen *pExpandWhen);
   int UxAddUpzoneWhen(UxScenario* pScenario, UxUpzoneWhen* pUpzoneWhen);
   UxScenario   *UxFindScenarioFromID( int id );
   bool UxExpandUGAs( EnvContext *pContext, MapLayer *pLayer );
   bool UxExpandUGA(UGA* pUGA, UxExpandWhen *pExpand, EnvContext* pContext);
   bool UxUpzoneUGA(UGA* pUGA, UxUpzoneWhen* pUpzone, EnvContext* pContext);

   void UxGetDemandAreas(UxScenario* pScenario, UGA* pUGA, float& resExpArea, float& commExpArea);
   void UxUpdateImperiousFromZone(EnvContext* pContext);



   float UxUpdateUGAStats( EnvContext *pContext, bool outputStartInfo );
   bool  UxPrioritizeUxAreas( EnvContext *pContext );
   float UxGetAllowedDensity(MapLayer* pLayer, int idu, int zone);      // du/ac
   bool  UxIsResidential(int zone);
   bool  UxIsRuralResidential(int zone);
   bool  UxIsCommercial(int zone);

   //-- general --------------------------------------
   void  ShufflePolyIDs( void ) { ShuffleArray< UINT >( m_polyIndexArray.GetData(), m_polyIndexArray.GetSize(), &m_randShuffle ); }

   int m_colUga;
   int m_colZone;
   int m_colPopDens;
   int m_colArea;

   // data collection
   FDataObj* m_pUxData;
   
   void InitOutput();
   void CollectOutput( int year );
  
   
   // supporting members
   QueryEngine *m_pQueryEngine;
   

   // dwelling units
   PtrArray< DUArea > m_duAreaArray;

   CUIntArray m_polyIndexArray;
   RandUniform m_randShuffle;    // used for shuffling arrays
   CUIntArray m_nDUs;
   CUIntArray m_prevNDUs;
   CUIntArray m_startNDUs;
   CArray< float > m_priorPops;
};


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new UrbanDev; }
