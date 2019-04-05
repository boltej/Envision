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
#include "EnvException.h"

#pragma warning(suppress: 6031)

#ifndef NO_MFC
IMPLEMENT_DYNAMIC(EnvException, CException)
#endif

EnvException::EnvException() 
:  m_errorStr( "Unspecified Error" )
   {
   }

EnvException::EnvException( LPCTSTR errorStr )
:  m_errorStr( errorStr ) 
   {
   }

EnvException::~EnvException()
   {
   }

const char * EnvException::what() const 
   { 
     return (const char*)m_errorStr.GetString() ; 
   }

BOOL EnvException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, PUINT /*pnHelpContext*/)
   {
#ifndef NO_MFC
   ASSERT(lpszError != NULL && AfxIsValidString(lpszError, nMaxError));
#endif
   if (lpszError == NULL || nMaxError == 0) 
      {
      return FALSE;
      }

   lstrcpyn(lpszError, m_errorStr, nMaxError);

   return TRUE;
   }
