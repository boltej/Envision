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

#include <map.h>
#include "EnvWinLib_resource.h"

// MapLabelDlg dialog

class MapLabelDlg : public CDialog
{
	DECLARE_DYNAMIC(MapLabelDlg)

public:
	MapLabelDlg(Map *pMap, CWnd* pParent = NULL);   // standard constructor
	virtual ~MapLabelDlg();

   Map      *m_pMap;
   MapLayer *m_pLayer;
   LOGFONT   m_logFont;
   COLORREF  m_color;
   CFont     m_font;
   bool      m_isDirty;

// Dialog Data
	enum { IDD = IDD_LIB_MAPLABELS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void LoadFields();

	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnBnClickedSetfont();
   afx_msg void OnBnClickedShowlabels();
   afx_msg void OnBnClickedOk();
   CStatic m_sample;
   CComboBox m_layers;
   CComboBox m_fields;
   virtual BOOL OnInitDialog();
   afx_msg void OnCbnEditchangeLayers();
   BOOL m_showLabels;
   };
