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
// LogbookGUI.cpp
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

#include "LogbookGUI.h"
#include "LogbookGUIItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogbookGUI

CLogbookGUI::CLogbookGUI(CLogbook *pLogbook)
 {
  m_pLogbook                = pLogbook;
  m_bAutoScroll             = TRUE;
  m_nCurrentTimeColumnWidth = 0;
  m_nCurrentTypeColumnWidth = 0;
 }

// --------------------------------------------------------------------------

CLogbookGUI::~CLogbookGUI()
 {
 }

// --------------------------------------------------------------------------
// PROTECTED MEMBER FUNCTIONS
// --------------------------------------------------------------------------

void CLogbookGUI::DrawNormalLine(CDC *pDC, LPDRAWITEMSTRUCT lpDIS, LVITEM *pstLVI, CLogbookGUIItem *pLogbookGUIItem)
 {
  // ----- GET NUMBER OF COLUMNS -----
  
  int nNumberOfColumns = GetHeaderCtrl() ? GetHeaderCtrl()->GetItemCount() : 0;

  // ----- DRAW ALL ITEMS -----
  
  for (int nCol=0; nCol<nNumberOfColumns; nCol++)
   {
    // COMPUTE CELL BACKGROUND COLOR
    
    COLORREF clrBackground = pLogbookGUIItem->GetBackgroundColor(nCol);

    if (clrBackground == -1) clrBackground = GetBkColor();

    if (pstLVI->state & LVIS_SELECTED && (!nCol || GetExtendedStyle() & LVS_EX_FULLROWSELECT)) clrBackground = GetSysColor(COLOR_HIGHLIGHT);

    // COMPUTE CELL TEXT COLOR
    
    COLORREF clrText = pLogbookGUIItem->GetForegroundColor(nCol);

    if (clrText == -1) clrText = GetTextBkColor();

    if (pstLVI->state & LVIS_SELECTED && (!nCol || GetExtendedStyle() & LVS_EX_FULLROWSELECT)) clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);

    // GET CELL RECTANGLE
    
    CRect rectCell, rectIconCell;

    rectIconCell.SetRectEmpty();

    if (!nCol)
     {
      GetSubItemRect(lpDIS->itemID, nCol, LVIR_ICON, rectIconCell);

      if (rectIconCell.left < rectIconCell.right)
       {
        // fill icon cell background color
        
        pDC->FillSolidRect(&rectIconCell, clrBackground);

        HICON hIcon = m_pLogbook->GetTypeIcon( pLogbookGUIItem->m_nTextType );

        // draw icon
        
        if ( hIcon && pLogbookGUIItem->IsFirstLine() )
         pDC->DrawState(CPoint(rectIconCell.left,rectIconCell.top),
                        CSize(rectIconCell.Width(),rectIconCell.Height()),
                        hIcon,
                        DST_ICON | DSS_NORMAL,
                        (HBRUSH)NULL);
       }
     }

    GetSubItemRect(lpDIS->itemID, nCol, LVIR_LABEL, rectCell);

    if ( !rectIconCell.IsRectEmpty() && !m_pLogbook->IconsPresent() ) rectCell.left = rectIconCell.left;

    // FILL CELL BACKGROUND COLOR
    
    pDC->FillSolidRect(&rectCell, clrBackground);

    // SET CELL TEXT COLOR
    
    COLORREF clrTextOld = pDC->SetTextColor(clrText);

    rectCell.InflateRect(-2, 0, -2, 0);

    // DRAW CELL TEXT

    CFont *pPrevFont = NULL;

    if (!nCol && m_pLogbook->m_stGuiInfos.bDateTimeBOLD)
     pPrevFont = pDC->SelectObject( &m_cLogbookFont[1] );
    else
     pPrevFont = pDC->SelectObject( &m_cLogbookFont[0] );

    pDC->DrawText(pLogbookGUIItem->GetText(nCol), -1, &rectCell, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
    
    pDC->SelectObject(pPrevFont);

    // DRAW COLUMN SEPARATOR
    
    if (m_pLogbook->m_stGuiInfos.bDrawColumnSeparators && nCol < nNumberOfColumns - 1)
     {
      pDC->MoveTo(rectCell.right + 1, rectCell.top);
      pDC->LineTo(rectCell.right + 1, rectCell.bottom);
     }

    // RESTORE OLD CELL TEXT COLOR
    
    pDC->SetTextColor(clrTextOld);
   } 
 }

// --------------------------------------------------------------------------

void CLogbookGUI::DrawSeparatorLine(CDC *pDC, LPDRAWITEMSTRUCT lpDIS, LVITEM *pstLVI, CLogbookGUIItem *pLogbookGUIItem)
 {
  // ----- GET NUMBER OF COLUMNS -----
  
  int nNumberOfColumns = GetHeaderCtrl() ? GetHeaderCtrl()->GetItemCount() : 0;

  // ----- FILL ALL BACKGROUND COLORS -----
  
  for (int nCol=0; nCol<nNumberOfColumns; nCol++)
   {
    // COMPUTE CELL BACKGROUND COLOR
    
    COLORREF clrBackground = pLogbookGUIItem->GetBackgroundColor(nCol);

    if (clrBackground == -1) clrBackground = GetBkColor();

    if (pstLVI->state & LVIS_SELECTED && (!nCol || GetExtendedStyle() & LVS_EX_FULLROWSELECT)) clrBackground = GetSysColor(COLOR_HIGHLIGHT);

    // COMPUTE CELL LINE COLOR
    
    COLORREF clrText = pLogbookGUIItem->GetForegroundColor(nCol);

    if (clrText == -1) clrText = GetTextBkColor();

    if (pstLVI->state & LVIS_SELECTED && (!nCol || GetExtendedStyle() & LVS_EX_FULLROWSELECT)) clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);

    // GET CELL RECTANGLE
    
    CRect rectCell, rectIconCell;

    rectIconCell.SetRectEmpty();

    if (!nCol)
     {
      GetSubItemRect(lpDIS->itemID, nCol, LVIR_ICON, rectIconCell);

      if (rectIconCell.left < rectIconCell.right)
       {
        // fill icon cell background color
        
        pDC->FillSolidRect(&rectIconCell, clrBackground);
       }
     }

    GetSubItemRect(lpDIS->itemID, nCol, LVIR_LABEL, rectCell);

    if ( !rectIconCell.IsRectEmpty() ) rectCell.left = rectIconCell.left;

    // FILL CELL BACKGROUND COLOR
    
    pDC->FillSolidRect(&rectCell, clrBackground);

    // CREATE NEEDED PEN
    
    CPen cPen;

    switch( pLogbookGUIItem->m_nSeparatorType )
     {
      case LOGBOOK_SEPARATOR           : cPen.CreatePen(PS_SOLID     , 1, clrText); break;
      case LOGBOOK_DBLSEPARATOR        : cPen.CreatePen(PS_SOLID     , 3, clrText); break;
      case LOGBOOK_DASHSEPARATOR       : cPen.CreatePen(PS_DASH      , 1, clrText); break;
      case LOGBOOK_DOTSEPARATOR        : cPen.CreatePen(PS_DOT       , 1, clrText); break;
      case LOGBOOK_DASHDOTSEPARATOR    : cPen.CreatePen(PS_DASHDOT   , 1, clrText); break;
      case LOGBOOK_DASHDOTDOTSEPARATOR : cPen.CreatePen(PS_DASHDOTDOT, 1, clrText); break;
     }

    // DRAW SEPARATOR LINE
    
    if ( cPen.GetSafeHandle() )
     {
      CPen *pOldPen = pDC->SelectObject(&cPen);

      pDC->MoveTo(rectCell.left , rectCell.top + rectCell.Height() / 2);
      pDC->LineTo(rectCell.right, rectCell.top + rectCell.Height() / 2);

      pDC->SelectObject(pOldPen);
     }
   }
 }

// --------------------------------------------------------------------------

int CLogbookGUI::Init()
 {
  CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();

  if ( pHeaderCtrl && pHeaderCtrl->GetItemCount() ) return 0; // already initialized

  // ----- CREATE LISTCTRL FONTS -----

  LOGFONT stLogFont;

  GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(stLogFont), &stLogFont);

  strcpy_s(stLogFont.lfFaceName, LF_FACESIZE, "Courier New");

  BOOL bRet = m_cLogbookFont[0].CreateFontIndirect(&stLogFont);

  if (!bRet)
   {
    TRACE("(CLogbookCtrl) Init() : Failed to create logbook font !");
    return -1;
   }

  stLogFont.lfWeight = FW_BOLD;

  bRet = m_cLogbookFont[1].CreateFontIndirect(&stLogFont);

  if (!bRet)
   {
    TRACE("(CLogbookCtrl) Init() : Failed to create logbook font !");
    return -2;
   }

  SetFont(&m_cLogbookFont[0], FALSE);

  // ----- CREATE COLUMNS -----

  int nRet = InsertColumn(0, "Date / Time", LVCFMT_LEFT, 150);
  
  if (nRet == -1)
   {
    TRACE("(CLogbookCtrl) Init() : Failed to insert date/time column !");
    return -3;
   }

  nRet = InsertColumn(1, "Type"       , LVCFMT_LEFT, 100);
  
  if (nRet == -1)
   {
    TRACE("(CLogbookCtrl) Init() : Failed to insert type column !");
    return -4;
   }

  nRet = InsertColumn(2, "Description", LVCFMT_LEFT, 200);

  if (nRet == -1)
   {
    TRACE("(CLogbookCtrl) Init() : Failed to insert description column !");
    return -5;
   }

  // ----- CREATE IMAGELIST -----
  
  bRet = m_cImageList.Create(16, 16, ILC_COLOR, 24, 1);

  if (!bRet)
   {
    TRACE("(CLogbookCtrl) Init() : Failed to create image list !");
    return -6;
   }

  SetImageList(&m_cImageList, LVSIL_SMALL);

  // ----- RECALC COLUMNS -----

  RecalcColumns(10);

  // ----- SET SOME STYLES -----

  SetExtendedStyle(LVS_EX_FULLROWSELECT);

  // ----- END -----

  return 1; // OK
 }

// --------------------------------------------------------------------------

void CLogbookGUI::RecalcColumns(int nPixelAdd)
 {
  SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
  SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
  
  if (nPixelAdd > 0)
   {
    SetColumnWidth(0, GetColumnWidth(0) + 10);
    SetColumnWidth(1, GetColumnWidth(1) + 10);
   }

  SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
 }

// --------------------------------------------------------------------------
// PUBLIC MEMBER FUNCTIONS
// --------------------------------------------------------------------------

int CLogbookGUI::AddLine(const LOGBOOKITEM_INFO &stInfo, const SYSTEMTIME &stST, BOOL bDisplayTime, BOOL bDisplayType)
 {
  if (!m_hWnd) return -1;

  Init();

  SetRedraw(FALSE);

  int nItemLine = GetGUIInfos()->bAddToEndOfList ? GetItemCount() : 0;

  if (stInfo.nTextType != LOGBOOK_NONE)
   {
   int length = stInfo.strText.GetLength() + 1;
   char *szText = new char [ length ];

    strcpy_s(szText, length, stInfo.strText);

    BOOL  bFirstLine = TRUE;
    char *next;
    char *sz         = strtok_s(szText, "\n", &next);

    while (sz)
     {
      CLogbookGUIItem *pLogbookGUIItem = new CLogbookGUIItem(m_pLogbook, stST, stInfo.nTextType, stInfo.nSeparatorType);

      pLogbookGUIItem->m_clrTimeBackgroundColor = stInfo.clrTimeBackgroundColor;
      pLogbookGUIItem->m_clrTimeForegroundColor = stInfo.clrTimeForegroundColor;

      pLogbookGUIItem->m_clrTypeBackgroundColor = stInfo.clrTypeBackgroundColor;
      pLogbookGUIItem->m_clrTypeForegroundColor = stInfo.clrTypeForegroundColor;
     
      pLogbookGUIItem->m_clrTextBackgroundColor = stInfo.clrTextBackgroundColor;
      pLogbookGUIItem->m_clrTextForegroundColor = stInfo.clrTextForegroundColor;

      pLogbookGUIItem->m_bDisplayTime = bDisplayTime;
      pLogbookGUIItem->m_bDisplayType = bDisplayType;

      if (!bFirstLine)
       {
        pLogbookGUIItem->m_bDisplayTime = FALSE;
        pLogbookGUIItem->m_bDisplayType = FALSE;
       }

      pLogbookGUIItem->m_strText = sz;
      
      nItemLine = GetGUIInfos()->bAddToEndOfList ? GetItemCount() : 0;

      nItemLine = InsertItem(nItemLine, m_pLogbook->GetStringTime(stST), 0);

      SetItemData(nItemLine, (DWORD_PTR)pLogbookGUIItem);

      SetItemText(nItemLine, 1, m_pLogbook->GetStringType(stInfo.nTextType));

      SetItemText(nItemLine, 2, pLogbookGUIItem->m_strText);

      Update(nItemLine);

      sz = strtok_s(NULL, "\n", &next);

      bFirstLine = FALSE;
     }

    delete [] szText;
   }
  else {
        CLogbookGUIItem *pLogbookGUIItem = new CLogbookGUIItem(m_pLogbook, stST, stInfo.nTextType, stInfo.nSeparatorType);
        
        nItemLine = InsertItem(nItemLine, " ");

        SetItemData(nItemLine, (DWORD_PTR)pLogbookGUIItem);

        Update(nItemLine);
       }

  CDC *pDC = GetDC();

  CFont *pOldFont = NULL;
  
  if (m_pLogbook->m_stGuiInfos.bDateTimeBOLD)
   pOldFont = pDC->SelectObject( &m_cLogbookFont[1] );
  else
   pOldFont = pDC->SelectObject( &m_cLogbookFont[0] );
  
  CSize sizeTimeText = pDC->GetTextExtent( m_pLogbook->GetStringTime(stST) );

  pDC->SelectObject( &m_cLogbookFont[0] );

  CSize sizeTypeText = pDC->GetTextExtent( m_pLogbook->GetStringType(stInfo.nTextType) );

  sizeTimeText.cx += 10;

  sizeTypeText.cx += 10;

  pDC->SelectObject(pOldFont);

  ReleaseDC(pDC);

  if (m_nCurrentTimeColumnWidth < sizeTimeText.cx || m_nCurrentTypeColumnWidth < sizeTypeText.cx)
   {
    RecalcColumns(10);

    m_nCurrentTimeColumnWidth = GetColumnWidth(0);

    m_nCurrentTypeColumnWidth = GetColumnWidth(1);
   }

  SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);

  if (m_bAutoScroll) EnsureVisible(nItemLine, FALSE);

  SetRedraw(TRUE);

  if ( nItemLine >= GetGUIInfos()->nMaxLines ) DeleteItem(GetGUIInfos()->bAddToEndOfList ? 0 : GetItemCount() - 1);

  return 1;
 }

// --------------------------------------------------------------------------

BOOL CLogbookGUI::Create(CWnd *pParentWnd, UINT nID)
 {
  DWORD dwStyle = WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOSORTHEADER | LVS_NOCOLUMNHEADER | LVS_OWNERDRAWFIXED;

  BOOL bRet = CListCtrl::CreateEx(WS_EX_CLIENTEDGE, dwStyle, CRect(0,0,0,0), pParentWnd, nID);

  if (!bRet) TRACE("(CLogbookGUI) Create() : Failed to create logbook control !");

  return bRet;
 }

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CLogbookGUI, CListCtrl)
	//{{AFX_MSG_MAP(CLogbookGUI)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_CONTEXTMENU()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteItem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogbookGUI message handlers

void CLogbookGUI::DrawItem(LPDRAWITEMSTRUCT lpDIS)
 {
  if (lpDIS->CtlType != ODT_LISTVIEW) return;

  CDC *pDC = CDC::FromHandle(lpDIS->hDC);

  // ----- SAVE DC -----

  int nSaveDC = pDC->SaveDC();

  // ----- GET ITEM STATE INFORMATION -----

  LVITEM stLVI;

  stLVI.mask      = LVIF_STATE;
  stLVI.iItem     = lpDIS->itemID;
  stLVI.iSubItem  = 0;
  stLVI.stateMask = (UINT)-1; // Get all state flags

  GetItem(&stLVI);

  // ----- GET LINE INFO -----

  CLogbookGUIItem *pLogbookGUIItem = (CLogbookGUIItem *)GetItemData(lpDIS->itemID);

  // ----- DRAW LINE -----

  if (pLogbookGUIItem)
   {
    if ( pLogbookGUIItem->IsSeparatorLine() ) DrawSeparatorLine(pDC, lpDIS, &stLVI, pLogbookGUIItem);

    else DrawNormalLine(pDC, lpDIS, &stLVI, pLogbookGUIItem);;
   }

  // ----- DRAW FOCUS RECTANGLE IF ITEM HAS FOCUS -----

  if (m_pLogbook->m_stGuiInfos.bShowFocusRectangle && stLVI.state & LVIS_FOCUSED && GetFocus() == this)
   {
    CRect rectFocus;

    GetItemRect(lpDIS->itemID, &rectFocus, (GetExtendedStyle() & LVS_EX_FULLROWSELECT) ? LVIR_BOUNDS : LVIR_LABEL);

    rectFocus.left  += 2;
    rectFocus.right -= 2;

    pDC->DrawFocusRect(&rectFocus);
   }

  // ----- RESTORE DC -----
  
  pDC->RestoreDC(nSaveDC);
 }

// --------------------------------------------------------------------------

void CLogbookGUI::PreSubclassWindow() 
 {
	CListCtrl::PreSubclassWindow();
 }

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

BOOL CLogbookGUI::OnCommand(WPARAM wParam, LPARAM lParam) 
 {
  UINT nID         = LOWORD(wParam);
  UINT nNotifyCode = HIWORD(wParam);
  HWND hCtrlWnd    = (HWND)lParam;

  // ........................................................................

  if (nID == 30000) // CLEAR LOGBOOK
   {
    DeleteAllItems();
    
    return TRUE;
   }

  // ........................................................................

  else if (nID == 30001) // CLEAR SELECTIONS
   {
    SetRedraw(FALSE);
    
    int nIndex = GetNextItem(-1, LVNI_SELECTED);

    while (nIndex != -1)
     {
      DeleteItem(nIndex);
      
      nIndex = GetNextItem(-1, LVNI_SELECTED);
     }

    SetRedraw(TRUE);

    Invalidate();

    return TRUE;
   }

  // ........................................................................

  else if (nID == 30002) // AUTO SCROLL
   {
    m_bAutoScroll = !m_bAutoScroll;
    
    return TRUE;
   }

  // ........................................................................

  else if (nID == 30002) // Write To File
   {
    return TRUE;
   }


  else if (nID == 30003) // PRINT
   {
    return TRUE;
   }



  // ........................................................................

  return CListCtrl::OnCommand(wParam, lParam);
 }

// --------------------------------------------------------------------------

void CLogbookGUI::OnContextMenu(CWnd *pWnd, CPoint point)
 {
  static BOOL st_bTrackMenu = FALSE;

  if (st_bTrackMenu)
   {
    st_bTrackMenu = FALSE;

    SendMessage(WM_CANCELMODE);

    ScreenToClient(&point);

    PostMessage(WM_CONTEXTMENU, (WPARAM)pWnd->GetSafeHwnd(), MAKELPARAM(point.x,point.y));

    return;
   }

  CMenu cContextMenu;

  BOOL bRet = cContextMenu.CreatePopupMenu();

  if (!bRet) return;

  cContextMenu.AppendMenu(MF_STRING | (m_bAutoScroll ? MF_CHECKED : MF_UNCHECKED), 30002, "Auto Scroll");

  cContextMenu.AppendMenu(MF_SEPARATOR);

  if (GetItemCount() > 0) cContextMenu.AppendMenu(MF_STRING, 30000, "Clear Logbook");

  if (GetSelectedCount() > 0) cContextMenu.AppendMenu(MF_STRING, 30001, "Clear Selection");

  if (cContextMenu.GetMenuItemCount() > 2) cContextMenu.AppendMenu(MF_SEPARATOR);

  cContextMenu.AppendMenu(MF_STRING | MF_DISABLED | MF_GRAYED, 30002, "Write To File");
  cContextMenu.AppendMenu(MF_STRING | MF_DISABLED | MF_GRAYED, 30003, "Print");

  st_bTrackMenu = TRUE;

  cContextMenu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);

  st_bTrackMenu = FALSE;
 }

// --------------------------------------------------------------------------

int CLogbookGUI::OnCreate(LPCREATESTRUCT lpCreateStruct) 
 {
  if ( CListCtrl::OnCreate(lpCreateStruct) == -1 ) return -1;

  if (m_pLogbook->m_stGuiInfos.clrList != -1) SetBkColor(m_pLogbook->m_stGuiInfos.clrList);
	
  return 0;
 }

// --------------------------------------------------------------------------

void CLogbookGUI::OnDestroy() 
 {
  CListCtrl::OnDestroy();
	
 }

// --------------------------------------------------------------------------

BOOL CLogbookGUI::OnEraseBkgnd(CDC *pDC)
 {
  //CRect rect; GetClientRect(&rect); pDC->FillSolidRect(&rect, RGB(0,0,255)); return TRUE;
	
  return CListCtrl::OnEraseBkgnd(pDC);
 }

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

void CLogbookGUI::OnDeleteItem(NMHDR *pNMHDR, LRESULT *pResult)
 {
  NM_LISTVIEW *pNMListView = (NM_LISTVIEW *)pNMHDR;
	
  *pResult = 0;

  CLogbookGUIItem *pLogbookGUIItem = (CLogbookGUIItem *)GetItemData(pNMListView->iItem);

  if (pLogbookGUIItem) delete pLogbookGUIItem;
 }

// --------------------------------------------------------------------------
