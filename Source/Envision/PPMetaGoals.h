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
#include <afxtempl.h>
#include "afxcmn.h"

// PPMetaGoals dialog

class PPMetaGoals : public CPropertyPage
{
	DECLARE_DYNAMIC(PPMetaGoals)

public:
	PPMetaGoals();   // standard constructor
	virtual ~PPMetaGoals();

// Dialog Data
	enum { IDD = IDD_RUNMETAGOALS };

   CArray <CSliderCtrl*, CSliderCtrl* > m_sliderArray;
   CArray <CStatic*, CStatic* > m_staticArray;

   CSliderCtrl m_altruismSlider;
   float       m_altruismWt;     // 0-1
   CSliderCtrl m_selfInterestSlider;
   float       m_selfInterestWt;
   CSliderCtrl m_policyPrefSlider;
   float       m_policyPrefWt;

   float GetGoalWeight( int i );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnDestroy();
   afx_msg void OnBnClickedBalance();
   afx_msg void OnWtSlider(UINT nSBCode, UINT nPos, CSliderCtrl* pSlider);
};
