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

/**********************************************************************
**
**	ThemeUtil.h : include file
**
**	by Andrzej Markowski June 2004
**
**********************************************************************/

//#include "TmSchema.h"

//#define WM_THEMECHANGED      0x031A
#define THM_WM_THEMECHANGED     0x031A

typedef struct _MY_MARGINS
{
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
} MY_MARGINS;

class CThemeUtil
{
public:
	CThemeUtil();
	virtual ~CThemeUtil();
	BOOL OpenThemeData(HWND hWnd, LPCWSTR pszClassList);
	void CloseThemeData();
	BOOL DrawThemePart(HDC hdc, int iPartId, int iStateId, const RECT *pRect);
	BOOL GetThemeColor(int iPartId, int iStateId, int iPropId, const COLORREF *pColor);
	BOOL GetThemeInt(int iPartId, int iStateId, int iPropId, const int *piVal);
	BOOL GetThemeMargins(int iPartId, int iStateId, int iPropId, const MY_MARGINS *pMargins);
	BOOL GetThemeEnumValue(int iPartId, int iStateId, int iPropId, const int *piVal);
	BOOL GetThemeFilename(int iPartId, int iStateId, int iPropId, 
							OUT LPWSTR pszThemeFileName, int cchMaxBuffChars);
	BOOL GetCurrentThemeName(OUT LPWSTR pszThemeFileName, int cchMaxNameChars, 
							OUT OPTIONAL LPWSTR pszColorBuff, int cchMaxColorChars,
							OUT OPTIONAL LPWSTR pszSizeBuff, int cchMaxSizeChars);
	HBITMAP LoadBitmap(LPWSTR pszBitmapName);
private:
	BOOL IsWinXP(void);
	void FreeLibrary();
private:
	HINSTANCE m_hUxThemeDll;
	HINSTANCE m_hRcDll;
	UINT m_hTheme;
};
