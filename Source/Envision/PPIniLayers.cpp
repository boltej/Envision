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
// PPIniLayers.cpp : implementation file
//

#include "stdafx.h"
#include "PPIniLayers.h"

#include "IniFileEditor.h"
#include "MapPanel.h"
#include "EnvDoc.h"
#include "FieldInfoDlg.h"

#include <EnvLoader.h>

#include <map.h>
#include <maplayer.h>
#include <DirPlaceholder.h>
#include <direct.h>


extern MapPanel  *gpMapPanel;
extern CEnvDoc   *gpDoc;
extern MapLayer  *gpCellLayer;


// PPIniLayers dialog

IMPLEMENT_DYNAMIC(PPIniLayers, CTabPageSSL)

PPIniLayers::PPIniLayers(IniFileEditor *pParent /*=NULL*/)
	: CTabPageSSL(PPIniLayers::IDD, pParent)
   , m_pParent( pParent )
   , m_labelLayer(_T(""))
   , m_pathLayer(_T(""))
   , m_pathFieldInfo(_T(""))
   , m_records(-1)
   , m_includeData(TRUE)
{ }

PPIniLayers::~PPIniLayers()
{}

void PPIniLayers::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LAYERS, m_layers);
DDX_Text(pDX, IDC_LABELLAYER, m_labelLayer);
DDX_Text(pDX, IDC_PATHLAYER, m_pathLayer);
DDX_Control(pDX, IDC_COLOR, m_color);
DDX_Text(pDX, IDC_RECORDS, m_records);
DDX_Check(pDX, IDC_INCLUDEDATA, m_includeData);
DDX_Radio(pDX, IDC_SHAPE, m_type);
DDX_Text(pDX, IDC_PATHFIELDINFO, m_pathFieldInfo);
}


BEGIN_MESSAGE_MAP(PPIniLayers, CTabPageSSL)
   ON_BN_CLICKED(IDC_BROWSELAYERS, &PPIniLayers::OnBnClickedBrowselayers)
   ON_BN_CLICKED(IDC_SETCOLOR, &PPIniLayers::OnBnClickedSetcolor)
   ON_BN_CLICKED(IDC_ADDLAYER, &PPIniLayers::OnBnClickedAddlayer)
   ON_BN_CLICKED(IDC_REMOVELAYER, &PPIniLayers::OnBnClickedRemovelayer)
   ON_LBN_SELCHANGE(IDC_LAYERS, &PPIniLayers::OnLbnSelchangeLayers)
   ON_WM_ACTIVATE()
   ON_EN_CHANGE(IDC_LABELLAYER, &PPIniLayers::OnEnChangeLabellayer)
   ON_EN_CHANGE(IDC_PATHLAYER, &PPIniLayers::MakeLayerDirty)
   ON_BN_CLICKED(IDC_BROWSEFIELDINFO, &PPIniLayers::OnBnClickedBrowsefieldinfo)
   ON_EN_CHANGE(IDC_PATHFIELDINFO, &PPIniLayers::MakeLayerDirty)
   ON_BN_CLICKED(IDC_RUNFIELDEDITOR, &PPIniLayers::OnBnClickedRunfieldeditor)
END_MESSAGE_MAP()



BOOL PPIniLayers::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();

   m_color.ShowWindow( SW_HIDE );

   Map *pMap = gpMapPanel->m_pMap;
   
   MapLayer *pLayer = NULL;

   for ( int i=0; i < pMap->GetLayerCount(); i++ )
      {
      pLayer = pMap->GetLayer( i );
      m_layers.AddString( pLayer->m_name );
      }
   m_layers.SetCurSel( 0 );

   if ( pMap->GetLayerCount() > 0 )
      LoadLayer();

  //for ( int i=0; i < CEnvDoc::m_fieldInfoFileArray.GetSize(); i++ )
  //    m_fieldInfo.AddString( CEnvDoc::m_fieldInfoFileArray[ i ] );

   return TRUE;  // return TRUE unless you set the focus to a control
   }


void PPIniLayers::LoadLayer()
   {
   int layer = m_layers.GetCurSel();

   if ( layer < 0 )
      return;

   Map *pMap = gpMapPanel->m_pMap;
   MapLayer *pLayer = pMap->GetLayer( layer );
    
   m_labelLayer    = pLayer->m_name;
   m_pathLayer     = pLayer->m_path;
   m_colorRef      = pLayer->GetOutlineColor();
   m_records       = pLayer->m_recordsLoaded;
   m_includeData   = pLayer->m_pData ? TRUE : FALSE;
   m_pathFieldInfo = pLayer->m_pFieldInfoArray->m_path;

   if ( pLayer->GetType() == LT_POLYGON || pLayer->GetType() == LT_LINE || pLayer->GetType() == LT_POINT )
      m_type = 0;
   else
      m_type = 1;

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

   m_isLayerDirty = false;

   UpdateData( 0 );
   }


bool PPIniLayers::StoreChanges()
   {
   /*
   CEnvDoc::m_fieldInfoFileArray.RemoveAll();
   for ( int i=0; i < m_fieldInfo.GetCount(); i++ )
      {
      CString fieldInfo;
      m_fieldInfo.GetText( i, fieldInfo );
      fieldInfo.Trim();
      if ( fieldInfo.IsEmpty() == false )
         CEnvDoc::m_fieldInfoFileArray.Add( fieldInfo );
      } */
   if ( m_isLayerDirty) 
      SaveCurrentLayer();
   return true;
   }

void PPIniLayers::MakeDirty()
   {
   m_isLayersDirty = true;
   m_pParent->MakeDirty( INI_LAYERS );
   }


void PPIniLayers::MakeLayerDirty()
   {
   MakeDirty();
   m_isLayerDirty = true;
   }


// PPIniLayers message handlers

void PPIniLayers::OnBnClickedBrowselayers()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "shp", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Shape files|*.shp|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_pathLayer = dlg.GetPathName();
      UpdateData( 0 );
      MakeLayerDirty();
      }
   }


void PPIniLayers::OnBnClickedSetcolor()
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

   MakeLayerDirty();
   }


void PPIniLayers::OnBnClickedAddlayer()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "shp", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Shape files|*.shp|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      if ( m_isLayerDirty )
         SaveCurrentLayer();

      CString path = dlg.GetPathName();
      int index = m_layers.AddString( path );
      m_layers.SetCurSel( index );

      CWaitCursor c;
      EnvLoader loader;
      loader.LoadLayer( gpMapPanel->m_pMap, path, path, AMLT_SHAPE, 0,0,0, 0, -1, NULL, NULL, true, true );

      gpMapPanel->m_mapFrame.RefreshList();
      LoadLayer();
      MakeDirty();
      }
   }


void PPIniLayers::OnBnClickedRemovelayer()
   {
   int index = m_layers.GetCurSel();

   if ( index < 0 )
      return;

   m_layers.DeleteString( index );
   gpMapPanel->m_pMap->RemoveLayer( index, true );
   gpMapPanel->m_mapFrame.RefreshList();

   m_layers.SetCurSel( 0 );

   m_isLayerDirty = false;
   LoadLayer();
   MakeDirty();
   }



void PPIniLayers::OnLbnSelchangeLayers()
   {
   if( m_isLayerDirty )
      SaveCurrentLayer();

   LoadLayer();
   }

void PPIniLayers::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
   {
   CTabPageSSL::OnActivate(nState, pWndOther, bMinimized);

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
   }

void PPIniLayers::OnEnChangeLabellayer()
   {
   UpdateData();

   int index = m_layers.GetCurSel();

   if ( index >= 0 )
      {
      m_layers.InsertString( index, m_labelLayer );
      m_layers.DeleteString( index+1 );
      m_layers.SetCurSel( index );

      Map *pMap = gpMapPanel->m_pMap;
      MapLayer *pLayer = pMap->GetLayer( index );
    
      if ( pLayer )
         pLayer->m_name = m_labelLayer;
  
      MakeLayerDirty();
      }
   }


void PPIniLayers::OnBnClickedBrowsefieldinfo()
   {
   //int index = m_fieldInfo.GetCurSel();
   //CString path;
   //m_fieldInfo.GetText( index, path );
   CString path( m_pathFieldInfo );

   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      //path = dlg.GetPathName();
      //m_fieldInfo.InsertString( index, path );
      //m_fieldInfo.DeleteString( index+1 );
      m_pathFieldInfo = dlg.GetPathName();
      MakeLayerDirty();
      }
   }

/*
void PPIniLayers::OnBnClickedRemovefieldinfo()
   {
   int index = m_fieldInfo.GetCurSel();

   if ( index < 0 )
      return;

   m_fieldInfo.DeleteString( index );
   MakeDirty();   
   }

void PPIniLayers::OnBnClickedAddfieldinfo()
   {
   DirPlaceholder d;

   CFileDialog dlg( TRUE, "xml", "*.xml", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "XML files|*.xml|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString path = dlg.GetPathName();
      m_fieldInfo.AddString( path );
      MakeDirty();
      }
   }
*/

void PPIniLayers::OnBnClickedRunfieldeditor()
   {
   FieldInfoDlg dlg;
   if ( dlg.DoModal() == IDOK )
      MakeDirty();
   }

void PPIniLayers::SaveCurrentLayer()
   {
   UpdateData( 1 );

   int layer = m_layers.GetCurSel();

   if ( layer < 0 )
      return;

   Map *pMap = gpMapPanel->m_pMap;
   MapLayer *pLayer = pMap->GetLayer( layer );
    
   pLayer->m_name = m_labelLayer;
   pLayer->m_path = m_pathLayer;
   pLayer->SetOutlineColor( m_colorRef );
   pLayer->m_recordsLoaded = m_records;
   pLayer->m_includeData = m_includeData ? true : false;
   pLayer->m_pFieldInfoArray->m_path = m_pathFieldInfo;

   m_isLayerDirty = false;
   }

