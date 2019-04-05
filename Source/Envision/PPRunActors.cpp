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
// PPRunActors.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "EnvDoc.h"
#include "PPRunActors.h"
#include "actordlg.h"
#include <Actor.h>

extern CEnvDoc      *gpDoc;
extern ActorManager *gpActorManager;
extern EnvModel *gpModel;

// PPRunActors dialog

IMPLEMENT_DYNAMIC(PPRunActors, CPropertyPage)
PPRunActors::PPRunActors()
	: CPropertyPage(PPRunActors::IDD)
   , m_count(0)
   {
}

PPRunActors::~PPRunActors()
{
}

void PPRunActors::DoDataExchange(CDataExchange* pDX)
{
CPropertyPage::DoDataExchange(pDX);
DDX_Text(pDX, IDC_COUNT, m_count);
}


BEGIN_MESSAGE_MAP(PPRunActors, CPropertyPage)
   ON_BN_CLICKED(IDC_GENERATE, OnBnClickedGenerate)
   ON_BN_CLICKED(IDC_RANDOM, OnBnClickedRandom)
   ON_BN_CLICKED(IDC_UNIFORM, OnBnClickedUniform)
   ON_BN_CLICKED(IDC_CURRENT, OnBnClickedCurrent)
   ON_BN_CLICKED(IDC_VIEWCURRENT, OnBnClickedViewcurrent)
END_MESSAGE_MAP()


// PPRunActors message handlers

BOOL PPRunActors::OnInitDialog()
   {
   CPropertyPage::OnInitDialog();

   CheckDlgButton( IDC_CURRENT, TRUE );
   GetDlgItem( IDC_GENERATE )->EnableWindow( FALSE );
   SetDlgItemInt( IDC_COUNT, 10 );
   
   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void PPRunActors::OnBnClickedGenerate()
   {
   UpdateData( 1 );

   if ( IsDlgButtonChecked( IDC_CURRENT ) )
      return;

   if ( m_count <= 0 )
      MessageBox( "Please specify the number of actors to generate" );

   gpActorManager->RemoveAllActors();  // clear out any existing actors and groups
   gpActorManager->RemoveAllActorGroups();

   ActorGroup *pGroup = new ActorGroup;
   pGroup->m_count = m_count;
   //pGroup->m_lulcLevel = 0;            // all lulc classes
   //pGroup->m_lulcClass = -1;
   pGroup->m_decisionFrequency = 1;    // every year
   pGroup->m_id = 0;

   bool random = IsDlgButtonChecked( IDC_RANDOM ) ? true : false; // which type of weights?
   
   float weight = 0.0f;
   char name[ 32 ];
   lstrcpy( name, "Actor " );
   char *p = name+5;

   RandUniform randUnif;

   for ( int i=0; i < m_count; i++ )
      {
      Actor *pActor = new Actor;
      gpActorManager->AddActor( pActor );
      pActor->m_pGroup = pGroup;

      int actorValueCount = gpModel->GetActorValueCount();

      // objectives and weights
      for ( int j=0; j < actorValueCount; j++ )
         {
         if ( random )
            weight = (float) randUnif.RandValue( -3.0f, 3.0f );

         pActor->AddValue( weight );
         }

      // other members
      _itoa_s( i, p, 27, 10 );
      //pActor->m_name = name;
      }

   //gpDoc->AssignActorsToCells();
   
   CheckDlgButton( IDC_CURRENT, TRUE );
   GetDlgItem( IDC_GENERATE )->EnableWindow( FALSE );
   MessageBox( "Successfully generated actors and assigned to cells" );
   }

void PPRunActors::OnBnClickedRandom()
   {
   GetDlgItem( IDC_GENERATE )->EnableWindow( TRUE );
   }

void PPRunActors::OnBnClickedUniform()
   {
   GetDlgItem( IDC_GENERATE )->EnableWindow( TRUE );

   // TODO: Add your control notification handler code here
   }

void PPRunActors::OnBnClickedCurrent()
   {
   GetDlgItem( IDC_GENERATE )->EnableWindow( FALSE );
   }

void PPRunActors::OnBnClickedViewcurrent()
   {
   ActorDlg dlg;
   dlg.DoModal();
   }
