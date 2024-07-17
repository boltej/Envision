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
// RuralDev.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "RuralDev.h"
#include <Maplayer.h>
#include <Map.h>
#include <Report.h>
#include <tixml.h>
#include <PathManager.h>
#include <UNITCONV.H>
#include <envconstants.h>
#include <EnvModel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int ComparePriorities(const void *elem0, const void *elem1 );
//float GetPeoplePerDU( int );

RandUniform randUnif;

int RuralUpzoneWhen::GetNewZone()
   {
   int zones = (int)m_newZones.size();

   ASSERT(zones > 0);
   if (zones == 1)
      return m_newZones[0].zone;

   float rn = (float) randUnif.RandValue(0, 1);
   float rnSoFar = 0;

   for (int i = 0; i < zones; i++)
      {
      rnSoFar += m_newZones[i].probability;

      if (rnSoFar >= rn)
         return m_newZones[i].zone;
      }

   // shouldn't ever get here
   ASSERT(true);
   return -1;
   }



RuralDev::RuralDev() 
: EnvModelProcess()
, m_colPopDens( -1 )
, m_colArea( -1 )
, m_colZone( -1 )
, m_colPopCap(-1)
, m_colAvailCap(-1)
, m_pQueryEngine( NULL )
, m_currScenarioID( 0 )
, m_pCurrentScenario( NULL )
, m_pRuralData(NULL)
{ }


bool RuralDev::Init( EnvContext *pContext, LPCTSTR initStr )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;

   m_pQueryEngine = pContext->pQueryEngine;

   if ( LoadXml( initStr, pContext ) == false )
      return false;


   // global things first
   InitRAs(pContext);

   AddInputVar("RuralScenarioID", m_currScenarioID, "Use this Rural Scenario");

   if (m_colImpervious >= 0)
      UpdateImperiousFromZone(pContext);   // writes to impervious col in IDUs

   // prioritize UX expansion areas  (this should be moved to init????
   //PrioritizeRuralAreas(pContext);  // GET RID OF
   

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
   //this->UpdateRAPops(pContext); //??? still needed?
   return TRUE; 
   }



bool RuralDev::InitRAs(EnvContext* pContext)
   {
   MapLayer* pLayer = (MapLayer*) pContext->pMapLayer;

   // for each IDU, add to lookup tables
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      for (int i = 0; i < (int)m_raArray.GetSize(); i++)
         {
         RuralArea* pRA = m_raArray[i];

         if (pRA != NULL)
            {
            pRA->m_upzoneArea = 0;
            pRA->m_impervious = 0;

            bool result = false;
            bool ok = pRA->m_pQuery->Run(idu, result);

            if (ok && result)
               {
               pRA->m_iduArray.push_back(idu);
               pRA->m_iduToUpzonedMap[idu] = 0;
               }
            }
         }
      }

   UpdateRAStats(pContext, false);

   return true;
   }




bool RuralDev::InitRun( EnvContext *pContext, bool useInitialSeed )
   {
   ASSERT(this->m_colAvailCap >= 0);
   this->m_pRuralData->ClearRows();
   // get the current growth scenario
   m_pCurrentScenario = this->FindScenarioFromID(m_currScenarioID);

   if (m_colImpervious >= 0)
      UpdateImperiousFromZone(pContext);   // writes to impervious col in IDUs


   for ( int i=0; i < this->m_raArray.GetSize(); i++ )
      {
      RuralArea* pRA = this->m_raArray[i];
      // prioritize UX expansion areas  (this should be moved to init????
      //PrioritizeRuralAreas(pContext);

      pRA->m_upzoneCriteria.resize(m_pCurrentScenario->m_upzoneArray.GetSize());
      std::fill(pRA->m_upzoneCriteria.begin(), pRA->m_upzoneCriteria.end(), -1);

      pRA->m_upzoneArea = 0;
      pRA->m_impervious = 0;
 
      UpdateRAStats(pContext, true);
      }

   //UpdateRAPops(pContext);

   return TRUE; 
   }


bool RuralDev::Run( EnvContext *pContext )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   int iduCount = pLayer->GetRecordCount();

   UpzoneRAs(pContext, pLayer );
   UpdateRAStats(pContext, false);
      
   //UpdateUGAPops(pContext);

   CollectOutput( pContext->currentYear );

   return true;
   }


bool RuralDev::EndRun(EnvContext* pContext)
   {
   return true;
   }

   

int RuralDev::AddRA( RuralArea *pRA )
   {
   int index = (int) m_raArray.Add(pRA);
   //pRA->m_index = index;
   m_idToRAMap.SetAt( pRA->m_id, pRA );

   return index;
   }

RuralArea *RuralDev::FindRAFromName( LPCTSTR name )
   {
   for ( int i=0; i < (int) m_raArray.GetSize(); i++ )
      if ( m_raArray[ i ]->m_name.CompareNoCase( name ) == 0 )
         return m_raArray[ i ];

   return NULL;
   }

RuralScenario *RuralDev::FindScenarioFromID( int id )
   {
   for ( int i=0; i < (int) m_scenarioArray.GetSize(); i++ )
      if ( m_scenarioArray[ i ]->m_id == id )
         return m_scenarioArray[ i ];

   return NULL;
   }


void RuralDev::InitOutput()
   {
   if (m_pRuralData == NULL)
      {
      // get current scenario
      m_pCurrentScenario = this->FindScenarioFromID(m_currScenarioID);

      // currentPopulation, currentArea, newPopulation, pctAvilCapacity
      int ugaCount = (int)m_raArray.GetSize();
      int cols = 1 + ((ugaCount+1) * 6);
      m_pRuralData = new FDataObj(cols, 0, U_YEARS);
      ASSERT(m_pRuralData != NULL);
      m_pRuralData->SetName("RuralDev");

      m_pRuralData->SetLabel(0, _T("Time"));

      int col = 1;
      for (int i = 0; i < m_raArray.GetSize(); i++)
         {
         RuralArea* pRA = m_raArray[i];
         CString name = pRA->m_name;
         m_pRuralData->SetLabel(col++, _T(name + ".Population"));
         m_pRuralData->SetLabel(col++, _T(name + ".Area (ac)"));
         m_pRuralData->SetLabel(col++, _T(name + ".New Population"));
         m_pRuralData->SetLabel(col++, _T(name + ".Pct Available Capacity"));
         //m_pRuralData->SetLabel(col++, _T(name + ".Res Expansion Area (ac)"));
         //m_pRuralData->SetLabel(col++, _T(name + ".Comm Expansion Area (ac)"));
         //m_pRuralData->SetLabel(col++, _T(name + ".Total Expansion Area (ac)"));
         m_pRuralData->SetLabel(col++, _T(name + ".Upzone Area (ac)"));
         m_pRuralData->SetLabel(col++, _T(name + ".Impervious Fraction"));
         }

      // add totals
      m_pRuralData->SetLabel(col++, _T("Total.Population"));
      m_pRuralData->SetLabel(col++, _T("Total.Area (ac)"));
      m_pRuralData->SetLabel(col++, _T("Total.New Population"));
      m_pRuralData->SetLabel(col++, _T("Total.Pct Available Capacity"));
      //m_pRuralData->SetLabel(col++, _T("Total.Res Expansion Area (ac)"));
      //m_pRuralData->SetLabel(col++, _T("Total.Comm Expansion Area (ac)"));
      //m_pRuralData->SetLabel(col++, _T("Total.Total Expansion Area (ac)"));
      m_pRuralData->SetLabel(col++, _T("Total.Total Upzone Area (ac)"));
      m_pRuralData->SetLabel(col++, _T("Total.Impervious Fraction"));
      ASSERT(col == cols);
      }

   this->AddOutputVar("RuralDev", m_pRuralData, "");
   }

void RuralDev::CollectOutput( int year )
   {
   ASSERT(m_pRuralData != NULL);

   // currentPopulation, currentArea, newPopulation, pctAvilCapacity
   int raCount = (int)m_raArray.GetSize();
   int cols = 1 + ((raCount+1) * 6);

   float* data = new float[cols];
   data[0] = (float) year;

   float totalPop = 0.0f;
   float totalArea = 0;
   float totalAc = 0.0f;
   float totalNewPop = 0.0f;
   float totalPctAvailCap = 0.0f;
   //float totalResExpAc = 0.0f;
   //float totalCommExpAc = 0.0f;
   //float totalExpAc = 0.0f;
   float totalUpzoneAc = 0.0f;
   float totalImpervious = 0.0f;
   int col = 1;

   for (int i = 0; i < m_raArray.GetSize(); i++)
      {
      RuralArea* pRA = m_raArray[i];

      data[col++] = pRA->m_currentPopulation;
      data[col++] = pRA->m_currentArea * ACRE_PER_M2;
      data[col++] = pRA->m_newPopulation;
      data[col++] = pRA->m_pctAvailCap;
      //data[col++] = pRA->m_resExpArea * ACRE_PER_M2;
      //data[col++] = pRA->m_commExpArea * ACRE_PER_M2;
      //data[col++] = pRA->m_totalExpArea * ACRE_PER_M2;
      data[col++] = pRA->m_upzoneArea * ACRE_PER_M2;
      data[col++] = pRA->m_impervious;

      totalPop         += pRA->m_currentPopulation;
      totalArea        += pRA->m_currentArea;
      totalAc          += pRA->m_currentArea * ACRE_PER_M2;
      totalNewPop      += pRA->m_newPopulation;
      totalPctAvailCap += (pRA->m_pctAvailCap * pRA->m_currentArea * ACRE_PER_M2);  //area weighted by UgUGA area
      //totalResExpAc    += pRA->m_resExpArea * ACRE_PER_M2;
      //totalCommExpAc   += pRA->m_commExpArea * ACRE_PER_M2;
      //totalExpAc       += pRA->m_totalExpArea * ACRE_PER_M2;
      totalUpzoneAc       += pRA->m_upzoneArea * ACRE_PER_M2;
      totalImpervious += pRA->m_impervious * pRA->m_currentArea;
      }

   data[col++] = totalPop;
   data[col++] = totalAc;
   data[col++] = totalNewPop;
   data[col++] = totalPctAvailCap / totalAc;
   //data[col++] = totalResExpAc;
   //data[col++] = totalCommExpAc;
   //data[col++] = totalExpAc;
   data[col++] = totalUpzoneAc;
   data[col++] = totalImpervious/totalArea;
   ASSERT(col == cols);
   m_pRuralData->AppendRow(data, cols);
   delete[] data;
   }
   

bool RuralDev::LoadXml( LPCTSTR _filename, EnvContext *pContext )
   {
   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   MapExprEngine *pMapExprEngine = pContext->pExprEngine;  // use Envision's MapExprEngine

   CString filename;
   if ( PathManager::FindPath( _filename, filename ) < 0 ) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format( "Input file '%s' not found - this process will be disabled", _filename );
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
   TiXmlElement *pXmlRoot = doc.RootElement();  // rural_dev

   LPTSTR name = NULL, popDensCol = NULL, popDensInitCol=NULL, popCapCol = NULL, popAvailCol = NULL, 
      priorityCol = NULL, zoneCol = NULL, query = NULL;

   XML_ATTR attrs[] = {
         // attr              type        address       isReq checkCol
         { "name",            TYPE_STRING, &name,          true,    0 },
         { "popDens_col",     TYPE_STRING, &popDensCol,    true,    CC_MUST_EXIST },
         { "pop_cap_col",     TYPE_STRING, &popCapCol,     true,    CC_MUST_EXIST },
         { "avail_cap_col",   TYPE_STRING, &popAvailCol,   true,    CC_MUST_EXIST },
         { "init_popdens_col",TYPE_STRING, &popDensInitCol,true,    CC_MUST_EXIST },
         { "priority_col",    TYPE_STRING, &priorityCol,   true,    CC_AUTOADD | TYPE_FLOAT },
         { "zone_col",        TYPE_STRING, &zoneCol,       true,    CC_MUST_EXIST },
         { NULL,              TYPE_NULL,    NULL,          false,   0 } };

   ok = TiXmlGetAttributes( pXmlRoot, attrs, filename, pLayer );

   if ( ! ok )
      return false;

   this->m_colArea     = pLayer->GetFieldCol( "AREA" );
   this->m_colPopDens  = pLayer->GetFieldCol(popDensCol );
   this->m_colPopDensInit = pLayer->GetFieldCol(popDensInitCol);
   this->m_colPopCap   = pLayer->GetFieldCol(popCapCol);
   this->m_colAvailCap = pLayer->GetFieldCol(popAvailCol);
   this->m_colPriority = pLayer->GetFieldCol(priorityCol);
   this->m_colZone = pLayer->GetFieldCol(zoneCol);

   //------------------------------------------------------------
   //-- UGAs ----------------------------------------------------
   //------------------------------------------------------------
   TiXmlElement* pXmlRAs = pXmlRoot->FirstChildElement(_T("rural_areas"));
   TiXmlElement* pXmlRA = pXmlRAs->FirstChildElement(_T("rural_area"));

   while (pXmlRA != NULL)
      {
      LPCTSTR name = NULL;
      int id = -1;
      float startPop = -1;
      LPCTSTR query = NULL;

      XML_ATTR ugaAttrs[] = {
         // attr        type           address     isReq  checkCol
         { "name",      TYPE_STRING,   &name,      true,    0 },
         { "id",        TYPE_INT,      &id,        true,    0 },
         { "query",     TYPE_STRING,   &query,     true,    0 },
         { NULL,        TYPE_NULL,       NULL,   false,   0 } };

      ok = TiXmlGetAttributes(pXmlRA, ugaAttrs, filename);
      if (!ok)
         {
         CString msg;
         msg.Format(_T("Misformed element reading <RuralArea> attributes in input file %s"), filename);
         Report::ErrorMsg(msg);
         return false;
         }

      RuralArea* pRA = new RuralArea;
      pRA->m_id = id;
      pRA->m_name = name;
      pRA->m_queryStr = query;

      AddRA(pRA);

      // compile res query
      if (pRA->m_queryStr.GetLength() > 0)
         {
         ASSERT(m_pQueryEngine != NULL);
         pRA->m_pQuery = m_pQueryEngine->ParseQuery(pRA->m_queryStr, 0, pRA->m_name);
         }

      pXmlRA = pXmlRA->NextSiblingElement("rural_area");
      }

      
   //---------------------------------------------------------------------------------------------
   //-- scenarios     ----------------------------------------------------------------------------
   //---------------------------------------------------------------------------------------------
   TiXmlElement* pXmlScns = pXmlRoot->FirstChildElement(_T("scenarios"));
   TiXmlElement *pXmlScn = pXmlScns->FirstChildElement( _T("scenario" ) );
      
   while ( pXmlScn != NULL )
      {
      RuralScenario *pScn = new RuralScenario;
   
      XML_ATTR scnAttrs[] = {
         // attr                      type           address                      isReq  checkCol
         { "name",                  TYPE_CSTRING,   &(pScn->m_name),            true,    0 },
         { "id",                    TYPE_INT,       &(pScn->m_id),              true,    0 },
         { "plan_horizon",          TYPE_INT,       &(pScn->m_planHorizon),     true,    0 },
         { "est_growth_rate",       TYPE_FLOAT,     &(pScn->m_estGrowthRate),   true,    0 },
         { "new_res_density",       TYPE_FLOAT,     &(pScn->m_newResDensity),   true,    0 },
         //{ "res_comm_ratio",        TYPE_FLOAT,     &(pUgScn->m_resCommRatio),    true,    0 },
         //{ "res_constraint",        TYPE_CSTRING,   &(pUgScn->m_resQuery),        true,    0 },
         //{ "comm_constraint",       TYPE_CSTRING,   &(pUgScn->m_commQuery),       true,    0 },
         //{ "res_zone",              TYPE_INT,       &(pUgScn->m_zoneRes),         false,   0 },
         //{ "comm_zone",             TYPE_INT,       &(pUgScn->m_zoneComm),        false,   0 },
         { "ppdu",                  TYPE_FLOAT,     &(pScn->m_ppdu),            false,   0 },
         //{ "max_expand_frac",       TYPE_FLOAT,     &(pUgScn->m_maxExpandFraction), false,   0 },
         { "new_impervious_factor", TYPE_FLOAT,     &(pScn->m_imperviousFactor),false,   0 },
         { NULL,               TYPE_NULL,      NULL,                         false,   0 } };

      ok = TiXmlGetAttributes( pXmlScn, scnAttrs, filename );
      if ( ! ok )
         {
         CString msg; 
         msg.Format( _T("Misformed element reading <scenario> attributes in input file %s"), filename );
         Report::ErrorMsg( msg );
         delete pScn;
         return false;
         }

      // add RuralUpzoneWhen element
      TiXmlElement* pXmlUpzone = pXmlScn->FirstChildElement(_T("upzone_when"));
      while (pXmlUpzone != NULL)
         {
         RuralUpzoneWhen* pUpzone = new RuralUpzoneWhen;
         LPCTSTR newZone = nullptr;

         XML_ATTR upAttrs[] = {
            // attr                 type           address                      isReq  checkCol
            { "name",             TYPE_CSTRING,   &(pUpzone->m_name),            false,   0 },
            { "use",              TYPE_BOOL,      &(pUpzone->m_use),             false,   0 },
            { "type",             TYPE_CSTRING,   &(pUpzone->m_type),            false,   0 },
            { "query",            TYPE_CSTRING,   &(pUpzone->m_queryStr),        false,   0 },
            { "new_zone",         TYPE_STRING,    &newZone,                      false,   0 },
            { "min_pop",          TYPE_FLOAT,     &(pUpzone->m_minPop),          false,   0 },
            { "max_pop",          TYPE_FLOAT,     &(pUpzone->m_maxPop),          false,   0 },
            { "trigger",          TYPE_FLOAT,     &(pUpzone->m_trigger),         false,   0 },
            { "start_after",      TYPE_INT,       &(pUpzone->m_startAfter),      false,   0 },
            { "stop_after",       TYPE_INT,       &(pUpzone->m_stopAfter),       false,   0 },
            { "steps",            TYPE_INT,       &(pUpzone->m_steps),           false,   0 },
            { "max_expand_area",  TYPE_FLOAT,     &(pUpzone->m_maxExpandArea),   false,   0 },
            { "demand_fraction",  TYPE_FLOAT,     &(pUpzone->m_demandFrac),      false,   0 },
            { NULL,               TYPE_NULL,      NULL,                          false,   0 } };

         ok = TiXmlGetAttributes(pXmlUpzone, upAttrs, filename);
         if (!ok)
            {
            CString msg;
            msg.Format(_T("Misformed element reading <upzone_when> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            delete pUpzone;
            return false;
            }

         if (pUpzone->m_use == 0)
            {
            delete pUpzone;
            }
         else
            {
            pScn->AddUpzoneWhen(pUpzone);

            // compile res query
            if (pUpzone->m_queryStr.GetLength() > 0)
               {
               ASSERT(m_pQueryEngine != NULL);
               pUpzone->m_pQuery = m_pQueryEngine->ParseQuery(pUpzone->m_queryStr, 0, pUpzone->m_name);
               }

            CStringArray tokens;
            ::Tokenize(newZone, ",;", tokens);

            for (int k = 0; k < tokens.GetSize(); k++)
               {
               CStringArray tokens2;
               ::Tokenize(tokens[k], ":", tokens2);

               int newZone = atoi(tokens2[0]);
               float prob = 1;
               if (tokens2.GetSize() > 1)
                  prob = (float)atof(tokens2[1])/100;

               pUpzone->m_newZones.push_back(NEW_ZONE(newZone, prob));
               }

            // process preferences
            TiXmlElement* pXmlPref = pXmlUpzone->FirstChildElement(_T("preference"));
            while (pXmlPref != NULL)
               {
               RuralPreference* pPref = new RuralPreference;
            
               XML_ATTR prefAttrs[] = {
                  // attr                 type           address                      isReq  checkCol
                  { "name",             TYPE_CSTRING,   &(pPref->m_name),            false,   0 },
                  { "query",            TYPE_CSTRING,   &(pPref->m_queryStr),        false,   0 },
                  { "value",            TYPE_FLOAT,     &(pPref->m_value),           false,   0 },
                  { NULL,               TYPE_NULL,      NULL,                        false,   0 } };
            
               ok = TiXmlGetAttributes(pXmlPref, prefAttrs, filename);
               if (!ok)
                  {
                  CString msg;
                  msg.Format(_T("Misformed element reading <preferences> attributes in input file %s"), filename);
                  Report::ErrorMsg(msg);
                  delete pPref;
                  return false;
                  }
            
               pUpzone->AddPreference(pPref);
            
               // compile res query
               if (pPref->m_queryStr.GetLength() > 0)
                  {
                  ASSERT(m_pQueryEngine != NULL);
                  pPref->m_pQuery = m_pQueryEngine->ParseQuery(pPref->m_queryStr, 0, pPref->m_name);
            
                  CString msg("Added Rural Preference- ");
                  msg += pPref->m_name;
                  Report::Log(msg);
                  }
            
               pXmlPref = pXmlPref->NextSiblingElement("preference");
               }  // end of: while ( pXmlPref != NULL )





            CString msg("Added RuralUpzoneWhen - ");
            msg += pUpzone->m_name;
            Report::Log(msg);
            }

         pXmlUpzone = pXmlUpzone->NextSiblingElement("upzone_when");
         }  // end of: while ( pXmlUupzone != NULL )

      // process preferences
      /*
      TiXmlElement* pXmlPref = pXmlScn->FirstChildElement(_T("preference"));
      while (pXmlPref != NULL)
         {
         RuralPreference* pPref = new RuralPreference;

         XML_ATTR prefAttrs[] = {
            // attr                 type           address                      isReq  checkCol
            { "name",             TYPE_CSTRING,   &(pPref->m_name),            false,   0 },
            { "query",            TYPE_CSTRING,   &(pPref->m_queryStr),        false,   0 },
            { "value",            TYPE_FLOAT,     &(pPref->m_value),           false,   0 },
            { NULL,               TYPE_NULL,      NULL,                        false,   0 } };

         ok = TiXmlGetAttributes(pXmlPref, prefAttrs, filename);
         if (!ok)
            {
            CString msg;
            msg.Format(_T("Misformed element reading <preferences> attributes in input file %s"), filename);
            Report::ErrorMsg(msg);
            delete pPref;
            return false;
            }

         pScn->AddPreference(pPref);

         // compile res query
         if (pPref->m_queryStr.GetLength() > 0)
            {
            ASSERT(m_pQueryEngine != NULL);
            pPref->m_pQuery = m_pQueryEngine->ParseQuery(pPref->m_queryStr, 0, pPref->m_name);
           
            CString msg("Added Rural Preference- ");
            msg += pPref->m_name;
            Report::Log(msg);
            }

         pXmlPref = pXmlPref->NextSiblingElement("preference");
         }  // end of: while ( pXmlPref != NULL )
         */

      this->m_scenarioArray.Add( pScn );

      // compile scenario res and comm queries
      if (pScn->m_resQuery.GetLength() > 0)
         {
         ASSERT(m_pQueryEngine != NULL);
         pScn->m_pResQuery = m_pQueryEngine->ParseQuery(pScn->m_resQuery, 0, pScn->m_name);
         }

      // compile comm query
      //if (pScn->m_commQuery.GetLength() > 0)
      //   {
      //   ASSERT(m_pQueryEngine != NULL);
      //   pUgScn->m_pCommQuery = m_pQueryEngine->ParseQuery(pUgScn->m_commQuery, 0, pUgScn->m_name);
      //   }

      pXmlScn = pXmlScn->NextSiblingElement( "scenario");
      }

   if ( m_scenarioArray.GetSize() > 0 )
      m_pCurrentScenario = m_scenarioArray.GetAt( 0 );
      
  // next, zone information      
  TiXmlElement *pXmlZones = pXmlRoot->FirstChildElement( _T("zones" ) );
  
  if ( pXmlZones )
     {
     TiXmlElement* pXmlRuralRes = pXmlZones->FirstChildElement("rural_res");
     while (pXmlRuralRes != NULL)
        {
        ZoneInfo* pZone = new ZoneInfo;

        XML_ATTR zoneAttrs[] = {
           // attr                 type           address                isReq  checkCol
           { "id",               TYPE_INT,       &(pZone->m_id),        true,    0 },
           { "density",          TYPE_FLOAT,     &(pZone->m_density),   true,    0 },
           { "impervious",       TYPE_FLOAT,     &(pZone->m_imperviousFraction),false,   0 },
           { "constraint",       TYPE_CSTRING,   &(pZone->m_query),     false,   0 },
           { NULL,               TYPE_NULL,      NULL,                  false,   0 } };

        ok = TiXmlGetAttributes(pXmlRuralRes, zoneAttrs, filename);
        if (!ok)
           {
           CString msg;
           msg.Format(_T("Misformed element reading <rural_res> attributes in input file %s"), filename);
           Report::ErrorMsg(msg);
           delete pZone;
           }
        else
           {
           pZone->m_index = (int)m_ruralResZoneArray.Add(pZone);

           if (pZone->m_query.GetLength() > 0)
              pZone->m_pQuery = m_pQueryEngine->ParseQuery(pZone->m_query, 0, "RuralDev");
           }
        pXmlRuralRes = pXmlRuralRes->NextSiblingElement("rural_res");
        }

     }  // end of:  if ( pXmlZones )

   return true;
   }




enum UPZONE_CRITERIA { UC_MAX_POP_NOT_MET = 1, UC_MIN_POP_NOT_MET = 2, UC_WRONG_TYPE = 4, UC_BELOW_TRIGGER = 8, UC_NOT_ENOUGH_EVENTS = 16, UC_TOO_MANY_EVENTS = 32 };

bool RuralDev::UpzoneRAs(EnvContext* pContext, MapLayer* pLayer)
   {
   if (m_pCurrentScenario == NULL)
      {
      Report::LogError("No current scenario defined during Run()");
      return false;
      }

   // for each UgUGA, determine available capacity
   UpdateRAStats(pContext, false);   // false

   int raCount = (int) m_raArray.GetSize();

   for (int i = 0; i < raCount; i++)
      {
      RuralArea* pRA = this->m_raArray[i];

      CString msg;
      msg.Format("Rural Area %s (%i): Available Capacity=%.2f", (LPCTSTR)pRA->m_name, pContext->currentYear, pRA->m_pctAvailCap);
      Report::LogInfo(msg);

      // reset arrays to n/a
      std::fill(pRA->m_upzoneCriteria.begin(), pRA->m_upzoneCriteria.end(), 0);

      // Upzones
      CString upzoneCriteria;
      upzoneCriteria.Format("");
      bool didUpzone = false;

      for (int j = 0; j < m_pCurrentScenario->m_upzoneArray.GetSize(); j++)
         {
         RuralUpzoneWhen* pUpzone = m_pCurrentScenario->m_upzoneArray[j];

         // are the criteria for this Upzone event met for this UGA?
         // Note:  Each conditional below is a DISQUALIFYING condition

         // are we above any maxPop associated with this UpZoneWhen?
         if ((pUpzone->m_maxPop >= 0) && (pRA->m_currentPopulation > pUpzone->m_maxPop))
            pRA->m_upzoneCriteria[j] = UC_MAX_POP_NOT_MET; // msg = "max pop criteria not met";

         // are we below any maxPop associated with this UpZoneWhen?
         if ((pUpzone->m_minPop >= 0) && (pRA->m_currentPopulation < pUpzone->m_minPop))
               pRA->m_upzoneCriteria[j] += UC_MIN_POP_NOT_MET; //  msg = "min pop criteria not met";

         // is this UGA type not associated with this UpZoneWhen?
         //if (pUpzone->m_type.Compare(pRA->m_type) != 0)
         //   pRA->m_upzoneCriteria[j] += UC_WRONG_TYPE; //  msg = "wrong UGA type";

         // are we below the available capacity need to trigger thie UpZoneWhen?
         if (pRA->m_pctAvailCap > pUpzone->m_trigger)
            pRA->m_upzoneCriteria[j] += UC_BELOW_TRIGGER; //  msg = "capacity trigger not met"; 

         // if specified, have enough events gone by?
         //if ((pUpzone->m_startAfter >= 0) && (pRA->m_currentEvent < pUpzone->m_startAfter))
         //   pRA->m_upzoneCriteria[j] += UC_NOT_ENOUGH_EVENTS; //   msg = "not enough events have passed";
         //
         //if ((pUpzone->m_stopAfter >= 0) && (pRA->m_currentEvent >= pUpzone->m_stopAfter))
         //   pRA->m_upzoneCriteria[j] += UC_NOT_ENOUGH_EVENTS; //  msg = "too may events have passed";

         if (pRA->m_upzoneCriteria[j] == 0 )
            {
            UpzoneRA(pRA, pUpzone, pContext);
            //didUpzone = true;
            //break;
            }
         else
            {
            int status = pRA->m_upzoneCriteria[j];
            CString msg;
            msg.Format("%s: ", (LPCTSTR)pUpzone->m_name);
            if (status & UC_MAX_POP_NOT_MET)
               msg += "Max Pop Not Met; ";
            if (status & UC_MIN_POP_NOT_MET)
               msg += "Min Pop Not Met; ";
            //if (status & UC_WRONG_TYPE)
            //   msg += "Wrong Type; ";
            if (status & UC_BELOW_TRIGGER)
               msg += "Below Trigger; ";
            if (status & UC_NOT_ENOUGH_EVENTS)
               msg += "Not Enough Events; ";
            if (status & UC_TOO_MANY_EVENTS)
               msg += "Too Many Events; ";

            CString msg2;
            msg2.Format("%s (%i): Upzone Criteria Not met (details: %s)", (LPCTSTR)pRA->m_name, pContext->currentYear, (LPCTSTR)upzoneCriteria);
            //Report::Log(msg2);
            //TRACE((LPCTSTR)msg2);
            }
         }  // end of: for each upzone
      }  // end of: for each RA
      return true;
   }


bool RuralDev::UpzoneRA(RuralArea* pRA, RuralUpzoneWhen* pUpzone, EnvContext* pContext)
   {
   // time to upzone this RA.  Basic idea is to:
   //  1) determine the area needed to accommodate new growth
   //  2) Find a random IDU that qualifies as an upzoneable zone.  
   //  3) Expand around IDU until area need satisfied or exhausted.
   //  4) Repeat until area need satisfied  
   MapLayer* pIDULayer = (MapLayer*)pContext->pMapLayer;

   PrioritizeRuralArea(pContext, pUpzone, pRA); // update the priority list for this RA based on UpzoneWhen preferences

   // how many people to plan for?
   float expPop = m_pCurrentScenario->m_planHorizon * m_pCurrentScenario->m_estGrowthRate * pRA->m_currentPopulation * pUpzone->m_demandFrac; // people
   float addedPop = 0;

   // for IDU's in this rural area, try to upzone until demand met
   int priorityCount = (int)pRA->m_priorityListRes.GetSize();

   for (int i = 0; i < priorityCount; i++)
      {
      int idu = pRA->m_priorityListRes[i]->idu;

      if (pUpzone->m_pQuery != NULL)
         {
         bool result = false;
         bool ok = pUpzone->m_pQuery->Run(idu, result);
         if (!ok || !result)
            continue;
         }

      // get existing zone
      int zone = -1;
      pIDULayer->GetData(idu, this->m_colZone, zone);

      // residential?
      bool upzone = true;
      if (! IsRuralResidential(zone))
         upzone = false;

      // hasn't been previously upzoned?
      UINT uzCount = 0;
      pRA->m_iduToUpzonedMap.Lookup(idu, uzCount);
      if (uzCount > 0)
         upzone = false;

      // additional criteria?
      if (upzone)
         {
         int steps = 0;
         //int upIndex = zIndex;
         int newZone = pUpzone->GetNewZone(); // m_newZone;
        
         int newIndex = -1;
         int oldIndex = -1;
         for (int k = 0; k < this->m_ruralResZoneArray.GetSize(); k++)
            {
            if (this->m_ruralResZoneArray[k]->m_id == newZone)
               newIndex = k;
            if (this->m_ruralResZoneArray[k]->m_id == zone)
               oldIndex = k;
            }

         ASSERT(newIndex != -1);

         float popDelta = 0;   // #/m2
         popDelta = this->m_ruralResZoneArray[newIndex]->m_density;
         if ( oldIndex >= 0 )
            popDelta -= this->m_ruralResZoneArray[oldIndex]->m_density;   // units are du/ac

         popDelta *= m_pCurrentScenario->m_ppdu * ACRE_PER_M2;  // convert to people/m2
                 
         // update the kernel IDU
         //UpdateIDU(pContext, idu, m_colEvent, pRA->m_currentEvent + 1, ADD_DELTA);    // indicate expansion event
         UpdateIDU(pContext, idu, m_colZone, newZone, ADD_DELTA);
         float area = 0;
         pIDULayer->GetData(idu, m_colArea, area);
         addedPop += popDelta * area;

         // flag this IDU as having been upzoned
         pRA->m_iduToUpzonedMap[idu] += 1;

         //------------------------------------------------------------------------------------------------------------------------------------
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
         // the max expansion area is ...
         // target area
         CArray< int, int > expandArray;
         VData _zone(zone);
         float expandArea = pIDULayer->GetExpandPolysFromAttr(idu, m_colZone, _zone, OPERATOR::EQ, m_colArea, pUpzone->m_maxExpandArea, expandArray);
         //float expandArea = pIDULayer->GetExpandPolysFromQuery(idu, pUpzone->m_pQuery, m_colArea, maxExpandArea, expandArray);

         // upzone all expanded polygons
         for (INT_PTR j = 0; j < expandArray.GetSize(); j++)
            {
            if (expandArray[j] >= 0)
               {
               float popCap = 0;
               pIDULayer->GetData(expandArray[j], this->m_colPopCap, popCap);

               if (popCap <= 0.00000001f)
                  continue;
               // does this expand IDU pass the upzone query?
               //if (pUpzone->m_pQuery != NULL)
               //   {
               //   bool result = false;
               //   bool ok = pUpzone->m_pQuery->Run(expandArray[j], result);
               //   if (!ok || !result)
               //      continue;
               //   }
            
               // query passed, update IDU zone info
               int _idu = expandArray[j];
               //UpdateIDU(pContext, _idu, m_colUgEvent, pRA->m_currentEvent + 1, ADD_DELTA);    // indicate expansion event
               UpdateIDU(pContext, _idu, m_colZone, newZone, ADD_DELTA);

               // update addPop so far
               float area;
               pIDULayer->GetData(_idu, m_colArea, area);
               addedPop += popDelta * area;

               // flag this IDU as having been upzoned
               pRA->m_iduToUpzonedMap[_idu] += 1;

               if (addedPop >= expPop)
                  break;
               }
            }

         // have we achieved the target?
         if (addedPop >= expPop)
            break;
         }  // end of: if ( upzone )

      pRA->m_iduToUpzonedMap[idu] = 0;   // reset map indicate IDU was upzoned this cycle
      }  // end of: for each IDU in this UGA or prioryt expansion area

   CString msg;
   if (addedPop == 0)
      {
      msg.Format("%s Upzone Event Failed (%i)! Trigger: %s, Demand: %.0f people (DF=%.2f), Achieved %.0f people (%.0f percent)",
         (LPCTSTR)pRA->m_name, pContext->currentYear, (LPCTSTR)pUpzone->m_name, expPop, pUpzone->m_demandFrac, addedPop, (addedPop * 100 / expPop));
      Report::LogWarning(msg);
      }
   else
      {
      msg.Format("%s Upzone Event (rural, %i): Trigger: %s,  Demand: %.0f people (DF=%.2f), Achieved %.0f people (%.0f percent)",
            (LPCTSTR)pRA->m_name, pContext->currentYear, (LPCTSTR) pUpzone->m_name, expPop, pUpzone->m_demandFrac, addedPop, (addedPop * 100 / expPop));

      Report::Log(msg);
      //pRA->m_currentEvent++;
      }

   return true;
   }


float RuralDev::UpdateRAStats( EnvContext *pContext, bool outputStartInfo )
   {
   // Update the following fields:
   //
   // pRA->m_currentArea;             // m2
   // pRA->m_currentResArea;          // m2
   // pRA->m_currentCommArea;         // m2
   // pRA->m_currentPopulation;       // people
   // pRA->m_newPopulation;           // people
   // pRA->m_popIncr;                 // people
   // pRA->m_capacity;                // people
   // pRA->m_pctAvailCap;             // fraction
   // pRA->m_avgAllowedDensity;       // du/ac
   // pRA->m_avgActualDensity;        // du/ac
   // pRA->m_impervious;               // area weighted fraction


   MapLayer *pLayer = (MapLayer*) pContext->pMapLayer;
   pContext->pEnvModel->ApplyDeltaArray(pLayer);

   if ( m_pCurrentScenario == NULL )
      return 0;

   INT_PTR raCount = this->m_raArray.GetSize();

   float *curPopArray = new float[ raCount ];

   // reset all 
   for ( INT_PTR i=0; i < raCount; i++ )
      {
      RuralArea *pRA = this->m_raArray[ i ];

      curPopArray[ i ] = pRA->m_currentPopulation;

      pRA->m_currentArea = 0;             // m2
      pRA->m_currentResArea = 0;          // m2
      //pRA->m_currentCommArea = 0;         // m2
      pRA->m_currentPopulation = 0;       // people
      pRA->m_newPopulation = 0;           // people
      pRA->m_popIncr = 0;                 // people
      pRA->m_capacity = 0;                // people
      pRA->m_availCapacity = 0;
      pRA->m_pctAvailCap = 0;             // fraction
      pRA->m_avgAllowedDensity = 0;       // du/ac
      pRA->m_avgActualDensity = 0;        // du/ac
      pRA->m_impervious = 0;               // area weighted fraction
      }

   
   float initPop = 0;

   // iterate through the RuralAreaS layer, computing capacities for each active RuralArea
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      for (int i = 0; i < (int)m_raArray.GetSize(); i++)
         {
         RuralArea* pRA = m_raArray[i];

         bool result = false;
         bool ok = pRA->m_pQuery->Run(idu, result);

         if (result && ok)
            {
            // have UGA, update stats
            float area = 0;
            pLayer->GetData(idu, m_colArea, area);
            pRA->m_currentArea += area;   // m2

            int zone = 0;
            pLayer->GetData(idu, m_colZone, zone);
            if (IsRuralResidential(zone))
               pRA->m_currentResArea += area;   // m2

            float popDens = 0;         // people/m2
            pLayer->GetData(idu, m_colPopDens, popDens);
            float pop = popDens * area;
            pRA->m_currentPopulation += pop;    // people

            float popCap = 0;          // people (not density)
            pLayer->GetData(idu, m_colPopCap, popCap);
            if (popCap > 0)  // ony count if positive 
               pRA->m_capacity += popCap;

            float availCap = 0;          // people (not density)
            pLayer->GetData(idu, m_colAvailCap, availCap);
            if (availCap > 0)  // ony count if positive 
               pRA->m_availCapacity += availCap;

            float popDens0 = 0;         // people/m2
            pLayer->GetData(idu, m_colPopDensInit, popDens0);
            initPop += popDens0 * area;

            pRA->m_newPopulation += ((popDens - popDens0) * area);    // people

            //float allowedDens = GetAllowedDensity(pLayer, idu, zone);  // du/ac
            //float ppdu = this->m_pCurrentScenario->m_ppdu;  //   pRA->m_ppdu;
            //allowedDens *= ppdu / M2_PER_ACRE;

            //pRA->m_avgAllowedDensity += allowedDens * area;   //#

            float impervious = 0;
            if (m_colImpervious >= 0)
               {
               pLayer->GetData(idu, m_colImpervious, impervious);
               pRA->m_impervious += impervious * area;
               }
            }  // end of: if ( results && ok )
         }  // end of: for each RA
      }

   // normalize as needed
   for ( INT_PTR i=0; i < raCount; i++ )
      {
      RuralArea *pRA = this->m_raArray[ i ];

      if (pRA->m_capacity == 0)
         pRA->m_pctAvailCap = 0;
      //else if (pRA->m_currentPopulation > pRA->m_capacity)
      //   pRA->m_pctAvailCap = 0;
      else
         pRA->m_pctAvailCap = pRA->m_availCapacity/pRA->m_capacity;     // fraction

      float peoplePerDU = this->m_pCurrentScenario->m_ppdu; // NOTE: NOT USING field value

      float allowedDens = ( pRA->m_avgAllowedDensity / pRA->m_currentArea ) * ( M2_PER_ACRE / peoplePerDU );
      pRA->m_avgAllowedDensity = allowedDens;     // du/ac

      float actualDens = ( pRA->m_currentPopulation / pRA->m_currentArea ) * ( M2_PER_ACRE / peoplePerDU );
      pRA->m_avgActualDensity = actualDens;        // du/ac

      pRA->m_impervious /= pRA->m_currentArea;

      //if ( outputStartInfo && pContext->yearOfRun == 0 )
         {
         CString msg;
         msg.Format("Population for %s: %i, Total Capacity: %i, Available Capacity: %i, Available Capacity Fraction: %.3f", 
                  (LPCTSTR) pRA->m_name, (int) pRA->m_currentPopulation, (int) pRA->m_capacity, (int) pRA->m_availCapacity, pRA->m_pctAvailCap);
         Report::Log( msg );
         }

      if ( pContext->yearOfRun > 0 )
         pRA->m_popIncr = pRA->m_currentPopulation - curPopArray[ i ];  // calculate population added this step
      
      //if (pRA->m_computeStartPop)
      //   {
      //   pRA->m_startPopulation += pRA->m_currentPopulation;
      //   pRA->m_computeStartPop = false;
      //   }
      }

   delete [] curPopArray;

   return 0;
   }

bool RuralDev::PrioritizeRuralArea( EnvContext *pContext, RuralUpzoneWhen *pUpzone, RuralArea *pRA )
   {
   // basic idea:
   // populates two required columns:
   //   1) m_colUgEvent, which contains the event id indicate the order of IDU was expanded into
   //   2) m_colUgPriority, which contains the priority of expansion for the given IDU
   // It does this by:
   //   For the current scenario, for each UGA,
   //   1) Find all expansion areas IDUs for the UGA.  These are indicated by a negative value in the m_colUga (UGA_ID) column
   //   2) Rank by increasing distance from the existing UGA boundary  (well, really, this just looks up the priority in the IDU)
   //      This becomes the priority list for the scenario, one for commercial, one for residential.  This is sorted arrays.
   //   3) The highest priority IDU idu is tagged with 0, the next with 1, etc... in column U_PRIORITY
   // At this point, the prioritized res/commercial lists can be used to expand UGAs

   if ( m_pCurrentScenario == NULL )
      return false;

   if (pRA->m_use == false)
      return false;

   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   pLayer->SetColData(this->m_colPriority, VData(-1), true);

   pRA->ShuffleIDUs(randUnif);
   pRA->m_priorityListRes.RemoveAll();
   for ( UINT i=0; i < pRA->m_iduArray.size(); i++)
      {
      int idu = pRA->m_iduArray[i];

      // is this IDU in this RA?
      bool result = false;
      bool ok = pRA->m_pQuery->Run(idu, result);

      float iduScore = 0;

      if (result && ok)
         {
         // apply any preference scores
         for (int j = 0; j < (int) pUpzone->m_prefArray.GetSize(); j++)
            {
            RuralPreference* pPref = pUpzone->m_prefArray[j];

            if (pPref->m_pQuery != nullptr)
               {
               pPref->m_pQuery->Run(idu, result);
               if (ok && result)
                  {
                  // preference applies to this IDUs' score, so apply it
                  iduScore += pPref->m_value;
                  }
               }
            }

         // idu preference score calulated for this IDU, this RA.  Store it prior to sorting
         pRA->m_priorityListRes.Add(new RURAL_PRIORITY(idu, iduScore));
         }  // end of: passed query
      }  // end of: MapLayer::Iterator

   // priority lists populated for each RA, now sort to prioritize

   // sort based on priorities
   RURAL_PRIORITY** priority = pRA->m_priorityListRes.GetData();

   qsort((void*)priority, pRA->m_priorityListRes.GetSize(), sizeof(INT_PTR), ComparePriorities);

   for (int k = 0; k < pRA->m_priorityListRes.GetSize(); k++)
      {
      RURAL_PRIORITY* pUD = pRA->m_priorityListRes.GetAt(k);
      pLayer->SetData(pUD->idu, m_colPriority, k);
      }

   CString msg;
   msg.Format( "Finished Prioritizing Rural Areas %s", (LPCTSTR) pRA->m_name);
   Report::Log( msg );

   return true;
   }


bool RuralDev::IsRuralResidential(int zone)
   {
   for (int i = 0; i < (int)m_ruralResZoneArray.GetSize(); i++)
      if (zone == m_ruralResZoneArray[i]->m_id)
         return true;

   return false;
   }


// returns allowed density in people/m2.  Note:  If idu<0, then density is based on passed in zone.
//  otherwise, zone is retrieved from the IDU and the "zone" argument is ignored

float RuralDev::GetAllowedDensity( MapLayer *pLayer, int idu, int zoneID )
   {
   float allowedDens = 0;     // du/acre

   // get zone info
   ZoneInfo *pZone = NULL;

   for ( int i=0; i < (int) m_ruralResZoneArray.GetSize(); i++ )
      {
      if ( m_ruralResZoneArray[ i ]->m_id == zoneID )
         {
         pZone = m_ruralResZoneArray[ i ];
         break;
         }
      }

   //if ( pZone == NULL )
   //   {
   //   for ( int i=0; i < (int) m_commZoneArray.GetSize(); i++ )
   //      {
   //      if ( m_commZoneArray[ i ]->m_id == zoneID )
   //         {
   //         pZone = m_commZoneArray[ i ];
   //         break;
   //         }
   //      }
   //   }

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

float RuralDev::GetImperviousFromZone(int zone)
   {

   for (int i = 0; i < (int)this->m_ruralResZoneArray.GetSize(); i++)
      {
      if (this->m_ruralResZoneArray[i]->m_id == zone)
         return this->m_ruralResZoneArray[i]->m_imperviousFraction;
      }

   //for (int i = 0; i < (int)this->m_commZoneArray.GetSize(); i++)
   //   {
   //   if (this->m_commZoneArray[i]->m_id == zone)
   //      return this->m_commZoneArray[i]->m_imperviousFraction;
   //   }

   return 0;
   }


void RuralDev::UpdateImperiousFromZone( EnvContext *pContext)
   {
   
   if (m_colImpervious < 0)
      return;

   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      int zone = -1;
      pLayer->GetData(idu, m_colZone, zone);

      float imperviousFraction = GetImperviousFromZone(zone);
      UpdateIDU(pContext, idu, m_colImpervious, imperviousFraction, SET_DATA);
      }  // end of: for each IDU
   }


/*
void RuralDev::UpdateRAPops(EnvContext* pContext)
   {
   if (m_colPop < 0)
      return;

   MapLayer* pLayer = (MapLayer*)pContext->pMapLayer;

   bool readOnly = pLayer->m_readOnly;
   pLayer->m_readOnly = false;
   pLayer->SetColData(m_colUgaPop, VData(0.0f), true);

   for (int i = 0; i < m_raArray.GetSize(); i++)
      {
      RuralDev* pRA = m_raArray[i];
      for (int j = 0; j < pRA->m_iduArray.GetSize(); j++)
         {
         int idu = pRA->m_iduArray[j];
         pLayer->SetData(idu, m_colUgaPop, pRA->m_currentPopulation);
         }
      }
   pLayer->m_readOnly= readOnly;
   } */


int ComparePriorities(const void *elem0, const void *elem1 )
   {
   RURAL_PRIORITY *p0 = *((RURAL_PRIORITY**) elem0 );
   RURAL_PRIORITY *p1 = *((RURAL_PRIORITY**) elem1 );

   return ( p0->priority > p1->priority ) ? -1 : 1;
   }



