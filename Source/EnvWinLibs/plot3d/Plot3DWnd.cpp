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
// Plot3DWnd.cpp : implementation file
//

#include "EnvWinLibs.h"
#include "plot3d/Plot3DWnd.h"

#include <float.h>
#include <math.h>

static const glReal pi = 3.14159265359;

//////////////////////////////////////////////////////////////////////
//   Axis
//////////////////////////////////////////////////////////////////////
Axis::Axis( glReal3 direction, bool primary )
   :
   m_visibility( 0 ),
   m_min( 0.0 ),
   m_max( 1.0 ),
   m_direction( direction ),
   m_fixedPoint( false ),
   m_fixedPlane( false ), 
   m_start( 0.0 ), 
   m_step( 1.0 ),
   m_flipText( false ), 
   m_rotateText( false ),
   m_primary( primary ),
   m_labelColor( RGB(0,0,0) )
   {   

   //if ( primary )
   //   m_visibility = ALL;
   //else
   //   m_visibility = 0;  // secondary axes are not visible initially

   }

bool Axis::Dum()
   { 
   return true; 
   }

//Axis::Axis( glReal3 location, glReal3 direction, glReal3 tickDir, glReal3 normal )
//   :
//   m_visibility( ALL ),
//   m_min( 0.0 ),
//   m_max( 1.0 ),
//   m_direction( direction ),
//   m_tickDir( tickDir ),
//   m_normal( normal ),
//   m_fixedPoint( true ),
//   m_fixedPlane( true ), 
//   m_start( 0.0 ), 
//   m_step( 1.0 ),
//   m_flipText( false ), 
//   m_primary( false )
//   {   
//   }

void Axis::AdjustToEye( glReal3 eye, glReal3 up )
   {

   if ( m_fixedPoint && m_fixedPlane )
      return;

   glReal3 axisAnchor;
   glReal3 normal;
   glReal3 tickDir;

   glReal3 basis[3] = { I, J, K };
   int     index;

   glReal3 center(0.5,0.5,0.5);
   glReal3 dirToEye = normalize( eye );

   if ( m_direction == I )
      index = 0;
   else if ( m_direction == J )
      index = 1;
   else if ( m_direction == K )
      index = 2;
   else
      {
      ASSERT(0);
      return;
      }

   if ( !m_fixedPoint )
      {
      // Choose location
      glReal3 location = NULLVECTOR;

      glReal3 basis1 = basis[ (index+1)%3 ];
      glReal3 basis2 = basis[ (index+2)%3 ];

      //glReal dist;
      //if (m_primary)
      //   dist = -FLT_MAX;
      //else
      //   dist = FLT_MAX;

      glReal height;
      if (m_primary)
         height = FLT_MAX;
      else
         height = -FLT_MAX;

      for ( int m=0; m<2; m++ )
         for ( int n=0; n<2; n++ )
            {
                        
            // only consider locations that are adjacent to exactly one back plane
            if (   ( dot( -(2*m-1)*basis1, dirToEye ) > 0.0 )
                 ^ ( dot( -(2*n-1)*basis2, dirToEye ) > 0.0 ) )  // ^ is binary exclusive or
               {
               bool doIt = false;
               //glReal thisDist = dot( m*basis1 + n*basis2 + 0.5*m_direction, dirToEye );
               glReal thisHeight = dot( m*basis1 + n*basis2 + 0.5*m_direction, up );
               if ( m_primary )
                  {
                  //if ( thisDist > dist )
                  //   doIt = true;
                  if ( thisHeight < height )
                     doIt = true;
                  }
               else
                  {
                  //if ( thisDist < dist )
                  //   doIt = true;
                  if ( thisHeight > height )
                     doIt = true;
                  }
               
               if (doIt)
                  {
                  location = m*basis1 + n*basis2;
                  //dist = thisDist;
                  height = thisHeight;
                  }
               }
            }

      m_location = location;
      }

   if ( !m_fixedPlane )
      {
      // Optimize tick direction in this location
      glReal  sign;
      glReal3 tick1;
      glReal3 tick2;

      sign  = ( m_location[ (index+1)%3 ] - 0.5 )/ fabs( m_location[ (index+1)%3 ] - 0.5 );
      tick1 = sign*basis[ (index+1)%3 ];
      sign  = ( m_location[ (index+2)%3 ] - 0.5 )/ fabs( m_location[ (index+2)%3 ] - 0.5 );
      tick2 = sign*basis[ (index+2)%3 ];
      
      if ( fabs( dot( eye, tick1 ) ) < fabs( dot( eye, tick2 ) ) )
         m_tickDir = tick1;
      else
         m_tickDir = tick2;

      glReal3 normal = cross( m_tickDir, m_direction );
      if ( angle( eye, normal ) < angle( eye, -normal ) )
         m_normal = normal;
      else
         m_normal = -normal;
      }
   else // enter this loop if m_fixedPlane == true && m_fixedPoint == false
      {
      m_tickDir  = normalize( cross( m_normal, m_direction ) );
      if ( abs(m_location+0.3*m_tickDir-glReal3(0.5,0.5,0.5)) < abs( m_location-0.3*m_tickDir-glReal3(0.5,0.5,0.5) ) )
         m_tickDir = -m_tickDir;

      if ( angle( eye, m_normal ) > angle( eye, -m_normal ) )
         m_normal = -m_normal;
      }

   if ( dot( m_tickDir, up ) > 0 )
      m_flipText = true;
   else
      m_flipText = false;

   }

void Axis::StandardFixed( bool fixedPlane /* = false */ )
   {
   m_location = NULLVECTOR;

   if ( m_direction == I )
      {
      m_tickDir = -J;
      m_normal  = K;
      }
   else if ( m_direction == J )
      {
      m_tickDir = -I;
      m_normal  = K;
      }
   else if ( m_direction == K )
      {
      m_tickDir = -I;
      m_normal  = -J;
      }
   else
      {
      ASSERT(0);
      return;
      } 
   m_fixedPoint = true;
   m_fixedPlane = fixedPlane;
   }

void Axis::AutoScale( glReal _min, glReal _max )
   {
   glReal range = NiceNum( _max - _min, false );
   m_step = NiceNum( range/4.0, true );
   m_min = floor( _min/m_step )*m_step;
   m_max = ceil( _max/m_step )*m_step;
   m_start = m_min;
   }

// Nice Numbers for Graph Labels
// by Paul Heckbert
// from "Graphics Gems", Academic Press, 1990
glReal Axis::NiceNum( glReal num, bool roundOff )
   {
   int expv = (int)floor(log10(num));  // exponent of num 
   glReal f = num/pow(10.0, expv);     // fractional part of num ( between 1 and 10 )
   glReal nf;                        // nice, rounded fraction 

   if (roundOff)
      {
      if ( f < 1.5 ) nf = 1.0;
      else if ( f < 3.0 ) nf = 2.0;
      else if (f < 7.0 ) nf = 5.0;
      else nf = 10.0;
      }
   else
      {
      if (f <= 1.0 ) nf = 1.0;
      else if ( f <= 2.0 ) nf = 2.0;
      else if ( f <= 5.0 ) nf = 5.0;
      else nf = 10.0;
      }

   return nf*pow(10.0, expv);
   }

////////////////////////////////////////////////////////////////////////
//   Plot3DWnd
////////////////////////////////////////////////////////////////////////
Plot3DWnd::Plot3DWnd()
   : m_leftDownPos(0,0),
     m_projType( P3D_PT_ORTHO ),
     m_frameType( P3D_FT_DEFAULT ),
     m_rotationType( P3D_RT_MAPLE /*P3D_RT_TRACKBALL*/ ),
     m_inMouseRotate( false ),
     m_guideMouseRotate( false ),
     m_useLighting( true ),
     m_whiteBackground( true ),
     m_aspect( 1.0 ),
     m_startIndex( -1 ),
     m_endIndex( -1 )
   {
   m_axisArray.SetSize(6,0);
   m_axisArray.SetAt( P3D_XAXIS,  new Axis( I, true ) );
   m_axisArray.SetAt( P3D_YAXIS,  new Axis( J, true ) );
   m_axisArray.SetAt( P3D_ZAXIS,  new Axis( K, true ) );
   m_axisArray.SetAt( P3D_XAXIS2, new Axis( I, false ) );
   m_axisArray.SetAt( P3D_YAXIS2, new Axis( J, false ) );
   m_axisArray.SetAt( P3D_ZAXIS2, new Axis( K, false ) );
   
   m_eye = normalize( glReal3( 1.0, 0.5, 0.5 ) + glReal3(0.5,0.5,0.5) );
   m_up  = normalize( cross( cross( m_eye, K ), m_eye ) );
   m_eyeDist = 4.5;

   if ( m_whiteBackground )
      {
      m_axesColor       = RGB(0,0,0);
      m_backgroundColor = RGB(255,255,255);
      }
   else
      {
      m_axesColor       = RGB(255,255,255);
      m_backgroundColor = RGB(0,0,0);
      }
   }

Plot3DWnd::~Plot3DWnd()
   {
   for ( int i=0; i<m_axisArray.GetCount(); i++ )
      delete m_axisArray.GetAt(i);

   m_axisArray.RemoveAll();
   }  

BEGIN_MESSAGE_MAP(Plot3DWnd, COpenGLWnd)
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_MOUSEMOVE()
   ON_WM_KEYDOWN()
   ON_WM_RBUTTONDOWN()
   ON_COMMAND_RANGE(1000, 10000, OnSelectOption)
END_MESSAGE_MAP()

void Plot3DWnd::OnCreateGL()
   {
   // Lighting
   if ( m_useLighting )
      SetLighting( true );

   glEnable(GL_DEPTH_TEST);
   SetClearCol( m_backgroundColor );

   // set clear Z-Buffer value
	glClearDepth(1.0f);

	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   }

void Plot3DWnd::OnDrawGL()
{
#ifdef _DEBUG
   // TRACE("Entered Plot3DWnd::OnDrawGL... ");
	int rv;
	glGetIntegerv(GL_RENDER_MODE,&rv);
	CString mode;
	if (rv==GL_RENDER)
		mode="Render";
	else if (rv==GL_FEEDBACK)
		mode="Feedback";
	else
		mode="Unknown";
   // TRACE("mode = %s\n",mode);
#endif	

   glLoadIdentity();

   SetProjection( m_projType );

   glReal3 eye = m_eyeDist*m_eye + glReal3( 0.5, 0.5, 0.5 );
   gluLookAt( eye.x, eye.y, eye.z, 0.5, 0.5, 0.5, m_up.x, m_up.y, m_up.z );

   // Call Overidables
   DrawData();
   DrawFrame();

   // Draw Axes
   if ( m_frameType != P3D_FT_NONE )
      DrawAxes();

   if ( m_inMouseRotate && m_guideMouseRotate )
      {
      glPushAttrib( GL_POLYGON_BIT );
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      SetColorEmission( RGB(200,200,200) );
      glTranslated( 0.5, 0.5, 0.5 );
      glScaled( 2.0, 2.0, 2.0 );
      GLUquadric *pQuad = gluNewQuadric();
      gluQuadricOrientation( pQuad, GLU_OUTSIDE );
      gluSphere( pQuad, 0.5, 10, 10 );
      gluDeleteQuadric( pQuad );
      glPopAttrib();
      }
}

void Plot3DWnd::DrawFrame()
   {
   // Antialiasing
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST ); //GL_DONT_CARE);
   glLineWidth(1.0);

   glReal3 pt;
	glReal3 orgPt(0.0,0.0,0.0);

   switch (m_frameType)
      {
      case P3D_FT_NONE:
         break;
      case P3D_FT_DEFAULT:
         {		
         SetColorEmission( m_axesColor );

         glBegin(GL_LINES);
            pt=orgPt;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + I;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + J;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + K;
		      glVertex3d(pt.x,pt.y,pt.z);
         glEnd();

         glLineStipple(2,0xAAAA);
         glEnable(GL_LINE_STIPPLE);

         glBegin(GL_LINES);
            // X
            pt=orgPt + J;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + I;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + K;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + I;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + J + K;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + I;
		      glVertex3d(pt.x,pt.y,pt.z);

            // Y
            pt=orgPt + I;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + J;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + K;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + J;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + I + K;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + J;
		      glVertex3d(pt.x,pt.y,pt.z);

            // Z
            pt=orgPt + I;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + K;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + J;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + K;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + I + J;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + K;
		      glVertex3d(pt.x,pt.y,pt.z);

         glEnd();
         glDisable(GL_LINE_STIPPLE);
/*
         glBegin(GL_LINES);

         // main X
		   pt=orgPt;
		   pt = pt + glReal3(-0.1,0.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(1.2,0.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);

         // main Y
		   pt=orgPt;
		   pt = pt + glReal3(0.0,-0.1,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(0.0,1.2,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);

         // main Z
		   pt=orgPt;
		   pt = pt + glReal3(0.0,0.0,-0.1);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(0.0,0.0,1.2);
		   glVertex3d(pt.x,pt.y,pt.z);
		   glEnd();

		   glLineStipple(2,0xAAAA);
		   glEnable(GL_LINE_STIPPLE);
		   glBegin(GL_LINE_STRIP);
		   pt=orgPt;
		   pt = pt + glReal3(0.0,1.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(0.0,0.0,1.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(0.0,-1.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   glEnd();
		   glBegin(GL_LINE_STRIP);
		   pt=orgPt;
		   pt = pt + glReal3(1.0,0.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(0.0,0.0,1.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(-1.0,0.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   glEnd();
		   glBegin(GL_LINE_STRIP);
		   pt=orgPt;
		   pt = pt + glReal3(0.0,1.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(1.0,0.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(0.0,-1.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   glEnd();
		   glBegin(GL_LINE_STRIP);
		   pt=orgPt;
		   pt = pt + glReal3(1.0,1.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(0.0,0.0,1.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(0.0,-1.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   glEnd();
		   glBegin(GL_LINES);
		   pt=orgPt;
		   pt = pt + glReal3(0.0,1.0,1.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   pt = pt + glReal3(1.0,0.0,0.0);
		   glVertex3d(pt.x,pt.y,pt.z);
		   glEnd();
		   glDisable(GL_LINE_STIPPLE);
*/
         }
         break;

      case P3D_FT_BOXED:
         {
         SetColorEmission( m_axesColor );
		   glBegin(GL_LINES);

            // X
		      pt=orgPt;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + I;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + J;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + I;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + K;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + I;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + J + K;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + I;
		      glVertex3d(pt.x,pt.y,pt.z);

            // Y
		      pt=orgPt;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + J;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + I;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + J;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + K;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + J;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + I + K;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + J;
		      glVertex3d(pt.x,pt.y,pt.z);

            // Z
		      pt=orgPt;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + K;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + I;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + K;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + J;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + K;
		      glVertex3d(pt.x,pt.y,pt.z);

            pt=orgPt + I + J;
		      glVertex3d(pt.x,pt.y,pt.z);
		      pt = pt + K;
		      glVertex3d(pt.x,pt.y,pt.z);

		   glEnd();

         }
         break;

      case P3D_FT_PLANES:
         {
         
         // I
         if ( dot( I, m_eye ) > 0 )
            {
            glBegin( GL_LINE_LOOP );
               pt=orgPt;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + J;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + K;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt - J;
               glVertex3d(pt.x,pt.y,pt.z);
            glEnd();
            }
         else
            {
            glBegin( GL_LINE_LOOP );
               pt=orgPt + I;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + K;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + J;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt - K;
               glVertex3d(pt.x,pt.y,pt.z);
            glEnd();
            }

         // J
         if ( dot( J, m_eye ) > 0 )
            {
            glBegin( GL_LINE_LOOP );
               pt=orgPt;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + K;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + I;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt - K;
               glVertex3d(pt.x,pt.y,pt.z);
            glEnd();
            }
         else
            {
            glBegin( GL_LINE_LOOP );
               pt=orgPt + J;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + I;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + K;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt - I;
               glVertex3d(pt.x,pt.y,pt.z);
            glEnd();
            }

         // K
         if ( dot( K, m_eye ) > 0 )
            {
            glBegin( GL_LINE_LOOP );
               pt=orgPt;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + I;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + J;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt - I;
               glVertex3d(pt.x,pt.y,pt.z);
            glEnd();
            }
         else
            {
            glBegin( GL_LINE_LOOP );
               pt=orgPt + K;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + J;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt + I;
               glVertex3d(pt.x,pt.y,pt.z);
               pt = pt - J;
               glVertex3d(pt.x,pt.y,pt.z);
            glEnd();
            }
         }
         break;

      } // end switch (m_frameType)

   // Un-Antialiasing
   glDisable(GL_LINE_SMOOTH);
   glDisable(GL_BLEND);
   glLineWidth(1.3f);
   }

void Plot3DWnd::DrawData()
   {
   }

void Plot3DWnd::DrawAxes()
   {
   static glReal tickSize       = 0.03;
   static glReal tickLabelSize  = 0.07;
   static glReal labelSize      = 0.1;
   static glReal gutter         = 0.005;

   glReal3 anchor;
   glReal3 start;
   glReal3 end;

   // Antialiasing
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST ); //GL_DONT_CARE);
   glLineWidth(1.0);

   //bool           m_showTickLabels;
   //bool           m_showTicks;
   //bool           m_showAxesLabels;

   for ( int i=0; i<m_axisArray.GetCount(); i++ )
      {
      SetColorEmission( m_axesColor );

      Axis *pAxis = m_axisArray.GetAt(i);

      if ( !pAxis->IsVisible() )
         continue;

      pAxis->AdjustToEye( m_eye, m_up );
   
      if ( pAxis->GetStep() > 0.0 )
         {
         start = pAxis->GetLocation();
         for ( glReal x = pAxis->GetStart(); x < pAxis->GetMax() + pAxis->GetStep()/2; x+=pAxis->GetStep() )
            {
            end   = start + tickSize*pAxis->GetTickDir();
            
            //if (m_showTicks)
            //   {
               glBegin( GL_LINES );
                  glVertex3d( start.x, start.y, start.z );
                  glVertex3d( end.x, end.y, end.z );
               glEnd();
            //   }

            if ( pAxis->IsLabelTicks() )
               {
               end = end + ( gutter + tickLabelSize/2.0 )*pAxis->GetTickDir();
               CString label;
               label.Format( "%g", x );
               DrawText3D( label, end, tickLabelSize*pAxis->GetTextDown(), pAxis->GetNormal(), P3D_TXT_CENTER );
               }

            start = start + pAxis->GetStep()/( pAxis->GetMax() - pAxis->GetMin() )*pAxis->GetDirection();
            }

         if ( pAxis->IsLabelAxis() )
            {
            SetColorEmission( pAxis->GetLabelColor() );
            glReal offset = tickSize + tickLabelSize + 2*gutter + 0.5*labelSize;
            anchor = pAxis->GetLocation() + 0.5*pAxis->GetDirection() + offset*pAxis->GetTickDir();
            DrawText3D( pAxis->GetLabel(), anchor, labelSize*pAxis->GetTextDown(), pAxis->GetNormal(), P3D_TXT_CENTER );      
            }
         }
      
      //////////////////////////
      }

   //if ( true /*m_showAxesLabels*/ )
   //   {
   //   glReal offset = tickSize + tickLabelSize + 2*gutter + 0.5*labelSize;
   //   anchor = GetAxis( P3D_XAXIS )->GetLocation() + 0.5*GetAxis( XAXIS ).GetDirection() + offset*GetAxis( XAXIS ).GetTickDir();
   //   DrawText3D( "X Axis", anchor, labelSize*GetAxis( XAXIS ).GetTextDown(), GetAxis( XAXIS ).GetNormal(), P3D_TXT_CENTER );
   //   
   //   anchor = GetAxis( P3D_YAXIS )->GetLocation() + 0.5*GetAxis( YAXIS ).GetDirection() + offset*GetAxis( YAXIS ).GetTickDir();
   //   DrawText3D( "Y Axis", anchor, labelSize*GetAxis( YAXIS ).GetTextDown(), GetAxis( YAXIS ).GetNormal(), P3D_TXT_CENTER ); 
   //   
   //   anchor = GetAxis( P3D_ZAXIS )->GetLocation() + 0.5*GetAxis( ZAXIS ).GetDirection() + offset*GetAxis( ZAXIS ).GetTickDir();
   //   DrawText3D( "Z Axis", anchor, labelSize*GetAxis( ZAXIS ).GetTextDown(), GetAxis( ZAXIS ).GetNormal(), P3D_TXT_CENTER ); 
   //   }

   // Un-Antialiasing
   glDisable(GL_LINE_SMOOTH);
   glDisable(GL_BLEND);
   glLineWidth(1.3f);

   }

void Plot3DWnd::RealToLogical( glReal3 *pPoint )
   {
   // scall point to the [0.0, 0.5]^3 cube
   pPoint->x = ( pPoint->x - GetXMin() )/( GetXMax() - GetXMin() );
	pPoint->y = ( pPoint->y - GetYMin() )/( GetYMax() - GetYMin() );
	pPoint->z = ( pPoint->z - GetZMin() )/( GetZMax() - GetZMin() );
   }


bool Plot3DWnd::IsPointInPlot( glReal3 point )
{
   glReal ds = 0.0001;

   return      GetXMin() - ds < point.x 
			 &&                 point.x < GetXMax() + ds 
			 &&   GetYMin() - ds < point.y   
			 &&                 point.y < GetYMax() + ds
			 &&   GetZMin() - ds < point.z
			 &&                 point.z < GetZMax() + ds;
}

void Plot3DWnd::SetAxesVisible( bool visible )
   {
   GetAxis( P3D_XAXIS )->ShowAll( visible ); 
   GetAxis( P3D_YAXIS )->ShowAll( visible );
   GetAxis( P3D_ZAXIS )->ShowAll( visible );
   }

void Plot3DWnd::SetAxisBounds( glReal xMin, glReal xMax, glReal yMin, glReal yMax, glReal zMin, glReal zMax )
   {
   ASSERT( xMin < xMax );
   ASSERT( yMin < yMax );
   ASSERT( zMin < zMax );

   GetAxis( P3D_XAXIS )->SetExtents( xMin, xMax ); 
   GetAxis( P3D_YAXIS )->SetExtents( yMin, yMax );
   GetAxis( P3D_ZAXIS )->SetExtents( zMin, zMax );

   }

void Plot3DWnd::SetAxisTicks( glReal xTick, glReal yTick, glReal zTick, glReal xTickStart, glReal yTickStart, glReal zTickStart )
   { 
   GetAxis( P3D_XAXIS )->SetTicks( xTickStart, xTick );
   GetAxis( P3D_YAXIS )->SetTicks( yTickStart, yTick );
   GetAxis( P3D_ZAXIS )->SetTicks( zTickStart, zTick );
   }

void Plot3DWnd::SetAxisTicks( glReal xTick, glReal yTick, glReal zTick )
   {
   GetAxis( P3D_XAXIS )->SetTicks( GetXMin(), xTick );
   GetAxis( P3D_YAXIS )->SetTicks( GetYMin(), yTick );
   GetAxis( P3D_ZAXIS )->SetTicks( GetZMin(), zTick );
   }

void Plot3DWnd::SetAxesLabels( LPCTSTR xLabel, LPCTSTR yLabel, LPCTSTR zLabel )
   { 
   GetAxis( P3D_XAXIS )->SetLabel( xLabel ); 
   GetAxis( P3D_YAXIS )->SetLabel( yLabel ); 
   GetAxis( P3D_ZAXIS )->SetLabel( zLabel ); 
   }

void Plot3DWnd::SetAxesVisible2( bool visible )
   {
   GetAxis( P3D_XAXIS2 )->ShowAll( visible ); 
   GetAxis( P3D_YAXIS2 )->ShowAll( visible );
   GetAxis( P3D_ZAXIS2 )->ShowAll( visible );
   }

void Plot3DWnd::SetAxisBounds2( glReal xMin, glReal xMax, glReal yMin, glReal yMax, glReal zMin, glReal zMax )
   {
   ASSERT( xMin < xMax );
   ASSERT( yMin < yMax );
   ASSERT( zMin < zMax );

   GetAxis( P3D_XAXIS2 )->SetExtents( xMin, xMax ); 
   GetAxis( P3D_YAXIS2 )->SetExtents( yMin, yMax );
   GetAxis( P3D_ZAXIS2 )->SetExtents( zMin, zMax );

   }

void Plot3DWnd::SetAxisTicks2( glReal xTick, glReal yTick, glReal zTick, glReal xTickStart, glReal yTickStart, glReal zTickStart )
   { 
   GetAxis( P3D_XAXIS2 )->SetTicks( xTickStart, xTick );
   GetAxis( P3D_YAXIS2 )->SetTicks( yTickStart, yTick );
   GetAxis( P3D_ZAXIS2 )->SetTicks( zTickStart, zTick );
   }

void Plot3DWnd::SetAxisTicks2( glReal xTick, glReal yTick, glReal zTick )
   {
   GetAxis( P3D_XAXIS2 )->SetTicks( GetXMin(), xTick );
   GetAxis( P3D_YAXIS2 )->SetTicks( GetYMin(), yTick );
   GetAxis( P3D_ZAXIS2 )->SetTicks( GetZMin(), zTick );
   }

void Plot3DWnd::SetAxesLabels2( LPCTSTR xLabel, LPCTSTR yLabel, LPCTSTR zLabel )
   { 
   GetAxis( P3D_XAXIS2 )->SetLabel( xLabel ); 
   GetAxis( P3D_YAXIS2 )->SetLabel( yLabel ); 
   GetAxis( P3D_ZAXIS2 )->SetLabel( zLabel ); 
   }

void Plot3DWnd::SetLighting( bool turnOn )
   {
   BeginGLCommands();
   if ( turnOn )
      {
      //glPushMatrix();
      //glLoadIdentity();
      GLfloat light_position[4] = { 0.0, 0.0, 0.0, 1.0 };
      GLfloat white_light[4]    = { 1.0, 1.0, 1.0, 1.0 };
      //GLfloat offwhite_light[4] = { 0.95, 0.95, 0.95, 1.0 };
      GLfloat no_mat[]        = { 0.0, 0.0, 0.0, 1.0 };

      GLfloat lmodel_ambient[] = { 0.4f, 0.4f, 0.4f }; 

      glShadeModel(GL_SMOOTH);
      glLightfv(GL_LIGHT0, GL_POSITION, light_position);
      //glLightfv(GL_LIGHT0, GL_AMBIENT, no_mat);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
      glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient );

      glEnable(GL_LIGHTING);
      glEnable(GL_NORMALIZE);  // normalize normal vectors after scalling
      glEnable(GL_LIGHT0);     // turn on the light
      //glPopMatrix();
      }
   else  // Note: this doesnt work
      {
      glDisable(GL_LIGHTING);
      glDisable(GL_NORMALIZE);  // normalize normal vectors after scalling
      glDisable(GL_LIGHT0);
      }
   EndGLCommands();
   }

void Plot3DWnd::SetColorObject( COLORREF rgb )
   {
   BeginGLCommands();
      float r=float(GetRValue(rgb))/255.0f;
   	float g=float(GetGValue(rgb))/255.0f;
	   float b=float(GetBValue(rgb))/255.0f;

      //CString msg;
      //msg.Format("SetColorObject to (%f,%f,%f)\n", r, g, b );
      //TRACE(msg);

	   glColor3f(r,g,b);

      float adj = 0.2f;
      GLfloat mat_specular[]   = { 0.0, 0.0, 0.0, 1.0 }; //{ r+adj, g+adj, b+adj, 1.0f }; // { r/3.0, g/3.0, b/3.0, 1.0f }; //{ r+adj, g+adj, b+adj, 1.0f }; //{ (1.0+2.0*r)/2.0, (1.0+2.0*g)/2.0, (1.0+2.0*g)/2.0, 1.0f }; //{ 1.0f, 1.0f, 1.0f, 1.0f };
      GLfloat mat_diff[]   = { r, g, b, 1.0f };
      GLfloat mat_amb[]   = { r-adj, g-adj, b-adj, 1.0f }; // { r/2.0f, g/2.0f, b/2.0f, 1.0f };
      GLfloat no_mat[]        = { 0.0, 0.0, 0.0, 1.0 };
      GLfloat mat_shininess[]  = { 50.0f };  

      glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
      glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diff);
      glMaterialfv(GL_FRONT, GL_AMBIENT, mat_amb);
      glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
      glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

   EndGLCommands();
   }

void Plot3DWnd::SetColorEmission( COLORREF rgb )
   {
   BeginGLCommands();
      float r=float(GetRValue(rgb))/255.0f;
      float g=float(GetGValue(rgb))/255.0f;
      float b=float(GetBValue(rgb))/255.0f;

      glColor3f(r,g,b);

      GLfloat no_mat[]        = { 0.0, 0.0, 0.0, 1.0 };
      GLfloat mat_shininess[] = { 0.0 };
      GLfloat mat_emission[]  = { r, g, b, 1.0 };

      glMaterialfv(GL_FRONT, GL_SPECULAR, no_mat);
      glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
      glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, no_mat);
      glMaterialfv(GL_FRONT, GL_EMISSION, mat_emission);
   EndGLCommands();
   }

void Plot3DWnd::OnSizeGL(int cx, int cy)
{
   m_aspect = (double)cx/double(cy);
   SetProjection( m_projType );
   glViewport( 0, 0, cx, cy );
}

void Plot3DWnd::SetProjection(P3D_PROJECTION_TYPE type)
{
	m_projType=type;
	BeginGLCommands();
 	glPushMatrix();
		glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			if (m_projType==P3D_PT_ORTHO)	// ortho
				//glOrtho(-1,1,-1,1,2.0,10.);	// need BIGGER Frustum for ortho
            glOrtho( -1.0*m_aspect, 1.0*m_aspect, -1.0, 1.0, 2.0, 10.0 );
			else
            //gluPerspective(25,1,0.001,1500.0);
				//gluPerspective(25,1,0.05,15.0);
            gluPerspective(25,m_aspect,0.05,15.0);
		glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	EndGLCommands();
}

void Plot3DWnd::EnableClippingPlanes( bool enable )
   {
   static GLdouble xp[] = { -1.0, 0.0, 0.0, 1.0 };
   static GLdouble xm[] = { 1.0, 0.0, 0.0, 0.0 };

   static GLdouble yp[] = { 0.0, -1.0, 0.0, 1.0 };
   static GLdouble ym[] = { 0.0, 1.0, 0.0, 0.0 };

   static GLdouble zp[] = { 0.0, 0.0, -1.0, 1.0 };
   static GLdouble zm[] = { 0.0, 0.0, 1.0, 0.0 };

   if ( enable )
      {
      glPushMatrix();

         glClipPlane( GL_CLIP_PLANE0, xp );
         glClipPlane( GL_CLIP_PLANE1, xm );

         glClipPlane( GL_CLIP_PLANE2, yp );
         glClipPlane( GL_CLIP_PLANE3, ym );

         glClipPlane( GL_CLIP_PLANE4, zp );
         glClipPlane( GL_CLIP_PLANE5, zm );

         glEnable( GL_CLIP_PLANE0 );
         glEnable( GL_CLIP_PLANE1 );
         glEnable( GL_CLIP_PLANE2 );
         glEnable( GL_CLIP_PLANE3 );
         glEnable( GL_CLIP_PLANE4 );
         glEnable( GL_CLIP_PLANE5 );
      glPopMatrix();
      }
   else
      {
      glDisable( GL_CLIP_PLANE0 );
      glDisable( GL_CLIP_PLANE1 );
      glDisable( GL_CLIP_PLANE2 );
      glDisable( GL_CLIP_PLANE3 );
      glDisable( GL_CLIP_PLANE4 );
      glDisable( GL_CLIP_PLANE5 );
      }
   }

void Plot3DWnd::DrawShape( P3D_SHAPE_TYPE type, glReal3 point, int size )
   {

   RealToLogical( &point );

   glPushMatrix();

   glTranslated( point.x, point.y, point.z );
   glScaled( .005*size, .005*size, .005*size );

   switch ( type )
      {
      case P3D_ST_PIXEL:
         {
         glBegin(GL_POINTS);
            glVertex3d( 0.0, 0.0, 0.0 );  
         glEnd();
         }
         break;

      case P3D_ST_SPHERE:
         DrawSphere();
         break;

      case P3D_ST_CYLINDER:
         DrawCylinder();
         break;
         
      case P3D_ST_CONE:
         DrawCone();
         break;

         case P3D_ST_CUBE:
         DrawCube();
         break;

      case P3D_ST_ISOSAHEDRON:
         DrawIsosahedron();
         break;
      }

   glPopMatrix();
   }

void Plot3DWnd::DrawSphere( )
   {           
   GLUquadric *pQuad = gluNewQuadric();
   gluQuadricOrientation( pQuad, GLU_OUTSIDE );
   gluSphere( pQuad, 0.5, 15, 15 );
   gluDeleteQuadric( pQuad );
   }

void Plot3DWnd::DrawCylinder()
   {
   GLUquadric *pSide   = gluNewQuadric();
   GLUquadric *pBottom = gluNewQuadric();
   GLUquadric *pTop    = gluNewQuadric();

   gluQuadricOrientation( pSide, GLU_OUTSIDE );
   gluQuadricOrientation( pBottom, GLU_INSIDE );
   gluQuadricOrientation( pTop, GLU_OUTSIDE );
      
   gluDisk( pBottom, 0.0, 0.5, 15, 3 );
   gluCylinder( pSide, 0.5, 0.5, 1, 15, 15 );

   glTranslated( 0.0, 0.0, 1.0 );
   gluDisk( pTop, 0.0, 0.5, 15, 3 );

   gluDeleteQuadric( pSide );
   gluDeleteQuadric( pBottom );
   gluDeleteQuadric( pTop );

   glPopMatrix();
   }

void Plot3DWnd::DrawCone()
   {

   GLUquadric *pSide   = gluNewQuadric();
   GLUquadric *pBottom = gluNewQuadric();

   gluQuadricOrientation( pSide, GLU_OUTSIDE );
   gluQuadricOrientation( pBottom, GLU_INSIDE );

   gluDisk( pBottom, 0.0, 0.5, 15, 3 );
   gluCylinder( pSide, 0.5, 0.0, 1, 15, 15 );

   gluDeleteQuadric( pSide );
   gluDeleteQuadric( pBottom );
   }

void Plot3DWnd::DrawCube( )
   {
   static GLdouble verts[8][3] = {
         {0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0},
         {0.0, 0.0, 1.0 }, { 1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}
      };

   //-z
   glBegin( GL_POLYGON );
      glNormal3d( 0.0, 0.0, -1.0 );
      glVertex3dv( &verts[3][0] );
      glVertex3dv( &verts[2][0] );
      glVertex3dv( &verts[1][0] );
      glVertex3dv( &verts[0][0] );
   glEnd();

   //z
   glBegin( GL_POLYGON );
      glNormal3d( 0.0, 0.0, 1.0 );
      glVertex3dv( &verts[4][0] );
      glVertex3dv( &verts[5][0] );
      glVertex3dv( &verts[6][0] );
      glVertex3dv( &verts[7][0] );
   glEnd();

//-x
   glBegin( GL_POLYGON );
      glNormal3d( -1.0, 0.0, 0.0 );
      glVertex3dv( &verts[0][0] );
      glVertex3dv( &verts[4][0] );
      glVertex3dv( &verts[7][0] );
      glVertex3dv( &verts[3][0] );
   glEnd();

   //x
   glBegin( GL_POLYGON );
      glNormal3d( 1.0, 0.0, 0.0 );
      glVertex3dv( &verts[1][0] );
      glVertex3dv( &verts[2][0] );
      glVertex3dv( &verts[6][0] );
      glVertex3dv( &verts[5][0] );
   glEnd();

   //-y
   glBegin( GL_POLYGON );
      glNormal3d( 0.0, -1.0, 0.0 );
      glVertex3dv( &verts[0][0] );
      glVertex3dv( &verts[1][0] );
      glVertex3dv( &verts[5][0] );
      glVertex3dv( &verts[4][0] );
   glEnd();

   //y
   glBegin( GL_POLYGON );
      glNormal3d( 0.0, 1.0, 0.0 );
      glVertex3dv( &verts[2][0] );
      glVertex3dv( &verts[3][0] );
      glVertex3dv( &verts[7][0] );
      glVertex3dv( &verts[6][0] );
   glEnd();

   }

void Plot3DWnd::DrawIsosahedron()
   {
   static glReal X = 0.525731112119133606;
   static glReal Z = 0.850650808352039932;
   static glReal3 verts[12] = {
         glReal3(-X, 0.0, Z ), glReal3( X, 0.0, Z), glReal3(-X, 0.0, -Z), glReal3(X, 0.0, -Z),
         glReal3(0.0, Z, X), glReal3(0.0, Z, -X), glReal3(0.0, -Z, X), glReal3(0.0, -Z, -X),
         glReal3(Z, X, 0.0), glReal3(-Z, X, 0.0), glReal3(Z, -X, 0.0), glReal3(-Z, -X, 0.0)
      };
   static GLuint tindices[20][3] = {
         {1,4,0}, {4,9,0}, {4,5,9}, {8,5,4}, {1,8,4}, 
         {1,10,8}, {10,3,8}, {8,3,5}, {3,2,5}, {3,7,2},
         {3,10,7}, {10,6,7}, {6,11,7}, {6,0,11}, {6,1,0},
         {10,1,6}, {11,0,9}, {2,11,9}, {5,2,9}, {11,2,7}
      };

   //static GLuint tindices[20][3] = {
   //      {1,0,4}, {4,0,9}, {4,9,5}, {8,4,5}, {1,4,8}, 
   //      {1,8,10}, {10,8,3}, {8,5,3}, {3,5,2}, {3,2,7},
   //      {3,7,10}, {10,7,6}, {6,7,11}, {6,11,0}, {6,0,1},
   //      {10,6,1}, {11,9,0}, {2,9,11}, {5,9,2}, {11,7,2}
   //   };

   glScaled( 0.5,0.5,0.5 );

   glBegin(GL_TRIANGLES);
      glReal3 v1;
      glReal3 v2;
      glReal3 v3;
      glReal3 n;
      for ( int i=0; i<20; i++ )
         {
         v1 = verts[ tindices[i][0] ];
         v2 = verts[ tindices[i][1] ];
         v3 = verts[ tindices[i][2] ];
         n = normalize( cross(v1-v2,v2-v3) );
         glNormal3d( n.x, n.y, n.z );

         glVertex3d( v1.x, v1.y, v1.z );
         glVertex3d( v2.x, v2.y, v2.z );
         glVertex3d( v3.x, v3.y, v3.z );
         }
   glEnd();
   }

void Plot3DWnd::DrawPrism( glReal3 length, glReal3 width, glReal3 height )
   {

   glReal3 normal;
   glReal3 point;

   if ( height.z < 0.0 )
      return;

   //z
   normal = normalize( height );
   glBegin( GL_POLYGON );
      glNormal3d( normal.x, normal.y, normal.z );
      point = height;
      glVertex3d( point.x, point.y, point.z );
      point = point + length;
      glVertex3d( point.x, point.y, point.z );
      point = point + width;
      glVertex3d( point.x, point.y, point.z );
      point = point - length;
      glVertex3d( point.x, point.y, point.z );
   glEnd();

   // if the prisim is very short ( i.e. zero height ), just draw the top face
   if ( height.z < FLT_MIN )
      return;

   //-z
   normal = -normalize( height );
   glBegin( GL_POLYGON );
      glNormal3d( normal.x, normal.y, normal.z );
      point = glReal3( 0.0, 0.0, 0.0 );
      glVertex3d( point.x, point.y, point.z );
      point = point + width;
      glVertex3d( point.x, point.y, point.z );
      point = point + length;
      glVertex3d( point.x, point.y, point.z );
      point = point - width;
      glVertex3d( point.x, point.y, point.z );
   glEnd(); 

   //-x
   normal = -normalize( length );
   glBegin( GL_POLYGON );
      glNormal3d( normal.x, normal.y, normal.z );
      point = glReal3( 0.0, 0.0, 0.0 );
      glVertex3d( point.x, point.y, point.z );
      point = point + height;
      glVertex3d( point.x, point.y, point.z );
      point = point + width;
      glVertex3d( point.x, point.y, point.z );
      point = point - height;
      glVertex3d( point.x, point.y, point.z );
   glEnd();

   //x
   normal = normalize( length );
   glBegin( GL_POLYGON );
      glNormal3d( normal.x, normal.y, normal.z );
      point = length;
      glVertex3d( point.x, point.y, point.z );
      point = point + width;
      glVertex3d( point.x, point.y, point.z );
      point = point + height;
      glVertex3d( point.x, point.y, point.z );
      point = point - width;
      glVertex3d( point.x, point.y, point.z );
   glEnd();

   //-y
   normal = -normalize( width );
   glBegin( GL_POLYGON );
      glNormal3d( normal.x, normal.y, normal.z );
      point = glReal3( 0.0, 0.0, 0.0 );
      glVertex3d( point.x, point.y, point.z );
      point = point + length;
      glVertex3d( point.x, point.y, point.z );
      point = point + height;
      glVertex3d( point.x, point.y, point.z );
      point = point - length;
      glVertex3d( point.x, point.y, point.z );
   glEnd();

   //y
   normal = normalize( width );
   glBegin( GL_POLYGON );
      glNormal3d( normal.x, normal.y, normal.z );
      point = width;
      glVertex3d( point.x, point.y, point.z );
      point = point + height;
      glVertex3d( point.x, point.y, point.z );
      point = point + length;
      glVertex3d( point.x, point.y, point.z );
      point = point - height;
      glVertex3d( point.x, point.y, point.z );
   glEnd();
   }

void Plot3DWnd::DrawLine( glReal3 start, glReal3 end )
   {
   RealToLogical( &start );
   RealToLogical( &end );

   glBegin( GL_LINES );
      glVertex3d( start.x, start.y, start.z );
      glVertex3d( end.x, end.y, end.z );
   glEnd();
   }

void Plot3DWnd::DrawText3D( const char *str, glReal3 anchor, glReal3 downSize, glReal3 normal, int style )
   {
   glReal scale = abs( downSize );

   glReal3 initNorm( 0.0, 0.0, 1.0 );
   glReal3 initDown( 0.0, -1.0, 0.0 );
   
   glReal  thetaN = angle( initNorm, normal );  // radians
   glReal3 axisN;

   if ( thetaN < 179.0*pi/180.0 )  // treat any angle larger than this as if it is 180
      axisN = normalize( cross( initNorm, normal ) );
   else
      axisN = normalize( cross( downSize, normal ) ); // direction doesnt matter in the case of 180
      
   rotate( initDown, thetaN, axisN );

   thetaN *= 180.0/pi;   //convert to degrees
   glReal  thetaD = angle( initDown, downSize )*180.0/pi;
   glReal3 axisD;

   if ( thetaD < 179.0 )  // treat any angle larger than this as if it is 180
      axisD  = normalize( cross( initDown, downSize ) );
   else
      axisD = normal;  // direction doesnt matter in the case of 180

   glReal3 ext = GetText3DExtent( str );
   glReal3 nudge(0,0,0);

   if ( style & P3D_TXT_VCENTER )
      nudge = nudge + glReal3( 0.0, -ext.y/2.0, 0.0 );
   else if ( style & P3D_TXT_TOP )
      nudge = nudge + glReal3( 0.0, -ext.y, 0.0 );

   if ( style & P3D_TXT_HCENTER )
      nudge = nudge + glReal3( -ext.x/2.0, 0.0, 0.0 );
   else if ( style & P3D_TXT_RIGHT )
      nudge = nudge + glReal3( -ext.x, 0.0, 0.0 );

   glPushMatrix();
      
      glTranslated( anchor.x, anchor.y, anchor.z );      
      glRotated( thetaD, axisD.x, axisD.y, axisD.z );
      glRotated( thetaN, axisN.x, axisN.y, axisN.z );
      glScaled( scale, scale, scale );
      glTranslated( nudge.x, nudge.y, nudge.z );      
      
      PrintString3D( str );

   glPopMatrix();
   }

glReal3 Plot3DWnd::GetSphericalFromRectangular( glReal3 rect )
   {
   //glReal ro = s
   return NULLVECTOR;
   }

glReal3 Plot3DWnd::GetRectangularFromSpherical( glReal3 rect )
   {
   return NULLVECTOR;
   }

void Plot3DWnd::OnLButtonDown(UINT nFlags, CPoint point)
   {
   //TRACE("**********LButtonDown\n");

   if ( m_rotationType != P3D_RT_NONE )
      {
	   m_leftDownPos = point;
	   SetCapture();
	   //::SetCursor(LoadCursor(NULL, IDC_SIZEALL));
	   m_inMouseRotate=TRUE;
      }

   COpenGLWnd::OnLButtonDown(nFlags, point);
   }

void Plot3DWnd::OnLButtonUp(UINT nFlags, CPoint point)
   {
   //TRACE("**********LButtonUp\n");
   m_leftDownPos = CPoint(0,0);	// forget where we clicked
	//SetMouseCursor( AfxGetApp()->LoadStandardCursor(IDC_ARROW));
	ReleaseCapture();
	m_inMouseRotate=FALSE;

   Invalidate();

   COpenGLWnd::OnLButtonUp(nFlags, point);
   }

void Plot3DWnd::OnMouseMove(UINT nFlags, CPoint point)
   {
   if (m_inMouseRotate)
      {
      ASSERT(GetCapture()==this);

      glReal3 nEye = m_eye;
      glReal3 nUp  = m_up;

      CRect rect;
      GetClientRect( &rect );

      switch ( m_rotationType )
         {
         case P3D_RT_NONE:
            ASSERT(0);
            break;
         case P3D_RT_TRACKBALL:
            {
            DWORD word = (DWORD) GetKeyState( VK_CONTROL );
            if ( HIWORD(word) )
               {
               CPoint pt = rect.CenterPoint();
               glReal3 v1( (glReal)(m_leftDownPos.x-pt.x), (glReal)(m_leftDownPos.y-pt.y), 0.0 );
			      glReal3 v2( (glReal)(point.x-pt.x), (glReal)(point.y-pt.y), 0.0 );
      			
			      glReal theta  = angle( v1, v2 );
			      glReal3 tork  = cross( v1, v2 );
               glReal sign	= tork.z;
			      sign = sign / fabs( sign );
      			
			      if (theta > 0.001)
                  {
				      nUp = cos(-sign*theta)*nUp + sin(-sign*theta)*cross( nUp, nEye );
                  }
               }
            else
               {
		         //::SetCursor(LoadCursor(NULL, IDC_SIZEALL));
               glReal dx =  2.0*(glReal)( point.x - m_leftDownPos.x )/(glReal)rect.Height();
               glReal dy = -2.0*(glReal)( point.y - m_leftDownPos.y )/(glReal)rect.Height();

               glReal3 yRotAxis = cross( m_eye, m_up );

               // y mouse motion
               nEye = normalize( m_eye - dy*nUp );  
               //glReal ang = angle( nEye, m_eye );
               nUp = normalize( cross( cross(nEye,nUp), nEye) );
               //rotate( nUp, ang, yRotAxis );
               

               // x mouse motion
               glReal3 v = cross( nUp, nEye );
               nEye = normalize( nEye - dx*v ); 
               nUp  = normalize( cross( cross(nEye,nUp), nEye) );               
               }
            }
            break;

         case P3D_RT_MAPLE:
            {
            glReal dx = 4.0*(glReal)( point.x - m_leftDownPos.x )/(glReal)rect.Height();
            glReal dy = -4.0*(glReal)( point.y - m_leftDownPos.y )/(glReal)rect.Height();

            // y mouse rotation
            glReal3 yRotAxis = cross( m_eye, m_up );
            rotate( nUp, -dy, yRotAxis );
            rotate( nEye, -dy, yRotAxis );

            nUp  = normalize( cross( cross(nEye,nUp), nEye) );

            // x mouse motion
            rotate( nUp, -dx, K );
            rotate( nEye, -dx, K );

            nUp  = normalize( cross( cross(nEye,nUp), nEye) );

            
            }
            break;

         default:
            ASSERT(0);

         }  // end switch ( m_rotationType )

      m_eye = nEye;
      m_up  = nUp;
      m_leftDownPos = point;
      Invalidate( true );
      }

   COpenGLWnd::OnMouseMove(nFlags, point);
   }


void Plot3DWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
   {
   char lsChar;

   WORD ctrl;

	lsChar = char(nChar);
   ctrl = HIWORD( (DWORD) GetKeyState( VK_CONTROL ) );

   if ( nChar == VK_UP )
      {
      m_eyeDist -= 0.1;

      Invalidate();
      }

   if ( nChar == VK_DOWN )
      {
      m_eyeDist += 0.1;

      Invalidate();
      }

   if ( ctrl)
      {
      if ( lsChar == 'R' )
         {
         m_eye = normalize( glReal3( 1.0, 0.5, 0.5 ) );
         m_up  = cross( m_eye, glReal3( 0.0, 0.0, 1.0 ) );
         m_up  = normalize( cross( m_up, m_eye ) );
         m_eyeDist = 5.0;
         Invalidate();
         }

      if ( lsChar == 'E' )
         {
         CString msg;
         msg.Format( " eye = ( %f, %f, %f )\n up = ( %f, %f, %f )\n", m_eye.x, m_eye.y, m_eye.z, m_up.x, m_up.y, m_up.z  );
         MessageBox(msg);
         }
      }

   COpenGLWnd::OnKeyDown(nChar, nRepCnt, nFlags);
   }

BOOL Plot3DWnd::PreCreateWindow(CREATESTRUCT& cs)
   {
   
   cs.style |= WS_CLIPCHILDREN;

   return COpenGLWnd::PreCreateWindow(cs);
   }

void Plot3DWnd::OnRButtonDown(UINT nFlags, CPoint point)
   {
   ClientToScreen( &point );


   CMenu mainMenu;
   mainMenu.CreatePopupMenu(); 
   BuildPopupMenu( &mainMenu );

   mainMenu.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, this, NULL );


   COpenGLWnd::OnRButtonDown(nFlags, point);
   }

void Plot3DWnd::BuildPopupMenu( CMenu* pMenu )
   {
   UINT flag;
  
   // --- Axes (P3D_M_FRAME) --- 
   CMenu axesMenu;
   axesMenu.CreatePopupMenu();
   // none
   flag = MF_STRING;
   if ( m_frameType == P3D_FT_NONE ) 
      flag |= MF_CHECKED;
   axesMenu.AppendMenu( flag, P3D_FT_NONE, "None" );
   // P3D_FT_DEFAULT
   flag = MF_STRING;
   if ( m_frameType == P3D_FT_DEFAULT ) 
      flag |= MF_CHECKED;
   axesMenu.AppendMenu( flag, P3D_FT_DEFAULT, "Default" );
   // boxed
   flag = MF_STRING;
   if ( m_frameType == P3D_FT_BOXED ) 
      flag |= MF_CHECKED;
   axesMenu.AppendMenu( flag, P3D_FT_BOXED, "Boxed" );
   // P3D_FT_PLANES
   flag = MF_STRING;
   if ( m_frameType == P3D_FT_PLANES ) 
      flag |= MF_CHECKED;
   axesMenu.AppendMenu( flag, P3D_FT_PLANES, "Planes" );

   pMenu->AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) axesMenu.m_hMenu, "Axes" );

/* 
   // The method for this doesnt work.  Will try to fix later
   // Use Lighting P3D_M_LIGHTING
   //flag = MF_STRING;
   //if ( m_useLighting == true )
   //   flag |= MF_CHECKED;
   //pMenu->AppendMenu( flag, P3D_M_LIGHTING, "Use Lighting" );
*/

   // --- Projection Menu (P3D_M_PROJECTION) ---
   CMenu projectionMenu;
   projectionMenu.CreatePopupMenu();
   // ortho
   flag = MF_STRING;
   if ( GetProjectionType() == P3D_PT_ORTHO ) 
      flag |= MF_CHECKED;
   projectionMenu.AppendMenu( flag, P3D_PT_ORTHO, "Orthographic" );
   // perspective
   flag = MF_STRING;
   if ( GetProjectionType() == P3D_PT_PERSPECTIVE ) 
      flag |= MF_CHECKED;
   projectionMenu.AppendMenu( flag, P3D_PT_PERSPECTIVE, "Perspective" );
   pMenu->AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) projectionMenu.m_hMenu, "Projection" );

   // --- Rotation (P3D_M_ROTATION) ---
   CMenu rotationMenu;
   rotationMenu.CreatePopupMenu();
   // None
   flag = MF_STRING;
   if ( GetRotationType() == P3D_RT_NONE ) 
      flag |= MF_CHECKED;
   rotationMenu.AppendMenu( flag, P3D_RT_NONE, "None" );
   // TrackBall
   flag = MF_STRING;
   if ( GetRotationType() == P3D_RT_TRACKBALL ) 
      flag |= MF_CHECKED;
   rotationMenu.AppendMenu( flag, P3D_RT_TRACKBALL, "Track-Ball" );
   // Maple
   flag = MF_STRING;
   if ( GetRotationType() == P3D_RT_MAPLE ) 
      flag |= MF_CHECKED;
   rotationMenu.AppendMenu( flag, P3D_RT_MAPLE, "Maple" );
   pMenu->AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) rotationMenu.m_hMenu, "Mouse Rotation" );


   // The method for this doesnt work.  Will try to fix later
   //// --- Background Color Menu (P3D_M_BACKGROUND) ---
   //CMenu backgroundColorMenu;
   //backgroundColorMenu.CreatePopupMenu();
   //// Black
   //flag = MF_STRING;
   //if ( !m_whiteBackground ) 
   //   flag |= MF_CHECKED;
   //backgroundColorMenu.AppendMenu( flag, P3D_M_BACKGROUND, "Black" );
   //// White
   //flag = MF_STRING;
   //if ( m_whiteBackground ) 
   //   flag |= MF_CHECKED;
   //backgroundColorMenu.AppendMenu( flag, P3D_M_BACKGROUND+1, "White" );
   //pMenu->AppendMenu( MF_POPUP | MF_STRING, (UINT) backgroundColorMenu.m_hMenu, "Background Color" );
 
   //// --- P3D_M_SHOWTICKLABELS ---
   //flag = MF_STRING;
   //if ( GetShowTickLabels() == true )
   //   flag |= MF_CHECKED;
   //pMenu->AppendMenu( flag, LABEL_TICKS, "Label Ticks" );

   //// --- P3D_M_SHOWTICKS ---
   //flag = MF_STRING;
   //if ( GetShowAxesTicks() == true )
   //   flag |= MF_CHECKED;
   //pMenu->AppendMenu( flag, SHOW_AXES_TICKS, "Show Axes Ticks" );
   //
   //// --- P3D_M_SHOWAXESLABELS ---
   //flag = MF_STRING;
   //if ( GetShowAxesLabels() == true )
   //   flag |= MF_CHECKED;
   //pMenu->AppendMenu( flag, SHOW_AXES_TICKS, "Show Axes Ticks" );
   
   }


void Plot3DWnd::OnSelectOption( UINT nID )
   {

   // P3D_M_FRAME
   if ( P3D_M_FRAME <= nID && nID < P3D_M_FRAME + 100 )
      m_frameType = (P3D_FRAME_TYPE)nID;

   // P3D_M_PROJECTION
   if ( P3D_M_PROJECTION <= nID && nID < P3D_M_PROJECTION + 100 )
      m_projType = (P3D_PROJECTION_TYPE)nID;

   // P3D_M_ROTATION 
   if ( P3D_M_ROTATION <= nID && nID < P3D_M_ROTATION + 100 )
      {
      m_eye = normalize( glReal3( 1.0, 0.5, 0.5 ) + glReal3(0.5,0.5,0.5) );
      m_up  = normalize( cross( cross( m_eye, K ), m_eye ) );

      m_rotationType = (P3D_ROTATION_TYPE)nID;
      }

   //if ( abs( dot( K, m_up ) ) > 0.001 )
   //   normalize( cross( cross( m_eye, K ), m_eye ) );
   //SetRotationType( P3D_RT_MAPLE );

   switch (nID)
      {

      case P3D_M_LIGHTING:
         //m_useLighting = !m_useLighting;
         //if ( m_useLighting )
         //   {
         //   SetLighting( true );
         //   }
         //else
         //   {
         //   SetLighting( false );
         //   }
         break;

      case P3D_M_BACKGROUND:
         {
       //  TRACE("In ScatterPlot3DWnd: Changing Color\n");
       //  m_whiteBackground = false;
       //  m_axesColor       = RGB(255,255,255);
       //  m_backgroundColor = RGB(0,0,0);
       //  //SetClearCol( m_backgroundColor );

       //  float r=float(GetRValue(m_backgroundColor))/255;
	      //float g=float(GetGValue(m_backgroundColor))/255;
	      //float b=float(GetBValue(m_backgroundColor))/255;
	      //
	      //glClearColor(r,g,b,1.0f);
	      //
	      //Invalidate();
         }
         break;
      case P3D_M_BACKGROUND + 1:
         {
         //m_whiteBackground = true;
         //m_axesColor       = RGB(0,0,0);
         //m_backgroundColor = RGB(255,255,255);
         //SetClearCol( m_backgroundColor );
         }
         break;

      //case P3D_M_SHOWTICKLABELS:
      //   SetShowTickLabels( !GetShowTickLabels() );
      //   break;

      //case P3D_M_SHOWTICKS:
      //   SetShowTicks( !GetShowTicks() );
      //   break;
      //   
      //case P3D_M_SHOWAXESLABELS:     
      //   SetShowAxesLabels( !GetShowAxesLabels() );
         break;
       
      }
      
   Invalidate();
   }
