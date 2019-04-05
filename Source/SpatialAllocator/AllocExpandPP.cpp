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
// AllocExpandPP.cpp : implementation file
//

#include "stdafx.h"
#include "AllocExpandPP.h"
#include "afxdialogex.h"
#include "SAllocView.h"
#include <QueryBuilder.h>
#include <DirPlaceholder.h>


extern SAllocView *gpSAllocView;

// AllocExpandPP dialog

IMPLEMENT_DYNAMIC(AllocExpandPP, CTabPageSSL)

AllocExpandPP::AllocExpandPP()
: CTabPageSSL(AllocExpandPP::IDD)
, m_pCurrentAllocation(NULL)
, m_allowExpansion(FALSE)
, m_limitToQuery(FALSE)
, m_expandQuery(_T(""))
, m_limitSize(FALSE)
, m_expandMaxSize("")
, m_pQueryEngine( NULL )
{

}

AllocExpandPP::~AllocExpandPP()
{
}

void AllocExpandPP::DoDataExchange(CDataExchange* pDX)
   {
   CTabPageSSL::DoDataExchange(pDX);
   DDX_Check(pDX, IDC_ALLOWEXPANSION, m_allowExpansion);
   DDX_Check(pDX, IDC_LIMITTOQUERY, m_limitToQuery);
   DDX_Text(pDX, IDC_QUERYEXPAND, m_expandQuery);
   DDX_Check(pDX, IDC_LIMITSIZE, m_limitSize);
   DDX_Text(pDX, IDC_MAXAREA, m_expandMaxSize);
   }


BEGIN_MESSAGE_MAP(AllocExpandPP, CTabPageSSL)
   ON_BN_CLICKED(IDC_LIMITTOQUERY, EnableCtrls)
   ON_BN_CLICKED(IDC_LIMITSIZE, EnableCtrls)
   ON_BN_CLICKED( IDC_ALLOWEXPANSION, EnableCtrls )
   ON_WM_SIZE()
   ON_BN_CLICKED( IDC_UPDATEMAPEXPAND, &AllocExpandPP::OnBnClickedUpdateMap )
   ON_BN_CLICKED( IDC_CLEARQUERYEXPAND, &AllocExpandPP::OnBnClickedClearQuery )
   ON_BN_CLICKED( IDC_QBEXPAND, &AllocExpandPP::OnBnClickedQueryEditor )
   ON_BN_CLICKED( IDC_USECONSTRAINTQUERY, &AllocExpandPP::OnBnClickedUseconstraintquery )
END_MESSAGE_MAP()


BEGIN_EASYSIZE_MAP(AllocExpandPP)
   //         control                left       top        right        bottom     options
    EASYSIZE(IDC_ALLOWEXPANSION,     ES_BORDER, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)

    EASYSIZE(IDC_LIMITTOQUERY,       ES_BORDER, ES_BORDER, ES_BORDER, ES_KEEPSIZE, 0)
    EASYSIZE(IDC_QUERYEXPAND,        ES_BORDER, ES_BORDER, ES_BORDER, ES_BORDER, 0)

    EASYSIZE(IDC_USECONSTRAINTQUERY, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
    EASYSIZE(IDC_QBEXPAND,           ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
    EASYSIZE(IDC_UPDATEMAPEXPAND,    ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
    EASYSIZE(IDC_CLEARQUERYEXPAND,   ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, ES_BORDER, 0)
    
    EASYSIZE(IDC_LIMITSIZE,          ES_BORDER, IDC_QUERYEXPAND, ES_BORDER,   ES_KEEPSIZE, 0)
    EASYSIZE(IDC_MAXAREA,            ES_BORDER, IDC_QUERYEXPAND, ES_KEEPSIZE, ES_KEEPSIZE, 0)
    EASYSIZE(IDC_LABELAREA,          ES_BORDER, IDC_QUERYEXPAND, ES_BORDER, ES_KEEPSIZE, 0)
END_EASYSIZE_MAP


BOOL AllocExpandPP::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();
   INIT_EASYSIZE;

   return TRUE;  // return TRUE unless you set the focus to a control
   }


void AllocExpandPP::LoadFields(Allocation *pAlloc)
   {   
   m_pCurrentAllocation = pAlloc;

   if (pAlloc != NULL)
      {
      m_allowExpansion = m_pCurrentAllocation->m_useExpand;
      if ( m_allowExpansion )
         {
         CString str(m_pCurrentAllocation->m_expandQueryStr);
         str.Replace("  ", "");
         str.Replace("\t", "");
         str.Replace("\n", "\r\n");
         m_expandQuery = str;
         if (m_expandQuery.GetLength() != 0) 
            {
            GetDlgItem(IDC_QBEXPAND)->EnableWindow(1);
            m_limitToQuery = true;
            }

         m_expandMaxSize = m_pCurrentAllocation->m_expandAreaStr;
         m_limitSize = m_expandMaxSize.IsEmpty()? false : true;
         }
      }

   UpdateData(0);
   EnableCtrls();
   }


bool AllocExpandPP::SaveFields( Allocation *pAlloc )
   {
   UpdateData(1);

   m_pCurrentAllocation = pAlloc;

   if (m_pCurrentAllocation != NULL)
      {
      m_pCurrentAllocation->m_useExpand = (m_allowExpansion == 1) ? true : false;

      if ( m_limitToQuery )
         m_pCurrentAllocation->SetExpandQuery( m_expandQuery );
      else
         m_pCurrentAllocation->SetExpandQuery( "" );

      if ( m_limitSize )
         m_pCurrentAllocation->SetMaxExpandAreaStr( m_expandMaxSize );
      else
         m_pCurrentAllocation->SetMaxExpandAreaStr( "" );  // parses string

      pAlloc = m_pCurrentAllocation;
      }

   return true;
}

void AllocExpandPP::OnSize(UINT nType, int cx, int cy)
   {
   CTabPageSSL::OnSize(nType, cx, cy);
   UPDATE_EASYSIZE;
   }


void AllocExpandPP::SetMapLayer( MapLayer *pLayer )
   {
   m_pMapLayer = pLayer;
   }


void AllocExpandPP::RunQuery()
   {
   if ( m_pQueryEngine == NULL )
      return;

   UpdateData( 1 );     // make sure m_queryString is current

   if ( m_expandQuery.IsEmpty() )
      return;
   
   CString _query = m_expandQuery;
   _query.Replace( "@", m_pCurrentAllocation->m_constraint.m_queryStr );
   Query *pQuery = m_pQueryEngine->ParseQuery( _query, 0, "Allocation Expand Page" );
   
   if ( pQuery != NULL )
      {
      int count = m_pQueryEngine->SelectQuery( pQuery, true );
      gpSAllocView->RedrawMap();

      CString msg;
      msg.Format( "Selected %i IDUs", count );
      gpSAllocView->SetMapMsg( msg );
      }

   m_pQueryEngine->RemoveQuery( pQuery );
   }


void AllocExpandPP::EnableCtrls()
   {
   BOOL checked = IsDlgButtonChecked(IDC_ALLOWEXPANSION);
   GetDlgItem(IDC_LIMITTOQUERY)->EnableWindow( checked );
   GetDlgItem(IDC_LIMITSIZE)->EnableWindow( checked );

   BOOL checked2 = IsDlgButtonChecked(IDC_LIMITTOQUERY);
   GetDlgItem(IDC_QUERYEXPAND)->EnableWindow( checked && checked2 ? 1 : 0);
   GetDlgItem(IDC_USECONSTRAINTQUERY)->EnableWindow( checked && checked2 ? 1 : 0);
   GetDlgItem(IDC_QBEXPAND)->EnableWindow( checked && checked2 ? 1 : 0);

   checked2 = IsDlgButtonChecked(IDC_LIMITSIZE);
   GetDlgItem(IDC_MAXAREA)->EnableWindow( checked && checked2 ? 1 : 0);
   GetDlgItem(IDC_LABELAREA)->EnableWindow( checked && checked2 ? 1 : 0);
   }



void AllocExpandPP::OnBnClickedUseconstraintquery()
   {
   m_expandQuery = "@";
   UpdateData( 0 );
   }


void AllocExpandPP::OnBnClickedQueryEditor()
   {
   UpdateData(1);

   QueryBuilder dlg( m_pQueryEngine, NULL, m_pMapLayer, -1, this );
   dlg.m_queryString = this->m_expandQuery;
   
   if ( dlg.DoModal() == IDOK )
      {
      this->m_expandQuery = dlg.m_queryString;
      UpdateData( 0 );
      }
   }


void AllocExpandPP::OnBnClickedUpdateMap()
   {
   RunQuery();
   }

void AllocExpandPP::OnBnClickedClearQuery()
   {
   m_pMapLayer->ClearSelection();
   gpSAllocView->RedrawMap();
   }