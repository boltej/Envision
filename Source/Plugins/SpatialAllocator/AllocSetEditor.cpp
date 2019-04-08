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
// AllocSetEditor.cpp : implementation file
//

#include "stdafx.h"
#include "AllocSetEditor.h"
#include "SAllocWnd.h"
#include "SAllocView.h"

#include <DirPlaceholder.h>


extern SAllocView *gpSAllocView;


IMPLEMENT_DYNAMIC(AllocSetEditor, CDialog)

AllocSetEditor::AllocSetEditor(CWnd* pParent /*=NULL*/)
: CDialog(AllocSetEditor::IDD, pParent)
, m_pCurrentSet(NULL)
, m_allocationSetName(_T(""))
, m_inUse(FALSE)
, m_shuffle(FALSE)
, m_useSequences(FALSE)
, m_rate(0)
, m_timeSeriesValues(_T(""))
, m_timeSeriesFile(_T(""))
{
}

AllocSetEditor::AllocSetEditor(SpatialAllocator *pSA, AllocationSet *pAllocSet, CWnd* pParent /*=NULL*/)
: CDialog(AllocSetEditor::IDD, pParent)
//, m_pSpatialAllocator(pSA)
, m_pCurrentSet(pAllocSet)
, m_allocationSetName(_T(""))
, m_inUse(FALSE)
, m_shuffle(FALSE)
, m_useSequences(FALSE)
, m_description(_T(""))
, m_rate(0)
, m_timeSeriesValues(_T(""))
, m_timeSeriesFile(_T(""))
, m_fromFile(_T(""))
   {
}

AllocSetEditor::~AllocSetEditor()
{
}

void AllocSetEditor::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange( pDX );
DDX_Text( pDX, IDC_AS_NAME, m_allocationSetName );
DDX_Control( pDX, IDC_AS_FIELD, m_allocationSetField );
DDX_Check( pDX, IDC_INUSE, m_inUse );
DDX_Check( pDX, IDC_USESEQUENCES, m_useSequences );
DDX_Control( pDX, IDC_SEQUENCEFIELD, m_sequenceField );
DDX_Text( pDX, IDC_DESCRIPTION, m_description );

DDX_Text( pDX, IDC_GROWTHRATE, m_rate );
DDX_Text( pDX, IDC_TIMESERIES, m_timeSeriesValues );
DDX_Radio( pDX, IDC_TTRATE, m_targetSourceGroupIndex );
DDX_Text( pDX, IDC_FROMFILE, m_fromFile );
DDX_Control( pDX, IDC_TARGETBASIS, m_targetBasis );
}


BEGIN_MESSAGE_MAP(AllocSetEditor, CDialog)
   ON_BN_CLICKED(IDC_USESEQUENCES, EnableCtrls)
   ON_BN_CLICKED(IDC_TTRATE, EnableCtrls )
   ON_BN_CLICKED(IDC_TTTIMESERIES, EnableCtrls )
   ON_BN_CLICKED(IDC_TTFILE, EnableCtrls )
   ON_BN_CLICKED(IDC_TTALLOCATION, EnableCtrls )

   ON_BN_CLICKED(IDC_AS_ADDNEW, &AllocSetEditor::OnAddNew)
   ON_BN_CLICKED(IDC_AS_COPY, &AllocSetEditor::OnCopy)
   ON_BN_CLICKED(IDC_AS_DELETE, &AllocSetEditor::OnDelete)
   ON_BN_CLICKED(IDC_AS_IMPORT, &AllocSetEditor::OnImportXml)
   ON_BN_CLICKED(IDC_AS_SAVE, &AllocSetEditor::OnSaveXml)
   ON_BN_CLICKED(IDOKAS, &AllocSetEditor::OnOKAS)
   ON_WM_SIZE()
   ON_BN_CLICKED(IDC_BROWSE, &AllocSetEditor::OnBnClickedBrowse)
   ON_BN_CLICKED(IDC_AS_ADDNEWALLOC, &AllocSetEditor::OnAddNewAllocation)
   ON_EN_CHANGE(IDC_AS_NAME, &AllocSetEditor::OnEnChangeAsName)
END_MESSAGE_MAP()


BEGIN_EASYSIZE_MAP(AllocSetEditor)
   //         control               left          top        right        bottom     options
   EASYSIZE(IDC_AS_DESC,            ES_BORDER,    ES_BORDER, ES_BORDER,   ES_BORDER, 0 )
   EASYSIZE(IDC_FIELDGRP,           ES_BORDER,    ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_AS_TITLE,           IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_AS_NAME,            IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_DESCTITLE,          IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_DESCRIPTION,        IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_IDUTITLE,           IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_AS_FIELD,           IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_INUSE,              IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_USESEQUENCES,       IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_SEQUENCETITLE,      IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_SEQUENCEFIELD,      IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)

   EASYSIZE(IDC_GROWTHRATE,         IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_TIMESERIES,         IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_TTRATE,             IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_TTTIMESERIES,       IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_TTFILE,             IDC_FIELDGRP, ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0) 

   EASYSIZE(IDC_AS_ADDNEW,          ES_BORDER,  ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_AS_COPY,            ES_BORDER,  ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_AS_DELETE,          ES_BORDER,  ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_AS_IMPORT,          ES_BORDER,  ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDC_AS_SAVE,            ES_BORDER,  ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
   EASYSIZE(IDOKAS,                 ES_BORDER,  ES_BORDER, ES_KEEPSIZE, ES_KEEPSIZE, 0)
END_EASYSIZE_MAP


BOOL AllocSetEditor::OnInitDialog()
{
   CDialog::OnInitDialog();
   INIT_EASYSIZE;

   LoadFields();  // loads allocation sets with associate allocations

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


void AllocSetEditor::OnAddNew()
   {
   SaveFields(); 

   AllocationSet *pNew = new AllocationSet( this->m_pCurrentSet->m_pSAModel, "New Allocation Set", "AREA");
   gpSAllocView->m_allocationSetArray.Add( pNew );

   // put new Allocation into tree within current allocation set
   gpSAllocView->m_pTreeWnd->InsertAllocationSet(pNew);
   }



void AllocSetEditor::OnAddNewAllocation()
   {
   SaveFields();

   Allocation *pAllocNew = new Allocation( m_pCurrentSet, "New Allocation", -1 );
   m_pCurrentSet->m_allocationArray.Add( pAllocNew );

   // put new Allocation into tree within current allocation set
   gpSAllocView->m_pTreeWnd->InsertAllocation( pAllocNew );
   }


void AllocSetEditor::OnCopy()
   {
   SaveFields(); 

   AllocationSet *pNew = new AllocationSet(*m_pCurrentSet);
   pNew->m_name = "New Allocation Set";

   gpSAllocView->m_allocationSetArray.Add( pNew );

   // put new Allocation into tree within current allocation set
   gpSAllocView->m_pTreeWnd->InsertAllocationSet(pNew);
   }



void AllocSetEditor::OnDelete()
   {
   if ( m_pCurrentSet == NULL )
      return;

   gpSAllocView->m_allocationSetArray.Remove( m_pCurrentSet );      // deletes Allocation
   gpSAllocView->m_pTreeWnd->RemoveAllocationSet( m_pCurrentSet );

   m_pCurrentSet = NULL;
   }


void AllocSetEditor::OnImportXml()
   {
   SaveFields(); 
   gpSAllocView->ImportXml();
   }

void AllocSetEditor::OnSaveXml()
   {
   SaveFields();
   gpSAllocView->SaveXml();
   }


// AllocSetEditor message handlers

void AllocSetEditor::OnOKAS()
   {
   int retVal = AfxMessageBox( "Do you want to save your changes?", MB_YESNOCANCEL );


   switch( retVal )
      {
      case IDYES:
         OnSaveXml();
         break;

      case IDNO:
         break;

      case IDCANCEL:
         return;
      }

   // TODO: Add your specialized code here and/or call the base class

   CDialog::OnOK();
   }


void AllocSetEditor::LoadFields(void)
   {
   if ( m_pCurrentSet == NULL )
      {
      EnableCtrls();
      return;
      }

   // fill idu, sequence field combos
   int col = 0;
   int colSequence = 0;

   m_allocationSetField.ResetContent();
   m_sequenceField.ResetContent();
   m_targetBasis.ResetContent();

   CString hdr = "Allocation Set: ";
   hdr += m_pCurrentSet->m_name;
   GetDlgItem(IDC_AS_DESC)->SetWindowText( hdr );

   // load idu field combo boxes
   for (int i = 0; i < m_pMapLayer->GetColCount(); i++)
      {
      LPCTSTR field = m_pMapLayer->GetFieldLabel(i);
      MAP_FIELD_INFO *pInfo = m_pMapLayer->FindFieldInfo(field);

      CString label;
      if ( pInfo != NULL ) // info found?
         label.Format("%s (%s)", field, (LPCTSTR) pInfo->label );
      else
         label = field;

      // restrict allocationID and sequenceID to integer fields
      if ( ::IsInteger( m_pMapLayer->GetFieldType( i ) ) )
         {
         int index = m_allocationSetField.AddString(field);
         m_allocationSetField.SetItemData(index, i);
         
         index = m_sequenceField.AddString(field);
         m_sequenceField.SetItemData(index, i);      // store the column offset with the item
         }

      // restrict targetBasis to floating point fields
      if ( ::IsReal( m_pMapLayer->GetFieldType( i ) ) )
         {
         int index = m_targetBasis.AddString( field );
         m_targetBasis.SetItemData( index, i );
         }
      }

   // fill rest of fields
   m_allocationSetName = m_pCurrentSet->m_name;
   m_description = m_pCurrentSet->m_description;

   int index = m_allocationSetField.FindString(-1, (LPCTSTR) m_pCurrentSet->m_field );
   m_allocationSetField.SetCurSel( index );

   m_inUse = m_pCurrentSet->m_inUse ? TRUE : FALSE;
   //m_shuffle = m_pCurrentSet->m_shuffleIDUs ? TRUE : FALSE;

   if (m_pCurrentSet->m_seqField.GetLength() != 0)
      {
      m_useSequences = true;
      int index = m_sequenceField.FindString(-1, (LPCTSTR) m_pCurrentSet->m_seqField );
      m_sequenceField.SetCurSel( index );
      }

   index = 0;
   if ((index = m_allocationSetField.FindString(-1, m_pCurrentSet->m_field)) != CB_ERR)
      {
      //found
      col = index;
      }
   else
      {
      // not found, add
      // TODO: check for empty string or add something not empty
      index = m_allocationSetField.AddString(m_pCurrentSet->m_field);
      m_allocationSetField.SetItemData(m_pCurrentSet->m_col, index);
      col = index;
      }

   if (m_useSequences)
      {
      index = 0;
      if ((index = m_sequenceField.FindString(-1, m_pCurrentSet->m_seqField)) != CB_ERR)
         {
         // found
         colSequence = index;
         }
      else
         {
         // not found, add
         // TODO: check for empty string or add something not empty
         index = m_sequenceField.AddString(m_pCurrentSet->m_seqField);
         m_sequenceField.SetItemData(m_pCurrentSet->m_colSequence, index);
         colSequence = index;
         }
      }

   m_allocationSetField.SetCurSel(col);
   m_sequenceField.SetCurSel(colSequence);
   
   // target info
   TARGET_SOURCE targetSource = m_pCurrentSet->m_targetSource;
   if (targetSource == TS_RATE)
      {
      m_targetSourceGroupIndex = 0;
      m_pCurrentSet->GetTargetRate(m_rate);
      }
   else if (targetSource == TS_TIMESERIES)
      {
      m_targetSourceGroupIndex = 1;
      m_timeSeriesValues = m_pCurrentSet->m_targetValues;
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

   // take care of taret basis 
   index = -1;
   if ( m_pCurrentSet->m_targetBasisField.GetLength() > 0 )
      index = m_targetBasis.FindString( -1, (LPCTSTR) m_pCurrentSet->m_targetBasisField );
   else
      index =  m_targetBasis.FindString( -1, "AREA" );
   
   m_targetBasis.SetCurSel( index );
   
   UpdateData( 0 );
   EnableCtrls();   
   }




void AllocSetEditor::OnSize(UINT nType, int cx, int cy)
   {
   CDialog::OnSize(nType, cx, cy);
   UPDATE_EASYSIZE;

   // TODO: Add your message handler code here
   }


bool AllocSetEditor::SaveFields( void )
   {
   // save current dlg entries to local AllocationSet
  // TARGET_SOURCE targetSource = TS_UNDEFINED;

   UpdateData(1);

   if (m_pCurrentSet != NULL)
      {
      m_pCurrentSet->m_name = m_allocationSetName;
      m_pCurrentSet->m_inUse = (m_inUse == 1) ? true : false;
      //m_pCurrentSet->m_shuffleIDUs = (m_shuffle == 1) ? true : false;
      m_pCurrentSet->m_description = m_description;

      // get strings of selected comboboxes
      int colIndex = m_allocationSetField.GetCurSel();
      m_allocationSetField.GetLBText(colIndex, m_pCurrentSet->m_field);

      bool useSequences = (m_useSequences == 1) ? true : false;
      if (useSequences)
         {
         int seqIndex = m_sequenceField.GetCurSel();
         m_sequenceField.GetLBText(seqIndex, m_pCurrentSet->m_seqField);
         }

      TARGET_SOURCE targetSource = TS_UNDEFINED;
      CString targetValues;
      bool useTargetBasis = true;
      if (m_targetSourceGroupIndex == 0)
         {
         m_pCurrentSet->m_targetRate = m_rate;
         targetSource = TS_RATE;
         GetDlgItemText( IDC_GROWTHRATE, targetValues ); 
         }
      else if (m_targetSourceGroupIndex == 1)
         {
         m_pCurrentSet->m_targetValues = m_timeSeriesValues;
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
         useTargetBasis = false;
         }

      m_pCurrentSet->m_targetSource = targetSource;
      m_pCurrentSet->m_targetValues = targetValues;
            
      if ( useTargetBasis )
         {
         int index = m_targetBasis.GetCurSel();

         if ( index < 0 )
            m_pCurrentSet->m_targetBasisField = "AREA";
         else
            m_targetBasis.GetLBText( index, m_pCurrentSet->m_targetBasisField );

         m_pCurrentSet->m_colTargetBasis = m_pMapLayer->GetFieldCol( m_pCurrentSet->m_targetBasisField );
         }
      else
         {
         m_pCurrentSet->m_colTargetBasis = -1;
         m_pCurrentSet->m_targetBasisField.Empty();
         }
      }

   return true;
   }



void AllocSetEditor::EnableCtrls()
   {
   BOOL enabled = 1;
   if ( m_pCurrentSet == NULL )
      enabled = 0;

   GetDlgItem(IDC_AS_NAME)->EnableWindow( enabled );
   GetDlgItem(IDC_DESCRIPTION)->EnableWindow( enabled );
   GetDlgItem(IDC_AS_FIELD)->EnableWindow( enabled );
   GetDlgItem(IDC_INUSE)->EnableWindow( enabled );
   
   GetDlgItem(IDC_USESEQUENCES)->EnableWindow( enabled );
   GetDlgItem(IDC_SEQUENCETITLE)->EnableWindow( enabled );
   GetDlgItem(IDC_SEQUENCEFIELD)->EnableWindow( enabled );

   GetDlgItem(IDC_TTRATE)->EnableWindow( enabled );
   GetDlgItem(IDC_LABELGROWTHRATE)->EnableWindow( enabled );
   GetDlgItem(IDC_GROWTHRATE)->EnableWindow( enabled );

   GetDlgItem(IDC_TTTIMESERIES)->EnableWindow( enabled );
   GetDlgItem(IDC_TIMESERIES)->EnableWindow( enabled );
   
   GetDlgItem(IDC_TTFILE)->EnableWindow( enabled );
   GetDlgItem(IDC_FROMFILE)->EnableWindow( enabled );
   GetDlgItem(IDC_BROWSE)->EnableWindow( enabled );

   GetDlgItem(IDC_TTALLOCATION)->EnableWindow( enabled );
   GetDlgItem(IDC_TARGETBASIS)->EnableWindow( enabled );
   GetDlgItem(IDC_TARGETBASISLABEL)->EnableWindow( enabled );

   GetDlgItem(IDC_AS_ADDNEWALLOC)->EnableWindow( enabled );
   GetDlgItem(IDC_AS_COPY)->EnableWindow( enabled );
   GetDlgItem(IDC_AS_DELETE)->EnableWindow( enabled );   
   GetDlgItem(IDC_AS_SAVE)->EnableWindow( enabled );


   // selectively turn things on and off
   BOOL useTargetBasis = TRUE;
   BOOL checked = IsDlgButtonChecked(IDC_USESEQUENCES);
   GetDlgItem(IDC_SEQUENCETITLE)->EnableWindow( checked );
   GetDlgItem(IDC_SEQUENCEFIELD)->EnableWindow( checked );

   // Rate?
   checked = IsDlgButtonChecked(IDC_TTRATE);
   GetDlgItem(IDC_LABELGROWTHRATE)->EnableWindow( checked );
   GetDlgItem(IDC_GROWTHRATE)->EnableWindow( checked );

   checked = IsDlgButtonChecked(IDC_TTTIMESERIES);
   GetDlgItem(IDC_TIMESERIES)->EnableWindow( checked );
   
   checked = IsDlgButtonChecked(IDC_TTFILE);
   GetDlgItem(IDC_FROMFILE)->EnableWindow( checked );
   GetDlgItem(IDC_BROWSE)->EnableWindow( checked );

   checked = IsDlgButtonChecked(IDC_TTALLOCATION);
   GetDlgItem(IDC_TARGETBASIS)->EnableWindow( checked ? 0 : 1 );
   GetDlgItem(IDC_TARGETBASISLABEL)->EnableWindow( checked ? 0 : 1 );
   }


void AllocSetEditor::OnBnClickedBrowse()
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



void AllocSetEditor::OnEnChangeAsName()
   {
   UpdateData(1);
   m_pCurrentSet->m_name = this->m_allocationSetName;

   gpSAllocView->m_pTreeWnd->RefreshItemLabel( m_pCurrentSet );
   }
