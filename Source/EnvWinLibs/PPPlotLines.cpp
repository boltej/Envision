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
// PPPlotLines.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "EnvWinLib_resource.h"
#include "combo.hpp"
#include <ppplotlines.h>
#include <graphwnd.h>


// PPPlotLines dialog

IMPLEMENT_DYNAMIC(PPPlotLines, CPropertyPage)
PPPlotLines::PPPlotLines( GraphWnd *pGraph )
	: CPropertyPage(PPPlotLines::IDD),
     m_pGraph( pGraph ),
     m_isDirty( false )
     , m_tension(0)
   { }


PPPlotLines::~PPPlotLines()
{ }

void PPPlotLines::DoDataExchange(CDataExchange* pDX)
{
CPropertyPage::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LIB_LINE, m_lines);
//DDX_Control(pDX, IDC_LIB_LINECOLOR, m_color);
//DDX_Control(pDX, IDC_LIB_LINESTYLE, m_lineStyle);
//DDX_Control(pDX, IDC_LIB_LINEWIDTH, m_lineWidth);
//DDX_Control(pDX, IDC_LIB_MARKER, m_markerSymbol);
DDX_Text(pDX, IDC_LIB_TENSION, m_tension);
   }


BEGIN_MESSAGE_MAP(PPPlotLines, CPropertyPage)
   ON_WM_DRAWITEM()
   ON_WM_MEASUREITEM()
   ON_LBN_SELCHANGE(IDC_LIB_LINE, &PPPlotLines::OnLbnSelchangeLine)
   ON_EN_CHANGE(IDC_LIB_LINELABEL, &PPPlotLines::OnEnChangeLinelabel)
   ON_CBN_SELCHANGE(IDC_LIB_LINECOLOR, &PPPlotLines::OnCbnSelchangeLinecolor)
   ON_CBN_SELCHANGE(IDC_LIB_LINESTYLE, &PPPlotLines::OnCbnSelchangeLinestyle)
   ON_CBN_SELCHANGE(IDC_LIB_LINEWIDTH, &PPPlotLines::OnCbnSelchangeLinewidth)
   ON_CBN_SELCHANGE(IDC_LIB_MARKER, &PPPlotLines::OnCbnSelchangeMarker)
   ON_EN_CHANGE(IDC_LIB_MARKERINCR, &PPPlotLines::OnEnChangeMarkerincr)
   ON_BN_CLICKED(IDC_LIB_SHOWLINE, &PPPlotLines::OnBnClickedShowline)
   ON_BN_CLICKED(IDC_LIB_SMOOTH, &PPPlotLines::OnBnClickedSmooth)
   ON_BN_CLICKED(IDC_LIB_SHOWMARKER, &PPPlotLines::OnBnClickedShowmarker)
   ON_EN_CHANGE(IDC_LIB_LEGENDHEIGHT, &PPPlotLines::OnEnChangeLegendheight)
   ON_EN_CHANGE(IDC_LIB_TENSION, &PPPlotLines::OnEnChangeTension)
END_MESSAGE_MAP()


// PPPlotLines message handlers


BOOL PPPlotLines::OnInitDialog()
   {
   CPropertyPage::OnInitDialog();

   //-- load line listbox --//
   for ( int i=0; i < m_pGraph->GetLineCount(); i++ )
      {
      m_lines.AddString( m_pGraph->GetLineInfo( i )->text );
      //m_lines.SetCheck( 1, 1 );
      }

   m_lines.SetCurSel( 0 );

   //-- owner draws --//
   InitComboColor    ( m_hWnd, IDC_LIB_LINECOLOR );
   InitComboLineStyle( m_hWnd, IDC_LIB_LINESTYLE );
   InitComboLineWidth( m_hWnd, IDC_LIB_LINEWIDTH );
   InitComboMarker   ( m_hWnd, IDC_LIB_MARKER );

   SetDlgItemInt( IDC_LIB_LEGENDHEIGHT, m_pGraph->m_legendRowHeight );

   LoadLineInfo();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void PPPlotLines::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT pItem)
   {
   switch( nIDCtl )
      {
      case IDC_LIB_LINECOLOR:                           // "color" combo
         DrawComboColor( m_hWnd, pItem );
         break;

      case IDC_LIB_LINESTYLE:                           // "line style" combo
         DrawComboLineStyle( m_hWnd, pItem );
         break;

      case IDC_LIB_LINEWIDTH:                           // "line width" combo
         DrawComboLineWidth( m_hWnd, pItem );
         break;

      case IDC_LIB_MARKER:                           // "marker" combo
         DrawComboMarker( m_hWnd, pItem );
         break;

      //case IDC_LIB_LINE:
      //   m_lines.DrawItem( pItem );
      //   return;
      }

   CPropertyPage::OnDrawItem(nIDCtl, pItem);
   }


void PPPlotLines::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT pItem)
   {
   switch( nIDCtl )
      {
      case IDC_LIB_LINECOLOR:    // "color" combo
      case IDC_LIB_LINESTYLE:    // "line style" combo
      case IDC_LIB_LINEWIDTH:    // "line width" combo
      case IDC_LIB_MARKER:    // "marker" combo
         {
         DWORD dlgUnits = GetDialogBaseUnits();

         pItem->itemWidth  = ( 38 * LOWORD( dlgUnits ) ) / 4;
         pItem->itemHeight = ( 8  * HIWORD( dlgUnits ) ) / 8; 
         }
         break;

      //case IDC_LIB_LINE:
      //   m_lines.MeasureItem( pItem );
      //   return;
      }

   CPropertyPage::OnMeasureItem(nIDCtl, pItem);
   }


void PPPlotLines::LoadLineInfo( void )
   {
   //-- get current values and load into box --//
   m_lineNo = m_lines.GetCurSel();
   if ( m_lineNo < 0 )
      return;

   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   //-- label --//
   SetDlgItemText( IDC_LIB_LINELABEL, (LPCTSTR) pLine->text );

   //-- load combo boxes --//
   ::SendDlgItemMessage( m_hWnd, IDC_LIB_LINECOLOR, CB_SELECTSTRING, -1, (LPARAM) pLine->color );
   ::SendDlgItemMessage( m_hWnd, IDC_LIB_LINESTYLE, CB_SETCURSEL, (WPARAM) pLine->lineStyle, 0L );
   ::SendDlgItemMessage( m_hWnd, IDC_LIB_LINEWIDTH, CB_SETCURSEL, (WPARAM) pLine->lineWidth, 0L );

   SetComboMarker( m_hWnd, IDC_LIB_MARKER, pLine->markerStyle );

   //-- scale factors --//
   ::SetDlgItemInt( m_hWnd, IDC_LIB_MARKERINCR, (UINT) pLine->markerIncr, TRUE );

   CheckDlgButton(IDC_LIB_SHOWLINE, pLine->showLine   );
   CheckDlgButton( IDC_LIB_SMOOTH, pLine->m_smooth );
   CheckDlgButton(IDC_LIB_SHOWMARKER, pLine->showMarker );
   m_tension = pLine->m_tension;
   
   UpdateData( 0 );
   }


void PPPlotLines::OnLbnSelchangeLine()
   {
   LoadLineInfo();
   }


void PPPlotLines::OnEnChangeLinelabel()
   {
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   CString label;
   this->GetDlgItemText( IDC_LIB_LINELABEL, label );
   pLine->text = label;

   m_lines.InsertString( m_lineNo, label );
   m_lines.DeleteString( m_lineNo+1 );
   m_lines.SetCurSel( m_lineNo );

   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnCbnSelchangeLinecolor()
   {
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   int index = (int) SendDlgItemMessage( IDC_LIB_LINECOLOR, CB_GETCURSEL, 0, 0 );
   //int index = m_color.GetCurSel();
   pLine->color = GetComboColor( index );
   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnCbnSelchangeLinestyle()
   {
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   int index = (int) SendDlgItemMessage( IDC_LIB_LINESTYLE, CB_GETCURSEL, 0, 0 );
   //int index = m_lineStyle.GetCurSel();
   LOGPEN logPen;
   
   if ( GetComboLineStyle( index, &logPen ) )
      pLine->lineStyle = logPen.lopnStyle;

   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnCbnSelchangeLinewidth()
   {
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   int index = (int) SendDlgItemMessage( IDC_LIB_LINEWIDTH, CB_GETCURSEL, 0, 0 );
   //int index = m_lineWidth.GetCurSel();
   pLine->lineWidth = GetComboLineWidth( index );
   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnCbnSelchangeMarker()
   {
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   int index = (int) SendDlgItemMessage( IDC_LIB_MARKER, CB_GETCURSEL, 0, 0 );
   //int index = m_markerSymbol.GetCurSel();
   pLine->markerStyle = GetComboMarker( index );
   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnEnChangeMarkerincr()
   {
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   int incr = GetDlgItemInt( IDC_LIB_MARKERINCR );
   pLine->markerIncr = incr;
   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnBnClickedShowline()
   {
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   pLine->showLine = IsDlgButtonChecked( IDC_LIB_SHOWLINE ) ? true : false;
   
   EnableControls();
   
   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnBnClickedShowmarker()
   {
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   pLine->showMarker = IsDlgButtonChecked( IDC_LIB_SHOWMARKER ) ? true : false;
   m_pGraph->RedrawWindow();
   }

void PPPlotLines::OnBnClickedSmooth()
   {
   UpdateData();
   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );

   pLine->m_smooth = IsDlgButtonChecked( IDC_LIB_SMOOTH ) ? true : false;
   EnableControls();
   
   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnEnChangeLegendheight()
   {
   int height = GetDlgItemInt( IDC_LIB_LEGENDHEIGHT );
   m_pGraph->m_legendRowHeight = height;
   m_pGraph->RedrawWindow();
   }


void PPPlotLines::OnEnChangeTension()
   {
   UpdateData( 1 );

   m_lineNo = m_lines.GetCurSel();
   LINE *pLine =  m_pGraph->GetLineInfo( m_lineNo );
   pLine->m_tension = m_tension;

   m_pGraph->RedrawWindow();
   }

void PPPlotLines::EnableControls( void )
   {
   BOOL showLine = IsDlgButtonChecked( IDC_LIB_SHOWLINE );
   BOOL smooth   = IsDlgButtonChecked( IDC_LIB_SMOOTH );

   GetDlgItem( IDC_LIB_SMOOTH )->EnableWindow( showLine );
   GetDlgItem( IDC_LIB_TENSIONLABEL )->EnableWindow( showLine & smooth );
   GetDlgItem( IDC_LIB_TENSION )->EnableWindow( showLine & smooth );
   }