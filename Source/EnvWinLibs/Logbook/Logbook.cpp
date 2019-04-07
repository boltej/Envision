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
// Logbook.cpp
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

#include "Logbook.h"
#include "ThreadObject.h"
#include "LogbookFile.h"
#include "LogbookGUI.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// ==========================================================================
// Les Structures
// ==========================================================================

typedef struct
 {
  LOGBOOK_TEXTTYPES nType;
  HICON             hIcon;
 } TEXTTYPE_ICON;

// ==========================================================================
// Les Variables Globales
// ==========================================================================

CLogbook *g_pLogbook = NULL;

/////////////////////////////////////////////////////////////////////////////
// class CLogbook

CLogbook::CLogbook()
 {
  m_bInitialized   = FALSE;
  m_pLogbookFile   = NULL;
  m_bFileInfos     = FALSE;
  m_pLogbookGUI    = NULL;
  m_bGuiInfos      = FALSE;
  m_hThread        = NULL;
  m_hStopEvent     = CreateEvent(NULL, TRUE, FALSE, NULL);
  m_hCrtReportHook = NULL;

  InitializeCriticalSection(&m_stCriticalSection);
 }

// --------------------------------------------------------------------------

CLogbook::~CLogbook()
 {
  Uninitialize();

  DetachFile();
  DetachGUI();

  for (int I=0; I<m_aIcons.GetSize(); I++)
   {
    TEXTTYPE_ICON *pTTI = (TEXTTYPE_ICON *)m_aIcons.GetAt(I);

    if (pTTI && pTTI->hIcon) DestroyIcon(pTTI->hIcon);

    if (pTTI) delete pTTI;
   }

  CloseHandle(m_hStopEvent);

  DeleteCriticalSection(&m_stCriticalSection);
 }


// --------------------------------------------------------------------------
// STATIC PROTECTED MEMBER FUNCTIONS
// --------------------------------------------------------------------------

int CLogbook::LogbookReportHookProc(int nReportType, char *szUserMsg, int *pnRetValue)
 {
  *pnRetValue = 0;

  switch(nReportType)
   {
    case _CRT_WARN :
                     if (g_pLogbook && strlen(szUserMsg) > 0)
                      {
                       //if (szUserMsg[strlen(szUserMsg) - 1] == '\n') szUserMsg[strlen(szUserMsg) - 1] = 0;
                       //if (szUserMsg[strlen(szUserMsg) - 1] == '\r') szUserMsg[strlen(szUserMsg) - 1] = 0;
                       //if (szUserMsg[strlen(szUserMsg) - 1] == '\n') szUserMsg[strlen(szUserMsg) - 1] = 0;

                       g_pLogbook->AddLogLine(szUserMsg, LOGBOOK_TRACE);
                      }
                     break;
   }

  return 0;
 }

// --------------------------------------------------------------------------

DWORD WINAPI CLogbook::ThreadProc(LPVOID lpParameter)
 {
  CLogbook *pLogbook = (CLogbook *)lpParameter;
  
  TRACE("(CLogbook) ThreadProc() STARTED\n");

  while(1)
   {
    DWORD nRet = WaitForSingleObject(pLogbook->m_hStopEvent, 0);

    if (nRet == WAIT_OBJECT_0) break; // stop event received

    if ( !pLogbook->m_cThreadList.GetCount() )
     {
      Sleep(100);

      continue;
     }

    pLogbook->Lock();

    CThreadObject *pThreadObject = (CThreadObject *)pLogbook->m_cThreadList.RemoveHead();

    pLogbook->Unlock();

    pLogbook->AddLine(pThreadObject->m_stItemInfo,
                      &pThreadObject->m_stST,
                      pThreadObject->m_bDisplayTime,
                      pThreadObject->m_bDisplayType);

    delete pThreadObject;
   }
  
  TRACE("(CLogbook) ThreadProc() STOPPED\n");

  return 0;
 }

// --------------------------------------------------------------------------
// PROTECTED MEMBER FUNCTIONS
// --------------------------------------------------------------------------

int CLogbook::AddLine(const LOGBOOKITEM_INFO &stInfo, const SYSTEMTIME *pstST, BOOL bDisplayTime, BOOL bDisplayType)
 {
  BOOL bAddToList = (m_hThread && !pstST);

  SYSTEMTIME stST;
  
  if (!pstST)
   GetLocalTime(&stST);
  else
   memcpy(&stST, pstST, sizeof(SYSTEMTIME));
  
  if (bAddToList) // insert into thread list
   {
    CThreadObject *pThreadObject = new CThreadObject;

    LogbookItemCopy(&stInfo, pThreadObject->m_stItemInfo);

    pThreadObject->m_bDisplayTime = bDisplayTime;
    pThreadObject->m_bDisplayType = bDisplayType;

    memcpy(&pThreadObject->m_stST, &stST, sizeof(SYSTEMTIME));

    Lock();

    m_cThreadList.AddTail(pThreadObject);

    Unlock();

    return 1;
   }

  int nRet = 0;

  Lock();
  if (m_pLogbookFile && nRet >= 0) nRet = m_pLogbookFile->AddLine(stInfo, stST, bDisplayTime, bDisplayType);
    
  if (m_pLogbookGUI && nRet >= 0) nRet = m_pLogbookGUI->AddLine(stInfo, stST, bDisplayTime, bDisplayType);
  Unlock();
  return nRet;
 }

// --------------------------------------------------------------------------

CString CLogbook::GetDayOfWeek(WORD nDayOfWeek)
 {
  switch(nDayOfWeek)
   {
    case 0 : return "Sunday";
    case 1 : return "Monday";
    case 2 : return "Tuesday";
    case 3 : return "Wednesday";
    case 4 : return "Thursday";
    case 5 : return "Friday";
    case 6 : return "Saturday";
   }
  
  return "";
 }

// --------------------------------------------------------------------------

CString CLogbook::GetMonth(WORD nMonth)
 {
  switch(nMonth)
   {
    case 1  : return "January";
    case 2  : return "February";
    case 3  : return "March";
    case 4  : return "April";
    case 5  : return "May";
    case 6  : return "June";
    case 7  : return "July";
    case 8  : return "August";
    case 9  : return "September";
    case 10 : return "October";
    case 11 : return "November";
    case 12 : return "December";
   }

  return "";
 }

// --------------------------------------------------------------------------

CString CLogbook::GetStringTime(const SYSTEMTIME &stST, BOOL bForceFirstMethod)
 {
  CString str = bForceFirstMethod ? "DD-MM-YYYY hh:mm:ss" : m_stInfos.strDateTimeFormat;

  str.Replace("DAY", GetDayOfWeek(stST.wDayOfWeek));
  
  STRING_REPLACE(str, "DD", stST.wDay);

  STRING_REPLACE(str, "MM", stST.wMonth);

  str.Replace("MONTH", GetMonth(stST.wMonth));

  STRING_REPLACE(str, "YYYY", stST.wYear, 4);

  STRING_REPLACE(str, "YY", stST.wYear - 2000);

  STRING_REPLACE(str, "hh", stST.wHour);

  STRING_REPLACE(str, "mm", stST.wMinute);

  STRING_REPLACE(str, "ss", stST.wSecond);

  STRING_REPLACE(str, "mil", stST.wMilliseconds, 3);

  return str;
 }

// --------------------------------------------------------------------------

CString CLogbook::GetStringType(LOGBOOK_TEXTTYPES nTextType)
 {
  if (nTextType == LOGBOOK_INFO   ) return "INFO";
  if (nTextType == LOGBOOK_WARNING) return "WARNING";
  if (nTextType == LOGBOOK_ERROR  ) return "ERROR";
  if (nTextType == LOGBOOK_FATAL  ) return "FATAL";
  if (nTextType == LOGBOOK_SYSTEM ) return "SYSTEM";
  if (nTextType == LOGBOOK_DEBUG1 ) return "DEBUG1";
  if (nTextType == LOGBOOK_DEBUG2 ) return "DEBUG2";
  if (nTextType == LOGBOOK_DEBUG3 ) return "DEBUG3";
  if (nTextType == LOGBOOK_DEBUG4 ) return "DEBUG4";
  if (nTextType == LOGBOOK_TRACE  ) return "TRACE";

  if (nTextType == LOGBOOK_PERSONAL1 ) return m_strPersonal[0];
  if (nTextType == LOGBOOK_PERSONAL2 ) return m_strPersonal[1];
  if (nTextType == LOGBOOK_PERSONAL3 ) return m_strPersonal[2];
  if (nTextType == LOGBOOK_PERSONAL4 ) return m_strPersonal[3];
  if (nTextType == LOGBOOK_PERSONAL5 ) return m_strPersonal[4];
  if (nTextType == LOGBOOK_PERSONAL6 ) return m_strPersonal[5];
  if (nTextType == LOGBOOK_PERSONAL7 ) return m_strPersonal[6];
  if (nTextType == LOGBOOK_PERSONAL8 ) return m_strPersonal[7];
  if (nTextType == LOGBOOK_PERSONAL9 ) return m_strPersonal[8];
  if (nTextType == LOGBOOK_PERSONAL10) return m_strPersonal[9];
  if (nTextType == LOGBOOK_PERSONAL11) return m_strPersonal[10];
  if (nTextType == LOGBOOK_PERSONAL12) return m_strPersonal[11];
  if (nTextType == LOGBOOK_PERSONAL13) return m_strPersonal[12];
  if (nTextType == LOGBOOK_PERSONAL14) return m_strPersonal[13];

  return "";
 }

// --------------------------------------------------------------------------

HICON CLogbook::GetTypeIcon(LOGBOOK_TEXTTYPES nType)
 {
  for (int I=0; I<m_aIcons.GetSize(); I++)
   {
    TEXTTYPE_ICON *pTTI2 = (TEXTTYPE_ICON *)m_aIcons.GetAt(I);

    if (pTTI2 && pTTI2->nType == nType) return pTTI2->hIcon;
   }

  return NULL;
 }

// --------------------------------------------------------------------------

void CLogbook::STRING_REPLACE(CString &str, LPCTSTR lpszOld, int nVal, int nDigits)
 {
  CString strVal;

  if (nDigits == 4)
   strVal.Format("%04u", nVal);
  else if (nDigits == 3)
   strVal.Format("%03u", nVal);
  else
   strVal.Format("%02u", nVal);

  str.Replace(lpszOld, strVal);
 }

// --------------------------------------------------------------------------
/*
void CLogbook::UnixTimeToFileTime(time_t nTime, LPFILETIME lpstFileTime)
 {
  __int64 nVal;

  nVal = Int32x32To64(nTime, 10000000) + 116444736000000000;

  lpstFileTime->dwLowDateTime = (DWORD)nVal;

  lpstFileTime->dwHighDateTime = (unsigned long)(nVal >> 32);
 }
*/
// --------------------------------------------------------------------------
// STATIC PUBLIC MEMBER FUNCTIONS
// --------------------------------------------------------------------------

void CLogbook::FillDateTimeFormatComboBox(CComboBox *pComboBox, CString strFormatToSelect)
 {
  if (!pComboBox) return;

  pComboBox->ResetContent();

  int nIndex = pComboBox->AddString("DD-MM-YYYY hh:mm:ss");

  nIndex = pComboBox->AddString("DD-MM-YY hh:mm:ss");

  nIndex = pComboBox->AddString("DD-MM-YYYY hh:mm:ss.mil");

  nIndex = pComboBox->AddString("DAY DD MONTH YYYY hh:mm:ss");

  nIndex = pComboBox->AddString("DAY DD MONTH YYYY hh:mm:ss.mil");

  for (int I=0; I<pComboBox->GetCount(); I++)
   {
    CString str;

    pComboBox->GetLBText(I, str);

    if (str != strFormatToSelect) continue;

    pComboBox->SetCurSel(I);

    break;
   }
 }

// --------------------------------------------------------------------------

void CLogbook::FillSeparatorTypeComboBox(CComboBox *pComboBox, LOGBOOK_SEPARATORTYPES nTypeToSelect)
 {
  if (!pComboBox) return;

  pComboBox->ResetContent();

  int nIndex = pComboBox->AddString("EMPTY LINE"); pComboBox->SetItemData(nIndex, LOGBOOK_EMPTYLINE);

  nIndex = pComboBox->AddString("SEPARATOR"); pComboBox->SetItemData(nIndex, LOGBOOK_SEPARATOR);
  nIndex = pComboBox->AddString("DBLSEPARATOR"); pComboBox->SetItemData(nIndex, LOGBOOK_DBLSEPARATOR);
  nIndex = pComboBox->AddString("DASHSEPARATOR"); pComboBox->SetItemData(nIndex, LOGBOOK_DASHSEPARATOR);
  nIndex = pComboBox->AddString("DOTSEPARATOR"); pComboBox->SetItemData(nIndex, LOGBOOK_DOTSEPARATOR);
  nIndex = pComboBox->AddString("DASHDOTSEPARATOR"); pComboBox->SetItemData(nIndex, LOGBOOK_DASHDOTSEPARATOR);
  nIndex = pComboBox->AddString("DASHDOTDOTSEPARATOR"); pComboBox->SetItemData(nIndex, LOGBOOK_DASHDOTDOTSEPARATOR);

  for (int I=0; I<pComboBox->GetCount(); I++)
   {
    LOGBOOK_SEPARATORTYPES nType = (LOGBOOK_SEPARATORTYPES)pComboBox->GetItemData(I);

    if (nType != nTypeToSelect) continue;

    pComboBox->SetCurSel(I);

    break;
   }
 }

// --------------------------------------------------------------------------

void CLogbook::FillTextTypeComboBox(CComboBox *pComboBox, LOGBOOK_TEXTTYPES nTypeToSelect)
 {
  if (!pComboBox) return;

  pComboBox->ResetContent();

  int nIndex = pComboBox->AddString("NONE"); pComboBox->SetItemData(nIndex, LOGBOOK_NONE);

  nIndex = pComboBox->AddString("INFO"); pComboBox->SetItemData(nIndex, LOGBOOK_INFO);
  nIndex = pComboBox->AddString("WARNING"); pComboBox->SetItemData(nIndex, LOGBOOK_WARNING);
  nIndex = pComboBox->AddString("ERROR"); pComboBox->SetItemData(nIndex, LOGBOOK_ERROR);
  nIndex = pComboBox->AddString("FATAL"); pComboBox->SetItemData(nIndex, LOGBOOK_FATAL);
  nIndex = pComboBox->AddString("SYSTEM"); pComboBox->SetItemData(nIndex, LOGBOOK_SYSTEM);
  nIndex = pComboBox->AddString("DEBUG1"); pComboBox->SetItemData(nIndex, LOGBOOK_DEBUG1);
  nIndex = pComboBox->AddString("DEBUG2"); pComboBox->SetItemData(nIndex, LOGBOOK_DEBUG2);
  nIndex = pComboBox->AddString("DEBUG3"); pComboBox->SetItemData(nIndex, LOGBOOK_DEBUG3);
  nIndex = pComboBox->AddString("DEBUG4"); pComboBox->SetItemData(nIndex, LOGBOOK_DEBUG4);
  nIndex = pComboBox->AddString("TRACE"); pComboBox->SetItemData(nIndex, LOGBOOK_TRACE);
  nIndex = pComboBox->AddString("PERSONAL1"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL1);
  nIndex = pComboBox->AddString("PERSONAL2"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL2);
  nIndex = pComboBox->AddString("PERSONAL3"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL3);
  nIndex = pComboBox->AddString("PERSONAL4"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL4);
  nIndex = pComboBox->AddString("PERSONAL5"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL5);
  nIndex = pComboBox->AddString("PERSONAL6"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL6);
  nIndex = pComboBox->AddString("PERSONAL7"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL7);
  nIndex = pComboBox->AddString("PERSONAL8"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL8);
  nIndex = pComboBox->AddString("PERSONAL9"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL9);
  nIndex = pComboBox->AddString("PERSONAL10"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL10);
  nIndex = pComboBox->AddString("PERSONAL11"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL11);
  nIndex = pComboBox->AddString("PERSONAL12"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL12);
  nIndex = pComboBox->AddString("PERSONAL13"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL13);
  nIndex = pComboBox->AddString("PERSONAL14"); pComboBox->SetItemData(nIndex, LOGBOOK_PERSONAL14);

  for (int I=0; I<pComboBox->GetCount(); I++)
   {
    LOGBOOK_TEXTTYPES nType = (LOGBOOK_TEXTTYPES)pComboBox->GetItemData(I);

    if (nType != nTypeToSelect) continue;

    pComboBox->SetCurSel(I);

    break;
   }
 }

// --------------------------------------------------------------------------

void CLogbook::InitInfos(LOGBOOK_INIT_INFOS &stInfos, const LOGBOOK_INIT_INFOS *pstInfos)
 {
  stInfos.strDateTimeFormat    = "DD-MM-YYYY hh:mm:ss";
  stInfos.bUseThreadManagement = FALSE;
  stInfos.bCaptureTRACE        = FALSE;

  if (!pstInfos) return;

  stInfos.strDateTimeFormat    = pstInfos->strDateTimeFormat;
  stInfos.bUseThreadManagement = pstInfos->bUseThreadManagement;
  stInfos.bCaptureTRACE        = pstInfos->bCaptureTRACE;
 }

// --------------------------------------------------------------------------

void CLogbook::InitInfos(LOGBOOK_INIT_FILE_INFOS &stInfos, const LOGBOOK_INIT_FILE_INFOS *pstInfos)
 {
  stInfos.strPath               = "";
  stInfos.nMaxSize              = 0;
  stInfos.bCreateNewFileEachDay = FALSE;
  stInfos.bClearFileEachOpen    = FALSE;
  stInfos.bReuseSameFile        = TRUE;
  stInfos.nMaxFlushCounter      = 0;

  if (!pstInfos) return;

  stInfos.strPath               = pstInfos->strPath;
  stInfos.nMaxSize              = pstInfos->nMaxSize;
  stInfos.bCreateNewFileEachDay = pstInfos->bCreateNewFileEachDay;
  stInfos.bClearFileEachOpen    = pstInfos->bClearFileEachOpen;
  stInfos.bReuseSameFile        = pstInfos->bReuseSameFile;
  stInfos.nMaxFlushCounter      = pstInfos->nMaxFlushCounter;
 }

// --------------------------------------------------------------------------

void CLogbook::InitInfos(LOGBOOK_INIT_GUI_INFOS &stInfos, const LOGBOOK_INIT_GUI_INFOS *pstInfos)
 {
  stInfos.bDrawColumnSeparators = FALSE;
  stInfos.bAddToEndOfList       = TRUE;
  stInfos.nMaxLines             = 1000;
  stInfos.bShowFocusRectangle   = TRUE;
  stInfos.bUseDefaultTypeColors = TRUE;
  stInfos.clrList               = -1;
  stInfos.bDateTimeBOLD         = FALSE;

  if (!pstInfos) return;

  stInfos.bDrawColumnSeparators = pstInfos->bDrawColumnSeparators;
  stInfos.bAddToEndOfList       = pstInfos->bAddToEndOfList;
  stInfos.nMaxLines             = pstInfos->nMaxLines;
  stInfos.bShowFocusRectangle   = pstInfos->bShowFocusRectangle;
  stInfos.bUseDefaultTypeColors = pstInfos->bUseDefaultTypeColors;
  stInfos.clrList               = pstInfos->clrList;
  stInfos.bDateTimeBOLD         = pstInfos->bDateTimeBOLD;
 }

// --------------------------------------------------------------------------

void CLogbook::LogbookItemCopy(const LOGBOOKITEM_INFO *psrc, LOGBOOKITEM_INFO &dst)
 {
  dst.clrTimeBackgroundColor = -1;
  dst.clrTimeForegroundColor = -1;
  dst.clrTypeBackgroundColor = -1;
  dst.clrTypeForegroundColor = -1;
  dst.nTextType              = LOGBOOK_NONE;
  dst.nSeparatorType         = LOGBOOK_EMPTYLINE;
  dst.clrTextBackgroundColor = -1;
  dst.clrTextForegroundColor = -1;
  dst.strText                = "";
  
  if (!psrc) return;

  dst.clrTimeBackgroundColor = psrc->clrTimeBackgroundColor;
  dst.clrTimeForegroundColor = psrc->clrTimeForegroundColor;
  dst.clrTypeBackgroundColor = psrc->clrTypeBackgroundColor;
  dst.clrTypeForegroundColor = psrc->clrTypeForegroundColor;
  dst.nTextType              = psrc->nTextType;
  dst.nSeparatorType         = psrc->nSeparatorType;
  dst.clrTextBackgroundColor = psrc->clrTextBackgroundColor;
  dst.clrTextForegroundColor = psrc->clrTextForegroundColor;
  dst.strText                = psrc->strText;
 }

// --------------------------------------------------------------------------
// PUBLIC MEMBER FUNCTIONS
// --------------------------------------------------------------------------

int CLogbook::AddItemLine(const LOGBOOKITEM_INFO &stItemInfo)
 {
  return AddLine(stItemInfo);
 }

// --------------------------------------------------------------------------

int CLogbook::AddLogLine(LPCTSTR lpszText, LOGBOOK_TEXTTYPES nType, COLORREF textColor, ...)
 {
  va_list Args;
  CString str;

  va_start(Args, nType);
  str.FormatV(lpszText, Args);
  va_end(Args);

  LOGBOOKITEM_INFO stItemInfo;

  LogbookItemCopy(NULL, stItemInfo);

  stItemInfo.nTextType = nType;
  stItemInfo.strText   = str;
  stItemInfo.clrTextForegroundColor = textColor;

  return AddLine(stItemInfo);
 }

// --------------------------------------------------------------------------

int CLogbook::AddMemoryLine()
 {
  MEMORYSTATUS stMS;

  GlobalMemoryStatus(&stMS);

  SIZE_T dwTotal = stMS.dwTotalPhys- stMS.dwAvailPhys;

  return AddLogLine("Physical Memory Usage : %lu MB out of %lu MB available", LOGBOOK_INFO, COLORREF( dwTotal / (1024*1024 ) ), stMS.dwTotalPhys / (1024*1024) );
 }

// --------------------------------------------------------------------------

int CLogbook::AddSeparatorLine(LOGBOOK_SEPARATORTYPES nType)
 {
  LOGBOOKITEM_INFO stItemInfo;

  LogbookItemCopy(NULL, stItemInfo);

  stItemInfo.nSeparatorType = nType;

  return AddLine(stItemInfo);
 }

// --------------------------------------------------------------------------

int CLogbook::AddTitleLine(LPCTSTR lpszTitle)
 {
  int nRet = AddSeparatorLine(LOGBOOK_DBLSEPARATOR);

  if (!nRet) return -1;

  LOGBOOKITEM_INFO stItemInfo;

  LogbookItemCopy(NULL, stItemInfo);

  stItemInfo.nTextType = LOGBOOK_INFO;
  stItemInfo.strText   = lpszTitle;

  nRet = AddLine(stItemInfo, 0, FALSE, FALSE);

  if (!nRet) return -1;

  return AddSeparatorLine(LOGBOOK_DBLSEPARATOR);
 }

// --------------------------------------------------------------------------

BOOL CLogbook::AttachFile(LPCTSTR lpszFileName)
 {
  DetachFile();

  if (!m_bInitialized || !m_bFileInfos) return FALSE;
  
  m_pLogbookFile = new CLogbookFile(this, lpszFileName);

  if (!m_pLogbookFile) return FALSE;

  return TRUE;
 }

// --------------------------------------------------------------------------

HWND CLogbook::AttachGUI(CWnd *pParentWnd)
 {
  DetachGUI();
  
  if (!m_bInitialized || !m_bGuiInfos) return FALSE;

  m_pLogbookGUI = new CLogbookGUI(this);

  if (!m_pLogbookGUI) return NULL;

  BOOL bRet = m_pLogbookGUI->Create(pParentWnd);

  if (!bRet)
   {
    delete m_pLogbookGUI;
    m_pLogbookGUI = NULL;
    return FALSE;
   }

  return m_pLogbookGUI->m_hWnd;
 }

// --------------------------------------------------------------------------

void CLogbook::DetachFile()
 {
  if (!m_pLogbookFile) return;

  delete m_pLogbookFile;

  m_pLogbookFile = NULL;
 }

// --------------------------------------------------------------------------

void CLogbook::DetachGUI()
 {
  if (!m_pLogbookGUI) return;

  m_pLogbookGUI->DestroyWindow();

  delete m_pLogbookGUI;

  m_pLogbookGUI = NULL;
 }

// --------------------------------------------------------------------------

LPCTSTR CLogbook::GetPersonalType(LOGBOOK_TEXTTYPES nTextType)
 {
  if (nTextType < LOGBOOK_PERSONAL1) return "";

  return m_strPersonal[nTextType- LOGBOOK_PERSONAL1];
 }

// --------------------------------------------------------------------------

int CLogbook::Initialize(const LOGBOOK_INIT_INFOS &stInfos, const LOGBOOK_INIT_FILE_INFOS *pstFileInfos, const LOGBOOK_INIT_GUI_INFOS *pstGuiInfos)
 {
  if (m_bInitialized)
   {
    TRACE("(CLogbook) Initialize() : Already initialized\n");
    return -1;
   }
  
  m_bFileInfos = (pstFileInfos != NULL);
  m_bGuiInfos  = (pstGuiInfos  != NULL);

  InitInfos(m_stInfos    , &stInfos);
  InitInfos(m_stFileInfos, pstFileInfos);
  InitInfos(m_stGuiInfos , pstGuiInfos);

  if (stInfos.bUseThreadManagement) // start thread (if used)
   {
    ResetEvent(m_hStopEvent);
    
    DWORD nThreadID;

    m_hThread = CreateThread(NULL, 0, ThreadProc, this, 0, &nThreadID);

    if (!m_hThread)
     {
      TRACE("(CLogbook) Initialize() : Failed to create thread management procedure [error code = %lu]\n", GetLastError());
      return -2;
     }

    SetThreadPriority(m_hThread, THREAD_PRIORITY_NORMAL);
   }

  #ifdef _DEBUG
  
  if (stInfos.bCaptureTRACE)
   {
    m_hCrtReportHook = _CrtSetReportHook(LogbookReportHookProc);

    if (m_hCrtReportHook) g_pLogbook = this;
   }

  #endif

  m_bInitialized = TRUE;

  return 1;
 }

// --------------------------------------------------------------------------

void CLogbook::SetPersonalType(LOGBOOK_TEXTTYPES nTextType, LPCTSTR lpszText)
 {
  if (nTextType < LOGBOOK_PERSONAL1) return;

  m_strPersonal[nTextType- LOGBOOK_PERSONAL1] = lpszText;
 }

// --------------------------------------------------------------------------

void CLogbook::SetTypeIcon(LOGBOOK_TEXTTYPES nType, UINT nIconID)
 {
  SetTypeIcon(nType, nIconID ? MAKEINTRESOURCE(nIconID) : NULL);
 }

// --------------------------------------------------------------------------

void CLogbook::SetTypeIcon(LOGBOOK_TEXTTYPES nType, LPCTSTR lpszIconName)
 {
  TEXTTYPE_ICON *pTTI = NULL;
  int I;

  for (I=0; I<m_aIcons.GetSize(); I++)
   {
    TEXTTYPE_ICON *pTTI2 = (TEXTTYPE_ICON *)m_aIcons.GetAt(I);

    if (!pTTI2 || pTTI2->nType != nType) continue;

    pTTI = pTTI2;

    break;
   }

  if (!pTTI && !lpszIconName) return;

  if (!pTTI)
   {
    pTTI = new TEXTTYPE_ICON;

    pTTI->nType = nType;
    pTTI->hIcon = NULL;

    I = (int) m_aIcons.Add(pTTI);
   }

  if (pTTI->hIcon)
   {
    DeleteObject(pTTI->hIcon);

    pTTI->hIcon = NULL;
   }

  if (!lpszIconName)
   {
    m_aIcons.RemoveAt(I);

    delete pTTI; 

    return;
   }

  pTTI->hIcon = (HICON)LoadImage(AfxGetInstanceHandle(), lpszIconName, IMAGE_ICON, 0, 0, 0);

  if (!pTTI->hIcon)
   {
    m_aIcons.RemoveAt(I);

    delete pTTI;
   }
 }

// --------------------------------------------------------------------------

void CLogbook::Uninitialize()
 {
  if (!m_bInitialized) return;

  DetachGUI();
  DetachFile();

  if (m_hCrtReportHook)
   {
    _CrtSetReportHook(NULL);

    m_hCrtReportHook = NULL;

    g_pLogbook = NULL;
   }

  if (m_hThread)
   {
    SetEvent(m_hStopEvent);

    DWORD dwExitCode;

    DWORD nStopCounter = 0;

    CString str;

    while (1)
     {
      if ( GetExitCodeThread(m_hThread,&dwExitCode) && nStopCounter < 20 )
       {
        if (dwExitCode != STILL_ACTIVE) break;
       }
      else {
            TRACE("[LOGBOOK THREAD] force thread 0x%X to stop ...\n", m_hThread);
            TerminateThread(m_hThread, 2);
            break;
           }

      Sleep(100);

      nStopCounter++;
     }

    TRACE("[LOGBOOK THREAD] thread 0x%X stopped.\n", m_hThread);

    CloseHandle(m_hThread);

    m_hThread = NULL;
   }

  while ( m_cThreadList.GetCount() )
   {
    CThreadObject *pThreadObject = (CThreadObject *)m_cThreadList.RemoveHead();

    delete pThreadObject;
   }

  m_bInitialized = FALSE;
 }

// --------------------------------------------------------------------------
