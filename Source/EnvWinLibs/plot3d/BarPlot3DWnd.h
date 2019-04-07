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

#include "Plot3DWnd.h"
#include <afxtempl.h>
#include <math.h>
#include <gl/glReal.h>

#include "FDataobj.h"

#include "plot3d/ColorScaleWnd.h"
#include "EnvWinLib_resource.h"



// BarPlot3DWnd

class WINLIBSAPI BarPlot3DWnd : public Plot3DWnd
{

public:
	BarPlot3DWnd();
	virtual ~BarPlot3DWnd();

protected:
   FDataObj *m_pDataObj;
   bool      m_delOnDel;

   ColorScaleWnd m_colorScaleWnd;
   bool m_showLegend;
   bool m_showLabels;
   bool m_showOutline;

   // Plot Properties
   COLORREF m_axesColor;
   COLORREF m_backgroundColor;
   bool m_whiteBackground;

   void DrawData( void );
   void DrawAxes( void );

   enum B3D_MENUS
      {
      SHOW_LEGEND      = 9000,
      SHOW_OUTLINE,
      DATA_SET_PROPS
      };
   virtual void BuildPopupMenu( CMenu* pMenu );
   void OnSelectOption( UINT nID );

public:
   void SetDataObj( FDataObj *pDataObj, bool delOnDel, bool onlyIncreaseScale = false );

protected:
	DECLARE_MESSAGE_MAP()
public:
   virtual void OnCreateGL(); // override to set bg color, activate z-buffer, and other global settings
   virtual void OnSizeGL(int cx, int cy); // override to adapt the viewport to the window

   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};


