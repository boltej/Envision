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
// MapLabelDlg.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "MapLabelDlg.h"

#include <afxdlgs.h>


// MapLabelDlg dialog

IMPLEMENT_DYNAMIC(MapLabelDlg, CDialog)

MapLabelDlg::MapLabelDlg(Map *pMap, CWnd* pParent /*=NULL*/)
	: CDialog(MapLabelDlg::IDD, pParent )
   , m_pMap( pMap )
   , m_showLabels(FALSE)
   {
   memset(&m_logFont, 0, sizeof(LOGFONT));
   lstrcpy(m_logFont.lfFaceName, "Arial");

   m_font.CreateFontIndirect( &m_logFont );
   }

MapLabelDlg::~MapLabelDlg()
{
}

void MapLabelDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LIB_FONTSAMPLE, m_sample);
DDX_Control(pDX, IDC_LIB_LLAYERS, m_layers);
DDX_Control(pDX, IDC_LIB_FIELDS, m_fields);
DDX_Check(pDX, IDC_LIB_SHOWLABELS, m_showLabels);
   }


BEGIN_MESSAGE_MAP(MapLabelDlg, CDialog)
   ON_BN_CLICKED(IDC_LIB_SETFONT, &MapLabelDlg::OnBnClickedSetfont)
   ON_BN_CLICKED(IDC_LIB_SHOWLABELS, &MapLabelDlg::OnBnClickedShowlabels)
   ON_BN_CLICKED(IDOK, &MapLabelDlg::OnBnClickedOk)
   ON_CBN_EDITCHANGE(IDC_LIB_LLAYERS, &MapLabelDlg::OnCbnEditchangeLayers)
END_MESSAGE_MAP()


// MapLabelDlg message handlers

BOOL MapLabelDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

   m_layers.ResetContent();
   for ( int i=0; i < m_pMap->GetLayerCount(); i++ )
      m_layers.AddString( m_pMap->GetLayer( i )->m_name );
   m_layers.SetCurSel( 0 );

   m_pLayer = m_pMap->GetLayer( 0 );

   m_isDirty = false;

   m_showLabels = m_pLayer->m_showLabels ? TRUE : FALSE;

   LoadFields();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }


void MapLabelDlg::LoadFields()
   {
   ASSERT( m_pMap != NULL );
   m_fields.ResetContent();

   int layer = m_layers.GetCurSel();

   if ( layer >= 0 )
      {
      MapLayer *pLayer = m_pMap->GetLayer( layer );

      for ( int j=0; j < pLayer->GetFieldCount(); j++ )
         m_fields.AddString( pLayer->GetFieldLabel( j ) );

      m_fields.SetCurSel( 0 );
      }
   }


void MapLabelDlg::OnBnClickedSetfont()
   {
   LOGFONT lf;
   memset(&lf, 0, sizeof(LOGFONT));
   m_font.GetLogFont( &lf );

   CFontDialog dlg( &lf );
   dlg.m_cf.rgbColors = m_pLayer->m_labelColor;

   if( dlg.DoModal() == IDOK )
      {
      memcpy( &m_logFont, &dlg.m_lf, sizeof( LOGFONT ) );

      m_font.DeleteObject();
      m_font.CreateFontIndirect( &m_logFont );

      m_sample.SetFont( &m_font );
      }   
   }


void MapLabelDlg::OnBnClickedShowlabels()
   {
   }


void MapLabelDlg::OnBnClickedOk()
   {
   OnOK();
   }

void MapLabelDlg::OnCbnEditchangeLayers()
   {
   UpdateData( 1 );

   if ( m_isDirty  && m_pLayer != NULL )
      {
      int result = MessageBox( _T("Do you want to save label information for this layer?" ), _T("Save Label Information"), MB_YESNO );
      if ( result == IDYES )
         {
         int col = m_fields.GetCurSel();
         m_pLayer->SetLabelFont( m_logFont.lfFaceName );
         m_pLayer->SetLabelSize( float ( m_logFont.lfHeight/10 ) );
         m_pLayer->SetLabelField( col );
         m_pLayer->SetLabelColor( m_color );
         if ( m_showLabels )
            m_pLayer->ShowLabels();
         else
            m_pLayer->HideLabels();
         }
      }

   m_isDirty = false;
   LoadFields();

   int layer = m_layers.GetCurSel();
   if( layer >= 0 )
      {
      m_pLayer = m_pMap->GetLayer( layer );
      memcpy( &m_logFont, &m_pLayer->m_labelFont, sizeof( LOGFONT ) );

      m_showLabels = m_pLayer->m_showLabels ? TRUE : FALSE;
      m_color = m_pLayer->m_labelColor;
      m_logFont.lfHeight = int( m_pLayer->m_labelSize*10 );

      m_font.DeleteObject();
      m_font.CreateFontIndirect( &m_logFont );
   
      m_sample.SetFont( &m_font );
      }
   }
