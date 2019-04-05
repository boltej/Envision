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
//====================================================================
//  ERRORMSG.CPP
//
//  -- Purpose:  Error routines for GUI
//
// Note:  optional logging of messages to an handler
//====================================================================

#include "libs.h"
#pragma hdrstop

//#include <ImageHlp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errormsg.h>



//------------------------ Externs --------------------------------//

int ErrorMsg( LPCTSTR msg, LPCTSTR hdr, int flags /*=MB_OK*/ )
   {
   CString title = ( hdr == NULL ) ? "Error" : hdr;
   return MessageBox( GetActiveWindow(), msg, title, flags | MB_ICONEXCLAMATION );
   }


int ErrorMsgEx  ( LPCTSTR msg,  LPCTSTR hdr/*='\0'*/, int flags/*=MB_OK*/ )
{
   CString title = ( hdr == NULL ) ? "Error" : hdr;
   CString message = "StackDump provided debug:TRACE or release:Clipboard.  ";
   title += message;
   message += msg;
   AfxDumpStack();
   return MessageBox( GetActiveWindow(), message, title, flags | MB_ICONEXCLAMATION );
}


int SystemErrorMsg  ( LONG s_err, LPCTSTR __msg, LPCTSTR hdr, int flags)
   {
   CString title = ( hdr == NULL ) ? "Error" : hdr;

   LPVOID lpMsgBuf;
   FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, s_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf,  0,  NULL  );   

   CString msg = ( __msg == NULL ) ? "" : __msg ;
   msg += (LPTSTR) lpMsgBuf;
   LocalFree( lpMsgBuf );

   return MessageBox( GetActiveWindow(), msg, title, flags | MB_ICONEXCLAMATION );
   }




//-- WarningMsg() ---------------------------------------------------
//
//-- Displays an warning message
//
//-- Arguments:
//      wWarningNumber = string table string id
//-------------------------------------------------------------------

int WarningMsg( LPCTSTR msg, LPCTSTR hdr, int flags /*=MB_OK*/ )
   {
   CString title = ( hdr == NULL ) ? "Warning" : hdr;
   return MessageBox( GetActiveWindow(), msg, title, flags | MB_ICONEXCLAMATION );
   }


int OKMsg( LPCTSTR msg, LPCTSTR hdr )
   {
   CString title = ( hdr == NULL ) ? "Success" : hdr;
   MessageBox( GetActiveWindow(), msg, title, MB_OK | MB_ICONEXCLAMATION );
   return 0;
   }
   
int YesNoMsg  ( LPCTSTR msg,  LPCTSTR hdr)
   {
   CString title = ( hdr == NULL ) ? "Response Requested" : hdr;
   return MessageBox( GetActiveWindow(), msg, title, MB_YESNO | MB_ICONQUESTION );
   }



void WinAssertError( LPCTSTR fileName, int lineNum, LPCTSTR exp )
   {
   CString msg;
   msg.Format("File %s, Line %d\nStatement: %s", fileName,lineNum, exp);
   MessageBox( GetActiveWindow(), msg, "Assertion Error", MB_OK | MB_ICONSTOP );
   }


//////////////////////////// class version /////////////////////////////

int ErrorReport::reportFlag = ERF_MESSAGEBOX | ERF_CALLBACK;

ERRORMSG_PROC ErrorReport::errorMsgProc = NULL;

int ErrorReport::ErrorMsg( LPCTSTR msg,  LPCTSTR hdr, int flags )
   {
   int retVal = 0;

   if ( reportFlag & ERF_MESSAGEBOX )
      retVal = ::ErrorMsg( msg, hdr, flags );

   if ( reportFlag & ERF_CALLBACK && errorMsgProc != NULL )
      errorMsgProc( msg, hdr, ERT_ERROR );

   return retVal;
   }

int ErrorReport::ErrorMsgEx( LPCTSTR msg,  LPCTSTR hdr, int flags )
   {
   int retVal = 0;

   if ( reportFlag & ERF_MESSAGEBOX )
      retVal = ::ErrorMsgEx( msg, hdr, flags );

   if ( reportFlag & ERF_CALLBACK && errorMsgProc != NULL )
      errorMsgProc( msg, hdr, ERT_ERROR );

   return retVal;
   }


int ErrorReport::WarningMsg( LPCTSTR msg,  LPCTSTR hdr, int flags )
   {
   int retVal = 0;

   if ( reportFlag & ERF_MESSAGEBOX )
      retVal = ::WarningMsg( msg, hdr, flags );

   if ( reportFlag & ERF_CALLBACK && errorMsgProc != NULL )
      errorMsgProc( msg, hdr, ERT_WARNING );

   return retVal;
   }

int ErrorReport::InfoMsg( LPCTSTR msg, LPCTSTR hdr, int flags )
   {
   int retVal = 0;

   if ( reportFlag & ERF_MESSAGEBOX )
      {
      if ( hdr == NULL )
         hdr = "Information";

      retVal = AfxGetMainWnd()->MessageBox( msg, hdr, flags );
      }

   if ( reportFlag & ERF_CALLBACK && errorMsgProc != NULL )
      errorMsgProc( msg, "Info: ", ERT_INFO );

   return retVal;
   }

int ErrorReport::FatalMsg( LPCTSTR msg, LPCTSTR hdr, int flags )
   {
   int retVal = 0;

   if ( reportFlag & ERF_MESSAGEBOX )
      {
      if ( hdr == NULL )
         hdr = "Fatal Error";

      retVal = AfxGetMainWnd()->MessageBox( msg, hdr, flags );
      }

   if ( reportFlag & ERF_CALLBACK && errorMsgProc != NULL )
      errorMsgProc( msg, "Fatal: ", ERT_FATAL );

   return retVal;
   }

int ErrorReport::SystemMsg( LPCTSTR msg, LPCTSTR hdr, int flags )
   {
   int retVal = 0;

   if ( reportFlag & ERF_MESSAGEBOX )
      {
      if ( hdr == NULL )
         hdr = "System Message";

      retVal = AfxGetMainWnd()->MessageBox( msg, hdr, flags );
      }

   if ( reportFlag & ERF_CALLBACK && errorMsgProc != NULL )
      errorMsgProc( msg, "System: ", ERT_SYSTEM );

   return retVal;
   }

int ErrorReport::OKMsg( LPCTSTR msg, LPCTSTR hdr ) { return ::OKMsg( msg, hdr ); }

int ErrorReport::YesNoMsg( LPCTSTR msg, LPCTSTR hdr ) { return ::YesNoMsg( msg, hdr ); }

int ErrorReport::LogMsg( LPCTSTR msg, int type )
   {
   int oldReportFlag = reportFlag;
   reportFlag = ERF_CALLBACK;
   int retVal = 0;
   switch ( type )
      {
      case ERT_INFO:    retVal = ErrorReport::InfoMsg( msg ); break;
      case ERT_WARNING: retVal = ErrorReport::WarningMsg( msg );  break;
      case ERT_ERROR:   retVal = ErrorReport::ErrorMsg( msg );    break;
      case ERT_FATAL:   retVal = ErrorReport::FatalMsg( msg );    break;
      case ERT_SYSTEM:  retVal = ErrorReport::SystemMsg( msg );   break;
      }
   
   reportFlag = oldReportFlag;
   return retVal;
   }
