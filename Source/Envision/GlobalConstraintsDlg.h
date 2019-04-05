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


class PolEditor;


// GlobalConstraintsDlg dialog

class GlobalConstraintsDlg : public CDialog
{
	DECLARE_DYNAMIC(GlobalConstraintsDlg)

public:
	GlobalConstraintsDlg(PolEditor *pPolEditor, CWnd* pParent = NULL);   // standard constructor
	virtual ~GlobalConstraintsDlg();

   PolEditor *m_pPolEditor;

   bool StoreGlobalChanges( void );
   void LoadGlobalConstraint( void );

   CListBox m_globalConstraints;
   CString m_maxAnnualValueExpr;

// Dialog Data
	enum { IDD = IDD_GLOBALCONSTRAINTS };

protected:
   int m_globalConstraintIndex;   // current global constraint

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void MakeDirtyGlobal( void );
   void EnableControls( void );

	DECLARE_MESSAGE_MAP()

   virtual BOOL OnInitDialog();

   afx_msg void OnBnClickedResourceLimit();  // for all buttons
   afx_msg void OnBnClickedAddgc();
   afx_msg void OnBnClickedDeletegc();
   afx_msg void OnLbnSelchangeGlobalconstraints();
   afx_msg void OnEnChangeMaxAnnualValue();

};
