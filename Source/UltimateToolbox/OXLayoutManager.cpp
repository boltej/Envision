// =============================================================================
// 							Class Implementation : COXLayoutManager
// =============================================================================
//
// Source file : 		OXLayoutManager.cpp

// Version: 9.3

// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.

// //////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "OXLayoutManager.h"
#include <afxpriv.h>		// For WM_INITIALUPDATE definition

#ifndef __OXMFCIMPL_H__
#if _MFC_VER >= 0x0700
#if _MFC_VER >= 1400
#include <afxtempl.h>
#endif
#include <..\src\mfc\afximpl.h>
#else
#include <..\src\afximpl.h>
#endif
#define __OXMFCIMPL_H__
#endif

#include "UTB64Bit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(COXLayoutManager, CObject)

/////////////////////////////////////////////////////////////////////////////
// maps between single side parameter and array index in sc[]
static int OX_LMS_TO_INDEX[] = { -1, 0, 1, -1, 2, -1, -1, -1, 3 };
static int OX_INDEX_TO_LMS[] = { OX_LMS_TOP, OX_LMS_BOTTOM, OX_LMS_LEFT, OX_LMS_RIGHT };

// side index
#define OX_LMSI_TOP	   0
#define OX_LMSI_BOTTOM 1
#define OX_LMSI_HORZ   1	// if nSideIndex > OX_LMSI_HORZ, ... 
#define OX_LMSI_LEFT   2
#define OX_LMSI_RIGHT  3
#define OX_LMSI_OPPOSITE(nSideIndex) (nSideIndex > OX_LMSI_HORZ ? 5 : 1) - nSideIndex

// return values of COXLayoutManager::CalcLayout()
#define OX_LM_SIZING_OK					0
#define OX_LM_SIZING_FAIL				1
#define OX_LM_SIZING_NEEDLARGERCX		2
#define OX_LM_SIZING_NEEDLARGERCY		4
#define OX_LM_SIZING_NEEDSMALLERCX		8
#define OX_LM_SIZING_NEEDSMALLERCY		16

/////////////////////////////////////////////////////////////////////////////
// Definition of static members
CMap<HWND, HWND, COXLayoutManager*, COXLayoutManager*> COXLayoutManager::m_allLayoutManagers;
// --- A map of all the containers that have been subclassed and that are associated with
//     a COXLayoutManager object.  This object will handle the windows messages first

// Data members -------------------------------------------------------------
// protected:
//
//	CWnd* m_pContainerWnd;		the container window
//	int	  m_nBase;				the current fraction base
//	UINT  m_nMinMaxCount;		used to speed up min/max checking, if non-zero,
//								min/max will be checked when resizing
//	int	  m_cx;					for storing current new width of the container window
//	int	  m_cy;					for storing current new height of the container window
//	int   m_cxMin;				minimum width of the container window
//	int	  m_cxMax;				maximum width of the container window
//	int   m_cyMin;				minimum height of the container window
//	int   m_cyMax;				maximum height of the container window
//	CArray<COXWndConstraint*, COXWndConstraint*> m_wcTable;
//								window constraints of all child windows

// Member functions ---------------------------------------------------------

COXLayoutManager::COXLayoutManager(CWnd* pContainerWnd /* = NULL */)
	:
	m_pContainerWnd(NULL),
	m_nBase(100),
	m_nMinMaxCount(0),
	m_hWnd(NULL),
	m_pfnSuper(NULL)
	{
	if (pContainerWnd != NULL)
		Attach(pContainerWnd);
	}

COXLayoutManager::~COXLayoutManager()
	{
	RemoveAllChildren();
	UnsubclassContainer();
	}

void COXLayoutManager::Attach(CWnd* pContainerWnd)
	{
	ASSERT(m_pContainerWnd == NULL);
	ASSERT(pContainerWnd);

	RemoveAllChildren();
	m_pContainerWnd = pContainerWnd;
	VERIFY(SubclassContainer(pContainerWnd));
	}

void COXLayoutManager::Detach()
{
	ASSERT(m_pContainerWnd);

	RemoveAllChildren();
	m_pContainerWnd = NULL;
	UnsubclassContainer();
}

int COXLayoutManager::AddChild(UINT nChildWnd,
							   BOOL bSetDefaultConstraints /* = TRUE */)
{
	// we do not validate ID here in case child window not created yet, instead we'll do it
	// at run-time when calculating coordinates

	// cannot be topmost window
	if (nChildWnd == 0)
		return -1;

	// already added previously
	//int nIndex = PtrToInt(GetChildIndex(nChildWnd));
	int nIndex = GetChildIndex( nChildWnd );  // jpb 5/1/09 for WIN64 conversion
	if (nIndex == -1)
	{
		COXWndConstraint* pWC = new COXWndConstraint(nChildWnd);
		//nIndex = PtrToInt(m_wcTable.Add(pWC));
		nIndex = (int) m_wcTable.Add( pWC );  // jpb 5/1/09 for WIN64 conversion
	}

	if (bSetDefaultConstraints)
		SetDefaultConstraint(nChildWnd);

	return nIndex;
}

void COXLayoutManager::AddAllChildren(BOOL bSetDefaultConstraints /* = TRUE */)
{
	ASSERT(m_pContainerWnd);

	CWnd* pChild = m_pContainerWnd->GetWindow(GW_CHILD);
	for (; pChild; pChild = pChild->GetNextWindow())
		AddChild(pChild, bSetDefaultConstraints);
}

BOOL COXLayoutManager::RemoveChild(UINT nChildWnd)
{
	ASSERT(nChildWnd);

	int nIndex = GetChildIndex(nChildWnd);
	if (nIndex == -1)
		return FALSE;

	// look for dependents of nChildWnd
	for (int i = 0; i < m_wcTable.GetSize(); i++)
	{
		COXWndConstraint* pWC = m_wcTable[i];
		for (int j = 0; j < 4; j++)
		{
			COXSideConstraint* pSC = &pWC->sc[j];
			if (pSC->nBaseWnd == nChildWnd)
			{
				TRACE2("COXLayoutManager::RemoveChild(): failed to remove child %d, window %d depends on it.\r\n", nChildWnd, pWC->nID);
				return FALSE;
			}
		}
	}

	COXWndConstraint* pWC = m_wcTable[nIndex];
	if (pWC->bHasMinMax)
		m_nMinMaxCount--;
	delete pWC;
	m_wcTable.RemoveAt(nIndex);
	ResetContainerMinMax();
	return TRUE;
}

void COXLayoutManager::RemoveAllChildren()
{
	for (int i = 0; i < m_wcTable.GetSize(); i++)
		delete m_wcTable[i];
	m_wcTable.RemoveAll();
	m_nMinMaxCount = 0;
	ResetContainerMinMax();
}

BOOL COXLayoutManager::SetConstraint(UINT nChildWnd, int nSide, int nType,
									 int nOffset /* = 0 */, UINT nBaseWnd /* = 0 */)
{
	ASSERT(nChildWnd);
	ASSERT(nType == OX_LMT_SAME || nType == OX_LMT_OPPOSITE || nType == OX_LMT_POSITION);

	// we add nBaseWnd first to linearlize window dependency better (faster when calculating)
	if (nBaseWnd && GetChildIndex(nBaseWnd) == -1)
		AddChild(nBaseWnd, FALSE);

	int nIndex = GetChildIndex(nChildWnd);
	if (nIndex == -1)
	{
		nIndex = AddChild(nChildWnd, FALSE);
		if (nIndex == -1)
			return FALSE;
	}

	for (int i = 0; i < 4; i++)
	{
		int nSingleSide = OX_INDEX_TO_LMS[i];
		if (nSide & nSingleSide)
		{
			COXSideConstraint* pSC = &m_wcTable[nIndex]->sc[i];

			BOOL bFlipSide=
				((nSingleSide & OX_LMS_MAJOR) && (nType == OX_LMT_OPPOSITE)) || 
				((nSingleSide & OX_LMS_MINOR) && (nType == OX_LMT_SAME));

			int nDefOffsetPos = (nType == OX_LMT_POSITION ? m_nBase : OX_LMOFFSET_ANY);
			pSC->nBaseWnd = nBaseWnd;
			pSC->nOffset1 = bFlipSide ? nDefOffsetPos : nOffset;
			pSC->nOffset2 = bFlipSide ? nOffset : nDefOffsetPos;
		}
	}

	ResetContainerMinMax();
	return TRUE;
}

BOOL COXLayoutManager::SetMinMax(UINT nChildWnd, CSize sizeMin, CSize sizeMax /* CSize(0,0) */)
{
	ASSERT(nChildWnd);

	int nIndex = GetChildIndex(nChildWnd);
	if (nIndex == -1)
	{
		nIndex = AddChild(nChildWnd, FALSE);
		if (nIndex == -1)
			return FALSE;
	}

	COXWndConstraint* pWC = m_wcTable[nIndex];
	if (sizeMin.cx <=0 && sizeMin.cy <=0 && sizeMax.cx <=0 && sizeMax.cy <=0)
	{
		if (pWC->bHasMinMax)
		{
			pWC->bHasMinMax = FALSE;
			m_nMinMaxCount--;
		}
	}
	else
	{
		if (!pWC->bHasMinMax)
		{
			pWC->bHasMinMax = TRUE;
			m_nMinMaxCount++;
		}
	}
	pWC->sizeMin = sizeMin;
	pWC->sizeMax = sizeMax;

	ResetContainerMinMax();
	return TRUE;
}

BOOL COXLayoutManager::RemoveConstraint(UINT nChildWnd, int nSide)
	{
	ASSERT(nChildWnd);

	int nIndex = GetChildIndex(nChildWnd);
	if (nIndex == -1)
		return FALSE;

	for (int i = 0; i < 4; i++)
		{
		if (nSide & OX_INDEX_TO_LMS[i])
			m_wcTable[nIndex]->sc[i].Empty();
		}
	ResetContainerMinMax();
	return TRUE;
	}

BOOL COXLayoutManager::GetConstraint(UINT nChildWnd, int nSide, int& nType,
									 int& nOffset, UINT& nBaseWnd)
	{
	ASSERT(nChildWnd);
	ASSERT(nSide == OX_LMS_TOP  || nSide == OX_LMS_BOTTOM ||
		   nSide == OX_LMS_LEFT || nSide == OX_LMS_RIGHT);

	int nIndex = GetChildIndex(nChildWnd);
	if (nIndex == -1)
		return FALSE;

	COXSideConstraint* pSC = &m_wcTable[nIndex]->sc[OX_LMS_TO_INDEX[nSide]];
	nBaseWnd = pSC->nBaseWnd;
	if (pSC->nOffset1 == OX_LMOFFSET_ANY)
	{
		if (pSC->nOffset2 == OX_LMOFFSET_ANY)
		{
			nType = 0;
			nOffset = OX_LMOFFSET_ANY;
		}
		else
		{
			nType = (nSide & OX_LMS_MINOR) ? OX_LMT_SAME : OX_LMT_OPPOSITE;
			nOffset = pSC->nOffset2;
		}
	}
	else
	{
		if (pSC->nOffset2 == OX_LMOFFSET_ANY)
		{
			nType = (nSide & OX_LMS_MAJOR) ? OX_LMT_SAME : OX_LMT_OPPOSITE;
			nOffset = pSC->nOffset1;
		}
		else
		{
			nType = OX_LMT_POSITION;
			ASSERT (pSC->nOffset2);
			nOffset = (int)(pSC->nOffset1 * m_nBase / pSC->nOffset2);
		}
	}

	return TRUE;
}

BOOL COXLayoutManager::GetMinMax(UINT nChildWnd, CSize& sizeMin, CSize& sizeMax)
{
	ASSERT(nChildWnd);

	int nIndex = GetChildIndex(nChildWnd);
	if (nIndex == -1)
		return FALSE;

	COXWndConstraint* pWC = m_wcTable[nIndex];
	sizeMin = pWC->sizeMin;
	sizeMax = pWC->sizeMax;
	return TRUE;
}

BOOL COXLayoutManager::RedrawLayout()
{
	if (m_pContainerWnd == NULL || !::IsWindow(m_pContainerWnd->m_hWnd))
		return FALSE;

	CRect rect;
	m_pContainerWnd->GetClientRect(rect);
	return OnSize(rect.Width(), rect.Height());
}

BOOL COXLayoutManager::OnSize(int cx, int cy)
{
	if(m_pContainerWnd == NULL || !::IsWindow(m_pContainerWnd->m_hWnd))
		return TRUE;

	if(cx == 0 || cy == 0 || m_wcTable.GetSize() == 0)
		return TRUE;

	m_cx = __min(__max(cx, m_cxMin), m_cxMax);
	m_cy = __min(__max(cy, m_cyMin), m_cyMax);
	
	int nLastResult = OX_LM_SIZING_OK;
	while (true)
	{
		// m_cx and m_cy adjustment reaching screen limit
		if (m_cx <= 0 || m_cx > 32767 || m_cy <= 0 || m_cy > 32767)
		{
			// whenever seeing this msg, check all constraint and min/max settings
			TRACE0("COXLayoutManager::OnSize(): contraints have internal conflicts.\r\n");
			return FALSE;
		}
		
		int nResult = CalcLayout();

		if (nResult == OX_LM_SIZING_FAIL)
		{
			TRACE0("COXLayoutManager::OnSize(): contraints have internal conflicts.\r\n");
			return FALSE;
		}
			
		if (nResult == OX_LM_SIZING_OK)
		{
			// save container's min/max good value
			if (nLastResult != OX_LM_SIZING_OK)
			{
				if (nLastResult & OX_LM_SIZING_NEEDLARGERCX)
					m_cxMin = m_cx;
				else if (nLastResult & OX_LM_SIZING_NEEDSMALLERCX)
					m_cxMax = m_cx;

				if (nLastResult & OX_LM_SIZING_NEEDLARGERCY)
					m_cyMin = m_cy;
				else if (nLastResult & OX_LM_SIZING_NEEDSMALLERCY)
					m_cyMax = m_cy;
			}
		
			// normal exit from while loop
			break;
		}

		// adjust m_cx and m_cy until all constraints can be conformed
		nLastResult = nResult;
		if (nResult & OX_LM_SIZING_NEEDLARGERCX)
			m_cx++;
		else if (nResult & OX_LM_SIZING_NEEDSMALLERCX)
			m_cx--;

		if (nResult & OX_LM_SIZING_NEEDLARGERCY)
			m_cy++;
		else if (nResult & OX_LM_SIZING_NEEDSMALLERCY)
			m_cy--;
	}

	// reposition wnd
	//int nNumWindows = PtrToInt( m_wcTable.GetSize());  // jpb 5/1/09 for WIN64 conversion
	int nNumWindows = (int) m_wcTable.GetSize();

	HDWP hdwp = ::BeginDeferWindowPos(nNumWindows);
	if (hdwp)
	{
		for (int i = 0; i < nNumWindows; i++)
		{
			COXWndConstraint* pWC = m_wcTable[i];
			CWnd* pWnd = m_pContainerWnd->GetDlgItem(pWC->nID);
			ASSERT(pWnd);
			if (pWnd)
			{
				CRect rectWC = pWC->GetRect();
				::DeferWindowPos(hdwp, pWnd->m_hWnd, NULL, 
					rectWC.left, rectWC.top, rectWC.Width(), rectWC.Height(), 
					SWP_NOZORDER);
				pWnd->RedrawWindow();

			}
		}
		return ::EndDeferWindowPos(hdwp);
	}
	return FALSE;
}

// protected:
int COXLayoutManager::CalcLayout()
{
	// --- In      :
	// --- Out     : 
	// --- Returns : OX_LM_SIZING_OK			successful
	//				 OX_LM_SIZING_FAIL			abnormal failure (self-conflict constraints)
	//				 OX_LM_SIZING_NEEDLARGERCX	width too small to position all childs
	//				 OX_LM_SIZING_NEEDLARGERCY	height too small to position all childs
	//				 OX_LM_SIZING_NEEDSMALLERCX	width too large to position all childs
	//				 OX_LM_SIZING_NEEDSMALLERCY	height too large to position all childs
	// --- Effect  : use m_cx and m_cy as width and height of the container window to 
	//					calculate layout of all child windows

	int i, j;

	// set all need-to-determine pos to null
	for (i = 0; i < m_wcTable.GetSize(); i++)
	{
		COXWndConstraint* pWC = m_wcTable[i];
		pWC->ResetPos();
	}

	// determine all pos
	// whole array is sequentially filled up with calculated positions
	// even though internally some later positions will be filled up
	// before an earlier one, if it depends on the later ones
	for (i = 0; i < m_wcTable.GetSize(); i++)
	{
		COXWndConstraint* pWC = m_wcTable[i];
		for (j = 0; j < 4; j++)
		{
			if (CalcSideConstraint(pWC, j) == OX_LMPOS_NULL)
				return OX_LM_SIZING_FAIL;
		}
	}

	int nResult = OX_LM_SIZING_OK;
	// check min/max
	if (m_nMinMaxCount)
	{
		for (i = 0; i < m_wcTable.GetSize(); i++)
		{
			COXWndConstraint* pWC = m_wcTable[i];
			if (pWC->bHasMinMax)
			{
				CRect rectWC = pWC->GetRect();
				CSize sizeWC = CSize(rectWC.right - rectWC.left, rectWC.bottom - rectWC.top);

				if (pWC->sizeMin.cx > 0 && sizeWC.cx < pWC->sizeMin.cx)
				{
					if (nResult & OX_LM_SIZING_NEEDSMALLERCX)
						return OX_LM_SIZING_FAIL;
					else
						nResult |= OX_LM_SIZING_NEEDLARGERCX;
				}
					
				if (pWC->sizeMax.cx > 0 && sizeWC.cx > pWC->sizeMax.cx)
				{
					if (nResult & OX_LM_SIZING_NEEDLARGERCX)
						return OX_LM_SIZING_FAIL;
					else
						nResult |= OX_LM_SIZING_NEEDSMALLERCX;
				}

				if (pWC->sizeMin.cy > 0 && sizeWC.cy < pWC->sizeMin.cy)
				{
					if (nResult & OX_LM_SIZING_NEEDSMALLERCY)
						return OX_LM_SIZING_FAIL;
					else
						nResult |= OX_LM_SIZING_NEEDLARGERCY;
				}

				if (pWC->sizeMax.cy > 0 && sizeWC.cy > pWC->sizeMax.cy)
				{
					if (nResult & OX_LM_SIZING_NEEDLARGERCY)
						return OX_LM_SIZING_FAIL;
					else
						nResult |= OX_LM_SIZING_NEEDSMALLERCY;
				}
			}
		}
	}

	return nResult;
}

int	COXLayoutManager::GetChildIndex(UINT nChildWnd) const
	// --- In      : nChildWnd, the child window to search
	// --- Out     : 
	// --- Returns : index of a child window in m_wcTable
	// --- Effect  : search m_wcTable for nChildWnd
{
	int i = 0;
	for (i = /*PtrToInt*/int(m_wcTable.GetSize()) - 1; i >= 0 && m_wcTable[i]->nID != nChildWnd; i--);
	return i;
}

BOOL COXLayoutManager::CalcBaseWndIndex(COXSideConstraint* pSC)
	// --- In      : pSC, the side constraint from which to search for base window's index
	// --- Out     : 
	// --- Returns : TRUE if successful; FALSE otherwise
	// --- Effect  : base window's index is needed when resizing for tracing down all
	//				 positions; last good value is stored in pSC->nBaseWndIndex for 
	//				 speeding up, yet boundary checking and quick re-evaluation is needed
	//				 for a safer operation, esp. after run-time adding or removing constraints
	//				 Overall this function provides a dynamic linked list
{
	ASSERT(pSC->nBaseWnd > 0);
	if (pSC->nBaseWndIndex < 0 || pSC->nBaseWndIndex >= m_wcTable.GetSize() ||
		m_wcTable[pSC->nBaseWndIndex]->nID != pSC->nBaseWnd)
	{
		for (int i = 0; i < m_wcTable.GetSize(); i++)
		{
			if (m_wcTable[i]->nID == pSC->nBaseWnd)
			{
				pSC->nBaseWndIndex = i;
				return TRUE;
			}
		}
		TRACE0("COXLayoutManager::CalcBaseWndIndex(): failed to find base window in children list.\r\n");
		return FALSE;
	}
	return TRUE;
}

int COXLayoutManager::CalcSideConstraint(COXWndConstraint* pWC, int nSideIndex)
	// --- In      : pWC, the window constraint in which side constraint is to calculate
	//				 nSideIndex, used to determine which side of the base window to trace
	// --- Out     : 
	// --- Returns : calculated position. OX_LMPOS_NULL if failed
	// --- Effect  : calculate a side constraint (storing new value into nPos)
	//				 if not calculatable, recursively trace down to the next side
	//				 constraint until all done. OX_LMPOS_TRACING is used to flag for 
	//				 detecting circular constraints.
{
	COXSideConstraint* pSC = &pWC->sc[nSideIndex];
	int& p  = pSC->nPos;
	int& d1 = pSC->nOffset1;
	int& d2 = pSC->nOffset2;

	if (p == OX_LMPOS_TRACING)
	{
		TRACE0("COXLayoutManager::CalcSideConstraint(): circular constraints found.\r\n");
		return OX_LMPOS_NULL;
	}

	// already calculated
	if (p != OX_LMPOS_NULL)
		return p;

	// if not constrainted
	if (pSC->IsEmpty())
	{
		CWnd* pWnd = m_pContainerWnd->GetDlgItem(pWC->nID);
		ASSERT(pWnd);
		if (pWnd == NULL)
			return OX_LMPOS_NULL;

		CRect rect;
		pWnd->GetWindowRect(rect);
		m_pContainerWnd->ScreenToClient(rect);

		int nOppositeSideIndex = OX_LMSI_OPPOSITE(nSideIndex);
		COXSideConstraint* pOppositeSC = &pWC->sc[nOppositeSideIndex];
		if (pOppositeSC->IsEmpty())
		{ // use current coordinates
			switch (nSideIndex)
			{
				case OX_LMSI_TOP:	p = rect.top;		break;
				case OX_LMSI_BOTTOM:p = rect.bottom;	break;
				case OX_LMSI_LEFT:	p = rect.left;		break;
				case OX_LMSI_RIGHT:	p = rect.right;		break;
			}
		}
		else
		{ // maintain child window's width and height
			int nBasePos = CalcSideConstraint(pWC, nOppositeSideIndex);
			if (nBasePos == OX_LMPOS_NULL)
				return OX_LMPOS_NULL;
			switch (nSideIndex)
			{
				case OX_LMSI_TOP:	p = nBasePos - rect.Height();	break;
				case OX_LMSI_BOTTOM:p = nBasePos + rect.Height();	break;
				case OX_LMSI_LEFT:	p = nBasePos - rect.Width();	break;
				case OX_LMSI_RIGHT:	p = nBasePos + rect.Width();	break;
			}
		}
		return p;
	}

	// calculate base positions
	int nBasePos1 = OX_LMPOS_NULL;
	int nBasePos2 = OX_LMPOS_NULL;
	BOOL bVert = nSideIndex > OX_LMSI_HORZ;
	if (pSC->nBaseWnd == 0)
	{
		nBasePos1 = 0;
		nBasePos2 = (bVert ? m_cx : m_cy);
	}
	else
	{
		p = OX_LMPOS_TRACING;

		if (!CalcBaseWndIndex(pSC))
			return OX_LMPOS_NULL;
		COXWndConstraint* pBaseWC = m_wcTable[pSC->nBaseWndIndex];

		if (d1 != OX_LMOFFSET_ANY)
		{
			nBasePos1 = CalcSideConstraint(pBaseWC, (bVert ? OX_LMSI_LEFT : OX_LMSI_TOP));
			if (nBasePos1 == OX_LMPOS_NULL)
				return OX_LMPOS_NULL;
		}

		if (d2 != OX_LMOFFSET_ANY)
		{
			nBasePos2 = CalcSideConstraint(pBaseWC, (bVert ? OX_LMSI_RIGHT : OX_LMSI_BOTTOM));
			if (nBasePos2 == OX_LMPOS_NULL)
				return OX_LMPOS_NULL;
		}
	}

	// compose new position using base positions
	if (d2 == OX_LMOFFSET_ANY)
		p = nBasePos1 + d1;
	else if (d1 == OX_LMOFFSET_ANY)
		p = nBasePos2 + d2;
	else
	{
		ASSERT(d2);
		p = nBasePos1 + (int)((nBasePos2 - nBasePos1) * d1 / d2);
	}

	ASSERT(p != OX_LMPOS_NULL && p != OX_LMPOS_TRACING);
	return p;
}

void COXLayoutManager::SetDefaultConstraint(UINT nChildWnd)
{
	ASSERT(m_pContainerWnd);
	
	CWnd* pWnd = m_pContainerWnd->GetDlgItem(nChildWnd);
	ASSERT(pWnd);
	if (pWnd == NULL)
	{
		TRACE0("COXLayoutManager::SetDefaultConstraint(): failed. Default constraint can only be applied after window has been created.\r\n");
		return;
	}

	CRect rect;
	pWnd->GetWindowRect(rect);
	m_pContainerWnd->ScreenToClient(rect);

	SetConstraint(nChildWnd, OX_LMS_TOP, OX_LMT_SAME, rect.top);
	SetConstraint(nChildWnd, OX_LMS_LEFT, OX_LMT_SAME, rect.left);
	RemoveConstraint(nChildWnd, OX_LMS_BOTTOM);
	RemoveConstraint(nChildWnd, OX_LMS_RIGHT);
}

BOOL COXLayoutManager::TieChild(UINT nChildWnd, int nSide, int nType, 
								UINT nBaseWnd /* = 0 */)
{
	ASSERT(nChildWnd);
	ASSERT(nType == OX_LMT_SAME || nType == OX_LMT_OPPOSITE);

	if(nType!=OX_LMT_SAME && nType!=OX_LMT_OPPOSITE)
		return FALSE;

	CWnd* pBaseWnd=(nBaseWnd==0 ? m_pContainerWnd : 
		m_pContainerWnd->GetDlgItem(nBaseWnd));
	ASSERT(pBaseWnd!=NULL);
	CRect rectBase;
	if(pBaseWnd->GetSafeHwnd()==m_pContainerWnd->GetSafeHwnd())
		pBaseWnd->GetClientRect(rectBase);
	else
	{
		pBaseWnd->GetWindowRect(rectBase);
		m_pContainerWnd->ScreenToClient(rectBase);
	}

	CWnd* pChildWnd=m_pContainerWnd->GetDlgItem(nChildWnd);
	ASSERT(pChildWnd!=NULL);
	CRect rectChild;
	pChildWnd->GetWindowRect(rectChild);
	m_pContainerWnd->ScreenToClient(rectChild);

	BOOL bResult=TRUE;
	if(bResult && (nSide&OX_LMS_TOP)!=0)
	{
		if(nType==OX_LMT_SAME)
			bResult&=SetConstraint(nChildWnd,OX_LMS_TOP,OX_LMT_SAME,
				rectChild.top-rectBase.top,nBaseWnd);
		else
			bResult&=SetConstraint(nChildWnd,OX_LMS_TOP,OX_LMT_OPPOSITE,
				rectChild.top-rectBase.bottom,nBaseWnd);
	}

	if(bResult && (nSide&OX_LMS_LEFT)!=0)
	{
		if(nType==OX_LMT_SAME)
			bResult&=SetConstraint(nChildWnd,OX_LMS_LEFT,OX_LMT_SAME,
				rectChild.left-rectBase.left,nBaseWnd);
		else
			bResult&=SetConstraint(nChildWnd,OX_LMS_LEFT,OX_LMT_OPPOSITE,
				rectChild.left-rectBase.right,nBaseWnd);
	}

	if(bResult && (nSide&OX_LMS_BOTTOM)!=0)
	{
		if(nType==OX_LMT_SAME)
			bResult&=SetConstraint(nChildWnd,OX_LMS_BOTTOM,OX_LMT_SAME,
				rectChild.bottom-rectBase.bottom,nBaseWnd);
		else
			bResult&=SetConstraint(nChildWnd,OX_LMS_BOTTOM,OX_LMT_OPPOSITE,
				rectChild.bottom-rectBase.top,nBaseWnd);
	}

	if(bResult && (nSide&OX_LMS_RIGHT)!=0)
	{
		if(nType==OX_LMT_SAME)
			bResult&=SetConstraint(nChildWnd,OX_LMS_RIGHT,OX_LMT_SAME,
				rectChild.right-rectBase.right,nBaseWnd);
		else
			bResult&=SetConstraint(nChildWnd,OX_LMS_RIGHT,OX_LMT_OPPOSITE,
				rectChild.right-rectBase.left,nBaseWnd);
	}

	return bResult;
}

BOOL COXLayoutManager::SubclassContainer(CWnd* pContainerWnd)
	// --- In  : pContainerWnd : The container window 
	// --- Out : 
	// --- Returns : Whether it was successful or not
	// --- Effect : This function subclasses the container window 
{
	ASSERT(pContainerWnd != NULL);
	ASSERT(pContainerWnd->GetSafeHwnd() != NULL);
	ASSERT_VALID(pContainerWnd);

	if (m_pfnSuper != NULL)
	{
		// Already subclasses, check that hWnd and window proc is correct
		if ( (m_hWnd != pContainerWnd->GetSafeHwnd()) || 
		     ((WNDPROC)(LONG_PTR)::GetWindowLongPtr(pContainerWnd->GetSafeHwnd(), GWLP_WNDPROC) != GlobalLayoutManagerProc) )
		{
			TRACE0("COXLayoutManager::SubclassContainer : Failed because already subclassed by other window proc\n");
			return FALSE;
		}
		return TRUE;
	}

	ASSERT(m_hWnd == NULL);
	ASSERT(m_pfnSuper == NULL);
#ifdef _DEBUG
	COXLayoutManager* pDummyLayoutManager = NULL;
	// ... Should not yet be in list of subclassed layout managers
	ASSERT(!m_allLayoutManagers.Lookup(m_hWnd, pDummyLayoutManager));
#endif // _DEBUG

	// ... Subclass window (Windows way, not MFC : because may already be subclessed by MFC)
	m_hWnd = pContainerWnd->GetSafeHwnd();
	m_pfnSuper = (WNDPROC)(LONG_PTR)::GetWindowLongPtr(pContainerWnd->GetSafeHwnd(), GWLP_WNDPROC);
	::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)GlobalLayoutManagerProc);

	// ... Store in the global map
	m_allLayoutManagers.SetAt(m_hWnd, this);

	ASSERT_VALID(this);
	return (m_hWnd != NULL);;
}

void COXLayoutManager::UnsubclassContainer()
	// --- In  : 
	// --- Out : 
	// --- Returns : 
	// --- Effect : This function unsubclasses the window 
	//				It removes this object from the double linked list
	//				When it is the last in the list it restores the original
	//				windows procedure
{
	ASSERT_VALID(this);

	if (m_hWnd != NULL)
	{
		// Put back original window procedure
		ASSERT(m_pfnSuper != NULL);
		ASSERT(m_pfnSuper != GlobalLayoutManagerProc);
		// ...	GlobalLayoutManagerProc is not used anymore : 
		//		set WNDPROC back to original value
		if(::IsWindow(m_hWnd))
			::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pfnSuper);
		// ... Remove use from global map
		m_allLayoutManagers.RemoveKey(m_hWnd);
		m_hWnd = NULL;
		m_pfnSuper = NULL;
	}

	ASSERT(m_hWnd == NULL);
	ASSERT(m_pfnSuper == NULL);

	ASSERT_VALID(this);
}

LRESULT CALLBACK COXLayoutManager::GlobalLayoutManagerProc(HWND hWnd, UINT uMsg, 
														   WPARAM wParam, 
														   LPARAM lParam)
	// --- In  : hWnd : 
	//			 uMsg : 
	//			 wParam : 
	//			 lParam :
	// --- Out : 
	// --- Returns : The result of the message
	// --- Effect : This is the global windows procedure of all the COXScrollTipOwner
	//              objects that have subclasses a window
{
#if defined (_WINDLL)
#if defined (_AFXDLL)
	AFX_MANAGE_STATE(AfxGetAppModuleState());
#else
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
#endif

	COXLayoutManager* pLayoutManager = NULL;

	VERIFY(m_allLayoutManagers.Lookup(hWnd, pLayoutManager));
	ASSERT_VALID(pLayoutManager);
	return pLayoutManager->LayoutManagerProc(hWnd, uMsg, wParam, lParam);
}

LRESULT COXLayoutManager::LayoutManagerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	// --- In  : hWnd : 
	//			 uMsg : 
	//			 wParam : 
	//			 lParam :
	// --- Out : 
	// --- Returns : The result of the message
	// --- Effect : This is the member function called by the windows procedure of the 
	//				COXLayoutManager object
{
#ifdef _WINDLL 
#ifndef _AFXEXT
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
#endif

	ASSERT_VALID(this);
	ASSERT(hWnd == m_hWnd);
	ASSERT(hWnd == m_hWnd);

	// Handling before base class
	switch (uMsg)
	{
		case WM_SIZE:
			OnSize(LOWORD(lParam), HIWORD(lParam));
			break;
		default:
			// Do nothing special
			;
	}

	// Let the original message procedure handle it
	ASSERT(m_pfnSuper != NULL);
	ASSERT(m_pfnSuper != GlobalLayoutManagerProc);
	LRESULT result = CallWindowProc(m_pfnSuper,hWnd, uMsg, wParam, lParam);

	// Handling after base class

	return result;
}


/////////////////////////////////////////////////////////////////////////////
// COXSideConstraint
//
//	int	nBaseWnd;		base window's ID
//	int	nOffset1;		offset value from the top/left side of the base window
//							if OX_LMOFFSET_ANY, no constraint form this side
//	int	nOffset2;		offset value from the bottom/right side of the base window
//							if OX_LMOFFSET_ANY, no constraint form this side
//	int	nPos;			cache for storing calculated position
//	int nBaseWndIndex;		base window's index in the m_wcTable
//
//	NOTE: if neither nOffset1 nor nOffset2 is OX_LMOFFSET_ANY, indicating
//		  "rubber band" OX_LMT_POSITION.

COXSideConstraint::COXSideConstraint()
: nBaseWnd( 0 )
, nOffset1( OX_LMOFFSET_ANY )
, nOffset2( OX_LMOFFSET_ANY )
, nPos( OX_LMPOS_NULL )
, nBaseWndIndex( -1 )
{
}

void COXSideConstraint::Empty()
	// --- In      :
	// --- Out     : 
	// --- Returns :
	// --- Effect  : set default empty values of a side constraint
{
	nBaseWnd = 0;
	nOffset1 = nOffset2 = OX_LMOFFSET_ANY;
	nPos = OX_LMPOS_NULL;
}

BOOL COXSideConstraint::IsEmpty()
	// --- In      :
	// --- Out     : 
	// --- Returns : TRUE if there is a constraint; FALSE otherwise
	// --- Effect  : determine if there is a constraint
{
	return ((nOffset1 == OX_LMOFFSET_ANY) && (nOffset2 == OX_LMOFFSET_ANY));
}

/////////////////////////////////////////////////////////////////////////////
// COXWndConstraint
//
//	int nID;				ID of this window
//	BOOL bHasMinMax;		TRUE if sizeMin or sizeMax is valid, FALSE otherwise 
//	CSize sizeMin;			minimum size
//	CSize sizeMax;			maximum size
//	COXSideConstraint sc[4];	four side constraints
//							this array uses OX_INDEX_TO_LMS[] and 
//							OX_LMS_TO_INDEX[] to map between index and 
//							side parameter (input by user)

COXWndConstraint::COXWndConstraint(UINT nChildID)
{
	Empty();
	nID = nChildID;
}

void COXWndConstraint::Empty()
	// --- In      :
	// --- Out     :  
	// --- Returns :
	// --- Effect  : set default empty values of a window constraint
{
	bHasMinMax = FALSE;
	sizeMin = sizeMax = CSize(0,0);
	for (int i = 0; i < 4; i++)
		sc[i].Empty();
}

void COXWndConstraint::ResetPos()
	// --- In      :
	// --- Out     :  
	// --- Returns :
	// --- Effect  : reset positions to null if it's needed to calculate
{
	if (!sc[OX_LMSI_TOP].IsEmpty() || !sc[OX_LMSI_BOTTOM].IsEmpty())
		sc[OX_LMSI_TOP].nPos = sc[OX_LMSI_BOTTOM].nPos = OX_LMPOS_NULL;
	if (!sc[OX_LMSI_LEFT].IsEmpty() || !sc[OX_LMSI_RIGHT].IsEmpty())
		sc[OX_LMSI_LEFT].nPos = sc[OX_LMSI_RIGHT].nPos = OX_LMPOS_NULL;
}

CRect COXWndConstraint::GetRect()
	// --- In      :
	// --- Out     : 
	// --- Returns : rect of calculated positions (new window coordinates)
	// --- Effect  : get calculated positions from a window constraint
{
	return CRect(sc[OX_LMSI_LEFT].nPos, sc[OX_LMSI_TOP].nPos, 
		sc[OX_LMSI_RIGHT].nPos, sc[OX_LMSI_BOTTOM].nPos);
}

/////////////////////////////////////////////////////////////////////////////

// end of OXLayoutManager.cpp
