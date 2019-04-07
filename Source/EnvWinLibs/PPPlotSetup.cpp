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
// PPPlotSetup.cpp : implementation file
//

#include "EnvWinLibs.h"
#pragma hdrstop

#include "graphwnd.h"
#include "PPPlotSetup.h"


// PPPlotSetup dialog

IMPLEMENT_DYNAMIC(PPPlotSetup, CPropertyPage)
PPPlotSetup::PPPlotSetup( GraphWnd *pGraph )
	: CPropertyPage(PPPlotSetup::IDD),
   m_pGraph( pGraph )
{
}

PPPlotSetup::~PPPlotSetup()
{
}

void PPPlotSetup::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(PPPlotSetup, CPropertyPage)
   ON_BN_CLICKED(IDC_LIB_FONTTITLE, &PPPlotSetup::OnBnClickedFonttitle)
   ON_BN_CLICKED(IDC_LIB_FONTXAXIS, &PPPlotSetup::OnBnClickedFontxaxis)
   ON_BN_CLICKED(IDC_LIB_FONTYAXIS, &PPPlotSetup::OnBnClickedFontyaxis)
   ON_EN_CHANGE(IDC_LIB_CAPTION, &PPPlotSetup::OnEnChangeCaption)
   ON_EN_CHANGE(IDC_LIB_TITLE, &PPPlotSetup::OnEnChangeTitle)
   ON_EN_CHANGE(IDC_LIB_XAXIS, &PPPlotSetup::OnEnChangeXaxis)
   ON_EN_CHANGE(IDC_LIB_YAXIS, &PPPlotSetup::OnEnChangeYaxis)
   ON_BN_CLICKED(IDC_LIB_SHOWHORZ, &PPPlotSetup::OnBnClickedShowhorz)
   ON_BN_CLICKED(IDC_LIB_SHOWVERT, &PPPlotSetup::OnBnClickedShowvert)
   ON_BN_CLICKED(IDC_LIB_GROOVED, &PPPlotSetup::OnBnClickedGrooved)
   ON_BN_CLICKED(IDC_LIB_RAISED, &PPPlotSetup::OnBnClickedRaised)
   ON_BN_CLICKED(IDC_LIB_SUNKEN, &PPPlotSetup::OnBnClickedSunken)
END_MESSAGE_MAP()


BOOL PPPlotSetup::OnInitDialog()
   {
   CPropertyPage::OnInitDialog();

   //-- initialization --//
   char buffer[ 120 ];

   //-- window caption --//
   m_pGraph->GetWindowText( buffer, 120 );
   SetDlgItemText( IDC_LIB_CAPTION, (LPCTSTR) buffer );

   //- plot labels --//
   SetDlgItemText( IDC_LIB_TITLE, (LPCTSTR) m_pGraph->label[ 0 ].text );
   SetDlgItemText( IDC_LIB_XAXIS, (LPCTSTR) m_pGraph->label[ 1 ].text );
   SetDlgItemText( IDC_LIB_YAXIS, (LPCTSTR) m_pGraph->label[ 2 ].text );

   //-- check marks --//
   CheckDlgButton( IDC_LIB_SHOWVERT, m_pGraph->showXGrid );
   CheckDlgButton( IDC_LIB_SHOWHORZ, m_pGraph->showYGrid );

   CheckDlgButton( IDC_LIB_GROOVED, m_pGraph->frameStyle & FS_GROOVED );
   CheckDlgButton( IDC_LIB_SUNKEN,  m_pGraph->frameStyle & FS_SUNKEN );
   CheckDlgButton( IDC_LIB_RAISED,  m_pGraph->frameStyle & FS_RAISED );

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


BOOL PPPlotSetup::OnCommand(WPARAM wParam, LPARAM lParam)
   {
   int ctrlCode = HIWORD( wParam );
   int ctrlID   = LOWORD( wParam );

   if ( ctrlCode = BN_CLICKED )
      {
      switch( ctrlID )
         {
         case IDC_LIB_FONTTITLE:
         case IDC_LIB_FONTXAXIS:
         case IDC_LIB_FONTYAXIS:
            {
            int labelID = TITLE;
            if ( ctrlID == IDC_LIB_FONTXAXIS )
               labelID = XAXIS;
            else if ( ctrlID == IDC_LIB_FONTYAXIS )
               labelID = YAXIS;

            // get current font:
            LOGFONT lf;
            memset( &lf, 0, sizeof( LOGFONT ) );

            /*CFont &font = m_pGraph->label[ labelID ].font;
            font.GetLogFont( &lf );

            CFontDialog dlg(&lf);
            if ( dlg.DoModal() == IDOK )
               {
               font.CreateFontIndirect( &lf );
               m_pGraph->RedrawWindow();
               } */
            } 

         }
      }

   return CPropertyPage::OnCommand(wParam, lParam);
   }



bool PPPlotSetup::Apply( bool redraw /*=true*/ )
   {
   //-- get current values and load into box --//
   char buffer[ 120 ];

   //-- caption --//
   GetDlgItemText( IDC_LIB_CAPTION, buffer, 120 );
   m_pGraph->SetWindowText((LPCTSTR) buffer );

   GetDlgItemText( IDC_LIB_TITLE, buffer, 120 );
   m_pGraph->SetLabelText( TITLE, buffer );

   GetDlgItemText( IDC_LIB_XAXIS, buffer, 120 );
   m_pGraph->SetLabelText( XAXIS, buffer );

   GetDlgItemText( IDC_LIB_YAXIS, buffer, 120 );
   m_pGraph->SetLabelText( YAXIS, buffer );

   //-- scaling --//
   m_pGraph->showXGrid = IsDlgButtonChecked( IDC_LIB_SHOWVERT ) ? true : false;
   m_pGraph->showYGrid = IsDlgButtonChecked( IDC_LIB_SHOWHORZ ) ? true : false;

   //-- frame style --//
   m_pGraph->frameStyle = 0;
   if ( IsDlgButtonChecked( IDC_LIB_GROOVED ) )
      m_pGraph->frameStyle |= FS_GROOVED;
   else if ( IsDlgButtonChecked( IDC_LIB_SUNKEN ) )
      m_pGraph->frameStyle |= FS_SUNKEN;
   else if ( IsDlgButtonChecked( IDC_LIB_RAISED ) )
      m_pGraph->frameStyle |= FS_RAISED;
      
   if ( redraw )
      m_pGraph->RedrawWindow();

   return true;
   }


void PPPlotSetup::OnBnClickedFonttitle()
   {
   CFontDialog dlg(&(m_pGraph->label[ TITLE ].logFont ) );
   if ( dlg.DoModal() == IDOK )
      m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnBnClickedFontxaxis()
   {
   CFontDialog dlg(&(m_pGraph->label[ XAXIS ].logFont ) );
   if ( dlg.DoModal() == IDOK )
      m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnBnClickedFontyaxis()
   {
   CFontDialog dlg(&(m_pGraph->label[ YAXIS ].logFont ) );
   if ( dlg.DoModal() == IDOK )
      m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnEnChangeCaption()
   {
   CString text;
   GetDlgItemText( IDC_LIB_CAPTION, text );

   m_pGraph->SetWindowText( text );
   }


void PPPlotSetup::OnEnChangeTitle()
   {
   CString text;
   GetDlgItemText( IDC_LIB_TITLE, text );

   m_pGraph->label[ TITLE ].text = text;
   m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnEnChangeXaxis()
   {
   CString text;
   GetDlgItemText( IDC_LIB_XAXIS, text );

   m_pGraph->label[ XAXIS ].text = text;
   m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnEnChangeYaxis()
   {
   CString text;
   GetDlgItemText( IDC_LIB_YAXIS, text );

   m_pGraph->label[ YAXIS ].text = text;
   m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnBnClickedShowhorz()
   {
   m_pGraph->showYGrid = IsDlgButtonChecked( IDC_LIB_SHOWHORZ ) ? true : false;
   m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnBnClickedShowvert()
   {
   m_pGraph->showXGrid = IsDlgButtonChecked( IDC_LIB_SHOWVERT ) ? true : false;
   m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnBnClickedGrooved()
   {
   m_pGraph->frameStyle = FS_GROOVED;
   m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnBnClickedRaised()
   {
   m_pGraph->frameStyle = FS_RAISED;
   m_pGraph->RedrawWindow();
   }


void PPPlotSetup::OnBnClickedSunken()
   {
   m_pGraph->frameStyle = FS_SUNKEN;
   m_pGraph->RedrawWindow();
   }
