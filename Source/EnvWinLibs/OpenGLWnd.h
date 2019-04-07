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
#if !defined(AFX_OPENGLWND_H__455CD4BE_C24C_4CF0_8289_73173B606EE5__INCLUDED_)
#define AFX_OPENGLWND_H__455CD4BE_C24C_4CF0_8289_73173B606EE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OpenGLWnd.h : header file
//
//* MS openGL libraries (link with OPENGL32.LIB and GLU32.LIB)

#include "EnvWinLibs.h"

#include <gl\gl.h>
#include <gl\glu.h>

#include <gl\glReal.h>
//#define GLdouble double
/////////////////////////////////////////////////////////////////////////////
// Global type definitions
	enum InfoField {VENDOR,RENDERER,VERSION,ACCELERATION,EXTENSIONS};
	enum ColorsNumber{INDEXED,THOUSANDS,MILLIONS,MILLIONS_WITH_TRANSPARENCY};
	enum ZAccuracy{NORMAL,ACCURATE};


/////////////////////////////////////////////////////////////////////////////
// COpenGLWnd window

class WINLIBSAPI COpenGLWnd : public CWnd
{
// Construction
public:
	COpenGLWnd();

/** CGLDispList
DESC:-this is an helper class which let you create "display list objects",
       use these objects to define the key elements in your scene (a disp.
	   list is faster than the corresponding GL commands).
      -Through the class members functions you have total control on a
       single display list.
      -An isolated display list save OGL parameters before execution
	   (so it's not affected by preceding transformations or settings).
*******/
	class CGLDispList
	{
	friend class COpenGLWnd;
	private:
		BOOL m_bIsolated;
		int m_glListId;
	public:
		CGLDispList();  // constructor
		~CGLDispList(); // destructor
		void StartDef(BOOL bImmediateExec=FALSE);// enclose a disp.list def.
		void EndDef();
		void Draw();// execute disp list GL commands 
		void SetIsolation(BOOL bValue) {m_bIsolated=bValue;}; // set isolation property
	};

// Attributes
public:

// Operations
public:

/* Stock Display lists functions
DESC.: these display lists are internally organized in a vector (20 max),
       you have control on definition and redrawing only. 
       use them for background elements which are to be drawn everytime
       all together.
NOTE: between BeginStockDispList and EndStockDispList should be present 
OpenGL calls only (see documentation for which are allowed and how are them treated)
*/
	void StartStockDListDef();	// allocates a new stock display list entry and opens a display list definition
	void EndStockListDef();		// closes a stock display list definition
	void DrawStockDispLists();	// executes all the stock display lists
	void ClearStockDispLists(); // deletes all the stock display lists
// Information retrieval function
	const CString GetInformation(InfoField type);
// Mouse cursor function
	void SetMouseCursor(HCURSOR mcursor=NULL);
// Attribute retrieval function
	double GetAspectRatio() {return m_dAspectRatio;};
// Rendering Context switching
	void BeginGLCommands();// use to issue GL commands outside Overridables
	void EndGLCommands();// i.e: in menu event handlers, button events handler etc.

// font stuff
	void MakeFont();
   void MakeFont3D();
	void PrintString(const char* str);
   void PrintString3D(const char* str);

   GLuint m_uiListID;
   glReal3 GetText3DExtent(CString str);
   glReal3 GetText3DExtent( const char* str ){ return GetText3DExtent( CString( str ) ); }

// Overridables
	virtual void OnCreateGL(); // override to set bg color, activate z-buffer, and other global settings
	virtual void OnSizeGL(int cx, int cy); // override to adapt the viewport to the window
	virtual void OnDrawGL(); // override to issue drawing functions
	virtual void OnDrawGDI(CPaintDC *pDC); // override to issue GDI drawing functions
	virtual void VideoMode(ColorsNumber &c,ZAccuracy &z,BOOL &dbuf); // override to specify some video mode parameters
// Overrides
// NOTE: these have been declared private because they shouldn't be
//		 overridden, use the provided virtual functions instead.
private:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COpenGLWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	void CopyToClipboard();
	void SetClearCol(COLORREF rgb);
	virtual ~COpenGLWnd();

	// Generated message map functions
// NOTE: these have been declared private because they shouldn't be
//		 overridden, use the provided virtual functions instead.
private:
	//{{AFX_MSG(COpenGLWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// member variables
private:
	CDC* m_pCDC;	// WinGDI Device Context
	HGLRC m_hRC;	// OpenGL Rendering Context
	HCURSOR m_hMouseCursor;	// mouse cursor handle for the view
	CPalette m_CurrentPalette; // palettes
	CPalette* m_pOldPalette;
	CRect m_ClientRect;    // client area size
	double m_dAspectRatio;    // aspect
	int m_DispListVector[20];	// Internal stock display list vector
	BOOL m_bInsideDispList;	// Disp List definition semaphore
	BOOL m_bExternGLCall;
	BOOL m_bExternDispListCall;
	GLuint m_FontListBase;
	BOOL m_bGotFont;
   BOOL m_bGotFont3D;
   GLYPHMETRICSFLOAT m_gmf[256];

// initialization helper functions
	unsigned char ComponentFromIndex(int i, UINT nbits, UINT shift);
	void CreateRGBPalette();
	BOOL bSetupPixelFormat();
protected:
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPENGLWND_H__455CD4BE_C24C_4CF0_8289_73173B606EE5__INCLUDED_)
