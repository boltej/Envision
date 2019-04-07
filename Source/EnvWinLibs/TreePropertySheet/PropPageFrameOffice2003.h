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
// PropPageFrameOffice2003.h: interface for the CPropPageFrameOffice2003 class.
//
//////////////////////////////////////////////////////////////////////
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

#ifndef _PROPPAGEFRAMEOFFICE2003_H__INCLUDED_
#define _PROPPAGEFRAMEOFFICE2003_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPageFrameEx.h"

namespace TreePropSheet
{
//! Forward declaration of helper class for XP Theme.
class CThemeLibEx;

/*! @brief Frame class thats draws a simple border and header similar
           to option dialog in Office 2003

  CPropPageFrameOffice2003 ensures that the frame draws a border
  on all systems. This is done by setting the extended style of
  the window to WS_EX_CLIENTEDGE on non theme systems and drawing 
  a simple rectangle on theme systems. The header is bold text and a 
  simple line is drawn as a separator. 

  @version 0.1
  @author Yves Tkaczyk <yves@tkaczyk.net> 
  @date 09/2004 */
class CPropPageFrameOffice2003
 : public CPropPageFrameEx  
{
// Construction/Destruction
public:
	CPropPageFrameOffice2003();
	virtual ~CPropPageFrameOffice2003();

// CPropPageFrame implementation
public:
	/*! Create the Windows window for the frame.
	  @param dwWindowStyle Specifies the window style attributes
	  @param rect The size and position of the window, in client coordinates of pParentWnd.
	  @param pwndParent The parent window. This cannot be NULL.
	  @param nID The ID of the child window.
	  @return Nonzero if successful; otherwise 0. */
  virtual BOOL Create(DWORD dwWindowStyle,const RECT &rect,CWnd *pwndParent,UINT nID);

// CPropPageFrame overridings
protected:
	virtual void DrawCaption(CDC *pDc, CRect rect, LPCTSTR lpszCaption, HICON hIcon);

// CPropPageFrameEx overrides
  /*! Draw the background.
    @param pDC Pointer to device context. */
  virtual void DrawBackground(CDC* pDc);
};

}; // namespace TreePropSheet

#endif // _PROPPAGEFRAMEOFFICE2003_H__INCLUDED_
