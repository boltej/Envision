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
#include "stdafx.h"
#pragma hdrstop

#include "envmsg.h"
#include "envview.h"
#include "envdoc.h"
#include "mainfrm.h"
#include <colors.hpp>
#include <Report.h>
#include <EnvModel.h>
#include <ReportDlg.h>

extern CEnvView   *gpView;
extern CEnvDoc    *gpDoc;
extern CMainFrame *gpMain;
extern EnvModel   *gpModel;

// called whenever a Report message is invoked (e.g. Report::Log();
void LogMsgProc( LPCTSTR msg, REPORT_ACTION action, REPORT_TYPE type )
   {
   COLORREF color;

   switch( type )
      {
      case RT_WARNING: 
         color = BLUE;
         gpDoc->m_warnings++;

         if ( gpModel->m_logMsgLevel > 0 && ( ! (gpModel->m_logMsgLevel & ERL_WARNINGS ) ) )
            return;
         break;

      case RT_ERROR:
      case RT_FATAL:
      case RT_SYSTEM:
         color = RED;
         gpDoc->m_errors++;
         if ( gpModel->m_logMsgLevel > 0 && ( ! (gpModel->m_logMsgLevel & ERL_ERRORS ) ) )
            return;
         break;

      default:
         color = BLACK;
         gpDoc->m_infos++;

         if ( gpModel->m_logMsgLevel > 0 && ( ! (gpModel->m_logMsgLevel & ERL_INFOS ) ) )
            return;

         break;
      }

   gpMain->AddLogLine( msg, (LOGBOOK_TEXTTYPES)type, color );
   }


void StatusMsgProc( LPCTSTR msg )
   {
   gpMain->SetStatusMsg( msg );
   }


// called whenever a Report message is invoked (e.g. Report::Log();
int PopupMsgProc(  LPCTSTR hdr, LPCTSTR msg, int flags )
   {
   ReportBox dlg;
   dlg.m_msg = msg;
   dlg.m_hdr = hdr;
   dlg.m_noShow = 0;
   dlg.m_flags = flags;
   int retVal = (int) dlg.DoModal();

   retVal = AfxGetMainWnd()->MessageBox( msg, hdr, flags );
   
   return retVal;
   }


