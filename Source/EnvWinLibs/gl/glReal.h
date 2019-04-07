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
// Real Euclidian Space Classes


#pragma once


#include <math.h>

#include "gl\gl.h"
#include "gl\glu.h"

////////////////////////////////////////////////////////////////////////////
// glReal

typedef GLdouble glReal;


////////////////////////////////////////////////////////////////////////////
// glReal2

class glReal2 
{
public:
	glReal2(){ x=y=0.0;}
	glReal2( glReal _x, glReal _y ){ x = _x;       y = _y; }
	glReal2( int _x, int _y )      { x = (glReal)_x; y = (glReal)_y; }
   glReal2( float _x, float _y )  { x = (glReal)_x; y = (glReal)_y; }
	~glReal2(){}

	glReal x;
	glReal y;
};

// arithmatic
inline glReal2 operator+( glReal2 v, glReal2 w ){ return glReal2( v.x+w.x, v.y+w.y ); }
inline glReal2 operator-( glReal2 v, glReal2 w ){ return glReal2( v.x-w.x, v.y-w.y ); }
inline glReal2 operator-( glReal2 v ){ return glReal2( -v.x, -v.y ); }
inline bool  operator==( glReal2 v, glReal2 w ){ return ( v.x == w.x && v.y == w.y ); }

// scalar multiplication
inline glReal2 operator*(glReal  s, glReal2 v){ return glReal2(s*v.x, s*v.y); }
inline glReal2 operator*(glReal2 v, glReal  s){ return glReal2(s*v.x, s*v.y); }
inline glReal2 operator/(glReal2 v, glReal  s){ return glReal2(v.x/s, v.y/s); }

// dot product
inline glReal dot( glReal2 v, glReal2 w ){ return v.x*w.x + v.y*w.y; }

// norms
inline glReal  normL1( glReal2 v ){ return fabs(v.x) + fabs(v.y); }
inline glReal  normL2( glReal2 v ){ return sqrt( v.x*v.x + v.y*v.y ); }
inline glReal  normInfinity( glReal2 v ){ return max( fabs(v.x), fabs(v.y) ); }
inline glReal  abs( glReal2 v ){ return normL2(v); }

// others
inline glReal  angle( glReal2 v, glReal2 w ){ return acos( dot(v,w) / ( abs(v)*abs(w) ) ); }
inline glReal2 normalize( glReal2 v ){ if ( abs(v) < 0.0000001 ) return v; else return v/abs(v); }


/////////////////////////////////////////////////////////////////////////////////
// glReal3


class glReal3
{

public:
	glReal3(){}
	~glReal3(){}

	glReal3( glReal _x, glReal _y, glReal _z)      { x = _x;       y = _y;       z = _z; }
	glReal3( int _x, int _y, int _z)         { x = (glReal)_x; y = (glReal)_y; z = (glReal)_z; }
   glReal3( float _x, float _y, float _z)   { x = (glReal)_x; y = (glReal)_y; z = (glReal)_z; }
   
	glReal x;
	glReal y;
	glReal z;

   glReal  operator[]( int i )
      { 
      switch ( i % 3 )
         {
         case 0:
            return x;
         case 1:
            return y;
         case 2:
            return z;
         default:
            ASSERT(0);
         }
      return 0;
      }
};

// operators
inline bool    operator==( glReal3 v, glReal3 w ){ return ( v.x == w.x && v.y == w.y && v.z == w.z ); }


// arithmatic
inline glReal3 operator+( glReal3 v, glReal3 w ){ return glReal3( v.x+w.x, v.y+w.y, v.z+w.z ); }
inline glReal3 operator-( glReal3 v, glReal3 w ){ return glReal3( v.x-w.x, v.y-w.y, v.z-w.z ); }
inline glReal3 operator-( glReal3 v ){ return glReal3( -v.x, -v.y, -v.z ); }


// scalar multiplication
inline glReal3 operator*(glReal  s, glReal3 v){ return glReal3(s*v.x, s*v.y, s*v.z); }
inline glReal3 operator*(glReal3 v, glReal  s){ return glReal3(s*v.x, s*v.y, s*v.z); }
inline glReal3 operator/(glReal3 v, glReal  s){ return glReal3(v.x/s, v.y/s, v.z/s); }

// dot and cross products
inline glReal  dot( glReal3 v, glReal3 w ){ return v.x*w.x + v.y*w.y + v.z*w.z; }
inline glReal3 cross( glReal3 v, glReal3 w ){ return glReal3(v.y*w.z-v.z*w.y, v.z*w.x-v.x*w.z, v.x*w.y-v.y*w.x); }

// norms
inline glReal  normL1( glReal3 v ){ return fabs(v.x) + fabs(v.y) + fabs(v.z); }
inline glReal  normL2( glReal3 v ){ return sqrt( v.x*v.x + v.y*v.y + v.z*v.z ); }
inline glReal  normInfinity( glReal3 v ){ return max( fabs(v.x), max( fabs(v.y), fabs(v.z) ) ); }
inline glReal  abs( glReal3 v ){ return normL2(v); }

// others
inline glReal  angle( glReal3 v, glReal3 w ){ return acos( dot(v,w) / ( abs(v)*abs(w) ) ); }
inline glReal3 normalize( glReal3 v ){ if ( abs(v) < 0.0000001 ) return v; else return v/abs(v); }

inline void  rotate( glReal3 &v, glReal angle, glReal3 axis )
   {
   glReal3 ret;
   glReal   c = cos( angle );
   glReal   s = sin( angle );

   axis = normalize( axis );
   ret.x = ( axis.x*axis.x*( 1.0-c ) +        c )*v.x   +    ( axis.x*axis.y*(1.0-c) - axis.z*s )*v.y   +   ( axis.x*axis.z*(1-c) + axis.y*s )*v.z;
   ret.y = ( axis.y*axis.x*( 1.0-c ) + axis.z*s )*v.x   +    ( axis.y*axis.y*(1.0-c) +        c )*v.y   +   ( axis.y*axis.z*(1-c) - axis.x*s )*v.z;
   ret.z = ( axis.x*axis.z*( 1.0-c ) - axis.y*s )*v.x   +    ( axis.y*axis.z*(1.0-c) + axis.x*s )*v.y   +   ( axis.z*axis.z*(1-c) +        c )*v.z;
   v = ret;
   }
