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

#include "resource.h"
#include "afxwin.h"

// PPRunActors dialog

class PPRunActors : public CPropertyPage
{
	DECLARE_DYNAMIC(PPRunActors)

public:
	PPRunActors();
	virtual ~PPRunActors();

// Dialog Data
	enum { IDD = IDD_RUNACTORS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnBnClickedGenerate();
   int m_count;
   afx_msg void OnBnClickedRandom();
   afx_msg void OnBnClickedUniform();
   afx_msg void OnBnClickedCurrent();
   afx_msg void OnBnClickedViewcurrent();
};
