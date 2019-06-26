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
#pragma once
#include "resource.h"
#include "afxeditbrowsectrl.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "afxshelllistctrl.h"

// CFlamMapAPDlg dialog
class CustomEditBrowseCtrl : public CMFCEditBrowseCtrl
{
public:
	CustomEditBrowseCtrl();
	void OnBrowse();
	CString topDir;
};

class CFlamMapAPDlg : public CDialog
{
	DECLARE_DYNAMIC(CFlamMapAPDlg)

public:
	CFlamMapAPDlg(FlamMapAP *pFlamMapAP, MapLayer *pLayer, CWnd* pParent = NULL);   // standard constructor
	virtual ~CFlamMapAPDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_FLAMMAPAP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CustomEditBrowseCtrl m_ctrlOutputsPath;
	CString m_strOutputsPath;
	virtual BOOL OnInitDialog();
private:
	void SelectScenario();
	FlamMapAP *m_pFlamMapAP;
public:
	CString m_strPGLFile;
	CString m_strFMSPath;
	CString m_strStartingLCP;
	CString m_strVegclassLookup;
	CString m_strFMDFile;
	CString m_strConfigXML;
	BOOL m_bLogFlamelengths;
	BOOL m_bOutputFirelist;
	BOOL m_bOutputIgnitionProbs;
	BOOL m_bDeleteLCPs;
	BOOL m_bLogArrivalTimes;
	BOOL m_bLogPerimeters;
	virtual void OnOK();
//	CustomEditBrowseCtrl m_ctrlConfigFile;
	CustomEditBrowseCtrl m_ctrlStartingLCP;
	CustomEditBrowseCtrl m_ctrlPGL;
	CustomEditBrowseCtrl m_ctrlVegClass;
	CustomEditBrowseCtrl m_ctrlFMD;
	CustomEditBrowseCtrl m_ctrlFMS;
	int m_WindSpeed;
	int m_windDir;
	CString m_strStaticFMS;
	CustomEditBrowseCtrl m_ctrlStaticFMS;
	CComboBox m_comboScenarios;
	//CListCtrl m_ctrlFiresList;
	CListBox m_ctrlFireListBox;
	afx_msg void OnBnClickedButtonSelectFirelists();
	CButton m_btnSelectFirelists;
	afx_msg void OnSelchangeComboScenario();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonDeleteAll();
	int m_HeightUnits;
	int m_BulkDensityUnits;
	int m_CanopyCoverUnits;
	afx_msg void OnBnClickedButtonNewScenario();
	afx_msg void OnBnClickedButtonDeleteScenario();
	int m_FoliarMoisture;
	int m_CrownFireMethod;
	int m_RandomSeed;
	double m_SpotProbability;
	PtrArray< FMScenario > m_scenarioArray;

	afx_msg void OnBnClickedButtonSaveAsXml();
	afx_msg void OnBnClickedButtonLoadXml();
	BOOL m_UseFirelistIgnitions;
	double m_IDUBurnPcntThreshold;
	BOOL m_bIsDirty;
	BOOL m_bScenariosChanged;
	BOOL m_bLCPGeneratorDirty;
	afx_msg void OnEnChangeMfceditbrowseStartingLcp();
	afx_msg void OnEnChangeMfceditbrowsePglFile();
	afx_msg void OnEnChangeMfceditbrowseVegclassLookup();
	afx_msg void OnBnClickedButtonEditScenario();
	BOOL m_bLogDeltaArrayUpdates;
	afx_msg void OnBnClickedCheckLogDeltaArrayUpdates();
	BOOL m_bLogAnnualFlameLengths;
	afx_msg void OnBnClickedCheckLogannualflamelengths();
	double m_MinFireSizeProportion;
	int m_maxRetries;
	BOOL m_bSequentialFirelists;
};
