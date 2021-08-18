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
extern MapPanel   *gpMapPanel;

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
bool showMsg = true;
bool redirectToLog = false;
int PopupMsgProc(LPCTSTR msg, LPCTSTR hdr, REPORT_TYPE type, int flags, int extra)
   {
   int retVal = 0;

   switch (type)
      {
      case RT_INFO:
      case RT_WARNING:
      case RT_ERROR:
      case RT_FATAL:
         {
         if (showMsg)
            {
            ReportBox dlg;
            dlg.m_msg = msg;
            dlg.m_hdr = hdr;
            dlg.m_noShow = 0;
            //dlg.m_output = ( reportFlag & RF_CALLBACK ) ? 1 : 0;
            dlg.m_flags = flags;
            retVal = (int)dlg.DoModal();

            // suppress in future?
            if (dlg.m_noShow)
               showMsg = false;
            // direct to log window?
            if (dlg.m_output)
               redirectToLog = true;
            }
         if (redirectToLog)
            Report::Log(msg, REPORT_ACTION::RA_NEWLINE, type);
         }
         break;

      case RT_SYSTEM:
         {
         CString title = (hdr == NULL) ? "Error" : hdr;

         LPVOID lpMsgBuf;
         FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, extra, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

         CString _msg = (msg == NULL) ? "" : msg;
         _msg += (LPTSTR)lpMsgBuf;
         LocalFree(lpMsgBuf);

         retVal = MessageBox(GetActiveWindow(), msg, title, flags | MB_ICONEXCLAMATION);
         }
         break;

      case RT_OK:
         {
         CString title = (hdr == NULL) ? "Success" : hdr;
         retVal = MessageBox(GetActiveWindow(), msg, title, MB_OK | MB_ICONEXCLAMATION);
         }
         break;

      case RT_YESNO:
         {
         CString title = (hdr == NULL) ? "Response Requested" : hdr;
         retVal = MessageBox(GetActiveWindow(), msg, title, MB_YESNO | MB_ICONQUESTION);
         }
         break;


      case RT_BALLOON:
         {
         CWnd *pMain = ::AfxGetMainWnd();
         CRect rect;
         pMain->GetClientRect(&rect);

         //CBalloonHelp::LaunchBalloon( hdr, msg, CPoint( 400, rect.bottom-40 ), IDI_WARNING,
         //            CBalloonHelp::unCLOSE_ON_ANYTHING | CBalloonHelp::unSHOW_CLOSE_BUTTON,
         //            pMain, "", duration );

         CMFCDesktopAlertWnd *pPopup = new CMFCDesktopAlertWnd;
         pPopup->SetAutoCloseTime(extra);
         pPopup->SetAnimationType(CMFCPopupMenu::SLIDE);
         pPopup->SetSmallCaption(0);
         //pPopup->SetTransparency( 100 );

         CMFCDesktopAlertWndInfo params;

         switch (type)
            {
            case RT_INFO:    params.m_hIcon = LoadIcon(NULL, IDI_INFORMATION);        break;
            case RT_WARNING: params.m_hIcon = LoadIcon(NULL, IDI_WARNING);            break;
            case RT_ERROR:
            case RT_FATAL:
            case RT_SYSTEM:  params.m_hIcon = LoadIcon(NULL, IDI_ERROR);              break;
            default: params.m_hIcon = NULL;
            }

         params.m_strText = msg;
         params.m_strURL = _T("");
         //params.m_nURLCmdID = 101;
         CPoint pos;    // screen coords
         pos.x = 400;
         pos.y = rect.bottom - 40;
         pPopup->Create(pMain, params, NULL, pos);
         pPopup->SetWindowText(hdr);
         }
         break;
      }
  
   return retVal;
   }


void EnvSetLLMapTextProc(LPCTSTR text)
   {
   gpMapPanel->UpdateLLText(text);
   }


void EnvRedrawMapProc()
   {
   if (gpMapPanel)
      {
      CDC *dc = gpMapPanel->m_pMapWnd->GetDC();
      gpMapPanel->m_pMapWnd->DrawMap(*dc);
      gpMapPanel->m_pMapWnd->ReleaseDC(dc);
   
      // yield control to other processes
      MSG  msg;
      while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
         {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
         }
      }
   }

