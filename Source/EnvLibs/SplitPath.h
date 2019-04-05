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

// classe permettant de splitter un nom de fichier
// en composants (semblable à _splitpath)


class LIBSAPI CSplitPath
{
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];

public:
  CSplitPath (const char *path) ;
  CSplitPath () ;
  ~CSplitPath () ;
  
  const char *getdrive () ;
  const char *getdir () ;
  const char *getname () ;  
  const char *getextension () ;
  
  const char *getpath () ;
} ;

