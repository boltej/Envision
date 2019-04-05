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
// AnalyticsViewer.cpp : implementation file
//

#include "stdafx.h"
#pragma hdrstop

#include "AnalyticsViewer.h"

#include <Report.h>
#include <UNITCONV.H>
#include <Maplayer.h>
#include <DeltaArray.h>

//#include "PointSet.h"
//#include "ShaderMgr.h"


// gl camera support
//#include <3rdParty\GL\glew.h>

//using namespace std;
//using namespace glm;

// AnalyticsViewer

IMPLEMENT_DYNAMIC(AnalyticsViewer, CWnd)

AnalyticsViewer::AnalyticsViewer()
// : m_camera()
// , m_hRC( 0 )
// , m_inDrag( false )
// , m_startXPos( 0 )
// , m_startYPos( 0 )
// , m_currXRot( 0 )
// , m_currYRot( 0 )
// , m_mwDelta( 0 )
// , m_pPointSet( NULL )
// , m_pShdrMgr( NULL )
   { }


AnalyticsViewer::~AnalyticsViewer()
   { 
//   if ( m_pPointSet != NULL )
//      delete m_pPointSet;
//
//   if ( m_pShdrMgr != NULL )
//      delete m_pShdrMgr;
   }


BEGIN_MESSAGE_MAP(AnalyticsViewer, CWnd)
   ON_WM_CREATE()
   ON_WM_PAINT()
   ON_WM_SIZE()
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_MOUSEWHEEL()
   ON_WM_MOUSEMOVE()
   ON_WM_DESTROY()
   ON_WM_KEYDOWN()
END_MESSAGE_MAP()


// AnalyticsViewer message handlers
int AnalyticsViewer::OnCreate(LPCREATESTRUCT lpCreateStruct)
   {
   if (CWnd::OnCreate(lpCreateStruct) == -1)
      return -1;

   Init( NULL, NULL );
   
   return 0;
   }


int AnalyticsViewer::Init( MapLayer *pLayer, DeltaArray *pDA )
   {
//   // initialize the viewer window
//   InitOpenGL();     //  create, set up an openGL context
//   
//   PopulateTestPointSet();
//   //PopulateSinglePointSet();
//   //PopulateFullGridPointSet( pLayer, pDA );   // creates points
//   m_pPointSet->PopulateAxesDataPoints();
//   m_pPointSet->UpdateOpenGLData();
//
//   // configure GPU storage for point set
//   m_pPointSet->InitOpenGL();
//
//   // load shader program
//   GLuint prog = m_pShdrMgr->LoadShaderProgramSet( "Points" );
//   m_pPointSet->SetDataShaderProgramIndex(prog);
//
//   // and axes program
//   GLuint progAxes = m_pShdrMgr->LoadShaderProgramSet( "Axes" );
//   m_pPointSet->SetAxesShaderProgramIndex( progAxes );
//
//   // set up initial camera position - look at the minimum of the data from
//   // twice the distance of the data
//   glm::vec3 minExtents, maxExtents;
//   m_pPointSet->GetExtents( minExtents, maxExtents );
//
//   m_cameraPos.x = (maxExtents.x - minExtents.x) + maxExtents.x;
//   m_cameraPos.y = (maxExtents.y - minExtents.y) + maxExtents.y;
//   m_cameraPos.z = (maxExtents.z - minExtents.z) + maxExtents.z;
//
//   m_lookAtPos.x = minExtents.x;
//   m_lookAtPos.y = minExtents.y; 
//   m_lookAtPos.z = minExtents.z;
//
//   float cameraDistance = sqrt(((m_cameraPos.x - m_lookAtPos.x)*(m_cameraPos.x - m_lookAtPos.x))
//	                          + ((m_cameraPos.y - m_lookAtPos.y)*(m_cameraPos.y - m_lookAtPos.y))
//	                          + ((m_cameraPos.z - m_lookAtPos.z)*(m_cameraPos.z - m_lookAtPos.z)));
//   // set up camera
//   m_camera.SetMode(FREE);		// perspective?
//   m_camera.SetPosition( m_cameraPos );
//   m_camera.SetLookAt( m_lookAtPos );
//   m_camera.SetClipping(1, cameraDistance*2);
//   m_camera.SetFOV(80);
//
//   // set up camera viewport
//   CRect rect;
//   GetClientRect( &rect );
//   m_camera.SetViewport( 0,0, rect.right, rect.bottom );
//
//   CString msg;
//   msg.Format( "Analytics Camera Params: Position=(%.1f,%.1f,%.1f), Distance=%.1f, LookAt=(%.1f,%.1f,%.1f)",
//         m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, cameraDistance, m_lookAtPos.x, m_lookAtPos.y, m_lookAtPos.z );
//   Report::Log( msg );
//
//   msg.Format( "Analytics Dataset: MinExtents=(%.1f,%.1f,%.1f), MaxExtents=(%.1f,%.1f,%.1f)",
//         minExtents[0], minExtents[1], minExtents[2], maxExtents[0], maxExtents[1], maxExtents[2] );
//   Report::Log( msg );
//
//   // refresh screen
//   RedrawWindow();
   return 0;
   }


//The following function is all the special setup stuff needed to prep OpenGL for VISTAS use
/*
void AnalyticsViewer::InitOpenGL()
   {
   if ( m_pShdrMgr != NULL )
      return;

   m_pShdrMgr = new ShaderMgr;

   //grab context
   HDC hDC = ::GetDC( this->GetSafeHwnd() );

   // The following structure defines attributes to produce an OGL context
   PIXELFORMATDESCRIPTOR pfd;
   ZeroMemory(&pfd,sizeof(pfd));
   pfd.nSize=sizeof(pfd);
   pfd.nVersion=1;
   pfd.dwFlags= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   pfd.iPixelType=PFD_TYPE_RGBA;
   pfd.cColorBits=24;
   pfd.cDepthBits=16;
   pfd.iLayerType=PFD_MAIN_PLANE;
   
   // ask Windows to give us the closest pixelformat to what is requested above
   int iFormat=ChoosePixelFormat( hDC, &pfd );

   // set pixel format for draw context
   SetPixelFormat( hDC, iFormat, &pfd );
   
   // create OpenGL Rendering context
   m_hRC = wglCreateContext( hDC );
   
   // initial flags
   wglMakeCurrent( hDC, m_hRC );

   // load signatures using GLEW
   glewExperimental = GL_TRUE; 
   GLenum err = glewInit();
   if (err != GLEW_OK)
      {
      //std::cout << "Error loading GLEW: " << glewGetErrorString(err) << std::endl;
      CString msg( "Error loading GLEW: " );
      msg += (PCTSTR) glewGetErrorString( err );
      Report::ErrorMsg( msg );
      }

   //enable depth test
   glEnable(GL_DEPTH_TEST);

   //enable variable point size
   glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
   ::ReleaseDC( this->GetSafeHwnd(), hDC );
   }


void AnalyticsViewer::PopulateTestPointSet()
   {
   if ( m_pPointSet != NULL )
      delete m_pPointSet;

   m_pPointSet = new PointSet;
   PointSet &ps = *m_pPointSet;

   ps.AllocatePoints( 5 );
   ps[0].x = 100.0;
   ps[0].y = 0.0;
   ps[0].z = 15.0;
   ps[0].r = 1.0;
   ps[0].g = 0.0;
   ps[0].b = 0.0;
   ps[0].a = 1;
   ps[0].scale = 1.0;
   
   ps[1].x = 100.0;
   ps[1].y = 15;
   ps[1].z = 40.0;
   ps[1].r = 0.0;
   ps[1].g = 1.0;
   ps[1].b = 0.0;
   ps[1].a = 1;
   ps[1].scale = 10.0;
   
   ps[2].x = 140.0;
   ps[2].y = 30.0;
   ps[2].z = 60.0;
   ps[2].r = 0.0;
   ps[2].g = 0.0;
   ps[2].b = 1.0;
   ps[3].a = 1;
   ps[2].scale = 5.0;
   
   ps[3].x = 160.0;
   ps[3].y = 80.0;
   ps[3].z = 20.0;
   ps[3].r = 1.0;
   ps[3].g = 0.4f;
   ps[3].b = 1.0;
   ps[3].a = 1;
   ps[3].scale = 1.0;
   
   ps[4].x = 0;
   ps[4].y = 0;
   ps[4].z = 0;
   ps[4].r = 1.0;
   ps[4].g = 1.0;
   ps[4].b = 1.0;
   ps[4].a = 1;
   ps[4].scale = 20.0;

   float xMin = ps[0].x;   float xMax = xMin;
   float yMin = ps[0].y;   float yMax = yMin;
   float zMin = ps[0].z;   float zMax = zMin;

   for (int i = 1; i < 5; i++)
      {
      if (ps[i].x < xMin) xMin = ps[i].x;
      if (ps[i].x > xMax) xMax = ps[i].x;

      if (ps[i].y < yMin) yMin = ps[i].y;
      if (ps[i].y > yMax) yMax = ps[i].y;

      if (ps[i].z < zMin) zMin = ps[i].z;
      if (ps[i].z > zMax) zMax = ps[i].z;
      }

   ps.SetExtents(glm::vec3(xMin, yMin, zMin), glm::vec3(xMax, yMax, zMax));

   //make sure new data is transferred over to GPU
   //ps.UpdateOpenGLData();
   }



void AnalyticsViewer::PopulateSinglePointSet()
   {
   if ( m_pPointSet != NULL )
      delete m_pPointSet;

   m_pPointSet = new PointSet;
   PointSet &ps = *m_pPointSet;

   ps.AllocatePoints( 1 );
   ps[0].x = 0.0f;
   ps[0].y = 0.0f;
   ps[0].z = 0.0f;
   ps[0].r = 0.5;
   ps[0].g = 0.5;
   ps[0].b = 0.5;
   ps[0].a = 1;
   ps[0].scale = 3.0;
   
   ps.SetExtents(glm::vec3(0,0,0), glm::vec3(1,1,1));

   //make sure new data is transferred over to GPU
   //ps.UpdateOpenGLData();
   }


void AnalyticsViewer::PopulateFullGridPointSet( MapLayer *pLayer, DeltaArray *pDA )
   {
   if ( m_pPointSet != NULL )
      delete m_pPointSet;

   m_pPointSet = new PointSet;
   PointSet &ps = *m_pPointSet;

   CArray<int, int> yearArray;

   for ( int i=0; i < 50; i++ )
      yearArray.Add( i );

   int years = 50; //pDA->GetYears( yearArray );
   int idus  = pLayer->GetRecordCount();

   INT_PTR pointCount = ((INT_PTR) idus) * years;
   
   m_pPointSet->AllocatePoints( (unsigned int) pointCount );

   int xMin=LONG_MAX, yMin=LONG_MAX, xMax=-LONG_MAX, yMax=-LONG_MAX;

   int zMin = yearArray[ 0 ];
   int zMax = yearArray[ yearArray.GetSize()-1 ];

   int index = 0;
   for (int idu = 0; idu < idus; idu++)
      {
      Poly *pPoly = pLayer->GetPolygon(idu);
      const Vertex &v = pPoly->GetCentroid();

      for ( int i= 0; i < years; i++ )
         {
         int year = yearArray[ i ];
      
         ps[index].x = (GLfloat) v.x;
         ps[index].z = (GLfloat) v.y;      // note:  GL coord system has Z pointing out the screen, so we switch here
         ps[index].y = (GLfloat) year;
         ps[index].r = 1.0;
         ps[index].g = 0.0;
         ps[index].b = 0.0;
         ps[index].a = 1;	// show by default
         ps[index].scale = 1.0;

         if ( v.x > xMax ) xMax = (int) v.x;
         if ( v.x < xMin ) xMin = (int) v.x;
         if ( v.y > yMax ) yMax = (int) v.y;
         if ( v.y < yMin ) yMin = (int) v.y;
         }  // end of: for each IDU
      }  // end of: for each year site-based strategies for optimizing fram management

   // update point set extents
   glm::vec3 minExtents( xMin, yMin, zMin );
   glm::vec3 maxExtents( xMax, yMax, zMax );
   ps.SetExtents( minExtents, maxExtents );

   CString msg;
   msg.Format( "Analytics: Loaded %i polygons, %i years, %iK data points - range=(%i,%i,%i)-(%i,%i,%i)", idus, years, int (pointCount/1000), xMin, yMin, zMin, xMax, yMax, zMax );
   Report::Log( msg );
   }
*/

void AnalyticsViewer::OnSize(UINT nType, int cx, int cy)
   {
   CWnd::OnSize(nType, cx, cy);
//   m_camera.SetViewport( 0, 0, cx, cy );
   }


void AnalyticsViewer::OnPaint()
   {
   CPaintDC dc(this); // device context for painting
   // TODO: Add your message handler code here
   // Do not call CWnd::OnPaint() for painting messages

   // Do OpenGL drawing
   // cache any current targeted contexts
   ////// HDC hOldGLDC = wglGetCurrentDC();
   ////// HGLRC hOldGLRC = wglGetCurrentContext();
   ////// 
   ////// HDC hDC= ::GetDC( this->GetSafeHwnd() );
   ////// 
   ////// //set our context as target for drawing; all OpenGL draw calls from this point
   ////// //on will go to our context until it is changed.
   ////// wglMakeCurrent( hDC, m_hRC);
   ////// 
   ////// // a little GL setup
   ////// glClearColor( 0,0,0, 1.0 );  // "background" color
   ////// glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );    // refresh
   ////// 
   ////// // prepare GL with proper mvp matrix from camera
   ////// m_camera.SetPosition( m_cameraPos );
   ////// m_camera.SetLookAt( m_lookAtPos );
   ////// m_camera.Update();      // this updates the camera's MVP matrix
   ////// 
   ////// glm::mat4 mvp = m_camera.MVP;  // projection* view * model;	//Compute the mvp matrix
   ////// 
   ////// //this is where draw code goes
   ////// if (m_pPointSet != NULL)
   //////    {
   //////    m_pPointSet->DrawPointSet( mvp );  
   //////    }
   ////// 
   ////// // draw axes
   ////// // OpenGL should always be double buffered if possible. Swap buffers to update after drawing
   ////// SwapBuffers( hDC );
   ////// 
   ////// CString msg;
   ////// msg.Format( "Camera: Position=(%.1f,%.1f,%.1f), LookAt=(%.1f,%.1f,%.1f)",
   //////       m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, m_lookAtPos.x, m_lookAtPos.y, m_lookAtPos.z );
   ////// 
   ////// //SetTextColor( hDC, RGB( 128, 128, 128) ); //RGB( 255, 255, 255 ));
   ////// //SetBkMode( hDC, TRANSPARENT );
   ////// //TextOut( hDC, 20, 20, (LPCTSTR) msg, msg.GetLength() );
   ////// Report::Log( msg );
   ////// 
   ////// //release draw context, restore previous contexts
   //::ReleaseDC( this->GetSafeHwnd(), hDC );
   ////// wglMakeCurrent(hOldGLDC,hOldGLRC);
   }


//void AnalyticsViewer::Rotate(float rotYAxis, float rotXAxis)
//   {
//   static const float DTOR = PI / 180.0f;
//   const float rotYTrans = rotYAxis * DTOR;
//   const float rotXTrans = rotXAxis * DTOR;
//
//   if ( m_pPointSet != NULL )
//      {
//      m_pPointSet->Rotate(rotXTrans,PointSet::X_AXIS);
//      m_pPointSet->Rotate(rotYTrans, PointSet::Y_AXIS);
//      }
//   }



void AnalyticsViewer::OnLButtonDown(UINT nFlags, CPoint point)
   {
   //m_inDrag = true;
   //m_startXPos = point.x;
   //m_startYPos = point.y;
   }


void AnalyticsViewer::OnLButtonUp(UINT nFlags, CPoint point)
   {
   //m_inDrag = false;

   CWnd::OnLButtonUp(nFlags, point);
   }


void AnalyticsViewer::OnMouseMove(UINT nFlags, CPoint point)
   {
   //if (m_inDrag )
   //   {
   //   int mXPos = point.x;
   //   int mYPos = point.y;
   //   //DoRotate(mXPos - startXPos, mYPos - startYPos);
   //   m_currXRot += mXPos - m_startXPos;
   //   m_currYRot += mYPos - m_startYPos;
   //   //Rotate( -m_currXRot, -m_currYRot);
   //   m_startXPos = mXPos;
   //   m_startYPos = mYPos;
   //
   //   RedrawWindow();
   //   }

   CWnd::OnMouseMove(nFlags, point);
   }


BOOL AnalyticsViewer::OnMouseWheel(UINT fFlags,short zDelta, CPoint point) 
   {
   //const float fraction = 0.01f;
   //const int cumNotchThreshold = 5;
   //
   //if ( zDelta > 0 )  // zooming in
   //   {
   //   m_mwDelta += zDelta;
   //
   //   if ( m_mwDelta > cumNotchThreshold )
   //      {
   //      ZoomCamera( -0.02f, true );
   //      m_mwDelta = 0;
   //      }
   //   }
   //
   //else if ( zDelta < 0 )  // zooming out
   //   {
   //   m_mwDelta -= zDelta;
   //
   //   if ( m_mwDelta > cumNotchThreshold )
   //      {
   //      ZoomCamera( 0.02f, true );
   //      glm::vec3 newPos;
   //      newPos.x = (m_cameraPos.x - fraction*m_lookAtPos.x )/(1-fraction);
   //      newPos.y = (m_cameraPos.y - fraction*m_lookAtPos.y )/(1-fraction);
   //      newPos.z = (m_cameraPos.z - fraction*m_lookAtPos.z )/(1-fraction);
   //      m_cameraPos = newPos;
   //
   //      RedrawWindow();
   //      m_mwDelta = 0;
   //      }
   //   }
   //
   return TRUE;
   }



void AnalyticsViewer::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
   {/*
   char lsChar = char(nChar);
   WORD ctrl = HIWORD( (DWORD) GetKeyState( VK_CONTROL ) );

   // CTL + Page Down
   // if ( ctrl && nChar == VK_NEXT )
   //    {
   //    //ChangeTabSelection( m_tabIndex + 1 );
   //    }
   // 
   // // CTL + Page Up
   // if ( ctrl && nChar == VK_PRIOR )
   //    {
   //    //ChangeTabSelection( m_tabIndex - 1 );
   //    }

   // toggle output window
   if (! ctrl )
      {
      
      switch( nChar )
         {
         case VK_LEFT:
            RotateCamera( AV_YAXIS, 0.1f, true );
            break;

         case VK_UP:
            RotateCamera( AV_XAXIS, 0.1f, true );
            break;

         case VK_RIGHT:
            RotateCamera( AV_YAXIS, -0.1f, true );
            break;

         case VK_DOWN:
            RotateCamera( AV_XAXIS, -0.1f, true );
            break;

         case 187:   // '+'
            OnMouseWheel( 0, 120, CPoint() );
            break;

         case 189:   // '-'
            OnMouseWheel( 0, -120, CPoint() );
            break;
         }
      }
   /////////////////////////////
   //// this is to debug whatever
   //if ( ctrl && lsChar == 'R' )
   */
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
   }


void AnalyticsViewer::OnDestroy()
   {
   //wglMakeCurrent(NULL,NULL);
   //wglDeleteContext( m_hRC );

   CWnd::OnDestroy();
   }


/*
void AnalyticsViewer::RotateCamera( AVAXIS axis, float radians, bool redraw )
   {
   // translate camera as if the lookAt position is on the origin
   glm::vec3 pos = m_cameraPos;
   pos.x -= m_lookAtPos.x;
   pos.y -= m_lookAtPos.y;
   pos.z -= m_lookAtPos.z;

   switch ( axis )
      {
      case AV_ZAXIS:
         {
         // compare to unit vector along x axis
         float dot = pos.x * 1 + pos.y * 0;
         float magnitude = sqrt( pos.x*pos.x + pos.y*pos.y );
         float cosAlpha = dot / magnitude;
         float alpha = acos( cosAlpha );         // this is the angle, in radians, between the existing camera position and and the unit vector aligned with the axis of rotation 

         alpha += radians;    // get a new angle
         pos.x = magnitude * cos( alpha );
         pos.y = magnitude * sin( alpha );

         m_cameraPos.x = pos.x + m_lookAtPos.x;
         m_cameraPos.y = pos.y + m_lookAtPos.y;
         m_cameraPos.z = pos.z + m_lookAtPos.z;
         }
         break;

      case AV_YAXIS:
         {
         // compare to unit vector along x axis
         float dot = pos.x * 1 + pos.z * 0;
         float magnitude = sqrt( pos.x*pos.x + pos.z*pos.z );
         float cosAlpha = dot / magnitude;
         float alpha = acos( cosAlpha );         // this is the angle, in radians, between the existing camera position and and the unit vector aligned with the axis of rotation 
      
         alpha += radians;    // get a new angle
         pos.x = magnitude * cos( alpha );
         pos.z = magnitude * sin( alpha );
      
         m_cameraPos.x = pos.x + m_lookAtPos.x;
         m_cameraPos.y = pos.y + m_lookAtPos.y;
         m_cameraPos.z = pos.z + m_lookAtPos.z;
         }
         break;

      case AV_XAXIS:
         {
         // compare to unit vector along x axis
         float dot = pos.y * 1 + pos.z * 0;
         float magnitude = sqrt( pos.y*pos.y + pos.z*pos.z );
         float cosAlpha = dot / magnitude;
         float alpha = acos( cosAlpha );         // this is the angle, in radians, between the existing camera position and and the unit vector aligned with the axis of rotation 
     
         alpha += radians;    // get a new angle
         pos.y = magnitude * cos( alpha );
         pos.z = magnitude * sin( alpha );
     
         m_cameraPos.x = pos.x + m_lookAtPos.x;
         m_cameraPos.y = pos.y + m_lookAtPos.y;
         m_cameraPos.z = pos.z + m_lookAtPos.z;
         }
         break;
      }
   if ( redraw )
      RedrawWindow();
   }


// positive fraction=zoom out, negative fraction=zoom in
void AnalyticsViewer::ZoomCamera( float fraction, bool redraw )
   {
   if ( fraction == 0 )
      return;

   glm::vec3 newPos;

   if ( fraction < 0 )  // zooming in
      {
      newPos.x = m_cameraPos.x + ( fraction * ( m_cameraPos.x - m_lookAtPos.x ) );
      newPos.y = m_cameraPos.y + ( fraction * ( m_cameraPos.y - m_lookAtPos.y ) );
      newPos.z = m_cameraPos.z + ( fraction * ( m_cameraPos.z - m_lookAtPos.z ) );
      }
   else // zooming out
      {
      newPos.x = ( m_cameraPos.x - fraction*m_lookAtPos.x ) / ( 1 - fraction );
      newPos.y = ( m_cameraPos.y - fraction*m_lookAtPos.y ) / ( 1 - fraction );
      newPos.z = ( m_cameraPos.z + fraction*m_lookAtPos.z ) / ( 1 + fraction );
      }

   m_cameraPos = newPos;

   if ( redraw )
      RedrawWindow();
   }


///////////////////////////////



//Keyboard input for camera, also handles exit case
//void AnalyticsViewer::KeyboardFunc(unsigned char c, int x, int y) 
//   {
//   switch (c) 
//      {
//      case 'w':
//         m_camera.Move(FORWARD);
//         break;
//      case 'a':
//         m_camera.Move(LEFT);
//         break;
//      case 's':
//         m_camera.Move(BACK);
//         break;
//      case 'd':
//         m_camera.Move(RIGHT);
//         break;
//      case 'q':
//         m_camera.Move(DOWN);
//         break;
//      case 'e':
//         m_camera.Move(UP);
//         break;
//      //case 'x':
//      //case 27:
//      //   exit(0);
//      //   return;
//      default:
//         break;
//      }
//   }

//void SpecialFunc(int c, int x, int y) {}
//void CallBackPassiveFunc(int x, int y) {}

//Used when person clicks mouse
//void AnalyticsViewer::CallBackMouseFunc(int button, int state, int x, int y) 
//   {
//   m_camera.SetPos(button, state, x, y);
//   }


//Used when person drags mouse around
//void AnalyticsViewer::CallBackMotionFunc(int x, int y)
//   {
//   m_camera.Move2D(x, y);
//   }

//Draw a wire cube! (nothing fancy here)
//void AnalyticsViewer::DisplayFunc() 
//   {
//   glEnable(GL_CULL_FACE);
//   glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glViewport(0, 0, m_windowSize.x, m_windowSize.y);
//
//   glm::mat4 model, view, projection;
//   m_camera.Update();
//   m_camera.GetMatricies(projection, view, model);
//
//   glm::mat4 mvp = projection* view * model;	//Compute the mvp matrix
//   glLoadMatrixf(glm::value_ptr(mvp));
//   glutWireCube(1);
//   glutSwapBuffers();
//   }


//Redraw based on fps set for window
//void AnalyticsViewer::TimerFunc(int value)
//   {
//   if ( m_windowHandle != -1) 
//      {
//      glutTimerFunc( m_windowInterval, TimerFunc, value);
//      glutPostRedisplay();
//      }
//   }
*/
/*
int main(int argc, char * argv[]) {
   //glut boilerplate
   //glutInit(&argc, argv);
   //glutInitWindowSize(1024, 512);
   //glutInitWindowPosition(0, 0);
   //glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
   //Setup window and callbacks
   
   
   window.window_handle = glutCreateWindow("MODERN_GL_CAMERA");
   glutReshapeFunc(ReshapeFunc);
   glutDisplayFunc(DisplayFunc);
   glutKeyboardFunc(KeyboardFunc);
   glutSpecialFunc(SpecialFunc);
   glutMouseFunc(CallBackMouseFunc);
   glutMotionFunc(CallBackMotionFunc);
   glutPassiveMotionFunc(CallBackPassiveFunc);
   glutTimerFunc(window.interval, TimerFunc, 0);

   glewExperimental = GL_TRUE;

   //if (glewInit() != GLEW_OK) {
   //   cerr << "GLEW failed to initialize." << endl;
   //   return 0;
   //   }
   //Setup camera
   //camera.SetMode(FREE);
   //camera.SetPosition(glm::vec3(0, 0, -1));
   //camera.SetLookAt(glm::vec3(0, 0, 0));
   //camera.SetClipping(.1, 1000);
   //camera.SetFOV(45);
   //Start the glut loop!
   glutMainLoop();
   return 0;
   }



   */
