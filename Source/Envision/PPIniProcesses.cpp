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
// PPIniProcesses.cpp : implementation file
//

#include "stdafx.h"
#include "PPIniProcesses.h"

#include "IniFileEditor.h"
#include "EnvDoc.h"
#include <EnvExtension.h>
#include <EnvConstants.h>
#include <EnvLoader.h>
#include <DirPlaceholder.h>
#include <PathManager.h>
#include <maplayer.h>

extern CEnvDoc   *gpDoc;
extern EnvModel *gpModel;

// PPIniProcesses dialog

IMPLEMENT_DYNAMIC(PPIniProcesses, CTabPageSSL)

PPIniProcesses::PPIniProcesses(IniFileEditor *pParent /*=NULL*/)
	: CTabPageSSL(PPIniProcesses::IDD, pParent)
   , m_pParent( pParent )
   , m_label(_T(""))
   , m_path(_T(""))
   , m_timing(FALSE)
   , m_ID(0)
   , m_useCol(FALSE)
   , m_fieldName(_T(""))
   , m_initStr(_T(""))
   , m_isProcessDirty( false )
   , m_isProcessesDirty( false )
{ }

PPIniProcesses::~PPIniProcesses()
{}

void PPIniProcesses::DoDataExchange(CDataExchange* pDX)
{
CTabPageSSL::DoDataExchange(pDX);
DDX_Control(pDX, IDC_APS, m_aps);
DDX_Text(pDX, IDC_LABEL, m_label);
DDX_Text(pDX, IDC_PATH, m_path);
DDX_Radio(pDX, IDC_PREYEAR, m_timing);
DDX_Text(pDX, IDC_ID, m_ID);
DDX_Check(pDX, IDC_USECOL, m_useCol);
DDX_Text(pDX, IDC_FIELD, m_fieldName);
DDX_Text(pDX, IDC_INITINFO, m_initStr);
}


BEGIN_MESSAGE_MAP(PPIniProcesses, CTabPageSSL)
   ON_BN_CLICKED(IDC_BROWSEAPS, &PPIniProcesses::OnBnClickedBrowse)
   ON_BN_CLICKED(IDC_ADDAPS, &PPIniProcesses::OnBnClickedAdd)
   ON_BN_CLICKED(IDC_REMOVEAPS, &PPIniProcesses::OnBnClickedRemove)
   ON_LBN_SELCHANGE(IDC_APS, &PPIniProcesses::OnLbnSelchangeAps)
   ON_CLBN_CHKCHANGE(IDC_APS, &PPIniProcesses::OnLbnChkchangeAps)
   ON_WM_ACTIVATE()
   ON_EN_CHANGE(IDC_LABEL, &PPIniProcesses::OnEnChangeLabel)
   ON_EN_CHANGE(IDC_PATH, &PPIniProcesses::OnEnChangePath)
   ON_EN_CHANGE(IDC_ID, &PPIniProcesses::OnEnChangeID)
   ON_EN_CHANGE(IDC_FIELD, &PPIniProcesses::OnEnChangeField)
   ON_EN_CHANGE(IDC_INITINFO, &PPIniProcesses::OnEnChangeInitStr)
   ON_BN_CLICKED(IDC_PREYEAR, &PPIniProcesses::OnBnClickedTiming)
   ON_BN_CLICKED(IDC_POSTYEAR, &PPIniProcesses::OnBnClickedTiming)
   ON_BN_CLICKED(IDC_USECOL, &PPIniProcesses::OnBnClickedUsecol)
END_MESSAGE_MAP()


BOOL PPIniProcesses::OnInitDialog()
   {
   CTabPageSSL::OnInitDialog();

   EnvModelProcess  *pInfo  = NULL;

   for ( int i=0; i < gpDoc->m_model.GetModelProcessCount(); i++ )
      {
      pInfo = gpDoc->m_model.GetModelProcessInfo( i );
      m_aps.AddString( pInfo->m_name );

      m_aps.SetCheck( i, pInfo->m_use ? 1 : 0 );
      }

   m_aps.SetCurSel( 0 );

   TCHAR _path[ MAX_PATH ];
   GetModuleFileName( NULL, _path, MAX_PATH );

   for ( int i=0; i < lstrlen( _path ); i++ )
      _path[ i ] = tolower( _path[ i ] );

   TCHAR *end = _tcsstr( _path, _T("envision") );
   if ( end != NULL )
      {
      end += 8;
      *end = NULL;

      _chdir( _path );
      }

   if ( gpDoc->m_model.GetModelProcessCount() > 0 )
      LoadAP();

   return TRUE;  // return TRUE unless you set the focus to a control
   }


void PPIniProcesses::LoadAP()
   {
   EnvModelProcess *pInfo = GetAP();

   if ( pInfo == NULL )
      return;

   m_label   = pInfo->m_name;
   m_path    = pInfo->m_path;
   m_timing  = pInfo->m_timing;
   m_ID      = pInfo->m_id;
   //m_useCol  = pInfo->col >= 0 ? TRUE : FALSE;
   //m_fieldName  = pInfo->fieldName;
   m_initStr = pInfo->m_initInfo;

   m_aps.SetCheck( m_aps.GetCurSel(), pInfo->m_use ? 1 : 0 );

   m_isProcessDirty = false;

   UpdateData( 0 );
   }


EnvModelProcess *PPIniProcesses::GetAP()
   {
   int index = m_aps.GetCurSel();

   if ( index >= 0 )
      return gpModel->GetModelProcessInfo( index );
   else
      return NULL;
   }


bool PPIniProcesses::StoreChanges()
   {
   if ( m_isProcessDirty )
      {
      // not needed at this point
      }

   return true;
   }

void PPIniProcesses::MakeDirty()
   {
   m_isProcessesDirty = true;
   m_pParent->MakeDirty( INI_PROCESSES );
   }


void PPIniProcesses::OnLbnChkchangeAps()
   {
   EnvModelProcess *pInfo = GetAP();

   if ( pInfo != NULL )
      {
      int index = m_aps.GetCurSel();
      int check = m_aps.GetCheck( index );

      pInfo->m_use = ( check ? true : false);
      MakeDirty();
      }
   }


void PPIniProcesses::OnBnClickedAdd()
   {
   DirPlaceholder d;

   CString _path = PathManager::GetPath( PM_ENVISION_DIR );
   //char *cwd = _getcwd( NULL, 0 );
   //TCHAR _path[ MAX_PATH ];
   //GetModuleFileName( NULL, _path, MAX_PATH );
   //
   //for ( int i=0; i < lstrlen( _path ); i++ )
   //   _path[ i ] = tolower( _path[ i ] );
   //
   //TCHAR *end = _tcsstr( _path, _T("envision") );
   //if ( end != NULL )
   //   {
   //   end += 8;
   //   *end = NULL;
   //
   //   _chdir( _path );
   //   }

   CFileDialog dlg( TRUE, "dll", _path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "DLL files|*.dll|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString path = dlg.GetPathName();
      int index = m_aps.AddString( path );
      m_aps.SetCurSel( index );

      EnvLoader loader;
      loader.LoadModel( &(gpDoc->m_model), path, path, "", "", 0, 1, 0, NULL, NULL );

      LoadAP();
      MakeDirty();
      }
   }


void PPIniProcesses::OnBnClickedRemove()
   {
   int index = m_aps.GetCurSel();

   if ( index < 0 )
      return;

   m_aps.DeleteString( index );

   gpDoc->m_model.RemoveModelProcess( index );
   m_aps.SetCurSel( 0 );
   LoadAP();
   MakeDirty();
   }


void PPIniProcesses::OnLbnSelchangeAps()
   {
   LoadAP();
   }


void PPIniProcesses::OnBnClickedBrowse()
   {
   DirPlaceholder d;

   TCHAR _path[ MAX_PATH ];
   GetModuleFileName( NULL, _path, MAX_PATH );

   for ( int i=0; i < lstrlen( _path ); i++ )
      _path[ i ] = tolower( _path[ i ] );

   TCHAR *end = _tcsstr( _path, _T("envision") );
   if ( end != NULL )
      {
      end += 8;
      *end = NULL;

      _chdir( _path );
      }

   CFileDialog dlg( TRUE, "dll", "", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "DLL files|*.dll|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      m_path = dlg.GetPathName();

      UpdateData( 0 );
      MakeDirty();
      }
   }


void PPIniProcesses::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
   {
   CTabPageSSL::OnActivate(nState, pWndOther, bMinimized);
   }


void PPIniProcesses::OnEnChangeLabel()
   {
   UpdateData();

   int index = m_aps.GetCurSel();

   if ( index >= 0 )
      {
      m_aps.InsertString( index, m_label );
      m_aps.DeleteString( index+1 );
      m_aps.SetCurSel( index );

      EnvModelProcess *pInfo = gpModel->GetModelProcessInfo( index );

      if ( pInfo != NULL )
         pInfo->m_name = m_label;
   
      MakeDirty();
      }
   }

void PPIniProcesses::OnEnChangePath()
   {
   UpdateData();

   EnvModelProcess *pInfo = GetAP();

   if ( pInfo != NULL )
      {
      pInfo->m_path = m_path;
      MakeDirty();
      }
   }

void PPIniProcesses::OnEnChangeID()
   {
   UpdateData();

   EnvModelProcess *pInfo = GetAP();

   if ( pInfo != NULL )
      {
      pInfo->m_id = m_ID;
      MakeDirty();
      }
   }


void PPIniProcesses::OnBnClickedTiming()
   {
   EnvModelProcess *pInfo = GetAP();

   if ( pInfo == NULL )
      return;

   if ( this->IsDlgButtonChecked( IDC_PREYEAR ) )
      pInfo->m_timing = 0;
   else
      pInfo->m_timing = 1;
   
   MakeDirty();
   }


void PPIniProcesses::OnBnClickedUsecol()
   {
   UpdateData();
   EnvModelProcess *pInfo = GetAP();

   if ( pInfo == NULL )
      return;

   MakeDirty();

   //if ( this->IsDlgButtonChecked( IDC_FIELD ) )
   //   {
   //   GetDlgItem( IDC_FIELD )->EnableWindow( 1 );
   //   pInfo->fieldName = m_fieldName;
   //   }
   //else
   //   {
   //   GetDlgItem( IDC_FIELD )->EnableWindow( 0 );
   //   pInfo->fieldName.Empty();
   //   }
   }

void PPIniProcesses::OnEnChangeField()
   {
   //UpdateData();
   //
   //EnvModelProcess *pInfo = GetAP();
   //
   //if ( pInfo != NULL )
   //   {
   //   pInfo->fieldName = m_fieldName;
   //   MakeDirty();
   //   }
   }

void PPIniProcesses::OnEnChangeInitStr()
   {
   UpdateData();

   EnvModelProcess *pInfo = GetAP();

   if ( pInfo != NULL )
      {
      pInfo->m_initInfo = m_initStr;
      MakeDirty();
      }
   }