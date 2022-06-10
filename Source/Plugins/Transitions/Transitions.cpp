// Transitions.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "Transitions.h"

#include <misc.h>
#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>
#include <PathManager.h>
#include <EnvConstants.h>
#include <DeltaArray.h>
#include <QueryEngine.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//using namespace std;

extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new Transitions; }

///////////////////////////////////////////////////////
//                 Transitions
///////////////////////////////////////////////////////



Transitions::Transitions()
   :EnvModelProcess()
   , m_name()
   , m_colArea(-1)
   , m_transColArray()
   , m_pQueryEngine(NULL)
   {}


Transitions::~Transitions()
   {

   }



Transitions& Transitions::operator = (Transitions& t)
   {
   m_name = t.m_name;
   m_colArea = t.m_colArea;
   m_transColArray.Copy(t.m_transColArray);

   CString msg;
   msg.Format("Transitions copy constructor: m_colArea=%i (%i)", m_colArea, t.m_colArea);
   Report::LogWarning(msg);

   return *this;
   }


bool Transitions::Init(EnvContext* pEnvContext, LPCTSTR initStr) 
   {
   MapLayer* pLayer = (MapLayer*) pEnvContext->pMapLayer;

   m_colArea = pLayer->GetFieldCol("AREA");

   if (m_colArea < 0)
      {
      Report::ErrorMsg("Transitions: Can't find area column");
      return false;
      }

   m_pQueryEngine = pEnvContext->pQueryEngine;

   if (LoadXml(pEnvContext,  initStr) == false)
      return false;

   for (int i = 0; i < (int) this->m_transColArray.GetSize(); i++)
      {
      TransCol* pCol = this->m_transColArray[i];

      // get list of unique attribute
      int col = pCol->m_col;
      int nAttr = pLayer->GetUniqueValues(col, pCol->m_attrValues);
      bool bAscending = true;
      std::sort(pCol->m_attrValues.GetData(), pCol->m_attrValues.GetData() + pCol->m_attrValues.GetSize(),
         [bAscending](const VData& left, const VData& right) { return bAscending ? (left < right) : (left > right); } // lambda fn
         );

      pCol->m_pTransData = new FDataObj(nAttr, nAttr+2, 0.0f);  // cols, rows (add a row for initial areas)

      // add col labels for each attr bvalue
      for (int j = 0; j < nAttr; j++)
         {
         CString label;
         pCol->m_attrValues[j].GetAsString(label);

         MAP_FIELD_INFO* pInfo = pLayer->FindFieldInfo(pCol->m_field);
         if (pInfo != NULL)
            {
            FIELD_ATTR* pAttr = pInfo->FindAttribute(pCol->m_attrValues[j]);
            if (pAttr != NULL)
               {
               label += "-";
               label += pAttr->label;
               }
            }

         pCol->m_pTransData->SetLabel(j,label);
         }

      // populate cmap (key=attr value, value = index in attr array and matrix)
      for (int j = 0; j < nAttr; j++)
         {
         VData& key = pCol->m_attrValues[j];
         pCol->transMap[key] = j;
         }
      }

   CString msg;
   msg.Format("Transistions::Init(): m_colArea=%i", m_colArea);
   Report::LogWarning(msg);

   // set initial areas
   int notFoundCount = 0;
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      for (int i = 0; i < (int)this->m_transColArray.GetSize(); i++)
         {
         TransCol* pCol = this->m_transColArray[i];
         
         if (pCol->m_pQuery != NULL)
            {
            bool result = false;
            bool ok = pCol->m_pQuery->Run(idu, result);
            if (!ok || !result)
               continue;
            }
         
         VData v;
         pLayer->GetData(idu, pCol->m_col, v);
         int index = -1;
         bool found = pCol->transMap.Lookup(v, index);

         if (found)
            {
            float area = 0;
            pLayer->GetData(idu, m_colArea, area);
            pCol->m_pTransData->Add(index, 0, area);
            }
         else
            {
            notFoundCount++;
            CString msg;
            msg.Format("Attribute %s not found in %s map.", v.GetAsString(), (LPCTSTR) pCol->m_field);
            Report::LogWarning(msg);
            }
         }
      }

   if (notFoundCount > 0)
      {
      }

   return true; 
   }


bool Transitions::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   { 
   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;

   for (int i = 0; i < (int)this->m_transColArray.GetSize(); i++)
      {
      TransCol* pCol = this->m_transColArray[i];
      int nAttr = (int)pCol->m_attrValues.GetSize();

      // zero out the transdata matrix
      for (int col = 0; col < nAttr; col++)
         for (int row = 1; row < nAttr+1; row++)
            pCol->m_pTransData->Set(col, row, 0.0f);
      }

   return true;  
   }


bool Transitions::Run(EnvContext* pEnvContext)
   { 
   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;

   DeltaArray* deltaArray = pEnvContext->pDeltaArray;// get a ptr to the delta array

   // iterate through deltas added since last “seen”
   INT_PTR size = deltaArray->GetSize();
   float area = 0;

   for (INT_PTR i = pEnvContext->firstUnseenDelta; i < size; ++i)
      {
      if (i < 0)
         break;

      DELTA& delta = ::EnvGetDelta(deltaArray, i);

      for (int j = 0; j < (int)this->m_transColArray.GetSize(); j++)
         {
         TransCol* pCol = this->m_transColArray[j];
         if (delta.col == pCol->m_col)   // does the delta column match this transition mapping?
            {
            if (pCol->m_pQuery != NULL)
               {
               bool result = false;
               bool ok = pCol->m_pQuery->Run(delta.cell, result);
               if (!ok || !result)
                  continue;
               }

            int fromIndex = -1, toIndex = -1;
            bool found = pCol->transMap.Lookup(delta.oldValue, fromIndex);
            ASSERT(found);
            found = pCol->transMap.Lookup(delta.newValue, toIndex);
            ASSERT(found);

            pLayer->GetData(delta.cell, m_colArea, area);
            pCol->m_pTransData->Add(toIndex, fromIndex+1, area);   // col, row
            }
         }  // end of  if ( delta.col = colSource )
      }  // end of: deltaArray loop
   return true; 
   }


bool Transitions::EndRun(EnvContext* pEnvContext)
   {
   MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;

   for (int i = 0; i < (int)this->m_transColArray.GetSize(); i++)
      {
      TransCol* pCol = this->m_transColArray[i];
      int nAttr = (int)pCol->m_attrValues.GetSize();

      // zero out the transdata matrix for final area
      for (int col = 0; col < nAttr; col++)
         pCol->m_pTransData->Set(col, nAttr + 1, 0.0f);   // final areas go in last row
      }

   // update areas by iterating maplayer
   for (MapLayer::Iterator idu = pLayer->Begin(); idu < pLayer->End(); idu++)
      {
      for (int i = 0; i < (int)this->m_transColArray.GetSize(); i++)
         {
         TransCol* pCol = this->m_transColArray[i];

         if (pCol->m_pQuery != NULL)
            {
            bool result = false;
            bool ok = pCol->m_pQuery->Run(idu, result);
            if (!ok || !result)
               continue;
            }

         int nAttr = (int)pCol->m_attrValues.GetSize();

         VData v;
         pLayer->GetData(idu, pCol->m_col, v);
         int index = -1;
         bool found = pCol->transMap.Lookup(v, index);

         if (found)
            {
            float area = 0;
            pLayer->GetData(idu, m_colArea, area);
            pCol->m_pTransData->Add(index, nAttr+1, area);
            }
         else
            {
            CString msg;
            msg.Format("Attribute %s not found in %s map.", v.GetAsString(), (LPCTSTR)pCol->m_field);
            Report::LogWarning(msg);
            }
         }
      }

   // write output
   CString outPath = PathManager::GetPath(PM_OUTPUT_DIR);
   LPCTSTR currentScenario = EnvGetCurrentScenarioName(pEnvContext->pEnvModel);

   for (int i = 0; i < (int)this->m_transColArray.GetSize(); i++)
      {
      TransCol* pCol = this->m_transColArray[i];

      CString path;
      path.Format("%sTransitions_%s_%s.csv", (LPCTSTR) outPath, (LPCTSTR)pCol->m_field, currentScenario);

      CString msg("Writing Transition File at end of run: ");
      msg += path;
      Report::LogInfo(msg);
      pCol->m_pTransData->WriteAscii( + path, ',');
      }
     
   return true;
   }


bool Transitions::LoadXml(EnvContext* pEnvContext, LPCTSTR _filename)
   {
   CString filename;
   if (PathManager::FindPath(_filename, filename) < 0) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      CString msg;
      msg.Format("  Input file '%s' not found - this process will be disabled", _filename);
      Report::ErrorMsg(msg);
      return false;
      }

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // transitions

   TiXmlElement* pXmlTrans = pXmlRoot->FirstChildElement("transition");  // transitions

   while (pXmlTrans != NULL)
      {
      LPTSTR name = NULL, field = NULL, query = NULL;

      XML_ATTR attrs[] = {
         // attr              type        address       isReq checkCol
         { "name",            TYPE_STRING, &name,       true,    0 },
         { "col",             TYPE_STRING, &field,      true,    0 },
         { "query",           TYPE_STRING, &query,      false,   0 },
         { NULL,              TYPE_NULL,    NULL,       false,   0 } };

      ok = TiXmlGetAttributes(pXmlTrans, attrs, filename, NULL);

      if (!ok)
         return false;

      this->m_name = name;

      MapLayer* pLayer = (MapLayer*)pEnvContext->pMapLayer;
      int col = pLayer->GetFieldCol(field);

      if (col < 0)
         {
         CString msg;
         msg.Format("Unable to find field %s when processing transition %s", field, name);
         Report::LogError(msg);
         }
      else
         {
         TransCol* pCol = new TransCol(field, col);
         this->m_transColArray.Add(pCol);

         if (query != NULL)
            {
            pCol->m_queryStr = query;
            pCol->m_pQuery = m_pQueryEngine->ParseQuery(query, 0, name);
            }
         }
      
      pXmlTrans = pXmlTrans->NextSiblingElement("transition");
      }

   return true;
   }

