// SyncBuildings.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "SyncBuildings.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>
#include <PathManager.h>
#include <Scenario.h>
#include <EnvConstants.h>

#include <iostream>
#include <filesystem>

#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new SyncBuildings; }


///////////////////////////////////////////////////////
//                 SyncBuildings
///////////////////////////////////////////////////////

SyncBuildings::SyncBuildings(void)
   //: m_randLogNormal(1, 1, 0)
   //, m_randNormal(1, 1, 0)
   //, m_randUniform(0.0, 1.0, 0)
   { }

SyncBuildings::~SyncBuildings(void)
   {
   }

// override API Methods
bool SyncBuildings::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);

   if (!ok)
      {
      CString msg;
      msg.Format(_T("SyncBuildings: Failed to load XML from %s"), initStr);
      Report::ErrorMsg(msg);
      return false;
      }

   this->AddInputVar("use", this->m_use, "");
     
   return ok;
   }

bool SyncBuildings::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   if (this->m_use == false)
      {
      Report::LogInfo("SyncBuildings is not active for this run");
      return true; // nothing to do
      }

   // reset annual output variables
   //MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   return true;
   }

bool SyncBuildings::Run(EnvContext* pEnvContext)
   {
   if (this->m_use == false)
      {
      Report::LogInfo("SyncBuildings is not active for this run");
      return true; // nothing to do
      }

   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   pIDULayer->m_pDbTable->SaveDataDBF((LPCTSTR)m_iduFile);

   // Run external process synchronously (cross-platform)
   int exitCode = std::system(m_processPath);
   // To suppress the command window when running a process in C++ on Windows,replace your std::system(m_processPath); line 
   // with a CreateProcess call using the CREATE_NO_WINDOW flag

   //if (exitCode != 0)
       //{
       //CString msg;
       //msg.Format(_T("Failed to launch process: %s (exit code %d)"), m_processPath, exitCode);
       //Report::ErrorMsg(msg);
       //Report::LogError(msg);
       //return false;
       //}

   VDataObj resultsData;
   int rows = resultsData.ReadAscii(m_resultsFile);
   if (rows <= 0)
      {
      CString msg;
      msg.Format(_T("Failed to read results file: %s"), m_resultsFile);
      Report::ErrorMsg(msg);
      Report::LogError(msg);
      return false;
      }

   for (int i = 0; i < m_colMaps.size(); i++)
      {
      ColMap& map = m_colMaps[i];
      // check if results field exists in results data
      map.resultCol = resultsData.GetCol((LPCTSTR)map.resultsField);
      if (map.resultCol < 0)
         {
         CString msg;
         msg.Format("SyncBuildings: Results column [%s] not found in results data", (LPCTSTR)map.resultsField);
         Report::ErrorMsg(msg);
         Report::LogError(msg);
         return false;
         }
      }

   VData data;
   for (int i = 0; i < pIDULayer->GetRowCount(); i++)
      {
      // get data from CSV and write to IDUs
      for (auto map : m_colMaps)
         {
         resultsData.Get(map.resultCol, i, data);
         UpdateIDU(pEnvContext, i, map.iduCol, data, ADD_DELTA);
         }
      }

   return true;
   }

bool SyncBuildings::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
   {
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   // have xml string, start parsing
   TiXmlDocument doc;
   bool ok = doc.LoadFile(filename);

   if (!ok)
      {
      Report::ErrorMsg(doc.ErrorDesc());
      return false;
      }

   // start interating through the nodes
   TiXmlElement* pXmlRoot = doc.RootElement();  // <acute_hazards>
   CString codePath;

   XML_ATTR attrs[] = {
      { "process_path",  TYPE_CSTRING,  &m_processPath,     true,  0 },
      { "idu_file",      TYPE_CSTRING,  &m_iduFile,        true,  0 },
      { "results_file",  TYPE_CSTRING,  &m_resultsFile,    true,  0 },
      { NULL,           TYPE_NULL,     NULL,               false, 0 } };

   ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, NULL);

   if (!ok)
      {
      Report::ErrorMsg(_T("Error reading attributes from flood_damage root element"));
      return false;
      }

   // get any sync_maps
   TiXmlElement* pXmlMappings = pXmlRoot->FirstChildElement(_T("column_mappings"));
   TiXmlElement* pXmlMap = pXmlMappings->FirstChildElement(_T("map"));
   int count = 0;
   while (pXmlMap != NULL)
      {
      ColMap map;

      XML_ATTR mapAttrs[] =
         { // attr          type           address                   isReq   checkCol
               { "idu_col",       TYPE_CSTRING, &(map.iduField),    true,  CC_AUTOADD },
               { "results_col",   TYPE_CSTRING, &(map.resultsField), true, 0 },
               { NULL, TYPE_NULL, NULL, false, 0 } };

      if (TiXmlGetAttributes(pXmlMap, mapAttrs, filename, pIDULayer) == false)
         return false;

      map.iduCol = pIDULayer->GetFieldCol(map.iduField);

      if (map.iduCol < 0)
         {
         CString msg;
         msg.Format("SyncBuildings: IDU column [%s] not found when reading <map> element", (LPCTSTR)map.iduField);
         Report::ErrorMsg(msg);
         return false;
         }


      // map.resultCol = pIDULayer->GetFieldCol(map.resultsField);

       //if (map.resultCol < 0)
       //{
           //CString msg;
           //msg.Format("SyncBuildings: Results column [%s] not found when reading <map> element", (LPCTSTR)map.resultsField);
           //Report::ErrorMsg(msg);
           //return false;
       //}

      this->m_colMaps.push_back(map);
      pXmlMap = pXmlMap->NextSiblingElement(_T("map"));
      }

   return true;
   }
