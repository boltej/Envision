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
// DatabaseInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DatabaseInfoDlg.h"

#include <maplayer.h>
#include <path.h>
#include <direct.h>
#include <fstream>

using namespace std;



// DatabaseInfoDlg dialog

IMPLEMENT_DYNAMIC(DatabaseInfoDlg, CDialog)

DatabaseInfoDlg::DatabaseInfoDlg(MapLayer *pLayer, CWnd* pParent /*=NULL*/)
	: CDialog(DatabaseInfoDlg::IDD, pParent)
   , m_path(_T(""))
   , m_name(_T(""))
   , m_pLayer( pLayer )
   {

}

DatabaseInfoDlg::~DatabaseInfoDlg()
{
}

void DatabaseInfoDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FIELDS, m_fields);
DDX_Text(pDX, IDC_PATH, m_path);
DDX_Text(pDX, IDC_NAME, m_name);
   }


BEGIN_MESSAGE_MAP(DatabaseInfoDlg, CDialog)
   ON_BN_CLICKED(IDC_SAVE, &DatabaseInfoDlg::OnBnClickedSave)
END_MESSAGE_MAP()


// DatabaseInfoDlg message handlers


BOOL DatabaseInfoDlg::OnInitDialog()
   {
   m_name = m_pLayer->m_name;
   m_path = m_pLayer->m_path;
   
   CDialog::OnInitDialog();

   LoadFields();

   return TRUE;  // return TRUE unless you set the focus to a control
   }


void DatabaseInfoDlg::LoadFields()
   {
   m_fields.DeleteAllItems();

   HTREEITEM hItem = m_fields.InsertItem( m_name );

   for ( int i=0; i < m_pLayer->GetFieldCount(); i++ )
      {
      CString field = m_pLayer->GetFieldLabel( i );
      HTREEITEM hField = m_fields.InsertItem( field, hItem );

      // add basic information
      TYPE type = m_pLayer->GetFieldType( i );
      LPCTSTR typeStr = GetTypeLabel( type );
      m_fields.InsertItem( typeStr, hField );

      MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( field );
      if ( pInfo != NULL )
         {
         CString desc( "Label: " );
         desc += pInfo->label;
         m_fields.InsertItem( desc, hField );

         if ( pInfo->GetAttributeCount() > 0 )
            {
            HTREEITEM hAttrs = m_fields.InsertItem( "Attributes", hField );

            for ( int j=0; j < pInfo->GetAttributeCount(); j++ )
               {
               FIELD_ATTR &fa = pInfo->GetAttribute( j );
   
               HTREEITEM hAttr = m_fields.InsertItem( fa.label, hAttrs );

               if ( pInfo->mfiType == MFIT_QUANTITYBINS )
                  {                  
                  CString minVal, maxVal;
                  minVal.Format( "Min: %g", fa.minVal );
                  maxVal.Format( "Max: %g", fa.maxVal );

                  m_fields.InsertItem( minVal, hAttr );
                  m_fields.InsertItem( maxVal, hAttr );
                  }
               else
                  {
                  CString value = "Value: ";
                  value += fa.value.GetAsString();   
                  m_fields.InsertItem( value, hAttr );
                  }
               }
            }
         }
      }

   m_fields.Expand( hItem, TVE_EXPAND );
   }


void DatabaseInfoDlg::OnBnClickedSave()
   {
   char *cwd = _getcwd( NULL, 0 );

   nsPath::CPath path( m_path );
   path.RenameExtension( "txt" );

   CFileDialog dlg( FALSE, "txt", path, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
         "Text Files|*.txt|All files|*.*||");

   if ( dlg.DoModal() == IDOK )
      {
      CString filename( dlg.GetPathName() );

      // open writing stream
      ofstream file;
      file.open( filename );

      file << "Name: " << m_name << endl;
      file << "Path: " << m_path << endl << endl;

      for ( int i=0; i < m_pLayer->GetFieldCount(); i++ )
         {
         CString field = m_pLayer->GetFieldLabel( i );
         TYPE type = m_pLayer->GetFieldType( i );
         LPCTSTR typeStr = GetTypeLabel( type );

         file << endl << "FIELD: " << field << endl;
         file << "\tType: " << typeStr << endl;

         MAP_FIELD_INFO *pInfo = m_pLayer->FindFieldInfo( field );
         if ( pInfo != NULL )
            {
            file << "\tLabel: " << pInfo->label << endl;

            if ( pInfo->GetAttributeCount() > 0 )
               {
               file << "\tAttributes" << endl;
               
               for ( int j=0; j < pInfo->GetAttributeCount(); j++ )
                  {
                  FIELD_ATTR &fa = pInfo->GetAttribute( j );
                  file << "\t\tLabel: " << fa.label << endl;

                  if ( pInfo->mfiType == MFIT_QUANTITYBINS )
                     {                  
                     file << "\t\t\tMin: " << fa.minVal << endl;
                     file << "\t\t\tMax: " << fa.maxVal << endl;
                     }
                  else
                     file << "\t\t\tValue: " << fa.value.GetAsString() << endl;
                  }
               }
            }
         }
      file.close();
      }

   _chdir( cwd );
   free( cwd );
   }
