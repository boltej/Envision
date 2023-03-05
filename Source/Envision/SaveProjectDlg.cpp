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
// SaveProjectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "SaveProjectDlg.h"
#include "afxdialogex.h"

#include "EnvDoc.h"
#include <LULCTREE.H>
#include <EnvPolicy.h>
#include <Scenario.h>

#include <DirPlaceholder.h>


extern CEnvDoc *gpDoc;
extern ScenarioManager *gpScenarioManager;
extern PolicyManager   *gpPolicyManager;


// SaveProjectDlg dialog

IMPLEMENT_DYNAMIC(SaveProjectDlg, CDialogEx)

SaveProjectDlg::SaveProjectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(SaveProjectDlg::IDD, pParent)
   , m_projectPath(_T(""))
   , m_lulcPath(_T(""))
   , m_policyPath(_T(""))
   , m_scenarioPath(_T(""))
   , m_lulcNoSave(FALSE)
   , m_policyNoSave(FALSE)
   , m_scenarioNoSave(FALSE)
   {
m_projectPath = gpDoc->m_iniFile;
m_lulcPath = gpDoc->m_model.m_lulcTree.m_importPath;
m_policyPath = gpPolicyManager->m_importPath;
m_scenarioPath = gpScenarioManager->m_importPath;

m_lulcNoSave = gpDoc->m_model.m_lulcTree.m_includeInSave ? false : true;
m_policyNoSave = gpPolicyManager->m_includeInSave ? false : true;
m_scenarioNoSave = gpScenarioManager->m_includeInSave ? false : true;
}

SaveProjectDlg::~SaveProjectDlg()
{
}

void SaveProjectDlg::DoDataExchange(CDataExchange* pDX)
{
CDialogEx::DoDataExchange(pDX);
DDX_Text(pDX, IDC_ENVXFILE, m_projectPath);
DDX_Text(pDX, IDC_LULCFILE, m_lulcPath);
DDX_Text(pDX, IDC_POLICYFILE, m_policyPath);
DDX_Text(pDX, IDC_SCENARIOFILE, m_scenarioPath);

DDX_Control(pDX, IDC_LULC_NOSAVE, m_cbLulcNoSave);
DDX_Check(pDX, IDC_LULC_NOSAVE, m_lulcNoSave);

DDX_Control(pDX, IDC_POLICY_NOSAVE, m_cbPolicyNoSave);
DDX_Check(pDX, IDC_POLICY_NOSAVE, m_policyNoSave);

DDX_Control(pDX, IDC_SCENARIO_NOSAVE, m_cbScenarioNoSave);
DDX_Check(pDX, IDC_SCENARIO_NOSAVE, m_scenarioNoSave);
}


BEGIN_MESSAGE_MAP(SaveProjectDlg, CDialogEx)
   ON_BN_CLICKED(IDC_BROWSEENVX, &SaveProjectDlg::OnBnClickedBrowseenvx)
   ON_BN_CLICKED(IDC_BROWSELULC, &SaveProjectDlg::OnBnClickedBrowselulc)
   ON_BN_CLICKED(IDC_BROWSEPOLICY, &SaveProjectDlg::OnBnClickedBrowsepolicy)
   ON_BN_CLICKED(IDC_BROWSESCENARIO, &SaveProjectDlg::OnBnClickedBrowsescenario)
   ON_BN_CLICKED(IDC_LULC_USEENVX, &SaveProjectDlg::UpdateUI)
   ON_BN_CLICKED(IDC_LULC_USEXML,  &SaveProjectDlg::UpdateUI)
   ON_BN_CLICKED(IDC_POLICY_USEENVX, &SaveProjectDlg::UpdateUI)
   ON_BN_CLICKED(IDC_POLICY_USEXML,  &SaveProjectDlg::UpdateUI)
   ON_BN_CLICKED(IDC_SCENARIO_USEENVX, &SaveProjectDlg::UpdateUI)
   ON_BN_CLICKED(IDC_SCENARIO_USEXML , &SaveProjectDlg::UpdateUI)
END_MESSAGE_MAP()


// SaveProjectDlg message handlers

BOOL SaveProjectDlg::OnInitDialog()
   {
   CDialogEx::OnInitDialog();

   if ( gpDoc->m_model.m_lulcTree.m_loadStatus == 1 )
      CheckDlgButton( IDC_LULC_USEXML, 1 );
   else
      CheckDlgButton( IDC_LULC_USEENVX, 1 );

   if ( gpPolicyManager->m_loadStatus == 1 )
      CheckDlgButton( IDC_POLICY_USEXML, 1 );
   else
      CheckDlgButton( IDC_POLICY_USEENVX, 1 );

   if ( gpScenarioManager->m_loadStatus == 1 )
      CheckDlgButton( IDC_SCENARIO_USEXML, 1 );
   else
      CheckDlgButton( IDC_SCENARIO_USEENVX, 1 );

   UpdateUI();
   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void SaveProjectDlg::UpdateUI( void )
   {
   BOOL isExternal = IsDlgButtonChecked( IDC_LULC_USEXML );
   GetDlgItem( IDC_LULC_NOSAVE )->EnableWindow( isExternal );
   GetDlgItem( IDC_LULCFILE    )->EnableWindow( isExternal );
   GetDlgItem( IDC_BROWSELULC  )->EnableWindow( isExternal );

   isExternal = IsDlgButtonChecked( IDC_POLICY_USEXML );
   GetDlgItem( IDC_POLICY_NOSAVE )->EnableWindow( isExternal );
   GetDlgItem( IDC_POLICYFILE    )->EnableWindow( isExternal );
   GetDlgItem( IDC_BROWSEPOLICY  )->EnableWindow( isExternal );

   isExternal = IsDlgButtonChecked( IDC_SCENARIO_USEXML );
   GetDlgItem( IDC_SCENARIO_NOSAVE )->EnableWindow( isExternal );
   GetDlgItem( IDC_SCENARIOFILE    )->EnableWindow( isExternal );
   GetDlgItem( IDC_BROWSESCENARIO  )->EnableWindow( isExternal );
   }


void SaveProjectDlg::OnBnClickedBrowseenvx()
   {
   UpdateData( 1 );

   DirPlaceholder d;

   CFileDialog dlg( TRUE, "envx", m_projectPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Envx files|*.envx|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_projectPath = dlg.GetPathName();
      UpdateData( 0 );
      }   
   }


void SaveProjectDlg::OnBnClickedBrowselulc()
   {
   UpdateData( 1 );

   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", m_lulcPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_lulcPath = dlg.GetPathName();
      UpdateData( 0 );
      }   
   }


void SaveProjectDlg::OnBnClickedBrowsepolicy()
   {
   UpdateData( 1 );

   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", m_policyPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_policyPath = dlg.GetPathName();
      UpdateData( 0 );
      }   
   }


void SaveProjectDlg::OnBnClickedBrowsescenario()
   {
   UpdateData( 1 );

   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", m_scenarioPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_scenarioPath = dlg.GetPathName();
      UpdateData( 0 );
      }   
   }

void SaveProjectDlg::OnOK()
   {
   UpdateData( 1 );

   // lulc
   if ( IsDlgButtonChecked( IDC_LULC_USEXML ) )
      {
      gpDoc->m_model.m_lulcTree.m_loadStatus = 1;
      gpDoc->m_model.m_lulcTree.m_importPath = m_lulcPath;

      gpDoc->m_model.m_lulcTree.m_includeInSave = ! m_lulcNoSave;
      }
   else
      gpDoc->m_model.m_lulcTree.m_loadStatus = 0;

   // policies
   if ( IsDlgButtonChecked( IDC_POLICY_USEXML ) )
      {
      gpPolicyManager->m_loadStatus = 1;
      gpPolicyManager->m_importPath = m_policyPath;

      gpPolicyManager->m_includeInSave = ! m_policyNoSave;
      }
   else
      gpPolicyManager->m_loadStatus = 0;

   // scenarios
   if ( IsDlgButtonChecked( IDC_SCENARIO_USEXML ) )
      {
      gpScenarioManager->m_loadStatus = 1;
      gpScenarioManager->m_importPath = m_scenarioPath;

      gpScenarioManager->m_includeInSave = ! m_scenarioNoSave;
      }
   else
      gpScenarioManager->m_loadStatus = 0;

   gpDoc->SaveAs( m_projectPath );
   
   CDialogEx::OnOK();
   }


