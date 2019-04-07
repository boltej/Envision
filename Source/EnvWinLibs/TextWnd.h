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

///////////////////////////////////////////////////////////////////////
// TextWnd

class WINLIBSAPI TextWnd : public CWnd
{

public:
	TextWnd();
	virtual ~TextWnd();

protected:
	CFont m_font;
	int   m_cxChar;
	int   m_cyChar;
   int   m_indent;
   int   m_spacesPerIndent;

   bool m_current;
   int  m_width;
   int  m_height;
   int  m_xOffset;
   int  m_yOffset;

   bool m_showNewText;   //If true, automaticly scrolls to show appended text

	CStringArray m_strArray;
   
public:
	void AppendText( LPCTSTR text );
   void SetText( LPCTSTR text ){ ClearAll(); AppendText(text); }
   void ClearAll( void );

   void UpIndent();
   void DownIndent();
   void ResetIndent(){ m_indent = 0; }

   int  GetCharWidth() { return m_cxChar; } // returns 0 before a paint
   int  GetCharHeight(){ return m_cyChar; } // returns 0 before a paint

protected:
   void UpdateDimensions();

protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
	afx_msg void OnPaint();
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
