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
#include "afxcmn.h"

#include <map.h>

// SpatialIndexDlg dialog

class SpatialIndexDlg : public CDialog
{
	DECLARE_DYNAMIC(SpatialIndexDlg)

public:
	SpatialIndexDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~SpatialIndexDlg();

   static int MapProc( Map *pMap, NOTIFY_TYPE type, int a0, LONG_PTR a1, LONG_PTR extra );

// Dialog Data
	enum { IDD = IDD_SPATIALINDEX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

   bool m_canceled;
   bool m_inBuild;

public:
   CProgressCtrl m_progress;
   
protected:
   virtual void OnOK();
   virtual void OnCancel();
public:
   int m_maxDistance;
   };
