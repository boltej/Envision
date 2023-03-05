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
// PPPolicyResults.cpp : implementation file
//

#include "stdafx.h"
#include "PPPolicyResults.h"
#include <EnvPolicy.h>
#include <EnvModel.h>
#include "envdoc.h"
#include <DataManager.h>
#include "MapPanel.h"

#include <deltaArray.h>
#include <map.h>


extern CEnvDoc       *gpDoc;
extern EnvModel      *gpModel;
extern MapPanel      *gpMapPanel;
extern PolicyManager *gpPolicyManager;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PPPolicyResults property page

IMPLEMENT_DYNCREATE(PPPolicyResults, CPropertyPage)

PPPolicyResults::PPPolicyResults( int cell, int run ) 
: CPropertyPage(PPPolicyResults::IDD)
, m_cell( cell )
, m_run( run )
{
	//{{AFX_DATA_INIT(PPPolicyResults)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

PPPolicyResults::~PPPolicyResults()
{
}

void PPPolicyResults::DoDataExchange(CDataExchange* pDX)
{
CPropertyPage::DoDataExchange(pDX);
DDX_Control(pDX, IDC_PREVCELL, m_prevCell);
DDX_Control(pDX, IDC_NEXTCELL, m_nextCell);
DDX_Control(pDX, IDC_FLASHCELL, m_flashCell);
DDX_Control(pDX, IDC_RESULTS, m_results);
}


BEGIN_MESSAGE_MAP(PPPolicyResults, CPropertyPage)
	//{{AFX_MSG_MAP(PPPolicyResults)
	ON_BN_CLICKED(IDC_NEXTCELL, OnNextcell)
	ON_BN_CLICKED(IDC_PREVCELL, OnPrevcell)
	ON_BN_CLICKED(IDC_FLASHCELL, OnFlashcell)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PPPolicyResults message handlers
BOOL PPPolicyResults::OnInitDialog() 
   {
	CPropertyPage::OnInitDialog();
   
   // set up list control
   m_results.InsertColumn( 0, "Item", LVCFMT_LEFT, 120, -1 );
   m_results.InsertColumn( 1, "Date", LVCFMT_LEFT, 60, -1 );
   m_results.InsertColumn( 2, "Starting Value", LVCFMT_LEFT, 120, -1 );
   m_results.InsertColumn( 3, "Ending Value", LVCFMT_LEFT, 120, -1 );
   m_results.InsertColumn( 4, "Policy Site Constraints", LVCFMT_LEFT, 120, -1 );
   m_results.InsertColumn( 5, "Policy Outcome", LVCFMT_LEFT, 120, -1 );

   Load();
 	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
   }


void PPPolicyResults::Load( void )
   {
   char cellStr[ 32 ];
   sprintf_s( cellStr, 32, "Show Results: Cell %i", m_cell );
   GetParent()->SetWindowText( cellStr );

   m_results.DeleteAllItems();

   LVITEM item;
   item.mask = LVIF_TEXT;
   int colPolicy   = gpModel->m_colPolicy;
   int colLulcA    = gpModel->m_colLulcA;
   int colLulcB    = gpModel->m_colLulcB;
   int colLulcC    = gpModel->m_colLulcC;

   int colScore      = gpModel->m_colScore;
   int colLastD      = gpModel->m_colLastDecision;
   int colPolicyApps = gpModel->m_colPolicyApps;
   int colAlps       = gpModel->m_pIDULayer->GetFieldCol( "AlpsParam" );
   int items = 0;

   DeltaArray *pDeltaArray = NULL;
   if ( m_run < 0 )
      pDeltaArray = gpModel->m_pDeltaArray;
   else
      pDeltaArray = gpModel->m_pDataManager->GetDeltaArray( m_run );

   for ( INT_PTR i=0; i < pDeltaArray->GetSize(); i++ )
      {
      DELTA &delta = pDeltaArray->GetAt( i );
      int col = delta.col;

      if ( delta.cell == m_cell )
         {
         if ( col == colScore || col == colLastD ||  col == colPolicyApps || col == colAlps )
            continue;
         
         // policy column
         item.iItem = items;
         item.iSubItem = 0;
         char text[ 128];

         if ( col == colPolicy )
            {
            int policyID;
            delta.newValue.GetAsInt( policyID );
            EnvPolicy *pPolicy = gpPolicyManager->GetPolicyFromID( policyID );
            sprintf_s( text, 128, "%s (ID:%i)", (LPCTSTR) pPolicy->m_name, pPolicy->m_id );
            item.pszText = text;
            }
         else if ( col == colLulcA )
            item.pszText = "  Lulc A Change";
         else if ( col == colLulcB )
            item.pszText = "  Lulc B Change";
         else if ( col == colLulcC )
            item.pszText = "  Lulc C Change";
         else
            {
            sprintf_s( text, 128, "  %s Change", gpModel->m_pIDULayer->GetFieldLabel( col ) );
            item.pszText = text;
            }

         m_results.InsertItem( &item );

         // application date
         SYSTEMTIME time;
         GetSystemTime( &time );
         char year[ 8 ];
         sprintf_s( year, 8, "%i", time.wYear + delta.year );
         item.iSubItem = 1;
         item.pszText = year;
         m_results.SetItem( &item );

         // outcome start value
         int value, level;
         if ( col == colPolicy )
            {
            delta.oldValue.GetAsInt( value );
            if ( value >= 0 )
               {
               EnvPolicy *pOldPolicy = gpPolicyManager->GetPolicyFromID( value );
               sprintf_s( text, 128, "%s (ID:%i)", (LPCTSTR) pOldPolicy->m_name, pOldPolicy->m_id );
               item.pszText = text;
               }
            else
               item.pszText = "<no prior policy>";
            }
         else if ( col == colLulcA || col == colLulcB || col == colLulcC )
            {
            if ( col == colLulcA )      level = 1;
            else if ( col == colLulcB ) level = 2;
            else level = 3;

            delta.oldValue.GetAsInt( value );
            LulcNode *pNode = gpDoc->m_model.m_lulcTree.FindNode( level, value );
            ASSERT( pNode != NULL );
            sprintf_s( text, 128, "%s (%i)", (LPCTSTR) pNode->m_name, value );
            item.pszText = text;
            }
         else
            item.pszText = (LPTSTR) delta.oldValue.GetAsString();

         item.iSubItem = 2;
         m_results.SetItem( &item );

         // outcome end value
         if ( col == colPolicy )
            {
            delta.newValue.GetAsInt( value );
            if ( value >= 0 )
               {
               EnvPolicy *pNewPolicy = gpPolicyManager->GetPolicyFromID( value );
               sprintf_s( text, 128, "%s (ID:%i)", (LPCTSTR) pNewPolicy->m_name, pNewPolicy->m_id );
               item.pszText = text;
               }
            else
               item.pszText = "<no prior policy>";
            }

         else if ( col == colLulcA || col == colLulcB || col == colLulcC )
            {
            if ( col == colLulcA )      level = 1;
            else if ( col == colLulcB ) level = 2;
            else level = 3;

            delta.newValue.GetAsInt( value );
            LulcNode *pNode = gpDoc->m_model.m_lulcTree.FindNode( level, value );
            ASSERT( pNode != NULL );
            sprintf_s( text, 128, "%s (%i)", (LPCTSTR) pNode->m_name, value );
            item.pszText = text;
            }
         else
            item.pszText = (LPSTR) delta.newValue.GetAsString();

         item.iSubItem = 3;
         m_results.SetItem( &item );

         // site constraints
         if ( col == colPolicy )
            {
            delta.newValue.GetAsInt( value );
            if ( value >= 0 )
               {
               EnvPolicy *pNewPolicy = gpPolicyManager->GetPolicyFromID( value );
               item.pszText = (LPSTR)(LPCTSTR)pNewPolicy->m_siteAttrStr;
               }
            else
               item.pszText = "<n/a>";
            }

         else
            item.pszText = "<n/a>";

         item.iSubItem = 4;
         m_results.SetItem( &item );

         // outcome string
         if ( col == colPolicy )
            {
            delta.newValue.GetAsInt( value );
            if ( value >= 0 )
               {
               EnvPolicy *pNewPolicy = gpPolicyManager->GetPolicyFromID( value );
               item.pszText = (LPTSTR)(LPCTSTR)pNewPolicy->m_outcomeStr;
               }
            else
               item.pszText = "<n/a>";
            }
         else
            item.pszText = "<n/a>";

         item.iSubItem = 5;
         m_results.SetItem( &item );
         items++;
         }
      }
   }

void PPPolicyResults::OnNextcell() 
   {
   (m_cell)++;

   GetDlgItem( IDC_PREVCELL )->EnableWindow( TRUE );

   if ( m_cell == gpModel->m_pIDULayer->GetRecordCount() - 1 )
      GetDlgItem( IDC_NEXTCELL )->EnableWindow( false );

   Load();
   }

void PPPolicyResults::OnPrevcell() 
   {
   (m_cell)--;

   GetDlgItem( IDC_NEXTCELL )->EnableWindow( TRUE );

   if ( m_cell == 0 )
      GetDlgItem( IDC_PREVCELL )->EnableWindow( false );

   Load();
   }

void PPPolicyResults::OnFlashcell() 
   {
   MapWindow *pMapWnd = gpMapPanel->m_pMapWnd;
   pMapWnd->FlashPoly( 0, m_cell );  // 0th layer, m_cell cell
   }


