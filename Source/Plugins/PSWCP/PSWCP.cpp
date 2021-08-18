// PSWCP.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "PSWCP.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new PSWCP; }

///////////////////////////////////////////////////////
//                 PSWCP
///////////////////////////////////////////////////////

PSWCP::PSWCP(void)
   : EnvModelProcess()
   , m_colS(-1)
   , m_colS_Q(-1)
   , m_colS_Code(-1)
   , m_colP(-1)
   , m_colP_Q(-1)
   , m_colP_Code(-1)
   , m_colM(-1)
   , m_colM_Q(-1)
   , m_colM_Code(-1)
   , m_colN(-1)
   , m_colN_Q(-1)
   , m_colN_Code(-1)
   , m_colPa(-1)
   , m_colPa_Q(-1)
   , m_colPa_Code(-1)
   , m_pIDULayer(NULL)
   , m_pWQ_RP(NULL)
   { }

PSWCP::~PSWCP(void)
   {
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
   //MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   //int idus = pIDULayer->GetRowCount();
   //for (int idu = 0; idu < idus; idu++)
   //   {
   //   float imprValue = 0;
   //   pIDULayer->GetData(idu, m_colIduImprValue, imprValue);
   //
   //   }

   return true;
   }

bool PSWCP::Run(EnvContext* pEnvContext)
   {
   int currentYear = pEnvContext->currentYear;

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
   // calculated Importance (M1) scores for Sediment/Phos/Metals/Nit/Pathogens
   this->CheckCol(m_pIDULayer, m_colS, "S_M1_CAL", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colP, "P_M1_CAL", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colM, "M_M1_CAL", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colN, "N_M1_CAL", TYPE_FLOAT, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colPa, "PA_M1_CAL", TYPE_FLOAT, CC_MUST_EXIST);

   // quartile ranking for Importance (M1)
   this->CheckCol(m_pIDULayer, m_colS_Q, "S_Q", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colP_Q, "P_Q", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colM_Q, "ME_Q", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colN_Q, "N_Q", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colPa_Q, "PA_Q", TYPE_STRING, CC_MUST_EXIST);

   // priority code for Importance (M1)
   this->CheckCol(m_pIDULayer, m_colS_Code, "SED_RP", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colP_Code, "P_RP", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colM_Code, "ME_RP", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colN_Code, "N_RP", TYPE_STRING, CC_MUST_EXIST);
   this->CheckCol(m_pIDULayer, m_colPa_Code, "PA_RP", TYPE_STRING, CC_MUST_EXIST);

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
   for (MapLayer::Iterator idu = m_pWQ_RP->Begin(); idu < m_pWQ_RP->End(); idu++)
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
