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

#define _EXPORT __declspec( dllexport )


struct NEW_ZONE 
   {
   int zone;
   float probability;

   NEW_ZONE(int _zone, float _prob) : zone(_zone), probability(_prob) {}
   };

struct RURAL_PRIORITY
   {
   public:
      int idu;    // in idu coverage
      float priority;

      RURAL_PRIORITY(int _idu, float _priority) : idu(_idu), priority(_priority) { }
   };

struct RuralArea  // new
   {
   int m_id;
   CString m_name;
   CString m_queryStr;
   Query* m_pQuery = nullptr;

   float m_startPopulation;         // people - set in init 
   float m_upzoneArea;              // m2
   float m_currentArea;             // m2
   float m_currentResArea;          // m2
   //float m_currentCommArea;         // m2
   float m_currentPopulation;       // people
   float m_newPopulation;           // people
   float m_popIncr;                 // people
   float m_capacity;                // people
   float m_availCapacity;           // people
   float m_pctAvailCap;             // fraction
   float m_avgAllowedDensity;       // du/ac
   float m_avgActualDensity;        // du/ac
   float m_impervious;              // % impervious

   int m_nextResPriority;     // ptrs to current spot in list
   //int m_nextCommPriority;
   //int m_currentEvent;

   //std::vector < unique_ptr<RURAL_PRIORITY> > m_priorityListRes;
   PtrArray< RURAL_PRIORITY > m_priorityListRes;   // sorted lists of UGB Expansion areas
   //PtrArray< RURAL_PRIORITY > m_priorityListComm;
   std::vector<UINT> m_iduArray;

   CMap<int, int, UINT, UINT> m_iduToUpzonedMap;   // actual IDU index to count of upzones

   std::vector<int> m_upzoneCriteria;   // -1=n/a, 0=not met, 1=met
   
   // temproary variable fr IDU score calculations
   //float m_iduScore = 0;

   bool m_use;

   RuralArea(void)
      : m_id(-1)
      , m_startPopulation(0)   
      //, m_resExpArea(0)        
      //, m_commExpArea(0)       
      //, m_totalExpArea(0)
      , m_upzoneArea(0)
      , m_currentArea(0)       
      , m_currentResArea(0)    
      //, m_currentCommArea(0)   
      , m_currentPopulation(0) 
      , m_newPopulation(0)     
      , m_popIncr(0)           
      , m_capacity(0)          
      , m_pctAvailCap(0)       
      , m_avgAllowedDensity(0) 
      , m_avgActualDensity(0)
      , m_impervious(0)
      , m_nextResPriority(0)   
      //, m_nextCommPriority(0)
      //, m_currentEvent(0)
      , m_use(true)
      {}

   void  ShuffleIDUs(RandUniform &rn) {
      ShuffleArray< UINT >(m_iduArray.data(), m_iduArray.size(), &rn);
      }


   };


class RuralPreference
   {
   public:
      CString m_name;
      CString m_queryStr;
      Query* m_pQuery = nullptr;
      float m_value = 0;
   };


class RuralTrigger
   {
   public:
      CString m_name;
      bool m_use = true;
      CString m_type;
      CString m_queryStr;
      float m_minPop = -1;
      float m_maxPop = -1;
      float m_trigger = 0;
      int   m_startAfter = -1;
      int   m_stopAfter = -1;

      Query* m_pQuery = nullptr;

      RuralTrigger() {}
   };


class RuralUpzoneWhen : public RuralTrigger
   {
   public:
      RuralUpzoneWhen(void) : RuralTrigger() { }

   public:
      int GetNewZone();
      int AddPreference(RuralPreference* pPref) { return (int)this->m_prefArray.Add(pPref); }
      
      // members
      int m_steps = -1;
      std::vector<NEW_ZONE> m_newZones;
      float m_maxExpandArea = 5000000.0f;  // 500ha 
      float m_demandFrac = 1.0f;

      PtrArray< RuralPreference> m_prefArray;
   };


class RuralScenario
{
public:
   RuralScenario(void)
      : m_id(-1)
      , m_planHorizon(20)
      , m_estGrowthRate(0.01f)
      , m_newResDensity(0.01f)
      //, m_resCommRatio(8)
      , m_ppdu(2.3f)
      , m_pResQuery(NULL)
      //, m_pCommQuery(NULL)
   {}

   int      m_id;
   CString  m_name;

   int m_planHorizon;
   float m_estGrowthRate;  // annual estimated growth rate, decimal percent (required)
   float m_newResDensity;  // estimate density (du/ac) to new residential (required)
   //float m_resCommRatio;   // ratio of residential area to comm/ind area in urban expansion areas (required)
   float m_ppdu;           // people per dwelling unit 
   float m_imperviousFactor;  // factor scaling new impervious development
   float m_maxExpandFraction = 0.1f;

   CString m_resQuery;
   //CString m_commQuery;

   int  m_zoneRes;
   //int  m_zoneComm;

   Query* m_pResQuery;
   //Query* m_pCommQuery;

   PtrArray< RuralUpzoneWhen > m_upzoneArray;
   //PtrArray< RuralPreference> m_prefArray;

   int AddUpzoneWhen(RuralUpzoneWhen* pUpzoneWhen) {return (int)this->m_upzoneArray.Add(pUpzoneWhen);}
   //int AddPreference(RuralPreference* pPref) { return (int)this->m_prefArray.Add(pPref); }
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


class _EXPORT RuralDev : public EnvModelProcess
{
public:
   RuralDev( void );
   ~RuralDev( void ) { if ( m_pRuralData != NULL ) delete m_pRuralData; }

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
   bool InitRAs(EnvContext* pContext);

   void InitOutputs();
   
   // operation flags
   bool m_upzone;         // true if upzone> defined

   //-------------------------------------------------
   //-- Impervious
   //-------------------------------------------------

   float GetImperviousFromZone(int zone);

   //void UpdateRAPops(EnvContext* pContext);



   //--------------------------------------------------
   //-- UGAs ------------------------------------------
   //--------------------------------------------------
   PtrArray<RuralArea> m_raArray;
   CMap< int, int, RuralArea*, RuralArea* > m_idToRAMap;   // maps unique DUArea identifiers to the corresponding DUArea ptr
   
   int AddRA(RuralArea* pRA);
   //RuralArea* FindRAFromID(int id) {RuralArea* pRA = NULL;  BOOL ok = m_idToRAMap.Lookup(id, pRA);  if (ok) return pRA; else return nullptr; }
   RuralArea* FindRAFromName(LPCTSTR name);

   //--------------------------------------------------
   //-- RuralArea Growth ---------------------------------
   //--------------------------------------------------
   // urban growth data
   int m_currScenarioID;      // exposed scenario variable
   RuralScenario* m_pCurrentScenario;
   PtrArray< RuralScenario > m_scenarioArray;

   PtrArray< ZoneInfo > m_ruralResZoneArray;

   int m_colArea = -1;
   int m_colPopDens = -1;
   int m_colPopDensInit = -1;
   int m_colPopCap=-1;
   int m_colAvailCap = -1;
   int m_colPriority = -1;
   int m_colImpervious = -1;
   int m_colZone = -1;


   // methods
   RuralScenario   *FindScenarioFromID( int id );
   bool UpzoneRAs( EnvContext *pContext, MapLayer *pLayer );
   bool UpzoneRA(RuralArea* pUGA, RuralUpzoneWhen* pUpzone, EnvContext* pContext);

   float GetResDemand(RuralScenario* pScenario, RuralArea* pRA);// , float& resExpDemand, float& commExpDemand);
   void  UpdateImperiousFromZone(EnvContext* pContext);

   float UpdateRAStats( EnvContext *pContext, bool outputStartInfo );
   bool  PrioritizeRuralArea(EnvContext* pContext, RuralUpzoneWhen* pUpzone, RuralArea* pRA);
   float GetAllowedDensity(MapLayer* pLayer, int idu, int zone);      // du/ac
   //bool  IsResidential(int zone);
   bool  IsRuralResidential(int zone);
   //bool  IsCommercial(int zone);

   //-- general --------------------------------------
   //void  ShufflePolyIDs(void) { ShuffleArray< UINT >(m_polyIndexArray.data(), m_polyIndexArray.size(), &m_randShuffle); }


   // data collection
   FDataObj* m_pRuralData;
   
   void InitOutput();
   void CollectOutput( int year );
   
   // supporting members
   QueryEngine *m_pQueryEngine;
   

   //CUIntArray m_polyIndexArray;
   std::vector<UINT> m_polyIndexArray;

   RandUniform m_randShuffle;    // used for shuffling arrays
};


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new RuralDev; }
