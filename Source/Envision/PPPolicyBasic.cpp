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
// PPPolicyBasic.cpp : implementation file
//

#include "stdafx.h"

#include ".\pppolicybasic.h"

#include "PolEditor.h"


// PPPolicyBasic dialog
IMPLEMENT_DYNAMIC(PPPolicyBasic, CTabPageSSL)

PPPolicyBasic::PPPolicyBasic( PolEditor *pParent, Policy *&pPolicy )
: CTabPageSSL()
  , m_narrative(_T(""))
  , m_persistenceMin(0)
  , m_persistenceMax(0)
  , m_isScheduled( FALSE )
  , m_mandatory(FALSE)
  , m_startDate(0)
  , m_endDate(0)
  , m_exclusive(FALSE)
  , m_useStartDate( false )
  , m_useEndDate( false )
  , m_isShared( false )
  , m_pPolicy( pPolicy )
  , m_pParent( pParent )
   { }


PPPolicyBasic::~PPPolicyBasic()
   { }

void PPPolicyBasic::DoDataExchange(CDataExchange* pDX)
   {
   CTabPageSSL::DoDataExchange(pDX);

   DDX_Control(pDX, IDC_COLOR, m_color);
   DDX_Text(pDX, IDC_NARRATIVE, m_narrative);
   DDX_Text(pDX, IDC_PERSISTENCEMIN, m_persistenceMin);
   DDV_MinMaxInt(pDX, m_persistenceMin, 0, 500);
   DDX_Text(pDX, IDC_PERSISTENCEMAX, m_persistenceMax);
   DDV_MinMaxInt(pDX, m_persistenceMax, 0, 500);
   DDX_Check(pDX, IDC_MANDATORY, m_mandatory);
   DDX_Check(pDX, IDC_ISSCHEDULED, m_isScheduled);
   DDX_Check(pDX, IDC_USESTARTDATE, m_useStartDate);
   DDX_Check(pDX, IDC_USEENDDATE, m_useEndDate);
   DDX_Text(pDX, IDC_STARTDATE, m_startDate);
   DDX_Text(pDX, IDC_ENDDATE, m_endDate);
   DDX_Check(pDX, IDC_EXCLUSIVE, m_exclusive);
   DDX_Check(pDX, IDC_ISSHARED, m_isShared);
   DDX_Check(pDX, IDC_ISEDITABLE, m_isEditable);
   }

BEGIN_MESSAGE_MAP(PPPolicyBasic, CTabPageSSL)
	ON_BN_CLICKED(IDC_SETCOLOR, OnSetcolor)
   ON_BN_CLICKED( IDC_ISSCHEDULED, OnBnClickedIsScheduled)
   ON_BN_CLICKED(IDC_MANDATORY, OnBnClickedMandatory)
   ON_BN_CLICKED(IDC_EXCLUSIVE, MakeDirty)
   ON_BN_CLICKED(IDC_PERSISTCHECK, OnBnClickedPersistcheck)
   ON_EN_UPDATE(IDC_NARRATIVE, MakeDirty)
   ON_EN_UPDATE(IDC_COMPLIANCE, MakeDirty)
   ON_EN_UPDATE(IDC_STARTDATE, MakeDirty)
   ON_EN_UPDATE(IDC_ENDDATE, MakeDirty)
   ON_EN_UPDATE(IDC_PERSISTENCEMIN, MakeDirty)
   ON_EN_UPDATE(IDC_PERSISTENCEMAX, MakeDirty)
   ON_BN_CLICKED(IDC_USESTARTDATE, OnBnClickedUsestartdate)
   ON_BN_CLICKED(IDC_USEENDDATE, OnBnClickedUseenddate)
   ON_BN_CLICKED(IDC_ISSHARED, OnBnClickedIsshared)   
END_MESSAGE_MAP()

// PPPolicyBasic message handlers
/////////////////////////////////////////////////////////////////////////////

BOOL PPPolicyBasic::OnInitDialog() 
   {   
	CDialog::OnInitDialog();

   m_color.ShowWindow( SW_HIDE );

	return TRUE;
   }


void PPPolicyBasic::MakeDirty( void )
   {
   m_pParent->MakeDirty( DIRTY_BASIC);
   }


void PPPolicyBasic::LoadPolicy()
   {
   if ( m_pPolicy == NULL )
      return;

   // color
   m_colorRef = m_pPolicy->m_color;

   RECT rect;
   m_color.GetWindowRect( &rect );

   this->ScreenToClient( &rect );
   CDC *pDC = this->GetDC();
   CBrush br;
   br.CreateSolidBrush( m_colorRef );
   CBrush *pOldBr = pDC->SelectObject( &br );
   pDC->Rectangle( &rect );
   pDC->SelectObject( pOldBr );
   this->ReleaseDC( pDC );

   // other basic info
   if ( m_pPolicy->m_persistenceMin < 0 )
      CheckDlgButton( IDC_PERSISTCHECK, FALSE );      
   else
      {
      m_persistenceMin = m_pPolicy->m_persistenceMin;
      m_persistenceMax = m_pPolicy->m_persistenceMax;
      CheckDlgButton( IDC_PERSISTCHECK, TRUE );
      }

   m_mandatory   = m_pPolicy->m_mandatory ? 1 : 0;
   m_startDate   = m_pPolicy->m_startDate;
   m_endDate     = m_pPolicy->m_endDate;
   m_exclusive   = m_pPolicy->m_exclusive ? 1 : 0;
   m_isShared    = m_pPolicy->m_isShared ? 1 : 0;
   m_isScheduled = m_pPolicy->m_isScheduled ? 1 : 0;
   m_isEditable  = m_pPolicy->m_isEditable ? 1 : 0;
   m_colorRef    = m_pPolicy->m_color;

   CheckDlgButton( IDC_USESTARTDATE, m_pPolicy->m_startDate <= 0 ? 0 : 1 );
   CheckDlgButton( IDC_USEENDDATE,   m_pPolicy->m_endDate <= 0 ? 0 : 1 );

   //m_constraints = m_pPolicy->m_siteAttrStr;
   //m_outcomes = m_pPolicy->m_outcomeStr;
   m_narrative = m_pPolicy->m_narrative;

   UpdateData( FALSE );  // loads the controls with updated data
   EnableControls();
   }


bool PPPolicyBasic::StoreChanges()
   {
   UpdateData( TRUE );  // save and validate member variables

   m_pPolicy->m_color      = m_colorRef;
   m_pPolicy->m_mandatory  = m_mandatory == TRUE ? true : false;
   m_pPolicy->m_isShared   = m_isShared ? true : false;
   m_pPolicy->m_isEditable = m_isEditable ? true : false;
   m_pPolicy->m_isScheduled= m_isScheduled ? true : false;

   m_pPolicy->m_exclusive = m_exclusive == TRUE ? true : false;
   m_pPolicy->m_narrative = m_narrative;

   
   if ( IsDlgButtonChecked( IDC_USESTARTDATE ) )
      m_pPolicy->m_startDate  = m_startDate;
   else
      m_pPolicy->m_startDate  = -1;

   if ( IsDlgButtonChecked( IDC_USEENDDATE ) )
      m_pPolicy->m_endDate  = m_endDate;
   else
      m_pPolicy->m_endDate  = -1;
   
   if ( IsDlgButtonChecked( IDC_PERSISTCHECK ) )
      {
      m_pPolicy->m_persistenceMin = m_persistenceMin;
      m_pPolicy->m_persistenceMax = m_persistenceMax;
      }
   else
      {
      m_pPolicy->m_persistenceMin = -1;
      m_pPolicy->m_persistenceMax = -1;
      }

   return true;
   }


void PPPolicyBasic::EnableControls()
   {
   BOOL editable = m_pParent->IsEditable() ? TRUE : FALSE;

   GetDlgItem( IDC_PERSISTCHECK   )->EnableWindow( editable );
   GetDlgItem( IDC_PERSISTENCEMIN )->EnableWindow( editable );
   GetDlgItem( IDC_PERSISTENCEMAX )->EnableWindow( editable );
   GetDlgItem( IDC_SETCOLOR       )->EnableWindow( editable );
   GetDlgItem( IDC_MANDATORY      )->EnableWindow( editable );
   GetDlgItem( IDC_EXCLUSIVE      )->EnableWindow( editable );
   GetDlgItem( IDC_USESTARTDATE   )->EnableWindow( editable );
   GetDlgItem( IDC_USEENDDATE     )->EnableWindow( editable );
   GetDlgItem( IDC_STARTDATE      )->EnableWindow( editable );
   GetDlgItem( IDC_ENDDATE        )->EnableWindow( editable );
   GetDlgItem( IDC_ISSHARED       )->EnableWindow( editable );
   GetDlgItem( IDC_ISEDITABLE     )->EnableWindow( editable );
   GetDlgItem( IDC_NARRATIVE      )->EnableWindow( editable );
   GetDlgItem( IDC_ISSCHEDULED    )->EnableWindow( editable );
   GetDlgItem( IDC_COMPLIANCE     )->EnableWindow( editable );

   if ( editable )
      {
      if ( m_pPolicy->m_persistenceMin < 0 )
         {
         GetDlgItem( IDC_PERSISTENCEMIN )->EnableWindow( FALSE );
         GetDlgItem( IDC_PERSISTENCEMAX )->EnableWindow( FALSE );
         GetDlgItem( IDC_MINLABEL )->EnableWindow( FALSE );
         GetDlgItem( IDC_MAXLABEL )->EnableWindow( FALSE );
         }
      else
         {
         GetDlgItem( IDC_PERSISTENCEMIN )->EnableWindow( TRUE );
         GetDlgItem( IDC_PERSISTENCEMAX )->EnableWindow( TRUE );
         GetDlgItem( IDC_MINLABEL )->EnableWindow( TRUE );
         GetDlgItem( IDC_MAXLABEL )->EnableWindow( TRUE );
         }

      if ( IsDlgButtonChecked( IDC_USESTARTDATE ) )
         GetDlgItem( IDC_STARTDATE )->EnableWindow( TRUE );
      else
         GetDlgItem( IDC_STARTDATE )->EnableWindow( FALSE );

      if ( IsDlgButtonChecked( IDC_USEENDDATE ) )
         GetDlgItem( IDC_ENDDATE )->EnableWindow( TRUE );
      else
         GetDlgItem( IDC_ENDDATE )->EnableWindow( FALSE );
     
      if ( IsDlgButtonChecked( IDC_MANDATORY ) )
         GetDlgItem( IDC_COMPLIANCE )->EnableWindow( TRUE );
      else
         GetDlgItem( IDC_COMPLIANCE )->EnableWindow( FALSE );
      }
   }


void PPPolicyBasic::OnSetcolor() 
   {
   CColorDialog dlg;
   dlg.m_cc.rgbResult = m_colorRef;
   dlg.m_cc.Flags |= CC_RGBINIT;

   if ( dlg.DoModal() != IDOK )
      return;

   // selected new color, get it and refresh dialog box
   m_colorRef = dlg.m_cc.rgbResult;

   RECT rect;
   m_color.GetWindowRect( &rect );
   this->ScreenToClient( &rect );
   CDC *pDC = this->GetDC();
   CBrush br;
   br.CreateSolidBrush( m_colorRef );
   CBrush *pOldBr = pDC->SelectObject( &br );
   pDC->Rectangle( &rect );
   pDC->SelectObject( pOldBr );
   this->ReleaseDC( pDC );

   MakeDirty();
   }


void PPPolicyBasic::OnBnClickedPersistcheck()
   {
   MakeDirty();

   BOOL enable = FALSE;
   if ( IsDlgButtonChecked( IDC_PERSISTCHECK ) )
      enable = TRUE;

   GetDlgItem( IDC_PERSISTENCEMIN )->EnableWindow( enable );
   GetDlgItem( IDC_PERSISTENCEMAX )->EnableWindow( enable );
   GetDlgItem( IDC_MINLABEL )->EnableWindow( enable );
   GetDlgItem( IDC_MAXLABEL )->EnableWindow( enable );
   }


void PPPolicyBasic::OnBnClickedIsshared()
   {
   MakeDirty();
   }

void PPPolicyBasic::OnBnClickedMandatory()
   {
   MakeDirty();
   EnableControls();
   //BOOL enable = FALSE;

   //if ( IsDlgButtonChecked( IDC_ISSCHEDULED ) )
   //   enable = TRUE;

   //GetDlgItem( IDC_STARTDATE )->EnableWindow( enable );
   //CheckDlgButton( IDC_USESTARTDATE, enable );
   }


void PPPolicyBasic::OnBnClickedIsScheduled()
   {
   MakeDirty();
   //BOOL enable = FALSE;

   //if ( IsDlgButtonChecked( IDC_ISSCHEDULED ) )
   //   enable = TRUE;

   //GetDlgItem( IDC_STARTDATE )->EnableWindow( enable );
   //CheckDlgButton( IDC_USESTARTDATE, enable );
   }


void PPPolicyBasic::OnBnClickedUsestartdate()
   {
   MakeDirty();

   BOOL enable = FALSE;
   if ( IsDlgButtonChecked( IDC_USESTARTDATE ) )
      enable = TRUE;

   GetDlgItem( IDC_STARTDATE )->EnableWindow( enable );
   }


void PPPolicyBasic::OnBnClickedUseenddate()
   {
   MakeDirty();

   BOOL enable = FALSE;
   if ( IsDlgButtonChecked( IDC_USEENDDATE ) )
      enable = TRUE;

   GetDlgItem( IDC_ENDDATE )->EnableWindow( enable );
   }

