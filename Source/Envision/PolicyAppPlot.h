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

#include <EasySize.h>
#include <ScatterWnd.h>
#include <FDataObj.h>

#include "Resource.h"
#include "afxwin.h"

// PolicyAppPlot dialog

class PolicyAppPlot : public CDialog
{
	DECLARE_DYNAMIC(PolicyAppPlot)

   DECLARE_EASYSIZE

public:
	PolicyAppPlot( int run );   // standard constructor
	virtual ~PolicyAppPlot();

   int m_run;

   CComboBox m_targetMetagoalCombo;
   CComboBox m_toleranceCombo;

   ScatterWnd *m_pPlot;
   FDataObj   *m_pData;

// Dialog Data
	enum { IDD = IDD_POLICYAPPPLOT };

protected:
   void RefreshPlot();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL Create( const RECT &rect, CWnd* pParentWnd = NULL);
   virtual BOOL OnInitDialog();
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnChangeEffectiveness();
   afx_msg void OnChangeTolerance();
protected:
   virtual void OnCancel();
   virtual void OnOK();   
   
};
