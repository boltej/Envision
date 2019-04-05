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
// ReclassDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "ReclassDlg.h"
#include "afxdialogex.h"

#include "EnvDoc.h"
#include <MAP.h>
#include <Maplayer.h>
#include <DirPlaceholder.h>

extern MapLayer *gpCellLayer;
extern CEnvDoc *gpDoc;

// ReclassDlg dialog

IMPLEMENT_DYNAMIC(ReclassDlg, CDialog)

ReclassDlg::ReclassDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ReclassDlg::IDD, pParent)
   , m_fieldName(_T(""))
   , m_reclassPairs(_T(""))
   {

}


ReclassDlg::~ReclassDlg()
{
}


void ReclassDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_LAYER, m_layers);
DDX_Control(pDX, IDC_FIELD, m_fields);
//DDX_CBString(pDX, IDC_FIELD, m_fieldName);
DDX_Text(pDX, IDC_RECLASSVALUES, m_reclassPairs);
   }


BEGIN_MESSAGE_MAP(ReclassDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_LAYER, &ReclassDlg::OnCbnSelchangeLayer)
   ON_CBN_SELCHANGE(IDC_FIELD, &ReclassDlg::OnCbnSelchangeField)
   ON_BN_CLICKED(IDC_EXTRACT, &ReclassDlg::OnBnClickedExtract)
   ON_BN_CLICKED(IDC_LOADFROMFILE, &ReclassDlg::OnBnClickedLoadfromfile)
   ON_EN_CHANGE(IDC_RECLASSVALUES, &ReclassDlg::OnEnChangeReclassvalues)
END_MESSAGE_MAP()


// ReclassDlg message handlers


void ReclassDlg::OnCbnSelchangeLayer()
   {
   int layer = m_layers.GetCurSel();
   m_pLayer = gpCellLayer->m_pMap->GetLayer( layer );

   LoadFieldInfo();
   OnCbnSelchangeField();
   }


void ReclassDlg::OnCbnSelchangeField()
   {
   m_reclassPairs.Empty();
   m_fields.GetWindowText( m_fieldName );
   }


void ReclassDlg::OnBnClickedExtract()
   {
   CWaitCursor c;

   m_fields.GetWindowText( m_fieldName );

   m_reclassPairs.Empty();
   //m_pairs.SetWindowText( "" );

   int col = m_pLayer->GetFieldCol( m_fieldName );
   ASSERT( col >= 0 );
   
   CStringArray values;
   m_pLayer->GetUniqueValues( col, values, -1 );

   //CString pairs;
   for ( int i=0; i < (int) values.GetCount(); i++ )
      {
      m_reclassPairs += values[ i ];
      m_reclassPairs += ",";

      if ( i < (int) values.GetCount()-1 )
         m_reclassPairs += "\r\n";
      }

   //m_pairs.SetWindowText( pairs );
   UpdateData( 0 );
   }


BOOL ReclassDlg::OnInitDialog()
   {
   CDialog::OnInitDialog();

    // load layers combo
   Map *pMap = gpCellLayer->m_pMap;

   int layerCount = pMap->GetLayerCount();
   for ( int i=0; i < layerCount; i++ )
      {
      MapLayer *pLayer = pMap->GetLayer( i );
      m_layers.AddString( pLayer->m_name );
      }

   if ( layerCount > 0 )
      {
      m_layers.SetCurSel( 0 );
      m_pLayer = pMap->GetLayer( 0 );
      }
   else
      {
      m_layers.SetCurSel( -1 );
      m_pLayer = NULL;
      }

   LoadFieldInfo();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
   }



void ReclassDlg::LoadFieldInfo()
   {
   m_fields.ResetContent();

   for ( int i=0; i < m_pLayer->GetFieldCount(); i++ )
      {
      //TYPE type = pLayer->GetFieldType( i );
      m_fields.AddString( m_pLayer->GetFieldLabel( i ) );
      }

   m_fields.SetCurSel( 0 );
   }



void ReclassDlg::OnOK()
   {
   UpdateData( 1 );    // this should update m_reclassPairs to whatever is in the edit control
   m_fields.GetWindowText( m_fieldName );

   int size = m_reclassPairs.GetLength();  // m_pairs.GetWindowTextLength();
   int col = m_pLayer->GetFieldCol( m_fieldName );

   LPTSTR buffer = new TCHAR[ size + 64 ];
   lstrcpy( buffer, m_reclassPairs ); //m_pairs.GetWindowText( buffer, size + 64 );

   LPTSTR next;
   TCHAR *pair = _tcstok_s( buffer, _T("\r\n"), &next );

   CArray< VData, VData& > startValues;
   CArray< VData, VData& > reclassValues;

   while( pair != NULL )
      {
      // next, parse pair;
      LPTSTR comma = _tcschr( pair, ',' );
      if ( comma != NULL )
         {
         *comma = NULL;
         CString startStr( pair );
         CString reclassStr( comma+1 );
         startStr.Trim();
         reclassStr.Trim();

         if ( startStr.GetLength() > 0 && reclassStr.GetLength() > 0 )
            {
            VData startValue, reclassValue;
            startValue.Parse( startStr );
            reclassValue.Parse( reclassStr );
            startValues.Add( startValue );
            reclassValues.Add( reclassValue );
            }
         }
      
      pair = _tcstok_s( NULL, _T( "\r\n" ), &next );
      }

   delete [] buffer;

   CWaitCursor c;
   m_pLayer->Reclass( col, startValues, reclassValues );

   int retVal = MessageBox( "Finished reclassifying field.  Do you want to save the database?", "Save Database", MB_YESNO );

   if ( retVal == IDYES )
      {
      DirPlaceholder d;

      CFileDialog dlg( FALSE, "dbf", m_pLayer->m_tableName , OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            "DBase Files|*.dbf|All files|*.*||");

      if ( dlg.DoModal() == IDOK )
         {
         CString filename( dlg.GetPathName() );

         bool useWide = VData::SetUseWideChar( true );

         m_pLayer->SaveDataDB( filename );

         VData::SetUseWideChar( useWide );
 
         gpDoc->SetChanged( 0 );
         }
      else
         gpDoc->SetChanged( CHANGED_COVERAGE );
      }
   }



void ReclassDlg::OnBnClickedLoadfromfile()
   {
   DirPlaceholder d;
   m_reclassPairs.Empty();

   CFileDialog dlg( TRUE, ".csv", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
               "CSV files|*.csv|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );

      FILE *fp = NULL;
      PCTSTR file = (PCTSTR)filename;
      fopen_s( &fp, file, "rt" );

      int count = 0;
      if ( fp != NULL )
         {
         TCHAR start[ 256 ], end[ 256 ];
         while ( fscanf_s( fp, "%[0-9],%[0-9]\n", start, 255, end, 255 ) == 2 )
            {
            m_reclassPairs += start;
            m_reclassPairs += ",";
            m_reclassPairs += end;
            m_reclassPairs += "\r\n";
            
            count++;            
            }

         fclose( fp );
         UpdateData( 0 );
         }

      CString msg;
      msg.Format( "Reclass: Loaded %i reclass pairs from %s", count, (LPCTSTR) filename );
      Report::OKMsg( msg );
      }
   }


void ReclassDlg::OnEnChangeReclassvalues()
   {
   m_reclassPairs.Empty();

   CEdit *pEdit = (CEdit*) GetDlgItem( IDC_RECLASSVALUES );
   int count = 0;
   int length = pEdit->GetWindowTextLength();
   
   TCHAR *buffer = new TCHAR[ length+2 ]; 
   pEdit->GetWindowText( buffer, length+1 );

   TCHAR start[ 256 ], end[ 256 ];
   while ( scanf_s( buffer, "%[0-9],%[0-9]\n", start, 255, end, 255 ) == 2 )
      {
      m_reclassPairs += start;
      m_reclassPairs += ",";
      m_reclassPairs += end;
      m_reclassPairs += "\r\n";
      
      count++;            
      }

   delete [] buffer;
   //CString msg;
   //msg.Format( "Reclass: %i reclass pairs );
   //Report::OKMsg( msg );
   }

