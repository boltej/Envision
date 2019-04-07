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


// StaticSplitterWnd

class WINLIBSAPI StaticSplitterWnd : public CWnd
{
	DECLARE_DYNAMIC(StaticSplitterWnd)

public:
	StaticSplitterWnd();
	virtual ~StaticSplitterWnd();

   void LeftRight( CWnd *pLeft, CWnd *pRight );
   void MoveBar( int newLocation );
   int GetBarLoc() const { return m_bar; }
   int GetBarWidth() const { return m_barWidth; }
   void Draw();

protected:
    CWnd *m_pLeft;
    CWnd *m_pRight;

    int  m_bar;
    int  m_barWidth;
    bool m_inBarMove;
    int  m_mouseBarOffset;

protected:
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};


