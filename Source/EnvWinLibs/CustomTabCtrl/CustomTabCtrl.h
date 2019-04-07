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
#pragma once

/**********************************************************************
**
**	CustomTabCtrl.h : include file
**
**	by Andrzej Markowski June 2004
**
**********************************************************************/

#include <Afxtempl.h>
#include <afxcmn.h>

#include "themeutil.h"
#include "EnvWinLibs.h"

// CustomTabCtrlItem

#define TAB_SHAPE1		0		//  Invisible

#define TAB_SHAPE2		1		//	 __
								//	| /
								//	|/

#define TAB_SHAPE3		2		//	|\
								//	|/

#define TAB_SHAPE4		3		//	____________
								//	\          /
								//   \________/

#define TAB_SHAPE5		4		//	___________
								//	\          \
								//	  \________/

#define RECALC_PREV_PRESSED			0
#define RECALC_NEXT_PRESSED			1
#define RECALC_ITEM_SELECTED		2
#define RECALC_RESIZED				3
#define RECALC_FIRST_PRESSED		4
#define RECALC_LAST_PRESSED			5
#define RECALC_EDIT_RESIZED			6

#define MAX_LABEL_TEXT				30

typedef struct _CTC_NMHDR 
{
    NMHDR hdr;
	int	nItem;
	TCHAR pszText[MAX_LABEL_TEXT];
	LPARAM lParam;
	RECT rItem;
	POINT ptHitTest;
	BOOL fSelected;
	BOOL fHighlighted;
} CTC_NMHDR;

class WINLIBSAPI CCustomTabCtrlItem
{
	friend class CCustomTabCtrl;
private:
								CCustomTabCtrlItem(CString sText, LPARAM lParam);
	void						ComputeRgn();
	void						Draw(CDC& dc, CFont& font);
	BOOL						HitTest(CPoint pt)			{ return (m_bShape && m_rgn.PtInRegion(pt)) ? TRUE : FALSE; }
	void						GetRegionPoints(const CRect& rc, CPoint* pts) const;
	void						GetDrawPoints(const CRect& rc, CPoint* pts) const;
	void						operator=(const CCustomTabCtrlItem &other);
private:
	CString						m_sText;
	LPARAM						m_lParam;
	CRect						m_rect;
	CRect						m_rectText;
	CRgn						m_rgn;			
	BYTE						m_bShape;
	BOOL						m_fSelected;
	BOOL						m_fHighlighted;
	BOOL						m_fHighlightChanged;
};

// CCustomTabCtrl

// styles
#define CTCS_FIXEDWIDTH			1		// Makes all tabs the same width. 
#define CTCS_FOURBUTTONS		2		// Four buttons (First, Prev, Next, Last) 
#define CTCS_AUTOHIDEBUTTONS	4		// Auto hide buttons
#define CTCS_TOOLTIPS			8		// Tooltips
#define CTCS_MULTIHIGHLIGHT		16		// Multi highlighted items
#define CTCS_EDITLABELS			32		// Allows item text to be edited in place
#define CTCS_DRAGMOVE			64		// Allows move items
#define CTCS_DRAGCOPY			128		// Allows copy items

// hit test
#define CTCHT_ONFIRSTBUTTON		-1
#define CTCHT_ONPREVBUTTON		-2
#define CTCHT_ONNEXTBUTTON		-3
#define CTCHT_ONLASTBUTTON		-4
#define CTCHT_NOWHERE			-5

// notification messages
#define CTCN_CLICK				1
#define CTCN_RCLICK				2
#define CTCN_SELCHANGE			3
#define CTCN_HIGHLIGHTCHANGE	4
#define CTCN_ITEMMOVE			5
#define CTCN_ITEMCOPY			6
#define CTCN_LABELUPDATE		7
#define CTCN_OUTOFMEMORY		8

#define CTCID_FIRSTBUTTON		-1
#define CTCID_PREVBUTTON		-2
#define CTCID_NEXTBUTTON		-3	
#define CTCID_LASTBUTTON		-4
#define CTCID_NOBUTTON			-5

#define CTCID_EDITCTRL			1

#define REPEAT_TIMEOUT			250

// error codes
#define CTCERR_NOERROR					0
#define CTCERR_OUTOFMEMORY				-1
#define CTCERR_INDEXOUTOFRANGE			-2
#define CTCERR_NOEDITLABELSTYLE			-3
#define CTCERR_NOMULTIHIGHLIGHTSTYLE	-4
#define CTCERR_ITEMNOTSELECTED			-5
#define CTCERR_ALREADYINEDITMODE		-6
#define CTCERR_TEXTTOOLONG				-7
#define CTCERR_NOTOOLTIPSSTYLE			-8
#define CTCERR_CREATETOOLTIPFAILED		-9

// button states
#define BNST_INVISIBLE			0
#define BNST_NORMAL				DNHZS_NORMAL
#define BNST_HOT				DNHZS_HOT
#define BNST_PRESSED			DNHZS_PRESSED

#define CustomTabCtrl_CLASSNAME    _T("CCustomTabCtrl")  // Window class name

class WINLIBSAPI CCustomTabCtrl : public CWnd
{
public:

	// Construction

	CCustomTabCtrl();
	virtual						~CCustomTabCtrl();
	BOOL						Create(UINT dwStyle, const CRect & rect, CWnd * pParentWnd, UINT nID);

	// Attributes

	int							GetItemCount() {return (int) m_aItems.GetSize();}
	int							GetCurSel() { return m_nItemSelected; }
	int							SetCurSel(int nItem);
	int							IsItemHighlighted(int nItem);
	int							HighlightItem(int nItem, BOOL fHighlight);
	int							GetItemData(int nItem, DWORD& dwData);
	int							SetItemData(int nItem, DWORD dwData);
	int							GetItemText(int nItem, CString& sText);
	int							SetItemText(int nItem, CString sText);
	int							GetItemRect(int nItem, CRect& rect) const;
	int							SetItemTooltipText(int nItem, CString sText);
	void						SetDragCursors(HCURSOR hCursorMove, HCURSOR hCursorCopy);
	BOOL						ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags);
	void						SetControlFont(const LOGFONT& lf, BOOL fRedraw=FALSE);
	static const LOGFONT&		GetDefaultFont() {return lf_default;}

	// Operations

	int							InsertItem(int nItem, CString sText, LPARAM lParam=0);
	int							DeleteItem(int nItem);
	void						DeleteAllItems();
	int							MoveItem(int nItemSrc, int nItemDst);
	int							CopyItem(int nItemSrc, int nItemDst);
	int							HitTest(CPoint pt);
	int							EditLabel(int nItem);

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCustomTabCtrl)
	protected:
	virtual void PreSubclassWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CCustomTabCtrl)
	afx_msg BOOL				OnEraseBkgnd(CDC* pDC);
	afx_msg void				OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void				OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void				OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT 			OnMouseLeave(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT				OnThemeChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void				OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void				OnPaint();
	afx_msg void				OnSize(UINT nType, int cx, int cy);
	afx_msg void				OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void				OnTimer(UINT_PTR nIDEvent);
	afx_msg void				OnUpdateEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()	

private:
	void						RecalcLayout(int nRecalcType,int nItem);
	void						RecalcEditResized(int nOffset, int nItem);
	void						RecalcOffset(int nOffset);
	int							RecalcRectangles();
	BOOL						RegisterWindowClass();
	int							ProcessLButtonDown(int nHitTest, UINT nFlags, CPoint point);
	int							MoveItem(int nItemSrc, int nItemDst, BOOL fMouseSel);
	int							CopyItem(int nItemSrc, int nItemDst, BOOL fMouseSel);
	int							SetCurSel(int nItem, BOOL fMouseSel, BOOL fCtrlPressed);
	int							HighlightItem(int nItem, BOOL fMouseSel, BOOL fCtrlPressed);
	void						DrawGlyph(CDC& dc, CPoint& pt, int nImageNdx, int nColorNdx);
	void						DrawBkLeftSpin(CDC& dc, CRect& r, int nImageNdx);
	void						DrawBkRightSpin(CDC& dc, CRect& r, int nImageNdx);
	BOOL						NotifyParent(UINT code, int nItem, CPoint pt);
	int							EditLabel(int nItem, BOOL fMouseSel);
private:
	static LOGFONT				lf_default;
	static BYTE					m_bBitsGlyphs[];
	HCURSOR						m_hCursorMove;
	HCURSOR						m_hCursorCopy;
	CFont						m_Font;
	CFont						m_FontSelected;
	int							m_nItemSelected;
	int							m_nItemNdxOffset;
	int							m_nItemDragDest;
	int							m_nPrevState;
	int							m_nNextState;
	int							m_nFirstState;
	int							m_nLastState;
	int							m_nButtonIDDown;
	DWORD						m_dwLastRepeatTime;
	COLORREF					m_rgbGlyph[4];
	CBitmap						m_bmpGlyphsMono;
	HBITMAP						m_hBmpBkLeftSpin;
	BOOL						m_fIsLeftImageHorLayout;
	MY_MARGINS					m_mrgnLeft;
	MY_MARGINS					m_mrgnRight;
	HBITMAP						m_hBmpBkRightSpin;
	BOOL						m_fIsRightImageHorLayout;
	CToolTipCtrl				m_ctrlToolTip;
	CEdit						m_ctrlEdit;
	CArray <CCustomTabCtrlItem*,CCustomTabCtrlItem*>	m_aItems;
};
