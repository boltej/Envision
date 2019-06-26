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
// FlamMapAPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FlammapAP.h"
#include "FlamMapAPDlg.h"
#include "afxdialogex.h"

#include <EnvEngine\EnvConstants.h>
#include <Path.h>
#include <PathManager.h>
#include "ScenarioDlg.h"
using namespace nsPath;
extern FlamMapAP *gpModel;
//#include <ShlObj.h>
CString MakeRelativeToProjectDir(CString input, bool isDir)
{
	char resultPath[MAX_PATH];
	CString resultStr = "";
	CPath prjPath = PathManager::GetPath(PM_PROJECT_DIR);   // slash terminated, assumes directory is off of study area
	CString basePath;
	PathManager::FindPath(prjPath, basePath);
	if (PathRelativePathTo(resultPath, basePath, FILE_ATTRIBUTE_DIRECTORY, input, isDir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL))
	{
		if (resultPath[0] == '.' && resultPath[1] == '\\')
		{
			strcpy(resultPath, &resultPath[2]);
		}
		CPath newPath = resultPath;
		newPath.Canonicalize();
		resultStr = resultPath;
	}
	else
		resultStr = input;
	return resultStr;
}
// CFlamMapAPDlg dialog
IMPLEMENT_DYNAMIC(CFlamMapAPDlg, CDialog)
CustomEditBrowseCtrl::CustomEditBrowseCtrl() : CMFCEditBrowseCtrl()
{
	topDir = "";
}
void CustomEditBrowseCtrl::OnBrowse()
{
	if (GetMode() == BrowseMode_Folder)
	{
		LPMALLOC pMalloc;
		if (::SHGetMalloc(&pMalloc) == NOERROR)
		{
			DWORD  retval = 0;
			const int BUFSIZE = 1024;
			char buffer[BUFSIZE];
			char **lppPart = { NULL };
			retval = GetFullPathName(topDir,
				BUFSIZE,
				buffer,
				lppPart);
			//CString path;
			//PathManager::FindPath(topDir, path);
			char pszBuffer[MAX_PATH];
			BROWSEINFO browseInfo;
			browseInfo.hwndOwner = GetSafeHwnd();
			if (topDir.GetLength() > 0)
				browseInfo.pidlRoot = ILCreateFromPath(buffer);
			else
				browseInfo.pidlRoot = NULL;
			browseInfo.pszDisplayName = pszBuffer;
			browseInfo.lpszTitle = "Select a folder";
			browseInfo.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
			browseInfo.lpfn = NULL;
			browseInfo.lParam = FALSE;

			LPITEMIDLIST pidl = NULL;
			if ((pidl = ::SHBrowseForFolder(&browseInfo)) != NULL)
			{
				if (::SHGetPathFromIDList(pidl, pszBuffer))
				{
					SetWindowText(pszBuffer);
				}
				pMalloc->Free(pidl);
			}
			pMalloc->Release();
		}
	}
	else
	{
		CMFCEditBrowseCtrl::OnBrowse();
	}
	//CMFCEditBrowseCtrl::OnBrowse();
}

CFlamMapAPDlg::CFlamMapAPDlg(FlamMapAP *pFlamMapAP, MapLayer *pLayer, CWnd* pParent /*=NULL*/)
	: CDialog(CFlamMapAPDlg::IDD, pParent)
	, m_strOutputsPath(_T(""))
	, m_strPGLFile(_T(""))
	, m_strFMSPath(_T(""))
	, m_strStartingLCP(_T(""))
	, m_strVegclassLookup(_T(""))
	, m_strFMDFile(_T(""))
	, m_strConfigXML(_T(""))
	, m_bLogFlamelengths(FALSE)
	, m_bOutputFirelist(FALSE)
	, m_bOutputIgnitionProbs(FALSE)
	, m_bDeleteLCPs(FALSE)
	, m_bLogArrivalTimes(FALSE)
	, m_bLogPerimeters(FALSE)
	, m_WindSpeed(0)
	, m_windDir(0)
	, m_strStaticFMS(_T(""))
	, m_HeightUnits(0)
	, m_BulkDensityUnits(0)
	, m_CanopyCoverUnits(0)
	, m_FoliarMoisture(0)
	, m_CrownFireMethod(0)
	, m_RandomSeed(0)
	, m_SpotProbability(0)
	, m_UseFirelistIgnitions(FALSE)
	, m_IDUBurnPcntThreshold(0)
	, m_bLogDeltaArrayUpdates(FALSE)
	, m_bLogAnnualFlameLengths(FALSE)
	, m_MinFireSizeProportion(0.0)
	, m_maxRetries(0)
	, m_bSequentialFirelists(FALSE)
{
	m_pFlamMapAP = pFlamMapAP;
	m_bIsDirty = m_bScenariosChanged = m_bLCPGeneratorDirty = FALSE;
}

CFlamMapAPDlg::~CFlamMapAPDlg()
{
}

void CFlamMapAPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_OUTPUTS_PATH, m_ctrlOutputsPath);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_OUTPUTS_PATH, m_strOutputsPath);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_PGL_FILE, m_strPGLFile);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_FMS_PATH, m_strFMSPath);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_VEGCLASS_LOOKUP, m_strVegclassLookup);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_FMD_FILE, m_strFMDFile);
	DDX_Text(pDX, IDC_EDIT_CONFIG_XML, m_strConfigXML);
	DDX_Check(pDX, IDC_CHECK_LOGFLAMELENGTHS, m_bLogFlamelengths);
	DDX_Check(pDX, IDC_CHECK_OUTPUT_FIRELIST, m_bOutputFirelist);
	DDX_Check(pDX, TPUT_IGNITION_PROBABILITIES, m_bOutputIgnitionProbs);
	DDX_Check(pDX, IDC_CHECK_DELETE_LCPS, m_bDeleteLCPs);
	DDX_Check(pDX, IDC_CHECK_LOG_ARRIVAL_TIMES, m_bLogArrivalTimes);
	DDX_Check(pDX, IDC_CHECK_LOG_PERIMETERS, m_bLogPerimeters);
	//	DDX_Control(pDX, IDC_MFCEDITBROWSE_CONFIG_XML, m_ctrlConfigFile);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_STARTING_LCP, m_ctrlStartingLCP);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_PGL_FILE, m_ctrlPGL);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_VEGCLASS_LOOKUP, m_ctrlVegClass);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_FMD_FILE, m_ctrlFMD);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_FMS_PATH, m_ctrlFMS);
	DDX_Text(pDX, IDC_EDIT_WIND_SPEED, m_WindSpeed);
	DDV_MinMaxInt(pDX, m_WindSpeed, 0, 99);
	DDX_Text(pDX, IDC_EDIT_WIND_DIR, m_windDir);
	DDV_MinMaxInt(pDX, m_windDir, 0, 360);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_STATIC_FMS, m_strStaticFMS);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_STATIC_FMS, m_ctrlStaticFMS);
	DDX_Control(pDX, IDC_COMBO1, m_comboScenarios);
	DDX_Control(pDX, IDC_LIST_FIRELIST_BOX, m_ctrlFireListBox);
	DDX_Control(pDX, IDC_BUTTON_SELECT_FIRELISTS, m_btnSelectFirelists);
	DDX_Text(pDX, IDC_EDIT_HEIGHT_UNITS, m_HeightUnits);
	DDV_MinMaxInt(pDX, m_HeightUnits, 1, 4);
	DDX_Text(pDX, IDC_EDIT_DENSITY_UNITS, m_BulkDensityUnits);
	DDV_MinMaxInt(pDX, m_BulkDensityUnits, 1, 4);
	DDX_Text(pDX, IDC_EDIT_CANOPY_COVER_UNITS, m_CanopyCoverUnits);
	DDV_MinMaxInt(pDX, m_CanopyCoverUnits, 0, 1);
	DDX_Text(pDX, IDC_EDIT_FOLIAR_MOISTURE, m_FoliarMoisture);
	DDV_MinMaxInt(pDX, m_FoliarMoisture, 20, 200);
	DDX_Text(pDX, IDC_EDIT_CROWN_FIRE_METHOD, m_CrownFireMethod);
	DDV_MinMaxInt(pDX, m_CrownFireMethod, 0, 1);
	DDX_Text(pDX, IDC_EDIT_RANDOM_SEED, m_RandomSeed);
	DDX_Text(pDX, IDC_EDIT_SPOT_PROBABILITY, m_SpotProbability);
	DDV_MinMaxDouble(pDX, m_SpotProbability, 0.0, 1.0);
	DDX_Check(pDX, IDC_CHECK_USE_FIRELIST_IGNITIONS, m_UseFirelistIgnitions);
	DDX_Text(pDX, IDC_EDIT_IDU_BURN_PERCENT, m_IDUBurnPcntThreshold);
	DDV_MinMaxDouble(pDX, m_IDUBurnPcntThreshold, 0.01, 0.95);
	//if (!pDX->m_bSaveAndValidate)
	//{
	m_strStartingLCP = m_strStartingLCP;// , false);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_STARTING_LCP, m_strStartingLCP);
	//}
	DDX_Check(pDX, IDC_CHECK_LOG_DELTA_ARRAY_UPDATES, m_bLogDeltaArrayUpdates);
	DDX_Check(pDX, IDC_CHECK_LOGANNUALFLAMELENGTHS, m_bLogAnnualFlameLengths);
	DDX_Text(pDX, IDC_EDIT_MIN_PROPORTION, m_MinFireSizeProportion);
	DDV_MinMaxDouble(pDX, m_MinFireSizeProportion, 0.0, 10.0);
	DDX_Text(pDX, IDC_EDIT_MAX_RETRIES, m_maxRetries);
	DDV_MinMaxInt(pDX, m_maxRetries, 0, 100);
	DDX_Check(pDX, IDC_CHECK_SEQUENTIAL_FIRELISTS, m_bSequentialFirelists);
}


BEGIN_MESSAGE_MAP(CFlamMapAPDlg, CDialog)
	//ON_BN_CLICKED(IDC_BUTTON_FMS_PATH, &CFlamMapAPDlg::OnBnClickedButtonFmsPath)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_FIRELISTS, &CFlamMapAPDlg::OnBnClickedButtonSelectFirelists)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CFlamMapAPDlg::OnSelchangeComboScenario)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, &CFlamMapAPDlg::OnBnClickedButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_ALL, &CFlamMapAPDlg::OnBnClickedButtonDeleteAll)
	ON_BN_CLICKED(IDC_BUTTON_NEW_SCENARIO, &CFlamMapAPDlg::OnBnClickedButtonNewScenario)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_SCENARIO, &CFlamMapAPDlg::OnBnClickedButtonDeleteScenario)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_AS_XML, &CFlamMapAPDlg::OnBnClickedButtonSaveAsXml)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_XML, &CFlamMapAPDlg::OnBnClickedButtonLoadXml)
	ON_EN_CHANGE(IDC_MFCEDITBROWSE_STARTING_LCP, &CFlamMapAPDlg::OnEnChangeMfceditbrowseStartingLcp)
	ON_EN_CHANGE(IDC_MFCEDITBROWSE_PGL_FILE, &CFlamMapAPDlg::OnEnChangeMfceditbrowsePglFile)
	ON_EN_CHANGE(IDC_MFCEDITBROWSE_VEGCLASS_LOOKUP, &CFlamMapAPDlg::OnEnChangeMfceditbrowseVegclassLookup)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_SCENARIO, &CFlamMapAPDlg::OnBnClickedButtonEditScenario)
	ON_BN_CLICKED(IDC_CHECK_LOG_DELTA_ARRAY_UPDATES, &CFlamMapAPDlg::OnBnClickedCheckLogDeltaArrayUpdates)
	ON_BN_CLICKED(IDC_CHECK_LOGANNUALFLAMELENGTHS, &CFlamMapAPDlg::OnBnClickedCheckLogannualflamelengths)
END_MESSAGE_MAP()


// CFlamMapAPDlg message handlers


BOOL CFlamMapAPDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	const int BUFSIZE = 4096;
	char buffer[BUFSIZE];
	char **lppPart = { NULL };
	DWORD  retval = 0;
	m_strConfigXML = m_pFlamMapAP->m_initInfo;
	retval = GetFullPathName(m_strConfigXML,
		BUFSIZE,
		buffer,
		lppPart);
	if (retval != 0)
	{
		m_strConfigXML = buffer;
	}
	else
	{
		m_strConfigXML = "Lost!";
	}
	m_strOutputsPath = m_pFlamMapAP->m_outputPath;
	retval = GetFullPathName(m_strOutputsPath,
		BUFSIZE,
		buffer,
		lppPart);
	if (retval != 0)
	{
		m_strOutputsPath = buffer;
	}
	else
	{
		m_strOutputsPath = "Lost!";
	}
	CString iduPath = PathManager::GetPath(PM_IDU_DIR);   // slash terminated, assumes directory is off of study area
	CPath oPath = m_pFlamMapAP->m_fmsPath;
	if (oPath.IsRelative())
	{
		m_strFMSPath = iduPath + m_pFlamMapAP->m_fmsPath;
	}
	else
		m_strFMSPath = m_pFlamMapAP->m_fmsPath;
	retval = GetFullPathName(m_strFMSPath,
		BUFSIZE,
		buffer,
		lppPart);
	if (retval != 0)
	{
		m_strFMSPath = buffer;
	}
	else
	{
		m_strFMSPath = "Lost!";
	}
	oPath = m_pFlamMapAP->m_polyGridFName;
	if (oPath.IsRelative())
		m_strPGLFile = iduPath + m_pFlamMapAP->m_polyGridFName;
	else
		m_strPGLFile = m_pFlamMapAP->m_polyGridFName;

	retval = GetFullPathName(m_strPGLFile,
		BUFSIZE,
		buffer,
		lppPart);
	if (retval != 0)
	{
		m_strPGLFile = buffer;
	}
	else
	{
		m_strPGLFile = "Lost!";
	}
	m_strStartingLCP = m_pFlamMapAP->m_startingLCPFName;
	retval = GetFullPathName(m_strStartingLCP,
		BUFSIZE,
		buffer,
		lppPart);
	if (retval != 0)
	{
		m_strStartingLCP = buffer;
	}
	else
	{
		m_strStartingLCP = "Lost!";
	}
	m_strVegclassLookup = m_pFlamMapAP->m_vegClassLCPLookupFName;
	retval = GetFullPathName(m_strVegclassLookup,
		BUFSIZE,
		buffer,
		lppPart);
	if (retval != 0)
	{
		m_strVegclassLookup = buffer;
	}
	else
	{
		m_strVegclassLookup = "Lost!";
	}
	m_strFMDFile = m_pFlamMapAP->m_customFuelFileName;
	retval = GetFullPathName(m_strFMDFile,
		BUFSIZE,
		buffer,
		lppPart);
	if (retval != 0)
	{
		m_strFMDFile = buffer;
	}
	else
	{
		m_strFMDFile = "Lost!";
	}
	m_strStaticFMS = m_pFlamMapAP->m_strStaticFMS;
	retval = GetFullPathName(m_strStaticFMS,
		BUFSIZE,
		buffer,
		lppPart);
	if (retval != 0)
	{
		m_strStaticFMS = buffer;
	}
	else
	{
		m_strStaticFMS = "Lost!";
	}
	m_IDUBurnPcntThreshold = m_pFlamMapAP->m_iduBurnPercentThreshold;
	m_UseFirelistIgnitions = m_pFlamMapAP->m_useFirelistIgnitions;
	m_bLogFlamelengths = m_pFlamMapAP->m_logFlameLengths;
	m_bLogAnnualFlameLengths = m_pFlamMapAP->m_logAnnualFlameLengths;
	m_bOutputFirelist = m_pFlamMapAP->m_outputEnvisionFirelists;
	m_bLogDeltaArrayUpdates = m_pFlamMapAP->m_logDeltaArrayUpdates;
	m_bOutputIgnitionProbs = m_pFlamMapAP->m_outputIgnitProbGrids;
	m_bDeleteLCPs = m_pFlamMapAP->m_deleteLCPs;
	m_bLogArrivalTimes = m_pFlamMapAP->m_logArrivalTimes;
	m_bLogPerimeters = m_pFlamMapAP->m_logPerimeters;
	m_windDir = m_pFlamMapAP->m_staticWindDir;
	m_WindSpeed = m_pFlamMapAP->m_staticWindSpeed;
	//m_ctrlConfigFile.topDir = iduPath;
	m_ctrlStartingLCP.topDir = iduPath;
	m_ctrlPGL.topDir = iduPath;
	m_ctrlVegClass.topDir = iduPath;
	m_ctrlFMD.topDir = iduPath;
	m_ctrlFMS.topDir = iduPath;
	m_ctrlOutputsPath.topDir = iduPath;
	m_ctrlStaticFMS.topDir = iduPath;
//	m_ctrlConfigFile.EnableFileBrowseButton("xml", "xml files (*.xml)|*.xml||");
	m_ctrlStartingLCP.EnableFileBrowseButton("lcp", "Landscape Files (*.lcp)|*.lcp||");
	m_ctrlPGL.EnableFileBrowseButton("pgl", "pgl files (*.pgl)|*.pgl||");
	m_ctrlVegClass.EnableFileBrowseButton("csv", "csv files (*.csv)|*.csv||");
	m_ctrlFMD.EnableFileBrowseButton("fmd", "Custon Fuel Files (*.fmd)|*.fmd||");
	m_ctrlStaticFMS.EnableFileBrowseButton("fms", "Fuel Moisture Files (*.fms)|*.fms||");

	m_SpotProbability = m_pFlamMapAP->m_spotProbability;
	m_RandomSeed = m_pFlamMapAP->m_randomSeed;
	m_CrownFireMethod = m_pFlamMapAP->m_crownFireMethod;
	m_FoliarMoisture = m_pFlamMapAP->m_foliarMoistureContent;
	m_HeightUnits = m_pFlamMapAP->m_vegClassHeightUnits;
	m_BulkDensityUnits = m_pFlamMapAP->m_vegClassBulkDensUnits;
	m_CanopyCoverUnits = m_pFlamMapAP->m_vegClassCanopyCoverUnits;
	m_MinFireSizeProportion = m_pFlamMapAP->m_FireSizeRatio;
	m_maxRetries = m_pFlamMapAP->m_maxRetries;
	m_bSequentialFirelists = m_pFlamMapAP->m_sequentialFirelists;
	//add scenarios to combobox
	m_scenarioArray.Clear();

	CString tmpStr;
	int loc;
	for (int i = 0; i < m_pFlamMapAP->m_scenarioArray.GetSize(); i++)
	{
		FMScenario *scenario = new FMScenario();
		scenario->m_id = m_pFlamMapAP->m_scenarioArray[i]->m_id;
		scenario->m_name = m_pFlamMapAP->m_scenarioArray[i]->m_name;
		for (int s = 0; s < m_pFlamMapAP->m_scenarioArray[i]->m_fireListArray.GetSize(); s++)
		{
			scenario->AddFireList(new FIRELIST(m_pFlamMapAP->m_scenarioArray[i]->GetFireList(s)->m_path));
		}
		m_scenarioArray.Add(scenario);
	}

	for (int i = 0; i < m_scenarioArray.GetSize(); i++)
	{
		if (m_scenarioArray[i]->m_id != 0)
			tmpStr.Format("%d - %s", m_scenarioArray[i]->m_id, m_scenarioArray[i]->m_name);
		else
			tmpStr.Format("%d (Default) - %s", m_scenarioArray[i]->m_id, m_scenarioArray[i]->m_name);
		loc = m_comboScenarios.AddString(tmpStr);
		if (m_scenarioArray[i]->m_id == 0)
			m_comboScenarios.SetCurSel(loc);

	}
	SelectScenario();
	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}




void CFlamMapAPDlg::OnOK()
{
	UpdateData();
	CWaitCursor wait;
	m_pFlamMapAP->m_logFlameLengths = m_bLogFlamelengths;
	m_pFlamMapAP->m_logAnnualFlameLengths = m_bLogAnnualFlameLengths;
	m_pFlamMapAP->m_logDeltaArrayUpdates = m_bLogDeltaArrayUpdates;
	m_pFlamMapAP->m_outputEnvisionFirelists = m_bOutputFirelist;
	m_pFlamMapAP->m_outputIgnitProbGrids = m_bOutputIgnitionProbs ? true : false;
	m_pFlamMapAP->m_deleteLCPs = m_bDeleteLCPs ? true : false;
	m_pFlamMapAP->m_logArrivalTimes = m_bLogArrivalTimes;
	m_pFlamMapAP->m_logPerimeters = m_bLogPerimeters;
	m_pFlamMapAP->m_crownFireMethod = m_CrownFireMethod;
	m_pFlamMapAP->m_foliarMoistureContent = m_FoliarMoisture;
	m_pFlamMapAP->m_spotProbability = m_SpotProbability;
	m_pFlamMapAP->m_vegClassBulkDensUnits = m_BulkDensityUnits;
	m_pFlamMapAP->m_vegClassCanopyCoverUnits = m_CanopyCoverUnits;
	m_pFlamMapAP->m_vegClassHeightUnits = m_HeightUnits;
	m_pFlamMapAP->m_staticWindDir = m_windDir;
	m_pFlamMapAP->m_staticWindSpeed = m_WindSpeed;
	m_pFlamMapAP->m_strStaticFMS = m_strStaticFMS;
	m_pFlamMapAP->m_useFirelistIgnitions = m_UseFirelistIgnitions;
	//those were easy, now the toughies....
	m_pFlamMapAP->m_outputPath = m_strOutputsPath;
	m_pFlamMapAP->m_customFuelFileName = m_strFMDFile;
	m_pFlamMapAP->m_fmsPath = m_strFMSPath;
	m_pFlamMapAP->m_FireSizeRatio = m_MinFireSizeProportion;
	m_pFlamMapAP->m_maxRetries = m_maxRetries;
	m_pFlamMapAP->m_sequentialFirelists = m_bSequentialFirelists;

	if (m_pFlamMapAP->m_randomSeed != m_RandomSeed)
	{
		m_pFlamMapAP->DestroyRandUniforms();
		m_pFlamMapAP->m_randomSeed = m_RandomSeed;
		m_pFlamMapAP->CreateRandUniforms(true);
	}
	if (m_bScenariosChanged)
	{
		m_pFlamMapAP->m_scenarioArray.Clear();
		for (int s = 0; s < m_scenarioArray.GetSize(); s++)
		{
			FMScenario *scn = m_scenarioArray.GetAt(s);
			FMScenario *newScn = new FMScenario(*scn);
			m_pFlamMapAP->m_scenarioArray.Add(newScn);
		}
		CString desc;
		for (int i = 0; i < (int)m_scenarioArray.GetSize(); i++)
		{
			CString temp;
			temp.Format("%i=%s", m_scenarioArray[i]->m_id, (LPCTSTR)m_scenarioArray[i]->m_name);
			desc += temp;
			if (i < (int)m_scenarioArray.GetSize() - 1)
				desc += ", ";
		}
		m_pFlamMapAP->m_inputVars.RemoveAll();
		m_pFlamMapAP->AddInputVar("Scenario ID", m_pFlamMapAP->m_scenarioID, desc);
	}
	if (m_bLCPGeneratorDirty)
	{
		m_pFlamMapAP->m_startingLCPFName = this->m_strStartingLCP;
		m_pFlamMapAP->m_vegClassLCPLookupFName = this->m_strVegclassLookup;
		m_pFlamMapAP->m_polyGridFName = this->m_strPGLFile;
		//need new polygrid lookup, lcp, and/or vegclass lookup
		int status = 0;
		if (!m_pFlamMapAP->CreateLCPGenerator())
		{
			status = -1;
			Report::Log(_T("An error occurred creating the LCPGenerator"));
			return;
		}
	}
	if (m_pFlamMapAP->m_outputEnvisionFirelists)
	{
		m_pFlamMapAP->m_outputEnvisionFirelistName.Format("%s%d_EnvisionFireList.csv", m_pFlamMapAP->m_outputPath, gpModel->processID);
		m_pFlamMapAP->m_outputEchoFirelistName.Format("%s%d_EchoFireList.csv", m_pFlamMapAP->m_outputPath, gpModel->processID);
		//m_outputEnvisionFirelistName += _T(".csv");
		FILE *envFireList = fopen(m_pFlamMapAP->m_outputEnvisionFirelistName, "wt");
		fprintf(envFireList, "Yr, Prob, Julian, BurnPer, WindSpeed, Azimuth, FMFile, IgnitionX, IgnitionY, Hectares, ERC, Original_Size, IgnitionFuel, IDU_HA_Burned, IDU_Proportion, FireID, Scenario, Run, ResultCode, fireID, EnvFire_ID, FIL1, FIL2, FIL3, FIL4, FIL5, FIL6, FIL7, FIL8, FIL9, FIL10, FIL11, FIL12, FIL13, FIL14, FIL15, FIL16, FIL17, FIL18, FIL19, FIL20\n");// Julian, WindSpeed, Azimuth, BurnPeriod, Hectares\n");
		fclose(envFireList);
	}
	if (m_pFlamMapAP->m_logDeltaArrayUpdates)
	{
		m_pFlamMapAP->m_outputDeltaArrayUpdatesName.Format("%s%d_FlamMapAPDeltaArrayUpdates.csv", m_pFlamMapAP->m_outputPath, gpModel->processID);
		//m_outputEnvisionFirelistName += _T(".csv");
		FILE *deltaArrayCSV = fopen(m_pFlamMapAP->m_outputDeltaArrayUpdatesName, "wt");
		fprintf(deltaArrayCSV, "Yr, Run, IDU, FireID, Flamelength, PFlameLen, CTSS, VegClass, Variant, DISTURB, FUELMODEL\n");
		fclose(deltaArrayCSV);
	}
	m_pFlamMapAP->m_replicateNum = 0;
	Report::Log(_T("  FlamMapAP settings changes complete"));

	m_bIsDirty = m_bScenariosChanged = m_bLCPGeneratorDirty = FALSE;
	//CDialog::OnOK();
}
void CFlamMapAPDlg::SelectScenario()
{
	m_ctrlFireListBox.ResetContent();
	if (m_comboScenarios.GetCount() <= 0)
		return;
	CString str;
	//m_comboScenarios.GetWindowText(str);
	//int selLoc = m_comboScenarios.GetCurSel();
	m_comboScenarios.GetLBText(m_comboScenarios.GetCurSel(), str);
	int scenarioID = atoi(str);
	for (int i = 0; i < m_scenarioArray.GetSize(); i++)
	{
		//tmpStr = m_pFlamMapAP->m_scenarioArray[i]->m_id + m_pFlamMapAP->m_scenarioArray[i]->m_name;
		if (m_scenarioArray[i]->m_id == scenarioID)
		{
			for (int f = 0; f < m_scenarioArray[i]->m_fireListArray.GetSize(); f++)
			{
				m_ctrlFireListBox.AddString(m_scenarioArray[i]->GetFireList(f)->m_path);
			}
		}
	}
}

#define MAX_CFileDialog_FILE_COUNT 99
#define FILE_LIST_BUFFER_SIZE ((MAX_CFileDialog_FILE_COUNT * (MAX_PATH + 1)) + 1)

void CFlamMapAPDlg::OnBnClickedButtonSelectFirelists()
{
	const int MF_BUFSIZE = 32768;
	UpdateData();
	CFileDialog fd(TRUE, "*.csv", 0, OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST | OFN_EXPLORER,
		"csv files (*.csv)|*.csv|text files (*.txt)|*.txt|All Files (*.*)|*.*||");
	char *mfBuf = new char[MF_BUFSIZE];
	mfBuf[0] = 0;
	fd.m_ofn.lpstrFile = mfBuf;
	fd.m_ofn.nMaxFile = MF_BUFSIZE;
	CString fileName, str;
	//char* p = fileName.GetBuffer(FILE_LIST_BUFFER_SIZE);
	//OPENFILENAME& ofn = fd.GetOFN();
	//ofn.Flags |= OFN_ALLOWMULTISELECT;
	//ofn.lpstrFile = p;
	//ofn.nMaxFile = FILE_LIST_BUFFER_SIZE;
	if (IDOK == fd.DoModal())
	{
		m_comboScenarios.GetLBText(m_comboScenarios.GetCurSel(), str);
		//m_comboScenarios.GetWindowText(str);
		int scenarioID = atoi(str), scenarioLoc = 0;
		for (int i = 0; i < m_scenarioArray.GetSize(); i++)
		{
			//tmpStr = m_pFlamMapAP->m_scenarioArray[i]->m_id + m_pFlamMapAP->m_scenarioArray[i]->m_name;
			if (m_scenarioArray[i]->m_id == scenarioID)
			{
				scenarioLoc = i;
				break;
			}
		}
		POSITION filenamePosition = fd.GetStartPosition();
		while (filenamePosition != NULL)
		{
			fileName = fd.GetNextPathName(filenamePosition);
			m_ctrlFireListBox.AddString(fileName);
			FIRELIST *fl = new FIRELIST(fileName);

			m_scenarioArray[scenarioLoc]->AddFireList(fl);
		}

		m_bScenariosChanged = TRUE;
		SelectScenario();
	}
	else
	{
		fileName.ReleaseBuffer();
	}
}


void CFlamMapAPDlg::OnSelchangeComboScenario()
{
	SelectScenario();
}


void CFlamMapAPDlg::OnBnClickedButtonDelete()
{
	if (m_ctrlFireListBox.GetCount() <= 0)
		return;
	int loc = m_ctrlFireListBox.GetCurSel();
		if (loc >= 0)
		{
			CString scnStr;
			m_comboScenarios.GetWindowText(scnStr);
			int id = atoi(scnStr);
			int s;
			FMScenario *pScenario = NULL;
			for (s = 0; s < m_scenarioArray.GetSize(); s++)
			{
				pScenario = m_scenarioArray.GetAt(s);
				if (pScenario->m_id == id)
					break;
			}
			CString selStr;
			m_ctrlFireListBox.GetText(m_ctrlFireListBox.GetCurSel(), selStr);
			for (int f = 0; f < pScenario->m_fireListArray.GetSize(); f++)
			{
				FIRELIST *fName = pScenario->m_fireListArray.GetAt(f);
				if (fName->m_path.Compare(selStr) == 0)
				{
					pScenario->m_fireListArray.RemoveAt(f);
					m_ctrlFireListBox.DeleteString(s);
				}
			}
			SelectScenario();
			m_bScenariosChanged = TRUE;
		}
}


void CFlamMapAPDlg::OnBnClickedButtonDeleteAll()
{
	if (m_ctrlFireListBox.GetCount() > 0)
	{
		CString scnStr, msg;
		m_comboScenarios.GetWindowText(scnStr);
		int id = atoi(scnStr);
		int s;
		FMScenario *pScenario = NULL;
		for (s = 0; s < m_scenarioArray.GetSize(); s++)
		{
			pScenario = m_scenarioArray.GetAt(s);
			if (pScenario->m_id == id)
				break;
		}
		msg.Format("Delete all fire lists for scenario %s?", scnStr);
		int ret = AfxMessageBox(msg, MB_YESNO);
		if (ret == IDYES)
		{
			pScenario->m_fireListArray.Clear();
			m_bScenariosChanged = TRUE;
			SelectScenario();
		}
	}

}


void CFlamMapAPDlg::OnBnClickedButtonNewScenario()
{
	CScenarioDlg dlg(this);
	dlg.m_id = (int) m_scenarioArray.GetSize();
	dlg.m_name = "New Scenario";
	dlg.m_pScenarioArray = &m_scenarioArray;
	dlg.selectedLoc = -1;
	int ret = (int) dlg.DoModal();
	if (ret == IDOK)
	{
		FMScenario * newScenario = new FMScenario;
		newScenario->m_id = dlg.m_id;
		newScenario->m_name = dlg.m_name;
		m_scenarioArray.Add(newScenario);
		CString str;
		if (dlg.m_id != 0)
			str.Format("%d - %s", dlg.m_id, dlg.m_name);
		else
			str.Format("%d (Default) - %s", dlg.m_id, dlg.m_name);
		int loc = m_comboScenarios.AddString(str);
		m_comboScenarios.SetCurSel(loc);
		SelectScenario();
		m_bScenariosChanged = TRUE;

	}
}


void CFlamMapAPDlg::OnBnClickedButtonDeleteScenario()
{
	if (m_comboScenarios.GetCount() <= 0)
		return;
	CString str, msg;
	m_comboScenarios.GetWindowText(str);
	msg.Format("Delete Scenario %s?", str);
	if (AfxMessageBox(msg, MB_YESNO) == IDYES)
	{
		int id = atoi(str);
		int s;
		FMScenario *pScenario = NULL;
		for (s = 0; s < m_scenarioArray.GetSize(); s++)
		{
			pScenario = m_scenarioArray.GetAt(s);
			if (pScenario->m_id == id)
				break;
		}
		m_scenarioArray.RemoveAt(s);
		m_bScenariosChanged = TRUE;
		m_comboScenarios.DeleteString(m_comboScenarios.GetCurSel());

		if (m_comboScenarios.GetCount() > 0)
		{
			m_comboScenarios.SetCurSel(0);
		}
		else
			m_comboScenarios.SetWindowText("");
		SelectScenario();

	}
}


void CFlamMapAPDlg::OnBnClickedButtonSaveAsXml()
{
	UpdateData();
	CFileDialog fd(FALSE, "xml", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT, "XML files (*.xml)|*.xml||");
	if (IDOK == fd.DoModal())
	{
		// open the file and write contents
		FILE *fp;
		fopen_s(&fp, fd.GetPathName(), "wt");
		if (fp == NULL)
		{
			LONG s_err = GetLastError();
			CString msg = "Failure to open " + fd.GetPathName() + " for writing.  ";
			Report::SystemErrorMsg(s_err, msg);
			return;
		}

		fputs("<?xml version='1.0' encoding='utf-8' ?>\n\n", fp);

		// settings
		fputs("<!--\n", fp);
		fputs("Initialization file for Flammap\n", fp);

		fputs("logFlameLengths: 1 = on, 0 = off - Output flame lengths from each run of FlamMap DLL\n", fp);
		fputs("outputPath : Path to directory where FlamMapAP outputs are written\n", fp);
		fputs("fmsPath : Path to fuel moisture files.\n", fp);
		fputs("polyGridFileName : Path and name of polyGridLookUp read file name.If it doesn't exist, it will be generatoed\n", fp);
		fputs("startingLCPFileName : Path and name of starting condition landscape file name\n", fp);
		fputs("vegClassLCPLookupFName : name of file for veg class to LCP conversions.\n", fp);
		fputs("vegClassHeightUnits : 1 = meters, 2 = feet, 3 = mx10, 4 = ftx10\n", fp);
		fputs("vegClassBulkDensUnits : 1 = kg / m ^ 3, 2 = lb / ft ^ 3, 3 = kg / m ^ 3 x 100, 4 = lb / ft ^ 3 x 1000\n", fp);
		fputs("vegClassCanopyCoverUnits : 0 = categories(0 - 4), 1 = percent\n", fp);
		fputs("spotProbability : Floating point, valid range : 0.0 - 1.0, probability a burning cell will launch a successful ember that returns to the landscape\n", fp);
		fputs("crownFireMethod : 0 = Finney, 1 = ScottRheinhart\n", fp);
		fputs("foliarMoistureContent : Integer, valid range 20 - 200, default = 100\n", fp);
		fputs("outputEnvisionFirelists : Integer, 0 = no Envision style firelists output, 1 = output Envision style firelist\n", fp);
		fputs("iduBurnPercentThreshold : Proportion of grid cells in an idu that must burn to consider the idu as burned\n", fp);
		fputs("outputIgnitionProbabilities : 0 = no output, 1 = output ignition probabilities to an ascii grid\n", fp);
		fputs("randomSeed : Seed to be used for FlamMapAP ignition number generators\n", fp);
		fputs("deleteLCPs : 0 (default) = do not delete generated lcp files, 1 = delete generated lcp files\n", fp);
		fputs("-\t-\t-\t-\t-\t-\t-\t-\t-\n", fp);
		fputs("useExistingFireList : filename of firelist - will duplicate fires from a previous Envision run(optional).\n", fp);

		fputs("The following switches are for creating a static FlamMap basic fire behavior run each year of the simulation under the static conditions.All three switches must be complete for this output to be produced.\n", fp);
		fputs("staticFMS : file to use for fuels moistures\n", fp);
		fputs("staticWindSpeed : windspeed(units ? )\n", fp);
		fputs("staticWindDirection : wind direction(degrees ? )\n", fp);

		fputs("-\t-\t-\t-\t-\t-\t-\t-\t-\n", fp);
		fputs("<firelist> tags define fires files(firesFName)\n", fp);
		fputs("-\t-\t-\t-\t-\t-\t-\t-\t-\n", fp);
		fputs("Note : Flammap incorporates Envision path searching, in the following order : 1) IDU directory, \n", fp);
		fputs("2) envx file directory, 3) Envision exe directory unless file pathis fully qualified\n", fp);


		fputs("Directory Structure\n", fp);
		fputs(" CentralOregon\\n", fp);
		fputs(" StudyArea\\n", fp);
		fputs("Flammap\\n", fp);
		fputs(" Outputs\\n", fp);
		fputs(" fmsFiles\\n", fp);
		fputs(" firelists\\n", fp);
		fputs(" Flammap.pgl\n", fp);
		fputs(" CO_Allocations.xml\n", fp);
		fputs(" idu.shp etc\n", fp);

		fputs("1) need to update FlammapAP code to reflect paths - jb\n", fp);
		fputs("2) clean up / propogate directory changes to SVN - jb\n", fp);

		fputs("-->\n\n", fp);


		fputs("<flammap\n", fp);
		fprintf(fp, "\tpolyGridFileName=\"%s\"\n", (LPCTSTR) m_strPGLFile);
		fprintf(fp, "\toutputPath=\"%s\"\n", (LPCTSTR)m_strOutputsPath);
		fprintf(fp, "\tfmsPath=\"%s\"\n", (LPCTSTR) m_strFMSPath);
		fprintf(fp, "\tlogFlameLengths=\"%d\"\n", m_bLogFlamelengths);
		fprintf(fp, "\tlogAnnualFlameLengths=\"%d\"\n", m_bLogAnnualFlameLengths);
		fprintf(fp, "\tstartingLCPFileName=\"%s\"\n", (LPCTSTR) m_strStartingLCP);
		fprintf(fp, "\tvegClassLCPLookupFileName=\"%s\"\n", (LPCTSTR) m_strVegclassLookup);
		fprintf(fp, "\tvegClassHeightUnits=\"%d\"\n", m_HeightUnits);
		fprintf(fp, "\tvegClassBulkDensUnits=\"%d\"\n", m_BulkDensityUnits);
		fprintf(fp, "\tvegClassCanopyCoverUnits=\"%d\"\n", m_CanopyCoverUnits);
		fprintf(fp, "\tspotProbability=\"%.4f\"\n", m_SpotProbability);
		fprintf(fp, "\tcrownFireMethod=\"%d\"\n", m_CrownFireMethod);
		fprintf(fp, "\tfoliarMoistureContent=\"%d\"\n", m_FoliarMoisture);
		fprintf(fp, "\toutputEnvisionFirelists=\"%d\"\n", m_bOutputFirelist);
		fprintf(fp, "\tlogDeltaArrayUpdates=\"%d\"\n", m_bLogDeltaArrayUpdates);
		fprintf(fp, "\tiduBurnPercentThreshold=\"%.4f\"\n", m_IDUBurnPcntThreshold);
		fprintf(fp, "\toutputIgnitionProbabilities=\"%d\"\n", m_bOutputIgnitionProbs);
		fprintf(fp, "\trandomSeed=\"%d\"\n", m_RandomSeed);
		fprintf(fp, "\tcustomFuelFileName=\"%s\"\n", (LPCTSTR) m_strFMDFile);
		fprintf(fp, "\tdeleteLCPs=\"%d\"\n", m_bDeleteLCPs);
		fprintf(fp, "\tstaticFMS=\"%s\"\n", (LPCTSTR) m_strStaticFMS);
		fprintf(fp, "\tstaticWindSpeed=\"%d\"\n", m_WindSpeed);
		fprintf(fp, "\tstaticWindDirection=\"%d\"\n", m_windDir);
		fprintf(fp, "\tlogArrivalTimes=\"%d\"\n", m_bLogArrivalTimes);
		fprintf(fp, "\tlogPerimeters=\"%d\"\n", m_bLogPerimeters);
		fprintf(fp, "\tminFireSizeRatio=\"%lf\"\n", m_MinFireSizeProportion);
		fprintf(fp, "\tmaxFireRetries=\"%d\"\n", m_maxRetries);
		fprintf(fp, "\tsequentialFirelists=\"%d\"\n", m_bSequentialFirelists);
		fputs(">\n", fp);

		//scenario time...
		fputs("\t<scenarios>\n\n", fp);
		for (int s = 0; s < m_scenarioArray.GetSize(); s++)
		{
			fprintf(fp, "\t\t<scenario id=\"%d\" name=\"%s\">\n", m_scenarioArray[s]->m_id, (LPCTSTR)m_scenarioArray[s]->m_name);
			for (int f = 0; f < m_scenarioArray[s]->m_fireListArray.GetSize(); f++)
			{
				fprintf(fp, "\t\t\t<firelist name=\"%s\" />\n", (LPCTSTR) m_scenarioArray[s]->GetFireList(f)->m_path);
			}
			fputs("\t\t</scenario>\n\n", fp);
		}
		fputs("\t</scenarios>\n\n", fp);
		fputs("</flammap>\n\n", fp);
		fclose(fp);
	}
}


void CFlamMapAPDlg::OnBnClickedButtonLoadXml()
{
}


void CFlamMapAPDlg::OnEnChangeMfceditbrowseStartingLcp()
{
	m_bLCPGeneratorDirty = true;
}


void CFlamMapAPDlg::OnEnChangeMfceditbrowsePglFile()
{
	m_bLCPGeneratorDirty = true;
}


void CFlamMapAPDlg::OnEnChangeMfceditbrowseVegclassLookup()
{
	m_bLCPGeneratorDirty = true;
}


void CFlamMapAPDlg::OnBnClickedButtonEditScenario()
{
	if (m_comboScenarios.GetCount() <= 0)
		return;
	CScenarioDlg dlg(this);
	CString scnStr;
	m_comboScenarios.GetWindowText(scnStr);
	int id = atoi(scnStr);
	//find the scenario entry
	int s;
	FMScenario *pScenario = NULL;
	for (s = 0; s < m_scenarioArray.GetSize(); s++)
	{
		pScenario = m_scenarioArray.GetAt(s);
		if (pScenario->m_id == id)
			break;
	}
	dlg.m_id = pScenario->m_id;
	dlg.m_name = pScenario->m_name;
	dlg.m_pScenarioArray = &m_scenarioArray;
	dlg.selectedLoc = s;
	int ret = (int) dlg.DoModal();
	if (ret == IDOK)
	{
		pScenario->m_id = dlg.m_id;
		pScenario->m_name = dlg.m_name;
		CString str;
		if (dlg.m_id != 0)
			str.Format("%d - %s", dlg.m_id, dlg.m_name);
		else
			str.Format("%d (Default) - %s", dlg.m_id, dlg.m_name);
		int loc = m_comboScenarios.GetCurSel();
		m_comboScenarios.DeleteString(loc);
		loc = m_comboScenarios.AddString(str);
		m_comboScenarios.SetCurSel(loc);
		m_bScenariosChanged = TRUE;
	}
}


void CFlamMapAPDlg::OnBnClickedCheckLogDeltaArrayUpdates()
{
	// TODO: Add your control notification handler code here
}


void CFlamMapAPDlg::OnBnClickedCheckLogannualflamelengths()
{
	// TODO: Add your control notification handler code here
}
