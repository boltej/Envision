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
// DeltaViewer.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
//#include "Envsion.h"
#include ".\deltaviewer.h"
#include "Envdoc.h"
#include <Policy.h>
#include <EnvModel.h>
#include <deltaArray.h>
#include <DataManager.h>
#include "SelectIduDiag.h"
#include "DeltaReport.h"

#include <algorithm>

#include <maplayer.h>

extern MapLayer      *gpCellLayer;
extern CEnvDoc       *gpDoc;
extern EnvModel      *gpModel;
extern PolicyManager *gpPolicyManager;


// DeltaViewer dialog

IMPLEMENT_DYNAMIC(DeltaViewer, CDialog)

DeltaViewer::DeltaViewer(CWnd* pParent /*=NULL*/, int deltaArrayIndex /*=-1*/, bool selectedOnly /*=false*/ )
   : CDialog(DeltaViewer::IDD, pParent),
   m_selectedDeltaArray( deltaArrayIndex ),
   m_selectedOnly( selectedOnly ),
   m_run( 0 ),
   m_lastRowYear( -1 ),
   m_highlighted( true )
   {
   if ( deltaArrayIndex == -1 )
      m_selectedDeltaArray= gpModel->m_pDataManager->GetDeltaArrayCount() - 1;

   // create lock for m_lastGoodPosition
   m_ctrlStop = false;
   m_ctrlMutex = new ::CMutex();

   pThread = 0;

   int count = gpCellLayer->GetSelectionCount();
   m_selectedIDUs.SetSize(count);

   for(int i = 0; i < count; i++)
      {
      int idu = gpCellLayer->GetSelection(i);
      m_selectedIDUs.SetAt(i, idu);
      }

   //m_iduCountStatic.Format(_T("Selected: %d"), count);
   }


DeltaViewer::~DeltaViewer()
{
}


void DeltaViewer::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LISTVIEW, m_listView);
DDX_Control(pDX, IDC_TREE, m_tree);
DDX_Control(pDX, IDC_FIELDS, m_fields);
//DDX_Text(pDX, IDC_RUN, m_run);
DDX_Control(pDX, IDC_RUN, m_runs);
//DDX_Text(pDX, IDC_IDUCOUNT, m_iduCountStatic);
}


BEGIN_MESSAGE_MAP(DeltaViewer, CDialog)
   ON_LBN_SELCHANGE(IDC_RUNS, OnLbnSelchangeRuns)
   //ON_WM_SIZING()
   ON_BN_CLICKED(IDC_EVALMODEL, OnBnClicked)
   ON_BN_CLICKED(IDC_AUTOPROCESS, OnBnClicked)
   ON_BN_CLICKED(IDC_LULC, OnBnClicked)
   ON_BN_CLICKED(IDC_OTHER, OnBnClicked)
   ON_BN_CLICKED(IDC_POLICIES, OnBnClicked)
   ON_BN_CLICKED(IDC_SELIDUONLY, OnBnClicked)
   ON_WM_SIZE()
   ON_NOTIFY(LVN_GETDISPINFO, IDC_LISTVIEW, OnGetDispInfo)
   ON_BN_CLICKED(IDC_SELECTIDUBTN, &DeltaViewer::OnBnClickedSelectidubtn)
   ON_BN_CLICKED(IDC_SUMMARY, &DeltaViewer::OnBnClickedSummary)
   ON_BN_CLICKED(IDC_SELECTALLFIELDS, &DeltaViewer::OnBnClickedSelectallfields)
   ON_BN_CLICKED(IDC_UNSELECTALLFIELDS, &DeltaViewer::OnBnClickedUnselectallfields)
   ON_NOTIFY(NM_CLICK, IDC_TREE, &DeltaViewer::OnNMClickTree)
   ON_LBN_SELCHANGE(IDC_FIELDS, &DeltaViewer::OnLbnSelchangeFields)
   ON_NOTIFY( NM_CUSTOMDRAW, IDC_LISTVIEW, &DeltaViewer::OnNMCustomdraw )
END_MESSAGE_MAP()




// DeltaViewer message handlers

BOOL DeltaViewer::OnInitDialog()
   {
   CWaitCursor c;
   CDialog::OnInitDialog();

   INIT_EASYSIZE;

   // hack to fix checkbox bug
   m_tree.ModifyStyle (TVS_CHECKBOXES, 0);
   m_tree.ModifyStyle (0, TVS_CHECKBOXES);

   // load runs
   for ( int i=0; i < gpModel->m_pDataManager->GetDeltaArrayCount(); i++ )
      {
      char text[ 16 ];
      sprintf_s( text, 16, "%i", i );
      m_runs.AddString( text );
      }

   m_runs.SetCurSel( m_selectedDeltaArray );

   // set up list control
   m_listView.InsertColumn( 0, "Item", LVCFMT_LEFT, 120, -1 );
   m_listView.InsertColumn( 1, "IDU", LVCFMT_LEFT, 60, -1 );
   m_listView.InsertColumn( 2, "Source", LVCFMT_LEFT, 60, -1 );
   m_listView.InsertColumn( 3, "Date", LVCFMT_LEFT, 60, -1 );
   m_listView.InsertColumn( 4, "Starting Value", LVCFMT_LEFT, 120, -1 );
   m_listView.InsertColumn( 5, "Ending Value", LVCFMT_LEFT, 120, -1 );
   m_listView.InsertColumn( 6, "Policy Site Constraints", LVCFMT_LEFT, 120, -1 );
   m_listView.InsertColumn( 7, "Policy Outcome", LVCFMT_LEFT, 120, -1 );
   m_listView.InsertColumn( 8, "Notes", LVCFMT_LEFT, 120, -1 );

   CheckDlgButton( IDC_SELIDUONLY, m_selectedOnly ? 1 : 0 );

   // load tree
   m_hItemPolicies = m_tree.InsertItem( "Policies" );
   m_tree.SetCheck( m_hItemPolicies, 1 );
   
   m_hItemLulc     = m_tree.InsertItem( "LULC" );
   m_tree.SetCheck( m_hItemLulc, 1 );

   m_hItemEvaluators = m_tree.InsertItem( "Eval Models" );
   m_hItemAutoProcs  = m_tree.InsertItem( "Auto Processes" );
   BOOL ok = m_tree.SetCheck( m_hItemEvaluators, 1 );
   ok = m_tree.SetCheck( m_hItemAutoProcs, 1 );

   m_hItemOther = m_tree.InsertItem( "Other" );
   ok = m_tree.SetCheck( m_hItemOther, 1 );

   for ( int i=0; i < gpModel->GetEvaluatorCount(); i++ )
      {
      EnvEvaluator *pInfo = gpModel->GetEvaluatorInfo( i );
      CString label;
      label.Format( "%s (%i)", pInfo->m_name, pInfo->m_handle );
      HTREEITEM hItem = m_tree.InsertItem( label, m_hItemEvaluators );
      ok = m_tree.SetCheck( hItem, 1 );
      }

   for ( int i=0; i < gpModel->GetModelProcessCount(); i++ )
      {
      EnvModelProcess *pInfo = gpModel->GetModelProcessInfo( i );
      CString label;
      label.Format( "%s (%i)", pInfo->m_name, pInfo->m_handle );
      HTREEITEM hItem = m_tree.InsertItem( label, m_hItemAutoProcs );
      ok = m_tree.SetCheck( hItem, 1 );
      }

   // load fields
   for( int i=0; i < gpCellLayer->GetFieldCount(); i++ )
      {
      int index = m_fields.AddString( gpCellLayer->GetFieldLabel( i ) );
      m_fields.SetCheck( index, 1 );
      m_fields.SetItemData( index, i );
      }

   Load();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }

void DeltaViewer::OnLbnSelchangeRuns()
   {
   m_selectedDeltaArray = this->m_runs.GetCurSel();

   Load();
   }



// handle mouse click on check box 

void DeltaViewer::OnNMClickTree(NMHDR *pNMHDR, LRESULT *pResult)
   {
   HTREEITEM item;
   UINT flags;
 
   // verify that we have a mouse click in the check box area
   DWORD pos = GetMessagePos();
 
   CPoint point(LOWORD(pos), HIWORD(pos));
   m_tree.ScreenToClient(&point);
 
   item = m_tree.HitTest(point, &flags); 

   if ( item && (flags & TVHT_ONITEMSTATEICON))
      {
      // clicked on the checkbox - get state
      if ( item == m_hItemEvaluators || item ==  m_hItemAutoProcs )
         {
         BOOL isChecked = m_tree.GetCheck( item ) ? FALSE : TRUE;   // before toggled state, so switch for children

         HTREEITEM hChildItem = m_tree.GetChildItem( item );
         while ( hChildItem != NULL )
            {
            m_tree.SetCheck( hChildItem, isChecked );
            hChildItem = m_tree.GetNextItem( hChildItem, TVGN_NEXT );
            }
         }

      Load();
      }

   *pResult = 0;
   } 


void DeltaViewer::Load()
   {
   if ( pThread )
      {
      if ( m_ctrlMutex->Lock(10) == 0 )
         {
         // send message to re-call Load()
         SendMessage(BN_CLICKED);
         return;
         }
         
      m_ctrlStop = true;
      m_ctrlMutex->Unlock();

      ::WaitForSingleObject(pThread->m_hThread, 2000);
      }

   m_listView.DeleteAllItems();

   m_deltaBuffer.clear();

   LVITEM item;
   item.mask = LVIF_TEXT;
   int items = 0;

   int run = m_runs.GetCurSel();
   DeltaArray *pDeltaArray = gpModel->m_pDataManager->GetDeltaArray( run );      

   bool showSelectedIDUs = IsDlgButtonChecked( IDC_SELIDUONLY ) ? true : false;
   bool showPolicies   = m_tree.GetCheck( m_hItemPolicies   ) ? true : false;
   bool showLulc       = m_tree.GetCheck( m_hItemLulc       ) ? true : false;
   bool showEvaluators = m_tree.GetCheck( m_hItemEvaluators ) ? true : false;
   bool showProcesses  = m_tree.GetCheck( m_hItemAutoProcs  ) ? true : false;
   bool showOther      = m_tree.GetCheck( m_hItemOther      ) ? true : false;

   m_listView.SetItemCountEx(0, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

   m_deltaBuffer.reserve(pDeltaArray->GetSize());

   FilterThreadParams *ftp = new FilterThreadParams;
   ftp->pDeltaBuffer = &m_deltaBuffer;
   ftp->run = run;

   ftp->showSelectedIDUs= showSelectedIDUs;
   ftp->showPolicies    = showPolicies;
   ftp->showLulc        = showLulc;
   ftp->showEvaluators  = showEvaluators;
   ftp->showProcesses   = showProcesses;
   ftp->showOther       = showOther;

   // do models
   int evalModelCount = gpModel->GetEvaluatorCount();
   ftp->showModelArray.SetSize( evalModelCount );

   int index = 0;
   if ( evalModelCount > 0 )
      {
      // see which models are active
      HTREEITEM hChildItem = m_tree.GetChildItem( m_hItemEvaluators );
      while ( hChildItem != NULL )
         {
         bool isChecked = m_tree.GetCheck( hChildItem ) ? true : false;
         ftp->showModelArray.SetAt( index++, isChecked ); 
         hChildItem = m_tree.GetNextItem( hChildItem, TVGN_NEXT );
         }
      }

   // do autoprocesses
   int apCount = gpModel->GetModelProcessCount();
   ftp->showAPArray.SetSize( apCount );

   index = 0;
   if ( apCount > 0 )
      {
      HTREEITEM hChildItem = m_tree.GetChildItem( m_hItemAutoProcs );
      while ( hChildItem != NULL )
         {
         bool isChecked = m_tree.GetCheck( hChildItem ) ? true : false;
         ftp->showAPArray.SetAt( index++, isChecked ); 
         hChildItem = m_tree.GetNextItem( hChildItem, TVGN_NEXT );
         }
      }
   
   // do fields
   int fieldCount = gpCellLayer->GetFieldCount();
   ftp->showFieldArray.SetSize( fieldCount );

   for ( int i=0; i < fieldCount; i++ )
      {
      CString field;
      m_fields.GetText( i, field );
      
      int col =(int)  m_fields.GetItemData( i );
      //int col = gpCellLayer->GetFieldCol( field );
      bool isChecked = m_fields.GetCheck( i ) ? true : false;
      ftp->showFieldArray[ col ] = isChecked;
      }

   ftp->ctrlMutex      = m_ctrlMutex;
   ftp->pListCtrl      = &m_listView;

   m_ctrlStop = false;
   ftp->ctrlStop       = &m_ctrlStop;

   ftp->selectedIDUs   = &m_selectedIDUs;

   pThread = AfxBeginThread(FilterByOpts, ftp);
   }



UINT DeltaViewer::FilterByOpts( LPVOID pParam )
   {
   FilterThreadParams* params = (FilterThreadParams*)pParam;

   if (params == NULL)
      return 1;   // if pObject is not valid

   int colPolicyID = gpModel->m_colPolicy;
   int colLulcA    = gpModel->m_colLulcA;
   int colLulcB    = gpModel->m_colLulcB;
   int colLulcC    = gpModel->m_colLulcC;
   int colLulcD    = gpModel->m_colLulcD;

   int colScore      = gpModel->m_colScore;
   int colLastD      = gpModel->m_colLastDecision;
   int colPolicyApps = gpModel->m_colPolicyApps;
   int items = 0;

   DeltaArray *pDeltaArray = gpModel->m_pDataManager->GetDeltaArray( params->run );  

   bool showSelectedIDUs = params->showSelectedIDUs;
   bool showPolicies    = params->showPolicies;
   bool showLulc        = params->showLulc;
   bool showEvaluators  = params->showEvaluators;
   bool showProcesses   = params->showProcesses;

   bool *showModelArray = params->showModelArray.GetData();
   bool *showAPArray    = params->showAPArray.GetData();
   bool *showFieldArray = params->showFieldArray.GetData();
   
   bool showOther       = params->showOther;

   int count = 0;

   // sort selected IDUs to prepare for binary search
   int seliducount = (int) params->selectedIDUs->GetCount();
   int *begin = params->selectedIDUs->GetData();
   int *end = begin + seliducount;

   std::sort(begin, end);


   // start with initial values




   for( INT_PTR i=0; i < pDeltaArray->GetCount(); i++)
      {
      //int  AddDelta( int cell, int col, int year, VData newValue, DELTA_TYPE type );
      DELTA &delta = pDeltaArray->GetAt( i );
      int col = delta.col;
      bool other = true;

      // if col not checked, skip 
      if ( showFieldArray[ col ] == false )
         continue;
      
      // skip these columns
      if ( col == colScore || col == colLastD ||  col == colPolicyApps )
         continue;

      // don't show policies if checked off
      else if ( col == colPolicyID && showPolicies == false )
         continue;

      // don't show LULC's if checked off
      else if ( ( col == colLulcA || col == colLulcB || col == colLulcC || col == colLulcD ) && showLulc == false )
         continue;

      // don't show eval model results is checked OFF
      else if ( delta.type >= DT_MODEL && delta.type <= DT_MODEL_END && showModelArray[ delta.type-DT_MODEL ] == false )
         continue;

        // don't show autonomous process results is checked OFF
      else if ( delta.type >= DT_PROCESS && delta.type <= DT_PROCESS_END && showAPArray[ delta.type-DT_PROCESS ] == false )
         continue;

      // don't show anything else if other checked off
      bool isOther = true;
      if ( col == colPolicyID ) isOther = false;
      if ( col == colLulcA || col == colLulcB || col == colLulcC || col == colLulcD ) isOther = false;
      if ( delta.type >= DT_MODEL && delta.type <= DT_PROCESS_END ) isOther = false;
      if ( ( !showOther ) && isOther )
         continue;

      // If its not a selected IDU, and selected IDUs only is turned on,
      // skip this one. 
      if ( showSelectedIDUs )
         {
         if( std::binary_search(begin, end, delta.cell) == false)
            continue;
         }

      // If we're still here, add the delta to the buffer
      params->pDeltaBuffer->push_back(&delta);
      count++;

      // Every 100 results, check the mutex to see if we need to stop now.


      // Every 1000 results, send the SetItemCount message to update the list
      if(count % 1000 == 0)
         {
         params->ctrlMutex->Lock();
         if(*(params->ctrlStop))
            {
            params->ctrlMutex->Unlock();
            break;
            }
         else
            {
            params->pListCtrl->SetItemCountEx(count, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
            params->ctrlMutex->Unlock();
            }
         }

      else if (count % 100 == 0)
         {
         params->ctrlMutex->Lock();
         if(*(params->ctrlStop))
            break;
         params->ctrlMutex->Unlock();
         }
      }

   // Make sure the list is fully up to date
   if ( params->ctrlMutex->Lock(10) )
      {
      if(*(params->ctrlStop))
         {
         params->ctrlMutex->Unlock();
         }
      else
         {
         params->pListCtrl->SetItemCountEx(count, 
            LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

         params->ctrlMutex->Unlock();
         }
      }

   delete params;
   ::AfxEndThread( 0, TRUE ); 
   return 0;   // thread completed successfully
   }


void DeltaViewer::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult) 
   {
   LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

   LV_ITEM* pItem= &(pDispInfo)->item;

   int itemid = pItem->iItem;

   int colPolicyID = gpModel->m_colPolicy;
   int colLulcA    = gpModel->m_colLulcA;
   int colLulcB    = gpModel->m_colLulcB;
   int colLulcC    = gpModel->m_colLulcC;
   int colLulcD    = gpModel->m_colLulcD;

   int colScore      = gpModel->m_colScore;
   int colLastD      = gpModel->m_colLastDecision;
   int colPolicyApps = gpModel->m_colPolicyApps;
   int items = 0;  

   bool showPolicies   = m_tree.GetCheck( m_hItemPolicies   ) ? true : false;
   bool showLulc       = m_tree.GetCheck( m_hItemLulc       ) ? true : false;
   bool showEvaluators = m_tree.GetCheck( m_hItemEvaluators ) ? true : false;
   bool showProcesses  = m_tree.GetCheck( m_hItemAutoProcs  ) ? true : false;
   bool showOther      = m_tree.GetCheck( m_hItemOther      ) ? true : false;

   if ( pItem->mask & LVIF_TEXT )
      {
      //int run = m_runs.GetCurSel();
      //DeltaArray *pDeltaArray = gpModel->m_pDataManager->GetDeltaArray( run );      

      CString text;
      DELTA &delta = *m_deltaBuffer.at( itemid );

      int col = delta.col;
      int value, level;

      //-- item --//
      if ( pItem->iSubItem == 0)
         {
         if ( col == colPolicyID && showPolicies )
            {
            int policyID;
            delta.newValue.GetAsInt( policyID );

            Policy *pPolicy = gpPolicyManager->GetPolicyFromID( policyID );
            text.Format( "%s (%i)", (LPCTSTR) pPolicy->m_name, policyID );
            }

         else if ( col == colLulcA && showLulc  )
            text.Format( "[%s] Change", gpModel->m_lulcTree.GetFieldName( 1 ) );
         else if ( col == colLulcB && showLulc )
            text.Format( "[%s] Change", gpModel->m_lulcTree.GetFieldName( 2 ) );
         else if ( col == colLulcC && showLulc )
            text.Format( "[%s] Change", gpModel->m_lulcTree.GetFieldName( 3 ) );
         else if ( col == colLulcD && showLulc )
            text.Format( "[%s] Change", gpModel->m_lulcTree.GetFieldName( 4 ) );
         else
            text.Format( "[%s] Change", gpCellLayer->GetFieldLabel( col ) );
         }

      //-- IDU --//
      else if (pItem->iSubItem == 1)
         {
         char idu[ 16 ];
         sprintf_s( idu, 16, "%i", delta.cell );
         text = idu;
         }

      //-- TYPE --//
      else if (pItem->iSubItem == 2)
        {
        text = (LPSTR) GetDeltaTypeStr( delta.type );
        }
     
      //-- DATE --//
      else if (pItem->iSubItem == 3)
        {
        //SYSTEMTIME time;
        //GetSystemTime( &time );
        //char year[ 8 ];
        //sprintf_s( year, 8, "%i", time.wYear + delta.year );
        text.Format( "%i", delta.year );
        }

      //-- STARTING VALUE --//
      else if (pItem->iSubItem == 4)
        {
        // outcome start value
        int value, level;
        if ( col == colPolicyID )
           {
           delta.oldValue.GetAsInt( value );
           if ( value >= 0 )
               {
               Policy *pOldPolicy = gpPolicyManager->GetPolicy( value );
               text.Format( "%s (%i)", (LPCTSTR) pOldPolicy->m_name, value );
               }
            else
               text = "<no prior policy>";
            }
         else if ( col == colLulcA || col == colLulcB || col == colLulcC || col == colLulcD )
            {
            if ( col == colLulcA )      level = 1;
            else if ( col == colLulcB ) level = 2;
            else if ( col == colLulcC ) level = 3;
            else level = 4;

            delta.oldValue.GetAsInt( value );
            LulcNode *pNode = gpDoc->m_model.m_lulcTree.FindNode( level, value );
            ASSERT( pNode != NULL );
            char name[ 128 ];
            sprintf_s( name, 128, "%s (%i)", (LPCTSTR) pNode->m_name, value );
            text = name;
            }
         else
            {
            // is the an int column with categorical field info attached?
            bool hasFieldInfo = false;
            if ( ::IsInteger( gpCellLayer->GetFieldType( col ) ) )
               {
               MAP_FIELD_INFO *pInfo = gpCellLayer->FindFieldInfo( gpCellLayer->GetFieldLabel( col ) );
               if ( pInfo != NULL )
                  { int intVal; bool rtnFlag;
                  rtnFlag = delta.oldValue.GetAsInt( intVal ); ASSERT(rtnFlag);
                  FIELD_ATTR *pAttr = pInfo->FindAttribute( intVal );
                  if ( pAttr )
                     {
                     text.Format( "%s (%i)", (LPCTSTR) pAttr->label, intVal );
                     hasFieldInfo = true;
                     }
                  }
               }

            if ( ! hasFieldInfo )
               text = delta.oldValue.GetAsString();
            }
         }

      //-- ENDING VALUE --//
      else if (pItem->iSubItem == 5)
         {
         // outcome end value
         if ( col == colPolicyID )
            {
            delta.newValue.GetAsInt( value );
            if ( value >= 0 )
               {
               Policy *pNewPolicy = gpPolicyManager->GetPolicyFromID( value );
               char policy[ 128];
               sprintf_s( policy, 128, "%s (%i)", (LPCTSTR) pNewPolicy->m_name, value );
               text = policy;
               }
            else
               text = "<no prior policy>";
            }

         else if ( col == colLulcA || col == colLulcB || col == colLulcC )
            {
            if ( col == colLulcA )      level = 1;
            else if ( col == colLulcB ) level = 2;
            else if ( col == colLulcC ) level = 3;
            else level = 4;

            delta.newValue.GetAsInt( value );
            LulcNode *pNode = gpDoc->m_model.m_lulcTree.FindNode( level, value );
            if ( pNode != NULL )
               {
               char name[ 128 ];
               sprintf_s( name, 128, "%s (%i)", (LPCTSTR) pNode->m_name, value );
               text = name;
               }
            else
               text.Format( "Unknown LULC (%i)", value );
            }
         else
            {
            // is the an int column with categorical field info attached?
            bool hasFieldInfo = false;
            if ( ::IsInteger( gpCellLayer->GetFieldType( col ) ) )
               {
               MAP_FIELD_INFO *pInfo = gpCellLayer->FindFieldInfo( gpCellLayer->GetFieldLabel( col ) );
               if ( pInfo != NULL )
                  { int intVal; bool rtnFlag;
                  rtnFlag = delta.newValue.GetAsInt(intVal); ASSERT(rtnFlag);
                  FIELD_ATTR *pAttr = pInfo->FindAttribute( intVal );
                  if ( pAttr )
                     {
                     text.Format( "%s (%i)", (LPCTSTR) pAttr->label, intVal );
                     hasFieldInfo = true;
                     }
                  }
               }

            if ( ! hasFieldInfo )
               text = delta.newValue.GetAsString();
            }
         }

      // -- Policy Site Constraints --//
      else if (pItem->iSubItem == 6)
         {
         // site constraints
         if ( col == colPolicyID )
            {
            delta.newValue.GetAsInt( value );
            if ( value >= 0 )
               {
               Policy *pNewPolicy = gpPolicyManager->GetPolicyFromID( value );
               text = (LPSTR)(LPCTSTR)pNewPolicy->m_siteAttrStr;
               }
            else
               text = "<n/a>";
            }
         else
            text = "<n/a>";
         }

      //-- Policy Outcome" --//
      else if (pItem->iSubItem == 7)
         {
         // outcome string
         if ( col == colPolicyID )
            {
            delta.newValue.GetAsInt( value );
            if ( value >= 0 )
               {
               Policy *pNewPolicy = gpPolicyManager->GetPolicyFromID( value );
               text = (LPSTR)(LPCTSTR)pNewPolicy->m_outcomeStr;
               }
            else
               text = "<n/a>";
            }
         else
            text = "<n/a>";
         }

      //-- Notes:  --//
      else if (pItem->iSubItem == 8)
         {
         // if duplicate delta:
         if(delta.newValue == delta.oldValue)
            text = "Duplicate;";
         else
            text = "";
         }

      lstrcpyn(pItem->pszText, text, pItem->cchTextMax);
      }

    *pResult = 0;
   }


//void DeltaViewer::OnSizing(UINT fwSide, LPRECT pRect)
//   {
//   CDialog::OnSizing(fwSide, pRect);
//   CWnd *pOk = GetDlgItem( IDOK );
//   CWnd *pSingleIDU  = GetDlgItem(  IDC_SINGLEIDU );
//   CWnd *pPolicies   = GetDlgItem( IDC_POLICIES );
//   CWnd *pLulc       = GetDlgItem( IDC_LULC );
//   CWnd *pEvaluator  = GetDlgItem( IDC_EVALMODEL );
//   CWnd *pOther      = GetDlgItem( IDC_OTHER );
//
//   RECT rectButton, rectRuns;
//   pOk->GetWindowRect( &rectButton );
//   m_runs.GetWindowRect( &rectRuns );      // screen coords
//   ScreenToClient( &rectRuns );   
//
//   int widthButton  = rectButton.right - rectButton.left;
//   int heightButton = rectButton.bottom - rectButton.top;
//
//   int widthDlg  = pRect->right - pRect->left;
//   int heightDlg = pRect->bottom - pRect->top;
//
//   RECT rect;
//   GetClientRect( &rect );
//   widthDlg = rect.right;
//   heightDlg = rect.bottom;
//
//   int spacing = 2;
//   pOk->MoveWindow       ( spacing, heightDlg-spacing-heightButton, widthButton, heightButton );
//   pOther->MoveWindow    ( spacing, heightDlg - 2*(spacing+heightButton), widthButton, heightButton );
//   pEvaluator->MoveWindow( spacing, heightDlg - 3*(spacing+heightButton), widthButton, heightButton );
//   pLulc->MoveWindow     ( spacing, heightDlg - 4*(spacing+heightButton), widthButton, heightButton );
//   pPolicies->MoveWindow ( spacing, heightDlg - 5*(spacing+heightButton), widthButton, heightButton );
//   pSingleIDU->MoveWindow( spacing, heightDlg - 6*(spacing+heightButton), widthButton, heightButton );
//
//   rectRuns.bottom = heightDlg - 6*(spacing+heightButton) - spacing;
//   m_runs.MoveWindow( &rectRuns, TRUE );
//
//   m_listView.MoveWindow( spacing*2+widthButton, spacing, widthDlg-spacing*3-widthButton, heightDlg-2*spacing );
//   }

void DeltaViewer::OnBnClicked()
   {
   Load();
   }


BEGIN_EASYSIZE_MAP( DeltaViewer )
   EASYSIZE( IDC_RUN,          ES_BORDER, ES_BORDER,   ES_KEEPSIZE, ES_BORDER,   0 )
   EASYSIZE( IDC_TREE,         ES_BORDER, ES_BORDER,   ES_KEEPSIZE, ES_KEEPSIZE, 0 )
   EASYSIZE( IDC_FIELDS,       ES_BORDER, IDC_TREE,    ES_KEEPSIZE, ES_BORDER,   0 )

   EASYSIZE( IDC_SELECTALLFIELDS,   ES_BORDER, IDC_FIELDS, ES_KEEPSIZE, ES_BORDER,   0 )
   EASYSIZE( IDC_UNSELECTALLFIELDS, ES_BORDER, IDC_FIELDS, ES_KEEPSIZE, ES_BORDER,   0 )
   EASYSIZE( IDC_BAR,               ES_BORDER, IDC_FIELDS, ES_KEEPSIZE, ES_BORDER,   0 )

   EASYSIZE( IDC_SELIDUONLY,   ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0 )
   
   EASYSIZE( IDC_SELECTIDUBTN, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0 )
   EASYSIZE( IDC_SUMMARY,      ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0 )
   EASYSIZE( IDOK,             ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0 )
   EASYSIZE( IDC_LISTVIEW,     ES_BORDER, ES_BORDER, ES_BORDER, ES_BORDER, 0 )
 END_EASYSIZE_MAP


void DeltaViewer::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);

   UPDATE_EASYSIZE;
   }

void DeltaViewer::OnBnClickedSelectidubtn()
{
   POSITION pos = m_listView.GetFirstSelectedItemPosition();
   if(pos)
      {
      int ind = m_listView.GetNextSelectedItem(pos);
      CString data = (const char*)m_listView.GetItemText(ind, 1);

      char *t;
      long idu = strtol(data.GetString(), &t, 10);
      
      m_selectedDeltaIDU = idu;
      }
   else
      {
      m_selectedDeltaIDU = -1;
      }

   SelectIduDiag selidu(this);
   selidu.DoModal();

   int count = (int) m_selectedIDUs.GetCount();
   //m_iduCountStatic.Format(_T("Selected: %d"), count);
   UpdateData(false);
   Load();
   }

void DeltaViewer::OnBnClickedSummary()
   {
   int run = m_runs.GetCurSel();
   DeltaArray *pDeltaArray = gpModel->m_pDataManager->GetDeltaArray( run );      

   ASSERT( pDeltaArray != NULL );

   DeltaReport dlg( pDeltaArray );
   dlg.DoModal();
   }


void DeltaViewer::OnBnClickedSelectallfields()
   {
   for( int i=0; i < gpCellLayer->GetFieldCount(); i++ )
      m_fields.SetCheck( i, 1 );

   Load();
   }


void DeltaViewer::OnBnClickedUnselectallfields()
   {
   for( int i=0; i < gpCellLayer->GetFieldCount(); i++ )
      m_fields.SetCheck( i, 0 );

   Load();
   }


void DeltaViewer::OnLbnSelchangeFields()
   {
   Load();
   }


void DeltaViewer::OnNMCustomdraw( NMHDR *pNMHDR, LRESULT *pResult )
   {
   NMLVCUSTOMDRAW* cd = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

   *pResult = CDRF_DODEFAULT;

   switch( cd->nmcd.dwDrawStage)
      {
      case CDDS_PREPAINT:
         *pResult = CDRF_NOTIFYITEMDRAW;
         break;

      case CDDS_ITEMPREPAINT:
         {
         DELTA &delta = *m_deltaBuffer.at( cd->nmcd.dwItemSpec );

         if ( delta.year != m_lastRowYear )
            {
            m_highlighted = ! m_highlighted;
            m_lastRowYear = delta.year;
            }
         
         if ( m_highlighted ) //highlightRow)
            {
            COLORREF backgroundColor = RGB(240, 240, 240);
            cd->clrTextBk = backgroundColor;
            *pResult = CDRF_NEWFONT;
            }
         }
         break;

      default:
         break;
      }
   }


LPCTSTR DeltaViewer::GetDeltaTypeStr( short type )
   {
   switch( type )
      {
      case DT_NULL:           return _T("Null");
      case DT_POLICY:         return _T("Policy");
      case DT_SUBDIVIDE:      return _T("Subdivide");
      case DT_MERGE:          return _T("Merge");
      case DT_INCREMENT_COL:  return _T("IncrementCol");
      case DT_POP:            return _T("POP");
      }

   if ( type >= DT_MODEL && type < DT_PROCESS )
      {
      int index = type-DT_MODEL;
      ASSERT( index < gpModel->GetEvaluatorCount() );
      EnvEvaluator *pInfo = gpModel->GetEvaluatorInfo( index );
      return pInfo->m_name;
      }

   else if ( type >= DT_PROCESS )
      {
      int index = type-DT_PROCESS;
      ASSERT( index < gpModel->GetModelProcessCount() );
      EnvModelProcess *pInfo = gpModel->GetModelProcessInfo( index );
      return pInfo->m_name;
      }

   return _T("Unknown");
   }