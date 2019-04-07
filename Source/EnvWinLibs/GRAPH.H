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
#if !defined _GRAPH_H
#define _GRAPH_H

#if !defined ( _COLORS_HPP )
#include <colors.hpp>
#endif

#if !defined _MARKER_HPP
#include <marker.hpp>
#endif


class DataObj;


//------------------------- constants -----------------------------//

//-- window Logical coordinants ( mins are 0 ) --//
const int XLOGMAX  = 1280;
const int YLOGMAX  = 1280;

//-- data window offsets (logical coords?) --/
const int XDATAOFFSET_LEFT  = 150;
const int XDATAOFFSET_RIGHT = 300;
const int YDATAOFFSET       = 200;

const int LEGEND_HEIGHT  = 36;
const int LABEL_HEIGHT   = 48;


//-- maximum constraints --//
const int MAXTEXTLENGTH  = 60;

//-- orientation flags --//
const int OR_HORZ  = 0;
const int OR_VERT  = 2700;

//-- label text flags --//
enum MEMBER
   { TITLE, XAXIS, YAXIS };

//-- Bar plot equivalents --//
#define MS_BAR2DSOLID   MS_SOLIDSQUARE
#define MS_BAR2DHOLLOW  MS_HOLLOWSQUARE

//#define MS_BAR3DSOLID   MS_SOLIDCIRCLE  // not implemented
//#define MS_BAR3DHOLLOW  MS_HOLLOWCIRCLE


const LONG PLOTWNDCOLORREF = LTGRAY;
const int  TICSIZE  = 10;

//-- text information ( title, labels, legends ) --//
struct LABEL
   {
   char     *text;            // text
   COLORREF  color;           // color to draw with
   HFONT     hFont;           // handle to font for this label
   RECT      rect;            // rects containing text
   int       orientation;     // OR_HORZ, OR_VERT

   LABEL( void ) : text( NULL ), color( BLACK ), hFont( 0 ),
                   rect(), orientation( OR_HORZ ) { }

   ~LABEL( void ) { if ( text ) delete [] text; }
   };


struct LINE
   {
   char     *text;            // text
   DataObj  *pDataObj;        // associated data object
   UINT      dataCol;         // corresponding column in the data object
   RECT      rect;            // rects containing text
   int       orientation;     // OR_HORZ, OR_VERT
   COLORREF  color;           // color to draw with
   BOOL      showLine
   BOOL      showMarker;
   FLAG      lineStyle;       // line style (see windows.h)
   MSTYLE    markerStyle;
   int       lineWidth;       // line width
   int       markerIncr;      // marker display increment
   float     min;             // scale factors
   float     max;
   bool      m_smooth;
   float     m_tension;

   LINE( void ) : text( NULL ), pDataObj( NULL ), dataCol( 0 ), rect(),
                  orientation( OR_HORZ ),
                  color      ( 0 ),
                  showLine   ( TRUE ),
                  showMarker ( TRUE ),
                  m_smooth   ( true )
                  lineStyle  ( PS_DASH ),
                  markerStyle( MS_CROSS ),
                  lineWidth  ( 1 ),
                  markerIncr ( 5 ),
                  m_smooth   ( true ),
                  m_tension  ( 0.5f )
                 { min = max = float(0.0); }

   ~LINE( void ) { if ( text ) delete [] text; }
   };


//-- x, y Axis information --//
struct AXIS
   {
   int      digits;           // number of digits to the right of the decimal
   float    major;            // major tic increment
   float    minor;            // minor tic increment
   COLORREF color;            // color to draw with
   BOOL     showLabel;
   BOOL     showTics;
   BOOL     showScale;
   BOOL     autoScale;
   BOOL     autoIncr;

   AXIS( void ) : digits( 0 ), major( float(0) ), minor( float(0) ), color( BLACK ),
                  showLabel( TRUE ), showTics( TRUE ), showScale( TRUE ),
                  autoScale( TRUE ), autoIncr( TRUE ) { }
   };

struct COLDATA
   {
   DataObj *pDataObj;     // ptr to the data object
   UINT     colNum;       // column of data in the dataobj
   };

#endif
