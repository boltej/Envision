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

#include "PathManager.h"
#include "Report.h"



map<int, string> PathManager::m_pathMap;


PathManager::PathManager(void)
   {
   }


PathManager::~PathManager(void)
   {
   }


int PathManager::AddPath( LPCTSTR _path, int id /*=-9999*/ )
   {
   // clean up whatever is passed in
   CPath path( _path, epcTrim | epcSlashToBackslash );

   path.AddBackslash();

   CString msg("Adding Path: ");
   msg += path;
   
   Report::Log(msg);
   if (id == -9999)
      {
      int count = (int) m_pathMap.size();
      m_pathMap[-count] = path;
      }
   else
      m_pathMap[id] = path;

   return (int) m_pathMap.size();
   }


int PathManager::SetPath( int index, LPCTSTR _path )
   {
   if ( index < 0 )
      return -1;
       
   if ( index >= (int) m_pathMap.size() )
      return -2;

   // clean up whatever is passed in
   CPath path( _path, epcTrim | epcSlashToBackslash );
   path.AddBackslash();

   m_pathMap[ index ] = path;

   return index;
   }


LPCTSTR PathManager::GetPath(int i)
   {
   return m_pathMap[i].c_str();
   }       // this will always be terminated with a '\'


int PathManager::FindPath( LPCTSTR _path, CString &_fullPath )
   {
   _fullPath.Empty();

   //  return value: > 0 = success; < 0 = failure (file not found), 0 = path fully qualified and found 
   CPath path( _path, epcTrim | epcSlashToBackslash );
   
   // case 1:  just a filename or a relative path - look through pathArray
   if ( path.IsFileSpec() || path.IsRelative() )  // only a file, not a path
      {
      for (auto it = m_pathMap.rbegin(); it != m_pathMap.rend(); ++it) 
         {
         CPath fullPath(it->second.c_str());
   
         // append the relative path that was passed in
         fullPath.Append( path );
   
         if ( fullPath.Exists() )
            {
            _fullPath = fullPath;
            return it->first;
            }
         }

      _fullPath.Empty();
      return -1;
      }

   else // it is already full qualified
      {
      CPath fullPath( path );
      if ( fullPath.Exists() )
         {
         _fullPath = path;
         return 0;
         }
      }

   _fullPath.Empty();
   return -2;
   }


int PathManager::FileOpen( LPCTSTR _path, FILE **fp, LPCTSTR _mode )
   {
   // _path can be:
   //   1) a file name  e.g. file.ext.    In this case, the paths are searched
   //         until a file is found.
   //   2) a relative path  e.g. somedir\file.ext.  In this case, search the paths 
   //   3) fully-qualified  e.g.  c:\somepath\file.ext. In that case, just open it
   //      if possible.
   //
   //  return value: > 0 = success; <= 0 = failure
   
   CPath path( _path, epcTrim | epcSlashToBackslash );

   LPCTSTR mode = _mode;
   if ( mode == NULL )
      mode = _T("rt");

   errno_t err = 0;
   *fp = NULL;

   // case 1:  just a filename
   if ( path.IsFileSpec() )  // only a file, not a path
      {
      PCTSTR file = (PCTSTR)path;
    	err = fopen_s( fp, file, mode );  // first, look in the current directory

      if ( ! err && fp != NULL )
         return 1;
      else                             // if not there, search paths
         return SearchPaths( path, fp, mode );
      }

   // case 2: relative path
   else if ( path.IsRelative() )
      {
      return SearchPaths( path, fp, mode );
      }

   // case 3:: fully qualified path
   else 
      {
      PCTSTR file = (PCTSTR)path;
      err = fopen_s( fp, file, mode );  // first, look in the current directory

      if ( ! err && fp != NULL )
         return 1;
      else                             // if not there, search paths
         return SearchPaths( path, fp, mode );
      }

   return 0;
   }


int PathManager::SearchPaths( CPath &path, FILE **fp, LPCTSTR mode )
   {
   errno_t err = 0;
   *fp = NULL;

   for (auto it = m_pathMap.rbegin(); it != m_pathMap.rend(); ++it)
      {
      CPath fullPath(it->second.c_str());

      // append the relative path that was passed in
      fullPath.Append( path );

      // try to open it
      PCTSTR file = (PCTSTR)fullPath;
    	err = fopen_s( fp, file, mode );  // first, look in the current directory

      if ( ! err && fp != NULL )
         return 2;
      }

   return 0;
   }
