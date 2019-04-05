//===================================================================
//  EXTENTS.HPP
//
//  -- Include file for global extents and conversions
//
//  NOTE: this file should be used in CPP files only, not HPP
//===================================================================

#pragma once
#include "libs.h"

#if !defined _TYPEDEFS_H
#include <typedefs.h>
#endif

//------------------------- Extents class -------------------------//

//-------------------------------------------------------------------
// Comments:
//   1.  xClient and yClient are the client dimensions of
//       the associated window, and must be reset each time the
//       window is resized
//   2.  View extents are virtual extents which map directly to the
//       current window client area, maintaining a square aspect
//       ratio.  The do NOT map exactly into the client rect.  Each
//       time the window is resized, panned or zoomed, they must be set.
//   3.  Frame extents are the user-define intended viewing window.
//       It does NOT need to have the same aspect as the Client rect.
//       When the application sets the frame rect, the corresponding 
//       View extents are calculated based on the client aspect.
//   4.  In all cases, and MM_TEXT mapping is assumed in the hdc.
//-------------------------------------------------------------------

class Extents
   {
   protected:
      //-- handle of the window this extents is associated with --//
      HWND  hWnd;

      int   xClient;    // logical (device) extents - based on client rect
      int   yClient;

      float xView;      // view window width, height (virtual coords)
      float yView;      // NOTE: these are for INTERNAL use only

      //-- viewing extents (virtual coords) (see comments above) --//
      float xMinView,  yMinView,  xMaxView,  yMaxView;

      //-- current frame extents (see comments above) --//
      float xMinFrame, yMinFrame, xMaxFrame, yMaxFrame;
      float xMinPrev,  yMinPrev,  xMaxPrev,  yMaxPrev;

   public:
      //-- constructor --//
      Extents( HWND hWnd );

      //-- use this to set up view --//
		void SetFrame( REAL xMin, REAL  yMin, REAL xMax, REAL yMax );

      //-- use this to set up view --//
		void GetFrame( REAL *xMin, REAL  *yMin, REAL *xMax, REAL *yMax )
         { *xMin=xMinFrame; *yMin=yMinFrame; *xMax=xMaxFrame; *yMax=yMaxFrame; }

      //-- get current virtual screen dimensions --//
      void GetView( REAL *xMin, REAL  *yMin, REAL *xMax, REAL *yMax )
                           { *xMin = xMinView; *yMin = yMinView;
                             *xMax = xMaxView; *yMax = yMaxView; }

      //-- miscellaneous --//
      void RestorePrevFrame( void )
               { SetFrame( xMinPrev, yMinPrev, xMaxPrev, yMaxPrev ); }

      void PanLeft( REAL offset )
               { SetFrame( xMinFrame-offset, yMinFrame,
                           xMaxFrame-offset, yMaxFrame ); }

      void PanUp( REAL offset )
               { SetFrame( xMinFrame, yMinFrame-offset,
                           xMaxFrame, yMaxFrame-offset ); }

      void ZoomX      ( float factor );
      void ZoomIn2x   ( void ) { ZoomX( float(0.5) ); }
      void ZoomOut2x  ( void ) { ZoomX( float(2.0) ); }
      void ZoomExtents( void );

      void Resize    ( void );  // called whenever hWnd is resized

      //-- coordinant transformation --//
      void  VPtoLP( REAL &x, REAL &y );
      void  VPtoLP( COORD2d &coord ) { VPtoLP( coord.x, coord.y ); }

      void  LPtoVP( REAL &x, REAL &y );
      void  LPtoVP( COORD2d &coord ) { LPtoVP( coord.x, coord.y ); }
      COORD2d LPtoVP( POINT &point );


      //-- convert distance between Client and virtual --//
      int   VDtoLD( float size );
      float LDtoVD( int  size );

      BOOL IsRectInView( const COORD2d &upperLeft, const COORD2d &lowerRight );
   };


//----------------------------- Inlines ---------------------------//

inline
int Extents::VDtoLD( float size )
   {
   //-- check for divide-by-zero error --//
   if ( ! xView )
      return 0;

   return (int) ( ( (float) xClient * size / xView ) + 0.5 );
   }


inline
float Extents::LDtoVD( int size )
   {
   return ( ((float) size) * xView / xClient );
   }


//-- Extents::VPtoLP() ----------------------------------------------
//
//-- Scales a virtual coordinant to a Client coordinant for use in
//   a window device context. This assures sufficient resolution
//   in moving from float to int based display units.  Note:  the
//   device context is assumed to be using MM_TEXT mapping (isotropic)
//   Also, the Extents MUST be associated with the window that provide
//   the device context for conversion
//
//-- Note: 0.5 is added to the converted coords to round correctly
//   when truncating to an int.
//-------------------------------------------------------------------

inline
void Extents::VPtoLP( REAL &x, REAL &y )
   {
   //-- check for divide-by-zero error --//
   if ( xView == 0 || yView == 0 )
      return;

   x = ( xClient * ( x - xMinView ) / xView ) + 0.5f;
   y = ( yClient - ( yClient * ( y - yMinView ) / yView ) ) + 0.5f;
   }


//-- Extents::LPtoVP() ----------------------------------------------
//
//-- Scales a Client coordinant to a virtual coordinant.
//-------------------------------------------------------------------

inline
void Extents::LPtoVP( REAL &x, REAL &y )
   {
   x = xMinView + ( x * xView / xClient );
   y = yMinView + ( ( yClient-y ) * yView / yClient );
   }


inline
COORD2d Extents::LPtoVP( POINT &point )
   {
   COORD2d coord;
   coord.x = (float) point.x;
   coord.y = (float) point.y;
   LPtoVP( coord.x, coord.y );
   return coord;
   }

