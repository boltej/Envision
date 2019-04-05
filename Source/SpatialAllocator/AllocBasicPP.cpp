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
// AllocBasicPP.cpp : implementation file
//

#include "stdafx.h"
#include "AllocBasicPP.h"
#include "afxdialogex.h"
#include "SAllocView.h"

#include <QueryBuilder.h>
#include <DirPlaceholder.h>


extern SAllocView *gpSAllocView;


// AllocBasicPP dialog

IMPLEMENT_DYNAMIC(AllocBasicPP, CTabPageSSL)

AllocBasicPP::AllocBasicPP()
: CTabPageSSL(AllocBasicPP::IDD)
, m_pCurrentAllocation(NULL)
, m_allocationName(_T(""))
, m_allocationCode(0)
, m_rate(0)
, m_timeSeriesValues(_T(""))
, m_timeSeriesFile(_T(""))
{

}

AllocBasicPP::~AllocBasicPP()
{
}

void AllocBasicPP::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
DDX_Text(pDX, IDC_ALLOCATIONNAME, m_allocationName);
DDX_Text(pDX, IDC_ALLOCATIONID, m_allocationCode);
DDX_Text(pDX, IDC_GROWTHRATE, m_rate);
DDX_Text(pDX, IDC_TIMESERIES, m_timeSeriesValues);
DDX_Radio(pDX, IDC_TTRATE, m_targetSourceGroupIndex);
DDX_Text(pDX, IDC_FROMFILE, m_fromFile);
   }


BEGIN_MESSAGE_MAP(AllocBasicPP, CTabPageSSL)
   ON_WM_SIZE()
   ON_BN_CLICKED(IDC_TTRATE, EnableCtrls )
   ON_BN_CLICKED(IDC_TTTIMESERIES, EnableCtrls )
   ON_BN_CLICKED(IDC_TTFILE, EnableCtrls )
   ON_BN_CLICKED(IDC_TTALLOCATION, EnableCtrls )
   ON_BN_CLICKED(IDC_BROWSE, &AllocBasicPP::OnBnClickedBrowse)
   ON_EN_CHANGE(IDC_ALLOCATIONNAME, &AllocBasicPP::OnEnChangeAllocationname)
END_MESSAGE_MAP()


BEGIN_EASYSIZE_MAP(AllocBasicPP)
   //         control                left       top        right        bottom     options
    EASYSIZE(IDC_ALLOCATIONNAME,     ES_BORDER, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
    EASYSIZE(IDC_ALLOCATIONID,       ES_BORDER, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
    EASYSIZE(IDC_GROWTHRATE,         ES_BORDER, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
    EASYSIZE(IDC_TIMESERIES,         ES_BORDER, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
    EASYSIZE(IDC_TTRATE,             ES_BORDER, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
    EASYSIZE(IDC_TTTIMESERIES,       ES_BORDER, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
    EASYSIZE(IDC_TTFILE,             ES_BORDER, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0) 
END_EASYSIZE_MAP


BOOL AllocBasicPP::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();
   INIT_EASYSIZE;

   return TRUE;  // return TRUE unless you set the focus to a control
   }

void AllocBasicPP::LoadFields(Allocation *pAlloc)
   {   
   m_pCurrentAllocation = pAlloc;
      
   if (pAlloc != NULL)
      {
      m_allocationName = m_pCurrentAllocation->m_name;
      m_allocationCode = m_pCurrentAllocation->m_id;

      TARGET_SOURCE targetSource = m_pCurrentAllocation->m_targetSource;
      if (targetSource == TS_RATE)
         {
         m_targetSourceGroupIndex = 0;
         m_pCurrentAllocation->GetTargetRate(m_rate);
         }
      else if (targetSource == TS_TIMESERIES)
         {
         m_targetSourceGroupIndex = 1;
         m_timeSeriesValues = m_pCurrentAllocation->m_targetValues;
         }
      else if (targetSource == TS_FILE)
         {
         // TODO: how to handle files
         m_targetSourceGroupIndex = 2;
         //??? TODO - finish!
         }
      else
         {
         m_targetSourceGroupIndex = 3;
         }
      }

   UpdateData(0);
   EnableCtrls();
   }


bool AllocBasicPP::SaveFields( Allocation *pAlloc )
   {
   UpdateData(1);

   m_pCurrentAllocation = pAlloc;

   if (m_pCurrentAllocation != NULL)
      {
      m_pCurrentAllocation->m_name = m_allocationName;
      m_pCurrentAllocation->m_id = m_allocationCode;

      // handle targets
      TARGET_SOURCE targetSource = TS_UNDEFINED;
      CString targetValues;
      
      if (m_targetSourceGroupIndex == 0)
         {
         m_pCurrentAllocation->m_targetRate = m_rate;
         targetSource = TS_RATE;
         GetDlgItemText( IDC_GROWTHRATE, targetValues ); 
         }
      else if (m_targetSourceGroupIndex == 1)
         {
         m_pCurrentAllocation->m_targetValues = m_timeSeriesValues;
         targetSource = TS_TIMESERIES;
         targetValues = m_timeSeriesValues;
         }
      else if (m_targetSourceGroupIndex == 2)
         {
         // TODO: how to handle files
         targetSource = TS_FILE;
         targetValues = m_fromFile;
         }
      else
         { 
         targetSource = TS_USEALLOCATIONSET;
         targetValues = "useAllocationSet";
         }

      m_pCurrentAllocation->m_targetSource = targetSource;
      m_pCurrentAllocation->m_targetValues = targetValues;

      pAlloc = m_pCurrentAllocation;
      }

   return true;
}

void AllocBasicPP::OnSize(UINT nType, int cx, int cy)
   {
   CTabPageSSL::OnSize(nType, cx, cy);

   UPDATE_EASYSIZE;
   }


void AllocBasicPP::EnableCtrls()
   {
   // TODO: Add your specialized code here and/or call the base class
   BOOL checked = IsDlgButtonChecked(IDC_TTRATE);
   GetDlgItem(IDC_LABELGROWTHRATE)->EnableWindow( checked );
   GetDlgItem(IDC_GROWTHRATE)->EnableWindow( checked );

   checked = IsDlgButtonChecked(IDC_TTTIMESERIES);
   GetDlgItem(IDC_TIMESERIES)->EnableWindow( checked );
   
   checked = IsDlgButtonChecked(IDC_TTFILE);
   GetDlgItem(IDC_FROMFILE)->EnableWindow( checked );
   GetDlgItem(IDC_BROWSE)->EnableWindow( checked );  
   }


void AllocBasicPP::OnBnClickedBrowse()
   {
   UpdateData( 1 );
   DirPlaceholder d;

   // TODO get path
   CString startPath = m_fromFile;
   CString ext = _T("csv");
   CString extensions = _T("CSV Files|*.csv|All files|*.*||");
   
   CFileDialog fileDlg( FALSE, ext, startPath, OFN_HIDEREADONLY, extensions);
   if (fileDlg.DoModal() == IDOK)
      {
      m_fromFile = fileDlg.GetPathName();
      UpdateData( 0 );
      }
   }

void AllocBasicPP::OnEnChangeAllocationname()
   {
   UpdateData(1);
   m_pCurrentAllocation->m_name = this->m_allocationName;
   gpSAllocView->m_pTreeWnd->RefreshItemLabel( m_pCurrentAllocation );
   }
