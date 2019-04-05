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

#include <EnvExtension.h>
#include <PtrArray.h>
#include <EnvContext.h>
#include <CustomTabCtrl\CustomTabCtrl.h>


// InputPanel is the main container  it contains an...
//    InputTab control. This tab control manages...
//       InputWnd's, which are the parent window for Input Visualizers

class InputWnd : public CWnd
{
public:
   EnvVisualizer *m_pViz;

public:
   DECLARE_MESSAGE_MAP()
   afx_msg void OnSize(UINT nType, int cx, int cy);
 
};


class InputPanel : public CWnd
{
public:
   InputPanel() : CWnd(), m_pActiveWnd( NULL ) { }
   ~InputPanel();

   InputWnd *AddInputWnd( EnvVisualizer *pViz );
   InputWnd *GetActiveWnd();

protected:
   PtrArray< InputWnd > m_inputWndArray;
   InputWnd *m_pActiveWnd;

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


