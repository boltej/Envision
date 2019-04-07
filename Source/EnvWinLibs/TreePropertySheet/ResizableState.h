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
// ResizableState.h: interface for the CResizableState class.
//
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2000-2002 by Paolo Messina
// (http://www.geocities.com/ppescher - ppescher@yahoo.com)
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RESIZABLESTATE_H__INCLUDED_)
#define AFX_RESIZABLESTATE_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CResizableState  
{
protected:
	// non-zero if successful
	BOOL LoadWindowRect(LPCTSTR pszSection, BOOL bRectOnly);
	BOOL SaveWindowRect(LPCTSTR pszSection, BOOL bRectOnly);

	virtual CWnd* GetResizableWnd() = 0;

public:
	CResizableState();
	virtual ~CResizableState();
};

#endif // !defined(AFX_RESIZABLESTATE_H__INCLUDED_)
