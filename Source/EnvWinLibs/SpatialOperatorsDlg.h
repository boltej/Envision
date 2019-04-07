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
#include "afxwin.h"
#include "EnvWinLibs.h"
#include "EnvWinLib_resource.h"


// SpatialOperatorsDlg dialog

class WINLIBSAPI SpatialOperatorsDlg : public CDialog
{
	DECLARE_DYNAMIC(SpatialOperatorsDlg)

public:
	SpatialOperatorsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~SpatialOperatorsDlg();

// Dialog Data
	enum { IDD = IDD_LIB_SPATIALOPERATORS };
   int m_group;
   float m_distance;
   float m_areaThreshold;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   CEdit m_comment;
   virtual BOOL OnInitDialog();
protected:
   virtual void OnOK();
public:
   afx_msg void OnBnClickedNextto();
   afx_msg void OnBnClickedNexttoarea();
   afx_msg void OnBnClickedWithin();
   afx_msg void OnBnClickedWithinarea();
};
