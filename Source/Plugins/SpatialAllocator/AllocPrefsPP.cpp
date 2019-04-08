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
// AllocPrefsPP.cpp : implementation file
//

#include "stdafx.h"
#include "SpatialAllocator.h"
#include "AllocPrefsPP.h"
#include "SAllocView.h"
#include "afxdialogex.h"

#include <QueryBuilder.h>


extern SAllocView *gpSAllocView;

int PrefMapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );


// AllocPrefsPP dialog

IMPLEMENT_DYNAMIC(AllocPrefsPP, CTabPageSSL)

AllocPrefsPP::AllocPrefsPP()
	: CTabPageSSL(AllocPrefsPP::IDD)
   //, MapWndContainer()
   , m_pAlloc( NULL )
   , m_queryStr(_T(""))
   , m_pQueryEngine( NULL )
   , m_currentIndex( -1 )
{
}

AllocPrefsPP::~AllocPrefsPP()
{
}

void AllocPrefsPP::DoDataExchange(CDataExchange* pDX)
   {
	CTabPageSSL::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_PREFERENCES, m_preferences);
   DDX_Text(pDX, IDC_QUERYPREF, m_queryStr);
   DDX_Text(pDX, IDC_PREFNAME, m_name);
   DDX_Text(pDX, IDC_PREFEXPR, m_weightExpr);
   }


BEGIN_MESSAGE_MAP(AllocPrefsPP, CTabPageSSL)
   ON_WM_SIZE()
   ON_BN_CLICKED(IDC_QBPREF, &AllocPrefsPP::OnBnClickedEditPrefQuery)
   ON_CBN_SELCHANGE(IDC_PREFERENCES, LoadPreference)
   ON_BN_CLICKED( IDC_UPDATEMAPPREF, &AllocPrefsPP::OnBnClickedUpdateMap )
   //ON_WM_ACTIVATE()
   ON_BN_CLICKED(IDC_ADDPREF, &AllocPrefsPP::OnBnClickedAddpref)
   ON_BN_CLICKED(IDC_REMOVEPREF, &AllocPrefsPP::OnBnClickedRemovepref)
   ON_BN_CLICKED( IDC_CLEARQUERYPREF, &AllocPrefsPP::OnBnClickedClearquery )
END_MESSAGE_MAP()

BEGIN_EASYSIZE_MAP(AllocPrefsPP)
   //         control         left          top          right         bottom     options
   EASYSIZE(IDC_PREFERENCES,    ES_BORDER,   ES_BORDER,   ES_KEEPSIZE,  ES_KEEPSIZE, 0)
   EASYSIZE(IDC_PREFBORDER,     ES_BORDER,   ES_BORDER,   ES_KEEPSIZE,  ES_BORDER,  0)
   EASYSIZE(IDC_PREFEXPR,       ES_BORDER,   ES_BORDER,   ES_KEEPSIZE,  ES_KEEPSIZE, 0)
   EASYSIZE(IDC_QUERYPREF,      ES_BORDER,   ES_BORDER,   ES_KEEPSIZE,  ES_BORDER,   0)
   EASYSIZE(IDC_ADDPREF,        ES_BORDER,   ES_BORDER,   ES_KEEPSIZE,  ES_KEEPSIZE,  0)
   EASYSIZE(IDC_REMOVEPREF,     ES_BORDER,   ES_BORDER,   ES_KEEPSIZE,  ES_KEEPSIZE,  0)
   EASYSIZE(IDC_QBPREF,         ES_BORDER,   ES_KEEPSIZE, ES_KEEPSIZE,  ES_BORDER,  0)
   EASYSIZE(IDC_UPDATEMAPPREF,  ES_BORDER,   ES_KEEPSIZE, ES_KEEPSIZE,  ES_BORDER,  0)
   EASYSIZE(IDC_CLEARQUERYPREF, ES_BORDER,   ES_KEEPSIZE, ES_KEEPSIZE,  ES_BORDER,  0)
END_EASYSIZE_MAP


BOOL AllocPrefsPP::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();

   INIT_EASYSIZE;

   // Generate map
   //CreateMapWnd( this, IDC_PLACEHOLDER, IDC_STATUS );

   return TRUE;  // return TRUE unless you set the focus to a control
   }


void AllocPrefsPP::LoadFields(Allocation *pAlloc)
   {
   m_currentIndex = -1;
   m_preferences.ResetContent();
   m_pAlloc = pAlloc;
      
   if (pAlloc != NULL)
      {

      int prefCount = (int) m_pAlloc->m_prefsArray.GetSize();
      for (int i = 0; i < prefCount; i++)
         {
         Preference *pPref = m_pAlloc->m_prefsArray[i];
         if (pPref != NULL)
            m_preferences.AddString( pPref->m_name );
         }

      if ( prefCount > 0 )
         {
         m_preferences.SetCurSel(0);
         LoadPreference();
         }
      else
         m_preferences.SetCurSel( -1 );
      }
   }


void AllocPrefsPP::LoadPreference()
   {
   if ( m_currentIndex >= 0 )
      SaveFields( m_pAlloc );
        
   if ( m_pAlloc != NULL )
      {
      m_currentIndex = m_preferences.GetCurSel();

      if ( m_currentIndex >= 0 )
         {
         Preference *pPref = m_pAlloc->m_prefsArray[m_currentIndex];
      
         if (pPref != NULL)
            {
            m_weightExpr = pPref->m_weightExpr;
            m_name = pPref->m_name;
   
            if (pPref->m_queryStr.GetLength() > 0 ) // is the preference associated with a query?
               {
               CString str(pPref->m_queryStr);
               str.Replace("\n", "\r\n");
               m_queryStr = str;
               }
            }
         }
      else
         {
         m_name="";
         m_weightExpr="";
         m_queryStr = "";
         }
      }

   UpdateData( 0 );
   RunQuery();
   }


bool AllocPrefsPP::SaveFields(Allocation *pAlloc)
   {
   UpdateData(1);

   if (m_pAlloc != NULL && m_currentIndex >= 0 )
      {
      Preference *pPref = m_pAlloc->m_prefsArray[m_currentIndex];
      
      pPref->m_name = m_name;
      pPref->m_weightExpr = m_weightExpr;
      pPref->m_queryStr = m_queryStr;
      }

   return true;
   }


void AllocPrefsPP::OnSize(UINT nType, int cx, int cy)
   {
   CTabPageSSL::OnSize(nType, cx, cy);
   UPDATE_EASYSIZE;
   //MapWndContainer::OnSize( nType, cx, cy);
   }


// AllocPrefsPP message handlers
void AllocPrefsPP::SetMapLayer( MapLayer *pLayer )
   {
   m_pMapLayer = pLayer;

   //MapWndContainer::SetMapLayer(pLayer);

   if ( m_pQueryEngine != NULL )
      delete m_pQueryEngine;
      
   m_pQueryEngine = new QueryEngine( pLayer );
   }


void AllocPrefsPP::OnBnClickedEditPrefQuery()
   {
   UpdateData(1);

   QueryBuilder dlg( m_pQueryEngine, NULL, m_pMapLayer, -1, this );
   dlg.m_queryString = this->m_queryStr;
   
   if ( dlg.DoModal() == IDOK )
      {
      this->m_queryStr = dlg.m_queryString;
      UpdateData( 0 );
      RunQuery();      
      }
   }


void AllocPrefsPP::OnBnClickedUpdateMap()
   {
   RunQuery();
   }


void AllocPrefsPP::RunQuery()
   {
   if ( m_pQueryEngine == NULL )
      return;

   UpdateData( 1 );     // make sure m_queryString is current

   Query *pQuery = m_pQueryEngine->ParseQuery( m_queryStr, 0, "Allocation Preference Page" );
   
   if ( pQuery != NULL )
      {
      int count = m_pQueryEngine->SelectQuery( pQuery, true );
      gpSAllocView->RedrawMap();

      CString msg;
      msg.Format( "Selected %i polygons", count );
      gpSAllocView->SetMapMsg( msg );
      }
   }

  
//void AllocPrefsPP::OnCellproperties() 
//   {
//   MapWndContainer::OnCellproperties();
//   }
//
//void AllocPrefsPP::OnSelectField( UINT nID) 
//   {
//   m_mapFrame.OnSelectField( nID );
//   }


//void AllocPrefsPP::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized )
//   {
//   __super::OnActivate( nState, pWndOther, bMinimized );
//
//   this->RunQuery();
//   // TODO: Add your message handler code here
//   }


void AllocPrefsPP::OnBnClickedAddpref()
   {
   if ( m_pAlloc != NULL )
      {
      // make a new pref
      Preference *pPref = new Preference( m_pAlloc, "New Preference", "", "1" );
      m_pAlloc->m_prefsArray.Add( pPref );

      // add it to the combo
      int index = m_preferences.AddString( pPref->m_name );
      m_preferences.SetCurSel( index );

      LoadPreference();
      }
   }


void AllocPrefsPP::OnBnClickedRemovepref()
   {
   if ( m_pAlloc != NULL && m_currentIndex >= 0 )
      {
      m_pAlloc->m_prefsArray.RemoveAt( m_currentIndex );

      m_preferences.SetCurSel( m_currentIndex-1 );
      LoadPreference();
      }
   }


void AllocPrefsPP::OnBnClickedClearquery()
   {
   m_pMapLayer->ClearSelection();
   gpSAllocView->RedrawMap();
   }
