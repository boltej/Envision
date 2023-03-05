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
// PPPolicyScenarios.cpp : implementation file
//

#include "stdafx.h"

#include ".\pppolicyscenarios.h"

#include <EnvPolicy.h>
#include <Scenario.h>
#include "PolEditor.h"
#include "outcomeEditor.h"

extern PolicyManager   *gpPolicyManager;
extern ScenarioManager *gpScenarioManager;


// PPPolicyScenarios dialog

IMPLEMENT_DYNAMIC(PPPolicyScenarios, CTabPageSSL)

PPPolicyScenarios::PPPolicyScenarios( PolEditor *pParent, EnvPolicy *&pPolicy )
	: CTabPageSSL()
   , m_pPolicy( pPolicy )
   , m_pParent( pParent )
{ }

PPPolicyScenarios::~PPPolicyScenarios()
{ }


void PPPolicyScenarios::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
DDX_Control(pDX, IDC_SCENARIOS, m_scenarios);
}


BEGIN_MESSAGE_MAP(PPPolicyScenarios, CTabPageSSL)
   ON_CLBN_CHKCHANGE(IDC_SCENARIOS, &PPPolicyScenarios::OnCheckScenario)   
END_MESSAGE_MAP()


// PPPolicyScenarios message handlers


BOOL PPPolicyScenarios::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();

   m_scenarios.ResetContent();

   for ( int i=0; i < (int) gpScenarioManager->GetCount(); i++ )
      m_scenarios.AddString( gpScenarioManager->GetScenario( i )->m_name );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void PPPolicyScenarios::LoadPolicy()
   {
   if ( this->m_pPolicy == NULL )
      return;

   for ( int i=0; i < (int) gpScenarioManager->GetCount(); i++ )
      {
      Scenario *pScenario =  gpScenarioManager->GetScenario( i );

      BOOL use = pScenario->GetPolicyInfo( m_pPolicy->m_name )->inUse ? 1 : 0;

      m_scenarios.SetCheck( i, use );
      }     
   }


// make sure any changes are stored in the policy
bool PPPolicyScenarios::StoreChanges()
   {
   if ( m_pParent->IsDirty( DIRTY_SCENARIOS ) )
      {      
      for ( int i=0; i < (int) gpScenarioManager->GetCount(); i++ )
         {
         Scenario *pScenario =  gpScenarioManager->GetScenario( i );

         BOOL use = m_scenarios.GetCheck( i );

         pScenario->GetPolicyInfo( m_pPolicy->m_name )->inUse = use ? true : false;
         }
      }

   m_pParent->MakeClean( DIRTY_SCENARIOS );   

   return true;
   }

void PPPolicyScenarios::OnCheckScenario()
   {
   m_pParent->MakeDirty( DIRTY_SCENARIOS );
   }