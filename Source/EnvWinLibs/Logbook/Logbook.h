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
// Logbook.h
//
// Author : Marquet Mike
//
// Company : /
//
// Date of creation  : 14/04/2003
// Last modification : 14/04/2003
// ==========================================================================

#if !defined(AFX_LOGBOOK_H__6421D5EE_FF99_4760_A0C3_85FBF17A8591__INCLUDED_)
#define AFX_LOGBOOK_H__6421D5EE_FF99_4760_A0C3_85FBF17A8591__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EnvWinLibs.h"

/*         
                        -------------------------
                        D O C U M E N T A T I O N
                        -------------------------

    DATETIME format : DAY   ->  day of week in text format
                      DD    ->  day
                      MM    ->  month
                      MONTH ->  month in text format
                      YY    ->  year in 2 digits format
                      YYYY  ->  year in 4 digits format
                      hh    ->  hour
                      mm    ->  minuts
                      ss    ->  seconds
                      mil   ->  milliseconds

*/

// ==========================================================================
// Les Enumérateurs
// ==========================================================================

enum LOGBOOK_TEXTTYPES
 {
  LOGBOOK_NONE       = 0,
  LOGBOOK_INFO       = 1,
  LOGBOOK_WARNING    = 2,
  LOGBOOK_ERROR      = 3,
  LOGBOOK_FATAL      = 4,
  LOGBOOK_SYSTEM     = 5,
  LOGBOOK_DEBUG1     = 6,
  LOGBOOK_DEBUG2     = 7,
  LOGBOOK_DEBUG3     = 8,
  LOGBOOK_DEBUG4     = 9,
  LOGBOOK_TRACE      = 10,
  LOGBOOK_PERSONAL1  = 11,
  LOGBOOK_PERSONAL2  = 12,
  LOGBOOK_PERSONAL3  = 13,
  LOGBOOK_PERSONAL4  = 14,
  LOGBOOK_PERSONAL5  = 15,
  LOGBOOK_PERSONAL6  = 16,
  LOGBOOK_PERSONAL7  = 17,
  LOGBOOK_PERSONAL8  = 18,
  LOGBOOK_PERSONAL9  = 19,
  LOGBOOK_PERSONAL10 = 20,
  LOGBOOK_PERSONAL11 = 21,
  LOGBOOK_PERSONAL12 = 22,
  LOGBOOK_PERSONAL13 = 23,
  LOGBOOK_PERSONAL14 = 24,
 };

// --------------------------------------------------------------------------

enum LOGBOOK_SEPARATORTYPES
 {
  LOGBOOK_EMPTYLINE           = 0,
  LOGBOOK_SEPARATOR           = 1,
  LOGBOOK_DBLSEPARATOR        = 2,
  LOGBOOK_DASHSEPARATOR       = 3,
  LOGBOOK_DOTSEPARATOR        = 4,
  LOGBOOK_DASHDOTSEPARATOR    = 5,
  LOGBOOK_DASHDOTDOTSEPARATOR = 6,
 };

// --------------------------------------------------------------------------

enum LOGBOOK_CLASSTYPES
 {
  CLASSTYPE_NULL        = 0,
  CLASSTYPE_LOGBOOKFILE = 1,
  CLASSTYPE_LOGBOOKCTRL = 2,
  CLASSTYPE_LOGBOOKVIEW = 3,
 };

// ==========================================================================
// Les Structures
// ==========================================================================

typedef struct
 {
  CString strDateTimeFormat;    // represent the date time format to use                              (default is DD-MM-YYYY hh:mm:ss)
  BOOL    bUseThreadManagement; // write directly to file and/or GUI or transit via thread processing (default is FALSE)
  BOOL    bCaptureTRACE;        // capture all OutputDebugString commands                             (default is FALSE)
 } LOGBOOK_INIT_INFOS;

// --------------------------------------------------------------------------

typedef struct
 {
  CString strPath;               // path to store the file. If empty, then write it into current directory (default is empty)
  DWORD   nMaxSize;              // maximum file size. 0 = unlimited                                       (default is 0)
  BOOL    bCreateNewFileEachDay; // create a new file every new day                                        (defaulr is FALSE)
  BOOL    bClearFileEachOpen;    // clear the file every time it is opened                                 (default is FALSE)
  BOOL    bReuseSameFile;        // if max size is used, create a new file name if size is reached         (default is TRUE)
  int     nMaxFlushCounter;      // max insertion to commit to file, 0 = don't use                         (default is 0)
 } LOGBOOK_INIT_FILE_INFOS;

// --------------------------------------------------------------------------

typedef struct
 {
  BOOL     bDrawColumnSeparators; // draw columns separators or not    (default is FALSE)
  BOOL     bAddToEndOfList;       // add to end of list or not         (default is TRUE)
  int      nMaxLines;             // maximumn number of lines in list  (default is 1000)
  BOOL     bShowFocusRectangle;   // draw or not the focus rectangel   (default is TRUE)
  BOOL     bUseDefaultTypeColors; // Use or not the default colors     (default is TRUE)
  COLORREF clrList;               // List background color             (default is -1)
  BOOL     bDateTimeBOLD;         // Use a bold font to draw date/time (default is FALSE)
 } LOGBOOK_INIT_GUI_INFOS;

// --------------------------------------------------------------------------

typedef struct
 {
  COLORREF               clrTimeBackgroundColor; // -1 for default color
  COLORREF               clrTimeForegroundColor; // -1 for default color
  COLORREF               clrTypeBackgroundColor; // -1 for default color
  COLORREF               clrTypeForegroundColor; // -1 for default color
  LOGBOOK_TEXTTYPES      nTextType;              // LOGBOOK_NONE if separator line
  LOGBOOK_SEPARATORTYPES nSeparatorType;         // Not used if nTextType != LOGBOOK_NONE
  COLORREF               clrTextBackgroundColor; // -1 for default color
  COLORREF               clrTextForegroundColor; // -1 for default color
  CString                strText;                // actual text, lines separated by \n
 } LOGBOOKITEM_INFO;

/////////////////////////////////////////////////////////////////////////////
// class CLogbook

class WINLIBSAPI CLogbook
 {
  friend class CFileGUI_BaseClass;
  friend class CLogbookFile;
  friend class CLogbookGUI;
  friend class CLogbookGUIItem;

  protected :
              BOOL                     m_bInitialized;
              CString                  m_strPersonal[14]; // list of personal strings
              CPtrArray                m_aIcons;          // list of icons representing logbook types [ only used for GUI ]
              LOGBOOK_INIT_INFOS       m_stInfos;
              LOGBOOK_INIT_FILE_INFOS  m_stFileInfos;
              BOOL                     m_bFileInfos;
              LOGBOOK_INIT_GUI_INFOS   m_stGuiInfos;
              BOOL                     m_bGuiInfos;
              CLogbookFile            *m_pLogbookFile;    // specify the class to write into a file
              CLogbookGUI             *m_pLogbookGUI;     // specify the class to write into a GUI
              HANDLE                   m_hThread;
              HANDLE                   m_hStopEvent;
              CRITICAL_SECTION         m_stCriticalSection;
              CObList                  m_cThreadList;
              _CRT_REPORT_HOOK         m_hCrtReportHook;

              static int LogbookReportHookProc(int nReportType, char *szUserMsg, int *pnRetValue);

              static DWORD WINAPI ThreadProc(LPVOID lpParameter);

              int AddLine(const LOGBOOKITEM_INFO &stInfo, const SYSTEMTIME *pstST = NULL, BOOL bDisplayTime = TRUE, BOOL bDisplayType = TRUE);

              CString GetDayOfWeek(WORD nDayOfWeek);

              CString GetMonth(WORD nMonth);

              CString GetStringTime(const SYSTEMTIME &stST, BOOL bForceFirstMethod = FALSE);

              CString GetStringType(LOGBOOK_TEXTTYPES nTextType);

              HICON GetTypeIcon(LOGBOOK_TEXTTYPES nType);

              void STRING_REPLACE(CString &str, LPCTSTR lpszOld, int nVal, int nDigits = 2);

              //void UnixTimeToFileTime(time_t nTime, LPFILETIME lpstFileTime);

              // INLINE
              inline BOOL IconsPresent() { return ( m_aIcons.GetSize() ? TRUE : FALSE ); }

              inline void Lock() { EnterCriticalSection(&m_stCriticalSection); }

              inline void Unlock() { LeaveCriticalSection(&m_stCriticalSection); }

  public :
           CLogbook();
           virtual ~CLogbook();


           static void FillDateTimeFormatComboBox(CComboBox *pComboBox, CString strFormatToSelect = "DD-MM-YYYY hh:mm:ss");

           static void FillSeparatorTypeComboBox(CComboBox *pComboBox, LOGBOOK_SEPARATORTYPES nTypeToSelect = LOGBOOK_SEPARATOR);

           static void FillTextTypeComboBox(CComboBox *pComboBox, LOGBOOK_TEXTTYPES nTypeToSelect = LOGBOOK_INFO);

           static void InitInfos(LOGBOOK_INIT_INFOS &stInfos, const LOGBOOK_INIT_INFOS *pstInfos = NULL);

           static void InitInfos(LOGBOOK_INIT_FILE_INFOS &stInfos, const LOGBOOK_INIT_FILE_INFOS *pstInfos = NULL);

           static void InitInfos(LOGBOOK_INIT_GUI_INFOS &stInfos, const LOGBOOK_INIT_GUI_INFOS *pstInfos = NULL);

           static void LogbookItemCopy(const LOGBOOKITEM_INFO *psrc, LOGBOOKITEM_INFO &dst);


           int AddItemLine(const LOGBOOKITEM_INFO &stItemInfo);

           int AddLogLine(LPCTSTR lpszText, LOGBOOK_TEXTTYPES nType = LOGBOOK_INFO, COLORREF textColor=0, ...);

           int AddMemoryLine();

           int AddSeparatorLine(LOGBOOK_SEPARATORTYPES nType = LOGBOOK_SEPARATOR);

           int AddTitleLine(LPCTSTR lpszTitle);

           BOOL AttachFile(LPCTSTR lpszFileName);

           HWND AttachGUI(CWnd *pParentWnd);

           void DetachFile();
           void DetachGUI();

           LPCTSTR GetPersonalType(LOGBOOK_TEXTTYPES nTextType);

           int Initialize(const LOGBOOK_INIT_INFOS      &stInfos,
                          const LOGBOOK_INIT_FILE_INFOS *pstFileInfos, // set to NULL for not using file
                          const LOGBOOK_INIT_GUI_INFOS  *pstGuiInfos); // set to NULL for not using GUI

           void SetPersonalType(LOGBOOK_TEXTTYPES nTextType, LPCTSTR lpszText);

           void SetTypeIcon(LOGBOOK_TEXTTYPES nType, UINT nIconID);
           void SetTypeIcon(LOGBOOK_TEXTTYPES nType, LPCTSTR lpszIconName);

           void Uninitialize();

           // INLINE
           inline BOOL IsInitialized() { return m_bInitialized; }
 };

// ==========================================================================
// ==========================================================================

#endif // !defined(AFX_LOGBOOK_H__6421D5EE_FF99_4760_A0C3_85FBF17A8591__INCLUDED_)
