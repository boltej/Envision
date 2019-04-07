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

#include "EnvWinLib_resource.h"
#include "afxcmn.h"

#include "Maplayer.h"
//#include "DBTABLE.H"


// FieldSorterDlg dialog

class FieldSorterDlg : public CDialog
{
	DECLARE_DYNAMIC(FieldSorterDlg)

public:
	FieldSorterDlg(MapLayer *pLayer, CWnd* pParent = NULL);   // standard constructor
	virtual ~FieldSorterDlg();

// Dialog Data
	enum { IDD = IDD_LIB_SORTCOLS };

   MapLayer *m_pMapLayer;
   //DbTable *m_pDbTable;


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   CDragListBox m_fields;
	virtual BOOL OnInitDialog();
   virtual void OnOK();
   };
