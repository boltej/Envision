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
// Report.cpp : implementation file
//

#include "EnvLibs.h"
#pragma hdrstop

#include "Report.h"
#include <string>



//int Report::reportFlag = RF_MESSAGEBOX | RF_CALLBACK;
int Report::indentLevel = 0;
LOGMSG_PROC Report::logMsgProc = NULL;
STATUSMSG_PROC Report::statusMsgProc = NULL;
POPUPMSG_PROC Report::popupMsgProc = NULL;

FILE *Report::m_pFile = NULL;

void Report::StatusMsg(LPCTSTR msg)
   {
   if (statusMsgProc != NULL)
      statusMsgProc(msg);
   return;
   }


int Report::LogMsg(LPCTSTR _msg, REPORT_ACTION action, REPORT_TYPE type)
   {
   if (logMsgProc)
      {
      std::string msg = "";
      if (indentLevel > 0)
         {
         msg = "";
         for (int i = 0; i < indentLevel; i++)
            msg += "  ";
         }
      msg += _msg;

      logMsgProc(_msg, action, type);
      }

   if (m_pFile != NULL)
      {
      std::string msg;

      switch (type)
         {
         case RT_INFO:
            msg = _T("Info: ");
            msg += _msg;
            break;

         case RT_WARNING:
            msg = _T("Warning: ");
            msg += _msg;
            break;

         case RT_ERROR:
            msg = _T("Error: ");
            msg += _msg;
            break;

         case RT_FATAL:
            msg = _T("Fatal: ");
            msg += _msg;
            break;

         case RT_SYSTEM:
            msg = _T("System Error: ");
            msg += _msg;
            break;

         case RT_UNDEFINED:
         default:
            msg = _msg;
         }

      WriteFile(msg.c_str());

      return 0;
      }

   return -1;
   }


int Report::PopupMsg(LPCTSTR _msg, LPCTSTR hdr, REPORT_TYPE type, int flags, int extra )
   {
   if (popupMsgProc)
      {
      std::string msg;

      switch (type)
         {
         case RT_INFO:
            msg = _T("Info: ");
            msg += _msg;
            break;

         case RT_WARNING:
            msg = _T("Warning: ");
            msg += _msg;
            break;

         case RT_ERROR:
            msg = _T("Error: ");
            msg += _msg;
            break;

         case RT_FATAL:
            msg = _T("Fatal: ");
            msg += _msg;
            break;

         case RT_SYSTEM:
            msg = _T("System Error: ");
            msg += _msg;
            break;

         default:
            msg = _msg;
         }

      //         ReportBox dlg;
      //         dlg.m_msg = msg;
      //         dlg.m_hdr = hdr;
      //         dlg.m_noShow = 0;
      //         dlg.m_output = ( reportFlag & RF_CALLBACK ) ? 1 : 0;
      //         dlg.m_flags = flags;
      //         retVal = (int) dlg.DoModal();
      //
      //         reportFlag = 0;
      //         if ( ! dlg.m_noShow )
      //            reportFlag += RF_MESSAGEBOX;
      //
      //         if ( dlg.m_output )
      //            reportFlag += RF_CALLBACK;
             // retVal = AfxGetMainWnd()->MessageBox( msg, hdr, flags );

      CString fullMsg = msg.c_str();

      if (hdr == NULL)
         hdr = "Message";

      CString title(hdr);
      title += ": ";

      title.Replace("%", "percent");  // special formating character
      fullMsg.Replace("%", "percent");

      popupMsgProc((LPCTSTR) fullMsg, (LPCTSTR)title, type, flags, extra );
      }
   
   if ( m_pFile != NULL )
      WriteFile( _msg );

   return 0;
   }


/*
int Report::OKMsg( LPCTSTR msg, LPCTSTR hdr )
   {
   CString title = ( hdr == NULL ) ? "Success" : hdr;
   MessageBox( GetActiveWindow(), msg, title, MB_OK | MB_ICONEXCLAMATION );

   return 0;
   }
   

 int Report::YesNoMsg( LPCTSTR msg,  LPCTSTR hdr)
   {
   int retVal = 0;
   CString title = ( hdr == NULL ) ? "Response Requested" : hdr;
   retVal = MessageBox( GetActiveWindow(), msg, title, MB_YESNO | MB_ICONQUESTION );

   return retVal;
    }



int Report::BalloonMsg( LPCTSTR msg, REPORT_TYPE type, int duration)
   {
   int retVal = 0;

   CString hdr;

   switch ( type )
      {
      case RT_INFO:    hdr = _T("Information Message");  break;
      case RT_WARNING: hdr = _T("Warning! ");     break;
      case RT_ERROR:   hdr = _T("Error! ");       break;
      case RT_FATAL:   hdr = _T("Fatal Error! "); break;
      case RT_SYSTEM:  hdr = _T("System Error! ");break;
      }

   CWnd *pMain = ::AfxGetMainWnd();
   CRect rect;
   pMain->GetClientRect( &rect );
      
   //CBalloonHelp::LaunchBalloon( hdr, msg, CPoint( 400, rect.bottom-40 ), IDI_WARNING, 
   //            CBalloonHelp::unCLOSE_ON_ANYTHING | CBalloonHelp::unSHOW_CLOSE_BUTTON,
   //            pMain, "", duration );

   CMFCDesktopAlertWnd *pPopup = new CMFCDesktopAlertWnd;
   pPopup->SetAutoCloseTime( duration );
   pPopup->SetAnimationType( CMFCPopupMenu::SLIDE );
   pPopup->SetSmallCaption( 0 );
   //pPopup->SetTransparency( 100 );
      
   CMFCDesktopAlertWndInfo params;

   switch( type )
      {
      case RT_INFO:    params.m_hIcon = LoadIcon( NULL, IDI_INFORMATION );        break;
      case RT_WARNING: params.m_hIcon = LoadIcon( NULL, IDI_WARNING );            break;
      case RT_ERROR:   
      case RT_FATAL:   
      case RT_SYSTEM:  params.m_hIcon = LoadIcon( NULL, IDI_ERROR );              break;
      default: params.m_hIcon = NULL;
      }

   params.m_strText = msg;
   params.m_strURL = _T( "" );
   //params.m_nURLCmdID = 101;   
   CPoint pos;    // screen coords
   pos.x = 400;
   pos.y = rect.bottom-40;
   pPopup->Create( pMain, params, NULL, pos );
   pPopup->SetWindowText( hdr );     

   return retVal;
   }


int Report::SystemErrorMsg  ( LONG s_err, LPCTSTR __msg, LPCTSTR hdr, int flags)
   {
   int retVal = 0;

   CString title = ( hdr == NULL ) ? "Error" : hdr;

   LPVOID lpMsgBuf;
   FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, s_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf,  0,  NULL  );   

   CString msg = ( __msg == NULL ) ? "" : __msg ;
   msg += (LPTSTR) lpMsgBuf;
   LocalFree( lpMsgBuf );

   retVal = MessageBox( GetActiveWindow(), msg, title, flags | MB_ICONEXCLAMATION );

   return retVal;
   }

   */

int Report::OpenFile( LPCTSTR filename )
   {
   if ( m_pFile )
      CloseFile();

   int retVal = fopen_s( &m_pFile, filename, "wt");
   if ( retVal != 0 )
      {
      CString msg( "Report: Unable to open file " );
      msg += filename;
      Report::ErrorMsg( msg );
      }
      
   return retVal;
   }


int Report::CloseFile( void )
   {
   if ( m_pFile != NULL )
      {
      fclose( m_pFile );
      m_pFile = NULL;
      }

   return 0;
   }


int Report::WriteFile( LPCTSTR line )
   {
   if ( m_pFile == NULL )
      return -1;

   fputs( line, m_pFile );    // no newline, must add
   fputc( '\n', m_pFile );

   return 0;
   }
 



/*

int Report::reportFlag = RF_MESSAGEBOX | RF_CALLBACK;

REPORTMSG_PROC Report::reportMsgProc = NULL;
STATUSMSG_PROC Report::statusMsgProc = NULL;
MSGWND_PROC Report::msgWndProc = NULL;

FILE *Report::m_pFile = NULL;


int Report::Msg(LPCTSTR msg, int errorType, LPCTSTR hdr, int flags)
   {
   int retVal = 0;

   if (reportFlag & RF_MESSAGEBOX && msgWndProc != NULL)
      {
      if (hdr == NULL)
         hdr = "Report";

      int retVal = msgWndProc(hdr, msg, flags);
      }

   //#ifdef _WINDOWS
   //      ReportBox dlg;
   //      dlg.m_msg = msg;
   //      dlg.m_hdr = hdr;
   //      dlg.m_noShow = 0;
   //      dlg.m_output = ( reportFlag & RF_CALLBACK ) ? 1 : 0;
   //      dlg.m_flags = flags;
   //      retVal = (int) dlg.DoModal();
   //
   //      reportFlag = 0;
   //      if ( ! dlg.m_noShow )
   //         reportFlag += RF_MESSAGEBOX;
   //
   //      if ( dlg.m_output )
   //         reportFlag += RF_CALLBACK;
   //#endif
       // retVal = AfxGetMainWnd()->MessageBox( msg, hdr, flags );

   CString _msg = msg;

   if (reportFlag & RF_CALLBACK && reportMsgProc != NULL)
      {
      if (hdr == NULL)
         hdr = "Message";

      CString title(hdr);
      title += ": ";

      title.Replace("%", "percent");  // special formating character
      _msg.Replace("%", "percent");
      reportMsgProc(_msg, title, errorType);
      }

   if (m_pFile != NULL)
      WriteFile(_msg);

   return retVal;
   }



int Report::OKMsg(LPCTSTR msg, LPCTSTR hdr)
   {
#ifdef _WINDOWS   
   CString title = (hdr == NULL) ? "Success" : hdr;
   MessageBox(GetActiveWindow(), msg, title, MB_OK | MB_ICONEXCLAMATION);
#endif

   return 0;
   }


int Report::YesNoMsg(LPCTSTR msg, LPCTSTR hdr)
   {
   int retVal = 0;
#ifdef _WINDOWS   
   CString title = (hdr == NULL) ? "Response Requested" : hdr;
   retVal = MessageBox(GetActiveWindow(), msg, title, MB_YESNO | MB_ICONQUESTION);
#endif

   return retVal;
   }


int Report::LogMsg(LPCTSTR msg, int type)
   {
   int oldReportFlag = reportFlag;
   reportFlag = RF_CALLBACK;
   int retVal = 0;
   switch (type)
      {
      case RT_INFO:    retVal = Report::InfoMsg(msg);     break;
      case RT_WARNING: retVal = Report::WarningMsg(msg);  break;
      case RT_ERROR:   retVal = Report::ErrorMsg(msg);    break;
      case RT_FATAL:   retVal = Report::FatalMsg(msg);    break;
      case RT_SYSTEM:  retVal = Report::SystemMsg(msg);   break;
      }

   reportFlag = oldReportFlag;
   return retVal;
   }


int Report::BalloonMsg(LPCTSTR msg, int type, int duration)
   {
   int retVal = 0;

#ifdef _WINDOWS
   CString hdr;

   switch (type)
      {
      case RT_INFO:    hdr = _T("Information Message");  break;
      case RT_WARNING: hdr = _T("Warning! ");     break;
      case RT_ERROR:   hdr = _T("Error! ");       break;
      case RT_FATAL:   hdr = _T("Fatal Error! "); break;
      case RT_SYSTEM:  hdr = _T("System Error! "); break;
      }

   CWnd *pMain = ::AfxGetMainWnd();
   CRect rect;
   pMain->GetClientRect(&rect);

   //CBalloonHelp::LaunchBalloon( hdr, msg, CPoint( 400, rect.bottom-40 ), IDI_WARNING, 
   //            CBalloonHelp::unCLOSE_ON_ANYTHING | CBalloonHelp::unSHOW_CLOSE_BUTTON,
   //            pMain, "", duration );

   CMFCDesktopAlertWnd *pPopup = new CMFCDesktopAlertWnd;
   pPopup->SetAutoCloseTime(duration);
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

#endif
   return retVal;
   }


void Report::StatusMsg(LPCTSTR msg)
   {
   if (statusMsgProc != NULL)
      statusMsgProc(msg);
   return;
   }


int Report::SystemErrorMsg(LONG s_err, LPCTSTR __msg, LPCTSTR hdr, int flags)
   {
   int retVal = 0;

#ifdef _WINDOWS
   CString title = (hdr == NULL) ? "Error" : hdr;

   LPVOID lpMsgBuf;
   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, s_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

   CString msg = (__msg == NULL) ? "" : __msg;
   msg += (LPTSTR)lpMsgBuf;
   LocalFree(lpMsgBuf);

   retVal = MessageBox(GetActiveWindow(), msg, title, flags | MB_ICONEXCLAMATION);
#endif

   return retVal;
   }



*/