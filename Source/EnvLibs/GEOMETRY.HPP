#pragma once
#include "libs.h"


#if !defined _TYPEDEFS_H
#include "Typedefs.h"
#endif

#if !defined __MATH_H
#include <math.h>
#endif


//--------------------------- Query ----------------------//

int  LIBSAPI IsPointInPoly( COORD2d vertexArray[], int numverts, COORD2d &point );
int  LIBSAPI IsPointInPoly( LPPOINT vertexArray, int numVerts, POINT &point );
bool LIBSAPI IsPointOnLine( LPPOINT vertexArray, int numVerts, POINT &point, int tolerance );  // segment only
bool LIBSAPI IsPointOnRect( RECT &rect, POINT &pt, int tolerance );    // over edges only
bool LIBSAPI IsPointInRect( RECT &rect, POINT &pt, int tolerance );    // edges or interior
bool LIBSAPI IsPointInRect( COORD2d &pt, REAL rXMin, REAL rYMin, REAL rXMax, REAL rYMax );    // edges or interior

bool LIBSAPI IsPolyLineInRect( COORD2d vertexArray[], int numverts, REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0 );
//bool LIBSAPI IsPolyLineInRect( COORD3d vertexArray[], int numverts, REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0 );

//-------------------- Point to Point methods ------------//

float LIBSAPI LineLength( REAL x0, REAL y0, REAL x1, REAL y1 );

float LIBSAPI DistancePtToPt( const COORD2d &start, const COORD2d &end );
float LIBSAPI DistancePtToPt( const REAL, const REAL, const REAL, const REAL );
float LIBSAPI DistancePolyLineInRect( COORD2d vertexArray[], int numverts, REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0 );
//REAL LIBSAPI DistancePolyLineInRect( COORD3d vertexArray[], int numverts, REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0 );


//---------------------- Point-Line methods --------------//

float  LIBSAPI DistancePtToLine( REAL x, REAL y, REAL x0, REAL y0, REAL x1, REAL y1 );
float  LIBSAPI DistancePtToLine( POINT &point,   POINT &start, POINT &end );
float  LIBSAPI DistancePtToLine( REAL x, REAL y, COORD2d &start, COORD2d &end );


float LIBSAPI FastDistancePtToLine(REAL x, REAL y, REAL x0, REAL y0, REAL x1, REAL y1 );

float LIBSAPI FastDistancePtToLine( POINT &point,     POINT &start, POINT &end );
float LIBSAPI FastDistancePtToLine(REAL x, REAL y,    COORD2d &start, COORD2d &end );

bool LIBSAPI DistancePtToLineSegment(REAL x, REAL y, REAL x0, REAL y0, REAL x1, REAL y1, float &distance );
//bool DistancePtToLineSegment( int x, int y, int x0, int y0, int x1, int y1, float &distance );

// GetInterectionPt() returns true if intersection found witing the bounds of the segments, false otherwise
bool LIBSAPI GetIntersectionPt( const COORD3d &thisVertex0, const COORD3d &thisVertex1, const COORD3d &otherVertex0, const COORD3d &otherVertex1, COORD3d &intersection );
bool LIBSAPI GetIntersectionPt( const COORD2d &thisVertex0, const COORD2d &thisVertex1, const COORD2d &otherVertex0, const COORD2d &otherVertex1, COORD2d &intersection );

// rect methods
// Note: these methods assume x, y's are ordered
float LIBSAPI DistanceRectToRect(REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0, REAL xMin1, REAL yMin1, REAL xMax1, REAL yMax1 );

bool LIBSAPI DoRectsIntersect(REAL xMin0, REAL yMin0, REAL xMax0, REAL yMax0, REAL xMin1, REAL yMin1, REAL xMax1, REAL yMax1 );


//----------------------- Plane methods -------------------//
void LIBSAPI PlaneEquation( COORD3d *verts, int nverts, float &a, float &b, float &c, float &d );



//--------------------------- Inlines -----------------------------//

inline
float LineLength(REAL x0, REAL y0, REAL x1, REAL y1 )
   {
   return (float) sqrt( (x1-x0)*(x1-x0) + (y1-y0)*(y1-y0) );
   }

//-- DistancePtToPt() -----------------------------------------------
//
//-- calculates the distance between two Coords
//
//-- Comments:
//      1) Distance is always returned as a positive number
//      2) Doesn't matter if using logical or virtual, but both
//         coords must be consistent!
//-------------------------------------------------------------------

inline
float DistancePtToPt( const COORD2d &start, const COORD2d &end )
   {
   REAL dx = start.x - end.x;
   REAL dy = start.y - end.y;

   return (float) (sqrt( (dx*dx) + (dy*dy) ));
   }


inline
float DistancePtToPt( const REAL x0, const REAL y0,  // )
                      const REAL x1, const REAL y1 )
   {
   REAL dx = x0 - x1;
   REAL dy = y0 - y1;

   return (float) (sqrt( (dx*dx) + (dy*dy) ));
   }


//-- DistancePtToLine() ---------------------------------------------
//
//-- calculates the perpendicular distance from a point to a line
//-------------------------------------------------------------------

inline
float DistancePtToLine( POINT &point, POINT &start, POINT &end )
   {
   return DistancePtToLine( (REAL) point.x, (REAL) point.y, (REAL) start.x, (REAL) start.y, (REAL) end.x, (REAL) end.y );
   }


inline
float DistancePtToLine( REAL x, REAL y, COORD2d &start, COORD2d &end )
   {
   return (float) DistancePtToLine( x, y, start.x, start.y, end.x, end.y );
   }


inline
float FastDistancePtToLine( POINT &point, POINT &start, POINT &end )
   {
   return (float) (FastDistancePtToLine( (float) point.x, (float)point.y, 
                                (float)start.x, (float) start.y, (float) end.x, (float) end.y ));
   }


inline
float FastDistancePtToLine(REAL x, REAL y, COORD2d &start, COORD2d &end )
   {
   return FastDistancePtToLine( x, y, start.x, start.y, end.x, end.y );
   }


