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
// ThreadObject.cpp
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

#include "ThreadObject.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// class CThreadObject

CThreadObject::CThreadObject()
 {
  CLogbook::LogbookItemCopy(NULL, m_stItemInfo);

  m_bDisplayTime = TRUE;
  m_bDisplayType = TRUE;

  memset(&m_stST, 0, sizeof(SYSTEMTIME));
 }

// --------------------------------------------------------------------------

CThreadObject::~CThreadObject()
 {
 }

// --------------------------------------------------------------------------
