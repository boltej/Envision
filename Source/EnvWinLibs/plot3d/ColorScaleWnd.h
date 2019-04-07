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

class ColorScale
   {
   public:
      ColorScale() : m_baseColor( RGB(255,255,255) ), m_min(0.0f), m_max(1.0f) {}
      ~ColorScale(){}
   
   protected:
      COLORREF m_baseColor;
      float    m_min;
      float    m_max;

   public:
      void     SetBaseColor( COLORREF rgb ){ m_baseColor = rgb; }
      void     SetMinMax( float _min, float _max ){ m_min = _min; m_max = _max; }
      float    GetMin( void ){ return m_min; }
      float    GetMax( void ){ return m_max; }
      COLORREF GetColor( float h )
         {
         ASSERT( m_min < m_max );
         if ( h < m_min )
            h = m_min;
         if ( h > m_max )
            h = m_max;

         h = h/(m_max - m_min);

         int r = int( ( 0.8f*h + 0.2 )*(float)GetRValue( m_baseColor ) );
         int g = int( ( 0.8f*h + 0.2 )*(float)GetGValue( m_baseColor ) );
         int b = int( ( 0.8f*h + 0.2 )*(float)GetBValue( m_baseColor ) );

         return RGB(r,g,b);
         }
   };

// ColorScaleWnd

class ColorScaleWnd : public CWnd
{
	DECLARE_DYNAMIC(ColorScaleWnd)

public:
	ColorScaleWnd();
	virtual ~ColorScaleWnd();

protected:
   int m_height;
   int m_width;

   ColorScale m_colorScale;

   CFont m_font;

public:
   void     SetBaseColor( COLORREF rgb ){ m_colorScale.SetBaseColor( rgb ); }
   COLORREF GetColor( float h ){ return m_colorScale.GetColor( h ); }
   void     SetMinMax( float _min, float _max ){ m_colorScale.SetMinMax( _min, _max ); }

protected:
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnPaint();
protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
};


