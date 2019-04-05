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
// PPIniSettings.cpp : implementation file
//

#include "stdafx.h"
#include "PPIniSettings.h"

#include "EnvDoc.h"
#include <EnvModel.h>
#include <Scenario.h>
#include <DataManager.h>
#include "IniFileEditor.h"
#include <Maplayer.h>

#include <direct.h>

#include "MapPanel.h"
#include <map.h>

extern ScenarioManager *gpScenarioManager;
extern CEnvDoc   *gpDoc;
extern EnvModel  *gpModel;
extern MapPanel  *gpMapPanel;
extern MapLayer  *gpCellLayer;

// PPIniSettings dialog

IMPLEMENT_DYNAMIC(PPIniSettings, CTabPageSSL)

PPIniSettings::PPIniSettings(IniFileEditor* pParent /*=NULL*/)
	: CTabPageSSL(PPIniSettings::IDD, pParent)
   , m_pParent( pParent )
   , m_loadShared(FALSE)
   , m_debug(FALSE)
   , m_mapOutputs(FALSE)
   , m_interval(0)
   , m_defaultPeriod(0)
   , m_mapUnits(0)
   { }


PPIniSettings::~PPIniSettings()
{ }


void PPIniSettings::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
DDX_Check(pDX, IDC_LOADSHAREDPOLICIES, m_loadShared);
DDX_Check(pDX, IDC_DEBUG, m_debug);
DDX_Check(pDX, IDC_MAPOUTPUTS, m_mapOutputs);
DDX_Check(pDX, IDC_NOBUFFER, m_noBuffer);
DDX_Text(pDX, IDC_INTERVAL, m_interval);
DDV_MinMaxInt(pDX, m_interval, -1, 100);
DDX_Text(pDX, IDC_DEFAULTPERIOD, m_defaultPeriod);
DDV_MinMaxInt(pDX, m_defaultPeriod, -1, 100);
//DDX_Control(pDX, IDC_VARS, m_vars);
DDX_Control(pDX, IDC_GRID, m_varGrid);
DDX_CBIndex(pDX, IDC_MAPUNITS, m_mapUnits);
   }

BEGIN_MESSAGE_MAP(PPIniSettings, CTabPageSSL)
   ON_BN_CLICKED(IDC_MAPOUTPUTS, &PPIniSettings::OnBnClickedMapoutputs)
   ON_EN_CHANGE(IDC_INTERVAL, &PPIniSettings::MakeDirty)
   ON_BN_CLICKED(IDC_LOADSHAREDPOLICIES, &PPIniSettings::MakeDirty)
   ON_BN_CLICKED(IDC_DEBUG, &PPIniSettings::MakeDirty)
   ON_BN_CLICKED(IDC_MAPOUTPUTS, &PPIniSettings::MakeDirty)
   ON_BN_CLICKED(IDC_NOBUFFER, &PPIniSettings::MakeDirty)
   ON_BN_CLICKED(IDC_ADD, &PPIniSettings::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_REMOVE, &PPIniSettings::OnBnClickedRemove)
END_MESSAGE_MAP()


// PPIniSettings message handlers

BOOL PPIniSettings::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();
   m_loadShared = gpDoc->m_loadSharedPolicies;
   m_debug      = gpDoc->m_model.m_debug;
   //m_noBuffer   = BufferOutcomeFunction::m_enabled ? 0 : 1;
   m_interval   = gpDoc->m_model.m_pDataManager->multiRunDecadalMapsModulus; 
   m_mapOutputs = m_interval >= 0 ? 1 : 0;
   m_defaultPeriod = gpDoc->m_model.m_endYear;
   m_mapUnits = (int) gpMapPanel->m_pMap->m_mapUnits;

   m_varGrid.InsertColumn( "Variable" );
   m_varGrid.InsertColumn( "Value" );
   m_varGrid.InsertColumn( "Description" );
   m_varGrid.InsertColumn( "Column?" );
   m_varGrid.SetColumnWidth( 0, 120 );
   m_varGrid.SetColumnWidth( 1, 200 );
   m_varGrid.SetColumnWidth( 2, 120 );
   m_varGrid.SetColumnWidth( 2,  80 );
   m_varGrid.SetEditable( 1 );
   m_varGrid.SetSingleRowSelection( 1 );
   m_varGrid.SetFixedRowCount();
  
   for ( int i=0; i < gpModel->GetAppVarCount( AVT_APP ); i++ )
     {
     AppVar *pVar = gpModel->GetAppVar( i );
     int nIndex = m_varGrid.InsertRow( pVar->m_name );
     m_varGrid.SetItemText( nIndex, 1, pVar->m_expr );
     m_varGrid.SetItemText( nIndex, 2, pVar->m_description );

     if ( pVar->GetCol() >= 0 )
        m_varGrid.SetItemText( nIndex, 3, gpCellLayer->GetFieldLabel( pVar->GetCol() ) );
     else
        m_varGrid.SetItemText( nIndex, 3, "" );
     }

   UpdateData( 0 );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void PPIniSettings::MakeDirty( void ) { m_pParent->MakeDirty( INI_SETTINGS ); }


bool PPIniSettings::StoreChanges()
   {
   UpdateData( 1 );

   gpDoc->m_loadSharedPolicies = m_loadShared;
   gpDoc->m_model.m_debug = m_debug;
   //BufferOutcomeFunction::m_enabled = m_noBuffer ? false : true;

   int interval = -1;
   if ( m_mapOutputs )
      interval = m_interval;

   gpDoc->m_model.m_pDataManager->multiRunDecadalMapsModulus = interval; 
   gpDoc->m_model.m_endYear = m_defaultPeriod;

   gpMapPanel->m_pMap->m_mapUnits = (MAP_UNITS) m_mapUnits;

   // AppVars
   gpModel->RemoveAllAppVars();

   int rowCount = m_varGrid.GetRowCount();
   for ( int i=1; i < rowCount; i++ )
      {
      CString name = m_varGrid.GetItemText( i, 0 );
      CString expr = m_varGrid.GetItemText( i, 1 );
      CString desc = m_varGrid.GetItemText( i, 2 );
      CString col  = m_varGrid.GetItemText( i, 3 );
      AppVar *pVar = new AppVar( name, desc, expr );
      pVar->SetCol( gpCellLayer->GetFieldCol( col ) );
      pVar->m_avType = AVT_APP;

      gpModel->AddAppVar( pVar, true );
      }

   return true;
   }


void PPIniSettings::OnBnClickedMapoutputs()
   {
   MakeDirty();
   }


void PPIniSettings::OnBnClickedAdd()
   {
   int nIndex = m_varGrid.InsertRow( _T( "<Enter Variable Name>" ) );
   m_varGrid.SetItemText( nIndex, 1, _T( "<Enter Expression>" ) );
   m_varGrid.SetItemText( nIndex, 2, _T( "<Enter Description>" ) );
   m_varGrid.SetItemText( nIndex, 3, _T( "" ) );
   m_varGrid.RedrawWindow();
   }

void PPIniSettings::OnBnClickedRemove()
   {
   //int selRow = m_vars.GetSelectionMark();

   CCellRange range = m_varGrid.GetSelectedCellRange();
   if ( range.Count() == 0 )
      return;

   for ( int i=range.GetMinRow(); i < range.GetMaxRow()+1; i++ )
      m_varGrid.DeleteRow( i );

   m_varGrid.RedrawWindow();
   }
