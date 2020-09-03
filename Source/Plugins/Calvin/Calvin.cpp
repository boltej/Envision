// Calvin.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Calvin.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>

#include <direct.h>
#include <EnvExtension.h>
#include <FDataObj.h>
#include <Vdataobj.h>

#ifdef _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <python.h>
#endif
using namespace std;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern "C" _EXPORT EnvExtension* Factory(EnvContext*) { return (EnvExtension*) new Calvin; }
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



Calvin::Calvin()
	: EnvModelProcess()
	, m_pResData(NULL)
   , m_numRes(1)
   , m_reservoirList("")
   , m_outflowLinks("")
	//, m_randNormal(1, 1, 0)
	//, m_randUniform(0.0, 1.0, 0)

{ }



Calvin::~Calvin(void)
{
	// Clean up
	Py_Finalize();   // clean up python instance
}



bool Calvin::Init(EnvContext *pEnvContext, LPCTSTR initStr)
{
	bool ok = LoadXml(pEnvContext, initStr);
	if (!ok)
		return FALSE;

	MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

	InitPython();

   //// Step 3.  Write monthly data into the Calvin input spreadsheet
   //VDataObj *pExchange = new VDataObj(6, 0, U_UNDEFINED);
   //pExchange->ReadAscii("c:\\envision\\studyareas\\calfews\\calvin\\linksWY1922a.csv", ',');
   //pExchange->WriteAscii("c:\\envision\\studyareas\\calfews\\calvin\\linksWY1922.csv");



   for (int i = 0; i < m_reservoirNameArray.GetSize(); i++)
   {
      FDataObj *pData = new FDataObj(4,0);
    //  if (m_pResData == NULL)
     // {
     //    m_pResData = new FDataObj(4, 0, U_DAYS);
     //    ASSERT(m_pResData != NULL);
     // }

      pData->SetLabel(0, _T("Time"));
      pData->SetLabel(1, _T("Inflow (m3/s)"));
      pData->SetLabel(2, _T("Storage (acre/feet)"));
      pData->SetLabel(3, _T("Outflow (m3/s)"));

      this->AddOutputVar(m_reservoirNameArray.GetAt(i), pData, "");
      m_reservoirDataArray.Add(pData);
   }
	return TRUE;
}

bool Calvin::InitRun(EnvContext *pEnvContext, bool useInitialSeed)
   {   

	return TRUE;
}

bool Calvin::EndYear(EnvContext *pEnvContext)
   {
   // The Calvin simulated reservoir height and discharge have already been written to the output files.
   //  Here we write those data to the Calvin DataObjs.  This allows inflow to show up in Envision Output
//Step 1.  Get monthly data from Calvin
//   VDataObj *pExchange = new VDataObj(U_UNDEFINED);SR_PNF-C51
   
   int numRes = m_reservoirNameArray.GetSize();
   float *row = new float[4];//date, inflow, outflow, storage

   FDataObj *pExchange = new FDataObj();
   FDataObj *pStorage = new FDataObj();
   pStorage->ReadAscii("c:\\envision\\studyareas\\calfews\\calvin\\example-results\\storage.csv");
   pExchange->ReadAscii("c:\\envision\\studyareas\\calfews\\calvin\\example-results\\flow.csv");
   int yearOffset = pEnvContext->currentYear - pEnvContext->startYear;
   int inflowOffset = 0;
   int outflowOffset = 0;
   int storageOffset = 0;



   for (int i = 0; i < numRes; i++)
      {
      CString msg;
      CString name = m_reservoirNameArray.GetAt(i);
      FDataObj *pResData = m_reservoirDataArray.GetAt(i);
      //Step1.  Find Inflow column
      msg.Format(_T("INFLOW-%s"), name);
      for (int j = 0; j < pExchange->GetColCount(); j++)//search columns to find correct name
         {
         CString lab = pExchange->GetLabel(j);
         inflowOffset = j;
         if (lab == msg)
            break;
         }
      //Step 2.  Find Outflow column     
      name = m_outflowLinkArray.GetAt(i);
      for (int j = 0; j < pExchange->GetColCount(); j++)//search columns to find correct name
         {
         CString lab = pExchange->GetLabel(j);
         outflowOffset = j;
         if (lab == name)
            break;
         }

      //Step 3.  Find Storage column
      name = m_reservoirNameArray.GetAt(i);
      for (int j = 0; j < pStorage->GetColCount(); j++)//search columns to find correct name
         {
         CString lab = pStorage->GetLabel(j);
         storageOffset = j;
         if (lab == name)
            break;
         }

      for (int k = 0; k < 12; k++)//the output adds blank rows...This assumes the model was run for 1 year (so 12 rows total)
         {
         //float dat = pExchange->GetAsFloat(inflowOffset, k);//This is storage data
         //pResData->Add(2,k+(yearOffset*12),dat);
         row[0] = k+(12*yearOffset);
         row[1] = pExchange->GetAsFloat(inflowOffset, k);
         row[3] = pExchange->GetAsFloat(outflowOffset, k);
         row[2] = pStorage->GetAsFloat(storageOffset, k);
         pResData->AppendRow(row,4);
         }     
   }
   delete [] pExchange;
   delete [] pStorage;
   delete [] row;
/*
   pExchange = new FDataObj();
   pExchange->ReadAscii("c:\\envision\\studyareas\\calfews\\calvin\\example-results\\flow.csv");
   
   calvinOffset = 0;

   numRes = m_outflowLinkArray.GetSize();
   for (int i = 0; i < numRes; i++)
   {
      CString name = m_outflowLinkArray.GetAt(i);
      for (int j = 0; j < pExchange->GetColCount(); j++)//search columns to find correct name
      {
         CString lab = pExchange->GetLabel(j);
         calvinOffset = j;
         if (lab == name)
            break;
      }
      int offset = 0;
      // Step 2.  Read data from Calvin Output, add that data to an Calvin DataObj

      FDataObj *pResData = m_reservoirDataArray.GetAt(i);
      for (int k = 0; k < 12; k++)//the output adds blank rows...This assumes the model was run for 1 year (so 12 rows total)
      {
         float dat = pExchange->GetAsFloat(calvinOffset, k);//This is outflow data
         pResData->Set(3, k+(yearOffset*12), dat);
      }

   }
   delete[] pExchange;
*/

   return TRUE;
   }

bool Calvin::Run(EnvContext *pEnvContext)
{

   // The simulated inflow data have already been written to the links file.
   //  Here we write those data to the Calvin DataObjs.  This allows inflow to show up in Envision Output
/*   FDataObj *pMnthlyData = new FDataObj;
   pMnthlyData->ReadAscii("c:\\envision\\studyareas\\calfews\\calvin\\exchange.csv");
   int numRes = m_reservoirNameArray.GetSize() + 1;
   float *row = new float[4];
   for (int i = 0; i < m_reservoirNameArray.GetSize(); i++)
   {
      FDataObj *pResData = m_reservoirDataArray.GetAt(i);
      for (int month = 0; month < 12; month++)
      {
         row[0] = month + 1;
         row[1] = pMnthlyData->GetAsFloat(i + 1, month);
         row[2] = 0.0f;
         row[3] = 0.0f;
         pResData->AppendRow(row, 4);//add time and flow to this dataObj.
      }
   }
   delete[] pMnthlyData;
*/

   int retVal2;
   FILE *file = _Py_fopen("RunCalvin.py", "r+");

   if (file != NULL) {

      retVal2 = PyRun_SimpleFile(file, "RunCalvin.py");
   }

   EndYear(pEnvContext);
//Need to move data from Calvin files to Calvin DataObjs


/*
	int currentYear = pEnvContext->currentYear;

	// output the current IDU dataset as a DBF. Note that providing a bitArray would reduce size subtantially!
	//MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;
	//pIDULayer->SaveDataDB(this->m_envOutputPath, NULL);   // save entire IDU as DBF.  

	char cwd[512];
	_getcwd(cwd, 512);

	_chdir("c:/Envision/studyAreas/CalFEWS/Calvin");

	//CString code;
	//code.Format("sys.path.append(\"C:\\Envision\studyAreas\CalFEWS\Calvin\")");
	int retVal = PyRun_SimpleString("sys.path.append(\"C:\\Envision\\studyAreas\\CalFEWS\\Calvin\")");

	//int retVal2 = PyRun_SimpleString("sys.path.append(\"C:\\Program Files (x86)\\Microsoft Visual Studio\\Shared\\Anaconda2_64\\pkgs\\numpy-base-1.14.3-py27h917549b_1\\Lib\\site-packages\\numpy\")");

	//PyObject *pName = PyUnicode_DecodeFSDefault((LPCTSTR)this->m_pyModuleName);   // "e.g. "test1" (no .py)
	
	
	int retVal2;
	
	//PyObject *obj = Py_BuildValue("s", "C:\\Envision\\studyareas\\CalFEWS\\Calvin\\RunCalvin.py");
	//FILE *file = _Py_fopen_obj(obj, "r+");
	FILE *file = _Py_fopen("hello.py", "r+");

	if (file != NULL) {
		retVal2 = PyRun_SimpleFile(file, "hello.py");
	}
	
	//Dont use test.py as it actually searches sub module test>>py
	//PyObject * moduleName = PyUnicode_FromString("hello");
	//PyObject * pluginModule = PyImport_Import(moduleName);

	//if (pluginModule == nullptr)
	//{
	//	PyErr_Print();
	//	return "";
	//}
	
	PyObject *pNameC = PyUnicode_FromString("calvin");
	PyObject *pModuleC = NULL;

	try
	{
		pModuleC = PyImport_Import(pNameC);
	}
	catch (...)
	{
		CaptureException();
	}
	
	
	
	
	PyObject *pName = PyUnicode_FromString("RunCalvin");
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
		CString msg("Calvin:  Running Python module ");
		msg += this->m_pyModuleName;
		Report::Log(msg);

		// pDict is a borrowed reference 
		PyObject *pDict = PyModule_GetDict(pModule);
		//pFunc is also a borrowed reference
		PyObject *pFunc = PyObject_GetAttrString(pModule, (LPCTSTR)m_pyFunction);
		if (pFunc && PyCallable_Check(pFunc))
		   {


//			PyObject* pScenario = PyUnicode_FromString((LPCTSTR)this->m_hazardScenario);/
//			PyObject* pInDBFPath = PyUnicode_FromString((LPCTSTR)this->m_envOutputPath);
//			PyObject* pOutCSVPath = PyUnicode_FromString((LPCTSTR)this->m_envInputPath);
			//PyObject *pName1 = PyUnicode_FromString("linksWY1922.csv");
			PyObject* pRetVal = PyObject_CallFunction(pFunc, NULL);
			
			if (pRetVal != NULL)
			{

				Py_DECREF(pRetVal);
			}
			else
			{
				CaptureException();
			}

			//Py_DECREF(pScenario);
			//Py_DECREF(pInDBFPath);
			//Py_DECREF(pOutCSVPath);
		}
		else
		{
			PyErr_Print();
		}

		Py_DECREF(pModule);
	}

	// grab input file generated from damage model (csv)
	//Report::Log("Acute Hazards: reading building damage parameter file");
	//this->m_hazData.ReadAscii(this->m_envInputPath, ',', 0);

	// propagate event outcomes to IDU's
	//this->Propagate(pEnvContext);

	//this->m_status = AHS_POST_EVENT;

	_getcwd(cwd, 512);

	_chdir(cwd);
*/
	return TRUE;
}


bool Calvin::InitPython()
{
	
	Report::Log("Calvin:  Initializing embedded Python interpreter...");

	char cwd[512];
	_getcwd(cwd, 512);


	// launch a python instance and run the model
	wchar_t path[1024];

	Py_SetProgramName(L"C:\\Users\\kelli\\anaconda3\\envs\\calvin35d\\python");  // argv[0]
//	Py_SetProgramName(L"python");
//	Py_SetPythonHome(L"Envision");
//	Py_SetProgramName(L"Envision");  // argv[0]
	/*int cx = swprintf(path, 512, L"%hs;%hsbin;%hsScripts;%hsLibrary\\bin;%hsLibrary\\usr\\bin;%hsLibrary\\mingw-w64\\bin;%hsLibrary\\lib;%hsLibrary\\include;%hsDLLs;%hslib;%hs;%hslib\\site-packages;%hsScripts",
		(LPCTSTR)m_pyModulePath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath,
		(LPCTSTR)m_pyModulePath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath,
		(LPCTSTR)m_pyModulePath,(LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, 
		(LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath);
		*/
	int cx = swprintf(path, 512, L"%hs;%hsLibrary\\bin;%hsDLLs;%hslib;%hslib\\site-packages;%hslibrary",
		(LPCTSTR)m_pyModulePath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath,
		(LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath, (LPCTSTR)m_pythonPath);
	
	Py_SetPath(path);
	PyImport_AppendInittab("redirection", PyInit_redirection);

	Py_Initialize();


	// add Python function for redirecting output on the Python side
	int retV = PyRun_SimpleString("\
import redirection\n\
import sys\n\
class StdoutCatcher:\n\
    def write(self, stuff):\n\
        redirection.stdoutredirect(stuff)\n\
sys.stdout = StdoutCatcher()");
	/////////////

	int retValy = PyRun_SimpleString("import sys");

	_chdir("/Envision/studyAreas/Calfews/Calvin");

	CString code;
	//code.Format("sys.path.append('%s')", (LPCTSTR)this->m_pyModulePath);
	code.Format("sys.path.append('%s')", (LPCTSTR)"C:\\Users\\kelli\\anaconda3\\envs\\calvin35d\\Library\\bin");
	int retVal = PyRun_SimpleString(code);
	FILE* file = _Py_fopen("test.py", "r+");
	int retVal23 = PyRun_SimpleFile(file, "test.py");
	int retVa1 = PyRun_SimpleString("import numpy");
	int retV1 = PyRun_SimpleString("from pyomo.opt import *");
	int retV9 = PyRun_SimpleString("from pyomo.opt.base.solvers import _extract_version");
	int rV9 = PyRun_SimpleString("from pyomo.opt.solver import SystemCallSolver");


	int ret = PyRun_SimpleString("import pyutilib.subprocess");
	int re1 = PyRun_SimpleString("from pyutilib.misc import Bunch, Options");
	int reV9 = PyRun_SimpleString("from pyutilib.services import register_executable, registered_executable");
	int r9 = PyRun_SimpleString("from pyutilib.services import TempfileManager");




	Report::Log((LPCTSTR)"Calvin: Python initialization complete...");

	const char* comp = Py_GetCompiler();
	const char* comp2 = Py_GetVersion();
	const char* comp4 = Py_GetCopyright();
	wchar_t* comp3 = Py_GetProgramFullPath();
	wchar_t* comp8 = Py_GetPythonHome();
	return true;
}



// Update is called during Run() processing.
// For any events that are in AHS_POST_EVENT status,
// do any needed update of IDUs recovering from that event 
// Notes: 
//  1) Any building tagged for repair will have "BLDG_DAMAGE" set to a negative value

bool Calvin::Update(EnvContext *pEnvContext)
{
	// basic idea is to loop through IDUs', applying any post-event
	// changes to the landscape, e.g. habitability, recovery period, etc.
	MapLayer *pIDULayer = (MapLayer*)pEnvContext->pMapLayer;

	//int idus = this->m_hazData.GetRowCount();
	int idus = pIDULayer->GetRowCount();

	// loop through IDU's, interpreting the probability information
	// from the damage model into the IDUs
	int coli = 1;
	for (int idu = 0; idu < idus; idu++)
	{
		int damage = 0;
		pIDULayer->GetData(idu, coli, damage);

		switch (damage)
		{
		case 1:  coli++; break;

		}


	}  // end of: for each idu



	return true;
}


bool Calvin::LoadXml(EnvContext *pEnvContext, LPCTSTR filename)
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
		CString msg("Calvin: no <event>'s defined");
		msg += filename;
		Report::ErrorMsg(msg);
		return false;
	}
	while (pXmlEvent != NULL)
	{
		CString pyModulePath;
		CString pyFunction;
      CString resList;

		XML_ATTR attrs[] = {
			// attr              type           address            isReq checkCol

			{ "py_function",     TYPE_CSTRING,  &m_pyFunction,        true,  0 },
			{ "py_module",       TYPE_CSTRING,  &pyModulePath,        true,  0 },
         { "ReservoirList",   TYPE_CSTRING,  &m_reservoirList,     true,  0 },
         { "OutflowLinks",   TYPE_CSTRING,  &m_outflowLinks,     true,  0 },

			{ NULL,              TYPE_NULL,     NULL,        false, 0 } };

		ok = TiXmlGetAttributes(pXmlEvent, attrs, filename, NULL);

		if (!ok)
		   {
			CString msg;
			msg = "Calvin Model:  Error reading <event> tag in input file ";
			msg += filename;
			Report::ErrorMsg(msg);
		 	return false;
		   }
		else
		   {
			nsPath::CPath path(pyModulePath);
			m_pyModuleName = path.GetTitle();
			m_pyModulePath = path.GetPath();
         // parse reservoir names
         int _res = 0xFFFF;   // assume everything by default
         if (_res != NULL)
            {
            _res = 0;   // if specified, assume nothing by default

            TCHAR *buffer = new TCHAR[lstrlen(m_reservoirList) + 1];
            lstrcpy(buffer, m_reservoirList);
            TCHAR *next = NULL;
            TCHAR *token = _tcstok_s(buffer, _T(",+|:"), &next);

            while (token != NULL)
               {
               m_reservoirNameArray.Add(token);
               token = _tcstok_s(NULL, _T(",+|:"), &next);
               }

            delete[] buffer;
            
            int _out = 0xFFFF;
       //Outflow links
            if (_out != NULL)
            {
               _out = 0;   // if specified, assume nothing by default

               TCHAR *buffer = new TCHAR[lstrlen(m_outflowLinks) + 1];
               lstrcpy(buffer, m_outflowLinks);
               TCHAR *next = NULL;
               TCHAR *token = _tcstok_s(buffer, _T(",+|:"), &next);

               while (token != NULL)
               {
                  m_outflowLinkArray.Add(token);
                  token = _tcstok_s(NULL, _T(",+|:"), &next);
               }

               delete[] buffer;
            }
            }
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



