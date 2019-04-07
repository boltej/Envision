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
#include "EnvWinLibs.h"

#include "plot3d/Plot3DWnd.h"

class PointCollection;

// ShapePreviewWnd

class ShapePreviewWnd : public Plot3DWnd
{

public:
	ShapePreviewWnd();
	virtual ~ShapePreviewWnd();

protected:
   PointCollection *m_pPointCollection;

   void DrawPoint( void );

public:
   void SetBackgroundColor( COLORREF rgb ){ m_backgroundColor = rgb; }
   void SetPointCollection( PointCollection *pPointCollection ){ m_pPointCollection = pPointCollection; }

protected:
	DECLARE_MESSAGE_MAP()

public:
   virtual void OnCreateGL(); 
   virtual void DrawData();
};


