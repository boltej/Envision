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

#include "AnalyticsViewer.h"

#include <PtrArray.h>
#include <CustomTabCtrl\CustomTabCtrl.h>


// AnalyticsPanel is the main container  it contains an...
//    ViewerTab control. This tab control manages...
//       AnalyticsViewer's, which are the parent window for analytics visualizers


class AnalyticsPanel : public CWnd
{
public:
   AnalyticsPanel() : CWnd(), m_pActiveWnd( NULL ) { }
   ~AnalyticsPanel();

   AnalyticsViewer *AddViewer( );
   AnalyticsViewer *GetActiveView();
   int GetViewerCount() { return (int) m_viewerArray.GetSize(); }

   void Tile();
   void Cascade();
   void Clear() { m_viewerArray.RemoveAll(); m_pActiveWnd = NULL; RedrawWindow(); }

protected:
   PtrArray< AnalyticsViewer > m_viewerArray;
   AnalyticsViewer *m_pActiveWnd;

   CCustomTabCtrl m_tabCtrl;
   CFont m_font;

protected:
   void GetViewRect( RECT *rect );
   void ChangeTabSelection( int index );
 
	DECLARE_MESSAGE_MAP()

   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnAddWindow();

protected:
   //virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
   virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
public:
   afx_msg void OnPaint();
   };


