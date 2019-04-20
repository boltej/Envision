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


#include <Report.h>

void LogMsgProc( LPCTSTR msg, REPORT_ACTION, REPORT_TYPE );
void StatusMsgProc( LPCTSTR msg );
int PopupMsgProc(LPCTSTR msg, LPCTSTR hdr, REPORT_TYPE type, int flags, int extra);
void EnvSetLLMapTextProc(LPCTSTR text);
void EnvRedrawMapProc();


enum ENV_REPORTING_LEVEL
   {
   ERL_ERRORS   = 1,
   ERL_WARNINGS = 2,
   ERL_INFOS    = 4,
   ERL_ALL      = 7
   };


//int EnvWarningMsg( LPCTSTR msg, int flags=MB_OK );
//int EnvErrorMsg( LPCTSTR msg, int flags=MB_OK );
//int EnvInfoMsg( LPCTSTR msg, int flags=MB_OK );
//void EnvMsg( LPCTSTR msg );
