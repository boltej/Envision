// AcuteHazards.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"

#include "AcuteHazards.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>

#include <direct.h>

#ifdef _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <python.h>
#endif


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern "C" _EXPORT EnvExtension* Factory() { return (EnvExtension*) new AcuteHazards; }

void CaptureException();


/*
Sequence:
EnvModel:Run()
   EnvModel::UpdateAppVars()
   EnvModel::RunAutoPre()  
      AcuteHazards::Run()
         if (postevent) 
            AcuteHazards::Update() -> if event has run, checks for damaged bldgs
                    that are to be repaired according to repair policy (BLDGDAMAGE < 0), and resets
                    them to a repaired state if repairYrs == 0.  For multiyear repairs, reduce repairYrs
                    by one.
         if (eventScheduled) 
            AHEvent::Run() - runs model, writes repairCost, repairYrs, bldgDamage
         
         -> DeltaArray appled here

   EnvModel::RunGobalCOnstraints() -> resets global constraints
   EnvModel::ApplyMandatoryPolicies() -> selects policies within constraints
*/




// C function extending python for capturing stdout from python
// = spam_system in docs
static PyObject *Redirection_stdoutredirect(PyObject *self, PyObject *args)
   {
   const char *str;
   if (!PyArg_ParseTuple(args, "s", &str))
      return NULL;

   Report::Log(str);

   Py_INCREF(Py_None);
   return Py_None;
   }

// create a "Method Table" for Redirection methods
// = SpamMethods in example
static PyMethodDef RedirectionMethods[] = {
    {"stdoutredirect", Redirection_stdoutredirect, METH_VARARGS,
        "stdout redirection helper"},
    {NULL, NULL, 0, NULL}
   };

// define an "Module Definition" structure.
// this attaches the method table to this module
// = spammodule in example
static struct PyModuleDef redirectionmodule =
   {
   PyModuleDef_HEAD_INIT,
   "redirecton", /* name of module */
   "stdout redirection helper", /* module documentation, may be NULL */
   -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
   RedirectionMethods
   };


PyMODINIT_FUNC PyInit_redirection(void)
   {
   return PyModule_Create(&redirectionmodule);
   }



bool AHEvent::Run(EnvContext *pEnvContext)
   {
   // output the current IDU dataset as a DBF. Note that providing a bitArray would reduce size subtantially!
   MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   pIDULayer->SaveDataDB(this->m_envOutputPath, NULL );   // save entire IDU as DBF.  

   char cwd[512];
   _getcwd(cwd, 512);

   _chdir("d:/Envision/studyAreas/OrCoast/Hazus");
      
   CString code;
   code.Format("sys.path.append('%s')", (LPCTSTR)this->m_pyModulePath);
   int retVal = PyRun_SimpleString(code);

   PyObject *pName = PyUnicode_DecodeFSDefault((LPCTSTR)this->m_pyModuleName);   // "e.g. "test1" (no .py)
   PyObject *pModule = NULL;
   
   try
      {
      pModule = PyImport_Import(pName);
      }
   catch (...)
      {
      CaptureException();
      }

   Py_DECREF(pName);
   
   if (pModule == nullptr)
      CaptureException();

   else
      {
      CString msg("Acute Hazards:  Running Python module ");
      msg += this->m_pyModuleName;
      Report::Log(msg);
      
      // pDict is a borrowed reference 
      PyObject *pDict = PyModule_GetDict(pModule);
      //pFunc is also a borrowed reference
      PyObject *pFunc = PyObject_GetAttrString(pModule, (LPCTSTR)this->m_pyFunction);
      if (pFunc && PyCallable_Check(pFunc))
         {
         //PyObject *pArgs = Py_BuildValue("(s)", (LPCTSTR)this->m_hazardScenario);
         //PyObject *pRetVal = PyObject_CallObject(pFunc, pArgs); // pArgs);
         //Py_DECREF(pArgs);

         PyObject* pScenario = PyUnicode_FromString((LPCTSTR)this->m_hazardScenario);
         PyObject* pInDBFPath = PyUnicode_FromString((LPCTSTR)this->m_envOutputPath);
         PyObject* pOutCSVPath = PyUnicode_FromString((LPCTSTR)this->m_envInputPath);

         //PyObject* pRetVal = PyObject_CallFunctionObjArgs(pFunc, pScenario, pInDBFPath, pOutCSVPath, NULL);
         PyObject* pRetVal = PyObject_CallFunctionObjArgs(pFunc, pScenario, NULL);

         if (pRetVal != NULL)
            {
            //printf("Result of call: %ld\n", PyLong_AsLong(pValue));
            Py_DECREF(pRetVal);
            }
         else
            {
            CaptureException();
            }

         Py_DECREF( pScenario   );
         Py_DECREF( pInDBFPath  );
         Py_DECREF( pOutCSVPath );
         }
      else
         {
         PyErr_Print();
         }

      Py_DECREF(pModule);
      }

   // grab input file generated from damage model (csv)
   Report::Log("Acute Hazards: reading building damage parameter file");
   this->m_hazData.ReadAscii(this->m_envInputPath, ',', 0);

   // propagate event outcomes to IDU's
   this->Propagate(pEnvContext);

   this->m_status = AHS_POST_EVENT;

   _getcwd(cwd, 512);

   _chdir(cwd);
   
   return true;
   }

// Propagate is called ONCE when the event first runs.
bool AHEvent::Propagate(EnvContext *pEnvContext)
   {
   MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   int idus = pIDULayer->GetRowCount();
   ASSERT(idus == m_hazData.GetRowCount());

   // pull out needed columns
   PtrArray<HazDataColInfo> colInfos;
   colInfos.Add(new HazDataColInfo{ "P_DS_0", -1 });
   colInfos.Add(new HazDataColInfo{ "rep_time_mu_0", -1 });
   colInfos.Add(new HazDataColInfo{ "rep_time_std_0", -1 });
   colInfos.Add(new HazDataColInfo{ "rep_cost_DR_0",  -1 });
   colInfos.Add(new HazDataColInfo{ "hab_0",  -1 });

   for (int i = 0; i < colInfos.GetSize(); i++)
      {
      HazDataColInfo *pInfo =  colInfos[i];

      pInfo->col = this->m_hazData.GetCol(pInfo->field);
      ASSERT(pInfo->col >= 0);
      }

   // iterate through IDUs' populating data on hazard states as needed
   int currentYear = pEnvContext->currentYear;

   enum HAZARD_METRIC_INDEX { HMI_P_DS0, HMI_REP_TIME_MU0, HMI_REP_TIME_STD0, HMI_REP_COST_DR0, HMI_HAB_0 };
   
   for (int idu = 0; idu < idus; idu++)
      {
      // determine damage state
      float randVal = (float) m_pAHModel->m_randUniform.RandValue();

      float cumPDS = 0;
      int i = 0;
      for (i=0; i < 5; i++)
         {
         float pDS = m_hazData.Get(colInfos[HMI_P_DS0]->col+i, idu);
         cumPDS += pDS;

         if (randVal <= cumPDS)
            break;
         }

      if (i == 5)
         i--;

      // indicate the building is damaged
      int damageIndex = i;  // 0-4
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamage, damageIndex, ADD_DELTA);

      // get time to restore building
      float mean = m_hazData.Get(colInfos[HMI_REP_TIME_MU0]->col+damageIndex, idu);

      if (damageIndex > 0)
         {
         float std = m_hazData.Get(colInfos[HMI_REP_TIME_STD0]->col+damageIndex, idu);
         float timeToRepair = (float)m_pAHModel->m_randLogNormal.RandValue(mean, std);  // TEMPORARY

         int yearsToRepair = int(timeToRepair / 365);
         //yearsToRepair++;    // round up
         // for time to restore, we populate the following fields:
         // IDU field "REPAIR_YRS" (int) - put the year the building will be restored
         // IDU field "BLDG_STATUS" (int) - put the year the building status (0=normal, 1=being restored, 2=uninhabitable)
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduRepairYrs, yearsToRepair, ADD_DELTA);
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgStatus, 1, ADD_DELTA);

         // repair costs
         float repCostFrac = m_hazData.Get(colInfos[HMI_REP_COST_DR0]->col + damageIndex, idu);

         ASSERT(repCostFrac > 0);
         if (repCostFrac > 0)
            {
            ASSERT(repCostFrac < 1);
            // for time to restore, we populate the following fields:
            // IDU field "BD_REPAIR" (float) - cost of any repair ($1000's of $)
            float impValue = 0;
            pIDULayer->GetData(idu, m_pAHModel->m_colIduImprValue, impValue);
            float repairCost = impValue * repCostFrac / 1000000.0f;  // M$, since that's what the budget is
            m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduRepairCost, repairCost, ADD_DELTA);
            } 
         
         // habitable
         randVal = (float) m_pAHModel->m_randUniform.RandValue();
         float pHab = m_hazData.Get(colInfos[HMI_HAB_0]->col + damageIndex, idu);

         int habitable = (randVal >= pHab) ? 0 : 1;    
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduHabitable, habitable, ADD_DELTA);
         }  // end of: if ( damageIndex > 0 )

      }  // end of: for each IDU
   
   return true;
   }


///////////////////////////////////////////////////////
//                 AcuteHazards
///////////////////////////////////////////////////////

AcuteHazards::AcuteHazards(void)
      : m_randLogNormal(1, 1, 0)
      , m_randNormal(1, 1, 0)
      , m_randUniform(0.0, 1.0, 0)
      , m_colIduRepairYrs(-1)
      , m_colIduBldgStatus(-1)
      , m_colIduRepairCost(-1)
      , m_colIduHabitable(-1)
      , m_colIduBldgDamage(-1)
      //, m_annualRepairCosts(0)
      , m_nUninhabitableStructures(0)
      , m_nDamaged(0)
      , m_nDamaged2(0)
      , m_nDamaged3(0)
      , m_nDamaged4(0)
      , m_nDamaged5(0)
      , m_bldgsRepaired(0)
   { }

AcuteHazards::~AcuteHazards(void)
   {
   // Clean up
   Py_Finalize();   // clean up python instance
   }

// override API Methods
bool AcuteHazards::Init(EnvContext *pEnvContext, LPCTSTR initStr)
   {
   bool ok = LoadXml(pEnvContext, initStr);
   if (!ok)
      return FALSE;

   MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   // make sure necesary columns exist
   this->CheckCol(pIDULayer, m_colIduImprValue,  "IMPR_VALUE", TYPE_INT,   CC_MUST_EXIST);
   this->CheckCol(pIDULayer, m_colIduRepairYrs,  "REPAIR_YRS", TYPE_INT,   CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduBldgStatus, "BLDGSTATUS", TYPE_INT,   CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduRepairCost, "REPAIRCOST", TYPE_FLOAT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduHabitable,  "HABITABLE",  TYPE_INT,   CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduBldgDamage, "BLDGDAMAGE", TYPE_INT,   CC_AUTOADD);

   // initialize column info
   pIDULayer->SetColData(m_colIduRepairYrs, VData(-1), true);
   pIDULayer->SetColData(m_colIduBldgStatus, VData(0), true);
   pIDULayer->SetColData(m_colIduRepairCost, VData(0.0f), true);
   pIDULayer->SetColData(m_colIduHabitable, VData(1), true);
   pIDULayer->SetColData(m_colIduBldgDamage, VData(0), true);
     
   // add input variables
   for (int i = 0; i < m_events.GetSize(); i++)
      {
      AHEvent *pEvent = m_events[i];
      CString label = pEvent->m_name + "_use";
      this->AddInputVar((LPCTSTR)label, pEvent->m_use, "");
      }

   // add output variables
   //this->AddOutputVar("Annual Repair Expenditures", m_annualRepairCosts, "");
   this->AddOutputVar("Uninhabitable Structures", m_nUninhabitableStructures, "");
   this->AddOutputVar("Bldgs with Slight Damage", m_nDamaged2, "");
   this->AddOutputVar("Bldgs with Moderate Damage", m_nDamaged3, "");
   this->AddOutputVar("Bldgs with Extensive Damage", m_nDamaged4, "");
   this->AddOutputVar("Bldgs with Complete Damage", m_nDamaged5, "");
   this->AddOutputVar("Total No. Bldgs Damaged", m_nDamaged, "");
   this->AddOutputVar("Bldgs Repaired", m_bldgsRepaired, "");
   this->AddOutputVar("Bldgs Being Repaired", m_bldgsBeingRepaired, "");

   InitPython();

   return TRUE;
   }

bool AcuteHazards::InitRun(EnvContext *pEnvContext, bool useInitialSeed)
   {
   // reset annual output variables
   //m_annualRepairCosts = 0;
   m_nUninhabitableStructures = 0;
   m_nDamaged2 = 0;
   m_nDamaged3 = 0;
   m_nDamaged4 = 0;
   m_nDamaged5 = 0;
   m_bldgsRepaired = 0;
   m_bldgsBeingRepaired = 0;

   return TRUE;
   }

bool AcuteHazards::Run(EnvContext *pEnvContext)
   {
   int currentYear = pEnvContext->currentYear;
   
   // reset annual output variables
   //m_annualRepairCosts = 0;
   m_nUninhabitableStructures = 0;
   m_nDamaged  = 0;
   m_nDamaged2 = 0;
   m_nDamaged3 = 0;
   m_nDamaged4 = 0;
   m_nDamaged5 = 0;
   m_bldgsRepaired = 0;
   m_bldgsBeingRepaired = 0;

   // update results from any prior events if needed
   for (int i = 0; i < m_events.GetSize(); i++)
      {
      if (m_events[i]->m_use && m_events[i]->m_status == AHS_POST_EVENT)
         {
         Update(pEnvContext);
         break;
         }
      }   
   
   // run any events that are scheduled for this year
   for (int i = 0; i < m_events.GetSize(); i++)
      {
      if (m_events[i]->m_use && m_events[i]->m_year == currentYear)
         m_events[i]->Run(pEnvContext);
      }
     
   return TRUE;
   }


bool AcuteHazards::InitPython()
   {
   Report::Log("Acute Hazards:  Initializing embedded Python interpreter...");

   char cwd[512];
   _getcwd(cwd, 512);

   _chdir("d:/Envision/studyAreas/OrCoast/Hazus");

   // launch a python instance and run the model
   Py_SetProgramName(L"Envision");  // argv[0]

   wchar_t path[512];

   int cx = swprintf(path, 512, L"%hs/DLLs;%hs/Lib;%hs/Lib/site-packages",
      (LPCTSTR) m_pythonPath, (LPCTSTR) m_pythonPath, (LPCTSTR) m_pythonPath);

   Py_SetPath(path);

   PyImport_AppendInittab("redirection", PyInit_redirection);

   Py_Initialize();

   //CString code;
   //code.Format("sys.path.append('%s')", (LPCTSTR)this->m_pyModulePath);
   int retVal = PyRun_SimpleString("import sys");
   //retVal = PyRun_SimpleString(code);

   // add Python function for redirecting output on the Python side
   retVal = PyRun_SimpleString("\
import redirection\n\
import sys\n\
class StdoutCatcher:\n\
    def write(self, stuff):\n\
        redirection.stdoutredirect(stuff)\n\
sys.stdout = StdoutCatcher()");
   /////////////

   Report::Log("Acute Hazards: Python initialization complete...");

   return true;
   }



// Update is called during Run() processing.
// For any events that are in AHS_POST_EVENT status,
// do any needed update of IDUs recovering from that event 
// Notes: 
//  1) Any building tagged for repair will have "BLDG_DAMAGE" set to a negative value

bool AcuteHazards::Update(EnvContext *pEnvContext)
   {
   // basic idea is to loop through IDUs', applying any post-event
   // changes to the landscape, e.g. habitability, recovery period, etc.
   MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

   //int idus = this->m_hazData.GetRowCount();
   int idus = pIDULayer->GetRowCount();

   // loop through IDU's, interpreting the probability information
   // from the damage model into the IDUs

   for (int idu = 0; idu < idus; idu++)
      {
      int damage = 0;
      pIDULayer->GetData(idu, m_colIduBldgDamage, damage);
      
      switch (damage)
         {
         case 1:  m_nDamaged2++; break;
         case 2:  m_nDamaged3++; break;
         case 3:  m_nDamaged4++; break;
         case 4:  m_nDamaged5++; break;
         }

      // if the damage is negative, we are actively repairing it,
      // so indicate that it has been repaired
      
      if (damage < 0)
         {
         //float repairCost = 0;
         //pIDULayer->GetData(idu, m_colIduRepairCost, repairCost);

         int repairYears = 0;
         pIDULayer->GetData(idu, m_colIduRepairYrs, repairYears);

         if (repairYears > 0)
            {
            m_bldgsBeingRepaired++;
            UpdateIDU(pEnvContext, idu, m_colIduRepairYrs, repairYears - 1, ADD_DELTA);
            }
         else  // building is repaired, so indicate that in the IDUs
            {
            m_bldgsRepaired++;
            UpdateIDU(pEnvContext, idu, m_colIduRepairYrs, -1,    ADD_DELTA);
            UpdateIDU(pEnvContext, idu, m_colIduRepairCost, 0.0f, ADD_DELTA);
            UpdateIDU(pEnvContext, idu, m_colIduBldgStatus, 0,    ADD_DELTA);
            UpdateIDU(pEnvContext, idu, m_colIduHabitable,  1,    ADD_DELTA);
            UpdateIDU(pEnvContext, idu, m_colIduBldgDamage, 0,    ADD_DELTA);
            }
         }

      int habitable = 0;
      pIDULayer->GetData(idu, m_colIduHabitable, habitable);
      if (habitable == 0)
         m_nUninhabitableStructures++;
      }  // end of: for each idu

   m_nDamaged = m_nDamaged2 + m_nDamaged3 + m_nDamaged4 + m_nDamaged5;

   return true;
   }


bool AcuteHazards::LoadXml(EnvContext *pEnvContext, LPCTSTR filename)
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
   TiXmlElement *pXmlRoot = doc.RootElement();  // <acute_hazards>
   CString codePath;

   XML_ATTR attrs[] = {
      // attr            type           address            isReq checkCol
      { "python_path",  TYPE_CSTRING,   &m_pythonPath,     true,  0 },
      { NULL,           TYPE_NULL,     NULL,               false, 0 } };
   ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, NULL);

   TiXmlElement *pXmlEvent = pXmlRoot->FirstChildElement("event");
   if (pXmlEvent == NULL)
      {
      CString msg("Acute Hazards: no <event>'s defined");
      msg += filename;
      Report::ErrorMsg(msg);
      return false;
      }

   while (pXmlEvent != NULL)
      {
      AHEvent *pEvent = new AHEvent;

      CString pyModulePath;

      XML_ATTR attrs[] = {
         // attr              type           address            isReq checkCol
         { "year",            TYPE_INT,      &pEvent->m_year,           true,  0 },
         { "name",            TYPE_CSTRING,  &pEvent->m_name,           true,  0 },
         { "use",             TYPE_INT,      &pEvent->m_use,            true,  0 },
         { "py_function",     TYPE_CSTRING,  &pEvent->m_pyFunction,     true,  0 },
         { "py_module",       TYPE_CSTRING,  &pyModulePath,             true,  0 },
         { "env_output",      TYPE_CSTRING,  &pEvent->m_envOutputPath,  true,  0 },
         { "env_input",       TYPE_CSTRING,  &pEvent->m_envInputPath,   true,  0 },
         { "hazard_scenario", TYPE_CSTRING,  &pEvent->m_hazardScenario, true,  0 },
         { NULL,              TYPE_NULL,     NULL,        false, 0 } };

      ok = TiXmlGetAttributes(pXmlEvent, attrs, filename, NULL);

      if (!ok)
         {
         delete pEvent;
         CString msg;
         msg = "Acute Hazards Model:  Error reading <event> tag in input file ";
         msg += filename;
         Report::ErrorMsg(msg);
         return false;
         }
      else
         {
         nsPath::CPath path(pyModulePath);
         pEvent->m_pyModuleName = path.GetTitle();
         pEvent->m_pyModulePath = path.GetPath();
         pEvent->m_pAHModel = this;

         m_events.Add(pEvent);
         }

      pXmlEvent = pXmlEvent->NextSiblingElement("event");
      }

   return true;
   }


void CaptureException()
   {
   PyObject *exc_type = NULL, *exc_value = NULL, *exc_tb = NULL;
   PyErr_Fetch(&exc_type, &exc_value, &exc_tb);
   PyObject* str_exc_type = PyObject_Repr(exc_type); //Now a unicode object
   PyObject* pyStr = PyUnicode_AsEncodedString(str_exc_type, "utf-8", "Error ~");
   const char *msg = PyBytes_AS_STRING(pyStr);

   Report::LogError(msg);
   
   Py_XDECREF(str_exc_type);
   Py_XDECREF(pyStr);

   Py_XDECREF(exc_type);
   Py_XDECREF(exc_value);
   Py_XDECREF(exc_tb);
   }
