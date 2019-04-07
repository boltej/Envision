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
// PropPageFrameBordered.h: interface for the CPropPageFrameBordered class.
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2004 by Yves Tkaczyk
// (http://www.tkaczyk.net - yves@tkaczyk.net)
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// Documentation: http://www.codeproject.com/property/treepropsheetex.asp
// CVS tree:      http://sourceforge.net/projects/treepropsheetex
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEPROPSHEET_PROPPAGEFRAMEBORDERED_H__INCLUDED_
#define _TREEPROPSHEET_PROPPAGEFRAMEBORDERED_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPageFrameEx.h"

namespace TreePropSheet
{
/*! \brief Frame class thats draws a border on non XP themed systems

  CPropPageFrameBordered ensures that the frame draws a border
  on all systems. This is done by setting the extended style of
  the window to WS_EX_CLIENTEDGE.
  Note: Using this class when the tree is resizable makes grabbing the
  splitter easier.

  @version 0.1
  @author Yves Tkaczyk <yves@tkaczyk.net>
  @date 07/2004 */
class CPropPageFrameBordered
 : public CPropPageFrameEx  
{
// Construction/Destruction
public:
	CPropPageFrameBordered();
	virtual ~CPropPageFrameBordered();

// CPropPageFrame implementation
public:
	/*! Create the Windows window for the frame.
	  @param dwWindowStyle Specifies the window style attributes
	  @param rect The size and position of the window, in client coordinates of pParentWnd.
	  @param pwndParent The parent window. This cannot be NULL.
	  @param nID The ID of the child window.
	  @return Nonzero if successful; otherwise 0. */
  virtual BOOL Create(DWORD dwWindowStyle,const RECT &rect,CWnd *pwndParent,UINT nID);

// CPropPageFrameEx overrides
  /*! Draw the background.
    @param pDC Pointer to device context. */
  virtual void DrawBackground(CDC* pDC);
};

}; // namespace TreePropSheet

#endif // _TREEPROPSHEET_PROPPAGEFRAMEBORDERED_H__INCLUDED_
