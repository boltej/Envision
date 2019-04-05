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


#include <MapFrame.h>
#include <MapWnd.h>
#include <MAP.h>
#include <PtrArray.h>

// MapPanel

class MapPanel : public CWnd
{
	DECLARE_DYNAMIC(MapPanel)

public:
	MapPanel();
	virtual ~MapPanel();

   MapFrame   m_mapFrame;
   MapWindow *m_pMapWnd;
   Map       *m_pMap;

   //PtrArray< MapWnd > m_mapWndArray;  // array of map window, all connected to the same map


   //void DrawText( LPCTSTR text, int x, int y, int pts );
   void UpdateText   ( LPCTSTR text ) { this->m_pMapWnd->SetMapText( 0, text, true ); }    // 20, 180 ); }
   void UpdateSubText( LPCTSTR text ) { m_pMapWnd->SetMapText( 1, text, true ); }    // 50, 100 ); }
   void UpdateLLText ( LPCTSTR text ) { m_pMapWnd->SetMapText( 2, text, true ); }    // 50, 100 ); }
   void UpdateULText ( LPCTSTR text ) { m_pMapWnd->SetMapText( 3, text, true ); }    // 50, 100 ); }

   //void SetActiveField( int col, bool redraw ) { m_pMap->SetActiveField( col, redraw ); }
	//afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

protected:
	DECLARE_MESSAGE_MAP()

   //MapText *m_pText;
   //MapText *m_pSubtext;
   //MapText *m_pLLText;
   
   //void UpdateTextEx( LPCTSTR subtext, CRect &rect, int top, int ptSize );
   //CRect m_lastTextRect;
   //CRect m_lastSubtextRect;
   
public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
   
   afx_msg void OnEditCopy();
	afx_msg void OnShowresults();
	afx_msg void OnUpdateShowresults(CCmdUI* pCmdUI);
	afx_msg void OnShowdeltas();
	afx_msg void OnUpdateShowdeltas(CCmdUI* pCmdUI);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnRunQuery();
   afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	};


