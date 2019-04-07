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

//#include <afxwin.h>


/*============================================================================*/
// CDragCheckListBox

class CDragCheckListBox : public CCheckListBox
{
	DECLARE_DYNAMIC(CDragCheckListBox)

// Constructors
public:
	CDragCheckListBox();

// Attributes

	// find item index at given point
	int ItemFromPt(_In_ CPoint pt, _In_ BOOL bAutoScroll = TRUE) const;

// Operations

#pragma push_macro("DrawInsert")
#undef DrawInsert
	// draws insertion line
	virtual void DrawInsert(_In_ int nItem);
#pragma pop_macro("DrawInsert")

// Overridables

	// Override to respond to beginning of drag event.
	virtual BOOL BeginDrag(_In_ CPoint pt);

	// Overrdie to react to user cancelling drag.
	virtual void CancelDrag(_In_ CPoint pt);

	// Called as user drags. Return constant indicating cursor.
	virtual UINT Dragging(_In_ CPoint pt);

	// Called when user releases mouse button to end drag event.
	virtual void Dropped(_In_ int nSrcIndex, _In_ CPoint pt);

// Implementation
public:
	int m_nLast;
	void DrawSingle(_In_ int nIndex);
	virtual void PreSubclassWindow();
	virtual ~CDragCheckListBox();

protected:
	virtual BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*);
};
