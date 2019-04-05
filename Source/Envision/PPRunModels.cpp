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
// PPRunModels.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "PPRunModels.h"
#include "EnvDoc.h"

extern CEnvDoc *gpDoc;
extern EnvModel *gpModel;

// PPRunModels dialog

IMPLEMENT_DYNAMIC(PPRunModels, CPropertyPage)
PPRunModels::PPRunModels()
	: CPropertyPage(PPRunModels::IDD)
{
}

PPRunModels::~PPRunModels()
{
}

void PPRunModels::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_MODELS, m_models);
}


BEGIN_MESSAGE_MAP(PPRunModels, CPropertyPage)
END_MESSAGE_MAP()


// PPRunModels message handlers

BOOL PPRunModels::OnInitDialog()
   {
   CPropertyPage::OnInitDialog();

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

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }



int PPRunModels::SetModelsToUse( void )
   {
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

   return countInUse;
   }
