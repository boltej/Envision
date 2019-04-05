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

//#include <3rdParty/glm/glm.hpp>
//#include <gl\GL.h>


struct DataPoint
   {
   // position
   GLfloat x, y, z;

   // color channel
   GLfloat r, g, b, a;

   // point size
   GLfloat scale;

   //GLint hide;

   DataPoint() 
      {
      x = y = z = r = g = b = 0.0f;
	   a = 1.0f;
      scale = 1.0;
      //hide = 0;
      }
   };


// a PointSet is:
//    1) an array of DataPoints,
//    2) model and projection transformation matrices,
//    3) some other GLU stuff? 

class PointSet
{
public:
   enum AXIS {X_AXIS=0, Y_AXIS, Z_AXIS, AXIS_COUNT};

   PointSet( void ); //const unsigned int & count);
   ~PointSet();

   int AllocatePoints( const unsigned int count )
      {
      m_numPoints = count; m_dataPoints = new DataPoint[count]; /*m_axesPoints = new DataPoint[6];*/ return count;
      }

   bool PopulateAxesDataPoints();


   DataPoint& operator[](const unsigned int & idx);

   void InitOpenGL();

   //call this anytime the data may have been changed
   void UpdateOpenGLData();

   void SetDataShaderProgramIndex( const GLuint & idx ) { m_shdrProgramData = idx; }
   void SetAxesShaderProgramIndex( const GLuint & idx ) { m_shdrProgramAxes = idx; }


   void DrawPointSet( glm::mat4 &mvp );

   //void UpdateProjectionMatrix(float width, float height);
   //
   //void Rotate(float angle, const AXIS & rotAxis);
   //void Translate(const glm::vec3 & move);
   //void Recenter(const glm::vec3 & newPos);
   void SetExtents(glm::vec3 & minExts, glm::vec3 & maxExts);
   void GetExtents(glm::vec3 & minExts, glm::vec3 & maxExts);

private:
   GLuint m_shdrProgramData;
   GLuint m_vaoData;              // vertex array object for point data
   GLuint m_buffData;

   GLuint m_shdrProgramAxes;
   GLuint m_vaoAxes;              // vertex array object for axes data
   GLuint m_buffAxes;


   unsigned int m_numPoints;
   DataPoint   *m_dataPoints;

   GLfloat m_axesData[ 36 ];
   //DataPoint   *m_axesPoints;

   //transformation matrices
   //glm::mat4 m_modelMat;
   //glm::mat4 m_viewMat;
   //glm::mat4 m_projMat;
public:
   glm::vec3 m_minExtents;   //x=idu centroid.x, y=idu centroid.y, z=year
   glm::vec3 m_maxExtents;  

   //GLfloat m_lastRot[AXIS_COUNT];
};

