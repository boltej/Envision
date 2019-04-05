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
#include "EnvLibs.h"
#pragma hdrstop

#include "DirPlaceholder.h"

#ifdef NO_MFC
#include<unistd.h>
#endif

DirPlaceholder::DirPlaceholder(void )
   {
#ifndef NO_MFC
   m_cwd = _getcwd( NULL, 0 );
#else
   const unsigned int MAXPATHLEN=2048;
   m_cwd= (TCHAR*)malloc(sizeof(TCHAR)*MAXPATHLEN);
   getcwd(m_cwd,MAXPATHLEN);
#endif
   }

DirPlaceholder::~DirPlaceholder(void)
   {
#ifndef NO_MFC
   _chdir( m_cwd );
#else
   chdir(m_cwd);
#endif
   free( m_cwd );
   }
