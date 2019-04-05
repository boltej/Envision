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

#include <TabPageSSL.h>
#include "resource.h"
#include <Policy.h>
#include "afxwin.h"


class PolEditor;


// PPPolicyConstraints dialog

class PPPolicyConstraints : public CTabPageSSL
{
friend class PolEditor;

DECLARE_DYNAMIC(PPPolicyConstraints)

public:
	PPPolicyConstraints( PolEditor*, Policy *&pPolicy );
	virtual ~PPPolicyConstraints();

// Dialog Data
	enum { IDD = IDD_POLICYCONSTRAINTS };

protected:
   Policy    *&m_pPolicy;
   PolEditor  *m_pParent;
   
   int m_policyConstraintIndex;   // current policy constraint

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   // Generated message map functions
   virtual BOOL OnInitDialog();

   void MakeDirtyPolicy( void );
   void LoadPolicy( void );
   bool StorePolicyChanges( void );
   void EnableControls( void );
   void LoadPolicyConstraint( void );
   void LoadGCCombo( void );

public:
   CString m_lookupExpr;
   CListBox m_policyConstraints;

   CComboBox m_assocGC;
   CString m_initCostExpr;
   CString m_maintenanceCostExpr;
   CString m_durationExpr;

   afx_msg void OnBnClickedAddpolicyconstraint();
   afx_msg void OnBnClickedDeletepolicyconstraint();
   afx_msg void OnLbnSelchangePolicyconstraints();
   afx_msg void OnEnChangeLookupTable();
   afx_msg void OnBnClickedUnitArea();  // used for both unit area and absolute
   afx_msg void OnCbnSelchangeGccombo();
   afx_msg void OnEnChangeInitCost();
   afx_msg void OnEnChangeMaintenanceCost();
   afx_msg void OnEnChangeDuration();
   afx_msg void OnBnClickedAddglobalconstraint();
   };
