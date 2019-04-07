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
// A flicker-free resizing dialog class
// Copyright (c) 1999 Andy Brown <andy@mirage.dabsol.co.uk>
// You may do whatever you like with this file, I just don't care.

/* ResizeDlg.cpp
 *
 * Derived from 1999 Andy Brown <andy@mirage.dabsol.co.uk> by Robert Python Apr 2004 BJ
 * goto http://www.codeproject.com/dialog/resizabledialog.asp for Andy Brown's
 * article "Flicker-Free Resizing Dialog".
 *
 * 1, Enhanced by Robert Python (RobertPython@163.com, support@panapax.com, www.panapax.com) Apr 2004.
 *    In this class, add the following features:
 *    (1) controls reposition and/or resize direction(left-right or top-bottom) sensitive;
 *    (2) add two extra reposition/resize type: ZOOM and DELTA_ZOOM, which is useful in there
 *        are more than one controls need reposition/resize at the same direction.
 *
**/ 
//#include "stdafx.h"
#include "EnvWinLibs.h"
#pragma hdrstop

#include "ResizeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef OBM_SIZE
#define OBM_SIZE 32766
#endif

CItemCtrl::CItemCtrl()
{
	m_nID = 0;
	m_stxLeft = CST_NONE;
	m_stxRight = CST_NONE;
	m_styTop = CST_NONE;
	m_styBottom = CST_NONE;
	m_bFlickerFree = 1;
	m_xRatio = m_cxRatio = 1.0;
	m_yRatio = m_cyRatio = 1.0;
}

CItemCtrl::CItemCtrl(const CItemCtrl& src)
{
	Assign(src);
}

CItemCtrl& CItemCtrl::operator=(const CItemCtrl& src)
{
	Assign(src);
	return *this;
}

void CItemCtrl::Assign(const CItemCtrl& src)
{
	m_nID = src.m_nID;
	m_stxLeft = src.m_stxLeft;
	m_stxRight = src.m_stxRight;
	m_styTop = src.m_styTop;
	m_styBottom = src.m_styBottom;
	m_bFlickerFree = src.m_bFlickerFree;
	m_bInvalidate = src.m_bInvalidate;
	m_wRect = src.m_wRect;
	m_xRatio = src.m_xRatio;
	m_cxRatio = src.m_cxRatio;
	m_yRatio = src.m_yRatio;
	m_cyRatio = src.m_cyRatio;
}

HDWP CItemCtrl::OnSize(HDWP hDWP,int sizeType, CRect *pnRect, CRect *poRect, CRect *pR0, CWnd *pDlg)
{
	CRect	ctrlRect = m_wRect, curRect;
	CWnd  *pWnd = pDlg->GetDlgItem(m_nID);
	int	deltaX = pnRect->Width() - poRect->Width();
	int	deltaY = pnRect->Height()- poRect->Height();
	int   deltaX0 = pnRect->Width() - pR0->Width();
	int	deltaY0 = pnRect->Height()- pR0->Height();
	int	newCx, newCy;
	int	st, bUpdateRect = 1;

	// process left-horizontal option
	st = CST_NONE;
	if ((sizeType & WST_LEFT) && m_stxLeft != CST_NONE) {
		ASSERT((sizeType & WST_RIGHT) == 0);

		st = m_stxLeft;
	}
	else if ((sizeType & WST_RIGHT) && m_stxRight != CST_NONE) {
		ASSERT((sizeType & WST_LEFT) == 0);

		st = m_stxRight;
	}
	
	switch(st) {
	case CST_NONE:
		if (m_stxLeft == CST_ZOOM || m_stxRight == CST_ZOOM ||
				m_stxLeft == CST_DELTA_ZOOM || m_stxRight == CST_DELTA_ZOOM)
		{
			pWnd->GetWindowRect(&curRect);
			pDlg->ScreenToClient(&curRect);

			ctrlRect.left = curRect.left;
			ctrlRect.right = curRect.right;

			bUpdateRect = 0;
		}

		break;

	case CST_RESIZE:
		ctrlRect.right += deltaX;
		break;

	case CST_REPOS:
		ctrlRect.left += deltaX;
		ctrlRect.right += deltaX;
		break;

	case CST_RELATIVE:
		newCx = ctrlRect.Width();
		ctrlRect.left = (int)((double)m_xRatio * 1.0 * pnRect->Width() - newCx / 2.0);
		ctrlRect.right = ctrlRect.left + newCx; /* (int)((double)m_xRatio * 1.0 * pnRect->Width() + newCx / 2.0); */
		break;

	case CST_ZOOM:
		ctrlRect.left = (int)(1.0 * ctrlRect.left  * (double)pnRect->Width() / pR0->Width());
		ctrlRect.right = (int)(1.0 * ctrlRect.right * (double)pnRect->Width() / pR0->Width());
		bUpdateRect = 0;
		break;

	case CST_DELTA_ZOOM:
		newCx = ctrlRect.Width();
		ctrlRect.right = (int)(ctrlRect.left * 1.0 + deltaX0 * 1.0 * m_xRatio + newCx * 1.0 + deltaX0 * m_cxRatio);
		ctrlRect.left += (int)(deltaX0 * 1.0 * m_xRatio);
		bUpdateRect = 0;
		break;

	default:
		break;
	}

	// process vertical option
	st = CST_NONE;
	if ((sizeType & WST_TOP) && (m_styTop != CST_NONE)) {
		ASSERT((sizeType & WST_BOTTOM) == 0);

		st = m_styTop;
	}
	else if ((sizeType & WST_BOTTOM) && (m_styBottom != CST_NONE)) {
		ASSERT((sizeType & WST_TOP) == 0);

		st = m_styBottom;
	}

	switch (st) {
	case CST_NONE:
		if (m_styTop == CST_ZOOM || m_styTop == CST_ZOOM ||
				m_styBottom == CST_DELTA_ZOOM || m_styBottom == CST_DELTA_ZOOM)
		{
			pWnd->GetWindowRect(&curRect);
			pDlg->ScreenToClient(&curRect);

			ctrlRect.top = curRect.top;
			ctrlRect.bottom = curRect.bottom;

			bUpdateRect = 0;
		}

		break;

	case CST_RESIZE:
		ctrlRect.bottom += deltaY;
		break;

	case CST_REPOS:
		ctrlRect.top += deltaY;
		ctrlRect.bottom += deltaY;
		break;

	case CST_RELATIVE:
		newCy = ctrlRect.Width();
		ctrlRect.top = (int)((double)m_yRatio * 1.0 * pnRect->Width() - newCy / 2.0);
		ctrlRect.bottom = ctrlRect.top + newCy; /* (int)((double)m_yRatio * 1.0 * pnRect->Width() + newCy / 2.0); */

	case CST_ZOOM:
		ctrlRect.top = (int)(1.0 * ctrlRect.top * (double)pnRect->Height() / pR0->Height());
		ctrlRect.bottom = (int)(1.0 * ctrlRect.bottom * (double)pnRect->Height() / pR0->Height());
		bUpdateRect = 0;
		break;

	case CST_DELTA_ZOOM:
		newCy = ctrlRect.Height();
		ctrlRect.bottom = (int)(ctrlRect.top * 1.0 + deltaY0 * 1.0 * m_yRatio + newCy * 1.0 + deltaY0 * m_cyRatio);
		ctrlRect.top += (int)(deltaY0 * 1.0 * m_yRatio);
		bUpdateRect = 0;
		break;

	default:
		break;
	}

	if (!bUpdateRect) {		// in this case, m_wRect is not the real pos of DlgItem(m_nID)
		pWnd->GetWindowRect(&curRect);
		pDlg->ScreenToClient(&curRect);
	}
	else {
		curRect = m_wRect;
	}

	if (ctrlRect != curRect) {
		if (m_bInvalidate) {
			pWnd->Invalidate();
		}

		hDWP = ::DeferWindowPos(hDWP, (HWND)*pWnd, NULL, ctrlRect.left, ctrlRect.top, ctrlRect.Width(), ctrlRect.Height(), SWP_NOZORDER);

#if 1	/* why No effect!!!! */
		if (m_bInvalidate) {
			pDlg->InvalidateRect(&curRect);
			pDlg->UpdateWindow();
		}
#endif	/* No effect???? */

		if (bUpdateRect) {
			m_wRect = ctrlRect;
		}
	}

	return hDWP;
}

IMPLEMENT_DYNAMIC(CResizeDlg, CDialog)
BEGIN_MESSAGE_MAP(CResizeDlg, CDialog)
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CResizeDlg::CResizeDlg(const UINT resID,CWnd *pParent)
	  : CDialog(resID,pParent)
{
	m_xSt = CST_RESIZE;
	m_xSt = CST_RESIZE;
	m_xMin = 32;
	m_yMin = 32;
	m_nDelaySide = 0;
}

CResizeDlg::~CResizeDlg()
{
	// TODO:
}

BOOL CResizeDlg::OnInitDialog() 
{
	BOOL bret = CDialog::OnInitDialog();

	CRect	cltRect;
	CBitmap cBmpSize;
	BITMAP	Bitmap;

	GetClientRect(&cltRect);
	m_cltR0 = cltRect;
	ClientToScreen(&m_cltR0);
	m_cltRect = m_cltR0;

	cBmpSize.LoadOEMBitmap(OBM_SIZE);
	cBmpSize.GetBitmap(&Bitmap);

	m_wndSizeIcon.Create( NULL, 
				WS_CHILD | WS_VISIBLE | SS_BITMAP, 
				CRect(0, 0, Bitmap.bmWidth, Bitmap.bmHeight),
				this, m_idSizeIcon );
	m_wndSizeIcon.SetBitmap(cBmpSize);
	m_wndSizeIcon.SetWindowPos(&wndTop,
				cltRect.right - Bitmap.bmWidth, cltRect.bottom - Bitmap.bmHeight,
				0, 0,
				SWP_NOSIZE );
#if 0
	CRgn  cRgn;
	POINT bmpPt[3] = { { cltRect.right - Bitmap.bmWidth, cltRect.bottom },
					   { cltRect.right, cltRect.bottom - Bitmap.bmHeight},
					   { cltRect.right, cltRect.bottom } };
	cRgn.CreatePolygonRgn(bmpPt, 3, WINDING);
	m_wndSizeIcon.SetWindowRgn(cRgn, TRUE);

	cRgn.Detach();
#endif

	cBmpSize.Detach();

	AddControl(m_idSizeIcon, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS);

	CRect wRect;
	GetWindowRect(&wRect);
	m_xMin = wRect.Width();		// default x limit
	m_yMin = wRect.Height();	// default y limit

	return bret;
}

void CResizeDlg::OnSizing(UINT nSide, LPRECT lpRect)
{
	CDialog::OnSizing(nSide, lpRect);

	m_nDelaySide = nSide;
}

void CResizeDlg::OnSize(UINT nType,int cx,int cy)
{
	int	 nCount;

	std::vector<CItemCtrl>::iterator it;

	CDialog::OnSize(nType,cx,cy);

	if((nCount = (int)m_Items.size()) > 0) {
		CRect  cltRect;
		GetClientRect(&cltRect);
		ClientToScreen(cltRect);

		HDWP   hDWP;
		int    sizeType = WST_NONE;		

#if 0
		int    deltaX = cltRect.Width() - m_cltRect.Width();
		int	   deltaY = cltRect.Height()- m_cltRect.Height();
		int	   midX = (cltRect.left + cltRect.right) / 2;
		int    midY = (cltRect.top + cltRect.bottom) / 2;
		CPoint csrPt(::GetMessagePos());

		if (deltaX) {
			if (csrPt.x < midX)
				sizeType |= WST_LEFT;
			else
				sizeType |= WST_RIGHT;
		}

		if (deltaY) {
			if (csrPt.y < midY)
				sizeType |= WST_TOP;
			else
				sizeType |= WST_BOTTOM;
		}
#else
		switch (m_nDelaySide) {
		case WMSZ_BOTTOM:
			sizeType = WST_BOTTOM;
			break;
		case WMSZ_BOTTOMLEFT:
			sizeType = WST_BOTTOM|WST_LEFT;
			break;
		case WMSZ_BOTTOMRIGHT:
			sizeType = WST_BOTTOM|WST_RIGHT;
			break;
		case WMSZ_LEFT:
			sizeType = WST_LEFT;
			break;
		case WMSZ_RIGHT:
			sizeType = WST_RIGHT;
			break;
		case WMSZ_TOP:
			sizeType = WST_TOP;
			break;
		case WMSZ_TOPLEFT:
			sizeType = WST_TOP|WST_LEFT;
			break;
		case WMSZ_TOPRIGHT:
			sizeType = WST_TOP|WST_RIGHT;
			break;
		default:
			break;
		}
#endif

		if (sizeType != WST_NONE) {
			hDWP = ::BeginDeferWindowPos(nCount);

			for (it = m_Items.begin(); it != m_Items.end(); it++)
				hDWP = it->OnSize(hDWP, sizeType, &cltRect, &m_cltRect, &m_cltR0, this);

			::EndDeferWindowPos(hDWP);
		}

		m_cltRect = cltRect;
	}

	m_nDelaySide = 0;
}

void CResizeDlg::OnGetMinMaxInfo(MINMAXINFO *pmmi)
{
	if ((HWND)m_wndSizeIcon == NULL)
		return;

	pmmi->ptMinTrackSize.x = m_xMin;
	pmmi->ptMinTrackSize.y = m_yMin;

	if (m_xSt == CST_NONE)
		pmmi->ptMaxTrackSize.x = pmmi->ptMaxSize.x = m_xMin;
	if (m_ySt == CST_NONE)
		pmmi->ptMaxTrackSize.y = pmmi->ptMaxSize.y = m_yMin;
}

BOOL CResizeDlg::OnEraseBkgnd(CDC *pDC)
{
	if (!(GetStyle() & WS_CLIPCHILDREN)) {
		std::vector<CItemCtrl>::const_iterator it;

		for(it = m_Items.begin();it != m_Items.end(); it++) {
			// skip over the size icon if it's been hidden
			if(it->m_nID == m_idSizeIcon && !m_wndSizeIcon.IsWindowVisible())
				continue;

			if(it->m_bFlickerFree && ::IsWindowVisible(GetDlgItem(it->m_nID)->GetSafeHwnd())) {
				pDC->ExcludeClipRect(&it->m_wRect);
			}
		}
	}

	CDialog::OnEraseBkgnd(pDC);
	return FALSE;
}

void CResizeDlg::AddControl( UINT nID,int xl, int xr, int yt, int yb, int bFlickerFree,
								double xRatio, double cxRatio, double yRatio, double cyRatio )
{
	CItemCtrl	item;
	CRect		cltRect;

	GetDlgItem(nID)->GetWindowRect(&item.m_wRect);
	ScreenToClient(&item.m_wRect);

	item.m_nID = nID;
	item.m_stxLeft = xl;
	item.m_stxRight = xr;
	item.m_styTop = yt;
	item.m_styBottom = yb;
	item.m_bFlickerFree = !!(bFlickerFree & 0x01);
	item.m_bInvalidate = !!(bFlickerFree & 0x02);
	item.m_xRatio = xRatio;
	item.m_cxRatio = cxRatio;
	item.m_yRatio = yRatio;
	item.m_cyRatio = cyRatio;

	GetClientRect(&cltRect);
	if (xl == CST_RELATIVE || xl == CST_ZOOM || xr == CST_RELATIVE || xr == CST_ZOOM)
		item.m_xRatio = (item.m_wRect.left + item.m_wRect.right) / 2.0 / cltRect.Width();

	if (yt == CST_RELATIVE || yt == CST_ZOOM || yb == CST_RELATIVE || yb == CST_ZOOM)
		item.m_yRatio = (item.m_wRect.bottom + item.m_wRect.top ) / 2.0 / cltRect.Height();

	std::vector<CItemCtrl>::iterator it;
	int  nCount;

	if((nCount = (int)m_Items.size()) > 0) {
		for (it = m_Items.begin(); it != m_Items.end(); it++) {
			if (it->m_nID == item.m_nID) {
				*it = item;
				return;
			}
		}
	}

	m_Items.push_back(item);

}

void CResizeDlg::AllowSizing(int xst, int yst)
{
	m_xSt = xst;
	m_ySt = yst;
}

void CResizeDlg::HideSizeIcon(void)
{
	m_wndSizeIcon.ShowWindow(SW_HIDE);
}

int CResizeDlg::UpdateControlRect(UINT nID, CRect *pnr)
{
	std::vector<CItemCtrl>::iterator it;
	int  nCount;

	if ((nCount = (int)m_Items.size()) > 0) {
		for (it = m_Items.begin(); it != m_Items.end(); it++) {
			if (it->m_nID == nID) {
				if (pnr != NULL) {
					it->m_wRect = *pnr;
				}
				else {
					GetDlgItem(nID)->GetWindowRect(&it->m_wRect);
					ScreenToClient(&it->m_wRect);
				}

				return 0;
			}
		}
	}

	return -1;
}
