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
#include "afxcmn.h"

#include <PtrArray.h>


// UpdateDlg dialog

class StudyAreaInfo
   {
   public:
      CString m_name;
      CString m_path;
      CString m_localVer;
      CString m_webVer;
   };

class UpdateDlg : public CDialogEx
{
	DECLARE_DYNAMIC(UpdateDlg)

public:
	UpdateDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~UpdateDlg();

// Dialog Data
	enum { IDD = IDD_UPDATE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   PtrArray< StudyAreaInfo > m_saArray;

	DECLARE_MESSAGE_MAP()
public:
   CStatic m_versionInfo;
   BOOL m_disable;
   
   bool Init();  // return true to run dialg, false to not run dialog

   virtual BOOL OnInitDialog();
   virtual void OnCancel();
   virtual void OnOK();

   void SaveUpdateStatusToReg( void );

   CFont m_updateFont;

   
   CListCtrl m_studyAreas;
   CStatic m_updateText;

   CString m_updateMsg;
   };
