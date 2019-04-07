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
// LogbookGUIItem.cpp
//
// Author : Marquet Mike
//
// Company : /
//
// Date of creation  : 14/04/2003
// Last modification : 14/04/2003
// ==========================================================================

// ==========================================================================
// Les Includes
// ==========================================================================

#include "stdafx.h"
#include "EnvWinLibs.h"


#include "LogbookGUIItem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// class CLogbookGUIItem

CLogbookGUIItem::CLogbookGUIItem(CLogbook *pLogbook, const SYSTEMTIME &stST, LOGBOOK_TEXTTYPES nTextType, LOGBOOK_SEPARATORTYPES nSeparatorType)
 {
  m_pLogbook               = pLogbook;
  
  m_strDateTimeFormat      = "DD-MM-YYYY hh:mm:ss";
  m_clrTimeBackgroundColor = -1;
  m_clrTimeForegroundColor = -1;
  m_bDisplayTime           = TRUE;

  m_clrTypeBackgroundColor = -1;
  m_clrTypeForegroundColor = -1;
  m_nTextType              = nTextType;
  m_nSeparatorType         = nSeparatorType;
  m_bDisplayType           = TRUE;

  m_clrTextBackgroundColor = -1;
  m_clrTextForegroundColor = -1;
  m_strText                = "";

  memcpy(&m_stST, &stST, sizeof(SYSTEMTIME));
 }

// --------------------------------------------------------------------------

CLogbookGUIItem::~CLogbookGUIItem()
 {
 }

// --------------------------------------------------------------------------
// PROTECTED MEMBER FUNCTIONS
// --------------------------------------------------------------------------

COLORREF CLogbookGUIItem::GetDefaultBackgroundTypeColor()
 {
  switch(m_nTextType)
   {
    //case LOGBOOK_ERROR  : return RGB(255,  0,0);
    case LOGBOOK_ERROR  : return RGB(255,255,255);
    case LOGBOOK_FATAL  : return RGB(100, 40,0);
    case LOGBOOK_DEBUG1 : return RGB(255,255,0);
    case LOGBOOK_DEBUG2 : return RGB(255,255,0);
    case LOGBOOK_DEBUG3 : return RGB(255,255,0);
    case LOGBOOK_DEBUG4 : return RGB(255,255,0);
    case LOGBOOK_TRACE  : return RGB(  0,255,0);
   }

  return m_clrTypeBackgroundColor;
 }

// --------------------------------------------------------------------------

COLORREF CLogbookGUIItem::GetDefaultForegroundTypeColor()
 {
  switch(m_nTextType)
   {
    //case LOGBOOK_ERROR : return RGB(255,255,255);
    case LOGBOOK_ERROR : return RGB(127,0,0);
    case LOGBOOK_FATAL : return RGB(255,255,255);
    case LOGBOOK_TRACE : return RGB(  0,  0,  0);
   }

  return m_clrTypeForegroundColor;
 }

// --------------------------------------------------------------------------
// PUBLIC MEMBER FUNCTIONS
// --------------------------------------------------------------------------

COLORREF CLogbookGUIItem::GetBackgroundColor(int nCol)
 {
  if ( nCol == 1 && m_pLogbook && m_pLogbook->m_stGuiInfos.bUseDefaultTypeColors ) return GetDefaultBackgroundTypeColor();

  switch(nCol)
   {
    case 0 : return m_clrTimeBackgroundColor;

    case 1 : return m_clrTypeBackgroundColor;

    case 2 : return m_clrTextBackgroundColor;
   }

  return GetSysColor(COLOR_WINDOW);
 }

// --------------------------------------------------------------------------

COLORREF CLogbookGUIItem::GetForegroundColor(int nCol)
 {
  if ( nCol == 1 && m_pLogbook && m_pLogbook->m_stGuiInfos.bUseDefaultTypeColors ) return GetDefaultForegroundTypeColor();

  switch(nCol)
   {
    case 0 : return m_clrTimeForegroundColor;

    case 1 : return m_clrTypeForegroundColor;

    case 2 : return m_clrTextForegroundColor;
   }

  return GetSysColor(COLOR_WINDOWTEXT);
 }

// --------------------------------------------------------------------------

CString CLogbookGUIItem::GetText(int nCol)
 {
  switch(nCol)
   {
    case 0 : return m_bDisplayTime ? m_pLogbook->GetStringTime(m_stST) : "";

    case 1 : return m_bDisplayType ? m_pLogbook->GetStringType(m_nTextType) : "";

    case 2 : return m_strText;
   } 

  return "";
 }

// --------------------------------------------------------------------------
