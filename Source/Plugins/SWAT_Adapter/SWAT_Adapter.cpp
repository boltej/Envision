// SWAT_Adapter.cpp : Defines the initialization routines for the DLL.
//
#include "stdafx.h"
#include "SWAT_Adapter.h"

#include <Maplayer.h>
#include <tixml.h>
#include <Path.h>

#include <direct.h>
#include <EnvExtension.h>
#include <FDataObj.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern "C" _EXPORT EnvExtension * Factory(EnvContext*) { return (EnvExtension*) new SWAT_Adapter; }

SWAT_Adapter::SWAT_Adapter()
	: EnvModelProcess()
	, m_pResData(NULL)
	, m_swatPath("C:\\Envision\\StudyAreas\\CalFEWS\\Swat\\TxtInOut")
	, m_hInst(0)
	, m_GetNamFilFn(NULL)
	, m_RunSWATFn(NULL)
	, m_EndRunSWATFn(NULL)
	, m_InitSWATFn(NULL)
	, m_RunSWAT_AllFn(NULL)
	, m_GetalloFn(NULL)
	, m_ParmFn(NULL)
	, m_iGrid(1)

	//, m_randNormal(1, 1, 0)
	//, m_randUniform(0.0, 1.0, 0)

{ }



SWAT_Adapter::~SWAT_Adapter(void)
{
	// Clean up

}

bool SWAT_Adapter::Init(EnvContext* pEnvContext, LPCTSTR initStr)
{
#if defined( _DEBUG )
	m_hInst = ::AfxLoadLibrary("SWAT_DLL.dll");
	// m_hInst = ::AfxLoadLibrary("Example.dll");
//	 m_hInst = LoadLibrary("C:\\Users\\kelli\\Documents\\GitHub\\Envision\\Source\\Example\\debug\\Example_DLL.dll");
//	 m_hInst = ::AfxLoadLibrary("HBV.dll");
#else
	m_hInst = ::AfxLoadLibrary("\\Envision\\Source\\plugins\\SWAT_DLL\\debug\\SWAT_DLL.dll");
#endif

	if (m_hInst == 0)
	{
		CString msg("SWAT: Unable to find the SWAT dll");

		Report::ErrorMsg(msg);
		return FALSE;
	}

	//m_GetNamFilFn = (MODFLOW_GETNAMFILFN)GetProcAddress(m_hInst, "GETNAMFIL");
	m_RunSWAT_AllFn = (SWAT_RUN_ALLFN)GetProcAddress(m_hInst, "SWAT_DLL");
	m_InitSWATFn = (SWAT_INITFN)GetProcAddress(m_hInst, "INIT");
	m_RunSWATFn = (SWAT_RUNFN)GetProcAddress(m_hInst, "RUNYEAR");
	m_EndRunSWATFn = (SWAT_ENDRUNFN)GetProcAddress(m_hInst, "ENDRUN");
	//m_GetalloFn = (SWAT_GETALLOFN)GetProcAddress(m_hInst, "getallo");
	//m_ParmFn = (SWAT_PARMFN)GetProcAddress(m_hInst, "parm");

	if (m_hInst != 0)
	{
		CString msg("SWAT_Adapter::Init successfull");
		Report::LogInfo(msg);
	}
	m_cc = new float[2][2];


	//m_iunit = new int[100];
	int m_iunit[100];
	m_iGrid = 2;
	//InitSWAT(m_iunit[23 - 1], &m_iGrid, m_cc);
	//int* ppp = m_iunit;
	//int x = 1;


#define numNewAgents 5
	//In C, if a variable is an array or a pointer, do not apply the '&' operator to the variable name when passing it to Fortran.
	//In other words, pass the pointer, not the address of the pointer.
	double starttime = 0;
	double timeStep = 10;
	double agents_in_time[numNewAgents];
	int agents_in_door[numNewAgents];
	

		for (int i = 0; i < numNewAgents; ++i) 
		{
			agents_in_time[i] = 2.5;
			agents_in_door[i] = 15 - i;
		}

   InitSWAT(agents_in_door, &m_iGrid, agents_in_time);
	return TRUE;
}

bool SWAT_Adapter::InitRun(EnvContext* pEnvContext, bool useInitialSeed)
{

	return TRUE;
}
bool SWAT_Adapter::Run(EnvContext* pContext)
{
	RunSWAT();
	CString msg;
	msg.Format("SWAT_Adapter::Year %i done", pContext->currentYear);
	Report::LogInfo(msg);
	return TRUE;
}
bool SWAT_Adapter::EndYear(EnvContext* pContext)
{
	EndRunSWAT();
	CString msg;
	msg.Format("SWAT_Adapter::Simulation complete (%i years)", pContext->yearOfRun);
	Report::LogInfo(msg);
	return TRUE;
}

void SWAT_Adapter::GetNamFil(char* envFileName)
{
	m_GetNamFilFn(envFileName);
}

void SWAT_Adapter::OpenNamFil(int* envInUnit, char* envFileName, int* envIbdt)
{
	//m_OpenNamFilFn(envInUnit, envFileName, envIbdt);
}
void SWAT_Adapter::RunSWAT_All()
{
	m_RunSWAT_AllFn();
}
void SWAT_Adapter::RunSWAT()
{
	m_RunSWATFn();
}
void SWAT_Adapter::InitSWAT(int* jiunit, int* enviGrid, void* envCC)
{
	m_InitSWATFn(jiunit,enviGrid,envCC);
}
void SWAT_Adapter::EndRunSWAT()
{
	m_EndRunSWATFn();
}
void SWAT_Adapter::GetAlloFil()
{
	m_GetalloFn();
}
void SWAT_Adapter::Parm()
{
	m_ParmFn();
}

bool SWAT_Adapter::LoadXml(EnvContext* pEnvContext, LPCTSTR filename)
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


	XML_ATTR attrs[] = {
		// attr            type           address            isReq checkCol
		{ "python_path",  TYPE_CSTRING,   &m_swatPath,     true,  0 },
		{ NULL,           TYPE_NULL,     NULL,               false, 0 } };
	ok = TiXmlGetAttributes(pXmlRoot, attrs, filename, NULL);

	TiXmlElement* pXmlEvent = pXmlRoot->FirstChildElement("event");
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

			{ "py_function",     TYPE_CSTRING,  &m_swatPath,        true,  0 },
			{ "py_module",       TYPE_CSTRING,  &pyModulePath,        true,  0 },
			{ NULL,              TYPE_NULL,     NULL,        false, 0 } };

		ok = TiXmlGetAttributes(pXmlEvent, attrs, filename, NULL);

		if (!ok)
		{
			CString msg;
			msg = "SWAT Model:  Error reading <event> tag in input file ";
			msg += filename;
			Report::ErrorMsg(msg);
			return false;
		}
		else
		{
			nsPath::CPath path(pyModulePath);
		}

		pXmlEvent = pXmlEvent->NextSiblingElement("event");
	}
	return true;
}
