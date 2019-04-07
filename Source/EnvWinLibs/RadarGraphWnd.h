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
#if !defined(AFX_RADARGRAPHWND_H__443F401A_870E_44FE_85A0_AB9113540C0E__INCLUDED_)
#define AFX_RADARGRAPHWND_H__443F401A_870E_44FE_85A0_AB9113540C0E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <graphwnd.h>
#include <fdataobj.h>

#include <afxtempl.h>


// RadarGraphWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// RadarGraphWnd window

class WINLIBSAPI RadarGraphWnd : public GraphWnd
{
DECLARE_DYNAMIC( RadarGraphWnd )

// Construction
public:
   RadarGraphWnd();
	RadarGraphWnd( FDataObj*, float minValueArray[], float maxValueArray[], bool makeDataLocal );
	virtual ~RadarGraphWnd();

// Attributes
protected:
   bool   m_showInitialState;
   int    m_lastDynamicDisplayIndex;
   bool   m_isDataLocal;

public:
   FDataObj *m_pDataObj;
   int       m_row;      // draw polygon for this row in dataObj

protected:
   COLORREF m_initialStateColor;
   COLORREF m_currentStateColor;
   
// Operations
public:
   virtual bool UpdatePlot( CDC *pDC );
   
   void DrawPlot( CDC &dc );

   void AppendCol( FDataObj *pDataObj, UINT colNum, float minimum, float maximum );
   void SetRow( int row ){ m_row = row; }
   int  GetRow(){ return m_row; }

   // Generated message map functions
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RADARGRAPHWND_H__443F401A_870E_44FE_85A0_AB9113540C0E__INCLUDED_)
