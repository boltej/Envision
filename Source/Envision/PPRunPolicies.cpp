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
// PPRunPolicies.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include <EnvPolicy.h>
#include ".\pprunpolicies.h"
#include "EnvDoc.h"

extern CEnvDoc       *gpDoc;
extern PolicyManager *gpPolicyManager;


// PPRunPolicies dialog

IMPLEMENT_DYNAMIC(PPRunPolicies, CPropertyPage)
PPRunPolicies::PPRunPolicies()
	: CPropertyPage(PPRunPolicies::IDD)
{
}

PPRunPolicies::~PPRunPolicies()
{
}

void PPRunPolicies::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_POLICIES, m_policies);
}


BEGIN_MESSAGE_MAP(PPRunPolicies, CPropertyPage)
   ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
   ON_BN_CLICKED(IDC_SELECTNONE, OnBnClickedSelectnone)
   ON_BN_CLICKED(IDC_SHOWMYPOLICIES, OnBnClickedShowmypolicies)
   ON_BN_CLICKED(IDC_SHOWSHAREDPOLICIES, OnBnClickedShowsharedpolicies)
END_MESSAGE_MAP()


// PPRunPolicies message handlers

BOOL PPRunPolicies::OnInitDialog()
   {
   CPropertyPage::OnInitDialog();

   CheckDlgButton( IDC_SHOWMYPOLICIES, 1 );
   CheckDlgButton( IDC_SHOWSHAREDPOLICIES, 1 );
   LoadPolicies();

   return TRUE;
   }


void PPRunPolicies::LoadPolicies()
   {
   m_policies.ResetContent();

   int count = gpPolicyManager->GetPolicyCount();
   for ( int i=0; i < count; i++ )
      {
      EnvPolicy *pPolicy = gpPolicyManager->GetPolicy( i );
      
      // only show if its an shared policy or one of my policies
      if ( ShowPolicy( pPolicy ) )
         {
         int index = m_policies.AddString( pPolicy->m_name );
         m_policies.SetItemData( index, (DWORD_PTR) pPolicy );

         if ( pPolicy->m_use )
            m_policies.SetCheck( index, TRUE );
         else
            m_policies.SetCheck( index, FALSE );
         }
      }

   m_policies.SetCurSel( 0 );

   return;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


int PPRunPolicies::SetPoliciesToUse( void )
   {
   int count = m_policies.GetCount();

   int countInUse = 0;

   for ( int i=0; i < count; i++ )
      {
      EnvPolicy *pPolicy = (EnvPolicy*) m_policies.GetItemData( i );
      if ( m_policies.GetCheck( i ) )
         {
         pPolicy->m_use = true;
         countInUse++;
         }
      else
         pPolicy->m_use = false;        
      }

   return countInUse;
   }

void PPRunPolicies::OnBnClickedSelectall()
   {
   int count = m_policies.GetCount();

   for ( int i=0; i<count; i++ )
      m_policies.SetCheck( i, TRUE );
   }

void PPRunPolicies::OnBnClickedSelectnone()
   {
   int count = m_policies.GetCount();

   for ( int i=0; i<count; i++ )
      m_policies.SetCheck( i, FALSE );
   }


bool PPRunPolicies::ShowPolicy( EnvPolicy *pPolicy )
   {
   bool showMyPolicies     = IsDlgButtonChecked( IDC_SHOWMYPOLICIES ) ? true : false;
   bool showSharedPolicies = IsDlgButtonChecked( IDC_SHOWSHAREDPOLICIES ) ? true : false;
   bool showPolicy = false;

   if ( pPolicy->m_isShared && showSharedPolicies ) 
      showPolicy = true;
   if ( pPolicy->m_originator.CompareNoCase( gpDoc->m_userName ) == 0 && showMyPolicies )
       showPolicy = true;

   return showPolicy;
   }


void PPRunPolicies::OnBnClickedShowmypolicies()
   {
   LoadPolicies();
   }


void PPRunPolicies::OnBnClickedShowsharedpolicies()
   {
   LoadPolicies();
   }
