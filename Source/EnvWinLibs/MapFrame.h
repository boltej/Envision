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
#if !defined(AFX_MAPFRAME_H__27CF1BB8_178F_11D3_95A0_00A076B0010A__INCLUDED_)
#define AFX_MAPFRAME_H__27CF1BB8_178F_11D3_95A0_00A076B0010A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapWnd.h : header file
//

class Map;
class MapLayer;
class Poly;
class CellPropDlg;

#include "EnvWinLibs.h"

#include "MapListWnd.h"
#include "MapWnd.h"
#include <StaticSplitterWnd.h>


void WINLIBSAPI PopulateFieldsMenu( Map *pMap, MapLayer *pLayer, CMenu &mainMenu, bool addAdditionalLayers );



/////////////////////////////////////////////////////////////////////////////
// MapFrame window - includes splitter window with map legend, MapWindow

class WINLIBSAPI MapFrame : public CWnd
{
// Construction
DECLARE_DYNAMIC(MapFrame)

public:
	MapFrame();
   MapFrame( MapFrame & );  // copy constructor

// Attributes
public:
   StaticSplitterWnd m_splitterWnd;
 
   Map         *m_pMap;         // pointers to the two contained windows
   MapWindow   *m_pMapWnd;
   MapListWnd  *m_pMapList;

public:
   CellPropDlg *m_pCellPropDlg;
   int          m_currentPoly;   // set after clicking...index of the clicked polygon

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(MapWnd)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~MapFrame();

   void Update( void );  // int year );

   void RefreshList( void ) { if ( m_pMapList != NULL ) m_pMapList->Refresh(); }

   void SetActiveField( int col, bool redraw );


protected:
	// Generated message map functions
public:
	//{{AFX_MSG(MapFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEditCopy();
	afx_msg void OnCellproperties();
	afx_msg void OnShowSameAttribute();
   afx_msg void OnShowPolygonVertices();
   afx_msg void OnZoomToPoly();
	afx_msg void OnReclassifydata();
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
   afx_msg void OnSelectField(UINT nID);
   afx_msg void RouteToParent(UINT nID);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
  };

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPWND_H__27CF1BB8_178F_11D3_95A0_00A076B0010A__INCLUDED_)
