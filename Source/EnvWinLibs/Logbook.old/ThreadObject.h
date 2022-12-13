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
// ThreadObject.h
//
// Author : Marquet Mike
//
// Company : /
//
// Date of creation  : 14/04/2003
// Last modification : 14/04/2003
// ==========================================================================

#if !defined(AFX_THREADOBJECT_H__6AC375FE_7271_4B17_935F_98F9EBA517FC__INCLUDED_)
#define AFX_THREADOBJECT_H__6AC375FE_7271_4B17_935F_98F9EBA517FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ==========================================================================
// Les Includes
// ==========================================================================

#include "Logbook.h"

/////////////////////////////////////////////////////////////////////////////
// class CThreadObject

class WINLIBSAPI CThreadObject : public CObject
 {
  public :
           LOGBOOKITEM_INFO m_stItemInfo;
           SYSTEMTIME       m_stST;
           BOOL             m_bDisplayTime;
           BOOL             m_bDisplayType;

           CThreadObject();
           virtual ~CThreadObject();
 };

// ==========================================================================
// ==========================================================================

#endif // !defined(AFX_THREADOBJECT_H__6AC375FE_7271_4B17_935F_98F9EBA517FC__INCLUDED_)
