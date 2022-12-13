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
// ==========================================================================
// LogbookFile.h
//
// Author : Marquet Mike
//
// Company : /
//
// Date of creation  : 14/04/2003
// Last modification : 14/04/2003
// ==========================================================================

#if !defined(AFX_LOGBOOKFILE_H__9BB858FF_6166_45BC_8591_ABDEE909B58A__INCLUDED_)
#define AFX_LOGBOOKFILE_H__9BB858FF_6166_45BC_8591_ABDEE909B58A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ==========================================================================
// Les Includes
// ==========================================================================

#include "Logbook.h"

/////////////////////////////////////////////////////////////////////////////
// class CLogbookFile

class WINLIBSAPI CLogbookFile
 {
  protected :
              int         m_nFileHandle;
              int         m_nFlushCounter;
              SYSTEMTIME  m_stCurrentDate;
              CString     m_strPathName;
              CLogbook   *m_pLogbook;
              CString     m_strFileName;

              CString GetRenameFileName();

              CString GetStringType(LOGBOOK_TEXTTYPES nType);

              void MakeSeparator(char *sz, char cStyle, int nLen = 100);

              BOOL Open(BOOL bTrunc = FALSE);

              // INLINE
              inline LOGBOOK_INIT_FILE_INFOS *GetFileInfos() { return m_pLogbook ? &m_pLogbook->m_stFileInfos : NULL; }
              
              inline BOOL IsOpen() { return m_nFileHandle != -1; }

  public :
           CLogbookFile(CLogbook *pLogbook, LPCTSTR lpszFileName = "logbook.txt");
           virtual ~CLogbookFile();

           int AddLine(const LOGBOOKITEM_INFO &stInfo, const SYSTEMTIME &stST, BOOL bDisplayTime, BOOL bDisplayType);
 };

// ==========================================================================
// ==========================================================================

#endif // !defined(AFX_LOGBOOKFILE_H__9BB858FF_6166_45BC_8591_ABDEE909B58A__INCLUDED_)
