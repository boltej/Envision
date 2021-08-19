// PSWCP.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "PSWCP.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>
#include <PathManager.h>
#include <EnvConstants.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new PSWCP; }

///////////////////////////////////////////////////////
//                 PSWCP
///////////////////////////////////////////////////////

PSWCP::PSWCP(void)
   : EnvModelProcess()
   , m_col_IDU_AUIndex(-1)
   , m_col_IDU_WqS_rp(-1)
   , m_col_IDU_WqP_rp(-1)
   , m_col_IDU_WqMe_rp(-1)
   , m_col_IDU_WqN_rp(-1)
   , m_col_IDU_WqPa_rp(-1)
   , m_col_WQ_AU(-1)
   , m_col_WQ_S(-1)
   , m_col_WQ_S_Q(-1)
   , m_col_WQ_S_rp(-1)
   , m_col_WQ_P(-1)
   , m_col_WQ_P_Q(-1)
   , m_col_WQ_P_rp(-1)
   , m_col_WQ_Me(-1)
   , m_col_WQ_Me_Q(-1)
   , m_col_WQ_Me_rp(-1)
   , m_col_WQ_N(-1)
   , m_col_WQ_N_Q(-1)
   , m_col_WQ_N_rp(-1)
   , m_col_WQ_Pa(-1)
   , m_col_WQ_Pa_Q(-1)
   , m_col_WQ_Pa_rp(-1)
   , m_pIDULayer(NULL)
   , m_pWqRpTable(NULL)
   , m_pWfRpTable(NULL)
   , m_index_IDU()
   { }

PSWCP::~PSWCP(void)
   {
   if (m_pWqRpTable != NULL)
      delete m_pWqRpTable;
   if (m_pWfRpTable != NULL)
      delete m_pWfRpTable;
   }

// override API Methods
bool PSWCP::Init(EnvContext* pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);
   if (!ok)
      return FALSE;

   m_pIDULayer = (MapLayer*)pEnvContext->pMapLayer;


   // water quality model
   InitWQAssessment(pEnvContext);

   // water flow model
   InitWFAssessment(pEnvContext);

   // habitat
   InitHabAssessment(pEnvContext);

   // HCI
   InitHCIAssessment(pEnvContext);


   return true;
   }

bool PSWCP::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
   {
   // reset annual output variables

   return true;
   }

bool PSWCP::Run(EnvContext* pEnvContext)
   {
   int currentYear = pEnvContext->currentYear;

   RunWQAssessment(pEnvContext);

   // water flow model
   RunWFAssessment(pEnvContext);

   // habitat
   RunHabAssessment(pEnvContext);

   // HCI
   RunHCIAssessment(pEnvContext);



   return true;
   }



bool PSWCP::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
   {
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

   //XML_ATTR attrs[] = {
   //   // attr            type           address            isReq checkCol
   //   { "python_path",  TYPE_CSTRING,   &m_pythonPath,     true,  0 },
   //   { NULL,           TYPE_NULL,     NULL,               false, 0 } };
   //ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, NULL);
   //
   //TiXmlElement* pXmlEvent = pXmlRoot->FirstChildElement("event");
   //if (pXmlEvent == NULL)
   //   {
   //   CString msg("Acute Hazards: no <event>'s defined");
   //   msg += filename;
   //   Report::ErrorMsg(msg);
   //   return false;
   //   }
   //
   //while (pXmlEvent != NULL)
   //   {
   //   AHEvent* pEvent = new AHEvent;
   //
   //   CString pyModulePath;
   //
   //   XML_ATTR attrs[] = {
   //      // attr                  type           address            isReq checkCol
   //      { "year",                TYPE_INT,      &pEvent->m_year,           true,  0 },
   //      { "name",                TYPE_CSTRING,  &pEvent->m_name,           true,  0 },
   //      { "use",                 TYPE_INT,      &pEvent->m_use,            true,  0 },
   //      { "py_function",         TYPE_CSTRING,  &pEvent->m_pyFunction,     true,  0 },
   //      { "py_module",           TYPE_CSTRING,  &pyModulePath,             true,  0 },
   //      { "env_output",          TYPE_CSTRING,  &pEvent->m_envOutputPath,  true,  0 },
   //      { "earthquake_input",    TYPE_CSTRING,  &pEvent->m_earthquakeInputPath,   true,  0 },
   //      { "tsunami_input",       TYPE_CSTRING,  &pEvent->m_tsunamiInputPath,   true,  0 },
   //      { "earthquake_scenario", TYPE_CSTRING,  &pEvent->m_earthquakeScenario, true,  0 },
   //      { "tsunami_scenario",    TYPE_CSTRING,  &pEvent->m_tsunamiScenario, true,  0 },
   //      { NULL,                  TYPE_NULL,     NULL,        false, 0 } };
   //
   //   ok = TiXmlGetAttributes(pXmlEvent, attrs, filename, NULL);
   //
   //   if (!ok)
   //      {
   //      delete pEvent;
   //      CString msg;
   //      msg = "Acute Hazards Model:  Error reading <event> tag in input file ";
   //      msg += filename;
   //      Report::ErrorMsg(msg);
   //      return false;
   //      }
   //   else
   //      {
   //      nsPath::CPath path(pyModulePath);
   //      pEvent->m_pyModuleName = path.GetTitle();
   //      pEvent->m_pyModulePath = path.GetPath();
   //      pEvent->m_pAHModel = this;
   //
   //      m_events.Add(pEvent);
   //      }
   //
   //   pXmlEvent = pXmlEvent->NextSiblingElement("event");
   //   }

   return true;
   }



bool PSWCP::InitWQAssessment(EnvContext* pEnvContext)
   {
   this->CheckCol(m_pIDULayer, m_col_IDU_AUIndex, "AU_INDEX", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqS_rp,  "WqS_rp", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqP_rp,  "WqP_rp", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqMe_rp, "WqMe_rp", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqN_rp,  "WqN_rp", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_col_IDU_WqPa_rp, "WqPa_rp", TYPE_STRING, CC_MUST_EXIST);

   // load database
   m_pWqRpTable = new VDataObj(UNIT_MEASURE::U_UNDEFINED);
   CString path;
   if (PathManager::FindPath("PSWCP/WQ_RP.csv", path) < 0) //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
      {
      Report::ErrorMsg("PSWCP: Input file WQ_RP.csv not found - this process will be disabled");
      return false;
      }

   int rows = m_pWqRpTable->ReadAscii(path, ',');
   CString msg;
   msg.Format("PSWCP: Loaded %i records from %s", rows, (LPCTSTR)path);
   Report::LogInfo(msg);

   // set col values for useful columns
   this->m_col_WQ_AU    = m_pWqRpTable->GetCol("AU_ID");
   this->m_col_WQ_S_rp  = m_pWqRpTable->GetCol("SED_RP");
   this->m_col_WQ_P_rp  = m_pWqRpTable->GetCol("P_RP");
   this->m_col_WQ_Me_rp = m_pWqRpTable->GetCol("ME_RP");
   this->m_col_WQ_N_rp  = m_pWqRpTable->GetCol("N_RP");
   this->m_col_WQ_Pa_rp = m_pWqRpTable->GetCol("Pa_RP");

   // load/create index
   CString indexPath;
   if (PathManager::FindPath("PSWCP/WQ_RP.idx", indexPath) < 0)
      {
      // doesn't exist, build (and save) it
      Report::LogInfo("PSWCP: Building index WQ_RP.idx");
      m_index_IDU.BuildIndex(m_pIDULayer->m_pDbTable, m_col_IDU_AUIndex);

      indexPath = PathManager::GetPath(PM_PROJECT_DIR);
      indexPath += "PSWCP/WQ_RP.idx";
      m_index_IDU.WriteIndex(indexPath);
      }
   else 
      {
      m_index_IDU.ReadIndex(m_pIDULayer->m_pDbTable, indexPath);
      }

   // write WQ_RP values to IDUs by iterating through the WQ-RP table, writing
   // values to the IDU's using the index for 
   CUIntArray recordArray;
   for (int row = 0; row < m_pWqRpTable->GetRowCount(); row++)
      {
      // get the AU_ID for this record
      int auID = m_pWqRpTable->GetAsInt(m_col_WQ_AU, row);
      int count = m_index_IDU.GetRecordArray(m_col_IDU_AUIndex, VData(auID), recordArray);

      CString s_rp, p_rp, me_rp, n_rp, pa_rp;
      m_pWqRpTable->Get(m_col_WQ_S_rp, row, s_rp);
      m_pWqRpTable->Get(m_col_WQ_P_rp, row, p_rp);
      m_pWqRpTable->Get(m_col_WQ_Me_rp, row, me_rp);
      m_pWqRpTable->Get(m_col_WQ_N_rp, row, n_rp);
      m_pWqRpTable->Get(m_col_WQ_Pa_rp, row, pa_rp);

      for (int j = 0; j < count; j++)
         {
         int idu = recordArray[j];

         m_pIDULayer->SetData(idu, m_col_IDU_WqS_rp, s_rp);
         m_pIDULayer->SetData(idu, m_col_IDU_WqP_rp, p_rp);
         m_pIDULayer->SetData(idu, m_col_IDU_WqMe_rp, me_rp);
         m_pIDULayer->SetData(idu, m_col_IDU_WqN_rp, n_rp);
         m_pIDULayer->SetData(idu, m_col_IDU_WqPa_rp, pa_rp);
         }
      }  // end of: for each row in WQ_RP table


   // populate IDU columns
   //this->CheckCol(m_pIDULayer, m_col_WQ_AU, "S_M1_CAL", TYPE_FLOAT, CC_MUST_EXIST);


   //// initialize column info
   //pIDULayer->SetColData(m_colIduRepairYrs, VData(-1), true);
   //pIDULayer->SetColData(m_colIduBldgStatus, VData(0), true);

   //// add output variables
   ////this->AddOutputVar("Annual Repair Expenditures", m_annualRepairCosts, "");
   //this->AddOutputVar("Habitable Structures (count)", m_nInhabitableStructures, "");
   //this->AddOutputVar("Habitable Structures (pct)", m_pctInhabitableStructures, "");

   return true;
   }



bool PSWCP::InitWFAssessment(EnvContext* pEnvContext)
   {
   return true;
   }

bool PSWCP::InitHabAssessment(EnvContext* pEnvContext)
   {
   return true;
   }

bool PSWCP::InitHCIAssessment(EnvContext* pEnvContext)
   {
   return true;
   }





bool PSWCP::RunWQAssessment(EnvContext* pEnvContext)
   {
   for (MapLayer::Iterator idu = m_pIDULayer->Begin(); idu < m_pIDULayer->End(); idu++)
      {
      float imprValue = 0;
   //   pIDULayer->GetData(idu, m_colIduImprValue, imprValue);
   
      }

   return true;
   }


bool PSWCP::RunWFAssessment(EnvContext* pEnvContext)
   {
   return true;
   }

bool PSWCP::RunHabAssessment(EnvContext* pEnvContext)
   {
   return true;
   }

bool PSWCP::RunHCIAssessment(EnvContext* pEnvContext)
   {
   return true;
   }
