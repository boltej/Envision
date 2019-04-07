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
#if !defined _LEGEND_H
#define _LEGEND_H

#include <Typedefs.h>

class MapLayer;



class Frame
   {
   public:
      // data
      REAL m_left;     // virtual coods 
      REAL m_bottom;
      REAL m_right;
      REAL m_top;

      COLORREF m_textColor;
      LOGFONT  m_logFont;
      REAL     m_labelHeight;  // virtual coords

      MapLayer  *m_pMapLayer;  // map to draw on...

      // constructor
      Frame( MapLayer* );

      void DrawBoundingBox( CDC *pDC, int xOffset=0, int yOffset=0 );

      //Selection *IsOverSelection( COORD2d &coord, float, int *snode );
      //void DrawSelectNodes( HDC hDC, Extents *pExtents );
      //void DragObject( HDC hDC, Extents *pExtents, int xOffset, int yOffset );
      //void MoveObject( float xOffset, float yOffset );
      //BOOL GetRect( COORD2d &ul, COORD2d &lr );
   };


class Legend : public Frame
   {
   public:
      // constructor
      Legend( MapLayer *_pMapLayer );

      void  SetDefaults( void );
      float SetExtents( CDC* );
      void  Draw( CDC* );
   };


class BinHistogram : public Frame
   {
   public:
      BinHistogram( MapLayer *pMapLayer ) : Frame( pMapLayer ) { }

      void SetDefaults( REAL vRight, REAL vTop );
      void Draw( CDC* );
   };


class ScaleIndicator : public Frame
   {
   public:
      ScaleIndicator( MapLayer *pMapLayer ) : Frame( pMapLayer ) { }

      void SetDefaults( REAL vRight, REAL vTop );
      void Draw( CDC* );
   };
  


#endif   // defined _LEGEND_H