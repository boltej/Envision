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


extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new AcuteHazards; }

void CaptureException();





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
   // basic idea: run an earthquake and tsunami model for this event.
   // Steps include:
   //    1) Save the current IDU database.
   //    2) Append path to Hazus model python model code to system
   //    3) Load a python module with SimpleString 
   //    4) call a function in the python code that takes the following arguments:
   //       a) string defining earthquake scenario
   //       b) string defining tsunami scenario
   //   5) clean up python references

   // save the data
   MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   pIDULayer->SaveDataDB(this->m_envOutputPath, NULL );   // save entire IDU as DBF.  

   char cwd[512];
   _getcwd(cwd, 512);

   _chdir("/Envision/StudyAreas/OrCoast/Hazus");
     
   // add hazus path to system path
   CString code;
   code.Format("sys.path.append('%s')", (LPCTSTR)this->m_pyModulePath);
   int retVal = PyRun_SimpleString(code);

   // load model python code 
   PyObject *pName = PyUnicode_DecodeFSDefault((LPCTSTR)this->m_pyModuleName);   // "e.g. "test1" (no .py)
   PyObject *pModule = NULL;
   
   try
      {
      pModule = PyImport_Import(pName);
      }
   catch (...)
      {
      CString msg("Acute Hazards: Unable to load Python module ");
      msg += this->m_pyModulePath;
      msg += ":";
      msg += this->m_pyModuleName;
      Report::LogWarning(msg);
      CaptureException();
      }

   Py_DECREF(pName);
   
   if (pModule == nullptr)
      CaptureException();

   else
      {
      // if successful loading code, run the model
      CString msg("Acute Hazards:  Running Python module ");
      msg += this->m_pyModuleName;
      Report::Log(msg);
      
      // pDict is a borrowed reference 
      PyObject *pDict = PyModule_GetDict(pModule);
      //pFunc is also a borrowed reference
      PyObject *pFunc = PyObject_GetAttrString(pModule, (LPCTSTR)this->m_pyFunction);
      if (pFunc && PyCallable_Check(pFunc))
         {
         PyObject* pEqScenario    = PyUnicode_FromString((LPCTSTR)this->m_earthquakeScenario);
         PyObject* pTsuScenario   = PyUnicode_FromString((LPCTSTR)this->m_tsunamiScenario);
         PyObject* pInDBFPath     = PyUnicode_FromString((LPCTSTR)this->m_envOutputPath);
         PyObject* pOutEqCSVPath  = PyUnicode_FromString((LPCTSTR)this->m_earthquakeInputPath);
         PyObject* pOutTsuCSVPath = PyUnicode_FromString((LPCTSTR)this->m_tsunamiInputPath);

         // call the python entry point
         PyObject* pRetVal = PyObject_CallFunctionObjArgs(pFunc, pEqScenario, pTsuScenario, NULL);

         if (pRetVal != NULL)
            {
            CString msg("Acute Hazards: Python module ");
            msg += this->m_pyModulePath;
            msg += ":";
            msg += this->m_pyModuleName;
            msg += " executed successfully";
            Report::LogInfo(msg);

            Py_DECREF(pRetVal);
            }
         else
            {
            CString msg("Acute Hazards: Unable to execute Python module ");
            msg += this->m_pyModulePath;
            msg += this->m_pyModuleName;
            Report::LogWarning(msg);

            CaptureException();
            }

         Py_DECREF(pEqScenario);
         Py_DECREF(pTsuScenario);
         Py_DECREF(pInDBFPath  );
         Py_DECREF(pOutEqCSVPath );
         Py_DECREF(pOutTsuCSVPath);
         }
      else
         {
         PyErr_Print();
         }

      Py_DECREF(pModule);
      }

   // propagate event outcomes to IDU's
   this->Propagate(pEnvContext);

   this->m_status = AHS_POST_EVENT;

   _getcwd(cwd, 512);

   _chdir(cwd);
   
   return true;
   }

// Propagate() is called ONCE when the event first runs.
// It uses the Hazus Data returns from the model run and
// and translates it into the IDUs in Envision
bool AHEvent::Propagate(EnvContext *pEnvContext)
   {
   //---------------------
   // Building Damage
   //---------------------
   // grab input file generated from damage model (csv)
   CString msg("Acute Hazards: reading building damage parameter file ");
   msg += this->m_earthquakeInputPath;
   Report::Log(msg);
   this->m_earthquakeData.ReadAscii(this->m_earthquakeInputPath, ',', 0);

   // pull out needed columns
   PtrArray<HazDataColInfo> eqColInfos;
   eqColInfos.Add(new HazDataColInfo{ "DS", -1 });
   eqColInfos.Add(new HazDataColInfo{ "rep_time", -1 });
   eqColInfos.Add(new HazDataColInfo{ "rep_cost",  -1 });
   eqColInfos.Add(new HazDataColInfo{ "habitable",  -1 });
   eqColInfos.Add(new HazDataColInfo{ "CasSev1",  -1 });  // casuality severitys
   eqColInfos.Add(new HazDataColInfo{ "CasSev2",  -1 });
   eqColInfos.Add(new HazDataColInfo{ "CasSev3",  -1 });
   eqColInfos.Add(new HazDataColInfo{ "CasSev4",  -1 });

   // find the CSV column associated with each statistic type
   for (int i = 0; i < eqColInfos.GetSize(); i++)
      {
      HazDataColInfo *pInfo =  eqColInfos[i];
      pInfo->col = this->m_earthquakeData.GetCol(pInfo->field);
      ASSERT(pInfo->col >= 0);
      }

   //---------------------
   // Repeat for TSUNAMI
   //---------------------

   msg = "Acute Hazards: reading building damage parameter file ";
   msg += this->m_tsunamiInputPath;
   this->m_tsunamiData.ReadAscii(this->m_tsunamiInputPath, ',', 0);

   // pull out needed columns
   PtrArray<HazDataColInfo> tsuColInfos;
   tsuColInfos.Add(new HazDataColInfo{ "DS", -1 });
   tsuColInfos.Add(new HazDataColInfo{ "rep_time", -1 });
   tsuColInfos.Add(new HazDataColInfo{ "rep_cost", -1 });
   tsuColInfos.Add(new HazDataColInfo{ "habitable",-1 });
   //tsuColInfos.Add(new HazDataColInfo{ "CasSev1",  -1 });  // casuality severitys
   //tsuColInfos.Add(new HazDataColInfo{ "CasSev2",  -1 });
   //tsuColInfos.Add(new HazDataColInfo{ "CasSev3",  -1 });
   //tsuColInfos.Add(new HazDataColInfo{ "CasSev4",  -1 });

   // find the CSV column associated with each statistic type
   for (int i = 0; i < tsuColInfos.GetSize(); i++)
      {
      HazDataColInfo* pInfo = tsuColInfos[i];
      pInfo->col = this->m_tsunamiData.GetCol(pInfo->field);
      ASSERT(pInfo->col >= 0);
      }

   // iterate through IDUs, populating data on hazard states as needed
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   int idus = pIDULayer->GetRowCount();
   ASSERT(idus == m_earthquakeData.GetRowCount());
   ASSERT(idus == m_tsunamiData.GetRowCount());

   enum HAZARD_METRIC_INDEX { HMI_DS, HMI_REP_TIME, HMI_REP_COST, HMI_HABITABLE, HMI_CASSEV1,
      HMI_CASSEV2, HMI_CASSEV3, HMI_CASSEV4 };
   
   int currentYear = pEnvContext->currentYear;


   for (int idu = 0; idu < idus; idu++)
      {
      // we'll start with damage state
      int damageIndexEq = (int) m_earthquakeData.Get(eqColInfos[HMI_DS]->col, idu);
      int damageIndexTsu = (int) m_tsunamiData.Get(tsuColInfos[HMI_DS]->col, idu);

      damageIndexTsu--;  // TEMPORARY
      int damageIndex = max(damageIndexEq, damageIndexTsu);
      if (damageIndexEq == damageIndexTsu && damageIndex < 4)
         damageIndex++;

      // bldg damage 0(none)-4(max damage)
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamageEq, damageIndexEq, ADD_DELTA);
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamageTsu, damageIndexTsu, ADD_DELTA);
      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamage, damageIndex, ADD_DELTA);

      // was the building damaged? Then set repair time in IDUs
      if (damageIndex > 0)
         {
         float timeToRepair=0;
         if (damageIndexEq > damageIndexTsu)
            timeToRepair = m_earthquakeData.Get(eqColInfos[HMI_REP_TIME]->col, idu);
         else
            timeToRepair = m_tsunamiData.Get(eqColInfos[HMI_REP_TIME]->col, idu);

         int yearsToRepair = int(timeToRepair / 365);
         //yearsToRepair++;    // round up
         // for time to restore, we populate the following fields:
         // IDU field "REPAIR_YRS" (int) - put the year the building will be restored
         // IDU field "BLDG_STATUS" (int) - put the year the building status (0=normal, 1=being restored, 2=uninhabitable)
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduRepairYrs, yearsToRepair, ADD_DELTA);
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgStatus, 1, ADD_DELTA);

         // repair costs
         float repCost = m_earthquakeData.Get(eqColInfos[HMI_REP_COST]->col, idu);

         //ASSERT(repCost > 0);
         if (repCost > 0)
            {
            // for time to restore, we populate the following fields:
            // IDU field "BD_REPAIR" (float) - cost of any repair ($1000's of $)
            float impValue = 0;
            pIDULayer->GetData(idu, m_pAHModel->m_colIduImprValue, impValue);
            //float repairCost = impValue * repCostFrac / 1000000.0f;  // M$, since that's what the budget is
            float repCostM = repCost / 1000000.0f;  // M$, since that's what the budget is
            m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduRepairCost, repCostM, ADD_DELTA);
            } 
         
         // habitable
         float randVal = (float) m_pAHModel->m_randUniform.RandValue();   // 0-1
         float pHab = m_earthquakeData.Get(eqColInfos[HMI_HABITABLE]->col, idu);

         int habitable = (randVal >= pHab) ? 0 : 1;    
         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduHabitable, habitable, ADD_DELTA);
         }  // end of: if ( damageIndex > 0 )

      // life safety next
      float casSev1 = m_earthquakeData.Get(eqColInfos[HMI_CASSEV1]->col, idu);  // people
      float casSev2 = m_earthquakeData.Get(eqColInfos[HMI_CASSEV2]->col, idu);
      float casSev3 = m_earthquakeData.Get(eqColInfos[HMI_CASSEV3]->col, idu);
      float casSev4 = m_earthquakeData.Get(eqColInfos[HMI_CASSEV4]->col, idu);
      float casTotal = casSev1 + casSev2 + casSev3 + casSev4;
      m_pAHModel->m_numCasSev1 += casSev1;
      m_pAHModel->m_numCasSev2 += casSev2;
      m_pAHModel->m_numCasSev3 += casSev3;
      m_pAHModel->m_numCasSev4 += casSev4;

      m_pAHModel->m_numCasTotal += casTotal;

      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduCasualties, casTotal, ADD_DELTA);
      }  // end of: for each IDU

   return true;


/////   //---------------------
/////   // Earthquake first
/////   //---------------------
/////   // grab input file generated from damage model (csv)
/////   Report::Log("Acute Hazards: reading building damage parameter file");
/////   this->m_earthquakeData.ReadAscii(this->m_earthquakeInputPath, ',', 0);
/////
/////   // pull out needed columns
/////   PtrArray<HazDataColInfo> eqColInfos;
/////   eqColInfos.Add(new HazDataColInfo{ "P_DS_0", -1 });
/////   eqColInfos.Add(new HazDataColInfo{ "rep_time_mu_0", -1 });
/////   eqColInfos.Add(new HazDataColInfo{ "rep_time_std_0", -1 });
/////   eqColInfos.Add(new HazDataColInfo{ "rep_cost_DR_0",  -1 });
/////   eqColInfos.Add(new HazDataColInfo{ "hab_0",  -1 });
/////   eqColInfos.Add(new HazDataColInfo{ "DS",  -1 });
/////
/////   // find the CSV column associated with each statistic type
/////   for (int i = 0; i < eqColInfos.GetSize(); i++)
/////      {
/////      HazDataColInfo *pInfo =  eqColInfos[i];
/////
/////      pInfo->col = this->m_earthquakeData.GetCol(pInfo->field);
/////      ASSERT(pInfo->col >= 0);
/////      }
/////
/////   //---------------------
/////   // Repeat for TSUNAMI
/////   //---------------------
/////
/////   Report::Log("Acute Hazards: reading building damage parameter file");
/////   this->m_tsunamiData.ReadAscii(this->m_tsunamiInputPath, ',', 0);
/////
/////   // pull out needed columns
/////   PtrArray<HazDataColInfo> tsuColInfos;
/////   tsuColInfos.Add(new HazDataColInfo{ "P_DS_0", -1 });
/////   tsuColInfos.Add(new HazDataColInfo{ "rep_time_mu_0", -1 });
/////   tsuColInfos.Add(new HazDataColInfo{ "rep_time_std_0", -1 });
/////   tsuColInfos.Add(new HazDataColInfo{ "rep_cost_DR_0",  -1 });
/////   tsuColInfos.Add(new HazDataColInfo{ "hab_0",  -1 });
/////   tsuColInfos.Add(new HazDataColInfo{ "DS",  -1 });
/////
/////   // find the CSV column associated with each statistic type
/////   for (int i = 0; i < tsuColInfos.GetSize(); i++)
/////      {
/////      HazDataColInfo* pInfo = tsuColInfos[i];
/////
/////      pInfo->col = this->m_tsunamiData.GetCol(pInfo->field);
/////      ASSERT(pInfo->col >= 0);
/////      }
/////
/////   // iterate through IDUs, populating data on hazard states as needed
/////   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
/////   int idus = pIDULayer->GetRowCount();
/////   ASSERT(idus == m_earthquakeData.GetRowCount());
/////   ASSERT(idus == m_tsunamiData.GetRowCount());
/////
/////   enum HAZARD_METRIC_INDEX { HMI_P_DS0, HMI_REP_TIME_MU0, HMI_REP_TIME_STD0, HMI_REP_COST_DR0, HMI_HAB_0, HMI_DS };
/////   int currentYear = pEnvContext->currentYear;
/////
/////   // we'll start with damage state
/////   for (int idu = 0; idu < idus; idu++)
/////      {
/////      int damageIndexEq = (int) m_earthquakeData.Get(eqColInfos[HMI_DS]->col, idu);
/////      int damageIndexTsu = (int) m_tsunamiData.Get(tsuColInfos[HMI_DS]->col, idu);
/////      damageIndexTsu--;  // TEMPORARY
/////      int damageIndex = max(damageIndexEq, damageIndexTsu);
/////      if (damageIndexEq == damageIndexTsu && damageIndex < 4)
/////         damageIndex++;
/////
/////      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamageEq, damageIndexEq, ADD_DELTA);
/////      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamageTsu, damageIndexTsu, ADD_DELTA);
/////      m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgDamage, damageIndex, ADD_DELTA);
/////
/////      // was the building damaged? Then set repair time in IDUs
/////      if (damageIndex > 0)
/////         {
/////         float mean=0, std = 0,timeToRepair=0;
/////         if (damageIndexEq > damageIndexTsu)
/////            {
/////            mean = m_earthquakeData.Get(eqColInfos[HMI_REP_TIME_MU0]->col + damageIndex, idu);
/////            std = m_earthquakeData.Get(eqColInfos[HMI_REP_TIME_STD0]->col + damageIndex, idu);
/////            }
/////         else
/////            {
/////            mean = m_tsunamiData.Get(eqColInfos[HMI_REP_TIME_MU0]->col + damageIndex, idu);
/////            std =  m_tsunamiData.Get(eqColInfos[HMI_REP_TIME_STD0]->col + damageIndex, idu);
/////            }
/////
/////         timeToRepair = (float)m_pAHModel->m_randLogNormal.RandValue(mean, std);  // TEMPORARY
/////         int yearsToRepair = int(timeToRepair / 365);
/////         //yearsToRepair++;    // round up
/////         // for time to restore, we populate the following fields:
/////         // IDU field "REPAIR_YRS" (int) - put the year the building will be restored
/////         // IDU field "BLDG_STATUS" (int) - put the year the building status (0=normal, 1=being restored, 2=uninhabitable)
/////         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduRepairYrs, yearsToRepair, ADD_DELTA);
/////         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduBldgStatus, 1, ADD_DELTA);
/////
/////         // repair costs
/////         float repCostFrac = m_earthquakeData.Get(eqColInfos[HMI_REP_COST_DR0]->col + damageIndex, idu);
/////
/////         ASSERT(repCostFrac > 0);
/////         if (repCostFrac > 0)
/////            {
/////            ASSERT(repCostFrac < 1);
/////            // for time to restore, we populate the following fields:
/////            // IDU field "BD_REPAIR" (float) - cost of any repair ($1000's of $)
/////            float impValue = 0;
/////            pIDULayer->GetData(idu, m_pAHModel->m_colIduImprValue, impValue);
/////            float repairCost = impValue * repCostFrac / 1000000.0f;  // M$, since that's what the budget is
/////            m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduRepairCost, repairCost, ADD_DELTA);
/////            } 
/////         
/////         // habitable
/////         float randVal = (float) m_pAHModel->m_randUniform.RandValue();
/////         float pHab = m_earthquakeData.Get(eqColInfos[HMI_HAB_0]->col + damageIndex, idu);
/////
/////         int habitable = (randVal >= pHab) ? 0 : 1;    
/////         m_pAHModel->UpdateIDU(pEnvContext, idu, m_pAHModel->m_colIduHabitable, habitable, ADD_DELTA);
/////         }  // end of: if ( damageIndex > 0 )
/////
/////      }  // end of: for each IDU
/////   
/////   return true;
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
      , m_colIduBldgDamageEq(-1)
      , m_colIduBldgDamageTsu(-1)
      , m_colIduImprValue(-1)
      //, m_annualRepairCosts(0)
      , m_nUninhabitableStructures(0)
      , m_nDamaged(0)
      , m_nDamaged2(0)
      , m_nDamaged3(0)
      , m_nDamaged4(0)
      , m_nDamaged5(0)
      , m_bldgsRepaired(0)
      , m_bldgsBeingRepaired(0)
      , m_totalBldgs(0)
      , m_nInhabitableStructures(0)
      , m_pctInhabitableStructures(0)
      , m_nFunctionalBldgs(0)
      , m_pctFunctionalBldgs(0)
      , m_numCasSev1(0)
      , m_numCasSev2(0)
      , m_numCasSev3(0)
      , m_numCasSev4(0)
      , m_numCasTotal(0)
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
   this->CheckCol(pIDULayer, m_colIduBldgDamageEq, "BLDGDMGEQ", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduBldgDamageTsu, "BLDGDMGTS", TYPE_INT, CC_AUTOADD);
   this->CheckCol(pIDULayer, m_colIduRemoved, "REMOVED", TYPE_INT, CC_MUST_EXIST);
   this->CheckCol(pIDULayer, m_colIduCasualties, "CASUALTIES", TYPE_FLOAT, CC_AUTOADD);

   // initialize column info
   pIDULayer->SetColData(m_colIduRepairYrs, VData(-1), true);
   pIDULayer->SetColData(m_colIduBldgStatus, VData(0), true);
   pIDULayer->SetColData(m_colIduRepairCost, VData(0.0f), true);
   pIDULayer->SetColData(m_colIduHabitable, VData(1), true);
   pIDULayer->SetColData(m_colIduBldgDamage, VData(0), true);
   pIDULayer->SetColData(m_colIduBldgDamageEq, VData(0), true);
   pIDULayer->SetColData(m_colIduBldgDamageTsu, VData(0), true);

   // add input variables
   for (int i = 0; i < m_events.GetSize(); i++)
      {
      AHEvent *pEvent = m_events[i];
      CString label = pEvent->m_name + "_use";
      this->AddInputVar((LPCTSTR)label, pEvent->m_use, "");

      label = pEvent->m_name + "_year";
      this->AddInputVar((LPCTSTR)label, pEvent->m_year, "");
      }

   // add output variables
   //this->AddOutputVar("Annual Repair Expenditures", m_annualRepairCosts, "");
   this->AddOutputVar("Habitable Structures (count)", m_nInhabitableStructures, "");
   this->AddOutputVar("Habitable Structures (pct)", m_pctInhabitableStructures, "");
   this->AddOutputVar("Functional Bldgs (count)", m_nFunctionalBldgs, "");
   this->AddOutputVar("Functional Bldgs (pct)", m_pctFunctionalBldgs, "");
   this->AddOutputVar("Uninhabitable Structures", m_nUninhabitableStructures, "");
   this->AddOutputVar("Bldgs with Slight Damage", m_nDamaged2, "");
   this->AddOutputVar("Bldgs with Moderate Damage", m_nDamaged3, "");
   this->AddOutputVar("Bldgs with Extensive Damage", m_nDamaged4, "");
   this->AddOutputVar("Bldgs with Complete Damage", m_nDamaged5, "");
   this->AddOutputVar("Total No. Bldgs Damaged", m_nDamaged, "");
   this->AddOutputVar("Bldgs Repaired", m_bldgsRepaired, "");
   this->AddOutputVar("Bldgs Being Repaired", m_bldgsBeingRepaired, "");
   // life safety
   this->AddOutputVar("Casulties (Severity 1)", m_numCasSev1, "" );
   this->AddOutputVar("Casulties (Severity 2)", m_numCasSev2, "" );
   this->AddOutputVar("Casulties (Severity 3)", m_numCasSev3, "" );
   this->AddOutputVar("Casulties (Severity 4)", m_numCasSev4, "" );
   this->AddOutputVar("Casulties (Total)", m_numCasTotal, "");

   InitPython();

   return TRUE;
   }

bool AcuteHazards::InitRun(EnvContext *pEnvContext, bool useInitialSeed)
   {
   // reset annual output variables
   m_nUninhabitableStructures = 0;
   m_nDamaged2 = 0;
   m_nDamaged3 = 0;
   m_nDamaged4 = 0;
   m_nDamaged5 = 0;
   m_nDamaged = 0;
   m_bldgsRepaired = 0;
   m_bldgsBeingRepaired = 0;
   m_nInhabitableStructures = 0;
   m_pctInhabitableStructures = 0;
   m_totalBldgs = 0;
   m_nFunctionalBldgs = 0;
   m_pctFunctionalBldgs = 0;
 
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   int idus = pIDULayer->GetRowCount();
   for (int idu = 0; idu < idus; idu++)
      {
      float imprValue = 0;
      pIDULayer->GetData(idu, m_colIduImprValue, imprValue);
      
      if (imprValue > 10000)
         m_totalBldgs++;

      m_nInhabitableStructures = m_totalBldgs;
      m_pctInhabitableStructures = 1.0f;
      m_nFunctionalBldgs = m_totalBldgs;
      m_pctFunctionalBldgs = 1.0f;
      }
 
   // run any scheduled events
   for (int i = 0; i < m_events.GetSize(); i++)
      m_events[i]->m_status = AHS_PRE_EVENT;

   return TRUE;
   }

bool AcuteHazards::Run(EnvContext *pEnvContext)
   {
   int currentYear = pEnvContext->currentYear;
   
   // reset annual output variables
   m_nUninhabitableStructures = 0;
   m_nDamaged  = 0;
   m_nDamaged2 = 0;
   m_nDamaged3 = 0;
   m_nDamaged4 = 0;
   m_nDamaged5 = 0;
   m_bldgsRepaired = 0;
   m_bldgsBeingRepaired = 0;
   m_nInhabitableStructures = 0;
   m_pctInhabitableStructures = 0;
   m_totalBldgs = 0;
   m_nFunctionalBldgs = 0;
   m_pctFunctionalBldgs = 0;

   // update building stats
   MapLayer* pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
   int idus = pIDULayer->GetRowCount();
   for (int idu = 0; idu < idus; idu++)
      {
      float imprValue = 0;
      pIDULayer->GetData(idu, m_colIduImprValue, imprValue);

      if (imprValue > 10000)
         m_totalBldgs++;
      }

   m_nInhabitableStructures = m_totalBldgs;
   m_pctInhabitableStructures = 1.0f;
   m_nFunctionalBldgs = m_totalBldgs;
   m_pctFunctionalBldgs = 1.0f;
   
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

   //int idus = this->m_earthquakeData.GetRowCount();
   int idus = pIDULayer->GetRowCount();

   // loop through IDU's, interpreting the probability information
   // from the damage model into the IDUs
   int removed = 0;

   for (int idu = 0; idu < idus; idu++)
      {
      float imprValue = 0;
      pIDULayer->GetData(idu, m_colIduImprValue, imprValue);

      if (imprValue > 10000)
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
               UpdateIDU(pEnvContext, idu, m_colIduRepairYrs, -1, ADD_DELTA);
               UpdateIDU(pEnvContext, idu, m_colIduRepairCost, 0.0f, ADD_DELTA);
               UpdateIDU(pEnvContext, idu, m_colIduBldgStatus, 0, ADD_DELTA);
               UpdateIDU(pEnvContext, idu, m_colIduHabitable, 1, ADD_DELTA);
               UpdateIDU(pEnvContext, idu, m_colIduBldgDamage, 0, ADD_DELTA);
               }
            }

         int habitable = 0;
         pIDULayer->GetData(idu, m_colIduHabitable, habitable);
         if (habitable == 0)
            m_nUninhabitableStructures++;
   
         int _removed = 0;
         pIDULayer->GetData(idu, m_colIduRemoved, _removed);
         if (_removed > 0)
            removed += 1; // _removed;
         }
      }  // end of: for each idu

   // summary stats
   m_nDamaged = m_nDamaged2 + m_nDamaged3 + m_nDamaged4 + m_nDamaged5;

   m_totalBldgs += removed;
   m_nInhabitableStructures = m_totalBldgs - m_nUninhabitableStructures;
   m_pctInhabitableStructures = float(m_nInhabitableStructures)/m_totalBldgs;
   m_nFunctionalBldgs = m_totalBldgs - m_nDamaged;
   m_pctFunctionalBldgs = float(m_nDamaged) / m_totalBldgs;

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
         // attr                  type           address            isReq checkCol
         { "year",                TYPE_INT,      &pEvent->m_year,           true,  0 },
         { "name",                TYPE_CSTRING,  &pEvent->m_name,           true,  0 },
         { "use",                 TYPE_INT,      &pEvent->m_use,            true,  0 },
         { "py_function",         TYPE_CSTRING,  &pEvent->m_pyFunction,     true,  0 },
         { "py_module",           TYPE_CSTRING,  &pyModulePath,             true,  0 },
         { "env_output",          TYPE_CSTRING,  &pEvent->m_envOutputPath,  true,  0 },
         { "earthquake_input",    TYPE_CSTRING,  &pEvent->m_earthquakeInputPath,   true,  0 },
         { "tsunami_input",       TYPE_CSTRING,  &pEvent->m_tsunamiInputPath,   true,  0 },
         { "earthquake_scenario", TYPE_CSTRING,  &pEvent->m_earthquakeScenario, true,  0 },
         { "tsunami_scenario",    TYPE_CSTRING,  &pEvent->m_tsunamiScenario, true,  0 },
         { NULL,                  TYPE_NULL,     NULL,        false, 0 } };

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
