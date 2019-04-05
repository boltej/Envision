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
// XHTMLTipOfTheDayDlg.cpp  Version 1.0
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// History
//     Version 1.0 - 2004 June 15
//     - Initial public release
//
// License:
//     This software is released into the public domain.  You are free to use
//     it in any way you like, except that you may not sell this source code.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     I accept no liability for any damage or loss of business that this 
//     software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TipDlg.h"
#include "resource.h"
#include <sys\stat.h>
#include <sys\types.h>
//#include "Shlwapi.h."		// for PathCompactPath()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#pragma message("automatic link to SHLWAPI.LIB")
//#pragma comment(lib, "Shlwapi.lib")


#define MAX_BUFLEN 10000

static const TCHAR szSection[]    = _T("TipOfTheDay");
static const TCHAR szIntTipNo[]   = _T("TipNo");
static const TCHAR szTimeStamp[]  = _T("TimeStamp");
static const TCHAR szIntStartup[] = _T("StartUp");


///////////////////////////////////////////////////////////////////////////////
// CXHTMLTipOfTheDayDlg dialog

BEGIN_MESSAGE_MAP(CXHTMLTipOfTheDayDlg, CDialog)
	//{{AFX_MSG_MAP(CXHTMLTipOfTheDayDlg)
	ON_BN_CLICKED(IDC_NEXTTIP, OnNextTip)
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_PREVTIP, OnPrevTip)
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// ctor
CXHTMLTipOfTheDayDlg::CXHTMLTipOfTheDayDlg(LPCTSTR lpszTipFile, CWnd* pParent /*=NULL*/)
: CDialog( MAKEINTRESOURCE(IDD_TIPOFTHEDAY), pParent)
{
	//{{AFX_DATA_INIT(CXHTMLTipOfTheDayDlg)
	//}}AFX_DATA_INIT
	m_bVisible   = TRUE;
	m_pStream    = NULL;
	m_nTipNo     = 0;
	m_nLastTipNo = 0;
	ASSERT(lpszTipFile);
	m_strTipFile = lpszTipFile;
	m_SideBrush.CreateSolidBrush(::GetSysColor(COLOR_BTNSHADOW));

	CWinApp* pApp = AfxGetApp();
	m_bStartup = pApp->GetProfileInt(szSection, szIntStartup, TRUE);
}

///////////////////////////////////////////////////////////////////////////////
// dtor
CXHTMLTipOfTheDayDlg::~CXHTMLTipOfTheDayDlg()
{
	if (m_SideBrush.GetSafeHandle())
		m_SideBrush.DeleteObject();
}
  
///////////////////////////////////////////////////////////////////////////////
// DoDataExchange
void CXHTMLTipOfTheDayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CXHTMLTipOfTheDayDlg)
	DDX_Control(pDX, IDC_NEXTTIP,   m_btnNext);
	DDX_Control(pDX, IDC_PREVTIP,   m_btnPrev);
	DDX_Control(pDX, IDC_SIDE,      m_Side);
	DDX_Control(pDX, IDC_HEADER,    m_Header);
	DDX_Control(pDX, IDC_TIPSTRING, m_Tip);
	DDX_Control(pDX, IDC_STARTUP,   m_chkStartup);
	//}}AFX_DATA_MAP
}

///////////////////////////////////////////////////////////////////////////////
// OnInitDialog
BOOL CXHTMLTipOfTheDayDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
   CenterWindow();

	// set up glyph buttons
	m_btnPrev.SetGlyph(16, _T("Wingdings"), 0xEF);
	m_btnNext.SetGlyph(16, _T("Wingdings"), 0xF0);

	if (m_bStartup)
		m_bVisible = TRUE;		// OK to show
	else
		m_bVisible = FALSE;		// user doesn't want to see - hide the window
								// until OnWindowPosChanging() can send WM_CLOSE

	m_chkStartup.SetCheck(m_bStartup);

	m_nTipNo = AfxGetApp()->GetProfileInt(szSection, szIntTipNo, 0);
	if (m_nTipNo > 0)
		m_nTipNo++;

	// get path for tips file =============================

	TCHAR szPath[_MAX_PATH*3] = { 0 };

   /*
	HINSTANCE hinst = AfxGetInstanceHandle();
	::GetModuleFileName(hinst, szPath, sizeof(szPath)/sizeof(TCHAR)-2);
	TCHAR * cp = _tcsrchr(szPath, _T('\\'));
	if (cp)
		*cp = _T('\0');

   char *debug = strstr( szPath, "\\Debug" );
   if ( debug )
      *debug = NULL;
*/
	CString strTipPath(szPath);
	//strTipPath += _T("\\");
  

	if (!m_strTipFile.IsEmpty())
		strTipPath = m_strTipFile;
	else
		strTipPath = _T("Tips.txt");

	CString strStoredTime = _T("");
	CString strModifyTime = _T("");

	BOOL bFirstTime = FALSE;

	// open tips file =====================================

	_tfopen_s( &m_pStream, strTipPath, _T("r"));
	if (m_pStream == NULL) 
	{
		// can't open tips file - display filepath
		CString str = _T("");;
		str.LoadString(IDS_TIP_FILE_MISSING);
		strTipPath.MakeUpper();
		LPTSTR lpch = strTipPath.GetBuffer(MAX_PATH);
		CClientDC dc(this);
		CRect rectClient;
		GetDlgItem(IDC_TIPSTRING)->GetClientRect(&rectClient);

		// get shortened name that will fit in client rect
		if (::PathCompactPath(dc.m_hDC, lpch, rectClient.Width()-4))
			m_strTip.Format(str, lpch);
		else
			m_strTip.Format(str, strTipPath);
		strTipPath.ReleaseBuffer();
	}
	else
	{
		// tips file opened ok

		// If the saved timestamp is different from the current timestamp of
		// the tips file, then we know that the tips file has been modified.
		// Reset the file position to 0 and save the latest timestamp.

		struct _stat buf;
		_fstat(_fileno(m_pStream), &buf);
		strModifyTime = ctime(&buf.st_mtime);
		strModifyTime.TrimRight();

		strStoredTime = AfxGetApp()->GetProfileString(szSection, szTimeStamp);

		AfxGetApp()->WriteProfileString(szSection, szTimeStamp, strModifyTime);

		// tip file was modified, start at first tip
		if (!strStoredTime.IsEmpty())
		{
			if (strModifyTime != strStoredTime)
			{
				m_nTipNo = 1;
			}
		}
		else
		{
			// this is the first time that a tip is being displayed
			bFirstTime = TRUE;
			m_nTipNo = 0;
		}

		m_nLastTipNo = GetLastTipNo();

		GetTipString(&m_nTipNo, m_strTip, bFirstTime);

		// if Tips file does not exist then disable NextTip
		if (m_pStream == NULL)
		{
			GetDlgItem(IDC_NEXTTIP)->EnableWindow(FALSE);
			GetDlgItem(IDC_PREVTIP)->EnableWindow(FALSE);
		}
	}

	// display the header =================================
	m_Header.SetMargins(10, 10);
	int nHeader = 0;
	if (!strStoredTime.IsEmpty())
		nHeader = 1;
	DisplayHeader(nHeader);

	// display the tip ====================================
	m_Tip.SetMargins(10, 20);

	// set font a bit larger
	LOGFONT lf;
	CFont* pFont = GetFont();
	if (pFont)
		pFont->GetObject(sizeof(lf), &lf);
	else
		GetObject(GetStockObject(SYSTEM_FONT), sizeof(lf), &lf);
	if (lf.lfHeight < 0)
		lf.lfHeight -= 2;
	else
		lf.lfHeight += 2;
	m_Tip.SetLogFont(&lf);

	m_Tip.SetWindowText(m_strTip);

	return TRUE;  // return TRUE unless you set the focus to a control
}

///////////////////////////////////////////////////////////////////////////////
// OnNextTip
void CXHTMLTipOfTheDayDlg::OnNextTip()
{
	if (m_nTipNo == 0)
		DisplayHeader(1);
	m_nTipNo++;
	if (m_nTipNo > m_nLastTipNo)
		m_nTipNo = 0;
	if (GetTipString(&m_nTipNo, m_strTip, FALSE))
		m_Tip.SetWindowText(m_strTip);
}

///////////////////////////////////////////////////////////////////////////////
// OnPrevTip
void CXHTMLTipOfTheDayDlg::OnPrevTip() 
{
	if (m_nTipNo == 0)
		DisplayHeader(1);
	m_nTipNo--;
	if (m_nTipNo <= 0)
		m_nTipNo = m_nLastTipNo;
	if (GetTipString(&m_nTipNo, m_strTip, FALSE))
		m_Tip.SetWindowText(m_strTip);
}

///////////////////////////////////////////////////////////////////////////////
// DisplayHeader
void CXHTMLTipOfTheDayDlg::DisplayHeader(int nHeader) 
{
	static const TCHAR * szHeader = 
	{
		_T("<br>")
		_T("<b><font size=\"+8\" face=\"Times New Roman\">%s</font></b>")
		_T("<br><font color=\"%d,%d,%d\"><hr size=1></font>")
	};
	
	CString strText = _T("");

	if (nHeader == 0)
		VERIFY(strText.LoadString(IDS_WELCOME));
	else
		VERIFY(strText.LoadString(IDS_DIDYOUKNOW));

	// format the header with the rgb color for the rule
	CString strHeader = _T("");
	COLORREF rgb = ::GetSysColor(COLOR_BTNSHADOW);
	strHeader.Format(szHeader, strText, 
		GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
	m_Header.SetWindowText(strHeader);
}

///////////////////////////////////////////////////////////////////////////////
// GetTipLine
TCHAR * CXHTMLTipOfTheDayDlg::GetTipLine(TCHAR *buffer, int n)
{
	ASSERT(buffer);

	while (_fgetts(buffer, n-1, m_pStream) != NULL)
	{
		if (*buffer != _T(' ')  && *buffer != _T('\t') && 
			*buffer != _T('\n') && *buffer != _T(';')) 
		{
			// There should be no space at the beginning of the tip
			// This behavior is same as VC++ Tips file
			// Comment lines are ignored and they start with a semicolon
			return buffer;
		}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// GetLastTipNo
int CXHTMLTipOfTheDayDlg::GetLastTipNo()
{
	int nTipNo = 0;

	if (m_pStream)
	{
		CString str = _T("");
		LPTSTR lpsz = str.GetBuffer(MAX_BUFLEN);
		*lpsz = _T('\0');

		fseek(m_pStream, 0, SEEK_SET);	// rewind file

		for ( ; ; nTipNo++)
		{
			if (GetTipLine(lpsz, MAX_BUFLEN-1) == NULL) 
				break;
		}
		if (nTipNo > 0)
			nTipNo--;
	}

	return nTipNo;
}

///////////////////////////////////////////////////////////////////////////////
// GetTipString
BOOL CXHTMLTipOfTheDayDlg::GetTipString(int* pnTip, CString& str, BOOL bFirstTime)
{
	if (!bFirstTime && (*pnTip < 1))
		*pnTip = 1;		// don't show welcome tip again

	LPTSTR lpsz = str.GetBuffer(MAX_BUFLEN);
	*lpsz = _T('\0');

	if (m_pStream)
	{
		fseek(m_pStream, 0, SEEK_SET);	// rewind file
		int n = 0;

		do
		{
			if (GetTipLine(lpsz, MAX_BUFLEN-1) == NULL) 
			{
				// We have either reached EOF or enocuntered some problem
				// In both cases reset the pointer to the beginning of the file
				// This behavior is same as VC++ Tips file
				fseek(m_pStream, 0, SEEK_SET);	// rewind file

				*pnTip = 1;

				// skip the welcome tip
				if (GetTipLine(lpsz, MAX_BUFLEN-1) != NULL) 
				{
					// start at second tip
					GetTipLine(lpsz, MAX_BUFLEN-1);
				}
				break;
			} 
			else 
			{
				// There should be no space at the beginning of the tip
				// This behavior is same as VC++ Tips file
				// Comment lines are ignored and they start with a semicolon
				n++;
			}

		}  while (n <= *pnTip);
	}

	str.ReleaseBuffer();

	return !str.IsEmpty();
}

///////////////////////////////////////////////////////////////////////////////
// OnCtlColor
HBRUSH CXHTMLTipOfTheDayDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (pWnd && ::IsWindow(pWnd->m_hWnd))
	{
		if (pWnd->m_hWnd == m_Tip.m_hWnd)
			return (HBRUSH)GetStockObject(WHITE_BRUSH);
		if (pWnd->m_hWnd == m_Header.m_hWnd)
			return (HBRUSH)GetStockObject(WHITE_BRUSH);
		if (pWnd->m_hWnd == m_Side.m_hWnd)
			return (HBRUSH)GetStockObject(NULL_BRUSH);
	}

	return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

///////////////////////////////////////////////////////////////////////////////
// OnPaint
void CXHTMLTipOfTheDayDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	DoPaint(&dc);

	// Do not call CDialog::OnPaint() for painting messages
}

///////////////////////////////////////////////////////////////////////////////
// DoPaint
void CXHTMLTipOfTheDayDlg::DoPaint(CDC *pdc)
{
	CWnd* pStatic;
	CRect  rectBulb, rectSide, rectHeader;

	pStatic = GetDlgItem(IDC_LIGHTBULB);
	if (!pStatic)
	{
		ASSERT(FALSE);
		return;
	}
	pStatic->GetWindowRect(rectBulb);
	ScreenToClient(rectBulb);
	pStatic->ShowWindow(SW_HIDE);

	pStatic = GetDlgItem(IDC_SIDE);
	if (!pStatic)
	{
		ASSERT(FALSE);
		return;
	}
	pStatic->GetWindowRect(rectSide);
	ScreenToClient(rectSide);
	pStatic->ShowWindow(SW_HIDE);

	pStatic = GetDlgItem(IDC_HEADER);
	if (!pStatic)
	{
		ASSERT(FALSE);
		return;
	}
	pStatic->GetWindowRect(rectHeader);
	ScreenToClient(rectHeader);

	// draw border
	CRect rectBorder(rectSide.left-2, rectHeader.top-2, 
		rectHeader.right+2, rectSide.bottom+2);
	pdc->DrawEdge(&rectBorder, EDGE_SUNKEN, BF_RECT);

	// paint left side button shadow color
	pdc->FillSolidRect(rectSide, ::GetSysColor(COLOR_BTNSHADOW));	
	pdc->SetBkColor(::GetSysColor(COLOR_BTNSHADOW));

	// load bitmap and get dimensions of the bitmap
	CBitmap bmp;
	VERIFY(bmp.LoadBitmap(IDB_LIGHTBULB));
	BITMAP bmpInfo;	
	bmp.GetBitmap(&bmpInfo);

	// draw bitmap in top corner and validate only top portion of window
	CDC dcTmp;
	dcTmp.CreateCompatibleDC(pdc);
	dcTmp.SelectObject(&bmp);
	rectBulb.bottom = bmpInfo.bmHeight + rectBulb.top;
	rectBulb.right = bmpInfo.bmWidth + rectBulb.left;
	dcTmp.SelectObject(&m_SideBrush);
	dcTmp.ExtFloodFill(0,0, 0x00FFFFFF, FLOODFILLSURFACE);
	pdc->BitBlt(rectBulb.left, rectBulb.top, 
				rectBulb.Width(), rectBulb.Height(), 
				&dcTmp, 0, 0, SRCCOPY);
}

///////////////////////////////////////////////////////////////////////////////
// OnWindowPosChanging
void CXHTMLTipOfTheDayDlg::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos) 
{
	if (!m_bVisible)
	{
		lpwndpos->flags &= ~SWP_SHOWWINDOW ;
		SendMessage(WM_CLOSE);
	}

	CDialog::OnWindowPosChanging(lpwndpos);
}

///////////////////////////////////////////////////////////////////////////////
// OnDestroy
void CXHTMLTipOfTheDayDlg::OnDestroy() 
{
	// Update the tip number in the ini file (or registry) with
	// the latest position so we don't repeat the tips.
 
	m_bStartup = m_chkStartup.GetCheck();

	if (m_pStream) 
	{
		fclose(m_pStream);
		m_pStream = NULL;
		AfxGetApp()->WriteProfileInt(szSection, szIntTipNo, m_nTipNo);
		AfxGetApp()->WriteProfileInt(szSection, szIntStartup, m_bStartup);
	}

	CDialog::OnDestroy();
}
