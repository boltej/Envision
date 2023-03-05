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
// PPIniBasic.cpp : implementation file
//

#include "stdafx.h"
#include "PPIniBasic.h"

#include "envdoc.h"
#include <Scenario.h>
#include <DataManager.h>
#include "IniFileEditor.h"
#include <Actor.h>
#include "ActorDlg.h"

#include <EnvPolicy.h>
#include "PolEditor.h"
#include "ScenarioEditor.h"
#include "LulcEditor.h"

#include <DirPlaceholder.h>

extern ScenarioManager *gpScenarioManager;
extern PolicyManager   *gpPolicyManager;
extern CEnvDoc         *gpDoc;
extern ActorManager    *gpActorManager;


// PPIniBasic dialog

IMPLEMENT_DYNAMIC(PPIniBasic, CTabPageSSL)

PPIniBasic::PPIniBasic(IniFileEditor* pParent /*=NULL*/)
	: CTabPageSSL(PPIniBasic::IDD, pParent)
   , m_pParent( pParent )
{ }


PPIniBasic::~PPIniBasic()
{ }

void PPIniBasic::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(PPIniBasic, CTabPageSSL)
   ON_BN_CLICKED(IDC_BROWSEDATABASE, &PPIniBasic::OnBnClickedImportPolicies)
   ON_BN_CLICKED(IDC_BROWSESCENARIOS, &PPIniBasic::OnBnClickedImportScenarios)
   ON_BN_CLICKED(IDC_BROWSEACTORS, &PPIniBasic::OnBnClickedImportActors)
   ON_BN_CLICKED(IDC_BROWSELULCTREE, &PPIniBasic::OnBnClickedImportLulcTree)
   ON_BN_CLICKED(IDC_RUNPOLICYEDITOR, &PPIniBasic::OnBnClickedRunpolicyeditor)
   ON_BN_CLICKED(IDC_RUNSCENARIOEDITOR, &PPIniBasic::OnBnClickedRunscenarioeditor)
   ON_BN_CLICKED(IDC_RUNACTOREDITOR, &PPIniBasic::OnBnClickedRunactoreditor)
   ON_BN_CLICKED(IDC_RUNLULCEDITOR, &PPIniBasic::OnBnClickedRunlulceditor)
END_MESSAGE_MAP()


// PPIniBasic message handlers

BOOL PPIniBasic::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();

   UpdateCounts();

   UpdateData( 0 );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void PPIniBasic::UpdateCounts()
   {
   TCHAR buffer[ 128 ];

   sprintf_s( buffer, 128, "There are currently %i policies defined for this project.", gpPolicyManager->GetPolicyCount() );
   GetDlgItem( IDC_POLICIES )->SetWindowText( buffer );

   sprintf_s( buffer, 128, "There are currently %i actor groups defined for this project.", gpActorManager->GetActorGroupCount() );
   GetDlgItem( IDC_ACTORS )->SetWindowText( buffer );


   sprintf_s( buffer, 128, "There are currently %i scenarios defined for this project.", gpScenarioManager->GetCount() );
   GetDlgItem( IDC_SCENARIOS )->SetWindowText( buffer );

   int lulcLevels = gpDoc->m_model.m_lulcTree.GetLevels();
   int lulcCount = 0;
   for ( int i=1; i <= lulcLevels; i++ )
      lulcCount += gpDoc->m_model.m_lulcTree.GetNodeCount( i );

   sprintf_s( buffer, 128, "There are currently %i levels and %i land use/land cover classes defined for this project.", lulcLevels, lulcCount );
   GetDlgItem( IDC_LULCTREE )->SetWindowText( buffer );
   }


void PPIniBasic::MakeDirty( void ) { m_pParent->MakeDirty( INI_BASIC ); }


bool PPIniBasic::StoreChanges()
   {
   UpdateData( 1 );
   return true;
   }


void PPIniBasic::OnBnClickedImportPolicies()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Xml files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      gpPolicyManager->LoadXml( dlg.GetPathName(), true, false );
      UpdateCounts();
      MakeDirty();
      }
   }


void PPIniBasic::OnBnClickedImportActors()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Xml files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      gpActorManager->LoadXml( dlg.GetPathName(), true, false );
      UpdateCounts();
      MakeDirty();
      }
   }


void PPIniBasic::OnBnClickedImportScenarios()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Xml files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      gpScenarioManager->LoadXml( dlg.GetPathName(), true, false );
      UpdateCounts();
      MakeDirty();
      }
   }

void PPIniBasic::OnBnClickedImportLulcTree()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Xml files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      gpDoc->m_model.m_lulcTree.LoadXml( dlg.GetPathName(), true, false );
      UpdateCounts();
      MakeDirty();
      }
   }


void PPIniBasic::OnBnClickedRunpolicyeditor()
   {
   PolEditor dlg;
   if ( dlg.DoModal() == IDOK )
      {
      UpdateCounts();
      MakeDirty();
      }
   }

void PPIniBasic::OnBnClickedRunactoreditor()
   {
   ActorDlg dlg;
   if ( dlg.DoModal() == IDOK )
      {
      UpdateCounts();
      MakeDirty();
      }
   }

void PPIniBasic::OnBnClickedRunscenarioeditor()
   {
   ScenarioEditor dlg;
   if ( dlg.DoModal() == IDOK )
      {
      UpdateCounts();
      MakeDirty();
      }
   }

void PPIniBasic::OnBnClickedRunlulceditor()
   {
   LulcEditor dlg;
   if ( dlg.DoModal() == IDOK )
      {
      UpdateCounts();
      MakeDirty();
      }
   }
