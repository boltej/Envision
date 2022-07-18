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

#include "Path.h"
#include <map>
#include <string>

using namespace nsPath;
using namespace std;


class LIBSAPI PathManager
   {
   public:
      PathManager(void);
      ~PathManager(void);

   protected:
      //static CStringArray m_pathArray;
      static std::map<int, std::string> m_pathMap;

      static int SearchPaths( CPath &path, FILE **fp, LPCTSTR mode ); // Last In, First out

   public:
      static LPCTSTR GetPath(int i);    // this will always be terminated with a '\'
      static int AddPath( LPCTSTR path, int id=-9999 );
      static int SetPath( int index, LPCTSTR path );
      static int FindPath( LPCTSTR path, CString &fullPath );        // finds first full path that contains passed in path if relative or filename, return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 

      static int FileOpen( LPCTSTR path, FILE **fp, LPCTSTR mode );
   };


