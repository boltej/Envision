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
// LogbookGUIItem.h
//
// Author : Marquet Mike
//
// Company : /
//
// Date of creation  : 14/04/2003
// Last modification : 14/04/2003
// ==========================================================================

#if !defined(AFX_LOGBOOKGUIITEM_H__391F82B5_6161_4488_9C6F_A5FD0DAD1D31__INCLUDED_)
#define AFX_LOGBOOKGUIITEM_H__391F82B5_6161_4488_9C6F_A5FD0DAD1D31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ==========================================================================
// Les Includes
// ==========================================================================

#include "Logbook.h"

/////////////////////////////////////////////////////////////////////////////
// class CLogbookGUIItem

class WINLIBSAPI CLogbookGUIItem : public CObject
 {
  protected :
              CLogbook *m_pLogbook;
              
              COLORREF GetDefaultBackgroundTypeColor();
              COLORREF GetDefaultForegroundTypeColor();

  public :
           // TIME column specification
           CString                m_strDateTimeFormat;
           COLORREF               m_clrTimeBackgroundColor; // -1 = Use default color
           COLORREF               m_clrTimeForegroundColor; // -1 = Use default color
           SYSTEMTIME             m_stST;
           BOOL                   m_bDisplayTime;

           // TYPE column specification
           COLORREF               m_clrTypeBackgroundColor; // -1 = Use default color
           COLORREF               m_clrTypeForegroundColor; // -1 = Use default color
           LOGBOOK_TEXTTYPES      m_nTextType;
           LOGBOOK_SEPARATORTYPES m_nSeparatorType;
           BOOL                   m_bDisplayType;

           // TEXT column specification
           COLORREF               m_clrTextBackgroundColor; // -1 = Use default color
           COLORREF               m_clrTextForegroundColor; // -1 = Use default color
           CString                m_strText;
           
           CLogbookGUIItem(CLogbook *pLogbook, const SYSTEMTIME &stST, LOGBOOK_TEXTTYPES nTextType, LOGBOOK_SEPARATORTYPES nSeparatorType);
           virtual ~CLogbookGUIItem();

           COLORREF GetBackgroundColor(int nCol);
           COLORREF GetForegroundColor(int nCol);

           CString GetText(int nCol);

           // INLINE
           inline BOOL IsFirstLine() { return (m_bDisplayTime && m_bDisplayType); }

           inline BOOL IsSeparatorLine() { return (m_nTextType == LOGBOOK_NONE); }
 };

// ==========================================================================
// ==========================================================================

#endif // !defined(AFX_LOGBOOKGUIITEM_H__391F82B5_6161_4488_9C6F_A5FD0DAD1D31__INCLUDED_)
