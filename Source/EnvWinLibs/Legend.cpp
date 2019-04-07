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
#include "EnvWinLibs.h"
#pragma hdrstop

#include <legend.h>
#include <map.h>
#include <maplayer.h>

#include <colors.hpp>
#include <extents.hpp>

#include <math.h>

const float V_BORDER = 0.05f;
const float H_BORDER = 0.02f;
const float LEGEND_HEIGHT = 0.8f;
//const float LEGEND_WIDTH  = 0.2f;
const float LABEL_HBORDER = 0.05f;
const float LABEL_HEIGHT  = 0.04f;  // vertical label height, as portion of vertical map extent (virtual)
const int   TIC_HEIGHT    = 10;     // logical units



Frame::Frame( MapLayer *pMapLayer  )
	: m_left  ( 0 ),   // virtual coordinants (based on an Map object)
	  m_bottom( 0 ),
	  m_right ( 0 ),
     m_top   ( 0 ),
     m_pMapLayer ( pMapLayer ),
     m_labelHeight ( 1.0f ),
     m_textColor   ( BLACK )
   {
   //-- initialize logFont --//
   m_logFont.lfHeight         = 20;                  // character height
   m_logFont.lfWidth          = 0;                   // average width
   m_logFont.lfEscapement     = 0;                   // text angle
   m_logFont.lfOrientation    = 0;                   // individual character angle
   m_logFont.lfWeight         = FW_NORMAL;           // average pixels/1000
   m_logFont.lfItalic         = FALSE;               // flag != 0 if italic
	m_logFont.lfUnderline      = FALSE;               // flag != 0 if underline
   m_logFont.lfStrikeOut      = FALSE;               // flag != 0 if strikeout
	m_logFont.lfCharSet        = ANSI_CHARSET;        // character set: ANSI, OEM
   m_logFont.lfOutPrecision   = OUT_STROKE_PRECIS;   // mapping percision-unused
   m_logFont.lfClipPrecision  = CLIP_STROKE_PRECIS;  // clip percision-unused
   m_logFont.lfQuality        = PROOF_QUALITY;       // draft or proof quality
   m_logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS; // flags for font style
   lstrcpy((LPSTR) m_logFont.lfFaceName, "Arial");   // typeface name

 	REAL xMin, yMin, xMax, yMax;
   m_pMapLayer->m_pMap->GetMapExtents( xMin, xMax, yMin, yMax ); // virtual coords, full map

   REAL yExtent = yMax - yMin;

   m_labelHeight  = yExtent * LABEL_HEIGHT;//  ( LEGEND_HEIGHT - (2*LABEL_HBORDER) ) * yExtent / binCount;      
   m_labelHeight *= 0.5f; //0.8f;
   }



// assumes CDC has been initmapped

void Frame::DrawBoundingBox( CDC *pDC, int xOffset, int yOffset )
	{
	POINT ll, ur;

   Map *pMap = m_pMapLayer->m_pMap;

   pMap->VPtoLP( m_left, m_bottom, ll );  // convert virtual frame coords to logical
	pMap->VPtoLP( m_right, m_top, ur );

   //-- offset it --//
   ll.x += xOffset;
   ll.y += yOffset;
	ur.x += xOffset;
	ur.y += yOffset;

   CPen pen;
   pen.CreatePen( PS_SOLID, 0, BLACK );
   CPen *pOldPen = pDC->SelectObject( &pen );

   HBRUSH hBrush = (HBRUSH) GetStockObject( NULL_BRUSH );
   SelectObject( pDC->m_hDC, hBrush );

   pDC->Rectangle( ll.x, ur.y, ur.x, ll.y );

   pDC->SelectObject( pOldPen );
	}


/*
Selection *Frame::IsOverSelection( COORD2d &coord, float, int *snode )
	{
	// are we within bounds?
   if ( m_left <= coord.x && coord.x <= m_right 
     && m_top <= coord.y && coord.y <= m_bottom )
		{
      *snode = -1;
      return this;
      }

	else
      {
		*snode = 0;
		return NULL;
		}
	}


void Frame::DrawSelectNodes( HDC hDC, Extents *pExtents )
	{
	float _xLeft = m_left;
	float _yBottom = m_bottom;
	float _xRight = m_right;
	float _yTop = m_top;

	pExtents->VPtoLP( _xLeft, _yBottom );
	pExtents->VPtoLP( _xRight, _yTop );

	DrawSelectNode( hDC, _xLeft, _yBottom );
	DrawSelectNode( hDC, _xLeft, _yTop );
	DrawSelectNode( hDC, _xRight, _yTop );
	DrawSelectNode( hDC, _xRight, _yBottom );
	}


void Frame::DragObject( HDC hDC, Extents *pExtents, int xOffset, int yOffset )
	{
	int  rop2   = SetROP2( hDC, R2_NOT );
	DrawBoundingBox( hDC, pExtents, xOffset, yOffset );
	SetROP2( hDC, rop2 );
	}


void Frame::MoveObject( float xOffset, float yOffset )
	{
	m_left += xOffset;
	m_right += xOffset;
	m_bottom += yOffset;
	m_top += yOffset;
	}


BOOL Frame::GetRect( COORD2d &ul, COORD2d &lr )
   {
   ul.x = m_left;
   ul.y = m_top;
   lr.x = m_right;
   lr.y = m_bottom;

   return TRUE;
   }

*/

//----------------------------- Legend ----------------------------//

Legend::Legend( MapLayer *pMapLayer )
	: Frame( pMapLayer )
   { }



void Legend::SetDefaults( void )
   {
   //-- set upper left corner of legend --//
	REAL xMin, yMin, xMax, yMax;
   m_pMapLayer->m_pMap->GetMapExtents( xMin, xMax, yMin, yMax ); // virtual coords, full map

   REAL xExtent = xMax - xMin;
   REAL yExtent = yMax - yMin;

	m_left   = xMax + V_BORDER * xExtent; // position left frame just to the right of map
   m_top    = yMax - H_BORDER * yExtent;
   m_bottom = 0; // Max - (H_BORDER+LEGEND_HEIGHT)*yExtent;   set at run time
   m_right  = 0;

   //int binCount = m_pMapLayer->GetBinCount();
   //if ( binCount < 16 )
   //   binCount = 16;

   // note:  right hand side not set until SetExtents() is called
   return;
	}


//-- Legend::SetExtents() -------------------------------------------
//
//-- Sets the internal extents of the legend, in virtual coords.
//
//   Note: extents is set based on existing label height (to set font)
//   and the maximum width of the label string based on that font
//
// returns the right hand virtual coordinate of the bounding box
//-------------------------------------------------------------------

float Legend::SetExtents( CDC *pDC )
   {
   BinArray *pBinArray  = m_pMapLayer->GetBinArray( USE_ACTIVE_COL, false );
   if ( pBinArray == NULL )
      return 0;

   int       binCount   = (int) pBinArray->GetSize();
   REAL      lineHeight = m_labelHeight / 0.8f;

	//-- install the correct font into the HDC --//
   m_logFont.lfHeight = m_pMapLayer->m_pMap->VDtoLD( m_labelHeight );
   CFont font;
   font.CreateFontIndirect( &m_logFont );

   CFont *pOldFont = pDC->SelectObject( &font );

   CSize size = pDC->GetOutputTextExtent( m_pMapLayer->GetFieldLabel(), lstrlen( m_pMapLayer->GetFieldLabel() ) );
	int maxWidth = size.cx;

	for ( int i=0; i < binCount; i++ )
		{
      Bin &bin = pBinArray->ElementAt( i );
		CString& label = bin.m_label;

		size = pDC->GetOutputTextExtent( label, lstrlen( label ) );
		int width = size.cx;

		if ( width > maxWidth )
			maxWidth = width;
		}

   //-- convert maxWidth to virtual coords --//
	REAL vMaxWidth = m_pMapLayer->m_pMap->LDtoVD( maxWidth );

   //-- add in the color rectangle to the label width --//
   vMaxWidth += lineHeight;

	m_right = m_left + ( vMaxWidth ) + lineHeight*2;

   pDC->SelectObject( pOldFont );
   return (float) m_right;
	}


//-- Legend::Draw() -------------------------------------------------
//
//-- draws a color scale legend in the specified HDC.  Assume the
//   hDC is mappend into screen unit (no special mapping, assumes
//   y increases up the screen - i.e. HDC's been InitMapped)
//
//   Draws starting in the specified upper left coord
//-------------------------------------------------------------------

void Legend::Draw( CDC *pDC )
   {
#ifdef _WINDOWS

   Map *pMap = m_pMapLayer->m_pMap;

   REAL xMin, yMin, xMax, yMax;
   pMap->GetMapExtents( xMin, xMax, yMin, yMax ); // virtual coords
   REAL yExtent = yMax - yMin;

   BinArray *pBinArray = m_pMapLayer->GetBinArray( USE_ACTIVE_COL, false );
   if ( pBinArray == NULL )
      return;

   int binCount     = (int) pBinArray->GetSize();
   int _labelHeight = pMap->VDtoLD( m_labelHeight );
   int _lineHeight  = (int) (_labelHeight / 0.8);

   m_logFont.lfHeight = _labelHeight;

   CPen pen;
   pen.CreatePen( PS_SOLID, 0, BLACK );
   CPen *pOldPen = pDC->SelectObject( &pen );

   CFont font;
   font.CreateFontIndirect( &m_logFont );
   
   CFont italic;
   m_logFont.lfItalic = TRUE;
   italic.CreateFontIndirect( &m_logFont );
   m_logFont.lfItalic = FALSE;

   pDC->SetTextColor( BLACK ); //m_textColor );
   pDC->SetBkMode( TRANSPARENT );

   CBrush brush;
   brush.CreateSolidBrush( 0x0L );

   REAL x = m_left + m_labelHeight;
   REAL y = m_top  - (yExtent*LABEL_HBORDER);   // start at top

   POINT lp;
   pMap->VPtoLP( x, y, lp );  // convert from virtual to logical

   CFont *pOldFont = pDC->SelectObject( &italic );
   pDC->TextOut( lp.x, lp.y+_lineHeight*9/10, m_pMapLayer->GetFieldLabel() );

   pDC->SelectObject( &font );

   for ( int i=0; i < binCount; i++ )
      {
		Bin &bin      = pBinArray->ElementAt( i );
      //CBrush *pBrush = &bin.m_brush;
      //pBrush = pDC->SelectObject( pBrush );

      brush.DeleteObject();
      brush.CreateSolidBrush( bin.m_color );
      pDC->SelectObject( &brush );

      pDC->Rectangle( lp.x, lp.y, lp.x+_labelHeight, lp.y-_labelHeight );
      pDC->TextOut( lp.x+_lineHeight, lp.y, bin.m_label );

      //pDC->SelectObject( pBrush );

      lp.y -= _lineHeight;      // y increases going down
		}

   float min, max;
   m_pMapLayer->GetDataMinMax( m_pMapLayer->GetActiveField(), &min, &max );
   CString range;
   range.Format( "Range: %g-%g", min, max );
   pDC->TextOut( lp.x, lp.y, range );   

   pDC->SelectObject( pOldFont );

   SetExtents( pDC );
	pDC->SelectObject( pOldPen );


   // reset bounding box based on lines drawn
   lp.y -= 2 * _lineHeight;

   REAL vxBottom, vyBottom;
   pMap->LPtoVP( lp, vxBottom, vyBottom );

   m_bottom = vyBottom;

	DrawBoundingBox( pDC, 0, 0 );
#endif // _WINDOWS
   }



//----------------------- BinHistogram -----------------------------//

void BinHistogram::SetDefaults( REAL vRight, REAL vTop )
   {

   //-- set upper left corner of legend --//
	REAL xMin, yMin, xMax, yMax;
   m_pMapLayer->m_pMap->GetMapExtents( xMin, xMax, yMin, yMax ); // virtual coords

   REAL xExtent = xMax - xMin;
   REAL yExtent = yMax - yMin;

	m_left   = xMax + V_BORDER * xExtent; // position left frame just to the right of map
   m_top    = vTop; //yMax - (H_BORDER+LEGEND_HEIGHT)*yExtent;

   m_bottom = vTop - yExtent / 15; //yMin + H_BORDER*yExtent;
   m_right  = vRight;

   return;
	}



//-- BinHistogram::Draw() -------------------------------------------------
//
//-- draws a color scale BinHistogram in the specified HDC.  Assume the
//   hDC is mappend into screen unit (no special mapping, assumes
//   y increases up the screen - i.e. HDC's been InitMapped)
//-------------------------------------------------------------------

void BinHistogram::Draw( CDC *pDC )
   {   
   Map *pMap = m_pMapLayer->m_pMap;

   // first, draw axes
   REAL xMin = m_left  + (m_right-m_left)/10;
   REAL xMax = m_right - (m_right-m_left)/10;
   REAL yMin = m_bottom+2;
   REAL yMax = m_top - (m_top-m_bottom)/10;

   POINT ul, ll, lr;
   pMap->VPtoLP( xMin, yMax, ul );
   pDC->MoveTo( ul.x, ul.y );

   pMap->VPtoLP( xMin, yMin, ll );
   ll.y += 2;
   pDC->LineTo( ll.x, ll.y );

   pMap->VPtoLP( xMax, yMin, lr );
   lr.y += 2;
   pDC->LineTo( lr.x, lr.y );

   // draw bin counts
   BinArray *pBinArray = m_pMapLayer->GetBinArray( USE_ACTIVE_COL, false );

   if ( pBinArray == NULL )
      return;

   int binCount = (int) pBinArray->GetSize();

	//-- axes draw, now draw bars --//
   REAL vBinExt = m_right - m_left - 2*(m_right-m_left)/10;
   int barWidth = pMap->VDtoLD( vBinExt );
   barWidth /= binCount;

   int height = ul.y - ll.y;

	//-- get maximum bin popSize --//
	int maxPopSize = 0;
	for ( int i=0; i < binCount; i++ )
		if ( pBinArray->ElementAt( i ).m_popSize > maxPopSize )
			maxPopSize = pBinArray->ElementAt( i ).m_popSize;

   CBrush brush;
   brush.CreateSolidBrush( 0x0L );

   CPen pen( PS_SOLID, 0, (COLORREF) 0x0L );
     
	//-- now draw plots --//
	for ( int i=0; i < binCount; i++ )
		{
		int barHeight = (int) (height * pBinArray->ElementAt( i ).m_popSize / (float) maxPopSize);

		int x = ll.x + ( i*barWidth );

		Bin &bin = pBinArray->ElementAt( i );
      //if ( i == 0 )
      //   pBrush = pDC->SelectObject( &bin.m_brush );
      //else
         pen.DeleteObject();
         pen.CreatePen( PS_SOLID, 0, bin.m_color );

         brush.DeleteObject();
         brush.CreateSolidBrush( bin.m_color );
         pDC->SelectObject( &brush );  // bin.m_brush );
         pDC->SelectObject( &pen );

		pDC->Rectangle( x, ll.y, x+barWidth+1, ll.y+barHeight );
		}

   //SelectObject( pDC->m_hDC, hPen );

   //pDC->SelectObject( pBrush );
	DrawBoundingBox( pDC, 0, 0 );
   }



//----------------------- ScaleIndicator -----------------------------//

void ScaleIndicator::SetDefaults(REAL vRight, REAL vTop )
   {

   //-- set upper left corner of indicator --//
	REAL xMin, yMin, xMax, yMax;
   m_pMapLayer->m_pMap->GetMapExtents( xMin, xMax, yMin, yMax ); // virtual coords

   REAL xExtent = xMax - xMin;
   REAL yExtent = yMax - yMin;

	m_left   = xMax + V_BORDER * xExtent; // position left frame just to the right of map
   m_top    = vTop;

   m_bottom = vTop - yExtent / 15;
   m_right  = vRight;

   return;
	}



//-- ScaleIndicator::Draw() -------------------------------------------------
//
//-- draws a color scale ScaleIndicator in the specified HDC.  Assume the
//   hDC is mappend into screen unit (no special mapping, assumes
//   y increases up the screen - i.e. HDC's been InitMapped)
//-------------------------------------------------------------------


void ScaleIndicator::Draw( CDC *pDC )
   {
   // get scale
   REAL vWidth = m_right - m_left - 2*H_BORDER; // virtual coords
   float scale = (float) log10( vWidth );
   int iscale = int( scale );

   // now, draw x-axis 

   Map *pMap = m_pMapLayer->m_pMap;

   // first, draw axes
   REAL xMin = m_left  + (m_right-m_left)/10;
   REAL xMax = m_right - (m_right-m_left)/10;
   REAL y    = m_bottom + (m_top-m_bottom) * 0.33f;

   POINT left, right;

   pMap->VPtoLP( xMin, y, left );
   pDC->MoveTo( left.x, left.y );

   pMap->VPtoLP( xMax, y, right );
   pDC->LineTo( right.x, right.y );

   m_logFont.lfHeight = m_pMapLayer->m_pMap->VDtoLD( m_labelHeight );
   CFont font;
   font.CreateFontIndirect( &m_logFont );

   CFont *pOldFont = pDC->SelectObject( &font );

   // first tick
   pDC->MoveTo( left.x, left.y+5 );
   pDC->LineTo( left.x, left.y-0 );

   pDC->SetTextAlign( TA_CENTER | TA_BOTTOM );
   pDC->TextOut( left.x, left.y+5, "0" );

   // intermediate tick
   float a = (float) pow( 10.0f, scale );
   float b = (float) pow( 10.0f, (float) iscale );
   int r = int( a / b );
   float x = r * b;   // x is the number to use for the RHS of the scale indicator (virtual)

   int ix  = pMap->VDtoLD( x );   // distances

   pDC->MoveTo( left.x + ix/2, left.y+5 );
   pDC->LineTo( left.x + ix/2, left.y-0 );

   CString text;
   text.Format( "%g", x/2.0f );
   pDC->TextOut( left.x + ix/2, left.y+5, text );

   pDC->MoveTo( left.x + ix, left.y+5 );
   pDC->LineTo( left.x + ix, left.y-0 );

   text.Format( "%g", x );
   pDC->TextOut( left.x + ix, left.y+5, text );

   pDC->SelectObject( &font );
	DrawBoundingBox( pDC, 0, 0 );
   }


