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
// PPRunData.cpp : implementation file
//

#include "stdafx.h"
#include "Envision.h"
#include "resource.h"
#include "PPRunData.h"


// PPRunData dialog

IMPLEMENT_DYNAMIC(PPRunData, CPropertyPage)

PPRunData::PPRunData()
	: CPropertyPage(PPRunData::IDD)
   , m_collectActorData(FALSE)
   , m_collectLandscapeScoreData(FALSE)
   {
}

PPRunData::~PPRunData()
{
}

void PPRunData::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Check(pDX, IDC_COLLECTACTORDATA, m_collectActorData);
DDX_Check(pDX, IDC_COLLECTLANDSCAPESCOREDATA, m_collectLandscapeScoreData);
}


BEGIN_MESSAGE_MAP(PPRunData, CPropertyPage)
END_MESSAGE_MAP()


// PPRunData message handlers
