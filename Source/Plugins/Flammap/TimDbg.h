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


#define TIMDBG

#ifdef TIMDBG

#define TimInfoBox(TimDbgStr) \
  {CString TimDbgMsg; \
  TimDbgMsg.Format(_T("Function: %s: \nFile:    %s\nLine:     %d\n\n%s"), \
  __FUNCTION__,__FILE__,__LINE__,TimDbgStr); \
  MessageBox(NULL, TimDbgMsg.GetString(),"Envision",MB_ICONINFORMATION);}

#else

#define ErrorBox(TimDbgStr) ;

#endif

#define TimErrorBox(TimDbgStr) \
  {CString TimDbgMsg; \
  TimDbgMsg.Format(_T("Function: %s: \nFile:    %s\nLine:     %d\n\n%s"), \
  __FUNCTION__,__FILE__,__LINE__,TimDbgStr); \
  MessageBox(NULL, TimDbgMsg.GetString(),"Envision",MB_ICONHAND);} \
  ASSERT(0);
