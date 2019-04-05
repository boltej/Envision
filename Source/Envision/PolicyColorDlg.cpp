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
// PolicyColorDlg.cpp : implementation file
//

#include "stdafx.h"
#pragma hdrstop

#include "Envision.h"
#include <EnvModel.h>
#include ".\policycolordlg.h"
#include <Policy.h>


extern PolicyManager *gpPolicyManager;
extern EnvModel *gpModel;


// PolicyColorDlg dialog

IMPLEMENT_DYNAMIC(PolicyColorDlg, CDialog)
PolicyColorDlg::PolicyColorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(PolicyColorDlg::IDD, pParent)
{
}

PolicyColorDlg::~PolicyColorDlg()
{
}

void PolicyColorDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_RED, m_red);
DDX_Control(pDX, IDC_GREEN, m_green);
DDX_Control(pDX, IDC_BLUE, m_blue);
}


BEGIN_MESSAGE_MAP(PolicyColorDlg, CDialog)
   ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// PolicyColorDlg message handlers

void PolicyColorDlg::OnBnClickedOk()
   {
   int red   = m_red.GetCurSel()   - 1;
   int green = m_green.GetCurSel() - 1;
   int blue  = m_blue.GetCurSel()  - 1;

   // get scaling factors
   int count = gpModel->GetMetagoalCount();
 
   float *maxScoreArray = new float[ count ];
   float *minScoreArray = new float[ count ];

   for ( int i=0; i < count; i++ )
      {
      maxScoreArray[ i ] = -3;
      minScoreArray[ i ] = 3;
      }

   for ( int i=0; i < gpPolicyManager->GetPolicyCount(); i++ )
      {
      Policy *pPolicy = gpPolicyManager->GetPolicy( i );

      for ( int j=0; j < count; j++ )
         {
         float score = pPolicy->GetGoalScore( j );
         if ( score > maxScoreArray[ j ] )
            maxScoreArray[ j ] = score;
         if ( score < minScoreArray[ j ] )
            minScoreArray[ j ] = score;
         }
      }

      // having scaling factors, get colors

   for ( int i=0; i < gpPolicyManager->GetPolicyCount(); i++ )
      {
      Policy *pPolicy = gpPolicyManager->GetPolicy( i );

      int _red = 0, _green = 0, _blue = 0;

      if ( red >= 0 )
         _red = int( 255 * ( pPolicy->GetGoalScore( red ) + minScoreArray[red] ) / (maxScoreArray[red]-minScoreArray[red]) );
      
      if ( green >= 0 )
         _green = int( 255 * ( pPolicy->GetGoalScore( green ) + minScoreArray[green] ) / (maxScoreArray[green]-minScoreArray[green]) );
   
      if ( blue >= 0 )
         _blue = int( 255 * ( pPolicy->GetGoalScore( blue ) + minScoreArray[blue] ) / (maxScoreArray[blue]-minScoreArray[blue]) );

      pPolicy->m_color = RGB( _red, _green, _blue );
      }

   delete [] minScoreArray;
   delete [] maxScoreArray;

   OnOK();
   }


BOOL PolicyColorDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   m_red.AddString( "<don't use this color>" );
   m_green.AddString( "<don't use this color>" );
   m_blue.AddString( "<don't use this color>" );

   // add metagoals to the check list
   int count = gpModel->GetMetagoalCount();    // only worry about metagoals for policies
   for ( int i=0; i < count; i++ )
      {
      EnvEvaluator *pInfo = gpModel->GetMetagoalEvaluatorInfo( i );
      if ( pInfo != NULL )
         {
         m_red.AddString( (LPCTSTR) pInfo->m_name );
         m_green.AddString( (LPCTSTR) pInfo->m_name );
         m_blue.AddString( (LPCTSTR) pInfo->m_name );
         }
      }

   m_red.SetCurSel( 0 );
   m_green.SetCurSel( 0 );
   m_blue.SetCurSel( 0 );

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }
