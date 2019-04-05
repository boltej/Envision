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
// PPRunSetup.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PPRunSetup.h"
#include "EnvDoc.h"

extern CEnvDoc *gpDoc;
extern EnvModel *gpModel;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// PPRunSetup property page

IMPLEMENT_DYNCREATE(PPRunSetup, CPropertyPage)

PPRunSetup::PPRunSetup() : CPropertyPage(PPRunSetup::IDD)
//, m_culturalFrequency(0)
//, m_policyFrequency(0)
//, m_evalFrequency(0)
   {
   }

PPRunSetup::~PPRunSetup()
{
}

void PPRunSetup::DoDataExchange(CDataExchange* pDX)
{
//CPropertyPage::DoDataExchange(pDX);
//DDX_Text(pDX, IDC_CULTURALFREQUENCY, m_culturalFrequency);
//DDV_MinMaxInt(pDX, m_culturalFrequency, 0, 100);
//DDX_Text(pDX, IDC_POLICYFREQUENCY, m_policyFrequency);
//DDV_MinMaxInt(pDX, m_policyFrequency, 0, 100);
//DDX_Text(pDX, IDC_EVALFREQUENCY, m_evalFrequency);
//DDV_MinMaxInt(pDX, m_evalFrequency, 0, 100);
DDX_Control(pDX, IDC_MODELS, m_models);
DDX_Control(pDX, IDC_APS, m_aps);
}


BEGIN_MESSAGE_MAP(PPRunSetup, CPropertyPage)
   //ON_BN_CLICKED(IDC_ENABLECULTURALMETAPROCESS, OnBnClicked)
   //ON_BN_CLICKED(IDC_ENABLEPOLICYMETAPROCESS, OnBnClicked)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PPRunSetup message handlers

BOOL PPRunSetup::OnInitDialog() 
   {
	CPropertyPage::OnInitDialog();
/*
   if ( m_culturalEnabled && m_culturalFrequency > 0 )
      CheckDlgButton( IDC_ENABLECULTURALMETAPROCESS, BST_CHECKED );
   else
      CheckDlgButton( IDC_ENABLECULTURALMETAPROCESS, BST_UNCHECKED );

   if ( m_policyEnabled && m_policyFrequency > 0 )
      CheckDlgButton( IDC_ENABLEPOLICYMETAPROCESS, BST_CHECKED );
   else
      CheckDlgButton( IDC_ENABLEPOLICYMETAPROCESS, BST_UNCHECKED );
*/
   // models
   m_models.ResetContent();
   int count = gpModel->GetEvaluatorCount();
   for ( int i=0; i < count; i++ )
      {
      EnvEvaluator *pModel = gpModel->GetEvaluatorInfo( i );

      m_models.AddString( pModel->m_name );

      if ( pModel->m_use )
         m_models.SetCheck( i, TRUE );
      else
         m_models.SetCheck( i, FALSE );
      }

   m_models.SetCurSel( 0 );

   // autonomous processes
   m_aps.ResetContent();
   count = gpModel->GetModelProcessCount();
   for ( int i=0; i < count; i++ )
      {
      EnvModelProcess *pAP = gpModel->GetModelProcessInfo( i );

      m_aps.AddString( pAP->m_name );

      if ( pAP->m_use )
         m_aps.SetCheck( i, TRUE );
      else
         m_aps.SetCheck( i, FALSE );
      }

   m_aps.SetCurSel( 0 );

   //CSpinButtonCtrl *pSpin = (CSpinButtonCtrl*) GetDlgItem( IDC_EVALFREQSPIN );
   //pSpin->SetRange( 0, 100 );

   //EnableControls();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
   }
/*
void PPRunSetup::EnableControls(void)
   {
   m_culturalEnabled = FALSE;
   m_policyEnabled   = FALSE;

   if ( IsDlgButtonChecked( IDC_ENABLECULTURALMETAPROCESS ) )
      m_culturalEnabled = TRUE;
   
   GetDlgItem( IDC_CULTURALFREQUENCY )->EnableWindow( m_culturalEnabled );
   GetDlgItem( IDC_CULTURALFREQUENCY_LABEL )->EnableWindow( m_culturalEnabled );

   if ( IsDlgButtonChecked( IDC_ENABLEPOLICYMETAPROCESS ) )
      m_policyEnabled = TRUE;
   
   
   GetDlgItem( IDC_POLICYFREQUENCY )->EnableWindow( m_policyEnabled );
   GetDlgItem( IDC_POLICYFREQUENCY_LABEL )->EnableWindow( m_policyEnabled );
   }


void PPRunSetup::OnBnClicked()
   {
   EnableControls();
   }
*/

int PPRunSetup::SetModelsToUse( void )
   {
   // evaluative models
   int count = m_models.GetCount();
   ASSERT( count == gpModel->GetEvaluatorCount() );

   int countInUse = 0;

   for ( int i=0; i < count; i++ )
      {
      EnvEvaluator *pModel = gpModel->GetEvaluatorInfo( i );

      if ( m_models.GetCheck( i ) )
         {
         pModel->m_use = true;
         countInUse++;
         }
      else
         pModel->m_use = false;
      }

   count = m_aps.GetCount();
   ASSERT( count == gpModel->GetModelProcessCount() );

   for ( int i=0; i < count; i++ )
      {
      EnvModelProcess *pAP = gpModel->GetModelProcessInfo( i );

      if ( m_aps.GetCheck( i ) )
         {
         pAP->m_use = true;
         countInUse++;
         }
      else
         pAP->m_use = false;
      }

   return countInUse;
   }
