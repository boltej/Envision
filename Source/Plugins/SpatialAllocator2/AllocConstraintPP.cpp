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
// AllocConstraintPP.cpp : implementation file
//

#include "stdafx.h"
#include "SpatialAllocator.h"
#include "SAllocWnd.h"
#include "SAllocView.h"
#include "AllocConstraintPP.h"
#include "afxdialogex.h"

#include <QueryBuilder.h>


extern SAllocView *gpSAllocView;


// AllocConstraintPP dialog

IMPLEMENT_DYNAMIC(AllocConstraintPP, CTabPageSSL)

AllocConstraintPP::AllocConstraintPP()
	: CTabPageSSL(AllocConstraintPP::IDD)
   , m_pCurrentAllocation( NULL )
   , m_queryStr(_T(""))
   , m_pQueryEngine( NULL )
{
}

AllocConstraintPP::~AllocConstraintPP()
{
}

void AllocConstraintPP::DoDataExchange(CDataExchange* pDX)
   {
   CTabPageSSL::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_QUERYCONSTRAINT, m_queryStr);
   }


BEGIN_MESSAGE_MAP(AllocConstraintPP, CTabPageSSL)
   ON_WM_SIZE()
   ON_BN_CLICKED( IDC_QBCONSTRAINT, &AllocConstraintPP::OnBnClickedEditConstraintQuery )
   ON_BN_CLICKED( IDC_UPDATEMAPCONSTRAINT, &AllocConstraintPP::OnBnClickedUpdateMap )
   //ON_WM_ACTIVATE()
   ON_BN_CLICKED( IDC_CLEARQUERYCONSTRAINT, &AllocConstraintPP::OnBnClickedClearquery )
END_MESSAGE_MAP()

BEGIN_EASYSIZE_MAP(AllocConstraintPP)
   //       control                    left         top          right        bottom     options
   EASYSIZE(IDC_QUERYCONSTRAINT,       ES_BORDER,   ES_BORDER,   ES_KEEPSIZE, ES_BORDER,   0)
   EASYSIZE(IDC_QBCONSTRAINT,          ES_BORDER,   ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,   0)
   EASYSIZE(IDC_UPDATEMAPCONSTRAINT,   ES_BORDER,   ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,   0)
   EASYSIZE(IDC_CLEARQUERYCONSTRAINT,  ES_BORDER,   ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER,   0)
END_EASYSIZE_MAP


BOOL AllocConstraintPP::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();

   INIT_EASYSIZE;

   return TRUE;  // return TRUE unless you set the focus to a control
   }


void AllocConstraintPP::OnSize(UINT nType, int cx, int cy)
   {
   CTabPageSSL::OnSize(nType, cx, cy);
   UPDATE_EASYSIZE;
   }


// AllocConstraintPP message handlers
void AllocConstraintPP::SetMapLayer( MapLayer *pLayer )
   {
   m_pMapLayer = pLayer;
   //MapWndContainer::SetMapLayer(pLayer);
   }


void AllocConstraintPP::LoadFields(Allocation *pAlloc)
   {
   m_pCurrentAllocation = pAlloc;

   if (pAlloc != NULL)
      {
      CString str( pAlloc->m_constraint.m_queryStr);
      str.Replace("  ", "");
      str.Replace("\t", "");
      str.Replace("\n", "\r\n");
      m_queryStr = str;
      }

   UpdateData( 0 );
   RunQuery();
   }


bool AllocConstraintPP::SaveFields( Allocation *pAlloc )
   {
   UpdateData(1);

   if (m_pCurrentAllocation != NULL )
      m_pCurrentAllocation->m_constraint.m_queryStr = m_queryStr;

   return true;
   }


void AllocConstraintPP::OnBnClickedEditConstraintQuery()
   {
   UpdateData(1);

   QueryBuilder dlg(m_pQueryEngine, NULL, m_pMapLayer, -1, this );
   dlg.m_queryString = this->m_queryStr;
   
   if ( dlg.DoModal() == IDOK )
      {
      this->m_queryStr = dlg.m_queryString;
      UpdateData( 0 );
      RunQuery();      
      }
   }


void AllocConstraintPP::OnBnClickedUpdateMap()
   {
   RunQuery();
   }


void AllocConstraintPP::OnBnClickedClearquery()
   {
   m_pMapLayer->ClearSelection();
   gpSAllocView->RedrawMap();
   }


void AllocConstraintPP::RunQuery()
   {
   if ( m_pQueryEngine == NULL )
      return;

   UpdateData( 1 );     // make sure m_queryString is current

   if ( m_queryStr.IsEmpty() )
      return;

   Query *pQuery = m_pQueryEngine->ParseQuery( m_queryStr, 0, "Allocation Constraint Page" );
   
   if ( pQuery != NULL )
      {
      int count = m_pQueryEngine->SelectQuery( pQuery, true );
      gpSAllocView->RedrawMap();

      CString msg;
      msg.Format( "Selected %i polygons", count );
      gpSAllocView->SetMapMsg( msg );
      }

   m_pQueryEngine->RemoveQuery( pQuery );
   }


//void AllocConstraintPP::OnCellproperties() 
//   {
//   MapWndContainer::OnCellproperties();
//   }
   
//void AllocConstraintPP::OnSelectField( UINT nID) 
//   {
//   m_mapFrame.OnSelectField( nID );
//   }


//void AllocConstraintPP::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized )
//   {
//   __super::OnActivate( nState, pWndOther, bMinimized );
//
//   this->RunQuery();
//
//   // TODO: Add your message handler code here
//   }


