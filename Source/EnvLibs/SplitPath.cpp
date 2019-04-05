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

// Simple C++ _splitpath wrapper
                 
#include "EnvLibs.h"                  
#ifndef NO_MFC
#include <io.h>
#else
#include <limits.h>
#include <stdlib.h>
#endif
#include "SplitPath.h"


CSplitPath::CSplitPath (const char *path)
{
#ifndef NO_MFC
  _splitpath_s( path, drive, 3, dir, 256, fname, 256, ext, 256 );
#else
  //may PATH_MAX is a Linux constant
  char fullPath[PATH_MAX];
  
  //this would be a good place to check slashes.

  //grab canonical path
  realpath(path,fullPath);

  //use std::string to parse out pieces
  std::string pathStr(fullPath);
  
  //check for root drive (this will likely be empty or a slash)
  size_t rPos=pathStr.find('/');
  if(rPos!=std::string::npos)
    {
    std::copy_n(pathStr.begin(),rPos,drive);
    drive[rPos]='\0';
    rPos++;
    }
  else
    rPos=0;

  //find trailing slash for directory
  size_t dPos=pathStr.rfind('/');
  if(dPos!=std::string::npos)
    {
    std::copy_n(pathStr.begin()+rPos,dPos-rPos,dir);
    dir[dPos-rPos]='\0';
    dPos++;
    }
  else
    dPos=0;

  //find file name
  size_t fPos=pathStr.rfind('.');
  if(fPos==std::string::npos)
    fPos=pathStr.size()-1;

  std::copy_n(pathStr.begin()+dPos,fPos-dPos,fname);
  fname[fPos-dPos]='\0';

  //copy extension
  if(pathStr[fPos]=='.')
    {
      std::copy_n(pathStr.begin()+fPos,pathStr.size()-fPos,ext);
      ext[pathStr.size()-fPos]='\0';
    }
#endif
}

CSplitPath::~CSplitPath ()
{

}

const char *CSplitPath::getdrive()
{
  return drive ;
}


const char *CSplitPath::getname()
{
  return fname ;
}


const char *CSplitPath::getdir()
{
  return dir ;
}

const char *CSplitPath::getextension()
{
  return ext ;
}
