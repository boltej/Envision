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
//#if defined _MFC
#include "EnvWinLibs.h"
//#else
//#include <windowsb.h>
//#endif

#pragma hdrstop


#include <sysinit.hpp>
#include <colors.hpp>


//-------------------- system-wide constants ----------------------//

HMODULE ghSysResDLL = 0;

COLORREF gColorRef[ 16 ] =
   {
   BLACK, BLUE,   GREEN,   CYAN,   RED,   MAGENTA,   BROWN,  LTGRAY, \
   GRAY,  LTBLUE, LTGREEN, LTCYAN, LTRED, LTMAGENTA, YELLOW, WHITE
   };

#if !defined _MFC

HINSTANCE ghInst = 0;


//----------------------- Library initialization ------------------//

BOOL InitSysLibApp( void )
   {
   return TRUE;
   }


BOOL InitSysLibInst( HINSTANCE hInst )
   {
   //-- load Res DLL --//
   ghSysResDLL = LoadLibrary( (LPSTR) "SYSRES.DLL" );

   if ( (UINT_PTR) ghSysResDLL < 32 )    // error occurred?
      {
      MessageBox( NULL, "Couldn't find SYSRES.DLL", "Warning", MB_OK );
      ghSysResDLL = (HMODULE) 0;
      }

   ghInst = hInst;

   return TRUE;
   }




//---------------------- Library shutdown -------------------------//

BOOL CloseSysLib( void )
   {
   if ( ghSysResDLL )
      FreeLibrary( ghSysResDLL );

   return TRUE;
   }


#endif //!defined _MFC
