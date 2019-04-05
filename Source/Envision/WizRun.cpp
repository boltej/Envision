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
// WizRun.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "WizRun.h"
#include "EnvDoc.h"
#include <EnvModel.h>
#include <DataManager.h>
#include <Scenario.h>
#include <maplayer.h>
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

WizRun    *gpWizRun = NULL;

extern MapLayer        *gpCellLayer;
extern CEnvDoc         *gpDoc;
extern EnvModel        *gpModel;
extern ScenarioManager *gpScenarioManager;

/////////////////////////////////////////////////////////////////////////////
// WizRun

IMPLEMENT_DYNAMIC(WizRun, CPropertySheet)

WizRun::WizRun(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}


WizRun::WizRun(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
   {
   gpWizRun = this;

   m_runState = 0;   // setuponly by default
   
   // set up alternatives
   //m_mainPage.m_years          = gpDoc->m_model.m_yearsToRun;
   m_mainPage.m_dynamicUpdateViews = gpDoc->m_model.m_dynamicUpdate & 1;
   m_mainPage.m_dynamicUpdateMap   = gpDoc->m_model.m_dynamicUpdate & 2;   
   m_mainPage.m_iterations     = gpDoc->m_model.m_iterationsToRun;
   m_mainPage.m_psp.dwFlags   |= PSP_PREMATURE;
   //m_mainPage.m_evalFrequency  = gpDoc->m_model.m_evalFreq;
   m_mainPage.m_useInitialSeed = gpDoc->m_model.m_resetInfo.useInitialSeed ? TRUE : FALSE;
   m_mainPage.m_exportMap      = gpDoc->m_model.m_exportMapInterval > 0 ? 1 : 0;
   m_mainPage.m_exportInterval = gpDoc->m_model.m_exportMapInterval;

   m_mainPage.m_exportBmps     = gpDoc->m_model.m_exportBmps;
   m_mainPage.m_exportOutputs  = gpDoc->m_model.m_exportOutputs;


   // setup page data
   //m_setupPage.m_culturalEnabled   = gpDoc->m_model.m_actorUpdateEnabled;
   //m_setupPage.m_culturalFrequency = gpDoc->m_model.m_actorUpdateFreq;
   //m_setupPage.m_policyEnabled     = gpDoc->m_model.m_policyUpdateEnabled;
   //m_setupPage.m_policyFrequency   = gpDoc->m_model.m_policyUpdateFreq;
   m_setupPage.m_psp.dwFlags |= PSP_PREMATURE;

   // data page
   m_dataPage.m_collectActorData = gpModel->m_pDataManager->m_collectActorData;
   m_dataPage.m_collectLandscapeScoreData = gpModel->m_pDataManager->m_collectLandscapeScoreData;
   m_dataPage.m_psp.dwFlags |= PSP_PREMATURE;

   // policies page
   //m_policiesPage.m_psp.dwFlags |= PSP_PREMATURE;

   // actors page
   //m_actorsPage.m_psp.dwFlags |= PSP_PREMATURE;

   // metagoals page
   //m_metagoalsPage.m_altruismWt = gpDoc->m_model.m_altruismWt;
   //m_metagoalsPage.m_selfInterestWt = gpDoc->m_model.m_selfInterestWt;
   m_metagoalsPage.m_policyPrefWt = gpDoc->m_model.m_policyPrefWt;
   m_metagoalsPage.m_psp.dwFlags |= PSP_PREMATURE;

   AddPage( &m_mainPage );
   AddPage( &m_setupPage );
   AddPage( &m_dataPage );
   //AddPage( &m_policiesPage );
   //AddPage( &m_actorsPage );
   AddPage( &m_metagoalsPage );

   //m_psh.dwFlags |= PSH_NOAPPLYNOW;
   //SetWizardMode();
   }



BEGIN_MESSAGE_MAP(WizRun, CPropertySheet)
	//{{AFX_MSG_MAP(WizRun)
	//}}AFX_MSG_MAP
   ON_COMMAND (ID_APPLY_NOW, OnApplyNow)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// WizRun message handlers

BOOL WizRun::OnInitDialog() 
   {
	BOOL retVal = CPropertySheet::OnInitDialog();

   m_mainPage.SetModified( TRUE );

   // hide the ok button
   CWnd *pOK = GetDlgItem( IDOK );

   if ( pOK )
      pOK->ShowWindow( SW_HIDE );

   CWnd *pApply = GetDlgItem( ID_APPLY_NOW );

   return retVal;
   }


int WizRun::SaveSetup()
   {
   // collect information from forms
   m_mainPage.UpdateData( TRUE );
   m_setupPage.UpdateData( TRUE );
   m_dataPage.UpdateData( TRUE );
   //m_policiesPage.UpdateData( TRUE );
   //m_actorsPage.UpdateData( TRUE );
   m_metagoalsPage.UpdateData( TRUE );

   // retrieve data - main page
   //gpDoc->m_model.m_yearsToRun = m_mainPage.m_years;

   int dynamicUpdate = 0;
   if ( m_mainPage.m_dynamicUpdateViews ) dynamicUpdate = 1;
   if ( m_mainPage.m_dynamicUpdateMap   ) dynamicUpdate += 2;
  // hack...
   if ( dynamicUpdate > 0 )
      {
      dynamicUpdate += 4;  // years
      dynamicUpdate += 8;  // process name
      }

   gpDoc->m_model.m_dynamicUpdate = dynamicUpdate;

   gpDoc->m_model.m_iterationsToRun = m_mainPage.m_iterations;
   //gpDoc->m_model.m_evalFreq        = m_mainPage.m_evalFrequency;
   gpDoc->m_model.m_resetInfo.useInitialSeed = m_mainPage.m_useInitialSeed ? true : false;

//   int scenario = m_mainPage.m_scenarios.GetCurSel();
//   if ( scenario >= 0 )
//      gpDoc->m_model.SetScenario( gpScenarioManager->GetScenario( scenario ) );
//   else
//      gpDoc->m_model.SetScenario( NULL );

   if ( m_mainPage.m_exportMap )
      gpDoc->m_model.m_exportMapInterval = -1;
   else
      gpDoc->m_model.m_exportMapInterval = m_mainPage.m_exportInterval;

   ///gpView->m_outputTab.m_scenarioTab.SetCurScenario( scenario );

   // setup page data
   //gpDoc->m_model.m_actorUpdateEnabled = m_setupPage.m_culturalEnabled;
   //gpDoc->m_model.m_actorUpdateFreq = m_setupPage.m_culturalFrequency;
   //gpDoc->m_model.m_policyUpdateEnabled = m_setupPage.m_policyEnabled;
   //gpDoc->m_model.m_policyUpdateFreq = m_setupPage.m_policyFrequency;

   m_setupPage.SetModelsToUse();

   // data page
   gpModel->m_pDataManager->m_collectActorData = m_dataPage.m_collectActorData ? true : false;
   gpModel->m_pDataManager->m_collectLandscapeScoreData = m_dataPage.m_collectLandscapeScoreData ? true : false;

   // policies page
   //m_policiesPage.SetPoliciesToUse();

   // metagoals page
   for ( int i=0; i < gpDoc->m_model.GetMetagoalCount(); i++ )
      {
      float wt = m_metagoalsPage.GetGoalWeight( i );

      METAGOAL *pGoal = gpDoc->m_model.GetMetagoalInfo( i );
      pGoal->weight = wt;
      }

   //gpDoc->m_model.m_altruismWt = m_metagoalsPage.m_altruismWt;
   //gpDoc->m_model.m_selfInterestWt = m_metagoalsPage.m_selfInterestWt;
   gpDoc->m_model.m_policyPrefWt = m_metagoalsPage.m_policyPrefWt;
   
   EndDialog( IDOK );
   return 1;
   }


void WizRun::OnApplyNow() 
   {
	SaveSetup();
   }
