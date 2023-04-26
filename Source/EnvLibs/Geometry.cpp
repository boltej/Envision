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
//-------------------------------------------------------------------
//  GEOMETRY.CPP
//
//-- miscellaneous geometric routines
//-------------------------------------------------------------------

#include "EnvLibs.h"
#pragma hdrstop

#include "GEOMETRY.HPP"
#include <math.h>

//#include <boost/geometry/geometry.hpp>
//#include <boost/geometry/geometries/point_xy.hpp>
//#include <boost/geometry/geometries/register/point.hpp>
//#include <boost/geometry/geometries/segment.hpp>


//struct BPoint
//   {
//   REAL x = 0;
//   REAL y = 0;
//
//   BPoint(REAL _x, REAL _y) : x(_x), y(_y) {}
//   BPoint(void) = default;
//   };
//
//namespace bg = boost::geometry;
//BOOST_GEOMETRY_REGISTER_POINT_2D(BPoint, REAL, bg::cs::cartesian, x, y)



// helper functions
bool _CheckSide( COORD2d &p0, COORD2d &p1, COORD2d &s0, COORD2d &s1,  bool isP0InRect, bool isP1InRect, COORD2d &start, COORD2d &end, bool &startPopulated, bool &endPopulated );


// Geometry.h implementations

bool IsPolyLineInRect( COORD2d vertexArray[], int numverts, REAL xMin, REAL yMin, REAL xMax, REAL yMax )
   {
   if ( numverts == 0 )
      return false;

   COORD2d intersectPt;
   
   COORD2d v0 = vertexArray[ 0 ];
   for ( int i=1; i < numverts; i++ )
      {
      COORD2d v1 = vertexArray[ i ];

      // check left side of bounding box
      if ( GetIntersectionPt( v0, v1, COORD2d( xMin, yMin), COORD2d( xMin, yMax ), intersectPt ) )
         return true;

      // check top of bounding box
      if ( GetIntersectionPt( v0, v1, COORD2d( xMin, yMax), COORD2d( xMax, yMax ), intersectPt ) )
         return true;

      // check right of bounding box
      if ( GetIntersectionPt( v0, v1, COORD2d( xMax, yMin), COORD2d( xMax, yMax ), intersectPt ) )
         return true;

      // check bottom of bounding box
      if ( GetIntersectionPt( v0, v1, COORD2d( xMin, yMin), COORD2d( xMax, yMin ), intersectPt ) )
         return true;

      //if ( ::DoRectsIntersect( xMin, yMin, xMax, yMax, v0.x, v0.y, v1.x, v1.y ) )
      //   return true;      

      v0 = v1;
      }

   return false;  
   }



bool IsPolyLineInRect( COORD3d vertexArray[], int numverts, float xMin,  float yMin,  float xMax,  float yMax )
   {
   COORD3d intersectPt;
   
   COORD3d v0 = vertexArray[ 0 ];
   for ( int i=1; i < numverts; i++ )
      {
      COORD3d v1 = vertexArray[ i ];

      // check left side of bounding box
      if ( GetIntersectionPt( v0, v1, COORD2d( xMin, yMin), COORD2d( xMin, yMax ), intersectPt ) )
         return true;

      // check top of bounding box
      if ( GetIntersectionPt( v0, v1, COORD2d( xMin, yMax), COORD2d( xMax, yMax ), intersectPt ) )
         return true;

      // check right of bounding box
      if ( GetIntersectionPt( v0, v1, COORD2d( xMax, yMin), COORD2d( xMax, yMax ), intersectPt ) )
         return true;

      // check bottom of bounding box
      if ( GetIntersectionPt( v0, v1, COORD2d( xMin, yMin), COORD2d( xMax, yMin ), intersectPt ) )
         return true;

      //if ( ::DoRectsIntersect( xMin, yMin, xMax, yMax, v0.x, v0.y, v1.x, v1.y ) )
      //   return true;      

      v0 = v1;
      }
   return false;
   }


int IsPointInPoly( COORD2d vertexArray[], int numverts, COORD2d &point )
   {
   int   xflag0, yflag1;
   REAL  dv0;

   //-- intermediate verticies --//
   COORD2d *v0 = &vertexArray[ numverts-1 ];
   COORD2d *v1 = v0;

   //-- get test bit for above/below Y axis --//
   int yflag0 = ( dv0 = v0->y - point.y ) >= 0.0;

   int crossings = 0;
   for ( int j=0; j < numverts; j++ )
      {
      // cleverness:  bobble between filling endpoints of edges, so
      // that the previous edge's shared endpoint is maintained.
      if ( j & 0x1 )
         {
         v0 = &vertexArray[ j ];
         yflag0 = ( dv0 = v0->y - point.y ) >= 0.0;
         }
      else
         {
         v1 = &vertexArray[ j ];
         yflag1 = ( v1->y >= point.y );
         }

      //-- check if points not both above/below X axis - can't hit ray  --//
      if ( yflag0 != yflag1 ) 
         {
         // check if points on same side of Y axis 
         if ( ( xflag0 = ( v0->x >= point.x ) ) == ( v1->x >= point.x ) )
            {
            if ( xflag0 ) crossings++;
            }
         else 
            {
            // compute intersection of poly segment with X ray, note
            // if > point's X.
            crossings += 
               ( v0->x - dv0*( v1->x-v0->x)/(v1->y-v0->y) ) >= point.x;
            }
         }
      }  // end of: for ( j < numverts )

   return ( crossings & 0x0001 );   // is # of crossings odd? then pt inside
   }



// integer version
int IsPointInPoly( LPPOINT vertexArray, int numverts, POINT &point )
   {
   int xflag0, yflag1;
   int dv0;

   //-- intermediate verticies --//
   POINT *v0 = vertexArray + numverts-1;
   POINT *v1 = v0;

   //-- get test bit for above/below Y axis --//
   int yflag0 = ( dv0 = v0->y - point.y ) >= 0;

   int crossings = 0;
   for ( int j=0; j < numverts; j++ )
      {
      // cleverness:  bobble between filling endpoints of edges, so
      // that the previous edge's shared endpoint is maintained.
      if ( j & 0x1 )
         {
         v0 = vertexArray + j;
         yflag0 = ( dv0 = v0->y - point.y ) >= 0;
         }
      else
         {
         v1 = vertexArray + j;
         yflag1 = ( v1->y >= point.y );
         }

      //-- check if points not both above/below X axis - can't hit ray  --//
      if ( yflag0 != yflag1 ) 
         {
         // check if points on same side of Y axis 
         if ( ( xflag0 = ( v0->x >= point.x ) ) == ( v1->x >= point.x ) )
            {
            if ( xflag0 ) crossings++;
            }
         else 
            {
            // compute intersection of poly segment with X ray, note
            // if > point's X.
            crossings += 
               ( v0->x - ((float)dv0)*( v1->x-v0->x)/(v1->y-v0->y) ) >= point.x;
            }
         }
      }  // end of: for ( j < numverts )

   return ( crossings & 0x0001 );   // is # of crossings odd? then pt inside
   }



// integer version
bool IsPointOnLine( LPPOINT vertexArray, int numverts, POINT &point, int tolerance )
   {
   if ( numverts <= 1 )
      return false;

   float x, x0, x1, y, y0, y1, distance;
   x = (float) point.x;
   y = (float) point.y;

   x1 = (float) vertexArray[ 0 ].x;
   y1 = (float) vertexArray[ 0 ].y;

   for ( int i=0; i < numverts-1; i++ )
      {
      x0 = x1;
      y0 = y1;
      x1 = (float) vertexArray[ i+1 ].x;
      y1 = (float) vertexArray[ i+1 ].y;

      DistancePtToLineSegment( x, y, x0, y0, x1, y1, distance );

      if ( distance <= (float) tolerance )
         return true;
      }

   return false;
   }


bool IsPointOnRect( RECT &rect, POINT &pt, int tolerance )
   {
   CPoint pts[ 5 ];
   pts[ 0 ].SetPoint( rect.left, rect.top );
   pts[ 1 ].SetPoint( rect.left, rect.bottom );
   pts[ 2 ].SetPoint( rect.right, rect.bottom );
   pts[ 3 ].SetPoint( rect.right, rect.top );
   pts[ 4 ].SetPoint( rect.left, rect.top );

   return ::IsPointOnLine( pts, 5, pt, tolerance );
   }

bool IsPointInRect( RECT &rect, POINT &pt, int tolerance )
   {
   rect.left   -= tolerance;
   rect.bottom -= tolerance;
   rect.right  += tolerance;
   rect.top    += tolerance;

   if ( ( (rect.left-tolerance)   < pt.x ) && ( pt.x  < (rect.right+tolerance) )
     && ( (rect.bottom+tolerance) < pt.y ) && ( pt.y < (rect.top+tolerance) ) )
     return true;

   return false;
   }


bool IsPointInRect( COORD2d &pt, REAL rXMin, REAL rYMin, REAL rXMax, REAL rYMax )    // edges or interior
   {
   if ( (rXMin <= pt.x ) && ( pt.x <= rXMax ) && ( rYMin < pt.y ) && ( pt.y < rYMax ) )
     return true;

   return false;
   }


//-- DistancePtToLine() -----------------------------------------
//
//-- doesn't check end segments (point to line, not line segment)
//---------------------------------------------------------------

float DistancePtToLine( REAL x, REAL y, REAL x0, REAL y0, REAL x1, REAL y1 )
   {
   //-- same point? --//
   if ( ( x0 == x1 ) && ( y0 == y1 ) )
      return (float) sqrt( (x-x0) * (x-x0) + (y-y0)*(y-y0) );

   //-- first, calculate the slope of the Vertex --//
   REAL slope;
   if ( x0 == x1 )   // vertical line?
      slope = 1.0e10f;  // fake a large slope
   else
      slope = ( y0 - y1 ) / ( x0 - x1 );

   //-- horizontal line --//
   if ( fabs( slope ) <= 0.0001 )
      slope = 1.0e-10f;

   //-- next, y-intercept of Vertex --//
   REAL b = y1 - ( slope * x1 );

   //-- next, y-intercept of perpendicular Vertex --//
   REAL bPerp = y + ( x / slope );

   //-- next, nearest x, y coord on Vertex --//
   REAL xNear = ( bPerp - b ) / ( ( 1.0f / slope ) + slope );
   REAL yNear = slope * xNear + b;

   //-- now, calculate distance between points --//
   return (float) sqrt( (x-xNear)*(x-xNear) + (y-yNear)*(y-yNear) );
   }



//-- DistancePtToLineSegment() -------------------------------------
//
//-- checks end segments (point to line segment, not line), and returns
//     the closest distance from the point to the line segment.
//
//   returns true if the perpendicular from the point to the line is
//     on the line segment, false otherwise
//
//   Algorithm:  First, check for horizontal and vertical cases
//     they are straightforward.  For any other case
//-------------------------------------------------------------------

bool DistancePtToLineSegment(REAL x, REAL y, REAL x0, REAL y0, REAL x1, REAL y1, float &distance )
   {
   distance = -1.0f;

   //-- same point? --//
   if ( ( x0 == x1 ) && ( y0 == y1 ) )
      {
      distance = (float) sqrt( (x-x0) * (x-x0) + (y-y0)*(y-y0) );
      return true;
      }

   //-- first, calculate the slope of the Vertex --//
   float slope;
   if ( x0 == x1 )   // vertical line?
      slope = 10000;  // fake a large slope
   else
      slope = (float) ( ( y0 - y1 ) / ( x0 - x1 ) );

   REAL xMin = x0 < x1 ? x0 : x1;
   REAL xMax = x0 > x1 ? x0 : x1;
   REAL yMin = y0 < y1 ? y0 : y1;
   REAL yMax = y0 > y1 ? y0 : y1;

   // vertical line
   if ( x0 == x1 )
      {
      // is the passed in point within the line segment?
      if ( yMin <= y && y <= yMax )
         {
         distance = (float) fabs( x-x0 );
         return true;
         }
      else if ( y < yMin  ) // passed point is "below" segment (y increasing up)
         {
         distance = (float) sqrt( (x-x0)*(x-x0) + (y-yMin)*(y-yMin) );
         return false;
         }
      else // (  y > yMax1  ) // passed point is "above" segment
         {
         distance = (float) sqrt( (x-x1)*(x-x1) + (y-yMax)*(y-yMax) );
         return false;
         }
      }


   //-- horizontal line --//
   if ( y0 == y1 )
      {
      // is he passed in point within the line segment?
      if ( xMin <= x && x <= xMax )
         {
         distance = (float) fabs( y-y0 );
         return true;
         }
      else if ( x < xMin  ) // passed point is off to the left
         {
         distance = (float) sqrt( (x-xMin)*(x-xMin) + (y-y0)*(y-y0) );
         return false;
         }
      else // (  x > xMax  ) // passed point is off to the right
         {
         distance = (float) sqrt( (x-xMax)*(x-xMax) + (y-y1)*(y-y1) );
         return false;
         }
      }


   //-- next, y-intercept of Vertex --//
   float b = (float)(y1 - ( slope * x1 ));

   //-- next, y-intercept of perpendicular --//
   float bPerp = (float)( y + ( x / slope ));

   //-- next, nearest x, y coord on on both the line and the perpendicular from the point  off the line
   float xNear = ( bPerp - b ) / ( ( 1.0f / slope ) + slope );
   float yNear = slope * xNear + b;

   // check to see if it is within the box defined by the line segments
   bool onSegment = true;

   if ( xNear < xMin )
      onSegment = false;

   if ( xNear > xMax )
      onSegment = false;

   if ( yNear < yMin )
      onSegment = false;

   if ( yNear > yMax )
      onSegment = false;

   if ( onSegment )
      distance = (float) sqrt( (x-xNear)*(x-xNear) + (y-yNear)*(y-yNear) );
   else
      {
      // find closest endpoint
      if ( fabs( xNear - x0 ) < fabs( xNear - x1 ) )
         distance = (float) sqrt( (x-x0)*(x-x0) + (y-y0)*(y-y0) );
      else
         distance = (float) sqrt( (x-x1)*(x-x1) + (y-y1)*(y-y1) );
      }

   return onSegment;
   }



float DistancePolyLineInRect( COORD2d vertexArray[], int numverts, REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0 )
   {
   // gets total distance along a polyline segment that is contained within the specified bounding box
   float distance = 0;
   REAL xLeft   = min( xMin0, xMax0 );
   REAL xRight  = max( xMin0, xMax0 );
   REAL yBottom = min( yMin0, yMax0 );
   REAL yTop    = max( yMin0, yMax0 );

   // basic idea - for each edge in the polyline, determine is any portion is in the bounding box.  if it is, calculate
   // length contained within bounding box.
   // For any given edge, it should be examined if
   //   1) the edge is fully contained in the rect
   //   2) the edge crosses the rect edge.

   COORD2d *p0 = &vertexArray[0];

   for ( int i=1; i < numverts; i++ )
      {
      COORD2d *p1 = &vertexArray[ i ];

      // case 1: edge fully contained - full segment distance should be included
      bool isP0InRect = ::IsPointInRect( *p0, xLeft, yBottom, xRight, yTop );
      bool isP1InRect = ::IsPointInRect( *p1, xLeft, yBottom, xRight, yTop );

      if ( isP0InRect && isP1InRect )
         distance += LineLength( p0->x, p0->y, p1->x, p1->y );  

      else
         {
         // case 2: does the edge intersect any of the rect borders?
         COORD2d intersect;
         COORD2d start;
         COORD2d end;
         bool startPopulated = false;
         bool endPopulated = false;

         // left
         COORD2d c0(xLeft,yBottom), c1(xLeft,yTop);
         bool done = _CheckSide( *p0, *p1, c0, c1, isP0InRect, isP1InRect, start, end, startPopulated, endPopulated );

         // top
         if (!done)
            {
            c0.Set(xLeft, yTop);
            c1.Set(xRight, yTop);
            done = _CheckSide(*p0, *p1, c0, c1, isP0InRect, isP1InRect, start, end, startPopulated, endPopulated);
            }

         // right
         if (!done)
            {
            c0.Set(xRight, yTop);
            c1.Set(xRight, yBottom);
            done = _CheckSide(*p0, *p1, c0, c1, isP0InRect, isP1InRect, start, end, startPopulated, endPopulated);
            }
         // bottom
         if (!done)
            {
            c0.Set(xLeft, yBottom);
            c1.Set(xRight, yBottom);
            done = _CheckSide(*p0, *p1, c0, c1, isP0InRect, isP1InRect, start, end, startPopulated, endPopulated);
            }
         if ( ! done )  // segment is completely outside of rect
            {
            //ASSERT( startPopulated == false );
            }
         else
            distance += LineLength( start.x, start.y, end.x, end.y );
         }

      p0 = p1;
      }  // end of: for each vertex

   return distance;
   }


//REAL DistancePolyLineInRect( COORD3d vertexArray[], int numverts, REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0 )
//   {
//   // gets total distance along a polyline segment that is contained within the specified bounding box
//   REAL distance = 0;
//   float xLeft   = min( xMin0, xMax0 );
//   float xRight  = max( xMin0, xMax0 );
//   float yBottom = min( yMin0, yMax0 );
//   float yTop    = max( yMin0, yMax0 );
//
//   // basic idea - for each edge in the polyline, determine is any portion is in the bounding box.  if it is, calculate
//   // length contained within bounding box.
//   // For any given edge, it should be examined if
//   //   1) the edge is fully contained in the rect
//   //   2) the edge crosses the rect edge.
//
//   COORD2d &p0 = vertexArray[0];
//
//   for ( int i=1; i < numverts; i++ )
//      {
//      COORD2d &p1 = vertexArray[ i ];
//
//      // case 1: edge fully contained - full segment distance should be included
//      bool isP0InRect = ::IsPointInRect( p0, xLeft, yBottom, xRight, yTop );
//      bool isP1InRect = ::IsPointInRect( p1, xLeft, yBottom, xRight, yTop );
//
//      if ( isP0InRect && isP1InRect )
//         distance += LineLength( p0.x, p0.y, p1.x, p1.y );  
//
//      else
//         {
//         // case 2: does the edge intersect any of the rect borders?
//         COORD2d intersect;
//         COORD2d start;
//         COORD2d end;
//         bool startPopulated = false;
//         bool endPopulated = false;
//
//         // left
//         bool done = _CheckSide( p0, p1, COORD2d( xLeft, yBottom ), COORD2d ( xLeft, yTop ), isP0InRect, isP1InRect, start, end, startPopulated, endPopulated );
//
//         // top
//         if ( ! done )
//            done = _CheckSide( p0, p1, COORD2d( xLeft, yTop ), COORD2d ( xRight, yTop ), isP0InRect, isP1InRect, start, end, startPopulated, endPopulated );
//
//         // right
//         if ( ! done )
//            done = _CheckSide( p0, p1, COORD2d( xRight, yTop ), COORD2d ( xRight, yBottom ), isP0InRect, isP1InRect, start, end, startPopulated, endPopulated );
//
//         // bottom
//         if ( ! done )
//            done = _CheckSide( p0, p1, COORD2d( xLeft, yBottom ), COORD2d ( xRight, yBottom ), isP0InRect, isP1InRect, start, end, startPopulated, endPopulated );
//
//         if ( ! done )  // segment is completely outside of rect
//            {
//    //        ASSERT( startPopulated == false );
//            }
//         else
//            distance += LineLength( start.x, start.y, end.x, end.y );
//         }
//
//      p0 = p1;
//      }  // end of: for each vertex
//
//   return distance;
//   }


// helper function for DistancePolyLineInRect()

bool _CheckSide( COORD2d &p0, COORD2d &p1, COORD2d &s0, COORD2d &s1,  bool isP0InRect, bool isP1InRect, COORD2d &start, COORD2d &end, bool &startPopulated, bool &endPopulated )
   {
   COORD2d intersect;
   bool crossed = GetIntersectionPt( p0, p1, s0, s1, intersect );

   if ( crossed )
      {
      if ( isP0InRect )
         {
         start = p0;
         end   = intersect;
         startPopulated = endPopulated  = true;
         }
      else if ( isP1InRect )
         {
         start = p1;
         end   = intersect;
         startPopulated = endPopulated  = true;
         }
      else
         {
         // neither are in rect, remember intersection point
         if ( startPopulated == false )
            {
            start = intersect;           
            startPopulated = true;
            }
         else
            {
            end = intersect;
            endPopulated = true;
            }
         }
      }

   return startPopulated && endPopulated;
   }


/*
bool GetIntersectionPtBoost(const COORD2d& thisVertex0, const COORD2d& thisVertex1, const COORD2d& otherVertex0, const COORD2d& otherVertex1, COORD2d& intersection)
   {
   using segment_t = bg::model::segment<BPoint>;
   using points_t = bg::model::multi_point<BPoint>;

   std::vector<BPoint> out;
   segment_t s0(BPoint(thisVertex0.x, thisVertex0.y), BPoint(thisVertex1.x, thisVertex1.y));
   segment_t s1(BPoint(otherVertex0.x, otherVertex0.y), BPoint(otherVertex1.x, otherVertex1.y));
   
   bg::intersection( s0, s1, out);

   if (out.size() == 0)
      return false;

   intersection.x = out[0].x;
   intersection.y = out[0].y;
   return true;
   }
   */

bool GetIntersectionPt( const COORD2d &thisVertex0, const COORD2d &thisVertex1, const COORD2d &otherVertex0, const COORD2d &otherVertex1, COORD2d &intersection)
	{	
	bool verticalOther = false, verticalThis = false, horizontalThis = false, horizontalOther = false;
	float mOther = -999.0f, mThis = -999.0f, bOther = -999.0f, bThis = -999.0f;

	// Get the slopes for Other
	if ( otherVertex0.x == otherVertex1.x )
		 verticalOther = true;

	else if ( otherVertex0.y == otherVertex1.y )
		{
      horizontalOther = true;
      mOther = 0.0f;
      }	
	else
		mOther = (float) (( otherVertex0.y-otherVertex1.y ) / ( otherVertex0.x - otherVertex1.x ));  // it isn't vertical, so calculate the slope of the stream segment
        

	if ( thisVertex0.x == thisVertex1.x )																 //vertical polygon segment??????????
		verticalThis = true;
	else if ( thisVertex0.y == thisVertex1.y )
		{
		horizontalThis = true;
		mThis = 0.0f;
		}
	else
		mThis = (float)(( thisVertex0.y-thisVertex1.y ) / ( thisVertex0.x - thisVertex1.x ) ); //calc slope of polygon segment
	
	
	// Get the intercepts for Other	
	if ( verticalOther == false  ) 
		bOther =  (float)( otherVertex0.y - mOther * otherVertex0.x );         //get intercept of stream line
	

	if ( verticalThis == false ) 
		bThis = (float)(thisVertex0.y - mThis * thisVertex0.x);  //get intercept of polygon side

	ASSERT( ( verticalThis  && horizontalThis  ) == 0 );
   ASSERT( ( verticalOther && horizontalOther ) == 0 );
         // compute the intersection point
   REAL yMinThis = -999.0f, yMaxThis = -999.0f, yMinOther = -999.0f, yMaxOther = -999.0f;
           
   if ( thisVertex0.y < thisVertex1.y )
		{
      yMinThis = thisVertex0.y;
      yMaxThis = thisVertex1.y;
      }
	else
      {
      yMaxThis = thisVertex0.y;
      yMinThis = thisVertex1.y;
      }
	if ( otherVertex0.y < otherVertex1.y )
		{
      yMinOther = otherVertex0.y;
      yMaxOther = otherVertex1.y;
      }
	else
      {
		yMaxOther = otherVertex0.y;
      yMinOther = otherVertex1.y;
      }
	REAL xMinThis = -999.0f, xMaxThis = -999.0f, xMinOther = -999.0f, xMaxOther = -999.0f;
   if ( otherVertex0.x < otherVertex1.x )
		{
      xMinOther = otherVertex0.x;
      xMaxOther = otherVertex1.x;
      }
	else
		{
      xMinOther = otherVertex1.x;
      xMaxOther = otherVertex0.x;
      }
	if ( thisVertex0.x < thisVertex1.x )
		{
      xMinThis = thisVertex0.x;
      xMaxThis = thisVertex1.x;
      }
	else
      {
      xMinThis = thisVertex1.x;
      xMaxThis = thisVertex0.x;
      }

	if ( verticalThis && verticalOther )   // both vertical?
		return false;

	else if ( verticalThis )  // polygon side is vertical, line segment is not
		{
       // get the intersection coordinate
      REAL yIntersect = mOther*thisVertex0.x + bOther;
      if ( horizontalOther )
			{
         if ( xMinOther <= xMinThis && xMinThis <= xMaxOther && yMinThis <= yMinOther && yMinOther <= yMaxThis )
				{
				intersection.x = thisVertex0.x;
				intersection.y = otherVertex0.y;	
				return true;
				}
			else
				return false;
			}
			//  the other line isn't horizontal, so calculate the intersection pt
		else if ( yMinOther <= yIntersect && yIntersect <= yMaxOther  && yMinThis <= yIntersect && yIntersect <= yMaxThis )
			{
			intersection.x = thisVertex0.x;
			intersection.y = yIntersect;
			return true;
			}
		else
			return false;
		}  

	else if ( verticalOther )  // line segment is vertical, polygon segment is not
		{
       // get the intersection coordinate
      REAL yIntersect = mThis*otherVertex0.x + bThis;
      if ( horizontalThis )
			{
         if ( xMinThis <= xMinOther && xMinOther <= xMaxThis  && yMinOther <= yMinThis && yMinThis <= yMaxOther  )
				{
				intersection.x = otherVertex0.x;
				intersection.y = thisVertex0.y;	
				return true;
				}
			else
				return false;

			}
		else if ( yMinThis <= yIntersect && yIntersect <= yMaxThis && yMinOther <= yIntersect && yIntersect <= yMaxOther )
			{
			intersection.x = otherVertex0.x;
			intersection.y = yIntersect;
			return true;
			}
		else
			return false; // they do not intersect
		}


	else   // neither of the line segments are vertical, so proceed...
		{
		if (horizontalOther && horizontalThis)
			return false;

      // get the x intersect         
      float xIntersect = (mThis - mOther) / ( bOther - bThis );

      // same slopes?  need to check to see if the points are colinear (same y intercepts)
      if ( mThis == mOther )
			{
         if ( bThis == bOther )  // colinear
				{
            if ( yMinThis <= yMaxOther && yMinOther <= yMaxThis )  // case where they DO overlap, there is actually a line of intersection...
					{
					intersection.x = xMinThis;
					intersection.y = xMinThis;
					return true;
					}
				else
					return false;
				}
			}
		else  // different slopes...do the lines intersect in the bounds of the segments?
			{
			float xIntersect = ( bOther-bThis )/( mThis - mOther );
			float yIntersect = mOther * xIntersect + bOther;
         if ( xIntersect >= xMinThis && xIntersect >= xMinOther && xIntersect <= xMaxThis && xIntersect <= xMaxOther )
				{
				intersection.x = xIntersect;
				intersection.y = yIntersect;
				return true;
				}
			else
				return false;
			}
		}  // end of:  else (this not vertical and other not vertical)

   //ASSERT( 0 );
   return false;
	}


bool GetIntersectionPt( const COORD3d &thisVertex0, const COORD3d &thisVertex1, const COORD3d &otherVertex0, const COORD3d &otherVertex1, COORD3d &intersection)
	{	
	bool verticalOther = false, verticalThis = false, horizontalThis = false, horizontalOther = false;
	float mOther = -999.0f, mThis = -999.0f, bOther = -999.0f, bThis = -999.0f;

	// Get the slopes for Other
	if ( otherVertex0.x == otherVertex1.x )
		 verticalOther = true;

	else if ( otherVertex0.y == otherVertex1.y )
		{
      horizontalOther = true;
      mOther = 0.0f;
      }	
	else
		mOther = (float)(( otherVertex0.y-otherVertex1.y ) / ( otherVertex0.x - otherVertex1.x ));  // it isn't vertical, so calculate the slope of the stream segment
        

	if ( thisVertex0.x == thisVertex1.x )																 //vertical polygon segment??????????
		verticalThis = true;
	else if ( thisVertex0.y == thisVertex1.y )
		{
		horizontalThis = true;
		mThis = 0.0f;
		}
	else
		mThis = (float)(( thisVertex0.y-thisVertex1.y ) / ( thisVertex0.x - thisVertex1.x )); //calc slope of polygon segment
	
	
	// Get the intercepts for Other	
	if ( verticalOther == false  ) 
		bOther =  (float)(otherVertex0.y - mOther * otherVertex0.x);         //get intercept of stream line
	

	if ( verticalThis == false ) 
		bThis = (float)(thisVertex0.y - mThis * thisVertex0.x);  //get intercept of polygon side

	ASSERT( ( verticalThis  && horizontalThis  ) == 0 );
   ASSERT( ( verticalOther && horizontalOther ) == 0 );
         // compute the intersection point
   REAL yMinThis = -999.0f, yMaxThis = -999.0f, yMinOther = -999.0f, yMaxOther = -999.0f;
           
   if ( thisVertex0.y < thisVertex1.y )
		{
      yMinThis = thisVertex0.y;
      yMaxThis = thisVertex1.y;
      }
	else
      {
      yMaxThis = thisVertex0.y;
      yMinThis = thisVertex1.y;
      }
	if ( otherVertex0.y < otherVertex1.y )
		{
      yMinOther = otherVertex0.y;
      yMaxOther = otherVertex1.y;
      }
	else
      {
		yMaxOther = otherVertex0.y;
      yMinOther = otherVertex1.y;
      }
	REAL xMinThis = -999.0f, xMaxThis = -999.0f, xMinOther = -999.0f, xMaxOther = -999.0f;
   if ( otherVertex0.x < otherVertex1.x )
		{
      xMinOther = otherVertex0.x;
      xMaxOther = otherVertex1.x;
      }
	else
		{
      xMinOther = otherVertex1.x;
      xMaxOther = otherVertex0.x;
      }
	if ( thisVertex0.x < thisVertex1.x )
		{
      xMinThis = thisVertex0.x;
      xMaxThis = thisVertex1.x;
      }
	else
      {
      xMinThis = thisVertex1.x;
      xMaxThis = thisVertex0.x;
      }

	if ( verticalThis && verticalOther )   // both vertical?
		return false;

	else if ( verticalThis )  // polygon side is vertical, line segment is not
		{
       // get the intersection coordinate
      float yIntersect = (float)(mOther*thisVertex0.x + bOther);
      if ( horizontalOther )
			{
         if ( xMinOther <= xMinThis && xMinThis <= xMaxOther && yMinThis <= yMinOther && yMinOther <= yMaxThis )
				{
				intersection.x = thisVertex0.x;
				intersection.y = otherVertex0.y;	
				return true;
				}
			else
				return false;
			}
			//  the other line isn't horizontal, so calculate the intersection pt
		else if ( yMinOther <= yIntersect && yIntersect <= yMaxOther  && yMinThis <= yIntersect && yIntersect <= yMaxThis )
			{
			intersection.x = thisVertex0.x;
			intersection.y = yIntersect;
			return true;
			}
		else
			return false;
		}  

	else if ( verticalOther )  // line segment is vertical, polygon segment is not
		{
       // get the intersection coordinate
      REAL yIntersect = mThis*otherVertex0.x + bThis;
      if ( horizontalThis )
			{
         if ( xMinThis <= xMinOther && xMinOther <= xMaxThis  && yMinOther <= yMinThis && yMinThis <= yMaxOther  )
				{
				intersection.x = otherVertex0.x;
				intersection.y = thisVertex0.y;	
				return true;
				}
			else
				return false;

			}
		else if ( yMinThis <= yIntersect && yIntersect <= yMaxThis && yMinOther <= yIntersect && yIntersect <= yMaxOther )
			{
			intersection.x = otherVertex0.x;
			intersection.y = yIntersect;
			return true;
			}
		else
			return false; // they do not intersect
		}


	else   // neither of the line segments are vertical, so proceed...
		{
		if (horizontalOther && horizontalThis)
			return false;

      // get the x intersect         
      float xIntersect = (mThis - mOther) / ( bOther - bThis );

      // same slopes?  need to check to see if the points are colinear (same y intercepts)
      if ( mThis == mOther )
			{
         if ( bThis == bOther )  // colinear
				{
            if ( yMinThis <= yMaxOther && yMinOther <= yMaxThis )  // case where they DO overlap, there is actually a line of intersection...
					{
					intersection.x = xMinThis;
					intersection.y = xMinThis;
					return true;
					}
				else
					return false;
				}
			}
		else  // different slopes...do the lines intersect in the bounds of the segments?
			{
			float xIntersect = ( bOther-bThis )/( mThis - mOther );
			float yIntersect = mOther * xIntersect + bOther;
         if ( xIntersect >= xMinThis && xIntersect >= xMinOther && xIntersect <= xMaxThis && xIntersect <= xMaxOther )
				{
				intersection.x = xIntersect;
				intersection.y = yIntersect;
				return true;
				}
			else
				return false;
			}
		}  // end of:  else (this not vertical and other not vertical)

   //ASSERT( 0 );
   return false;
	}


//-- FastDistancePtToLine() -----------------------------------------
//
//-- doesn't give correct sign (always positive), but very fast!
//-- doesn't check end segments (point to line, not line segment)
//-- from "Graphics Gems II" (J. Arvo)
//-------------------------------------------------------------------

float FastDistancePtToLine(REAL x0, REAL y0, REAL x1, REAL y1, REAL x, REAL y )
   {                 
   REAL ax = x1 - x0;
   REAL ay = y1 - y0;

   if ( ax == 0 && ay == 0 )  // zero divide check
      {
      ax = x - x0;
      ax = y - y0;
      return (float) sqrt( ax*ax + ay*ay );
      }
      
   float a2 = (float)(( ax * ( y - y0 ) ) - ( ay * ( x - x0 )));

   float dist = (float)(( a2 * a2 ) / ( ( ax * ax ) + ( ay * ay )));

   // approx dist (no sqrt required )...
   //float dist = ( a2 * a2 ) / ( ( ax * ax ) + ( ay * ay ) );

   return (float) sqrt( dist );
   }



/* a few useful vector operations */
#define VZERO(v)    (v.x = v.y = v.z = 0.0f)
#define VNORM(v)    (sqrt(v.x * v.x + v.y * v.y + v.z * v.z))
#define VDOT(u, v)  (u.x * v.x + u.y * v.y + u.z * v.z)
#define VINCR(u, v) (u.x += v->x, u.y += v->y, u.z += v->z)

/*
**  PlaneEquation--computes the plane equation of an arbitrary
**  3D polygon using Newell's method.
**
**  Entry:
**      verts  - list of the vertices of the polygon
**      nverts - number of vertices of the polygon
**  Exit:
**      plane  - normalized (unit normal) plane equation
**
**   from Graphic Gems
*/

void PlaneEquation( COORD3d *verts, int nverts, float &a, float &b, float &c, float &d )
   {
   COORD3d refpt; 
   COORD3d normal;

   // compute the polygon normal and a reference point on
   //  the plane. Note that the actual reference point is
   //  refpt / nverts
   VZERO(normal);
   VZERO(refpt);

   for( int i=0; i < nverts; i++)
      {
      COORD3d *u = &verts[ i ];
      COORD3d *v = &verts[ (i + 1) % nverts ];

      normal.x += (u->y - v->y) * (u->z + v->z);
      normal.y += (u->z - v->z) * (u->x + v->x);
      normal.z += (u->x - v->x) * (u->y + v->y);
      VINCR(refpt, u);
      }

   // normalize the polygon normal to obtain the first
   //    three coefficients of the plane equation
   float len = (float) VNORM( normal );
   a = (float) (normal.x / len);
   b = (float) (normal.y / len);
   c = (float) (normal.z / len);
   /* compute the last coefficient of the plane equation */
   len *= nverts;
   d = (float) (-VDOT(refpt, normal) / len);
   }


float DistanceRectToRect(REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0, REAL xMin1, REAL yMin1, REAL xMax1, REAL yMax1 )
   {
   // if they rects overlap in X and Y, the distance is 0
   bool interceptingX = false;
   bool interceptingY = false;

   if (  BETWEEN( xMin0, xMin1, xMax0 ) 
      || BETWEEN( xMin0, xMax1, xMax0 )
      || BETWEEN( xMin1, xMin0, xMax1 ) 
      || BETWEEN( xMin1, xMax0, xMax1 ) )
      interceptingX = true;

        // repeat for y's
  if ( BETWEEN( yMin0, yMin1, yMax0 ) 
    || BETWEEN( yMin0, yMax1, yMax0 )
    || BETWEEN( yMin1, yMin0, yMax1 ) 
    || BETWEEN( yMin1, yMax0, yMax1 ) )
    interceptingY = true;

  if ( interceptingX && interceptingY )
     return 0.0f;

  // rects don't overlap in both.  If they overlap in one only, compute distance directly
  if ( interceptingX )
     {
     if ( yMin1 > yMax0 )  // 1 "above" 0?
        return float( yMin1 - yMax0 );
     else      // 1 "below" 0
        return float( yMin0-yMax1 );
     }

   if ( interceptingY )
     {
     if ( xMin1 > xMax0 )  // 1 "to the right of" 0?
        return float( xMin1 - xMax0 );
     else      // 1 "to the left of" 0
        return float( xMin0 - xMax1 );
     }

   // not intercepting in either dimension.  find sides to compute from.
   REAL x0, y0, x1, y1;

   if ( xMin1 > xMax0 )  // 1 "to the right of" 0?
      {
      x0 = xMax0;
      x1 = xMin1;
      }
   else      // 1 "to the left of" 0
      {
      x0 = xMin0;
      x1 = xMax1;
      }

   if ( yMin1 > yMax0 )  // 1 "abovef" 0?
      {
      y0 = yMax0;
      y1 = yMin1;
      }
   else      // 1 "below" 0
      {
      y0 = yMin0;
      y1 = yMax1;
      }

   return (float) sqrt( (x0-x1)*(x0-x1) + (y0-y1)*(y0-y1) );
   }


bool DoRectsIntersect(REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0, REAL xMin1, REAL yMin1, REAL xMax1, REAL yMax1 )
   {
   // if they rects overlap in X and Y, the distance is 0
   bool interceptingX = false;
   bool interceptingY = false;

   if (  BETWEEN( xMin0, xMin1, xMax0 ) 
      || BETWEEN( xMin0, xMax1, xMax0 )
      || BETWEEN( xMin1, xMin0, xMax1 ) 
      || BETWEEN( xMin1, xMax0, xMax1 ) )
      interceptingX = true;

   // repeat for y's
   if (  BETWEEN( yMin0, yMin1, yMax0 ) 
      || BETWEEN( yMin0, yMax1, yMax0 )
      || BETWEEN( yMin1, yMin0, yMax1 ) 
      || BETWEEN( yMin1, yMax0, yMax1 ) )
      interceptingY = true;

   if ( interceptingX && interceptingY )
      return true;

   return false;
   }
