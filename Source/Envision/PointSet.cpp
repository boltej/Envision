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
#include "stdafx.h"

#include <stdexcept>
//#include "GLErrStream.hpp"

#include <3rdParty/GL/glew.h>
#include <GL/gl.h>
#include <3rdParty/glm/glm.hpp>
#include <3rdParty/glm/gtc/matrix_transform.hpp>
#include <3rdParty/glm/gtc/type_ptr.hpp>

#include "PointSet.h"


#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))


PointSet::PointSet( void ) //const unsigned int & count)
   : m_shdrProgramData( 0 )
   , m_vaoData( 0 )
   , m_buffData( 0 )
   , m_shdrProgramAxes( 0 )
   , m_vaoAxes( 0 )
   , m_buffAxes( 0 )
   , m_numPoints( -1 )
   , m_dataPoints( NULL ) 
   {
   //_dataPoints = new DataPoint[_numPoints]();
   
   //test translate
   //m_modelMat[3][2] = -10.0;
   }


PointSet::~PointSet()
   {
   if ( m_dataPoints != NULL )
      delete[] m_dataPoints;

   //clean up opengl memory

   glDeleteBuffers(1, &m_buffData);
   glDeleteVertexArrays(1, &m_vaoData);

   glDeleteBuffers( 1, &m_buffAxes );
   glDeleteVertexArrays( 1, &m_vaoAxes );
   }


void PointSet::InitOpenGL()
   {
   //check to see if already initialized
   if (m_vaoData || m_buffData)
      return;

   // generate-bind-delete sequence

   //ASSERT_GL("PointSet::Init Pre");
   //construct buffers
   glGenVertexArrays(1, &m_vaoData); // collection of vertext array objects (collections of attributes, buffers, etc for drawing specific set of vertices
   glGenBuffers(1, &m_buffData);		// create buffer - ptrs to actual data on the GPU where geometry information goes

   //make space for data on buffer (data will be copied when UpdateGLData() is called)
   glBindVertexArray(m_vaoData);
   glBindBuffer(GL_ARRAY_BUFFER, m_buffData);
   glBufferData(GL_ARRAY_BUFFER, sizeof(DataPoint)*m_numPoints, m_dataPoints, GL_STATIC_DRAW);

   //enable vertex attribute
   glEnableVertexAttribArray(0);   // 0=geometry(xyz)
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DataPoint), BUFFER_OFFSET(0));

   //enable color attribute
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DataPoint), BUFFER_OFFSET(sizeof(GLfloat) * 3));

   //enable size attribute
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(DataPoint), BUFFER_OFFSET(sizeof(GLfloat) * 7));

   //clear buffers
   glBindBuffer(GL_ARRAY_BUFFER, 0);   // 0=unbind
   glBindVertexArray(0);			   // 0=unbind

   //---------------------------//
   //--------  Axes ----------- //
   //---------------------------//
   // repeat for axes data
   glGenVertexArrays( 1, &m_vaoAxes ); // collection of vertext array objects (collections of attributes, buffers, etc for drawing specific set of vertices
   glGenBuffers( 1, &m_buffAxes );		// create buffer - ptrs to actual data on the GPU where geometry information goes
   
   //make space for data on buffer (data will be copied when UpdateGLData() is called)
   glBindVertexArray( m_vaoAxes );
   glBindBuffer( GL_ARRAY_BUFFER, m_buffAxes );
   glBufferData( GL_ARRAY_BUFFER, sizeof( GLfloat )*36, m_axesData, GL_STATIC_DRAW );
   
   //enable vertex attribute
   glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), BUFFER_OFFSET( 0 ) );
   glEnableVertexAttribArray( 0 );   // 0=geometry(xyz)
   
   //enable color attribute
   glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), BUFFER_OFFSET( 3*sizeof(GLfloat) ) );
   glEnableVertexAttribArray( 1 );
   
   //clear buffers
   glBindBuffer( GL_ARRAY_BUFFER, 0 );   // 0=unbind
   glBindVertexArray( 0 );			   // 0=unbind
   //ASSERT_GL("PointSet::Init Post");
   }


void PointSet::UpdateOpenGLData()
   {
   if (! m_vaoData || ! m_buffData)
      return;
   
   // update buffer data for the points in the point cloud
   glBindVertexArray( m_vaoData );
   glBindBuffer(GL_ARRAY_BUFFER, m_buffData);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DataPoint)*m_numPoints, m_dataPoints);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   // axes data
   glBindVertexArray( m_vaoAxes );
   glBindBuffer( GL_ARRAY_BUFFER, m_buffAxes );
   glBufferSubData( GL_ARRAY_BUFFER, 0, 36*sizeof(GLfloat), m_axesData );
   
   glBindBuffer( GL_ARRAY_BUFFER, 0 );
   glBindVertexArray( 0 );
   }


DataPoint& PointSet::operator[](const unsigned int & idx)
   {
   if (idx >= m_numPoints)
      throw std::out_of_range("PointSet: index out of range.");

   return m_dataPoints[idx];
   }


void PointSet::DrawPointSet(glm::mat4 &mvp)
   {
   if ( m_numPoints <= 0 )
      return;

   //ASSERT_GL("PointSet::Draw Pre");

   glBindVertexArray( m_vaoData );			// bind vertex array for current program
   glBindBuffer(GL_ARRAY_BUFFER, m_buffData);

   glUseProgram( m_shdrProgramData );		// get shader program
   int mvpLoc = glGetUniformLocation( m_shdrProgramData, "mvpMat" );   // define entry points for various uniform variables (same across all shder program - globals)
   
   if (mvpLoc != -1)
      glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp)); // 

   glDrawArrays(GL_POINTS, 0, m_numPoints);  // actual draw points

   glUseProgram(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);
   //ASSERT_GL("PointSet::Draw Post");


   // draw axes next
   glBindVertexArray( m_vaoAxes );			// bind vertex array for current program
   glBindBuffer( GL_ARRAY_BUFFER, m_buffAxes );

   glUseProgram( m_shdrProgramAxes );		// get shader program
   mvpLoc = glGetUniformLocation( m_shdrProgramAxes, "mvpMat" );   // define entry points for various uniform variables (same across all shder program - globals)

   if ( mvpLoc != -1 )
      glUniformMatrix4fv( mvpLoc, 1, GL_FALSE, glm::value_ptr( mvp ) ); // 

   glEnable( GL_LINE_SMOOTH );
   glLineWidth( 0.5f );
   glDrawArrays( GL_LINES, 0, 3 );  // actually draw lines

   glUseProgram( 0 );
   glBindBuffer( GL_ARRAY_BUFFER, 0 );
   glBindVertexArray( 0 );
   }


void PointSet::GetExtents(glm::vec3 & minExts, glm::vec3 & maxExts)
   {
   for (int i = 0; i < AXIS_COUNT; i++)
      {
      minExts[i] = m_minExtents[i];
      maxExts[i] = m_maxExtents[i];
      }
   }


void PointSet::SetExtents(glm::vec3 & minExts, glm::vec3 & maxExts)
   {
   for( int i=0; i < AXIS_COUNT; i++ )
      {
      m_minExtents[ i ] = minExts[ i ];
      m_maxExtents[i] = maxExts[i];
      }
   }

#define ADINDEX( axis, pos ) (axis*6+pos)

bool PointSet::PopulateAxesDataPoints()
   {
   for ( int axis=0; axis < 3; axis++)
      {
      int start = axis*6;
      m_axesData[ start   ] = m_minExtents.x;
      m_axesData[ start+1 ] = m_minExtents.y;
      m_axesData[ start+2 ] = m_minExtents.z;

      m_axesData[ start+3 ] = 0.5f;    // red
      m_axesData[ start+4 ] = 0.5f;    // green
      m_axesData[ start+5 ] = 0.5f;    // blue
      }

   m_axesData[ ADINDEX( 1,0 ) ] = m_maxExtents.x;
   m_axesData[ ADINDEX( 3,1 ) ] = m_maxExtents.y;
   m_axesData[ ADINDEX( 5,2 ) ] = m_maxExtents.z;

   m_axesData[ ADINDEX( 1,3 ) ] = 1.0f;  // x axis, red
   m_axesData[ ADINDEX( 3,4 ) ] = 1.0f;  // y axis, green
   m_axesData[ ADINDEX( 5,5 ) ] = 1.0f;  // z axis, blue
   

   /*
   if ( m_axesPoints == NULL )
      m_axesPoints = new DataPoint[6];
      
   m_axesPoints[0].x = m_axesPoints[2].x = m_axesPoints[4].x = m_minExtents.x;
   m_axesPoints[0].y = m_axesPoints[2].y = m_axesPoints[4].y = m_minExtents.y;
   m_axesPoints[0].z = m_axesPoints[2].z = m_axesPoints[4].z = m_minExtents.z;
   m_axesPoints[0].r = m_axesPoints[2].r = m_axesPoints[4].r = 1.0f;
   m_axesPoints[0].g = m_axesPoints[2].g = m_axesPoints[4].g = 0.0f;
   m_axesPoints[0].b = m_axesPoints[2].b = m_axesPoints[4].b = 1.0f;
   m_axesPoints[0].a = m_axesPoints[2].a = m_axesPoints[4].a = 1;
   m_axesPoints[0].scale = m_axesPoints[2].scale = m_axesPoints[4].scale = 1.0f;

   // x-axis 
   m_axesPoints[1].x = m_maxExtents.x;
   m_axesPoints[1].y = m_minExtents.y;
   m_axesPoints[1].z = m_minExtents.z;
   m_axesPoints[1].r = 1.0f;
   m_axesPoints[1].g = 0.0f;
   m_axesPoints[1].b = 0.0f;
   m_axesPoints[1].a = 1;
   m_axesPoints[1].scale = 1.0f;

   // y-axis 
   m_axesPoints[3].x = m_minExtents.x;
   m_axesPoints[3].y = m_maxExtents.y;
   m_axesPoints[3].z = m_minExtents.z;
   m_axesPoints[3].r = 0.0f;
   m_axesPoints[3].g = 1.0f;
   m_axesPoints[3].b = 0.0f;
   m_axesPoints[3].a = 1;
   m_axesPoints[3].scale = 1.0f;

   // x-axis 
   m_axesPoints[5].x = m_minExtents.x;
   m_axesPoints[5].y = m_minExtents.y;
   m_axesPoints[5].z = m_maxExtents.z;
   m_axesPoints[5].r = 0.0f;
   m_axesPoints[5].g = 0.0f;
   m_axesPoints[5].b = 1.0f;
   m_axesPoints[5].a = 1;
   m_axesPoints[5].scale = 1.0f;
   */
   //make sure new data is transferred over to GPU
   //UpdateOpenGLData();
   return true;
   }
