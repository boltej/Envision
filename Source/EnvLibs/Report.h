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
#pragma once

#include "EnvLibs.h"


// Report is a static class for handling message reporting, generally accomplished 
// using callback to the application.  Applications typically defined handlers
// for handling the callback messages. Type of message:
// Status Messages
//   - StatusInfo
// Log Messages
//   - LogInfo
//   - LogWarning
//   - LogError
//   - LogMsg (private)
// Popup Messages
//   - YesNoMsg
//   - WarningMsg
//   - ErrorMsg
//   - SystemMsg
// Each class of messages has it's own callback

 // report flags
enum REPORT_FLAG
   {
   RF_LOGMSG = 1,    // msgs reported to the callback funtion (if defined)
   RF_STATUSMSG = 2,
   RF_POPUPMSG = 3    // msgs displayed in a MessageBox
   };

// reporting types
enum REPORT_TYPE
   {
   RT_UNDEFINED = 0,
   RT_INFO    = 1,  // general info msgs
   RT_WARNING = 2,  // warning msgs
   RT_ERROR   = 3,  // error msgs
   RT_FATAL   = 4,  // fatal error msgs
   RT_SYSTEM  = 5   // system error msgs
   };


// action types (log only)
enum REPORT_ACTION
   {
   RA_NEWLINE   = 1,
   RA_APPEND    = 2,
   RA_OVERWRITE = 3
   };

typedef void (*LOGMSG_PROC   )( LPCTSTR msg,  REPORT_ACTION action, REPORT_TYPE type );
typedef void (*STATUSMSG_PROC)( LPCTSTR msg );
typedef int  (*POPUPMSG_PROC )( LPCTSTR header, LPCTSTR msg, int flags );


class LIBSAPI Report
{
public:
   static int            reportFlag; 

   // callbacks for each message type
   static LOGMSG_PROC    logMsgProc;
   static STATUSMSG_PROC statusMsgProc;
   static POPUPMSG_PROC  popupMsgProc;
   static FILE          *m_pFile;


   //   - StatusInfo
   // Log Messages
   //   - LogInfo
   //   - LogWarning
   //   - LogError
   //   - LogMsg (private)
   // Popup Messages
   //   - YesNoMsg
   //   - OkMsg
   //   - InfoMsg
   //   - WarningMsg
   //   - ErrorMsg
   //   - SystemMsg
   //   - FatalMsg
private:
   // these do most to the work
   static int LogMsg     (LPCTSTR msg, REPORT_ACTION action, REPORT_TYPE type );
   static int PopupMsg   (LPCTSTR msg, REPORT_TYPE type, LPCTSTR hdr = _T("\0"), int flags = MB_OK);

public:
   // status messages
   static void StatusMsg (LPCTSTR msg);

   // log messages
   static int  Log           (LPCTSTR msg, REPORT_ACTION action=RA_NEWLINE ) { return LogMsg(msg, action, RT_INFO); }
   static int  LogInfo       (LPCTSTR msg, REPORT_ACTION action=RA_NEWLINE ) { return LogMsg(msg, action, RT_INFO); }
   static int  LogWarning    (LPCTSTR msg, REPORT_ACTION action=RA_NEWLINE ) { return LogMsg(msg, action, RT_WARNING); }
   static int  LogError      (LPCTSTR msg, REPORT_ACTION action=RA_NEWLINE ) { return LogMsg(msg, action, RT_ERROR); }
   static int  LogFatalError (LPCTSTR msg, REPORT_ACTION action=RA_NEWLINE ) { return LogMsg(msg, action, RT_FATAL); }
   static int  LogSystemError(LPCTSTR msg, REPORT_ACTION action=RA_NEWLINE ) { return LogMsg(msg, action, RT_SYSTEM); }

   // popup messages
   // these have default implementations on Windows, undefined on *nix
   static int  YesNoMsg      (LPCTSTR msg, LPCTSTR hdr = _T("\0"));
   static int  OKMsg         (LPCTSTR msg, LPCTSTR hdr = _T("\0"));
   static int  BalloonMsg    (LPCTSTR msg, REPORT_TYPE type=RT_INFO, int duration=5000);
   static int  InfoMsg       (LPCTSTR msg, LPCTSTR hdr = _T("Info"), int flags = MB_OK) { return PopupMsg(msg, RT_INFO, hdr, flags); }
   static int  WarningMsg    (LPCTSTR msg, LPCTSTR hdr = _T("Warning"), int flags = MB_OK) { return PopupMsg(msg, RT_WARNING, hdr, flags); }
   static int  FatalMsg      (LPCTSTR msg, LPCTSTR hdr = _T("Fatal Error"), int flags = MB_OK) { return PopupMsg(msg, RT_FATAL, hdr, flags); }
   static int  ErrorMsg      (LPCTSTR msg, LPCTSTR hdr = _T("Error"), int flags = MB_OK) { return PopupMsg(msg, RT_ERROR, hdr, flags); }
   static int  SystemErrorMsg( LONG s_err, LPCTSTR __msg=_T( "\0"),  LPCTSTR hdr=_T("\0"), int flags=MB_OK );
 
   static int OpenFile( LPCTSTR filename );
   static int CloseFile( void );
   static int WriteFile( LPCTSTR );

protected:
};
