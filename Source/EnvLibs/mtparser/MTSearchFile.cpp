#include <EnvLibs.h>
#pragma hdrstop

#include "MTSearchFile.h"
#ifdef NO_MFC
#include <dirent.h>
#endif
//#include <windows.h>

void MTSearchFile::search(const std::vector<MTSTRING> &directories, const std::vector<MTSTRING> &searchPatterns, std::vector<MTSTRING> &results)
{      
    results.clear();
    
    for( unsigned int t=0; t < directories.size(); t++ )
    {
        // the path must end with a /
		MTSTRING dir = directories[t];

        if( dir[dir.size()-1] != '/' &&
            dir[dir.size()-1] != '\\' )
        {
            dir += _T("/");
        }

		search(dir.c_str(), searchPatterns, results);        
    }    
}

void MTSearchFile::search(const MTCHAR *directory, const std::vector<MTSTRING> &searchPatterns, std::vector<MTSTRING> &results)
{
    // file search
	for( unsigned int t=0; t<searchPatterns.size(); t++ )
	{	
		search( directory, searchPatterns[t].c_str(), results );		
	}    

#ifndef NO_MFC
	// now, we look for subfolders    
	WIN32_FIND_DATA findData;
    HANDLE hFind;
	MTSTRING curLookIn = directory;
	curLookIn += _T("*.*");
    
	hFind = FindFirstFile( curLookIn.c_str() , &findData);
    
    if( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {        
			// if this is a sub-folder then search it
            if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                findData.cFileName[0] != '.' )                
            {
                MTSTRING curLookDir = directory;
				curLookDir += findData.cFileName;
				curLookDir += _T("/");

				search( curLookDir.c_str(), searchPatterns, results );                
            }

        }while( FindNextFile(hFind, &findData) );        
    }     
    
    FindClose(hFind);
#else
    //for Linux
    //use directory utilities found in dirent.h

    DIR* dp;
    struct dirent *dirp;

    if((dp=opendir(directory)))
      {
	//iterate through directories
	while((dirp=readdir(dp))!=NULL)
	  {
	    MTSTRING dName(dirp->d_name);
	    if(dName!=".")
	      {
	    MTSTRING curLookDir=directory;
	    curLookDir+=dName;
	    curLookDir+=_T("/");
	    
	    search( curLookDir.c_str(), searchPatterns, results );                }
	  }

	closedir(dp);
      }
#endif

}

void MTSearchFile::search(const MTCHAR *directory, const MTCHAR *searchPattern, std::vector<MTSTRING> &results)
{	
	MTSTRING curLookIn = directory;
	curLookIn += searchPattern;
	
#ifndef NO_MFC
    WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(curLookIn.c_str(), &findData);
    
    if( hFind != INVALID_HANDLE_VALUE )
    {
        do
        { 	
            // skip directories and special files named "."
			if( !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				findData.cFileName[0] != '.' )                
            {
				MTSTRING found = directory;
				found += findData.cFileName;
                results.push_back( found );
            }
            
        }while( FindNextFile(hFind, &findData) );        
    }

    FindClose(hFind);
#else
    //for Linux
    //use directory utilities found in dirent.h

    DIR* dp;
    struct dirent *dirp;

    if((dp=opendir(directory)))
      {
	//iterate through directories
	while((dirp=readdir(dp))!=NULL)
	  {
	    MTSTRING dName(dirp->d_name);
	    if(dName!=".")
	      {
	    MTSTRING found=directory;
	    found+=dName;
	    results.push_back(found);
              }  
           }
	
	closedir(dp);
      }

#endif
}
