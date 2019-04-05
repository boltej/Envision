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
// PPRunMain.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "WizRun.h"
#include "EnvDoc.h"

//#include <Scenario.h>

#include <maplayer.h>
#include <MAP.h>

#include ".\pprunmain.h"

extern MapLayer        *gpCellLayer;
extern CEnvDoc         *gpDoc;
//extern ScenarioManager *gpScenarioManager;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PPRunMain property page

IMPLEMENT_DYNCREATE(PPRunMain, CPropertyPage)

PPRunMain::PPRunMain() : CPropertyPage(PPRunMain::IDD)
, m_dynamicUpdateViews(FALSE)
, m_dynamicUpdateMap(FALSE)
, m_iterations(0)
, m_useInitialSeed( 0 )
, m_exportMap( 0 )
, m_exportBmps( 0 )
, m_fieldList( "<comma-separated field list>" )
, m_pixelSize( 120 )
, m_exportOutputs( 0 )
   { }

PPRunMain::~PPRunMain()
{ }

void PPRunMain::DoDataExchange(CDataExchange* pDX)
{
CPropertyPage::DoDataExchange(pDX);
DDX_Check(pDX, IDC_DYNAMICUPDATEVIEWS, m_dynamicUpdateViews);
DDX_Check(pDX, IDC_DYNAMICUPDATEMAP, m_dynamicUpdateMap);
DDX_Text(pDX, IDC_ITERATIONS, m_iterations);
DDV_MinMaxInt(pDX, m_iterations, 0, 500);
DDX_Check(pDX, IDC_USEINITIALSEED, m_useInitialSeed);
DDX_Check(pDX, IDC_EXPORTMAP, m_exportMap);
DDX_Text(pDX, IDC_INTERVAL, m_exportInterval);

DDX_Check(pDX, IDC_EXPORTBMPS, m_exportBmps);
DDX_Check(pDX, IDC_EXPORTOUTPUT, m_exportOutputs);
DDX_Text(pDX, IDC_INTERVAL, m_fieldList);
DDX_Text(pDX, IDC_INTERVAL, m_pixelSize);
}


BEGIN_MESSAGE_MAP(PPRunMain, CPropertyPage)	
   ON_BN_CLICKED(IDC_EXPORTMAP, &PPRunMain::OnBnClickedExportmap)
   ON_BN_CLICKED(IDC_EXPORTBMPS, &PPRunMain::OnBnClickedExportBmps)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PPRunMain message handlers

BOOL PPRunMain::OnInitDialog() 
   {
  	CPropertyPage::OnInitDialog();

   //LoadScenarios();
   
   //CSpinButtonCtrl *pSpin = (CSpinButtonCtrl*) GetDlgItem( IDC_EVALFREQSPIN );
   //pSpin->SetRange( 0, 100 );

   CSpinButtonCtrl *pInterval = (CSpinButtonCtrl*) GetDlgItem( IDC_SPININTERVAL );
   pInterval->SetRange( 0, 100 );

   LPCTSTR mapUnits = gpCellLayer->GetMapPtr()->GetMapUnitsStr();

   if ( mapUnits != NULL )
      SetDlgItemText( IDC_MAPUNITS_LABEL, mapUnits );

   UpdateUI();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
   }


void PPRunMain::OnRun() 
   {
   UpdateData();
   WizRun *pWiz = (WizRun*) GetParent();
   pWiz->SaveSetup();	
   }


void PPRunMain::UpdateUI()
   {
   UpdateData();  // save and validate

   GetDlgItem( IDC_EXPORTBMPS )->EnableWindow( m_exportMap );
   GetDlgItem( IDC_FIELDLIST  )->EnableWindow( m_exportMap );

   GetDlgItem( IDC_PIXELSIZE_LABEL )->EnableWindow( m_exportBmps );
   GetDlgItem( IDC_PIXELSIZE )->EnableWindow( m_exportBmps );
   GetDlgItem( IDC_MAPUNITS_LABEL )->EnableWindow( m_exportBmps );
   }


void PPRunMain::OnBnClickedExportmap()
   {
   UpdateUI();
   }

void PPRunMain::OnBnClickedExportBmps()
   {
   UpdateUI();
   }
