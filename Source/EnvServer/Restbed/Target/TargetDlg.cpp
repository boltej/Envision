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
// TargetDlg.cpp : implementation file
//

#include "stdafx.h"

#include "resource.h"

#include "Target.h"
#include "TargetDlg.h"
#include "afxdialogex.h"

#include "SaveToDlg.h"
#include "NameDlg.h"

#include <direct.h>
#include <EnvisionAPI.h>


// TargetDlg dialog

IMPLEMENT_DYNAMIC(TargetDlg, CDialog)

TargetDlg::TargetDlg( TargetProcess *pTargetProcess, MapLayer *pLayer, CWnd* pParent /*=NULL*/)
   : CDialog(TargetDlg::IDD, pParent)
   , m_pLayer( pLayer )
   , m_pTargetProcess( pTargetProcess )
   , m_pTarget( NULL )
   , m_rate(0)
   , m_allocationSetID(0)
   { }


TargetDlg::~TargetDlg()
{ }


void TargetDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_METHOD, m_methods);
DDX_Control(pDX, IDC_SCENARIOS, m_scenarios);
DDX_Control(pDX, IDC_COLDENSITY, m_colDensity);
DDX_Control(pDX, IDC_COLCAPACITY, m_colCapacity);
DDX_Control(pDX, IDC_COLAVAIL, m_colAvail);
DDX_Control(pDX, IDC_COLPERCENTAVAIL, m_colPctAvail);
DDX_Text(pDX, IDC_VALUE, m_rate);
DDX_Text(pDX, IDC_ALLOCATIONSETID, m_allocationSetID);
   }


BEGIN_MESSAGE_MAP(TargetDlg, CDialog)
   ON_BN_CLICKED(IDC_ADDREPORT, &TargetDlg::OnBnClickedAddreport)
   ON_BN_CLICKED(IDC_DELETEREPORT, &TargetDlg::OnBnClickedDeletereport)
   ON_BN_CLICKED(IDC_ADDCONSTANT, &TargetDlg::OnBnClickedAddconstant)
   ON_BN_CLICKED(IDC_DELETECONSTANT, &TargetDlg::OnBnClickedDeleteconstant)
   ON_BN_CLICKED(IDC_ADDALLOCATION, &TargetDlg::OnBnClickedAddallocation)
   ON_BN_CLICKED(IDC_DELETEALLOCATION, &TargetDlg::OnBnClickedDeleteallocation)
   ON_BN_CLICKED(IDC_ADDPREFERENCE, &TargetDlg::OnBnClickedAddpreference)
   ON_BN_CLICKED(IDC_DELETEPREFERENCE, &TargetDlg::OnBnClickedDeletepreference)
   ON_CBN_SELCHANGE(IDC_SCENARIOS, &TargetDlg::OnCbnSelchangeScenarios)
   ON_BN_CLICKED(IDC_REMOVEALLOCATIONSET, &TargetDlg::OnBnClickedRemoveallocationset)
   ON_BN_CLICKED(IDC_ADDALLOCATIONSET, &TargetDlg::OnBnClickedAddallocationset)
   ON_EN_CHANGE(IDC_ALLOCATIONSETID, &TargetDlg::OnEnChangeAllocationsetid)
END_MESSAGE_MAP()


// TargetDlg message handlers

BOOL TargetDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();
      
   // assume one and only one for now...
   m_pTarget = m_pTargetProcess->m_targetArray[ 0 ];
   
   m_font.CreatePointFont( 90, "Arial" );

   // set up grids
   CRect rect;
   GetDlgItem( IDC_REPORTPLACEHOLDER )->GetWindowRect( &rect );
   this->ScreenToClient( &rect );
   m_reportsGrid.CreateGrid( WS_CHILD | WS_BORDER, rect, this, 19998 );
   m_reportsGrid.m_pParent = this;
   m_reportsGrid.ShowWindow( SW_SHOW );
   m_reportsGrid.SetDefFont( &m_font );
   
   GetDlgItem( IDC_CONSTANTPLACEHOLDER )->GetWindowRect( &rect );
   this->ScreenToClient( &rect );
   m_constantsGrid.CreateGrid( WS_CHILD | WS_BORDER, rect, this, 199999 );
   m_constantsGrid.m_pParent = this;
   m_constantsGrid.ShowWindow( SW_SHOW );
   m_constantsGrid.SetDefFont( &m_font );

   GetDlgItem(IDC_ALLOCATIONPLACEHOLDER )->GetWindowRect( &rect );  // screen coords
   this->ScreenToClient( &rect );

   for ( int i=0; i < (int) m_pTarget->m_allocationSetArray.GetSize(); i++ )
      {
      AllocationsGrid *pGrid = new AllocationsGrid;
      pGrid->CreateGrid( WS_CHILD | WS_BORDER, rect, this, 20000+i );
      m_allocationsGridArray.Add( pGrid );
      pGrid->m_pParent = this;
      pGrid->m_pAllocSet = m_pTarget->m_allocationSetArray[ i ];
      pGrid->SetDefFont( &m_font );

      if ( i == 0 )
         pGrid->ShowWindow( SW_SHOW );
      }

   GetDlgItem(IDC_PREFERENCESPLACEHOLDER )->GetWindowRect( &rect );  // screen coords
   this->ScreenToClient( &rect );

   for ( int i=0; i < (int) m_pTarget->m_allocationSetArray.GetSize(); i++ )
      {
      PreferencesGrid *pGrid = new PreferencesGrid;
      pGrid->CreateGrid( WS_CHILD | WS_BORDER, rect, this, 21000+i );
      m_preferencesGridArray.Add( pGrid );
      pGrid->m_pParent = this;
      pGrid->m_pAllocSet = m_pTarget->m_allocationSetArray[ i ];
      pGrid->SetDefFont( &m_font );

      if ( i == 0 )
         pGrid->ShowWindow( SW_SHOW );
      }

   // set up other stuff
   m_methods.AddString( _T("Linear") );
   m_methods.AddString( _T("Exponential") );
   //m_methods.AddString( _T("Time Series") );
   m_methods.SetCurSel( 0 );

   int cols = m_pLayer->GetFieldCount();
   for ( int i=0; i < cols; i++ )
      {
      LPCTSTR field = m_pLayer->GetFieldLabel( i );
      m_colDensity.AddString( field );
      m_colCapacity.AddString( field );
      m_colAvail.AddString( field );
      m_colPctAvail.AddString( field );
      }

   // scenarios (actually, allocation sets)
   // allocations and set depend on the scenario selected
   // assume one and only one for now...
   int index = -1; 
   for ( int i=0; i < m_pTarget->m_allocationSetArray.GetSize(); i++ )
      {
      AllocationSet *pSet = m_pTarget->m_allocationSetArray[ i ];
      int index = m_scenarios.AddString( pSet->m_name );
      m_scenarios.SetItemData( index, pSet->m_id );
      }

   m_scenarios.SetCurSel( 0 );

   // set fields for known columns
   CString popDensCol  = m_pLayer->GetFieldLabel( m_pTarget->m_colTarget );
   CString popCapCol   = m_pLayer->GetFieldLabel( m_pTarget->m_colTargetCapacity );
   CString popAvailCol =  m_pLayer->GetFieldLabel( m_pTarget->m_colAvailCapacity );
   CString pctPopAvailCol =  m_pLayer->GetFieldLabel( m_pTarget->m_colPctAvailCapacity );

   index = m_colDensity.FindString( -1, popDensCol );
   m_colDensity.SetCurSel( index >= 0 ? index : -1 );

   index = m_colCapacity.FindString( -1, popCapCol );
   m_colCapacity.SetCurSel( index >= 0 ? index : -1 );

   index = m_colAvail.FindString( -1, popAvailCol );
   m_colAvail.SetCurSel( index >= 0 ? index : -1 );

   index = m_colPctAvail.FindString( -1, pctPopAvailCol );
   m_colPctAvail.SetCurSel( index >= 0 ? index : -1 );

   LoadTarget();     // assumes only one target (for now)
   
   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


bool TargetDlg::LoadTarget( void )
   {
   ClearGrids( true );

   // assume one and only one for now...
   m_pTarget = m_pTargetProcess->m_targetArray[ 0 ];
   m_rate = (float) m_pTarget->m_rate;

   UpdateData( 0 );
   
   // reports
   for ( int i=0; i < (int) m_pTarget->m_reportArray.GetSize(); i++ )
      {
      TargetReport *pReport = m_pTarget->m_reportArray.GetAt( i );

      ASSERT( pReport != NULL );
      m_reportsGrid.AppendRow( pReport->m_name, pReport->m_query );
      }

   // constants
   for ( int i=0; i < (int) m_pTarget->m_constArray.GetSize(); i++ )
      {
      Constant *pConst = m_pTarget->m_constArray.GetAt( i );

      ASSERT( pConst != NULL );
      m_constantsGrid.AppendRow( pConst->m_name, pConst->m_expression );
      }

   for ( int i=0; i < m_pTarget->m_allocationSetArray.GetSize(); i++ )
      LoadAllocationSet(i);

   this->OnCbnSelchangeScenarios();

   return true;
   }


bool TargetDlg::LoadAllocationSet( int i )
   {
   ClearGrids( false );

   // allocations and set depend on the scenario selected
   int scenario = m_scenarios.GetCurSel();
   ASSERT( scenario >= 0 );
   
   AllocationSet *pSet = m_pTarget->m_allocationSetArray.GetAt( i );
   
   if ( pSet != NULL )
      {
      for ( int j=0; j < (int) pSet->GetSize(); j++ )
         {
         ALLOCATION *pAlloc = pSet->GetAt( j );
         m_allocationsGridArray[ i ]->AppendRow( pAlloc->name, pAlloc->queryStr, pAlloc->expression );
         }

      for (int j=0; j < (int) pSet->m_preferences.GetSize(); j++ )
         {
         PREFERENCE *pPref = pSet->m_preferences.GetAt( j );
         ASSERT( pPref != NULL );
         CString value;
         value.Format( "%f", pPref->value );
         m_preferencesGridArray[ i ]->AppendRow( pPref->name, pPref->queryStr, value );
         }
      }

   return true;
   }


void TargetDlg::ClearGrids( bool all )
   {
   int rows;
   if ( all )
      {
      rows = m_reportsGrid.GetNumberRows();
      for ( int i=rows-1; i >= 0; i-- )
         m_reportsGrid.DeleteRow( i );
   
      rows = m_constantsGrid.GetNumberRows();
      for ( int i=rows-1; i >= 0; i-- )
         m_constantsGrid.DeleteRow( i );

      for ( int i=0; i < m_allocationsGridArray.GetSize(); i++ )
         {
         rows = m_allocationsGridArray[ i ]->GetNumberRows();
         for ( int j=rows-1; j >= 0; j-- )
            m_allocationsGridArray[ i ]->DeleteRow( j );

         rows = m_preferencesGridArray[ i ]->GetNumberRows();
         for ( int j=rows-1; j >= 0; j-- )
            m_preferencesGridArray[ i ]->DeleteRow( j );
         }
      }
   }


void TargetDlg::OnBnClickedAddreport()
   {
   m_reportsGrid.AppendRow( "<name>", "<query (blank for all)>" );
   }


void TargetDlg::OnBnClickedDeletereport()
   {
   long row = m_reportsGrid.GetCurrentRow();
   if ( row < 0 )
      {
      MessageBeep( 0 );
      return;
      }

   // delete from allocation set
   m_reportsGrid.DeleteRow( row );
   }


void TargetDlg::OnBnClickedAddconstant()
   {
   m_constantsGrid.AppendRow( "<name>", "<value (number)>" );
   }


void TargetDlg::OnBnClickedDeleteconstant()
   {
   long row = m_constantsGrid.GetCurrentRow();
   if ( row < 0 )
      {
      MessageBeep( 0 );
      return;
      }

   // delete from allocation set
   m_constantsGrid.DeleteRow( row );
   }


void TargetDlg::OnBnClickedAddallocation()
   {
   int scenario = m_scenarios.GetCurSel();

   if ( scenario < 0 )
      {
      MessageBeep( 0 );
      return;
      }

   AllocationsGrid *pGrid = m_allocationsGridArray[ scenario ];
   //AllocationSet *pSet = pGrid->m_pAllocSet;

   // add to set
   //ALLOCATION *pAlloc = pSet->AddAllocation( "<name>", "<query>, blank for none", "<expr>", 1 );
   
   // add to grid
   pGrid->AppendRow( "<name>", "<query>, blank for none", "<expr>" );
   }


void TargetDlg::OnBnClickedDeleteallocation()
   {
   int scenario = m_scenarios.GetCurSel();

   if ( scenario < 0 )
      {
      MessageBeep( 0 );
      return;
      }

   AllocationsGrid *pGrid = m_allocationsGridArray[ scenario ];
   //AllocationSet *pSet = pGrid->m_pAllocSet;

   // get currently selected row
   long row = pGrid->GetCurrentRow();
   if ( row < 0 )
      {
      MessageBeep( 0 );
      return;
      }

   // delete from allocation set
   //pSet->RemoveAt( row );

   // delete from grid
   pGrid->DeleteRow( row );
   }


void TargetDlg::OnBnClickedAddpreference()
   {
   int scenario = m_scenarios.GetCurSel();

   if ( scenario < 0 )
      {
      MessageBeep( 0 );
      return;
      }

   PreferencesGrid *pGrid = m_preferencesGridArray[ scenario ];
   //AllocationSet *pSet = pGrid->m_pAllocSet;

   // add to set
   //PREFERENCE *pPref = pSet->m_preferences.Add( "<name>", "", 1 );

   // add to grid
   //CString value;
   //value.Format( "%f", pPref->value );
   pGrid->AppendRow( "<name>", "<query>, blank for everywhere", "1" );
   }


void TargetDlg::OnBnClickedDeletepreference()
   {
   int scenario = m_scenarios.GetCurSel();

   if ( scenario < 0 )
      {
      MessageBeep( 0 );
      return;
      }

   PreferencesGrid *pGrid = m_preferencesGridArray[ scenario ];
   //AllocationSet *pSet = pGrid->m_pAllocSet;

   // get currently selected row
   long row = pGrid->GetCurrentRow();
   if ( row < 0 )
      {
      MessageBeep( 0 );
      return;
      }

   // delete from allocation set
   //pSet->m_preferences.RemoveAt( row );

   // delete from grid
   pGrid->DeleteRow( row );
   }


void TargetDlg::SaveTarget()
   {
   UpdateData( 1 );

   m_pTarget->m_rate = m_rate;

   // columns
   CString colDensity, colAvail, colCapacity, colPctAvail;
   
   m_colDensity.GetWindowText( colDensity );
   m_colCapacity.GetWindowText( colCapacity );
   m_colAvail.GetWindowText( colAvail );
   m_colPctAvail.GetWindowText( colPctAvail );

   if ( ! colDensity.IsEmpty() )
      {
      m_pTarget->m_colTarget = m_pLayer->GetFieldCol( colDensity );
      if ( m_pTarget->m_colTarget < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_FLOAT, width, decimals );
         m_pTarget->m_colTarget =  m_pLayer->m_pDbTable->AddField( colDensity, TYPE_FLOAT, width, decimals, true );
         }
      }
   else
      m_pTarget->m_colTarget = -1;

   if ( ! colCapacity.IsEmpty() )
      {
      m_pTarget->m_colTargetCapacity = m_pLayer->GetFieldCol( colCapacity );
      if ( m_pTarget->m_colTargetCapacity < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_FLOAT, width, decimals );
         m_pTarget->m_colTargetCapacity =  m_pLayer->m_pDbTable->AddField( colCapacity, TYPE_FLOAT, width, decimals, true );
         }
      }
   else
      m_pTarget->m_colTargetCapacity = -1; 
      
   if ( ! colAvail.IsEmpty() )
      {
      m_pTarget->m_colAvailCapacity = m_pLayer->GetFieldCol( colAvail );
      if ( m_pTarget->m_colAvailCapacity < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_FLOAT, width, decimals );
         m_pTarget->m_colAvailCapacity =  m_pLayer->m_pDbTable->AddField( colAvail, TYPE_FLOAT, true );
         }
      }
   else
      m_pTarget->m_colAvailCapacity = -1;

   
   if ( ! colPctAvail.IsEmpty() )
      {
      m_pTarget->m_colPctAvailCapacity = m_pLayer->GetFieldCol( colPctAvail );
      if ( m_pTarget->m_colPctAvailCapacity < 0 )
         {
         int width, decimals;
         GetTypeParams( TYPE_FLOAT, width, decimals );
         m_pTarget->m_colPctAvailCapacity =  m_pLayer->m_pDbTable->AddField( colPctAvail, TYPE_FLOAT, width, decimals, true );
         }
      }
   else
      m_pTarget->m_colPctAvailCapacity = -1;

   // reports
   m_pTarget->m_reportArray.RemoveAll();
   for ( int i=0; i < m_reportsGrid.GetNumberRows(); i++ )
      {
      CString name = m_reportsGrid.QuickGetText( 0, i );
      CString query = m_reportsGrid.QuickGetText( 1, i );
      m_pTarget->AddReport( name, query );
      }

   // constants
   m_pTarget->m_constArray.RemoveAll();
   for ( int i=0; i < m_constantsGrid.GetNumberRows(); i++ )
      {
      CString name = m_constantsGrid.QuickGetText( 0, i );
      CString value = m_constantsGrid.QuickGetText( 1, i );
      m_pTarget->AddConstant( name, value );
      }

   // allocation sets (for the current allocation set)
   m_pTarget->m_allocationSetArray.RemoveAll();  
   m_pTarget->m_activeAllocSetID = -1;

   for ( int i=0; i < m_allocationsGridArray.GetSize(); i++ )
      {
      AllocationSet *pSet = new AllocationSet;

      m_scenarios.GetLBText( i, pSet->m_name );
      pSet->m_id = (int) m_scenarios.GetItemData( i );

      if ( m_pTarget->m_activeAllocSetID < 0 ) // first allocation set is the default
         m_pTarget->m_activeAllocSetID = pSet->m_id;

      // description...

      m_pTarget->m_allocationSetArray.Add( pSet );


      // havea allocation set, add allocations etc.

      AllocationsGrid *pGrid = m_allocationsGridArray[ i ];

      for ( int j=0; j < pGrid->GetNumberRows(); j++ )
         {
         CString name = pGrid->QuickGetText( 0, j );
         CString query = pGrid->QuickGetText( 1, j );
         CString value = pGrid->QuickGetText( 2, j );

         ALLOCATION *pAlloc = pSet->AddAllocation( name, m_pTarget, query, value, 1.0f );
         pAlloc->Compile( m_pTarget, m_pLayer );
         }

      PreferencesGrid *pPrefGrid = m_preferencesGridArray[ i ];
      
      for ( int j=0; j < pPrefGrid->GetNumberRows(); j++ )
         {
         CString name = pPrefGrid->QuickGetText( 0, j );
         CString query = pPrefGrid->QuickGetText( 1, j );
         CString value = pPrefGrid->QuickGetText( 2, j );
         float _value = (float) atof( value );
         pSet->AddPreference( name, m_pTarget, query, _value );

         //if ( _target != NULL )
         //   pModifier->target = (float) atof( _target );
         }

      }
   }


void TargetDlg::OnCbnSelchangeScenarios()
   {
   int scenario = m_scenarios.GetCurSel();

   m_allocationSetID = (int) m_scenarios.GetItemData( scenario );

   UpdateData( 0 );

   for ( int i=0; i < m_allocationsGridArray.GetSize(); i++ )
      {
      m_allocationsGridArray[ i ]->ShowWindow( i == scenario ? SW_SHOW : SW_HIDE );
      m_preferencesGridArray[ i ]->ShowWindow( i == scenario ? SW_SHOW : SW_HIDE );
      }
   }


void TargetDlg::OnOK()
   {
   if ( this->m_isDirty )
      {
      SaveToDlg dlg( this );
      dlg.m_saveToFile = TRUE;
      dlg.m_saveToSession = TRUE;
      dlg.m_path = this->m_pTargetProcess->m_configFile.c_str();
   
      if ( dlg.DoModal() != IDOK )
         return;
   
      if ( dlg.m_saveToSession )
         SaveTarget();
   
      if ( dlg.m_saveToFile )
         {
         m_pTargetProcess->SaveXml( dlg.m_path, this->m_pLayer );
         }
      }

   CDialog::OnOK();
   }

void TargetDlg::OnBnClickedAddallocationset()
   {
   CNameDlg dlg( this );
   if ( dlg.DoModal() != IDOK )
      return;

   if ( dlg.m_name.GetLength() == 0 )
      return;

   RECT rect;
   GetDlgItem(IDC_ALLOCATIONPLACEHOLDER )->GetWindowRect( &rect );  // screen coords
   this->ScreenToClient( &rect );

   int index = this->m_scenarios.AddString( dlg.m_name );

   // find next unique scenario ID
   m_allocationSetID = 0;
   int scenarios = (int) m_scenarios.GetCount();
   for ( int i=0; i < scenarios; i++ )
      {
      if ( m_scenarios.GetItemData( i ) >= m_allocationSetID )
         m_allocationSetID = (int) m_scenarios.GetItemData( i )+1;
      }

   m_scenarios.SetItemData( index, m_allocationSetID );
   UpdateData( 0 );

   AllocationsGrid *pGrid = new AllocationsGrid;

   pGrid->CreateGrid( WS_CHILD | WS_BORDER, rect, this, 20000+index );
   m_allocationsGridArray.Add( pGrid );
   pGrid->m_pParent = this;
   //pGrid->m_pAllocSet = m_pTarget->m_allocationSetArray[ i ];
   pGrid->SetDefFont( &m_font );

   m_scenarios.SetCurSel( index );
   this->OnCbnSelchangeScenarios();
   }


void TargetDlg::OnBnClickedRemoveallocationset()
   {
   int allocSet = m_scenarios.GetCurSel();

   if ( allocSet < 0 )
      return;

   this->m_allocationsGridArray.RemoveAt( allocSet );
   this->m_preferencesGridArray.RemoveAt( allocSet );

   m_scenarios.SetCurSel( 0 );
   this->OnCbnSelchangeScenarios();
   }


void TargetDlg::OnEnChangeAllocationsetid()
   {
   int scenario = m_scenarios.GetCurSel();

   if ( scenario < 0 )
      return;

   UpdateData( 1 );
   m_scenarios.SetItemData( scenario, m_allocationSetID );

   this->m_isDirty = true;
   }



//////// ReportsGrid ////////////////////////////////////////////

BEGIN_MESSAGE_MAP(ReportsGrid,CUGCtrl)
   //{{AFX_MSG_MAP(ScoreGrid)
      // NOTE - the ClassWizard will add and remove mapping macros here.
      //    DO NOT EDIT what you see in these blocks of generated code !
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

ReportsGrid::ReportsGrid()
: CUGCtrl()
, m_isEditable( true )
{ }


ReportsGrid::~ReportsGrid()
   {
   }

/////////////////////////////////////////////////////////////////////////////
//   OnSetup
//      This function is called just after the grid window 
//      is created or attached to a dialog item.
//      It can be used to initially setup the grid
void ReportsGrid::OnSetup()
   {
   //EnableMenu(TRUE);
   m_ellipsisIndex = AddCellType( &m_ellipsisCtrl );

   SetNumberCols( 2 );  // name, query

   CUGCell cell;
   cell.SetText( _T("Name" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 0, -1, &cell );

   cell.SetText( _T("Query" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 1, -1, &cell );

   SetSH_Width( 0 ) ;

   CRect rect;
   this->GetClientRect( &rect );
 
   SetColWidth( 0, 200 );
   SetColWidth( 1, rect.right - 200 );
   }
 

int ReportsGrid::AppendRow( LPCTSTR name, LPCTSTR query )
   {
   CUGCtrl::AppendRow();

   int row = this->GetNumberRows() - 1;
   // set up col 0 (name)
   CUGCell cell0;
   GetCell( 0, row, &cell0); 
   cell0.SetText( name );   //_T("<Report Name>") );
   cell0.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell(0, row, &cell0); 

   // query
   CUGCell cell1;
   GetCell( 1, row, &cell1 );
   cell1.SetCellType( m_ellipsisIndex );
   cell1.SetText( query ); //  _T("<Specify query or blank for everywhere>" ) );
   cell1.SetAlignment( UG_ALIGNLEFT | UG_ALIGNVCENTER ); 
   cell1.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell( 1, row, &cell1 );
   
   this->GotoCell( 0, row ); 
   RedrawRow( row );

   m_pParent->MakeDirty();

   return row;
   }


void ReportsGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
   {
   CUGCell cell;
   GetCell( col, row, &cell );
   
   int nParam = cell.GetParam();
   
   CString string;
   if(cell.GetLabelText() != NULL)
      string = cell.GetLabelText();

   //if(processed)
   //   {
   //   if(cell.GetCellType() == m_nSpinIndex)
   //      return ;
   //   }
   if( nParam == CELLTYPE_IS_EDITABLE || string == "CELLTYPE_IS_EDITABLE")
      {
      StartEdit();
      }
   }



/////////////////////////////////////////////////////////////////////////////
//   OnCellTypeNotify
//      This notification is sent by the celltype and it is different from cell-type
//      to celltype and even from notification to notification.  It is usually used to
//      provide the developer with some feed back on the cell events and sometimes to
//      ask the developer if given event is to be accepted or not
//   Params:
//      ID         - celltype ID
//      col, row   - co-ordinates cell that is processing the message
//      msg         - message ID to identify current process
//      param      - additional information or object that might be needed
//   Return:
//      TRUE - to allow celltype event
//      FALSE - to disallow the celltype event
int ReportsGrid::OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param)
   {
   UNREFERENCED_PARAMETER(param);

   if ( ID == m_ellipsisIndex )
      {
      CUGCell cell;
      GetCell(col,row,&cell);
      int nParam = cell.GetParam();

      if( msg == UGCT_ELLIPSISBUTTONCLICK )
         {
         TCHAR buffer[ 256 ];
         lstrcpy( buffer, cell.GetText() );

         if ( ::EnvRunQueryBuilderDlg( buffer, 255 ) == IDOK )
            {
            this->QuickSetText( col, row, buffer );
            m_pParent->MakeDirty();
            }
         }
      }

   return TRUE;
   }


/////////////////////////////////////////////////////////////////////////////
//   OnEditStart
//      This message is sent whenever the grid is ready to start editing a cell
//   Params:
//      col, row - location of the cell that edit was requested over
//      edit -   pointer to a pointer to the edit control, allows for swap of edit control
//            if edit control is swapped permanently (for the whole grid) is it better
//            to use 'SetNewEditClass' function.
//   Return:
//      TRUE - to allow the edit to start
//      FALSE - to prevent the edit from starting

int ReportsGrid::OnEditStart(int col, long row,CWnd **edit)
   {
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(**edit);

   if ( ! m_isEditable )
      return FALSE;

   // set any needed masks

   // mask edit control will be set once a cell has mask string set
   // we always want the initials to be in uppercase, modify the style of the edit control itself  

   //if( col == 1 )
   //   (*edit)->ModifyStyle( 0, ES_UPPERCASE ); 
   //else 
   //   (*edit)->ModifyStyle( ES_UPPERCASE, 0 );

   return TRUE; 
   } 


/////////////////////////////////////////////////////////////////////////////
//   OnEditVerify
//      This notification is sent every time the user hits a key while in edit mode.
//      It is mostly used to create custom behavior of the edit control, because it is
//      so easy to allow or disallow keys hit.
//   Params:
//      col, row   - location of the edit cell
//      edit      -   pointer to the edit control
//      vcKey      - virtual key code of the pressed key
//   Return:
//      TRUE - to accept pressed key
//      FALSE - to do not accept the key
int ReportsGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey)
{
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(*edit);
   UNREFERENCED_PARAMETER(*vcKey);
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   OnEditFinish
//      This notification is sent when the edit is being finished
//   Params:
//      col, row   - coordinates of the edit cell
//      edit      - pointer to the edit control
//      string      - actual string that user typed in
//      cancelFlag   - indicates if the edit is being canceled
//   Return:
//      TRUE - to allow the edit to proceed
//      FALSE - to force the user back to editing of that same cell
int ReportsGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag)
{
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(*edit);
   UNREFERENCED_PARAMETER(string);
   UNREFERENCED_PARAMETER(cancelFlag);
   m_pParent->MakeDirty();
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   OnMenuCommand
//      This notification is called when the user has selected a menu item
//      in the pop-up menu.
//   Params:
//      col, row - the cell coordinates of where the menu originated from
//      section - identify for which portion of the gird the menu is for.
//              possible sections:
//                  UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
//                  UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
//      item - ID of the menu item selected
//   Return:
//      <none>

void ReportsGrid::OnMenuCommand(int col,long row,int section,int item)
   {
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);

   if ( section == UG_GRID )
      {
      switch( item )
         {
         case 1000:
            CutSelected();
            break;

         case 2000:
            CopySelected();
            break;

         case 3000:     // paste
            Paste(); 
            break;
         }
      }
   } 

/////////////////////////////////////////////////////////////////////////////
//   OnHint
//      Is called whent the hint is about to be displayed on the main grid area,
//      here you have a chance to set the text that will be shown
//   Params:
//      col, row   - the cell coordinates of where the menu originated from
//      string      - pointer to the string that will be shown
//   Return:
//      TRUE - to show the hint
//      FALSE - to prevent the hint from showing
int ReportsGrid::OnHint(int col,long row,int section,CString *string)
{
   UNREFERENCED_PARAMETER(section);
   string->Format( _T("Col:%d Row:%ld") ,col,row);
   return TRUE;
}


//////// ConstantsGrid ////////////////////////////////////////////

BEGIN_MESSAGE_MAP(ConstantsGrid,CUGCtrl)
   //{{AFX_MSG_MAP(ScoreGrid)
      // NOTE - the ClassWizard will add and remove mapping macros here.
      //    DO NOT EDIT what you see in these blocks of generated code !
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


ConstantsGrid::ConstantsGrid()
: CUGCtrl()
, m_isEditable( true )
{ }


ConstantsGrid::~ConstantsGrid()
   {
   }


int ConstantsGrid::AppendRow( LPCTSTR name, LPCTSTR expr )
   {
   CUGCtrl::AppendRow();

   int row = this->GetNumberRows() - 1;

   // set up col 0 (name)
   CUGCell cell0;
   GetCell( 0, row, &cell0); 
   cell0.SetText( name ); //_T("<Constant Name (no blanks allowed)>") );
   cell0.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell(0, row, &cell0); 

   // set up col 0 (name)
   CUGCell cell1;
   GetCell( 1, row, &cell1); 
   cell1.SetText( expr );  //_T("<value (number)>") );
   cell1.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell(1, row, &cell1); 
   
   this->GotoCell( 0, row ); 
   RedrawRow( row );

   m_pParent->MakeDirty();

   return row;
   }


void ConstantsGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
   {
   CUGCell cell;
   GetCell( col, row, &cell );
   
   int nParam = cell.GetParam();
   
   CString string;
   if(cell.GetLabelText() != NULL)
      string = cell.GetLabelText();

   //if(processed)
   //   {
   //   if(cell.GetCellType() == m_nSpinIndex)
   //      return ;
   //   }
   if( nParam == CELLTYPE_IS_EDITABLE || string == "CELLTYPE_IS_EDITABLE")
      {
      StartEdit();
      }
   }


/////////////////////////////////////////////////////////////////////////////
//   OnSetup
//      This function is called just after the grid window 
//      is created or attached to a dialog item.
//      It can be used to initially setup the grid
void ConstantsGrid::OnSetup()
   {
   //EnableMenu(TRUE);
   SetNumberCols( 2 );  // name, value

   CUGCell cell;
   cell.SetText( _T("Name" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 0, -1, &cell );

   cell.SetText( _T("Value" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 1, -1, &cell );

   SetSH_Width( 0 ) ;

   CRect rect;
   GetClientRect( &rect );

   SetColWidth( 0, 160 );
   SetColWidth( 1, rect.right - 160 - 20 );
   }


/////////////////////////////////////////////////////////////////////////////
//   OnCellTypeNotify
//      This notification is sent by the celltype and it is different from cell-type
//      to celltype and even from notification to notification.  It is usually used to
//      provide the developer with some feed back on the cell events and sometimes to
//      ask the developer if given event is to be accepted or not
//   Params:
//      ID         - celltype ID
//      col, row   - co-ordinates cell that is processing the message
//      msg         - message ID to identify current process
//      param      - additional information or object that might be needed
//   Return:
//      TRUE - to allow celltype event
//      FALSE - to disallow the celltype event
int ConstantsGrid::OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param)
   {
   UNREFERENCED_PARAMETER(param);
   return TRUE;
   }


/////////////////////////////////////////////////////////////////////////////
//   OnEditStart
//      This message is sent whenever the grid is ready to start editing a cell
//   Params:
//      col, row - location of the cell that edit was requested over
//      edit -   pointer to a pointer to the edit control, allows for swap of edit control
//            if edit control is swapped permanently (for the whole grid) is it better
//            to use 'SetNewEditClass' function.
//   Return:
//      TRUE - to allow the edit to start
//      FALSE - to prevent the edit from starting

int ConstantsGrid::OnEditStart(int col, long row,CWnd **edit)
   {
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(**edit);

   if ( ! m_isEditable )
      return FALSE;

   // set any needed masks

   // mask edit control will be set once a cell has mask string set
   // we always want the initials to be in uppercase, modify the style of the edit control itself  

   //if( col == 1 )
   //   (*edit)->ModifyStyle( 0, ES_UPPERCASE ); 
   //else 
   //   (*edit)->ModifyStyle( ES_UPPERCASE, 0 );

   return TRUE; 
   } 


/////////////////////////////////////////////////////////////////////////////
//   OnEditVerify
//      This notification is sent every time the user hits a key while in edit mode.
//      It is mostly used to create custom behavior of the edit control, because it is
//      so easy to allow or disallow keys hit.
//   Params:
//      col, row   - location of the edit cell
//      edit      -   pointer to the edit control
//      vcKey      - virtual key code of the pressed key
//   Return:
//      TRUE - to accept pressed key
//      FALSE - to do not accept the key
int ConstantsGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey)
{
if ( col == 1 )
   {
   if ( _istdigit( *vcKey ) || *vcKey == '.' )
      return TRUE;
   return FALSE;
   }

if ( col == 0 && *vcKey == ' ' )
   return FALSE;

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   OnEditFinish
//      This notification is sent when the edit is being finished
//   Params:
//      col, row   - coordinates of the edit cell
//      edit      - pointer to the edit control
//      string      - actual string that user typed in
//      cancelFlag   - indicates if the edit is being canceled
//   Return:
//      TRUE - to allow the edit to proceed
//      FALSE - to force the user back to editing of that same cell
int ConstantsGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag)
{
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(*edit);
   UNREFERENCED_PARAMETER(string);
   UNREFERENCED_PARAMETER(cancelFlag);
   m_pParent->MakeDirty();
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   OnMenuCommand
//      This notification is called when the user has selected a menu item
//      in the pop-up menu.
//   Params:
//      col, row - the cell coordinates of where the menu originated from
//      section - identify for which portion of the gird the menu is for.
//              possible sections:
//                  UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
//                  UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
//      item - ID of the menu item selected
//   Return:
//      <none>

void ConstantsGrid::OnMenuCommand(int col,long row,int section,int item)
   {
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);

   if ( section == UG_GRID )
      {
      switch( item )
         {
         case 1000:
            CutSelected();
            break;

         case 2000:
            CopySelected();
            break;

         case 3000:     // paste
            Paste(); 
            break;
         }
      }
   } 

/////////////////////////////////////////////////////////////////////////////
//   OnHint
//      Is called whent the hint is about to be displayed on the main grid area,
//      here you have a chance to set the text that will be shown
//   Params:
//      col, row   - the cell coordinates of where the menu originated from
//      string      - pointer to the string that will be shown
//   Return:
//      TRUE - to show the hint
//      FALSE - to prevent the hint from showing
int ConstantsGrid::OnHint(int col,long row,int section,CString *string)
{
   UNREFERENCED_PARAMETER(section);
   string->Format( _T("Col:%d Row:%ld") ,col,row);
   return TRUE;
}



//////// AllocationsGrid ////////////////////////////////////////////

BEGIN_MESSAGE_MAP(AllocationsGrid,CUGCtrl)
   //{{AFX_MSG_MAP(ScoreGrid)
      // NOTE - the ClassWizard will add and remove mapping macros here.
      //    DO NOT EDIT what you see in these blocks of generated code !
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


AllocationsGrid::AllocationsGrid()
: CUGCtrl()
, m_isEditable( true )
{ }


AllocationsGrid::~AllocationsGrid()
   {
   }

int AllocationsGrid::AppendRow( LPCTSTR name, LPCTSTR query, LPCTSTR expr )
   {
   CUGCtrl::AppendRow();

   int row = this->GetNumberRows() - 1;

   // set up col 0 (name)
   CUGCell cell0;
   GetCell( 0, row, &cell0); 
   cell0.SetText( name ); //_T("<Name>") );
   cell0.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell(0, row, &cell0); 

   // query
   CUGCell cell1;
   GetCell( 1, row, &cell1 );
   cell1.SetCellType( m_ellipsisIndex );
   cell1.SetText( query );   // _T("<Specify query or blank for everywhere>" ) );
   cell1.SetAlignment( UG_ALIGNLEFT | UG_ALIGNVCENTER ); 
   cell1.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell( 1, row, &cell1 );

   // value
   CUGCell cell2;
   GetCell( 2, row, &cell2); 
   cell2.SetText( expr );   //_T("<value expression >") );
   cell2.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell(2, row, &cell2); 

   this->GotoCell( 0, row ); 
   RedrawRow( row );

   m_pParent->MakeDirty();

   return row;
   }


void AllocationsGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
   {
   CUGCell cell;
   GetCell( col, row, &cell );
   
   int nParam = cell.GetParam();
   
   CString string;
   if(cell.GetLabelText() != NULL)
      string = cell.GetLabelText();

   //if(processed)
   //   {
   //   if(cell.GetCellType() == m_nSpinIndex)
   //      return ;
   //   }
   if( nParam == CELLTYPE_IS_EDITABLE || string == "CELLTYPE_IS_EDITABLE")
      {
      StartEdit();
      }
   }


/////////////////////////////////////////////////////////////////////////////
//   OnSetup
//      This function is called just after the grid window 
//      is created or attached to a dialog item.
//      It can be used to initially setup the grid
void AllocationsGrid::OnSetup()
   {
   //EnableMenu(TRUE);
   m_ellipsisIndex = AddCellType( &m_ellipsisCtrl );

   SetNumberCols( 3 );  // name, query

   CUGCell cell;
   cell.SetText( _T("Name" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 0, -1, &cell );

   cell.SetText( _T("Query" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 1, -1, &cell );

   cell.SetText( _T("Value" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 2, -1, &cell );
   SetSH_Width( 0 ) ;

   CRect rect;
   GetClientRect( &rect );

   SetColWidth( 0, 150 );
   SetColWidth( 1, 150 );
   SetColWidth( 2, rect.right-300 );
   }


/////////////////////////////////////////////////////////////////////////////
//   OnCellTypeNotify
//      This notification is sent by the celltype and it is different from cell-type
//      to celltype and even from notification to notification.  It is usually used to
//      provide the developer with some feed back on the cell events and sometimes to
//      ask the developer if given event is to be accepted or not
//   Params:
//      ID         - celltype ID
//      col, row   - co-ordinates cell that is processing the message
//      msg         - message ID to identify current process
//      param      - additional information or object that might be needed
//   Return:
//      TRUE - to allow celltype event
//      FALSE - to disallow the celltype event
int AllocationsGrid::OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param)
   {
   UNREFERENCED_PARAMETER(param);

   if ( ID == m_ellipsisIndex )
      {
      CUGCell cell;
      GetCell(col,row,&cell);
      int nParam = cell.GetParam();

      if( msg == UGCT_ELLIPSISBUTTONCLICK )
         {
         TCHAR buffer[ 256 ];
         lstrcpy( buffer, cell.GetText() );

         if ( ::EnvRunQueryBuilderDlg( buffer, 255 ) == IDOK )
            {
            this->QuickSetText( col, row, buffer );
            m_pParent->MakeDirty();
            }
         }
      }

   return TRUE;
   }


/////////////////////////////////////////////////////////////////////////////
//   OnEditStart
//      This message is sent whenever the grid is ready to start editing a cell
//   Params:
//      col, row - location of the cell that edit was requested over
//      edit -   pointer to a pointer to the edit control, allows for swap of edit control
//            if edit control is swapped permanently (for the whole grid) is it better
//            to use 'SetNewEditClass' function.
//   Return:
//      TRUE - to allow the edit to start
//      FALSE - to prevent the edit from starting

int AllocationsGrid::OnEditStart(int col, long row,CWnd **edit)
   {
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(**edit);

   if ( ! m_isEditable )
      return FALSE;

   // set any needed masks

   // mask edit control will be set once a cell has mask string set
   // we always want the initials to be in uppercase, modify the style of the edit control itself  

   //if( col == 1 )
   //   (*edit)->ModifyStyle( 0, ES_UPPERCASE ); 
   //else 
   //   (*edit)->ModifyStyle( ES_UPPERCASE, 0 );

   return TRUE; 
   } 


/////////////////////////////////////////////////////////////////////////////
//   OnEditVerify
//      This notification is sent every time the user hits a key while in edit mode.
//      It is mostly used to create custom behavior of the edit control, because it is
//      so easy to allow or disallow keys hit.
//   Params:
//      col, row   - location of the edit cell
//      edit      -   pointer to the edit control
//      vcKey      - virtual key code of the pressed key
//   Return:
//      TRUE - to accept pressed key
//      FALSE - to do not accept the key
int AllocationsGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey)
{
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(*edit);
   UNREFERENCED_PARAMETER(*vcKey);
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   OnEditFinish
//      This notification is sent when the edit is being finished
//   Params:
//      col, row   - coordinates of the edit cell
//      edit      - pointer to the edit control
//      string      - actual string that user typed in
//      cancelFlag   - indicates if the edit is being canceled
//   Return:
//      TRUE - to allow the edit to proceed
//      FALSE - to force the user back to editing of that same cell
int AllocationsGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag)
{
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(*edit);
   UNREFERENCED_PARAMETER(string);
   UNREFERENCED_PARAMETER(cancelFlag);
   m_pParent->MakeDirty();
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   OnMenuCommand
//      This notification is called when the user has selected a menu item
//      in the pop-up menu.
//   Params:
//      col, row - the cell coordinates of where the menu originated from
//      section - identify for which portion of the gird the menu is for.
//              possible sections:
//                  UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
//                  UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
//      item - ID of the menu item selected
//   Return:
//      <none>

void AllocationsGrid::OnMenuCommand(int col,long row,int section,int item)
   {
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);

   if ( section == UG_GRID )
      {
      switch( item )
         {
         case 1000:
            CutSelected();
            break;

         case 2000:
            CopySelected();
            break;

         case 3000:     // paste
            Paste(); 
            break;
         }
      }
   } 

/////////////////////////////////////////////////////////////////////////////
//   OnHint
//      Is called whent the hint is about to be displayed on the main grid area,
//      here you have a chance to set the text that will be shown
//   Params:
//      col, row   - the cell coordinates of where the menu originated from
//      string      - pointer to the string that will be shown
//   Return:
//      TRUE - to show the hint
//      FALSE - to prevent the hint from showing
int AllocationsGrid::OnHint(int col,long row,int section,CString *string)
{
   UNREFERENCED_PARAMETER(section);
   string->Format( _T("Col:%d Row:%ld") ,col,row);
   return TRUE;
}





//////// PreferencesGrid ////////////////////////////////////////////

BEGIN_MESSAGE_MAP(PreferencesGrid,CUGCtrl)
   //{{AFX_MSG_MAP(ScoreGrid)
      // NOTE - the ClassWizard will add and remove mapping macros here.
      //    DO NOT EDIT what you see in these blocks of generated code !
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


PreferencesGrid::PreferencesGrid()
: CUGCtrl()
, m_isEditable( true )
, m_pAllocSet( NULL )
{ }


PreferencesGrid::~PreferencesGrid()
   {
   }

int PreferencesGrid::AppendRow( LPCTSTR name, LPCTSTR query, LPCTSTR expr )
   {
   CUGCtrl::AppendRow();

   int row = this->GetNumberRows() - 1;

   // set up col 0 (name)
   CUGCell cell0;
   GetCell( 0, row, &cell0); 
   cell0.SetText( name );   //_T("<Name>") );
    cell0.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell(0, row, &cell0); 

   // query
   CUGCell cell1;
   GetCell( 1, row, &cell1 );
   cell1.SetCellType( m_ellipsisIndex );
   cell1.SetText( query );    //_T("<Specify query or blank for everywhere>" ) );
   cell1.SetAlignment( UG_ALIGNLEFT | UG_ALIGNVCENTER ); 
   cell1.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell( 1, row, &cell1 );

   // value
   CUGCell cell2;
   GetCell( 2, row, &cell2); 
   cell2.SetText( expr );     /// _T("<value expression >") );
   cell2.SetParam(CELLTYPE_IS_EDITABLE);
   SetCell(2, row, &cell2); 
   
   this->GotoCell( 0, row ); 
   RedrawRow( row );

   m_pParent->MakeDirty();

   return row;
   }


void PreferencesGrid::OnDClicked(int col,long row,RECT *rect,POINT *point,BOOL processed)
   {
   CUGCell cell;
   GetCell( col, row, &cell );
   
   int nParam = cell.GetParam();
   
   CString string;
   if(cell.GetLabelText() != NULL)
      string = cell.GetLabelText();

   //if(processed)
   //   {
   //   if(cell.GetCellType() == m_nSpinIndex)
   //      return ;
   //   }
   if( nParam == CELLTYPE_IS_EDITABLE || string == "CELLTYPE_IS_EDITABLE")
      {
      StartEdit();
      }
   }


/////////////////////////////////////////////////////////////////////////////
//   OnSetup
//      This function is called just after the grid window 
//      is created or attached to a dialog item.
//      It can be used to initially setup the grid
void PreferencesGrid::OnSetup()
   {
   //EnableMenu(TRUE);
   m_ellipsisIndex = AddCellType( &m_ellipsisCtrl );

   SetNumberCols( 3 );  // name, query

   CUGCell cell;
   cell.SetText( _T("Name" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 0, -1, &cell );

   cell.SetText( _T("Query" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 1, -1, &cell );

   cell.SetText( _T("Value" ) );
   cell.SetBackColor( RGB( 220,220,220 ) );
   SetCell( 2, -1, &cell );
   SetSH_Width( 0 ) ;

   CRect rect;
   GetClientRect( &rect );

   SetColWidth( 0, 150 );
   SetColWidth( 1, 150 );
   SetColWidth( 2, rect.right - 300 );
   }


/////////////////////////////////////////////////////////////////////////////
//   OnCellTypeNotify
//      This notification is sent by the celltype and it is different from cell-type
//      to celltype and even from notification to notification.  It is usually used to
//      provide the developer with some feed back on the cell events and sometimes to
//      ask the developer if given event is to be accepted or not
//   Params:
//      ID         - celltype ID
//      col, row   - co-ordinates cell that is processing the message
//      msg         - message ID to identify current process
//      param      - additional information or object that might be needed
//   Return:
//      TRUE - to allow celltype event
//      FALSE - to disallow the celltype event
int PreferencesGrid::OnCellTypeNotify(long ID,int col,long row,long msg,LONG_PTR param)
   {
   UNREFERENCED_PARAMETER(param);

   if ( ID == m_ellipsisIndex )
      {
      CUGCell cell;
      GetCell(col,row,&cell);
      int nParam = cell.GetParam();

      if( msg == UGCT_ELLIPSISBUTTONCLICK )
         {
         TCHAR buffer[ 256 ];
         lstrcpy( buffer, cell.GetText() );

         if ( ::EnvRunQueryBuilderDlg( buffer, 255 ) == IDOK )
            {
            this->QuickSetText( col, row, buffer );
            m_pParent->MakeDirty();
            }
         }
      }

   return TRUE;
   }


/////////////////////////////////////////////////////////////////////////////
//   OnEditStart
//      This message is sent whenever the grid is ready to start editing a cell
//   Params:
//      col, row - location of the cell that edit was requested over
//      edit -   pointer to a pointer to the edit control, allows for swap of edit control
//            if edit control is swapped permanently (for the whole grid) is it better
//            to use 'SetNewEditClass' function.
//   Return:
//      TRUE - to allow the edit to start
//      FALSE - to prevent the edit from starting

int PreferencesGrid::OnEditStart(int col, long row,CWnd **edit)
   {
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(**edit);

   if ( ! m_isEditable )
      return FALSE;

   // set any needed masks

   // mask edit control will be set once a cell has mask string set
   // we always want the initials to be in uppercase, modify the style of the edit control itself  

   //if( col == 1 )
   //   (*edit)->ModifyStyle( 0, ES_UPPERCASE ); 
   //else 
   //   (*edit)->ModifyStyle( ES_UPPERCASE, 0 );

   return TRUE; 
   } 


/////////////////////////////////////////////////////////////////////////////
//   OnEditVerify
//      This notification is sent every time the user hits a key while in edit mode.
//      It is mostly used to create custom behavior of the edit control, because it is
//      so easy to allow or disallow keys hit.
//   Params:
//      col, row   - location of the edit cell
//      edit      -   pointer to the edit control
//      vcKey      - virtual key code of the pressed key
//   Return:
//      TRUE - to accept pressed key
//      FALSE - to do not accept the key
int PreferencesGrid::OnEditVerify(int col, long row,CWnd *edit,UINT *vcKey)
{
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(*edit);
   UNREFERENCED_PARAMETER(*vcKey);
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   OnEditFinish
//      This notification is sent when the edit is being finished
//   Params:
//      col, row   - coordinates of the edit cell
//      edit      - pointer to the edit control
//      string      - actual string that user typed in
//      cancelFlag   - indicates if the edit is being canceled
//   Return:
//      TRUE - to allow the edit to proceed
//      FALSE - to force the user back to editing of that same cell
int PreferencesGrid::OnEditFinish(int col, long row,CWnd *edit,LPCTSTR string,BOOL cancelFlag)
{
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);
   UNREFERENCED_PARAMETER(*edit);
   UNREFERENCED_PARAMETER(string);
   UNREFERENCED_PARAMETER(cancelFlag);
   m_pParent->MakeDirty();
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//   OnMenuCommand
//      This notification is called when the user has selected a menu item
//      in the pop-up menu.
//   Params:
//      col, row - the cell coordinates of where the menu originated from
//      section - identify for which portion of the gird the menu is for.
//              possible sections:
//                  UG_TOPHEADING, UG_SIDEHEADING,UG_GRID
//                  UG_HSCROLL  UG_VSCROLL  UG_CORNERBUTTON
//      item - ID of the menu item selected
//   Return:
//      <none>

void PreferencesGrid::OnMenuCommand(int col,long row,int section,int item)
   {
   UNREFERENCED_PARAMETER(col);
   UNREFERENCED_PARAMETER(row);

   if ( section == UG_GRID )
      {
      switch( item )
         {
         case 1000:
            CutSelected();
            break;

         case 2000:
            CopySelected();
            break;

         case 3000:     // paste
            Paste(); 
            break;
         }
      }
   } 

/////////////////////////////////////////////////////////////////////////////
//   OnHint
//      Is called whent the hint is about to be displayed on the main grid area,
//      here you have a chance to set the text that will be shown
//   Params:
//      col, row   - the cell coordinates of where the menu originated from
//      string      - pointer to the string that will be shown
//   Return:
//      TRUE - to show the hint
//      FALSE - to prevent the hint from showing
int PreferencesGrid::OnHint(int col,long row,int section,CString *string)
{
   UNREFERENCED_PARAMETER(section);
   string->Format( _T("Col:%d Row:%ld") ,col,row);
   return TRUE;
}

