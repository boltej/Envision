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
// XGlyphButton.cpp  Version 1.1
//
// Author: Hans Dietrich
//         hdietrich2@hotmail.com
//
// This software is released into the public domain.  You are free to use it
// in any way you like, except that you may not sell this source code.
//
// This software is provided "as is" with no expressed or implied warranty.
// I accept no liability for any damage or loss of business that this software
// may cause.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XGLYPHBUTTON_H
#define XGLYPHBUTTON_H

#include "EnvWinLibs.h"


/////////////////////////////////////////////////////////////////////////////
// CXGlyphButton window

class WINLIBSAPI CXGlyphButton : public CButton
{
// Construction/Destruction
public:
	CXGlyphButton();
	virtual ~CXGlyphButton();

	// some predefined glyphs from WingDings
	enum 
	{
		BTN_DELETE            = 0xFB, 
		BTN_CHECK             = 0xFC,
		BTN_LEFTARROW         = 0xDF,
		BTN_RIGHTARROW        = 0xE0,
		BTN_UPARROW           = 0xE1,
		BTN_DOWNARROW         = 0xE2,
		BTN_HOLLOW_LEFTARROW  = 0xEF,
		BTN_HOLLOW_RIGHTARROW = 0xF0,
		BTN_HOLLOW_UPARROW    = 0xF1,
		BTN_HOLLOW_DOWNARROW  = 0xF2
	};

	void SetCharSet(BYTE bCharSet);
	void SetFaceName(LPCTSTR lpszFaceName);
	void SetFont(LOGFONT* plf);
	void SetFont(CFont* pFont);
	void SetGlyph(UINT cGlyph);
	void SetGlyph(LOGFONT* plf, UINT cGlyph);
	void SetGlyph(CFont* pFont, UINT cGlyph);
	void SetGlyph(LONG lHeight, LONG lPointSize, LONG lWeight, 
			LPCTSTR lpszFaceName, UINT cGlyph);
	void SetGlyph(LONG lPointSize, LPCTSTR lpszFaceName, UINT cGlyph);
	void SetHeight(LONG lHeight);
	void SetPointSize(LONG lPointSize);
	void SetWeight(LONG lWeight);
	void SetWingDingButton(UINT nButton);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXGlyphButton)
protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
protected:
	LONG GetFontHeight(LONG nPointSize);
	LONG GetFontPointSize(LONG nHeight);
	void ReconstructFont();

	LOGFONT		m_lf;
	UINT		m_cGlyph;
	CFont		m_font;

	// Generated message map functions

	//{{AFX_MSG(CXGlyphButton)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //XGLYPHBUTTON_H
