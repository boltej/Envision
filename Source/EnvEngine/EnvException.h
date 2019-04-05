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

#include "EnvAPI.h"

///* Usage:  By design, use THROW, TRY, CATCH.  
//           But Standard C++ usage is, e.g.:
//               throw EnvException();
//               catch (EnvException & e) { /* do someting */ }
//*/

#define ENV_ASSERT(f) \
   if ( !(f) )\
      { \
      ASSERT(0); \
      CString str;\
      str.Format( "FAILED ASSERT(%s):\n  File: %s\n  Line: %d", #f, __FILE__, __LINE__ ); \
      throw new EnvFatalException( str ); \
      }

class ENVAPI EnvException : public CException
   {
#ifndef NO_MFC
   DECLARE_DYNAMIC(EnvException)
#endif
   public:
      EnvException();
      EnvException( LPCTSTR errorStr );
      virtual ~EnvException();

   private:
      CString m_errorStr;

   public:
      virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, PUINT pnHelpContext = NULL);
      virtual const char * what() const;
   };

class EnvRuntimeException : public EnvException
   {
   public:
      EnvRuntimeException() : EnvException( "Unspecified Runtime Error" ) {}
      EnvRuntimeException( LPCTSTR errorStr ) : EnvException( errorStr ) {}
   };

class EnvFatalException : public EnvException
   {
   public:
      EnvFatalException() : EnvException( "Unspecified Fatal Error" ) {}
      EnvFatalException( LPCTSTR errorStr ) : EnvException( errorStr ) {}
   };
