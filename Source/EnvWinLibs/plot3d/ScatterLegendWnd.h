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

#include <afxtempl.h>
#include "plot3d/ShapePreviewWnd.h"

class PointCollectionArray;
class ScatterPlot3DWnd;

// ScatterLegendWnd

class ScatterLegendWnd : public CWnd
{
	DECLARE_DYNAMIC(ScatterLegendWnd)

public:
	ScatterLegendWnd();
	virtual ~ScatterLegendWnd();

protected:
   PointCollectionArray *m_pDataArray;
   CArray< ShapePreviewWnd*, ShapePreviewWnd* > m_previewWndArray;

   int m_selection;
   int m_length;

   CFont m_font;
   static const int m_previewLength = 30;
   static const int m_gutter = 5;

   ScatterPlot3DWnd *m_pParent;

   void ClearPreviewWndArray( void );

public:
   void  SetPointCollectionArray( PointCollectionArray *pCollection );
   void  SetSelection( int sel ){ m_selection = sel; }
   CSize GetIdealSize();
protected:
	DECLARE_MESSAGE_MAP()
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
   afx_msg void OnPaint();
   afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};


