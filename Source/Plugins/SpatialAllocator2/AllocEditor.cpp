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
// AllocEditor.cpp : implementation file
//

#include "stdafx.h"
#include "AllocEditor.h"

#include "SAllocView.h"
#include <DirPlaceholder.h>

extern SAllocView *gpSAllocView;

// AllocEditor dialog


IMPLEMENT_DYNAMIC(AllocEditor, CDialog)

AllocEditor::AllocEditor(CWnd* pParent /*=NULL*/)
: CDialog(AllocEditor::IDD, pParent)
, m_pAlloc( NULL )
, m_pQueryEngine( NULL )
, m_tabCtrl()
, m_basic( )
, m_expand()
, m_constraint()
, m_prefs()
//, m_firstLoad(true)
//, m_fullHeight(0)
//, m_shrunkHeight(0)
{ }

AllocEditor::~AllocEditor()
{
}

void AllocEditor::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange( pDX );
DDX_Control( pDX, IDC_TAB2, m_tabCtrl );
DDX_Control( pDX, IDC_ALLOCHEADER, m_allocHeader );
}

BEGIN_MESSAGE_MAP(AllocEditor, CDialog)
   ON_BN_CLICKED(IDC_DELETE, OnDelete)
   ON_BN_CLICKED(IDC_COPY, OnCopy)
   ON_BN_CLICKED(IDC_ADDNEW, OnAddNew)
   ON_BN_CLICKED(IDC_SAVE, &AllocEditor::OnBnClickedSaveXml)
   ON_BN_CLICKED(IDC_IMPORT, &AllocEditor::OnBnClickedImport)
   ON_WM_SIZE()
END_MESSAGE_MAP()


BEGIN_EASYSIZE_MAP(AllocEditor)
   //         control     left      top          right        bottom     options
   EASYSIZE(IDC_TAB2,    ES_BORDER,  ES_BORDER,   ES_BORDER,   ES_BORDER, 0)
   EASYSIZE(IDC_ADDNEW,  ES_BORDER,  ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
   EASYSIZE(IDC_COPY,    ES_BORDER,  ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
   EASYSIZE(IDC_DELETE,  ES_BORDER,  ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
   EASYSIZE(IDC_IMPORT,  ES_BORDER,  ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
   EASYSIZE(IDC_SAVE,    ES_BORDER,  ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
   EASYSIZE(IDOK,        ES_BORDER,  ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
END_EASYSIZE_MAP


// AllocEditor message handlers

BOOL AllocEditor::OnInitDialog()
{
   CDialog::OnInitDialog();

   m_tabCtrl.SetResizable( true );

   INIT_EASYSIZE;

   CenterWindow();

   RECT rect;
   GetWindowRect(&rect);

   RECT rectButton;
   GetClientRect( &rectButton );

   // Setup the tab control
   int nPageID = 100;
   m_basic.SetMapLayer( m_pMapLayer );
   m_basic.Create(AllocBasicPP::IDD, this);
   m_tabCtrl.AddSSLPage(_T("Basic Properties"), nPageID++, &m_basic);
   
   m_constraint.SetMapLayer( m_pMapLayer );
   m_constraint.SetQueryEngine(m_pQueryEngine);
   m_constraint.Create(AllocConstraintPP::IDD, this);
   m_tabCtrl.AddSSLPage(_T("Constraints"), nPageID++, &m_constraint);
   
   m_prefs.SetMapLayer( m_pMapLayer );
   m_prefs.Create(AllocPrefsPP::IDD, this);
   m_tabCtrl.AddSSLPage(_T("Preferences"), nPageID++, &m_prefs);

   m_expand.SetMapLayer( m_pMapLayer );
   m_expand.SetQueryEngine(m_pQueryEngine);
   m_expand.Create(AllocExpandPP::IDD, this);
   m_tabCtrl.AddSSLPage(_T("Expansion"), nPageID++, &m_expand);

   m_tabCtrl.SetCurSel( 0 );

   SetWindowText("Allocation Editor");
   return TRUE;
}

void AllocEditor::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);
   UPDATE_EASYSIZE;
   }


void AllocEditor::OnCopy()
   {
   if ( m_pAlloc == NULL )
      return;

   SaveFields();

   Allocation *pAllocNew = new Allocation(*m_pAlloc);
   pAllocNew->m_name = "New Allocation"; 
   pAllocNew->m_pAllocSet = m_pAlloc->m_pAllocSet;
   pAllocNew->m_pAllocSet->m_allocationArray.Add( pAllocNew );
   
   // put new Allocation into tree within current allocation set
   gpSAllocView->m_pTreeWnd->InsertAllocation(pAllocNew); 
   }


void AllocEditor::OnAddNew()
   {
   SaveFields();

   Allocation *pAllocNew = new Allocation( m_pAlloc->m_pAllocSet, "New Allocation", -1 );
   m_pAlloc->m_pAllocSet->m_allocationArray.Add( pAllocNew );

   // put new Allocation into tree within current allocation set
   gpSAllocView->m_pTreeWnd->InsertAllocation(pAllocNew); 
   }


void AllocEditor::OnDelete()
   {
   if ( m_pAlloc == NULL )
      return;

   m_pAlloc->m_pAllocSet->m_allocationArray.Remove( m_pAlloc );      // deletes Allocation
   gpSAllocView->m_pTreeWnd->RemoveAllocation( m_pAlloc );
   }

void AllocEditor::OnOK()
   {
   return CDialog::OnOK();  
   }


void AllocEditor::OnCancel()
   {
   CDialog::OnCancel();
   }  

void AllocEditor::OnBnClickedSaveXml()
   {
   SaveFields();
   gpSAllocView->SaveXml();
   }


void AllocEditor::OnBnClickedImport()
   {
   SaveFields();
   gpSAllocView->ImportXml();
   }


void AllocEditor::LoadAllocation(void)
   {
   if ( m_pAlloc == NULL )
      m_allocHeader.SetWindowText( "No Allocation Loaded..." );
   else
      {
      CString hdr = "Allocation Set: ";
      hdr += m_pAlloc->m_pAllocSet->m_name;
      hdr += " --- Allocation: ";
      hdr += m_pAlloc->m_name;
      m_allocHeader.SetWindowText( hdr );
      }

   m_basic.LoadFields(m_pAlloc);
   m_constraint.LoadFields(m_pAlloc);;
   m_prefs.LoadFields(m_pAlloc);
   m_expand.LoadFields(m_pAlloc);
   }


bool AllocEditor::SaveFields( void )
   {
   bool bOK = m_basic.SaveFields( m_pAlloc );
   bool cOK = m_constraint.SaveFields( m_pAlloc );
   bool pOK = m_prefs.SaveFields( m_pAlloc );
   bool eOK = m_expand.SaveFields( m_pAlloc );

   return bOK && cOK && pOK && eOK;
   }



/////////////////////////////////////////////////////////////////////////////
//
// SAETabCtrl
//
/////////////////////////////////////////////////////////////////////////////


void SAETabCtrl::OnActivatePage (int nIndex, int nPageID)
   {
   switch ( nIndex )
      {
      case 0:  
         ///AllocBasicPP *pWnd = (AllocBasicPP*) this->m_tabs[0].pTabPage;
         break;
   
      case 1:
         {  
         AllocConstraintPP *pWnd = (AllocConstraintPP*) this->m_tabs[1].pTabPage;
         pWnd->RunQuery();
         }
         break;
   
      case 2:
         {  
         AllocPrefsPP *pWnd = (AllocPrefsPP*) this->m_tabs[2].pTabPage;
         pWnd->RunQuery();
         }
         break;

      case 3:
         {  
         AllocExpandPP *pWnd = (AllocExpandPP*) this->m_tabs[3].pTabPage;
         pWnd->RunQuery();
         }
         break;
      }

   CTabCtrlSSL::OnActivatePage( nIndex, nPageID );
   }


//void SAETabCtrl::OnDeactivatePage (int nIndex, int nPageID)
//{
//
//}
