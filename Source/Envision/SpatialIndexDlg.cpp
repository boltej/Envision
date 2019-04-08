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
// SpatialIndexDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "SpatialIndexDlg.h"

#include "MainFrm.h"

#include <maplayer.h>

extern CMainFrame *gpMain;
extern MapLayer *gpCellLayer;


// SpatialIndexDlg dialog

IMPLEMENT_DYNAMIC(SpatialIndexDlg, CDialog)

SpatialIndexDlg::SpatialIndexDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SpatialIndexDlg::IDD, pParent)
   , m_canceled( false )
   , m_inBuild( false )
   , m_maxDistance(500)
   { }


SpatialIndexDlg::~SpatialIndexDlg()
{ }


void SpatialIndexDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_PROGRESS, m_progress);
DDX_Text(pDX, IDC_MAXDISTANCE, m_maxDistance);
DDV_MinMaxInt(pDX, m_maxDistance, 0, 100000);
}


BEGIN_MESSAGE_MAP(SpatialIndexDlg, CDialog)
END_MESSAGE_MAP()


// SpatialIndexDlg message handlers

void SpatialIndexDlg::OnOK()
   {
   UpdateData( 1 );
   m_inBuild = true;
   m_canceled = false;
   gpCellLayer->m_pMap->InstallNotifyHandler(SpatialIndexDlg::MapProc, (LONG_PTR) this );

   gpCellLayer->CreateSpatialIndex( NULL, 10000, (float) m_maxDistance, SIM_NEAREST );
   
   gpCellLayer->m_pMap->RemoveNotifyHandler( SpatialIndexDlg::MapProc, (LONG_PTR) this );

   if ( ! m_canceled )
      {
      MessageBox( _T("Successful creating spatial index"), _T("Success"), MB_OK );
      CDialog::OnOK();
      }
   }


void SpatialIndexDlg::OnCancel()
   {
   if ( m_inBuild )
      {
      int id = MessageBox( _T( "Cancel build?" ), "Cancel?", MB_YESNO );

      if ( id == IDYES )
         {
         m_canceled = true;
         m_inBuild = false;
         }
      }

   else
      CDialog::OnCancel();
   }


   
int SpatialIndexDlg::MapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra )
   {
   SpatialIndexDlg *pDlg = (SpatialIndexDlg*) extra;

   switch( type )
      {
      case NT_BUILDSPATIALINDEX:  // moved to SpatialIndexDlg
         {
         if ( a0 == 0 ) // first time?
            pDlg->m_progress.SetRange32( 0, (int) a1 );

         if ( (a0 % 10 ) == 0 )
            {
            CString msg;
            msg.Format( "Building index for poly %i of %i", (int) a0, (int) a1 );
            gpMain->SetStatusMsg( msg );
            pDlg->m_progress.SetPos( a0 );
            }
         }
         break;
      }  // endof switch:

   if ( pDlg->m_canceled )
      return 0;
   else
      return 1;
   }

