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

#include "OpenGLWnd.h"
#include <afxtempl.h>

enum P3D_MENUS
   {
   P3D_M_FRAME          = 1000,         
   P3D_M_LIGHTING       = 1100,
   P3D_M_PROJECTION     = 1200, 
   P3D_M_ROTATION       = 1300, 
   P3D_M_BACKGROUND     = 1400,  
   //P3D_M_SHOWTICKLABELS = 1500,
   //P3D_M_SHOWTICKS,
   //P3D_M_SHOWAXESLABELS
   };

enum P3D_FRAME_TYPE
   {
   P3D_FT_NONE      = P3D_M_FRAME,
   P3D_FT_DEFAULT,
   P3D_FT_BOXED,
   P3D_FT_PLANES
   };

enum P3D_PROJECTION_TYPE
   {
   P3D_PT_ORTHO        = P3D_M_PROJECTION,
   P3D_PT_PERSPECTIVE
   };

enum P3D_ROTATION_TYPE
   {
   P3D_RT_NONE         = P3D_M_ROTATION,
   P3D_RT_TRACKBALL,
   P3D_RT_MAPLE
   };

enum P3D_SHAPE_TYPE
   {
   P3D_ST_CONE,
   P3D_ST_CUBE,
   P3D_ST_CYLINDER,
   P3D_ST_ISOSAHEDRON,
   P3D_ST_PIXEL,
   P3D_ST_SPHERE,
   P3D_ST_LAST
   };

static const char *P3D_SHAPE_TYPE_NAME[] = 
   {
   "Cone",
   "Cube",
   "Cylinder",
   "Isosahedron",
   "Pixel",
   "Sphere"
   };

enum P3D_LINE_TYPE
   {
   P3D_LT_PIXEL
   };


// 3DText Properties
const UINT P3D_TXT_TOP     = 2;
const UINT P3D_TXT_VCENTER = 4;
const UINT P3D_TXT_BOTTOM  = 8; 

const UINT P3D_TXT_LEFT    = 16;
const UINT P3D_TXT_HCENTER = 32;
const UINT P3D_TXT_RIGHT   = 64;

const UINT P3D_TXT_CENTER        = P3D_TXT_HCENTER | P3D_TXT_VCENTER;
const UINT P3D_TXT_TOPCENTER     = P3D_TXT_TOP | P3D_TXT_HCENTER;
const UINT P3D_TXT_BOTTOMCENTER  = P3D_TXT_BOTTOM | P3D_TXT_HCENTER;
const UINT P3D_TXT_TOPLEFT       = P3D_TXT_TOP | P3D_TXT_LEFT;
const UINT P3D_TXT_BOTTOMLEFT    = P3D_TXT_BOTTOM | P3D_TXT_LEFT;

// Standard basis
const glReal3 I = glReal3( 1.0, 0.0, 0.0 );
const glReal3 J = glReal3( 0.0, 1.0, 0.0 );
const glReal3 K = glReal3( 0.0, 0.0, 1.0 );

// Null Vector
const glReal3 NULLVECTOR = glReal3( 0, 0, 0 );

enum P3D_AXES       // indexies in axes array
   {
   P3D_XAXIS   = 0,
   P3D_YAXIS   = 1,
   P3D_ZAXIS   = 2,
   P3D_XAXIS2  = 3,
   P3D_YAXIS2  = 4,
   P3D_ZAXIS2  = 5
   };

// Axis
class Axis
   {
   private:
      Axis(){ ASSERT(0); }
   public:
      Axis( glReal3 direction, bool primary );
      //Axis( glReal3 location, glReal3 direction, glReal3 tickDir, glReal3 normal );
      
      // visibility
      static const UINT TICKS      = 1;
      static const UINT LABELTICKS = 2;
      static const UINT LABELAXIS  = 4;
      static const UINT ALL        = TICKS | LABELTICKS | LABELAXIS;
      static const UINT LABELS     = LABELTICKS | LABELAXIS;

   private:
      UINT m_visibility;

      // real coord extents
      glReal m_min;
      glReal m_max;

      // axis info
      glReal3 m_location;   // linear combination of I, J, and K
      glReal3 m_direction;  // I, J, or K, Set during construction and never changed
      bool    m_fixedPoint;
      bool    m_primary;
      
      // plane
      glReal3 m_tickDir;   
      glReal3 m_normal;  
      bool    m_fixedPlane;

      // ticks
      glReal m_start;      // Real coords
      glReal m_step;       // Real coords

      CString  m_label;
      COLORREF m_labelColor;

      bool    m_flipText;
      bool    m_rotateText;

   public:
      void AdjustToEye( glReal3 eyeLocation, glReal3 up );
      void StandardFixed( bool fixedPlane = false );
      
      // Visibility
      //void Show( int visibility ){ m_visibility = visibility; }
      void ShowAll( bool showAll ){ if (showAll) m_visibility = ALL; else m_visibility = 0; }
      void ShowTicks( bool showTicks ){ if (showTicks) m_visibility |= TICKS; else m_visibility &= ~TICKS; }
      void ShowLabels( bool showLabels ){ if (showLabels) m_visibility |= LABELS; else m_visibility &= ~LABELS; }   
      void ShowTickLabels( bool showTickLabels ){ if (showTickLabels) m_visibility |= LABELTICKS; else m_visibility &= ~LABELTICKS; }   
      void ShowAxisLabels( bool showAxisLabels ){ if (showAxisLabels) m_visibility |= LABELAXIS; else m_visibility &= ~LABELAXIS; }   
      
      // Sets
      void SetLocation( glReal3 location, bool fixed = true ){ m_location = location; m_fixedPoint = fixed; }
      void SetPlane( glReal3 normal, bool fixed = true ){ m_normal = normal; m_fixedPlane = fixed; }
      void SetTicks( glReal start, glReal step ){ m_start = start; m_step = step; }
      void SetLabel( LPCTSTR label ){ m_label = label; }
      void SetLabelColor( COLORREF rgb ){ m_labelColor = rgb; }
      void SetExtents( glReal _min, glReal _max ){ m_min = _min; m_max = _max; }
      
      // Gets
      glReal3  GetTextDown(){ if (m_flipText) return -m_tickDir; else return m_tickDir;  }  // do a little dance, make a little love
      glReal3  GetTickDir(){ return m_tickDir; }
      glReal3  GetNormal(){ return m_normal; }
      glReal3  GetLocation(){ return m_location; }
      glReal3  GetDirection(){ return m_direction; }
      glReal   GetStart(){ return m_start; }
      glReal   GetStep(){ return m_step; }
      glReal   GetMin(){ return m_min; }
      glReal   GetMax(){ return m_max; }
      CString  GetLabel(){ return m_label; }
      COLORREF GetLabelColor(){ return m_labelColor; }
      bool     IsFlipText(){ return m_flipText; }
      bool     IsVisible(){ return m_visibility ? true : false; }
      bool     IsLabelTicks(){ return m_visibility & LABELTICKS ? true : false; }
      bool     IsLabelAxis(){ return m_visibility & LABELAXIS ? true : false; }

      bool Dum();

      // others
      void AutoScale( glReal _min, glReal _max );

      // NiceNum: find a "nice" number approximately equal to x.
      // Round the number if roundOff, take ceiling if !roundOff
      glReal NiceNum( glReal num, bool roundOff ); 
   };


// Plot3DWnd

class WINLIBSAPI Plot3DWnd : public COpenGLWnd
{
public:
	Plot3DWnd();
	virtual ~Plot3DWnd();

protected:
   // Axes
   CArray< Axis*, Axis* > m_axisArray;

   // Mouse View Rotation
   P3D_ROTATION_TYPE m_rotationType;
   BOOL              m_inMouseRotate;
   BOOL              m_guideMouseRotate;
   CPoint            m_leftDownPos;

   // View Position
   glReal3 m_eye;   // rectangular choords
   glReal3 m_up;
   glReal  m_eyeDist;  // ro in spherical coords

   // Plot Properties
   COLORREF       m_axesColor;
   COLORREF       m_backgroundColor;
   bool           m_whiteBackground;
   bool           m_useLighting;
   P3D_FRAME_TYPE m_frameType;

   //bool           m_showTickLabels;
   //bool           m_showTicks;
   //bool           m_showAxesLabels;

   P3D_PROJECTION_TYPE m_projType;
   double m_aspect;

public:
   int     m_startIndex;      // starting point to plot, -1=all
   int     m_endIndex;        // ending point to plot, -1=all

protected:
   void SetLighting( bool turnOn );
   void SetProjection( P3D_PROJECTION_TYPE type );

   void SetColorObject( COLORREF rgb );
   void SetColorEmission( COLORREF rgb );

   void EnableClippingPlanes( bool enable );

   // Conversions
   void RealToLogical( glReal3 *pPoint );  // converts Real coordinates to the logical [0.0,1.0]^3 cube 

   bool IsPointInPlot( glReal3 point );

   // Shape Drawing
   void DrawShape( P3D_SHAPE_TYPE type, glReal3 point, int size = 1 );
   void DrawSphere();
   void DrawIsosahedron();
   void DrawCylinder();
   void DrawCone();
   void DrawCube();
   void DrawLine( glReal3 start, glReal3 end );

   void DrawPrism( glReal3 length, glReal3 width, glReal3 height );  

   void DrawText3D( const char *str, glReal3 anchor, glReal3 downSize, glReal3 normal, int style = P3D_TXT_TOPLEFT );

   // Context Menu
   CMenu m_menu;
   virtual void BuildPopupMenu( CMenu* pMenu );
   virtual void OnSelectOption( UINT nID );

   // Overridables
   virtual void DrawData();
   virtual void DrawFrame();
   virtual void DrawAxes();

public:
   // Gets
   P3D_PROJECTION_TYPE GetProjectionType( void ){ return m_projType; }
   P3D_ROTATION_TYPE   GetRotationType( void ){ return m_rotationType; }
   glReal3             GetSphericalFromRectangular( glReal3 rect );
   glReal3             GetRectangularFromSpherical( glReal3 rect );
   Axis               *GetAxis( P3D_AXES axis ){ return m_axisArray.GetAt( axis ); }

   glReal GetXMin(){ return GetAxis( P3D_XAXIS )->GetMin(); }
   glReal GetYMin(){ return GetAxis( P3D_YAXIS )->GetMin(); }
   glReal GetZMin(){ return GetAxis( P3D_ZAXIS )->GetMin(); }

   glReal GetXMax(){ return GetAxis( P3D_XAXIS )->GetMax(); }
   glReal GetYMax(){ return GetAxis( P3D_YAXIS )->GetMax(); }
   glReal GetZMax(){ return GetAxis( P3D_ZAXIS )->GetMax(); }

   //bool GetShowTicks(){ return m_showTicks; }
   //bool GetShowTickLabels(){ return m_showTickLabels; }
   //bool GetShowAxesLabels(){ return m_showAxesLabels; }

   // Sets
   void SetProjectionType( P3D_PROJECTION_TYPE type ){ m_projType=type; }
   void SetRotationType( P3D_ROTATION_TYPE type ){ m_rotationType=type; }
   void SetAllowMouseRotation( bool allow ){ m_rotationType = P3D_RT_NONE; }

   void SetAxesVisible( bool visible );
   void SetAxisBounds( glReal xMin, glReal xMax, glReal yMin, glReal yMax, glReal zMin, glReal zMax );
   void SetAxisTicks( glReal xTick, glReal yTick, glReal zTick, glReal xTickStart, glReal yTickStart, glReal zTickStart );
   void SetAxisTicks( glReal xTick, glReal yTick, glReal zTick );
   void SetAxesLabels( LPCTSTR xLabel, LPCTSTR yLabel, LPCTSTR zLabel );

   void SetAxesVisible2( bool visible );
   void SetAxisBounds2( glReal xMin, glReal xMax, glReal yMin, glReal yMax, glReal zMin, glReal zMax );
   void SetAxisTicks2( glReal xTick, glReal yTick, glReal zTick, glReal xTickStart, glReal yTickStart, glReal zTickStart );
   void SetAxisTicks2( glReal xTick, glReal yTick, glReal zTick );
   void SetAxesLabels2( LPCTSTR xLabel, LPCTSTR yLabel, LPCTSTR zLabel );

   void SetFrameType( P3D_FRAME_TYPE type ){ m_frameType = type; }

   //void SetShowTicks( bool showAxesTicks ){ m_showAxesTicks = showAxesTicks; }
   //void SetShowTickLabels( bool showTickLabels ){ m_labelTicks = showTickLabels; }
   //void SetShowAxesLabels( bool showAxesLabels ){ m_showAxesLabels = showAxesLabels; }


protected:
	DECLARE_MESSAGE_MAP()

public:
   virtual void OnCreateGL();
   virtual void OnDrawGL();
   virtual void OnSizeGL(int cx, int cy); // override to adapt the viewport to the window
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
protected:
   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
   afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};


